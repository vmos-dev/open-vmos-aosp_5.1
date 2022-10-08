/**************************************************************************
 *
 * Copyright 2006-2008 Tungsten Graphics, Inc., Cedar Park, TX., USA
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
 * Authors: Thomas Hellström <thomas-at-tungstengraphics-dot-com>
 */

#ifndef _WSBM_BUFPOOL_H_
#define _WSBM_BUFPOOL_H_

#include <errno.h>
#include "wsbm_util.h"
#include "wsbm_driver.h"
#include "wsbm_atomic.h"

struct _WsbmFenceObject;

struct _WsbmBufStorage
{
    struct _WsbmBufferPool *pool;
    struct _WsbmMutex mutex;
    struct _WsbmAtomic refCount;
    struct _WsbmAtomic onList;
    void *destroyArg;
    void (*destroyContainer) (void *);
};

struct _WsbmKernelBuf;

struct _WsbmBufferPool
{
    int fd;
    int (*map) (struct _WsbmBufStorage * buf, unsigned mode, void **virtual);
    void (*unmap) (struct _WsbmBufStorage * buf);
    int (*syncforcpu) (struct _WsbmBufStorage * buf, unsigned mode);
    void (*releasefromcpu) (struct _WsbmBufStorage * buf, unsigned mode);
    void (*destroy) (struct _WsbmBufStorage ** buf);
    unsigned long (*offset) (struct _WsbmBufStorage * buf);
    unsigned long (*poolOffset) (struct _WsbmBufStorage * buf);
        uint32_t(*placement) (struct _WsbmBufStorage * buf);
    unsigned long (*size) (struct _WsbmBufStorage * buf);
    struct _WsbmKernelBuf *(*kernel) (struct _WsbmBufStorage * buf);
    struct _WsbmBufStorage *(*create) (struct _WsbmBufferPool * pool,
				       unsigned long size,
				       uint32_t placement,
				       unsigned alignment);
    struct _WsbmBufStorage *(*createByReference) (struct _WsbmBufferPool *
						  pool, uint32_t handle);
    void (*fence) (struct _WsbmBufStorage * buf,
		   struct _WsbmFenceObject * fence);
    void (*unvalidate) (struct _WsbmBufStorage * buf);
    int (*validate) (struct _WsbmBufStorage * buf, uint64_t set_flags,
		     uint64_t clr_flags);
    int (*waitIdle) (struct _WsbmBufStorage * buf, int lazy);
    int (*setStatus) (struct _WsbmBufStorage * buf,
		      uint32_t set_placement, uint32_t clr_placement);
    void (*takeDown) (struct _WsbmBufferPool * pool);
};

static inline int
wsbmBufStorageInit(struct _WsbmBufStorage *storage,
		   struct _WsbmBufferPool *pool)
{
    int ret = WSBM_MUTEX_INIT(&storage->mutex);

    if (ret)
	return -ENOMEM;
    storage->pool = pool;
    wsbmAtomicSet(&storage->refCount, 1);
    wsbmAtomicSet(&storage->onList, 0);
    storage->destroyContainer = NULL;
    return 0;
}

static inline void
wsbmBufStorageTakedown(struct _WsbmBufStorage *storage)
{
    WSBM_MUTEX_FREE(&storage->mutex);
}

static inline void
wsbmBufStorageUnref(struct _WsbmBufStorage **pStorage)
{
    struct _WsbmBufStorage *storage = *pStorage;

    *pStorage = NULL;
    if (storage == NULL)
	return;

    if (wsbmAtomicDecZero(&storage->refCount)) {
	if (storage->destroyContainer)
	    storage->destroyContainer(storage->destroyArg);
	storage->pool->destroy(&storage);
	return;
    }
}

/*
 * Builtin pools.
 */

/*
 * Kernel buffer objects. Size in multiples of page size. Page size aligned.
 */

extern struct _WsbmBufferPool *wsbmTTMPoolInit(int fd,
					       unsigned int devOffset);
extern struct _WsbmBufferPool *wsbmMallocPoolInit(void);

struct _WsbmSlabCache;
extern struct _WsbmBufferPool *wsbmSlabPoolInit(int fd, uint32_t devOffset,
						uint32_t placement,
						uint32_t validMask,
						uint32_t smallestSize,
						uint32_t numSizes,
						uint32_t desiredNumBuffers,
						uint32_t maxSlabSize,
						uint32_t pageAlignment,
						struct _WsbmSlabCache *cache);
extern struct _WsbmSlabCache *wsbmSlabCacheInit(uint32_t checkIntervalMsec,
						uint32_t slabTimeoutMsec);
extern void wsbmSlabCacheFinish(struct _WsbmSlabCache *cache);

extern struct _WsbmBufferPool *wsbmUserPoolInit(void *vramAddr,
						unsigned long vramStart,
						unsigned long vramSize,
						void *agpAddr,
						unsigned long agpStart,
						unsigned long agpSize,
						uint32_t(*fenceTypes)
						    (uint64_t set_flags));

extern void wsbmUserPoolClean(struct _WsbmBufferPool *pool,
			      int cleanVram, int cleanAgp);

#endif
