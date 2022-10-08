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
/* this test is used to check that a memory-leak in vfprintf was fixed.
 * the initial code leaked heap memory each time a formatted double was printed
 */
#include <stdio.h>

extern size_t  dlmalloc_footprint();

int  main(void)
{
    size_t   initial = dlmalloc_footprint();
    size_t   final;
    char     temp[64];
    int      n;

    for (n = 0; n < 10000; n++)
        snprintf( temp, sizeof(temp), "%g", n*0.647287623 );

    final   = dlmalloc_footprint();
    /* vfprintf uses temporary heap blocks to do the formatting, so */
    /* it's OK to have one page in there                            */
    if (final <= 4096) {
        printf( "OK: initial = %ld, final == %ld\n", (long)initial, (long)final );
        return 0;
    } else {
        fprintf(stderr, "KO: initial == %ld, final == %ld\n", (long)initial, (long)final );
        return 1;
    }
}
