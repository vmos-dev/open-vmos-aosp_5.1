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

/* perform a simple init/lock/unlock/destroy test on a rwlock of given attributes */
static void do_test_rwlock_rd1(pthread_rwlockattr_t *attr)
{
    int               ret;
    pthread_rwlock_t  lock[1];

    TZERO(pthread_rwlock_init(lock, attr));
    TZERO(pthread_rwlock_rdlock(lock));
    TZERO(pthread_rwlock_unlock(lock));
    TZERO(pthread_rwlock_destroy(lock));
}

static void do_test_rwlock_wr1(pthread_rwlockattr_t *attr)
{
    int               ret;
    pthread_rwlock_t  lock[1];

    TZERO(pthread_rwlock_init(lock, attr));
    TZERO(pthread_rwlock_wrlock(lock));
    TZERO(pthread_rwlock_unlock(lock));
    TZERO(pthread_rwlock_destroy(lock));
}

static void set_rwlockattr_shared(pthread_rwlockattr_t *attr, int shared)
{
    int  newshared;
    TZERO(pthread_rwlockattr_setpshared(attr, shared));
    newshared = ~shared;
    TZERO(pthread_rwlockattr_getpshared(attr, &newshared));
    TEXPECT_INT(newshared,shared);
}

/* simple init/lock/unlock/destroy on all rwlock types */
static void do_test_1(void)
{
    int                  ret, type;
    pthread_rwlockattr_t  attr[1];

    do_test_rwlock_rd1(NULL);
    do_test_rwlock_wr1(NULL);

    /* non-shared version */

    TZERO(pthread_rwlockattr_init(attr));

    set_rwlockattr_shared(attr, PTHREAD_PROCESS_PRIVATE);
    do_test_rwlock_rd1(attr);
    do_test_rwlock_wr1(attr);

    set_rwlockattr_shared(attr, PTHREAD_PROCESS_SHARED);
    do_test_rwlock_rd1(attr);
    do_test_rwlock_wr1(attr);

    TZERO(pthread_rwlockattr_destroy(attr));
}

static void do_test_rwlock_rd2_rec(pthread_rwlockattr_t *attr)
{
    pthread_rwlock_t  lock[1];

    TZERO(pthread_rwlock_init(lock, attr));
    TZERO(pthread_rwlock_tryrdlock(lock));
    TZERO(pthread_rwlock_unlock(lock));
    TZERO(pthread_rwlock_destroy(lock));

    TZERO(pthread_rwlock_init(lock, attr));
    TZERO(pthread_rwlock_tryrdlock(lock));
    TZERO(pthread_rwlock_tryrdlock(lock));
    TZERO(pthread_rwlock_unlock(lock));
    TZERO(pthread_rwlock_unlock(lock));
    TZERO(pthread_rwlock_destroy(lock));
}

static void do_test_rwlock_wr2_rec(pthread_rwlockattr_t *attr)
{
    pthread_rwlock_t  lock[1];

    TZERO(pthread_rwlock_init(lock, attr));
    TZERO(pthread_rwlock_trywrlock(lock));
    TZERO(pthread_rwlock_unlock(lock));
    TZERO(pthread_rwlock_destroy(lock));

    TZERO(pthread_rwlock_init(lock, attr));
    TZERO(pthread_rwlock_trywrlock(lock));
#ifdef HOST
    /* The host implementation (GLibc) does not support recursive
     * write locks */
    TEXPECT_INT(pthread_rwlock_trywrlock(lock),EBUSY);
#else
    /* Our implementation supports recursive write locks ! */
    TZERO(pthread_rwlock_trywrlock(lock));
    TZERO(pthread_rwlock_unlock(lock));
#endif
    TZERO(pthread_rwlock_unlock(lock));
    TZERO(pthread_rwlock_destroy(lock));
}

static void do_test_2(void)
{
    pthread_rwlockattr_t  attr[1];

    do_test_rwlock_rd2_rec(NULL);
    do_test_rwlock_wr2_rec(NULL);

    /* non-shared version */

    TZERO(pthread_rwlockattr_init(attr));

    set_rwlockattr_shared(attr, PTHREAD_PROCESS_PRIVATE);
    do_test_rwlock_rd2_rec(attr);
    do_test_rwlock_wr2_rec(attr);

    set_rwlockattr_shared(attr, PTHREAD_PROCESS_SHARED);
    do_test_rwlock_rd2_rec(attr);
    do_test_rwlock_wr2_rec(attr);

    TZERO(pthread_rwlockattr_destroy(attr));
}

/* This is more complex example to test contention of rwlockes.
 * Essentially, what happens is this:
 *
 * - main thread creates a rwlock and rdlocks it
 * - it then creates thread 1 and thread 2
 *
 * - it then record the current time, sleep for a specific 'waitDelay'
 *   then unlock the rwlock.
 *
 * - thread 1 tryrdlocks() the rwlock. It shall acquire the lock
 *   immediately, then release it, then wrlock().
 *
 * - thread 2 trywrlocks() the rwlock. In case of failure (EBUSY), it waits
 *   for a small amount of time (see 'spinDelay') and tries again, until
 *   it succeeds. It then unlocks the rwlock.
 *
 * The goal of this test is to verify that thread 1 has been stopped
 * for a sufficiently long time (in the wrlock), and that thread 2 has
 * been spinning for the same minimum period. There is no guarantee as
 * to which thread is going to acquire the rwlock first.
 */
typedef struct {
    pthread_rwlock_t  rwlock[1];
    double            t0;
    double            waitDelay;
    double            spinDelay;
} Test3State;

static void* do_rwlock_test_rd3_t1(void* arg)
{
    Test3State *s = arg;
    double      t1;

    /* try-acquire the lock, should succeed immediately */
    TZERO(pthread_rwlock_tryrdlock(s->rwlock));
    TZERO(pthread_rwlock_unlock(s->rwlock));

    /* wrlock() the lock, now */
    TZERO(pthread_rwlock_wrlock(s->rwlock));

    t1 = time_now();
    //DEBUG ONLY: printf("t1-s->t0=%g waitDelay=%g\n", t1-s->t0, s->waitDelay);
    TTRUE((t1-s->t0) >= s->waitDelay); 
    TZERO(pthread_rwlock_unlock(s->rwlock));
    return NULL;
}

static void* do_rwlock_test_rd3_t2(void* arg)
{
    Test3State *s = arg;
    double      t1;

    for (;;) {
        int ret = pthread_rwlock_trywrlock(s->rwlock);
        if (ret == 0)
            break;
        if (ret == EBUSY) {
            time_sleep(s->spinDelay);
            continue;
        }
    }
    t1 = time_now();
    TTRUE((t1-s->t0) >= s->waitDelay);
    TZERO(pthread_rwlock_unlock(s->rwlock));
    return NULL;
}


static void do_test_rwlock_rd3(pthread_rwlockattr_t *attr, double delay)
{
    Test3State  s[1];
    pthread_t   th1, th2;
    void*       dummy;

    TZERO(pthread_rwlock_init(s->rwlock, attr));
    s->waitDelay = delay;
    s->spinDelay = delay/20.;

    TZERO(pthread_rwlock_rdlock(s->rwlock));

    pthread_create(&th1, NULL, do_rwlock_test_rd3_t1, s);
    pthread_create(&th2, NULL, do_rwlock_test_rd3_t2, s);

    s->t0 = time_now();
    time_sleep(delay);

    TZERO(pthread_rwlock_unlock(s->rwlock));

    TZERO(pthread_join(th1, &dummy));
    TZERO(pthread_join(th2, &dummy));
}

static void do_test_3(double  delay)
{
    pthread_rwlockattr_t  attr[1];

    do_test_rwlock_rd3(NULL, delay);

    /* non-shared version */

    TZERO(pthread_rwlockattr_init(attr));

    set_rwlockattr_shared(attr, PTHREAD_PROCESS_PRIVATE);
    do_test_rwlock_rd3(attr, delay);

    set_rwlockattr_shared(attr, PTHREAD_PROCESS_SHARED);
    do_test_rwlock_rd3(attr, delay);

    TZERO(pthread_rwlockattr_destroy(attr));
}


int main(void)
{
    do_test_1();
    do_test_2();
    do_test_3(0.1);
    return 0;
}
