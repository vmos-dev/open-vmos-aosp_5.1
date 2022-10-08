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


#define F_MASK(basename)  (MASK_##basename)
#define F_SHIFT(basename) (SHIFT_##basename)

#define F_ENCODE(val,basename)  (((val)<<(F_SHIFT(basename)))&(F_MASK(basename)))
#define F_ENCODE_MVEA(val,basename)  (((val)<<(F_SHIFT_MVEA(basename)))&(F_MASK_MVEA(basename)))

#define MVEA_LRB_SEARCH_HEIGHT 80
#define MVEA_LRB_TOP_OFFSET    (((MVEA_LRB_SEARCH_HEIGHT/2)/16)*16)


#define F_MASK_MVEA(basename)  (MASK_MVEA_##basename)   /*      MVEA    */
#define F_SHIFT_MVEA(basename) (SHIFT_MVEA_##basename)   /*     MVEA    */


#define MASK_MVEA_CR_IPE_GRID_FINE_SEARCH 0x00000C00
#define SHIFT_MVEA_CR_IPE_GRID_FINE_SEARCH 10
#define REGNUM_MVEA_CR_IPE_GRID_FINE_SEARCH 0x0200


#define MASK_MVEA_CR_IPE_GRID_SEARCH_SIZE 0x00000380
#define SHIFT_MVEA_CR_IPE_GRID_SEARCH_SIZE 7
#define REGNUM_MVEA_CR_IPE_GRID_SEARCH_SIZE 0x0200


#define MVEA_CR_IPE_CONTROL         0x0200
#define MASK_MVEA_CR_IPE_BLOCKSIZE  0x00000003
#define SHIFT_MVEA_CR_IPE_BLOCKSIZE 0
#define REGNUM_MVEA_CR_IPE_BLOCKSIZE 0x0200



#define MASK_MVEA_CR_IPE_ENCODING_FORMAT 0x00003000
#define SHIFT_MVEA_CR_IPE_ENCODING_FORMAT 12
#define REGNUM_MVEA_CR_IPE_ENCODING_FORMAT 0x0200


#define MASK_MVEA_CR_IPE_Y_CANDIDATE_NUM 0x0000003C
#define SHIFT_MVEA_CR_IPE_Y_CANDIDATE_NUM 2
#define REGNUM_MVEA_CR_IPE_Y_CANDIDATE_NUM 0x0200


#define MASK_MVEA_CR_IPE_Y_FINE_SEARCH 0x00000040
#define SHIFT_MVEA_CR_IPE_Y_FINE_SEARCH 6
#define REGNUM_MVEA_CR_IPE_Y_FINE_SEARCH 0x0200

#define MASK_MVEA_CR_JMCOMP_AC_ENABLE 0x00008000
#define SHIFT_MVEA_CR_JMCOMP_AC_ENABLE 15
#define REGNUM_MVEA_CR_JMCOMP_AC_ENABLE 0x0280


#define MB_START_OF_SLICE                       (1<<5)
#define MB_END_OF_SLICE                         (1<<4)
#define MB_END_OF_PICTURE                       (1<<6)
#define MB_END_OF_ROW                           (1<<7)
#define PARAMS_ABOVEL_VALID                     (1<<0)
#define PARAMS_ABOVE_VALID                      (1<<1)
#define PARAMS_ABOVER_VALID                     (1<<2)
#define PARAMS_LEFT_VALID                       (1<<3)#define VECTORS_ABOVE_LEFT_VALID        (1<<0)
#define VECTORS_ABOVE_VALID                     (1<<1)
#define VECTORS_LEFT_VALID                      (1<<2)
#define VECTORS_BELOW_VALID                     (1<<4)
#define VECTORS_2BELOW_VALID            (1<<5)


#define MVEA_CR_JMCOMP_CONTROL      0x0280
#define MASK_MVEA_CR_JMCOMP_MODE    0x00000003
#define SHIFT_MVEA_CR_JMCOMP_MODE   0
#define REGNUM_MVEA_CR_JMCOMP_MODE  0x0280

#define MASK_TOPAZ_VLC_CR_CODEC     0x00000003
#define SHIFT_TOPAZ_VLC_CR_CODEC    0

#define MASK_TOPAZ_VLC_CR_SLICE_CODING_TYPE 0x0000000C
#define SHIFT_TOPAZ_VLC_CR_SLICE_CODING_TYPE 2












