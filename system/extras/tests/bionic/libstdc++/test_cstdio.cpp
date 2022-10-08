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

#include <cstdio>
#if defined BIONIC && !defined BIONIC_LIBSTDCPP_INCLUDE_CSTDIO__
#error "Wrong header file included!!"
#endif

namespace {
const int kPassed = 0;
const int kFailed = 1;
#define FAIL_UNLESS(f) if (!android::f()) return kFailed;
}  // anonymous namespace

namespace android
{
#ifndef BUFSIZ
#error "BUFSIZ must be a macro"
#endif

#ifndef EOF
#error "EOF must be a macro"
#endif

#ifndef FILENAME_MAX
#error "FILENAME_MAX must be a macro"
#endif

#ifndef FOPEN_MAX
#error "FOPEN_MAX must be a macro"
#endif

#ifndef L_tmpnam
#error "L_tmpnam must be a macro"
#endif

#ifndef NULL
#error "NULL must be a macro"
#endif

#ifndef SEEK_CUR
#error "SEEK_CUR must be a macro"
#endif

#ifndef SEEK_END
#error "SEEK_END must be a macro"
#endif
#ifndef SEEK_SET
#error "SEEK_SET must be a macro"
#endif

#ifndef TMP_MAX
#error "TMP_MAX must be a macro"
#endif

#ifndef _IOFBF
#error "_IOFBF must be a macro"
#endif

#ifndef _IOLBF
#error "_IOLBF must be a macro"
#endif

#ifndef _IONBF
#error "_IONBF must be a macro"
#endif

#ifndef stderr
#error "stderr must be a macro"
#endif

#ifndef stdin
#error "stdin must be a macro"
#endif

#ifndef stdout
#error "stdout must be a macro"
#endif

using std::clearerr;
using std::fclose;
using std::feof;
using std::ferror;
using std::fflush;
using std::fgetc;
using std::fgetpos;
using std::fgets;
using std::fopen;
using std::fprintf;
using std::fputc;
using std::fputs;
using std::fread;
using std::freopen;
using std::fscanf;
using std::fseek;
using std::fsetpos;
using std::ftell;
using std::fwrite;
using std::getc;
using std::getchar;
using std::gets;
using std::perror;
using std::printf;
using std::putc;
using std::putchar;
using std::puts;
using std::remove;
using std::rename;
using std::rewind;
using std::scanf;
using std::setbuf;
using std::setvbuf;
using std::sprintf;
using std::sscanf;
using std::tmpfile;
using std::tmpnam;
using std::ungetc;
using std::vfprintf;
using std::vprintf;
using std::vsprintf;

using std::snprintf;
using std::vfscanf;
using std::vscanf;
using std::vsnprintf;
using std::vsscanf;

bool testTypesStd()
{
    volatile std::size_t size;
    volatile std::FILE file;
    volatile std::fpos_t fpos_t;
    return true;
}
}  // namespace android

int main(int argc, char **argv)
{
    FAIL_UNLESS(testTypesStd);
    return kPassed;
}
