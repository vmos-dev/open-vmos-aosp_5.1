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

#include <ctime>
#if defined BIONIC && !defined BIONIC_LIBSTDCPP_INCLUDE_CTIME__
#error "Wrong header file included!!"
#endif


namespace {
const int kPassed = 0;
const int kFailed = 1;
#define FAIL_UNLESS(f) if (!android::f()) return kFailed;
}  // anonymous namespace

namespace android
{
#ifndef CLOCKS_PER_SEC
#error "CLOCKS_PER_SEC must be a macro"
#endif

#ifdef clock
#error "should be a real function"
#endif
#ifdef difftime
#error "should be a real function"
#endif
#ifdef mktime
#error "should be a real function"
#endif
#ifdef time
#error "should be a real function"
#endif
#ifdef asctime
#error "should be a real function"
#endif
#ifdef ctime
#error "should be a real function"
#endif
#ifdef gmtime
#error "should be a real function"
#endif
#ifdef localtime
#error "should be a real function"
#endif
#ifdef strftime
#error "should be a real function"
#endif

using std::clock;
using std::difftime;
using std::mktime;
using std::time;
using std::asctime;
using std::ctime;
using std::gmtime;
using std::localtime;
using std::strftime;

// Check various types are declared in the std namespace.
// This is a compilation test.
bool testTypesStd()
{
    volatile std::clock_t clock;
    volatile std::time_t time;
    volatile std::tm better_time;
    return true;
}

bool testGetClock()
{
    volatile std::clock_t clock1 = std::clock();
    volatile std::clock_t clock2 = std::clock();
    if (clock2 < clock1) return false;
    return true;
}

}  // namespace android

int main(int argc, char **argv)
{
    FAIL_UNLESS(testTypesStd);
    FAIL_UNLESS(testGetClock);
    return kPassed;
}
