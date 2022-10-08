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
 *    Binglin Chen <binglin.chen@intel.com>
 *    Zhaohan Ren  <zhaohan.ren@intel.com>
 *    Shengquan Yuan  <shengquan.yuan@intel.com>
 *
 */

#ifndef         PSB_TEXTURE_H_
# define        PSB_TEXTURE_H_

#include "mrst/pvr2d.h"
#include <img_types.h>

#define DRI2_BACK_BUFFER_EXPORT_TYPE_BUFFERS 1
#define DRI2_BACK_BUFFER_EXPORT_TYPE_SWAPCHAIN 2

#define DRI2_FLIP_BUFFERS_NUM           2
#define DRI2_BLIT_BUFFERS_NUM           2
#define DRI2_MAX_BUFFERS_NUM            MAX( DRI2_FLIP_BUFFERS_NUM, DRI2_BLIT_BUFFERS_NUM )
#define VIDEO_BUFFER_NUM                20


typedef struct _psb_coeffs_ {
    signed char rY;
    signed char rU;
    signed char rV;
    signed char gY;
    signed char gU;
    signed char gV;
    signed char bY;
    signed char bU;
    signed char bV;
    unsigned char rShift;
    unsigned char gShift;
    unsigned char bShift;
    signed short rConst;
    signed short gConst;
    signed short bConst;
} psb_coeffs_s, *psb_coeffs_p;

typedef struct _sgx_psb_fixed32 {
    union {
        struct {
            unsigned short Fraction;
            short Value;
        };
        long ll;
    };
} sgx_psb_fixed32;

typedef struct _PVRDRI2BackBuffersExport_ {
    IMG_UINT32 ui32Type;
    //pixmap handles
    PVR2D_HANDLE hBuffers[3];

    IMG_UINT32 ui32BuffersCount;
    IMG_UINT32 ui32SwapChainID;
} PVRDRI2BackBuffersExport;

struct psb_texture_s {
    struct _WsbmBufferObject *vaSrf;

    unsigned int video_transfermatrix;
    unsigned int src_nominalrange;
    unsigned int dst_nominalrange;

    uint32_t gamma0;
    uint32_t gamma1;
    uint32_t gamma2;
    uint32_t gamma3;
    uint32_t gamma4;
    uint32_t gamma5;

    sgx_psb_fixed32 brightness;
    sgx_psb_fixed32 contrast;
    sgx_psb_fixed32 saturation;
    sgx_psb_fixed32 hue;

    psb_coeffs_s coeffs;

    uint32_t update_coeffs;
    PVRDRI2BackBuffersExport dri2_bb_export;
    PVRDRI2BackBuffersExport extend_dri2_bb_export;

    /* struct dri_drawable *extend_dri_drawable; */
    /* struct dri_drawable *dri_drawable; */
    unsigned char *extend_dri_drawable;
    unsigned char *dri_drawable;

    uint32_t dri_init_flag;
    uint32_t extend_dri_init_flag;
    uint32_t adjust_window_flag;
    uint32_t current_blt_buffer;

    uint32_t extend_current_blt_buffer;
    uint32_t destw_save;
    uint32_t desth_save;
    uint32_t drawable_update_flag; /* drawable resize or switch between window <==> pixmap */
    uint32_t local_rotation_save;
    uint32_t extend_rotation_save;

    PVR2DMEMINFO *pal_meminfo[6];
    PVR2DMEMINFO *blt_meminfo_pixmap;
    PVR2DMEMINFO *blt_meminfo[DRI2_BLIT_BUFFERS_NUM];
    PVR2DMEMINFO *flip_meminfo[DRI2_FLIP_BUFFERS_NUM];
    PVR2DMEMINFO *extend_blt_meminfo[DRI2_BLIT_BUFFERS_NUM];
};

int psb_ctexture_init(VADriverContextP ctx);

void psb_ctexture_deinit(VADriverContextP ctx);

void blit_texture_to_buf(VADriverContextP ctx, unsigned char * data, int src_x, int src_y, int src_w,
                         int src_h, int dst_x, int dst_y, int dst_w, int dst_h,
                         int width, int height, int src_pitch, struct _WsbmBufferObject * src_buf,
                         unsigned int placement);

void psb_putsurface_textureblit(
    VADriverContextP ctx, unsigned char *dst, VASurfaceID surface, int src_x, int src_y, int src_w,
    int src_h, int dst_x, int dst_y, int dst_w, int dst_h, unsigned int subtitle,
    int width, int height,
    int src_pitch, struct _WsbmBufferObject * src_buf,
    unsigned int placement, int wrap_dst);

#endif      /* !PSB_TEXTURE_H_ */
