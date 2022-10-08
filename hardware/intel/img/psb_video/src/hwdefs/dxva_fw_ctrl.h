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


/******************************************************************************

 @File         dxva_fw_ctrl.h

 @Title        va Firmware Control Allocation Commands

 @Platform

 @Description  Defined commands that may be placed int the Control Allocation

******************************************************************************/
#ifndef _VA_FW_CTRL_H_
#define _VA_FW_CTRL_H_


#define CMD_MASK                                                (0xF0000000)

/* No Operation */
#define CMD_NOP                                                 (0x00000000)

/* Register Value Pair Block */
#define CMD_REGVALPAIR_WRITE                    (0x10000000)
#define CMD_REGVALPAIR_COUNT_MASK               (0x000FFFFF)
#define CMD_REGVALPAIR_COUNT_SHIFT              (0)

#define CMD_REGVALPAIR_FLAG_MB_LAYER    (0x00100000)
#define CMD_REGVALPAIR_FLAG_HL_LAYER    (0x00200000)
#define CMD_REGVALPAIR_FLAG_PRELOAD             (0x00400000)
#define CMD_REGVALPAIR_FLAG_VC1PATCH            (0x00800000)

#define CMD_REGVALPAIR_FORCE_MASK               (0x08000000)

/* Rendec Write Block */
#define CMD_RENDEC_WRITE                                (0x20000000)
#define CMD_RENDEC_BLOCK                                (0x50000000)
#define CMD_RENDEC_COUNT_MASK                   (0x000FFFFF)
#define CMD_RENDEC_COUNT_SHIFT                  (0)

/* Rendec Block */
#define CMD_RENDEC_BLOCK_FLAG_VC1_CMD_PATCH     (0x01000000)
#define CMD_RENDEC_BLOCK_FLAG_VC1_BE_PATCH      (0x02000000)
#define CMD_RENDEC_BLOCK_FLAG_VC1_SP_PATCH      (0x04000000)
#define CMD_RENDEC_BLOCK_FLAG_VC1_IC_PATCH      (0x08000000)

/* Command Allocation temination Commands */
#define CMD_COMPLETION                                  (0x60000000)

/* Use this to notify mxt of the context */
#define CMD_HEADER                                      (0x70000000)
#define CMD_HEADER_CONTEXT_MASK         (0x0fffffff)

#define CMD_CONDITIONAL_SKIP            (0x80000000)

#define CMD_HEADER_VC1                          (0x90000000)

#define CMD_PARSE_HEADER                                        (0xF0000000)
#define CMD_PARSE_HEADER_NEWSLICE                       (0x00000001)

/*****************************************************************/
/* DMA */
/*****************************************************************/
#define CMD_DMA                                                                 (0xE0000000)
#define CMD_DMA_DMA_TYPE_MASK                                   (0x0ff00000)
#define CMD_DMA_DMA_TYPE_SHIFT                                  ( 24 )
#define CMD_DMA_DMA_SIZE_MASK                                   (0x0000ffff)
#define CMD_DMA_OFFSET_FLAG                                             (1<<19)

#define CMD_DMA_TYPE_VLC_TABLE                                  ( 0 << CMD_DMA_DMA_TYPE_SHIFT )
#define CMD_DMA_TYPE_RESIDUAL                                   ( 1 << CMD_DMA_DMA_TYPE_SHIFT )
#define CMD_DMA_TYPE_IDCT_INSERTION                             ( 2 << CMD_DMA_DMA_TYPE_SHIFT )
#define CMD_DMA_TYPE_PROBABILITY_DATA                   ( 3 << CMD_DMA_DMA_TYPE_SHIFT )

typedef struct _RENDER_BUFFER_HEADER_VC1_TAG {
    IMG_UINT32 ui32Cmd;
    IMG_UINT32 ui32RangeMappingBase[2]; /* Store flags in bottom bits of [0] */
    IMG_UINT32 ui32SliceParams;
    union {
        struct  _LLDMA_VC1_ {
            IMG_UINT32          ui32PreloadSave;
            IMG_UINT32          ui32PreloadRestore;
        }               LLDMA_VC1;
    }   ui32LLDMAPointers;

} RENDER_BUFFER_HEADER_VC1;

typedef struct _RENDER_BUFFER_HEADER_TAG {
    IMG_UINT32 ui32Cmd;
    IMG_UINT32 ui32Reserved; /* used as ui32SliceParams in MPEG4 */
    union {
        struct  _LLDMA_MPEG4_ {
            IMG_UINT32          ui32FEStatesSave;
            IMG_UINT32          ui32FEStatesRestore;
        }               LLDMA_MPEG4;

        struct _LLDMA_H264_ {
            IMG_UINT32          ui32PreloadSave;
            IMG_UINT32          ui32PreloadRestore;

        }               LLDMA_H264;
    }   ui32LLDMAPointers;

} RENDER_BUFFER_HEADER;

typedef struct _PARSE_HEADER_CMD_TAG {
    IMG_UINT32      ui32Cmd;
    IMG_UINT32      ui32SeqHdrData;
    IMG_UINT32      ui32PicDimensions;
    IMG_UINT32      ui32BitplaneAddr[3];
    IMG_UINT32      ui32VLCTableAddr;
    IMG_UINT32      ui32ICParamData[2];
} PARSE_HEADER_CMD;

/* Linked list DMA Command */
#define CMD_LLDMA                                       (0xA0000000)
#define CMD_SLLDMA                                      (0xC0000000)            /* Syncronose LLDMA */
#define CMD_DMA                                         (0xE0000000)
typedef struct {
    IMG_UINT32 ui32CmdAndDevLinAddr;
} LLDMA_CMD;

typedef struct
{
    IMG_UINT32 ui32Cmd;
    IMG_UINT32 ui32DevVirtAdd;
} DMA_CMD;

typedef struct
{
    IMG_UINT32 ui32Cmd;
    IMG_UINT32 ui32DevVirtAdd;
    IMG_UINT32 ui32ByteOffset;
} DMA_CMD_WITH_OFFSET;

typedef enum DMA_TYPE
{
    DMA_TYPE_VLC_TABLE                      = CMD_DMA_TYPE_VLC_TABLE,
    DMA_TYPE_RESIDUAL                       = CMD_DMA_TYPE_RESIDUAL,
    DMA_TYPE_IDCT_INSERTION         = CMD_DMA_TYPE_IDCT_INSERTION,
    DMA_TYPE_PROBABILITY_DATA       = CMD_DMA_TYPE_PROBABILITY_DATA,
} DMA_TYPE;

/* Shift Register Setup Command */
#define CMD_SR_SETUP                            (0xB0000000)
#define CMD_ENABLE_RBDU_EXTRACTION              (0x00000001)
#define CMD_SR_VERIFY_STARTCODE                 (0x00000004)
#ifdef SLICE_HEADER_PARSING
#define CMD_SR_BITSTR_PARSE_KEY                 (0x00000010)
#endif
#define CMD_BITSTREAM_DMA                       (0xA0000000)
typedef struct {
    IMG_UINT32 ui32Cmd;
    IMG_UINT32 ui32BitstreamOffsetBits;
    IMG_UINT32 ui32BitstreamSizeBytes;
} SR_SETUP_CMD;

/* Next Segment Command */
#define CMD_NEXT_SEG                            (0xD0000000)    /* Also Syncronose */

/*****************************************************************/
/* Ctrl Alloc Header */
/*****************************************************************/
#define CMD_CTRL_ALLOC_HEADER                                   (0x90000000)
typedef struct _CTRL_ALLOC_HEADER_TAG
{
        IMG_UINT32 ui32Cmd_AdditionalParams;
        IMG_UINT32 ui32SliceParams;
        IMG_UINT32 ui32ExternStateBuffAddr;
        IMG_UINT32 ui32MacroblockParamAddr;
        IMG_UINT32 uiSliceFirstMbYX_uiPicLastMbYX;
        IMG_UINT32 ui32AltOutputAddr[2]; /* VC1 only : Store Range Map flags in bottom bits of [0] */
        IMG_UINT32 ui32AltOutputFlags;
} CTRL_ALLOC_HEADER;

#define CMD_DEBLOCK                                                             (0x70000000)
#define CMD_DEBLOCK_TYPE_STD                                    (0x00000000)
#define CMD_DEBLOCK_TYPE_OOLD                                   (0x00000001)
#define CMD_DEBLOCK_TYPE_SKIP                                   (0x00000002)
typedef struct _DEBLOCK_CMD_TAG
{
        IMG_UINT32 ui32Cmd;                                     /* 0x70000000 */
        IMG_UINT32 ui32SourceMbData;
        IMG_UINT32 ui16PicWidthMbs;
        IMG_UINT32 ui16FrameHeightMbs;
        IMG_UINT32 ui32AddressA[2];
} DEBLOCK_CMD;

#endif
