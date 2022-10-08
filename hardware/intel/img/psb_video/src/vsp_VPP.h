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

#ifndef _VSP_VPP_H_
#define _VSP_VPP_H_

#include "psb_drv_video.h"
#include "vsp_fw.h"

#define CONTEXT_VPP_ID 0
#define CONTEXT_VP8_ID 1
#define CONTEXT_COMPOSE_ID 5

struct context_VPP_s {
	object_context_p obj_context; /* back reference */

	uint32_t profile; // ENTDEC BE_PROFILE & FE_PROFILE
	uint32_t profile_idc; // BE_PROFILEIDC

	struct psb_buffer_s *context_buf;
	struct psb_buffer_s *intermediate_buf;

	VABufferID *filters;
	unsigned int num_filters;

	enum vsp_format format;

	object_buffer_p filter_buf[VssProcPipelineMaxNumFilters];
	object_buffer_p frc_buf;

	unsigned int param_sz;
	unsigned int pic_param_sz;
	unsigned int pic_param_offset;
	unsigned int end_param_sz;
	unsigned int end_param_offset;
	unsigned int pipeline_param_sz;
	unsigned int pipeline_param_offset;
	unsigned int denoise_param_sz;
	unsigned int denoise_param_offset;
	unsigned int enhancer_param_sz;
	unsigned int enhancer_param_offset;
	unsigned int sharpen_param_sz;
	unsigned int sharpen_param_offset;
	unsigned int frc_param_sz;
	unsigned int frc_param_offset;
	unsigned int seq_param_sz;
	unsigned int seq_param_offset;
	unsigned int ref_param_sz;
	unsigned int ref_param_offset;
	unsigned int compose_param_sz;
	unsigned int compose_param_offset;
	struct VssProcDenoiseParameterBuffer denoise_deblock_param;
	struct VssProcColorEnhancementParameterBuffer enhancer_param;
	struct VssProcSharpenParameterBuffer sharpen_param;
	//used for vp8 only
       unsigned int max_frame_size;
       unsigned int vp8_seq_cmd_send;
       unsigned int re_send_seq_params;
       unsigned int temporal_layer_number;
       unsigned int frame_rate[3];
        struct VssVp8encSequenceParameterBuffer vp8_seq_param;
};

typedef struct context_VPP_s *context_VPP_p;

extern struct format_vtable_s vsp_VPP_vtable;

/**
 * Queries video processing filters.
 *
 * This function returns the list of video processing filters supported
 * by the driver. The filters array is allocated by the user and
 * num_filters shall be initialized to the number of allocated
 * elements in that array. Upon successful return, the actual number
 * of filters will be overwritten into num_filters. Otherwise,
 * VA_STATUS_ERROR_MAX_NUM_EXCEEDED is returned and num_filters
 * is adjusted to the number of elements that would be returned if enough
 * space was available.
 *
 * The list of video processing filters supported by the driver shall
 * be ordered in the way they can be iteratively applied. This is needed
 * for both correctness, i.e. some filters would not mean anything if
 * applied at the beginning of the pipeline; but also for performance
 * since some filters can be applied in a single pass (e.g. noise
 * reduction + deinterlacing).
 *
 */
VAStatus vsp_QueryVideoProcFilters(
        VADriverContextP    ctx,
        VAContextID         context,
        VAProcFilterType   *filters,
        unsigned int       *num_filters
	);

/**
 * Queries video filter capabilities.
 *
 * This function returns the list of capabilities supported by the driver
 * for a specific video filter. The filter_caps array is allocated by
 * the user and num_filter_caps shall be initialized to the number
 * of allocated elements in that array. Upon successful return, the
 * actual number of filters will be overwritten into num_filter_caps.
 * Otherwise, VA_STATUS_ERROR_MAX_NUM_EXCEEDED is returned and
 * num_filter_caps is adjusted to the number of elements that would be
 * returned if enough space was available.
 *
 */
VAStatus vsp_QueryVideoProcFilterCaps(
        VADriverContextP    ctx,
        VAContextID         context,
        VAProcFilterType    type,
        void               *filter_caps,
        unsigned int       *num_filter_caps
	);

/**
 * Queries video processing pipeline capabilities.
 *
 * This function returns the video processing pipeline capabilities. The
 * filters array defines the video processing pipeline and is an array
 * of buffers holding filter parameters.
 *
 * Note: the VAProcPipelineCaps structure contains user-provided arrays.
 * If non-NULL, the corresponding num_* fields shall be filled in on
 * input with the number of elements allocated. Upon successful return,
 * the actual number of elements will be overwritten into the num_*
 * fields. Otherwise, VA_STATUS_ERROR_MAX_NUM_EXCEEDED is returned
 * and num_* fields are adjusted to the number of elements that would
 * be returned if enough space was available.
 *
 */
VAStatus vsp_QueryVideoProcPipelineCaps(
	VADriverContextP    ctx,
        VAContextID         context,
        VABufferID         *filters,
        unsigned int        num_filters,
        VAProcPipelineCaps *pipeline_caps
    );

#endif /* _VSS_VPP_H_ */
