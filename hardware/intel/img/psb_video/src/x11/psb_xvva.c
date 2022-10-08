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


#include <va/va_backend.h>
#include "psb_surface.h"
#include "psb_output.h"
#include "psb_surface_ext.h"
#include "psb_x11.h"
#include "psb_xrandr.h"
#include "psb_drv_debug.h"

#include <X11/extensions/dpms.h>

#include <wsbm/wsbm_manager.h>

#define INIT_DRIVER_DATA    psb_driver_data_p driver_data = (psb_driver_data_p) ctx->pDriverData;
#define INIT_OUTPUT_PRIV    psb_x11_output_p output = (psb_x11_output_p)(((psb_driver_data_p)ctx->pDriverData)->ws_priv)

#define SURFACE(id)     ((object_surface_p) object_heap_lookup( &driver_data->surface_heap, id ))

static int psb_CheckDrawable(VADriverContextP ctx, Drawable draw);

int (*oldHandler)(Display *, XErrorEvent *) = 0;
static int XErrorFlag = 1;
static int psb_XErrorHandler(Display *dpy, XErrorEvent *event)
{
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "XErrorEvent caught in psb_XErrorHandler in psb_xvva.c\n");
    if (event->type == 0 && event->request_code == 132 && event->error_code == 11 /* BadAlloc */) {
        XErrorFlag = 1;
        return 0;
    }
    return oldHandler(dpy, event);
}

static int GetPortId(VADriverContextP ctx, psb_x11_output_p output)
{
    int i, j, k;
    unsigned int numAdapt;
    int numImages;
    XvImageFormatValues *formats;
    XvAdaptorInfo *info;
    int ret, grab_ret;
    Display *dpy = (Display *)ctx->native_dpy;

    ret = XvQueryAdaptors(dpy, DefaultRootWindow(dpy), &numAdapt, &info);
    /*Force overlay port num equal to one. OverlayC can't be used independently now.*/
    /* check for numAdapt before modifying the info[1]. Without this check
     * it will cause a memory corruption leading to segfaults */
    if (numAdapt > 1)
        info[1].num_ports = 1;

    if (Success != ret) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Can't find Xvideo adaptor\n");
        return -1;
    }

    grab_ret = XGrabServer(ctx->native_dpy);
    for (i = 0; i < numAdapt; i++) {
        if ((info[i].type & XvImageMask) == 0)
            continue;

        formats = XvListImageFormats(dpy, info[i].base_id, &numImages);
        for (j = 0; j < numImages; j++) {
            if (formats[j].id != FOURCC_XVVA) continue;
            for (k = 0; k < info[i].num_ports; k++) {
                int ret = XvGrabPort(dpy, info[i].base_id + k, CurrentTime);

                if (Success == ret) {
                    /* for textured adaptor 0 */
                    if (i == 0)
                        output->textured_portID = info[i].base_id + k;
                    /* for overlay adaptor 1 */
                    if (i == 1)
                        output->overlay_portID = info[i].base_id + k;
                    break;
                }
            }
        }
        XFree(formats);
    }

    if (grab_ret != 0)
        XUngrabServer(ctx->native_dpy);

    if ((output->textured_portID == 0) && (output->overlay_portID == 0)) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Can't detect any usable Xv XVVA port\n");
        return -1;
    }

    return 0;
}


VAStatus psb_init_xvideo(VADriverContextP ctx, psb_x11_output_p output)
{
#ifdef _FOR_FPGA_
    return VA_STATUS_SUCCESS;
#endif

    INIT_DRIVER_DATA;
    int dummy, ret;

    output->textured_portID = output->overlay_portID = 0;
    if (GetPortId(ctx, output)) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Grab Xvideo port failed, fallback to software vaPutSurface.\n");
        return VA_STATUS_ERROR_ALLOCATION_FAILED;
    }

    if (output->textured_portID)
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Detected textured Xvideo port_id = %d.\n", (unsigned int)output->textured_portID);
    if (output->overlay_portID)
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Detected overlay  Xvideo port_id = %d.\n", (unsigned int)output->overlay_portID);

    output->sprite_enabled = 0;
    if (getenv("PSB_SPRITE_ENABLE")) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "use sprite plane to playback rotated protected video\n");
        output->sprite_enabled = 1;
    }

    output->ignore_dpm = 1;
    if (getenv("PSB_VIDEO_DPMS_HACK")) {
        if (DPMSQueryExtension((Display *)ctx->native_dpy, &dummy, &dummy)
            && DPMSCapable((Display *)ctx->native_dpy)) {
            BOOL onoff;
            CARD16 state;

            DPMSInfo((Display *)ctx->native_dpy, &state, &onoff);
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "DPMS is %s, monitor state=%s\n", onoff ? "enabled" : "disabled",
                                     (state == DPMSModeOn) ? "on" : (
                                         (state == DPMSModeOff) ? "off" : (
                                             (state == DPMSModeStandby) ? "standby" : (
                                                 (state == DPMSModeSuspend) ? "suspend" : "unknow"))));
            if (onoff)
                output->ignore_dpm = 0;
        }
    }

    /* by default, overlay Xv */
    if (output->textured_portID)
        driver_data->output_method = PSB_PUTSURFACE_TEXTURE;
    if (output->overlay_portID)
        driver_data->output_method = PSB_PUTSURFACE_OVERLAY;

    ret = psb_xrandr_init(ctx);
    if (ret != 0) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: Failed to initialize psb xrandr error # %d\n", __func__, ret);
        return VA_STATUS_ERROR_UNKNOWN;
    }

    return VA_STATUS_SUCCESS;
}


VAStatus psb_deinit_xvideo(VADriverContextP ctx)
{
    INIT_DRIVER_DATA;
    INIT_OUTPUT_PRIV;

    if (output->gc) {
        XFreeGC((Display *)ctx->native_dpy, output->gc);
        output->gc = NULL;
    }

    if (output->extend_gc) {
        XFreeGC((Display *)ctx->native_dpy, output->extend_gc);
        output->extend_gc = NULL;
    }

    if (output->textured_xvimage) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Destroy XvImage for texture Xv\n");
        XFree(output->textured_xvimage);
        output->textured_xvimage = NULL;
    }

    if (output->overlay_xvimage) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Destroy XvImage for overlay  Xv\n");
        XFree(output->overlay_xvimage);
        output->textured_xvimage = NULL;
    }

    if (output->textured_portID) {
        if ((output->using_port == USING_TEXTURE_PORT) && output->output_drawable
            && (psb_CheckDrawable(ctx, output->output_drawable) == 0)) {
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "Deinit: stop textured Xvideo\n");
            XvStopVideo((Display *)ctx->native_dpy, output->textured_portID, output->output_drawable);
        }

        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Deinit: ungrab textured Xvideo port\n");
        XvUngrabPort((Display *)ctx->native_dpy, output->textured_portID, CurrentTime);
        output->textured_portID = 0;
    }

    if (output->overlay_portID) {
        if ((output->using_port == USING_OVERLAY_PORT) && output->output_drawable
            && (psb_CheckDrawable(ctx, output->output_drawable) == 0)) {
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "Deinit: stop overlay Xvideo\n");
            XvStopVideo((Display *)ctx->native_dpy, output->overlay_portID, output->output_drawable);
        }

        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Deinit: ungrab overlay Xvideo port\n");
        XvUngrabPort((Display *)ctx->native_dpy, output->overlay_portID, CurrentTime);
        output->overlay_portID = 0;
    }

    if (driver_data->use_xrandr_thread && driver_data->xrandr_thread_id) {
        psb_xrandr_thread_exit();
        pthread_join(driver_data->xrandr_thread_id, NULL);
        driver_data->xrandr_thread_id = 0;
    }
    psb_xrandr_deinit();

    output->using_port = 0;
    output->output_drawable = 0;
    output->extend_drawable = 0;
#ifndef _FOR_FPGA_
    XSync((Display *)ctx->native_dpy, False);
#endif
    return VA_STATUS_SUCCESS;
}


static void psb_surface_init(
    psb_driver_data_p driver_data,
    PsbVASurfaceRec *srf,
    int fourcc, int bpp, int w, int h, int stride, int size, unsigned int pre_add,
    struct _WsbmBufferObject *bo, int flags
)
{
    memset(srf, 0, sizeof(*srf));

    srf->fourcc = fourcc;
    srf->bo = bo;
    if (bo != NULL) {
        srf->bufid = wsbmKBufHandle(wsbmKBuf(bo));
        srf->pl_flags = wsbmBOPlacementHint(bo);
    }

/*
    if (srf->pl_flags & DRM_PSB_FLAG_MEM_CI)
        srf->reserved_phyaddr = driver_data->camera_phyaddr;
    if (srf->pl_flags & DRM_PSB_FLAG_MEM_RAR)
        srf->reserved_phyaddr = driver_data->rar_phyaddr;
*/
    srf->bytes_pp = bpp;

    srf->width = w;
    srf->pre_add = pre_add;
    if ((flags == VA_TOP_FIELD) || (flags == VA_BOTTOM_FIELD)) {
        if (driver_data->output_method ==  PSB_PUTSURFACE_FORCE_OVERLAY
            || driver_data->output_method == PSB_PUTSURFACE_OVERLAY) {
            srf->height = h;
            srf->stride = stride;
        } else {
            srf->height = h / 2;
            srf->stride = stride * 2;
        }
        if (flags == VA_BOTTOM_FIELD)
            srf->pre_add += stride;
    } else {
        srf->height = h;
        srf->stride = stride;
    }

    srf->size = size;

    if (flags == VA_CLEAR_DRAWABLE) {
        srf->clear_color = driver_data->clear_color; /* color */
        return;
    }
}

#if 0

#define WINDOW 1
#define PIXMAP 0

/* return 0 for no rotation, 1 for rotation occurs */
/* XRRGetScreenInfo has significant performance drop */
static int  psb__CheckCurrentRotation(VADriverContextP ctx)
{
    Rotation current_rotation;
    XRRScreenConfiguration *scrn_cfg;
    scrn_cfg = XRRGetScreenInfo((Display *)ctx->native_dpy, DefaultRootWindow((Display *)ctx->native_dpy));
    XRRConfigCurrentConfiguration(scrn_cfg, &current_rotation);
    XRRFreeScreenConfigInfo(scrn_cfg);
    return (current_rotation & 0x0f);
}

/* Check drawable type, 1 for window, 0 for pixmap
 * Have significant performance drop in XFCE environment
 */
static void psb__CheckDrawableType(Display *dpy, Window win, Drawable draw, int *type_ret)
{

    unsigned int child_num;
    Window root_return;
    Window parent_return;
    Window *child_return;
    int i;

    if (win == draw) {
        *type_ret = 1;
        return;
    }

    XQueryTree(dpy, win, &root_return, &parent_return, &child_return, &child_num);

    if (!child_num)
        return;

    for (i = 0; i < child_num; i++)
        psb__CheckDrawableType(dpy, child_return[i], draw, type_ret);
}
#endif


static int psb_CheckDrawable(VADriverContextP ctx, Drawable draw)
{
    INIT_DRIVER_DATA;
    INIT_OUTPUT_PRIV;
    Atom xvDrawable = XInternAtom((Display *)ctx->native_dpy, "XV_DRAWABLE", 0);
    int val = 0;

    driver_data->drawable_info = 0;
    if (output->overlay_portID) {
        XvSetPortAttribute((Display *)ctx->native_dpy, output->overlay_portID, xvDrawable, draw);
        XvGetPortAttribute((Display *)ctx->native_dpy, output->overlay_portID, xvDrawable, &val);
    } else if (output->textured_portID) {
        XvSetPortAttribute((Display *)ctx->native_dpy, output->textured_portID, xvDrawable, draw);
        XvGetPortAttribute((Display *)ctx->native_dpy, output->textured_portID, xvDrawable, &val);
    }
    driver_data->drawable_info = val;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Get xvDrawable = 0x%08x\n", val);

    if (driver_data->drawable_info == XVDRAWABLE_INVALID_DRAWABLE)
        return -1;

    return 0;
}

static int psb__CheckPutSurfaceXvPort(
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
    object_surface_p obj_surface = SURFACE(surface);
    uint32_t buf_pl;

    /* silent klockwork */
    if (obj_surface && obj_surface->psb_surface)
        buf_pl = obj_surface->psb_surface->buf.pl_flags;
    else
        return -1;

    if (flags & VA_CLEAR_DRAWABLE)
        return 0;

    if (output->overlay_portID == 0) { /* no overlay usable */
        driver_data->output_method = PSB_PUTSURFACE_TEXTURE;
        return 0;
    }

    if (driver_data->output_method == PSB_PUTSURFACE_FORCE_OVERLAY) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Force Overlay Xvideo for PutSurface\n");
        return 0;
    }

    if ((driver_data->output_method == PSB_PUTSURFACE_FORCE_TEXTURE)) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Force Textured Xvideo for PutSurface\n");
        return 0;
    }

    if (((buf_pl & (WSBM_PL_FLAG_TT)) == 0) /* buf not in TT/RAR or CI */
        || (obj_surface->width > 1920)  /* overlay have isue to support >1920xXXX resolution */
        || (obj_surface->subpic_count > 0)  /* overlay can't support subpicture */
        /*    || (flags & (VA_TOP_FIELD|VA_BOTTOM_FIELD))*/
       ) {
        driver_data->output_method = PSB_PUTSURFACE_TEXTURE;
        return 0;
    }


    /* Here should be overlay XV by defaut after overlay is stable */
    driver_data->output_method = PSB_PUTSURFACE_OVERLAY;
    /* driver_data->output_method = PSB_PUTSURFACE_TEXTURE; */

    /*
     *Query Overlay Adaptor by XvDrawable Attribute to know current
     * Xrandr information:rotation/downscaling
     * also set target drawable(window vs pixmap) into XvDrawable
     * to levage Xserver to determiate it is Pixmap or Window
     */
    /*
     *ROTATE_90: 0x2, ROTATE_180: 0x4, ROTATE_270:0x8
     *Overlay adopator can support video rotation,
     *but its performance is lower than texture video path.
     *When video protection and rotation are required (use RAR buffer),
     *only overlay adaptor will be used.
     *other attribute like down scaling and pixmap, use texture adaptor
     */
    if (driver_data->drawable_info
        & (XVDRAWABLE_ROTATE_180 | XVDRAWABLE_ROTATE_90 | XVDRAWABLE_ROTATE_270)) {
            driver_data->output_method = PSB_PUTSURFACE_TEXTURE;
    }

    if (driver_data->drawable_info & (XVDRAWABLE_PIXMAP | XVDRAWABLE_REDIRECT_WINDOW))
        driver_data->output_method = PSB_PUTSURFACE_TEXTURE;

    if (srcw >= destw * 8 || srch >= desth * 8)
        driver_data->output_method = PSB_PUTSURFACE_TEXTURE;

    return 0;
}


static int psb__CheckGCXvImage(
    VADriverContextP ctx,
    VASurfaceID surface,
    Drawable draw,
    XvImage **xvImage,
    XvPortID *port_id,
    unsigned int flags /* de-interlacing flags */
)
{
    INIT_DRIVER_DATA;
    INIT_OUTPUT_PRIV;
    object_surface_p obj_surface = SURFACE(surface); /* surface already checked */

    if (output->output_drawable != draw) {
        if (output->gc)
            XFreeGC((Display *)ctx->native_dpy, output->gc);
        output->gc = XCreateGC((Display *)ctx->native_dpy, draw, 0, NULL);
        output->output_drawable = draw;
    }

    if (flags & VA_CLEAR_DRAWABLE) {
        if (output->textured_portID && (output->using_port == USING_TEXTURE_PORT)) {
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "Clear drawable, and stop textured Xvideo\n");
            XvStopVideo((Display *)ctx->native_dpy, output->textured_portID, draw);
        }

        if (output->overlay_portID && (output->using_port == USING_OVERLAY_PORT)) {
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "Clear drawable, and stop overlay Xvideo\n");
            XvStopVideo((Display *)ctx->native_dpy, output->overlay_portID, draw);
        }

        output->using_port = 0;

        XSetForeground((Display *)ctx->native_dpy, output->gc, driver_data->clear_color);

        return 0;
    }

    if ((driver_data->output_method == PSB_PUTSURFACE_FORCE_OVERLAY) ||
        (driver_data->output_method == PSB_PUTSURFACE_OVERLAY)) {
        /* use OVERLAY XVideo */
        if (obj_surface &&
            ((output->output_width != obj_surface->width) ||
             (output->output_height != obj_surface->height) ||
             (!output->overlay_xvimage))) {

            if (output->overlay_xvimage)
                XFree(output->overlay_xvimage);

            drv_debug_msg(VIDEO_DEBUG_GENERAL, "Create new XvImage for overlay\n");
            output->overlay_xvimage = XvCreateImage((Display *)ctx->native_dpy, output->overlay_portID,
                                                    FOURCC_XVVA, 0,
                                                    obj_surface->width, obj_surface->height);

            output->overlay_xvimage->data = (char *) & output->imgdata_vasrf;
            output->output_width = obj_surface->width;
            output->output_height = obj_surface->height;
        }
        *xvImage = output->overlay_xvimage;
        *port_id = output->overlay_portID;

        if ((output->textured_portID) && (output->using_port == USING_TEXTURE_PORT)) { /* stop texture port */
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "Using overlay xvideo, stop textured xvideo\n");
            XvStopVideo((Display *)ctx->native_dpy, output->textured_portID, draw);
            XSync((Display *)ctx->native_dpy, False);
        }
        output->using_port = USING_OVERLAY_PORT;

        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Using Overlay Xvideo (%d) for PutSurface\n", output->textured_portID);

        return 0;
    }

    if ((driver_data->output_method == PSB_PUTSURFACE_FORCE_TEXTURE) ||
        (driver_data->output_method == PSB_PUTSURFACE_TEXTURE)) {
        /* use Textured XVideo */
        if (obj_surface &&
            ((output->output_width != obj_surface->width) ||
             (output->output_height != obj_surface->height ||
              (!output->textured_xvimage)))) {
            if (output->textured_xvimage)
                XFree(output->textured_xvimage);

            drv_debug_msg(VIDEO_DEBUG_GENERAL, "Create new XvImage for overlay\n");
            output->textured_xvimage = XvCreateImage((Display *)ctx->native_dpy, output->textured_portID, FOURCC_XVVA, 0,
                                       obj_surface->width, obj_surface->height);
            output->textured_xvimage->data = (char *) & output->imgdata_vasrf;
            output->output_width = obj_surface->width;
            output->output_height = obj_surface->height;

        }

        *xvImage = output->textured_xvimage;
        *port_id = output->textured_portID;

        if ((output->overlay_portID) && (output->using_port == USING_OVERLAY_PORT)) { /* stop overlay port */
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "Using textured xvideo, stop Overlay xvideo\n");
            XvStopVideo((Display *)ctx->native_dpy, output->overlay_portID, draw);
            XSync((Display *)ctx->native_dpy, False);

            output->using_port = USING_TEXTURE_PORT;
        }

        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Using Texture Xvideo (%d) for PutSurface\n", output->textured_portID);

        return 0;
    }

    return 0;
}

static int psb_force_dpms_on(VADriverContextP ctx)
{
    BOOL onoff;
    CARD16 state;

    DPMSInfo((Display *)ctx->native_dpy, &state, &onoff);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "DPMS is %s, monitor state=%s\n", onoff ? "enabled" : "disabled",
                             (state == DPMSModeOn) ? "on" : (
                                 (state == DPMSModeOff) ? "off" : (
                                     (state == DPMSModeStandby) ? "standby" : (
                                         (state == DPMSModeSuspend) ? "suspend" : "unknow"))));
    if (onoff && (state != DPMSModeOn)) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "DPMS is enabled, and monitor isn't DPMSModeOn, force it on\n");
        DPMSForceLevel((Display *)ctx->native_dpy, DPMSModeOn);
    }

    return 0;
}

VAStatus psb_check_rotatesurface(
    VADriverContextP ctx,
    unsigned short rotate_width,
    unsigned short rotate_height,
    unsigned int protected,
    int fourcc
)
{
    INIT_DRIVER_DATA;
    INIT_OUTPUT_PRIV;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    object_surface_p obj_rotate_surface;
    unsigned int flags = protected? IS_PROTECTED : 0;

    if (output->rotate_surface) {
        obj_rotate_surface = SURFACE(output->rotate_surfaceID);
        if (obj_rotate_surface &&
            ((obj_rotate_surface->width != rotate_width)
             || (obj_rotate_surface->height != rotate_height))) {
            psb_surface_destroy(output->rotate_surface);
            free(output->rotate_surface);
            object_heap_free(&driver_data->surface_heap, (object_base_p)obj_rotate_surface);
            output->rotate_surface = NULL;
        }
    }
    if (output->rotate_surface == NULL) {
        output->rotate_surfaceID = object_heap_allocate(&driver_data->surface_heap);
        obj_rotate_surface = SURFACE(output->rotate_surfaceID);
        if (NULL == obj_rotate_surface) {
            vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
            DEBUG_FAILURE;

            return VA_STATUS_ERROR_ALLOCATION_FAILED;
        }

        obj_rotate_surface->surface_id = output->rotate_surfaceID;
        obj_rotate_surface->context_id = -1;
        obj_rotate_surface->width = rotate_width;
        obj_rotate_surface->height = rotate_height;
        obj_rotate_surface->subpictures = NULL;
        obj_rotate_surface->subpic_count = 0;
        obj_rotate_surface->derived_imgcnt = 0;
        output->rotate_surface = (psb_surface_p) calloc(1, sizeof(struct psb_surface_s));
        if (NULL == output->rotate_surface) {
            object_heap_free(&driver_data->surface_heap, (object_base_p) obj_rotate_surface);
            obj_rotate_surface->surface_id = VA_INVALID_SURFACE;

            vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;

            DEBUG_FAILURE;

            return VA_STATUS_ERROR_ALLOCATION_FAILED;
        }

        flags |= IS_ROTATED;
        vaStatus = psb_surface_create(driver_data, rotate_width, rotate_height,
                                      fourcc, flags, output->rotate_surface);
        if (VA_STATUS_SUCCESS != vaStatus) {
            free(obj_rotate_surface->psb_surface);
            object_heap_free(&driver_data->surface_heap, (object_base_p) obj_rotate_surface);
            obj_rotate_surface->surface_id = VA_INVALID_SURFACE;

            DEBUG_FAILURE;
            return vaStatus;
        }
        obj_rotate_surface->psb_surface = output->rotate_surface;
    }
    return vaStatus;
}

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
)
{
    INIT_DRIVER_DATA;
    INIT_OUTPUT_PRIV;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    PsbVASurfaceRec *subpic_surface;
    PsbXvVAPutSurfacePtr vaPtr;
    XvPortID    portID = 0;
    XvImage *xvImage = NULL;
    object_surface_p obj_surface = SURFACE(surface);
    psb_surface_p psb_surface;
    int i = 0, j;


    if (obj_surface) /* silent klockwork, we already check it */
        psb_surface = obj_surface->psb_surface;
    else
        return VA_STATUS_ERROR_UNKNOWN;

    /* Catch X protocol errors with our own error handler */
    if (oldHandler == 0)
        oldHandler = XSetErrorHandler(psb_XErrorHandler);

    if (XErrorFlag == 1) {
        if (psb_CheckDrawable(ctx, draw) != 0) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "vaPutSurface: invalidate drawable\n");
            return VA_STATUS_ERROR_UNKNOWN;
        }

        drv_debug_msg(VIDEO_DEBUG_GENERAL, "ran psb_CheckDrawable the first time!\n");
        XErrorFlag = 0;
    }

    /* check display configuration for every 100 frames */
    if ((driver_data->frame_count % 100) == 0) {
        if (psb_CheckDrawable(ctx, draw) != 0) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "vaPutSurface: invalidate drawable\n");
            return VA_STATUS_ERROR_UNKNOWN;
        }
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "ran psb_CheckDrawable the first time!\n");
    }



    psb__CheckPutSurfaceXvPort(ctx, surface, draw,
                               srcx, srcy, srcw, srch,
                               destx, desty, destw, desth,
                               cliprects, number_cliprects, flags);
    psb__CheckGCXvImage(ctx, surface, draw, &xvImage, &portID, flags);

    if (flags & VA_CLEAR_DRAWABLE) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Clean draw with color 0x%08x\n", driver_data->clear_color);

        XFillRectangle((Display *)ctx->native_dpy, draw, output->gc, destx, desty, destw, desth);
        XSync((Display *)ctx->native_dpy, False);

        XFreeGC((Display *)ctx->native_dpy, output->gc);
        output->gc = NULL;
        output->output_drawable = 0;

        XSync((Display *)ctx->native_dpy, False);

        driver_data->cur_displaying_surface = VA_INVALID_SURFACE;
        driver_data->last_displaying_surface = VA_INVALID_SURFACE;
        obj_surface->display_timestamp = 0;


        return vaStatus;
    }

    vaPtr = (PsbXvVAPutSurfacePtr)xvImage->data;
    vaPtr->flags = flags;
    vaPtr->num_subpicture = obj_surface->subpic_count;
    vaPtr->num_clipbox = number_cliprects;
    for (j = 0; j < number_cliprects; j++) {
        vaPtr->clipbox[j].x = cliprects[j].x;
        vaPtr->clipbox[j].y = cliprects[j].y;
        vaPtr->clipbox[j].width = cliprects[j].width;
        vaPtr->clipbox[j].height = cliprects[j].height;
    }

    psb_surface_init(driver_data, &vaPtr->src_srf, VA_FOURCC_NV12, 2,
                     obj_surface->width, obj_surface->height,
                     psb_surface->stride, psb_surface->size,
                     psb_surface->buf.buffer_ofs, /* for surface created from RAR/camera device memory
                                                   * all surfaces share one BO but with different offset
                                                   * pass the offset as the "pre_add"
                                                   */
                     psb_surface->buf.drm_buf, flags);

    if ((driver_data->output_method == PSB_PUTSURFACE_OVERLAY)
        && (driver_data->drawable_info & (XVDRAWABLE_ROTATE_180 | XVDRAWABLE_ROTATE_90 | XVDRAWABLE_ROTATE_270))) {
        unsigned int rotate_width, rotate_height;
        int fourcc;
        if (output->sprite_enabled)
            fourcc = VA_FOURCC_RGBA;
        else
            fourcc = VA_FOURCC_NV12;
        if (driver_data->drawable_info & (XVDRAWABLE_ROTATE_90 | XVDRAWABLE_ROTATE_270)) {
            rotate_width = obj_surface->height;
            rotate_height = obj_surface->width;
        } else {
            rotate_width = obj_surface->width;
            rotate_height = obj_surface->height;
        }
        unsigned int protected = vaPtr->src_srf.pl_flags & 0;

        vaStatus = psb_check_rotatesurface(ctx, rotate_width, rotate_height, protected, fourcc);
        if (VA_STATUS_SUCCESS != vaStatus)
            return vaStatus;

        psb_surface_init(driver_data, &vaPtr->dst_srf, fourcc, 4,
                         rotate_width, rotate_height,
                         output->rotate_surface->stride, output->rotate_surface->size,
                         output->rotate_surface->buf.buffer_ofs, /* for surface created from RAR/camera device memory
                                                          * all surfaces share one BO but with different offset
                                                          * pass the offset as the "pre_add"
                                                          */
                         output->rotate_surface->buf.drm_buf, 0);
    }
    subpic_surface = obj_surface->subpictures;
    while (subpic_surface) {
        PsbVASurfaceRec *tmp = &vaPtr->subpic_srf[i++];

        memcpy(tmp, subpic_surface, sizeof(*tmp));

        /* reload palette for paletted subpicture
         * palete_ptr point to image palette
         */
        if (subpic_surface->palette_ptr)
            memcpy(&tmp->palette[0], subpic_surface->palette_ptr, 16 * sizeof(PsbAYUVSample8));

        subpic_surface = subpic_surface->next;
    }

    if (output->ignore_dpm == 0)
        psb_force_dpms_on(ctx);

    XvPutImage((Display *)ctx->native_dpy, portID, draw, output->gc, xvImage,
               srcx, srcy, srcw, srch, destx, desty, destw, desth);
    XFlush((Display *)ctx->native_dpy);
    //XSync((Display *)ctx->native_dpy, False);

    if (portID == output->overlay_portID) {
        if (driver_data->cur_displaying_surface != VA_INVALID_SURFACE)
            driver_data->last_displaying_surface = driver_data->cur_displaying_surface;
        obj_surface->display_timestamp = GetTickCount();
        driver_data->cur_displaying_surface = surface;
    } else {
        driver_data->cur_displaying_surface = VA_INVALID_SURFACE;
        driver_data->last_displaying_surface = VA_INVALID_SURFACE;
        obj_surface->display_timestamp = 0;
    }


    return vaStatus;
}

