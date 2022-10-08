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
 *    Jason Hu  <jason.hu@intel.com>
 *    Zhaohan Ren  <zhaohan.ren@intel.com>
 *    Shengquan Yuan  <shengquan.yuan@intel.com>
 *
 */

#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>
#include <va/va_backend.h>
#include "psb_output.h"
#include "psb_surface.h"
#include "psb_buffer.h"
#include "psb_x11.h"
#include "psb_drv_debug.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "psb_surface_ext.h"
#include <wsbm/wsbm_manager.h>
#include "psb_drv_video.h"
#include "psb_xrandr.h"
#include <sys/types.h>

#define INIT_DRIVER_DATA    psb_driver_data_p driver_data = (psb_driver_data_p) ctx->pDriverData
#define INIT_OUTPUT_PRIV    psb_x11_output_p output = (psb_x11_output_p)(((psb_driver_data_p)ctx->pDriverData)->ws_priv)
#define SURFACE(id)     ((object_surface_p) object_heap_lookup( &driver_data->surface_heap, id ))

static int
psb_x11_getWindowCoordinate(Display * display,
                            Window x11_window_id,
                            psb_x11_win_t * psX11Window,
                            int * pbIsVisible)
{
    Window DummyWindow;
    Status status;
    XWindowAttributes sXWinAttrib;

    if ((status = XGetWindowAttributes(display,
                                       x11_window_id,
                                       &sXWinAttrib)) == 0) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: Failed to get X11 window coordinates - error %lu\n", __func__, (unsigned long)status);
        return -1;
    }

    psX11Window->i32Left = sXWinAttrib.x;
    psX11Window->i32Top = sXWinAttrib.y;
    psX11Window->ui32Width = sXWinAttrib.width;
    psX11Window->ui32Height = sXWinAttrib.height;
    *pbIsVisible = (sXWinAttrib.map_state == IsViewable);

    if (!*pbIsVisible)
        return 0;

    if (XTranslateCoordinates(display,
                              x11_window_id,
                              DefaultRootWindow(display),
                              0,
                              0,
                              &psX11Window->i32Left,
                              &psX11Window->i32Top,
                              &DummyWindow) == 0) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: Failed to tranlate X coordinates - error %lu\n", __func__, (unsigned long)status);
        return -1;
    }

    psX11Window->i32Right  = psX11Window->i32Left + psX11Window->ui32Width - 1;
    psX11Window->i32Bottom = psX11Window->i32Top + psX11Window->ui32Height - 1;

    return 0;
}
static psb_x11_clip_list_t *
psb_x11_createClipBoxNode(psb_x11_win_t *       pRect,
                          psb_x11_clip_list_t * pClipNext)
{
    psb_x11_clip_list_t * pCurrent = NULL;

    pCurrent = (psb_x11_clip_list_t *)calloc(1, sizeof(psb_x11_clip_list_t));
    if (pCurrent) {
        pCurrent->rect = *pRect;
        pCurrent->next = pClipNext;

        return pCurrent;
    } else
        return pClipNext;
}

void
psb_x11_freeWindowClipBoxList(psb_x11_clip_list_t * pHead)
{
    psb_x11_clip_list_t * pNext = NULL;

    while (pHead) {
        pNext = pHead->next;
        free(pHead);
        pHead = pNext;
    }
}

#define IS_BETWEEN_RANGE(a,b,c) ((a<=b)&&(b<=c))

static psb_x11_clip_list_t *
psb_x11_substractRects(Display *             display,
                       psb_x11_clip_list_t * psRegion,
                       psb_x11_win_t *       psRect)
{
    psb_x11_clip_list_t * psCur, * psPrev, * psFirst, * psNext;
    psb_x11_win_t sCreateRect;
    int display_width  = (int)(DisplayWidth(display, DefaultScreen(display))) - 1;
    int display_height = (int)(DisplayHeight(display, DefaultScreen(display))) - 1;

    psFirst = psb_x11_createClipBoxNode(psRect, NULL);

    if (psFirst->rect.i32Left < 0)
        psFirst->rect.i32Left = 0;
    else if (psFirst->rect.i32Left > display_width)
        psFirst->rect.i32Left = display_width;

    if (psFirst->rect.i32Right < 0)
        psFirst->rect.i32Right = 0;
    else if (psFirst->rect.i32Right > display_width)
        psFirst->rect.i32Right = display_width;

    if (psFirst->rect.i32Top < 0)
        psFirst->rect.i32Top = 0;
    else if (psFirst->rect.i32Top > display_height)
        psFirst->rect.i32Top = display_height;

    if (psFirst->rect.i32Bottom < 0)
        psFirst->rect.i32Bottom = 0;
    else if (psFirst->rect.i32Bottom > display_height)
        psFirst->rect.i32Bottom = display_height;

    while (psRegion) {
        psCur  = psFirst;
        psPrev = NULL;

        while (psCur) {
            psNext = psCur->next;

            if ((psRegion->rect.i32Left > psCur->rect.i32Left) &&
                (psRegion->rect.i32Left <= psCur->rect.i32Right)) {
                sCreateRect.i32Right = psRegion->rect.i32Left - 1;

                sCreateRect.i32Left = psCur->rect.i32Left;
                sCreateRect.i32Top = psCur->rect.i32Top;
                sCreateRect.i32Bottom = psCur->rect.i32Bottom;

                psFirst = psb_x11_createClipBoxNode(&sCreateRect, psFirst);

                if (!psPrev)
                    psPrev = psFirst;

                psCur->rect.i32Left = psRegion->rect.i32Left;
            }

            if ((psRegion->rect.i32Right >= psCur->rect.i32Left) &&
                (psRegion->rect.i32Right < psCur->rect.i32Right))

            {
                sCreateRect.i32Left = psRegion->rect.i32Right + 1;

                sCreateRect.i32Right = psCur->rect.i32Right;
                sCreateRect.i32Top = psCur->rect.i32Top;
                sCreateRect.i32Bottom = psCur->rect.i32Bottom;

                psFirst = psb_x11_createClipBoxNode(&sCreateRect, psFirst);

                if (!psPrev)
                    psPrev = psFirst;

                psCur->rect.i32Right = psRegion->rect.i32Right;
            }

            if ((psRegion->rect.i32Top > psCur->rect.i32Top) &&
                (psRegion->rect.i32Top <= psCur->rect.i32Bottom)) {
                sCreateRect.i32Bottom = psRegion->rect.i32Top - 1;

                sCreateRect.i32Left   = psCur->rect.i32Left;
                sCreateRect.i32Right  = psCur->rect.i32Right;
                sCreateRect.i32Top    = psCur->rect.i32Top;

                psFirst = psb_x11_createClipBoxNode(&sCreateRect, psFirst);

                if (!psPrev)
                    psPrev = psFirst;

                psCur->rect.i32Top = psRegion->rect.i32Top;
            }

            if ((psRegion->rect.i32Bottom >= psCur->rect.i32Top) &&
                (psRegion->rect.i32Bottom <  psCur->rect.i32Bottom)) {
                sCreateRect.i32Top    = psRegion->rect.i32Bottom + 1;
                sCreateRect.i32Left   = psCur->rect.i32Left;
                sCreateRect.i32Right  = psCur->rect.i32Right;
                sCreateRect.i32Bottom = psCur->rect.i32Bottom;

                psFirst = psb_x11_createClipBoxNode(&sCreateRect, psFirst);

                if (!psPrev)
                    psPrev = psFirst;

                psCur->rect.i32Bottom = psRegion->rect.i32Bottom;
            }

            if ((IS_BETWEEN_RANGE(psRegion->rect.i32Left, psCur->rect.i32Left,   psRegion->rect.i32Right)) &&
                (IS_BETWEEN_RANGE(psRegion->rect.i32Left, psCur->rect.i32Right,  psRegion->rect.i32Right)) &&
                (IS_BETWEEN_RANGE(psRegion->rect.i32Top,  psCur->rect.i32Top,    psRegion->rect.i32Bottom)) &&
                (IS_BETWEEN_RANGE(psRegion->rect.i32Top,  psCur->rect.i32Bottom, psRegion->rect.i32Bottom))) {
                if (psPrev) {
                    psPrev->next = psCur->next;
                    free(psCur);
                    psCur = psPrev;
                } else {
                    free(psCur);
                    psCur = NULL;
                    psFirst = psNext;
                }
            }
            psPrev = psCur;
            psCur  = psNext;
        }//while(psCur)
        psRegion = psRegion->next;
    }//while(psRegion)

    return psFirst;
}

static int
psb_x11_createWindowClipBoxList(Display *              display,
                                Window                 x11_window_id,
                                psb_x11_clip_list_t ** ppWindowClipBoxList,
                                unsigned int *         pui32NumClipBoxList)
{
    Window CurrentWindow = x11_window_id;
    Window RootWindow, ParentWindow, ChildWindow;
    Window * pChildWindow;
    Status XResult;
    unsigned int i32NumChildren, i;
    int bIsVisible;
    unsigned int ui32NumRects = 0;
    psb_x11_clip_list_t *psRegions = NULL;
    psb_x11_win_t sRect;

    if (!display || (!ppWindowClipBoxList) || (!pui32NumClipBoxList))
        return -1;

    XResult = XQueryTree(display,
                         CurrentWindow,
                         &RootWindow,
                         &ParentWindow,
                         &pChildWindow,
                         &i32NumChildren);
    if (XResult == 0)
        return -2;

    if (i32NumChildren) {
        for (i = 0; i < i32NumChildren; i++) {

            psb_x11_getWindowCoordinate(display, x11_window_id, &sRect, &bIsVisible);
            if (bIsVisible) {
                psRegions = psb_x11_createClipBoxNode(&sRect, psRegions);
                ui32NumRects++;
            }
        }
        XFree(pChildWindow);
        i32NumChildren = 0;
    }

    while (CurrentWindow != RootWindow) {
        ChildWindow   = CurrentWindow;
        CurrentWindow = ParentWindow;

        XResult = XQueryTree(display,
                             CurrentWindow,
                             &RootWindow,
                             &ParentWindow,
                             &pChildWindow,
                             &i32NumChildren);
        if (XResult == 0) {
            if (i32NumChildren)
                XFree(pChildWindow);

            psb_x11_freeWindowClipBoxList(psRegions);
            return -3;
        }

        if (i32NumChildren) {
            unsigned int iStartWindow = 0;

            for (i = 0; i < i32NumChildren; i++) {
                if (pChildWindow[i] == ChildWindow) {
                    iStartWindow = i;
                    break;
                }
            }

            if (i == i32NumChildren) {
                XFree(pChildWindow);
                psb_x11_freeWindowClipBoxList(psRegions);
                return -4;
            }

            for (i = iStartWindow + 1; i < i32NumChildren; i++) {
                psb_x11_getWindowCoordinate(display, pChildWindow[i], &sRect, &bIsVisible);
                if (bIsVisible) {
                    psRegions = psb_x11_createClipBoxNode(&sRect, psRegions);
                    ui32NumRects++;
                }
            }

            XFree(pChildWindow);
        }
    }

    ui32NumRects = 0;

    if (psRegions) {
        psb_x11_getWindowCoordinate(display, x11_window_id, &sRect, &bIsVisible);
        *ppWindowClipBoxList = psb_x11_substractRects(display, psRegions, &sRect);
        psb_x11_freeWindowClipBoxList(psRegions);

        psRegions = *ppWindowClipBoxList;

        while (psRegions) {
            ui32NumRects++;
            psRegions = psRegions->next;
        }
    } else {
        *ppWindowClipBoxList = psb_x11_substractRects(display, NULL, &sRect);
        ui32NumRects = 1;
    }

    *pui32NumClipBoxList = ui32NumRects;

    return 0;
}

static int psb_cleardrawable_stopoverlay(
    VADriverContextP ctx,
    Drawable draw, /* X Drawable */
    short destx,
    short desty,
    unsigned short destw,
    unsigned short desth
)
{
    INIT_DRIVER_DATA;
    INIT_OUTPUT_PRIV;

    XFillRectangle((Display *)ctx->native_dpy, draw, output->gc, destx, desty, destw, desth);
    XSync((Display *)ctx->native_dpy, False);

    driver_data->cur_displaying_surface = VA_INVALID_SURFACE;
    driver_data->last_displaying_surface = VA_INVALID_SURFACE;

    return 0;
}

static VAStatus psb_DisplayRGBASubpicture(
    PsbVASurfaceRec *subpicture,
    VADriverContextP ctx,
    int win_width,
    int win_height,
    int surface_x,
    int surface_y,
    int surface_w,
    int surface_h,
    psb_extvideo_subtitle subtitle
)
{
    INIT_DRIVER_DATA;
    INIT_OUTPUT_PRIV;
    XImage *ximg = NULL;
    Visual *visual;
    PsbPortPrivRec *pPriv = (PsbPortPrivPtr)(&driver_data->coverlay_priv);
    struct _WsbmBufferObject *bo = subpicture->bo;
    int image_width, image_height, width, height, size;
    int srcx, srcy, srcw, srch;
    int destx, desty, destw, desth;
    int depth, i;

    if (subpicture->fourcc != VA_FOURCC_RGBA) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: Invalid image format, ONLY support RGBA subpicture now.\n", __func__);
        return VA_STATUS_ERROR_INVALID_IMAGE_FORMAT;
    }

    for (i = 0; subpicture != NULL; subpicture = subpicture->next, i++) {
        srcx = subpicture->subpic_srcx;
        srcy = subpicture->subpic_srcy;
        srcw = subpicture->subpic_srcw;
        srch = subpicture->subpic_srch;

        destx = subpicture->subpic_dstx + surface_x;
        desty = subpicture->subpic_dsty + surface_y;
        destw = subpicture->subpic_dstw;
        desth = subpicture->subpic_dsth;

        image_width = subpicture->width;
        image_height = subpicture->height;
        size = subpicture->size;

        //clip in image region
        if (srcx < 0) {
            srcw += srcx;
            srcx = 0;
        }

        if (srcy < 0) {
            srch += srcy;
            srcy = 0;
        }

        if ((srcx + srcw) > image_width)
            srcw = image_width - srcx;
        if ((srcy + srch) > image_height)
            srch = image_height - srcy;

        //clip in drawable region
        if (destx < 0) {
            destw += destx;
            destx = 0;
        }

        if (desty < 0) {
            desth += desty;
            desty = 0;
        }

        if ((destx + destw) > surface_w)
            destw = surface_w - destx;
        if ((desty + desth) > surface_h)
            desth = surface_h - desty;

        if (srcw <= destw)
            width = srcw;
        else
            width = destw;

        if (srch <= desth)
            height = srch;
        else
            height = desth;

        visual = DefaultVisual(ctx->native_dpy, 0);
        depth = DefaultDepth(ctx->native_dpy, 0);

        ximg = XCreateImage(ctx->native_dpy, visual, depth, ZPixmap, 0, NULL, image_width, image_height, 32, 0);

        if (NULL == ximg) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: XCreateImage failed! at L%d\n", __func__, __LINE__);
            return VA_STATUS_ERROR_UNKNOWN;
        }

        ximg->data = wsbmBOMap(bo, WSBM_ACCESS_READ);
        if (NULL == ximg->data) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: Failed to map to ximg->data.\n", __func__);
            return VA_STATUS_ERROR_ALLOCATION_FAILED;
        }

        pPriv->clear_key[i].subpic_dstx = destx;
        pPriv->clear_key[i].subpic_dsty = desty;
        pPriv->clear_key[i].subpic_dstw = destw;
        pPriv->clear_key[i].subpic_dsth = desth;
        if (psb_xrandr_extvideo_mode()) {
            /*It is a HACK: Adjust subtitle to proper position.*/
            float xScale, yScale;

            xScale = win_width * 1.0 / surface_w;
            yScale = win_height * 1.0 / surface_h;
            destx = subpicture->subpic_dstx * xScale;
            desty = subpicture->subpic_dsty * yScale;
        }
        XPutImage(ctx->native_dpy, output->output_drawable, output->gc, ximg, srcx, srcy, destx, desty, width, height);
        XSync((Display *)ctx->native_dpy, False);

        if (psb_xrandr_extvideo_mode() &&
            (subtitle == ONLY_HDMI || subtitle == BOTH)) {
            float xScale, yScale;

            xScale = pPriv->extend_display_width * 1.0 / surface_w;
            yScale = pPriv->extend_display_height * 1.0 / surface_h;

            destx = subpicture->subpic_dstx * xScale;
            desty = subpicture->subpic_dsty * yScale;

            XPutImage(ctx->native_dpy, output->extend_drawable, output->extend_gc, ximg,
                      srcx, srcy, destx, desty, destw, desth);
            XSync((Display *)ctx->native_dpy, False);
        }

        pPriv->subpic_clear_flag = 0;
        ximg->data = NULL;
        wsbmBOUnmap(bo);
        if (NULL != ximg)
            XDestroyImage(ximg);
    }
    return VA_STATUS_SUCCESS;
}

static VAStatus psb_repaint_colorkey(
    VADriverContextP ctx,
    Drawable draw, /* X Drawable */
    VASurfaceID surface,
    int x11_window_width,
    int x11_window_height
)
{
    INIT_DRIVER_DATA;
    INIT_OUTPUT_PRIV;
    int i, ret;
    psb_x11_clip_list_t *pClipNext = NULL;
    VARectangle *pVaWindowClipRects = NULL;
    object_surface_p obj_surface = SURFACE(surface);
    PsbPortPrivRec *pPriv = (PsbPortPrivPtr)(&driver_data->coverlay_priv);

    if (output->frame_count % 500 == 0 || driver_data->xrandr_update) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Repaint color key.\n");
        if (output->pClipBoxList)
            psb_x11_freeWindowClipBoxList(output->pClipBoxList);
        /* get window clipbox */
        ret = psb_x11_createWindowClipBoxList(ctx->native_dpy, draw, &output->pClipBoxList, &output->ui32NumClipBoxList);
        if (ret != 0) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: get window clip boxes error # %d\n", __func__, ret);
            return VA_STATUS_ERROR_UNKNOWN;
        }
        if (output->frame_count == 500)
            output->frame_count = 0;

        driver_data->xrandr_update = 0;
    }

    pVaWindowClipRects = (VARectangle *)calloc(1, sizeof(VARectangle) * output->ui32NumClipBoxList);
    if (!pVaWindowClipRects) {
        psb_x11_freeWindowClipBoxList(output->pClipBoxList);
        return VA_STATUS_ERROR_ALLOCATION_FAILED;
    }

    memset(pVaWindowClipRects, 0, sizeof(VARectangle)*output->ui32NumClipBoxList);
    pClipNext = output->pClipBoxList;
#ifdef CLIP_DEBUG
    drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: Total %d clip boxes\n", __func__, output->ui32NumClipBoxList);
#endif
    for (i = 0; i < output->ui32NumClipBoxList; i++) {
        pVaWindowClipRects[i].x      = pClipNext->rect.i32Left;
        pVaWindowClipRects[i].y      = pClipNext->rect.i32Top;
        pVaWindowClipRects[i].width  = pClipNext->rect.i32Right - pClipNext->rect.i32Left;
        pVaWindowClipRects[i].height = pClipNext->rect.i32Bottom - pClipNext->rect.i32Top;
#ifdef CLIP_DEBUG
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: clip boxes Left Top (%d, %d) Right Bottom (%d, %d) width %d height %d\n", __func__,
                           pClipNext->rect.i32Left, pClipNext->rect.i32Top,
                           pClipNext->rect.i32Right, pClipNext->rect.i32Bottom,
                           pVaWindowClipRects[i].width, pVaWindowClipRects[i].height);
#endif
        pClipNext = pClipNext->next;
    }

    /* repaint the color key when window size changed*/
    if (!obj_surface->subpictures &&
        ((pPriv->x11_window_width != x11_window_width) ||
         (pPriv->x11_window_height != x11_window_height))) {
        pPriv->x11_window_width = x11_window_width;
        pPriv->x11_window_height = x11_window_height;
        XSetForeground((Display *)ctx->native_dpy, output->gc, pPriv->colorKey);
        XFillRectangle((Display *)ctx->native_dpy, draw, output->gc, 0, 0, x11_window_width, x11_window_height);
        XSync((Display *)ctx->native_dpy, False);
    }


    if ((!obj_surface->subpictures) &&
        ((output->ui32NumClipBoxList != pPriv->last_num_clipbox) ||
         (memcmp(&pVaWindowClipRects[0], &(pPriv->last_clipbox[0]), (output->ui32NumClipBoxList > 16 ? 16 : output->ui32NumClipBoxList)*sizeof(VARectangle)) != 0))) {
        pPriv->last_num_clipbox = output->ui32NumClipBoxList;
        memcpy(&pPriv->last_clipbox[0], &pVaWindowClipRects[0], (output->ui32NumClipBoxList > 16 ? 16 : output->ui32NumClipBoxList)*sizeof(VARectangle));
        XFillRectangle((Display *)ctx->native_dpy, draw, output->gc, 0, 0, x11_window_width, x11_window_height);
        XSync((Display *)ctx->native_dpy, False);
    }

    free(pVaWindowClipRects);

    return VA_STATUS_SUCCESS;
}

static VAStatus psb_extendMode_getCoordinate(
    PsbPortPrivPtr pPriv,
    psb_xrandr_location extend_location,
    short destx,
    short desty,
    short srcx,
    short srcy,
    float xScaleFactor,
    float yScaleFactor,
    int *x11_window_width,
    int *x11_window_height,
    psb_overlay_rect_p local_rect,
    psb_overlay_rect_p extend_rect,
    enum overlay_id_t *extend_overlay
)
{
    switch (extend_location) {
    case LEFT_OF:
        if ((destx + *x11_window_width) > (pPriv->display_width + pPriv->extend_display_width)) {
            *x11_window_width = pPriv->display_width + pPriv->extend_display_width - destx;
        }
        if (((desty + *x11_window_height) < pPriv->display_height) &&
            ((desty + *x11_window_height) < pPriv->extend_display_height))
            local_rect->dHeight = extend_rect->dHeight = *x11_window_height;
        else if (pPriv->display_height < pPriv->extend_display_height) {
            local_rect->dHeight = pPriv->display_height - desty;
            if ((desty + *x11_window_height) > pPriv->extend_display_height)
                extend_rect->dHeight = *x11_window_height = pPriv->extend_display_height - desty;
            else
                extend_rect->dHeight = *x11_window_height;
        } else {
            extend_rect->dHeight = pPriv->extend_display_height - desty;
            if ((desty + *x11_window_height) > pPriv->display_height)
                local_rect->dHeight = *x11_window_height = pPriv->display_height - desty;
            else
                local_rect->dHeight = *x11_window_height;
        }

        if ((destx < pPriv->extend_display_width) && ((destx + *x11_window_width) < pPriv->extend_display_width)) {
            local_rect->dWidth = 0;
            extend_rect->dWidth = *x11_window_width;
            *extend_overlay = OVERLAY_A;
            local_rect->destx = 0;
        } else if ((destx < pPriv->extend_display_width) && ((destx + *x11_window_width) >= pPriv->extend_display_width)) {
            extend_rect->dWidth = pPriv->extend_display_width - destx;
            local_rect->dWidth = *x11_window_width - extend_rect->dWidth;
            local_rect->destx = 0;
        } else {
            local_rect->dWidth = *x11_window_width;
            extend_rect->dWidth = 0;
            local_rect->destx = destx - pPriv->extend_display_width;
        }
        local_rect->sWidth = (unsigned short)(local_rect->dWidth * xScaleFactor);
        local_rect->sHeight = (unsigned short)(local_rect->dHeight * yScaleFactor);
        extend_rect->sWidth = (unsigned short)(extend_rect->dWidth * xScaleFactor);
        extend_rect->sHeight = (unsigned short)(extend_rect->dHeight * yScaleFactor);

        local_rect->srcx = srcx + extend_rect->sWidth;
        extend_rect->srcx = srcx;
        local_rect->srcy = extend_rect->srcy = srcy;

        extend_rect->destx = destx;
        local_rect->desty = extend_rect->desty = desty;
        break;
    case RIGHT_OF:
        if ((destx + *x11_window_width) > (pPriv->display_width + pPriv->extend_display_width)) {
            *x11_window_width = pPriv->display_width + pPriv->extend_display_width - destx;
        }
        if (((desty + *x11_window_height) < pPriv->display_height) &&
            ((desty + *x11_window_height) < pPriv->extend_display_height))
            local_rect->dHeight = extend_rect->dHeight = *x11_window_height;
        else if (pPriv->display_height < pPriv->extend_display_height) {
            local_rect->dHeight = pPriv->display_height - desty;
            if ((desty + *x11_window_height) > pPriv->extend_display_height)
                extend_rect->dHeight = *x11_window_height = pPriv->extend_display_height - desty;
            else
                extend_rect->dHeight = *x11_window_height;
        } else {
            extend_rect->dHeight = pPriv->extend_display_height - desty;
            if ((desty + *x11_window_height) > pPriv->display_height)
                local_rect->dHeight = *x11_window_height = pPriv->display_height - desty;
            else
                local_rect->dHeight = *x11_window_height;
        }

        if ((destx < pPriv->display_width) && ((destx + *x11_window_width) < pPriv->display_width)) {
            local_rect->dWidth = *x11_window_width;
            extend_rect->dWidth = 0;
            extend_rect->destx = 0;
        } else if ((destx < pPriv->display_width) && ((destx + *x11_window_width) >= pPriv->display_width)) {
            local_rect->dWidth = pPriv->display_width - destx;
            extend_rect->dWidth = *x11_window_width - local_rect->dWidth;
            extend_rect->destx = 0;
        } else {
            local_rect->dWidth = 0;
            extend_rect->dWidth = *x11_window_width;
            *extend_overlay = OVERLAY_A;
            extend_rect->destx = destx - pPriv->display_width;
        }
        local_rect->sWidth = (unsigned short)(local_rect->dWidth * xScaleFactor);
        local_rect->sHeight = (unsigned short)(local_rect->dHeight * yScaleFactor);
        extend_rect->sWidth = (unsigned short)(extend_rect->dWidth * xScaleFactor);
        extend_rect->sHeight = (unsigned short)(extend_rect->dHeight * yScaleFactor);

        local_rect->srcx = srcx;
        extend_rect->srcx = srcx + local_rect->sWidth;
        local_rect->srcy = extend_rect->srcy = srcy;

        local_rect->destx = destx;
        local_rect->desty = extend_rect->desty = desty;
        break;
    case ABOVE:
        if (((destx + *x11_window_width) < pPriv->display_width) &&
            ((destx + *x11_window_width) < pPriv->extend_display_width))
            local_rect->dWidth = extend_rect->dWidth = *x11_window_width;
        else if (pPriv->display_width < pPriv->extend_display_width) {
            local_rect->dWidth = pPriv->display_width - destx;
            if ((destx + *x11_window_width) > pPriv->extend_display_width)
                extend_rect->dWidth = *x11_window_width = pPriv->extend_display_width - destx;
            else
                extend_rect->dWidth = *x11_window_width;
        } else {
            extend_rect->dWidth = pPriv->extend_display_width - destx;
            if ((destx + *x11_window_width) > pPriv->display_width)
                local_rect->dWidth = *x11_window_width = pPriv->display_width - destx;
            else
                local_rect->dWidth = *x11_window_width;
        }

        if ((desty + *x11_window_height) > (pPriv->display_height + pPriv->extend_display_height)) {
            *x11_window_height = pPriv->display_height + pPriv->extend_display_height - desty;
        }

        if ((desty < pPriv->extend_display_height) && ((desty + *x11_window_height) < pPriv->extend_display_height)) {
            local_rect->dHeight = 0;
            extend_rect->dHeight = *x11_window_height;
            *extend_overlay = OVERLAY_A;
            local_rect->desty = 0;
        } else if ((desty < pPriv->extend_display_height) && ((desty + *x11_window_height) >= pPriv->extend_display_height)) {
            extend_rect->dHeight = pPriv->extend_display_height - desty;
            local_rect->dHeight = *x11_window_height - extend_rect->dHeight;
            local_rect->desty = 0;
        } else {
            local_rect->dHeight = *x11_window_height;
            extend_rect->dHeight = 0;
            local_rect->desty = desty - pPriv->extend_display_height;
        }
        local_rect->sWidth = (unsigned short)(local_rect->dWidth * xScaleFactor);
        local_rect->sHeight = (unsigned short)(local_rect->dHeight * yScaleFactor);
        extend_rect->sWidth = (unsigned short)(extend_rect->dWidth * xScaleFactor);
        extend_rect->sHeight = (unsigned short)(extend_rect->dHeight * yScaleFactor);

        local_rect->srcy = srcy + extend_rect->sHeight;
        extend_rect->srcy = srcy;
        local_rect->srcx = extend_rect->srcx = srcx;

        extend_rect->desty = desty;
        local_rect->destx = extend_rect->destx = destx;
        break;
    case BELOW:
        if (((destx + *x11_window_width) < pPriv->display_width) &&
            ((destx + *x11_window_width) < pPriv->extend_display_width))
            local_rect->dWidth = extend_rect->dWidth = *x11_window_width;
        else if (pPriv->display_width < pPriv->extend_display_width) {
            local_rect->dWidth = pPriv->display_width - destx;
            if ((destx + *x11_window_width) > pPriv->extend_display_width)
                extend_rect->dWidth = *x11_window_width = pPriv->extend_display_width - destx;
            else
                extend_rect->dWidth = *x11_window_width;
        } else {
            extend_rect->dWidth = pPriv->extend_display_width - destx;
            if ((destx + *x11_window_width) > pPriv->display_width)
                local_rect->dWidth = *x11_window_width = pPriv->display_width - destx;
            else
                local_rect->dWidth = *x11_window_width;
        }

        if ((desty + *x11_window_height) > (pPriv->display_height + pPriv->extend_display_height)) {
            *x11_window_height = pPriv->display_height + pPriv->extend_display_height - desty;
        }

        if ((desty < pPriv->display_height) && ((desty + *x11_window_height) < pPriv->display_height)) {
            local_rect->dHeight = *x11_window_height;
            extend_rect->dHeight = 0;
            extend_rect->desty = 0;
        } else if ((desty < pPriv->display_height) && ((desty + *x11_window_height) >= pPriv->display_height)) {
            local_rect->dHeight = pPriv->display_height - desty;
            extend_rect->dHeight = *x11_window_height - local_rect->dHeight;
            extend_rect->desty = 0;
        } else {
            local_rect->dHeight = 0;
            extend_rect->dHeight = *x11_window_height;
            *extend_overlay = OVERLAY_A;
            extend_rect->desty = desty - pPriv->display_height;
        }
        local_rect->sWidth = (unsigned short)(local_rect->dWidth * xScaleFactor);
        local_rect->sHeight = (unsigned short)(local_rect->dHeight * yScaleFactor);
        extend_rect->sWidth = (unsigned short)(extend_rect->dWidth * xScaleFactor);
        extend_rect->sHeight = (unsigned short)(extend_rect->dHeight * yScaleFactor);

        local_rect->srcy = srcy;
        extend_rect->srcy = srcy + local_rect->sHeight;
        local_rect->srcx = extend_rect->srcx = srcx;

        local_rect->desty = desty;
        local_rect->destx = extend_rect->destx = destx;
        break;
    case NORMAL:
    default:
        break;
    }
    return VA_STATUS_SUCCESS;
}

static void psb_init_subpicture(VADriverContextP ctx, PsbPortPrivPtr pPriv)
{
    INIT_DRIVER_DATA;
    struct drm_psb_register_rw_arg regs;
    unsigned int subpicture_enable_mask = REGRWBITS_DSPACNTR;

    if (!pPriv->subpicture_enabled) {
        if (psb_xrandr_hdmi_enabled())
            subpicture_enable_mask |= REGRWBITS_DSPBCNTR;
        if (psb_xrandr_mipi1_enabled())
            subpicture_enable_mask |= REGRWBITS_DSPCCNTR;

        memset(&regs, 0, sizeof(regs));
        regs.subpicture_enable_mask = subpicture_enable_mask;
        pPriv->subpicture_enable_mask = subpicture_enable_mask;
        pPriv->subpicture_enabled = 1;
        drmCommandWriteRead(driver_data->drm_fd, DRM_PSB_REGISTER_RW, &regs, sizeof(regs));
    }
}

static void psb_clear_subpictures(
    VADriverContextP ctx,
    PsbPortPrivPtr pPriv,
    int win_width,
    int win_height,
    object_surface_p obj_surface
)
{
    INIT_OUTPUT_PRIV;
    PsbVASurfaceRec *subpicture = (PsbVASurfaceRec *)obj_surface->subpictures;
    int i;

    if (subpicture == NULL) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Surface has no subpicture to render.\n");
        return;
    }

    for (i = 0; subpicture != NULL; subpicture = subpicture->next, i++) {
        if ((subpicture->subpic_dstx != pPriv->clear_key[i].subpic_dstx) ||
            (subpicture->subpic_dsty != pPriv->clear_key[i].subpic_dsty) ||
            (subpicture->subpic_dstw != pPriv->clear_key[i].subpic_dstw) ||
            (subpicture->subpic_dsth != pPriv->clear_key[i].subpic_dsth)) {
            XSetForeground((Display *)ctx->native_dpy, output->gc, 0);
            XFillRectangle((Display *)ctx->native_dpy, output->output_drawable, output->gc, 0, 0, win_width, win_height);
            XSync((Display *)ctx->native_dpy, False);
            if (psb_xrandr_extvideo_mode()) {
                XSetForeground((Display *)ctx->native_dpy, output->extend_gc, 0);
                XFillRectangle((Display *)ctx->native_dpy, output->extend_drawable, output->extend_gc,
                               0, 0, pPriv->extend_display_width, pPriv->extend_display_height);
                XSync((Display *)ctx->native_dpy, False);
            }
            pPriv->subpic_clear_flag = 1;
        }
    }
    return;
}

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
)
{
    INIT_DRIVER_DATA;
    INIT_OUTPUT_PRIV;
    int ret;
    int x11_window_width = destw, x11_window_height = desth;
    psb_xrandr_location extend_location;
    object_surface_p obj_surface = SURFACE(surface);
    PsbPortPrivRec *pPriv = (PsbPortPrivPtr)(&driver_data->coverlay_priv);
    int primary_crtc_x, primary_crtc_y, extend_crtc_x, extend_crtc_y;
    enum pipe_id_t local_pipe = PIPEA, extend_pipe = PIPEB;
    int surfacex = destx, surfacey = desty;
    float xScaleFactor, yScaleFactor;
    Rotation rotation = RR_Rotate_0;
    psb_output_device local_device, extend_device;
    psb_extvideo_subtitle subtitle;

    if (flags & VA_CLEAR_DRAWABLE) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Clean draw with color 0x%08x\n", driver_data->clear_color);
        psb_cleardrawable_stopoverlay(ctx, draw, destx, desty, destw, desth);

        return VA_STATUS_SUCCESS;
    }

    if (output->frame_count % 500 == 0 || driver_data->xrandr_update) {
        /* get window screen coordination */
        ret = psb_x11_getWindowCoordinate(ctx->native_dpy, draw, &output->winRect, &output->bIsVisible);
        if (ret != 0) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: Failed to get X11 window coordinates error # %d\n", __func__, ret);
            return VA_STATUS_ERROR_UNKNOWN;
        }
    }

    if (!output->bIsVisible) {
        return VA_STATUS_SUCCESS;
    }

    if (NULL == obj_surface) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: Invalid surface id 0x%08x.\n", __func__, surface);
        return VA_STATUS_ERROR_INVALID_SURFACE;
    }

    if (output->output_drawable != draw) {
        output->output_drawable = draw;
    }

    if (!output->gc) {
        output->gc = XCreateGC((Display *)ctx->native_dpy, draw, 0, NULL);
        /* paint the color key */
        if (!obj_surface->subpictures && !driver_data->overlay_auto_paint_color_key) {
            XSetForeground((Display *)ctx->native_dpy, output->gc, pPriv->colorKey);
            XFillRectangle((Display *)ctx->native_dpy, draw, output->gc, 0, 0, x11_window_width, x11_window_height);
            XSync((Display *)ctx->native_dpy, False);
        }
    }

    if (driver_data->use_xrandr_thread && !driver_data->xrandr_thread_id) {
        ret = psb_xrandr_thread_create(ctx);
        if (ret != 0) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: Failed to create psb xrandr thread error # %d\n", __func__, ret);
            return VA_STATUS_ERROR_UNKNOWN;
        }
    }

    ret = psb_xrandr_local_crtc_coordinate(&local_device, &primary_crtc_x, &primary_crtc_y, &pPriv->display_width, &pPriv->display_height, &rotation);
    if (ret != VA_STATUS_SUCCESS) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: Failed to get primary crtc coordinates error # %d\n", __func__, ret);
        return VA_STATUS_ERROR_UNKNOWN;
    }
    switch (local_device) {
    case LVDS0:
    case MIPI0:
        local_pipe = PIPEA;
        break;
        /* single HDMI */
    case HDMI:
        local_pipe = PIPEB;
        break;
    case MIPI1:
        local_pipe = PIPEC;
        break;
    }

    if (!psb_xrandr_single_mode()) {

        ret = psb_xrandr_extend_crtc_coordinate(&extend_device, &extend_crtc_x, &extend_crtc_y,
                                                &pPriv->extend_display_width, &pPriv->extend_display_height, &extend_location, &rotation);
        if (ret != VA_STATUS_SUCCESS) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: Failed to get extend crtc coordinates error # %d\n", __func__, ret);
            return VA_STATUS_ERROR_UNKNOWN;
        }

        switch (extend_device) {
        case HDMI:
            extend_pipe = PIPEB;
            break;
        case MIPI1:
            extend_pipe = PIPEC;
            break;
        default:
            drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: Failed to get extend pipe\n", __func__);
            break;
        }
    }

    /*clip in the window area*/
    if (destx < 0) {
        x11_window_width += destx;
        destx = 0;
    }

    if (desty < 0) {
        x11_window_height += desty;
        desty = 0;
    }

    if (srcx < 0) {
        srcw += srcx;
        srcx = 0;
    }

    if (srcy < 0) {
        srch += srcy;
        srcy = 0;
    }

    if ((destx + x11_window_width) > output->winRect.ui32Width)
        x11_window_width = output->winRect.ui32Width - destx;

    if ((desty + x11_window_height) > output->winRect.ui32Height)
        x11_window_height = output->winRect.ui32Height - desty;

    /*translate destx, desty into screen coordinate*/
    destx += output->winRect.i32Left;
    desty += output->winRect.i32Top;

    /*clip in the screen area*/
    xScaleFactor = srcw * 1.0 / x11_window_width;
    yScaleFactor = srch * 1.0 / x11_window_height;

    if (destx < 0) {
        x11_window_width += destx;
        srcx = (short)((-destx) * xScaleFactor);
        destx = 0;
    }

    if (desty < 0) {
        x11_window_height += desty;
        srcy = (short)((-desty) * yScaleFactor);
        desty = 0;
    }

    /* display by overlay */
    if (psb_xrandr_single_mode() || IS_MRST(driver_data)) {
        if ((destx + x11_window_width) > pPriv->display_width) {
            x11_window_width = pPriv->display_width - destx;
            srcw = (unsigned short)(x11_window_width * xScaleFactor);
        }

        if ((desty + x11_window_height) > pPriv->display_height) {
            x11_window_height = pPriv->display_height - desty;
            srch = (unsigned short)(x11_window_height * yScaleFactor);
        }

        if (!driver_data->overlay_auto_paint_color_key) {
            ret = psb_repaint_colorkey(ctx, draw, surface, x11_window_width, x11_window_height);
            if (ret != 0) {
                drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: Failed to repaint color key error # %d\n", __func__, ret);
                return VA_STATUS_ERROR_UNKNOWN;
            }
        }

        psb_putsurface_overlay(
            ctx, surface, srcx, srcy, srcw, srch,
            /* screen coordinate */
            destx, desty, x11_window_width, x11_window_height,
            flags, OVERLAY_A, local_pipe);
    } else if (psb_xrandr_clone_mode()) {
        psb_overlay_rect_t local_rect, extend_rect;

        if (output->extend_drawable) {
            XDestroyWindow(ctx->native_dpy, output->extend_drawable);
            output->extend_drawable = 0;
            XFreeGC((Display *)ctx->native_dpy, output->extend_gc);
            output->extend_gc = 0;
        }

        if (((destx + x11_window_width) < pPriv->display_width) &&
            ((destx + x11_window_width) < pPriv->extend_display_width))
            local_rect.dWidth = extend_rect.dWidth = x11_window_width;
        else if (pPriv->display_width < pPriv->extend_display_width) {
            local_rect.dWidth = pPriv->display_width - destx;
            if ((destx + x11_window_width) > pPriv->extend_display_width)
                extend_rect.dWidth = x11_window_width = pPriv->extend_display_width - destx;
            else
                extend_rect.dWidth = x11_window_width;
        } else {
            extend_rect.dWidth = pPriv->extend_display_width - destx;
            if ((destx + x11_window_width) > pPriv->display_width)
                local_rect.dWidth = x11_window_width = pPriv->display_width - destx;
            else
                local_rect.dWidth = x11_window_width;
        }

        if (((desty + x11_window_height) < pPriv->display_height) &&
            ((desty + x11_window_height) < pPriv->extend_display_height))
            local_rect.dHeight = extend_rect.dHeight = x11_window_height;
        else if (pPriv->display_height < pPriv->extend_display_height) {
            local_rect.dHeight = pPriv->display_height - desty;
            if ((desty + x11_window_height) > pPriv->extend_display_height)
                extend_rect.dHeight = x11_window_height = pPriv->extend_display_height - desty;
            else
                extend_rect.dHeight = x11_window_height;
        } else {
            extend_rect.dHeight = pPriv->extend_display_height - desty;
            if ((desty + x11_window_height) > pPriv->display_height)
                local_rect.dHeight = x11_window_height = pPriv->display_height - desty;
            else
                local_rect.dHeight = x11_window_height;
        }
        if ((driver_data->mipi0_rotation != VA_ROTATION_NONE) ||
            (driver_data->hdmi_rotation != VA_ROTATION_NONE)) {
            local_rect.sWidth = srcw;
            local_rect.sHeight = srch;
            extend_rect.sWidth = srcw;
            extend_rect.sHeight = srch;
        } else {
            local_rect.sWidth = (unsigned short)(local_rect.dWidth * xScaleFactor);
            local_rect.sHeight = (unsigned short)(local_rect.dHeight * yScaleFactor);
            extend_rect.sWidth = (unsigned short)(extend_rect.dWidth * xScaleFactor);
            extend_rect.sHeight = (unsigned short)(extend_rect.dHeight * yScaleFactor);
        }
        ret = psb_repaint_colorkey(ctx, draw, surface, x11_window_width, x11_window_height);
        if (ret != 0) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: Failed to repaint color key error # %d\n", __func__, ret);
            return VA_STATUS_ERROR_UNKNOWN;
        }
        psb_putsurface_overlay(
            ctx, surface, srcx, srcy, extend_rect.sWidth, extend_rect.sHeight,
            /* screen coordinate */
            destx, desty, extend_rect.dWidth, extend_rect.dHeight,
            flags, OVERLAY_C, extend_pipe);
        psb_putsurface_overlay(
            ctx, surface,  srcx, srcy, local_rect.sWidth, local_rect.sHeight,
            /* screen coordinate */
            destx, desty, local_rect.dWidth, local_rect.dHeight,
            flags, OVERLAY_A, local_pipe);
    } else if (psb_xrandr_extend_mode()) {
        if (driver_data->extend_fullscreen) {
            switch (extend_location) {
            case RIGHT_OF:
                XMoveResizeWindow(ctx->native_dpy, output->output_drawable, pPriv->display_width, 0, pPriv->extend_display_width, pPriv->extend_display_height);
                break;
            case BELOW:
                XMoveResizeWindow(ctx->native_dpy, output->output_drawable, 0, pPriv->display_height, pPriv->extend_display_width, pPriv->extend_display_height);
                break;
            case LEFT_OF:
            case ABOVE:
                XMoveResizeWindow(ctx->native_dpy, output->output_drawable, 0, 0, pPriv->extend_display_width, pPriv->extend_display_height);
                break;
            default:
                break;

            }
            XSetForeground((Display *)ctx->native_dpy, output->gc, pPriv->colorKey);
            XFillRectangle((Display *)ctx->native_dpy, draw, output->gc, 0, 0, pPriv->extend_display_width, pPriv->extend_display_height);
            XFlush(ctx->native_dpy);

            psb_putsurface_overlay(
                ctx, surface, srcx, srcy, srcw, srch,
                /* screen coordinate */
                0, 0, pPriv->extend_display_width, pPriv->extend_display_height,
                flags, OVERLAY_A, PIPEB);
        } else {
            psb_overlay_rect_t local_rect, extend_rect;
            enum overlay_id_t extend_overlay = OVERLAY_C;

            if (output->extend_drawable) {
                XDestroyWindow(ctx->native_dpy, output->extend_drawable);
                output->extend_drawable = 0;
                XFreeGC((Display *)ctx->native_dpy, output->extend_gc);
                output->extend_gc = 0;
            }
            memset(&local_rect, 0, sizeof(psb_overlay_rect_t));
            memset(&extend_rect, 0, sizeof(psb_overlay_rect_t));
            psb_extendMode_getCoordinate(pPriv, extend_location, destx, desty, srcx, srcy,
                                         xScaleFactor, yScaleFactor, &x11_window_width, &x11_window_height,
                                         &local_rect, &extend_rect, &extend_overlay);

            ret = psb_repaint_colorkey(ctx, draw, surface, x11_window_width, x11_window_height);
            if (ret != 0) {
                drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: Failed to repaint color key error # %d\n", __func__, ret);
                return VA_STATUS_ERROR_UNKNOWN;
            }

            if ((extend_rect.dWidth > 0) && (extend_rect.dHeight > 0)) {
                psb_putsurface_overlay(
                    ctx, surface,
                    extend_rect.srcx, extend_rect.srcy, extend_rect.sWidth, extend_rect.sHeight,
                    extend_rect.destx, extend_rect.desty, extend_rect.dWidth, extend_rect.dHeight,
                    flags, extend_overlay, extend_pipe);
            }
            if ((local_rect.dWidth > 0) && (local_rect.dHeight > 0)) {
                psb_putsurface_overlay(
                    ctx, surface,
                    local_rect.srcx, local_rect.srcy, local_rect.sWidth, local_rect.sHeight,
                    local_rect.destx, local_rect.desty, local_rect.dWidth, local_rect.dHeight,
                    flags, OVERLAY_A, local_pipe);
            }
        }
    } else if (psb_xrandr_extvideo_mode()) {
        unsigned int xres, yres, xoffset, yoffset, overscanmode, pannelfitting, x, y;
        psb_extvideo_center center;

        psb_xrandr_extvideo_prop(&xres, &yres, &xoffset, &yoffset, &center, &subtitle, &overscanmode, &pannelfitting);
        x = xoffset;
        y = yoffset;

        switch (extend_location) {
        case RIGHT_OF:
            x += pPriv->display_width;
            break;
        case BELOW:
            y += pPriv->display_height;
            break;
        case NORMAL:
            break;
        case LEFT_OF:
            if (driver_data->xrandr_dirty & PSB_NEW_EXTVIDEO) {
                destx += pPriv->extend_display_width;
                XMoveResizeWindow(ctx->native_dpy, output->output_drawable, destx, desty, x11_window_width, x11_window_height);
                XFillRectangle((Display *)ctx->native_dpy, draw, output->gc, 0, 0, x11_window_width, x11_window_height);
                XFlush(ctx->native_dpy);
            }
            destx = destx - pPriv->extend_display_width;
            break;
        case ABOVE:
            if (driver_data->xrandr_dirty & PSB_NEW_EXTVIDEO) {
                desty += pPriv->extend_display_height;
                XMoveResizeWindow(ctx->native_dpy, output->output_drawable, destx, desty, x11_window_width, x11_window_height);
                XFillRectangle((Display *)ctx->native_dpy, draw, output->gc, 0, 0, x11_window_width, x11_window_height);
                XFlush(ctx->native_dpy);
            }
            desty = desty - pPriv->extend_display_height;
            break;
        }
        if ((destx + x11_window_width) > pPriv->display_width)
            x11_window_width = pPriv->display_width - destx;
        if ((desty + x11_window_height) > pPriv->display_height)
            x11_window_height = pPriv->display_height - desty;

        if (driver_data->xrandr_dirty & PSB_NEW_EXTVIDEO) {
            Window extend_win;
            extend_win = psb_xrandr_create_full_screen_window(x, y, xres, yres);
            if (output->extend_drawable != extend_win) {
                output->extend_drawable = extend_win;
                if (output->extend_gc)
                    XFreeGC((Display *)ctx->native_dpy, output->extend_gc);
                output->extend_gc = XCreateGC((Display *)ctx->native_dpy, extend_win, 0, NULL);

                /* paint the color key */
                if (!obj_surface->subpictures) {
                    XSetForeground((Display *)ctx->native_dpy, output->extend_gc, pPriv->colorKey);
                    XFillRectangle((Display *)ctx->native_dpy, extend_win, output->extend_gc, 0, 0, xres, yres);
                    XSync((Display *)ctx->native_dpy, False);
                }
            }
            driver_data->xrandr_dirty &= ~PSB_NEW_EXTVIDEO;
        }

        ret = psb_repaint_colorkey(ctx, draw, surface, x11_window_width, x11_window_height);
        if (ret != 0) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: Failed to repaint color key error # %d\n", __func__, ret);
            return VA_STATUS_ERROR_UNKNOWN;
        }

        psb_putsurface_overlay(
            ctx, surface, srcx, srcy, srcw, srch,
            /* screen coordinate */
            xoffset, yoffset, xres, yres,
            flags, OVERLAY_C, PIPEB);
        psb_putsurface_overlay(
            ctx, surface, srcx, srcy, srcw, srch,
            /* screen coordinate */
            destx, desty,
            x11_window_width, x11_window_height,
            flags, OVERLAY_A, local_pipe);
    }

    /*Init Overlay subpicuture blending and make proper clear.*/
    if (pPriv->is_mfld && obj_surface->subpictures) {
        PsbVASurfaceRec *subpicture = (PsbVASurfaceRec *)obj_surface->subpictures;

        psb_init_subpicture(ctx, pPriv);
        /*clear changed subpicture zones in drawable.*/
        psb_clear_subpictures(ctx, pPriv, x11_window_width, x11_window_height, obj_surface);
        if (pPriv->subpic_clear_flag) {
            psb_DisplayRGBASubpicture(subpicture, ctx, x11_window_width, x11_window_height,
                                      surfacex, surfacey, obj_surface->width, obj_surface->height, subtitle);
        }
    }

    output->frame_count++;

    return VA_STATUS_SUCCESS;
}
