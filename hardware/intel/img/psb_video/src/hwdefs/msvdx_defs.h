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


/*!****************************************************************************
@File                   msvdx_defs.h

@Title                  System Description Header

@Author                 Imagination Technologies

@date                   20 Decemner 2006

@Platform               generic

@Description    This header provides hardware-specific declarations and macros

@DoxygenVer

******************************************************************************/

/******************************************************************************
Modifications :-

$Log: msvdx_defs.h $
*/

#ifndef _MSVDX_DEFS_H_
#define _MSVDX_DEFS_H_

#define MSVDX_REG_SIZE  0x4000

/* MSVDX Register base definitions                                                                                                              */
#define REG_MSVDX_MTX_OFFSET            0x00000000
#define REG_MSVDX_VDMC_OFFSET           0x00000400
#define REG_MSVDX_VDEB_OFFSET           0x00000480
#define REG_MSVDX_DMAC_OFFSET           0x00000500
#define REG_MSVDX_SYS_OFFSET            0x00000600
#define REG_MSVDX_VEC_IQRAM_OFFSET      0x00000700
#define REG_MSVDX_VEC_OFFSET            0x00000800
#define REG_MSVDX_CMD_OFFSET            0x00001000
#define REG_MSVDX_VEC_RAM_OFFSET        0x00002000
#define REG_MSVDX_VEC_VLC_OFFSET        0x00003000

#define REG_MSVDX_MTX_SIZE                      0x00000400
#define REG_MSVDX_VDMC_SIZE                     0x00000080
#define REG_MSVDX_VDEB_SIZE                     0x00000080
#define REG_MSVDX_DMAC_SIZE                     0x00000100
#define REG_MSVDX_SYS_SIZE                      0x00000100
#define REG_MSVDX_VEC_IQRAM_SIZE        0x00000100
#define REG_MSVDX_VEC_SIZE                      0x00000800
#define REG_MSVDX_CMD_SIZE                      0x00001000
#define REG_MSVDX_VEC_RAM_SIZE          0x00001000
#define REG_MSVDX_VEC_VLC_SIZE          0x00002000

#endif /* _MSVDX_DEFS_H_ */
