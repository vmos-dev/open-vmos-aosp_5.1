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
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define  T(_name,_cond)                                               \
    errno = 0;                                                        \
    printf( "testing %-*s : ", 32, #_name );                          \
    ret   = sysconf( _name );                                         \
    if (ret < 0 && errno != 0) {                                      \
        printf( "error: %s\n", strerror(errno) );                     \
    } else {                                                          \
        if ( ret _cond )  {                                           \
            printf( "OK  (%d)\n", ret );                              \
        } else {                                                      \
            printf( "ERROR: %d does not meet %s\n", ret, #_cond );    \
        }                                                             \
    }

int  main( void )
{
    int  ret;
    T(_SC_ARG_MAX, > 0);
    T(_SC_BC_BASE_MAX, |1 );
    T(_SC_BC_DIM_MAX, |1 );
    T(_SC_BC_SCALE_MAX, |1 );
    T(_SC_BC_STRING_MAX, |1 );
    T(_SC_CHILD_MAX, >0 );
    T(_SC_CLK_TCK, >0 );
    T(_SC_COLL_WEIGHTS_MAX, |1 );
    T(_SC_EXPR_NEST_MAX, |1 );
    T(_SC_LINE_MAX, > 256 );
    T(_SC_NGROUPS_MAX, >0 );
    T(_SC_OPEN_MAX, >128 );
    T(_SC_PASS_MAX, |1 );
    T(_SC_2_C_BIND, >0 );
    T(_SC_2_C_DEV, |1 );
    T(_SC_2_C_VERSION, |1 );
    T(_SC_2_CHAR_TERM, |1 );
    T(_SC_2_FORT_DEV, |1 );
    T(_SC_2_FORT_RUN, |1 );
    T(_SC_2_LOCALEDEF, |1 );
    T(_SC_2_SW_DEV, |1 );
    T(_SC_2_UPE, |1 );
    T(_SC_2_VERSION, |1);
    T(_SC_JOB_CONTROL, == 1);
    T(_SC_SAVED_IDS, == 1);
    T(_SC_VERSION, |1);
    T(_SC_RE_DUP_MAX, |1);
    T(_SC_STREAM_MAX, > 0);
    T(_SC_TZNAME_MAX, |1 );
    T(_SC_XOPEN_CRYPT, |1 );
    T(_SC_XOPEN_ENH_I18N, |1 );
    T(_SC_XOPEN_SHM, |1 );
    T(_SC_XOPEN_VERSION, |1 );
    T(_SC_XOPEN_XCU_VERSION, |1 );
    T(_SC_XOPEN_REALTIME, |1 );
    T(_SC_XOPEN_REALTIME_THREADS, |1 );
    T(_SC_XOPEN_LEGACY, |1 );
    T(_SC_ATEXIT_MAX, >32 );
    T(_SC_IOV_MAX, >0 );
    T(_SC_PAGESIZE, == 4096 );
    T(_SC_PAGE_SIZE, == 4096 );
    T(_SC_XOPEN_UNIX, |1 );
    T(_SC_XBS5_ILP32_OFF32, |1 );
    T(_SC_XBS5_ILP32_OFFBIG, |1 );
    T(_SC_XBS5_LP64_OFF64, |1 );
    T(_SC_XBS5_LPBIG_OFFBIG, |1 );
    T(_SC_AIO_LISTIO_MAX, |1 );
    T(_SC_AIO_MAX, |1 );
    T(_SC_AIO_PRIO_DELTA_MAX, |1 );
    T(_SC_DELAYTIMER_MAX, >0 );
    T(_SC_MQ_OPEN_MAX, |1 );
    T(_SC_MQ_PRIO_MAX, >0 );
    T(_SC_RTSIG_MAX, |1 );
    T(_SC_SEM_NSEMS_MAX, |1 );
    T(_SC_SEM_VALUE_MAX, |1 );
    T(_SC_SIGQUEUE_MAX, >0 );
    T(_SC_TIMER_MAX, |1 );
    T(_SC_ASYNCHRONOUS_IO, |1 );
    T(_SC_FSYNC, |1 );
    T(_SC_MAPPED_FILES, |1 );
    T(_SC_MEMLOCK, |1 );
    T(_SC_MEMLOCK_RANGE, |1 );
    T(_SC_MEMORY_PROTECTION, |1 );
    T(_SC_MESSAGE_PASSING, |1 );
    T(_SC_PRIORITIZED_IO, |1 );
    T(_SC_PRIORITY_SCHEDULING, |1 );
    T(_SC_REALTIME_SIGNALS, |1 );
    T(_SC_SEMAPHORES, |1 );
    T(_SC_SHARED_MEMORY_OBJECTS, |1 );
    T(_SC_SYNCHRONIZED_IO, |1 );
    T(_SC_TIMERS, |1 );
    T(_SC_GETGR_R_SIZE_MAX, |1 );
    T(_SC_GETPW_R_SIZE_MAX, |1 );
    T(_SC_LOGIN_NAME_MAX, |1 );
    T(_SC_THREAD_DESTRUCTOR_ITERATIONS, |1 );
    T(_SC_THREAD_KEYS_MAX, > 0 );
    T(_SC_THREAD_STACK_MIN, >= 8192 );
    T(_SC_THREAD_THREADS_MAX, |1 );
    T(_SC_TTY_NAME_MAX, > 0 );
    T(_SC_THREADS, |1 );
    T(_SC_THREAD_ATTR_STACKADDR, |1 );
    T(_SC_THREAD_ATTR_STACKSIZE, |1 );
    T(_SC_THREAD_PRIORITY_SCHEDULING, |1 );
    T(_SC_THREAD_PRIO_INHERIT, |1 );
    T(_SC_THREAD_PRIO_PROTECT, |1 );
    T(_SC_THREAD_SAFE_FUNCTIONS, |1 );
    return 0;
}
