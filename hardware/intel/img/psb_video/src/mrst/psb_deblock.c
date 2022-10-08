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
 *    Zeng Li <zeng.li@intel.com>
 *
 */

#include "psb_cmdbuf.h"

#include <unistd.h>
#include <stdio.h>

#include "hwdefs/mem_io.h"
#include "hwdefs/msvdx_offsets.h"
#include "hwdefs/reg_io2.h"
#include "hwdefs/msvdx_vec_reg_io2.h"
#include "hwdefs/msvdx_vdmc_reg_io2.h"
#include "hwdefs/msvdx_mtx_reg_io2.h"
#include "hwdefs/msvdx_dmac_linked_list.h"
#include "hwdefs/msvdx_rendec_mtx_slice_cntrl_reg_io2.h"
#include "hwdefs/msvdx_core_regs_io2.h"
#include "hwdefs/h264_macroblock_mem_io.h"
#include "hwdefs/dxva_cmdseq_msg.h"
#include "hwdefs/dxva_fw_ctrl.h"
#include "hwdefs/fwrk_msg_mem_io.h"
#include "hwdefs/dxva_msg.h"
#include "hwdefs/msvdx_cmds_io2.h"
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "psb_def.h"
#include "psb_drv_debug.h"

#define H264_MACROBLOCK_DATA_SIZE       0x80

#define MSVDX_CMDS_BASE         0x1000
#define MSVDX_CORE_BASE         0x600
#define MSVDX_VEC_BASE          0x800

#define MSVDX_DEBLOCK_REG_SET   0x10000000
#define MSVDX_DEBLOCK_REG_GET   0x20000000
#define MSVDX_DEBLOCK_REG_POLLn 0x30000000
#define MSVDX_DEBLOCK_REG_POLLx 0x40000000

static int reg_set_count = 0;
static int reg_get_count = 0;
static int reg_poll_x = 0;
static int reg_poll_n = 0;

#define psb_deblock_reg_set(group, reg, value) \
        *cmdbuf->regio_idx++ = (group##_##reg##_##OFFSET + group##_##BASE) | MSVDX_DEBLOCK_REG_SET;     \
        *cmdbuf->regio_idx++ = value; reg_set_count++;

#define psb_deblock_reg_set_RELOC( group, reg, buffer, buffer_offset, dst)             \
        *cmdbuf->regio_idx++ = (group##_##reg##_##OFFSET + group##_##BASE) | MSVDX_DEBLOCK_REG_SET;  \
        RELOC_REGIO(*cmdbuf->regio_idx++, buffer_offset, buffer, dst); reg_set_count++;

#define psb_deblock_reg_table_set(group, reg, index, value)             \
        *cmdbuf->regio_idx++ = ( (group##_##reg##_OFFSET + group##_##BASE + index*group##_##reg##_STRIDE) | MSVDX_DEBLOCK_REG_SET); \
        *cmdbuf->regio_idx++ = value; reg_set_count++;

#define psb_deblock_reg_get(group, reg) \
        *cmdbuf->regio_idx++ = (group##_##reg##_##OFFSET + group##_##BASE) | MSVDX_DEBLOCK_REG_GET; reg_get_count++;

#if 1
#define h264_pollForSpaceForNCommands(NumCommands)\
        *cmdbuf->regio_idx++ = (MSVDX_CORE_CR_MSVDX_COMMAND_SPACE_OFFSET + MSVDX_CORE_BASE) | MSVDX_DEBLOCK_REG_POLLn; \
        *cmdbuf->regio_idx++ = NumCommands; reg_poll_n++;
#else
#define h264_pollForSpaceForNCommands(NumCommands)              /* Seems not needed, so just define it null */
#endif

#define PollForSpaceForXCommands  \
        *cmdbuf->regio_idx++ = (MSVDX_CORE_CR_MSVDX_COMMAND_SPACE_OFFSET + MSVDX_CORE_BASE) | MSVDX_DEBLOCK_REG_POLLx; reg_poll_x++;

typedef enum {
    H264_BLOCKSIZE_16X16                = 0, /* 1 block */
    H264_BLOCKSIZE_16X8                 = 1, /* 2 blocks */
    H264_BLOCKSIZE_8X16                 = 2,
    H264_BLOCKSIZE_8X8                  = 3, /* 4 blocks */
    H264_BLOCKSIZE_8X4                  = 4,
    H264_BLOCKSIZE_4X8                  = 5,
    H264_BLOCKSIZE_4X4                  = 6

} h264_eBlockSize;

static  uint32_t        BlockDownsizeMap[] = {
    1, 1, 3, 3, 3, 5, 5
};

static  uint32_t        BlocksToSendMap[] = {
    1, 2, 2, 4, 4, 4, 4
};

static  uint32_t        BlockAddressMap[7][4] = {
    { 0 },                                                                                      /* 16x16        */
    { 0, 2 },                                                                           /* 16x8         */
    { 0, 1 },                                                                           /* 8x16         */
    { 0, 1, 2, 3 },                                                                     /* 8x8          */
    { 0, 1, 2, 3 },                                                                     /* 8x4          */
    { 0, 1, 2, 3 },                                                                     /* 4x8          */
    { 0, 1, 2, 3 }                                                                      /* 4x4          */
};

static  uint32_t        VectorsToSendMap[] = {
    1, 1, 1, 1, 2, 2, 4
};

static  uint32_t        VectorOffsetMap[7][4] = {
    { 0 },                                                                                      /* 16x16        */
    { 0 },                                                                                      /* 16x8         */
    { 0 },                                                                                      /* 8x16         */
    { 0 },                                                                                      /* 8x8          */
    { 0, 2 },                                                                           /* 8x4          */
    { 0, 1 },                                                                           /* 4x8          */
    { 0, 1, 2, 3 }                                                                      /* 4x4          */
};


static  uint32_t        Above1AboveTileMap[] = {
    /*  9, 12, 8, 13    */
    13, 12, 9, 8
};

static  uint32_t        CurrentAboveTileMap[] = {
    /*  11, 14, 10, 15  */
    15, 14, 11, 10
};

static  uint32_t        CurrentColTileMap[] = {
    10, 15, 0, 5, 8, 13, 2, 7, 1, 4, 3, 6, 9, 12
};


static  uint32_t        ColBlockMap[] = {
    2, 3, 0, 1
};

void
h264_above1InterBlockSequence(psb_cmdbuf_p cmdbuf, uint8_t* MbData)
{
    uint32_t    i, BlockNum, Mv, MvAddr, Value;
    uint32_t    Block8x8, blockType;
    uint32_t    InterBlockCmd;
    uint32_t    MotionVecCmd[16];
    uint32_t    BlockType[4] = {0};
    uint32_t    DpbIdx[4];

    /* set MV vars to 0 */
    for (i = 0; i < 16; i++) {
        MotionVecCmd[i] = 0;
    }

    /* Read the size of blocks 2 and 3 and resize them so they are all ?x8 */
    Value = MEMIO_READ_FIELD(MbData, MSVDX_VEC_ENTDEC_VLRIF_H264_MB_UNIT_ASO_BLOCK2_PREDICTION_SIZE);
    if (Value > (sizeof(BlockDownsizeMap) / sizeof(uint32_t) - 1))
        Value = sizeof(BlockDownsizeMap) / sizeof(uint32_t) - 1;
    BlockType[2] = BlockDownsizeMap[Value];
    Value = MEMIO_READ_FIELD(MbData, MSVDX_VEC_ENTDEC_VLRIF_H264_MB_UNIT_ASO_BLOCK3_PREDICTION_SIZE);
    if (Value > (sizeof(BlockDownsizeMap) / sizeof(uint32_t) - 1))
        Value = sizeof(BlockDownsizeMap) / sizeof(uint32_t) - 1;
    BlockType[3] = BlockDownsizeMap[Value];

    /* read motion vectors for the bottom row, but store them in the correct locn. for ?x8 blocks */
    for (i = 0; i < 4; i++) {
        Value = MEMIO_READ_TABLE_FIELD(MbData, MSVDX_VEC_ENTDEC_VLRIF_H264_MB_UNIT_COMP_X_ABOVE, i);
        REGIO_WRITE_FIELD(MotionVecCmd[Above1AboveTileMap[i]], MSVDX_CMDS, MOTION_VECTOR, MV_X, Value);
        Value = MEMIO_READ_TABLE_FIELD(MbData, MSVDX_VEC_ENTDEC_VLRIF_H264_MB_UNIT_COMP_Y_ABOVE, i);
        REGIO_WRITE_FIELD(MotionVecCmd[Above1AboveTileMap[i]], MSVDX_CMDS, MOTION_VECTOR, MV_Y, Value);
    }

    /* read DPB index for blocks 2 and 3 */
    for (i = 0; i < 2; i++) {
        DpbIdx[ColBlockMap[i]] = MEMIO_READ_TABLE_FIELD(MbData, MSVDX_VEC_ENTDEC_VLRIF_H264_MB_UNIT_DPB_IDX_COL, i);
    }

    /* Send commands required blocks */
    for (BlockNum = BlocksToSendMap[BlockType[2]] / 2;
         BlockNum < BlocksToSendMap[BlockType[2]];
         BlockNum++) {
        /* block address */
        Block8x8 = BlockAddressMap[BlockType[2]][BlockNum];
        /* block type */
        blockType = BlockType[Block8x8];

        InterBlockCmd = 0;
        REGIO_WRITE_FIELD(InterBlockCmd, MSVDX_CMDS, INTER_BLOCK_PREDICTION, INTER_PRED_BLOCK_SIZE, blockType);
        REGIO_WRITE_FIELD(InterBlockCmd, MSVDX_CMDS, INTER_BLOCK_PREDICTION, REF_INDEX_A_VALID, 1);
        REGIO_WRITE_FIELD(InterBlockCmd, MSVDX_CMDS, INTER_BLOCK_PREDICTION, REF_INDEX_A, DpbIdx[Block8x8]);

        /* send commands */
        h264_pollForSpaceForNCommands(1 + VectorsToSendMap[blockType]);
        psb_deblock_reg_table_set(MSVDX_CMDS, INTER_BLOCK_PREDICTION_ABOVE1, Block8x8, InterBlockCmd);

        for (i = 0; i < VectorsToSendMap[blockType]; i++) {
            /* only send forward MVs in baseline */
            MvAddr = (2 * 4 * Block8x8) + VectorOffsetMap[blockType][i];
            Mv = (4 * Block8x8) + VectorOffsetMap[blockType][i];
            psb_deblock_reg_table_set(MSVDX_CMDS, MOTION_VECTOR_ABOVE1, MvAddr, MotionVecCmd[Mv]);
        }
    }
}

void
h264_currentInterBlockSequence(psb_cmdbuf_p cmdbuf, uint8_t * MbData)
{
    uint32_t    i, BlockNum, Mv, MvAddr, Value;
    uint32_t    Block8x8, blockType;
    uint32_t    InterBlockCmd;
    uint32_t    MotionVecCmd[16];
    uint32_t    BlockType[4];
    uint32_t    DpbIdx[4];

    /* set MV vars to 0 */
    for (i = 0; i < 16; i++) {
        MotionVecCmd[i] = 0;
    }

    /* read block size */
    BlockType[0] = MEMIO_READ_FIELD(MbData, MSVDX_VEC_ENTDEC_VLRIF_H264_MB_UNIT_ASO_BLOCK0_PREDICTION_SIZE);
    BlockType[1] = MEMIO_READ_FIELD(MbData, MSVDX_VEC_ENTDEC_VLRIF_H264_MB_UNIT_ASO_BLOCK1_PREDICTION_SIZE);
    BlockType[2] = MEMIO_READ_FIELD(MbData, MSVDX_VEC_ENTDEC_VLRIF_H264_MB_UNIT_ASO_BLOCK2_PREDICTION_SIZE);
    BlockType[3] = MEMIO_READ_FIELD(MbData, MSVDX_VEC_ENTDEC_VLRIF_H264_MB_UNIT_ASO_BLOCK3_PREDICTION_SIZE);

    /* read motion vectors in all 16 4x4 sub-blocks*/
    for (i = 1; i < 3; i++) {   /* get blocks 11 and 14 */
        Value = MEMIO_READ_TABLE_FIELD(MbData, MSVDX_VEC_ENTDEC_VLRIF_H264_MB_UNIT_COMP_X_ABOVE, i);
        REGIO_WRITE_FIELD(MotionVecCmd[CurrentAboveTileMap[i]], MSVDX_CMDS, MOTION_VECTOR, MV_X, Value);
        Value = MEMIO_READ_TABLE_FIELD(MbData, MSVDX_VEC_ENTDEC_VLRIF_H264_MB_UNIT_COMP_Y_ABOVE, i);
        REGIO_WRITE_FIELD(MotionVecCmd[CurrentAboveTileMap[i]], MSVDX_CMDS, MOTION_VECTOR, MV_Y, Value);
    }
    for (i = 0; i < 14; i++) {
        Value = MEMIO_READ_TABLE_FIELD(MbData, MSVDX_VEC_ENTDEC_VLRIF_H264_MB_UNIT_COMP_X_COL, i);
        REGIO_WRITE_FIELD(MotionVecCmd[CurrentColTileMap[i]], MSVDX_CMDS, MOTION_VECTOR, MV_X, Value);
        Value = MEMIO_READ_TABLE_FIELD(MbData, MSVDX_VEC_ENTDEC_VLRIF_H264_MB_UNIT_COMP_Y_COL, i);
        REGIO_WRITE_FIELD(MotionVecCmd[CurrentColTileMap[i]], MSVDX_CMDS, MOTION_VECTOR, MV_Y, Value);
    }

    /* read DPB index for all 4 blocks */
    for (i = 0; i < 4; i++) {
        DpbIdx[ColBlockMap[i]] = MEMIO_READ_TABLE_FIELD(MbData, MSVDX_VEC_ENTDEC_VLRIF_H264_MB_UNIT_DPB_IDX_COL, i);
    }

    /* Send commands required blocks */
    for (BlockNum = 0;
         BlockNum < BlocksToSendMap[BlockType[0]];
         BlockNum++) {
        /* block address */
        Block8x8 = BlockAddressMap[BlockType[0]][BlockNum];
        /* block type */
        blockType = BlockType[Block8x8];

        InterBlockCmd = 0;
        REGIO_WRITE_FIELD(InterBlockCmd, MSVDX_CMDS, INTER_BLOCK_PREDICTION, INTER_PRED_BLOCK_SIZE, blockType);
        REGIO_WRITE_FIELD(InterBlockCmd, MSVDX_CMDS, INTER_BLOCK_PREDICTION, REF_INDEX_A_VALID, 1);
        REGIO_WRITE_FIELD(InterBlockCmd, MSVDX_CMDS, INTER_BLOCK_PREDICTION, REF_INDEX_A, DpbIdx[Block8x8]);

        /* send commands */
        h264_pollForSpaceForNCommands(1 + VectorsToSendMap[blockType]);
        psb_deblock_reg_table_set(MSVDX_CMDS, INTER_BLOCK_PREDICTION, Block8x8, InterBlockCmd);

        for (i = 0; i < VectorsToSendMap[blockType]; i++) {
            /* only send forward MVs in baseline */
            MvAddr = (2 * 4 * Block8x8) + VectorOffsetMap[blockType][i];
            Mv = (4 * Block8x8) + VectorOffsetMap[blockType][i];
            psb_deblock_reg_table_set(MSVDX_CMDS, MOTION_VECTOR, MvAddr, MotionVecCmd[Mv]);
        }
    }
}

void
h264_currentIntraBlockPrediction(psb_cmdbuf_p cmdbuf, uint8_t * MbData, int bMbIsIPCM)
{
    uint32_t    BlockSizeY, BlockSizeC;
    uint32_t    IntraCmdY, IntraCmdC;

    /* select block size I_PCM or I_16x16 */
    BlockSizeY = (1 == bMbIsIPCM) ? 3 : 0;
    /* select block size I_PCM or I_8x8 */
    BlockSizeC = (1 == bMbIsIPCM) ? 3 : 1;

    IntraCmdY = IntraCmdC = 0;

    REGIO_WRITE_FIELD(
        IntraCmdY,
        MSVDX_CMDS, INTRA_BLOCK_PREDICTION,
        INTRA_PRED_BLOCK_SIZE,
        BlockSizeY);
    REGIO_WRITE_FIELD(
        IntraCmdC,
        MSVDX_CMDS, INTRA_BLOCK_PREDICTION,
        INTRA_PRED_BLOCK_SIZE,
        BlockSizeC);

    h264_pollForSpaceForNCommands(2);
    psb_deblock_reg_table_set(MSVDX_CMDS, INTRA_BLOCK_PREDICTION, 0, IntraCmdY);
    psb_deblock_reg_table_set(MSVDX_CMDS, INTRA_BLOCK_PREDICTION, 4, IntraCmdC);
}

void
h264_above1IntraBlockPrediction(psb_cmdbuf_p cmdbuf, uint8_t * MbData, int bMbIsIPCM)
{
    uint32_t    BlockSizeY;
    uint32_t    IntraCmdY;

    /* select block size I_PCM or I_16x16 */
    BlockSizeY = (1 == bMbIsIPCM) ? 3 : 0;
    IntraCmdY = 0;

    REGIO_WRITE_FIELD(
        IntraCmdY,
        MSVDX_CMDS, INTRA_BLOCK_PREDICTION_ABOVE1,
        INTRA_PRED_BLOCK_SIZE_ABOVE1,
        BlockSizeY);

    h264_pollForSpaceForNCommands(1);
    psb_deblock_reg_table_set(MSVDX_CMDS, INTRA_BLOCK_PREDICTION_ABOVE1, 0, IntraCmdY);
}

void
h264_macroblockCmdSequence(psb_cmdbuf_p cmdbuf, uint8_t * MbData, uint32_t X, uint32_t  Y, int bCurrent)
{
    int bMbIsIPCM;
    uint32_t    MbType;
    uint32_t    Value;
    uint32_t    MbNumberCmd, MbQuantCmd, MbTransZeroCmd;

    /* Macroblock Type */
    MbType  = MEMIO_READ_FIELD(MbData, MSVDX_VEC_ENTDEC_VLRIF_H264_MB_UNIT_MBTYPE);

    /* Macroblock Number */
    /* no need for MB_ERROR_FLAG as error info is not stored */
    /* no need for MB_FIELD_CODE as basline is always frame based */
    MbNumberCmd = 0;

    Value = (3 == MbType) ? 0 : MbType;
    REGIO_WRITE_FIELD(MbNumberCmd, MSVDX_CMDS, MACROBLOCK_NUMBER, MB_CODE_TYPE, Value);

    REGIO_WRITE_FIELD(MbNumberCmd, MSVDX_CMDS, MACROBLOCK_NUMBER, MB_NO_Y, Y);
    REGIO_WRITE_FIELD(MbNumberCmd, MSVDX_CMDS, MACROBLOCK_NUMBER, MB_NO_X, X);

    /* H264 Quant */
    /* TRANSFORM_SIZE_8X8 always false in basline */
    MbQuantCmd = 0;

    Value = MEMIO_READ_FIELD(MbData, MSVDX_VEC_ENTDEC_VLRIF_H264_MB_UNIT_QP_CR);
    REGIO_WRITE_FIELD(MbQuantCmd, MSVDX_CMDS, MACROBLOCK_H264_QUANT, MB_QUANT_CHROMA_CR, Value);

    Value = MEMIO_READ_FIELD(MbData, MSVDX_VEC_ENTDEC_VLRIF_H264_MB_UNIT_QP_CB);
    REGIO_WRITE_FIELD(MbQuantCmd, MSVDX_CMDS, MACROBLOCK_H264_QUANT, MB_QUANT_CHROMA_CB, Value);

    Value = MEMIO_READ_FIELD(MbData, MSVDX_VEC_ENTDEC_VLRIF_H264_MB_UNIT_QP);
    REGIO_WRITE_FIELD(MbQuantCmd, MSVDX_CMDS, MACROBLOCK_H264_QUANT, MB_QUANT_LUMA, Value);

    /* send 2 commands */
    h264_pollForSpaceForNCommands(2);
    if (1 == bCurrent) {
        /* Top and Left available flags are only sent for current MB */
        Value = MEMIO_READ_FIELD(MbData, MSVDX_VEC_ENTDEC_VLRIF_H264_MB_UNIT_MB_AVAILABLE_TOP_FLAG);
        Value = (1 == Value) ? 0 : 1;
        REGIO_WRITE_FIELD(MbNumberCmd, MSVDX_CMDS, MACROBLOCK_NUMBER, MB_SLICE_TOP, Value);

        Value = MEMIO_READ_FIELD(MbData, MSVDX_VEC_ENTDEC_VLRIF_H264_MB_UNIT_MB_AVAILABLE_LEFT_FLAG);
        Value = (1 == Value) ? 0 : 1;
        REGIO_WRITE_FIELD(MbNumberCmd, MSVDX_CMDS, MACROBLOCK_NUMBER, MB_SLICE_LHS, Value);

        psb_deblock_reg_set(MSVDX_CMDS, MACROBLOCK_NUMBER, MbNumberCmd);
        psb_deblock_reg_set(MSVDX_CMDS, MACROBLOCK_H264_QUANT, MbQuantCmd);
    } else {
        psb_deblock_reg_set(MSVDX_CMDS, MACROBLOCK_NUMBER_ABOVE1, MbNumberCmd);
        psb_deblock_reg_set(MSVDX_CMDS, MACROBLOCK_H264_QUANT_ABOVE1, MbQuantCmd);
    }

    /* Prediction Block Sequence */
    bMbIsIPCM = 0;
    switch (MbType) {
    case 3:             /* IPCM */
        bMbIsIPCM = 1;
        /* deliberate drop through */
    case 0:             /* I */
        if (1 == bCurrent) {
            h264_currentIntraBlockPrediction(cmdbuf, MbData, bMbIsIPCM);
        } else {
            h264_above1IntraBlockPrediction(cmdbuf, MbData, bMbIsIPCM);
        }
        break;
    case 1:             /* P */
        if (1 == bCurrent) {
            h264_currentInterBlockSequence(cmdbuf, MbData);
        } else {
            h264_above1InterBlockSequence(cmdbuf, MbData);
        }
        break;
    case 2:             /* B */
    default:
        /* invalid MB type */
        //IMG_ASSERT( 0 );
        break;
    }

    /* Trasform Zero */
    MbTransZeroCmd = 0;
    Value = MEMIO_READ_FIELD(MbData, MSVDX_VEC_ENTDEC_VLRIF_H264_MB_UNIT_ASO_MB_TRANSFORM_ZERO);
    REGIO_WRITE_FIELD(MbTransZeroCmd, MSVDX_CMDS, MACROBLOCK_BLOCK_TRANSFORM_ZERO, MB_BLOCK_TRANSFORM_ZERO, Value);

    /* write last command */
    h264_pollForSpaceForNCommands(1);
    if (1 == bCurrent) {
        psb_deblock_reg_set(MSVDX_CMDS, MACROBLOCK_BLOCK_TRANSFORM_ZERO, MbTransZeroCmd);
    } else {
        MbTransZeroCmd &= 0x0000CC00;   /* only send for sub-blocks 10,11,14 and 15 */
        psb_deblock_reg_set(MSVDX_CMDS, MACROBLOCK_BLOCK_TRANSFORM_ZERO_ABOVE1, MbTransZeroCmd);
    }
}

uint32_t
h264_getCurrentSliceCmd(uint8_t* MbData)
{
    uint32_t    Value, Cmd;

    Cmd = 0;

    /* unpack data from stored MB + repack in command */
    Value = MEMIO_READ_FIELD(MbData, MSVDX_VEC_ENTDEC_VLRIF_H264_MB_UNIT_COPY_DISABLE_DEBLOCK_FILTER_IDC);
    REGIO_WRITE_FIELD(Cmd, MSVDX_CMDS, SLICE_PARAMS, DISABLE_DEBLOCK_FILTER_IDC, Value);

    Value = MEMIO_READ_FIELD(MbData, MSVDX_VEC_ENTDEC_VLRIF_H264_MB_UNIT_COPY_H264_BE_SLICE0_ALPHA_CO_OFFSET_DIV2);
    REGIO_WRITE_FIELD(Cmd, MSVDX_CMDS, SLICE_PARAMS, SLICE_ALPHA_CO_OFFSET_DIV2, Value);

    Value = MEMIO_READ_FIELD(MbData, MSVDX_VEC_ENTDEC_VLRIF_H264_MB_UNIT_COPY_H264_BE_SLICE0_BETA_OFFSET_DIV2);
    REGIO_WRITE_FIELD(Cmd, MSVDX_CMDS, SLICE_PARAMS, SLICE_BETA_OFFSET_DIV2, Value);

    Value = MEMIO_READ_FIELD(MbData, MSVDX_VEC_ENTDEC_VLRIF_H264_MB_UNIT_COPY_H264_BE_SLICE0_FIELD_TYPE);
    REGIO_WRITE_FIELD(Cmd, MSVDX_CMDS, SLICE_PARAMS, SLICE_FIELD_TYPE, Value);

    Value = MEMIO_READ_FIELD(MbData, MSVDX_VEC_ENTDEC_VLRIF_H264_MB_UNIT_COPY_H264_BE_SLICE0_CODE_TYPE);
    REGIO_WRITE_FIELD(Cmd, MSVDX_CMDS, SLICE_PARAMS, SLICE_CODE_TYPE, Value);

    return Cmd;
}

int h264_secondPass(
    psb_cmdbuf_p cmdbuf,
    uint8_t     * MbData,
    uint32_t    OperatingModeCmd,
    uint32_t    Width,
    uint32_t    Height
)
{
    uint32_t    i, PicSize;
    uint32_t    EndOfPictureCmd;
    uint32_t    EndOfSliceCmd;
    uint32_t    SliceCmd, OldSliceCmd;
    uint32_t    EnableReg;
    uint8_t     * CurrMb;
    uint8_t     * Above1Mb;
    int bRetCode = 0;

    PicSize = Width * Height;

    /* End of Slice command */
    EndOfSliceCmd = 0;
    REGIO_WRITE_FIELD(EndOfSliceCmd, MSVDX_CMDS, END_SLICE_PICTURE, PICTURE_END, 0);


    /* put vdeb into second pass mode */
    /* send operating mode command for VDEB only */
    REGIO_WRITE_FIELD(OperatingModeCmd,  MSVDX_CMDS, OPERATING_MODE, ASYNC_MODE, 2);   /* VDEB only */
    h264_pollForSpaceForNCommands(1);
    psb_deblock_reg_get(MSVDX_CORE, CR_MSVDX_COMMAND_SPACE);
    psb_deblock_reg_set(MSVDX_CMDS, OPERATING_MODE, OperatingModeCmd);

    PollForSpaceForXCommands;

    /* Send Slice Command */
    SliceCmd = h264_getCurrentSliceCmd(MbData);
    h264_pollForSpaceForNCommands(2);
    psb_deblock_reg_set(MSVDX_CMDS, SLICE_PARAMS, SliceCmd);
    psb_deblock_reg_set(MSVDX_CMDS, SLICE_PARAMS_ABOVE1, SliceCmd);
    /* process top row */
    for (i = 0, CurrMb = MbData;
         i < Width;
         i++, CurrMb += H264_MACROBLOCK_DATA_SIZE) {
        OldSliceCmd = SliceCmd;
        SliceCmd = h264_getCurrentSliceCmd(CurrMb);
        if (OldSliceCmd != SliceCmd) {
            h264_pollForSpaceForNCommands(2);
            psb_deblock_reg_set(MSVDX_CMDS, END_SLICE_PICTURE, EndOfSliceCmd);
            psb_deblock_reg_set(MSVDX_CMDS, SLICE_PARAMS, SliceCmd);
        }

        h264_macroblockCmdSequence(cmdbuf, CurrMb, i, 0, 1);
    }

    /* process rest of picture */
    for (Above1Mb = MbData;
         i < PicSize;
         i++, CurrMb += H264_MACROBLOCK_DATA_SIZE, Above1Mb += H264_MACROBLOCK_DATA_SIZE) {
        OldSliceCmd = SliceCmd;
        SliceCmd = h264_getCurrentSliceCmd(CurrMb);
        if (OldSliceCmd != SliceCmd) {
            h264_pollForSpaceForNCommands(2);
            psb_deblock_reg_set(MSVDX_CMDS, END_SLICE_PICTURE, EndOfSliceCmd);
            psb_deblock_reg_set(MSVDX_CMDS, SLICE_PARAMS, SliceCmd);
        }

        h264_macroblockCmdSequence(cmdbuf, Above1Mb, (i % Width), (i / Width) - 1, 0);
        h264_macroblockCmdSequence(cmdbuf, CurrMb, (i % Width), (i / Width), 1);
    }

    /* send end of pic + restart back end */
    EndOfPictureCmd = 0;
    EnableReg = 0;
    REGIO_WRITE_FIELD(EndOfPictureCmd, MSVDX_CMDS, END_SLICE_PICTURE, PICTURE_END, 1);
    REGIO_WRITE_FIELD(EnableReg, MSVDX_VEC, CR_VEC_CONTROL, ENTDEC_ENABLE_BE, 1);

    h264_pollForSpaceForNCommands(2);
    psb_deblock_reg_set(MSVDX_CMDS, END_SLICE_PICTURE, EndOfSliceCmd);
    psb_deblock_reg_set(MSVDX_CMDS, END_SLICE_PICTURE, EndOfPictureCmd);
    //psb_deblock_reg_set( MSVDX_VEC, CR_VEC_CONTROL, EnableReg);                       /* this is not a command */
    return bRetCode;
}

int psb_cmdbuf_second_pass(object_context_p obj_context,
                           uint32_t OperatingModeCmd,
                           unsigned char * pvParamBase,
                           uint32_t PicWidthInMbs,
                           uint32_t FrameHeightInMbs,
                           psb_buffer_p target_buffer,
                           uint32_t chroma_offset
                          )
{
    int bRetVal = 1;
    uint32_t Cmd, item_loc;
    uint32_t *cmd_size;
    psb_cmdbuf_p cmdbuf =  obj_context->cmdbuf;

    if (psb_buffer_map(&cmdbuf->regio_buf, &cmdbuf->regio_base)) {
        //printf("map cmdbuf->regio_buf error\n");
        return bRetVal;
    }

    item_loc = psb_cmdbuf_buffer_ref(cmdbuf, &cmdbuf->regio_buf);

    cmdbuf->regio_idx = (uint32_t *)cmdbuf->regio_base;
    cmd_size = cmdbuf->regio_idx++;

    h264_pollForSpaceForNCommands(4);

    psb_deblock_reg_set_RELOC(MSVDX_CMDS, LUMA_RECONSTRUCTED_PICTURE_BASE_ADDRESSES, target_buffer, target_buffer->buffer_ofs, item_loc);
    psb_deblock_reg_set_RELOC(MSVDX_CMDS, CHROMA_RECONSTRUCTED_PICTURE_BASE_ADDRESSES, target_buffer, target_buffer->buffer_ofs + chroma_offset, item_loc);

    Cmd = 0;
    REGIO_WRITE_FIELD_LITE(Cmd, MSVDX_CMDS, DISPLAY_PICTURE_SIZE, DISPLAY_PICTURE_HEIGHT, (FrameHeightInMbs * 16) - 1);
    REGIO_WRITE_FIELD_LITE(Cmd, MSVDX_CMDS, DISPLAY_PICTURE_SIZE, DISPLAY_PICTURE_WIDTH, (PicWidthInMbs * 16) - 1);
    psb_deblock_reg_set(MSVDX_CMDS, DISPLAY_PICTURE_SIZE, Cmd);


    Cmd = 0;
    REGIO_WRITE_FIELD_LITE(Cmd, MSVDX_CMDS, CODED_PICTURE_SIZE, CODED_PICTURE_HEIGHT, (FrameHeightInMbs * 16) - 1);
    REGIO_WRITE_FIELD_LITE(Cmd, MSVDX_CMDS, CODED_PICTURE_SIZE, CODED_PICTURE_WIDTH, (PicWidthInMbs * 16) - 1);
    psb_deblock_reg_set(MSVDX_CMDS, CODED_PICTURE_SIZE, Cmd);

    /* BRN25312 */
    psb_deblock_reg_set(MSVDX_CMDS, VC1_LUMA_RANGE_MAPPING_BASE_ADDRESS, 0);
    psb_deblock_reg_set(MSVDX_CMDS, VC1_CHROMA_RANGE_MAPPING_BASE_ADDRESS, 0);
    psb_deblock_reg_set(MSVDX_CMDS, VC1_RANGE_MAPPING_FLAGS, 0);

    /* process second pass */
    bRetVal = h264_secondPass(cmdbuf,
                              (uint8_t*)pvParamBase,
                              OperatingModeCmd,
                              PicWidthInMbs,
                              FrameHeightInMbs);

    *cmd_size = (cmdbuf->regio_idx - (uint32_t *)cmdbuf->regio_base - 1);

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "DEBLOCK: REGIO size is %d\n", (uint32_t)(cmdbuf->regio_idx - (uint32_t *)cmdbuf->regio_base) - 1);
    //printf("DEBLOCK: REGIO size is %d\n", (uint32_t)(cmdbuf->regio_idx - (uint32_t *)cmdbuf->regio_base) - 1);
    psb_buffer_unmap(&cmdbuf->regio_buf);
    return bRetVal;
}
