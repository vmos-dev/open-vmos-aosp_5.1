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
 *    Zeng Li <zeng.li@intel.com>
 *    Shengquan Yuan  <shengquan.yuan@intel.com>
 *    Binglin Chen <binglin.chen@intel.com>
 *
 */


#include "psb_def.h"
#include "psb_surface.h"
#include "psb_cmdbuf.h"
#include "lnc_hostcode.h"
#include "lnc_H264ES.h"
#include "lnc_hostheader.h"
#include "va/va_enc_h264.h"
#include "psb_drv_debug.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>


#define TOPAZ_H264_MAX_BITRATE 14000000 /* FIXME ?? */
#define WORST_CASE_SLICE_HEADER_SIZE 200

#define INIT_CONTEXT_H264ES     context_ENC_p ctx = (context_ENC_p) obj_context->format_data
#define SURFACE(id)    ((object_surface_p) object_heap_lookup( &ctx->obj_context->driver_data->surface_heap, id ))
#define BUFFER(id)  ((object_buffer_p) object_heap_lookup( &ctx->obj_context->driver_data->buffer_heap, id ))



static void lnc_H264ES_QueryConfigAttributes(
    VAProfile profile,
    VAEntrypoint entrypoint,
    VAConfigAttrib *attrib_list,
    int num_attribs)
{
    int i;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "lnc_H264ES_QueryConfigAttributes\n");

    /* RateControl attributes */
    for (i = 0; i < num_attribs; i++) {
        switch (attrib_list[i].type) {
        case VAConfigAttribRTFormat:
            break;

        case VAConfigAttribRateControl:
            attrib_list[i].value = VA_RC_NONE | VA_RC_CBR | VA_RC_VBR | VA_RC_VCM;
            break;
#if 0
        case VAConfigAttribEncMaxSliceSize:
            attrib_list[i].value = 0;
            break;
#endif
        default:
            attrib_list[i].value = VA_ATTRIB_NOT_SUPPORTED;
            break;
        }
    }
}


static VAStatus lnc_H264ES_ValidateConfig(
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


static VAStatus lnc_H264ES_CreateContext(
    object_context_p obj_context,
    object_config_p obj_config)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    context_ENC_p ctx;
    int i;
    unsigned int eRCmode = 0;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "lnc_H264ES_CreateContext\n");

    vaStatus = lnc_CreateContext(obj_context, obj_config);

    if (VA_STATUS_SUCCESS != vaStatus)
        return VA_STATUS_ERROR_ALLOCATION_FAILED;

    ctx = (context_ENC_p) obj_context->format_data;

    ctx->max_slice_size = 0;
    ctx->delta_change = 1;
    eRCMode = VA_RC_NONE;
    for (i = 0; i < obj_config->attrib_count; i++) {
#if 0
        if (obj_config->attrib_list[i].type == VAConfigAttribEncMaxSliceSize)
            ctx->max_slice_size = obj_config->attrib_list[i].value;
        else if (obj_config->attrib_list[i].type == VAConfigAttribRateControl)
            eRCmode = obj_config->attrib_list[i].value;
#else
        if (obj_config->attrib_list[i].type == VAConfigAttribRateControl)
            eRCmode = obj_config->attrib_list[i].value;
#endif
    }

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "lnc_H264ES_CreateContext: max slice size %d\n", ctx->max_slice_size);

    if (eRCmode == VA_RC_VBR) {
        ctx->eCodec = IMG_CODEC_H264_VBR;
        ctx->sRCParams.RCEnable = IMG_TRUE;
    } else if (eRCmode == VA_RC_CBR) {
        ctx->eCodec = IMG_CODEC_H264_CBR;
        ctx->sRCParams.RCEnable = IMG_TRUE;
    } else if (eRCmode == VA_RC_VCM) {
        ctx->eCodec = IMG_CODEC_H264_VCM;
        ctx->sRCParams.RCEnable = IMG_TRUE;
    } else if (eRCmode == VA_RC_NONE) {
        ctx->eCodec = IMG_CODEC_H264_NO_RC;
        ctx->sRCParams.RCEnable = IMG_FALSE;
    } else
        return VA_STATUS_ERROR_UNSUPPORTED_RT_FORMAT;
    ctx->eFormat = IMG_CODEC_PL12;      /* use default */

    ctx->IPEControl = lnc__get_ipe_control(ctx->eCodec);
    ctx->idr_pic_id = 1;

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


static void lnc_H264ES_DestroyContext(
    object_context_p obj_context)
{
    struct coded_buf_aux_info *p_aux_info;
    INIT_CONTEXT_H264ES;
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "lnc_H264ES_DestroyPicture\n");

    while (ctx->p_coded_buf_info != NULL) {
        p_aux_info = ctx->p_coded_buf_info->next;
        free(ctx->p_coded_buf_info);
        ctx->p_coded_buf_info = p_aux_info;
    }
    lnc_DestroyContext(obj_context);
}

static VAStatus lnc_H264ES_BeginPicture(
    object_context_p obj_context)
{
    INIT_CONTEXT_H264ES;
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "lnc_H264ES_BeginPicture\n");

    vaStatus = lnc_BeginPicture(ctx);

    return vaStatus;
}

static VAStatus lnc__H264ES_process_sequence_param(context_ENC_p ctx, object_buffer_p obj_buffer)
{
    VAEncSequenceParameterBufferH264 *pSequenceParams;
    lnc_cmdbuf_p cmdbuf = ctx->obj_context->lnc_cmdbuf;
    H264_VUI_PARAMS VUI_Params;
    H264_CROP_PARAMS sCrop;
    char hardcoded_qp[4];

    ASSERT(obj_buffer->type == VAEncSequenceParameterBufferType);
    ASSERT(obj_buffer->num_elements == 1);
    ASSERT(obj_buffer->size == sizeof(VAEncSequenceParameterBufferH264));

    if ((obj_buffer->num_elements != 1) ||
        (obj_buffer->size != sizeof(VAEncSequenceParameterBufferH264))) {
        return VA_STATUS_ERROR_UNKNOWN;
    }

    ctx->obj_context->frame_count = 0;

    pSequenceParams = (VAEncSequenceParameterBufferH264 *) obj_buffer->buffer_data;
    obj_buffer->buffer_data = NULL;
    obj_buffer->size = 0;

    if ((ctx->obj_context->frame_count != 0) &&
        (ctx->sRCParams.BitsPerSecond != pSequenceParams->bits_per_second))
        ctx->update_rc_control = 1;

    /* a new sequence and IDR need reset frame_count */
    ctx->obj_context->frame_count = 0;

    if (pSequenceParams->bits_per_second > TOPAZ_H264_MAX_BITRATE) {
        ctx->sRCParams.BitsPerSecond = TOPAZ_H264_MAX_BITRATE;
        drv_debug_msg(VIDEO_DEBUG_GENERAL, " bits_per_second(%d) exceeds \
		the maximum bitrate, set it with %d\n",
                                 pSequenceParams->bits_per_second,
                                 TOPAZ_H264_MAX_BITRATE);
    } else
        ctx->sRCParams.BitsPerSecond = pSequenceParams->bits_per_second;

    ctx->sRCParams.Slices = 1;

    ctx->sRCParams.IntraFreq = pSequenceParams->intra_period;
    if (ctx->sRCParams.IntraFreq == 0)
        ctx->sRCParams.IntraFreq = 1000;

    ctx->sRCParams.IDRFreq = pSequenceParams->intra_idr_period;
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "IntraFreq: %d\n, intra_idr_period: %d\n",
                             ctx->sRCParams.IntraFreq, ctx->sRCParams.IDRFreq);

    VUI_Params.Time_Scale = ctx->sRCParams.FrameRate * 2;
    VUI_Params.bit_rate_value_minus1 = ctx->sRCParams.BitsPerSecond / 64 - 1;
    VUI_Params.cbp_size_value_minus1 = ctx->sRCParams.BufferSize / 64 - 1;
    VUI_Params.CBR = 1;
    VUI_Params.initial_cpb_removal_delay_length_minus1 = 0;
    VUI_Params.cpb_removal_delay_length_minus1 = 0;
    VUI_Params.dpb_output_delay_length_minus1 = 0;
    VUI_Params.time_offset_length = 0;

    sCrop.bClip = IMG_FALSE;
    sCrop.LeftCropOffset = 0;
    sCrop.RightCropOffset = 0;
    sCrop.TopCropOffset = 0;
    sCrop.BottomCropOffset = 0;

    if (ctx->RawHeight & 0xf) {
        sCrop.bClip = IMG_TRUE;
        sCrop.BottomCropOffset = (((ctx->RawHeight + 0xf) & (~0xf)) - ctx->RawHeight) / 2;
    }
    if (ctx->RawWidth & 0xf) {
        sCrop.bClip = IMG_TRUE;
        sCrop.RightCropOffset = (((ctx->RawWidth + 0xf) & (~0xf)) - ctx->RawWidth) / 2;
    }

    /* sequence header is always inserted */
    if (ctx->eCodec == IMG_CODEC_H264_NO_RC)
        VUI_Params.CBR = 0;

    lnc__H264_prepare_sequence_header((IMG_UINT32 *)(cmdbuf->header_mem_p + ctx->seq_header_ofs), pSequenceParams->max_num_ref_frames,
                                      pSequenceParams->picture_width_in_mbs,
                                      pSequenceParams->picture_height_in_mbs,
                                      pSequenceParams->vui_parameters_present_flag,
                                      pSequenceParams->vui_parameters_present_flag ? (&VUI_Params) : NULL,
                                      &sCrop,
                                      pSequenceParams->level_idc, ctx->profile_idc);

    lnc_cmdbuf_insert_command(cmdbuf, MTX_CMDID_DO_HEADER, 2, 0); /* sequence header */
    RELOC_CMDBUF(cmdbuf->cmd_idx++, ctx->seq_header_ofs, &cmdbuf->header_mem);

    if (0 != ctx->sRCParams.IDRFreq) {
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
    free(pSequenceParams);

    return VA_STATUS_SUCCESS;
}

static VAStatus lnc__H264ES_process_picture_param(context_ENC_p ctx, object_buffer_p obj_buffer)
{
    VAStatus vaStatus;
    VAEncPictureParameterBufferH264 *pBuffer;
    lnc_cmdbuf_p cmdbuf = ctx->obj_context->lnc_cmdbuf;
    struct coded_buf_aux_info *p_aux_info;
    int need_sps = 0;

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

    /* For H264, PicHeader only needed in the first picture*/
    if (!ctx->obj_context->frame_count) {
        cmdbuf = ctx->obj_context->lnc_cmdbuf;

        if (need_sps) {
            /* reuse the previous SPS */
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "TOPAZ: insert a SPS before IDR frame\n");
            memcpy((unsigned char *)(cmdbuf->header_mem_p + ctx->seq_header_ofs),
                   (unsigned char *)ctx->save_seq_header_p,
                   HEADER_SIZE);

            lnc_cmdbuf_insert_command(cmdbuf, MTX_CMDID_DO_HEADER, 2, 0); /* sequence header */
            RELOC_CMDBUF(cmdbuf->cmd_idx++, ctx->seq_header_ofs, &cmdbuf->header_mem);
        }

        lnc__H264_prepare_picture_header((IMG_UINT32 *)(cmdbuf->header_mem_p + ctx->pic_header_ofs));

        lnc_cmdbuf_insert_command(cmdbuf, MTX_CMDID_DO_HEADER, 2, 1);/* picture header */
        RELOC_CMDBUF(cmdbuf->cmd_idx++, ctx->pic_header_ofs, &cmdbuf->header_mem);
    }

    /*Record if EOSEQ or EOSTREAM should be appended to the coded buffer.*/
    if (0 != pBuffer->last_picture) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "It's the last picture in sequence or stream."
                                 " Will append EOSEQ/EOSTREAM to the coded buffer.\n");
        p_aux_info = calloc(1, sizeof(struct coded_buf_aux_info));
        if (NULL == p_aux_info) {
            free(pBuffer);
            return VA_STATUS_ERROR_ALLOCATION_FAILED;
        }
        p_aux_info->buf = ctx->coded_buf;
        p_aux_info->aux_flag = pBuffer->last_picture;
        if (NULL != ctx->p_coded_buf_info)
            p_aux_info->next = ctx->p_coded_buf_info;
        else
            p_aux_info->next = NULL;

        ctx->p_coded_buf_info = p_aux_info;
    }

    vaStatus = lnc_RenderPictureParameter(ctx);

    free(pBuffer);
    return vaStatus;
}

static VAStatus lnc__H264ES_process_slice_param(context_ENC_p ctx, object_buffer_p obj_buffer)
{
    /* Prepare InParams for macros of current slice, insert slice header, insert do slice command */
    VAEncSliceParameterBuffer *pBuffer;
    lnc_cmdbuf_p cmdbuf = ctx->obj_context->lnc_cmdbuf;
    unsigned int MBSkipRun, FirstMBAddress;
    PIC_PARAMS *psPicParams = (PIC_PARAMS *)(cmdbuf->pic_params_p);
    unsigned int i;
    int slice_param_idx;

    ASSERT(obj_buffer->type == VAEncSliceParameterBufferType);

    cmdbuf = ctx->obj_context->lnc_cmdbuf;
    psPicParams = (PIC_PARAMS *)cmdbuf->pic_params_p;

    /* Transfer ownership of VAEncPictureParameterBufferH264 data */
    pBuffer = (VAEncSliceParameterBuffer *) obj_buffer->buffer_data;
    obj_buffer->size = 0;

    /* save current cmdbuf write pointer for H264 frameskip redo
     * for H264, only slice header need to repatch
     */
    cmdbuf->cmd_idx_saved_frameskip = cmdbuf->cmd_idx;

    if (0 == pBuffer->start_row_number) {
        if (pBuffer->slice_flags.bits.is_intra)
            RELOC_PIC_PARAMS(&psPicParams->InParamsBase, ctx->in_params_ofs, cmdbuf->topaz_in_params_I);
        else
            RELOC_PIC_PARAMS(&psPicParams->InParamsBase, ctx->in_params_ofs, cmdbuf->topaz_in_params_P);
    }

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
            free(obj_buffer->buffer_data);
            return VA_STATUS_ERROR_ALLOCATION_FAILED;
        }
    }

    for (i = 0; i < obj_buffer->num_elements; i++) {
        /*Todo list:
         *1.Insert Do header command
         *2.setup InRowParams
         *3.setup Slice params
         *4.Insert Do slice command
         * */
        int deblock_on, force_idr = 0;

        if ((pBuffer->slice_height == 0) || (pBuffer->start_row_number >= ctx->Height)) {
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "slice number is %d, but it seems the last %d buffers are empty\n",
                                     obj_buffer->num_elements, obj_buffer->num_elements - i);
            free(obj_buffer->buffer_data);
            obj_buffer->buffer_data = NULL;
            return VA_STATUS_SUCCESS;
        }

        /* set to INTRA frame */
        if (ctx->force_idr_h264 || (ctx->obj_context->frame_count == 0)) {
            force_idr = 1;
            pBuffer->slice_flags.bits.is_intra = 1;
        }

        if ((pBuffer->slice_flags.bits.disable_deblocking_filter_idc == 0)
            || (pBuffer->slice_flags.bits.disable_deblocking_filter_idc == 2))
            deblock_on = IMG_TRUE;
        else
            deblock_on = IMG_FALSE;

        if (ctx->sRCParams.RCEnable && ctx->sRCParams.FrameSkip)
            MBSkipRun = (ctx->Width * ctx->Height) / 256;
        else
            MBSkipRun = 0;

        FirstMBAddress = (pBuffer->start_row_number * ctx->Width) / 16;
        /* Insert Do Header command, relocation is needed */
        lnc__H264_prepare_slice_header((IMG_UINT32 *)(cmdbuf->header_mem_p + ctx->slice_header_ofs + ctx->obj_context->slice_count * HEADER_SIZE),
                                       pBuffer->slice_flags.bits.is_intra,
                                       pBuffer->slice_flags.bits.disable_deblocking_filter_idc,
                                       ctx->obj_context->frame_count,
                                       FirstMBAddress,
                                       MBSkipRun, force_idr,
                                       pBuffer->slice_flags.bits.uses_long_term_ref,
                                       pBuffer->slice_flags.bits.is_long_term_ref,
                                       ctx->idr_pic_id);

        lnc_cmdbuf_insert_command(cmdbuf, MTX_CMDID_DO_HEADER, 2, (ctx->obj_context->slice_count << 2) | 2);
        RELOC_CMDBUF(cmdbuf->cmd_idx++,
                     ctx->slice_header_ofs + ctx->obj_context->slice_count * HEADER_SIZE,
                     &cmdbuf->header_mem);

        if (!(ctx->sRCParams.RCEnable && ctx->sRCParams.FrameSkip)) {
            if ((ctx->obj_context->frame_count == 0) && (pBuffer->start_row_number == 0) && pBuffer->slice_flags.bits.is_intra)
                lnc_reset_encoder_params(ctx);

            /*The corresponding slice buffer cache*/
            slice_param_idx = (pBuffer->slice_flags.bits.is_intra ? 0 : 1) * ctx->slice_param_num + i;

            if (VAEncSliceParameter_Equal(&ctx->slice_param_cache[slice_param_idx], pBuffer) == 0) {
                drv_debug_msg(VIDEO_DEBUG_GENERAL, "Cache slice%d's parameter buffer at index %d\n", i, slice_param_idx);

                /* cache current param parameters */
                memcpy(&ctx->slice_param_cache[slice_param_idx],
                       pBuffer, sizeof(VAEncSliceParameterBuffer));

                /* Setup InParams value*/
                lnc_setup_slice_params(ctx,
                                       pBuffer->start_row_number * 16,
                                       pBuffer->slice_height * 16,
                                       pBuffer->slice_flags.bits.is_intra,
                                       ctx->obj_context->frame_count > 0,
                                       psPicParams->sInParams.SeInitQP);
            }

            /* Insert do slice command and setup related buffer value */
            lnc__send_encode_slice_params(ctx,
                                          pBuffer->slice_flags.bits.is_intra,
                                          pBuffer->start_row_number * 16,
                                          deblock_on,
                                          ctx->obj_context->frame_count,
                                          pBuffer->slice_height * 16,
                                          ctx->obj_context->slice_count, ctx->max_slice_size);

            drv_debug_msg(VIDEO_DEBUG_GENERAL, "Now frame_count/slice_count is %d/%d\n",
                                     ctx->obj_context->frame_count, ctx->obj_context->slice_count);
        }
        ctx->obj_context->slice_count++;
        pBuffer++; /* Move to the next buffer */

        ASSERT(ctx->obj_context->slice_count < MAX_SLICES_PER_PICTURE);
    }

    free(obj_buffer->buffer_data);
    obj_buffer->buffer_data = NULL;

    return VA_STATUS_SUCCESS;
}

static VAStatus lnc__H264ES_process_misc_param(context_ENC_p ctx, object_buffer_p obj_buffer)
{
    /* Prepare InParams for macros of current slice, insert slice header, insert do slice command */
    VAEncMiscParameterBuffer *pBuffer;
    VAEncMiscParameterRateControl *rate_control_param;
    VAEncMiscParameterAIR *air_param;
    VAEncMiscParameterMaxSliceSize *max_slice_size_param;
    VAEncMiscParameterFrameRate *frame_rate_param;
    unsigned int bit_rate;
    char hardcoded_qp[4];

    VAStatus vaStatus = VA_STATUS_SUCCESS;

    ASSERT(obj_buffer->type == VAEncMiscParameterBufferType);

    /* Transfer ownership of VAEncMiscParameterBuffer data */
    pBuffer = (VAEncMiscParameterBuffer *) obj_buffer->buffer_data;
    obj_buffer->size = 0;

	//initialize the frame_rate and qp
	rate_control_param->initial_qp=26;
	rate_control_param->min_qp=3;
	frame_rate_param->framerate=30;
	
    switch (pBuffer->type) {
    case VAEncMiscParameterTypeFrameRate:
        frame_rate_param = (VAEncMiscParameterFrameRate *)pBuffer->data;

        if (frame_rate_param->framerate > 65535) {
            vaStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
            break;
        }

        drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: frame rate changed to %d\n", __FUNCTION__,
                                 frame_rate_param->framerate);

        if (ctx->sRCParams.FrameRate == frame_rate_param->framerate)
            break;

        ctx->sRCParams.FrameRate = frame_rate_param->framerate;
        ctx->sRCParams.BitsPerSecond += ctx->delta_change;
        ctx->delta_change *= -1;
        ctx->update_rc_control = 1;

        break;

    case VAEncMiscParameterTypeRateControl:
        rate_control_param = (VAEncMiscParameterRateControl *)pBuffer->data;

        if (psb_parse_config("PSB_VIDEO_IQP", hardcoded_qp) == 0)
            rate_control_param->initial_qp = atoi(hardcoded_qp);

        if (psb_parse_config("PSB_VIDEO_MQP", hardcoded_qp) == 0)
            rate_control_param->min_qp = atoi(hardcoded_qp);

        if (rate_control_param->initial_qp > 65535 ||
            rate_control_param->min_qp > 65535 ||
            rate_control_param->target_percentage > 65535) {
            vaStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
            break;
        }

        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Rate control parameter initial_qp=%d, min_qp=%d\n",
                                 rate_control_param->initial_qp,
                                 rate_control_param->min_qp);

        if (rate_control_param->bits_per_second > TOPAZ_H264_MAX_BITRATE) {
            bit_rate = TOPAZ_H264_MAX_BITRATE;
            drv_debug_msg(VIDEO_DEBUG_GENERAL, " bits_per_second(%d) exceeds \
                                             the maximum bitrate, set it with %d\n",
                                     rate_control_param->bits_per_second,
                                     TOPAZ_H264_MAX_BITRATE);
        } else {
            bit_rate = rate_control_param->bits_per_second;
        }

        drv_debug_msg(VIDEO_DEBUG_GENERAL, "rate control changed to %d\n",
                                 rate_control_param->bits_per_second);

        if ((rate_control_param->bits_per_second == ctx->sRCParams.BitsPerSecond) &&
            (ctx->sRCParams.VCMBitrateMargin == rate_control_param->target_percentage * 128 / 100) &&
            (ctx->sRCParams.BufferSize == ctx->sRCParams.BitsPerSecond / 1000 * rate_control_param->window_size) &&
            (ctx->sRCParams.MinQP == rate_control_param->min_qp) &&
            (ctx->sRCParams.InitialQp == rate_control_param->initial_qp))
            break;
        else
            ctx->update_rc_control = 1;

        if (rate_control_param->target_percentage != 0)
            ctx->sRCParams.VCMBitrateMargin = rate_control_param->target_percentage * 128 / 100;
        if (rate_control_param->window_size != 0)
            ctx->sRCParams.BufferSize = ctx->sRCParams.BitsPerSecond * rate_control_param->window_size / 1000;
        if (rate_control_param->initial_qp != 0)
            ctx->sRCParams.InitialQp = rate_control_param->initial_qp;
        if (rate_control_param->min_qp != 0)
            ctx->sRCParams.MinQP = rate_control_param->min_qp;

        if ((bit_rate == ctx->sRCParams.BitsPerSecond) || (bit_rate == 0)) {
            ctx->sRCParams.BitsPerSecond += ctx->delta_change;
            ctx->delta_change *= -1;
        } else {
            ctx->sRCParams.BitsPerSecond = bit_rate;
        }

        break;

    case VAEncMiscParameterTypeMaxSliceSize:
        max_slice_size_param = (VAEncMiscParameterMaxSliceSize *)pBuffer->data;

        if (max_slice_size_param->max_slice_size > INT_MAX ||
            (max_slice_size_param->max_slice_size != 0 &&
             max_slice_size_param->max_slice_size < WORST_CASE_SLICE_HEADER_SIZE)) {
            vaStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
            break;
        }

        if (max_slice_size_param->max_slice_size != 0)
            max_slice_size_param->max_slice_size -= WORST_CASE_SLICE_HEADER_SIZE;

        if (ctx->max_slice_size == max_slice_size_param->max_slice_size)
            break;

        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Max slice size changed to %d\n",
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

        drv_debug_msg(VIDEO_DEBUG_GENERAL, "air slice size changed to num_air_mbs %d "
                                 "air_threshold %d, air_auto %d\n",
                                 air_param->air_num_mbs, air_param->air_threshold,
                                 air_param->air_auto);

        if (((ctx->Height * ctx->Width) >> 8) < air_param->air_num_mbs)
            air_param->air_num_mbs = ((ctx->Height * ctx->Width) >> 8);
        if (air_param->air_threshold == 0)
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: air threshold is set to zero\n",
                                     __FUNCTION__);
        ctx->num_air_mbs = air_param->air_num_mbs;
        ctx->air_threshold = air_param->air_threshold;
        ctx->autotune_air_flag = air_param->air_auto;

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

static VAStatus lnc_H264ES_RenderPicture(
    object_context_p obj_context,
    object_buffer_p *buffers,
    int num_buffers)
{
    INIT_CONTEXT_H264ES;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    int i;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "lnc_H264ES_RenderPicture\n");

    for (i = 0; i < num_buffers; i++) {
        object_buffer_p obj_buffer = buffers[i];

        switch (obj_buffer->type) {
        case VAEncSequenceParameterBufferType:
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "lnc_H264_RenderPicture got VAEncSequenceParameterBufferType\n");
            vaStatus = lnc__H264ES_process_sequence_param(ctx, obj_buffer);
            DEBUG_FAILURE;
            break;

        case VAEncPictureParameterBufferType:
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "lnc_H264_RenderPicture got VAEncPictureParameterBuffer\n");
            vaStatus = lnc__H264ES_process_picture_param(ctx, obj_buffer);
            DEBUG_FAILURE;
            break;

        case VAEncSliceParameterBufferType:
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "lnc_H264_RenderPicture got VAEncSliceParameterBufferType\n");
            vaStatus = lnc__H264ES_process_slice_param(ctx, obj_buffer);
            DEBUG_FAILURE;
            break;

        case VAEncMiscParameterBufferType:
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "lnc_H264_RenderPicture got VAEncMiscParameterBufferType\n");
            vaStatus = lnc__H264ES_process_misc_param(ctx, obj_buffer);
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

static VAStatus lnc_H264ES_EndPicture(
    object_context_p obj_context)
{
    INIT_CONTEXT_H264ES;
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "lnc_H264ES_EndPicture\n");

    vaStatus = lnc_EndPicture(ctx);

    return vaStatus;
}


struct format_vtable_s lnc_H264ES_vtable = {
queryConfigAttributes:
    lnc_H264ES_QueryConfigAttributes,
validateConfig:
    lnc_H264ES_ValidateConfig,
createContext:
    lnc_H264ES_CreateContext,
destroyContext:
    lnc_H264ES_DestroyContext,
beginPicture:
    lnc_H264ES_BeginPicture,
renderPicture:
    lnc_H264ES_RenderPicture,
endPicture:
    lnc_H264ES_EndPicture
};

static inline void lnc_H264_append_EOSEQ(unsigned char *p_buf, unsigned int *p_size)
{
    /*nal_ref_idc should be 0 and nal_ref_idc should be 10 for End of Sequence RBSP*/
    const unsigned char EOSEQ[] = {0x00, 0x00, 0x00, 0x01, 0xa};
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Append %d bytes of End of Sequence RBSP at offset %d\n",
                             sizeof(EOSEQ), *p_size);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Previous 4 bytes: %x %x %x %x\n", *(p_buf - 3), *(p_buf - 2), *(p_buf - 1), *(p_buf));
    p_buf += *p_size;
    memcpy(p_buf, &EOSEQ[0], sizeof(EOSEQ));
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "After 4 bytes: %x %x %x %x\n", *(p_buf + 1), *(p_buf + 2), *(p_buf + 3), *(p_buf + 4));
    *p_size += sizeof(EOSEQ);
}

static inline void lnc_H264_append_EOSTREAM(unsigned char *p_buf, unsigned int *p_size)
{
    /*nal_ref_idc should be 0 and nal_ref_idc should be 11 for End of Stream RBSP*/
    const unsigned char EOSTREAM[] = {0x00, 0x00, 0x00, 0x01, 0xb};
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Append %d bytes of End of Stream RBSP.\n",
                             sizeof(EOSTREAM));
    p_buf += *p_size;
    memcpy(p_buf, EOSTREAM, sizeof(EOSTREAM));
    *p_size += sizeof(EOSTREAM);
}

VAStatus lnc_H264_append_aux_info(object_context_p obj_context,
                                  object_buffer_p obj_buffer,
                                  unsigned char *buf,
                                  unsigned int *p_size)
{
    INIT_CONTEXT_H264ES;
    struct coded_buf_aux_info *p_aux_info;
    struct coded_buf_aux_info *p_prev_aux_info;

    if (NULL == ctx->p_coded_buf_info ||
        (ctx->eCodec != IMG_CODEC_H264_VBR
         && ctx->eCodec != IMG_CODEC_H264_CBR
         && ctx->eCodec != IMG_CODEC_H264_NO_RC))
        return VA_STATUS_SUCCESS;

    ASSERT(obj_buffer);
    ASSERT(buf);
    ASSERT(p_size);

    p_aux_info = ctx->p_coded_buf_info;
    p_prev_aux_info = ctx->p_coded_buf_info;
    while (p_aux_info != NULL) {
        /*Check if we need to append NAL to this coded buffer*/
        if (p_aux_info->buf == obj_buffer)
            break;
        p_prev_aux_info = p_aux_info;
        p_aux_info = p_aux_info->next;
    }

    if (NULL != p_aux_info) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "coded_buf_aux_info 0x%08x\n", p_aux_info->aux_flag);
        if (0 != (p_aux_info->aux_flag & CODED_BUFFER_EOSEQ_FLAG))
            lnc_H264_append_EOSEQ(buf, p_size);

        if (0 != (p_aux_info->aux_flag & CODED_BUFFER_EOSTREAM_FLAG))
            lnc_H264_append_EOSTREAM(buf, p_size);

        if (p_aux_info == ctx->p_coded_buf_info)
            ctx->p_coded_buf_info = p_aux_info->next;
        else
            p_prev_aux_info->next = p_aux_info->next;

        free(p_aux_info);
    }
    return VA_STATUS_SUCCESS;
}
