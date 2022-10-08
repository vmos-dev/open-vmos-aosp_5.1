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
#include "psb_cmdbuf.h"
#include "psb_surface.h"
#include "tng_yuv_processor.h"
#include "tng_ved_scaling.h"

struct context_DEC_s {
    object_context_p obj_context; /* back reference */

    uint32_t *cmd_params;
    uint32_t *p_range_mapping_base0;
    uint32_t *p_range_mapping_base1;
    uint32_t *p_slice_params; /* pointer to ui32SliceParams in CMD_HEADER */
    uint32_t *slice_first_pic_last;
    uint32_t *alt_output_flags;

    struct psb_buffer_s aux_line_buffer_vld;
    psb_buffer_p preload_buffer;
    psb_buffer_p slice_data_buffer;


    /* Split buffers */
    int split_buffer_pending;

    /* List of VASliceParameterBuffers */
    object_buffer_p *slice_param_list;
    int slice_param_list_size;
    int slice_param_list_idx;

    unsigned int bits_offset;
    unsigned int SR_flags;

    void (*begin_slice)(struct context_DEC_s *, VASliceParameterBufferBase *);
    void (*process_slice)(struct context_DEC_s *, VASliceParameterBufferBase *);
    void (*end_slice)(struct context_DEC_s *);
    VAStatus (*process_buffer)(struct context_DEC_s *, object_buffer_p);

    struct psb_buffer_s *colocated_buffers;
    int colocated_buffers_size;
    int colocated_buffers_idx;
    context_yuv_processor_p yuv_ctx;
#ifdef SLICE_HEADER_PARSING
    uint32_t parse_enabled;
    uint32_t parse_key;
#endif
    /* scaling coeff reg */
    uint32_t scaler_coeff_reg[2][2][4];
    uint32_t h_scaler_ctrl;
    uint32_t v_scaler_ctrl;
};

typedef struct context_DEC_s *context_DEC_p;

void vld_dec_FE_state(object_context_p, psb_buffer_p);
void vld_dec_setup_alternative_frame(object_context_p);
VAStatus vld_dec_process_slice_data(context_DEC_p, object_buffer_p);
VAStatus vld_dec_add_slice_param(context_DEC_p, object_buffer_p);
VAStatus vld_dec_allocate_colocated_buffer(context_DEC_p, object_surface_p, uint32_t);
VAStatus vld_dec_CreateContext(context_DEC_p, object_context_p);
void vld_dec_DestroyContext(context_DEC_p);
psb_buffer_p vld_dec_lookup_colocated_buffer(context_DEC_p, psb_surface_p);
void vld_dec_write_kick(object_context_p);
VAStatus vld_dec_RenderPicture( object_context_p, object_buffer_p *, int);

#define AUX_LINE_BUFFER_VLD_SIZE        (1024*152)

void vld_dec_yuv_rotate(object_context_p);

VAStatus vld_dec_process_slice(context_DEC_p ctx,
                                        void *vld_slice_param,
                                        object_buffer_p obj_buffer);

