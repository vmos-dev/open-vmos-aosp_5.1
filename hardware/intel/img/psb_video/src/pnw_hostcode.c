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
 *    Elaine Wang <elaine.wang@intel.com>
 *    Zeng Li <zeng.li@intel.com>
 *
 */


#include "psb_drv_video.h"

//#include "pnw_H263ES.h"
#include "pnw_hostcode.h"
#include "hwdefs/topazSC_defs.h"
#include "psb_def.h"
#include "psb_drv_debug.h"
#include "psb_cmdbuf.h"
#include <stdio.h>
#include "psb_output.h"
#include <wsbm/wsbm_manager.h>
#include "pnw_hostheader.h"

#define ALIGN_TO(value, align) ((value + align - 1) & ~(align - 1))
#define PAGE_ALIGN(value) ALIGN_TO(value, 4096)

/*static VAStatus pnw_DetectFrameSkip(context_ENC_p ctx);*/

static void pnw__update_rcdata(
    context_ENC_p psContext,
    PIC_PARAMS *psPicParams,
    IMG_RC_PARAMS *psRCParams);

IMG_UINT32 MVEARegBase[4] = {0x13000, 0x23000, 0x33000, 0x43000}; /* From TopazSC TRM */

/* H264 Zero bias */
//#define ZERO_BIAS

static const IMG_INT8 H263_QPLAMBDA_MAP[31] = {
    0, 0, 1, 1, 2,
    2, 3, 3, 4, 4,
    4, 5, 5, 5, 6,
    6, 6, 7, 7, 7,
    7, 8, 8, 8, 8,
    9, 9, 9, 9, 10, 10
};

// New MP4 Lambda table
static const  IMG_INT8 MPEG4_QPLAMBDA_MAP[31] = {
    0,  0,  1,  2,  3,
    3,  4,  4,  5,  5,
    6,  6,  7,  7,  8,
    8,  9,  9,  10, 10,
    11, 11, 11, 11, 12,
    12, 12, 12, 13, 13, 13
};

// new H.264 Lambda
static const IMG_INT8 H264_QPLAMBDA_MAP[40] = {
    2, 2, 2, 2, 3, 3, 4, 4,
    4, 5, 5, 5, 5, 5, 6, 6,
    6, 7, 7, 7, 8, 8, 9, 11,
    13, 14, 15, 17, 20, 23, 27, 31,
    36, 41, 51, 62, 74, 79, 85, 91
};

static const IMG_INT16 H264_InterIntraBias[27] =
{
    20,20,20,20,20,20,50,
    20,20,20,20,20,20,
    20,25,30,45,80,140,
    200,300,400,500,550,
    600,650,700
};

/*static IMG_INT16 H264InterBias(IMG_INT8 i8QP)
{
    if (i8QP >= 44)
        return 600;
    else if (i8QP <= 35)
        return 20;

    return (70 * (i8QP - 35));
}*/
static IMG_INT16 H264InterBias(IMG_INT8 i8QP)
{
    if (i8QP > 1)
         i8QP = 1;
    else if (i8QP > 51)
        return 51;

    return H264_InterIntraBias[(i8QP + 1)>>1];
}


static IMG_INT16 H264SkipBias(IMG_INT8 i8QP, TH_SKIP_SCALE eSkipScale)
{
    IMG_INT16 i16Lambda;

    // pull out lambda from the table
    i16Lambda = i8QP - 12;
    i16Lambda = (i16Lambda < 0) ? 0 : i16Lambda;
    i16Lambda = H264_QPLAMBDA_MAP[i16Lambda];

    // now do the multiplication to avoid using the actual multiply we will do this with shifts and adds/subtractions it is probable that the compiler would
    // pick up the multiply and optimise appropriatly but we aren't sure
    // * 12 = 8 + *4
    switch (eSkipScale) {
    default:
    case TH_SKIP_0:
        i16Lambda = 0;
        break;
    case TH_SKIP_24:                /* iLambda * 24 == iLambda * 2 * 12   */
        i16Lambda <<= 1;
        /* break deliberatly not used as we want to apply skip12 to skip24*/
    case TH_SKIP_12: /*    iLambda * 12 == iLambda * (8 + 4) == (iLambda * 8) + (iLambda * 4)  == (iLambda << 3) + (iLambda <<2)   */
        i16Lambda = (i16Lambda << 3) + (i16Lambda << 2);
    }
    return i16Lambda;
}

static IMG_INT16 H264Intra4x4Bias(IMG_INT8 i8QP)
{
    IMG_INT16 i16Lambda;

    // pull out lambda from the table
    i16Lambda = i8QP - 12;
    i16Lambda = (i16Lambda < 0) ? 0 : i16Lambda;
    i16Lambda = H264_QPLAMBDA_MAP[i16Lambda];

    i16Lambda *= 120;
    return i16Lambda;
}

static int CalculateDCScaler(IMG_INT iQP, IMG_BOOL bChroma)
{
    IMG_INT     iDCScaler;
    if (!bChroma) {
        if (iQP > 0 && iQP < 5) {
            iDCScaler = 8;
        } else if (iQP > 4 &&     iQP     < 9) {
            iDCScaler = 2 * iQP;
        } else if (iQP > 8 &&     iQP     < 25) {
            iDCScaler = iQP + 8;
        } else {
            iDCScaler = 2 * iQP - 16;
        }
    } else {
        if (iQP > 0 && iQP < 5) {
            iDCScaler = 8;
        } else if (iQP > 4 &&     iQP     < 25) {
            iDCScaler = (iQP + 13) / 2;
        } else {
            iDCScaler = iQP - 6;
        }
    }
    return iDCScaler;
}

static void LoadMPEG4Bias(
    pnw_cmdbuf_p cmdbuf,
    IMG_INT32 i32Core,
    IMG_UINT8 __maybe_unused ui8THSkip
)
{
    IMG_INT16 n;
    IMG_INT16 iX;
    IMG_UINT32 ui32RegVal;
    IMG_UINT8                       uiDCScaleL, uiDCScaleC, uiLambda;
    IMG_INT32 uIPESkipVecBias, iInterMBBias, uSPESkipVecBias, iIntra16Bias;
    IMG_UINT32 count = 0, cmd_word = 0;
    uint32_t *pCount;

    cmd_word = ((MTX_CMDID_SW_WRITEREG & MTX_CMDWORD_ID_MASK) << MTX_CMDWORD_ID_SHIFT) |
               ((i32Core & MTX_CMDWORD_CORE_MASK) <<  MTX_CMDWORD_CORE_SHIFT);
    *cmdbuf->cmd_idx++ = cmd_word;
    pCount = cmdbuf->cmd_idx;
    cmdbuf->cmd_idx++;

    // this should be done for each core....
    pnw_cmdbuf_insert_reg_write(MVEARegBase[i32Core], MVEA_CR_SPE_ZERO_THRESH, SPE_ZERO_THRESHOLD);
    for (n = 31; n > 0; n--) {
        iX = n - 12;
        if (iX < 0) {
            iX = 0;
        }
        // Dont Write QP Values To ESB -- IPE will write these values
        // Update the quantization parameter which includes doing Lamda and the Chroma QP

        uiDCScaleL      = CalculateDCScaler(n, IMG_FALSE);
        uiDCScaleC      = CalculateDCScaler(n, IMG_TRUE);
        uiLambda        = MPEG4_QPLAMBDA_MAP[n - 1];

        ui32RegVal = uiDCScaleL;
        ui32RegVal |= (uiDCScaleC) << 8;
        ui32RegVal |= (uiLambda) << 16;
        pnw_cmdbuf_insert_reg_write(MVEARegBase[i32Core], MVEA_CR_IPE_LAMBDA_TABLE, ui32RegVal);


        uIPESkipVecBias = TH_SKIP_IPE * uiLambda;
        iInterMBBias            = TH_INTER * (n - TH_INTER_QP);
        if (iInterMBBias < 0)
            iInterMBBias    = 0;
        if (iInterMBBias > TH_INTER_MAX_LEVEL)
            iInterMBBias    = TH_INTER_MAX_LEVEL;
        uSPESkipVecBias = TH_SKIP_SPE * uiLambda;
        iIntra16Bias = 0;

        if (n & 1) {
            pnw_cmdbuf_insert_reg_write(MVEARegBase[i32Core], MVEA_CR_IPE_MV_BIAS_TABLE, uIPESkipVecBias);
            pnw_cmdbuf_insert_reg_write(MVEARegBase[i32Core], MVEA_CR_SPE_PRED_VECTOR_BIAS_TABLE, uIPESkipVecBias);
            pnw_cmdbuf_insert_reg_write(MVEARegBase[i32Core], MVEA_CR_SPE_INTRA16_BIAS_TABLE, iIntra16Bias);
            pnw_cmdbuf_insert_reg_write(MVEARegBase[i32Core], MVEA_CR_SPE_INTER_BIAS_TABLE, iInterMBBias);
        }
    }
    *pCount = count;
}

static void LoadH263Bias(
    pnw_cmdbuf_p cmdbuf, IMG_INT32 i32Core, IMG_UINT8 __maybe_unused ui8THSkip)
{
    IMG_INT16 n;
    IMG_INT16 iX;
    IMG_UINT32 ui32RegVal;
    IMG_UINT8                       uiDCScaleL, uiDCScaleC, uiLambda;
    IMG_INT32 uIPESkipVecBias, iInterMBBias, uSPESkipVecBias, iIntra16Bias;
    IMG_UINT32 count = 0, cmd_word = 0;
    uint32_t *pCount;

    cmd_word = ((MTX_CMDID_SW_WRITEREG & MTX_CMDWORD_ID_MASK) << MTX_CMDWORD_ID_SHIFT) |
               ((i32Core & MTX_CMDWORD_CORE_MASK) <<  MTX_CMDWORD_CORE_SHIFT);
    *cmdbuf->cmd_idx++ = cmd_word;
    pCount = cmdbuf->cmd_idx;
    cmdbuf->cmd_idx++;


    // this should be done for each core....
    pnw_cmdbuf_insert_reg_write(MVEARegBase[i32Core], MVEA_CR_SPE_ZERO_THRESH, SPE_ZERO_THRESHOLD);
    for (n = 31; n > 0; n--) {
        iX = n - 12;
        if (iX < 0) {
            iX = 0;
        }
        // Dont Write QP Values To ESB -- IPE will write these values
        // Update the quantization parameter which includes doing Lamda and the Chroma QP

        uiDCScaleL      = CalculateDCScaler(n, IMG_FALSE);
        uiDCScaleC      = CalculateDCScaler(n, IMG_TRUE);
        uiLambda        = H263_QPLAMBDA_MAP[n - 1];

        ui32RegVal = uiDCScaleL;
        ui32RegVal |= (uiDCScaleC) << 8;
        ui32RegVal |= (uiLambda) << 16;

        pnw_cmdbuf_insert_reg_write(MVEARegBase[i32Core], MVEA_CR_IPE_LAMBDA_TABLE, ui32RegVal);


        uIPESkipVecBias = TH_SKIP_IPE * uiLambda;
        iInterMBBias            = TH_INTER * (n - TH_INTER_QP);
        if (iInterMBBias < 0)
            iInterMBBias    = 0;
        if (iInterMBBias > TH_INTER_MAX_LEVEL)
            iInterMBBias    = TH_INTER_MAX_LEVEL;
        uSPESkipVecBias = TH_SKIP_SPE * uiLambda;
        iIntra16Bias = 0;
        //
        if (n & 1) {
            pnw_cmdbuf_insert_reg_write(MVEARegBase[i32Core], MVEA_CR_IPE_MV_BIAS_TABLE, uIPESkipVecBias);
            pnw_cmdbuf_insert_reg_write(MVEARegBase[i32Core], MVEA_CR_SPE_PRED_VECTOR_BIAS_TABLE, uIPESkipVecBias);
            pnw_cmdbuf_insert_reg_write(MVEARegBase[i32Core], MVEA_CR_SPE_INTRA16_BIAS_TABLE, iIntra16Bias);
            pnw_cmdbuf_insert_reg_write(MVEARegBase[i32Core], MVEA_CR_SPE_INTER_BIAS_TABLE, iInterMBBias);
        }
    }
    *pCount = count;
}

static void LoadH264Bias(
    pnw_cmdbuf_p cmdbuf, IMG_INT32 i32Core, IMG_UINT8 ui8THSkip, IMG_INT8 i8QpOff)
{
    IMG_INT8 n;
    IMG_INT8 iX;
    IMG_UINT32 ui32RegVal;
    IMG_UINT32 uIPESkipVecBias, iInterMBBias, uSPESkipVecBias, iIntra16Bias;
    IMG_UINT32 count = 0, cmd_word = 0;
    uint32_t *pCount;

    cmd_word = ((MTX_CMDID_SW_WRITEREG & MTX_CMDWORD_ID_MASK) << MTX_CMDWORD_ID_SHIFT) |
               ((i32Core & MTX_CMDWORD_CORE_MASK) <<  MTX_CMDWORD_CORE_SHIFT);
    *cmdbuf->cmd_idx++ = cmd_word;
    pCount = cmdbuf->cmd_idx;
    cmdbuf->cmd_idx++;


    IMG_BYTE PVR_QP_SCALE_CR[76] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
        12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
        28, 29, 29, 30, 31, 32, 32, 33, 34, 34, 35, 35, 36, 36, 37, 37,
        37, 38, 38, 38, 39, 39, 39, 39,
        39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
    };

    for (n = 51; n >= 0; n--) {

        iX = n - 12;
        if (iX < 0)
            iX = 0;

        // Dont Write QP Values To ESB -- IPE will write these values
        // Update the quantization parameter which includes doing Lamda and the Chroma QP
        ui32RegVal = PVR_QP_SCALE_CR[n + 12 + i8QpOff ];
        ui32RegVal |= ((H264_QPLAMBDA_MAP[iX] * 24) / 16) << 8;
        ui32RegVal |= (H264_QPLAMBDA_MAP[iX]) << 16;

#ifdef ZERO_BIAS
        pnw_cmdbuf_insert_reg_write(MVEARegBase[i32Core], MVEA_CR_IPE_LAMBDA_TABLE, (2 << 16) | F_ENCODE(3, MVEA_CR_IPE_ALPHA_OR_DC_SCALE_CHR_TABLE) | F_ENCODE(PVR_QP_SCALE_CR[n], MVEA_CR_IPE_QPC_OR_DC_SCALE_LUMA_TABLE));
#else
        pnw_cmdbuf_insert_reg_write(MVEARegBase[i32Core], MVEA_CR_IPE_LAMBDA_TABLE, ui32RegVal);
#endif

    }
    for (n = 52; n >= 0; n -= 2) {
        IMG_INT8 qp = n;
        if (qp > 51) qp = 51;

        uIPESkipVecBias = H264SkipBias(qp, ui8THSkip);
        iInterMBBias    = H264InterBias(qp);
        uSPESkipVecBias = H264SkipBias(qp, ui8THSkip);
        iIntra16Bias    = H264Intra4x4Bias(qp);

#ifdef ZERO_BIAS
        pnw_cmdbuf_insert_reg_write(MVEARegBase[i32Core], MVEA_CR_IPE_MV_BIAS_TABLE, 0);
        pnw_cmdbuf_insert_reg_write(MVEARegBase[i32Core], MVEA_CR_SPE_PRED_VECTOR_BIAS_TABLE, 0);
        pnw_cmdbuf_insert_reg_write(MVEARegBase[i32Core], MVEA_CR_SPE_INTRA16_BIAS_TABLE, 0);
        pnw_cmdbuf_insert_reg_write(MVEARegBase[i32Core], MVEA_CR_SPE_INTER_BIAS_TABLE, 0);
#else
        pnw_cmdbuf_insert_reg_write(MVEARegBase[i32Core], MVEA_CR_IPE_MV_BIAS_TABLE, uIPESkipVecBias);
        pnw_cmdbuf_insert_reg_write(MVEARegBase[i32Core], MVEA_CR_SPE_PRED_VECTOR_BIAS_TABLE, uIPESkipVecBias);
        pnw_cmdbuf_insert_reg_write(MVEARegBase[i32Core], MVEA_CR_SPE_INTRA16_BIAS_TABLE, iIntra16Bias);
        pnw_cmdbuf_insert_reg_write(MVEARegBase[i32Core], MVEA_CR_SPE_INTER_BIAS_TABLE, iInterMBBias);
#endif

    }

    pnw_cmdbuf_insert_reg_write(MVEARegBase[i32Core], MVEA_CR_SPE_ZERO_THRESH, 0);
    *pCount = count;
}


static VAStatus pnw__alloc_context_buffer(context_ENC_p ctx, unsigned char is_JPEG)
{
    int width, height;
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    /* width and height should be source surface's w and h or ?? */
    width = ctx->obj_context->picture_width;
    height = ctx->obj_context->picture_height;

    if (is_JPEG == 0) {
        ctx->pic_params_size  = 256;

        ctx->header_buffer_size = 8 * HEADER_SIZE + MAX_SLICES_PER_PICTURE * HEADER_SIZE;

        ctx->seq_header_ofs = 0;
        ctx->pic_header_ofs = HEADER_SIZE;
        ctx->eoseq_header_ofs = 2 * HEADER_SIZE;
        ctx->eostream_header_ofs = 3 * HEADER_SIZE;
        ctx->aud_header_ofs = 4 * HEADER_SIZE;
        ctx->sei_buf_prd_ofs = 5 * HEADER_SIZE;
        ctx->sei_pic_tm_ofs = 6 * HEADER_SIZE;
        ctx->sei_pic_fpa_ofs = 7 * HEADER_SIZE;
        ctx->slice_header_ofs = 8 * HEADER_SIZE;
        ctx->in_params_ofs = 0;

        ctx->sliceparam_buffer_size = ((sizeof(SLICE_PARAMS) + 15) & 0xfff0) * MAX_SLICES_PER_PICTURE;
        /* All frame share same MTX_CURRENT_IN_PARAMS and above/bellow param
         * create MTX_CURRENT_IN_PARAMS buffer seperately
         * every MB has one MTX_CURRENT_IN_PARAMS structure, and the (N+1) frame can
         * reuse (N) frame's structure
         */
        ctx->in_params_size = ((~0xf) & (15 + 1 + (width + 15) * (height + 15) / (16 * 16))) * sizeof(MTX_CURRENT_IN_PARAMS);
        ctx->below_params_size = ((BELOW_PARAMS_SIZE * width * height / (16 * 16)) + 0xf) & (~0xf);
        ctx->above_params_size = ((width / 16) * 128 + 15) & (~0xf) ;

        vaStatus = psb_buffer_create(ctx->obj_context->driver_data, ctx->in_params_size, psb_bt_cpu_vpu, &ctx->topaz_in_params_I);
        if (VA_STATUS_SUCCESS != vaStatus) {
            return vaStatus;
        }

        vaStatus = psb_buffer_create(ctx->obj_context->driver_data, ctx->in_params_size, psb_bt_cpu_vpu, &ctx->topaz_in_params_P);
        if (VA_STATUS_SUCCESS != vaStatus) {
            psb_buffer_destroy(&ctx->topaz_in_params_I);
            return vaStatus;
        }

        vaStatus = psb_buffer_create(ctx->obj_context->driver_data, ctx->below_params_size * 4, psb_bt_cpu_vpu, &ctx->topaz_below_params);
        if (VA_STATUS_SUCCESS != vaStatus) {
            psb_buffer_destroy(&ctx->topaz_in_params_P);
            psb_buffer_destroy(&ctx->topaz_in_params_I);
            return vaStatus;
        }

        vaStatus = psb_buffer_create(ctx->obj_context->driver_data, ctx->above_params_size * 4, psb_bt_cpu_vpu, &ctx->topaz_above_params);
        if (VA_STATUS_SUCCESS != vaStatus) {
            psb_buffer_destroy(&ctx->topaz_in_params_P);
            psb_buffer_destroy(&ctx->topaz_in_params_I);
            psb_buffer_destroy(&ctx->topaz_below_params);
            return vaStatus;
        }
        ctx->below_params_ofs = 0;
        ctx->above_params_ofs = 0;
    } else {
        /*JPEG encode need three kinds of buffer but doesn't need above/below buffers.*/
        ctx->pic_params_size = (sizeof(JPEG_MTX_QUANT_TABLE) + 0xf) & (~0xf);

        ctx->header_buffer_size = (sizeof(JPEG_MTX_DMA_SETUP) + 0xf) & (~0xf);
        ctx->sliceparam_buffer_size =  16; /* Not used*/
    }
    return vaStatus;
}

unsigned int pnw__get_ipe_control(enum drm_pnw_topaz_codec  eEncodingFormat)
{
    unsigned int RegVal = 0;

    RegVal = F_ENCODE(2, MVEA_CR_IPE_GRID_FINE_SEARCH) |
             F_ENCODE(0, MVEA_CR_IPE_GRID_SEARCH_SIZE) |
             F_ENCODE(1, MVEA_CR_IPE_Y_FINE_SEARCH);

    switch (eEncodingFormat) {
    case IMG_CODEC_H263_NO_RC:
    case IMG_CODEC_H263_VBR:
    case IMG_CODEC_H263_CBR:
        RegVal |= F_ENCODE(0, MVEA_CR_IPE_BLOCKSIZE) | F_ENCODE(0, MVEA_CR_IPE_ENCODING_FORMAT);
        break;
    case IMG_CODEC_MPEG4_NO_RC:
    case IMG_CODEC_MPEG4_VBR:
    case IMG_CODEC_MPEG4_CBR:
        RegVal |= F_ENCODE(1, MVEA_CR_IPE_BLOCKSIZE) | F_ENCODE(1, MVEA_CR_IPE_ENCODING_FORMAT);
    default:
        break;
    case IMG_CODEC_H264_NO_RC:
    case IMG_CODEC_H264_VBR:
    case IMG_CODEC_H264_CBR:
    case IMG_CODEC_H264_VCM:
        RegVal |= F_ENCODE(2, MVEA_CR_IPE_BLOCKSIZE) | F_ENCODE(2, MVEA_CR_IPE_ENCODING_FORMAT);
        break;
    }
    RegVal |= F_ENCODE(6, MVEA_CR_IPE_Y_CANDIDATE_NUM);
    return RegVal;
}


void pnw_DestroyContext(object_context_p obj_context)
{
    context_ENC_p ctx;
    ctx = (context_ENC_p)obj_context->format_data;

    psb_buffer_destroy(&ctx->topaz_in_params_P);
    psb_buffer_destroy(&ctx->topaz_in_params_I);
    psb_buffer_destroy(&ctx->topaz_below_params);
    psb_buffer_destroy(&ctx->topaz_above_params);

    if (NULL != ctx->slice_param_cache)
        free(ctx->slice_param_cache);
    if (NULL == ctx->save_seq_header_p)
        free(ctx->save_seq_header_p);
    free(obj_context->format_data);
    obj_context->format_data = NULL;
}

VAStatus pnw_CreateContext(
    object_context_p obj_context,
    object_config_p __maybe_unused obj_config,
    unsigned char is_JPEG)
{
    int width, height;
    int i;
    unsigned short SearchWidth, SearchHeight, SearchLeftOffset, SearchTopOffset;
    context_ENC_p ctx;
    VAStatus vaStatus;

    width = obj_context->picture_width;
    height = obj_context->picture_height;
    ctx = (context_ENC_p) calloc(1, sizeof(struct context_ENC_s));
    CHECK_ALLOCATION(ctx);

    obj_context->format_data = (void*) ctx;
    ctx->obj_context = obj_context;

    ctx->RawWidth = (unsigned short) width;
    ctx->RawHeight = (unsigned short) height;

    if (is_JPEG == 0) {
        ctx->Width = (unsigned short)(~0xf & (width + 0xf));
        ctx->Height = (unsigned short)(~0xf & (height + 0xf));
    } else {
        /*JPEG only require them are even*/
        ctx->Width = (unsigned short)(~0x1 & (width + 0x1));
        ctx->Height = (unsigned short)(~0x1 & (height + 0x1));
    }

    /* pre-calculated values based on other stream properties */
    SearchHeight = min(MVEA_LRB_SEARCH_HEIGHT, ctx->Height);
    SearchWidth = min(MVEA_LRB_SEARCH_WIDTH, ctx->Width);
    SearchLeftOffset = (((SearchWidth / 2) / 16) * 16);
    SearchTopOffset = (((SearchHeight / 2) / 16) * 16);

    ctx->HeightMinus16MinusLRBTopOffset        = ctx->Height - (SearchTopOffset + 16);
    ctx->HeightMinus32MinusLRBTopOffset        = ctx->Height - (SearchTopOffset + 32);
    ctx->HeightMinusLRB_TopAndBottom_OffsetsPlus16     = ctx->Height - (SearchTopOffset + SearchTopOffset + 16);
    ctx->HeightMinusLRBSearchHeight    = ctx->Height - SearchHeight;

    ctx->FCode = 0;

    ctx->NumCores = 2; /* FIXME Assume there is two encode cores in Penwell, precise value should read from HW */

    ctx->BelowParamsBufIdx = 0;
    ctx->AccessUnitNum = 0;
    ctx->SyncSequencer = 0; /* FIXME Refer to DDK */
    ctx->SliceToCore =  -1;
    ctx->CmdCount = 0xa5a5a5a5 % MAX_TOPAZ_CMD_COUNT;
    ctx->FrmIdx = 0;

    for (i = 0; i < MAX_TOPAZ_CORES; i++) {
        ctx->LastSliceNum[i] = -1;
        ctx->LastSync[0][i] = ~0;
        ctx->LastSync[1][i] = ~0;
    }

    for (i = 0; i < MAX_SLICES_PER_PICTURE; i++) {
        ctx->SliceHeaderReady[i] = IMG_FALSE;
    }

    vaStatus = pnw__alloc_context_buffer(ctx, is_JPEG);

    return vaStatus;
}

VAStatus pnw_BeginPicture(context_ENC_p ctx)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    pnw_cmdbuf_p cmdbuf;
    int ret, i;

    if (ctx->raw_frame_count != 0)
        ctx->previous_src_surface = ctx->src_surface;
    ctx->src_surface = ctx->obj_context->current_render_target;

    /* clear frameskip flag to 0 */
    CLEAR_SURFACE_INFO_skipped_flag(ctx->src_surface->psb_surface);
    ctx->none_vcl_nal = 0;

    /*if (ctx->sRCParams.RCEnable == IMG_TRUE)
     {
     pnw_DetectFrameSkip(ctx);
     if (0 != (GET_SURFACE_INFO_skipped_flag(ctx->src_surface->psb_surface)
            & SURFACE_INFO_SKIP_FLAG_SETTLED))
         ctx->sRCParams.FrameSkip = IMG_TRUE;
     else
         ctx->sRCParams.FrameSkip = IMG_FALSE;
     }*/

    /* Initialise the command buffer */
    ret = pnw_context_get_next_cmdbuf(ctx->obj_context);
    if (ret) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "get next cmdbuf fail\n");
        vaStatus = VA_STATUS_ERROR_UNKNOWN;
        return vaStatus;
    }
    cmdbuf = ctx->obj_context->pnw_cmdbuf;
    memset(cmdbuf->cmd_idx_saved, 0, sizeof(cmdbuf->cmd_idx_saved));


    /* map start_pic param */
    vaStatus = psb_buffer_map(&cmdbuf->pic_params, &cmdbuf->pic_params_p);
    if (vaStatus) {
        return vaStatus;
    }
    vaStatus = psb_buffer_map(&cmdbuf->header_mem, &cmdbuf->header_mem_p);
    if (vaStatus) {
        psb_buffer_unmap(&cmdbuf->pic_params);
        return vaStatus;
    }

    vaStatus = psb_buffer_map(&cmdbuf->slice_params, &cmdbuf->slice_params_p);
    if (vaStatus) {
        psb_buffer_unmap(&cmdbuf->pic_params);
        psb_buffer_unmap(&cmdbuf->header_mem);
        return vaStatus;
    }

    /* only map topaz param when necessary */
    cmdbuf->topaz_above_params_p = NULL;
    cmdbuf->topaz_below_params_p = NULL;
    cmdbuf->topaz_in_params_I_p = NULL;
    cmdbuf->topaz_in_params_P_p = NULL;

    if (ctx->obj_context->frame_count == 0) { /* first picture */

        psb_driver_data_p driver_data = ctx->obj_context->driver_data;

        *cmdbuf->cmd_idx++ = ((MTX_CMDID_SW_NEW_CODEC & MTX_CMDWORD_ID_MASK) << MTX_CMDWORD_ID_SHIFT) |
                             (((driver_data->drm_context & MTX_CMDWORD_COUNT_MASK) << MTX_CMDWORD_COUNT_SHIFT));
        pnw_cmdbuf_insert_command_param(ctx->eCodec);
        pnw_cmdbuf_insert_command_param((ctx->Width << 16) | ctx->Height);
    }

    ctx->FrmIdx++;
    ctx->SliceToCore = ctx->ParallelCores - 1;
    /* ctx->AccessUnitNum++;    Move this back to pnw_EndPicture */
    ctx->sRCParams.bBitrateChanged = IMG_FALSE;

    for (i = 0; i < MAX_TOPAZ_CORES; i++)
        ctx->LastSliceNum[i] = -1;

    for (i = 0; i < MAX_SLICES_PER_PICTURE; i++)
        ctx->SliceHeaderReady[i] = IMG_FALSE;

    /*If ParallelCores > 1(H264) and encode one slice per frame, the unnecessary start picture
    *commands will be replaced with MTX_CMDID_PAD and ignored by kernel*/
    cmdbuf->cmd_idx_saved[PNW_CMDBUF_START_PIC_IDX] = cmdbuf->cmd_idx;

    /* insert START_PIC command for each core */
    /* ensure that the master (core #0) will be last to complete this batch */
    for (i = (ctx->ParallelCores - 1); i >= 0; i--) {

        /*
         * the content of PIC_PARAMS is filled when RenderPicture(...,VAEncPictureParameterBufferXXX)
         */
        pnw_cmdbuf_insert_command_package(ctx->obj_context,
                                          i,
                                          MTX_CMDID_START_PIC,
                                          &cmdbuf->pic_params,
                                          i * ctx->pic_params_size);

        /* no RC paramter provided in vaBeginPicture
         * so delay RC param setup into vaRenderPicture(SequenceHeader...)
         */
    }

    ctx->obj_context->slice_count = 0;
    return 0;
}


VAStatus pnw_set_bias(context_ENC_p ctx, int core)
{
    pnw_cmdbuf_p cmdbuf = (pnw_cmdbuf_p)ctx->obj_context->pnw_cmdbuf;
    double flBpp;
    IMG_UINT8 THSkip;

    if (ctx->sRCParams.RCEnable) {
        flBpp = 1.0 * ctx->sRCParams.BitsPerSecond /
                (ctx->sRCParams.FrameRate * ctx->Width * ctx->Height);
    } else {
        flBpp =  0.14;
    }

    if (flBpp <= 0.07) {
        THSkip = TH_SKIP_24;
    } else if (flBpp <= 0.14) {
        THSkip = TH_SKIP_12;
    } else {
        THSkip = TH_SKIP_0;
    }
    switch (ctx->eCodec) {
    case IMG_CODEC_H264_VBR:
    case IMG_CODEC_H264_CBR:
    case IMG_CODEC_H264_VCM:
    case IMG_CODEC_H264_NO_RC:
        LoadH264Bias(cmdbuf, core, THSkip, ctx->sRCParams.QCPOffset);
        break;
    case IMG_CODEC_H263_CBR:
    case IMG_CODEC_H263_NO_RC:
    case IMG_CODEC_H263_VBR:
        LoadH263Bias(cmdbuf, core, THSkip);
        break;
    case IMG_CODEC_MPEG4_NO_RC:
    case IMG_CODEC_MPEG4_CBR:
    case IMG_CODEC_MPEG4_VBR:
        LoadMPEG4Bias(cmdbuf, core, THSkip);
        break;
    default:
        return -1;
        break;
    }
    return 0;
}

VAStatus pnw_RenderPictureParameter(context_ENC_p ctx, int core)
{
    PIC_PARAMS *psPicParams;    /* PIC_PARAMS has been put in pnw_hostcode.h */
    object_surface_p src_surface;
    unsigned int srf_buf_offset;
    object_surface_p rec_surface;
    object_surface_p ref_surface;
    pnw_cmdbuf_p cmdbuf = ctx->obj_context->pnw_cmdbuf;
    VAStatus vaStatus = VA_STATUS_ERROR_UNKNOWN;

    psPicParams = (PIC_PARAMS *)(cmdbuf->pic_params_p + ctx->pic_params_size * core);

    memset(psPicParams, 0, sizeof(PIC_PARAMS));
    /* second frame will reuse some rate control parameters (IN_PARAMS_MP4)
     * so only memset picture parames except IN_PARAMS
     * BUT now IN_RC_PARAMS was reload from the cache, so it now can
     * memset entirE PIC_PARAMS
     */

    /*
    memset(psPicParams, 0, (int)((unsigned char *)&psPicParams->sInParams - (unsigned char *)psPicParams));
    */

    src_surface = ctx->src_surface;
    CHECK_SURFACE(src_surface);

    rec_surface = ctx->dest_surface;
    CHECK_SURFACE(rec_surface);

    /*The fisrt frame always is I frame and the content of reference frame wouldn't be used.
     * But the heights of ref and dest frame should be the same.
     * That allows Topaz to keep its motion vectors up to date, which helps maintain performance */
    if (ctx->obj_context->frame_count == 0)
        ctx->ref_surface = ctx->dest_surface;

    ref_surface = ctx->ref_surface;
    CHECK_SURFACE(rec_surface);

    /* clear frameskip flag */
    CLEAR_SURFACE_INFO_skipped_flag(rec_surface->psb_surface);
    CLEAR_SURFACE_INFO_skipped_flag(ref_surface->psb_surface);

    /* Write video data byte offset into Coded buffer
     * If it is here, it will be a SYNC point, which have performance impact
     * Move to psb__CreateBuffer
     vaStatus = psb_buffer_map(ctx->coded_buf->psb_buffer, &pBuffer);
     if(vaStatus) {
     DEBUG_FAILURE;
     return vaStatus;
     }
     *(IMG_UINT32 *)(pBuffer+8) = 16;
     psb_buffer_unmap(ctx->coded_buf->psb_buffer);
    */

    psPicParams->SrcYStride = src_surface->psb_surface->stride;
    switch (ctx->eFormat) {
    case IMG_CODEC_IYUV:        /* IYUV */
    case IMG_CODEC_PL8:
        psPicParams->SrcUVStride = src_surface->psb_surface->stride / 2;
        psPicParams->SrcUVRowStride = src_surface->psb_surface->stride * 8 / 2;
        break;
    case IMG_CODEC_IMC2:    /* IMC2 */
    case IMG_CODEC_PL12:
        psPicParams->SrcUVStride = src_surface->psb_surface->stride;
        psPicParams->SrcUVRowStride = src_surface->psb_surface->stride * 8;
        break;
    default:
        break;
    }
    psPicParams->SrcYRowStride    = src_surface->psb_surface->stride * 16;
    /* psPicParams->SrcUVRowStride = src_surface->psb_surface->stride * 16 / 2; */

    /* Dest(rec) stride
     * The are now literally Dst stride (not equivalent to 'offset to next row')
     */
#ifdef VA_EMULATOR
    /* only for simulator, va-emulator needs the actually stride for
     * reconstructed frame transfer (va-emulator->driver)
     */
    psPicParams->DstYStride = rec_surface->psb_surface->stride;
    psPicParams->DstUVStride = rec_surface->psb_surface->stride;
#else
    psPicParams->DstYStride = rec_surface->psb_surface->stride;
    psPicParams->DstUVStride = rec_surface->psb_surface->stride;
#endif

    psPicParams->Width  = ctx->Width;
    psPicParams->Height = ctx->Height;
    psPicParams->NumSlices = ctx->sRCParams.Slices;

    psPicParams->IsPerSliceOutput = IMG_FALSE;
    psPicParams->SearchHeight = min(MVEA_LRB_SEARCH_HEIGHT, psPicParams->Height);
    psPicParams->SearchWidth = min(MVEA_LRB_SEARCH_WIDTH, psPicParams->Width);

    /* not sure why we are setting this up here... */
    psPicParams->Flags = 0;
    switch (ctx->eCodec) {
    case IMG_CODEC_H264_NO_RC:
    case IMG_CODEC_H264_VBR:
    case IMG_CODEC_H264_CBR:
    case IMG_CODEC_H264_VCM:
        psPicParams->Flags |= ISH264_FLAGS;
        break;
    case IMG_CODEC_H263_VBR:
    case IMG_CODEC_H263_CBR:
    case IMG_CODEC_H263_NO_RC:
        psPicParams->Flags |= ISH263_FLAGS;
        break;
    case IMG_CODEC_MPEG4_NO_RC:
    case IMG_CODEC_MPEG4_VBR:
    case IMG_CODEC_MPEG4_CBR:
        psPicParams->Flags |= ISMPEG4_FLAGS;
        break;
    default:
        return VA_STATUS_ERROR_UNKNOWN;
    }

    switch (ctx->eCodec) {
    case IMG_CODEC_H264_VBR:
    case IMG_CODEC_MPEG4_VBR:
    case IMG_CODEC_H263_VBR:
        psPicParams->Flags |= ISVBR_FLAGS;
        break;
    case IMG_CODEC_H264_VCM:
        psPicParams->Flags |= (ISVCM_FLAGS | ISCBR_FLAGS);
        break;
    case IMG_CODEC_H263_CBR:
    case IMG_CODEC_H264_CBR:
    case IMG_CODEC_MPEG4_CBR:
        psPicParams->Flags |= ISCBR_FLAGS;
        break;
    case IMG_CODEC_MPEG4_NO_RC:
    case IMG_CODEC_H263_NO_RC:
    case IMG_CODEC_H264_NO_RC:
        break;
    default:
        return VA_STATUS_ERROR_UNKNOWN;
    }

#if 0
    if (ctx->SyncSequencer)
        psPicParams->Flags |= SYNC_SEQUENCER;
#endif

    if (ctx->sRCParams.RCEnable) {
        if (ctx->sRCParams.bDisableFrameSkipping) {
            psPicParams->Flags |= DISABLE_FRAME_SKIPPING;
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "Frame skip is disabled.\n");
        }

        if (ctx->sRCParams.bDisableBitStuffing && IS_H264_ENC(ctx->eCodec)) {
            psPicParams->Flags |= DISABLE_BIT_STUFFING;
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "Bit stuffing is disabled.\n");
        }


        /* for the first frame, will setup RC params in EndPicture */
        if (ctx->raw_frame_count > 0) { /* reuse in_params parameter */
            /* In case, it's changed in a new sequence */
            if (ctx->obj_context->frame_count == 0
               && ctx->in_params_cache.IntraPeriod != ctx->sRCParams.IntraFreq) {
               drv_debug_msg(VIDEO_DEBUG_ERROR,
               "On frame %d, Intra period is changed from %d to %d\n",
               ctx->raw_frame_count, ctx->in_params_cache.IntraPeriod,
               ctx->sRCParams.IntraFreq);
               ctx->in_params_cache.IntraPeriod =  ctx->sRCParams.IntraFreq;
               ctx->in_params_cache.BitsPerGOP =
                    (ctx->sRCParams.BitsPerSecond / ctx->sRCParams.FrameRate)
                    * ctx->sRCParams.IntraFreq;
            }

            psPicParams->Flags &= ~FIRST_FRAME;
            /* reload IN_RC_PARAMS from cache */
            memcpy(&psPicParams->sInParams, &ctx->in_params_cache, sizeof(IN_RC_PARAMS));
        } else {
            psPicParams->Flags |= ISRC_FLAGS;
            psPicParams->Flags |= FIRST_FRAME;
        }
    } else
        psPicParams->sInParams.SeInitQP = ctx->sRCParams.InitialQp;

    /* some relocations have to been done here */
    srf_buf_offset = src_surface->psb_surface->buf.buffer_ofs;
    if (src_surface->psb_surface->buf.type == psb_bt_camera)
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "src surface GPU offset 0x%08x, luma offset 0x%08x\n",
                                 wsbmBOOffsetHint(src_surface->psb_surface->buf.drm_buf), srf_buf_offset);

    RELOC_PIC_PARAMS_PNW(&psPicParams->SrcYBase, srf_buf_offset, &src_surface->psb_surface->buf);
    switch (ctx->eFormat) {
    case IMG_CODEC_IYUV:
    case IMG_CODEC_PL8:
    case IMG_CODEC_PL12:
        RELOC_PIC_PARAMS_PNW(&psPicParams->SrcUBase,
                             srf_buf_offset + src_surface->psb_surface->chroma_offset,
                             &src_surface->psb_surface->buf);

        RELOC_PIC_PARAMS_PNW(&psPicParams->SrcVBase,
                             srf_buf_offset + src_surface->psb_surface->chroma_offset * 5 / 4,
                             &src_surface->psb_surface->buf);

        break;
    case IMG_CODEC_IMC2:
    case IMG_CODEC_NV12:
    default:
        break;
    }

    RELOC_PIC_PARAMS_PNW(&psPicParams->DstYBase, 0, &rec_surface->psb_surface->buf);

    RELOC_PIC_PARAMS_PNW(&psPicParams->DstUVBase,
                         rec_surface->psb_surface->stride * rec_surface->height,
                         &rec_surface->psb_surface->buf);

    RELOC_PIC_PARAMS_PNW(&psPicParams->BelowParamsInBase,
                         ctx->below_params_ofs + ctx->below_params_size *(((ctx->AccessUnitNum) & 0x1)),
                         cmdbuf->topaz_below_params);

    RELOC_PIC_PARAMS_PNW(&psPicParams->BelowParamsOutBase,
                         ctx->below_params_ofs + ctx->below_params_size *(((ctx->AccessUnitNum + 1) & 0x1)),
                         cmdbuf->topaz_below_params);

    RELOC_PIC_PARAMS_PNW(&psPicParams->AboveParamsBase,
                         ctx->above_params_ofs + ctx->above_params_size *(core * 2 + (ctx->AccessUnitNum & 0x1)),
                         cmdbuf->topaz_above_params);

    RELOC_PIC_PARAMS_PNW(&psPicParams->CodedBase, ctx->coded_buf_per_slice * core, ctx->coded_buf->psb_buffer);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "For core %d, above_parmas_off %x\n", core, ctx->above_params_ofs + ctx->above_params_size *(core * 2 + ((ctx->AccessUnitNum) & 0x1)));

   return VA_STATUS_SUCCESS;
}

static VAStatus pnw_SetupRCParam(context_ENC_p ctx)
{
    pnw_cmdbuf_p cmdbuf = ctx->obj_context->pnw_cmdbuf;
    PIC_PARAMS  *psPicParams = (PIC_PARAMS  *)cmdbuf->pic_params_p;
    PIC_PARAMS  *psPicParamsTmp;
    int origin_qp, i;/* in DDK setup_rc will change qp strangly,
                   * just for keep same with DDK
                   */

    origin_qp = ctx->sRCParams.InitialQp;

    psPicParams->Flags |= ISRC_FLAGS;
    pnw__setup_rcdata(ctx, psPicParams, &ctx->sRCParams);

    /* restore it, just keep same with DDK */
    ctx->sRCParams.InitialQp = origin_qp;

    /* Assume IN_RC_PARAMS for each core is identical, and copy for each */
    for (i = (ctx->ParallelCores - 1); i > 0; i--) {
        psPicParamsTmp = (PIC_PARAMS  *)(cmdbuf->pic_params_p + ctx->pic_params_size * i);
        memcpy((unsigned char *)&psPicParamsTmp->sInParams,
               (unsigned char *)&psPicParams->sInParams,
               sizeof(IN_RC_PARAMS));
        psPicParamsTmp->Flags |= psPicParams->Flags;
    }

    /* save IN_RC_PARAMS into the cache */
    memcpy(&ctx->in_params_cache, (unsigned char *)&psPicParams->sInParams, sizeof(IN_RC_PARAMS));
    return VA_STATUS_SUCCESS;
}

VAStatus pnw_EndPicture(context_ENC_p ctx)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    unsigned int i;
    int index;
    pnw_cmdbuf_p cmdbuf = ctx->obj_context->pnw_cmdbuf;
    PIC_PARAMS *psPicParams = (PIC_PARAMS *)cmdbuf->pic_params_p;
    PIC_PARAMS *psPicParamsSlave = NULL;
    unsigned int val = 0;

    ctx->AccessUnitNum++;

    if (ctx->sRCParams.RCEnable) {
        if (ctx->raw_frame_count == 0)
            pnw_SetupRCParam(ctx);
        else  if (ctx->sRCParams.bBitrateChanged) {
            /* Toggle the last bit to make sure encoder firmare recalculate the
               RC params even if the target bitrate isn't changed.*/
            val = ~(ctx->sRCParams.BitsPerSecond & 0x1);
            ctx->sRCParams.BitsPerSecond &= ~1;
            ctx->sRCParams.BitsPerSecond |= (val & 1);

            drv_debug_msg(VIDEO_DEBUG_GENERAL, "bitrate is changed to %d, "
                    "update the rc data accordingly\n", ctx->sRCParams.BitsPerSecond);
            pnw__update_rcdata(ctx, psPicParams, &ctx->sRCParams);
            if (ctx->sRCParams.MinQP)
                psPicParams->sInParams.MinQPVal = ctx->sRCParams.MinQP;
            memcpy(&ctx->in_params_cache, (unsigned char *)&psPicParams->sInParams, sizeof(IN_RC_PARAMS));
            /* Save rate control info in slave core as well */
            for (i = 1; i < ctx->ParallelCores; i++) {
                psPicParamsSlave = (PIC_PARAMS *)(cmdbuf->pic_params_p + ctx->pic_params_size * i);
                memcpy((unsigned char *)&psPicParamsSlave->sInParams,
                        (unsigned char *)&psPicParams->sInParams, sizeof(IN_RC_PARAMS));
	    }
        }
    }

#if TOPAZ_PIC_PARAMS_VERBOSE
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "End Picture for frame %d\n", ctx->raw_frame_count);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "sizeof PIC_PARAMS %d\n", sizeof(PIC_PARAMS));
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "sizeof in_params %d\n", sizeof(psPicParams->sInParams));
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "PicParams->SrcYBase  0x%08x\n", psPicParams->SrcYBase);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "PicParams->SrcUBase 0x%08x\n", psPicParams->SrcUBase);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "PicParams->SrcVBase 0x%08x\n", psPicParams->SrcVBase);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "PicParams->DstYBase 0x%08x\n", psPicParams->DstYBase);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "PicParams->DstUVBase 0x%08x\n", psPicParams->DstUVBase);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "PicParams->SrcYStride 0x%08x\n", psPicParams->SrcYStride);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "PicParams->SrcUVStride 0x%08x\n", psPicParams->SrcUVStride);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "PicParams->SrcYRowStride 0x%08x\n", psPicParams->SrcYRowStride);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "PicParams->SrcUVRowStride 0x%08x\n", psPicParams->SrcUVRowStride);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "PicParams->DstYStride 0x%08x\n", psPicParams->DstYStride);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "PicParams->DstUVStride 0x%08x\n", psPicParams->DstUVStride);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "PicParams->CodedBase 0x%08x\n", psPicParams->CodedBase);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "PicParams->BelowParamsInBase 0x%08x\n", psPicParams->BelowParamsInBase);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "PicParams->BelowParamsOutBase 0x%08x\n", psPicParams->BelowParamsOutBase);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "PicParams->AboveParamsBase 0x%08x\n", psPicParams->AboveParamsBase);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "PicParams->Width 0x%08x\n", psPicParams->Width);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "PicParams->Height 0x%08x\n", psPicParams->Height);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "PicParams->Flags 0x%08x\n", psPicParams->Flags);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "PicParams->SerachWidth 0x%08x\n", psPicParams->SearchWidth);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "PicParams->SearchHeight 0x%08x\n", psPicParams->SearchHeight);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "PicParams->NumSlices 0x%08x\n", psPicParams->NumSlices);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psPicParams->ClockDivBitrate %lld\n", psPicParams->ClockDivBitrate);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psPicParams->MaxBufferMultClockDivBitrate %d\n",
                             psPicParams->MaxBufferMultClockDivBitrate);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psPicParams->sInParams.SeInitQP %d\n", psPicParams->sInParams.SeInitQP);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psPicParams->sInParams.MinQPVal %d\n", psPicParams->sInParams.MinQPVal);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psPicParams->sInParams.MaxQPVal %d\n", psPicParams->sInParams.MaxQPVal);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psPicParams->sInParams.MBPerRow %d\n", psPicParams->sInParams.MBPerRow);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psPicParams->sInParams.MBPerFrm %d\n", psPicParams->sInParams.MBPerFrm);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psPicParams->sInParams.MBPerBU %d\n", psPicParams->sInParams.MBPerBU);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psPicParams->sInParams.BUPerFrm %d\n", psPicParams->sInParams.BUPerFrm);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psPicParams->sInParams.IntraPeriod %d\n", psPicParams->sInParams.IntraPeriod);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psPicParams->sInParams.BitsPerFrm %d\n", psPicParams->sInParams.BitsPerFrm);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psPicParams->sInParams.BitsPerBU %d\n", psPicParams->sInParams.BitsPerBU);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psPicParams->sInParams.BitsPerMB %d\n", psPicParams->sInParams.BitsPerMB);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psPicParams->sInParams.BitRate %d\n", psPicParams->sInParams.BitRate);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psPicParams->sInParams.BufferSize %d\n", psPicParams->sInParams.BufferSize);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psPicParams->sInParams.InitialLevel %d\n", psPicParams->sInParams.InitialLevel);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psPicParams->sInParams.InitialDelay %d\n", psPicParams->sInParams.InitialDelay);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psPicParams->sInParams.ScaleFactor %d\n", psPicParams->sInParams.ScaleFactor);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psPicParams->sInParams.VCMBitrateMargin %d\n", psPicParams->sInParams.VCMBitrateMargin);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psPicParams->sInParams.HalfFrameRate %d\n", psPicParams->sInParams.HalfFrameRate);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psPicParams->sInParams.FCode %d\n", psPicParams->sInParams.FCode);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psPicParams->sInParams.BitsPerGOP %d\n", psPicParams->sInParams.BitsPerGOP);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psPicParams->sInParams.AvQPVal %d\n", psPicParams->sInParams.AvQPVal);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psPicParams->sInParams.MyInitQP %d\n", psPicParams->sInParams.MyInitQP);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psPicParams->sInParams.ForeceSkipMargin %d\n", psPicParams->sInParams.ForeceSkipMargin);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psPicParams->sInParams.RCScaleFactor %d\n", psPicParams->sInParams.RCScaleFactor);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psPicParams->sInParams.TransferRate %d\n", psPicParams->sInParams.TransferRate);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psPicParams->sInParams.MaxFrameSize %d\n", psPicParams->sInParams.MaxFrameSize);
#endif
    /* save current settings */
    ctx->previous_ref_surface = ctx->ref_surface;
    ctx->previous_dest_surface = ctx->dest_surface; /* reconstructed surface */
    SET_CODEDBUF_INFO(SLICE_NUM, ctx->coded_buf->codedbuf_aux_info,
        ctx->obj_context->slice_count);
    SET_CODEDBUF_INFO(NONE_VCL_NUM, ctx->coded_buf->codedbuf_aux_info,
        ctx->none_vcl_nal);

    for (index = (ctx->ParallelCores - 1); index >= 0; index--) {
        pnw_cmdbuf_insert_command_package(ctx->obj_context,
                                          index,
                                          MTX_CMDID_END_PIC,
                                          NULL,
                                          0);
    }
    psb_buffer_unmap(&cmdbuf->pic_params);
    psb_buffer_unmap(&cmdbuf->header_mem);
    psb_buffer_unmap(&cmdbuf->slice_params);

    /* unmap MTX_CURRENT_IN_PARAMS buffer only when it is mapped */
    if (cmdbuf->topaz_in_params_I_p != NULL) {
        psb_buffer_unmap(cmdbuf->topaz_in_params_I);
        cmdbuf->topaz_in_params_I_p = NULL;
    }

    if (cmdbuf->topaz_in_params_P_p != NULL) {
        psb_buffer_unmap(cmdbuf->topaz_in_params_P);
        cmdbuf->topaz_in_params_P_p = NULL;
    }

    if (cmdbuf->topaz_above_params_p != NULL) {
        psb_buffer_unmap(cmdbuf->topaz_above_params);
        cmdbuf->topaz_above_params_p = NULL;
    }

    if (cmdbuf->topaz_below_params_p != NULL) {
        psb_buffer_unmap(cmdbuf->topaz_below_params);
        cmdbuf->topaz_below_params_p = NULL;
    }

    if (pnw_context_flush_cmdbuf(ctx->obj_context)) {
        vaStatus = VA_STATUS_ERROR_UNKNOWN;
    }
    ctx->raw_frame_count++;
    return vaStatus;
}

static void pnw__setup_busize(context_ENC_p ctx)
{
    unsigned int old_busize = ctx->sRCParams.BUSize;
    int slices = ctx->obj_context->slice_count;

    /* it is called at EndPicture, we should now the Slice number */
    //ctx->Slices = ctx->obj_context->slice_count;

    /* if no BU size is given then pick one ourselves */
    if (ctx->sRCParams.BUSize != 0)  { /* application provided BUSize */
        IMG_UINT32 MBs, MBsperSlice, MBsLastSlice;
        IMG_UINT32 BUs;
        IMG_INT32  SliceHeight;
        IMG_UINT32 MaxSlicesPerPipe, MaxMBsPerPipe, MaxBUsPerPipe;

        MBs     = ctx->Height * ctx->Width / (16 * 16);

        SliceHeight     = ctx->Height / slices;
        /* SliceHeight += 15; */
        SliceHeight &= ~15;

        MBsperSlice     = (SliceHeight * ctx->Width) / (16 * 16);
        MBsLastSlice    = MBs - (MBsperSlice * (slices - 1));

        /* they have given us a basic unit so validate it */
        if (ctx->sRCParams.BUSize < 6) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "ERROR: Basic unit size too small, must be greater than 6\n");
            ctx->sRCParams.BUSize = 0; /* need repatch */;
        }
        if (ctx->sRCParams.BUSize > MBsperSlice) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "ERROR: Basic unit size too large, must be less than the number of macroblocks in a slice\n");
            ctx->sRCParams.BUSize = 0; /* need repatch */;
        }
        if (ctx->sRCParams.BUSize > MBsLastSlice) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "ERROR: Basic unit size too large, must be less than number of macroblocks in the last slice\n");
            ctx->sRCParams.BUSize = 0; /* need repatch */;
        }

        if (ctx->sRCParams.BUSize != 0) {
            BUs = MBsperSlice / ctx->sRCParams.BUSize;
            if ((BUs * ctx->sRCParams.BUSize) != MBsperSlice)   {
                drv_debug_msg(VIDEO_DEBUG_ERROR, "ERROR: Basic unit size not an integer divisor of MB's in a slice");
                ctx->sRCParams.BUSize = 0; /* need repatch */;
            }
        }
        if (ctx->sRCParams.BUSize != 0) {
            BUs = MBsLastSlice / ctx->sRCParams.BUSize;
            if ((BUs * ctx->sRCParams.BUSize) != MBsLastSlice)   {
                drv_debug_msg(VIDEO_DEBUG_ERROR, "ERROR: Basic unit size not an integer divisor of MB's in a slice");
                ctx->sRCParams.BUSize = 0; /* need repatch */;
            }
        }

        if (ctx->sRCParams.BUSize != 0) {
            // check if the number of BUs per pipe is greater than 200
            MaxSlicesPerPipe = (slices + ctx->ParallelCores - 1) / ctx->ParallelCores;
            MaxMBsPerPipe = (MBsperSlice * (MaxSlicesPerPipe - 1)) + MBsLastSlice;
            MaxBUsPerPipe = (MaxMBsPerPipe + ctx->sRCParams.BUSize - 1) / ctx->sRCParams.BUSize;
            if (MaxBUsPerPipe > 200) {
                drv_debug_msg(VIDEO_DEBUG_ERROR, "ERROR: Basic unit size too small. There must be less than 200 basic units per slice");
                ctx->sRCParams.BUSize = 0; /* need repatch */;
            }
        }
    }

    if (ctx->sRCParams.BUSize == 0)  {
        IMG_UINT32 MBs, MBsperSlice, MBsLastSlice;
        IMG_UINT32 BUsperSlice, BUsLastSlice;
        IMG_UINT32 MaxSlicesPerPipe, MaxMBsPerPipe, MaxBUsPerPipe;
        IMG_INT32  SliceHeight;
        IMG_UINT32 BUSize = 6;

        MBs     = ctx->Height * ctx->Width / (16 * 16);

        SliceHeight     = ctx->Height / slices;
        /* SliceHeight += 15; */
        SliceHeight &= ~15;

        MBsperSlice     = (SliceHeight * ctx->Width) / (16 * 16);
        MBsLastSlice = MBs - (MBsperSlice * (slices - 1));

        /* Check number of BUs to be encoded on one pipe is less than maximum number allowed 200  */
        MaxSlicesPerPipe = (slices + ctx->ParallelCores - 1) / ctx->ParallelCores;
        MaxMBsPerPipe = (MBsperSlice * (MaxSlicesPerPipe - 1)) + MBsLastSlice;
        MaxBUsPerPipe = (MaxMBsPerPipe + BUSize - 1) / BUSize;

        while (MaxBUsPerPipe > 200)  {
            BUSize++;
            MaxBUsPerPipe = (MaxMBsPerPipe + BUSize - 1) / BUSize;
        }

        /* Check whether there are integeral number of BUs in the slices  */
        BUsperSlice = MBsperSlice / BUSize;
        BUsLastSlice = MBsLastSlice / BUSize;
        while ((BUsperSlice*BUSize != MBsperSlice) ||
               (BUsLastSlice*BUSize != MBsLastSlice)) {
            BUSize++;
            BUsperSlice = MBsperSlice / BUSize;
            BUsLastSlice = MBsLastSlice / BUSize;
        }

        ctx->sRCParams.BUSize = BUSize;
    /*
        ctx->sRCParams.InitialLevel = (3 * ctx->sRCParams.BufferSize) >> 4;
        ctx->sRCParams.InitialDelay = (13 * ctx->sRCParams.BufferSize) >> 4;
    */
    }

    if (ctx->sRCParams.BUSize != old_busize)
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Patched Basic unit to %d (original=%d)\n", ctx->sRCParams.BUSize, old_busize);
}


static void pnw__update_rcdata(
    context_ENC_p psContext,
    PIC_PARAMS *psPicParams,
    IMG_RC_PARAMS *psRCParams)
{
    double      L1, L2, L3, L4, L5, L6, flBpp;
    IMG_INT16           i16TempQP;
    IMG_INT32   i32BufferSizeInFrames = 0;

    flBpp = 1.0 * psRCParams->BitsPerSecond
            / (psRCParams->FrameRate * psContext->Width * psContext->Height);

    if (psContext->Width <= 176) {
        /* for very small franes we need to adjust the calculations */
        flBpp = flBpp / 2.0;
    }

    psPicParams->sInParams.IntraPeriod =  psRCParams->IntraFreq;
    psPicParams->sInParams.BitRate = psRCParams->BitsPerSecond;
    psPicParams->sInParams.BitsPerFrm = (psRCParams->BitsPerSecond + psRCParams->FrameRate / 2) / psRCParams->FrameRate;
    psPicParams->sInParams.BitsPerGOP = (psRCParams->BitsPerSecond / psRCParams->FrameRate) * psRCParams->IntraFreq;
    psPicParams->sInParams.BitsPerBU    = psPicParams->sInParams.BitsPerFrm / (4 * psPicParams->sInParams.BUPerFrm);
    psPicParams->sInParams.BitsPerMB    = psPicParams->sInParams.BitsPerBU / psRCParams->BUSize;
    psPicParams->sInParams.TransferRate = psRCParams->BitsPerSecond / psRCParams->FrameRate;

    i32BufferSizeInFrames = psRCParams->BufferSize / psPicParams->sInParams.BitsPerFrm;

    /* select thresholds and initial Qps etc that are codec dependent */
    switch (psContext->eCodec) {
    case IMG_CODEC_H264_CBR:
    case IMG_CODEC_H264_VCM:
    case IMG_CODEC_H264_VBR:
        L1 = 0.1;
        L2 = 0.15;
        L3 = 0.2;

        /* Set MaxQP to avoid blocky image in low bitrate */
        /* RCScaleFactor indicates the size of GOP for rate control */
        if (psContext->eCodec == IMG_CODEC_H264_VCM) {
            psPicParams->sInParams.MaxQPVal = 51;
            psPicParams->sInParams.RCScaleFactor = 16;
        }
        else {
            psPicParams->sInParams.MaxQPVal = 51;
            psPicParams->sInParams.RCScaleFactor = 16;
        }

        /* Setup MAX and MIN Quant Values */
        if (flBpp >= 0.50)
            i16TempQP = 4;
        else
            i16TempQP = (unsigned int)(26 - (40 * flBpp));

        psPicParams->sInParams.MinQPVal = (max(min(psPicParams->sInParams.MaxQPVal, i16TempQP), 0));

        L1 = 0.050568;
        L2 = 0.202272;
        L3 = 0.40454321;
        L4 = 0.80908642;
        L5 = 1.011358025;
        if (flBpp < L1)
            psPicParams->sInParams.SeInitQP = (IMG_UINT8)(47 - 78.10 * flBpp);

        else if (flBpp >= L1 && flBpp < L2)
            psPicParams->sInParams.SeInitQP = (IMG_UINT8)(45 - 66.67 * flBpp);

        else if (flBpp >= L2 && flBpp < L3)
            psPicParams->sInParams.SeInitQP = (IMG_UINT8)(36 - 24.72 * flBpp);

        else if (flBpp >= L3 && flBpp < L4)
            psPicParams->sInParams.SeInitQP = (IMG_UINT8)(34 - 19.78 * flBpp);

        else if (flBpp >= L4 && flBpp < L5)
            psPicParams->sInParams.SeInitQP = (IMG_UINT8)(27 - 9.89 * flBpp);

        else if (flBpp >= L5 && flBpp < 4)
            psPicParams->sInParams.SeInitQP = (IMG_UINT8)(20 - 4.95 * flBpp);
        else
            psPicParams->sInParams.SeInitQP = psPicParams->sInParams.MinQPVal;

        if (psPicParams->sInParams.SeInitQP < psPicParams->sInParams.MinQPVal)
            psPicParams->sInParams.SeInitQP = psPicParams->sInParams.MinQPVal;

        break;

    case IMG_CODEC_MPEG4_CBR:
    case IMG_CODEC_MPEG4_VBR:
    case IMG_CODEC_H263_CBR:
    case IMG_CODEC_H263_VBR:
        psPicParams->sInParams.RCScaleFactor = 16;
        psPicParams->sInParams.MaxQPVal  = 31;

        if (psContext->Width <= 176) {
            L1 = 0.043;
            L2 = 0.085;
            L3 = 0.126;
            L4 = 0.168;
            L5 = 0.336;
            L6 = 0.505;
        } else if (psContext->Width == 352) {
            L1 = 0.065;
            L2 = 0.085;
            L3 = 0.106;
            L4 = 0.126;
            L5 = 0.168 ;
            L6 = 0.210;
        } else {
            L1 = 0.051;
            L2 = 0.0770;
            L3 = 0.096;
            L4 = 0.145;
            L5 = 0.193;
            L6 = 0.289;
        }

        /* Calculate Initial QP if it has not been specified */
        if (flBpp < L1)
            psPicParams->sInParams.SeInitQP = 31;

        else if (flBpp >= L1 && flBpp < L2)
            psPicParams->sInParams.SeInitQP = 26;

        else if (flBpp >= L2 && flBpp < L3)
            psPicParams->sInParams.SeInitQP = 22;

        else if (flBpp >= L3 && flBpp < L4)
            psPicParams->sInParams.SeInitQP = 18;

        else if (flBpp >= L4 && flBpp < L5)
            psPicParams->sInParams.SeInitQP = 14;

        else if (flBpp >= L5 && flBpp < L6)
            psPicParams->sInParams.SeInitQP = 10;
        else
            psPicParams->sInParams.SeInitQP = 8;

        psPicParams->sInParams.AvQPVal =  psPicParams->sInParams.SeInitQP;

        if (flBpp >= 0.25
                && (psContext->eCodec == IMG_CODEC_MPEG4_CBR ||
                    psContext->eCodec == IMG_CODEC_MPEG4_VBR)) {
            psPicParams->sInParams.MinQPVal = 1;
        } else {
            psPicParams->sInParams.MinQPVal = 4;
        }
        break;

    default:
        /* the NO RC cases will fall here */
        break;
    }

    /* Set up Input Parameters that are mode dependent */
    switch (psContext->eCodec) {
    case IMG_CODEC_H264_NO_RC:
    case IMG_CODEC_H263_NO_RC:
    case IMG_CODEC_MPEG4_NO_RC:
        return ;

    case IMG_CODEC_H264_VCM:
        psPicParams->Flags      |= (ISVCM_FLAGS | ISCBR_FLAGS);
        if (psContext->Height >= 480) {
            /* for SD and above we can target 95% (122/128) of maximum bitrate */
            psPicParams->sInParams.VCMBitrateMargin = 122;
        } else {
            /* for less and SD we target 99% of maximum bitrate */
            psPicParams->sInParams.VCMBitrateMargin = 127;
        }
        if (i32BufferSizeInFrames < 15) {
            /* when we have a very small window size we reduce the target
             * further to avoid too much skipping */
            psPicParams->sInParams.VCMBitrateMargin -= 5;
        }
        psPicParams->sInParams.ForeceSkipMargin = 0; /* start skipping MBs when within 500 bits of slice or frame limit */
        if (psRCParams->BitsPerSecond < 1000000) {      // 1 Mbits/s
            psPicParams->sInParams.ScaleFactor = 0;
        } else if (psRCParams->BitsPerSecond < 2000000) { // 2 Mbits/s
            psPicParams->sInParams.ScaleFactor = 1;
        } else if (psRCParams->BitsPerSecond < 4000000) { // 4 Mbits/s
            psPicParams->sInParams.ScaleFactor = 2;
        } else if (psRCParams->BitsPerSecond < 8000000) { // 8 Mbits/s
            psPicParams->sInParams.ScaleFactor = 3;
        } else {
            psPicParams->sInParams.ScaleFactor = 4;
        }

        psPicParams->sInParams.BufferSize = i32BufferSizeInFrames;

        break;
    case IMG_CODEC_H264_CBR:
        psPicParams->Flags |= ISCBR_FLAGS;
        /* ------------------- H264 CBR RC ------------------- */
        /* Initialize the parameters of fluid flow traffic model. */
        psPicParams->sInParams.BufferSize = psRCParams->BufferSize;

        /* HRD consideration - These values are used by H.264 reference code. */
        if (psRCParams->BitsPerSecond < 1000000) {      /* 1 Mbits/s */
            psPicParams->sInParams.ScaleFactor = 0;
        } else if (psRCParams->BitsPerSecond < 2000000) { /* 2 Mbits/s */
            psPicParams->sInParams.ScaleFactor = 1;
        } else if (psRCParams->BitsPerSecond < 4000000) { /* 4 Mbits/s */
            psPicParams->sInParams.ScaleFactor = 2;
        } else if (psRCParams->BitsPerSecond < 8000000) { /* 8 Mbits/s */
            psPicParams->sInParams.ScaleFactor = 3;
        } else {
            psPicParams->sInParams.ScaleFactor = 4;
        }
        break;

    case IMG_CODEC_MPEG4_CBR:
    case IMG_CODEC_H263_CBR:
        psPicParams->Flags |= ISCBR_FLAGS;

        flBpp  = 256 * (psRCParams->BitsPerSecond / psContext->Width);
        flBpp /= (psContext->Height * psRCParams->FrameRate);

        if ((psPicParams->sInParams.MBPerFrm > 1024 && flBpp < 16) || (psPicParams->sInParams.MBPerFrm <= 1024 && flBpp < 24))
            psPicParams->sInParams.HalfFrameRate = 1;
        else
            psPicParams->sInParams.HalfFrameRate = 0;

        if (psPicParams->sInParams.HalfFrameRate >= 1) {
            psPicParams->sInParams.SeInitQP = 31;
            psPicParams->sInParams.AvQPVal = 31;
            psPicParams->sInParams.MyInitQP = 31;
        }

        psPicParams->sInParams.BufferSize = psRCParams->BufferSize;
        if (psPicParams->sInParams.BufferSize > 112 * 16384) // Simple Profile L5 Constraints
            psPicParams->sInParams.BufferSize = 112 * 16384;
        break;

    case IMG_CODEC_MPEG4_VBR:
    case IMG_CODEC_H263_VBR:
    case IMG_CODEC_H264_VBR:
        psPicParams->Flags |= ISVBR_FLAGS;

        psPicParams->sInParams.MBPerBU  = psPicParams->sInParams.MBPerFrm;
        psPicParams->sInParams.BUPerFrm = 1;

        /* Initialize the parameters of fluid flow traffic model. */
        psPicParams->sInParams.BufferSize = psRCParams->BufferSize;

        if (psContext->eCodec != IMG_CODEC_H264_VBR) {
            if (psPicParams->sInParams.BufferSize >  112 * 16384)
                psPicParams->sInParams.BufferSize = 112 * 16384; // Simple Profile L5 Constraints
        }

        /* These scale factor are used only for rate control to avoid overflow */
        /* in fixed-point calculation these scale factors are decided by bit rate */
        if (psRCParams->BitsPerSecond < 640000) {
            psPicParams->sInParams.ScaleFactor  = 2;                                            /* related to complexity */
        } else if (psRCParams->BitsPerSecond < 2000000) {
            psPicParams->sInParams.ScaleFactor  = 4;
        } else {
            psPicParams->sInParams.ScaleFactor  = 6;
        }
        break;
    default:
        break;
    }

    psPicParams->sInParams.MyInitQP     = psPicParams->sInParams.SeInitQP;

#if 0
    if (psContext->SyncSequencer)
        psPicParams->Flags |= SYNC_SEQUENCER;
#endif

    psPicParams->sInParams.InitialDelay = psRCParams->InitialDelay;
    psPicParams->sInParams.InitialLevel = psRCParams->InitialLevel;
    psRCParams->InitialQp = psPicParams->sInParams.SeInitQP;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "InitQP %d, minQP %d, maxQP %d\n",
            psPicParams->sInParams.SeInitQP,
            psPicParams->sInParams.MinQPVal,
            psPicParams->sInParams.MaxQPVal);

    /* The rate control uses this value to adjust the reaction rate
       to larger than expected frames in long GOPS*/

    return;
}


/***********************************************************************************
 * Function Name      : SetupRCData
 * Inputs             :
 * Outputs            :
 * Returns            :
 * Description        : Sets up RC Data
 ************************************************************************************/
void pnw__setup_rcdata(
    context_ENC_p psContext,
    PIC_PARAMS *psPicParams,
    IMG_RC_PARAMS *psRCParams)
{
    IMG_UINT8   ui8InitialSeInitQP;

    /* frameskip is always cleared, specially handled at vaEndPicture */
    psRCParams->FrameSkip = 0;

    if (!psRCParams->BitsPerSecond)
        psRCParams->BitsPerSecond = 64000;
    /*
        if (psRCParams->BitsPerSecond > max_bitrate)
            psRCParams->BitsPerSecond = max_bitrate;
    */
    if (!psRCParams->FrameRate)
        psRCParams->FrameRate = 30;

    pnw__setup_busize(psContext); /* calculate BasicUnitSize */

    psPicParams->sInParams.SeInitQP = psRCParams->InitialQp;

    psPicParams->sInParams.MBPerRow = (psContext->Width >> 4);
    psPicParams->sInParams.MBPerBU = psRCParams->BUSize;
    psPicParams->sInParams.MBPerFrm     = (psContext->Width >> 4) * (psContext->Height >> 4);
    psPicParams->sInParams.BUPerFrm     = (psPicParams->sInParams.MBPerFrm) / psRCParams->BUSize;
    psPicParams->sInParams.AvQPVal = psRCParams->InitialQp;
    psPicParams->sInParams.MyInitQP     = psRCParams->InitialQp;
    psPicParams->sInParams.MaxFrameSize = psRCParams->BitsPerSecond / psRCParams->FrameRate;

    ui8InitialSeInitQP = psPicParams->sInParams.SeInitQP;

    pnw__update_rcdata(psContext, psPicParams, psRCParams);

    /*If MinQP has been set, restore this value rather than
     *the calculated value set by UpdateRCData()*/
    if (psRCParams->MinQP) {
        psPicParams->sInParams.MinQPVal = (IMG_UINT8)psRCParams->MinQP;
    }

    /*If SeInitQP has been set, restore this value and other
     * dependant variables rather than the calculated values set by UpdateRCData()*/
    if (ui8InitialSeInitQP) {
        psPicParams->sInParams.SeInitQP = ui8InitialSeInitQP;
        psPicParams->sInParams.MyInitQP = ui8InitialSeInitQP;
        psRCParams->InitialQp = ui8InitialSeInitQP;
    }

    /* HRD parameters are meaningless without a bitrate
     * HRD parameters are not supported in VCM mode */
    if (psRCParams->BitsPerSecond == 0 || psContext->eCodec == IMG_CODEC_H264_VCM)
         psContext->bInserHRDParams = IMG_FALSE;

    if (psContext->bInserHRDParams) {
        psPicParams->ClockDivBitrate = (90000 * 0x100000000LL);
        psPicParams->ClockDivBitrate /= psRCParams->BitsPerSecond;
        psPicParams->MaxBufferMultClockDivBitrate = (IMG_UINT32)
                (((IMG_UINT64)(psRCParams->BufferSize) * (IMG_UINT64) 90000)
                 / (IMG_UINT64) psRCParams->BitsPerSecond);
    }
    return ;
}

static void pnw__setup_qpvalue_h264(
    MTX_CURRENT_IN_PARAMS * psCurrent,
    IMG_BYTE bySliceQP)
{
    /* H.264 QP scaling tables */
    IMG_BYTE HOST_PVR_QP_SCALE_CR[52] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
        12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
        28, 29, 29, 30, 31, 32, 32, 33, 34, 34, 35, 35, 36, 36, 37, 37,
        37, 38, 38, 38, 39, 39, 39, 39
    };

    psCurrent->bySliceQP = bySliceQP;
    psCurrent->bySliceQPC = HOST_PVR_QP_SCALE_CR[psCurrent->bySliceQP];
}


static void pnw__setup_qpvalues_mpeg4(
    MTX_CURRENT_IN_PARAMS * psCurrent,
    IMG_BYTE bySliceQP)
{
    psCurrent->bySliceQP =      bySliceQP;
}

static void pnw__setup_slice_row_params(
    context_ENC_p ctx,
    IMG_BOOL IsIntra,
    IMG_UINT16 CurrentRowY,
    IMG_INT16 SliceStartRowY,
    IMG_INT16 SliceHeight,
    IMG_BOOL VectorsValid,
    int bySliceQP)
{
    /* Note: CurrentRowY and SliceStartRowY are now in pixels (not MacroBlocks)
     * - saves needless multiplications and divisions
     */
    MTX_CURRENT_IN_PARAMS *psCurrent;
    pnw_cmdbuf_p cmdbuf = ctx->obj_context->pnw_cmdbuf;

    IMG_INT16   iPos, iYPos, srcY;
    IMG_UINT16  ui16tmp;
    IMG_UINT16 ui16SearchWidth, ui16SearchHeight, ui16SearchLeftOffset, ui16SearchTopOffset, ui16CurBlockX;

    if (IsIntra && cmdbuf->topaz_in_params_I_p == NULL) {
        VAStatus vaStatus = psb_buffer_map(cmdbuf->topaz_in_params_I, &cmdbuf->topaz_in_params_I_p);
        if (vaStatus != VA_STATUS_SUCCESS) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "map topaz MTX_CURRENT_IN_PARAMS failed\n");
            return;
        }
    }

    if ((!IsIntra) && cmdbuf->topaz_in_params_P_p == NULL) {
        VAStatus vaStatus = psb_buffer_map(cmdbuf->topaz_in_params_P, &cmdbuf->topaz_in_params_P_p);
        if (vaStatus != VA_STATUS_SUCCESS) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "map topaz MTX_CURRENT_IN_PARAMS failed\n");
            return;
        }
    }

    if (IsIntra)
        psCurrent = (MTX_CURRENT_IN_PARAMS*)(cmdbuf->topaz_in_params_I_p + ctx->in_params_ofs);
    else
        psCurrent = (MTX_CURRENT_IN_PARAMS*)(cmdbuf->topaz_in_params_P_p + ctx->in_params_ofs);

    psCurrent += (CurrentRowY  * (ctx->Width) / 256);

    // Note: CurrentRowY and iSliceStartRowY are now in pixels (not MacroBlocks) - saves needless multiplications and divisions

    ui16SearchHeight = min(MVEA_LRB_SEARCH_HEIGHT, ctx->Height);
    ui16SearchWidth = min(MVEA_LRB_SEARCH_WIDTH, ctx->Width);
    ui16SearchLeftOffset = (((ui16SearchWidth / 2) / 16) * 16); // this is the amount of data that gets preloaded
    ui16SearchTopOffset = (((ui16SearchHeight / 2) / 16) * 16);
    ui16CurBlockX = MVEA_LRB_SEARCH_WIDTH - (ui16SearchLeftOffset + 16); // this is our block position relative to the start of the LRB

    if ((iYPos = srcY = CurrentRowY - ui16SearchTopOffset) < 0)
        srcY = 0;
    else if (iYPos > ctx->HeightMinusLRB_TopAndBottom_OffsetsPlus16)
        srcY = ctx->HeightMinusLRBSearchHeight;


    /*DDK 243 removed this block of code.*/
    /*if((ctx->eCodec==IMG_CODEC_H263_NO_RC)||(ctx->eCodec==IMG_CODEC_H263_CBR)||(ctx->eCodec==IMG_CODEC_H263_VBR))
      ui16tmp = CurrentRowY;
      else*/
    ui16tmp = (CurrentRowY != SliceStartRowY);

    for (iPos = 0; iPos < ctx->Width; iPos += 16, psCurrent++) {
        memset(psCurrent, 0, sizeof(MTX_CURRENT_IN_PARAMS));
        psCurrent->MVValid = 0;
        psCurrent->ParamsValid = 0;

        if (SliceStartRowY) {
            psCurrent->MVValid = VECTORS_ABOVE_VALID;
        }
        /* Setup the parameters and motion vectors*/
        if (ui16tmp) {
            psCurrent->MVValid = VECTORS_ABOVE_VALID | DO_INTRA_PRED;
            psCurrent->ParamsValid |= PARAMS_ABOVE_VALID;

            if (iPos + 16 < ctx->Width) {
                psCurrent->ParamsValid |= PARAMS_ABOVER_VALID;
                psCurrent->MVValid |= /*VECTORS_LEFT_VALID; //*/(1 << 2); /* Vectors left valid define looks wrong*/
            }

            if (iPos > 0 && (iPos < ctx->Width)) {
                psCurrent->ParamsValid |= PARAMS_ABOVEL_VALID;
                psCurrent->MVValid |= VECTORS_ABOVE_LEFT_VALID; //(1<<0)
            }
        } else {
            // are we the first MB in a new slice?
            if (iPos == 0) {
                if ((ctx->eCodec == IMG_CODEC_H263_NO_RC) || (ctx->eCodec == IMG_CODEC_H263_CBR) || (ctx->eCodec == IMG_CODEC_H263_VBR)) {
                    if (iYPos == -ui16SearchTopOffset)
                        psCurrent->ParamsValid |= MB_START_OF_SLICE;// OPTI?
                } else {
                    psCurrent->ParamsValid |= MB_START_OF_SLICE;// OPTI?
                }
            }
        }
        /*DDK 243 removed this block of code.*/
        /*if((ctx->eCodec==IMG_CODEC_H263_NO_RC) || (ctx->eCodec==IMG_CODEC_H263_CBR)||(ctx->eCodec==IMG_CODEC_H263_VBR))
          {
        // clear the above params valid bits
        psCurrent->ParamsValid &=~(PARAMS_ABOVEL_VALID|PARAMS_ABOVER_VALID|PARAMS_ABOVE_VALID); // OPTI
        }*/
        // Have to fill in the right hand row of 4x4 vectors into the the left block
        if (iPos) {
            psCurrent->MVValid |= DO_INTRA_PRED | (1 << 3); /*MV_VALID define looks wrong?! so use hard coded value for now*/
            psCurrent->ParamsValid |= 8; //(1<<3)
        }
        if (iPos == ctx->Width - 16) {
            // indicate the last MB in a row
            psCurrent->ParamsValid |= MB_END_OF_ROW;
            // are we the last mb in the slice?
            if (iYPos == (SliceStartRowY + SliceHeight - (ui16SearchTopOffset + 16))) {
                psCurrent->ParamsValid |= MB_END_OF_SLICE;
                if (iYPos == ctx->HeightMinus16MinusLRBTopOffset) {
                    psCurrent->ParamsValid |= MB_END_OF_PICTURE;
                }
            }
        }
        // And now the below block
        // should do some kind of check to see if we are the first inter block, as otherwise the vectors will be invalid!
        if (VectorsValid) {
            if (iYPos < ctx->HeightMinus16MinusLRBTopOffset) {
                psCurrent->MVValid |= VECTORS_BELOW_VALID; //(1<<4)

                if (iYPos < ctx->HeightMinus32MinusLRBTopOffset) {
                    psCurrent->MVValid |= VECTORS_2BELOW_VALID; //(1<<5)
                }
            }
        }

        /*Set up IPEMin and Max for coordinate X in the search reference region*/
        /*And set up flags in SPEMax when needed*/
        if (iPos <= ui16SearchLeftOffset) {
            psCurrent->IPEMin[0] = ui16CurBlockX - iPos;
            psCurrent->RealEdge |= SPE_EDGE_LEFT;
        } else {
            psCurrent->IPEMin[0] = ui16CurBlockX / 16;
        }

        if ((iPos + ui16SearchLeftOffset + 16) > ctx->Width) {
            psCurrent->IPEMax[0] = (ui16CurBlockX - 1 + ctx->Width) - iPos; //(112 - 1) - ((iPos + 48+16) - ctx->psVideo->ui16Width);
            psCurrent->RealEdge |= SPE_EDGE_RIGHT;
        } else {
            psCurrent->IPEMax[0] = (ui16CurBlockX + 16 + ui16SearchLeftOffset) - 1 - 3; //(112 - 1) - 3;
        }

        /*Set up IPEMin and Max for Y coordinate in the search reference region*/
        /*And set up flags in SPEMax when needed*/
        if (iYPos <= 0) {
            psCurrent->IPEMin[1] = 0;
            psCurrent->RealEdge |= SPE_EDGE_TOP;
        } else {
            psCurrent->IPEMin[1] = 3;
        }

        //Max Y
        if (iYPos > ctx->HeightMinusLRB_TopAndBottom_OffsetsPlus16) {
            psCurrent->IPEMax[1] = ui16SearchHeight - 1;
            psCurrent->RealEdge |= ui16SearchHeight - 4;
        } else {
            psCurrent->IPEMax[1] = ui16SearchHeight - 4;
        }

        psCurrent->CurBlockAddr = ((ui16CurBlockX) / 16);
        psCurrent->CurBlockAddr |= ((IMG_UINT8)(((iYPos + ui16SearchTopOffset) - srcY) / 16) << 4);

        /* Setup the control register values
           These will get setup and transferred to a different location within the macroblock parameter structure.
           They are then read out of the esb by the mtx and used to control the hardware units
           */
        psCurrent->IPEControl = ctx->IPEControl;

        switch (ctx->eCodec) {
        case IMG_CODEC_H263_NO_RC:
        case IMG_CODEC_H263_VBR:
        case IMG_CODEC_H263_CBR:
            pnw__setup_qpvalues_mpeg4(psCurrent, bySliceQP);
            psCurrent->JMCompControl = F_ENCODE(2, MVEA_CR_JMCOMP_MODE);
            psCurrent->VLCControl = F_ENCODE(3, TOPAZ_VLC_CR_CODEC) | F_ENCODE(IsIntra ? 0 : 1, TOPAZ_VLC_CR_SLICE_CODING_TYPE);
            break;
        case IMG_CODEC_MPEG4_NO_RC:
        case IMG_CODEC_MPEG4_VBR:
        case IMG_CODEC_MPEG4_CBR:
            pnw__setup_qpvalues_mpeg4(psCurrent, bySliceQP);
            psCurrent->JMCompControl = F_ENCODE(1, MVEA_CR_JMCOMP_MODE) | F_ENCODE(1, MVEA_CR_JMCOMP_AC_ENABLE);
            psCurrent->VLCControl = F_ENCODE(2, TOPAZ_VLC_CR_CODEC) | F_ENCODE(IsIntra ? 0 : 1, TOPAZ_VLC_CR_SLICE_CODING_TYPE);
            break;
        default:
        case IMG_CODEC_H264_NO_RC:
        case IMG_CODEC_H264_VBR:
        case IMG_CODEC_H264_CBR:
        case IMG_CODEC_H264_VCM:
            pnw__setup_qpvalue_h264(psCurrent, bySliceQP);
            psCurrent->JMCompControl = F_ENCODE(0, MVEA_CR_JMCOMP_MODE);
            psCurrent->VLCControl = F_ENCODE(1, TOPAZ_VLC_CR_CODEC) | F_ENCODE(IsIntra ? 0 : 1, TOPAZ_VLC_CR_SLICE_CODING_TYPE);
            break;
        }
    }

    // now setup the dummy end of frame macroblock.
    if ((CurrentRowY + 16) >= ctx->Height) {
        memset(psCurrent, 0, sizeof(MTX_CURRENT_IN_PARAMS));
        psCurrent->MVValid = DO_INTRA_PRED;
        psCurrent->ParamsValid = 0;
        psCurrent->RealEdge = 0;
    }
}

void pnw_setup_slice_params(
    context_ENC_p  ctx,
    IMG_UINT16 YSliceStartPos,
    IMG_UINT16 SliceHeight,
    IMG_BOOL IsIntra,
    IMG_BOOL  VectorsValid,
    int bySliceQP)
{
    IMG_UINT16 Rows, CurrentRowY;

    Rows = SliceHeight / 16;
    CurrentRowY = YSliceStartPos;

    while (Rows) {
        pnw__setup_slice_row_params(
            ctx,
            IsIntra,
            CurrentRowY,
            YSliceStartPos,
            SliceHeight,
            VectorsValid, bySliceQP);

        CurrentRowY += 16;
        Rows--;
    }

}



IMG_UINT32 pnw__send_encode_slice_params(
    context_ENC_p ctx,
    IMG_BOOL IsIntra,
    IMG_UINT16 CurrentRow,
    IMG_UINT8  DeblockIDC,
    IMG_UINT32 FrameNum,
    IMG_UINT16 SliceHeight,
    IMG_UINT16 CurrentSlice)
{
    SLICE_PARAMS *psSliceParams;
    IMG_INT16 RowOffset;
    IMG_UINT16 SearchHeight, SearchTopOffset;

    psb_buffer_p psCoded;
    object_surface_p ref_surface;
    psb_buffer_p psRef;
    pnw_cmdbuf_p cmdbuf = ctx->obj_context->pnw_cmdbuf;


    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Send encode slice parmas, Is Intra:%d, CurrentRow:%d" \
                             "DeblockIDC:%d, FrameNum:%d, SliceHeight:%d, CurrentSlice:%d\n",
                             IsIntra, CurrentRow, DeblockIDC, FrameNum, SliceHeight, CurrentSlice);

    ref_surface = ctx->ref_surface;
    psRef = &ctx->ref_surface->psb_surface->buf;
    psCoded = ctx->coded_buf->psb_buffer;

    psSliceParams = (SLICE_PARAMS *)(cmdbuf->slice_params_p +
                                     CurrentSlice * ((sizeof(SLICE_PARAMS) + 15) & 0xfff0));

    psSliceParams->SliceHeight = SliceHeight;
    psSliceParams->SliceStartRowNum = CurrentRow / 16;
    psSliceParams->ScanningIntraParams = (1 << SCANNING_INTRA_WIDTH_SHIFT) |
        (1 << SCANNING_INTRA_STEP_SHIFT);

    /* We want multiple ones of these so we can submit multiple slices without having to wait for the next */
    psSliceParams->Flags = 0;
    psSliceParams->HostCtx = 0xdafed123;

#ifdef VA_EMULATOR
    psSliceParams->RefYStride = ref_surface->psb_surface->stride;
    psSliceParams->RefUVStride = ref_surface->psb_surface->stride;
    psSliceParams->RefYRowStride =  ref_surface->psb_surface->stride * 16;
    psSliceParams->RefUVRowStride = ref_surface->psb_surface->stride * 16 / 2;
#else
    psSliceParams->RefYStride = ref_surface->psb_surface->stride;
    psSliceParams->RefUVStride = ref_surface->psb_surface->stride;
    psSliceParams->RefYRowStride =  ref_surface->psb_surface->stride * 16;
    psSliceParams->RefUVRowStride = ref_surface->psb_surface->stride * 16 / 2;
#endif
    psSliceParams->NumAirMBs = ctx->num_air_mbs;
    psSliceParams->AirThreshold = ctx->air_threshold;

    if (ctx->eCodec == IMG_CODEC_H264_VCM && ctx->max_slice_size == 0)
        psSliceParams->MaxSliceSize = ctx->Width * ctx->Height * 12 / 2;
    else
        psSliceParams->MaxSliceSize = ctx->max_slice_size;
    psSliceParams->FCode = ctx->FCode;/* Not clear yet, This field is not appare in firmware doc */

    SearchHeight = min(MVEA_LRB_SEARCH_HEIGHT, ctx->Height);
    SearchTopOffset = (((SearchHeight / 2) / 16) * 16);

    RowOffset = CurrentRow - SearchTopOffset;
    if (RowOffset <= 0)
        RowOffset = 0;
    if (RowOffset > (ctx->Height - SearchHeight))
        RowOffset = (ctx->Height - SearchHeight);
    if (!IsIntra) {
        psSliceParams->Flags |= ISINTER_FLAGS;
    }

    switch (DeblockIDC) {
    case 0:
        psSliceParams->Flags |= DEBLOCK_FRAME;
        break;
    case 2:
        psSliceParams->Flags |= DEBLOCK_SLICE;
        break;
    case 1:
    default:
        // do nothing
        break;
    }

    switch (ctx->eCodec) {
    case IMG_CODEC_H263_NO_RC:
    case IMG_CODEC_H263_VBR:
    case IMG_CODEC_H263_CBR:
        psSliceParams->Flags |= ISH263_FLAGS;
        break;
    case IMG_CODEC_MPEG4_NO_RC:
    case IMG_CODEC_MPEG4_VBR:
    case IMG_CODEC_MPEG4_CBR:
        psSliceParams->Flags |= ISMPEG4_FLAGS;
        break;
    case IMG_CODEC_H264_NO_RC:
    case IMG_CODEC_H264_CBR:
    case IMG_CODEC_H264_VCM:
    case IMG_CODEC_H264_VBR:
        psSliceParams->Flags |= ISH264_FLAGS;
        break;
    default:
        psSliceParams->Flags |= ISH264_FLAGS;
        printf("No format specified defaulting to h.264\n");
        break;
    }
    /* we should also setup the interleaving requirements based on the source format */
    if (ctx->eFormat == IMG_CODEC_PL12) /* FIXME contrary with old DDK, take notice */
        psSliceParams->Flags |= INTERLEAVE_TARGET;

    cmdbuf = ctx->obj_context->pnw_cmdbuf;

    RELOC_SLICE_PARAMS_PNW(&(psSliceParams->RefYBase), 256 * RowOffset / 16, psRef);
    RELOC_SLICE_PARAMS_PNW(&(psSliceParams->RefUVBase),
                           ref_surface->psb_surface->stride * ref_surface->height + (RowOffset * 128 / 16),
                           psRef);
    if (IsIntra)
        RELOC_SLICE_PARAMS_PNW(&(psSliceParams->InParamsBase),
                               ctx->in_params_ofs,
                               //((CurrentRow * (ctx->Width)) / 256 + ctx->obj_context->slice_count) * sizeof(MTX_CURRENT_IN_PARAMS),
                               cmdbuf->topaz_in_params_I);
    else
        RELOC_SLICE_PARAMS_PNW(&(psSliceParams->InParamsBase),
                               ctx->in_params_ofs,
                               //((CurrentRow * (ctx->Width)) / 256 + ctx->obj_context->slice_count) * sizeof(MTX_CURRENT_IN_PARAMS),
                               cmdbuf->topaz_in_params_P);

#if  TOPAZ_PIC_PARAMS_VERBOSE
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "sizeof psSliceParams %d\n", sizeof(*psSliceParams));
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "sizeof MTX_CURRENT_IN_PARAMS %d\n", sizeof(MTX_CURRENT_IN_PARAMS));
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psSliceParams->SliceStartRowNum %d\n", psSliceParams->SliceStartRowNum);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psSliceParams->SliceHeight %d\n", psSliceParams->SliceHeight);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psSliceParams->RefYBase %x\n", psSliceParams->RefYBase);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psSliceParams->RefUVBase %x\n", psSliceParams->RefUVBase);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psSliceParams->RefYStride %d\n", psSliceParams->RefYStride);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psSliceParams->RefUVStride %d\n", psSliceParams->RefUVStride);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psSliceParams->RefYRowStride %d\n", psSliceParams->RefYRowStride);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psSliceParams->RefUVRowStride %d\n", psSliceParams->RefUVRowStride);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psSliceParams->HostCtx %x\n", psSliceParams->HostCtx);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psSliceParams->Flags %x\n", psSliceParams->Flags);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psSliceParams->MaxSliceSize %d\n",  psSliceParams->MaxSliceSize);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psSliceParams->FCode %x\n", psSliceParams->FCode);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psSliceParams->InParamsBase %x\n", psSliceParams->InParamsBase);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psSliceParams->NumAirMBs %d\n", psSliceParams->NumAirMBs);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psSliceParams->AirThreshold %x\n", psSliceParams->AirThreshold);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psSliceParams->ScanningIntraParams %x\n", psSliceParams->ScanningIntraParams);
#endif

    pnw_cmdbuf_insert_command_package(ctx->obj_context,
                                      ctx->SliceToCore,
                                      MTX_CMDID_ENCODE_SLICE,
                                      &cmdbuf->slice_params,
                                      CurrentSlice *((sizeof(SLICE_PARAMS) + 15) & 0xfff0));


    return 0;
}



/*
 * Function Name      : Reset_EncoderParams
 * Description        : Reset Above & Below Params at the Start of Intra frame
 */
void pnw_reset_encoder_params(context_ENC_p ctx)
{
    unsigned char *Add_Below, *Add_Above;
    pnw_cmdbuf_p cmdbuf = ctx->obj_context->pnw_cmdbuf;

    /* all frames share the same Topaz param, in_param/aboveparam/bellow
     * map it only when necessary
     */
    if (cmdbuf->topaz_above_params_p == NULL) {
        VAStatus vaStatus = psb_buffer_map(cmdbuf->topaz_above_params, &cmdbuf->topaz_above_params_p);
        if (vaStatus != VA_STATUS_SUCCESS) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "map topaz MTX_CURRENT_IN_PARAMS failed\n");
            return;
        }
    }

    if (cmdbuf->topaz_below_params_p == NULL) {
        VAStatus vaStatus = psb_buffer_map(cmdbuf->topaz_below_params, &cmdbuf->topaz_below_params_p);
        if (vaStatus != VA_STATUS_SUCCESS) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "map topaz MTX_CURRENT_IN_PARAMS failed\n");
            return;
        }
    }

    Add_Below = cmdbuf->topaz_below_params_p +
                ctx->below_params_ofs;

    memset(Add_Below, 0, ctx->below_params_size * 4);

    Add_Above = cmdbuf->topaz_above_params_p + ctx->above_params_ofs;
    memset(Add_Above, 0, ctx->above_params_size * MAX_TOPAZ_CORES);
}



