/*
 * Copyright (c) 2014 Intel Corporation. All Rights Reserved.
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
 *    Kun Wang <kun.k.wang@intel.com>
 *
 */

#include "vsp_VPP.h"
#include "vsp_compose.h"
#include "psb_buffer.h"
#include "psb_surface.h"
#include "vsp_cmdbuf.h"
#include "psb_drv_debug.h"


#define INIT_DRIVER_DATA    psb_driver_data_p driver_data = (psb_driver_data_p) ctx->pDriverData;
#define INIT_CONTEXT_VPP    context_VPP_p ctx = (context_VPP_p) obj_context->format_data;
#define CONFIG(id)  ((object_config_p) object_heap_lookup( &driver_data->config_heap, id ))
#define CONTEXT(id) ((object_context_p) object_heap_lookup( &driver_data->context_heap, id ))
#define BUFFER(id)  ((object_buffer_p) object_heap_lookup( &driver_data->buffer_heap, id ))

#define SURFACE(id)    ((object_surface_p) object_heap_lookup( &ctx->obj_context->driver_data->surface_heap, id ))

#define ALIGN_TO_128(value) ((value + 128 - 1) & ~(128 - 1))
#define ALIGN_TO_16(value) ((value + 16 - 1) & ~(16 - 1))

VAStatus vsp_compose_process_pipeline_param(context_VPP_p ctx, object_context_p __maybe_unused obj_context, object_buffer_p obj_buffer)
{

	VAStatus vaStatus = VA_STATUS_SUCCESS;
	vsp_cmdbuf_p cmdbuf = ctx->obj_context->vsp_cmdbuf;
	VAProcPipelineParameterBuffer *pipeline_param = (VAProcPipelineParameterBuffer *)obj_buffer->buffer_data;
	struct VssWiDi_ComposeSequenceParameterBuffer *cell_compose_param = NULL;
	object_surface_p yuv_surface = NULL;
	object_surface_p rgb_surface = NULL;
	object_surface_p output_surface = NULL;
	int yuv_width = 0, yuv_height = 0, yuv_stride = 0;
	int rgb_width = 0, rgb_height = 0, rgb_stride = 0;
	int out_width = 0, out_height = 0, out_stride = 0;

	cell_compose_param = (struct VssWiDi_ComposeSequenceParameterBuffer *)cmdbuf->compose_param_p;

	/* The END command */
	if (pipeline_param->pipeline_flags & VA_PIPELINE_FLAG_END) {
		vsp_cmdbuf_compose_end(cmdbuf);
		/* Destory the VSP context */
		vsp_cmdbuf_vpp_context(cmdbuf, VssGenDestroyContext, CONTEXT_COMPOSE_ID, 0);
		goto out;
	}

	if (pipeline_param->num_additional_outputs <= 0 || !pipeline_param->additional_outputs) {
		drv_debug_msg(VIDEO_DEBUG_ERROR, "there isn't RGB surface!\n");
		vaStatus = VA_STATUS_ERROR_UNKNOWN;
		goto out;
	}

	/* Init the VSP context */
	if (ctx->obj_context->frame_count == 0)
		vsp_cmdbuf_vpp_context(cmdbuf, VssGenInitializeContext, CONTEXT_COMPOSE_ID, VSP_APP_ID_WIDI_ENC);

	yuv_surface = SURFACE(pipeline_param->surface);
	if (yuv_surface == NULL) {
		drv_debug_msg(VIDEO_DEBUG_ERROR, "invalid yuv surface %x\n", pipeline_param->surface);
		vaStatus = VA_STATUS_ERROR_UNKNOWN;
		goto out;
	}

	/* The RGB surface will be the first element */
	rgb_surface = SURFACE(pipeline_param->additional_outputs[0]);
	if (rgb_surface == NULL) {
		drv_debug_msg(VIDEO_DEBUG_ERROR, "invalid RGB surface %x\n", pipeline_param->additional_outputs[0]);
		vaStatus = VA_STATUS_ERROR_UNKNOWN;
		goto out;
	}

	output_surface = ctx->obj_context->current_render_target;

	yuv_width = ALIGN_TO_16(yuv_surface->width);
	yuv_height = yuv_surface->height_origin;
	yuv_stride = yuv_surface->psb_surface->stride;

	out_width = ALIGN_TO_16(output_surface->width);
	out_height = output_surface->height_origin;
	out_stride = output_surface->psb_surface->stride;

	rgb_width = ALIGN_TO_16(rgb_surface->width);
	rgb_height = ALIGN_TO_16(rgb_surface->height);
	rgb_stride = rgb_surface->psb_surface->stride;

	/* RGB related */
	cell_compose_param->ActualWidth = rgb_width;
	cell_compose_param->ActualHeight = rgb_height;
	cell_compose_param->ProcessedWidth = cell_compose_param->ActualWidth;
	cell_compose_param->ProcessedHeight = cell_compose_param->ActualHeight;
	cell_compose_param->TotalMBCount = ((cell_compose_param->ProcessedWidth >> 4) * (cell_compose_param->ProcessedHeight >> 4));
	cell_compose_param->Stride = rgb_stride;
	vsp_cmdbuf_reloc_pic_param(
				&(cell_compose_param->RGBA_Buffer),
				ctx->compose_param_offset,
				&(rgb_surface->psb_surface->buf),
				cmdbuf->param_mem_loc,
				cell_compose_param);


	/* Input YUV Video related */
	cell_compose_param->Video_IN_xsize = yuv_width;
	cell_compose_param->Video_IN_ysize = yuv_height;
	cell_compose_param->Video_IN_stride = yuv_stride;
	cell_compose_param->Video_IN_yuv_format = YUV_4_2_0_NV12;
	vsp_cmdbuf_reloc_pic_param(
				&(cell_compose_param->Video_IN_Y_Buffer),
				ctx->compose_param_offset,
				&(yuv_surface->psb_surface->buf),
				cmdbuf->param_mem_loc,
				cell_compose_param);

	cell_compose_param->Video_IN_UV_Buffer =
				cell_compose_param->Video_IN_Y_Buffer +
				cell_compose_param->Video_IN_ysize * cell_compose_param->Video_IN_stride;

	/* Output Video related */
	cell_compose_param->Video_OUT_xsize = out_width;
	cell_compose_param->Video_OUT_ysize = out_height;
	cell_compose_param->Video_OUT_stride = out_stride;
	cell_compose_param->Video_OUT_yuv_format = cell_compose_param->Video_IN_yuv_format;
	vsp_cmdbuf_reloc_pic_param(
				&(cell_compose_param->Video_OUT_Y_Buffer),
				ctx->compose_param_offset,
				&(output_surface->psb_surface->buf),
				cmdbuf->param_mem_loc,
				cell_compose_param);

	cell_compose_param->Video_OUT_UV_Buffer =
				cell_compose_param->Video_OUT_Y_Buffer +
				cell_compose_param->Video_OUT_ysize * cell_compose_param->Video_OUT_stride;

	/* Blending related params */
	cell_compose_param->Is_video_the_back_ground = 1;
	if (cell_compose_param->Is_video_the_back_ground) {
		cell_compose_param->ROI_width = cell_compose_param->ProcessedWidth;
		cell_compose_param->ROI_height = cell_compose_param->ProcessedHeight;
		cell_compose_param->ROI_x1 = 0;
		cell_compose_param->ROI_y1 = 0;
		cell_compose_param->ROI_x2 = 0;
		cell_compose_param->ROI_y2 = 0;

	} else {
		cell_compose_param->ROI_width = cell_compose_param->Video_IN_xsize;
		cell_compose_param->ROI_height = cell_compose_param->Video_IN_ysize;
		cell_compose_param->ROI_x1 = 0;
		cell_compose_param->ROI_y1 = 0;
		cell_compose_param->ROI_x2 = 0;
		cell_compose_param->ROI_y2 = 0;
	}

	cell_compose_param->scaled_width = out_width;
	cell_compose_param->scaled_height = out_height;
	cell_compose_param->scalefactor_dx = (unsigned int)(1024 / (((float)out_width) / yuv_width) + 0.5);
	cell_compose_param->scalefactor_dy = (unsigned int)(1024 / (((float)out_height) / yuv_height) + 0.5);
	cell_compose_param->Video_TotalMBCount =
		(((out_width + 15) >> 4) * ((out_height + 15) >> 4));

	cell_compose_param->ROI_width = cell_compose_param->scaled_width;
	cell_compose_param->ROI_height = cell_compose_param->scaled_height;

	cell_compose_param->Is_Blending_Enabled = 1;
	cell_compose_param->alpha1 = 128;
	cell_compose_param->alpha2 = 255;
	cell_compose_param->Is_source_1_image_available = 1;
	cell_compose_param->Is_source_2_image_available = 1;
	cell_compose_param->Is_alpha_channel_available = 1; /* 0: RGB Planar; 1: RGBA Interleaved */
	cell_compose_param->CSC_FormatSelect = 0; /* 0: YUV420NV12; 1: YUV444; */
	cell_compose_param->CSC_InputFormatSelect = 1; /* 0: RGB Planar; 1: RGBA Interleaved */

	/* The first frame */
	if (obj_context->frame_count == 0) {
		vsp_cmdbuf_insert_command(cmdbuf,
					CONTEXT_COMPOSE_ID,
					&cmdbuf->param_mem,
					VssWiDi_ComposeSetSequenceParametersCommand,
					ctx->compose_param_offset,
					sizeof(struct VssWiDi_ComposeSequenceParameterBuffer));
	}

	vsp_cmdbuf_insert_command(cmdbuf,
				CONTEXT_COMPOSE_ID,
				&cmdbuf->param_mem,
				VssWiDi_ComposeFrameCommand,
				ctx->compose_param_offset,
				sizeof(struct VssWiDi_ComposeSequenceParameterBuffer));

	/* Insert Fence Command */
	vsp_cmdbuf_fence_compose_param(cmdbuf, wsbmKBufHandle(wsbmKBuf(cmdbuf->param_mem.drm_buf)));

out:
	free(pipeline_param);
	obj_buffer->buffer_data = NULL;
	obj_buffer->size = 0;

	return vaStatus;
}
