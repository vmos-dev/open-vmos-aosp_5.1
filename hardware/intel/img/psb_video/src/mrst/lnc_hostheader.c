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
 *    Shengquan Yuan  <shengquan.yuan@intel.com>
 *    Binglin Chen <binglin.chen@intel.com>
 *
 */


#include <stdio.h>
#include <string.h>
#include "img_types.h"
#include "psb_def.h"
#include "lnc_hostheader.h"
#include "psb_drv_debug.h"


/* Global stores the latest QP information for the DoHeader()
 * routine, should be filled in by the rate control algorithm.
 */
#define HEADERS_VERBOSE_OUTPUT 0

/* #define USESTATICWHEREPOSSIBLE 1 */

#define MAXNUMBERELEMENTS 16


/* SOME USEFUL TEST FUNCTIONS */
#ifndef TOPAZ_MTX_HW
static void Show_Bits(
    IMG_UINT8 *ucBitStream,
    IMG_UINT32 ByteStartBit,
    IMG_UINT32 Bits)
{
    char Txt[1024];
    IMG_UINT32 uiByteSize;
    IMG_UINT32 uiLp, uiBt, Bcnt;
    Bcnt = 0;
    uiByteSize = (Bits + ByteStartBit + 7) >> 3;
    for (uiLp = 0; uiLp < uiByteSize; uiLp++) {
        snprintf(Txt, strlen(" "), " ");
        for (uiBt = 128; uiBt >= 1; uiBt = uiBt >> 1) {
            Bcnt++;
            if (Bcnt > Bits + ByteStartBit || Bcnt <= ByteStartBit)
                snprintf(Txt, sizeof(Txt), "%sX", Txt);
            else
                snprintf(Txt, sizeof(Txt), "%s%i", Txt, (ucBitStream[uiLp] & uiBt) > 0);
        }

        snprintf(Txt, sizeof(Txt), "%s ", Txt);
        printf("%s", Txt);
        if ((uiLp + 1) % 8 == 0) printf("\n");
    }

    printf("\n\n");
}
#endif

#ifndef TOPAZ_MTX_HW

static void Show_Elements(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p)
{
    IMG_UINT8 f;
    IMG_UINT32 TotalByteSize;
    IMG_UINT32 RTotalByteSize;

    RTotalByteSize = TotalByteSize = 0;
    for (f = 0; f < mtx_hdr->Elements; f++) {
#if HEADERS_VERBOSE_OUTPUT
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Encoding Element [%i] - Type:%i\n", f, elt_p[f]->Element_Type);
#endif
        if (elt_p[f]->Element_Type == ELEMENT_STARTCODE_RAWDATA ||
            elt_p[f]->Element_Type == ELEMENT_RAWDATA) {
            TotalByteSize = elt_p[f]->Size;
#if HEADERS_VERBOSE_OUTPUT
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "Writing %i RAW bits to element.\n", elt_p[f]->Size);
            Show_Bits((IMG_UINT8 *)(&elt_p[f]->Size) + 1, 0, TotalByteSize);
#endif
            TotalByteSize += 8;

            RTotalByteSize += (((IMG_UINT32)((TotalByteSize + 7) / 8)) * 8);
            RTotalByteSize += 32;
        } else {
            TotalByteSize = 0;
            switch (elt_p[f]->Element_Type) {
            case ELEMENT_QP:
#if HEADERS_VERBOSE_OUTPUT
                drv_debug_msg(VIDEO_DEBUG_GENERAL, "Insert token ELEMENT_QP (H264)- for MTX to generate and insert this value\n");
#endif
                break;
            case ELEMENT_SQP:
#if HEADERS_VERBOSE_OUTPUT
                drv_debug_msg(VIDEO_DEBUG_GENERAL, "Insert token ELEMENT_SQP (H264)- for MTX to generate and insert this value\n");
#endif
                break;
            case ELEMENT_FRAMEQSCALE:
#if HEADERS_VERBOSE_OUTPUT
                drv_debug_msg(VIDEO_DEBUG_GENERAL, "Insert token ELEMENT_FRAMEQSCALE (H263/MPEG4) - for MTX to generate and insert this value\n");
#endif
                break;
            case ELEMENT_SLICEQSCALE:
#if HEADERS_VERBOSE_OUTPUT
                drv_debug_msg(VIDEO_DEBUG_GENERAL, "Insert token ELEMENT_SLICEQSCALE (H263/MPEG4) - for MTX to generate and insert this value\n");
#endif
                break;
            case ELEMENT_INSERTBYTEALIGN_H264:
#if HEADERS_VERBOSE_OUTPUT
                drv_debug_msg(VIDEO_DEBUG_GENERAL, "Insert token ELEMENT_INSERTBYTEALIGN_H264 -  MTX to generate 'rbsp_trailing_bits()' field\n");
#endif
                break;
            case ELEMENT_INSERTBYTEALIGN_MPG4:
#if HEADERS_VERBOSE_OUTPUT
                drv_debug_msg(VIDEO_DEBUG_GENERAL, "Insert token ELEMENT_INSERTBYTEALIGN_MPG4 -  MTX to generate MPEG4 'byte_aligned_bits' field\n");
#endif
                break;
            default:
                break;
            }

            RTotalByteSize += 32;
#if HEADERS_VERBOSE_OUTPUT
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "No RAW bits\n\n");
#endif
        }
    }

    /* TotalByteSize=TotalByteSize+32+(&elt_p[f-1]->Element_Type-&elt_p[0]->Element_Type)*8; */

#if HEADERS_VERBOSE_OUTPUT
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "\nCombined ELEMENTS Stream:\n");
    Show_Bits((IMG_UINT8 *) pMTX_Header->asElementStream, 0, RTotalByteSize);
#endif
}
#endif


/**
 * Header Writing Functions
 * Low level bit writing and ue, se functions
 * HOST CODE
 */
static void lnc__write_upto8bits_elements(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p,
    IMG_UINT8 wrt_bits,
    IMG_UINT16 bit_cnt)
{
    /* This is the core function to write bits/bytes
     * to a header stream, it writes them directly to ELEMENT structures.
     */
    IMG_UINT8 *wrt_bytes_p;
    IMG_UINT8 *size_bits_p;
    union {
        IMG_UINT32 UI16Input;
        IMG_UINT8 UI8Input[2];
    } InputVal = {0, };
    IMG_UINT8 OutByteIndex;
    IMG_INT16 Shift;

    if (bit_cnt == 0) return;

    /* WA for klockwork */
    if (mtx_hdr->Elements >= MAXNUMBERELEMENTS) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "mtx_hdr->Elments overflow\n");
        return;
    }

    /* First ensure that unused bits in wrt_bits are zeroed */
    wrt_bits &= (0x00ff >> (8 - bit_cnt));

    InputVal.UI16Input = 0;

    /* Pointer to the bit count field */
    size_bits_p = &(elt_p[mtx_hdr->Elements]->Size);

    /* Pointer to the space where header bits are to be written */
    wrt_bytes_p = &(elt_p[mtx_hdr->Elements]->Bits);

    OutByteIndex = (size_bits_p[0] / 8);

    if (!(size_bits_p[0] & 7)) {
        if (size_bits_p[0] >= 120) {
            /* Element maximum bits send to element, time to start a new one */
            mtx_hdr->Elements++; /* Increment element index */
            /* Element pointer set to position of next element (120/8 = 15 bytes) */
            elt_p[mtx_hdr->Elements] = (MTX_HEADER_ELEMENT *) & wrt_bytes_p[15];

            /* Write ELEMENT_TYPE */
            elt_p[mtx_hdr->Elements]->Element_Type = ELEMENT_RAWDATA;

            /* Set new element size (bits) to zero */
            elt_p[mtx_hdr->Elements]->Size = 0;

            /* Begin writing to the new element */
            lnc__write_upto8bits_elements(mtx_hdr, elt_p, wrt_bits, bit_cnt);
            return;
        }

        wrt_bytes_p[OutByteIndex] = 0; /* Beginning a new byte, clear byte */
    }

    Shift = (IMG_INT16)((8 - bit_cnt) - (size_bits_p[0] & 7));

    if (Shift >= 0) {
        wrt_bits <<= Shift;
        wrt_bytes_p[OutByteIndex] |= wrt_bits;
        size_bits_p[0] = size_bits_p[0] + bit_cnt;
    } else {
        InputVal.UI8Input[1] = (IMG_UINT8) wrt_bits + 256;
        InputVal.UI16Input >>= -Shift;

        wrt_bytes_p[OutByteIndex] |= InputVal.UI8Input[1];

        size_bits_p[0] = size_bits_p[0] + bit_cnt;
        size_bits_p[0] = size_bits_p[0] - ((IMG_UINT8) - Shift);
        InputVal.UI8Input[0] = InputVal.UI8Input[0] >> (8 + Shift);

        lnc__write_upto8bits_elements(mtx_hdr, elt_p, InputVal.UI8Input[0], (IMG_UINT16) - Shift);
    }
}

static void lnc__write_upto32bits_elements(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p,
    IMG_UINT32 wrt_bits,
    IMG_UINT32 bit_cnt)
{
    IMG_UINT32 BitLp;
    IMG_UINT32 EndByte;
    IMG_UINT8 Bytes[4];

    for (BitLp = 0; BitLp < 4; BitLp++) {
        Bytes[BitLp] = (IMG_UINT8)(wrt_bits & 255);
        wrt_bits = wrt_bits >> 8;
    }

    EndByte = ((bit_cnt + 7) / 8);

    if ((bit_cnt) % 8)
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, Bytes[EndByte-1], (IMG_UINT8)((bit_cnt) % 8));
    else
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, Bytes[EndByte-1], 8);

    if (EndByte > 1)
        for (BitLp = EndByte - 1; BitLp > 0; BitLp--) {
            lnc__write_upto8bits_elements(mtx_hdr, elt_p, Bytes[BitLp-1], 8);
        }
}

static void lnc__generate_ue(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p,
    IMG_UINT32 uiVal)
{
    IMG_UINT32 uiLp;
    IMG_UINT8 ucZeros;
    IMG_UINT32 uiChunk;

    for (uiLp = 1, ucZeros = 0; (uiLp - 1) < uiVal ; uiLp = uiLp + uiLp, ucZeros++)
        uiVal = uiVal - uiLp;

    /* ucZeros = number of preceding zeros required
     * uiVal = value to append after zeros and 1 bit
     */

    /* Write preceding zeros */
    for (uiLp = (IMG_UINT32) ucZeros; uiLp + 1 > 8; uiLp -= 8)
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 8);

    /* Write zeros and 1 bit set */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, (IMG_UINT8) 1, (IMG_UINT8)(uiLp + 1));

    /* Write Numeric part */
    while (ucZeros > 8) {
        ucZeros -= 8;
        uiChunk = (uiVal >> ucZeros);
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, (IMG_UINT8) uiChunk, 8);
        uiVal = uiVal - (uiChunk << ucZeros);
    }

    lnc__write_upto8bits_elements(mtx_hdr, elt_p, (IMG_UINT8) uiVal, ucZeros);
}


static void lnc__generate_se(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p,
    int iVal)
{
    IMG_UINT32 uiCodeNum;

    if (iVal > 0)
        uiCodeNum = (IMG_UINT32)(iVal + iVal - 1);
    else
        uiCodeNum = (IMG_UINT32)(-iVal - iVal);

    lnc__generate_ue(mtx_hdr, elt_p, uiCodeNum);
}


static void lnc__insert_element_token(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p,
    HEADER_ELEMENT_TYPE Token)
{
    IMG_UINT8 Offset;
    IMG_UINT8 *P;

    if (mtx_hdr->Elements != ELEMENTS_EMPTY) {
        if (elt_p[mtx_hdr->Elements]->Element_Type == ELEMENT_STARTCODE_RAWDATA
            || elt_p[mtx_hdr->Elements]->Element_Type == ELEMENT_RAWDATA) {
            /*Add a new element aligned to word boundary
             *Find RAWBit size in bytes (rounded to word boundary))
             */

            /* NumberofRawbits (excluding size of bit count field)+ size of the bitcount field  */
            Offset = elt_p[mtx_hdr->Elements]->Size + 8 + 31;
            Offset /= 32;/* Now contains rawbits size in words */
            Offset += 1;/* Now contains rawbits+element_type size in words */
            Offset *= 4;/* Convert to number of bytes (total size of structure in bytes, aligned to word boundary) */
        } else {
            Offset = 4;
        }

        mtx_hdr->Elements++;
        P = (IMG_UINT8 *) elt_p[mtx_hdr->Elements-1];
        P += Offset;
        elt_p[mtx_hdr->Elements] = (MTX_HEADER_ELEMENT *) P;
    } else
        mtx_hdr->Elements = 0;

    elt_p[mtx_hdr->Elements]->Element_Type = Token;
    elt_p[mtx_hdr->Elements]->Size = 0;
}

/*
 * Intermediary functions to build H264 headers
 */
static void lnc__H264_writebits_startcode_prefix_element(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p,
    IMG_UINT32 ByteSize)
{
    /* GENERATES THE FIRST ELEMENT OF THE H264_SEQUENCE_HEADER() STRUCTURE */
    IMG_UINT32 Lp;

    /*
     * Byte aligned (bit 0)
     * (3 bytes in slice header when slice is first in a picture
     * without sequence/picture_header before picture
     */

    for (Lp = 0; Lp < ByteSize - 1; Lp++)
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 8);

    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 8);

    /* Byte aligned (bit 32 or 24) */
    return;
}

static void lnc__H264_writebits_sequence_header0(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p,
    H264_SEQUENCE_HEADER_PARAMS *pSHParams)
{
    /* GENERATES THE FIRST ELEMENT OF THE H264_SEQUENCE_HEADER() STRUCTURE */

    /* 4 Byte StartCodePrefix Pregenerated in: lnc__H264_writebits_startcode_prefix_element()
     * Byte aligned (bit 32)
     */
    lnc__write_upto8bits_elements(
        mtx_hdr,
        elt_p, (0 << 7) |/* forbidden_zero_bit=0 */
        (0x3 << 5) |/* nal_ref_idc=01 (may be 11) */
        (7), /* nal_unit_type=00111 */
        8);


    /* Byte aligned (bit 40)
     * profile_idc = 8 bits = 66 for BP (PROFILE_IDC_BP), 77 for MP (PROFILE_IDC_MP)
     */
    lnc__write_upto8bits_elements(
        mtx_hdr,
        elt_p,
        (pSHParams->ucProfile == SH_PROFILE_BP ? 66 : 77),
        8);

    /* Byte aligned (bit 48) */
    lnc__write_upto8bits_elements(
        mtx_hdr,
        elt_p,
        (1 << 7) |/* constrain_set0_flag = 0 for MP, 1 for BP */
        (1 << 6) |/* constrain_set1_flag = 0 for BP, 1 for MP */
        (0 << 5) | /* constrain_set2_flag = always 0 in BP/MP */
        ((pSHParams->ucLevel == SH_LEVEL_1B ? 1 : 0) << 4),     /* constrain_set3_flag = 1 for level 1b, 0 for others */
        /* reserved_zero_4bits = 0 */
        8);

    /* Byte aligned (bit 56) */
    lnc__write_upto8bits_elements(
        mtx_hdr,
        elt_p,
        (IMG_UINT8)((pSHParams->ucLevel > 100) ? (pSHParams->ucLevel - 100) : pSHParams->ucLevel) ,
        8);/* level_idc (8 bits) = 11 for 1b, 10xlevel for others */

    /* Byte aligned (bit 64) */
    lnc__write_upto8bits_elements(
        mtx_hdr, elt_p, (1 << 7) | /* seq_parameter_Set_id = 0 in Topaz ->  ue(0)= 1b */
        (2 << 4) | /* log2_max_frame_num_minus4 = 1 in Topaz ->  ue(1)= 010b */
        (1 << 3) | /* pic_order_cnt_type = 0 in Topaz ->  ue(0)= 1b */
        (3), /* log2_max_pic_order_cnt_Isb_minus4 = 2 in Topaz ->  ue(2)= 011b */
        8);
}

static void lnc__H264_writebits_sequence_header1(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p,
    H264_SEQUENCE_HEADER_PARAMS *pSHParams,
    H264_CROP_PARAMS *psCropParams)
{
    /* GENERATES THE SECOND, VARIABLE LENGTH, ELEMENT OF THE H264_SEQUENCE_HEADER() STRUCTURE
     * ELEMENT BITCOUNT: xx
     * pic_width_in_mbs_minus1: ue(v) from 10 to 44 (176 to 720 pixel per row)
     */
    /* max_num_ref_frames = 1 ue(1) = 010b */
    lnc__generate_ue(mtx_hdr, elt_p, pSHParams->ucMax_num_ref_frames);
    /* gaps_in_frame_num_value_allowed_Flag - (1 bit) - Not supported
     * in Topaz (0) */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

    lnc__generate_ue(mtx_hdr, elt_p, pSHParams->ucWidth_in_mbs_minus1);

    /* pic_height_in_maps_units_minus1: ue(v) Value from 8 to 35 (144 to 576 pixels per column) */
    lnc__generate_ue(mtx_hdr, elt_p, pSHParams->ucHeight_in_maps_units_minus1);

    /* We don't know the alignment at this point, so will have to use bit writing functions */
    lnc__write_upto8bits_elements(
        mtx_hdr, elt_p,
        (1 << 2) | /* frame_mb_only_flag (always 1) */
        (1 << 1), /* direct_8x8_inference_flag=1 in Topaz */
        2);

    if (psCropParams && psCropParams->bClip) {
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);
        lnc__generate_ue(mtx_hdr, elt_p, psCropParams->LeftCropOffset);
        lnc__generate_ue(mtx_hdr, elt_p, psCropParams->RightCropOffset);
        lnc__generate_ue(mtx_hdr, elt_p, psCropParams->TopCropOffset);
        lnc__generate_ue(mtx_hdr, elt_p, psCropParams->BottomCropOffset);

    } else {
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);
    }
}


static void lnc__H264_writebits_VUI_params(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p,
    H264_VUI_PARAMS *VUIParams)
{
    /* Builds VUI Params for the Sequence Header (only present in the 1st sequence of stream) */
    lnc__write_upto8bits_elements(
        mtx_hdr, elt_p,
        (0 << 4) | /* aspect_ratio_info_present_flag = 0 in Topaz */
        (0 << 3) | /* overscan_info_present_flag (1 bit) = 0 in Topaz */
        (0 << 2) | /* video_signal_type_present_flag (1 bit) = 0 in Topaz */
        (0 << 1) | /* chroma_loc_info_present_flag (1 bit) = 0 in Topaz */
        (1),/* timing_info_present_flag (1 bit) = 1 in Topaz */
        5);

    /* num_units_in_tick (32 bits) = 1 in Topaz */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 8);
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 8);
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 8);
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 8);

    /* time_scale (32 bits) = frame rate */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 8);
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 8);
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 8);
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, (IMG_UINT8)  VUIParams->Time_Scale, 8);

    /* fixed_frame_rate_flag (1 bit) = 1 in Topaz */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);
    /* nal_hrd_parameters_present_flag (1 bit) = 1 in Topaz */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);

    /** Definitions for nal_hrd_parameters() contained in VUI structure for Topaz
     *  cpb_cnt_minus1 ue(v) = 0 in Topaz = 1b
     */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);
    /* bit_rate_scale (4 bits) = 0 in Topaz, cpb_size_scale (4 bits) = 0 in Topaz */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 2, 8);

    /* bit_rate_value_minus1[0] ue(v) = (Bitrate/64)-1 [RANGE:0 to (2^32)-2] */
    lnc__generate_ue(mtx_hdr, elt_p, VUIParams->bit_rate_value_minus1);
    /* cpb_size_value_minus1[0] ue(v) = (CPB_Bits_Size/16)-1
     * where CPB_Bits_Size = 1.5 * Bitrate  [RANGE:0 to (2^32)-2]
     */
    lnc__generate_ue(mtx_hdr, elt_p, VUIParams->cbp_size_value_minus1);

    /* cbr_flag[0] (1 bit) = 0 for VBR, 1 for CBR */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, VUIParams->CBR, 1);

    lnc__write_upto8bits_elements(
        mtx_hdr, elt_p,
        VUIParams->initial_cpb_removal_delay_length_minus1,
        5); /* initial_cpb_removal_delay_length_minus1 (5 bits) = ??? */

    lnc__write_upto8bits_elements(
        mtx_hdr,
        elt_p,
        VUIParams->cpb_removal_delay_length_minus1,
        5); /* cpb_removal_delay_length_minus1 (5 bits) = ??? */

    lnc__write_upto8bits_elements(
        mtx_hdr, elt_p,
        VUIParams->dpb_output_delay_length_minus1,
        5); /* dpb_output_delay_length_minus1 (5 bits) = ??? */

    lnc__write_upto8bits_elements(
        mtx_hdr,
        elt_p,
        VUIParams->time_offset_length,
        5); /* time_offst_length (5 bits) = ??? */

    /* End of nal_hrd_parameters()  */

    lnc__write_upto8bits_elements(
        mtx_hdr,
        elt_p,
        0, 1);/* vcl_hrd_parameters_present_flag (1 bit) = 0 in Topaz */

    /* if( nal_hrd_parameters_present_flag  ||  vcl_hrd_parameters_present_flag )
     * FIX for BRN23039
     * low_delay_hrd_flag
     */
    lnc__write_upto8bits_elements(
        mtx_hdr, elt_p,
        0, 1);/* low_delay_hrd_flag */


    lnc__write_upto8bits_elements(
        mtx_hdr, elt_p,
        0, 1);/* pic_struct_present_flag (1 bit) = 0 in Topaz */

    lnc__write_upto8bits_elements(
        mtx_hdr, elt_p,
        0, 1);/* bitstream_restriction_flag (1 bit) = 0 in Topaz */
}

static void lnc__H264_writebits_sequence_header2(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p,
    H264_SEQUENCE_HEADER_PARAMS *pSHParams)
{
    /* GENERATES THE THIRD ELEMENT OF THE H264_SEQUENCE_HEADER() STRUCTURE
     * ELEMENT BITCOUNT: xx
     */
    IMG_UINT8 SBP;
    lnc__write_upto8bits_elements(
        mtx_hdr, elt_p,
        (pSHParams->VUI_Params_Present),/* vui_parameters_present_flag (VUI only in 1st sequence of stream) */
        1);

    if (pSHParams->VUI_Params_Present > 0)
        lnc__H264_writebits_VUI_params(mtx_hdr, elt_p, &(pSHParams->VUI_Params));

    /* Finally we need to align to the next byte
     * We know the size of the data in the sequence header (no MTX variables)
     * and start is byte aligned, so it's possible to add this field here rather than
     * MTX ELEMENT_INSERTBYTEALIGN_H264 command.
     */

    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);
    SBP = (elt_p[mtx_hdr->Elements]->Size) & 7;

    if (SBP > 0) lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 8 - (SBP));
}

static void lnc__H264_writebits_picture_header0(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p)
{
    /* GENERATES THE FIRST (STATIC) ELEMENT OF THE H264_PICTURE_HEADER() STRUCTURE
     * ELEMENT BITCOUNT: 18
     */

    /* 4 Byte StartCodePrefix Pregenerated in: lnc__H264_writebits_startcode_prefix_element()
     * Byte aligned (bit 32)
     */
    lnc__write_upto8bits_elements(
        mtx_hdr,
        elt_p, (0 << 7) | /* forbidden_zero_bit */
        (1 << 5) | /* nal_ref_idc (2 bits) = 1 */
        (8),/* nal_unit_tpye (5 bits) = 8 */
        8);

    /* Byte aligned (bit 40) */
    lnc__write_upto8bits_elements(
        mtx_hdr,
        elt_p,
        (1 << 7) | /* pic_parameter_set_id ue(v) = 0 in Topaz */
        (1 << 6) | /* seq_parameter_set_id ue(v) = 0 in Topaz */
        (0 << 5) | /* entropy_coding_mode_flag (1 bit) 0 for CAVLC */
        (0 << 4) | /* pic_order_present_flag (1 bit) = 0 */
        (1 << 3) | /* num_slice_group_minus1 ue(v) = 0 in Topaz */
        (1 << 2) | /* num_ref_idx_l0_active_minus1 ue(v) = 0 in Topaz */
        (1 << 1) | /* num_ref_idx_l1_active_minus1 ue(v) = 0 in Topaz */
        (0), /* weighted_pred_flag (1 bit) = 0 in Topaz  */
        8);

    /* Byte aligned (bit 48) */
    lnc__write_upto8bits_elements(
        mtx_hdr,
        elt_p, 0,
        2);/* weighted_bipred_flag (2 bits) = 0 in Topaz */
}

static void lnc__H264_writebits_picture_header1(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p)
{
    /* GENERATES THE SECOND ELEMENT OF THE H264_PICTURE_HEADER() STRUCTURE
     * ELEMENT BITCOUNT: 5
     */

    /* The following field will be generated as a special case by MTX - so not here
     * lnc__generate_se(mtx_hdr, pPHParams->pic_init_qp_minus26); // pic_int_qp_minus26 se(v) = -26 to 25 in Topaz
     */
    lnc__generate_se(mtx_hdr, elt_p, 0); /* pic_int_qs_minus26 se(v) = 0 in Topaz */
    lnc__generate_se(mtx_hdr, elt_p, 0); /* chroma_qp_index_offset se(v) = 0 in Topaz */

    lnc__write_upto8bits_elements(
        mtx_hdr,
        elt_p,
        (1 << 2) | /* deblocking_filter_control_present_flag (1 bit) = 1 in Topaz */
        (0 << 1) | /* constrained_intra_pred_Flag (1 bit) = 0 in Topaz */
        (0), /* redundant_pic_cnt_present_flag (1 bit) = 0 in Topaz */
        3);

    /* Byte align is done using an element command (MTX elements in this structure, we don't know it's size) */
}


static void lnc__H264_writebits_slice_header0(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p,
    H264_SLICE_HEADER_PARAMS *pSlHParams)
{
    /* GENERATES THE FIRST ELEMENT OF THE H264_SLICE_HEADER() STRUCTURE
     * ELEMENT BITCOUNT: 8
     *
     * StartCodePrefix Pregenerated in: Build_H264_4Byte_StartCodePrefix_Element() (4 or 3 bytes)
     * (3 bytes when slice is first in a picture without sequence/picture_header before picture
     * Byte aligned (bit 32 or 24)
     * NOTE: Slice_Type and Frame_Type are always the same, hence SliceFrame_Type
     */
    lnc__write_upto8bits_elements(
        mtx_hdr, elt_p,
        (0 << 7) |/* forbidden_zero_bit */
        ((pSlHParams->SliceFrame_Type == SLHP_B_SLICEFRAME_TYPE ? 0 : 1) << 5) | /* nal_ref_idc (2 bits) = 0 for B-frame and 1 for I or P-frame */
        ((pSlHParams->SliceFrame_Type == SLHP_IDR_SLICEFRAME_TYPE ? 5 : 1)),/* nal_unit_tpye (5 bits) = I-frame IDR, and 1 for  rest */
        8);
}


static void lnc__H264_writebits_slice_header1(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p,
    H264_SLICE_HEADER_PARAMS *pSlHParams,
    IMG_UINT16 uiIdrPicId)
{
    /* GENERATES THE SECOND ELEMENT OF THE H264_SLICE_HEADER() STRUCTURE
     * The following is slice parameter set in BP/MP
     */

    /* first_mb_in_slice = First MB address in slice: ue(Range 0 -  1619) */
    lnc__generate_ue(mtx_hdr, elt_p, (IMG_UINT32) pSlHParams->First_MB_Address);

    lnc__generate_ue(
        mtx_hdr, elt_p,
        (IMG_UINT32)((pSlHParams->SliceFrame_Type == SLHP_IDR_SLICEFRAME_TYPE) ? SLHP_I_SLICEFRAME_TYPE : pSlHParams->SliceFrame_Type));  /* slice_type ue(v): 0 for P-slice, 1 for B-slice, 2 for I-slice */

    /* kab: not clean change from IDR to intra, IDR should have separate flag */

    lnc__write_upto8bits_elements(
        mtx_hdr, elt_p,
        (1 << 5) | /* pic_parameter_set_id, ue(v) = 0  (=1b) in Topaz */
        pSlHParams->Frame_Num_DO,/* frame_num (5 bits) = frame nuo. in decoding order */
        6);

    /* frame_mb_only_flag is always 1, so no need for field_pic_flag or bottom_field_flag */
    if (pSlHParams->SliceFrame_Type == SLHP_IDR_SLICEFRAME_TYPE)
        lnc__generate_ue(mtx_hdr, elt_p, uiIdrPicId);

    /* kab: Idr_pic_id only for IDR, not nessesarely for all I pictures */

    /* if (pic_order_cnt_type == 0) //Note: (pic_order_cnt_type always 0 in Topaz) */
    lnc__write_upto8bits_elements(
        mtx_hdr, elt_p,
        pSlHParams->Picture_Num_DO,
        6); /* pic_order_cnt_lsb (6 bits) - picture no in display order */

    if (pSlHParams->SliceFrame_Type == SLHP_B_SLICEFRAME_TYPE)
        lnc__write_upto8bits_elements(
            mtx_hdr,
            elt_p,
            0,
            1);/* direct_spatial_mv_pred_flag (1 bit) = 0, spatial direct mode not supported in Topaz */

    if (pSlHParams->SliceFrame_Type == SLHP_P_SLICEFRAME_TYPE || pSlHParams->SliceFrame_Type == SLHP_B_SLICEFRAME_TYPE)
        lnc__write_upto8bits_elements(
            mtx_hdr,
            elt_p,
            0,
            1);/* num_ref_idx_active_override_flag (1 bit) = 0 in Topaz */

    if (pSlHParams->SliceFrame_Type != SLHP_I_SLICEFRAME_TYPE &&
        pSlHParams->SliceFrame_Type != SLHP_IDR_SLICEFRAME_TYPE) {
        if (pSlHParams->bUsesLongTermRef) {
            /* ref_pic_list_ordering_flag_IO (1 bit) = 1, L0
             * reference picture ordering */
            lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);

            /* modification_of_pic_nums_idc = 2 */
            lnc__generate_ue(mtx_hdr, elt_p, 2);
            /* long_term_pic_num = 0 */
            lnc__generate_ue(mtx_hdr, elt_p, 0);
            /* modification_of_pic_nums_idc = 3 */
            lnc__generate_ue(mtx_hdr, elt_p, 3);
        } else {
            /* ref_pic_list_ordering_flag_IO (1 bit) = 0, no
             * L0 reference picture ordering */
            lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);
        }
    }

    if (pSlHParams->SliceFrame_Type == SLHP_B_SLICEFRAME_TYPE)
        lnc__write_upto8bits_elements(
            mtx_hdr,
            elt_p,
            0,
            1); /* ref_pic_list_ordering_flag_I1 (1 bit) = 0, no L1 reference picture ordering in Topaz */

    if (pSlHParams->SliceFrame_Type == SLHP_IDR_SLICEFRAME_TYPE) {
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);/* no_output_of_prior_pics_flag (1 bit) = 0 */
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, pSlHParams->bIsLongTermRef ? 1 : 0, 1);/* long_term_reference_flag (1 bit) */
    } else if (pSlHParams->bIsLongTermRef) {
        /* adaptive_ref_pic_marking_mode_flag (1 bit) = 1 */
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);
        /* allow a single long-term reference */
        /* memory_management_control_operation */
        lnc__generate_ue(mtx_hdr, elt_p, 4);
        /* max_long_term_frame_idx_plus1 */
        lnc__generate_ue(mtx_hdr, elt_p, 1);
        /* set current picture as the long-term reference */
        /* memory_management_control_operation */
        lnc__generate_ue(mtx_hdr, elt_p, 6);
        /* long_term_frame_idx */
        lnc__generate_ue(mtx_hdr, elt_p, 0);
        /* END */
        lnc__generate_ue(mtx_hdr, elt_p, 0);
    } else {
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);/* adaptive_ref_pic_marking_mode_flag (1 bit) = 0 */
    }
}


static void lnc__H264_writebits_slice_header2(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p,
    H264_SLICE_HEADER_PARAMS *pSlHParams)
{
    /* GENERATES ELEMENT OF THE H264_SLICE_HEADER() STRUCTURE
     * ELEMENT BITCOUNT: 11
     */

#if 0
    /* Next field is generated on MTX with a special commnad (not ELEMENT_RAW) - so not defined here */

    /* slice_qp_delta se(v) = SliceQPy - (pic_init_qp_minus26+26) */
    /* pucHS=lnc__generate_se(pucHS, puiBitPos, pSlHParams->Slice_QP_Delta);  */
#endif

    lnc__generate_ue(
        mtx_hdr,
        elt_p,
        pSlHParams->Disable_Deblocking_Filter_Idc); /* disable_deblocking_filter_idc ue(v) = 2?  */

    if (pSlHParams->Disable_Deblocking_Filter_Idc != 1) {
        lnc__generate_se(mtx_hdr, elt_p, 0); /* slice_alpha_c0_offset_div2 se(v) = 0 (1b) in Topaz */
        lnc__generate_se(mtx_hdr, elt_p, 0); /* slice_beta_offset_div2 se(v) = 0 (1b) in Topaz */
    }

    /* num_slice_groups_minus1 ==0 in Topaz, so no slice_group_change_cycle field here
     * no byte alignment at end of slice headers
     */
}



static void lnc__H264_writebits_sequence_header(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p,
    H264_SEQUENCE_HEADER_PARAMS *pSHParams,
    H264_CROP_PARAMS *psCropParams)
{
    lnc__insert_element_token(mtx_hdr, elt_p, ELEMENT_STARTCODE_RAWDATA);

    lnc__H264_writebits_startcode_prefix_element(mtx_hdr, elt_p, 4);
    lnc__H264_writebits_sequence_header0(mtx_hdr, elt_p, pSHParams);
    lnc__H264_writebits_sequence_header1(mtx_hdr, elt_p, pSHParams, psCropParams);
    lnc__H264_writebits_sequence_header2(mtx_hdr, elt_p, pSHParams);
}


static void lnc__H264_writebits_picture_header(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p)
{
    /* Begin building the picture header element */
#ifdef USESTATICWHEREPOSSIBLE
    IMG_UINT32 *p;
    p = (IMG_UINT32 *) mtx_hdr;
    p[0] = 3;
    p[1] = 0;
    p[2] = 50;
    p[3] = 13510657;
    p[4] = 2;
    p[5] = 1;
    p[6] = 57349;
    p[7] = 6;
#else
    lnc__insert_element_token(mtx_hdr, elt_p, ELEMENT_STARTCODE_RAWDATA);

    lnc__H264_writebits_startcode_prefix_element(mtx_hdr, elt_p, 4);
    lnc__H264_writebits_picture_header0(mtx_hdr, elt_p);

    lnc__insert_element_token(mtx_hdr, elt_p, ELEMENT_QP); /* MTX fills this value in */

    lnc__insert_element_token(mtx_hdr, elt_p, ELEMENT_RAWDATA);
    lnc__H264_writebits_picture_header1(mtx_hdr, elt_p);

    /* Tell MTX to insert the byte align field */
    /* (we don't know final stream size for alignment at this point) */
    lnc__insert_element_token(mtx_hdr, elt_p, ELEMENT_INSERTBYTEALIGN_H264);
#endif

}


static void lnc__H264_writebits_slice_header(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p,
    H264_SLICE_HEADER_PARAMS *pSlHParams,
    IMG_UINT16 uiIdrPicId)
{
#ifdef USESTATICWHEREPOSSIBLE
    IMG_UINT32 *p;
    p = (IMG_UINT32 *) mtx_hdr;

    p[0] = p[1] = 0;
    p[2] = 40;
    if (pSlHParams->SliceFrame_Type == SLHP_B_SLICEFRAME_TYPE) p[3] = 257;
    else if (pSlHParams->SliceFrame_Type == SLHP_IDR_SLICEFRAME_TYPE) p[3] = 9473;
    else p[3] = 8449;
#else
    /* -- Begin building the picture header element */
    lnc__insert_element_token(mtx_hdr, elt_p, ELEMENT_STARTCODE_RAWDATA);

    lnc__H264_writebits_startcode_prefix_element(
        mtx_hdr,
        elt_p,
        pSlHParams->Start_Code_Prefix_Size_Bytes); /* Can be 3 or 4 bytes - always 4 bytes in our implementations */
    lnc__H264_writebits_slice_header0(mtx_hdr, elt_p, pSlHParams);
#endif

    lnc__H264_writebits_slice_header1(mtx_hdr, elt_p, pSlHParams, uiIdrPicId);
    lnc__insert_element_token(mtx_hdr, elt_p, ELEMENT_SQP); //MTX fills this value in

    lnc__insert_element_token(mtx_hdr, elt_p, ELEMENT_RAWDATA);
    lnc__H264_writebits_slice_header2(mtx_hdr, elt_p, pSlHParams);

    /* no byte alignment at end of slice headers */
}


static void lnc__H264_writebits_endofsequence_header(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p)
{
    /* GENERATES THE FIRST ELEMENT OF THE H264_ENDOFSEQUENCE_HEADER() STRUCTURE */
    lnc__insert_element_token(mtx_hdr, elt_p, ELEMENT_RAWDATA);

    /* Byte aligned (bit 32) */
    lnc__write_upto8bits_elements(
        mtx_hdr,
        elt_p,
        (0 << 7) |/* forbidden_zero_bit=0 */
        (0 << 5) |/* nal_ref_idc=0 for nal_unit_type=10 */
        (10),/* nal_unit_type=10 */
        8);
}


static void lnc__H264_writebits_SEI_rbspheader(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p,
    IMG_UINT32 Initial_cpb_removal_delay)
{
    /*  Byte aligned (bit 32) */
    lnc__H264_writebits_startcode_prefix_element(
        mtx_hdr, elt_p, 4);/* 32 bit start code prefix    */

    lnc__write_upto8bits_elements(
        mtx_hdr,
        elt_p,
        (0 << 7) |/* forbidden_zero_bit */
        (1 << 5) |/* nal_ref_idc (2 bits) = 1 */
        (6),/* nal_unit_tpye (5 bits) = 6 (SEI packet) */
        8);

    /* last_payload_type_byte (8 bits) = 0 for buffering period     */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 8);

    /* last_payload_size_byte (8 bits) = 41 as SEI_message length is 41-bit */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 41, 8);

    /* sequence_parameter_set_id        ue(0) = 1b sequence_parameter_set_id=0 in Topaz */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);

    /* Initial_cpb_removal_delay (20 bits)      x is initial cpb delay of each sequence     */
    lnc__write_upto32bits_elements(mtx_hdr, elt_p, Initial_cpb_removal_delay, 20);
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 8);
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 8);

    /* Initial_cpb_removal_delay_offset (20 bits) 0x10101 (It won't be used in Topaz) */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 5, 4);

    /* Tell MTX to insert the byte align field (we don't know final stream size for alignment at this point) */
    lnc__insert_element_token(mtx_hdr, elt_p, ELEMENT_INSERTBYTEALIGN_H264);
}


static void lnc__H264_writebits_endofstream_header(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p)
{
    /* GENERATES THE FIRST ELEMENT OF THE H264_ENDOFSTREAM_HEADER() STRUCTURE */
    lnc__insert_element_token(mtx_hdr, elt_p, ELEMENT_RAWDATA);
    /* Byte aligned (bit 32) */
    lnc__write_upto8bits_elements(
        mtx_hdr,
        elt_p,
        (0 << 7) |/* forbidden_zero_bit=0 */
        (0 << 5) |/* nal_ref_idc=0 for nal_unit_type=11 */
        (11),/* nal_unit_type=11 */
        8);
}

/*
 * High level functions to call when a H264 header is required
 */
static void lnc__H264_getelements_skip_B_slice(
    MTX_HEADER_PARAMS *mtx_hdr,
    H264_SLICE_HEADER_PARAMS *pSlHParams,
    IMG_UINT8 MB_No_In_Slice)
{
    /* Skipped P-Slice
     * Ensure pSlHParams is filled with appropriate parameters for a P-slice
     * Essential we initialise our header structures before building
     */
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *elt_p[MAXNUMBERELEMENTS];
    mtx_hdr->Elements = ELEMENTS_EMPTY;
    This_Element = (MTX_HEADER_ELEMENT *) mtx_hdr->asElementStream;
    elt_p[0] = This_Element;

    /* Not sure if this will be required in the final spec
     * lnc__insert_element_token(mtx_hdr, ELEMENT_STARTCOUNTER);
     */
    lnc__H264_writebits_slice_header(mtx_hdr, elt_p, pSlHParams, 0);

    /* mb_skip_run = mb_no_in_slice */
    lnc__generate_ue(mtx_hdr, elt_p, MB_No_In_Slice);

    /* Tell MTX to insert the byte align field
     * (we don't know final stream size for alignment at this point)
     */
    lnc__insert_element_token(mtx_hdr, elt_p, ELEMENT_INSERTBYTEALIGN_H264);

    /* Has been used as an index, so need to add 1 for a valid element count */
    mtx_hdr->Elements++;
}

static void lnc__H264_getelements_backward_zero_B_slice(
    MTX_HEADER_PARAMS *mtx_hdr,
    IMG_UINT8 MB_No_In_Slice)
{
    /* Skipped P-Slice
     * Ensure pSlHParams is filled with appropriate parameters for a P-slice
     * Essential we initialise our header structures before building
     */
    IMG_UINT8 Lp;
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *elt_p[MAXNUMBERELEMENTS];
    mtx_hdr->Elements = ELEMENTS_EMPTY;
    This_Element = (MTX_HEADER_ELEMENT *) mtx_hdr->asElementStream;
    elt_p[0] = This_Element;

    for (Lp = 0; Lp < MB_No_In_Slice; Lp++) {
        /* mb_skip_run = ue(0) = 1b */
        lnc__write_upto8bits_elements(mtx_hdr, elt_p,   1, 1);
        /* backward_zero_B_mb() - all static         */
        lnc__write_upto8bits_elements(mtx_hdr, elt_p,   15, 5);
    }

    /* Tell MTX to insert the byte align field
     * (we don't know final stream size for alignment at this point)
     */
    lnc__insert_element_token(
        mtx_hdr,
        elt_p,
        ELEMENT_INSERTBYTEALIGN_H264);

    /* Has been used as an index, so need to add 1 for a valid element count */
    mtx_hdr->Elements++;
}


static void lnc__H264_getelements_rbsp_ATE_only(MTX_HEADER_PARAMS *mtx_hdr)
{
    /* Skipped P-Slice
     * Ensure pSlHParams is filled with appropriate parameters for a P-slice
     * Essential we initialise our header structures before building
     */
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *elt_p[MAXNUMBERELEMENTS];
    mtx_hdr->Elements = ELEMENTS_EMPTY;
    This_Element = (MTX_HEADER_ELEMENT *) mtx_hdr->asElementStream;
    elt_p[0] = This_Element;

    /* Tell MTX to insert the byte align field
     * (we don't know final stream size for alignment at this point)
     */
    lnc__insert_element_token(
        mtx_hdr,
        elt_p,
        ELEMENT_INSERTBYTEALIGN_H264);

    /* Has been used as an index, so need to add 1 for a valid element count */
    mtx_hdr->Elements++;
}


static void lnc__H264_getelements_skip_P_slice(
    MTX_HEADER_PARAMS *mtx_hdr,
    H264_SLICE_HEADER_PARAMS *pSlHParams,
    IMG_UINT32 MB_No_In_Slice)
{
    /* Skipped P-Slice
     * Ensure pSlHParams is filled with appropriate parameters for a B-slice
     * Essential we initialise our header structures before building
     */
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *elt_p[MAXNUMBERELEMENTS];
    mtx_hdr->Elements = ELEMENTS_EMPTY;
    This_Element = (MTX_HEADER_ELEMENT *) mtx_hdr->asElementStream;
    elt_p[0] = This_Element;

    /* lnc__insert_element_token(mtx_hdr, ELEMENT_STARTCOUNTER); */
    /* Not sure if this will be required in the final spec */
    lnc__H264_writebits_slice_header(mtx_hdr, elt_p, pSlHParams, 0);
    lnc__generate_ue(mtx_hdr, elt_p, MB_No_In_Slice); /* mb_skip_run = mb_no_in_slice */

    /* Tell MTX to insert the byte align field (we don't know final stream size for alignment at this point) */
    lnc__insert_element_token(mtx_hdr, elt_p, ELEMENT_INSERTBYTEALIGN_H264);
    mtx_hdr->Elements++; /* Has been used as an index, so need to add 1 for a valid element count */
}



static void lnc__H264_getelements_sequence_header(
    MTX_HEADER_PARAMS *mtx_hdr,
    H264_SEQUENCE_HEADER_PARAMS *pSHParams,
    H264_CROP_PARAMS *psCropParams)
{
    /* Builds a sequence, picture and slice header with from the given inputs parameters (start of new frame)
     * Essential we initialise our header structures before building
     */
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *elt_p[MAXNUMBERELEMENTS];
    mtx_hdr->Elements = ELEMENTS_EMPTY;
    This_Element = (MTX_HEADER_ELEMENT *) mtx_hdr->asElementStream;
    elt_p[0] = This_Element;

    lnc__H264_writebits_sequence_header(mtx_hdr, elt_p, pSHParams, psCropParams);
    mtx_hdr->Elements++; /* Has been used as an index, so need to add 1 for a valid element count */
}


static void lnc__H264_getelements_picture_header(MTX_HEADER_PARAMS *mtx_hdr)
{
    /* Builds a sequence, picture and slice header with from the given inputs parameters (start of new frame)
     * Essential we initialise our header structures before building
     */
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *elt_p[MAXNUMBERELEMENTS];
    mtx_hdr->Elements = ELEMENTS_EMPTY;
    This_Element = (MTX_HEADER_ELEMENT *) mtx_hdr->asElementStream;
    elt_p[0] = This_Element;

    lnc__H264_writebits_picture_header(mtx_hdr, elt_p);
    mtx_hdr->Elements++; //Has been used as an index, so need to add 1 for a valid element count
}


static void lnc__H264_getelements_slice_header(
    MTX_HEADER_PARAMS *mtx_hdr,
    H264_SLICE_HEADER_PARAMS *pSlHParams,
    IMG_UINT16 uiIdrPicId)
{
    /* Builds a single slice header from the given parameters (mid frame)
     * Essential we initialise our header structures before building
     */
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *elt_p[MAXNUMBERELEMENTS];
    mtx_hdr->Elements = ELEMENTS_EMPTY;
    This_Element = (MTX_HEADER_ELEMENT *) mtx_hdr->asElementStream;
    elt_p[0] = This_Element;

    /* Not sure if this will be required in the final spec */
    /* lnc__insert_element_token(mtx_hdr, ELEMENT_STARTCOUNTER);*/
    lnc__H264_writebits_slice_header(mtx_hdr, elt_p, pSlHParams, uiIdrPicId);

    mtx_hdr->Elements++; /* Has been used as an index, so need to add 1 for a valid element count */
}


static void lnc__H264_getelements_endofsequence_header(
    MTX_HEADER_PARAMS *mtx_hdr)
{
    /* Builds a single endofsequence header from the given parameters (mid frame) */

    /* Essential we initialise our header structures before building */
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *elt_p[MAXNUMBERELEMENTS];
    mtx_hdr->Elements = ELEMENTS_EMPTY;
    This_Element = (MTX_HEADER_ELEMENT *) mtx_hdr->asElementStream;
    elt_p[0] = This_Element;

    lnc__H264_writebits_endofsequence_header(mtx_hdr, elt_p);
    mtx_hdr->Elements++; /* Has been used as an index, so need to add 1 for a valid element count */
}



static void lnc__H264_getelements_endofstream_header(MTX_HEADER_PARAMS *mtx_hdr)
{
    /* Builds a single endofstream header from the given parameters (mid frame) */

    /* Essential we initialise our header structures before building */
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *elt_p[MAXNUMBERELEMENTS];
    mtx_hdr->Elements = ELEMENTS_EMPTY;
    This_Element = (MTX_HEADER_ELEMENT *) mtx_hdr->asElementStream;
    elt_p[0] = This_Element;

    lnc__H264_writebits_endofstream_header(mtx_hdr, elt_p);
    mtx_hdr->Elements++; /* Has been used as an index, so need to add 1 for a valid element count */
}

static IMG_UINT8 Bits2Code(IMG_UINT32 CodeVal)
{
    IMG_UINT8 Bits = 32;
    if (CodeVal == 0)
        return 1;
    while (!(CodeVal & 0x80000000)) {
        CodeVal <<= 1;
        Bits--;
    }
    return Bits;
}

/*
 * Intermediary functions to build MPEG4 headers
 */
#define MATCH_TO_ENC


static void lnc__MPEG4_writebits_sequence_header(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p,
    IMG_BOOL bBFrame,
    MPEG4_PROFILE_TYPE bProfile,
    IMG_UINT8 Profile_and_level_indication,
    FIXED_VOP_TIME_TYPE sFixed_vop_time_increment,
    IMG_UINT32 Picture_Width_Pixels,
    IMG_UINT32 Picture_Height_Pixels,
    VBVPARAMS *sVBVParams, IMG_UINT32 VopTimeResolution) /* Send NULL pointer if there are no VBVParams */
{
    /* Essential we insert the element before we try to fill it! */
    lnc__insert_element_token(mtx_hdr, elt_p, ELEMENT_STARTCODE_RAWDATA);

    /* visual_object_sequence_start_code        = 32 Bits       = 0x1B0 */
    lnc__write_upto32bits_elements(mtx_hdr, elt_p, 432, 32);

    /* profile_and_level_indication = 8 Bits = SP L0-L3 and SP L4-L5 are supported */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, Profile_and_level_indication, 8);

    /* visual_object_start_code = 32 Bits       = 0x1B5 */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 8);
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 8);
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 8);
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 181, 8);

    /* is_visual_object_identifier = 1 Bit      = 0 */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

    /* visual_object_type = 4 Bits      = Video ID = 1 */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 4);

    /* video_signal_type = 1 Bit                = 1 */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

    /* byte_aligned_bits = 2 Bits       = 01b (byte_aligned_bits is 2-bit stuffing bit field 01) */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 2);

    /* video_object_start_code = 32 Bits        = 0x100 One VO only in a Topaz video stream */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 8);
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 8);
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 8);
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 8);

    /* video_object_layer_start_code = 32 Bits  = 0x120 One VOL only in a Topaz stream */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 8);
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 8);
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 8);
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 32, 8);

    /* random_accessible_vol = 1 Bit            = 0 (P-Frame in GOP) */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

    if (bProfile == SP) {
        /* video_object_type_indication = 8 Bits        = 0x01 for SP */
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 8);
#ifndef MATCH_TO_ENC
        /* is_object_layer_identifier   = 1 Bit         = 0 for SP */
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);
#else
        /* to match the encoder */

        /* is_object_layer_identifier   = 1 Bit          */
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);

        /* video_object_layer_verid     = 4 Bits         */
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 4);

        /* video_object_layer_priority  = 3 Bits         */
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 3);  // 0 is reserved...
#endif
    } else {
        /* video_object_type_indication = 8 Bits        = 0x11 for ASP */
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 3, 8);

        /* is_object_layer_identifier   = 1 Bit         = 1 for ASP */
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);

        /* video_object_layer_verid     = 4 Bits        = 5 is for ASP */
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 5, 4);

        /* video_object_layer_priority  = 3 Bits        = 1 (Highest priority) */
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 3);
    }

    /* aspect_ratio_info                = 4 Bits        =0x1 (Square pixel) */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 4);
#ifndef MATCH_TO_ENC
    /* vol_control_parameters           = 1 Bit         = 1 (Always send VOL control parameters) */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);
#else
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

#endif

#ifndef MATCH_TO_ENC
    /* chroma_format                    = 2 Bits        = 01b (4:2:0) */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 2);

    /* low_delay = 1 Bit = 0 with B-frame and 1 without B-frame */
    if (bBFrame)
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);
    else
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);

    /* vbv_parameters                   = 1 Bit                 =0/1  */
    if (sVBVParams) {
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);

        /* For recording, only send vbv parameters in 1st sequence header.
         * For video phone, it should be sent more often, such as once per sequence
         */
        lnc__write_upto32bits_elements(
            mtx_hdr,
            elt_p,
            sVBVParams->First_half_bit_rate,
            15); /* first_half_bit_rate */

        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);/* Marker Bit = 1 */
        lnc__write_upto32bits_elements(
            mtx_hdr,
            elt_p,
            sVBVParams->Latter_half_bit_rate,
            15); /* latter_half_bit_rate */

        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);/* Marker Bit = 1 */
        lnc__write_upto32bits_elements(
            mtx_hdr,
            elt_p,
            sVBVParams->First_half_vbv_buffer_size,
            15);/* first_half_vbv_buffer_size */

        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1); /* Marker Bit = 1 */
        lnc__write_upto32bits_elements(
            mtx_hdr, elt_p,
            sVBVParams->Latter_half_vbv_buffer_size,
            15); /* latter_half_vbv_buffer_size */
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);/* Marker Bit = 1 */
        lnc__write_upto32bits_elements(
            mtx_hdr,
            elt_p,
            sVBVParams->First_half_vbv_occupancy,
            15);  /* first_half_vbv_occupancy */
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);/* Marker Bit = 1 */
        lnc__write_upto32bits_elements(
            mtx_hdr,
            elt_p,
            sVBVParams->Latter_half_vbv_occupancy,
            15); /* latter_half_vbv_occupancy */
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);/* Marker Bit = 1 */
    } else
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1); /* No vbv parameters present */
#endif
    /* video_object_layer_shape = 2 Bits =      00b     Rectangular shape */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 2);
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1); /* Marker Bit = 1 */

    /* vop_time_increment_solution = 16 Bits */
    lnc__write_upto32bits_elements(mtx_hdr, elt_p, VopTimeResolution, 16);
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1); /* Marker Bit = 1 */

#ifndef MATCH_TO_ENC
    /* fixed_vop_rate = 1 Bits  = 1 Always fixed frame rate */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);

    /* fixed_vop_time_increment         = Variable number of bits based on the time increment resolution. */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, Bits2Code(VopTimeResolution - 1));
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1); /* Marker Bit = 1 */
#else
    /* fixed_vop_rate   = 1 Bits =      0 */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1); /* Marker Bit = 1 */

#endif
    /* video_object_layer_width = 13 Bits  Picture width in pixel units */
    lnc__write_upto32bits_elements(mtx_hdr, elt_p, Picture_Width_Pixels, 13);
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1); /* Marker Bit = 1 */

    /* video_object_layer_height = 13 Bits Picture height in pixel units */
    lnc__write_upto32bits_elements(mtx_hdr, elt_p, Picture_Height_Pixels, 13);
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1); /* Marker Bit = 1 */

    /* interlaced = 1 Bit = 0 Topaz only encodes progressive frames */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

    /* obmc_disable = 1 Bit = 1 No overlapped MC in Topaz */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);

    /* sprite_enable = 1 Bit = 0 Not use sprite in Topaz */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

    /* not_8_bit = 1    Bit = 0 8-bit video in Topaz */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

    /* quant_type = 1 Bit = 0 2nd quantization method in Topaz */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

    if (bProfile == ASP) {
        /* quarter_sample = 1 Bit = 0 No ¼-pel MC in Topaz */
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);
    }

    /* complexity_estimation_disable    = 1 Bit = 1 No complexity estimation in Topaz */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);
#ifndef MATCH_TO_ENC

    /* resync_marker_disable = 1 Bit = 0 Always enable resync marker in Topaz */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);
#else
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);
#endif
    /* data_partitioned = 1 Bit = 0 No data partitioning in Topaz */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

    if (bProfile == ASP) {
        /* newpred_enable = 1 Bit = 0 No newpred mode in SP/ASP */
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);
        /* reduced_vop_resolution_enable=1 Bit = 0      No reduced resolution frame in SP/ASP */
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);
    }

    /* scalability = 1 Bit = 0  No scalability in SP/ASP */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

    /* byte_aligned_bits */

    /* Tell MTX to insert the byte align field
     * (we don't know final stream size for alignment at this point)
     */
    lnc__insert_element_token(
        mtx_hdr,
        elt_p,
        ELEMENT_INSERTBYTEALIGN_MPG4);

    return;
}


/* Utility function */
/*
  IMG_UINT8 Bits2Code(IMG_UINT32 CodeVal)
  {
  IMG_UINT8 Bits=32;
  if(CodeVal==0)
  return 1;
  while(!(CodeVal & 0x80000000))
  {
  CodeVal<<=1;
  Bits--;
  }
  return Bits;
  }
*/

/* MPEG 4 VOP (Picture) Header */
static void lnc__MPEG4_writebits_VOP_header(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p,
    IMG_BOOL    bIsVOP_coded,
    IMG_UINT8   VOP_time_increment,
    SEARCH_RANGE_TYPE sSearch_range,
    VOP_CODING_TYPE sVopCodingType,
    IMG_UINT32 VopTimeResolution)
{
    IMG_BOOL bIsSyncPoint;
    /* Essential we insert the element before we try to fill it! */
    lnc__insert_element_token(mtx_hdr, elt_p, ELEMENT_STARTCODE_RAWDATA);

    /* visual_object_sequence_start_code        = 32 Bits       = 0x1B6 */
    lnc__write_upto32bits_elements(mtx_hdr, elt_p, 438, 32);

    /* vop_coding_type  = 2 Bits = 0 for I-frame and 1 for P-frame */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, sVopCodingType, 2);
    bIsSyncPoint = (VOP_time_increment > 1) && ((VOP_time_increment) % VopTimeResolution == 0);

#ifndef MATCH_TO_ENC
    /* modulo_time_base = 1 Bit = 0 As at least  1 synchronization point (I-frame) per second in Topaz */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);
#else

    lnc__write_upto8bits_elements(mtx_hdr, elt_p, bIsSyncPoint  ? 2 : 0 , bIsSyncPoint ? 2 : 1);

#endif

    /* marker_bit = 1   Bits    = 1      */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);

#ifndef MATCH_TO_ENC
    /* vop_time_increment = Variable bits based on resolution
     *  = x Reset to 0 at I-frame and plus fixed_vop_time_increment each frame
     */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, VOP_time_increment, 5);
#else
    /* will chrash here... */
    lnc__write_upto8bits_elements(
        mtx_hdr, elt_p,
        (VOP_time_increment) % VopTimeResolution,
        Bits2Code(VopTimeResolution - 1));

#endif
    /* marker_bit = 1 Bit               = 1      */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);

    if (!bIsVOP_coded) {
        /* vop_coded    = 1 Bit         = 0 for skipped frame */
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

        /* byte_aligned_bits (skipped pictures are byte aligned) */
        /* Tell MTX to insert the byte align field (we don't know final stream size for alignment at this point)
         * End of VOP - skipped picture
         */
        lnc__insert_element_token(mtx_hdr, elt_p, ELEMENT_INSERTBYTEALIGN_MPG4);
    } else {
        /* vop_coded = 1 Bit            = 1 for normal coded frame */
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);

        if (sVopCodingType == P_FRAME) {
            /* vop_rounding_type = 1 Bit = 0 vop_rounding_type is 0 in Topaz */
            lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);
        }

        /* intra_dc_vlc_thr = 3 Bits = 0 Use intra DC VLC in Topaz */
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 3);

        /* vop_quant = 5 Bits   = x     5-bit frame Q_scale from rate control - GENERATED BY MTX */
        /* lnc__write_upto8bits_elements(mtx_hdr, elt_p, Frame_Q_scale, 5); */
        lnc__insert_element_token(mtx_hdr, elt_p, ELEMENT_FRAMEQSCALE);

        if (sVopCodingType == P_FRAME) {
            /* vop_fcode_forward = 3 bits       = 2 for +/-32 and 3 for +/-64 search range  */
            lnc__insert_element_token(mtx_hdr, elt_p, ELEMENT_RAWDATA);
            lnc__write_upto8bits_elements(mtx_hdr, elt_p, sSearch_range, 3);
        }

        /*
        **** THE FINAL PART OF VOP STRUCTURE CAN'T BE GENERATED HERE
        lnc__insert_element_token(mtx_hdr, elt_p, ELEMENT_RAWDATA);
        video_packet_data ( )                   = 1st  VP that doesn’t have the VP header

        while (nextbits_bytealigned ( ) == resync_marker)
        {
        video_packet _header( )
        video_packet _data( )                   All MB in the slice
        }
        */
    }
}


/* MPEG 4 Video Packet (Slice) Header */
static void lnc__MPEG4_writebits_videopacket_header(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p,
    VOP_CODING_TYPE eVop_Coding_Type,
    IMG_UINT8 Fcode,
    IMG_UINT32 MBNumber,
    IMG_UINT32 MBNumberlength,
    IMG_BOOL bHeader_Extension_Code,
    IMG_UINT8 VOP_Time_Increment,
    SEARCH_RANGE_TYPE sSearch_range)
{
    /* Essential we insert the element before we try to fill it! */
    lnc__insert_element_token(mtx_hdr, elt_p, ELEMENT_RAWDATA);

    if (eVop_Coding_Type == I_FRAME) {
        /* resync_marker        = 17 bit        =0x1    17-bit for I-frame */
        lnc__write_upto32bits_elements(mtx_hdr, elt_p, 1, 17);
    } else {
        /* resync_marker = 17 bit       =0x1    (16+fcode) bits for P-frame */
        lnc__write_upto32bits_elements(mtx_hdr, elt_p, 1, 16 + Fcode);
    }

    /* macroblock_number = 1-14 bits    = ?????? */
    lnc__write_upto32bits_elements(mtx_hdr, elt_p, MBNumber, MBNumberlength);

    /* quant_scale = 5 bits     =1-32   VP (Slice) Q_scale
     * lnc__write_upto8bits_elements(mtx_hdr, elt_p, VP_Slice_Q_Scale, 5);
     */
    lnc__insert_element_token(
        mtx_hdr, elt_p,
        ELEMENT_SLICEQSCALE); /* Insert token to tell MTX to insert rate-control value */

    lnc__insert_element_token(mtx_hdr, elt_p, ELEMENT_RAWDATA); /* Begin writing rawdata again */

    if (bHeader_Extension_Code) {
        /* header_extension_code = 1bit = 1     picture header parameters are repeated */
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

        /* modulo_time_base = 1 bit = 0 The same as it is in the current picture header */
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

        /* marker_bit = 1 bit   = 1 */
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);

        /* vop_time_increment = 5 bits  = 0-30  The same as it is in the current picture header */
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, VOP_Time_Increment, 5);

        /* marker_bit   = 1 bit         = 1      */
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);

        /* vop_coding_type= 2   bits    = 0/1 The same as it is in the current picture header */
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, eVop_Coding_Type, 2);

        /* intra_dc_vlc_thr = 3 bits    = 0 The same as it is in the current picture header */
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 3);

        if (eVop_Coding_Type == P_FRAME) {
            /* vop_fcode_forward = 3 bits = 2/3 The same as it is in the current picture header */
            lnc__write_upto8bits_elements(mtx_hdr, elt_p, sSearch_range, 3);
        }
    } else {
        /* header_extension_code = 1 bits =0    picture header parameters are NOT repeated */
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);
    }
}

/*
 * High level functions to call when a MPEG4 header is required - HOST ROUTINES
 */
static void lnc__MPEG4_getelements_sequence_header(
    MTX_HEADER_PARAMS *mtx_hdr,
    IMG_BOOL bBFrame,
    MPEG4_PROFILE_TYPE sProfile,
    IMG_UINT8 Profile_and_level_indication,
    FIXED_VOP_TIME_TYPE sFixed_vop_time_increment,
    IMG_UINT32 Picture_Width_Pixels,
    IMG_UINT32 Picture_Height_Pixels,
    VBVPARAMS *sVBVParams,
    IMG_UINT32 VopTimeResolution) /* NULL pointer if there are no VBVParams */
{
    /* Builds a single MPEG4 video sequence header from the given parameters */

    /* Essential we initialise our header structures before building */
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *elt_p[MAXNUMBERELEMENTS];
    mtx_hdr->Elements = ELEMENTS_EMPTY;
    This_Element = (MTX_HEADER_ELEMENT *) mtx_hdr->asElementStream;
    elt_p[0] = This_Element;

    lnc__MPEG4_writebits_sequence_header(
        mtx_hdr,
        elt_p,
        bBFrame, sProfile,
        Profile_and_level_indication,
        sFixed_vop_time_increment,
        Picture_Width_Pixels,
        Picture_Height_Pixels,
        sVBVParams, VopTimeResolution);

    mtx_hdr->Elements++; /* Has been used as an index, so need to add 1 for a valid element count */
}


/* MPEG 4 VOP (Picture) Header */
static void lnc__MPEG4_getelements_VOP_header(
    MTX_HEADER_PARAMS *mtx_hdr,
    IMG_BOOL    bIsVOP_coded,
    IMG_UINT8   VOP_time_increment,
    SEARCH_RANGE_TYPE sSearch_range,
    VOP_CODING_TYPE sVopCodingType,
    IMG_UINT32 VopTimeResolution)
{
    /* Builds a single MPEG4 VOP (picture) header from the given parameters */

    /* Essential we initialise our header structures before building */
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *elt_p[MAXNUMBERELEMENTS];
    mtx_hdr->Elements = ELEMENTS_EMPTY;
    This_Element = (MTX_HEADER_ELEMENT *) mtx_hdr->asElementStream;
    elt_p[0] = This_Element;

    /* Frame QScale no longer written here as it is inserted by MTX later
     * (add as parameter to MTX_Send_Elements_To_VLC)
     */
    lnc__MPEG4_writebits_VOP_header(
        mtx_hdr, elt_p, bIsVOP_coded,
        VOP_time_increment,
        sSearch_range,
        sVopCodingType, VopTimeResolution);

    mtx_hdr->Elements++; /* Has been used as an index, so need to add 1 for a valid element count */
}


static void lnc_MPEG4_getelements_video_packet_header(
    MTX_HEADER_PARAMS *mtx_hdr,
    VOP_CODING_TYPE eVop_Coding_Type,
    IMG_UINT8 Fcode,
    IMG_UINT32 MBNumber,
    IMG_UINT32 MBNumberlength,
    IMG_BOOL bHeader_Extension_Code,
    IMG_UINT8 VOP_Time_Increment,
    SEARCH_RANGE_TYPE sSearch_range)
{
    /* Builds a single MPEG4 video packet (slice) header from the given parameters */

    /* Essential we initialise our header structures before building */
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *elt_p[MAXNUMBERELEMENTS];
    mtx_hdr->Elements = ELEMENTS_EMPTY;
    This_Element = (MTX_HEADER_ELEMENT *) mtx_hdr->asElementStream;
    elt_p[0] = This_Element;

    /* Slice QScale no longer written here as it is inserted by MTX later
     * (add as parameter when sending to VLC)
     */
    lnc__MPEG4_writebits_videopacket_header(
        mtx_hdr, elt_p, eVop_Coding_Type,
        Fcode, MBNumber, MBNumberlength,
        bHeader_Extension_Code, VOP_Time_Increment, sSearch_range);

    mtx_hdr->Elements++; /* Has been used as an index, so need to add 1 for a valid element count */
}

/*
 * Intermediary functions to build H263 headers
 */
static void H263_writebits_VideoSequenceHeader(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p,
    IMG_UINT8 Profile_and_level_indication)
{
    /* Essential we insert the element before we try to fill it! */
    lnc__insert_element_token(mtx_hdr, elt_p, ELEMENT_STARTCODE_RAWDATA);

    /* visual_object_sequence_start_code        = 32 Bits       = 0x1B0 */
    lnc__write_upto32bits_elements(mtx_hdr, elt_p, 432, 32);

    /* profile_and_level_indication = 8 Bits =  x SP L0-L3 and SP L4-L5 are supported */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, Profile_and_level_indication, 8);

    /* visual_object_start_code = 32 Bits       = 0x1B5 */

    /* 437 too large for the   lnc__write_upto32bits_elements function */
    lnc__write_upto32bits_elements(mtx_hdr, elt_p, 437, 32);

    /* is_visual_object_identifier = 1 Bit              = 0 */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

    /* is_visual_object_type    = 4 Bits        = 1 Video ID */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 4);

    /* video_signal_type = 1 Bit                = 0      */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

    /* byte_aligned_bits = 2 Bits = 01b byte_aligned_bits is 2-bit stuffing bit field 01 */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 2);

    /* video_object_start_code  =32 Bits        = 0x100 One VO only in a Topaz video stream */
    lnc__write_upto32bits_elements(mtx_hdr, elt_p, 256, 32);

    return;
}


static void H263_writebits_VideoPictureHeader(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p,
    IMG_UINT8 Temporal_Ref,
    H263_PICTURE_CODING_TYPE PictureCodingType,
    //IMG_UINT8 Q_Scale,
    H263_SOURCE_FORMAT_TYPE SourceFormatType,
    IMG_UINT8 FrameRate,
    IMG_UINT16 PictureWidth,
    IMG_UINT16 PictureHeight,
    IMG_UINT8 *OptionalCustomPCF)
{
    IMG_UINT8 UFEP;

#ifdef USESTATICWHEREPOSSIBLE
    IMG_UINT16 *p;

    p = (IMG_UINT16 *) mtx_hdr;
    p[0] = p[1] = p[2] = p[3] = 0;
    p[4] = 38;
    p[5] = 32768 | ((Temporal_Ref >> 6) << 8);
    p[6] = ((Temporal_Ref & 63)  << 2) | 2 | (SourceFormatType << 10);
#else

    /* Essential we insert the element before we try to fill it! */
    lnc__insert_element_token(mtx_hdr, elt_p, ELEMENT_STARTCODE_RAWDATA);

    /* short_video_start_marker = 22 Bits       = 0x20 Picture start code */
    lnc__write_upto32bits_elements(mtx_hdr, elt_p, 32, 22);

    /* temporal_reference = 8 Bits      = 0-255 Each picture increased by 1 */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, Temporal_Ref, 8);

    /* marker_bit = 1 Bit = 1    */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);

    /* zero_bit = 1 Bits        = 0      */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

    /* split_screen_indicator   = 1     Bits    = 0     No direct effect on encoding of picture */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

    /* document_camera_indicator= 1     Bits    = 0     No direct effect on encoding of picture */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

    /* full_picture_freeze_release=1 Bits       = 0     No direct effect on encoding of picture */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

    /* source_format                            = 3     Bits    = 1-4   See note */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, SourceFormatType, 3);
#endif


    /*Write optional Custom Picture Clock Frequency(OCPCF)*/
    if (FrameRate == 30 || FrameRate == 0/* unspecified */) {
        *OptionalCustomPCF = 0; // 0 for CIF PCF
    } else {
        *OptionalCustomPCF = 1; //1 for Custom PCF
    }


    if (SourceFormatType != 7) {
        /* picture_coding_type          = 1 Bit         = 0/1   0 for I-frame and 1 for P-frame */
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, PictureCodingType, 1);

        /* four_reserved_zero_bits      = 4 Bits        = 0      */
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 4);
    } else {
        static IMG_UINT8 RTYPE = 0;

        // if I- Frame set Update Full Extended PTYPE to true
        if ((PictureCodingType == I_FRAME) || (SourceFormatType == 7) || *OptionalCustomPCF) {
            UFEP = 1;
        } else {
            UFEP = 0;

            // RTYPE can be set to 1 only in case of P or PB picture.
            //RTYPE ^= 1;
        }

        // write UFEP of 3 bits.
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, UFEP, 3);

        // if UFEP was present( if it was 1).
        // Optional part of PPTYPE.
        if (UFEP == 1) {
            // write souce_format_optional. value = 110 (custom source format).
            lnc__write_upto8bits_elements(mtx_hdr, elt_p, 6, 3);
            /* souce_format_optional */

            lnc__write_upto8bits_elements(mtx_hdr, elt_p, *OptionalCustomPCF , 1);
            lnc__write_upto32bits_elements(mtx_hdr, elt_p, 0, 10);
            /* 10 reserve bits */
            lnc__write_upto8bits_elements(mtx_hdr, elt_p, 8, 4);
            /* 4 reserve bits */
        }
        /* picture_coding_type          = 1 Bit         = 0/1   0 for I-frame and 1 for P-frame */
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, PictureCodingType, 3);

        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 2);
        /* two_reserve_bits,      rounding_type,       two_reserve_bits       marker_bit       CPM */

        // Rounding Type (RTYPE) (1 for P Picture, 0 for all other Picture frames.
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, RTYPE, 1);
        //2 reserve bits
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 2);
        //   - 1 (ON) to prevent start code emulation.
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1 , 1);
        // CPM immediately follows the PPTYPE part of the header.
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0 , 1);

        if (UFEP == 1) {
            IMG_UINT16 ui16PWI, ui16PHI;

            lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 4);
            /* aspect ratio */
            //PictureWidth --;
            //lnc__write_upto8bits_elements(mtx_hdr, elt_p, (IMG_UINT8)(PictureWidth >> 8), 1);
            //lnc__write_upto8bits_elements(mtx_hdr, elt_p, (IMG_UINT8)(PictureWidth & 0xFF), 8);

            ui16PWI = (PictureWidth >> 2) - 1;
            lnc__write_upto32bits_elements(mtx_hdr, elt_p, (IMG_UINT8)ui16PWI, 9);
            lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);
            /* marker_bit                               = 1 Bit         = 1      */
            //lnc__write_upto8bits_elements(mtx_hdr, elt_p, (IMG_UINT8)(PictureHeigth >> 8), 1);
            //lnc__write_upto8bits_elements(mtx_hdr, elt_p, (IMG_UINT8)(PictureHeigth & 0xFF), 8);
            /* good up to that point */
            ui16PHI = PictureHeight >> 2;
            lnc__write_upto32bits_elements(mtx_hdr, elt_p, (IMG_UINT8)ui16PHI, 9);

            /* lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1); */
            /* marker_bit                               = 1 Bit         = 1      */
            /* just checking */
            if (*OptionalCustomPCF == 1) {
                //IMG_UINT8 CPCFC;
                //CPCFC = (IMG_UINT8)(1800/(IMG_UINT16)FrameRate);
                /* you can use the table for division */
                //CPCFC <<= 1; /* for Clock Conversion Code */
                lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);
                // Clock Divisor : 7 bits The natural binary representation of the value of the clock divisor.
                lnc__write_upto8bits_elements(mtx_hdr, elt_p, 1800000 / (FrameRate * 1000), 7);
            }
        }
        if (*OptionalCustomPCF == 1) {
            IMG_UINT8 ui8ETR; // extended Temporal reference
            // Two MSBs of 10 bit temporal_reference : value 0
            ui8ETR = Temporal_Ref >> 8;

            lnc__write_upto8bits_elements(mtx_hdr, elt_p, ui8ETR, 2);
            /* Two MSBs of temporal_reference */
        }
    }
    /* vop_quant = 5 Bits       = x     5-bit frame Q_scale from rate control - GENERATED BY MTX */
    /* lnc__write_upto8bits_elements(mtx_hdr, elt_p, Q_Scale, 5); */

    /* Insert token to tell MTX to insert rate-control value
     * (QScale is sent as an argument in MTX_Send_Elements_To_VLC(&MTX_Header, FrameQScale))
     */
    lnc__insert_element_token(mtx_hdr, elt_p, ELEMENT_FRAMEQSCALE);


    lnc__insert_element_token(mtx_hdr, elt_p, ELEMENT_RAWDATA);

    /* zero_bit = 1 Bit         = 0
     * pei      = 1 Bit         = 0     No direct effect on encoding of picture
     */
    if (SourceFormatType != 7) {
        lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);
    }

    /*
      FOLLOWING SECTION CAN'T BE GENERATED HERE
      gob_data( )
      for(i=1; i<num_gob_in_picture; i++) {
      gob_header( )
      gob_data( )
      }
    */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);
    return;
}


static void H263_writebits_GOBSliceHeader(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p,
    IMG_UINT8 GOBNumber,
    IMG_UINT8 GOBFrameId)
{
#ifdef USESTATICWHEREPOSSIBLE
    IMG_UINT16 *p;
    p = (IMG_UINT16 *) mtx_hdr;
    /*
      p[0]=1;
      p[1]=p[2]=p[3]=0;
    */
    *(int *)mtx_hdr = 1;
    p[4] = 24;
    p[5] = (128 | ((GOBNumber & 31) << 2) | (GOBFrameId & 3)) << 8;
    p[6] = 5;
#else
    /* Essential we insert the element before we try to fill it! */
    lnc__insert_element_token(mtx_hdr, elt_p, ELEMENT_STARTCODE_RAWDATA);

    /* gob_resync_marker                = 17            = 0x1 */
    lnc__write_upto32bits_elements(mtx_hdr, elt_p, 1, 17);

    /* gob_number = 5   = 0-17  It is gob number in a picture */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, GOBNumber, 5);

    /* gob_frame_id     = 2 = 0-3       See note */
    lnc__write_upto8bits_elements(mtx_hdr, elt_p, GOBFrameId, 2);

    /* quant_scale      = 5     = 1-32  gob (Slice) Q_scale  */
    /* lnc__write_upto8bits_elements(mtx_hdr, elt_p, GOB_Q_Scale, 5); */

    /* Insert token to tell MTX to insert rate-control value
     *  (QScale is sent as an argument in MTX_Send_Elements_To_VLC(&MTX_Header, SliceQScale))
     */
    lnc__insert_element_token(mtx_hdr, elt_p, ELEMENT_SLICEQSCALE);
#endif
}

/*
 * High level functions to call when a H263 header is required - HOST ROUTINES
 */
static void lnc__H263_getelements_videosequence_header(
    MTX_HEADER_PARAMS *mtx_hdr,
    IMG_UINT8 Profile_and_level_indication)
{
    /* Builds a single H263 video sequence header from the given parameters */

    /* Essential we initialise our header structures before building */
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *elt_p[MAXNUMBERELEMENTS];
    mtx_hdr->Elements = ELEMENTS_EMPTY;
    This_Element = (MTX_HEADER_ELEMENT *) mtx_hdr->asElementStream;
    elt_p[0] = This_Element;

    H263_writebits_VideoSequenceHeader(mtx_hdr, elt_p, Profile_and_level_indication);

    mtx_hdr->Elements++; /* Has been used as an index, so need to add 1 for a valid element count */
}

static void lnc__H263_getelements_videopicture_header(
    MTX_HEADER_PARAMS *mtx_hdr,
    IMG_UINT8 Temporal_Ref,
    H263_PICTURE_CODING_TYPE PictureCodingType,
    H263_SOURCE_FORMAT_TYPE SourceFormatType,
    IMG_UINT8 FrameRate,
    IMG_UINT16 PictureWidth,
    IMG_UINT16 PictureHeight,
    IMG_UINT8 *OptionalCustomPCF)
{
    /* Essential we initialise our header structures before building */
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *elt_p[MAXNUMBERELEMENTS];
    mtx_hdr->Elements = ELEMENTS_EMPTY;
    This_Element = (MTX_HEADER_ELEMENT *) mtx_hdr->asElementStream;
    elt_p[0] = This_Element;

    H263_writebits_VideoPictureHeader(
        mtx_hdr, elt_p,
        Temporal_Ref,
        PictureCodingType,
        SourceFormatType,
        FrameRate,
        PictureWidth,
        PictureHeight,
        OptionalCustomPCF);
    mtx_hdr->Elements++; /* Has been used as an index, so need to add 1 for a valid element count */
}

static void lnc__H263_getelements_GOBslice_header(
    MTX_HEADER_PARAMS *mtx_hdr,
    IMG_UINT8 GOBNumber,
    IMG_UINT8 GOBFrameId)
{
    /* Essential we initialise our header structures before building */
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *elt_p[MAXNUMBERELEMENTS];
    mtx_hdr->Elements = ELEMENTS_EMPTY;
    This_Element = (MTX_HEADER_ELEMENT *) mtx_hdr->asElementStream;
    elt_p[0] = This_Element;

    H263_writebits_GOBSliceHeader(mtx_hdr, elt_p, GOBNumber, GOBFrameId);

    mtx_hdr->Elements++; //Has been used as an index, so need to add 1 for a valid element count
}



void lnc__H264_prepare_sequence_header(
    IMG_UINT32 *pHeaderMemory,
    IMG_UINT32 uiMaxNumRefFrames,
    IMG_UINT32 uiPicWidthInMbs,
    IMG_UINT32 uiPicHeightInMbs,
    IMG_BOOL VUI_present, H264_VUI_PARAMS *VUI_params,
    H264_CROP_PARAMS *psCropParams,
    IMG_UINT8 uiLevel,
    IMG_UINT8 uiProfile)
{
    H264_SEQUENCE_HEADER_PARAMS SHParams;
    MTX_HEADER_PARAMS   *mtx_hdr;

    memset(&SHParams, 0, sizeof(SHParams));

    /* Route output elements to memory provided */
    mtx_hdr = (MTX_HEADER_PARAMS *) pHeaderMemory;

    /* Setup Sequence Header information  */
    switch (uiProfile) {
    case 5:
        SHParams.ucProfile  = SH_PROFILE_BP;
        break;
    case 6:
        SHParams.ucProfile  = SH_PROFILE_MP;
        break;
    default:
        SHParams.ucProfile  = SH_PROFILE_MP;
        break;
    }

    switch (uiLevel) {
    case 10:
        SHParams.ucLevel =  SH_LEVEL_1;
        break;
    case 111:
        SHParams.ucLevel =  SH_LEVEL_1B;
        break;
    case 11:
        SHParams.ucLevel =  SH_LEVEL_11;
        break;
    case 12:
        SHParams.ucLevel =  SH_LEVEL_12;
        break;
    case 20:
        SHParams.ucLevel =  SH_LEVEL_2;
        break;
    case 30:
        SHParams.ucLevel =  SH_LEVEL_3;
        break;
    case 31:
        SHParams.ucLevel =  SH_LEVEL_31;
        break;
    default:
        SHParams.ucLevel =  SH_LEVEL_3;
        break;
    }

    SHParams.ucMax_num_ref_frames = uiMaxNumRefFrames;
    SHParams.ucWidth_in_mbs_minus1 = (IMG_UINT8)(uiPicWidthInMbs - 1);
    SHParams.ucHeight_in_maps_units_minus1 = (IMG_UINT8)(uiPicHeightInMbs - 1);
    SHParams.VUI_Params_Present = VUI_present;
    if (VUI_present)
        SHParams.VUI_Params = *VUI_params;

    /* All picture header information is static
     * (apart from 'pic_init_qp_minus26' and 'rsbp_byte_align' fields, which are set in MTX anyway)
     */

    /* Clears ensures elementstream memory is zeroed.. not necessary, but makes it easier to debug
     * for (Lp=0;Lp<MAX_HEADERSIZEWORDS-1;Lp++) MTX_Header.asElementStream[Lp]=0;
     */

#if HEADERS_VERBOSE_OUTPUT
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "\n\n**********************************************************************\n");
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "******** HOST FIRMWARE ROUTINES TO PASS HEADERS AND TOKENS TO MTX******\n");
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "**********************************************************************\n\n");
#endif

    /* Functions that actually pack Elements (MTX_HEADER_PARAMS structure) with header information */
    lnc__H264_getelements_sequence_header(mtx_hdr, &SHParams, psCropParams);
}

void lnc__H264_prepare_picture_header(IMG_UINT32 *pHeaderMemory)
{
    MTX_HEADER_PARAMS   *mtx_hdr;


    /* Route output elements to memory provided */
    mtx_hdr = (MTX_HEADER_PARAMS *) pHeaderMemory;

    /* All picture header information is static
     * (apart from 'pic_init_qp_minus26' and 'rsbp_byte_align' fields, which are set in MTX anyway)
     */

    /* Clears ensures elementstream memory is zeroed.. not necessary, but makes it easier to debug
     * for (Lp=0;Lp<MAX_HEADERSIZEWORDS-1;Lp++) MTX_Header.asElementStream[Lp]=0;
     */

#if HEADERS_VERBOSE_OUTPUT
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "\n\n**********************************************************************\n");
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "******** HOST FIRMWARE ROUTINES TO PASS HEADERS AND TOKENS TO MTX******\n");
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "**********************************************************************\n\n");
#endif

    /* Functions that actually pack Elements (MTX_HEADER_PARAMS structure) with header information */
    lnc__H264_getelements_picture_header(mtx_hdr);
}

void lnc__H264_prepare_slice_header(
    IMG_UINT32 *pHeaderMemory,
    IMG_BOOL    bIntraSlice,
    IMG_UINT32 uiDisableDeblockingFilterIDC,
    IMG_UINT32 uiFrameNumber,
    IMG_UINT32 uiFirst_MB_Address,
    IMG_UINT32 uiMBSkipRun,
    IMG_UINT32 force_idr,
    IMG_BOOL bUsesLongTermRef,
    IMG_BOOL bIsLongTermRef,
    IMG_UINT16 uiIdrPicId)
{
    H264_SLICE_HEADER_PARAMS SlHParams;
    MTX_HEADER_PARAMS *mtx_hdr;

    /* Route output elements to memory provided */
    mtx_hdr = (MTX_HEADER_PARAMS *) pHeaderMemory;

    SlHParams.Start_Code_Prefix_Size_Bytes = 4;

    /* pcb -  I think that this is more correct now*/
    if (force_idr)
        SlHParams.SliceFrame_Type = SLHP_IDR_SLICEFRAME_TYPE;
    else
        SlHParams.SliceFrame_Type = bIntraSlice ? SLHP_I_SLICEFRAME_TYPE : SLHP_P_SLICEFRAME_TYPE;

    SlHParams.Frame_Num_DO = (IMG_UINT8) uiFrameNumber % (1 << 5);
    SlHParams.Picture_Num_DO = (IMG_UINT8)(SlHParams.Frame_Num_DO * 2);

    SlHParams.bUsesLongTermRef = bUsesLongTermRef;
    SlHParams.bIsLongTermRef = bIsLongTermRef;

    SlHParams.First_MB_Address  =  uiFirst_MB_Address;
    SlHParams.Disable_Deblocking_Filter_Idc = (IMG_UINT8) uiDisableDeblockingFilterIDC;

    if (uiMBSkipRun)
        lnc__H264_getelements_skip_P_slice(mtx_hdr, &SlHParams, uiMBSkipRun);
    else
        lnc__H264_getelements_slice_header(mtx_hdr, &SlHParams, uiIdrPicId);
}

void lnc__H264_prepare_eodofstream_header(IMG_UINT32 *pHeaderMemory)
{
    MTX_HEADER_PARAMS *mtx_hdr;

    /* Route output elements to memory provided */
    mtx_hdr = (MTX_HEADER_PARAMS *) pHeaderMemory;

    lnc__H264_getelements_endofstream_header(mtx_hdr);
}

void lnc__H264_prepare_endofpicture_header(IMG_UINT32 *pHeaderMemory)
{
    MTX_HEADER_PARAMS *mtx_hdr;

    /* Route output elements to memory provided */
    mtx_hdr = (MTX_HEADER_PARAMS *) pHeaderMemory;

    /* H264_GetElements_EndOfPicture_Header(MTX_Header); */
}

void lnc__H264_prepare_endofsequence_header(IMG_UINT32 *pHeaderMemory)
{
    MTX_HEADER_PARAMS *mtx_hdr;

    /* Route output elements to memory provided */
    mtx_hdr = (MTX_HEADER_PARAMS *) pHeaderMemory;

    lnc__H264_getelements_endofsequence_header(mtx_hdr);
}

void lnc__MPEG4_prepare_sequence_header(
    IMG_UINT32 *pHeaderMemory,
    IMG_BOOL bBFrame,
    MPEG4_PROFILE_TYPE sProfile,
    IMG_UINT8 Profile_and_level_indication,
    FIXED_VOP_TIME_TYPE sFixed_vop_time_increment,
    IMG_UINT32 Picture_Width_Pixels,
    IMG_UINT32 Picture_Height_Pixels,
    IMG_BOOL bVBVPresent,
    IMG_UINT32  First_half_bit_rate,
    IMG_UINT32  Latter_half_bit_rate,
    IMG_UINT32  First_half_vbv_buffer_size,
    IMG_UINT32  Latter_half_vbv_buffer_size,
    IMG_UINT32  First_half_vbv_occupancy,
    IMG_UINT32  Latter_half_vbv_occupancy,
    IMG_UINT32 VopTimeResolution)
{
    MTX_HEADER_PARAMS *mtx_hdr;
    VBVPARAMS sVBVParams;

    sVBVParams.First_half_bit_rate = First_half_bit_rate;
    sVBVParams.Latter_half_bit_rate = Latter_half_bit_rate;
    sVBVParams.First_half_vbv_buffer_size = First_half_vbv_buffer_size;
    sVBVParams.Latter_half_vbv_buffer_size = Latter_half_vbv_buffer_size;
    sVBVParams.First_half_vbv_occupancy = First_half_vbv_occupancy;
    sVBVParams.Latter_half_vbv_occupancy = Latter_half_vbv_occupancy;

    /* Route output elements to memory provided */
    mtx_hdr = (MTX_HEADER_PARAMS *) pHeaderMemory;

    if (bVBVPresent)
        lnc__MPEG4_getelements_sequence_header(
            mtx_hdr, bBFrame, sProfile,
            Profile_and_level_indication,
            sFixed_vop_time_increment,
            Picture_Width_Pixels, Picture_Height_Pixels,
            &sVBVParams, VopTimeResolution);
    else
        lnc__MPEG4_getelements_sequence_header(
            mtx_hdr, bBFrame, sProfile,
            Profile_and_level_indication,
            sFixed_vop_time_increment,
            Picture_Width_Pixels, Picture_Height_Pixels,
            NULL, VopTimeResolution);/* NULL pointer if there are no VBVParams */
}

void lnc__MPEG4_prepare_vop_header(
    IMG_UINT32 *pHeaderMem,
    IMG_BOOL bIsVOP_coded,
    IMG_UINT32 VOP_time_increment,
    IMG_UINT8 sSearch_range,
    IMG_UINT8 eVop_Coding_Type,
    IMG_UINT32 VopTimeResolution)
{
    MTX_HEADER_PARAMS *mtx_hdr;

    mtx_hdr = (MTX_HEADER_PARAMS *) pHeaderMem;

    /* Frame QScale (and Slice QScale in the slice case)
     * no longer written here as it is inserted by MTX later
     * (add as parameter to MTX_Send_Elements_To_VLC)
     */
    lnc__MPEG4_getelements_VOP_header(
        mtx_hdr,
        bIsVOP_coded,
        VOP_time_increment,
        sSearch_range,
        eVop_Coding_Type, VopTimeResolution);
}

void lnc__H263_prepare_sequence_header(
    IMG_UINT32 *pHeaderMem,
    IMG_UINT8 Profile_and_level_indication)
{
    MTX_HEADER_PARAMS *mtx_hdr;

    mtx_hdr = (MTX_HEADER_PARAMS *) pHeaderMem;

    lnc__H263_getelements_videosequence_header(mtx_hdr, Profile_and_level_indication);
}

void lnc__H263_prepare_picture_header(
    IMG_UINT32 *pHeaderMem,
    IMG_UINT8 Temporal_Ref,
    H263_PICTURE_CODING_TYPE PictureCodingType,
    H263_SOURCE_FORMAT_TYPE SourceFormatType,
    IMG_UINT8 FrameRate,
    IMG_UINT16 PictureWidth,
    IMG_UINT16 PictureHeight,
    IMG_UINT8 *OptionalCustomPCF)
{
    MTX_HEADER_PARAMS *mtx_hdr;

    mtx_hdr = (MTX_HEADER_PARAMS *) pHeaderMem;
    lnc__H263_getelements_videopicture_header(
        mtx_hdr,
        Temporal_Ref,
        PictureCodingType,
        SourceFormatType,
        FrameRate,
        PictureWidth,
        PictureHeight,
        OptionalCustomPCF);
}

void lnc__H263_prepare_GOBslice_header(
    IMG_UINT32 *pHeaderMem,
    IMG_UINT8 GOBNumber,
    IMG_UINT8 GOBFrameId)
{
    MTX_HEADER_PARAMS *mtx_hdr;

    mtx_hdr = (MTX_HEADER_PARAMS *) pHeaderMem;

    lnc__H263_getelements_GOBslice_header(
        mtx_hdr,
        GOBNumber,
        GOBFrameId);

    /* silent the warning message */
    (void)Show_Bits;
    (void)Show_Elements;
    (void)lnc__H264_writebits_SEI_rbspheader;
    (void)lnc__H264_getelements_skip_B_slice;
    (void)lnc__H264_getelements_backward_zero_B_slice;
    (void)lnc__H264_getelements_rbsp_ATE_only;
    (void)lnc_MPEG4_getelements_video_packet_header;
}

