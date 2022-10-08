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
 *    Zhaohan Ren <zhaohan.ren@intel.com>
 *
 */


#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

#include "psb_def.h"
#include "psb_surface.h"
#include "tng_cmdbuf.h"
#include "tng_hostcode.h"
#include "tng_hostheader.h"
#include "tng_MPEG4ES.h"
#include "psb_drv_debug.h"

#include "hwdefs/coreflags.h"
#include "hwdefs/topaz_vlc_regs.h"
#include "hwdefs/topaz_db_regs.h"
#include "hwdefs/topazhp_default_params.h"

#define TOPAZ_MPEG4_MAX_BITRATE 16000000

#define INIT_CONTEXT_MPEG4ES    context_ENC_p ctx = (context_ENC_p) obj_context->format_data
#define SURFACE(id)    ((object_surface_p) object_heap_lookup( &ctx->obj_context->driver_data->surface_heap, id ))
#define BUFFER(id)  ((object_buffer_p) object_heap_lookup( &ctx->obj_context->driver_data->buffer_heap, id ))

static void tng_MPEG4ES_QueryConfigAttributes(
    VAProfile __maybe_unused profile,
    VAEntrypoint __maybe_unused entrypoint,
    VAConfigAttrib *attrib_list,
    int num_attribs)
{
    int i;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s\n", __FUNCTION__);

    /* RateControl attributes */
    for (i = 0; i < num_attribs; i++) {
        switch (attrib_list[i].type) {
        case VAConfigAttribRTFormat:
            break;

        case VAConfigAttribEncAutoReference:
            attrib_list[i].value = 1;
            break;

        case VAConfigAttribEncMaxRefFrames:
            attrib_list[i].value = 2;
            break;

        case VAConfigAttribRateControl:
            attrib_list[i].value = VA_RC_NONE | VA_RC_CBR | VA_RC_VBR;
            break;

        default:
            attrib_list[i].value = VA_ATTRIB_NOT_SUPPORTED;
            break;
        }
    }
}


static VAStatus tng_MPEG4ES_ValidateConfig(
    object_config_p obj_config)
{
    int i;
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s\n", __FUNCTION__);
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

static VAStatus tng_MPEG4ES_CreateContext(
    object_context_p obj_context,
    object_config_p obj_config)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    context_ENC_p ctx;
    unsigned int eRCMode;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s\n", __FUNCTION__);

    vaStatus = tng_CreateContext(obj_context, obj_config, 0);

    if (VA_STATUS_SUCCESS != vaStatus)
        return VA_STATUS_ERROR_ALLOCATION_FAILED;

    ctx = (context_ENC_p) obj_context->format_data;
    ctx->eStandard = IMG_STANDARD_MPEG4;
    ctx->eFormat = IMG_CODEC_PL12;                          // use default

    switch(ctx->sRCParams.eRCMode) {
	case IMG_RCMODE_NONE:
	    ctx->eCodec = IMG_CODEC_MPEG4_NO_RC;
	    break;
	case IMG_RCMODE_VBR:
	    ctx->eCodec = IMG_CODEC_MPEG4_VBR;
	    break;
	case IMG_RCMODE_CBR:
	    ctx->eCodec = IMG_CODEC_MPEG4_CBR;
	    break;
	default:
	    drv_debug_msg(VIDEO_DEBUG_ERROR, "Unknown RCMode %08x\n", ctx->sRCParams.eRCMode);
	    break;
    }

    ctx->bIsInterlaced = IMG_FALSE;
    ctx->bIsInterleaved = IMG_FALSE;
    ctx->ui16PictureHeight = ctx->ui16FrameHeight;

    ctx->bVPAdaptiveRoundingDisable = IMG_TRUE;

    /* TopazHP only support Simple Profile */
    switch (obj_config->profile) {
        case VAProfileMPEG4Simple:
            ctx->ui8ProfileIdc = SP;
            break;
        default:
            ctx->ui8ProfileIdc = SP;
        break;
    }

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
    ctx->bInsertHRDParams = 0;


    ctx->bArbitrarySO = IMG_FALSE;
    ctx->ui32BasicUnit = 0;

    return vaStatus;
}

static void tng_MPEG4ES_DestroyContext(
    object_context_p obj_context)
{
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s\n", __FUNCTION__);
    tng_DestroyContext(obj_context, 0);
}

static VAStatus tng_MPEG4ES_BeginPicture(
    object_context_p obj_context)
{
    INIT_CONTEXT_MPEG4ES;
    tng_cmdbuf_p cmdbuf = ctx->obj_context->tng_cmdbuf;
    context_ENC_mem *ps_mem = &(ctx->ctx_mem[ctx->ui32StreamID]);
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s start\n", __FUNCTION__);
    vaStatus = tng_BeginPicture(ctx);

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s end\n", __FUNCTION__);
    return vaStatus;
}

static VAStatus tng__MPEG4ES_process_sequence_param(context_ENC_p ctx, object_buffer_p obj_buffer)
{
    context_ENC_mem *ps_mem = &(ctx->ctx_mem[ctx->ui32StreamID]);
    VAEncSequenceParameterBufferMPEG4 *psSeqParams;
    tng_cmdbuf_p cmdbuf = ctx->obj_context->tng_cmdbuf;
    IMG_RC_PARAMS *psRCParams = &(ctx->sRCParams);

    ASSERT(obj_buffer->type == VAEncSequenceParameterBufferType);
    ASSERT(obj_buffer->size == sizeof(VAEncSequenceParameterBufferMPEG4));

    if (obj_buffer->size != sizeof(VAEncSequenceParameterBufferMPEG4)) {
        return VA_STATUS_ERROR_UNKNOWN;
    }

    ctx->obj_context->frame_count = 0;
    psSeqParams = (VAEncSequenceParameterBufferMPEG4 *) obj_buffer->buffer_data;
    obj_buffer->buffer_data = NULL;
    obj_buffer->size = 0;

    ctx->ui32IdrPeriod = psSeqParams->intra_period;
    ctx->ui32IntraCnt = psSeqParams->intra_period;

    if (ctx->ui32IntraCnt == 0) {
        ctx->ui32IntraCnt = INT_MAX;
        ctx->ui32IdrPeriod = 1;
        drv_debug_msg(VIDEO_DEBUG_GENERAL,
           "%s: only ONE I frame in the sequence, %d\n",
           __FUNCTION__, ctx->ui32IdrPeriod);
    }

    ctx->bCustomScaling = IMG_FALSE;
    ctx->bUseDefaultScalingList = IMG_FALSE;

    //set MV limit infor
    ctx->ui32VertMVLimit = 255 ;//(63.75 in qpel increments)
    ctx->bLimitNumVectors = IMG_TRUE;

    ctx->ui8LevelIdc = psSeqParams->profile_and_level_indication;

    /**************set rc params ****************/
    if (psSeqParams->bits_per_second > TOPAZ_MPEG4_MAX_BITRATE) {
        ctx->sRCParams.ui32BitsPerSecond = TOPAZ_MPEG4_MAX_BITRATE;
        drv_debug_msg(VIDEO_DEBUG_GENERAL, " bits_per_second(%d) exceeds \
		the maximum bitrate, set it with %d\n",
                                 psSeqParams->bits_per_second,
                                 TOPAZ_MPEG4_MAX_BITRATE);
    } else
        ctx->sRCParams.ui32BitsPerSecond = psSeqParams->bits_per_second;

    //FIXME: Zhaohan, this should be figured out in testsuite?
    if (!ctx->uiCbrBufferTenths)
	ctx->uiCbrBufferTenths = TOPAZHP_DEFAULT_uiCbrBufferTenths;

    if (ctx->uiCbrBufferTenths) {
        psRCParams->ui32BufferSize      = (IMG_UINT32)(psRCParams->ui32BitsPerSecond * ctx->uiCbrBufferTenths / 10.0);
    } else {
        if (psRCParams->ui32BitsPerSecond < 256000)
            psRCParams->ui32BufferSize = ((9 * psRCParams->ui32BitsPerSecond) >> 1);
        else
            psRCParams->ui32BufferSize = ((5 * psRCParams->ui32BitsPerSecond) >> 1);
    }

    psRCParams->i32InitialDelay = (13 * psRCParams->ui32BufferSize) >> 4;
    psRCParams->i32InitialLevel = (3 * psRCParams->ui32BufferSize) >> 4;
    psRCParams->ui32IntraFreq = psSeqParams->intra_period;

    psRCParams->ui32InitialQp = psSeqParams->initial_qp;
    psRCParams->iMinQP = psSeqParams->min_qp;
    ctx->ui32BasicUnit = psSeqParams->min_qp;
    //psRCParams->ui32BUSize = psSeqParams->basic_unit_size;
    //ctx->ui32KickSize = psRCParams->ui32BUSize;
    psRCParams->ui32FrameRate = psSeqParams->frame_rate;

    //B-frames are not supported for non-H.264 streams
    ctx->sRCParams.ui16BFrames = 0;
    ctx->ui8SlotsInUse = psRCParams->ui16BFrames + 2;

#if HEADERS_VERBOSE_OUTPUT
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "\n\n**********************************************************************\n");
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "******** HOST FIRMWARE ROUTINES TO PASS HEADERS AND TOKENS TO MTX******\n");
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "**********************************************************************\n\n");
#endif

    free(psSeqParams);

    return VA_STATUS_SUCCESS;
}

static VAStatus tng__MPEG4ES_process_picture_param(context_ENC_p ctx, object_buffer_p obj_buffer)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    context_ENC_mem *ps_mem = &(ctx->ctx_mem[ctx->ui32StreamID]);
    context_ENC_frame_buf *ps_buf = &(ctx->ctx_frame_buf);
    VAEncPictureParameterBufferMPEG4 *psPicParams;
    IMG_BOOL bDepViewPPS = IMG_FALSE;
    void* pTmpBuf = NULL;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: start\n",__FUNCTION__);
    ASSERT(obj_buffer->type == VAEncPictureParameterBufferType);
    if (obj_buffer->size != sizeof(VAEncPictureParameterBufferMPEG4)) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s L%d Invalid coded buffer handle\n", __FUNCTION__, __LINE__);
        return VA_STATUS_ERROR_UNKNOWN;
    }

    /* Transfer ownership of VAEncPictureParameterBufferMPEG4 data */
    psPicParams = (VAEncPictureParameterBufferMPEG4 *) obj_buffer->buffer_data;
    obj_buffer->buffer_data = NULL;
    obj_buffer->size = 0;

    ASSERT(ctx->ui16Width == psPicParams->picture_width);
    ASSERT(ctx->ui16PictureHeight == psPicParams->picture_height);
#ifndef _TNG_FRAMES_
    ps_buf->ref_surface[0] = ps_buf->ref_surface[2] = SURFACE(psPicParams->reference_picture);
    ps_buf->ref_surface[1] = ps_buf->ref_surface[3] = SURFACE(psPicParams->reconstructed_picture);

    ps_buf->ref_surface[0]->is_ref_surface = ps_buf->ref_surface[2]->is_ref_surface = 1;
    ps_buf->ref_surface[1]->is_ref_surface = ps_buf->ref_surface[3]->is_ref_surface = 1;
#else
    ps_buf->ref_surface = SURFACE(psPicParams->reference_picture);
    ps_buf->rec_surface = SURFACE(psPicParams->reconstructed_picture);
#endif
    ps_buf->coded_buf = BUFFER(psPicParams->coded_buf);

    if (NULL == ps_buf->coded_buf) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s L%d Invalid coded buffer handle\n", __FUNCTION__, __LINE__);
        free(psPicParams);
        return VA_STATUS_ERROR_INVALID_BUFFER;
    }

    free(psPicParams);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: end\n",__FUNCTION__);

    return vaStatus;
}

static VAStatus tng__MPEG4ES_process_slice_param(context_ENC_p ctx, object_buffer_p obj_buffer)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAEncSliceParameterBuffer *psSliceParams;
    
    ASSERT(obj_buffer->type == VAEncSliceParameterBufferType);
    /* Prepare InParams for macros of current slice, insert slice header, insert do slice command */
    
    /* Transfer ownership of VAEncPictureParameterBufferMPEG4 data */
    psSliceParams = (VAEncSliceParameterBuffer*) obj_buffer->buffer_data;
    obj_buffer->size = 0;

    //deblocking behaviour
    ctx->bArbitrarySO = IMG_FALSE;
    ctx->ui8DeblockIDC = psSliceParams->slice_flags.bits.disable_deblocking_filter_idc;
    ++ctx->ui8SlicesPerPicture;
    return vaStatus;
}

static VAStatus tng__MPEG4ES_process_misc_param(context_ENC_p ctx, object_buffer_p obj_buffer)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAEncMiscParameterBuffer *pBuffer;
    VAEncMiscParameterFrameRate *frame_rate_param;
    VAEncMiscParameterRateControl *rate_control_param;
    IMG_RC_PARAMS   *psRCParams = &(ctx->sRCParams);

    ASSERT(obj_buffer->type == VAEncMiscParameterBufferType);

    /* Transfer ownership of VAEncMiscParameterBuffer data */
    pBuffer = (VAEncMiscParameterBuffer *) obj_buffer->buffer_data;
    obj_buffer->size = 0;

    switch (pBuffer->type) {
    case VAEncMiscParameterTypeRateControl:
        rate_control_param = (VAEncMiscParameterRateControl *)pBuffer->data;

        if (rate_control_param->initial_qp > 51 || rate_control_param->min_qp > 51) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "Initial_qp(%d) and min_qpinitial_qp(%d) "
                               "are invalid.\nQP shouldn't be larger than 51 for MPEG4\n",
                               rate_control_param->initial_qp, rate_control_param->min_qp);
            vaStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
            break;
        }

        drv_debug_msg(VIDEO_DEBUG_GENERAL, "rate control changed from %d to %d\n",
                                 psRCParams->ui32BitsPerSecond,
                                 rate_control_param->bits_per_second);

        if ((rate_control_param->bits_per_second == psRCParams->ui32BitsPerSecond) &&
            (psRCParams->ui32BufferSize == psRCParams->ui32BitsPerSecond / 1000 * rate_control_param->window_size) &&
            (psRCParams->iMinQP == rate_control_param->min_qp) &&
            (psRCParams->ui32InitialQp == rate_control_param->initial_qp))
            break;

        if (rate_control_param->bits_per_second > TOPAZ_MPEG4_MAX_BITRATE) {
            psRCParams->ui32BitsPerSecond = TOPAZ_MPEG4_MAX_BITRATE;
            drv_debug_msg(VIDEO_DEBUG_GENERAL, " bits_per_second(%d) exceeds \
				the maximum bitrate, set it with %d\n",
                                     rate_control_param->bits_per_second,
                                     TOPAZ_MPEG4_MAX_BITRATE);
        } else
            psRCParams->ui32BitsPerSecond = rate_control_param->bits_per_second;

        if (rate_control_param->window_size != 0)
            psRCParams->ui32BufferSize = psRCParams->ui32BitsPerSecond * rate_control_param->window_size / 1000;
        if (rate_control_param->initial_qp != 0)
            psRCParams->ui32InitialQp = rate_control_param->initial_qp;
        if (rate_control_param->min_qp != 0)
            psRCParams->iMinQP = rate_control_param->min_qp;
        break;
    default:
        break;
    }

    return vaStatus;
}

static VAStatus tng_MPEG4ES_RenderPicture(
    object_context_p obj_context,
    object_buffer_p *buffers,
    int num_buffers)
{
    INIT_CONTEXT_MPEG4ES;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    int i;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: start\n", __FUNCTION__);
    for (i = 0; i < num_buffers; i++) {
        object_buffer_p obj_buffer = buffers[i];
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: type = %d, num = %d\n", __FUNCTION__, obj_buffer->type, num_buffers);

        switch (obj_buffer->type) {
        case VAEncSequenceParameterBufferType:
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "tng_MPEG4_RenderPicture got VAEncSequenceParameterBufferType\n");
            vaStatus = tng__MPEG4ES_process_sequence_param(ctx, obj_buffer);
            DEBUG_FAILURE;
            break;
        case VAEncPictureParameterBufferType:
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "tng_MPEG4_RenderPicture got VAEncPictureParameterBuffer\n");
            vaStatus = tng__MPEG4ES_process_picture_param(ctx, obj_buffer);
            DEBUG_FAILURE;
            break;

        case VAEncSliceParameterBufferType:
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "tng_MPEG4_RenderPicture got VAEncSliceParameterBufferType\n");
            vaStatus = tng__MPEG4ES_process_slice_param(ctx, obj_buffer);
            DEBUG_FAILURE;
            break;

        case VAEncMiscParameterBufferType:
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "tng_MPEG4_RenderPicture got VAEncMiscParameterBufferType\n");
            vaStatus = tng__MPEG4ES_process_misc_param(ctx, obj_buffer);
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
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: end\n", __FUNCTION__);

    return vaStatus;
}

static VAStatus tng_MPEG4ES_EndPicture(
    object_context_p obj_context)
{
    INIT_CONTEXT_MPEG4ES;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s start\n", __FUNCTION__);
    vaStatus = tng_EndPicture(ctx);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s end\n", __FUNCTION__);

    return vaStatus;
}

struct format_vtable_s tng_MPEG4ES_vtable = {
queryConfigAttributes:
    tng_MPEG4ES_QueryConfigAttributes,
validateConfig:
    tng_MPEG4ES_ValidateConfig,
createContext:
    tng_MPEG4ES_CreateContext,
destroyContext:
    tng_MPEG4ES_DestroyContext,
beginPicture:
    tng_MPEG4ES_BeginPicture,
renderPicture:
    tng_MPEG4ES_RenderPicture,
endPicture:
    tng_MPEG4ES_EndPicture
};

/*EOF*/
