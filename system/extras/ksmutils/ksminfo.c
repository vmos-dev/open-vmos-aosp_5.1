/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <pagemap/pagemap.h>

#define MAX_FILENAME  64

#define GROWTH_FACTOR 10

#define NO_PATTERN    0x100

#define PR_SORTED       1
#define PR_VERBOSE      2
#define PR_ALL          4

struct vaddr {
    unsigned long addr;
    size_t num_pages;
    pid_t pid;
};

struct ksm_page {
    uint64_t count;
    uint32_t hash;
    struct vaddr *vaddr;
    size_t vaddr_len, vaddr_size;
    size_t vaddr_count;
    uint16_t pattern;
};

struct ksm_pages {
    struct ksm_page *pages;
    size_t len, size;
};

static void usage(char *myname);
static int getprocname(pid_t pid, char *buf, int len);
static int read_pages(struct ksm_pages *kp, pm_map_t **maps, size_t num_maps, uint8_t pr_flags);
static void print_pages(struct ksm_pages *kp, uint8_t pr_flags);
static void free_pages(struct ksm_pages *kp, uint8_t pr_flags);
static bool is_pattern(uint8_t *data, size_t len);
static int cmp_pages(const void *a, const void *b);
extern uint32_t hashword(const uint32_t *, size_t, int32_t);

int main(int argc, char *argv[]) {
    pm_kernel_t *ker;
    pm_process_t *proc;
    pid_t *pids;
    size_t num_procs;
    size_t i;
    pm_map_t **maps;
    size_t num_maps;
    char cmdline[256]; // this must be within the range of int
    int error;
    int rc = EXIT_SUCCESS;
    uint8_t pr_flags = 0;
    struct ksm_pages kp;

    memset(&kp, 0, sizeof(kp));

    opterr = 0;
    do {
        int c = getopt(argc, argv, "hvsa");
        if (c == -1)
            break;

        switch (c) {
            case 'a':
                pr_flags |= PR_ALL;
                break;
            case 's':
                pr_flags |= PR_SORTED;
                break;
            case 'v':
                pr_flags |= PR_VERBOSE;
                break;
            case 'h':
                usage(argv[0]);
                exit(EXIT_SUCCESS);
            case '?':
                fprintf(stderr, "unknown option: %c\n", optopt);
                usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    } while (1);

    error = pm_kernel_create(&ker);
    if (error) {
        fprintf(stderr, "Error creating kernel interface -- "
                        "does this kernel have pagemap?\n");
        exit(EXIT_FAILURE);
    }

    if (pr_flags & PR_ALL) {
        error = pm_kernel_pids(ker, &pids, &num_procs);
        if (error) {
            fprintf(stderr, "Error listing processes.\n");
            exit(EXIT_FAILURE);
        }
    } else {
        if (optind != argc - 1) {
            usage(argv[0]);
            exit(EXIT_FAILURE);
        }

        pids = malloc(sizeof(*pids));
        if (pids == NULL) {
           fprintf(stderr, "Error allocating pid memory\n");
           exit(EXIT_FAILURE);
        }

        *pids = strtoul(argv[optind], NULL, 10);
        if (*pids == 0) {
            fprintf(stderr, "Invalid PID\n");
            rc = EXIT_FAILURE;
            goto exit;
        }
        num_procs = 1;
        if (getprocname(*pids, cmdline, sizeof(cmdline)) < 0) {
            cmdline[0] = '\0';
        }
        printf("%s (%u):\n", cmdline, *pids);
    }

    printf("Warning: this tool only compares the KSM CRCs of pages, there is a chance of "
            "collisions\n");

    for (i = 0; i < num_procs; i++) {
        error = pm_process_create(ker, pids[i], &proc);
        if (error) {
            fprintf(stderr, "warning: could not create process interface for %d\n", pids[i]);
            rc = EXIT_FAILURE;
            goto exit;
        }

        error = pm_process_maps(proc, &maps, &num_maps);
        if (error) {
            pm_process_destroy(proc);
            fprintf(stderr, "warning: could not read process map for %d\n", pids[i]);
            rc = EXIT_FAILURE;
            goto exit;
        }

        if (read_pages(&kp, maps, num_maps, pr_flags) < 0) {
            free(maps);
            pm_process_destroy(proc);
            rc = EXIT_FAILURE;
            goto exit;
        }

        free(maps);
        pm_process_destroy(proc);
    }

    if (pr_flags & PR_SORTED) {
        qsort(kp.pages, kp.len, sizeof(*kp.pages), cmp_pages);
    }
    print_pages(&kp, pr_flags);

exit:
    free_pages(&kp, pr_flags);
    free(pids);
    return rc;
}

static int read_pages(struct ksm_pages *kp, pm_map_t **maps, size_t num_maps, uint8_t pr_flags) {
    size_t i, j, k;
    size_t len;
    uint64_t *pagemap;
    size_t map_len;
    uint64_t flags;
    pm_kernel_t *ker;
    int error;
    unsigned long vaddr;
    int fd;
    off_t off;
    char filename[MAX_FILENAME];
    uint32_t *data;
    uint32_t hash;
    int rc = 0;
    struct ksm_page *cur_page;
    pid_t pid;

    if (num_maps == 0)
        return 0;

    pid = pm_process_pid(maps[0]->proc);
    ker = maps[0]->proc->ker;
    error = snprintf(filename, MAX_FILENAME, "/proc/%d/mem", pid);
    if (error < 0 || error >= MAX_FILENAME) {
        return -1;
    }

    data = malloc(pm_kernel_pagesize(ker));
    if (data == NULL) {
        fprintf(stderr, "warning: not enough memory to malloc data buffer\n");
        return -1;
    }

    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "warning: could not open %s\n", filename);
        rc = -1;
        goto err_open;
    }

    for (i = 0; i < num_maps; i++) {
        error = pm_map_pagemap(maps[i], &pagemap, &map_len);
        if (error) {
            fprintf(stderr, "warning: could not read the pagemap of %d\n",
                    pm_process_pid(maps[i]->proc));
            continue;
        }
        for (j = 0; j < map_len; j++) {
            error = pm_kernel_flags(ker, PM_PAGEMAP_PFN(pagemap[j]), &flags);
            if (error) {
                fprintf(stderr, "warning: could not read flags for pfn at address 0x%016" PRIx64 "\n",
                        pagemap[i]);
                continue;
            }
            if (!(flags & PM_PAGE_KSM)) {
                continue;
            }
            vaddr = pm_map_start(maps[i]) + j * pm_kernel_pagesize(ker);
            off = lseek(fd, vaddr, SEEK_SET);
            if (off == (off_t)-1) {
                fprintf(stderr, "warning: could not lseek to 0x%08lx\n", vaddr);
                continue;
            }
            len = read(fd, data, pm_kernel_pagesize(ker));
            if (len != pm_kernel_pagesize(ker)) {
                fprintf(stderr, "warning: could not read page at 0x%08lx\n", vaddr);
                continue;
            }

            hash = hashword(data, pm_kernel_pagesize(ker) / sizeof(*data), 17);

            for (k = 0; k < kp->len; k++) {
                if (kp->pages[k].hash == hash) break;
            }

            if (k == kp->len) {
                if (kp->len == kp->size) {
                    struct ksm_page *tmp = realloc(kp->pages,
                            (kp->size + GROWTH_FACTOR) * sizeof(*kp->pages));
                    if (tmp == NULL) {
                        fprintf(stderr, "warning: not enough memory to realloc pages struct\n");
                        free(pagemap);
                        rc = -1;
                        goto err_realloc;
                    }
                    memset(&tmp[k], 0, sizeof(tmp[k]) * GROWTH_FACTOR);
                    kp->pages = tmp;
                    kp->size += GROWTH_FACTOR;
                }
                rc = pm_kernel_count(ker, PM_PAGEMAP_PFN(pagemap[j]), &kp->pages[kp->len].count);
                if (rc) {
                    fprintf(stderr, "error reading page count\n");
                    free(pagemap);
                    goto err_count;
                }
                kp->pages[kp->len].hash = hash;
                kp->pages[kp->len].pattern =
                        is_pattern((uint8_t *)data, pm_kernel_pagesize(ker)) ?
                        (data[0] & 0xFF) : NO_PATTERN;
                kp->len++;
            }

            cur_page = &kp->pages[k];

            if (pr_flags & PR_VERBOSE) {
                if (cur_page->vaddr_len > 0 &&
                        cur_page->vaddr[cur_page->vaddr_len - 1].pid == pid &&
                        cur_page->vaddr[cur_page->vaddr_len - 1].addr ==
                        vaddr - (cur_page->vaddr[cur_page->vaddr_len - 1].num_pages *
                        pm_kernel_pagesize(ker))) {
                    cur_page->vaddr[cur_page->vaddr_len - 1].num_pages++;
                } else {
                    if (cur_page->vaddr_len == cur_page->vaddr_size) {
                        struct vaddr *tmp = realloc(cur_page->vaddr,
                                (cur_page->vaddr_size + GROWTH_FACTOR) * sizeof(*(cur_page->vaddr)));
                        if (tmp == NULL) {
                            fprintf(stderr, "warning: not enough memory to realloc vaddr array\n");
                            free(pagemap);
                            rc = -1;
                            goto err_realloc;
                        }
                        memset(&tmp[cur_page->vaddr_len], 0, sizeof(tmp[cur_page->vaddr_len]) * GROWTH_FACTOR);
                        cur_page->vaddr = tmp;
                        cur_page->vaddr_size += GROWTH_FACTOR;
                    }
                    cur_page->vaddr[cur_page->vaddr_len].addr = vaddr;
                    cur_page->vaddr[cur_page->vaddr_len].num_pages = 1;
                    cur_page->vaddr[cur_page->vaddr_len].pid = pid;
                    cur_page->vaddr_len++;
                }
            }
            cur_page->vaddr_count++;
        }
        free(pagemap);
    }
    goto no_err;

err_realloc:
err_count:
    if (pr_flags & PR_VERBOSE) {
        for (i = 0; i < kp->len; i++) {
            free(kp->pages[i].vaddr);
        }
    }
    free(kp->pages);

no_err:
    close(fd);
err_open:
    free(data);
    return rc;
}

static void print_pages(struct ksm_pages *kp, uint8_t pr_flags) {
    size_t i, j, k;
    char suffix[13];
    int index;

    for (i = 0; i < kp->len; i++) {
        if (kp->pages[i].pattern != NO_PATTERN) {
            printf("0x%02x byte pattern: ", kp->pages[i].pattern);
        } else {
            printf("KSM CRC 0x%08x:", kp->pages[i].hash);
        }
        printf(" %4zu page", kp->pages[i].vaddr_count);
        if (kp->pages[i].vaddr_count > 1) {
            printf("s");
        }
        if (!(pr_flags & PR_ALL)) {
            printf(" (%" PRIu64 " reference", kp->pages[i].count);
            if (kp->pages[i].count > 1) {
                printf("s");
            }
            printf(")");
        }
        printf("\n");

        if (pr_flags & PR_VERBOSE) {
            j = 0;
            while (j < kp->pages[i].vaddr_len) {
                printf("                   ");
                for (k = 0; k < 8 && j < kp->pages[i].vaddr_len; k++, j++) {
                    printf(" 0x%08lx", kp->pages[i].vaddr[j].addr);

                    index = snprintf(suffix, sizeof(suffix), ":%zu",
                            kp->pages[i].vaddr[j].num_pages);
                    if (pr_flags & PR_ALL) {
                        index += snprintf(suffix + index, sizeof(suffix) - index, "[%d]",
                                kp->pages[i].vaddr[j].pid);
                    }
                    printf("%-12s", suffix);
                }
                printf("\n");
            }
        }
    }
}

static void free_pages(struct ksm_pages *kp, uint8_t pr_flags) {
    size_t i;

    if (pr_flags & PR_VERBOSE) {
        for (i = 0; i < kp->len; i++) {
            free(kp->pages[i].vaddr);
        }
    }
    free(kp->pages);
}

static void usage(char *myname) {
    fprintf(stderr, "Usage: %s [-s | -v | -a | -h ] <pid>\n"
                    "    -s  Sort pages by usage count.\n"
                    "    -v  Verbose: print virtual addresses.\n"
                    "    -a  Display all the KSM pages in the system. Ignore the pid argument.\n"
                    "    -h  Display this help screen.\n",
    myname);
}

static int cmp_pages(const void *a, const void *b) {
    const struct ksm_page *pg_a = a;
    const struct ksm_page *pg_b = b;
    int cmp = pg_b->vaddr_count - pg_a->vaddr_count;

    return cmp ? cmp : pg_b->count - pg_a->count;
}

static bool is_pattern(uint8_t *data, size_t len) {
    size_t i;
    uint8_t first_byte = data[0];

    for (i = 1; i < len; i++) {
        if (first_byte != data[i]) return false;
    }

    return true;
}

/*
 * Get the process name for a given PID. Inserts the process name into buffer
 * buf of length len. The size of the buffer must be greater than zero to get
 * any useful output.
 *
 * Note that fgets(3) only declares length as an int, so our buffer size is
 * also declared as an int.
 *
 * Returns 0 on success, a positive value on partial success, and -1 on
 * failure. Other interesting values:
 *   1 on failure to create string to examine proc cmdline entry
 *   2 on failure to open proc cmdline entry
 *   3 on failure to read proc cmdline entry
 */
static int getprocname(pid_t pid, char *buf, int len) {
    char *filename;
    FILE *f;
    int rc = 0;
    static const char* unknown_cmdline = "<unknown>";

    if (len <= 0) {
        return -1;
    }

    if (asprintf(&filename, "/proc/%d/cmdline", (int)pid) < 0) {
        rc = 1;
        goto exit;
    }

    f = fopen(filename, "r");
    if (f == NULL) {
        rc = 2;
        goto releasefilename;
    }

    if (fgets(buf, len, f) == NULL) {
        rc = 3;
        goto closefile;
    }

closefile:
    (void) fclose(f);
releasefilename:
    free(filename);
exit:
    if (rc != 0) {
        /*
         * The process went away before we could read its process name. Try
         * to give the user "<unknown>" here, but otherwise they get to look
         * at a blank.
         */
        if (strlcpy(buf, unknown_cmdline, (size_t)len) >= (size_t)len) {
            rc = 4;
        }
    }

    return rc;
}

