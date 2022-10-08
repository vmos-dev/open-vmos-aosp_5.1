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
 *    Jason Hu <jason.hu@intel.com>
 */

#ifndef _PSB_OUTPUT_ANDROID_H_
#define _PSB_OUTPUT_ANDROID_H_

typedef struct _psb_android_output_s {
    /* information of output display */
    unsigned short screen_width;
    unsigned short screen_height;

    /* for memory heap base used by putsurface */
    unsigned char* heap_addr;

    //void* psb_HDMIExt_info; /* HDMI extend video mode info */
    int sf_composition; /* surfaceflinger compostion */
    /* save dest box here */
    short destx;
    short desty;
    unsigned short destw;
    unsigned short desth;
    int new_destbox;
#ifdef TARGET_HAS_MULTIPLE_DISPLAY
    void* mds;/* mds listener */
#endif
} psb_android_output_s, *psb_android_output_p;

#endif /*_PSB_OUTPUT_ANDROID_H_*/
