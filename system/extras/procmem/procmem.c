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

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <pagemap/pagemap.h>

/* Information about a single mapping */
struct map_info {
    pm_map_t *map;
    pm_memusage_t usage;
    /* page counts */
    unsigned long shared_clean;
    unsigned long shared_dirty;
    unsigned long private_clean;
    unsigned long private_dirty;
};

/* display the help screen */
static void usage(const char *cmd);

/* qsort compare function to compare maps by PSS */
int comp_pss(const void *a, const void *b);

int main(int argc, char *argv[]) {
    pid_t pid;

    /* libpagemap context */
    pm_kernel_t *ker;
    int pagesize; /* cached for speed */
    pm_process_t *proc;

    /* maps and such */
    pm_map_t **maps; size_t num_maps;

    struct map_info **mis;
    struct map_info *mi;

    /* pagemap information */
    uint64_t *pagemap; size_t num_pages;
    unsigned long address; uint64_t mapentry;
    uint64_t count, flags;

    /* totals */
    unsigned long total_shared_clean, total_shared_dirty, total_private_clean, total_private_dirty;
    pm_memusage_t total_usage;

    /* command-line options */
    int ws;
#define WS_OFF (0)
#define WS_ONLY (1)
#define WS_RESET (2)
    int (*compfn)(const void *a, const void *b);
    int hide_zeros;

    /* temporary variables */
    size_t i, j;
    char *endptr;
    int error;

    if (argc < 2) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    ws = WS_OFF;
    compfn = NULL;
    hide_zeros = 0;
    for (i = 1; i < (size_t)(argc - 1); i++) {
        if (!strcmp(argv[i], "-w")) { ws = WS_ONLY; continue; }
        if (!strcmp(argv[i], "-W")) { ws = WS_RESET; continue; }
        if (!strcmp(argv[i], "-m")) { compfn = NULL; continue; }
        if (!strcmp(argv[i], "-p")) { compfn = &comp_pss; continue; }
        if (!strcmp(argv[i], "-h")) { hide_zeros = 1; continue; }
        fprintf(stderr, "Invalid argument \"%s\".\n", argv[i]);
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    pid = (pid_t)strtol(argv[argc - 1], &endptr, 10);
    if (*endptr != '\0') {
        fprintf(stderr, "Invalid PID \"%s\".\n", argv[argc - 1]);
        exit(EXIT_FAILURE);
    }

    error = pm_kernel_create(&ker);
    if (error) {
        fprintf(stderr, "error creating kernel interface -- "
                        "does this kernel have pagemap?\n");
        exit(EXIT_FAILURE);
    }

    pagesize = pm_kernel_pagesize(ker);

    error = pm_process_create(ker, pid, &proc);
    if (error) {
        fprintf(stderr, "error creating process interface -- "
                        "does process %d really exist?\n", pid);
        exit(EXIT_FAILURE);
    }

    if (ws == WS_RESET) {
        error = pm_process_workingset(proc, NULL, 1);
        if (error) {
            fprintf(stderr, "error resetting working set for process.\n");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }

    /* get maps, and allocate our map_info array */
    error = pm_process_maps(proc, &maps, &num_maps);
    if (error) {
        fprintf(stderr, "error listing maps.\n");
        exit(EXIT_FAILURE);
    }

    mis = (struct map_info **)calloc(num_maps, sizeof(struct map_info *));
    if (!mis) {
        fprintf(stderr, "error allocating map_info array: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* print header */
    if (ws == WS_ONLY) {
        printf("%7s  %7s  %7s  %7s  %7s  %7s  %7s  %s\n",
               "WRss", "WPss", "WUss", "WShCl", "WShDi", "WPrCl", "WPrDi", "Name");
        printf("%7s  %7s  %7s  %7s  %7s  %7s  %7s  %s\n",
               "-------", "-------", "-------", "-------", "-------", "-------", "-------", "");
    } else {
        printf("%7s  %7s  %7s  %7s  %7s  %7s  %7s  %7s  %s\n",
               "Vss", "Rss", "Pss", "Uss", "ShCl", "ShDi", "PrCl", "PrDi", "Name");
        printf("%7s  %7s  %7s  %7s  %7s  %7s  %7s  %7s  %s\n",
               "-------", "-------", "-------", "-------", "-------", "-------", "-------", "-------", "");
    }

    /* zero things */
    pm_memusage_zero(&total_usage);
    total_shared_clean = total_shared_dirty = total_private_clean = total_private_dirty = 0;

    for (i = 0; i < num_maps; i++) {
        mi = (struct map_info *)calloc(1, sizeof(struct map_info));
        if (!mi) {
            fprintf(stderr, "error allocating map_info: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        mi->map = maps[i];

        /* get, and sum, memory usage */

        if (ws == WS_ONLY)
            error = pm_map_workingset(mi->map, &mi->usage);
        else
            error = pm_map_usage(mi->map, &mi->usage);
        if (error) {
            fflush(stdout);
            fprintf(stderr, "error getting usage for map.\n");
            continue;
        }

        pm_memusage_add(&total_usage, &mi->usage);

        /* get, and sum, individual page counts */

        error = pm_map_pagemap(mi->map, &pagemap, &num_pages);
        if (error) {
            fflush(stdout);
            fprintf(stderr, "error getting pagemap for map.\n");
            continue;
        }

        mi->shared_clean = mi->shared_dirty = mi->private_clean = mi->private_dirty = 0;

        for (j = 0; j < num_pages; j++) {
            address = pm_map_start(mi->map) + j * ker->pagesize;
            mapentry = pagemap[j];

            if (PM_PAGEMAP_PRESENT(mapentry) && !PM_PAGEMAP_SWAPPED(mapentry)) {

                error = pm_kernel_count(ker, PM_PAGEMAP_PFN(mapentry), &count);
                if (error) {
                    fflush(stdout);
                    fprintf(stderr, "error getting count for frame.\n");
                }

                error = pm_kernel_flags(ker, PM_PAGEMAP_PFN(mapentry), &flags);
                if (error) {
                    fflush(stdout);
                    fprintf(stderr, "error getting flags for frame.\n");
                }

                if ((ws != WS_ONLY) || (flags & PM_PAGE_REFERENCED)) {
                    if (count > 1) {
                        if (flags & PM_PAGE_DIRTY)
                            mi->shared_dirty++;
                        else
                            mi->shared_clean++;
                    } else {
                        if (flags & PM_PAGE_DIRTY)
                            mi->private_dirty++;
                        else
                            mi->private_clean++;
                    }
                }
            }
        }

        total_shared_clean += mi->shared_clean;
        total_shared_dirty += mi->shared_dirty;
        total_private_clean += mi->private_clean;
        total_private_dirty += mi->private_dirty;

        /* add to array */
        mis[i] = mi;
    }

    /* sort the array, if requested (compfn == NULL for original order) */
    if (compfn)
        qsort(mis, num_maps, sizeof(mis[0]), compfn);

    for (i = 0; i < num_maps; i++) {
        mi = mis[i];

        if ((!mi) || (hide_zeros && !mi->usage.rss))
            continue;

        if (ws == WS_ONLY) {
            printf("%6ldK  %6ldK  %6ldK  %6ldK  %6ldK  %6ldK  %6ldK  %s\n",
                (long)mi->usage.rss / 1024,
                (long)mi->usage.pss / 1024,
                (long)mi->usage.uss / 1024,
                mi->shared_clean * pagesize / 1024,
                mi->shared_dirty * pagesize / 1024,
                mi->private_clean * pagesize / 1024,
                mi->private_dirty * pagesize / 1024,
                pm_map_name(mi->map)
            );
        } else {
            printf("%6ldK  %6ldK  %6ldK  %6ldK  %6ldK  %6ldK  %6ldK  %6ldK  %s\n",
                (long)mi->usage.vss / 1024,
                (long)mi->usage.rss / 1024,
                (long)mi->usage.pss / 1024,
                (long)mi->usage.uss / 1024,
                mi->shared_clean * pagesize / 1024,
                mi->shared_dirty * pagesize / 1024,
                mi->private_clean * pagesize / 1024,
                mi->private_dirty * pagesize / 1024,
                pm_map_name(mi->map)
            );
        }
    }

    /* print totals */
    if (ws == WS_ONLY) {
        printf("%7s  %7s  %7s  %7s  %7s  %7s  %7s  %s\n",
               "-------", "-------", "-------", "-------", "-------", "-------", "-------", "");
        printf("%6ldK  %6ldK  %6ldK  %6ldK  %6ldK  %6ldK  %6ldK  %s\n",
            (long)total_usage.rss / 1024,
            (long)total_usage.pss / 1024,
            (long)total_usage.uss / 1024,
            total_shared_clean * pagesize / 1024,
            total_shared_dirty * pagesize / 1024,
            total_private_clean * pagesize / 1024,
            total_private_dirty * pagesize / 1024,
            "TOTAL"
        );
    } else {
        printf("%7s  %7s  %7s  %7s  %7s  %7s  %7s  %7s  %s\n",
               "-------", "-------", "-------", "-------", "-------", "-------", "-------", "-------", "");
        printf("%6ldK  %6ldK  %6ldK  %6ldK  %6ldK  %6ldK  %6ldK  %6ldK  %s\n",
            (long)total_usage.vss / 1024,
            (long)total_usage.rss / 1024,
            (long)total_usage.pss / 1024,
            (long)total_usage.uss / 1024,
            total_shared_clean * pagesize / 1024,
            total_shared_dirty * pagesize / 1024,
            total_private_clean * pagesize / 1024,
            total_private_dirty * pagesize / 1024,
            "TOTAL"
        );
    }

    return 0;
}

static void usage(const char *cmd) {
    fprintf(stderr, "Usage: %s [ -w | -W ] [ -p | -m ] [ -h ] pid\n"
                    "    -w  Displays statistics for the working set only.\n"
                    "    -W  Resets the working set of the process.\n"
                    "    -p  Sort by PSS.\n"
                    "    -m  Sort by mapping order (as read from /proc).\n"
                    "    -h  Hide maps with no RSS.\n",
        cmd);
}

int comp_pss(const void *a, const void *b) {
    struct map_info *ma, *mb;

    ma = *((struct map_info **)a);
    mb = *((struct map_info **)b);

    if (mb->usage.pss < ma->usage.pss) return -1;
    if (mb->usage.pss > ma->usage.pss) return 1;
    return 0;
}
