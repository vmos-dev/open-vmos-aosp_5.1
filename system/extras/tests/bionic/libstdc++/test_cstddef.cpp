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

#include <cstddef>
#if defined BIONIC && !defined BIONIC_LIBSTDCPP_INCLUDE_CSTDDEF__
#error "Wrong header file included!!"
#endif


namespace {
const int kPassed = 0;
const int kFailed = 1;
#define FAIL_UNLESS(f) if (!android::f()) return kFailed;
}  // anonymous namespace

namespace android {
// Dummy struct used to calculate offset of some of its fields.
struct Foo
{
    char field1;
    char field2;
};

// Check various types are declared in the std namespace.
bool testTypesStd()
{
    // size_t should be defined in both namespaces
    volatile ::size_t size_t_in_top_ns = 0;
    volatile ::std::size_t size_t_in_std_ns = 0;

    if (sizeof(::size_t) != sizeof(::std::size_t))
    {
        return false;
    }

    // ptrdiff_t should be defined in both namespaces
    volatile ::ptrdiff_t ptrdiff_t_in_top_ns = 0;
    volatile ::std::ptrdiff_t ptrdiff_t_in_std_ns = 0;

    if (sizeof(::ptrdiff_t) != sizeof(::std::ptrdiff_t))
    {
        return false;
    }
    // NULL is only in the top namespace
    volatile int *null_is_defined = NULL;
    return true;
}

bool testOffsetOf()
{
#ifndef offsetof
#error "offsetof is not a macro"
#endif

    // offsetof is only in the top namespace
    volatile size_t offset = offsetof(struct Foo, field2);
    return offset == 1;
}

bool testNull()
{
#ifndef NULL
#error "NULL is not a macro"
#endif
    // If NULL is void* this will issue a warning.
    volatile int null_is_not_void_star = NULL;
    return true;
}

}  // android namespace

int main(int argc, char **argv)
{
    FAIL_UNLESS(testTypesStd);
    FAIL_UNLESS(testOffsetOf);
    FAIL_UNLESS(testNull);
    return kPassed;
}
