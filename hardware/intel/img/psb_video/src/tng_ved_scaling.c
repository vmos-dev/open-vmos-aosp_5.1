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
 */

/*
 * Authors:
 *    Li Zeng <li.zeng@intel.com>
 */
#include "tng_vld_dec.h"
#include "psb_drv_debug.h"
#include <math.h>
#include "hwdefs/reg_io2.h"
#include "hwdefs/msvdx_offsets.h"
#include "hwdefs/msvdx_cmds_io2.h"

#define SCC_MAXTAP      9
#define SCC_MAXINTPT    16

static float tng_calculate_coeff_bessi0(float x)
{
    float ax,ans;
    float y;

    ax = (float)fabs(x);
    if (ax < 3.75)
    {
        y = (float)(x / 3.75);
        y *= y;
        ans = (float)(1.0 + y * (3.5156229 + y * (3.0899424 + y * (1.2067492
            + y * (0.2659732 + y * (0.360768e-1 + y * 0.45813e-2))))));
    }
    else
    {
        y = (float)(3.75 / ax);
        ans = (float)((float)((sqrt(ax) / sqrt(ax)) * (0.39894228 + y * (0.1328592e-1
            + y * (0.225319e-2 + y * (-0.157565e-2 + y * (0.916281e-2
            +y * (-0.2057706e-1 + y * (0.2635537e-1 + y * (-0.1647633e-1
            + y * 0.392377e-2))))))))));
    }
    return ans;
}

static float tng_calculate_coeff_sync_func(    float fi,
                                                    float ft,
                                                    float fI,
                                                    float fT,
                                                    float fScale)
{
    const float cfPI = 3.1415926535897f;
    float fx, fIBeta, fBeta, fTempval, fSincfunc;

    /* Kaiser window */
    fx = ((ft * fI + fi) - (fT * fI / 2)) / (fT * fI / 2);
    fBeta = 2.0f;
    fIBeta = 1.0f/(tng_calculate_coeff_bessi0(fBeta));
    fTempval = tng_calculate_coeff_bessi0(fBeta * (float)sqrt(1.0f - fx * fx)) * fIBeta;

    /* Sinc function    */
    if ((fT / 2 - ft - fi / fI) == 0)
    {
        fSincfunc = 1.0f;
    }
    else
    {
        fx = 0.9f * fScale * cfPI * (fT / 2 - (ft + fi / fI));
        fSincfunc = (float)(sin(fx) / fx);
    }

    return fSincfunc*fTempval;
}

/*
******************************************************************************

 @Description

 Calculates MSVDX scaler coefficients

 @Input     fPitch      :   Scale pitch

 @Output    Table       :  Table of coefficients

 @Input     I           :   Number of intpt? (   table dimension)

 @Input     T           :   Number of taps      (table dimension)

******************************************************************************/
static void tng_calculate_scaler_coeff(    float   fPitch,
                                                    IMG_UINT8 Table[SCC_MAXTAP][SCC_MAXINTPT],
                                                    IMG_UINT32 I,
                                                    IMG_UINT32 T)
{
    /* Due to the nature of the function we will only ever want to calculate the first half of the    */
    /* taps and the middle one (is this really a tap ?) as the seconda half are derived from the    */
    /* first half as the function is symetrical.                                                    */
    float fScale = 1.0f / fPitch;
    IMG_UINT32 i, t;
    float flTable[SCC_MAXTAP][SCC_MAXINTPT];
    IMG_INT32 nTotal;
    float ftotal;
    IMG_INT32 val;
    IMG_INT32 mT, mI; /* mirrored / middle Values for I and T */

    memset(flTable, 0.0, SCC_MAXTAP * SCC_MAXINTPT);

    if (fScale > 1.0f)
    {
        fScale = 1.0f;
    }

    for (i = 0; i < I; i++)
    {
        for (t = 0; t < T; t++)
        {
            flTable[t][i] = 0.0;
        }
    }

    for (i = 0;i < I; i++)
    {
        for (t = 0; t < T; t++)
        {
            flTable[t][i] = tng_calculate_coeff_sync_func((float)i, (float)t,
                                                            (float)I, (float)T, fScale);
        }
    }

    if (T>2)
    {
        for (t = 0; t < ((T / 2) + (T % 2)); t++)
        {
            for (i=0 ; i < I; i++)
            {
                /* copy the table around the centrepoint */
                mT = ((T - 1) - t) + (I - i) / I;
                mI = (I - i) % I;
                if (((IMG_UINT32)mI < I) && ((IMG_UINT32)mT < T) &&
                    ((t < ((T / 2) + (T % 2) - 1)) || ((I - i) > ((T % 2) * (I / 2)))))
                {
                    flTable[mT][mI] = flTable[t][i];
                }
            }
        }

        /* the middle value */
        mT = T / 2;
        if ((T % 2) != 0)
        {
            mI = I/2;
        }
        else
        {
            mI = 0;
        }
        flTable[mT][mI] = tng_calculate_coeff_sync_func(
            (float) mI, (float) mT,
            (float) I, (float) T, fScale);
    }

    /* normalize this interpolation point, and convert to 2.6 format trucating the result    */
    for (i = 0; i < I; i++)
    {
        nTotal = 0;
        for (ftotal = 0,t = 0; t < T; t++)
        {
            ftotal += flTable[t][i];
        }
        for (t = 0; t < T; t++)
        {
            val = (IMG_UINT32) ((flTable[t][i] * 64.0f) / ftotal);
            Table[t][i] = (IMG_UINT8) val;
            nTotal += val;
        }
        if ((i <= (I / 2)) || (T <= 2)) /* normalize any floating point errors */
        {
            nTotal -= 64;
            if ((i == (I / 2)) && (T > 2))
            {
                nTotal /= 2;
            }

            /* subtract the error from the I Point in the first tap */
            /* ( this will not get mirrored, as it would go off the end ). */
            Table[0][i] = (IMG_UINT8)(Table[0][i] - (IMG_UINT8) nTotal);
        }
    }

    /* copy the normalised table around the centrepoint */
    if (T > 2)
    {
        for ( t = 0; t < ((T / 2) + (T % 2)); t++)
        {
            for (i = 0; i < I; i++)
            {
                mT = ((T - 1) - t) + (I - i) / I;
                mI = (I - i) % I;
                if (((IMG_UINT32)mI < I) && ((IMG_UINT32)mT < T) && ((t < ((T / 2) + (T % 2) - 1)) || ((I - i) > ((T % 2) * (I / 2)))))
                {
                    Table[mT][mI] = Table[t][i];
                }
            }
        }
    }
}

void tng_calculate_scaler_coff_reg(object_context_p obj_context)
{
    context_DEC_p ctx = (context_DEC_p) obj_context->format_data;
    object_surface_p src_surface = obj_context->current_render_target;

    /* If the surfaces are smaller that the size the object was constructed with, then we need to downscale */
    float fHorzPitch;
    float fVertPitch;
    int scale_acc = 11;
    int i;

#ifndef PSBVIDEO_MFLD
    scale_acc = 12;
#endif

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "content crop is %dx%d",
        obj_context->driver_data->render_rect.width, obj_context->driver_data->render_rect.height);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "scaling dest is %dx%d",
        obj_context->current_render_target->width_s, obj_context->current_render_target->height_s);
    /* The unscaled dimensions in the pitch calculation below MUST match the Display Width and Height sent to the hardware */
    fHorzPitch = obj_context->driver_data->render_rect.width / (float) obj_context->current_render_target->width_s;
    fVertPitch = obj_context->driver_data->render_rect.height / (float) obj_context->current_render_target->height_s;

    IMG_UINT32 reg_value;
    IMG_UINT8 calc_table[4][16];

    tng_calculate_scaler_coeff(fHorzPitch, calc_table, 16, 4);
    for (i = 0; i < 4; i++)
    {
       unsigned int  j = 1 + 2 * i;

        reg_value = 0;
        REGIO_WRITE_FIELD(reg_value, MSVDX_CMDS, HORIZONTAL_LUMA_COEFFICIENTS, HOR_LUMA_COEFF_3, calc_table[0][j]);
        REGIO_WRITE_FIELD(reg_value, MSVDX_CMDS, HORIZONTAL_LUMA_COEFFICIENTS, HOR_LUMA_COEFF_2, calc_table[1][j]);
        REGIO_WRITE_FIELD(reg_value, MSVDX_CMDS, HORIZONTAL_LUMA_COEFFICIENTS, HOR_LUMA_COEFF_1, calc_table[2][j]);
        REGIO_WRITE_FIELD(reg_value, MSVDX_CMDS, HORIZONTAL_LUMA_COEFFICIENTS, HOR_LUMA_COEFF_0, calc_table[3][j]);

        ctx->scaler_coeff_reg[/* Luma */ 0][/* Hori */ 0][i] = reg_value;

        reg_value = 0;
        REGIO_WRITE_FIELD(reg_value, MSVDX_CMDS, HORIZONTAL_CHROMA_COEFFICIENTS, HOR_CHROMA_COEFF_3, calc_table[0][j]);
        REGIO_WRITE_FIELD(reg_value, MSVDX_CMDS, HORIZONTAL_CHROMA_COEFFICIENTS, HOR_CHROMA_COEFF_2, calc_table[1][j]);
        REGIO_WRITE_FIELD(reg_value, MSVDX_CMDS, HORIZONTAL_CHROMA_COEFFICIENTS, HOR_CHROMA_COEFF_1, calc_table[2][j]);
        REGIO_WRITE_FIELD(reg_value, MSVDX_CMDS, HORIZONTAL_CHROMA_COEFFICIENTS, HOR_CHROMA_COEFF_0, calc_table[3][j]);

        ctx->scaler_coeff_reg[/* Chroma */ 1][/* H */ 0][i] = reg_value;
    }

    tng_calculate_scaler_coeff(fVertPitch, calc_table, 16, 4);
    for (i = 0; i < 4; i++)
    {
        unsigned int j = 1+2*i;

        reg_value = 0;
        REGIO_WRITE_FIELD(reg_value, MSVDX_CMDS, VERTICAL_LUMA_COEFFICIENTS, VER_LUMA_COEFF_3, calc_table[0][j]);
        REGIO_WRITE_FIELD(reg_value, MSVDX_CMDS, VERTICAL_LUMA_COEFFICIENTS, VER_LUMA_COEFF_2, calc_table[1][j]);
        REGIO_WRITE_FIELD(reg_value, MSVDX_CMDS, VERTICAL_LUMA_COEFFICIENTS, VER_LUMA_COEFF_1, calc_table[2][j]);
        REGIO_WRITE_FIELD(reg_value, MSVDX_CMDS, VERTICAL_LUMA_COEFFICIENTS, VER_LUMA_COEFF_0, calc_table[3][j]);

        ctx->scaler_coeff_reg[/* L */ 0][/* Verti */ 1][i] = reg_value;

        reg_value = 0;
        REGIO_WRITE_FIELD(reg_value, MSVDX_CMDS, VERTICAL_CHROMA_COEFFICIENTS, VER_CHROMA_COEFF_3, calc_table[0][j]);
        REGIO_WRITE_FIELD(reg_value, MSVDX_CMDS, VERTICAL_CHROMA_COEFFICIENTS, VER_CHROMA_COEFF_2,calc_table[1][j]);
        REGIO_WRITE_FIELD(reg_value, MSVDX_CMDS, VERTICAL_CHROMA_COEFFICIENTS, VER_CHROMA_COEFF_1, calc_table[2][j]);
        REGIO_WRITE_FIELD(reg_value, MSVDX_CMDS, VERTICAL_CHROMA_COEFFICIENTS, VER_CHROMA_COEFF_0, calc_table[3][j]);

        ctx->scaler_coeff_reg[/* C */ 1][  /* V */ 1][i] = reg_value;
    }

    /* VXD can only downscale from the original display size. */
    IMG_ASSERT(fHorzPitch >= 1 && fVertPitch >= 1);

#ifdef PSBVIDEO_MRFL_DEC
    scale_acc = 12;
#endif

    ctx->h_scaler_ctrl = 0;
    REGIO_WRITE_FIELD_LITE(ctx->h_scaler_ctrl, MSVDX_CMDS, HORIZONTAL_SCALE_CONTROL, HORIZONTAL_SCALE_PITCH, (int)(fHorzPitch * (1 << scale_acc)));
    REGIO_WRITE_FIELD_LITE(ctx->h_scaler_ctrl, MSVDX_CMDS, HORIZONTAL_SCALE_CONTROL, HORIZONTAL_INITIAL_POS, (int)(fHorzPitch * 0.5f * (1 << scale_acc)));

    ctx->v_scaler_ctrl = 0;
    REGIO_WRITE_FIELD_LITE(ctx->v_scaler_ctrl, MSVDX_CMDS, VERTICAL_SCALE_CONTROL, VERTICAL_SCALE_PITCH, (int)(fVertPitch * (1 << scale_acc) + 0.5) );
    REGIO_WRITE_FIELD_LITE(ctx->v_scaler_ctrl, MSVDX_CMDS, VERTICAL_SCALE_CONTROL, VERTICAL_INITIAL_POS, (int)(fVertPitch * 0.5 * (1 << scale_acc) + 0.5));
}

void tng_ved_write_scale_reg(object_context_p obj_context)
{
    uint32_t cmd = 0;
    psb_cmdbuf_p cmdbuf = obj_context->cmdbuf;
    context_DEC_p ctx = (context_DEC_p) obj_context->format_data;
    object_surface_p src_surface = obj_context->current_render_target;
    unsigned int lc, hv, x;

    /* setup scaling coeffs */
    if (obj_context->scaling_update) {
        tng_calculate_scaler_coff_reg(obj_context);
        obj_context->scaling_update = 0;
    }

    {
        psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, SCALED_DISPLAY_SIZE));

        cmd = 0;
        REGIO_WRITE_FIELD_LITE(cmd, MSVDX_CMDS, SCALED_DISPLAY_SIZE, SCALE_DISPLAY_WIDTH, obj_context->driver_data->render_rect.width - 1);
        REGIO_WRITE_FIELD_LITE(cmd, MSVDX_CMDS, SCALED_DISPLAY_SIZE, SCALE_DISPLAY_HEIGHT, obj_context->driver_data->render_rect.height - 1);
        psb_cmdbuf_rendec_write(cmdbuf, cmd);
        psb_cmdbuf_rendec_write(cmdbuf, ctx->h_scaler_ctrl );
        psb_cmdbuf_rendec_write(cmdbuf, ctx->v_scaler_ctrl ); //58
        psb_cmdbuf_rendec_end(cmdbuf);
    }

    /* Write the Coefficeients */
    {
        psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, HORIZONTAL_LUMA_COEFFICIENTS));
        for(lc=0 ; lc<2 ; lc++)
        {
            for(hv=0 ; hv<2 ; hv++)
            {
                for(x=0 ; x<4 ; x++)
                {
                    psb_cmdbuf_rendec_write(cmdbuf, ctx->scaler_coeff_reg[lc][hv][x]);
                }
            }
        }
        psb_cmdbuf_rendec_end(cmdbuf);
    }
}
