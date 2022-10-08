/*
 * Copyright (C) 2009 The Android Open Source Project
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


#include <climits>
#if defined BIONIC && !defined BIONIC_LIBSTDCPP_INCLUDE_CLIMITS__
#error "Wrong header file included!!"
#endif


namespace {
const int kPassed = 0;
const int kFailed = 1;
#define FAIL_UNLESS(f) if (!android::f()) return kFailed;
}  // anonymous namespace

namespace android
{
bool testLimits()
{
    // char
    volatile char c1 = CHAR_BIT;
    volatile char c2 = CHAR_MAX;
    volatile char c3 = CHAR_MIN;

    // int
    volatile int i1 = INT_MAX;
    volatile int i2 = INT_MIN;

    // short
    volatile short s1 = SHRT_MAX;
    volatile short s2 = SHRT_MIN;

    // long
    volatile long l1 = LONG_MAX;
    volatile long l2 = LONG_MIN;

    // long long
    volatile long long ll1 = LLONG_MAX;
    volatile long long ll2 = LLONG_MIN;

    volatile unsigned long mb = MB_LEN_MAX;

    // signed char
    volatile signed char sc1 = SCHAR_MIN;
    volatile signed char sc2 = SCHAR_MAX;

    // unsigned
    volatile unsigned int ui = UINT_MAX;
    volatile unsigned short us = USHRT_MAX;
    volatile unsigned long ul = ULONG_MAX;
    volatile unsigned long long ull = ULLONG_MAX;

    return true;
}

}  // namespace android

int main(int argc, char **argv)
{
    FAIL_UNLESS(testLimits);
    return kPassed;
}
