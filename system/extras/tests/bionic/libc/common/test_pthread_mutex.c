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

#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

/* Posix states that EDEADLK should be returned in case a deadlock condition
 * is detected with a PTHREAD_MUTEX_ERRORCHECK lock() or trylock(), but
 * GLibc returns EBUSY instead.
 */
#ifdef HOST
#  define ERRNO_PTHREAD_EDEADLK   EBUSY
#else
#  define ERRNO_PTHREAD_EDEADLK   EDEADLK
#endif

static void __attribute__((noreturn))
panic(const char* func, const char* format, ...)
{
    va_list  args;
    fprintf(stderr, "%s: ", func);
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
    exit(1);
}

#define  PANIC(...)   panic(__FUNCTION__,__VA_ARGS__)

static void __attribute__((noreturn))
error(int  errcode, const char* func, const char* format, ...)
{
    va_list  args;
    fprintf(stderr, "%s: ", func);
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, " error=%d: %s\n", errcode, strerror(errcode));
    exit(1);
}

/* return current time in seconds as floating point value */
static double
time_now(void)
{
    struct timespec ts[1];

    clock_gettime(CLOCK_MONOTONIC, ts);
    return (double)ts->tv_sec + ts->tv_nsec/1e9;
}

static void
time_sleep(double  delay)
{
    struct timespec ts;
    int             ret;

    ts.tv_sec  = (time_t)delay;
    ts.tv_nsec = (long)((delay - ts.tv_sec)*1e9);

    do {
        ret = nanosleep(&ts, &ts);
    } while (ret < 0 && errno == EINTR);
}

#define  ERROR(errcode,...)   error((errcode),__FUNCTION__,__VA_ARGS__)

#define  TZERO(cond)   \
    { int _ret = (cond); if (_ret != 0) ERROR(_ret,"%d:%s", __LINE__, #cond); }

#define  TTRUE(cond)   \
    { if (!(cond)) PANIC("%d:%s", __LINE__, #cond); }

#define  TFALSE(cond)   \
    { if (!!(cond)) PANIC("%d:%s", __LINE__, #cond); }

#define  TEXPECT_INT(cond,val) \
    { int _ret = (cond); if (_ret != (val)) PANIC("%d:%s returned %d (%d expected)", __LINE__, #cond, _ret, (val)); }

/* perform a simple init/lock/unlock/destroy test on a mutex of given attributes */
static void do_test_mutex_1(pthread_mutexattr_t *attr)
{
    int              ret;
    pthread_mutex_t  lock[1];

    TZERO(pthread_mutex_init(lock, attr));
    TZERO(pthread_mutex_lock(lock));
    TZERO(pthread_mutex_unlock(lock));
    TZERO(pthread_mutex_destroy(lock));
}

static void set_mutexattr_type(pthread_mutexattr_t *attr, int type)
{
    int  newtype;
    TZERO(pthread_mutexattr_settype(attr, type));
    newtype = ~type;
    TZERO(pthread_mutexattr_gettype(attr, &newtype));
    TEXPECT_INT(newtype,type);
}

/* simple init/lock/unlock/destroy on all mutex types */
static void do_test_1(void)
{
    int                  ret, type;
    pthread_mutexattr_t  attr[1];

    do_test_mutex_1(NULL);

    /* non-shared version */

    TZERO(pthread_mutexattr_init(attr));

    set_mutexattr_type(attr, PTHREAD_MUTEX_NORMAL);
    do_test_mutex_1(attr);

    set_mutexattr_type(attr, PTHREAD_MUTEX_RECURSIVE);
    do_test_mutex_1(attr);

    set_mutexattr_type(attr, PTHREAD_MUTEX_ERRORCHECK);
    do_test_mutex_1(attr);

    TZERO(pthread_mutexattr_destroy(attr));

    /* shared version */
    TZERO(pthread_mutexattr_init(attr));
    TZERO(pthread_mutexattr_setpshared(attr, PTHREAD_PROCESS_SHARED));

    set_mutexattr_type(attr, PTHREAD_MUTEX_NORMAL);
    do_test_mutex_1(attr);

    set_mutexattr_type(attr, PTHREAD_MUTEX_RECURSIVE);
    do_test_mutex_1(attr);

    set_mutexattr_type(attr, PTHREAD_MUTEX_ERRORCHECK);
    do_test_mutex_1(attr);

    TZERO(pthread_mutexattr_destroy(attr));
}

/* perform init/trylock/unlock/destroy then init/lock/trylock/destroy */
static void do_test_mutex_2(pthread_mutexattr_t *attr)
{
    pthread_mutex_t  lock[1];

    TZERO(pthread_mutex_init(lock, attr));
    TZERO(pthread_mutex_trylock(lock));
    TZERO(pthread_mutex_unlock(lock));
    TZERO(pthread_mutex_destroy(lock));

    TZERO(pthread_mutex_init(lock, attr));
    TZERO(pthread_mutex_trylock(lock));
    TEXPECT_INT(pthread_mutex_trylock(lock),EBUSY);
    TZERO(pthread_mutex_unlock(lock));
    TZERO(pthread_mutex_destroy(lock));
}

static void do_test_mutex_2_rec(pthread_mutexattr_t *attr)
{
    pthread_mutex_t  lock[1];

    TZERO(pthread_mutex_init(lock, attr));
    TZERO(pthread_mutex_trylock(lock));
    TZERO(pthread_mutex_unlock(lock));
    TZERO(pthread_mutex_destroy(lock));

    TZERO(pthread_mutex_init(lock, attr));
    TZERO(pthread_mutex_trylock(lock));
    TZERO(pthread_mutex_trylock(lock));
    TZERO(pthread_mutex_unlock(lock));
    TZERO(pthread_mutex_unlock(lock));
    TZERO(pthread_mutex_destroy(lock));
}

static void do_test_mutex_2_chk(pthread_mutexattr_t *attr)
{
    pthread_mutex_t  lock[1];

    TZERO(pthread_mutex_init(lock, attr));
    TZERO(pthread_mutex_trylock(lock));
    TZERO(pthread_mutex_unlock(lock));
    TZERO(pthread_mutex_destroy(lock));

    TZERO(pthread_mutex_init(lock, attr));
    TZERO(pthread_mutex_trylock(lock));
    TEXPECT_INT(pthread_mutex_trylock(lock),ERRNO_PTHREAD_EDEADLK);
    TZERO(pthread_mutex_unlock(lock));
    TZERO(pthread_mutex_destroy(lock));
}

static void do_test_2(void)
{
    pthread_mutexattr_t  attr[1];

    do_test_mutex_2(NULL);

    /* non-shared version */

    TZERO(pthread_mutexattr_init(attr));

    set_mutexattr_type(attr, PTHREAD_MUTEX_NORMAL);
    do_test_mutex_2(attr);

    set_mutexattr_type(attr, PTHREAD_MUTEX_RECURSIVE);
    do_test_mutex_2_rec(attr);

    set_mutexattr_type(attr, PTHREAD_MUTEX_ERRORCHECK);
    do_test_mutex_2_chk(attr);

    TZERO(pthread_mutexattr_destroy(attr));

    /* shared version */
    TZERO(pthread_mutexattr_init(attr));
    TZERO(pthread_mutexattr_setpshared(attr, PTHREAD_PROCESS_SHARED));

    set_mutexattr_type(attr, PTHREAD_MUTEX_NORMAL);
    do_test_mutex_2(attr);

    set_mutexattr_type(attr, PTHREAD_MUTEX_RECURSIVE);
    do_test_mutex_2_rec(attr);

    set_mutexattr_type(attr, PTHREAD_MUTEX_ERRORCHECK);
    do_test_mutex_2_chk(attr);

    TZERO(pthread_mutexattr_destroy(attr));
}

/* This is more complex example to test contention of mutexes.
 * Essentially, what happens is this:
 *
 * - main thread creates a mutex and locks it
 * - it then creates thread 1 and thread 2
 *
 * - it then record the current time, sleep for a specific 'waitDelay'
 *   then unlock the mutex.
 *
 * - thread 1 locks() the mutex. It shall be stopped for a least 'waitDelay'
 *   seconds. It then unlocks the mutex.
 *
 * - thread 2 trylocks() the mutex. In case of failure (EBUSY), it waits
 *   for a small amount of time (see 'spinDelay') and tries again, until
 *   it succeeds. It then unlocks the mutex.
 *
 * The goal of this test is to verify that thread 1 has been stopped
 * for a sufficiently long time, and that thread 2 has been spinning for
 * the same minimum period. There is no guarantee as to which thread is
 * going to acquire the mutex first.
 */
typedef struct {
    pthread_mutex_t  mutex[1];
    double           t0;
    double           waitDelay;
    double           spinDelay;
} Test3State;

static void* do_mutex_test_3_t1(void* arg)
{
    Test3State *s = arg;
    double      t1;

    TZERO(pthread_mutex_lock(s->mutex));
    t1 = time_now();
    //DEBUG ONLY: printf("t1-s->t0=%g waitDelay=%g\n", t1-s->t0, s->waitDelay);
    TTRUE((t1-s->t0) >= s->waitDelay); 
    TZERO(pthread_mutex_unlock(s->mutex));
    return NULL;
}

static void* do_mutex_test_3_t2(void* arg)
{
    Test3State *s = arg;
    double      t1;

    for (;;) {
        int ret = pthread_mutex_trylock(s->mutex);
        if (ret == 0)
            break;
        if (ret == EBUSY) {
            time_sleep(s->spinDelay);
            continue;
        }
    }
    t1 = time_now();
    TTRUE((t1-s->t0) >= s->waitDelay);
    TZERO(pthread_mutex_unlock(s->mutex));
    return NULL;
}


static void do_test_mutex_3(pthread_mutexattr_t *attr, double delay)
{
    Test3State  s[1];
    pthread_t   th1, th2;
    void*       dummy;

    TZERO(pthread_mutex_init(s->mutex, attr));
    s->waitDelay = delay;
    s->spinDelay = delay/20.;

    TZERO(pthread_mutex_lock(s->mutex));

    pthread_create(&th1, NULL, do_mutex_test_3_t1, s);
    pthread_create(&th2, NULL, do_mutex_test_3_t2, s);

    s->t0 = time_now();
    time_sleep(delay);

    TZERO(pthread_mutex_unlock(s->mutex));

    TZERO(pthread_join(th1, &dummy));
    TZERO(pthread_join(th2, &dummy));
}

static void do_test_3(double  delay)
{
    pthread_mutexattr_t  attr[1];

    do_test_mutex_3(NULL, delay);

    /* non-shared version */

    TZERO(pthread_mutexattr_init(attr));

    set_mutexattr_type(attr, PTHREAD_MUTEX_NORMAL);
    do_test_mutex_3(attr, delay);

    set_mutexattr_type(attr, PTHREAD_MUTEX_RECURSIVE);
    do_test_mutex_3(attr, delay);

    set_mutexattr_type(attr, PTHREAD_MUTEX_ERRORCHECK);
    do_test_mutex_3(attr, delay);

    TZERO(pthread_mutexattr_destroy(attr));

    /* shared version */
    TZERO(pthread_mutexattr_init(attr));
    TZERO(pthread_mutexattr_setpshared(attr, PTHREAD_PROCESS_SHARED));

    set_mutexattr_type(attr, PTHREAD_MUTEX_NORMAL);
    do_test_mutex_3(attr, delay);

    set_mutexattr_type(attr, PTHREAD_MUTEX_RECURSIVE);
    do_test_mutex_3(attr, delay);

    set_mutexattr_type(attr, PTHREAD_MUTEX_ERRORCHECK);
    do_test_mutex_3(attr, delay);

    TZERO(pthread_mutexattr_destroy(attr));
}


int main(void)
{
    do_test_1();
    do_test_2();
    do_test_3(0.1);
    return 0;
}
