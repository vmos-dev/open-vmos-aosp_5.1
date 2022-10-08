/*
 * Copyright (C) 2010 The Android Open Source Project
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

#define N_PAGES (4096)

#define WARMUP (1<<10)

#define WORKLOAD (1<<24)

int numPagesList[] = {
  1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
  12, 14, 16, 18, 20, 24, 28, 30, 32, 34, 48, 62, 64, 66, 80,
  96, 112, 128, 144, 160, 320, 480, 496, 512, 528, 544, 576, 640, 960,
  1024, 2048, 3072, 4000,
};

static unsigned long long stop_watch()
{
    struct timespec t;
    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec*1000000000ULL + t.tv_nsec;
}   

int main()
{
    char *mem = malloc((N_PAGES+1) * 4096);
    intptr_t *p;
    int i;
    unsigned int j;

    /* Align to page start */
    mem = (char *) ((intptr_t) (mem + 4096) & ~0xfff);

    for (j = 0; j < sizeof(numPagesList)/sizeof(int); j++) {
        int numPages = numPagesList[j];
        int pageIdx = 0;
        int entryOffset = 0;

        /*
         * page 0      page 1      page 2     ....     page N  
         * ------      ------      ------              ------  
         * word 0   -> word 0   -> word 0 ->  ....  -> word 0 -> (page 0/word 0)
         *   :           :           :         :         :
         * word 1023   word 1023   word 1023   :       word 1023
         */
        for (i = 0; i < numPages; i++) {
            int nextPageIdx = (pageIdx + 1) % numPages;
            /* Looks like spread the pointer across cache lines introduce noise
             * to get to the asymptote
             * int nextEntryOffset = (entryOffset + 32) % 1024;
             */
            int nextEntryOffset = entryOffset;

            if (i != numPages -1) {
                *(intptr_t *) (mem + 4096 * pageIdx + entryOffset) = 
                    (intptr_t) (mem + 4096 * nextPageIdx + nextEntryOffset);
            } else {
                /* Last page - form the cycle */
                *(intptr_t *) (mem + 4096 * pageIdx + entryOffset) =
                    (intptr_t) &mem[0];
            }

            pageIdx = nextPageIdx;
            entryOffset = nextEntryOffset;
        }

        /* Starting point of the pointer chase */
        p = (intptr_t *) &mem[0];

        /* Warmup (ie pre-thrash the memory system */
        for (i = 0; i < WARMUP; i++) {
            p = (intptr_t *) *p;
        }

        /* Real work */
        unsigned long long t0 = stop_watch();
        for (i = 0; i < WORKLOAD; i++) {
            p = (intptr_t *) *p;
        }
        unsigned long long t1 = stop_watch();

        /* To keep p from being optimized by gcc */
        if (p) 
            printf("%d, %f\n", numPages, (float) (t1 - t0) / WORKLOAD);
    }
    return 0;
}
