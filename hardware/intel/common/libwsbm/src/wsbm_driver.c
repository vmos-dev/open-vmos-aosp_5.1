/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, TX., USA
 * All Rights Reserved.
 * Copyright 2009 VMware, Inc., Palo Alto, CA., USA
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/
/*
 * Authors:
 * Thomas Hellström <thomas-at-tungstengraphics-dot-com>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include "wsbm_driver.h"

struct _WsbmThreadFuncs *wsbmCurThreadFunc = NULL;
struct _WsbmVNodeFuncs *wsbmCurVNodeFunc = NULL;

/*
 * Single-threaded implementation.
 */

static int
n_mutexInit(struct _WsbmMutex *mutex, struct _WsbmThreadFuncs *func)
{
    mutex->func = func;
    return 0;
}

static int
n_condInit(struct _WsbmCond *cond, struct _WsbmThreadFuncs *func)
{
    cond->func = func;
    return 0;
}

static void
n_mutexNone(struct _WsbmMutex *mutex __attribute__ ((unused)))
{
    ;
}

static void
n_condNone(struct _WsbmCond *cond __attribute__ ((unused)))
{
    ;
}

static void
n_condWait(struct _WsbmCond *cond __attribute__ ((unused)), struct _WsbmMutex *mutex __attribute__ ((unused)))
{
    ;
}

static struct _WsbmThreadFuncs nullFunc = {
    .mutexInit = n_mutexInit,
    .mutexFree = n_mutexNone,
    .mutexLock = n_mutexNone,
    .mutexUnlock = n_mutexNone,
    .condInit = n_condInit,
    .condFree = n_condNone,
    .condWait = n_condWait,
    .condBroadcast = n_condNone
};

struct _WsbmThreadFuncs *
wsbmNullThreadFuncs(void)
{
    return &nullFunc;
}

#if (HAVE_PTHREADS == 1)
#include "pthread.h"

/*
 * pthreads implementation:
 */

struct _WsbmPMutex
{
    struct _WsbmThreadFuncs *func;
    pthread_mutex_t mutex;
};

struct _WsbmPCond
{
    struct _WsbmThreadFuncs *func;
    pthread_cond_t cond;
};

static inline struct _WsbmPMutex *
pMutexConvert(struct _WsbmMutex *m)
{
    union _PMutexConverter
    {
	struct _WsbmMutex wm;
	struct _WsbmPMutex pm;
    }  *um = containerOf(m, union _PMutexConverter, wm);

    return &um->pm;
}

static inline struct _WsbmPCond *
pCondConvert(struct _WsbmCond *c)
{
    union _PCondConverter
    {
	struct _WsbmCond wc;
	struct _WsbmPCond pc;
    }  *uc = containerOf(c, union _PCondConverter, wc);

    return &uc->pc;
}

static int
p_mutexInit(struct _WsbmMutex *mutex, struct _WsbmThreadFuncs *func)
{
    struct _WsbmPMutex *pMutex = pMutexConvert(mutex);

    if (sizeof(struct _WsbmMutex) < sizeof(struct _WsbmPMutex))
	return -EINVAL;

    pMutex->func = func;
    pthread_mutex_init(&pMutex->mutex, NULL);
    return 0;
}

static void
p_mutexFree(struct _WsbmMutex *mutex)
{
    struct _WsbmPMutex *pMutex = pMutexConvert(mutex);

    pthread_mutex_destroy(&pMutex->mutex);
}

static void
p_mutexLock(struct _WsbmMutex *mutex)
{
    struct _WsbmPMutex *pMutex = pMutexConvert(mutex);

    pthread_mutex_lock(&pMutex->mutex);
}

static void
p_mutexUnlock(struct _WsbmMutex *mutex)
{
    struct _WsbmPMutex *pMutex = pMutexConvert(mutex);

    pthread_mutex_unlock(&pMutex->mutex);
}

static int
p_condInit(struct _WsbmCond *cond, struct _WsbmThreadFuncs *func)
{
    struct _WsbmPCond *pCond = pCondConvert(cond);

    if (sizeof(struct _WsbmCond) < sizeof(struct _WsbmPCond))
	return -EINVAL;

    pCond->func = func;
    pthread_cond_init(&pCond->cond, NULL);
    return 0;
}

static void
p_condFree(struct _WsbmCond *cond)
{
    struct _WsbmPCond *pCond = pCondConvert(cond);

    pthread_cond_destroy(&pCond->cond);
}

static void
p_condBroadcast(struct _WsbmCond *cond)
{
    struct _WsbmPCond *pCond = pCondConvert(cond);

    pthread_cond_broadcast(&pCond->cond);
}

static void
p_condWait(struct _WsbmCond *cond, struct _WsbmMutex *mutex)
{
    struct _WsbmPCond *pCond = pCondConvert(cond);
    struct _WsbmPMutex *pMutex = pMutexConvert(mutex);

    pthread_cond_wait(&pCond->cond, &pMutex->mutex);
}

static struct _WsbmThreadFuncs pthreadFunc = {
    .mutexInit = p_mutexInit,
    .mutexFree = p_mutexFree,
    .mutexLock = p_mutexLock,
    .mutexUnlock = p_mutexUnlock,
    .condInit = p_condInit,
    .condFree = p_condFree,
    .condWait = p_condWait,
    .condBroadcast = p_condBroadcast
};

struct _WsbmThreadFuncs *
wsbmPThreadFuncs(void)
{
    return &pthreadFunc;
}

#else
#warning Pthreads is not present. Compiling without.

struct _WsbmThreadFuncs *
wsbmPThreadFuncs(void)
{
    return &pthreadFunc;
}

#endif
