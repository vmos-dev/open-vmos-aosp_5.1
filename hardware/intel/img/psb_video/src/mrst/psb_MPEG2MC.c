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
 *    Shengquan Yuan  <shengquan.yuan@intel.com>
 *
 */

#include "psb_MPEG2.h"
#include "psb_def.h"
#include "psb_surface.h"
#include "psb_cmdbuf.h"
#include "psb_drv_debug.h"

#include "hwdefs/reg_io2.h"
#include "hwdefs/msvdx_offsets.h"
#include "hwdefs/msvdx_cmds_io2.h"
#include "hwdefs/msvdx_vdmc_reg_io2.h"
#include "hwdefs/msvdx_vec_reg_io2.h"
#include "hwdefs/msvdx_vec_mpeg2_reg_io2.h"
#include "hwdefs/dxva_fw_ctrl.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>


/* TODO: for interlace
 * bit 0: first vector forward
 * bit 1: first vector backward
 * bit 2: second vector forward
 * bit 3: second vector backward
 */
#define MBPARAM_MvertFieldSel_3(ptr)    (((ptr)->motion_vertical_field_select & 0x8) >> 3)
#define MBPARAM_MvertFieldSel_2(ptr)    (((ptr)->motion_vertical_field_select & 0x4) >> 2)
#define MBPARAM_MvertFieldSel_1(ptr)    (((ptr)->motion_vertical_field_select & 0x2) >> 1)
#define MBPARAM_MvertFieldSel_0(ptr)    (((ptr)->motion_vertical_field_select & 0x1) >> 0)
#define MBPARAM_MotionType(ptr)         (((ptr)->macroblock_modes.bits.frame_motion_type << 1) | (ptr)->macroblock_modes.bits.field_motion_type)
#define MBPARAM_MotionBackward(ptr)     (((ptr)->macroblock_type & VA_MB_TYPE_MOTION_BACKWARD)?1:0)
#define MBPARAM_MotionForward(ptr)      (((ptr)->macroblock_type & VA_MB_TYPE_MOTION_FORWARD)?1:0)
#define MBPARAM_IntraMacroblock(ptr)    ((ptr)->macroblock_type & VA_MB_TYPE_MOTION_INTRA )
#define MBPARAM_CodedBlockPattern(ptr)  ((ptr)->coded_block_pattern << 6) /* align with VA code */
#define MBPARAM_MBskipsFollowing(ptr)   ((ptr)->num_skipped_macroblocks)

typedef enum { MB_CODE_TYPE_I , MB_CODE_TYPE_P , MB_CODE_TYPE_B , MB_CODE_TYPE_GMC } eMB_CODE_TYPE;

/* Constants */
#define PICTURE_CODING_I                0x01
#define PICTURE_CODING_P                0x02
#define PICTURE_CODING_B                0x03

#define CODEC_MODE_MPEG2                3
#define CODEC_PROFILE_MPEG2_MAIN        1

/* picture structure */
#define TOP_FIELD                       1
#define BOTTOM_FIELD                    2
#define FRAME_PICTURE                   3

#define INTRA_MB_WORST_CASE             6
#define INTER_MB_WORST_CASE             100

static  const uint32_t  pict_libva2msvdx[] = {0,/* Invalid picture type */
        0,/* I picture */
        1,/* P picture */
        2,/* B pricture */
        3
                                             };/* Invalid picture type */


static  const uint32_t  ft_libva2msvdx[] = {0,/* Invalid picture type   */
        0,/* Top field */
        1,/* Bottom field */
        2
                                           };/* Frame picture     */


struct context_MPEG2MC_s {
    object_context_p obj_context; /* back reference */

    VAMacroblockParameterBufferMPEG2 *mb_param ;        /* Pointer to the mbCtrl structure */
    uint32_t mb_in_buffer;  /* Number of MBs in current buffer */
    uint32_t mb_first_addr; /* MB address of first mb in buffer */

    uint32_t picture_coding_type;
    uint32_t picture_structure;

    uint32_t coded_picture_width;
    uint32_t coded_picture_height;

    uint32_t picture_width_mb; /* in macroblocks */
    uint32_t picture_height_mb; /* in macroblocks */
    uint32_t size_mb; /* in macroblocks */

    VAPictureParameterBufferMPEG2 *pic_params;

    object_surface_p forward_ref_surface;
    object_surface_p backward_ref_surface;

    uint32_t ref_indexA;
    uint32_t ref_indexB;

    uint32_t coded_picture_size;
    uint32_t display_picture_size;
    uint32_t operation_mode;

    uint32_t *lldma_idx; /* Index in command stream for LLDMA pointer */
    uint32_t residual_pendingDMA;
    IMG_INT32   residual_sent;

    psb_buffer_p residual_buf;
    uint32_t blk_in_buffer;/* buffer elements */
    uint32_t blk_size;/* buffer elements size */

    uint32_t fstmb_slice;
};

typedef struct context_MPEG2MC_s *context_MPEG2MC_p;

#define INIT_CONTEXT_MPEG2MC    context_MPEG2MC_p ctx = (context_MPEG2MC_p) obj_context->format_data
#define SURFACE(id)     ((object_surface_p) object_heap_lookup( &ctx->obj_context->driver_data->surface_heap, id ))

static void     psb__MPEG2MC_send_interPB_prediction(
    context_MPEG2MC_p ctx,
    psb_cmdbuf_p const  cmdbuf,
    VAMacroblockParameterBufferMPEG2 * const    mb_param,
    int second_pred
)
{
    uint32_t    cmd;
    uint32_t    blk_size;
    uint32_t    pred_offset;

    /* Determine residual data's block size (16x8 or 16x16)     */
    if (FRAME_PICTURE == ctx->picture_structure) {
        if ((1 == MBPARAM_MotionType(mb_param)) ||  /* Field MC */
            (3 == MBPARAM_MotionType(mb_param))) { /* Dual Prime        */
            blk_size = 1;       /* 16 x 8 */
        } else {
            blk_size = 0;       /* 16 x 16 */
        }
    } else {
        if (2 == MBPARAM_MotionType(mb_param)) { /* Non-Frame MC        */
            blk_size = 1;       /* 16 x 8 */
        } else {
            blk_size = 0; /* 16 x 16 */
        }
    }

    /* Determine whether this is for 1st MV or 2nd MV */
    if (TRUE == second_pred) {
        pred_offset = 8;
    } else {
        pred_offset = 0;
    }

    cmd = 0;

    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, INTER_BLOCK_PREDICTION, INTER_PRED_BLOCK_SIZE,   blk_size);

    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, INTER_BLOCK_PREDICTION, REF_INDEX_A, ctx->ref_indexA);
    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, INTER_BLOCK_PREDICTION, REF_INDEX_B, ctx->ref_indexB);

    if (3 == MBPARAM_MotionType(mb_param)) {  /* Dual Prime */
        REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, INTER_BLOCK_PREDICTION, REF_INDEX_A_VALID, 1);
        REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, INTER_BLOCK_PREDICTION, REF_INDEX_B_VALID, 1);
    } else {
        REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, INTER_BLOCK_PREDICTION, REF_INDEX_A_VALID,
                          MBPARAM_MotionForward(mb_param));
        REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, INTER_BLOCK_PREDICTION, REF_INDEX_B_VALID,
                          MBPARAM_MotionBackward(mb_param));
    }

    if (FRAME_PICTURE == ctx->picture_structure) {
        /* Frame picture processing */
        if ((1 == MBPARAM_MotionType(mb_param)) ||  /* Field MC */
            (3 == MBPARAM_MotionType(mb_param))) { /* Dual Prime        */
            if ((1 == MBPARAM_MotionForward(mb_param)) ||
                (3 == MBPARAM_MotionType(mb_param))) {  /* Dual Prime */
                if (second_pred) {
                    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, INTER_BLOCK_PREDICTION,
                                      REF_INDEX_FIELD_A, MBPARAM_MvertFieldSel_2(mb_param));
                } else {
                    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, INTER_BLOCK_PREDICTION,
                                      REF_INDEX_FIELD_A, MBPARAM_MvertFieldSel_0(mb_param));
                }
            }

            if ((1 == MBPARAM_MotionBackward(mb_param)) ||
                (3 == MBPARAM_MotionType(mb_param))) {  /* Dual Prime   */
                if (second_pred) {
                    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, INTER_BLOCK_PREDICTION,
                                      REF_INDEX_FIELD_B, MBPARAM_MvertFieldSel_3(mb_param));
                } else {
                    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, INTER_BLOCK_PREDICTION,
                                      REF_INDEX_FIELD_B, MBPARAM_MvertFieldSel_1(mb_param));
                }
            }
        }
    } else {
        /* Field picture processing */
        if ((0 == MBPARAM_MotionForward(mb_param)) &&
            (0 == MBPARAM_MotionBackward(mb_param))) {
            REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, INTER_BLOCK_PREDICTION, REF_INDEX_A, ctx->ref_indexA);
            REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, INTER_BLOCK_PREDICTION, REF_INDEX_B, ctx->ref_indexB);
        }

        if (1 == MBPARAM_MotionForward(mb_param)) {
            if (second_pred) {
                if ((0 == MBPARAM_MvertFieldSel_2(mb_param)) &&
                    (PICTURE_CODING_P == ctx->picture_coding_type)) { /* Top field of this frame        */
                    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, INTER_BLOCK_PREDICTION, REF_INDEX_A, ctx->ref_indexB);
                } else { /* Bottom field of previous frame*/
                    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, INTER_BLOCK_PREDICTION, REF_INDEX_A, ctx->ref_indexA);
                }

                REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, INTER_BLOCK_PREDICTION, REF_INDEX_FIELD_A,
                                  MBPARAM_MvertFieldSel_2(mb_param));
            } else {
                if (((ctx->picture_structure == BOTTOM_FIELD) != MBPARAM_MvertFieldSel_0(mb_param)) &&
                    (PICTURE_CODING_P == ctx->picture_coding_type)) {   /* Top field of this frame      */
                    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, INTER_BLOCK_PREDICTION, REF_INDEX_A, ctx->ref_indexB);
                } else { /* Bottom field of previous frame*/
                    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, INTER_BLOCK_PREDICTION, REF_INDEX_A, ctx->ref_indexA);
                }

                REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, INTER_BLOCK_PREDICTION, REF_INDEX_FIELD_A,
                                  MBPARAM_MvertFieldSel_0(mb_param));
            }
        }


        if (1 == MBPARAM_MotionBackward(mb_param)) {
            if (second_pred) {
                if ((1 == MBPARAM_MvertFieldSel_3(mb_param)) &&
                    (PICTURE_CODING_P == ctx->picture_coding_type)) { /* Top field of this frame        */
                    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, INTER_BLOCK_PREDICTION, REF_INDEX_B, ctx->ref_indexA);
                } else {        /* Bottom field of previous frame*/
                    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, INTER_BLOCK_PREDICTION, REF_INDEX_B, ctx->ref_indexB);
                }

                REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, INTER_BLOCK_PREDICTION, REF_INDEX_FIELD_B,
                                  MBPARAM_MvertFieldSel_3(mb_param));
            } else {
                if ((1 == MBPARAM_MvertFieldSel_1(mb_param)) &&
                    (PICTURE_CODING_P == ctx->picture_coding_type)) { /* Top field of this frame        */
                    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, INTER_BLOCK_PREDICTION, REF_INDEX_B, ctx->ref_indexA);
                } else { /* Bottom field of previous frame*/
                    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, INTER_BLOCK_PREDICTION, REF_INDEX_B, ctx->ref_indexB);
                }

                REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, INTER_BLOCK_PREDICTION, REF_INDEX_FIELD_B,
                                  MBPARAM_MvertFieldSel_1(mb_param));
            }
        }
    }

    /* Dual Prime */
    if (3 == MBPARAM_MotionType(mb_param) && (ctx->picture_structure != FRAME_PICTURE)) {
        if (ctx->picture_structure == TOP_FIELD) {
            REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, INTER_BLOCK_PREDICTION, REF_INDEX_A, 0);
            REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, INTER_BLOCK_PREDICTION, REF_INDEX_FIELD_A, 0);
            REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, INTER_BLOCK_PREDICTION, REF_INDEX_FIELD_B, 1);
        } else {
            REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, INTER_BLOCK_PREDICTION, REF_INDEX_A, 1);
            REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, INTER_BLOCK_PREDICTION, REF_INDEX_FIELD_A, 1);
            REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, INTER_BLOCK_PREDICTION, REF_INDEX_FIELD_B, 0);
        }
    }

    psb_cmdbuf_reg_set(cmdbuf , REGISTER_OFFSET(MSVDX_CMDS, INTER_BLOCK_PREDICTION) + pred_offset, cmd);
}

typedef struct _PSB_MVvalue {
    short  horz;
    short  vert;
} psb_MVvalue, *psb_MVvalue_p;

#define MV_LIBVA2PSB(mb_param)                  \
do {                                            \
    MVector[0].horz  = mb_param->PMV[0][0][0];  \
    MVector[0].vert  = mb_param->PMV[0][0][1];  \
                                                \
    MVector[1].horz  = mb_param->PMV[0][1][0];  \
    MVector[1].vert  = mb_param->PMV[0][1][1];  \
                                                \
    MVector[2].horz  = mb_param->PMV[1][0][0];  \
    MVector[2].vert  = mb_param->PMV[1][0][1];  \
                                                \
    MVector[3].horz  = mb_param->PMV[1][1][0];  \
    MVector[3].vert  = mb_param->PMV[1][1][1];  \
} while (0)

static void     psb__MPEG2MC_send_motion_vectores(
    context_MPEG2MC_p   const   ctx,
    psb_cmdbuf_p cmdbuf,
    VAMacroblockParameterBufferMPEG2 * const mb_param
)
{
    uint32_t            cmd = 0;
    uint32_t            MV1Address = 0;
    uint32_t            MV2Address = 0;

    psb_MVvalue  MVector[4];

    MV_LIBVA2PSB(mb_param);

    MV1Address = 0x00;
    MV2Address = 0x00;

    if (FRAME_PICTURE == ctx->picture_structure) {
        /* FRAME PICTURE PROCESSING */
        if (2 == MBPARAM_MotionType(mb_param)) {  /* Frame MC */
            if ((0 == MBPARAM_MotionForward(mb_param)) &&
                (0 == MBPARAM_MotionBackward(mb_param))) {
                cmd = 0;
                REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MOTION_VECTOR, MV_X, 0);
                REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MOTION_VECTOR, MV_Y, 0);
                psb_cmdbuf_reg_set(cmdbuf , REGISTER_OFFSET(MSVDX_CMDS, MOTION_VECTOR) , cmd);
            } else {
                if (1 == MBPARAM_MotionForward(mb_param)) {
                    cmd = 0;
                    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MOTION_VECTOR, MV_X, MVector[0].horz << 1);
                    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MOTION_VECTOR, MV_Y, MVector[0].vert << 1);
                    psb_cmdbuf_reg_set(cmdbuf , REGISTER_OFFSET(MSVDX_CMDS, MOTION_VECTOR) , cmd);
                }

                if (1 == MBPARAM_MotionBackward(mb_param)) {
                    cmd = 0;
                    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MOTION_VECTOR, MV_X, MVector[1].horz << 1);
                    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MOTION_VECTOR, MV_Y, MVector[1].vert << 1);
                    psb_cmdbuf_reg_set(cmdbuf , REGISTER_OFFSET(MSVDX_CMDS, MOTION_VECTOR) + 0x10 , cmd);
                }
            }
        } else  {
            if ((1 == MBPARAM_MotionType(mb_param)) ||  /* Field MC */
                (3 == MBPARAM_MotionType(mb_param))) {  /* Dual Prime */
                /*
                 * Vertical motion vectors for fields located in frame pictures
                 * should be divided by 2 (MPEG-2 7.6.3.1). Thus the original value
                 * contained in the stream is equivalent to 1/4-pel     format; the
                 * resolution required by MSVDX.
                 */
                if ((1 == MBPARAM_MotionForward(mb_param)) ||
                    (3 == MBPARAM_MotionType(mb_param))) {  /* Dual Prime       */
                    cmd = 0;
                    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MOTION_VECTOR, MV_X, MVector[0].horz << 1);
                    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MOTION_VECTOR, MV_Y, MVector[0].vert);
                    psb_cmdbuf_reg_set(cmdbuf , REGISTER_OFFSET(MSVDX_CMDS, MOTION_VECTOR) , cmd);
                }

                if ((1 == MBPARAM_MotionBackward(mb_param)) ||
                    (3 == MBPARAM_MotionType(mb_param))) {  /* Dual Prime */
                    cmd = 0;

                    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MOTION_VECTOR, MV_X, MVector[1].horz << 1);

                    if (3 == MBPARAM_MotionType(mb_param)) {  /* Dual Prime */
                        REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MOTION_VECTOR, MV_Y, MVector[1].vert << 1);
                    } else {
                        REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MOTION_VECTOR, MV_Y, MVector[1].vert);
                    }
                    psb_cmdbuf_reg_set(cmdbuf , REGISTER_OFFSET(MSVDX_CMDS, MOTION_VECTOR) + 0x10 , cmd);
                }

                /* Fields and Dual Prime are 16x8 and need 2nd inter_block_pred cmd     */
                psb__MPEG2MC_send_interPB_prediction(ctx, cmdbuf, mb_param, IMG_TRUE);

                if ((1 == MBPARAM_MotionForward(mb_param)) ||
                    (3 == MBPARAM_MotionType(mb_param))) {  /* Dual Prime */
                    cmd = 0;
                    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MOTION_VECTOR, MV_X, MVector[2].horz << 1);
                    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MOTION_VECTOR, MV_Y, MVector[2].vert);
                    psb_cmdbuf_reg_set(cmdbuf , REGISTER_OFFSET(MSVDX_CMDS, MOTION_VECTOR) + 0x40 , cmd);
                }

                if ((1 == MBPARAM_MotionBackward(mb_param)) ||
                    (3 == MBPARAM_MotionType(mb_param))) {      /* Dual Prime                   */
                    cmd = 0;
                    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MOTION_VECTOR, MV_X, MVector[3].horz << 1);
                    if (3 == MBPARAM_MotionType(mb_param)) {    /* Dual Prime                   */
                        REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MOTION_VECTOR, MV_Y, MVector[3].vert << 1);
                    } else {
                        REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MOTION_VECTOR, MV_Y, MVector[3].vert);
                    }
                    psb_cmdbuf_reg_set(cmdbuf , REGISTER_OFFSET(MSVDX_CMDS, MOTION_VECTOR) + 0x50 , cmd);
                }
            }
        }
    } else {
        /* FIELD PICTURE PROCESSING */
        int MV0index = 0, MV1index = 1;

        if ((ctx->picture_structure == BOTTOM_FIELD) && (3 == MBPARAM_MotionType(mb_param))) {
            MV0index = 1;
            MV1index = 0;
        }

        if ((1 == MBPARAM_MotionForward(mb_param)) ||   /* Forward MV   */
            (3 == MBPARAM_MotionType(mb_param))) { /* Dual Prime        */
            cmd = 0;
            REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MOTION_VECTOR, MV_X, MVector[MV0index].horz << 1);
            REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MOTION_VECTOR, MV_Y, MVector[MV0index].vert << 1);
            psb_cmdbuf_reg_set(cmdbuf , REGISTER_OFFSET(MSVDX_CMDS, MOTION_VECTOR) , cmd);

        }

        if ((1 == MBPARAM_MotionBackward(mb_param)) ||  /* Backward MV  */
            (3 == MBPARAM_MotionType(mb_param))) { /* Dual Prime        */
            cmd = 0;
            REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MOTION_VECTOR, MV_X, MVector[MV1index].horz << 1);
            REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MOTION_VECTOR, MV_Y, MVector[MV1index].vert << 1);
            psb_cmdbuf_reg_set(cmdbuf , REGISTER_OFFSET(MSVDX_CMDS, MOTION_VECTOR)  + 0x10, cmd);
        }

        if (2 == MBPARAM_MotionType(mb_param)) { /* 16x8 MC */
            psb__MPEG2MC_send_interPB_prediction(ctx, cmdbuf, mb_param, IMG_TRUE);

            if ((1 == MBPARAM_MotionForward(mb_param)) ||  /* Forward MV */
                (3 == MBPARAM_MotionType(mb_param))) {  /* Dual Prime */
                cmd = 0;
                REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MOTION_VECTOR, MV_X, MVector[2].horz << 1);
                REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MOTION_VECTOR, MV_Y, MVector[2].vert << 1);
                psb_cmdbuf_reg_set(cmdbuf , REGISTER_OFFSET(MSVDX_CMDS, MOTION_VECTOR)  + 0x40, cmd);
            }

            if ((1 == MBPARAM_MotionBackward(mb_param)) ||      /* Backward MV                          */
                (3 == MBPARAM_MotionType(mb_param))) {  /* Dual Prime                           */
                cmd = 0;
                REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MOTION_VECTOR, MV_X, MVector[3].horz << 1);
                REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MOTION_VECTOR, MV_Y, MVector[3].vert << 1);

                psb_cmdbuf_reg_set(cmdbuf , REGISTER_OFFSET(MSVDX_CMDS, MOTION_VECTOR)  + 0x50, cmd);
            }
        }
    }
}


/* Send macroblock_number command to MSVDX. */
static void     psb__MPEG2MC_send_mb_number(
    context_MPEG2MC_p   const   ctx,
    psb_cmdbuf_p cmdbuf,
    const uint32_t mb_addr,
    const uint32_t motion_type,
    const eMB_CODE_TYPE MB_coding_type
)
{
    uint32_t cmd;
    uint32_t mb_x;

    /* 3.3.21.  Macroblock Number */
    cmd = 0;

    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MACROBLOCK_NUMBER, MB_ERROR_FLAG, 0);

    /*
      MB_FIELD_CODE (MC + VDEB): Indicate if the macroblock is field predicted (when = 1),
                                 or frame predicted (when VDEB = 0).
          MPEG2: For Interlaced frame, derived from ‘frame_motion_type’, else same
                 frame/filed type as SLICE_FIELD_TYPE

     */
    if (FRAME_PICTURE == ctx->picture_structure) {
        if ((0 == motion_type)  ||
            (2 == motion_type)) {
            /* MB is frame predicted    */
            REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MACROBLOCK_NUMBER, MB_FIELD_CODE, 0);
        } else { /* MB is field predicted       */
            REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MACROBLOCK_NUMBER, MB_FIELD_CODE, 1);
        }
    } else { /* MB is field predicted   */
        REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MACROBLOCK_NUMBER, MB_FIELD_CODE, 1);
    }

    ASSERT((0 != ctx->picture_coding_type) && (4 != ctx->picture_coding_type));

    /*
      MB_CODE_TYPE (MC + VDEB): Indicate macroblock type is I, P, B or MPEG4 S(GMC).
                  0x0: I or SI macroblock
                  0x1: P or SP macroblock
                  0x2: B macroblock
                  0x3: MPEG4 GMC
           MPEG2: Derived from ‘macroblock_type’
     */
    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MACROBLOCK_NUMBER, MB_CODE_TYPE, (uint32_t) MB_coding_type);


    /*
      MB_NO_Y (MC + VDEB): Vertical offset of current Macroblock (where 0 = topmost macroblock of picture)
                           Derived from macroblock number divided by picture width in macroblocks.
      MB_NO_X (MC + VDEB): Horizontal offset of current Macroblock (where 0 = leftmost macroblock of picture)
                           Derived from macroblock number mod picture width in macroblocks.
     */
    if ((FRAME_PICTURE != ctx->picture_structure) &&
        (mb_addr / ctx->picture_width_mb)) {
        REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MACROBLOCK_NUMBER, MB_NO_Y, (mb_addr / ctx->picture_width_mb) * 2);
    } else {
        REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MACROBLOCK_NUMBER, MB_NO_Y, mb_addr / ctx->picture_width_mb);
    }

    mb_x = mb_addr % ctx->picture_width_mb;
    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MACROBLOCK_NUMBER, MB_NO_X, mb_x);
    /*
      Only defined for current MB, set to 0 for above1 and above2.
              Indicate if MB is on Left Hand Side of slice or picture
              0: MB is not on left hand side of slice or picture
              1: MB is on left hand side of slice or picture
    */
    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MACROBLOCK_NUMBER , MB_SLICE_LHS, ctx->fstmb_slice || (mb_x == 0));
    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MACROBLOCK_NUMBER , MB_SLICE_TOP, 1);

    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_CMDS, MACROBLOCK_NUMBER) , cmd);
}

static void psb__MPEG2MC_finalise_residDMA(
    context_MPEG2MC_p   const   ctx
)
{
    uint32_t *save_idx = ctx->obj_context->cmdbuf->cmd_idx;
    ctx->obj_context->cmdbuf->cmd_idx = ctx->lldma_idx;
    if (ctx->residual_pendingDMA) {
#if 0
        if (ctx->residual_pendingDMA != (ctx->blk_size * ctx->blk_in_buffer)) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "psb__MPEG2MC_finalise_residDMA:residual_pendingDMA=%d(block:%d),"
                               "actual data size=%d (block:%d)\n",
                               ctx->residual_pendingDMA, ctx->residual_pendingDMA / ctx->blk_size,
                               ctx->blk_size * ctx->blk_in_buffer,
                               ctx->blk_in_buffer);
        }
#endif

        psb_cmdbuf_lldma_write_cmdbuf(ctx->obj_context->cmdbuf,
                                      ctx->residual_buf,
                                      ctx->residual_sent,
                                      ctx->residual_pendingDMA,
                                      0,
                                      LLDMA_TYPE_RESIDUAL);
    } else {
        //*ctx->obj_context->cmdbuf->cmd_idx = CMD_NOP;
        *ctx->obj_context->cmdbuf->cmd_idx = 0xf000000;
    }
    ctx->obj_context->cmdbuf->cmd_idx = save_idx;
    ctx->residual_sent += ctx->residual_pendingDMA;
    ctx->residual_pendingDMA = 0;
}

static void psb__MPEG2MC_check_segment_residDMA(
    context_MPEG2MC_p   const   ctx,
    uint32_t min_space
)
{
    if (psb_cmdbuf_segment_space(ctx->obj_context->cmdbuf) < min_space) {
        psb__MPEG2MC_finalise_residDMA(ctx);

        psb_cmdbuf_next_segment(ctx->obj_context->cmdbuf);

        ctx->lldma_idx = ctx->obj_context->cmdbuf->cmd_idx++; /* Insert the LLDMA record here later */
    }
}


/* Send residual difference data to MSVDX. */
static void     psb__MPEG2MC_send_residual(
    context_MPEG2MC_p   ctx,
    uint32_t    pattern_code)
{
    uint8_t pattern_code_byte = (uint8_t)(pattern_code >> 6);
    uint8_t blocks = 0;

    while (pattern_code_byte) {
        blocks += (pattern_code_byte & 1);
        pattern_code_byte >>= 1;
    }

    if (PICTURE_CODING_I == ctx->picture_coding_type) {
        ctx->residual_pendingDMA += blocks * (8 * 8); /* 8bit */
    } else {
        /* We do not suport ConfigSpatialResid8==1  */
        /* ASSERT(ConfigSpatialResid8 == 0); */
        ctx->residual_pendingDMA += blocks * (8 * 8) * 2;/*  16 bit */
    }
}


static void     psb__MPEG2MC_send_slice_parameters(
    context_MPEG2MC_p   const   ctx
)
{
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    uint32_t cmd;

    ctx->lldma_idx = ctx->obj_context->cmdbuf->cmd_idx++; /* Insert the LLDMA record here later */

    psb_cmdbuf_reg_start_block(cmdbuf, 0);

    /* 3.3.19.  Slice Params*/
    /*
      3:2 SLICE_FIELD_TYPE (MC+VDEB)  Indicate if slice is a frame, top fie
                      0x0: Top field picture
                      0x1: Bottom field picture
                      0x2: Frame picture
      1:0 SLICE_CODE_TYPE  (MC+VDEB)  Indicate if it is an I, P, B  slice
                      0x0: I slice
                      0x1: P slice
                      0x2: B slice
                      0x3: S(GMC) (MPEG4 only)
     */
    cmd = 0;
    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, SLICE_PARAMS, SLICE_FIELD_TYPE,
                      ft_libva2msvdx[ctx->picture_structure]);
    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, SLICE_PARAMS, SLICE_CODE_TYPE,
                      pict_libva2msvdx[ctx->picture_coding_type]);
    psb_cmdbuf_reg_set(cmdbuf , REGISTER_OFFSET(MSVDX_CMDS, SLICE_PARAMS), cmd);

    psb_cmdbuf_reg_end_block(cmdbuf);
}

static void psb__MPEG2MC_send_slice_picture_endcommand(
    context_MPEG2MC_p   const   ctx,
    int end_picture
)
{
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    uint32_t cmd;

    /*
      3.3.22.         End Slice/Picture
      Offset:           0x0404
      This command is sent at the end of a slice or picture. The final macroblock of slice/picture will not be
      processed until this command is sent. The End Slice/Picture commands can be sent more than once
      at the end of a slice (provided no other commands are interleaved).
      If the command is sent more than once at the end of a slice, the first command should indicate the
      end of the slice (no data bits set) with repeat commands used to indicate end of slice, flushing VDEB
      buffers or picture end. The FLUSH_VDEB_BUFFERS bit should not be set in any repeat commands
      sent after a command in which the PICTURE_END bit is set.
      31:2 -                       Reserved
      1    FLUSH_VDEB_BUFFERS(VDEB) If set, indicates VDEB should flush its internal buffers to system memory
                                    when slice processing is complete
      0    PICTURE_END(MC + VDEB)   If set, indicates both Picture and Slice end, otherwise Slice end
     */
    psb_cmdbuf_reg_start_block(cmdbuf, 0);

    psb_cmdbuf_reg_set(cmdbuf , REGISTER_OFFSET(MSVDX_CMDS, END_SLICE_PICTURE), 0);

    if (end_picture) {
        cmd = 0;
        REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, END_SLICE_PICTURE, PICTURE_END, end_picture);
        psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_CMDS, END_SLICE_PICTURE), cmd);
    }

    psb_cmdbuf_reg_end_block(cmdbuf);
}

static void     psb__MPEG2MC_send_highlevel_commands(
    context_MPEG2MC_p   const   ctx
)
{
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    psb_surface_p target_surface = ctx->obj_context->current_render_target->psb_surface;

    psb_cmdbuf_reg_start_block(cmdbuf, 0);

    /* 3.3.1.   Display Picture Size */
    psb_cmdbuf_reg_set(cmdbuf , REGISTER_OFFSET(MSVDX_CMDS, DISPLAY_PICTURE_SIZE), ctx->display_picture_size);

    /* 3.3.2.   Coded Picture Size*/
    psb_cmdbuf_reg_set(cmdbuf , REGISTER_OFFSET(MSVDX_CMDS, CODED_PICTURE_SIZE), ctx->coded_picture_size);

    /* 3.3.3.   Operating Mode */
    psb_cmdbuf_reg_set(cmdbuf , REGISTER_OFFSET(MSVDX_CMDS, OPERATING_MODE), ctx->obj_context->operating_mode);

    /* 3.3.4.   Luma Reconstructed Picture Base Address */
    psb_cmdbuf_reg_set_RELOC(cmdbuf , REGISTER_OFFSET(MSVDX_CMDS, LUMA_RECONSTRUCTED_PICTURE_BASE_ADDRESSES),
                             &target_surface->buf, 0);

    /* 3.3.5.   Chroma Reconstructed Picture Base Address */
    psb_cmdbuf_reg_set_RELOC(cmdbuf , REGISTER_OFFSET(MSVDX_CMDS, CHROMA_RECONSTRUCTED_PICTURE_BASE_ADDRESSES),
                             &target_surface->buf, target_surface->chroma_offset);

    /* 3.3.13.  Reference Picture Base Addresses */
    /*
      Reference Picture Base Addresses
              Offset:          0x0100 + N*8 = Luma base addresses
              Offset:          0x0104 + N*8 = Chroma base addresses
      This register can store up to 16 luma picture base addresses and 16 chroma picture base addresses.
      Luma and chroma ref base registers are interleaved.

      Bit      Symbol                    Used     Description
              31:12    LUMA_REF_BASE_ADDR        MC       Luma picture base byte address [31:12]
              11:0     -                                  Reserved
      Bit      Symbol                     Used     Description
              31:12    CHROMA_REF_BASE_ADDR       MC       Chroma picture base byte address [31:12]
              11:0     -                                   Reserved
      -   In MPEG2, the registers at N=0 are always used to store the base address of the luma and
      chroma buffers of the most recently decoded reference picture. The registers at N=1 are used
      to store the base address of the luma and chroma buffers of the older reference picture, if
      more than one reference picture is used. This means that when decoding a P picture the
      forward reference picture’s address is at index 0. When decoding a B-picture the backward
      reference picture’s address is at index 0 and the address of the forward reference picture –
      which was decoded earlier than the backward reference – is at index 1.
     */

    /* WABA: backward / forward refs are always set, even when they aren't strictly needed */
    psb_surface_p forward_surface = ctx->forward_ref_surface->psb_surface;
    psb_surface_p backward_surface = ctx->backward_ref_surface->psb_surface;

    if (backward_surface) {
        psb_cmdbuf_reg_set_RELOC(cmdbuf , REGISTER_OFFSET(MSVDX_CMDS, REFERENCE_PICTURE_BASE_ADDRESSES) + (0 * 8),
                                 &backward_surface->buf, 0);

        psb_cmdbuf_reg_set_RELOC(cmdbuf , REGISTER_OFFSET(MSVDX_CMDS, REFERENCE_PICTURE_BASE_ADDRESSES) + 4 + (0 * 8),
                                 &backward_surface->buf, backward_surface->chroma_offset);
    }
    if (forward_surface) {
        psb_cmdbuf_reg_set_RELOC(cmdbuf , REGISTER_OFFSET(MSVDX_CMDS, REFERENCE_PICTURE_BASE_ADDRESSES) + (1 * 8),
                                 &forward_surface->buf, 0);

        psb_cmdbuf_reg_set_RELOC(cmdbuf , REGISTER_OFFSET(MSVDX_CMDS, REFERENCE_PICTURE_BASE_ADDRESSES) + 4 + (1 * 8),
                                 &forward_surface->buf, forward_surface->chroma_offset);
    }

    /*
     * VDMC_RESIDUAL_DIRECT_INSERT_CONTROL, spec p 159
     * 0 VDMC_RESIDUAL_DIRECT_CONTROL (VDMC) residual direct insert control.
     *           Control insertion of spatial residual data. When set to 1 residual
     *           data taken from writes to VDMC_RESIDUAL_DIRECT_INSERT_DATA
     *           when set to 0, residual data taken from vEC
     */
    psb_cmdbuf_reg_set(cmdbuf , REGISTER_OFFSET(MSVDX_VDMC, CR_VDMC_RESIDUAL_DIRECT_INSERT_CONTROL), 1);

    psb_cmdbuf_reg_end_block(cmdbuf);
}



/* Control building of the MSVDX command buffer for a B and P pictures.*/
static void     psb__MPEG2MC_interPB_mb(
    context_MPEG2MC_p const     ctx,
    VAMacroblockParameterBufferMPEG2 * const    mb_param
)
{
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    uint32_t cmd;

    psb_cmdbuf_reg_start_block(cmdbuf, 0);

    /* 3.3.21.  Macroblock Number */
    psb__MPEG2MC_send_mb_number(ctx, cmdbuf, mb_param->macroblock_address, MBPARAM_MotionType(mb_param),
                                MBPARAM_IntraMacroblock(mb_param) ? MB_CODE_TYPE_I : pict_libva2msvdx[ctx->picture_coding_type]
                               );

    cmd = 0;
    /* Only used for direct insert of residual data;-
       00 = 8-bit signed data
       01 = 8-bit unsigned data
       10 = 16-bit signed
       11 = Reserved
    */
    /* TODO:: ASSERT(ConfigSpatialResid8==0 ); */
    if (MBPARAM_IntraMacroblock(mb_param)) {
        if (1/* TODO:: ConfigIntraResidUnsigned == 0 */) {
            /* sent as as 16 bit signed relative to 128*/
            REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MACROBLOCK_RESIDUAL_FORMAT, VA_ADD_128, 1);
            REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MACROBLOCK_RESIDUAL_FORMAT, VA_DATA_FORMAT, 2);
        } else { /* ConfigIntraResidUnsigned == 1 */
            /* 16 bit unsigned unsigned relative to 0*/
            REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MACROBLOCK_RESIDUAL_FORMAT, VA_ADD_128, 0);
            REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MACROBLOCK_RESIDUAL_FORMAT, VA_DATA_FORMAT,       2);
        }
    } else {
        /* For non-intra in Inter frames : 16 bit signed */
        REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MACROBLOCK_RESIDUAL_FORMAT, VA_ADD_128, 0);
        REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MACROBLOCK_RESIDUAL_FORMAT, VA_DATA_FORMAT, 2);
    }


    if (FRAME_PICTURE == ctx->picture_structure) {
        /* mb_param->macroblock_modes.bits.dct_type =1: field DCT */
        REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MACROBLOCK_RESIDUAL_FORMAT, RESIDUAL_FIELD_CODED, mb_param->macroblock_modes.bits.dct_type ? 1 : 0);
    } else {
        REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MACROBLOCK_RESIDUAL_FORMAT, RESIDUAL_FIELD_CODED, 1);
    }

    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MACROBLOCK_RESIDUAL_FORMAT, CR_FROM_VEC,
                      ((MBPARAM_CodedBlockPattern(mb_param) & 0x40) ? 1 : 0));
    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MACROBLOCK_RESIDUAL_FORMAT, CB_FROM_VEC,
                      ((MBPARAM_CodedBlockPattern(mb_param) & 0x80) ? 1 : 0));
    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MACROBLOCK_RESIDUAL_FORMAT, Y3_FROM_VEC,
                      ((MBPARAM_CodedBlockPattern(mb_param) & 0x100) ? 1 : 0));
    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MACROBLOCK_RESIDUAL_FORMAT, Y2_FROM_VEC,
                      ((MBPARAM_CodedBlockPattern(mb_param) & 0x200) ? 1 : 0));
    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MACROBLOCK_RESIDUAL_FORMAT, Y1_FROM_VEC,
                      ((MBPARAM_CodedBlockPattern(mb_param) & 0x400) ? 1 : 0));
    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MACROBLOCK_RESIDUAL_FORMAT, Y0_FROM_VEC,
                      ((MBPARAM_CodedBlockPattern(mb_param) & 0x800) ? 1 : 0));
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_CMDS, MACROBLOCK_RESIDUAL_FORMAT) , cmd);

    /* Send Residuals   */
    if (MBPARAM_IntraMacroblock(mb_param)) {
        cmd = 0;
        REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, INTRA_BLOCK_PREDICTION, INTRA_PRED_MODE0, 0);
        REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, INTRA_BLOCK_PREDICTION, INTRA_PRED_MODE1, 0);
        REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, INTRA_BLOCK_PREDICTION, INTRA_PRED_MODE2, 0);
        REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, INTRA_BLOCK_PREDICTION, INTRA_PRED_MODE3, 0);
        psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_CMDS, INTRA_BLOCK_PREDICTION) , cmd);

        psb__MPEG2MC_send_residual(ctx, MBPARAM_CodedBlockPattern(mb_param));
    } else {
        psb__MPEG2MC_send_interPB_prediction(ctx, cmdbuf, mb_param, IMG_FALSE);

        /* Send motion vectors  */
        psb__MPEG2MC_send_motion_vectores(ctx, cmdbuf, mb_param);

        psb__MPEG2MC_send_residual(ctx, MBPARAM_CodedBlockPattern(mb_param));
    }

    psb_cmdbuf_reg_end_block(cmdbuf);
}



/* Process Macroblocks of a B or P picture.*/
static VAStatus psb__MPEG2MC_process_mbs_interPB(
    context_MPEG2MC_p   const   ctx
)
{
    uint32_t    skip_count = 0;
    VAMacroblockParameterBufferMPEG2 *mb_param  = NULL;
    VAMacroblockParameterBufferMPEG2 mbparam_skip;

    uint32_t mb_pending = ctx->mb_in_buffer;
    uint32_t mb_addr = ctx->mb_first_addr;
    uint32_t mb_last = mb_addr + mb_pending;
    mb_param = (VAMacroblockParameterBufferMPEG2 *) ctx->mb_param;

    /* Proccess all VA_MBctrl_P_HostResidDiff_1 structure in the buffer
     * - this may genererate more macroblocks due to skipped
     */
    while (mb_pending || skip_count) {
        uint32_t mb_in_buffer = (ctx->picture_width_mb);
        psb_cmdbuf_p cmdbuf;
        unsigned char *cmd_start;

        ctx->fstmb_slice = IMG_TRUE;

        psb_context_get_next_cmdbuf(ctx->obj_context);
        cmdbuf = ctx->obj_context->cmdbuf;
        cmd_start = (unsigned char *) cmdbuf->cmd_idx;

        /* Build the high-level commands */
        psb__MPEG2MC_send_highlevel_commands(ctx);

        psb__MPEG2MC_send_slice_parameters(ctx);

        /* Process all the macroblocks in the slice */
        while ((mb_pending || skip_count) && mb_in_buffer--) {
            /* Check for segment space - do we have space for at least one more
             * worst case InterMB plus completion
            */
            psb__MPEG2MC_check_segment_residDMA(ctx, INTER_MB_WORST_CASE + 2);

            if (skip_count) {  /* Skipped macroblock processing */
                mbparam_skip.macroblock_address++;

                ASSERT(mb_param->macroblock_address < ctx->size_mb);
                ASSERT(mbparam_skip.macroblock_address  == mb_addr);

                psb__MPEG2MC_interPB_mb(ctx, &mbparam_skip);

                skip_count--;
            } else {
                ASSERT(mb_pending);
                ASSERT(mb_param->macroblock_address < ctx->size_mb);
                ASSERT(mb_param->macroblock_address  == mb_addr);

                psb__MPEG2MC_interPB_mb(ctx, mb_param);

                skip_count = MBPARAM_MBskipsFollowing(mb_param);
                if (skip_count) {
                    memcpy(&mbparam_skip, mb_param, sizeof(mbparam_skip));
                }

                mb_param++;
                mb_pending--;
            }

            ctx->fstmb_slice = IMG_FALSE;

            mb_addr++;
        }

        /* Tell hardware we're done     */
        psb__MPEG2MC_send_slice_picture_endcommand(ctx, (mb_pending == 0) && (skip_count == 0) && (ctx->size_mb == mb_last));
        psb__MPEG2MC_finalise_residDMA(ctx);

        /* write_kick */
        *cmdbuf->cmd_idx++ = CMD_COMPLETION;

        ctx->obj_context->video_op = psb_video_mc;
        ctx->obj_context->flags = (mb_pending == 0) && (skip_count == 0) && (ctx->size_mb == mb_last) ? FW_VA_RENDER_IS_LAST_SLICE : 0;
        ctx->obj_context->first_mb = 0;
        ctx->obj_context->last_mb = 0;
        psb_context_submit_cmdbuf(ctx->obj_context);

        /* check if the remained cmdbuf size can fill the commands of next slice */
        if (1 || (cmdbuf->lldma_base - (unsigned char *) cmdbuf->cmd_idx) < ((unsigned char *) cmdbuf->cmd_idx - cmd_start))
            psb_context_flush_cmdbuf(ctx->obj_context);
    }

    return VA_STATUS_SUCCESS;
}


static void     psb__MPEG2MC_intra_mb(
    context_MPEG2MC_p const ctx,
    const VAMacroblockParameterBufferMPEG2* const mb_param
)
{
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    uint32_t cmd;

    psb_cmdbuf_reg_start_block(cmdbuf, 0);

    /* 3.3.21.  Macroblock Number */
    psb__MPEG2MC_send_mb_number(ctx, cmdbuf, mb_param->macroblock_address, MBPARAM_MotionType(mb_param), MB_CODE_TYPE_I);

    /*3.3.25.   Macroblock Residual Format */
    cmd = 0;

    /* In INTRA pictures, spatial-blocks are always 8bit when BBP is 8 */
    /*  00 = 8-bit signed data
     *  01 = 8-bit unsigned data
     */
    if (1/* TODO:: ConfigIntraResidUnsigned==0 */) {
        /* spec p67:
         * VA_ADD_128 MC Only used for direct insert of residual data;-
         *       0: add 0 to residual data input
         *       1: indicates 128 should be added to residual data input
         *
         * VA_DATA_FORMAT:Only used for direct insert of residual data;-
         *       0x0: 8-bit signed data
         *       0x1: 8-bit unsigned data
         *       0x2: 16-bit signed
         */
        /* Sent as  8 bit signed relative to 128 */
        REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MACROBLOCK_RESIDUAL_FORMAT, VA_DATA_FORMAT, 0);  /* ok ish */
        REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MACROBLOCK_RESIDUAL_FORMAT, VA_ADD_128,       1);   /* ok ish */
    } else {
        /* Sent as 8 bit unsigned relative to 0 */
        REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MACROBLOCK_RESIDUAL_FORMAT, VA_DATA_FORMAT, 1);
        REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MACROBLOCK_RESIDUAL_FORMAT, VA_ADD_128,       0);
    }

    if (FRAME_PICTURE == ctx->picture_structure) {
        /*
         * RESIDUAL_FIELD_CODED MC
         *               0: residual data frame coded
         *               1: (luma)residual data field coded
         *       N.B. For VC1, MPEG4 and MPEG2, if SLICE_FIELD_TYPE =
         *            frame, chroma residual will be frame coded, even if the luma
         *            residual is field coded.
         */
        /*
         * (VA:wMBType bit 5: FieldResidual, wMBtype & 0x20)/libVA(dct_type:1 field DCT):
         * Indicates whether the residual difference blocks use a field IDCT structure as specified in MPEG-2.
         *
         * Must be 1 if the bPicStructure member of VA_PictureParameters is 1 or 2. When used for MPEG-2,
         * FieldResidual must be zero if the frame_pred_frame_DCT variable in the MPEG-2 syntax is 1, and
         * must be equal to the dct_type variable in the MPEG-2 syntax if dct_type is present for the macroblock.
         */
        REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MACROBLOCK_RESIDUAL_FORMAT,
                          RESIDUAL_FIELD_CODED, ((mb_param->macroblock_modes.bits.dct_type) ? 1 : 0));
    } else {
        REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MACROBLOCK_RESIDUAL_FORMAT, RESIDUAL_FIELD_CODED, 1);
    }

    /*   CR_FROM_VEC MC  0: constant zero residual
                        1: block Cr provided by VEC
                        (If REVERSE_FLAG_ORDER=1, this bit indicates if Y0 provided)
         CB_FROM_VEC MC 0: constant zero residual
                        1: block Cb provided by VEC
                     (If REVERSE_FLAG_ORDER=1, this bit indicates if Y1 provided)
         Y3_FROM_VEC MC 0: constant zero residual
                        1: block Y3 provided by VEC
                        (If REVERSE_FLAG_ORDER=1, this bit indicates if Y2 provided)
         Y2_FROM_VEC MC 0: constant zero residual
                        1: block Y2 provided by VEC
                         (If REVERSE_FLAG_ORDER=1, this bit indicates if Y3 provided)
         Y1_FROM_VEC MC 0: constant zero residual
                        1: block Y1 provided by VEC
                         (If REVERSE_FLAG_ORDER=1, this bit indicates if Cb provided)
         Y0_FROM_VEC MC 0: constant zero residual
                        1: block Y0 provided by VEC
                         (If REVERSE_FLAG_ORDER=1, this bit indicates if Cr provided)
    */
    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MACROBLOCK_RESIDUAL_FORMAT, CR_FROM_VEC,
                      ((MBPARAM_CodedBlockPattern(mb_param) & 0x40) ? 1 : 0));
    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MACROBLOCK_RESIDUAL_FORMAT, CB_FROM_VEC,
                      ((MBPARAM_CodedBlockPattern(mb_param) & 0x80) ? 1 : 0));
    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MACROBLOCK_RESIDUAL_FORMAT, Y3_FROM_VEC,
                      ((MBPARAM_CodedBlockPattern(mb_param) & 0x100) ? 1 : 0));
    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MACROBLOCK_RESIDUAL_FORMAT, Y2_FROM_VEC,
                      ((MBPARAM_CodedBlockPattern(mb_param) & 0x200) ? 1 : 0));
    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MACROBLOCK_RESIDUAL_FORMAT, Y1_FROM_VEC,
                      ((MBPARAM_CodedBlockPattern(mb_param) & 0x400) ? 1 : 0));
    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, MACROBLOCK_RESIDUAL_FORMAT, Y0_FROM_VEC,
                      ((MBPARAM_CodedBlockPattern(mb_param) & 0x800) ? 1 : 0));
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_CMDS, MACROBLOCK_RESIDUAL_FORMAT) , cmd);

    /* Send Residuals, spec p69,h264 only */
    cmd = 0;
    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, INTRA_BLOCK_PREDICTION, INTRA_PRED_MODE0, 0);
    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, INTRA_BLOCK_PREDICTION, INTRA_PRED_MODE1, 0);
    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, INTRA_BLOCK_PREDICTION, INTRA_PRED_MODE2, 0);
    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, INTRA_BLOCK_PREDICTION, INTRA_PRED_MODE3, 0);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_CMDS, INTRA_BLOCK_PREDICTION) , cmd);

    psb__MPEG2MC_send_residual(ctx, MBPARAM_CodedBlockPattern(mb_param));

    psb_cmdbuf_reg_end_block(cmdbuf);
}


static VAStatus psb__MPEG2MC_process_mbs_intra(
    context_MPEG2MC_p   const   ctx
)
{
    const VAMacroblockParameterBufferMPEG2* mb_param = NULL;
    uint32_t mb_pending = ctx->mb_in_buffer;
    uint32_t mb_addr = ctx->mb_first_addr;
    uint32_t mb_last = mb_addr + mb_pending;

    mb_param = (VAMacroblockParameterBufferMPEG2*) ctx->mb_param;

    while (mb_pending) { /* one slice per loop */
        uint32_t mb_in_buffer =  min(mb_pending, ctx->picture_width_mb);
        psb_cmdbuf_p cmdbuf;
        unsigned char *cmd_start;

        mb_pending -= mb_in_buffer;

        psb_context_get_next_cmdbuf(ctx->obj_context);
        cmdbuf = ctx->obj_context->cmdbuf;
        cmd_start = (unsigned char *) cmdbuf->cmd_idx;

        ctx->fstmb_slice = IMG_TRUE;

        /* Build the high-level commands */
        psb__MPEG2MC_send_highlevel_commands(ctx);

        psb__MPEG2MC_send_slice_parameters(ctx);

        /* Process all the macroblocks in the slice */
        while (mb_in_buffer--) { /* for every MB */
            ASSERT(mb_param->macroblock_address < ctx->size_mb);
            ASSERT(mb_param->macroblock_address  == mb_addr);

            /* Check for segment space - do we have space for at least one more
             * worst case IntraMB plus completion
            */
            psb__MPEG2MC_check_segment_residDMA(ctx, INTRA_MB_WORST_CASE + 2);

            psb__MPEG2MC_intra_mb(ctx, mb_param);

            mb_param++; /* next macroblock parameter */
            mb_addr++;

            ctx->fstmb_slice = IMG_FALSE;
        }

        psb__MPEG2MC_send_slice_picture_endcommand(ctx, (mb_pending == 0) && (ctx->size_mb == mb_last)); /* Tell hardware we're done    */

        psb__MPEG2MC_finalise_residDMA(ctx);

        /* write_kick */
        *cmdbuf->cmd_idx++ = CMD_COMPLETION;

        ctx->obj_context->video_op = psb_video_mc;
        ctx->obj_context->flags = (mb_pending == 0) && (ctx->size_mb == mb_last) ? FW_VA_RENDER_IS_LAST_SLICE : 0;
        ctx->obj_context->first_mb = 0;
        ctx->obj_context->last_mb = 0;
        psb_context_submit_cmdbuf(ctx->obj_context);

        /* check if the remained cmdbuf size can fill the commands of next slice */
        if (1 || (cmdbuf->lldma_base - (unsigned char *) cmdbuf->cmd_idx) < ((unsigned char *) cmdbuf->cmd_idx - cmd_start))
            psb_context_flush_cmdbuf(ctx->obj_context);
    }

    //ASSERT(ctx->residual_bytes == 0); /* There should be no more data left */

    return VA_STATUS_SUCCESS;
}


static VAStatus psb__MPEG2MC_process_picture_param(context_MPEG2MC_p ctx, object_buffer_p obj_buffer)
{
    int coded_pic_height;

    /* Take a copy of the picture parameters */
    ctx->pic_params = (VAPictureParameterBufferMPEG2 *) obj_buffer->buffer_data;
    obj_buffer->buffer_data = NULL;
    obj_buffer->size = 0;

    ctx->picture_coding_type = ctx->pic_params->picture_coding_type;
    ctx->picture_structure = ctx->pic_params->picture_coding_extension.bits.picture_structure;

    ctx->forward_ref_surface = SURFACE(ctx->pic_params->forward_reference_picture);
    ctx->backward_ref_surface = SURFACE(ctx->pic_params->backward_reference_picture);

    /* Set picture type and reference indices for reference frames */
    if (ctx->picture_coding_type != PICTURE_CODING_I) {
        if (ctx->pic_params->picture_coding_extension.bits.is_first_field) { /* first field */
            if (ctx->backward_ref_surface) {
                ctx->picture_coding_type = PICTURE_CODING_B;
                ctx->ref_indexA = 0x01;/* Forward reference frame*/
                ctx->ref_indexB = 0x00;/* Backward reference frame*/
            } else {
                ctx->picture_coding_type =  PICTURE_CODING_P;
                ctx->ref_indexA = 0x00; /* Always reference frame 0*/
            }
        } else {
            if ((PICTURE_CODING_B == ctx->picture_coding_type) && (ctx->backward_ref_surface)) {
                ctx->picture_coding_type = PICTURE_CODING_B;
                ctx->ref_indexA = 0x01; /* Forward reference frame*/
                ctx->ref_indexB = 0x00;/* Backward reference frame */
            } else {
                ctx->picture_coding_type = PICTURE_CODING_P;
                if (ctx->forward_ref_surface) {
                    ctx->ref_indexA = 0x00;
                } else {
                    ctx->ref_indexA = 0x01;
                    ctx->ref_indexB = 0x00;
                }
            }
        }
    }
    ctx->pic_params->picture_coding_type = ctx->picture_coding_type;

    /* residual data size per element */
    if (ctx->picture_coding_type == PICTURE_CODING_I) {
        ctx->blk_size = 64; /* unsigned char */
    } else {
        ctx->blk_size = 2 * 64; /* unsigned short */
    }

    if (ctx->picture_coding_type != PICTURE_CODING_I) {
        if (ctx->backward_ref_surface) {
            if (ctx->forward_ref_surface == NULL)
                ctx->forward_ref_surface = ctx->backward_ref_surface;
        } else {
            ctx->backward_ref_surface = ctx->forward_ref_surface;
        }
    }
    if (NULL == ctx->backward_ref_surface) {
        ctx->backward_ref_surface = ctx->obj_context->current_render_target;
    }
    if (NULL == ctx->forward_ref_surface) {
        ctx->forward_ref_surface = ctx->obj_context->current_render_target;
    }

    ctx->coded_picture_width = ctx->pic_params->horizontal_size;
    ctx->coded_picture_height = ctx->pic_params->vertical_size;
    ctx->picture_width_mb = ctx->pic_params->horizontal_size / 16;
    if (ctx->pic_params->picture_coding_extension.bits.progressive_frame == 1) /* should be progressive_sequence? */
        ctx->picture_height_mb = (ctx->coded_picture_height + 15) / 16;
    else {
        if (FRAME_PICTURE != ctx->picture_structure) { /*Interlaced Field Pictures */
            ctx->picture_height_mb = ((ctx->coded_picture_height + 31) / 32);
        } else {
            ctx->picture_height_mb = 2 * ((ctx->coded_picture_height + 31) / 32);
        }
    }
    coded_pic_height = (ctx->picture_structure != FRAME_PICTURE) ?
                       ((ctx->picture_height_mb) * 32) : ((ctx->picture_height_mb) * 16);
    ctx->size_mb = ctx->picture_width_mb * (coded_pic_height >> 4);

    ctx->display_picture_size = 0;
    REGIO_WRITE_FIELD_LITE(ctx->display_picture_size, MSVDX_CMDS,
                           DISPLAY_PICTURE_SIZE, DISPLAY_PICTURE_HEIGHT, ctx->coded_picture_height - 1);
    REGIO_WRITE_FIELD_LITE(ctx->display_picture_size, MSVDX_CMDS,
                           DISPLAY_PICTURE_SIZE, DISPLAY_PICTURE_WIDTH, ctx->coded_picture_width - 1);

    ctx->coded_picture_size = 0;
    REGIO_WRITE_FIELD_LITE(ctx->coded_picture_size, MSVDX_CMDS,
                           CODED_PICTURE_SIZE, CODED_PICTURE_HEIGHT, ctx->coded_picture_height - 1);
    REGIO_WRITE_FIELD_LITE(ctx->coded_picture_size, MSVDX_CMDS,
                           CODED_PICTURE_SIZE, CODED_PICTURE_WIDTH, ctx->coded_picture_width - 1);

    ctx->obj_context->operating_mode = 0;
    REGIO_WRITE_FIELD(ctx->obj_context->operating_mode, MSVDX_CMDS, OPERATING_MODE, CHROMA_FORMAT,  1);
    REGIO_WRITE_FIELD(ctx->obj_context->operating_mode, MSVDX_CMDS, OPERATING_MODE, ASYNC_MODE,     1);
    /* 0 = VDMC and VDEB active.  1 = VDEB pass-thru. */
    REGIO_WRITE_FIELD(ctx->obj_context->operating_mode, MSVDX_CMDS, OPERATING_MODE,
                      CODEC_MODE, CODEC_MODE_MPEG2);
    REGIO_WRITE_FIELD(ctx->obj_context->operating_mode, MSVDX_CMDS, OPERATING_MODE,
                      CODEC_PROFILE,  CODEC_PROFILE_MPEG2_MAIN);
    REGIO_WRITE_FIELD(ctx->obj_context->operating_mode, MSVDX_CMDS, OPERATING_MODE,
                      ROW_STRIDE, (ctx->obj_context->current_render_target->psb_surface->stride_mode));

    return VA_STATUS_SUCCESS;
}



static void psb_MPEG2MC_QueryConfigAttributes(
    VAProfile profile,
    VAEntrypoint entrypoint,
    VAConfigAttrib *attrib_list,
    int num_attribs)
{
    /* No MPEG2 specific attributes */
}

static VAStatus psb_MPEG2MC_ValidateConfig(
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

static VAStatus psb__MPEG2MC_check_legal_picture(object_context_p obj_context, object_config_p obj_config)
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

    /* MSVDX decode capability for MPEG2:
     *     MP@HL
     *
     * Refer to Table 8-11 (Upper bounds for luminance sample rate) of ISO/IEC 13818-2: 1995(E),
     * and the "MSVDX MPEG2 decode capability" table of "Poulsbo Media Software Overview"
     */

    switch (obj_config->profile) {
    case VAProfileMPEG2Simple:
        if ((obj_context->picture_width <= 0) || (obj_context->picture_width > 352)
            || (obj_context->picture_height <= 0) || (obj_context->picture_height > 288)) {
            vaStatus = VA_STATUS_ERROR_RESOLUTION_NOT_SUPPORTED;
        }
        break;

    case VAProfileMPEG2Main:
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

static VAStatus psb_MPEG2MC_CreateContext(
    object_context_p obj_context,
    object_config_p obj_config)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    context_MPEG2MC_p ctx;

    /* Validate flag */
    /* Validate picture dimensions */
    vaStatus = psb__MPEG2MC_check_legal_picture(obj_context, obj_config);
    if (VA_STATUS_SUCCESS != vaStatus) {
        DEBUG_FAILURE;
        return vaStatus;
    }

    if (obj_context->num_render_targets < 1) {
        vaStatus = VA_STATUS_ERROR_UNKNOWN;
        DEBUG_FAILURE;
        return vaStatus;
    }

    ctx = (context_MPEG2MC_p) calloc(1, sizeof(struct context_MPEG2MC_s));
    if (NULL == ctx) {
        vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
        DEBUG_FAILURE;
        return vaStatus;
    }

    /* TODO: initialize ctx content */
    obj_context->format_data = (void*) ctx;
    ctx->obj_context = obj_context;
    ctx->pic_params = NULL;
    /* TODO create and map buffer */

    return vaStatus;
}


static void psb_MPEG2MC_DestroyContext(
    object_context_p obj_context)
{
    INIT_CONTEXT_MPEG2MC;

    /* TODO:unmap and destroy buffers */
    if (ctx->pic_params) {
        free(ctx->pic_params);
        ctx->pic_params = NULL;
    }

    free(obj_context->format_data);
    obj_context->format_data = NULL;
}

static VAStatus psb_MPEG2MC_BeginPicture(
    object_context_p obj_context)
{
    INIT_CONTEXT_MPEG2MC;

#if 0 /* clear surface for debugging */
    unsigned char *surface_data = NULL;
    static psb_surface_p target_surface = NULL;
    psb_surface_p tmp = ctx->obj_context->current_render_target->psb_surface;
    if (target_surface != tmp) { /* for field picture, only reset one time */
        target_surface = tmp;

        int ret = psb_buffer_map(&target_surface->buf, &surface_data);
        if (ret) {
            goto out;
        }
        memset(surface_data, 0x33, target_surface->size);
        psb_buffer_unmap(&target_surface->buf);
    }
out:
#endif

    if (ctx->pic_params) {
        free(ctx->pic_params);
        ctx->pic_params = NULL;
    }

    /* TODO: others */
    return VA_STATUS_SUCCESS;
}


static VAStatus psb_MPEG2MC_RenderPicture(
    object_context_p obj_context,
    object_buffer_p *buffers,
    int num_buffers)
{
    int i;
    INIT_CONTEXT_MPEG2MC;
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    ctx->mb_param = NULL; /* Init. compressed buffer pointers   */
    ctx->residual_buf = NULL;

    for (i = 0; i < num_buffers; i++) {
        object_buffer_p obj_buffer = buffers[i];

        switch (obj_buffer->type) {
            /* Picture parameters processing */
        case VAPictureParameterBufferType: {
            vaStatus = psb__MPEG2MC_process_picture_param(ctx, obj_buffer);
            DEBUG_FAILURE;
            break;
        }
        /* Macroblock Data processing */
        case VAMacroblockParameterBufferType: {
            ctx->mb_param = (VAMacroblockParameterBufferMPEG2 *)obj_buffer->buffer_data;
            ctx->mb_first_addr = ctx->mb_param->macroblock_address;
            ctx->mb_in_buffer = obj_buffer->num_elements;
            /* drv_debug_msg(VIDEO_DEBUG_GENERAL, "Macroblock count %d\n",ctx->mb_in_buffer); */
            break;
        }
        /* Residual Difference Data processing */
        case VAResidualDataBufferType: {
            /* store the data after VLD+IDCT */
            ctx->residual_buf = obj_buffer->psb_buffer;
            ctx->blk_in_buffer = obj_buffer->num_elements;
            break;
        }
        default:
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "Unhandled buffer type 0x%x\n", obj_buffer->type);
            break;
        }
    }

    /* Assuming residual and MB control buffers have been located */
    if (ctx->residual_buf && ctx->mb_param) {
        ctx->residual_sent = 0;
        ctx->residual_pendingDMA = 0;

        if (ctx->picture_coding_type == PICTURE_CODING_I) {
            psb__MPEG2MC_process_mbs_intra(ctx);
        } else {
            psb__MPEG2MC_process_mbs_interPB(ctx);
        }
        ctx->mb_param = NULL;
    }

    return VA_STATUS_SUCCESS;
}


static VAStatus psb_MPEG2MC_EndPicture(
    object_context_p obj_context)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    INIT_CONTEXT_MPEG2MC;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psb_MPEG2MC_EndPicture\n");

    if (psb_context_flush_cmdbuf(ctx->obj_context)) {
        vaStatus = VA_STATUS_ERROR_UNKNOWN;
    }

    return vaStatus;
}

/* TODO: integrate with VLD */
struct format_vtable_s psb_MPEG2MC_vtable = {
queryConfigAttributes:
    psb_MPEG2MC_QueryConfigAttributes,
validateConfig:
    psb_MPEG2MC_ValidateConfig,
createContext:
    psb_MPEG2MC_CreateContext,
destroyContext:
    psb_MPEG2MC_DestroyContext,
beginPicture:
    psb_MPEG2MC_BeginPicture,
renderPicture:
    psb_MPEG2MC_RenderPicture,
endPicture:
    psb_MPEG2MC_EndPicture
};
