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
#include <stdio.h>
#include <grp.h>

#define  MAX_GROUPS  100
#define  TEST_GROUP  1337
#define  TEST_USER   "nobodyisreallyhere"

int  main(void)
{
    int    count = MAX_GROUPS;
    gid_t  groups[MAX_GROUPS];
    int    ret;

    /* this only tests the funky behaviour of our stubbed getgrouplist()
     * implementation. which should only return TEST_GROUP, independent
     * of the user
     */
    ret = getgrouplist( TEST_USER, TEST_GROUP, groups, &count );
    if (ret != 1) {
        fprintf(stderr, "getgrouplist() returned %d (expecting 1), ngroups=%d\n", 
                ret, count);
        return 1;
    }
    if (groups[0] != TEST_GROUP) {
        fprintf(stderr, "getgrouplist() returned group %d (expecting %d)\n",
                        groups[0], TEST_GROUP);
        return 1;
    }
    printf ("ok\n");
    return 0;
}
