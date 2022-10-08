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


#ifndef _REGCONV_H_topazhp_multicore_regs_old_h
#define _REGCONV_H_topazhp_multicore_regs_old_h

#ifdef __cplusplus 
#include "img_types.h"
#include "systemc_utils.h"
#endif 


/* Register CR_LAMBDA_DC_TABLE */
#define TOPAZHP_TOP_CR_LAMBDA_DC_TABLE 0x0120
#define MASK_TOPAZHP_TOP_CR_QPC_OR_DC_SCALE_LUMA_TABLE 0x000000FF
#define SHIFT_TOPAZHP_TOP_CR_QPC_OR_DC_SCALE_LUMA_TABLE 0
#define REGNUM_TOPAZHP_TOP_CR_QPC_OR_DC_SCALE_LUMA_TABLE 0x0120
#define SIGNED_TOPAZHP_TOP_CR_QPC_OR_DC_SCALE_LUMA_TABLE 0

#define MASK_TOPAZHP_TOP_CR_SATD_LAMBDA_OR_DC_SCALE_CHROMA_TABLE 0x0000FF00
#define SHIFT_TOPAZHP_TOP_CR_SATD_LAMBDA_OR_DC_SCALE_CHROMA_TABLE 8
#define REGNUM_TOPAZHP_TOP_CR_SATD_LAMBDA_OR_DC_SCALE_CHROMA_TABLE 0x0120
#define SIGNED_TOPAZHP_TOP_CR_SATD_LAMBDA_OR_DC_SCALE_CHROMA_TABLE 0

#define MASK_TOPAZHP_TOP_CR_SAD_LAMBDA_TABLE 0x007F0000
#define SHIFT_TOPAZHP_TOP_CR_SAD_LAMBDA_TABLE 16
#define REGNUM_TOPAZHP_TOP_CR_SAD_LAMBDA_TABLE 0x0120
#define SIGNED_TOPAZHP_TOP_CR_SAD_LAMBDA_TABLE 0


/* Register CR_MVCALC_COLOCATED (from topazhp_core_regs.h) */
#define MASK_TOPAZHP_CR_TEMPORAL_BLEND 0x001F0000
#define SHIFT_TOPAZHP_CR_TEMPORAL_BLEND 16
#define REGNUM_TOPAZHP_CR_TEMPORAL_BLEND 0x0174
#define SIGNED_TOPAZHP_CR_TEMPORAL_BLEND 0

#endif
