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
#include "lnc_MPEG4ES.h"
#include "lnc_hostcode.h"
#include "lnc_hostheader.h"
#include "psb_drv_debug.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>


#define TOPAZ_MPEG4_MAX_BITRATE 16000000

#define INIT_CONTEXT_MPEG4ES    context_ENC_p ctx = (context_ENC_p) obj_context->format_data
#define SURFACE(id)    ((object_surface_p) object_heap_lookup( &ctx->obj_context->driver_data->surface_heap, id ))
#define BUFFER(id)  ((object_buffer_p) object_heap_lookup( &ctx->obj_context->driver_data->buffer_heap, id ))



static void lnc_MPEG4ES_QueryConfigAttributes(
    VAProfile profile,
    VAEntrypoint entrypoint,
    VAConfigAttrib *attrib_list,
    int num_attribs)
{
    int i;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "lnc_MPEG4ES_QueryConfigAttributes\n");

    /* RateControl attributes */
    for (i = 0; i < num_attribs; i++) {
        switch (attrib_list[i].type) {
        case VAConfigAttribRTFormat:
            break;

        case VAConfigAttribRateControl:
            attrib_list[i].value = VA_RC_NONE | VA_RC_CBR | VA_RC_VBR;
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

    return;
}


static VAStatus lnc_MPEG4ES_ValidateConfig(
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


static VAStatus lnc_MPEG4ES_CreateContext(
    object_context_p obj_context,
    object_config_p obj_config)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    context_ENC_p ctx;
    int i;
    unsigned int eRCmode;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "lnc_MPEG4ES_CreateContext\n");

    vaStatus = lnc_CreateContext(obj_context, obj_config);
    if (VA_STATUS_SUCCESS != vaStatus)
        return VA_STATUS_ERROR_ALLOCATION_FAILED;

    ctx = (context_ENC_p) obj_context->format_data;
    ctx->FCode = 3;

    ctx->max_slice_size = 0;
    eRCmode = VA_RC_NONE;

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

    if (eRCmode == VA_RC_VBR) {
        ctx->eCodec = IMG_CODEC_MPEG4_VBR;
        ctx->sRCParams.RCEnable = IMG_TRUE;
    } else if (eRCmode == VA_RC_CBR) {
        ctx->eCodec = IMG_CODEC_MPEG4_CBR;
        ctx->sRCParams.RCEnable = IMG_TRUE;
    } else if (eRCmode == VA_RC_NONE) {
        ctx->eCodec = IMG_CODEC_MPEG4_NO_RC;
        ctx->sRCParams.RCEnable = IMG_FALSE;
    } else
        return VA_STATUS_ERROR_UNSUPPORTED_RT_FORMAT;
    ctx->eFormat = IMG_CODEC_PL12;

    ctx->IPEControl = lnc__get_ipe_control(ctx->eCodec);

    switch (obj_config->profile) {
    case VAProfileMPEG4Simple:
        ctx->profile_idc = 2;
        break;
    case VAProfileMPEG4AdvancedSimple:
        ctx->profile_idc = 3;
        break;
    default:
        ctx->profile_idc = 2;
        break;
    }

    return vaStatus;
}


static void lnc_MPEG4ES_DestroyContext(
    object_context_p obj_context)
{
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "lnc_MPEG4ES_DestroyPicture\n");

    lnc_DestroyContext(obj_context);
}

static VAStatus lnc_MPEG4ES_BeginPicture(
    object_context_p obj_context)
{
    INIT_CONTEXT_MPEG4ES;
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "lnc_MPEG4ES_BeginPicture\n");

    vaStatus = lnc_BeginPicture(ctx);

    return vaStatus;
}

static VAStatus lnc__MPEG4ES_process_sequence_param(context_ENC_p ctx, object_buffer_p obj_buffer)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAEncSequenceParameterBufferMPEG4 *seq_params;
    lnc_cmdbuf_p cmdbuf = ctx->obj_context->lnc_cmdbuf;
    MPEG4_PROFILE_TYPE profile;

    ASSERT(obj_buffer->type == VAEncSequenceParameterBufferType);
    ASSERT(obj_buffer->num_elements == 1);
    ASSERT(obj_buffer->size == sizeof(VAEncSequenceParameterBufferMPEG4));

    if ((obj_buffer->num_elements != 1) ||
        (obj_buffer->size != sizeof(VAEncSequenceParameterBufferMPEG4))) {
        return VA_STATUS_ERROR_UNKNOWN;
    }

    seq_params = (VAEncSequenceParameterBufferMPEG4 *) obj_buffer->buffer_data;
    obj_buffer->buffer_data = NULL;
    obj_buffer->size = 0;

    if ((ctx->obj_context->frame_count != 0) &&
        (ctx->sRCParams.BitsPerSecond != seq_params->bits_per_second))
        ctx->update_rc_control = 1;

    if (seq_params->bits_per_second > TOPAZ_MPEG4_MAX_BITRATE) {
        ctx->sRCParams.BitsPerSecond = TOPAZ_MPEG4_MAX_BITRATE;
        drv_debug_msg(VIDEO_DEBUG_GENERAL, " bits_per_second(%d) exceeds \
		the maximum bitrate, set it with %d\n",
                                 seq_params->bits_per_second,
                                 TOPAZ_MPEG4_MAX_BITRATE);
    } else
        ctx->sRCParams.BitsPerSecond = seq_params->bits_per_second;

    if (seq_params->initial_qp < 3)
        seq_params->initial_qp = 3;

    ctx->sRCParams.FrameRate = seq_params->frame_rate;
    ctx->sRCParams.InitialQp = seq_params->initial_qp;
    ctx->sRCParams.MinQP = seq_params->min_qp;
    ctx->sRCParams.BUSize = 0;  /* default 0, and will be set in lnc__setup_busize */

    ctx->sRCParams.Slices = 1;
    ctx->sRCParams.IntraFreq = seq_params->intra_period;

    cmdbuf = ctx->obj_context->lnc_cmdbuf;

    switch (ctx->profile_idc) {
    case 2:
        profile = SP;
        break;
    case 3:
        profile = ASP;
        break;
    default:
        profile = SP;
        break;
    }

    lnc__MPEG4_prepare_sequence_header(
        (IMG_UINT32 *)(cmdbuf->header_mem_p + ctx->seq_header_ofs),
        0, /* BFrame? */
        profile, /* sProfile */
        seq_params->profile_and_level_indication, /* */
        seq_params->fixed_vop_time_increment, /*3,*/  /* sFixed_vop_time_increment */
        seq_params->video_object_layer_width,/* Picture_Width_Pixels */
        seq_params->video_object_layer_height, /* Picture_Height_Pixels */
        0, /*   bVBVPresent  */
        0, /*   First_half_bit_rate   */
        0, /*   Latter_half_bit_rate */
        0, /*   First_half_vbv_buffer_size */
        0, /*   Latter_half_vbv_buffer_size  */
        0, /*   First_half_vbv_occupancy */
        0, /*   Latter_half_vbv_occupancy */
        seq_params->vop_time_increment_resolution); /* VopTimeResolution */

    ctx->MPEG4_vop_time_increment_resolution = seq_params->vop_time_increment_resolution;

    lnc_cmdbuf_insert_command(cmdbuf, MTX_CMDID_DO_HEADER, 2, 0);
    RELOC_CMDBUF(cmdbuf->cmd_idx++, ctx->seq_header_ofs, &cmdbuf->header_mem);

    free(seq_params);
    return vaStatus;
}


static VAStatus lnc__MPEG4ES_process_picture_param(context_ENC_p ctx, object_buffer_p obj_buffer)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAEncPictureParameterBufferMPEG4 *pBuffer;
    lnc_cmdbuf_p cmdbuf = ctx->obj_context->lnc_cmdbuf;
    IMG_BOOL bIsVOPCoded = IMG_TRUE;

    ASSERT(obj_buffer->type == VAEncPictureParameterBufferType);

    if ((obj_buffer->num_elements != 1) ||
        (obj_buffer->size != sizeof(VAEncPictureParameterBufferMPEG4))) {
        return VA_STATUS_ERROR_UNKNOWN;
    }

    /* Transfer ownership of VAEncPictureParameterBufferMPEG4 data */
    pBuffer = (VAEncPictureParameterBufferMPEG4 *) obj_buffer->buffer_data;
    obj_buffer->buffer_data = NULL;
    obj_buffer->size = 0;

    ctx->ref_surface = SURFACE(pBuffer->reference_picture);
    ctx->dest_surface = SURFACE(pBuffer->reconstructed_picture);
    ctx->coded_buf = BUFFER(pBuffer->coded_buf);

    ASSERT(ctx->Width == pBuffer->picture_width);
    ASSERT(ctx->Height == pBuffer->picture_height);

    if (ctx->sRCParams.RCEnable && ctx->sRCParams.FrameSkip)
        bIsVOPCoded = IMG_FALSE;

    /* save current cmdbuf write pointer for MPEG4 frameskip redo
     * MPEG4 picture header need re-patch, and no slice header needed
     * for a skipped frame
     */
    cmdbuf->cmd_idx_saved_frameskip = cmdbuf->cmd_idx;

    lnc__MPEG4_prepare_vop_header((IMG_UINT32 *)(cmdbuf->header_mem_p + ctx->pic_header_ofs),
                                  bIsVOPCoded,
                                  pBuffer->vop_time_increment, /* In testbench, this should be FrameNum */
                                  4,/* default value is 4,search range */
                                  pBuffer->picture_type,
                                  ctx->MPEG4_vop_time_increment_resolution/* defaule value */);

    lnc_cmdbuf_insert_command(cmdbuf, MTX_CMDID_DO_HEADER, 2, 1);
    RELOC_CMDBUF(cmdbuf->cmd_idx++, ctx->pic_header_ofs, &cmdbuf->header_mem);

    vaStatus = lnc_RenderPictureParameter(ctx);

    free(pBuffer);
    return vaStatus;
}

static VAStatus lnc__MPEG4ES_process_slice_param(context_ENC_p ctx, object_buffer_p obj_buffer)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAEncSliceParameterBuffer *pBuffer;
    lnc_cmdbuf_p cmdbuf = ctx->obj_context->lnc_cmdbuf;
    PIC_PARAMS *psPicParams = (PIC_PARAMS *)(cmdbuf->pic_params_p);
    unsigned int i;
    int slice_param_idx;

    ASSERT(obj_buffer->type == VAEncSliceParameterBufferType);

    pBuffer = (VAEncSliceParameterBuffer *) obj_buffer->buffer_data;

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
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Allocate %d VAEncSliceParameterBuffer cache buffers\n", 2 * ctx->slice_param_num);
        ctx->slice_param_num = obj_buffer->num_elements;
        ctx->slice_param_cache = calloc(2 * ctx->slice_param_num, sizeof(VAEncSliceParameterBuffer));
        if (NULL == ctx->slice_param_cache) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "Run out of memory!\n");
            free(obj_buffer->buffer_data);
            return VA_STATUS_ERROR_ALLOCATION_FAILED;
        }
    }

    for (i = 0; i < obj_buffer->num_elements; i++) {
        if (!(ctx->sRCParams.RCEnable && ctx->sRCParams.FrameSkip)) {
            if ((ctx->obj_context->frame_count == 0) && (pBuffer->start_row_number == 0) && pBuffer->slice_flags.bits.is_intra)
                lnc_reset_encoder_params(ctx);

            /*The corresponding slice buffer cache*/
            slice_param_idx = (pBuffer->slice_flags.bits.is_intra ? 0 : 1) * ctx->slice_param_num + i;

            if (VAEncSliceParameter_Equal(&ctx->slice_param_cache[slice_param_idx], pBuffer) == 0) {
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

            lnc__send_encode_slice_params(ctx,
                                          pBuffer->slice_flags.bits.is_intra,
                                          pBuffer->start_row_number * 16,
                                          IMG_FALSE,    /* Deblock is off for MPEG4*/
                                          ctx->obj_context->frame_count,
                                          pBuffer->slice_height * 16,
                                          ctx->obj_context->slice_count,
                                          ctx->max_slice_size);

            drv_debug_msg(VIDEO_DEBUG_GENERAL, "Now frame_count/slice_count is %d/%d\n",
                                     ctx->obj_context->frame_count, ctx->obj_context->slice_count);
        }
        ctx->obj_context->slice_count++;
        pBuffer++;

        ASSERT(ctx->obj_context->slice_count < MAX_SLICES_PER_PICTURE);
    }

    free(obj_buffer->buffer_data);
    obj_buffer->buffer_data = NULL;

    return vaStatus;
}

static VAStatus lnc__MPEG4ES_process_misc_param(context_ENC_p ctx, object_buffer_p obj_buffer)
{
    /* Prepare InParams for macros of current slice, insert slice header, insert do slice command */
    VAEncMiscParameterBuffer *pBuffer;
    VAEncMiscParameterRateControl *rate_control_param;
    VAEncMiscParameterAIR *air_param;
    VAEncMiscParameterMaxSliceSize *max_slice_size_param;
    VAEncMiscParameterFrameRate *frame_rate_param;

    VAStatus vaStatus = VA_STATUS_SUCCESS;

    ASSERT(obj_buffer->type == VAEncMiscParameterBufferType);

    pBuffer = (VAEncMiscParameterBuffer *) obj_buffer->buffer_data;
    obj_buffer->size = 0;

    switch (pBuffer->type) {
    case VAEncMiscParameterTypeFrameRate:
        frame_rate_param = (VAEncMiscParameterFrameRate *)pBuffer->data;
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: frame rate changed to %d\n",
                                 frame_rate_param->framerate);
        break;

    case VAEncMiscParameterTypeRateControl:
        rate_control_param = (VAEncMiscParameterRateControl *)pBuffer->data;

        drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: bit rate changed to %d\n",
                                 rate_control_param->bits_per_second);

        if (rate_control_param->bits_per_second == ctx->sRCParams.BitsPerSecond)
            break;
        else
            ctx->update_rc_control = 1;

        if (rate_control_param->bits_per_second > TOPAZ_MPEG4_MAX_BITRATE) {
            ctx->sRCParams.BitsPerSecond = TOPAZ_MPEG4_MAX_BITRATE;
            drv_debug_msg(VIDEO_DEBUG_GENERAL, " bits_per_second(%d) exceeds \
		the maximum bitrate, set it with %d\n",
                                     rate_control_param->bits_per_second,
                                     TOPAZ_MPEG4_MAX_BITRATE);
        } else
            ctx->sRCParams.BitsPerSecond = rate_control_param->bits_per_second;

        break;

    case VAEncMiscParameterTypeMaxSliceSize:
        max_slice_size_param = (VAEncMiscParameterMaxSliceSize *)pBuffer->data;

        if (ctx->max_slice_size == max_slice_size_param->max_slice_size)
            break;

        drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: max slice size changed to %d\n",
                                 max_slice_size_param->max_slice_size);

        ctx->max_slice_size = max_slice_size_param->max_slice_size;

        break;

    case VAEncMiscParameterTypeAIR:
        air_param = (VAEncMiscParameterAIR *)pBuffer->data;

        drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: air slice size changed to num_air_mbs %d "
                                 "air_threshold %d, air_auto %d\n",
                                 air_param->air_num_mbs, air_param->air_threshold,
                                 air_param->air_auto);

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

    return VA_STATUS_SUCCESS;
}

static VAStatus lnc_MPEG4ES_RenderPicture(
    object_context_p obj_context,
    object_buffer_p *buffers,
    int num_buffers)
{
    INIT_CONTEXT_MPEG4ES;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    int i;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "lnc_MPEG4ES_RenderPicture\n");

    for (i = 0; i < num_buffers; i++) {
        object_buffer_p obj_buffer = buffers[i];

        switch (obj_buffer->type) {
        case VAEncSequenceParameterBufferType:
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "lnc_MPEG4ES_RenderPicture got VAEncSequenceParameterBufferType\n");
            vaStatus = lnc__MPEG4ES_process_sequence_param(ctx, obj_buffer);
            DEBUG_FAILURE;
            break;

        case VAEncPictureParameterBufferType:
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "lnc_MPEG4ES_RenderPicture got VAEncPictureParameterBufferType\n");
            vaStatus = lnc__MPEG4ES_process_picture_param(ctx, obj_buffer);
            DEBUG_FAILURE;
            break;

        case VAEncSliceParameterBufferType:
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "lnc_MPEG4ES_RenderPicture got VAEncSliceParameterBufferType\n");
            vaStatus = lnc__MPEG4ES_process_slice_param(ctx, obj_buffer);
            DEBUG_FAILURE;
            break;

        case VAEncMiscParameterBufferType:
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "lnc_MPEG4ES_RenderPicture got VAEncMiscParameterBufferType\n");
            vaStatus = lnc__MPEG4ES_process_misc_param(ctx, obj_buffer);
            DEBUG_FAILURE;
            break;

        default:
            vaStatus = VA_STATUS_ERROR_UNKNOWN;
            DEBUG_FAILURE;
        }
    }

    return vaStatus;
}

static VAStatus lnc_MPEG4ES_EndPicture(
    object_context_p obj_context)
{
    INIT_CONTEXT_MPEG4ES;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "lnc_MPEG4ES_EndPicture\n");
    return lnc_EndPicture(ctx);
}


struct format_vtable_s lnc_MPEG4ES_vtable = {
queryConfigAttributes:
    lnc_MPEG4ES_QueryConfigAttributes,
validateConfig:
    lnc_MPEG4ES_ValidateConfig,
createContext:
    lnc_MPEG4ES_CreateContext,
destroyContext:
    lnc_MPEG4ES_DestroyContext,
beginPicture:
    lnc_MPEG4ES_BeginPicture,
renderPicture:
    lnc_MPEG4ES_RenderPicture,
endPicture:
    lnc_MPEG4ES_EndPicture
};
