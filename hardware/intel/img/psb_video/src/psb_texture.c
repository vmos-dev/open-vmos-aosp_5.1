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
  */

#include <stdio.h>
#include <unistd.h>
#include <math.h>

#ifdef ANDROID
#include <drm/ttm/ttm_placement.h>
#include <linux/psb_drm.h>
#else
#include <psb_drm.h>
#endif

#include <va/va_backend.h>
#include <va/va_drmcommon.h>
#include "psb_drv_debug.h"

#include <wsbm/wsbm_manager.h>

#ifndef ANDROID
#include <X11/Xlib.h>
#include "x11/psb_xrandr.h"
#include "x11/psb_x11.h"
#endif

#include "mrst/pvr2d.h"

#include "psb_drv_video.h"
#include "psb_output.h"
#include "psb_surface_ext.h"

#include "psb_texture.h"

#define INIT_DRIVER_DATA    psb_driver_data_p driver_data = (psb_driver_data_p) ctx->pDriverData;
#define INIT_OUTPUT_PRIV    psb_x11_output_p output = (psb_x11_output_p)(((psb_driver_data_p)ctx->pDriverData)->ws_priv)
#define SURFACE(id) ((object_surface_p) object_heap_lookup( &driver_data->surface_heap, id ))
#define SUBPIC(id)  ((object_subpic_p) object_heap_lookup( &driver_data->subpic_heap, id ))

#define Degree (2*PI / 360.0)
#define PI 3.1415927

#define OV_HUE_DEFAULT_VALUE   0
#define OV_HUE_MIN            -30
#define OV_HUE_MAX             30

#define OV_BRIGHTNESS_DEFAULT_VALUE   0
#define OV_BRIGHTNESS_MIN            -50
#define OV_BRIGHTNESS_MAX             50

#define OV_CONTRAST_DEFAULT_VALUE     0
#define OV_CONTRAST_MIN              -100
#define OV_CONTRAST_MAX               100

#define OV_SATURATION_DEFAULT_VALUE   100
#define OV_SATURATION_MIN             0
#define OV_SATURATION_MAX             200

typedef struct _psb_transform_coeffs_ {
    double rY, rCb, rCr;
    double gY, gCb, gCr;
    double bY, bCb, bCr;
} psb_transform_coeffs;

typedef enum _psb_videotransfermatrix {
    PSB_VideoTransferMatrixMask = 0x07,
    PSB_VideoTransferMatrix_Unknown = 0,
    PSB_VideoTransferMatrix_BT709 = 1,
    PSB_VideoTransferMatrix_BT601 = 2,
    PSB_VideoTransferMatrix_SMPTE240M = 3
} psb_videotransfermatrix;

typedef enum _psb_nominalrange {
    PSB_NominalRangeMask = 0x07,
    PSB_NominalRange_Unknown = 0,
    PSB_NominalRange_Normal = 1,
    PSB_NominalRange_Wide = 2,
    /* explicit range forms */
    PSB_NominalRange_0_255 = 1,
    PSB_NominalRange_16_235 = 2,
    PSB_NominalRange_48_208 = 3
} psb_nominalrange;

/*
 * ITU-R BT.601, BT.709 and SMPTE 240M transfer matrices from VA 2.0
 * Video Color Field definitions Design Spec(Version 0.03).
 * [R', G', B'] values are in the range [0, 1], Y' is in the range [0,1]
 * and [Pb, Pr] components are in the range [-0.5, 0.5].
 */
static psb_transform_coeffs s601 = {
    1, -0.000001, 1.402,
    1, -0.344136, -0.714136,
    1, 1.772, 0
};

static psb_transform_coeffs s709 = {
    1, 0, 1.5748,
    1, -0.187324, -0.468124,
    1, 1.8556, 0
};

static psb_transform_coeffs s240M = {
    1, -0.000657, 1.575848,
    1, -0.226418, -0.476529,
    1, 1.825958, 0.000378
};

static void psb_setup_coeffs(struct psb_texture_s * pPriv);
static void psb_scale_transfermatrix(psb_transform_coeffs * transfer_matrix,
                                     double YColumScale, double CbColumScale,
                                     double CrColumnScale);
static void psb_select_transfermatrix(struct psb_texture_s * pPriv,
                                      psb_transform_coeffs * transfer_matrix,
                                      double *Y_offset, double *CbCr_offset,
                                      double *RGB_offset);
static void psb_create_coeffs(double yOff, double uOff, double vOff, double rgbOff,
                              double yScale, double uScale, double vScale,
                              double brightness, double contrast,
                              double *pYCoeff, double *pUCoeff, double *pVCoeff,
                              double *pConstant);
static void psb_convert_coeffs(double Ycoeff, double Ucoeff, double Vcoeff,
                               double ConstantTerm, signed char *pY, signed char *pU,
                               signed char *pV, signed short *constant,
                               unsigned char *pShift);
static int psb_check_coeffs(double Ycoeff, double Ucoeff, double Vcoeff,
                            double ConstantTerm, signed char byShift);
static void
psb_transform_sathuecoeffs(psb_transform_coeffs * dest,
                           const psb_transform_coeffs * const source,
                           double fHue, double fSat);

static unsigned long PVRCalculateStride(unsigned long widthInPixels, unsigned int bitsPerPixel, unsigned int stride_alignment)
{
    int ulActiveLinelenInPixels = (widthInPixels + (stride_alignment - 1)) & ~(stride_alignment - 1);
    return ((ulActiveLinelenInPixels * bitsPerPixel) + 7) >> 3;
}

static int pvr_context_create(unsigned char **pvr_ctx)
{
#ifdef _FOR_FPGA_
    return PVR2D_OK;
#endif
    int ret = 0;
    int pvr_devices = PVR2DEnumerateDevices(0);
    PVR2DDEVICEINFO *pvr_devs = NULL;

    if ((pvr_devices < PVR2D_OK) || (pvr_devices == 0)) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s(): PowerVR device not found", __func__);
        goto out;
    }

    pvr_devs = calloc(1, pvr_devices * sizeof(*pvr_devs));
    if (!pvr_devs) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s(): not enough memory", __func__);
        goto out;
    }

    ret = PVR2DEnumerateDevices(pvr_devs);
    if (ret != PVR2D_OK) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s(): PVR2DEnumerateDevices() failed(%d)", __func__,
                           ret);
        goto out;
    }

    /* Choose the first display device */
    ret = PVR2DCreateDeviceContext(pvr_devs[0].ulDevID, (PVR2DCONTEXTHANDLE *)pvr_ctx, 0);
    if (ret != PVR2D_OK) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s(): PVR2DCreateDeviceContext() failed(%d)", __func__,
                           ret);
        goto out;
    }

out:
    if (pvr_devs)
        free(pvr_devs);

    return ret;
}

void psb_fix_drmfd_closesequence(psb_driver_data_p driver_data)
{
    driver_data->dup_drm_fd = dup(driver_data->drm_fd);
}


int psb_ctexture_init(VADriverContextP ctx)
{
    INIT_DRIVER_DATA;

    struct psb_texture_s *texture_priv = &driver_data->ctexture_priv;
    int i, ret;

    ret = pvr_context_create(&driver_data->hPVR2DContext);
    if (ret != PVR2D_OK) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s(): null PVR context!!", __func__);
        return ret;
    }

    texture_priv->video_transfermatrix = PSB_VideoTransferMatrix_BT709;
    texture_priv->src_nominalrange = PSB_NominalRange_0_255;
    texture_priv->dst_nominalrange = PSB_NominalRange_0_255;

    texture_priv->brightness.Value = OV_BRIGHTNESS_DEFAULT_VALUE;
    texture_priv->brightness.Fraction = 0;
    texture_priv->contrast.Value = OV_CONTRAST_DEFAULT_VALUE;
    texture_priv->contrast.Fraction = 0;
    texture_priv->hue.Value = OV_HUE_DEFAULT_VALUE;
    texture_priv->hue.Fraction = 0;
    texture_priv->saturation.Value = OV_SATURATION_DEFAULT_VALUE;
    texture_priv->saturation.Fraction = 0;

    texture_priv->gamma5 = 0xc0c0c0;
    texture_priv->gamma4 = 0x808080;
    texture_priv->gamma3 = 0x404040;
    texture_priv->gamma2 = 0x202020;
    texture_priv->gamma1 = 0x101010;
    texture_priv->gamma0 = 0x080808;

    texture_priv->dri_init_flag = 0;
    texture_priv->drawable_update_flag = 0;
    texture_priv->extend_dri_init_flag = 0;
    texture_priv->current_blt_buffer = 0;
    texture_priv->extend_current_blt_buffer = 0;
    texture_priv->adjust_window_flag = 0;
    texture_priv->destw_save = 0;
    texture_priv->desth_save = 0;
    texture_priv->local_rotation_save = -1;
    texture_priv->extend_rotation_save = -1;
    texture_priv->dri_drawable = NULL;
    texture_priv->extend_dri_drawable = NULL;

    for (i = 0; i < DRI2_BLIT_BUFFERS_NUM; i++) {
        texture_priv->blt_meminfo[i] = NULL;
        texture_priv->extend_blt_meminfo[i] = NULL;
    }

    for (i = 0; i < DRI2_FLIP_BUFFERS_NUM; i++)
        texture_priv->flip_meminfo[i] = NULL;

    texture_priv->blt_meminfo_pixmap = NULL;

    for (i = 0; i < 6; i++)
        texture_priv->pal_meminfo[i] = NULL;

    psb_setup_coeffs(texture_priv);
    psb_fix_drmfd_closesequence(driver_data);

    return 0;
}

void psb_ctexture_deinit(VADriverContextP ctx)
{
    INIT_DRIVER_DATA;
    PVR2DERROR ePVR2DStatus;
    int i;

    struct psb_texture_s *texture_priv = &driver_data->ctexture_priv;

    if (texture_priv->blt_meminfo_pixmap) {
        ePVR2DStatus = PVR2DMemFree(driver_data->hPVR2DContext, texture_priv->blt_meminfo_pixmap);
        if (ePVR2DStatus != PVR2D_OK)
            drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: PVR2DMemFree error %d\n", __FUNCTION__, ePVR2DStatus);
    }

    for (i = 0; i < DRI2_BLIT_BUFFERS_NUM; i++) {
        if (texture_priv->blt_meminfo[i]) {
            ePVR2DStatus = PVR2DMemFree(driver_data->hPVR2DContext, texture_priv->blt_meminfo[i]);
            if (ePVR2DStatus != PVR2D_OK)
                drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: PVR2DMemFree error %d\n", __FUNCTION__, ePVR2DStatus);
            texture_priv->blt_meminfo[i] = NULL;
        }
    }

    for (i = 0; i < DRI2_FLIP_BUFFERS_NUM; i++) {
        if (texture_priv->flip_meminfo[i]) {
            ePVR2DStatus = PVR2DMemFree(driver_data->hPVR2DContext, texture_priv->flip_meminfo[i]);
            if (ePVR2DStatus != PVR2D_OK)
                drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: PVR2DMemFree error %d\n", __FUNCTION__, ePVR2DStatus);
            texture_priv->flip_meminfo[i] = NULL;
        }
    }

    for (i = 0; i < DRI2_BLIT_BUFFERS_NUM; i++) {
        if (texture_priv->extend_blt_meminfo[i]) {
            ePVR2DStatus = PVR2DMemFree(driver_data->hPVR2DContext, texture_priv->extend_blt_meminfo[i]);
            if (ePVR2DStatus != PVR2D_OK)
                drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: PVR2DMemFree error %d\n", __FUNCTION__, ePVR2DStatus);
            texture_priv->extend_blt_meminfo[i] = NULL;
        }
    }


    for (i = 0; i < 6; i++) {
        if (texture_priv->pal_meminfo[i]) {
            ePVR2DStatus = PVR2DMemFree(driver_data->hPVR2DContext, texture_priv->pal_meminfo[i]);
            if (ePVR2DStatus != PVR2D_OK)
                drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: PVR2DMemFree error %d\n", __FUNCTION__, ePVR2DStatus);
            texture_priv->pal_meminfo[i] = NULL;
        }
    }

    if (driver_data->hPVR2DContext) {
        ePVR2DStatus = PVR2DDestroyDeviceContext(driver_data->hPVR2DContext);
        if (ePVR2DStatus != PVR2D_OK)
            drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: PVR2DMemFree error %d\n", __FUNCTION__, ePVR2DStatus);
        driver_data->hPVR2DContext = NULL;
    }

    if (driver_data->dup_drm_fd)
        close(driver_data->dup_drm_fd);
}

/* calculate subpicture size according to the downscale situation of both main and subpicture bitstream */
static void psb_calculate_subpic_size(int surf_width, int surf_height, int dst_w, int dst_h, PsbVASurfaceRec *surface_subpic)
{
    float src_h_ratio, src_v_ratio;
    float subpic_h_ratio, subpic_v_ratio;
    float subpic_h_dest_ratio, subpic_v_dest_ratio;

    src_h_ratio = (float)surf_width / dst_w;
    src_v_ratio = (float)surf_height / dst_h;

    subpic_h_ratio = (float)surface_subpic->subpic_srcw / surface_subpic->subpic_dstw;
    subpic_v_ratio = (float)surface_subpic->subpic_srch / surface_subpic->subpic_dsth;

    subpic_h_dest_ratio = (float)dst_w / surface_subpic->subpic_dstw;
    subpic_v_dest_ratio = (float)dst_h / surface_subpic->subpic_dsth;

    if (!(surface_subpic->subpic_flags & VA_SUBPICTURE_DESTINATION_IS_SCREEN_COORD)) {
        /* If coordinates are video relative then scale subpicture with video */
        surface_subpic->subpic_dstx /= src_h_ratio;
        surface_subpic->subpic_dsty /= src_v_ratio;
        surface_subpic->subpic_dstx /= subpic_h_ratio;
        surface_subpic->subpic_dsty /= subpic_v_ratio;

        surface_subpic->subpic_dstw /= src_h_ratio;
        surface_subpic->subpic_dsth /= src_v_ratio;
        surface_subpic->subpic_dstw /= subpic_h_ratio;
        surface_subpic->subpic_dsth /= subpic_v_ratio;
    }
}

static PPVR2DMEMINFO psb_check_subpic_buffer(psb_driver_data_p driver_data, PsbVASurfaceRec* surface_subpic)
{
    unsigned int i, j;
    unsigned char* tmp_buffer;
    unsigned char tmp;
    PVR2DERROR ePVR2DStatus;

    /* Find and return the wrapped buffer index */
    for (i = 0; i < VIDEO_BUFFER_NUM; i++) {
        if (driver_data->wrapped_subpic_id[i] == surface_subpic->subpic_id && driver_data->subpicBuf[i]) {
            return driver_data->subpicBuf[i];
        }
    }

    /* Wrap a un-wrapped buffer and return */
    for (i = 0; i < VIDEO_BUFFER_NUM; i++) {
        if (driver_data->wrapped_subpic_id[i] == VA_INVALID_ID) {
            tmp_buffer = NULL;
            tmp_buffer = wsbmBOMap(surface_subpic->bo, WSBM_ACCESS_READ | WSBM_ACCESS_WRITE);
            if (NULL == tmp_buffer) {
                drv_debug_msg(VIDEO_DEBUG_ERROR, "%s L%d: wsbmBOMap failed!",
                        __FUNCTION__, __LINE__);
                return NULL;
            }
            for (j = 0; j < surface_subpic->size; j = j + 4096) {
                tmp = *(tmp_buffer + j);
                if (tmp == 0)
                    *(tmp_buffer + j) = 0;
            }

            ePVR2DStatus = PVR2DMemWrap(driver_data->hPVR2DContext,
                                        tmp_buffer,
                                        0,
                                        surface_subpic->size,
                                        NULL,
                                        &driver_data->subpicBuf[i]);
            if (ePVR2DStatus != PVR2D_OK) {
                drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: PVR2DMemWrap error %d\n", __FUNCTION__, ePVR2DStatus);
                return NULL;
            }

            driver_data->wrapped_subpic_id[i] = surface_subpic->subpic_id;
            return driver_data->subpicBuf[i];
        }
    }

    if (i == VIDEO_BUFFER_NUM - 1) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: Out of warpped subpic buffer memory\n", __FUNCTION__);
        return NULL;
    }

    return NULL;
}


void psb_init_surface_pvr2dbuf(psb_driver_data_p driver_data)
{
    int i;
    for (i = 0; i < VIDEO_BUFFER_NUM; i++) {
        driver_data->videoBuf[i] = NULL;
        driver_data->subpicBuf[i] = NULL;
        driver_data->wrapped_surface_id[i] = VA_INVALID_ID;
        driver_data->wrapped_subpic_id[i] = VA_INVALID_ID;
    }

}

void psb_free_surface_pvr2dbuf(psb_driver_data_p driver_data)
{
    int i;
    PVR2DERROR ePVR2DStatus;

    for (i = 0; i < VIDEO_BUFFER_NUM; i++) {
        if ((driver_data->wrapped_surface_id[i] != VA_INVALID_ID) && driver_data->videoBuf[i]) {
            ePVR2DStatus = PVR2DMemFree(driver_data->hPVR2DContext, driver_data->videoBuf[i]);
            if (ePVR2DStatus != PVR2D_OK)
                drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: PVR2DMemFree error %d\n", __FUNCTION__, ePVR2DStatus);
        }

        if ((driver_data->wrapped_subpic_id[i] != VA_INVALID_ID) && driver_data->subpicBuf[i]) {
            ePVR2DStatus = PVR2DMemFree(driver_data->hPVR2DContext, driver_data->subpicBuf[i]);
            if (ePVR2DStatus != PVR2D_OK)
                drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: PVR2DMemFree error %d\n", __FUNCTION__, ePVR2DStatus);
        }

        driver_data->wrapped_surface_id[i] = VA_INVALID_ID;
        driver_data->wrapped_subpic_id[i] = -1;

        driver_data->videoBuf[i] = NULL;
        driver_data->subpicBuf[i] = NULL;
    }
}


static PPVR2DMEMINFO psb_wrap_surface_pvr2dbuf(psb_driver_data_p driver_data, VASurfaceID surface)
{
    int i, j;
    unsigned char* tmp_buffer;
    unsigned char tmp;
    object_surface_p obj_surface = SURFACE(surface);
    psb_surface_p psb_surface;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    PVR2DERROR ePVR2DStatus;

    CHECK_SURFACE(obj_surface);
    psb_surface = obj_surface->psb_surface;

    /* Find and return the wrapped buffer index */
    for (i = 0; i < VIDEO_BUFFER_NUM; i++) {
        if (driver_data->wrapped_surface_id[i] == surface && driver_data->videoBuf[i]) {
            return driver_data->videoBuf[i];
        }
    }

    /* Wrap a un-wrapped buffer and return */
    for (i = 0; i < VIDEO_BUFFER_NUM; i++) {
        if (driver_data->wrapped_surface_id[i] == VA_INVALID_ID) {
            tmp_buffer = NULL;
            tmp_buffer = wsbmBOMap(psb_surface->buf.drm_buf, WSBM_ACCESS_READ | WSBM_ACCESS_WRITE);
            if (NULL == tmp_buffer) {
                drv_debug_msg(VIDEO_DEBUG_ERROR, "%s L%d: wsbmBOMap failed!",
                        __FUNCTION__, __LINE__);
                return NULL;
            }
            for (j = 0; j < psb_surface->size; j = j + 4096) {
                tmp = *(tmp_buffer + j);
                if (tmp == 0)
                    *(tmp_buffer + j) = 0;
            }

            ePVR2DStatus = PVR2DMemWrap(driver_data->hPVR2DContext,
                                        tmp_buffer,
                                        0,
                                        psb_surface->size,
                                        NULL,
                                        &driver_data->videoBuf[i]);
            if (ePVR2DStatus != PVR2D_OK) {
                drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: PVR2DMemWrap error %d\n", __FUNCTION__, ePVR2DStatus);
            }

            driver_data->wrapped_surface_id[i] = surface;
            return driver_data->videoBuf[i];
        }
    }

    if (i == VIDEO_BUFFER_NUM - 1) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: Out of warpped buffer memory\n", __FUNCTION__);
        return NULL;
    }

    return NULL;
}

#if 0
void psb_putsurface_textureblit(
    VADriverContextP ctx, unsigned char *dst, VASurfaceID surface, int src_x, int src_y, int src_w,
    int src_h, int dst_x, int dst_y, int dst_w, int dst_h, unsigned int subtitle,
    int width, int height,
    int src_pitch, struct _WsbmBufferObject * src_buf,
    unsigned int placement, int wrap_dst)
{
    INIT_DRIVER_DATA;
    unsigned int i;
    unsigned char *tmp_palette;
    struct psb_texture_s *texture_priv = &driver_data->ctexture_priv;
    object_surface_p obj_surface;
    PsbVASurfaceRec *surface_subpic = NULL;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    obj_surface = SURFACE(surface);

    PVR2D_VPBLT sBltVP;
    PVR2DERROR ePVR2DStatus;
    PPVR2DMEMINFO pVaVideoSubpicMemInfo;
    PPVR2DMEMINFO pVaVideoMemInfo;
    PPVR2DMEMINFO pDstMeminfo;

    src_pitch = (src_pitch + 0x3) & ~0x3;

    if (NULL == obj_surface) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: Invalid surface ID 0x%08x!\n", __func__, surface);
        return;
    }
    surface_subpic = (PsbVASurfaceRec *)obj_surface->subpictures;
    /* check whether we need to update coeffs */
    if ((height > 576) &&
        (texture_priv->video_transfermatrix != PSB_VideoTransferMatrix_BT709)) {
        texture_priv->video_transfermatrix = PSB_VideoTransferMatrix_BT709;
        texture_priv->update_coeffs = 1;
    } else if ((height <= 576) &&
               (texture_priv->video_transfermatrix != PSB_VideoTransferMatrix_BT601)) {
        texture_priv->video_transfermatrix = PSB_VideoTransferMatrix_BT601;
        texture_priv->update_coeffs = 1;
    }

    /* prepare coeffs if needed */
    memset(&sBltVP, 0, sizeof(PVR2D_VPBLT));
    if (texture_priv->update_coeffs == 1) {
        psb_setup_coeffs(texture_priv);
        sBltVP.psYUVCoeffs = (PPVR2D_YUVCOEFFS) & texture_priv->coeffs;
        /* FIXME: is it right? */
        sBltVP.bCoeffsGiven  = 1;
    }

    pVaVideoMemInfo = psb_wrap_surface_pvr2dbuf(driver_data, surface);
    if (!pVaVideoMemInfo) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: Failed to get source PVR2DMEMINFO!\n", __func__);
        return;
    }

    /* wrap the dest source */
    /* FIXME: this is wrap for rgb565 */
    if (wrap_dst == 0) {
        /* comment out for rebasing to staging
        pDstMeminfo = (PPVR2DMEMINFO)dst;
        if (IS_MFLD(driver_data))
            sBltVP.sDst.Stride = PVRCalculateStride(((struct dri_drawable*)texture_priv->dri_drawable)->width, 32, 8);
        sBltVP.sDst.Format = PVR2D_ARGB8888;
        */
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: Not support ARGB8888!\n", __func__);
        return;
    } else {
        ePVR2DStatus = PVR2DMemWrap(driver_data->hPVR2DContext,
                                    dst,
                                    0,
                                    (dst_w * dst_h * 2),
                                    NULL,
                                    &pDstMeminfo);
        if (ePVR2DStatus != PVR2D_OK) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: PVR2DMemWrap error %d\n", __FUNCTION__, ePVR2DStatus);
            return;
        }

        /* FIXME: this wrong, how to get system pitch */
        sBltVP.sDst.Stride = dst_w * 2;//align_to(dst_w, 64);
        sBltVP.sDst.Format = PVR2D_RGB565;
    }
    sBltVP.sDst.pSurfMemInfo = pDstMeminfo;
    sBltVP.sDst.SurfOffset   = 0;
    sBltVP.sDst.SurfWidth = dst_w;
    sBltVP.sDst.SurfHeight = dst_h;

    /* Y plane UV plane */
    sBltVP.uiNumLayers = 1;
    sBltVP.sSrc->Stride = src_pitch;
    sBltVP.sSrc->Format = VA_FOURCC_NV12;
    sBltVP.sSrc->SurfWidth = width;
    sBltVP.sSrc->SurfHeight = height;
    sBltVP.sSrc[0].pSurfMemInfo = pVaVideoMemInfo;

    /* FIXME: check for top-bottom */
    sBltVP.sSrc->SurfOffset = 0;

    /* FIXME: check rotation setting */
    /* FIXME: use PVR define */
    sBltVP.RotationValue = 1;

    /* clip box */
    sBltVP.rcDest.left = dst_x;
    sBltVP.rcDest.right = dst_x + dst_w;
    sBltVP.rcDest.top = dst_y;
    sBltVP.rcDest.bottom = dst_y + dst_h;

    sBltVP.rcSource->left = src_x;
    sBltVP.rcSource->right = src_x + src_w;
    sBltVP.rcSource->top = src_y;
    sBltVP.rcSource->bottom = src_y + src_h;

    if (subtitle == 1 && obj_surface->subpic_count) {
        for (i = 0; i < obj_surface->subpic_count; i++) {
            sBltVP.uiNumLayers += 1;

            psb_calculate_subpic_size(obj_surface->width, obj_surface->height, dst_w, dst_h, surface_subpic);

            pVaVideoSubpicMemInfo = psb_check_subpic_buffer(driver_data, surface_subpic);
            if (!pVaVideoSubpicMemInfo) {
                drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: Failed to get subpic PVR2DMEMINFO!\n", __func__);
                return;
            }

            object_subpic_p obj_subpic = SUBPIC(surface_subpic->subpic_id);
            CHECK_SURFACE(obj_subpic);
            // sBltVP.AlphaBlendingFunc = PVR2D_ALPHA_OP_GLOBAL;
            sBltVP.AlphaBlendingFunc = 3;
            sBltVP.subpicGlobalAlpha[i] = obj_subpic->global_alpha;

            sBltVP.sSrcSubpic[i].pSurfMemInfo = pVaVideoSubpicMemInfo;
            sBltVP.sSrcSubpic[i].SurfOffset = 0;
            sBltVP.sSrcSubpic[i].Stride = surface_subpic->stride;

            if (surface_subpic->fourcc == VA_FOURCC_AI44)
                sBltVP.sSrcSubpic[i].Format = MAKEFOURCC('A', 'I' , '4', '4');
            else
                sBltVP.sSrcSubpic[i].Format = surface_subpic->fourcc;

            sBltVP.sSrcSubpic[i].SurfWidth = surface_subpic->subpic_srcw;
            sBltVP.sSrcSubpic[i].SurfHeight = surface_subpic->subpic_srch;

            sBltVP.rcSubpicSource[i].left = surface_subpic->subpic_srcx;
            sBltVP.rcSubpicSource[i].right = surface_subpic->subpic_srcx + surface_subpic->subpic_srcw;
            sBltVP.rcSubpicSource[i].top = surface_subpic->subpic_srcy;
            sBltVP.rcSubpicSource[i].bottom = surface_subpic->subpic_srcy + surface_subpic->subpic_srch;

            sBltVP.rcSubpicDest[i].left = surface_subpic->subpic_dstx;
            sBltVP.rcSubpicDest[i].right = surface_subpic->subpic_dstx + surface_subpic->subpic_dstw;
            sBltVP.rcSubpicDest[i].top = surface_subpic->subpic_dsty;
            sBltVP.rcSubpicDest[i].bottom = surface_subpic->subpic_dsty + surface_subpic->subpic_dsth;

            //only allocate memory once for palette
            if (surface_subpic->fourcc == VA_FOURCC_AI44) {
                if (!texture_priv->pal_meminfo[i]) {
                    ePVR2DStatus = PVR2DMemAlloc(driver_data->hPVR2DContext, 16 * sizeof(unsigned int), 0, 0, &texture_priv->pal_meminfo[i]);
                    if (ePVR2DStatus != PVR2D_OK) {
                        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: PVR2DMemAlloc error %d\n", __FUNCTION__, ePVR2DStatus);
                        return;
                    }
                }

                sBltVP.pPalMemInfo[i] = texture_priv->pal_meminfo[i];
                tmp_palette = sBltVP.pPalMemInfo[i]->pBase;
                memcpy(tmp_palette, surface_subpic->palette_ptr, 16 * sizeof(unsigned int));
                sBltVP.PalOffset[i] = 0;
            }
            surface_subpic = surface_subpic->next;
        }
    }

//#ifndef ANDROID /* MRST Android not enable this API, uncomment for MRST */
    ePVR2DStatus = PVR2DBltVideo(driver_data->hPVR2DContext, &sBltVP);
//#endif

    if (ePVR2DStatus != PVR2D_OK)
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: failed to do PVR2DBltVideo with error code %d\n",
                           __FUNCTION__, ePVR2DStatus);

    if (wrap_dst) {
        ePVR2DStatus = PVR2DMemFree(driver_data->hPVR2DContext, pDstMeminfo);
        if (ePVR2DStatus != PVR2D_OK)
            drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: PVR2DMemFree error %d\n", __FUNCTION__, ePVR2DStatus);
    }

    driver_data->cur_displaying_surface = VA_INVALID_SURFACE;
    driver_data->last_displaying_surface = VA_INVALID_SURFACE;
    obj_surface->display_timestamp = 0;
}
#endif

static void
psb_setup_coeffs(struct psb_texture_s * pPriv)
{
    double yCoeff, uCoeff, vCoeff, Constant;
    double fContrast;
    double Y_offset, CbCr_offset, RGB_offset;
    int bright_off = 0;
    psb_transform_coeffs coeffs, transfer_matrix;
    memset(&coeffs, 0, sizeof(psb_transform_coeffs));
    memset(&transfer_matrix, 0, sizeof(psb_transform_coeffs));

    /* Offsets in the input and output ranges are
     * included in the constant of the transform equation
     */
    psb_select_transfermatrix(pPriv, &transfer_matrix,
                              &Y_offset, &CbCr_offset, &RGB_offset);

    /*
     * It is at this point we should adjust the parameters for the procamp:
     * - Brightness is handled as an offset of the Y parameter.
     * - Contrast is an adjustment of the Y scale.
     * - Saturation is a scaling of the U anc V parameters.
     * - Hue is a rotation of the U and V parameters.
     */

    bright_off = pPriv->brightness.Value;
    fContrast = (pPriv->contrast.Value + 100) / 100.0;

    /* Apply hue and saturation correction to transfer matrix */
    psb_transform_sathuecoeffs(&coeffs,
                               &transfer_matrix,
                               pPriv->hue.Value * Degree,
                               pPriv->saturation.Value / 100.0);

    /* Create coefficients to get component R
     * (including brightness and contrast correction)
     */
    psb_create_coeffs(-1 * Y_offset, -1 * CbCr_offset, -1 * CbCr_offset,
                      RGB_offset, coeffs.rY, coeffs.rCb, coeffs.rCr,
                      bright_off, fContrast, &yCoeff, &uCoeff, &vCoeff,
                      &Constant);

    /* Convert transform operation from floating point to fixed point */
    psb_convert_coeffs(yCoeff, uCoeff, vCoeff, Constant,        /* input coefficients */
                       &pPriv->coeffs.rY, &pPriv->coeffs.rU,
                       &pPriv->coeffs.rV, &pPriv->coeffs.rConst,
                       &pPriv->coeffs.rShift);

    /* Create coefficients to get component G
     * (including brightness and contrast correction)
     */
    psb_create_coeffs(-1 * Y_offset, -1 * CbCr_offset, -1 * CbCr_offset,
                      RGB_offset, coeffs.gY, coeffs.gCb, coeffs.gCr,
                      bright_off, fContrast, &yCoeff, &uCoeff, &vCoeff,
                      &Constant);

    /* Convert transform operation from floating point to fixed point */
    psb_convert_coeffs(yCoeff, uCoeff, vCoeff, Constant,
                       /* tranfer matrix coefficients for G */
                       &pPriv->coeffs.gY, &pPriv->coeffs.gU,
                       &pPriv->coeffs.gV, &pPriv->coeffs.gConst,
                       &pPriv->coeffs.gShift);

    /* Create coefficients to get component B
     * (including brightness and contrast correction)
     */
    psb_create_coeffs(-1 * Y_offset, -1 * CbCr_offset, -1 * CbCr_offset,
                      RGB_offset, coeffs.bY, coeffs.bCb, coeffs.bCr,
                      bright_off, fContrast, &yCoeff, &uCoeff, &vCoeff,
                      &Constant);

    /* Convert transform operation from floating point to fixed point */
    psb_convert_coeffs(yCoeff, uCoeff, vCoeff, Constant,
                       /* tranfer matrix coefficients for B */
                       &pPriv->coeffs.bY, &pPriv->coeffs.bU,
                       &pPriv->coeffs.bV, &pPriv->coeffs.bConst,
                       &pPriv->coeffs.bShift);
}

/*
  These are the corresponding matrices when using NominalRange_16_235
  for the input surface and NominalRange_0_255 for the outpur surface:

  static const psb_transform_coeffs s601 = {
  1.164,                0,              1.596,
  1.164,                -0.391,         -0.813,
  1.164,                2.018,          0
  };

  static const psb_transform_coeffs s709 = {
  1.164,                0,              1.793,
  1.164,                -0.213,         -0.534,
  1.164,                2.115,          0
  };

  static const psb_transform_coeffs s240M = {
  1.164,                -0.0007,        1.793,
  1.164,                -0.257,         -0.542,
  1.164,                2.078,          0.0004
  };
*/

/**
 * Select which transfer matrix to use in the YUV->RGB conversion.
 */
static void
psb_select_transfermatrix(struct psb_texture_s * pPriv,
                          psb_transform_coeffs * transfer_matrix,
                          double *Y_offset, double *CbCr_offset,
                          double *RGB_offset)
{
    double RGB_scale, Y_scale, Cb_scale, Cr_scale;

    /*
     * Depending on the nominal ranges of the input YUV surface and the output RGB
     * surface, it might be needed to perform some scaling on the transfer matrix.
     * The excursion in the YUV values implies that the first column of the matrix
     * must be divided by the Y excursion, and the second and third columns be
     * divided by the U and V excursions respectively. The offset does not affect
     * the values of the matrix.
     * The excursion in the RGB values implies that all the values in the transfer
     * matrix must be multiplied by the value of the excursion.
     *
     * Example: Conversion of the SMPTE 240M transfer matrix.
     *
     * Conversion from [Y', Pb, Pr] to [R', G', B'] in the range of [0, 1]. Y' is in
     * the range of [0, 1]      and Pb and Pr in the range of [-0.5, 0.5].
     *
     * R'               1       -0.000657       1.575848                Y'
     * G'       =       1       -0.226418       -0.476529       *       Pb
     * B'               1       1.825958        0.000378                Pr
     *
     * Conversion from [Y', Cb, Cr] to {R', G', B'] in the range of [0, 1]. Y' has an
     * excursion of 219 and an offset of +16, and CB and CR have excursions of +/-112
     * and offset of +128, for a range of 16 through 240 inclusive.
     *
     * R'               1/219   -0.000657/224   1.575848/224            Y'       16
     * G'       =       1/219   -0.226418/224   -0.476529/224   *       Cb - 128
     * B'               1/219   1.825958/224    0.000378/224            Cr   128
     *
     * Conversion from [Y', Cb, Cr] to R'G'B' in the range [0, 255].
     *
     * R'                         1/219 -0.000657/224 1.575848/224                      Y'       16
     * G'       =       255 * 1/219     -0.226418/224 -0.476529/224             *       Cb - 128
     * B'                         1/219 1.825958/224  0.000378/224                      Cr   128
     */

    switch (pPriv->src_nominalrange) {
    case PSB_NominalRange_0_255:
        /* Y has a range of [0, 255], U and V have a range of [0, 255] */
    {
        double tmp = 0.0;

        (void)tmp;
    }                          /* workaroud for float point bug? */
    Y_scale = 255.0;
    *Y_offset = 0;
    Cb_scale = Cr_scale = 255;
    *CbCr_offset = 128;
    break;
    case PSB_NominalRange_16_235:
    case PSB_NominalRange_Unknown:
        /* Y has a range of [16, 235] and Cb, Cr have a range of [16, 240] */
        Y_scale = 219;
        *Y_offset = 16;
        Cb_scale = Cr_scale = 224;
        *CbCr_offset = 128;
        break;
    case PSB_NominalRange_48_208:
        /* Y has a range of [48, 208] and Cb, Cr have a range of [48, 208] */
        Y_scale = 160;
        *Y_offset = 48;
        Cb_scale = Cr_scale = 160;
        *CbCr_offset = 128;
        break;

    default:
        /* Y has a range of [0, 1], U and V have a range of [-0.5, 0.5] */
        Y_scale = 1;
        *Y_offset = 0;
        Cb_scale = Cr_scale = 1;
        *CbCr_offset = 0;
        break;
    }

    /*
     * 8-bit computer RGB,      also known as sRGB or "full-scale" RGB, and studio
     * video RGB, or "RGB with  head-room and toe-room." These are defined as follows:
     *
     * - Computer RGB uses 8 bits for each sample of red, green, and blue. Black
     * is represented by R = G = B = 0, and white is represented by R = G = B = 255.
     * - Studio video RGB uses some number of bits N for each sample of red, green,
     * and blue, where N is 8 or more. Studio video RGB uses a different scaling
     * factor than computer RGB, and it has an offset. Black is represented by
     * R = G = B = 16*2^(N-8), and white is represented by R = G = B = 235*2^(N-8).
     * However, actual values may fall outside this range.
     */
    switch (pPriv->dst_nominalrange) {
    case PSB_NominalRange_0_255:      // for sRGB
    case PSB_NominalRange_Unknown:
        /* R, G and B have a range of [0, 255] */
        RGB_scale = 255;
        *RGB_offset = 0;
        break;
    case PSB_NominalRange_16_235:     // for stRGB
        /* R, G and B have a range of [16, 235] */
        RGB_scale = 219;
        *RGB_offset = 16;
        break;
    case PSB_NominalRange_48_208:     // for Bt.1361 RGB
        /* R, G and B have a range of [48, 208] */
        RGB_scale = 160;
        *RGB_offset = 48;
        break;
    default:
        /* R, G and B have a range of [0, 1] */
        RGB_scale = 1;
        *RGB_offset = 0;
        break;
    }

    switch (pPriv->video_transfermatrix) {
    case PSB_VideoTransferMatrix_BT709:
        memcpy(transfer_matrix, &s709, sizeof(psb_transform_coeffs));
        break;
    case PSB_VideoTransferMatrix_BT601:
        memcpy(transfer_matrix, &s601, sizeof(psb_transform_coeffs));
        break;
    case PSB_VideoTransferMatrix_SMPTE240M:
        memcpy(transfer_matrix, &s240M, sizeof(psb_transform_coeffs));
        break;
    case PSB_VideoTransferMatrix_Unknown:
        /*
         * Specifies that the video transfer matrix is not specified.
         * The default value is BT601 for standard definition (SD) video and BT709
         * for high definition (HD) video.
         */
        if (1 /*pPriv->sVideoDesc.SampleWidth < 720 */) {       /* TODO, width selection */
            memcpy(transfer_matrix, &s601, sizeof(psb_transform_coeffs));
        } else {
            memcpy(transfer_matrix, &s709, sizeof(psb_transform_coeffs));
        }
        break;
    default:
        break;
    }

    if (Y_scale != 1 || Cb_scale != 1 || Cr_scale != 1) {
        /* Each column of the transfer matrix has to
         * be scaled by the excursion of each component
         */
        psb_scale_transfermatrix(transfer_matrix, 1 / Y_scale, 1 / Cb_scale,
                                 1 / Cr_scale);
    }
    if (RGB_scale != 1) {
        /* All the values in the transfer matrix have to be multiplied
         * by the excursion of the RGB components
         */
        psb_scale_transfermatrix(transfer_matrix, RGB_scale, RGB_scale,
                                 RGB_scale);
    }
}

static void
psb_scale_transfermatrix(psb_transform_coeffs * transfer_matrix,
                         double YColumScale, double CbColumScale,
                         double CrColumnScale)
{
    /* First column of the transfer matrix */
    transfer_matrix->rY *= YColumScale;
    transfer_matrix->gY *= YColumScale;
    transfer_matrix->bY *= YColumScale;

    /* Second column of the transfer matrix */
    transfer_matrix->rCb *= CbColumScale;
    transfer_matrix->gCb *= CbColumScale;
    transfer_matrix->bCb *= CbColumScale;

    /* Third column of the transfer matrix */
    transfer_matrix->rCr *= CrColumnScale;
    transfer_matrix->gCr *= CrColumnScale;
    transfer_matrix->bCr *= CrColumnScale;
}

/*
 * Calculates the coefficintes of a YUV->RGB conversion based on
 * the provided basis coefficients (already had HUe and Satu applied).
 * Performs brightness and contrast adjustment as well as the required
 * offsets to put into correct range for hardware conversion.
 */
static void
psb_create_coeffs(double yOff, double uOff, double vOff, double rgbOff,
                  double yScale, double uScale, double vScale,
                  double brightness, double contrast,
                  double *pYCoeff, double *pUCoeff, double *pVCoeff,
                  double *pConstant)
{
    *pYCoeff = yScale * contrast;
    *pUCoeff = uScale * contrast;
    *pVCoeff = vScale * contrast;

    *pConstant = (((yOff + brightness) * yScale)
                  + (uOff * uScale) + (vOff * vScale)) * contrast + rgbOff;
}

/*
 * Converts a floating point function in the form
 *    a*yCoeff + b*uCoeff + c * vCoeff + d
 *  Into a fixed point function of the forrm
 *   (a*pY + b * pU + c * pV + constant)>>pShift
 */
static void
psb_convert_coeffs(double Ycoeff, double Ucoeff, double Vcoeff,
                   double ConstantTerm, signed char *pY, signed char *pU,
                   signed char *pV, signed short *constant,
                   unsigned char *pShift)
{
    *pShift = 0;

    Ycoeff *= 256;
    Ucoeff *= 256;
    Vcoeff *= 256;
    ConstantTerm *= 256;
    *pShift = 8;

    /*
     * What we want to do is scale up the coefficients so that they just fit into their
     * allowed bits, so we are using signed maths giving us coefficients can be between +-128.
     * The constant can be between =- 32767.
     * The divide can be between 0 and 256 (on powers of two only).
     * A mathematical approach would be nice, but for simplicity do an iterative compare
     * and divide. Until something fits.
     */
    while (psb_check_coeffs(Ycoeff, Ucoeff, Vcoeff, ConstantTerm, *pShift)) {
        Ycoeff /= 2;
        Ucoeff /= 2;
        Vcoeff /= 2;
        ConstantTerm /= 2;
        (*pShift)--;
    }
    *pY = (signed char)(Ycoeff + 0.5);
    *pU = (signed char)(Ucoeff + 0.5);
    *pV = (signed char)(Vcoeff + 0.5);
    *constant = (signed short)(ConstantTerm + 0.5);
}

/**
 * Checks if the specified coefficients are within the ranges required
 * and returns true if they are else false.
 */
static int
psb_check_coeffs(double Ycoeff, double Ucoeff, double Vcoeff,
                 double ConstantTerm, signed char byShift)
{
    if ((Ycoeff > 127) || (Ycoeff < -128)) {
        return 1;
    }
    if ((Ucoeff > 127) || (Ucoeff < -128)) {
        return 1;
    }
    if ((Vcoeff > 127) || (Vcoeff < -128)) {
        return 1;
    }
    if ((ConstantTerm > 32766) || (ConstantTerm < -32767)) {
        return 1;
    }
    return 0;
}

static void
psb_transform_sathuecoeffs(psb_transform_coeffs * dest,
                           const psb_transform_coeffs * const source,
                           double fHue, double fSat)
{
    double fHueSatSin, fHueSatCos;

    fHueSatSin = sin(fHue) * fSat;
    fHueSatCos = cos(fHue) * fSat;

    dest->rY = source->rY;
    dest->rCb = source->rCb * fHueSatCos - source->rCr * fHueSatSin;
    dest->rCr = source->rCr * fHueSatCos + source->rCb * fHueSatSin;

    dest->gY = source->gY;
    dest->gCb = source->gCb * fHueSatCos - source->gCr * fHueSatSin;
    dest->gCr = source->gCr * fHueSatCos + source->gCb * fHueSatSin;

    dest->bY = source->bY;
    dest->bCb = source->bCb * fHueSatCos - source->bCr * fHueSatSin;
    dest->bCr = source->bCr * fHueSatCos + source->bCb * fHueSatSin;
}

