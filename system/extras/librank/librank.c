/*
 * Copyright (C) 2008 The Android Open Source Project
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

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <pagemap/pagemap.h>

#define MAX_CMDLINE 256

struct process_info {
    pid_t pid;
    char cmdline[MAX_CMDLINE];
};

struct mapping_info {
    struct process_info *proc;
    pm_memusage_t usage;
};

struct library_info {
    struct library_info *next;
    char *name;
    struct mapping_info **mappings;
    size_t mappings_count;
    size_t mappings_size;
    pm_memusage_t total_usage;
};

static void usage(char *myname);
static int getprocname(pid_t pid, char *buf, size_t len);
static int numcmp(long long a, long long b);
static int licmp(const void *a, const void *b);

char *library_name_blacklist[] = { "[heap]", "[stack]", "", NULL };

#define declare_sort(field) \
    static int sort_by_ ## field (const void *a, const void *b)

declare_sort(vss);
declare_sort(rss);
declare_sort(pss);
declare_sort(uss);
declare_sort(swap);

#define INIT_LIBRARIES 16
#define INIT_MAPPINGS 4

static int order;

struct library_info **libraries;
size_t libraries_count;
size_t libraries_size;

struct library_info *get_library(const char *name, bool all) {
    size_t i;
    struct library_info *library;

    if (!all) {
        for (i = 0; library_name_blacklist[i]; i++)
            if (!strcmp(name, library_name_blacklist[i]))
                return NULL;
    } else {
        if (name[0] == 0) {
            name = "[anon]";
        }
    }

    for (i = 0; i < libraries_count; i++) {
        if (!strcmp(libraries[i]->name, name))
            return libraries[i];
    }

    if (libraries_count >= libraries_size) {
        libraries = realloc(libraries, 2 * libraries_size * sizeof(struct library_info *));
        if (!libraries) {
            fprintf(stderr, "Couldn't resize libraries array: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        libraries_size = 2 * libraries_size;
    }

    library = calloc(1, sizeof(*library));
    if (!library) {
        fprintf(stderr, "Couldn't allocate space for library struct: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    library->name = malloc(strlen(name) + 1);
    if (!library->name) {
        fprintf(stderr, "Couldn't allocate space for library name: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    strcpy(library->name, name);
    library->mappings = malloc(INIT_MAPPINGS * sizeof(struct mapping_info *));
    if (!library->mappings) {
        fprintf(stderr, "Couldn't allocate space for library mappings array: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    library->mappings_count = 0; library->mappings_size = INIT_MAPPINGS;
    pm_memusage_zero(&library->total_usage);

    libraries[libraries_count++] = library;

    return library;
}

struct mapping_info *get_mapping(struct library_info *library, struct process_info *proc) {
    struct mapping_info *mapping;
    size_t i;

    for (i = 0; i < library->mappings_count; i++) {
        if (library->mappings[i]->proc == proc)
            return library->mappings[i];
    }

    if (library->mappings_count >= library->mappings_size) {
        library->mappings = realloc(library->mappings,
            2 * library->mappings_size * sizeof(struct mapping*));
        if (!library->mappings) {
            fprintf(stderr, "Couldn't resize mappings array: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        library->mappings_size = 2 * library->mappings_size;
    }

    mapping = calloc(1, sizeof(*mapping));
    if (!mapping) {
        fprintf(stderr, "Couldn't allocate space for mapping struct: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    mapping->proc = proc;
    pm_memusage_zero(&mapping->usage);

    library->mappings[library->mappings_count++] = mapping;

    return mapping;
}

struct process_info *get_process(pid_t pid) {
    struct process_info *process;

    process = calloc(1, sizeof(*process));
    if (!process) {
        fprintf(stderr, "Couldn't allocate space for process struct: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    process->pid = pid;
    getprocname(pid, process->cmdline, sizeof(process->cmdline));

    return process;
}

static int parse_perm(const char *perm)
{
    int ret = 0;

    while (*perm) {
        switch(*perm) {
        case 'r':
            ret |= PM_MAP_READ;
            break;
        case 'w':
            ret |= PM_MAP_WRITE;
            break;
        case 'x':
            ret |= PM_MAP_EXEC;
            break;
        default:
            fprintf(stderr, "Unknown permission '%c'\n", *perm);
            exit(EXIT_FAILURE);
        }
        perm++;
    }
    return ret;
}

int main(int argc, char *argv[]) {
    char cmdline[256];
    char *prefix;
    size_t prefix_len;
    int (*compfn)(const void *a, const void *b);

    pm_kernel_t *ker;
    pm_process_t *proc;

    pid_t *pids;
    size_t num_procs;

    pm_map_t **maps;
    size_t num_maps;
    pm_memusage_t map_usage;

    struct library_info *li, **lis;
    struct mapping_info *mi, **mis;
    struct process_info *pi;

    size_t i, j;
    int error;
    int perm;
    bool all;
    uint64_t required_flags;
    uint64_t flags_mask;

    bool has_swap = false;

    signal(SIGPIPE, SIG_IGN);
    compfn = &sort_by_pss;
    order = -1;
    prefix = NULL;
    prefix_len = 0;
    opterr = 0;
    perm = 0;
    all = false;
    required_flags = 0;
    flags_mask = 0;

    while (1) {
        int c;
        const struct option longopts[] = {
            {"all", 0, 0, 'a'},
            {"cached", 0, 0, 'c'},
            {"nocached", 0, 0, 'C'},
            {"ksm", 0, 0, 'k'},
            {"help", 0, 0, 'h'},
            {"pss", 0, 0, 'p'},
            {"uss", 0, 0, 'u'},
            {"vss", 0, 0, 'v'},
            {"rss", 0, 0, 'r'},
            {"swap", 0, 0, 's'},
            {"reverse", 0, 0, 'R'},
            {"path", required_argument, 0, 'P'},
            {"perm", required_argument, 0, 'm'},
            {0, 0, 0, 0}
        };
        c = getopt_long(argc, argv, "acChkm:pP:uvrsR", longopts, NULL);
        if (c < 0) {
            break;
        }
        /* Alphabetical cases */
        switch (c) {
        case 'a':
            all = true;
            break;
        case 'c':
            required_flags = 0;
            flags_mask = PM_PAGE_SWAPBACKED;
            break;
        case 'C':
            required_flags = PM_PAGE_SWAPBACKED;
            flags_mask = PM_PAGE_SWAPBACKED;
            break;
        case 'k':
            required_flags = PM_PAGE_KSM;
            flags_mask = PM_PAGE_KSM;
            break;
        case 'h':
            usage(argv[0]);
            exit(EXIT_SUCCESS);
        case 'm':
            perm = parse_perm(optarg);
            break;
        case 'p':
            compfn = &sort_by_pss;
            break;
        case 'P':
            prefix = optarg;
            prefix_len = strlen(prefix);
            break;
        case 'u':
            compfn = &sort_by_uss;
            break;
        case 'v':
            compfn = &sort_by_vss;
            break;
        case 'r':
            compfn = &sort_by_rss;
            break;
        case 's':
            compfn = &sort_by_swap;
            break;
        case 'R':
            order *= -1;
            break;
        case '?':
            fprintf(stderr, "Invalid argument \"%s\".\n", argv[optind - 1]);
            usage(argv[0]);
            exit(EXIT_FAILURE);
        default:
            abort();
        }
    }

    argc -= optind;
    argv += optind;

    libraries = malloc(INIT_LIBRARIES * sizeof(struct library_info *));
    libraries_count = 0; libraries_size = INIT_LIBRARIES;

    error = pm_kernel_create(&ker);
    if (error) {
        fprintf(stderr, "Error initializing kernel interface -- "
                        "does this kernel have pagemap?\n");
        exit(EXIT_FAILURE);
    }

    error = pm_kernel_pids(ker, &pids, &num_procs);
    if (error) {
        fprintf(stderr, "Error listing processes.\n");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < num_procs; i++) {
        error = pm_process_create(ker, pids[i], &proc);
        if (error) {
            fprintf(stderr, "warning: could not create process interface for %d\n", pids[i]);
            continue;
        }

        pi = get_process(pids[i]);

        error = pm_process_maps(proc, &maps, &num_maps);
        if (error) {
            fprintf(stderr, "Error listing maps for process %d.\n", proc->pid);
            exit(EXIT_FAILURE);
        }

        for (j = 0; j < num_maps; j++) {
            if (prefix && (strncmp(pm_map_name(maps[j]), prefix, prefix_len)))
                continue;

            if (perm && (pm_map_flags(maps[j]) & PM_MAP_PERMISSIONS) != perm)
                continue;

            li = get_library(pm_map_name(maps[j]), all);
            if (!li)
                continue;

            mi = get_mapping(li, pi);

            error = pm_map_usage_flags(maps[j], &map_usage, flags_mask,
                                       required_flags);
            if (error) {
                fprintf(stderr, "Error getting map memory usage of "
                                "map %s in process %d.\n",
                        pm_map_name(maps[j]), proc->pid);
                exit(EXIT_FAILURE);
            }

            if (map_usage.swap) {
                has_swap = true;
            }

            pm_memusage_add(&mi->usage, &map_usage);
            pm_memusage_add(&li->total_usage, &map_usage);
        }
    }

    printf(" %6s   %6s   %6s   %6s   %6s  ", "RSStot", "VSS", "RSS", "PSS", "USS");

    if (has_swap) {
        printf(" %6s  ", "Swap");
    }

    printf("Name/PID\n");
    fflush(stdout);

    qsort(libraries, libraries_count, sizeof(libraries[0]), &licmp);

    for (i = 0; i < libraries_count; i++) {
        li = libraries[i];

        printf("%6zdK   %6s   %6s   %6s   %6s  ", li->total_usage.pss / 1024, "", "", "", "");
        if (has_swap) {
            printf(" %6s  ", "");
        }
        printf("%s\n", li->name);
        fflush(stdout);

        qsort(li->mappings, li->mappings_count, sizeof(li->mappings[0]), compfn);

        for (j = 0; j < li->mappings_count; j++) {
            mi = li->mappings[j];
            pi = mi->proc;
            printf(   " %6s  %6zdK  %6zdK  %6zdK  %6zdK  ", "",
                mi->usage.vss / 1024,
                mi->usage.rss / 1024,
                mi->usage.pss / 1024,
                mi->usage.uss / 1024);
            if (has_swap) {
                printf("%6zdK  ", mi->usage.swap / 1024);
            }
            printf("  %s [%d]\n",
                pi->cmdline,
                pi->pid);
        }
        printf("\n");
        fflush(stdout);
    }

    return 0;
}

static void usage(char *myname) {
    fprintf(stderr, "Usage: %s [ -P | -L ] [ -v | -r | -p | -u | -s | -h ]\n"
                    "\n"
                    "Sort options:\n"
                    "    -v  Sort processes by VSS.\n"
                    "    -r  Sort processes by RSS.\n"
                    "    -p  Sort processes by PSS.\n"
                    "    -u  Sort processes by USS.\n"
                    "    -s  Sort processes by swap.\n"
                    "        (Default sort order is PSS.)\n"
                    "    -a  Show all mappings, including stack, heap and anon.\n"
                    "    -P /path  Limit libraries displayed to those in path.\n"
                    "    -R  Reverse sort order (default is descending).\n"
                    "    -m [r][w][x] Only list pages that exactly match permissions\n"
                    "    -c  Only show cached (storage backed) pages\n"
                    "    -C  Only show non-cached (ram/swap backed) pages\n"
                    "    -k  Only show pages collapsed by KSM\n"
                    "    -h  Display this help screen.\n",
    myname);
}

static int getprocname(pid_t pid, char *buf, size_t len) {
    char filename[20];
    FILE *f;

    sprintf(filename, "/proc/%d/cmdline", pid);
    f = fopen(filename, "r");
    if (!f) { *buf = '\0'; return 1; }
    if (!fgets(buf, len, f)) { *buf = '\0'; return 2; }
    fclose(f);
    return 0;
}

static int numcmp(long long a, long long b) {
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

static int licmp(const void *a, const void *b) {
    return order * numcmp(
        (*((struct library_info**)a))->total_usage.pss,
        (*((struct library_info**)b))->total_usage.pss
    );
}

#define create_sort(field, compfn) \
    static int sort_by_ ## field (const void *a, const void *b) { \
        return order * compfn( \
            (*((struct mapping_info**)a))->usage.field, \
            (*((struct mapping_info**)b))->usage.field \
        ); \
    }

create_sort(vss, numcmp)
create_sort(rss, numcmp)
create_sort(pss, numcmp)
create_sort(uss, numcmp)
create_sort(swap, numcmp)
