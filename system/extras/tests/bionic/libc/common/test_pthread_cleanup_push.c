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


#define MAGIC1 0xcafebabeU
#define MAGIC2 0x8badf00dU
#define MAGIC3 0x12345667U

static int g_ok1 = 0;
static int g_ok2 = 0;
static int g_ok3 = 0;

static void
cleanup1( void* arg )
{
    if ((unsigned)arg != MAGIC1)
        g_ok1 = -1;
    else
        g_ok1 = +1;
}

static void
cleanup2( void* arg )
{
    if ((unsigned)arg != MAGIC2) {
        g_ok2 = -1;
    } else
        g_ok2 = +1;
}

static void
cleanup3( void* arg )
{
    if ((unsigned)arg != MAGIC3)
        g_ok3 = -1;
    else
        g_ok3 = +1;
}


static void*
thread1_func( void* arg )
{
    pthread_cleanup_push( cleanup1, (void*)MAGIC1 );
    pthread_cleanup_push( cleanup2, (void*)MAGIC2 );
    pthread_cleanup_push( cleanup3, (void*)MAGIC3 );

    if (arg != NULL)
        pthread_exit(0);

    pthread_cleanup_pop(0);
    pthread_cleanup_pop(1);
    pthread_cleanup_pop(1);

    return NULL;
}

static int test( int do_exit )
{
    pthread_t t;

    pthread_create( &t, NULL, thread1_func, (void*)do_exit );
    pthread_join( t, NULL );

    if (g_ok1 != +1) {
        if (g_ok1 == 0) {
            fprintf(stderr, "cleanup1 not called !!\n");
        } else {
            fprintf(stderr, "cleanup1 called with wrong argument\n" );
        }
        exit(1);
    }
    else if (g_ok2 != +1) {
        if (g_ok2 == 0)
            fprintf(stderr, "cleanup2 not called !!\n");
        else
            fprintf(stderr, "cleanup2 called with wrong argument\n");
        exit(2);
    }
    else if (do_exit && g_ok3 != +1) {
        if (g_ok3 == 0) {
            fprintf(stderr, "cleanup3 not called !!\n");
        } else {
            fprintf(stderr, "cleanup3 called with bad argument !!\n");
        }
        exit(3);
    }
    else if (!do_exit && g_ok3 != 0) {
        if (g_ok3 == 1) {
            fprintf(stderr, "cleanup3 wrongly called !!\n");
        } else {
            fprintf(stderr, "cleanup3 wrongly called with bad argument !!\n");
        }
        exit(3);
    }

    return 0;
}

int main( void )
{
    test(0);
    test(1);
    printf("OK\n");
    return 0;
}
