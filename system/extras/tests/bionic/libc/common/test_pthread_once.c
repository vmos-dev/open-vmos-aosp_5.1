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
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define  N_THREADS  100

static pthread_once_t   once = PTHREAD_ONCE_INIT;

static int      global_count = 0;

static void
once_function( void )
{
    struct timespec ts;

    global_count += 1;

    ts.tv_sec = 2;
    ts.tv_nsec = 0;
    nanosleep (&ts, NULL);
}

static void*
thread_function(void*  arg)
{
    pthread_once( &once, once_function );

    if (global_count != 1) {
        printf ("thread %ld: global == %d\n", (long int) arg, global_count);
        exit (1);
    }
    return NULL;
}

int  main( void )
{
    pthread_t   threads[N_THREADS];
    int         nn;

    for (nn = 0; nn < N_THREADS; nn++) {
        if (pthread_create( &threads[nn], NULL, thread_function, (void*)(long int)nn) < 0) {
            printf("creation of thread %d failed\n", nn);
            return 1;
        }
    }

    for (nn = 0; nn < N_THREADS; nn++) {
        if (pthread_join(threads[nn], NULL)) {
            printf("joining thread %d failed\n", nn);
            return 1;
        }
    }
    return 0;
}
