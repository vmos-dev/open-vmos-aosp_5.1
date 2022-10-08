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
 * Authors: Thomas Hellstrom <thomas-at-tungstengraphics-dot-com>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <drm/psb_ttm_placement_user.h>
#include <stdint.h>
#include <sys/time.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <sys/mman.h>
#include <xf86drm.h>
#include "wsbm_pool.h"
#include "wsbm_fencemgr.h"
#include "wsbm_priv.h"
#include "wsbm_manager.h"

#define WSBM_SLABPOOL_ALLOC_RETRIES 100
#define DRMRESTARTCOMMANDWRITE(_fd, _val, _arg, _ret)			\
	do {								\
		(_ret) = drmCommandWrite(_fd, _val, &(_arg), sizeof(_arg)); \
	} while ((_ret) == -EAGAIN || (_ret) == -ERESTART);		\

#define DRMRESTARTCOMMANDWRITEREAD(_fd, _val, _arg, _ret)		\
	do {								\
		(_ret) = drmCommandWriteRead(_fd, _val, &(_arg), sizeof(_arg)); \
	} while ((_ret) == -EAGAIN || (_ret) == -ERESTART);		\


#ifdef DEBUG_FENCESIGNALED
static int createbuffer = 0;
static int fencesignaled = 0;
#endif

struct _WsbmSlab;

struct _WsbmSlabBuffer
{
    struct _WsbmKernelBuf kBuf;
    struct _WsbmBufStorage storage;
    struct _WsbmCond event;

    /*
     * Remains constant after creation.
     */

    int isSlabBuffer;
    struct _WsbmSlab *parent;
    uint32_t start;
    void *virtual;
    unsigned long requestedSize;
    uint64_t mapHandle;

    /*
     * Protected by struct _WsbmSlabSizeHeader::mutex
     */

    struct _WsbmListHead head;

    /*
     * Protected by this::mutex
     */

    struct _WsbmFenceObject *fence;
    uint32_t fenceType;
    struct _WsbmAtomic writers;	       /* (Only upping) */
    int unFenced;
};

struct _WsbmSlabPool;
struct _WsbmSlabKernelBO
{

    /*
     * Constant at creation
     */

    struct _WsbmKernelBuf kBuf;
    uint32_t pageAlignment;
    void *virtual;
    unsigned long actualSize;
    uint64_t mapHandle;

    /*
     * Protected by struct _WsbmSlabCache::mutex
     */

    struct _WsbmSlabPool *slabPool;
    uint32_t proposedPlacement;
    struct _WsbmListHead timeoutHead;
    struct _WsbmListHead head;
    struct timeval timeFreed;
};

struct _WsbmSlab
{
    struct _WsbmListHead head;
    struct _WsbmListHead freeBuffers;
    uint32_t numBuffers;
    uint32_t numFree;
    struct _WsbmSlabBuffer *buffers;
    struct _WsbmSlabSizeHeader *header;
    struct _WsbmSlabKernelBO *kbo;
};

struct _WsbmSlabSizeHeader
{
    /*
     * Constant at creation.
     */
    struct _WsbmSlabPool *slabPool;
    uint32_t bufSize;

    /*
     * Protected by this::mutex
     */

    struct _WsbmListHead slabs;
    struct _WsbmListHead freeSlabs;
    struct _WsbmListHead delayedBuffers;
    uint32_t numDelayed;
    struct _WsbmMutex mutex;
};

struct _WsbmSlabCache
{
    struct timeval slabTimeout;
    struct timeval checkInterval;
    struct timeval nextCheck;
    struct _WsbmListHead timeoutList;
    struct _WsbmListHead unCached;
    struct _WsbmListHead cached;
    struct _WsbmMutex mutex;
};

struct _WsbmSlabPool
{
    struct _WsbmBufferPool pool;

    /*
     * The data of this structure remains constant after
     * initialization and thus needs no mutex protection.
     */

    unsigned int devOffset;
    struct _WsbmSlabCache *cache;
    uint32_t proposedPlacement;
    uint32_t validMask;
    uint32_t *bucketSizes;
    uint32_t numBuckets;
    uint32_t pageSize;
    int pageAlignment;
    int maxSlabSize;
    int desiredNumBuffers;
    struct _WsbmSlabSizeHeader *headers;
};

static inline struct _WsbmSlabPool *
slabPoolFromPool(struct _WsbmBufferPool *pool)
{
    return containerOf(pool, struct _WsbmSlabPool, pool);
}

static inline struct _WsbmSlabPool *
slabPoolFromBuf(struct _WsbmSlabBuffer *sBuf)
{
    return slabPoolFromPool(sBuf->storage.pool);
}

static inline struct _WsbmSlabBuffer *
slabBuffer(struct _WsbmBufStorage *buf)
{
    return containerOf(buf, struct _WsbmSlabBuffer, storage);
}

/*
 * FIXME: Perhaps arrange timeout slabs in size buckets for fast
 * retreival??
 */

static inline int
wsbmTimeAfterEq(struct timeval *arg1, struct timeval *arg2)
{
    return ((arg1->tv_sec > arg2->tv_sec) ||
	    ((arg1->tv_sec == arg2->tv_sec) &&
	     (arg1->tv_usec > arg2->tv_usec)));
}

static inline void
wsbmTimeAdd(struct timeval *arg, struct timeval *add)
{
    unsigned int sec;

    arg->tv_sec += add->tv_sec;
    arg->tv_usec += add->tv_usec;
    sec = arg->tv_usec / 1000000;
    arg->tv_sec += sec;
    arg->tv_usec -= sec * 1000000;
}

static void
wsbmFreeKernelBO(struct _WsbmSlabKernelBO *kbo)
{
    struct ttm_pl_reference_req arg;
    struct _WsbmSlabPool *slabPool;

    if (!kbo)
	return;

    slabPool = kbo->slabPool;
    arg.handle = kbo->kBuf.handle;
    (void)munmap(kbo->virtual, kbo->actualSize);
    (void)drmCommandWrite(slabPool->pool.fd,
			  slabPool->devOffset + TTM_PL_UNREF, &arg,
			  sizeof(arg));
    free(kbo);
}

static void
wsbmFreeTimeoutKBOsLocked(struct _WsbmSlabCache *cache, struct timeval *time)
{
    struct _WsbmListHead *list, *next;
    struct _WsbmSlabKernelBO *kbo;

    if (!wsbmTimeAfterEq(time, &cache->nextCheck))
	return;

    WSBMLISTFOREACHSAFE(list, next, &cache->timeoutList) {
	kbo = WSBMLISTENTRY(list, struct _WsbmSlabKernelBO, timeoutHead);

	if (!wsbmTimeAfterEq(time, &kbo->timeFreed))
	    break;

	WSBMLISTDELINIT(&kbo->timeoutHead);
	WSBMLISTDELINIT(&kbo->head);
	wsbmFreeKernelBO(kbo);
    }

    cache->nextCheck = *time;
    wsbmTimeAdd(&cache->nextCheck, &cache->checkInterval);
}

/*
 * Add a _SlabKernelBO to the free slab manager.
 * This means that it is available for reuse, but if it's not
 * reused in a while, it will be freed.
 */

static void
wsbmSetKernelBOFree(struct _WsbmSlabCache *cache,
		    struct _WsbmSlabKernelBO *kbo)
{
    struct timeval time;
    struct timeval timeFreed;

    gettimeofday(&time, NULL);
    timeFreed = time;
    WSBM_MUTEX_LOCK(&cache->mutex);
    wsbmTimeAdd(&timeFreed, &cache->slabTimeout);
    kbo->timeFreed = timeFreed;

    if (kbo->kBuf.placement & TTM_PL_FLAG_CACHED)
	WSBMLISTADD(&kbo->head, &cache->cached);
    else
	WSBMLISTADD(&kbo->head, &cache->unCached);

    WSBMLISTADDTAIL(&kbo->timeoutHead, &cache->timeoutList);
    wsbmFreeTimeoutKBOsLocked(cache, &time);

    WSBM_MUTEX_UNLOCK(&cache->mutex);
}

/*
 * Get a _SlabKernelBO for us to use as storage for a slab.
 */

static struct _WsbmSlabKernelBO *
wsbmAllocKernelBO(struct _WsbmSlabSizeHeader *header)
{
    struct _WsbmSlabPool *slabPool = header->slabPool;
    struct _WsbmSlabCache *cache = slabPool->cache;
    struct _WsbmListHead *list, *head;
    uint32_t size = header->bufSize * slabPool->desiredNumBuffers;
    struct _WsbmSlabKernelBO *kbo;
    struct _WsbmSlabKernelBO *kboTmp;
    int ret;

    /*
     * FIXME: We should perhaps allow some variation in slabsize in order
     * to efficiently reuse slabs.
     */

    size = (size <= (uint32_t) slabPool->maxSlabSize) ? size : (uint32_t) slabPool->maxSlabSize;
    if (size < header->bufSize)
	size = header->bufSize;
    size = (size + slabPool->pageSize - 1) & ~(slabPool->pageSize - 1);
    WSBM_MUTEX_LOCK(&cache->mutex);

    kbo = NULL;

  retry:
    head = (slabPool->proposedPlacement & TTM_PL_FLAG_CACHED) ?
	&cache->cached : &cache->unCached;

    WSBMLISTFOREACH(list, head) {
	kboTmp = WSBMLISTENTRY(list, struct _WsbmSlabKernelBO, head);

	if ((kboTmp->actualSize == size) &&
	    (slabPool->pageAlignment == 0 ||
	     (kboTmp->pageAlignment % slabPool->pageAlignment) == 0)) {

	    if (!kbo)
		kbo = kboTmp;

	    if ((kbo->proposedPlacement ^ slabPool->proposedPlacement) == 0)
		break;

	}
    }

    if (kbo) {
	WSBMLISTDELINIT(&kbo->head);
	WSBMLISTDELINIT(&kbo->timeoutHead);
    }

    WSBM_MUTEX_UNLOCK(&cache->mutex);

    if (kbo) {
	uint32_t new_mask =
	    kbo->proposedPlacement ^ slabPool->proposedPlacement;

	ret = 0;
	if (new_mask) {
	    union ttm_pl_setstatus_arg arg;
	    struct ttm_pl_setstatus_req *req = &arg.req;
	    struct ttm_pl_rep *rep = &arg.rep;

	    req->handle = kbo->kBuf.handle;
	    req->set_placement = slabPool->proposedPlacement & new_mask;
	    req->clr_placement = ~slabPool->proposedPlacement & new_mask;
	    DRMRESTARTCOMMANDWRITEREAD(slabPool->pool.fd,
				       slabPool->devOffset + TTM_PL_SETSTATUS,
				       arg, ret);
	    if (ret == 0) {
		kbo->kBuf.gpuOffset = rep->gpu_offset;
		kbo->kBuf.placement = rep->placement;
	    }
	    kbo->proposedPlacement = slabPool->proposedPlacement;
	}

	if (ret == 0)
	    return kbo;

	wsbmFreeKernelBO(kbo);
	kbo = NULL;
	goto retry;
    }

    kbo = calloc(1, sizeof(*kbo));
    if (!kbo)
	return NULL;

    {
	union ttm_pl_create_arg arg;

	kbo->slabPool = slabPool;
	WSBMINITLISTHEAD(&kbo->head);
	WSBMINITLISTHEAD(&kbo->timeoutHead);

	arg.req.size = size;
	arg.req.placement = slabPool->proposedPlacement;
	arg.req.page_alignment = slabPool->pageAlignment;

	DRMRESTARTCOMMANDWRITEREAD(slabPool->pool.fd,
				   slabPool->devOffset + TTM_PL_CREATE,
				   arg, ret);
	if (ret)
	    goto out_err0;

	kbo->kBuf.gpuOffset = arg.rep.gpu_offset;
	kbo->kBuf.placement = arg.rep.placement;
	kbo->kBuf.handle = arg.rep.handle;

	kbo->actualSize = arg.rep.bo_size;
	kbo->mapHandle = arg.rep.map_handle;
	kbo->proposedPlacement = slabPool->proposedPlacement;
    }

    kbo->virtual = mmap(0, kbo->actualSize,
			PROT_READ | PROT_WRITE, MAP_SHARED,
			slabPool->pool.fd, kbo->mapHandle);

    if (kbo->virtual == MAP_FAILED) {
	ret = -errno;
	goto out_err1;
    }

    return kbo;

  out_err1:
    {
	struct ttm_pl_reference_req arg = {.handle = kbo->kBuf.handle };

	(void)drmCommandWrite(slabPool->pool.fd,
			      slabPool->devOffset + TTM_PL_UNREF,
			      &arg, sizeof(arg));
    }
  out_err0:
    free(kbo);
    return NULL;
}

static int
wsbmAllocSlab(struct _WsbmSlabSizeHeader *header)
{
    struct _WsbmSlab *slab;
    struct _WsbmSlabBuffer *sBuf;
    uint32_t numBuffers;
    uint32_t ret;
    uint32_t i;

    slab = calloc(1, sizeof(*slab));
    if (!slab)
	return -ENOMEM;

    slab->kbo = wsbmAllocKernelBO(header);
    if (!slab->kbo) {
	ret = -ENOMEM;
	goto out_err0;
    }

    numBuffers = slab->kbo->actualSize / header->bufSize;

    slab->buffers = calloc(numBuffers, sizeof(*slab->buffers));
    if (!slab->buffers) {
	ret = -ENOMEM;
	goto out_err1;
    }

    WSBMINITLISTHEAD(&slab->head);
    WSBMINITLISTHEAD(&slab->freeBuffers);
    slab->numBuffers = numBuffers;
    slab->numFree = 0;
    slab->header = header;

    sBuf = slab->buffers;
    for (i = 0; i < numBuffers; ++i) {
	ret = wsbmBufStorageInit(&sBuf->storage, &header->slabPool->pool);
	if (ret)
	    goto out_err2;
	sBuf->parent = slab;
	sBuf->start = i * header->bufSize;
	sBuf->virtual = (void *)((uint8_t *) slab->kbo->virtual +
				 sBuf->start);
	wsbmAtomicSet(&sBuf->writers, 0);
	sBuf->isSlabBuffer = 1;
	WSBM_COND_INIT(&sBuf->event);
	WSBMLISTADDTAIL(&sBuf->head, &slab->freeBuffers);
	slab->numFree++;
	sBuf++;
    }

    WSBMLISTADDTAIL(&slab->head, &header->slabs);

    return 0;

  out_err2:
    sBuf = slab->buffers;
    for (i = 0; i < numBuffers; ++i) {
	if (sBuf->parent == slab) {
	    WSBM_COND_FREE(&sBuf->event);
	    wsbmBufStorageTakedown(&sBuf->storage);
	}
	sBuf++;
    }
    free(slab->buffers);
  out_err1:
    wsbmSetKernelBOFree(header->slabPool->cache, slab->kbo);
  out_err0:
    free(slab);
    return ret;
}

/*
 * Delete a buffer from the slab header delayed list and put
 * it on the slab free list.
 */

static void
wsbmSlabFreeBufferLocked(struct _WsbmSlabBuffer *buf)
{
    struct _WsbmSlab *slab = buf->parent;
    struct _WsbmSlabSizeHeader *header = slab->header;
    struct _WsbmListHead *list = &buf->head;

    WSBMLISTDEL(list);
    WSBMLISTADDTAIL(list, &slab->freeBuffers);
    slab->numFree++;

    if (slab->head.next == &slab->head)
	WSBMLISTADDTAIL(&slab->head, &header->slabs);

    if (slab->numFree == slab->numBuffers) {
	list = &slab->head;
	WSBMLISTDEL(list);
	WSBMLISTADDTAIL(list, &header->freeSlabs);
    }

    if (header->slabs.next == &header->slabs ||
	slab->numFree != slab->numBuffers) {

	struct _WsbmListHead *next;
	struct _WsbmSlabCache *cache = header->slabPool->cache;

	WSBMLISTFOREACHSAFE(list, next, &header->freeSlabs) {
	    uint32_t i;
	    struct _WsbmSlabBuffer *sBuf;

	    slab = WSBMLISTENTRY(list, struct _WsbmSlab, head);

	    WSBMLISTDELINIT(list);

	    sBuf = slab->buffers;
	    for (i = 0; i < slab->numBuffers; ++i) {
		if (sBuf->parent == slab) {
		    WSBM_COND_FREE(&sBuf->event);
		    wsbmBufStorageTakedown(&sBuf->storage);
		}
		sBuf++;
	    }
	    wsbmSetKernelBOFree(cache, slab->kbo);
	    free(slab->buffers);
	    free(slab);
	}
    }
}

static void
wsbmSlabCheckFreeLocked(struct _WsbmSlabSizeHeader *header, int wait)
{
  struct _WsbmListHead *list, *prev, *first, *head;
    struct _WsbmSlabBuffer *sBuf;
    struct _WsbmSlab *slab;
    int firstWasSignaled = 1;
    int signaled;
    uint32_t i;
    int ret;

    /*
     * Rerun the freeing test if the youngest tested buffer
     * was signaled, since there might be more idle buffers
     * in the delay list.
     */

    while (firstWasSignaled) {
	firstWasSignaled = 0;
	signaled = 0;
	first = header->delayedBuffers.next;

	/* Only examine the oldest 1/3 of delayed buffers:
	 */
	if (header->numDelayed > 3) {
	    for (i = 0; i < header->numDelayed; i += 3) {
		first = first->next;
	    }
	}

	/*
	 * No need to take the buffer mutex for each buffer we loop
	 * through since we're currently the only user.
	 */

	head = first->next;
	WSBMLISTFOREACHPREVSAFE(list, prev, head) {

	    if (list == &header->delayedBuffers)
		break;

	    sBuf = WSBMLISTENTRY(list, struct _WsbmSlabBuffer, head);

	    slab = sBuf->parent;

	    if (!signaled) {
		if (wait) {
		    ret = wsbmFenceFinish(sBuf->fence, sBuf->fenceType, 0);
		    if (ret)
			break;
		    signaled = 1;
		    wait = 0;
		} else {
		    signaled =
			wsbmFenceSignaled(sBuf->fence, sBuf->fenceType);
#ifdef DEBUG_FENCESIGNALED
		    fencesignaled++;
#endif
		}
		if (signaled) {
		    if (list == first)
			firstWasSignaled = 1;
		    wsbmFenceUnreference(&sBuf->fence);
		    header->numDelayed--;
		    wsbmSlabFreeBufferLocked(sBuf);
		} else
		    break;
	    } else if (wsbmFenceSignaledCached(sBuf->fence, sBuf->fenceType)) {
		wsbmFenceUnreference(&sBuf->fence);
		header->numDelayed--;
		wsbmSlabFreeBufferLocked(sBuf);
	    }
	}
    }
}

static struct _WsbmSlabBuffer *
wsbmSlabAllocBuffer(struct _WsbmSlabSizeHeader *header)
{
    static struct _WsbmSlabBuffer *buf;
    struct _WsbmSlab *slab;
    struct _WsbmListHead *list;
    int count = WSBM_SLABPOOL_ALLOC_RETRIES;

    WSBM_MUTEX_LOCK(&header->mutex);
    while (header->slabs.next == &header->slabs && count > 0) {
	wsbmSlabCheckFreeLocked(header, 0);
	if (header->slabs.next != &header->slabs)
	    break;

	WSBM_MUTEX_UNLOCK(&header->mutex);
	if (count != WSBM_SLABPOOL_ALLOC_RETRIES)
	    usleep(1000);
	WSBM_MUTEX_LOCK(&header->mutex);
	(void)wsbmAllocSlab(header);
	count--;
    }

    list = header->slabs.next;
    if (list == &header->slabs) {
	WSBM_MUTEX_UNLOCK(&header->mutex);
	return NULL;
    }
    slab = WSBMLISTENTRY(list, struct _WsbmSlab, head);
    if (--slab->numFree == 0)
	WSBMLISTDELINIT(list);

    list = slab->freeBuffers.next;
    WSBMLISTDELINIT(list);

    WSBM_MUTEX_UNLOCK(&header->mutex);
    buf = WSBMLISTENTRY(list, struct _WsbmSlabBuffer, head);

    buf->storage.destroyContainer = NULL;

#ifdef DEBUG_FENCESIGNALED
    createbuffer++;
#endif
    return buf;
}

static struct _WsbmBufStorage *
pool_create(struct _WsbmBufferPool *pool, unsigned long size,
	    uint32_t placement, unsigned alignment)
{
    struct _WsbmSlabPool *slabPool = slabPoolFromPool(pool);
    struct _WsbmSlabSizeHeader *header;
    struct _WsbmSlabBuffer *sBuf;
    uint32_t i;
    int ret;

    /*
     * FIXME: Check for compatibility.
     */

    header = slabPool->headers;
    for (i = 0; i < slabPool->numBuckets; ++i) {
	if (header->bufSize >= size)
	    break;
	header++;
    }

    if (i < slabPool->numBuckets) {
	sBuf = wsbmSlabAllocBuffer(header);
	return ((sBuf) ? &sBuf->storage : NULL);
    }

    /*
     * Fall back to allocate a buffer object directly from DRM.
     * and wrap it in a wsbmBO structure.
     */

    sBuf = calloc(1, sizeof(*sBuf));

    if (!sBuf)
	return NULL;

    if (alignment) {
	if ((alignment < slabPool->pageSize)
	    && (slabPool->pageSize % alignment))
	    goto out_err0;
	if ((alignment > slabPool->pageSize)
	    && (alignment % slabPool->pageSize))
	    goto out_err0;
    }

    ret = wsbmBufStorageInit(&sBuf->storage, pool);
    if (ret)
	goto out_err0;

    ret = WSBM_COND_INIT(&sBuf->event);
    if (ret)
	goto out_err1;

    {
	union ttm_pl_create_arg arg;

	arg.req.size = size;
	arg.req.placement = placement;
	arg.req.page_alignment = alignment / slabPool->pageSize;

	DRMRESTARTCOMMANDWRITEREAD(pool->fd,
				   slabPool->devOffset + TTM_PL_CREATE,
				   arg, ret);

	if (ret)
	    goto out_err2;

	sBuf->kBuf.gpuOffset = arg.rep.gpu_offset;
	sBuf->kBuf.placement = arg.rep.placement;
	sBuf->kBuf.handle = arg.rep.handle;
	sBuf->mapHandle = arg.rep.map_handle;
	sBuf->requestedSize = size;

	sBuf->virtual = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED,
			     pool->fd, sBuf->mapHandle);

	if (sBuf->virtual == MAP_FAILED)
	    goto out_err3;
    }

    wsbmAtomicSet(&sBuf->writers, 0);
    return &sBuf->storage;
  out_err3:
    {
	struct ttm_pl_reference_req arg;

	arg.handle = sBuf->kBuf.handle;
	(void)drmCommandWriteRead(pool->fd,
				  slabPool->devOffset + TTM_PL_UNREF,
				  &arg, sizeof(arg));
    }
  out_err2:
    WSBM_COND_FREE(&sBuf->event);
  out_err1:
    wsbmBufStorageTakedown(&sBuf->storage);
  out_err0:
    free(sBuf);
    return NULL;
}

static void
pool_destroy(struct _WsbmBufStorage **p_buf)
{
    struct _WsbmBufStorage *buf = *p_buf;
    struct _WsbmSlabBuffer *sBuf = slabBuffer(buf);
    struct _WsbmSlab *slab;
    struct _WsbmSlabSizeHeader *header;

    *p_buf = NULL;

    if (!sBuf->isSlabBuffer) {
	struct _WsbmSlabPool *slabPool = slabPoolFromBuf(sBuf);
	struct ttm_pl_reference_req arg;

	if (sBuf->virtual != NULL) {
	    (void)munmap(sBuf->virtual, sBuf->requestedSize);
	    sBuf->virtual = NULL;
	}

	arg.handle = sBuf->kBuf.handle;
	(void)drmCommandWrite(slabPool->pool.fd,
			      slabPool->devOffset + TTM_PL_UNREF,
			      &arg, sizeof(arg));

	WSBM_COND_FREE(&sBuf->event);
	wsbmBufStorageTakedown(&sBuf->storage);
	free(sBuf);
	return;
    }

    slab = sBuf->parent;
    header = slab->header;

    /*
     * No need to take the buffer mutex below since we're the only user.
     */

    WSBM_MUTEX_LOCK(&header->mutex);
    sBuf->unFenced = 0;
    wsbmAtomicSet(&sBuf->writers, 0);
    wsbmAtomicSet(&sBuf->storage.refCount, 1);

    if (sBuf->fence && !wsbmFenceSignaledCached(sBuf->fence, sBuf->fenceType)) {
	WSBMLISTADDTAIL(&sBuf->head, &header->delayedBuffers);
	header->numDelayed++;
    } else {
	if (sBuf->fence)
	    wsbmFenceUnreference(&sBuf->fence);
	wsbmSlabFreeBufferLocked(sBuf);
    }

    WSBM_MUTEX_UNLOCK(&header->mutex);
}

static void
waitIdleLocked(struct _WsbmSlabBuffer *sBuf, int lazy)
{
    struct _WsbmBufStorage *storage = &sBuf->storage;

    while (sBuf->unFenced || sBuf->fence != NULL) {

	if (sBuf->unFenced)
	    WSBM_COND_WAIT(&sBuf->event, &storage->mutex);

	if (sBuf->fence != NULL) {
	    if (!wsbmFenceSignaled(sBuf->fence, sBuf->fenceType)) {
		struct _WsbmFenceObject *fence =
		    wsbmFenceReference(sBuf->fence);

		WSBM_MUTEX_UNLOCK(&storage->mutex);
		(void)wsbmFenceFinish(fence, sBuf->fenceType, lazy);
		WSBM_MUTEX_LOCK(&storage->mutex);
		if (sBuf->fence == fence)
		    wsbmFenceUnreference(&sBuf->fence);

		wsbmFenceUnreference(&fence);
	    } else {
		wsbmFenceUnreference(&sBuf->fence);
	    }
	}
    }
}

static int
pool_waitIdle(struct _WsbmBufStorage *buf, int lazy)
{
    struct _WsbmSlabBuffer *sBuf = slabBuffer(buf);

    WSBM_MUTEX_LOCK(&buf->mutex);
    waitIdleLocked(sBuf, lazy);
    WSBM_MUTEX_UNLOCK(&buf->mutex);

    return 0;
}

static int
pool_map(struct _WsbmBufStorage *buf, unsigned mode __attribute__ ((unused)), void **virtual)
{
    struct _WsbmSlabBuffer *sBuf = slabBuffer(buf);

    *virtual = sBuf->virtual;

    return 0;
}

static void
pool_releaseFromCpu(struct _WsbmBufStorage *buf, unsigned mode __attribute__ ((unused)))
{
    struct _WsbmSlabBuffer *sBuf = slabBuffer(buf);

    if (wsbmAtomicDecZero(&sBuf->writers))
	WSBM_COND_BROADCAST(&sBuf->event);
}

static int
pool_syncForCpu(struct _WsbmBufStorage *buf, unsigned mode)
{
    struct _WsbmSlabBuffer *sBuf = slabBuffer(buf);
    int ret = 0;

    WSBM_MUTEX_LOCK(&buf->mutex);
    if ((mode & WSBM_SYNCCPU_DONT_BLOCK)) {
	int signaled;

	if (sBuf->unFenced) {
	    ret = -EBUSY;
	    goto out_unlock;
	}

	if (sBuf->isSlabBuffer)
	    signaled = (sBuf->fence == NULL) ||
		wsbmFenceSignaledCached(sBuf->fence, sBuf->fenceType);
	else
	    signaled = (sBuf->fence == NULL) ||
		wsbmFenceSignaled(sBuf->fence, sBuf->fenceType);

	ret = 0;
	if (signaled) {
	    wsbmFenceUnreference(&sBuf->fence);
	    wsbmAtomicInc(&sBuf->writers);
	} else
	    ret = -EBUSY;
	goto out_unlock;
    }
    waitIdleLocked(sBuf, 0);
    wsbmAtomicInc(&sBuf->writers);
  out_unlock:
    WSBM_MUTEX_UNLOCK(&buf->mutex);
    return ret;
}

static void
pool_unmap(struct _WsbmBufStorage *buf __attribute__ ((unused)))
{
    ;
}

static unsigned long
pool_poolOffset(struct _WsbmBufStorage *buf)
{
    struct _WsbmSlabBuffer *sBuf = slabBuffer(buf);

    return sBuf->start;
}

static unsigned long
pool_size(struct _WsbmBufStorage *buf)
{
    struct _WsbmSlabBuffer *sBuf = slabBuffer(buf);

    if (!sBuf->isSlabBuffer)
	return sBuf->requestedSize;

    return sBuf->parent->header->bufSize;
}

static struct _WsbmKernelBuf *
pool_kernel(struct _WsbmBufStorage *buf)
{
    struct _WsbmSlabBuffer *sBuf = slabBuffer(buf);

    return (sBuf->isSlabBuffer) ? &sBuf->parent->kbo->kBuf : &sBuf->kBuf;
}

static unsigned long
pool_offset(struct _WsbmBufStorage *buf)
{
    return pool_kernel(buf)->gpuOffset + pool_poolOffset(buf);
}

static void
pool_fence(struct _WsbmBufStorage *buf, struct _WsbmFenceObject *fence)
{
    struct _WsbmSlabBuffer *sBuf = slabBuffer(buf);
    struct _WsbmKernelBuf *kBuf;

    WSBM_MUTEX_LOCK(&buf->mutex);
    if (sBuf->fence)
	wsbmFenceUnreference(&sBuf->fence);

    kBuf = pool_kernel(buf);
    sBuf->fenceType = kBuf->fence_type_mask;
    if (!wsbmFenceSignaledCached(fence, sBuf->fenceType))
	sBuf->fence = wsbmFenceReference(fence);

    sBuf->unFenced = 0;
    WSBM_COND_BROADCAST(&sBuf->event);
    WSBM_MUTEX_UNLOCK(&buf->mutex);
}

static int
pool_validate(struct _WsbmBufStorage *buf,
        uint64_t set_flags __attribute__ ((unused)), uint64_t clr_flags __attribute__ ((unused)))
{
    struct _WsbmSlabBuffer *sBuf = slabBuffer(buf);

    WSBM_MUTEX_LOCK(&buf->mutex);
    while (wsbmAtomicRead(&sBuf->writers) != 0) {
	WSBM_COND_WAIT(&sBuf->event, &buf->mutex);
    }

    sBuf->unFenced = 1;
    WSBM_MUTEX_UNLOCK(&buf->mutex);
    return 0;
}

static void
pool_unvalidate(struct _WsbmBufStorage *buf)
{
    struct _WsbmSlabBuffer *sBuf = slabBuffer(buf);

    WSBM_MUTEX_LOCK(&buf->mutex);
    if (sBuf->unFenced) {
	sBuf->unFenced = 0;
	WSBM_COND_BROADCAST(&sBuf->event);
    }
    WSBM_MUTEX_UNLOCK(&buf->mutex);
}

struct _WsbmSlabCache *
wsbmSlabCacheInit(uint32_t checkIntervalMsec, uint32_t slabTimeoutMsec)
{
    struct _WsbmSlabCache *tmp;

    tmp = calloc(1, sizeof(*tmp));
    if (!tmp)
	return NULL;

    WSBM_MUTEX_INIT(&tmp->mutex);
    WSBM_MUTEX_LOCK(&tmp->mutex);
    tmp->slabTimeout.tv_usec = slabTimeoutMsec * 1000;
    tmp->slabTimeout.tv_sec = tmp->slabTimeout.tv_usec / 1000000;
    tmp->slabTimeout.tv_usec -= tmp->slabTimeout.tv_sec * 1000000;

    tmp->checkInterval.tv_usec = checkIntervalMsec * 1000;
    tmp->checkInterval.tv_sec = tmp->checkInterval.tv_usec / 1000000;
    tmp->checkInterval.tv_usec -= tmp->checkInterval.tv_sec * 1000000;

    gettimeofday(&tmp->nextCheck, NULL);
    wsbmTimeAdd(&tmp->nextCheck, &tmp->checkInterval);
    WSBMINITLISTHEAD(&tmp->timeoutList);
    WSBMINITLISTHEAD(&tmp->unCached);
    WSBMINITLISTHEAD(&tmp->cached);
    WSBM_MUTEX_UNLOCK(&tmp->mutex);

    return tmp;
}

void
wsbmSlabCacheFinish(struct _WsbmSlabCache *cache)
{
    struct timeval time;

    time = cache->nextCheck;
    WSBM_MUTEX_LOCK(&cache->mutex);
    wsbmTimeAdd(&time, &cache->checkInterval);
    wsbmFreeTimeoutKBOsLocked(cache, &time);
    WSBM_MUTEX_UNLOCK(&cache->mutex);

    assert(cache->timeoutList.next == &cache->timeoutList);
    assert(cache->unCached.next == &cache->unCached);
    assert(cache->cached.next == &cache->cached);

    WSBM_MUTEX_FREE(&cache->mutex);
    free(cache);
}

static void
wsbmInitSizeHeader(struct _WsbmSlabPool *slabPool, uint32_t size,
		   struct _WsbmSlabSizeHeader *header)
{
    WSBM_MUTEX_INIT(&header->mutex);
    WSBM_MUTEX_LOCK(&header->mutex);

    WSBMINITLISTHEAD(&header->slabs);
    WSBMINITLISTHEAD(&header->freeSlabs);
    WSBMINITLISTHEAD(&header->delayedBuffers);

    header->numDelayed = 0;
    header->slabPool = slabPool;
    header->bufSize = size;

    WSBM_MUTEX_UNLOCK(&header->mutex);
}

static void
wsbmFinishSizeHeader(struct _WsbmSlabSizeHeader *header)
{
    struct _WsbmListHead *list, *next;
    struct _WsbmSlabBuffer *sBuf;

    WSBM_MUTEX_LOCK(&header->mutex);
    WSBMLISTFOREACHSAFE(list, next, &header->delayedBuffers) {
	sBuf = WSBMLISTENTRY(list, struct _WsbmSlabBuffer, head);

	if (sBuf->fence) {
	    (void)wsbmFenceFinish(sBuf->fence, sBuf->fenceType, 0);
	    wsbmFenceUnreference(&sBuf->fence);
	}
	header->numDelayed--;
	wsbmSlabFreeBufferLocked(sBuf);
    }
    WSBM_MUTEX_UNLOCK(&header->mutex);
    WSBM_MUTEX_FREE(&header->mutex);
}

static void
pool_takedown(struct _WsbmBufferPool *pool)
{
    struct _WsbmSlabPool *slabPool = slabPoolFromPool(pool);
    unsigned int i;

    for (i = 0; i < slabPool->numBuckets; ++i) {
	wsbmFinishSizeHeader(&slabPool->headers[i]);
    }

    free(slabPool->headers);
    free(slabPool->bucketSizes);
    free(slabPool);
}

struct _WsbmBufferPool *
wsbmSlabPoolInit(int fd,
		 uint32_t devOffset,
		 uint32_t placement,
		 uint32_t validMask,
		 uint32_t smallestSize,
		 uint32_t numSizes,
		 uint32_t desiredNumBuffers,
		 uint32_t maxSlabSize,
		 uint32_t pageAlignment, struct _WsbmSlabCache *cache)
{
    struct _WsbmBufferPool *pool;
    struct _WsbmSlabPool *slabPool;
    uint32_t i;

    slabPool = calloc(1, sizeof(*slabPool));
    if (!slabPool)
	return NULL;

    pool = &slabPool->pool;

    slabPool->bucketSizes = calloc(numSizes, sizeof(*slabPool->bucketSizes));
    if (!slabPool->bucketSizes)
	goto out_err0;

    slabPool->headers = calloc(numSizes, sizeof(*slabPool->headers));
    if (!slabPool->headers)
	goto out_err1;

    slabPool->devOffset = devOffset;
    slabPool->cache = cache;
    slabPool->proposedPlacement = placement;
    slabPool->validMask = validMask;
    slabPool->numBuckets = numSizes;
    slabPool->pageSize = getpagesize();
    slabPool->pageAlignment = pageAlignment;
    slabPool->maxSlabSize = maxSlabSize;
    slabPool->desiredNumBuffers = desiredNumBuffers;

    for (i = 0; i < slabPool->numBuckets; ++i) {
	slabPool->bucketSizes[i] = (smallestSize << i);
	wsbmInitSizeHeader(slabPool, slabPool->bucketSizes[i],
			   &slabPool->headers[i]);
    }

    pool->fd = fd;
    pool->map = &pool_map;
    pool->unmap = &pool_unmap;
    pool->destroy = &pool_destroy;
    pool->offset = &pool_offset;
    pool->poolOffset = &pool_poolOffset;
    pool->size = &pool_size;
    pool->create = &pool_create;
    pool->fence = &pool_fence;
    pool->kernel = &pool_kernel;
    pool->validate = &pool_validate;
    pool->unvalidate = &pool_unvalidate;
    pool->waitIdle = &pool_waitIdle;
    pool->takeDown = &pool_takedown;
    pool->releasefromcpu = &pool_releaseFromCpu;
    pool->syncforcpu = &pool_syncForCpu;

    return pool;

  out_err1:
    free(slabPool->bucketSizes);
  out_err0:
    free(slabPool);

    return NULL;
}
