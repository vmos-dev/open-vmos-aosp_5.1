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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include "errno.h"
#include "string.h"
#include "wsbm_pool.h"
#include "wsbm_manager.h"
#include "wsbm_fencemgr.h"
#include "wsbm_driver.h"
#include "wsbm_priv.h"
#include "wsbm_util.h"
#include "wsbm_atomic.h"
#include "assert.h"

#define WSBM_BODATA_SIZE_ACCEPT 4096

#define WSBM_BUFFER_COMPLEX 0
#define WSBM_BUFFER_SIMPLE  1
#define WSBM_BUFFER_REF     2

struct _ValidateList
{
    unsigned numTarget;
    unsigned numCurrent;
    unsigned numOnList;
    unsigned hashSize;
    uint32_t hashMask;
    int driverData;
    struct _WsbmListHead list;
    struct _WsbmListHead free;
    struct _WsbmListHead *hashTable;
};

struct _WsbmBufferObject
{
    /* Left to the client to protect this data for now. */

    struct _WsbmAtomic refCount;
    struct _WsbmBufStorage *storage;

    uint32_t placement;
    unsigned alignment;
    unsigned bufferType;
    struct _WsbmBufferPool *pool;
};

struct _WsbmBufferList
{
    int hasKernelBuffers;

    struct _ValidateList kernelBuffers;	/* List of kernel buffers needing validation */
    struct _ValidateList userBuffers;  /* List of user-space buffers needing validation */
};

static struct _WsbmMutex bmMutex;
static struct _WsbmCond bmCond;
static int initialized = 0;
static void *commonData = NULL;

static int kernelReaders = 0;
static int kernelLocked = 0;

int
wsbmInit(struct _WsbmThreadFuncs *tf, struct _WsbmVNodeFuncs *vf)
{
    int ret;

    wsbmCurThreadFunc = tf;
    wsbmCurVNodeFunc = vf;

    ret = WSBM_MUTEX_INIT(&bmMutex);
    if (ret)
	return -ENOMEM;
    ret = WSBM_COND_INIT(&bmCond);
    if (ret) {
	WSBM_MUTEX_FREE(&bmMutex);
	return -ENOMEM;
    }

    initialized = 1;
    return 0;
}

void
wsbmCommonDataSet(void *d)
{
    commonData = d;
}

void *
wsbmCommonDataGet(void)
{
    return commonData;
}

int
wsbmIsInitialized(void)
{
    return initialized;
}

void
wsbmTakedown(void)
{
    initialized = 0;
    commonData = NULL;
    WSBM_COND_FREE(&bmCond);
    WSBM_MUTEX_FREE(&bmMutex);
}

static struct _ValidateNode *
validateListAddNode(struct _ValidateList *list, void *item,
		    uint32_t hash, uint64_t flags, uint64_t mask)
{
    struct _ValidateNode *node;
    struct _WsbmListHead *l;
    struct _WsbmListHead *hashHead;

    l = list->free.next;
    if (l == &list->free) {
	node = wsbmVNodeFuncs()->alloc(wsbmVNodeFuncs(), 0);
	if (!node) {
	    return NULL;
	}
	list->numCurrent++;
    } else {
	WSBMLISTDEL(l);
	node = WSBMLISTENTRY(l, struct _ValidateNode, head);
    }
    node->buf = item;
    node->set_flags = flags & mask;
    node->clr_flags = (~flags) & mask;
    node->listItem = list->numOnList;
    WSBMLISTADDTAIL(&node->head, &list->list);
    list->numOnList++;
    hashHead = list->hashTable + hash;
    WSBMLISTADDTAIL(&node->hashHead, hashHead);

    return node;
}

static uint32_t
wsbmHashFunc(uint8_t * key, uint32_t len, uint32_t mask)
{
    uint32_t hash, i;

    for (hash = 0, i = 0; i < len; ++i) {
	hash += *key++;
	hash += (hash << 10);
	hash ^= (hash >> 6);
    }

    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    return hash & mask;
}

static void
validateFreeList(struct _ValidateList *list)
{
    struct _ValidateNode *node;
    struct _WsbmListHead *l;

    l = list->list.next;
    while (l != &list->list) {
	WSBMLISTDEL(l);
	node = WSBMLISTENTRY(l, struct _ValidateNode, head);

	WSBMLISTDEL(&node->hashHead);
	node->func->free(node);
	l = list->list.next;
	list->numCurrent--;
	list->numOnList--;
    }

    l = list->free.next;
    while (l != &list->free) {
	WSBMLISTDEL(l);
	node = WSBMLISTENTRY(l, struct _ValidateNode, head);

	node->func->free(node);
	l = list->free.next;
	list->numCurrent--;
    }
    free(list->hashTable);
}

static int
validateListAdjustNodes(struct _ValidateList *list)
{
    struct _ValidateNode *node;
    struct _WsbmListHead *l;
    int ret = 0;

    while (list->numCurrent < list->numTarget) {
	node = wsbmVNodeFuncs()->alloc(wsbmVNodeFuncs(), list->driverData);
	if (!node) {
	    ret = -ENOMEM;
	    break;
	}
	list->numCurrent++;
	WSBMLISTADD(&node->head, &list->free);
    }

    while (list->numCurrent > list->numTarget) {
	l = list->free.next;
	if (l == &list->free)
	    break;
	WSBMLISTDEL(l);
	node = WSBMLISTENTRY(l, struct _ValidateNode, head);

	node->func->free(node);
	list->numCurrent--;
    }
    return ret;
}

static inline int
wsbmPot(unsigned int val)
{
    unsigned int shift = 0;
    while(val > (unsigned int)(1 << shift))
	shift++;

    return shift;
}



static int
validateCreateList(int numTarget, struct _ValidateList *list, int driverData)
{
    unsigned int i;
    unsigned int shift = wsbmPot(numTarget);
    int ret;

    list->hashSize = (1 << shift);
    list->hashMask = list->hashSize - 1;

    list->hashTable = malloc(list->hashSize * sizeof(*list->hashTable));
    if (!list->hashTable)
	return -ENOMEM;

    for (i = 0; i < list->hashSize; ++i)
	WSBMINITLISTHEAD(&list->hashTable[i]);

    WSBMINITLISTHEAD(&list->list);
    WSBMINITLISTHEAD(&list->free);
    list->numTarget = numTarget;
    list->numCurrent = 0;
    list->numOnList = 0;
    list->driverData = driverData;
    ret = validateListAdjustNodes(list);
    if (ret != 0)
	free(list->hashTable);

    return ret;
}

static int
validateResetList(struct _ValidateList *list)
{
    struct _WsbmListHead *l;
    struct _ValidateNode *node;
    int ret;

    ret = validateListAdjustNodes(list);
    if (ret)
	return ret;

    l = list->list.next;
    while (l != &list->list) {
	WSBMLISTDEL(l);
	node = WSBMLISTENTRY(l, struct _ValidateNode, head);

	WSBMLISTDEL(&node->hashHead);
	WSBMLISTADD(l, &list->free);
	list->numOnList--;
	l = list->list.next;
    }
    return validateListAdjustNodes(list);
}

void
wsbmWriteLockKernelBO(void)
{
    WSBM_MUTEX_LOCK(&bmMutex);
    while (kernelReaders != 0)
	WSBM_COND_WAIT(&bmCond, &bmMutex);
    kernelLocked = 1;
}

void
wsbmWriteUnlockKernelBO(void)
{
    kernelLocked = 0;
    WSBM_MUTEX_UNLOCK(&bmMutex);
}

void
wsbmReadLockKernelBO(void)
{
    WSBM_MUTEX_LOCK(&bmMutex);
    if (kernelReaders++ == 0)
	kernelLocked = 1;
    WSBM_MUTEX_UNLOCK(&bmMutex);
}

void
wsbmReadUnlockKernelBO(void)
{
    WSBM_MUTEX_LOCK(&bmMutex);
    if (--kernelReaders == 0) {
	kernelLocked = 0;
	WSBM_COND_BROADCAST(&bmCond);
    }
    WSBM_MUTEX_UNLOCK(&bmMutex);
}

void
wsbmBOWaitIdle(struct _WsbmBufferObject *buf, int lazy)
{
    struct _WsbmBufStorage *storage;

    storage = buf->storage;
    if (!storage)
	return;

    (void)storage->pool->waitIdle(storage, lazy);
}

void *
wsbmBOMap(struct _WsbmBufferObject *buf, unsigned mode)
{
    struct _WsbmBufStorage *storage = buf->storage;
    void *virtual;
    int retval;

    retval = storage->pool->map(storage, mode, &virtual);

    return (retval == 0) ? virtual : NULL;
}

void
wsbmBOUnmap(struct _WsbmBufferObject *buf)
{
    struct _WsbmBufStorage *storage = buf->storage;

    if (!storage)
	return;

    storage->pool->unmap(storage);
}

int
wsbmBOSyncForCpu(struct _WsbmBufferObject *buf, unsigned mode)
{
    struct _WsbmBufStorage *storage = buf->storage;

    return storage->pool->syncforcpu(storage, mode);
}

void
wsbmBOReleaseFromCpu(struct _WsbmBufferObject *buf, unsigned mode)
{
    struct _WsbmBufStorage *storage = buf->storage;

    storage->pool->releasefromcpu(storage, mode);
}

unsigned long
wsbmBOOffsetHint(struct _WsbmBufferObject *buf)
{
    struct _WsbmBufStorage *storage = buf->storage;

    return storage->pool->offset(storage);
}

unsigned long
wsbmBOPoolOffset(struct _WsbmBufferObject *buf)
{
    struct _WsbmBufStorage *storage = buf->storage;

    return storage->pool->poolOffset(storage);
}

uint32_t
wsbmBOPlacementHint(struct _WsbmBufferObject * buf)
{
    struct _WsbmBufStorage *storage = buf->storage;

    assert(buf->storage != NULL);

    return storage->pool->placement(storage);
}

struct _WsbmBufferObject *
wsbmBOReference(struct _WsbmBufferObject *buf)
{
    if (buf->bufferType == WSBM_BUFFER_SIMPLE) {
	wsbmAtomicInc(&buf->storage->refCount);
    } else {
	wsbmAtomicInc(&buf->refCount);
    }
    return buf;
}

int
wsbmBOSetStatus(struct _WsbmBufferObject *buf,
		uint32_t setFlags, uint32_t clrFlags)
{
    struct _WsbmBufStorage *storage = buf->storage;

    if (!storage)
	return 0;

    if (storage->pool->setStatus == NULL)
	return -EINVAL;

    return storage->pool->setStatus(storage, setFlags, clrFlags);
}

void
wsbmBOUnreference(struct _WsbmBufferObject **p_buf)
{
    struct _WsbmBufferObject *buf = *p_buf;

    *p_buf = NULL;

    if (!buf)
	return;

    if (buf->bufferType == WSBM_BUFFER_SIMPLE) {
	struct _WsbmBufStorage *dummy = buf->storage;

	wsbmBufStorageUnref(&dummy);
	return;
    }

    if (wsbmAtomicDecZero(&buf->refCount)) {
	wsbmBufStorageUnref(&buf->storage);
	free(buf);
    }
}

int
wsbmBOData(struct _WsbmBufferObject *buf,
	   unsigned size, const void *data,
	   struct _WsbmBufferPool *newPool, uint32_t placement)
{
    void *virtual = NULL;
    int newBuffer;
    int retval = 0;
    struct _WsbmBufStorage *storage;
    int synced = 0;
    uint32_t placement_diff;
    struct _WsbmBufferPool *curPool;

    if (buf->bufferType == WSBM_BUFFER_SIMPLE)
	return -EINVAL;

    storage = buf->storage;

    if (newPool == NULL)
	newPool = buf->pool;

    if (newPool == NULL)
	return -EINVAL;

    newBuffer = (!storage || storage->pool != newPool ||
		 storage->pool->size(storage) < size ||
		 storage->pool->size(storage) >
		 size + WSBM_BODATA_SIZE_ACCEPT);

    if (!placement)
	placement = buf->placement;

    if (newBuffer) {
	if (buf->bufferType == WSBM_BUFFER_REF)
	    return -EINVAL;

	wsbmBufStorageUnref(&buf->storage);

	if (size == 0) {
	    buf->pool = newPool;
	    buf->placement = placement;
	    retval = 0;
	    goto out;
	}

	buf->storage =
	    newPool->create(newPool, size, placement, buf->alignment);
	if (!buf->storage) {
	    retval = -ENOMEM;
	    goto out;
	}

	buf->placement = placement;
	buf->pool = newPool;
    } else if (wsbmAtomicRead(&storage->onList) ||
	       0 != storage->pool->syncforcpu(storage, WSBM_SYNCCPU_WRITE |
					      WSBM_SYNCCPU_DONT_BLOCK)) {
	/*
	 * Buffer is busy. need to create a new one.
	 */

	struct _WsbmBufStorage *tmp_storage;

	curPool = storage->pool;

	tmp_storage =
	    curPool->create(curPool, size, placement, buf->alignment);

	if (tmp_storage) {
	    wsbmBufStorageUnref(&buf->storage);
	    buf->storage = tmp_storage;
	    buf->placement = placement;
	} else {
	    retval = curPool->syncforcpu(storage, WSBM_SYNCCPU_WRITE);
	    if (retval)
		goto out;
	    synced = 1;
	}
    } else
	synced = 1;

    placement_diff = placement ^ buf->placement;

    /*
     * We might need to change buffer placement.
     */

    storage = buf->storage;
    curPool = storage->pool;

    if (placement_diff) {
	assert(curPool->setStatus != NULL);
	curPool->releasefromcpu(storage, WSBM_SYNCCPU_WRITE);
	retval = curPool->setStatus(storage,
				    placement_diff & placement,
				    placement_diff & ~placement);
	if (retval)
	    goto out;

	buf->placement = placement;

    }

    if (!synced) {
	retval = curPool->syncforcpu(buf->storage, WSBM_SYNCCPU_WRITE);

	if (retval)
	    goto out;
	synced = 1;
    }

    storage = buf->storage;
    curPool = storage->pool;

    if (data) {
	retval = curPool->map(storage, WSBM_ACCESS_WRITE, &virtual);
	if (retval)
	    goto out;
	memcpy(virtual, data, size);
	curPool->unmap(storage);
    }

  out:

    if (synced)
	curPool->releasefromcpu(storage, WSBM_SYNCCPU_WRITE);

    return retval;
}

int
wsbmBODataUB(struct _WsbmBufferObject *buf,
        unsigned size, const void *data, struct _WsbmBufferPool *newPool,
        uint32_t placement, const unsigned long *user_ptr)
{
    int newBuffer;
    int retval = 0;
    struct _WsbmBufStorage *storage;
    int synced = 0;
    uint32_t placement_diff;
    struct _WsbmBufferPool *curPool;
    extern struct _WsbmBufStorage *
    ttm_pool_ub_create(struct _WsbmBufferPool *pool,
        unsigned long size, uint32_t placement, unsigned alignment,
        const unsigned long *user_ptr);

    if (buf->bufferType == WSBM_BUFFER_SIMPLE)
        return -EINVAL;

    storage = buf->storage;

    if (newPool == NULL)
        newPool = buf->pool;

    if (newPool == NULL)
        return -EINVAL;

    newBuffer = (!storage || storage->pool != newPool ||
        storage->pool->size(storage) < size ||
        storage->pool->size(storage) >
        size + WSBM_BODATA_SIZE_ACCEPT);

    if (!placement)
        placement = buf->placement;

    if (newBuffer) {
        if (buf->bufferType == WSBM_BUFFER_REF)
            return -EINVAL;

        wsbmBufStorageUnref(&buf->storage);

        if (size == 0) {
            buf->pool = newPool;
            buf->placement = placement;
            retval = 0;
            goto out;
        }

        buf->storage =
            ttm_pool_ub_create(newPool, size, placement, buf->alignment, user_ptr);
        if (!buf->storage) {
            retval = -ENOMEM;
            goto out;
        }

        buf->placement = placement;
        buf->pool = newPool;
    } else if (wsbmAtomicRead(&storage->onList) ||
        0 != storage->pool->syncforcpu(storage, WSBM_SYNCCPU_WRITE |
        WSBM_SYNCCPU_DONT_BLOCK)) {
        /*
        * Buffer is busy. need to create a new one.
        * Actually such case will not be encountered for current ICS implementation
        * TODO: maybe need refine the following code when such usage case is required
        */

        struct _WsbmBufStorage *tmp_storage;

        curPool = storage->pool;

        tmp_storage =
            ttm_pool_ub_create(curPool, size, placement, buf->alignment, user_ptr);

        if (tmp_storage) {
            wsbmBufStorageUnref(&buf->storage);
            buf->storage = tmp_storage;
            buf->placement = placement;
        } else {
            retval = curPool->syncforcpu(storage, WSBM_SYNCCPU_WRITE);
            if (retval)
                goto out;
            synced = 1;
        }
    } else {
        synced = 1;
    }

    placement_diff = placement ^ buf->placement;

    /*
    * We might need to change buffer placement.
    */

    storage = buf->storage;
    curPool = storage->pool;

    if (placement_diff) {
        assert(curPool->setStatus != NULL);
        curPool->releasefromcpu(storage, WSBM_SYNCCPU_WRITE);
        retval = curPool->setStatus(storage,
            placement_diff & placement,
            placement_diff & ~placement);
        if (retval)
            goto out;

        buf->placement = placement;
    }

    if (!synced) {
        retval = curPool->syncforcpu(buf->storage, WSBM_SYNCCPU_WRITE);
        if (retval)
            goto out;
        synced = 1;
    }

    storage = buf->storage;
    curPool = storage->pool;

    if (data) {
        memcpy((unsigned long *) user_ptr, data, size);
    }

    out:

    if (synced)
        curPool->releasefromcpu(storage, WSBM_SYNCCPU_WRITE);

    return retval;
}

static struct _WsbmBufStorage *
wsbmStorageClone(struct _WsbmBufferObject *buf)
{
    struct _WsbmBufStorage *storage = buf->storage;
    struct _WsbmBufferPool *pool = storage->pool;

    return pool->create(pool, pool->size(storage), buf->placement,
			buf->alignment);
}

struct _WsbmBufferObject *
wsbmBOClone(struct _WsbmBufferObject *buf,
	    int (*accelCopy) (struct _WsbmBufferObject *,
			      struct _WsbmBufferObject *))
{
    struct _WsbmBufferObject *newBuf;
    int ret;

    newBuf = malloc(sizeof(*newBuf));
    if (!newBuf)
	return NULL;

    *newBuf = *buf;
    newBuf->storage = wsbmStorageClone(buf);
    if (!newBuf->storage)
	goto out_err0;

    wsbmAtomicSet(&newBuf->refCount, 1);
    if (!accelCopy || accelCopy(newBuf, buf) != 0) {

	struct _WsbmBufferPool *pool = buf->storage->pool;
	struct _WsbmBufStorage *storage = buf->storage;
	struct _WsbmBufStorage *newStorage = newBuf->storage;
	void *virtual;
	void *nVirtual;

	ret = pool->syncforcpu(storage, WSBM_SYNCCPU_READ);
	if (ret)
	    goto out_err1;
	ret = pool->map(storage, WSBM_ACCESS_READ, &virtual);
	if (ret)
	    goto out_err2;
	ret = pool->map(newStorage, WSBM_ACCESS_WRITE, &nVirtual);
	if (ret)
	    goto out_err3;

	memcpy(nVirtual, virtual, pool->size(storage));
	pool->unmap(newBuf->storage);
	pool->unmap(buf->storage);
	pool->releasefromcpu(storage, WSBM_SYNCCPU_READ);
    }

    return newBuf;
  out_err3:
    buf->pool->unmap(buf->storage);
  out_err2:
    buf->pool->releasefromcpu(buf->storage, WSBM_SYNCCPU_READ);
  out_err1:
    wsbmBufStorageUnref(&newBuf->storage);
  out_err0:
    free(newBuf);
    return 0;
}

int
wsbmBOSubData(struct _WsbmBufferObject *buf,
	      unsigned long offset, unsigned long size, const void *data,
	      int (*accelCopy) (struct _WsbmBufferObject *,
				struct _WsbmBufferObject *))
{
    int ret = 0;

    if (buf->bufferType == WSBM_BUFFER_SIMPLE)
	return -EINVAL;

    if (size && data) {
	void *virtual;
	struct _WsbmBufStorage *storage = buf->storage;
	struct _WsbmBufferPool *pool = storage->pool;

	ret = pool->syncforcpu(storage, WSBM_SYNCCPU_WRITE);
	if (ret)
	    goto out;

	if (wsbmAtomicRead(&storage->onList)) {

	    struct _WsbmBufferObject *newBuf;

	    /*
	     * Another context has this buffer on its validate list.
	     * This should be a very rare situation, but it can be valid,
	     * and therefore we must deal with it by cloning the storage.
	     */

	    pool->releasefromcpu(storage, WSBM_SYNCCPU_WRITE);
	    newBuf = wsbmBOClone(buf, accelCopy);

	    /*
	     * If clone fails we have the choice of either bailing.
	     * (The other context will be happy), or go on and update
	     * the old buffer anyway. (We will be happy). We choose the
	     * latter.
	     */

	    if (newBuf) {
		storage = newBuf->storage;
		wsbmAtomicInc(&storage->refCount);
		wsbmBufStorageUnref(&buf->storage);
		buf->storage = storage;
		wsbmBOUnreference(&newBuf);
		pool = storage->pool;
	    }

	    ret = pool->syncforcpu(storage, WSBM_SYNCCPU_WRITE);
	    if (ret)
		goto out;
	}

	ret = pool->map(storage, WSBM_ACCESS_WRITE, &virtual);
	if (ret) {
	    pool->releasefromcpu(storage, WSBM_SYNCCPU_WRITE);
	    goto out;
	}

	memcpy((unsigned char *)virtual + offset, data, size);
	pool->unmap(storage);
	pool->releasefromcpu(storage, WSBM_SYNCCPU_WRITE);
    }
  out:
    return ret;
}

int
wsbmBOGetSubData(struct _WsbmBufferObject *buf,
		 unsigned long offset, unsigned long size, void *data)
{
    int ret = 0;

    if (size && data) {
	void *virtual;
	struct _WsbmBufStorage *storage = buf->storage;
	struct _WsbmBufferPool *pool = storage->pool;

	ret = pool->syncforcpu(storage, WSBM_SYNCCPU_READ);
	if (ret)
	    goto out;
	ret = pool->map(storage, WSBM_ACCESS_READ, &virtual);
	if (ret) {
	    pool->releasefromcpu(storage, WSBM_SYNCCPU_WRITE);
	    goto out;
	}
	memcpy(data, (unsigned char *)virtual + offset, size);
	pool->unmap(storage);
	pool->releasefromcpu(storage, WSBM_SYNCCPU_WRITE);
    }
  out:
    return ret;
}

int
wsbmBOSetReferenced(struct _WsbmBufferObject *buf, unsigned long handle)
{
    int ret = 0;

    wsbmBufStorageUnref(&buf->storage);
    if (buf->pool->createByReference == NULL) {
	ret = -EINVAL;
	goto out;
    }
    buf->storage = buf->pool->createByReference(buf->pool, handle);
    if (!buf->storage) {
	ret = -EINVAL;
	goto out;
    }
    buf->bufferType = WSBM_BUFFER_REF;
  out:
    return ret;
}

void
wsbmBOFreeSimple(void *ptr)
{
    free(ptr);
}

struct _WsbmBufferObject *
wsbmBOCreateSimple(struct _WsbmBufferPool *pool,
		   unsigned long size,
		   uint32_t placement,
		   unsigned alignment, size_t extra_size, size_t * offset)
{
    struct _WsbmBufferObject *buf;
    struct _WsbmBufStorage *storage;

    *offset = (sizeof(*buf) + 15) & ~15;

    if (extra_size) {
	extra_size += *offset - sizeof(*buf);
    }

    buf = (struct _WsbmBufferObject *)calloc(1, sizeof(*buf) + extra_size);
    if (!buf)
	return NULL;

    storage = pool->create(pool, size, placement, alignment);
    if (!storage)
	goto out_err0;

    storage->destroyContainer = &wsbmBOFreeSimple;
    storage->destroyArg = buf;

    buf->storage = storage;
    buf->alignment = alignment;
    buf->pool = pool;
    buf->placement = placement;
    buf->bufferType = WSBM_BUFFER_SIMPLE;

    return buf;

  out_err0:
    free(buf);
    return NULL;
}

int
wsbmGenBuffers(struct _WsbmBufferPool *pool,
	       unsigned n,
	       struct _WsbmBufferObject *buffers[],
	       unsigned alignment, uint32_t placement)
{
    struct _WsbmBufferObject *buf;
    unsigned int i;

    placement = (placement) ? placement :
	WSBM_PL_FLAG_SYSTEM | WSBM_PL_FLAG_CACHED;

    for (i = 0; i < n; ++i) {
	buf = (struct _WsbmBufferObject *)calloc(1, sizeof(*buf));
	if (!buf)
	    return -ENOMEM;

	wsbmAtomicSet(&buf->refCount, 1);
	buf->placement = placement;
	buf->alignment = alignment;
	buf->pool = pool;
	buf->bufferType = WSBM_BUFFER_COMPLEX;
	buffers[i] = buf;
    }
    return 0;
}

void
wsbmDeleteBuffers(unsigned n, struct _WsbmBufferObject *buffers[])
{
    unsigned int i;

    for (i = 0; i < n; ++i) {
	wsbmBOUnreference(&buffers[i]);
    }
}

/*
 * Note that lists are per-context and don't need mutex protection.
 */

struct _WsbmBufferList *
wsbmBOCreateList(int target, int hasKernelBuffers)
{
    struct _WsbmBufferList *list = calloc(sizeof(*list), 1);
    int ret;

    if (!list)
        return NULL;
    list->hasKernelBuffers = hasKernelBuffers;
    if (hasKernelBuffers) {
	ret = validateCreateList(target, &list->kernelBuffers, 0);
	if (ret)
	    return NULL;
    }

    ret = validateCreateList(target, &list->userBuffers, 1);
    if (ret) {
	validateFreeList(&list->kernelBuffers);
	return NULL;
    }

    return list;
}

int
wsbmBOResetList(struct _WsbmBufferList *list)
{
    int ret;

    if (list->hasKernelBuffers) {
	ret = validateResetList(&list->kernelBuffers);
	if (ret)
	    return ret;
    }
    ret = validateResetList(&list->userBuffers);
    return ret;
}

void
wsbmBOFreeList(struct _WsbmBufferList *list)
{
    if (list->hasKernelBuffers)
	validateFreeList(&list->kernelBuffers);
    validateFreeList(&list->userBuffers);
    free(list);
}

static int
wsbmAddValidateItem(struct _ValidateList *list, void *buf, uint64_t flags,
		    uint64_t mask, int *itemLoc,
		    struct _ValidateNode **pnode, int *newItem)
{
    struct _ValidateNode *node, *cur;
    struct _WsbmListHead *l;
    struct _WsbmListHead *hashHead;
    uint32_t hash;
    uint32_t count = 0;
    uint32_t key = (unsigned long) buf;

    cur = NULL;
    hash = wsbmHashFunc((uint8_t *) &key, 4, list->hashMask);
    hashHead = list->hashTable + hash;
    *newItem = 0;

    for (l = hashHead->next; l != hashHead; l = l->next) {
        count++;
	node = WSBMLISTENTRY(l, struct _ValidateNode, hashHead);

	if (node->buf == buf) {
	    cur = node;
	    break;
	}
    }

    if (!cur) {
	cur = validateListAddNode(list, buf, hash, flags, mask);
	if (!cur)
	    return -ENOMEM;
	*newItem = 1;
	cur->func->clear(cur);
    } else {
	uint64_t set_flags = flags & mask;
	uint64_t clr_flags = (~flags) & mask;

	if (((cur->clr_flags | clr_flags) & WSBM_PL_MASK_MEM) ==
	    WSBM_PL_MASK_MEM) {
	    /*
	     * No available memory type left. Bail.
	     */
	    return -EINVAL;
	}

	if ((cur->set_flags | set_flags) &
	    (cur->clr_flags | clr_flags) & ~WSBM_PL_MASK_MEM) {
	    /*
	     * Conflicting flags. Bail.
	     */
	    return -EINVAL;
	}

	cur->set_flags &= ~(clr_flags & WSBM_PL_MASK_MEM);
	cur->set_flags |= (set_flags & ~WSBM_PL_MASK_MEM);
	cur->clr_flags |= clr_flags;
    }
    *itemLoc = cur->listItem;
    if (pnode)
	*pnode = cur;
    return 0;
}

int
wsbmBOAddListItem(struct _WsbmBufferList *list,
		  struct _WsbmBufferObject *buf,
		  uint64_t flags, uint64_t mask, int *itemLoc,
		  struct _ValidateNode **node)
{
    int newItem;
    struct _WsbmBufStorage *storage = buf->storage;
    int ret;
    int dummy;
    struct _ValidateNode *dummyNode;

    if (list->hasKernelBuffers) {
	ret = wsbmAddValidateItem(&list->kernelBuffers,
				  storage->pool->kernel(storage),
				  flags, mask, itemLoc, node, &dummy);
	if (ret)
	    goto out_unlock;
    } else {
	*node = NULL;
	*itemLoc = -1000;
    }

    ret = wsbmAddValidateItem(&list->userBuffers, storage,
			      flags, mask, &dummy, &dummyNode, &newItem);
    if (ret)
	goto out_unlock;

    if (newItem) {
	wsbmAtomicInc(&storage->refCount);
	wsbmAtomicInc(&storage->onList);
    }

  out_unlock:
    return ret;
}

void
wsbmBOFence(struct _WsbmBufferObject *buf, struct _WsbmFenceObject *fence)
{
    struct _WsbmBufStorage *storage;

    storage = buf->storage;
    if (storage->pool->fence)
	storage->pool->fence(storage, fence);

}

int
wsbmBOOnList(const struct _WsbmBufferObject *buf)
{
    if (buf->storage == NULL)
	return 0;
    return wsbmAtomicRead(&buf->storage->onList);
}

int
wsbmBOUnrefUserList(struct _WsbmBufferList *list)
{
    struct _WsbmBufStorage *storage;
    void *curBuf;

    curBuf = validateListIterator(&list->userBuffers);

    while (curBuf) {
	storage = (struct _WsbmBufStorage *)(validateListNode(curBuf)->buf);
	wsbmAtomicDec(&storage->onList);
	wsbmBufStorageUnref(&storage);
	curBuf = validateListNext(&list->userBuffers, curBuf);
    }

    return wsbmBOResetList(list);
}


int
wsbmBOFenceUserList(struct _WsbmBufferList *list,
		    struct _WsbmFenceObject *fence)
{
    struct _WsbmBufStorage *storage;
    void *curBuf;

    curBuf = validateListIterator(&list->userBuffers);

    /*
     * User-space fencing callbacks.
     */

    while (curBuf) {
	storage = (struct _WsbmBufStorage *)(validateListNode(curBuf)->buf);

	storage->pool->fence(storage, fence);
	wsbmAtomicDec(&storage->onList);
	wsbmBufStorageUnref(&storage);
	curBuf = validateListNext(&list->userBuffers, curBuf);
    }

    return wsbmBOResetList(list);
}

int
wsbmBOValidateUserList(struct _WsbmBufferList *list)
{
    void *curBuf;
    struct _WsbmBufStorage *storage;
    struct _ValidateNode *node;
    int ret;

    curBuf = validateListIterator(&list->userBuffers);

    /*
     * User-space validation callbacks.
     */

    while (curBuf) {
	node = validateListNode(curBuf);
	storage = (struct _WsbmBufStorage *)node->buf;
	if (storage->pool->validate) {
	    ret = storage->pool->validate(storage, node->set_flags,
					  node->clr_flags);
	    if (ret)
		return ret;
	}
	curBuf = validateListNext(&list->userBuffers, curBuf);
    }
    return 0;
}

int
wsbmBOUnvalidateUserList(struct _WsbmBufferList *list)
{
    void *curBuf;
    struct _WsbmBufStorage *storage;
    struct _ValidateNode *node;

    curBuf = validateListIterator(&list->userBuffers);

    /*
     * User-space validation callbacks.
     */

    while (curBuf) {
	node = validateListNode(curBuf);
	storage = (struct _WsbmBufStorage *)node->buf;
	if (storage->pool->unvalidate) {
	    storage->pool->unvalidate(storage);
	}
	wsbmAtomicDec(&storage->onList);
	wsbmBufStorageUnref(&storage);
	curBuf = validateListNext(&list->userBuffers, curBuf);
    }
    return wsbmBOResetList(list);
}

void
wsbmPoolTakeDown(struct _WsbmBufferPool *pool)
{
    pool->takeDown(pool);

}

unsigned long
wsbmBOSize(struct _WsbmBufferObject *buf)
{
    unsigned long size;
    struct _WsbmBufStorage *storage;

    storage = buf->storage;
    size = storage->pool->size(storage);

    return size;

}

struct _ValidateList *
wsbmGetKernelValidateList(struct _WsbmBufferList *list)
{
    return (list->hasKernelBuffers) ? &list->kernelBuffers : NULL;
}

struct _ValidateList *
wsbmGetUserValidateList(struct _WsbmBufferList *list)
{
    return &list->userBuffers;
}

struct _ValidateNode *
validateListNode(void *iterator)
{
    struct _WsbmListHead *l = (struct _WsbmListHead *)iterator;

    return WSBMLISTENTRY(l, struct _ValidateNode, head);
}

void *
validateListIterator(struct _ValidateList *list)
{
    void *ret = list->list.next;

    if (ret == &list->list)
	return NULL;
    return ret;
}

void *
validateListNext(struct _ValidateList *list, void *iterator)
{
    void *ret;

    struct _WsbmListHead *l = (struct _WsbmListHead *)iterator;

    ret = l->next;
    if (ret == &list->list)
	return NULL;
    return ret;
}

uint32_t
wsbmKBufHandle(const struct _WsbmKernelBuf * kBuf)
{
    return kBuf->handle;
}

extern void
wsbmUpdateKBuf(struct _WsbmKernelBuf *kBuf,
	       uint64_t gpuOffset, uint32_t placement,
	       uint32_t fence_type_mask)
{
    kBuf->gpuOffset = gpuOffset;
    kBuf->placement = placement;
    kBuf->fence_type_mask = fence_type_mask;
}

extern struct _WsbmKernelBuf *
wsbmKBuf(const struct _WsbmBufferObject *buf)
{
    struct _WsbmBufStorage *storage = buf->storage;

    return storage->pool->kernel(storage);
}
