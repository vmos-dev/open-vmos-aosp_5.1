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
 * Authors: Thomas Hellström <thomas-at-tungstengraphics-dot-com>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <errno.h>
#include "wsbm_pool.h"
#include "wsbm_manager.h"

struct _WsbmMallocBuffer
{
    struct _WsbmBufStorage buf;
    size_t size;
    void *mem;
};

static inline struct _WsbmMallocBuffer *
mallocBuf(struct _WsbmBufStorage *buf)
{
    return containerOf(buf, struct _WsbmMallocBuffer, buf);
}

static struct _WsbmBufStorage *
pool_create(struct _WsbmBufferPool *pool,
        unsigned long size, uint32_t placement, unsigned alignment __attribute__ ((unused)))
{
    struct _WsbmMallocBuffer *mBuf = malloc(size + sizeof(*mBuf) + 16);

    if (!mBuf)
	return NULL;

    wsbmBufStorageInit(&mBuf->buf, pool);
    mBuf->size = size;
    mBuf->mem = (void *)((unsigned long)mBuf + sizeof(*mBuf));
    if ((placement & WSBM_PL_MASK_MEM) != WSBM_PL_FLAG_SYSTEM)
	abort();

    return &mBuf->buf;
}

static void
pool_destroy(struct _WsbmBufStorage **buf)
{
    free(mallocBuf(*buf));
    *buf = NULL;
}

static int
pool_waitIdle(struct _WsbmBufStorage *buf __attribute__ ((unused)), int lazy __attribute__ ((unused)))
{
    return 0;
}

static int
pool_map(struct _WsbmBufStorage *buf, unsigned mode __attribute__ ((unused)), void **virtual __attribute__ ((unused)))
{
    *virtual = mallocBuf(buf)->mem;
    return 0;
}

static void
pool_unmap(struct _WsbmBufStorage *buf __attribute__ ((unused)))
{
    ;
}

static int
pool_syncforcpu(struct _WsbmBufStorage *buf __attribute__ ((unused)), unsigned mode __attribute__ ((unused)))
{
    return 0;
}

static void
pool_releasefromcpu(struct _WsbmBufStorage *buf __attribute__ ((unused)), unsigned mode __attribute__ ((unused)))
{
    ;
}

static unsigned long
pool_offset(struct _WsbmBufStorage *buf __attribute__ ((unused)))
{
    /*
     * BUG
     */
    abort();
    return 0UL;
}

static unsigned long
pool_poolOffset(struct _WsbmBufStorage *buf __attribute__ ((unused)))
{
    /*
     * BUG
     */
    abort();
}

static uint32_t
pool_placement(struct _WsbmBufStorage *buf __attribute__ ((unused)))
{
    return WSBM_PL_FLAG_SYSTEM | WSBM_PL_FLAG_CACHED;
}

static unsigned long
pool_size(struct _WsbmBufStorage *buf)
{
    return mallocBuf(buf)->size;
}

static void
pool_fence(struct _WsbmBufStorage *buf __attribute__ ((unused)), struct _WsbmFenceObject *fence __attribute__ ((unused)))
{
    abort();
}

static struct _WsbmKernelBuf *
pool_kernel(struct _WsbmBufStorage *buf __attribute__ ((unused)))
{
    abort();
    return NULL;
}

static void
pool_takedown(struct _WsbmBufferPool *pool)
{
    free(pool);
}

struct _WsbmBufferPool *
wsbmMallocPoolInit(void)
{
    struct _WsbmBufferPool *pool;

    pool = (struct _WsbmBufferPool *)calloc(1, sizeof(*pool));
    if (!pool)
	return NULL;

    pool->fd = -1;
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
    pool->waitIdle = &pool_waitIdle;
    pool->takeDown = &pool_takedown;
    return pool;
}
