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
 *    Waldo Bastian <waldo.bastian@intel.com>
 *
 */

#include "psb_H264.h"
#include "psb_def.h"
#include "psb_surface.h"
#include "psb_cmdbuf.h"
#include "psb_drv_debug.h"

#include "hwdefs/reg_io2.h"
#include "hwdefs/msvdx_offsets.h"
#include "hwdefs/msvdx_cmds_io2.h"
#include "hwdefs/msvdx_core_regs_io2.h"
#include "hwdefs/msvdx_vec_reg_io2.h"
#include "hwdefs/msvdx_vec_h264_reg_io2.h"
#include "hwdefs/dxva_fw_ctrl.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>


#define GET_SURFACE_INFO_is_used(psb_surface) ((int) (psb_surface->extra_info[0]))
#define SET_SURFACE_INFO_is_used(psb_surface, val) psb_surface->extra_info[0] = (uint32_t) val;
#define GET_SURFACE_INFO_col_pic_params(psb_surface) (psb_surface->extra_info[1])
#define SET_SURFACE_INFO_col_pic_params(psb_surface, val) psb_surface->extra_info[1] = val;
#define GET_SURFACE_INFO_dpb_idx(psb_surface) (psb_surface->extra_info[2])
#define SET_SURFACE_INFO_dpb_idx(psb_surface, val) psb_surface->extra_info[2] = val;
#define GET_SURFACE_INFO_colocated_index(psb_surface) ((int) (psb_surface->extra_info[3]))
#define SET_SURFACE_INFO_colocated_index(psb_surface, val) psb_surface->extra_info[3] = (uint32_t) val;
#define IS_USED_AS_REFERENCE(pic_flags)         ( pic_flags & (VA_PICTURE_H264_SHORT_TERM_REFERENCE | VA_PICTURE_H264_LONG_TERM_REFERENCE) )

/* Truncates a signed integer to 17 bits */
#define SIGNTRUNC( x ) ((( (x)  >> 15) & 0x10000) | ( (x) & 0xffff))

#define MSVDX_VEC_REGS_BASE_MTX 0x0800
#define MSVDX_COMMANDS_BASE_MTX 0x1000
#define MSVDX_IQRAM_BASE_MTX    0x700

#define SLICEDATA_BUFFER_TYPE(type) ((type==VASliceDataBufferType)?"VASliceDataBufferType":"VAProtectedSliceDataBufferType")

typedef enum {
    PICT_NONE,
    PICT_FRAME,
    PICT_TOP,
    PICT_BOTTOM,
    PICT_PAIR
} PICTYPE;

typedef enum {
    H264_BASELINE_PROFILE = 0,
    H264_MAIN_PROFILE = 1,
    H264_HIGH_PROFILE = 2
} PROFILE;

const char *profile2str[] = {
    "H264_BASELINE_PROFILE",
    "H264_MAIN_PROFILE",
    "H264_HIGH_PROFILE"
};

typedef enum {
    ST_P,
    ST_B ,
    ST_I ,
    ST_SP ,
    ST_SI
} SLICE_TYPE;

IMG_UINT8 aSliceTypeVAtoMsvdx[] = { 1 , 2 , 0, 1, 0 };

const char *slice2str[] = {
    "ST_P",
    "ST_B",
    "ST_I",
    "ST_SP",
    "ST_SI"
};

struct context_H264_s {
    object_context_p obj_context; /* back reference */

    uint32_t profile; // ENTDEC BE_PROFILE & FE_PROFILE
    uint32_t profile_idc; // BE_PROFILEIDC

    /* Picture parameters */
    VAPictureParameterBufferH264 *pic_params;
    object_surface_p forward_ref_surface;
    object_surface_p backward_ref_surface;

    uint32_t coded_picture_width;    /* in pixels */
    uint32_t coded_picture_height;    /* in pixels */

    uint32_t picture_width_mb;        /* in macroblocks */
    uint32_t picture_height_mb;        /* in macroblocks */
    uint32_t size_mb;                /* in macroblocks */

    uint32_t first_mb_x;
    uint32_t first_mb_y;

    uint32_t mb_width_c;                /* Chroma macroblock width */
    uint32_t mb_height_c;               /* Chroma macroblock height */

    uint32_t bit_depth_l;               /* Luma bit depth */
    uint32_t qp_bd_offset_l;
    uint32_t bit_depth_c;               /* Chroma bit depth */
    uint32_t qp_bd_offset_c;

    uint32_t raw_mb_bits;               /* Number of bits per macroblock */

    uint32_t picture_width_samples_l;
    uint32_t picture_height_samples_l;
    uint32_t picture_width_samples_c;
    uint32_t picture_height_samples_c;

    uint32_t picture_height_map_units;
    uint32_t picture_size_map_units;

    PICTYPE pic_type;
    uint32_t field_type;

    uint32_t long_term_frame_flags;
    uint32_t two_pass_mode;
    uint32_t slice_count;

    /* Registers */
    uint32_t reg_SPS0;
    uint32_t reg_PPS0;
    uint32_t reg_PIC0;

    uint32_t slice0_params;
    uint32_t slice1_params;

    /* Split buffers */
    int split_buffer_pending;

    /* List of VASliceParameterBuffers */
    object_buffer_p *slice_param_list;
    int slice_param_list_size;
    int slice_param_list_idx;

    /* VLC packed data */
    struct psb_buffer_s vlc_packed_table;

    /* Preload buffer */
    struct psb_buffer_s preload_buffer;

    /* Slice Group Map buffer */
    psb_buffer_p slice_group_map_buffer;

    /* IQ matrix */
    VAIQMatrixBufferH264 *iq_matrix;

    /* Reference Cache */
    struct psb_buffer_s reference_cache;

    uint32_t *p_slice_params; /* pointer to ui32SliceParams in CMD_HEADER */
    /* CoLocated buffers - aka ParamMemInfo */
    struct psb_buffer_s *colocated_buffers;
    int colocated_buffers_size;
    int colocated_buffers_idx;
};

typedef struct context_H264_s *context_H264_p;

#define INIT_CONTEXT_H264    context_H264_p ctx = (context_H264_p) obj_context->format_data;

#define SURFACE(id)    ((object_surface_p) object_heap_lookup( &ctx->obj_context->driver_data->surface_heap, id ))

#define CACHE_REF_OFFSET        72
#define CACHE_ROW_OFFSET        4

#define REFERENCE_CACHE_SIZE    (512 * 1024)

#define MAX_PRELOAD_CMDS                                (40*2)
typedef struct {
    IMG_UINT8           ui8Address[MAX_PRELOAD_CMDS]; /* Address = (ui8Address << 1 | 0x400 ) */
    IMG_UINT32          ui32Value[MAX_PRELOAD_CMDS];
} ADDRDATA;

typedef struct {
    IMG_UINT32          ui32ContextId;
    IMG_UINT32          ui32PreloadBufferSize;
    ADDRDATA            aData;
} PRELOAD;



/* **************************************************************************************************************** */
/* Prepacked H264 VLC Tables */
/* **************************************************************************************************************** */
static const IMG_UINT16 ui16H264VLCTableData[] = {
    0x4000, 0x4205, 0x440a, 0x2204, 0x2206, 0x0208, 0x040b, 0x400f,
    0x4204, 0x4209, 0x4013, 0x420e, 0x4217, 0x421b, 0x4212, 0x420d,
    0x4208, 0x2a08, 0x0232, 0x0035, 0x0036, 0x441f, 0x4416, 0x4411,
    0x440c, 0x0407, 0x040e, 0x0415, 0x041c, 0x0223, 0x4a35, 0x3a00,
    0x4420, 0x4426, 0x4421, 0x441c, 0x442b, 0x4422, 0x441d, 0x4418,
    0x4433, 0x442e, 0x4429, 0x4428, 0x442f, 0x442a, 0x4425, 0x4424,
    0x443b, 0x4436, 0x4431, 0x4430, 0x4437, 0x4432, 0x442d, 0x442c,
    0x4443, 0x443e, 0x443d, 0x4438, 0x443f, 0x443a, 0x4439, 0x4434,
    0x4240, 0x4242, 0x4241, 0x423c, 0x4227, 0x421e, 0x4219, 0x4214,
    0x4023, 0x401a, 0x4015, 0x4010, 0x0410, 0x0249, 0x024c, 0x004f,
    0x4613, 0x460f, 0x440a, 0x440a, 0x4205, 0x4205, 0x4205, 0x4205,
    0x4200, 0x4200, 0x4200, 0x4200, 0x2a08, 0x0231, 0x0034, 0x0035,
    0x4423, 0x4416, 0x4415, 0x440c, 0x0407, 0x040e, 0x0415, 0x121c,
    0x0222, 0x4a3f, 0x3a00, 0x442f, 0x4426, 0x4425, 0x4420, 0x442b,
    0x4422, 0x4421, 0x441c, 0x442c, 0x442e, 0x442d, 0x4428, 0x4433,
    0x442a, 0x4429, 0x4424, 0x443b, 0x4436, 0x4435, 0x4434, 0x4437,
    0x4432, 0x4431, 0x4430, 0x0203, 0x423a, 0x4238, 0x423d, 0x423c,
    0x423e, 0x4239, 0x4243, 0x4242, 0x4241, 0x4240, 0x4227, 0x421e,
    0x421d, 0x4218, 0x4014, 0x401a, 0x4019, 0x4010, 0x421f, 0x4212,
    0x4211, 0x4208, 0x421b, 0x420e, 0x420d, 0x4204, 0x4017, 0x4009,
    0x2210, 0x0432, 0x0239, 0x023c, 0x600a, 0x6008, 0x003d, 0x003e,
    0x461f, 0x461b, 0x4617, 0x4613, 0x460f, 0x460a, 0x4605, 0x4600,
    0x0403, 0x040a, 0x0611, 0x4433, 0x442e, 0x4429, 0x4424, 0x442f,
    0x442a, 0x4425, 0x4420, 0x4430, 0x4436, 0x4431, 0x442c, 0x4437,
    0x4432, 0x442d, 0x4428, 0x3600, 0x4640, 0x4643, 0x4642, 0x4641,
    0x463c, 0x463f, 0x463e, 0x463d, 0x4638, 0x463b, 0x463a, 0x4639,
    0x4634, 0x4435, 0x4435, 0x441c, 0x4418, 0x4426, 0x4414, 0x442b,
    0x4422, 0x4421, 0x4410, 0x420c, 0x421e, 0x421d, 0x4208, 0x4227,
    0x421a, 0x4219, 0x4204, 0x400d, 0x4023, 0x400e, 0x4009, 0x2208,
    0x5406, 0x540a, 0x540e, 0x5412, 0x5416, 0x541a, 0x541e, 0x5204,
    0x0002, 0x5002, 0x3000, 0x4000, 0x4005, 0x4200, 0x440a, 0x0401,
    0x1208, 0x000a, 0x4410, 0x440c, 0x4408, 0x440f, 0x4409, 0x4404,
    0x4013, 0x4212, 0x4211, 0x400e, 0x400d, 0x4000, 0x4205, 0x440a,
    0x0404, 0x480f, 0x4a13, 0x2609, 0x441b, 0x4417, 0x4412, 0x440e,
    0x440d, 0x4409, 0x4408, 0x4404, 0x0205, 0x0208, 0x020b, 0x020e,
    0x1411, 0x4216, 0x4211, 0x4210, 0x420c, 0x421f, 0x421a, 0x4215,
    0x4214, 0x4223, 0x421e, 0x4219, 0x4218, 0x4222, 0x4221, 0x421d,
    0x421c, 0x3400, 0x3400, 0x3400, 0x4420, 0x4000, 0x0006, 0x0007,
    0x0008, 0x0009, 0x000a, 0x040b, 0x4002, 0x4001, 0x4004, 0x4003,
    0x4006, 0x4005, 0x4008, 0x4007, 0x400a, 0x4009, 0x3400, 0x440f,
    0x440e, 0x440d, 0x420c, 0x420c, 0x420b, 0x420b, 0x1208, 0x000e,
    0x000f, 0x4404, 0x4403, 0x4402, 0x4401, 0x4400, 0x0203, 0x420a,
    0x4209, 0x420e, 0x420d, 0x420c, 0x420b, 0x4008, 0x4007, 0x4006,
    0x4005, 0x0208, 0x000d, 0x000e, 0x4407, 0x4406, 0x4403, 0x4402,
    0x4401, 0x0004, 0x420c, 0x420a, 0x4209, 0x400d, 0x400b, 0x4008,
    0x4005, 0x4004, 0x4000, 0x0208, 0x000b, 0x000c, 0x4408, 0x4406,
    0x4405, 0x4404, 0x4401, 0x420c, 0x420b, 0x420a, 0x4200, 0x4009,
    0x4007, 0x4003, 0x4002, 0x2208, 0x000a, 0x000b, 0x4407, 0x4406,
    0x4405, 0x4404, 0x4403, 0x400a, 0x4209, 0x420b, 0x4008, 0x4002,
    0x4001, 0x4000, 0x2408, 0x4409, 0x4407, 0x4406, 0x4405, 0x4404,
    0x4403, 0x4402, 0x4008, 0x4201, 0x4400, 0x440a, 0x2408, 0x4408,
    0x4406, 0x4404, 0x4403, 0x4402, 0x4205, 0x4205, 0x4007, 0x4201,
    0x4400, 0x4409, 0x2604, 0x0008, 0x4205, 0x4204, 0x4007, 0x4201,
    0x4402, 0x4600, 0x4608, 0x4006, 0x4003, 0x2604, 0x4206, 0x4204,
    0x4203, 0x4005, 0x4202, 0x4407, 0x4600, 0x4601, 0x2404, 0x4205,
    0x4204, 0x4203, 0x4002, 0x4206, 0x4400, 0x4401, 0x4004, 0x0003,
    0x4402, 0x5000, 0x4003, 0x4005, 0x4003, 0x4202, 0x4404, 0x5000,
    0x4002, 0x4203, 0x5000, 0x5000, 0x4002, 0x4000, 0x4001, 0x4000,
    0x4201, 0x4402, 0x4403, 0x4000, 0x4201, 0x4202, 0x4001, 0x4000,
    0x4001, 0x4000, 0x4000, 0x4201, 0x4202, 0x4203, 0x4202, 0x4201,
    0x4200, 0x0004, 0x4202, 0x4201, 0x4200, 0x4004, 0x4003, 0x0203,
    0x4201, 0x4200, 0x4205, 0x4204, 0x4203, 0x4202, 0x4401, 0x4402,
    0x4404, 0x4403, 0x4406, 0x4405, 0x4200, 0x4200, 0x2a08, 0x4406,
    0x4405, 0x4404, 0x4403, 0x4402, 0x4401, 0x4400, 0x4007, 0x4208,
    0x4409, 0x460a, 0x480b, 0x4a0c, 0x2201, 0x400d, 0x420e, 0x3200,
};

/* Set bottom field flag in bit 7 and DPB index in bits 0:3 */
static uint32_t PICTURE2INDEX(context_H264_p ctx, VAPictureH264 *pic)
{
    uint32_t result = 0xff; /* unused */
    object_surface_p ref_surface = SURFACE(pic->picture_id);
    if (ref_surface) {
        result = GET_SURFACE_INFO_dpb_idx(ref_surface->psb_surface);
    }
    if (pic->flags & VA_PICTURE_H264_BOTTOM_FIELD) {
        result |= 0x80; /* Set bit 7 */
    }
    return result;
}

static void psb_H264_QueryConfigAttributes(
    VAProfile profile,
    VAEntrypoint entrypoint,
    VAConfigAttrib *attrib_list,
    int num_attribs)
{
    /* No H264 specific attributes */
}

static VAStatus psb_H264_ValidateConfig(
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

static VAStatus psb__H264_check_legal_picture(object_context_p obj_context, object_config_p obj_config)
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

    /* MSVDX decode capability for H.264:
     *     BP@L3
     *     MP@L4.1
     *     HP@L4.1
     *
     * Refer to Table A-6 (Maximum frame rates for some example frame sizes) of ISO/IEC 14496-10:2005 (E).
     */
    switch (obj_config->profile) {
    case VAProfileH264Baseline:
        if ((obj_context->picture_width <= 0) || (obj_context->picture_width > 720)
            || (obj_context->picture_height <= 0) || (obj_context->picture_height > 576)) {
            vaStatus = VA_STATUS_ERROR_RESOLUTION_NOT_SUPPORTED;
        }
        break;

    case VAProfileH264Main:
    case VAProfileH264High:
    case VAProfileH264ConstrainedBaseline:
        if ((obj_context->picture_width <= 0) || (obj_context->picture_width > 1920)
            || (obj_context->picture_height <= 0) || (obj_context->picture_height > 1088)) {
            vaStatus = VA_STATUS_ERROR_RESOLUTION_NOT_SUPPORTED;
        }
        break;

    default:
        vaStatus = VA_STATUS_ERROR_UNSUPPORTED_PROFILE;
        break;
    }

    return vaStatus;
}

static void psb_H264_DestroyContext(object_context_p obj_context);

static VAStatus psb_H264_CreateContext(
    object_context_p obj_context,
    object_config_p obj_config)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    context_H264_p ctx;
    /* Validate flag */
    /* Validate picture dimensions */
    vaStatus = psb__H264_check_legal_picture(obj_context, obj_config);
    if (VA_STATUS_SUCCESS != vaStatus) {
        DEBUG_FAILURE;
        return vaStatus;
    }

    ctx = (context_H264_p) calloc(1, sizeof(struct context_H264_s));
    if (NULL == ctx) {
        vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
        DEBUG_FAILURE;
        return vaStatus;
    }

    obj_context->format_data = (void*) ctx;
    ctx->obj_context = obj_context;
    ctx->pic_params = NULL;

    ctx->split_buffer_pending = FALSE;

    ctx->slice_param_list_size = 8;
    ctx->slice_param_list = (object_buffer_p*) calloc(1, sizeof(object_buffer_p) * ctx->slice_param_list_size);
    ctx->slice_param_list_idx = 0;

    if (NULL == ctx->slice_param_list) {
        vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
        DEBUG_FAILURE;
    }

    ctx->colocated_buffers_size = obj_context->num_render_targets;
    ctx->colocated_buffers_idx = 0;
    ctx->colocated_buffers = (psb_buffer_p) calloc(1, sizeof(struct psb_buffer_s) * ctx->colocated_buffers_size);
    if (NULL == ctx->colocated_buffers) {
        vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
        DEBUG_FAILURE;
    }

    switch (obj_config->profile) {
    case VAProfileH264Baseline:
    case VAProfileH264ConstrainedBaseline:
        ctx->profile = H264_BASELINE_PROFILE;
        ctx->profile_idc = 0;
        break;

    case VAProfileH264Main:
        ctx->profile = H264_MAIN_PROFILE;
        ctx->profile_idc = 1;
        break;

    case VAProfileH264High:
        ctx->profile = H264_HIGH_PROFILE;
        ctx->profile_idc = 3;
        break;

    default:
        ASSERT(0);
        vaStatus = VA_STATUS_ERROR_UNKNOWN;
    }


    // TODO
    if (vaStatus == VA_STATUS_SUCCESS) {
        vaStatus = psb_buffer_create(obj_context->driver_data,
                                     sizeof(PRELOAD),
                                     psb_bt_vpu_only,
                                     &ctx->preload_buffer);
        DEBUG_FAILURE;
    }

    if (vaStatus == VA_STATUS_SUCCESS) {
        vaStatus = psb_buffer_create(obj_context->driver_data,
                                     REFERENCE_CACHE_SIZE,
                                     psb_bt_vpu_only,
                                     &ctx->reference_cache);
        DEBUG_FAILURE;
    }

    if (vaStatus == VA_STATUS_SUCCESS) {
        vaStatus = psb_buffer_create(obj_context->driver_data,
                                     sizeof(ui16H264VLCTableData),
                                     psb_bt_cpu_vpu,
                                     &ctx->vlc_packed_table);
        DEBUG_FAILURE;
    }
    if (vaStatus == VA_STATUS_SUCCESS) {
        unsigned char *vlc_packed_data_address;
        if (0 ==  psb_buffer_map(&ctx->vlc_packed_table, &vlc_packed_data_address)) {
            memcpy(vlc_packed_data_address, ui16H264VLCTableData, sizeof(ui16H264VLCTableData));
            psb_buffer_unmap(&ctx->vlc_packed_table);
        } else {
            vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
            DEBUG_FAILURE;
        }
    }
    /* allocate in-loop targets for oold */
    if (obj_context->is_oold) {
        int i;
        for (i = 0; i < obj_context->num_render_targets; i++) {
            object_surface_p obj_surface = SURFACE(obj_context->render_targets[i]);
            psb_surface_p psb_surface;

            if (NULL == obj_surface) {
                vaStatus = VA_STATUS_ERROR_INVALID_SURFACE;
                DEBUG_FAILURE;
                return vaStatus;
            }

            psb_surface = obj_surface->psb_surface;

            psb_surface->in_loop_buf = calloc(1, sizeof(struct psb_buffer_s));
            if (NULL == psb_surface->in_loop_buf) {
                vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
                DEBUG_FAILURE;
                return vaStatus;
            }

            /* FIXME: For RAR surface, need allocate RAR buffer  */
            vaStatus = psb_buffer_create(obj_context->driver_data,
                                         psb_surface->size,
                                         psb_bt_surface,
                                         psb_surface->in_loop_buf);
        }

    }

    if (vaStatus != VA_STATUS_SUCCESS) {
        psb_H264_DestroyContext(obj_context);
    }

    return vaStatus;
}

static void psb_H264_DestroyContext(
    object_context_p obj_context)
{
    INIT_CONTEXT_H264
    int i;

    psb_buffer_destroy(&ctx->reference_cache);
    psb_buffer_destroy(&ctx->preload_buffer);
    psb_buffer_destroy(&ctx->vlc_packed_table);

    if (ctx->pic_params) {
        free(ctx->pic_params);
        ctx->pic_params = NULL;
    }
    if (ctx->iq_matrix) {
        free(ctx->iq_matrix);
        ctx->iq_matrix = NULL;
    }

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

    free(obj_context->format_data);
    obj_context->format_data = NULL;
}

static VAStatus psb__H264_allocate_colocated_buffer(context_H264_p ctx, object_surface_p obj_surface, uint32_t size)
{
    psb_surface_p surface = obj_surface->psb_surface;

    if (!GET_SURFACE_INFO_colocated_index(surface)) {
        VAStatus vaStatus;
        psb_buffer_p buf;
        int index = ctx->colocated_buffers_idx;
        if (index >= ctx->colocated_buffers_size) {
            return VA_STATUS_ERROR_UNKNOWN;
        }
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "psb_H264: Allocating colocated buffer for surface %08x size = %08x\n", surface, size);

        buf = &(ctx->colocated_buffers[index]);
        vaStatus = psb_buffer_create(ctx->obj_context->driver_data, size, psb_bt_vpu_only, buf);
        if (VA_STATUS_SUCCESS != vaStatus) {
            return vaStatus;
        }
        ctx->colocated_buffers_idx++;
        SET_SURFACE_INFO_colocated_index(surface, index + 1); /* 0 means unset, index is offset by 1 */
    }
    return VA_STATUS_SUCCESS;
}

static psb_buffer_p psb__H264_lookup_colocated_buffer(context_H264_p ctx, psb_surface_p surface)
{
    /* drv_debug_msg(VIDEO_DEBUG_GENERAL, "psb_H264: Looking up colocated buffer for surface %08x\n", surface); */
    int index = GET_SURFACE_INFO_colocated_index(surface);
    if (!index) {
        return NULL;
    }

    return &(ctx->colocated_buffers[index-1]); /* 0 means unset, index is offset by 1 */
}

#define P(x)    psb__trace_message("PARAMS: " #x "\t= %08x (%d)\n", p->x, p->x)
static void psb__H264_trace_pic_params(VAPictureParameterBufferH264 *p)
{
    P(CurrPic);
    P(picture_width_in_mbs_minus1);
    P(picture_height_in_mbs_minus1);
    P(bit_depth_luma_minus8);
    P(bit_depth_chroma_minus8);
    P(num_ref_frames);
    P(seq_fields);
    P(num_slice_groups_minus1);
    P(slice_group_map_type);
    P(pic_init_qp_minus26);
    P(chroma_qp_index_offset);
    P(second_chroma_qp_index_offset);
    P(pic_fields);
    P(frame_num);
}

static VAStatus psb__H264_process_picture_param(context_H264_p ctx, object_buffer_p obj_buffer)
{
    psb_surface_p target_surface = ctx->obj_context->current_render_target->psb_surface;
    uint32_t reg_value;
    VAStatus vaStatus;
    psb_driver_data_p driver_data = ctx->obj_context->driver_data;

    ASSERT(obj_buffer->type == VAPictureParameterBufferType);
    ASSERT(obj_buffer->num_elements == 1);
    ASSERT(obj_buffer->size == sizeof(VAPictureParameterBufferH264));
    ASSERT(target_surface);

    if ((obj_buffer->num_elements != 1) ||
        (obj_buffer->size != sizeof(VAPictureParameterBufferH264)) ||
        (NULL == target_surface) ||
        (NULL == obj_buffer->buffer_data)) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "picture parameter buffer is not valid.\n");
        return VA_STATUS_ERROR_UNKNOWN;
    }

    /* Transfer ownership of VAPictureParameterBufferH264 data */
    VAPictureParameterBufferH264 *pic_params = (VAPictureParameterBufferH264 *) obj_buffer->buffer_data;
    if (ctx->pic_params) {
        free(ctx->pic_params);
    }
    ctx->pic_params = pic_params;
    obj_buffer->buffer_data = NULL;
    obj_buffer->size = 0;

    if (psb_video_trace_fp && (psb_video_trace_level & VABUF_TRACE))
        psb__H264_trace_pic_params(pic_params);

    /* Table 6-1 */
    uint32_t sub_width_c  = (pic_params->seq_fields.bits.chroma_format_idc > 2) ? 1 : 2;
    uint32_t sub_height_c = (pic_params->seq_fields.bits.chroma_format_idc > 1) ? 1 : 2;

    if (pic_params->seq_fields.bits.chroma_format_idc == 0) {
        ctx->mb_width_c = 0;
        ctx->mb_height_c = 0;
    } else {
        ctx->mb_width_c = 16 / sub_width_c;             /* 6-1 */
        ctx->mb_height_c = 16 / sub_height_c;           /* 6-2 */
    }

    ctx->bit_depth_l = 8 + pic_params->bit_depth_luma_minus8;                   /* (7-1) */
    ctx->qp_bd_offset_l = 6 * pic_params->bit_depth_luma_minus8;                /* (7-2) */

    ctx->bit_depth_c = 8 + pic_params->bit_depth_chroma_minus8;                 /* (7-3) */
    ctx->qp_bd_offset_c = 6 * (pic_params->bit_depth_chroma_minus8 + pic_params->seq_fields.bits.residual_colour_transform_flag);       /* (7-4) */

    ctx->picture_width_mb = pic_params->picture_width_in_mbs_minus1 + 1;
    ctx->picture_height_mb = pic_params->picture_height_in_mbs_minus1 + 1;

    ctx->size_mb = ctx->picture_width_mb * ctx->picture_height_mb;              /* (7-25) */

    //uint32_t colocated_size = (ctx->picture_width_mb + extra_size) * (ctx->picture_height_mb + extra_size) * 192;
    /*for resolution change feature, need allocat co-located buffer according the size of surface*/
    uint32_t size_mb = ((ctx->obj_context->current_render_target->width + 15) / 16) *
                       ((ctx->obj_context->current_render_target->height + 15) / 16);
    uint32_t colocated_size = ((size_mb + 100) * 128 + 0xfff) & ~0xfff;

    vaStatus = psb__H264_allocate_colocated_buffer(ctx, ctx->obj_context->current_render_target, colocated_size);
    if (VA_STATUS_SUCCESS != vaStatus) {
        DEBUG_FAILURE;
        return vaStatus;
    }

    ctx->raw_mb_bits = 256 * ctx->bit_depth_l + 2 * ctx->mb_width_c * ctx->mb_height_c * ctx->bit_depth_c;      /* (7-5) */

    ctx->picture_width_samples_l = ctx->picture_width_mb * 16;
    ctx->picture_width_samples_c = ctx->picture_width_mb * ctx->mb_width_c;

    ctx->picture_height_samples_l = ctx->picture_height_mb * 16;
    ctx->picture_height_samples_c = ctx->picture_height_mb * ctx->mb_height_c;

    // BECAUSE OF
    //  sps->FrameHeightInMbs   = ( 2 - sps->seq_fields.bits.frame_mbs_only_flag ) * sps->PicHeightInMapUnits;  /* (7-15) */
    ctx->picture_height_map_units = 1 + ctx->picture_height_mb / (2 - pic_params->seq_fields.bits.frame_mbs_only_flag);
    ctx->picture_size_map_units = ctx->picture_width_mb * ctx->picture_height_map_units;/* (7-14) */

    /* record just what type of picture we are */
    if (pic_params->pic_fields.bits.field_pic_flag) {
        if (pic_params->CurrPic.flags & VA_PICTURE_H264_BOTTOM_FIELD) {
            ctx->pic_type = PICT_BOTTOM;
            ctx->field_type = 1;
        } else {
            ctx->pic_type = PICT_TOP;
            ctx->field_type = 0;
        }
    } else {
        ctx->pic_type = PICT_FRAME;
        ctx->field_type = pic_params->seq_fields.bits.mb_adaptive_frame_field_flag ? 3 : 2;
    }

    uint32_t i;
    ctx->long_term_frame_flags = 0;
    /* We go from high to low so that we are left with the lowest index */
    for (i = pic_params->num_ref_frames; i--;) {
        object_surface_p ref_surface = SURFACE(pic_params->ReferenceFrames[i].picture_id);
        if (pic_params->ReferenceFrames[i].flags & VA_PICTURE_H264_BOTTOM_FIELD) {
            ctx->long_term_frame_flags |= 0x01 << i;
        }
        if (ref_surface) {
            SET_SURFACE_INFO_dpb_idx(ref_surface->psb_surface, i);
        }
    }

    /* If the MB are not guarenteed to be consecutive - we must do a 2pass */
    ctx->two_pass_mode = (pic_params->num_slice_groups_minus1 > 0);

    ctx->reg_SPS0 = 0;
    REGIO_WRITE_FIELD_LITE(ctx->reg_SPS0, MSVDX_VEC_H264, CR_VEC_H264_BE_SPS0, H264_BE_SPS0_DEFAULT_MATRIX_FLAG, (ctx->profile == H264_BASELINE_PROFILE));    /* Always use suplied matrix non baseline otherwise use default*/
    REGIO_WRITE_FIELD_LITE(ctx->reg_SPS0, MSVDX_VEC_H264, CR_VEC_H264_BE_SPS0, H264_BE_SPS0_2PASS_FLAG,         ctx->two_pass_mode);                    /* Always 0 for VA - we cant handle otherwise yet */
    /* Assume SGM_8BIT */
    REGIO_WRITE_FIELD_LITE(ctx->reg_SPS0, MSVDX_VEC_H264, CR_VEC_H264_FE_SPS0, H264_FE_SPS0_4BIT_SGM_FLAG,      0);
    REGIO_WRITE_FIELD_LITE(ctx->reg_SPS0, MSVDX_VEC_H264, CR_VEC_H264_BE_SPS0, BE_PROFILEIDC,                   ctx->profile_idc);
    REGIO_WRITE_FIELD_LITE(ctx->reg_SPS0, MSVDX_VEC_H264, CR_VEC_H264_FE_SPS0, MIN_LUMA_BIPRED_SIZE_8X8,        pic_params->seq_fields.bits.MinLumaBiPredSize8x8);
    REGIO_WRITE_FIELD_LITE(ctx->reg_SPS0, MSVDX_VEC_H264, CR_VEC_H264_FE_SPS0, DIRECT_8X8_INFERENCE_FLAG,       pic_params->seq_fields.bits.direct_8x8_inference_flag);
    REGIO_WRITE_FIELD_LITE(ctx->reg_SPS0, MSVDX_VEC_H264, CR_VEC_H264_FE_SPS0, CHROMA_FORMAT_IDC,               pic_params->seq_fields.bits.chroma_format_idc);
    REGIO_WRITE_FIELD_LITE(ctx->reg_SPS0, MSVDX_VEC_H264, CR_VEC_H264_FE_SPS0, FRAME_MBS_ONLY_FLAG,             pic_params->seq_fields.bits.frame_mbs_only_flag);
    REGIO_WRITE_FIELD_LITE(ctx->reg_SPS0, MSVDX_VEC_H264, CR_VEC_H264_FE_SPS0, PICWIDTHINMBSLESS1,              ctx->picture_width_mb - 1);

    ctx->reg_PPS0 = 0;
    REGIO_WRITE_FIELD_LITE(ctx->reg_PPS0, MSVDX_VEC_H264, CR_VEC_H264_FE_PPS0, TRANSFORM_8X8_MODE_FLAG,         pic_params->pic_fields.bits.transform_8x8_mode_flag);
    REGIO_WRITE_FIELD_LITE(ctx->reg_PPS0, MSVDX_VEC_H264, CR_VEC_H264_FE_PPS0, CONSTRAINED_INTRA_PRED_FLAG,     pic_params->pic_fields.bits.constrained_intra_pred_flag);
    REGIO_WRITE_FIELD_LITE(ctx->reg_PPS0, MSVDX_VEC_H264, CR_VEC_H264_FE_PPS0, ENTROPY_CODING_MODE_FLAG,        pic_params->pic_fields.bits.entropy_coding_mode_flag);
    REGIO_WRITE_FIELD_LITE(ctx->reg_PPS0, MSVDX_VEC_H264, CR_VEC_H264_FE_PPS0, NUM_SLICE_GROUPS_MINUS1,         pic_params->num_slice_groups_minus1);
    REGIO_WRITE_FIELD_LITE(ctx->reg_PPS0, MSVDX_VEC_H264, CR_VEC_H264_BE_PPS0, BE_WEIGHTED_BIPRED_IDC,          pic_params->pic_fields.bits.weighted_bipred_idc);
    REGIO_WRITE_FIELD_MASKEDLITE(ctx->reg_PPS0, MSVDX_VEC_H264, CR_VEC_H264_BE_PPS0, BE_CHROMA_QP_INDEX_OFFSET, pic_params->chroma_qp_index_offset);
    REGIO_WRITE_FIELD_MASKEDLITE(ctx->reg_PPS0, MSVDX_VEC_H264, CR_VEC_H264_BE_PPS0, BE_SECOND_CHROMA_QP_INDEX_OFFSET,  pic_params->second_chroma_qp_index_offset);

    uint32_t PicHeightInMbs     = ctx->picture_height_mb >> pic_params->pic_fields.bits.field_pic_flag;         /* (7-23) */
    uint32_t PicSizeInMbs       = ctx->picture_width_mb * PicHeightInMbs;                       /* (7-26) */

    ctx->reg_PIC0 = 0;
    REGIO_WRITE_FIELD_LITE(ctx->reg_PIC0, MSVDX_VEC_H264, CR_VEC_H264_FE_CUR_PIC0, PICSIZEINMBSLESS1,           PicSizeInMbs - 1);
    REGIO_WRITE_FIELD_LITE(ctx->reg_PIC0, MSVDX_VEC_H264, CR_VEC_H264_FE_CUR_PIC0, PICHEIGHTINMBSLESS1,         PicHeightInMbs - 1);
    /* TODO */
    REGIO_WRITE_FIELD_LITE(ctx->reg_PIC0, MSVDX_VEC_H264, CR_VEC_H264_BE_CUR_PIC0, BE_REFERENCE_FLAG,           IS_USED_AS_REFERENCE(pic_params->CurrPic.flags) ? 1 : 0);
    REGIO_WRITE_FIELD_LITE(ctx->reg_PIC0, MSVDX_VEC_H264, CR_VEC_H264_FE_CUR_PIC0, MBAFFFRAMEFLAG,              pic_params->seq_fields.bits.mb_adaptive_frame_field_flag);
    REGIO_WRITE_FIELD_LITE(ctx->reg_PIC0, MSVDX_VEC_H264, CR_VEC_H264_FE_CUR_PIC0, FIELD_PIC_FLAG,              pic_params->pic_fields.bits.field_pic_flag);
    REGIO_WRITE_FIELD_LITE(ctx->reg_PIC0, MSVDX_VEC_H264, CR_VEC_H264_FE_CUR_PIC0, BOTTOM_FIELD_FLAG,           pic_params->CurrPic.flags & VA_PICTURE_H264_BOTTOM_FIELD ? 1 : 0);

    /* Record some info about current picture */
    reg_value = 0;
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_H264, CR_VEC_H264_BE_COL_PIC0, COL_NOTFRAMEFLAG, (PICT_FRAME != ctx->pic_type) ? 1 : 0);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_H264, CR_VEC_H264_BE_COL_PIC0, COL_MBAFFFRAMEFLAG, pic_params->seq_fields.bits.mb_adaptive_frame_field_flag);
    SET_SURFACE_INFO_col_pic_params(target_surface, reg_value);

    if (pic_params->seq_fields.bits.chroma_format_idc == 0) {
        vaStatus = psb_surface_set_chroma(target_surface, 128);
        if (VA_STATUS_SUCCESS != vaStatus) {
            DEBUG_FAILURE;
            return vaStatus;
        }
    }

    if (driver_data->ec_enabled && IS_MRST(driver_data)) {
        psb_context_get_next_cmdbuf(ctx->obj_context);
        psb_context_submit_frame_info(ctx->obj_context, &target_surface->buf,
                                      target_surface->stride, target_surface->size,
                                      ctx->picture_width_mb, ctx->size_mb);
    }

    return VA_STATUS_SUCCESS;
}

static VAStatus psb__H264_process_iq_matrix(context_H264_p ctx, object_buffer_p obj_buffer)
{
    ASSERT(obj_buffer->type == VAIQMatrixBufferType);
    ASSERT(obj_buffer->num_elements == 1);
    ASSERT(obj_buffer->size == sizeof(VAIQMatrixBufferH264));

    if ((obj_buffer->num_elements != 1) ||
        (obj_buffer->size != sizeof(VAIQMatrixBufferH264)) ||
        (NULL == obj_buffer->buffer_data)) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "iq matrix buffer is not valid.\n");
        return VA_STATUS_ERROR_UNKNOWN;
    }

    /* Transfer ownership of VAIQMatrixBufferH264 data */
    if (ctx->iq_matrix) {
        free(ctx->iq_matrix);
    }
    ctx->iq_matrix = (VAIQMatrixBufferH264 *) obj_buffer->buffer_data;
    obj_buffer->buffer_data = NULL;
    obj_buffer->size = 0;

    return VA_STATUS_SUCCESS;
}

static VAStatus psb__H264_process_slice_group_map(context_H264_p ctx, object_buffer_p obj_buffer)
{
    ASSERT(obj_buffer->type == VASliceGroupMapBufferType);
    ASSERT(obj_buffer->num_elements == 1);
//    ASSERT(obj_buffer->size == ...);

    if ((obj_buffer->num_elements != 1) ||
        (NULL == obj_buffer->psb_buffer)) {
        return VA_STATUS_ERROR_UNKNOWN;
    }

    ctx->slice_group_map_buffer = obj_buffer->psb_buffer;

    return VA_STATUS_SUCCESS;
}

#define SCALING_LIST_4x4_SIZE   ((4*4))
#define SCALING_LIST_8x8_SIZE   ((8*8))

static void psb__H264_build_SCA_chunk(context_H264_p ctx)
{
    VAIQMatrixBufferH264 dummy_iq_matrix;
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;

    VAIQMatrixBufferH264 *iq_matrix = ctx->iq_matrix;

    if (!iq_matrix) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "H264: No IQ matrix received for frame. Sending dummy IQ matrix.\n");
        iq_matrix = &dummy_iq_matrix;
        memset(iq_matrix, 0, sizeof(VAIQMatrixBufferH264));
    }

    psb_cmdbuf_rendec_start_chunk(cmdbuf, REG_MSVDX_VEC_IQRAM_OFFSET);

    /* 8x8 Inter Y */
    psb_cmdbuf_rendec_write_block(cmdbuf, iq_matrix->ScalingList8x8[1], SCALING_LIST_8x8_SIZE);

    /* 8x8 Intra Y */
    psb_cmdbuf_rendec_write_block(cmdbuf, iq_matrix->ScalingList8x8[0], SCALING_LIST_8x8_SIZE);

    /* 4x4 Intra Y */
    psb_cmdbuf_rendec_write_block(cmdbuf, iq_matrix->ScalingList4x4[0], SCALING_LIST_4x4_SIZE);

    /* 4x4 Inter Y */
    psb_cmdbuf_rendec_write_block(cmdbuf, iq_matrix->ScalingList4x4[3], SCALING_LIST_4x4_SIZE);

    /* 4x4 Inter Cb */
    psb_cmdbuf_rendec_write_block(cmdbuf, iq_matrix->ScalingList4x4[4], SCALING_LIST_4x4_SIZE);

    /* 4x4 Intra Cb */
    psb_cmdbuf_rendec_write_block(cmdbuf, iq_matrix->ScalingList4x4[1], SCALING_LIST_4x4_SIZE);

    /* 4x4 Inter Cr */
    psb_cmdbuf_rendec_write_block(cmdbuf, iq_matrix->ScalingList4x4[5], SCALING_LIST_4x4_SIZE);

    /* 4x4 Intra Cr */
    psb_cmdbuf_rendec_write_block(cmdbuf, iq_matrix->ScalingList4x4[2], SCALING_LIST_4x4_SIZE);

    psb_cmdbuf_rendec_end_chunk(cmdbuf);
}

static void psb__H264_build_picture_order_chunk(context_H264_p ctx)
{
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    VAPictureParameterBufferH264 *pic_params = ctx->pic_params;
    uint32_t reg_value;
    int i;

    /* CHUNK: POC */
    /* send Picture Order Counts (b frame only?) */
    /* maybe need a state variable to track if this has already been sent for the frame */
    psb_cmdbuf_rendec_start_chunk(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_VEC, H264_CR_VEC_H264_BE_FOC0));

    reg_value = 0;
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_H264, CR_VEC_H264_BE_FOC0, TOPFIELDORDERCNT_CURR,
                           SIGNTRUNC(pic_params->CurrPic.TopFieldOrderCnt));
    psb_cmdbuf_rendec_write(cmdbuf, reg_value);

    reg_value = 0;
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_H264, CR_VEC_H264_BE_FOC1, BOTTOMFIELDORDERCNT_CURR,
                           SIGNTRUNC(pic_params->CurrPic.BottomFieldOrderCnt));
    psb_cmdbuf_rendec_write(cmdbuf, reg_value);

    if (pic_params->num_ref_frames > 16) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Invalid reference number %d, set to 16\n", pic_params->num_ref_frames);
        pic_params->num_ref_frames = 16;
    }

    for (i = 0; i < pic_params->num_ref_frames; i++) {
        reg_value = 0;
        REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_H264, CR_VEC_H264_BE_TOP_FOC, TOPFIELDORDERCNT,
                               SIGNTRUNC(pic_params->ReferenceFrames[i].TopFieldOrderCnt));
        psb_cmdbuf_rendec_write(cmdbuf, reg_value);

        reg_value = 0;
        REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_H264, CR_VEC_H264_BE_BOT_FOC, BOTTOMFIELDORDERCNT,
                               SIGNTRUNC(pic_params->ReferenceFrames[i].BottomFieldOrderCnt));
        psb_cmdbuf_rendec_write(cmdbuf, reg_value);
    }

    psb_cmdbuf_rendec_end_chunk(cmdbuf);
}

static void psb__H264_build_B_slice_chunk(context_H264_p ctx, VASliceParameterBufferH264 *slice_param)
{
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    VAPictureParameterBufferH264 *pic_params = ctx->pic_params;
    uint32_t reg_value;
    int i;

    psb_cmdbuf_rendec_start_chunk(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_VEC, H264_CR_VEC_H264_BE_COL_PIC0));

    /* Colocated picture is picture 0 in list 1*/
    object_surface_p colocated_surface = SURFACE(slice_param->RefPicList1[0].picture_id);
    if (colocated_surface) {
        uint32_t bottom_field_flag;
        if (pic_params->pic_fields.bits.field_pic_flag) {
            bottom_field_flag  = (slice_param->RefPicList1[0].flags & VA_PICTURE_H264_BOTTOM_FIELD) ? 1 : 0;
        } else {
            /* when current pic is a frame col bottom field flag is different */
            IMG_INT32   i32Cur;
            IMG_INT32   i32Top, i32Bot;
            IMG_INT32   i32TopAbsDiffPoc, i32BotAbsDiffPoc;

            /* current pic */
            i32Top = pic_params->CurrPic.TopFieldOrderCnt;
            i32Bot = pic_params->CurrPic.BottomFieldOrderCnt;
            i32Cur = (i32Top < i32Bot) ? i32Top : i32Bot;

            /* col pic */
            i32Top = slice_param->RefPicList1[0].TopFieldOrderCnt;
            i32Bot = slice_param->RefPicList1[0].BottomFieldOrderCnt;

            i32TopAbsDiffPoc = (i32Cur < i32Top) ? i32Top - i32Cur : i32Cur - i32Top;
            i32BotAbsDiffPoc = (i32Cur < i32Bot) ? i32Bot - i32Cur : i32Cur - i32Bot;

            bottom_field_flag = (i32TopAbsDiffPoc < i32BotAbsDiffPoc) ? 0 : 1;
        }

        reg_value = GET_SURFACE_INFO_col_pic_params(colocated_surface->psb_surface);
        REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_H264, CR_VEC_H264_BE_COL_PIC0, COL_BOTTOM_FIELD_FLAG, bottom_field_flag);
        psb_cmdbuf_rendec_write(cmdbuf, reg_value);

        psb_buffer_p colocated_target_buffer = psb__H264_lookup_colocated_buffer(ctx, colocated_surface->psb_surface);
        ASSERT(colocated_target_buffer);
        if (colocated_target_buffer) {
            psb_cmdbuf_rendec_write_address(cmdbuf, colocated_target_buffer, 0);
        } else {
            /* This is an error */
            psb_cmdbuf_rendec_write(cmdbuf, 0);
        }
    } else {
        /* Need some better error handling here */
        psb_cmdbuf_rendec_write(cmdbuf, 0);
        psb_cmdbuf_rendec_write(cmdbuf, 0xDEADBEEF);
    }

    /* Calculate inverse index for reference pictures */
    {
        IMG_UINT8 list0_inverse[32];
        memset(list0_inverse, 0xff, 32); /* Unused entries get 0xff */

        if (slice_param->num_ref_idx_l0_active_minus1 + 1 > 32) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "num_ref_idx_l0_active_minus1(%d) is too big. Set it with 31\n",
                               slice_param->num_ref_idx_l0_active_minus1);
            slice_param->num_ref_idx_l0_active_minus1 = 31;
        }

        if (slice_param->num_ref_idx_l0_active_minus1 > 30)
            slice_param->num_ref_idx_l0_active_minus1 = 30;
        for (i = slice_param->num_ref_idx_l0_active_minus1 + 1; i--;) {
            object_surface_p surface = SURFACE(slice_param->RefPicList0[i].picture_id);
            if (surface) {
                uint32_t dpb_idx = GET_SURFACE_INFO_dpb_idx(surface->psb_surface);
                if (dpb_idx < 16) {
                    if (slice_param->RefPicList0[i].flags & VA_PICTURE_H264_BOTTOM_FIELD) {
                        dpb_idx |= 0x10;
                    }
                    list0_inverse[dpb_idx] = i;
                }
            }
        }
        for (i = 0; i < 32; i += 4) {
            reg_value = 0;
            reg_value |= list0_inverse[i];
            reg_value |= list0_inverse[i+1] << 8;
            reg_value |= list0_inverse[i+2] << 16;
            reg_value |= list0_inverse[i+3] << 24;
            psb_cmdbuf_rendec_write(cmdbuf, reg_value);
        }
    }

    if (slice_param->num_ref_idx_l1_active_minus1 > 28)
        slice_param->num_ref_idx_l1_active_minus1 = 28;

    /* Write Ref List 1 - but only need the valid ones */
    for (i = 0; i <= slice_param->num_ref_idx_l1_active_minus1; i += 4) {
        reg_value = 0;
        reg_value |= PICTURE2INDEX(ctx, &slice_param->RefPicList1[i]);
        reg_value |= PICTURE2INDEX(ctx, &slice_param->RefPicList1[i+1]) << 8;
        reg_value |= PICTURE2INDEX(ctx, &slice_param->RefPicList1[i+2]) << 16;
        reg_value |= PICTURE2INDEX(ctx, &slice_param->RefPicList1[i+3]) << 24;
        psb_cmdbuf_rendec_write(cmdbuf, reg_value);
    }

    psb_cmdbuf_rendec_end_chunk(cmdbuf);
}

static void psb__H264_build_register(context_H264_p ctx, VASliceParameterBufferH264 *slice_param)
{
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    uint32_t reg_value;

    psb_cmdbuf_reg_start_block(cmdbuf, 0);

    reg_value = 0;
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC, CR_VEC_ENTDEC_FE_CONTROL, ENTDEC_FE_PROFILE, ctx->profile);
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC, CR_VEC_ENTDEC_FE_CONTROL, ENTDEC_FE_MODE, 1); /* 1 - H.264 */
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC, CR_VEC_ENTDEC_FE_CONTROL), reg_value);

    /* write the FE registers */
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC_H264, CR_VEC_H264_FE_SPS0),    ctx->reg_SPS0);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC_H264, CR_VEC_H264_FE_PPS0),    ctx->reg_PPS0);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC_H264, CR_VEC_H264_FE_CUR_PIC0),        ctx->reg_PIC0);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC_H264, CR_VEC_H264_FE_SLICE0),  ctx->slice0_params);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC_H264, CR_VEC_H264_FE_SLICE1),  ctx->slice1_params);

    reg_value = 0;
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_H264, CR_VEC_H264_FE_SLICE2, FIRST_MB_IN_SLICE, slice_param->first_mb_in_slice);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC_H264, CR_VEC_H264_FE_SLICE2), reg_value);

    if (ctx->pic_params->num_slice_groups_minus1 >= 1) {
        ASSERT(ctx->slice_group_map_buffer);
        if (ctx->slice_group_map_buffer) {
            psb_cmdbuf_reg_set_address(cmdbuf, REGISTER_OFFSET(MSVDX_VEC_H264, CR_VEC_H264_FE_BASE_ADDR_SGM),
                                       ctx->slice_group_map_buffer, 0);
        }
    }
    psb_cmdbuf_reg_end_block(cmdbuf);
}

/* Programme the Alt output if there is a rotation*/
static void psb__H264_setup_alternative_frame(context_H264_p ctx)
{
    uint32_t cmd;
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    psb_surface_p rotate_surface = ctx->obj_context->current_render_target->out_loop_surface;
    object_context_p obj_context = ctx->obj_context;

    if (GET_SURFACE_INFO_rotate(rotate_surface) != obj_context->msvdx_rotate)
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Display rotate mode does not match surface rotate mode!\n");


    /* CRendecBlock    RendecBlk( mCtrlAlloc , RENDEC_REGISTER_OFFSET(MSVDX_CMDS, VC1_LUMA_RANGE_MAPPING_BASE_ADDRESS) ); */
    psb_cmdbuf_rendec_start_chunk(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, VC1_LUMA_RANGE_MAPPING_BASE_ADDRESS));

    psb_cmdbuf_rendec_write_address(cmdbuf, &rotate_surface->buf, rotate_surface->buf.buffer_ofs);
    psb_cmdbuf_rendec_write_address(cmdbuf, &rotate_surface->buf, rotate_surface->buf.buffer_ofs + rotate_surface->chroma_offset);

    psb_cmdbuf_rendec_end_chunk(cmdbuf);

    /* Set the rotation registers */
    psb_cmdbuf_rendec_start_chunk(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, ALTERNATIVE_OUTPUT_PICTURE_ROTATION));
    cmd = 0;
    REGIO_WRITE_FIELD_LITE(cmd, MSVDX_CMDS, ALTERNATIVE_OUTPUT_PICTURE_ROTATION , ALT_PICTURE_ENABLE, 1);
    REGIO_WRITE_FIELD_LITE(cmd, MSVDX_CMDS, ALTERNATIVE_OUTPUT_PICTURE_ROTATION , ROTATION_ROW_STRIDE, rotate_surface->stride_mode);
    REGIO_WRITE_FIELD_LITE(cmd, MSVDX_CMDS, ALTERNATIVE_OUTPUT_PICTURE_ROTATION , RECON_WRITE_DISABLE, 0); /* FIXME Always generate Rec */
    REGIO_WRITE_FIELD_LITE(cmd, MSVDX_CMDS, ALTERNATIVE_OUTPUT_PICTURE_ROTATION , ROTATION_MODE, GET_SURFACE_INFO_rotate(rotate_surface));

    psb_cmdbuf_rendec_write(cmdbuf, cmd);

    psb_cmdbuf_rendec_end_chunk(cmdbuf);
}


static void psb__H264_build_rendec_params(context_H264_p ctx, VASliceParameterBufferH264 *slice_param)
{
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    psb_surface_p target_surface = ctx->obj_context->current_render_target->psb_surface;
    VAPictureParameterBufferH264 *pic_params = ctx->pic_params;
    uint32_t reg_value;
    unsigned int i;

    psb_cmdbuf_rendec_start_block(cmdbuf);

    /* CHUNK: Entdec back-end profile and level */
    {
        psb_cmdbuf_rendec_start_chunk(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_VEC, CR_VEC_ENTDEC_BE_CONTROL));

        reg_value = 0;
        REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC, CR_VEC_ENTDEC_BE_CONTROL, ENTDEC_BE_PROFILE, ctx->profile);
        REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC, CR_VEC_ENTDEC_BE_CONTROL, ENTDEC_BE_MODE, 1); /* 1 - H.264 */
        psb_cmdbuf_rendec_write(cmdbuf, reg_value);

        psb_cmdbuf_rendec_end_chunk(cmdbuf);
    }

    /* CHUNK: SEQ Registers */
    /* send Slice Data for every slice */
    /* MUST be the last slice sent */
    psb_cmdbuf_rendec_start_chunk(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_VEC, H264_CR_VEC_H264_BE_SPS0));

    psb_cmdbuf_rendec_write(cmdbuf, ctx->reg_SPS0);
    psb_cmdbuf_rendec_write(cmdbuf, ctx->reg_PPS0);
    psb_cmdbuf_rendec_write(cmdbuf, ctx->reg_PIC0);
    psb_cmdbuf_rendec_write(cmdbuf, ctx->slice0_params);
    psb_cmdbuf_rendec_write(cmdbuf, ctx->slice1_params);

    /* 5.5.75. VEC_H264_BE_BASE_ADDR_CUR_PIC */
    psb_buffer_p colocated_target_buffer = psb__H264_lookup_colocated_buffer(ctx, target_surface);
    ASSERT(colocated_target_buffer);
    if (colocated_target_buffer) {
        psb_cmdbuf_rendec_write_address(cmdbuf, colocated_target_buffer, colocated_target_buffer->buffer_ofs);
    } else {
        /* This is an error */
        psb_cmdbuf_rendec_write(cmdbuf, 0);
    }

    reg_value = 0;
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_H264, CR_VEC_H264_BE_REF0, BE_LONGTERMFRAMEFLAG, ctx->long_term_frame_flags);
    psb_cmdbuf_rendec_write(cmdbuf, reg_value);

    psb_cmdbuf_rendec_end_chunk(cmdbuf);

    //#warning "TODO: MUST be done after fe slice1 (which gives MB address) "
    /*          REGIO_WRITE_REGISTER(0, MSVDX_VEC_H264, CR_VEC_H264_FE_BASE_ADDR_SGM, gui32SliceGroupType6BaseAddressHack); */

    /* CHUNK: SCA */
    /* send Scaling Lists in High Profile for first slice*/
    if (ctx->profile == H264_HIGH_PROFILE) {
        psb__H264_build_SCA_chunk(ctx);
    }

    /* CHUNK: POC */
    /* send Picture Order Counts (b frame only?) */
    /* maybe need a state variable to track if this has already been sent for the frame */
    if (slice_param->slice_type == ST_B) {
        psb__H264_build_picture_order_chunk(ctx);
    }

    /* CHUNK: BIN */
    /* send B-slice information for B-slices */
    if (slice_param->slice_type == ST_B) {
        psb__H264_build_B_slice_chunk(ctx, slice_param);
    }

    /* CHUNK: PIN */
    /* send P+B-slice information for P and B slices */
    if (slice_param->slice_type == ST_B ||  slice_param->slice_type == ST_P) {
        psb_cmdbuf_rendec_start_chunk(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_VEC, H264_CR_VEC_H264_BE_LIST0));

        if (slice_param->num_ref_idx_l0_active_minus1 > 31) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "num_ref_idx_l0_active_minus1(%d) is too big, limit it to 31.\n",
                               slice_param->num_ref_idx_l0_active_minus1);
            slice_param->num_ref_idx_l0_active_minus1 = 28;
        }

        for (i = 0; i <= slice_param->num_ref_idx_l0_active_minus1; i += 4) {
            reg_value = 0;
            reg_value |= PICTURE2INDEX(ctx, &slice_param->RefPicList0[i]);
            reg_value |= PICTURE2INDEX(ctx, &slice_param->RefPicList0[i+1]) << 8;
            reg_value |= PICTURE2INDEX(ctx, &slice_param->RefPicList0[i+2]) << 16;
            reg_value |= PICTURE2INDEX(ctx, &slice_param->RefPicList0[i+3]) << 24;
            psb_cmdbuf_rendec_write(cmdbuf, reg_value);
        }

        psb_cmdbuf_rendec_end_chunk(cmdbuf);
    }

    /* CHUNK: DPB */
    /* send DPB information (for P and B slices?) only needed once per frame */
//      if ( sh->slice_type == ST_B || sh->slice_type == ST_P )
    if (pic_params->num_ref_frames > 0) {
        IMG_BOOL is_used[16];
        psb_cmdbuf_rendec_start_chunk(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, REFERENCE_PICTURE_BASE_ADDRESSES));

        /* Mark all surfaces as unused */
        memset(is_used, 0, sizeof(is_used));

        if (slice_param->num_ref_idx_l0_active_minus1 > 31)
            slice_param->num_ref_idx_l0_active_minus1 = 31;
        /* Mark onlys frame that are actualy used */
        for (i = 0; i <= slice_param->num_ref_idx_l0_active_minus1; i++) {
            object_surface_p ref_surface = SURFACE(slice_param->RefPicList0[i].picture_id);
            if (ref_surface) {
                uint32_t idx = GET_SURFACE_INFO_dpb_idx(ref_surface->psb_surface);
                if (idx < 16) {
                    is_used[idx] = IMG_TRUE;
                }
            }
        }

        if (slice_param->num_ref_idx_l1_active_minus1 > 31)
            slice_param->num_ref_idx_l1_active_minus1 = 31;

        /* Mark onlys frame that are actualy used */
        for (i = 0; i <= slice_param->num_ref_idx_l1_active_minus1; i++) {
            object_surface_p ref_surface = SURFACE(slice_param->RefPicList1[i].picture_id);
            if (ref_surface) {
                uint32_t idx = GET_SURFACE_INFO_dpb_idx(ref_surface->psb_surface);
                if (idx < 16) {
                    is_used[idx] = IMG_TRUE;
                }
            }
        }

        if (pic_params->num_ref_frames > 16)
            pic_params->num_ref_frames = 16;
        /* Only load used surfaces */
        for (i = 0; i < pic_params->num_ref_frames; i++) {
            object_surface_p ref_surface = SURFACE(pic_params->ReferenceFrames[i].picture_id);
            psb_buffer_p buffer;

            if (NULL == ref_surface) {
                drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s L%d Invalide reference surface handle, but still continue\n",
                                         __FUNCTION__, __LINE__);
                /* return; */
            }
            /*
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "pic_params->ReferenceFrames[%d] = %08x --> %08x frame_idx:0x%08x flags:%02x TopFieldOrderCnt: 0x%08x BottomFieldOrderCnt: 0x%08x %s\n",
                                     i,
                                     pic_params->ReferenceFrames[i].picture_id,
                                     ref_surface,
                                     pic_params->ReferenceFrames[i].frame_idx,
                                     pic_params->ReferenceFrames[i].flags,
                                     pic_params->ReferenceFrames[i].TopFieldOrderCnt,
                                     pic_params->ReferenceFrames[i].BottomFieldOrderCnt,
                                     is_used[i] ? "used" : "");
            */
            if (ref_surface && is_used[i] && ref_surface->psb_surface->ref_buf)
                // GET_SURFACE_INFO_is_used(ref_surface->psb_surface))
            {
                buffer = ref_surface->psb_surface->ref_buf;
                psb_cmdbuf_rendec_write_address(cmdbuf, buffer,
                                                buffer->buffer_ofs);
                psb_cmdbuf_rendec_write_address(cmdbuf, buffer,
                                                buffer->buffer_ofs +
                                                ref_surface->psb_surface->chroma_offset);
            } else {
                psb_cmdbuf_rendec_write(cmdbuf, 0xdeadbeef);
                psb_cmdbuf_rendec_write(cmdbuf, 0xdeadbeef);
            }
        }
        psb_cmdbuf_rendec_end_chunk(cmdbuf);
    }

    /* CHUNK: MVA and MVB */
    /* works as long as weighted factors A and B commands remain the same */
    if ((pic_params->pic_fields.bits.weighted_pred_flag && (slice_param->slice_type == ST_P)) ||
        ((pic_params->pic_fields.bits.weighted_bipred_idc != 0) && (slice_param->slice_type == ST_B))) {
        IMG_UINT32 num_ref_0 = slice_param->num_ref_idx_l0_active_minus1;

        psb_cmdbuf_rendec_start_chunk(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, H264_WEIGHTED_FACTORS_A));

        if (num_ref_0 > 31)
            num_ref_0 = 31;

        /* weighted factors */
        for (i = 0; i <= num_ref_0; i++) {
            reg_value = 0;
            REGIO_WRITE_FIELD_MASKEDLITE(reg_value, MSVDX_CMDS, H264_WEIGHTED_FACTORS_A, CR_WEIGHT_A,   slice_param->chroma_weight_l0[i][1]);/* Cr - 1 */
            REGIO_WRITE_FIELD_MASKEDLITE(reg_value, MSVDX_CMDS, H264_WEIGHTED_FACTORS_A, CB_WEIGHT_A,   slice_param->chroma_weight_l0[i][0]);/* Cb - 0 */
            REGIO_WRITE_FIELD_MASKEDLITE(reg_value, MSVDX_CMDS, H264_WEIGHTED_FACTORS_A, Y_WEIGHT_A,    slice_param->luma_weight_l0[i]);

            psb_cmdbuf_rendec_write(cmdbuf, reg_value);
        }
        /* pad remainder */
        for (; i < 32; i++) {
            psb_cmdbuf_rendec_write(cmdbuf, 0);
        }

        /* weighted offsets */
        for (i = 0; i <= num_ref_0; i++) {
            reg_value = 0;
            REGIO_WRITE_FIELD_MASKEDLITE(reg_value, MSVDX_CMDS, H264_WEIGHTED_OFFSET_A, CR_OFFSET_A,    slice_param->chroma_offset_l0[i][1]);/* Cr - 1 */
            REGIO_WRITE_FIELD_MASKEDLITE(reg_value, MSVDX_CMDS, H264_WEIGHTED_OFFSET_A, CB_OFFSET_A,    slice_param->chroma_offset_l0[i][0]);/* Cb - 0 */
            REGIO_WRITE_FIELD_MASKEDLITE(reg_value, MSVDX_CMDS, H264_WEIGHTED_OFFSET_A, Y_OFFSET_A,     slice_param->luma_offset_l0[i]);

            psb_cmdbuf_rendec_write(cmdbuf, reg_value);
        }
        /* pad remainder */
        for (; i < 32; i++) {
            psb_cmdbuf_rendec_write(cmdbuf, 0);
        }
        psb_cmdbuf_rendec_end_chunk(cmdbuf);

        if (slice_param->slice_type == ST_B) {
            IMG_UINT32 num_ref_1 = slice_param->num_ref_idx_l1_active_minus1;

            psb_cmdbuf_rendec_start_chunk(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, H264_WEIGHTED_FACTORS_B));

            if (num_ref_1 > 31) {
                drv_debug_msg(VIDEO_DEBUG_ERROR, "num_ref_1 shouldn't be larger than 31\n");
                num_ref_1 = 31;
            }

            /* weighted factors */
            for (i = 0; i <= num_ref_1; i++) {
                reg_value = 0;
                REGIO_WRITE_FIELD_MASKEDLITE(reg_value, MSVDX_CMDS, H264_WEIGHTED_FACTORS_B, CR_WEIGHT_B,       slice_param->chroma_weight_l1[i][1]);/* Cr - 1 */
                REGIO_WRITE_FIELD_MASKEDLITE(reg_value, MSVDX_CMDS, H264_WEIGHTED_FACTORS_B, CB_WEIGHT_B,       slice_param->chroma_weight_l1[i][0]);/* Cb - 0 */
                REGIO_WRITE_FIELD_MASKEDLITE(reg_value, MSVDX_CMDS, H264_WEIGHTED_FACTORS_B, Y_WEIGHT_B,        slice_param->luma_weight_l1[i]);

                psb_cmdbuf_rendec_write(cmdbuf, reg_value);
            }
            /* pad remainder */
            for (; i < 32; i++) {
                psb_cmdbuf_rendec_write(cmdbuf, 0);
            }

            /* weighted offsets */
            for (i = 0; i <= num_ref_1; i++) {
                reg_value = 0;
                REGIO_WRITE_FIELD_MASKEDLITE(reg_value, MSVDX_CMDS, H264_WEIGHTED_OFFSET_B, CR_OFFSET_B,        slice_param->chroma_offset_l1[i][1]);/* Cr - 1 */
                REGIO_WRITE_FIELD_MASKEDLITE(reg_value, MSVDX_CMDS, H264_WEIGHTED_OFFSET_B, CB_OFFSET_B,        slice_param->chroma_offset_l1[i][0]);/* Cb - 0 */
                REGIO_WRITE_FIELD_MASKEDLITE(reg_value, MSVDX_CMDS, H264_WEIGHTED_OFFSET_B, Y_OFFSET_B, slice_param->luma_offset_l1[i]);

                psb_cmdbuf_rendec_write(cmdbuf, reg_value);
            }
            /* pad remainder */
            for (; i < 32; i++) {
                psb_cmdbuf_rendec_write(cmdbuf, 0);
            }
            psb_cmdbuf_rendec_end_chunk(cmdbuf);
        }
    }

    /*          If this a two pass mode deblock, then we will perform the rotation as part of the
     *          2nd pass deblock procedure
     */
    if (/*!ctx->two_pass_mode &&*/ CONTEXT_ROTATE(ctx->obj_context)) /* FIXME field coded should not issue */
        psb__H264_setup_alternative_frame(ctx);

    /* CHUNK: SEQ Commands 1 */
    /* send Slice Data for every slice */
    /* MUST be the last slice sent */
    psb_cmdbuf_rendec_start_chunk(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, DISPLAY_PICTURE_SIZE));

    reg_value = 0;
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, DISPLAY_PICTURE_SIZE, DISPLAY_PICTURE_HEIGHT, (ctx->picture_height_mb * 16) - 1);
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, DISPLAY_PICTURE_SIZE, DISPLAY_PICTURE_WIDTH, (ctx->picture_width_mb * 16) - 1);
    psb_cmdbuf_rendec_write(cmdbuf, reg_value);

    reg_value = 0;
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, CODED_PICTURE_SIZE, CODED_PICTURE_HEIGHT, (ctx->picture_height_mb * 16) - 1);
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, CODED_PICTURE_SIZE, CODED_PICTURE_WIDTH, (ctx->picture_width_mb * 16) - 1);
    psb_cmdbuf_rendec_write(cmdbuf, reg_value);

    reg_value = 0;
    // TODO: Must check how these should be set
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, OPERATING_MODE, CHROMA_INTERLEAVED, 0);
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, OPERATING_MODE, ROW_STRIDE, target_surface->stride_mode);
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, OPERATING_MODE, CODEC_PROFILE, ctx->profile);
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, OPERATING_MODE, CODEC_MODE, 1);       /* H.264 */
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, OPERATING_MODE, ASYNC_MODE, ctx->two_pass_mode);
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, OPERATING_MODE, CHROMA_FORMAT, pic_params->seq_fields.bits.chroma_format_idc);
    psb_cmdbuf_rendec_write(cmdbuf, reg_value);

    ctx->obj_context->operating_mode = reg_value;

    if (ctx->obj_context->is_oold && !ctx->two_pass_mode) { /* Need to mark which buf is to be used as ref*/
        /* LUMA_RECONSTRUCTED_PICTURE_BASE_ADDRESSES */
        psb_cmdbuf_rendec_write_address(cmdbuf, target_surface->in_loop_buf, target_surface->in_loop_buf->buffer_ofs);

        /* CHROMA_RECONSTRUCTED_PICTURE_BASE_ADDRESSES */
        psb_cmdbuf_rendec_write_address(cmdbuf, target_surface->in_loop_buf, target_surface->in_loop_buf->buffer_ofs + target_surface->chroma_offset);
        target_surface->ref_buf = target_surface->in_loop_buf;
    } else {
        psb_cmdbuf_rendec_write_address(cmdbuf, &target_surface->buf, target_surface->buf.buffer_ofs);
        psb_cmdbuf_rendec_write_address(cmdbuf, &target_surface->buf, target_surface->buf.buffer_ofs + target_surface->chroma_offset);
        target_surface->ref_buf = &target_surface->buf;
    }
    /* Aux Msb Buffer base address: H.264 does not use this command */
    reg_value = 0;
    psb_cmdbuf_rendec_write(cmdbuf, reg_value);

    /* Intra Reference Cache */
    psb_cmdbuf_rendec_write_address(cmdbuf, &ctx->reference_cache, 0);

    reg_value = 0;
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, MC_CACHE_CONFIGURATION, CONFIG_REF_OFFSET, CACHE_REF_OFFSET);
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, MC_CACHE_CONFIGURATION, CONFIG_ROW_OFFSET, CACHE_ROW_OFFSET);
    psb_cmdbuf_rendec_write(cmdbuf, reg_value);

    /* Vc1 Intensity compensation: H.264 does not use this command */
    reg_value = 0;
    psb_cmdbuf_rendec_write(cmdbuf, reg_value);

    reg_value = 0;
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, H264_WEIGHTED_FACTOR_DENOMINATOR, C_LOG2_WEIGHT_DENOM, slice_param->chroma_log2_weight_denom);
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, H264_WEIGHTED_FACTOR_DENOMINATOR, Y_LOG2_WEIGHT_DENOM, slice_param->luma_log2_weight_denom);
    psb_cmdbuf_rendec_write(cmdbuf, reg_value);

    psb_cmdbuf_rendec_end_chunk(cmdbuf);

    /* CHUNK: SEQ Commands 2 */
    /* send Slice Data for every slice */
    /* MUST be the last slice sent */
    {
        IMG_UINT32 ui32Mode  = pic_params->pic_fields.bits.weighted_pred_flag | (pic_params->pic_fields.bits.weighted_bipred_idc << 1);

        psb_cmdbuf_rendec_start_chunk(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, SLICE_PARAMS));

        reg_value = 0;
        REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, SLICE_PARAMS, CONSTRAINED_INTRA_PRED, pic_params->pic_fields.bits.constrained_intra_pred_flag);
        REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, SLICE_PARAMS, MODE_CONFIG, ui32Mode);
        REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, SLICE_PARAMS, DISABLE_DEBLOCK_FILTER_IDC, slice_param->disable_deblocking_filter_idc);
        REGIO_WRITE_FIELD_MASKEDLITE(reg_value, MSVDX_CMDS, SLICE_PARAMS, SLICE_ALPHA_CO_OFFSET_DIV2, slice_param->slice_alpha_c0_offset_div2);
        REGIO_WRITE_FIELD_MASKEDLITE(reg_value, MSVDX_CMDS, SLICE_PARAMS, SLICE_BETA_OFFSET_DIV2, slice_param->slice_beta_offset_div2);
        REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, SLICE_PARAMS, SLICE_FIELD_TYPE, ctx->field_type);
        REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, SLICE_PARAMS, SLICE_CODE_TYPE, aSliceTypeVAtoMsvdx[slice_param->slice_type % 5]);
        psb_cmdbuf_rendec_write(cmdbuf, reg_value);

        /* Store slice parameters in header */
        *(ctx->p_slice_params) = reg_value;

        psb_cmdbuf_rendec_end_chunk(cmdbuf);
    }


    psb_cmdbuf_rendec_end_block(cmdbuf);
}

/*
 * Adds a VASliceParameterBuffer to the list of slice params
 */
static VAStatus psb__H264_add_slice_param(context_H264_p ctx, object_buffer_p obj_buffer)
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

static void psb__H264_write_kick(context_H264_p ctx, VASliceParameterBufferH264 *slice_param)
{
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;

    (void) slice_param; /* Unused for now */

    *cmdbuf->cmd_idx++ = CMD_COMPLETION;
}

static void psb__H264_FE_state(context_H264_p ctx)
{
    uint32_t lldma_record_offset;
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;

    /* See RENDER_BUFFER_HEADER */
    *cmdbuf->cmd_idx++ = CMD_HEADER;

    ctx->p_slice_params = cmdbuf->cmd_idx;
    *cmdbuf->cmd_idx++ = 0; /* ui32SliceParams */

    lldma_record_offset = psb_cmdbuf_lldma_create(cmdbuf, &(ctx->preload_buffer), 0,
                          sizeof(PRELOAD), 0, LLDMA_TYPE_H264_PRELOAD_SAVE);
    RELOC(*cmdbuf->cmd_idx, lldma_record_offset, &(cmdbuf->buf));
    cmdbuf->cmd_idx++;

    lldma_record_offset = psb_cmdbuf_lldma_create(cmdbuf, &(ctx->preload_buffer), 0,
                          sizeof(PRELOAD), 0, LLDMA_TYPE_H264_PRELOAD_RESTORE);
    RELOC(*cmdbuf->cmd_idx, lldma_record_offset, &(cmdbuf->buf));
    cmdbuf->cmd_idx++;
}

static void psb__H264_preprocess_slice(context_H264_p ctx,
                                       VASliceParameterBufferH264 *slice_param)
{
    VAPictureParameterBufferH264 *pic_params = ctx->pic_params;
    uint32_t slice_qpy;

    ctx->first_mb_x = slice_param->first_mb_in_slice % ctx->picture_width_mb;
    ctx->first_mb_y = slice_param->first_mb_in_slice / ctx->picture_width_mb;
    ctx->slice0_params = 0;
    ctx->slice1_params = 0;

    ASSERT(pic_params);
    if (!pic_params) {
        /* This is an error */
        return;
    }

    if (!pic_params->pic_fields.bits.field_pic_flag && pic_params->seq_fields.bits.mb_adaptive_frame_field_flag) {
        /* If in MBAFF mode multiply MB y-address by 2 */
        ctx->first_mb_y *= 2;
    }

    slice_qpy = 26 + pic_params->pic_init_qp_minus26 + slice_param->slice_qp_delta;     /* (7-27) */

    REGIO_WRITE_FIELD_LITE(ctx->slice0_params, MSVDX_VEC_H264, CR_VEC_H264_BE_SLICE0, BE_DIRECT_SPATIAL_MV_PRED_FLAG,                   slice_param->direct_spatial_mv_pred_flag);
    REGIO_WRITE_FIELD_LITE(ctx->slice0_params, MSVDX_VEC_H264, CR_VEC_H264_BE_SLICE0, H264_BE_SLICE0_DISABLE_DEBLOCK_FILTER_IDC,        slice_param->disable_deblocking_filter_idc);
    REGIO_WRITE_FIELD_MASKEDLITE(ctx->slice0_params, MSVDX_VEC_H264, CR_VEC_H264_BE_SLICE0, H264_BE_SLICE0_ALPHA_CO_OFFSET_DIV2,        slice_param->slice_alpha_c0_offset_div2);
    REGIO_WRITE_FIELD_MASKEDLITE(ctx->slice0_params, MSVDX_VEC_H264, CR_VEC_H264_BE_SLICE0, H264_BE_SLICE0_BETA_OFFSET_DIV2,            slice_param->slice_beta_offset_div2);
    REGIO_WRITE_FIELD_LITE(ctx->slice0_params, MSVDX_VEC_H264, CR_VEC_H264_BE_SLICE0, H264_BE_SLICE0_FIELD_TYPE,                        ctx->field_type);
    REGIO_WRITE_FIELD_LITE(ctx->slice0_params, MSVDX_VEC_H264, CR_VEC_H264_FE_SLICE0, SLICETYPE,                                        aSliceTypeVAtoMsvdx[ slice_param->slice_type % 5]);
    REGIO_WRITE_FIELD_LITE(ctx->slice0_params, MSVDX_VEC_H264, CR_VEC_H264_FE_SLICE0, CABAC_INIT_IDC,                                   slice_param->cabac_init_idc);
    REGIO_WRITE_FIELD_LITE(ctx->slice0_params, MSVDX_VEC_H264, CR_VEC_H264_FE_SLICE0, SLICECOUNT,                                       ctx->slice_count);

    REGIO_WRITE_FIELD_LITE(ctx->slice1_params, MSVDX_VEC_H264, CR_VEC_H264_FE_SLICE1, FIRST_MB_IN_SLICE_X,      ctx->first_mb_x);
    REGIO_WRITE_FIELD_LITE(ctx->slice1_params, MSVDX_VEC_H264, CR_VEC_H264_FE_SLICE1, FIRST_MB_IN_SLICE_Y,      ctx->first_mb_y);
    REGIO_WRITE_FIELD_LITE(ctx->slice1_params, MSVDX_VEC_H264, CR_VEC_H264_FE_SLICE1, SLICEQPY,                 slice_qpy);
    REGIO_WRITE_FIELD_LITE(ctx->slice1_params, MSVDX_VEC_H264, CR_VEC_H264_FE_SLICE1, NUM_REF_IDX_L0_ACTIVE_MINUS1, slice_param->num_ref_idx_l0_active_minus1);
    REGIO_WRITE_FIELD_LITE(ctx->slice1_params, MSVDX_VEC_H264, CR_VEC_H264_FE_SLICE1, NUM_REF_IDX_L1_ACTIVE_MINUS1, slice_param->num_ref_idx_l1_active_minus1);
}

/* **************************************************************************************************************** */
/* Prepacked calculates VLC table offsets and reg address                                                           */
/* **************************************************************************************************************** */
static const  IMG_UINT32 ui32H264VLCTableRegValPair[] = {
    (REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR0) + 0x00000000), 0x00026000,
    (REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR0) + 0x00000004), 0x000738a0,
    (REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR0) + 0x00000008), 0x000828f4,
    (REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR0) + 0x0000000c), 0x000a312d,
    (REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR0) + 0x00000010), 0x000b5959,
    (REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR0) + 0x00000014), 0x000c517b,
    (REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR0) + 0x00000018), 0x000d1196,
    (REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR0) + 0x0000001c), 0x000db1ad,
    (REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR0) + 0x00000020), 0x000e21be,
    (REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR0) + 0x00000024), 0x000e59c8,
    (REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR0) + 0x00000028), 0x000e79cd,
    (REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR0) + 0x0000002c), 0x000eb1d3,
    (REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR0) + 0x00000030), 0x000ed1d8,
    (REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR0) + 0x00000034), 0x000f09dd,
    (REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR0) + 0x00000038), 0x000f71e7,
    (REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR0) + 0x0000003c), 0x000001f6,
    (REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR0) + 0x00000040), 0x1256a4dd,
    (REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR0) + 0x00000044), 0x01489292,
    (REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR0) + 0x00000048), 0x11248050,
    (REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR0) + 0x0000004c), 0x00000002,
    (REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR0) + 0x00000050), 0x00002a02,
    (REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR0) + 0x00000054), 0x0108282a,
};


static void psb__H264_write_VLC_tables(context_H264_p ctx)
{
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    unsigned int i;

    psb_cmdbuf_skip_start_block(cmdbuf, SKIP_ON_CONTEXT_SWITCH);

    /* VLC Table */
    /* Write a LLDMA Cmd to transfer VLD Table data */
    psb_cmdbuf_lldma_write_cmdbuf(cmdbuf, &ctx->vlc_packed_table, 0,
                                  sizeof(ui16H264VLCTableData),
                                  0, LLDMA_TYPE_VLC_TABLE);

    /* Writes the VLD offsets.  mtx need only do this on context switch*/
    psb_cmdbuf_reg_start_block(cmdbuf, 0);

    for (i = 0; i < (sizeof(ui32H264VLCTableRegValPair) / sizeof(ui32H264VLCTableRegValPair[0])) ; i += 2) {
        psb_cmdbuf_reg_set(cmdbuf, ui32H264VLCTableRegValPair[i] , ui32H264VLCTableRegValPair[i + 1]);
    }

    psb_cmdbuf_reg_end_block(cmdbuf);

    psb_cmdbuf_skip_end_block(cmdbuf);
}

static void psb__set_macroblock_parameters(context_H264_p ctx, unsigned int value, unsigned int size)
{
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    psb_surface_p target_surface = ctx->obj_context->current_render_target->psb_surface;
    psb_buffer_p colocated_target_buffer = psb__H264_lookup_colocated_buffer(ctx, target_surface);

    if (NULL == colocated_target_buffer) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s can't find colocated buffer for target surface!\n", __func__);
        return;
    }

    psb_cmdbuf_reg_start_block(cmdbuf, 0);
    psb_cmdbuf_reg_set(cmdbuf, (REG_MSVDX_VEC_RAM_OFFSET + 0xcc0) , value);
    psb_cmdbuf_reg_end_block(cmdbuf);

    psb_cmdbuf_lldma_write_cmdbuf(cmdbuf, colocated_target_buffer, colocated_target_buffer->buffer_ofs,
                                  size,
                                  0, LLDMA_TYPE_MEM_SET);
}

static VAStatus psb__H264_process_slice(context_H264_p ctx,
                                        VASliceParameterBufferH264 *slice_param,
                                        object_buffer_p obj_buffer)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    ASSERT((obj_buffer->type == VASliceDataBufferType) || (obj_buffer->type == VAProtectedSliceDataBufferType));

#if 0
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "H264 process slice %d\n", ctx->slice_count);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "    profile = %s\n", profile2str[ctx->profile]);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "    size = %08x offset = %08x\n", slice_param->slice_data_size, slice_param->slice_data_offset);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "    first mb = %d macroblock offset = %d\n", slice_param->first_mb_in_slice, slice_param->slice_data_bit_offset);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "    slice_data_flag = %d\n", slice_param->slice_data_flag);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "    coded size = %dx%d\n", ctx->picture_width_mb, ctx->picture_height_mb);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "    slice type = %s\n", slice2str[(slice_param->slice_type % 5)]);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "    weighted_pred_flag = %d weighted_bipred_idc = %d\n", ctx->pic_params->pic_fields.bits.weighted_pred_flag, ctx->pic_params->pic_fields.bits.weighted_bipred_idc);
#endif

    if ((slice_param->slice_data_flag == VA_SLICE_DATA_FLAG_BEGIN) ||
        (slice_param->slice_data_flag == VA_SLICE_DATA_FLAG_ALL)) {
        if (0 == slice_param->slice_data_size) {
            vaStatus = VA_STATUS_ERROR_UNKNOWN;
            DEBUG_FAILURE;
            return vaStatus;
        }
        ASSERT(!ctx->split_buffer_pending);

        psb__H264_preprocess_slice(ctx, slice_param);

        /* Initialise the command buffer */
        /* TODO: Reuse current command buffer until full */
        psb_context_get_next_cmdbuf(ctx->obj_context);
        /* Add slice data buffer to the buffer list */
        psb_cmdbuf_buffer_ref(ctx->obj_context->cmdbuf, obj_buffer->psb_buffer);

        psb__H264_FE_state(ctx);

        psb__H264_write_VLC_tables(ctx);

        psb_cmdbuf_lldma_write_bitstream(ctx->obj_context->cmdbuf,
                                         obj_buffer->psb_buffer,
                                         obj_buffer->psb_buffer->buffer_ofs + slice_param->slice_data_offset,
                                         slice_param->slice_data_size,
                                         slice_param->slice_data_bit_offset,
                                         CMD_ENABLE_RBDU_EXTRACTION);

        if (slice_param->slice_data_flag == VA_SLICE_DATA_FLAG_BEGIN) {
            ctx->split_buffer_pending = TRUE;
        }
    } else {
        ASSERT(ctx->split_buffer_pending);
        ASSERT(0 == slice_param->slice_data_offset);
        /* Create LLDMA chain to continue buffer */
        if (slice_param->slice_data_size) {
            psb_cmdbuf_lldma_write_bitstream_chained(ctx->obj_context->cmdbuf,
                    obj_buffer->psb_buffer,
                    slice_param->slice_data_size);
        }
    }

    if ((slice_param->slice_data_flag == VA_SLICE_DATA_FLAG_ALL) ||
        (slice_param->slice_data_flag == VA_SLICE_DATA_FLAG_END)) {
        if (slice_param->slice_data_flag == VA_SLICE_DATA_FLAG_END) {
            ASSERT(ctx->split_buffer_pending);
        }
#if 0
        if (ctx->slice_count == 0) {
            const unsigned int size = (((ctx->size_mb + 100) * 128 + 0xfff) & ~0xfff);

            /* this code only works for progressive frames,  for interlaced, a 2d dma is required to
               set only one of the fields data to 0 */
            ASSERT(ctx->pic_params->pic_fields.bits.field_pic_flag == 0);

            /* Set the macroblock parameters to 0 */
            psb__set_macroblock_parameters(ctx, 0, size);
        }
#endif
        psb__H264_build_register(ctx, slice_param);

        psb__H264_build_rendec_params(ctx, slice_param);

        psb__H264_write_kick(ctx, slice_param);

        ctx->split_buffer_pending = FALSE;
        ctx->obj_context->video_op = psb_video_vld;
        ctx->obj_context->flags = 0;
        if (ctx->slice_count == 0) {
            ctx->obj_context->flags |= FW_VA_RENDER_IS_FIRST_SLICE;
        }
        if (ctx->pic_params->seq_fields.bits.mb_adaptive_frame_field_flag) {
            ctx->obj_context->flags |= FW_VA_RENDER_IS_H264_MBAFF;
        }
        if (ctx->two_pass_mode) {
            ctx->obj_context->flags |= FW_VA_RENDER_IS_TWO_PASS_DEBLOCK;
        }

        if (ctx->obj_context->driver_data->ec_enabled)
            ctx->obj_context->flags |= (FW_ERROR_DETECTION_AND_RECOVERY); /* FW_ERROR_DETECTION_AND_RECOVERY */

        ctx->obj_context->first_mb = (ctx->first_mb_y << 8) | ctx->first_mb_x;
        ctx->obj_context->last_mb = (((ctx->picture_height_mb >> ctx->pic_params->pic_fields.bits.field_pic_flag) - 1) << 8) | (ctx->picture_width_mb - 1);
        if (psb_context_submit_cmdbuf(ctx->obj_context)) {
            vaStatus = VA_STATUS_ERROR_UNKNOWN;
        }

        ctx->slice_count++;
    }
    return vaStatus;
}

static VAStatus psb__H264_process_slice_data(context_H264_p ctx, object_buffer_p obj_buffer)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VASliceParameterBufferH264 *slice_param;
    int buffer_idx = 0;
    unsigned int element_idx = 0;

    ASSERT((obj_buffer->type == VASliceDataBufferType) || (obj_buffer->type == VAProtectedSliceDataBufferType));

    ASSERT(ctx->pic_params);
    ASSERT(ctx->slice_param_list_idx);

    if ((!ctx->pic_params) || (!ctx->slice_param_list_idx)) {
        /* Picture params missing */
        drv_debug_msg(VIDEO_DEBUG_ERROR, "picture/slice parameter buffer should not be empty.\n");
        return VA_STATUS_ERROR_UNKNOWN;
    }
    if ((NULL == obj_buffer->psb_buffer) ||
        (0 == obj_buffer->size)) {
        /* We need to have data in the bitstream buffer */
        drv_debug_msg(VIDEO_DEBUG_ERROR, "bitstream buffer should not be empty.\n");
        return VA_STATUS_ERROR_UNKNOWN;
    }

    while (buffer_idx < ctx->slice_param_list_idx) {
        object_buffer_p slice_buf = ctx->slice_param_list[buffer_idx];
        /*need check whether slice parameter buffer is valid*/
        if ((NULL == slice_buf) ||
            (NULL == slice_buf->buffer_data) ||
            (slice_buf->size != sizeof(VASliceParameterBufferH264))) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "slice parameter buffer is not valid.\n");
            return VA_STATUS_ERROR_UNKNOWN;
        }

        if (element_idx >= slice_buf->num_elements) {
            /* Move to next buffer */
            element_idx = 0;
            buffer_idx++;
            continue;
        }

        slice_param = (VASliceParameterBufferH264 *) slice_buf->buffer_data;
        slice_param += element_idx;
        element_idx++;
        vaStatus = psb__H264_process_slice(ctx, slice_param, obj_buffer);
        if (vaStatus != VA_STATUS_SUCCESS) {
            DEBUG_FAILURE;
            break;
        }
    }
    ctx->slice_param_list_idx = 0;

    return vaStatus;
}

static VAStatus psb_H264_BeginPicture(
    object_context_p obj_context)
{
    INIT_CONTEXT_H264

    if (ctx->pic_params) {
        free(ctx->pic_params);
        ctx->pic_params = NULL;
    }
    if (ctx->iq_matrix) {
        free(ctx->iq_matrix);
        ctx->iq_matrix = NULL;
    }
    ctx->slice_count = 0;
    ctx->slice_group_map_buffer = NULL;
    return VA_STATUS_SUCCESS;
}

static VAStatus psb_H264_RenderPicture(
    object_context_p obj_context,
    object_buffer_p *buffers,
    int num_buffers)
{
    int i;
    INIT_CONTEXT_H264
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    for (i = 0; i < num_buffers; i++) {
        object_buffer_p obj_buffer = buffers[i];

        switch (obj_buffer->type) {
        case VAPictureParameterBufferType:
            /* drv_debug_msg(VIDEO_DEBUG_GENERAL, "psb_H264_RenderPicture got VAPictureParameterBuffer\n"); */
            vaStatus = psb__H264_process_picture_param(ctx, obj_buffer);
            DEBUG_FAILURE;
            break;

        case VAIQMatrixBufferType:
            /* drv_debug_msg(VIDEO_DEBUG_GENERAL, "psb_H264_RenderPicture got VAIQMatrixBufferType\n"); */
            vaStatus = psb__H264_process_iq_matrix(ctx, obj_buffer);
            DEBUG_FAILURE;
            break;

        case VASliceGroupMapBufferType:
            /* drv_debug_msg(VIDEO_DEBUG_GENERAL, "psb_H264_RenderPicture got VASliceGroupMapBufferType\n"); */
            vaStatus = psb__H264_process_slice_group_map(ctx, obj_buffer);
            DEBUG_FAILURE;
            break;

        case VASliceParameterBufferType:
            /* drv_debug_msg(VIDEO_DEBUG_GENERAL, "psb_H264_RenderPicture got VASliceParameterBufferType\n"); */
            vaStatus = psb__H264_add_slice_param(ctx, obj_buffer);
            DEBUG_FAILURE;
            break;

        case VASliceDataBufferType:
        case VAProtectedSliceDataBufferType:
            /* drv_debug_msg(VIDEO_DEBUG_GENERAL, "psb_H264_RenderPicture got %s\n", SLICEDATA_BUFFER_TYPE(obj_buffer->type)); */
            vaStatus = psb__H264_process_slice_data(ctx, obj_buffer);
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

static VAStatus psb_H264_EndPicture(
    object_context_p obj_context)
{
    INIT_CONTEXT_H264
    psb_driver_data_p driver_data = obj_context->driver_data;
    psb_surface_p target_surface = ctx->obj_context->current_render_target->psb_surface;

    if (obj_context->is_oold && !ctx->two_pass_mode) {

        psb_buffer_p colocated_target_buffer = psb__H264_lookup_colocated_buffer(ctx, target_surface);

        psb_context_submit_oold(ctx->obj_context,
                                target_surface->in_loop_buf,
                                &target_surface->buf,
                                colocated_target_buffer,
                                ctx->picture_width_mb,
                                ctx->picture_height_mb,
                                ctx->field_type,
                                target_surface->chroma_offset);
    }

    driver_data->decode_info.num_surface = ctx->obj_context->num_render_targets;
    driver_data->decode_info.surface_id = obj_context->current_render_surface_id;

#if 0
    if (driver_data->ec_enabled && IS_MRST(driver_data)) {
        psb_context_submit_host_be_opp(ctx->obj_context, &target_surface->buf,
                                       target_surface->stride, target_surface->size,
                                       ctx->picture_width_mb, ctx->size_mb);
    }
#endif

    if (psb_context_flush_cmdbuf(ctx->obj_context)) {
        return VA_STATUS_ERROR_UNKNOWN;
    }

#if 0
    if (driver_data->ec_enabled && IS_MRST(driver_data)) {
        psb_context_get_next_cmdbuf(ctx->obj_context);
        psb_context_submit_host_be_opp(ctx->obj_context, &target_surface->buf,
                                       target_surface->stride, target_surface->size,
                                       ctx->picture_width_mb, ctx->size_mb);
        if (psb_context_flush_cmdbuf(ctx->obj_context))
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "psb_H264: flush deblock cmdbuf error\n");

    }
#endif

    if (ctx->two_pass_mode && (CONTEXT_ROTATE(ctx->obj_context) == 0)) {
        unsigned char *pMbData = NULL;

        psb_surface_p target_surface = ctx->obj_context->current_render_target->psb_surface;
        psb_buffer_p colocated_target_buffer = psb__H264_lookup_colocated_buffer(ctx, target_surface);

        drv_debug_msg(VIDEO_DEBUG_GENERAL, "psb_H264_EndPicture got two pass mode frame\n");

        psb_surface_sync(target_surface);

        psb_context_get_next_cmdbuf(ctx->obj_context);

        if ((NULL == colocated_target_buffer)
            || psb_buffer_map(colocated_target_buffer, &pMbData)) {
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "psb_H264: map colocated buffer error!\n");
        } else {
            int ret;
            pMbData += colocated_target_buffer->buffer_ofs;

            ret = psb_cmdbuf_second_pass(ctx->obj_context,
                                         ctx->obj_context->operating_mode, /* Write RegIO pairs into cmdbuf->buf */
                                         pMbData,
                                         ctx->picture_width_mb,
                                         ctx->picture_height_mb,
                                         &target_surface->buf,
                                         target_surface->chroma_offset);

            psb_buffer_unmap(colocated_target_buffer);

            //printf("Ret of psb_cmdbuf_second_pass is %d\n", ret);
            if (!ret) {
                //printf("Submit deblock msg and flush cmdbuf\n");
                psb_context_submit_deblock(ctx->obj_context);

                if (psb_context_flush_cmdbuf(ctx->obj_context))
                    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psb_H264: flush deblock cmdbuf error\n");
            }
        }
    }

    if (ctx->pic_params) {
        free(ctx->pic_params);
        ctx->pic_params = NULL;
    }

    if (ctx->iq_matrix) {
        free(ctx->iq_matrix);
        ctx->iq_matrix = NULL;
    }

    return VA_STATUS_SUCCESS;
}

struct format_vtable_s psb_H264_vtable = {
queryConfigAttributes:
    psb_H264_QueryConfigAttributes,
validateConfig:
    psb_H264_ValidateConfig,
createContext:
    psb_H264_CreateContext,
destroyContext:
    psb_H264_DestroyContext,
beginPicture:
    psb_H264_BeginPicture,
renderPicture:
    psb_H264_RenderPicture,
endPicture:
    psb_H264_EndPicture
};
