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
#include "hwdefs/topazhp_core_regs.h"
#include "hwdefs/topaz_vlc_regs.h"
#include "hwdefs/topazhp_multicore_regs.h"
#include "hwdefs/topazhp_multicore_regs_old.h"
#include "psb_drv_video.h"
#include "tng_cmdbuf.h"
#include "tng_hostbias.h"
#include "psb_drv_debug.h"
#include <stdlib.h>

#define UNINIT_PARAM 0xCDCDCDCD
#define TOPAZHP_DEFAULT_bZeroDetectionDisable IMG_FALSE
#define TH_SKIP_IPE                      6
#define TH_INTER                           60
#define TH_INTER_QP                    10
#define TH_INTER_MAX_LEVEL      1500
#define TH_SKIP_SPE                     6

#define TOPAZHP_DEFAULT_uTHInter                           TH_INTER
#define TOPAZHP_DEFAULT_uTHInterQP                      TH_INTER_QP
#define TOPAZHP_DEFAULT_uTHInterMaxLevel		     TH_INTER_MAX_LEVEL
#define TOPAZHP_DEFAULT_uTHSkipIPE                      TH_SKIP_IPE
#define TOPAZHP_DEFAULT_uTHSkipSPE                      TH_SKIP_SPE

#define MIN_32_REV 0x00030200
#define MAX_32_REV 0x00030299
// New MP4 Lambda table
static IMG_UINT8 MPEG4_QPLAMBDA_MAP[31] = {
    0,  0,  1,  2,  3,
    3,  4,  4,  5,  5,
    6,  6,  7,  7,  8,
    8,  9,  9,  10, 10,
    11, 11, 11, 11, 12,
    12, 12, 12, 13, 13, 13
};

static IMG_UINT8 H263_QPLAMBDA_MAP[31] = {
    0, 0, 1, 1, 2,
    2, 3, 3, 4, 4,
    4, 5, 5, 5, 6,
    6, 6, 7, 7, 7,
    7, 8, 8, 8, 8,
    9, 9, 9, 9, 10,10
};

// new H.264 Lambda
static IMG_INT8 H264_QPLAMBDA_MAP_SAD[40] = {
    2, 2, 2, 2, 3, 3, 4, 4,
    4, 5, 5, 5, 5, 5, 6, 6,
    6, 7, 7, 7, 8, 8, 9, 11,
    13, 14, 15, 17, 20, 23, 27, 31,
    36, 41, 51, 62, 74, 79, 85, 91
};

static IMG_INT8 H264_QPLAMBDA_MAP_SATD_TABLES[][52] = {
    //table 0
    {
        8,  8,  8,  8,  8,  8,  8,
        8,  8,  8,  8,  8,  8,  8,
        8,  8,  8,  9,  9,     10, 10,
        11, 11, 12, 12, 13,     13, 14,
        14, 15, 15, 16, 17,     18, 20,
        21, 23, 25, 27, 30,     32, 38,
        44, 50, 56, 63, 67,     72, 77,
        82, 87, 92
    },
};

static IMG_INT32 H264_DIRECT_BIAS[27] = {
    24, 24, 24, 24, 24, 24, 24, 24, 36,
    48, 48, 60, 60, 72, 72, 84, 96, 108,
    200, 324, 384, 528, 672, 804, 924, 1044, 1104
};

static IMG_INT32 H264_INTRA8_SCALE[27] = {
    (234 + 8) >> 4, (231 + 8) >> 4,
    (226 + 8) >> 4, (221 + 8) >> 4,
    (217 + 8) >> 4, (213 + 8) >> 4,
    (210 + 8) >> 4, (207 + 8) >> 4,
    (204 + 8) >> 4, (202 + 8) >> 4,
    (200 + 8) >> 4, (199 + 8) >> 4,
    (197 + 8) >> 4, (197 + 8) >> 4,
    (196 + 8) >> 4, (196 + 8) >> 4,
    (197 + 8) >> 4, (197 + 8) >> 4,
    (198 + 8) >> 4, (200 + 8) >> 4,
    (202 + 8) >> 4, (204 + 8) >> 4,
    (207 + 8) >> 4, (210 + 8) >> 4,
    (213 + 8) >> 4, (217 + 8) >> 4,
    (217 + 8) >> 4
};

/***********************************************************************************
* Function Name : H264InterBias
* Inputs                                : ui8QP
* Outputs                               :
* Returns                               : IMG_INT16
* Description           : return the Inter Bias Value to use for the given QP
  ************************************************************************************/

static IMG_INT16 H264_InterIntraBias[27] = {
    20, 20, 20, 20, 20, 20, 50,
    100, 210, 420, 420, 445, 470,
    495, 520, 535, 550, 570, 715,
    860, 900, 1000, 1200, 1400,
    1600, 1800, 2000
};

static IMG_INT16 tng__H264ES_inter_bias(IMG_INT8 i8QP)
{
    if (i8QP < 1) {
        i8QP = 1;
    }
    if (i8QP > 51) {
        i8QP = 51;
    }

    //return aui16InterBiasValues[i8QP-36];
    return H264_InterIntraBias[(i8QP+1)>>1];
}

/*****************************************************************************
 *  Function Name      : CalculateDCScaler
 *  Inputs             : iQP, bChroma
 *  Outputs            : iDCScaler
 *  Returns            : IMG_INT
 *  Description        : Calculates either the Luma or Chroma DC scaler from the quantization scaler
 *******************************************************************************/
IMG_INT
CalculateDCScaler(IMG_INT iQP, IMG_BOOL bChroma)
{
    IMG_INT     iDCScaler;
    if(!bChroma)
    {
        if (iQP > 0 && iQP < 5)
        {
            iDCScaler = 8;
        }
        else if (iQP > 4 &&     iQP     < 9)
        {
            iDCScaler = 2 * iQP;
        }
        else if (iQP > 8 &&     iQP     < 25)
        {
            iDCScaler = iQP + 8;
        }
        else
        {
            iDCScaler = 2 * iQP -16;
        }
    }
    else
    {
        if (iQP > 0 && iQP < 5)
        {
            iDCScaler = 8;
        }
        else if (iQP > 4 &&     iQP     < 25)
        {
            iDCScaler = (iQP + 13) / 2;
        }
        else
        {
            iDCScaler = iQP - 6;
        }
    }
    return iDCScaler;
}

/**************************************************************************************************
* Function:		MPEG4_GenerateBiasTables
* Description:	Genereate the bias tables for MPEG4
*
***************************************************************************************************/
void
tng__MPEG4ES_generate_bias_tables(
	context_ENC_p ctx)
{
    IMG_INT16 n;
    IMG_INT16 iX;
    IMG_UINT32 ui32RegVal;
    IMG_UINT8 uiDCScaleL,uiDCScaleC,uiLambda;
    IMG_UINT32 uDirectVecBias,iInterMBBias,iIntra16Bias;
    IMG_BIAS_PARAMS *psBiasParams = &(ctx->sBiasParams);

    ctx->sBiasTables.ui32LritcCacheChunkConfig = F_ENCODE(ctx->uChunksPerMb, INTEL_CH_PM) |
						 F_ENCODE(ctx->uMaxChunks, INTEL_CH_MX) |
						 F_ENCODE(ctx->uMaxChunks - ctx->uPriorityChunks, INTEL_CH_PY);

 
    for(n=31;n>=1;n--)
    {
	iX = n-12;
	if(iX < 0)
	{
	    iX = 0;
	}
	// Dont Write QP Values To ESB -- IPE will write these values
	// Update the quantization parameter which includes doing Lamda and the Chroma QP

	uiDCScaleL = CalculateDCScaler(n, IMG_FALSE);
	uiDCScaleC = CalculateDCScaler(n, IMG_TRUE);
	uiLambda = psBiasParams->uLambdaSAD ? psBiasParams->uLambdaSAD : MPEG4_QPLAMBDA_MAP[n - 1];

	ui32RegVal = uiDCScaleL;
	ui32RegVal |= (uiDCScaleC)<<8;
	ui32RegVal |= (uiLambda)<<16;
	ctx->sBiasTables.aui32LambdaBias[n] = ui32RegVal;
    }

    for(n=31;n>=1;n-=2) {
        if(psBiasParams->bRCEnable || psBiasParams->bRCBiases) {
            uDirectVecBias = psBiasParams->uTHSkipIPE * uiLambda;
            iInterMBBias    = psBiasParams->uTHInter * (n - psBiasParams->uTHInterQP);
            //if(iInterMBBias < 0)
            //    iInterMBBias	= 0;
            if(iInterMBBias > psBiasParams->uTHInterMaxLevel)
                iInterMBBias	= psBiasParams->uTHInterMaxLevel;
            iIntra16Bias = 0;
        } else {
            uDirectVecBias  = psBiasParams->uIPESkipVecBias;
            iInterMBBias    = psBiasParams->iInterMBBias;
            iIntra16Bias    = psBiasParams->iIntra16Bias;
        }

        ctx->sBiasTables.aui32IntraBias[n] = iIntra16Bias;
        ctx->sBiasTables.aui32InterBias_P[n] = iInterMBBias;
        ctx->sBiasTables.aui32DirectBias_P[n] = uDirectVecBias;
    }

    if(psBiasParams->bRCEnable || psBiasParams->bRCBiases)
        ctx->sBiasTables.ui32sz1 = psBiasParams->uisz1;
    else
        ctx->sBiasTables.ui32sz1 = psBiasParams->uisz2;
}

/**************************************************************************************************
* Function:		H263_GenerateBiasTables
* Description:	Genereate the bias tables for H.263
*
***************************************************************************************************/
void
tng__H263ES_generate_bias_tables(
	context_ENC_p ctx)
{
    IMG_INT16 n;
    IMG_INT16 iX;
    IMG_UINT32 ui32RegVal;
    IMG_UINT8 uiDCScaleL,uiDCScaleC,uiLambda;
    IMG_UINT32 uDirectVecBias,iInterMBBias,iIntra16Bias;
    IMG_BIAS_PARAMS * psBiasParams = &(ctx->sBiasParams);

    ctx->sBiasTables.ui32LritcCacheChunkConfig = F_ENCODE(ctx->uChunksPerMb, INTEL_CH_PM) |
						 F_ENCODE(ctx->uMaxChunks, INTEL_CH_MX) |
						 F_ENCODE(ctx->uMaxChunks - ctx->uPriorityChunks, INTEL_CH_PY);

    for(n=31;n>=1;n--)
    {
	iX = n-12;
	if(iX < 0)
	{
	    iX = 0;
	}
	// Dont Write QP Values To ESB -- IPE will write these values
	// Update the quantization parameter which includes doing Lamda and the Chroma QP

	uiDCScaleL	= CalculateDCScaler(n, IMG_FALSE);
	uiDCScaleC	= CalculateDCScaler(n, IMG_TRUE);
	uiLambda	= psBiasParams->uLambdaSAD ? psBiasParams->uLambdaSAD : H263_QPLAMBDA_MAP[n - 1];

	ui32RegVal=uiDCScaleL;
	ui32RegVal |= (uiDCScaleC)<<8;
	ui32RegVal |= (uiLambda)<<16;

   	ctx->sBiasTables.aui32LambdaBias[n] = ui32RegVal;
    }

    for(n=31;n>=1;n-=2) {
        if(psBiasParams->bRCEnable || psBiasParams->bRCBiases) {
            uDirectVecBias = psBiasParams->uTHSkipIPE * uiLambda;
            iInterMBBias    = psBiasParams->uTHInter * (n - psBiasParams->uTHInterQP);
            //if(iInterMBBias < 0)
            //    iInterMBBias	= 0;
            if(iInterMBBias > psBiasParams->uTHInterMaxLevel)
                iInterMBBias	= psBiasParams->uTHInterMaxLevel;
            iIntra16Bias = 0;
        } else {
            uDirectVecBias  = psBiasParams->uIPESkipVecBias;
            iInterMBBias    = psBiasParams->iInterMBBias;
            iIntra16Bias    = psBiasParams->iIntra16Bias;
        }

        ctx->sBiasTables.aui32IntraBias[n] = iIntra16Bias;
        ctx->sBiasTables.aui32InterBias_P[n] = iInterMBBias;
        ctx->sBiasTables.aui32DirectBias_P[n] = uDirectVecBias;
    }

    if(psBiasParams->bRCEnable || psBiasParams->bRCBiases)
        ctx->sBiasTables.ui32sz1 = psBiasParams->uisz1;
    else
        ctx->sBiasTables.ui32sz1 = psBiasParams->uisz2;
}

/**************************************************************************************************
* Function:             H264_GenerateBiasTables
* Description:  Generate the bias tables for H.264
*
***************************************************************************************************/
static void tng__H264ES_generate_bias_tables(context_ENC_p ctx)
{
    IMG_INT32 n;
    IMG_UINT32 ui32RegVal;
    IMG_UINT32 iIntra16Bias, uisz2, uIntra8Scale, uDirectVecBias_P, iInterMBBias_P, uDirectVecBias_B, iInterMBBias_B;
    IMG_BIAS_PARAMS * psBiasParams = &(ctx->sBiasParams);

    IMG_BYTE PVR_QP_SCALE_CR[52] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
        12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
        28, 29, 29, 30, 31, 32, 32, 33, 34, 34, 35, 35, 36, 36, 37, 37,
        37, 38, 38, 38, 39, 39, 39, 39
    };

    ctx->sBiasTables.ui32LritcCacheChunkConfig = 
        F_ENCODE(ctx->uChunksPerMb, INTEL_CH_PM) |
        F_ENCODE(ctx->uMaxChunks, INTEL_CH_MX) |
        F_ENCODE(ctx->uMaxChunks - ctx->uPriorityChunks, INTEL_CH_PY);

    uIntra8Scale = 0;
    for (n = 51; n >= 0; n--) {
        IMG_INT32  iX;
        IMG_UINT32 uiLambdaSAD, uiLambdaSATD;

        iX = n - 12;
        if (iX < 0) iX = 0;

        uiLambdaSAD  = H264_QPLAMBDA_MAP_SAD[iX];
        uiLambdaSATD = H264_QPLAMBDA_MAP_SATD_TABLES[psBiasParams->uLambdaSATDTable][n];

        if (psBiasParams->uLambdaSAD  != 0) uiLambdaSAD  = psBiasParams->uLambdaSAD;
        if (psBiasParams->uLambdaSATD != 0) uiLambdaSATD = psBiasParams->uLambdaSATD;

        // Dont Write QP Values To ESB -- IPE will write these values
        // Update the quantization parameter which includes doing Lamda and the Chroma QP
        ui32RegVal = PVR_QP_SCALE_CR[n];
        ui32RegVal |= (uiLambdaSATD) << 8; //SATD lambda
        ui32RegVal |= (uiLambdaSAD) << 16; //SAD lambda

        ctx->sBiasTables.aui32LambdaBias[n] = ui32RegVal;
    }

    for (n = 52; n >= 0; n -= 2) {
        IMG_INT8 qp = n;
        if (qp > 51) qp = 51;

        if (psBiasParams->bRCEnable || psBiasParams->bRCBiases) {
            iInterMBBias_P  = tng__H264ES_inter_bias(qp);
            uDirectVecBias_P  = H264_DIRECT_BIAS[n/2];

            iInterMBBias_B  = iInterMBBias_P;
            uDirectVecBias_B  = uDirectVecBias_P  ;

            iIntra16Bias    = 0;
            uIntra8Scale    = H264_INTRA8_SCALE[n/2] - 8;

            if (psBiasParams->uDirectVecBias != UNINIT_PARAM)
                uDirectVecBias_B  = psBiasParams->uDirectVecBias;
            if (psBiasParams->iInterMBBiasB != UNINIT_PARAM)
                iInterMBBias_B    = psBiasParams->iInterMBBiasB;

            if (psBiasParams->uIPESkipVecBias != UNINIT_PARAM)
                uDirectVecBias_P  = psBiasParams->uIPESkipVecBias;
            if (psBiasParams->iInterMBBias != UNINIT_PARAM)
                iInterMBBias_P    = psBiasParams->iInterMBBias;

            if (psBiasParams->iIntra16Bias != UNINIT_PARAM) iIntra16Bias    = psBiasParams->iIntra16Bias;
        } else {
            if (psBiasParams->uDirectVecBias == UNINIT_PARAM || psBiasParams->iInterMBBiasB == UNINIT_PARAM) {
                drv_debug_msg(VIDEO_DEBUG_GENERAL, "ERROR: Bias for B pictures not set up ... uDirectVecBias = 0x%x, iInterMBBiasB = 0x%x\n", psBiasParams->uDirectVecBias, psBiasParams->iInterMBBiasB);
                abort();
            }
            uDirectVecBias_B  = psBiasParams->uDirectVecBias;
            iInterMBBias_B    = psBiasParams->iInterMBBiasB;

            if (psBiasParams->uIPESkipVecBias == UNINIT_PARAM || psBiasParams->iInterMBBias == UNINIT_PARAM) {
                drv_debug_msg(VIDEO_DEBUG_GENERAL, "ERROR: Bias for I/P pictures not set up ... uIPESkipVecBias = 0x%x, iInterMBBias = 0x%x\n", psBiasParams->uIPESkipVecBias, psBiasParams->iInterMBBias);
                abort();
            }
            uDirectVecBias_P  = psBiasParams->uIPESkipVecBias;
            iInterMBBias_P    = psBiasParams->iInterMBBias;

            iIntra16Bias    = psBiasParams->iIntra16Bias;
            uisz2   = psBiasParams->uisz2;
        }

#ifdef BRN_30029
        //adjust the intra8x8 bias so that we don't do anything silly when 8x8 mode is not in use.
        if (ctx->ui32PredCombControl & F_ENCODE(1, TOPAZHP_CR_INTRA8X8_DISABLE)) {
            iIntra16Bias |= 0x7fff << 16;
        }
#endif
//      drv_debug_msg(VIDEO_DEBUG_GENERAL, "qp %d, iIntra16Bias %d, iInterMBBias %d, uDirectVecBias %d\n", qp, iIntra16Bias, iInterMBBias, uDirectVecBias);
        ctx->sBiasTables.aui32IntraBias[n] = iIntra16Bias;
        ctx->sBiasTables.aui32InterBias_P[n] = iInterMBBias_P;
        ctx->sBiasTables.aui32DirectBias_P[n] = uDirectVecBias_P;
        ctx->sBiasTables.aui32InterBias_B[n] = iInterMBBias_B;
        ctx->sBiasTables.aui32DirectBias_B[n] = uDirectVecBias_B;
        ctx->sBiasTables.aui32IntraScale[n] = uIntra8Scale;
    }

    if (psBiasParams->bRCEnable || psBiasParams->bRCBiases)
        ctx->sBiasTables.ui32sz1 = psBiasParams->uisz1;
    else
        ctx->sBiasTables.ui32sz1 = psBiasParams->uisz2;

    if (psBiasParams->bZeroDetectionDisable) {
        ctx->sBiasTables.ui32RejectThresholdH264 = F_ENCODE(0, INTEL_H264_ConfigReg1)
                | F_ENCODE(0, INTEL_H264_ConfigReg2);
    } else {
        ctx->sBiasTables.ui32RejectThresholdH264 = F_ENCODE(psBiasParams->uzb4, INTEL_H264_ConfigReg1)
                | F_ENCODE(psBiasParams->uzb8, INTEL_H264_ConfigReg2);
    }
}


/**************************************************************************************************
* Function:             VIDEO_GenerateBias
* Description:  Generate the bias tables
*
***************************************************************************************************/
VAStatus tng__generate_bias(context_ENC_p ctx)
{
    assert(ctx);

    switch (ctx->eStandard) {
        case IMG_STANDARD_H264:
            tng__H264ES_generate_bias_tables(ctx);
            break;
        case IMG_STANDARD_H263:
            tng__H263ES_generate_bias_tables(ctx);
            break;
        case IMG_STANDARD_MPEG4:
	    tng__MPEG4ES_generate_bias_tables(ctx);
	    break;
/*
        case IMG_STANDARD_MPEG2:
                MPEG2_GenerateBiasTables(ctx, psBiasParams);
                break;
*/
        default:
            break;
    }

    return VA_STATUS_SUCCESS;
}

//load bias
static IMG_INT H264_LAMBDA_COEFFS[4][3] = {
	{175, -10166, 163244 },	 //SATD Lambda High
	{ 16,   -236,   8693 },  //SATD Lambda Low
	{198, -12240, 198865 },	 //SAD Lambda High
	{ 12,   -176,   1402 },  //SAD Lambda Low
};

static IMG_INT MPEG4_LAMBDA_COEFFS[3] = {
	0, 458, 1030
};

static IMG_INT H263_LAMBDA_COEFFS[3] = {
	0, 333, 716
};

static void tng__H263ES_load_bias_tables(
    context_ENC_p ctx,
    IMG_FRAME_TYPE __maybe_unused eFrameType)
{
    IMG_INT32 n;
    IMG_UINT32 ui32RegVal;
    IMG_UINT32 count = 0, cmd_word = 0;
    tng_cmdbuf_p cmdbuf = ctx->obj_context->tng_cmdbuf;
    IMG_BIAS_TABLES* psBiasTables = &(ctx->sBiasTables);
    IMG_UINT32 *pCount;
    IMG_UINT32 ui8Pipe;

    cmd_word = (MTX_CMDID_SW_WRITEREG & MTX_CMDWORD_ID_MASK) << MTX_CMDWORD_ID_SHIFT;
    *cmdbuf->cmd_idx++ = cmd_word;
    pCount = cmdbuf->cmd_idx;
    cmdbuf->cmd_idx++;

    ctx->ui32CoreRev = 0x00030401;

    for (ui8Pipe = 0; ui8Pipe < ctx->ui8PipesToUse; ui8Pipe++)
        tng_cmdbuf_insert_reg_write(TOPAZ_CORE_REG, TOPAZHP_CR_SEQUENCER_CONFIG, 0, psBiasTables->ui32SeqConfigInit);

    if (ctx->ui32CoreRev <= MAX_32_REV)
    {
	for(n=31;n>=1;n--)
	{
	    //FIXME: Zhaohan, missing register TOPAZHP_TOP_CR_LAMBDA_DC_TABLE
	    //tng_cmdbuf_insert_reg_write(TOPAZHP_TOP_CR_LAMBDA_DC_TABLE, 0, psBiasTables->aui32LambdaBias[n]);
	}
    } else {
	ui32RegVal = (((H263_LAMBDA_COEFFS[0]) << (SHIFT_TOPAZHP_TOP_CR_POLYNOM_ALPHA_COEFF_CORE_00)) & (MASK_TOPAZHP_TOP_CR_POLYNOM_ALPHA_COEFF_CORE_00));
	ui32RegVal |= (((H263_LAMBDA_COEFFS[1]) << (SHIFT_TOPAZHP_TOP_CR_POLYNOM_BETA_COEFF_CORE_00)) & (MASK_TOPAZHP_TOP_CR_POLYNOM_BETA_COEFF_CORE_00));
	tng_cmdbuf_insert_reg_write(TOPAZ_MULTICORE_REG, TOPAZHP_TOP_CR_POLYNOM_ALPHA_COEFF_CORE_0, 0, ui32RegVal);

	ui32RegVal = (((H263_LAMBDA_COEFFS[2]) << (SHIFT_TOPAZHP_TOP_CR_POLYNOM_GAMMA_COEFF_CORE_01)) & (MASK_TOPAZHP_TOP_CR_POLYNOM_GAMMA_COEFF_CORE_01));
	tng_cmdbuf_insert_reg_write(TOPAZ_MULTICORE_REG, TOPAZHP_TOP_CR_POLYNOM_GAMMA_COEFF_CORE_0, 0, ui32RegVal);

	ui32RegVal = 0x3f;
	tng_cmdbuf_insert_reg_write(TOPAZ_MULTICORE_REG, TOPAZHP_TOP_CR_POLYNOM_CUTOFF_CORE_0, 0, ui32RegVal);
    }

    for(n=31;n>=1;n-=2)
    {
	tng_cmdbuf_insert_reg_write(TOPAZ_MULTICORE_REG, TOPAZHP_TOP_CR_INTRA_BIAS_TABLE, 0, psBiasTables->aui32IntraBias[n]);
	tng_cmdbuf_insert_reg_write(TOPAZ_MULTICORE_REG, TOPAZHP_TOP_CR_INTER_BIAS_TABLE, 0, psBiasTables->aui32InterBias_P[n]);
	tng_cmdbuf_insert_reg_write(TOPAZ_MULTICORE_REG, TOPAZHP_TOP_CR_DIRECT_BIAS_TABLE, 0, psBiasTables->aui32DirectBias_P[n]);
    }

    for (ui8Pipe = 0; ui8Pipe < ctx->ui8PipesToUse; ui8Pipe++) {
	tng_cmdbuf_insert_reg_write(TOPAZ_CORE_REG, INTEL_SZ, 0, psBiasTables->ui32sz1);
    }

    for (ui8Pipe = 0; ui8Pipe < ctx->ui8PipesToUse; ui8Pipe++)
        tng_cmdbuf_insert_reg_write(TOPAZ_CORE_REG, INTEL_CHCF, 0, psBiasTables->ui32LritcCacheChunkConfig);

    *pCount = count;
}

static void tng__MPEG4_load_bias_tables(context_ENC_p ctx)
{
    IMG_INT32 n;
    IMG_UINT32 ui32RegVal;
    IMG_UINT32 count = 0, cmd_word = 0;
    tng_cmdbuf_p cmdbuf = ctx->obj_context->tng_cmdbuf;
    IMG_BIAS_TABLES* psBiasTables = &(ctx->sBiasTables);
    IMG_UINT32 *pCount;
    IMG_UINT8 ui8Pipe;

    cmd_word = (MTX_CMDID_SW_WRITEREG & MTX_CMDWORD_ID_MASK) << MTX_CMDWORD_ID_SHIFT;
    *cmdbuf->cmd_idx++ = cmd_word;
    pCount = cmdbuf->cmd_idx;
    cmdbuf->cmd_idx++;

    ctx->ui32CoreRev = 0x00030401;

    for (ui8Pipe = 0; ui8Pipe < ctx->ui8PipesToUse; ui8Pipe++)
        tng_cmdbuf_insert_reg_write(TOPAZ_CORE_REG, TOPAZHP_CR_SEQUENCER_CONFIG, 0, psBiasTables->ui32SeqConfigInit);

    if (ctx->ui32CoreRev <= MAX_32_REV) {
        for (n=31; n >= 1; n--) {
	    //FIXME: Zhaohan, missing register TOPAZHP_TOP_CR_LAMBDA_DC_TABLE
            //tng_cmdbuf_insert_reg_write(TOPAZHP_TOP_CR_LAMBDA_DC_TABLE, 0, psBiasTables->aui32LambdaBias[n]);
        }
    } else {
	    //ui32RegVal = MPEG4_LAMBDA_COEFFS[0]| (MPEG4_LAMBDA_COEFFS[1]<<8);
	    ui32RegVal = (((MPEG4_LAMBDA_COEFFS[0]) << (SHIFT_TOPAZHP_TOP_CR_POLYNOM_ALPHA_COEFF_CORE_00)) & (MASK_TOPAZHP_TOP_CR_POLYNOM_ALPHA_COEFF_CORE_00));
	    ui32RegVal |= (((MPEG4_LAMBDA_COEFFS[1]) << (SHIFT_TOPAZHP_TOP_CR_POLYNOM_BETA_COEFF_CORE_00)) & (MASK_TOPAZHP_TOP_CR_POLYNOM_BETA_COEFF_CORE_00));

	    tng_cmdbuf_insert_reg_write(TOPAZ_MULTICORE_REG, TOPAZHP_TOP_CR_POLYNOM_ALPHA_COEFF_CORE_0, 0, ui32RegVal);
	    //ui32RegVal = MPEG4_LAMBDA_COEFFS[2];
	    ui32RegVal = (((MPEG4_LAMBDA_COEFFS[2]) << (SHIFT_TOPAZHP_TOP_CR_POLYNOM_GAMMA_COEFF_CORE_01)) & (MASK_TOPAZHP_TOP_CR_POLYNOM_GAMMA_COEFF_CORE_01));

	    tng_cmdbuf_insert_reg_write(TOPAZ_MULTICORE_REG, TOPAZHP_TOP_CR_POLYNOM_GAMMA_COEFF_CORE_0, 0, ui32RegVal);	
	
	    ui32RegVal = 0x3f;
	    tng_cmdbuf_insert_reg_write(TOPAZ_MULTICORE_REG, TOPAZHP_TOP_CR_POLYNOM_CUTOFF_CORE_0, 0, ui32RegVal);
    }

    for(n=31;n>=1;n-=2)
    {
	tng_cmdbuf_insert_reg_write(TOPAZ_MULTICORE_REG, TOPAZHP_TOP_CR_INTRA_BIAS_TABLE, 0, psBiasTables->aui32IntraBias[n]);
	tng_cmdbuf_insert_reg_write(TOPAZ_MULTICORE_REG, TOPAZHP_TOP_CR_INTER_BIAS_TABLE, 0, psBiasTables->aui32InterBias_P[n]);
	tng_cmdbuf_insert_reg_write(TOPAZ_MULTICORE_REG, TOPAZHP_TOP_CR_DIRECT_BIAS_TABLE, 0, psBiasTables->aui32DirectBias_P[n]);
    }

    for (ui8Pipe = 0; ui8Pipe < ctx->ui8PipesToUse; ui8Pipe++) {
	tng_cmdbuf_insert_reg_write(TOPAZ_CORE_REG, INTEL_SZ, 0, psBiasTables->ui32sz1);
		
	//VLC RSize is fcode - 1 and only done for mpeg4 AND mpeg2 not H263
	tng_cmdbuf_insert_reg_write(TOPAZ_VLC_REG, TOPAZ_VLC_CR_VLC_MPEG4_CFG, 0, F_ENCODE(psBiasTables->ui32FCode - 1, TOPAZ_VLC_CR_RSIZE));
    }

    for (ui8Pipe = 0; ui8Pipe < ctx->ui8PipesToUse; ui8Pipe++)
        tng_cmdbuf_insert_reg_write(TOPAZ_CORE_REG, INTEL_CHCF, 0, psBiasTables->ui32LritcCacheChunkConfig);

    *pCount = count;
}

static void tng__H264ES_load_bias_tables(
    context_ENC_p ctx,
    IMG_FRAME_TYPE eFrameType)
{
    IMG_INT32 n;
    IMG_UINT32 ui32RegVal;
    IMG_BIAS_TABLES* psBiasTables = &(ctx->sBiasTables);
    tng_cmdbuf_p cmdbuf = ctx->obj_context->tng_cmdbuf;
    IMG_UINT32 count = 0, cmd_word = 0;
    IMG_UINT32 *pCount;
    IMG_UINT32 ui8Pipe;
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: start\n", __FUNCTION__);

    cmd_word = (MTX_CMDID_SW_WRITEREG & MTX_CMDWORD_ID_MASK) << MTX_CMDWORD_ID_SHIFT;
    *cmdbuf->cmd_idx++ = cmd_word;
    pCount = cmdbuf->cmd_idx;
    cmdbuf->cmd_idx++;

    psBiasTables->ui32SeqConfigInit = 0x40038412;

    for (ui8Pipe = 0; ui8Pipe < ctx->ui8PipesToUse; ui8Pipe++)
        tng_cmdbuf_insert_reg_write(TOPAZ_CORE_REG, TOPAZHP_CR_SEQUENCER_CONFIG, 0, psBiasTables->ui32SeqConfigInit);

    ctx->ui32CoreRev = 0x00030401;
    
    if (ctx->ui32CoreRev <= MAX_32_REV) {
        for (n=51; n >= 0; n--) {
	    //FIXME: Zhaohan, missing register TOPAZHP_TOP_CR_LAMBDA_DC_TABLE
            //tng_cmdbuf_insert_reg_write(TOPAZHP_TOP_CR_LAMBDA_DC_TABLE, 0, psBiasTables->aui32LambdaBias[n]);
        }
    } else {
        //Load the lambda coeffs
        for (n = 0; n < 4; n++) {
            //ui32RegVal = H264_LAMBDA_COEFFS[n][0]| (H264_LAMBDA_COEFFS[n][1]<<8);
            ui32RegVal = (((H264_LAMBDA_COEFFS[n][0]) << (SHIFT_TOPAZHP_TOP_CR_POLYNOM_ALPHA_COEFF_CORE_00)) & (MASK_TOPAZHP_TOP_CR_POLYNOM_ALPHA_COEFF_CORE_00));
            ui32RegVal |= (((H264_LAMBDA_COEFFS[n][1]) << (SHIFT_TOPAZHP_TOP_CR_POLYNOM_BETA_COEFF_CORE_00)) & (MASK_TOPAZHP_TOP_CR_POLYNOM_BETA_COEFF_CORE_00));

            tng_cmdbuf_insert_reg_write(TOPAZ_MULTICORE_REG, TOPAZHP_TOP_CR_POLYNOM_ALPHA_COEFF_CORE_0, 0, ui32RegVal);
            tng_cmdbuf_insert_reg_write(TOPAZ_MULTICORE_REG, TOPAZHP_TOP_CR_POLYNOM_ALPHA_COEFF_CORE_1, 0, ui32RegVal);
            tng_cmdbuf_insert_reg_write(TOPAZ_MULTICORE_REG, TOPAZHP_TOP_CR_POLYNOM_ALPHA_COEFF_CORE_2, 0, ui32RegVal);

            //ui32RegVal = H264_LAMBDA_COEFFS[n][2];
            ui32RegVal = (((H264_LAMBDA_COEFFS[n][2]) << (SHIFT_TOPAZHP_TOP_CR_POLYNOM_GAMMA_COEFF_CORE_01)) & (MASK_TOPAZHP_TOP_CR_POLYNOM_GAMMA_COEFF_CORE_01));

            tng_cmdbuf_insert_reg_write(TOPAZ_MULTICORE_REG, TOPAZHP_TOP_CR_POLYNOM_GAMMA_COEFF_CORE_0, 0, ui32RegVal);
            tng_cmdbuf_insert_reg_write(TOPAZ_MULTICORE_REG, TOPAZHP_TOP_CR_POLYNOM_GAMMA_COEFF_CORE_1, 0, ui32RegVal);
            tng_cmdbuf_insert_reg_write(TOPAZ_MULTICORE_REG, TOPAZHP_TOP_CR_POLYNOM_GAMMA_COEFF_CORE_2, 0, ui32RegVal);
        }
        ui32RegVal = 29 |(29<<6);
        tng_cmdbuf_insert_reg_write(TOPAZ_MULTICORE_REG, TOPAZHP_TOP_CR_POLYNOM_CUTOFF_CORE_0, 0, ui32RegVal);
        tng_cmdbuf_insert_reg_write(TOPAZ_MULTICORE_REG, TOPAZHP_TOP_CR_POLYNOM_CUTOFF_CORE_1, 0, ui32RegVal);
        tng_cmdbuf_insert_reg_write(TOPAZ_MULTICORE_REG, TOPAZHP_TOP_CR_POLYNOM_CUTOFF_CORE_2, 0, ui32RegVal);
    }

    for (n=52;n>=0;n-=2) {
        if (eFrameType == IMG_INTER_B) {
            tng_cmdbuf_insert_reg_write(TOPAZ_MULTICORE_REG, TOPAZHP_TOP_CR_INTER_BIAS_TABLE, 0, psBiasTables->aui32InterBias_B[n]);
            tng_cmdbuf_insert_reg_write(TOPAZ_MULTICORE_REG, TOPAZHP_TOP_CR_DIRECT_BIAS_TABLE, 0, psBiasTables->aui32DirectBias_B[n]);
        } else {
            tng_cmdbuf_insert_reg_write(TOPAZ_MULTICORE_REG, TOPAZHP_TOP_CR_INTER_BIAS_TABLE, 0, psBiasTables->aui32InterBias_P[n]);
            tng_cmdbuf_insert_reg_write(TOPAZ_MULTICORE_REG, TOPAZHP_TOP_CR_DIRECT_BIAS_TABLE, 0, psBiasTables->aui32DirectBias_P[n]);
        }
        tng_cmdbuf_insert_reg_write(TOPAZ_MULTICORE_REG, TOPAZHP_TOP_CR_INTRA_BIAS_TABLE, 0, psBiasTables->aui32IntraBias[n]);
        tng_cmdbuf_insert_reg_write(TOPAZ_MULTICORE_REG, TOPAZHP_TOP_CR_INTRA_SCALE_TABLE, 0, psBiasTables->aui32IntraScale[n]);
    }

    //aui32HpCoreRegId[ui32Pipe]
    for (ui8Pipe = 0; ui8Pipe < ctx->ui8PipesToUse; ui8Pipe++) {
        tng_cmdbuf_insert_reg_write(TOPAZ_CORE_REG, INTEL_SZ, 0, psBiasTables->ui32sz1);
        tng_cmdbuf_insert_reg_write(TOPAZ_CORE_REG, INTEL_H264_RT, 0, psBiasTables->ui32RejectThresholdH264);
    }

//    tng_cmdbuf_insert_reg_write(TOPAZHP_TOP_CR_FIRMWARE_REG_1, (MTX_SCRATCHREG_TOMTX<<2), ui32BuffersReg);
//    tng_cmdbuf_insert_reg_write(TOPAZHP_TOP_CR_FIRMWARE_REG_1, (MTX_SCRATCHREG_TOHOST<<2),ui32ToHostReg);

    // now setup the LRITC chache priority
    {
        //aui32HpCoreRegId[ui32Pipe]
        for (ui8Pipe = 0; ui8Pipe < ctx->ui8PipesToUse; ui8Pipe++) {
            tng_cmdbuf_insert_reg_write(TOPAZ_CORE_REG, INTEL_CHCF, 0, psBiasTables->ui32LritcCacheChunkConfig);
        }
    }
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: count = %d\n", __FUNCTION__, (int)count);

    *pCount = count;
}

VAStatus tng_load_bias(context_ENC_p ctx, IMG_FRAME_TYPE eFrameType)
{
    IMG_STANDARD eStandard = ctx->eStandard;

    switch (eStandard) {
        case IMG_STANDARD_H264:
            tng__H264ES_load_bias_tables(ctx, eFrameType); //IMG_INTER_P);
            break;
        case IMG_STANDARD_H263:
            tng__H263ES_load_bias_tables(ctx, eFrameType); //IMG_INTER_P);
            break;
        case IMG_STANDARD_MPEG4:
            tng__MPEG4_load_bias_tables(ctx);
            break;
/*
        case IMG_STANDARD_MPEG2:
            tng__MPEG2_LoadBiasTables(psBiasTables);
            break;
*/
        default:
            break;
    }

    return VA_STATUS_SUCCESS;
}

void tng_init_bias_params(context_ENC_p ctx)
{
    IMG_BIAS_PARAMS * psBiasParams = &(ctx->sBiasParams);
    memset(psBiasParams, 0, sizeof(IMG_BIAS_PARAMS));
    //default
    psBiasParams->uLambdaSAD = 0;   
    psBiasParams->uLambdaSATD = 0;
    psBiasParams->uLambdaSATDTable = 0;

    psBiasParams->bRCEnable = ctx->sRCParams.bRCEnable;
    psBiasParams->bRCBiases = IMG_TRUE;

    psBiasParams->iIntra16Bias = UNINIT_PARAM;
    psBiasParams->iInterMBBias = UNINIT_PARAM;
    psBiasParams->iInterMBBiasB = UNINIT_PARAM;

    psBiasParams->uDirectVecBias = UNINIT_PARAM;
    psBiasParams->uIPESkipVecBias = UNINIT_PARAM;
    psBiasParams->uSPESkipVecBias = 0;      //not in spec

    psBiasParams->uisz1 = 6;
    psBiasParams->uisz2 = 6;
    psBiasParams->bZeroDetectionDisable = TOPAZHP_DEFAULT_bZeroDetectionDisable;

    psBiasParams->uzb4 = 6;
    psBiasParams->uzb8 = 4;

    psBiasParams->uTHInter = TOPAZHP_DEFAULT_uTHInter;
    psBiasParams->uTHInterQP = TOPAZHP_DEFAULT_uTHInterQP;
    psBiasParams->uTHInterMaxLevel = TOPAZHP_DEFAULT_uTHInterMaxLevel;
    psBiasParams->uTHSkipIPE = TOPAZHP_DEFAULT_uTHSkipIPE;
    psBiasParams->uTHSkipSPE = TOPAZHP_DEFAULT_uTHSkipSPE;
}
