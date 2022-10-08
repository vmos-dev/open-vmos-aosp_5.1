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
 *    Edward Lin <edward.lin@intel.com>
 *
 */

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>

#include "hwdefs/coreflags.h"
#include "hwdefs/topaz_vlc_regs.h"
#include "hwdefs/topaz_db_regs.h"

#include "va/va_enc_h264.h"

#include "psb_def.h"
#include "psb_drv_debug.h"
#include "psb_surface.h"
#include "tng_cmdbuf.h"
#include "tng_hostcode.h"
#include "tng_hostheader.h"
#include "tng_picmgmt.h"
#include "tng_slotorder.h"
#include "tng_hostair.h"
#include "tng_H264ES.h"
#ifdef _TOPAZHP_PDUMP_
#include "tng_trace.h"
#endif

#define TOPAZ_H264_MAX_BITRATE 135000000

#define INIT_CONTEXT_H264ES     context_ENC_p ctx = (context_ENC_p) obj_context->format_data
#define SURFACE(id)    ((object_surface_p) object_heap_lookup( &ctx->obj_context->driver_data->surface_heap, id ))
#define BUFFER(id)  ((object_buffer_p) object_heap_lookup( &ctx->obj_context->driver_data->buffer_heap, id ))

static VAStatus tng__H264ES_init_profile(
    object_context_p obj_context,
    object_config_p obj_config)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    context_ENC_p ctx;
    ctx = (context_ENC_p) obj_context->format_data;
    switch (obj_config->profile) {
        case VAProfileH264Baseline:
        case VAProfileH264ConstrainedBaseline:
            ctx->ui8ProfileIdc = H264ES_PROFILE_BASELINE;
            break;
        case VAProfileH264Main:
            ctx->ui8ProfileIdc = H264ES_PROFILE_MAIN;
            break;
        case VAProfileH264High:
            ctx->ui8ProfileIdc = H264ES_PROFILE_HIGH;
            break;
        case VAProfileH264StereoHigh:
            ctx->ui8ProfileIdc = H264ES_PROFILE_HIGH;
            ctx->bEnableMVC = 1;
            ctx->ui16MVCViewIdx = 0;
            break;
        default:
            ctx->ui8ProfileIdc = H264ES_PROFILE_BASELINE;
            vaStatus = VA_STATUS_ERROR_UNSUPPORTED_PROFILE;
            break;
    }
    drv_debug_msg(VIDEO_DEBUG_GENERAL,
        "%s: obj_config->profile = %dctx->eStandard = %d, ctx->bEnableMVC = %d\n",
        __FUNCTION__, obj_config->profile, ctx->eStandard, ctx->bEnableMVC);
    return vaStatus;
}

static VAStatus tng__H264ES_get_codec_type(
    object_context_p obj_context,
    object_config_p __maybe_unused obj_config)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    context_ENC_p ctx = (context_ENC_p) obj_context->format_data;
    IMG_RC_PARAMS *psRCParams = &(ctx->sRCParams);

    ctx->eCodec = IMG_CODEC_NUM;

    if (ctx->bEnableMVC) {
        switch (psRCParams->eRCMode) {
            case IMG_RCMODE_NONE:
                ctx->eCodec = IMG_CODEC_H264MVC_NO_RC;
                break;
            case IMG_RCMODE_CBR:
                ctx->eCodec = IMG_CODEC_H264MVC_CBR;
                break;
            case IMG_RCMODE_VBR:
                ctx->eCodec = IMG_CODEC_H264MVC_VBR;
                break;
            default:
                drv_debug_msg(VIDEO_DEBUG_ERROR, "RC mode MVC\n");
                break;
        }
    } else {
        switch (psRCParams->eRCMode) {
            case IMG_RCMODE_NONE:
                ctx->eCodec = IMG_CODEC_H264_NO_RC;
                break;
            case IMG_RCMODE_CBR:
                ctx->eCodec = IMG_CODEC_H264_CBR;
                break;
            case IMG_RCMODE_VBR:
                ctx->eCodec = IMG_CODEC_H264_VBR;
                break;
            case IMG_RCMODE_VCM:
                ctx->eCodec = IMG_CODEC_H264_VCM;
                break;
            default:
                drv_debug_msg(VIDEO_DEBUG_ERROR, "RC mode\n");
                break;
        }
    }
    return vaStatus;
}

static VAStatus tng__H264ES_init_format_mode(
    object_context_p obj_context,
    object_config_p __maybe_unused obj_config)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    context_ENC_p ctx = (context_ENC_p) obj_context->format_data;

    ctx->bIsInterlaced = IMG_FALSE;
    ctx->bIsInterleaved = IMG_FALSE;
    ctx->ui16PictureHeight = ctx->ui16FrameHeight;
    ctx->eCodec = IMG_CODEC_H264_NO_RC;
    return vaStatus;
}

static void tng__H264ES_init_context(object_context_p obj_context,
    object_config_p __maybe_unused obj_config)
{
    context_ENC_p ctx = (context_ENC_p) obj_context->format_data;
    //This parameter need not be exposed
    ctx->ui8InterIntraIndex = 3;
    ctx->ui8CodedSkippedIndex = 3;
    ctx->bEnableHostQP = IMG_FALSE;
    ctx->uMaxChunks = 0xA0;
    ctx->uChunksPerMb = 0x40;
    ctx->uPriorityChunks = (0xA0 - 0x60);
    ctx->ui32FCode = 4;
    ctx->iFineYSearchSize = 2;

    //This parameter need not be exposed
    //host to control the encoding process
    ctx->bEnableInpCtrl = IMG_FALSE;
    ctx->bEnableHostBias = IMG_FALSE;
    //By default false Newly Added
    ctx->bEnableCumulativeBiases = IMG_FALSE;

    //Weighted Prediction is not supported in TopazHP Version 3.0
    ctx->bWeightedPrediction = IMG_FALSE;
    ctx->ui8VPWeightedImplicitBiPred = 0;
    ctx->bInsertHRDParams = IMG_FALSE;
    ctx->bArbitrarySO = IMG_FALSE;
    ctx->ui32BasicUnit = 0;
    ctx->idr_force_flag = 0;
    ctx->bVPAdaptiveRoundingDisable = IMG_FALSE;
}

static VAStatus tng__H264ES_process_misc_framerate_param(context_ENC_p ctx, object_buffer_p obj_buffer)
{
    VAEncMiscParameterBuffer *pBuffer = (VAEncMiscParameterBuffer *) obj_buffer->buffer_data;
    VAEncMiscParameterFrameRate *psMiscFrameRateParam;
    IMG_RC_PARAMS *psRCParams = &(ctx->sRCParams);

    ASSERT(obj_buffer->type == VAEncMiscParameterTypeFrameRate);
    ASSERT(obj_buffer->size == sizeof(VAEncMiscParameterFrameRate));

    psMiscFrameRateParam = (VAEncMiscParameterFrameRate *)(pBuffer->data);

    if (psMiscFrameRateParam == NULL)
        return VA_STATUS_ERROR_INVALID_BUFFER;

    if (psMiscFrameRateParam->framerate < 1 || psMiscFrameRateParam->framerate > 65535)
        return VA_STATUS_ERROR_INVALID_PARAMETER;


    if (psRCParams->ui32FrameRate == 0)
        psRCParams->ui32FrameRate = psMiscFrameRateParam->framerate;
    else {
        if(psMiscFrameRateParam->framerate != psRCParams->ui32FrameRate){
	    if (psMiscFrameRateParam->framerate > psRCParams->ui32FrameRate)
		psRCParams->ui32BitsPerSecond /= (float)psMiscFrameRateParam->framerate / psRCParams->ui32FrameRate;
	    else
		psRCParams->ui32BitsPerSecond *= (float)psRCParams->ui32FrameRate / psMiscFrameRateParam->framerate;
            psRCParams->ui32FrameRate = psMiscFrameRateParam->framerate;
            ctx->rc_update_flag |= RC_MASK_frame_rate;
        }
    }

    return VA_STATUS_SUCCESS;
}

static VAStatus tng__H264ES_process_misc_ratecontrol_param(context_ENC_p ctx, object_buffer_p obj_buffer)
{
    IMG_INT32 ui32BitsPerFrame;
    VAEncMiscParameterRateControl *psMiscRcParams;
    IMG_RC_PARAMS *psRCParams = &(ctx->sRCParams);
    VAEncMiscParameterBuffer *pBuffer = (VAEncMiscParameterBuffer *) obj_buffer->buffer_data;
    unsigned int max_bps;

    ASSERT(obj_buffer->type == VAEncMiscParameterTypeRateControl);
    ASSERT(obj_buffer->size == sizeof(VAEncMiscParameterRateControl));
    psMiscRcParams = (VAEncMiscParameterRateControl *)pBuffer->data;

#ifdef _TOPAZHP_PDUMP_
    tng_H264ES_trace_misc_rc_params(psMiscRcParams);
#endif

    drv_debug_msg(VIDEO_DEBUG_GENERAL,
        "%s psRCParams->ui32BitsPerSecond = %d, psMiscRcParams->bits_per_second = %d \n",
        __FUNCTION__, psRCParams->ui32BitsPerSecond, psMiscRcParams->bits_per_second);

    if (psMiscRcParams->bits_per_second > TOPAZ_H264_MAX_BITRATE) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL,
            "%s: bits_per_second(%d) exceeds the maximum bitrate, set it with %d\n",
            __FUNCTION__, psMiscRcParams->bits_per_second, TOPAZ_H264_MAX_BITRATE);
        psMiscRcParams->bits_per_second = TOPAZ_H264_MAX_BITRATE;
    }

    if ((psRCParams->ui32BitsPerSecond != psMiscRcParams->bits_per_second) && 
        psMiscRcParams->bits_per_second != 0) {
        psRCParams->ui32BitsPerSecond = psMiscRcParams->bits_per_second;
	ctx->rc_update_flag |= RC_MASK_bits_per_second;
    }

    drv_debug_msg(VIDEO_DEBUG_GENERAL,
        "%s: rate control changed from %d to %d\n", __FUNCTION__,
        psRCParams->ui32BitsPerSecond, psMiscRcParams->bits_per_second);

    if (psMiscRcParams->rc_flags.value != 0) {
       if (psMiscRcParams->rc_flags.bits.disable_bit_stuffing)
           ctx->sRCParams.bDisableBitStuffing = IMG_TRUE;
       drv_debug_msg(VIDEO_DEBUG_GENERAL, "bDisableBitStuffing is %d\n",
           ctx->sRCParams.bDisableBitStuffing);
    }

    if (psMiscRcParams->window_size > 2000) {
	drv_debug_msg(VIDEO_DEBUG_ERROR, "window_size is too much!\n");
	return VA_STATUS_ERROR_INVALID_PARAMETER;
    }

    if (psMiscRcParams->window_size != 0)
        ctx->uiCbrBufferTenths = psMiscRcParams->window_size / 100;

    if (psRCParams->ui32FrameRate == 0)
        psRCParams->ui32FrameRate = 30;

    /* According to Table A-1 Level limits, if resolution is bigger than 625SD,
       min compression ratio is 4, otherwise min compression ratio is 2 */
    if (psRCParams->ui32BitsPerSecond == 0) {
	max_bps =  (ctx->obj_context->picture_width * ctx->obj_context->picture_height * 3 / 2 ) * 8 * psRCParams->ui32FrameRate;
	if (ctx->ui16SourceWidth > 720)
	    max_bps /= 4;
	else
	    max_bps /= 2;
	psRCParams->ui32BitsPerSecond = max_bps;
    }

    if (ctx->uiCbrBufferTenths) {
        psRCParams->ui32BufferSize = (IMG_UINT32)(psRCParams->ui32BitsPerSecond * ctx->uiCbrBufferTenths / 10.0);
    } else {
        if (psRCParams->ui32BitsPerSecond < 256000)
            psRCParams->ui32BufferSize = ((9 * psRCParams->ui32BitsPerSecond) >> 1);
        else
            psRCParams->ui32BufferSize = ((5 * psRCParams->ui32BitsPerSecond) >> 1);
    }

    drv_debug_msg(VIDEO_DEBUG_GENERAL,
        "%s ctx->uiCbrBufferTenths = %d, psRCParams->ui32BufferSize = %d\n",
        __FUNCTION__, ctx->uiCbrBufferTenths, psRCParams->ui32BufferSize);
    drv_debug_msg(VIDEO_DEBUG_GENERAL,
        "%s psRCParams->ui32BitsPerSecond = %d, psMiscRcParams->bits_per_second = %d\n",
        __FUNCTION__, psRCParams->ui32BitsPerSecond, psMiscRcParams->bits_per_second);

    //psRCParams->ui32BUSize = psMiscRcParams->basic_unit_size;
    psRCParams->i32InitialDelay = (13 * psRCParams->ui32BufferSize) >> 4;
    psRCParams->i32InitialLevel = (3 * psRCParams->ui32BufferSize) >> 4;

    ui32BitsPerFrame = psRCParams->ui32BitsPerSecond / psRCParams->ui32FrameRate;
    /* in order to minimise mismatches between firmware and external world InitialLevel should be a multiple of ui32BitsPerFrame */
    psRCParams->i32InitialLevel = ((psRCParams->i32InitialLevel + ui32BitsPerFrame / 2) / ui32BitsPerFrame) * ui32BitsPerFrame;
    psRCParams->i32InitialLevel = tng__max(psRCParams->i32InitialLevel, ui32BitsPerFrame);
    psRCParams->i32InitialDelay = psRCParams->ui32BufferSize - psRCParams->i32InitialLevel;

    //free(psMiscRcParams);
    if (psMiscRcParams->initial_qp > 51 ||
	psMiscRcParams->min_qp > 51 ||
	psMiscRcParams->max_qp > 51) {
        drv_debug_msg(VIDEO_DEBUG_ERROR,
            "%s: Initial_qp(%d) min_qp(%d) max_qp(%d) invalid.\nQP shouldn't be larger than 51 for H264\n",
            __FUNCTION__, psMiscRcParams->initial_qp, psMiscRcParams->min_qp, psMiscRcParams->max_qp);
        return VA_STATUS_ERROR_INVALID_PARAMETER;
    }

    if ((psRCParams->ui32InitialQp != psMiscRcParams->initial_qp) &&
	(psMiscRcParams->initial_qp != 0)) {
	drv_debug_msg(VIDEO_DEBUG_GENERAL,
	    "%s: initial_qp updated from %d to %d\n",
	    __FUNCTION__, psRCParams->ui32InitialQp, psMiscRcParams->initial_qp);
	ctx->rc_update_flag |= RC_MASK_initial_qp;
	psRCParams->ui32InitialQp = psMiscRcParams->initial_qp;
    }

    if ((psRCParams->iMinQP != psMiscRcParams->min_qp) &&
	(psMiscRcParams->min_qp != 0)) {
	drv_debug_msg(VIDEO_DEBUG_GENERAL,
	    "%s: min_qp updated from %d to %d\n",
	    __FUNCTION__, psRCParams->iMinQP, psMiscRcParams->min_qp);
	ctx->rc_update_flag |= RC_MASK_min_qp;
	psRCParams->iMinQP = psMiscRcParams->min_qp;
    }

    if ((ctx->max_qp != psMiscRcParams->max_qp) &&
	(psMiscRcParams->max_qp != 0)) {
	drv_debug_msg(VIDEO_DEBUG_GENERAL,
	    "%s: max_qp updated from %d to %d\n",
	    __FUNCTION__, ctx->max_qp, psMiscRcParams->max_qp);
	ctx->rc_update_flag |= RC_MASK_max_qp;
	ctx->max_qp = psMiscRcParams->max_qp;
    }

    return VA_STATUS_SUCCESS;
}

static VAStatus tng__H264ES_process_misc_hrd_param(context_ENC_p ctx, object_buffer_p obj_buffer)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAEncMiscParameterBuffer *pBuffer = (VAEncMiscParameterBuffer *) obj_buffer->buffer_data;
    VAEncMiscParameterHRD *psMiscHrdParams = NULL;
    IMG_RC_PARAMS *psRCParams = &(ctx->sRCParams);
    ASSERT(obj_buffer->type == VAEncMiscParameterTypeHRD);
    ASSERT(obj_buffer->size == sizeof(VAEncMiscParameterHRD));
    psMiscHrdParams = (VAEncMiscParameterHRD *)pBuffer->data;

    if (psMiscHrdParams->buffer_size == 0
	|| psMiscHrdParams->initial_buffer_fullness == 0) {
	drv_debug_msg(VIDEO_DEBUG_ERROR, "Find zero value for buffer_size "
		"and initial_buffer_fullness.\n");
	vaStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
	return vaStatus;
    }

    if (ctx->initial_buffer_fullness > ctx->buffer_size) {
	drv_debug_msg(VIDEO_DEBUG_ERROR, "initial_buffer_fullnessi(%d) shouldn't be"
		" larger that buffer_size(%d)!\n",
		psMiscHrdParams->initial_buffer_fullness,
		psMiscHrdParams->buffer_size);
	vaStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
	return vaStatus;
    }

    if (!psRCParams->bRCEnable) {
	drv_debug_msg(VIDEO_DEBUG_ERROR, "Only when rate control is enabled,"
		" VAEncMiscParameterTypeHRD will take effect.\n");
	vaStatus = VA_STATUS_ERROR_ATTR_NOT_SUPPORTED;
	return vaStatus;
    }

    ctx->buffer_size = psMiscHrdParams->buffer_size;
    ctx->initial_buffer_fullness = psMiscHrdParams->initial_buffer_fullness;
    ctx->bInsertHRDParams = IMG_TRUE;
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "hrd param buffer_size set to %d "
	"initial buffer fullness set to %d\n",
	ctx->buffer_size, ctx->initial_buffer_fullness);

    return VA_STATUS_SUCCESS;
}

//APP_SetVideoParams
static VAStatus tng__H264ES_process_misc_air_param(context_ENC_p ctx, object_buffer_p obj_buffer)
{
    VAEncMiscParameterBuffer *pBuffer = (VAEncMiscParameterBuffer *) obj_buffer->buffer_data;
    VAEncMiscParameterAIR *psMiscAirParams = NULL;
    ADAPTIVE_INTRA_REFRESH_INFO_TYPE *psAIRInfo = &(ctx->sAirInfo);
    IMG_UINT32 ui32MbNum;
    ASSERT(obj_buffer->type == VAEncMiscParameterTypeAIR);
    ASSERT(obj_buffer->size == sizeof(VAEncMiscParameterAIR));
    
    psMiscAirParams = (VAEncMiscParameterAIR*)pBuffer->data;
    ui32MbNum = (ctx->ui16PictureHeight * ctx->ui16Width) >> 8;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: enable_AIR = %d, mb_num = %d, thresh_hold = %d, air_auto = %d\n", __FUNCTION__,
        ctx->bEnableAIR, psMiscAirParams->air_num_mbs, psMiscAirParams->air_threshold, psMiscAirParams->air_auto);

    ctx->bEnableAIR = 1;
    ctx->bEnableInpCtrl = 1;
    ctx->bEnableHostBias = 1;
    ctx->ui8EnableSelStatsFlags |= ESF_FIRST_STAGE_STATS;
    ctx->ui8EnableSelStatsFlags |= ESF_MP_BEST_MB_DECISION_STATS;

    /*FIXME
    if (psMiscAirParams->air_threshold == -1 && psMiscAirParams->air_num_mbs == 0) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, 
            "%s: ERROR: Cannot have both -AIRMBperFrame set to zero"
            "AND and -AIRSADThreshold set to -1 (APP_SetVideoParams)\n",
            __FUNCTION__);
        return VA_STATUS_ERROR_INVALID_PARAMETER;
    }*/

    if (psMiscAirParams->air_num_mbs > 65535) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: ERROR: air_num_mbs = %d should not bigger than 65536\n",
            __FUNCTION__, psMiscAirParams->air_num_mbs);
        return VA_STATUS_ERROR_INVALID_PARAMETER;
    }

    if (psMiscAirParams->air_num_mbs > ui32MbNum)
        psMiscAirParams->air_num_mbs = ui32MbNum;

    if (psMiscAirParams->air_threshold > 65535) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, 
            "%s: ERROR: air_threshold = %d should not bigger than 65536\n",
            __FUNCTION__, psMiscAirParams->air_threshold);
        return VA_STATUS_ERROR_INVALID_PARAMETER;
    }

    if (psMiscAirParams->air_auto) {
	psAIRInfo->i32NumAIRSPerFrame = -1;
	psAIRInfo->i32SAD_Threshold = -1;
	psAIRInfo->i16AIRSkipCnt = -1;
    } else {
	psAIRInfo->i32NumAIRSPerFrame = psMiscAirParams->air_num_mbs;
	psAIRInfo->i32SAD_Threshold = psMiscAirParams->air_threshold;
	psAIRInfo->i16AIRSkipCnt = -1;
    }

    psAIRInfo->ui16AIRScanPos = 0;

    drv_debug_msg(VIDEO_DEBUG_GENERAL,
        "%s: air slice size changed to num_air_mbs %d "
        "air_threshold %d, air_auto %d\n", __FUNCTION__,
        psMiscAirParams->air_num_mbs, psMiscAirParams->air_threshold,
        psMiscAirParams->air_auto);

    if (psAIRInfo->pi8AIR_Table != NULL)
        free(psAIRInfo->pi8AIR_Table);
    tng_air_buf_create(ctx);
    
    return VA_STATUS_SUCCESS;
}

static VAStatus tng__H264ES_process_misc_cir_param(context_ENC_p ctx, object_buffer_p obj_buffer)
{
    VAEncMiscParameterBuffer *pBuffer = (VAEncMiscParameterBuffer *)obj_buffer->buffer_data;
    VAEncMiscParameterCIR *psMiscCirParams = NULL;

    ASSERT(obj_buffer->type == VAEncMiscParameterTypeCIR);
    ASSERT(obj_buffer->size == sizeof(VAEncMiscParameterCIR));

    psMiscCirParams = (VAEncMiscParameterCIR*)pBuffer->data;

    if (psMiscCirParams->cir_num_mbs > 0) {
	drv_debug_msg(VIDEO_DEBUG_GENERAL,
	    "CIR enabled with MB count %d\n", ctx->ui16IntraRefresh);
	ctx->ui16IntraRefresh = psMiscCirParams->cir_num_mbs;
	ctx->bEnableCIR = 1;
	ctx->bEnableInpCtrl = 1;
	ctx->bEnableHostBias = 1;
    } else {
        drv_debug_msg(VIDEO_DEBUG_ERROR,
            "%s: ERROR: invalid cir num mbs(%d), should bigger than 0\n",
            __FUNCTION__, ctx->ui16IntraRefresh);
        return VA_STATUS_ERROR_INVALID_PARAMETER;
    }

    return VA_STATUS_SUCCESS;
}

static IMG_UINT8 tng__H264ES_calculate_level(context_ENC_p ctx)
{
    IMG_RC_PARAMS *psRCParams = &(ctx->sRCParams);
    IMG_UINT32 ui32MBf=0;
    IMG_UINT32 ui32MBs;
    IMG_UINT32 ui32Level=0;
    IMG_UINT32 ui32TempLevel=0;
    IMG_UINT32 ui32DpbMbs;
    // We need to calculate the level based on a few constraints these are
    // Macroblocks per second
    // picture size
    // decoded picture buffer size, and bitrate
    ui32MBf = (ctx->ui16Width)*(ctx->ui16FrameHeight) >> 8;
    ui32MBs = ui32MBf * psRCParams->ui32FrameRate;


    // could do these in nice tables, but this is clearer
    if      (ui32MBf > 22080) ui32Level = SH_LEVEL_51;
    else if (ui32MBf >  8704) ui32Level = SH_LEVEL_50;
    else if (ui32MBf >  8192) ui32Level = SH_LEVEL_42;
    else if (ui32MBf >  5120) ui32Level = SH_LEVEL_40;
    else if (ui32MBf >  3600) ui32Level = SH_LEVEL_32;
    else if (ui32MBf >  1620) ui32Level = SH_LEVEL_31;
    else if (ui32MBf >   792) ui32Level = SH_LEVEL_22;
    else if (ui32MBf >   396) ui32Level = SH_LEVEL_21;
    else if (ui32MBf >    99) ui32Level = SH_LEVEL_11;
    else ui32Level = SH_LEVEL_10;

    drv_debug_msg(VIDEO_DEBUG_GENERAL,
        "%s: ui32MBf = %d, ui32MBs = %d, ui32Level = %d\n",
        __FUNCTION__, ui32MBf, ui32MBs, ui32Level);

    //ui32DpbMbs = ui32MBf * (psContext->ui32MaxNumRefFrames + 1);
    ui32DpbMbs = ui32MBf * (ctx->ui8MaxNumRefFrames + 1);

    if      (ui32DpbMbs > 110400) ui32TempLevel = SH_LEVEL_51;
    else if (ui32DpbMbs >  34816) ui32TempLevel = SH_LEVEL_50;
    else if (ui32DpbMbs >  32768) ui32TempLevel = SH_LEVEL_42;
    else if (ui32DpbMbs >  20480) ui32TempLevel = SH_LEVEL_40;
    else if (ui32DpbMbs >  18000) ui32TempLevel = SH_LEVEL_32;
    else if (ui32DpbMbs >   8100) ui32TempLevel = SH_LEVEL_31;
    else if (ui32DpbMbs >   4752) ui32TempLevel = SH_LEVEL_22;
    else if (ui32DpbMbs >   2376) ui32TempLevel = SH_LEVEL_21;
    else if (ui32DpbMbs >    900) ui32TempLevel = SH_LEVEL_12;
    else if (ui32DpbMbs >    396) ui32TempLevel = SH_LEVEL_11;
    else ui32TempLevel = SH_LEVEL_10;
    ui32Level = tng__max(ui32Level, ui32TempLevel);

    drv_debug_msg(VIDEO_DEBUG_GENERAL,
        "%s: ui32DpbMbs = %d, ui32Level = %d\n",
        __FUNCTION__, ui32DpbMbs, ui32Level);

    // now restrict based on the number of macroblocks per second
    if      (ui32MBs > 589824) ui32TempLevel = SH_LEVEL_51;
    else if (ui32MBs > 522240) ui32TempLevel = SH_LEVEL_50;
    else if (ui32MBs > 245760) ui32TempLevel = SH_LEVEL_42;
    else if (ui32MBs > 216000) ui32TempLevel = SH_LEVEL_40;
    else if (ui32MBs > 108000) ui32TempLevel = SH_LEVEL_32;
    else if (ui32MBs >  40500) ui32TempLevel = SH_LEVEL_31;
    else if (ui32MBs >  20250) ui32TempLevel = SH_LEVEL_30;
    else if (ui32MBs >  19800) ui32TempLevel = SH_LEVEL_22;
    else if (ui32MBs >  11880) ui32TempLevel = SH_LEVEL_21;
    else if (ui32MBs >   6000) ui32TempLevel = SH_LEVEL_13;
    else if (ui32MBs >   3000) ui32TempLevel = SH_LEVEL_12;
    else if (ui32MBs >   1485) ui32TempLevel = SH_LEVEL_11;
    else ui32TempLevel = SH_LEVEL_10;
    ui32Level = tng__max(ui32Level, ui32TempLevel);

    drv_debug_msg(VIDEO_DEBUG_GENERAL,
        "%s: ui32TempLevel = %d, ui32Level = %d\n",
        __FUNCTION__, ui32TempLevel, ui32Level);

    if (psRCParams->bRCEnable) {
        // now restrict based on the requested bitrate
        if      (psRCParams->ui32FrameRate > 135000000) ui32TempLevel = SH_LEVEL_51;
        else if (psRCParams->ui32FrameRate >  50000000) ui32TempLevel = SH_LEVEL_50;
        else if (psRCParams->ui32FrameRate >  20000000) ui32TempLevel = SH_LEVEL_41;
        else if (psRCParams->ui32FrameRate >  14000000) ui32TempLevel = SH_LEVEL_32;
        else if (psRCParams->ui32FrameRate >  10000000) ui32TempLevel = SH_LEVEL_31;
        else if (psRCParams->ui32FrameRate >   4000000) ui32TempLevel = SH_LEVEL_30;
        else if (psRCParams->ui32FrameRate >   2000000) ui32TempLevel = SH_LEVEL_21;
        else if (psRCParams->ui32FrameRate >    768000) ui32TempLevel = SH_LEVEL_20;
        else if (psRCParams->ui32FrameRate >    384000) ui32TempLevel = SH_LEVEL_13;
        else if (psRCParams->ui32FrameRate >    192000) ui32TempLevel = SH_LEVEL_12;
        else if (psRCParams->ui32FrameRate >    128000) ui32TempLevel = SH_LEVEL_11;
        else if (psRCParams->ui32FrameRate >     64000) ui32TempLevel = SH_LEVEL_1B;
        else ui32TempLevel = SH_LEVEL_10;

        ui32Level = tng__max(ui32Level, ui32TempLevel);
    }
/*
    if (pParams->bLossless) {
        ui32Level = tng__max(ui32Level, 320);
    }
*/
    drv_debug_msg(VIDEO_DEBUG_GENERAL,
        "%s: target level is %d, input level is %d\n",
        __FUNCTION__, ui32Level, ctx->ui8LevelIdc);
    return (IMG_UINT8)ui32Level;
}

static VAStatus tng__H264ES_process_sequence_param(context_ENC_p ctx, object_buffer_p obj_buffer)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAEncSequenceParameterBufferH264 *psSeqParams;
    H264_CROP_PARAMS* psCropParams = &(ctx->sCropParams);
    IMG_RC_PARAMS *psRCParams = &(ctx->sRCParams);
    FRAME_ORDER_INFO *psFrameInfo = &(ctx->sFrameOrderInfo);
    H264_VUI_PARAMS *psVuiParams = &(ctx->sVuiParams);
    IMG_UINT32 ui32MaxUnit32 = (IMG_UINT32)0x7ffa;
    IMG_UINT32 ui32IPCount = 0;
    IMG_UINT64 ui64Temp = 0;

    ASSERT(obj_buffer->type == VAEncSequenceParameterBufferType);
    ASSERT(obj_buffer->size == sizeof(VAEncSequenceParameterBufferH264));

    if (obj_buffer->size != sizeof(VAEncSequenceParameterBufferH264)) {
        vaStatus = VA_STATUS_ERROR_UNKNOWN;
        goto out1;
    }

    ctx->obj_context->frame_count = 0;
    psSeqParams = (VAEncSequenceParameterBufferH264 *) obj_buffer->buffer_data;
    obj_buffer->buffer_data = NULL;
    obj_buffer->size = 0;

#ifdef _TOPAZHP_PDUMP_
    tng_H264ES_trace_seq_params(psSeqParams);
#endif

    ctx->ui8LevelIdc = psSeqParams->level_idc;
    ctx->ui8MaxNumRefFrames = psSeqParams->max_num_ref_frames;

    ctx->ui32IdrPeriod = psSeqParams->intra_idr_period;
    ctx->ui32IntraCnt = psSeqParams->intra_period;
    ui32IPCount = (IMG_UINT32)(psSeqParams->ip_period);

    if ((ui32IPCount > 4) || (ui32IPCount == 0)) {
        drv_debug_msg(VIDEO_DEBUG_ERROR,
            "%s: ip_period %d, it should be in [1, 4]\n",
            __FUNCTION__, psSeqParams->ip_period);
        vaStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
        goto out1;
    }

    if (ctx->ui32IntraCnt == 0) {
        if (ui32IPCount == 1)
            ctx->ui32IntraCnt = INT_MAX;
        else
            ctx->ui32IntraCnt = INT_MAX - (INT_MAX % ui32IPCount);
        ctx->ui32IdrPeriod = 1;
    } else if (ctx->ui32IntraCnt == 1) {
        //only I frame or IDR frames;
        ui32IPCount = 1;
        if (ctx->ui32IdrPeriod == 0)
            ctx->ui32IdrPeriod = INT_MAX;
    } else {
        if (ctx->ui32IdrPeriod == 0) {
            ctx->ui32IdrPeriod = INT_MAX / ctx->ui32IntraCnt;
        } else if (ctx->ui32IdrPeriod > 1) {
            ui64Temp = (IMG_UINT64)(ctx->ui32IdrPeriod) * (IMG_UINT64)(ctx->ui32IntraCnt);
            if (ui64Temp >= (IMG_UINT64)INT_MAX) {
                ctx->ui32IdrPeriod = INT_MAX / ctx->ui32IntraCnt;
            }
        }

        if ((ctx->ui32IntraCnt % ui32IPCount) != 0) {
            if (ctx->ui32IntraCnt > INT_MAX - ui32IPCount + (ctx->ui32IntraCnt % ui32IPCount))
                ctx->ui32IntraCnt = INT_MAX - ui32IPCount + (ctx->ui32IntraCnt % ui32IPCount);
            else
                ctx->ui32IntraCnt += ui32IPCount - (ctx->ui32IntraCnt % ui32IPCount);
        }
    }

    if (ctx->ui32FrameCount[ctx->ui32StreamID] > 0) {
        ctx->idr_force_flag = 1;
	if (ctx->ui32IntraCntSave != ctx->ui32IntraCnt) {
	    drv_debug_msg(VIDEO_DEBUG_GENERAL,
		"%s: intra_period updated from %d to %d\n",
		__FUNCTION__, ctx->ui32IntraCntSave, ctx->ui32IntraCnt);
	    ctx->rc_update_flag |= RC_MASK_intra_period;
	}
    }

    ctx->ui32IntraCntSave = ctx->ui32IntraCnt;

    ctx->ui8SlotsInUse = ui32IPCount + 1; //Bframes + 2

    //bits per second
    if (!psSeqParams->bits_per_second) {
        psSeqParams->bits_per_second = ctx->ui16Width * ctx->ui16PictureHeight * 30 * 12;
    }

    if (psSeqParams->bits_per_second > TOPAZ_H264_MAX_BITRATE) {
        psSeqParams->bits_per_second = TOPAZ_H264_MAX_BITRATE;
        drv_debug_msg(VIDEO_DEBUG_ERROR,
            "%s: bits_per_second(%d) exceeds the maximum bitrate, set it with %d\n",
            __FUNCTION__, psSeqParams->bits_per_second,
            TOPAZ_H264_MAX_BITRATE);
    }

    if (psRCParams->ui32BitsPerSecond == 0)
        psRCParams->ui32BitsPerSecond = psSeqParams->bits_per_second;

    if (psSeqParams->bits_per_second != psRCParams->ui32BitsPerSecond) {
        psRCParams->ui32BitsPerSecond = psSeqParams->bits_per_second;
	ctx->rc_update_flag |= RC_MASK_bits_per_second;
    }

    psRCParams->ui32IntraFreq = ctx->ui32IntraCnt;
    psRCParams->ui32TransferBitsPerSecond = psRCParams->ui32BitsPerSecond;
    psRCParams->ui16BFrames = ui32IPCount - 1;

    if (psRCParams->ui32FrameRate == 0)
        psRCParams->ui32FrameRate = 30;

    //set the B frames
    if (psRCParams->eRCMode == IMG_RCMODE_VCM)
        psRCParams->ui16BFrames = 0;

    if ((psRCParams->ui16BFrames > 0) && (ctx->ui8ProfileIdc == H264ES_PROFILE_BASELINE)) {
        ctx->ui8ProfileIdc = H264ES_PROFILE_MAIN;
    }

    if (psFrameInfo->slot_consume_dpy_order != NULL)
        free(psFrameInfo->slot_consume_dpy_order);
    if (psFrameInfo->slot_consume_enc_order != NULL)
        free(psFrameInfo->slot_consume_enc_order);

    if (psRCParams->ui16BFrames != 0) {
        memset(psFrameInfo, 0, sizeof(FRAME_ORDER_INFO));
        psFrameInfo->slot_consume_dpy_order = (int *)malloc(ctx->ui8SlotsInUse * sizeof(int));
        psFrameInfo->slot_consume_enc_order = (int *)malloc(ctx->ui8SlotsInUse * sizeof(int));

        if ((psFrameInfo->slot_consume_dpy_order == NULL) || 
            (psFrameInfo->slot_consume_enc_order == NULL)) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: error malloc slot order array\n", __FUNCTION__);
        }
    }

    //set the crop parameters
    psCropParams->bClip = psSeqParams->frame_cropping_flag;
    psCropParams->ui16LeftCropOffset = psSeqParams->frame_crop_left_offset;
    psCropParams->ui16RightCropOffset = psSeqParams->frame_crop_right_offset;
    psCropParams->ui16TopCropOffset = psSeqParams->frame_crop_top_offset;
    psCropParams->ui16BottomCropOffset = psSeqParams->frame_crop_bottom_offset;

    //set level idc parameter
    ctx->ui32VertMVLimit = 255 ;//(63.75 in qpel increments)
    ctx->bLimitNumVectors = IMG_FALSE;

    if (ctx->ui8LevelIdc == 111)
        ctx->ui8LevelIdc = SH_LEVEL_1B;

    ctx->ui8LevelIdc = tng__H264ES_calculate_level(ctx);

    /*Setting VertMVLimit and LimitNumVectors only for H264*/
    if (ctx->ui8LevelIdc >= SH_LEVEL_30)
        ctx->bLimitNumVectors = IMG_TRUE;
    else
        ctx->bLimitNumVectors = IMG_FALSE;

    if (ctx->ui8LevelIdc >= SH_LEVEL_31)
        ctx->ui32VertMVLimit = 2047 ;//(511.75 in qpel increments)
    else if (ctx->ui8LevelIdc >= SH_LEVEL_21)
        ctx->ui32VertMVLimit = 1023 ;//(255.75 in qpel increments)
    else if (ctx->ui8LevelIdc >= SH_LEVEL_11)
        ctx->ui32VertMVLimit = 511 ;//(127.75 in qpel increments)

    //set VUI info
    memset(psVuiParams, 0, sizeof(H264_VUI_PARAMS));
    if (psSeqParams->time_scale != 0 && psSeqParams->num_units_in_tick != 0
		&& (psSeqParams->time_scale > psSeqParams->num_units_in_tick)) {
        psVuiParams->Time_Scale = psSeqParams->time_scale;
        psVuiParams->num_units_in_tick = psSeqParams->num_units_in_tick;
    }

out1:
    free(obj_buffer->buffer_data);
    obj_buffer->buffer_data = NULL;
    obj_buffer->size = 0;

    return vaStatus;
}

#if 0
static VAStatus tng__H264ES_process_picture_param(context_ENC_p ctx, object_buffer_p obj_buffer)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    context_ENC_mem *ps_mem = &(ctx->ctx_mem[ctx->ui32StreamID]);
    context_ENC_frame_buf *ps_buf = &(ctx->ctx_frame_buf[ctx->ui32StreamID]);
    IMG_RC_PARAMS * psRCParams = &(ctx->sRCParams);
    VAEncPictureParameterBufferH264 *psPicParams;
    IMG_BOOL bDepViewPPS = IMG_FALSE;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: start\n",__FUNCTION__);
    ASSERT(obj_buffer->type == VAEncPictureParameterBufferType);
    if (obj_buffer->size != sizeof(VAEncPictureParameterBufferH264)) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s L%d Invalid coded buffer handle\n", __FUNCTION__, __LINE__);
        return VA_STATUS_ERROR_UNKNOWN;
    }

    /* Transfer ownership of VAEncPictureParameterBufferH264 data */
    psPicParams = (VAEncPictureParameterBufferH264 *) obj_buffer->buffer_data;

    obj_buffer->buffer_data = NULL;
    obj_buffer->size = 0;

    ASSERT(ctx->ui16Width == psPicParams->picture_width);
    ASSERT(ctx->ui16PictureHeight == psPicParams->picture_height);

#ifdef _TOPAZHP_OLD_LIBVA_
    ps_buf->ref_surface = SURFACE(psPicParams->ReferenceFrames[0].picture_id);
    ps_buf->coded_buf = BUFFER(psPicParams->coded_buf);
#else
    {
        IMG_INT32 i;
        ps_buf->rec_surface = SURFACE(psPicParams->CurrPic.picture_id);
        for (i = 0; i < 16; i++)
            ps_buf->ref_surface[i] = SURFACE(psPicParams->ReferenceFrames[i].picture_id);
        ps_buf->coded_buf = BUFFER(psPicParams->coded_buf);
    }
#endif

    if (NULL == ps_buf->coded_buf) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s L%d Invalid coded buffer handle\n", __FUNCTION__, __LINE__);
        free(psPicParams);
        return VA_STATUS_ERROR_INVALID_BUFFER;
    }

    if ((ctx->bEnableMvc) && (ctx->ui16MVCViewIdx != 0) &&
        (ctx->ui16MVCViewIdx != (IMG_UINT16)(NON_MVC_VIEW))) {
        bDepViewPPS = IMG_TRUE;
    }

    /************* init ****************
    ctx->bCabacEnabled = psPicParams->pic_fields.bits.entropy_coding_mode_flag;
    ctx->bH2648x8Transform = psPicParams->pic_fields.bits.transform_8x8_mode_flag;
    ctx->bH264IntraConstrained = psPicParams->pic_fields.bits.constrained_intra_pred_flag;
    ctx->bWeightedPrediction = psPicParams->pic_fields.bits.weighted_pred_flag;
    ctx->ui8VPWeightedImplicitBiPred = psPicParams->pic_fields.bits.weighted_bipred_idc;
    ctx->bCustomScaling = psPicParams->pic_fields.bits.pic_scaling_matrix_present_flag;
    ************* set rc params *************/
//    ctx->sRCParams.ui32InitialQp = psPicParams->pic_init_qp;
//    ctx->sRCParams.i8QCPOffset = psPicParams->chroma_qp_index_offset;
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s psRCParams->ui32InitialQp = %d, psRCParams->iMinQP = %d\n", __FUNCTION__, psPicParams->pic_init_qp, psPicParams->chroma_qp_index_offset);
    tng__H264ES_prepare_picture_header(
        ps_mem->bufs_pic_template.virtual_addr,
        0, //IMG_BOOL    bCabacEnabled,
        ctx->bH2648x8Transform, //IMG_BOOL    b_8x8transform,
        0, //IMG_BOOL    bIntraConstrained,
        0, //IMG_INT8    i8CQPOffset,
        0, //IMG_BOOL    bWeightedPrediction,
        0, //IMG_UINT8   ui8WeightedBiPred,
        0, //IMG_BOOL    bMvcPPS,
        0, //IMG_BOOL    bScalingMatrix,
        0  //IMG_BOOL    bScalingLists
    );
    free(psPicParams);
/*
    if (psRCParams->ui16BFrames == 0) {
        tng_send_codedbuf(ctx, (ctx->obj_context->frame_count&1));
        tng_send_source_frame(ctx, (ctx->obj_context->frame_count&1), ctx->obj_context->frame_count);
    } else
        tng__H264ES_provide_buffer_for_BFrames(ctx);
*/
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: end\n",__FUNCTION__);

    return vaStatus;
}
#endif

static VAStatus tng__H264ES_process_picture_param_base(context_ENC_p ctx, unsigned char *buf)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    context_ENC_frame_buf *ps_buf = &(ctx->ctx_frame_buf);
    VAEncPictureParameterBufferH264 *psPicParams;
#ifndef _TNG_FRAMES_
    IMG_INT32 i;
#endif
    psPicParams = (VAEncPictureParameterBufferH264 *) buf;

#ifdef _TOPAZHP_PDUMP_
    tng_H264ES_trace_pic_params(psPicParams);
#endif

#ifdef _TNG_FRAMES_
    ps_buf->rec_surface  = SURFACE(psPicParams->CurrPic.picture_id);
    ps_buf->ref_surface  = SURFACE(psPicParams->ReferenceFrames[0].picture_id);
    ps_buf->ref_surface1 = SURFACE(psPicParams->ReferenceFrames[1].picture_id);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: psPicParams->coded_buf = 0x%08x, ps_buf->coded_buf = 0x%08x\n",
        __FUNCTION__, psPicParams->coded_buf, ps_buf->coded_buf);
#else
    {
        ps_buf->rec_surface = SURFACE(psPicParams->CurrPic.picture_id);
        for (i = 0; i < 4; i++) {
            ps_buf->ref_surface[i] = SURFACE(psPicParams->ReferenceFrames[i].picture_id);
	    ps_buf->ref_surface[i]->is_ref_surface = 1;
	}
    }
#endif

    ps_buf->coded_buf = BUFFER(psPicParams->coded_buf);

    if (NULL == ps_buf->coded_buf) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s L%d Invalid coded buffer handle\n", __FUNCTION__, __LINE__);
        free(psPicParams);
        return VA_STATUS_ERROR_INVALID_BUFFER;
    }

    ctx->bH2648x8Transform = psPicParams->pic_fields.bits.transform_8x8_mode_flag;
    if ((ctx->bH2648x8Transform == 1) && (ctx->ui8ProfileIdc != H264ES_PROFILE_HIGH)) {
        drv_debug_msg(VIDEO_DEBUG_ERROR,
        "%s L%d only high profile could set bH2648x8Transform TRUE\n",
        __FUNCTION__, __LINE__);
        ctx->bH2648x8Transform = 0;
    }

    ctx->bH264IntraConstrained = psPicParams->pic_fields.bits.constrained_intra_pred_flag;
    if (ctx->bEnableMVC == 1) {
        drv_debug_msg(VIDEO_DEBUG_ERROR,
            "%s L%d MVC could not set bH264IntraConstrained TRUE\n",
            __FUNCTION__, __LINE__);
        ctx->bH264IntraConstrained = 0;
    }

    ctx->bCabacEnabled = psPicParams->pic_fields.bits.entropy_coding_mode_flag;
    ctx->bWeightedPrediction = psPicParams->pic_fields.bits.weighted_pred_flag;
    ctx->ui8VPWeightedImplicitBiPred = psPicParams->pic_fields.bits.weighted_bipred_idc;

    /************* init ****************
    ctx->bCustomScaling = psPicParams->pic_fields.bits.pic_scaling_matrix_present_flag;
    ************* set rc params *************/
    if (ctx->sRCParams.ui32InitialQp == 0)
        ctx->sRCParams.ui32InitialQp = psPicParams->pic_init_qp;

    ctx->ui32LastPicture = psPicParams->last_picture;

    return vaStatus;
}

static VAStatus tng__H264ES_process_picture_param_mvc(context_ENC_p ctx, unsigned char *buf)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAEncPictureParameterBufferH264_MVC *psPicMvcParams;

    psPicMvcParams = (VAEncPictureParameterBufferH264_MVC *) buf;
    ctx->ui16MVCViewIdx = ctx->ui32StreamID = psPicMvcParams->view_id;
    //vaStatus = tng__H264ES_process_picture_param_base(ctx, (unsigned char*)&(psPicMvcParams->base_picture_param));

    return vaStatus;
}

static VAStatus tng__H264ES_process_picture_param(context_ENC_p ctx, object_buffer_p obj_buffer)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    ASSERT(obj_buffer->type == VAEncPictureParameterBufferType);

    drv_debug_msg(VIDEO_DEBUG_GENERAL,
        "%s: ctx->bEnableMVC = %d\n", __FUNCTION__, ctx->bEnableMVC);

    if (ctx->bEnableMVC) {
        if (obj_buffer->size != sizeof(VAEncPictureParameterBufferH264_MVC)) {
            drv_debug_msg(VIDEO_DEBUG_ERROR,
                "%s L%d Invalid picture parameter H264 mvc buffer handle\n",
                __FUNCTION__, __LINE__);
            return VA_STATUS_ERROR_UNKNOWN;
        }
        vaStatus = tng__H264ES_process_picture_param_mvc(ctx, obj_buffer->buffer_data);
    } else {
        if (obj_buffer->size != sizeof(VAEncPictureParameterBufferH264)) {
            drv_debug_msg(VIDEO_DEBUG_ERROR,
                "%s L%d Invalid picture parameter H264 buffer handle\n",
                __FUNCTION__, __LINE__);
            return VA_STATUS_ERROR_UNKNOWN;
        }
        vaStatus = tng__H264ES_process_picture_param_base(ctx, obj_buffer->buffer_data);
    }
    free(obj_buffer->buffer_data);
    obj_buffer->buffer_data = NULL;
    obj_buffer->size = 0;
    return vaStatus;
}

static VAStatus tng__H264ES_process_slice_param_mrfld(context_ENC_p ctx, object_buffer_p obj_buffer)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAEncSliceParameterBufferH264 *psSliceParamsH264;
    unsigned int i;
    unsigned int uiPreMBAddress = 0;
    unsigned int uiCurMBAddress = 0;
    unsigned int uiPreMBNumbers = 0;
    unsigned int uiCurMBNumbers = 0;
    unsigned int uiAllMBNumbers = 0;
    unsigned char ucPreDeblockIdc = 0;
    unsigned char ucCurDeblockIdc = 0;
    
    /* Transfer ownership of VAEncPictureParameterBufferH264 data */
    psSliceParamsH264 = (VAEncSliceParameterBufferH264*) obj_buffer->buffer_data;

#ifdef _TOPAZHP_PDUMP_
    tng_H264ES_trace_slice_params(psSliceParamsH264);
#endif

    ucPreDeblockIdc = psSliceParamsH264->disable_deblocking_filter_idc;

    for (i = 0; i < obj_buffer->num_elements; i++) {
        uiCurMBAddress = psSliceParamsH264->macroblock_address;
        uiCurMBNumbers = psSliceParamsH264->num_macroblocks;
        if (uiCurMBAddress != uiPreMBAddress + uiPreMBNumbers) {
            drv_debug_msg(VIDEO_DEBUG_ERROR,
                "%s L%d Error Macroblock Address (%d), address (%d), number (%d)\n",
                __FUNCTION__, __LINE__, i, psSliceParamsH264->macroblock_address,
                psSliceParamsH264->num_macroblocks);
            return VA_STATUS_ERROR_INVALID_PARAMETER;
        }
        uiPreMBNumbers = uiCurMBNumbers;
        uiPreMBAddress = uiCurMBAddress;
        uiAllMBNumbers += uiCurMBNumbers;

        ucCurDeblockIdc = psSliceParamsH264->disable_deblocking_filter_idc;
        if (ucPreDeblockIdc != ucCurDeblockIdc) {
            drv_debug_msg(VIDEO_DEBUG_ERROR,
                "%s L%d Error Macroblock Address (%d), deblock idc pre (%d), cur (%d)\n",
                __FUNCTION__, __LINE__, i, ucPreDeblockIdc, ucCurDeblockIdc);
            return VA_STATUS_ERROR_INVALID_PARAMETER;
        }
        psSliceParamsH264++;
    }

    if (uiAllMBNumbers != (unsigned int)(((IMG_UINT16)(ctx->ui16Width) * (IMG_UINT16)(ctx->ui16PictureHeight)) >> 8)) {
        drv_debug_msg(VIDEO_DEBUG_ERROR,
            "%s L%d Error Macroblock all number (%d), (%d)\n",
            __FUNCTION__, __LINE__, i, uiAllMBNumbers,
            ((ctx->ui16Width * ctx->ui16PictureHeight) >> 8));
        return VA_STATUS_ERROR_INVALID_PARAMETER;
    }

    //deblocking behaviour
    ctx->bArbitrarySO = IMG_FALSE;
    ctx->ui8DeblockIDC = ucCurDeblockIdc;
    ctx->ui8SlicesPerPicture = obj_buffer->num_elements;

    return vaStatus;
}

static VAStatus tng__H264ES_process_slice_param_mdfld(context_ENC_p ctx, object_buffer_p obj_buffer)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAEncSliceParameterBuffer *psSliceParams = NULL;
    psSliceParams = (VAEncSliceParameterBuffer*) obj_buffer->buffer_data;

    //deblocking behaviour
    ctx->bArbitrarySO = IMG_FALSE;
    ctx->ui8DeblockIDC = psSliceParams->slice_flags.bits.disable_deblocking_filter_idc;
    ctx->ui8SlicesPerPicture = obj_buffer->num_elements;
    return vaStatus;
}

static VAStatus tng__H264ES_process_misc_max_slice_size_param(context_ENC_p ctx, object_buffer_p obj_buffer)
{
    VAEncMiscParameterBuffer *pBuffer = (VAEncMiscParameterBuffer *) obj_buffer->buffer_data;
    VAEncMiscParameterMaxSliceSize *psMiscMaxSliceSizeParams = NULL;
    IMG_RC_PARAMS *psRCParams = &(ctx->sRCParams);

    ASSERT(obj_buffer->type == VAEncMiscParameterTypeMaxSliceSize);
    ASSERT(obj_buffer->size == sizeof(VAEncMiscParameterMaxSliceSize));

    psMiscMaxSliceSizeParams = (VAEncMiscParameterMaxSliceSize*)pBuffer->data;

    if (psMiscMaxSliceSizeParams->max_slice_size > 0) {
	psRCParams->ui32SliceByteLimit = psMiscMaxSliceSizeParams->max_slice_size;
	drv_debug_msg(VIDEO_DEBUG_GENERAL,
	    "Max slice size is %d\n", psRCParams->ui32SliceByteLimit);
    } else {
        drv_debug_msg(VIDEO_DEBUG_ERROR,
            "%s: ERROR: invalid max slice size(%d), should bigger than 0\n",
            __FUNCTION__, psMiscMaxSliceSizeParams->max_slice_size);
	psRCParams->ui32SliceByteLimit = 0;
        return VA_STATUS_ERROR_INVALID_PARAMETER;
    }

    return VA_STATUS_SUCCESS;
}

static VAStatus tng__H264ES_process_slice_param(context_ENC_p ctx, object_buffer_p obj_buffer)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    ASSERT(obj_buffer->type == VAEncSliceParameterBufferType);
    /* Prepare InParams for macros of current slice, insert slice header, insert do slice command */

    if (obj_buffer->size == sizeof(VAEncSliceParameterBufferH264)) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Receive VAEncSliceParameterBufferH264 buffer\n");
        vaStatus = tng__H264ES_process_slice_param_mrfld(ctx, obj_buffer);
    } else if (obj_buffer->size == sizeof(VAEncSliceParameterBuffer)) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Receive VAEncSliceParameterBuffer buffer\n");
        vaStatus = tng__H264ES_process_slice_param_mdfld(ctx, obj_buffer);
    } else {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Buffer size(%d) is wrong. It should be %d or %d\n",
            obj_buffer->size, sizeof(VAEncSliceParameterBuffer),
            sizeof(VAEncSliceParameterBufferH264));
        vaStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
    }

    free(obj_buffer->buffer_data);
    obj_buffer->size = 0;
    obj_buffer->buffer_data = NULL;
    return vaStatus;
}

static VAStatus tng__H264ES_process_misc_param(context_ENC_p ctx, object_buffer_p obj_buffer)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAEncMiscParameterBuffer *pBuffer;

    ASSERT(obj_buffer->type == VAEncMiscParameterBufferType);
    ASSERT(ctx != NULL);
    /* Transfer ownership of VAEncMiscParameterBuffer data */
    pBuffer = (VAEncMiscParameterBuffer *) obj_buffer->buffer_data;
    obj_buffer->size = 0;

    switch (pBuffer->type) {
        case VAEncMiscParameterTypeFrameRate:
            vaStatus = tng__H264ES_process_misc_framerate_param(ctx, obj_buffer);
            break;
        case VAEncMiscParameterTypeRateControl:
            vaStatus = tng__H264ES_process_misc_ratecontrol_param(ctx, obj_buffer);
            break;
        case VAEncMiscParameterTypeHRD:
            vaStatus = tng__H264ES_process_misc_hrd_param(ctx, obj_buffer);
            break;
        case VAEncMiscParameterTypeAIR:
            vaStatus = tng__H264ES_process_misc_air_param(ctx, obj_buffer);
            break;
	case VAEncMiscParameterTypeCIR:
            vaStatus = tng__H264ES_process_misc_cir_param(ctx, obj_buffer);
            break;
	case VAEncMiscParameterTypeMaxSliceSize:
	    vaStatus = tng__H264ES_process_misc_max_slice_size_param(ctx, obj_buffer);
            break;
        default:
            break;
    }
    free(obj_buffer->buffer_data);
    obj_buffer->buffer_data = NULL;

    return vaStatus;
}

static void tng_H264ES_QueryConfigAttributes(
    VAProfile __maybe_unused profile,
    VAEntrypoint __maybe_unused entrypoint,
    VAConfigAttrib *attrib_list,
    int num_attribs)
{
    int i;

    /* RateControl attributes */
    for (i = 0; i < num_attribs; i++) {
        switch (attrib_list[i].type) {
        case VAConfigAttribRTFormat:
            break;

        case VAConfigAttribRateControl:
            attrib_list[i].value = VA_RC_NONE | VA_RC_CBR | VA_RC_VBR | VA_RC_VCM;
            break;

        case VAConfigAttribEncAutoReference:
            attrib_list[i].value = 1;
            break;

        case VAConfigAttribEncMaxRefFrames:
            attrib_list[i].value = 4;
            break;

        default:
            attrib_list[i].value = VA_ATTRIB_NOT_SUPPORTED;
            break;
        }
    }
}

static VAStatus tng_H264ES_ValidateConfig(
    object_config_p obj_config)
{
    int i;

    /* Check all attributes */
    for (i = 0; i < obj_config->attrib_count; i++) {
        switch (obj_config->attrib_list[i].type) {
            case VAConfigAttribRTFormat:
                /* Ignore */
                break;
            case VAConfigAttribRateControl:
                break;
            case VAConfigAttribEncAutoReference:
                break;
            case VAConfigAttribEncMaxRefFrames:
                break;
            default:
                return VA_STATUS_ERROR_ATTR_NOT_SUPPORTED;
        }
    }

    return VA_STATUS_SUCCESS;
}

static VAStatus tng_H264ES_setup_profile_features(context_ENC_p ctx)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    IMG_ENCODE_FEATURES * pEncFeatures = &ctx->sEncFeatures;
    pEncFeatures->bEnable8x16MVDetect = IMG_TRUE;
    pEncFeatures->bEnable16x8MVDetect = IMG_TRUE;

    return vaStatus;
}


static VAStatus tng_H264ES_CreateContext(
    object_context_p obj_context,
    object_config_p obj_config)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    context_ENC_p ctx;

    vaStatus = tng_CreateContext(obj_context, obj_config, 0);

    if (VA_STATUS_SUCCESS != vaStatus)
        return VA_STATUS_ERROR_ALLOCATION_FAILED;

    ctx = (context_ENC_p) obj_context->format_data;
    ctx->eStandard = IMG_STANDARD_H264;

    tng__H264ES_init_context(obj_context, obj_config);

    vaStatus = tng__H264ES_init_profile(obj_context, obj_config);
    if (vaStatus != VA_STATUS_SUCCESS) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: tng__H264ES_init_profile\n", __FUNCTION__);
    }

    vaStatus = tng__H264ES_init_format_mode(obj_context, obj_config);
    if (vaStatus != VA_STATUS_SUCCESS) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: tng__H264ES_init_format_mode\n", __FUNCTION__);
    }

    vaStatus = tng__H264ES_get_codec_type(obj_context, obj_config);
    if (vaStatus != VA_STATUS_SUCCESS) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: tng__H264ES_init_rc_mode\n", __FUNCTION__);
    }

    vaStatus = tng_H264ES_setup_profile_features(ctx);
    if (vaStatus != VA_STATUS_SUCCESS) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: tng__profile_features\n", __FUNCTION__);
    }

    vaStatus = tng__patch_hw_profile(ctx);
    if (vaStatus != VA_STATUS_SUCCESS) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: tng__patch_hw_profile\n", __FUNCTION__);
    }

    return vaStatus;
}

static void tng_H264ES_DestroyContext(
    object_context_p obj_context)
{
    tng_DestroyContext(obj_context, 0);
}

static VAStatus tng_H264ES_BeginPicture(
    object_context_p obj_context)
{
    INIT_CONTEXT_H264ES;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    vaStatus = tng_BeginPicture(ctx);
    return vaStatus;
}

static VAStatus tng_H264ES_RenderPicture(
    object_context_p obj_context,
    object_buffer_p *buffers,
    int num_buffers)
{
    INIT_CONTEXT_H264ES;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    int i;

    for (i = 0; i < num_buffers; i++) {
        object_buffer_p obj_buffer = buffers[i];
        drv_debug_msg(VIDEO_DEBUG_GENERAL,
            "%s: type = %d, num = %d\n",
            __FUNCTION__, obj_buffer->type, num_buffers);

        switch (obj_buffer->type) {
            case VAEncSequenceParameterBufferType:
                drv_debug_msg(VIDEO_DEBUG_GENERAL,
                    "tng_H264_RenderPicture got VAEncSequenceParameterBufferType\n");
                vaStatus = tng__H264ES_process_sequence_param(ctx, obj_buffer);
                DEBUG_FAILURE;
                break;
            case VAEncPictureParameterBufferType:
                drv_debug_msg(VIDEO_DEBUG_GENERAL,
                    "tng_H264_RenderPicture got VAEncPictureParameterBuffer\n");
                vaStatus = tng__H264ES_process_picture_param(ctx, obj_buffer);
                DEBUG_FAILURE;
                break;

            case VAEncSliceParameterBufferType:
                drv_debug_msg(VIDEO_DEBUG_GENERAL,
                    "tng_H264_RenderPicture got VAEncSliceParameterBufferType\n");
                vaStatus = tng__H264ES_process_slice_param(ctx, obj_buffer);
                DEBUG_FAILURE;
                break;

            case VAEncMiscParameterBufferType:
                drv_debug_msg(VIDEO_DEBUG_GENERAL,
                    "tng_H264_RenderPicture got VAEncMiscParameterBufferType\n");
                vaStatus = tng__H264ES_process_misc_param(ctx, obj_buffer);
                DEBUG_FAILURE;
                break;
            default:
                vaStatus = VA_STATUS_ERROR_UNKNOWN;
                DEBUG_FAILURE;
        }
        if (vaStatus != VA_STATUS_SUCCESS) {
            break;
        }
    }
    return vaStatus;
}

static VAStatus tng_H264ES_EndPicture(
    object_context_p obj_context)
{
    INIT_CONTEXT_H264ES;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    vaStatus = tng_EndPicture(ctx);
    return vaStatus;
}

struct format_vtable_s tng_H264ES_vtable = {
queryConfigAttributes:
    tng_H264ES_QueryConfigAttributes,
validateConfig:
    tng_H264ES_ValidateConfig,
createContext:
    tng_H264ES_CreateContext,
destroyContext:
    tng_H264ES_DestroyContext,
beginPicture:
    tng_H264ES_BeginPicture,
renderPicture:
    tng_H264ES_RenderPicture,
endPicture:
    tng_H264ES_EndPicture
};

/*EOF*/
