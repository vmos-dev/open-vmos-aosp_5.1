/*
 * Copyright (c) 2011 Intel Corporation. All Rights Reserved.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <sys/mman.h>
#include <va/va_tpi.h>
#include "psb_drv_video.h"
#include "psb_drv_debug.h"
#include "psb_surface.h"
#include "psb_surface_attrib.h"

#include <gralloc.h>
#include "android/psb_gralloc.h"
#include "android/psb_android_glue.h"
#ifndef BAYTRAIL
#include <hal/hal_public.h>
#endif
#include <wsbm/wsbm_manager.h>

#define INIT_DRIVER_DATA    psb_driver_data_p driver_data = (psb_driver_data_p) ctx->pDriverData;
#define CONFIG(id)  ((object_config_p) object_heap_lookup( &driver_data->config_heap, id ))
#define CONTEXT(id) ((object_context_p) object_heap_lookup( &driver_data->context_heap, id ))
#define SURFACE(id)    ((object_surface_p) object_heap_lookup( &driver_data->surface_heap, id ))
#define BUFFER(id)  ((object_buffer_p) object_heap_lookup( &driver_data->buffer_heap, id ))
#define SHARE_INFO_INIT_VALUE   0x12345678

static pthread_mutex_t gralloc_mutex = PTHREAD_MUTEX_INITIALIZER;

/*FIXME: include hal_public.h instead of define it here*/
enum {
    GRALLOC_SUB_BUFFER0 = 0,
    GRALLOC_SUB_BUFFER1,
    GRALLOC_SUB_BUFFER2,
    GRALLOC_SUB_BUFFER_MAX,
};

VAStatus psb_DestroySurfaceGralloc(object_surface_p obj_surface)
{
    void *vaddr[GRALLOC_SUB_BUFFER_MAX];
    int usage = GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_COMPOSER;
    buffer_handle_t handle = obj_surface->psb_surface->buf.handle;

#ifdef PSBVIDEO_MRFL
    usage |= GRALLOC_USAGE_SW_WRITE_OFTEN;
#endif

    pthread_mutex_lock(&gralloc_mutex);
    if (!gralloc_lock(handle, usage, 0, 0,
                      obj_surface->width, obj_surface->height, (void **)&vaddr[GRALLOC_SUB_BUFFER0])){
        if (obj_surface->share_info && vaddr[GRALLOC_SUB_BUFFER1] == obj_surface->share_info) {
            int metadata_rotate = obj_surface->share_info->metadata_rotate;
            int surface_protected = obj_surface->share_info->surface_protected;
            int force_output_method = obj_surface->share_info->force_output_method;
            int bob_deinterlace = obj_surface->share_info->bob_deinterlace;

            memset(obj_surface->share_info, 0, sizeof(struct psb_surface_share_info_s));
            /* Still need to keep these info so that hwc can get them after suspend/resume cycle */
            obj_surface->share_info->metadata_rotate = metadata_rotate;
            obj_surface->share_info->surface_protected = surface_protected;
            obj_surface->share_info->force_output_method = force_output_method;
            obj_surface->share_info->bob_deinterlace = bob_deinterlace;
        }
        gralloc_unlock(handle);
    }
    pthread_mutex_unlock(&gralloc_mutex);

    return VA_STATUS_SUCCESS;
}

#ifdef BAYTRAIL
VAStatus psb_CreateSurfacesFromGralloc(
    VADriverContextP ctx,
    int width,
    int height,
    int format,
    int num_surfaces,
    VASurfaceID *surface_list,        /* out */
    PsbSurfaceAttributeTPI *attribute_tpi
)
{
    INIT_DRIVER_DATA
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    int i, height_origin, usage, buffer_stride = 0;
    int protected = (VA_RT_FORMAT_PROTECTED & format);
    unsigned long fourcc;
    VASurfaceAttributeTPI *external_buffers = NULL;
    unsigned long handle;
    int size = num_surfaces * sizeof(unsigned int);
    void *vaddr;


    /* follow are gralloc-buffers */
    format = format & (~VA_RT_FORMAT_PROTECTED);
    driver_data->protected = protected;

    CHECK_INVALID_PARAM(num_surfaces <= 0);
    CHECK_SURFACE(surface_list);

    external_buffers = attribute_tpi;

    ALOGD("format is 0x%x, width is %d, height is %d, num_surfaces is %d.\n", format, width, height, num_surfaces);
    /* We only support one format */
    if ((VA_RT_FORMAT_YUV420 != format)
        && (VA_RT_FORMAT_YUV422 != format)) {
        vaStatus = VA_STATUS_ERROR_UNSUPPORTED_RT_FORMAT;
        DEBUG_FAILURE;
        return vaStatus;
    }

    CHECK_INVALID_PARAM(external_buffers == NULL);

    /*
    vaStatus = psb__checkSurfaceDimensions(driver_data, width, height);
    CHECK_VASTATUS();
    */
    /* Adjust height to be a multiple of 32 (height of macroblock in interlaced mode) */
    height_origin = height;
    height = (height + 0x1f) & ~0x1f;
    ALOGD("external_buffers->pixel_format is 0x%x.\n", external_buffers->pixel_format);
    /* get native window from the reserved field */
    driver_data->native_window = (void *)external_buffers->reserved[0];

    for (i = 0; i < num_surfaces; i++) {
        int surfaceID;
        object_surface_p obj_surface;
        psb_surface_p psb_surface;

        surfaceID = object_heap_allocate(&driver_data->surface_heap);
        obj_surface = SURFACE(surfaceID);
        if (NULL == obj_surface) {
            vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
            DEBUG_FAILURE;
            break;
        }
        MEMSET_OBJECT(obj_surface, struct object_surface_s);

        obj_surface->surface_id = surfaceID;
        surface_list[i] = surfaceID;
        obj_surface->context_id = -1;
        obj_surface->width = width;
        obj_surface->height = height;
        obj_surface->width_r = width;
        obj_surface->height_r = height;
        obj_surface->height_origin = height_origin;
        obj_surface->is_ref_surface = 0;

        psb_surface = (psb_surface_p) calloc(1, sizeof(struct psb_surface_s));
        if (NULL == psb_surface) {
            object_heap_free(&driver_data->surface_heap, (object_base_p) obj_surface);
            obj_surface->surface_id = VA_INVALID_SURFACE;

            vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;

            DEBUG_FAILURE;
            break;
        }

        switch (format) {
        case VA_RT_FORMAT_YUV422:
            fourcc = VA_FOURCC_YV16;
            break;
        case VA_RT_FORMAT_YUV420:
        default:
            fourcc = VA_FOURCC_NV12;
            break;
        }

        /*hard code the gralloc buffer usage*/
        usage = GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_COMPOSER;

        /* usage hack for byt */
        usage |= GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN;
        /* usage hack to force pages alloc and CPU/GPU cache flush */
        usage |= GRALLOC_USAGE_HW_VIDEO_ENCODER;

        handle = (unsigned long)external_buffers->buffers[i];
        pthread_mutex_lock(&gralloc_mutex);
        if (gralloc_lock(handle, usage, 0, 0, width, height, (void **)&vaddr)) {
            vaStatus = VA_STATUS_ERROR_UNKNOWN;
        } else {
            int cache_flag = PSB_USER_BUFFER_UNCACHED;

            vaStatus = psb_surface_create_from_ub(driver_data, width, height, fourcc,
                    external_buffers, psb_surface, vaddr,
                    cache_flag);

            psb_surface->buf.handle = handle;
            obj_surface->share_info = NULL;
            gralloc_unlock(handle);
        }
        pthread_mutex_unlock(&gralloc_mutex);

        if (VA_STATUS_SUCCESS != vaStatus) {
            free(psb_surface);
            object_heap_free(&driver_data->surface_heap, (object_base_p) obj_surface);
            obj_surface->surface_id = VA_INVALID_SURFACE;

            DEBUG_FAILURE;
            break;
        }
        buffer_stride = psb_surface->stride;
#ifdef PSBVIDEO_MSVDX_DEC_TILING
        psb_surface->extra_info[7] = external_buffers->tiling;
#endif
        /* by default, surface fourcc is NV12 */
        psb_surface->extra_info[4] = fourcc;
        obj_surface->psb_surface = psb_surface;
    }

    /* Error recovery */
    if (VA_STATUS_SUCCESS != vaStatus) {
        /* surface_list[i-1] was the last successful allocation */
        for (; i--;) {
            object_surface_p obj_surface = SURFACE(surface_list[i]);
            psb__destroy_surface(driver_data, obj_surface);
            surface_list[i] = VA_INVALID_SURFACE;
        }
        drv_debug_msg(VIDEO_DEBUG_ERROR, "CreateSurfaces failed\n");

        return vaStatus;
    }

    return vaStatus;
}
#else
VAStatus psb_CreateSurfacesFromGralloc(
    VADriverContextP ctx,
    int width,
    int height,
    int format,
    int num_surfaces,
    VASurfaceID *surface_list,        /* out */
    PsbSurfaceAttributeTPI *attribute_tpi
)
{
    INIT_DRIVER_DATA
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    int i, height_origin, usage, buffer_stride = 0;
    int protected = (VA_RT_FORMAT_PROTECTED & format);
    unsigned long fourcc;
    PsbSurfaceAttributeTPI *external_buffers = NULL;
    unsigned long handle;
    int size = num_surfaces * sizeof(unsigned int);
    void *vaddr[GRALLOC_SUB_BUFFER_MAX];
    unsigned char * surface_data = NULL;


    /* follow are gralloc-buffers */
    format = format & (~VA_RT_FORMAT_PROTECTED);
    driver_data->protected = protected;

    CHECK_INVALID_PARAM(num_surfaces <= 0);
    CHECK_SURFACE(surface_list);

    external_buffers = attribute_tpi;

    /* We only support one format */
    if ((VA_RT_FORMAT_YUV420 != format)
        && (VA_RT_FORMAT_YUV422 != format)
        && (VA_RT_FORMAT_RGB32 != format)) {
        vaStatus = VA_STATUS_ERROR_UNSUPPORTED_RT_FORMAT;
        DEBUG_FAILURE;
        return vaStatus;
    }

    CHECK_INVALID_PARAM(external_buffers == NULL);

    /*
    vaStatus = psb__checkSurfaceDimensions(driver_data, width, height);
    CHECK_VASTATUS();
    */
    /* Adjust height to be a multiple of 32 (height of macroblock in interlaced mode) */
    height_origin = height;

    IMG_native_handle_t* h = (IMG_native_handle_t*)external_buffers->buffers[0];
    int gfx_colorformat = h->iFormat;

    if (gfx_colorformat != HAL_PIXEL_FORMAT_NV12 && format != VA_RT_FORMAT_RGB32)
        height = (height + 0x1f) & ~0x1f;

    /* get native window from the reserved field */
    driver_data->native_window = (void *)external_buffers->reserved[0];

    for (i = 0; i < num_surfaces; i++) {
        int surfaceID;
        object_surface_p obj_surface;
        psb_surface_p psb_surface;

        surfaceID = object_heap_allocate(&driver_data->surface_heap);
        obj_surface = SURFACE(surfaceID);
        if (NULL == obj_surface) {
            vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
            DEBUG_FAILURE;
            break;
        }
        MEMSET_OBJECT(obj_surface, struct object_surface_s);

        obj_surface->surface_id = surfaceID;
        surface_list[i] = surfaceID;
        obj_surface->context_id = -1;
        obj_surface->width = width;
        obj_surface->height = height;
        obj_surface->width_r = width;
        obj_surface->height_r = height;
        obj_surface->height_origin = height_origin;

        psb_surface = (psb_surface_p) calloc(1, sizeof(struct psb_surface_s));
        if (NULL == psb_surface) {
            object_heap_free(&driver_data->surface_heap, (object_base_p) obj_surface);
            obj_surface->surface_id = VA_INVALID_SURFACE;

            vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;

            DEBUG_FAILURE;
            break;
        }

        switch (format) {
        case VA_RT_FORMAT_YUV422:
            fourcc = VA_FOURCC_YV16;
            break;
        case VA_RT_FORMAT_RGB32:
            fourcc = VA_FOURCC_RGBA;
            break;
        case VA_RT_FORMAT_YUV420:
        default:
            fourcc = VA_FOURCC_NV12;
            break;
        }

#ifndef PSBVIDEO_MSVDX_DEC_TILING
        external_buffers->tiling = 0;
#endif
        /*hard code the gralloc buffer usage*/
        usage = GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_COMPOSER;

        if (gfx_colorformat == HAL_PIXEL_FORMAT_NV12)
            usage |= GRALLOC_USAGE_SW_READ_OFTEN;
        else {
#ifdef PSBVIDEO_MRFL
            usage |= GRALLOC_USAGE_SW_WRITE_OFTEN;
#endif
        }

        handle = (unsigned long)external_buffers->buffers[i];
        pthread_mutex_lock(&gralloc_mutex);
        if (gralloc_lock((buffer_handle_t)handle, usage, 0, 0, width, height, (void **)&vaddr[GRALLOC_SUB_BUFFER0])) {
            vaStatus = VA_STATUS_ERROR_UNKNOWN;
        } else {
            int cache_flag = PSB_USER_BUFFER_UNCACHED;
#ifdef PSBVIDEO_MRFL
            //cache_flag = 0;
#endif
            vaStatus = psb_surface_create_from_ub(driver_data, width, height, fourcc,
                    (VASurfaceAttributeTPI *)external_buffers, psb_surface,
                    vaddr[GRALLOC_SUB_BUFFER0], cache_flag);
            psb_surface->buf.handle = (void *)handle;
            obj_surface->share_info = NULL;

            if ((gfx_colorformat != HAL_PIXEL_FORMAT_NV12) &&
                (gfx_colorformat != HAL_PIXEL_FORMAT_YV12) &&
                (format != VA_RT_FORMAT_RGB32)) {

                unsigned int decoder_share_info = (unsigned int)external_buffers->reserved[2];
                drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s : Create graphic buffer initialized share info %d",__FUNCTION__, decoder_share_info);
                obj_surface->share_info = (psb_surface_share_info_t *)vaddr[GRALLOC_SUB_BUFFER1];

                if (obj_surface->share_info->initialized != SHARE_INFO_INIT_VALUE) {
                    memset(obj_surface->share_info, 0, sizeof(struct psb_surface_share_info_s));
                    // Set clear video the default output method as OUTPUT_FORCE_OVERLAY_FOR_SW_DECODE
                    // if the video can be decoded by HW, will reset the output method as 0 in psb_BeginPicture
#ifdef PSBVIDEO_MSVDX_DEC_TILING
                    obj_surface->share_info->tiling = external_buffers->tiling;
#endif
                    obj_surface->share_info->width = obj_surface->width;
                    obj_surface->share_info->height = obj_surface->height_origin;

                    obj_surface->share_info->luma_stride = psb_surface->stride;
                    obj_surface->share_info->chroma_u_stride = psb_surface->stride;
                    obj_surface->share_info->chroma_v_stride = psb_surface->stride;
                    obj_surface->share_info->format = VA_FOURCC_NV12;

                    obj_surface->share_info->khandle = (uint32_t)(wsbmKBufHandle(wsbmKBuf(psb_surface->buf.drm_buf)));

                    obj_surface->share_info->initialized = SHARE_INFO_INIT_VALUE;
                }

                if (decoder_share_info) {
                    obj_surface->share_info->force_output_method = protected ? OUTPUT_FORCE_OVERLAY : OUTPUT_FORCE_OVERLAY_FOR_SW_DECODE;
                    obj_surface->share_info->native_window = (void *)external_buffers->reserved[0];

                    attribute_tpi->reserved[1] = (unsigned long)obj_surface->share_info;

                    obj_surface->share_info->surface_protected = driver_data->protected;
                    if (driver_data->render_rect.width == 0 || driver_data->render_rect.height == 0) {
                        obj_surface->share_info->crop_width = obj_surface->share_info->width;
                        obj_surface->share_info->crop_height = obj_surface->share_info->height;
                    } else {
                        obj_surface->share_info->crop_width = driver_data->render_rect.width;
                        obj_surface->share_info->crop_height = driver_data->render_rect.height;
                    }

                    if (obj_surface->share_info->coded_width == 0 || obj_surface->share_info->coded_height == 0) {
                        obj_surface->share_info->coded_width = (obj_surface->share_info->width + 0xf) & ~0xf;
                        obj_surface->share_info->coded_height = (obj_surface->share_info->height + 0xf) & ~0xf;
                    }

                    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s : Create graphic buffer success"
                            "surface_id= 0x%x, vaddr[0] (0x%x), vaddr[1] (0x%x)\n",
                            __FUNCTION__, surfaceID, vaddr[GRALLOC_SUB_BUFFER0], vaddr[GRALLOC_SUB_BUFFER1]);
                }
            }
            gralloc_unlock((buffer_handle_t)handle);
            psb_surface->buf.user_ptr = NULL;

            if (psb_buffer_map(&psb_surface->buf, &surface_data)) {
                drv_debug_msg(VIDEO_DEBUG_ERROR, "Failed to map rotation buffer before clear it");
            }
            else {
                size = psb_surface->chroma_offset;
                memset(surface_data, 0, size);
                memset(surface_data + size, 0x80, psb_surface->size - size);
                psb_buffer_unmap(&psb_surface->buf);
            }
        }
        pthread_mutex_unlock(&gralloc_mutex);

        if (VA_STATUS_SUCCESS != vaStatus) {
            free(psb_surface);
            object_heap_free(&driver_data->surface_heap, (object_base_p) obj_surface);
            obj_surface->surface_id = VA_INVALID_SURFACE;

            DEBUG_FAILURE;
            break;
        }
        buffer_stride = psb_surface->stride;
        /* by default, surface fourcc is NV12 */
        psb_surface->extra_info[4] = fourcc;
        /* save the pixel format set by application */
        psb_surface->extra_info[8] = external_buffers->pixel_format;
#ifdef PSBVIDEO_MSVDX_DEC_TILING
        psb_surface->extra_info[7] = external_buffers->tiling;
#endif
        obj_surface->psb_surface = psb_surface;
    }

    /* Error recovery */
    if (VA_STATUS_SUCCESS != vaStatus) {
        /* surface_list[i-1] was the last successful allocation */
        for (; i--;) {
            object_surface_p obj_surface = SURFACE(surface_list[i]);
            psb__destroy_surface(driver_data, obj_surface);
            surface_list[i] = VA_INVALID_SURFACE;
        }
        drv_debug_msg(VIDEO_DEBUG_ERROR, "CreateSurfaces failed\n");

        return vaStatus;
    }

    return vaStatus;
}

#endif
