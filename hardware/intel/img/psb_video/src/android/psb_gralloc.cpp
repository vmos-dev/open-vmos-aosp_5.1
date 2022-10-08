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
 *    Fei Jiang  <fei.jiang@intel.com>
 *    Austin Yuan <austin.yuan@intel.com>
 *
 */

#include "android/psb_gralloc.h"
#include <cutils/log.h>
#include <utils/threads.h>
#include <ui/PixelFormat.h>
#include <hardware/gralloc.h>
#include <system/graphics.h>
#include <hardware/hardware.h>
#ifdef BAYTRAIL
#include <ufo/gralloc.h>
#endif
using namespace android;

#ifdef  LOG_TAG
#undef  LOG_TAG
#endif

#define LOG_TAG "pvr_drv_video"

static hw_module_t const *module;
static gralloc_module_t *mAllocMod; /* get by force hw_module_t */

int gralloc_lock(buffer_handle_t handle,
                int usage, int left, int top, int width, int height,
                void** vaddr)
{
    int err, j;

    if (!mAllocMod) {
        ALOGW("%s: gralloc module has not been initialized. Should initialize it first", __func__);
        if (gralloc_init()) {
            ALOGE("%s: can't find the %s module", __func__, GRALLOC_HARDWARE_MODULE_ID);
            return -1;
        }
    }

    err = mAllocMod->lock(mAllocMod, handle, usage,
                          left, top, width, height,
                          vaddr);
    ALOGV("gralloc_lock: handle is %p, usage is %x, vaddr is %p.\n", handle, usage, *vaddr);

//#ifdef BAYTRAIL
#if 0
    unsigned char *tmp_buffer = (unsigned char *)(*vaddr);
    int dst_stride;
    if (width <= 512)
        dst_stride = 512;
    else if (width <= 1024)
        dst_stride = 1024;
    else if (width <= 1280)
        dst_stride = 1280;
    else if (width <= 2048)
        dst_stride = 2048;

    int align_h = 32;
    int dsth = (height + align_h - 1) & ~(align_h - 1);
    LOGD("width is %d, dst_stride is %d, dsth is %d.\n",
         width, dst_stride, dsth);

    for (j = 0; j < dst_stride * dsth * 3 / 2; j = j + 4096) {
        *(tmp_buffer + j) = 0xa5;
        if (*(tmp_buffer + j) !=  0xa5)
            LOGE("access page failed, width is %d, dst_stride is %d, dsth is %d.\n",
                 width, dst_stride, dsth);
    }
#endif
    if (err){
        ALOGE("lock(...) failed %d (%s).\n", err, strerror(-err));
        return -1;
    } else {
        ALOGV("lock returned with address %p\n", *vaddr);
    }

    return err;
}

int gralloc_unlock(buffer_handle_t handle)
{
    int err;

    if (!mAllocMod) {
        ALOGW("%s: gralloc module has not been initialized. Should initialize it first", __func__);
        if (gralloc_init()) {
            ALOGE("%s: can't find the %s module", __func__, GRALLOC_HARDWARE_MODULE_ID);
            return -1;
        }
    }

    err = mAllocMod->unlock(mAllocMod, handle);
    if (err) {
        ALOGE("unlock(...) failed %d (%s)", err, strerror(-err));
        return -1;
    } else {
        ALOGV("unlock returned\n");
    }

    return err;
}

int gralloc_init(void)
{
    int err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);
    if (err) {
        ALOGE("FATAL: can't find the %s module", GRALLOC_HARDWARE_MODULE_ID);
        return -1;
    } else
        ALOGD("hw_get_module returned\n");
    mAllocMod = (gralloc_module_t *)module;

    return 0;
}

int gralloc_getdisplaystatus(buffer_handle_t handle,  int* status)
{
    int err;
#ifndef BAYTRAIL
    int (*get_display_status)(gralloc_module_t*, buffer_handle_t, int*);

    get_display_status = (int (*)(gralloc_module_t*, buffer_handle_t, int*))(mAllocMod->reserved_proc[0]);
    if (get_display_status == NULL) {
        ALOGE("can't get gralloc_getdisplaystatus(...) \n");
        return -1;
    }
    err = (*get_display_status)(mAllocMod, handle, status);
#else
    err = 0;
    *status = mAllocMod->perform(mAllocMod, INTEL_UFO_GRALLOC_MODULE_PERFORM_GET_BO_STATUS, handle);
#endif
    if (err){
        ALOGE("gralloc_getdisplaystatus(...) failed %d (%s).\n", err, strerror(-err));
        return -1;
    }

    return err;
}
