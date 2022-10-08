/*
 * Copyright (C) 2008 The Android Open Source Project
 * Copyright (c) 2011-2014, The Linux Foundation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <limits.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <cutils/log.h>
#include <cutils/atomic.h>

#include <hardware/hardware.h>
#include <hardware/gralloc.h>

#include "gralloc_priv.h"
#include "gr.h"
#include "alloc_controller.h"
#include "memalloc.h"
#include <qdMetaData.h>

using namespace gralloc;
/*****************************************************************************/

// Return the type of allocator -
// these are used for mapping/unmapping
static IMemAlloc* getAllocator(int flags)
{
    IMemAlloc* memalloc;
    IAllocController* alloc_ctrl = IAllocController::getInstance();
    memalloc = alloc_ctrl->getAllocator(flags);
    return memalloc;
}

static int gralloc_map(gralloc_module_t const* module,
                       buffer_handle_t handle)
{
    if(!module)
        return -EINVAL;

    private_handle_t* hnd = (private_handle_t*)handle;
    void *mappedAddress;
    if (!(hnd->flags & private_handle_t::PRIV_FLAGS_FRAMEBUFFER) &&
        !(hnd->flags & private_handle_t::PRIV_FLAGS_SECURE_BUFFER)) {
        size_t size = hnd->size;
        IMemAlloc* memalloc = getAllocator(hnd->flags) ;
        int err = memalloc->map_buffer(&mappedAddress, size,
                                       hnd->offset, hnd->fd);
        if(err || mappedAddress == MAP_FAILED) {
            ALOGE("Could not mmap handle %p, fd=%d (%s)",
                  handle, hnd->fd, strerror(errno));
            hnd->base = 0;
            return -errno;
        }

        hnd->base = intptr_t(mappedAddress) + hnd->offset;
        mappedAddress = MAP_FAILED;
        size = ROUND_UP_PAGESIZE(sizeof(MetaData_t));
        err = memalloc->map_buffer(&mappedAddress, size,
                                       hnd->offset_metadata, hnd->fd_metadata);
        if(err || mappedAddress == MAP_FAILED) {
            ALOGE("Could not mmap handle %p, fd=%d (%s)",
                  handle, hnd->fd_metadata, strerror(errno));
            hnd->base_metadata = 0;
            return -errno;
        }
        hnd->base_metadata = intptr_t(mappedAddress) + hnd->offset_metadata;
    }
    return 0;
}

static int gralloc_unmap(gralloc_module_t const* module,
                         buffer_handle_t handle)
{
    if(!module)
        return -EINVAL;

    private_handle_t* hnd = (private_handle_t*)handle;
    if (!(hnd->flags & private_handle_t::PRIV_FLAGS_FRAMEBUFFER)) {
        int err = -EINVAL;
        void* base = (void*)hnd->base;
        size_t size = hnd->size;
        IMemAlloc* memalloc = getAllocator(hnd->flags) ;
        if(memalloc != NULL) {
            err = memalloc->unmap_buffer(base, size, hnd->offset);
            if (err) {
                ALOGE("Could not unmap memory at address %p", base);
            }
            base = (void*)hnd->base_metadata;
            size = ROUND_UP_PAGESIZE(sizeof(MetaData_t));
            err = memalloc->unmap_buffer(base, size, hnd->offset_metadata);
            if (err) {
                ALOGE("Could not unmap memory at address %p", base);
            }
        }
    }
    /* need to initialize the pointer to NULL otherwise unmapping for that
     * buffer happens twice which leads to crash */
    hnd->base = 0;
    hnd->base_metadata = 0;
    return 0;
}

/*****************************************************************************/

static pthread_mutex_t sMapLock = PTHREAD_MUTEX_INITIALIZER;

/*****************************************************************************/

int gralloc_register_buffer(gralloc_module_t const* module,
                            buffer_handle_t handle)
{
    if (!module || private_handle_t::validate(handle) < 0)
        return -EINVAL;

    // In this implementation, we don't need to do anything here

    /* NOTE: we need to initialize the buffer as not mapped/not locked
     * because it shouldn't when this function is called the first time
     * in a new process. Ideally these flags shouldn't be part of the
     * handle, but instead maintained in the kernel or at least
     * out-of-line
     */

    private_handle_t* hnd = (private_handle_t*)handle;
    hnd->base = 0;
    hnd->base_metadata = 0;
    int err = gralloc_map(module, handle);
    if (err) {
        ALOGE("%s: gralloc_map failed", __FUNCTION__);
        return err;
    }

    return 0;
}

int gralloc_unregister_buffer(gralloc_module_t const* module,
                              buffer_handle_t handle)
{
    if (!module || private_handle_t::validate(handle) < 0)
        return -EINVAL;

    /*
     * If the buffer has been mapped during a lock operation, it's time
     * to un-map it. It's an error to be here with a locked buffer.
     * NOTE: the framebuffer is handled differently and is never unmapped.
     */

    private_handle_t* hnd = (private_handle_t*)handle;

    if (hnd->base != 0) {
        gralloc_unmap(module, handle);
    }
    hnd->base = 0;
    hnd->base_metadata = 0;
    return 0;
}

int terminateBuffer(gralloc_module_t const* module,
                    private_handle_t* hnd)
{
    if(!module)
        return -EINVAL;

    /*
     * If the buffer has been mapped during a lock operation, it's time
     * to un-map it. It's an error to be here with a locked buffer.
     */

    if (hnd->base != 0) {
        // this buffer was mapped, unmap it now
        if (hnd->flags & (private_handle_t::PRIV_FLAGS_USES_PMEM |
                          private_handle_t::PRIV_FLAGS_USES_PMEM_ADSP |
                          private_handle_t::PRIV_FLAGS_USES_ASHMEM |
                          private_handle_t::PRIV_FLAGS_USES_ION)) {
                gralloc_unmap(module, hnd);
        } else {
            ALOGE("terminateBuffer: unmapping a non pmem/ashmem buffer flags = 0x%x",
                  hnd->flags);
            gralloc_unmap(module, hnd);
        }
    }

    return 0;
}

static int gralloc_map_and_invalidate (gralloc_module_t const* module,
                                       buffer_handle_t handle, int usage)
{
    if (!module || private_handle_t::validate(handle) < 0)
        return -EINVAL;

    int err = 0;
    private_handle_t* hnd = (private_handle_t*)handle;
    if (usage & (GRALLOC_USAGE_SW_READ_MASK | GRALLOC_USAGE_SW_WRITE_MASK)) {
        if (hnd->base == 0) {
            // we need to map for real
            pthread_mutex_t* const lock = &sMapLock;
            pthread_mutex_lock(lock);
            err = gralloc_map(module, handle);
            pthread_mutex_unlock(lock);
        }
        if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_ION) {
            //Invalidate if reading in software. No need to do this for the
            //metadata buffer as it is only read/written in software.
            IMemAlloc* memalloc = getAllocator(hnd->flags) ;
            err = memalloc->clean_buffer((void*)hnd->base,
                                         hnd->size, hnd->offset, hnd->fd,
                                         CACHE_INVALIDATE);
            if (usage & GRALLOC_USAGE_SW_WRITE_MASK) {
                // Mark the buffer to be flushed after cpu read/write
                hnd->flags |= private_handle_t::PRIV_FLAGS_NEEDS_FLUSH;
            }
        }
    } else {
        hnd->flags |= private_handle_t::PRIV_FLAGS_DO_NOT_FLUSH;
    }
    return err;
}

int gralloc_lock(gralloc_module_t const* module,
                 buffer_handle_t handle, int usage,
                 int /*l*/, int /*t*/, int /*w*/, int /*h*/,
                 void** vaddr)
{
    private_handle_t* hnd = (private_handle_t*)handle;
    int err = gralloc_map_and_invalidate(module, handle, usage);
    if(!err)
        *vaddr = (void*)hnd->base;
    return err;
}

int gralloc_lock_ycbcr(gralloc_module_t const* module,
                 buffer_handle_t handle, int usage,
                 int /*l*/, int /*t*/, int /*w*/, int /*h*/,
                 struct android_ycbcr *ycbcr)
{
    private_handle_t* hnd = (private_handle_t*)handle;
    int err = gralloc_map_and_invalidate(module, handle, usage);
    if(!err)
        err = getYUVPlaneInfo(hnd, ycbcr);
    return err;
}

int gralloc_unlock(gralloc_module_t const* module,
                   buffer_handle_t handle)
{
    if (!module || private_handle_t::validate(handle) < 0)
        return -EINVAL;

    int err = 0;
    private_handle_t* hnd = (private_handle_t*)handle;

    if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_ION) {
        IMemAlloc* memalloc = getAllocator(hnd->flags);
        if (hnd->flags & private_handle_t::PRIV_FLAGS_NEEDS_FLUSH) {
            err = memalloc->clean_buffer((void*)hnd->base,
                                         hnd->size, hnd->offset, hnd->fd,
                                         CACHE_CLEAN_AND_INVALIDATE);
            hnd->flags &= ~private_handle_t::PRIV_FLAGS_NEEDS_FLUSH;
        } else if(hnd->flags & private_handle_t::PRIV_FLAGS_DO_NOT_FLUSH) {
            hnd->flags &= ~private_handle_t::PRIV_FLAGS_DO_NOT_FLUSH;
        } else {
            //Probably a round about way to do this, but this avoids adding new
            //flags
            err = memalloc->clean_buffer((void*)hnd->base,
                                         hnd->size, hnd->offset, hnd->fd,
                                         CACHE_INVALIDATE);
        }
    }

    return err;
}

/*****************************************************************************/

int gralloc_perform(struct gralloc_module_t const* module,
                    int operation, ... )
{
    int res = -EINVAL;
    va_list args;
    if(!module)
        return res;

    va_start(args, operation);
    switch (operation) {
        case GRALLOC_MODULE_PERFORM_CREATE_HANDLE_FROM_BUFFER:
            {
                int fd = va_arg(args, int);
                size_t size = va_arg(args, size_t);
                size_t offset = va_arg(args, size_t);
                void* base = va_arg(args, void*);
                int width = va_arg(args, int);
                int height = va_arg(args, int);
                int format = va_arg(args, int);

                native_handle_t** handle = va_arg(args, native_handle_t**);
                private_handle_t* hnd = (private_handle_t*)native_handle_create(
                    private_handle_t::sNumFds, private_handle_t::sNumInts);
                hnd->magic = private_handle_t::sMagic;
                hnd->fd = fd;
                hnd->flags =  private_handle_t::PRIV_FLAGS_USES_ION;
                hnd->size = size;
                hnd->offset = offset;
                hnd->base = intptr_t(base) + offset;
                hnd->gpuaddr = 0;
                hnd->width = width;
                hnd->height = height;
                hnd->format = format;
                *handle = (native_handle_t *)hnd;
                res = 0;
                break;

            }
#ifdef QCOM_BSP
        case GRALLOC_MODULE_PERFORM_UPDATE_BUFFER_GEOMETRY:
            {
                int width = va_arg(args, int);
                int height = va_arg(args, int);
                int format = va_arg(args, int);
                private_handle_t* hnd =  va_arg(args, private_handle_t*);
                if (private_handle_t::validate(hnd)) {
                    return res;
                }
                hnd->width = width;
                hnd->height = height;
                hnd->format = format;
                res = 0;
            }
            break;
#endif
        case GRALLOC_MODULE_PERFORM_GET_STRIDE:
            {
                int width   = va_arg(args, int);
                int format  = va_arg(args, int);
                int *stride = va_arg(args, int *);
                int alignedw = 0, alignedh = 0;
                AdrenoMemInfo::getInstance().getAlignedWidthAndHeight(width,
                        0, format, false, alignedw, alignedh);
                *stride = alignedw;
                res = 0;
            } break;

        case GRALLOC_MODULE_PERFORM_GET_CUSTOM_STRIDE_FROM_HANDLE:
            {
                private_handle_t* hnd =  va_arg(args, private_handle_t*);
                int *stride = va_arg(args, int *);
                if (private_handle_t::validate(hnd)) {
                    return res;
                }
                MetaData_t *metadata = (MetaData_t *)hnd->base_metadata;
                if(metadata && metadata->operation & UPDATE_BUFFER_GEOMETRY) {
                    *stride = metadata->bufferDim.sliceWidth;
                } else {
                    *stride = hnd->width;
                }
                res = 0;
            } break;

        case GRALLOC_MODULE_PERFORM_GET_CUSTOM_STRIDE_AND_HEIGHT_FROM_HANDLE:
            {
                private_handle_t* hnd =  va_arg(args, private_handle_t*);
                int *stride = va_arg(args, int *);
                int *height = va_arg(args, int *);
                if (private_handle_t::validate(hnd)) {
                    return res;
                }
                MetaData_t *metadata = (MetaData_t *)hnd->base_metadata;
                if(metadata && metadata->operation & UPDATE_BUFFER_GEOMETRY) {
                    *stride = metadata->bufferDim.sliceWidth;
                    *height = metadata->bufferDim.sliceHeight;
                } else {
                    *stride = hnd->width;
                    *height = hnd->height;
                }
                res = 0;
            } break;

        case GRALLOC_MODULE_PERFORM_GET_ATTRIBUTES:
            {
                int width   = va_arg(args, int);
                int height  = va_arg(args, int);
                int format  = va_arg(args, int);
                int usage   = va_arg(args, int);
                int *alignedWidth = va_arg(args, int *);
                int *alignedHeight = va_arg(args, int *);
                int *tileEnabled = va_arg(args,int *);
                *tileEnabled = isMacroTileEnabled(format, usage);
                AdrenoMemInfo::getInstance().getAlignedWidthAndHeight(width,
                        height, format, *tileEnabled, *alignedWidth,
                        *alignedHeight);
                res = 0;
            } break;

        case GRALLOC_MODULE_PERFORM_GET_COLOR_SPACE_FROM_HANDLE:
            {
                private_handle_t* hnd =  va_arg(args, private_handle_t*);
                int *color_space = va_arg(args, int *);
                if (private_handle_t::validate(hnd)) {
                    return res;
                }
                MetaData_t *metadata = (MetaData_t *)hnd->base_metadata;
                if(metadata && metadata->operation & UPDATE_COLOR_SPACE) {
                    *color_space = metadata->colorSpace;
                    res = 0;
                }
            } break;
        case GRALLOC_MODULE_PERFORM_GET_YUV_PLANE_INFO:
            {
                private_handle_t* hnd =  va_arg(args, private_handle_t*);
                android_ycbcr* ycbcr = va_arg(args, struct android_ycbcr *);
                if (!private_handle_t::validate(hnd)) {
                    res = getYUVPlaneInfo(hnd, ycbcr);
                }
            } break;

        default:
            break;
    }
    va_end(args);
    return res;
}
