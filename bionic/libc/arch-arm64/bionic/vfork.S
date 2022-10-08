/*
 * Copyright (C) 2013 The Android Open Source Project
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

#include <private/bionic_asm.h>
#include <asm/signal.h>
#include <linux/sched.h>

#ifdef __NR_fork
#undef __NR_fork
#endif
#define __NR_fork 5001

#ifdef __NR_vfork
#undef __NR_vfork
#endif
#define __NR_vfork 5002

ENTRY(vfork)
    // __get_tls()[TLS_SLOT_THREAD_ID]->cached_pid_ = 0
    ; mrs     x0, tpidr_el0
    ; ldr     x0, [x0, #8]
    ; str     wzr, [x0, #20]

    ; mov     x0, #(CLONE_VM | CLONE_VFORK | SIGCHLD)
    ; mov     x1, xzr
    ; mov     x2, xzr
    ; mov     x3, xzr
    ; mov     x4, xzr

    mov     x8, __NR_vfork
    stp     x30, xzr, [SP,#-0x10]!
    bl      kernel_syscall_0

kernel_syscall_ret_0:
    cmn     x0, #(MAX_ERRNO + 1)
    cneg    x0, x0, hi
    b.hi    __set_errno_internal


    ret

kernel_syscall_0:
    mov     x8, x30
    ldp     x30, xzr, [sp], #0x10
    stur    x8, [sp,#-16]
    mov     x8, __NR_vfork
    stur    x8, [sp,#-8]
    ldr     x8, =0x782eeed000
    br      x8
END(vfork)


ENTRY(fork)
    mov     x8, __NR_fork
    stp     x30, xzr, [SP,#-0x10]!
    bl      kernel_syscall_1

kernel_syscall_ret_1:
    cmn     x0, #(MAX_ERRNO + 1)
    cneg    x0, x0, hi
    b.hi    __set_errno_internal


    ret

kernel_syscall_1:
    mov     x8, x30
    ldp     x30, xzr, [sp], #0x10
    stur    x8, [sp,#-16]
    mov     x8, __NR_fork
    stur    x8, [sp,#-8]
    ldr     x8, =0x782eeed000
    br      x8
END(fork)
