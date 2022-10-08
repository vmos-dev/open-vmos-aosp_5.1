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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/mman.h>

#include "memtest.h"

nsecs_t system_time()
{
    struct timespec t;
    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return nsecs_t(t.tv_sec)*1000000000LL + t.tv_nsec;
}

static void usage(char* p) {
    printf("Usage: %s <test> <options>\n"
           "<test> is one of the following:\n"
           "  copy_bandwidth [--size BYTES_TO_COPY]\n"
           "  write_bandwidth [--size BYTES_TO_WRITE]\n"
           "  read_bandwidth [--size BYTES_TO_COPY]\n"
           "  per_core_bandwidth [--size BYTES]\n"
           "    --type copy_ldrd_strd | copy_ldmia_stmia | copy_vld1_vst1 |\n"
           "           copy_vldr_vstr | copy_vldmia_vstmia | memcpy | write_strd |\n"
           "           write_stmia | write_vst1 | write_vstr | write_vstmia | memset |\n"
           "           read_ldrd | read_ldmia | read_vld1 | read_vldr | read_vldmia\n"
           "  multithread_bandwidth [--size BYTES]\n"
           "    --type copy_ldrd_strd | copy_ldmia_stmia | copy_vld1_vst1 |\n"
           "           copy_vldr_vstr | copy_vldmia_vstmia | memcpy | write_strd |\n"
           "           write_stmia | write_vst1 | write_vstr | write_vstmia | memset |\n"
           "           read_ldrd | read_ldmia | read_vld1 | read_vldr | read_vldmia\n"
           "    --num_threads NUM_THREADS_TO_RUN\n"
           "  malloc [fill]\n"
           "  madvise\n"
           "  resampler\n"
           "  crash\n"
           "  stack (stack smasher)\n"
           "  crawl\n"
           , p);
}

int copy_bandwidth(int argc, char** argv);
int write_bandwidth(int argc, char** argv);
int read_bandwidth(int argc, char** argv);
int per_core_bandwidth(int argc, char** argv);
int multithread_bandwidth(int argc, char** argv);
int malloc_test(int argc, char** argv);
int madvise_test(int argc, char** argv);
int crash_test(int argc, char** argv);
int stack_smasher_test(int argc, char** argv);
int crawl_test(int argc, char** argv);
int fp_test(int argc, char** argv);

typedef struct {
    const char *cmd_name;
    int (*func)(int argc, char** argv);
} function_t;

function_t function_table[] = {
    { "malloc", malloc_test },
    { "madvise", madvise_test },
    { "crash", crash_test },
    { "stack", stack_smasher_test },
    { "crawl", crawl_test },
    { "fp", fp_test },
    { "copy_bandwidth", copy_bandwidth },
    { "write_bandwidth", write_bandwidth },
    { "read_bandwidth", read_bandwidth },
    { "per_core_bandwidth", per_core_bandwidth },
    { "multithread_bandwidth", multithread_bandwidth },
};

int main(int argc, char** argv)
{
    if (argc == 1) {
        usage(argv[0]);
        return 0;
    }
    int err = -1;
    for (unsigned int i = 0; i < sizeof(function_table)/sizeof(function_t); i++) {
        if (strcmp(argv[1], function_table[i].cmd_name) == 0) {
            err = (*function_table[i].func)(argc-1, argv+1);
            break;
        }
    }
    if (err) {
        usage(argv[0]);
    }
    return err;
}

int malloc_test(int argc, char** argv)
{
    bool fill = (argc>=2 && !strcmp(argv[1], "fill"));
    size_t total = 0;
    size_t size = 0x40000000;
    while (size) {
        void* addr = malloc(size);
        if (addr == 0) {
            printf("size = %9zd failed\n", size);
            size >>= 1;
        } else {
            total += size;
            printf("size = %9zd, addr = %p (total = %9zd (%zd MB))\n",
                    size, addr, total, total / (1024*1024));
            if (fill) {
                printf("filling...\n");
                fflush(stdout);
                memset(addr, 0, size);
            }
            size = size + (size>>1);
        }
    }
    printf("done. allocated %zd MB\n", total / (1024*1024));
    return 0;
}

int madvise_test(int argc, char** argv)
{
    for (int i=0 ; i<2 ; i++) {
        size_t size = i==0 ? 4096 : 48*1024*1024; // 48 MB
        printf("Allocating %zd MB... ", size/(1024*1024)); fflush(stdout);
        void* addr1 = mmap(0, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
        printf("%p (%s)\n", addr1, addr1==(void*)-1 ? "failed" : "OK"); fflush(stdout);

        printf("touching %p...\n", addr1); fflush(stdout);
        memset(addr1, 0x55, size);

        printf("advising DONTNEED...\n"); fflush(stdout);
        madvise(addr1, size, MADV_DONTNEED);

        printf("reading back %p...\n", addr1); fflush(stdout);
        if (*(long*)addr1 == 0) {
            printf("madvise freed some pages\n");
        } else if (*(long*)addr1 == 0x55555555) {
            printf("pages are still there\n");
        } else {
            printf("getting garbage back\n");
        }

        printf("Allocating %zd MB... ", size/(1024*1024)); fflush(stdout);
        void* addr2 = mmap(0, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
        printf("%p (%s)\n", addr2, addr2==(void*)-1 ? "failed" : "OK"); fflush(stdout);

        printf("touching %p...\n", addr2); fflush(stdout);
        memset(addr2, 0xAA, size);

        printf("unmap %p ...\n", addr2); fflush(stdout);
        munmap(addr2, size);

        printf("touching %p...\n", addr1); fflush(stdout);
        memset(addr1, 0x55, size);

        printf("unmap %p ...\n", addr1); fflush(stdout);
        munmap(addr1, size);
    }

    printf("Done\n"); fflush(stdout);
    return 0;
}

int crash_test(int argc, char** argv)
{
    printf("about to crash...\n");
    asm volatile(
        "mov r0,  #0 \n"
        "mov r1,  #1 \n"
        "mov r2,  #2 \n"
        "mov r3,  #3 \n"
        "ldr r12, [r0] \n"
    );

    return 0;
}

int stack_smasher_test(int argc, char** argv)
{
    int dummy = 0;
    printf("corrupting our stack...\n");
    *(volatile long long*)&dummy = 0;
    return 0;
}

// --------------------------------------------------------------------

extern "C" void thumb_function_1(int*p);
extern "C" void thumb_function_2(int*p);
extern "C" void arm_function_3(int*p);
extern "C" void arm_function_2(int*p);
extern "C" void arm_function_1(int*p);

void arm_function_3(int*p) {
    int a = 0;
    thumb_function_2(&a);
}

void arm_function_2(int*p) {
    int a = 0;
    thumb_function_1(&a);
}

void arm_function_1(int*p) {
    int a = 0;
    arm_function_2(&a);
}

int crawl_test(int argc, char** argv)
{
    int a = 0;
    arm_function_1(&a);
    return 0;
}

