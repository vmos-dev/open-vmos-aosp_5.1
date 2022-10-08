/**************************************************************************
 *
 * Copyright 2006-2008 Tungsten Graphics, Inc., Cedar Park, TX., USA
 * All Rights Reserved.
 * Copyright 2009 Vmware, Inc., Palo Alto, CA., USA
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
 *          Keith Whitwell <keithw-at-tungstengraphics-dot-com>
 */

#ifndef _WSBM_MANAGER_H_
#define _WSBM_MANAGER_H_
#include "wsbm_fencemgr.h"
#include "wsbm_util.h"
#include "wsbm_driver.h"

#define WSBM_VERSION_MAJOR 1
#define WSBM_VERSION_MINOR 1
#define WSBM_VERSION_PL 0

struct _WsbmFenceObject;
struct _WsbmBufferObject;
struct _WsbmBufferPool;
struct _WsbmBufferList;

/*
 * These flags mimics the TTM closely, but since
 * this library is not dependant on TTM, we need to
 * replicate them here, and if there is a discrepancy,
 * that needs to be resolved in the buffer pool using
 * the TTM flags.
 */

#define WSBM_PL_MASK_MEM         0x0000FFFF

#define WSBM_PL_FLAG_SYSTEM      (1 << 0)
#define WSBM_PL_FLAG_TT          (1 << 1)
#define WSBM_PL_FLAG_VRAM        (1 << 2)
#define WSBM_PL_FLAG_PRIV0       (1 << 3)
#define WSBM_PL_FLAG_SWAPPED     (1 << 15)
#define WSBM_PL_FLAG_CACHED      (1 << 16)
#define WSBM_PL_FLAG_UNCACHED    (1 << 17)
#define WSBM_PL_FLAG_WC          (1 << 18)
#define WSBM_PL_FLAG_SHARED      (1 << 20)
#define WSBM_PL_FLAG_NO_EVICT    (1 << 21)

#define WSBM_ACCESS_READ         (1 << 0)
#define WSBM_ACCESS_WRITE        (1 << 1)

#define WSBM_SYNCCPU_READ        WSBM_ACCESS_READ
#define WSBM_SYNCCPU_WRITE       WSBM_ACCESS_WRITE
#define WSBM_SYNCCPU_DONT_BLOCK  (1 << 2)
#define WSBM_SYNCCPU_TRY_CACHED  (1 << 3)

extern void *wsbmBOMap(struct _WsbmBufferObject *buf, unsigned mode);
extern void wsbmBOUnmap(struct _WsbmBufferObject *buf);
extern int wsbmBOSyncForCpu(struct _WsbmBufferObject *buf, unsigned mode);
extern void wsbmBOReleaseFromCpu(struct _WsbmBufferObject *buf,
				 unsigned mode);

extern unsigned long wsbmBOOffsetHint(struct _WsbmBufferObject *buf);
extern unsigned long wsbmBOPoolOffset(struct _WsbmBufferObject *buf);

extern uint32_t wsbmBOPlacementHint(struct _WsbmBufferObject *buf);
extern struct _WsbmBufferObject *wsbmBOReference(struct _WsbmBufferObject
						 *buf);
extern void wsbmBOUnreference(struct _WsbmBufferObject **p_buf);

extern int wsbmBOData(struct _WsbmBufferObject *r_buf,
		      unsigned size, const void *data,
		      struct _WsbmBufferPool *pool, uint32_t placement);

extern int wsbmBODataUB(struct _WsbmBufferObject *buf,
            unsigned size, const void *data, struct _WsbmBufferPool *newPool,
            uint32_t placement, const unsigned long *user_ptr);

extern int wsbmBOSetStatus(struct _WsbmBufferObject *buf,
			   uint32_t setPlacement, uint32_t clrPlacement);
extern int wsbmBOSubData(struct _WsbmBufferObject *buf,
			 unsigned long offset, unsigned long size,
			 const void *data,
			 int (*accelCopy) (struct _WsbmBufferObject *,
					   struct _WsbmBufferObject *));
extern struct _WsbmBufferObject *wsbmBOClone(struct _WsbmBufferObject *buf,
					     int (*accelCopy) (struct
							       _WsbmBufferObject
							       *,
							       struct
							       _WsbmBufferObject
							       *));

extern int wsbmBOGetSubData(struct _WsbmBufferObject *buf,
			    unsigned long offset, unsigned long size,
			    void *data);
extern int wsbmGenBuffers(struct _WsbmBufferPool *pool,
			  unsigned n,
			  struct _WsbmBufferObject *buffers[],
			  unsigned alignment, uint32_t placement);

struct _WsbmBufferObject *wsbmBOCreateSimple(struct _WsbmBufferPool *pool,
					     unsigned long size,
					     uint32_t placement,
					     unsigned alignment,
					     size_t extra_size,
					     size_t * offset);

extern void wsbmDeleteBuffers(unsigned n,
			      struct _WsbmBufferObject *buffers[]);
extern struct _WsbmBufferList *wsbmBOCreateList(int target,
						int hasKernelBuffers);
extern int wsbmBOResetList(struct _WsbmBufferList *list);
extern int wsbmBOAddListItem(struct _WsbmBufferList *list,
			     struct _WsbmBufferObject *buf,
			     uint64_t flags, uint64_t mask, int *itemLoc,
			     struct _ValidateNode **node);

extern void wsbmBOFreeList(struct _WsbmBufferList *list);
extern int wsbmBOFenceUserList(struct _WsbmBufferList *list,
			       struct _WsbmFenceObject *fence);

extern int wsbmBOUnrefUserList(struct _WsbmBufferList *list);
extern int wsbmBOValidateUserList(struct _WsbmBufferList *list);
extern int wsbmBOUnvalidateUserList(struct _WsbmBufferList *list);

extern void wsbmBOFence(struct _WsbmBufferObject *buf,
			struct _WsbmFenceObject *fence);

extern void wsbmPoolTakeDown(struct _WsbmBufferPool *pool);
extern int wsbmBOSetReferenced(struct _WsbmBufferObject *buf,
			       unsigned long handle);
unsigned long wsbmBOSize(struct _WsbmBufferObject *buf);
extern void wsbmBOWaitIdle(struct _WsbmBufferObject *buf, int lazy);
extern int wsbmBOOnList(const struct _WsbmBufferObject *buf);

extern void wsbmPoolTakeDown(struct _WsbmBufferPool *pool);

extern void wsbmReadLockKernelBO(void);
extern void wsbmReadUnlockKernelBO(void);
extern void wsbmWriteLockKernelBO(void);
extern void wsbmWriteUnlockKernelBO(void);

extern int wsbmInit(struct _WsbmThreadFuncs *tf, struct _WsbmVNodeFuncs *vf);
extern void wsbmTakedown(void);
extern int wsbmIsInitialized(void);
extern void wsbmCommonDataSet(void *d);
extern void *wsbmCommonDataGet(void);

extern struct _ValidateList *wsbmGetKernelValidateList(struct _WsbmBufferList
						       *list);
extern struct _ValidateList *wsbmGetUserValidateList(struct _WsbmBufferList
						     *list);

extern struct _ValidateNode *validateListNode(void *iterator);
extern void *validateListIterator(struct _ValidateList *list);
extern void *validateListNext(struct _ValidateList *list, void *iterator);

extern uint32_t wsbmKBufHandle(const struct _WsbmKernelBuf *);
extern void wsbmUpdateKBuf(struct _WsbmKernelBuf *,
			   uint64_t gpuOffset,
			   uint32_t placement, uint32_t fence_flags);

extern struct _WsbmKernelBuf *wsbmKBuf(const struct _WsbmBufferObject *buf);

#endif
