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

#ifndef _WSBM_DRIVER_H
#define _WSBM_DRIVER_H
#include <stdint.h>
#include "wsbm_util.h"

#define WSBM_MUTEX_SPACE 16
#define WSBM_COND_SPACE  16

struct _WsbmMutex
{
    struct _WsbmThreadFuncs *func;
    unsigned long storage[WSBM_MUTEX_SPACE];
};

struct _WsbmCond
{
    struct _WsbmThreadFuncs *func;
    unsigned long storage[WSBM_COND_SPACE];
};

struct _WsbmThreadFuncs
{
    int (*mutexInit) (struct _WsbmMutex *, struct _WsbmThreadFuncs *);
    void (*mutexFree) (struct _WsbmMutex *);
    void (*mutexLock) (struct _WsbmMutex *);
    void (*mutexUnlock) (struct _WsbmMutex *);
    int (*condInit) (struct _WsbmCond *, struct _WsbmThreadFuncs *);
    void (*condFree) (struct _WsbmCond *);
    void (*condWait) (struct _WsbmCond *, struct _WsbmMutex *);
    void (*condBroadcast) (struct _WsbmCond *);
};

extern struct _WsbmThreadFuncs *wsbmCurThreadFunc;

#define WSBM_MUTEX_INIT(_mutex)			\
    wsbmThreadFuncs()->mutexInit(_mutex,wsbmThreadFuncs())
#define WSBM_MUTEX_FREE(_mutex)		\
    {						\
	(_mutex)->func->mutexFree(_mutex);	\
    }
#define WSBM_MUTEX_LOCK(_mutex) \
    (_mutex)->func->mutexLock(_mutex);
#define WSBM_MUTEX_UNLOCK(_mutex) \
    (_mutex)->func->mutexUnlock(_mutex);

#define WSBM_COND_INIT(_mutex)			\
    wsbmThreadFuncs()->condInit(_mutex, wsbmThreadFuncs())
#define WSBM_COND_FREE(_cond)			\
    {						\
	(_cond)->func->condFree(_cond);		\
    }
#define WSBM_COND_WAIT(_cond, _mutex)		\
    (_cond)->func->condWait(_cond, _mutex);
#define WSBM_COND_BROADCAST(_cond)		\
    (_cond)->func->condBroadcast(_cond);

struct _WsbmVNodeFuncs
{
    struct _ValidateNode *(*alloc) (struct _WsbmVNodeFuncs *, int);
    void (*free) (struct _ValidateNode *);
    void (*clear) (struct _ValidateNode *);
};

extern struct _WsbmVNodeFuncs *wsbmCurVNodeFunc;

struct _WsbmBufStorage;
struct _WsbmKernelBuf;

struct _ValidateNode
{
    uint32_t hash;
    int type_id;
    struct _WsbmListHead head;
    struct _WsbmListHead hashHead;
    int listItem;
    uint64_t set_flags;
    uint64_t clr_flags;
    void *buf;
    struct _WsbmVNodeFuncs *func;
};

static inline struct _WsbmVNodeFuncs *
wsbmVNodeFuncs(void)
{
    return wsbmCurVNodeFunc;
}

static inline struct _WsbmThreadFuncs *
wsbmThreadFuncs(void)
{
    return wsbmCurThreadFunc;
}

extern struct _WsbmThreadFuncs *wsbmNullThreadFuncs(void);

extern struct _WsbmThreadFuncs *wsbmPThreadFuncs(void);

#endif
