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
 *    Elaine Wang <elaine.wang@intel.com>
 *
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "psb_def.h"
#include "psb_drv_debug.h"
#include "psb_surface.h"
#include "psb_cmdbuf.h"
#include "tng_hostcode.h"
#include "tng_hostheader.h"
#include "tng_jpegES.h"
#ifdef _TOPAZHP_PDUMP_
#include "tng_trace.h"
#endif

static void tng__trace_cmdbuf(tng_cmdbuf_p cmdbuf)
{
    int i;
    IMG_UINT32 ui32CmdTmp[4];
    IMG_UINT32 *ptmp = (IMG_UINT32 *)(cmdbuf->cmd_start);

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: start\n", __FUNCTION__);

    //skip the newcodec
    if (*ptmp != MTX_CMDID_SW_NEW_CODEC) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: error new coded\n", __FUNCTION__);
        return ;
    }
    ptmp += 6;

    if ((*ptmp & 0xf) != MTX_CMDID_SETUP_INTERFACE) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: error setup interface\n", __FUNCTION__);
        return ;
    }

    ui32CmdTmp[0] = *ptmp++;
    ui32CmdTmp[1] = *ptmp++;
    ui32CmdTmp[2] = *ptmp++;
    ui32CmdTmp[3] = 0;
#ifdef _TOPAZHP_PDUMP_
    topazhp_dump_command((unsigned int*)ui32CmdTmp);
#endif
    for (i = 0; i < 3; i++) {
        ui32CmdTmp[0] = *ptmp++;
        ui32CmdTmp[1] = *ptmp++;
        ui32CmdTmp[2] = 0;
        ui32CmdTmp[3] = 0;
#ifdef _TOPAZHP_PDUMP_
        topazhp_dump_command((unsigned int*)ui32CmdTmp);
#endif
    }

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: end\n", __FUNCTION__);

    return;
}

#define SURFACE(id)    ((object_surface_p) object_heap_lookup( &ctx->obj_context->driver_data->surface_heap, id ))
#define BUFFER(id)  ((object_buffer_p) object_heap_lookup( &ctx->obj_context->driver_data->buffer_heap, id ))

#define PTG_JPEG_MAX_MCU_PER_SCAN (0x4000)
#define PTG_JPEG_HEADER_MAX_SIZE (1024)


#define C_INTERLEAVE 1
#define LC_YUYVINTERLEAVE 2
#define LC_YVYUINTERLEAVE 3
#define LC_UYVYINTERLEAVE 4
#define LC_VYUYINTERLEAVE 5

#define ISCHROMAINTERLEAVED(eSurfaceFormat) ((IMG_UINT)(eSurfaceFormat==IMG_CODEC_PL12) * C_INTERLEAVE)


/******************************************************************************
General definitions
******************************************************************************/
#define BYTE                            8
#define BYTES_IN_INT                    4
#define BITS_IN_INT                     32
#define BLOCK_SIZE                      8
#define PELS_IN_BLOCK                   64

/******************************************************************************
JPEG marker definitions
******************************************************************************/
#define START_OF_IMAGE              0xFFD8
#define SOF_BASELINE_DCT            0xFFC0
#define END_OF_IMAGE                0xFFD9
#define START_OF_SCAN               0xFFDA

/* Definitions for the huffman table specification in the Marker segment */
#define DHT_MARKER                  0xFFC4
#define LH_DC                       0x001F
#define LH_AC                       0x00B5
#define LEVEL_SHIFT                 128

/* Definitions for the quantization table specification in the Marker segment */
#define DQT_MARKER                  0xFFDB
#define ACMAX                       0x03FF
#define DCMAX                       0x07FF
/* Length and precision of the quantization table parameters */
#define LQPQ                        0x00430
#define QMAX                        255
#define CLIP(Number,Max,Min)    if((Number) > (Max)) (Number) = (Max); \
                                else if((Number) < (Min)) (Number) = (Min)

/////////////////////////////////////////////////////////////////////////////////////
// BMP Reading Header Stuff
/////////////////////////////////////////////////////////////////////////////////////

static const IMG_UINT8 gQuantLuma[QUANT_TABLE_SIZE_BYTES] = {
    16, 11, 10, 16, 24, 40, 51, 61,
    12, 12, 14, 19, 26, 58, 60, 55,
    14, 13, 16, 24, 40, 57, 69, 56,
    14, 17, 22, 29, 51, 87, 80, 62,
    18, 22, 37, 56, 68, 109, 103, 77,
    24, 35, 55, 64, 81, 104, 113, 92,
    49, 64, 78, 87, 103, 121, 120, 101,
    72, 92, 95, 98, 112, 100, 103, 99
};

/*****************************************************************************/
/*  \brief   gQuantChroma                                                    */
/*                                                                           */
/*  Contains the data that needs to be sent in the marker segment of an      */
/*  interchange format JPEG stream or an abbreviated format table            */
/*  specification data stream.                                               */
/*  Quantizer table for the chrominance component                            */
/*****************************************************************************/
static const IMG_UINT8 gQuantChroma[QUANT_TABLE_SIZE_BYTES] = {
    17, 18, 24, 47, 99, 99, 99, 99,
    18, 21, 26, 66, 99, 99, 99, 99,
    24, 26, 56, 99, 99, 99, 99, 99,
    47, 66, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99
};

/*****************************************************************************/
/*  \brief   gZigZag                                                         */
/*                                                                           */
/*  Zigzag scan pattern                                                      */
/*****************************************************************************/
static const IMG_UINT8 gZigZag[] = {
    0,  1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};

/*****************************************************************************/
/*  \brief   gMarkerDataLumaDc                                               */
/*                                                                           */
/*  Contains the data that needs to be sent in the marker segment of an      */
/*  interchange format JPEG stream or an abbreviated format table            */
/*  specification data stream.                                               */
/*  Specifies the huffman table used for encoding the luminance DC           */
/*  coefficient differences. The table represents Table K.3 of               */
/*  IS0/IEC 10918-1:1994(E)                                                  */
/*****************************************************************************/
static const IMG_UINT8 gMarkerDataLumaDc[] = {
    //TcTh  Li
    0x00, 0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    // Vi
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B
};

/*****************************************************************************/
/*  \brief   gMarkerDataLumaAc                                               */
/*                                                                           */
/*  Contains the data that needs to be sent in the marker segment of an      */
/*  interchange format JPEG stream or an abbreviated format table            */
/*  specification data stream.                                               */
/*  Specifies the huffman table used for encoding the luminance AC           */
/*  coefficients. The table represents Table K.5 of IS0/IEC 10918-1:1994(E)  */
/*****************************************************************************/
static const IMG_UINT8 gMarkerDataLumaAc[] = {
    // TcTh  Li
    0x10, 0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03, 0x05, 0x05, 0x04,
    0x04, 0x00, 0x00, 0x01, 0x7D,
    // Vi
    0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06,
    0x13, 0x51, 0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08,
    0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52, 0xD1, 0xF0, 0x24, 0x33, 0x62, 0x72,
    0x82, 0x09, 0x0A, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45,
    0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
    0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75,
    0x76, 0x77, 0x78, 0x79, 0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
    0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3,
    0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6,
    0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9,
    0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2,
    0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xF1, 0xF2, 0xF3, 0xF4,
    0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA
};

/*****************************************************************************/
/*  \brief   gMarkerDataChromaDc                                             */
/*                                                                           */
/*  Contains the data that needs to be sent in the marker segment of an      */
/*  interchange format JPEG stream or an abbreviated format table            */
/*  specification data stream.                                               */
/*  Specifies the huffman table used for encoding the chrominance DC         */
/*  coefficient differences. The table represents Table K.4 of               */
/*  IS0/IEC 10918-1:1994(E)                                                  */
/*****************************************************************************/
static const IMG_UINT8 gMarkerDataChromaDc[] = {
    // TcTh Li
    0x01, 0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00,

    // Vi
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B
};

/*****************************************************************************/
/*  \brief   gMarkerDataChromaAc                                             */
/*                                                                           */
/*  Contains the data that needs to be sent in the marker segment of an      */
/*  interchange format JPEG stream or an abbreviated format table            */
/*  specification data stream.                                               */
/*  Specifies the huffman table used for encoding the chrominance AC         */
/*  coefficients. The table represents Table K.6 of IS0/IEC 10918-1:1994(E)  */
/*****************************************************************************/
static const IMG_UINT8 gMarkerDataChromaAc[] = {
    // TcTh
    0x11, 0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04, 0x07, 0x05, 0x04,
    0x04, 0x00, 0x01, 0x02, 0x77,

    // Vi
    0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41,
    0x51, 0x07, 0x61, 0x71, 0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
    0xA1, 0xB1, 0xC1, 0x09, 0x23, 0x33, 0x52, 0xF0, 0x15, 0x62, 0x72, 0xD1,
    0x0A, 0x16, 0x24, 0x34, 0xE1, 0x25, 0xF1, 0x17, 0x18, 0x19, 0x1A, 0x26,
    0x27, 0x28, 0x29, 0x2A, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44,
    0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
    0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A,
    0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4,
    0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
    0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA,
    0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xF2, 0xF3, 0xF4,
    0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA
};

static int CustomizeQuantizationTables(unsigned char *luma_matrix,
                                         unsigned char *chroma_matrix,
                                         unsigned int ui32Quality)
{
    unsigned int uc_qVal;
    unsigned int uc_j;

    if((NULL == luma_matrix) || (NULL == chroma_matrix) || 
       (ui32Quality < 1) || (ui32Quality > 100))
        return 1;

    /* Compute luma quantization table */
    ui32Quality = (ui32Quality<50) ? (5000/ui32Quality) : (200-ui32Quality*2);
    for(uc_j=0; uc_j<QUANT_TABLE_SIZE_BYTES; ++uc_j) {
        uc_qVal = (gQuantLuma[uc_j] * ui32Quality + 50) / 100;
        uc_qVal =  (uc_qVal>0xFF)? 0xFF:uc_qVal;
        uc_qVal =  (uc_qVal<1)? 1:uc_qVal;
        luma_matrix[uc_j] = (unsigned char)uc_qVal;
    }

    /* Compute chroma quantization table */
    for(uc_j=0; uc_j<QUANT_TABLE_SIZE_BYTES; ++uc_j) {
        uc_qVal = (gQuantChroma[uc_j] * ui32Quality + 50) / 100;
        uc_qVal =  (uc_qVal>0xFF)? 0xFF:uc_qVal;
        uc_qVal =  (uc_qVal<1)? 1:uc_qVal;
        chroma_matrix[uc_j] = (unsigned char)uc_qVal;
    }

    return 0;
}

static void SetDefaultQmatix(void *pMemInfoTableBlock)
{
    JPEG_MTX_QUANT_TABLE *pQTable =  pMemInfoTableBlock;
    memcpy(pQTable->aui8LumaQuantParams, gQuantLuma, QUANT_TABLE_SIZE_BYTES);
    memcpy(pQTable->aui8ChromaQuantParams, gQuantChroma, QUANT_TABLE_SIZE_BYTES);
    return;
}


static void IssueQmatix(TOPAZHP_JPEG_ENCODER_CONTEXT *pJPEGContext)
{
    int i;
    context_ENC_p ctx = (context_ENC_p)pJPEGContext->ctx;

    /* Dump MTX setup data for debug */
    ASSERT(NULL != pJPEGContext->pMemInfoTableBlock);

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Issue Quantization Table data\n");
    for (i=0; i<128; i+=8) {
        if (0 == i) {
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "Table 0:\n");
        }
        else if (64 == i) {
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "Table 1:\n");
        }
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "%d %d %d %d %d %d %d %d\n", 
                      *((unsigned char *)ctx->obj_context->tng_cmdbuf->jpeg_pic_params_p+i),
                      *((unsigned char *)ctx->obj_context->tng_cmdbuf->jpeg_pic_params_p+i+1),
                      *((unsigned char *)ctx->obj_context->tng_cmdbuf->jpeg_pic_params_p+i+2),
                      *((unsigned char *)ctx->obj_context->tng_cmdbuf->jpeg_pic_params_p+i+3),
                      *((unsigned char *)ctx->obj_context->tng_cmdbuf->jpeg_pic_params_p+i+4),
                      *((unsigned char *)ctx->obj_context->tng_cmdbuf->jpeg_pic_params_p+i+5),
                      *((unsigned char *)ctx->obj_context->tng_cmdbuf->jpeg_pic_params_p+i+6),
                      *((unsigned char *)ctx->obj_context->tng_cmdbuf->jpeg_pic_params_p+i+7));
    }

    tng_cmdbuf_insert_command(ctx->obj_context,
                                      0,
                                      MTX_CMDID_SETQUANT,
                                      0,
                                      &(ctx->obj_context->tng_cmdbuf->jpeg_pic_params),
                                      0);
}

static void InitializeJpegEncode(TOPAZHP_JPEG_ENCODER_CONTEXT *pJPEGContext)
{
    context_ENC_p ctx = (context_ENC_p)pJPEGContext->ctx;
    IMG_UINT16 ui16_width;
    IMG_UINT16 ui16_height;
    IMG_UINT32 ui32UpperLimit;

    /*********************************************************************/
    /* Determine the horizonal and the vertical sampling frequency of    */
    /* each of components in the image                                   */
    /*********************************************************************/

    ui16_width = ctx->ui16Width;
    ui16_height = ctx->ui16FrameHeight; //pTFrame->height isn't the real height of image, because vaCreateSurface has made it aligned with 32

    switch (pJPEGContext->eFormat) {
    case IMG_CODEC_PL12:
    default:
        pJPEGContext->MCUComponent[0].ui32WidthBlocks = 16;
        pJPEGContext->MCUComponent[0].ui32HeightBlocks = 16;
        pJPEGContext->MCUComponent[0].ui32XLimit = ui16_width;
        pJPEGContext->MCUComponent[0].ui32YLimit = ui16_height;

        pJPEGContext->MCUComponent[1].ui32WidthBlocks = 8;
        pJPEGContext->MCUComponent[1].ui32HeightBlocks = 8;
        pJPEGContext->MCUComponent[1].ui32XLimit = ui16_width >> 1;
        pJPEGContext->MCUComponent[1].ui32YLimit = ui16_height >> 1;

        pJPEGContext->MCUComponent[2].ui32WidthBlocks = 8;
        pJPEGContext->MCUComponent[2].ui32HeightBlocks = 8;
        pJPEGContext->MCUComponent[2].ui32XLimit = ui16_width >> 1;
        pJPEGContext->MCUComponent[2].ui32YLimit = ui16_height >> 1;

        break;
    }

    switch (ISCHROMAINTERLEAVED(pJPEGContext->eFormat)) {
    case C_INTERLEAVE:
    default:
        // Chroma format is byte interleaved, as the engine runs using planar colour surfaces we need
        // to fool the engine into offsetting by 16 instead of 8
        pJPEGContext->MCUComponent[1].ui32WidthBlocks +=
            pJPEGContext->MCUComponent[2].ui32WidthBlocks;
        pJPEGContext->MCUComponent[1].ui32XLimit +=
            pJPEGContext->MCUComponent[2].ui32XLimit;
        pJPEGContext->MCUComponent[2].ui32XLimit =
            pJPEGContext->MCUComponent[2].ui32YLimit =
                pJPEGContext->MCUComponent[2].ui32WidthBlocks =
                    pJPEGContext->MCUComponent[2].ui32HeightBlocks = 0;
        break;
    }

    pJPEGContext->sScan_Encode_Info.ui32NumberMCUsX =
        (pJPEGContext->MCUComponent[0].ui32XLimit +
         (pJPEGContext->MCUComponent[0].ui32WidthBlocks - 1)) /
        pJPEGContext->MCUComponent[0].ui32WidthBlocks;
    pJPEGContext->sScan_Encode_Info.ui32NumberMCUsY =
        (pJPEGContext->MCUComponent[0].ui32YLimit +
         (pJPEGContext->MCUComponent[0].ui32HeightBlocks - 1)) /
        pJPEGContext->MCUComponent[0].ui32HeightBlocks;
    pJPEGContext->sScan_Encode_Info.ui32NumberMCUsToEncode =
        pJPEGContext->sScan_Encode_Info.ui32NumberMCUsX *
        pJPEGContext->sScan_Encode_Info.ui32NumberMCUsY;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Number of X MCUs: %d\n", pJPEGContext->sScan_Encode_Info.ui32NumberMCUsX);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Number of Y MCUs: %d\n", pJPEGContext->sScan_Encode_Info.ui32NumberMCUsY);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Number of total MCUs: %d\n", pJPEGContext->sScan_Encode_Info.ui32NumberMCUsToEncode);


    // Number of MCUs sent for a scan _must_ lie at the beginning of a line so that the chroma component can't violate the 16 byte DMA start alignment constraint
    // (Actual memory alignment for final DMA will have width aligned to 64, so start of line will automatically meet the 16 byte alignment required by the DMA engine)
    pJPEGContext->sScan_Encode_Info.ui32NumberMCUsToEncodePerScan =
        (pJPEGContext->sScan_Encode_Info.ui32NumberMCUsY + (pJPEGContext->NumCores - 1)) / pJPEGContext->NumCores;
    pJPEGContext->sScan_Encode_Info.ui32NumberMCUsToEncodePerScan *=
        pJPEGContext->sScan_Encode_Info.ui32NumberMCUsX;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Number of MCUs per core: %d\n", pJPEGContext->sScan_Encode_Info.ui32NumberMCUsToEncodePerScan);


    // Limit the scan size to maximum useable (due to it being used as the 16 bit field for Restart Intervals) = 0xFFFF MCUs
    // In reality, worst case allocatable bytes is less than this, something around 0x159739C == 0x4b96 MCUs = 139 x 139 MCUS = 2224 * 2224 pixels, approx.
    // We'll give this upper limit some margin for error, and limit our MCUsPerScan to 2000 * 2000 pixels = 125 * 125 MCUS = 0x3D09 MCUS = 0x116F322 bytes (1170 worst case per MCU)
    // If more MCUs are required, then the image will be automatically encoded with multiple scans on the same pipes
    ui32UpperLimit = PTG_JPEG_MAX_MCU_PER_SCAN / pJPEGContext->sScan_Encode_Info.ui32NumberMCUsX;
    ui32UpperLimit *= pJPEGContext->sScan_Encode_Info.ui32NumberMCUsX;

    if (pJPEGContext->sScan_Encode_Info.ui32NumberMCUsToEncodePerScan > ui32UpperLimit) {
        // Set MCUs to encode per scan to equal maximum limit and then truncate to ensure it lies at the first MCU of a line (to satisfy the 64 byte requirement)
        pJPEGContext->sScan_Encode_Info.ui32NumberMCUsToEncodePerScan = ui32UpperLimit;
    }

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Number of MCUs per scan: %d\n", pJPEGContext->sScan_Encode_Info.ui32NumberMCUsToEncodePerScan);

    //Need to set up CB Output slicenumber to equal number of slices required to encode image
    // Set current CB scan to maximum scan number (will count down as scans are output)
    pJPEGContext->sScan_Encode_Info.ui16ScansInImage =
        (pJPEGContext->sScan_Encode_Info.ui32NumberMCUsToEncode +
         (pJPEGContext->sScan_Encode_Info.ui32NumberMCUsToEncodePerScan - 1)) /
        pJPEGContext->sScan_Encode_Info.ui32NumberMCUsToEncodePerScan;

    pJPEGContext->sScan_Encode_Info.ui8NumberOfCodedBuffers =
        pJPEGContext->sScan_Encode_Info.ui16ScansInImage;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Scans in image: %d\n", pJPEGContext->sScan_Encode_Info.ui16ScansInImage);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Number of coded buffers: %d\n", pJPEGContext->sScan_Encode_Info.ui8NumberOfCodedBuffers);

    return;
}

static void AssignCodedDataBuffers(TOPAZHP_JPEG_ENCODER_CONTEXT *pJPEGContext)
{
    IMG_UINT8 ui8Loop;
    pJPEGContext->ui32SizePerCodedBuffer =
        (pJPEGContext->jpeg_coded_buf.ui32Size - PTG_JPEG_HEADER_MAX_SIZE) /
        pJPEGContext->sScan_Encode_Info.ui8NumberOfCodedBuffers;
    pJPEGContext->ui32SizePerCodedBuffer &= ~0xf;    

    memset((void *)pJPEGContext->sScan_Encode_Info.aBufferTable, 0x0,
           sizeof(TOPAZHP_JPEG_BUFFER_INFO)*pJPEGContext->sScan_Encode_Info.ui8NumberOfCodedBuffers);

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "jpeg_coded_buf.pMemInfo: 0x%x\n", (unsigned int)(pJPEGContext->jpeg_coded_buf.pMemInfo));

    for (ui8Loop = 0 ; ui8Loop < pJPEGContext->sScan_Encode_Info.ui8NumberOfCodedBuffers; ui8Loop++) {
        //pJPEGContext->sScan_Encode_Info.aBufferTable[ui8Loop].ui32DataBufferSizeBytes = DATA_BUFFER_SIZE(pJPEGContext->sScan_Encode_Info.ui32NumberMCUsToEncodePerScan);
        //pJPEGContext->sScan_Encode_Info.aBufferTable[ui8Loop].ui32DataBufferSizeBytes = (pJPEGContext->sScan_Encode_Info.aBufferTable[ui8Loop].ui32DataBufferSizeBytes+sizeof(BUFFER_HEADER)) + 3 & ~3;
        pJPEGContext->sScan_Encode_Info.aBufferTable[ui8Loop].ui32DataBufferUsedBytes = 0;
        pJPEGContext->sScan_Encode_Info.aBufferTable[ui8Loop].i8PipeNumber = 0; // Indicates buffer is idle
        pJPEGContext->sScan_Encode_Info.aBufferTable[ui8Loop].ui16ScanNumber = 0; // Indicates buffer is idle

        pJPEGContext->sScan_Encode_Info.aBufferTable[ui8Loop].pMemInfo = (void *)
                ((IMG_UINT32)pJPEGContext->jpeg_coded_buf.pMemInfo + PTG_JPEG_HEADER_MAX_SIZE +
                 ui8Loop * pJPEGContext->ui32SizePerCodedBuffer);
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "aBufferTable[%d].pMemInfo: 0x%x\n", ui8Loop,
                                 (unsigned int)(pJPEGContext->sScan_Encode_Info.aBufferTable[ui8Loop].pMemInfo));
    }

    return;
}

static void SetSetupInterface(TOPAZHP_JPEG_ENCODER_CONTEXT *pJPEGContext)
{
    context_ENC_p ctx = (context_ENC_p)pJPEGContext->ctx;
    context_ENC_mem *ps_mem = &(ctx->ctx_mem[ctx->ui32StreamID]);
    context_ENC_mem_size *ps_mem_size = &(ctx->ctx_mem_size);

    tng_cmdbuf_set_phys(pJPEGContext->pMTXWritebackMemory->apWritebackRegions, WB_FIFO_SIZE,
                        &(ctx->bufs_writeback), 0, ps_mem_size->writeback);
}

static void IssueSetupInterface(TOPAZHP_JPEG_ENCODER_CONTEXT *pJPEGContext)
{
    int i;
    context_ENC_p ctx = (context_ENC_p)pJPEGContext->ctx;

    ASSERT(NULL != pJPEGContext->pMTXSetup);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Issue SetupInterface\n");

    for (i = 0; i < WB_FIFO_SIZE; i++) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "apWritebackRegions[%d]: 0x%x\n", i,
                                 pJPEGContext->pMTXWritebackMemory->apWritebackRegions[i]);
    }

    tng_cmdbuf_insert_command(ctx->obj_context,
                                      0,
                                      MTX_CMDID_SETUP_INTERFACE,
                                      0,
                                      &(ctx->obj_context->tng_cmdbuf->jpeg_header_interface_mem),
                                      0);
}

static IMG_ERRORCODE SetMTXSetup(
    TOPAZHP_JPEG_ENCODER_CONTEXT *pJPEGContext,
    object_surface_p pTFrame)
{
    IMG_UINT32 srf_buf_offset;
    context_ENC_p ctx = (context_ENC_p)pJPEGContext->ctx;
    tng_cmdbuf_p cmdbuf = ctx->obj_context->tng_cmdbuf;
    context_ENC_mem *ps_mem = &(ctx->ctx_mem[ctx->ui32StreamID]);
    context_ENC_mem_size *ps_mem_size = &(ctx->ctx_mem_size);

    pJPEGContext->pMTXSetup->ui32ComponentsInScan = MTX_MAX_COMPONENTS;

    switch (pJPEGContext->eFormat) {
    case IMG_CODEC_PL12:
        if (pTFrame->psb_surface->stride % 64) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "Surface stride isn't aligned to 64 bytes as HW requires: %u!\n",
                          pTFrame->psb_surface->stride);
            return IMG_ERR_INVALID_CONTEXT;
        }
        pJPEGContext->pMTXSetup->ComponentPlane[0].ui32Stride = pTFrame->psb_surface->stride;
        pJPEGContext->pMTXSetup->ComponentPlane[1].ui32Stride = pTFrame->psb_surface->stride;
        pJPEGContext->pMTXSetup->ComponentPlane[2].ui32Stride = pTFrame->psb_surface->stride;

        pJPEGContext->pMTXSetup->ComponentPlane[0].ui32Height = pJPEGContext->MCUComponent[0].ui32YLimit;
        pJPEGContext->pMTXSetup->ComponentPlane[1].ui32Height = pJPEGContext->MCUComponent[0].ui32YLimit / 2;
        pJPEGContext->pMTXSetup->ComponentPlane[2].ui32Height = pJPEGContext->MCUComponent[0].ui32YLimit / 2;
        break;
    default:
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Not supported FOURCC: %x!\n", pJPEGContext->eFormat);
        return IMG_ERR_INVALID_CONTEXT;
    }

    srf_buf_offset = pTFrame->psb_surface->buf.buffer_ofs;
    RELOC_JPEG_PIC_PARAMS_PTG(&pJPEGContext->pMTXSetup->ComponentPlane[0].ui32PhysAddr, srf_buf_offset, 
                              &pTFrame->psb_surface->buf);
    switch (pJPEGContext->eFormat) {
    case IMG_CODEC_PL12:
        RELOC_JPEG_PIC_PARAMS_PTG(&pJPEGContext->pMTXSetup->ComponentPlane[1].ui32PhysAddr,
                                  srf_buf_offset + pTFrame->psb_surface->stride * pTFrame->height,
                                  &pTFrame->psb_surface->buf);
        //Byte interleaved surface, so need to force chroma to use single surface by fooling it into
        //thinking it's dealing with standard 8x8 planaerblocks
        RELOC_JPEG_PIC_PARAMS_PTG(&pJPEGContext->pMTXSetup->ComponentPlane[2].ui32PhysAddr,
                                  srf_buf_offset + pTFrame->psb_surface->stride * pTFrame->height + 8,
                                  &pTFrame->psb_surface->buf);
        break;
    default:
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Not supported FOURCC: %x!\n", pJPEGContext->eFormat);
        return IMG_ERR_INVALID_CONTEXT;
    }

    memcpy((void *)pJPEGContext->pMTXSetup->MCUComponent,
           (void *)pJPEGContext->MCUComponent,
           sizeof(pJPEGContext->MCUComponent));

    pJPEGContext->pMTXSetup->ui32TableA = 0;
    pJPEGContext->pMTXSetup->ui16DataInterleaveStatus = ISCHROMAINTERLEAVED(pJPEGContext->eFormat);
    pJPEGContext->pMTXSetup->ui16MaxPipes = (IMG_UINT16)pJPEGContext->NumCores;

    return IMG_ERR_OK;
}

static void IssueMTXSetup(TOPAZHP_JPEG_ENCODER_CONTEXT *pJPEGContext)
{
    uint32_t i;
    context_ENC_p ctx = (context_ENC_p)pJPEGContext->ctx;

    /* Dump MTX setup data for debug */
    ASSERT(NULL != pJPEGContext->pMTXSetup);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Issue MTX setup data\n");

    for (i = 0; i < pJPEGContext->pMTXSetup->ui32ComponentsInScan; i++) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "ComponentPlane[%d]: 0x%x, %d, %d\n", i,
                                 pJPEGContext->pMTXSetup->ComponentPlane[i].ui32PhysAddr,
                                 pJPEGContext->pMTXSetup->ComponentPlane[i].ui32Stride,
                                 pJPEGContext->pMTXSetup->ComponentPlane[i].ui32Height);
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "MCUComponent[%d]: %d, %d, %d, %d\n", i,
                                 pJPEGContext->pMTXSetup->MCUComponent[i].ui32WidthBlocks,
                                 pJPEGContext->pMTXSetup->MCUComponent[i].ui32HeightBlocks,
                                 pJPEGContext->pMTXSetup->MCUComponent[i].ui32XLimit,
                                 pJPEGContext->pMTXSetup->MCUComponent[i].ui32YLimit);
    }

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "ui32ComponentsInScan: %d\n", pJPEGContext->pMTXSetup->ui32ComponentsInScan);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "ui32TableA: %d\n", pJPEGContext->pMTXSetup->ui32TableA);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "ui16DataInterleaveStatus: %d\n", pJPEGContext->pMTXSetup->ui16DataInterleaveStatus);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "ui16MaxPipes: %d\n", pJPEGContext->pMTXSetup->ui16MaxPipes);

    tng_cmdbuf_insert_command(ctx->obj_context,
                                      0,
                                      MTX_CMDID_SETUP,
                                      0,
                                      &(ctx->obj_context->tng_cmdbuf->jpeg_header_mem),
                                      0);

    return;
}



static void fPutBitsToBuffer(STREAMTYPEW *BitStream, IMG_UINT8 NoOfBytes, IMG_UINT32 ActualBits)
{
    IMG_UINT8 ui8Lp;
    IMG_UINT8 *pui8S;

    pui8S = (IMG_UINT8 *)BitStream->Buffer;
    pui8S += BitStream->Offset;

    for (ui8Lp = NoOfBytes; ui8Lp > 0; ui8Lp--)
        *(pui8S++) = ((IMG_UINT8 *) &ActualBits)[ui8Lp-1];

    BitStream->Offset += NoOfBytes;
}

static IMG_UINT32 EncodeMarkerSegment(TOPAZHP_JPEG_ENCODER_CONTEXT *pJPEGContext,
                                      IMG_UINT8 *puc_stream_buff, IMG_BOOL bIncludeHuffmanTables)
{
    STREAMTYPEW s_streamW;
    IMG_UINT8 uc_i;

    s_streamW.Offset = 0;
    s_streamW.Buffer = puc_stream_buff;

    /* Writing the start of image marker */
    fPutBitsToBuffer(&s_streamW, 2, START_OF_IMAGE);

    /* Writing the quantization table for luminance into the stream */
    fPutBitsToBuffer(&s_streamW, 2, DQT_MARKER);

    fPutBitsToBuffer(&s_streamW, 3, LQPQ << 4); // 20 bits = LQPQ, 4 bits = 0 (Destination identifier for the luminance quantizer tables)

    IMG_ASSERT(PELS_IN_BLOCK <= QUANT_TABLE_SIZE_BYTES);
    for (uc_i = 0; uc_i < PELS_IN_BLOCK; uc_i++) {
        // Write zigzag ordered luma quantization values to our JPEG header
        fPutBitsToBuffer(&s_streamW, 1, pJPEGContext->psTablesBlock->aui8LumaQuantParams[gZigZag[uc_i]]);
    }

    /* Writing the quantization table for chrominance into the stream */
    fPutBitsToBuffer(&s_streamW, 2, DQT_MARKER);

    fPutBitsToBuffer(&s_streamW, 3, (LQPQ << 4) | 1); // 20 bits = LQPQ, 4 bits = 1 (Destination identifier for the chrominance quantizer tables)

    for (uc_i = 0; uc_i < PELS_IN_BLOCK; uc_i++) {
        // Write zigzag ordered chroma quantization values to our JPEG header
        fPutBitsToBuffer(&s_streamW, 1, pJPEGContext->psTablesBlock->aui8ChromaQuantParams[gZigZag[uc_i]]);
    }




    if (bIncludeHuffmanTables) {
        /* Writing the huffman tables for luminance dc coeffs */
        /* Write the DHT Marker */
        fPutBitsToBuffer(&s_streamW, 2, DHT_MARKER);
        fPutBitsToBuffer(&s_streamW, 2, LH_DC);
        for (uc_i = 0; uc_i < LH_DC - 2; uc_i++) {
            fPutBitsToBuffer(&s_streamW, 1, gMarkerDataLumaDc[uc_i]);
        }
        /* Writing the huffman tables for luminance ac coeffs */
        /* Write the DHT Marker */
        fPutBitsToBuffer(&s_streamW, 2, DHT_MARKER);
        fPutBitsToBuffer(&s_streamW, 2, LH_AC);
        for (uc_i = 0; uc_i < LH_AC - 2; uc_i++) {
            fPutBitsToBuffer(&s_streamW, 1, gMarkerDataLumaAc[uc_i]);
        }
        /* Writing the huffman tables for chrominance dc coeffs */
        fPutBitsToBuffer(&s_streamW, 2, DHT_MARKER);
        fPutBitsToBuffer(&s_streamW, 2, LH_DC);
        for (uc_i = 0; uc_i < LH_DC - 2; uc_i++) {
            fPutBitsToBuffer(&s_streamW, 1, gMarkerDataChromaDc[uc_i]);
        }
        /* Writing the huffman tables for luminance ac coeffs */
        /* Write the DHT Marker */
        fPutBitsToBuffer(&s_streamW, 2, DHT_MARKER);
        fPutBitsToBuffer(&s_streamW, 2, LH_AC);
        for (uc_i = 0; uc_i < LH_AC - 2; uc_i++) {
            fPutBitsToBuffer(&s_streamW, 1, gMarkerDataChromaAc[uc_i]);
        }
    }

    // Activate Restart markers
    if (pJPEGContext->sScan_Encode_Info.ui16CScan > 1) {
        // Only use restart intervals if we need them (ie. multiple Scan encode and/or parallel CB encode)
        fPutBitsToBuffer(&s_streamW, 2, 0xFFDD); //Marker header
        fPutBitsToBuffer(&s_streamW, 2, 4); // Byte size of marker (header not included)
        fPutBitsToBuffer(&s_streamW, 2, pJPEGContext->sScan_Encode_Info.ui32NumberMCUsToEncodePerScan); // Restart Interval (same as MCUs per buffer)
    }

    return s_streamW.Offset;
}

static IMG_UINT32 EncodeFrameHeader(TOPAZHP_JPEG_ENCODER_CONTEXT *pJPEGContext,
                                    IMG_UINT8 *puc_stream_buff)
{
    STREAMTYPEW ps_streamW;
    IMG_UINT8  uc_num_comp_in_img;

    uc_num_comp_in_img = pJPEGContext->pMTXSetup->ui32ComponentsInScan;

    ps_streamW.Offset = 0;
    ps_streamW.Buffer = puc_stream_buff;


    //if(ps_jpeg_params->uc_isAbbreviated != 0)
    //   fPutBitsToBuffer(&ps_streamW, 2, START_OF_IMAGE);

    /* Writing the frame header */
    fPutBitsToBuffer(&ps_streamW, 2, SOF_BASELINE_DCT);
    /* Frame header length */
    fPutBitsToBuffer(&ps_streamW, 2, 8 + 3 * uc_num_comp_in_img);
    /* Precision */
    fPutBitsToBuffer(&ps_streamW, 1, 8);
    /* Height : sample lines */
    fPutBitsToBuffer(&ps_streamW, 2, pJPEGContext->ui32OutputHeight);
    /* Width : samples per line */
    fPutBitsToBuffer(&ps_streamW, 2, pJPEGContext->ui32OutputWidth);
    /* Number of image components */
    fPutBitsToBuffer(&ps_streamW, 1, uc_num_comp_in_img);


    //Chroma Details
    if (pJPEGContext->pMTXSetup->ui16DataInterleaveStatus < C_INTERLEAVE) {
        //Luma Details
        /* Component identifier */
        fPutBitsToBuffer(&ps_streamW, 1, 1); //CompId 0 = 1, 1 = 2, 2 = 3
        fPutBitsToBuffer(&ps_streamW, 1, ((pJPEGContext->pMTXSetup->MCUComponent[0].ui32WidthBlocks >> 3) << 4) | (pJPEGContext->pMTXSetup->MCUComponent[0].ui32HeightBlocks >> 3));
        fPutBitsToBuffer(&ps_streamW, 1, 0); // 0 = Luma(0), 1,2 = Chroma(1)

        //Chroma planar
        fPutBitsToBuffer(&ps_streamW, 1, 2); //CompId 0 = 1, 1 = 2, 2 = 3
        /* 4 bit Horizontal and 4 bit vertical sampling factors */
        fPutBitsToBuffer(&ps_streamW, 1, ((pJPEGContext->pMTXSetup->MCUComponent[1].ui32WidthBlocks >> 3) << 4) | (pJPEGContext->pMTXSetup->MCUComponent[1].ui32HeightBlocks >> 3));
        fPutBitsToBuffer(&ps_streamW, 1, 1); // 0 = Luma(0), 1,2 = Chroma(1)
        fPutBitsToBuffer(&ps_streamW, 1, 3); //CompId 0 = 1, 1 = 2, 2 = 3
        /* 4 bit Horizontal and 4 bit vertical sampling factors */
        fPutBitsToBuffer(&ps_streamW, 1, ((pJPEGContext->pMTXSetup->MCUComponent[2].ui32WidthBlocks >> 3) << 4) | (pJPEGContext->pMTXSetup->MCUComponent[2].ui32HeightBlocks >> 3));
        fPutBitsToBuffer(&ps_streamW, 1, 1); // 0 = Luma(0), 1,2 = Chroma(1)
    } else if (pJPEGContext->pMTXSetup->ui16DataInterleaveStatus == C_INTERLEAVE) {
        //Luma Details
        /* Component identifier */
        fPutBitsToBuffer(&ps_streamW, 1, 1); //CompId 0 = 1, 1 = 2, 2 = 3
        fPutBitsToBuffer(&ps_streamW, 1, ((pJPEGContext->pMTXSetup->MCUComponent[0].ui32WidthBlocks >> 3) << 4) | (pJPEGContext->pMTXSetup->MCUComponent[0].ui32HeightBlocks >> 3));
        fPutBitsToBuffer(&ps_streamW, 1, 0); // 0 = Luma(0), 1,2 = Chroma(1)

        // Chroma Interleaved
        fPutBitsToBuffer(&ps_streamW, 1, 2); //CompId 0 = 1, 1 = 2, 2 = 3
        /* 4 bit Horizontal and 4 bit vertical sampling factors */
        fPutBitsToBuffer(&ps_streamW, 1, ((pJPEGContext->pMTXSetup->MCUComponent[1].ui32WidthBlocks >> 4) << 4) | (pJPEGContext->pMTXSetup->MCUComponent[1].ui32HeightBlocks >> 3));
        fPutBitsToBuffer(&ps_streamW, 1, 1); // 0 = Luma(0), 1,2 = Chroma(1)

        fPutBitsToBuffer(&ps_streamW, 1, 3); //CompId 0 = 1, 1 = 2, 2 = 3
        /* 4 bit Horizontal and 4 bit vertical sampling factors */
        fPutBitsToBuffer(&ps_streamW, 1, ((pJPEGContext->pMTXSetup->MCUComponent[1].ui32WidthBlocks >> 4) << 4) | (pJPEGContext->pMTXSetup->MCUComponent[1].ui32HeightBlocks >> 3));
        fPutBitsToBuffer(&ps_streamW, 1, 1); // 0 = Luma(0), 1,2 = Chroma(1)
    } else {
        //Luma Details
        /* Component identifier */
        fPutBitsToBuffer(&ps_streamW, 1, 1); //CompId 0 = 1, 1 = 2, 2 = 3
        fPutBitsToBuffer(&ps_streamW, 1, ((pJPEGContext->pMTXSetup->MCUComponent[0].ui32WidthBlocks >> 4) << 4) | (pJPEGContext->pMTXSetup->MCUComponent[0].ui32HeightBlocks >> 3));
        fPutBitsToBuffer(&ps_streamW, 1, 0); // 0 = Luma(0), 1,2 = Chroma(1)

        //Chroma YUYV - Special case
        fPutBitsToBuffer(&ps_streamW, 1, 2); //CompId 0 = 1, 1 = 2, 2 = 3
        /* 4 bit Horizontal and 4 bit vertical sampling factors */
        fPutBitsToBuffer(&ps_streamW, 1, ((pJPEGContext->pMTXSetup->MCUComponent[0].ui32WidthBlocks >> 5) << 4) | (pJPEGContext->pMTXSetup->MCUComponent[0].ui32HeightBlocks >> 3));
        fPutBitsToBuffer(&ps_streamW, 1, 1); // 0 = Luma(0), 1,2 = Chroma(1)
        fPutBitsToBuffer(&ps_streamW, 1, 3); //CompId 0 = 1, 1 = 2, 2 = 3
        /* 4 bit Horizontal and 4 bit vertical sampling factors */
        fPutBitsToBuffer(&ps_streamW, 1, ((pJPEGContext->pMTXSetup->MCUComponent[0].ui32WidthBlocks >> 5) << 4) | (pJPEGContext->pMTXSetup->MCUComponent[0].ui32HeightBlocks >> 3));
        fPutBitsToBuffer(&ps_streamW, 1, 1); // 0 = Luma(0), 1,2 = Chroma(1)
    }


    //Use if you want start of scan (image data) to align to 32
    //fPutBitsToBuffer(&ps_streamW, 1, 0xFF);

    return ps_streamW.Offset;
}

static IMG_UINT32 JPGEncodeMarker(TOPAZHP_JPEG_ENCODER_CONTEXT *pJPEGContext ,
                                  IMG_UINT8* pui8BitStreamBuffer ,
                                  IMG_UINT32 *pui32BytesWritten, IMG_BOOL bIncludeHuffmanTables)
{
#ifdef JPEG_VERBOSE
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "PVRJPGEncodeMarker");
#endif


    *pui32BytesWritten += EncodeMarkerSegment(pJPEGContext, pui8BitStreamBuffer + *pui32BytesWritten, bIncludeHuffmanTables);

    return 0;
}

static IMG_UINT32 JPGEncodeHeader(TOPAZHP_JPEG_ENCODER_CONTEXT *pJPEGContext,
                                  IMG_UINT8*      pui8BitStreamBuffer ,
                                  IMG_UINT32*     pui32BytesWritten)
{
#ifdef JPEG_VERBOSE
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "JPGEncodeHeader");
#endif

    *pui32BytesWritten += EncodeFrameHeader(pJPEGContext, pui8BitStreamBuffer + *pui32BytesWritten);

    return 0;
}

static IMG_UINT32 JPGEncodeSOSHeader(TOPAZHP_JPEG_ENCODER_CONTEXT *pJPEGContext,
                                     IMG_UINT8*      pui8BitStreamBuffer ,
                                     IMG_UINT32*     pui32BytesWritten)
{
    IMG_UINT8 uc_comp_id, ui8Comp;
    STREAMTYPEW s_streamW;

    s_streamW.Offset = 0;
    s_streamW.Buffer = pui8BitStreamBuffer + *pui32BytesWritten;

    /* Start of scan */
    fPutBitsToBuffer(&s_streamW, 2, START_OF_SCAN);
    /* Scan header length */
    fPutBitsToBuffer(&s_streamW, 2, 6 + (pJPEGContext->pMTXSetup->ui32ComponentsInScan << 1));
    /* Number of image components in scan */
    fPutBitsToBuffer(&s_streamW, 1, pJPEGContext->pMTXSetup->ui32ComponentsInScan);
    for (ui8Comp = 0; ui8Comp < pJPEGContext->pMTXSetup->ui32ComponentsInScan; ui8Comp++) {
        uc_comp_id = ui8Comp + 1;

        /* Scan component selector */
        fPutBitsToBuffer(&s_streamW, 1, uc_comp_id);

        /*4 Bits Dc entropy coding table destination selector */
        /*4 Bits Ac entropy coding table destination selector */
        fPutBitsToBuffer(&s_streamW, 1, ((ui8Comp != 0 ? 1 : 0) << 4) | (ui8Comp != 0 ? 1 : 0)); // Huffman table refs = 0 Luma 1 Chroma
    }

    /* Start of spectral or predictor selection  */
    fPutBitsToBuffer(&s_streamW, 1, 0);
    /* End of spectral selection */
    fPutBitsToBuffer(&s_streamW, 1, 63);
    /*4 Bits Successive approximation bit position high (0)*/
    /*4 Bits Successive approximation bit position low or point transform (0)*/
    fPutBitsToBuffer(&s_streamW, 1, 0);

    *pui32BytesWritten += s_streamW.Offset;

    return 0;
}

static void InitializeScanCounter(TOPAZHP_JPEG_ENCODER_CONTEXT *pJPEGContext)
{
    pJPEGContext->sScan_Encode_Info.ui16SScan =
        pJPEGContext->sScan_Encode_Info.ui16CScan =
            pJPEGContext->sScan_Encode_Info.ui16ScansInImage;
}

static IMG_ERRORCODE PrepareHeader(TOPAZHP_JPEG_ENCODER_CONTEXT * pJPEGContext, IMG_CODED_BUFFER *pCBuffer, IMG_UINT32 ui32StartOffset, IMG_BOOL bIncludeHuffmanTables)
{
    IMG_ERRORCODE rc;
    IMG_UINT8 *ui8OutputBuffer;

    //Locate our JPEG Coded buffer
    ui8OutputBuffer = (IMG_UINT8 *)pCBuffer->pMemInfo;

    pCBuffer->ui32BytesWritten = ui32StartOffset;
    *((IMG_UINT32*)ui8OutputBuffer + pCBuffer->ui32BytesWritten) = 0;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Before writing headers, ui32BytesWritten: %d\n", pCBuffer->ui32BytesWritten);

    // JPGEncodeMarker - Currently misses out the APP0 header
    rc = JPGEncodeMarker(pJPEGContext, (IMG_UINT8 *) ui8OutputBuffer,  &pCBuffer->ui32BytesWritten, bIncludeHuffmanTables);
    if (rc) return rc;
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "After JPGEncodeMarker, ui32BytesWritten: %d\n", pCBuffer->ui32BytesWritten);

    rc = JPGEncodeHeader(pJPEGContext , (IMG_UINT8 *) ui8OutputBuffer ,  &pCBuffer->ui32BytesWritten);
    if (rc) return rc;
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "After JPGEncodeHeader, ui32BytesWritten: %d\n", pCBuffer->ui32BytesWritten);

    rc = JPGEncodeSOSHeader(pJPEGContext, (IMG_UINT8 *) ui8OutputBuffer, &pCBuffer->ui32BytesWritten);
    if (rc) return rc;
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "After JPGEncodeSOSHeader, ui32BytesWritten: %d\n", pCBuffer->ui32BytesWritten);

    return IMG_ERR_OK;
}

static IMG_ERRORCODE IssueBufferToHW(
    TOPAZHP_JPEG_ENCODER_CONTEXT *pJPEGContext,
    IMG_UINT16 ui16BCnt,
    IMG_INT8 i8PipeNumber,
    IMG_UINT32 ui32NoMCUsToEncode)
{
    MTX_ISSUE_BUFFERS *psBufferCmd;
    context_ENC_p ctx = (context_ENC_p)(pJPEGContext->ctx);
    context_ENC_frame_buf *ps_buf = &(ctx->ctx_frame_buf);

    pJPEGContext->sScan_Encode_Info.aBufferTable[ui16BCnt].ui32DataBufferUsedBytes = ((BUFFER_HEADER*)(pJPEGContext->sScan_Encode_Info.aBufferTable[ui16BCnt].pMemInfo))->ui32BytesUsed = -1; // Won't be necessary with SC Peek commands enabled

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Submit Scan %d which contains %d MCU in Buffer %d to MTX %d\n",
                             pJPEGContext->sScan_Encode_Info.aBufferTable[ui16BCnt].ui16ScanNumber,
                             ui32NoMCUsToEncode, ui16BCnt, i8PipeNumber);

    // Issue to MTX ////////////////////////////

    psBufferCmd = (MTX_ISSUE_BUFFERS *)(pJPEGContext->sScan_Encode_Info.aBufferTable[ui16BCnt].pMemInfo);
    ASSERT(psBufferCmd);

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "ui16ScansInImage: %d\n", pJPEGContext->sScan_Encode_Info.ui16ScansInImage);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "ui16ScanNumber: %d\n", pJPEGContext->sScan_Encode_Info.aBufferTable[ui16BCnt].ui16ScanNumber);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "ui32NumberMCUsToEncodePerScan: %d\n", pJPEGContext->sScan_Encode_Info.ui32NumberMCUsToEncodePerScan);
		
    psBufferCmd->ui32MCUCntAndResetFlag = (ui32NoMCUsToEncode << 1) | 0x1;

    psBufferCmd->ui32MCUPositionOfScanAndPipeNo =
        (((pJPEGContext->sScan_Encode_Info.ui16ScansInImage -
          pJPEGContext->sScan_Encode_Info.aBufferTable[ui16BCnt].ui16ScanNumber) *
         pJPEGContext->sScan_Encode_Info.ui32NumberMCUsToEncodePerScan)<<2)&(~2);

    ASSERT(0 == i8PipeNumber);
    if (i8PipeNumber <= 3)
        psBufferCmd->ui32MCUPositionOfScanAndPipeNo |= i8PipeNumber;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "ui32MCUPositionOfScanAndPipeNo: 0x%x\n", psBufferCmd->ui32MCUPositionOfScanAndPipeNo);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "ui32MCUCntAndResetFlag: 0x%x\n", psBufferCmd->ui32MCUCntAndResetFlag);

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psBufferCmd: 0x%x\n", (unsigned int)(psBufferCmd));
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Command Data: 0x%x\n", (unsigned int)(PTG_JPEG_HEADER_MAX_SIZE + ui16BCnt * pJPEGContext->ui32SizePerCodedBuffer));

    // Issue buffers
    tng_cmdbuf_insert_command(ctx->obj_context,
                                      0,
                                      MTX_CMDID_ISSUEBUFF,
                                      0,
                                      ps_buf->coded_buf->psb_buffer,
                                      PTG_JPEG_HEADER_MAX_SIZE + ui16BCnt * pJPEGContext->ui32SizePerCodedBuffer);

    return IMG_ERR_OK;
}

static void tng_jpeg_QueryConfigAttributes(
    VAProfile __maybe_unused profile,
    VAEntrypoint __maybe_unused entrypoint,
    VAConfigAttrib *attrib_list,
    int num_attribs)
{
    int i;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "tng_jpeg_QueryConfigAttributes\n");

    /* Return supported attributes */
    for (i = 0; i < num_attribs; i++) {
        switch (attrib_list[i].type) {
        case VAConfigAttribRTFormat:
            /* Already handled in psb_GetConfigAttributes */
            break;
        case VAConfigAttribEncJPEG:
            /* The below JPEG ENC capabilities are fixed by TopazHP and not changable. */
            {
                VAConfigAttribValEncJPEG* ptr = (VAConfigAttribValEncJPEG *)&(attrib_list[i].value);
                (ptr->bits).arithmatic_coding_mode = 0; /* Unsupported */
                (ptr->bits).progressive_dct_mode = 0; /* Unsupported */
                (ptr->bits).non_interleaved_mode = 1; /* Supported */
                (ptr->bits).differential_mode = 0; /* Unsupported */
                (ptr->bits).max_num_components = MTX_MAX_COMPONENTS; /* Only 3 is supported */
                (ptr->bits).max_num_scans = PTG_JPEG_MAX_SCAN_NUM;
                (ptr->bits).max_num_huffman_tables = 4; /* Only 4 is supported */
                (ptr->bits).max_num_huffman_tables = 2; /* Only 2 is supported */
            }
            break;
        case VAConfigAttribMaxPictureWidth:
        case VAConfigAttribMaxPictureHeight:
            /* No pure limitation on an image's width or height seperately, 
               as long as the image's MCUs need less than max_num_scans rounds of encoding
               and a surface of that source size is allocatable. */
            attrib_list[i].value = 0; /* No pure limitation */
            break;
        default:
            attrib_list[i].value = VA_ATTRIB_NOT_SUPPORTED;
            break;
        }
    }

    return;
}


static VAStatus tng_jpeg_ValidateConfig(
    object_config_p obj_config)
{
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "tng_jpeg_ValidateConfig\n");
    int i;
    /* Check all attributes */
    for (i = 0; i < obj_config->attrib_count; i++) {
        switch (obj_config->attrib_list[i].type) {
        case VAConfigAttribRTFormat:
            /* Ignore */
            break;
        case VAConfigAttribRateControl:
            break;
        default:
            return VA_STATUS_ERROR_ATTR_NOT_SUPPORTED;
        }
    }

    return VA_STATUS_SUCCESS;
}

static VAStatus tng_jpeg_CreateContext(
    object_context_p obj_context,
    object_config_p obj_config)
{
    int i;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    context_ENC_p ctx;
    TOPAZHP_JPEG_ENCODER_CONTEXT *jpeg_ctx_p;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "tng_jpeg_CreateContext\n");

    vaStatus = tng_CreateContext(obj_context, obj_config, 1);
    if (VA_STATUS_SUCCESS != vaStatus)
        return VA_STATUS_ERROR_ALLOCATION_FAILED;

    ctx = (context_ENC_p) obj_context->format_data;
    ctx->eCodec = IMG_CODEC_JPEG;

    ASSERT(0 == (ctx->ui16Width % 2));
    ASSERT(0 == (ctx->ui16FrameHeight % 2));

    for (i = 0; i < obj_config->attrib_count; i++) {
        if (VAConfigAttribRTFormat ==  obj_config->attrib_list[i].type) {
            switch (obj_config->attrib_list[i].value) {
            case VA_RT_FORMAT_YUV420:
                ctx->eFormat = IMG_CODEC_PL12;
                drv_debug_msg(VIDEO_DEBUG_GENERAL, "JPEG encoding: NV12 format chose.\n");
                break;
            default:
                drv_debug_msg(VIDEO_DEBUG_ERROR, "JPEG encoding: unsupported YUV format and force it to be NV12!\n");
                ctx->eFormat = IMG_CODEC_PL12;
                break;
            }
            break;
        }
    }

    ctx->jpeg_ctx = (TOPAZHP_JPEG_ENCODER_CONTEXT *)calloc(1, sizeof(TOPAZHP_JPEG_ENCODER_CONTEXT));
    if (NULL == ctx->jpeg_ctx)
        return VA_STATUS_ERROR_ALLOCATION_FAILED;
    jpeg_ctx_p = ctx->jpeg_ctx;
    jpeg_ctx_p->ctx = ctx;

    memset((void *)jpeg_ctx_p, 0x0, sizeof(jpeg_ctx_p));

    jpeg_ctx_p->NumCores = TOPAZHP_PIPE_NUM;
    jpeg_ctx_p->eFormat = ctx->eFormat;
    jpeg_ctx_p->ui32OutputWidth = ctx->ui16Width;
    jpeg_ctx_p->ui32OutputHeight = ctx->ui16FrameHeight;

    InitializeJpegEncode(jpeg_ctx_p);

    if ((jpeg_ctx_p->sScan_Encode_Info.ui16ScansInImage < 1) ||
        (jpeg_ctx_p->sScan_Encode_Info.ui16ScansInImage > PTG_JPEG_MAX_SCAN_NUM)) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "JPEG MCU scanning number(%d) is wrong!\n", jpeg_ctx_p->sScan_Encode_Info.ui16ScansInImage);
        free(ctx->jpeg_ctx);
        ctx->jpeg_ctx = NULL;
        return VA_STATUS_ERROR_UNKNOWN;
    }

    /*Allocate coded buffers' descripters */
    jpeg_ctx_p->sScan_Encode_Info.aBufferTable = (TOPAZHP_JPEG_BUFFER_INFO *)calloc(1, (jpeg_ctx_p->sScan_Encode_Info.ui8NumberOfCodedBuffers) * (sizeof(TOPAZHP_JPEG_BUFFER_INFO)));
    if (NULL == jpeg_ctx_p->sScan_Encode_Info.aBufferTable)
        return VA_STATUS_ERROR_ALLOCATION_FAILED;

    return vaStatus;
}


static void tng_jpeg_DestroyContext(
    object_context_p obj_context)
{
    context_ENC_p ctx;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "tng_jpeg_DestroyPicture\n");

    ctx = (context_ENC_p)(obj_context->format_data);

    if (ctx->jpeg_ctx) {
        if (ctx->jpeg_ctx->sScan_Encode_Info.aBufferTable) {
            free(ctx->jpeg_ctx->sScan_Encode_Info.aBufferTable);
            ctx->jpeg_ctx->sScan_Encode_Info.aBufferTable = NULL;
        }

        free(ctx->jpeg_ctx);
    }

    tng_DestroyContext(obj_context, 1);
}

static VAStatus tng__cmdbuf_lowpower(context_ENC_p ctx)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    tng_cmdbuf_p cmdbuf = ctx->obj_context->tng_cmdbuf;
    psb_driver_data_p driver_data = ctx->obj_context->driver_data;

    *cmdbuf->cmd_idx++ =
        ((MTX_CMDID_SW_LEAVE_LOWPOWER & MTX_CMDWORD_ID_MASK) << MTX_CMDWORD_ID_SHIFT) |
        ((ctx->ui32RawFrameCount  & MTX_CMDWORD_CORE_MASK) << MTX_CMDWORD_CORE_SHIFT) |
        (((driver_data->context_id & MTX_CMDWORD_COUNT_MASK) << MTX_CMDWORD_COUNT_SHIFT));

    tng_cmdbuf_insert_command_param(ctx->eCodec);

    return vaStatus;
}

static VAStatus tng_jpeg_BeginPicture(
    object_context_p obj_context)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    int ret;
    tng_cmdbuf_p cmdbuf;

    context_ENC_p ctx = (context_ENC_p) obj_context->format_data;
    TOPAZHP_JPEG_ENCODER_CONTEXT *jpeg_ctx_p = ctx->jpeg_ctx;
    context_ENC_frame_buf *ps_buf = &(ctx->ctx_frame_buf);
    psb_driver_data_p driver_data = ctx->obj_context->driver_data;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "tng_jpeg_BeginPicture: Frame %d\n", ctx->obj_context->frame_count);


    /* Get the current surface */
    ps_buf->src_surface = ctx->obj_context->current_render_target;


    /* Initialize the command buffer */
    ret = tng_context_get_next_cmdbuf(ctx->obj_context);
    if (ret) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "get next cmdbuf fail\n");
        vaStatus = VA_STATUS_ERROR_UNKNOWN;
        return vaStatus;
    }
    cmdbuf = ctx->obj_context->tng_cmdbuf;


    //For the first picture of a set to be encoded, need to ask kernel to perpare JPEG encoding
    if (ctx->obj_context->frame_count == 0) { /* first picture */

        *cmdbuf->cmd_idx++ = ((MTX_CMDID_SW_NEW_CODEC & MTX_CMDWORD_ID_MASK) << MTX_CMDWORD_ID_SHIFT) |
			     ((ctx->eCodec) << MTX_CMDWORD_CORE_SHIFT) |
                             (((driver_data->drm_context & MTX_CMDWORD_COUNT_MASK) << MTX_CMDWORD_COUNT_SHIFT));
        tng_cmdbuf_insert_command_param((ctx->ui16Width << 16) | ctx->ui16FrameHeight);
    }


    /* Map MTX setup buffer */
    vaStatus = psb_buffer_map(&cmdbuf->jpeg_header_mem, (unsigned char **)&cmdbuf->jpeg_header_mem_p);
    if (vaStatus) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Fail to map MTX setup buffer\n");
        return vaStatus;
    }
    jpeg_ctx_p->pMemInfoMTXSetup = cmdbuf->jpeg_header_mem_p;
    jpeg_ctx_p->pMTXSetup = (JPEG_MTX_DMA_SETUP*)jpeg_ctx_p->pMemInfoMTXSetup;
    memset(jpeg_ctx_p->pMemInfoMTXSetup, 0x0, ctx->jpeg_header_mem_size);


    /* Map MTX setup interface buffer */
    vaStatus = psb_buffer_map(&cmdbuf->jpeg_header_interface_mem, (unsigned char **)&cmdbuf->jpeg_header_interface_mem_p);
    if (vaStatus) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Fail to map MTX setup interface buffer\n");
        psb_buffer_unmap(&cmdbuf->jpeg_header_mem);
        return vaStatus;
    }
    jpeg_ctx_p->pMemInfoWritebackMemory = cmdbuf->jpeg_header_interface_mem_p;
    jpeg_ctx_p->pMTXWritebackMemory = (JPEG_MTX_WRITEBACK_MEMORY*)jpeg_ctx_p->pMemInfoWritebackMemory;
    memset(jpeg_ctx_p->pMemInfoWritebackMemory, 0x0, ctx->jpeg_header_interface_mem_size);


    /* Map quantization table buffer */
    vaStatus = psb_buffer_map(&cmdbuf->jpeg_pic_params, (unsigned char **)&cmdbuf->jpeg_pic_params_p);
    if (vaStatus) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Fail to map quantization table buffer\n");
        psb_buffer_unmap(&cmdbuf->jpeg_header_mem);
        psb_buffer_unmap(&cmdbuf->jpeg_header_interface_mem);
        return vaStatus;
    }
    jpeg_ctx_p->pMemInfoTableBlock = cmdbuf->jpeg_pic_params_p;
    jpeg_ctx_p->psTablesBlock = (JPEG_MTX_QUANT_TABLE *)jpeg_ctx_p->pMemInfoTableBlock;
    memset(jpeg_ctx_p->pMemInfoTableBlock, 0x0, ctx->jpeg_pic_params_size);

    vaStatus = tng__cmdbuf_lowpower(ctx);
    if (vaStatus != VA_STATUS_SUCCESS) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "cmdbuf lowpower\n");
    }

    /* Set SetupInterface*/
    SetSetupInterface(jpeg_ctx_p);
    IssueSetupInterface(jpeg_ctx_p);

    /* Set MTX setup struture */
    ret = SetMTXSetup(jpeg_ctx_p, ps_buf->src_surface);
    if (ret != IMG_ERR_OK)
        return ret;
    IssueMTXSetup(jpeg_ctx_p);

    /* Initialize the default quantization tables */
    SetDefaultQmatix(jpeg_ctx_p->pMemInfoTableBlock);

    /* Initialize scan counters */
    InitializeScanCounter(jpeg_ctx_p);

    tng_cmdbuf_buffer_ref(cmdbuf, &(ctx->obj_context->current_render_target->psb_surface->buf));

    return vaStatus;
}

static VAStatus ProcessQmatrixParam(context_ENC_p ctx, object_buffer_p obj_buffer)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAQMatrixBufferJPEG *pBuffer;
    JPEG_MTX_QUANT_TABLE* pQMatrix = (JPEG_MTX_QUANT_TABLE *)(ctx->jpeg_ctx->psTablesBlock);

    ASSERT(obj_buffer->type == VAQMatrixBufferType);

    pBuffer = (VAQMatrixBufferJPEG *)obj_buffer->buffer_data;

    if (0 != pBuffer->load_lum_quantiser_matrix) {
        memcpy(pQMatrix->aui8LumaQuantParams,
               pBuffer->lum_quantiser_matrix,
               QUANT_TABLE_SIZE_BYTES);
    }

    if (0 != pBuffer->load_chroma_quantiser_matrix) {
        memcpy(pQMatrix->aui8ChromaQuantParams,
               pBuffer->chroma_quantiser_matrix,
               QUANT_TABLE_SIZE_BYTES);
    }

    free(obj_buffer->buffer_data);
    obj_buffer->buffer_data = NULL;

    return vaStatus;
}

static VAStatus ProcessPictureParam(context_ENC_p ctx, object_buffer_p obj_buffer)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAEncPictureParameterBufferJPEG *pBuffer = NULL;
    BUFFER_HEADER *pBufHeader = NULL;
    TOPAZHP_JPEG_ENCODER_CONTEXT *jpeg_ctx = ctx->jpeg_ctx;
    JPEG_MTX_QUANT_TABLE* pQMatrix = (JPEG_MTX_QUANT_TABLE *)
                                     (ctx->jpeg_ctx->pMemInfoTableBlock);
    context_ENC_frame_buf *ps_buf = &(ctx->ctx_frame_buf);
    IMG_ERRORCODE rc;
    
    /* Check the input buffer */
    ASSERT(obj_buffer->type == VAEncPictureParameterBufferType);
    if ((obj_buffer->num_elements != 1) ||
        (obj_buffer->size != sizeof(VAEncPictureParameterBufferJPEG))) {
        return VA_STATUS_ERROR_UNKNOWN;
    }


    /* Lookup and get coded buffer */
    pBuffer = (VAEncPictureParameterBufferJPEG *)obj_buffer->buffer_data;

    /* Parameters checking */
    if (((pBuffer->pic_flags).bits.profile != 0) || /* Only "0 - Baseline" is supported */
       ((pBuffer->pic_flags).bits.progressive != 0) || /* Only "0 - sequential" is supported */
       ((pBuffer->pic_flags).bits.huffman != 1) || /* Only "1 - huffman" is supported */
       ((pBuffer->pic_flags).bits.interleaved != 0) || /* Only "0 - non interleaved" is supported */
       ((pBuffer->pic_flags).bits.differential != 0)) /* Only "0 - non differential" is supported */
        return VA_STATUS_ERROR_INVALID_PARAMETER;

    if ((pBuffer->sample_bit_depth != 8) || /* Only 8-bits sample depth is supported */
       (pBuffer->num_components != MTX_MAX_COMPONENTS) || /* Only 3 components setting is supported */
       (pBuffer->quality > 100))
        return VA_STATUS_ERROR_INVALID_PARAMETER;

    /* Set quality */
    if (pBuffer->quality != 0) { /* Quality value is set */
        CustomizeQuantizationTables(pQMatrix->aui8LumaQuantParams, 
                                    pQMatrix->aui8ChromaQuantParams,
                                    pBuffer->quality);
    }

    ASSERT(ctx->ui16Width == pBuffer->picture_width);
    ASSERT(ctx->ui16FrameHeight == pBuffer->picture_height);

    ps_buf->coded_buf = BUFFER(pBuffer->coded_buf);

    free(pBuffer);
    obj_buffer->buffer_data = NULL;
    obj_buffer->size = 0;

    if (NULL == ps_buf->coded_buf) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s L%d Invalid coded buffer handle\n", __FUNCTION__, __LINE__);
        return VA_STATUS_ERROR_INVALID_BUFFER;
    }

    /* Map coded buffer */
    vaStatus = psb_buffer_map(ps_buf->coded_buf->psb_buffer, (unsigned char **)&jpeg_ctx->jpeg_coded_buf.pMemInfo);
    if (vaStatus) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "ERROR: Map coded_buf failed!");
        return vaStatus;
    }
    jpeg_ctx->jpeg_coded_buf.ui32Size = ps_buf->coded_buf->size;
    jpeg_ctx->jpeg_coded_buf.sLock = BUFFER_FREE;
    jpeg_ctx->jpeg_coded_buf.ui32BytesWritten = 0;

    if ((jpeg_ctx->jpeg_coded_buf.ui32Size) < (9 + 6 + (4 * 3))) {
        return VA_STATUS_ERROR_INVALID_BUFFER;
    }


    /* Assign coded buffer to each scan */
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Assign coded buffer to each scan\n");
    AssignCodedDataBuffers(jpeg_ctx);

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Coded buffer total size is %d,"
                             "coded segment size per scan is %d\n",
                             jpeg_ctx->jpeg_coded_buf.ui32Size, jpeg_ctx->ui32SizePerCodedBuffer);


    /* Write JPEG headers to coded buffer */
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Write JPEG Headers to coded buf\n");

    pBufHeader = (BUFFER_HEADER *)jpeg_ctx->jpeg_coded_buf.pMemInfo;
    pBufHeader->ui32BytesUsed = 0; /* Not include BUFFER_HEADER*/

    rc = PrepareHeader(jpeg_ctx, &jpeg_ctx->jpeg_coded_buf, sizeof(BUFFER_HEADER), IMG_TRUE);
    if (rc != IMG_ERR_OK)
        return VA_STATUS_ERROR_UNKNOWN;

    pBufHeader->ui32Reserved3 = PTG_JPEG_HEADER_MAX_SIZE;//Next coded buffer offset
    pBufHeader->ui32BytesUsed = jpeg_ctx->jpeg_coded_buf.ui32BytesWritten - sizeof(BUFFER_HEADER);

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "sizeof(BUFFER_HEADER): %d, ui32BytesUsed :%d, ui32Reserved3: %d, ui32BytesWritten: %d\n",
                             sizeof(BUFFER_HEADER), pBufHeader->ui32BytesUsed, pBufHeader->ui32Reserved3, jpeg_ctx->jpeg_coded_buf.ui32BytesWritten);

    return vaStatus;
}
static VAStatus tng_jpeg_RenderPicture(
    object_context_p obj_context,
    object_buffer_p *buffers,
    int num_buffers)
{
    context_ENC_p ctx = (context_ENC_p) obj_context->format_data;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    int i;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "tng_jpeg_RenderPicture\n");

    for (i = 0; i < num_buffers; i++) {
        object_buffer_p obj_buffer = buffers[i];

        switch (obj_buffer->type) {
        case VAQMatrixBufferType:
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "tng_jpeg_RenderPicture got VAEncSliceParameterBufferType\n");
            vaStatus = ProcessQmatrixParam(ctx, obj_buffer);
            DEBUG_FAILURE;
            break;
        case VAEncPictureParameterBufferType:
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "tng_jpeg_RenderPicture got VAEncPictureParameterBufferType\n");
            vaStatus = ProcessPictureParam(ctx, obj_buffer);
            DEBUG_FAILURE;
            break;
        case VAEncSliceParameterBufferType:
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "tng_jpeg_RenderPicture got VAEncSliceParameterBufferType\n");
            drv_debug_msg(VIDEO_DEBUG_WARNING, "VAEncSliceParameterBufferType is ignored on TopazHP\n");
            vaStatus = VA_STATUS_SUCCESS;
            DEBUG_FAILURE;
            break;
        default:
            vaStatus = VA_STATUS_ERROR_UNKNOWN;
            DEBUG_FAILURE;
        }
    }

    return vaStatus;
}

static VAStatus tng_jpeg_EndPicture(
    object_context_p obj_context)
{
    IMG_UINT16 ui16BCnt;
    IMG_UINT32 rc = 0;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    IMG_UINT32 ui32NoMCUsToEncode = 0;
    IMG_UINT32 ui32RemainMCUs = 0;

    context_ENC_p ctx = (context_ENC_p) obj_context->format_data;
    TOPAZHP_JPEG_ENCODER_CONTEXT *jpeg_ctx_p = ctx->jpeg_ctx;
    tng_cmdbuf_p cmdbuf = (tng_cmdbuf_p)ctx->obj_context->tng_cmdbuf;
    context_ENC_mem *ps_mem = &(ctx->ctx_mem[ctx->ui32StreamID]);
    context_ENC_frame_buf *ps_buf = &(ctx->ctx_frame_buf);

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "tng_jpeg_EndPicture\n");

    IssueQmatix(jpeg_ctx_p);

    /* Compute the next scan to be sent */
    ui32RemainMCUs = jpeg_ctx_p->sScan_Encode_Info.ui32NumberMCUsToEncode;
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "ui32RemainMCUs: %d\n", ui32RemainMCUs);

    for (ui16BCnt = 0; (ui16BCnt < jpeg_ctx_p->sScan_Encode_Info.ui8NumberOfCodedBuffers)
         && (jpeg_ctx_p->sScan_Encode_Info.ui16SScan > 0); ui16BCnt++) {

        jpeg_ctx_p->sScan_Encode_Info.aBufferTable[ui16BCnt].ui16ScanNumber = jpeg_ctx_p->sScan_Encode_Info.ui16SScan--;
        jpeg_ctx_p->sScan_Encode_Info.aBufferTable[ui16BCnt].i8PipeNumber =
            (1 == jpeg_ctx_p->NumCores) ? 0 : ((ui16BCnt+1) % jpeg_ctx_p->NumCores);

        if (jpeg_ctx_p->sScan_Encode_Info.ui16SScan > 0) {
            ui32NoMCUsToEncode = jpeg_ctx_p->sScan_Encode_Info.ui32NumberMCUsToEncodePerScan;
        } else {
            // Final scan, may need fewer MCUs than buffer size, calculate the remainder
            ui32NoMCUsToEncode = ui32RemainMCUs;
            jpeg_ctx_p->sScan_Encode_Info.aBufferTable[ui16BCnt].i8PipeNumber = 0;
        }

        drv_debug_msg(VIDEO_DEBUG_GENERAL, "ui32NoMCUsToEncode: %d\n", ui32NoMCUsToEncode);
        //Send scan to MTX
        rc = IssueBufferToHW(jpeg_ctx_p, ui16BCnt,
                             jpeg_ctx_p->sScan_Encode_Info.aBufferTable[ui16BCnt].i8PipeNumber,
                             ui32NoMCUsToEncode);
        if (rc != IMG_ERR_OK) {
            vaStatus = VA_STATUS_ERROR_UNKNOWN;
            DEBUG_FAILURE;
            return vaStatus;
        }

        ui32RemainMCUs -= ui32NoMCUsToEncode;
    }


    psb_buffer_unmap(&cmdbuf->jpeg_pic_params);
    cmdbuf->jpeg_pic_params_p = NULL;
    psb_buffer_unmap(&cmdbuf->jpeg_header_mem);
    cmdbuf->jpeg_header_mem_p = NULL;
    psb_buffer_unmap(ps_buf->coded_buf->psb_buffer);
    jpeg_ctx_p->jpeg_coded_buf.pMemInfo = NULL;

    psb_buffer_unmap(&(ctx->bufs_writeback));


    //tng__trace_cmdbuf(cmdbuf);

    if (tng_context_flush_cmdbuf(ctx->obj_context)) {
        vaStatus = VA_STATUS_ERROR_UNKNOWN;
        return vaStatus;
    }

    ctx->obj_context->frame_count++;
    return VA_STATUS_SUCCESS;
}

/* Add Restart interval termination (RSTm)to coded buf 1 ~ NumCores-1*/
static inline VAStatus tng_OutputResetIntervalToCB(IMG_UINT8 *pui8Buf, IMG_UINT8 ui8_marker)
{
    if (NULL == pui8Buf)
        return VA_STATUS_ERROR_UNKNOWN;
    /*Refer to CCITT Rec. T.81 (1992 E), B.2.1*/
    /*RSTm: Restart marker conditional marker which is placed between
     * entropy-coded segments only if restartis enabled. There are 8 unique
     * restart markers (m = 0 - 7) which repeat in sequence from 0 to 7, starting with
     * zero for each scan, to provide a modulo 8 restart interval count*/

    *pui8Buf++ = 0xff;
    *pui8Buf = (ui8_marker & 0x7) | 0xD0;

    return 0;
}

VAStatus tng_jpeg_AppendMarkers(object_context_p obj_context, void *raw_coded_buf)
{
    context_ENC_p ctx = (context_ENC_p) obj_context->format_data;
    TOPAZHP_JPEG_ENCODER_CONTEXT *jpeg_ctx_p = ctx->jpeg_ctx;

    IMG_UINT16 ui16BCnt;
    BUFFER_HEADER* pBufHeader;
    STREAMTYPEW s_streamW;
    void *pSegStart = raw_coded_buf;

    if (pSegStart == NULL) {
        return VA_STATUS_ERROR_UNKNOWN;
    }

    pBufHeader = (BUFFER_HEADER *)pSegStart;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Number of Coded buffers %d, Per Coded Buffer size : %d\n",
                             jpeg_ctx_p->sScan_Encode_Info.ui8NumberOfCodedBuffers,
                             jpeg_ctx_p->ui32SizePerCodedBuffer);

    /*The first part of coded buffer contains JPEG headers*/
    pBufHeader->ui32Reserved3 = PTG_JPEG_HEADER_MAX_SIZE;
    jpeg_ctx_p->jpeg_coded_buf.ui32BytesWritten = 0;

    for (ui16BCnt = 0;
         ui16BCnt < jpeg_ctx_p->sScan_Encode_Info.ui8NumberOfCodedBuffers;
         ui16BCnt++) {
        pBufHeader = (BUFFER_HEADER *)pSegStart;
        pBufHeader->ui32Reserved3 =
            PTG_JPEG_HEADER_MAX_SIZE + jpeg_ctx_p->ui32SizePerCodedBuffer * ui16BCnt ;

        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Coded Buffer Part %d, size %d, next part offset: %d\n",
                                 ui16BCnt, pBufHeader->ui32BytesUsed, pBufHeader->ui32Reserved3);

        if (ui16BCnt > 0) {
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "Append 2 bytes Reset Interval %d "
                                     "to Coded Buffer Part %d\n", ui16BCnt - 1, ui16BCnt);

            // OUTPUT RESTART INTERVAL TO CODED BUFFER
            tng_OutputResetIntervalToCB(
                (IMG_UINT8 *)((IMG_UINT32)pSegStart + sizeof(BUFFER_HEADER) + pBufHeader->ui32BytesUsed),
                ui16BCnt - 1);

            pBufHeader->ui32BytesUsed += 2;
        }

        jpeg_ctx_p->jpeg_coded_buf.ui32BytesWritten += pBufHeader->ui32BytesUsed;
        pSegStart = (void *)((IMG_UINT32)raw_coded_buf + pBufHeader->ui32Reserved3);
    }

    pBufHeader = (BUFFER_HEADER *)pSegStart;
    pBufHeader->ui32Reserved3 = 0; /*Last Part of Coded Buffer*/
    jpeg_ctx_p->jpeg_coded_buf.ui32BytesWritten += pBufHeader->ui32BytesUsed;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Coded Buffer Part %d, size %d, next part offset: %d\n",
                             ui16BCnt, pBufHeader->ui32BytesUsed, pBufHeader->ui32Reserved3);

    s_streamW.Buffer = pSegStart;
    s_streamW.Offset = (sizeof(BUFFER_HEADER) + pBufHeader->ui32BytesUsed);

    fPutBitsToBuffer(&s_streamW, 2, END_OF_IMAGE);

    pBufHeader->ui32BytesUsed += 2;
    jpeg_ctx_p->jpeg_coded_buf.ui32BytesWritten += 2;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Add two bytes to last part of coded buffer,"
                             " total: %d\n", jpeg_ctx_p->jpeg_coded_buf.ui32BytesWritten);
    return VA_STATUS_SUCCESS;
}

struct format_vtable_s tng_JPEGES_vtable = {
queryConfigAttributes:
    tng_jpeg_QueryConfigAttributes,
validateConfig:
    tng_jpeg_ValidateConfig,
createContext:
    tng_jpeg_CreateContext,
destroyContext:
    tng_jpeg_DestroyContext,
beginPicture:
    tng_jpeg_BeginPicture,
renderPicture:
    tng_jpeg_RenderPicture,
endPicture:
    tng_jpeg_EndPicture
};
