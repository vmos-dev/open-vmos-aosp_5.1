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
 *    Zeng Li <zeng.li@intel.com>
 *
 */


#include <stdio.h>
#include <string.h>
#include "img_types.h"
#include "psb_def.h"
#include "psb_drv_debug.h"
#include "pnw_hostheader.h"


/* Global stores the latest QP information for the DoHeader()
 * routine, should be filled in by the rate control algorithm.
 */
#define HEADERS_VERBOSE_OUTPUT 0

/* #define USESTATICWHEREPOSSIBLE 1 */

#define MAXNUMBERELEMENTS 16
#define HEADER_SIZE (128*2)

/* SOME USEFUL TEST FUNCTIONS */
#if 0

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

#if 0

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
    Show_Bits((IMG_UINT8 *) mtx_hdr->asElementStream, 0, RTotalByteSize);
#endif
}
#endif


/**
 * Header Writing Functions
 * Low level bit writing and ue, se functions
 * HOST CODE
 */
static void pnw__write_upto8bits_elements(
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
            pnw__write_upto8bits_elements(mtx_hdr, elt_p, wrt_bits, bit_cnt);
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

        pnw__write_upto8bits_elements(mtx_hdr, elt_p, InputVal.UI8Input[0], (IMG_UINT16) - Shift);
    }
}

static void pnw__write_upto32bits_elements(
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
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, Bytes[EndByte-1], (IMG_UINT8)((bit_cnt) % 8));
    else
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, Bytes[EndByte-1], 8);

    if (EndByte > 1)
        for (BitLp = EndByte - 1; BitLp > 0; BitLp--) {
            pnw__write_upto8bits_elements(mtx_hdr, elt_p, Bytes[BitLp-1], 8);
        }
}

static void pnw__generate_ue(
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
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 8);

    /* Write zeros and 1 bit set */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, (IMG_UINT8) 1, (IMG_UINT8)(uiLp + 1));

    /* Write Numeric part */
    while (ucZeros > 8) {
        ucZeros -= 8;
        uiChunk = (uiVal >> ucZeros);
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, (IMG_UINT8) uiChunk, 8);
        uiVal = uiVal - (uiChunk << ucZeros);
    }

    pnw__write_upto8bits_elements(mtx_hdr, elt_p, (IMG_UINT8) uiVal, ucZeros);
}


static void pnw__generate_se(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p,
    int iVal)
{
    IMG_UINT32 uiCodeNum;

    if (iVal > 0)
        uiCodeNum = (IMG_UINT32)(iVal + iVal - 1);
    else
        uiCodeNum = (IMG_UINT32)(-iVal - iVal);

    pnw__generate_ue(mtx_hdr, elt_p, uiCodeNum);
}


static void pnw__insert_element_token(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p,
    HEADER_ELEMENT_TYPE Token)
{
    IMG_UINT8 Offset;
    IMG_UINT8 *P;

    if (mtx_hdr->Elements != ELEMENTS_EMPTY &&
	    mtx_hdr->Elements > (MAXNUMBERELEMENTS - 1))
	return;

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

	if (mtx_hdr->Elements > (MAXNUMBERELEMENTS - 1))
	    return;
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
static void pnw__H264_writebits_startcode_prefix_element(
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
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 8);

    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 8);

    /* Byte aligned (bit 32 or 24) */
    return;
}


static void pnw__H264_writebits_VUI_params(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p,
    H264_VUI_PARAMS *VUIParams)
{
    /* Builds VUI Params for the Sequence Header (only present in the 1st sequence of stream) */
    if (VUIParams->aspect_ratio_info_present_flag && (VUIParams->aspect_ratio_idc == 0xff)) {
        /* aspect_ratio_info_present_flag u(1) */
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);
        /* aspect_ratio_idc u(8) Extended_SAR */
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0xff, 8);
        /* sar_width u(16) */
        pnw__write_upto32bits_elements(mtx_hdr, elt_p, VUIParams->sar_width, 16);
        /* sar_height u(16) */
        pnw__write_upto32bits_elements(mtx_hdr, elt_p, VUIParams->sar_height, 16);
    } else {
        /* aspect_ratio_info_present_flag u(1) */
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);
    }

    pnw__write_upto8bits_elements(
                mtx_hdr, elt_p,
                (0 << 3) | /* overscan_info_present_flag (1 bit) = 0 in Topaz */
                (0 << 2) | /* video_signal_type_present_flag (1 bit) = 0 in Topaz */
                (0 << 1) | /* chroma_loc_info_present_flag (1 bit) = 0 in Topaz */
                (1),/* timing_info_present_flag (1 bit) = 1 in Topaz */
                4);

    /* num_units_in_tick (32 bits) = 1 in Topaz */
    pnw__write_upto32bits_elements(mtx_hdr, elt_p, VUIParams->num_units_in_tick, 32);

    /* time_scale (32 bits) = frame rate */
    pnw__write_upto32bits_elements(mtx_hdr, elt_p, VUIParams->Time_Scale, 32);

    /* fixed_frame_rate_flag (1 bit) = 1 in Topaz */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);
    /* nal_hrd_parameters_present_flag (1 bit) = 1 in Topaz */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);

    /** Definitions for nal_hrd_parameters() contained in VUI structure for Topaz
     *  cpb_cnt_minus1 ue(v) = 0 in Topaz = 1b
     */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);
    /* bit_rate_scale (4 bits) = 0 in Topaz, cpb_size_scale (4 bits) = 0 in Topaz */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 4);
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 2, 4);

    /* bit_rate_value_minus1[0] ue(v) = (Bitrate/64)-1 [RANGE:0 to (2^32)-2] */
    pnw__generate_ue(mtx_hdr, elt_p, VUIParams->bit_rate_value_minus1);
    /* cpb_size_value_minus1[0] ue(v) = (CPB_Bits_Size/16)-1
     * where CPB_Bits_Size = 1.5 * Bitrate  [RANGE:0 to (2^32)-2]
     */
    pnw__generate_ue(mtx_hdr, elt_p, VUIParams->cbp_size_value_minus1);

    /* cbr_flag[0] (1 bit) = 0 for VBR, 1 for CBR */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, VUIParams->CBR, 1);

    pnw__write_upto8bits_elements(
        mtx_hdr, elt_p,
        VUIParams->initial_cpb_removal_delay_length_minus1,
        5); /* initial_cpb_removal_delay_length_minus1 (5 bits) = ??? */

    pnw__write_upto8bits_elements(
        mtx_hdr,
        elt_p,
        VUIParams->cpb_removal_delay_length_minus1,
        5); /* cpb_removal_delay_length_minus1 (5 bits) = ??? */

    pnw__write_upto8bits_elements(
        mtx_hdr, elt_p,
        VUIParams->dpb_output_delay_length_minus1,
        5); /* dpb_output_delay_length_minus1 (5 bits) = ??? */

    pnw__write_upto8bits_elements(
        mtx_hdr,
        elt_p,
        VUIParams->time_offset_length,
        5); /* time_offst_length (5 bits) = ??? */

    /* End of nal_hrd_parameters()  */

    pnw__write_upto8bits_elements(
        mtx_hdr,
        elt_p,
        0, 1);/* vcl_hrd_parameters_present_flag (1 bit) = 0 in Topaz */

    /* if( nal_hrd_parameters_present_flag  ||  vcl_hrd_parameters_present_flag )
     * FIX for BRN23039
     * low_delay_hrd_flag
     */
    pnw__write_upto8bits_elements(
        mtx_hdr, elt_p,
        0, 1);/* low_delay_hrd_flag */


    pnw__write_upto8bits_elements(
        mtx_hdr, elt_p,
        0, 1);/* pic_struct_present_flag (1 bit) = 0 in Topaz */

    pnw__write_upto8bits_elements(
        mtx_hdr, elt_p,
        0, 1);/* bitstream_restriction_flag (1 bit) = 0 in Topaz */
}


static void pnw__H264_writebits_picture_header(
    MTX_HEADER_PARAMS *pMTX_Header,
    MTX_HEADER_ELEMENT **aui32ElementPointers,
    IMG_BOOL bCabacEnabled,
    IMG_BOOL b_8x8transform,
    IMG_BOOL bIntraConstrained,
    IMG_INT8 i8CbQPOffset,
    IMG_INT8 i8CrQPOffset)
{
    //**-- Begin building the picture header element
    IMG_UINT8 ui8ECMF = (bCabacEnabled ? 1 : 0);

    pnw__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_STARTCODE_RAWDATA);

    pnw__H264_writebits_startcode_prefix_element(pMTX_Header, aui32ElementPointers, 4);

    ///* GENERATES THE FIRST (STATIC) ELEMENT OF THE H264_PICTURE_HEADER() STRUCTURE *///
    ///**** ELEMENT BITCOUNT: 18

    // 4 Byte StartCodePrefix Pregenerated in: H264_WriteBits_StartCodePrefix_Element()
    // Byte aligned (bit 32)
    pnw__write_upto8bits_elements(pMTX_Header,
                                  aui32ElementPointers,
                                  (0 << 7) |    // forbidden_zero_bit
                                  (1 << 5) |    // nal_ref_idc (2 bits) = 1
                                  (8),  // nal_unit_tpye (5 bits) = 8
                                  8);
    // Byte aligned (bit 40)
    pnw__write_upto8bits_elements(pMTX_Header,
                                  aui32ElementPointers,
                                  (1 << 7) |            // pic_parameter_set_id ue(v) = 0 in Topaz
                                  (1 << 6) |            // seq_parameter_set_id ue(v) = 0 in Topaz
                                  (ui8ECMF << 5) |      // entropy_coding_mode_flag (1 bit) 0 for CAVLC
                                  (0 << 4) |            // pic_order_present_flag (1 bit) = 0
                                  (1 << 3) |            // num_slice_group_minus1 ue(v) = 0 in Topaz
                                  (1 << 2) |            // num_ref_idx_l0_active_minus1 ue(v) = 0 in Topaz
                                  (1 << 1) |            // num_ref_idx_l1_active_minus1 ue(v) = 0 in Topaz
                                  (0),          // weighted_pred_flag (1 bit) = 0 in Topaz
                                  8);
    // Byte aligned (bit 48)
    // weighted_bipred_flag (2 bits) = 0 in Topaz
    pnw__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 2);

    //MTX fills this value in
    pnw__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_QP);
    pnw__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_RAWDATA);

    ///**** GENERATES THE SECOND ELEMENT OF THE H264_PICTURE_HEADER() STRUCTURE ****///
    ///**** ELEMENT BITCOUNT: 5
    //The following field will be generated as a special case by MTX - so not here
    // pnw__generate_se(pMTX_Header, pPHParams->pic_init_qp_minus26);
    // pic_int_qp_minus26 se(v) = -26 to 25 in Topaz
    // pic_int_qs_minus26 se(v) = 0 in Topaz
    //chroma_qp_index_offset se(v) = 0 in Topaz
    pnw__generate_se(pMTX_Header, aui32ElementPointers, 0);
    pnw__generate_se(pMTX_Header, aui32ElementPointers, i8CbQPOffset);
    pnw__write_upto8bits_elements(pMTX_Header,
                                  aui32ElementPointers,
                                  // deblocking_filter_control_present_flag (1 bit) = 1 in Topaz
                                  (1 << 2) |
                                  // constrained_intra_pred_Flag (1 bit) = 0 in Topaz
                                  ((bIntraConstrained ? 1 : 0) << 1) |
                                  (0),// redundant_pic_cnt_present_flag (1 bit) = 0 in Topaz
                                  3);
    // Byte align is done using an element command (MTX elements in this structure, we don't know it's size)

    if (b_8x8transform || (i8CbQPOffset != i8CrQPOffset)) {
        pnw__write_upto8bits_elements(pMTX_Header, aui32ElementPointers,
                                      (b_8x8transform << 1) |   // 8x8 transform flag
                                      (0),      // pic_scaling_matrix_present_flag
                                      2);
        pnw__generate_se(pMTX_Header, aui32ElementPointers, i8CrQPOffset);
        // second_chroma_qp_index_offset se(v) = 0 in Topaz
    }
    // Tell MTX to insert the byte align field (we don't know final stream size for alignment at this point)
    pnw__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_INSERTBYTEALIGN_H264);

    return;
}


static void pnw__H264_writebits_slice_header0(
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
    pnw__write_upto8bits_elements(
        mtx_hdr, elt_p,
        (0 << 7) |/* forbidden_zero_bit */
        ((pSlHParams->SliceFrame_Type == SLHP_B_SLICEFRAME_TYPE ? 0 : 1) << 5) | /* nal_ref_idc (2 bits) = 0 for B-frame and 1 for I or P-frame */
        ((pSlHParams->SliceFrame_Type == SLHP_IDR_SLICEFRAME_TYPE ? 5 : 1)),/* nal_unit_tpye (5 bits) = I-frame IDR, and 1 for  rest */
        8);
}


static void pnw__H264_writebits_slice_header1(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p,
    H264_SLICE_HEADER_PARAMS *pSlHParams,
    IMG_UINT16 uiIdrPicId)
{
    /* GENERATES THE SECOND ELEMENT OF THE H264_SLICE_HEADER() STRUCTURE
     * The following is slice parameter set in BP/MP
     */

    /* first_mb_in_slice = First MB address in slice: ue(Range 0 -  1619) */
    pnw__generate_ue(mtx_hdr, elt_p, (IMG_UINT32) pSlHParams->First_MB_Address);

    pnw__generate_ue(
        mtx_hdr, elt_p,
        (IMG_UINT32)((pSlHParams->SliceFrame_Type == SLHP_IDR_SLICEFRAME_TYPE) ? SLHP_I_SLICEFRAME_TYPE : pSlHParams->SliceFrame_Type));  /* slice_type ue(v): 0 for P-slice, 1 for B-slice, 2 for I-slice */

    /* kab: not clean change from IDR to intra, IDR should have separate flag */

    pnw__write_upto8bits_elements(
        mtx_hdr, elt_p,
        (1 << 5) | /* pic_parameter_set_id, ue(v) = 0  (=1b) in Topaz */
        pSlHParams->Frame_Num_DO,/* frame_num (5 bits) = frame nuo. in decoding order */
        6);

    /* frame_mb_only_flag is always 1, so no need for field_pic_flag or bottom_field_flag */
    if (pSlHParams->SliceFrame_Type == SLHP_IDR_SLICEFRAME_TYPE)
        pnw__generate_ue(mtx_hdr, elt_p, uiIdrPicId);/* idr_pic_id ue(v) = 0 (1b) in Topaz */

    /* kab: Idr_pic_id only for IDR, not nessesarely for all I pictures */

    /* customer request POC type should be 2*/
    /* if (pic_order_cnt_type == 0) Note: (pic_order_cnt_type always 0 in Topaz)
    pnw__write_upto8bits_elements(
        mtx_hdr, elt_p,
        pSlHParams->Picture_Num_DO,
        6);  pic_order_cnt_lsb (6 bits) - picture no in display order */

#if 0
    if (pSlHParams->SliceFrame_Type == SLHP_B_SLICEFRAME_TYPE)
        pnw__write_upto8bits_elements(
            mtx_hdr,
            elt_p,
            0,
            1);/* direct_spatial_mv_pred_flag (1 bit) = 0, spatial direct mode not supported in Topaz */
#endif

    if (pSlHParams->SliceFrame_Type == SLHP_P_SLICEFRAME_TYPE || pSlHParams->SliceFrame_Type == SLHP_B_SLICEFRAME_TYPE)
        pnw__write_upto8bits_elements(
            mtx_hdr,
            elt_p,
            0,
            1);/* num_ref_idx_active_override_flag (1 bit) = 0 in Topaz */

    if (pSlHParams->SliceFrame_Type != SLHP_I_SLICEFRAME_TYPE &&
        pSlHParams->SliceFrame_Type != SLHP_IDR_SLICEFRAME_TYPE) {
        /*
        if (pSlHParams->UsesLongTermRef) {
            // ref_pic_list_ordering_flag_I0 (1 bit) = 1, L0 reference picture ordering
            pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);
            // modification_of_pic_nums_idc = 2
            pnw__generate_ue(mtx_hdr, elt_p, 2);
            // long_term_pic_num = 0
            pnw__generate_ue(mtx_hdr, elt_p, 0);
            // modification_of_pic_nums_idc = 3
            pnw__generate_ue(mtx_hdr, elt_p, 3);
        } else
        */
            pnw__write_upto8bits_elements(
                mtx_hdr,
                elt_p,
                0,
                1);/* ref_pic_list_ordering_flag_I0 (1 bit) = 0 */
    }

#if 0    
    if (pSlHParams->SliceFrame_Type == SLHP_B_SLICEFRAME_TYPE)
        pnw__write_upto8bits_elements(
            mtx_hdr,
            elt_p,
            0,
            1); /* ref_pic_list_ordering_flag_I1 (1 bit) = 0, no reference picture ordering in Topaz */
#endif

    if (pSlHParams->SliceFrame_Type == SLHP_IDR_SLICEFRAME_TYPE) {
        /* no_output_of_prior_pics_flag (1 bit) = 0 */
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);
        /* long_term_reference_flag (1 bit)*/
        pnw__write_upto8bits_elements(mtx_hdr, elt_p,
                                      pSlHParams->IsLongTermRef ? 1 : 0, 1);
    } 
#if 0
    else if (pSlHParams->UsesLongTermRef) {
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);/* long_term_reference_flag (1 bit) = 0 */
        // Allow a single long-term reference
        pnw__generate_ue(mtx_hdr, elt_p, 4);
        // memory_management_control_operation
        pnw__generate_ue(mtx_hdr, elt_p, 1);

        // Set current picture as the long-term reference
        // memory_management_control_operation
        pnw__generate_ue(mtx_hdr, elt_p, 6);
        // long_term_frame_idx
        pnw__generate_ue(mtx_hdr, elt_p, 0);

        // End
        pnw__generate_ue(mtx_hdr, elt_p, 0);
    } 
#endif
    else {
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);/* adaptive_ref_pic_marking_mode_flag (1 bit) = 0 */
    }
}


static void pnw__H264_writebits_slice_header2(
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
    /* pucHS=pnw__generate_se(pucHS, puiBitPos, pSlHParams->Slice_QP_Delta);  */
#endif

    pnw__generate_ue(
        mtx_hdr,
        elt_p,
        pSlHParams->Disable_Deblocking_Filter_Idc); /* disable_deblocking_filter_idc ue(v) = 2?  */

    if (pSlHParams->Disable_Deblocking_Filter_Idc != 1) {
        pnw__generate_se(mtx_hdr, elt_p, 0); /* slice_alpha_c0_offset_div2 se(v) = 0 (1b) in Topaz */
        pnw__generate_se(mtx_hdr, elt_p, 0); /* slice_beta_offset_div2 se(v) = 0 (1b) in Topaz */
    }

    /* num_slice_groups_minus1 ==0 in Topaz, so no slice_group_change_cycle field here
     * no byte alignment at end of slice headers
     */
}

static void pnw__H264_writebits_sequence_header(
    MTX_HEADER_PARAMS *pMTX_Header,
    MTX_HEADER_ELEMENT **aui32ElementPointers,
    H264_SEQUENCE_HEADER_PARAMS *pSHParams, H264_CROP_PARAMS *psCrop)
{
    IMG_UINT8 ui8SBP;

    pnw__insert_element_token(pMTX_Header,
                              aui32ElementPointers,
                              ELEMENT_STARTCODE_RAWDATA);

    pnw__H264_writebits_startcode_prefix_element(pMTX_Header,
            aui32ElementPointers,
            4);

    ///**** GENERATES THE FIRST ELEMENT OF THE H264_SEQUENCE_HEADER() STRUCTURE ****///

    // 4 Byte StartCodePrefix Pregenerated in: H264_WriteBits_StartCodePrefix_Element()
    // Byte aligned (bit 32)
    pnw__write_upto8bits_elements(pMTX_Header,
                                  aui32ElementPointers, (0 << 7) |      // forbidden_zero_bit=0
                                  (0x3 << 5) |                  // nal_ref_idc=01 (may be 11)
                                  (7),          // nal_unit_type=00111
                                  8);

    if (pSHParams->ucProfile != SH_PROFILE_HP) {
        // profile_idc = 8 bits = 66 for BP (PROFILE_IDC_BP), 77 for MP (PROFILE_IDC_MP)
        pnw__write_upto8bits_elements(pMTX_Header,
                                      aui32ElementPointers,
                                      (pSHParams->ucProfile == SH_PROFILE_BP ? 66 : 77), 8);

        // Byte aligned (bit 48)
        pnw__write_upto8bits_elements(pMTX_Header,
                                      // constrain_set0_flag = 1 for BP constraints
                                      aui32ElementPointers, (0 << 7) |
                                      // constrain_set1_flag  = 1 for MP constraints and Constrained Baseline profile.
                                      ((pSHParams->ucProfile == SH_PROFILE_BP ? 1 : 0) << 6) |
                                      (0 << 5) |        // constrain_set2_flag = 1 for HP
                                      // constrain_set3_flag = 1 for level 1b, 0 for others
                                      ((pSHParams->ucLevel == SH_LEVEL_1B ?     1 : 0) <<       4),
                                      // reserved_zero_4bits = 0
                                      8);
        // Byte aligned (bit 56)
        pnw__write_upto8bits_elements(pMTX_Header,
                                      aui32ElementPointers,
                                      // level_idc (8 bits) = 11 for 1b, 10xlevel for others
                                      (IMG_UINT8) pSHParams->ucLevel, 8);
#if 0
        // Byte aligned (bit 64)
        pnw__write_upto8bits_elements(pMTX_Header,
                                      // seq_parameter_Set_id = 0 in Topaz ->  ue(0)= 1b
                                      aui32ElementPointers, (1 << 7) |
                                      (2 << 4) |// log2_max_frame_num_minus4 = 1 in Topaz ->  ue(1)= 010b
                                      (1 << 3) |        // pic_order_cnt_type = 0 in Topaz ->  ue(0)= 1b
                                      (3),      // log2_max_pic_order_cnt_Isb_minus4 = 2 in Topaz ->  ue(2)= 011b
                                      8);
        // Bytes aligned (bit 72)
#else
        /*Customer request POC type should be 2*/
        pnw__write_upto8bits_elements(pMTX_Header,
                                      // seq_parameter_Set_id = 0 in Topaz ->  ue(0)= 1b
                                      aui32ElementPointers, (1 << 6) |
                                      (2 << 3) |// log2_max_frame_num_minus4 = 1 in Topaz ->  ue(1)= 010b
                                      (3),// log2_max_pic_order_cnt_Isb_minus4 = 2 in Topaz ->  ue(2)= 011b$
                                      7);
#endif
    } else {
#if PSB_MFLD_DUMMY_CODE
        // Byte aligned (bit 40)
        // profile_idc = 8 bits = 66 for BP (PROFILE_IDC_BP), 77 for MP (PROFILE_IDC_MP)
        pnw__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 100, 8);

        // Byte aligned (bit 48)
        pnw__write_upto8bits_elements(pMTX_Header,
                                      aui32ElementPointers, (1 << 7) |
                                      // constrain_set0_flag = 1 for MP + BP constraints
                                      (1 << 6) |        // constrain_set1_flag  = 1 for MP + BP constraints
                                      (0 << 5) |        // constrain_set2_flag = always 0 in BP/MP
                                      (0 << 4), // constrain_set3_flag = 1 for level 1b, 0 for others
                                      // reserved_zero_4bits = 0
                                      8);

        // Byte aligned (bit 56)
        pnw__write_upto8bits_elements(pMTX_Header,
                                      aui32ElementPointers, (IMG_UINT8) pSHParams->ucLevel, 8);
        // level_idc (8 bits) = 11 for 1b, 10xlevel for others

        pnw__generate_ue(pMTX_Header, aui32ElementPointers, 0);
        // seq_parameter_Set_id = 0
        pnw__generate_ue(pMTX_Header, aui32ElementPointers, 1);
        // chroma_format_idc = 1
        pnw__generate_ue(pMTX_Header, aui32ElementPointers, 0);
        // bit_depth_luma_minus8 = 0
        pnw__generate_ue(pMTX_Header, aui32ElementPointers, 0);
        // bit_depth_chroma_minus8 = 0
        pnw__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);
        // qpprime_y_zero_transform_bypass_flag = 0

        //if (pSHParams->bUseDefaultScalingList)
        //{
        pnw__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1);
        // seq_scaling_matrix_present_flag
        pnw__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 8);
        // seq_scaling_matrix_present_flag[i] = 0; 0 < i < 8
        //}
        //else
        //{
        //      pnw__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);         // seq_scaling_matrix_present_flag
        //}

        pnw__generate_ue(pMTX_Header, aui32ElementPointers, 1);
        // log2_max_frame_num_minus4 = 1
        // Customer request POC type should be 2.
        //pnw__generate_ue(pMTX_Header, aui32ElementPointers, 0);
        // pic_order_cnt_type = 0
        pnw__generate_ue(pMTX_Header, aui32ElementPointers, 2);
        // log2_max_pic_order_cnt_Isb_minus4 = 2

        // Bytes aligned (bit 72)
#endif
    }
    // max_num_ref_frames. long term reference isn't used.
    //pnw__generate_u(pMTX_Header, aui32ElementPointers, pSHParams->max_num_ref_frames);
    pnw__generate_ue(pMTX_Header, aui32ElementPointers, 1);
    // gaps_in_frame_num_value_allowed_Flag - (1 bit)
    pnw__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, pSHParams->gaps_in_frame_num_value, 1);


    ///**** GENERATES THE SECOND, VARIABLE LENGTH, ELEMENT OF THE H264_SEQUENCE_HEADER() STRUCTURE ****///
    ///**** ELEMENT BITCOUNT: xx
    pnw__generate_ue(pMTX_Header, aui32ElementPointers, pSHParams->ucWidth_in_mbs_minus1);
    //pic_width_in_mbs_minus1: ue(v) from 10 to 44 (176 to 720 pixel per row)
    pnw__generate_ue(pMTX_Header, aui32ElementPointers, pSHParams->ucHeight_in_maps_units_minus1);
    //pic_height_in_maps_units_minus1: ue(v) Value from 8 to 35 (144 to 576 pixels per column)

    // We don't know the alignment at this point, so will have to use bit writing functions
    pnw__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, pSHParams->ucFrame_mbs_only_flag, 1);
    // frame_mb_only_flag 1=frame encoding, 0=field encoding

#if 0
    if (!pSHParams->ucFrame_mbs_only_flag) // in the case of interlaced encoding
        pnw__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);
    // mb_adaptive_frame_field_flag = 0 in Topaz(field encoding at the sequence level)
#endif

    pnw__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1);
    // direct_8x8_inference_flag=1 in Topaz

    if (psCrop->bClip) {
        pnw__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1);
        pnw__generate_ue(pMTX_Header, aui32ElementPointers, psCrop->LeftCropOffset);
        pnw__generate_ue(pMTX_Header, aui32ElementPointers, psCrop->RightCropOffset);
        pnw__generate_ue(pMTX_Header, aui32ElementPointers, psCrop->TopCropOffset);
        pnw__generate_ue(pMTX_Header, aui32ElementPointers, psCrop->BottomCropOffset);

    } else {
        pnw__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 1);
    }

    ///**** GENERATES THE THIRD ELEMENT OF THE H264_SEQUENCE_HEADER() STRUCTURE ****///
    ///**** ELEMENT BITCOUNT: xx
    pnw__write_upto8bits_elements(pMTX_Header, aui32ElementPointers,
                                  (pSHParams->VUI_Params_Present),
                                  // vui_parameters_present_flag (VUI only in 1st sequence of stream)
                                  1);
    if (pSHParams->VUI_Params_Present > 0)
        pnw__H264_writebits_VUI_params(pMTX_Header, aui32ElementPointers, &(pSHParams->VUI_Params));

    // Finally we need to align to the next byte
    // We know the size of the data in the sequence header (no MTX variables)  and start is byte aligned, so it's possible to add this field here rather than MTX ELEMENT_INSERTBYTEALIGN_H264 command.
    pnw__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 1, 1);
    ui8SBP = (aui32ElementPointers[pMTX_Header->Elements]->Size) & 7;
    if (ui8SBP > 0)
        pnw__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0, 8 - (ui8SBP));
    return;
}


static void pnw__H264_writebits_slice_header(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p,
    H264_SLICE_HEADER_PARAMS *pSlHParams,
    IMG_BOOL __maybe_unused bCabacEnabled,
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
    pnw__insert_element_token(mtx_hdr, elt_p, ELEMENT_STARTCODE_RAWDATA);

    pnw__H264_writebits_startcode_prefix_element(
        mtx_hdr,
        elt_p,
        pSlHParams->Start_Code_Prefix_Size_Bytes); /* Can be 3 or 4 bytes - always 4 bytes in our implementations */
    pnw__H264_writebits_slice_header0(mtx_hdr, elt_p, pSlHParams);
#endif

    pnw__H264_writebits_slice_header1(mtx_hdr, elt_p, pSlHParams, uiIdrPicId);

#if 0
    if (bCabacEnabled && ((SLHP_P_SLICEFRAME_TYPE == pSlHParams->SliceFrame_Type) ||
                          (SLHP_B_SLICEFRAME_TYPE == pSlHParams->SliceFrame_Type))) {
        pnw__generate_ue(mtx_hdr, elt_p, 0);    /* hard code cabac_init_idc value of 0 */
    }
#endif

    pnw__insert_element_token(mtx_hdr, elt_p, ELEMENT_SQP); /* MTX fills this value in */

    pnw__insert_element_token(mtx_hdr, elt_p, ELEMENT_RAWDATA);
    pnw__H264_writebits_slice_header2(mtx_hdr, elt_p, pSlHParams);

    /* no byte alignment at end of slice headers */
}


static void pnw__H264_getelements_skip_P_slice(
    MTX_HEADER_PARAMS *mtx_hdr,
    H264_SLICE_HEADER_PARAMS *pSlHParams,
    IMG_UINT32 MB_No_In_Slice,
    IMG_BOOL bCabacEnabled)
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

    /* pnw__insert_element_token(mtx_hdr, ELEMENT_STARTCOUNTER); */
    /* Not sure if this will be required in the final spec */
    pnw__H264_writebits_slice_header(mtx_hdr, elt_p, pSlHParams, bCabacEnabled, 0);
    pnw__generate_ue(mtx_hdr, elt_p, MB_No_In_Slice); /* mb_skip_run = mb_no_in_slice */

    /* Tell MTX to insert the byte align field (we don't know final stream size for alignment at this point) */
    pnw__insert_element_token(mtx_hdr, elt_p, ELEMENT_INSERTBYTEALIGN_H264);
    mtx_hdr->Elements++; /* Has been used as an index, so need to add 1 for a valid element count */
}



static void pnw__H264_getelements_sequence_header(
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

    pnw__H264_writebits_sequence_header(mtx_hdr, elt_p, pSHParams, psCropParams);
    mtx_hdr->Elements++; /* Has been used as an index, so need to add 1 for a valid element count */
}



static void pnw__H264_getelements_slice_header(
    MTX_HEADER_PARAMS *mtx_hdr,
    H264_SLICE_HEADER_PARAMS *pSlHParams,
    IMG_BOOL bCabacEnabled,
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
    /* pnw__insert_element_token(mtx_hdr, ELEMENT_STARTCOUNTER);*/
    pnw__H264_writebits_slice_header(mtx_hdr, elt_p, pSlHParams, bCabacEnabled, uiIdrPicId);

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


static void pnw__MPEG4_writebits_sequence_header(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p,
    IMG_BOOL __maybe_unused bBFrame,
    MPEG4_PROFILE_TYPE bProfile,
    IMG_UINT8 Profile_and_level_indication,
    FIXED_VOP_TIME_TYPE __maybe_unused sFixed_vop_time_increment,
    IMG_UINT32 Picture_Width_Pixels,
    IMG_UINT32 Picture_Height_Pixels,
    VBVPARAMS __maybe_unused * sVBVParams,
    IMG_UINT32 VopTimeResolution) /* Send NULL pointer if there are no VBVParams */
{
    /* Essential we insert the element before we try to fill it! */
    pnw__insert_element_token(mtx_hdr, elt_p, ELEMENT_STARTCODE_RAWDATA);

    /* visual_object_sequence_start_code        = 32 Bits       = 0x1B0 */
    pnw__write_upto32bits_elements(mtx_hdr, elt_p, 432, 32);

    /* profile_and_level_indication = 8 Bits = SP L0-L3 and SP L4-L5 are supported */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, Profile_and_level_indication, 8);

    /* visual_object_start_code = 32 Bits       = 0x1B5 */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 8);
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 8);
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 8);
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 181, 8);

    /* is_visual_object_identifier = 1 Bit      = 0 */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

    /* visual_object_type = 4 Bits      = Video ID = 1 */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 4);

    /* video_signal_type = 1 Bit                = 1 */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

    /* byte_aligned_bits = 2 Bits       = 01b (byte_aligned_bits is 2-bit stuffing bit field 01) */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 2);

    /* video_object_start_code = 32 Bits        = 0x100 One VO only in a Topaz video stream */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 8);
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 8);
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 8);
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 8);

    /* video_object_layer_start_code = 32 Bits  = 0x120 One VOL only in a Topaz stream */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 8);
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 8);
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 8);
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 32, 8);

    /* random_accessible_vol = 1 Bit            = 0 (P-Frame in GOP) */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

    if (bProfile == SP) {
        /* video_object_type_indication = 8 Bits        = 0x01 for SP */
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 8);
#ifndef MATCH_TO_ENC
        /* is_object_layer_identifier   = 1 Bit         = 0 for SP */
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);
#else
        /* to match the encoder */

        /* is_object_layer_identifier   = 1 Bit          */
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);

        /* video_object_layer_verid     = 4 Bits         */
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 4);

        /* video_object_layer_priority  = 3 Bits         */
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 3);  // 0 is reserved...
#endif
    } else {
        /* video_object_type_indication = 8 Bits        = 0x11 for ASP */
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, 3, 8);

        /* is_object_layer_identifier   = 1 Bit         = 1 for ASP */
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);

        /* video_object_layer_verid     = 4 Bits        = 5 is for ASP */
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, 5, 4);

        /* video_object_layer_priority  = 3 Bits        = 1 (Highest priority) */
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 3);
    }

    /* aspect_ratio_info                = 4 Bits        =0x1 (Square pixel) */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 4);
#ifndef MATCH_TO_ENC
    /* vol_control_parameters           = 1 Bit         = 1 (Always send VOL control parameters) */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);
#else
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

#endif

#ifndef MATCH_TO_ENC
    /* chroma_format                    = 2 Bits        = 01b (4:2:0) */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 2);

    /* low_delay = 1 Bit = 0 with B-frame and 1 without B-frame */
    if (bBFrame)
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);
    else
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);

    /* vbv_parameters                   = 1 Bit                 =0/1  */
    if (sVBVParams) {
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);

        /* For recording, only send vbv parameters in 1st sequence header.
         * For video phone, it should be sent more often, such as once per sequence
         */
        pnw__write_upto32bits_elements(
            mtx_hdr,
            elt_p,
            sVBVParams->First_half_bit_rate,
            15); /* first_half_bit_rate */

        pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);/* Marker Bit = 1 */
        pnw__write_upto32bits_elements(
            mtx_hdr,
            elt_p,
            sVBVParams->Latter_half_bit_rate,
            15); /* latter_half_bit_rate */

        pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);/* Marker Bit = 1 */
        pnw__write_upto32bits_elements(
            mtx_hdr,
            elt_p,
            sVBVParams->First_half_vbv_buffer_size,
            15);/* first_half_vbv_buffer_size */

        pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1); /* Marker Bit = 1 */
        pnw__write_upto32bits_elements(
            mtx_hdr, elt_p,
            sVBVParams->Latter_half_vbv_buffer_size,
            15); /* latter_half_vbv_buffer_size */
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);/* Marker Bit = 1 */
        pnw__write_upto32bits_elements(
            mtx_hdr,
            elt_p,
            sVBVParams->First_half_vbv_occupancy,
            15);  /* first_half_vbv_occupancy */
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);/* Marker Bit = 1 */
        pnw__write_upto32bits_elements(
            mtx_hdr,
            elt_p,
            sVBVParams->Latter_half_vbv_occupancy,
            15); /* latter_half_vbv_occupancy */
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);/* Marker Bit = 1 */
    } else
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1); /* No vbv parameters present */
#endif
    /* video_object_layer_shape = 2 Bits =      00b     Rectangular shape */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 2);
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1); /* Marker Bit = 1 */

    /* vop_time_increment_solution = 16 Bits */
    pnw__write_upto32bits_elements(mtx_hdr, elt_p, VopTimeResolution, 16);
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1); /* Marker Bit = 1 */

#ifndef MATCH_TO_ENC
    /* fixed_vop_rate = 1 Bits  = 1 Always fixed frame rate */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);

    /* fixed_vop_time_increment         = Variable number of bits based on the time increment resolution. */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, Bits2Code(VopTimeResolution - 1));
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1); /* Marker Bit = 1 */
#else
    /* fixed_vop_rate   = 1 Bits =      0 */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1); /* Marker Bit = 1 */

#endif
    /* video_object_layer_width = 13 Bits  Picture width in pixel units */
    pnw__write_upto32bits_elements(mtx_hdr, elt_p, Picture_Width_Pixels, 13);
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1); /* Marker Bit = 1 */

    /* video_object_layer_height = 13 Bits Picture height in pixel units */
    pnw__write_upto32bits_elements(mtx_hdr, elt_p, Picture_Height_Pixels, 13);
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1); /* Marker Bit = 1 */

    /* interlaced = 1 Bit = 0 Topaz only encodes progressive frames */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

    /* obmc_disable = 1 Bit = 1 No overlapped MC in Topaz */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);

    /* sprite_enable = 1 Bit = 0 Not use sprite in Topaz */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

    /* not_8_bit = 1    Bit = 0 8-bit video in Topaz */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

    /* quant_type = 1 Bit = 0 2nd quantization method in Topaz */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

    if (bProfile == ASP) {
        /* quarter_sample = 1 Bit = 0 No ¼-pel MC in Topaz */
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);
    }

    /* complexity_estimation_disable    = 1 Bit = 1 No complexity estimation in Topaz */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);
#ifndef MATCH_TO_ENC

    /* resync_marker_disable = 1 Bit = 0 Always enable resync marker in Topaz */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);
#else
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);
#endif
    /* data_partitioned = 1 Bit = 0 No data partitioning in Topaz */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

    if (bProfile == ASP) {
        /* newpred_enable = 1 Bit = 0 No newpred mode in SP/ASP */
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);
        /* reduced_vop_resolution_enable=1 Bit = 0      No reduced resolution frame in SP/ASP */
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);
    }

    /* scalability = 1 Bit = 0  No scalability in SP/ASP */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

    /* byte_aligned_bits */

    /* Tell MTX to insert the byte align field
     * (we don't know final stream size for alignment at this point)
     */
    pnw__insert_element_token(
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
static void pnw__MPEG4_writebits_VOP_header(
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
    pnw__insert_element_token(mtx_hdr, elt_p, ELEMENT_STARTCODE_RAWDATA);

    /* visual_object_sequence_start_code        = 32 Bits       = 0x1B6 */
    pnw__write_upto32bits_elements(mtx_hdr, elt_p, 438, 32);

    /* vop_coding_type  = 2 Bits = 0 for I-frame and 1 for P-frame */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, sVopCodingType, 2);
    bIsSyncPoint = (VOP_time_increment > 1) && ((VOP_time_increment) % VopTimeResolution == 0);

#ifndef MATCH_TO_ENC
    /* modulo_time_base = 1 Bit = 0 As at least  1 synchronization point (I-frame) per second in Topaz */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);
#else

    pnw__write_upto8bits_elements(mtx_hdr, elt_p, bIsSyncPoint  ? 2 : 0 , bIsSyncPoint ? 2 : 1);

#endif

    /* marker_bit = 1   Bits    = 1      */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);

#ifndef MATCH_TO_ENC
    /* vop_time_increment = Variable bits based on resolution
     *  = x Reset to 0 at I-frame and plus fixed_vop_time_increment each frame
     */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, VOP_time_increment, 5);
#else
    /* will chrash here... */
    pnw__write_upto8bits_elements(
        mtx_hdr, elt_p,
        (VOP_time_increment) % VopTimeResolution,
        Bits2Code(VopTimeResolution - 1));

#endif
    /* marker_bit = 1 Bit               = 1      */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);

    if (!bIsVOP_coded) {
        /* vop_coded    = 1 Bit         = 0 for skipped frame */
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

        /* byte_aligned_bits (skipped pictures are byte aligned) */
        /* Tell MTX to insert the byte align field (we don't know final stream size for alignment at this point)
         * End of VOP - skipped picture
         */
        pnw__insert_element_token(mtx_hdr, elt_p, ELEMENT_INSERTBYTEALIGN_MPG4);
    } else {
        /* vop_coded = 1 Bit            = 1 for normal coded frame */
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);

        if (sVopCodingType == P_FRAME) {
            /* vop_rounding_type = 1 Bit = 0 vop_rounding_type is 0 in Topaz */
            pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);
        }

        /* intra_dc_vlc_thr = 3 Bits = 0 Use intra DC VLC in Topaz */
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 3);

        /* vop_quant = 5 Bits   = x     5-bit frame Q_scale from rate control - GENERATED BY MTX */
        /* pnw__write_upto8bits_elements(mtx_hdr, elt_p, Frame_Q_scale, 5); */
        pnw__insert_element_token(mtx_hdr, elt_p, ELEMENT_FRAMEQSCALE);

        if (sVopCodingType == P_FRAME) {
            /* vop_fcode_forward = 3 bits       = 2 for +/-32 and 3 for +/-64 search range  */
            pnw__insert_element_token(mtx_hdr, elt_p, ELEMENT_RAWDATA);
            pnw__write_upto8bits_elements(mtx_hdr, elt_p, sSearch_range, 3);
        }

        /*
        **** THE FINAL PART OF VOP STRUCTURE CAN'T BE GENERATED HERE
        pnw__insert_element_token(mtx_hdr, elt_p, ELEMENT_RAWDATA);
        video_packet_data ( )                   = 1st  VP that doesn’t have the VP header

        while (nextbits_bytealigned ( ) == resync_marker)
        {
        video_packet _header( )
        video_packet _data( )                   All MB in the slice
        }
        */
    }
}

#if PSB_MFLD_DUMMY_CODE
/*
 * Intermediary functions to build H263 headers
 */
static void H263_writebits_VideoSequenceHeader(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p,
    IMG_UINT8 Profile_and_level_indication)
{
    /* Essential we insert the element before we try to fill it! */
    pnw__insert_element_token(mtx_hdr, elt_p, ELEMENT_STARTCODE_RAWDATA);

    /* visual_object_sequence_start_code        = 32 Bits       = 0x1B0 */
    pnw__write_upto32bits_elements(mtx_hdr, elt_p, 432, 32);

    /* profile_and_level_indication = 8 Bits =  x SP L0-L3 and SP L4-L5 are supported */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, Profile_and_level_indication, 8);

    /* visual_object_start_code = 32 Bits       = 0x1B5 */

    /* 437 too large for the   pnw__write_upto32bits_elements function */
    pnw__write_upto32bits_elements(mtx_hdr, elt_p, 437, 32);

    /* is_visual_object_identifier = 1 Bit              = 0 */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

    /* is_visual_object_type    = 4 Bits        = 1 Video ID */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 4);

    /* video_signal_type = 1 Bit                = 0      */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

    /* byte_aligned_bits = 2 Bits = 01b byte_aligned_bits is 2-bit stuffing bit field 01 */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 2);

    /* video_object_start_code  =32 Bits        = 0x100 One VO only in a Topaz video stream */
    pnw__write_upto32bits_elements(mtx_hdr, elt_p, 256, 32);

    return;
}
#endif


static void H263_writebits_VideoPictureHeader(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p,
    IMG_UINT8 Temporal_Ref,
    H263_PICTURE_CODING_TYPE PictureCodingType,
    //IMG_UINT8 Q_Scale,
    H263_SOURCE_FORMAT_TYPE SourceFormatType,
    IMG_UINT8 FrameRate,
    IMG_UINT32 PictureWidth,
    IMG_UINT32 PictureHeight)
{
    IMG_UINT8 UFEP;
    IMG_UINT8 OCPCF = 0;

    /* Essential we insert the element before we try to fill it! */
    pnw__insert_element_token(mtx_hdr, elt_p, ELEMENT_STARTCODE_RAWDATA);

    /* short_video_start_marker = 22 Bits       = 0x20 Picture start code */
    pnw__write_upto32bits_elements(mtx_hdr, elt_p, 32, 22);

    /* temporal_reference = 8 Bits      = 0-255 Each picture increased by 1 */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, Temporal_Ref, 8);

    /* marker_bit = 1 Bit = 1    */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);

    /* zero_bit = 1 Bits        = 0      */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

    /* split_screen_indicator   = 1     Bits    = 0     No direct effect on encoding of picture */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

    /* document_camera_indicator= 1     Bits    = 0     No direct effect on encoding of picture */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

    /* full_picture_freeze_release=1 Bits       = 0     No direct effect on encoding of picture */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);

    /* source_format                            = 3     Bits    = 1-4   See note */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, SourceFormatType, 3);

    /*Write optional Custom Picture Clock Frequency(OCPCF)*/
    if (FrameRate == 30 || FrameRate == 0/* unspecified */) {
        OCPCF = 0; // 0 for CIF PCF
    } else {
        OCPCF = 1; //1 for Custom PCF
    }

    if (SourceFormatType != 7) {
        // picture_coding_type          = 1 Bit         = 0/1   0 for I-frame and 1 for P-frame
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, PictureCodingType, 1);
        // four_reserved_zero_bits      = 4 Bits        = 0
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 4);
    } else {
        static unsigned char  RTYPE = 0;

        // if I- Frame set Update Full Extended PTYPE to true
        if ((PictureCodingType == I_FRAME) || (SourceFormatType == 7) || OCPCF) {
            UFEP = 1;
        } else {
            UFEP = 0;
        }
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, UFEP, 3);
        if (UFEP == 1) {
            pnw__write_upto8bits_elements(mtx_hdr, elt_p, 6, 3);
            pnw__write_upto8bits_elements(mtx_hdr, elt_p, OCPCF , 1);

            /* 10 reserve bits ( Optional support for the encoding). All are OFF(0).
             *  - Optional Unrestricted Motion Vector (UMV)
             *  - Optional Syntax-based Arithmetic Coding (SAC)
             *  - Optional Advanced Prediction (AP) mode
             *  - Optional Advanced INTRA Coding (AIC) mode
             *  - Optional Deblocking Filter (DF) mode
             *  - Optional Slice Structured (SS) mode
             *  - Optional Reference Picture Selection(RPS) mode.
             *  - Optional Independent Segment Decoding (ISD) mode
             *  - Optional Alternative INTER VLC (AIV) mode
             *  - Optional Modified Quantization (MQ) mode */

            pnw__write_upto32bits_elements(mtx_hdr, elt_p, 0, 10);
            // 10 reserve bits
            pnw__write_upto8bits_elements(mtx_hdr, elt_p, 8, 4);
            // 4 reserve bits
        }
        // picture_coding_type          = 1 Bit         = 0/1   0 for I-frame and 1 for P-frame
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, PictureCodingType, 3);

        pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 2);
        // two_reserve_bits,      rounding_type,       two_reserve_bits       marker_bit       CPM
        // Rounding Type (RTYPE) (1 for P Picture, 0 for all other Picture frames.
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, RTYPE, 1);
        //2 reserve bits
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 2);
        //   - 1 (ON) to prevent start code emulation.
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1 , 1);
        // CPM immediately follows the PPTYPE part of the header.
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0 , 1);


        if (UFEP == 1) {
            IMG_UINT16 ui16PWI, ui16PHI;
            pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 4);
            // aspect ratio
            //PictureWidth--;
            //pnw__write_upto8bits_elements(mtx_hdr, elt_p, (IMG_UINT8)(PictureWidth >> 8), 1);
            //pnw__write_upto8bits_elements(mtx_hdr, elt_p, (IMG_UINT8)(PictureWidth & 0xFF), 8);
            //Width = (PWI-1)*4, Height = PHI*4, see H263 spec 5.1.5
            ui16PWI = (PictureWidth >> 2) - 1;
            pnw__write_upto32bits_elements(mtx_hdr, elt_p, (IMG_UINT8)ui16PWI, 9);

            pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);
            // marker_bit                               = 1 Bit         = 1
            //pnw__write_upto8bits_elements(mtx_hdr, elt_p, (IMG_UINT8)(PictureHeight >> 8), 1);
            //pnw__write_upto8bits_elements(mtx_hdr, elt_p, (IMG_UINT8)(PictureHeight & 0xFF), 8);

            ui16PHI = PictureHeight >> 2;
            pnw__write_upto32bits_elements(mtx_hdr, elt_p, (IMG_UINT8)ui16PHI, 9);
            // good up to that point
            //  pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);
            // marker_bit                               = 1 Bit         = 1
            // just checking
            if (OCPCF == 1) {
                //IMG_UINT8 CPCFC;
                //CPCFC = (IMG_UINT8)(1800/(IMG_UINT16)FrameRate);
                /* you can use the table for division */
                //CPCFC <<= 1; /* for Clock Conversion Code */
                pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1, 1);
                // Clock Divisor : 7 bits The natural binary representation of the value of the clock divisor.
                pnw__write_upto8bits_elements(mtx_hdr, elt_p, 1800000 / (FrameRate * 1000), 7);
            }
        }
        if (OCPCF == 1) {
            IMG_UINT8 ui8ETR; // extended Temporal reference
            // Two MSBs of 10 bit temporal_reference : value 0
            ui8ETR = Temporal_Ref >> 8;

            pnw__write_upto8bits_elements(mtx_hdr, elt_p, ui8ETR, 2);
            /* Two MSBs of temporal_reference */
        }
    }
    // vop_quant                                = 5 Bits        = x     5-bit frame Q_scale from rate control - GENERATED BY MTX
    //pnw__write_upto8bits_elements(mtx_hdr, elt_p, ui8Q_Scale, 5);
    pnw__insert_element_token(mtx_hdr, elt_p, ELEMENT_FRAMEQSCALE); // Insert token to tell MTX to insert rate-control value (QScale is sent as an argument in MTX_Send_Elements_To_VLC(&MTX_Header, FrameQScale))
    pnw__insert_element_token(mtx_hdr, elt_p, ELEMENT_RAWDATA);
    // zero_bit                                 = 1 Bit         = 0
    // pei                                              = 1 Bit         = 0     No direct effect on encoding of picture
    if (SourceFormatType != 7) {
        pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);
    }

    pnw__write_upto8bits_elements(mtx_hdr, elt_p, 0, 1);
    // FOLLOWING SECTION CAN'T BE GENERATED HERE
    //gob_data( )
    //for(i=1; i<num_gob_in_picture; i++) {
    //      gob_header( )
    //      gob_data( )
    // }
    return;
}

static void H263_writebits_GOBSliceHeader(
    MTX_HEADER_PARAMS *mtx_hdr,
    MTX_HEADER_ELEMENT **elt_p,
    IMG_UINT8 GOBNumber,
    IMG_UINT8 GOBFrameId)
{
    /* Essential we insert the element before we try to fill it! */
    pnw__insert_element_token(mtx_hdr, elt_p, ELEMENT_STARTCODE_RAWDATA);

    /* gob_resync_marker                = 17            = 0x1 */
    pnw__write_upto32bits_elements(mtx_hdr, elt_p, 1, 17);

    /* gob_number = 5   = 0-17  It is gob number in a picture */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, GOBNumber, 5);

    /* gob_frame_id     = 2 = 0-3       See note */
    pnw__write_upto8bits_elements(mtx_hdr, elt_p, GOBFrameId, 2);

    /* quant_scale      = 5     = 1-32  gob (Slice) Q_scale  */
    /* pnw__write_upto8bits_elements(mtx_hdr, elt_p, GOB_Q_Scale, 5); */

    /* Insert token to tell MTX to insert rate-control value
     *  (QScale is sent as an argument in MTX_Send_Elements_To_VLC(&MTX_Header, SliceQScale))
     */
    pnw__insert_element_token(mtx_hdr, elt_p, ELEMENT_SLICEQSCALE);
    return;
}

/*
 * High level functions to call when a H263 header is required - HOST ROUTINES
 */

// SEI_INSERTION
#if PSB_MFLD_DUMMY_CODE
static void pnw__H264_writebits_AUD_header(
    MTX_HEADER_PARAMS *pMTX_Header,
    MTX_HEADER_ELEMENT **aui32ElementPointers)
{
    // Essential we insert the element before we try to fill it!
    pnw__insert_element_token(pMTX_Header,
                              aui32ElementPointers,
                              ELEMENT_STARTCODE_RAWDATA);

    pnw__H264_writebits_startcode_prefix_element(pMTX_Header,
            aui32ElementPointers, 4); // 00 00 00 01 start code prefix

    pnw__write_upto8bits_elements(pMTX_Header,
                                  aui32ElementPointers,
                                  9,
                                  8); // AUD nal_unit_type = 09

    // primary_pic_type  u(3) 0=I slice, 1=P or I slice, 2=P,B or I slice
    pnw__write_upto8bits_elements(pMTX_Header,
                                  aui32ElementPointers,
                                  2,
                                  3);

    pnw__write_upto8bits_elements(pMTX_Header,
                                  aui32ElementPointers,
                                  1 << 4, 5); // rbsp_trailing_bits

    // Write terminator
    pnw__write_upto8bits_elements(pMTX_Header,
                                  aui32ElementPointers, 0x80, 8);
    return;
}
#endif
#define SEI_NOT_USE_TOKEN_ALIGN

static void pnw__H264_writebits_SEI_buffering_period_header(
    MTX_HEADER_PARAMS *pMTX_Header,
    MTX_HEADER_ELEMENT **aui32ElementPointers,
    IMG_UINT8 ui8NalHrdBpPresentFlag,
    IMG_UINT8 ui8nal_cpb_cnt_minus1,
    IMG_UINT8 ui8nal_initial_cpb_removal_delay_length,
    IMG_UINT32 ui32nal_initial_cpb_removal_delay,
    IMG_UINT32 ui32nal_initial_cpb_removal_delay_offset,
    IMG_UINT8 ui8VclHrdBpPresentFlag,
    IMG_UINT8 ui8vcl_cpb_cnt_minus1,
    IMG_UINT32 ui32vcl_initial_cpb_removal_delay,
    IMG_UINT32 ui32vcl_initial_cpb_removal_delay_offset)
{
    IMG_UINT8 ui8SchedSelIdx;
    IMG_UINT8 ui8PayloadSizeBits;
#ifdef SEI_NOT_USE_TOKEN_ALIGN
    IMG_UINT8 ui8Pad;
#endif
    // Essential we insert the element before we try to fill it!
    pnw__insert_element_token(pMTX_Header,
                              aui32ElementPointers,
                              ELEMENT_STARTCODE_RAWDATA);

    pnw__H264_writebits_startcode_prefix_element(pMTX_Header,
            aui32ElementPointers,
            4); // 00 00 01 start code prefix

    pnw__write_upto8bits_elements(pMTX_Header,
                                  aui32ElementPointers,
                                  6, 8); // nal_unit_type = 06 (SEI Message)

    pnw__write_upto8bits_elements(pMTX_Header,
                                  aui32ElementPointers,
                                  0, 8); // SEI payload type (buffering period)

    ui8PayloadSizeBits = 1; // seq_parameter_set_id bitsize = 1
    if (ui8NalHrdBpPresentFlag)
        ui8PayloadSizeBits += ((ui8nal_cpb_cnt_minus1 + 1)
                               * ui8nal_initial_cpb_removal_delay_length * 2);
    if (ui8VclHrdBpPresentFlag)
        ui8PayloadSizeBits += ((ui8vcl_cpb_cnt_minus1 + 1)
                               * ui8nal_initial_cpb_removal_delay_length * 2);

    pnw__write_upto8bits_elements(pMTX_Header,
                                  aui32ElementPointers,
                                  ((ui8PayloadSizeBits + 7) / 8),
                                  8);
    // SEI payload size = No of bytes required for SEI payload
    // (including seq_parameter_set_id)

    //seq_parameter_set_id      ue(v) = 0 default? = 1 (binary)
    //= sequence parameter set containing HRD attributes
    pnw__generate_ue(pMTX_Header, aui32ElementPointers, 0);

    if (ui8NalHrdBpPresentFlag) {
        for (ui8SchedSelIdx = 0; ui8SchedSelIdx <= ui8nal_cpb_cnt_minus1; ui8SchedSelIdx++) {
            // ui32nal_initial_cpb_removal_delay = delay between time of arrival in CODED PICTURE BUFFER of coded data of this access
            // unit and time of removal from CODED PICTURE BUFFER of the coded data of the same access unit.
            // Delay is based on the time taken for a 90 kHz clock.
            // Range >0 and < 90000 * (CPBsize / BitRate)
            // For the 1st buffering period after HARDWARE REFERENCE DECODER initialisation.
            // pnw__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_NAL_INIT_CPB_REMOVAL_DELAY); // Eventually use this if firmware value required

            pnw__write_upto32bits_elements(pMTX_Header, aui32ElementPointers,
		    ui32nal_initial_cpb_removal_delay,
		    ui8nal_initial_cpb_removal_delay_length);

/*
            pnw__insert_element_token(pMTX_Header,
                                      aui32ElementPointers,
                                      BPH_SEI_NAL_INITIAL_CPB_REMOVAL_DELAY);*/

            // ui32nal_initial_cpb_removal_delay_offset = used for the SchedSelIdx-th CPB in combination with the cpb_removal_delay to
            // specify the initial delivery time of coded access units to the CODED PICTURE BUFFER initial_cpb_removal_delay_offset
            // Delay is based on the time taken for a 90 kHz clock.
            // NOT USED BY DECODERS and is needed only for the delivery scheduler (HSS) specified in Annex C

            pnw__write_upto32bits_elements(pMTX_Header, aui32ElementPointers,
		    ui32nal_initial_cpb_removal_delay_offset,
		    ui8nal_initial_cpb_removal_delay_length);

            /*pnw__insert_element_token(pMTX_Header,
                                      aui32ElementPointers,
                                      BPH_SEI_NAL_INITIAL_CPB_REMOVAL_DELAY_OFFSET);*/

        }
    }
    if (ui8VclHrdBpPresentFlag) {
        for (ui8SchedSelIdx = 0; ui8SchedSelIdx <= ui8vcl_cpb_cnt_minus1; ui8SchedSelIdx++) {
            pnw__insert_element_token(pMTX_Header,
                                      aui32ElementPointers,
                                      ELEMENT_STARTCODE_RAWDATA);
            // pnw__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_VCL_INIT_CPB_REMOVAL_DELAY); // Eventually use this if firmware value required
            pnw__write_upto32bits_elements(pMTX_Header,
                                           aui32ElementPointers,
                                           ui32vcl_initial_cpb_removal_delay,
                                           ui8nal_initial_cpb_removal_delay_length);
            // pnw__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_VCL_INIT_CPB_REMOVAL_DELAY_CPB); // Eventually use this if firmware value required
            pnw__write_upto32bits_elements(pMTX_Header,
                                           aui32ElementPointers,
                                           ui32vcl_initial_cpb_removal_delay_offset,
                                           ui8nal_initial_cpb_removal_delay_length);
        }
    }

    // Pad to end of byte
#ifdef SEI_NOT_USE_TOKEN_ALIGN
    if (!ui8VclHrdBpPresentFlag)
        pnw__insert_element_token(pMTX_Header,
                                  aui32ElementPointers,
                                  ELEMENT_STARTCODE_RAWDATA);
    ui8Pad = (ui8PayloadSizeBits + 7) / 8;
    ui8Pad = (ui8Pad * 8) - ui8PayloadSizeBits;
    if (ui8Pad > 0)
        pnw__write_upto8bits_elements(pMTX_Header,
                                      aui32ElementPointers,
                                      1 << (ui8Pad - 1),
                                      ui8Pad); // SEI payload type (buffering period)
#else
    pnw__write_upto8bits_elements(pMTX_Header,
                                  aui32ElementPointers,
                                  1, 1); // rbsp_trailing_bits

    pnw__insert_element_token(pMTX_Header,
                              aui32ElementPointers,
                              ELEMENT_INSERTBYTEALIGN_H264);
    // Tell MTX to insert the byte align field
    pnw__insert_element_token(pMTX_Header,
                              aui32ElementPointers,
                              ELEMENT_STARTCODE_RAWDATA);
#endif

    // Write terminator
    pnw__write_upto8bits_elements(pMTX_Header, aui32ElementPointers, 0x80, 8);

    return;
}

#define SEI_HOSTCALC_CPB_DPB

static void pnw__H264_writebits_SEI_picture_timing_header(
    MTX_HEADER_PARAMS *pMTX_Header, MTX_HEADER_ELEMENT **aui32ElementPointers,
    IMG_UINT8 ui8CpbDpbDelaysPresentFlag,
    IMG_UINT32 ui32cpb_removal_delay_length_minus1,
    IMG_UINT32 ui32dpb_output_delay_length_minus1,
    IMG_UINT32 ui32cpb_removal_delay,
    IMG_UINT32 ui32dpb_output_delay,
    IMG_UINT8 ui8pic_struct_present_flag,
    IMG_UINT8 __maybe_unused ui8pic_struct,
    IMG_UINT8 __maybe_unused ui8NumClockTS,
    IMG_UINT8 __maybe_unused * aui8clock_timestamp_flag,
    IMG_UINT8 __maybe_unused ui8full_timestamp_flag,
    IMG_UINT8 __maybe_unused ui8seconds_flag,
    IMG_UINT8 __maybe_unused ui8minutes_flag,
    IMG_UINT8 __maybe_unused ui8hours_flag,
    IMG_UINT8 __maybe_unused ui8seconds_value,
    IMG_UINT8 __maybe_unused ui8minutes_value,
    IMG_UINT8 __maybe_unused ui8hours_value,
    IMG_UINT8 __maybe_unused ui8ct_type,
    IMG_UINT8 __maybe_unused ui8nuit_field_based_flag,
    IMG_UINT8 __maybe_unused ui8counting_type,
    IMG_UINT8 __maybe_unused ui8discontinuity_flag,
    IMG_UINT8 __maybe_unused ui8cnt_dropped_flag,
    IMG_UINT8 __maybe_unused ui8n_frames,
    IMG_UINT8 __maybe_unused ui8time_offset_length,
    IMG_UINT32 __maybe_unused i32time_offset)
{
    IMG_UINT8 ui8PayloadSizeBits, ui8Tmp;
#ifdef SEI_NOT_USE_TOKEN_ALIGN
    IMG_UINT8 ui8Pad;
#endif

    // Essential we insert the element before we try to fill it!
    pnw__insert_element_token(pMTX_Header,
                              aui32ElementPointers,
                              ELEMENT_STARTCODE_RAWDATA);

    pnw__H264_writebits_startcode_prefix_element(pMTX_Header,
            aui32ElementPointers,
            4); // 00 00 01 start code prefix

    pnw__write_upto8bits_elements(pMTX_Header,
                                  aui32ElementPointers,
                                  6, 8); // nal_unit_type = 06 (SEI Message)

    pnw__write_upto8bits_elements(pMTX_Header,
                                  aui32ElementPointers,
                                  1, 8); // SEI payload type (picture timing)


    // Precalculate the payload bit size
    ui8PayloadSizeBits = 0;
    if (ui8CpbDpbDelaysPresentFlag)
        ui8PayloadSizeBits += ui32cpb_removal_delay_length_minus1
                              + 1 + ui32dpb_output_delay_length_minus1 + 1;

#if 0
    if (ui8pic_struct_present_flag) {
        ui8PayloadSizeBits += 4;
        for (ui8Tmp = 0; ui8Tmp < ui8NumClockTS ; ui8Tmp++) {
            ui8PayloadSizeBits += 1;

            if (aui8clock_timestamp_flag[ui8Tmp]) {
                ui8PayloadSizeBits += 2 + 1 + 5 + 1 + 1 + 1 + 8;
                if (ui8full_timestamp_flag)
                    ui8PayloadSizeBits += 6 + 6 + 5;
                else {
                    ui8PayloadSizeBits += 1;
                    if (ui8seconds_flag) {
                        ui8PayloadSizeBits += 6 + 1;
                        if (ui8minutes_flag) {
                            ui8PayloadSizeBits += 6 + 1;
                            if (ui8hours_flag)
                                ui8PayloadSizeBits += 5;
                        }
                    }
                }

                if (ui8time_offset_length > 0)
                    ui8PayloadSizeBits += ui8time_offset_length;
            }
        }
    }
#endif

    pnw__write_upto8bits_elements(pMTX_Header,
                                  aui32ElementPointers,
                                  ((ui8PayloadSizeBits + 7) / 8), 8);
    // SEI payload size = No of bytes required for SEI payload (including seq_parameter_set_id)


    if (ui8CpbDpbDelaysPresentFlag) {
        //SEI_INSERTION
#ifdef SEI_HOSTCALC_CPB_DPB
        pnw__write_upto32bits_elements(pMTX_Header,
                                       aui32ElementPointers,
                                       ui32cpb_removal_delay,
                                       ui32cpb_removal_delay_length_minus1 + 1); // cpb_removal_delay
        pnw__write_upto32bits_elements(pMTX_Header,
                                       aui32ElementPointers,
                                       ui32dpb_output_delay,
                                       ui32dpb_output_delay_length_minus1 + 1); // dpb_output_delay
#else
        pnw__insert_element_token(pMTX_Header,
                                  aui32ElementPointers,
                                  PTH_SEI_NAL_CPB_REMOVAL_DELAY);
        pnw__insert_element_token(pMTX_Header,
                                  aui32ElementPointers,
                                  PTH_SEI_NAL_DPB_OUTPUT_DELAY);
#endif
    }

#if 0
    if (ui8pic_struct_present_flag) {
        pnw__insert_element_token(pMTX_Header,
                                  aui32ElementPointers,
                                  ELEMENT_STARTCODE_RAWDATA);
        pnw__write_upto8bits_elements(pMTX_Header,
                                      aui32ElementPointers,
                                      ui8pic_struct, 4); // See TRM able D 1 – Interpretation of pic_struct

        for (ui8Tmp = 0; ui8Tmp < ui8NumClockTS ; ui8Tmp++) {
            pnw__write_upto8bits_elements(pMTX_Header,
                                          aui32ElementPointers,
                                          aui8clock_timestamp_flag[ui8Tmp], 1);

            if (aui8clock_timestamp_flag[ui8Tmp]) {
                pnw__write_upto8bits_elements(pMTX_Header,
                                              aui32ElementPointers,
                                              ui8ct_type, 2);
                // (2=Unknown) See TRM Table D 2 – Mapping of ct_type to source picture scan
                pnw__write_upto8bits_elements(pMTX_Header,
                                              aui32ElementPointers,
                                              ui8nuit_field_based_flag, 1);
                pnw__write_upto8bits_elements(pMTX_Header,
                                              aui32ElementPointers,
                                              ui8counting_type, 5);
                // See TRM Table D 3 – Definition of counting_type values
                pnw__write_upto8bits_elements(pMTX_Header,
                                              aui32ElementPointers,
                                              ui8full_timestamp_flag, 1);
                pnw__write_upto8bits_elements(pMTX_Header,
                                              aui32ElementPointers,
                                              ui8discontinuity_flag, 1);
                pnw__write_upto8bits_elements(pMTX_Header,
                                              aui32ElementPointers,
                                              ui8cnt_dropped_flag, 1);
                pnw__write_upto8bits_elements(pMTX_Header,
                                              aui32ElementPointers,
                                              ui8n_frames, 8);

                if (ui8full_timestamp_flag) {
                    pnw__write_upto8bits_elements(pMTX_Header,
                                                  aui32ElementPointers,
                                                  ui8seconds_value, 6); // 0 - 59
                    pnw__write_upto8bits_elements(pMTX_Header,
                                                  aui32ElementPointers,
                                                  ui8minutes_value, 6); // 0 - 59
                    pnw__write_upto8bits_elements(pMTX_Header,
                                                  aui32ElementPointers,
                                                  ui8hours_value, 5); // 0 - 23
                } else {
                    pnw__write_upto8bits_elements(pMTX_Header,
                                                  aui32ElementPointers,
                                                  ui8seconds_flag, 1);

                    if (ui8seconds_flag) {
                        pnw__write_upto8bits_elements(pMTX_Header,
                                                      aui32ElementPointers,
                                                      ui8seconds_value, 6); // 0 - 59
                        pnw__write_upto8bits_elements(pMTX_Header,
                                                      aui32ElementPointers,
                                                      ui8minutes_flag, 1);

                        if (ui8minutes_flag) {
                            pnw__write_upto8bits_elements(pMTX_Header,
                                                          aui32ElementPointers,
                                                          ui8minutes_value, 6); // 0 - 59
                            pnw__write_upto8bits_elements(pMTX_Header,
                                                          aui32ElementPointers,
                                                          ui8hours_flag, 1);

                            if (ui8hours_flag)
                                pnw__write_upto8bits_elements(pMTX_Header,
                                                              aui32ElementPointers,
                                                              ui8hours_value, 5); // 0 - 23
                        }
                    }
                }

                if (ui8time_offset_length > 0) {
                    // Two's complement storage : If time_offset<0 = ((2 ^ v) + time_offset)
                    if ((int)i32time_offset < 0)
                        pnw__write_upto32bits_elements(pMTX_Header,
                                                       aui32ElementPointers,
                                                       (IMG_UINT32)((2 ^ ui8time_offset_length) + i32time_offset),
                                                       ui8time_offset_length);
                    else
                        pnw__write_upto32bits_elements(pMTX_Header,
                                                       aui32ElementPointers,
                                                       (IMG_UINT32) i32time_offset,
                                                       ui8time_offset_length);
                }
            }
        }
    }
#endif

#ifdef SEI_NOT_USE_TOKEN_ALIGN
    // Pad to end of byte
    if (!ui8pic_struct_present_flag)
        pnw__insert_element_token(pMTX_Header,
                                  aui32ElementPointers,
                                  ELEMENT_STARTCODE_RAWDATA);
    ui8Pad = (ui8PayloadSizeBits + 7) / 8;
    ui8Pad = (ui8Pad * 8) - ui8PayloadSizeBits;
    if (ui8Pad > 0)
        pnw__write_upto8bits_elements(pMTX_Header,
                                      aui32ElementPointers,
                                      1 << (ui8Pad - 1),
                                      ui8Pad); // SEI payload type (buffering period)
#else
    pnw__insert_element_token(pMTX_Header,
                              aui32ElementPointers,
                              ELEMENT_INSERTBYTEALIGN_H264); // Tell MTX to insert the byte align field
    pnw__insert_element_token(pMTX_Header,
                              aui32ElementPointers,
                              ELEMENT_STARTCODE_RAWDATA);
#endif

    // Write terminator
    pnw__write_upto8bits_elements(pMTX_Header,
                                  aui32ElementPointers,
                                  0x80, 8);
    return;
}

static void pnw__H264_writebits_SEI_FPA_header(
        MTX_HEADER_PARAMS *pMTX_Header, MTX_HEADER_ELEMENT **aui32ElementPointers,
        char* sei_data_buf, IMG_UINT32 data_size)
{
    IMG_UINT8 i;

    // Essential we insert the element before we try to fill it!
    pnw__insert_element_token(pMTX_Header,
            aui32ElementPointers,
            ELEMENT_STARTCODE_RAWDATA);

    pnw__H264_writebits_startcode_prefix_element(pMTX_Header,
            aui32ElementPointers,
            4); // 00 00 01 start code prefix

    for (i = 0; i < data_size; i++)
        pnw__write_upto8bits_elements(pMTX_Header,
                aui32ElementPointers,
                sei_data_buf[i], 8); //sei_data_buf (SEI Message)
    // Tell MTX to insert the byte align field (we don't know final stream size for alignment at this point)
    //pnw__insert_element_token(pMTX_Header, aui32ElementPointers, ELEMENT_INSERTBYTEALIGN_H264);

    return;
}


#if PSB_MFLD_DUMMY_CODE
void pnw__H264_prepare_AUD_header(MTX_HEADER_PARAMS * pMTX_Header)
{
    // Essential we initialise our header structures before building
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *aui32ElementPointers[MAXNUMBERELEMENTS];
    pMTX_Header->Elements = ELEMENTS_EMPTY;
    This_Element = (MTX_HEADER_ELEMENT *) pMTX_Header->asElementStream;
    aui32ElementPointers[0] = This_Element;

    pnw__H264_writebits_AUD_header(pMTX_Header, aui32ElementPointers);

    pMTX_Header->Elements++; //Has been used as an index, so need to add 1 for a valid element count
}
#endif

void pnw__H264_prepare_SEI_buffering_period_header(
    MTX_HEADER_PARAMS * pMTX_Header,
    IMG_UINT8 ui8NalHrdBpPresentFlag,
    IMG_UINT8 ui8nal_cpb_cnt_minus1,
    IMG_UINT8 ui8nal_initial_cpb_removal_delay_length,
    IMG_UINT32 ui32nal_initial_cpb_removal_delay,
    IMG_UINT32 ui32nal_initial_cpb_removal_delay_offset,
    IMG_UINT8 ui8VclHrdBpPresentFlag,
    IMG_UINT8 ui8vcl_cpb_cnt_minus1,
    IMG_UINT32 ui32vcl_initial_cpb_removal_delay,
    IMG_UINT32 ui32vcl_initial_cpb_removal_delay_offset)
{
    // Essential we initialise our header structures before building
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *aui32ElementPointers[MAXNUMBERELEMENTS];
    pMTX_Header->Elements = ELEMENTS_EMPTY;
    This_Element = (MTX_HEADER_ELEMENT *) pMTX_Header->asElementStream;
    aui32ElementPointers[0] = This_Element;

    pnw__H264_writebits_SEI_buffering_period_header(
        pMTX_Header, aui32ElementPointers,
        ui8NalHrdBpPresentFlag,
        ui8nal_cpb_cnt_minus1,
        ui8nal_initial_cpb_removal_delay_length,
        ui32nal_initial_cpb_removal_delay,
        ui32nal_initial_cpb_removal_delay_offset,
        ui8VclHrdBpPresentFlag,
        ui8vcl_cpb_cnt_minus1,
        ui32vcl_initial_cpb_removal_delay,
        ui32vcl_initial_cpb_removal_delay_offset);

    pMTX_Header->Elements++;
    //Has been used as an index, so need to add 1 for a valid element count
    return;
}

void pnw__H264_prepare_SEI_picture_timing_header(
    MTX_HEADER_PARAMS * pMTX_Header,
    IMG_UINT8 ui8CpbDpbDelaysPresentFlag,
    IMG_UINT32 ui32cpb_removal_delay_length_minus1,
    IMG_UINT32 ui32dpb_output_delay_length_minus1,
    IMG_UINT32 ui32cpb_removal_delay,
    IMG_UINT32 ui32dpb_output_delay,
    IMG_UINT8 ui8pic_struct_present_flag,
    IMG_UINT8 ui8pic_struct,
    IMG_UINT8 ui8NumClockTS,
    IMG_UINT8 *aui8clock_timestamp_flag,
    IMG_UINT8 ui8full_timestamp_flag,
    IMG_UINT8 ui8seconds_flag,
    IMG_UINT8 ui8minutes_flag,
    IMG_UINT8 ui8hours_flag,
    IMG_UINT8 ui8seconds_value,
    IMG_UINT8 ui8minutes_value,
    IMG_UINT8 ui8hours_value,
    IMG_UINT8 ui8ct_type,
    IMG_UINT8 ui8nuit_field_based_flag,
    IMG_UINT8 ui8counting_type,
    IMG_UINT8 ui8discontinuity_flag,
    IMG_UINT8 ui8cnt_dropped_flag,
    IMG_UINT8 ui8n_frames,
    IMG_UINT8 ui8time_offset_length,
    IMG_INT32 i32time_offset)
{
    // Essential we initialise our header structures before building
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *aui32ElementPointers[MAXNUMBERELEMENTS];
    pMTX_Header->Elements = ELEMENTS_EMPTY;
    This_Element = (MTX_HEADER_ELEMENT *) pMTX_Header->asElementStream;
    aui32ElementPointers[0] = This_Element;

    pnw__H264_writebits_SEI_picture_timing_header(
        pMTX_Header, aui32ElementPointers,
        ui8CpbDpbDelaysPresentFlag,
        ui32cpb_removal_delay_length_minus1,
        ui32dpb_output_delay_length_minus1,
        ui32cpb_removal_delay,
        ui32dpb_output_delay,
        ui8pic_struct_present_flag,
        ui8pic_struct,
        ui8NumClockTS,
        aui8clock_timestamp_flag,
        ui8full_timestamp_flag,
        ui8seconds_flag,
        ui8minutes_flag,
        ui8hours_flag,
        ui8seconds_value,
        ui8minutes_value,
        ui8hours_value,
        ui8ct_type,
        ui8nuit_field_based_flag,
        ui8counting_type,
        ui8discontinuity_flag,
        ui8cnt_dropped_flag,
        ui8n_frames,
        ui8time_offset_length,
        i32time_offset);

    pMTX_Header->Elements++;
    //Has been used as an index, so need to add 1 for a valid element count
    return;
}

#if PSB_MFLD_DUMMY_CODE
void pnw__H264_prepare_SEI_FPA_header(
        MTX_HEADER_PARAMS * pMTX_Header,
        char* sei_data_buf,
        IMG_UINT32 data_size)
{
    // Essential we initialise our header structures before building
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *aui32ElementPointers[MAXNUMBERELEMENTS];
    pMTX_Header->Elements = ELEMENTS_EMPTY;
    This_Element = (MTX_HEADER_ELEMENT *) pMTX_Header->asElementStream;
    aui32ElementPointers[0] = This_Element;

    pnw__H264_writebits_SEI_FPA_header(
            pMTX_Header, aui32ElementPointers,
            sei_data_buf, data_size);

    pMTX_Header->Elements++;
    //Has been used as an index, so need to add 1 for a valid element count
    return;
}
#endif


void pnw__H264_prepare_sequence_header(
        unsigned char *pHeaderMemory,
        IMG_UINT32 uiPicWidthInMbs,
        IMG_UINT32 uiPicHeightInMbs,
        IMG_BOOL VUI_present, H264_VUI_PARAMS *VUI_params,
        H264_CROP_PARAMS *psCropParams,
        IMG_UINT8 uiLevel,
        IMG_UINT8 uiProfile)
{
    H264_SEQUENCE_HEADER_PARAMS SHParams;
    MTX_HEADER_PARAMS   *mtx_hdr;

    /* Route output elements to memory provided */
    memset(&SHParams, 0, sizeof(SHParams));
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
    case 32:
        SHParams.ucLevel =  SH_LEVEL_32;
        break;
    case 40:
        SHParams.ucLevel =  SH_LEVEL_4;
        break;
    case 41:
        SHParams.ucLevel =  SH_LEVEL_41;
        break;
    case 42:
        SHParams.ucLevel =  SH_LEVEL_42;
        break;
    default:
        SHParams.ucLevel =  SH_LEVEL_3;
        break;
    }

    SHParams.ucWidth_in_mbs_minus1 = (IMG_UINT8)(uiPicWidthInMbs - 1);
    SHParams.ucHeight_in_maps_units_minus1 = (IMG_UINT8)(uiPicHeightInMbs - 1);
    SHParams.VUI_Params_Present = VUI_present;
    if (VUI_present)
        SHParams.VUI_Params = *VUI_params;
    SHParams.gaps_in_frame_num_value = IMG_FALSE;
    SHParams.ucFrame_mbs_only_flag = IMG_TRUE;

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
    pnw__H264_getelements_sequence_header(mtx_hdr, &SHParams, psCropParams);
}

void pnw__H264_prepare_picture_header(unsigned char *pHeaderMemory, IMG_BOOL bCabacEnabled, IMG_INT8 CQPOffset)
{
    MTX_HEADER_PARAMS *mtx_hdr;

    /* Route output elements to memory provided */
    mtx_hdr = (MTX_HEADER_PARAMS *) pHeaderMemory;


    /* Builds a sequence, picture and slice header with from the given inputs parameters (start of new frame)
     * Essential we initialise our header structures before building
     */
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *elt_p[MAXNUMBERELEMENTS];
    mtx_hdr->Elements = ELEMENTS_EMPTY;
    This_Element = (MTX_HEADER_ELEMENT *) mtx_hdr->asElementStream;
    elt_p[0] = This_Element;

    pnw__H264_writebits_picture_header(mtx_hdr, elt_p, bCabacEnabled,
                                       0, 0,
                                       CQPOffset, CQPOffset);
    mtx_hdr->Elements++; /* Has been used as an index, so need to add 1 for a valid element count */
}

void pnw__H264_prepare_slice_header(
    unsigned char *pHeaderMemory,
    IMG_BOOL    bIntraSlice,
    IMG_UINT32 uiDisableDeblockingFilterIDC,
    IMG_UINT32 uiFrameNumber,
    IMG_UINT32 uiFirst_MB_Address,
    IMG_UINT32 uiMBSkipRun,
    IMG_BOOL bCabacEnabled,
    IMG_BOOL bForceIDR,
    IMG_BOOL bUsesLongTermRef,
    IMG_BOOL bIsLongTermRef,
    IMG_UINT16 uiIdrPicId)
{
    H264_SLICE_HEADER_PARAMS SlHParams;
    MTX_HEADER_PARAMS *mtx_hdr;

    memset(&SlHParams, 0, sizeof(SlHParams));

    /* Route output elements to memory provided */
    mtx_hdr = (MTX_HEADER_PARAMS *) pHeaderMemory;

    SlHParams.Start_Code_Prefix_Size_Bytes = 4;
    SlHParams.UsesLongTermRef = bUsesLongTermRef;
    SlHParams.IsLongTermRef = bIsLongTermRef;

    if (bForceIDR || (uiFrameNumber == 0)) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "TOPAZ: Generate a IDR slice\n");
        SlHParams.SliceFrame_Type = SLHP_IDR_SLICEFRAME_TYPE;
    } else
        SlHParams.SliceFrame_Type = bIntraSlice ? SLHP_I_SLICEFRAME_TYPE : SLHP_P_SLICEFRAME_TYPE;

    /*SlHParams.SliceFrame_Type =       bIntraSlice ? (((uiFrameNumber%(1<<5))==0) ? SLHP_IDR_SLICEFRAME_TYPE :SLHP_I_SLICEFRAME_TYPE ) : SLHP_P_SLICEFRAME_TYPE;*/
    SlHParams.Frame_Num_DO = (IMG_UINT8) uiFrameNumber % (1 << 5);
    SlHParams.Picture_Num_DO = (IMG_UINT8)(SlHParams.Frame_Num_DO * 2);


    SlHParams.First_MB_Address  =  uiFirst_MB_Address;
    SlHParams.Disable_Deblocking_Filter_Idc = (IMG_UINT8) uiDisableDeblockingFilterIDC;

    {
        IMG_UINT32      *pMTX_Header_Mem = (IMG_UINT32 *)mtx_hdr;
        // rhk: first insert normal header.
        pnw__H264_getelements_slice_header(mtx_hdr, &SlHParams, bCabacEnabled, uiIdrPicId);

        // put a marker to indicate that this is a complex header
        // note that first "int" in the buffer is used for number of elements
        // which is not going to be more than 255
        *pMTX_Header_Mem |= 0x100;

        // rhk: insert skipped frame header at an offset of 128 bytes
        pMTX_Header_Mem += (HEADER_SIZE >> 3);  // get a pointer to the second half of memory
        mtx_hdr = (MTX_HEADER_PARAMS *)pMTX_Header_Mem;
        pnw__H264_getelements_skip_P_slice(mtx_hdr, &SlHParams, uiMBSkipRun, bCabacEnabled);
    }

}


void pnw__MPEG4_prepare_sequence_header(
    unsigned char *pHeaderMemory,
    IMG_BOOL bBFrame,
    MPEG4_PROFILE_TYPE sProfile,
    IMG_UINT8 Profile_and_level_indication,
    FIXED_VOP_TIME_TYPE sFixed_vop_time_increment,
    IMG_UINT32 Picture_Width_Pixels,
    IMG_UINT32 Picture_Height_Pixels,
    VBVPARAMS * psVBVParams,
    IMG_UINT32 VopTimeResolution)
{
    MTX_HEADER_PARAMS *mtx_hdr;

    /* Route output elements to memory provided */
    mtx_hdr = (MTX_HEADER_PARAMS *) pHeaderMemory;
    /* Builds a single MPEG4 video sequence header from the given parameters */

    /* Essential we initialise our header structures before building */
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *elt_p[MAXNUMBERELEMENTS];
    mtx_hdr->Elements = ELEMENTS_EMPTY;
    This_Element = (MTX_HEADER_ELEMENT *) mtx_hdr->asElementStream;
    elt_p[0] = This_Element;

    pnw__MPEG4_writebits_sequence_header(
        mtx_hdr,
        elt_p,
        bBFrame, sProfile,
        Profile_and_level_indication,
        sFixed_vop_time_increment,
        Picture_Width_Pixels,
        Picture_Height_Pixels,
        psVBVParams, VopTimeResolution);

    mtx_hdr->Elements++; /* Has been used as an index, so need to add 1 for a valid element count */

}

void pnw__MPEG4_prepare_vop_header(
    unsigned char *pHeaderMem,
    IMG_BOOL bIsVOP_coded,
    IMG_UINT32 VOP_time_increment,
    IMG_UINT8 sSearch_range,
    IMG_UINT8 eVop_Coding_Type,
    IMG_UINT32 VopTimeResolution)
{
    MTX_HEADER_PARAMS *mtx_hdr;

    mtx_hdr = (MTX_HEADER_PARAMS *) pHeaderMem;

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
    pnw__MPEG4_writebits_VOP_header(
        mtx_hdr, elt_p, bIsVOP_coded,
        VOP_time_increment,
        sSearch_range,
        eVop_Coding_Type,
        VopTimeResolution);

    mtx_hdr->Elements++; /* Has been used as an index, so need to add 1 for a valid element count */

}


#if PSB_MFLD_DUMMY_CODE
void pnw__H263_prepare_sequence_header(
    unsigned char *pHeaderMem,
    IMG_UINT8 Profile_and_level_indication)
{
    MTX_HEADER_PARAMS *mtx_hdr;

    mtx_hdr = (MTX_HEADER_PARAMS *) pHeaderMem;

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
#endif

void pnw__H263_prepare_picture_header(
    unsigned char *pHeaderMem,
    IMG_UINT8 Temporal_Ref,
    H263_PICTURE_CODING_TYPE PictureCodingType,
    H263_SOURCE_FORMAT_TYPE SourceFormatType,
    IMG_UINT8 FrameRate,
    IMG_UINT16 PictureWidth,
    IMG_UINT16 PictureHeigth)
{
    MTX_HEADER_PARAMS *mtx_hdr;

    mtx_hdr = (MTX_HEADER_PARAMS *) pHeaderMem;

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
        PictureHeigth);

    mtx_hdr->Elements++; /* Has been used as an index, so need to add 1 for a valid element count */
}

void pnw__H263_prepare_GOBslice_header(
    unsigned char *pHeaderMem,
    IMG_UINT8 GOBNumber,
    IMG_UINT8 GOBFrameId)
{
    MTX_HEADER_PARAMS *mtx_hdr;

    mtx_hdr = (MTX_HEADER_PARAMS *) pHeaderMem;

    /* Essential we initialise our header structures before building */
    MTX_HEADER_ELEMENT *This_Element;
    MTX_HEADER_ELEMENT *elt_p[MAXNUMBERELEMENTS];
    mtx_hdr->Elements = ELEMENTS_EMPTY;
    This_Element = (MTX_HEADER_ELEMENT *) mtx_hdr->asElementStream;
    elt_p[0] = This_Element;

    H263_writebits_GOBSliceHeader(mtx_hdr, elt_p, GOBNumber, GOBFrameId);

    mtx_hdr->Elements++; //Has been used as an index, so need to add 1 for a valid element count
    
    /*
    (void)pnw__H264_writebits_SEI_rbspheader;
    (void)pnw__H264_getelements_skip_B_slice;
    (void)pnw__H264_getelements_backward_zero_B_slice;
    (void)pnw__H264_getelements_rbsp_ATE_only;
    (void)pnw_MPEG4_getelements_video_packet_header;
    */
}

