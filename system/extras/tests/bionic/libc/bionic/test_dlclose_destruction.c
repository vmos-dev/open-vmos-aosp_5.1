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

/* this program is used to check that static C++ destructors are
 * properly called when dlclose() is called. We do this by using
 * a helper C++ shared library.
 *
 * See libdlclosetest1.cpp for details.
 */
#include <dlfcn.h>
#include <stdio.h>

static int
check_library(const char*  libname)
{
    void*  lib = dlopen(libname, RTLD_NOW);
    int*   to_x;
    void  (*set_y)(int *);
    int    y = 0;

    if (lib == NULL) {
        fprintf(stderr, "Could not load shared library %s: %s\n", libname, dlerror());
        return 1;
    }

    fprintf(stderr, "%s loaded.\n", libname);

    to_x = dlsym(lib, "x");
    if (to_x == NULL) {
        fprintf(stderr, "Could not access global DLL variable (x) in %s: %s\n", libname, dlerror());
        return 10;
    }

    if (*to_x != 1) {
        fprintf(stderr, "Constructor was not run on dlopen(\"%s\") !\n", libname);
        return 11;
    }

    set_y = dlsym(lib, "set_y");
    if (set_y == NULL) {
        fprintf(stderr, "Could not access global DLL function (set_y) in %s: %s\n", libname, dlerror());
        return 12;
    }

    y = 0;
    (*set_y)(&y);

    if (dlclose(lib) < 0) {
        fprintf(stderr, "Could not unload shared library %s: %s\n", libname, dlerror());
        return 2;
    }

    fprintf(stderr, "%s unloaded.\n", libname);
    if (y != 2) {
        fprintf(stderr, "Static destructors was not called on dlclose()!\n");
        return 2;
    }
    return 0;
}

int main(void)
{
    /* Testing static C++ construction/destruction */
    if (check_library("libdlclosetest1.so"))
        return 1;
    if (check_library("libdlclosetest2.so"))
        return 2;
    return 0;
}
