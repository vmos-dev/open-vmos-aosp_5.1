/*
 * Copyright (C) 2008 The Android Open Source Project
 * Copyright (c) 2010-2014, The Linux Foundation. All rights reserved.
 *
 * Not a Contribution, Apache license notifications and license are retained
 * for attribution purposes only.
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
#include <cutils/log.h>
#include <sys/resource.h>
#include <sys/prctl.h>

#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>

#include <linux/msm_kgsl.h>

#include <EGL/eglplatform.h>
#include <cutils/native_handle.h>
#include <gralloc_priv.h>

#include <copybit.h>
#include <alloc_controller.h>
#include <memalloc.h>

#include "c2d2.h"
#include "software_converter.h"

#include <dlfcn.h>

using gralloc::IMemAlloc;
using gralloc::IonController;
using gralloc::alloc_data;

C2D_STATUS (*LINK_c2dCreateSurface)( uint32 *surface_id,
                                     uint32 surface_bits,
                                     C2D_SURFACE_TYPE surface_type,
                                     void *surface_definition );

C2D_STATUS (*LINK_c2dUpdateSurface)( uint32 surface_id,
                                     uint32 surface_bits,
                                     C2D_SURFACE_TYPE surface_type,
                                     void *surface_definition );

C2D_STATUS (*LINK_c2dReadSurface)( uint32 surface_id,
                                   C2D_SURFACE_TYPE surface_type,
                                   void *surface_definition,
                                   int32 x, int32 y );

C2D_STATUS (*LINK_c2dDraw)( uint32 target_id,
                            uint32 target_config, C2D_RECT *target_scissor,
                            uint32 target_mask_id, uint32 target_color_key,
                            C2D_OBJECT *objects_list, uint32 num_objects );

C2D_STATUS (*LINK_c2dFinish)( uint32 target_id);

C2D_STATUS (*LINK_c2dFlush)( uint32 target_id, c2d_ts_handle *timestamp);

C2D_STATUS (*LINK_c2dWaitTimestamp)( c2d_ts_handle timestamp );

C2D_STATUS (*LINK_c2dDestroySurface)( uint32 surface_id );

C2D_STATUS (*LINK_c2dMapAddr) ( int mem_fd, void * hostptr, size_t len,
                                size_t offset, uint32 flags, void ** gpuaddr);

C2D_STATUS (*LINK_c2dUnMapAddr) ( void * gpuaddr);

C2D_STATUS (*LINK_c2dGetDriverCapabilities) ( C2D_DRIVER_INFO * driver_info);

/* create a fence fd for the timestamp */
C2D_STATUS (*LINK_c2dCreateFenceFD) ( uint32 target_id, c2d_ts_handle timestamp,
                                                            int32 *fd);

C2D_STATUS (*LINK_c2dFillSurface) ( uint32 surface_id, uint32 fill_color,
                                    C2D_RECT * fill_rect);

/******************************************************************************/

#if defined(COPYBIT_Z180)
#define MAX_SCALE_FACTOR    (4096)
#define MAX_DIMENSION       (4096)
#else
#error "Unsupported HW version"
#endif

// The following defines can be changed as required i.e. as we encounter
// complex use cases.
#define MAX_RGB_SURFACES 32       // Max. RGB layers currently supported per draw
#define MAX_YUV_2_PLANE_SURFACES 4// Max. 2-plane YUV layers currently supported per draw
#define MAX_YUV_3_PLANE_SURFACES 1// Max. 3-plane YUV layers currently supported per draw
// +1 for the destination surface. We cannot have multiple destination surfaces.
#define MAX_SURFACES (MAX_RGB_SURFACES + MAX_YUV_2_PLANE_SURFACES + MAX_YUV_3_PLANE_SURFACES + 1)
#define NUM_SURFACE_TYPES 3      // RGB_SURFACE + YUV_SURFACE_2_PLANES + YUV_SURFACE_3_PLANES
#define MAX_BLIT_OBJECT_COUNT 50 // Max. blit objects that can be passed per draw

enum {
    RGB_SURFACE,
    YUV_SURFACE_2_PLANES,
    YUV_SURFACE_3_PLANES
};

enum eConversionType {
    CONVERT_TO_ANDROID_FORMAT,
    CONVERT_TO_C2D_FORMAT
};

enum eC2DFlags {
    FLAGS_PREMULTIPLIED_ALPHA  = 1<<0,
    FLAGS_YUV_DESTINATION      = 1<<1,
    FLAGS_TEMP_SRC_DST         = 1<<2
};

static gralloc::IAllocController* sAlloc = 0;
/******************************************************************************/

/** State information for each device instance */
struct copybit_context_t {
    struct copybit_device_t device;
    // Templates for the various source surfaces. These templates are created
    // to avoid the expensive create/destroy C2D Surfaces
    C2D_OBJECT_STR blit_rgb_object[MAX_RGB_SURFACES];
    C2D_OBJECT_STR blit_yuv_2_plane_object[MAX_YUV_2_PLANE_SURFACES];
    C2D_OBJECT_STR blit_yuv_3_plane_object[MAX_YUV_3_PLANE_SURFACES];
    C2D_OBJECT_STR blit_list[MAX_BLIT_OBJECT_COUNT]; // Z-ordered list of blit objects
    C2D_DRIVER_INFO c2d_driver_info;
    void *libc2d2;
    alloc_data temp_src_buffer;
    alloc_data temp_dst_buffer;
    unsigned int dst[NUM_SURFACE_TYPES]; // dst surfaces
    uintptr_t mapped_gpu_addr[MAX_SURFACES]; // GPU addresses mapped inside copybit
    int blit_rgb_count;         // Total RGB surfaces being blit
    int blit_yuv_2_plane_count; // Total 2 plane YUV surfaces being
    int blit_yuv_3_plane_count; // Total 3 plane YUV  surfaces being blit
    int blit_count;             // Total blit objects.
    unsigned int trg_transform;      /* target transform */
    int fb_width;
    int fb_height;
    int src_global_alpha;
    int config_mask;
    int dst_surface_type;
    bool is_premultiplied_alpha;
    void* time_stamp;
    bool dst_surface_mapped; // Set when dst surface is mapped to GPU addr
    void* dst_surface_base; // Stores the dst surface addr

    // used for signaling the wait thread
    bool wait_timestamp;
    pthread_t wait_thread_id;
    bool stop_thread;
    pthread_mutex_t wait_cleanup_lock;
    pthread_cond_t wait_cleanup_cond;

};

struct bufferInfo {
    int width;
    int height;
    int format;
};

struct yuvPlaneInfo {
    int yStride;       //luma stride
    int plane1_stride;
    int plane2_stride;
    size_t plane1_offset;
    size_t plane2_offset;
};

/**
 * Common hardware methods
 */

static int open_copybit(const struct hw_module_t* module, const char* name,
                        struct hw_device_t** device);

static struct hw_module_methods_t copybit_module_methods = {
open:  open_copybit
};

/*
 * The COPYBIT Module
 */
struct copybit_module_t HAL_MODULE_INFO_SYM = {
common: {
tag: HARDWARE_MODULE_TAG,
     version_major: 1,
     version_minor: 0,
     id: COPYBIT_HARDWARE_MODULE_ID,
     name: "QCT COPYBIT C2D 2.0 Module",
     author: "Qualcomm",
     methods: &copybit_module_methods
        }
};


/* thread function which waits on the timeStamp and cleans up the surfaces */
static void* c2d_wait_loop(void* ptr) {
    copybit_context_t* ctx = (copybit_context_t*)(ptr);
    char thread_name[64] = "copybitWaitThr";
    prctl(PR_SET_NAME, (unsigned long) &thread_name, 0, 0, 0);
    setpriority(PRIO_PROCESS, 0, HAL_PRIORITY_URGENT_DISPLAY);

    while(ctx->stop_thread == false) {
        pthread_mutex_lock(&ctx->wait_cleanup_lock);
        while(ctx->wait_timestamp == false && !ctx->stop_thread) {
            pthread_cond_wait(&(ctx->wait_cleanup_cond),
                              &(ctx->wait_cleanup_lock));
        }
        if(ctx->wait_timestamp) {
            if(LINK_c2dWaitTimestamp(ctx->time_stamp)) {
                ALOGE("%s: LINK_c2dWaitTimeStamp ERROR!!", __FUNCTION__);
            }
            ctx->wait_timestamp = false;
            // Unmap any mapped addresses.
            for (int i = 0; i < MAX_SURFACES; i++) {
                if (ctx->mapped_gpu_addr[i]) {
                    LINK_c2dUnMapAddr( (void*)ctx->mapped_gpu_addr[i]);
                    ctx->mapped_gpu_addr[i] = 0;
                }
            }
            // Reset the counts after the draw.
            ctx->blit_rgb_count = 0;
            ctx->blit_yuv_2_plane_count = 0;
            ctx->blit_yuv_3_plane_count = 0;
            ctx->blit_count = 0;
            ctx->dst_surface_mapped = false;
            ctx->dst_surface_base = 0;
        }
        pthread_mutex_unlock(&ctx->wait_cleanup_lock);
        if(ctx->stop_thread)
            break;
    }
    pthread_exit(NULL);
    return NULL;
}


/* convert COPYBIT_FORMAT to C2D format */
static int get_format(int format) {
    switch (format) {
        case HAL_PIXEL_FORMAT_RGB_565:        return C2D_COLOR_FORMAT_565_RGB;
        case HAL_PIXEL_FORMAT_RGBX_8888:      return C2D_COLOR_FORMAT_8888_ARGB |
                                              C2D_FORMAT_SWAP_RB |
                                                  C2D_FORMAT_DISABLE_ALPHA;
        case HAL_PIXEL_FORMAT_RGBA_8888:      return C2D_COLOR_FORMAT_8888_ARGB |
                                              C2D_FORMAT_SWAP_RB;
        case HAL_PIXEL_FORMAT_BGRA_8888:      return C2D_COLOR_FORMAT_8888_ARGB;
        case HAL_PIXEL_FORMAT_YCbCr_420_SP:   return C2D_COLOR_FORMAT_420_NV12;
        case HAL_PIXEL_FORMAT_NV12_ENCODEABLE:return C2D_COLOR_FORMAT_420_NV12;
        case HAL_PIXEL_FORMAT_YCrCb_420_SP:   return C2D_COLOR_FORMAT_420_NV21;
        case HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED: return C2D_COLOR_FORMAT_420_NV12 |
                                                  C2D_FORMAT_MACROTILED;
        default:                              ALOGE("%s: invalid format (0x%x",
                                                     __FUNCTION__, format);
                                              return -EINVAL;
    }
    return -EINVAL;
}

/* Get the C2D formats needed for conversion to YUV */
static int get_c2d_format_for_yuv_destination(int halFormat) {
    switch (halFormat) {
        // We do not swap the RB when the target is YUV
        case HAL_PIXEL_FORMAT_RGBX_8888:      return C2D_COLOR_FORMAT_8888_ARGB |
                                              C2D_FORMAT_DISABLE_ALPHA;
        case HAL_PIXEL_FORMAT_RGBA_8888:      return C2D_COLOR_FORMAT_8888_ARGB;
        // The U and V need to be interchanged when the target is YUV
        case HAL_PIXEL_FORMAT_YCbCr_420_SP:   return C2D_COLOR_FORMAT_420_NV21;
        case HAL_PIXEL_FORMAT_NV12_ENCODEABLE:return C2D_COLOR_FORMAT_420_NV21;
        case HAL_PIXEL_FORMAT_YCrCb_420_SP:   return C2D_COLOR_FORMAT_420_NV12;
        default:                              return get_format(halFormat);
    }
    return -EINVAL;
}

/* ------------------------------------------------------------------- *//*!
 * \internal
 * \brief Get the bpp for a particular color format
 * \param color format
 * \return bits per pixel
 *//* ------------------------------------------------------------------- */
int c2diGetBpp(int32 colorformat)
{

    int c2dBpp = 0;

    switch(colorformat&0xFF)
    {
        case C2D_COLOR_FORMAT_4444_RGBA:
        case C2D_COLOR_FORMAT_4444_ARGB:
        case C2D_COLOR_FORMAT_1555_ARGB:
        case C2D_COLOR_FORMAT_565_RGB:
        case C2D_COLOR_FORMAT_5551_RGBA:
            c2dBpp = 16;
            break;
        case C2D_COLOR_FORMAT_8888_RGBA:
        case C2D_COLOR_FORMAT_8888_ARGB:
            c2dBpp = 32;
            break;
        case C2D_COLOR_FORMAT_8_L:
        case C2D_COLOR_FORMAT_8_A:
            c2dBpp = 8;
            break;
        case C2D_COLOR_FORMAT_4_A:
            c2dBpp = 4;
            break;
        case C2D_COLOR_FORMAT_1:
            c2dBpp = 1;
            break;
        default:
            ALOGE("%s ERROR", __func__);
            break;
    }
    return c2dBpp;
}

static size_t c2d_get_gpuaddr(copybit_context_t* ctx,
                              struct private_handle_t *handle, int &mapped_idx)
{
    uint32 memtype;
    size_t *gpuaddr = 0;
    C2D_STATUS rc;
    int freeindex = 0;
    bool mapaddr = false;

    if(!handle)
        return 0;

    if (handle->flags & (private_handle_t::PRIV_FLAGS_USES_PMEM |
                         private_handle_t::PRIV_FLAGS_USES_PMEM_ADSP))
        memtype = KGSL_USER_MEM_TYPE_PMEM;
    else if (handle->flags & private_handle_t::PRIV_FLAGS_USES_ASHMEM)
        memtype = KGSL_USER_MEM_TYPE_ASHMEM;
    else if (handle->flags & private_handle_t::PRIV_FLAGS_USES_ION)
        memtype = KGSL_USER_MEM_TYPE_ION;
    else {
        ALOGE("Invalid handle flags: 0x%x", handle->flags);
        return 0;
    }

    // Check for a freeindex in the mapped_gpu_addr list
    for (freeindex = 0; freeindex < MAX_SURFACES; freeindex++) {
        if (ctx->mapped_gpu_addr[freeindex] == 0) {
            // free index is available
            // map GPU addr and use this as mapped_idx
            mapaddr = true;
            break;
        }
    }

    if(mapaddr) {
        rc = LINK_c2dMapAddr(handle->fd, (void*)handle->base, handle->size,
                             handle->offset, memtype, (void**)&gpuaddr);

        if (rc == C2D_STATUS_OK) {
            // We have mapped the GPU address inside copybit. We need to unmap
            // this address after the blit. Store this address
            ctx->mapped_gpu_addr[freeindex] = (size_t)gpuaddr;
            mapped_idx = freeindex;
        }
    }
    return (size_t)gpuaddr;
}

static void unmap_gpuaddr(copybit_context_t* ctx, int mapped_idx)
{
    if (!ctx || (mapped_idx == -1))
        return;

    if (ctx->mapped_gpu_addr[mapped_idx]) {
        LINK_c2dUnMapAddr( (void*)ctx->mapped_gpu_addr[mapped_idx]);
        ctx->mapped_gpu_addr[mapped_idx] = 0;
    }
}

static int is_supported_rgb_format(int format)
{
    switch(format) {
        case HAL_PIXEL_FORMAT_RGBA_8888:
        case HAL_PIXEL_FORMAT_RGBX_8888:
        case HAL_PIXEL_FORMAT_RGB_565:
        case HAL_PIXEL_FORMAT_BGRA_8888: {
            return COPYBIT_SUCCESS;
        }
        default:
            return COPYBIT_FAILURE;
    }
}

static int get_num_planes(int format)
{
    switch(format) {
        case HAL_PIXEL_FORMAT_YCbCr_420_SP:
        case HAL_PIXEL_FORMAT_YCrCb_420_SP:
        case HAL_PIXEL_FORMAT_NV12_ENCODEABLE:
        case HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED: {
            return 2;
        }
        case HAL_PIXEL_FORMAT_YV12: {
            return 3;
        }
        default:
            return COPYBIT_FAILURE;
    }
}

static int is_supported_yuv_format(int format)
{
    switch(format) {
        case HAL_PIXEL_FORMAT_YCbCr_420_SP:
        case HAL_PIXEL_FORMAT_YCrCb_420_SP:
        case HAL_PIXEL_FORMAT_NV12_ENCODEABLE:
        case HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED: {
            return COPYBIT_SUCCESS;
        }
        default:
            return COPYBIT_FAILURE;
    }
}

static int is_valid_destination_format(int format)
{
    if (format == HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED) {
        // C2D does not support NV12Tile as a destination format.
        return COPYBIT_FAILURE;
    }
    return COPYBIT_SUCCESS;
}

static int calculate_yuv_offset_and_stride(const bufferInfo& info,
                                           yuvPlaneInfo& yuvInfo)
{
    int width  = info.width;
    int height = info.height;
    int format = info.format;

    int aligned_height = 0;
    int aligned_width = 0, size = 0;

    switch (format) {
        case HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED: {
            /* NV12 Tile buffers have their luma height aligned to 32bytes and width
             * aligned to 128 bytes. The chroma offset starts at an 8K boundary
             */
            aligned_height = ALIGN(height, 32);
            aligned_width  = ALIGN(width, 128);
            size = aligned_width * aligned_height;
            yuvInfo.plane1_offset = ALIGN(size,8192);
            yuvInfo.yStride = aligned_width;
            yuvInfo.plane1_stride = aligned_width;
            break;
        }
        case HAL_PIXEL_FORMAT_YCbCr_420_SP:
        case HAL_PIXEL_FORMAT_NV12_ENCODEABLE:
        case HAL_PIXEL_FORMAT_YCrCb_420_SP: {
            aligned_width = ALIGN(width, 32);
            yuvInfo.yStride = aligned_width;
            yuvInfo.plane1_stride = aligned_width;
            if (HAL_PIXEL_FORMAT_NV12_ENCODEABLE == format) {
                // The encoder requires a 2K aligned chroma offset
                yuvInfo.plane1_offset = ALIGN(aligned_width * height, 2048);
            } else
                yuvInfo.plane1_offset = aligned_width * height;

            break;
        }
        default: {
            return COPYBIT_FAILURE;
        }
    }
    return COPYBIT_SUCCESS;
}

/** create C2D surface from copybit image */
static int set_image(copybit_context_t* ctx, uint32 surfaceId,
                      const struct copybit_image_t *rhs,
                      const eC2DFlags flags, int &mapped_idx)
{
    struct private_handle_t* handle = (struct private_handle_t*)rhs->handle;
    C2D_SURFACE_TYPE surfaceType;
    int status = COPYBIT_SUCCESS;
    uintptr_t gpuaddr = 0;
    int c2d_format;
    mapped_idx = -1;

    if (flags & FLAGS_YUV_DESTINATION) {
        c2d_format = get_c2d_format_for_yuv_destination(rhs->format);
    } else {
        c2d_format = get_format(rhs->format);
    }

    if(c2d_format == -EINVAL) {
        ALOGE("%s: invalid format", __FUNCTION__);
        return -EINVAL;
    }

    if(handle == NULL) {
        ALOGE("%s: invalid handle", __func__);
        return -EINVAL;
    }

    if (handle->gpuaddr == 0) {
        gpuaddr = c2d_get_gpuaddr(ctx, handle, mapped_idx);
        if(!gpuaddr) {
            ALOGE("%s: c2d_get_gpuaddr failed", __FUNCTION__);
            return COPYBIT_FAILURE;
        }
    } else {
        gpuaddr = handle->gpuaddr;
    }

    /* create C2D surface */
    if(is_supported_rgb_format(rhs->format) == COPYBIT_SUCCESS) {
        /* RGB */
        C2D_RGB_SURFACE_DEF surfaceDef;

        surfaceType = (C2D_SURFACE_TYPE) (C2D_SURFACE_RGB_HOST | C2D_SURFACE_WITH_PHYS);

        surfaceDef.phys = (void*) gpuaddr;
        surfaceDef.buffer = (void*) (handle->base);

        surfaceDef.format = c2d_format |
            ((flags & FLAGS_PREMULTIPLIED_ALPHA) ? C2D_FORMAT_PREMULTIPLIED : 0);
        surfaceDef.width = rhs->w;
        surfaceDef.height = rhs->h;
        int aligned_width = ALIGN((int)surfaceDef.width,32);
        surfaceDef.stride = (aligned_width * c2diGetBpp(surfaceDef.format))>>3;

        if(LINK_c2dUpdateSurface( surfaceId,C2D_TARGET | C2D_SOURCE, surfaceType,
                                  &surfaceDef)) {
            ALOGE("%s: RGB Surface c2dUpdateSurface ERROR", __FUNCTION__);
            unmap_gpuaddr(ctx, mapped_idx);
            status = COPYBIT_FAILURE;
        }
    } else if (is_supported_yuv_format(rhs->format) == COPYBIT_SUCCESS) {
        C2D_YUV_SURFACE_DEF surfaceDef;
        memset(&surfaceDef, 0, sizeof(surfaceDef));
        surfaceType = (C2D_SURFACE_TYPE)(C2D_SURFACE_YUV_HOST | C2D_SURFACE_WITH_PHYS);
        surfaceDef.format = c2d_format;

        bufferInfo info;
        info.width = rhs->w;
        info.height = rhs->h;
        info.format = rhs->format;

        yuvPlaneInfo yuvInfo = {0};
        status = calculate_yuv_offset_and_stride(info, yuvInfo);
        if(status != COPYBIT_SUCCESS) {
            ALOGE("%s: calculate_yuv_offset_and_stride error", __FUNCTION__);
            unmap_gpuaddr(ctx, mapped_idx);
        }

        surfaceDef.width = rhs->w;
        surfaceDef.height = rhs->h;
        surfaceDef.plane0 = (void*) (handle->base);
        surfaceDef.phys0 = (void*) (gpuaddr);
        surfaceDef.stride0 = yuvInfo.yStride;

        surfaceDef.plane1 = (void*) (handle->base + yuvInfo.plane1_offset);
        surfaceDef.phys1 = (void*) (gpuaddr + yuvInfo.plane1_offset);
        surfaceDef.stride1 = yuvInfo.plane1_stride;
        if (3 == get_num_planes(rhs->format)) {
            surfaceDef.plane2 = (void*) (handle->base + yuvInfo.plane2_offset);
            surfaceDef.phys2 = (void*) (gpuaddr + yuvInfo.plane2_offset);
            surfaceDef.stride2 = yuvInfo.plane2_stride;
        }

        if(LINK_c2dUpdateSurface( surfaceId,C2D_TARGET | C2D_SOURCE, surfaceType,
                                  &surfaceDef)) {
            ALOGE("%s: YUV Surface c2dUpdateSurface ERROR", __FUNCTION__);
            unmap_gpuaddr(ctx, mapped_idx);
            status = COPYBIT_FAILURE;
        }
    } else {
        ALOGE("%s: invalid format 0x%x", __FUNCTION__, rhs->format);
        unmap_gpuaddr(ctx, mapped_idx);
        status = COPYBIT_FAILURE;
    }

    return status;
}

/** copy the bits */
static int msm_copybit(struct copybit_context_t *ctx, unsigned int target)
{
    if (ctx->blit_count == 0) {
        return COPYBIT_SUCCESS;
    }

    for (int i = 0; i < ctx->blit_count; i++)
    {
        ctx->blit_list[i].next = &(ctx->blit_list[i+1]);
    }
    ctx->blit_list[ctx->blit_count-1].next = NULL;
    uint32_t target_transform = ctx->trg_transform;
    if (ctx->c2d_driver_info.capabilities_mask &
        C2D_DRIVER_SUPPORTS_OVERRIDE_TARGET_ROTATE_OP) {
        // For A3xx - set 0x0 as the transform is set in the config_mask
        target_transform = 0x0;
    }
    if(LINK_c2dDraw(target, target_transform, 0x0, 0, 0, ctx->blit_list,
                    ctx->blit_count)) {
        ALOGE("%s: LINK_c2dDraw ERROR", __FUNCTION__);
        return COPYBIT_FAILURE;
    }
    return COPYBIT_SUCCESS;
}



static int flush_get_fence_copybit (struct copybit_device_t *dev, int* fd)
{
    struct copybit_context_t* ctx = (struct copybit_context_t*)dev;
    int status = COPYBIT_FAILURE;
    if (!ctx)
        return COPYBIT_FAILURE;
    pthread_mutex_lock(&ctx->wait_cleanup_lock);
    status = msm_copybit(ctx, ctx->dst[ctx->dst_surface_type]);

    if(LINK_c2dFlush(ctx->dst[ctx->dst_surface_type], &ctx->time_stamp)) {
        ALOGE("%s: LINK_c2dFlush ERROR", __FUNCTION__);
        // unlock the mutex and return failure
        pthread_mutex_unlock(&ctx->wait_cleanup_lock);
        return COPYBIT_FAILURE;
    }
    if(LINK_c2dCreateFenceFD(ctx->dst[ctx->dst_surface_type], ctx->time_stamp,
                                                                        fd)) {
        ALOGE("%s: LINK_c2dCreateFenceFD ERROR", __FUNCTION__);
        status = COPYBIT_FAILURE;
    }
    if(status == COPYBIT_SUCCESS) {
        //signal the wait_thread
        ctx->wait_timestamp = true;
        pthread_cond_signal(&ctx->wait_cleanup_cond);
    }
    pthread_mutex_unlock(&ctx->wait_cleanup_lock);
    return status;
}

static int finish_copybit(struct copybit_device_t *dev)
{
    struct copybit_context_t* ctx = (struct copybit_context_t*)dev;
    if (!ctx)
        return COPYBIT_FAILURE;

   int status = msm_copybit(ctx, ctx->dst[ctx->dst_surface_type]);

   if(LINK_c2dFinish(ctx->dst[ctx->dst_surface_type])) {
        ALOGE("%s: LINK_c2dFinish ERROR", __FUNCTION__);
        return COPYBIT_FAILURE;
    }

    // Unmap any mapped addresses.
    for (int i = 0; i < MAX_SURFACES; i++) {
        if (ctx->mapped_gpu_addr[i]) {
            LINK_c2dUnMapAddr( (void*)ctx->mapped_gpu_addr[i]);
            ctx->mapped_gpu_addr[i] = 0;
        }
    }

    // Reset the counts after the draw.
    ctx->blit_rgb_count = 0;
    ctx->blit_yuv_2_plane_count = 0;
    ctx->blit_yuv_3_plane_count = 0;
    ctx->blit_count = 0;
    ctx->dst_surface_mapped = false;
    ctx->dst_surface_base = 0;

    return status;
}

static int clear_copybit(struct copybit_device_t *dev,
                         struct copybit_image_t const *buf,
                         struct copybit_rect_t *rect)
{
    int ret = COPYBIT_SUCCESS;
    int flags = FLAGS_PREMULTIPLIED_ALPHA;
    int mapped_dst_idx = -1;
    struct copybit_context_t* ctx = (struct copybit_context_t*)dev;
    C2D_RECT c2drect = {rect->l, rect->t, rect->r - rect->l, rect->b - rect->t};
    pthread_mutex_lock(&ctx->wait_cleanup_lock);
    if(!ctx->dst_surface_mapped) {
        ret = set_image(ctx, ctx->dst[RGB_SURFACE], buf,
                        (eC2DFlags)flags, mapped_dst_idx);
        if(ret) {
            ALOGE("%s: set_image error", __FUNCTION__);
            unmap_gpuaddr(ctx, mapped_dst_idx);
            pthread_mutex_unlock(&ctx->wait_cleanup_lock);
            return COPYBIT_FAILURE;
        }
        //clear_copybit is the first call made by HWC for each composition
        //with the dest surface, hence set dst_surface_mapped.
        ctx->dst_surface_mapped = true;
        ctx->dst_surface_base = buf->base;
        ret = LINK_c2dFillSurface(ctx->dst[RGB_SURFACE], 0x0, &c2drect);
    }
    pthread_mutex_unlock(&ctx->wait_cleanup_lock);
    return ret;
}


/** setup rectangles */
static void set_rects(struct copybit_context_t *ctx,
                      C2D_OBJECT *c2dObject,
                      const struct copybit_rect_t *dst,
                      const struct copybit_rect_t *src,
                      const struct copybit_rect_t *scissor)
{
    // Set the target rect.
    if((ctx->trg_transform & C2D_TARGET_ROTATE_90) &&
       (ctx->trg_transform & C2D_TARGET_ROTATE_180)) {
        /* target rotation is 270 */
        c2dObject->target_rect.x        = (dst->t)<<16;
        c2dObject->target_rect.y        = ctx->fb_width?
                (ALIGN(ctx->fb_width,32)- dst->r):dst->r;
        c2dObject->target_rect.y        = c2dObject->target_rect.y<<16;
        c2dObject->target_rect.height   = ((dst->r) - (dst->l))<<16;
        c2dObject->target_rect.width    = ((dst->b) - (dst->t))<<16;
    } else if(ctx->trg_transform & C2D_TARGET_ROTATE_90) {
        c2dObject->target_rect.x        = ctx->fb_height?(ctx->fb_height - dst->b):dst->b;
        c2dObject->target_rect.x        = c2dObject->target_rect.x<<16;
        c2dObject->target_rect.y        = (dst->l)<<16;
        c2dObject->target_rect.height   = ((dst->r) - (dst->l))<<16;
        c2dObject->target_rect.width    = ((dst->b) - (dst->t))<<16;
    } else if(ctx->trg_transform & C2D_TARGET_ROTATE_180) {
        c2dObject->target_rect.y        = ctx->fb_height?(ctx->fb_height - dst->b):dst->b;
        c2dObject->target_rect.y        = c2dObject->target_rect.y<<16;
        c2dObject->target_rect.x        = ctx->fb_width?
                (ALIGN(ctx->fb_width,32) - dst->r):dst->r;
        c2dObject->target_rect.x        = c2dObject->target_rect.x<<16;
        c2dObject->target_rect.height   = ((dst->b) - (dst->t))<<16;
        c2dObject->target_rect.width    = ((dst->r) - (dst->l))<<16;
    } else {
        c2dObject->target_rect.x        = (dst->l)<<16;
        c2dObject->target_rect.y        = (dst->t)<<16;
        c2dObject->target_rect.height   = ((dst->b) - (dst->t))<<16;
        c2dObject->target_rect.width    = ((dst->r) - (dst->l))<<16;
    }
    c2dObject->config_mask |= C2D_TARGET_RECT_BIT;

    // Set the source rect
    c2dObject->source_rect.x        = (src->l)<<16;
    c2dObject->source_rect.y        = (src->t)<<16;
    c2dObject->source_rect.height   = ((src->b) - (src->t))<<16;
    c2dObject->source_rect.width    = ((src->r) - (src->l))<<16;
    c2dObject->config_mask |= C2D_SOURCE_RECT_BIT;

    // Set the scissor rect
    c2dObject->scissor_rect.x       = scissor->l;
    c2dObject->scissor_rect.y       = scissor->t;
    c2dObject->scissor_rect.height  = (scissor->b) - (scissor->t);
    c2dObject->scissor_rect.width   = (scissor->r) - (scissor->l);
    c2dObject->config_mask |= C2D_SCISSOR_RECT_BIT;
}

/*****************************************************************************/

/** Set a parameter to value */
static int set_parameter_copybit(
    struct copybit_device_t *dev,
    int name,
    int value)
{
    struct copybit_context_t* ctx = (struct copybit_context_t*)dev;
    int status = COPYBIT_SUCCESS;
    if (!ctx) {
        ALOGE("%s: null context", __FUNCTION__);
        return -EINVAL;
    }

    pthread_mutex_lock(&ctx->wait_cleanup_lock);
    switch(name) {
        case COPYBIT_PLANE_ALPHA:
        {
            if (value < 0)      value = 0;
            if (value >= 256)   value = 255;

            ctx->src_global_alpha = value;
            if (value < 255)
                ctx->config_mask |= C2D_GLOBAL_ALPHA_BIT;
            else
                ctx->config_mask &= ~C2D_GLOBAL_ALPHA_BIT;
        }
        break;
        case COPYBIT_BLEND_MODE:
        {
            if (value == COPYBIT_BLENDING_NONE) {
                ctx->config_mask |= C2D_ALPHA_BLEND_NONE;
                ctx->is_premultiplied_alpha = true;
            } else if (value == COPYBIT_BLENDING_PREMULT) {
                ctx->is_premultiplied_alpha = true;
            } else {
                ctx->config_mask &= ~C2D_ALPHA_BLEND_NONE;
            }
        }
        break;
        case COPYBIT_TRANSFORM:
        {
            unsigned int transform = 0;
            uint32 config_mask = 0;
            config_mask |= C2D_OVERRIDE_GLOBAL_TARGET_ROTATE_CONFIG;
            if((value & 0x7) == COPYBIT_TRANSFORM_ROT_180) {
                transform = C2D_TARGET_ROTATE_180;
                config_mask |= C2D_OVERRIDE_TARGET_ROTATE_180;
            } else if((value & 0x7) == COPYBIT_TRANSFORM_ROT_270) {
                transform = C2D_TARGET_ROTATE_90;
                config_mask |= C2D_OVERRIDE_TARGET_ROTATE_90;
            } else if(value == COPYBIT_TRANSFORM_ROT_90) {
                transform = C2D_TARGET_ROTATE_270;
                config_mask |= C2D_OVERRIDE_TARGET_ROTATE_270;
            } else {
                config_mask |= C2D_OVERRIDE_TARGET_ROTATE_0;
                if(value & COPYBIT_TRANSFORM_FLIP_H) {
                    config_mask |= C2D_MIRROR_H_BIT;
                } else if(value & COPYBIT_TRANSFORM_FLIP_V) {
                    config_mask |= C2D_MIRROR_V_BIT;
                }
            }

            if (ctx->c2d_driver_info.capabilities_mask &
                C2D_DRIVER_SUPPORTS_OVERRIDE_TARGET_ROTATE_OP) {
                ctx->config_mask |= config_mask;
            } else {
                // The transform for this surface does not match the current
                // target transform. Draw all previous surfaces. This will be
                // changed once we have a new mechanism to send different
                // target rotations to c2d.
                finish_copybit(dev);
            }
            ctx->trg_transform = transform;
        }
        break;
        case COPYBIT_FRAMEBUFFER_WIDTH:
            ctx->fb_width = value;
            break;
        case COPYBIT_FRAMEBUFFER_HEIGHT:
            ctx->fb_height = value;
            break;
        case COPYBIT_ROTATION_DEG:
        case COPYBIT_DITHER:
        case COPYBIT_BLUR:
        case COPYBIT_BLIT_TO_FRAMEBUFFER:
            // Do nothing
            break;
        default:
            ALOGE("%s: default case param=0x%x", __FUNCTION__, name);
            status = -EINVAL;
            break;
    }
    pthread_mutex_unlock(&ctx->wait_cleanup_lock);
    return status;
}

/** Get a static info value */
static int get(struct copybit_device_t *dev, int name)
{
    struct copybit_context_t* ctx = (struct copybit_context_t*)dev;
    int value;

    if (!ctx) {
        ALOGE("%s: null context error", __FUNCTION__);
        return -EINVAL;
    }

    switch(name) {
        case COPYBIT_MINIFICATION_LIMIT:
            value = MAX_SCALE_FACTOR;
            break;
        case COPYBIT_MAGNIFICATION_LIMIT:
            value = MAX_SCALE_FACTOR;
            break;
        case COPYBIT_SCALING_FRAC_BITS:
            value = 32;
            break;
        case COPYBIT_ROTATION_STEP_DEG:
            value = 1;
            break;
        default:
            ALOGE("%s: default case param=0x%x", __FUNCTION__, name);
            value = -EINVAL;
    }
    return value;
}

static int is_alpha(int cformat)
{
    int alpha = 0;
    switch (cformat & 0xFF) {
        case C2D_COLOR_FORMAT_8888_ARGB:
        case C2D_COLOR_FORMAT_8888_RGBA:
        case C2D_COLOR_FORMAT_5551_RGBA:
        case C2D_COLOR_FORMAT_4444_ARGB:
            alpha = 1;
            break;
        default:
            alpha = 0;
            break;
    }

    if(alpha && (cformat&C2D_FORMAT_DISABLE_ALPHA))
        alpha = 0;

    return alpha;
}

/* Function to check if we need a temporary buffer for the blit.
 * This would happen if the requested destination stride and the
 * C2D stride do not match. We ignore RGB buffers, since their
 * stride is always aligned to 32.
 */
static bool need_temp_buffer(struct copybit_image_t const *img)
{
    if (COPYBIT_SUCCESS == is_supported_rgb_format(img->format))
        return false;

    struct private_handle_t* handle = (struct private_handle_t*)img->handle;

    // The width parameter in the handle contains the aligned_w. We check if we
    // need to convert based on this param. YUV formats have bpp=1, so checking
    // if the requested stride is aligned should suffice.
    if (0 == (handle->width)%32) {
        return false;
    }

    return true;
}

/* Function to extract the information from the copybit image and set the corresponding
 * values in the bufferInfo struct.
 */
static void populate_buffer_info(struct copybit_image_t const *img, bufferInfo& info)
{
    info.width = img->w;
    info.height = img->h;
    info.format = img->format;
}

/* Function to get the required size for a particular format, inorder for C2D to perform
 * the blit operation.
 */
static size_t get_size(const bufferInfo& info)
{
    int size = 0;
    int w = info.width;
    int h = info.height;
    int aligned_w = ALIGN(w, 32);
    switch(info.format) {
        case HAL_PIXEL_FORMAT_NV12_ENCODEABLE:
            {
                // Chroma for this format is aligned to 2K.
                size = ALIGN((aligned_w*h), 2048) +
                        ALIGN(aligned_w/2, 32) * (h/2) *2;
                size = ALIGN(size, 4096);
            } break;
        case HAL_PIXEL_FORMAT_YCbCr_420_SP:
        case HAL_PIXEL_FORMAT_YCrCb_420_SP:
            {
                size = aligned_w * h +
                       ALIGN(aligned_w/2, 32) * (h/2) * 2;
                size = ALIGN(size, 4096);
            } break;
        default: break;
    }
    return size;
}

/* Function to allocate memory for the temporary buffer. This memory is
 * allocated from Ashmem. It is the caller's responsibility to free this
 * memory.
 */
static int get_temp_buffer(const bufferInfo& info, alloc_data& data)
{
    ALOGD("%s E", __FUNCTION__);
    // Alloc memory from system heap
    data.base = 0;
    data.fd = -1;
    data.offset = 0;
    data.size = get_size(info);
    data.align = getpagesize();
    data.uncached = true;
    int allocFlags = GRALLOC_USAGE_PRIVATE_SYSTEM_HEAP;

    if (sAlloc == 0) {
        sAlloc = gralloc::IAllocController::getInstance();
    }

    if (sAlloc == 0) {
        ALOGE("%s: sAlloc is still NULL", __FUNCTION__);
        return COPYBIT_FAILURE;
    }

    int err = sAlloc->allocate(data, allocFlags);
    if (0 != err) {
        ALOGE("%s: allocate failed", __FUNCTION__);
        return COPYBIT_FAILURE;
    }

    ALOGD("%s X", __FUNCTION__);
    return err;
}

/* Function to free the temporary allocated memory.*/
static void free_temp_buffer(alloc_data &data)
{
    if (-1 != data.fd) {
        IMemAlloc* memalloc = sAlloc->getAllocator(data.allocType);
        memalloc->free_buffer(data.base, data.size, 0, data.fd);
    }
}

/* Function to perform the software color conversion. Convert the
 * C2D compatible format to the Android compatible format
 */
static int copy_image(private_handle_t *src_handle,
                      struct copybit_image_t const *rhs,
                      eConversionType conversionType)
{
    if (src_handle->fd == -1) {
        ALOGE("%s: src_handle fd is invalid", __FUNCTION__);
        return COPYBIT_FAILURE;
    }

    // Copy the info.
    int ret = COPYBIT_SUCCESS;
    switch(rhs->format) {
        case HAL_PIXEL_FORMAT_NV12_ENCODEABLE:
        case HAL_PIXEL_FORMAT_YCbCr_420_SP:
        case HAL_PIXEL_FORMAT_YCrCb_420_SP:
            {
                if (CONVERT_TO_ANDROID_FORMAT == conversionType) {
                    return convert_yuv_c2d_to_yuv_android(src_handle, rhs);
                } else {
                    return convert_yuv_android_to_yuv_c2d(src_handle, rhs);
                }

            } break;
        default: {
            ALOGE("%s: invalid format 0x%x", __FUNCTION__, rhs->format);
            ret = COPYBIT_FAILURE;
        } break;
    }
    return ret;
}

static void delete_handle(private_handle_t *handle)
{
    if (handle) {
        delete handle;
        handle = 0;
    }
}

static bool need_to_execute_draw(eC2DFlags flags)
{
    if (flags & FLAGS_TEMP_SRC_DST) {
        return true;
    }
    if (flags & FLAGS_YUV_DESTINATION) {
        return true;
    }
    return false;
}

/** do a stretch blit type operation */
static int stretch_copybit_internal(
    struct copybit_device_t *dev,
    struct copybit_image_t const *dst,
    struct copybit_image_t const *src,
    struct copybit_rect_t const *dst_rect,
    struct copybit_rect_t const *src_rect,
    struct copybit_region_t const *region,
    bool enableBlend)
{
    struct copybit_context_t* ctx = (struct copybit_context_t*)dev;
    int status = COPYBIT_SUCCESS;
    int flags = 0;
    int src_surface_type;
    int mapped_src_idx = -1, mapped_dst_idx = -1;
    C2D_OBJECT_STR src_surface;

    if (!ctx) {
        ALOGE("%s: null context error", __FUNCTION__);
        return -EINVAL;
    }

    if (src->w > MAX_DIMENSION || src->h > MAX_DIMENSION) {
        ALOGE("%s: src dimension error", __FUNCTION__);
        return -EINVAL;
    }

    if (dst->w > MAX_DIMENSION || dst->h > MAX_DIMENSION) {
        ALOGE("%s : dst dimension error dst w %d h %d",  __FUNCTION__, dst->w,
                                                         dst->h);
        return -EINVAL;
    }

    if (is_valid_destination_format(dst->format) == COPYBIT_FAILURE) {
        ALOGE("%s: Invalid destination format format = 0x%x", __FUNCTION__,
                                                              dst->format);
        return COPYBIT_FAILURE;
    }

    int dst_surface_type;
    if (is_supported_rgb_format(dst->format) == COPYBIT_SUCCESS) {
        dst_surface_type = RGB_SURFACE;
        flags |= FLAGS_PREMULTIPLIED_ALPHA;
    } else if (is_supported_yuv_format(dst->format) == COPYBIT_SUCCESS) {
        int num_planes = get_num_planes(dst->format);
        flags |= FLAGS_YUV_DESTINATION;
        if (num_planes == 2) {
            dst_surface_type = YUV_SURFACE_2_PLANES;
        } else if (num_planes == 3) {
            dst_surface_type = YUV_SURFACE_3_PLANES;
        } else {
            ALOGE("%s: dst number of YUV planes is invalid dst format = 0x%x",
                  __FUNCTION__, dst->format);
            return COPYBIT_FAILURE;
        }
    } else {
        ALOGE("%s: Invalid dst surface format 0x%x", __FUNCTION__,
                                                     dst->format);
        return COPYBIT_FAILURE;
    }

    if (ctx->blit_rgb_count == MAX_RGB_SURFACES ||
        ctx->blit_yuv_2_plane_count == MAX_YUV_2_PLANE_SURFACES ||
        ctx->blit_yuv_3_plane_count == MAX_YUV_2_PLANE_SURFACES ||
        ctx->blit_count == MAX_BLIT_OBJECT_COUNT ||
        ctx->dst_surface_type != dst_surface_type) {
        // we have reached the max. limits of our internal structures or
        // changed the target.
        // Draw the remaining surfaces. We need to do the finish here since
        // we need to free up the surface templates.
        finish_copybit(dev);
    }

    ctx->dst_surface_type = dst_surface_type;

    // Update the destination
    copybit_image_t dst_image;
    dst_image.w = dst->w;
    dst_image.h = dst->h;
    dst_image.format = dst->format;
    dst_image.handle = dst->handle;
    // Check if we need a temp. copy for the destination. We'd need this the destination
    // width is not aligned to 32. This case occurs for YUV formats. RGB formats are
    // aligned to 32.
    bool need_temp_dst = need_temp_buffer(dst);
    bufferInfo dst_info;
    populate_buffer_info(dst, dst_info);
    private_handle_t* dst_hnd = new private_handle_t(-1, 0, 0, 0, dst_info.format,
                                                     dst_info.width, dst_info.height);
    if (dst_hnd == NULL) {
        ALOGE("%s: dst_hnd is null", __FUNCTION__);
        return COPYBIT_FAILURE;
    }
    if (need_temp_dst) {
        if (get_size(dst_info) != ctx->temp_dst_buffer.size) {
            free_temp_buffer(ctx->temp_dst_buffer);
            // Create a temp buffer and set that as the destination.
            if (COPYBIT_FAILURE == get_temp_buffer(dst_info, ctx->temp_dst_buffer)) {
                ALOGE("%s: get_temp_buffer(dst) failed", __FUNCTION__);
                delete_handle(dst_hnd);
                return COPYBIT_FAILURE;
            }
        }
        dst_hnd->fd = ctx->temp_dst_buffer.fd;
        dst_hnd->size = ctx->temp_dst_buffer.size;
        dst_hnd->flags = ctx->temp_dst_buffer.allocType;
        dst_hnd->base = (uintptr_t)(ctx->temp_dst_buffer.base);
        dst_hnd->offset = ctx->temp_dst_buffer.offset;
        dst_hnd->gpuaddr = 0;
        dst_image.handle = dst_hnd;
    }
    if(!ctx->dst_surface_mapped) {
        //map the destination surface to GPU address
        status = set_image(ctx, ctx->dst[ctx->dst_surface_type], &dst_image,
                           (eC2DFlags)flags, mapped_dst_idx);
        if(status) {
            ALOGE("%s: dst: set_image error", __FUNCTION__);
            delete_handle(dst_hnd);
            unmap_gpuaddr(ctx, mapped_dst_idx);
            return COPYBIT_FAILURE;
        }
        ctx->dst_surface_mapped = true;
        ctx->dst_surface_base = dst->base;
    } else if(ctx->dst_surface_mapped && ctx->dst_surface_base != dst->base) {
        // Destination surface for the operation should be same for multiple
        // requests, this check is catch if there is any case when the
        // destination changes
        ALOGE("%s: a different destination surface!!", __FUNCTION__);
    }

    // Update the source
    flags = 0;
    if(is_supported_rgb_format(src->format) == COPYBIT_SUCCESS) {
        src_surface_type = RGB_SURFACE;
        src_surface = ctx->blit_rgb_object[ctx->blit_rgb_count];
    } else if (is_supported_yuv_format(src->format) == COPYBIT_SUCCESS) {
        int num_planes = get_num_planes(src->format);
        if (num_planes == 2) {
            src_surface_type = YUV_SURFACE_2_PLANES;
            src_surface = ctx->blit_yuv_2_plane_object[ctx->blit_yuv_2_plane_count];
        } else if (num_planes == 3) {
            src_surface_type = YUV_SURFACE_3_PLANES;
            src_surface = ctx->blit_yuv_3_plane_object[ctx->blit_yuv_2_plane_count];
        } else {
            ALOGE("%s: src number of YUV planes is invalid src format = 0x%x",
                  __FUNCTION__, src->format);
            delete_handle(dst_hnd);
            unmap_gpuaddr(ctx, mapped_dst_idx);
            return -EINVAL;
        }
    } else {
        ALOGE("%s: Invalid source surface format 0x%x", __FUNCTION__,
                                                        src->format);
        delete_handle(dst_hnd);
        unmap_gpuaddr(ctx, mapped_dst_idx);
        return -EINVAL;
    }

    copybit_image_t src_image;
    src_image.w = src->w;
    src_image.h = src->h;
    src_image.format = src->format;
    src_image.handle = src->handle;

    bool need_temp_src = need_temp_buffer(src);
    bufferInfo src_info;
    populate_buffer_info(src, src_info);
    private_handle_t* src_hnd = new private_handle_t(-1, 0, 0, 0, src_info.format,
                                                 src_info.width, src_info.height);
    if (NULL == src_hnd) {
        ALOGE("%s: src_hnd is null", __FUNCTION__);
        delete_handle(dst_hnd);
        unmap_gpuaddr(ctx, mapped_dst_idx);
        return COPYBIT_FAILURE;
    }
    if (need_temp_src) {
        if (get_size(src_info) != ctx->temp_src_buffer.size) {
            free_temp_buffer(ctx->temp_src_buffer);
            // Create a temp buffer and set that as the destination.
            if (COPYBIT_SUCCESS != get_temp_buffer(src_info,
                                               ctx->temp_src_buffer)) {
                ALOGE("%s: get_temp_buffer(src) failed", __FUNCTION__);
                delete_handle(dst_hnd);
                delete_handle(src_hnd);
                unmap_gpuaddr(ctx, mapped_dst_idx);
                return COPYBIT_FAILURE;
            }
        }
        src_hnd->fd = ctx->temp_src_buffer.fd;
        src_hnd->size = ctx->temp_src_buffer.size;
        src_hnd->flags = ctx->temp_src_buffer.allocType;
        src_hnd->base = (uintptr_t)(ctx->temp_src_buffer.base);
        src_hnd->offset = ctx->temp_src_buffer.offset;
        src_hnd->gpuaddr = 0;
        src_image.handle = src_hnd;

        // Copy the source.
        status = copy_image((private_handle_t *)src->handle, &src_image,
                                CONVERT_TO_C2D_FORMAT);
        if (status == COPYBIT_FAILURE) {
            ALOGE("%s:copy_image failed in temp source",__FUNCTION__);
            delete_handle(dst_hnd);
            delete_handle(src_hnd);
            unmap_gpuaddr(ctx, mapped_dst_idx);
            return status;
        }

        // Clean the cache
        IMemAlloc* memalloc = sAlloc->getAllocator(src_hnd->flags);
        if (memalloc->clean_buffer((void *)(src_hnd->base), src_hnd->size,
                                   src_hnd->offset, src_hnd->fd,
                                   gralloc::CACHE_CLEAN)) {
            ALOGE("%s: clean_buffer failed", __FUNCTION__);
            delete_handle(dst_hnd);
            delete_handle(src_hnd);
            unmap_gpuaddr(ctx, mapped_dst_idx);
            return COPYBIT_FAILURE;
        }
    }

    flags |= (ctx->is_premultiplied_alpha) ? FLAGS_PREMULTIPLIED_ALPHA : 0;
    flags |= (ctx->dst_surface_type != RGB_SURFACE) ? FLAGS_YUV_DESTINATION : 0;
    status = set_image(ctx, src_surface.surface_id, &src_image,
                       (eC2DFlags)flags, mapped_src_idx);
    if(status) {
        ALOGE("%s: set_image (src) error", __FUNCTION__);
        delete_handle(dst_hnd);
        delete_handle(src_hnd);
        unmap_gpuaddr(ctx, mapped_dst_idx);
        unmap_gpuaddr(ctx, mapped_src_idx);
        return COPYBIT_FAILURE;
    }

    src_surface.config_mask = C2D_NO_ANTIALIASING_BIT | ctx->config_mask;
    src_surface.global_alpha = ctx->src_global_alpha;
    if (enableBlend) {
        if(src_surface.config_mask & C2D_GLOBAL_ALPHA_BIT) {
            src_surface.config_mask &= ~C2D_ALPHA_BLEND_NONE;
            if(!(src_surface.global_alpha)) {
                // src alpha is zero
                delete_handle(dst_hnd);
                delete_handle(src_hnd);
                unmap_gpuaddr(ctx, mapped_dst_idx);
                unmap_gpuaddr(ctx, mapped_src_idx);
                return COPYBIT_FAILURE;
            }
        }
    } else {
        src_surface.config_mask |= C2D_ALPHA_BLEND_NONE;
    }

    if (src_surface_type == RGB_SURFACE) {
        ctx->blit_rgb_object[ctx->blit_rgb_count] = src_surface;
        ctx->blit_rgb_count++;
    } else if (src_surface_type == YUV_SURFACE_2_PLANES) {
        ctx->blit_yuv_2_plane_object[ctx->blit_yuv_2_plane_count] = src_surface;
        ctx->blit_yuv_2_plane_count++;
    } else {
        ctx->blit_yuv_3_plane_object[ctx->blit_yuv_3_plane_count] = src_surface;
        ctx->blit_yuv_3_plane_count++;
    }

    struct copybit_rect_t clip;
    while ((status == 0) && region->next(region, &clip)) {
        set_rects(ctx, &(src_surface), dst_rect, src_rect, &clip);
        if (ctx->blit_count == MAX_BLIT_OBJECT_COUNT) {
            ALOGW("Reached end of blit count");
            finish_copybit(dev);
        }
        ctx->blit_list[ctx->blit_count] = src_surface;
        ctx->blit_count++;
    }

    // Check if we need to perform an early draw-finish.
    flags |= (need_temp_dst || need_temp_src) ? FLAGS_TEMP_SRC_DST : 0;
    if (need_to_execute_draw((eC2DFlags)flags))
    {
        finish_copybit(dev);
    }

    if (need_temp_dst) {
        // copy the temp. destination without the alignment to the actual
        // destination.
        status = copy_image(dst_hnd, dst, CONVERT_TO_ANDROID_FORMAT);
        if (status == COPYBIT_FAILURE) {
            ALOGE("%s:copy_image failed in temp Dest",__FUNCTION__);
            delete_handle(dst_hnd);
            delete_handle(src_hnd);
            unmap_gpuaddr(ctx, mapped_dst_idx);
            unmap_gpuaddr(ctx, mapped_src_idx);
            return status;
        }
        // Clean the cache.
        IMemAlloc* memalloc = sAlloc->getAllocator(dst_hnd->flags);
        memalloc->clean_buffer((void *)(dst_hnd->base), dst_hnd->size,
                               dst_hnd->offset, dst_hnd->fd,
                               gralloc::CACHE_CLEAN);
    }
    delete_handle(dst_hnd);
    delete_handle(src_hnd);

    ctx->is_premultiplied_alpha = false;
    ctx->fb_width = 0;
    ctx->fb_height = 0;
    ctx->config_mask = 0;
    return status;
}

static int set_sync_copybit(struct copybit_device_t *dev,
    int /*acquireFenceFd*/)
{
    if(!dev)
        return -EINVAL;

    return 0;
}

static int stretch_copybit(
    struct copybit_device_t *dev,
    struct copybit_image_t const *dst,
    struct copybit_image_t const *src,
    struct copybit_rect_t const *dst_rect,
    struct copybit_rect_t const *src_rect,
    struct copybit_region_t const *region)
{
    struct copybit_context_t* ctx = (struct copybit_context_t*)dev;
    int status = COPYBIT_SUCCESS;
    bool needsBlending = (ctx->src_global_alpha != 0);
    pthread_mutex_lock(&ctx->wait_cleanup_lock);
    status = stretch_copybit_internal(dev, dst, src, dst_rect, src_rect,
                                    region, needsBlending);
    pthread_mutex_unlock(&ctx->wait_cleanup_lock);
    return status;
}

/** Perform a blit type operation */
static int blit_copybit(
    struct copybit_device_t *dev,
    struct copybit_image_t const *dst,
    struct copybit_image_t const *src,
    struct copybit_region_t const *region)
{
    int status = COPYBIT_SUCCESS;
    struct copybit_context_t* ctx = (struct copybit_context_t*)dev;
    struct copybit_rect_t dr = { 0, 0, (int)dst->w, (int)dst->h };
    struct copybit_rect_t sr = { 0, 0, (int)src->w, (int)src->h };
    pthread_mutex_lock(&ctx->wait_cleanup_lock);
    status = stretch_copybit_internal(dev, dst, src, &dr, &sr, region, false);
    pthread_mutex_unlock(&ctx->wait_cleanup_lock);
    return status;
}

/** Fill the rect on dst with RGBA color **/
static int fill_color(struct copybit_device_t *dev,
                      struct copybit_image_t const *dst,
                      struct copybit_rect_t const *rect,
                      uint32_t /*color*/)
{
    // TODO: Implement once c2d driver supports color fill
    if(!dev || !dst || !rect)
       return -EINVAL;

    return -EINVAL;
}

/*****************************************************************************/

static void clean_up(copybit_context_t* ctx)
{
    void* ret;
    if (!ctx)
        return;

    // stop the wait_cleanup_thread
    pthread_mutex_lock(&ctx->wait_cleanup_lock);
    ctx->stop_thread = true;
    // Signal waiting thread
    pthread_cond_signal(&ctx->wait_cleanup_cond);
    pthread_mutex_unlock(&ctx->wait_cleanup_lock);
    // waits for the cleanup thread to exit
    pthread_join(ctx->wait_thread_id, &ret);
    pthread_mutex_destroy(&ctx->wait_cleanup_lock);
    pthread_cond_destroy (&ctx->wait_cleanup_cond);

    for (int i = 0; i < NUM_SURFACE_TYPES; i++) {
        if (ctx->dst[i])
            LINK_c2dDestroySurface(ctx->dst[i]);
    }

    for (int i = 0; i < MAX_RGB_SURFACES; i++) {
        if (ctx->blit_rgb_object[i].surface_id)
            LINK_c2dDestroySurface(ctx->blit_rgb_object[i].surface_id);
    }

    for (int i = 0; i < MAX_YUV_2_PLANE_SURFACES; i++) {
        if (ctx->blit_yuv_2_plane_object[i].surface_id)
            LINK_c2dDestroySurface(ctx->blit_yuv_2_plane_object[i].surface_id);
    }

    for (int i = 0; i < MAX_YUV_3_PLANE_SURFACES; i++) {
        if (ctx->blit_yuv_3_plane_object[i].surface_id)
            LINK_c2dDestroySurface(ctx->blit_yuv_3_plane_object[i].surface_id);
    }

    if (ctx->libc2d2) {
        ::dlclose(ctx->libc2d2);
        ALOGV("dlclose(libc2d2)");
    }

    free(ctx);
}

/** Close the copybit device */
static int close_copybit(struct hw_device_t *dev)
{
    struct copybit_context_t* ctx = (struct copybit_context_t*)dev;
    if (ctx) {
        free_temp_buffer(ctx->temp_src_buffer);
        free_temp_buffer(ctx->temp_dst_buffer);
    }
    clean_up(ctx);
    return 0;
}

/** Open a new instance of a copybit device using name */
static int open_copybit(const struct hw_module_t* module, const char* name,
                        struct hw_device_t** device)
{
    int status = COPYBIT_SUCCESS;
    if (strcmp(name, COPYBIT_HARDWARE_COPYBIT0)) {
        return COPYBIT_FAILURE;
    }

    C2D_RGB_SURFACE_DEF surfDefinition = {0};
    C2D_YUV_SURFACE_DEF yuvSurfaceDef = {0} ;
    struct copybit_context_t *ctx;

    ctx = (struct copybit_context_t *)malloc(sizeof(struct copybit_context_t));
    if(!ctx) {
        ALOGE("%s: malloc failed", __FUNCTION__);
        return COPYBIT_FAILURE;
    }

    /* initialize drawstate */
    memset(ctx, 0, sizeof(*ctx));
    ctx->libc2d2 = ::dlopen("libC2D2.so", RTLD_NOW);
    if (!ctx->libc2d2) {
        ALOGE("FATAL ERROR: could not dlopen libc2d2.so: %s", dlerror());
        clean_up(ctx);
        status = COPYBIT_FAILURE;
        *device = NULL;
        return status;
    }
    *(void **)&LINK_c2dCreateSurface = ::dlsym(ctx->libc2d2,
                                               "c2dCreateSurface");
    *(void **)&LINK_c2dUpdateSurface = ::dlsym(ctx->libc2d2,
                                               "c2dUpdateSurface");
    *(void **)&LINK_c2dReadSurface = ::dlsym(ctx->libc2d2,
                                             "c2dReadSurface");
    *(void **)&LINK_c2dDraw = ::dlsym(ctx->libc2d2, "c2dDraw");
    *(void **)&LINK_c2dFlush = ::dlsym(ctx->libc2d2, "c2dFlush");
    *(void **)&LINK_c2dFinish = ::dlsym(ctx->libc2d2, "c2dFinish");
    *(void **)&LINK_c2dWaitTimestamp = ::dlsym(ctx->libc2d2,
                                               "c2dWaitTimestamp");
    *(void **)&LINK_c2dDestroySurface = ::dlsym(ctx->libc2d2,
                                                "c2dDestroySurface");
    *(void **)&LINK_c2dMapAddr = ::dlsym(ctx->libc2d2,
                                         "c2dMapAddr");
    *(void **)&LINK_c2dUnMapAddr = ::dlsym(ctx->libc2d2,
                                           "c2dUnMapAddr");
    *(void **)&LINK_c2dGetDriverCapabilities = ::dlsym(ctx->libc2d2,
                                           "c2dGetDriverCapabilities");
    *(void **)&LINK_c2dCreateFenceFD = ::dlsym(ctx->libc2d2,
                                           "c2dCreateFenceFD");
    *(void **)&LINK_c2dFillSurface = ::dlsym(ctx->libc2d2,
                                           "c2dFillSurface");

    if (!LINK_c2dCreateSurface || !LINK_c2dUpdateSurface || !LINK_c2dReadSurface
        || !LINK_c2dDraw || !LINK_c2dFlush || !LINK_c2dWaitTimestamp ||
        !LINK_c2dFinish  || !LINK_c2dDestroySurface ||
        !LINK_c2dGetDriverCapabilities || !LINK_c2dCreateFenceFD ||
        !LINK_c2dFillSurface) {
        ALOGE("%s: dlsym ERROR", __FUNCTION__);
        clean_up(ctx);
        status = COPYBIT_FAILURE;
        *device = NULL;
        return status;
    }

    ctx->device.common.tag = HARDWARE_DEVICE_TAG;
    ctx->device.common.version = 1;
    ctx->device.common.module = (hw_module_t*)(module);
    ctx->device.common.close = close_copybit;
    ctx->device.set_parameter = set_parameter_copybit;
    ctx->device.get = get;
    ctx->device.blit = blit_copybit;
    ctx->device.set_sync = set_sync_copybit;
    ctx->device.stretch = stretch_copybit;
    ctx->device.finish = finish_copybit;
    ctx->device.flush_get_fence = flush_get_fence_copybit;
    ctx->device.clear = clear_copybit;
    ctx->device.fill_color = fill_color;

    /* Create RGB Surface */
    surfDefinition.buffer = (void*)0xdddddddd;
    surfDefinition.phys = (void*)0xdddddddd;
    surfDefinition.stride = 1 * 4;
    surfDefinition.width = 1;
    surfDefinition.height = 1;
    surfDefinition.format = C2D_COLOR_FORMAT_8888_ARGB;
    if (LINK_c2dCreateSurface(&(ctx->dst[RGB_SURFACE]), C2D_TARGET | C2D_SOURCE,
                              (C2D_SURFACE_TYPE)(C2D_SURFACE_RGB_HOST |
                                                 C2D_SURFACE_WITH_PHYS |
                                                 C2D_SURFACE_WITH_PHYS_DUMMY ),
                                                 &surfDefinition)) {
        ALOGE("%s: create ctx->dst_surface[RGB_SURFACE] failed", __FUNCTION__);
        ctx->dst[RGB_SURFACE] = 0;
        clean_up(ctx);
        status = COPYBIT_FAILURE;
        *device = NULL;
        return status;
    }

    unsigned int surface_id = 0;
    for (int i = 0; i < MAX_RGB_SURFACES; i++)
    {
        if (LINK_c2dCreateSurface(&surface_id, C2D_TARGET | C2D_SOURCE,
                              (C2D_SURFACE_TYPE)(C2D_SURFACE_RGB_HOST |
                                                 C2D_SURFACE_WITH_PHYS |
                                                 C2D_SURFACE_WITH_PHYS_DUMMY ),
                                                 &surfDefinition)) {
            ALOGE("%s: create RGB source surface %d failed", __FUNCTION__, i);
            ctx->blit_rgb_object[i].surface_id = 0;
            status = COPYBIT_FAILURE;
            break;
        } else {
            ctx->blit_rgb_object[i].surface_id = surface_id;
            ALOGW("%s i = %d surface_id=%d",  __FUNCTION__, i,
                                          ctx->blit_rgb_object[i].surface_id);
        }
    }

    if (status == COPYBIT_FAILURE) {
        clean_up(ctx);
        status = COPYBIT_FAILURE;
        *device = NULL;
        return status;
    }

    // Create 2 plane YUV surfaces
    yuvSurfaceDef.format = C2D_COLOR_FORMAT_420_NV12;
    yuvSurfaceDef.width = 4;
    yuvSurfaceDef.height = 4;
    yuvSurfaceDef.plane0 = (void*)0xaaaaaaaa;
    yuvSurfaceDef.phys0 = (void*) 0xaaaaaaaa;
    yuvSurfaceDef.stride0 = 4;

    yuvSurfaceDef.plane1 = (void*)0xaaaaaaaa;
    yuvSurfaceDef.phys1 = (void*) 0xaaaaaaaa;
    yuvSurfaceDef.stride1 = 4;
    if (LINK_c2dCreateSurface(&(ctx->dst[YUV_SURFACE_2_PLANES]),
                              C2D_TARGET | C2D_SOURCE,
                              (C2D_SURFACE_TYPE)(C2D_SURFACE_YUV_HOST |
                               C2D_SURFACE_WITH_PHYS |
                               C2D_SURFACE_WITH_PHYS_DUMMY),
                              &yuvSurfaceDef)) {
        ALOGE("%s: create ctx->dst[YUV_SURFACE_2_PLANES] failed", __FUNCTION__);
        ctx->dst[YUV_SURFACE_2_PLANES] = 0;
        clean_up(ctx);
        status = COPYBIT_FAILURE;
        *device = NULL;
        return status;
    }

    for (int i=0; i < MAX_YUV_2_PLANE_SURFACES; i++)
    {
        if (LINK_c2dCreateSurface(&surface_id, C2D_TARGET | C2D_SOURCE,
                              (C2D_SURFACE_TYPE)(C2D_SURFACE_YUV_HOST |
                                                 C2D_SURFACE_WITH_PHYS |
                                                 C2D_SURFACE_WITH_PHYS_DUMMY ),
                              &yuvSurfaceDef)) {
            ALOGE("%s: create YUV source %d failed", __FUNCTION__, i);
            ctx->blit_yuv_2_plane_object[i].surface_id = 0;
            status = COPYBIT_FAILURE;
            break;
        } else {
            ctx->blit_yuv_2_plane_object[i].surface_id = surface_id;
            ALOGW("%s: 2 Plane YUV i=%d surface_id=%d",  __FUNCTION__, i,
                                   ctx->blit_yuv_2_plane_object[i].surface_id);
        }
    }

    if (status == COPYBIT_FAILURE) {
        clean_up(ctx);
        status = COPYBIT_FAILURE;
        *device = NULL;
        return status;
    }

    // Create YUV 3 plane surfaces
    yuvSurfaceDef.format = C2D_COLOR_FORMAT_420_YV12;
    yuvSurfaceDef.plane2 = (void*)0xaaaaaaaa;
    yuvSurfaceDef.phys2 = (void*) 0xaaaaaaaa;
    yuvSurfaceDef.stride2 = 4;

    if (LINK_c2dCreateSurface(&(ctx->dst[YUV_SURFACE_3_PLANES]),
                              C2D_TARGET | C2D_SOURCE,
                              (C2D_SURFACE_TYPE)(C2D_SURFACE_YUV_HOST |
                                                 C2D_SURFACE_WITH_PHYS |
                                                 C2D_SURFACE_WITH_PHYS_DUMMY),
                              &yuvSurfaceDef)) {
        ALOGE("%s: create ctx->dst[YUV_SURFACE_3_PLANES] failed", __FUNCTION__);
        ctx->dst[YUV_SURFACE_3_PLANES] = 0;
        clean_up(ctx);
        status = COPYBIT_FAILURE;
        *device = NULL;
        return status;
    }

    for (int i=0; i < MAX_YUV_3_PLANE_SURFACES; i++)
    {
        if (LINK_c2dCreateSurface(&(surface_id),
                              C2D_TARGET | C2D_SOURCE,
                              (C2D_SURFACE_TYPE)(C2D_SURFACE_YUV_HOST |
                                                 C2D_SURFACE_WITH_PHYS |
                                                 C2D_SURFACE_WITH_PHYS_DUMMY),
                              &yuvSurfaceDef)) {
            ALOGE("%s: create 3 plane YUV surface %d failed", __FUNCTION__, i);
            ctx->blit_yuv_3_plane_object[i].surface_id = 0;
            status = COPYBIT_FAILURE;
            break;
        } else {
            ctx->blit_yuv_3_plane_object[i].surface_id = surface_id;
            ALOGW("%s: 3 Plane YUV i=%d surface_id=%d",  __FUNCTION__, i,
                                   ctx->blit_yuv_3_plane_object[i].surface_id);
        }
    }

    if (status == COPYBIT_FAILURE) {
        clean_up(ctx);
        status = COPYBIT_FAILURE;
        *device = NULL;
        return status;
    }

    if (LINK_c2dGetDriverCapabilities(&(ctx->c2d_driver_info))) {
         ALOGE("%s: LINK_c2dGetDriverCapabilities failed", __FUNCTION__);
         clean_up(ctx);
         status = COPYBIT_FAILURE;
        *device = NULL;
        return status;
    }
    // Initialize context variables.
    ctx->trg_transform = C2D_TARGET_ROTATE_0;

    ctx->temp_src_buffer.fd = -1;
    ctx->temp_src_buffer.base = 0;
    ctx->temp_src_buffer.size = 0;

    ctx->temp_dst_buffer.fd = -1;
    ctx->temp_dst_buffer.base = 0;
    ctx->temp_dst_buffer.size = 0;

    ctx->fb_width = 0;
    ctx->fb_height = 0;

    ctx->blit_rgb_count = 0;
    ctx->blit_yuv_2_plane_count = 0;
    ctx->blit_yuv_3_plane_count = 0;
    ctx->blit_count = 0;

    ctx->wait_timestamp = false;
    ctx->stop_thread = false;
    pthread_mutex_init(&(ctx->wait_cleanup_lock), NULL);
    pthread_cond_init(&(ctx->wait_cleanup_cond), NULL);
    /* Start the wait thread */
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    pthread_create(&ctx->wait_thread_id, &attr, &c2d_wait_loop,
                                                            (void *)ctx);
    pthread_attr_destroy(&attr);

    *device = &ctx->device.common;
    return status;
}
