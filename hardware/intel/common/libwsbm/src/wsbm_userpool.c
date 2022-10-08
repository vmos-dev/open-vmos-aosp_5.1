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
 * Authors: Thomas Hellstrom <thomas-at-tungstengraphics-dot-com>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wsbm_pool.h"
#include "wsbm_fencemgr.h"
#include "wsbm_manager.h"
#include "wsbm_mm.h"
#include "wsbm_priv.h"

/*
 * Malloced memory must be aligned to 16 bytes, since that's what
 * the DMA bitblt requires.
 */

#define WSBM_USER_ALIGN_ADD 16
#define WSBM_USER_ALIGN_SYSMEM(_val) \
    ((void *)(((unsigned long) (_val) + 15) & ~15))

struct _WsbmUserBuffer
{
    struct _WsbmBufStorage buf;
    struct _WsbmKernelBuf kBuf;

    /* Protected by the pool mutex */

    struct _WsbmListHead lru;
    struct _WsbmListHead delayed;

    /* Protected by the buffer mutex */

    unsigned long size;
    unsigned long alignment;

    struct _WsbmCond event;
    uint32_t proposedPlacement;
    uint32_t newFenceType;

    void *map;
    void *sysmem;
    int unFenced;
    struct _WsbmFenceObject *fence;
    struct _WsbmMMNode *node;

    struct _WsbmAtomic writers;
};

struct _WsbmUserPool
{
    /*
     * Constant after initialization.
     */

    struct _WsbmBufferPool pool;
    unsigned long agpOffset;
    unsigned long agpMap;
    unsigned long agpSize;
    unsigned long vramOffset;
    unsigned long vramMap;
    unsigned long vramSize;
    struct _WsbmMutex mutex;
    struct _WsbmListHead delayed;
    struct _WsbmListHead vramLRU;
    struct _WsbmListHead agpLRU;
    struct _WsbmMM vramMM;
    struct _WsbmMM agpMM;
        uint32_t(*fenceTypes) (uint64_t);
};

static inline struct _WsbmUserPool *
userPool(struct _WsbmUserBuffer *buf)
{
    return containerOf(buf->buf.pool, struct _WsbmUserPool, pool);
}

static inline struct _WsbmUserBuffer *
userBuf(struct _WsbmBufStorage *buf)
{
    return containerOf(buf, struct _WsbmUserBuffer, buf);
}

static void
waitIdleLocked(struct _WsbmBufStorage *buf, int lazy)
{
    struct _WsbmUserBuffer *vBuf = userBuf(buf);

    while (vBuf->unFenced || vBuf->fence != NULL) {
	if (vBuf->unFenced)
	    WSBM_COND_WAIT(&vBuf->event, &buf->mutex);

	if (vBuf->fence != NULL) {
	    if (!wsbmFenceSignaled(vBuf->fence, vBuf->kBuf.fence_type_mask)) {
		struct _WsbmFenceObject *fence =
		    wsbmFenceReference(vBuf->fence);

		WSBM_MUTEX_UNLOCK(&buf->mutex);
		(void)wsbmFenceFinish(fence, vBuf->kBuf.fence_type_mask,
				      lazy);
		WSBM_MUTEX_LOCK(&buf->mutex);

		if (vBuf->fence == fence)
		    wsbmFenceUnreference(&vBuf->fence);

		wsbmFenceUnreference(&fence);
	    } else {
		wsbmFenceUnreference(&vBuf->fence);
	    }
	}
    }
}

static int
pool_waitIdle(struct _WsbmBufStorage *buf, int lazy)
{
    WSBM_MUTEX_UNLOCK(&buf->mutex);
    waitIdleLocked(buf, lazy);
    WSBM_MUTEX_UNLOCK(&buf->mutex);

    return 0;
}

static int
evict_lru(struct _WsbmListHead *lru)
{
    struct _WsbmUserBuffer *vBuf;
    struct _WsbmUserPool *p;
    struct _WsbmListHead *list = lru->next;
    int err;

    if (list == lru) {
	return -ENOMEM;
    }

    vBuf = WSBMLISTENTRY(list, struct _WsbmUserBuffer, lru);
    p = userPool(vBuf);
    WSBM_MUTEX_UNLOCK(&p->mutex);
    WSBM_MUTEX_LOCK(&vBuf->buf.mutex);
    WSBM_MUTEX_LOCK(&p->mutex);

    vBuf->sysmem = malloc(vBuf->size + WSBM_USER_ALIGN_ADD);

    if (!vBuf->sysmem) {
	err = -ENOMEM;
	goto out_unlock;
    }

    (void)wsbmFenceFinish(vBuf->fence, vBuf->kBuf.fence_type_mask, 0);
    wsbmFenceUnreference(&vBuf->fence);

    memcpy(WSBM_USER_ALIGN_SYSMEM(vBuf->sysmem), vBuf->map, vBuf->size);
    WSBMLISTDELINIT(&vBuf->lru);
    vBuf->kBuf.placement = WSBM_PL_FLAG_SYSTEM;
    vBuf->map = WSBM_USER_ALIGN_SYSMEM(vBuf->sysmem);

    /*
     * FIXME: Free memory.
     */

    err = 0;
  out_unlock:
    WSBM_MUTEX_UNLOCK(&vBuf->buf.mutex);
    return err;
}

static struct _WsbmBufStorage *
pool_create(struct _WsbmBufferPool *pool,
	    unsigned long size, uint32_t placement, unsigned alignment)
{
    struct _WsbmUserPool *p = containerOf(pool, struct _WsbmUserPool, pool);
    struct _WsbmUserBuffer *vBuf = calloc(1, sizeof(*vBuf));

    if (!vBuf)
	return NULL;

    wsbmBufStorageInit(&vBuf->buf, pool);
    vBuf->sysmem = NULL;
    vBuf->proposedPlacement = placement;
    vBuf->size = size;
    vBuf->alignment = alignment;

    WSBMINITLISTHEAD(&vBuf->lru);
    WSBMINITLISTHEAD(&vBuf->delayed);
    WSBM_MUTEX_LOCK(&p->mutex);

    if (placement & WSBM_PL_FLAG_TT) {
	vBuf->node = wsbmMMSearchFree(&p->agpMM, size, alignment, 1);
	if (vBuf->node)
	    vBuf->node = wsbmMMGetBlock(vBuf->node, size, alignment);

	if (vBuf->node) {
	    vBuf->kBuf.placement = WSBM_PL_FLAG_TT;
	    vBuf->kBuf.gpuOffset = p->agpOffset + vBuf->node->start;
	    vBuf->map = (void *)(p->agpMap + vBuf->node->start);
	    WSBMLISTADDTAIL(&vBuf->lru, &p->agpLRU);
	    goto have_mem;
	}
    }

    if (placement & WSBM_PL_FLAG_VRAM) {
	vBuf->node = wsbmMMSearchFree(&p->vramMM, size, alignment, 1);
	if (vBuf->node)
	    vBuf->node = wsbmMMGetBlock(vBuf->node, size, alignment);

	if (vBuf->node) {
	    vBuf->kBuf.placement = WSBM_PL_FLAG_VRAM;
	    vBuf->kBuf.gpuOffset = p->vramOffset + vBuf->node->start;
	    vBuf->map = (void *)(p->vramMap + vBuf->node->start);
	    WSBMLISTADDTAIL(&vBuf->lru, &p->vramLRU);
	    goto have_mem;
	}
    }

    if ((placement & WSBM_PL_FLAG_NO_EVICT)
	&& !(placement & WSBM_PL_FLAG_SYSTEM)) {
	WSBM_MUTEX_UNLOCK(&p->mutex);
	goto out_err;
    }

    vBuf->sysmem = malloc(size + WSBM_USER_ALIGN_ADD);
    vBuf->kBuf.placement = WSBM_PL_FLAG_SYSTEM;
    vBuf->map = WSBM_USER_ALIGN_SYSMEM(vBuf->sysmem);

  have_mem:

    WSBM_MUTEX_UNLOCK(&p->mutex);
    if (vBuf->sysmem != NULL
	|| (!(vBuf->kBuf.placement & WSBM_PL_FLAG_SYSTEM)))
	return &vBuf->buf;
  out_err:
    free(vBuf);
    return NULL;
}

static int
pool_validate(struct _WsbmBufStorage *buf, uint64_t set_flags,
	      uint64_t clr_flags)
{
    struct _WsbmUserBuffer *vBuf = userBuf(buf);
    struct _WsbmUserPool *p = userPool(vBuf);
    int err = -ENOMEM;

    WSBM_MUTEX_LOCK(&buf->mutex);

    while (wsbmAtomicRead(&vBuf->writers) != 0)
	WSBM_COND_WAIT(&vBuf->event, &buf->mutex);

    vBuf->unFenced = 1;

    WSBM_MUTEX_LOCK(&p->mutex);
    WSBMLISTDELINIT(&vBuf->lru);

    vBuf->proposedPlacement =
	(vBuf->proposedPlacement | set_flags) & ~clr_flags;

    if ((vBuf->proposedPlacement & vBuf->kBuf.placement & WSBM_PL_MASK_MEM) ==
	vBuf->kBuf.placement) {
	err = 0;
	goto have_mem;
    }

    /*
     * We're moving to another memory region, so evict first and we'll
     * do a sw copy to the other region.
     */

    if (!(vBuf->kBuf.placement & WSBM_PL_FLAG_SYSTEM)) {
	struct _WsbmListHead tmpLRU;

	WSBMINITLISTHEAD(&tmpLRU);
	WSBMLISTADDTAIL(&tmpLRU, &vBuf->lru);
	err = evict_lru(&tmpLRU);
	if (err)
	    goto have_mem;
    }

    if (vBuf->proposedPlacement & WSBM_PL_FLAG_TT) {
	do {
	    vBuf->node =
		wsbmMMSearchFree(&p->agpMM, vBuf->size, vBuf->alignment, 1);
	    if (vBuf->node)
		vBuf->node =
		    wsbmMMGetBlock(vBuf->node, vBuf->size, vBuf->alignment);

	    if (vBuf->node) {
		vBuf->kBuf.placement = WSBM_PL_FLAG_TT;
		vBuf->kBuf.gpuOffset = p->agpOffset + vBuf->node->start;
		vBuf->map = (void *)(p->agpMap + vBuf->node->start);
		memcpy(vBuf->map, WSBM_USER_ALIGN_SYSMEM(vBuf->sysmem),
		       vBuf->size);
		free(vBuf->sysmem);
		goto have_mem;
	    }
	} while (evict_lru(&p->agpLRU) == 0);
    }

    if (vBuf->proposedPlacement & WSBM_PL_FLAG_VRAM) {
	do {
	    vBuf->node =
		wsbmMMSearchFree(&p->vramMM, vBuf->size, vBuf->alignment, 1);
	    if (vBuf->node)
		vBuf->node =
		    wsbmMMGetBlock(vBuf->node, vBuf->size, vBuf->alignment);

	    if (!err && vBuf->node) {
		vBuf->kBuf.placement = WSBM_PL_FLAG_VRAM;
		vBuf->kBuf.gpuOffset = p->vramOffset + vBuf->node->start;
		vBuf->map = (void *)(p->vramMap + vBuf->node->start);
		memcpy(vBuf->map, WSBM_USER_ALIGN_SYSMEM(vBuf->sysmem),
		       vBuf->size);
		free(vBuf->sysmem);
		goto have_mem;
	    }
	} while (evict_lru(&p->vramLRU) == 0);
    }

    if (vBuf->proposedPlacement & WSBM_PL_FLAG_SYSTEM)
	goto have_mem;

    err = -ENOMEM;

  have_mem:
    vBuf->newFenceType = p->fenceTypes(set_flags);
    WSBM_MUTEX_UNLOCK(&p->mutex);
    WSBM_MUTEX_UNLOCK(&buf->mutex);
    return err;
}

static int
pool_setStatus(struct _WsbmBufStorage *buf,
	       uint32_t set_placement, uint32_t clr_placement)
{
    struct _WsbmUserBuffer *vBuf = userBuf(buf);
    int ret;

    ret = pool_validate(buf, set_placement, clr_placement);
    vBuf->unFenced = 0;
    return ret;
}

void
release_delayed_buffers(struct _WsbmUserPool *p)
{
    struct _WsbmUserBuffer *vBuf;
    struct _WsbmListHead *list, *next;

    WSBM_MUTEX_LOCK(&p->mutex);

    /*
     * We don't need to take the buffer mutexes in this loop, since
     * the only other user is the evict_lru function, which has the
     * pool mutex held when accessing the buffer fence member.
     */

    WSBMLISTFOREACHSAFE(list, next, &p->delayed) {
	vBuf = WSBMLISTENTRY(list, struct _WsbmUserBuffer, delayed);

	if (!vBuf->fence
	    || wsbmFenceSignaled(vBuf->fence, vBuf->kBuf.fence_type_mask)) {
	    if (vBuf->fence)
		wsbmFenceUnreference(&vBuf->fence);

	    WSBMLISTDEL(&vBuf->delayed);
	    WSBMLISTDEL(&vBuf->lru);

	    if ((vBuf->kBuf.placement & WSBM_PL_FLAG_SYSTEM) == 0)
		wsbmMMPutBlock(vBuf->node);
	    else
		free(vBuf->sysmem);

	    free(vBuf);
	} else
	    break;

    }
    WSBM_MUTEX_UNLOCK(&p->mutex);
}

static void
pool_destroy(struct _WsbmBufStorage **buf)
{
    struct _WsbmUserBuffer *vBuf = userBuf(*buf);
    struct _WsbmUserPool *p = userPool(vBuf);

    *buf = NULL;

    WSBM_MUTEX_LOCK(&vBuf->buf.mutex);
    if ((vBuf->fence
	 && !wsbmFenceSignaled(vBuf->fence, vBuf->kBuf.fence_type_mask))) {
	WSBM_MUTEX_LOCK(&p->mutex);
	WSBMLISTADDTAIL(&vBuf->delayed, &p->delayed);
	WSBM_MUTEX_UNLOCK(&p->mutex);
	WSBM_MUTEX_UNLOCK(&vBuf->buf.mutex);
	return;
    }

    if (vBuf->fence)
	wsbmFenceUnreference(&vBuf->fence);

    WSBM_MUTEX_LOCK(&p->mutex);
    WSBMLISTDEL(&vBuf->lru);
    WSBM_MUTEX_UNLOCK(&p->mutex);

    if (!(vBuf->kBuf.placement & WSBM_PL_FLAG_SYSTEM))
	wsbmMMPutBlock(vBuf->node);
    else
	free(vBuf->sysmem);

    free(vBuf);
    return;
}

static int
pool_map(struct _WsbmBufStorage *buf, unsigned mode __attribute__ ((unused)), void **virtual)
{
    struct _WsbmUserBuffer *vBuf = userBuf(buf);

    *virtual = vBuf->map;
    return 0;
}

static void
pool_unmap(struct _WsbmBufStorage *buf __attribute__ ((unused)))
{
    ;
}

static void
pool_releaseFromCpu(struct _WsbmBufStorage *buf, unsigned mode __attribute__ ((unused)))
{
    struct _WsbmUserBuffer *vBuf = userBuf(buf);

    if (wsbmAtomicDecZero(&vBuf->writers))
	WSBM_COND_BROADCAST(&vBuf->event);

}

static int
pool_syncForCpu(struct _WsbmBufStorage *buf, unsigned mode)
{
    struct _WsbmUserBuffer *vBuf = userBuf(buf);
    int ret = 0;

    WSBM_MUTEX_LOCK(&buf->mutex);
    if ((mode & WSBM_SYNCCPU_DONT_BLOCK)) {

	if (vBuf->unFenced) {
	    ret = -EBUSY;
	    goto out_unlock;
	}

	ret = 0;
	if ((vBuf->fence == NULL) ||
	    wsbmFenceSignaled(vBuf->fence, vBuf->kBuf.fence_type_mask)) {
	    wsbmFenceUnreference(&vBuf->fence);
	    wsbmAtomicInc(&vBuf->writers);
	} else
	    ret = -EBUSY;

	goto out_unlock;
    }
    waitIdleLocked(buf, 0);
    wsbmAtomicInc(&vBuf->writers);
  out_unlock:
    WSBM_MUTEX_UNLOCK(&buf->mutex);
    return ret;
}

static unsigned long
pool_offset(struct _WsbmBufStorage *buf)
{
    return userBuf(buf)->kBuf.gpuOffset;
}

static unsigned long
pool_poolOffset(struct _WsbmBufStorage *buf __attribute__ ((unused)))
{
    return 0UL;
}

static unsigned long
pool_size(struct _WsbmBufStorage *buf)
{
    return userBuf(buf)->size;
}

static void
pool_fence(struct _WsbmBufStorage *buf, struct _WsbmFenceObject *fence)
{
    struct _WsbmUserBuffer *vBuf = userBuf(buf);
    struct _WsbmUserPool *p = userPool(vBuf);

    WSBM_MUTEX_LOCK(&buf->mutex);

    if (vBuf->fence)
	wsbmFenceUnreference(&vBuf->fence);

    vBuf->fence = wsbmFenceReference(fence);
    vBuf->unFenced = 0;
    vBuf->kBuf.fence_type_mask = vBuf->newFenceType;

    WSBM_COND_BROADCAST(&vBuf->event);
    WSBM_MUTEX_LOCK(&p->mutex);
    if (vBuf->kBuf.placement & WSBM_PL_FLAG_VRAM)
	WSBMLISTADDTAIL(&vBuf->lru, &p->vramLRU);
    else if (vBuf->kBuf.placement & WSBM_PL_FLAG_TT)
	WSBMLISTADDTAIL(&vBuf->lru, &p->agpLRU);
    WSBM_MUTEX_UNLOCK(&p->mutex);
    WSBM_MUTEX_UNLOCK(&buf->mutex);
}

static void
pool_unvalidate(struct _WsbmBufStorage *buf)
{
    struct _WsbmUserBuffer *vBuf = userBuf(buf);
    struct _WsbmUserPool *p = userPool(vBuf);

    WSBM_MUTEX_LOCK(&buf->mutex);

    if (!vBuf->unFenced)
	goto out_unlock;

    vBuf->unFenced = 0;
    WSBM_COND_BROADCAST(&vBuf->event);
    WSBM_MUTEX_LOCK(&p->mutex);
    if (vBuf->kBuf.placement & WSBM_PL_FLAG_VRAM)
	WSBMLISTADDTAIL(&vBuf->lru, &p->vramLRU);
    else if (vBuf->kBuf.placement & WSBM_PL_FLAG_TT)
	WSBMLISTADDTAIL(&vBuf->lru, &p->agpLRU);
    WSBM_MUTEX_UNLOCK(&p->mutex);

  out_unlock:

    WSBM_MUTEX_UNLOCK(&buf->mutex);
}

static struct _WsbmKernelBuf *
pool_kernel(struct _WsbmBufStorage *buf)
{
    struct _WsbmUserBuffer *vBuf = userBuf(buf);

    return &vBuf->kBuf;
}

static void
pool_takedown(struct _WsbmBufferPool *pool)
{
    struct _WsbmUserPool *p = containerOf(pool, struct _WsbmUserPool, pool);
    int empty;

    do {
	release_delayed_buffers(p);
	WSBM_MUTEX_LOCK(&p->mutex);
	empty = (p->delayed.next == &p->delayed);
	WSBM_MUTEX_UNLOCK(&p->mutex);

	if (!empty)
	    usleep(1000);

    } while (!empty);
    WSBM_MUTEX_LOCK(&p->mutex);

    while (evict_lru(&p->vramLRU) == 0) ;
    while (evict_lru(&p->agpLRU) == 0) ;

    WSBM_MUTEX_UNLOCK(&p->mutex);

    wsbmMMtakedown(&p->agpMM);
    wsbmMMtakedown(&p->vramMM);

    free(p);
}

void
wsbmUserPoolClean(struct _WsbmBufferPool *pool, int cleanVram, int cleanAgp)
{
    struct _WsbmUserPool *p = containerOf(pool, struct _WsbmUserPool, pool);

    WSBM_MUTEX_LOCK(&p->mutex);
    if (cleanVram)
	while (evict_lru(&p->vramLRU) == 0) ;
    if (cleanAgp)
	while (evict_lru(&p->agpLRU) == 0) ;
    WSBM_MUTEX_UNLOCK(&p->mutex);
}

struct _WsbmBufferPool *
wsbmUserPoolInit(void *vramAddr,
		 unsigned long vramStart, unsigned long vramSize,
		 void *agpAddr, unsigned long agpStart,
		 unsigned long agpSize,
		 uint32_t(*fenceTypes) (uint64_t set_flags))
{
    struct _WsbmBufferPool *pool;
    struct _WsbmUserPool *uPool;
    int ret;

    uPool = calloc(1, sizeof(*uPool));
    if (!uPool)
	goto out_err0;

    ret = WSBM_MUTEX_INIT(&uPool->mutex);
    if (ret)
	goto out_err0;

    ret = wsbmMMinit(&uPool->vramMM, 0, vramSize);
    if (ret)
	goto out_err1;

    ret = wsbmMMinit(&uPool->agpMM, 0, agpSize);
    if (ret)
	goto out_err2;

    WSBMINITLISTHEAD(&uPool->delayed);
    WSBMINITLISTHEAD(&uPool->vramLRU);
    WSBMINITLISTHEAD(&uPool->agpLRU);

    uPool->agpOffset = agpStart;
    uPool->agpMap = (unsigned long)agpAddr;
    uPool->vramOffset = vramStart;
    uPool->vramMap = (unsigned long)vramAddr;
    uPool->fenceTypes = fenceTypes;

    pool = &uPool->pool;
    pool->map = &pool_map;
    pool->unmap = &pool_unmap;
    pool->destroy = &pool_destroy;
    pool->offset = &pool_offset;
    pool->poolOffset = &pool_poolOffset;
    pool->size = &pool_size;
    pool->create = &pool_create;
    pool->fence = &pool_fence;
    pool->unvalidate = &pool_unvalidate;
    pool->kernel = &pool_kernel;
    pool->validate = &pool_validate;
    pool->waitIdle = &pool_waitIdle;
    pool->takeDown = &pool_takedown;
    pool->setStatus = &pool_setStatus;
    pool->syncforcpu = &pool_syncForCpu;
    pool->releasefromcpu = &pool_releaseFromCpu;

    return pool;

  out_err2:
    wsbmMMtakedown(&uPool->vramMM);
  out_err1:
    WSBM_MUTEX_FREE(&uPool->mutex);
  out_err0:
    free(uPool);

    return NULL;
}
