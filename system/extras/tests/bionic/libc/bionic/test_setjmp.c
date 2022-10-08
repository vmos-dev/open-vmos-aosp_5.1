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

/* Basic test of setjmp() functionality */

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

#define INT_VALUE1   0x12345678
#define INT_VALUE2   0xfedcba98

#define FLOAT_VALUE1   (1.2345678)
#define FLOAT_VALUE2   (9.8765432)

int     dummy_int;
double  dummy_double;

/* test that integer registers are restored properly */
static void
test_int(void)
{
    jmp_buf  jumper;
    register int xx = INT_VALUE1;

    if (setjmp(jumper) == 0) {
        xx = INT_VALUE2;
        longjmp(jumper, 1);
    } else {
        if (xx != INT_VALUE1) {
            fprintf(stderr, "setjmp() is broken for integer registers !\n");
            exit(1);
        }
    }
    dummy_int = xx;
}

static void
test_float(void)
{
    jmp_buf  jumper;
    register double xx = FLOAT_VALUE1;

    if (setjmp(jumper) == 0) {
        xx = FLOAT_VALUE2;
        longjmp(jumper, 1);
    } else {
        if (xx != FLOAT_VALUE1) {
            fprintf(stderr, "setjmp() is broken for floating point registers !\n");
            exit(1);
        }
    }
    dummy_double = xx;
}

int main(void)
{
    test_int();
    test_float();
    return 0;
}
