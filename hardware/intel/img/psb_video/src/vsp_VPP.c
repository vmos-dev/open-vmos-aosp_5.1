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
 *    Binglin Chen <binglin.chen@intel.com>
 *
 */

#include "vsp_VPP.h"
#include "psb_buffer.h"
#include "psb_surface.h"
#include "vsp_cmdbuf.h"
#include "psb_drv_debug.h"
#include "vsp_compose.h"

#define INIT_DRIVER_DATA    psb_driver_data_p driver_data = (psb_driver_data_p) ctx->pDriverData;
#define INIT_CONTEXT_VPP    context_VPP_p ctx = (context_VPP_p) obj_context->format_data;
#define CONFIG(id)  ((object_config_p) object_heap_lookup( &driver_data->config_heap, id ))
#define CONTEXT(id) ((object_context_p) object_heap_lookup( &driver_data->context_heap, id ))
#define BUFFER(id)  ((object_buffer_p) object_heap_lookup( &driver_data->buffer_heap, id ))

#define SURFACE(id)    ((object_surface_p) object_heap_lookup( &ctx->obj_context->driver_data->surface_heap, id ))

#define KB 1024
#define MB (KB * KB)
#define VSP_CONTEXT_BUF_SIZE (60*KB)
#define VSP_INTERMEDIATE_BUF_SIZE (29*MB)

#define MAX_VPP_PARAM (100)
#define MIN_VPP_PARAM (0)
#define STEP_VPP_PARAM (33)
#define MAX_VPP_AUTO_PARAM (1)
#define MIN_VPP_AUTO_PARAM (0)
#define STEP_VPP_AUTO_PARAM (1)

#define VSP_FORWARD_REF_NUM 3

#define VSP_COLOR_ENHANCE_FEATURES 2

#define ALIGN_TO_128(value) ((value + 128 - 1) & ~(128 - 1))
#define ALIGN_TO_16(value) ((value + 16 - 1) & ~(16 - 1))

#define QVGA_AREA (320 * 240)
#define VGA_AREA (640 * 480)
#define SD_AREA (720 * 576)
#define HD720P_AREA (1280 * 720)
#define HD1080P_AREA (1920 * 1088)

#define MIN_SUPPORTED_HEIGHT 96
#define MAX_SUPPORTED_HEIGHT 1088

/**
 * The number of supported filter is 5:
 * VAProcFilterDeblocking
 * VAProcFilterNoiseReduction
 * VAProcFilterSharpening
 * VAProcFilterColorBalance
 * VAProcFilterFrameRateConversion
 */
#define VSP_SUPPORTED_FILTERS_NUM 5

/* The size of supported color standard */
#define COLOR_STANDARDS_NUM 1

enum resolution_set {
	NOT_SUPPORTED_RESOLUTION = -1,
	QCIF_TO_QVGA = 0,
	QVGA_TO_VGA,
	VGA_TO_SD,
	SD_TO_720P,
	HD720P_TO_1080P,
	RESOLUTION_SET_NUM
};

struct vpp_chain_capability {
	int frc_enabled;
	int sharpen_enabled;
	int color_balance_enabled;
	int denoise_enabled;
	int deblock_enabled;
};

enum filter_status {
	FILTER_DISABLED = 0,
	FILTER_ENABLED
};

struct vpp_chain_capability vpp_chain_caps[RESOLUTION_SET_NUM] = {
	[HD720P_TO_1080P] = {FILTER_ENABLED, FILTER_ENABLED, FILTER_DISABLED, FILTER_DISABLED, FILTER_DISABLED},
	[SD_TO_720P] = {FILTER_ENABLED, FILTER_ENABLED, FILTER_DISABLED, FILTER_DISABLED, FILTER_DISABLED},
	[VGA_TO_SD] = {FILTER_ENABLED, FILTER_ENABLED, FILTER_DISABLED, FILTER_DISABLED, FILTER_DISABLED},
	[QVGA_TO_VGA] = {FILTER_ENABLED, FILTER_ENABLED, FILTER_ENABLED, FILTER_ENABLED, FILTER_DISABLED},
	[QCIF_TO_QVGA] = {FILTER_ENABLED, FILTER_ENABLED, FILTER_ENABLED, FILTER_DISABLED, FILTER_ENABLED}
};

struct filter_strength {
	struct VssProcDenoiseParameterBuffer denoise_deblock[RESOLUTION_SET_NUM];
	struct VssProcColorEnhancementParameterBuffer enhancer[RESOLUTION_SET_NUM];
	struct VssProcSharpenParameterBuffer sharpen[RESOLUTION_SET_NUM];
};

enum filter_strength_type {
	INVALID_STRENGTH = -1,
	LOW_STRENGTH = 0,
	MEDIUM_STRENGTH,
	HIGH_STRENGTH,
	STRENGTH_NUM
};

#define SHARPEN_ON (1)

struct filter_strength vpp_strength[STRENGTH_NUM] = {
	[LOW_STRENGTH] = {
		/* structure:
		 * type(0-Denoise,1-Deblock), value_thr, cnt_thr, coef, temp_thr1, temp_thr2, _pad[2]
		 */
		.denoise_deblock = {
			[QCIF_TO_QVGA]    = {1, 15, 47, 35, 0, 0, {0, 0}},
			[QVGA_TO_VGA]     = {0, 7,  48, 47, 0, 0, {0, 0}},
			[VGA_TO_SD]       = {0, 10, 8,  9,  1, 3, {0, 0}},
			[SD_TO_720P]      = {0, 10, 48, 47, 0, 0, {0, 0}},
			[HD720P_TO_1080P] = {0, 10, 48, 47, 0, 0, {0, 0}}
		},
		/* structure:
		 * temp_detect, temp_correct, clip_thr, mid_thr, luma_amm, chroma_amm, _pad[2]
		 */
		.enhancer = {
			[QCIF_TO_QVGA]    = {200, 100, 1, 42, 40, 60, {0, 0}},
			[QVGA_TO_VGA]     = {220, 180, 1, 42, 40, 60, {0, 0}},
			[VGA_TO_SD]       = {220, 200, 1, 42, 40, 60, {0, 0}},
			[SD_TO_720P]      = {100, 100, 5, 33, 0,  0,  {0, 0}},
			[HD720P_TO_1080P] = {100, 100, 5, 33, 0,  0,  {0, 0}}
		},
		.sharpen = {
			[QCIF_TO_QVGA]    = { .quality = SHARPEN_ON },
			[QVGA_TO_VGA]     = { .quality = SHARPEN_ON },
			[VGA_TO_SD]       = { .quality = SHARPEN_ON },
			[SD_TO_720P]      = { .quality = SHARPEN_ON },
			[HD720P_TO_1080P] = { .quality = SHARPEN_ON }
		}
	},
	[MEDIUM_STRENGTH] = {
		.denoise_deblock = {
			[QCIF_TO_QVGA]    = {1, 25, 47, 12, 0, 0, {0, 0}},
			[QVGA_TO_VGA]     = {0, 10, 48, 47, 0, 0, {0, 0}},
			[VGA_TO_SD]       = {0, 20, 8,  9,  2, 4, {0, 0}},
			[SD_TO_720P]      = {0, 10, 48, 47, 0, 0, {0, 0}},
			[HD720P_TO_1080P] = {0, 10, 48, 47, 0, 0, {0, 0}}
		},
		.enhancer = {
			[QCIF_TO_QVGA]    = {100, 100, 1, 33, 100, 100, {0, 0}},
			[QVGA_TO_VGA]     = {100, 180, 1, 33, 100, 100, {0, 0}},
			[VGA_TO_SD]       = {100, 200, 1, 33, 100, 100, {0, 0}},
			[SD_TO_720P]      = {100, 100, 5, 33, 0,   0,   {0, 0}},
			[HD720P_TO_1080P] = {100, 100, 5, 33, 0,   0,   {0, 0}}
		},
		.sharpen = {
			[QCIF_TO_QVGA]    = { .quality = SHARPEN_ON },
			[QVGA_TO_VGA]     = { .quality = SHARPEN_ON },
			[VGA_TO_SD]       = { .quality = SHARPEN_ON },
			[SD_TO_720P]      = { .quality = SHARPEN_ON },
			[HD720P_TO_1080P] = { .quality = SHARPEN_ON }
		}
	},
	[HIGH_STRENGTH] = {
		.denoise_deblock = {
			[QCIF_TO_QVGA]    = {1, 30, 40, 10, 0, 0, {0, 0}},
			[QVGA_TO_VGA]     = {0, 15, 45, 25, 0, 0, {0, 0}},
			[VGA_TO_SD]       = {0, 20, 7,  5,  3, 6, {0, 0}},
			[SD_TO_720P]      = {0, 10, 48, 47, 0, 0, {0, 0}},
			[HD720P_TO_1080P] = {0, 10, 48, 47, 0, 0, {0, 0}}
		},
		.enhancer = {
			[QCIF_TO_QVGA]    = {100, 100, 5, 33, 150, 200, {0, 0}},
			[QVGA_TO_VGA]     = {100, 180, 5, 33, 150, 200, {0, 0}},
			[VGA_TO_SD]       = {100, 200, 5, 33, 100, 150, {0, 0}},
			[SD_TO_720P]      = {100, 100, 5, 33, 0,   0,   {0, 0}},
			[HD720P_TO_1080P] = {100, 100, 5, 33, 0,   0,   {0, 0}}
		},
		.sharpen = {
			[QCIF_TO_QVGA]    = { .quality = SHARPEN_ON },
			[QVGA_TO_VGA]     = { .quality = SHARPEN_ON },
			[VGA_TO_SD]       = { .quality = SHARPEN_ON },
			[SD_TO_720P]      = { .quality = SHARPEN_ON },
			[HD720P_TO_1080P] = { .quality = SHARPEN_ON }
		}
	}
};

static void vsp_VPP_DestroyContext(object_context_p obj_context);
static VAStatus vsp_set_pipeline(context_VPP_p ctx);
static VAStatus vsp_set_filter_param(context_VPP_p ctx);
static VAStatus vsp__VPP_check_legal_picture(object_context_p obj_context, object_config_p obj_config);
static int check_resolution(int width, int height);
static int check_vpp_strength(int value);

static void vsp_VPP_QueryConfigAttributes(
	VAProfile __maybe_unused profile,
	VAEntrypoint __maybe_unused entrypoint,
	VAConfigAttrib __maybe_unused *attrib_list,
	int __maybe_unused num_attribs)
{
	/* No VPP specific attributes */
	return;
}

static VAStatus vsp_VPP_ValidateConfig(
	object_config_p obj_config)
{
	int i;
	/* Check all attributes */
	for (i = 0; i < obj_config->attrib_count; i++) {
		switch (obj_config->attrib_list[i].type) {
		case VAConfigAttribRTFormat:
			/* Ignore */
			break;

		default:
			return VA_STATUS_ERROR_ATTR_NOT_SUPPORTED;
		}
	}

	return VA_STATUS_SUCCESS;
}

static VAStatus vsp__VPP_check_legal_picture(object_context_p obj_context, object_config_p obj_config)
{
	VAStatus vaStatus = VA_STATUS_SUCCESS;

	if (NULL == obj_context) {
		vaStatus = VA_STATUS_ERROR_INVALID_CONTEXT;
		DEBUG_FAILURE;
		return vaStatus;
	}

	if (NULL == obj_config) {
		vaStatus = VA_STATUS_ERROR_INVALID_CONFIG;
		DEBUG_FAILURE;
		return vaStatus;
	}

	return vaStatus;
}

static VAStatus vsp_VPP_CreateContext(
	object_context_p obj_context,
	object_config_p obj_config)
{
	VAStatus vaStatus = VA_STATUS_SUCCESS;
	context_VPP_p ctx;
	int i;

	/* Validate flag */
	/* Validate picture dimensions */
	vaStatus = vsp__VPP_check_legal_picture(obj_context, obj_config);
	if (VA_STATUS_SUCCESS != vaStatus) {
		DEBUG_FAILURE;
		return vaStatus;
	}

	ctx = (context_VPP_p) calloc(1, sizeof(struct context_VPP_s));
	if (NULL == ctx) {
		vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
		DEBUG_FAILURE;
		return vaStatus;
	}

	ctx->filters = NULL;
	ctx->num_filters = 0;

	ctx->frc_buf = NULL;

	/* set size */
	ctx->param_sz = 0;
	ctx->pic_param_sz = ALIGN_TO_128(sizeof(struct VssProcPictureParameterBuffer));
	ctx->param_sz += ctx->pic_param_sz;
	ctx->end_param_sz = ALIGN_TO_128(sizeof(struct VssProcPictureParameterBuffer));
	ctx->param_sz += ctx->end_param_sz;

	ctx->pipeline_param_sz = ALIGN_TO_128(sizeof(struct VssProcPipelineParameterBuffer));
	ctx->param_sz += ctx->pipeline_param_sz;
	ctx->denoise_param_sz = ALIGN_TO_128(sizeof(struct VssProcDenoiseParameterBuffer));
	ctx->param_sz += ctx->denoise_param_sz;
	ctx->enhancer_param_sz = ALIGN_TO_128(sizeof(struct VssProcColorEnhancementParameterBuffer));
	ctx->param_sz += ctx->enhancer_param_sz;
	ctx->sharpen_param_sz = ALIGN_TO_128(sizeof(struct VssProcSharpenParameterBuffer));
	ctx->param_sz += ctx->sharpen_param_sz;
	ctx->frc_param_sz = ALIGN_TO_128(sizeof(struct VssProcFrcParameterBuffer));
	ctx->param_sz += ctx->frc_param_sz;
	ctx->compose_param_sz = ALIGN_TO_128(sizeof(struct VssWiDi_ComposeSequenceParameterBuffer));
	ctx->param_sz += ctx->compose_param_sz;

	/* set offset */
	ctx->pic_param_offset = 0;
	ctx->end_param_offset = ctx->pic_param_offset + ctx->pic_param_sz;
	ctx->pipeline_param_offset = ctx->end_param_offset + ctx->end_param_sz;
	ctx->denoise_param_offset = ctx->pipeline_param_offset + ctx->pipeline_param_sz;
	ctx->enhancer_param_offset = ctx->denoise_param_offset + ctx->denoise_param_sz;
	ctx->sharpen_param_offset = ctx->enhancer_param_offset + ctx->enhancer_param_sz;
	ctx->frc_param_offset = ctx->sharpen_param_offset + ctx->sharpen_param_sz;
	/* For composer, it'll start on 0 */
	ctx->compose_param_offset = 0;

	/* create intermediate buffer */
	ctx->intermediate_buf = (psb_buffer_p) calloc(1, sizeof(struct psb_buffer_s));
	if (NULL == ctx->intermediate_buf) {
		vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
		DEBUG_FAILURE;
		goto out;
	}
	vaStatus = psb_buffer_create(obj_context->driver_data, VSP_INTERMEDIATE_BUF_SIZE, psb_bt_vpu_only, ctx->intermediate_buf);
	if (VA_STATUS_SUCCESS != vaStatus) {
		goto out;
	}

	obj_context->format_data = (void*) ctx;
	ctx->obj_context = obj_context;

	for (i = 0; i < obj_config->attrib_count; ++i) {
		if (VAConfigAttribRTFormat == obj_config->attrib_list[i].type) {
			switch (obj_config->attrib_list[i].value) {
			case VA_RT_FORMAT_YUV420:
				ctx->format = VSP_NV12;
				break;
			case VA_RT_FORMAT_YUV422:
				ctx->format = VSP_NV16;
			default:
				ctx->format = VSP_NV12;
				break;
			}
			break;
		}
	}

	bzero(&ctx->denoise_deblock_param, sizeof(ctx->denoise_deblock_param));
	bzero(&ctx->enhancer_param, sizeof(ctx->enhancer_param));
	bzero(&ctx->sharpen_param, sizeof(ctx->sharpen_param));

	return vaStatus;
out:
	vsp_VPP_DestroyContext(obj_context);

	if (ctx)
		free(ctx);

	return vaStatus;
}

static void vsp_VPP_DestroyContext(
	object_context_p obj_context)
{
	INIT_CONTEXT_VPP;

	if (ctx->intermediate_buf) {
		psb_buffer_destroy(ctx->intermediate_buf);

		free(ctx->intermediate_buf);
		ctx->intermediate_buf = NULL;
	}

	if (ctx->filters) {
		free(ctx->filters);
		ctx->num_filters = 0;
	}

	free(obj_context->format_data);
	obj_context->format_data = NULL;
}

static VAStatus vsp__VPP_process_pipeline_param(context_VPP_p ctx, object_context_p obj_context, object_buffer_p obj_buffer)
{
	VAStatus vaStatus = VA_STATUS_SUCCESS;
	vsp_cmdbuf_p cmdbuf = ctx->obj_context->vsp_cmdbuf;
	unsigned int i = 0;
	VAProcPipelineParameterBuffer *pipeline_param = (VAProcPipelineParameterBuffer *) obj_buffer->buffer_data;
	struct VssProcPictureParameterBuffer *cell_proc_picture_param = (struct VssProcPictureParameterBuffer *)cmdbuf->pic_param_p;
	struct VssProcPictureParameterBuffer *cell_end_param = (struct VssProcPictureParameterBuffer *)cmdbuf->end_param_p;
	VAProcFilterParameterBufferFrameRateConversion *frc_param;
	object_surface_p input_surface = NULL;
	object_surface_p cur_output_surf = NULL;
	unsigned int rotation_angle = 0, vsp_rotation_angle = 0;
	unsigned int tiled = 0, width = 0, height = 0, stride = 0;
	unsigned char *src_addr, *dest_addr;
	struct psb_surface_s *output_surface;
	psb_surface_share_info_p input_share_info = NULL;
	psb_surface_share_info_p output_share_info = NULL;
 	enum vsp_format format;


	psb_driver_data_p driver_data = obj_context->driver_data;

	if (pipeline_param->surface_region != NULL) {
		drv_debug_msg(VIDEO_DEBUG_ERROR, "Cann't scale\n");
		vaStatus = VA_STATUS_ERROR_UNKNOWN;
		goto out;
	}

	if (pipeline_param->output_region != NULL) {
		drv_debug_msg(VIDEO_DEBUG_ERROR, "Cann't scale\n");
		vaStatus = VA_STATUS_ERROR_UNKNOWN;
		goto out;
	}

	if (pipeline_param->output_background_color != 0) {
		drv_debug_msg(VIDEO_DEBUG_ERROR, "Cann't support background color here\n");
		vaStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
		goto out;
	}

	if (pipeline_param->filters == NULL) {
		drv_debug_msg(VIDEO_DEBUG_ERROR, "invalid filter setting filters = %p\n", pipeline_param->filters);
		vaStatus = VA_STATUS_ERROR_UNKNOWN;
		goto out;
	}

#if 0
	/* for pass filter */
	if (pipeline_param->num_filters == 0 || pipeline_param->num_filters > VssProcPipelineMaxNumFilters) {
		drv_debug_msg(VIDEO_DEBUG_ERROR, "invalid filter number = %d\n", pipeline_param->num_filters);
		vaStatus = VA_STATUS_ERROR_UNKNOWN;
		goto out;
	}
#endif

	if (pipeline_param->forward_references == NULL) {
		drv_debug_msg(VIDEO_DEBUG_ERROR, "invalid forward_refereces %p setting\n", pipeline_param->forward_references);
		vaStatus = VA_STATUS_ERROR_UNKNOWN;
		goto out;
	}

	/* should we check it? since the begining it's not VSP_FORWARD_REF_NUM */
	if (pipeline_param->num_forward_references != VSP_FORWARD_REF_NUM) {
		drv_debug_msg(VIDEO_DEBUG_ERROR, "invalid num_forward_refereces %d setting, should be %d\n", pipeline_param->num_forward_references, VSP_FORWARD_REF_NUM);
		vaStatus = VA_STATUS_ERROR_UNKNOWN;
		goto out;
	}

	/* first picture, need to setup the VSP context */
	if (ctx->obj_context->frame_count == 0)
		vsp_cmdbuf_vpp_context(cmdbuf, VssGenInitializeContext, CONTEXT_VPP_ID, VSP_APP_ID_FRC_VPP);

	/* get the input surface */
	if (!(pipeline_param->pipeline_flags & VA_PIPELINE_FLAG_END)) {
		input_surface = SURFACE(pipeline_param->surface);
		if (input_surface == NULL) {
			drv_debug_msg(VIDEO_DEBUG_ERROR, "invalid input surface %x\n", pipeline_param->surface);
			vaStatus = VA_STATUS_ERROR_UNKNOWN;
			goto out;
		}
	} else {
		input_surface = NULL;
	}

	/* if it is the first pipeline command */
	if (pipeline_param->num_filters != ctx->num_filters || pipeline_param->num_filters == 0) {
		if (ctx->num_filters != 0) {
			drv_debug_msg(VIDEO_DEBUG_ERROR, "can not reset pipeline in the mid of post-processing or without create a new context\n");
			vaStatus = VA_STATUS_ERROR_UNKNOWN;
			goto out;
		} else {
			/* save filters */
			ctx->num_filters = pipeline_param->num_filters;
			if (ctx->num_filters == 0) {
				ctx->filters = NULL;
			} else {
				ctx->filters = (VABufferID *) calloc(ctx->num_filters, sizeof(*ctx->filters));
				if (ctx->filters == NULL) {
					drv_debug_msg(VIDEO_DEBUG_ERROR, "can not reset pipeline in the mid of post-processing or without create a new context\n");
					vaStatus = VA_STATUS_ERROR_UNKNOWN;
					goto out;
				}
				memcpy(ctx->filters, pipeline_param->filters, ctx->num_filters * sizeof(*ctx->filters));
			}

			/* set pipeline command to FW */
			vaStatus = vsp_set_pipeline(ctx);
			if (vaStatus) {
				drv_debug_msg(VIDEO_DEBUG_ERROR, "failed to set pipeline\n");
				goto out;
			}

			/* set filter parameter to FW, record frc parameter buffer */
			vaStatus = vsp_set_filter_param(ctx);
			if (vaStatus) {
				drv_debug_msg(VIDEO_DEBUG_ERROR, "failed to set filter parameter\n");
				goto out;
			}
		}
	} else {
		/* else ignore pipeline/filter setting  */
#if 0
		/* FIXME: we can save these check for PnP */
		for (i = 0; i < pipeline_param->num_filters; i++) {
			if (pipeline_param->filters[i] != ctx->filters[i]) {
				drv_debug_msg(VIDEO_DEBUG_ERROR, "can not reset pipeline in the mid of post-processing or without create a new context\n");
				vaStatus = VA_STATUS_ERROR_UNKNOWN;
				goto out;
			}
		}
#endif
	}

	/* fill picture command to FW */
	if (ctx->frc_buf != NULL)
		frc_param = (VAProcFilterParameterBufferFrameRateConversion *)ctx->frc_buf->buffer_data;
	else
		frc_param = NULL;

	/* end picture command */
	if (pipeline_param->pipeline_flags & VA_PIPELINE_FLAG_END) {
		cell_end_param->num_input_pictures = 0;
		cell_end_param->num_output_pictures = 0;
		vsp_cmdbuf_insert_command(cmdbuf, CONTEXT_VPP_ID, &cmdbuf->param_mem, VssProcPictureCommand,
					  ctx->end_param_offset, sizeof(struct VssProcPictureParameterBuffer));
		/* Destory the VSP context */
		vsp_cmdbuf_vpp_context(cmdbuf, VssGenDestroyContext, CONTEXT_VPP_ID, 0);
		goto out;
	}

#ifdef PSBVIDEO_VPP_TILING
	/* get the tiling flag*/
	tiled = GET_SURFACE_INFO_tiling(input_surface->psb_surface);
#endif

	/* get the surface format info  */
	switch (input_surface->psb_surface->extra_info[8]) {
		case VA_FOURCC_YV12:
			format = VSP_YV12;
			break;
		case VA_FOURCC_NV12:
			format = VSP_NV12;
			break;
		default:
			vaStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
			drv_debug_msg(VIDEO_DEBUG_ERROR, "Only support NV12 and YV12 format!\n");
			goto out;
	}

	/*  According to VIED's design, the width must be multiple of 16 */
	width = ALIGN_TO_16(input_surface->width);
	if (width > input_surface->psb_surface->stride)
		width = input_surface->psb_surface->stride;

	/* get the input share info */
	input_share_info = input_surface->share_info;
	drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s The input surface %p share info %p\n", __func__, input_surface,input_surface->share_info);

	/* Setup input surface */
	cell_proc_picture_param->num_input_pictures  = 1;
	cell_proc_picture_param->input_picture[0].surface_id = pipeline_param->surface;
	vsp_cmdbuf_reloc_pic_param(&(cell_proc_picture_param->input_picture[0].base), ctx->pic_param_offset, &(input_surface->psb_surface->buf),
				   cmdbuf->param_mem_loc, cell_proc_picture_param);
	cell_proc_picture_param->input_picture[0].height = input_surface->height_origin;
	cell_proc_picture_param->input_picture[0].width = width;
	cell_proc_picture_param->input_picture[0].irq = 0;
	cell_proc_picture_param->input_picture[0].stride = input_surface->psb_surface->stride;
	cell_proc_picture_param->input_picture[0].format = format;
	cell_proc_picture_param->input_picture[0].tiled = tiled;
	cell_proc_picture_param->input_picture[0].rot_angle = 0;

	/* Setup output surfaces */
	if (frc_param == NULL)
		cell_proc_picture_param->num_output_pictures = 1;
	else
		cell_proc_picture_param->num_output_pictures = frc_param->num_output_frames + 1;

	for (i = 0; i < cell_proc_picture_param->num_output_pictures; ++i) {
		if (i == 0) {
			cur_output_surf = ctx->obj_context->current_render_target;

#ifdef PSBVIDEO_MRFL_VPP_ROTATE
			/* The rotation info is saved in the first frame */
			rotation_angle = GET_SURFACE_INFO_rotate(cur_output_surf->psb_surface);
			switch (rotation_angle) {
				case VA_ROTATION_90:
					vsp_rotation_angle = VSP_ROTATION_90;
					break;
				case VA_ROTATION_180:
					vsp_rotation_angle = VSP_ROTATION_180;
					break;
				case VA_ROTATION_270:
					vsp_rotation_angle = VSP_ROTATION_270;
					break;
				default:
					vsp_rotation_angle = VSP_ROTATION_NONE;
			}
#endif
		} else {
			if (frc_param == NULL) {
				drv_debug_msg(VIDEO_DEBUG_ERROR, "invalid output surface numbers %x\n",
					      cell_proc_picture_param->num_output_pictures);
				vaStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
				goto out;
			}

			cur_output_surf = SURFACE(frc_param->output_frames[i-1]);
			if (cur_output_surf == NULL) {
				drv_debug_msg(VIDEO_DEBUG_ERROR, "invalid input surface %x\n", frc_param->output_frames[i-1]);
				vaStatus = VA_STATUS_ERROR_UNKNOWN;
				goto out;
			}

#ifdef PSBVIDEO_MRFL_VPP_ROTATE
			/* VPP rotation is just for 1080P */
			if (tiled && rotation_angle != VA_ROTATION_NONE) {
				if (VA_STATUS_SUCCESS != psb_CreateRotateSurface(obj_context, cur_output_surf, rotation_angle)) {
					drv_debug_msg(VIDEO_DEBUG_ERROR, "failed to alloc rotation surface!\n");
					vaStatus = VA_STATUS_ERROR_UNKNOWN;
					goto out;
				}
			}
#endif
		}

		if (tiled && rotation_angle != VA_ROTATION_NONE) {
#ifdef PSBVIDEO_MRFL_VPP_ROTATE
			/* For 90d and 270d, we need to alloc rotation buff and
			 * copy the 0d data from input to output
			 */
			psb_buffer_map(&(input_surface->psb_surface->buf), &src_addr);
			psb_buffer_map(&(cur_output_surf->psb_surface->buf), &dest_addr);
			memcpy(dest_addr, src_addr, cur_output_surf->psb_surface->size);
			psb_buffer_unmap(&(cur_output_surf->psb_surface->buf));
			psb_buffer_unmap(&(input_surface->psb_surface->buf));

			output_surface = cur_output_surf->out_loop_surface;

			/*  According to VIED's design, the width must be multiple of 16 */
			width = ALIGN_TO_16(cur_output_surf->height_origin);
			if (width > cur_output_surf->out_loop_surface->stride)
				width = cur_output_surf->out_loop_surface->stride;
			height = cur_output_surf->width;
			stride = cur_output_surf->out_loop_surface->stride;
#endif
		} else {
			output_surface = cur_output_surf->psb_surface;

			/*  According to VIED's design, the width must be multiple of 16 */
			width = ALIGN_TO_16(cur_output_surf->width);
			if (width > cur_output_surf->psb_surface->stride)
				width = cur_output_surf->psb_surface->stride;
			height = cur_output_surf->height_origin;
			stride = cur_output_surf->psb_surface->stride;

			/* Check the rotate bit */
			if (pipeline_param->rotation_state == VA_ROTATION_90)
				vsp_rotation_angle = VSP_ROTATION_90;
			else if (pipeline_param->rotation_state == VA_ROTATION_180)
				vsp_rotation_angle = VSP_ROTATION_180;
			else if (pipeline_param->rotation_state == VA_ROTATION_270)
				vsp_rotation_angle = VSP_ROTATION_270;
			else
				vsp_rotation_angle = VSP_ROTATION_NONE;
		}

		cell_proc_picture_param->output_picture[i].surface_id = wsbmKBufHandle(wsbmKBuf(output_surface->buf.drm_buf));

		vsp_cmdbuf_reloc_pic_param(&(cell_proc_picture_param->output_picture[i].base),
					   ctx->pic_param_offset, &(output_surface->buf),
					   cmdbuf->param_mem_loc, cell_proc_picture_param);
		cell_proc_picture_param->output_picture[i].height = height;
		cell_proc_picture_param->output_picture[i].width = width;
		cell_proc_picture_param->output_picture[i].stride = stride;
		cell_proc_picture_param->output_picture[i].irq = 1;
		cell_proc_picture_param->output_picture[i].format = format;
		cell_proc_picture_param->output_picture[i].rot_angle = vsp_rotation_angle;
		cell_proc_picture_param->output_picture[i].tiled = tiled;

		/* copy the input share info to output */
		output_share_info = cur_output_surf->share_info;
		if (input_share_info != NULL && output_share_info != NULL) {
            output_share_info->native_window = input_share_info->native_window;
            output_share_info->force_output_method = input_share_info->force_output_method;
            output_share_info->surface_protected = input_share_info->surface_protected;
            output_share_info->bob_deinterlace = input_share_info->bob_deinterlace;

            output_share_info->crop_width = input_share_info->crop_width;
            output_share_info->crop_height = input_share_info->crop_height;
            output_share_info->coded_width = input_share_info->coded_width;
            output_share_info->coded_height = input_share_info->coded_height;
			drv_debug_msg(VIDEO_DEBUG_GENERAL, "The input/output wxh %dx%d\n",input_share_info->width,input_share_info->height);
		} else {
			drv_debug_msg(VIDEO_DEBUG_WARNING, "The input/output share_info is NULL!!\n");
		}
	}

	vsp_cmdbuf_insert_command(cmdbuf, CONTEXT_VPP_ID, &cmdbuf->param_mem, VssProcPictureCommand,
				  ctx->pic_param_offset, sizeof(struct VssProcPictureParameterBuffer));

	vsp_cmdbuf_fence_pic_param(cmdbuf, wsbmKBufHandle(wsbmKBuf(cmdbuf->param_mem.drm_buf)));

#if 0
	/* handle reference frames, ignore backward reference */
	for (i = 0; i < pipeline_param->num_forward_references; ++i) {
		cur_output_surf = SURFACE(pipeline_param->forward_references[i]);
		if (cur_output_surf == NULL)
			continue;
		if (vsp_cmdbuf_buffer_ref(cmdbuf, &cur_output_surf->psb_surface->buf) < 0) {
			drv_debug_msg(VIDEO_DEBUG_ERROR, "vsp_cmdbuf_buffer_ref() failed\n");
			vaStatus = VA_STATUS_ERROR_UNKNOWN;
			goto out;
		}
	}
#endif
out:
	free(pipeline_param);
	obj_buffer->buffer_data = NULL;
	obj_buffer->size = 0;

	return vaStatus;
}

static VAStatus vsp_VPP_RenderPicture(
	object_context_p obj_context,
	object_buffer_p *buffers,
	int num_buffers)
{
	int i;
	INIT_CONTEXT_VPP;
	VAProcPipelineParameterBuffer *pipeline_param = NULL;
	VAStatus vaStatus = VA_STATUS_SUCCESS;

	for (i = 0; i < num_buffers; i++) {
		object_buffer_p obj_buffer = buffers[i];
		pipeline_param = (VAProcPipelineParameterBuffer *) obj_buffer->buffer_data;

		switch (obj_buffer->type) {
		case VAProcPipelineParameterBufferType:
			if (!pipeline_param->num_filters && pipeline_param->blend_state)
				/* For Security Composer */
				vaStatus = vsp_compose_process_pipeline_param(ctx, obj_context, obj_buffer);
			else
				/* For VPP/FRC */
				vaStatus = vsp__VPP_process_pipeline_param(ctx, obj_context, obj_buffer);
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

static VAStatus vsp_VPP_BeginPicture(
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

	/* map param mem */
	vaStatus = psb_buffer_map(&cmdbuf->param_mem, &cmdbuf->param_mem_p);
	if (vaStatus) {
		return vaStatus;
	}

	cmdbuf->pic_param_p = cmdbuf->param_mem_p + ctx->pic_param_offset;
	cmdbuf->end_param_p = cmdbuf->param_mem_p + ctx->end_param_offset;
	cmdbuf->pipeline_param_p = cmdbuf->param_mem_p + ctx->pipeline_param_offset;
	cmdbuf->denoise_param_p = cmdbuf->param_mem_p + ctx->denoise_param_offset;
	cmdbuf->enhancer_param_p = cmdbuf->param_mem_p + ctx->enhancer_param_offset;
	cmdbuf->sharpen_param_p = cmdbuf->param_mem_p + ctx->sharpen_param_offset;
	cmdbuf->frc_param_p = cmdbuf->param_mem_p + ctx->frc_param_offset;
	cmdbuf->compose_param_p = cmdbuf->param_mem_p + ctx->compose_param_offset;

	return VA_STATUS_SUCCESS;
}

static VAStatus vsp_VPP_EndPicture(
	object_context_p obj_context)
{
	INIT_CONTEXT_VPP;
	psb_driver_data_p driver_data = obj_context->driver_data;
	vsp_cmdbuf_p cmdbuf = obj_context->vsp_cmdbuf;

	if(cmdbuf->param_mem_p != NULL) {
		psb_buffer_unmap(&cmdbuf->param_mem);
		cmdbuf->param_mem_p = NULL;
		cmdbuf->pic_param_p = NULL;
		cmdbuf->end_param_p = NULL;
		cmdbuf->pipeline_param_p = NULL;
		cmdbuf->denoise_param_p = NULL;
		cmdbuf->enhancer_param_p = NULL;
		cmdbuf->sharpen_param_p = NULL;
		cmdbuf->frc_param_p = NULL;
		cmdbuf->compose_param_p = NULL;
	}

	if (vsp_context_flush_cmdbuf(ctx->obj_context)) {
		drv_debug_msg(VIDEO_DEBUG_GENERAL, "psb_VPP: flush deblock cmdbuf error\n");
		return VA_STATUS_ERROR_UNKNOWN;
	}

	return VA_STATUS_SUCCESS;
}

struct format_vtable_s vsp_VPP_vtable = {
queryConfigAttributes:
vsp_VPP_QueryConfigAttributes,
validateConfig:
vsp_VPP_ValidateConfig,
createContext:
vsp_VPP_CreateContext,
destroyContext:
vsp_VPP_DestroyContext,
beginPicture:
vsp_VPP_BeginPicture,
renderPicture:
vsp_VPP_RenderPicture,
endPicture:
vsp_VPP_EndPicture
};

VAStatus vsp_QueryVideoProcFilters(
	VADriverContextP    ctx,
	VAContextID         context,
	VAProcFilterType   *filters,
	unsigned int       *num_filters
	)
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
	if (*num_filters < VSP_SUPPORTED_FILTERS_NUM) {
		drv_debug_msg(VIDEO_DEBUG_ERROR, "The filters array size(%d) is NOT valid! Supported filters num is %d\n",
				*num_filters, VSP_SUPPORTED_FILTERS_NUM);
		vaStatus = VA_STATUS_ERROR_MAX_NUM_EXCEEDED;
		*num_filters = VSP_SUPPORTED_FILTERS_NUM;
		goto err;
	}

	/* check if current HW support Video proc */
	if (IS_MRFL(driver_data)) {
		count = 0;
		filters[count++] = VAProcFilterDeblocking;
		filters[count++] = VAProcFilterNoiseReduction;
		filters[count++] = VAProcFilterSharpening;
		filters[count++] = VAProcFilterColorBalance;
		filters[count++] = VAProcFilterFrameRateConversion;
		*num_filters = count;
	} else {
		*num_filters = 0;
	}
err:
	return vaStatus;
}

VAStatus vsp_QueryVideoProcFilterCaps(
	VADriverContextP    ctx,
	VAContextID         context,
	VAProcFilterType    type,
	void               *filter_caps,
	unsigned int       *num_filter_caps
	)
{
	INIT_DRIVER_DATA;
	VAStatus vaStatus = VA_STATUS_SUCCESS;
	object_context_p obj_context;
	object_config_p obj_config;
	VAEntrypoint tmp;
	VAProcFilterCap *denoise_cap, *deblock_cap;
	VAProcFilterCap *sharpen_cap;
	VAProcFilterCapColorBalance *color_balance_cap;
	VAProcFilterCap *frc_cap;

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
	if (IS_MRFL(driver_data)) {
		/* FIXME: we should use a constant table to return caps */
		switch (type) {
		case VAProcFilterNoiseReduction:
			denoise_cap = filter_caps;
			denoise_cap->range.min_value = MIN_VPP_PARAM;
			denoise_cap->range.max_value = MAX_VPP_PARAM;
			denoise_cap->range.default_value = MIN_VPP_PARAM;
			denoise_cap->range.step = STEP_VPP_PARAM;
			*num_filter_caps = 1;
			break;
		case VAProcFilterDeblocking:
			deblock_cap = filter_caps;
			deblock_cap->range.min_value = MIN_VPP_PARAM;
			deblock_cap->range.max_value = MAX_VPP_PARAM;
			deblock_cap->range.default_value = MIN_VPP_PARAM;
			deblock_cap->range.step = STEP_VPP_PARAM;
			*num_filter_caps = 1;
			break;

		case VAProcFilterSharpening:
			sharpen_cap = filter_caps;
			sharpen_cap->range.min_value = MIN_VPP_PARAM;
			sharpen_cap->range.max_value = MAX_VPP_PARAM;
			sharpen_cap->range.default_value = MIN_VPP_PARAM;
			sharpen_cap->range.step = STEP_VPP_PARAM;
			*num_filter_caps = 1;
			break;

		case VAProcFilterColorBalance:
			if (*num_filter_caps < VSP_COLOR_ENHANCE_FEATURES) {
				drv_debug_msg(VIDEO_DEBUG_ERROR, "filter cap num is should big than %d(%d)\n",
					      VSP_COLOR_ENHANCE_FEATURES, *num_filter_caps);
				vaStatus = VA_STATUS_ERROR_MAX_NUM_EXCEEDED;
				*num_filter_caps = VSP_COLOR_ENHANCE_FEATURES;
				goto err;
			}
			color_balance_cap = filter_caps;
			color_balance_cap->type = VAProcColorBalanceAutoSaturation;
			color_balance_cap->range.min_value = MIN_VPP_AUTO_PARAM;
			color_balance_cap->range.max_value = MAX_VPP_AUTO_PARAM;
			color_balance_cap->range.default_value = MIN_VPP_AUTO_PARAM;
			color_balance_cap->range.step = STEP_VPP_AUTO_PARAM;

			color_balance_cap++;
			color_balance_cap->type = VAProcColorBalanceAutoBrightness;
			color_balance_cap->range.min_value = MIN_VPP_AUTO_PARAM;
			color_balance_cap->range.max_value = MAX_VPP_AUTO_PARAM;
			color_balance_cap->range.default_value = MIN_VPP_AUTO_PARAM;
			color_balance_cap->range.step = STEP_VPP_AUTO_PARAM;

			*num_filter_caps = 2;
			break;

		case VAProcFilterFrameRateConversion:
			frc_cap = filter_caps;
			frc_cap->range.min_value = 2;
			frc_cap->range.max_value = 4;
			frc_cap->range.default_value = 2;
			/* FIXME: it's a set, step is helpless */
			frc_cap->range.step = 0.5;
			*num_filter_caps = 1;
			break;

		default:
			drv_debug_msg(VIDEO_DEBUG_ERROR, "invalide filter type %d\n", type);
			vaStatus = VA_STATUS_ERROR_UNSUPPORTED_FILTER;
			*num_filter_caps = 0;
			goto err;
		}
	} else {
		*num_filter_caps = 0;
	}

err:
	return vaStatus;
}

VAStatus vsp_QueryVideoProcPipelineCaps(
	VADriverContextP    ctx,
	VAContextID         context,
	VABufferID         *filters,
	unsigned int        num_filters,
	VAProcPipelineCaps *pipeline_caps
    )
{
	INIT_DRIVER_DATA;
	VAStatus vaStatus = VA_STATUS_SUCCESS;
	object_context_p obj_context;
	object_config_p obj_config;
	VAEntrypoint tmp;
	unsigned int i, j;
	VAProcFilterParameterBuffer *deblock, *denoise, *sharpen;
	VAProcFilterParameterBufferFrameRateConversion *frc;
	VAProcFilterParameterBufferColorBalance *balance;
	VAProcFilterParameterBufferBase *base;
	object_buffer_p buf;
	uint32_t enabled_brightness, enabled_saturation;
	float ratio;
	int res_set;
	int strength;
	context_VPP_p vpp_ctx;
	int combination_check;

	/* check if ctx is right */
	obj_context = CONTEXT(context);
	if (NULL == obj_context) {
		drv_debug_msg(VIDEO_DEBUG_ERROR, "Failed to find context\n");
		vaStatus = VA_STATUS_ERROR_INVALID_CONTEXT;
		goto err;
	}

	vpp_ctx = (context_VPP_p) obj_context->format_data;

	obj_config = CONFIG(obj_context->config_id);
	if (NULL == obj_config) {
		drv_debug_msg(VIDEO_DEBUG_ERROR, "Failed to find config\n");
		vaStatus = VA_STATUS_ERROR_INVALID_CONFIG;
		goto err;
	}

	/* Don't check the filter number.
	 * According to VIED's design, without any filter, HW will just copy input data
	 */
#if 0
	if (num_filters == 0) {
		drv_debug_msg(VIDEO_DEBUG_ERROR, "invalid num_filters %d\n", num_filters);
		vaStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
		goto err;
	}
#endif
	if (NULL == filters || pipeline_caps == NULL) {
		drv_debug_msg(VIDEO_DEBUG_ERROR, "invalid filters %p or pipeline_caps %p\n", filters, pipeline_caps);
		vaStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
		goto err;
	}

	/* base on HW capability check the filters and return pipeline caps */
	if (IS_MRFL(driver_data)) {
		pipeline_caps->pipeline_flags = 0;
		pipeline_caps->filter_flags = 0;
		pipeline_caps->num_forward_references = VSP_FORWARD_REF_NUM;
		pipeline_caps->num_backward_references = 0;

		/* check the input color standard */
		if (pipeline_caps->input_color_standards == NULL){
			drv_debug_msg(VIDEO_DEBUG_ERROR, "invalid input color standard array!\n");
			vaStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
			goto err;
		}
		if (pipeline_caps->num_input_color_standards < COLOR_STANDARDS_NUM) {
			drv_debug_msg(VIDEO_DEBUG_ERROR, "invalid num_input_color_standards %d\n", pipeline_caps->num_input_color_standards);
			vaStatus = VA_STATUS_ERROR_MAX_NUM_EXCEEDED;
			pipeline_caps->num_input_color_standards = COLOR_STANDARDS_NUM;
			goto err;
		}
		pipeline_caps->input_color_standards[0] = VAProcColorStandardNone;
		pipeline_caps->num_input_color_standards = COLOR_STANDARDS_NUM;

		/* check the output color standard */
		if (pipeline_caps->output_color_standards == NULL){
			drv_debug_msg(VIDEO_DEBUG_ERROR, "invalid output color standard array!\n");
			vaStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
			goto err;
		}
		if (pipeline_caps->num_output_color_standards < COLOR_STANDARDS_NUM) {
			drv_debug_msg(VIDEO_DEBUG_ERROR, "invalid num_output_color_standards %d\n", pipeline_caps->num_output_color_standards);
			vaStatus = VA_STATUS_ERROR_MAX_NUM_EXCEEDED;
			pipeline_caps->num_output_color_standards = COLOR_STANDARDS_NUM;
			goto err;
		}
		pipeline_caps->output_color_standards[0] = VAProcColorStandardNone;
		pipeline_caps->num_output_color_standards = COLOR_STANDARDS_NUM;

		/* check the resolution */
		res_set = check_resolution(obj_context->picture_width,
					   obj_context->picture_height);
		if (res_set == NOT_SUPPORTED_RESOLUTION) {
			vaStatus = VA_STATUS_ERROR_RESOLUTION_NOT_SUPPORTED;
			goto err;
		}

		/* Blend type */
		pipeline_caps->blend_flags = VA_BLEND_PREMULTIPLIED_ALPHA;

		if (getenv("VSP_PIPELINE_CHECK") != NULL)
			combination_check = 1;
		else
			combination_check = 0;

		/* FIXME: should check filter value settings here */
		for (i = 0; i < num_filters; ++i) {
			/* find buffer */
			buf = BUFFER(*(filters + i));
			if (!buf) {
				vaStatus = VA_STATUS_ERROR_UNKNOWN;
				goto err;
			}

			base = (VAProcFilterParameterBufferBase *)buf->buffer_data;
			/* check filter buffer setting */
			switch (base->type) {
			case VAProcFilterDeblocking:
				deblock = (VAProcFilterParameterBuffer *)base;

				if (combination_check &&
				    vpp_chain_caps[res_set].deblock_enabled != FILTER_ENABLED) {
					drv_debug_msg(VIDEO_DEBUG_ERROR, "The deblock is DISABLE for %d format\n", res_set);
					vaStatus = VA_STATUS_ERROR_INVALID_FILTER_CHAIN;
					goto err;
				} else {
					/* check if the value is right */
					strength = check_vpp_strength(deblock->value);
					if (strength == INVALID_STRENGTH) {
						vaStatus = VA_STATUS_ERROR_INVALID_VALUE;
						goto err;
					}
					memcpy(&vpp_ctx->denoise_deblock_param,
					       &vpp_strength[strength].denoise_deblock[res_set],
					       sizeof(vpp_ctx->denoise_deblock_param));
				}
				break;

			case VAProcFilterNoiseReduction:
				denoise = (VAProcFilterParameterBuffer *)base;

				if (combination_check &&
				    vpp_chain_caps[res_set].denoise_enabled != FILTER_ENABLED) {
					drv_debug_msg(VIDEO_DEBUG_ERROR, "The denoise is DISABLE for %d format\n", res_set);
					vaStatus = VA_STATUS_ERROR_INVALID_FILTER_CHAIN;
					goto err;
				} else {
					strength = check_vpp_strength(denoise->value);
					if (strength == INVALID_STRENGTH) {
						vaStatus = VA_STATUS_ERROR_INVALID_VALUE;
						goto err;
					}
					memcpy(&vpp_ctx->denoise_deblock_param,
					       &vpp_strength[strength].denoise_deblock[res_set],
					       sizeof(vpp_ctx->denoise_deblock_param));
				}
				break;

			case VAProcFilterSharpening:
				sharpen = (VAProcFilterParameterBuffer *)base;

				if (combination_check &&
				    vpp_chain_caps[res_set].sharpen_enabled != FILTER_ENABLED) {
					drv_debug_msg(VIDEO_DEBUG_ERROR, "The sharpen is DISABLE for %d format\n", res_set);
					vaStatus = VA_STATUS_ERROR_INVALID_FILTER_CHAIN;
					goto err;
				} else {
					strength = check_vpp_strength(sharpen->value);
					if (strength == INVALID_STRENGTH) {
						vaStatus = VA_STATUS_ERROR_INVALID_VALUE;
						goto err;
					}
					memcpy(&vpp_ctx->sharpen_param,
					      &vpp_strength[strength].sharpen[res_set],
					       sizeof(vpp_ctx->sharpen_param));
				}
				break;

			case VAProcFilterColorBalance:
				balance = (VAProcFilterParameterBufferColorBalance *)base;

				enabled_brightness = 0;
				enabled_saturation = 0;

				for (j = 0; j < buf->num_elements; ++j, ++balance) {
					if (balance->attrib == VAProcColorBalanceAutoSaturation &&
					    balance->value == MAX_VPP_AUTO_PARAM) {
						enabled_saturation = 1;
					} else if (balance->attrib == VAProcColorBalanceAutoBrightness &&
						   balance->value == MAX_VPP_AUTO_PARAM) {
						enabled_brightness = 1;
					} else {
						drv_debug_msg(VIDEO_DEBUG_ERROR, "The color_banlance do NOT support this attrib %d\n",
							      balance->attrib);
						vaStatus = VA_STATUS_ERROR_UNSUPPORTED_FILTER;
						goto err;
					}
				}

				/* check filter chain */
				if (combination_check &&
				    vpp_chain_caps[res_set].color_balance_enabled != FILTER_ENABLED) {
					drv_debug_msg(VIDEO_DEBUG_ERROR, "The color_balance is DISABLE for %d format\n", res_set);
					vaStatus = VA_STATUS_ERROR_INVALID_FILTER_CHAIN;
					goto err;
				} else {
					strength = MEDIUM_STRENGTH;
					memcpy(&vpp_ctx->enhancer_param,
					       &vpp_strength[strength].enhancer[res_set],
					       sizeof(vpp_ctx->enhancer_param));
					if (!enabled_saturation)
						vpp_ctx->enhancer_param.chroma_amm = 0;
					if (!enabled_brightness)
						vpp_ctx->enhancer_param.luma_amm = 0;
				}

				break;

			case VAProcFilterFrameRateConversion:
				frc = (VAProcFilterParameterBufferFrameRateConversion *)base;

				/* check frame rate */
				ratio = frc->output_fps / (float)frc->input_fps;

				if (!((ratio == 2 || ratio == 2.5 || ratio == 4) && frc->output_fps <= 60)) {
					drv_debug_msg(VIDEO_DEBUG_ERROR, "The FRC do NOT support the ration(%f) and fps(%d)\n",
						      ratio, frc->output_fps);
					vaStatus = VA_STATUS_ERROR_UNSUPPORTED_FILTER;
					goto err;
				}

				/* check the chain */
				if (combination_check &&
				    vpp_chain_caps[res_set].frc_enabled != FILTER_ENABLED) {
					drv_debug_msg(VIDEO_DEBUG_ERROR, "The FRC is DISABLE for %d format\n", res_set);
					vaStatus = VA_STATUS_ERROR_INVALID_FILTER_CHAIN;
					goto err;
				}

				break;
			default:
				drv_debug_msg(VIDEO_DEBUG_ERROR, "Do NOT support the filter type %d\n", base->type);
				vaStatus = VA_STATUS_ERROR_UNKNOWN;
				goto err;
			}
		}
	} else {
		drv_debug_msg(VIDEO_DEBUG_ERROR, "no HW support\n");
		vaStatus = VA_STATUS_ERROR_UNKNOWN;
		goto err;
	}
err:
	return vaStatus;
}

static VAStatus vsp_set_pipeline(context_VPP_p ctx)
{
	VAStatus vaStatus = VA_STATUS_SUCCESS;
	vsp_cmdbuf_p cmdbuf = ctx->obj_context->vsp_cmdbuf;
	struct VssProcPipelineParameterBuffer *cell_pipeline_param = (struct VssProcPipelineParameterBuffer *)cmdbuf->pipeline_param_p;
	unsigned int i, j, filter_count, check_filter = 0;
	VAProcFilterParameterBufferBase *cur_param;
	enum VssProcFilterType tmp;
	psb_driver_data_p driver_data = ctx->obj_context->driver_data;

	/* set intermediate buffer */
	cell_pipeline_param->intermediate_buffer_size = VSP_INTERMEDIATE_BUF_SIZE;
	cell_pipeline_param->intermediate_buffer_base = wsbmBOOffsetHint(ctx->intermediate_buf->drm_buf);

	/* init pipeline cmd */
	for (i = 0; i < VssProcPipelineMaxNumFilters; ++i)
		cell_pipeline_param->filter_pipeline[i] = -1;
	cell_pipeline_param->num_filters = 0;

	filter_count = 0;

	/* store filter buffer object */
	if (ctx->num_filters != 0) {
		for (i = 0; i < ctx->num_filters; ++i)
			ctx->filter_buf[i] = BUFFER(ctx->filters[i]);
	} else {
		goto finished;
	}

	/* loop the filter, set correct pipeline param for FW */
	for (i = 0; i < ctx->num_filters; ++i) {
		cur_param = (VAProcFilterParameterBufferBase *)ctx->filter_buf[i]->buffer_data;
		switch (cur_param->type) {
		case VAProcFilterNone:
			goto finished;
			break;
		case VAProcFilterNoiseReduction:
		case VAProcFilterDeblocking:
			cell_pipeline_param->filter_pipeline[filter_count++] = VssProcFilterDenoise;
			check_filter++;
			break;
		case VAProcFilterSharpening:
			cell_pipeline_param->filter_pipeline[filter_count++] = VssProcFilterSharpening;
			break;
		case VAProcFilterColorBalance:
			cell_pipeline_param->filter_pipeline[filter_count++] = VssProcFilterColorEnhancement;
			break;
		case VAProcFilterFrameRateConversion:
			cell_pipeline_param->filter_pipeline[filter_count++] = VssProcFilterFrameRateConversion;
			break;
		default:
			cell_pipeline_param->filter_pipeline[filter_count++] = -1;
			vaStatus = VA_STATUS_ERROR_UNKNOWN;
			goto out;
		}
	}

	/* Denoise and Deblock is alternative */
	if (check_filter >= 2) {
		drv_debug_msg(VIDEO_DEBUG_ERROR, "Denoise and Deblock is alternative!\n");
		cell_pipeline_param->filter_pipeline[filter_count++] = -1;
		vaStatus = VA_STATUS_ERROR_UNKNOWN;
		goto out;
	}

finished:
	cell_pipeline_param->num_filters = filter_count;

	/* reorder */
	for (i = 1; i < filter_count; ++i)
		for (j = i; j > 0; --j)
			if (cell_pipeline_param->filter_pipeline[j] < cell_pipeline_param->filter_pipeline[j - 1]) {
				/* swap */
				tmp = cell_pipeline_param->filter_pipeline[j];
				cell_pipeline_param->filter_pipeline[j] = cell_pipeline_param->filter_pipeline[j - 1];
				cell_pipeline_param->filter_pipeline[j - 1] = tmp;
			}

	vsp_cmdbuf_insert_command(cmdbuf, CONTEXT_VPP_ID, &cmdbuf->param_mem, VssProcPipelineParameterCommand,
				  ctx->pipeline_param_offset, sizeof(struct VssProcPipelineParameterBuffer));
out:
	return vaStatus;
}

static VAStatus vsp_set_filter_param(context_VPP_p ctx)
{
	VAStatus vaStatus = VA_STATUS_SUCCESS;
	vsp_cmdbuf_p cmdbuf = ctx->obj_context->vsp_cmdbuf;
	struct VssProcDenoiseParameterBuffer *cell_denoiser_param = (struct VssProcDenoiseParameterBuffer *)cmdbuf->denoise_param_p;
	struct VssProcColorEnhancementParameterBuffer *cell_enhancer_param = (struct VssProcColorEnhancementParameterBuffer *)cmdbuf->enhancer_param_p;
	struct VssProcSharpenParameterBuffer *cell_sharpen_param = (struct VssProcSharpenParameterBuffer *)cmdbuf->sharpen_param_p;
	struct VssProcFrcParameterBuffer *cell_proc_frc_param = (struct VssProcFrcParameterBuffer *)cmdbuf->frc_param_p;
	VAProcFilterParameterBufferBase *cur_param = NULL;
	VAProcFilterParameterBufferFrameRateConversion *frc_param = NULL;
	unsigned int i;
	float ratio;

	for (i = 0; i < ctx->num_filters; ++i) {
		cur_param = (VAProcFilterParameterBufferBase *)ctx->filter_buf[i]->buffer_data;
		switch (cur_param->type) {
		case VAProcFilterDeblocking:
			memcpy(cell_denoiser_param,
			       &ctx->denoise_deblock_param,
			       sizeof(ctx->denoise_deblock_param));
			cell_denoiser_param->type = VssProcDeblock;

			vsp_cmdbuf_insert_command(cmdbuf,
						  CONTEXT_VPP_ID,
						  &cmdbuf->param_mem,
						  VssProcDenoiseParameterCommand,
						  ctx->denoise_param_offset,
						  sizeof(struct VssProcDenoiseParameterBuffer));
			break;

		case VAProcFilterNoiseReduction:
			memcpy(cell_denoiser_param,
			       &ctx->denoise_deblock_param,
			       sizeof(ctx->denoise_deblock_param));
			cell_denoiser_param->type = VssProcDegrain;

			vsp_cmdbuf_insert_command(cmdbuf,
						  CONTEXT_VPP_ID,
						  &cmdbuf->param_mem,
						  VssProcDenoiseParameterCommand,
						  ctx->denoise_param_offset,
						  sizeof(struct VssProcDenoiseParameterBuffer));
			break;

		case VAProcFilterSharpening:
			memcpy(cell_sharpen_param,
			       &ctx->sharpen_param,
			       sizeof(ctx->sharpen_param));

			vsp_cmdbuf_insert_command(cmdbuf,
						  CONTEXT_VPP_ID,
						  &cmdbuf->param_mem,
						  VssProcSharpenParameterCommand,
						  ctx->sharpen_param_offset,
						  sizeof(struct VssProcSharpenParameterBuffer));
			break;

		case VAProcFilterColorBalance:
			memcpy(cell_enhancer_param,
			       &ctx->enhancer_param,
			       sizeof(ctx->enhancer_param));

			vsp_cmdbuf_insert_command(cmdbuf,
						  CONTEXT_VPP_ID,
						  &cmdbuf->param_mem,
						  VssProcColorEnhancementParameterCommand,
						  ctx->enhancer_param_offset,
						  sizeof(struct VssProcColorEnhancementParameterBuffer));

			break;

		case VAProcFilterFrameRateConversion:
			ctx->frc_buf = ctx->filter_buf[i];

			frc_param = (VAProcFilterParameterBufferFrameRateConversion *)ctx->filter_buf[i]->buffer_data;
			ratio = frc_param->output_fps / (float)frc_param->input_fps;

			/* set the FRC quality */
			/* cell_proc_frc_param->quality = VssFrcMediumQuality; */
			cell_proc_frc_param->quality = VssFrcHighQuality;

			/* check if the input fps is in the range of HW capability */
			if (ratio == 2)
				cell_proc_frc_param->conversion_rate = VssFrc2xConversionRate;
			else if (ratio == 2.5)
				cell_proc_frc_param->conversion_rate = VssFrc2_5xConversionRate;
			else if (ratio == 4)
				cell_proc_frc_param->conversion_rate = VssFrc4xConversionRate;
			else if (ratio == 1.25)
				cell_proc_frc_param->conversion_rate = VssFrc1_25xConversionRate;
			else {
				drv_debug_msg(VIDEO_DEBUG_ERROR, "invalid frame rate conversion ratio %f \n", ratio);
				vaStatus = VA_STATUS_ERROR_UNKNOWN;
				goto out;
			}

			vsp_cmdbuf_insert_command(cmdbuf,
						  CONTEXT_VPP_ID,
						  &cmdbuf->param_mem,
						  VssProcFrcParameterCommand,
						  ctx->frc_param_offset,
						  sizeof(struct VssProcFrcParameterBuffer));
			break;
		default:
			vaStatus = VA_STATUS_ERROR_UNKNOWN;
			goto out;
		}
	}
out:
	return vaStatus;
}

static int check_resolution(int width, int height)
{
	int ret;
	int image_area;

	if (height < MIN_SUPPORTED_HEIGHT || height > MAX_SUPPORTED_HEIGHT)
		return NOT_SUPPORTED_RESOLUTION;

	image_area = height * width;

	if (image_area <= QVGA_AREA)
		ret = QCIF_TO_QVGA;
	else if (image_area <= VGA_AREA)
		ret = QVGA_TO_VGA;
	else if (image_area <= SD_AREA)
		ret = VGA_TO_SD;
	else if (image_area <= HD720P_AREA)
		ret = SD_TO_720P;
	else if (image_area <= HD1080P_AREA)
		ret = HD720P_TO_1080P;
	else
		ret = NOT_SUPPORTED_RESOLUTION;

	return ret;
}

/*
 * The strength area is:
 *
 * 0______33______66______100
 *   LOW     MED     HIGH
 *
 * MIN=0; MAX=100; STEP=33
 */
static int check_vpp_strength(int value)
{
	if (value < MIN_VPP_PARAM || value > MAX_VPP_PARAM)
		return INVALID_STRENGTH;

	if (value >= MIN_VPP_PARAM &&
	    value < MIN_VPP_PARAM + STEP_VPP_PARAM)
		return LOW_STRENGTH;
	else if (value >= MIN_VPP_PARAM + STEP_VPP_PARAM &&
		 value < MIN_VPP_PARAM + 2 * STEP_VPP_PARAM)
		return MEDIUM_STRENGTH;
	else
		return HIGH_STRENGTH;
}
