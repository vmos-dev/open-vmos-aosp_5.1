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
*/
#ifndef _TNG_SLOTORDER_H_
#define _TNG_SLOTORDER_H_

#include "tng_hostdefs.h"

#define SLOT_STAUS_OCCUPIED 1
#define SLOT_STAUS_EMPTY 0

//#define FRAME_I 0
//#define FRAME_P 1
//#define FRAME_B 2
//#define FRAME_IDR 3

typedef struct _FRAME_ORDER_INFO {
    unsigned long long max_dpy_num;
    int *slot_consume_dpy_order;
    int *slot_consume_enc_order;
    IMG_FRAME_TYPE last_frame_type;
    short last_slot;
} FRAME_ORDER_INFO;

/* Input, the encoding order, start from 0
 * Input, The number of B frames between P and I 
 * Input, Intra period
 * Input & Output. Reset to 0 on first call
 * Output. The displaying order
 * Output. Frame type. 1: I frame. 2: P frame. 3: B frame
 * Output. The corresponding slot index
 */

int getFrameDpyOrder(
    unsigned long long encoding_count, /*Input, the encoding order, start from 0*/ 
    int bframes, /*Input, The number of B frames between P and I */
    int intracnt, /*Input, Intra period*/
    int idrcnt, /*INput, IDR period. 0: only one IDR; */
    FRAME_ORDER_INFO *p_last_info, /*Input & Output. Reset to 0 on first call*/
    unsigned long long *displaying_order); /* Output. The displaying order */
#endif  //_TNG_SLOTORDER_H_
