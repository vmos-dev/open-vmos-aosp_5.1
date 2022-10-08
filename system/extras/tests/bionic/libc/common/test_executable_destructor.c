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
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/resource.h>

/* A very simple program used to test constructor and destructor functions
 * in executables (instead of shared libraries).
 */

int x = 0;

/* Initialize x to 1 when the program starts. This will be checked
 * later by the main() function.
 */
static void __attribute__((constructor))
on_load(void)
{
    x = 1;
}

/* Crash intentionally if 'x' is set to 1 */
static void __attribute__((destructor))
on_exit(void)
{
    if (x == 1)
        *(int*)(void*)0 = 10;  /* force a crash */
}

int main(void)
{
    int status;
    pid_t pid;

    /* First, check that the constructor was properly called ! */
    if (x != 1) {
        fprintf(stderr, "Constructor function not called!!\n");
        return 1;
    }

    /* prevent our crashing child process from generating a core file */
    {
        struct rlimit rlim;
        rlim.rlim_cur = 0;
        rlim.rlim_max = RLIM_INFINITY;
        setrlimit(RLIMIT_CORE, &rlim);
    }

    /* Fork the current process, then wait for the child to exit
     * and crash.
     */
    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Could not fork process: %s\n", strerror(errno));
        return 2;
    }
    /* in the child, simply exit after 1 second. */
    if (pid == 0) {
        sleep(1);
        return 0;
    }
    /* in the parent, wait for the child to terminate */
    if (wait(&status) < 0) {
        fprintf(stderr, "Could not wait for child: %s\n", strerror(errno));
        return 3;
    }
    if (!WIFSIGNALED(status)) {
        fprintf(stderr, "Destructor not called!!\n");
        return 4;
    }

    /* Prevent crashing */
    x = 2;
    printf("ok\n");
    return 0;
}
