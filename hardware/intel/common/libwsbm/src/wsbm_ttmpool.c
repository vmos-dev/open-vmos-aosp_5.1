/**************************************************************************
 *
 * Copyright 2006-2008 Tungsten Graphics, Inc., Cedar Park, Tx., USA
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <xf86drm.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <drm/psb_ttm_placement_user.h>
#include "wsbm_pool.h"
#include "assert.h"
#include "wsbm_priv.h"
#include "wsbm_manager.h"

#define DRMRESTARTCOMMANDWRITE(_fd, _val, _arg, _ret)			\
	do {								\
		(_ret) = drmCommandWrite(_fd, _val, &(_arg), sizeof(_arg)); \
	} while ((_ret) == -EAGAIN || (_ret) == -ERESTART);		\

#define DRMRESTARTCOMMANDWRITEREAD(_fd, _val, _arg, _ret)		\
	do {								\
		(_ret) = drmCommandWriteRead(_fd, _val, &(_arg), sizeof(_arg)); \
	} while ((_ret) == -EAGAIN || (_ret) == -ERESTART);		\

/*
 * Buffer pool implementation using DRM buffer objects as wsbm buffer objects.
 */

struct _TTMBuffer
{
    struct _WsbmBufStorage buf;
    struct _WsbmCond event;

    /*
     * Remains constant after creation.
     */

    uint64_t requestedSize;
    uint64_t mapHandle;
    uint64_t realSize;

    /*
     * Protected by the kernel lock.
     */

    struct _WsbmKernelBuf kBuf;

    /*
     * Protected by the mutex.
     */

    void *virtual;
    int syncInProgress;
    unsigned readers;
    unsigned writers;
};

struct _TTMPool
{
    struct _WsbmBufferPool pool;
    unsigned int pageSize;
    unsigned int devOffset;
};

static inline struct _TTMPool *
ttmGetPool(struct _TTMBuffer *dBuf)
{
    return containerOf(dBuf->buf.pool, struct _TTMPool, pool);
}

static inline struct _TTMBuffer *
ttmBuffer(struct _WsbmBufStorage *buf)
{
    return containerOf(buf, struct _TTMBuffer, buf);
}

static struct _WsbmBufStorage *
pool_create(struct _WsbmBufferPool *pool,
	    unsigned long size, uint32_t placement, unsigned alignment)
{
    struct _TTMBuffer *dBuf = (struct _TTMBuffer *)
	calloc(1, sizeof(*dBuf));
    struct _TTMPool *ttmPool = containerOf(pool, struct _TTMPool, pool);
    int ret;
    unsigned pageSize = ttmPool->pageSize;
    union ttm_pl_create_arg arg;

    if (!dBuf)
	return NULL;

    if ((alignment > pageSize) && (alignment % pageSize))
	goto out_err0;

    ret = wsbmBufStorageInit(&dBuf->buf, pool);
    if (ret)
	goto out_err0;

    ret = WSBM_COND_INIT(&dBuf->event);
    if (ret)
	goto out_err1;

    arg.req.size = size;
    arg.req.placement = placement;
    arg.req.page_alignment = alignment / pageSize;

    DRMRESTARTCOMMANDWRITEREAD(pool->fd, ttmPool->devOffset + TTM_PL_CREATE,
			       arg, ret);

    if (ret)
	goto out_err2;

    dBuf->requestedSize = size;
    dBuf->kBuf.gpuOffset = arg.rep.gpu_offset;
    dBuf->mapHandle = arg.rep.map_handle;
    dBuf->realSize = arg.rep.bo_size;
    dBuf->kBuf.placement = arg.rep.placement;
    dBuf->kBuf.handle = arg.rep.handle;

    return &dBuf->buf;

  out_err2:
    WSBM_COND_FREE(&dBuf->event);
  out_err1:
    wsbmBufStorageTakedown(&dBuf->buf);
  out_err0:
    free(dBuf);
    return NULL;
}

static struct _WsbmBufStorage *
pool_reference(struct _WsbmBufferPool *pool, unsigned handle)
{
    struct _TTMBuffer *dBuf = (struct _TTMBuffer *)calloc(1, sizeof(*dBuf));
    struct _TTMPool *ttmPool = containerOf(pool, struct _TTMPool, pool);
    union ttm_pl_reference_arg arg;
    int ret;

    if (!dBuf)
	return NULL;

    ret = wsbmBufStorageInit(&dBuf->buf, pool);
    if (ret)
	goto out_err0;

    ret = WSBM_COND_INIT(&dBuf->event);
    if (ret)
	goto out_err1;

    arg.req.handle = handle;
    ret = drmCommandWriteRead(pool->fd, ttmPool->devOffset + TTM_PL_REFERENCE,
			      &arg, sizeof(arg));

    if (ret)
	goto out_err2;

    dBuf->requestedSize = arg.rep.bo_size;
    dBuf->kBuf.gpuOffset = arg.rep.gpu_offset;
    dBuf->mapHandle = arg.rep.map_handle;
    dBuf->realSize = arg.rep.bo_size;
    dBuf->kBuf.placement = arg.rep.placement;
    dBuf->kBuf.handle = arg.rep.handle;
    dBuf->kBuf.fence_type_mask = arg.rep.sync_object_arg;

    return &dBuf->buf;

  out_err2:
    WSBM_COND_FREE(&dBuf->event);
  out_err1:
    wsbmBufStorageTakedown(&dBuf->buf);
  out_err0:
    free(dBuf);
    return NULL;
}

static void
pool_destroy(struct _WsbmBufStorage **buf)
{
    struct _TTMBuffer *dBuf = ttmBuffer(*buf);
    struct _TTMPool *ttmPool = ttmGetPool(dBuf);
    struct ttm_pl_reference_req arg;

    *buf = NULL;
    if (dBuf->virtual != NULL) {
	(void)munmap(dBuf->virtual, dBuf->requestedSize);
	dBuf->virtual = NULL;
    }
    arg.handle = dBuf->kBuf.handle;
    (void)drmCommandWrite(dBuf->buf.pool->fd,
			  ttmPool->devOffset + TTM_PL_UNREF,
			  &arg, sizeof(arg));

    WSBM_COND_FREE(&dBuf->event);
    wsbmBufStorageTakedown(&dBuf->buf);
    free(dBuf);
}

static int
syncforcpu_locked(struct _WsbmBufStorage *buf, unsigned mode)
{
    uint32_t kmode = 0;
    struct _TTMBuffer *dBuf = ttmBuffer(buf);
    struct _TTMPool *ttmPool = ttmGetPool(dBuf);
    unsigned int readers;
    unsigned int writers;
    int ret = 0;

    while (dBuf->syncInProgress)
	WSBM_COND_WAIT(&dBuf->event, &buf->mutex);

    readers = dBuf->readers;
    writers = dBuf->writers;

    if ((mode & WSBM_SYNCCPU_READ) && (++dBuf->readers == 1))
	kmode |= TTM_PL_SYNCCPU_MODE_READ;

    if ((mode & WSBM_SYNCCPU_WRITE) && (++dBuf->writers == 1))
	kmode |= TTM_PL_SYNCCPU_MODE_WRITE;

    if (kmode) {
	struct ttm_pl_synccpu_arg arg;

	if (mode & WSBM_SYNCCPU_DONT_BLOCK)
	    kmode |= TTM_PL_SYNCCPU_MODE_NO_BLOCK;

	dBuf->syncInProgress = 1;

	/*
	 * This might be a lengthy wait, so
	 * release the mutex.
	 */

	WSBM_MUTEX_UNLOCK(&buf->mutex);

	arg.handle = dBuf->kBuf.handle;
	arg.access_mode = kmode;
	arg.op = TTM_PL_SYNCCPU_OP_GRAB;

	DRMRESTARTCOMMANDWRITE(dBuf->buf.pool->fd,
			       ttmPool->devOffset + TTM_PL_SYNCCPU, arg, ret);

	WSBM_MUTEX_LOCK(&buf->mutex);
	dBuf->syncInProgress = 0;
	WSBM_COND_BROADCAST(&dBuf->event);

	if (ret) {
	    dBuf->readers = readers;
	    dBuf->writers = writers;
	}
    }

    return ret;
}

static int
releasefromcpu_locked(struct _WsbmBufStorage *buf, unsigned mode)
{
    uint32_t kmode = 0;
    struct _TTMBuffer *dBuf = ttmBuffer(buf);
    struct _TTMPool *ttmPool = ttmGetPool(dBuf);
    int ret = 0;

    while (dBuf->syncInProgress)
	WSBM_COND_WAIT(&dBuf->event, &buf->mutex);

    if ((mode & WSBM_SYNCCPU_READ) && (--dBuf->readers == 0))
	kmode |= TTM_PL_SYNCCPU_MODE_READ;

    if ((mode & WSBM_SYNCCPU_WRITE) && (--dBuf->writers == 0))
	kmode |= TTM_PL_SYNCCPU_MODE_WRITE;

    if (kmode) {
	struct ttm_pl_synccpu_arg arg;

	arg.handle = dBuf->kBuf.handle;
	arg.access_mode = kmode;
	arg.op = TTM_PL_SYNCCPU_OP_RELEASE;

	DRMRESTARTCOMMANDWRITE(dBuf->buf.pool->fd,
			       ttmPool->devOffset + TTM_PL_SYNCCPU, arg, ret);

    }

    return ret;
}

static int
pool_syncforcpu(struct _WsbmBufStorage *buf, unsigned mode)
{
    int ret;

    WSBM_MUTEX_LOCK(&buf->mutex);
    ret = syncforcpu_locked(buf, mode);
    WSBM_MUTEX_UNLOCK(&buf->mutex);
    return ret;
}

static void
pool_releasefromcpu(struct _WsbmBufStorage *buf, unsigned mode)
{
    WSBM_MUTEX_LOCK(&buf->mutex);
    (void)releasefromcpu_locked(buf, mode);
    WSBM_MUTEX_UNLOCK(&buf->mutex);
}

#ifdef ANDROID

/* No header but syscall provided by bionic */
void*  __mmap2(void*, size_t, int, int, int, size_t);
#define MMAP2_SHIFT 12 // 2**12 == 4096

static void* _temp_mmap(void *addr, size_t size, int prot, int flags, int fd, long long offset)
{
    return __mmap2(addr, size, prot, flags, fd, (unsigned long)(offset >> MMAP2_SHIFT));
}

#endif

static int
pool_map(struct _WsbmBufStorage *buf, unsigned mode __attribute__ ((unused)), void **virtual)
{
    struct _TTMBuffer *dBuf = ttmBuffer(buf);
    void *virt;
    int ret = 0;

    WSBM_MUTEX_LOCK(&buf->mutex);

    /*
     * mmaps are expensive, so we only really unmap if
     * we destroy the buffer.
     */

    if (dBuf->virtual == NULL) {
#if defined(__LP64__) || defined(_LP64) || defined(__LP64)
	virt = mmap(0, dBuf->requestedSize,
		    PROT_READ | PROT_WRITE, MAP_SHARED,
		    buf->pool->fd, dBuf->mapHandle);
#else
	virt = _temp_mmap(0, dBuf->requestedSize,
		    PROT_READ | PROT_WRITE, MAP_SHARED,
		    buf->pool->fd, dBuf->mapHandle);
#endif
	if (virt == MAP_FAILED) {
	    ret = -errno;
	    goto out_unlock;
	}
	dBuf->virtual = virt;
    }

    *virtual = dBuf->virtual;
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
pool_offset(struct _WsbmBufStorage *buf)
{
    struct _TTMBuffer *dBuf = ttmBuffer(buf);

    return dBuf->kBuf.gpuOffset;
}

static unsigned long
pool_poolOffset(struct _WsbmBufStorage *buf __attribute__ ((unused)))
{
    return 0;
}

static uint32_t
pool_placement(struct _WsbmBufStorage *buf)
{
    struct _TTMBuffer *dBuf = ttmBuffer(buf);

    return dBuf->kBuf.placement;
}

static unsigned long
pool_size(struct _WsbmBufStorage *buf)
{
    struct _TTMBuffer *dBuf = ttmBuffer(buf);

    return dBuf->realSize;
}

static void
pool_fence(struct _WsbmBufStorage *buf __attribute__ ((unused)),
        struct _WsbmFenceObject *fence __attribute__ ((unused)))
{
    /*
     * Noop. The kernel handles all fencing.
     */
}

static int
pool_waitIdle(struct _WsbmBufStorage *buf, int lazy)
{
    struct _TTMBuffer *dBuf = ttmBuffer(buf);
    struct _TTMPool *ttmPool = ttmGetPool(dBuf);
    struct ttm_pl_waitidle_arg req;
    struct _WsbmBufferPool *pool = buf->pool;
    int ret;

    req.handle = dBuf->kBuf.handle;
    req.mode = (lazy) ? TTM_PL_WAITIDLE_MODE_LAZY : 0;

    DRMRESTARTCOMMANDWRITE(pool->fd, ttmPool->devOffset + TTM_PL_WAITIDLE,
			   req, ret);

    return ret;
}

static void
pool_takedown(struct _WsbmBufferPool *pool)
{
    struct _TTMPool *ttmPool = containerOf(pool, struct _TTMPool, pool);

    free(ttmPool);
}

static int
pool_setStatus(struct _WsbmBufStorage *buf, uint32_t set_placement,
	       uint32_t clr_placement)
{
    struct _TTMBuffer *dBuf = ttmBuffer(buf);
    struct _TTMPool *ttmPool = ttmGetPool(dBuf);
    union ttm_pl_setstatus_arg arg;
    struct ttm_pl_setstatus_req *req = &arg.req;
    struct ttm_pl_rep *rep = &arg.rep;
    struct _WsbmBufferPool *pool = buf->pool;
    int ret;

    req->handle = dBuf->kBuf.handle;
    req->set_placement = set_placement;
    req->clr_placement = clr_placement;

    DRMRESTARTCOMMANDWRITEREAD(pool->fd,
			       ttmPool->devOffset + TTM_PL_SETSTATUS,
			       arg, ret);

    if (!ret) {
	dBuf->kBuf.gpuOffset = rep->gpu_offset;
	dBuf->kBuf.placement = rep->placement;
    }

    return ret;
}

static struct _WsbmKernelBuf *
pool_kernel(struct _WsbmBufStorage *buf)
{
    return (void *)&ttmBuffer(buf)->kBuf;
}

struct _WsbmBufferPool *
wsbmTTMPoolInit(int fd, unsigned int devOffset)
{
    struct _TTMPool *ttmPool;
    struct _WsbmBufferPool *pool;

    ttmPool = (struct _TTMPool *)calloc(1, sizeof(*ttmPool));

    if (!ttmPool)
	return NULL;

    ttmPool->pageSize = getpagesize();
    ttmPool->devOffset = devOffset;
    pool = &ttmPool->pool;

    pool->fd = fd;
    pool->map = &pool_map;
    pool->unmap = &pool_unmap;
    pool->syncforcpu = &pool_syncforcpu;
    pool->releasefromcpu = &pool_releasefromcpu;
    pool->destroy = &pool_destroy;
    pool->offset = &pool_offset;
    pool->poolOffset = &pool_poolOffset;
    pool->placement = &pool_placement;
    pool->size = &pool_size;
    pool->create = &pool_create;
    pool->fence = &pool_fence;
    pool->kernel = &pool_kernel;
    pool->validate = NULL;
    pool->unvalidate = NULL;
    pool->waitIdle = &pool_waitIdle;
    pool->takeDown = &pool_takedown;
    pool->createByReference = &pool_reference;
    pool->setStatus = &pool_setStatus;
    return pool;
}

struct _WsbmBufStorage *
ttm_pool_ub_create(struct _WsbmBufferPool *pool, unsigned long size, uint32_t placement, unsigned alignment, const unsigned long *user_ptr)
{
    struct _TTMBuffer *dBuf = (struct _TTMBuffer *)
	    calloc(1, sizeof(*dBuf));
    struct _TTMPool *ttmPool = containerOf(pool, struct _TTMPool, pool);
    int ret;
    unsigned pageSize = ttmPool->pageSize;
    union ttm_pl_create_ub_arg arg;

    if (!dBuf)
	    return NULL;

    if ((alignment > pageSize) && (alignment % pageSize))
	    goto out_err0;

    ret = wsbmBufStorageInit(&dBuf->buf, pool);
    if (ret)
	    goto out_err0;

    ret = WSBM_COND_INIT(&dBuf->event);
    if (ret)
	    goto out_err1;

    arg.req.size = size;
    arg.req.placement = placement;
    arg.req.page_alignment = alignment / pageSize;
    arg.req.user_address = (unsigned long)user_ptr;

    DRMRESTARTCOMMANDWRITEREAD(pool->fd, ttmPool->devOffset + TTM_PL_CREATE_UB,
			       arg, ret);
    if (ret)
        goto out_err2;

    dBuf->requestedSize = size;
    dBuf->kBuf.gpuOffset = arg.rep.gpu_offset;
    dBuf->mapHandle = arg.rep.map_handle;
    dBuf->realSize = arg.rep.bo_size;
    dBuf->kBuf.placement = arg.rep.placement;
    dBuf->kBuf.handle = arg.rep.handle;

    return &dBuf->buf;

  out_err2:
    WSBM_COND_FREE(&dBuf->event);
  out_err1:
    wsbmBufStorageTakedown(&dBuf->buf);
  out_err0:
    free(dBuf);
    return NULL;
}

