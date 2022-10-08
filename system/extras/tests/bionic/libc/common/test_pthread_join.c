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
#include <unistd.h>

static void*
thread1_func(void* arg)
{
    usleep( 2000*1000 );
    printf("thread 1 exited\n");
    return (void*) 0x8badf00d;
}

static void*
thread2_func(void* arg)
{
    pthread_t t1 = (pthread_t)arg;
    void* result;

    pthread_join(t1, &result);
    printf("thread2 received code %08x from thread1\n", (int)result);
    return NULL;
}


static void*
thread3_func(void* arg)
{
    pthread_t t1 = (pthread_t)arg;
    void* result;

    pthread_join(t1, &result);
    printf("thread3 received code %08x from thread1\n", (int)result);
    return NULL;
}

int main( void )
{
    pthread_t t1, t2, t3;

    pthread_create( &t1, NULL, thread1_func, NULL );
    pthread_create( &t2, NULL, thread2_func, (void*)t1 );
    pthread_create( &t3, NULL, thread3_func, (void*)t1 );

    pthread_join(t2, NULL);
    pthread_join(t3, NULL);

    printf("OK\n");
    return 0;
}
