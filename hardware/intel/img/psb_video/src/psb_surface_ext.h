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
 *    Shengquan Yuan  <shengquan.yuan@intel.com>
 *    Zhaohan Ren  <zhaohan.ren@intel.com>
 */

#ifndef _PSB_XVVA_H
#define _PSB_XVVA_H

#include <pthread.h>
#include <stdint.h>


#ifndef MAKEFOURCC

#define MAKEFOURCC(ch0, ch1, ch2, ch3)                                  \
    ((unsigned long)(unsigned char) (ch0) | ((unsigned long)(unsigned char) (ch1) << 8) | \
    ((unsigned long)(unsigned char) (ch2) << 16) | ((unsigned long)(unsigned char) (ch3) << 24 ))

/* a few common FourCCs */
#define VA_FOURCC_AI44         0x34344149
#define VA_FOURCC_UYVY          0x59565955
#define VA_FOURCC_YUY2          0x32595559
#define VA_FOURCC_AYUV          0x56555941
#define VA_FOURCC_NV11          0x3131564e
#define VA_FOURCC_YV12          0x32315659
#define VA_FOURCC_P208          0x38303250
#define VA_FOURCC_IYUV          0x56555949
#define VA_FOURCC_I420          0x30323449

#endif

/* XvDrawable information */
#define XVDRAWABLE_NORMAL       0x00
#define XVDRAWABLE_PIXMAP       0x01
#define XVDRAWABLE_ROTATE_90    0x02
#define XVDRAWABLE_ROTATE_180   0x04
#define XVDRAWABLE_ROTATE_270   0x08
#define XVDRAWABLE_REDIRECT_WINDOW 0x10
#define XVDRAWABLE_SCALE        0x20

#define XVDRAWABLE_INVALID_DRAWABLE     0x8000

typedef struct _PsbAYUVSample8 {
    unsigned char     Cr;
    unsigned char     Cb;
    unsigned char     Y;
    unsigned char     Alpha;
} PsbAYUVSample8;

typedef struct _VaClipBox {
    short x;
    short y;
    unsigned short width;
    unsigned short height;
} VaClipBox;


struct _PsbVASurface {
    struct _PsbVASurface *next; /* next subpicture, only used by client */

    struct _WsbmBufferObject *bo;
    uint32_t bufid;
    uint64_t pl_flags; /* placement */
    uint32_t size;

    unsigned int fourcc;
    unsigned int planar;
    unsigned int width;
    unsigned int height;
    unsigned int bytes_pp;
    unsigned int stride;
    unsigned int pre_add;
    unsigned int reserved_phyaddr; /* for reserved memory, e.g. CI/RAR */

    unsigned int clear_color;

    unsigned int subpic_id; /* subpic id, only used by client */
    unsigned int subpic_flags;/* flags for subpictures
                               * #define VA_SUBPICTURE_CHROMA_KEYING    0x0001
                               * #define VA_SUBPICTURE_GLOBAL_ALPHA     0x0002
                                *#define VA_SUBPICTURE_DESTINATION_IS_SCREEN_COORD 0x0004
                               */
    float global_alpha;
    unsigned int chromakey_min;
    unsigned int chromakey_max;
    unsigned int chromakey_mask;

    PsbAYUVSample8 *palette_ptr; /* point to image palette */
    union {
        uint32_t  palette[16]; /* used to pass palette to server */
        PsbAYUVSample8  constant[16]; /* server convert palette into SGX constants */
    };
    int subpic_srcx;
    int subpic_srcy;
    int subpic_srcw;
    int subpic_srch;

    int subpic_dstx;
    int subpic_dsty;
    int subpic_dstw;
    int subpic_dsth;

    /* only used by server side */
    unsigned int num_constant;
    unsigned int *constants;

    unsigned int mem_layout;
    unsigned int tex_fmt;
    unsigned int pack_mode;

    unsigned int fragment_start;
    unsigned int fragment_end;
};

typedef struct _PsbVASurface PsbVASurfaceRec;
typedef struct _PsbVASurface *PsbVASurfacePtr;


#ifndef VA_FRAME_PICTURE

/* de-interlace flags for vaPutSurface */
#define VA_FRAME_PICTURE        0x00000000
#define VA_TOP_FIELD            0x00000001
#define VA_BOTTOM_FIELD         0x00000002
/*
 * clears the drawable with background color.
 * for hardware overlay based implementation this flag
 * can be used to turn off the overlay
 */
#define VA_CLEAR_DRAWABLE       0x00000008

/* color space conversion flags for vaPutSurface */
#define VA_SRC_BT601            0x00000010
#define VA_SRC_BT709            0x00000020

#endif /* end for _VA_X11_H_ */



#define PSB_SUBPIC_MAX_NUM      6
#define PSB_CLIPBOX_MAX_NUM     6

typedef struct _PsbXvVAPutSurface {
    uint32_t flags;/* #define VA_FRAME_PICTURE 0x00000000
                    * #define VA_TOP_FIELD     0x00000001
                    * #define VA_BOTTOM_FIELD  0x00000002
                    */
    unsigned int num_subpicture;
    unsigned int num_clipbox;

    PsbVASurfaceRec dst_srf; /* filled by Xserver */
    PsbVASurfaceRec src_srf; /* provided by VA client */
    PsbVASurfaceRec subpic_srf[PSB_SUBPIC_MAX_NUM];
    VaClipBox clipbox[PSB_CLIPBOX_MAX_NUM];
} PsbXvVAPutSurfaceRec, *PsbXvVAPutSurfacePtr;

#endif
