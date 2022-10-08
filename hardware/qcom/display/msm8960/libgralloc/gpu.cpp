/*
 * Copyright (C) 2010 The Android Open Source Project
 * Copyright (c) 2011-2013 The Linux Foundation. All rights reserved.
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
#include <unistd.h>
#include <fcntl.h>
#include <cutils/properties.h>
#include <sys/mman.h>

#include "gr.h"
#include "gpu.h"
#include "memalloc.h"
#include "alloc_controller.h"
#include <qdMetaData.h>
#include "mdp_version.h"

using namespace gralloc;

#define SZ_1M 0x100000

gpu_context_t::gpu_context_t(const private_module_t* module,
                             IAllocController* alloc_ctrl ) :
    mAllocCtrl(alloc_ctrl)
{
    // Zero out the alloc_device_t
    memset(static_cast<alloc_device_t*>(this), 0, sizeof(alloc_device_t));

    // Initialize the procs
    common.tag     = HARDWARE_DEVICE_TAG;
    common.version = 0;
    common.module  = const_cast<hw_module_t*>(&module->base.common);
    common.close   = gralloc_close;
    alloc          = gralloc_alloc;
#ifdef QCOM_BSP
    allocSize      = gralloc_alloc_size;
#endif
    free           = gralloc_free;

}

int gpu_context_t::gralloc_alloc_buffer(size_t size, int usage,
                                        buffer_handle_t* pHandle, int bufferType,
                                        int format, int width, int height)
{
    int err = 0;
    int flags = 0;
    size = roundUpToPageSize(size);
    alloc_data data;
    data.offset = 0;
    data.fd = -1;
    data.base = 0;
    if(format == HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED)
        data.align = 8192;
    else
        data.align = getpagesize();

    /* force 1MB alignment selectively for secure buffers, MDP5 onwards */
    if ((qdutils::MDPVersion::getInstance().getMDPVersion() >= \
         qdutils::MDSS_V5) && (usage & GRALLOC_USAGE_PROTECTED)) {
        data.align = ALIGN(data.align, SZ_1M);
        size = ALIGN(size, data.align);
    }
    data.size = size;
    data.pHandle = (unsigned int) pHandle;
    err = mAllocCtrl->allocate(data, usage);

    if (!err) {
        /* allocate memory for enhancement data */
        alloc_data eData;
        eData.fd = -1;
        eData.base = 0;
        eData.offset = 0;
        eData.size = ROUND_UP_PAGESIZE(sizeof(MetaData_t));
        eData.pHandle = data.pHandle;
        eData.align = getpagesize();
        int eDataUsage = GRALLOC_USAGE_PRIVATE_SYSTEM_HEAP;
        int eDataErr = mAllocCtrl->allocate(eData, eDataUsage);
        ALOGE_IF(eDataErr, "gralloc failed for eDataErr=%s",
                                          strerror(-eDataErr));

        if (usage & GRALLOC_USAGE_PRIVATE_EXTERNAL_ONLY) {
            flags |= private_handle_t::PRIV_FLAGS_EXTERNAL_ONLY;
            //The EXTERNAL_BLOCK flag is always an add-on
            if (usage & GRALLOC_USAGE_PRIVATE_EXTERNAL_BLOCK) {
                flags |= private_handle_t::PRIV_FLAGS_EXTERNAL_BLOCK;
            }
            if (usage & GRALLOC_USAGE_PRIVATE_EXTERNAL_CC) {
                flags |= private_handle_t::PRIV_FLAGS_EXTERNAL_CC;
            }
        }

        if (bufferType == BUFFER_TYPE_VIDEO) {
            if (usage & GRALLOC_USAGE_HW_CAMERA_WRITE) {
                if ((qdutils::MDPVersion::getInstance().getMDPVersion() <
                     qdutils::MDSS_V5)) { //A-Family
                    flags |= private_handle_t::PRIV_FLAGS_ITU_R_601_FR;
                } else {
                    if (usage & (GRALLOC_USAGE_HW_TEXTURE |
                                 GRALLOC_USAGE_HW_VIDEO_ENCODER))
                        flags |= private_handle_t::PRIV_FLAGS_ITU_R_709;
                    else if (usage & GRALLOC_USAGE_HW_CAMERA_ZSL)
                        flags |= private_handle_t::PRIV_FLAGS_ITU_R_601_FR;
                }
            } else {
                flags |= private_handle_t::PRIV_FLAGS_ITU_R_601;
            }
        }

        if (usage & GRALLOC_USAGE_HW_VIDEO_ENCODER ) {
            flags |= private_handle_t::PRIV_FLAGS_VIDEO_ENCODER;
        }

        if (usage & GRALLOC_USAGE_HW_CAMERA_WRITE) {
            flags |= private_handle_t::PRIV_FLAGS_CAMERA_WRITE;
        }

        if (usage & GRALLOC_USAGE_HW_CAMERA_READ) {
            flags |= private_handle_t::PRIV_FLAGS_CAMERA_READ;
        }

        if (usage & GRALLOC_USAGE_HW_COMPOSER) {
            flags |= private_handle_t::PRIV_FLAGS_HW_COMPOSER;
        }

        if (usage & GRALLOC_USAGE_HW_TEXTURE) {
            flags |= private_handle_t::PRIV_FLAGS_HW_TEXTURE;
        }

        flags |= data.allocType;
        int eBaseAddr = int(eData.base) + eData.offset;
        private_handle_t *hnd = new private_handle_t(data.fd, size, flags,
                bufferType, format, width, height, eData.fd, eData.offset,
                eBaseAddr);

        hnd->offset = data.offset;
        hnd->base = int(data.base) + data.offset;
        hnd->gpuaddr = 0;

        *pHandle = hnd;
    }

    ALOGE_IF(err, "gralloc failed err=%s", strerror(-err));

    return err;
}

void gpu_context_t::getGrallocInformationFromFormat(int inputFormat,
                                                    int *bufferType)
{
    *bufferType = BUFFER_TYPE_VIDEO;

    if (inputFormat <= HAL_PIXEL_FORMAT_sRGB_X_8888) {
        // RGB formats
        *bufferType = BUFFER_TYPE_UI;
    } else if ((inputFormat == HAL_PIXEL_FORMAT_R_8) ||
               (inputFormat == HAL_PIXEL_FORMAT_RG_88)) {
        *bufferType = BUFFER_TYPE_UI;
    }
}

int gpu_context_t::gralloc_alloc_framebuffer_locked(size_t size, int usage,
                                                    buffer_handle_t* pHandle)
{
    private_module_t* m = reinterpret_cast<private_module_t*>(common.module);

    // we don't support framebuffer allocations with graphics heap flags
    if (usage & GRALLOC_HEAP_MASK) {
        return -EINVAL;
    }

    if (m->framebuffer == NULL) {
        ALOGE("%s: Invalid framebuffer", __FUNCTION__);
        return -EINVAL;
    }

    const uint32_t bufferMask = m->bufferMask;
    const uint32_t numBuffers = m->numBuffers;
    size_t bufferSize = m->finfo.line_length * m->info.yres;

    //adreno needs FB size to be page aligned
    bufferSize = roundUpToPageSize(bufferSize);

    if (numBuffers == 1) {
        // If we have only one buffer, we never use page-flipping. Instead,
        // we return a regular buffer which will be memcpy'ed to the main
        // screen when post is called.
        int newUsage = (usage & ~GRALLOC_USAGE_HW_FB) | GRALLOC_USAGE_HW_2D;
        return gralloc_alloc_buffer(bufferSize, newUsage, pHandle, BUFFER_TYPE_UI,
                                    m->fbFormat, m->info.xres, m->info.yres);
    }

    if (bufferMask >= ((1LU<<numBuffers)-1)) {
        // We ran out of buffers.
        return -ENOMEM;
    }

    // create a "fake" handle for it
    intptr_t vaddr = intptr_t(m->framebuffer->base);
    private_handle_t* hnd = new private_handle_t(
        dup(m->framebuffer->fd), bufferSize,
        private_handle_t::PRIV_FLAGS_USES_PMEM |
        private_handle_t::PRIV_FLAGS_FRAMEBUFFER,
        BUFFER_TYPE_UI, m->fbFormat, m->info.xres,
        m->info.yres);

    // find a free slot
    for (uint32_t i=0 ; i<numBuffers ; i++) {
        if ((bufferMask & (1LU<<i)) == 0) {
            m->bufferMask |= (1LU<<i);
            break;
        }
        vaddr += bufferSize;
    }
    hnd->base = vaddr;
    hnd->offset = vaddr - intptr_t(m->framebuffer->base);
    *pHandle = hnd;
    return 0;
}


int gpu_context_t::gralloc_alloc_framebuffer(size_t size, int usage,
                                             buffer_handle_t* pHandle)
{
    private_module_t* m = reinterpret_cast<private_module_t*>(common.module);
    pthread_mutex_lock(&m->lock);
    int err = gralloc_alloc_framebuffer_locked(size, usage, pHandle);
    pthread_mutex_unlock(&m->lock);
    return err;
}

int gpu_context_t::alloc_impl(int w, int h, int format, int usage,
                              buffer_handle_t* pHandle, int* pStride,
                              size_t bufferSize) {
    if (!pHandle || !pStride)
        return -EINVAL;

    size_t size;
    int alignedw, alignedh;
    int grallocFormat = format;
    int bufferType;

    //If input format is HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED then based on
    //the usage bits, gralloc assigns a format.
    if(format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED ||
       format == HAL_PIXEL_FORMAT_YCbCr_420_888) {
        if(usage & GRALLOC_USAGE_HW_VIDEO_ENCODER)
            grallocFormat = HAL_PIXEL_FORMAT_NV12_ENCODEABLE; //NV12
        else if(usage & GRALLOC_USAGE_HW_CAMERA_READ)
            grallocFormat = HAL_PIXEL_FORMAT_YCrCb_420_SP; //NV21
        else if(usage & GRALLOC_USAGE_HW_CAMERA_WRITE)
            grallocFormat = HAL_PIXEL_FORMAT_YCrCb_420_SP; //NV21
    }

    getGrallocInformationFromFormat(grallocFormat, &bufferType);
    size = getBufferSizeAndDimensions(w, h, grallocFormat, alignedw, alignedh);

    if ((ssize_t)size <= 0)
        return -EINVAL;
    size = (bufferSize >= size)? bufferSize : size;

    // All buffers marked as protected or for external
    // display need to go to overlay
    if ((usage & GRALLOC_USAGE_EXTERNAL_DISP) ||
        (usage & GRALLOC_USAGE_PROTECTED)) {
        bufferType = BUFFER_TYPE_VIDEO;
    }

    bool useFbMem = false;
    char property[PROPERTY_VALUE_MAX];
    if((usage & GRALLOC_USAGE_HW_FB) &&
       (property_get("debug.gralloc.map_fb_memory", property, NULL) > 0) &&
       (!strncmp(property, "1", PROPERTY_VALUE_MAX ) ||
        (!strncasecmp(property,"true", PROPERTY_VALUE_MAX )))) {
        useFbMem = true;
    }

    int err = 0;
    if(useFbMem) {
        err = gralloc_alloc_framebuffer(size, usage, pHandle);
    } else {
        err = gralloc_alloc_buffer(size, usage, pHandle, bufferType,
                                   grallocFormat, alignedw, alignedh);
    }

    if (err < 0) {
        return err;
    }

    *pStride = alignedw;
    return 0;
}

int gpu_context_t::free_impl(private_handle_t const* hnd) {
    private_module_t* m = reinterpret_cast<private_module_t*>(common.module);
    if (hnd->flags & private_handle_t::PRIV_FLAGS_FRAMEBUFFER) {
        const size_t bufferSize = m->finfo.line_length * m->info.yres;
        int index = (hnd->base - m->framebuffer->base) / bufferSize;
        m->bufferMask &= ~(1<<index);
    } else {

        terminateBuffer(&m->base, const_cast<private_handle_t*>(hnd));
        IMemAlloc* memalloc = mAllocCtrl->getAllocator(hnd->flags);
        int err = memalloc->free_buffer((void*)hnd->base, (size_t) hnd->size,
                                        hnd->offset, hnd->fd);
        if(err)
            return err;
        // free the metadata space
        unsigned long size = ROUND_UP_PAGESIZE(sizeof(MetaData_t));
        err = memalloc->free_buffer((void*)hnd->base_metadata,
                                    (size_t) size, hnd->offset_metadata,
                                    hnd->fd_metadata);
        if (err)
            return err;
    }
    delete hnd;
    return 0;
}

int gpu_context_t::gralloc_alloc(alloc_device_t* dev, int w, int h, int format,
                                 int usage, buffer_handle_t* pHandle,
                                 int* pStride)
{
    if (!dev) {
        return -EINVAL;
    }
    gpu_context_t* gpu = reinterpret_cast<gpu_context_t*>(dev);
    return gpu->alloc_impl(w, h, format, usage, pHandle, pStride, 0);
}
int gpu_context_t::gralloc_alloc_size(alloc_device_t* dev, int w, int h,
                                      int format, int usage,
                                      buffer_handle_t* pHandle, int* pStride,
                                      int bufferSize)
{
    if (!dev) {
        return -EINVAL;
    }
    gpu_context_t* gpu = reinterpret_cast<gpu_context_t*>(dev);
    return gpu->alloc_impl(w, h, format, usage, pHandle, pStride, bufferSize);
}


int gpu_context_t::gralloc_free(alloc_device_t* dev,
                                buffer_handle_t handle)
{
    if (private_handle_t::validate(handle) < 0)
        return -EINVAL;

    private_handle_t const* hnd = reinterpret_cast<private_handle_t const*>(handle);
    gpu_context_t* gpu = reinterpret_cast<gpu_context_t*>(dev);
    return gpu->free_impl(hnd);
}

/*****************************************************************************/

int gpu_context_t::gralloc_close(struct hw_device_t *dev)
{
    gpu_context_t* ctx = reinterpret_cast<gpu_context_t*>(dev);
    if (ctx) {
        /* TODO: keep a list of all buffer_handle_t created, and free them
         * all here.
         */
        delete ctx;
    }
    return 0;
}

