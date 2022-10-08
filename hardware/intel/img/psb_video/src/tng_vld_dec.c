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
#include "va/va_dec_jpeg.h"
#include "va/va_dec_vp8.h"

#define GET_SURFACE_INFO_colocated_index(psb_surface) ((int) (psb_surface->extra_info[3]))
#define SET_SURFACE_INFO_colocated_index(psb_surface, val) psb_surface->extra_info[3] = (uint32_t) val;

/* Set MSVDX Front end register */
void vld_dec_FE_state(object_context_p obj_context, psb_buffer_p buf)
{
    psb_cmdbuf_p cmdbuf = obj_context->cmdbuf;
    context_DEC_p ctx = (context_DEC_p) obj_context->format_data;
    CTRL_ALLOC_HEADER *cmd_header = (CTRL_ALLOC_HEADER *)psb_cmdbuf_alloc_space(cmdbuf, sizeof(CTRL_ALLOC_HEADER));

    cmd_header->ui32Cmd_AdditionalParams = CMD_CTRL_ALLOC_HEADER;
    cmd_header->ui32ExternStateBuffAddr = 0;
    if (buf)
        RELOC(cmd_header->ui32ExternStateBuffAddr, 0, buf);
    cmd_header->ui32MacroblockParamAddr = 0; /* Only EC needs to set this */

    ctx->cmd_params = &cmd_header->ui32Cmd_AdditionalParams;
    ctx->p_slice_params = &cmd_header->ui32SliceParams;
    cmd_header->ui32SliceParams = 0;

    ctx->slice_first_pic_last = &cmd_header->uiSliceFirstMbYX_uiPicLastMbYX;
    *ctx->slice_first_pic_last = 0;

    ctx->p_range_mapping_base0 = &cmd_header->ui32AltOutputAddr[0];
    ctx->p_range_mapping_base1 = &cmd_header->ui32AltOutputAddr[1];

    ctx->alt_output_flags = &cmd_header->ui32AltOutputFlags;

    cmd_header->ui32AltOutputFlags = 0;
    cmd_header->ui32AltOutputAddr[0] = 0;
    cmd_header->ui32AltOutputAddr[1] = 0;
}

/* Programme the Alt output if there is a rotation*/
void vld_dec_setup_alternative_frame(object_context_p obj_context)
{
    uint32_t cmd = 0;
    psb_cmdbuf_p cmdbuf = obj_context->cmdbuf;
    context_DEC_p ctx = (context_DEC_p) obj_context->format_data;
    psb_surface_p src_surface = obj_context->current_render_target->psb_surface;
    psb_surface_p out_loop_surface = obj_context->current_render_target->out_loop_surface;
    int ved_scaling = (CONTEXT_SCALING(obj_context) && !ctx->yuv_ctx);
    uint32_t startX = 0, startY = 0, luma_addr_offset = 0, chroma_addr_offset = 0;

    /*  In VPP ctx, current_render_target is rotated surface */
    if (ctx->yuv_ctx && (VAEntrypointVideoProc == obj_context->entry_point)) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Setup second-pass rotation\n");
        out_loop_surface = src_surface;
        src_surface = ctx->yuv_ctx->src_surface;
    }

    if (CONTEXT_ALTERNATIVE_OUTPUT(obj_context) || obj_context->entry_point == VAEntrypointVideoProc) {
        if (ved_scaling) {
            out_loop_surface = obj_context->current_render_target->scaling_surface;
#ifndef BAYTRAIL
            tng_ved_write_scale_reg(obj_context);

            REGIO_WRITE_FIELD_LITE(cmd, MSVDX_CMDS,ALTERNATIVE_OUTPUT_PICTURE_ROTATION, SCALE_INPUT_SIZE_SEL, 1);
            REGIO_WRITE_FIELD_LITE(cmd, MSVDX_CMDS,ALTERNATIVE_OUTPUT_PICTURE_ROTATION, SCALE_ENABLE, 1);
#endif
        } else {
            startX = ((uint32_t)obj_context->current_render_target->offset_x_s + 0x3f) & ~0x3f;
            startY = ((uint32_t)obj_context->current_render_target->offset_y_s + 0x1) & ~0x1;
            luma_addr_offset = (((uint32_t)(startX + out_loop_surface->stride * startY))  + 0x3f ) & ~0x3f;
            chroma_addr_offset = (((uint32_t)(startX + out_loop_surface->stride * startY / 2))  + 0x3f ) & ~0x3f;
        }

        if (out_loop_surface == NULL) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "out-loop surface is NULL, abort msvdx alternative output\n");
            return;
        }

        if (GET_SURFACE_INFO_rotate(out_loop_surface) != obj_context->msvdx_rotate && !ved_scaling)
            drv_debug_msg(VIDEO_DEBUG_WARNING, "Display rotate mode does not match surface rotate mode!\n");

        /* CRendecBlock    RendecBlk( mCtrlAlloc , RENDEC_REGISTER_OFFSET(MSVDX_CMDS, VC1_LUMA_RANGE_MAPPING_BASE_ADDRESS) ); */
        psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, VC1_LUMA_RANGE_MAPPING_BASE_ADDRESS));

        psb_cmdbuf_rendec_write_address(cmdbuf, &out_loop_surface->buf, out_loop_surface->buf.buffer_ofs + luma_addr_offset);
        psb_cmdbuf_rendec_write_address(cmdbuf, &out_loop_surface->buf, out_loop_surface->buf.buffer_ofs + chroma_addr_offset + out_loop_surface->chroma_offset);

        psb_cmdbuf_rendec_end(cmdbuf);

        REGIO_WRITE_FIELD_LITE(cmd, MSVDX_CMDS, ALTERNATIVE_OUTPUT_PICTURE_ROTATION , ALT_PICTURE_ENABLE, 1);
        REGIO_WRITE_FIELD_LITE(cmd, MSVDX_CMDS, ALTERNATIVE_OUTPUT_PICTURE_ROTATION , ROTATION_ROW_STRIDE, out_loop_surface->stride_mode);
        REGIO_WRITE_FIELD_LITE(cmd, MSVDX_CMDS, ALTERNATIVE_OUTPUT_PICTURE_ROTATION , RECON_WRITE_DISABLE, 0); /* FIXME Always generate Rec */
        REGIO_WRITE_FIELD_LITE(cmd, MSVDX_CMDS, ALTERNATIVE_OUTPUT_PICTURE_ROTATION , ROTATION_MODE, GET_SURFACE_INFO_rotate(out_loop_surface));

        RELOC(*ctx->p_range_mapping_base0, out_loop_surface->buf.buffer_ofs + luma_addr_offset, &out_loop_surface->buf);
        RELOC(*ctx->p_range_mapping_base1, out_loop_surface->buf.buffer_ofs + chroma_addr_offset + out_loop_surface->chroma_offset, &out_loop_surface->buf);
    }

    if (obj_context->profile == VAProfileVP8Version0_3 ||
        obj_context->profile == VAProfileJPEGBaseline || ctx->yuv_ctx) {
        psb_cmdbuf_rendec_start(cmdbuf, (REG_MSVDX_CMD_OFFSET + MSVDX_CMDS_AUX_LINE_BUFFER_BASE_ADDRESS_OFFSET));
        psb_cmdbuf_rendec_write_address(cmdbuf, &ctx->aux_line_buffer_vld, ctx->aux_line_buffer_vld.buffer_ofs);
        psb_cmdbuf_rendec_end(cmdbuf);

        REGIO_WRITE_FIELD_LITE(cmd, MSVDX_CMDS, ALTERNATIVE_OUTPUT_PICTURE_ROTATION, USE_AUX_LINE_BUF, 1);
        if (ctx->yuv_ctx)
            REGIO_WRITE_FIELD_LITE(cmd, MSVDX_CMDS, ALTERNATIVE_OUTPUT_PICTURE_ROTATION , RECON_WRITE_DISABLE, 1);
    }

    /* Set the rotation registers */
    psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, ALTERNATIVE_OUTPUT_PICTURE_ROTATION));
    psb_cmdbuf_rendec_write(cmdbuf, cmd);
    *ctx->alt_output_flags = cmd;

    cmd = 0;
    REGIO_WRITE_FIELD_LITE(cmd, MSVDX_CMDS, EXTENDED_ROW_STRIDE, EXT_ROW_STRIDE, src_surface->stride / 64);
    psb_cmdbuf_rendec_write(cmdbuf, cmd);

    psb_cmdbuf_rendec_end(cmdbuf);
}

int vld_dec_slice_parameter_size(object_context_p obj_context)
{
    int size;

    switch (obj_context->profile) {
    case VAProfileMPEG2Simple:
    case VAProfileMPEG2Main:
        size = sizeof(VASliceParameterBufferMPEG2);
        break;
    case VAProfileMPEG4Simple:
    case VAProfileMPEG4AdvancedSimple:
    case VAProfileMPEG4Main:
    case VAProfileH263Baseline:
        size = sizeof(VASliceParameterBufferMPEG4);
        break;
    case VAProfileH264Baseline:
    case VAProfileH264Main:
    case VAProfileH264High:
    case VAProfileH264ConstrainedBaseline:
        size = sizeof(VASliceParameterBufferH264);
        break;
    case VAProfileVC1Simple:
    case VAProfileVC1Main:
    case VAProfileVC1Advanced:
        size = sizeof(VASliceParameterBufferVC1);
        break;
    case VAProfileVP8Version0_3:
        size = sizeof(VASliceParameterBufferVP8);
    case VAProfileJPEGBaseline:
        size = sizeof(VASliceParameterBufferJPEGBaseline);
    default:
        size = 0;
        break;
    }

    return size;
}

VAStatus vld_dec_process_slice_data(context_DEC_p ctx, object_buffer_p obj_buffer)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    void *slice_param;
    int buffer_idx = 0;
    unsigned int element_idx = 0, element_size;

    ASSERT((obj_buffer->type == VASliceDataBufferType) || (obj_buffer->type == VAProtectedSliceDataBufferType));

    ASSERT(ctx->pic_params);
    ASSERT(ctx->slice_param_list_idx);

#if 0
    if (!ctx->pic_params) {
        /* Picture params missing */
        return VA_STATUS_ERROR_UNKNOWN;
    }
#endif
    if ((NULL == obj_buffer->psb_buffer) ||
        (0 == obj_buffer->size)) {
        /* We need to have data in the bitstream buffer */
        return VA_STATUS_ERROR_UNKNOWN;
    }

    element_size = vld_dec_slice_parameter_size(ctx->obj_context);

    while (buffer_idx < ctx->slice_param_list_idx) {
        object_buffer_p slice_buf = ctx->slice_param_list[buffer_idx];
        if (element_idx >= slice_buf->num_elements) {
            /* Move to next buffer */
            element_idx = 0;
            buffer_idx++;
            continue;
        }

        slice_param = slice_buf->buffer_data;
        slice_param = (void *)((unsigned long)slice_param + element_idx * element_size);
        element_idx++;
        vaStatus = vld_dec_process_slice(ctx, slice_param, obj_buffer);
        if (vaStatus != VA_STATUS_SUCCESS) {
            DEBUG_FAILURE;
            break;
        }
    }
    ctx->slice_param_list_idx = 0;

    return vaStatus;
}
/*
 * Adds a VASliceParameterBuffer to the list of slice params
 */
VAStatus vld_dec_add_slice_param(context_DEC_p ctx, object_buffer_p obj_buffer)
{
    ASSERT(obj_buffer->type == VASliceParameterBufferType);
    if (ctx->slice_param_list_idx >= ctx->slice_param_list_size) {
        unsigned char *new_list;
        ctx->slice_param_list_size += 8;
        new_list = realloc(ctx->slice_param_list,
                           sizeof(object_buffer_p) * ctx->slice_param_list_size);
        if (NULL == new_list) {
            return VA_STATUS_ERROR_ALLOCATION_FAILED;
        }
        ctx->slice_param_list = (object_buffer_p*) new_list;
    }
    ctx->slice_param_list[ctx->slice_param_list_idx] = obj_buffer;
    ctx->slice_param_list_idx++;
    return VA_STATUS_SUCCESS;
}

void vld_dec_write_kick(object_context_p obj_context)
{
    psb_cmdbuf_p cmdbuf = obj_context->cmdbuf;
    *cmdbuf->cmd_idx++ = CMD_COMPLETION;
}

VAStatus vld_dec_process_slice(context_DEC_p ctx,
                                        void *vld_slice_param,
                                        object_buffer_p obj_buffer)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VASliceParameterBufferBase *slice_param = (VASliceParameterBufferBase *) vld_slice_param;

    ASSERT((obj_buffer->type == VASliceDataBufferType) || (obj_buffer->type == VAProtectedSliceDataBufferType));

    if ((slice_param->slice_data_flag == VA_SLICE_DATA_FLAG_BEGIN) ||
        (slice_param->slice_data_flag == VA_SLICE_DATA_FLAG_ALL)) {
#ifndef SLICE_HEADER_PARSING
        if (0 == slice_param->slice_data_size) {
            vaStatus = VA_STATUS_ERROR_UNKNOWN;
            DEBUG_FAILURE;
            return vaStatus;
        }
#endif
        ASSERT(!ctx->split_buffer_pending);

        if (psb_context_get_next_cmdbuf(ctx->obj_context)) {
            vaStatus = VA_STATUS_ERROR_UNKNOWN;
            DEBUG_FAILURE;
            return vaStatus;
        }
        vld_dec_FE_state(ctx->obj_context, ctx->preload_buffer);
        ctx->begin_slice(ctx, slice_param);
        ctx->slice_data_buffer = obj_buffer->psb_buffer;
#ifdef SLICE_HEADER_PARSING
        if (ctx->parse_enabled == 1)
            psb_cmdbuf_dma_write_key(ctx->obj_context->cmdbuf,
                                         ctx->SR_flags,
                                         ctx->parse_key);
        else
#endif
            psb_cmdbuf_dma_write_bitstream(ctx->obj_context->cmdbuf,
                                         obj_buffer->psb_buffer,
                                         obj_buffer->psb_buffer->buffer_ofs + slice_param->slice_data_offset,
                                         slice_param->slice_data_size,
                                         ctx->bits_offset,
                                         ctx->SR_flags);

        if (slice_param->slice_data_flag == VA_SLICE_DATA_FLAG_BEGIN) {
            ctx->split_buffer_pending = TRUE;
        }
    } else {
        ASSERT(ctx->split_buffer_pending);
        ASSERT(0 == slice_param->slice_data_offset);
        if (slice_param->slice_data_size) {
            psb_cmdbuf_dma_write_bitstream_chained(ctx->obj_context->cmdbuf,
                    obj_buffer->psb_buffer,
                    slice_param->slice_data_size);
        }
    }

    if ((slice_param->slice_data_flag == VA_SLICE_DATA_FLAG_ALL) ||
        (slice_param->slice_data_flag == VA_SLICE_DATA_FLAG_END)) {
        if (slice_param->slice_data_flag == VA_SLICE_DATA_FLAG_END) {
            ASSERT(ctx->split_buffer_pending);
        }

        ctx->process_slice(ctx, slice_param);
        vld_dec_write_kick(ctx->obj_context);

        ctx->split_buffer_pending = FALSE;
        ctx->obj_context->video_op = psb_video_vld;
        ctx->obj_context->flags = 0;

        ctx->end_slice(ctx);

        if (psb_context_submit_cmdbuf(ctx->obj_context)) {
            vaStatus = VA_STATUS_ERROR_UNKNOWN;
        }
    }
    return vaStatus;
}

VAStatus vld_dec_allocate_colocated_buffer(context_DEC_p ctx, object_surface_p obj_surface, uint32_t size)
{
    psb_buffer_p buf;
    VAStatus vaStatus;
    psb_surface_p surface = obj_surface->psb_surface;
    int index = GET_SURFACE_INFO_colocated_index(surface);

    if (!index) {
        index = ctx->colocated_buffers_idx;
        if (index >= ctx->colocated_buffers_size) {
            return VA_STATUS_ERROR_UNKNOWN;
        }

        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Allocating colocated buffer for surface %08x size = %08x\n", surface, size);

        buf = &(ctx->colocated_buffers[index]);
        vaStatus = psb_buffer_create(ctx->obj_context->driver_data, size, psb_bt_vpu_only, buf);
        if (VA_STATUS_SUCCESS != vaStatus) {
            return vaStatus;
        }
        ctx->colocated_buffers_idx++;
        SET_SURFACE_INFO_colocated_index(surface, index + 1); /* 0 means unset, index is offset by 1 */
    } else {
        buf = &(ctx->colocated_buffers[index - 1]);
        if (buf->size < size) {
            psb_buffer_destroy(buf);
            vaStatus = psb_buffer_create(ctx->obj_context->driver_data, size, psb_bt_vpu_only, buf);
            if (VA_STATUS_SUCCESS != vaStatus) {
                return vaStatus;
            }
            SET_SURFACE_INFO_colocated_index(surface, index); /* replace the original buffer */
        }
    }
    return VA_STATUS_SUCCESS;
}

psb_buffer_p vld_dec_lookup_colocated_buffer(context_DEC_p ctx, psb_surface_p surface)
{
    int index = GET_SURFACE_INFO_colocated_index(surface);
    if (!index) {
        return NULL;
    }
    return &(ctx->colocated_buffers[index-1]); /* 0 means unset, index is offset by 1 */
}

VAStatus vld_dec_CreateContext(context_DEC_p ctx, object_context_p obj_context)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    ctx->obj_context = obj_context;
    ctx->split_buffer_pending = FALSE;
    ctx->slice_param_list_size = 8;
    ctx->slice_param_list = (object_buffer_p*) calloc(1, sizeof(object_buffer_p) * ctx->slice_param_list_size);
    ctx->slice_param_list_idx = 0;

    if (NULL == ctx->slice_param_list) {
        vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
        DEBUG_FAILURE;
        return vaStatus;
    }

    ctx->colocated_buffers_size = obj_context->num_render_targets;
    ctx->colocated_buffers_idx = 0;
    ctx->colocated_buffers = (psb_buffer_p) calloc(1, sizeof(struct psb_buffer_s) * ctx->colocated_buffers_size);
    if (NULL == ctx->colocated_buffers) {
        vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
        DEBUG_FAILURE;
        free(ctx->slice_param_list);
    }

    if (vaStatus == VA_STATUS_SUCCESS) {
        vaStatus = psb_buffer_create(obj_context->driver_data,
                                     AUX_LINE_BUFFER_VLD_SIZE,
                                     psb_bt_cpu_vpu,
                                     &ctx->aux_line_buffer_vld);
        DEBUG_FAILURE;
    }

    return vaStatus;
}

void vld_dec_DestroyContext(context_DEC_p ctx)
{
    int i;
    ctx->preload_buffer = NULL;

    psb_buffer_destroy(&ctx->aux_line_buffer_vld);

    if (ctx->slice_param_list) {
        free(ctx->slice_param_list);
        ctx->slice_param_list = NULL;
    }

    if (ctx->colocated_buffers) {
        for (i = 0; i < ctx->colocated_buffers_idx; ++i)
            psb_buffer_destroy(&(ctx->colocated_buffers[i]));

        free(ctx->colocated_buffers);
        ctx->colocated_buffers = NULL;
    }
}

VAStatus vld_dec_RenderPicture(
    object_context_p obj_context,
    object_buffer_p *buffers,
    int num_buffers)
{
    int i;
    context_DEC_p ctx = (context_DEC_p) obj_context->format_data;
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    for (i = 0; i < num_buffers; i++) {
        object_buffer_p obj_buffer = buffers[i];
        psb__dump_va_buffers_verbose(obj_buffer);

        switch (obj_buffer->type) {
        case VASliceParameterBufferType:
            vaStatus = vld_dec_add_slice_param(ctx, obj_buffer);
            DEBUG_FAILURE;
            break;

        case VASliceDataBufferType:
        case VAProtectedSliceDataBufferType:
            vaStatus = vld_dec_process_slice_data(ctx, obj_buffer);
            DEBUG_FAILURE;
            break;

        default:
            vaStatus = ctx->process_buffer(ctx, obj_buffer);
            DEBUG_FAILURE;
        }
        if (vaStatus != VA_STATUS_SUCCESS) {
            break;
        }
    }

    return vaStatus;
}

void vld_dec_yuv_rotate(object_context_p obj_context)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    struct format_vtable_s *vtable = &tng_yuv_processor_vtable;
    struct surface_param_s surface_param;
    struct object_buffer_s buffer;
    object_buffer_p buffer_p = &buffer;

    surface_param.src_surface = obj_context->current_render_target->scaling_surface;
    surface_param.display_width =	obj_context->current_render_target->buffer_width_s;
    surface_param.display_height = obj_context->current_render_target->buffer_height_s;
    surface_param.coded_width = obj_context->current_render_target->width_s;
    surface_param.coded_height = obj_context->current_render_target->height_s;

    buffer.num_elements = 1;
    buffer.type = YUVProcessorSurfaceType;
    buffer.size = sizeof(struct surface_param_s);
    buffer.buffer_data = (unsigned char *)&surface_param;

    vtable->createContext(obj_context, NULL);
    vtable->beginPicture(obj_context);
    vtable->renderPicture(obj_context, &buffer_p, 1);
    vtable->endPicture(obj_context);
    vtable->destroyContext(obj_context);
}
