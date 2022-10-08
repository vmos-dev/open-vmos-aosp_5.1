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
 *    Binglin Chen <binglin.chen@intel.com>
 *    Jason Hu <jason.hu@intel.com>
 *    Zeng Li <zeng.li@intel.com>
 */

/*
 * Most of rendering codes are ported from xf86-video-i810/src/i810_overlay.c
 */

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <va/va_backend.h>
#include <wsbm/wsbm_manager.h>
#include "psb_drv_video.h"
#include "psb_output.h"
#include "psb_overlay.h"
#include "psb_drv_debug.h"

#ifdef ANDROID
#define psb_xrandr_single_mode() 0
#else
int psb_xrandr_single_mode();
#endif

#define INIT_DRIVER_DATA    psb_driver_data_p driver_data = (psb_driver_data_p) ctx->pDriverData
#define SURFACE(id) ((object_surface_p) object_heap_lookup( &driver_data->surface_heap, id ))
#define CONTEXT(id) ((object_context_p) object_heap_lookup( &driver_data->context_heap, id ))
//#define GET_SURFACE_INFO_rotate(psb_surface) ((int) (psb_surface)->extra_info[5])

#ifndef VA_FOURCC_I420
#define VA_FOURCC_I420          0x30323449
#endif

/**********************************************************************************************
 * I830ResetVideo
 *
 * Description: Use this function to reset the overlay register back buffer to its default
 * values.  Note that this function does not actually apply these values.  To do so, please
 * write to OVADD.
 **********************************************************************************************/
static void
I830ResetVideo(VADriverContextP ctx, PsbPortPrivPtr pPriv)
{
    INIT_DRIVER_DATA;
    I830OverlayRegPtr overlayA = (I830OverlayRegPtr)(pPriv->regmap[0]);
    I830OverlayRegPtr overlayC = (I830OverlayRegPtr)(pPriv->regmap[1]);

    memset(overlayA, 0, sizeof(*overlayA));
    memset(overlayC, 0, sizeof(*overlayC));

    overlayA->OCLRC0 = (pPriv->contrast.Value << 18) | (pPriv->brightness.Value & 0xff);
    overlayA->OCLRC1 = pPriv->saturation.Value;

    overlayC->OCLRC0 = (pPriv->contrast.Value << 18) | (pPriv->brightness.Value & 0xff);
    overlayC->OCLRC1 = pPriv->saturation.Value;

#if USE_DCLRK
    /* case bit depth 16 */
    overlayA->DCLRKV = pPriv->colorKey;
    overlayA->DCLRKM |= DEST_KEY_ENABLE;
    overlayA->DCLRKM &= ~CONST_ALPHA_ENABLE;

    overlayC->DCLRKV = pPriv->colorKey;
    overlayC->DCLRKM |= DEST_KEY_ENABLE;
    overlayC->DCLRKM &= ~CONST_ALPHA_ENABLE;
#else
    overlayA->DCLRKM &= ~DEST_KEY_ENABLE;
    overlayC->DCLRKM &= ~DEST_KEY_ENABLE;
#endif
    overlayA->DWINSZ = 0x00000000;
    overlayA->OCONFIG = CC_OUT_8BIT;

    overlayC->DWINSZ = 0x00000000;
    overlayC->OCONFIG = CC_OUT_8BIT;
}

static uint32_t I830BoundGammaElt(uint32_t elt, uint32_t eltPrev)
{
    elt &= 0xff;
    eltPrev &= 0xff;
    if (elt < eltPrev)
        elt = eltPrev;
    else if ((elt - eltPrev) > 0x7e)
        elt = eltPrev + 0x7e;
    return elt;
}

static uint32_t I830BoundGamma(uint32_t gamma, uint32_t gammaPrev)
{
    return (I830BoundGammaElt(gamma >> 24, gammaPrev >> 24) << 24 |
            I830BoundGammaElt(gamma >> 16, gammaPrev >> 16) << 16 |
            I830BoundGammaElt(gamma >>  8, gammaPrev >>  8) <<  8 |
            I830BoundGammaElt(gamma      , gammaPrev));
}

static void
I830UpdateGamma(VADriverContextP ctx, PsbPortPrivPtr pPriv)
{
#ifndef BAYTRAIL
    INIT_DRIVER_DATA;
    uint32_t gamma0 = pPriv->gamma0;
    uint32_t gamma1 = pPriv->gamma1;
    uint32_t gamma2 = pPriv->gamma2;
    uint32_t gamma3 = pPriv->gamma3;
    uint32_t gamma4 = pPriv->gamma4;
    uint32_t gamma5 = pPriv->gamma5;
    struct drm_psb_register_rw_arg regs;

    gamma1 = I830BoundGamma(gamma1, gamma0);
    gamma2 = I830BoundGamma(gamma2, gamma1);
    gamma3 = I830BoundGamma(gamma3, gamma2);
    gamma4 = I830BoundGamma(gamma4, gamma3);
    gamma5 = I830BoundGamma(gamma5, gamma4);

    memset(&regs, 0, sizeof(regs));
    if (pPriv->is_mfld)
        regs.overlay_write_mask |= OV_REGRWBITS_OGAM_ALL | OVC_REGRWBITS_OGAM_ALL;
    else
        regs.overlay_write_mask |= OV_REGRWBITS_OGAM_ALL;
    regs.overlay.OGAMC0 = gamma0;
    regs.overlay.OGAMC1 = gamma1;
    regs.overlay.OGAMC2 = gamma2;
    regs.overlay.OGAMC3 = gamma3;
    regs.overlay.OGAMC4 = gamma4;
    regs.overlay.OGAMC5 = gamma5;
    drmCommandWriteRead(driver_data->drm_fd, DRM_PSB_REGISTER_RW, &regs, sizeof(regs));
#endif
}

static void I830StopVideo(VADriverContextP ctx)
{
#ifndef BAYTRAIL
    INIT_DRIVER_DATA;
    PsbPortPrivPtr pPriv = (PsbPortPrivPtr)(&driver_data->coverlay_priv);
    I830OverlayRegPtr overlayA, overlayC;
    struct drm_psb_register_rw_arg regs;

    if (!pPriv->overlayA_enabled && !pPriv->overlayC_enabled) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "I830StopVideo : no overlay has been enabled, do nothing.\n");
        return;
    }

    overlayA = (I830OverlayRegPtr)(pPriv->regmap[0]);
    overlayC = (I830OverlayRegPtr)(pPriv->regmap[1]);
#if 0
    REGION_EMPTY(pScrn->pScreen, &pPriv->clip);
#endif
    memset(&regs, 0, sizeof(regs));
    if (pPriv->subpicture_enabled) {
        regs.subpicture_disable_mask = pPriv->subpicture_enable_mask;
        pPriv->subpicture_enabled = 0;
        drmCommandWriteRead(driver_data->drm_fd, DRM_PSB_REGISTER_RW, &regs, sizeof(regs));
    }

    memset(&regs, 0, sizeof(regs));

    if (pPriv->is_mfld && psb_xrandr_single_mode() == 0) {
        if (pPriv->overlayC_enabled) {
            regs.overlay_read_mask = OVC_REGRWBITS_OVADD;
            drmCommandWriteRead(driver_data->drm_fd, DRM_PSB_REGISTER_RW, &regs, sizeof(regs));

            overlayC->DWINSZ = 0x00000000;
            overlayC->OCMD &= ~OVERLAY_ENABLE;
            regs.overlay_read_mask = 0;
            regs.overlay_write_mask = OVC_REGRWBITS_OVADD;
            regs.overlay.b_wait_vblank = 1;
            drmCommandWriteRead(driver_data->drm_fd, DRM_PSB_REGISTER_RW, &regs, sizeof(regs));

            memset(&regs, 0, sizeof(regs));
            pPriv->overlayC_enabled = 0;
        }
        if (pPriv->overlayA_enabled) {
            regs.overlay_read_mask = OV_REGRWBITS_OVADD;
            drmCommandWriteRead(driver_data->drm_fd, DRM_PSB_REGISTER_RW, &regs, sizeof(regs));

            overlayA->DWINSZ = 0x00000000;
            overlayA->OCMD &= ~OVERLAY_ENABLE;
            regs.overlay_read_mask = 0;
            regs.overlay_write_mask = OV_REGRWBITS_OVADD;
            regs.overlay.b_wait_vblank = 1;
            drmCommandWriteRead(driver_data->drm_fd, DRM_PSB_REGISTER_RW, &regs, sizeof(regs));
            pPriv->overlayA_enabled = 0;
        }
    } else {
        regs.overlay_read_mask = OV_REGRWBITS_OVADD;
        drmCommandWriteRead(driver_data->drm_fd, DRM_PSB_REGISTER_RW, &regs, sizeof(regs));

        overlayA->DWINSZ = 0x00000000;
        overlayA->OCMD &= ~OVERLAY_ENABLE;
        regs.overlay_read_mask = 0;
        regs.overlay_write_mask = OV_REGRWBITS_OVADD;
        regs.overlay.b_wait_vblank = 1;
        drmCommandWriteRead(driver_data->drm_fd, DRM_PSB_REGISTER_RW, &regs, sizeof(regs));
        pPriv->overlayA_enabled = 0;
    }
#endif
}

#if 0
static void I830SwitchPipe(VADriverContextP ctx , int overlayId, int pipeId)
{
    INIT_DRIVER_DATA;
    PsbPortPrivPtr pPriv = (PsbPortPrivPtr)(&driver_data->coverlay_priv);
    I830OverlayRegPtr overlay = (I830OverlayRegPtr)(pPriv->regmap[overlayId]);
    struct drm_psb_register_rw_arg regs;
    uint32_t overlay_mask;

    if ((overlayId == OVERLAY_A) && pPriv->overlayA_enabled)
        overlay_mask = OV_REGRWBITS_OVADD;
    else if ((overlayId == OVERLAY_C) && pPriv->overlayC_enabled)
        overlay_mask = OVC_REGRWBITS_OVADD;
    else
        return;  /*No overlay enabled, do nothing.*/

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Overlay %d switch to pipe %d\n", overlayId, pipeId);
    memset(&regs, 0, sizeof(regs));
    memset(overlay, 0, sizeof(*overlay));
    overlay->OCLRC0 = (pPriv->contrast.Value << 18) | (pPriv->brightness.Value & 0xff);
    overlay->OCLRC1 = pPriv->saturation.Value;

    /* case bit depth 16 */
    overlay->DCLRKV = pPriv->colorKey;
    overlay->DCLRKM |= DEST_KEY_ENABLE;
    overlay->DCLRKM &= ~CONST_ALPHA_ENABLE;
    overlay->DWINSZ = 0x00000000;
    overlay->OCONFIG = CC_OUT_8BIT;

    regs.overlay_read_mask = overlay_mask;
    drmCommandWriteRead(driver_data->drm_fd, DRM_PSB_REGISTER_RW, &regs, sizeof(regs));

    switch (pipeId) {
    case PIPEA:
        overlay->OCONFIG |= OVERLAY_C_PIPE_A;
        break;
    case PIPEB:
        overlay->OCONFIG |= OVERLAY_C_PIPE_B;
        break;
    case PIPEC:
        overlay->OCONFIG |= OVERLAY_C_PIPE_C;
        break;
    }
    regs.overlay_read_mask = 0;
    regs.overlay_write_mask = overlay_mask;
    regs.overlay.b_wait_vblank = 1;
    drmCommandWriteRead(driver_data->drm_fd, DRM_PSB_REGISTER_RW, &regs, sizeof(regs));
}
#endif

static int
i830_swidth(unsigned int offset, unsigned int width, unsigned int mask, int shift)
{
    int swidth = ((offset + width + mask) >> shift) - (offset >> shift);
    swidth <<= 1;
    swidth -= 1;
    return swidth << 2;
}

static Bool
SetCoeffRegs(double *coeff, int mantSize, coeffPtr pCoeff, int pos)
{
    int maxVal, icoeff, res;
    int sign;
    double c;

    sign = 0;
    maxVal = 1 << mantSize;
    c = *coeff;
    if (c < 0.0) {
        sign = 1;
        c = -c;
    }

    res = 12 - mantSize;
    if ((icoeff = (int)(c * 4 * maxVal + 0.5)) < maxVal) {
        pCoeff[pos].exponent = 3;
        pCoeff[pos].mantissa = icoeff << res;
        *coeff = (double)icoeff / (double)(4 * maxVal);
    } else if ((icoeff = (int)(c * 2 * maxVal + 0.5)) < maxVal) {
        pCoeff[pos].exponent = 2;
        pCoeff[pos].mantissa = icoeff << res;
        *coeff = (double)icoeff / (double)(2 * maxVal);
    } else if ((icoeff = (int)(c * maxVal + 0.5)) < maxVal) {
        pCoeff[pos].exponent = 1;
        pCoeff[pos].mantissa = icoeff << res;
        *coeff = (double)icoeff / (double)(maxVal);
    } else if ((icoeff = (int)(c * maxVal * 0.5 + 0.5)) < maxVal) {
        pCoeff[pos].exponent = 0;
        pCoeff[pos].mantissa = icoeff << res;
        *coeff = (double)icoeff / (double)(maxVal / 2);
    } else {
        /* Coeff out of range */
        return FALSE;
    }

    pCoeff[pos].sign = sign;
    if (sign)
        *coeff = -(*coeff);
    return TRUE;
}

static void
UpdateCoeff(int taps, double fCutoff, Bool isHoriz, Bool isY, coeffPtr pCoeff)
{
    int i, j, j1, num, pos, mantSize;
    double pi = 3.1415926535, val, sinc, window, sum;
    double rawCoeff[MAX_TAPS * 32], coeffs[N_PHASES][MAX_TAPS];
    double diff;
    int tapAdjust[MAX_TAPS], tap2Fix;
    Bool isVertAndUV;

    if (isHoriz)
        mantSize = 7;
    else
        mantSize = 6;

    isVertAndUV = !isHoriz && !isY;
    num = taps * 16;
    for (i = 0; i < num  * 2; i++) {
        val = (1.0 / fCutoff) * taps * pi * (i - num) / (2 * num);
        if (val == 0.0)
            sinc = 1.0;
        else
            sinc = sin(val) / val;

        /* Hamming window */
        window = (0.5 - 0.5 * cos(i * pi / num));
        rawCoeff[i] = sinc * window;
    }

    for (i = 0; i < N_PHASES; i++) {
        /* Normalise the coefficients. */
        sum = 0.0;
        for (j = 0; j < taps; j++) {
            pos = i + j * 32;
            sum += rawCoeff[pos];
        }
        for (j = 0; j < taps; j++) {
            pos = i + j * 32;
            coeffs[i][j] = rawCoeff[pos] / sum;
        }

        /* Set the register values. */
        for (j = 0; j < taps; j++) {
            pos = j + i * taps;
            if ((j == (taps - 1) / 2) && !isVertAndUV)
                SetCoeffRegs(&coeffs[i][j], mantSize + 2, pCoeff, pos);
            else
                SetCoeffRegs(&coeffs[i][j], mantSize, pCoeff, pos);
        }

        tapAdjust[0] = (taps - 1) / 2;
        for (j = 1, j1 = 1; j <= tapAdjust[0]; j++, j1++) {
            tapAdjust[j1] = tapAdjust[0] - j;
            tapAdjust[++j1] = tapAdjust[0] + j;
        }

        /* Adjust the coefficients. */
        sum = 0.0;
        for (j = 0; j < taps; j++)
            sum += coeffs[i][j];
        if (sum != 1.0) {
            for (j1 = 0; j1 < taps; j1++) {
                tap2Fix = tapAdjust[j1];
                diff = 1.0 - sum;
                coeffs[i][tap2Fix] += diff;
                pos = tap2Fix + i * taps;
                if ((tap2Fix == (taps - 1) / 2) && !isVertAndUV)
                    SetCoeffRegs(&coeffs[i][tap2Fix], mantSize + 2, pCoeff, pos);
                else
                    SetCoeffRegs(&coeffs[i][tap2Fix], mantSize, pCoeff, pos);

                sum = 0.0;
                for (j = 0; j < taps; j++)
                    sum += coeffs[i][j];
                if (sum == 1.0)
                    break;
            }
        }
    }
}

static void
i830_display_video(
    VADriverContextP ctx, PsbPortPrivPtr pPriv, VASurfaceID __maybe_unused surface,
    int id, short width, short height,
    int dstPitch, int srcPitch, int __maybe_unused x1, int __maybe_unused y1, int __maybe_unused x2, int __maybe_unused y2, BoxPtr dstBox,
    short src_w, short src_h, short drw_w, short drw_h,
    unsigned int flags, int overlayId, int pipeId)
{
#ifndef BAYTRAIL
    INIT_DRIVER_DATA;
    unsigned int        swidth, swidthy, swidthuv;
    unsigned int        mask, shift, offsety, offsetu;
    int                 tmp;
    uint32_t            OCMD;
    Bool                scaleChanged = FALSE;
    unsigned int        offset = wsbmBOOffsetHint(pPriv->wsbo[overlayId]) & 0x0FFFFFFF;
    I830OverlayRegPtr   overlay = (I830OverlayRegPtr)(pPriv->regmap[overlayId]);
    struct drm_psb_register_rw_arg regs;
    int i32EnableIEP = 0;
    int i32EnableIEPBLE = 0;

    /*before enabling overlay, make sure overlay is disabled first.*/
    if ((overlayId == OVERLAY_A) && !pPriv->overlayA_enabled) {
        memset(&regs, 0, sizeof(regs));
        regs.overlay_read_mask = OV_REGRWBITS_OVADD;
        drmCommandWriteRead(driver_data->drm_fd, DRM_PSB_REGISTER_RW, &regs, sizeof(regs));

        overlay->OCMD &= ~OVERLAY_ENABLE;
        regs.overlay_read_mask = 0;
        regs.overlay_write_mask = OV_REGRWBITS_OVADD;
        drmCommandWriteRead(driver_data->drm_fd, DRM_PSB_REGISTER_RW, &regs, sizeof(regs));
    }

    /* FIXME: don't know who and why add this
     *        comment it for full screen scale issue
     *        any concern contact qiang.miao@intel.com
     */
#if 0
    if (drw_w >= 800) {
        x2 = x2 / 4;
        y2 = y2 / 4;
        dstBox->x2 = dstBox->x2 / 4;
        dstBox->y2 = dstBox->y2 / 4;
        drw_w = drw_w / 4;
        drw_h = drw_h / 4;
    }
#endif

#if USE_DCLRK
    overlay->DCLRKM &= ~CONST_ALPHA_ENABLE;
    if (pPriv->subpicture_enabled)
        overlay->DCLRKM &= ~DEST_KEY_ENABLE;
    else
        overlay->DCLRKM |= DEST_KEY_ENABLE;
    overlay->DCLRKV = pPriv->colorKey;
    overlay->DCLRKM |= 0xffffff;
#else
    /* disable overlay destination color key didn't work,
    * it seems z-order of overlay has been bellow display pipe.
    */
    overlay->DCLRKM &= ~DEST_KEY_ENABLE;
#endif

#if USE_ROTATION_FUNC
    if (((pipeId == PIPEA) && (driver_data->mipi0_rotation != VA_ROTATION_NONE)) ||
        ((pipeId == PIPEB) && (driver_data->hdmi_rotation != VA_ROTATION_NONE))) {
        switch (pPriv->rotation) {
        case VA_ROTATION_NONE:
            break;
        case VA_ROTATION_270:
            tmp = dstBox->x1;
            dstBox->x1 = dstBox->y1;
            dstBox->y1 = pPriv->height_save - tmp;
            tmp = dstBox->x2;
            dstBox->x2 = dstBox->y2;
            dstBox->y2 = pPriv->height_save - tmp;
            tmp = dstBox->y1;
            dstBox->y1 = dstBox->y2;
            dstBox->y2 = tmp;
            tmp = drw_w;
            drw_w = drw_h;
            drw_h = tmp;
            break;
        case VA_ROTATION_180:
            tmp = dstBox->x1;
            dstBox->x1 = pPriv->width_save - dstBox->x2;
            dstBox->x2 = pPriv->width_save - tmp;
            tmp = dstBox->y1;
            dstBox->y1 = pPriv->height_save - dstBox->y2;
            dstBox->y2 = pPriv->height_save - tmp;
            break;
        case VA_ROTATION_90:
            tmp = dstBox->x1;
            dstBox->x1 = pPriv->width_save - dstBox->y1;
            dstBox->y1 = tmp;
            tmp = dstBox->x2;
            dstBox->x2 = pPriv->width_save - dstBox->y2;
            dstBox->y2 = tmp;
            tmp = dstBox->x1;
            dstBox->x1 = dstBox->x2;
            dstBox->x2 = tmp;
            tmp = drw_w;
            drw_w = drw_h;
            drw_h = tmp;
            break;
        }
    }
#endif

    if (pPriv->oneLineMode) {
        /* change the coordinates with panel fitting active */
        dstBox->y1 = (((dstBox->y1 - 1) * pPriv->scaleRatio) >> 16) + 1;
        dstBox->y2 = ((dstBox->y2 * pPriv->scaleRatio) >> 16) + 1;

        /* Now, alter the height, so we scale to the correct size */
        drw_h = ((drw_h * pPriv->scaleRatio) >> 16) + 1;
    }

    shift = 6;
    mask = 0x3f;

    if (pPriv->curBuf == 0) {
        offsety = pPriv->YBuf0offset;
        offsetu = pPriv->UBuf0offset;
    } else {
        offsety = pPriv->YBuf1offset;
        offsetu = pPriv->UBuf1offset;
    }

    switch (id) {
    case VA_FOURCC_NV12:
        overlay->SWIDTH = width | ((width / 2 & 0x7ff) << 16);
        swidthy = i830_swidth(offsety, width, mask, shift);
        swidthuv = i830_swidth(offsetu, width / 2, mask, shift);
        overlay->SWIDTHSW = (swidthy) | (swidthuv << 16);
        overlay->SHEIGHT = height | ((height / 2) << 16);
        break;
    case VA_FOURCC_YV12:
    case VA_FOURCC_I420:
        overlay->SWIDTH = width | ((width / 2 & 0x7ff) << 16);
        swidthy  = i830_swidth(offsety, width, mask, shift);
        swidthuv = i830_swidth(offsetu, width / 2, mask, shift);
        overlay->SWIDTHSW = (swidthy) | (swidthuv << 16);
        overlay->SHEIGHT = height | ((height / 2) << 16);
        break;
    case VA_FOURCC_UYVY:
    case VA_FOURCC_YUY2:
    default:
        overlay->SWIDTH = width;
        swidth = ((offsety + (width << 1) + mask) >> shift) -
                 (offsety >> shift);

        swidth <<= 1;
        swidth -= 1;
        swidth <<= 2;

        overlay->SWIDTHSW = swidth;
        overlay->SHEIGHT = height;
        break;
    }

    overlay->DWINPOS = (dstBox->y1 << 16) | dstBox->x1;

    overlay->DWINSZ = (((dstBox->y2 - dstBox->y1) << 16) |
                       (dstBox->x2 - dstBox->x1));

    /* buffer locations */
    overlay->OBUF_0Y = pPriv->YBuf0offset;
    overlay->OBUF_0U = pPriv->UBuf0offset;
    overlay->OBUF_0V = pPriv->VBuf0offset;
    overlay->OBUF_1Y = pPriv->YBuf1offset;
    overlay->OBUF_1U = pPriv->UBuf1offset;
    overlay->OBUF_1V = pPriv->VBuf1offset;

    /*
     * Calculate horizontal and vertical scaling factors and polyphase
     * coefficients.
     */

    if (1) {
        int xscaleInt, xscaleFract, yscaleInt, yscaleFract;
        int xscaleIntUV, xscaleFractUV;
        int yscaleIntUV, yscaleFractUV;
        /* UV is half the size of Y -- YUV420 */
        int uvratio = 2;
        uint32_t newval;
        coeffRec xcoeffY[N_HORIZ_Y_TAPS * N_PHASES];
        coeffRec xcoeffUV[N_HORIZ_UV_TAPS * N_PHASES];
        int i, j, pos;
        int deinterlace_factor;

        /*
         * Y down-scale factor as a multiple of 4096.
         */
        if ((id == VA_FOURCC_NV12) && (0 != (flags & (VA_TOP_FIELD | VA_BOTTOM_FIELD))))
            deinterlace_factor = 2;
        else
            deinterlace_factor = 1;

        /* deinterlace requires twice of VSCALE setting*/
        if (src_w == drw_w && src_h == drw_h) {
            xscaleFract = 1 << 12;
            yscaleFract = (1 << 12) / deinterlace_factor;
        } else {
            xscaleFract = ((src_w - 1) << 12) / drw_w;
            yscaleFract = ((src_h - 1) << 12) / (deinterlace_factor * drw_h);
        }

        /* Calculate the UV scaling factor. */
        xscaleFractUV = xscaleFract / uvratio;
        yscaleFractUV = yscaleFract / uvratio;

        /*
         * To keep the relative Y and UV ratios exact, round the Y scales
         * to a multiple of the Y/UV ratio.
         */
        xscaleFract = xscaleFractUV * uvratio;
        yscaleFract = yscaleFractUV * uvratio;

        /* Integer (un-multiplied) values. */
        xscaleInt = xscaleFract >> 12;
        yscaleInt = yscaleFract >> 12;

        xscaleIntUV = xscaleFractUV >> 12;
        yscaleIntUV = yscaleFractUV >> 12;

        /* shouldn't get here */
        if (xscaleInt > 7) {
            return;
        }

        /* shouldn't get here */
        if (xscaleIntUV > 7) {
            return;
        }

        if (pPriv->is_mfld)
            newval = (xscaleInt << 15) |
                     ((xscaleFract & 0xFFF) << 3) | ((yscaleFract & 0xFFF) << 20);
        else
            newval = (xscaleInt << 16) |
                     ((xscaleFract & 0xFFF) << 3) | ((yscaleFract & 0xFFF) << 20);

        if (newval != overlay->YRGBSCALE) {
            scaleChanged = TRUE;
            overlay->YRGBSCALE = newval;
        }

        if (pPriv->is_mfld)
            newval = (xscaleIntUV << 15) | ((xscaleFractUV & 0xFFF) << 3) |
                     ((yscaleFractUV & 0xFFF) << 20);
        else
            newval = (xscaleIntUV << 16) | ((xscaleFractUV & 0xFFF) << 3) |
                     ((yscaleFractUV & 0xFFF) << 20);

        if (newval != overlay->UVSCALE) {
            scaleChanged = TRUE;
            overlay->UVSCALE = newval;
        }

        newval = yscaleInt << 16 | yscaleIntUV;
        if (newval != overlay->UVSCALEV) {
            scaleChanged = TRUE;
            overlay->UVSCALEV = newval;
        }

        /* Recalculate coefficients if the scaling changed. */

        /*
         * Only Horizontal coefficients so far.
         */
        if (scaleChanged) {
            double fCutoffY;
            double fCutoffUV;

            fCutoffY = xscaleFract / 4096.0;
            fCutoffUV = xscaleFractUV / 4096.0;

            /* Limit to between 1.0 and 3.0. */
            if (fCutoffY < MIN_CUTOFF_FREQ)
                fCutoffY = MIN_CUTOFF_FREQ;
            if (fCutoffY > MAX_CUTOFF_FREQ)
                fCutoffY = MAX_CUTOFF_FREQ;
            if (fCutoffUV < MIN_CUTOFF_FREQ)
                fCutoffUV = MIN_CUTOFF_FREQ;
            if (fCutoffUV > MAX_CUTOFF_FREQ)
                fCutoffUV = MAX_CUTOFF_FREQ;

            UpdateCoeff(N_HORIZ_Y_TAPS, fCutoffY, TRUE, TRUE, xcoeffY);
            UpdateCoeff(N_HORIZ_UV_TAPS, fCutoffUV, TRUE, FALSE, xcoeffUV);

            for (i = 0; i < N_PHASES; i++) {
                for (j = 0; j < N_HORIZ_Y_TAPS; j++) {
                    pos = i * N_HORIZ_Y_TAPS + j;
                    overlay->Y_HCOEFS[pos] = (xcoeffY[pos].sign << 15 |
                                              xcoeffY[pos].exponent << 12 |
                                              xcoeffY[pos].mantissa);
                }
            }
            for (i = 0; i < N_PHASES; i++) {
                for (j = 0; j < N_HORIZ_UV_TAPS; j++) {
                    pos = i * N_HORIZ_UV_TAPS + j;
                    overlay->UV_HCOEFS[pos] = (xcoeffUV[pos].sign << 15 |
                                               xcoeffUV[pos].exponent << 12 |
                                               xcoeffUV[pos].mantissa);
                }
            }
        }
    }

    OCMD = OVERLAY_ENABLE;

    switch (id) {
    case VA_FOURCC_NV12:
        overlay->OSTRIDE = dstPitch | (dstPitch << 16);
        OCMD &= ~SOURCE_FORMAT;
        OCMD &= ~OV_BYTE_ORDER;
        OCMD |= NV12;//in the spec, there are two NV12, which to use?
        break;
    case VA_FOURCC_YV12:
    case VA_FOURCC_I420:
        /* set UV vertical phase to -0.25 */
        /* overlay->UV_VPH = 0x30003000; */
        overlay->OSTRIDE = (dstPitch * 2) | (dstPitch << 16);
        OCMD &= ~SOURCE_FORMAT;
        OCMD &= ~OV_BYTE_ORDER;
        OCMD |= YUV_420;
        break;
    case VA_FOURCC_UYVY:
    case VA_FOURCC_YUY2:
        overlay->OSTRIDE = dstPitch;
        OCMD &= ~SOURCE_FORMAT;
        OCMD |= YUV_422;
        OCMD &= ~OV_BYTE_ORDER;
        if (id == VA_FOURCC_UYVY)
            OCMD |= Y_SWAP;
        break;
    }

    if (flags & (VA_TOP_FIELD | VA_BOTTOM_FIELD)) {
        OCMD |= BUF_TYPE_FIELD;
        OCMD &= ~FIELD_SELECT;

        if (flags & VA_BOTTOM_FIELD) {
            OCMD |= FIELD1;
            overlay->OBUF_0Y = pPriv->YBuf0offset - srcPitch;
            overlay->OBUF_0U = pPriv->UBuf0offset - srcPitch;
            overlay->OBUF_0V = pPriv->VBuf0offset - srcPitch;
            overlay->OBUF_1Y = pPriv->YBuf1offset - srcPitch;
            overlay->OBUF_1U = pPriv->UBuf1offset - srcPitch;
            overlay->OBUF_1V = pPriv->VBuf1offset - srcPitch;
        } else
            OCMD |= FIELD0;
    } else {
        OCMD &= ~(FIELD_SELECT);
        OCMD &= ~BUF_TYPE_FIELD;
    }

    OCMD &= ~(BUFFER_SELECT);

    if (pPriv->curBuf == 0)
        OCMD |= BUFFER0;
    else
        OCMD |= BUFFER1;

    overlay->OCMD = OCMD;

    memset(&regs, 0, sizeof(regs));
    switch (overlayId) {
    case OVERLAY_A:
        pPriv->overlayA_enabled = 1;
        regs.overlay_write_mask = OV_REGRWBITS_OVADD;
        break;
    case OVERLAY_C:
        pPriv->overlayC_enabled = 1;
        regs.overlay_write_mask = OVC_REGRWBITS_OVADD;
        break;
    }

    if (pPriv->is_mfld) {
        i32EnableIEP = 0;

        i32EnableIEPBLE = 0;

        if (i32EnableIEP == 0) {
            overlay->OCONFIG = CC_OUT_8BIT;
            overlay->OCONFIG &= OVERLAY_C_PIPE_A | (~OVERLAY_C_PIPE_MASK);
            overlay->OCONFIG |= IEP_LITE_BYPASS;
            regs.overlay.OVADD = offset | 1;
            regs.overlay.IEP_ENABLED = 0;
            regs.overlay.buffer_handle = wsbmKBufHandle(wsbmKBuf(pPriv->wsbo[overlayId]));
        }
    } else {
        overlay->OCONFIG = CC_OUT_8BIT;
        overlay->OCONFIG |= IEP_LITE_BYPASS;
        regs.overlay.OVADD = offset | 1;
    }

    if (pPriv->is_mfld) {
        switch (pipeId) {
        case PIPEA:
            overlay->OCONFIG |= OVERLAY_C_PIPE_A;
            overlay->OCONFIG |= ZORDER_TOP;
            break;
        case PIPEB:
            overlay->OCONFIG |= OVERLAY_C_PIPE_B;
            overlay->OCONFIG |= ZORDER_TOP;
            regs.overlay.OVADD |= 0x80;
            break;
        case PIPEC:
            overlay->OCONFIG |= OVERLAY_C_PIPE_C;
            overlay->OCONFIG |= ZORDER_TOP;
            regs.overlay.OVADD |= 0x40;
            break;
        }
        overlay->OCONFIG |= ZORDER_TOP;
    } else
        overlay->OCONFIG |= pipeId << 18; /* mrst */

    if (IS_CTP(driver_data) || IS_MRFL(driver_data) )
        regs.overlay.b_wms = 1;

    drmCommandWriteRead(driver_data->drm_fd, DRM_PSB_REGISTER_RW, &regs, sizeof(regs));

    if (pPriv->is_mfld) {
        if (regs.overlay.IEP_ENABLED) {
#if 0
            printf("regs.overlay BLE minmax 0x%x, BSSCC control 0x%x\n",
                   regs.overlay.IEP_BLE_MINMAX, regs.overlay.IEP_BSSCC_CONTROL);
#endif
            *(unsigned int *)((unsigned int)&(overlay->IEP_SPACE[0]) + 0x804)  = regs.overlay.IEP_BLE_MINMAX;
        }
    }
#endif
}


static void I830PutImageFlipRotateSurface(
    VADriverContextP ctx,
    object_surface_p obj_surface,
    int *src_w_new, int *src_h_new,
    int *width_new, int *height_new,
    psb_surface_p *psb_surface_new,
    int pipeId)
{
    int src_w = *src_w_new, src_h =  *src_h_new;
    int width = *width_new, height = *height_new;
    int  tmp = 0;

    psb_surface_p psb_surface = NULL;
    INIT_DRIVER_DATA;
    PsbPortPrivPtr pPriv;

    /* local/extend display doesn't have render rotation */
    if (((pipeId == PIPEA) && (driver_data->local_rotation == VA_ROTATION_NONE)) ||
        ((pipeId == PIPEB) && (driver_data->extend_rotation == VA_ROTATION_NONE)))
        return;

    pPriv = (PsbPortPrivPtr)(&driver_data->coverlay_priv);

    if (pipeId == PIPEA) {
        if (driver_data->local_rotation != VA_ROTATION_NONE) {
            psb_surface = obj_surface->out_loop_surface;
            width = obj_surface->width_r;
            height = obj_surface->height_r;
            if (driver_data->local_rotation != VA_ROTATION_180) {
                tmp = src_w;
                src_w = src_h;
                src_h = tmp;
            }
        }
        if ((driver_data->local_rotation == VA_ROTATION_NONE) ||
            (driver_data->local_rotation == VA_ROTATION_180)) {
            pPriv->width_save = pPriv->display_width;
            pPriv->height_save = pPriv->display_height;
        } else {
            pPriv->width_save = pPriv->display_height;
            pPriv->height_save = pPriv->display_width;
        }
        if (driver_data->is_android == 0)
            pPriv->rotation = driver_data->local_rotation;
        else
            pPriv->rotation = 0;
    } else if (pipeId == PIPEB) {
        if (driver_data->extend_rotation != VA_ROTATION_NONE) {
            psb_surface = obj_surface->out_loop_surface;
            width = obj_surface->width_r;
            height = obj_surface->height_r;
            if (driver_data->extend_rotation != VA_ROTATION_180) {
                tmp = src_w;
                src_w = src_h;
                src_h = tmp;
            }
        }
        if ((driver_data->extend_rotation == VA_ROTATION_NONE) ||
            (driver_data->extend_rotation == VA_ROTATION_180)) {
            pPriv->width_save = pPriv->extend_display_width;
            pPriv->height_save = pPriv->extend_display_height;
        } else {
            pPriv->width_save = pPriv->extend_display_height;
            pPriv->height_save = pPriv->extend_display_width;
        }
        if (driver_data->is_android == 0)
            pPriv->rotation = driver_data->extend_rotation;
        else
            pPriv->rotation = 0;
    }

    *src_w_new = src_w;
    *src_h_new = src_h;
    *width_new = width;
    *height_new = height;
    *psb_surface_new = psb_surface;
}


static int I830PutImageFlipRotateDebug(
    VADriverContextP ctx,
    VASurfaceID surface,
    short __maybe_unused src_x, short __maybe_unused src_y,
    short __maybe_unused src_w, short __maybe_unused src_h,
    short __maybe_unused drw_x, short __maybe_unused drw_y,
    short __maybe_unused drw_w, short __maybe_unused drw_h,
    int __maybe_unused fourcc, int __maybe_unused flags,
    int __maybe_unused overlayId,
    int pipeId)
{
    INIT_DRIVER_DATA;
    object_surface_p obj_surface;
    psb_surface_p psb_surface = NULL;
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    obj_surface = SURFACE(surface);
    CHECK_SURFACE(obj_surface);

    if (pipeId != 0)
        return -1;

    psb_surface = obj_surface->out_loop_surface;
    psb_buffer_p buf = &psb_surface->buf;
    unsigned char *data, *chroma, *buffer, *header;
    static FILE *pf = NULL;
    int ret, i;
    if (!psb_surface)
        goto dump_out;
    if (pf == NULL)
        if ((pf = fopen("/home/dump.yuv", "w+")) == NULL) {
            printf("Open yuv file fails\n");
            return -1;
        }
    ret = psb_buffer_map(buf, &data);

    if (ret) {
        printf("Map buffer fail\n");
        return -1;
    }
    for (i = 0; i < obj_surface->height_r; i++) {
        fwrite(data, 1, obj_surface->width_r, pf);
        data += psb_surface->stride;
    }

    buffer = malloc(obj_surface->height_r * obj_surface->width_r);
    if (!buffer) {
        printf("Alloc chroma buffer fail\n");
        return -1;
    }
    header = buffer;
    chroma = data;
    for (i = 0; i < obj_surface->height_r / 2; i++) {
        int j;
        for (j = 0; j < obj_surface->width_r / 2; j++) {
            *buffer++ = data[j*2];
        }
        data += psb_surface->stride;
    }

    data = chroma;
    for (i = 0; i < obj_surface->height_r / 2; i++) {
        int j;
        for (j = 0; j < obj_surface->width_r / 2; j++) {
            *buffer++ = data[j*2 + 1];
        }
        data += psb_surface->stride;
    }

    fwrite(header, obj_surface->height_r / 2, obj_surface->width_r, pf);
    free(header);
    psb_buffer_unmap(buf);
    return 0;

dump_out:
    return -1;
}


/*
 * The source rectangle of the video is defined by (src_x, src_y, src_w, src_h).
 * The dest rectangle of the video is defined by (drw_x, drw_y, drw_w, drw_h).
 * id is a fourcc code for the format of the video.
 * buf is the pointer to the source data in system memory.
 * width and height are the w/h of the source data.
 * If "sync" is TRUE, then we must be finished with *buf at the point of return
 * (which we always are).
 * clipBoxes is the clipping region in screen space.
 * data is a pointer to our port private.
 * pDraw is a Drawable, which might not be the screen in the case of
 * compositing.  It's a new argument to the function in the 1.1 server.
 */
static int I830PutImage(
    VADriverContextP ctx,
    VASurfaceID surface,
    int src_x, int src_y,
    int src_w, int src_h,
    int drw_x, int drw_y,
    int drw_w, int drw_h,
    int fourcc, int flags,
    int overlayId,
    int pipeId)
{
    INIT_DRIVER_DATA;
    int x1, x2, y1, y2;
    int width, height;
    int top, left, npixels;
    int pitch = 0, pitch2 = 0;
    unsigned int pre_add;
    unsigned int gtt_ofs;
    struct _WsbmBufferObject *drm_buf;
    BoxRec dstBox;
    PsbPortPrivPtr pPriv;
    object_surface_p obj_surface = SURFACE(surface);
    psb_surface_p psb_surface = NULL;

    /* silent kw */
    if (NULL == obj_surface)
        return 1;

    pPriv = (PsbPortPrivPtr)(&driver_data->coverlay_priv);

    switch (fourcc) {
    case VA_FOURCC_NV12:
        width = obj_surface->width;
        height = obj_surface->height;
        break;
    default:
        width = obj_surface->width;
        height = obj_surface->height;
        break;
    }

    /* rotate support here: more check?
     * and for oold also?
     */
    psb_surface = obj_surface->psb_surface;
    I830PutImageFlipRotateSurface(ctx, obj_surface,
                                  &src_w, &src_h, &width, &height,
                                  &psb_surface, pipeId);

    if (NULL == psb_surface) {
        /*Rotate surface may not be ready, so we have to discard this frame.*/
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Discard this frame if rotate surface hasn't be ready.\n");

        return 1;
    }
    width = (width <= 1920) ? width : 1920;

    /* If dst width and height are less than 1/8th the src size, the
     * src/dst scale factor becomes larger than 8 and doesn't fit in
     * the scale register.
     */
    if (src_w >= (drw_w * 8))
        drw_w = src_w / 7;

    if (src_h >= (drw_h * 8))
        drw_h = src_h / 7;

    /* Clip */
    x1 = src_x;
    x2 = src_x + src_w;
    y1 = src_y;
    y2 = src_y + src_h;

    dstBox.x1 = drw_x;
    dstBox.x2 = drw_x + drw_w;
    dstBox.y1 = drw_y;
    dstBox.y2 = drw_y + drw_h;

#if USE_CLIP_FUNC
    if (!i830_get_crtc(pScrn, &crtc, &dstBox))
        return Success;

    /*
     *Update drw_* and 'clipBoxes' according to current downscale/upscale state
     * Make sure the area determined by drw_* is in 'clipBoxes'
     */
    if (crtc->rotation & (RR_Rotate_90 | RR_Rotate_270)) {
        h_ratio = (float)pScrn->pScreen->height / pPriv->width_save;
        v_ratio = (float)pScrn->pScreen->width / pPriv->height_save;
    } else {
        h_ratio = (float)pScrn->pScreen->width / pPriv->width_save;
        v_ratio = (float)pScrn->pScreen->height / pPriv->height_save;
    }

    /* Horizontal downscale/upscale */
    if ((int)h_ratio)
        clipBoxes->extents.x1 /= h_ratio;
    else if (!(int)h_ratio)
        clipBoxes->extents.x2 /= h_ratio;

    /* Vertical downscale/upscale */
    if ((int)v_ratio)
        clipBoxes->extents.y1 /= v_ratio;
    else if (!(int)v_ratio)
        clipBoxes->extents.y2 /= v_ratio;

    drw_x /= h_ratio;
    drw_y /= v_ratio;
    drw_w /= h_ratio;
    drw_h /= v_ratio;

    dstBox.x1 = drw_x;
    dstBox.x2 = drw_x + drw_w;
    dstBox.y1 = drw_y;
    dstBox.y2 = drw_y + drw_h;

    /* Count in client supplied clipboxes */
    clipRegion = clipBoxes;
    psb_perform_clip(pScrn, vaPtr->clipbox, vaPtr->num_clipbox, clipBoxes, clipRegion, pDraw);

    if (!i830_clip_video_helper(pScrn,
                                &crtc,
                                &dstBox, &x1, &x2, &y1, &y2, clipRegion,
                                width, height)) {
        xf86DrvMsg(pScrn->scrnIndex, X_WARNING, "%s: Fail to clip video to any crtc!\n", __FUNCTION__);
        return 0;
    }
#endif

    switch (fourcc) {
    case VA_FOURCC_NV12:
        pitch = (width + 0x3) & ~0x3;
        pitch2 = psb_surface->stride;
        break;
    case VA_FOURCC_YV12:
    case VA_FOURCC_I420:
        pitch = (width + 0x3) & ~0x3;
        break;
#if USE_DISPLAY_C_SPRITE
    case FOURCC_RGBA:
        pitch = width << 2;
        break;
#endif
    case VA_FOURCC_UYVY:
    case VA_FOURCC_YUY2:
    default:
        pitch = width << 1;
        break;
    }

    top = (y1) & ~1;
    left = (x1) & ~1;
    npixels = ((((x2 + 0xffff) >> 16) + 1) & ~1) - left;

    if (fourcc == VA_FOURCC_NV12) {
        pre_add = psb_surface->buf.buffer_ofs;
        drm_buf = psb_surface->buf.drm_buf;
        gtt_ofs = wsbmBOOffsetHint(drm_buf) & 0x0FFFFFFF;

        /*skip pad bytes.*/
        if (driver_data->local_rotation == VA_ROTATION_90) {
            left += ((src_w + 0xf) & ~0xf) - src_w;
        } else if (driver_data->local_rotation == VA_ROTATION_270) {
            top += ((src_h + 0xf) & ~0xf) - src_h;
        } else if (driver_data->local_rotation == VA_ROTATION_180) {
            left += ((src_w + 0xf) & ~0xf) - src_w;
            top += ((src_h + 0xf) & ~0xf) - src_h;
        }
        pPriv->YBuf0offset = pre_add + gtt_ofs  + top * pitch2 + left;
        pPriv->YBuf1offset = pPriv->YBuf0offset;
        pPriv->UBuf0offset = pre_add + gtt_ofs + (pitch2  * height) + top * (pitch2 / 2) + left;
        pPriv->VBuf0offset = pPriv->UBuf0offset;
        pPriv->UBuf1offset = pPriv->UBuf0offset;
        pPriv->VBuf1offset = pPriv->UBuf0offset;
    } else {
        //TBD
        //pPriv->YBuf0offset = pPriv->videoBuf0_gtt_offset << PAGE_SHIFT;
        //pPriv->YBuf1offset = pPriv->videoBuf1_gtt_offset << PAGE_SHIFT;
        if (pPriv->rotation & (RR_Rotate_90 | RR_Rotate_270)) {
            pPriv->UBuf0offset = pPriv->YBuf0offset + (pitch2  * width);
            pPriv->VBuf0offset = pPriv->UBuf0offset + (pitch2 * width / 2);
            pPriv->UBuf1offset = pPriv->YBuf1offset + (pitch2 * width);
            pPriv->VBuf1offset = pPriv->UBuf1offset + (pitch2 * width / 2);
        } else {
            pPriv->UBuf0offset = pPriv->YBuf0offset + (pitch2 * height);
            pPriv->VBuf0offset = pPriv->UBuf0offset + (pitch2 * height / 2);
            pPriv->UBuf1offset = pPriv->YBuf1offset + (pitch2 * height);
            pPriv->VBuf1offset = pPriv->UBuf1offset + (pitch2 * height / 2);
        }
    }

#if USE_DISPLAY_C_SPRITE
    if (fourcc == FOURCC_RGBA   \
        || (fourcc == FOURCC_XVVA   \
            && (pPriv->rotation != RR_Rotate_0) \
            && (vaPtr->dst_srf.fourcc == VA_FOURCC_RGBA)))
        i830_display_video_sprite(pScrn, crtc, width, height, dstPitch,
                                  &dstBox, sprite_offset);
    else
#endif
        i830_display_video(ctx, pPriv, surface, fourcc, src_w, src_h, pitch2, pitch,
                           x1, y1, x2, y2, &dstBox, src_w, src_h,
                           drw_w, drw_h, flags, overlayId, pipeId);

    // FIXME : do I use two buffers here really?
    //    pPriv->curBuf = (pPriv->curBuf + 1) & 1;

    return Success;
}



static void psbPortPrivCreate(PsbPortPrivPtr pPriv)
{
#if 0
    REGION_NULL(pScreen, &pPriv->clip);
#endif

    /* coeffs defaut value */
    pPriv->brightness.Value = OV_BRIGHTNESS_DEFAULT_VALUE;
    pPriv->brightness.Fraction = 0;

    pPriv->contrast.Value = OV_CONTRAST_DEFAULT_VALUE;
    pPriv->contrast.Fraction = 0;

    pPriv->hue.Value = OV_HUE_DEFAULT_VALUE;
    pPriv->hue.Fraction = 0;

    pPriv->saturation.Value = OV_SATURATION_DEFAULT_VALUE;
    pPriv->saturation.Fraction = 0;
    pPriv->subpicture_enabled = 0;
    pPriv->subpicture_enable_mask = 0;
    pPriv->overlayA_enabled = 0;
    pPriv->overlayC_enabled = 0;
    pPriv->overlayA_pipeId = PIPEA;
    pPriv->overlayC_pipeId = PIPEB;

    /* FIXME: is this right? set up to current screen size */
#if 1
    pPriv->width_save = 1024;
    pPriv->height_save = 600;
#endif
}

static void
psbPortPrivDestroy(VADriverContextP ctx, PsbPortPrivPtr pPriv)
{
    I830StopVideo(ctx);

    wsbmBOUnmap(pPriv->wsbo[0]);
    wsbmBOUnreference(&pPriv->wsbo[0]);
    wsbmBOUnmap(pPriv->wsbo[1]);
    wsbmBOUnreference(&pPriv->wsbo[1]);
    if (pPriv->is_mfld) {
        if (pPriv->p_iep_lite_context)
            free(pPriv->p_iep_lite_context);
    }
    pPriv->p_iep_lite_context = NULL;
}

static int
psbSetupImageVideoOverlay(VADriverContextP ctx, PsbPortPrivPtr pPriv)
{
    INIT_DRIVER_DATA;
    I830OverlayRegPtr overlayA = NULL;
    I830OverlayRegPtr overlayC = NULL;
    int ret;
    psbPortPrivCreate(pPriv);


    /* use green as color key by default for android media player */
    pPriv->colorKey = driver_data->color_key/*0x0440*/;

    /*Bypass color correction. Because these color
    correction can be done in pipe color correction in future.*/
    pPriv->brightness.Value = 0;   /*-19*/
    pPriv->contrast.Value = 0x40;  /*75*/
    pPriv->saturation.Value = 0x80;  /*146*/

    pPriv->gamma5 = 0xc0c0c0;
    pPriv->gamma4 = 0x808080;
    pPriv->gamma3 = 0x404040;
    pPriv->gamma2 = 0x202020;
    pPriv->gamma1 = 0x101010;
    pPriv->gamma0 = 0x080808;

    pPriv->rotation = VA_ROTATION_NONE;
    pPriv->subpic_clear_flag = 1;
#if 0
    /* gotta uninit this someplace */
    REGION_NULL(pScreen, &pPriv->clip);
#endif

    /* With LFP's we need to detect whether we're in One Line Mode, which
     * essentially means a resolution greater than 1024x768, and fix up
     * the scaler accordingly.
     */
    pPriv->scaleRatio = 0x10000;
    pPriv->oneLineMode = FALSE;

    ret = wsbmGenBuffers(driver_data->main_pool, 2,
                         &pPriv->wsbo[0], 64 * 1024, /* 64k alignment */
                         WSBM_PL_FLAG_TT);
    if (ret)
        goto out_err;

    ret = wsbmBOData(pPriv->wsbo[0],
                     5 * 4096,
                     NULL, NULL,
                     WSBM_PL_FLAG_TT);
    if (ret)
        goto out_err_bo0;

    pPriv->regmap[0] = wsbmBOMap(pPriv->wsbo[0], WSBM_ACCESS_READ | WSBM_ACCESS_WRITE);
    if (!pPriv->regmap[0]) {
        goto out_err_bo0;
    }

    ret = wsbmBOData(pPriv->wsbo[1],
                     5 * 4096,
                     NULL, NULL,
                     WSBM_PL_FLAG_TT);
    if (ret)
        goto out_err_bo1;

    pPriv->regmap[1] = wsbmBOMap(pPriv->wsbo[1], WSBM_ACCESS_READ | WSBM_ACCESS_WRITE);
    if (!pPriv->regmap[1]) {
        goto out_err_bo1;
    }

    overlayA = (I830OverlayRegPtr)(pPriv->regmap[0]);
    overlayC = (I830OverlayRegPtr)(pPriv->regmap[1]);

    if (pPriv->is_mfld) {
        driver_data->ble_black_mode.value = 1;
        driver_data->ble_white_mode.value = 3;
        driver_data->blueStretch_gain.value = 200;
        driver_data->skinColorCorrection_gain.value = 100;
        driver_data->hue.value = (5.25f * (1 << 25));
        driver_data->saturation.value = (1.07f * (1 << 25));
        driver_data->brightness.value = (-10.1f * (1 << 10));
        driver_data->contrast.value = (0.99f * (1 << 25));
    }

    return 0;

out_err_bo1:
    wsbmBOUnreference(&pPriv->wsbo[1]);
out_err_bo0:
    wsbmBOUnreference(&pPriv->wsbo[0]);

out_err:
    return -1;
}

int psb_coverlay_init(VADriverContextP ctx)
{
#ifndef BAYTRAIL
    INIT_DRIVER_DATA;
    PsbPortPrivPtr pPriv = &driver_data->coverlay_priv;
    struct drm_psb_register_rw_arg regs;
    int ret;

    memset(pPriv, 0, sizeof(PsbPortPrivRec));
    pPriv->is_mfld = (IS_MFLD(driver_data) || IS_MRFL(driver_data));

    ret = psbSetupImageVideoOverlay(ctx, pPriv);
    if (ret != 0) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "psb_coverlay_init : Create overlay cmd buffer failed.\n");
        return -1;
    }

    if (pPriv->is_mfld && driver_data->is_android) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Android ExtVideo: set PIPEB(HDMI)display plane on the bottom.\n");

        memset(&regs, 0, sizeof(regs));
        regs.display_read_mask = REGRWBITS_DSPBCNTR;
        drmCommandWriteRead(driver_data->drm_fd, DRM_PSB_REGISTER_RW, &regs, sizeof(regs));
        regs.display.dspcntr_b |= DISPPLANE_BOTTOM;
        regs.display_write_mask = REGRWBITS_DSPBCNTR;
        drmCommandWriteRead(driver_data->drm_fd, DRM_PSB_REGISTER_RW, &regs, sizeof(regs));
    }

    I830ResetVideo(ctx, pPriv);
    I830UpdateGamma(ctx, pPriv);
#endif
    return 0;
}

int psb_coverlay_stop(VADriverContextP ctx)
{
    I830StopVideo(ctx);
    return 0;
}

int psb_coverlay_deinit(VADriverContextP ctx)
{
#ifndef BAYTRAIL
    INIT_DRIVER_DATA;
    PsbPortPrivPtr pPriv = &driver_data->coverlay_priv;
    struct drm_psb_register_rw_arg regs;

    if (pPriv->is_mfld && driver_data->is_android) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Android ExtVideo: set PIPEB(HDMI)display plane normal.\n");

        memset(&regs, 0, sizeof(regs));
        regs.display_read_mask = REGRWBITS_DSPBCNTR;
        drmCommandWriteRead(driver_data->drm_fd, DRM_PSB_REGISTER_RW, &regs, sizeof(regs));
        regs.display.dspcntr_b &= ~DISPPLANE_BOTTOM;
        regs.display_write_mask = REGRWBITS_DSPBCNTR;
        drmCommandWriteRead(driver_data->drm_fd, DRM_PSB_REGISTER_RW, &regs, sizeof(regs));
    }

    psbPortPrivDestroy(ctx, pPriv);
#endif
    return 0;
}

VAStatus psb_putsurface_overlay(
    VADriverContextP ctx,
    VASurfaceID surface,
    short srcx,
    short srcy,
    unsigned short srcw,
    unsigned short srch,
    short destx,
    short desty,
    unsigned short destw,
    unsigned short desth,
    unsigned int flags, /* de-interlacing flags */
    int overlayId,
    int pipeId
)
{
    INIT_DRIVER_DATA;
    object_surface_p obj_surface = SURFACE(surface);
    PsbPortPrivPtr pPriv = (PsbPortPrivPtr)(&driver_data->coverlay_priv);
#if 0
    if ((overlayId == OVERLAY_A) && (pPriv->overlayA_pipeId != pipeId)) {
        pPriv->overlayA_pipeId = pipeId;
        I830SwitchPipe(ctx, OVERLAY_A, pipeId);
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "OverlayA switch pipe to %d, stop overlayA first.\n", pipeId);
    } else if ((overlayId == OVERLAY_C) && (pPriv->overlayC_pipeId != pipeId)) {
        pPriv->overlayC_pipeId = pipeId;
        I830SwitchPipe(ctx, OVERLAY_C, pipeId);
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "OverlayC switch pipe to %d, stop overlayC first.\n", pipeId);
    }
#endif
    I830PutImage(ctx, surface, srcx, srcy, srcw, srch,
                 destx, desty, destw, desth,
                 VA_FOURCC_NV12, flags, overlayId, pipeId);

    /* current surface is being displayed */
    if (driver_data->cur_displaying_surface != VA_INVALID_SURFACE)
        driver_data->last_displaying_surface = driver_data->cur_displaying_surface;

    if (obj_surface == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Invalid surface ID: 0x%08x\n", surface);
        return VA_STATUS_ERROR_INVALID_SURFACE;
    }

    obj_surface->display_timestamp = GetTickCount();
    driver_data->cur_displaying_surface = surface;

    return VA_STATUS_SUCCESS;
}
