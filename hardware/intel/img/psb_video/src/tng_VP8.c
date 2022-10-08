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
 *    Mingruo Sun <mingruo.sun@intel.com>
 *    Li Zeng <li.zeng@intel.com>
 *
 */
#include "tng_VP8.h"
#include "tng_vld_dec.h"
#include "psb_def.h"
#include "psb_drv_debug.h"

#include "hwdefs/reg_io2.h"
#include "hwdefs/msvdx_offsets.h"
#include "hwdefs/msvdx_cmds_io2.h"
#include "hwdefs/msvdx_core_regs_io2.h"
#include "hwdefs/msvdx_vec_reg_io2.h"
#include "hwdefs/msvdx_vec_vp8_line_store_mem_io2.h"
#include "hwdefs/msvdx_vec_vp8_reg_io2.h"
#include "hwdefs/dxva_fw_ctrl.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "pnw_rotate.h"

#define VEC_MODE_VP8	11

#define HW_SUPPORTED_MAX_PICTURE_WIDTH_VP8   1920
#define HW_SUPPORTED_MAX_PICTURE_HEIGHT_VP8  1088

#define RENDEC_REGISTER_OFFSET(__group__, __reg__ )	( (__group__##_##__reg__##_OFFSET) + ( REG_##__group__##_OFFSET ) )

const Probability b_mode_prob[10][10][9] =
{
    {
        { 231, 120, 48, 89, 115, 113, 120, 152, 112},
        { 152, 179, 64, 126, 170, 118, 46, 70, 95},
        { 175, 69, 143, 80, 85, 82, 72, 155, 103},
        { 56, 58, 10, 171, 218, 189, 17, 13, 152},
        { 144, 71, 10, 38, 171, 213, 144, 34, 26},
        { 114, 26, 17, 163, 44, 195, 21, 10, 173},
        { 121, 24, 80, 195, 26, 62, 44, 64, 85},
        { 170, 46, 55, 19, 136, 160, 33, 206, 71},
        { 63, 20, 8, 114, 114, 208, 12, 9, 226},
        { 81, 40, 11, 96, 182, 84, 29, 16, 36}
    },
    {
        { 134, 183, 89, 137, 98, 101, 106, 165, 148},
        { 72, 187, 100, 130, 157, 111, 32, 75, 80},
        { 66, 102, 167, 99, 74, 62, 40, 234, 128},
        { 41, 53, 9, 178, 241, 141, 26, 8, 107},
        { 104, 79, 12, 27, 217, 255, 87, 17, 7},
        { 74, 43, 26, 146, 73, 166, 49, 23, 157},
        { 65, 38, 105, 160, 51, 52, 31, 115, 128},
        { 87, 68, 71, 44, 114, 51, 15, 186, 23},
        { 47, 41, 14, 110, 182, 183, 21, 17, 194},
        { 66, 45, 25, 102, 197, 189, 23, 18, 22}
    },
    {
        { 88, 88, 147, 150, 42, 46, 45, 196, 205},
        { 43, 97, 183, 117, 85, 38, 35, 179, 61},
        { 39, 53, 200, 87, 26, 21, 43, 232, 171},
        { 56, 34, 51, 104, 114, 102, 29, 93, 77},
        { 107, 54, 32, 26, 51, 1, 81, 43, 31},
        { 39, 28, 85, 171, 58, 165, 90, 98, 64},
        { 34, 22, 116, 206, 23, 34, 43, 166, 73},
        { 68, 25, 106, 22, 64, 171, 36, 225, 114},
        { 34, 19, 21, 102, 132, 188, 16, 76, 124},
        { 62, 18, 78, 95, 85, 57, 50, 48, 51}
    },
    {
        { 193, 101, 35, 159, 215, 111, 89, 46, 111},
        { 60, 148, 31, 172, 219, 228, 21, 18, 111},
        { 112, 113, 77, 85, 179, 255, 38, 120, 114},
        { 40, 42, 1, 196, 245, 209, 10, 25, 109},
        { 100, 80, 8, 43, 154, 1, 51, 26, 71},
        { 88, 43, 29, 140, 166, 213, 37, 43, 154},
        { 61, 63, 30, 155, 67, 45, 68, 1, 209},
        { 142, 78, 78, 16, 255, 128, 34, 197, 171},
        { 41, 40, 5, 102, 211, 183, 4, 1, 221},
        { 51, 50, 17, 168, 209, 192, 23, 25, 82}
    },
    {
        { 125, 98, 42, 88, 104, 85, 117, 175, 82},
        { 95, 84, 53, 89, 128, 100, 113, 101, 45},
        { 75, 79, 123, 47, 51, 128, 81, 171, 1},
        { 57, 17, 5, 71, 102, 57, 53, 41, 49},
        { 115, 21, 2, 10, 102, 255, 166, 23, 6},
        { 38, 33, 13, 121, 57, 73, 26, 1, 85},
        { 41, 10, 67, 138, 77, 110, 90, 47, 114},
        { 101, 29, 16, 10, 85, 128, 101, 196, 26},
        { 57, 18, 10, 102, 102, 213, 34, 20, 43},
        { 117, 20, 15, 36, 163, 128, 68, 1, 26}
    },
    {
        { 138, 31, 36, 171, 27, 166, 38, 44, 229},
        { 67, 87, 58, 169, 82, 115, 26, 59, 179},
        { 63, 59, 90, 180, 59, 166, 93, 73, 154},
        { 40, 40, 21, 116, 143, 209, 34, 39, 175},
        { 57, 46, 22, 24, 128, 1, 54, 17, 37},
        { 47, 15, 16, 183, 34, 223, 49, 45, 183},
        { 46, 17, 33, 183, 6, 98, 15, 32, 183},
        { 65, 32, 73, 115, 28, 128, 23, 128, 205},
        { 40, 3, 9, 115, 51, 192, 18, 6, 223},
        { 87, 37, 9, 115, 59, 77, 64, 21, 47}
    },
    {
        { 104, 55, 44, 218, 9, 54, 53, 130, 226},
        { 64, 90, 70, 205, 40, 41, 23, 26, 57},
        { 54, 57, 112, 184, 5, 41, 38, 166, 213},
        { 30, 34, 26, 133, 152, 116, 10, 32, 134},
        { 75, 32, 12, 51, 192, 255, 160, 43, 51},
        { 39, 19, 53, 221, 26, 114, 32, 73, 255},
        { 31, 9, 65, 234, 2, 15, 1, 118, 73},
        { 88, 31, 35, 67, 102, 85, 55, 186, 85},
        { 56, 21, 23, 111, 59, 205, 45, 37, 192},
        { 55, 38, 70, 124, 73, 102, 1, 34, 98}
    },
    {
        { 102, 61, 71, 37, 34, 53, 31, 243, 192},
        { 69, 60, 71, 38, 73, 119, 28, 222, 37},
        { 68, 45, 128, 34, 1, 47, 11, 245, 171},
        { 62, 17, 19, 70, 146, 85, 55, 62, 70},
        { 75, 15, 9, 9, 64, 255, 184, 119, 16},
        { 37, 43, 37, 154, 100, 163, 85, 160, 1},
        { 63, 9, 92, 136, 28, 64, 32, 201, 85},
        { 86, 6, 28, 5, 64, 255, 25, 248, 1},
        { 56, 8, 17, 132, 137, 255, 55, 116, 128},
        { 58, 15, 20, 82, 135, 57, 26, 121, 40}
    },
    {
        { 164, 50, 31, 137, 154, 133, 25, 35, 218},
        { 51, 103, 44, 131, 131, 123, 31, 6, 158},
        { 86, 40, 64, 135, 148, 224, 45, 183, 128},
        { 22, 26, 17, 131, 240, 154, 14, 1, 209},
        { 83, 12, 13, 54, 192, 255, 68, 47, 28},
        { 45, 16, 21, 91, 64, 222, 7, 1, 197},
        { 56, 21, 39, 155, 60, 138, 23, 102, 213},
        { 85, 26, 85, 85, 128, 128, 32, 146, 171},
        { 18, 11, 7, 63, 144, 171, 4, 4, 246},
        { 35, 27, 10, 146, 174, 171, 12, 26, 128}
    },
    {
        { 190, 80, 35, 99, 180, 80, 126, 54, 45},
        { 85, 126, 47, 87, 176, 51, 41, 20, 32},
        { 101, 75, 128, 139, 118, 146, 116, 128, 85},
        { 56, 41, 15, 176, 236, 85, 37, 9, 62},
        { 146, 36, 19, 30, 171, 255, 97, 27, 20},
        { 71, 30, 17, 119, 118, 255, 17, 18, 138},
        { 101, 38, 60, 138, 55, 70, 43, 26, 142},
        { 138, 45, 61, 62, 219, 1, 81, 188, 64},
        { 32, 41, 20, 117, 151, 142, 20, 21, 163},
        { 112, 19, 12, 61, 195, 128, 48, 4, 24}
    }
};

static const uint32_t RegEntdecFeControl = ( (VEC_MODE_VP8 & 0x7) << MSVDX_VEC_CR_VEC_ENTDEC_FE_CONTROL_ENTDEC_FE_MODE_SHIFT ) 
                                    | ( (VEC_MODE_VP8 >> 3) << MSVDX_VEC_CR_VEC_ENTDEC_FE_CONTROL_ENTDEC_FE_EXTENDED_MODE_SHIFT );

#define GET_SURFACE_INFO_picture_coding_type(psb_surface)	((int) (psb_surface->extra_info[2]))
#define SET_SURFACE_INFO_picture_coding_type(psb_surface, val) psb_surface->extra_info[2] = (uint32_t) val;
#define SET_SURFACE_INFO_rotate(psb_surface, rotate) psb_surface->extra_info[5]		  = (uint32_t) rotate;
#define GET_SURFACE_INFO_rotate(psb_surface)			((int) psb_surface->extra_info[5])

/* Truncates a signed integer to 17 bits */
#define SIGNTRUNC( x ) ((( (x)  >> 15) & 0x10000) | ( (x) & 0xffff))

#define MSVDX_VEC_REGS_BASE_MTX 0x0800
#define MSVDX_COMMANDS_BASE_MTX 0x1000
#define MSVDX_IQRAM_BASE_MTX    0x700

#define SLICEDATA_BUFFER_TYPE(type)	((type==VASliceDataBufferType)?"VASliceDataBufferType":"VAProtectedSliceDataBufferType")

#define VP8_BYTES_PER_MB                        (128)
#define VLR_OFFSET_VP8_PARSER                   (0x0920)
#define INTRA_BUFFER_SIZE                       (32768)
#define DECODECONTROL_BUFFER_SIZE               (1024*12)
#define DECODECONTROL_BUFFER_LLDMA_OFFSET       (1024*8)
// AuxLineBuffer is 256k
#define AUX_LINE_BUFFER_SIZE                    (1024*256)

#define VP8_BUFFOFFSET_MASK		(0x00ffffff)
#define VP8_PARTITIONSCOUNT_MASK	(0x0f000000)
#define VP8_PARTITIONSCOUNT_SHIFT	(24)

#define VP8_FRAMETYPE_MASK      (0x08000000)
#define VP8_FRAMETYPE_SHIFT     (27)

#define MAX_MB_SEGMENTS         4
#define SEGMENT_DELTADATA       0
#define SEGMENT_ABSDATA         1
#define MAX_LOOP_FILTER         63
#define MAX_QINDEX              127

#define CABAC_LSR_ProbsBaseOffset	0x000

const uint32_t	CABAC_LSR_KeyFrame_BModeProb_Address   = CABAC_LSR_ProbsBaseOffset;
const uint32_t	CABAC_LSR_KeyFrame_BModeProb_Stride    = 12;
const uint32_t	CABAC_LSR_KeyFrame_BModeProb_Valid     = 9;
const uint32_t CABAC_LSR_KeyFrame_BModeProb_ToIdxMap[] = {0, 1, 2, 3, 4, 6, 5, 0/*ignored value here*/, 7, 8, 0/*ignored value here*/, 0/*ignored value here*/};

const uint32_t	CABAC_LSR_InterFrame_YModeProb_Address   = CABAC_LSR_ProbsBaseOffset;
const uint32_t	CABAC_LSR_InterFrame_YModeProb_Stride    = 4;
const uint32_t	CABAC_LSR_InterFrame_YModeProb_Valid     = 4;
const uint32_t CABAC_LSR_InterFrame_YModeProb_ToIdxMap[] = {0, 1, 2, 3};

const uint32_t	CABAC_LSR_InterFrame_UVModeProb_Address   = CABAC_LSR_ProbsBaseOffset + 0x04;
const uint32_t	CABAC_LSR_InterFrame_UVModeProb_Stride    = 4;
const uint32_t	CABAC_LSR_InterFrame_UVModeProb_Valid     = 3;
const uint32_t CABAC_LSR_InterFrame_UVModeProb_ToIdxMap[] = {0, 1, 2, 0/*ignored value here*/};

const uint32_t	CABAC_LSR_InterFrame_MVContextProb_Address   = CABAC_LSR_ProbsBaseOffset + 0x08;
const uint32_t	CABAC_LSR_InterFrame_MVContextProb_Stride    = 20;
const uint32_t	CABAC_LSR_InterFrame_MVContextProb_Valid     = 19;
const uint32_t CABAC_LSR_InterFrame_MVContextProb_ToIdxMap[] = {0, 2,  3, 6, 4, 5, 7, 8, 9, 10, 11, 18, 17, 16, 15, 14, 13, 12, 1, 0/*ignored value here*/};
                                                                  /* this unusual table as it moves coefficient for sgn from idx1 to idx 18 as well as arranges the rest */

const uint32_t	CABAC_LSR_CoefficientProb_Address   = CABAC_LSR_ProbsBaseOffset;
const uint32_t	CABAC_LSR_CoefficientProb_Stride    = 12;
const uint32_t	CABAC_LSR_CoefficientProb_Valid     = 11;
const uint32_t CABAC_LSR_CoefficientProb_ToIdxMap[] = {0, 1, 2, 3, 4, 6, 0/*ignored value here*/, 5, 7, 8, 9, 10};


typedef enum
{
    Level_AltQuant = 0,
    Level_AltLoopFilter,
    Level_Max
} LevelFeatures;

struct context_VP8_s {
    struct context_DEC_s dec_ctx;
    object_context_p	obj_context;	/* back reference */

    uint32_t	display_picture_size;
    uint32_t	coded_picture_size;

    /* Picture parameters */
    VAPictureParameterBufferVP8 *pic_params;

    /* Probability tables */
    VAProbabilityDataBufferVP8	*probs_params;

    /* VP8 Inverse Quantization Matrix Buffer */
    VAIQMatrixBufferVP8 *iq_params;

    VASliceParameterBufferVP8   *slice_params;

    object_surface_p	golden_ref_picture;
    object_surface_p	alt_ref_picture;
    object_surface_p	last_ref_picture;
    /* VP8_FRAME_TYPE	picture_coding_mode; */

    int8_t      baseline_filter[MAX_MB_SEGMENTS];

    uint32_t	size_mb;        /* in macroblocks */
    uint32_t	slice_count;

    /* uint32_t	FE_MB_segment_probs; */
    /* uint32_t	FE_MB_flags_probs; */
    uint32_t	DCT_Base_Address_Offset;

    uint32_t	operating_mode;

    uint16_t	cache_ref_offset;
    uint16_t	cache_row_offset;

    uint32_t	buffer_size;
    uint32_t	segid_size;

    int8_t	q_index[MAX_MB_SEGMENTS];

    /* Registers */
    /*
    uint32_t	reg_FE_PIC0;
    uint32_t	reg_FE_PIC1;
    uint32_t	reg_FE_PIC2;
    uint32_t	reg_BE_PIC0;
    uint32_t	reg_BE_PIC1;
    uint32_t	reg_BE_PIC2;
    uint32_t	reg_BE_BaseAdr1stPartPic;
   */

    uint32_t cmd_slice_params;

/*      VP8 features        */
    /* MB Flags Buffer */
    struct psb_buffer_s MB_flags_buffer;

    /* Cur Pic Buffer */
    struct psb_buffer_s cur_pic_buffer;

    /* 1 St Part Buffer */
    struct psb_buffer_s buffer_1st_part;

    struct psb_buffer_s segID_buffer;

    /* Probability tables */                                                                                                                                               
    uint32_t		probability_data_buffer_size;                                                                                                                                 
    struct psb_buffer_s probability_data_buffer;

    uint32_t		probability_data_1st_part_size;
    struct psb_buffer_s probability_data_1st_part;

    uint32_t		probability_data_2nd_part_size;
    struct psb_buffer_s probability_data_2nd_part;

    struct psb_buffer_s intra_buffer;
};

typedef struct context_VP8_s	*context_VP8_p;

#define INIT_CONTEXT_VP8	context_VP8_p	ctx = (context_VP8_p) obj_context->format_data;

#define SURFACE(id)	((object_surface_p) object_heap_lookup( &ctx->obj_context->driver_data->surface_heap, id ))

static psb_surface_stride_t get_opp_mode_stride(context_VP8_p ctx, const unsigned int uiStride) {
    psb_surface_p    target_surface = ctx->obj_context->current_render_target->psb_surface;
    psb_surface_stride_t    eOppModeStride = target_surface->stride_mode;
    switch (uiStride) {
    case 4096:
        eOppModeStride = STRIDE_4096;
        break;
    case 2048:
	eOppModeStride = STRIDE_2048;
        break;
    case 1280:
	eOppModeStride = STRIDE_1280;
        break;
    case 1920:
        eOppModeStride = STRIDE_1920;
        break;
    case 1024:
        eOppModeStride = STRIDE_1024;
        break;
    case 512:
        eOppModeStride = STRIDE_512;
        break;
    case 352:
        eOppModeStride = STRIDE_352;
        break;
    default:
        eOppModeStride = STRIDE_UNDEFINED;/* Assume Extended stride is used */
        break;
    }

    return eOppModeStride;
}

static int get_inloop_opmod(context_VP8_p ctx) {
    ctx->obj_context->operating_mode = 0;
    psb_surface_p target_surface = ctx->obj_context->current_render_target->psb_surface;
    psb_surface_stride_t eOppModeStride = get_opp_mode_stride(ctx,target_surface->stride);

    if (eOppModeStride == STRIDE_UNDEFINED) {
        REGIO_WRITE_FIELD_LITE(ctx->obj_context->operating_mode, MSVDX_CMDS, OPERATING_MODE, USE_EXT_ROW_STRIDE, 1);
    } else {
        REGIO_WRITE_FIELD_LITE(ctx->obj_context->operating_mode, MSVDX_CMDS, OPERATING_MODE, ROW_STRIDE, eOppModeStride);
    }

    REGIO_WRITE_FIELD_LITE(ctx->obj_context->operating_mode, MSVDX_CMDS, OPERATING_MODE, CHROMA_INTERLEAVED, 0);

    return ctx->obj_context->operating_mode;
}

static void tng_VP8_QueryConfigAttributes(
    VAProfile profile,
    VAEntrypoint entrypoint,
    VAConfigAttrib *attrib_list,
    int num_attribs) {

    int i;
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "tng_VP8_QueryConfigAttributes\n");

    for (i = 0; i < num_attribs; i++) {
        switch (attrib_list[i].type) {
        case VAConfigAttribMaxPictureWidth:
            if ((entrypoint == VAEntrypointVLD) &&
                (profile == VAProfileVP8Version0_3))
                attrib_list[i].value = HW_SUPPORTED_MAX_PICTURE_WIDTH_VP8;
            else
                attrib_list[i].value = VA_ATTRIB_NOT_SUPPORTED;
            break;
        case VAConfigAttribMaxPictureHeight:
            if ((entrypoint == VAEntrypointVLD) &&
                (profile == VAProfileVP8Version0_3))
                attrib_list[i].value = HW_SUPPORTED_MAX_PICTURE_HEIGHT_VP8;
            else
                attrib_list[i].value = VA_ATTRIB_NOT_SUPPORTED;
            break;
        default:
            break;
        }
    }

}

static VAStatus tng_VP8_ValidateConfig(
    object_config_p obj_config) {
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


static void tng_VP8_DestroyContext(object_context_p obj_context);

static void tng__VP8_process_slice_data(context_DEC_p dec_ctx, VASliceParameterBufferBase *vld_slice_param);
static void tng__VP8_end_slice(context_DEC_p dec_ctx);
static void tng__VP8_begin_slice(context_DEC_p dec_ctx, VASliceParameterBufferBase *vld_slice_param);
static VAStatus tng_VP8_process_buffer(context_DEC_p dec_ctx, object_buffer_p buffer);

static VAStatus tng_VP8_CreateContext(
    object_context_p obj_context,
    object_config_p __maybe_unused obj_config) {
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    context_VP8_p ctx;

    ctx = (context_VP8_p) calloc(1,sizeof(struct context_VP8_s));

    if (NULL == ctx) {
        vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
        DEBUG_FAILURE;
        return vaStatus;
    }

    obj_context->format_data = (void *) ctx;
    ctx->obj_context = obj_context;
    ctx->pic_params  = NULL;

    ctx->dec_ctx.begin_slice = tng__VP8_begin_slice;
    ctx->dec_ctx.process_slice = tng__VP8_process_slice_data;
    ctx->dec_ctx.end_slice = tng__VP8_end_slice;
    ctx->dec_ctx.process_buffer = tng_VP8_process_buffer;

    /* ctx->cache_ref_offset = 72; */
    /* ctx->cache_row_offset = 4; */
    ctx->cache_ref_offset = 144;
    ctx->cache_row_offset = 8;

    //uint32_t TotalMBs = ((obj_context->picture_width + 19) * obj_context->picture_height) / (16*16);
    uint32_t total_mbs = (((obj_context->picture_height + 15) >> 4) + 4) * ((obj_context->picture_width + 15) >> 4);
    ctx->buffer_size = total_mbs * 64;    // 64 bytes per MB
    ctx->segid_size = total_mbs / 4;      // 2 bits per MB
    //ctx->segid_size = (ctx->buffer_size + 0xfff) & ~0xfff;
    ctx->segid_size = (ctx->segid_size + 0xfff) & ~0xfff;

    /* Create mem resource for current picture macroblock data to be stored */
    if (vaStatus == VA_STATUS_SUCCESS) {
        vaStatus = psb_buffer_create(obj_context->driver_data,
                                     ctx->buffer_size,
                                     psb_bt_cpu_vpu,
                                     &ctx->cur_pic_buffer);
        DEBUG_FAILURE;
    }

    /* Create mem resource for storing 1st partition .*/
    if (vaStatus == VA_STATUS_SUCCESS) {
        vaStatus = psb_buffer_create(obj_context->driver_data,
                                     ctx->buffer_size,
                                     psb_bt_cpu_vpu,
                                     &ctx->buffer_1st_part);
        DEBUG_FAILURE;
    }

    if (vaStatus == VA_STATUS_SUCCESS) {
        vaStatus = psb_buffer_create(obj_context->driver_data,
                                     ctx->segid_size,
                                     psb_bt_cpu_vpu,
                                     &ctx->segID_buffer);
        DEBUG_FAILURE;
    }

    /* Create mem resource for PIC MB Flags .*/ 
    /* one MB would take 2 bits to store Y2 flag and mb_skip_coeff flag, so size would be same as ui32segidsize */
    if (vaStatus == VA_STATUS_SUCCESS) {
        vaStatus = psb_buffer_create(obj_context->driver_data,
                                     ctx->segid_size,
                                     psb_bt_cpu_vpu,
                                     &ctx->MB_flags_buffer);
        DEBUG_FAILURE;
    }

    /* calculate the size of prbability buffer size for both the partitions */
    ctx->probability_data_1st_part_size = 1200;
    ctx->probability_data_2nd_part_size = 1200;

    /* allocate device memory for prbability table for the both the partitions.*/
    if (vaStatus == VA_STATUS_SUCCESS) {
        vaStatus = psb_buffer_create(obj_context->driver_data,
                                     ctx->probability_data_1st_part_size,
                                     psb_bt_cpu_vpu,
                                     &ctx->probability_data_1st_part);
        DEBUG_FAILURE;
    }

    /* allocate device memory for prbability table for the both the partitions.*/
    if (vaStatus == VA_STATUS_SUCCESS) {
        vaStatus = psb_buffer_create(obj_context->driver_data,
                                     ctx->probability_data_2nd_part_size,
                                     psb_bt_cpu_vpu,
                                     &ctx->probability_data_2nd_part);
        DEBUG_FAILURE;
    }
    ctx->dec_ctx.preload_buffer = &ctx->probability_data_2nd_part;

    if (vaStatus == VA_STATUS_SUCCESS) {
        vaStatus = psb_buffer_create(obj_context->driver_data,
                                     INTRA_BUFFER_SIZE,
                                     psb_bt_cpu_vpu,
                                     &ctx->intra_buffer);
        DEBUG_FAILURE;
    }

    if (vaStatus == VA_STATUS_SUCCESS) {
        vaStatus = vld_dec_CreateContext(&ctx->dec_ctx, obj_context);
        DEBUG_FAILURE;
    }

    if (vaStatus != VA_STATUS_SUCCESS) {
        tng_VP8_DestroyContext(obj_context);
    }

    return vaStatus;
}

static void tng_VP8_DestroyContext(
    object_context_p obj_context) {
    INIT_CONTEXT_VP8
    uint32_t i;

    vld_dec_DestroyContext(&ctx->dec_ctx);

    psb_buffer_destroy(&ctx->cur_pic_buffer);
    psb_buffer_destroy(&ctx->buffer_1st_part);
    psb_buffer_destroy(&ctx->segID_buffer);
    psb_buffer_destroy(&ctx->MB_flags_buffer);
    psb_buffer_destroy(&ctx->probability_data_1st_part);
    psb_buffer_destroy(&ctx->probability_data_2nd_part);
    psb_buffer_destroy(&ctx->intra_buffer);

    if (ctx->pic_params) {
        free(ctx->pic_params);
        ctx->pic_params = NULL;
    }

    if (ctx->probs_params) {
        free(ctx->probs_params);
        ctx->probs_params = NULL;
    }

    free(obj_context->format_data);
    obj_context->format_data = NULL;
}

#ifdef DEBUG_TRACE
#define P(x)    psb__trace_message("PARAMS: " #x "\t= %08x (%d)\n", p->x, p->x)
static void tng__VP8_trace_pic_params(VAPictureParameterBufferVP8 *p) {
    P(prob_skip_false);
    P(prob_intra);
    P(prob_last);
    P(prob_gf);
}
static void tng__VP8_trace_slc_params(VASliceParameterBufferVP8 *p) {
    P(num_of_partitions);
    P(macroblock_offset);
}
#endif

static VAStatus tng__VP8_process_picture_param(context_VP8_p ctx, object_buffer_p obj_buffer) {
    object_surface_p obj_surface = ctx->obj_context->current_render_target;
    psb_surface_p target_surface = obj_surface->psb_surface;
    uint32_t reg_value;
    VAStatus vaStatus;

    ASSERT(obj_buffer->type == VAPictureParameterBufferType);
    ASSERT(obj_buffer->num_elements == 1);
    ASSERT(obj_buffer->size == sizeof(VAPictureParameterBufferVP8));
    ASSERT(target_surface);

    if ((obj_buffer->num_elements != 1) ||
        (obj_buffer->size != sizeof(VAPictureParameterBufferVP8)) ||
        (NULL == target_surface)) {
        return VA_STATUS_ERROR_UNKNOWN;
    }

    /* Transfer ownership of VAPictureParameterBufferVP8 data */
    VAPictureParameterBufferVP8 *pic_params = (VAPictureParameterBufferVP8 *) obj_buffer->buffer_data;
    if (ctx->pic_params) {
        free(ctx->pic_params);
    }

    ctx->pic_params = pic_params;
    obj_buffer->buffer_data = NULL;
    obj_buffer->size = 0;

#ifdef DEBUG_TRACE
    tng__VP8_trace_pic_params(pic_params);
#endif

    if (obj_surface->share_info) {
        obj_surface->share_info->coded_width = ctx->pic_params->frame_width;
        obj_surface->share_info->coded_height = ctx->pic_params->frame_height;
    }

    ctx->size_mb = ((ctx->pic_params->frame_width) * (ctx->pic_params->frame_height)) >> 8;
    /* port DDK D3DDDIFMT_PICTUREPARAMSDATA ->  */
/*
    SetCodedWidth (  ((PicParam.FrameWidth + 15) / 16) * 16 );
    SetCodedHeight(   ((PicParam.FrameHeight + 15) / 16) * 16 );
    SetDisplayWidth(   PicParam.FrameWidth);
    SetDisplayHeight(  PicParam.FrameHeight);
*/
/*
    {
        object_surface_p obj_surface = SURFACE(pic_params->current_picture);
	if(obj_surface != ctx->obj_context->current_render_target)
		return VA_STATUS_ERROR_INVALID_SURFACE;
    }
*/

    /*ASYN_MODE would depend upon certain condition, VDEB module would be activated on the condition basis*/
    // uint32_t bdeblock = IMG_FALSE;
    /* // move to vp8vld to calculate loop_filter_disable
    if((pic_params->pic_fields.bits.version == 0) || (pic_params->pic_fields.bits.version == 1))
    {
     //   bdeblock =  (pic_params->loop_filter_level > 0 )? IMG_TRUE : IMG_FALSE ;
          pic_params->pic_fields.bits.loop_filter_disable =  (pic_params->loop_filter_level[0] > 0 )? IMG_TRUE : IMG_FALSE ;
    }
    */

    ctx->obj_context->operating_mode = get_inloop_opmod(ctx); /* port from ui32OperatingMode = mpDestFrame->GetInloopOpMode() */
    REGIO_WRITE_FIELD_LITE(ctx->obj_context->operating_mode, MSVDX_CMDS, OPERATING_MODE, CHROMA_FORMAT, 1);
    REGIO_WRITE_FIELD_LITE(ctx->obj_context->operating_mode, MSVDX_CMDS, OPERATING_MODE, ASYNC_MODE, (pic_params->pic_fields.bits.loop_filter_disable == 0)? 0:1);/* 0 = VDMC and VDEB active.  1 = VDEB pass-thru. */
    REGIO_WRITE_FIELD_LITE(ctx->obj_context->operating_mode, MSVDX_CMDS, OPERATING_MODE, CODEC_MODE, VEC_MODE_VP8);
    REGIO_WRITE_FIELD_LITE(ctx->obj_context->operating_mode, MSVDX_CMDS, OPERATING_MODE, CODEC_PROFILE, pic_params->pic_fields.bits.version);

    if (pic_params->last_ref_frame != VA_INVALID_SURFACE) {
        ctx->last_ref_picture = SURFACE(pic_params->last_ref_frame);
    } else {
        /* for mmu fault protection */
        ctx->last_ref_picture = ctx->obj_context->current_render_target;
    }

    if (pic_params->golden_ref_frame != VA_INVALID_SURFACE) {
        ctx->golden_ref_picture = SURFACE(pic_params->golden_ref_frame);
    } else {
        /* for mmu fault protection */
        ctx->golden_ref_picture = ctx->obj_context->current_render_target;
    }

    if (pic_params->alt_ref_frame != VA_INVALID_SURFACE) {
        ctx->alt_ref_picture = SURFACE(pic_params->alt_ref_frame);
    } else {
        /* for mmu fault protection */
        ctx->alt_ref_picture = ctx->obj_context->current_render_target;
    }

    /* TODO Add VP8 support in this function */
    psb_CheckInterlaceRotate(ctx->obj_context, (unsigned char *)pic_params);

    return VA_STATUS_SUCCESS;
}

static VAStatus tng__VP8_process_probility_param(context_VP8_p ctx, object_buffer_p obj_buffer) {
    ASSERT(obj_buffer->type == VAProbabilityBufferType);
    ASSERT(obj_buffer->num_elements == 1);
    ASSERT(obj_buffer->size == sizeof(VANodeProbabilityBufferVP8));

    if ((obj_buffer->num_elements != 1) ||
        (obj_buffer->size != sizeof(VAProbabilityDataBufferVP8))) {
        return VA_STATUS_ERROR_UNKNOWN;
    }

    if (ctx->probs_params) {
        free(ctx->probs_params);
    }

    /* Transfer ownership of VANodeProbabilityBufferVP8 data */
    ctx->probs_params = (VAProbabilityDataBufferVP8 *) obj_buffer->buffer_data;
    obj_buffer->buffer_data = NULL;
    obj_buffer->size = 0;

    return VA_STATUS_SUCCESS;
}

static VAStatus tng__VP8_process_iq_matrix(context_VP8_p ctx, object_buffer_p obj_buffer) {
     ASSERT(obj_buffer->type == VAIQMatrixBufferType);
     ASSERT(obj_buffer->num_elements == 1);
     ASSERT(obj_buffer->size == sizeof(VAIQMatrixBufferVP8));
     if ((obj_buffer->num_elements != 1) ||
        (obj_buffer->size != sizeof(VAIQMatrixBufferVP8))) {
		return VA_STATUS_ERROR_UNKNOWN;
     }

     /* Transfer ownership of VAIQMatrixBufferVP8 data */
     if (ctx->iq_params) {
        free(ctx->iq_params);
    }

    ctx->iq_params = (VAIQMatrixBufferVP8 *) obj_buffer->buffer_data;
    obj_buffer->buffer_data = NULL;
    obj_buffer->size = 0;

    return VA_STATUS_SUCCESS;

}

/*
static void tng__VP8_set_operation_target(context_VP8_p ctx)
{
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    psb_surface_p target_surface = ctx->obj_context->current_render_target->psb_surface;

    psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, DISPLAY_PICTURE_SIZE));

    psb_cmdbuf_rendec_write(cmdbuf, ctx->display_picture_size);
    psb_cmdbuf_rendec_write(cmdbuf, ctx->coded_picture_size);
    psb_cmdbuf_rendec_write(cmdbuf, ctx->obj_context->operating_mode);

    psb_cmdbuf_rendec_write_address(cmdbuf, &target_surface->buf, target_surface->buf.buffer_ofs);
    psb_cmdbuf_rendec_write_address(cmdbuf, &target_surface->buf, target_surface->buf.buffer_ofs + target_surface->chroma_offset);
    psb_cmdbuf_rendec_end(cmdbuf);

    tng__VP8_setup_alternative_frame(ctx);
}
*/

static void tng__VP8_set_target_picture(context_VP8_p ctx) {
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    psb_surface_p target_surface = ctx->obj_context->current_render_target->psb_surface;
    psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, LUMA_RECONSTRUCTED_PICTURE_BASE_ADDRESSES));

    psb_cmdbuf_rendec_write_address(cmdbuf, &target_surface->buf, target_surface->buf.buffer_ofs + target_surface->luma_offset);
    psb_cmdbuf_rendec_write_address(cmdbuf, &target_surface->buf, target_surface->buf.buffer_ofs + target_surface->chroma_offset);
    psb_cmdbuf_rendec_end(cmdbuf);

}

/* Set Reference pictures address */
static void tng__VP8_set_reference_picture(context_VP8_p ctx) {
    /* Reference pictures base addresses */
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    psb_surface_p last_ref_surface = ctx->last_ref_picture->psb_surface;
    psb_surface_p golden_ref_surface = ctx->golden_ref_picture->psb_surface;
    psb_surface_p alt_ref_surface = ctx->alt_ref_picture->psb_surface;

    psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, REFERENCE_PICTURE_BASE_ADDRESSES));

    /* write last refernce picture address */
    psb_cmdbuf_rendec_write_address(cmdbuf, &last_ref_surface->buf, last_ref_surface->buf.buffer_ofs);
    psb_cmdbuf_rendec_write_address(cmdbuf, &last_ref_surface->buf, last_ref_surface->buf.buffer_ofs + last_ref_surface->chroma_offset);

    /* write Golden refernce picture address */
    psb_cmdbuf_rendec_write_address(cmdbuf, &golden_ref_surface->buf, golden_ref_surface->buf.buffer_ofs);
    psb_cmdbuf_rendec_write_address(cmdbuf, &golden_ref_surface->buf, golden_ref_surface->buf.buffer_ofs + golden_ref_surface->chroma_offset);

    /* write Alternate refernce picture address */
    psb_cmdbuf_rendec_write_address(cmdbuf, &alt_ref_surface->buf, alt_ref_surface->buf.buffer_ofs);
    psb_cmdbuf_rendec_write_address(cmdbuf, &alt_ref_surface->buf, alt_ref_surface->buf.buffer_ofs + alt_ref_surface->chroma_offset);

    psb_cmdbuf_rendec_end(cmdbuf);
}

/*
static void tng__VP8_set_picture_header_data(context_VP8_p ctx)
{
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    uint32_t reg_value;

    psb_cmdbuf_reg_start_block(cmdbuf, 0);

    psb_cmdbuf_reg_set(cmdbuf,  REGISTER_OFFSET(MSVDX_VEC_VP8, CR_VEC_VP8_FE_PIC0), ctx->reg_FE_PIC0);
    psb_cmdbuf_reg_set(cmdbuf,  REGISTER_OFFSET(MSVDX_VEC_VP8, CR_VEC_VP8_FE_PIC1), ctx->reg_FE_PIC1);
    psb_cmdbuf_reg_set(cmdbuf,  REGISTER_OFFSET(MSVDX_VEC_VP8, CR_VEC_VP8_FE_PIC2), ctx->reg_FE_PIC2);

    psb_cmdbuf_reg_end_block(cmdbuf);

    psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, VP8_FILTER_SELECT));
    psb_cmdbuf_rendec_write(cmdbuf, ctx->reg_filter_param);
    psb_cmdbuf_rendec_end(cmdbuf);

    // BE Section
    {
        psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_VEC_VP8, CR_VEC_VP8_BE_PIC0));

        psb_cmdbuf_rendec_write(cmdbuf, ctx->reg_BE_PIC0 );
        psb_cmdbuf_rendec_write(cmdbuf, ctx->reg_BE_PIC1 );

        psb_cmdbuf_rendec_end(cmdbuf);
    }

    {
        psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_VEC_VP8, CR_VEC_VP8_BE_PIC2));

        psb_cmdbuf_rendec_write(cmdbuf, ctx->reg_BE_PIC2 );

        psb_cmdbuf_rendec_end(cmdbuf);
    }

    {
        psb_cmdbuf_rendec_start(cmdbuf , RENDEC_REGISTER_OFFSET(MSVDX_VEC_VP8, CR_VEC_VP8_BE_BASE_ADDR_1STPART_PIC));

        psb_cmdbuf_rendec_write_address(cmdbuf, &ctx->base_addr_1st_pic_buffer, ctx->base_addr_1st_pic_buffer.buffer_ofs);

        psb_cmdbuf_rendec_end(cmdbuf);
    }
    //rendec block
    {
        psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, MC_CACHE_CONFIGURATION));

        reg_value = 0;
        REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, MC_CACHE_CONFIGURATION, CONFIG_REF_OFFSET, ctx->cache_ref_offset );
        REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, MC_CACHE_CONFIGURATION, CONFIG_ROW_OFFSET, ctx->cache_row_offset );

        psb_cmdbuf_rendec_write(cmdbuf, reg_value);

        psb_cmdbuf_rendec_end(cmdbuf);
    }
}
*/

static void tng__VP8_compile_baseline_filter_data(context_VP8_p ctx) {
    uint32_t i;

    for (i = 0; i < MAX_MB_SEGMENTS; i++) {
        ctx->baseline_filter[i] = ctx->pic_params->loop_filter_level[i];
    }
}

static void tng__VP8_compile_qindex_data(context_VP8_p ctx) {
    uint32_t i;
    for (i = 0; i < MAX_MB_SEGMENTS; i++) {
	ctx->q_index[i] = ctx->iq_params->quantization_index[i][0];
    }
}

static void tng__CMDS_registers_write(context_VP8_p ctx) {
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    uint32_t reg_value;

    // psb_cmdbuf_reg_start_block(cmdbuf, 0);

    // psb_cmdbuf_reg_set(cmdbuf,  REGISTER_OFFSET(MSVDX_VEC_VP8, CR_VEC_VP8_FE_PIC0), ctx->reg_FE_PIC0);
    // psb_cmdbuf_reg_set(cmdbuf,  REGISTER_OFFSET(MSVDX_VEC_VP8, CR_VEC_VP8_FE_PIC1), ctx->reg_FE_PIC1);
    // psb_cmdbuf_reg_set(cmdbuf,  REGISTER_OFFSET(MSVDX_VEC_VP8, CR_VEC_VP8_FE_PIC2), ctx->reg_FE_PIC2);

    // psb_cmdbuf_reg_end_block(cmdbuf);

    psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, DISPLAY_PICTURE_SIZE));

    /* DISPLAY_PICTURE_SIZE */
    //ctx->display_picture_size = 0;
    reg_value = 0;
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, DISPLAY_PICTURE_SIZE, DISPLAY_PICTURE_WIDTH, (((ctx->pic_params->frame_width + 15) / 16) * 16) - 1);
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, DISPLAY_PICTURE_SIZE, DISPLAY_PICTURE_HEIGHT, (((ctx->pic_params->frame_height + 15) / 16) * 16) - 1);
    psb_cmdbuf_rendec_write(cmdbuf, reg_value);
   // psb_cmdbuf_rendec_end(cmdbuf);

    // psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, CODED_PICTURE_SIZE));
    /* CODED_PICTURE_SIZE */
    //ctx->coded_picture_size = 0;
    reg_value = 0;
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, CODED_PICTURE_SIZE, CODED_PICTURE_WIDTH, (((ctx->pic_params->frame_width + 15) / 16) * 16) - 1);
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_CMDS, CODED_PICTURE_SIZE, CODED_PICTURE_HEIGHT, (((ctx->pic_params->frame_height + 15) / 16) * 16) - 1);
    psb_cmdbuf_rendec_write(cmdbuf, reg_value);

    /* Set operating mode */
    psb_cmdbuf_rendec_write(cmdbuf, ctx->obj_context->operating_mode);

    psb_cmdbuf_rendec_end(cmdbuf);

    /* VP8_LOOP_FILTER_CONTROL */
    psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, VP8_LOOP_FILTER_CONTROL));

    reg_value = 0;
    REGIO_WRITE_FIELD_LITE( reg_value, MSVDX_CMDS,  VP8_LOOP_FILTER_CONTROL,  VP8_MODE_REF_LF_DELTA_ENABLED, ctx->pic_params->pic_fields.bits.loop_filter_adj_enable);
    REGIO_WRITE_FIELD_LITE( reg_value, MSVDX_CMDS,  VP8_LOOP_FILTER_CONTROL,  VP8_SHARPNESS_LEVEL, ctx->pic_params->pic_fields.bits.sharpness_level);
    psb_cmdbuf_rendec_write(cmdbuf, reg_value);
    psb_cmdbuf_rendec_end(cmdbuf); 

    /* VP8_LOOP_FILTER_BASELINE_LEVEL */
    psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, VP8_LOOP_FILTER_BASELINE_LEVEL));
    reg_value = 0;
    tng__VP8_compile_baseline_filter_data(ctx);
    REGIO_WRITE_FIELD( reg_value, MSVDX_CMDS, VP8_LOOP_FILTER_BASELINE_LEVEL, VP8_LF_BASLINE3, ctx->baseline_filter[3] );
    REGIO_WRITE_FIELD( reg_value, MSVDX_CMDS, VP8_LOOP_FILTER_BASELINE_LEVEL, VP8_LF_BASLINE2, ctx->baseline_filter[2]);
    REGIO_WRITE_FIELD( reg_value, MSVDX_CMDS, VP8_LOOP_FILTER_BASELINE_LEVEL, VP8_LF_BASLINE1, ctx->baseline_filter[1]);
    REGIO_WRITE_FIELD( reg_value, MSVDX_CMDS, VP8_LOOP_FILTER_BASELINE_LEVEL, VP8_LF_BASLINE0, ctx->baseline_filter[0]);
    psb_cmdbuf_rendec_write(cmdbuf, reg_value);
    psb_cmdbuf_rendec_end(cmdbuf);

    /* VP8_LOOP_FILTER_REFERENCE_DELTAS */
    psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, VP8_LOOP_FILTER_REFERENCE_DELTAS));
    reg_value = 0;
    REGIO_WRITE_FIELD(reg_value, MSVDX_CMDS, VP8_LOOP_FILTER_REFERENCE_DELTAS, VP8_REF_LF_DELTA3, ctx->pic_params->loop_filter_deltas_ref_frame[3]);
    REGIO_WRITE_FIELD(reg_value, MSVDX_CMDS, VP8_LOOP_FILTER_REFERENCE_DELTAS, VP8_REF_LF_DELTA2, ctx->pic_params->loop_filter_deltas_ref_frame[2]);
    REGIO_WRITE_FIELD(reg_value, MSVDX_CMDS, VP8_LOOP_FILTER_REFERENCE_DELTAS, VP8_REF_LF_DELTA1, ctx->pic_params->loop_filter_deltas_ref_frame[1]);
    REGIO_WRITE_FIELD(reg_value, MSVDX_CMDS, VP8_LOOP_FILTER_REFERENCE_DELTAS, VP8_REF_LF_DELTA0, ctx->pic_params->loop_filter_deltas_ref_frame[0]);
    psb_cmdbuf_rendec_write(cmdbuf, reg_value);
    psb_cmdbuf_rendec_end(cmdbuf);

    /* VP8_LOOP_FILTER_MODE_DELTAS */
    psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, VP8_LOOP_FILTER_MODE_DELTAS));
    reg_value = 0;
    REGIO_WRITE_FIELD( reg_value, MSVDX_CMDS, VP8_LOOP_FILTER_MODE_DELTAS, VP8_MODE_LF_DELTA3, ctx->pic_params->loop_filter_deltas_mode[3]);
    REGIO_WRITE_FIELD( reg_value, MSVDX_CMDS, VP8_LOOP_FILTER_MODE_DELTAS, VP8_MODE_LF_DELTA2, ctx->pic_params->loop_filter_deltas_mode[2]);
    REGIO_WRITE_FIELD( reg_value, MSVDX_CMDS, VP8_LOOP_FILTER_MODE_DELTAS, VP8_MODE_LF_DELTA1, ctx->pic_params->loop_filter_deltas_mode[1]);
    REGIO_WRITE_FIELD( reg_value, MSVDX_CMDS, VP8_LOOP_FILTER_MODE_DELTAS, VP8_MODE_LF_DELTA0, ctx->pic_params->loop_filter_deltas_mode[0]);
    psb_cmdbuf_rendec_write(cmdbuf, reg_value);
    psb_cmdbuf_rendec_end(cmdbuf); 

    psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, MC_CACHE_CONFIGURATION));
    reg_value = 0;
    REGIO_WRITE_FIELD_LITE( reg_value, MSVDX_CMDS, MC_CACHE_CONFIGURATION, CONFIG_REF_OFFSET, ctx->cache_ref_offset);
    REGIO_WRITE_FIELD_LITE( reg_value, MSVDX_CMDS, MC_CACHE_CONFIGURATION, CONFIG_ROW_OFFSET, ctx->cache_row_offset);
    psb_cmdbuf_rendec_write(cmdbuf, reg_value);
    psb_cmdbuf_rendec_end(cmdbuf); 

    /* psb_surface_p forward_ref_surface = ctx->forward_ref_picture->psb_surface; */
    /* psb_surface_p golden_ref_surface = ctx->golden_ref_picture->psb_surface; */

    /* rendec block */
    psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, INTRA_BUFFER_BASE_ADDRESS));
    // psb_cmdbuf_rendec_write_address(cmdbuf, &ctx->MB_flags_buffer, ctx->MB_flags_buffer.buffer_ofs);
    psb_cmdbuf_rendec_write_address(cmdbuf, &ctx->intra_buffer, ctx->intra_buffer.buffer_ofs);
    // psb_cmdbuf_rendec_write_address(cmdbuf, &forward_ref_surface->buf, forward_ref_surface->buf.buffer_ofs + forward_ref_surface->chroma_offset);
    psb_cmdbuf_rendec_end(cmdbuf);

    vld_dec_setup_alternative_frame(ctx->obj_context);  /* port from CVldDecoder::ProgramOutputModeRegisters */
}

static void tng__VP8_set_slice_param(context_VP8_p ctx) {
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;

    {
        psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, SLICE_PARAMS)  );

        ctx->cmd_slice_params = 0;
        REGIO_WRITE_FIELD_LITE( ctx->cmd_slice_params, MSVDX_CMDS, SLICE_PARAMS, SLICE_FIELD_TYPE, 0x02 ); // always a frame
        REGIO_WRITE_FIELD_LITE( ctx->cmd_slice_params, MSVDX_CMDS, SLICE_PARAMS, SLICE_CODE_TYPE, (ctx->pic_params->pic_fields.bits.key_frame == 0)? 0 : 1 );

        psb_cmdbuf_rendec_write(cmdbuf, ctx->cmd_slice_params);
        psb_cmdbuf_rendec_end(cmdbuf);
    }

    *ctx->dec_ctx.p_slice_params = ctx->cmd_slice_params;

    {
        uint32_t last_mb_row = ((ctx->pic_params->frame_height + 15) / 16) - 1;
        unsigned int pic_last_mb_xy = (last_mb_row << 8) | (((ctx->pic_params->frame_width + 15) / 16) - 1);
	unsigned int slice_first_mb_xy = 0; /* NA */
        *ctx->dec_ctx.slice_first_pic_last = (slice_first_mb_xy << 16) | pic_last_mb_xy;
	//ctx->cmd_header->uiSliceFirstMbYX_uiPicLastMbYX = (slice_first_mb_xy << 16) | pic_last_mb_xy;
    }

    /* if VP8 bitstream is multi-partitioned then convey all the info including buffer offest information for 2nd partition to the firmware  */
    //ctx->cmd_header->ui32Cmd_AdditionalParams |= ((ctx->pic_params->partition_size[0] + ((ctx->pic_params->pic_fields.bits.key_frame == 0) ? 10 : 3)) & VP8_BUFFOFFSET_MASK) ;
    //ctx->cmd_header->ui32Cmd_AdditionalParams |= ((ctx->pic_params->num_of_partitions << VP8_PARTITIONSCOUNT_SHIFT) & VP8_PARTITIONSCOUNT_MASK) ; /* if the bistream is multistream */

    (*ctx->dec_ctx.cmd_params) |= ((3 + ctx->slice_params->partition_size[0]) & VP8_BUFFOFFSET_MASK);
    (*ctx->dec_ctx.cmd_params) |= (((ctx->slice_params->num_of_partitions - 1) << VP8_PARTITIONSCOUNT_SHIFT) & VP8_PARTITIONSCOUNT_MASK) ; /* if the bistream is multistream */
    // not used in fw ctx->cmd_header->ui32Cmd_AdditionalParams |= ((ctx->pic_params->frame_type << VP8_FRAMETYPE_SHIFT) & VP8_BUFFOFFSET_MASK) ;
}

static void tng__VP8_FE_Registers_Write(context_VP8_p ctx) {
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    uint32_t reg_value;

    {
        /* set entdec control to a valid codec before accessing SR */
        psb_cmdbuf_reg_start_block(cmdbuf, 0);

	psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET (MSVDX_VEC, CR_VEC_ENTDEC_FE_CONTROL), RegEntdecFeControl);
	psb_cmdbuf_reg_end_block(cmdbuf);
    }

    {
        psb_cmdbuf_reg_start_block(cmdbuf, 0);
        reg_value=0;

        REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_VP8, CR_VEC_VP8_FE_PIC0, VP8_FE_FRAME_TYPE, (ctx->pic_params->pic_fields.bits.key_frame == 0)? 0 : 1 );
        REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_VP8, CR_VEC_VP8_FE_PIC0, VP8_FE_UPDATE_SEGMENTATION_MAP, (ctx->pic_params->pic_fields.bits.update_mb_segmentation_map == 0 )? 0 : 1 );
        REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_VP8, CR_VEC_VP8_FE_PIC0, VP8_FE_NUM_PARTITION_MINUS1, ctx->slice_params->num_of_partitions - 2);

       /* SEGMENTATION ID CONTROL */
       if(ctx->pic_params->pic_fields.bits.segmentation_enabled) {
            if(ctx->pic_params->pic_fields.bits.update_segment_feature_data) {
                REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_VP8, CR_VEC_VP8_FE_PIC0, VP8_FE_SEG_ID_CTRL, 2); // current
            } else {
                REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_VP8, CR_VEC_VP8_FE_PIC0, VP8_FE_SEG_ID_CTRL, 1); // previous
            }
       } else {
            REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_VP8, CR_VEC_VP8_FE_PIC0, VP8_FE_SEG_ID_CTRL, 0); // none
       }

       psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC_VP8, CR_VEC_VP8_FE_PIC0), reg_value);

       reg_value=0;
       REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_VP8, CR_VEC_VP8_FE_PIC1, VP8_FE_PIC_HEIGHT_IN_MBS_LESS1,((ctx->pic_params->frame_height + 15 )>> 4) - 1);
       REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_VP8, CR_VEC_VP8_FE_PIC1, VP8_FE_PIC_WIDTH_IN_MBS_LESS1, ((ctx->pic_params->frame_width + 15) >> 4) - 1 );
       psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC_VP8, CR_VEC_VP8_FE_PIC1), reg_value);

       reg_value=0;
       /* Important for VP8_FE_DECODE_PRED_NOT_COEFFS. First partition always has macroblock level data. See PDF, p. 34. */
       REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_VP8, CR_VEC_VP8_FE_PIC2, VP8_FE_DECODE_PRED_NOT_COEFFS, 1);
       REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_VP8, CR_VEC_VP8_FE_PIC2, VP8_FE_MB_NO_COEFF_SKIP, ctx->pic_params->pic_fields.bits.mb_no_coeff_skip);
       REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_VP8, CR_VEC_VP8_FE_PIC2, VP8_FE_SIGN_BIAS_FOR_GF, ctx->pic_params->pic_fields.bits.sign_bias_golden);
       REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_VP8, CR_VEC_VP8_FE_PIC2, VP8_FE_SIGN_BIAS_FOR_ALTREF, ctx->pic_params->pic_fields.bits.sign_bias_alternate);
       psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC_VP8, CR_VEC_VP8_FE_PIC2), reg_value);

       reg_value=0;
       REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_VP8, CR_VEC_VP8_FE_MB_SEGMENT_PROBS, VP8_FE_SEGMENT_ID_PROB0, ctx->pic_params->mb_segment_tree_probs[0]);
       REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_VP8, CR_VEC_VP8_FE_MB_SEGMENT_PROBS, VP8_FE_SEGMENT_ID_PROB1, ctx->pic_params->mb_segment_tree_probs[1]);
       REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_VP8, CR_VEC_VP8_FE_MB_SEGMENT_PROBS, VP8_FE_SEGMENT_ID_PROB2, ctx->pic_params->mb_segment_tree_probs[2]);
       psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET (MSVDX_VEC_VP8, CR_VEC_VP8_FE_MB_SEGMENT_PROBS), reg_value);

       reg_value = 0;
       REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_VP8, CR_VEC_VP8_FE_MB_FLAGS_PROBS, VP8_FE_SKIP_PROB, ctx->pic_params->prob_skip_false);
       REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_VP8, CR_VEC_VP8_FE_MB_FLAGS_PROBS, VP8_FE_INTRA_MB_PROB, ctx->pic_params->prob_intra);
       REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_VP8, CR_VEC_VP8_FE_MB_FLAGS_PROBS, VP8_FE_LAST_FRAME_PROB, ctx->pic_params->prob_last);
       REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_VP8, CR_VEC_VP8_FE_MB_FLAGS_PROBS, VP8_FE_GOLDEN_FRAME_PROB, ctx->pic_params->prob_gf);
       psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET (MSVDX_VEC_VP8, CR_VEC_VP8_FE_MB_FLAGS_PROBS), reg_value);
       psb_cmdbuf_reg_end_block(cmdbuf);
    }

    {
        psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;

	psb_cmdbuf_reg_start_block(cmdbuf, 0);

	/* psb_buffer_p* const ctx->MB_flags_buffer = ctx->MB_flags_buffer.buffer_ofs; */
        /* unsigned int MB_flags_buffer_alloc = ctx->MB_flags_buffer.buffer_ofs; */

	// psb_cmdbuf_reg_set(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_VEC_VP8, CR_VEC_VP8_FE_PIC_MB_FLAGS_BASE_ADDRESS), 0);
	psb_cmdbuf_reg_set_address(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_VEC_VP8, CR_VEC_VP8_FE_PIC_MB_FLAGS_BASE_ADDRESS), 
			           &ctx->MB_flags_buffer, ctx->MB_flags_buffer.buffer_ofs);
	psb_cmdbuf_reg_end_block(cmdbuf);
   }

   {  
        psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;

        psb_cmdbuf_reg_start_block(cmdbuf, 0);
	// psb_cmdbuf_reg_set(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_VEC_VP8, CR_VEC_VP8_FE_SEG_ID_BASE_ADDRESS),0);
	psb_cmdbuf_reg_set_address(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_VEC_VP8, CR_VEC_VP8_FE_SEG_ID_BASE_ADDRESS), &ctx->segID_buffer, ctx->segID_buffer.buffer_ofs);

	psb_cmdbuf_reg_end_block(cmdbuf);
   }

   {
       /* add the first partition offset */
       psb_cmdbuf_reg_start_block(cmdbuf, 0);

       ctx->DCT_Base_Address_Offset = (3 + ctx->slice_params->partition_size[0]) + 3 * (ctx->slice_params->num_of_partitions - 2) ;
       /* REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_VP8, CR_VEC_VP8_FE_DCT_BASE_ADDRESS, VP8_FE_DCT_BASE_ADDRESS, ctx->DCT_Base_Address); */
       psb_cmdbuf_reg_set_address(cmdbuf, REGISTER_OFFSET (MSVDX_VEC_VP8, CR_VEC_VP8_FE_DCT_BASE_ADDRESS),
				  ctx->dec_ctx.slice_data_buffer, ctx->DCT_Base_Address_Offset);

       psb_cmdbuf_reg_end_block(cmdbuf);
   }

}

static void tng__VP8_BE_Registers_Write(context_VP8_p ctx) {
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    uint32_t reg_value;

    {
        /* BE Section */
        psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_VEC, CR_VEC_ENTDEC_BE_CONTROL));

        psb_cmdbuf_rendec_write(cmdbuf, RegEntdecFeControl);

        psb_cmdbuf_rendec_end(cmdbuf);
    }

    {
        /* PIC0 */
        psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_VEC_VP8, CR_VEC_VP8_BE_PIC0));
        reg_value = 0;
        REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_VP8, CR_VEC_VP8_BE_PIC0, VP8_BE_FRAME_TYPE, (ctx->pic_params->pic_fields.bits.key_frame == 0)? 0 : 1);
        psb_cmdbuf_rendec_write(cmdbuf, reg_value);

        /* PIC1 */
        // psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_VEC_VP8, CR_VEC_VP8_BE_PIC1));
        reg_value = 0;
        REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_VP8, CR_VEC_VP8_BE_PIC1, VP8_BE_PIC_HEIGHT_IN_MBS_LESS1, ((ctx->pic_params->frame_height + 15) >> 4) - 1);
        REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_VP8, CR_VEC_VP8_BE_PIC1, VP8_BE_PIC_WIDTH_IN_MBS_LESS1, ((ctx->pic_params->frame_width + 15) >> 4) - 1);
        psb_cmdbuf_rendec_write(cmdbuf, reg_value);
        psb_cmdbuf_rendec_end(cmdbuf);
    }

    {
        /* PIC2 */
        psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_VEC_VP8, CR_VEC_VP8_BE_PIC2));
        reg_value = 0;
        REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_VP8, CR_VEC_VP8_BE_PIC2, VP8_BE_DECODE_PRED_NOT_COEFFS, 1);
        // REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_VP8, CR_VEC_VP8_BE_PIC2, VP8_BE_USE_STORED_SEGMENT_MAP, 1);
        psb_cmdbuf_rendec_write(cmdbuf, reg_value);
        psb_cmdbuf_rendec_end(cmdbuf);
    }

    {
        psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_VEC_VP8, CR_VEC_VP8_BE_QINDEXMAP));
        tng__VP8_compile_qindex_data(ctx);
        /* QINDEX MAP */
        reg_value = 0;
        REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VP8, CR_VEC_VP8_BE_QINDEXMAP, VP8_BE_QINDEX_SEG0_DEFAULT, ctx->q_index[0]);
        REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VP8, CR_VEC_VP8_BE_QINDEXMAP, VP8_BE_QINDEX_SEG1        , ctx->q_index[1]);
        REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VP8, CR_VEC_VP8_BE_QINDEXMAP, VP8_BE_QINDEX_SEG2        , ctx->q_index[2]);
        REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VP8, CR_VEC_VP8_BE_QINDEXMAP, VP8_BE_QINDEX_SEG3        , ctx->q_index[3]);
        psb_cmdbuf_rendec_write(cmdbuf, reg_value);
	psb_cmdbuf_rendec_end(cmdbuf);
    }

    {
        psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_VEC_VP8, CR_VEC_VP8_BE_QDELTAS));
        /* QDELTAS - this was removed in msvdx_vec_vp8_regs.def in 1.11 */
        reg_value = 0;
        uint16_t Y1_DC_Delta;
        Y1_DC_Delta = ctx->iq_params->quantization_index[0][1] - ctx->iq_params->quantization_index[0][0];
        REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VP8, CR_VEC_VP8_BE_QDELTAS, VP8_BE_YDC_DELTA, Y1_DC_Delta);

        uint16_t Y2_DC_Delta;
        Y2_DC_Delta  = ctx->iq_params->quantization_index[0][2] - ctx->iq_params->quantization_index[0][0];
        REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VP8, CR_VEC_VP8_BE_QDELTAS, VP8_BE_Y2DC_DELTA, Y2_DC_Delta);

        uint16_t Y2_AC_Delta;
        Y2_AC_Delta = ctx->iq_params->quantization_index[0][3] - ctx->iq_params->quantization_index[0][0];
        REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VP8, CR_VEC_VP8_BE_QDELTAS, VP8_BE_Y2AC_DELTA, Y2_AC_Delta);

       uint16_t UV_DC_Delta;
        UV_DC_Delta = ctx->iq_params->quantization_index[0][4] - ctx->iq_params->quantization_index[0][0];
        REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VP8, CR_VEC_VP8_BE_QDELTAS, VP8_BE_UVDC_DELTA, UV_DC_Delta);

        uint16_t UV_AC_Delta;
        UV_AC_Delta = ctx->iq_params->quantization_index[0][5] - ctx->iq_params->quantization_index[0][0];
        REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VP8, CR_VEC_VP8_BE_QDELTAS, VP8_BE_UVAC_DELTA, UV_AC_Delta);

        psb_cmdbuf_rendec_write(cmdbuf, reg_value);
        psb_cmdbuf_rendec_end(cmdbuf);
    }
#if 0
    {
        psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_VEC_VP8, CR_VEC_VP8_BE_VLR_BASE_ADDR));
        reg_value = 0;
        REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_VP8, CR_VEC_VP8_BE_VLR_BASE_ADDR, VP8_BE_DP_BUFFER_VLR_BASE_ADDR, VLR_OFFSET_VP8_PARSER);
        psb_cmdbuf_rendec_write(cmdbuf, reg_value);
	psb_cmdbuf_rendec_end(cmdbuf);
    }
#endif

    {  
        psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;

        psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_VEC_VP8, CR_VEC_VP8_BE_BASE_ADDR_1STPART_PIC));

	// psb_cmdbuf_rendec_write_address(cmdbuf, &ctx->buffer_1st_part, 0);
        psb_cmdbuf_rendec_write_address(cmdbuf, &ctx->buffer_1st_part, ctx->buffer_1st_part.buffer_ofs);

        psb_cmdbuf_rendec_end(cmdbuf);
    }

    {  
        psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;

        psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_VEC_VP8, CR_VEC_VP8_BE_BASE_ADDR_CURR));

        // psb_cmdbuf_rendec_write(cmdbuf, 0);
        // psb_cmdbuf_rendec_write_address(cmdbuf, &ctx->cur_pic_buffer, 0);
        psb_cmdbuf_rendec_write_address(cmdbuf, &ctx->cur_pic_buffer, ctx->cur_pic_buffer.buffer_ofs);

        psb_cmdbuf_rendec_end(cmdbuf);
    }
}


/***********************************************************************************
* Description        : Set Bool coder context which has been conveyed from application.
************************************************************************************/
static void tng__VP8_set_bool_coder_context(context_VP8_p ctx) {
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    uint32_t bool_init = 0;
    uint32_t bool_ctrl = 0;
    uint32_t reg_value = 0;
    {
        /* Entdec Front-End controls*/
        psb_cmdbuf_reg_start_block(cmdbuf, 0);

        REGIO_WRITE_FIELD_LITE (bool_init, MSVDX_VEC, CR_VEC_BOOL_INIT, BOOL_INIT_RANGE, ctx->pic_params->bool_coder_ctx.range);
        REGIO_WRITE_FIELD_LITE (bool_init, MSVDX_VEC, CR_VEC_BOOL_INIT, BOOL_INIT_VALUE, (ctx->pic_params->bool_coder_ctx.value) & 0xff);  /* Set only MSB of value param of bool context */
        psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC, CR_VEC_BOOL_INIT), bool_init);

        psb_cmdbuf_reg_end_block(cmdbuf);
    }

    {
        psb_cmdbuf_reg_start_block(cmdbuf, 0);

        REGIO_WRITE_FIELD_LITE(bool_ctrl, MSVDX_VEC, CR_VEC_BOOL_CTRL, BOOL_MASTER_SELECT, 0x02);
        psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC, CR_VEC_BOOL_CTRL), bool_ctrl);

        psb_cmdbuf_reg_end_block(cmdbuf);
    }

    {
        psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, MC_CACHE_CONFIGURATION));
        REGIO_WRITE_FIELD_LITE( reg_value, MSVDX_CMDS, MC_CACHE_CONFIGURATION, CONFIG_REF_OFFSET, ctx->cache_ref_offset);
        REGIO_WRITE_FIELD_LITE( reg_value, MSVDX_CMDS, MC_CACHE_CONFIGURATION, CONFIG_ROW_OFFSET, ctx->cache_row_offset);
        psb_cmdbuf_rendec_write(cmdbuf, reg_value);
        psb_cmdbuf_rendec_end(cmdbuf);
    }
}

/***********************************************************************************
* Description        : Write probability data in buffer according to MSVDX setting.
************************************************************************************/
static void tng_KeyFrame_BModeProbsDataCompile(Probability* ui8_probs_to_write, uint32_t* ui32_probs_buffer) {
    uint32_t dim0, dim1;
    uint32_t i, j, address;
    uint32_t src_tab_offset = 0;

    address = 0;

    for(dim1 = 0; dim1 < 10; dim1++) {
        for(dim0 = 0; dim0 < 10; dim0++) { 
            j =0;
            for (i =0 ; i < CABAC_LSR_KeyFrame_BModeProb_Stride ; i += 4) {
                *(ui32_probs_buffer+ address +  j++ ) =
                  (uint32_t)ui8_probs_to_write[CABAC_LSR_KeyFrame_BModeProb_ToIdxMap[i] + src_tab_offset  ]
                | (uint32_t)ui8_probs_to_write[CABAC_LSR_KeyFrame_BModeProb_ToIdxMap[i+1] + src_tab_offset ] << 8
                | (uint32_t)ui8_probs_to_write[CABAC_LSR_KeyFrame_BModeProb_ToIdxMap[i+2] + src_tab_offset ] << 16
                | (uint32_t)ui8_probs_to_write[CABAC_LSR_KeyFrame_BModeProb_ToIdxMap[i+3] + src_tab_offset ] << 24;            
            }
            /* increment the address by the stride */
            address +=  (CABAC_LSR_KeyFrame_BModeProb_Stride >> 2);
            /* increment source table offset */
            src_tab_offset += CABAC_LSR_KeyFrame_BModeProb_Valid;
        }
    }
}

/***********************************************************************************
* Description        : Write probability data in buffer according to MSVDX setting.
************************************************************************************/
static void tng_InterFrame_YModeProbsDataCompile(Probability* ui8_probs_to_write, uint32_t* ui32_probs_buffer) {
    *(ui32_probs_buffer) =      (uint32_t)ui8_probs_to_write[CABAC_LSR_InterFrame_YModeProb_ToIdxMap[0]]
                            |   (uint32_t)ui8_probs_to_write[CABAC_LSR_InterFrame_YModeProb_ToIdxMap[1]] << 8
                            |   (uint32_t)ui8_probs_to_write[CABAC_LSR_InterFrame_YModeProb_ToIdxMap[2]] << 16
                            |   (uint32_t)ui8_probs_to_write[CABAC_LSR_InterFrame_YModeProb_ToIdxMap[3]] << 24;
}

/***********************************************************************************
* Description        : Write probability data in buffer according to MSVDX setting.
************************************************************************************/
static void tng_InterFrame_UVModeProbsDataCompile(Probability* ui8_probs_to_write, uint32_t* ui32_probs_buffer) {
    *(ui32_probs_buffer) =      (uint32_t)ui8_probs_to_write[CABAC_LSR_InterFrame_UVModeProb_ToIdxMap[0]]
                            |   (uint32_t)ui8_probs_to_write[CABAC_LSR_InterFrame_UVModeProb_ToIdxMap[1]] << 8
                            |   (uint32_t)ui8_probs_to_write[CABAC_LSR_InterFrame_UVModeProb_ToIdxMap[2]] << 16;
}

/***********************************************************************************
* Description        : Write probability data in buffer according to MSVDX setting.
************************************************************************************/
static void tng_InterFrame_MVContextProbsDataCompile(uint8_t* ui8_probs_to_write, uint32_t* ui32_probs_buffer) {
    uint32_t dim0;
    uint32_t i, j, address;
    uint32_t src_tab_offset = 0;

    address = 0;

    for(dim0 = 0; dim0 < 2; dim0++) { 
        j = 0;
        for (i =0 ; i < CABAC_LSR_InterFrame_MVContextProb_Stride ; i += 4) {
                *(ui32_probs_buffer+ address +  j++ ) =
                  (uint32_t)ui8_probs_to_write[CABAC_LSR_InterFrame_MVContextProb_ToIdxMap[i] + src_tab_offset  ]
                | (uint32_t)ui8_probs_to_write[CABAC_LSR_InterFrame_MVContextProb_ToIdxMap[i+1] + src_tab_offset ] << 8
                | (uint32_t)ui8_probs_to_write[CABAC_LSR_InterFrame_MVContextProb_ToIdxMap[i+2] + src_tab_offset ] << 16
                | (uint32_t)ui8_probs_to_write[CABAC_LSR_InterFrame_MVContextProb_ToIdxMap[i+3] + src_tab_offset ] << 24;  
        }
        /* increment the address by the stride */
        address += (CABAC_LSR_InterFrame_MVContextProb_Stride >> 2);
        /* increment source table offset */
        src_tab_offset += CABAC_LSR_InterFrame_MVContextProb_Valid;
    }
}

/***********************************************************************************
* Description        : Write probability data in buffer according to MSVDX setting.
************************************************************************************/
static void tng_DCT_Coefficient_ProbsDataCompile(Probability* ui8_probs_to_write, uint32_t* ui32_probs_buffer) {
    uint32_t dim0, dim1, dim2;
    uint32_t i, j, address;
    uint32_t src_tab_offset = 0;

    address = 0;

    for(dim2 = 0; dim2 < 4; dim2++) {
        for(dim1 = 0; dim1 < 8; dim1++) {
            for(dim0 = 0; dim0 < 3; dim0++) { 
                j =0;
                for (i =0 ; i < CABAC_LSR_CoefficientProb_Stride ; i += 4) {
                    *(ui32_probs_buffer + address + j++ ) =
                          (uint32_t)ui8_probs_to_write[CABAC_LSR_CoefficientProb_ToIdxMap[i] + src_tab_offset]
                        | (uint32_t)ui8_probs_to_write[CABAC_LSR_CoefficientProb_ToIdxMap[i+1] + src_tab_offset] << 8
                        | (uint32_t)ui8_probs_to_write[CABAC_LSR_CoefficientProb_ToIdxMap[i+2] + src_tab_offset] << 16
                        | (uint32_t)ui8_probs_to_write[CABAC_LSR_CoefficientProb_ToIdxMap[i+3] + src_tab_offset] << 24;
                    
                }
                /* increment the address by the stride */
                address +=  (CABAC_LSR_CoefficientProb_Stride >> 2);
                /* increment source table offset */
                src_tab_offset += CABAC_LSR_CoefficientProb_Valid;
            }
        }
    }
}

/***********************************************************************************
* Description        : programme the DMA to send probability data.
************************************************************************************/
static void tng__VP8_set_probility_reg(context_VP8_p ctx) {
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    uint32_t *probs_buffer_1stPart , *probs_buffer_2ndPart;
    //uint32_t* probs_table_1stbuffer = NULL;

    /* First write the data for the first partition */
    /* Write the probability data in the probability data buffer */
    psb_buffer_map(&ctx->probability_data_1st_part, (unsigned char **)&probs_buffer_1stPart);
    if(NULL == probs_buffer_1stPart) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "tng__VP8_set_probility_reg: map buffer fail\n");
        return;
    }
    {
        memset(probs_buffer_1stPart, 0, sizeof(ctx->probability_data_1st_part));
        if(ctx->pic_params->pic_fields.bits.key_frame == 0) {
            tng_KeyFrame_BModeProbsDataCompile((uint8_t *)b_mode_prob, probs_buffer_1stPart);
        } else {
            tng_InterFrame_YModeProbsDataCompile(ctx->pic_params->y_mode_probs, probs_buffer_1stPart);

            probs_buffer_1stPart += ( CABAC_LSR_InterFrame_UVModeProb_Address >> 2);
            tng_InterFrame_UVModeProbsDataCompile(ctx->pic_params->uv_mode_probs, probs_buffer_1stPart);

            probs_buffer_1stPart += (CABAC_LSR_InterFrame_MVContextProb_Address >>2) - ( CABAC_LSR_InterFrame_UVModeProb_Address >> 2);
            tng_InterFrame_MVContextProbsDataCompile((uint8_t *)ctx->pic_params->mv_probs, probs_buffer_1stPart);
        }

        psb_buffer_unmap(&ctx->probability_data_1st_part);
        psb_cmdbuf_dma_write_cmdbuf(cmdbuf, &ctx->probability_data_1st_part, 0,
                                    ctx->probability_data_1st_part_size, 0,
                                    DMA_TYPE_PROBABILITY_DATA);
    }
    
    /* Write the probability data for the second partition and create a linked list */ 
    psb_buffer_map(&ctx->probability_data_2nd_part, (unsigned char **)&probs_buffer_2ndPart);
    if(NULL == probs_buffer_2ndPart) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "tng__VP8_set_probility_reg: map buffer fail\n");
        return;
    }

    {
        memset(probs_buffer_2ndPart, 0, sizeof(ctx->probability_data_2nd_part));
        /* for any other partition */
        tng_DCT_Coefficient_ProbsDataCompile((Probability *)ctx->probs_params->dct_coeff_probs, probs_buffer_2ndPart);

        psb_buffer_unmap(&ctx->probability_data_2nd_part);

    }
}

static void tng__VP8_begin_slice(context_DEC_p dec_ctx, VASliceParameterBufferBase *vld_slice_param)
{
    VASliceParameterBufferVP8 *slice_param = (VASliceParameterBufferVP8 *) vld_slice_param;
    context_VP8_p ctx = (context_VP8_p)dec_ctx;

    dec_ctx->bits_offset = slice_param->macroblock_offset;
    ctx->slice_params = slice_param;
    /* dec_ctx->SR_flags = 0; */
}

static void tng__VP8_process_slice_data(context_DEC_p dec_ctx, VASliceParameterBufferBase *vld_slice_param)
{
    VASliceParameterBufferVP8 *slice_param = (VASliceParameterBufferVP8 *) vld_slice_param;
    context_VP8_p ctx = (context_VP8_p)dec_ctx;

    tng__CMDS_registers_write(ctx);
    tng__VP8_FE_Registers_Write(ctx);
    tng__VP8_BE_Registers_Write(ctx);
    tng__VP8_set_slice_param(ctx);
    tng__VP8_set_probility_reg(ctx);
    tng__VP8_set_target_picture(ctx);
    tng__VP8_set_reference_picture(ctx);
    tng__VP8_set_bool_coder_context(ctx);
}

static void tng__VP8_end_slice(context_DEC_p dec_ctx)
{
    context_VP8_p ctx = (context_VP8_p)dec_ctx;

#ifdef PSBVIDEO_MSVDX_EC
    if (ctx->obj_context->driver_data->ec_enabled)
        ctx->obj_context->flags |= (FW_ERROR_DETECTION_AND_RECOVERY); /* FW_ERROR_DETECTION_AND_RECOVERY */
#endif

    ctx->obj_context->first_mb = (*(ctx->dec_ctx.slice_first_pic_last) >> 16);
    ctx->obj_context->last_mb = (*(ctx->dec_ctx.slice_first_pic_last) & 0xffff);
}

static VAStatus tng_VP8_BeginPicture(
    object_context_p obj_context) {
    INIT_CONTEXT_VP8

    if (ctx->pic_params) {
        free(ctx->pic_params);
        ctx->pic_params = NULL;
    }

    if (ctx->probs_params) {
        free(ctx->probs_params);
        ctx->probs_params = NULL;
    }

    if (ctx->iq_params) {
	    free(ctx->iq_params);
	    ctx->iq_params = NULL;
    }
    /* ctx->table_stats[VP8_VLC_NUM_TABLES-1].size = 16; */
    ctx->slice_count = 0;

    return VA_STATUS_SUCCESS;
}

#ifdef PSBVIDEO_MSVDX_EC
static void tng__VP8_choose_ec_frames(context_VP8_p ctx)
{
    ctx->obj_context->ec_target = NULL;
    if (ctx->pic_params == NULL)
        return;

    if (ctx->pic_params->pic_fields.bits.key_frame == 0)
    {
        /* To conceal key frame using golden referece frame */
        ctx->obj_context->ec_target = ctx->golden_ref_picture;
        /* In case the next frame is an I frame we will need this */
        ctx->obj_context->ec_candidate = ctx->obj_context->current_render_target;
    } else {
        /* To conceal inter frame using last reference frame */
        ctx->obj_context->ec_target = ctx->last_ref_picture;
    }

    /* Otherwise we conceal from the previous I or P frame*/
    if (!ctx->obj_context->ec_target)
    {
        ctx->obj_context->ec_target = ctx->obj_context->ec_candidate;
    }

    if (!ctx->obj_context->ec_target) {
        ctx->obj_context->ec_target = ctx->obj_context->current_render_target;
    }
}
#endif

static VAStatus tng_VP8_process_buffer(
    context_DEC_p dec_ctx,
    object_buffer_p buffer) {
    context_VP8_p ctx = (context_VP8_p)dec_ctx;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    object_buffer_p obj_buffer = buffer;

        switch (obj_buffer->type) {
        case VAPictureParameterBufferType:
            vaStatus = tng__VP8_process_picture_param(ctx, obj_buffer);
            DEBUG_FAILURE;
            break;

        case VAProbabilityBufferType:
            vaStatus = tng__VP8_process_probility_param(ctx, obj_buffer);
            DEBUG_FAILURE;
            break;

	case VAIQMatrixBufferType:
	    vaStatus = tng__VP8_process_iq_matrix(ctx, obj_buffer);
	    DEBUG_FAILURE;
	    break;

        default:
            vaStatus = VA_STATUS_ERROR_UNKNOWN;
            DEBUG_FAILURE;
        }

    return vaStatus;
}

static VAStatus tng_VP8_EndPicture(
    object_context_p obj_context) {
    INIT_CONTEXT_VP8
    psb_surface_p    target_surface = ctx->obj_context->current_render_target->psb_surface;
    psb_driver_data_p driver_data = obj_context->driver_data;
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    object_surface_p obj_surface = NULL;
    if (!obj_surface)
        obj_surface = obj_context->current_render_target;

#ifdef PSBVIDEO_MSVDX_EC
    if (driver_data->ec_enabled) {
        uint32_t rotation_flags = 0;
        uint32_t ext_stride_a = 0;
        object_surface_p ec_target;

        tng__VP8_choose_ec_frames(ctx);
        ec_target = ctx->obj_context->ec_target;
        //ec_target = obj_surface;
        REGIO_WRITE_FIELD_LITE(ext_stride_a, MSVDX_CMDS, EXTENDED_ROW_STRIDE, EXT_ROW_STRIDE, target_surface->stride / 64);

        /* FIXME ec ignor rotate condition */
        if(ec_target) {
	    if (psb_context_get_next_cmdbuf(ctx->obj_context)) {
                vaStatus = VA_STATUS_ERROR_UNKNOWN;
                DEBUG_FAILURE;
                return vaStatus;
            }

            if (psb_context_submit_host_be_opp(ctx->obj_context,
                                          &target_surface->buf,
                                          &ec_target->psb_surface->buf,
                                          NULL,
                                          (ctx->pic_params->frame_width+15)/16,
                                          (ctx->pic_params->frame_height+15)/16,
                                          rotation_flags,
                                          2,//ctx->field_type,
                                          ext_stride_a,
                                          target_surface->chroma_offset + target_surface->buf.buffer_ofs,
                                          ec_target->psb_surface->chroma_offset + ec_target->psb_surface->buf.buffer_ofs)) {
                return VA_STATUS_ERROR_UNKNOWN;
            }
        }
    }
#endif

    if (psb_context_flush_cmdbuf(ctx->obj_context)) {
	    return VA_STATUS_ERROR_UNKNOWN;
    }

    if (ctx->pic_params) {
        free(ctx->pic_params);
        ctx->pic_params = NULL;
    }

    if (ctx->probs_params) {
        free(ctx->probs_params);
        ctx->probs_params = NULL;
    }

    if (ctx->iq_params) {
	free(ctx->iq_params);
	ctx->iq_params = NULL;
    }

    return VA_STATUS_SUCCESS;
}


struct format_vtable_s tng_VP8_vtable = {
queryConfigAttributes:
    tng_VP8_QueryConfigAttributes,
validateConfig:
    tng_VP8_ValidateConfig,
createContext:
    tng_VP8_CreateContext,
destroyContext:
    tng_VP8_DestroyContext,
beginPicture:
    tng_VP8_BeginPicture,
renderPicture:
    vld_dec_RenderPicture,
endPicture:
    tng_VP8_EndPicture
};


