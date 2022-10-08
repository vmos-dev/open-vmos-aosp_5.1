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


#ifndef _LLDMA_DEFS_H_
#define _LLDMA_DEFS_H_

typedef enum {
    LLDMA_TYPE_VLC_TABLE ,
    LLDMA_TYPE_BITSTREAM ,
    LLDMA_TYPE_RESIDUAL ,
    LLDMA_TYPE_RENDER_BUFF_MC,
    LLDMA_TYPE_RENDER_BUFF_VLD,

    LLDMA_TYPE_MPEG4_FESTATE_SAVE,
    LLDMA_TYPE_MPEG4_FESTATE_RESTORE,

    LLDMA_TYPE_H264_PRELOAD_SAVE,
    LLDMA_TYPE_H264_PRELOAD_RESTORE,

    LLDMA_TYPE_VC1_PRELOAD_SAVE,
    LLDMA_TYPE_VC1_PRELOAD_RESTORE,

    LLDMA_TYPE_MEM_SET,

} LLDMA_TYPE;

#endif /* _LLDMA_DEFS_H_ */
