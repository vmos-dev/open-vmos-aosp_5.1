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

 @File         msvdx_offsets.h

 @Title        MSVDX Offsets

 @Platform

 @Description

******************************************************************************/
#ifndef _MSVDX_OFFSETS_H_
#define _MSVDX_OFFSETS_H_

#include "msvdx_defs.h"

#define REG_MAC_OFFSET                          REG_MSVDX_DMAC_OFFSET
#define REG_MSVDX_CMDS_OFFSET           REG_MSVDX_CMD_OFFSET
#define REG_MSVDX_VEC_MPEG2_OFFSET      REG_MSVDX_VEC_OFFSET
#define REG_MSVDX_VEC_MPEG4_OFFSET      REG_MSVDX_VEC_OFFSET
#define REG_MSVDX_VEC_H264_OFFSET       REG_MSVDX_VEC_OFFSET
#define REG_MSVDX_VEC_VC1_OFFSET        REG_MSVDX_VEC_OFFSET
#define REG_MSVDX_VEC_VP6_OFFSET        REG_MSVDX_VEC_OFFSET
#define REG_MSVDX_VEC_VP8_OFFSET        REG_MSVDX_VEC_OFFSET
#define REG_MSVDX_VEC_JPEG_OFFSET       REG_MSVDX_VEC_OFFSET
#define REG_MSVDX_CORE_OFFSET           REG_MSVDX_SYS_OFFSET
#define REG_DMAC_OFFSET                         REG_MSVDX_DMAC_OFFSET

#define RENDEC_REGISTER_OFFSET(__group__, __reg__ ) ( (__group__##_##__reg__##_OFFSET) + ( REG_##__group__##_OFFSET ) )

/* Not the best place for this */
#ifdef PLEASE_DONT_INCLUDE_REGISTER_BASE

/* This macro is used by KM gpu sim code */

#define REGISTER_OFFSET(__group__, __reg__ ) ( (__group__##_##__reg__##_OFFSET) )

#else

/* This is the macro used by UM Drivers - it included the Mtx memory offser to the msvdx redisters */

// #define REGISTER_OFFSET(__group__, __reg__ ) ( (__group__##_##__reg__##_OFFSET) + ( REG_##__group__##_OFFSET ) )
#define REGISTER_OFFSET(__group__, __reg__ ) ( (__group__##_##__reg__##_OFFSET) + ( REG_##__group__##_OFFSET ) + 0x04800000 )

#endif


#endif /*_MSVDX_OFFSETS_H_*/
