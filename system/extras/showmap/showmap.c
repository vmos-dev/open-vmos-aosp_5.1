#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <ctype.h>
#include <stddef.h>

typedef struct mapinfo mapinfo;

struct mapinfo {
    mapinfo *next;
    unsigned start;
    unsigned end;
    unsigned size;
    unsigned rss;
    unsigned pss;
    unsigned shared_clean;
    unsigned shared_dirty;
    unsigned private_clean;
    unsigned private_dirty;
    int is_bss;
    int count;
    char name[1];
};

static int is_library(const char *name) {
    int len = strlen(name);
    return len >= 4 && name[0] == '/'
            && name[len - 3] == '.' && name[len - 2] == 's' && name[len - 1] == 'o';
}

// 6f000000-6f01e000 rwxp 00000000 00:0c 16389419   /android/lib/libcomposer.so
// 012345678901234567890123456789012345678901234567890123456789
// 0         1         2         3         4         5

static int parse_header(const char* line, const mapinfo* prev, mapinfo** mi) {
    unsigned long start;
    unsigned long end;
    char name[128];
    int name_pos;
    int is_bss = 0;

    if (sscanf(line, "%lx-%lx %*s %*x %*x:%*x %*d%n", &start, &end, &name_pos) != 2) {
        *mi = NULL;
        return -1;
    }

    while (isspace(line[name_pos])) {
        name_pos += 1;
    }

    if (line[name_pos]) {
        strlcpy(name, line + name_pos, sizeof(name));
    } else {
        if (prev && start == prev->end && is_library(prev->name)) {
            // anonymous mappings immediately adjacent to shared libraries
            // usually correspond to the library BSS segment, so we use the
            // library's own name
            strlcpy(name, prev->name, sizeof(name));
            is_bss = 1;
        } else {
            strlcpy(name, "[anon]", sizeof(name));
        }
    }

    const int name_size = strlen(name) + 1;
    struct mapinfo* info = calloc(1, sizeof(mapinfo) + name_size);
    if (info == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }

    info->start = start;
    info->end = end;
    info->is_bss = is_bss;
    info->count = 1;
    strlcpy(info->name, name, name_size);

    *mi = info;
    return 0;
}

static int parse_field(mapinfo* mi, const char* line) {
    char field[64];
    int len;

    if (sscanf(line, "%63s %n", field, &len) == 1
            && *field && field[strlen(field) - 1] == ':') {
        int size;
        if (sscanf(line + len, "%d kB", &size) == 1) {
            if (!strcmp(field, "Size:")) {
                mi->size = size;
            } else if (!strcmp(field, "Rss:")) {
                mi->rss = size;
            } else if (!strcmp(field, "Pss:")) {
                mi->pss = size;
            } else if (!strcmp(field, "Shared_Clean:")) {
                mi->shared_clean = size;
            } else if (!strcmp(field, "Shared_Dirty:")) {
                mi->shared_dirty = size;
            } else if (!strcmp(field, "Private_Clean:")) {
                mi->private_clean = size;
            } else if (!strcmp(field, "Private_Dirty:")) {
                mi->private_dirty = size;
            }
        }
        return 0;
    }
    return -1;
}

static int order_before(const mapinfo *a, const mapinfo *b, int sort_by_address) {
    if (sort_by_address) {
        return a->start < b->start
                || (a->start == b->start && a->end < b->end);
    } else {
        return strcmp(a->name, b->name) < 0;
    }
}

static void enqueue_map(mapinfo **head, mapinfo *map, int sort_by_address, int coalesce_by_name) {
    mapinfo *prev = NULL;
    mapinfo *current = *head;

    if (!map) {
        return;
    }

    for (;;) {
        if (current && coalesce_by_name && !strcmp(map->name, current->name)) {
            current->size += map->size;
            current->rss += map->rss;
            current->pss += map->pss;
            current->shared_clean += map->shared_clean;
            current->shared_dirty += map->shared_dirty;
            current->private_clean += map->private_clean;
            current->private_dirty += map->private_dirty;
            current->is_bss &= map->is_bss;
            current->count++;
            free(map);
            break;
        }

        if (!current || order_before(map, current, sort_by_address)) {
            if (prev) {
                prev->next = map;
            } else {
                *head = map;
            }
            map->next = current;
            break;
        }

        prev = current;
        current = current->next;
    }
}

static mapinfo *load_maps(int pid, int sort_by_address, int coalesce_by_name)
{
    char fn[128];
    FILE *fp;
    char line[1024];
    mapinfo *head = NULL;
    mapinfo *current = NULL;
    int len;

    snprintf(fn, sizeof(fn), "/proc/%d/smaps", pid);
    fp = fopen(fn, "r");
    if (fp == 0) {
        fprintf(stderr, "cannot open /proc/%d/smaps: %s\n", pid, strerror(errno));
        return NULL;
    }

    while (fgets(line, sizeof(line), fp) != 0) {
        len = strlen(line);
        if (line[len - 1] == '\n') {
            line[--len] = 0;
        }

        if (current != NULL && !parse_field(current, line)) {
            continue;
        }

        mapinfo *next;
        if (!parse_header(line, current, &next)) {
            enqueue_map(&head, current, sort_by_address, coalesce_by_name);
            current = next;
            continue;
        }

        fprintf(stderr, "warning: could not parse map info line: %s\n", line);
    }

    enqueue_map(&head, current, sort_by_address, coalesce_by_name);

    fclose(fp);

    if (!head) {
        fprintf(stderr, "could not read /proc/%d/smaps\n", pid);
        return NULL;
    }

    return head;
}

static int verbose = 0;
static int terse = 0;
static int addresses = 0;

static void print_header()
{
    if (addresses) {
        printf("   start      end ");
    }
    printf(" virtual                     shared   shared  private  private\n");

    if (addresses) {
        printf("    addr     addr ");
    }
    printf("    size      RSS      PSS    clean    dirty    clean    dirty ");
    if (!verbose && !addresses) {
        printf("   # ");
    }
    printf("object\n");
}

static void print_divider()
{
    if (addresses) {
        printf("-------- -------- ");
    }
    printf("-------- -------- -------- -------- -------- -------- -------- ");
    if (!verbose && !addresses) {
        printf("---- ");
    }
    printf("------------------------------\n");
}

static int show_map(int pid)
{
    mapinfo *milist;
    mapinfo *mi;
    unsigned shared_dirty = 0;
    unsigned shared_clean = 0;
    unsigned private_dirty = 0;
    unsigned private_clean = 0;
    unsigned rss = 0;
    unsigned pss = 0;
    unsigned size = 0;
    unsigned count = 0;

    milist = load_maps(pid, addresses, !verbose && !addresses);
    if (milist == NULL) {
        return 1;
    }

    print_header();
    print_divider();

    for (mi = milist; mi;) {
        mapinfo* last = mi;

        shared_clean += mi->shared_clean;
        shared_dirty += mi->shared_dirty;
        private_clean += mi->private_clean;
        private_dirty += mi->private_dirty;
        rss += mi->rss;
        pss += mi->pss;
        size += mi->size;
        count += mi->count;
        
        if (terse && !mi->private_dirty) {
            goto out;
        }

        if (addresses) {
            printf("%08x %08x ", mi->start, mi->end);
        }
        printf("%8d %8d %8d %8d %8d %8d %8d ", mi->size,
               mi->rss,
               mi->pss,
               mi->shared_clean, mi->shared_dirty,
               mi->private_clean, mi->private_dirty);
        if (!verbose && !addresses) {
            printf("%4d ", mi->count);
        }
        printf("%s%s\n", mi->name, mi->is_bss ? " [bss]" : "");

out:
        mi = mi->next;
        free(last);
    }

    print_divider();
    print_header();
    print_divider();

    if (addresses) {
        printf("                  ");
    }
    printf("%8d %8d %8d %8d %8d %8d %8d ", size,
            rss, pss,
            shared_clean, shared_dirty,
            private_clean, private_dirty);
    if (!verbose && !addresses) {
        printf("%4d ", count);
    }
    printf("TOTAL\n");

    return 0;
}

int main(int argc, char *argv[])
{
    int usage = 1;
    int result = 0;
    int pid;
    char *arg;
    char *argend;

    signal(SIGPIPE, SIG_IGN);
    for (argc--, argv++; argc > 0; argc--, argv++) {
        arg = argv[0];
        if (!strcmp(arg,"-v")) {
            verbose = 1;
            continue;
        }
        if (!strcmp(arg,"-t")) {
            terse = 1;
            continue;
        }
        if (!strcmp(arg,"-a")) {
            addresses = 1;
            continue;
        }
        if (argc != 1) {
            fprintf(stderr, "too many arguments\n");
            break;
        }
        pid = strtol(arg, &argend, 10);
        if (*arg && !*argend) {
            usage = 0;
            if (show_map(pid)) {
                result = 1;
            }
            break;
        }
        fprintf(stderr, "unrecognized argument: %s\n", arg);
        break;
    }

    if (usage) {
        fprintf(stderr,
                "showmap [-t] [-v] [-c] <pid>\n"
                "        -t = terse (show only items with private pages)\n"
                "        -v = verbose (don't coalesce maps with the same name)\n"
                "        -a = addresses (show virtual memory map)\n"
                );
        result = 1;
    }

    return result;
}
