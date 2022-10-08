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
 *    Guo Nana <nana.n.guo@intel.com>
 *    Zeng Li <li.zeng@intel.com>
 *
 */

#include "va/va_dec_jpeg.h"
#include "tng_jpegdec.h"
#include "tng_vld_dec.h"
#include "psb_def.h"
#include "psb_drv_debug.h"

#include "hwdefs/reg_io2.h"
#include "hwdefs/msvdx_offsets.h"
#include "hwdefs/msvdx_cmds_io2.h"
#include "hwdefs/msvdx_vec_reg_io2.h"
#include "hwdefs/msvdx_vec_jpeg_reg_io2.h"
#include "hwdefs/dxva_fw_ctrl.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define GET_SURFACE_INFO_is_defined(psb_surface) ((int) (psb_surface->extra_info[0]))
#define SET_SURFACE_INFO_is_defined(psb_surface, val) psb_surface->extra_info[0] = (uint32_t) val;
#define GET_SURFACE_INFO_picture_structure(psb_surface) (psb_surface->extra_info[1])
#define SET_SURFACE_INFO_picture_structure(psb_surface, val) psb_surface->extra_info[1] = val;
#define GET_SURFACE_INFO_picture_coding_type(psb_surface) ((int) (psb_surface->extra_info[2]))
#define SET_SURFACE_INFO_picture_coding_type(psb_surface, val) psb_surface->extra_info[2] = (uint32_t) val;

#define JPEG_MAX_SETS_HUFFMAN_TABLES 2
#define JPEG_MAX_QUANT_TABLES 4

#define TABLE_CLASS_DC  0
#define TABLE_CLASS_AC  1
#define TABLE_CLASS_NUM 2

#define JPEG_PROFILE_BASELINE 0

/************************************/
/* VLC table defines and structures */

/* max number of bits allowed in a VLC code */
#define JPG_VLC_MAX_CODE_LEN        (16)

/* max num bits to decode in any one decode direct operation */
#define JPG_VLC_MAX_DIRECT_WIDTH    (6)

static const uint8_t inverse_zigzag[64] =
{
    0x00, 0x01, 0x05, 0x06, 0x0e, 0x0f, 0x1b, 0x1c,
    0x02, 0x04, 0x07, 0x0D, 0x10, 0x1a, 0x1d, 0x2a,
    0x03, 0x08, 0x0C, 0x11, 0x19, 0x1e, 0x29, 0x2b,
    0x09, 0x0B, 0x12, 0x18, 0x1f, 0x28, 0x2c, 0x35,
    0x0A, 0x13, 0x17, 0x20, 0x27, 0x2d, 0x34, 0x36,
    0x14, 0x16, 0x21, 0x26, 0x2e, 0x33, 0x37, 0x3c,
    0x15, 0x22, 0x25, 0x2f, 0x32, 0x38, 0x3b, 0x3d,
    0x23, 0x24, 0x30, 0x31, 0x39, 0x3a, 0x3e, 0x3f
};


/*
******************************************************************************

 This structure defines the VLC code used for a partiular symbol

******************************************************************************/
typedef struct
{
    uint16_t code;    // VLC code with valid data in top-most bits
    uint8_t code_length;   // VLC code length
    uint8_t symbol;

} vlc_symbol_code_jpeg; // VLCSymbolCodeJPEG


/*
******************************************************************************

 This structure describes a set of VLC codes for a particular Huffman tree

******************************************************************************/
typedef struct
{
    uint32_t num_codes;
    uint32_t min_len;
    uint32_t max_len;

} vlc_symbol_stats_jpeg; // VLCSymbolStatsJPEG


/*
******************************************************************************

 This structure describes the generated VLC code table

******************************************************************************/
typedef struct
{
    uint32_t size;
    uint32_t initial_width;
    uint32_t initial_opcode;

} vlc_table_stats_jpeg; // VLCTableStatsJPEG


/**************************************/
/* JPEG VLC Table defines and OpCodes */

#define JPG_MAKE_MASK(X) ((1<<(X))-1)

#define JPG_INSTR_OP_CODE_WIDTH (3)
#define JPG_INSTR_SHIFT_WIDTH   (3)
#define JPG_INSTR_OFFSET_WIDTH  (9)

#define JPG_INSTR_OP_CODE_MASK  JPG_MAKE_MASK(JPG_JPG_INSTR_OP_CODE_WIDTH)
#define JPG_INSTR_SHIFT_MASK    JPG_MAKE_MASK(JPG_INSTR_SHIFT_WIDTH)
#define JPG_INSTR_OFFSET_MASK   JPG_MAKE_MASK(JPG_INSTR_OFFSET_WIDTH)


#define JPG_MAKE_OFFSET(code,width,leading) \
(((code<<leading) & JPG_MAKE_MASK(JPG_VLC_MAX_CODE_LEN)) >> (JPG_VLC_MAX_CODE_LEN-width))

typedef enum {
    JPG_OP_DECODE_DIRECT = 0,
    JPG_OP_DECODE_LEADING_1,
    JPG_OP_DECODE_LEADING_0,

    JPG_OP_CODE_INVALID,

    JPG_OP_VALID_SYMBOL,
    JPG_OP_VALID_RANGE_EVEN,
    JPG_OP_VALID_RANGE_ODD,
    JPG_OP_VALID_RANGE_EVEN_SET_FLAG,

} vlc_op_code_jpeg; // VLCOpCodeJPEG

/**************************************/

struct context_JPEG_s {
    struct context_DEC_s dec_ctx;
    object_context_p obj_context; /* back reference */

    uint32_t profile;

    /* Picture parameters */
    VAPictureParameterBufferJPEGBaseline *pic_params;

    uint32_t display_picture_width;    /* in pixels */
    uint32_t display_picture_height;    /* in pixels */

    uint32_t coded_picture_width;    /* in pixels */
    uint32_t coded_picture_height;    /* in pixels */

    uint32_t MCU_width;
    uint32_t MCU_height;
    uint32_t size_mb;                /* in macroblocks */
    uint32_t picture_width_mb;        /* in macroblocks */
    uint32_t picture_height_mb;        /* in macroblocks */

    uint8_t max_scalingH;
    uint8_t max_scalingV;

    /* VLC packed data */
    struct psb_buffer_s vlc_packed_table;

    uint32_t vlctable_buffer_size;
    uint32_t rendec_qmatrix[JPEG_MAX_QUANT_TABLES][16];

    /* Huffman table information as parsed from the bitstream */
    vlc_symbol_code_jpeg* symbol_codes[TABLE_CLASS_NUM][JPEG_MAX_SETS_HUFFMAN_TABLES];
    vlc_symbol_stats_jpeg symbol_stats[TABLE_CLASS_NUM][JPEG_MAX_SETS_HUFFMAN_TABLES];

    /*  Huffman table information compiled for the hardware */
    uint32_t huffman_table_space;
    uint16_t* huffman_table_RAM;
    vlc_table_stats_jpeg table_stats[TABLE_CLASS_NUM][JPEG_MAX_SETS_HUFFMAN_TABLES];
};

typedef struct context_JPEG_s *context_JPEG_p;

#define INIT_CONTEXT_JPEG    context_JPEG_p ctx = (context_JPEG_p) obj_context->format_data;

#define SURFACE(id)    ((object_surface_p) object_heap_lookup( &ctx->obj_context->driver_data->surface_heap, id ))

static void tng_JPEG_QueryConfigAttributes(
    VAProfile __maybe_unused profile,
    VAEntrypoint __maybe_unused entrypoint,
    VAConfigAttrib __maybe_unused * attrib_list,
    int __maybe_unused num_attribs) {
    /* No JPEG specific attributes */
}

static VAStatus tng_JPEG_ValidateConfig(object_config_p obj_config) {
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

static VAStatus tng__JPEG_check_legal_picture(object_context_p obj_context, object_config_p obj_config) {
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

    switch (obj_config->profile) {
    case VAProfileJPEGBaseline:
        if ((obj_context->picture_width <= 0) || (obj_context->picture_height <= 0)) {
            vaStatus = VA_STATUS_ERROR_RESOLUTION_NOT_SUPPORTED;
        }
        break;

    default:
        vaStatus = VA_STATUS_ERROR_UNSUPPORTED_PROFILE;
        break;
    }

    return vaStatus;
}

static void tng_JPEG_DestroyContext(object_context_p obj_context);

static void tng__JPEG_process_slice_data(context_DEC_p dec_ctx, VASliceParameterBufferBase *vld_slice_param);
static void tng__JPEG_end_slice(context_DEC_p dec_ctx);
static void tng__JPEG_begin_slice(context_DEC_p dec_ctx, VASliceParameterBufferBase *vld_slice_param);
static VAStatus tng_JPEG_process_buffer(context_DEC_p dec_ctx, object_buffer_p buffer);

static VAStatus tng_JPEG_CreateContext(
    object_context_p obj_context,
    object_config_p obj_config) {
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    context_JPEG_p ctx;

    /* Validate flag */
    /* Validate picture dimensions */
    vaStatus = tng__JPEG_check_legal_picture(obj_context, obj_config);
    if (VA_STATUS_SUCCESS != vaStatus) {
        DEBUG_FAILURE;
        return vaStatus;
    }

    ctx = (context_JPEG_p) calloc(1, sizeof(struct context_JPEG_s));
    if (NULL == ctx) {
        vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
        DEBUG_FAILURE;
        return vaStatus;
    }
    obj_context->format_data = (void*) ctx;
    ctx->obj_context = obj_context;
    ctx->pic_params = NULL;

    ctx->dec_ctx.begin_slice = tng__JPEG_begin_slice;
    ctx->dec_ctx.process_slice = tng__JPEG_process_slice_data;
    ctx->dec_ctx.end_slice = tng__JPEG_end_slice;
    ctx->dec_ctx.process_buffer = tng_JPEG_process_buffer;
    ctx->dec_ctx.preload_buffer = NULL;

    switch (obj_config->profile) {
    case VAProfileJPEGBaseline:
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "JPEG_PROFILE_BASELINE\n");
        ctx->profile = JPEG_PROFILE_BASELINE;
        break;

    default:
        ASSERT(0 == 1);
        vaStatus = VA_STATUS_ERROR_UNKNOWN;
    }

    ctx->vlctable_buffer_size = 1984 * 2;
    if (vaStatus == VA_STATUS_SUCCESS) {
        vaStatus = psb_buffer_create(obj_context->driver_data,
                                     ctx->vlctable_buffer_size,
                                     psb_bt_cpu_vpu,
                                     &ctx->vlc_packed_table);
        DEBUG_FAILURE;
    }

    if (vaStatus == VA_STATUS_SUCCESS) {
        vaStatus = vld_dec_CreateContext(&ctx->dec_ctx, obj_context);
        DEBUG_FAILURE;
    }

    if (vaStatus != VA_STATUS_SUCCESS) {
        tng_JPEG_DestroyContext(obj_context);
    }

    return vaStatus;
}

static void tng_JPEG_DestroyContext(
    object_context_p obj_context) {
    INIT_CONTEXT_JPEG
    int i;

    vld_dec_DestroyContext(&ctx->dec_ctx);

    psb_buffer_destroy(&ctx->vlc_packed_table);

    if (ctx->pic_params) {
        free(ctx->pic_params);
        ctx->pic_params = NULL;
    }

    if (ctx->symbol_codes[0][0]) {
        free(ctx->symbol_codes[0][0]);
        ctx->symbol_codes[0][0] = NULL;
    }
    if (ctx->symbol_codes[0][1]) {
        free(ctx->symbol_codes[0][1]);
        ctx->symbol_codes[0][1] = NULL;
    }
    if (ctx->symbol_codes[1][0]) {
        free(ctx->symbol_codes[1][0]);
        ctx->symbol_codes[1][0] = NULL;
    }
    if (ctx->symbol_codes[1][1]) {
        free(ctx->symbol_codes[1][1]);
        ctx->symbol_codes[1][1] = NULL;
    }
    free(obj_context->format_data);
    obj_context->format_data = NULL;
}

static uint16_t jpg_vlc_valid_symbol(const vlc_symbol_code_jpeg * symbol_code, const uint32_t leading) {
    uint16_t entry = 0;
    IMG_ASSERT( (symbol_code->code_length - leading - 1) >= 0 );

    /* VLC microcode entry for a valid symbol */
    entry |= (JPG_OP_VALID_SYMBOL << (JPG_INSTR_SHIFT_WIDTH+JPG_INSTR_OFFSET_WIDTH));
    entry |= ((symbol_code->code_length - leading - 1) << JPG_INSTR_OFFSET_WIDTH);
    entry |= symbol_code->symbol;
    return entry;
}

static uint32_t
jpg_vlc_write_direct_command(uint16_t * const vlc_ram, uint32_t result_offset) {
    uint32_t width = 0x7fff & *vlc_ram;
    uint16_t entry = 0;

    /* check that the max width read from the VLC entry is valid */
    IMG_ASSERT( 0x8000 & *vlc_ram );

    /* limit to the maximum width for this algorithm */
    width = (width > JPG_VLC_MAX_DIRECT_WIDTH)? JPG_VLC_MAX_DIRECT_WIDTH: width;

    /* VLC microcode for decode direct command */
    entry |= (JPG_OP_DECODE_DIRECT << (JPG_INSTR_SHIFT_WIDTH+JPG_INSTR_OFFSET_WIDTH));
    entry |= ((width - 1) << JPG_INSTR_OFFSET_WIDTH);
    entry |= result_offset;

    /* write command */
    *vlc_ram = entry;

    return width;
}

static uint32_t
jpg_vlc_get_offset(const vlc_symbol_code_jpeg * symbol_code, uint32_t width, uint32_t leading) {
    uint32_t offset;

    /* lose bits already decoded */
    offset = symbol_code->code << leading;
    offset &= JPG_MAKE_MASK(JPG_VLC_MAX_CODE_LEN);
    /* convert truncated code to offset */
    offset >>= (JPG_VLC_MAX_CODE_LEN - width);
    return offset;
}

static uint32_t
jpg_vlc_decode_direct_symbols(const vlc_symbol_code_jpeg * symbol_code, uint16_t * const table_ram, const uint32_t width,
    const uint32_t leading) {
    uint32_t offset, limit;
    uint16_t entry;

    /* this function is only for codes short enough to produce valid symbols */
    IMG_ASSERT( symbol_code->code_length <= leading + width );
    IMG_ASSERT( symbol_code->code_length > leading );

    /* lose bits already decoded */
    offset = symbol_code->code << leading;
    offset &= JPG_MAKE_MASK(JPG_VLC_MAX_CODE_LEN);

    /* convert truncated code to offset */
    offset >>= (JPG_VLC_MAX_CODE_LEN - width);
    /* expand offset to encorporate undefined code bits */
    limit = offset + (1 << (width - (symbol_code->code_length - leading)));

    /* for all code variants - insert symbol into the decode direct result table */
    entry = jpg_vlc_valid_symbol( symbol_code, leading );
    for (; offset < limit; offset++) {
        table_ram[offset] = entry;
    }

    /* return the number of entries written */
    return limit - offset - 1;
}

static uint32_t
jpg_vlc_decode_direct(
    const vlc_symbol_code_jpeg * symbol_codes,
    const uint32_t num_codes,
    uint16_t * const table_ram,
    const uint32_t direct_width,
    const uint32_t leading_width,
    const uint32_t leading_pattern) {
    const uint32_t next_leading_width = leading_width + direct_width;
    const uint32_t next_leading_mask = JPG_MAKE_MASK(next_leading_width) << (JPG_VLC_MAX_CODE_LEN-next_leading_width);
    const uint32_t leading_mask = JPG_MAKE_MASK(leading_width) << (JPG_VLC_MAX_CODE_LEN-leading_width);

    uint32_t num_vlc_ops = 1 << direct_width;
    uint32_t next_section, next_width, next_leading_pattern;
    uint32_t offset;
    uint32_t i;

    /* sanity - check this decode direct will not exceed the max code len */
    IMG_ASSERT(next_leading_width <= JPG_VLC_MAX_CODE_LEN);

    /* set all VLC ops for this decode direct to invalid */
    for (i = 0; i < num_vlc_ops; i++) {
        table_ram[i] = (JPG_OP_CODE_INVALID << (JPG_INSTR_SHIFT_WIDTH+JPG_INSTR_OFFSET_WIDTH));
    }

    /* iterate over code table and insert VLC ops until */
    /* codes become too long for this iteration or we run out of codes */
    for (i = 0; symbol_codes[i].code_length <= next_leading_width && i < num_codes; i++) {
        /* only use codes that match the specified leading portion */
        if ((((uint32_t)symbol_codes[i].code) & leading_mask) == leading_pattern) {
            jpg_vlc_decode_direct_symbols(&symbol_codes[i], table_ram, direct_width, leading_width );
        }
    }
    next_section = i;

    /* assign the longest code length for each remaining entry */
    for (i = next_section; i < num_codes; i++) {
        /* only use codes that match the specified leading portion */
        if ((((uint32_t)symbol_codes[i].code) & leading_mask) == leading_pattern) {
            /* enable the unused VLC bit to indicate this is not a command */
            offset = jpg_vlc_get_offset(&symbol_codes[i], direct_width, leading_width);
            table_ram[offset] = 0x8000 | (symbol_codes[i].code_length - next_leading_width);
        }
    }

    /* for the remaining (long) codes */
    for (i = next_section; i < num_codes; i++) {
        /* only use codes that match the specified leading portion */
        if ((((uint32_t)symbol_codes[i].code) & leading_mask) == leading_pattern) {
            /* if a command has not been written for this direct offset */
            offset = jpg_vlc_get_offset(&symbol_codes[i], direct_width, leading_width);
            if (table_ram[offset] & 0x8000) {
                /* write command the decode direct command */
                next_width = jpg_vlc_write_direct_command(
                        &table_ram[offset],
                        num_vlc_ops - offset );

                next_leading_pattern = (uint32_t)symbol_codes[i].code & next_leading_mask;

                /* decode direct recursive call */
                num_vlc_ops += jpg_vlc_decode_direct(
                    &symbol_codes[i],
                    num_codes - i,
                    &table_ram[num_vlc_ops],
                    next_width,
                    next_leading_width,
                    next_leading_pattern);
            }
        }
    }
    return num_vlc_ops;
}

static void JPG_VLC_CompileTable(
    const vlc_symbol_code_jpeg * symbol_codes,
    const vlc_symbol_stats_jpeg * psSymbolStats,
    const uint32_t __maybe_unused ram_size,
    uint16_t * table_ram,
    vlc_table_stats_jpeg * ptable_stats)
{
    ptable_stats->initial_width = 5;
    ptable_stats->initial_opcode = JPG_OP_DECODE_DIRECT;

    ptable_stats->size = jpg_vlc_decode_direct(
        symbol_codes,
        psSymbolStats->num_codes,
        table_ram,
        JPG_VLC_MAX_DIRECT_WIDTH,
        0,
        0);

    IMG_ASSERT( ptable_stats->size <= ram_size );
}

static void compile_huffman_tables(context_JPEG_p ctx) {
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    ctx->huffman_table_space = 1984;

    if (0 == psb_buffer_map(&ctx->vlc_packed_table, (unsigned char **)&ctx->huffman_table_RAM)) {
        // Compile Tables
        uint32_t table_class;
        uint32_t table_id;
        for (table_class = 0; table_class < TABLE_CLASS_NUM; table_class++) {
            for (table_id = 0; table_id < JPEG_MAX_SETS_HUFFMAN_TABLES; table_id++) {
                if (ctx->symbol_stats[table_class][table_id].num_codes) {
                    JPG_VLC_CompileTable(ctx->symbol_codes[table_class][table_id],
                        &ctx->symbol_stats[table_class][table_id], ctx->huffman_table_space, ctx->huffman_table_RAM,
                        &ctx->table_stats[table_class][table_id]);
                    ctx->huffman_table_space -= ctx->table_stats[table_class][table_id].size;
                    ctx->huffman_table_RAM += ctx->table_stats[table_class][table_id].size;
                }

            }
        }
        psb_buffer_unmap(&ctx->vlc_packed_table);
    } else {
        vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
        DEBUG_FAILURE;
    }

}

static VAStatus tng__JPEG_process_picture_param(context_JPEG_p ctx, object_buffer_p obj_buffer) {
    VAStatus vaStatus;
    ASSERT(obj_buffer->type == VAPictureParameterBufferType);
    ASSERT(obj_buffer->num_elements == 1);
    ASSERT(obj_buffer->size == sizeof(VAPictureParameterBufferJPEGBaseline));

    if ((obj_buffer->num_elements != 1) ||
        (obj_buffer->size != sizeof(VAPictureParameterBufferJPEGBaseline))) {
        return VA_STATUS_ERROR_UNKNOWN;
    }

    /* Transfer ownership of VAPictureParameterBufferJPEGBaseline data */
    if (ctx->pic_params) {
        free(ctx->pic_params);
    }
    ctx->pic_params = (VAPictureParameterBufferJPEGBaseline *) obj_buffer->buffer_data;
    ctx->display_picture_width = ctx->pic_params->picture_width;
    ctx->display_picture_height = ctx->pic_params->picture_height;

    ctx->coded_picture_width = ( ctx->display_picture_width + 7 ) & ( ~7 );
    ctx->coded_picture_height = ( ctx->display_picture_height + 7 ) & ( ~7 );
    ctx->max_scalingH = 0;
    ctx->max_scalingV = 0;

    uint8_t component_id;
    for (component_id = 0; component_id < ctx->pic_params->num_components; component_id++) {
        if (ctx->max_scalingH < ctx->pic_params->components->h_sampling_factor)
            ctx->max_scalingH = ctx->pic_params->components->h_sampling_factor;
        if (ctx->max_scalingV < ctx->pic_params->components->v_sampling_factor)
            ctx->max_scalingV = ctx->pic_params->components->v_sampling_factor;
    }

    ctx->MCU_width = (ctx->coded_picture_width + (8 * ctx->max_scalingH) - 1) / (8 * ctx->max_scalingH);
    ctx->MCU_height = (ctx->coded_picture_height + (8 * ctx->max_scalingV) - 1) / (8 * ctx->max_scalingV);

    ctx->picture_width_mb = (ctx->coded_picture_width  + 15) / 16;
    ctx->picture_height_mb = (ctx->coded_picture_height  + 15) / 16;
    ctx->size_mb = ctx->picture_width_mb * ctx->picture_height_mb;


    obj_buffer->buffer_data = NULL;
    obj_buffer->size = 0;


    return VA_STATUS_SUCCESS;
}

static void tng__JPEG_write_qmatrices(context_JPEG_p ctx) {
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    int i;

    psb_cmdbuf_rendec_start(cmdbuf, REG_MSVDX_VEC_IQRAM_OFFSET);

    for (i = 0; i < 16; i++) {
        psb_cmdbuf_rendec_write(cmdbuf, ctx->rendec_qmatrix[0][i]);
    }
    for (i = 0; i < 16; i++) {
        psb_cmdbuf_rendec_write(cmdbuf, ctx->rendec_qmatrix[1][i]);
    }
    for (i = 0; i < 16; i++) {
        psb_cmdbuf_rendec_write(cmdbuf, ctx->rendec_qmatrix[2][i]);
    }
    for (i = 0; i < 16; i++) {
        psb_cmdbuf_rendec_write(cmdbuf, ctx->rendec_qmatrix[3][i]);
    }

    psb_cmdbuf_rendec_end(cmdbuf);
}

static VAStatus tng__JPEG_process_iq_matrix(context_JPEG_p ctx, object_buffer_p obj_buffer) {
    VAIQMatrixBufferJPEGBaseline *qmatrix_data = (VAIQMatrixBufferJPEGBaseline *) obj_buffer->buffer_data;
    ASSERT(obj_buffer->type == VAIQMatrixBufferType);
    ASSERT(obj_buffer->num_elements == 1);
    ASSERT(obj_buffer->size == sizeof(VAIQMatrixBufferJPEGBaseline));

    uint32_t dqt_ind;

    for (dqt_ind = 0; dqt_ind < 4; dqt_ind++) {
        // Reorder Quant table for hardware
        uint32_t table_ind = 0;
        uint32_t rendec_table_ind = 0;
        if (qmatrix_data->load_quantiser_table[dqt_ind]) {
            while(table_ind < 64) {
                ctx->rendec_qmatrix[dqt_ind][rendec_table_ind] =
                        (qmatrix_data->quantiser_table[dqt_ind][inverse_zigzag[table_ind+3]] << 24) |
                        (qmatrix_data->quantiser_table[dqt_ind][inverse_zigzag[table_ind+2]] << 16) |
                        (qmatrix_data->quantiser_table[dqt_ind][inverse_zigzag[table_ind+1]] << 8) |
                        (qmatrix_data->quantiser_table[dqt_ind][inverse_zigzag[table_ind]]);

                table_ind += 4;
                rendec_table_ind++;
            }
        }
    }

    return VA_STATUS_SUCCESS;
}


static void tng__JPEG_write_huffman_tables(context_JPEG_p ctx) {
    uint32_t reg_value;
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;

    // VLC Table
    // Write a LLDMA Cmd to transfer VLD Table data
    psb_cmdbuf_dma_write_cmdbuf(cmdbuf, &ctx->vlc_packed_table, 0,
                                  ctx->vlctable_buffer_size, 0,
                                  DMA_TYPE_VLC_TABLE);

    // Write Table addresses
    psb_cmdbuf_reg_start_block(cmdbuf, 0);
    reg_value = 0;
    uint32_t table_address = 0;
    REGIO_WRITE_FIELD_LITE( reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR0, VLC_TABLE_ADDR0, table_address );
    table_address += ctx->table_stats[0][0].size;
    REGIO_WRITE_FIELD_LITE( reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR0, VLC_TABLE_ADDR1, table_address );
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET( MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR0 ), reg_value);

    reg_value = 0;
    table_address += ctx->table_stats[0][1].size;
    REGIO_WRITE_FIELD_LITE( reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR2, VLC_TABLE_ADDR4, table_address );
    table_address += ctx->table_stats[1][0].size;
    REGIO_WRITE_FIELD_LITE( reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR2, VLC_TABLE_ADDR5, table_address );
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET( MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR2 ), reg_value);
    psb_cmdbuf_reg_end_block(cmdbuf);

    // Write Initial Widths
    psb_cmdbuf_reg_start_block(cmdbuf, 0);
    reg_value = 0;
    REGIO_WRITE_FIELD_LITE( reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_WIDTH0, VLC_TABLE_INITIAL_WIDTH0,
        ctx->table_stats[0][0].initial_width );
    REGIO_WRITE_FIELD_LITE( reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_WIDTH0, VLC_TABLE_INITIAL_WIDTH1,
        ctx->table_stats[0][1].initial_width );
    REGIO_WRITE_FIELD_LITE( reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_WIDTH0, VLC_TABLE_INITIAL_WIDTH4,
        ctx->table_stats[1][0].initial_width );
    REGIO_WRITE_FIELD_LITE( reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_WIDTH0, VLC_TABLE_INITIAL_WIDTH5,
        ctx->table_stats[1][1].initial_width );
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET( MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_WIDTH0 ), reg_value);
    psb_cmdbuf_reg_end_block(cmdbuf);

    // Write Initial Opcodes
    psb_cmdbuf_reg_start_block(cmdbuf, 0);
    reg_value = 0;
    REGIO_WRITE_FIELD_LITE( reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_OPCODE0, VLC_TABLE_INITIAL_OPCODE0,
        ctx->table_stats[0][0].initial_opcode );
    REGIO_WRITE_FIELD_LITE( reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_OPCODE0, VLC_TABLE_INITIAL_OPCODE1,
        ctx->table_stats[0][1].initial_opcode );
    REGIO_WRITE_FIELD_LITE( reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_OPCODE0, VLC_TABLE_INITIAL_OPCODE4,
        ctx->table_stats[1][0].initial_opcode );
    REGIO_WRITE_FIELD_LITE( reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_OPCODE0, VLC_TABLE_INITIAL_OPCODE5,
        ctx->table_stats[1][1].initial_opcode );
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET( MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_OPCODE0 ), reg_value);
    psb_cmdbuf_reg_end_block(cmdbuf);

}

static VAStatus tng__JPEG_process_huffman_tables(context_JPEG_p ctx, object_buffer_p obj_buffer) {

    VAHuffmanTableBufferJPEGBaseline *huff = (VAHuffmanTableBufferJPEGBaseline *) obj_buffer->buffer_data;
    ASSERT(obj_buffer->type == VAHuffmanTableBufferType);
    ASSERT(obj_buffer->num_elements == 1);
    ASSERT(obj_buffer->size == sizeof(VAHuffmanTableBufferJPEGBaseline));

    uint32_t table_id;
    for (table_id = 0; table_id < JPEG_MAX_SETS_HUFFMAN_TABLES; table_id++) {
        // Find out the number of entries in the table
        uint32_t table_entries = 0;
        uint32_t bit_ind;
        for (bit_ind = 0; bit_ind < 16; bit_ind++) {
            table_entries += huff->huffman_table[table_id].num_dc_codes[bit_ind];
        }

        ctx->symbol_codes[0][table_id] =(vlc_symbol_code_jpeg *)malloc(sizeof(vlc_symbol_code_jpeg) * table_entries);
        // Parse huffman code sizes
        uint32_t huff_ind = 0;
        for (bit_ind = 0; bit_ind < 16; bit_ind++) {
            uint32_t num_codes = huff->huffman_table[table_id].num_dc_codes[bit_ind];
            while (num_codes) {
                ctx->symbol_codes[0][table_id][huff_ind].code_length = bit_ind + 1;
                num_codes--;
                huff_ind++;
            }
        }

        // Calculate huffman codes

        uint16_t code = 0;
        uint8_t code_size = ctx->symbol_codes[0][table_id][0].code_length;
        huff_ind = 0;

        while (huff_ind < table_entries) {
            if (ctx->symbol_codes[0][table_id][huff_ind].code_length == code_size) {
                ctx->symbol_codes[0][table_id][huff_ind].code = code << ( 16 - code_size );
                huff_ind++;
                code++;
            } else {
                code <<= 1;
                code_size++;
            }
        }

        // Create table of code values
        uint32_t table_ind;
        for (table_ind = 0; table_ind < table_entries; table_ind++) {
            ctx->symbol_codes[0][table_id][table_ind].symbol = huff->huffman_table[table_id].dc_values[table_ind];
        }

        // Store the number of codes in the table
        ctx->symbol_stats[0][table_id].num_codes = table_entries;

        // for AC table
        // Find out the number of entries in the table
        table_entries = 0;
        for (bit_ind = 0; bit_ind < 16; bit_ind++) {
            table_entries += huff->huffman_table[table_id].num_ac_codes[bit_ind];
        }

        // Allocate memory for huffman table codes
        ctx->symbol_codes[1][table_id] = (vlc_symbol_code_jpeg *)malloc(sizeof(vlc_symbol_code_jpeg) * table_entries);

        // Parse huffman code sizes
        huff_ind = 0;
        for (bit_ind = 0; bit_ind < 16; bit_ind++) {
            uint32_t num_codes = huff->huffman_table[table_id].num_ac_codes[bit_ind];
            while (num_codes) {
                ctx->symbol_codes[1][table_id][huff_ind].code_length = bit_ind + 1;
                num_codes--;
                huff_ind++;
            }
        }

        // Calculate huffman codes
        code = 0;
        code_size = ctx->symbol_codes[1][table_id][0].code_length;
        huff_ind = 0;
        while (huff_ind < table_entries) {
            if (ctx->symbol_codes[1][table_id][huff_ind].code_length == code_size) {
                ctx->symbol_codes[1][table_id][huff_ind].code = code << ( 16 - code_size );
                huff_ind++;
                code++;
            } else {
                code <<= 1;
                code_size++;
            }
        }

        // Create table of code values
        for (table_ind = 0; table_ind < table_entries; table_ind++) {
            ctx->symbol_codes[1][table_id][table_ind].symbol = huff->huffman_table[table_id].ac_values[table_ind];;
        }
        // Store the number of codes in the table
        ctx->symbol_stats[1][table_id].num_codes = table_entries;
    }

    compile_huffman_tables(ctx);

    return VA_STATUS_SUCCESS;
}


static void tng__JPEG_set_operating_mode(context_JPEG_p ctx) {
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    ctx->obj_context->operating_mode = 0;

    REGIO_WRITE_FIELD_LITE(ctx->obj_context->operating_mode, MSVDX_CMDS, OPERATING_MODE, USE_EXT_ROW_STRIDE, 1 );
    REGIO_WRITE_FIELD_LITE(ctx->obj_context->operating_mode, MSVDX_CMDS, OPERATING_MODE, ASYNC_MODE, 1 );
    REGIO_WRITE_FIELD_LITE(ctx->obj_context->operating_mode, MSVDX_CMDS, OPERATING_MODE, CHROMA_FORMAT,	1);

    psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET( MSVDX_CMDS, OPERATING_MODE ));

    psb_cmdbuf_rendec_write(cmdbuf, ctx->obj_context->operating_mode);

    psb_cmdbuf_rendec_end(cmdbuf);

}

static void tng__JPEG_set_reference_pictures(context_JPEG_p ctx)
{
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    psb_surface_p target_surface = ctx->obj_context->current_render_target->psb_surface;

    psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, REFERENCE_PICTURE_BASE_ADDRESSES));

    uint32_t planr_size =  target_surface->chroma_offset;
    psb_cmdbuf_rendec_write_address(cmdbuf, &target_surface->buf, target_surface->buf.buffer_ofs);
    psb_cmdbuf_rendec_write_address(cmdbuf, &target_surface->buf, target_surface->buf.buffer_ofs + planr_size);
    psb_cmdbuf_rendec_write_address(cmdbuf, &target_surface->buf, target_surface->buf.buffer_ofs + planr_size * 2);
    // psb_cmdbuf_rendec_write_address(cmdbuf, &target_surface->buf, target_surface->buf.buffer_ofs + planr_size * 3);
    psb_cmdbuf_rendec_end(cmdbuf);
}

static void tng__JPEG_set_ent_dec(context_JPEG_p ctx) {
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    uint32_t reg_value;

    psb_cmdbuf_reg_start_block(cmdbuf, 0);
    reg_value = 0;
    REGIO_WRITE_FIELD_LITE( reg_value, MSVDX_VEC, CR_VEC_ENTDEC_FE_CONTROL, ENTDEC_FE_MODE, 0 );
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET( MSVDX_VEC, CR_VEC_ENTDEC_FE_CONTROL ), reg_value);
    psb_cmdbuf_reg_end_block(cmdbuf);

    psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET( MSVDX_VEC, CR_VEC_ENTDEC_BE_CONTROL ));
    reg_value = 0;
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_ENTDEC_BE_CONTROL, ENTDEC_BE_MODE, 0);
    psb_cmdbuf_rendec_write(cmdbuf, reg_value);
    psb_cmdbuf_rendec_end(cmdbuf);

}

static void tng__JPEG_set_register(context_JPEG_p ctx, VASliceParameterBufferJPEGBaseline *slice_param) {
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    uint32_t reg_value;
    const uint32_t num_MCUs = ctx->MCU_width * ctx->MCU_height;
    const uint32_t num_MCUs_dec = slice_param->restart_interval ? min(slice_param->restart_interval, num_MCUs) :  num_MCUs;

    psb_cmdbuf_reg_start_block(cmdbuf, 0);
    // CR_VEC_JPEG_FE_COMPONENTS
    reg_value = 0;
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_JPEG, CR_VEC_JPEG_FE_COMPONENTS, MAX_V, ctx->max_scalingV);
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_JPEG, CR_VEC_JPEG_FE_COMPONENTS, MAX_H, ctx->max_scalingH);
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_JPEG, CR_VEC_JPEG_FE_COMPONENTS, FE_COMPONENTS,
        slice_param->num_components);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC_JPEG, CR_VEC_JPEG_FE_COMPONENTS ), reg_value);

    // CR_VEC_JPEG_FE_HEIGHT
    reg_value = 0;
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_JPEG, CR_VEC_JPEG_FE_HEIGHT, FE_HEIGHT_MINUS1,
        ctx->coded_picture_height - 1);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC_JPEG, CR_VEC_JPEG_FE_HEIGHT ), reg_value);

    // CR_VEC_JPEG_FE_RESTART_POS
    reg_value = 0;
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC_JPEG, CR_VEC_JPEG_FE_RESTART_POS), reg_value);

    // CR_VEC_JPEG_FE_WIDTH
    reg_value = 0;
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_JPEG, CR_VEC_JPEG_FE_WIDTH, FE_WIDTH_MINUS1,
        ctx->coded_picture_width - 1 );
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC_JPEG, CR_VEC_JPEG_FE_WIDTH), reg_value);

    // CR_VEC_JPEG_FE_ENTROPY_CODING
    reg_value = 0;
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_JPEG, CR_VEC_JPEG_FE_ENTROPY_CODING, NUM_MCUS_LESS1, num_MCUs_dec - 1);

    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_JPEG, CR_VEC_JPEG_FE_ENTROPY_CODING, TA3,
        slice_param->components[3].ac_table_selector);
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_JPEG, CR_VEC_JPEG_FE_ENTROPY_CODING, TD3,
        slice_param->components[3].dc_table_selector);
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_JPEG, CR_VEC_JPEG_FE_ENTROPY_CODING, TA2,
        slice_param->components[2].ac_table_selector);
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_JPEG, CR_VEC_JPEG_FE_ENTROPY_CODING, TD2,
        slice_param->components[2].dc_table_selector);
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_JPEG, CR_VEC_JPEG_FE_ENTROPY_CODING, TA1,
        slice_param->components[1].ac_table_selector);
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_JPEG, CR_VEC_JPEG_FE_ENTROPY_CODING, TD1,
        slice_param->components[1].dc_table_selector );
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_JPEG, CR_VEC_JPEG_FE_ENTROPY_CODING, TA0,
        slice_param->components[0].ac_table_selector);
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_JPEG, CR_VEC_JPEG_FE_ENTROPY_CODING, TD0,
        slice_param->components[0].dc_table_selector);

    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET( MSVDX_VEC_JPEG, CR_VEC_JPEG_FE_ENTROPY_CODING ), reg_value);

    // CR_VEC_JPEG_FE_SCALING
    reg_value = 0;
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_JPEG, CR_VEC_JPEG_FE_SCALING, FE_V3,
        ctx->pic_params->components[3].v_sampling_factor);
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_JPEG, CR_VEC_JPEG_FE_SCALING, FE_H3,
        ctx->pic_params->components[3].h_sampling_factor );
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_JPEG, CR_VEC_JPEG_FE_SCALING, FE_V2,
        ctx->pic_params->components[2].v_sampling_factor);
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_JPEG, CR_VEC_JPEG_FE_SCALING, FE_H2,
        ctx->pic_params->components[2].h_sampling_factor );
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_JPEG, CR_VEC_JPEG_FE_SCALING, FE_V1,
        ctx->pic_params->components[1].v_sampling_factor );
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_JPEG, CR_VEC_JPEG_FE_SCALING, FE_H1,
        ctx->pic_params->components[1].h_sampling_factor );
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_JPEG, CR_VEC_JPEG_FE_SCALING, FE_V0,
        ctx->pic_params->components[0].v_sampling_factor );
    REGIO_WRITE_FIELD_LITE(reg_value, MSVDX_VEC_JPEG, CR_VEC_JPEG_FE_SCALING, FE_H0,
        ctx->pic_params->components[0].h_sampling_factor );

    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET( MSVDX_VEC_JPEG, CR_VEC_JPEG_FE_SCALING ), reg_value);
    psb_cmdbuf_reg_end_block(cmdbuf);

    psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET( MSVDX_VEC_JPEG, CR_VEC_JPEG_BE_HEIGHT ));
    // CR_VEC_JPEG_BE_HEIGHT
    reg_value = 0;
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_JPEG, CR_VEC_JPEG_BE_HEIGHT, BE_HEIGHT_MINUS1,
        ctx->coded_picture_height - 1);
    psb_cmdbuf_rendec_write(cmdbuf, reg_value);

    // CR_VEC_JPEG_BE_WIDTH
    reg_value = 0;
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_JPEG, CR_VEC_JPEG_BE_WIDTH, BE_WIDTH_MINUS1, ctx->coded_picture_width - 1);
    psb_cmdbuf_rendec_write(cmdbuf, reg_value);

    // CR_VEC_JPEG_BE_QUANTISATION
    reg_value = 0;
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_JPEG, CR_VEC_JPEG_BE_QUANTISATION, TQ3,
        ctx->pic_params->components[3].quantiser_table_selector);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_JPEG, CR_VEC_JPEG_BE_QUANTISATION, TQ2,
        ctx->pic_params->components[2].quantiser_table_selector);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_JPEG, CR_VEC_JPEG_BE_QUANTISATION, TQ1,
        ctx->pic_params->components[1].quantiser_table_selector);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_JPEG, CR_VEC_JPEG_BE_QUANTISATION, TQ0,
        ctx->pic_params->components[0].quantiser_table_selector );
    psb_cmdbuf_rendec_write(cmdbuf, reg_value);

    // CR_VEC_JPEG_BE_CONTROL
    reg_value = 0;
    REGIO_WRITE_FIELD( reg_value, MSVDX_VEC_JPEG, CR_VEC_JPEG_BE_CONTROL, RGB, 0 );
    psb_cmdbuf_rendec_write(cmdbuf, reg_value);

    psb_cmdbuf_rendec_end(cmdbuf);
}

static void tng__JPEG_begin_slice(context_DEC_p dec_ctx, VASliceParameterBufferBase __maybe_unused * vld_slice_param)
{
    context_JPEG_p ctx = (context_JPEG_p)dec_ctx;

    dec_ctx->bits_offset = 0;
    dec_ctx->SR_flags = CMD_ENABLE_RBDU_EXTRACTION;
    *dec_ctx->cmd_params |=ctx->MCU_width * ctx->MCU_height;
    *dec_ctx->slice_first_pic_last &= 0xfefe;
    tng__JPEG_write_huffman_tables(ctx);
    tng__JPEG_write_qmatrices(ctx);
}

static void tng__JPEG_process_slice_data(context_DEC_p dec_ctx, VASliceParameterBufferBase *vld_slice_param)
{
    VASliceParameterBufferJPEGBaseline *slice_param = (VASliceParameterBufferJPEGBaseline *) vld_slice_param;
    context_JPEG_p ctx = (context_JPEG_p)dec_ctx;

    tng__JPEG_set_operating_mode(ctx);
    tng__JPEG_set_reference_pictures(ctx);
    vld_dec_setup_alternative_frame(ctx->obj_context);
    tng__JPEG_set_ent_dec(ctx);
    tng__JPEG_set_register(ctx, slice_param);
}

static void tng__JPEG_end_slice(context_DEC_p dec_ctx)
{
    context_JPEG_p ctx = (context_JPEG_p)dec_ctx;

    ctx->obj_context->flags = FW_VA_RENDER_IS_FIRST_SLICE | FW_VA_RENDER_IS_LAST_SLICE | FW_INTERNAL_CONTEXT_SWITCH;
    ctx->obj_context->first_mb = 0;
    ctx->obj_context->last_mb = ((ctx->picture_height_mb - 1) << 8) | (ctx->picture_width_mb - 1);
    *(dec_ctx->slice_first_pic_last) = (ctx->obj_context->first_mb << 16) | ((ctx->obj_context->last_mb) & 0xfefe);

}

static VAStatus tng_JPEG_BeginPicture(
    object_context_p obj_context) {
    INIT_CONTEXT_JPEG

    if (ctx->pic_params) {
        free(ctx->pic_params);
        ctx->pic_params = NULL;
    }

    return VA_STATUS_SUCCESS;
}

static VAStatus tng_JPEG_process_buffer(
    context_DEC_p dec_ctx,
    object_buffer_p buffer) {
    context_JPEG_p ctx = (context_JPEG_p)dec_ctx;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    object_buffer_p obj_buffer = buffer;

    switch (obj_buffer->type) {
    case VAPictureParameterBufferType:
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "tng_JPEG_RenderPicture got VAPictureParameterBuffer\n");
        vaStatus = tng__JPEG_process_picture_param(ctx, obj_buffer);
        DEBUG_FAILURE;
        break;

    case VAIQMatrixBufferType:
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "tng_JPEG_RenderPicture got VAIQMatrixBufferType\n");
        vaStatus = tng__JPEG_process_iq_matrix(ctx, obj_buffer);
        DEBUG_FAILURE;
        break;

    case VAHuffmanTableBufferType:
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "tng_JPEG_RenderPicture got VAIQMatrixBufferType\n");
        vaStatus = tng__JPEG_process_huffman_tables(ctx, obj_buffer);
        DEBUG_FAILURE;
        break;

    default:
        vaStatus = VA_STATUS_ERROR_UNKNOWN;
        DEBUG_FAILURE;
    }

    return vaStatus;
}

static VAStatus tng_JPEG_EndPicture(
    object_context_p obj_context) {
    INIT_CONTEXT_JPEG

    if (psb_context_flush_cmdbuf(ctx->obj_context)) {
        return VA_STATUS_ERROR_UNKNOWN;
    }

    if (ctx->pic_params) {
        free(ctx->pic_params);
        ctx->pic_params = NULL;
    }

    return VA_STATUS_SUCCESS;
}

struct format_vtable_s tng_JPEG_vtable = {
queryConfigAttributes:
    tng_JPEG_QueryConfigAttributes,
validateConfig:
    tng_JPEG_ValidateConfig,
createContext:
    tng_JPEG_CreateContext,
destroyContext:
    tng_JPEG_DestroyContext,
beginPicture:
    tng_JPEG_BeginPicture,
renderPicture:
    vld_dec_RenderPicture,
endPicture:
    tng_JPEG_EndPicture
};
