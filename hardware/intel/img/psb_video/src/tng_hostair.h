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
 *    Edward Lin <edward.lin@intel.com>
 *
 */

#ifndef _TNG_HOSTAIR_H_
#define _TNG_HOSTAIR_H_

#include "img_types.h"
#include "tng_hostdefs.h"
#include "tng_hostcode.h"

IMG_UINT32 tng_fill_slice_map(context_ENC_p ctx, IMG_INT32 i32SlotNum, IMG_UINT32 ui32StreamIndex);
void tng_air_set_input_control(context_ENC_p ctx, IMG_UINT8 ui8StreamID);
void tng_air_set_output_control(context_ENC_p ctx, IMG_UINT8 ui8StreamID);
VAStatus tng_air_buf_create(context_ENC_p ctx);
void tng_air_buf_free(context_ENC_p ctx);


typedef struct
{
    IMG_UINT32 ui32SAD_Intra_MBInfo;		//!< SATD/SAD for best Intra candidate (24-bit unsigned value) plus 8 bit field containing MB info
    IMG_UINT32 ui32SAD_Inter_MBInfo;		//!< SATD/SAD for best Inter candidate (24-bit unsigned value) plus 8 bit field containing MB info
    IMG_UINT32 ui32SAD_Direct_MBInfo;	//!< SATD/SAD for best Direct candidate (24-bit unsigned value) plus 8 bit field containing MB info
    IMG_UINT32 ui32Reserved;
} IMG_BEST_MULTIPASS_MB_PARAMS, *P_IMG_BEST_MULTIPASS_MB_PARAMS;

typedef struct
{
    IMG_UINT16  ui16MV4_0_X;       //!< MV4_0_X (this is also MV8_0_X if block size is 8x8, or MV16_X if block size is 16x16)
    IMG_UINT16  ui16MV4_0_Y;       //!< MV4_0_Y (this is also MV8_0_Y if block size is 8x8, or MV16_Y if block size is 16x16)
    IMG_UINT16  ui16MV4_1_X;       //!< MV4_1_X
    IMG_UINT16  ui16MV4_1_Y;       //!< MV4_1_Y
    IMG_UINT16  ui16MV4_2_X;       //!< MV4_2_X
    IMG_UINT16  ui16MV4_2_Y;       //!< MV4_2_Y
    IMG_UINT16  ui16MV4_3_X;       //!< MV4_3_X
    IMG_UINT16  ui16MV4_3_Y;       //!< MV4_3_Y

    IMG_UINT16  ui16MV4_4_X;       //!< MV4_4_X (this is also MV8_1_X if block size is 8x8, or 2nd MV if block size is 8x16)
    IMG_UINT16  ui16MV4_4_Y;       //!< MV4_4_Y (this is also MV8_1_Y if block size is 8x8, or 2nd MV if block size is 8x16)
    IMG_UINT16  ui16MV4_5_X;       //!< MV4_5_X
    IMG_UINT16  ui16MV4_5_Y;       //!< MV4_5_Y
    IMG_UINT16  ui16MV4_6_X;       //!< MV4_6_X
    IMG_UINT16  ui16MV4_6_Y;       //!< MV4_6_Y
    IMG_UINT16  ui16MV4_7_X;       //!< MV4_7_X
    IMG_UINT16  ui16MV4_7_Y;       //!< MV4_7_Y

    IMG_UINT16  ui16MV4_8_X;       //!< MV4_8_X (this is also MV8_2_X if block size is 8x8, or 2nd MV if block size is 16x8)
    IMG_UINT16  ui16MV4_8_Y;       //!< MV4_8_Y (this is also MV8_2_Y if block size is 8x8, or 2nd MV if block size is 16x8)
    IMG_UINT16  ui16MV4_9_X;       //!< MV4_9_X
    IMG_UINT16  ui16MV4_9_Y;       //!< MV4_9_Y
    IMG_UINT16  ui16MV4_10_X;     //!< MV4_10_X
    IMG_UINT16  ui16MV4_10_Y;     //!< MV4_10_Y
    IMG_UINT16  ui16MV4_11_X;     //!< MV4_11_X
    IMG_UINT16  ui16MV4_11_Y;     //!< MV4_11_Y

    IMG_UINT16  ui16MV4_12_X;       //!< MV4_12_X (this is also MV8_3_X if block size is 8x8)
    IMG_UINT16  ui16MV4_12_Y;       //!< MV4_12_Y (this is also MV8_3_Y if block size is 8x8)
    IMG_UINT16  ui16MV4_13_X;       //!< MV4_13_X
    IMG_UINT16  ui16MV4_13_Y;       //!< MV4_13_Y
    IMG_UINT16  ui16MV4_14_X;       //!< MV4_14_X
    IMG_UINT16  ui16MV4_14_Y;       //!< MV4_14_Y
    IMG_UINT16  ui16MV4_15_X;       //!< MV4_15_X
    IMG_UINT16  ui16MV4_15_Y;       //!< MV4_15_Y
} IMG_BEST_MULTIPASS_MB_PARAMS_IPMV, *P_IMG_BEST_MULTIPASS_MB_PARAMS_IPMV;

/*!
*****************************************************************************
* @details    
* @brief      Bit fields for IMG_BEST_MULTIPASS_MB_PARAMS structure
****************************************************************************/
#define     IMG_BEST_MULTIPASS_SAD_SHIFT					(0)
#define     IMG_BEST_MULTIPASS_SAD_MASK						(0xFFFFFF)
#define     IMG_BEST_MULTIPASS_MB_TYPE_SHIFT				(24)
#define     IMG_BEST_MULTIPASS_MB_TYPE_MASK					(0x03 << IMG_BEST_MULTIPASS_MB_TYPE_SHIFT)
#define     IMG_BEST_MULTIPASS_INTRA_BLOCK_SIZE_SHIFT		(26)
#define     IMG_BEST_MULTIPASS_INTRA_BLOCK_SIZE_MASK		(0x03 << IMG_BEST_MULTIPASS_INTRA_BLOCK_SIZE_SHIFT)
#define     IMG_BEST_MULTIPASS_TOPLEFT_SKIPDIRECT_SHIFT		(28)
#define     IMG_BEST_MULTIPASS_TOPLEFT_SKIPDIRECT_MASK		(0x01 << IMG_BEST_MULTIPASS_TOPLEFT_SKIPDIRECT_SHIFT)
#define     IMG_BEST_MULTIPASS_TOPRIGHT_SKIPDIRECT_SHIFT	(29)
#define     IMG_BEST_MULTIPASS_TOPRIGHT_SKIPDIRECT_MASK		(0x01 << IMG_BEST_MULTIPASS_TOPRIGHT_SKIPDIRECT_SHIFT)
#define     IMG_BEST_MULTIPASS_BOTTOMLEFT_SKIPDIRECT_SHIFT	(30)
#define     IMG_BEST_MULTIPASS_BOTTOMLEFT_SKIPDIRECT_MASK	(0x01 << IMG_BEST_MULTIPASS_BOTTOMLEFT_SKIPDIRECT_SHIFT)
#define     IMG_BEST_MULTIPASS_BOTTOMRIGHT_SKIPDIRECT_SHIFT	(31)
#define     IMG_BEST_MULTIPASS_BOTTOMRIGHT_SKIPDIRECT_MASK	(0x01 << IMG_BEST_MULTIPASS_BOTTOMRIGHT_SKIPDIRECT_SHIFT)

#define     IMG_BEST_MULTIPASS_TOPLEFT_CODING_SHIFT			(24)
#define     IMG_BEST_MULTIPASS_TOPLEFT_CODING_MASK			(0x01 << IMG_BEST_MULTIPASS_TOPLEFT_CODING_SHIFT)
#define     IMG_BEST_MULTIPASS_TOPRIGHT_CODING_SHIFT		(25)
#define     IMG_BEST_MULTIPASS_TOPRIGHT_CODING_MASK			(0x01 << IMG_BEST_MULTIPASS_TOPRIGHT_CODING_SHIFT)
#define     IMG_BEST_MULTIPASS_BOTTOMLEFT_CODING_SHIFT		(26)
#define     IMG_BEST_MULTIPASS_BOTTOMLEFT_CODING_MASK		(0x01 << IMG_BEST_MULTIPASS_BOTTOMLEFT_CODING_SHIFT)
#define     IMG_BEST_MULTIPASS_BOTTOMRIGHT_CODING_SHIFT		(27)
#define     IMG_BEST_MULTIPASS_BOTTOMRIGHT_CODING_MASK		(0x01 << IMG_BEST_MULTIPASS_BOTTOMRIGHT_CODING_SHIFT)
#define     IMG_BEST_MULTIPASS_IS_16x16_BEST_SHIFT			(28)
#define     IMG_BEST_MULTIPASS_IS_16x16_BEST_MASK			(0x01 << IMG_BEST_MULTIPASS_IS_16x16_BEST_SHIFT)
#define     IMG_BEST_MULTIPASS_IS_16x8_BEST_SHIFT			(29)
#define     IMG_BEST_MULTIPASS_IS_16x8_BEST_MASK			(0x01 << IMG_BEST_MULTIPASS_IS_16x8_BEST_SHIFT)
#define     IMG_BEST_MULTIPASS_IS_8x16_BEST_SHIFT			(30)
#define     IMG_BEST_MULTIPASS_IS_8x16_BEST_MASK			(0x01 << IMG_BEST_MULTIPASS_IS_8x16_BEST_SHIFT)
// Final bit reserved

#define     IMG_BEST_MULTIPASS_TOPLEFT_BEST_INTER_CAND_SHIFT		(24)
#define     IMG_BEST_MULTIPASS_TOPLEFT_BEST_INTER_CAND_MASK			(0x03 << IMG_BEST_MULTIPASS_TOPLEFT_BEST_INTER_CAND_SHIFT)
#define     IMG_BEST_MULTIPASS_TOPRIGHT_BEST_INTER_CAND_SHIFT		(26)
#define     IMG_BEST_MULTIPASS_TOPRIGHT_BEST_INTER_CAND_MASK		(0x03 << IMG_BEST_MULTIPASS_TOPRIGHT_BEST_INTER_CAND_SHIFT)
#define     IMG_BEST_MULTIPASS_BOTTOMLEFT_BEST_INTER_CAND_SHIFT		(28)
#define     IMG_BEST_MULTIPASS_BOTTOMLEFT_BEST_INTER_CAND_MASK		(0x03 << IMG_BEST_MULTIPASS_BOTTOMLEFT_BEST_INTER_CAND_SHIFT)
#define     IMG_BEST_MULTIPASS_BOTTOMRIGHT_BEST_INTER_CAND_SHIFT	(30)
#define     IMG_BEST_MULTIPASS_BOTTOMRIGHT_BEST_INTER_CAND_MASK		(0x03 << IMG_BEST_MULTIPASS_BOTTOMRIGHT_BEST_INTER_CAND_SHIFT)


#endif //_TNG_HOSTBIAS_H_
