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
 *
 */


#include "psb_MPEG2.h"
#include "psb_def.h"
#include "psb_surface.h"
#include "psb_cmdbuf.h"
#include "psb_drv_debug.h"

#include "hwdefs/reg_io2.h"
#include "hwdefs/msvdx_offsets.h"
#include "hwdefs/msvdx_cmds_io2.h"
#include "hwdefs/msvdx_vec_reg_io2.h"
#include "hwdefs/msvdx_vec_mpeg2_reg_io2.h"
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

/*
 * Frame types - format dependant!
 */
#define PICTURE_CODING_I    0x01
#define PICTURE_CODING_P    0x02
#define PICTURE_CODING_B    0x03
#define PICTURE_CODING_D    0x04


/************************************************************************************/
/*                Variable length codes in 'packed' format                            */
/************************************************************************************/

/* Format is: opcode, width, symbol. All VLC tables are concatenated. Index            */
/* infomation is stored in gui16mpeg2VlcIndexData[]                                    */
#define VLC_PACK(a,b,c)         ( ( (a) << 12 ) | ( (b) << 9  ) | (c) )
static const IMG_UINT16 gaui16mpeg2VlcTableDataPacked[] = {
    VLC_PACK(6 , 0 , 0) ,
    VLC_PACK(0 , 0 , 6) ,
    VLC_PACK(4 , 2 , 4) ,
    VLC_PACK(4 , 3 , 5) ,
    VLC_PACK(4 , 4 , 6) ,
    VLC_PACK(4 , 5 , 7) ,
    VLC_PACK(1 , 1 , 3) ,
    VLC_PACK(4 , 0 , 0) ,
    VLC_PACK(4 , 0 , 3) ,
    VLC_PACK(4 , 0 , 8) ,
    VLC_PACK(4 , 1 , 9) ,
    VLC_PACK(5 , 0 , 5) ,
    VLC_PACK(5 , 0 , 0) ,
    VLC_PACK(4 , 1 , 2) ,
    VLC_PACK(4 , 2 , 3) ,
    VLC_PACK(4 , 3 , 4) ,
    VLC_PACK(4 , 4 , 5) ,
    VLC_PACK(4 , 5 , 6) ,
    VLC_PACK(1 , 2 , 1) ,
    VLC_PACK(4 , 0 , 7) ,
    VLC_PACK(4 , 1 , 8) ,
    VLC_PACK(4 , 2 , 9) ,
    VLC_PACK(5 , 0 , 5) ,
    VLC_PACK(0 , 2 , 16) ,
    VLC_PACK(0 , 1 , 87) ,
    VLC_PACK(2 , 3 , 90) ,
    VLC_PACK(0 , 0 , 98) ,
    VLC_PACK(5 , 0 , 253) ,
    VLC_PACK(5 , 0 , 366) ,
    VLC_PACK(4 , 3 , 380) ,
    VLC_PACK(4 , 3 , 381) ,
    VLC_PACK(4 , 1 , 508) ,
    VLC_PACK(4 , 1 , 508) ,
    VLC_PACK(4 , 1 , 508) ,
    VLC_PACK(4 , 1 , 508) ,
    VLC_PACK(4 , 1 , 509) ,
    VLC_PACK(4 , 1 , 509) ,
    VLC_PACK(4 , 1 , 509) ,
    VLC_PACK(4 , 1 , 509) ,
    VLC_PACK(0 , 4 , 8) ,
    VLC_PACK(0 , 2 , 63) ,
    VLC_PACK(4 , 1 , 255) ,
    VLC_PACK(4 , 1 , 255) ,
    VLC_PACK(5 , 0 , 365) ,
    VLC_PACK(5 , 0 , 470) ,
    VLC_PACK(5 , 0 , 251) ,
    VLC_PACK(5 , 0 , 471) ,
    VLC_PACK(3 , 4 , 0) ,
    VLC_PACK(0 , 1 , 31) ,
    VLC_PACK(0 , 0 , 40) ,
    VLC_PACK(2 , 2 , 41) ,
    VLC_PACK(5 , 2 , 224) ,
    VLC_PACK(5 , 2 , 228) ,
    VLC_PACK(5 , 2 , 232) ,
    VLC_PACK(5 , 2 , 236) ,
    VLC_PACK(5 , 1 , 437) ,
    VLC_PACK(0 , 0 , 39) ,
    VLC_PACK(0 , 0 , 40) ,
    VLC_PACK(0 , 0 , 41) ,
    VLC_PACK(5 , 1 , 241) ,
    VLC_PACK(0 , 0 , 41) ,
    VLC_PACK(5 , 1 , 454) ,
    VLC_PACK(5 , 1 , 456) ,
    VLC_PACK(5 , 0 , 244) ,
    VLC_PACK(5 , 0 , 439) ,
    VLC_PACK(5 , 0 , 348) ,
    VLC_PACK(5 , 0 , 245) ,
    VLC_PACK(5 , 0 , 363) ,
    VLC_PACK(5 , 0 , 325) ,
    VLC_PACK(5 , 0 , 458) ,
    VLC_PACK(5 , 0 , 459) ,
    VLC_PACK(5 , 0 , 246) ,
    VLC_PACK(5 , 0 , 460) ,
    VLC_PACK(5 , 0 , 461) ,
    VLC_PACK(5 , 0 , 186) ,
    VLC_PACK(5 , 0 , 356) ,
    VLC_PACK(5 , 0 , 247) ,
    VLC_PACK(5 , 0 , 333) ,
    VLC_PACK(5 , 0 , 462) ,
    VLC_PACK(5 , 2 , 173) ,
    VLC_PACK(2 , 1 , 3) ,
    VLC_PACK(1 , 1 , 5) ,
    VLC_PACK(5 , 2 , 449) ,
    VLC_PACK(5 , 1 , 432) ,
    VLC_PACK(5 , 0 , 431) ,
    VLC_PACK(5 , 0 , 332) ,
    VLC_PACK(5 , 1 , 434) ,
    VLC_PACK(5 , 0 , 436) ,
    VLC_PACK(5 , 0 , 448) ,
    VLC_PACK(5 , 2 , 215) ,
    VLC_PACK(5 , 2 , 219) ,
    VLC_PACK(5 , 2 , 180) ,
    VLC_PACK(5 , 1 , 178) ,
    VLC_PACK(5 , 0 , 177) ,
    VLC_PACK(5 , 0 , 223) ,
    VLC_PACK(5 , 0 , 340) ,
    VLC_PACK(5 , 0 , 355) ,
    VLC_PACK(5 , 0 , 362) ,
    VLC_PACK(5 , 0 , 184) ,
    VLC_PACK(5 , 0 , 185) ,
    VLC_PACK(5 , 0 , 240) ,
    VLC_PACK(5 , 0 , 243) ,
    VLC_PACK(5 , 0 , 453) ,
    VLC_PACK(5 , 0 , 463) ,
    VLC_PACK(5 , 0 , 341) ,
    VLC_PACK(5 , 0 , 248) ,
    VLC_PACK(5 , 0 , 364) ,
    VLC_PACK(5 , 0 , 187) ,
    VLC_PACK(5 , 0 , 464) ,
    VLC_PACK(5 , 0 , 465) ,
    VLC_PACK(5 , 0 , 349) ,
    VLC_PACK(5 , 0 , 326) ,
    VLC_PACK(5 , 0 , 334) ,
    VLC_PACK(5 , 0 , 189) ,
    VLC_PACK(5 , 0 , 342) ,
    VLC_PACK(5 , 0 , 252) ,
    VLC_PACK(0 , 1 , 4) ,
    VLC_PACK(5 , 1 , 467) ,
    VLC_PACK(5 , 0 , 249) ,
    VLC_PACK(5 , 0 , 466) ,
    VLC_PACK(5 , 0 , 357) ,
    VLC_PACK(5 , 0 , 188) ,
    VLC_PACK(5 , 0 , 250) ,
    VLC_PACK(5 , 0 , 469) ,
    VLC_PACK(5 , 0 , 350) ,
    VLC_PACK(5 , 0 , 358) ,
    VLC_PACK(0 , 2 , 16) ,
    VLC_PACK(0 , 1 , 87) ,
    VLC_PACK(2 , 3 , 90) ,
    VLC_PACK(0 , 0 , 98) ,
    VLC_PACK(5 , 0 , 253) ,
    VLC_PACK(5 , 0 , 366) ,
    VLC_PACK(4 , 3 , 380) ,
    VLC_PACK(4 , 3 , 381) ,
    VLC_PACK(4 , 1 , 254) ,
    VLC_PACK(4 , 1 , 254) ,
    VLC_PACK(4 , 1 , 254) ,
    VLC_PACK(4 , 1 , 254) ,
    VLC_PACK(4 , 2 , 508) ,
    VLC_PACK(4 , 2 , 508) ,
    VLC_PACK(4 , 2 , 509) ,
    VLC_PACK(4 , 2 , 509) ,
    VLC_PACK(0 , 4 , 8) ,
    VLC_PACK(0 , 2 , 63) ,
    VLC_PACK(4 , 1 , 255) ,
    VLC_PACK(4 , 1 , 255) ,
    VLC_PACK(5 , 0 , 365) ,
    VLC_PACK(5 , 0 , 470) ,
    VLC_PACK(5 , 0 , 251) ,
    VLC_PACK(5 , 0 , 471) ,
    VLC_PACK(3 , 4 , 0) ,
    VLC_PACK(0 , 1 , 31) ,
    VLC_PACK(0 , 0 , 40) ,
    VLC_PACK(2 , 2 , 41) ,
    VLC_PACK(5 , 2 , 224) ,
    VLC_PACK(5 , 2 , 228) ,
    VLC_PACK(5 , 2 , 232) ,
    VLC_PACK(5 , 2 , 236) ,
    VLC_PACK(5 , 1 , 437) ,
    VLC_PACK(0 , 0 , 39) ,
    VLC_PACK(0 , 0 , 40) ,
    VLC_PACK(0 , 0 , 41) ,
    VLC_PACK(5 , 1 , 241) ,
    VLC_PACK(0 , 0 , 41) ,
    VLC_PACK(5 , 1 , 454) ,
    VLC_PACK(5 , 1 , 456) ,
    VLC_PACK(5 , 0 , 244) ,
    VLC_PACK(5 , 0 , 439) ,
    VLC_PACK(5 , 0 , 348) ,
    VLC_PACK(5 , 0 , 245) ,
    VLC_PACK(5 , 0 , 363) ,
    VLC_PACK(5 , 0 , 325) ,
    VLC_PACK(5 , 0 , 458) ,
    VLC_PACK(5 , 0 , 459) ,
    VLC_PACK(5 , 0 , 246) ,
    VLC_PACK(5 , 0 , 460) ,
    VLC_PACK(5 , 0 , 461) ,
    VLC_PACK(5 , 0 , 186) ,
    VLC_PACK(5 , 0 , 356) ,
    VLC_PACK(5 , 0 , 247) ,
    VLC_PACK(5 , 0 , 333) ,
    VLC_PACK(5 , 0 , 462) ,
    VLC_PACK(5 , 2 , 173) ,
    VLC_PACK(2 , 1 , 3) ,
    VLC_PACK(1 , 1 , 5) ,
    VLC_PACK(5 , 2 , 449) ,
    VLC_PACK(5 , 1 , 432) ,
    VLC_PACK(5 , 0 , 431) ,
    VLC_PACK(5 , 0 , 332) ,
    VLC_PACK(5 , 1 , 434) ,
    VLC_PACK(5 , 0 , 436) ,
    VLC_PACK(5 , 0 , 448) ,
    VLC_PACK(5 , 2 , 215) ,
    VLC_PACK(5 , 2 , 219) ,
    VLC_PACK(5 , 2 , 180) ,
    VLC_PACK(5 , 1 , 178) ,
    VLC_PACK(5 , 0 , 177) ,
    VLC_PACK(5 , 0 , 223) ,
    VLC_PACK(5 , 0 , 340) ,
    VLC_PACK(5 , 0 , 355) ,
    VLC_PACK(5 , 0 , 362) ,
    VLC_PACK(5 , 0 , 184) ,
    VLC_PACK(5 , 0 , 185) ,
    VLC_PACK(5 , 0 , 240) ,
    VLC_PACK(5 , 0 , 243) ,
    VLC_PACK(5 , 0 , 453) ,
    VLC_PACK(5 , 0 , 463) ,
    VLC_PACK(5 , 0 , 341) ,
    VLC_PACK(5 , 0 , 248) ,
    VLC_PACK(5 , 0 , 364) ,
    VLC_PACK(5 , 0 , 187) ,
    VLC_PACK(5 , 0 , 464) ,
    VLC_PACK(5 , 0 , 465) ,
    VLC_PACK(5 , 0 , 349) ,
    VLC_PACK(5 , 0 , 326) ,
    VLC_PACK(5 , 0 , 334) ,
    VLC_PACK(5 , 0 , 189) ,
    VLC_PACK(5 , 0 , 342) ,
    VLC_PACK(5 , 0 , 252) ,
    VLC_PACK(0 , 1 , 4) ,
    VLC_PACK(5 , 1 , 467) ,
    VLC_PACK(5 , 0 , 249) ,
    VLC_PACK(5 , 0 , 466) ,
    VLC_PACK(5 , 0 , 357) ,
    VLC_PACK(5 , 0 , 188) ,
    VLC_PACK(5 , 0 , 250) ,
    VLC_PACK(5 , 0 , 469) ,
    VLC_PACK(5 , 0 , 350) ,
    VLC_PACK(5 , 0 , 358) ,
    VLC_PACK(2 , 2 , 32) ,
    VLC_PACK(0 , 1 , 87) ,
    VLC_PACK(5 , 1 , 248) ,
    VLC_PACK(0 , 0 , 89) ,
    VLC_PACK(1 , 2 , 90) ,
    VLC_PACK(5 , 0 , 366) ,
    VLC_PACK(5 , 0 , 189) ,
    VLC_PACK(5 , 0 , 358) ,
    VLC_PACK(4 , 3 , 380) ,
    VLC_PACK(4 , 3 , 380) ,
    VLC_PACK(4 , 3 , 381) ,
    VLC_PACK(4 , 3 , 381) ,
    VLC_PACK(4 , 3 , 254) ,
    VLC_PACK(4 , 3 , 254) ,
    VLC_PACK(4 , 4 , 504) ,
    VLC_PACK(4 , 4 , 505) ,
    VLC_PACK(4 , 2 , 508) ,
    VLC_PACK(4 , 2 , 508) ,
    VLC_PACK(4 , 2 , 508) ,
    VLC_PACK(4 , 2 , 508) ,
    VLC_PACK(4 , 2 , 509) ,
    VLC_PACK(4 , 2 , 509) ,
    VLC_PACK(4 , 2 , 509) ,
    VLC_PACK(4 , 2 , 509) ,
    VLC_PACK(4 , 3 , 506) ,
    VLC_PACK(4 , 3 , 506) ,
    VLC_PACK(4 , 3 , 507) ,
    VLC_PACK(4 , 3 , 507) ,
    VLC_PACK(5 , 0 , 251) ,
    VLC_PACK(5 , 0 , 250) ,
    VLC_PACK(0 , 1 , 71) ,
    VLC_PACK(0 , 2 , 74) ,
    VLC_PACK(4 , 0 , 255) ,
    VLC_PACK(0 , 1 , 3) ,
    VLC_PACK(0 , 2 , 8) ,
    VLC_PACK(0 , 3 , 17) ,
    VLC_PACK(5 , 0 , 341) ,
    VLC_PACK(5 , 0 , 465) ,
    VLC_PACK(0 , 0 , 2) ,
    VLC_PACK(5 , 0 , 464) ,
    VLC_PACK(5 , 0 , 363) ,
    VLC_PACK(5 , 0 , 463) ,
    VLC_PACK(5 , 1 , 438) ,
    VLC_PACK(5 , 1 , 348) ,
    VLC_PACK(5 , 1 , 324) ,
    VLC_PACK(5 , 1 , 458) ,
    VLC_PACK(5 , 1 , 459) ,
    VLC_PACK(5 , 1 , 461) ,
    VLC_PACK(5 , 1 , 356) ,
    VLC_PACK(0 , 0 , 1) ,
    VLC_PACK(5 , 0 , 333) ,
    VLC_PACK(5 , 0 , 462) ,
    VLC_PACK(3 , 3 , 0) ,
    VLC_PACK(0 , 1 , 15) ,
    VLC_PACK(0 , 0 , 24) ,
    VLC_PACK(2 , 2 , 25) ,
    VLC_PACK(5 , 2 , 224) ,
    VLC_PACK(5 , 2 , 228) ,
    VLC_PACK(5 , 2 , 232) ,
    VLC_PACK(5 , 2 , 236) ,
    VLC_PACK(5 , 1 , 437) ,
    VLC_PACK(0 , 0 , 23) ,
    VLC_PACK(0 , 0 , 24) ,
    VLC_PACK(5 , 1 , 185) ,
    VLC_PACK(3 , 3 , 0) ,
    VLC_PACK(5 , 1 , 452) ,
    VLC_PACK(5 , 1 , 454) ,
    VLC_PACK(5 , 1 , 456) ,
    VLC_PACK(5 , 2 , 173) ,
    VLC_PACK(2 , 1 , 3) ,
    VLC_PACK(1 , 1 , 5) ,
    VLC_PACK(5 , 2 , 449) ,
    VLC_PACK(5 , 1 , 432) ,
    VLC_PACK(5 , 0 , 431) ,
    VLC_PACK(5 , 0 , 332) ,
    VLC_PACK(5 , 1 , 434) ,
    VLC_PACK(5 , 0 , 436) ,
    VLC_PACK(5 , 0 , 448) ,
    VLC_PACK(5 , 2 , 215) ,
    VLC_PACK(5 , 2 , 219) ,
    VLC_PACK(5 , 2 , 180) ,
    VLC_PACK(5 , 1 , 178) ,
    VLC_PACK(5 , 0 , 177) ,
    VLC_PACK(5 , 0 , 223) ,
    VLC_PACK(5 , 0 , 340) ,
    VLC_PACK(5 , 0 , 355) ,
    VLC_PACK(5 , 0 , 362) ,
    VLC_PACK(5 , 0 , 184) ,
    VLC_PACK(5 , 0 , 326) ,
    VLC_PACK(5 , 0 , 471) ,
    VLC_PACK(5 , 0 , 334) ,
    VLC_PACK(5 , 0 , 365) ,
    VLC_PACK(5 , 0 , 350) ,
    VLC_PACK(5 , 0 , 342) ,
    VLC_PACK(2 , 1 , 4) ,
    VLC_PACK(5 , 1 , 466) ,
    VLC_PACK(5 , 0 , 357) ,
    VLC_PACK(5 , 0 , 187) ,
    VLC_PACK(5 , 1 , 244) ,
    VLC_PACK(5 , 0 , 468) ,
    VLC_PACK(5 , 0 , 186) ,
    VLC_PACK(5 , 0 , 470) ,
    VLC_PACK(5 , 0 , 188) ,
    VLC_PACK(5 , 0 , 469) ,
    VLC_PACK(5 , 0 , 247) ,
    VLC_PACK(4 , 2 , 492) ,
    VLC_PACK(4 , 2 , 493) ,
    VLC_PACK(5 , 0 , 243) ,
    VLC_PACK(5 , 0 , 242) ,
    VLC_PACK(5 , 0 , 364) ,
    VLC_PACK(5 , 0 , 349) ,
    VLC_PACK(5 , 0 , 241) ,
    VLC_PACK(5 , 0 , 240) ,
    VLC_PACK(4 , 0 , 30) ,
    VLC_PACK(5 , 0 , 14) ,
    VLC_PACK(5 , 0 , 13) ,
    VLC_PACK(5 , 0 , 12) ,
    VLC_PACK(0 , 0 , 3) ,
    VLC_PACK(2 , 2 , 4) ,
    VLC_PACK(0 , 1 , 7) ,
    VLC_PACK(5 , 1 , 9) ,
    VLC_PACK(5 , 0 , 11) ,
    VLC_PACK(5 , 0 , 8) ,
    VLC_PACK(5 , 1 , 6) ,
    VLC_PACK(5 , 0 , 5) ,
    VLC_PACK(5 , 1 , 3) ,
    VLC_PACK(3 , 1 , 0) ,
    VLC_PACK(2 , 2 , 3) ,
    VLC_PACK(3 , 1 , 0) ,
    VLC_PACK(2 , 1 , 5) ,
    VLC_PACK(3 , 2 , 0) ,
    VLC_PACK(3 , 2 , 0) ,
    VLC_PACK(3 , 2 , 0) ,
    VLC_PACK(4 , 2 , 226) ,
    VLC_PACK(5 , 1 , 1) ,
    VLC_PACK(5 , 0 , 0) ,
    VLC_PACK(5 , 0 , 31) ,
    VLC_PACK(4 , 0 , 62) ,
    VLC_PACK(5 , 0 , 30) ,
    VLC_PACK(5 , 0 , 29) ,
    VLC_PACK(5 , 0 , 28) ,
    VLC_PACK(0 , 0 , 3) ,
    VLC_PACK(2 , 2 , 4) ,
    VLC_PACK(1 , 1 , 7) ,
    VLC_PACK(5 , 1 , 25) ,
    VLC_PACK(5 , 0 , 27) ,
    VLC_PACK(5 , 0 , 24) ,
    VLC_PACK(5 , 1 , 22) ,
    VLC_PACK(5 , 0 , 21) ,
    VLC_PACK(5 , 1 , 19) ,
    VLC_PACK(3 , 1 , 0) ,
    VLC_PACK(3 , 1 , 0) ,
    VLC_PACK(5 , 2 , 15)
};

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

struct context_MPEG2_s {
    object_context_p obj_context; /* back reference */

    /* Picture parameters */
    VAPictureParameterBufferMPEG2 *pic_params;
    object_surface_p forward_ref_surface;
    object_surface_p backward_ref_surface;

    uint32_t coded_picture_width;    /* in pixels */
    uint32_t coded_picture_height;    /* in pixels */

    uint32_t picture_width_mb;        /* in macroblocks */
    uint32_t picture_height_mb;        /* in macroblocks */
    uint32_t size_mb;                /* in macroblocks */

    uint32_t coded_picture_size;    /* MSVDX format */
    uint32_t display_picture_size;    /* MSVDX format */

    uint32_t BE_PPS0;
    uint32_t BE_PPS1;
    uint32_t BE_PPS2;
    uint32_t BE_SPS0;
    uint32_t BE_SPS1;
    uint32_t FE_PPS0;
    uint32_t FE_PPS1;

    /* IQ Matrix */
    uint32_t qmatrix_data[MAX_QUANT_TABLES][16];
    int got_iq_matrix;

    /* Split buffers */
    int split_buffer_pending;

    /* List of VASliceParameterBuffers */
    object_buffer_p *slice_param_list;
    int slice_param_list_size;
    int slice_param_list_idx;

    /* VLC packed data */
    struct psb_buffer_s vlc_packed_table;

    /* Misc */
    unsigned int previous_slice_vertical_position;

    uint32_t *p_slice_params; /* pointer to ui32SliceParams in CMD_HEADER */
};

typedef struct context_MPEG2_s *context_MPEG2_p;

#define INIT_CONTEXT_MPEG2    context_MPEG2_p ctx = (context_MPEG2_p) obj_context->format_data;

#define SURFACE(id)    ((object_surface_p) object_heap_lookup( &ctx->obj_context->driver_data->surface_heap, id ))


static void psb_MPEG2_QueryConfigAttributes(
    VAProfile profile,
    VAEntrypoint entrypoint,
    VAConfigAttrib *attrib_list,
    int num_attribs)
{
    /* No MPEG2 specific attributes */
}

static VAStatus psb_MPEG2_ValidateConfig(
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

static VAStatus psb__MPEG2_check_legal_picture(object_context_p obj_context, object_config_p obj_config)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    if (NULL == obj_context) {
        vaStatus = VA_STATUS_ERROR_INVALID_CONTEXT;
        DEBUG_FAILURE;
        return vaStatus;
    }

    if (NULL == obj_config) {
        vaStatus = VA_STATUS_ERROR_INVALID_CONFIG;
        DEBUG_FAILURE;
        return vaStatus;
    }

    /* MSVDX decode capability for MPEG2:
     *     MP@HL
     *
     * Refer to Table 8-11 (Upper bounds for luminance sample rate) of ISO/IEC 13818-2: 1995(E),
     * and the "MSVDX MPEG2 decode capability" table of "Poulsbo Media Software Overview"
     */

    switch (obj_config->profile) {
    case VAProfileMPEG2Simple:
        if ((obj_context->picture_width <= 0) || (obj_context->picture_width > 352)
            || (obj_context->picture_height <= 0) || (obj_context->picture_height > 288)) {
            vaStatus = VA_STATUS_ERROR_RESOLUTION_NOT_SUPPORTED;
        }
        break;

    case VAProfileMPEG2Main:
        if ((obj_context->picture_width <= 0) || (obj_context->picture_width > 1920)
            || (obj_context->picture_height <= 0) || (obj_context->picture_height > 1088)) {
            vaStatus = VA_STATUS_ERROR_RESOLUTION_NOT_SUPPORTED;
        }
        break;

    default:
        vaStatus = VA_STATUS_ERROR_UNSUPPORTED_PROFILE;
        break;
    }

    return vaStatus;
}

static void psb_MPEG2_DestroyContext(object_context_p obj_context);

static VAStatus psb_MPEG2_CreateContext(
    object_context_p obj_context,
    object_config_p obj_config)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    context_MPEG2_p ctx;
    /* Validate flag */
    /* Validate picture dimensions */
    vaStatus = psb__MPEG2_check_legal_picture(obj_context, obj_config);
    if (VA_STATUS_SUCCESS != vaStatus) {
        DEBUG_FAILURE;
        return vaStatus;
    }

    if (obj_context->num_render_targets < 1) {
        vaStatus = VA_STATUS_ERROR_UNKNOWN;
        DEBUG_FAILURE;
        return vaStatus;
    }
    ctx = (context_MPEG2_p) calloc(1, sizeof(struct context_MPEG2_s));
    if (NULL == ctx) {
        vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
        DEBUG_FAILURE;
        return vaStatus;
    }

    obj_context->format_data = (void*) ctx;
    ctx->obj_context = obj_context;
    ctx->pic_params = NULL;
    ctx->got_iq_matrix = FALSE;
    ctx->previous_slice_vertical_position = ~1;

    ctx->split_buffer_pending = FALSE;

    ctx->slice_param_list_size = 8;
    ctx->slice_param_list = (object_buffer_p*) calloc(1, sizeof(object_buffer_p) * ctx->slice_param_list_size);
    ctx->slice_param_list_idx = 0;

    if (NULL == ctx->slice_param_list) {
        vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
        DEBUG_FAILURE;
    }

    if (vaStatus == VA_STATUS_SUCCESS) {
        vaStatus = psb_buffer_create(obj_context->driver_data,
                                     sizeof(gaui16mpeg2VlcTableDataPacked),
                                     psb_bt_cpu_vpu,
                                     &ctx->vlc_packed_table);
        DEBUG_FAILURE;
    }
    if (vaStatus == VA_STATUS_SUCCESS) {
        unsigned char *vlc_packed_data_address;
        if (0 ==  psb_buffer_map(&ctx->vlc_packed_table, &vlc_packed_data_address)) {
            memcpy(vlc_packed_data_address, gaui16mpeg2VlcTableDataPacked, sizeof(gaui16mpeg2VlcTableDataPacked));
            psb_buffer_unmap(&ctx->vlc_packed_table);
        } else {
            vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
            DEBUG_FAILURE;
        }
    }

    if (vaStatus != VA_STATUS_SUCCESS) {
        psb_MPEG2_DestroyContext(obj_context);
    }

    return vaStatus;
}

static void psb_MPEG2_DestroyContext(
    object_context_p obj_context)
{
    INIT_CONTEXT_MPEG2

    psb_buffer_destroy(&ctx->vlc_packed_table);

    if (ctx->pic_params) {
        free(ctx->pic_params);
        ctx->pic_params = NULL;
    }

    if (ctx->slice_param_list) {
        free(ctx->slice_param_list);
        ctx->slice_param_list = NULL;
    }

    free(obj_context->format_data);
    obj_context->format_data = NULL;
}

static VAStatus psb__MPEG2_process_picture_param(context_MPEG2_p ctx, object_buffer_p obj_buffer)
{
    ASSERT(obj_buffer->type == VAPictureParameterBufferType);
    ASSERT(obj_buffer->num_elements == 1);
    ASSERT(obj_buffer->size == sizeof(VAPictureParameterBufferMPEG2));

    if ((obj_buffer->num_elements != 1) ||
        (obj_buffer->size != sizeof(VAPictureParameterBufferMPEG2))) {
        return VA_STATUS_ERROR_UNKNOWN;
    }

    /* Transfer ownership of VAPictureParameterBufferMPEG2 data */
    if (ctx->pic_params) {
        free(ctx->pic_params);
    }
    ctx->pic_params = (VAPictureParameterBufferMPEG2 *) obj_buffer->buffer_data;
    obj_buffer->buffer_data = NULL;
    obj_buffer->size = 0;

    /* Lookup surfaces for backward/forward references */
    switch (ctx->pic_params->picture_coding_type) {
    case PICTURE_CODING_I:
        ctx->forward_ref_surface = NULL;
        ctx->backward_ref_surface = NULL;
        break;

    case PICTURE_CODING_P:
        ctx->forward_ref_surface = SURFACE(ctx->pic_params->forward_reference_picture);
        ctx->backward_ref_surface = NULL;
        if (NULL == ctx->forward_ref_surface) {
            return VA_STATUS_ERROR_INVALID_SURFACE;
        }
        break;

    case PICTURE_CODING_B:
        ctx->forward_ref_surface = SURFACE(ctx->pic_params->forward_reference_picture);
        ctx->backward_ref_surface = SURFACE(ctx->pic_params->backward_reference_picture);
        if ((NULL == ctx->forward_ref_surface) ||
            (NULL == ctx->backward_ref_surface)) {
            return VA_STATUS_ERROR_INVALID_SURFACE;
        }
        break;

    default:
        return VA_STATUS_ERROR_UNKNOWN;
    }

    ctx->picture_width_mb = (ctx->pic_params->horizontal_size + 15) / 16;
    if (ctx->obj_context->va_flags & VA_PROGRESSIVE) {
        ctx->picture_height_mb = (ctx->pic_params->vertical_size + 15) / 16;
    } else {
        ctx->picture_height_mb = (ctx->pic_params->vertical_size + 31) / 32;
        ctx->picture_height_mb *= 2;
    }
    ctx->coded_picture_width = ctx->picture_width_mb * 16;
    ctx->coded_picture_height = ctx->picture_height_mb * 16;

    ctx->size_mb = ctx->picture_width_mb * ctx->picture_height_mb;

    /* Display picture size                                                            */
    ctx->display_picture_size = 0;
    /*
     * coded_picture_width/height is aligned to the size of a macroblock..
     * Both coded_picture_height or vertical_size can be used for DISPLAY_SIZE and both give correct results,
     * however Vista driver / test app uses the aligned value that's in coded_picture_height so we do too.
     * See e.g. low4.m2v for an example clip where vertical_size will differ from coded_picture_height
     */
#if 0
    REGIO_WRITE_FIELD_LITE(ctx->display_picture_size, MSVDX_CMDS, DISPLAY_PICTURE_SIZE, DISPLAY_PICTURE_HEIGHT, ctx->pic_params->vertical_size - 1);
    REGIO_WRITE_FIELD_LITE(ctx->display_picture_size, MSVDX_CMDS, DISPLAY_PICTURE_SIZE, DISPLAY_PICTURE_WIDTH, ctx->pic_params->horizontal_size - 1);
#else
    REGIO_WRITE_FIELD_LITE(ctx->display_picture_size, MSVDX_CMDS, DISPLAY_PICTURE_SIZE, DISPLAY_PICTURE_HEIGHT, ctx->coded_picture_height - 1);
    REGIO_WRITE_FIELD_LITE(ctx->display_picture_size, MSVDX_CMDS, DISPLAY_PICTURE_SIZE, DISPLAY_PICTURE_WIDTH, ctx->coded_picture_width - 1);
#endif

    /* Coded picture size                                                            */
    ctx->coded_picture_size = 0;
    REGIO_WRITE_FIELD_LITE(ctx->coded_picture_size, MSVDX_CMDS, CODED_PICTURE_SIZE, CODED_PICTURE_HEIGHT, ctx->coded_picture_height - 1);
    REGIO_WRITE_FIELD_LITE(ctx->coded_picture_size, MSVDX_CMDS, CODED_PICTURE_SIZE, CODED_PICTURE_WIDTH, ctx->coded_picture_width - 1);

    ctx->BE_SPS0 = 0;
    REGIO_WRITE_FIELD_LITE(ctx->BE_SPS0, MSVDX_VEC_MPEG2, CR_VEC_MPEG2_BE_SPS0, BE_HORIZONTAL_SIZE_MINUS1, ctx->picture_width_mb - 1);

    ctx->BE_SPS1 = 0;
    REGIO_WRITE_FIELD_LITE(ctx->BE_SPS1, MSVDX_VEC_MPEG2, CR_VEC_MPEG2_BE_SPS1, BE_VERTICAL_SIZE_MINUS1, ctx->picture_height_mb - 1);

    ctx->FE_PPS0 = 0;
    REGIO_WRITE_FIELD_LITE(ctx->FE_PPS0, MSVDX_VEC_MPEG2, CR_VEC_MPEG2_FE_PPS0, FE_ALTERNATE_SCAN,                 !!(ctx->pic_params->picture_coding_extension.bits.alternate_scan));
    REGIO_WRITE_FIELD_LITE(ctx->FE_PPS0, MSVDX_VEC_MPEG2, CR_VEC_MPEG2_FE_PPS0, FE_Q_SCALE_TYPE,                 !!(ctx->pic_params->picture_coding_extension.bits.q_scale_type));
    REGIO_WRITE_FIELD_LITE(ctx->FE_PPS0, MSVDX_VEC_MPEG2, CR_VEC_MPEG2_FE_PPS0, FE_INTRA_DC_PRECISION,                ctx->pic_params->picture_coding_extension.bits.intra_dc_precision);
    REGIO_WRITE_FIELD_LITE(ctx->FE_PPS0, MSVDX_VEC_MPEG2, CR_VEC_MPEG2_FE_PPS0, FE_TOP_FIELD_FIRST,             !!(ctx->pic_params->picture_coding_extension.bits.top_field_first));
    REGIO_WRITE_FIELD_LITE(ctx->FE_PPS0, MSVDX_VEC_MPEG2, CR_VEC_MPEG2_FE_PPS0, FE_CONCEALMENT_MOTION_VECTORS,  !!(ctx->pic_params->picture_coding_extension.bits.concealment_motion_vectors));
    REGIO_WRITE_FIELD_LITE(ctx->FE_PPS0, MSVDX_VEC_MPEG2, CR_VEC_MPEG2_FE_PPS0, FE_FRAME_PRED_FRAME_DCT,         !!(ctx->pic_params->picture_coding_extension.bits.frame_pred_frame_dct));
    REGIO_WRITE_FIELD_LITE(ctx->FE_PPS0, MSVDX_VEC_MPEG2, CR_VEC_MPEG2_FE_PPS0, FE_PICTURE_STRUCTURE,                ctx->pic_params->picture_coding_extension.bits.picture_structure);
    REGIO_WRITE_FIELD_LITE(ctx->FE_PPS0, MSVDX_VEC_MPEG2, CR_VEC_MPEG2_FE_PPS0, FE_INTRA_VLC_FORMAT,             !!(ctx->pic_params->picture_coding_extension.bits.intra_vlc_format));

    ctx->FE_PPS1 = 0;
    REGIO_WRITE_FIELD_LITE(ctx->FE_PPS1, MSVDX_VEC_MPEG2, CR_VEC_MPEG2_FE_PPS1, FE_PICTURE_CODING_TYPE, ctx->pic_params->picture_coding_type);
    REGIO_WRITE_FIELD_LITE(ctx->FE_PPS1, MSVDX_VEC_MPEG2, CR_VEC_MPEG2_FE_PPS1, FE_F_CODE00, (ctx->pic_params->f_code >> 12) & 0x0F);
    REGIO_WRITE_FIELD_LITE(ctx->FE_PPS1, MSVDX_VEC_MPEG2, CR_VEC_MPEG2_FE_PPS1, FE_F_CODE01, (ctx->pic_params->f_code >>  8) & 0x0F);
    REGIO_WRITE_FIELD_LITE(ctx->FE_PPS1, MSVDX_VEC_MPEG2, CR_VEC_MPEG2_FE_PPS1, FE_F_CODE10, (ctx->pic_params->f_code >>  4) & 0x0F);
    REGIO_WRITE_FIELD_LITE(ctx->FE_PPS1, MSVDX_VEC_MPEG2, CR_VEC_MPEG2_FE_PPS1, FE_F_CODE11, (ctx->pic_params->f_code >>  0) & 0x0F);

    /* VEC Control register: Back-End MPEG2 PPS0                                    */
    ctx->BE_PPS0 = 0;
    REGIO_WRITE_FIELD_LITE(ctx->BE_PPS0, MSVDX_VEC_MPEG2, CR_VEC_MPEG2_BE_PPS0, BE_FRAME_PRED_FRAME_DCT,    !!(ctx->pic_params->picture_coding_extension.bits.frame_pred_frame_dct));
    REGIO_WRITE_FIELD_LITE(ctx->BE_PPS0, MSVDX_VEC_MPEG2, CR_VEC_MPEG2_BE_PPS0, BE_INTRA_DC_PRECISION,           ctx->pic_params->picture_coding_extension.bits.intra_dc_precision);
    REGIO_WRITE_FIELD_LITE(ctx->BE_PPS0, MSVDX_VEC_MPEG2, CR_VEC_MPEG2_BE_PPS0, BE_Q_SCALE_TYPE,            !!(ctx->pic_params->picture_coding_extension.bits.q_scale_type));
    REGIO_WRITE_FIELD_LITE(ctx->BE_PPS0, MSVDX_VEC_MPEG2, CR_VEC_MPEG2_BE_PPS0, BE_ALTERNATE_SCAN,            !!(ctx->pic_params->picture_coding_extension.bits.alternate_scan));

    ctx->BE_PPS1 = 0;
    REGIO_WRITE_FIELD_LITE(ctx->BE_PPS1, MSVDX_VEC_MPEG2, CR_VEC_MPEG2_BE_PPS1, BE_F_CODE00, (ctx->pic_params->f_code >> 12) & 0x0F);
    REGIO_WRITE_FIELD_LITE(ctx->BE_PPS1, MSVDX_VEC_MPEG2, CR_VEC_MPEG2_BE_PPS1, BE_F_CODE01, (ctx->pic_params->f_code >>  8) & 0x0F);
    REGIO_WRITE_FIELD_LITE(ctx->BE_PPS1, MSVDX_VEC_MPEG2, CR_VEC_MPEG2_BE_PPS1, BE_F_CODE10, (ctx->pic_params->f_code >>  4) & 0x0F);
    REGIO_WRITE_FIELD_LITE(ctx->BE_PPS1, MSVDX_VEC_MPEG2, CR_VEC_MPEG2_BE_PPS1, BE_F_CODE11, (ctx->pic_params->f_code >>  0) & 0x0F);

    /* VEC Control register: Back-End MPEG2 PPS2                                    */
    ctx->BE_PPS2 = 0;
    REGIO_WRITE_FIELD_LITE(ctx->BE_PPS2, MSVDX_VEC_MPEG2, CR_VEC_MPEG2_BE_PPS2, BE_PICTURE_CODING_TYPE,           ctx->pic_params->picture_coding_type);
    REGIO_WRITE_FIELD_LITE(ctx->BE_PPS2, MSVDX_VEC_MPEG2, CR_VEC_MPEG2_BE_PPS2, BE_TOP_FIELD_FIRST,            !!(ctx->pic_params->picture_coding_extension.bits.top_field_first));
    REGIO_WRITE_FIELD_LITE(ctx->BE_PPS2, MSVDX_VEC_MPEG2, CR_VEC_MPEG2_BE_PPS2, BE_CONCEALMENT_MOTION_VECTORS, !!(ctx->pic_params->picture_coding_extension.bits.concealment_motion_vectors));
    REGIO_WRITE_FIELD_LITE(ctx->BE_PPS2, MSVDX_VEC_MPEG2, CR_VEC_MPEG2_BE_PPS2, BE_PICTURE_STRUCTURE,               ctx->pic_params->picture_coding_extension.bits.picture_structure);

    ctx->obj_context->operating_mode = 0;
    REGIO_WRITE_FIELD_LITE(ctx->obj_context->operating_mode, MSVDX_CMDS, OPERATING_MODE, CHROMA_INTERLEAVED, 0);
    REGIO_WRITE_FIELD_LITE(ctx->obj_context->operating_mode, MSVDX_CMDS, OPERATING_MODE, ROW_STRIDE, ctx->obj_context->current_render_target->psb_surface->stride_mode);
    REGIO_WRITE_FIELD_LITE(ctx->obj_context->operating_mode, MSVDX_CMDS, OPERATING_MODE, CODEC_PROFILE, 1);                                 /* MPEG2 profile            */
    REGIO_WRITE_FIELD_LITE(ctx->obj_context->operating_mode, MSVDX_CMDS, OPERATING_MODE, CODEC_MODE, 3);                                 /* MPEG2 mode                */
    REGIO_WRITE_FIELD_LITE(ctx->obj_context->operating_mode, MSVDX_CMDS, OPERATING_MODE, ASYNC_MODE, 1);                                 /* VDMC only                */
    REGIO_WRITE_FIELD_LITE(ctx->obj_context->operating_mode, MSVDX_CMDS, OPERATING_MODE, CHROMA_FORMAT, 1);

    return VA_STATUS_SUCCESS;
}

static void psb__MPEG2_convert_iq_matrix(uint32_t *dest32, unsigned char *src)
{
    int i;
    int *idx = scan0;
    uint8_t *dest8 = (uint8_t*) dest32;

    for (i = 0; i < 64; i++) {
        *dest8++ = src[*idx++];
    }
}

static VAStatus psb__MPEG2_process_iq_matrix(context_MPEG2_p ctx, object_buffer_p obj_buffer)
{
    VAIQMatrixBufferMPEG2 *iq_matrix = (VAIQMatrixBufferMPEG2 *) obj_buffer->buffer_data;
    ASSERT(obj_buffer->type == VAIQMatrixBufferType);
    ASSERT(obj_buffer->num_elements == 1);
    ASSERT(obj_buffer->size == sizeof(VAIQMatrixBufferMPEG2));

    if ((obj_buffer->num_elements != 1) ||
        (obj_buffer->size != sizeof(VAIQMatrixBufferMPEG2))) {
        return VA_STATUS_ERROR_UNKNOWN;
    }

    /* Only update the qmatrix data if the load flag is set */
    if (iq_matrix->load_non_intra_quantiser_matrix) {
        psb__MPEG2_convert_iq_matrix(ctx->qmatrix_data[NONINTRA_LUMA_Q], iq_matrix->non_intra_quantiser_matrix);
    }
    if (iq_matrix->load_intra_quantiser_matrix) {
        psb__MPEG2_convert_iq_matrix(ctx->qmatrix_data[INTRA_LUMA_Q], iq_matrix->intra_quantiser_matrix);
    }
    /* We ignore the Chroma tables because those are not supported with VA_RT_FORMAT_YUV420 */
    ctx->got_iq_matrix = TRUE;

    return VA_STATUS_SUCCESS;
}

/*
 * Adds a VASliceParameterBuffer to the list of slice params
 */
static VAStatus psb__MPEG2_add_slice_param(context_MPEG2_p ctx, object_buffer_p obj_buffer)
{
    ASSERT(obj_buffer->type == VASliceParameterBufferType);
    if (ctx->slice_param_list_idx >= ctx->slice_param_list_size) {
        unsigned char *new_list;
        ctx->slice_param_list_size += 8;
        new_list = realloc(ctx->slice_param_list,
                           sizeof(object_buffer_p) * ctx->slice_param_list_size);
        if (NULL == new_list) {
            return VA_STATUS_ERROR_ALLOCATION_FAILED;
        }
        ctx->slice_param_list = (object_buffer_p*) new_list;
    }
    ctx->slice_param_list[ctx->slice_param_list_idx] = obj_buffer;
    ctx->slice_param_list_idx++;
    return VA_STATUS_SUCCESS;
}

/* Precalculated values */
#define ADDR0       (0x00006000)
#define ADDR1       (0x0003f017)
#define ADDR2       (0x000ab0e5)
#define ADDR3       (0x0000016e)
#define WIDTH0      (0x0016c6ed)
#define OPCODE0     (0x00002805)

static void psb__MPEG2_write_VLC_tables(context_MPEG2_p ctx)
{
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;

    /* Write the vec registers with the index data for each of the tables and then write    */
    /* the actual table data.                                                                */
    psb_cmdbuf_reg_start_block(cmdbuf, 0);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR0),            ADDR0);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR1),            ADDR1);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR2),            ADDR2);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR3),            ADDR3);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_WIDTH0),    WIDTH0);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_OPCODE0), OPCODE0);
    psb_cmdbuf_reg_end_block(cmdbuf);

    /* VLC Table */
    /* Write a LLDMA Cmd to transfer VLD Table data */
    psb_cmdbuf_lldma_write_cmdbuf(cmdbuf, &ctx->vlc_packed_table, 0,
                                  sizeof(gaui16mpeg2VlcTableDataPacked),
                                  0, LLDMA_TYPE_VLC_TABLE);
}

/* Programme the Alt output if there is a rotation*/
static void psb__MPEG2_setup_alternative_frame(context_MPEG2_p ctx)
{
    uint32_t cmd;
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    psb_surface_p rotate_surface = ctx->obj_context->current_render_target->out_loop_surface;
    object_context_p obj_context = ctx->obj_context;

    if (GET_SURFACE_INFO_rotate(rotate_surface) != obj_context->msvdx_rotate)
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Display rotate mode does not match surface rotate mode!\n");


    /* CRendecBlock    RendecBlk( mCtrlAlloc , RENDEC_REGISTER_OFFSET(MSVDX_CMDS, VC1_LUMA_RANGE_MAPPING_BASE_ADDRESS) ); */
    psb_cmdbuf_rendec_start_chunk(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, VC1_LUMA_RANGE_MAPPING_BASE_ADDRESS));

    psb_cmdbuf_rendec_write_address(cmdbuf, &rotate_surface->buf, rotate_surface->buf.buffer_ofs);
    psb_cmdbuf_rendec_write_address(cmdbuf, &rotate_surface->buf, rotate_surface->buf.buffer_ofs + rotate_surface->chroma_offset);

    psb_cmdbuf_rendec_end_chunk(cmdbuf);

    /* Set the rotation registers */
    psb_cmdbuf_rendec_start_chunk(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, ALTERNATIVE_OUTPUT_PICTURE_ROTATION));
    cmd = 0;
    REGIO_WRITE_FIELD_LITE(cmd, MSVDX_CMDS, ALTERNATIVE_OUTPUT_PICTURE_ROTATION , ALT_PICTURE_ENABLE, 1);
    REGIO_WRITE_FIELD_LITE(cmd, MSVDX_CMDS, ALTERNATIVE_OUTPUT_PICTURE_ROTATION , ROTATION_ROW_STRIDE, rotate_surface->stride_mode);
    REGIO_WRITE_FIELD_LITE(cmd, MSVDX_CMDS, ALTERNATIVE_OUTPUT_PICTURE_ROTATION , RECON_WRITE_DISABLE, 0); /* FIXME Always generate Rec */
    REGIO_WRITE_FIELD_LITE(cmd, MSVDX_CMDS, ALTERNATIVE_OUTPUT_PICTURE_ROTATION , ROTATION_MODE, GET_SURFACE_INFO_rotate(rotate_surface));

    psb_cmdbuf_rendec_write(cmdbuf, cmd);

    psb_cmdbuf_rendec_end_chunk(cmdbuf);
}

static void psb__MPEG2_set_operating_mode(context_MPEG2_p ctx)
{
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    psb_surface_p target_surface = ctx->obj_context->current_render_target->psb_surface;

    psb_cmdbuf_rendec_start_block(cmdbuf);

    if (CONTEXT_ROTATE(ctx->obj_context))
        psb__MPEG2_setup_alternative_frame(ctx);

    psb_cmdbuf_rendec_start_chunk(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, DISPLAY_PICTURE_SIZE));
    psb_cmdbuf_rendec_write(cmdbuf, ctx->display_picture_size);
    psb_cmdbuf_rendec_write(cmdbuf, ctx->coded_picture_size);
    psb_cmdbuf_rendec_write(cmdbuf, ctx->obj_context->operating_mode);

    /* LUMA_RECONSTRUCTED_PICTURE_BASE_ADDRESSES                                    */
    psb_cmdbuf_rendec_write_address(cmdbuf, &target_surface->buf, target_surface->buf.buffer_ofs);

    /* CHROMA_RECONSTRUCTED_PICTURE_BASE_ADDRESSES                                    */
    psb_cmdbuf_rendec_write_address(cmdbuf, &target_surface->buf, target_surface->buf.buffer_ofs + target_surface->chroma_offset);

    psb_cmdbuf_rendec_end_chunk(cmdbuf);
    psb_cmdbuf_rendec_end_block(cmdbuf);
}

static void psb__MPEG2_set_reference_pictures(context_MPEG2_p ctx)
{
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    psb_surface_p target_surface = ctx->obj_context->current_render_target->psb_surface;

    psb_cmdbuf_rendec_start_block(cmdbuf);
    psb_cmdbuf_rendec_start_chunk(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, REFERENCE_PICTURE_BASE_ADDRESSES));

    /* In MPEG2, the registers at N=0 are always used to store the base address of the luma and chroma buffers
        of the most recently decoded reference picture. The registers at N=1 are used to store the base address
        of the luma and chroma buffers of the older reference picture, if more than one reference picture is used.
        This means that when decoding a P picture the forward reference picture’s address is at index 0.
        When decoding a B-picture the backward reference picture’s address is at index 0 and the address of the
        forward reference picture – which was decoded earlier than the backward reference – is at index 1.
    */

    switch (ctx->pic_params->picture_coding_type) {
    case PICTURE_CODING_I:
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "    I-Frame\n");
        /* No reference pictures */
        psb_cmdbuf_rendec_write(cmdbuf, 0);
        psb_cmdbuf_rendec_write(cmdbuf, 0);
        psb_cmdbuf_rendec_write(cmdbuf, 0);
        psb_cmdbuf_rendec_write(cmdbuf, 0);
        break;

    case PICTURE_CODING_P:
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "    P-Frame\n");
        if (ctx->pic_params->picture_coding_extension.bits.is_first_field) {
            /* forward reference picture */
            /* LUMA_RECONSTRUCTED_PICTURE_BASE_ADDRESSES */
            psb_cmdbuf_rendec_write_address(cmdbuf, &ctx->forward_ref_surface->psb_surface->buf, ctx->forward_ref_surface->psb_surface->buf.buffer_ofs);

            /* CHROMA_RECONSTRUCTED_PICTURE_BASE_ADDRESSES */
            psb_cmdbuf_rendec_write_address(cmdbuf, &ctx->forward_ref_surface->psb_surface->buf,  ctx->forward_ref_surface->psb_surface\
                                            ->buf.buffer_ofs + ctx->forward_ref_surface->psb_surface->chroma_offset);

            /* No backward reference picture */
            psb_cmdbuf_rendec_write(cmdbuf, 0);
            psb_cmdbuf_rendec_write(cmdbuf, 0);
        } else {
            /* backward reference picture */
            /* LUMA_RECONSTRUCTED_PICTURE_BASE_ADDRESSES                                    */
            psb_cmdbuf_rendec_write_address(cmdbuf, &target_surface->buf, target_surface->buf.buffer_ofs);

            /* CHROMA_RECONSTRUCTED_PICTURE_BASE_ADDRESSES                                    */
            psb_cmdbuf_rendec_write_address(cmdbuf, &target_surface->buf, target_surface->buf.buffer_ofs + target_surface->chroma_offset);

            /* forward reference picture */
            /* LUMA_RECONSTRUCTED_PICTURE_BASE_ADDRESSES                                    */
            psb_cmdbuf_rendec_write_address(cmdbuf, &ctx->forward_ref_surface->psb_surface->buf, ctx->forward_ref_surface->psb_surface->buf.buffer_ofs);

            /* CHROMA_RECONSTRUCTED_PICTURE_BASE_ADDRESSES                                    */
            psb_cmdbuf_rendec_write_address(cmdbuf, &ctx->forward_ref_surface->psb_surface->buf, ctx->forward_ref_surface->psb_surface->buf.buffer_ofs + ctx->forward_ref_surface->psb_surface->chroma_offset);
        }
        break;

    case PICTURE_CODING_B:
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "    B-Frame\n");
        /* backward reference picture */
        /* LUMA_RECONSTRUCTED_PICTURE_BASE_ADDRESSES                                    */
        psb_cmdbuf_rendec_write_address(cmdbuf, &ctx->backward_ref_surface->psb_surface->buf, ctx->backward_ref_surface->psb_surface->buf.buffer_ofs);

        /* CHROMA_RECONSTRUCTED_PICTURE_BASE_ADDRESSES                                    */
        psb_cmdbuf_rendec_write_address(cmdbuf, &ctx->backward_ref_surface->psb_surface->buf, ctx->backward_ref_surface->psb_surface->buf.buffer_ofs + ctx->backward_ref_surface->psb_surface->chroma_offset);

        /* forward reference picture */
        /* LUMA_RECONSTRUCTED_PICTURE_BASE_ADDRESSES                                    */
        psb_cmdbuf_rendec_write_address(cmdbuf, &ctx->forward_ref_surface->psb_surface->buf, ctx->forward_ref_surface->psb_surface->buf.buffer_ofs);

        /* CHROMA_RECONSTRUCTED_PICTURE_BASE_ADDRESSES                                    */
        psb_cmdbuf_rendec_write_address(cmdbuf, &ctx->forward_ref_surface->psb_surface->buf, ctx->forward_ref_surface->psb_surface->buf.buffer_ofs + ctx->forward_ref_surface->psb_surface->chroma_offset);
        break;
    }

    psb_cmdbuf_rendec_end_chunk(cmdbuf);
    psb_cmdbuf_rendec_end_block(cmdbuf);
}

static void psb__MPEG2_set_picture_header(context_MPEG2_p ctx, VASliceParameterBufferMPEG2 *slice_param)
{
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    psb_surface_p target_surface = ctx->obj_context->current_render_target->psb_surface;
    uint32_t FE_slice;
    uint32_t BE_slice;

    psb_cmdbuf_reg_start_block(cmdbuf, 0);

    /* VEC Control register: Front-End MPEG2 PPS0 */
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC_MPEG2, CR_VEC_MPEG2_FE_PPS0) , ctx->FE_PPS0);

    /* VEC Control register: Front-End MPEG2 PPS1 */
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC_MPEG2, CR_VEC_MPEG2_FE_PPS1) , ctx->FE_PPS1);

    /* Slice level */
    FE_slice = 0;
    REGIO_WRITE_FIELD(FE_slice,
                      MSVDX_VEC_MPEG2,
                      CR_VEC_MPEG2_FE_SLICE,
                      FE_FIRST_IN_ROW,
                      (ctx->previous_slice_vertical_position != slice_param->slice_vertical_position));

    REGIO_WRITE_FIELD(FE_slice,
                      MSVDX_VEC_MPEG2,
                      CR_VEC_MPEG2_FE_SLICE,
                      FE_SLICE_VERTICAL_POSITION_MINUS1,
                      slice_param->slice_vertical_position);

    REGIO_WRITE_FIELD(FE_slice,
                      MSVDX_VEC_MPEG2,
                      CR_VEC_MPEG2_FE_SLICE,
                      FE_QUANTISER_SCALE_CODE,
                      slice_param->quantiser_scale_code);

    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC_MPEG2, CR_VEC_MPEG2_FE_SLICE) , FE_slice);

    psb_cmdbuf_reg_end_block(cmdbuf);


    /* BE Section */
    psb_cmdbuf_rendec_start_block(cmdbuf);
    psb_cmdbuf_rendec_start_chunk(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_VEC, MPEG2_CR_VEC_MPEG2_BE_SPS0));

    psb_cmdbuf_rendec_write(cmdbuf, ctx->BE_SPS0);
    psb_cmdbuf_rendec_write(cmdbuf, ctx->BE_SPS1);
    psb_cmdbuf_rendec_write(cmdbuf, ctx->BE_PPS0);   /* VEC Control register: Back-End MPEG2 PPS0 */
    psb_cmdbuf_rendec_write(cmdbuf, ctx->BE_PPS1);   /* VEC Control register: Back-End MPEG2 PPS1 */
    psb_cmdbuf_rendec_write(cmdbuf, ctx->BE_PPS2);   /* VEC Control register: Back-End MPEG2 PPS2 */

    BE_slice = 0;
    if (!ctx->pic_params->picture_coding_extension.bits.is_first_field) {
        /*
            BE_IP_PAIR_FLAG is 1 if the current picture_data is a P-coded field which is the 2nd field of a frame,
                        and which uses the first field of the frame (which was I-coded) as a reference. 0 otherwise.
            BE_IP_PAIR_FLAG will only be 1 if BE_SECOND_FIELD_FLAG is 1, and the condition
                of "this is a P-field which uses the accompanying I-field as a reference" is met.
        */
        if (ctx->pic_params->picture_coding_type == PICTURE_CODING_P) {
            if (GET_SURFACE_INFO_picture_coding_type(target_surface) == PICTURE_CODING_I)            {
                REGIO_WRITE_FIELD_LITE(BE_slice,
                                       MSVDX_VEC_MPEG2,
                                       CR_VEC_MPEG2_BE_SLICE,
                                       BE_IP_PAIR_FLAG, 1);
            }
        }

        REGIO_WRITE_FIELD_LITE(BE_slice,
                               MSVDX_VEC_MPEG2,
                               CR_VEC_MPEG2_BE_SLICE,
                               BE_SECOND_FIELD_FLAG,     1);

    } else {
        // BE_IP_PAIR_FLAG = 0;
        // BE_SECOND_FIELD_FLAG = 0;

        /*  Update with current settings first field */
        SET_SURFACE_INFO_is_defined(target_surface, TRUE);
        SET_SURFACE_INFO_picture_structure(target_surface, ctx->pic_params->picture_coding_extension.bits.picture_structure);
        SET_SURFACE_INFO_picture_coding_type(target_surface, ctx->pic_params->picture_coding_type);
    }

    REGIO_WRITE_FIELD_LITE(BE_slice,
                           MSVDX_VEC_MPEG2,
                           CR_VEC_MPEG2_BE_SLICE,
                           BE_FIRST_IN_ROW,
                           (ctx->previous_slice_vertical_position != slice_param->slice_vertical_position));

    REGIO_WRITE_FIELD_LITE(BE_slice,
                           MSVDX_VEC_MPEG2,
                           CR_VEC_MPEG2_BE_SLICE,
                           BE_SLICE_VERTICAL_POSITION_MINUS1,
                           slice_param->slice_vertical_position);

    REGIO_WRITE_FIELD_LITE(BE_slice,
                           MSVDX_VEC_MPEG2,
                           CR_VEC_MPEG2_BE_SLICE,
                           BE_QUANTISER_SCALE_CODE,
                           slice_param->quantiser_scale_code);

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "BE_slice = %08x first_field = %d\n", BE_slice, ctx->pic_params->picture_coding_extension.bits.is_first_field);

    psb_cmdbuf_rendec_write(cmdbuf, BE_slice);

    psb_cmdbuf_rendec_end_chunk(cmdbuf);
    psb_cmdbuf_rendec_end_block(cmdbuf);

    ctx->previous_slice_vertical_position = slice_param->slice_vertical_position;
}

static void psb__MPEG2_set_slice_params(context_MPEG2_p ctx)
{
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;

    uint32_t cmd_data;

    psb_cmdbuf_rendec_start_block(cmdbuf);
    psb_cmdbuf_rendec_start_chunk(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, SLICE_PARAMS));

    cmd_data = 0;    /* Build slice parameters */
    REGIO_WRITE_FIELD(cmd_data,
                      MSVDX_CMDS,
                      SLICE_PARAMS,
                      SLICE_FIELD_TYPE,
                      ctx->pic_params->picture_coding_extension.bits.picture_structure - 1);

    REGIO_WRITE_FIELD(cmd_data,
                      MSVDX_CMDS,
                      SLICE_PARAMS,
                      SLICE_CODE_TYPE,
                      PICTURE_CODING_D == ctx->pic_params->picture_coding_type ? 0 : ctx->pic_params->picture_coding_type - 1);

    psb_cmdbuf_rendec_write(cmdbuf, cmd_data);

    *ctx->p_slice_params = cmd_data;

    psb_cmdbuf_rendec_end_chunk(cmdbuf);
    psb_cmdbuf_rendec_end_block(cmdbuf);
}

static void psb__MPEG2_write_qmatrices(context_MPEG2_p ctx)
{
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    int i;

    /* Since we only decode 4:2:0 We only need to the Intra tables.
    Chroma quant tables are only used in Mpeg 4:2:2 and 4:4:4.
    The hardware wants non-intra followed by intra */
    psb_cmdbuf_rendec_start_block(cmdbuf);
    psb_cmdbuf_rendec_start_chunk(cmdbuf, REG_MSVDX_VEC_IQRAM_OFFSET);

    /* todo : optimisation here is to only load the need table */

    /*  NONINTRA_LUMA_Q --> REG_MSVDX_VEC_IQRAM_OFFSET + 0 */
    for (i = 0; i < 16; i++) {
        psb_cmdbuf_rendec_write(cmdbuf, ctx->qmatrix_data[NONINTRA_LUMA_Q][i]);
// drv_debug_msg(VIDEO_DEBUG_GENERAL, "NONINTRA_LUMA_Q[i] = %08x\n", ctx->qmatrix_data[NONINTRA_LUMA_Q][i]);
    }
    /*  INTRA_LUMA_Q --> REG_MSVDX_VEC_IQRAM_OFFSET + (16*4) */
    for (i = 0; i < 16; i++) {
        psb_cmdbuf_rendec_write(cmdbuf, ctx->qmatrix_data[INTRA_LUMA_Q][i]);
// drv_debug_msg(VIDEO_DEBUG_GENERAL, "INTRA_LUMA_Q[i] = %08x\n", ctx->qmatrix_data[INTRA_LUMA_Q][i]);
    }

    psb_cmdbuf_rendec_end_chunk(cmdbuf);
    psb_cmdbuf_rendec_end_block(cmdbuf);
}

static void psb__MPEG2_set_ent_dec(context_MPEG2_p ctx)
{
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;

    uint32_t cmd_data;

    psb_cmdbuf_reg_start_block(cmdbuf, 0);

    cmd_data = 0;     /* Entdec Front-End controls    */
    REGIO_WRITE_FIELD_LITE(cmd_data, MSVDX_VEC,  CR_VEC_ENTDEC_FE_CONTROL,  ENTDEC_FE_PROFILE, 1); /* MPEG2 Main Profile */
    REGIO_WRITE_FIELD_LITE(cmd_data, MSVDX_VEC,  CR_VEC_ENTDEC_FE_CONTROL,  ENTDEC_FE_MODE,       3); /* Set MPEG2 mode */

    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC, CR_VEC_ENTDEC_FE_CONTROL), cmd_data);

    psb_cmdbuf_reg_end_block(cmdbuf);

    psb_cmdbuf_rendec_start_block(cmdbuf);
    psb_cmdbuf_rendec_start_chunk(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_VEC, CR_VEC_ENTDEC_BE_CONTROL));

    cmd_data = 0;     /* Entdec Back-End controls    */
    REGIO_WRITE_FIELD(cmd_data,
                      MSVDX_VEC,
                      CR_VEC_ENTDEC_BE_CONTROL,
                      ENTDEC_BE_PROFILE,
                      1);                                /* MPEG2 Main Profile        */

    REGIO_WRITE_FIELD(cmd_data,
                      MSVDX_VEC,
                      CR_VEC_ENTDEC_BE_CONTROL,
                      ENTDEC_BE_MODE,
                      3);                                /* Set MPEG2 mode            */

    psb_cmdbuf_rendec_write(cmdbuf, cmd_data);

    psb_cmdbuf_rendec_end_chunk(cmdbuf);
    psb_cmdbuf_rendec_end_block(cmdbuf);
}

static void psb__MPEG2_write_kick(context_MPEG2_p ctx, VASliceParameterBufferMPEG2 *slice_param)
{
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;

    (void) slice_param; /* Unused for now */

    *cmdbuf->cmd_idx++ = CMD_COMPLETION;
}

static void psb__MPEG2_FE_state(context_MPEG2_p ctx)
{
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;

    /* See RENDER_BUFFER_HEADER */
    *cmdbuf->cmd_idx++ = CMD_HEADER;

    ctx->p_slice_params = cmdbuf->cmd_idx;
    *cmdbuf->cmd_idx++ = 0; /* ui32SliceParams */

    cmdbuf->cmd_idx++; /* skip two lldma addr field */

    cmdbuf->cmd_idx++;
}

static VAStatus psb__MPEG2_process_slice(context_MPEG2_p ctx,
        VASliceParameterBufferMPEG2 *slice_param,
        object_buffer_p obj_buffer)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    ASSERT((obj_buffer->type == VASliceDataBufferType) || (obj_buffer->type == VAProtectedSliceDataBufferType));

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "MPEG2 process slice\n");
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "    size = %08x offset = %08x\n", slice_param->slice_data_size, slice_param->slice_data_offset);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "    vertical pos = %d\n", slice_param->slice_vertical_position);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "    slice_data_flag = %d\n", slice_param->slice_data_flag);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "    coded size = %dx%d\n", ctx->picture_width_mb, ctx->picture_height_mb);

    if ((slice_param->slice_data_flag == VA_SLICE_DATA_FLAG_BEGIN) ||
        (slice_param->slice_data_flag == VA_SLICE_DATA_FLAG_ALL)) {
        if (0 == slice_param->slice_data_size) {
            vaStatus = VA_STATUS_ERROR_UNKNOWN;
            DEBUG_FAILURE;
            return vaStatus;
        }

        ASSERT(!ctx->split_buffer_pending);

        /* Initialise the command buffer */
        psb_context_get_next_cmdbuf(ctx->obj_context);

        psb__MPEG2_FE_state(ctx);
        psb__MPEG2_write_VLC_tables(ctx);

        psb_cmdbuf_lldma_write_bitstream(ctx->obj_context->cmdbuf,
                                         obj_buffer->psb_buffer,
                                         obj_buffer->psb_buffer->buffer_ofs + slice_param->slice_data_offset,
                                         slice_param->slice_data_size,
                                         slice_param->macroblock_offset,
                                         0);

        if (slice_param->slice_data_flag == VA_SLICE_DATA_FLAG_BEGIN) {
            ctx->split_buffer_pending = TRUE;
        }
    } else {
        ASSERT(ctx->split_buffer_pending);
        ASSERT(0 == slice_param->slice_data_offset);
        /* Create LLDMA chain to continue buffer */
        if (slice_param->slice_data_size) {
            psb_cmdbuf_lldma_write_bitstream_chained(ctx->obj_context->cmdbuf,
                    obj_buffer->psb_buffer,
                    slice_param->slice_data_size);
        }
    }

    if ((slice_param->slice_data_flag == VA_SLICE_DATA_FLAG_ALL) ||
        (slice_param->slice_data_flag == VA_SLICE_DATA_FLAG_END)) {
        if (slice_param->slice_data_flag == VA_SLICE_DATA_FLAG_END) {
            ASSERT(ctx->split_buffer_pending);
        }

        psb__MPEG2_set_operating_mode(ctx);

        psb__MPEG2_set_reference_pictures(ctx);

        psb__MPEG2_set_picture_header(ctx, slice_param);

        psb__MPEG2_set_slice_params(ctx);

        psb__MPEG2_write_qmatrices(ctx);

        psb__MPEG2_set_ent_dec(ctx);

        psb__MPEG2_write_kick(ctx, slice_param);

        ctx->split_buffer_pending = FALSE;
        ctx->obj_context->video_op = psb_video_vld;
        ctx->obj_context->flags = 0;
        ctx->obj_context->first_mb = 0;
        ctx->obj_context->last_mb = ((ctx->picture_height_mb - 1) << 8) | (ctx->picture_width_mb - 1);
        if (psb_context_submit_cmdbuf(ctx->obj_context)) {
            vaStatus = VA_STATUS_ERROR_UNKNOWN;
        }
    }
    return vaStatus;
}

static VAStatus psb__MPEG2_process_slice_data(context_MPEG2_p ctx, object_buffer_p obj_buffer)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VASliceParameterBufferMPEG2 *slice_param;
    int buffer_idx = 0;
    unsigned int element_idx = 0;

    ASSERT((obj_buffer->type == VASliceDataBufferType) || (obj_buffer->type == VAProtectedSliceDataBufferType));

    ASSERT(ctx->got_iq_matrix);
    ASSERT(ctx->pic_params);
    ASSERT(ctx->slice_param_list_idx);

    if (!ctx->got_iq_matrix || !ctx->pic_params) {
        /* IQ matrix or Picture params missing */
        return VA_STATUS_ERROR_UNKNOWN;
    }
    if ((NULL == obj_buffer->psb_buffer) ||
        (0 == obj_buffer->size)) {
        /* We need to have data in the bitstream buffer */
        return VA_STATUS_ERROR_UNKNOWN;
    }

    while (buffer_idx < ctx->slice_param_list_idx) {
        object_buffer_p slice_buf = ctx->slice_param_list[buffer_idx];
        if (element_idx >= slice_buf->num_elements) {
            /* Move to next buffer */
            element_idx = 0;
            buffer_idx++;
            continue;
        }

        slice_param = (VASliceParameterBufferMPEG2 *) slice_buf->buffer_data;
        slice_param += element_idx;
        element_idx++;
        vaStatus = psb__MPEG2_process_slice(ctx, slice_param, obj_buffer);
        if (vaStatus != VA_STATUS_SUCCESS) {
            DEBUG_FAILURE;
            break;
        }
    }
    ctx->slice_param_list_idx = 0;

    return vaStatus;
}

static void psb__MEPG2_send_highlevel_cmd(context_MPEG2_p ctx)
{
    uint32_t cmd;
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    psb_surface_p target_surface = ctx->obj_context->current_render_target->psb_surface;

    psb_cmdbuf_reg_start_block(cmdbuf, 0);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_CMDS, DISPLAY_PICTURE_SIZE), ctx->display_picture_size);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_CMDS, CODED_PICTURE_SIZE), ctx->coded_picture_size);

    cmd = 0;
    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, OPERATING_MODE, CHROMA_FORMAT, 1);
    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, OPERATING_MODE, ASYNC_MODE, 1); // 0 = VDMC and VDEB active.  1 = VDEB pass-thru.
    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, OPERATING_MODE, CODEC_MODE, 3);        // MPEG2
    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, OPERATING_MODE, CODEC_PROFILE, 1); // MAIN
    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, OPERATING_MODE, ROW_STRIDE, target_surface->stride_mode);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_CMDS, OPERATING_MODE), cmd);

    psb_cmdbuf_reg_set_RELOC(cmdbuf, REGISTER_OFFSET(MSVDX_CMDS, LUMA_RECONSTRUCTED_PICTURE_BASE_ADDRESSES),
                             &target_surface->buf, target_surface->buf.buffer_ofs);

    psb_cmdbuf_reg_set_RELOC(cmdbuf, REGISTER_OFFSET(MSVDX_CMDS, CHROMA_RECONSTRUCTED_PICTURE_BASE_ADDRESSES),
                             &target_surface->buf, target_surface->buf.buffer_ofs + target_surface->chroma_offset);

    psb_cmdbuf_reg_set_RELOC(cmdbuf, REGISTER_OFFSET(MSVDX_CMDS, REFERENCE_PICTURE_BASE_ADDRESSES) + (0 * 8),
                             &target_surface->buf, target_surface->buf.buffer_ofs);

    psb_cmdbuf_reg_set_RELOC(cmdbuf, REGISTER_OFFSET(MSVDX_CMDS, REFERENCE_PICTURE_BASE_ADDRESSES) + 4 + (0 * 8),
                             &target_surface->buf, target_surface->buf.buffer_ofs + target_surface->chroma_offset);

    psb_cmdbuf_reg_set_RELOC(cmdbuf, REGISTER_OFFSET(MSVDX_CMDS, REFERENCE_PICTURE_BASE_ADDRESSES) + (1 * 8),
                             &target_surface->buf, target_surface->buf.buffer_ofs);

    psb_cmdbuf_reg_set_RELOC(cmdbuf, REGISTER_OFFSET(MSVDX_CMDS, REFERENCE_PICTURE_BASE_ADDRESSES) + 4 + (1 * 8),
                             &target_surface->buf, target_surface->buf.buffer_ofs + target_surface->chroma_offset);

    cmd = 0;
    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, SLICE_PARAMS, SLICE_FIELD_TYPE,   2); /* FRAME PICTURE -- ui8SliceFldType */
    REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, SLICE_PARAMS, SLICE_CODE_TYPE,    1); /* P PICTURE -- (ui8PicType == WMF_PTYPE_BI) ? WMF_PTYPE_I : (ui8PicType & 0x3) */
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_CMDS, SLICE_PARAMS), cmd);
    *ctx->p_slice_params = cmd;
    psb_cmdbuf_reg_end_block(cmdbuf);


    psb_cmdbuf_reg_start_block(cmdbuf, 0);
    psb_cmdbuf_reg_set_RELOC(cmdbuf, REGISTER_OFFSET(MSVDX_CMDS, VC1_LUMA_RANGE_MAPPING_BASE_ADDRESS),
                             &target_surface->buf, target_surface->buf.buffer_ofs);

    psb_cmdbuf_reg_set_RELOC(cmdbuf, REGISTER_OFFSET(MSVDX_CMDS, VC1_CHROMA_RANGE_MAPPING_BASE_ADDRESS),
                             &target_surface->buf, target_surface->buf.buffer_ofs + target_surface->chroma_offset);
    psb_cmdbuf_reg_end_block(cmdbuf);

}

static void psb__MEPG2_send_blit_cmd(context_MPEG2_p ctx)
{
    uint32_t cmd;
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    psb_surface_p rotate_surface = ctx->obj_context->current_render_target->out_loop_surface;

    psb_cmdbuf_reg_start_block(cmdbuf, 0);
    cmd = 0;
    REGIO_WRITE_FIELD_LITE(cmd, MSVDX_CMDS, ALTERNATIVE_OUTPUT_PICTURE_ROTATION , ALT_PICTURE_ENABLE, 1);
    REGIO_WRITE_FIELD_LITE(cmd, MSVDX_CMDS, ALTERNATIVE_OUTPUT_PICTURE_ROTATION , ROTATION_ROW_STRIDE, rotate_surface->stride_mode);
    REGIO_WRITE_FIELD_LITE(cmd, MSVDX_CMDS, ALTERNATIVE_OUTPUT_PICTURE_ROTATION , RECON_WRITE_DISABLE, 0);
    REGIO_WRITE_FIELD_LITE(cmd, MSVDX_CMDS, ALTERNATIVE_OUTPUT_PICTURE_ROTATION , ROTATION_MODE, GET_SURFACE_INFO_rotate(rotate_surface));
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_CMDS, ALTERNATIVE_OUTPUT_PICTURE_ROTATION), cmd);
    psb_cmdbuf_reg_end_block(cmdbuf);

    *cmdbuf->cmd_idx++ = 0x40000000; /* CMD_BLIT_CMD */
    *cmdbuf->cmd_idx++ = ctx->picture_width_mb;
    *cmdbuf->cmd_idx++ = ctx->picture_height_mb * 2; /* FIXME */
}

static void psb__MPEG2_insert_blit_cmd_to_rotate(context_MPEG2_p ctx)
{
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;

    /* See RENDER_BUFFER_HEADER */
    *cmdbuf->cmd_idx++ = CMD_HEADER; /* FIXME  use CMD_HEADER_VC1? */

    ctx->p_slice_params = cmdbuf->cmd_idx;
    *cmdbuf->cmd_idx++ = 0; /* ui32SliceParams */

    cmdbuf->cmd_idx++; /* skip two lldma addr field */
    cmdbuf->cmd_idx++;

    psb__MEPG2_send_highlevel_cmd(ctx);
    psb__MEPG2_send_blit_cmd(ctx);
}

static VAStatus psb_MPEG2_BeginPicture(
    object_context_p obj_context)
{
    INIT_CONTEXT_MPEG2

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psb_MPEG2_BeginPicture\n");
    if (ctx->pic_params) {
        free(ctx->pic_params);
        ctx->pic_params = NULL;
    }
    ctx->previous_slice_vertical_position = ~1;

    return VA_STATUS_SUCCESS;
}

static VAStatus psb_MPEG2_RenderPicture(
    object_context_p obj_context,
    object_buffer_p *buffers,
    int num_buffers)
{
    int i;
    INIT_CONTEXT_MPEG2
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    for (i = 0; i < num_buffers; i++) {
        object_buffer_p obj_buffer = buffers[i];

        switch (obj_buffer->type) {
        case VAPictureParameterBufferType:
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "psb_MPEG2_RenderPicture got VAPictureParameterBuffer\n");
            vaStatus = psb__MPEG2_process_picture_param(ctx, obj_buffer);
            DEBUG_FAILURE;
            break;

        case VAIQMatrixBufferType:
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "psb_MPEG2_RenderPicture got VAIQMatrixBufferType\n");
            vaStatus = psb__MPEG2_process_iq_matrix(ctx, obj_buffer);
            DEBUG_FAILURE;
            break;

        case VASliceParameterBufferType:
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "psb_MPEG2_RenderPicture got VASliceParameterBufferType\n");
            vaStatus = psb__MPEG2_add_slice_param(ctx, obj_buffer);
            DEBUG_FAILURE;
            break;

        case VASliceDataBufferType:
        case VAProtectedSliceDataBufferType:

            drv_debug_msg(VIDEO_DEBUG_GENERAL, "psb_MPEG2_RenderPicture got %s\n", SLICEDATA_BUFFER_TYPE(obj_buffer->type));
            vaStatus = psb__MPEG2_process_slice_data(ctx, obj_buffer);
            DEBUG_FAILURE;
            break;

        default:
            vaStatus = VA_STATUS_ERROR_UNKNOWN;
            DEBUG_FAILURE;
        }
        if (vaStatus != VA_STATUS_SUCCESS) {
            break;
        }
    }

    return vaStatus;
}

static VAStatus psb_MPEG2_EndPicture(
    object_context_p obj_context)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    INIT_CONTEXT_MPEG2

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psb_MPEG2_EndPicture\n");

    if (CONTEXT_ROTATE(ctx->obj_context)) {
        if (!(ctx->pic_params->picture_coding_extension.bits.progressive_frame) &&
            !(ctx->pic_params->picture_coding_extension.bits.is_first_field))
            psb__MPEG2_insert_blit_cmd_to_rotate(ctx);
    }

    if (psb_context_flush_cmdbuf(ctx->obj_context)) {
        vaStatus = VA_STATUS_ERROR_UNKNOWN;
    }

    if (ctx->pic_params) {
        free(ctx->pic_params);
        ctx->pic_params = NULL;
    }

    return vaStatus;
}

struct format_vtable_s psb_MPEG2_vtable = {
queryConfigAttributes:
    psb_MPEG2_QueryConfigAttributes,
validateConfig:
    psb_MPEG2_ValidateConfig,
createContext:
    psb_MPEG2_CreateContext,
destroyContext:
    psb_MPEG2_DestroyContext,
beginPicture:
    psb_MPEG2_BeginPicture,
renderPicture:
    psb_MPEG2_RenderPicture,
endPicture:
    psb_MPEG2_EndPicture
};
