/*
 * Copyright (C) 2010 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#define _GNU_SOURCE 1
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int failures = 0;

#define TEST_INT_EQ(cond,exp) \
    do {\
        int  _cond = (cond); \
        int  _exp  = (exp); \
        if ((_cond) != (_exp)) {\
            fprintf(stderr, "Assertion failure:%s:%d: '%s' returned %d (%d expected)\n", \
                    __FUNCTION__, __LINE__, #cond, _cond, _exp);\
        }\
    }while(0)

#define  T(cond)  \
    do {\
        if (!(cond)) {\
            fprintf(stderr,"Assertion failure:%s:%d: %s is not TRUE\n",\
                           __FUNCTION__, __LINE__, #cond);\
            failures++;\
        }\
    } while(0)

#define  F(cond) \
    do {\
        if (!!(cond)) {\
            fprintf(stderr,"Assertion failure:%s:%d: %s is not FALSE\n",\
                           __FUNCTION__, __LINE__, #cond);\
            failures++;\
        }\
    } while (0)


static void
test_1(cpu_set_t* set)
{
    cpu_set_t  other[1];
    int nn, nnMax = CPU_SETSIZE;

    memset(other, 0, sizeof *other);
    TEST_INT_EQ(CPU_COUNT(other),0);

    /* First, cheeck against zero */
    CPU_ZERO(set);
    TEST_INT_EQ(CPU_COUNT(set),0);
    T(CPU_EQUAL(set, other));
    T(CPU_EQUAL(other, set));

    for (nn = 0; nn < nnMax; nn++)
        F(CPU_ISSET(nn, set));

    /* Check individual bits */
    for (nn = 0; nn < nnMax; nn++) {
        int mm;
        CPU_SET(nn, set);
        TEST_INT_EQ(CPU_COUNT(set),1);
        for (mm = 0; mm < nnMax; mm++) {
            T(CPU_ISSET(mm, set) == (mm == nn));
        }
        CPU_CLR(nn, set);
        T(CPU_EQUAL(set, other));
    }

    /* Check cumulative bits, incrementing */
    for (nn = 0; nn < nnMax; nn++) {
        int mm;
        CPU_SET(nn, set);
        TEST_INT_EQ(CPU_COUNT(set), nn+1);
        for (mm = 0; mm < nnMax; mm++) {
            T(CPU_ISSET(mm, set) == (mm <= nn));
        }
    }

    /* Check individual clear bits */
    for (nn = 0; nn < nnMax; nn++) {
        int mm;
        CPU_CLR(nn, set);
        TEST_INT_EQ(CPU_COUNT(set), nnMax-1);
        for (mm = 0; mm < nnMax; mm++) {
            T(CPU_ISSET(mm, set) == (mm != nn));
        }
        CPU_SET(nn, set);
    }

    /* Check cumulative bits, decrementing */
    for (nn = nnMax-1; nn >= 0; nn--) {
        int mm;
        CPU_CLR(nn, set);
        TEST_INT_EQ(CPU_COUNT(set), nn);
        for (mm = 0; mm < nnMax; mm++) {
            T(CPU_ISSET(mm, set) == (mm < nn));
        }
    }
    T(CPU_EQUAL(set, other));
}

static void
test_1_s(size_t setsize, cpu_set_t* set)
{
    int nn, nnMax;
    cpu_set_t* other;

    /* First, cheeck against zero */
    other = calloc(1,setsize);
    TEST_INT_EQ(CPU_COUNT(other),0);
    CPU_ZERO_S(setsize, set);
    T(CPU_EQUAL_S(setsize, set, other));
    T(CPU_EQUAL_S(setsize, other, set));

    nnMax = setsize*8;
    for (nn = 0; nn < nnMax; nn++)
        F(CPU_ISSET_S(nn, setsize, set));

    /* Check individual bits */
    for (nn = 0; nn < nnMax; nn++) {
        int mm;
        CPU_SET_S(nn, setsize, set);
        TEST_INT_EQ(CPU_COUNT_S(setsize, set), 1);
        for (mm = 0; mm < nnMax; mm++) {
            T(CPU_ISSET_S(mm, setsize, set) == (mm == nn));
        }
        CPU_CLR_S(nn, setsize, set);
        T(CPU_EQUAL_S(setsize, set, other));
    }

    /* Check cumulative bits, incrementing */
    for (nn = 0; nn < nnMax; nn++) {
        int mm;
        CPU_SET_S(nn, setsize, set);
        TEST_INT_EQ(CPU_COUNT_S(setsize, set), nn+1);
        for (mm = 0; mm < nnMax; mm++) {
            T(CPU_ISSET_S(mm, setsize, set) == (mm <= nn));
        }
    }

    /* Check individual clear bits */
    for (nn = 0; nn < nnMax; nn++) {
        int mm;
        CPU_CLR_S(nn, setsize, set);
        TEST_INT_EQ(CPU_COUNT_S(setsize, set), nnMax-1);
        for (mm = 0; mm < nnMax; mm++) {
            T(CPU_ISSET_S(mm, setsize, set) == (mm != nn));
        }
        CPU_SET_S(nn, setsize, set);
    }

    /* Check cumulative bits, decrementing */
    for (nn = nnMax-1; nn >= 0; nn--) {
        int mm;
        CPU_CLR_S(nn, setsize, set);
        TEST_INT_EQ(CPU_COUNT_S(setsize, set), nn);
        for (mm = 0; mm < nnMax; mm++) {
            T(CPU_ISSET_S(mm, setsize, set) == (mm < nn));
        }
    }
    T(CPU_EQUAL_S(setsize, set, other));

    free(other);
}


int main(void)
{
    cpu_set_t  set0;
    int cpu;
    test_1(&set0);
    test_1_s(sizeof(set0), &set0);

    size_t count;
    for (count = 32; count <= 1024; count *= 2) {
        cpu_set_t* set = CPU_ALLOC(count);
        test_1_s(count/8, set);
        CPU_FREE(set);
    }

    T((cpu = sched_getcpu()) >= 0);

    int ret;
    TEST_INT_EQ((ret = sched_getaffinity(getpid(), sizeof(cpu_set_t), &set0)), 0);

    CPU_ZERO(&set0);
    CPU_SET(cpu, &set0);

    TEST_INT_EQ((ret = sched_setaffinity(getpid(), sizeof(cpu_set_t), &set0)), 0);

    if (failures == 0) {
        printf("OK\n");
        return 0;
    } else {
        printf("KO: %d failures\n", failures);
        return 1;
    }
}
