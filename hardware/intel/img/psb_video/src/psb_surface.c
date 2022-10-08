/*
 * Copyright (c) 2011 Intel Corporation. All Rights Reserved.
 * Copyright (c) Imagination Technologies Limited, UK
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
 * Authors:
 *    Waldo Bastian <waldo.bastian@intel.com>
 *
 */

#include <wsbm/wsbm_manager.h>

#include "psb_def.h"
#include "psb_surface.h"
#include "psb_drv_debug.h"

/*
 * Create surface
 */
VAStatus psb_surface_create(psb_driver_data_p driver_data,
                            int width, int height, int fourcc, unsigned int flags,
                            psb_surface_p psb_surface /* out */
                           )
{
    int ret = 0;
    int buffer_type = psb_bt_surface;

#ifndef BAYTRAIL
    if ((flags & IS_ROTATED) || (driver_data->render_mode & VA_RENDER_MODE_LOCAL_OVERLAY))
        buffer_type = psb_bt_surface_tt;
#endif

#ifdef PSBVIDEO_MSVDX_DEC_TILING
    int tiling = GET_SURFACE_INFO_tiling(psb_surface);
    if (tiling)
        buffer_type = psb_bt_surface_tiling;
#endif

    if (fourcc == VA_FOURCC_NV12) {
        if ((width <= 0) || (width * height > 5120 * 5120) || (height <= 0)) {
            return VA_STATUS_ERROR_ALLOCATION_FAILED;
        }

        if (0) {
            ;
        } else if (512 >= width) {
            psb_surface->stride_mode = STRIDE_512;
            psb_surface->stride = 512;
        } else if (1024 >= width) {
            psb_surface->stride_mode = STRIDE_1024;
            psb_surface->stride = 1024;
        } else if (1280 >= width) {
            psb_surface->stride_mode = STRIDE_1280;
            psb_surface->stride = 1280;
#ifdef PSBVIDEO_MSVDX_DEC_TILING
            if (tiling) {
                psb_surface->stride_mode = STRIDE_2048;
                psb_surface->stride = 2048;
            }
#endif
        } else if (2048 >= width) {
            psb_surface->stride_mode = STRIDE_2048;
            psb_surface->stride = 2048;
        } else if (4096 >= width) {
            psb_surface->stride_mode = STRIDE_4096;
            psb_surface->stride = 4096;
        } else {
            psb_surface->stride_mode = STRIDE_NA;
            psb_surface->stride = (width + 0x3f) & ~0x3f;
        }

        psb_surface->luma_offset = 0;
        psb_surface->chroma_offset = psb_surface->stride * height;
        psb_surface->size = (psb_surface->stride * height * 3) / 2;
        psb_surface->extra_info[4] = VA_FOURCC_NV12;
    } else if (fourcc == VA_FOURCC_RGBA) {
        unsigned int pitchAlignMask = 63;
        psb_surface->stride_mode = STRIDE_NA;
        psb_surface->stride = (width * 4 + pitchAlignMask) & ~pitchAlignMask;;
        psb_surface->luma_offset = 0;
        psb_surface->chroma_offset = 0;
        psb_surface->size = psb_surface->stride * height;
        psb_surface->extra_info[4] = VA_FOURCC_RGBA;
    } else if (fourcc == VA_FOURCC_YV16) {
        if ((width <= 0) || (width * height > 5120 * 5120) || (height <= 0)) {
            return VA_STATUS_ERROR_ALLOCATION_FAILED;
        }

        if (0) {
            ;
        } else if (512 >= width) {
            psb_surface->stride_mode = STRIDE_512;
            psb_surface->stride = 512;
        } else if (1024 >= width) {
            psb_surface->stride_mode = STRIDE_1024;
            psb_surface->stride = 1024;
        } else if (1280 >= width) {
            psb_surface->stride_mode = STRIDE_1280;
            psb_surface->stride = 1280;
#ifdef PSBVIDEO_MSVDX_DEC_TILING
            if (tiling) {
                psb_surface->stride_mode = STRIDE_2048;
                psb_surface->stride = 2048;
            }
#endif
        } else if (2048 >= width) {
            psb_surface->stride_mode = STRIDE_2048;
            psb_surface->stride = 2048;
        } else if (4096 >= width) {
            psb_surface->stride_mode = STRIDE_4096;
            psb_surface->stride = 4096;
        } else {
            psb_surface->stride_mode = STRIDE_NA;
            psb_surface->stride = (width + 0x3f) & ~0x3f;
        }

        psb_surface->luma_offset = 0;
        psb_surface->chroma_offset = psb_surface->stride * height;
        psb_surface->size = psb_surface->stride * height * 2;
        psb_surface->extra_info[4] = VA_FOURCC_YV16;
    } else if (fourcc == VA_FOURCC_YV32) {
        psb_surface->stride_mode = STRIDE_NA;
        psb_surface->stride = (width + 0x3f) & ~0x3f; /*round up to 16 */
        psb_surface->luma_offset = 0;
        psb_surface->chroma_offset = psb_surface->stride * height;
        psb_surface->size = psb_surface->stride * height * 4;
        psb_surface->extra_info[4] = VA_FOURCC_YV32;
    }

    if (flags & IS_PROTECTED)
        SET_SURFACE_INFO_protect(psb_surface, 1);

    ret = psb_buffer_create(driver_data, psb_surface->size, buffer_type, &psb_surface->buf);

    return ret ? VA_STATUS_ERROR_ALLOCATION_FAILED : VA_STATUS_SUCCESS;
}



VAStatus psb_surface_create_for_userptr(
    psb_driver_data_p driver_data,
    int width, int height,
    unsigned size, /* total buffer size need to be allocated */
    unsigned int __maybe_unused fourcc, /* expected fourcc */
    unsigned int luma_stride, /* luma stride, could be width aligned with a special value */
    unsigned int __maybe_unused chroma_u_stride, /* chroma stride */
    unsigned int __maybe_unused chroma_v_stride,
    unsigned int luma_offset, /* could be 0 */
    unsigned int chroma_u_offset, /* UV offset from the beginning of the memory */
    unsigned int __maybe_unused chroma_v_offset,
    psb_surface_p psb_surface /* out */
)
{
    int ret;

    if ((width <= 0) || (width > 5120) || (height <= 0) || (height > 5120))
        return VA_STATUS_ERROR_ALLOCATION_FAILED;

    psb_surface->stride_mode = STRIDE_NA;
    psb_surface->stride = luma_stride;


    psb_surface->luma_offset = luma_offset;
    psb_surface->chroma_offset = chroma_u_offset;
    psb_surface->size = size;
    psb_surface->extra_info[4] = VA_FOURCC_NV12;
    psb_surface->extra_info[8] = VA_FOURCC_NV12;

    ret = psb_buffer_create(driver_data, psb_surface->size, psb_bt_cpu_vpu_shared, &psb_surface->buf);

    return ret ? VA_STATUS_ERROR_ALLOCATION_FAILED : VA_STATUS_SUCCESS;
}

VAStatus psb_surface_create_from_kbuf(
    psb_driver_data_p driver_data,
    int width, int height,
    unsigned size, /* total buffer size need to be allocated */
    unsigned int __maybe_unused fourcc, /* expected fourcc */
    int __maybe_unused kbuf_handle,
    unsigned int luma_stride, /* luma stride, could be width aligned with a special value */
    unsigned int __maybe_unused chroma_u_stride, /* chroma stride */
    unsigned int __maybe_unused chroma_v_stride,
    unsigned int __maybe_unused luma_offset, /* could be 0 */
    unsigned int chroma_u_offset, /* UV offset from the beginning of the memory */
    unsigned int __maybe_unused chroma_v_offset,
    psb_surface_p psb_surface /* out */
)
{
    int ret;

    if ((width <= 0) || (width > 5120) || (height <= 0) || (height > 5120))
        return VA_STATUS_ERROR_ALLOCATION_FAILED;

    psb_surface->stride = luma_stride;

    if (0) {
        ;
    } else if (512 == luma_stride) {
        psb_surface->stride_mode = STRIDE_512;
    } else if (1024 == luma_stride) {
        psb_surface->stride_mode = STRIDE_1024;
    } else if (1280 == luma_stride) {
        psb_surface->stride_mode = STRIDE_1280;
    } else if (2048 == luma_stride) {
        psb_surface->stride_mode = STRIDE_2048;
    } else if (4096 == luma_stride) {
        psb_surface->stride_mode = STRIDE_4096;
    } else {
        psb_surface->stride_mode = STRIDE_NA;
    }

    psb_surface->luma_offset = luma_offset;
    psb_surface->chroma_offset = chroma_u_offset;
    psb_surface->size = size;
    psb_surface->extra_info[4] = VA_FOURCC_NV12;
    psb_surface->extra_info[8] = VA_FOURCC_NV12;

    ret = psb_kbuffer_reference(driver_data, &psb_surface->buf, kbuf_handle);

    return ret ? VA_STATUS_ERROR_ALLOCATION_FAILED : VA_STATUS_SUCCESS;
}

#if PSB_MFLD_DUMMY_CODE
/* id_or_ofs: it is frame ID or frame offset in camear device memory
 *     for CI frame: it it always frame offset currently
 *     for v4l2 buf: it is offset used in V4L2 buffer mmap
 */
VAStatus psb_surface_create_camera(psb_driver_data_p driver_data,
                                   int width, int height, int stride, int size,
                                   psb_surface_p psb_surface, /* out */
                                   int is_v4l2,
                                   unsigned int id_or_ofs
                                  )
{
    int ret;

    if ((width <= 0) || (width > 4096) || (height <= 0) || (height > 4096)) {
        return VA_STATUS_ERROR_ALLOCATION_FAILED;
    }

    psb_surface->stride = stride;
    if ((width == 640) && (height == 360)) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "CI Frame is 640x360, and allocated as 640x368,adjust chroma_offset\n");
        psb_surface->chroma_offset = psb_surface->stride * 368;
    } else
        psb_surface->chroma_offset = psb_surface->stride * height;
    psb_surface->size = (psb_surface->stride * height * 3) / 2;

    ret = psb_buffer_create_camera(driver_data, &psb_surface->buf,
                                   is_v4l2, id_or_ofs);

    if (ret != VA_STATUS_SUCCESS) {
        psb_surface_destroy(psb_surface);

        drv_debug_msg(VIDEO_DEBUG_ERROR, "Get surfae offset of camear device memory failed!\n");
        return ret;
    }

    return VA_STATUS_SUCCESS;
}

/* id_or_ofs: it is frame ID or frame offset in camear device memory
 *     for CI frame: it it always frame offset currently
 *     for v4l2 buf: it is offset used in V4L2 buffer mmap
 * user_ptr: virtual address of user buffer.
 */
VAStatus psb_surface_create_camera_from_ub(psb_driver_data_p driver_data,
        int width, int height, int stride, int size,
        psb_surface_p psb_surface, /* out */
        int is_v4l2,
        unsigned int id_or_ofs,
        const unsigned long *user_ptr)
{
    int ret;

    if ((width <= 0) || (width > 4096) || (height <= 0) || (height > 4096)) {
        return VA_STATUS_ERROR_ALLOCATION_FAILED;
    }

    psb_surface->stride = stride;
    psb_surface->chroma_offset = psb_surface->stride * height;
    psb_surface->size = (psb_surface->stride * height * 3) / 2;

    ret = psb_buffer_create_camera_from_ub(driver_data, &psb_surface->buf,
                                           is_v4l2, psb_surface->size, user_ptr);

    if (ret != VA_STATUS_SUCCESS) {
        psb_surface_destroy(psb_surface);

        drv_debug_msg(VIDEO_DEBUG_ERROR, "Get surfae offset of camear device memory failed!\n");
        return ret;
    }

    return VA_STATUS_SUCCESS;
}
#endif

/*
 * Temporarily map surface and set all chroma values of surface to 'chroma'
 */
VAStatus psb_surface_set_chroma(psb_surface_p psb_surface, int chroma)
{
    unsigned char *surface_data;
    int ret = psb_buffer_map(&psb_surface->buf, &surface_data);

    if (ret) return VA_STATUS_ERROR_UNKNOWN;

    memset(surface_data + psb_surface->chroma_offset, chroma, psb_surface->size - psb_surface->chroma_offset);

    psb_buffer_unmap(&psb_surface->buf);

    return VA_STATUS_SUCCESS;
}

/*
 * Destroy surface
 */
void psb_surface_destroy(psb_surface_p psb_surface)
{
    psb_buffer_destroy(&psb_surface->buf);
    if (NULL != psb_surface->in_loop_buf)
        psb_buffer_destroy(psb_surface->in_loop_buf);

}

VAStatus psb_surface_sync(psb_surface_p psb_surface)
{
    wsbmBOWaitIdle(psb_surface->buf.drm_buf, 0);

    return VA_STATUS_SUCCESS;
}

VAStatus psb_surface_query_status(psb_surface_p psb_surface, VASurfaceStatus *status)
{
    int ret;
    uint32_t synccpu_flag = WSBM_SYNCCPU_READ | WSBM_SYNCCPU_WRITE | WSBM_SYNCCPU_DONT_BLOCK;

    ret = wsbmBOSyncForCpu(psb_surface->buf.drm_buf, synccpu_flag);

    if (ret == 0) {
        (void) wsbmBOReleaseFromCpu(psb_surface->buf.drm_buf, synccpu_flag);
        *status = VASurfaceReady;
    } else {
        *status = VASurfaceRendering;
    }
    return VA_STATUS_SUCCESS;
}

/*
 * Set current displaying surface info to kernel
 * so that other component can access it in another process
 */
int psb_surface_set_displaying(psb_driver_data_p driver_data,
                               int width, int height,
                               psb_surface_p psb_surface)
{
    struct drm_lnc_video_getparam_arg arg;
    struct drm_video_displaying_frameinfo value;
    int ret = 0;

    if (psb_surface) {
        value.buf_handle = (uint32_t)(wsbmKBufHandle(wsbmKBuf(psb_surface->buf.drm_buf)));
        value.width = width;
        value.height = height;
        value.size = psb_surface->size;
        value.format = psb_surface->extra_info[4];
        value.luma_stride = psb_surface->stride;
        value.chroma_u_stride = psb_surface->stride;
        value.chroma_v_stride = psb_surface->stride;
        value.luma_offset = psb_surface->luma_offset;
        value.chroma_u_offset = psb_surface->chroma_offset;
        value.chroma_v_offset = psb_surface->chroma_offset;
    } else /* clean kernel displaying surface info */
        memset(&value, 0, sizeof(value));

    arg.key = IMG_VIDEO_SET_DISPLAYING_FRAME;
    arg.value = (uint64_t)((unsigned long) & value);
    ret = drmCommandWriteRead(driver_data->drm_fd, driver_data->getParamIoctlOffset,
                              &arg, sizeof(arg));
    if (ret != 0)
        drv_debug_msg(VIDEO_DEBUG_ERROR, "IMG_VIDEO_SET_DISPLAYING_FRAME failed\n");

    return ret;
}
