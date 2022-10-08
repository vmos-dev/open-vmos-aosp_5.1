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
 * Authors:
 *    Shengquan Yuan  <shengquan.yuan@intel.com>
 *    Forrest Zhang  <forrest.zhang@intel.com>
 *
 */

#include "psb_buffer.h"

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <wsbm/wsbm_manager.h>

#include "psb_def.h"
#include "psb_drv_debug.h"

#if PSB_MFLD_DUMMY_CODE
static VAStatus psb_buffer_offset_camerav4l2(psb_driver_data_p driver_data,
        psb_buffer_p buf,
        unsigned int v4l2_buf_offset,
        unsigned int *bo_offset
                                            )
{
    *bo_offset = v4l2_buf_offset;
    return VA_STATUS_SUCCESS;
}


static VAStatus psb_buffer_offset_cameraci(psb_driver_data_p driver_data,
        psb_buffer_p buf,
        unsigned int ci_frame_offset_or_handle,
        unsigned int *bo_offset
                                          )
{
    *bo_offset = ci_frame_offset_or_handle;

    return VA_STATUS_SUCCESS;
}


static int psb_buffer_info_ci(psb_driver_data_p driver_data)
{
    struct drm_lnc_video_getparam_arg arg;
    unsigned long camera_info[2] = {0, 0};
    int ret = 0;

    driver_data->camera_phyaddr = driver_data->camera_size = 0;

    arg.key = LNC_VIDEO_GETPARAM_CI_INFO;
    arg.value = (uint64_t)((unsigned long) & camera_info[0]);
    ret = drmCommandWriteRead(driver_data->drm_fd, driver_data->getParamIoctlOffset,
                              &arg, sizeof(arg));
    if (ret == 0) {
        driver_data->camera_phyaddr = camera_info[0];
        driver_data->camera_size = camera_info[1];
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "CI region physical address = 0x%08x, size=%dK\n",
                                 driver_data->camera_phyaddr,  driver_data->camera_size / 1024);

        return ret;
    }

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "CI region get_info failed\n");
    return ret;
}

/*
 * Allocate global BO which maps camear device memory as encode MMU memory
 * the global BO shared by several encode surfaces created from camear memory
 */
static VAStatus psb_buffer_init_camera(psb_driver_data_p driver_data)
{
    int ret = 0;

    /* hasn't grab camera device memory region
     * grab the whole 4M camera device memory
     */
    driver_data->camera_bo = calloc(1, sizeof(struct psb_buffer_s));
    if (driver_data->camera_bo == NULL)
        return VA_STATUS_ERROR_ALLOCATION_FAILED;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Grab whole camera device memory\n");
    ret = psb_buffer_create(driver_data, driver_data->camera_size, psb_bt_camera, (psb_buffer_p) driver_data->camera_bo);

    if (ret != VA_STATUS_SUCCESS) {
        free(driver_data->camera_bo);
        driver_data->camera_bo = NULL;

        drv_debug_msg(VIDEO_DEBUG_ERROR, "Grab camera device memory failed\n");
    }

    return ret;
}


/*
 * Create one buffer from camera device memory
 * is_v4l2 means if the buffer is V4L2 buffer
 * id_or_ofs is CI frame ID (actually now is frame offset), or V4L2 buffer offset
 */
VAStatus psb_buffer_create_camera(psb_driver_data_p driver_data,
                                  psb_buffer_p buf,
                                  int is_v4l2,
                                  int id_or_ofs
                                 )
{
    VAStatus vaStatus;
    int ret = 0;
    unsigned int camera_offset = 0;

    if (driver_data->camera_bo  == NULL) {
        if (psb_buffer_info_ci(driver_data)) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "Can't get CI region information\n");
            return VA_STATUS_ERROR_UNKNOWN;
        }

        vaStatus = psb_buffer_init_camera(driver_data);
        if (vaStatus != VA_STATUS_SUCCESS) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "Grab camera device memory failed\n");
            return ret;
        }
    }

    /* reference the global camear BO */
    ret = psb_buffer_reference(driver_data, buf, (psb_buffer_p) driver_data->camera_bo);
    if (ret != VA_STATUS_SUCCESS) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Reference camera device memory failed\n");
        return ret;
    }

    if (is_v4l2)
        ret = psb_buffer_offset_camerav4l2(driver_data, buf, id_or_ofs, &camera_offset);
    else
        ret = psb_buffer_offset_cameraci(driver_data, buf, id_or_ofs, &camera_offset);

    buf->buffer_ofs = camera_offset;

    return ret;
}

/*
 * Create one buffer from user buffer
 * id_or_ofs is CI frame ID (actually now is frame offset), or V4L2 buffer offset
 * user_ptr :virtual address of user buffer start.
 */
VAStatus psb_buffer_create_camera_from_ub(psb_driver_data_p driver_data,
        psb_buffer_p buf,
        int id_or_ofs,
        int size,
        const unsigned long * user_ptr)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    int allignment;
    uint32_t placement;
    int ret;

    buf->rar_handle = 0;
    buf->buffer_ofs = 0;
    buf->type = psb_bt_user_buffer;
    buf->user_ptr = (unsigned char *)user_ptr;
    buf->driver_data = driver_data;

    allignment = 4096;
    placement =  DRM_PSB_FLAG_MEM_MMU | WSBM_PL_FLAG_TT | WSBM_PL_FLAG_CACHED | WSBM_PL_FLAG_SHARED ;
    ret = LOCK_HARDWARE(driver_data);
    if (ret) {
        UNLOCK_HARDWARE(driver_data);
        vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
        DEBUG_FAILURE_RET;
        return vaStatus;
    }
    ret = wsbmGenBuffers(driver_data->main_pool, 1, &buf->drm_buf,
                         allignment, placement);
    if (!buf->drm_buf) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "failed to gen wsbm buffers\n");
        UNLOCK_HARDWARE(driver_data);
        return VA_STATUS_ERROR_ALLOCATION_FAILED;
    }

    /* here use the placement when gen buffer setted */
    ret = wsbmBODataUB(buf->drm_buf, size, NULL, NULL, 0, user_ptr);
    UNLOCK_HARDWARE(driver_data);
    if (ret) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "failed to alloc wsbm buffers\n");
        return VA_STATUS_ERROR_ALLOCATION_FAILED;
    }
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Create BO from user buffer 0x%08x (%d byte),BO GPU offset hint=0x%08x\n",
                             user_ptr, size, wsbmBOOffsetHint(buf->drm_buf));


    buf->pl_flags = placement;
    buf->status = psb_bs_ready;
    buf->wsbm_synccpu_flag = 0;

    return VA_STATUS_SUCCESS;
}
#endif

#ifdef ANDROID
#ifndef BAYTRAIL
static int psb_buffer_info_rar(psb_driver_data_p driver_data)
{
    struct drm_lnc_video_getparam_arg arg;
    unsigned long rar_info[2] = {0, 0};
    int ret = 0;

    driver_data->rar_phyaddr = driver_data->rar_size = 0;

    arg.key = LNC_VIDEO_GETPARAM_IMR_INFO;
    arg.value = (uint64_t)((unsigned long) & rar_info[0]);
    ret = drmCommandWriteRead(driver_data->drm_fd, driver_data->getParamIoctlOffset,
                              &arg, sizeof(arg));
    if (ret == 0) {
        driver_data->rar_phyaddr = rar_info[0];
        driver_data->rar_size = rar_info[1];
        driver_data->rar_size = driver_data->rar_size & 0xfffff000; /* page align */
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "RAR region physical address = 0x%08x, size=%dK\n",
                                 driver_data->rar_phyaddr,  driver_data->rar_size / 1024);

        return ret;
    }

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "RAR region get size failed\n");
    return ret;
}
#endif
#endif

#ifndef BAYTRAIL
static VAStatus psb_buffer_init_imr(psb_driver_data_p driver_data)
{
    int ret = 0;

    /* hasn't grab IMR device memory region
     * grab the whole IMR3 device memory
     */
    driver_data->rar_bo = calloc(1, sizeof(struct psb_buffer_s));
    if (driver_data->rar_bo == NULL)
        goto exit_error;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Init IMR device\n");
#ifdef ANDROID
    if (psb_buffer_info_rar(driver_data)) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Get IMR region size failed\n");
        goto exit_error;
    }
#else
    drv_debug_msg(VIDEO_DEBUG_ERROR, "NON ANDROID:Get IMR region size failed\n");
    goto exit_error;
#endif
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Grab whole camera device memory\n");
    ret = psb_buffer_create(driver_data, driver_data->rar_size, psb_bt_imr, (psb_buffer_p) driver_data->rar_bo);

    if (ret != VA_STATUS_SUCCESS) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Grab IMR device memory failed\n");
        goto exit_error;
    }

    return VA_STATUS_SUCCESS;

exit_error:
    if (driver_data->rar_bo)
        free(driver_data->rar_bo);

    driver_data->rar_bo = NULL;

    return VA_STATUS_ERROR_ALLOCATION_FAILED;
}


/*
 * Reference one IMR buffer from offset
 * only used to reference a slice IMR buffer which is created outside of video driver
 */
VAStatus psb_buffer_reference_imr(psb_driver_data_p driver_data,
                                  uint32_t imr_offset,
                                  psb_buffer_p buf
                                 )
{
    VAStatus vaStatus;
    int ret;

    if (driver_data->rar_bo  == NULL) {
        vaStatus = psb_buffer_init_imr(driver_data);
        if (vaStatus != VA_STATUS_SUCCESS) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "IMR init failed!\n");
            return vaStatus;
        }
    }

    /* don't need to assign the offset to buffer
     * so that when destroy the buffer, we just
     * need to unreference
     */
    /* buf->imr_offset = imr_offset; */

    /* reference the global IMR BO */
    ret = psb_buffer_reference(driver_data, buf, (psb_buffer_p) driver_data->rar_bo);
    if (ret != VA_STATUS_SUCCESS) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Reference IMR device memory failed\n");
        return ret;
    }

    buf->rar_handle = imr_offset;
    buf->buffer_ofs = imr_offset;

    /* reference the global IMR buffer, reset buffer type */
    buf->type = psb_bt_imr_slice; /* don't need to IMR_release */

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Reference IMR buffer, IMR region offset =0x%08x, IMR BO GPU offset hint=0x%08x\n",
                             imr_offset, wsbmBOOffsetHint(buf->drm_buf));

    return VA_STATUS_SUCCESS;
}
#endif
