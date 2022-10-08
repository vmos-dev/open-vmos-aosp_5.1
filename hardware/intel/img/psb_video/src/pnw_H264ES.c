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


#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

#include "psb_def.h"
#include "psb_drv_debug.h"
#include "psb_surface.h"
#include "psb_cmdbuf.h"
#include "pnw_hostcode.h"
#include "pnw_H264ES.h"
#include "pnw_hostheader.h"
#include "va/va_enc_h264.h"
#define TOPAZ_H264_MAX_BITRATE 50000000

#define INIT_CONTEXT_H264ES     context_ENC_p ctx = (context_ENC_p) obj_context->format_data
#define SURFACE(id)    ((object_surface_p) object_heap_lookup( &ctx->obj_context->driver_data->surface_heap, id ))
#define BUFFER(id)  ((object_buffer_p) object_heap_lookup( &ctx->obj_context->driver_data->buffer_heap, id ))
static void pnw_H264ES_QueryConfigAttributes(
        VAProfile __maybe_unused profile,
        VAEntrypoint __maybe_unused entrypoint,
        VAConfigAttrib *attrib_list,
        int num_attribs)
{
    int i;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "pnw_H264ES_QueryConfigAttributes\n");

    /* RateControl attributes */
    for (i = 0; i < num_attribs; i++) {
        switch (attrib_list[i].type) {
            case VAConfigAttribRTFormat:
                break;

            case VAConfigAttribRateControl:
                attrib_list[i].value = VA_RC_NONE | VA_RC_CBR | VA_RC_VBR | VA_RC_VCM;
	        break;

            case VAConfigAttribEncMaxRefFrames:
                attrib_list[i].value = 1;
                break;

	    default:
		attrib_list[i].value = VA_ATTRIB_NOT_SUPPORTED;
		break;
	}
    }
}


static VAStatus pnw_H264ES_ValidateConfig(
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
            default:
                return VA_STATUS_ERROR_ATTR_NOT_SUPPORTED;
        }
    }

    return VA_STATUS_SUCCESS;
}


static VAStatus pnw_H264ES_CreateContext(
        object_context_p obj_context,
        object_config_p obj_config)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    context_ENC_p ctx;
    int i;
    unsigned int eRCmode;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "pnw_H264ES_CreateContext\n");

    vaStatus = pnw_CreateContext(obj_context, obj_config, 0);

    if (VA_STATUS_SUCCESS != vaStatus)
        return VA_STATUS_ERROR_ALLOCATION_FAILED;

    ctx = (context_ENC_p) obj_context->format_data;

    for (i = 0; i < obj_config->attrib_count; i++) {
        if (obj_config->attrib_list[i].type == VAConfigAttribRateControl)
            break;
    }

    if (i >= obj_config->attrib_count)
        eRCmode = VA_RC_NONE;
    else
        eRCmode = obj_config->attrib_list[i].value;


    if (eRCmode == VA_RC_VBR) {
        ctx->eCodec = IMG_CODEC_H264_VBR;
        ctx->sRCParams.RCEnable = IMG_TRUE;
        ctx->sRCParams.bDisableBitStuffing = IMG_FALSE;
    } else if (eRCmode == VA_RC_CBR) {
        ctx->eCodec = IMG_CODEC_H264_CBR;
        ctx->sRCParams.RCEnable = IMG_TRUE;
        ctx->sRCParams.bDisableBitStuffing = IMG_TRUE;
    } else if (eRCmode == VA_RC_NONE) {
        ctx->eCodec = IMG_CODEC_H264_NO_RC;
        ctx->sRCParams.RCEnable = IMG_FALSE;
        ctx->sRCParams.bDisableBitStuffing = IMG_FALSE;
    } else if (eRCmode == VA_RC_VCM) {
        ctx->eCodec = IMG_CODEC_H264_VCM;
        ctx->sRCParams.RCEnable = IMG_TRUE;
        ctx->sRCParams.bDisableBitStuffing = IMG_FALSE;
    } else
        return VA_STATUS_ERROR_UNSUPPORTED_RT_FORMAT;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "eCodec is %d\n", ctx->eCodec);
    ctx->eFormat = IMG_CODEC_PL12;      /* use default */

    ctx->Slices = 1;
    ctx->idr_pic_id = 1;
    ctx->buffer_size = 0;
    ctx->initial_buffer_fullness = 0;
    //initialize the frame_rate and qp
    ctx->sRCParams.FrameRate = 30;

    if (getenv("PSB_VIDEO_SIG_CORE") == NULL) {
        ctx->Slices = 2;
        ctx->NumCores = 2;
    }

    ctx->ParallelCores = min(ctx->NumCores, ctx->Slices);

    ctx->IPEControl = pnw__get_ipe_control(ctx->eCodec);

    switch (obj_config->profile) {
        case VAProfileH264Baseline:
            ctx->profile_idc = 5;
            break;
        case VAProfileH264Main:
            ctx->profile_idc = 6;
            break;
        default:
            ctx->profile_idc = 6;
            break;
    }

    return vaStatus;
}

static void pnw_H264ES_DestroyContext(
        object_context_p obj_context)
{
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "pnw_H264ES_DestroyPicture\n");

    pnw_DestroyContext(obj_context);
}

static VAStatus pnw_H264ES_BeginPicture(
        object_context_p obj_context)
{
    INIT_CONTEXT_H264ES;
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "pnw_H264ES_BeginPicture\n");

    vaStatus = pnw_BeginPicture(ctx);

    return vaStatus;
}

static VAStatus pnw__H264ES_process_sequence_param(context_ENC_p ctx, object_buffer_p obj_buffer)
{
    VAEncSequenceParameterBufferH264 *pSequenceParams;
    pnw_cmdbuf_p cmdbuf = ctx->obj_context->pnw_cmdbuf;
    H264_VUI_PARAMS *pVUI_Params = &(ctx->VUI_Params);
    H264_CROP_PARAMS sCrop;
    int i;
    unsigned int frame_size;
    unsigned int max_bps;

    ASSERT(obj_buffer->type == VAEncSequenceParameterBufferType);
    ASSERT(obj_buffer->num_elements == 1);
    ASSERT(obj_buffer->size == sizeof(VAEncSequenceParameterBufferH264));

    if ((obj_buffer->num_elements != 1) ||
            (obj_buffer->size != sizeof(VAEncSequenceParameterBufferH264))) {
        return VA_STATUS_ERROR_UNKNOWN;
    }

    if(ctx->sRCParams.FrameRate == 0)
        ctx->sRCParams.FrameRate = 30;
    ctx->obj_context->frame_count = 0;

    pSequenceParams = (VAEncSequenceParameterBufferH264 *) obj_buffer->buffer_data;
    obj_buffer->buffer_data = NULL;
    obj_buffer->size = 0;

    if (!pSequenceParams->bits_per_second) {
        pSequenceParams->bits_per_second = ctx->Height * ctx->Width * 30 * 12;
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "bits_per_second is 0, set to %d\n",
                pSequenceParams->bits_per_second);
    }
    ctx->sRCParams.bBitrateChanged =
        (pSequenceParams->bits_per_second == ctx->sRCParams.BitsPerSecond ?
         IMG_FALSE : IMG_TRUE);

    if (pSequenceParams->bits_per_second > TOPAZ_H264_MAX_BITRATE) {
        ctx->sRCParams.BitsPerSecond = TOPAZ_H264_MAX_BITRATE;
        drv_debug_msg(VIDEO_DEBUG_GENERAL, " bits_per_second(%d) exceeds \
                the maximum bitrate, set it with %d\n",
                pSequenceParams->bits_per_second,
                TOPAZ_H264_MAX_BITRATE);
    }

    /* According to Table A-1 Level limits, if resolution is bigger than 625SD,
       min compression ratio is 4, otherwise min compression ratio is 2 */
    max_bps =  (ctx->Width * ctx->Height * 3 / 2 ) * 8 * ctx->sRCParams.FrameRate;
    if (ctx->Width > 720)
        max_bps /= 4;
    else
        max_bps /= 2;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, " width %d height %d, frame rate %d\n",
            ctx->Width, ctx->Height, ctx->sRCParams.FrameRate);
    if (pSequenceParams->bits_per_second > max_bps) {
        drv_debug_msg(VIDEO_DEBUG_ERROR,
                "Invalid bitrate %d, violate ITU-T Rec. H.264 (03/2005) A.3.1"
                "\n clip to %d bps\n", pSequenceParams->bits_per_second, max_bps);
        ctx->sRCParams.BitsPerSecond = max_bps;
    } else {
        /* See 110% target bitrate for VCM. Otherwise, the resulted bitrate is much lower
           than target bitrate */
        if (ctx->eCodec == IMG_CODEC_H264_VCM)
            pSequenceParams->bits_per_second =
                pSequenceParams->bits_per_second / 100 * 110;

        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Bitrate is set to %d\n",
                pSequenceParams->bits_per_second);
        ctx->sRCParams.BitsPerSecond = pSequenceParams->bits_per_second;
    }

    /*if (ctx->sRCParams.IntraFreq != pSequenceParams->intra_period)
      ctx->sRCParams.bBitrateChanged = IMG_TRUE;*/
    ctx->sRCParams.IDRFreq = pSequenceParams->intra_idr_period;

    ctx->sRCParams.Slices = ctx->Slices;
    ctx->sRCParams.QCPOffset = 0;

    if (ctx->sRCParams.IntraFreq != pSequenceParams->intra_period
            && ctx->raw_frame_count != 0
            && ctx->sRCParams.IntraFreq != 0
            && ((ctx->obj_context->frame_count + 1) % ctx->sRCParams.IntraFreq) != 0
            && (!ctx->sRCParams.bDisableFrameSkipping)) {
        drv_debug_msg(VIDEO_DEBUG_ERROR,
                "Changing intra period value in the middle of a GOP is\n"
                "not allowed if frame skip isn't disabled.\n"
                "it can cause I frame been skipped\n");
        free(pSequenceParams);
        return VA_STATUS_ERROR_INVALID_PARAMETER;
    }
    else
        ctx->sRCParams.IntraFreq = pSequenceParams->intra_period;

    frame_size = ctx->sRCParams.BitsPerSecond / ctx->sRCParams.FrameRate;

    if (ctx->bInserHRDParams &&
            ctx->buffer_size != 0 && ctx->initial_buffer_fullness != 0) {
        ctx->sRCParams.BufferSize = ctx->buffer_size;
        ctx->sRCParams.InitialLevel = ctx->buffer_size - ctx->initial_buffer_fullness;
        ctx->sRCParams.InitialDelay = ctx->initial_buffer_fullness;
    }
    else {
        ctx->buffer_size = ctx->sRCParams.BitsPerSecond;
        ctx->initial_buffer_fullness = ctx->sRCParams.BitsPerSecond;
        ctx->sRCParams.BufferSize = ctx->buffer_size;
        ctx->sRCParams.InitialLevel = (3 * ctx->sRCParams.BufferSize) >> 4;
        /* Aligned with target frame size */
        ctx->sRCParams.InitialLevel += (frame_size / 2);
        ctx->sRCParams.InitialLevel /= frame_size;
        ctx->sRCParams.InitialLevel *= frame_size;
        ctx->sRCParams.InitialDelay = ctx->buffer_size - ctx->sRCParams.InitialLevel;
    }

    if (ctx->raw_frame_count == 0) {
        for (i = (ctx->ParallelCores - 1); i >= 0; i--)
            pnw_set_bias(ctx, i);
    }

    pVUI_Params->bit_rate_value_minus1 = ctx->sRCParams.BitsPerSecond / 64 - 1;
    pVUI_Params->cbp_size_value_minus1 = ctx->sRCParams.BufferSize / 64 - 1;
    if (IMG_CODEC_H264_CBR != ctx->eCodec ||
            ctx->sRCParams.bDisableBitStuffing ||
            ctx->sRCParams.bDisableFrameSkipping)
        pVUI_Params->CBR = 0;
    else
        pVUI_Params->CBR = 1;

    pVUI_Params->initial_cpb_removal_delay_length_minus1 = BPH_SEI_NAL_INITIAL_CPB_REMOVAL_DELAY_SIZE - 1;
    pVUI_Params->cpb_removal_delay_length_minus1 = PTH_SEI_NAL_CPB_REMOVAL_DELAY_SIZE - 1;
    pVUI_Params->dpb_output_delay_length_minus1 = PTH_SEI_NAL_DPB_OUTPUT_DELAY_SIZE - 1;
    pVUI_Params->time_offset_length = 24;
    ctx->bInsertVUI = pSequenceParams->vui_parameters_present_flag ? IMG_TRUE: IMG_FALSE;
    if (ctx->bInsertVUI) {
        if (pSequenceParams->num_units_in_tick !=0 && pSequenceParams->time_scale !=0
                && (pSequenceParams->time_scale > pSequenceParams->num_units_in_tick) ) {
            pVUI_Params->Time_Scale = pSequenceParams->time_scale;
            pVUI_Params->num_units_in_tick = pSequenceParams->num_units_in_tick;
        }
        else {
            pVUI_Params->num_units_in_tick = 1;
            pVUI_Params->Time_Scale = ctx->sRCParams.FrameRate * 2;
        }
    }

    if (ctx->bInsertVUI && pSequenceParams->vui_fields.bits.aspect_ratio_info_present_flag &&
            (pSequenceParams->aspect_ratio_idc == 0xff /* Extended_SAR */)) {
        pVUI_Params->aspect_ratio_info_present_flag = IMG_TRUE;
        pVUI_Params->aspect_ratio_idc = 0xff;
        pVUI_Params->sar_width = pSequenceParams->sar_width;
        pVUI_Params->sar_height = pSequenceParams->sar_height;
    }

    sCrop.bClip = pSequenceParams->frame_cropping_flag;
    sCrop.LeftCropOffset = 0;
    sCrop.RightCropOffset = 0;
    sCrop.TopCropOffset = 0;
    sCrop.BottomCropOffset = 0;

    if (!sCrop.bClip) {
        if (ctx->RawHeight & 0xf) {
            sCrop.bClip = IMG_TRUE;
            sCrop.BottomCropOffset = (((ctx->RawHeight + 0xf) & (~0xf)) - ctx->RawHeight) / 2;
        }
        if (ctx->RawWidth & 0xf) {
            sCrop.bClip = IMG_TRUE;
            sCrop.RightCropOffset = (((ctx->RawWidth + 0xf) & (~0xf)) - ctx->RawWidth) / 2;
        }
    } else {
        sCrop.LeftCropOffset = pSequenceParams->frame_crop_left_offset;
        sCrop.RightCropOffset = pSequenceParams->frame_crop_right_offset;
        sCrop.TopCropOffset = pSequenceParams->frame_crop_top_offset;
        sCrop.BottomCropOffset = pSequenceParams->frame_crop_bottom_offset;
    }
    /* sequence header is always inserted */

    memset(cmdbuf->header_mem_p + ctx->seq_header_ofs,
            0,
            HEADER_SIZE);

    /*
       if (ctx->bInserHRDParams) {
       memset(cmdbuf->header_mem_p + ctx->aud_header_ofs,
       0,
       HEADER_SIZE);

       pnw__H264_prepare_AUD_header(cmdbuf->header_mem_p + ctx->aud_header_ofs);
       pnw_cmdbuf_insert_command_package(ctx->obj_context,
       ctx->ParallelCores - 1,
       MTX_CMDID_DO_HEADER,
       &cmdbuf->header_mem,
       ctx->aud_header_ofs);
       }
     */
    if (ctx->eCodec == IMG_CODEC_H264_NO_RC)
        pnw__H264_prepare_sequence_header(cmdbuf->header_mem_p + ctx->seq_header_ofs,
                pSequenceParams->picture_width_in_mbs,
                pSequenceParams->picture_height_in_mbs,
                pSequenceParams->vui_parameters_present_flag,
                pSequenceParams->vui_parameters_present_flag ? (pVUI_Params) : NULL,
                &sCrop,
                pSequenceParams->level_idc, ctx->profile_idc);
    else
        pnw__H264_prepare_sequence_header(cmdbuf->header_mem_p + ctx->seq_header_ofs,
                pSequenceParams->picture_width_in_mbs,
                pSequenceParams->picture_height_in_mbs,
                pSequenceParams->vui_parameters_present_flag,
                pSequenceParams->vui_parameters_present_flag ? (pVUI_Params) : NULL,
                &sCrop,
                pSequenceParams->level_idc, ctx->profile_idc);

    /*Periodic IDR need SPS. We save the sequence header here*/
    if (ctx->sRCParams.IDRFreq != 0) {
        if (NULL == ctx->save_seq_header_p) {
            ctx->save_seq_header_p = malloc(HEADER_SIZE);
            if (NULL == ctx->save_seq_header_p) {
                drv_debug_msg(VIDEO_DEBUG_ERROR, "Ran out of memory!\n");
                free(pSequenceParams);
                return VA_STATUS_ERROR_ALLOCATION_FAILED;
            }
            memcpy((unsigned char *)ctx->save_seq_header_p,
                    (unsigned char *)(cmdbuf->header_mem_p + ctx->seq_header_ofs),
                    HEADER_SIZE);
        }
    }
    ctx->none_vcl_nal++;
    cmdbuf->cmd_idx_saved[PNW_CMDBUF_SEQ_HEADER_IDX] = cmdbuf->cmd_idx;
    /* Send to the last core as this will complete first */
    pnw_cmdbuf_insert_command_package(ctx->obj_context,
            ctx->ParallelCores - 1,
            MTX_CMDID_DO_HEADER,
            &cmdbuf->header_mem,
            ctx->seq_header_ofs);
    free(pSequenceParams);

    return VA_STATUS_SUCCESS;
}


static VAStatus pnw__H264ES_insert_SEI_buffer_period(context_ENC_p ctx)
{
    unsigned int ui32nal_initial_cpb_removal_delay;
    unsigned int ui32nal_initial_cpb_removal_delay_offset;
    pnw_cmdbuf_p cmdbuf = ctx->obj_context->pnw_cmdbuf;

    ui32nal_initial_cpb_removal_delay =
        90000 * (1.0 * ctx->sRCParams.InitialDelay / ctx->sRCParams.BitsPerSecond);
    ui32nal_initial_cpb_removal_delay_offset =
        90000 * (1.0 * ctx->buffer_size / ctx->sRCParams.BitsPerSecond)
        - ui32nal_initial_cpb_removal_delay;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Insert SEI buffer period message with "
            "ui32nal_initial_cpb_removal_delay(%d) and "
            "ui32nal_initial_cpb_removal_delay_offset(%d)\n",
            ui32nal_initial_cpb_removal_delay,
            ui32nal_initial_cpb_removal_delay_offset);

    memset(cmdbuf->header_mem_p + ctx->sei_buf_prd_ofs,
            0,
            HEADER_SIZE);

    pnw__H264_prepare_SEI_buffering_period_header(
            (MTX_HEADER_PARAMS *)(cmdbuf->header_mem_p + ctx->sei_buf_prd_ofs),
            1, //ui8NalHrdBpPresentFlag,
            0, //ui8nal_cpb_cnt_minus1,
            1 + ctx->VUI_Params.initial_cpb_removal_delay_length_minus1, //ui8nal_initial_cpb_removal_delay_length,
            ui32nal_initial_cpb_removal_delay, //ui32nal_initial_cpb_removal_delay,
            ui32nal_initial_cpb_removal_delay_offset, //ui32nal_initial_cpb_removal_delay_offset,
            0, //ui8VclHrdBpPresentFlag,
            NOT_USED_BY_TOPAZ, //ui8vcl_cpb_cnt_minus1,
            0, //ui32vcl_initial_cpb_removal_delay,
            0 //ui32vcl_initial_cpb_removal_delay_offset
            );
    cmdbuf->cmd_idx_saved[PNW_CMDBUF_SEI_BUF_PERIOD_IDX] = cmdbuf->cmd_idx;
    pnw_cmdbuf_insert_command_package(ctx->obj_context,
            ctx->ParallelCores - 1,
            MTX_CMDID_DO_HEADER,
            &cmdbuf->header_mem,
            ctx->sei_buf_prd_ofs);

    ctx->none_vcl_nal++;
    return VA_STATUS_SUCCESS;
}


static VAStatus pnw__H264ES_insert_SEI_pic_timing(context_ENC_p ctx)
{
    pnw_cmdbuf_p cmdbuf = ctx->obj_context->pnw_cmdbuf;
    uint32_t ui32cpb_removal_delay;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Insert SEI picture timing message. \n");

    memset(cmdbuf->header_mem_p + ctx->sei_pic_tm_ofs,
            0,
            HEADER_SIZE);

    /* ui32cpb_removal_delay is zero for 1st frame and will be reset
     * after a IDR frame */
    if (ctx->obj_context->frame_count == 0) {
        if (ctx->raw_frame_count == 0)
            ui32cpb_removal_delay = 0;
        else
            ui32cpb_removal_delay =
                ctx->sRCParams.IDRFreq * ctx->sRCParams.IntraFreq * 2;
    } else
        ui32cpb_removal_delay = 2 * ctx->obj_context->frame_count;

    pnw__H264_prepare_SEI_picture_timing_header(
            (MTX_HEADER_PARAMS *)(cmdbuf->header_mem_p + ctx->sei_pic_tm_ofs),
            1,
            ctx->VUI_Params.cpb_removal_delay_length_minus1,
            ctx->VUI_Params.dpb_output_delay_length_minus1,
            ui32cpb_removal_delay, //ui32cpb_removal_delay,
            2, //ui32dpb_output_delay,
            0, //ui8pic_struct_present_flag,
            0, //ui8pic_struct,
            0, //ui8NumClockTS,
            0, //*aui8clock_timestamp_flag,
            0, //ui8full_timestamp_flag,
            0, //ui8seconds_flag,
            0, //ui8minutes_flag,
            0, //ui8hours_flag,
            0, //ui8seconds_value,
            0, //ui8minutes_value,
            0, //ui8hours_value,
            0, //ui8ct_type,
            0, //ui8nuit_field_based_flag,
            0, //ui8counting_type,
            0, //ui8discontinuity_flag,
            0, //ui8cnt_dropped_flag,
            0, //ui8n_frames,
            0, //ui8time_offset_length,
            0 //i32time_offset)
                );

    cmdbuf->cmd_idx_saved[PNW_CMDBUF_SEI_PIC_TIMING_IDX] = cmdbuf->cmd_idx;
    pnw_cmdbuf_insert_command_package(ctx->obj_context,
            ctx->ParallelCores - 1,
            MTX_CMDID_DO_HEADER,
            &cmdbuf->header_mem,
            ctx->sei_pic_tm_ofs);

    ctx->none_vcl_nal++;
    return VA_STATUS_SUCCESS;
}

#if PSB_MFLD_DUMMY_CODE
static VAStatus pnw__H264ES_insert_SEI_FPA_param(context_ENC_p ctx, object_buffer_p obj_buffer)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    pnw_cmdbuf_p cmdbuf = ctx->obj_context->pnw_cmdbuf;
    VAEncPackedHeaderParameterBuffer *sei_param_buf = (VAEncPackedHeaderParameterBuffer *)obj_buffer->buffer_data;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Insert SEI frame packing arrangement message. \n");
    ctx->sei_pic_data_size = sei_param_buf->bit_length/8;

    return VA_STATUS_SUCCESS;
}

static VAStatus pnw__H264ES_insert_SEI_FPA_data(context_ENC_p ctx, object_buffer_p obj_buffer)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    pnw_cmdbuf_p cmdbuf = ctx->obj_context->pnw_cmdbuf;
    char *sei_data_buf;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Insert SEI frame packing arrangement message. \n");

    memset(cmdbuf->header_mem_p + ctx->sei_pic_fpa_ofs,
            0,
            HEADER_SIZE);
    sei_data_buf = (char *)obj_buffer->buffer_data;

    pnw__H264_prepare_SEI_FPA_header((MTX_HEADER_PARAMS *)(cmdbuf->header_mem_p + ctx->sei_pic_fpa_ofs), sei_data_buf, ctx->sei_pic_data_size);
    pnw_cmdbuf_insert_command_package(ctx->obj_context,
            ctx->ParallelCores - 1,
            MTX_CMDID_DO_HEADER,
            &cmdbuf->header_mem,
            ctx->sei_pic_fpa_ofs);

    return VA_STATUS_SUCCESS;
}
#endif

static VAStatus pnw__H264ES_process_picture_param(context_ENC_p ctx, object_buffer_p obj_buffer)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    int i;
    VAEncPictureParameterBufferH264 *pBuffer;
    int need_sps = 0;
    pnw_cmdbuf_p cmdbuf = ctx->obj_context->pnw_cmdbuf;

    ASSERT(obj_buffer->type == VAEncPictureParameterBufferType);

    if ((obj_buffer->num_elements != 1) ||
            (obj_buffer->size != sizeof(VAEncPictureParameterBufferH264))) {
        return VA_STATUS_ERROR_UNKNOWN;
    }

    /* Transfer ownership of VAEncPictureParameterBufferH264 data */
    pBuffer = (VAEncPictureParameterBufferH264 *) obj_buffer->buffer_data;
    obj_buffer->buffer_data = NULL;
    obj_buffer->size = 0;

    ctx->ref_surface = SURFACE(pBuffer->ReferenceFrames[0].picture_id);
    ctx->dest_surface = SURFACE(pBuffer->CurrPic.picture_id);
    ctx->coded_buf = BUFFER(pBuffer->coded_buf);

    //ASSERT(ctx->Width == pBuffer->picture_width);
    //ASSERT(ctx->Height == pBuffer->picture_height);

    if (NULL == ctx->coded_buf) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s L%d Invalid coded buffer handle\n", __FUNCTION__, __LINE__);
        free(pBuffer);
        return VA_STATUS_ERROR_INVALID_BUFFER;
    }
    if ((ctx->sRCParams.IntraFreq != 0) && (ctx->sRCParams.IDRFreq != 0)) { /* period IDR is desired */
        unsigned int is_intra = 0;
        unsigned int intra_cnt = 0;

        ctx->force_idr_h264 = 0;

        if ((ctx->obj_context->frame_count % ctx->sRCParams.IntraFreq) == 0) {
            is_intra = 1; /* suppose current frame is I frame */
            intra_cnt = ctx->obj_context->frame_count / ctx->sRCParams.IntraFreq;
        }

        /* current frame is I frame (suppose), and an IDR frame is desired*/
        if ((is_intra) && ((intra_cnt % ctx->sRCParams.IDRFreq) == 0)) {
            ctx->force_idr_h264 = 1;
            /*When two consecutive access units in decoding order are both IDR access
             * units, the value of idr_pic_id in the slices of the first such IDR
             * access unit shall differ from the idr_pic_id in the second such IDR
             * access unit. We set it with 1 or 0 alternately.*/
            ctx->idr_pic_id = 1 - ctx->idr_pic_id;

            /* it is periodic IDR in the middle of one sequence encoding, need SPS */
            if (ctx->obj_context->frame_count > 0)
                need_sps = 1;

            ctx->obj_context->frame_count = 0;
        }
    }

    /* If VUI header isn't enabled, we'll igore the request for HRD header insertion */
    if (ctx->bInserHRDParams)
        ctx->bInserHRDParams = ctx->bInsertVUI;

    /* For H264, PicHeader only needed in the first picture*/
    if (!(ctx->obj_context->frame_count)) {
        cmdbuf = ctx->obj_context->pnw_cmdbuf;

        if (need_sps) {
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "TOPAZ: insert a SPS before IDR frame\n");
            /* reuse the previous SPS */
            memcpy((unsigned char *)(cmdbuf->header_mem_p + ctx->seq_header_ofs),
                    (unsigned char *)ctx->save_seq_header_p,
                    HEADER_SIZE);

            cmdbuf->cmd_idx_saved[PNW_CMDBUF_SEQ_HEADER_IDX] = cmdbuf->cmd_idx;
            /* Send to the last core as this will complete first */
            pnw_cmdbuf_insert_command_package(ctx->obj_context,
                    ctx->ParallelCores - 1,
                    MTX_CMDID_DO_HEADER,
                    &cmdbuf->header_mem,
                    ctx->seq_header_ofs);
            ctx->none_vcl_nal++;
        }

        if (ctx->bInserHRDParams) {
            pnw__H264ES_insert_SEI_buffer_period(ctx);
            pnw__H264ES_insert_SEI_pic_timing(ctx);
        }

        pnw__H264_prepare_picture_header(cmdbuf->header_mem_p + ctx->pic_header_ofs, IMG_FALSE, ctx->sRCParams.QCPOffset);

        cmdbuf->cmd_idx_saved[PNW_CMDBUF_PIC_HEADER_IDX] = cmdbuf->cmd_idx;
        /* Send to the last core as this will complete first */
        pnw_cmdbuf_insert_command_package(ctx->obj_context,
                ctx->ParallelCores - 1,
                MTX_CMDID_DO_HEADER,
                &cmdbuf->header_mem,
                ctx->pic_header_ofs);
        ctx->none_vcl_nal++;
    }
    else if (ctx->bInserHRDParams)
        pnw__H264ES_insert_SEI_pic_timing(ctx);

    if (ctx->ParallelCores == 1) {
        ctx->coded_buf_per_slice = 0;
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "TOPAZ: won't splite coded buffer(%d) since only one slice being encoded\n",
                ctx->coded_buf->size);
    } else {
        /*Make sure DMA start is 128bits alignment*/
        ctx->coded_buf_per_slice = (ctx->coded_buf->size / ctx->ParallelCores) & (~0xf) ;
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "TOPAZ: the size of coded_buf per slice %d( Total %d) \n", ctx->coded_buf_per_slice,
                ctx->coded_buf->size);
    }

    /* Prepare START_PICTURE params */
    /* FIXME is really need multiple picParams? Need multiple calculate for each? */
    for (i = (ctx->ParallelCores - 1); i >= 0; i--)
        vaStatus = pnw_RenderPictureParameter(ctx, i);

    free(pBuffer);
    return vaStatus;
}

static VAStatus pnw__H264ES_encode_one_slice(context_ENC_p ctx,
        VAEncSliceParameterBuffer *pBuffer)
{
    pnw_cmdbuf_p cmdbuf = ctx->obj_context->pnw_cmdbuf;
    unsigned int MBSkipRun, FirstMBAddress;
    unsigned char deblock_idc;
    unsigned char is_intra = 0;
    int slice_param_idx;
    PIC_PARAMS *psPicParams = (PIC_PARAMS *)(cmdbuf->pic_params_p);
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    /*Slice encoding Order:
     *1.Insert Do header command
     *2.setup InRowParams
     *3.setup Slice params
     *4.Insert Do slice command
     * */

    if (pBuffer->slice_height > (ctx->Height / 16) ||
           pBuffer->start_row_number > (ctx->Height / 16) ||
           (pBuffer->slice_height + pBuffer->start_row_number) > (ctx->Height / 16)) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "slice height %d or start row number %d is too large",
                pBuffer->slice_height,  pBuffer->start_row_number);
        return VA_STATUS_ERROR_INVALID_PARAMETER;
    }
    MBSkipRun = (pBuffer->slice_height * ctx->Width) / 16;
    deblock_idc = pBuffer->slice_flags.bits.disable_deblocking_filter_idc;

    /*If the frame is skipped, it shouldn't be a I frame*/
    if (ctx->force_idr_h264 || (ctx->obj_context->frame_count == 0)) {
        is_intra = 1;
    } else
        is_intra = (ctx->sRCParams.RCEnable && ctx->sRCParams.FrameSkip) ? 0 : pBuffer->slice_flags.bits.is_intra;

    FirstMBAddress = (pBuffer->start_row_number * ctx->Width) / 16;

    memset(cmdbuf->header_mem_p + ctx->slice_header_ofs
            + ctx->obj_context->slice_count * HEADER_SIZE,
            0,
            HEADER_SIZE);

    /* Insert Do Header command, relocation is needed */
    pnw__H264_prepare_slice_header(cmdbuf->header_mem_p + ctx->slice_header_ofs
            + ctx->obj_context->slice_count * HEADER_SIZE,
            is_intra,
            pBuffer->slice_flags.bits.disable_deblocking_filter_idc,
            ctx->obj_context->frame_count,
            FirstMBAddress,
            MBSkipRun,
            0,
            ctx->force_idr_h264,
            IMG_FALSE,
            IMG_FALSE,
            ctx->idr_pic_id);

    /* ensure that this slice is consequtive to that last processed by the target core */
    /*
       ASSERT( -1 == ctx->LastSliceNum[ctx->SliceToCore]
       || ctx->obj_context->slice_count == 1 + ctx->LastSliceNum[ctx->SliceToCore] );
     */
    /* note the slice number the target core is now processing */
    ctx->LastSliceNum[ctx->SliceToCore] = ctx->obj_context->slice_count;

    pnw_cmdbuf_insert_command_package(ctx->obj_context,
            ctx->SliceToCore,
            MTX_CMDID_DO_HEADER,
            &cmdbuf->header_mem,
            ctx->slice_header_ofs + ctx->obj_context->slice_count * HEADER_SIZE);
    if (!(ctx->sRCParams.RCEnable && ctx->sRCParams.FrameSkip)) {
        /*Only reset on the first frame. It's more effective than DDK. Have confirmed with IMG*/
        if (ctx->obj_context->frame_count == 0)
            pnw_reset_encoder_params(ctx);
        if ((pBuffer->start_row_number == 0) && pBuffer->slice_flags.bits.is_intra) {
            ctx->BelowParamsBufIdx = (ctx->BelowParamsBufIdx + 1) & 0x1;
        }

        slice_param_idx = (pBuffer->slice_flags.bits.is_intra ? 0 : 1) * ctx->slice_param_num
            + ctx->obj_context->slice_count;
        if (VAEncSliceParameter_Equal(&ctx->slice_param_cache[slice_param_idx], pBuffer) == 0) {
            /* cache current param parameters */
            memcpy(&ctx->slice_param_cache[slice_param_idx],
                    pBuffer, sizeof(VAEncSliceParameterBuffer));

            /* Setup InParams value*/
            pnw_setup_slice_params(ctx,
                    pBuffer->start_row_number * 16,
                    pBuffer->slice_height * 16,
                    pBuffer->slice_flags.bits.is_intra,
                    ctx->obj_context->frame_count > 0,
                    psPicParams->sInParams.SeInitQP);
        }

        /* Insert do slice command and setup related buffer value */
        pnw__send_encode_slice_params(ctx,
                pBuffer->slice_flags.bits.is_intra,
                pBuffer->start_row_number * 16,
                deblock_idc,
                ctx->obj_context->frame_count,
                pBuffer->slice_height * 16,
                ctx->obj_context->slice_count);

        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Now frame_count/slice_count is %d/%d\n",
                ctx->obj_context->frame_count, ctx->obj_context->slice_count);
    }
    ctx->obj_context->slice_count++;

    return vaStatus;
}

/* convert from VAEncSliceParameterBufferH264  to VAEncSliceParameterBuffer */
static VAStatus pnw__convert_sliceparameter_buffer(VAEncSliceParameterBufferH264 *pBufferH264,
                                                   VAEncSliceParameterBuffer *pBuffer,
                                                   int picture_width_in_mbs,
                                                   unsigned int num_elemenent)
{
    unsigned int i;

    for (i = 0; i < num_elemenent; i++) {
        pBuffer->start_row_number = pBufferH264->macroblock_address / picture_width_in_mbs;
        pBuffer->slice_height =  pBufferH264->num_macroblocks / picture_width_in_mbs;
        pBuffer->slice_flags.bits.is_intra =
            (((pBufferH264->slice_type == 2) || (pBufferH264->slice_type == 7)) ? 1 : 0);
        pBuffer->slice_flags.bits.disable_deblocking_filter_idc =  pBufferH264->disable_deblocking_filter_idc;

        /* next conversion */
        pBuffer++;
        pBufferH264++;
    }

    return 0;
}

static VAStatus pnw__H264ES_process_slice_param(context_ENC_p ctx, object_buffer_p obj_buffer)
{
    /* Prepare InParams for macros of current slice, insert slice header, insert do slice command */
    VAEncSliceParameterBuffer *pBuf_per_core, *pBuffer;
    pnw_cmdbuf_p cmdbuf = ctx->obj_context->pnw_cmdbuf;
    PIC_PARAMS *psPicParams = (PIC_PARAMS *)(cmdbuf->pic_params_p);
    unsigned int i, j, slice_per_core;
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    ASSERT(obj_buffer->type == VAEncSliceParameterBufferType);

    if (obj_buffer->num_elements > (ctx->Height / 16)) {
        vaStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
        goto out2;
    }
    cmdbuf = ctx->obj_context->pnw_cmdbuf;
    psPicParams = (PIC_PARAMS *)cmdbuf->pic_params_p;

    /* Transfer ownership of VAEncPictureParameterBuffer data */
    if (obj_buffer->size == sizeof(VAEncSliceParameterBufferH264)) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Receive VAEncSliceParameterBufferH264 buffer");
        pBuffer = calloc(obj_buffer->num_elements, sizeof(VAEncSliceParameterBuffer));

        if (pBuffer == NULL) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "Run out of memory!\n");
            vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
            goto out2;
        }

        pnw__convert_sliceparameter_buffer((VAEncSliceParameterBufferH264 *)obj_buffer->buffer_data,
                                           pBuffer,
                                           ctx->Width / 16,
                                           obj_buffer->num_elements);
    } else if (obj_buffer->size == sizeof(VAEncSliceParameterBuffer)) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Receive VAEncSliceParameterBuffer buffer");
        pBuffer = (VAEncSliceParameterBuffer *) obj_buffer->buffer_data;
    } else {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Buffer size(%d) is wrong. It should be %d or %d\n",
                obj_buffer->size, sizeof(VAEncSliceParameterBuffer),
                sizeof(VAEncSliceParameterBufferH264));
        vaStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
        goto out2;
    }

    obj_buffer->size = 0;

    /*In case the slice number changes*/
    if ((ctx->slice_param_cache != NULL) && (obj_buffer->num_elements != ctx->slice_param_num)) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Slice number changes. Previous value is %d. Now it's %d\n",
                ctx->slice_param_num, obj_buffer->num_elements);
        free(ctx->slice_param_cache);
        ctx->slice_param_cache = NULL;
        ctx->slice_param_num = 0;
    }

    if (NULL == ctx->slice_param_cache) {
        ctx->slice_param_num = obj_buffer->num_elements;
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Allocate %d VAEncSliceParameterBuffer cache buffers\n", 2 * ctx->slice_param_num);
        ctx->slice_param_cache = calloc(2 * ctx->slice_param_num, sizeof(VAEncSliceParameterBuffer));
        if (NULL == ctx->slice_param_cache) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "Run out of memory!\n");

            /* free the converted VAEncSliceParameterBuffer */
            if (obj_buffer->size == sizeof(VAEncSliceParameterBufferH264))
                free(pBuffer);
            free(obj_buffer->buffer_data);
            return VA_STATUS_ERROR_ALLOCATION_FAILED;
        }
    }

    ctx->sRCParams.Slices = obj_buffer->num_elements;
    if (getenv("PSB_VIDEO_SIG_CORE") == NULL) {
        if ((ctx->ParallelCores == 2) && (obj_buffer->num_elements == 1)) {
            /*Need to replace unneccesary MTX_CMDID_STARTPICs with MTX_CMDID_PAD*/
            for (i = 0; i < (ctx->ParallelCores - 1); i++) {
                *(cmdbuf->cmd_idx_saved[PNW_CMDBUF_START_PIC_IDX] + i * 4) &= (~MTX_CMDWORD_ID_MASK);
                *(cmdbuf->cmd_idx_saved[PNW_CMDBUF_START_PIC_IDX] + i * 4) |= MTX_CMDID_PAD;
            }
            drv_debug_msg(VIDEO_DEBUG_GENERAL, " Remove unneccesary %d MTX_CMDID_STARTPIC commands from cmdbuf\n",
                    ctx->ParallelCores - obj_buffer->num_elements);
            ctx->ParallelCores = obj_buffer->num_elements;

            /* All header generation commands should be send to core 0*/
            for (i = PNW_CMDBUF_SEQ_HEADER_IDX; i < PNW_CMDBUF_SAVING_MAX; i++) {
                if (cmdbuf->cmd_idx_saved[i] != 0)
                    *(cmdbuf->cmd_idx_saved[i]) &=
                        ~(MTX_CMDWORD_CORE_MASK << MTX_CMDWORD_CORE_SHIFT);
            }

            ctx->SliceToCore = ctx->ParallelCores - 1;
        }
    }

    slice_per_core = obj_buffer->num_elements / ctx->ParallelCores;
    pBuf_per_core = pBuffer;
    for (i = 0; i < slice_per_core; i++) {
        pBuffer = pBuf_per_core;
        for (j = 0; j < ctx->ParallelCores; j++) {
            vaStatus = pnw__H264ES_encode_one_slice(ctx, pBuffer);
            if (vaStatus != VA_STATUS_SUCCESS)
                goto out1;
            if (0 == ctx->SliceToCore) {
                ctx->SliceToCore = ctx->ParallelCores;
            }
            ctx->SliceToCore--;

            ASSERT(ctx->obj_context->slice_count < MAX_SLICES_PER_PICTURE);
            /*Move to the next buffer which will be sent to core j*/
            pBuffer += slice_per_core;
        }
        pBuf_per_core++; /* Move to the next buffer */
    }

    /*Cope with last slice when slice number is odd and parallelCores is even*/
    if (obj_buffer->num_elements > (slice_per_core * ctx->ParallelCores)) {
        ctx->SliceToCore = 0;
        pBuffer -= slice_per_core;
        pBuffer ++;
        vaStatus = pnw__H264ES_encode_one_slice(ctx, pBuffer);
    }
out1:
    /* free the converted VAEncSliceParameterBuffer */
    if (obj_buffer->size == sizeof(VAEncSliceParameterBufferH264))
        free(pBuffer);

out2:
    free(obj_buffer->buffer_data);
    obj_buffer->buffer_data = NULL;

    return vaStatus;
}

static VAStatus pnw__H264ES_process_misc_param(context_ENC_p ctx, object_buffer_p obj_buffer)
{
    /* Prepare InParams for macros of current slice, insert slice header, insert do slice command */
    VAEncMiscParameterBuffer *pBuffer;
    VAEncMiscParameterRateControl *rate_control_param;
    VAEncMiscParameterAIR *air_param;
    VAEncMiscParameterMaxSliceSize *max_slice_size_param;
    VAEncMiscParameterFrameRate *frame_rate_param;
    VAEncMiscParameterHRD *hrd_param;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    unsigned int max_bps;
    unsigned int frame_size;

    ASSERT(obj_buffer->type == VAEncMiscParameterBufferType);


    /* Transfer ownership of VAEncMiscParameterBuffer data */
    pBuffer = (VAEncMiscParameterBuffer *) obj_buffer->buffer_data;
    obj_buffer->size = 0;

    if (ctx->eCodec != IMG_CODEC_H264_VCM
            && (pBuffer->type != VAEncMiscParameterTypeHRD
                && pBuffer->type != VAEncMiscParameterTypeRateControl
                && pBuffer->type != VAEncMiscParameterTypeFrameRate)) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Buffer type %d isn't supported in none VCM mode.\n",
                pBuffer->type);
        free(obj_buffer->buffer_data);
        obj_buffer->buffer_data = NULL;
        return VA_STATUS_SUCCESS;
    }

    switch (pBuffer->type) {
        case VAEncMiscParameterTypeFrameRate:
            frame_rate_param = (VAEncMiscParameterFrameRate *)pBuffer->data;

            if (frame_rate_param->framerate < 1 || frame_rate_param->framerate > 65535) {
                vaStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
                break;
            }

            if (ctx->sRCParams.FrameRate == frame_rate_param->framerate)
                break;

            drv_debug_msg(VIDEO_DEBUG_GENERAL, "frame rate changed from %d to %d\n",
                    ctx->sRCParams.FrameRate,
                    frame_rate_param->framerate);
            ctx->sRCParams.FrameRate = frame_rate_param->framerate;
            ctx->sRCParams.bBitrateChanged = IMG_TRUE;

            ctx->sRCParams.FrameRate = (frame_rate_param->framerate < 1) ? 1 :
                ((65535 < frame_rate_param->framerate) ? 65535 : frame_rate_param->framerate);
            break;

        case VAEncMiscParameterTypeRateControl:
            rate_control_param = (VAEncMiscParameterRateControl *)pBuffer->data;

            /* Currently, none VCM mode only supports frame skip and bit stuffing
             * disable flag and doesn't accept other parameters in
             * buffer of VAEncMiscParameterTypeRateControl type */
            if (rate_control_param->rc_flags.value != 0 || ctx->raw_frame_count == 0) {
                if (rate_control_param->rc_flags.bits.disable_frame_skip)
                    ctx->sRCParams.bDisableFrameSkipping = IMG_TRUE;
                if (rate_control_param->rc_flags.bits.disable_bit_stuffing)
                    ctx->sRCParams.bDisableBitStuffing = IMG_TRUE;
                drv_debug_msg(VIDEO_DEBUG_GENERAL,
                        "bDisableFrameSkipping is %d and bDisableBitStuffing is %d\n",
                        ctx->sRCParams.bDisableFrameSkipping, ctx->sRCParams.bDisableBitStuffing);
            }

            if (rate_control_param->initial_qp > 51 ||
                    rate_control_param->min_qp > 51) {
                drv_debug_msg(VIDEO_DEBUG_ERROR, "Initial_qp(%d) and min_qpinitial_qp(%d) "
                        "are invalid.\nQP shouldn't be larger than 51 for H264\n",
                        rate_control_param->initial_qp, rate_control_param->min_qp);
                vaStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
                break;
            }

            if (rate_control_param->window_size > 2000) {
                drv_debug_msg(VIDEO_DEBUG_ERROR, "window_size is too much!\n");
                vaStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
                break;
            }

            /* Check if any none-zero RC parameter is changed*/
            if ((rate_control_param->bits_per_second == 0 ||
                        rate_control_param->bits_per_second == ctx->sRCParams.BitsPerSecond) &&
                    (rate_control_param->window_size == 0 ||
                    ctx->sRCParams.BufferSize == ctx->sRCParams.BitsPerSecond / 1000 * rate_control_param->window_size) &&
                    (ctx->sRCParams.MinQP == rate_control_param->min_qp) &&
                    (ctx->sRCParams.InitialQp == rate_control_param->initial_qp) &&
                    (rate_control_param->basic_unit_size == 0 ||
                     ctx->sRCParams.BUSize == rate_control_param->basic_unit_size)) {
		     drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s No RC parameter is changed\n",
				     __FUNCTION__);
                break;
	    }
            else if (ctx->raw_frame_count != 0 || ctx->eCodec == IMG_CODEC_H264_VCM)
                ctx->sRCParams.bBitrateChanged = IMG_TRUE;

            /* The initial target bitrate is set by Sequence parameter buffer.
               Here is for changed bitrate only */
            if (rate_control_param->bits_per_second > TOPAZ_H264_MAX_BITRATE) {
                drv_debug_msg(VIDEO_DEBUG_ERROR, " bits_per_second(%d) exceeds \
                        the maximum bitrate, set it with %d\n",
                        rate_control_param->bits_per_second,
                        TOPAZ_H264_MAX_BITRATE);
                break;
            }
            /* The initial target bitrate is set by Sequence parameter buffer.
               Here is for changed bitrate only */
            if (rate_control_param->bits_per_second != 0 &&
                    ctx->raw_frame_count != 0) {
                drv_debug_msg(VIDEO_DEBUG_GENERAL,
                        "bitrate is changed from %d to %d on frame %d\n",
                        ctx->sRCParams.BitsPerSecond,
                        rate_control_param->bits_per_second,
                        ctx->raw_frame_count);
                max_bps =  (ctx->Width * ctx->Height * 3 / 2 ) * 8 * ctx->sRCParams.FrameRate;
                if (ctx->Width > 720)
                    max_bps /= 4;
                else
                    max_bps /= 2;

                drv_debug_msg(VIDEO_DEBUG_GENERAL, " width %d height %d, frame rate %d\n",
                        ctx->Width, ctx->Height, ctx->sRCParams.FrameRate);
                if (rate_control_param->bits_per_second > max_bps) {
                    drv_debug_msg(VIDEO_DEBUG_ERROR,
                            "Invalid bitrate %d, violate ITU-T Rec. H.264 (03/2005) A.3.1"
                            "\n clip to %d bps\n", rate_control_param->bits_per_second, max_bps);
                    ctx->sRCParams.BitsPerSecond = max_bps;
                } else {
                    /* See 110% target bitrate for VCM. Otherwise, the resulted bitrate is much lower
                       than target bitrate */
                    if (ctx->eCodec == IMG_CODEC_H264_VCM)
                        rate_control_param->bits_per_second =
                            rate_control_param->bits_per_second / 100 * 110;
                    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Bitrate is set to %d\n",
                            rate_control_param->bits_per_second);
                    ctx->sRCParams.BitsPerSecond = rate_control_param->bits_per_second;
                }
            }

            if (rate_control_param->min_qp != 0)
                ctx->sRCParams.MinQP = rate_control_param->min_qp;
            if (rate_control_param->window_size != 0) {
                ctx->sRCParams.BufferSize =
                    ctx->sRCParams.BitsPerSecond / 1000 * rate_control_param->window_size;
		if (ctx->sRCParams.FrameRate == 0) {
                    drv_debug_msg(VIDEO_DEBUG_ERROR, "frame rate can't be zero. Set it to 30");
                    ctx->sRCParams.FrameRate = 30;
                }

                frame_size = ctx->sRCParams.BitsPerSecond / ctx->sRCParams.FrameRate;
                if (frame_size == 0) {
                    drv_debug_msg(VIDEO_DEBUG_ERROR, "Bitrate is too low %d\n",
                            ctx->sRCParams.BitsPerSecond);
                    break;
                }
                ctx->sRCParams.InitialLevel = (3 * ctx->sRCParams.BufferSize) >> 4;
                ctx->sRCParams.InitialLevel += (frame_size / 2);
                ctx->sRCParams.InitialLevel /= frame_size;
                ctx->sRCParams.InitialLevel *= frame_size;
                ctx->sRCParams.InitialDelay =
                    ctx->sRCParams.BufferSize - ctx->sRCParams.InitialLevel;
            }

            if (rate_control_param->initial_qp != 0)
                ctx->sRCParams.InitialQp = rate_control_param->initial_qp;
            if (rate_control_param->basic_unit_size != 0)
                ctx->sRCParams.BUSize = rate_control_param->basic_unit_size;

            drv_debug_msg(VIDEO_DEBUG_GENERAL,
                    "Set Misc parameters(frame %d): window_size %d, initial qp %d\n" \
                    "\tmin qp %d, bunit size %d\n",
                    ctx->raw_frame_count,
                    rate_control_param->window_size,
                    rate_control_param->initial_qp,
                    rate_control_param->min_qp,
                    rate_control_param->basic_unit_size);
            break;

        case VAEncMiscParameterTypeMaxSliceSize:
            max_slice_size_param = (VAEncMiscParameterMaxSliceSize *)pBuffer->data;

            /*The max slice size should not be bigger than 1920x1080x1.5x8 */
            if (max_slice_size_param->max_slice_size > 24883200) {
                drv_debug_msg(VIDEO_DEBUG_ERROR,"Invalid max_slice_size. It should be 1~ 24883200.\n");
                vaStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
                break;
            }

            if (ctx->max_slice_size == max_slice_size_param->max_slice_size)
                break;

            drv_debug_msg(VIDEO_DEBUG_GENERAL, "max slice size changed to %d\n",
                    max_slice_size_param->max_slice_size);

            ctx->max_slice_size = max_slice_size_param->max_slice_size;

            break;

        case VAEncMiscParameterTypeAIR:
            air_param = (VAEncMiscParameterAIR *)pBuffer->data;

            if (air_param->air_num_mbs > 65535 ||
                    air_param->air_threshold > 65535) {
                vaStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
                break;
            }

            drv_debug_msg(VIDEO_DEBUG_GENERAL,"air slice size changed to num_air_mbs %d "
                    "air_threshold %d, air_auto %d\n",
                    air_param->air_num_mbs, air_param->air_threshold,
                    air_param->air_auto);

            if (((ctx->Height * ctx->Width) >> 8) < (int)air_param->air_num_mbs)
                air_param->air_num_mbs = ((ctx->Height * ctx->Width) >> 8);
            if (air_param->air_threshold == 0)
                drv_debug_msg(VIDEO_DEBUG_GENERAL,"%s: air threshold is set to zero\n",
                        __func__);
            ctx->num_air_mbs = air_param->air_num_mbs;
            ctx->air_threshold = air_param->air_threshold;
            //ctx->autotune_air_flag = air_param->air_auto;

            break;

        case VAEncMiscParameterTypeHRD:
            hrd_param = (VAEncMiscParameterHRD *)pBuffer->data;

            if (hrd_param->buffer_size == 0
                    || hrd_param->initial_buffer_fullness == 0)
                drv_debug_msg(VIDEO_DEBUG_GENERAL, "Find zero value for buffer_size "
                        "and initial_buffer_fullness.\n"
                        "Will assign default value to them later \n");

            if (ctx->initial_buffer_fullness > ctx->buffer_size) {
                drv_debug_msg(VIDEO_DEBUG_ERROR, "initial_buffer_fullnessi(%d) shouldn't be"
                        " larger that buffer_size(%d)!\n",
                        hrd_param->initial_buffer_fullness,
                        hrd_param->buffer_size);
                vaStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
                break;
            }

            if (!ctx->sRCParams.RCEnable) {
                drv_debug_msg(VIDEO_DEBUG_ERROR, "Only when rate control is enabled,"
                        " VAEncMiscParameterTypeHRD will take effect.\n");
                break;
            }

            ctx->buffer_size = hrd_param->buffer_size;
            ctx->initial_buffer_fullness = hrd_param->initial_buffer_fullness;
            ctx->bInserHRDParams = IMG_TRUE;
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "hrd param buffer_size set to %d "
                    "initial buffer fullness set to %d\n",
                    ctx->buffer_size, ctx->initial_buffer_fullness);

            break;

        default:
            vaStatus = VA_STATUS_ERROR_UNKNOWN;
            DEBUG_FAILURE;
            break;
    }

    free(obj_buffer->buffer_data);
    obj_buffer->buffer_data = NULL;

    return vaStatus;
}



static VAStatus pnw_H264ES_RenderPicture(
        object_context_p obj_context,
        object_buffer_p *buffers,
        int num_buffers)
{
    INIT_CONTEXT_H264ES;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    int i;

    drv_debug_msg(VIDEO_DEBUG_GENERAL,"pnw_H264ES_RenderPicture\n");

    for (i = 0; i < num_buffers; i++) {
        object_buffer_p obj_buffer = buffers[i];

        switch (obj_buffer->type) {
            case VAEncSequenceParameterBufferType:
                drv_debug_msg(VIDEO_DEBUG_GENERAL, "pnw_H264_RenderPicture got VAEncSequenceParameterBufferType\n");
                vaStatus = pnw__H264ES_process_sequence_param(ctx, obj_buffer);
                DEBUG_FAILURE;
                break;

            case VAEncPictureParameterBufferType:
                drv_debug_msg(VIDEO_DEBUG_GENERAL, "pnw_H264_RenderPicture got VAEncPictureParameterBuffer\n");
                vaStatus = pnw__H264ES_process_picture_param(ctx, obj_buffer);
                DEBUG_FAILURE;
                break;

            case VAEncSliceParameterBufferType:
                drv_debug_msg(VIDEO_DEBUG_GENERAL, "pnw_H264_RenderPicture got VAEncSliceParameterBufferType\n");
                vaStatus = pnw__H264ES_process_slice_param(ctx, obj_buffer);
                DEBUG_FAILURE;
                break;

            case VAEncMiscParameterBufferType:
                drv_debug_msg(VIDEO_DEBUG_GENERAL, "pnw_H264_RenderPicture got VAEncMiscParameterBufferType\n");
                vaStatus = pnw__H264ES_process_misc_param(ctx, obj_buffer);
                DEBUG_FAILURE;
                break;
#if PSB_MFLD_DUMMY_CODE
            case VAEncPackedHeaderParameterBufferType:
                drv_debug_msg(VIDEO_DEBUG_GENERAL, "pnw_H264_RenderPicture got VAEncPackedHeaderParameterBufferType\n");
                vaStatus = pnw__H264ES_insert_SEI_FPA_param(ctx, obj_buffer);
                DEBUG_FAILURE;
                break;
            case VAEncPackedHeaderDataBufferType:
                drv_debug_msg(VIDEO_DEBUG_GENERAL, "pnw_H264_RenderPicture got VAEncPackedHeaderDataBufferType\n");
                vaStatus = pnw__H264ES_insert_SEI_FPA_data(ctx, obj_buffer);
                DEBUG_FAILURE;
                break;
#endif

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

static VAStatus pnw_H264ES_EndPicture(
        object_context_p obj_context)
{
    INIT_CONTEXT_H264ES;
    pnw_cmdbuf_p cmdbuf = ctx->obj_context->pnw_cmdbuf;
    PIC_PARAMS *psPicParams = (PIC_PARAMS *)cmdbuf->pic_params_p;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    unsigned char core = 0;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "pnw_H264ES_EndPicture\n");

    /* Unlike MPEG4 and H263, slices number is defined by user */
    for (core = 0; core < ctx->ParallelCores; core++) {
        psPicParams = (PIC_PARAMS *)
            (cmdbuf->pic_params_p +  ctx->pic_params_size * core);
        psPicParams->NumSlices =  ctx->sRCParams.Slices;
    }

    vaStatus = pnw_EndPicture(ctx);

    return vaStatus;
}


struct format_vtable_s pnw_H264ES_vtable = {
queryConfigAttributes:
    pnw_H264ES_QueryConfigAttributes,
    validateConfig:
        pnw_H264ES_ValidateConfig,
    createContext:
        pnw_H264ES_CreateContext,
    destroyContext:
        pnw_H264ES_DestroyContext,
    beginPicture:
        pnw_H264ES_BeginPicture,
    renderPicture:
        pnw_H264ES_RenderPicture,
    endPicture:
        pnw_H264ES_EndPicture
};

VAStatus pnw_set_frame_skip_flag(
        object_context_p obj_context)
{
    INIT_CONTEXT_H264ES;
    VAStatus vaStatus = VA_STATUS_SUCCESS;


    if (ctx && ctx->previous_src_surface) {
        SET_SURFACE_INFO_skipped_flag(ctx->previous_src_surface->psb_surface, 1);
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Detected a skipped frame for surface 0x%08x.\n",
            ctx->previous_src_surface->psb_surface);
    }

    return vaStatus;
}


