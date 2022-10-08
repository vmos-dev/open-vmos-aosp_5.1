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
 *    Zeng Li <zeng.li@intel.com>
 *    Jason Hu <jason.hu@intel.com>
 *    Shengquan Yuan  <shengquan.yuan@intel.com>
 */
#include "psb_surface.h"
#ifdef ANDROID
static uint32_t VAROTATION2HAL(int va_rotate) {
        switch (va_rotate) {
        case VA_ROTATION_90:
            return HAL_TRANSFORM_ROT_90;
        case VA_ROTATION_180:
            return HAL_TRANSFORM_ROT_180;
        case VA_ROTATION_270:
            return HAL_TRANSFORM_ROT_270;
        default:
            return 0;
        }
}


static uint32_t HAL2VAROTATION(int hal_rotate) {
        switch (hal_rotate) {
        case HAL_TRANSFORM_ROT_90:
            return VA_ROTATION_90;
        case HAL_TRANSFORM_ROT_180:
            return VA_ROTATION_180;
        case HAL_TRANSFORM_ROT_270:
            return VA_ROTATION_270;
        default:
            return 0;
        }
}

#else
#define VAROTATION2HAL(a) a
#define HAL2VAROTATION(a) a
#define psb_android_is_extvideo_mode(a) 0
#define psb_android_surfaceflinger_rotate(a, b)
#endif

void psb_InitOutLoop(VADriverContextP ctx);
void psb_RecalcAlternativeOutput(object_context_p obj_context);
void psb_CheckInterlaceRotate(object_context_p obj_context, unsigned char *pic_param_tmp);
VAStatus psb_DestroyRotateSurface(
    VADriverContextP ctx,
    object_surface_p obj_surface,
    int rotate
);
VAStatus psb_CreateOutLoopSurface(
    object_context_p obj_context,
    object_surface_p obj_surface,
    int msvdx_rotate
);

VAStatus psb_CreateScalingSurface(
    object_context_p obj_context,
    object_surface_p obj_surface
);

VAStatus psb_CreateRotateSurface(
    object_context_p obj_context,
    object_surface_p obj_surface,
    int msvdx_rotate
);

int psb__dump_NV12_buffers(
    psb_surface_p psb_surface,
    short srcx,
    short srcy,
    unsigned short srcw,
    unsigned short srch
);

