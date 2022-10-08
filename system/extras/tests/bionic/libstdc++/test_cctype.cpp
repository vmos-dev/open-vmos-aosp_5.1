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

#include <cctype>
#if defined BIONIC && !defined BIONIC_LIBSTDCPP_INCLUDE_CCTYPE__
#error "Wrong header file included!!"
#endif


namespace {
const int kPassed = 0;
}  // anonymous namespace

namespace android
{
#ifdef isalnum
#error "should be a real function"
#endif
#ifdef isalpha
#error "should be a real function"
#endif
#ifdef iscntrl
#error "should be a real function"
#endif
#ifdef isdigit
#error "should be a real function"
#endif
#ifdef isgraph
#error "should be a real function"
#endif
#ifdef islower
#error "should be a real function"
#endif
#ifdef isprint
#error "should be a real function"
#endif
#ifdef ispunct
#error "should be a real function"
#endif
#ifdef isspace
#error "should be a real function"
#endif
#ifdef isupper
#error "should be a real function"
#endif
#ifdef isxdigit
#error "should be a real function"
#endif
#ifdef tolower
#error "should be a real function"
#endif
#ifdef toupper
#error "should be a real function"
#endif

using std::isalnum;
using std::isdigit;
using std::isprint;
using std::isupper;
using std::tolower;
using std::isalpha;
using std::isgraph;
using std::ispunct;
using std::isxdigit;
using std::toupper;
using std::iscntrl;
using std::islower;
using std::isspace;

}  // namespace android

int main(int argc, char **argv)
{
    return kPassed;
}
