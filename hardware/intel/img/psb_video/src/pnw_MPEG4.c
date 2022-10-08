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
 *    Waldo Bastian <waldo.bastian@intel.com>
 *    Li Zeng <li.zeng@intel.com>
 *
 */

#include "pnw_MPEG4.h"
#include "tng_vld_dec.h"
#include "psb_def.h"
#include "psb_drv_debug.h"

#include "hwdefs/reg_io2.h"
#include "hwdefs/msvdx_offsets.h"
#include "hwdefs/msvdx_cmds_io2.h"
#include "hwdefs/msvdx_vec_reg_io2.h"
#include "hwdefs/msvdx_vec_mpeg4_reg_io2.h"
#include "hwdefs/dxva_fw_ctrl.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define GET_SURFACE_INFO_is_defined(psb_surface) ((int) (psb_surface->extra_info[0]))
#define SET_SURFACE_INFO_is_defined(psb_surface, val) psb_surface->extra_info[0] = (uint32_t) val;
#define GET_SURFACE_INFO_picture_structure(psb_surface) (psb_surface->extra_info[1])
#define SET_SURFACE_INFO_picture_structure(psb_surface, val) psb_surface->extra_info[1] = val;
#define GET_SURFACE_INFO_picture_coding_type(psb_surface) ((int) (psb_surface->extra_info[2]))
#define SET_SURFACE_INFO_picture_coding_type(psb_surface, val) psb_surface->extra_info[2] = (uint32_t) val;

#define SLICEDATA_BUFFER_TYPE(type) ((type==VASliceDataBufferType)?"VASliceDataBufferType":"VAProtectedSliceDataBufferType")

#define PIXELS_TO_MB(x)    ((x + 15) / 16)

/*
 * Frame types - format dependant!
 */
#define PICTURE_CODING_I    0x00
#define PICTURE_CODING_P    0x01
#define PICTURE_CODING_B    0x02
#define PICTURE_CODING_S    0x03


#define FE_STATE_BUFFER_SIZE    4096
#define FE_STATE_SAVE_SIZE      ( 0xB40 - 0x700 )

#define MPEG4_PROFILE_SIMPLE    0
#define MPEG4_PROFILE_ASP    2

#define HW_SUPPORTED_MAX_PICTURE_WIDTH_MPEG4   1920
#define HW_SUPPORTED_MAX_PICTURE_HEIGHT_MPEG4  1088
#define HW_SUPPORTED_MAX_PICTURE_WIDTH_H263    720
#define HW_SUPPORTED_MAX_PICTURE_HEIGHT_H263   576

/* Table V2-2 ISO/IEC 14496-2:2001(E) - sprite enable codewords */
typedef enum {
    SPRITE_NOT_USED = 0,
    STATIC,
    GMC,
} MPEG4_eSpriteEnable;


#define MAX_QUANT_TABLES    (2) /* only 2 tables for 4:2:0 decode */

static int scan0[64] = { // spec, fig 7-2
    /*u 0  .....                   7*/
    0,  1,  5,  6,  14, 15, 27, 28,  /* v = 0 */
    2,  4,  7,  13, 16, 26, 29, 42,
    3,  8,  12, 17, 25, 30, 41, 43,
    9,  11, 18, 24, 31, 40, 44, 53,
    10, 19, 23, 32, 39, 45, 52, 54,
    20, 22, 33, 38, 46, 51, 55, 60,
    21, 34, 37, 47, 50, 56, 59, 61,
    35, 36, 48, 49, 57, 58, 62, 63  /* v = 7 */
};

typedef enum {
    NONINTRA_LUMA_Q = 0,
    INTRA_LUMA_Q = 1
} QUANT_IDX;

/************************************************************************************/
/*                Variable length codes in 'packed' format                            */
/************************************************************************************/

/* Format is: opcode, width, symbol. All VLC tables are concatenated.                 */
#define VLC_PACK(a,b,c)         ( ( (a) << 12 ) | ( (b) << 9  ) | (c) )
static const IMG_UINT16 gaui16mpeg4VlcTableDataPacked[] =
{
/* B6_mcbpc_i_s_vops_piece.out */
	VLC_PACK( 4 , 0 , 12 ) ,
	VLC_PACK( 5 , 0 , 7 ) ,
	VLC_PACK( 4 , 2 , 13 ) ,
	VLC_PACK( 4 , 3 , 16 ) ,
	VLC_PACK( 5 , 0 , 9 ) ,
	VLC_PACK( 4 , 5 , 17 ) ,
	VLC_PACK( 2 , 2 , 1 ) ,
	VLC_PACK( 3 , 2 , 0 ) ,
	VLC_PACK( 3 , 2 , 0 ) ,
	VLC_PACK( 4 , 2 , 36 ) ,
	VLC_PACK( 3 , 2 , 0 ) ,
/* B7_mcbpc_p_s_vops_update.out */
	VLC_PACK( 4 , 0 , 0 ) ,
	VLC_PACK( 0 , 0 , 6 ) ,
	VLC_PACK( 0 , 0 , 7 ) ,
	VLC_PACK( 2 , 1 , 8 ) ,
	VLC_PACK( 0 , 1 , 10 ) ,
	VLC_PACK( 2 , 1 , 13 ) ,
	VLC_PACK( 0 , 2 , 15 ) ,
	VLC_PACK( 4 , 0 , 8 ) ,
	VLC_PACK( 4 , 0 , 4 ) ,
	VLC_PACK( 4 , 0 , 2 ) ,
	VLC_PACK( 4 , 0 , 1 ) ,
	VLC_PACK( 4 , 0 , 12 ) ,
	VLC_PACK( 4 , 1 , 3 ) ,
	VLC_PACK( 4 , 1 , 16 ) ,
	VLC_PACK( 4 , 1 , 10 ) ,
	VLC_PACK( 4 , 1 , 9 ) ,
	VLC_PACK( 4 , 1 , 6 ) ,
	VLC_PACK( 4 , 1 , 5 ) ,
	VLC_PACK( 4 , 0 , 15 ) ,
	VLC_PACK( 4 , 1 , 11 ) ,
	VLC_PACK( 4 , 1 , 13 ) ,
	VLC_PACK( 3 , 2 , 0 ) ,
	VLC_PACK( 4 , 2 , 36 ) ,
	VLC_PACK( 4 , 2 , 19 ) ,
	VLC_PACK( 4 , 2 , 18 ) ,
	VLC_PACK( 4 , 2 , 17 ) ,
	VLC_PACK( 4 , 2 , 7 ) ,
	VLC_PACK( 4 , 1 , 14 ) ,
	VLC_PACK( 4 , 1 , 14 ) ,
/* B8_cbpy_intra.out */
	VLC_PACK( 1 , 1 , 16 ) ,
	VLC_PACK( 0 , 0 , 18 ) ,
	VLC_PACK( 0 , 0 , 19 ) ,
	VLC_PACK( 4 , 3 , 0 ) ,
	VLC_PACK( 4 , 3 , 12 ) ,
	VLC_PACK( 4 , 3 , 10 ) ,
	VLC_PACK( 4 , 3 , 14 ) ,
	VLC_PACK( 4 , 3 , 5 ) ,
	VLC_PACK( 4 , 3 , 13 ) ,
	VLC_PACK( 4 , 3 , 3 ) ,
	VLC_PACK( 4 , 3 , 11 ) ,
	VLC_PACK( 4 , 3 , 7 ) ,
	VLC_PACK( 4 , 1 , 15 ) ,
	VLC_PACK( 4 , 1 , 15 ) ,
	VLC_PACK( 4 , 1 , 15 ) ,
	VLC_PACK( 4 , 1 , 15 ) ,
	VLC_PACK( 3 , 1 , 0 ) ,
	VLC_PACK( 4 , 1 , 6 ) ,
	VLC_PACK( 4 , 1 , 9 ) ,
	VLC_PACK( 4 , 0 , 8 ) ,
	VLC_PACK( 4 , 0 , 4 ) ,
	VLC_PACK( 4 , 0 , 2 ) ,
	VLC_PACK( 4 , 0 , 1 ) ,
/* B8_cbpy_inter.out */
	VLC_PACK( 1 , 1 , 16 ) ,
	VLC_PACK( 0 , 0 , 18 ) ,
	VLC_PACK( 6 , 0 , 6 ) ,
	VLC_PACK( 4 , 3 , 15 ) ,
	VLC_PACK( 4 , 3 , 3 ) ,
	VLC_PACK( 4 , 3 , 5 ) ,
	VLC_PACK( 4 , 3 , 1 ) ,
	VLC_PACK( 4 , 3 , 10 ) ,
	VLC_PACK( 4 , 3 , 2 ) ,
	VLC_PACK( 4 , 3 , 12 ) ,
	VLC_PACK( 4 , 3 , 4 ) ,
	VLC_PACK( 4 , 3 , 8 ) ,
	VLC_PACK( 4 , 1 , 0 ) ,
	VLC_PACK( 4 , 1 , 0 ) ,
	VLC_PACK( 4 , 1 , 0 ) ,
	VLC_PACK( 4 , 1 , 0 ) ,
	VLC_PACK( 3 , 1 , 0 ) ,
	VLC_PACK( 4 , 1 , 9 ) ,
	VLC_PACK( 4 , 1 , 6 ) ,
	VLC_PACK( 4 , 0 , 7 ) ,
	VLC_PACK( 4 , 0 , 11 ) ,
/* B3_modb.out */
	VLC_PACK( 4 , 0 , 0 ) ,
	VLC_PACK( 4 , 1 , 1 ) ,
	VLC_PACK( 4 , 1 , 2 ) ,
/* B4_mb_type.out */
	VLC_PACK( 4 , 0 , 0 ) ,
	VLC_PACK( 4 , 1 , 1 ) ,
	VLC_PACK( 4 , 2 , 2 ) ,
	VLC_PACK( 4 , 3 , 3 ) ,
	VLC_PACK( 3 , 3 , 0 ) ,
/* 6_33_dbquant.out */
	VLC_PACK( 4 , 0 , 0 ) ,
	VLC_PACK( 4 , 1 , 6 ) ,
	VLC_PACK( 4 , 1 , 2 ) ,
/* B12_mvd.out */
	VLC_PACK( 4 , 0 , 0 ) ,
	VLC_PACK( 5 , 0 , 1 ) ,
	VLC_PACK( 5 , 0 , 2 ) ,
	VLC_PACK( 5 , 0 , 3 ) ,
	VLC_PACK( 2 , 1 , 3 ) ,
	VLC_PACK( 2 , 3 , 5 ) ,
	VLC_PACK( 0 , 3 , 11 ) ,
	VLC_PACK( 5 , 0 , 4 ) ,
	VLC_PACK( 5 , 0 , 5 ) ,
	VLC_PACK( 5 , 0 , 6 ) ,
	VLC_PACK( 5 , 0 , 7 ) ,
	VLC_PACK( 0 , 0 , 4 ) ,
	VLC_PACK( 5 , 0 , 10 ) ,
	VLC_PACK( 5 , 0 , 11 ) ,
	VLC_PACK( 5 , 0 , 12 ) ,
	VLC_PACK( 5 , 0 , 9 ) ,
	VLC_PACK( 5 , 0 , 8 ) ,
	VLC_PACK( 1 , 1 , 16 ) ,
	VLC_PACK( 0 , 0 , 18 ) ,
	VLC_PACK( 0 , 0 , 19 ) ,
	VLC_PACK( 0 , 0 , 20 ) ,
	VLC_PACK( 5 , 0 , 24 ) ,
	VLC_PACK( 5 , 0 , 23 ) ,
	VLC_PACK( 5 , 0 , 22 ) ,
	VLC_PACK( 5 , 0 , 21 ) ,
	VLC_PACK( 5 , 0 , 20 ) ,
	VLC_PACK( 5 , 0 , 19 ) ,
	VLC_PACK( 5 , 0 , 18 ) ,
	VLC_PACK( 5 , 0 , 17 ) ,
	VLC_PACK( 5 , 0 , 16 ) ,
	VLC_PACK( 5 , 0 , 15 ) ,
	VLC_PACK( 5 , 0 , 14 ) ,
	VLC_PACK( 5 , 0 , 13 ) ,
	VLC_PACK( 3 , 1 , 0 ) ,
	VLC_PACK( 5 , 0 , 32 ) ,
	VLC_PACK( 5 , 0 , 31 ) ,
	VLC_PACK( 5 , 0 , 30 ) ,
	VLC_PACK( 5 , 0 , 29 ) ,
	VLC_PACK( 5 , 0 , 28 ) ,
	VLC_PACK( 5 , 0 , 27 ) ,
	VLC_PACK( 5 , 0 , 26 ) ,
	VLC_PACK( 5 , 0 , 25 ) ,
/* B13_dct_dc_size_luminance.out */
	VLC_PACK( 2 , 5 , 4 ) ,
	VLC_PACK( 0 , 0 , 14 ) ,
	VLC_PACK( 4 , 1 , 2 ) ,
	VLC_PACK( 4 , 1 , 1 ) ,
	VLC_PACK( 4 , 0 , 4 ) ,
	VLC_PACK( 4 , 1 , 5 ) ,
	VLC_PACK( 4 , 2 , 6 ) ,
	VLC_PACK( 4 , 3 , 7 ) ,
	VLC_PACK( 4 , 4 , 8 ) ,
	VLC_PACK( 4 , 5 , 9 ) ,
	VLC_PACK( 2 , 2 , 1 ) ,
	VLC_PACK( 4 , 0 , 10 ) ,
	VLC_PACK( 4 , 1 , 11 ) ,
	VLC_PACK( 4 , 2 , 12 ) ,
	VLC_PACK( 3 , 2 , 0 ) ,
	VLC_PACK( 4 , 0 , 3 ) ,
	VLC_PACK( 4 , 0 , 0 ) ,
/* B14_dct_dc_size_chrominance.out */
	VLC_PACK( 2 , 5 , 4 ) ,
	VLC_PACK( 4 , 1 , 2 ) ,
	VLC_PACK( 4 , 1 , 1 ) ,
	VLC_PACK( 4 , 1 , 0 ) ,
	VLC_PACK( 4 , 0 , 3 ) ,
	VLC_PACK( 4 , 1 , 4 ) ,
	VLC_PACK( 4 , 2 , 5 ) ,
	VLC_PACK( 4 , 3 , 6 ) ,
	VLC_PACK( 4 , 4 , 7 ) ,
	VLC_PACK( 4 , 5 , 8 ) ,
	VLC_PACK( 2 , 3 , 1 ) ,
	VLC_PACK( 4 , 0 , 9 ) ,
	VLC_PACK( 4 , 1 , 10 ) ,
	VLC_PACK( 4 , 2 , 11 ) ,
	VLC_PACK( 4 , 3 , 12 ) ,
	VLC_PACK( 3 , 3 , 0 ) ,
/* B16_intra_tcoeff.out */
	VLC_PACK( 2 , 1 , 16 ) ,
	VLC_PACK( 0 , 3 , 77 ) ,
	VLC_PACK( 0 , 2 , 96 ) ,
	VLC_PACK( 0 , 1 , 103 ) ,
	VLC_PACK( 2 , 1 , 106 ) ,
	VLC_PACK( 2 , 1 , 108 ) ,
	VLC_PACK( 5 , 1 , 250 ) ,
	VLC_PACK( 7 , 0 , 254 ) ,
	VLC_PACK( 4 , 2 , 508 ) ,
	VLC_PACK( 4 , 2 , 508 ) ,
	VLC_PACK( 4 , 2 , 509 ) ,
	VLC_PACK( 4 , 2 , 509 ) ,
	VLC_PACK( 4 , 3 , 506 ) ,
	VLC_PACK( 4 , 3 , 507 ) ,
	VLC_PACK( 5 , 0 , 222 ) ,
	VLC_PACK( 5 , 0 , 252 ) ,
	VLC_PACK( 0 , 3 , 3 ) ,
	VLC_PACK( 2 , 1 , 18 ) ,
	VLC_PACK( 0 , 3 , 44 ) ,
	VLC_PACK( 5 , 1 , 237 ) ,
	VLC_PACK( 7 , 0 , 433 ) ,
	VLC_PACK( 7 , 0 , 434 ) ,
	VLC_PACK( 7 , 0 , 435 ) ,
	VLC_PACK( 7 , 0 , 436 ) ,
	VLC_PACK( 7 , 0 , 437 ) ,
	VLC_PACK( 7 , 0 , 221 ) ,
	VLC_PACK( 7 , 0 , 251 ) ,
	VLC_PACK( 5 , 0 , 435 ) ,
	VLC_PACK( 5 , 0 , 436 ) ,
	VLC_PACK( 5 , 0 , 29 ) ,
	VLC_PACK( 5 , 0 , 61 ) ,
	VLC_PACK( 5 , 0 , 93 ) ,
	VLC_PACK( 5 , 0 , 156 ) ,
	VLC_PACK( 5 , 0 , 188 ) ,
	VLC_PACK( 5 , 0 , 217 ) ,
	VLC_PACK( 4 , 0 , 255 ) ,
	VLC_PACK( 0 , 3 , 2 ) ,
	VLC_PACK( 0 , 2 , 17 ) ,
	VLC_PACK( 5 , 0 , 230 ) ,
	VLC_PACK( 5 , 0 , 229 ) ,
	VLC_PACK( 5 , 0 , 228 ) ,
	VLC_PACK( 5 , 0 , 214 ) ,
	VLC_PACK( 5 , 0 , 60 ) ,
	VLC_PACK( 5 , 0 , 213 ) ,
	VLC_PACK( 5 , 0 , 186 ) ,
	VLC_PACK( 5 , 0 , 28 ) ,
	VLC_PACK( 5 , 0 , 433 ) ,
	VLC_PACK( 7 , 0 , 247 ) ,
	VLC_PACK( 7 , 0 , 93 ) ,
	VLC_PACK( 7 , 0 , 61 ) ,
	VLC_PACK( 7 , 0 , 430 ) ,
	VLC_PACK( 7 , 0 , 429 ) ,
	VLC_PACK( 7 , 0 , 428 ) ,
	VLC_PACK( 7 , 0 , 427 ) ,
	VLC_PACK( 5 , 0 , 232 ) ,
	VLC_PACK( 5 , 0 , 231 ) ,
	VLC_PACK( 5 , 0 , 215 ) ,
	VLC_PACK( 5 , 0 , 374 ) ,
	VLC_PACK( 7 , 0 , 157 ) ,
	VLC_PACK( 7 , 0 , 125 ) ,
	VLC_PACK( 7 , 0 , 432 ) ,
	VLC_PACK( 7 , 0 , 431 ) ,
	VLC_PACK( 3 , 3 , 0 ) ,
	VLC_PACK( 3 , 3 , 0 ) ,
	VLC_PACK( 7 , 1 , 248 ) ,
	VLC_PACK( 5 , 1 , 233 ) ,
	VLC_PACK( 7 , 0 , 189 ) ,
	VLC_PACK( 7 , 0 , 220 ) ,
	VLC_PACK( 7 , 0 , 250 ) ,
	VLC_PACK( 5 , 0 , 434 ) ,
	VLC_PACK( 5 , 0 , 92 ) ,
	VLC_PACK( 5 , 0 , 375 ) ,
	VLC_PACK( 5 , 0 , 124 ) ,
	VLC_PACK( 5 , 0 , 155 ) ,
	VLC_PACK( 5 , 0 , 187 ) ,
	VLC_PACK( 5 , 0 , 216 ) ,
	VLC_PACK( 5 , 0 , 235 ) ,
	VLC_PACK( 5 , 0 , 236 ) ,
	VLC_PACK( 0 , 0 , 16 ) ,
	VLC_PACK( 0 , 0 , 17 ) ,
	VLC_PACK( 5 , 1 , 241 ) ,
	VLC_PACK( 7 , 0 , 439 ) ,
	VLC_PACK( 7 , 0 , 30 ) ,
	VLC_PACK( 7 , 0 , 62 ) ,
	VLC_PACK( 7 , 0 , 252 ) ,
	VLC_PACK( 5 , 0 , 437 ) ,
	VLC_PACK( 5 , 0 , 438 ) ,
	VLC_PACK( 5 , 0 , 439 ) ,
	VLC_PACK( 7 , 0 , 438 ) ,
	VLC_PACK( 5 , 0 , 157 ) ,
	VLC_PACK( 5 , 0 , 219 ) ,
	VLC_PACK( 5 , 0 , 243 ) ,
	VLC_PACK( 5 , 0 , 244 ) ,
	VLC_PACK( 5 , 0 , 245 ) ,
	VLC_PACK( 5 , 0 , 218 ) ,
	VLC_PACK( 5 , 0 , 239 ) ,
	VLC_PACK( 5 , 0 , 125 ) ,
	VLC_PACK( 5 , 0 , 240 ) ,
	VLC_PACK( 7 , 0 , 126 ) ,
	VLC_PACK( 7 , 0 , 158 ) ,
	VLC_PACK( 5 , 0 , 62 ) ,
	VLC_PACK( 7 , 0 , 94 ) ,
	VLC_PACK( 5 , 0 , 30 ) ,
	VLC_PACK( 5 , 0 , 189 ) ,
	VLC_PACK( 5 , 0 , 220 ) ,
	VLC_PACK( 5 , 0 , 246 ) ,
	VLC_PACK( 7 , 0 , 253 ) ,
	VLC_PACK( 5 , 0 , 94 ) ,
	VLC_PACK( 7 , 0 , 190 ) ,
	VLC_PACK( 7 , 0 , 222 ) ,
	VLC_PACK( 5 , 1 , 247 ) ,
	VLC_PACK( 5 , 0 , 158 ) ,
	VLC_PACK( 5 , 0 , 126 ) ,
	VLC_PACK( 5 , 0 , 190 ) ,
	VLC_PACK( 5 , 0 , 249 ) ,
	VLC_PACK( 5 , 0 , 221 ) ,
/* B17_inter_tcoeff.out */
	VLC_PACK( 2 , 4 , 16 ) ,
	VLC_PACK( 2 , 3 , 68 ) ,
	VLC_PACK( 0 , 2 , 84 ) ,
	VLC_PACK( 0 , 1 , 91 ) ,
	VLC_PACK( 1 , 1 , 94 ) ,
	VLC_PACK( 2 , 1 , 96 ) ,
	VLC_PACK( 0 , 0 , 98 ) ,
	VLC_PACK( 7 , 0 , 254 ) ,
	VLC_PACK( 4 , 2 , 508 ) ,
	VLC_PACK( 4 , 2 , 508 ) ,
	VLC_PACK( 4 , 2 , 509 ) ,
	VLC_PACK( 4 , 2 , 509 ) ,
	VLC_PACK( 4 , 3 , 444 ) ,
	VLC_PACK( 4 , 3 , 445 ) ,
	VLC_PACK( 5 , 0 , 190 ) ,
	VLC_PACK( 5 , 0 , 253 ) ,
	VLC_PACK( 2 , 3 , 6 ) ,
	VLC_PACK( 2 , 1 , 14 ) ,
	VLC_PACK( 0 , 2 , 40 ) ,
	VLC_PACK( 7 , 2 , 419 ) ,
	VLC_PACK( 2 , 1 , 46 ) ,
	VLC_PACK( 3 , 4 , 0 ) ,
	VLC_PACK( 2 , 2 , 5 ) ,
	VLC_PACK( 7 , 2 , 426 ) ,
	VLC_PACK( 7 , 1 , 424 ) ,
	VLC_PACK( 7 , 0 , 423 ) ,
	VLC_PACK( 5 , 1 , 246 ) ,
	VLC_PACK( 5 , 2 , 427 ) ,
	VLC_PACK( 5 , 1 , 425 ) ,
	VLC_PACK( 7 , 0 , 253 ) ,
	VLC_PACK( 7 , 0 , 430 ) ,
	VLC_PACK( 4 , 0 , 255 ) ,
	VLC_PACK( 0 , 3 , 2 ) ,
	VLC_PACK( 0 , 2 , 17 ) ,
	VLC_PACK( 5 , 0 , 217 ) ,
	VLC_PACK( 5 , 0 , 187 ) ,
	VLC_PACK( 5 , 0 , 124 ) ,
	VLC_PACK( 5 , 0 , 92 ) ,
	VLC_PACK( 5 , 0 , 60 ) ,
	VLC_PACK( 5 , 0 , 373 ) ,
	VLC_PACK( 5 , 0 , 422 ) ,
	VLC_PACK( 5 , 0 , 421 ) ,
	VLC_PACK( 7 , 0 , 414 ) ,
	VLC_PACK( 7 , 0 , 413 ) ,
	VLC_PACK( 7 , 0 , 412 ) ,
	VLC_PACK( 7 , 0 , 411 ) ,
	VLC_PACK( 7 , 0 , 410 ) ,
	VLC_PACK( 7 , 0 , 409 ) ,
	VLC_PACK( 7 , 0 , 408 ) ,
	VLC_PACK( 7 , 0 , 407 ) ,
	VLC_PACK( 5 , 0 , 243 ) ,
	VLC_PACK( 5 , 0 , 218 ) ,
	VLC_PACK( 5 , 0 , 424 ) ,
	VLC_PACK( 5 , 0 , 423 ) ,
	VLC_PACK( 7 , 0 , 418 ) ,
	VLC_PACK( 7 , 0 , 417 ) ,
	VLC_PACK( 7 , 0 , 416 ) ,
	VLC_PACK( 7 , 0 , 415 ) ,
	VLC_PACK( 5 , 0 , 374 ) ,
	VLC_PACK( 5 , 0 , 375 ) ,
	VLC_PACK( 5 , 0 , 29 ) ,
	VLC_PACK( 5 , 0 , 61 ) ,
	VLC_PACK( 5 , 0 , 93 ) ,
	VLC_PACK( 5 , 0 , 156 ) ,
	VLC_PACK( 5 , 0 , 188 ) ,
	VLC_PACK( 5 , 0 , 219 ) ,
	VLC_PACK( 5 , 1 , 244 ) ,
	VLC_PACK( 7 , 0 , 252 ) ,
	VLC_PACK( 7 , 0 , 221 ) ,
	VLC_PACK( 0 , 2 , 5 ) ,
	VLC_PACK( 7 , 2 , 432 ) ,
	VLC_PACK( 0 , 0 , 11 ) ,
	VLC_PACK( 0 , 0 , 12 ) ,
	VLC_PACK( 5 , 1 , 431 ) ,
	VLC_PACK( 7 , 0 , 436 ) ,
	VLC_PACK( 7 , 0 , 437 ) ,
	VLC_PACK( 7 , 0 , 438 ) ,
	VLC_PACK( 5 , 0 , 433 ) ,
	VLC_PACK( 5 , 0 , 434 ) ,
	VLC_PACK( 5 , 0 , 189 ) ,
	VLC_PACK( 5 , 0 , 220 ) ,
	VLC_PACK( 5 , 0 , 250 ) ,
	VLC_PACK( 5 , 1 , 248 ) ,
	VLC_PACK( 7 , 0 , 431 ) ,
	VLC_PACK( 5 , 0 , 125 ) ,
	VLC_PACK( 5 , 0 , 157 ) ,
	VLC_PACK( 7 , 0 , 439 ) ,
	VLC_PACK( 7 , 0 , 30 ) ,
	VLC_PACK( 7 , 0 , 62 ) ,
	VLC_PACK( 7 , 0 , 94 ) ,
	VLC_PACK( 5 , 0 , 435 ) ,
	VLC_PACK( 5 , 0 , 436 ) ,
	VLC_PACK( 5 , 0 , 437 ) ,
	VLC_PACK( 5 , 0 , 251 ) ,
	VLC_PACK( 7 , 0 , 126 ) ,
	VLC_PACK( 7 , 0 , 158 ) ,
	VLC_PACK( 7 , 0 , 190 ) ,
	VLC_PACK( 7 , 0 , 222 ) ,
	VLC_PACK( 5 , 1 , 438 ) ,
	VLC_PACK( 5 , 0 , 30 ) ,
	VLC_PACK( 5 , 0 , 62 ) ,
	VLC_PACK( 5 , 0 , 94 ) ,
	VLC_PACK( 5 , 0 , 252 ) ,
	VLC_PACK( 5 , 0 , 221 ) ,
	VLC_PACK( 5 , 0 , 126 ) ,
	VLC_PACK( 5 , 0 , 158 ) ,
/* B23_rvlc_tcoeff_0.out */
	VLC_PACK( 5 , 0 , 255 ) ,
	VLC_PACK( 5 , 0 , 222 ) ,
	VLC_PACK( 0 , 0 , 14 ) ,
	VLC_PACK( 0 , 1 , 15 ) ,
	VLC_PACK( 0 , 0 , 44 ) ,
	VLC_PACK( 0 , 1 , 45 ) ,
	VLC_PACK( 0 , 1 , 74 ) ,
	VLC_PACK( 0 , 2 , 103 ) ,
	VLC_PACK( 0 , 1 , 230 ) ,
	VLC_PACK( 0 , 0 , 256 ) ,
	VLC_PACK( 5 , 0 , 252 ) ,
	VLC_PACK( 7 , 0 , 254 ) ,
	VLC_PACK( 4 , 3 , 508 ) ,
	VLC_PACK( 4 , 3 , 509 ) ,
	VLC_PACK( 4 , 3 , 506 ) ,
	VLC_PACK( 4 , 3 , 507 ) ,
	VLC_PACK( 5 , 0 , 190 ) ,
	VLC_PACK( 5 , 0 , 158 ) ,
	VLC_PACK( 5 , 0 , 126 ) ,
	VLC_PACK( 5 , 0 , 94 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 0 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 62 ) ,
	VLC_PACK( 5 , 0 , 30 ) ,
	VLC_PACK( 5 , 0 , 439 ) ,
	VLC_PACK( 5 , 0 , 438 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 0 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 437 ) ,
	VLC_PACK( 5 , 0 , 93 ) ,
	VLC_PACK( 5 , 0 , 436 ) ,
	VLC_PACK( 5 , 0 , 435 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 0 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 434 ) ,
	VLC_PACK( 5 , 0 , 374 ) ,
	VLC_PACK( 5 , 0 , 373 ) ,
	VLC_PACK( 5 , 0 , 123 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 0 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 433 ) ,
	VLC_PACK( 5 , 0 , 432 ) ,
	VLC_PACK( 5 , 0 , 431 ) ,
	VLC_PACK( 5 , 0 , 430 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 3 , 1 , 0 ) ,
	VLC_PACK( 5 , 0 , 228 ) ,
	VLC_PACK( 5 , 0 , 150 ) ,
	VLC_PACK( 5 , 0 , 221 ) ,
	VLC_PACK( 5 , 0 , 251 ) ,
	VLC_PACK( 5 , 0 , 250 ) ,
	VLC_PACK( 5 , 0 , 249 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 0 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 189 ) ,
	VLC_PACK( 5 , 0 , 220 ) ,
	VLC_PACK( 5 , 0 , 157 ) ,
	VLC_PACK( 5 , 0 , 125 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 0 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 188 ) ,
	VLC_PACK( 5 , 0 , 156 ) ,
	VLC_PACK( 5 , 0 , 61 ) ,
	VLC_PACK( 5 , 0 , 29 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 0 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 92 ) ,
	VLC_PACK( 5 , 0 , 60 ) ,
	VLC_PACK( 5 , 0 , 91 ) ,
	VLC_PACK( 5 , 0 , 59 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 0 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 372 ) ,
	VLC_PACK( 5 , 0 , 311 ) ,
	VLC_PACK( 5 , 0 , 429 ) ,
	VLC_PACK( 5 , 0 , 503 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 3 , 1 , 0 ) ,
	VLC_PACK( 5 , 0 , 58 ) ,
	VLC_PACK( 5 , 0 , 26 ) ,
	VLC_PACK( 7 , 0 , 158 ) ,
	VLC_PACK( 7 , 0 , 126 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 0 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 248 ) ,
	VLC_PACK( 7 , 0 , 30 ) ,
	VLC_PACK( 5 , 0 , 219 ) ,
	VLC_PACK( 5 , 0 , 218 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 0 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 217 ) ,
	VLC_PACK( 5 , 0 , 245 ) ,
	VLC_PACK( 5 , 0 , 375 ) ,
	VLC_PACK( 5 , 0 , 124 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 0 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 28 ) ,
	VLC_PACK( 5 , 0 , 155 ) ,
	VLC_PACK( 5 , 0 , 154 ) ,
	VLC_PACK( 5 , 0 , 122 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 0 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 310 ) ,
	VLC_PACK( 5 , 0 , 27 ) ,
	VLC_PACK( 5 , 0 , 90 ) ,
	VLC_PACK( 5 , 0 , 121 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 3 , 1 , 0 ) ,
	VLC_PACK( 5 , 0 , 502 ) ,
	VLC_PACK( 5 , 0 , 371 ) ,
	VLC_PACK( 7 , 0 , 439 ) ,
	VLC_PACK( 7 , 0 , 438 ) ,
	VLC_PACK( 0 , 0 , 6 ) ,
	VLC_PACK( 0 , 1 , 7 ) ,
	VLC_PACK( 0 , 0 , 27 ) ,
	VLC_PACK( 0 , 1 , 28 ) ,
	VLC_PACK( 0 , 1 , 48 ) ,
	VLC_PACK( 0 , 2 , 68 ) ,
	VLC_PACK( 5 , 0 , 247 ) ,
	VLC_PACK( 5 , 0 , 246 ) ,
	VLC_PACK( 5 , 0 , 244 ) ,
	VLC_PACK( 7 , 0 , 221 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 0 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 187 ) ,
	VLC_PACK( 5 , 0 , 216 ) ,
	VLC_PACK( 5 , 0 , 186 ) ,
	VLC_PACK( 5 , 0 , 185 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 0 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 213 ) ,
	VLC_PACK( 5 , 0 , 237 ) ,
	VLC_PACK( 5 , 0 , 153 ) ,
	VLC_PACK( 5 , 0 , 184 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 2 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 89 ) ,
	VLC_PACK( 5 , 0 , 152 ) ,
	VLC_PACK( 3 , 1 , 0 ) ,
	VLC_PACK( 7 , 0 , 218 ) ,
	VLC_PACK( 5 , 0 , 428 ) ,
	VLC_PACK( 7 , 0 , 253 ) ,
	VLC_PACK( 7 , 0 , 435 ) ,
	VLC_PACK( 7 , 0 , 432 ) ,
	VLC_PACK( 7 , 0 , 431 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 0 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 243 ) ,
	VLC_PACK( 5 , 0 , 242 ) ,
	VLC_PACK( 5 , 0 , 215 ) ,
	VLC_PACK( 5 , 0 , 214 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 0 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 236 ) ,
	VLC_PACK( 5 , 0 , 233 ) ,
	VLC_PACK( 5 , 0 , 183 ) ,
	VLC_PACK( 5 , 0 , 182 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 2 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 151 ) ,
	VLC_PACK( 5 , 0 , 181 ) ,
	VLC_PACK( 3 , 1 , 0 ) ,
	VLC_PACK( 7 , 0 , 370 ) ,
	VLC_PACK( 7 , 0 , 188 ) ,
	VLC_PACK( 7 , 0 , 430 ) ,
	VLC_PACK( 7 , 0 , 429 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 0 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 241 ) ,
	VLC_PACK( 7 , 0 , 426 ) ,
	VLC_PACK( 5 , 0 , 240 ) ,
	VLC_PACK( 5 , 0 , 239 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 0 , 1 , 3 ) ,
	VLC_PACK( 7 , 0 , 220 ) ,
	VLC_PACK( 7 , 0 , 157 ) ,
	VLC_PACK( 5 , 0 , 212 ) ,
	VLC_PACK( 5 , 0 , 235 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 2 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 180 ) ,
	VLC_PACK( 5 , 0 , 211 ) ,
	VLC_PACK( 3 , 1 , 0 ) ,
	VLC_PACK( 7 , 0 , 405 ) ,
	VLC_PACK( 7 , 0 , 406 ) ,
	VLC_PACK( 7 , 0 , 425 ) ,
	VLC_PACK( 7 , 0 , 424 ) ,
	VLC_PACK( 0 , 0 , 6 ) ,
	VLC_PACK( 0 , 1 , 7 ) ,
	VLC_PACK( 0 , 0 , 18 ) ,
	VLC_PACK( 0 , 1 , 19 ) ,
	VLC_PACK( 0 , 1 , 27 ) ,
	VLC_PACK( 2 , 2 , 35 ) ,
	VLC_PACK( 5 , 0 , 238 ) ,
	VLC_PACK( 7 , 0 , 252 ) ,
	VLC_PACK( 7 , 0 , 125 ) ,
	VLC_PACK( 7 , 0 , 418 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 0 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 234 ) ,
	VLC_PACK( 5 , 0 , 232 ) ,
	VLC_PACK( 5 , 0 , 210 ) ,
	VLC_PACK( 5 , 0 , 231 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 3 , 1 , 0 ) ,
	VLC_PACK( 7 , 0 , 404 ) ,
	VLC_PACK( 7 , 0 , 403 ) ,
	VLC_PACK( 7 , 0 , 189 ) ,
	VLC_PACK( 7 , 0 , 421 ) ,
	VLC_PACK( 7 , 0 , 417 ) ,
	VLC_PACK( 7 , 0 , 416 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 2 , 1 , 3 ) ,
	VLC_PACK( 7 , 0 , 251 ) ,
	VLC_PACK( 7 , 0 , 93 ) ,
	VLC_PACK( 3 , 1 , 0 ) ,
	VLC_PACK( 5 , 0 , 229 ) ,
	VLC_PACK( 5 , 0 , 230 ) ,
	VLC_PACK( 7 , 0 , 415 ) ,
	VLC_PACK( 7 , 0 , 414 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 2 , 1 , 3 ) ,
	VLC_PACK( 7 , 0 , 61 ) ,
	VLC_PACK( 7 , 0 , 29 ) ,
	VLC_PACK( 3 , 1 , 0 ) ,
	VLC_PACK( 7 , 0 , 219 ) ,
	VLC_PACK( 7 , 0 , 250 ) ,
	VLC_PACK( 2 , 2 , 4 ) ,
	VLC_PACK( 2 , 1 , 7 ) ,
	VLC_PACK( 7 , 0 , 374 ) ,
	VLC_PACK( 7 , 0 , 375 ) ,
	VLC_PACK( 3 , 2 , 0 ) ,
	VLC_PACK( 3 , 2 , 0 ) ,
	VLC_PACK( 7 , 0 , 409 ) ,
	VLC_PACK( 7 , 0 , 371 ) ,
	VLC_PACK( 3 , 1 , 0 ) ,
	VLC_PACK( 7 , 0 , 372 ) ,
	VLC_PACK( 7 , 0 , 373 ) ,
	VLC_PACK( 0 , 1 , 4 ) ,
	VLC_PACK( 0 , 0 , 24 ) ,
	VLC_PACK( 7 , 0 , 94 ) ,
	VLC_PACK( 7 , 0 , 62 ) ,
	VLC_PACK( 0 , 1 , 4 ) ,
	VLC_PACK( 0 , 0 , 18 ) ,
	VLC_PACK( 7 , 0 , 434 ) ,
	VLC_PACK( 7 , 0 , 433 ) ,
	VLC_PACK( 0 , 1 , 4 ) ,
	VLC_PACK( 0 , 0 , 12 ) ,
	VLC_PACK( 7 , 0 , 423 ) ,
	VLC_PACK( 7 , 0 , 422 ) ,
	VLC_PACK( 1 , 1 , 4 ) ,
	VLC_PACK( 0 , 0 , 6 ) ,
	VLC_PACK( 7 , 0 , 413 ) ,
	VLC_PACK( 7 , 0 , 412 ) ,
	VLC_PACK( 3 , 1 , 0 ) ,
	VLC_PACK( 7 , 0 , 408 ) ,
	VLC_PACK( 7 , 0 , 407 ) ,
	VLC_PACK( 7 , 0 , 411 ) ,
	VLC_PACK( 7 , 0 , 410 ) ,
	VLC_PACK( 7 , 0 , 420 ) ,
	VLC_PACK( 7 , 0 , 419 ) ,
	VLC_PACK( 7 , 0 , 428 ) ,
	VLC_PACK( 7 , 0 , 427 ) ,
	VLC_PACK( 7 , 0 , 437 ) ,
	VLC_PACK( 7 , 0 , 436 ) ,
	VLC_PACK( 7 , 0 , 222 ) ,
	VLC_PACK( 7 , 0 , 190 ) ,
/* B23_rvlc_tcoeff_1.out */
	VLC_PACK( 5 , 0 , 255 ) ,
	VLC_PACK( 5 , 0 , 253 ) ,
	VLC_PACK( 0 , 0 , 14 ) ,
	VLC_PACK( 0 , 1 , 15 ) ,
	VLC_PACK( 0 , 0 , 44 ) ,
	VLC_PACK( 0 , 1 , 45 ) ,
	VLC_PACK( 0 , 1 , 74 ) ,
	VLC_PACK( 0 , 2 , 103 ) ,
	VLC_PACK( 0 , 1 , 230 ) ,
	VLC_PACK( 0 , 0 , 256 ) ,
	VLC_PACK( 5 , 0 , 190 ) ,
	VLC_PACK( 7 , 0 , 254 ) ,
	VLC_PACK( 4 , 3 , 508 ) ,
	VLC_PACK( 4 , 3 , 509 ) ,
	VLC_PACK( 4 , 3 , 444 ) ,
	VLC_PACK( 4 , 3 , 445 ) ,
	VLC_PACK( 5 , 0 , 252 ) ,
	VLC_PACK( 5 , 0 , 158 ) ,
	VLC_PACK( 5 , 0 , 221 ) ,
	VLC_PACK( 5 , 0 , 62 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 0 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 251 ) ,
	VLC_PACK( 5 , 0 , 189 ) ,
	VLC_PACK( 5 , 0 , 250 ) ,
	VLC_PACK( 5 , 0 , 249 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 0 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 248 ) ,
	VLC_PACK( 5 , 0 , 219 ) ,
	VLC_PACK( 5 , 0 , 247 ) ,
	VLC_PACK( 5 , 0 , 246 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 0 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 245 ) ,
	VLC_PACK( 5 , 0 , 244 ) ,
	VLC_PACK( 5 , 0 , 243 ) ,
	VLC_PACK( 5 , 0 , 216 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 0 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 242 ) ,
	VLC_PACK( 5 , 0 , 241 ) ,
	VLC_PACK( 5 , 0 , 238 ) ,
	VLC_PACK( 5 , 0 , 237 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 3 , 1 , 0 ) ,
	VLC_PACK( 5 , 0 , 236 ) ,
	VLC_PACK( 5 , 0 , 152 ) ,
	VLC_PACK( 5 , 0 , 126 ) ,
	VLC_PACK( 5 , 0 , 94 ) ,
	VLC_PACK( 5 , 0 , 30 ) ,
	VLC_PACK( 5 , 0 , 439 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 0 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 438 ) ,
	VLC_PACK( 5 , 0 , 437 ) ,
	VLC_PACK( 5 , 0 , 220 ) ,
	VLC_PACK( 5 , 0 , 157 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 0 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 188 ) ,
	VLC_PACK( 5 , 0 , 93 ) ,
	VLC_PACK( 5 , 0 , 218 ) ,
	VLC_PACK( 5 , 0 , 156 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 0 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 217 ) ,
	VLC_PACK( 5 , 0 , 187 ) ,
	VLC_PACK( 5 , 0 , 186 ) ,
	VLC_PACK( 5 , 0 , 155 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 0 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 240 ) ,
	VLC_PACK( 5 , 0 , 239 ) ,
	VLC_PACK( 5 , 0 , 214 ) ,
	VLC_PACK( 5 , 0 , 213 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 3 , 1 , 0 ) ,
	VLC_PACK( 5 , 0 , 122 ) ,
	VLC_PACK( 5 , 0 , 27 ) ,
	VLC_PACK( 7 , 0 , 158 ) ,
	VLC_PACK( 7 , 0 , 126 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 0 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 436 ) ,
	VLC_PACK( 7 , 0 , 30 ) ,
	VLC_PACK( 5 , 0 , 125 ) ,
	VLC_PACK( 5 , 0 , 435 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 0 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 432 ) ,
	VLC_PACK( 5 , 0 , 431 ) ,
	VLC_PACK( 5 , 0 , 61 ) ,
	VLC_PACK( 5 , 0 , 29 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 0 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 124 ) ,
	VLC_PACK( 5 , 0 , 92 ) ,
	VLC_PACK( 5 , 0 , 60 ) ,
	VLC_PACK( 5 , 0 , 28 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 0 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 215 ) ,
	VLC_PACK( 5 , 0 , 154 ) ,
	VLC_PACK( 5 , 0 , 185 ) ,
	VLC_PACK( 5 , 0 , 184 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 3 , 1 , 0 ) ,
	VLC_PACK( 5 , 0 , 366 ) ,
	VLC_PACK( 5 , 0 , 410 ) ,
	VLC_PACK( 7 , 0 , 439 ) ,
	VLC_PACK( 7 , 0 , 438 ) ,
	VLC_PACK( 0 , 0 , 6 ) ,
	VLC_PACK( 0 , 1 , 7 ) ,
	VLC_PACK( 0 , 0 , 27 ) ,
	VLC_PACK( 0 , 1 , 28 ) ,
	VLC_PACK( 0 , 1 , 48 ) ,
	VLC_PACK( 0 , 2 , 68 ) ,
	VLC_PACK( 5 , 0 , 434 ) ,
	VLC_PACK( 5 , 0 , 433 ) ,
	VLC_PACK( 5 , 0 , 430 ) ,
	VLC_PACK( 7 , 0 , 221 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 0 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 375 ) ,
	VLC_PACK( 5 , 0 , 374 ) ,
	VLC_PACK( 5 , 0 , 373 ) ,
	VLC_PACK( 5 , 0 , 426 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 0 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 372 ) ,
	VLC_PACK( 5 , 0 , 420 ) ,
	VLC_PACK( 5 , 0 , 123 ) ,
	VLC_PACK( 5 , 0 , 91 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 2 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 153 ) ,
	VLC_PACK( 5 , 0 , 59 ) ,
	VLC_PACK( 3 , 1 , 0 ) ,
	VLC_PACK( 7 , 0 , 218 ) ,
	VLC_PACK( 5 , 0 , 409 ) ,
	VLC_PACK( 7 , 0 , 253 ) ,
	VLC_PACK( 7 , 0 , 435 ) ,
	VLC_PACK( 7 , 0 , 432 ) ,
	VLC_PACK( 7 , 0 , 431 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 0 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 429 ) ,
	VLC_PACK( 5 , 0 , 428 ) ,
	VLC_PACK( 5 , 0 , 425 ) ,
	VLC_PACK( 5 , 0 , 424 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 0 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 419 ) ,
	VLC_PACK( 5 , 0 , 418 ) ,
	VLC_PACK( 5 , 0 , 311 ) ,
	VLC_PACK( 5 , 0 , 371 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 2 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 310 ) ,
	VLC_PACK( 5 , 0 , 370 ) ,
	VLC_PACK( 3 , 1 , 0 ) ,
	VLC_PACK( 7 , 0 , 370 ) ,
	VLC_PACK( 7 , 0 , 188 ) ,
	VLC_PACK( 7 , 0 , 430 ) ,
	VLC_PACK( 7 , 0 , 429 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 0 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 427 ) ,
	VLC_PACK( 7 , 0 , 426 ) ,
	VLC_PACK( 5 , 0 , 423 ) ,
	VLC_PACK( 5 , 0 , 422 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 0 , 1 , 3 ) ,
	VLC_PACK( 7 , 0 , 220 ) ,
	VLC_PACK( 7 , 0 , 157 ) ,
	VLC_PACK( 5 , 0 , 417 ) ,
	VLC_PACK( 5 , 0 , 416 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 2 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 369 ) ,
	VLC_PACK( 5 , 0 , 368 ) ,
	VLC_PACK( 3 , 1 , 0 ) ,
	VLC_PACK( 7 , 0 , 405 ) ,
	VLC_PACK( 7 , 0 , 406 ) ,
	VLC_PACK( 7 , 0 , 425 ) ,
	VLC_PACK( 7 , 0 , 424 ) ,
	VLC_PACK( 0 , 0 , 6 ) ,
	VLC_PACK( 0 , 1 , 7 ) ,
	VLC_PACK( 0 , 0 , 18 ) ,
	VLC_PACK( 0 , 1 , 19 ) ,
	VLC_PACK( 0 , 1 , 27 ) ,
	VLC_PACK( 2 , 2 , 35 ) ,
	VLC_PACK( 5 , 0 , 421 ) ,
	VLC_PACK( 7 , 0 , 252 ) ,
	VLC_PACK( 7 , 0 , 125 ) ,
	VLC_PACK( 7 , 0 , 418 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 0 , 1 , 3 ) ,
	VLC_PACK( 5 , 0 , 415 ) ,
	VLC_PACK( 5 , 0 , 414 ) ,
	VLC_PACK( 5 , 0 , 367 ) ,
	VLC_PACK( 5 , 0 , 413 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 3 , 1 , 0 ) ,
	VLC_PACK( 7 , 0 , 404 ) ,
	VLC_PACK( 7 , 0 , 403 ) ,
	VLC_PACK( 7 , 0 , 189 ) ,
	VLC_PACK( 7 , 0 , 421 ) ,
	VLC_PACK( 7 , 0 , 417 ) ,
	VLC_PACK( 7 , 0 , 416 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 2 , 1 , 3 ) ,
	VLC_PACK( 7 , 0 , 251 ) ,
	VLC_PACK( 7 , 0 , 93 ) ,
	VLC_PACK( 3 , 1 , 0 ) ,
	VLC_PACK( 5 , 0 , 411 ) ,
	VLC_PACK( 5 , 0 , 412 ) ,
	VLC_PACK( 7 , 0 , 415 ) ,
	VLC_PACK( 7 , 0 , 414 ) ,
	VLC_PACK( 0 , 0 , 2 ) ,
	VLC_PACK( 2 , 1 , 3 ) ,
	VLC_PACK( 7 , 0 , 61 ) ,
	VLC_PACK( 7 , 0 , 29 ) ,
	VLC_PACK( 3 , 1 , 0 ) ,
	VLC_PACK( 7 , 0 , 219 ) ,
	VLC_PACK( 7 , 0 , 250 ) ,
	VLC_PACK( 2 , 2 , 4 ) ,
	VLC_PACK( 2 , 1 , 7 ) ,
	VLC_PACK( 7 , 0 , 374 ) ,
	VLC_PACK( 7 , 0 , 375 ) ,
	VLC_PACK( 3 , 2 , 0 ) ,
	VLC_PACK( 3 , 2 , 0 ) ,
	VLC_PACK( 7 , 0 , 409 ) ,
	VLC_PACK( 7 , 0 , 371 ) ,
	VLC_PACK( 3 , 1 , 0 ) ,
	VLC_PACK( 7 , 0 , 372 ) ,
	VLC_PACK( 7 , 0 , 373 ) ,
	VLC_PACK( 0 , 1 , 4 ) ,
	VLC_PACK( 0 , 0 , 24 ) ,
	VLC_PACK( 7 , 0 , 94 ) ,
	VLC_PACK( 7 , 0 , 62 ) ,
	VLC_PACK( 0 , 1 , 4 ) ,
	VLC_PACK( 0 , 0 , 18 ) ,
	VLC_PACK( 7 , 0 , 434 ) ,
	VLC_PACK( 7 , 0 , 433 ) ,
	VLC_PACK( 0 , 1 , 4 ) ,
	VLC_PACK( 0 , 0 , 12 ) ,
	VLC_PACK( 7 , 0 , 423 ) ,
	VLC_PACK( 7 , 0 , 422 ) ,
	VLC_PACK( 1 , 1 , 4 ) ,
	VLC_PACK( 0 , 0 , 6 ) ,
	VLC_PACK( 7 , 0 , 413 ) ,
	VLC_PACK( 7 , 0 , 412 ) ,
	VLC_PACK( 3 , 1 , 0 ) ,
	VLC_PACK( 7 , 0 , 408 ) ,
	VLC_PACK( 7 , 0 , 407 ) ,
	VLC_PACK( 7 , 0 , 411 ) ,
	VLC_PACK( 7 , 0 , 410 ) ,
	VLC_PACK( 7 , 0 , 420 ) ,
	VLC_PACK( 7 , 0 , 419 ) ,
	VLC_PACK( 7 , 0 , 428 ) ,
	VLC_PACK( 7 , 0 , 427 ) ,
	VLC_PACK( 7 , 0 , 437 ) ,
	VLC_PACK( 7 , 0 , 436 ) ,
	VLC_PACK( 7 , 0 , 222 ) ,
	VLC_PACK( 7 , 0 , 190 ) ,
/* B29_MVDs.out */
	VLC_PACK( 4 , 0 , 0 ) ,
	VLC_PACK( 5 , 0 , 1 ) ,
	VLC_PACK( 5 , 0 , 2 ) ,
	VLC_PACK( 5 , 0 , 3 ) ,
	VLC_PACK( 5 , 0 , 4 ) ,
	VLC_PACK( 5 , 0 , 5 ) ,
	VLC_PACK( 1 , 5 , 1 ) ,
	VLC_PACK( 5 , 0 , 6 ) ,
	VLC_PACK( 5 , 0 , 7 ) ,
	VLC_PACK( 5 , 0 , 8 ) ,
	VLC_PACK( 5 , 0 , 9 ) ,
	VLC_PACK( 5 , 0 , 10 ) ,
	VLC_PACK( 5 , 0 , 11 ) ,
	VLC_PACK( 1 , 3 , 1 ) ,
	VLC_PACK( 5 , 0 , 12 ) ,
	VLC_PACK( 5 , 0 , 13 ) ,
	VLC_PACK( 5 , 0 , 14 ) ,
	VLC_PACK( 5 , 0 , 15 ) ,
	VLC_PACK( 5 , 1 , 16 ) ,
/* B30_MVDs.out */
	VLC_PACK( 5 , 0 , 1 ) ,
	VLC_PACK( 5 , 0 , 2 ) ,
	VLC_PACK( 5 , 0 , 3 ) ,
	VLC_PACK( 5 , 0 , 4 ) ,
	VLC_PACK( 5 , 0 , 5 ) ,
	VLC_PACK( 5 , 0 , 6 ) ,
	VLC_PACK( 1 , 5 , 1 ) ,
	VLC_PACK( 5 , 0 , 7 ) ,
	VLC_PACK( 5 , 0 , 8 ) ,
	VLC_PACK( 5 , 0 , 9 ) ,
	VLC_PACK( 5 , 0 , 10 ) ,
	VLC_PACK( 5 , 0 , 11 ) ,
	VLC_PACK( 5 , 0 , 12 ) ,
	VLC_PACK( 1 , 2 , 1 ) ,
	VLC_PACK( 5 , 0 , 13 ) ,
	VLC_PACK( 5 , 0 , 14 ) ,
	VLC_PACK( 5 , 0 , 15 ) ,
	VLC_PACK( 5 , 1 , 16 ) ,
/* B31_conv_ratio.out */
	VLC_PACK( 4 , 0 , 1 ) ,
	VLC_PACK( 4 , 1 , 2 ) ,
	VLC_PACK( 4 , 1 , 4 ) ,
};


struct context_MPEG4_s {
    struct context_DEC_s dec_ctx;
    object_context_p obj_context; /* back reference */

    uint32_t profile;

    /* Picture parameters */
    VAPictureParameterBufferMPEG4 *pic_params;
    object_surface_p forward_ref_surface;
    object_surface_p backward_ref_surface;

    uint32_t display_picture_width;    /* in pixels */
    uint32_t display_picture_height;    /* in pixels */

    uint32_t coded_picture_width;    /* in pixels */
    uint32_t coded_picture_height;    /* in pixels */

    uint32_t picture_width_mb;        /* in macroblocks */
    uint32_t picture_height_mb;        /* in macroblocks */
    uint32_t size_mb;                /* in macroblocks */

    uint32_t FEControl;
    uint32_t FE_SPS0;
    uint32_t FE_VOP_PPS0;
    uint32_t FE_VOP_SPS0;
    uint32_t FE_PICSH_PPS0;

    uint32_t BE_SPS0;
    uint32_t BE_SPS1;
    uint32_t BE_VOP_PPS0;
    uint32_t BE_VOP_SPS0;
    uint32_t BE_VOP_SPS1;
    uint32_t BE_PICSH_PPS0;

    /* IQ Matrix */
    uint32_t qmatrix_data[MAX_QUANT_TABLES][16];
    int load_non_intra_quant_mat;
    int load_intra_quant_mat;

    /* VLC packed data */
    struct psb_buffer_s vlc_packed_table;

    /* FE state buffer */
    struct psb_buffer_s preload_buffer;

    struct psb_buffer_s *data_partition_buffer0;
    struct psb_buffer_s *data_partition_buffer1;

    uint32_t field_type;
};

typedef struct context_MPEG4_s *context_MPEG4_p;

#define INIT_CONTEXT_MPEG4    context_MPEG4_p ctx = (context_MPEG4_p) obj_context->format_data;

#define SURFACE(id)    ((object_surface_p) object_heap_lookup( &ctx->obj_context->driver_data->surface_heap, id ))

static const char *psb__debug_picture_coding_str(unsigned char vop_coding_type)
{
    switch (vop_coding_type) {
    case PICTURE_CODING_I:
        return ("PICTURE_CODING_I");
    case PICTURE_CODING_P:
        return ("PICTURE_CODING_P");
    case PICTURE_CODING_B:
        return ("PICTURE_CODING_B");
    case PICTURE_CODING_S:
        return ("PICTURE_CODING_S");
    }
    return ("UNKNOWN!!!");
}


static void pnw_MPEG4_QueryConfigAttributes(
    VAProfile profile,
    VAEntrypoint entrypoint,
    VAConfigAttrib *attrib_list,
    int num_attribs)
{
    int i;
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "pnw_MPEG4_QueryConfigAttributes\n");

    for (i = 0; i < num_attribs; i++) {
        switch (attrib_list[i].type) {
        case VAConfigAttribMaxPictureWidth:
            if (entrypoint == VAEntrypointVLD) {
                if (profile == VAProfileMPEG4AdvancedSimple)
                    attrib_list[i].value = HW_SUPPORTED_MAX_PICTURE_WIDTH_MPEG4;
                else if(profile == VAProfileH263Baseline)
                    attrib_list[i].value = HW_SUPPORTED_MAX_PICTURE_WIDTH_H263;
                else
                    attrib_list[i].value = VA_ATTRIB_NOT_SUPPORTED;
            }
            else
                attrib_list[i].value = VA_ATTRIB_NOT_SUPPORTED;
            break;
        case VAConfigAttribMaxPictureHeight:
            if (entrypoint == VAEntrypointVLD) {
                if (profile == VAProfileMPEG4AdvancedSimple)
                    attrib_list[i].value = HW_SUPPORTED_MAX_PICTURE_HEIGHT_MPEG4;
                else if(profile == VAProfileH263Baseline)
                    attrib_list[i].value = HW_SUPPORTED_MAX_PICTURE_HEIGHT_H263;
            }
            else
                attrib_list[i].value = VA_ATTRIB_NOT_SUPPORTED;
            break;
        default:
            break;
        }
    }
}

static VAStatus pnw_MPEG4_ValidateConfig(
    object_config_p obj_config)
{
    int i;
    /* Check all attributes */
    for (i = 0; i < obj_config->attrib_count; i++) {
        switch (obj_config->attrib_list[i].type) {
        case VAConfigAttribRTFormat:
            /* Ignore */
            break;

        default:
            return VA_STATUS_ERROR_ATTR_NOT_SUPPORTED;
        }
    }

    return VA_STATUS_SUCCESS;
}

static VAStatus psb__MPEG4_check_legal_picture(object_context_p obj_context, object_config_p obj_config)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    CHECK_CONTEXT(obj_context);

    CHECK_CONFIG(obj_config);

    /* MSVDX decode capability for MPEG4:
     *     SP@L3
     *     ASP@L5
     *
     * Refer to the "MSVDX MPEG4 decode capability" table of "Poulsbo Media Software Overview".
     */
    switch (obj_config->profile) {
    case VAProfileMPEG4Simple:
    case VAProfileMPEG4AdvancedSimple:
        if ((obj_context->picture_width <= 0) || (obj_context->picture_height <= 0)) {
            vaStatus = VA_STATUS_ERROR_RESOLUTION_NOT_SUPPORTED;
        }
        break;

    default:
        vaStatus = VA_STATUS_ERROR_UNSUPPORTED_PROFILE;
        break;
    }

    return vaStatus;
}

static void pnw_MPEG4_DestroyContext(object_context_p obj_context);
static void psb__MPEG4_process_slice_data(context_DEC_p dec_ctx, VASliceParameterBufferBase *vld_slice_param);
static void psb__MPEG4_end_slice(context_DEC_p dec_ctx);
static void psb__MPEG4_begin_slice(context_DEC_p dec_ctx, VASliceParameterBufferBase *vld_slice_param);
static VAStatus pnw_MPEG4_process_buffer(context_DEC_p dec_ctx, object_buffer_p buffer);

static VAStatus pnw_MPEG4_CreateContext(
    object_context_p obj_context,
    object_config_p obj_config)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    context_MPEG4_p ctx;
    /* Validate flag */
    /* Validate picture dimensions */
    vaStatus = psb__MPEG4_check_legal_picture(obj_context, obj_config);
    CHECK_VASTATUS();

    ctx = (context_MPEG4_p) calloc(1, sizeof(struct context_MPEG4_s));
    CHECK_ALLOCATION(ctx);

    obj_context->format_data = (void*) ctx;
    ctx->obj_context = obj_context;
    ctx->pic_params = NULL;
    ctx->load_non_intra_quant_mat = FALSE;
    ctx->load_intra_quant_mat = FALSE;

    ctx->dec_ctx.begin_slice = psb__MPEG4_begin_slice;
    ctx->dec_ctx.process_slice = psb__MPEG4_process_slice_data;
    ctx->dec_ctx.end_slice = psb__MPEG4_end_slice;
    ctx->dec_ctx.process_buffer = pnw_MPEG4_process_buffer;

    ctx->data_partition_buffer0 = NULL;
    ctx->data_partition_buffer1 = NULL;

    switch (obj_config->profile) {
    case VAProfileMPEG4Simple:
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "MPEG4_PROFILE_SIMPLE\n");
        ctx->profile = MPEG4_PROFILE_SIMPLE;
        break;

    case VAProfileMPEG4AdvancedSimple:
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "MPEG4_PROFILE_ASP\n");
        ctx->profile = MPEG4_PROFILE_ASP;
        break;

    default:
        ASSERT(0 == 1);
        vaStatus = VA_STATUS_ERROR_UNKNOWN;
    }

    // TODO

    if (vaStatus == VA_STATUS_SUCCESS) {
        vaStatus = psb_buffer_create(obj_context->driver_data,
                                     FE_STATE_BUFFER_SIZE,
                                     psb_bt_vpu_only,
                                     &ctx->preload_buffer);
        DEBUG_FAILURE;
    }
    ctx->dec_ctx.preload_buffer = &ctx->preload_buffer;

    if (vaStatus == VA_STATUS_SUCCESS) {
        vaStatus = psb_buffer_create(obj_context->driver_data,
                                     sizeof(gaui16mpeg4VlcTableDataPacked),
                                     psb_bt_cpu_vpu,
                                     &ctx->vlc_packed_table);
        DEBUG_FAILURE;
    }
    if (vaStatus == VA_STATUS_SUCCESS) {
        unsigned char *vlc_packed_data_address;
        if (0 ==  psb_buffer_map(&ctx->vlc_packed_table, &vlc_packed_data_address)) {
            memcpy(vlc_packed_data_address, gaui16mpeg4VlcTableDataPacked, sizeof(gaui16mpeg4VlcTableDataPacked));
            psb_buffer_unmap(&ctx->vlc_packed_table);
        } else {
            vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
            DEBUG_FAILURE;
        }
    }

    if (vaStatus == VA_STATUS_SUCCESS) {
        vaStatus = vld_dec_CreateContext(&ctx->dec_ctx, obj_context);
        DEBUG_FAILURE;
    }

    ctx->field_type = 2;

    if (vaStatus != VA_STATUS_SUCCESS) {
        pnw_MPEG4_DestroyContext(obj_context);
    }

    return vaStatus;
}

static void pnw_MPEG4_DestroyContext(
    object_context_p obj_context)
{
    INIT_CONTEXT_MPEG4
    int i;

    vld_dec_DestroyContext(&ctx->dec_ctx);

    psb_buffer_destroy(&ctx->vlc_packed_table);
    psb_buffer_destroy(&ctx->preload_buffer);

    if(ctx->data_partition_buffer0)
        psb_buffer_destroy(ctx->data_partition_buffer0);

    if(ctx->data_partition_buffer1)
        psb_buffer_destroy(ctx->data_partition_buffer1);

    ctx->data_partition_buffer0 = NULL;
    ctx->data_partition_buffer1 = NULL;

    if (ctx->pic_params) {
        free(ctx->pic_params);
        ctx->pic_params = NULL;
    }

    free(obj_context->format_data);
    obj_context->format_data = NULL;
}

static VAStatus psb__MPEG4_process_picture_param(context_MPEG4_p ctx, object_buffer_p obj_buffer)
{
    VAStatus vaStatus;
    object_surface_p obj_surface = ctx->obj_context->current_render_target;

    ASSERT(obj_buffer->type == VAPictureParameterBufferType);
    ASSERT(obj_buffer->num_elements == 1);
    ASSERT(obj_buffer->size == sizeof(VAPictureParameterBufferMPEG4));

    if ((obj_buffer->num_elements != 1) ||
        (obj_buffer->size != sizeof(VAPictureParameterBufferMPEG4))) {
        return VA_STATUS_ERROR_UNKNOWN;
    }

    /* Transfer ownership of VAPictureParameterBufferMPEG4 data */
    if (ctx->pic_params) {
        free(ctx->pic_params);
    }
    ctx->pic_params = (VAPictureParameterBufferMPEG4 *) obj_buffer->buffer_data;
    obj_buffer->buffer_data = NULL;
    obj_buffer->size = 0;


    /* Lookup surfaces for backward/forward references */
    /* Lookup surfaces for backward/forward references */
    switch (ctx->pic_params->vop_fields.bits.vop_coding_type) {
    case PICTURE_CODING_I:
        ctx->forward_ref_surface = NULL;
        ctx->backward_ref_surface = NULL;
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "PICTURE_CODING_I\nTarget surface = %08x (%08x)\n", ctx->obj_context->current_render_target->psb_surface, ctx->obj_context->current_render_target->base.id);
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Forward ref  = NULL\n");
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Backward ref = NULL\n");
        break;

    case PICTURE_CODING_P:
        ctx->forward_ref_surface = SURFACE(ctx->pic_params->forward_reference_picture);
        ctx->backward_ref_surface = NULL;

        if (ctx->pic_params->forward_reference_picture == VA_INVALID_SURFACE)
            ctx->forward_ref_surface = NULL;
        if (NULL == ctx->forward_ref_surface && ctx->pic_params->forward_reference_picture != VA_INVALID_SURFACE) {
            return VA_STATUS_ERROR_INVALID_SURFACE;
        }
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "PICTURE_CODING_P\nTarget surface = %08x (%08x)\n", ctx->obj_context->current_render_target->psb_surface, ctx->obj_context->current_render_target->base.id);
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Forward ref  = %08x (%08x)\n", (ctx->forward_ref_surface ? ctx->forward_ref_surface->psb_surface : 0), ctx->pic_params->forward_reference_picture);
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Backward ref = NULL\n");
        break;

    case PICTURE_CODING_B:
        ctx->forward_ref_surface = SURFACE(ctx->pic_params->forward_reference_picture);
        ctx->backward_ref_surface = SURFACE(ctx->pic_params->backward_reference_picture);
        if ((NULL == ctx->forward_ref_surface) ||
            (NULL == ctx->backward_ref_surface)) {
            return VA_STATUS_ERROR_INVALID_SURFACE;
        }
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "PICTURE_CODING_B\nTarget surface = %08x (%08x)\n", ctx->obj_context->current_render_target->psb_surface, ctx->obj_context->current_render_target->base.id);
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Forward ref  = %08x (%08x)\n", ctx->forward_ref_surface->psb_surface, ctx->pic_params->forward_reference_picture);
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Backward ref = %08x (%08x)\n", ctx->backward_ref_surface->psb_surface, ctx->pic_params->backward_reference_picture);
        break;

    case PICTURE_CODING_S:
        ctx->forward_ref_surface = SURFACE(ctx->pic_params->forward_reference_picture);
        ctx->backward_ref_surface = SURFACE(ctx->pic_params->backward_reference_picture);
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "PICTURE_CODING_S\nTarget surface = %08x (%08x)\n", ctx->obj_context->current_render_target->psb_surface, ctx->obj_context->current_render_target->base.id);
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Forward ref  = %08x (%08x)\n", ctx->forward_ref_surface ? ctx->forward_ref_surface->psb_surface : 0, ctx->pic_params->forward_reference_picture);
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Backward ref = %08x (%08x)\n", ctx->backward_ref_surface ? ctx->backward_ref_surface->psb_surface : 0, ctx->pic_params->backward_reference_picture);
        break;

    default:
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Unhandled MPEG4 vop_coding_type '%d'\n", ctx->pic_params->vop_fields.bits.vop_coding_type);
        return VA_STATUS_ERROR_UNKNOWN;
    }

    if (NULL == ctx->forward_ref_surface) {
        /* for mmu fault protection */
        ctx->forward_ref_surface = ctx->obj_context->current_render_target;
    }
    if (NULL == ctx->backward_ref_surface) {
        /* for mmu fault protection */
        ctx->backward_ref_surface = ctx->obj_context->current_render_target;
    }

    ctx->display_picture_width = ctx->pic_params->vop_width;
    ctx->display_picture_height = ctx->pic_params->vop_height;
    ctx->picture_width_mb = PIXELS_TO_MB(ctx->display_picture_width);
    ctx->picture_height_mb = PIXELS_TO_MB(ctx->display_picture_height);
    ctx->coded_picture_width = ctx->picture_width_mb * 16;
    ctx->coded_picture_height = ctx->picture_height_mb * 16;
    ctx->size_mb = ctx->picture_width_mb * ctx->picture_height_mb;

    if (obj_surface->share_info) {
        obj_surface->share_info->coded_width = ctx->coded_picture_width;
        obj_surface->share_info->coded_height = ctx->coded_picture_height;
    }

    uint32_t mbInPic = ctx->picture_width_mb * ctx->picture_height_mb;

    mbInPic += 4;

    uint32_t colocated_size = ((mbInPic * 200) + 0xfff) & ~0xfff;

    vaStatus = vld_dec_allocate_colocated_buffer(&ctx->dec_ctx, ctx->obj_context->current_render_target, colocated_size);
    CHECK_VASTATUS();

    vaStatus = vld_dec_allocate_colocated_buffer(&ctx->dec_ctx, ctx->forward_ref_surface, colocated_size);
    CHECK_VASTATUS();

    ctx->FEControl = 0;
    REGIO_WRITE_FIELD_LITE(ctx->FEControl ,
                           MSVDX_VEC,
                           CR_VEC_ENTDEC_FE_CONTROL,
                           ENTDEC_FE_PROFILE,
                           ctx->profile);    /* MPEG4 SP / ASP profile    */

    REGIO_WRITE_FIELD_LITE(ctx->FEControl ,
                           MSVDX_VEC,
                           CR_VEC_ENTDEC_FE_CONTROL,
                           ENTDEC_FE_MODE,
                           4);                                /* Set MPEG4 mode            */

    /* FE_SPS0                                                                        */
    ctx->FE_SPS0 = 0;
    REGIO_WRITE_FIELD_LITE(ctx->FE_SPS0, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_FE_SPS0, FE_VOP_WIDTH_IN_MBS_LESS_1,    ctx->picture_width_mb - 1);
    REGIO_WRITE_FIELD_LITE(ctx->FE_SPS0, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_FE_SPS0, FE_SHORT_HEADER_FLAG,               ctx->pic_params->vol_fields.bits.short_video_header);
    REGIO_WRITE_FIELD_LITE(ctx->FE_SPS0, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_FE_SPS0, FE_PROFILE,                    ctx->profile);

    /* FE_VOP_SPS0                                                                    */
    ctx->FE_VOP_SPS0 = 0;
    REGIO_WRITE_FIELD_LITE(ctx->FE_VOP_SPS0, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_FE_VOP_SPS0, FE_VOP_HEIGHT_IN_MBS_LESS_1, ctx->picture_height_mb - 1);
    REGIO_WRITE_FIELD_LITE(ctx->FE_VOP_SPS0, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_FE_VOP_SPS0, QUANT_PRECISION,        ctx->pic_params->quant_precision);
    REGIO_WRITE_FIELD_LITE(ctx->FE_VOP_SPS0, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_FE_VOP_SPS0, FE_NO_OF_GMC_WARPING_POINTS, ((ctx->pic_params->no_of_sprite_warping_points) ? 1 : 0));
    REGIO_WRITE_FIELD_LITE(ctx->FE_VOP_SPS0, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_FE_VOP_SPS0, FE_GMC_ENABLE, (ctx->pic_params->vol_fields.bits.sprite_enable == GMC));
    REGIO_WRITE_FIELD_LITE(ctx->FE_VOP_SPS0, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_FE_VOP_SPS0, REVERSIBLE_VLC,        ctx->pic_params->vol_fields.bits.reversible_vlc);
    REGIO_WRITE_FIELD_LITE(ctx->FE_VOP_SPS0, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_FE_VOP_SPS0, FE_DATA_PARTITIONED,    ctx->pic_params->vol_fields.bits.data_partitioned);
    REGIO_WRITE_FIELD_LITE(ctx->FE_VOP_SPS0, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_FE_VOP_SPS0, FE_INTERLACED,            ctx->pic_params->vol_fields.bits.interlaced);

    if (ctx->pic_params->vol_fields.bits.short_video_header) {
        /* FE_PICSH_PPS0                                                            */
        ctx->FE_PICSH_PPS0 = 0;
        REGIO_WRITE_FIELD_LITE(ctx->FE_PICSH_PPS0, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_FE_PICSH_PPS0, NUM_MBS_IN_GOB,        ctx->pic_params->num_macroblocks_in_gob);
        REGIO_WRITE_FIELD_LITE(ctx->FE_PICSH_PPS0, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_FE_PICSH_PPS0, NUM_GOBS_IN_VOP,        ctx->pic_params->num_gobs_in_vop);
        REGIO_WRITE_FIELD_LITE(ctx->FE_PICSH_PPS0, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_FE_PICSH_PPS0, FE_PICSH_CODING_TYPE,    ctx->pic_params->vop_fields.bits.vop_coding_type);
    }

    /* FE_VOP_PPS0 */
    ctx->FE_VOP_PPS0 = 0;
    REGIO_WRITE_FIELD_LITE(ctx->FE_VOP_PPS0, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_FE_VOP_PPS0, BACKWARD_REF_VOP_CODING_TYPE,    ctx->pic_params->vop_fields.bits.backward_reference_vop_coding_type);
    REGIO_WRITE_FIELD_LITE(ctx->FE_VOP_PPS0, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_FE_VOP_PPS0, FE_FCODE_BACKWARD,                ctx->pic_params->vop_fcode_backward);
    REGIO_WRITE_FIELD_LITE(ctx->FE_VOP_PPS0, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_FE_VOP_PPS0, FE_FCODE_FORWARD,                ctx->pic_params->vop_fcode_forward);
    REGIO_WRITE_FIELD_LITE(ctx->FE_VOP_PPS0, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_FE_VOP_PPS0, INTRA_DC_VLC_THR,                ctx->pic_params->vop_fields.bits.intra_dc_vlc_thr);
    REGIO_WRITE_FIELD_LITE(ctx->FE_VOP_PPS0, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_FE_VOP_PPS0, FE_VOP_CODING_TYPE,              ctx->pic_params->vop_fields.bits.vop_coding_type);

    /* BE_SPS0                                                                        */
    /* Common for VOPs and pictures with short header                                */
    ctx->BE_SPS0 = 0;
    REGIO_WRITE_FIELD_LITE(ctx->BE_SPS0, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_BE_SPS0, BE_SHORT_HEADER_FLAG,    ctx->pic_params->vol_fields.bits.short_video_header);
    REGIO_WRITE_FIELD_LITE(ctx->BE_SPS0, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_BE_SPS0, BE_PROFILE,            ctx->profile);

    /* BE_SPS1                                                                        */
    /* Common for VOPs and pictures with short header                                */
    ctx->BE_SPS1 = 0;
    REGIO_WRITE_FIELD_LITE(ctx->BE_SPS1, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_BE_SPS1, BE_VOP_WIDTH_IN_MBS_LESS_1, ctx->picture_width_mb - 1);
    REGIO_WRITE_FIELD_LITE(ctx->BE_SPS1, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_BE_SPS1, VOP_HEIGHT_IN_MBS_LESS_1,   ctx->picture_height_mb - 1);

    if (0 == ctx->pic_params->vol_fields.bits.short_video_header) {
        /* BE_VOP_SPS0                                                                */
        ctx->BE_VOP_SPS0 = 0;
        REGIO_WRITE_FIELD_LITE(ctx->BE_VOP_SPS0, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_BE_VOP_SPS0, QUANT_TYPE,     ctx->pic_params->vol_fields.bits.quant_type);
        REGIO_WRITE_FIELD_LITE(ctx->BE_VOP_SPS0, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_BE_VOP_SPS0, OBMC_DISABLE,   ctx->pic_params->vol_fields.bits.obmc_disable);
        REGIO_WRITE_FIELD_LITE(ctx->BE_VOP_SPS0, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_BE_VOP_SPS0, QUARTER_SAMPLE, ctx->pic_params->vol_fields.bits.quarter_sample);

        /* BE_VOP_SPS1                                                                */
        ctx->BE_VOP_SPS1 = 0;
        REGIO_WRITE_FIELD_LITE(ctx->BE_VOP_SPS1, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_BE_VOP_SPS1, GMC_WARPING_ACCURACY,        ctx->pic_params->vol_fields.bits.sprite_warping_accuracy);
        REGIO_WRITE_FIELD_LITE(ctx->BE_VOP_SPS1, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_BE_VOP_SPS1, BE_NO_OF_GMC_WARPING_POINTS, ((ctx->pic_params->no_of_sprite_warping_points) ? 1 : 0));
        REGIO_WRITE_FIELD_LITE(ctx->BE_VOP_SPS1, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_BE_VOP_SPS1, BE_GMC_ENABLE, (ctx->pic_params->vol_fields.bits.sprite_enable == GMC));
        REGIO_WRITE_FIELD_LITE(ctx->BE_VOP_SPS1, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_BE_VOP_SPS1, BE_DATA_PARTITIONED,         ctx->pic_params->vol_fields.bits.data_partitioned);
        REGIO_WRITE_FIELD_LITE(ctx->BE_VOP_SPS1, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_BE_VOP_SPS1, BE_INTERLACED,               ctx->pic_params->vol_fields.bits.interlaced);

        /* BE_VOP_PPS0                                                                */
        ctx->BE_VOP_PPS0 = 0;
        REGIO_WRITE_FIELD_LITE(ctx->BE_VOP_PPS0, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_BE_VOP_PPS0, BE_FCODE_BACKWARD,                ctx->pic_params->vop_fcode_backward);
        REGIO_WRITE_FIELD_LITE(ctx->BE_VOP_PPS0, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_BE_VOP_PPS0, BE_FCODE_FORWARD,                ctx->pic_params->vop_fcode_forward);
        REGIO_WRITE_FIELD_LITE(ctx->BE_VOP_PPS0, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_BE_VOP_PPS0, ALTERNATE_VERTICAL_SCAN_FLAG,    ctx->pic_params->vop_fields.bits.alternate_vertical_scan_flag);
        REGIO_WRITE_FIELD_LITE(ctx->BE_VOP_PPS0, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_BE_VOP_PPS0, TOP_FIELD_FIRST,                ctx->pic_params->vop_fields.bits.top_field_first);
        REGIO_WRITE_FIELD_LITE(ctx->BE_VOP_PPS0,
                               MSVDX_VEC_MPEG4,
                               CR_VEC_MPEG4_BE_VOP_PPS0,
                               ROUNDING_TYPE,
                               ((PICTURE_CODING_I == ctx->pic_params->vop_fields.bits.vop_coding_type ||
                                 PICTURE_CODING_B == ctx->pic_params->vop_fields.bits.vop_coding_type) ?
                                0 : ctx->pic_params->vop_fields.bits.vop_rounding_type));
        REGIO_WRITE_FIELD_LITE(ctx->BE_VOP_PPS0, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_BE_VOP_PPS0, BE_VOP_CODING_TYPE,                ctx->pic_params->vop_fields.bits.vop_coding_type);
    } else {
        /* BE_VOP_PPS0                                                                */
        ctx->BE_VOP_PPS0 = 0;
        REGIO_WRITE_FIELD_LITE(ctx->BE_VOP_PPS0, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_BE_VOP_PPS0, BE_FCODE_FORWARD,                1);  // Always 1 in short header mode 6.3.5.2
        REGIO_WRITE_FIELD_LITE(ctx->BE_VOP_PPS0, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_BE_VOP_PPS0, ALTERNATE_VERTICAL_SCAN_FLAG,    ctx->pic_params->vop_fields.bits.alternate_vertical_scan_flag);
        REGIO_WRITE_FIELD_LITE(ctx->BE_VOP_PPS0, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_BE_VOP_PPS0, TOP_FIELD_FIRST,                ctx->pic_params->vop_fields.bits.top_field_first);
        REGIO_WRITE_FIELD_LITE(ctx->BE_VOP_PPS0, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_BE_VOP_PPS0, ROUNDING_TYPE,                     0);  // Always 0 in short header mode 6.3.5.2
        REGIO_WRITE_FIELD_LITE(ctx->BE_VOP_PPS0, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_BE_VOP_PPS0, BE_VOP_CODING_TYPE,            ctx->pic_params->vop_fields.bits.vop_coding_type);

        /* BE_PICSH_PPS0                                                            */
        ctx->BE_PICSH_PPS0 = 0;
        REGIO_WRITE_FIELD_LITE(ctx->BE_PICSH_PPS0, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_BE_PICSH_PPS0, BE_PICSH_CODING_TYPE,         ctx->pic_params->vop_fields.bits.vop_coding_type);
    }

    if(ctx->pic_params->vol_fields.bits.data_partitioned) {
        if(!ctx->data_partition_buffer0) {
            int size = 16 * ctx->size_mb;
            ctx->data_partition_buffer0 = (psb_buffer_p) calloc(1, sizeof(struct psb_buffer_s));
            CHECK_ALLOCATION(ctx->data_partition_buffer0);
            size = (size + 0xfff) & (~0xfff);
            vaStatus = psb_buffer_create(ctx->obj_context->driver_data, size, psb_bt_vpu_only, ctx->data_partition_buffer0);
            CHECK_VASTATUS();
        }
        if(!ctx->data_partition_buffer1) {
            int size = 16 * ctx->size_mb;
            ctx->data_partition_buffer1 = (psb_buffer_p) calloc(1, sizeof(struct psb_buffer_s));
            CHECK_ALLOCATION(ctx->data_partition_buffer1);
            size = (size + 0xfff) & (~0xfff);
            vaStatus = psb_buffer_create(ctx->obj_context->driver_data, size, psb_bt_vpu_only, ctx->data_partition_buffer1);
            CHECK_VASTATUS();
        }
    }

    return VA_STATUS_SUCCESS;
}

static void psb__MPEG4_convert_iq_matrix(uint32_t *dest32, unsigned char *src)
{
    int i;
    int *idx = scan0;
    uint8_t *dest8 = (uint8_t*) dest32;

    for (i = 0; i < 64; i++) {
        *dest8++ = src[*idx++];
    }
}

static VAStatus psb__MPEG4_process_iq_matrix(context_MPEG4_p ctx, object_buffer_p obj_buffer)
{
    VAIQMatrixBufferMPEG4 *iq_matrix = (VAIQMatrixBufferMPEG4 *) obj_buffer->buffer_data;
    ASSERT(obj_buffer->type == VAIQMatrixBufferType);
    ASSERT(obj_buffer->num_elements == 1);
    ASSERT(obj_buffer->size == sizeof(VAIQMatrixBufferMPEG4));

    if ((obj_buffer->num_elements != 1) ||
        (obj_buffer->size != sizeof(VAIQMatrixBufferMPEG4))) {
        return VA_STATUS_ERROR_UNKNOWN;
    }

    if (iq_matrix->load_non_intra_quant_mat) {
        psb__MPEG4_convert_iq_matrix(ctx->qmatrix_data[NONINTRA_LUMA_Q], iq_matrix->non_intra_quant_mat);
    }
    if (iq_matrix->load_intra_quant_mat) {
        psb__MPEG4_convert_iq_matrix(ctx->qmatrix_data[INTRA_LUMA_Q], iq_matrix->intra_quant_mat);
    }
    ctx->load_non_intra_quant_mat = iq_matrix->load_non_intra_quant_mat;
    ctx->load_intra_quant_mat = iq_matrix->load_intra_quant_mat;

    return VA_STATUS_SUCCESS;
}

static void psb__MPEG4_write_qmatrices(context_MPEG4_p ctx)
{
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    int i;

    // TODO: Verify that this is indeed the same as MPEG2

    /* Since we only decode 4:2:0 We only need to the Intra tables.
    Chroma quant tables are only used in Mpeg 4:2:2 and 4:4:4.
    The hardware wants non-intra followed by intra */
    /* psb_cmdbuf_rendec_start_block( cmdbuf ); */
    psb_cmdbuf_rendec_start(cmdbuf, REG_MSVDX_VEC_IQRAM_OFFSET);

    /* todo : optimisation here is to only load the need table */
    if (ctx->load_non_intra_quant_mat) {
        /*  NONINTRA_LUMA_Q --> REG_MSVDX_VEC_IQRAM_OFFSET + 0 */
        for (i = 0; i < 16; i++) {
            psb_cmdbuf_rendec_write(cmdbuf, ctx->qmatrix_data[NONINTRA_LUMA_Q][i]);
        }
    } else {
        for (i = 0; i < 16; i++) {
            psb_cmdbuf_rendec_write(cmdbuf, 0);
        }
    }
    if (ctx->load_intra_quant_mat) {
        /*  INTRA_LUMA_Q --> REG_MSVDX_VEC_IQRAM_OFFSET + (16*4) */
        for (i = 0; i < 16; i++) {
            psb_cmdbuf_rendec_write(cmdbuf, ctx->qmatrix_data[INTRA_LUMA_Q][i]);
        }
    } else {
        for (i = 0; i < 16; i++) {
            psb_cmdbuf_rendec_write(cmdbuf, 0);
        }
    }

    psb_cmdbuf_rendec_end(cmdbuf);
    /* psb_cmdbuf_rendec_end_block( cmdbuf ); */
}

/* Precalculated values */
#define ADDR0        (0x00005800)
#define ADDR1        (0x0001f828)
#define ADDR2        (0x0002b854)
#define ADDR3        (0x0002f85c)
#define ADDR4        (0x0004d089)
#define ADDR5        (0x0008f0aa)
#define ADDR6        (0x00149988)
#define ADDR7        (0x001d8b9e)
#define ADDR8        (0x000003c3)
#define WIDTH0        (0x09a596ed)
#define WIDTH1        (0x0006d6db)
#define OPCODE0        (0x50009a0a)
#define OPCODE1        (0x00000001)

static void psb__MPEG4_write_VLC_tables(context_MPEG4_p ctx)
{
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;

    psb_cmdbuf_skip_start_block(cmdbuf, SKIP_ON_CONTEXT_SWITCH);
    /* VLC Table */
    /* Write a LLDMA Cmd to transfer VLD Table data */
    psb_cmdbuf_dma_write_cmdbuf(cmdbuf, &ctx->vlc_packed_table, 0,
                                  sizeof(gaui16mpeg4VlcTableDataPacked), 0,
                                  DMA_TYPE_VLC_TABLE);

    /* Write the vec registers with the index data for each of the tables and then write    */
    /* the actual table data.                                                                */
    psb_cmdbuf_reg_start_block(cmdbuf, 0);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR0),            ADDR0);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR1),            ADDR1);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR2),            ADDR2);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR3),            ADDR3);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR4),            ADDR4);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR5),            ADDR5);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR6),            ADDR6);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR7),            ADDR7);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR8),            ADDR8);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_WIDTH0),    WIDTH0);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_WIDTH1),    WIDTH1);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_OPCODE0),    OPCODE0);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_OPCODE1),    OPCODE1);
    psb_cmdbuf_reg_end_block(cmdbuf);

    psb_cmdbuf_skip_end_block(cmdbuf);
}

static void psb__MPEG4_set_picture_params(context_MPEG4_p ctx, VASliceParameterBufferMPEG4 __maybe_unused * slice_param)
{
    uint32_t cmd;
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    psb_surface_p target_surface = ctx->obj_context->current_render_target->psb_surface;

    psb_buffer_p colocated_target_buffer = vld_dec_lookup_colocated_buffer(&ctx->dec_ctx, target_surface);
    psb_buffer_p colocated_ref_buffer = vld_dec_lookup_colocated_buffer(&ctx->dec_ctx, ctx->forward_ref_surface->psb_surface); /* FIXME DE2.0 use backward ref surface */
    ASSERT(colocated_target_buffer);
    ASSERT(colocated_ref_buffer);

    /* psb_cmdbuf_rendec_start_block( cmdbuf ); */

    /* BE_PARAM_BASE_ADDR                                                            */
    psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_VEC, MPEG4_CR_VEC_MPEG4_BE_PARAM_BASE_ADDR));
    if (colocated_target_buffer) {
        psb_cmdbuf_rendec_write_address(cmdbuf, colocated_target_buffer, 0);
    } else {
        /* This is an error */
        psb_cmdbuf_rendec_write(cmdbuf, 0);
    }
    psb_cmdbuf_rendec_end(cmdbuf);

    /* PARAM_BASE_ADDRESS                                                            */
    psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_VEC, MPEG4_CR_VEC_MPEG4_BE_COLPARAM_BASE_ADDR));
    if (colocated_ref_buffer) {
        psb_cmdbuf_rendec_write_address(cmdbuf, colocated_ref_buffer, 0);
    } else {
        /* This is an error */
        psb_cmdbuf_rendec_write(cmdbuf, 0);
    }
    psb_cmdbuf_rendec_end(cmdbuf);

    vld_dec_setup_alternative_frame(ctx->obj_context);

    /* Send VDMC and VDEB commands                                                    */
    psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, DISPLAY_PICTURE_SIZE));

    /* Display picture size cmd                                                        */
    cmd = 0;
    REGIO_WRITE_FIELD_LITE(cmd, MSVDX_CMDS, DISPLAY_PICTURE_SIZE, DISPLAY_PICTURE_HEIGHT, ctx->coded_picture_height - 1);
    REGIO_WRITE_FIELD_LITE(cmd, MSVDX_CMDS, DISPLAY_PICTURE_SIZE, DISPLAY_PICTURE_WIDTH,  ctx->coded_picture_width - 1);
    psb_cmdbuf_rendec_write(cmdbuf, cmd);

    /* Coded picture size cmd                                                        */
    cmd = 0;
    REGIO_WRITE_FIELD_LITE(cmd, MSVDX_CMDS, CODED_PICTURE_SIZE, CODED_PICTURE_HEIGHT, ctx->coded_picture_height - 1);
    REGIO_WRITE_FIELD_LITE(cmd, MSVDX_CMDS, CODED_PICTURE_SIZE, CODED_PICTURE_WIDTH,  ctx->coded_picture_width - 1);
    psb_cmdbuf_rendec_write(cmdbuf, cmd);

    /* Operating mode cmd                                                            */
    cmd = 0;
    REGIO_WRITE_FIELD_LITE(cmd, MSVDX_CMDS, OPERATING_MODE, CHROMA_INTERLEAVED, 0);                                 /* 0 = CbCr, 1 = CrCb        */
    REGIO_WRITE_FIELD_LITE(cmd, MSVDX_CMDS, OPERATING_MODE, ROW_STRIDE,         ctx->obj_context->current_render_target->psb_surface->stride_mode);
    REGIO_WRITE_FIELD_LITE(cmd, MSVDX_CMDS, OPERATING_MODE, CODEC_PROFILE,      ctx->profile); /* MPEG4 SP / ASP profile    */
    REGIO_WRITE_FIELD_LITE(cmd, MSVDX_CMDS, OPERATING_MODE, CODEC_MODE,         4);                                 /* MPEG4                    */
    REGIO_WRITE_FIELD_LITE(cmd, MSVDX_CMDS, OPERATING_MODE, ASYNC_MODE,         1);                                 /* VDMC only                */
    REGIO_WRITE_FIELD_LITE(cmd, MSVDX_CMDS, OPERATING_MODE, CHROMA_FORMAT,      1);
    REGIO_WRITE_FIELD_LITE(cmd, MSVDX_CMDS, OPERATING_MODE, INTERLACED,         ctx->pic_params->vol_fields.bits.interlaced);
    psb_cmdbuf_rendec_write(cmdbuf, cmd);
    ctx->obj_context->operating_mode = cmd;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "    vop_coding_type = %s\n", psb__debug_picture_coding_str(ctx->pic_params->vop_fields.bits.vop_coding_type));
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "    backward ref vop_coding_type = %s\n", psb__debug_picture_coding_str(ctx->pic_params->vop_fields.bits.backward_reference_vop_coding_type));

    /* LUMA_RECONSTRUCTED_PICTURE_BASE_ADDRESSES                                    */
    psb_cmdbuf_rendec_write_address(cmdbuf, &target_surface->buf, target_surface->buf.buffer_ofs);
    /* CHROMA_RECONSTRUCTED_PICTURE_BASE_ADDRESSES                                    */
    psb_cmdbuf_rendec_write_address(cmdbuf, &target_surface->buf, target_surface->buf.buffer_ofs + target_surface->chroma_offset);

    psb_cmdbuf_rendec_end(cmdbuf);

    /* Reference pictures base addresses                                            */
    psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, REFERENCE_PICTURE_BASE_ADDRESSES));

//drv_debug_msg(VIDEO_DEBUG_GENERAL, "Target surface = %08x\n", target_surface);
//drv_debug_msg(VIDEO_DEBUG_GENERAL, "Forward ref = %08x\n", ctx->forward_ref_surface->psb_surface);
//drv_debug_msg(VIDEO_DEBUG_GENERAL, "Backward ref = %08x\n", ctx->backward_ref_surface->psb_surface);

    /* forward reference picture */
    /* LUMA_RECONSTRUCTED_PICTURE_BASE_ADDRESSES                                    */
    psb_cmdbuf_rendec_write_address(cmdbuf, &ctx->forward_ref_surface->psb_surface->buf, ctx->forward_ref_surface->psb_surface->buf.buffer_ofs);
    /* CHROMA_RECONSTRUCTED_PICTURE_BASE_ADDRESSES                                    */
    psb_cmdbuf_rendec_write_address(cmdbuf, &ctx->forward_ref_surface->psb_surface->buf, ctx->forward_ref_surface->psb_surface->buf.buffer_ofs + ctx->forward_ref_surface->psb_surface->chroma_offset);

    /* backward reference picture */
    /* LUMA_RECONSTRUCTED_PICTURE_BASE_ADDRESSES                                    */
    psb_cmdbuf_rendec_write_address(cmdbuf, &ctx->backward_ref_surface->psb_surface->buf, ctx->backward_ref_surface->psb_surface->buf.buffer_ofs);
    /* CHROMA_RECONSTRUCTED_PICTURE_BASE_ADDRESSES                                    */
    psb_cmdbuf_rendec_write_address(cmdbuf, &ctx->backward_ref_surface->psb_surface->buf, ctx->backward_ref_surface->psb_surface->buf.buffer_ofs + ctx->backward_ref_surface->psb_surface->chroma_offset);

    psb_cmdbuf_rendec_end(cmdbuf);
    /* psb_cmdbuf_rendec_end_block( cmdbuf ); */
}

static void psb__MPEG4_set_backend_registers(context_MPEG4_p ctx, VASliceParameterBufferMPEG4 *slice_param)
{
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    uint32_t cmd;
    unsigned short width_mb = PIXELS_TO_MB(ctx->pic_params->vop_width);

    /* psb_cmdbuf_rendec_start_block( cmdbuf ); */

    /* Write Back-End EntDec registers                                                */
    psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_VEC, MPEG4_CR_VEC_MPEG4_BE_SPS0));
    /* BE_SPS0                                                                        */
    /* Common for VOPs and pictures with short header                                */
    psb_cmdbuf_rendec_write(cmdbuf, ctx->BE_SPS0);
    /* BE_SPS1                                                                        */
    /* Common for VOPs and pictures with short header                                */
    psb_cmdbuf_rendec_write(cmdbuf, ctx->BE_SPS1);
    psb_cmdbuf_rendec_end(cmdbuf);

    psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_VEC, MPEG4_CR_VEC_MPEG4_BE_VOP_SPS0));
    if (0 == ctx->pic_params->vol_fields.bits.short_video_header) {
        /* BE_VOP_SPS0                                                                */
        psb_cmdbuf_rendec_write(cmdbuf, ctx->BE_VOP_SPS0);
        /* BE_VOP_SPS1                                                                */
        psb_cmdbuf_rendec_write(cmdbuf, ctx->BE_VOP_SPS1);
        /* BE_VOP_PPS0                                                                */
        psb_cmdbuf_rendec_write(cmdbuf, ctx->BE_VOP_PPS0);
    } else { /* Short-header mode */
        /* BE_VOP_SPS0 */
        psb_cmdbuf_rendec_write(cmdbuf, 0);
        /* BE_VOP_SPS1 */
        psb_cmdbuf_rendec_write(cmdbuf, 0);
        /* BE_VOP_PPS0                                                                */
        psb_cmdbuf_rendec_write(cmdbuf, ctx->BE_VOP_PPS0);
        /* BE_PICSH_PPS0                                                            */
        psb_cmdbuf_rendec_write(cmdbuf, ctx->BE_PICSH_PPS0);
    }
    psb_cmdbuf_rendec_end(cmdbuf);

    if (0 == ctx->pic_params->vol_fields.bits.short_video_header) {
        if ((GMC == ctx->pic_params->vol_fields.bits.sprite_enable) &&
            (PICTURE_CODING_S == ctx->pic_params->vop_fields.bits.vop_coding_type)) {
            psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_VEC, MPEG4_CR_VEC_MPEG4_BE_GMC_X));

            /* TODO: GMC Motion Vectors */
            /* It is still needed to specify the precision of the motion vectors (should they be in        */
            /* half-sample, quarter-sample...?) and how much processing    is done on the firmware on        */
            /* the values of the warping points.                                                        */

            // TODO: Which index to use?
            int sprite_index = 0;
            while (sprite_index < 3) {
                if (ctx->pic_params->sprite_trajectory_du[sprite_index] || ctx->pic_params->sprite_trajectory_dv[sprite_index])
                    break;
                sprite_index++;
            }
            if (sprite_index >= 3)
                sprite_index = 0;

            /* BE_GMC_X                                                                */
            cmd = 0;
            REGIO_WRITE_FIELD_LITE(cmd, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_BE_GMC_X, GMC_X, ctx->pic_params->sprite_trajectory_du[sprite_index] & 0x3FFF);
            psb_cmdbuf_rendec_write(cmdbuf, cmd);

            /* BE_GMC_Y                                                                */
            cmd = 0;
            REGIO_WRITE_FIELD_LITE(cmd, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_BE_GMC_Y, GMC_Y, ctx->pic_params->sprite_trajectory_dv[sprite_index] & 0x3FFF);
            psb_cmdbuf_rendec_write(cmdbuf, cmd);

            psb_cmdbuf_rendec_end(cmdbuf);
        }
    }

    psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_VEC, MPEG4_CR_VEC_MPEG4_BE_SLICE0));

    /* BE_SLICE0                                                                    */
    cmd = 0;
    REGIO_WRITE_FIELD_LITE(cmd, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_BE_SLICE0, BE_FIRST_MB_IN_SLICE_Y, slice_param->macroblock_number / width_mb);
    REGIO_WRITE_FIELD_LITE(cmd, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_BE_SLICE0, BE_FIRST_MB_IN_SLICE_X, slice_param->macroblock_number % width_mb);
    psb_cmdbuf_rendec_write(cmdbuf, cmd);

    /* CR_VEC_MPEG4_BE_VOP_TR                                                        */
    cmd = 0;
    REGIO_WRITE_FIELD_LITE(cmd, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_BE_VOP_TRB, BE_TRB, ctx->pic_params->TRB);
    psb_cmdbuf_rendec_write(cmdbuf, cmd);

    cmd = 0;
    REGIO_WRITE_FIELD_LITE(cmd, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_BE_VOP_TRD, BE_TRD, ctx->pic_params->TRD);
    psb_cmdbuf_rendec_write(cmdbuf, cmd);

    psb_cmdbuf_rendec_end(cmdbuf);


    /* Send Slice Data for every slice */
    /* MUST be the last slice sent */
    psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, SLICE_PARAMS));

    /* Slice params command                                                            */
    cmd = 0;
    REGIO_WRITE_FIELD_LITE(cmd,
                           MSVDX_CMDS,
                           SLICE_PARAMS,
                           RND_CTL_BIT,
                           ((PICTURE_CODING_I == ctx->pic_params->vop_fields.bits.vop_coding_type ||
                             PICTURE_CODING_B == ctx->pic_params->vop_fields.bits.vop_coding_type) ?
                            0 : ctx->pic_params->vop_fields.bits.vop_rounding_type));
    REGIO_WRITE_FIELD_LITE(cmd, MSVDX_CMDS, SLICE_PARAMS, MODE_CONFIG,            ctx->pic_params->vol_fields.bits.sprite_warping_accuracy);
    REGIO_WRITE_FIELD_LITE(cmd, MSVDX_CMDS, SLICE_PARAMS, SUBPEL_FILTER_MODE,    ctx->pic_params->vol_fields.bits.quarter_sample);
    /* SP and ASP profiles don't support field coding in different VOPs */
    REGIO_WRITE_FIELD_LITE(cmd, MSVDX_CMDS, SLICE_PARAMS, SLICE_FIELD_TYPE,   2);
    REGIO_WRITE_FIELD_LITE(cmd, MSVDX_CMDS, SLICE_PARAMS, SLICE_CODE_TYPE,    ctx->pic_params->vop_fields.bits.vop_coding_type);
    psb_cmdbuf_rendec_write(cmdbuf, cmd);

    psb_cmdbuf_rendec_end(cmdbuf);

    *ctx->dec_ctx.p_slice_params = cmd;

    /* CHUNK: Entdec back-end profile and level                                        */
    psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_VEC, CR_VEC_ENTDEC_BE_CONTROL));

    cmd = 0;
    REGIO_WRITE_FIELD_LITE(cmd, MSVDX_VEC, CR_VEC_ENTDEC_BE_CONTROL, ENTDEC_BE_PROFILE, ctx->profile);     /* MPEG4 SP / ASP profile*/
    REGIO_WRITE_FIELD_LITE(cmd, MSVDX_VEC, CR_VEC_ENTDEC_BE_CONTROL, ENTDEC_BE_MODE,    4);             /* 4 - MPEG4             */
    psb_cmdbuf_rendec_write(cmdbuf, cmd);

    psb_cmdbuf_rendec_end(cmdbuf);
    /* psb_cmdbuf_rendec_end_block( cmdbuf ); */
    if (ctx->pic_params->vol_fields.bits.data_partitioned)
    {
        /*set buffer pointer to store the parsed data-partition data */
        psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_VEC_MPEG4, CR_VEC_MPEG4_BE_DATAPARTITION0_BASE_ADDR));
        psb_cmdbuf_rendec_write_address(cmdbuf, ctx->data_partition_buffer0, ctx->data_partition_buffer0->buffer_ofs);
        psb_cmdbuf_rendec_end(cmdbuf);

        psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_VEC_MPEG4, CR_VEC_MPEG4_BE_DATAPARTITION1_BASE_ADDR));
        psb_cmdbuf_rendec_write_address(cmdbuf, ctx->data_partition_buffer1, ctx->data_partition_buffer1->buffer_ofs);
        psb_cmdbuf_rendec_end(cmdbuf);
   }

    /* Send IQ matrices to Rendec */
    psb__MPEG4_write_qmatrices(ctx);
}

static void psb__MPEG4_set_frontend_registers(context_MPEG4_p ctx, VASliceParameterBufferMPEG4 *slice_param)
{
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    uint32_t FE_slice0;
    unsigned short width_mb = PIXELS_TO_MB(ctx->pic_params->vop_width);

    psb_cmdbuf_reg_start_block(cmdbuf, 0);

    /* FE_SLICE0                                                                    */
    FE_slice0 = 0;
    REGIO_WRITE_FIELD_LITE(FE_slice0, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_FE_SLICE0, FE_VOP_QUANT,            slice_param->quant_scale);
    REGIO_WRITE_FIELD_LITE(FE_slice0, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_FE_SLICE0, FE_FIRST_MB_IN_SLICE_Y, slice_param->macroblock_number / width_mb);
    REGIO_WRITE_FIELD_LITE(FE_slice0, MSVDX_VEC_MPEG4, CR_VEC_MPEG4_FE_SLICE0, FE_FIRST_MB_IN_SLICE_X, slice_param->macroblock_number % width_mb);

    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC_MPEG4, CR_VEC_MPEG4_FE_SLICE0) , FE_slice0);

    /* Entdec Front-End controls*/
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC, CR_VEC_ENTDEC_FE_CONTROL) , ctx->FEControl);

    /* FE_SPS0                                                                        */
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC_MPEG4, CR_VEC_MPEG4_FE_SPS0) , ctx->FE_SPS0);

    /* FE_VOP_SPS0                                                                    */
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC_MPEG4, CR_VEC_MPEG4_FE_VOP_SPS0) , ctx->FE_VOP_SPS0);


    if (ctx->pic_params->vol_fields.bits.short_video_header) {
        /* FE_PICSH_PPS0                                                            */
        psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC_MPEG4, CR_VEC_MPEG4_FE_PICSH_PPS0) , ctx->FE_PICSH_PPS0);
    }

    /* FE_VOP_PPS0 */
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC_MPEG4, CR_VEC_MPEG4_FE_VOP_PPS0) , ctx->FE_VOP_PPS0);

    psb_cmdbuf_reg_end_block(cmdbuf);

    if (ctx->pic_params->vol_fields.bits.data_partitioned)
    {
        /*set buffer pointer to store the parsed data-partition data */
        psb_cmdbuf_reg_start_block(cmdbuf, 0);
        psb_cmdbuf_reg_set_address(cmdbuf, REGISTER_OFFSET(MSVDX_VEC_MPEG4, CR_VEC_MPEG4_FE_DATAPARTITION0_BASE_ADDR),
                                   ctx->data_partition_buffer0, ctx->data_partition_buffer0->buffer_ofs);
        psb_cmdbuf_reg_end_block(cmdbuf);

        psb_cmdbuf_reg_start_block(cmdbuf, 0);
        psb_cmdbuf_reg_set_address(cmdbuf, REGISTER_OFFSET(MSVDX_VEC_MPEG4, CR_VEC_MPEG4_FE_DATAPARTITION1_BASE_ADDR),
                                   ctx->data_partition_buffer1, ctx->data_partition_buffer1->buffer_ofs);
        psb_cmdbuf_reg_end_block(cmdbuf);
   }
}

static void psb__MPEG4_begin_slice(context_DEC_p dec_ctx, VASliceParameterBufferBase *vld_slice_param)
{
    VASliceParameterBufferMPEG4 *slice_param = (VASliceParameterBufferMPEG4 *) vld_slice_param;

    dec_ctx->bits_offset = slice_param->macroblock_offset;
    /* dec_ctx->SR_flags = 0; */
}

static void psb__MPEG4_process_slice_data(context_DEC_p dec_ctx, VASliceParameterBufferBase *vld_slice_param)
{
    VASliceParameterBufferMPEG4 *slice_param = (VASliceParameterBufferMPEG4 *) vld_slice_param;
    context_MPEG4_p ctx = (context_MPEG4_p)dec_ctx;

    psb__MPEG4_write_VLC_tables(ctx);
    psb__MPEG4_set_picture_params(ctx, slice_param);
    psb__MPEG4_set_frontend_registers(ctx, slice_param);
    psb__MPEG4_set_backend_registers(ctx, slice_param);
}

static void psb__MPEG4_end_slice(context_DEC_p dec_ctx)
{
    context_MPEG4_p ctx = (context_MPEG4_p)dec_ctx;

#ifdef PSBVIDEO_MSVDX_EC
    if (ctx->obj_context->driver_data->ec_enabled)
        ctx->obj_context->flags |= (FW_ERROR_DETECTION_AND_RECOVERY); /* FW_ERROR_DETECTION_AND_RECOVERY */
#endif

    ctx->obj_context->first_mb = 0;
    ctx->obj_context->last_mb = ((ctx->picture_height_mb - 1) << 8) | (ctx->picture_width_mb - 1);
    *(ctx->dec_ctx.slice_first_pic_last) = (ctx->obj_context->first_mb << 16) | (ctx->obj_context->last_mb);
}

#ifdef PSBVIDEO_MSVDX_EC
static void psb__MPEG4_choose_ec_frames(context_MPEG4_p ctx)
{
    if (ctx->pic_params == NULL)
        return;
    int is_inter = (ctx->pic_params->vop_fields.bits.vop_coding_type == PICTURE_CODING_P ||
		    ctx->pic_params->vop_fields.bits.vop_coding_type == PICTURE_CODING_B);

    ctx->obj_context->ec_target = NULL;

    /* choose forward ref frame as possible */
    if (is_inter && ctx->forward_ref_surface)
        ctx->obj_context->ec_target = ctx->forward_ref_surface;

    /* Otherwise we conceal from the previous I or P frame*/
    if (!ctx->obj_context->ec_target)
    {
        ctx->obj_context->ec_target = ctx->obj_context->ec_candidate;
    }

    if (ctx->pic_params->vop_fields.bits.vop_coding_type != PICTURE_CODING_B)
    {
        ctx->obj_context->ec_candidate = ctx->obj_context->current_render_target; /* in case the next frame is an I frame we will need this */
    }
    if (!ctx->obj_context->ec_target) {
        ctx->obj_context->ec_target = ctx->obj_context->current_render_target;
    }
}
#endif

static VAStatus pnw_MPEG4_BeginPicture(
    object_context_p obj_context)
{
    INIT_CONTEXT_MPEG4

    if (ctx->pic_params) {
        free(ctx->pic_params);
        ctx->pic_params = NULL;
    }
    ctx->load_non_intra_quant_mat = FALSE;
    ctx->load_intra_quant_mat = FALSE;

    return VA_STATUS_SUCCESS;
}

static VAStatus pnw_MPEG4_process_buffer(
    context_DEC_p dec_ctx,
    object_buffer_p buffer)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    context_MPEG4_p ctx = (context_MPEG4_p)dec_ctx;
    object_buffer_p obj_buffer = buffer;

    switch (obj_buffer->type) {
    case VAPictureParameterBufferType:
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "pnw_MPEG4_RenderPicture got VAPictureParameterBuffer\n");
        vaStatus = psb__MPEG4_process_picture_param(ctx, obj_buffer);
        DEBUG_FAILURE;
        break;

    case VAIQMatrixBufferType:
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "pnw_MPEG4_RenderPicture got VAIQMatrixBufferType\n");
        vaStatus = psb__MPEG4_process_iq_matrix(ctx, obj_buffer);
        DEBUG_FAILURE;
        break;

    default:
        vaStatus = VA_STATUS_ERROR_UNKNOWN;
        DEBUG_FAILURE;
    }

    return vaStatus;
}

static VAStatus pnw_MPEG4_EndPicture(
    object_context_p obj_context)
{
    INIT_CONTEXT_MPEG4
    psb_surface_p target_surface = ctx->obj_context->current_render_target->psb_surface;
    psb_driver_data_p driver_data = obj_context->driver_data;
    VAStatus vaStatus = VA_STATUS_SUCCESS;

#ifdef PSBVIDEO_MSVDX_EC
    /* Sent the HOST_BE_OPP command to detect slice error */
    if (ctx->pic_params && ctx->pic_params->vol_fields.bits.interlaced)
        driver_data->ec_enabled = 0;

    if (driver_data->ec_enabled) {
        uint32_t rotation_flags = 0;
        uint32_t ext_stride_a = 0;
        object_surface_p ec_target;

        psb__MPEG4_choose_ec_frames(ctx);
        ec_target = ctx->obj_context->ec_target;
        REGIO_WRITE_FIELD_LITE(ext_stride_a, MSVDX_CMDS, EXTENDED_ROW_STRIDE, EXT_ROW_STRIDE, target_surface->stride / 64);

    /* FIXME ec ignor rotate condition */
        if(ec_target) {
	    if (psb_context_get_next_cmdbuf(ctx->obj_context)) {
                vaStatus = VA_STATUS_ERROR_UNKNOWN;
                DEBUG_FAILURE;
                return vaStatus;
            }

            if (psb_context_submit_host_be_opp(ctx->obj_context,
                                          &target_surface->buf,
                                          &ec_target->psb_surface->buf,
                                          NULL,
                                          ctx->picture_width_mb,
                                          ctx->picture_height_mb,
                                          rotation_flags,
                                          ctx->field_type,
                                          ext_stride_a,
                                          target_surface->chroma_offset + target_surface->buf.buffer_ofs,
                                          ec_target->psb_surface->chroma_offset + ec_target->psb_surface->buf.buffer_ofs)) {
                  return VA_STATUS_ERROR_UNKNOWN;
            }
        }
    }
#endif

    if (psb_context_flush_cmdbuf(ctx->obj_context)) {
        return VA_STATUS_ERROR_UNKNOWN;
    }

    if (ctx->pic_params) {
        free(ctx->pic_params);
        ctx->pic_params = NULL;
    }

    return VA_STATUS_SUCCESS;
}

struct format_vtable_s pnw_MPEG4_vtable = {
queryConfigAttributes:
    pnw_MPEG4_QueryConfigAttributes,
validateConfig:
    pnw_MPEG4_ValidateConfig,
createContext:
    pnw_MPEG4_CreateContext,
destroyContext:
    pnw_MPEG4_DestroyContext,
beginPicture:
    pnw_MPEG4_BeginPicture,
renderPicture:
    vld_dec_RenderPicture,
endPicture:
    pnw_MPEG4_EndPicture
};
