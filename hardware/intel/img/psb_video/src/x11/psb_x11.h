/*
 * Copyright (c) 2011 Intel Corporation. All Rights Reserved.
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
 *
 */



#ifndef _PSB_X11_H_
#define _PSB_X11_H_

#include <X11/Xutil.h>

#include <inttypes.h>
#include "psb_drv_video.h"
#include "psb_drm.h"
#include "psb_surface.h"
#include "psb_output.h"
#include "psb_surface_ext.h"

#include <va/va.h>

#define USING_OVERLAY_PORT  1
#define USING_TEXTURE_PORT  2

typedef struct {
    /*src coordinate*/
    short srcx;
    short srcy;
    unsigned short sWidth;
    unsigned short sHeight;
    /*dest coordinate*/
    short destx;
    short desty;
    unsigned short dWidth;
    unsigned short dHeight;
} psb_overlay_rect_t, *psb_overlay_rect_p;

typedef struct {
    int                 i32Left;
    int                 i32Top;
    int                 i32Right;
    int                 i32Bottom;
    unsigned int        ui32Width;
    unsigned int        ui32Height;
} psb_x11_win_t;

typedef struct x11_rect_list {
    psb_x11_win_t     rect;
    struct x11_rect_list * next;
} psb_x11_clip_list_t;

typedef struct _psb_x11_output_s {
    /* information for xvideo */
    XvPortID textured_portID;
    XvPortID overlay_portID;
    XvImage *textured_xvimage;
    XvImage *overlay_xvimage;
    PsbXvVAPutSurfaceRec        imgdata_vasrf;
    GC                          gc;
    Drawable                    output_drawable;
    int                         is_pixmap;
    Drawable                    output_drawable_save;
    GC                          extend_gc;
    Drawable                    extend_drawable;
    unsigned short              output_width;
    unsigned short              output_height;
    int                         using_port;

    int bIsVisible;
    psb_x11_win_t winRect;
    psb_x11_clip_list_t *pClipBoxList;
    unsigned int ui32NumClipBoxList;
    unsigned int frame_count;

    int ignore_dpm;

    /*for video rotation with overlay adaptor*/
    psb_surface_p rotate_surface;
    int rotate_surfaceID;
    int rotate;
    unsigned int sprite_enabled;

} psb_x11_output_s, *psb_x11_output_p;

VAStatus psb_putsurface_coverlay(
    VADriverContextP ctx,
    VASurfaceID surface,
    Drawable draw, /* X Drawable */
    short srcx,
    short srcy,
    unsigned short srcw,
    unsigned short srch,
    short destx,
    short desty,
    unsigned short destw,
    unsigned short desth,
    VARectangle *cliprects, /* client supplied clip list */
    unsigned int number_cliprects, /* number of clip rects in the clip list */
    unsigned int flags /* de-interlacing flags */
);


VAStatus psb_putsurface_xvideo(
    VADriverContextP ctx,
    VASurfaceID surface,
    Drawable draw,
    short srcx,
    short srcy,
    unsigned short srcw,
    unsigned short srch,
    short destx,
    short desty,
    unsigned short destw,
    unsigned short desth,
    VARectangle *cliprects, /* client supplied clip list */
    unsigned int number_cliprects, /* number of clip rects in the clip list */
    unsigned int flags /* de-interlacing flags */
);

VAStatus psb_putsurface_ctexture(
    VADriverContextP ctx,
    VASurfaceID surface,
    Drawable draw,
    short srcx,
    short srcy,
    unsigned short srcw,
    unsigned short srch,
    short destx,
    short desty,
    unsigned short destw,
    unsigned short desth,
    unsigned int flags /* de-interlacing flags */
);

VAStatus psb_init_xvideo(VADriverContextP ctx, psb_x11_output_p output);
VAStatus psb_deinit_xvideo(VADriverContextP ctx);

#endif
