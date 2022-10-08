/*
 * Copyright (C) 2008 The Android Open Source Project
 * Copyright@ Samsung Electronics Co. LTD
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

/*!
 * \file      exynos_rotator.c
 * \brief     source file for exynos_rotator HAL
 * \author    Sunmi Lee (carrotsm.lee@samsung.com)
 * \date      2012/03/05
 *
 * <b>Revision History: </b>
 * - 2012/03/05 : Sunmi Lee (carrotsm.lee@samsung.com) \n
 *   Create
 *
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "libexynosrotator"
#include <cutils/log.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "exynos_rotator.h"

#include "exynos_format.h"
#include "ExynosMutex.h"
#include "exynos_v4l2.h"

#define NUM_OF_ROTATOR_PLANES           (3)
#define NODE_NUM_ROTATOR                (21)
#define PFX_NODE_ROTATOR                "/dev/video"

#define ROTATOR_MIN_W_SIZE (8)
#define ROTATOR_MIN_H_SIZE (8)

#define MAX_ROTATOR_WAITING_TIME_FOR_TRYLOCK (16000) // 16msec
#define ROTATOR_WAITING_TIME_FOR_TRYLOCK      (8000) //  8msec

struct rotator_info {
    unsigned int       width;
    unsigned int       height;
    unsigned int       crop_left;
    unsigned int       crop_top;
    unsigned int       crop_width;
    unsigned int       crop_height;
    unsigned int       v4l2_colorformat;
    unsigned int       cacheable;

    int                rotation;

    void              *addr[NUM_OF_ROTATOR_PLANES];
    bool               stream_on;

    enum v4l2_buf_type buf_type;
    struct v4l2_format format;
    struct v4l2_buffer buffer;
    struct v4l2_plane  planes[NUM_OF_ROTATOR_PLANES];
    struct v4l2_crop   crop;
};

struct ROTATOR_HANDLE {
    int              rotator_fd;
    struct rotator_info  src;
    struct rotator_info  dst;
    void                *op_mutex;
    void                *obj_mutex;
    void                *cur_obj_mutex;
};

static unsigned int m_rotator_get_plane_count(
    int v4l_pixel_format)
{
    int plane_count = 0;

    switch (v4l_pixel_format) {
    case V4L2_PIX_FMT_RGB32:
    case V4L2_PIX_FMT_BGR32:
    case V4L2_PIX_FMT_RGB24:
    case V4L2_PIX_FMT_RGB565:
    case V4L2_PIX_FMT_RGB555X:
    case V4L2_PIX_FMT_RGB444:
        plane_count = 1;
        break;
    case V4L2_PIX_FMT_NV12M:
    case V4L2_PIX_FMT_NV12MT_16X16:
    case V4L2_PIX_FMT_NV21:
    case V4L2_PIX_FMT_NV16:
    case V4L2_PIX_FMT_NV61:
        plane_count = 2;
        break;
    case V4L2_PIX_FMT_YVU420M:
    case V4L2_PIX_FMT_YUV422P:
    case V4L2_PIX_FMT_YUYV:
    case V4L2_PIX_FMT_UYVY:
        plane_count = 3;
        break;
    default:
        ALOGE("%s::unmatched v4l_pixel_format color_space(0x%x)",
             __func__, v4l_pixel_format);
        plane_count = -1;
        break;
    }

    return plane_count;
}

static unsigned int m_rotator_get_plane_size(
    unsigned int *plane_size,
    unsigned int  width,
    unsigned int  height,
    int           v4l_pixel_format)
{
    switch (v4l_pixel_format) {
    case V4L2_PIX_FMT_RGB32:
    case V4L2_PIX_FMT_BGR32:
        plane_size[0] = width * height * 4;
        break;
    case V4L2_PIX_FMT_RGB24:
        plane_size[0] = width * height * 3;
        break;
    case V4L2_PIX_FMT_RGB565:
    case V4L2_PIX_FMT_RGB555X:
    case V4L2_PIX_FMT_RGB444:
        plane_size[0] = width * height * 2;
        break;
    case V4L2_PIX_FMT_NV12M:
    case V4L2_PIX_FMT_NV21:
    case V4L2_PIX_FMT_NV16:
    case V4L2_PIX_FMT_NV61:
        plane_size[0] = width * height;
        plane_size[1] = width * (height / 2);
        break;
    case V4L2_PIX_FMT_NV12MT_16X16:
        plane_size[0] = ALIGN(width, 16) * ALIGN(height, 16);
        plane_size[1] = ALIGN(width, 16) * ALIGN(height / 2, 8);
        break;
    case V4L2_PIX_FMT_YVU420M:
    case V4L2_PIX_FMT_YUV422P:
    case V4L2_PIX_FMT_YUYV:
    case V4L2_PIX_FMT_UYVY:
        plane_size[0] = width * height;
        plane_size[1] = (width / 2) * (height / 2);
        plane_size[2] = (width / 2) * (height / 2);
        break;
    default:
        ALOGE("%s::unmatched v4l_pixel_format color_space(0x%x)",
             __func__, v4l_pixel_format);
        return -1;
        break;
    }

    return 0;
}

static int m_exynos_rotator_multiple_of_n(
    int number, int N)
{
    int result = number;
    switch (N) {
    case 1:
    case 2:
    case 4:
    case 8:
    case 16:
    case 32:
    case 64:
    case 128:
    case 256:
        result = (number - (number & (N-1)));
        break;
    default:
        result = number - (number % N);
        break;
    }
    return result;
}

static bool m_exynos_rotator_check_src_size(
    unsigned int *w,      unsigned int *h,
    unsigned int *crop_x, unsigned int *crop_y,
    unsigned int *crop_w, unsigned int *crop_h,
    int v4l2_colorformat)
{
    if (*w < ROTATOR_MIN_W_SIZE || *h < ROTATOR_MIN_H_SIZE) {
        ALOGE("%s::too small size (w : %d < %d) (h : %d < %d)",
            __func__, ROTATOR_MIN_W_SIZE, *w, ROTATOR_MIN_H_SIZE, *h);
        return false;
    }

    if (*crop_w < ROTATOR_MIN_W_SIZE || *crop_h < ROTATOR_MIN_H_SIZE) {
        ALOGE("%s::too small size (w : %d < %d) (h : %d < %d)",
            __func__, ROTATOR_MIN_W_SIZE,* crop_w, ROTATOR_MIN_H_SIZE, *crop_h);
        return false;
    }

    switch (v4l2_colorformat) {
    // YUV420 3p
    case V4L2_PIX_FMT_YUV420M:
    case V4L2_PIX_FMT_YVU420M:
        *w = ALIGN(*w, 16);
        *h = ALIGN(*h, 16);
        break;
    // YUV420 2p
    case V4L2_PIX_FMT_NV12M:
    case V4L2_PIX_FMT_NV12MT:
    case V4L2_PIX_FMT_NV21M:
        *w = ALIGN(*w, 8);
        *h = ALIGN(*h, 8);
        break;
    // YUV422
    case V4L2_PIX_FMT_YUYV:
    case V4L2_PIX_FMT_YUV422P:
    case V4L2_PIX_FMT_UYVY:
    case V4L2_PIX_FMT_NV21:
    case V4L2_PIX_FMT_NV16:
    case V4L2_PIX_FMT_YVYU:
    case V4L2_PIX_FMT_VYUY:
    // RGB
    case V4L2_PIX_FMT_RGB32:
    case V4L2_PIX_FMT_RGB24:
    case V4L2_PIX_FMT_RGB565:
    case V4L2_PIX_FMT_BGR32:
    case V4L2_PIX_FMT_RGB555X:
    case V4L2_PIX_FMT_RGB444:
    default:
        *w = ALIGN(*w, 4);
        *h = ALIGN(*h, 4);
        break;
    }
    *crop_w = m_exynos_rotator_multiple_of_n(*crop_w, 4);
    *crop_h = m_exynos_rotator_multiple_of_n(*crop_h, 4);

    return true;
}

static bool m_exynos_rotator_check_dst_size(
    unsigned int *w,      unsigned int *h,
    unsigned int *crop_x, unsigned int *crop_y,
    unsigned int *crop_w, unsigned int *crop_h,
    int v4l2_colorformat,
    int rotation)
{
    unsigned int *new_w;
    unsigned int *new_h;
    unsigned int *new_crop_w;
    unsigned int *new_crop_h;

    if (rotation == 90 || rotation == 270) {
        new_w = h;
        new_h = w;
        new_crop_w = crop_h;
        new_crop_h = crop_w;
    } else {
        new_w = w;
        new_h = h;
        new_crop_w = crop_w;
        new_crop_h = crop_h;
    }

    if (*w < ROTATOR_MIN_W_SIZE || *h < ROTATOR_MIN_H_SIZE) {
        ALOGE("%s::too small size (w : %d < %d) (h : %d < %d)",
            __func__, ROTATOR_MIN_W_SIZE, *w, ROTATOR_MIN_H_SIZE, *h);
        return false;
    }

    if (*crop_w < ROTATOR_MIN_W_SIZE || *crop_h < ROTATOR_MIN_H_SIZE) {
        ALOGE("%s::too small size (w : %d < %d) (h : %d < %d)",
            __func__, ROTATOR_MIN_W_SIZE,* crop_w, ROTATOR_MIN_H_SIZE, *crop_h);
        return false;
    }

    switch (v4l2_colorformat) {
    // YUV420 3p
    case V4L2_PIX_FMT_YUV420M:
    case V4L2_PIX_FMT_YVU420M:
        *new_w = ALIGN(*new_w, 16);
        *new_h = ALIGN(*new_h, 16);
        break;
    // YUV420 2p
    case V4L2_PIX_FMT_NV12M:
    case V4L2_PIX_FMT_NV12MT:
    case V4L2_PIX_FMT_NV21M:
        *new_w = ALIGN(*new_w, 8);
        *new_h = ALIGN(*new_h, 8);
        break;
    // YUV422
    case V4L2_PIX_FMT_YUYV:
    case V4L2_PIX_FMT_YUV422P:
    case V4L2_PIX_FMT_UYVY:
    case V4L2_PIX_FMT_NV21:
    case V4L2_PIX_FMT_NV16:
    case V4L2_PIX_FMT_YVYU:
    case V4L2_PIX_FMT_VYUY:
    // RGB
    case V4L2_PIX_FMT_RGB32:
    case V4L2_PIX_FMT_RGB24:
    case V4L2_PIX_FMT_RGB565:
    case V4L2_PIX_FMT_BGR32:
    case V4L2_PIX_FMT_RGB555X:
    case V4L2_PIX_FMT_RGB444:
    default:
        *new_w = ALIGN(*new_w, 4);
        *new_h = ALIGN(*new_h, 4);
        break;
    }
    *new_crop_w = m_exynos_rotator_multiple_of_n(*new_crop_w, 4);
    *new_crop_h = m_exynos_rotator_multiple_of_n(*new_crop_h, 4);

    return true;
}

static int m_exynos_rotator_create(void)
{
    int          fd = 0;
    unsigned int cap;
    char         node[32];

    sprintf(node, "%s%d", PFX_NODE_ROTATOR, NODE_NUM_ROTATOR);
    fd = exynos_v4l2_open(node, O_RDWR);
    if (fd < 0) {
        ALOGE("%s::exynos_v4l2_open(%s) fail", __func__, node);
        return -1;
    }

    cap = V4L2_CAP_STREAMING |
          V4L2_CAP_VIDEO_OUTPUT_MPLANE |
          V4L2_CAP_VIDEO_CAPTURE_MPLANE;

    if (exynos_v4l2_querycap(fd, cap) == false) {
        ALOGE("%s::exynos_v4l2_querycap() fail", __func__);
        if (0 < fd)
            close(fd);
        fd = 0;
        return -1;
    }
    return fd;
}

static bool m_exynos_rotator_destroy(
    struct ROTATOR_HANDLE *rotator_handle)
{
    if (rotator_handle->src.stream_on == true) {
        if (exynos_v4l2_streamoff(rotator_handle->rotator_fd, rotator_handle->src.buf_type) < 0)
            ALOGE("%s::exynos_v4l2_streamoff() fail", __func__);

        rotator_handle->src.stream_on = false;
    }

    if (rotator_handle->dst.stream_on == true) {
        if (exynos_v4l2_streamoff(rotator_handle->rotator_fd, rotator_handle->dst.buf_type) < 0)
            ALOGE("%s::exynos_v4l2_streamoff() fail", __func__);

        rotator_handle->dst.stream_on = false;
    }

    if (0 < rotator_handle->rotator_fd)
        close(rotator_handle->rotator_fd);
    rotator_handle->rotator_fd = 0;

    return true;
}

bool m_exynos_rotator_find_and_trylock_and_create(
    struct ROTATOR_HANDLE *rotator_handle)
{
    int          i                 = 0;
    bool         flag_find_new_rotator = false;
    unsigned int total_sleep_time  = 0;

    do {
        if (exynos_mutex_trylock(rotator_handle->obj_mutex) == true) {

            // destroy old one.
            m_exynos_rotator_destroy(rotator_handle);

            // create new one.
            rotator_handle->rotator_fd = m_exynos_rotator_create();
            if (rotator_handle->rotator_fd < 0) {
                rotator_handle->rotator_fd = 0;
                exynos_mutex_unlock(rotator_handle->obj_mutex);
                continue;
            }

            if (rotator_handle->cur_obj_mutex)
                exynos_mutex_unlock(rotator_handle->cur_obj_mutex);

            rotator_handle->cur_obj_mutex = rotator_handle->obj_mutex;

            flag_find_new_rotator = true;
            break;
        }

        // waiting for another process doesn't use rotator.
        // we need to make decision how to do.
        if (flag_find_new_rotator == false) {
            usleep(ROTATOR_WAITING_TIME_FOR_TRYLOCK);
            total_sleep_time += ROTATOR_WAITING_TIME_FOR_TRYLOCK;
            ALOGV("%s::waiting for anthere process doens't use rotator", __func__);
        }

    } while(   flag_find_new_rotator == false
            && total_sleep_time < MAX_ROTATOR_WAITING_TIME_FOR_TRYLOCK);

    if (flag_find_new_rotator == false)
        ALOGE("%s::we don't have no available rotator.. fail", __func__);

    return flag_find_new_rotator;
}

static bool m_exynos_rotator_set_format(
    int                  fd,
    struct rotator_info *info,
    bool                 force)
{
    struct v4l2_requestbuffers req_buf;
    int                        plane_count;

    plane_count = m_rotator_get_plane_count(info->v4l2_colorformat);
    if (plane_count < 0) {
        ALOGE("%s::not supported v4l2_colorformat", __func__);
        return false;
    }

    if (force == false) {
        // format
        info->format.type = info->buf_type;
        if (exynos_v4l2_g_fmt(fd, &info->format) < 0) {
            ALOGE("%s::exynos_v4l2_g_fmt() fail type=%d", __func__, info->buf_type);
            return false;
        }

        if (info->width            != info->format.fmt.pix_mp.width ||
            info->height           != info->format.fmt.pix_mp.height ||
            info->v4l2_colorformat != info->format.fmt.pix_mp.pixelformat) {
            ALOGV("%s::info is different..)", __func__);
            goto set_hw;
        }

        // crop
        if (info->buf_type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
        info->crop.type = info->buf_type;
        if (exynos_v4l2_g_crop(fd, &info->crop) < 0) {
            ALOGE("%s::exynos_v4l2_g_crop() fail", __func__);
            return false;
        }

        if (info->crop_left   != info->crop.c.left ||
            info->crop_top    != info->crop.c.top ||
            info->crop_width  != info->crop.c.width ||
            info->crop_height != info->crop.c.height) {
            ALOGV("%s::crop is different..", __func__);
            goto set_hw;
        }
        }

        // rotation value;

        int value = 0;

        if (exynos_v4l2_g_ctrl(fd, V4L2_CID_ROTATE, &value) < 0) {
            ALOGE("%s::exynos_v4l2_g_ctrl(V4L2_CID_ROTATE) fail", __func__);
            return false;
        }

        if (info->rotation != value) {
            ALOGV("%s::rotation is different..", __func__);
            goto set_hw;
        }

        // skip s_fmt
        ALOGV("%s::fmt, crop is same with old-one, so skip s_fmt crop..", __func__);
        return true;
    }

set_hw:

    if (info->stream_on == true) {
        if (exynos_v4l2_streamoff(fd, info->buf_type) < 0) {
            ALOGE("%s::exynos_v4l2_streamoff() fail", __func__);
            return false;
        }
        info->stream_on = false;
    }

    if (exynos_v4l2_s_ctrl(fd, V4L2_CID_ROTATE, info->rotation) < 0) {
        ALOGE("%s::exynos_v4l2_s_ctrl(V4L2_CID_ROTATE) fail", __func__);
        return false;
    }

    info->format.fmt.pix_mp.width       = info->width;
    info->format.fmt.pix_mp.height      = info->height;
    info->format.fmt.pix_mp.pixelformat = info->v4l2_colorformat;
    info->format.fmt.pix_mp.field       = V4L2_FIELD_ANY;
    info->format.fmt.pix_mp.num_planes  = plane_count;

    if (exynos_v4l2_s_fmt(fd, &info->format) < 0) {
        ALOGE("%s::exynos_v4l2_s_fmt() fail", __func__);
        return false;
    }

    if (info->buf_type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
    info->crop.type     = info->buf_type;
    info->crop.c.left   = info->crop_left;
    info->crop.c.top    = info->crop_top;
    info->crop.c.width  = info->crop_width;
    info->crop.c.height = info->crop_height;

    if (exynos_v4l2_s_crop(fd, &info->crop) < 0) {
        ALOGE("%s::exynos_v4l2_s_crop() fail", __func__);
        return false;
    }
    }

    if (exynos_v4l2_s_ctrl(fd, V4L2_CID_CACHEABLE, info->cacheable) < 0) {
        ALOGE("%s::exynos_v4l2_s_ctrl() fail", __func__);
        return false;
    }

    req_buf.count  = 1;
    req_buf.type   = info->buf_type;
    req_buf.memory = V4L2_MEMORY_USERPTR;
    if (exynos_v4l2_reqbufs(fd, &req_buf) < 0) {
        ALOGE("%s::exynos_v4l2_reqbufs() fail", __func__);
        return false;
    }

    return true;
}

static bool m_exynos_rotator_set_addr(
    int                  fd,
    struct rotator_info *info)
{
    unsigned int i;
    unsigned int plane_size[NUM_OF_ROTATOR_PLANES];

    m_rotator_get_plane_size(plane_size,
                         info->width,
                         info->height,
                         info->v4l2_colorformat);

    info->buffer.index    = 0;
    info->buffer.type     = info->buf_type;
    info->buffer.memory   = V4L2_MEMORY_USERPTR;
    info->buffer.m.planes = info->planes;
    info->buffer.length   = info->format.fmt.pix_mp.num_planes;

    for (i = 0; i < info->format.fmt.pix_mp.num_planes; i++) {
        info->buffer.m.planes[i].m.userptr = (unsigned long)info->addr[i];
        info->buffer.m.planes[i].length    = plane_size[i];
        info->buffer.m.planes[i].bytesused = 0;
    }

    if (exynos_v4l2_qbuf(fd, &info->buffer) < 0) {
        ALOGE("%s::exynos_v4l2_qbuf() fail", __func__);
        return false;
    }

    return true;
}

void *exynos_rotator_create(void)
{
    int i     = 0;
    int op_id = 0;
    char mutex_name[32];

    struct ROTATOR_HANDLE *rotator_handle = (struct ROTATOR_HANDLE *)malloc(sizeof(struct ROTATOR_HANDLE));
    if (rotator_handle == NULL) {
        ALOGE("%s::malloc(struct ROTATOR_HANDLE) fail", __func__);
        goto err;
    }

    rotator_handle->rotator_fd = 0;
    memset(&rotator_handle->src, 0, sizeof(struct rotator_info));
    memset(&rotator_handle->dst, 0, sizeof(struct rotator_info));

    rotator_handle->src.buf_type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    rotator_handle->dst.buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

    rotator_handle->op_mutex = NULL;
    rotator_handle->obj_mutex = NULL;
    rotator_handle->cur_obj_mutex = NULL;

    srand(time(NULL));
    op_id = rand() % 1000000; // just make random id
    sprintf(mutex_name, "%sOp%d", LOG_TAG, op_id);
    rotator_handle->op_mutex = exynos_mutex_create(EXYNOS_MUTEX_TYPE_PRIVATE, mutex_name);
    if (rotator_handle->op_mutex == NULL) {
        ALOGE("%s::exynos_mutex_create(%s) fail", __func__, mutex_name);
        goto err;
    }

    exynos_mutex_lock(rotator_handle->op_mutex);

    sprintf(mutex_name, "%sObject%d", LOG_TAG, i);

    rotator_handle->obj_mutex = exynos_mutex_create(EXYNOS_MUTEX_TYPE_SHARED, mutex_name);
    if (rotator_handle->obj_mutex == NULL) {
        ALOGE("%s::exynos_mutex_create(%s) fail", __func__, mutex_name);
        goto err;
    }

    if (m_exynos_rotator_find_and_trylock_and_create(rotator_handle) == false) {
        ALOGE("%s::m_exynos_rotator_find_and_trylock_and_create() fail", __func__);
        goto err;
    }

    exynos_mutex_unlock(rotator_handle->cur_obj_mutex);
    exynos_mutex_unlock(rotator_handle->op_mutex);

    return (void *)rotator_handle;

err:
    if (rotator_handle) {
        m_exynos_rotator_destroy(rotator_handle);

        if (rotator_handle->cur_obj_mutex)
            exynos_mutex_unlock(rotator_handle->cur_obj_mutex);

        if ((rotator_handle->obj_mutex != NULL) &&
            (exynos_mutex_get_created_status(rotator_handle->obj_mutex) == true)) {
            if (exynos_mutex_destroy(rotator_handle->obj_mutex) == false)
                ALOGE("%s::exynos_mutex_destroy() fail", __func__);
        }

        if (rotator_handle->op_mutex)
            exynos_mutex_unlock(rotator_handle->op_mutex);

        free(rotator_handle);
    }

    return NULL;
}

void exynos_rotator_destroy(
    void *handle)
{
    int i = 0;
    struct ROTATOR_HANDLE *rotator_handle = (struct ROTATOR_HANDLE *)handle;

    if (handle == NULL) {
        ALOGE("%s::handle == NULL() fail", __func__);
        return;
    }

    exynos_mutex_lock(rotator_handle->op_mutex);
    exynos_mutex_lock(rotator_handle->cur_obj_mutex);

    m_exynos_rotator_destroy(rotator_handle);

    exynos_mutex_unlock(rotator_handle->cur_obj_mutex);

    if ((rotator_handle->obj_mutex != NULL) &&
        (exynos_mutex_get_created_status(rotator_handle->obj_mutex) == true)) {
        if (exynos_mutex_destroy(rotator_handle->obj_mutex) == false)
            ALOGE("%s::exynos_mutex_destroy() fail", __func__);
    }

    exynos_mutex_unlock(rotator_handle->op_mutex);

    if (rotator_handle)
        free(rotator_handle);
}

int exynos_rotator_set_src_format(
    void        *handle,
    unsigned int width,
    unsigned int height,
    unsigned int crop_left,
    unsigned int crop_top,
    unsigned int crop_width,
    unsigned int crop_height,
    unsigned int v4l2_colorformat,
    unsigned int cacheable)
{
    struct ROTATOR_HANDLE *rotator_handle;
    rotator_handle = (struct ROTATOR_HANDLE *)handle;

    if (handle == NULL) {
        ALOGE("%s::handle == NULL() fail", __func__);
        return -1;
    }

    exynos_mutex_lock(rotator_handle->op_mutex);

    rotator_handle->src.width            = width;
    rotator_handle->src.height           = height;
    rotator_handle->src.crop_left        = crop_left;
    rotator_handle->src.crop_top         = crop_top;
    rotator_handle->src.crop_width       = crop_width;
    rotator_handle->src.crop_height      = crop_height;
    rotator_handle->src.v4l2_colorformat = v4l2_colorformat;
    rotator_handle->src.cacheable        = cacheable;

    exynos_mutex_unlock(rotator_handle->op_mutex);

    return 0;
}

int exynos_rotator_set_dst_format(
    void        *handle,
    unsigned int width,
    unsigned int height,
    unsigned int crop_left,
    unsigned int crop_top,
    unsigned int v4l2_colorformat,
    unsigned int cacheable)
{
    struct ROTATOR_HANDLE *rotator_handle;
    rotator_handle = (struct ROTATOR_HANDLE *)handle;

    if (handle == NULL) {
        ALOGE("%s::handle == NULL() fail", __func__);
        return -1;
    }

    exynos_mutex_lock(rotator_handle->op_mutex);

    rotator_handle->dst.width            = width;
    rotator_handle->dst.height           = height;
    rotator_handle->dst.crop_left        = crop_left;
    rotator_handle->dst.crop_top         = crop_top;
    rotator_handle->dst.crop_width       = rotator_handle->src.crop_width;
    rotator_handle->dst.crop_height      = rotator_handle->src.crop_height;
    rotator_handle->dst.v4l2_colorformat = v4l2_colorformat;
    rotator_handle->dst.cacheable        = cacheable;

    exynos_mutex_unlock(rotator_handle->op_mutex);

    return 0;
}

int exynos_rotator_set_rotation(
    void *handle,
    int   rotation)
{
    int ret = -1;
    struct ROTATOR_HANDLE *rotator_handle;
    rotator_handle = (struct ROTATOR_HANDLE *)handle;

    if (handle == NULL) {
        ALOGE("%s::handle == NULL() fail", __func__);
        return ret;
    }

    exynos_mutex_lock(rotator_handle->op_mutex);

    int new_rotation = rotation % 360;

    if (new_rotation % 90 != 0) {
        ALOGE("%s::rotation(%d) cannot be acceptable fail", __func__, rotation);
        goto done;
    }

    if(new_rotation < 0)
        new_rotation = -new_rotation;

    rotator_handle->src.rotation = new_rotation;
    rotator_handle->dst.rotation = new_rotation;

    ret = 0;
done:
    exynos_mutex_unlock(rotator_handle->op_mutex);

    return ret;
}

int exynos_rotator_set_src_addr(
    void *handle,
    void *addr[3])
{
    struct ROTATOR_HANDLE *rotator_handle;
    rotator_handle = (struct ROTATOR_HANDLE *)handle;

    if (handle == NULL) {
        ALOGE("%s::handle == NULL() fail", __func__);
        return -1;
    }

    exynos_mutex_lock(rotator_handle->op_mutex);

    rotator_handle->src.addr[0] = addr[0];
    rotator_handle->src.addr[1] = addr[1];
    rotator_handle->src.addr[2] = addr[2];

    exynos_mutex_unlock(rotator_handle->op_mutex);

    return 0;
}

int exynos_rotator_set_dst_addr(
    void *handle,
    void *addr[3])
{
    struct ROTATOR_HANDLE *rotator_handle;
    rotator_handle = (struct ROTATOR_HANDLE *)handle;

    if (handle == NULL) {
        ALOGE("%s::handle == NULL() fail", __func__);
        return -1;
    }

    exynos_mutex_lock(rotator_handle->op_mutex);

    rotator_handle->dst.addr[0] = addr[0];
    rotator_handle->dst.addr[1] = addr[1];
    rotator_handle->dst.addr[2] = addr[2];

    exynos_mutex_unlock(rotator_handle->op_mutex);

    return 0;
}

int exynos_rotator_convert(
    void *handle)
{
    struct ROTATOR_HANDLE *rotator_handle;
    int ret    = -1;
    int i      = 0;
    rotator_handle = (struct ROTATOR_HANDLE *)handle;

    if (handle == NULL) {
        ALOGE("%s::handle == NULL() fail", __func__);
        return -1;
    }

    char mutex_name[32];
    bool flag_new_rotator = false;

    exynos_mutex_lock(rotator_handle->op_mutex);

    if (exynos_mutex_trylock(rotator_handle->cur_obj_mutex) == false) {
        if (m_exynos_rotator_find_and_trylock_and_create(rotator_handle) == false) {
            ALOGE("%s::m_exynos_rotator_find_and_trylock_and_create() fail", __func__);
            goto done;
        }
        flag_new_rotator = true;
    }

    if (m_exynos_rotator_check_src_size(&rotator_handle->src.width, &rotator_handle->src.width,
                                    &rotator_handle->src.crop_left, &rotator_handle->src.crop_top,
                                    &rotator_handle->src.crop_width, &rotator_handle->src.crop_height,
                                    rotator_handle->src.v4l2_colorformat) == false) {
        ALOGE("%s::m_exynos_rotator_check_size(src) fail", __func__);
        goto done;
    }

    if (m_exynos_rotator_check_dst_size(&rotator_handle->dst.width, &rotator_handle->dst.height,
                                    &rotator_handle->dst.crop_left, &rotator_handle->dst.crop_top,
                                    &rotator_handle->dst.crop_width, &rotator_handle->dst.crop_height,
                                    rotator_handle->dst.v4l2_colorformat,
                                    rotator_handle->dst.rotation) == false) {
        ALOGE("%s::m_exynos_rotator_check_size(dst) fail", __func__);
        goto done;
    }

    if (m_exynos_rotator_set_format(rotator_handle->rotator_fd, &rotator_handle->src, flag_new_rotator) == false) {
        ALOGE("%s::m_exynos_rotator_set_format(src) fail", __func__);
        goto done;
    }

    if (m_exynos_rotator_set_format(rotator_handle->rotator_fd, &rotator_handle->dst, flag_new_rotator) == false) {
        ALOGE("%s::m_exynos_rotator_set_format(dst) fail", __func__);
        goto done;
    }

    if (m_exynos_rotator_set_addr(rotator_handle->rotator_fd, &rotator_handle->src) == false) {
        ALOGE("%s::m_exynos_rotator_set_addr(src) fail", __func__);
        goto done;
    }

    if (m_exynos_rotator_set_addr(rotator_handle->rotator_fd, &rotator_handle->dst) == false) {
        ALOGE("%s::m_exynos_rotator_set_addr(dst) fail", __func__);
        goto done;
    }

    if (rotator_handle->src.stream_on == false) {
        if (exynos_v4l2_streamon(rotator_handle->rotator_fd, rotator_handle->src.buf_type) < 0) {
            ALOGE("%s::exynos_v4l2_streamon(src) fail", __func__);
            goto done;
        }
        rotator_handle->src.stream_on = true;
    }

    if (rotator_handle->dst.stream_on == false) {
        if (exynos_v4l2_streamon(rotator_handle->rotator_fd, rotator_handle->dst.buf_type) < 0) {
            ALOGE("%s::exynos_v4l2_streamon(dst) fail", __func__);
            goto done;
        }
        rotator_handle->dst.stream_on = true;
    }

    if (exynos_v4l2_dqbuf(rotator_handle->rotator_fd, &rotator_handle->src.buffer) < 0) {
        ALOGE("%s::exynos_v4l2_dqbuf(src) fail", __func__);
        goto done;
    }

    if (exynos_v4l2_dqbuf(rotator_handle->rotator_fd, &rotator_handle->dst.buffer) < 0) {
        ALOGE("%s::exynos_v4l2_dqbuf(dst) fail", __func__);
        goto done;
    }

    if (rotator_handle->src.stream_on == true) {
        if (exynos_v4l2_streamoff(rotator_handle->rotator_fd, rotator_handle->src.buf_type) < 0) {
            ALOGE("%s::exynos_v4l2_streamon(src) fail", __func__);
            goto done;
        }
        rotator_handle->src.stream_on = false;
    }

    if (rotator_handle->dst.stream_on == true) {
        if (exynos_v4l2_streamoff(rotator_handle->rotator_fd, rotator_handle->dst.buf_type) < 0) {
            ALOGE("%s::exynos_v4l2_streamon(dst) fail", __func__);
            goto done;
        }
        rotator_handle->dst.stream_on = false;
    }

    ret = 0;

done:
    exynos_mutex_unlock(rotator_handle->cur_obj_mutex);
    exynos_mutex_unlock(rotator_handle->op_mutex);

    return ret;
}

int exynos_rotator_connect(
    void *handle,
    void *hw)
{
    struct ROTATOR_HANDLE *rotator_handle;
    int ret    = -1;
    rotator_handle = (struct ROTATOR_HANDLE *)handle;

    if (handle == NULL) {
        ALOGE("%s::handle == NULL() fail", __func__);
        return -1;
    }

    exynos_mutex_lock(rotator_handle->op_mutex);

    if (exynos_mutex_trylock(rotator_handle->cur_obj_mutex) == false) {
        if (m_exynos_rotator_find_and_trylock_and_create(rotator_handle) == false) {
            ALOGE("%s::m_exynos_rotator_find_and_trylock_and_create() fail", __func__);
            goto done;
        }
    }

    ret = 0;

done:
    exynos_mutex_unlock(rotator_handle->op_mutex);

    return ret;
}

int exynos_rotator_disconnect(
    void *handle,
    void *hw)
{
    struct ROTATOR_HANDLE *rotator_handle;
    rotator_handle = (struct ROTATOR_HANDLE *)handle;

    if (handle == NULL) {
        ALOGE("%s::handle == NULL() fail", __func__);
        return -1;
    }

    exynos_mutex_lock(rotator_handle->op_mutex);

    exynos_mutex_unlock(rotator_handle->cur_obj_mutex);
    exynos_mutex_unlock(rotator_handle->op_mutex);

    return 0;
}
