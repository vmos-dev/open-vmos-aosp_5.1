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
#include "hwdefs/dxva_fw_ctrl.h"
#include "hwdefs/reg_io2.h"
#include "hwdefs/msvdx_offsets.h"
#include "hwdefs/msvdx_cmds_io2.h"

#define SURFACE(id)   ((object_surface_p) object_heap_lookup( &dec_ctx->obj_context->driver_data->surface_heap, id ))

static void tng_yuv_processor_QueryConfigAttributes(
    VAProfile __maybe_unused rofile,
    VAEntrypoint __maybe_unused entrypoint,
    VAConfigAttrib __maybe_unused * attrib_list,
    int __maybe_unused num_attribs)
{
    /* No specific attributes */
}

static VAStatus tng_yuv_processor_ValidateConfig(
    object_config_p __maybe_unused obj_config)
{
    return VA_STATUS_SUCCESS;
}

static VAStatus tng_yuv_processor_process_buffer( context_DEC_p, object_buffer_p);

static VAStatus tng_yuv_processor_CreateContext(
    object_context_p obj_context,
    object_config_p __maybe_unused obj_config)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    context_DEC_p dec_ctx = (context_DEC_p) obj_context->format_data;
    context_yuv_processor_p ctx;

    ctx = (context_yuv_processor_p) malloc(sizeof(struct context_yuv_processor_s));
    CHECK_ALLOCATION(ctx);

    /* ctx could be create in/out another dec context */
    ctx->has_dec_ctx = 0;
    ctx->src_surface = NULL;

    if (!dec_ctx) {
        dec_ctx = (context_DEC_p) malloc(sizeof(struct context_DEC_s));
        CHECK_ALLOCATION(dec_ctx);
        obj_context->format_data = (void *)dec_ctx;
        ctx->has_dec_ctx = 1;
        vaStatus = vld_dec_CreateContext(dec_ctx, obj_context);
        DEBUG_FAILURE;
    }

    dec_ctx->yuv_ctx = ctx;
    dec_ctx->process_buffer = tng_yuv_processor_process_buffer;

    return vaStatus;
}

static void tng_yuv_processor_DestroyContext(
    object_context_p obj_context)
{
    context_DEC_p dec_ctx = (context_DEC_p) obj_context->format_data;
    context_yuv_processor_p yuv_ctx = NULL;
    int has_dec_ctx = 0;

    if (dec_ctx == NULL)
        return;

    yuv_ctx = dec_ctx->yuv_ctx;

    if (yuv_ctx) {
        has_dec_ctx = yuv_ctx->has_dec_ctx;
        free(yuv_ctx);
        dec_ctx->yuv_ctx = NULL;
    }

    if (has_dec_ctx) {
        free(dec_ctx);
        obj_context->format_data = NULL;
    }
}

static VAStatus tng_yuv_processor_BeginPicture(
    object_context_p __maybe_unused obj_context)
{
    return VA_STATUS_SUCCESS;
}

static void tng__yuv_processor_process(context_DEC_p dec_ctx)
{
    context_yuv_processor_p ctx = dec_ctx->yuv_ctx;
    psb_cmdbuf_p cmdbuf = dec_ctx->obj_context->cmdbuf;
    /* psb_surface_p target_surface = dec_ctx->obj_context->current_render_target->psb_surface; */
    psb_surface_p src_surface = ctx->src_surface;
    psb_buffer_p buffer;
    uint32_t reg_value;

    psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, DISPLAY_PICTURE_SIZE));

    reg_value = 0;
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, DISPLAY_PICTURE_SIZE, DISPLAY_PICTURE_HEIGHT, (ctx->display_height) - 1);
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, DISPLAY_PICTURE_SIZE, DISPLAY_PICTURE_WIDTH, (ctx->display_width) - 1);
    psb_cmdbuf_rendec_write(cmdbuf, reg_value);

    reg_value = 0;
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, CODED_PICTURE_SIZE, CODED_PICTURE_HEIGHT, (ctx->coded_height) - 1);
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, CODED_PICTURE_SIZE, CODED_PICTURE_WIDTH, (ctx->coded_width) - 1);
    psb_cmdbuf_rendec_write(cmdbuf, reg_value);
    psb_cmdbuf_rendec_end(cmdbuf);


    /*TODO add stride and else there*/
    reg_value = 0;
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, OPERATING_MODE, CODEC_MODE, 3);
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, OPERATING_MODE, ASYNC_MODE, 1);
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, OPERATING_MODE, CODEC_PROFILE, 1);
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, OPERATING_MODE, CHROMA_FORMAT, 1);
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, OPERATING_MODE, ROW_STRIDE, src_surface->stride_mode);

    psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET( MSVDX_CMDS, OPERATING_MODE ));
    psb_cmdbuf_rendec_write(cmdbuf, reg_value);
    psb_cmdbuf_rendec_end(cmdbuf);

    psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, REFERENCE_PICTURE_BASE_ADDRESSES));
    buffer = &src_surface->buf;
    psb_cmdbuf_rendec_write_address(cmdbuf, buffer, buffer->buffer_ofs);
    psb_cmdbuf_rendec_write_address(cmdbuf, buffer,
                                    buffer->buffer_ofs +
                                    src_surface->chroma_offset);
    psb_cmdbuf_rendec_end(cmdbuf);

    reg_value = 0;
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, SLICE_PARAMS, CONSTRAINED_INTRA_PRED, 0 );
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, SLICE_PARAMS, MODE_CONFIG, 0 );
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, SLICE_PARAMS, DISABLE_DEBLOCK_FILTER_IDC, 1 );
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, SLICE_PARAMS, SLICE_ALPHA_CO_OFFSET_DIV2, 0 );
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, SLICE_PARAMS, SLICE_BETA_OFFSET_DIV2, 0 );
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, SLICE_PARAMS, SLICE_FIELD_TYPE, 2 );
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, SLICE_PARAMS, SLICE_CODE_TYPE, 1 ); // P
    *dec_ctx->p_slice_params = reg_value;

    psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET( MSVDX_CMDS, SLICE_PARAMS ) );
    psb_cmdbuf_rendec_write(cmdbuf, reg_value);
    psb_cmdbuf_rendec_end(cmdbuf);

    vld_dec_setup_alternative_frame(dec_ctx->obj_context);

    *cmdbuf->cmd_idx++ = CMD_DEBLOCK | CMD_DEBLOCK_TYPE_SKIP;
    *cmdbuf->cmd_idx++ = 0;
    *cmdbuf->cmd_idx++ = ctx->coded_width / 16;
    *cmdbuf->cmd_idx++ = ctx->coded_height / 16;
    *cmdbuf->cmd_idx++ = 0;
    *cmdbuf->cmd_idx++ = 0;

}

static VAStatus tng__yuv_processor_execute(context_DEC_p dec_ctx, object_buffer_p obj_buffer)
{
    /* psb_surface_p target_surface = dec_ctx->obj_context->current_render_target->psb_surface; */
    context_yuv_processor_p ctx = dec_ctx->yuv_ctx;
    uint32_t reg_value;
    VAStatus vaStatus;

    ASSERT(obj_buffer->type == YUVProcessorSurfaceType ||
           obj_buffer->type == VAProcPipelineParameterBufferType);
    ASSERT(obj_buffer->num_elements == 1);
    ASSERT(obj_buffer->size == sizeof(struct surface_param_s));

    if ((obj_buffer->num_elements != 1) ||
        ((obj_buffer->size != sizeof(struct surface_param_s)) &&
        (obj_buffer->size != sizeof(VAProcPipelineParameterBuffer)))) {
        return VA_STATUS_ERROR_UNKNOWN;
    }

    /* yuv rotation issued from dec driver, TODO removed later */
    if (obj_buffer->type == YUVProcessorSurfaceType) {
        surface_param_p surface_params = (surface_param_p) obj_buffer->buffer_data;
        psb_surface_p rotate_surface = dec_ctx->obj_context->current_render_target->out_loop_surface;
        object_context_p obj_context = dec_ctx->obj_context;
        psb_driver_data_p driver_data = obj_context->driver_data;

        ctx->display_width = (surface_params->display_width + 0xf) & ~0xf;
        ctx->display_height = (surface_params->display_height + 0xf) & ~0xf;
        ctx->coded_width = (surface_params->coded_width + 0xf) & ~0xf;
        ctx->coded_height = (surface_params->coded_height + 0xf) & ~0xf;
        ctx->src_surface = surface_params->src_surface;

        ctx->proc_param = NULL;
        dec_ctx->obj_context->msvdx_rotate = obj_context->msvdx_rotate;
        SET_SURFACE_INFO_rotate(rotate_surface, dec_ctx->obj_context->msvdx_rotate);

        obj_buffer->buffer_data = NULL;
        obj_buffer->size = 0;

#ifdef PSBVIDEO_MSVDX_DEC_TILING
        if (GET_SURFACE_INFO_tiling(ctx->src_surface)) {
            unsigned long msvdx_tile = psb__tile_stride_log2_256(rotate_surface->stride);
            obj_context->msvdx_tile &= 0xf; /* clear rotate tile */
            obj_context->msvdx_tile |= (msvdx_tile << 4);
            obj_context->ctp_type &= (~PSB_CTX_TILING_MASK); /* clear tile context */
            obj_context->ctp_type |= ((obj_context->msvdx_tile & 0xff) << 16);
            psb_update_context(driver_data, obj_context->ctp_type);
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "update tile context, msvdx_tiled is 0x%08x \n", obj_context->msvdx_tile);
        }
#endif
    } else if (obj_buffer->type == VAProcPipelineParameterBufferType) {
        VAProcPipelineParameterBuffer *vpp_params = (VAProcPipelineParameterBuffer *) obj_buffer->buffer_data;
        object_surface_p obj_surface = SURFACE(vpp_params->surface);
        psb_surface_p rotate_surface = dec_ctx->obj_context->current_render_target->psb_surface;

        if (obj_surface == NULL){
            vaStatus = VA_STATUS_ERROR_UNKNOWN;
            return vaStatus;
        }

        //ctx->display_width = ((vpp_params->surface_region->width + 0xf) & ~0xf);
        //ctx->display_height = ((vpp_params->surface_region->height + 0x1f) & ~0x1f);
        ctx->display_width = ((obj_surface->width + 0xf) & ~0xf);
        ctx->display_height = ((obj_surface->height + 0xf) & ~0xf);
        ctx->coded_width = ctx->display_width;
        ctx->coded_height = ctx->display_height;

        ctx->src_surface = obj_surface->psb_surface;
        dec_ctx->obj_context->msvdx_rotate = vpp_params->rotation_state;
        SET_SURFACE_INFO_rotate(rotate_surface, dec_ctx->obj_context->msvdx_rotate);

        ctx->proc_param = vpp_params;
        obj_buffer->buffer_data = NULL;
        obj_buffer->size = 0;

#ifdef PSBVIDEO_MSVDX_DEC_TILING
        object_context_p obj_context = dec_ctx->obj_context;
	psb_driver_data_p driver_data = obj_context->driver_data;
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "attempt to update tile context\n");
        if (GET_SURFACE_INFO_tiling(ctx->src_surface)) {
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "update tile context\n");

            unsigned long msvdx_tile = psb__tile_stride_log2_256(rotate_surface->stride);
            obj_context->msvdx_tile &= 0xf; /* clear rotate tile */
            obj_context->msvdx_tile |= (msvdx_tile << 4);
            obj_context->ctp_type &= (~PSB_CTX_TILING_MASK); /* clear tile context */
            obj_context->ctp_type |= ((obj_context->msvdx_tile & 0xff) << 16);
            psb_update_context(driver_data, obj_context->ctp_type);
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "update tile context, msvdx_tiled is 0x%08x \n", obj_context->msvdx_tile);
        }
#endif
    }

#ifdef ADNROID
        LOGV("%s, %d %d %d %d***************************************************\n",
                  __func__, ctx->display_width, ctx->display_height, ctx->coded_width, ctx->coded_height);
#endif

    vaStatus = VA_STATUS_SUCCESS;

    if (psb_context_get_next_cmdbuf(dec_ctx->obj_context)) {
        vaStatus = VA_STATUS_ERROR_UNKNOWN;
        DEBUG_FAILURE;
        return vaStatus;
    }
    /* ctx->begin_slice(ctx, slice_param); */
    vld_dec_FE_state(dec_ctx->obj_context, NULL);

    tng__yuv_processor_process(dec_ctx);
    /* ctx->process_slice(ctx, slice_param); */
    vld_dec_write_kick(dec_ctx->obj_context);

    dec_ctx->obj_context->video_op = psb_video_vld;
    dec_ctx->obj_context->flags = 0;

    /* ctx->end_slice(ctx); */
    dec_ctx->obj_context->flags = FW_VA_RENDER_IS_FIRST_SLICE | FW_VA_RENDER_IS_LAST_SLICE | FW_INTERNAL_CONTEXT_SWITCH;

    if (psb_context_submit_cmdbuf(dec_ctx->obj_context)) {
        vaStatus = VA_STATUS_ERROR_UNKNOWN;
    }
    return vaStatus;
}

static VAStatus tng_yuv_processor_process_buffer(
    context_DEC_p dec_ctx,
    object_buffer_p buffer)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    object_buffer_p obj_buffer = buffer;
    unsigned int type = obj_buffer->type;
    {
        switch (type) {
        case YUVProcessorSurfaceType:
        case VAProcPipelineParameterBufferType:
            vaStatus = tng__yuv_processor_execute(dec_ctx, obj_buffer);
            DEBUG_FAILURE;
            break;

        default:
            vaStatus = VA_STATUS_ERROR_UNKNOWN;
            DEBUG_FAILURE;
        }
    }

    return vaStatus;
}

static VAStatus tng_yuv_processor_EndPicture(
    object_context_p obj_context)
{
    context_DEC_p dec_ctx = (context_DEC_p) obj_context->format_data;
    context_yuv_processor_p ctx = dec_ctx->yuv_ctx;

    if (psb_context_flush_cmdbuf(obj_context)) {
        return VA_STATUS_ERROR_UNKNOWN;
    }

    if (ctx->proc_param) {
        free(ctx->proc_param);
        ctx->proc_param = NULL;
    }

    return VA_STATUS_SUCCESS;
}

struct format_vtable_s tng_yuv_processor_vtable = {
queryConfigAttributes:
    tng_yuv_processor_QueryConfigAttributes,
validateConfig:
    tng_yuv_processor_ValidateConfig,
createContext:
    tng_yuv_processor_CreateContext,
destroyContext:
    tng_yuv_processor_DestroyContext,
beginPicture:
    tng_yuv_processor_BeginPicture,
renderPicture:
    vld_dec_RenderPicture,
endPicture:
    tng_yuv_processor_EndPicture
};

#define VED_SUPPORTED_FILTERS_NUM 1
#define INIT_DRIVER_DATA    psb_driver_data_p driver_data = (psb_driver_data_p) ctx->pDriverData;
#define CONFIG(id)  ((object_config_p) object_heap_lookup( &driver_data->config_heap, id ))
#define CONTEXT(id) ((object_context_p) object_heap_lookup( &driver_data->context_heap, id ))
#define BUFFER(id)  ((object_buffer_p) object_heap_lookup( &driver_data->buffer_heap, id ))

VAStatus ved_QueryVideoProcFilters(
    VADriverContextP    ctx,
    VAContextID         context,
    VAProcFilterType   *filters,
    unsigned int       *num_filters)
{
    INIT_DRIVER_DATA;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    object_context_p obj_context;
    object_config_p obj_config;
    VAEntrypoint tmp;
    int count;

    /* check if ctx is right */
    obj_context = CONTEXT(context);
    if (NULL == obj_context) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Failed to find context\n");
        vaStatus = VA_STATUS_ERROR_INVALID_CONTEXT;
        goto err;
    }

    obj_config = CONFIG(obj_context->config_id);
    if (NULL == obj_config) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Failed to find config\n");
        vaStatus = VA_STATUS_ERROR_INVALID_CONFIG;
        goto err;
    }

    tmp = obj_config->entrypoint;
    if (tmp != VAEntrypointVideoProc) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "current entrypoint is %d, not VAEntrypointVideoProc\n", tmp);
        vaStatus = VA_STATUS_ERROR_UNKNOWN;
        goto err;
    }

    /* check if filters and num_filters is valid */
    if (NULL == num_filters || NULL == filters) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "invalide input parameter num_filters %p, filters %p\n", num_filters, filters);
        vaStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
        goto err;
    }

    /* check if the filter array size is valid */
    if (*num_filters < VED_SUPPORTED_FILTERS_NUM) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "The filters array size(%d) is NOT valid! Supported filters num is %d\n",
                *num_filters, VED_SUPPORTED_FILTERS_NUM);
        vaStatus = VA_STATUS_ERROR_MAX_NUM_EXCEEDED;
        *num_filters = VED_SUPPORTED_FILTERS_NUM;
        goto err;
    }

    count = 0;
    filters[count++] = VAProcFilterNone;
    *num_filters = count;

err:
    return vaStatus;
}

VAStatus ved_QueryVideoProcFilterCaps(
        VADriverContextP    ctx,
        VAContextID         context,
        VAProcFilterType    type,
        void               *filter_caps,
        unsigned int       *num_filter_caps)
{
    INIT_DRIVER_DATA;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    object_context_p obj_context;
    object_config_p obj_config;
    VAProcFilterCap *no_cap;

    /* check if context is right */
    obj_context = CONTEXT(context);
    if (NULL == obj_context) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Failed to find context\n");
        vaStatus = VA_STATUS_ERROR_INVALID_CONTEXT;
        goto err;
    }

    obj_config = CONFIG(obj_context->config_id);
    if (NULL == obj_config) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Failed to find config\n");
        vaStatus = VA_STATUS_ERROR_INVALID_CONFIG;
        goto err;
    }

    /* check if filter_caps and num_filter_caps is right */
    if (NULL == num_filter_caps || NULL == filter_caps){
        drv_debug_msg(VIDEO_DEBUG_ERROR, "invalide input parameter num_filters %p, filters %p\n", num_filter_caps, filter_caps);
        vaStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
        goto err;
    }

    if (*num_filter_caps < 1) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "invalide input parameter num_filters == %d (> 1)\n", *num_filter_caps);
        vaStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
        goto err;
    }

    /* check if curent HW support and return corresponding caps */
        /* FIXME: we should use a constant table to return caps */
    switch (type) {
    case VAProcFilterNone:
        no_cap = filter_caps;
        no_cap->range.min_value = 0;
        no_cap->range.max_value = 0;
        no_cap->range.default_value = 0;
        no_cap->range.step = 0;
        *num_filter_caps = 1;
        break;
    default:
        drv_debug_msg(VIDEO_DEBUG_ERROR, "invalide filter type %d\n", type);
        vaStatus = VA_STATUS_ERROR_UNSUPPORTED_FILTER;
        *num_filter_caps = 0;
        goto err;
    }

err:
    return vaStatus;
}

VAStatus ved_QueryVideoProcPipelineCaps(
        VADriverContextP    ctx,
        VAContextID         context,
        VABufferID         *filters,
        unsigned int        num_filters,
        VAProcPipelineCaps *pipeline_caps)
{
    INIT_DRIVER_DATA;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    object_context_p obj_context;
    object_config_p obj_config;
    VAProcFilterParameterBufferBase *base;
    object_buffer_p buf;

    /* check if ctx is right */
    obj_context = CONTEXT(context);
    if (NULL == obj_context) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Failed to find context\n");
        vaStatus = VA_STATUS_ERROR_INVALID_CONTEXT;
        goto err;
    }

    obj_config = CONFIG(obj_context->config_id);
    if (NULL == obj_config) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Failed to find config\n");
        vaStatus = VA_STATUS_ERROR_INVALID_CONFIG;
        goto err;
    }

    /* check if filters and num_filters and pipeline-caps are right */
    if (num_filters != 1) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "invalid num_filters %d\n", num_filters);
        vaStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
        goto err;
    }

    if (NULL == filters || pipeline_caps == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "invalid filters %p or pipeline_caps %p\n", filters, pipeline_caps);
        vaStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
        goto err;
    }

    memset(pipeline_caps, 0, sizeof(*pipeline_caps));

    buf = BUFFER(*(filters));

    if (buf == NULL){
        drv_debug_msg(VIDEO_DEBUG_ERROR, "invalid filter buffer: NULL \n");
        vaStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
        goto err;
    }

    base = (VAProcFilterParameterBufferBase *)buf->buffer_data;
    /* check filter buffer setting */
    switch (base->type) {
    case VAProcFilterNone:
        pipeline_caps->rotation_flags = (1 << VA_ROTATION_NONE);
        pipeline_caps->rotation_flags |= (1 << VA_ROTATION_90);
        pipeline_caps->rotation_flags |= (1 << VA_ROTATION_180);
        pipeline_caps->rotation_flags |= (1 << VA_ROTATION_270);
        break;

    default:
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Do NOT support the filter type %d\n", base->type);
        vaStatus = VA_STATUS_ERROR_UNKNOWN;
        goto err;
    }
err:
    return vaStatus;
}
