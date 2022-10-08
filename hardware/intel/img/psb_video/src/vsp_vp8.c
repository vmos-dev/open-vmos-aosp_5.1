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
 *    Zhangfei Zhang <zhangfei.zhang@intel.com>
 *    Mingruo Sun <mingruo.sun@intel.com>
 *
 */
#include "vsp_VPP.h"
#include "vsp_vp8.h"
#include "psb_buffer.h"
#include "psb_surface.h"
#include "vsp_cmdbuf.h"
#include "psb_drv_debug.h"
#include "va/va_enc_vp8.h"

#define INIT_DRIVER_DATA    psb_driver_data_p driver_data = (psb_driver_data_p) ctx->pDriverData;
#define INIT_CONTEXT_VPP    context_VPP_p ctx = (context_VPP_p) obj_context->format_data;
#define CONFIG(id)  ((object_config_p) object_heap_lookup( &driver_data->config_heap, id ))
#define CONTEXT(id) ((object_context_p) object_heap_lookup( &driver_data->context_heap, id ))
#define BUFFER(id)  ((object_buffer_p) object_heap_lookup( &driver_data->buffer_heap, id ))

#define SURFACE(id)    ((object_surface_p) object_heap_lookup( &ctx->obj_context->driver_data->surface_heap, id ))

#define KB 1024
#define MB (KB * KB)
#define VSP_VP8ENC_STATE_SIZE (1*MB)

#define ALIGN_TO_128(value) ((value + 128 - 1) & ~(128 - 1))

#define REF_FRAME_WIDTH  1920
#define REF_FRAME_HEIGHT 1088
#define REF_FRAME_BORDER   32

#define VP8_ENC_CBR 0
#define VP8_ENC_CBR_HRD 1

#define XMEM_FRAME_BUFFER_SIZE_IN_BYTE ((REF_FRAME_WIDTH + 2 * REF_FRAME_BORDER) * (REF_FRAME_HEIGHT + 2 * REF_FRAME_BORDER) + \
        2 * ((REF_FRAME_WIDTH + 2 * REF_FRAME_BORDER) >> 1) * (REF_FRAME_HEIGHT / 2 + REF_FRAME_BORDER)) // Allocated for HD

enum filter_status {
    FILTER_DISABLED = 0,
    FILTER_ENABLED
};

typedef struct _Ref_frame_surface {
    struct VssProcPictureVP8 ref_frame_buffers[4];
} ref_frame_surface;

#define FUNCTION_NAME \
    printf("ENTER %s.\n",__FUNCTION__);

#define EXIT_FUNCTION_NAME \
    printf("EXIT %s.\n",__FUNCTION__);


typedef union {
    struct {
        /* force this frame to be a keyframe */
        unsigned int force_kf                       : 1;
        /* don't reference the last frame */
        unsigned int no_ref_last                    : 1;
        /* don't reference the golden frame */
        unsigned int no_ref_gf                      : 1;
        /* don't reference the alternate reference frame */
        unsigned int no_ref_arf                     : 1;

        unsigned int upd_last                     : 1;
        unsigned int upd_gf                     : 2;
        unsigned int upd_arf                     : 2;
        unsigned int no_upd_last                     : 1;
        unsigned int no_upd_gf                     : 1;
        unsigned int no_upd_arf                     : 1;
        unsigned int upd_entropy                     : 1;
    } bits;
    unsigned int value;
} vp8_fw_pic_flags;


static void vsp_VP8_DestroyContext(object_context_p obj_context);

static VAStatus vsp__VP8_check_legal_picture(object_context_p obj_context, object_config_p obj_config);

static void vsp_VP8_QueryConfigAttributes(
    VAProfile __maybe_unused profile,
    VAEntrypoint __maybe_unused entrypoint,
    VAConfigAttrib *attrib_list,
    int num_attribs)
{
    int i;
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s\n", __FUNCTION__);

    for (i = 0; i < num_attribs; i++) {
        switch (attrib_list[i].type) {
        case VAConfigAttribRTFormat:
            break;
        case VAConfigAttribRateControl:
            attrib_list[i].value = VA_RC_CBR | VA_RC_VCM | VA_RC_VBR;
            break;
        case VAConfigAttribEncAutoReference:
            attrib_list[i].value = 1;
            break;
        case VAConfigAttribEncMaxRefFrames:
            attrib_list[i].value = 4;
            break;
        case VAConfigAttribEncRateControlExt:
            attrib_list[i].value = 3;
            break;

        default:
            attrib_list[i].value = VA_ATTRIB_NOT_SUPPORTED;
            break;
        }
    }
}

static VAStatus vsp_VP8_ValidateConfig(
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
        case VAConfigAttribEncRateControlExt:
            break;

        default:
            return VA_STATUS_ERROR_ATTR_NOT_SUPPORTED;
        }
    }

    return VA_STATUS_SUCCESS;
}

void vsp_VP8_set_default_params(struct VssVp8encSequenceParameterBuffer *vp8_seq)
{

    vp8_seq->frame_width = 1280;
    vp8_seq->frame_height = 720;
    vp8_seq->frame_rate = 30;
    vp8_seq->error_resilient = 0;
    vp8_seq->num_token_partitions = 2;
    vp8_seq->kf_mode = 1;
    vp8_seq->kf_min_dist = 0;
    vp8_seq->kf_max_dist = 30;
    vp8_seq->rc_target_bitrate = 2000;
    vp8_seq->rc_min_quantizer = 4;
    vp8_seq->rc_max_quantizer = 63;
    vp8_seq->rc_undershoot_pct = 100;
    vp8_seq->rc_overshoot_pct = 100;
    vp8_seq->rc_end_usage = VP8_ENC_CBR;
    vp8_seq->rc_buf_sz = 6000;
    vp8_seq->rc_buf_initial_sz = 4000;
    vp8_seq->rc_buf_optimal_sz = 5000;
    vp8_seq->max_intra_rate = 0;
    vp8_seq->cyclic_intra_refresh = 0;
    vp8_seq->concatenate_partitions = 1;
    vp8_seq->recon_buffer_mode = vss_vp8enc_seq_param_recon_buffer_mode_per_seq;
    vp8_seq->ts_number_layers = 1;
    vp8_seq->ts_layer_id[0] = 0;
    vp8_seq->ts_rate_decimator[0] = 0;
    vp8_seq->ts_periodicity = 0;
    vp8_seq->ts_target_bitrate[0] = 0;
}

static VAStatus vsp_VP8_CreateContext(
    object_context_p obj_context,
    object_config_p obj_config)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    /* currently vp8 will use vpp's context since they will use the same cmdbuf */
    context_VPP_p ctx;
    int i;

    ctx = (context_VPP_p) calloc(1, sizeof(struct context_VPP_s));
    if (NULL == ctx) {
        vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
        DEBUG_FAILURE;
        return vaStatus;
    }

    //set default VP8 sequence params
    vsp_VP8_set_default_params(&ctx->vp8_seq_param);
    ctx->temporal_layer_number = 1;

    for (i = 0; i < obj_config->attrib_count; i++) {
        if (obj_config->attrib_list[i].type == VAConfigAttribRateControl) {
            if (obj_config->attrib_list[i].value == VA_RC_VCM)
               ctx->vp8_seq_param.rc_end_usage = VP8_ENC_CBR_HRD;
            else
               ctx->vp8_seq_param.rc_end_usage = VP8_ENC_CBR;
        }
    }

    /* set size */
    ctx->param_sz = 0;
    ctx->pic_param_sz = ALIGN_TO_128(sizeof(struct VssVp8encPictureParameterBuffer));
    ctx->param_sz += ctx->pic_param_sz;
    ctx->seq_param_sz = ALIGN_TO_128(sizeof(struct VssVp8encSequenceParameterBuffer));
    ctx->param_sz += ctx->seq_param_sz;
    ctx->ref_param_sz = ALIGN_TO_128(sizeof(ref_frame_surface));
    ctx->param_sz += ctx->ref_param_sz;

    /* set offset */
    ctx->pic_param_offset = 0;
    ctx->seq_param_offset = ctx->pic_param_sz;
    ctx->ref_param_offset = ctx->pic_param_sz + ctx->seq_param_sz;


    ctx->context_buf = (psb_buffer_p) calloc(1, sizeof(struct psb_buffer_s));
    if (NULL == ctx->context_buf) {
        vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
        DEBUG_FAILURE;
        goto out;
    }

    vaStatus = psb_buffer_create(obj_context->driver_data, VSP_VP8ENC_STATE_SIZE, psb_bt_vpu_only, ctx->context_buf);

    if (VA_STATUS_SUCCESS != vaStatus) {
        goto out;
    }

    obj_context->format_data = (void*) ctx;
    ctx->obj_context = obj_context;

    return vaStatus;

out:
    vsp_VP8_DestroyContext(obj_context);

    if (ctx)
        free(ctx);

    return vaStatus;
}

static void vsp_VP8_DestroyContext(
    object_context_p obj_context)
{
    INIT_CONTEXT_VPP;

    if (ctx->context_buf) {
        psb_buffer_destroy(ctx->context_buf);
        free(ctx->context_buf);
        ctx->context_buf = NULL;
    }

    if (ctx->filters) {
        free(ctx->filters);
        ctx->num_filters = 0;
    }

    free(obj_context->format_data);
    obj_context->format_data = NULL;
}

static VAStatus vsp_vp8_process_seqence_param(
    psb_driver_data_p driver_data,
    context_VPP_p ctx,
    object_buffer_p obj_buffer)
{

    VAStatus vaStatus = VA_STATUS_SUCCESS;
    vsp_cmdbuf_p cmdbuf = ctx->obj_context->vsp_cmdbuf;
    int i;
    int ref_frame_width, ref_frame_height, ref_chroma_height, ref_size;

    VAEncSequenceParameterBufferVP8 *va_seq =
        (VAEncSequenceParameterBufferVP8 *) obj_buffer->buffer_data;
    struct VssVp8encSequenceParameterBuffer *seq = &ctx->vp8_seq_param;
    struct VssVp8encSequenceParameterBuffer *seq_to_firmware =
        (struct VssVp8encSequenceParameterBuffer *)cmdbuf->seq_param_p;

    struct ref_frame_surface *ref =
        (struct ref_frame_surface*)cmdbuf->ref_param_p;

    seq->frame_width       = va_seq->frame_width;
    seq->frame_height      = va_seq->frame_height;
    seq->rc_target_bitrate = va_seq->bits_per_second / 1000;
    seq->max_intra_rate    = 100 * ctx->max_frame_size /
                             (va_seq->bits_per_second / seq->frame_rate);
    /* FIXME: API doc says max 5000, but for current default test vector we still use 6000 */
    seq->kf_mode           = va_seq->kf_auto;   /* AUTO */
    seq->kf_max_dist       = va_seq->kf_max_dist;
    seq->kf_min_dist       = va_seq->kf_min_dist;
    seq->error_resilient   = va_seq->error_resilient;

    ref_frame_width = (seq->frame_width + 2 * 32 + 63) & (~63);
    ref_frame_height = (seq->frame_height + 2 * 32 + 63) & (~63);
    ref_chroma_height = (ref_frame_height / 2 + 63) & (~63);
    ref_size = ref_frame_width * (ref_frame_height + ref_chroma_height);

    for (i = 0; i < 4; i++) {
        seq->ref_frame_buffers[i].surface_id = va_seq->reference_frames[i];
        seq->ref_frame_buffers[i].width = ref_frame_width;
        seq->ref_frame_buffers[i].height = ref_frame_height;
    }

    for (i = 0; i < 4; i++) {
        object_surface_p ref_surf = SURFACE(va_seq->reference_frames[i]);
        if (!ref_surf)
            return VA_STATUS_ERROR_UNKNOWN;

        ref_surf->is_ref_surface = 2;

        if (ref_surf->psb_surface->size < ref_size) {
            /* re-alloc buffer */
            ref_surf->psb_surface->size = ref_size;
            psb_buffer_destroy(&ref_surf->psb_surface->buf);
            vaStatus = psb_buffer_create(driver_data, ref_surf->psb_surface->size, psb_bt_surface, &ref_surf->psb_surface->buf);
            if (VA_STATUS_SUCCESS != vaStatus)
                return VA_STATUS_ERROR_ALLOCATION_FAILED;
        }

        vsp_cmdbuf_reloc_pic_param(&(seq->ref_frame_buffers[i].base),
                                   0,
                                   &(ref_surf->psb_surface->buf),
                                   cmdbuf->param_mem_loc, seq);
    }

    *seq_to_firmware = *seq;

    vsp_cmdbuf_insert_command(cmdbuf, CONTEXT_VP8_ID, &cmdbuf->param_mem,
                              VssVp8encSetSequenceParametersCommand,
                              ctx->seq_param_offset,
                              sizeof(struct VssVp8encSequenceParameterBuffer));
    ctx->vp8_seq_cmd_send = 1;

    return vaStatus;
}

static VAStatus vsp_vp8_process_dynamic_seqence_param(
    context_VPP_p ctx)
{

    VAStatus vaStatus = VA_STATUS_SUCCESS;
    vsp_cmdbuf_p cmdbuf = ctx->obj_context->vsp_cmdbuf;
    int ref_frame_width, ref_frame_height;

    struct VssVp8encSequenceParameterBuffer *seq =
        (struct VssVp8encSequenceParameterBuffer *)cmdbuf->seq_param_p;

    *seq = ctx->vp8_seq_param ;
    seq->max_intra_rate    = 100 * ctx->max_frame_size /
                             (seq->rc_target_bitrate * 1000 / seq->frame_rate);

    if (!ctx->vp8_seq_cmd_send) {
        vsp_cmdbuf_insert_command(cmdbuf, CONTEXT_VP8_ID, &cmdbuf->param_mem,
                                  VssVp8encSetSequenceParametersCommand,
                                  ctx->seq_param_offset,
                                  sizeof(struct VssVp8encSequenceParameterBuffer));
    }

    return vaStatus;
}


static VAStatus vsp_vp8_process_picture_param(
    psb_driver_data_p driver_data,
    context_VPP_p ctx,
    object_buffer_p obj_buffer,
    VASurfaceID surface_id)

{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    vsp_cmdbuf_p cmdbuf = ctx->obj_context->vsp_cmdbuf;

    VAEncPictureParameterBufferVP8 *va_pic =
        (VAEncPictureParameterBufferVP8 *) obj_buffer->buffer_data;
    struct VssVp8encPictureParameterBuffer *pic =
        (struct VssVp8encPictureParameterBuffer *)cmdbuf->pic_param_p;
    struct VssVp8encSequenceParameterBuffer *seq =
        (struct VssVp8encSequenceParameterBuffer *)cmdbuf->seq_param_p;
    int ref_frame_width, ref_frame_height;
    vp8_fw_pic_flags flags;

    ref_frame_width = (ctx->vp8_seq_param.frame_width + 2 * 32 + 63) & (~63);
    ref_frame_height = (ctx->vp8_seq_param.frame_height + 2 * 32 + 63) & (~63);

    //map parameters
    object_buffer_p pObj = BUFFER(va_pic->coded_buf); //tobe modified
    if (!pObj)
        return VA_STATUS_ERROR_UNKNOWN;

    object_surface_p src_surface = SURFACE(surface_id);

    pic->input_frame.surface_id = surface_id;
    pic->input_frame.irq        = 1;
    pic->input_frame.height     = ctx->vp8_seq_param.frame_height;
    pic->input_frame.width      = ctx->vp8_seq_param.frame_width;
    /* NOTE: In VIED API doc, stride must be the nearest integer multiple of 32 */
    /* use vaCreateSurfaceWithAttribute with VAExternalMemoryNULL to create surface*/
    //pic->input_frame.stride     = (ctx->frame_width + 31) & (~31);
    pic->input_frame.stride     = ctx->obj_context->current_render_target->psb_surface->stride;
    pic->input_frame.format     = 0; /* TODO: Specify NV12 = 0 */

    pic->recon_frame.irq = 0;
    pic->recon_frame.width = ref_frame_width;
    pic->recon_frame.height = ref_frame_height;

    pic->version = 0;

    flags.value = 0;
    flags.bits.force_kf = va_pic->ref_flags.bits.force_kf;
    flags.bits.no_ref_last = va_pic->ref_flags.bits.no_ref_last;
    flags.bits.no_ref_gf = va_pic->ref_flags.bits.no_ref_gf;
    flags.bits.no_ref_arf = va_pic->ref_flags.bits.no_ref_arf;
    flags.bits.upd_last  = va_pic->pic_flags.bits.refresh_last;
    flags.bits.upd_gf  = va_pic->pic_flags.bits.copy_buffer_to_golden;
    flags.bits.upd_arf  = va_pic->pic_flags.bits.copy_buffer_to_alternate;
    flags.bits.no_upd_last  = !va_pic->pic_flags.bits.refresh_last;
    flags.bits.no_upd_gf  = !va_pic->pic_flags.bits.refresh_golden_frame;
    flags.bits.no_upd_arf  = !va_pic->pic_flags.bits.refresh_alternate_frame;
    flags.bits.upd_entropy  = va_pic->pic_flags.bits.refresh_entropy_probs;
    if (ctx->temporal_layer_number > 1)
        flags.bits.upd_entropy = 0;
    pic->pic_flags = flags.value;

    pic->prev_frame_dropped = 0; /* Not yet used */
    pic->cpuused            = 5;
    pic->sharpness          = va_pic->sharpness_level;
    pic->num_token_partitions = va_pic->pic_flags.bits.num_token_partitions; /* 2^2 = 4 partitions */
    pic->encoded_frame_size = pObj->size & ~31;
    pic->encoded_frame_base = (uint32_t)pObj->buffer_data;

    {
        vsp_cmdbuf_reloc_pic_param(&(pic->encoded_frame_base),
                                   ctx->pic_param_offset, pObj->psb_buffer,
                                   cmdbuf->param_mem_loc, pic);
    }

    {
        object_surface_p cur_surf = SURFACE(surface_id);
        if (!cur_surf)
            return VA_STATUS_ERROR_UNKNOWN;

        vsp_cmdbuf_reloc_pic_param(&(pic->input_frame.base),
                                   0, &(cur_surf->psb_surface->buf),
                                   cmdbuf->param_mem_loc, pic);
        vsp_cmdbuf_reloc_pic_param(&(pic->input_frame.base_uv),
                                   pic->input_frame.stride * ctx->obj_context->current_render_target->height,
                                   &(cur_surf->psb_surface->buf),
                                   cmdbuf->param_mem_loc, pic);
    }

    *cmdbuf->cmd_idx++ = CONTEXT_VP8_ID;
    *cmdbuf->cmd_idx++ = VssVp8encEncodeFrameCommand;
    VSP_RELOC_CMDBUF(cmdbuf->cmd_idx++, ctx->pic_param_offset, &cmdbuf->param_mem);
    *cmdbuf->cmd_idx++ = sizeof(struct VssVp8encPictureParameterBuffer);
    *cmdbuf->cmd_idx++ = 0;
    *cmdbuf->cmd_idx++ = 0;
    *cmdbuf->cmd_idx++ = wsbmKBufHandle(wsbmKBuf(pObj->psb_buffer->drm_buf)) ;
    *cmdbuf->cmd_idx++ = wsbmKBufHandle(wsbmKBuf((&cmdbuf->param_mem)->drm_buf));

    return vaStatus;
}

static VAStatus vsp_vp8_process_misc_param(context_VPP_p ctx, object_buffer_p obj_buffer)
{
    VAEncMiscParameterBuffer *pBuffer;
    VAEncMiscParameterAIR *air_param;
    VAEncMiscParameterBufferMaxFrameSize *max_frame_size_param;
    VAEncMiscParameterFrameRate *frame_rate_param;
    VAEncMiscParameterRateControl *rate_control_param;
    VAEncMiscParameterHRD *hrd_param;
    VAEncMiscParameterTemporalLayerStructure* tslayer_param;
    struct VssVp8encSequenceParameterBuffer *seq = &ctx->vp8_seq_param;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    uint32_t layer_id, i;

    ASSERT(obj_buffer->type == VAEncMiscParameterBufferType);
    pBuffer = (VAEncMiscParameterBuffer *) obj_buffer->buffer_data;
    obj_buffer->size = 0;

    switch (pBuffer->type) {
    case VAEncMiscParameterTypeTemporalLayerStructure:
        tslayer_param = (VAEncMiscParameterTemporalLayerStructure *)pBuffer->data;
        //verify parameter
        if (tslayer_param->number_of_layers < 2 && tslayer_param->number_of_layers > 3) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "Temporal Layer Number should be 2 or 3\n");
            vaStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
            break;
        }

        if (tslayer_param->periodicity > 32 || tslayer_param->periodicity < 1) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "ts_periodicity shoulde be 1 - 32\n");
            vaStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
            break;
        }

        for (i = 0; i < tslayer_param->periodicity; i++) {
            layer_id = tslayer_param->layer_id[i];
            if (layer_id > (tslayer_param->number_of_layers - 1)) {
                drv_debug_msg(VIDEO_DEBUG_ERROR, "layer_id shoulde be 0 - %d\n",
                              tslayer_param->number_of_layers - 1);
                vaStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
                break;
            }
        }

        if (vaStatus == VA_STATUS_ERROR_INVALID_PARAMETER)
            break;

        seq->ts_number_layers = tslayer_param->number_of_layers;
        ctx->temporal_layer_number = tslayer_param->number_of_layers;
        seq->ts_periodicity = tslayer_param->periodicity;

        for (i = 0; i < seq->ts_periodicity; i++)
            seq->ts_layer_id[i] = tslayer_param->layer_id[i];

        //set default bitrate and framerate
        if (seq->ts_number_layers == 2) {
            seq->ts_target_bitrate[0] = seq->rc_target_bitrate * 6 / 10;
            seq->ts_target_bitrate[1] = seq->rc_target_bitrate ;
            seq->ts_rate_decimator[0] = 2;
            seq->ts_rate_decimator[1] = 1;
        }
        if (seq->ts_number_layers == 3) {
            seq->ts_target_bitrate[0] = seq->rc_target_bitrate * 4 / 10;
            seq->ts_target_bitrate[1] = seq->rc_target_bitrate * 6 / 10;
            seq->ts_target_bitrate[2] = seq->rc_target_bitrate ;
            seq->ts_rate_decimator[0] = 4;
            seq->ts_rate_decimator[1] = 2;
            seq->ts_rate_decimator[2] = 1;
        }

        ctx->re_send_seq_params = 1;
        break;
    case VAEncMiscParameterTypeFrameRate:
        frame_rate_param = (VAEncMiscParameterFrameRate *)pBuffer->data;
        if (frame_rate_param->framerate > 120) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "%d is invalid."
                          "framerate could not be larger than 120\n",
                          frame_rate_param->framerate);
            frame_rate_param->framerate = 120;
        }
        if (frame_rate_param->framerate < 1) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "%d is invalid."
                          "framerate could not be smaller than 1\n",
                          frame_rate_param->framerate);
            frame_rate_param->framerate = 1;
        }

        if (ctx->temporal_layer_number == 1) {
            if (seq->frame_rate != frame_rate_param->framerate) {
                drv_debug_msg(VIDEO_DEBUG_GENERAL, "frame rate changed from %d to %d\n",
                              seq->frame_rate,
                              frame_rate_param->framerate);
                seq->frame_rate = frame_rate_param->framerate;
                ctx->re_send_seq_params = 1;
            }
        } else {
            layer_id = frame_rate_param->framerate_flags.bits.temporal_id % 3;
            if (ctx->frame_rate[layer_id] != frame_rate_param->framerate) {
                drv_debug_msg(VIDEO_DEBUG_GENERAL, "frame rate of layer %d will be changed from %d to %d\n",
                              layer_id, ctx->frame_rate[layer_id], frame_rate_param->framerate);
                ctx->frame_rate[layer_id] = frame_rate_param->framerate;
                if (layer_id == ctx->temporal_layer_number - 1) { //top layer
                    seq->frame_rate = ctx->frame_rate[layer_id];
                }
                ctx->re_send_seq_params = 1 ;
            }
        }
        break;
    case VAEncMiscParameterTypeRateControl:
        rate_control_param = (VAEncMiscParameterRateControl *)pBuffer->data;
        if (rate_control_param->max_qp > 63 ||
            rate_control_param->min_qp > 63) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "max_qp(%d) and min_qp(%d) "
                          "are invalid.\nQP shouldn't be larger than 63 for VP8\n",
                          rate_control_param->max_qp, rate_control_param->min_qp);
            rate_control_param->max_qp = 63;
            rate_control_param->min_qp = rate_control_param->max_qp;
        }

        if (rate_control_param->min_qp != seq->rc_min_quantizer) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "min_qp was changed from %d to %d\n",
                          seq->rc_min_quantizer, rate_control_param->min_qp);
            seq->rc_min_quantizer = rate_control_param->min_qp;
        }

        if (rate_control_param->max_qp != seq->rc_max_quantizer) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "max_qp was changed from %d to %d\n",
                          seq->rc_max_quantizer, rate_control_param->max_qp);
            seq->rc_max_quantizer = rate_control_param->max_qp;
        }

        // no initial qp for vp8

        if (rate_control_param->target_percentage != seq->rc_undershoot_pct) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "rc_undershoot was changed from %d to %d\n",
                          seq->rc_undershoot_pct, rate_control_param->target_percentage);
            seq->rc_undershoot_pct = rate_control_param->target_percentage;
        }

        if (rate_control_param->bits_per_second / 1000 > 20000) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "%d is invalid."
                          "bitrate could not be larger than 20000\n",
                          rate_control_param->bits_per_second / 1000);
            rate_control_param->bits_per_second = 20000000;
        }

        if (ctx->temporal_layer_number == 1) {
            if (rate_control_param->bits_per_second / 1000 != seq->rc_target_bitrate) {
                drv_debug_msg(VIDEO_DEBUG_ERROR, "bitrate was changed from %dkbps to %dkbps\n",
                              seq->rc_target_bitrate, rate_control_param->bits_per_second / 1000);
                seq->rc_target_bitrate = rate_control_param->bits_per_second / 1000;
                seq->ts_target_bitrate[0] = rate_control_param->bits_per_second / 1000;

            }
        } else {
            layer_id = rate_control_param->rc_flags.bits.temporal_id % 3;
            if (rate_control_param->bits_per_second / 1000 != seq->ts_target_bitrate[layer_id]) {
                drv_debug_msg(VIDEO_DEBUG_ERROR, "bitrate was changed from %dkbps to %dkbps\n",
                              seq->ts_target_bitrate[layer_id], rate_control_param->bits_per_second / 1000);
                seq->ts_target_bitrate[layer_id] = rate_control_param->bits_per_second / 1000;
            }
        }

        ctx->re_send_seq_params = 1;
        break;
    case VAEncMiscParameterTypeMaxFrameSize:
        max_frame_size_param = (VAEncMiscParameterBufferMaxFrameSize *)pBuffer->data;
        if (ctx->max_frame_size == max_frame_size_param->max_frame_size)
            break;

        drv_debug_msg(VIDEO_DEBUG_ERROR, "max frame size changed from %d to %d\n",
                      ctx->max_frame_size, max_frame_size_param->max_frame_size);
        ctx->max_frame_size = max_frame_size_param->max_frame_size ;
        ctx->re_send_seq_params = 1 ;
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
        seq->cyclic_intra_refresh = air_param->air_threshold;
        break;
    case VAEncMiscParameterTypeHRD:
        hrd_param = (VAEncMiscParameterHRD *)pBuffer->data;
        seq->rc_buf_sz = hrd_param->buffer_size;
        seq->rc_buf_initial_sz = hrd_param->initial_buffer_fullness;
        seq->rc_buf_optimal_sz = hrd_param->optimal_buffer_fullness;
        ctx->re_send_seq_params = 1;
        break;
    case VAEncMiscParameterTypeQualityLevel:
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
static VAStatus vsp_VP8_RenderPicture(
    object_context_p obj_context,
    object_buffer_p *buffers,
    int num_buffers)
{

    int i;
    psb_driver_data_p driver_data = obj_context->driver_data;
    INIT_CONTEXT_VPP;
    VASurfaceID surface_id;
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    for (i = 0; i < num_buffers; i++) {

        object_buffer_p obj_buffer = buffers[i];
        switch (obj_buffer->type) {
        case VAEncSequenceParameterBufferType:
            vaStatus = vsp_vp8_process_seqence_param(driver_data, ctx, obj_buffer);
            break;
        case VAEncPictureParameterBufferType:
            surface_id = obj_context->current_render_surface_id;
            vaStatus = vsp_vp8_process_picture_param(driver_data, ctx, obj_buffer, surface_id);
            break;
        case VAEncMiscParameterBufferType:
            vaStatus = vsp_vp8_process_misc_param(ctx, obj_buffer);
            break;
        default:
            vaStatus = VA_STATUS_SUCCESS;//VA_STATUS_ERROR_UNKNOWN;
            DEBUG_FAILURE;
        }

        if (vaStatus != VA_STATUS_SUCCESS) {
            break;
        }
    }

    return vaStatus;
}

static VAStatus vsp_VP8_BeginPicture(
    object_context_p obj_context)
{
    int ret;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    INIT_CONTEXT_VPP;
    vsp_cmdbuf_p cmdbuf;

    /* Initialise the command buffer */
    ret = vsp_context_get_next_cmdbuf(ctx->obj_context);
    if (ret) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "get next cmdbuf fail\n");
        vaStatus = VA_STATUS_ERROR_UNKNOWN;
        return vaStatus;
    }

    cmdbuf = obj_context->vsp_cmdbuf;

    if (ctx->obj_context->frame_count == 0) { /* first picture */
        vsp_cmdbuf_insert_command(cmdbuf, CONTEXT_VP8_ID, ctx->context_buf, Vss_Sys_STATE_BUF_COMMAND,
                                  0, VSP_VP8ENC_STATE_SIZE);
    }

    /* map param mem */
    vaStatus = psb_buffer_map(&cmdbuf->param_mem, &cmdbuf->param_mem_p);
    if (vaStatus) {
        return vaStatus;
    }

    cmdbuf->pic_param_p = cmdbuf->param_mem_p;
    cmdbuf->seq_param_p = cmdbuf->param_mem_p + ctx->seq_param_offset;
    cmdbuf->ref_param_p = cmdbuf->param_mem_p + ctx->ref_param_offset;
    ctx->vp8_seq_cmd_send = 0;
    ctx->re_send_seq_params = 0;

    return VA_STATUS_SUCCESS;
}

static inline void  dump_vssporcPicture(struct VssProcPictureVP8 * frame)
{
    drv_debug_msg(VIDEO_ENCODE_DEBUG, "surface_id 0x%08x\n", frame->surface_id);
    drv_debug_msg(VIDEO_ENCODE_DEBUG, "irq %d\n", frame->irq);
    drv_debug_msg(VIDEO_ENCODE_DEBUG, "base 0x%08x\n", frame->base);
    drv_debug_msg(VIDEO_ENCODE_DEBUG, "base_uv 0x%08x\n", frame->base_uv);
    drv_debug_msg(VIDEO_ENCODE_DEBUG, "height %d\n", frame->height);
    drv_debug_msg(VIDEO_ENCODE_DEBUG, "width %d\n", frame->width);
    drv_debug_msg(VIDEO_ENCODE_DEBUG, "stride %d\n", frame->stride);
    drv_debug_msg(VIDEO_ENCODE_DEBUG, "format %d\n", frame->format);
}

static void vsp_vp8_dump_commands(vsp_cmdbuf_p cmdbuf)
{
    unsigned int i, cmd_idx;
    unsigned int cmdbuffer_size = (unsigned char *)cmdbuf->cmd_idx - cmdbuf->cmd_start; /* In bytes */
    unsigned int cmd_number = cmdbuffer_size / sizeof(struct vss_command_t);
    struct vss_command_t *cmd = (struct vss_command_t *)cmdbuf->cmd_start;
    struct VssVp8encPictureParameterBuffer *pic =
        (struct VssVp8encPictureParameterBuffer *)cmdbuf->pic_param_p;
    struct VssVp8encSequenceParameterBuffer *seq =
        (struct VssVp8encSequenceParameterBuffer *)cmdbuf->seq_param_p;

    for (cmd_idx = 0; cmd_idx < cmd_number; cmd_idx++) {
        drv_debug_msg(VIDEO_ENCODE_DEBUG, "\n============command start============\ncontext %d\ntype%d\n",
                      cmd[cmd_idx].context, cmd[cmd_idx].type);
        if (cmd[cmd_idx].type == VssVp8encEncodeFrameCommand) {
            drv_debug_msg(VIDEO_ENCODE_DEBUG, "input frame\n");
            dump_vssporcPicture(&pic->input_frame);

            drv_debug_msg(VIDEO_ENCODE_DEBUG, "recon frame\n");
            dump_vssporcPicture(&pic->recon_frame);

            drv_debug_msg(VIDEO_ENCODE_DEBUG, "version %d\n", pic->version);
            drv_debug_msg(VIDEO_ENCODE_DEBUG, "pic_flags 0x%08x\n", pic->pic_flags);
            drv_debug_msg(VIDEO_ENCODE_DEBUG, "prev_frame_dropped %d\n", pic->prev_frame_dropped);
            drv_debug_msg(VIDEO_ENCODE_DEBUG, "cpuused %d\n", pic->cpuused);
            drv_debug_msg(VIDEO_ENCODE_DEBUG, "sharpness %d\n", pic->sharpness);
            drv_debug_msg(VIDEO_ENCODE_DEBUG, "num_token_partitions %d\n", pic->num_token_partitions);
            drv_debug_msg(VIDEO_ENCODE_DEBUG, "encoded_frame_size %d\n", pic->encoded_frame_size);
            drv_debug_msg(VIDEO_ENCODE_DEBUG, "encoded_frame_base 0x%08x\n", pic->encoded_frame_base);
        }

        if (cmd[cmd_idx].type == VssVp8encSetSequenceParametersCommand) {
            drv_debug_msg(VIDEO_ENCODE_DEBUG, "frame_width %d\n", seq->frame_width);
            drv_debug_msg(VIDEO_ENCODE_DEBUG, "frame_height %d\n", seq->frame_height);
            drv_debug_msg(VIDEO_ENCODE_DEBUG, "frame_rate %d\n", seq->frame_rate);
            drv_debug_msg(VIDEO_ENCODE_DEBUG, "error_resilient %d\n", seq->error_resilient);
            drv_debug_msg(VIDEO_ENCODE_DEBUG, "num_token_partitions %d\n", seq->num_token_partitions);
            drv_debug_msg(VIDEO_ENCODE_DEBUG, "kf_mode %d\n", seq->kf_mode);
            drv_debug_msg(VIDEO_ENCODE_DEBUG, "kf_min_dist %d\n", seq->kf_min_dist);
            drv_debug_msg(VIDEO_ENCODE_DEBUG, "kf_max_dist %d\n", seq->kf_max_dist);
            drv_debug_msg(VIDEO_ENCODE_DEBUG, "rc_target_bitrate %d\n", seq->rc_target_bitrate);
            drv_debug_msg(VIDEO_ENCODE_DEBUG, "rc_min_quantizer %d\n", seq->rc_min_quantizer);
            drv_debug_msg(VIDEO_ENCODE_DEBUG, "rc_max_quantizer %d\n", seq->rc_max_quantizer);
            drv_debug_msg(VIDEO_ENCODE_DEBUG, "rc_undershoot_pct %d\n", seq->rc_undershoot_pct);
            drv_debug_msg(VIDEO_ENCODE_DEBUG, "rc_overshoot_pct %d\n", seq->rc_overshoot_pct);
            drv_debug_msg(VIDEO_ENCODE_DEBUG, "rc_end_usage %d\n", seq->rc_end_usage);
            drv_debug_msg(VIDEO_ENCODE_DEBUG, "rc_buf_sz %d\n", seq->rc_buf_sz);
            drv_debug_msg(VIDEO_ENCODE_DEBUG, "rc_buf_initial_sz %d\n", seq->rc_buf_initial_sz);
            drv_debug_msg(VIDEO_ENCODE_DEBUG, "rc_buf_optimal_sz %d\n", seq->rc_buf_optimal_sz);
            drv_debug_msg(VIDEO_ENCODE_DEBUG, "max_intra_rate %d\n", seq->max_intra_rate);
            drv_debug_msg(VIDEO_ENCODE_DEBUG, "cyclic_intra_refresh %d\n", seq->cyclic_intra_refresh);
            drv_debug_msg(VIDEO_ENCODE_DEBUG, "concatenate_partitions %d\n", seq->concatenate_partitions);
            drv_debug_msg(VIDEO_ENCODE_DEBUG, "recon_buffer_mode %d\n", seq->recon_buffer_mode);
            drv_debug_msg(VIDEO_ENCODE_DEBUG, "ts_number_layers %d\n", seq->ts_number_layers);

            for (i = 0; i < 3; i++)
                drv_debug_msg(VIDEO_ENCODE_DEBUG, "ts_target_bitrate[%d] %d\n", i, seq->ts_target_bitrate[i]);

            for (i = 0; i < 3; i++)
                drv_debug_msg(VIDEO_ENCODE_DEBUG, "ts_rate_decimator[%d] %d\n", i, seq->ts_rate_decimator[i]);
            drv_debug_msg(VIDEO_ENCODE_DEBUG, "ts_periodicity %d\n", seq->ts_periodicity);

            for (i = 0; i < seq->ts_periodicity; i++)
                drv_debug_msg(VIDEO_ENCODE_DEBUG, "ts_layer_id[%d] %d\n", i, seq->ts_layer_id[i]);

            for (i = 0; i < 4; i++) {
                drv_debug_msg(VIDEO_ENCODE_DEBUG, "ref_frame_buffer %d\n", i);
                dump_vssporcPicture(&seq->ref_frame_buffers[i]);
            }
        }

        drv_debug_msg(VIDEO_ENCODE_DEBUG, "============command end============\n");
    }
}


static VAStatus vsp_VP8_EndPicture(
    object_context_p obj_context)
{
    INIT_CONTEXT_VPP;
    psb_driver_data_p driver_data = obj_context->driver_data;
    vsp_cmdbuf_p cmdbuf = obj_context->vsp_cmdbuf;

    if (ctx->re_send_seq_params) {
        vsp_vp8_process_dynamic_seqence_param(ctx);
    }

    drv_debug_msg(VIDEO_ENCODE_DEBUG, "ctx->obj_context->frame_count=%d\n", ctx->obj_context->frame_count + 1);
    vsp_vp8_dump_commands(cmdbuf);

    if (cmdbuf->param_mem_p != NULL) {
        psb_buffer_unmap(&cmdbuf->param_mem);
        cmdbuf->param_mem_p = NULL;
        cmdbuf->pic_param_p = NULL;
        cmdbuf->end_param_p = NULL;
        cmdbuf->pipeline_param_p = NULL;
        cmdbuf->denoise_param_p = NULL;
        cmdbuf->enhancer_param_p = NULL;
        cmdbuf->sharpen_param_p = NULL;
        cmdbuf->frc_param_p = NULL;
        cmdbuf->ref_param_p = NULL;
    }

    if (vsp_context_flush_cmdbuf(ctx->obj_context)) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "psb_VP8: flush deblock cmdbuf error\n");
        return VA_STATUS_ERROR_UNKNOWN;
    }

    return VA_STATUS_SUCCESS;
}

struct format_vtable_s vsp_VP8_vtable = {
queryConfigAttributes:
    vsp_VP8_QueryConfigAttributes,
validateConfig:
    vsp_VP8_ValidateConfig,
createContext:
    vsp_VP8_CreateContext,
destroyContext:
    vsp_VP8_DestroyContext,
beginPicture:
    vsp_VP8_BeginPicture,
renderPicture:
    vsp_VP8_RenderPicture,
endPicture:
    vsp_VP8_EndPicture
};
