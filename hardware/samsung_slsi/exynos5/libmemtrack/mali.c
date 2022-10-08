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
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include <hardware/memtrack.h>

#include "memtrack_exynos5.h"

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
#define min(x, y) ((x) < (y) ? (x) : (y))

struct memtrack_record record_templates[] = {
    {
        .flags = MEMTRACK_FLAG_SMAPS_ACCOUNTED |
                 MEMTRACK_FLAG_PRIVATE |
                 MEMTRACK_FLAG_NONSECURE,
    },
    {
        .flags = MEMTRACK_FLAG_SMAPS_UNACCOUNTED |
                 MEMTRACK_FLAG_PRIVATE |
                 MEMTRACK_FLAG_NONSECURE,
    },
};

int mali_memtrack_get_memory(pid_t pid, int type,
                             struct memtrack_record *records,
                             size_t *num_records)
{
    size_t allocated_records = min(*num_records, ARRAY_SIZE(record_templates));
    int i;
    FILE *fp;
    char line[1024];

    *num_records = ARRAY_SIZE(record_templates);

    /* fastpath to return the necessary number of records */
    if (allocated_records == 0) {
        return 0;
    }

    fp = fopen("/sys/devices/platform/mali.0/gpu_memory", "r");
    if (fp == NULL) {
        return -errno;
    }

    memcpy(records, record_templates,
           sizeof(struct memtrack_record) * allocated_records);

    while (1) {
        if (fgets(line, sizeof(line), fp) == NULL) {
            break;
        }

        /* Format:
         * Name                 pid  cap(pages) usage(pages) unmapped(pages)
         * =================================================================
         * mali0                        524288      85163
         *   ndroid.calendar  10708    1048575        181        111
         */
        if (line[0] == ' ' && line[1] == ' ') {
            unsigned int allocated;
            unsigned int unmapped;
            int line_pid;

            int ret = sscanf(line, "  %*s %u %*u %u %u\n",
                             &line_pid, &allocated, &unmapped);
            if (ret == 3 && line_pid == pid) {
                if (allocated_records > 0) {
                    records[0].size_in_bytes = (allocated - unmapped) * PAGE_SIZE;
                }
                if (allocated_records > 1) {
                    records[1].size_in_bytes = unmapped * PAGE_SIZE;
                }
                break;
            }
        }
    }

    fclose(fp);

    return 0;
}
