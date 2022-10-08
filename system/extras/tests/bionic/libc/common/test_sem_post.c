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

/* This program is used to test that sem_post() will wake up all
 * threads that are waiting on a single semaphore, and not all of
 * them.
 */

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

static sem_t  semaphore;

/* Thread function, just wait for the semaphore */
static void*
thread_func(void* arg)
{
    sem_t *psem = (sem_t *)arg;
    void *me = (void *)pthread_self();
    printf("thread %p waiting\n", me);
    sem_wait(psem);
    printf("thread %p exiting\n", me);
    return me;
}

#define MAX_THREADS  50

int main(void)
{
    pthread_t   t[MAX_THREADS];
    int         nn, value;

    /* Initialize to 1, first thread will exit immediately.
     * this is used to exercize more of the semaphore code path */
    if ( sem_init( &semaphore, 0, 1 ) < 0 ) {
        printf( "Could not initialize semaphore: %s\n", strerror(errno) );
        return 1;
    }

    for ( nn = 0; nn < MAX_THREADS; nn++ ) {
        if ( pthread_create( &t[nn], NULL, thread_func, &semaphore ) < 0 ) {
            printf("Could not create thread %d: %s\n", nn+1, strerror(errno) );
            return 2;
        }
    }
    sleep( 1 );

    for (nn = 0; nn < MAX_THREADS; nn++) {
        sem_post(&semaphore);
    }

    for ( nn = 0; nn < MAX_THREADS; nn++) {
        void* result;
        pthread_join(t[nn], &result);
        if (result != (void*)t[nn]) {
            printf("Thread %p joined but returned %p\n", (void*)t[nn], result);
        }
    }

    if (sem_getvalue(&semaphore, &value) < 0) {
        printf("Could not get semaphore value: %s\n", strerror(errno));
        return 3;
    }
    if (value != 1) {
        printf("Error: Semaphore value = %d\n", value);
        return 4;
    }
    return 0;
}
