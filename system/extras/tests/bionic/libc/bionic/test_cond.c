/*
 * Copyright (C) 2008 The Android Open Source Project
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
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

static pthread_mutex_t lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
static pthread_cond_t  wait = PTHREAD_COND_INITIALIZER;

static void* _thread1(void *__u __attribute__((unused)))
{
    printf("1: obtaining mutex\n");
    pthread_mutex_lock(&lock);
    printf("1: waiting on condition variable\n");
    pthread_cond_wait(&wait, &lock);
    printf("1: releasing mutex\n");
    pthread_mutex_unlock(&lock);
    printf("1: exiting\n");    
    return NULL;
}

static void* _thread2(void *__u __attribute__((unused)))
{
    int cnt = 2;
    while(cnt--) {
        printf("2: obtaining mutex\n");
        pthread_mutex_lock(&lock);
        printf("2: signaling\n");
        pthread_cond_signal(&wait);
        printf("2: releasing mutex\n");
        pthread_mutex_unlock(&lock);
    }

    printf("2: exiting\n");
    return NULL;
}

typedef void* (*thread_func)(void*);
static const  thread_func thread_routines[] =
{
    &_thread1,
    &_thread2,
};

int main(void)
{
    pthread_t t[2];
    int nn;
    int count = (int)(sizeof t/sizeof t[0]);

    for (nn = 0; nn < count; nn++) {
        printf("main: creating thread %d\n", nn+1);
        if (pthread_create( &t[nn], NULL, thread_routines[nn], NULL) < 0) {
            printf("main: could not create thread %d: %s\n", nn+1, strerror(errno));
            return -2;
        }
    }

    for (nn = 0; nn < count; nn++) {
        printf("main: joining thread %d\n", nn+1);
        if (pthread_join(t[nn], NULL)) {
            printf("main: could not join thread %d: %s\n", nn+1, strerror(errno));
            return -2;
        }
    }

    return 0;
}
