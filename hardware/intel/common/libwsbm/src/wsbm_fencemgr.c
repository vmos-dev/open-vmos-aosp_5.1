/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Tx., USA
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

#include "wsbm_fencemgr.h"
#include "wsbm_pool.h"
#include "wsbm_manager.h"
#include <xf86drm.h>
#include <drm/psb_ttm_fence_user.h>
#include <string.h>
#include <unistd.h>

struct _WsbmFenceClass
{
    struct _WsbmListHead head;
    struct _WsbmMutex mutex;
    struct _WsbmMutex cmd_mutex;
};

/*
 * Note: The struct _WsbmFenceMgr::Mutex should never be held
 * during sleeps, since that may block fast concurrent access to
 * fence data.
 */

struct _WsbmFenceMgr
{
    /*
     * Constant members. Need no mutex protection.
     */
    struct _WsbmFenceMgrCreateInfo info;
    void *private;

    /*
     * Atomic members. No mutex protection.
     */

    struct _WsbmAtomic count;

    /*
     * These members are protected by this->mutex
     */

    struct _WsbmFenceClass *classes;
    uint32_t num_classes;
};

struct _WsbmFenceObject
{

    /*
     * These members are constant and need no mutex protection.
     * Note that @private may point to a structure with its own
     * mutex protection, that we don't care about.
     */

    struct _WsbmFenceMgr *mgr;
    uint32_t fence_class;
    uint32_t fence_type;
    void *private;

    /*
     * Atomic members. No mutex protection. note that
     * @signaled types is updated using a compare-and-swap
     * scheme to guarantee atomicity.
     */

    struct _WsbmAtomic refCount;
    struct _WsbmAtomic signaled_types;

    /*
     * These members are protected by mgr->mutex.
     */
    struct _WsbmListHead head;
};

uint32_t
wsbmFenceType(struct _WsbmFenceObject *fence)
{
    return fence->fence_type;
}

struct _WsbmFenceMgr *
wsbmFenceMgrCreate(const struct _WsbmFenceMgrCreateInfo *info)
{
    struct _WsbmFenceMgr *tmp;
    uint32_t i, j;
    int ret;

    tmp = calloc(1, sizeof(*tmp));
    if (!tmp)
	return NULL;

    tmp->info = *info;
    tmp->classes = calloc(tmp->info.num_classes, sizeof(*tmp->classes));
    if (!tmp->classes)
	goto out_err;

    for (i = 0; i < tmp->info.num_classes; ++i) {
	struct _WsbmFenceClass *fc = &tmp->classes[i];

	WSBMINITLISTHEAD(&fc->head);
	ret = WSBM_MUTEX_INIT(&fc->mutex);
	if (ret)
	    goto out_err1;
	ret = WSBM_MUTEX_INIT(&fc->cmd_mutex);
	if (ret) {
	    WSBM_MUTEX_FREE(&fc->mutex);
	    goto out_err1;
	}
    }
    wsbmAtomicSet(&tmp->count, 0);

    return tmp;

  out_err1:
    for (j = 0; j < i; ++j) {
	WSBM_MUTEX_FREE(&tmp->classes[j].mutex);
	WSBM_MUTEX_FREE(&tmp->classes[j].cmd_mutex);
    }
    free(tmp->classes);
  out_err:
    if (tmp)
	free(tmp);
    return NULL;
}

void
wsbmFenceUnreference(struct _WsbmFenceObject **pFence)
{
    struct _WsbmFenceObject *fence = *pFence;
    struct _WsbmFenceMgr *mgr;

    *pFence = NULL;
    if (fence == NULL)
	return;

    mgr = fence->mgr;
    if (wsbmAtomicDecZero(&fence->refCount)) {
	struct _WsbmFenceClass *fc = &mgr->classes[fence->fence_class];

	WSBM_MUTEX_LOCK(&fc->mutex);
	WSBMLISTDELINIT(&fence->head);
	WSBM_MUTEX_UNLOCK(&fc->mutex);
	if (fence->private)
	    mgr->info.unreference(mgr, &fence->private);
	fence->mgr = NULL;
	wsbmAtomicDecZero(&mgr->count);
	free(fence);
    }
}

static void
wsbmSignalPreviousFences(struct _WsbmFenceMgr *mgr,
			 struct _WsbmListHead *list,
			 uint32_t fence_class, uint32_t signaled_types)
{
    struct _WsbmFenceClass *fc = &mgr->classes[fence_class];
    struct _WsbmFenceObject *entry;
    struct _WsbmListHead *prev;
    uint32_t old_signaled_types;
    uint32_t ret_st;

    WSBM_MUTEX_LOCK(&fc->mutex);
    while (list != &fc->head && list->next != list) {
	entry = WSBMLISTENTRY(list, struct _WsbmFenceObject, head);

	prev = list->prev;

	do {
	    old_signaled_types = wsbmAtomicRead(&entry->signaled_types);
	    signaled_types =
		old_signaled_types | (signaled_types & entry->fence_type);
	    if (signaled_types == old_signaled_types)
		break;

	    ret_st =
		wsbmAtomicCmpXchg(&entry->signaled_types, old_signaled_types,
				  signaled_types);
	} while (ret_st != old_signaled_types);

	if (signaled_types == entry->fence_type)
	    WSBMLISTDELINIT(list);

	list = prev;
    }
    WSBM_MUTEX_UNLOCK(&fc->mutex);
}

int
wsbmFenceFinish(struct _WsbmFenceObject *fence, uint32_t fence_type,
		int lazy_hint)
{
    struct _WsbmFenceMgr *mgr = fence->mgr;
    int ret = 0;

    if ((wsbmAtomicRead(&fence->signaled_types) & fence_type) == fence_type)
	goto out;

    ret = mgr->info.finish(mgr, fence->private, fence_type, lazy_hint);
    if (ret)
	goto out;

    wsbmSignalPreviousFences(mgr, &fence->head, fence->fence_class,
			     fence_type);
  out:
    return ret;
}

uint32_t
wsbmFenceSignaledTypeCached(struct _WsbmFenceObject * fence)
{
    return wsbmAtomicRead(&fence->signaled_types);
}

int
wsbmFenceSignaledType(struct _WsbmFenceObject *fence, uint32_t flush_type,
		      uint32_t * signaled)
{
    int ret = 0;
    struct _WsbmFenceMgr *mgr;
    uint32_t signaled_types;
    uint32_t old_signaled_types;
    uint32_t ret_st;

    mgr = fence->mgr;
    *signaled = wsbmAtomicRead(&fence->signaled_types);
    if ((*signaled & flush_type) == flush_type)
	goto out0;

    ret = mgr->info.signaled(mgr, fence->private, flush_type, signaled);
    if (ret) {
	*signaled = wsbmAtomicRead(&fence->signaled_types);
	goto out0;
    }

    do {
	old_signaled_types = wsbmAtomicRead(&fence->signaled_types);
	signaled_types = old_signaled_types | *signaled;
	if (signaled_types == old_signaled_types)
	    break;

	ret_st = wsbmAtomicCmpXchg(&fence->signaled_types, old_signaled_types,
				   signaled_types);
	if (old_signaled_types == ret_st)
	    wsbmSignalPreviousFences(mgr, &fence->head, fence->fence_class,
				     *signaled);
    } while (old_signaled_types != ret_st);

    return 0;
  out0:
    return ret;
}

struct _WsbmFenceObject *
wsbmFenceReference(struct _WsbmFenceObject *fence)
{
    if (fence == NULL)
	return NULL;
    wsbmAtomicInc(&fence->refCount);
    return fence;
}

struct _WsbmFenceObject *
wsbmFenceCreateSig(struct _WsbmFenceMgr *mgr, uint32_t fence_class,
		   uint32_t fence_type, uint32_t signaled_types, 
		   void *private, size_t private_size)
{
    struct _WsbmFenceClass *fc = &mgr->classes[fence_class];
    struct _WsbmFenceObject *fence;
    size_t fence_size = sizeof(*fence);

    if (private_size)
	fence_size = ((fence_size + 15) & ~15);

    fence = calloc(1, fence_size + private_size);

    if (!fence)
	goto out_err;

    wsbmAtomicSet(&fence->refCount, 1);
    fence->mgr = mgr;
    fence->fence_class = fence_class;
    fence->fence_type = fence_type;
    wsbmAtomicSet(&fence->signaled_types, signaled_types);
    fence->private = private;
    if (private_size) {
	fence->private = (void *)(((uint8_t *) fence) + fence_size);
	memcpy(fence->private, private, private_size);
    }

    WSBM_MUTEX_LOCK(&fc->mutex);
    WSBMLISTADDTAIL(&fence->head, &fc->head);
    WSBM_MUTEX_UNLOCK(&fc->mutex);
    wsbmAtomicInc(&mgr->count);
    return fence;

  out_err:
    {
	int ret = mgr->info.finish(mgr, private, fence_type, 0);

	if (ret)
	    usleep(10000000);
    }
    if (fence)
	free(fence);

    mgr->info.unreference(mgr, &private);
    return NULL;
}

struct _WsbmFenceObject *
wsbmFenceCreate(struct _WsbmFenceMgr *mgr, uint32_t fence_class,
		uint32_t fence_type, void *private, size_t private_size)
{
  return wsbmFenceCreateSig(mgr, fence_class, fence_type, 0, private,
			    private_size);
}

struct _WsbmTTMFenceMgrPriv
{
    int fd;
    unsigned int devOffset;
};

static int
tSignaled(struct _WsbmFenceMgr *mgr, void *private, uint32_t flush_type,
	  uint32_t * signaled_type)
{
    struct _WsbmTTMFenceMgrPriv *priv =
	(struct _WsbmTTMFenceMgrPriv *)mgr->private;
    union ttm_fence_signaled_arg arg;
    int ret;

    arg.req.handle = (unsigned long)private;
    arg.req.fence_type = flush_type;
    arg.req.flush = 1;
    *signaled_type = 0;

    ret = drmCommandWriteRead(priv->fd, priv->devOffset + TTM_FENCE_SIGNALED,
			      &arg, sizeof(arg));
    if (ret)
	return ret;

    *signaled_type = arg.rep.signaled_types;
    return 0;
}

static int
tFinish(struct _WsbmFenceMgr *mgr, void *private, uint32_t fence_type,
	int lazy_hint)
{
    struct _WsbmTTMFenceMgrPriv *priv =
	(struct _WsbmTTMFenceMgrPriv *)mgr->private;
    union ttm_fence_finish_arg arg =
	{.req = {.handle = (unsigned long)private,
		 .fence_type = fence_type,
		 .mode = (lazy_hint) ? TTM_FENCE_FINISH_MODE_LAZY : 0}
    };
    int ret;

    do {
	ret = drmCommandWriteRead(priv->fd, priv->devOffset + TTM_FENCE_FINISH,
				  &arg, sizeof(arg));
    } while (ret == -EAGAIN || ret == -ERESTART);

    return ret;
}

static int
tUnref(struct _WsbmFenceMgr *mgr, void **private)
{
    struct _WsbmTTMFenceMgrPriv *priv =
	(struct _WsbmTTMFenceMgrPriv *)mgr->private;
    struct ttm_fence_unref_arg arg = {.handle = (unsigned long)*private };

    *private = NULL;

    return drmCommandWrite(priv->fd, priv->devOffset + TTM_FENCE_UNREF,
			   &arg, sizeof(arg));
}

struct _WsbmFenceMgr *
wsbmFenceMgrTTMInit(int fd, unsigned int numClass, unsigned int devOffset)
{
    struct _WsbmFenceMgrCreateInfo info;
    struct _WsbmFenceMgr *mgr;
    struct _WsbmTTMFenceMgrPriv *priv = malloc(sizeof(*priv));

    if (!priv)
	return NULL;

    priv->fd = fd;
    priv->devOffset = devOffset;

    info.flags = WSBM_FENCE_CLASS_ORDERED;
    info.num_classes = numClass;
    info.signaled = tSignaled;
    info.finish = tFinish;
    info.unreference = tUnref;

    mgr = wsbmFenceMgrCreate(&info);
    if (mgr == NULL) {
	free(priv);
	return NULL;
    }

    mgr->private = (void *)priv;
    return mgr;
}

void
wsbmFenceCmdLock(struct _WsbmFenceMgr *mgr, uint32_t fence_class)
{
    WSBM_MUTEX_LOCK(&mgr->classes[fence_class].cmd_mutex);
}

void
wsbmFenceCmdUnlock(struct _WsbmFenceMgr *mgr, uint32_t fence_class)
{
    WSBM_MUTEX_UNLOCK(&mgr->classes[fence_class].cmd_mutex);
}

void
wsbmFenceMgrTTMTakedown(struct _WsbmFenceMgr *mgr)
{
    unsigned int i;

    if (!mgr)
	return;

    if (mgr->private)
	free(mgr->private);

    for (i = 0; i < mgr->info.num_classes; ++i) {
	WSBM_MUTEX_FREE(&mgr->classes[i].mutex);
	WSBM_MUTEX_FREE(&mgr->classes[i].cmd_mutex);
    }
    free(mgr);

    return;
}
