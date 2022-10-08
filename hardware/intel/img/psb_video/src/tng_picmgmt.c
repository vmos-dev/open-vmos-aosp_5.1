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
 *    Edward Lin <edward.lin@intel.com>
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <memory.h>
#include <wsbm/wsbm_manager.h>
#include "tng_picmgmt.h"
#include "psb_drv_debug.h"

#define MASK_TOPAZHP_CR_H264COMP_CUSTOM_QUANT_Q_0 0x0000007F
#define SHIFT_TOPAZHP_CR_H264COMP_CUSTOM_QUANT_Q_0 0
#define MASK_TOPAZHP_CR_H264COMP_CUSTOM_QUANT_Q_1 0x00007F00
#define SHIFT_TOPAZHP_CR_H264COMP_CUSTOM_QUANT_Q_1 8
#define MASK_TOPAZHP_CR_H264COMP_CUSTOM_QUANT_Q_2 0x007F0000
#define SHIFT_TOPAZHP_CR_H264COMP_CUSTOM_QUANT_Q_2 16
#define MASK_TOPAZHP_CR_H264COMP_CUSTOM_QUANT_Q_3 0x7F000000
#define SHIFT_TOPAZHP_CR_H264COMP_CUSTOM_QUANT_Q_3 24

#define MASK_TOPAZHP_CR_H264COMP_CUSTOM_QUANT_SP_0 0x00003FFF
#define SHIFT_TOPAZHP_CR_H264COMP_CUSTOM_QUANT_SP_0 0
#define MASK_TOPAZHP_CR_H264COMP_CUSTOM_QUANT_SP_1 0x3FFF0000
#define SHIFT_TOPAZHP_CR_H264COMP_CUSTOM_QUANT_SP_1 16

/************************* MTX_CMDID_PICMGMT *************************/
VAStatus tng_picmgmt_update(context_ENC_p ctx, IMG_PICMGMT_TYPE eType, unsigned int ref)
{
    //VAStatus vaStatus = VA_STATUS_SUCCESS;
    IMG_UINT32 ui32CmdData = 0;

    //IMG_V_SetNextRefType  eFrameType
    //IMG_V_SkipFrame       bProcess
    //IMG_V_EndOfStream     ui32FrameCount
    //IMG_PICMGMT_FLUSH     ui32FrameCount
    ui32CmdData = F_ENCODE(eType, MTX_MSG_PICMGMT_SUBTYPE) |
        F_ENCODE(ref, MTX_MSG_PICMGMT_DATA);

    /* Send PicMgmt Command */
    tng_cmdbuf_insert_command(ctx->obj_context, ctx->ui32StreamID,
        MTX_CMDID_PICMGMT | MTX_CMDID_PRIORITY,
        ui32CmdData, 0, 0);

    return VA_STATUS_SUCCESS;
}

/*!
******************************************************************************
*
* Picture management functions
*
******************************************************************************/
void tng__picmgmt_long_term_refs(context_ENC_p __maybe_unused ctx, IMG_UINT32 __maybe_unused ui32FrameNum)
{
#ifdef _TNG_ENABLE_PITMGMT_
    IMG_BOOL                bIsLongTermRef;
    IMG_BOOL                bUsesLongTermRef0;
    IMG_BOOL                bUsesLongTermRef1;
    IMG_UINT32              ui32FrameCnt;

    // Determine frame position in source stream
    // This assumes there are no IDR frames after the first one
    if (ui32FrameNum == 0) {
        // Initial IDR frame
        ui32FrameCnt = 0;
    } else if (((ui32FrameNum - 1) % (ctx->sRCParams.ui16BFrames + 1)) == 0) {
        // I or P frame
        ui32FrameCnt = ui32FrameNum + ctx->sRCParams.ui16BFrames;
        if (ui32FrameCnt >= ctx->ui32Framecount) ui32FrameCnt = ctx->ui32Framecount - 1;
    } else {
        // B frame
        // This will be incorrect for hierarchical B-pictures
        ui32FrameCnt = ui32FrameNum - 1;
    }

    // Decide if the current frame should be used as a long-term reference
    bIsLongTermRef = ctx->ui32LongTermRefFreq ?
                     (ui32FrameCnt % ctx->ui32LongTermRefFreq == 0) :
                     IMG_FALSE;

    // Decide if the current frame should refer to a long-term reference
    bUsesLongTermRef0 = ctx->ui32LongTermRefUsageFreq ?
                        (ui32FrameCnt % ctx->ui32LongTermRefUsageFreq == ctx->ui32LongTermRefUsageOffset) :
                        IMG_FALSE;
    bUsesLongTermRef1 = IMG_FALSE;

    if (bIsLongTermRef || bUsesLongTermRef0 || bUsesLongTermRef1) {
        // Reconstructed/reference frame to be written to host buffer
        // Send the buffer to be used as reference
        tng__send_ref_frames(ctx, 0, bIsLongTermRef);
        if (bIsLongTermRef) ctx->byCurBufPointer = (ctx->byCurBufPointer + 1) % 3;
    }
#endif
}

static VAStatus tng__H264ES_CalcCustomQuantSp(IMG_UINT8 list, IMG_UINT8 param, IMG_UINT8 customQuantQ)
{
    // Derived from sim/topaz/testbench/tests/mved1_tests.c
    IMG_UINT32 mfflat[2][16] = {
        {
            13107, 8066,   13107,  8066,
            8066,   5243,   8066,   5243,
            13107,  8066,   13107,  8066,
            8066,   5243,   8066,   5243
        }, // 4x4
        {
            13107, 12222,  16777,  12222,
            12222,  11428,  15481,  11428,
            16777,  15481,  20972,  15481,
            12222,  11428,  15481,  11428
        } // 8x8
    };
    IMG_UINT8 uVi[2][16] = {
        {
            20, 26,  20,  26,
            26,  32,  26,  32,
            20,  26,  20,  26,
            26,  32,  26,  32
        }, // 4x4
        {
            20, 19,  25,  19,
            19,  18,  24,  18,
            25,  24,  32,  24,
            19,  18,  24,  18
        } // 8x8
    };

    int mfnew;
    double fSp;
    int uSp;

    if (customQuantQ == 0) customQuantQ = 1;
    mfnew = (mfflat[list][param] * 16) / customQuantQ;
    fSp = ((double)(mfnew * uVi[list][param])) / (double)(1 << 22);
    fSp = (fSp * 100000000.0f) / 100000000.0f;
    uSp = (IMG_UINT16)(fSp * 65536);

    return uSp & 0x03FFF;
}


static VAStatus tng__set_custom_scaling_values(
    context_ENC_p ctx,
    IMG_UINT8* aui8Sl4x4IntraY,
    IMG_UINT8* aui8Sl4x4IntraCb,
    IMG_UINT8* aui8Sl4x4IntraCr,
    IMG_UINT8* aui8Sl4x4InterY,
    IMG_UINT8* aui8Sl4x4InterCb,
    IMG_UINT8* aui8Sl4x4InterCr,
    IMG_UINT8* aui8Sl8x8IntraY,
    IMG_UINT8* aui8Sl8x8InterY)
{
    IMG_UINT8  *pui8QuantMem;
    IMG_UINT32 *pui32QuantReg;
    IMG_UINT8  *apui8QuantTables[8];
    IMG_UINT32  ui32Table, ui32Val;
    psb_buffer_p pCustomBuf = NULL;
    IMG_UINT32  custom_quant_size = 0;

    // Scanning order for coefficients, see section 8.5.5 of H.264 specification
    // Note that even for interlaced mode, hardware takes the scaling values as if frame zig-zag scanning were being used
    IMG_UINT8 aui8ZigZagScan4x4[16] = {
        0,  1,  5,  6,
        2,  4,  7,  12,
        3,  8,  11, 13,
        9,  10, 14, 15
    };
    IMG_UINT8 aui8ZigZagScan8x8[64] = {
        0,  1,  5,  6,  14, 15, 27, 28,
        2,  4,  7,  13, 16, 26, 29, 42,
        3,  8,  12, 17, 25, 30, 41, 43,
        9,  11, 18, 24, 31, 40, 44, 53,
        10, 19, 23, 32, 39, 45, 52, 54,
        20, 22, 33, 38, 46, 51, 55, 60,
        21, 34, 37, 47, 50, 56, 59, 61,
        35, 36, 48, 49, 57, 58, 62, 63
    };


    if (ctx == NULL) {
        return VA_STATUS_ERROR_UNKNOWN;
    }

    if (ctx->bCustomScaling == IMG_FALSE) {
        return VA_STATUS_ERROR_UNKNOWN;
    }

    pCustomBuf = &(ctx->ctx_mem[ctx->ui32StreamID].bufs_custom_quant);
    custom_quant_size = ctx->ctx_mem_size.custom_quant;


    /* Copy quantization values (in header order) */
    pui8QuantMem = (IMG_UINT8*)(pCustomBuf);
    memcpy(pui8QuantMem, aui8Sl4x4IntraY, 16);
    memcpy(pui8QuantMem + 16, aui8Sl4x4IntraCb, 16);
    memcpy(pui8QuantMem + 32, aui8Sl4x4IntraCr, 16);
    memcpy(pui8QuantMem + 48, aui8Sl4x4InterY, 16);
    memcpy(pui8QuantMem + 64, aui8Sl4x4InterCb, 16);
    memcpy(pui8QuantMem + 80, aui8Sl4x4InterCr, 16);
    memcpy(pui8QuantMem + 96, aui8Sl8x8IntraY, 64);
    memcpy(pui8QuantMem + 160, aui8Sl8x8InterY, 64);

    /* Create quantization register values */

    /* Assign based on the order values are written to registers */
    apui8QuantTables[0] = aui8Sl4x4IntraY;
    apui8QuantTables[1] = aui8Sl4x4InterY;
    apui8QuantTables[2] = aui8Sl4x4IntraCb;
    apui8QuantTables[3] = aui8Sl4x4InterCb;
    apui8QuantTables[4] = aui8Sl4x4IntraCr;
    apui8QuantTables[5] = aui8Sl4x4InterCr;
    apui8QuantTables[6] = aui8Sl8x8IntraY;
    apui8QuantTables[7] = aui8Sl8x8InterY;

    /* H264COMP_CUSTOM_QUANT_SP register values "psCustomQuantRegs4x4Sp"*/
    pui8QuantMem = (IMG_UINT8*)(pCustomBuf + custom_quant_size);
    pui32QuantReg = (IMG_UINT32 *)pui8QuantMem;
    for (ui32Table = 0; ui32Table < 6; ui32Table++) {
        for (ui32Val = 0; ui32Val < 16; ui32Val += 4) {
            *pui32QuantReg = F_ENCODE(tng__H264ES_CalcCustomQuantSp(0, ui32Val, apui8QuantTables[ui32Table][aui8ZigZagScan4x4[ui32Val]]), TOPAZHP_CR_H264COMP_CUSTOM_QUANT_SP_0)
                             | F_ENCODE(tng__H264ES_CalcCustomQuantSp(0, ui32Val + 1, apui8QuantTables[ui32Table][aui8ZigZagScan4x4[ui32Val + 1]]), TOPAZHP_CR_H264COMP_CUSTOM_QUANT_SP_1);
            pui32QuantReg++;
            *pui32QuantReg = F_ENCODE(tng__H264ES_CalcCustomQuantSp(0, ui32Val + 2, apui8QuantTables[ui32Table][aui8ZigZagScan4x4[ui32Val + 2]]), TOPAZHP_CR_H264COMP_CUSTOM_QUANT_SP_0)
                             | F_ENCODE(tng__H264ES_CalcCustomQuantSp(0, ui32Val + 3, apui8QuantTables[ui32Table][aui8ZigZagScan4x4[ui32Val + 3]]), TOPAZHP_CR_H264COMP_CUSTOM_QUANT_SP_1);
            pui32QuantReg++;
        }
    }

    /*psCustomQuantRegs8x8Sp*/
    pui8QuantMem = (IMG_UINT8*)(pCustomBuf + custom_quant_size + custom_quant_size);
    pui32QuantReg = (IMG_UINT32 *)pui8QuantMem;
    for (; ui32Table < 8; ui32Table++) {
        for (ui32Val = 0; ui32Val < 64; ui32Val += 8) {
            *pui32QuantReg = F_ENCODE(tng__H264ES_CalcCustomQuantSp(1, ((ui32Val & 24) >> 1), apui8QuantTables[ui32Table][aui8ZigZagScan8x8[ui32Val]]), TOPAZHP_CR_H264COMP_CUSTOM_QUANT_SP_0)
                             | F_ENCODE(tng__H264ES_CalcCustomQuantSp(1, ((ui32Val & 24) >> 1) + 1, apui8QuantTables[ui32Table][aui8ZigZagScan8x8[ui32Val + 1]]), TOPAZHP_CR_H264COMP_CUSTOM_QUANT_SP_1);
            pui32QuantReg++;
            *pui32QuantReg = F_ENCODE(tng__H264ES_CalcCustomQuantSp(1, ((ui32Val & 24) >> 1) + 2, apui8QuantTables[ui32Table][aui8ZigZagScan8x8[ui32Val + 2]]), TOPAZHP_CR_H264COMP_CUSTOM_QUANT_SP_0)
                             | F_ENCODE(tng__H264ES_CalcCustomQuantSp(1, ((ui32Val & 24) >> 1) + 3, apui8QuantTables[ui32Table][aui8ZigZagScan8x8[ui32Val + 3]]), TOPAZHP_CR_H264COMP_CUSTOM_QUANT_SP_1);
            pui32QuantReg++;
            *pui32QuantReg = F_ENCODE(tng__H264ES_CalcCustomQuantSp(1, ((ui32Val & 24) >> 1), apui8QuantTables[ui32Table][aui8ZigZagScan8x8[ui32Val + 4]]), TOPAZHP_CR_H264COMP_CUSTOM_QUANT_SP_0)
                             | F_ENCODE(tng__H264ES_CalcCustomQuantSp(1, ((ui32Val & 24) >> 1) + 1, apui8QuantTables[ui32Table][aui8ZigZagScan8x8[ui32Val + 5]]), TOPAZHP_CR_H264COMP_CUSTOM_QUANT_SP_1);
            pui32QuantReg++;
            *pui32QuantReg = F_ENCODE(tng__H264ES_CalcCustomQuantSp(1, ((ui32Val & 24) >> 1) + 2, apui8QuantTables[ui32Table][aui8ZigZagScan8x8[ui32Val + 6]]), TOPAZHP_CR_H264COMP_CUSTOM_QUANT_SP_0)
                             | F_ENCODE(tng__H264ES_CalcCustomQuantSp(1, ((ui32Val & 24) >> 1) + 3, apui8QuantTables[ui32Table][aui8ZigZagScan8x8[ui32Val + 7]]), TOPAZHP_CR_H264COMP_CUSTOM_QUANT_SP_1);
            pui32QuantReg++;
        }
    }

    /* H264COMP_CUSTOM_QUANT_Q register values "psCustomQuantRegs4x4Q" */
    pui8QuantMem = (IMG_UINT8*)(pCustomBuf + custom_quant_size + custom_quant_size + custom_quant_size);
    pui32QuantReg = (IMG_UINT32 *)pui8QuantMem;
    for (ui32Table = 0; ui32Table < 6; ui32Table++) {
        for (ui32Val = 0; ui32Val < 16; ui32Val += 4) {
            *pui32QuantReg = F_ENCODE(apui8QuantTables[ui32Table][aui8ZigZagScan4x4[ui32Val]], TOPAZHP_CR_H264COMP_CUSTOM_QUANT_Q_0)
                             | F_ENCODE(apui8QuantTables[ui32Table][aui8ZigZagScan4x4[ui32Val + 1]], TOPAZHP_CR_H264COMP_CUSTOM_QUANT_Q_1)
                             | F_ENCODE(apui8QuantTables[ui32Table][aui8ZigZagScan4x4[ui32Val + 2]], TOPAZHP_CR_H264COMP_CUSTOM_QUANT_Q_2)
                             | F_ENCODE(apui8QuantTables[ui32Table][aui8ZigZagScan4x4[ui32Val + 3]], TOPAZHP_CR_H264COMP_CUSTOM_QUANT_Q_3);
            pui32QuantReg++;
        }
    }

    /*psCustomQuantRegs8x8Q)*/
    pui8QuantMem = (IMG_UINT8*)(pCustomBuf + custom_quant_size + custom_quant_size + custom_quant_size + custom_quant_size);

    pui32QuantReg = (IMG_UINT32 *)pui8QuantMem;
    for (; ui32Table < 8; ui32Table++) {
        for (ui32Val = 0; ui32Val < 64; ui32Val += 8) {
            *pui32QuantReg = F_ENCODE(apui8QuantTables[ui32Table][aui8ZigZagScan8x8[ui32Val]], TOPAZHP_CR_H264COMP_CUSTOM_QUANT_Q_0)
                             | F_ENCODE(apui8QuantTables[ui32Table][aui8ZigZagScan8x8[ui32Val + 1]], TOPAZHP_CR_H264COMP_CUSTOM_QUANT_Q_1)
                             | F_ENCODE(apui8QuantTables[ui32Table][aui8ZigZagScan8x8[ui32Val + 2]], TOPAZHP_CR_H264COMP_CUSTOM_QUANT_Q_2)
                             | F_ENCODE(apui8QuantTables[ui32Table][aui8ZigZagScan8x8[ui32Val + 3]], TOPAZHP_CR_H264COMP_CUSTOM_QUANT_Q_3);
            pui32QuantReg++;
            *pui32QuantReg = F_ENCODE(apui8QuantTables[ui32Table][aui8ZigZagScan8x8[ui32Val + 4]], TOPAZHP_CR_H264COMP_CUSTOM_QUANT_Q_0)
                             | F_ENCODE(apui8QuantTables[ui32Table][aui8ZigZagScan8x8[ui32Val + 5]], TOPAZHP_CR_H264COMP_CUSTOM_QUANT_Q_1)
                             | F_ENCODE(apui8QuantTables[ui32Table][aui8ZigZagScan8x8[ui32Val + 6]], TOPAZHP_CR_H264COMP_CUSTOM_QUANT_Q_2)
                             | F_ENCODE(apui8QuantTables[ui32Table][aui8ZigZagScan8x8[ui32Val + 7]], TOPAZHP_CR_H264COMP_CUSTOM_QUANT_Q_3);
            pui32QuantReg++;
        }
    }

    if (ctx->bPpsScaling)
        ctx->bInsertPicHeader = IMG_TRUE;

    return VA_STATUS_SUCCESS;
}


void tng__picmgmt_custom_scaling(context_ENC_p ctx, IMG_UINT32 ui32FrameNum)
{
    if (ui32FrameNum % ctx->ui32PpsScalingCnt == 0) {
        // Swap inter and intra scaling lists on alternating picture parameter sets
        if (ui32FrameNum % (ctx->ui32PpsScalingCnt * 2) == 0) {
            tng__set_custom_scaling_values(
                ctx,
                ctx->aui8CustomQuantParams4x4[0],
                ctx->aui8CustomQuantParams4x4[1],
                ctx->aui8CustomQuantParams4x4[2],
                ctx->aui8CustomQuantParams4x4[3],
                ctx->aui8CustomQuantParams4x4[4],
                ctx->aui8CustomQuantParams4x4[5],
                ctx->aui8CustomQuantParams8x8[0],
                ctx->aui8CustomQuantParams8x8[1]);
        } else {
            tng__set_custom_scaling_values(
                ctx,
                ctx->aui8CustomQuantParams4x4[3],
                ctx->aui8CustomQuantParams4x4[4],
                ctx->aui8CustomQuantParams4x4[5],
                ctx->aui8CustomQuantParams4x4[0],
                ctx->aui8CustomQuantParams4x4[1],
                ctx->aui8CustomQuantParams4x4[2],
                ctx->aui8CustomQuantParams8x8[1],
                ctx->aui8CustomQuantParams8x8[0]);
        }
    }
}

/************************* MTX_CMDID_PROVIDE_BUFFER *************************/
IMG_UINT32 tng_send_codedbuf(
    context_ENC_p ctx,
    IMG_UINT32 ui32SlotIndex)
{
    context_ENC_frame_buf *ps_buf = &(ctx->ctx_frame_buf);
    object_buffer_p object_buffer  = ps_buf->coded_buf;
    IMG_UINT32 ui32Offset = 0;

    drv_debug_msg(VIDEO_DEBUG_GENERAL,
        "%s slot 1 = %x\n", __FUNCTION__, ui32SlotIndex);

    if ((ctx->ui8PipesToUse == 2) && ((ui32SlotIndex & 1) == 1))
	ui32Offset = object_buffer->size >> 1;

    tng_cmdbuf_insert_command(
        ctx->obj_context, ctx->ui32StreamID,
        MTX_CMDID_PROVIDE_CODED_BUFFER,
        F_ENCODE(object_buffer->size, MTX_MSG_PROVIDE_CODED_BUFFER_SIZE) |
        F_ENCODE(ui32SlotIndex, MTX_MSG_PROVIDE_CODED_BUFFER_SLOT),
        object_buffer->psb_buffer, tng_align_KB(ui32Offset));

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s end\n", __FUNCTION__);
    return  VA_STATUS_SUCCESS;
}

static VAStatus tng__set_component_offsets(
    context_ENC_p ctx,
    object_surface_p obj_surface_p,
    IMG_FRAME * psFrame
)
{
    IMG_FORMAT eFormat;
    IMG_UINT16 ui16Width;
    IMG_UINT16 ui16Stride;
    IMG_UINT16 ui16PictureHeight;

    if (!ctx)
        return VA_STATUS_ERROR_UNKNOWN;
    // if source slot is NULL then it's just a next portion of slices
    if (psFrame == IMG_NULL)
        return VA_STATUS_ERROR_UNKNOWN;

    eFormat = ctx->eFormat;
    ui16Width = obj_surface_p->width;
    ui16PictureHeight = obj_surface_p->height;
    ui16Stride = obj_surface_p->psb_surface->stride;
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s eFormat = %d, w = %d, h = %d, stride = %d\n",
        __FUNCTION__, eFormat, ui16Width, ui16PictureHeight, ui16Stride);
    // 3 Components: Y, U, V
    // Y component is always at the beginning
    psFrame->i32YComponentOffset = 0;
    psFrame->ui16SrcYStride = ui16Stride;

    // Assume for now that field 0 comes first
    psFrame->i32Field0YOffset = 0;
    psFrame->i32Field0UOffset = 0;
    psFrame->i32Field0VOffset = 0;


    switch (eFormat) {
    case IMG_CODEC_YUV:
        psFrame->ui16SrcUVStride = ui16Stride / 2;              // ui16SrcUStride

        psFrame->i32UComponentOffset = ui16Stride * ui16PictureHeight;   // ui16SrcUBase
        psFrame->i32VComponentOffset = ui16Stride * ui16PictureHeight + (ui16Stride / 2) * (ui16PictureHeight / 2); // ui16SrcVBase
        break;

    case IMG_CODEC_PL8:
        psFrame->ui16SrcUVStride = ui16Stride / 2;              // ui16SrcUStride

        psFrame->i32UComponentOffset = 0;   // ui16SrcUBase
        psFrame->i32VComponentOffset = 0; // ui16SrcVBase
        break;

    case IMG_CODEC_PL12:
        psFrame->ui16SrcUVStride = ui16Stride;                         // ui16SrcUStride
        //FIXME
        psFrame->i32UComponentOffset = ui16Stride * ui16PictureHeight; // ui16SrcUBase
        psFrame->i32VComponentOffset = ui16Stride * ui16PictureHeight; // ui16SrcVBase
        break;

    case IMG_CODEC_YV12:    /* YV12 */
        psFrame->ui16SrcUVStride = ui16Stride / 2;              // ui16SrcUStride

        psFrame->i32UComponentOffset = ui16Stride * ui16PictureHeight + (ui16Stride / 2) * (ui16PictureHeight / 2);   // ui16SrcUBase
        psFrame->i32VComponentOffset = ui16Stride * ui16PictureHeight; // ui16SrcVBase
        break;

    case IMG_CODEC_IMC2:    /* IMC2 */
        psFrame->ui16SrcUVStride = ui16Stride;                  // ui16SrcUStride

        psFrame->i32UComponentOffset = ui16Stride * ui16PictureHeight + (ui16Stride / 2);   // ui16SrcUBase
        psFrame->i32VComponentOffset = ui16Stride * ui16PictureHeight; // ui16SrcVBase
        break;

    case IMG_CODEC_422_YUV:
        psFrame->ui16SrcUVStride = ui16Stride;          // ui16SrcUStride

        psFrame->i32UComponentOffset = ui16Stride * ui16PictureHeight;   // ui16SrcUBase
        psFrame->i32VComponentOffset = ui16Stride * ui16PictureHeight + (ui16Stride / 2) * ui16PictureHeight; // ui16SrcVBase
        break;

    case IMG_CODEC_422_YV12:        /* YV16 */
        psFrame->ui16SrcUVStride = ui16Stride;          // ui16SrcUStride

        psFrame->i32UComponentOffset = ui16Stride * ui16PictureHeight + (ui16Stride / 2) * ui16PictureHeight;   // ui16SrcUBase
        psFrame->i32VComponentOffset = ui16Stride * ui16PictureHeight; // ui16SrcVBase
        break;

    case IMG_CODEC_422_PL8:
        psFrame->ui16SrcUVStride = ui16Stride;          // ui16SrcUStride

        psFrame->i32UComponentOffset = 0;   // ui16SrcUBase
        psFrame->i32VComponentOffset = 0; // ui16SrcVBase
        break;

    case IMG_CODEC_422_IMC2:        /* IMC2 */
        psFrame->ui16SrcUVStride = ui16Stride * 2;                      // ui16SrcUStride

        psFrame->i32UComponentOffset = ui16Stride * ui16PictureHeight + (ui16Stride / 2);   // ui16SrcUBase
        psFrame->i32VComponentOffset = ui16Stride * ui16PictureHeight; // ui16SrcVBase
        break;

    case IMG_CODEC_422_PL12:
        psFrame->ui16SrcUVStride = ui16Stride * 2;                      // ui16SrcUStride

        psFrame->i32UComponentOffset = 0;   // ui16SrcUBase
        psFrame->i32VComponentOffset = 0; // ui16SrcVBase
        break;

    case IMG_CODEC_Y0UY1V_8888:
    case IMG_CODEC_Y0VY1U_8888:
    case IMG_CODEC_UY0VY1_8888:
    case IMG_CODEC_VY0UY1_8888:
        psFrame->ui16SrcUVStride = ui16Stride;                  // ui16SrcUStride

        psFrame->i32UComponentOffset = 0;   // ui16SrcUBase
        psFrame->i32VComponentOffset = 0; // ui16SrcVBase
        break;

    default:
        break;
    }

    if (ctx->bIsInterlaced) {
        if (ctx->bIsInterleaved) {
            switch (eFormat) {
            case IMG_CODEC_IMC2:
            case IMG_CODEC_422_IMC2:
                psFrame->i32VComponentOffset *= 2;
                psFrame->i32UComponentOffset = psFrame->i32VComponentOffset + (ui16Stride / 2);
                break;
            default:
                psFrame->i32UComponentOffset *= 2;
                psFrame->i32VComponentOffset *= 2;
                break;
            }

            psFrame->i32Field1YOffset = psFrame->i32Field0YOffset + psFrame->ui16SrcYStride;
            psFrame->i32Field1UOffset = psFrame->i32Field0UOffset + psFrame->ui16SrcUVStride;
            psFrame->i32Field1VOffset = psFrame->i32Field0VOffset + psFrame->ui16SrcUVStride;

            psFrame->ui16SrcYStride *= 2;                           // ui16SrcYStride
            psFrame->ui16SrcUVStride *= 2;                  // ui16SrcUStride

            if (!ctx->bTopFieldFirst)       {
                IMG_INT32 i32Temp;

                i32Temp = psFrame->i32Field1YOffset;
                psFrame->i32Field1YOffset = psFrame->i32Field0YOffset;
                psFrame->i32Field0YOffset = i32Temp;

                i32Temp = psFrame->i32Field1UOffset;
                psFrame->i32Field1UOffset = psFrame->i32Field0UOffset;
                psFrame->i32Field0UOffset = i32Temp;

                i32Temp = psFrame->i32Field1VOffset;
                psFrame->i32Field1VOffset = psFrame->i32Field0VOffset;
                psFrame->i32Field0VOffset = i32Temp;
            }
        } else {
            IMG_UINT32 ui32YFieldSize, ui32CFieldSize;

            switch (eFormat) {
            case IMG_CODEC_Y0UY1V_8888:
            case IMG_CODEC_UY0VY1_8888:
            case IMG_CODEC_Y0VY1U_8888:
            case IMG_CODEC_VY0UY1_8888:
                ui32YFieldSize = ui16PictureHeight * ui16Stride * 2;
                ui32CFieldSize = ui32YFieldSize;
                break;
            case IMG_CODEC_PL8:
                ui32YFieldSize = ui16PictureHeight * ui16Stride;
                ui32CFieldSize = ui16PictureHeight * ui16Stride / 4;
                break;
            case IMG_CODEC_PL12:
                ui32YFieldSize = ui16PictureHeight * ui16Stride;
                ui32CFieldSize = ui16PictureHeight * ui16Stride / 2;
                break;
            case IMG_CODEC_422_YUV:
            case IMG_CODEC_422_YV12:
            case IMG_CODEC_422_IMC2:
                ui32YFieldSize = ui16PictureHeight * ui16Stride * 2;
                ui32CFieldSize = ui32YFieldSize;
                break;
            case IMG_CODEC_422_PL8:
                ui32YFieldSize = ui16PictureHeight * ui16Stride;
                ui32CFieldSize = ui16PictureHeight * ui16Stride / 2;
                break;
            case IMG_CODEC_422_PL12:
                ui32YFieldSize = ui16PictureHeight * ui16Stride;
                ui32CFieldSize = ui32YFieldSize;
                break;
            default:
                ui32YFieldSize = ui16PictureHeight * ui16Stride * 3 / 2;
                ui32CFieldSize = ui32YFieldSize;
                break;
            }

            psFrame->i32Field1YOffset = ui32YFieldSize;
            psFrame->i32Field1UOffset = ui32CFieldSize;
            psFrame->i32Field1VOffset = ui32CFieldSize;
        }
    } else {
        psFrame->i32Field1YOffset = psFrame->i32Field0YOffset;
        psFrame->i32Field1UOffset = psFrame->i32Field0UOffset;
        psFrame->i32Field1VOffset = psFrame->i32Field0VOffset;
    }
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s i32YComponentOffset = %d, i32UComponentOffset = %d, i32VComponentOffset = %d\n",
        __FUNCTION__, (int)(psFrame->i32YComponentOffset), (int)(psFrame->i32UComponentOffset), (int)(psFrame->i32VComponentOffset));
     return VA_STATUS_SUCCESS;
}

IMG_UINT32 tng_send_source_frame(
    context_ENC_p ctx,
    IMG_UINT32 ui32SlotIndex,
    IMG_UINT32 ui32DisplayOrder)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    context_ENC_frame_buf *ps_buf = &(ctx->ctx_frame_buf);
    IMG_FRAME  sSrcFrame;
    IMG_FRAME  *psSrcFrame = &sSrcFrame;
    tng_cmdbuf_p cmdbuf = ctx->obj_context->tng_cmdbuf;
    IMG_SOURCE_BUFFER_PARAMS  *psSrcBufParams = NULL;
    object_surface_p src_surface = ps_buf->src_surface;
    unsigned int frame_mem_index = 0;
    unsigned int srf_buf_offset = src_surface->psb_surface->buf.buffer_ofs;

    drv_debug_msg(VIDEO_DEBUG_GENERAL,
        "%s: ui32SlotIndex = %d, ui32DisplayOrder = %d\n",
        __FUNCTION__, ui32SlotIndex, ui32DisplayOrder);

    if (cmdbuf->frame_mem_index >= COMM_CMD_FRAME_BUF_NUM) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: Error: frame_mem buffer index overflow\n", __FUNCTION__);
        cmdbuf->frame_mem_index = 0;
    }

    vaStatus = psb_buffer_map(&cmdbuf->frame_mem, &(cmdbuf->frame_mem_p));
    if (vaStatus) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: map frame buf\n", __FUNCTION__);
        return vaStatus;
    }

    frame_mem_index = cmdbuf->frame_mem_index * cmdbuf->mem_size;
    psSrcBufParams = (IMG_SOURCE_BUFFER_PARAMS *)(cmdbuf->frame_mem_p + frame_mem_index);
    memset(psSrcBufParams, 0, sizeof(IMG_SOURCE_BUFFER_PARAMS));
    memset(psSrcFrame, 0, sizeof(IMG_FRAME));
    tng__set_component_offsets(ctx, src_surface, psSrcFrame);

    drv_debug_msg(VIDEO_DEBUG_GENERAL,
        "%s: cmdbuf->frame_mem_index = %d, frame_mem_index = 0x%08x, cmdbuf->frame_mem_p = 0x%08x\n",
        __FUNCTION__, cmdbuf->frame_mem_index, frame_mem_index, cmdbuf->frame_mem_p);
    drv_debug_msg(VIDEO_DEBUG_GENERAL,
        "%s: frame_mem_index = %d, psBufferParams = 0x%08x\n",
        __FUNCTION__, frame_mem_index, (unsigned int)psSrcBufParams);

    /* Prepare ProvideBuffer data */
    {
        psSrcBufParams->ui8SlotNum = (IMG_UINT8)(ui32SlotIndex & 0xff);
        psSrcBufParams->ui8DisplayOrderNum = (IMG_UINT8)(ui32DisplayOrder & 0xff);
        psSrcBufParams->ui32HostContext = (IMG_UINT32)ctx;

#ifdef _TNG_RELOC_
        TNG_RELOC_CMDBUF_FRAMES(
            &(psSrcBufParams->ui32PhysAddrYPlane_Field0),
            srf_buf_offset + psSrcFrame->i32YComponentOffset + psSrcFrame->i32Field0YOffset,
            &(src_surface->psb_surface->buf));
        TNG_RELOC_CMDBUF_FRAMES(
            &(psSrcBufParams->ui32PhysAddrUPlane_Field0),
            srf_buf_offset + psSrcFrame->i32UComponentOffset + psSrcFrame->i32Field0UOffset,
            &(src_surface->psb_surface->buf));
        TNG_RELOC_CMDBUF_FRAMES(
            &(psSrcBufParams->ui32PhysAddrVPlane_Field0),
            srf_buf_offset + psSrcFrame->i32VComponentOffset + psSrcFrame->i32Field0VOffset,
            &(src_surface->psb_surface->buf));

        TNG_RELOC_CMDBUF_FRAMES(
            &(psSrcBufParams->ui32PhysAddrYPlane_Field1),
            srf_buf_offset + psSrcFrame->i32YComponentOffset + psSrcFrame->i32Field1YOffset,
            &(src_surface->psb_surface->buf));
        TNG_RELOC_CMDBUF_FRAMES(
            &(psSrcBufParams->ui32PhysAddrUPlane_Field1),
            srf_buf_offset + psSrcFrame->i32UComponentOffset + psSrcFrame->i32Field1UOffset,
            &(src_surface->psb_surface->buf));
        TNG_RELOC_CMDBUF_FRAMES(
            &(psSrcBufParams->ui32PhysAddrVPlane_Field1),
            srf_buf_offset + psSrcFrame->i32VComponentOffset + psSrcFrame->i32Field1VOffset,
            &(src_surface->psb_surface->buf));
#else
        tng_cmdbuf_set_phys(&(psSrcBufParams->ui32PhysAddrYPlane_Field0), 0,
            &(src_surface->psb_surface->buf), 
            srf_buf_offset + psSrcFrame->i32YComponentOffset + psSrcFrame->i32Field0YOffset, 0);
        tng_cmdbuf_set_phys(&(psSrcBufParams->ui32PhysAddrUPlane_Field0), 0,
            &(src_surface->psb_surface->buf), 
            srf_buf_offset + psSrcFrame->i32UComponentOffset + psSrcFrame->i32Field0UOffset, 0);
        tng_cmdbuf_set_phys(&(psSrcBufParams->ui32PhysAddrVPlane_Field0), 0,
            &(src_surface->psb_surface->buf), 
            srf_buf_offset + psSrcFrame->i32VComponentOffset + psSrcFrame->i32Field0VOffset, 0);
    
        tng_cmdbuf_set_phys(&(psSrcBufParams->ui32PhysAddrYPlane_Field1), 0,
            &(src_surface->psb_surface->buf), 
            srf_buf_offset + psSrcFrame->i32YComponentOffset + psSrcFrame->i32Field1YOffset, 0);
        tng_cmdbuf_set_phys(&(psSrcBufParams->ui32PhysAddrUPlane_Field1), 0,
            &(src_surface->psb_surface->buf), 
            srf_buf_offset + psSrcFrame->i32UComponentOffset + psSrcFrame->i32Field1UOffset, 0);
        tng_cmdbuf_set_phys(&(psSrcBufParams->ui32PhysAddrVPlane_Field1), 0,
            &(src_surface->psb_surface->buf), 
            srf_buf_offset + psSrcFrame->i32VComponentOffset + psSrcFrame->i32Field1VOffset, 0);
#endif
    }
    drv_debug_msg(VIDEO_DEBUG_GENERAL, 
        "%s slot_idx = %d, frame_count = %d\n", __FUNCTION__,
        (int)(ui32SlotIndex), (int)(ctx->ui32FrameCount[ctx->ui32StreamID]));
    drv_debug_msg(VIDEO_DEBUG_GENERAL,
        "%s: YPlane_Field0 = 0x%08x, UPlane_Field0 = 0x%08x, VPlane_Field0 = 0x%08x\n",
        __FUNCTION__, (unsigned int)(psSrcBufParams->ui32PhysAddrYPlane_Field0),
        (unsigned int)(psSrcBufParams->ui32PhysAddrUPlane_Field0),
        (unsigned int)(psSrcBufParams->ui32PhysAddrVPlane_Field0));
    drv_debug_msg(VIDEO_DEBUG_GENERAL,
        "%s: YPlane_Field1 = 0x%08x, UPlane_Field1 = 0x%08x, VPlane_Field1 = 0x%08x\n",
        __FUNCTION__, (unsigned int)(psSrcBufParams->ui32PhysAddrYPlane_Field1),
        (unsigned int)(psSrcBufParams->ui32PhysAddrUPlane_Field1),
        (unsigned int)(psSrcBufParams->ui32PhysAddrVPlane_Field1));

    /* Send ProvideBuffer Command */
    tng_cmdbuf_insert_command(ctx->obj_context, ctx->ui32StreamID,
        MTX_CMDID_PROVIDE_SOURCE_BUFFER,
        0, &(cmdbuf->frame_mem), frame_mem_index);

    ++(cmdbuf->frame_mem_index);
    psb_buffer_unmap(&cmdbuf->frame_mem);

    return 0;
}


IMG_UINT32 tng_send_rec_frames(
    context_ENC_p ctx,
    IMG_INT8 i8HeaderSlotNum,
    IMG_BOOL bLongTerm)
{
    //VAStatus vaStatus = VA_STATUS_SUCCESS;
    unsigned int srf_buf_offset;
    context_ENC_frame_buf *ps_buf = &(ctx->ctx_frame_buf);
    object_surface_p rec_surface = ps_buf->rec_surface;
    IMG_UINT32 ui32CmdData = 0;

    srf_buf_offset = rec_surface->psb_surface->buf.buffer_ofs;
    /* Send ProvideBuffer Command */
    ui32CmdData = F_ENCODE(IMG_BUFFER_RECON, MTX_MSG_PROVIDE_REF_BUFFER_USE) |
        F_ENCODE(i8HeaderSlotNum, MTX_MSG_PROVIDE_REF_BUFFER_SLOT) |
        F_ENCODE(bLongTerm, MTX_MSG_PROVIDE_REF_BUFFER_LT);

    tng_cmdbuf_insert_command(ctx->obj_context, ctx->ui32StreamID,
        MTX_CMDID_PROVIDE_REF_BUFFER,
        ui32CmdData, &(rec_surface->psb_surface->buf), 0);

    return 0;
}

IMG_UINT32 tng_send_ref_frames(
    context_ENC_p ctx,
    IMG_UINT32    ui32refindex,
    IMG_BOOL      __maybe_unused bLongTerm)
{
    //VAStatus vaStatus = VA_STATUS_SUCCESS;
    unsigned int srf_buf_offset;
    context_ENC_frame_buf *ps_buf = &(ctx->ctx_frame_buf);
    object_surface_p ref_surface = ps_buf->ref_surface[ui32refindex];
    IMG_UINT32 ui32CmdData = 0;

#ifdef _TNG_FRAMES_
    if (ui32RefIndex == 0) {
        ref_surface = ps_buf->ref_surface;
        ui32CmdData = F_ENCODE(IMG_BUFFER_REF0, MTX_MSG_PROVIDE_REF_BUFFER_USE) |
            F_ENCODE(bLongTerm, MTX_MSG_PROVIDE_REF_BUFFER_LT);
    } else {
        ref_surface = ps_buf->ref_surface1;
        ui32CmdData = F_ENCODE(IMG_BUFFER_REF1, MTX_MSG_PROVIDE_REF_BUFFER_USE) |
        F_ENCODE(bLongTerm, MTX_MSG_PROVIDE_REF_BUFFER_LT);
    }
#endif
    srf_buf_offset = ref_surface->psb_surface->buf.buffer_ofs;
    /* Send ProvideBuffer Command */
    tng_cmdbuf_insert_command(ctx->obj_context, ctx->ui32StreamID,
        MTX_CMDID_PROVIDE_REF_BUFFER,
        ui32CmdData, &(ref_surface->psb_surface->buf), 0);

    return VA_STATUS_SUCCESS;
}

