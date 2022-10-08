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

 @File         msvdx_rendec_mtx_slice_cntrl_reg_io2.h

 @Title        MSVDX Offsets

 @Platform     </b>\n

 @Description  </b>\n This file contains the
               MSVDX_RENDEC_MTX_SLICE_CNTRL_REG_IO2_H Defintions.

******************************************************************************/
#if !defined (__MSVDX_RENDEC_MTX_SLICE_CNTRL_REG_IO2_H__)
#define __MSVDX_RENDEC_MTX_SLICE_CNTRL_REG_IO2_H__

#ifdef __cplusplus
extern "C" {
#endif


#define RENDEC_SLICE_INFO_SL_HDR_CK_START_OFFSET                (0x0000)

// RENDEC_SLICE_INFO     SL_HDR_CK_START     SL_NUM_SYMBOLS_LESS1
#define RENDEC_SLICE_INFO_SL_HDR_CK_START_SL_NUM_SYMBOLS_LESS1_MASK             (0x07FF0000)
#define RENDEC_SLICE_INFO_SL_HDR_CK_START_SL_NUM_SYMBOLS_LESS1_LSBMASK          (0x000007FF)
#define RENDEC_SLICE_INFO_SL_HDR_CK_START_SL_NUM_SYMBOLS_LESS1_SHIFT            (16)

// RENDEC_SLICE_INFO     SL_HDR_CK_START     SL_ROUTING_INFO
#define RENDEC_SLICE_INFO_SL_HDR_CK_START_SL_ROUTING_INFO_MASK          (0x00000018)
#define RENDEC_SLICE_INFO_SL_HDR_CK_START_SL_ROUTING_INFO_LSBMASK               (0x00000003)
#define RENDEC_SLICE_INFO_SL_HDR_CK_START_SL_ROUTING_INFO_SHIFT         (3)

// RENDEC_SLICE_INFO     SL_HDR_CK_START     SL_ENCODING_METHOD
#define RENDEC_SLICE_INFO_SL_HDR_CK_START_SL_ENCODING_METHOD_MASK               (0x00000007)
#define RENDEC_SLICE_INFO_SL_HDR_CK_START_SL_ENCODING_METHOD_LSBMASK            (0x00000007)
#define RENDEC_SLICE_INFO_SL_HDR_CK_START_SL_ENCODING_METHOD_SHIFT              (0)

#define RENDEC_SLICE_INFO_SL_HDR_CK_PARAMS_OFFSET               (0x0004)

// RENDEC_SLICE_INFO     SL_HDR_CK_PARAMS     SL_DATA
#define RENDEC_SLICE_INFO_SL_HDR_CK_PARAMS_SL_DATA_MASK         (0xFFFFFFFF)
#define RENDEC_SLICE_INFO_SL_HDR_CK_PARAMS_SL_DATA_LSBMASK              (0xFFFFFFFF)
#define RENDEC_SLICE_INFO_SL_HDR_CK_PARAMS_SL_DATA_SHIFT                (0)

#define RENDEC_SLICE_INFO_CK_HDR_OFFSET         (0x0008)

// RENDEC_SLICE_INFO     CK_HDR     CK_NUM_SYMBOLS_LESS1
#define RENDEC_SLICE_INFO_CK_HDR_CK_NUM_SYMBOLS_LESS1_MASK              (0x07FF0000)
#define RENDEC_SLICE_INFO_CK_HDR_CK_NUM_SYMBOLS_LESS1_LSBMASK           (0x000007FF)
#define RENDEC_SLICE_INFO_CK_HDR_CK_NUM_SYMBOLS_LESS1_SHIFT             (16)

// RENDEC_SLICE_INFO     CK_HDR     CK_START_ADDRESS
#define RENDEC_SLICE_INFO_CK_HDR_CK_START_ADDRESS_MASK          (0x0000FFF0)
#define RENDEC_SLICE_INFO_CK_HDR_CK_START_ADDRESS_LSBMASK               (0x00000FFF)
#define RENDEC_SLICE_INFO_CK_HDR_CK_START_ADDRESS_SHIFT         (4)

// RENDEC_SLICE_INFO     CK_HDR     CK_ENCODING_METHOD
#define RENDEC_SLICE_INFO_CK_HDR_CK_ENCODING_METHOD_MASK                (0x00000007)
#define RENDEC_SLICE_INFO_CK_HDR_CK_ENCODING_METHOD_LSBMASK             (0x00000007)
#define RENDEC_SLICE_INFO_CK_HDR_CK_ENCODING_METHOD_SHIFT               (0)

#define RENDEC_SLICE_INFO_SLICE_SEPARATOR_OFFSET                (0x000C)

// RENDEC_SLICE_INFO     SLICE_SEPARATOR     SL_SEP_SUFFIX
#define RENDEC_SLICE_INFO_SLICE_SEPARATOR_SL_SEP_SUFFIX_MASK            (0x00000007)
#define RENDEC_SLICE_INFO_SLICE_SEPARATOR_SL_SEP_SUFFIX_LSBMASK         (0x00000007)
#define RENDEC_SLICE_INFO_SLICE_SEPARATOR_SL_SEP_SUFFIX_SHIFT           (0)

#define RENDEC_SLICE_INFO_CK_GENERIC_OFFSET             (0x0010)

// RENDEC_SLICE_INFO     CK_GENERIC     CK_HW_CODE
#define RENDEC_SLICE_INFO_CK_GENERIC_CK_HW_CODE_MASK            (0x0000FFFF)
#define RENDEC_SLICE_INFO_CK_GENERIC_CK_HW_CODE_LSBMASK         (0x0000FFFF)
#define RENDEC_SLICE_INFO_CK_GENERIC_CK_HW_CODE_SHIFT           (0)



#ifdef __cplusplus
}
#endif

#endif /* __MSVDX_RENDEC_MTX_SLICE_CNTRL_REG_IO2_H__ */
