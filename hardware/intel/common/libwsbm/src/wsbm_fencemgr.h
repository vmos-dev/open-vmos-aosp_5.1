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

#ifndef WSBM_FENCEMGR_H
#define WSBM_FENCEMGR_H

#include <stdint.h>
#include <stdlib.h>

struct _WsbmFenceObject;
struct _WsbmFenceMgr;

/*
 * Do a quick check to see if the fence manager has registered the fence
 * object as signaled. Note that this function may return a false negative
 * answer.
 */
extern uint32_t wsbmFenceSignaledTypeCached(struct _WsbmFenceObject *fence);

/*
 * Check if the fence object is signaled. This function can be substantially
 * more expensive to call than the above function, but will not return a false
 * negative answer. The argument "flush_type" sets the types that the
 * underlying mechanism must make sure will eventually signal.
 */
extern int wsbmFenceSignaledType(struct _WsbmFenceObject *fence,
				 uint32_t flush_type, uint32_t * signaled);

/*
 * Convenience functions.
 */

static inline int
wsbmFenceSignaled(struct _WsbmFenceObject *fence, uint32_t flush_type)
{
    uint32_t signaled_types;
    int ret = wsbmFenceSignaledType(fence, flush_type, &signaled_types);

    if (ret)
	return 0;
    return ((signaled_types & flush_type) == flush_type);
}

static inline int
wsbmFenceSignaledCached(struct _WsbmFenceObject *fence, uint32_t flush_type)
{
    uint32_t signaled_types = wsbmFenceSignaledTypeCached(fence);

    return ((signaled_types & flush_type) == flush_type);
}

/*
 * Reference a fence object.
 */
extern struct _WsbmFenceObject *wsbmFenceReference(struct _WsbmFenceObject
						   *fence);

/*
 * Unreference a fence object. The fence object pointer will be reset to NULL.
 */

extern void wsbmFenceUnreference(struct _WsbmFenceObject **pFence);

/*
 * Wait for a fence to signal the indicated fence_type.
 * If "lazy_hint" is true, it indicates that the wait may sleep to avoid
 * busy-wait polling.
 */
extern int wsbmFenceFinish(struct _WsbmFenceObject *fence,
			   uint32_t fence_type, int lazy_hint);

/*
 * Create a WsbmFenceObject for manager "mgr".
 *
 * "private" is a pointer that should be used for the callbacks in
 * struct _WsbmFenceMgrCreateInfo.
 *
 * if private_size is nonzero, then the info stored at *private, with size
 * private size will be copied and the fence manager will instead use a
 * pointer to the copied data for the callbacks in
 * struct _WsbmFenceMgrCreateInfo. In that case, the object pointed to by
 * "private" may be destroyed after the call to wsbmFenceCreate.
 */
extern struct _WsbmFenceObject *wsbmFenceCreate(struct _WsbmFenceMgr *mgr,
						uint32_t fence_class,
						uint32_t fence_type,
						void *private,
						size_t private_size);


extern struct _WsbmFenceObject *wsbmFenceCreateSig(struct _WsbmFenceMgr *mgr,
						   uint32_t fence_class,
						   uint32_t fence_type,
						   uint32_t signaled_types,
						   void *private,
						   size_t private_size);

extern uint32_t wsbmFenceType(struct _WsbmFenceObject *fence);

/*
 * Fence creations are ordered. If a fence signals a fence_type,
 * it is safe to assume that all fences of the same class that was
 * created before that fence has signaled the same type.
 */

#define WSBM_FENCE_CLASS_ORDERED (1 << 0)

struct _WsbmFenceMgrCreateInfo
{
    uint32_t flags;
    uint32_t num_classes;
    int (*signaled) (struct _WsbmFenceMgr * mgr, void *private,
		     uint32_t flush_type, uint32_t * signaled_type);
    int (*finish) (struct _WsbmFenceMgr * mgr, void *private,
		   uint32_t fence_type, int lazy_hint);
    int (*unreference) (struct _WsbmFenceMgr * mgr, void **private);
};

extern struct _WsbmFenceMgr *wsbmFenceMgrCreate(const struct
						_WsbmFenceMgrCreateInfo
						*info);
extern void wsbmFenceCmdLock(struct _WsbmFenceMgr *mgr, uint32_t fence_class);
extern void wsbmFenceCmdUnlock(struct _WsbmFenceMgr *mgr,
			       uint32_t fence_class);
/*
 * Builtin drivers.
 */

extern struct _WsbmFenceMgr *wsbmFenceMgrTTMInit(int fd,
						 unsigned int numClass,
						 unsigned int devOffset);
extern void wsbmFenceMgrTTMTakedown(struct _WsbmFenceMgr *mgr);
#endif
