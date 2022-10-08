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
 *
 */

#include <unistd.h>
#include "psb_xrandr.h"
#include "psb_x11.h"
#include "psb_drv_debug.h"

/* Global variable for xrandr */
psb_xrandr_info_p psb_xrandr_info;

#define INIT_DRIVER_DATA    psb_driver_data_p driver_data = (psb_driver_data_p) ctx->pDriverData
#define INIT_OUTPUT_PRIV    psb_x11_output_p output = (psb_x11_output_p)(((psb_driver_data_p)ctx->pDriverData)->ws_priv)

#define MWM_HINTS_DECORATIONS (1L << 1)
typedef struct {
    int flags;
    int functions;
    int decorations;
    int input_mode;
    int status;
} MWMHints;

char* location2string(psb_xrandr_location location)
{
    switch (location) {
    case ABOVE:
        return "ABOVE";
        break;
    case BELOW:
        return "BELOW";
        break;
    case LEFT_OF:
        return "LEFT_OF";
        break;
    case RIGHT_OF:
        return "RIGHT_OF";
        break;
    default:
        return "NORMAL";
        break;
    }
}

static int RRrotation2VArotation(Rotation rotation)
{
    switch (rotation) {
    case RR_Rotate_0:
        return VA_ROTATION_NONE;
    case RR_Rotate_90:
        return VA_ROTATION_270;
    case RR_Rotate_180:
        return VA_ROTATION_180;
    case RR_Rotate_270:
        return VA_ROTATION_90;
    }

    return 0;
}
static psb_xrandr_crtc_p get_crtc_by_id(RRCrtc crtc_id)
{
    psb_xrandr_crtc_p p_crtc;
    for (p_crtc = psb_xrandr_info->crtc_head; p_crtc; p_crtc = p_crtc->next)
        if (p_crtc->crtc_id == crtc_id)
            return p_crtc;
    return NULL;
}

static void psb_xrandr_hdmi_property(VADriverContextP ctx)
{
    INIT_DRIVER_DATA;
    Atom *props;
    Atom actual_type;
    XRRPropertyInfo *propinfo;
    int i, nprop, actual_format;
    unsigned long nitems, bytes_after;
    char* prop_name;
    unsigned char* prop;

    /* Check HDMI properties */
    props = XRRListOutputProperties(psb_xrandr_info->dpy, psb_xrandr_info->extend_output->output_id, &nprop);
    if (!props) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Xrandr: XRRListOutputProperties failed\n", psb_xrandr_info->extend_output->output_id);
        return;
    }

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Xrandr: extend output %08x has %d properties\n", psb_xrandr_info->extend_output->output_id, nprop);

    for (i = 0; i < nprop; i++) {
        XRRGetOutputProperty(psb_xrandr_info->dpy, psb_xrandr_info->extend_output->output_id, props[i],
                             0, 100, False, False, AnyPropertyType, &actual_type, &actual_format,
                             &nitems, &bytes_after, &prop);

        propinfo = XRRQueryOutputProperty(psb_xrandr_info->dpy, psb_xrandr_info->extend_output->output_id, props[i]);
        if (!propinfo) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "Xrandr: get output %08x prop %08x failed\n", psb_xrandr_info->extend_output->output_id, props[i]);
            return;
        }

        prop_name = XGetAtomName(psb_xrandr_info->dpy, props[i]);

        /* Currently all properties are XA_INTEGER, 32 */
        if (!strcmp(prop_name, "ExtVideoMode")) {
            psb_xrandr_info->hdmi_extvideo_prop->ExtVideoMode = (int)((INT32*)prop)[0];
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "Xrandr: ExtVideoMode (%08x)\n", psb_xrandr_info->hdmi_extvideo_prop->ExtVideoMode);
        } else if (!strcmp(prop_name, "ExtVideoMode_Xres")) {
            psb_xrandr_info->hdmi_extvideo_prop->ExtVideoMode_XRes = (int)((INT32*)prop)[0];
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "Xrandr: ExtVideoMode_XRes (%08x)\n", psb_xrandr_info->hdmi_extvideo_prop->ExtVideoMode_XRes);
        } else if (!strcmp(prop_name, "ExtVideoMode_Yres")) {
            psb_xrandr_info->hdmi_extvideo_prop->ExtVideoMode_YRes = (int)((INT32*)prop)[0];
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "Xrandr: ExtVideoMode_YRes (%08x)\n", psb_xrandr_info->hdmi_extvideo_prop->ExtVideoMode_YRes);
        } else if (!strcmp(prop_name, "ExtVideoMode_X_Offset")) {
            psb_xrandr_info->hdmi_extvideo_prop->ExtVideoMode_X_Offset = (int)((INT32*)prop)[0];
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "Xrandr: ExtVideoMode_X_Offset (%08x)\n", psb_xrandr_info->hdmi_extvideo_prop->ExtVideoMode_X_Offset);
        } else if (!strcmp(prop_name, "ExtVideoMode_Y_Offset")) {
            psb_xrandr_info->hdmi_extvideo_prop->ExtVideoMode_Y_Offset = (int)((INT32*)prop)[0];
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "Xrandr: ExtVideoMode_Y_Offset (%08x)\n", psb_xrandr_info->hdmi_extvideo_prop->ExtVideoMode_Y_Offset);
        } else if (!strcmp(prop_name, "ExtVideoMode_Center")) {
            psb_xrandr_info->hdmi_extvideo_prop->ExtVideoMode_Center = (int)((INT32*)prop)[0];
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "Xrandr: ExtVideoMode_Center (%08x)\n", psb_xrandr_info->hdmi_extvideo_prop->ExtVideoMode_Center);
        } else if (!strcmp(prop_name, "ExtVideoMode_SubTitle")) {
            psb_xrandr_info->hdmi_extvideo_prop->ExtVideoMode_SubTitle = (int)((INT32*)prop)[0];
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "Xrandr: ExtVideoMode_SubTitle (%08x)\n", psb_xrandr_info->hdmi_extvideo_prop->ExtVideoMode_SubTitle);
        } else if (!strcmp(prop_name, "ExtDesktopMode")) {
            if ((psb_xrandr_info->hdmi_extvideo_prop->ExtDesktopMode != EXTENDEDVIDEO) &&
                ((int)((INT32*)prop)[0] == EXTENDEDVIDEO)) {
                driver_data->xrandr_dirty |= PSB_NEW_EXTVIDEO;
            }
            psb_xrandr_info->hdmi_extvideo_prop->ExtDesktopMode = (int)((INT32*)prop)[0];
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "Xrandr: ExtDesktopMode (%08x)\n", psb_xrandr_info->hdmi_extvideo_prop->ExtDesktopMode);
        } else if (!strcmp(prop_name, "OverscanMode")) {
            psb_xrandr_info->hdmi_extvideo_prop->OverscanMode = (int)((INT32*)prop)[0];
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "Xrandr: OverscanMode (%08x)\n", psb_xrandr_info->hdmi_extvideo_prop->OverscanMode);
        } else if (!strcmp(prop_name, "PANELFITTING")) {
            psb_xrandr_info->hdmi_extvideo_prop->PANELFITTING = (int)((INT32*)prop)[0];
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "Xrandr: PANELFITTING (%08x)\n", psb_xrandr_info->hdmi_extvideo_prop->PANELFITTING);
        }
    }
}

static void psb_xrandr_mipi_location_init(psb_output_device_mode output_device_mode)
{
    psb_xrandr_crtc_p local_crtc = NULL, extend_crtc = NULL;

    switch (output_device_mode) {
    case SINGLE_MIPI0:
        psb_xrandr_info->hdmi_extvideo_prop->ExtDesktopMode = SINGLE;
        psb_xrandr_info->local_crtc[0]->location = NORMAL;
        return;
    case SINGLE_MIPI1:
        psb_xrandr_info->hdmi_extvideo_prop->ExtDesktopMode = SINGLE;
        psb_xrandr_info->local_crtc[1]->location = NORMAL;
        return;
    case MIPI0_MIPI1:
        local_crtc = psb_xrandr_info->local_crtc[0];
        extend_crtc = psb_xrandr_info->local_crtc[1];
        break;
    default:
        break;
    }

    if (!local_crtc || !extend_crtc) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Failed to get crtc info\n");
        return;
    }

    /* MIPI1 clone MIPI0 */
    if (local_crtc->x == 0 && local_crtc->y == 0 &&
        extend_crtc->x == 0 && extend_crtc->y == 0) {
        psb_xrandr_info->hdmi_extvideo_prop->ExtDesktopMode = CLONE;
        extend_crtc->location = NORMAL;
    } else {
        /* MIPI1 entend MIPI0 */
        psb_xrandr_info->hdmi_extvideo_prop->ExtDesktopMode = EXTENDED;
        if (local_crtc->y == extend_crtc->height)
            extend_crtc->location = ABOVE;
        else if (extend_crtc->y == local_crtc->height)
            extend_crtc->location = BELOW;
        else if (local_crtc->x == extend_crtc->width)
            extend_crtc->location = LEFT_OF;
        else if (extend_crtc->x == local_crtc->width)
            extend_crtc->location = RIGHT_OF;
    }
}

static void psb_xrandr_hdmi_location_init(psb_output_device_mode output_device_mode)
{
    psb_xrandr_crtc_p local_crtc = NULL, extend_crtc = NULL;

    switch (output_device_mode) {
    case SINGLE_HDMI:
        psb_xrandr_info->hdmi_extvideo_prop->ExtDesktopMode = SINGLE;
        psb_xrandr_info->extend_crtc->location = NORMAL;
        return;
    case MIPI0_HDMI:
    case MIPI0_MIPI1_HDMI:
        local_crtc = psb_xrandr_info->local_crtc[0];
        extend_crtc = psb_xrandr_info->extend_crtc;
        break;
    case MIPI1_HDMI:
        local_crtc = psb_xrandr_info->local_crtc[1];
        extend_crtc = psb_xrandr_info->extend_crtc;
        break;
    default:
        break;
    }

    if (!local_crtc || !extend_crtc) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Failed to get crtc info\n");
        return;
    }

    if (psb_xrandr_info->hdmi_extvideo_prop->ExtDesktopMode == CLONE)
        psb_xrandr_info->extend_crtc->location = NORMAL;

    if (psb_xrandr_info->hdmi_extvideo_prop->ExtDesktopMode == EXTENDED
        || psb_xrandr_info->hdmi_extvideo_prop->ExtDesktopMode == EXTENDEDVIDEO) {
        if (local_crtc->y == extend_crtc->height)
            psb_xrandr_info->extend_crtc->location = ABOVE;
        else if (extend_crtc->y == local_crtc->height)
            psb_xrandr_info->extend_crtc->location = BELOW;
        else if (local_crtc->x == extend_crtc->width)
            psb_xrandr_info->extend_crtc->location = LEFT_OF;
        else if (extend_crtc->x == local_crtc->width)
            psb_xrandr_info->extend_crtc->location = RIGHT_OF;
    }
}

static void psb_xrandr_coordinate_init(VADriverContextP ctx)
{
    INIT_DRIVER_DATA;
    psb_xrandr_output_p p_output;

    psb_xrandr_info->output_changed = 1;

    for (p_output = psb_xrandr_info->output_head; p_output; p_output = p_output->next) {
        if (p_output->connection == RR_Connected) {
            if (!strcmp(p_output->name, "MIPI0")) {
                if (p_output->crtc) {
                    psb_xrandr_info->mipi0_enabled = 1;
                    psb_xrandr_info->local_output[0] = p_output;
                    psb_xrandr_info->local_crtc[0] = p_output->crtc;
                    if (psb_xrandr_info->mipi0_rotation != p_output->crtc->rotation) {
                        psb_xrandr_info->mipi0_rotation = p_output->crtc->rotation;
                        driver_data->mipi0_rotation = RRrotation2VArotation(psb_xrandr_info->mipi0_rotation);
                        driver_data->xrandr_dirty |= PSB_NEW_ROTATION;
                    }
                } else {
                    psb_xrandr_info->mipi0_enabled = 0;
                    psb_xrandr_info->local_output[0] = NULL;
                    psb_xrandr_info->local_crtc[0] = NULL;
                }
            } else if (!strcmp(p_output->name, "MIPI1")) {
                if (p_output->crtc) {
                    psb_xrandr_info->mipi1_enabled = 1;
                    psb_xrandr_info->local_output[1] = p_output;
                    psb_xrandr_info->local_crtc[1] = p_output->crtc;
                    if (psb_xrandr_info->mipi1_rotation != p_output->crtc->rotation) {
                        psb_xrandr_info->mipi1_rotation = p_output->crtc->rotation;
                        driver_data->mipi1_rotation = RRrotation2VArotation(psb_xrandr_info->mipi1_rotation);
                        driver_data->xrandr_dirty |= PSB_NEW_ROTATION;
                    }
                } else {
                    psb_xrandr_info->mipi1_enabled = 0;
                    psb_xrandr_info->local_output[1] = NULL;
                    psb_xrandr_info->local_crtc[1] = NULL;
                }
            } else if (!strcmp(p_output->name, "TMDS0-1")) {
                if (p_output->crtc) {
                    psb_xrandr_info->hdmi_enabled = 1;
                    psb_xrandr_info->extend_output = p_output;
                    psb_xrandr_info->extend_crtc = p_output->crtc;
                    if (psb_xrandr_info->hdmi_rotation != p_output->crtc->rotation) {
                        psb_xrandr_info->hdmi_rotation = p_output->crtc->rotation;
                        driver_data->hdmi_rotation = RRrotation2VArotation(psb_xrandr_info->hdmi_rotation);
                        driver_data->xrandr_dirty |= PSB_NEW_ROTATION;
                    }
                } else {
                    psb_xrandr_info->hdmi_enabled = 0;
                    psb_xrandr_info->extend_output = NULL;
                    psb_xrandr_info->extend_crtc = NULL;
                }
            } else if (!strcmp(p_output->name, "LVDS0") && IS_MRST(driver_data)) {
                if (p_output->crtc) {
                    psb_xrandr_info->lvds0_enabled = 1;
                    psb_xrandr_info->local_output[0] = p_output;
                    psb_xrandr_info->local_crtc[0] = p_output->crtc;
                } else {
                    psb_xrandr_info->lvds0_enabled = 0;
                    psb_xrandr_info->local_output[0] = NULL;
                    psb_xrandr_info->local_crtc[0] = NULL;
                }
            }
        }
    }

    /* for MRST */
    if (IS_MRST(driver_data) && psb_xrandr_info->lvds0_enabled) {
        psb_xrandr_info->hdmi_extvideo_prop->ExtDesktopMode = SINGLE;
        psb_xrandr_info->output_device_mode = SINGLE_LVDS0;
        psb_xrandr_info->local_crtc[0]->location = NORMAL;
        return;
    }

    /* HDMI + either MIPI0 or MIPI1 */
    if (psb_xrandr_info->hdmi_enabled) {

        /* Get HDMI properties if it is enabled*/
        psb_xrandr_hdmi_property(ctx);

        /* Only HDMI */
        if (!psb_xrandr_info->mipi0_enabled && !psb_xrandr_info->mipi1_enabled)
            psb_xrandr_info->output_device_mode = SINGLE_HDMI;

        /* HDMI + MIPI0 */
        if (psb_xrandr_info->mipi0_enabled && !psb_xrandr_info->mipi1_enabled)
            psb_xrandr_info->output_device_mode = MIPI0_HDMI;

        /* HDMI + MIPI1 */
        if (!psb_xrandr_info->mipi0_enabled && psb_xrandr_info->mipi1_enabled)
            psb_xrandr_info->output_device_mode = MIPI1_HDMI;

        /* HDMI + MIPI0 + MIPI1 */
        if (psb_xrandr_info->mipi0_enabled && psb_xrandr_info->mipi1_enabled)
            psb_xrandr_info->output_device_mode = MIPI0_MIPI1_HDMI;

        psb_xrandr_hdmi_location_init(psb_xrandr_info->output_device_mode);
    } else {
        /* MIPI0 + MIPI1 */
        if (psb_xrandr_info->mipi0_enabled && psb_xrandr_info->mipi1_enabled) {
            psb_xrandr_info->output_device_mode = MIPI0_MIPI1;
        } else {
            /* MIPI0/MIPI1 */
            if (psb_xrandr_info->mipi0_enabled)
                psb_xrandr_info->output_device_mode = SINGLE_MIPI0;
            else if (psb_xrandr_info->mipi1_enabled)
                psb_xrandr_info->output_device_mode = SINGLE_MIPI1;
        }

        psb_xrandr_mipi_location_init(psb_xrandr_info->output_device_mode);
    }
}

void psb_xrandr_refresh(VADriverContextP ctx)
{
    int i;

    XRROutputInfo *output_info;
    XRRCrtcInfo *crtc_info;

    psb_xrandr_crtc_p p_crtc;
    psb_xrandr_output_p p_output;

    pthread_mutex_lock(&psb_xrandr_info->psb_extvideo_mutex);

    //deinit crtc
    if (psb_xrandr_info->crtc_head) {
        while (psb_xrandr_info->crtc_head) {
            psb_xrandr_info->crtc_tail = psb_xrandr_info->crtc_head->next;

            free(psb_xrandr_info->crtc_head);

            psb_xrandr_info->crtc_head = psb_xrandr_info->crtc_tail;
        }
        psb_xrandr_info->crtc_head = psb_xrandr_info->crtc_tail = NULL;
    }

    for (i = 0; i < psb_xrandr_info->res->ncrtc; i++) {
        crtc_info = XRRGetCrtcInfo(psb_xrandr_info->dpy, psb_xrandr_info->res, psb_xrandr_info->res->crtcs[i]);
        if (crtc_info) {
            p_crtc = (psb_xrandr_crtc_p)calloc(1, sizeof(psb_xrandr_crtc_s));
            if (!p_crtc) {
                drv_debug_msg(VIDEO_DEBUG_ERROR, "output of memory\n");
                return;
            }

            if (i == 0)
                psb_xrandr_info->crtc_head = psb_xrandr_info->crtc_tail = p_crtc;

            p_crtc->crtc_id = psb_xrandr_info->res->crtcs[i];
            p_crtc->x = crtc_info->x;
            p_crtc->y = crtc_info->y;
            p_crtc->width = crtc_info->width;
            p_crtc->height = crtc_info->height;
            p_crtc->crtc_mode = crtc_info->mode;
            p_crtc->noutput = crtc_info->noutput;
            p_crtc->rotation = crtc_info->rotation;

            psb_xrandr_info->crtc_tail->next = p_crtc;
            p_crtc->next = NULL;
            psb_xrandr_info->crtc_tail = p_crtc;
        } else {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "failed to get crtc_info\n");
            pthread_mutex_unlock(&psb_xrandr_info->psb_extvideo_mutex);
            return;
        }
    }

    //deinit output
    if (psb_xrandr_info->output_head) {
        while (psb_xrandr_info->output_head) {
            psb_xrandr_info->output_tail = psb_xrandr_info->output_head->next;

            free(psb_xrandr_info->output_head);

            psb_xrandr_info->output_head = psb_xrandr_info->output_tail;
        }
        psb_xrandr_info->output_head = psb_xrandr_info->output_tail = NULL;
    }
#if 0
    //destroy the full-screen window
    //FIXME: commited out for X Error message: BadDrawable, need more investigation
    if (va_output) {
        if (va_output->extend_drawable) {
            XDestroyWindow(ctx->native_dpy, va_output->extend_drawable);
            va_output->extend_drawable = 0;
            texture_priv->extend_dri_init_flag = 0;
        }
    }
#endif
    for (i = 0; i < psb_xrandr_info->res->noutput; i++) {
        output_info = XRRGetOutputInfo(psb_xrandr_info->dpy, psb_xrandr_info->res, psb_xrandr_info->res->outputs[i]);
        if (output_info) {
            p_output = (psb_xrandr_output_p)calloc(1, sizeof(psb_xrandr_output_s));
            if (!p_output) {
                drv_debug_msg(VIDEO_DEBUG_ERROR, "output of memory\n");
                return;
            }

            if (i == 0)
                psb_xrandr_info->output_head = psb_xrandr_info->output_tail = p_output;

            p_output->output_id = psb_xrandr_info->res->outputs[i];

            p_output->connection = output_info->connection;
            if (p_output->connection == RR_Connected)
                psb_xrandr_info->nconnected_output++;

            strcpy(p_output->name, output_info->name);

            if (output_info->crtc)
                p_output->crtc = get_crtc_by_id(output_info->crtc);
            else
                p_output->crtc = NULL;

            psb_xrandr_info->output_tail->next = p_output;
            p_output->next = NULL;
            psb_xrandr_info->output_tail = p_output;
        } else {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "failed to get output_info\n");
            pthread_mutex_unlock(&psb_xrandr_info->psb_extvideo_mutex);
            return;
        }
    }

    psb_xrandr_coordinate_init(ctx);

    psb_RecalcRotate(ctx);
    pthread_mutex_unlock(&psb_xrandr_info->psb_extvideo_mutex);
}

static Bool
outputChangePredicate(Display *display, XEvent *event, char *args)
{
    int event_base, error_base;

    XRRQueryExtension(psb_xrandr_info->dpy, &event_base, &error_base);
    return ((event->type == event_base + RRNotify_OutputChange) ||
            ((event->type == ClientMessage) &&
             (((XClientMessageEvent*)event)->message_type == psb_xrandr_info->psb_exit_atom)));
}

void psb_xrandr_thread(void* arg)
{
    VADriverContextP ctx = (VADriverContextP)arg;
    INIT_DRIVER_DATA;
    int event_base, error_base;
    XEvent event;
    XRRQueryExtension(psb_xrandr_info->dpy, &event_base, &error_base);
    XRRSelectInput(psb_xrandr_info->dpy, psb_xrandr_info->root, RRScreenChangeNotifyMask | RRCrtcChangeNotifyMask | RROutputChangeNotifyMask | RROutputPropertyNotifyMask);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Xrandr: psb xrandr thread start\n");

    while (1) {
        if (XCheckIfEvent(psb_xrandr_info->dpy, (XEvent *)&event, outputChangePredicate, NULL)) {
            if (event.type == ClientMessage) {
                drv_debug_msg(VIDEO_DEBUG_GENERAL, "Xrandr: receive ClientMessage event, thread should exit\n");
                XClientMessageEvent *evt;
                evt = (XClientMessageEvent*) & event;
                if (evt->message_type == psb_xrandr_info->psb_exit_atom) {
                    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Xrandr: xrandr thread exit safely\n");
                    pthread_exit(NULL);
                }
            }
            switch (event.type - event_base) {
            case RRNotify_OutputChange:
                XRRUpdateConfiguration(&event);
                drv_debug_msg(VIDEO_DEBUG_GENERAL, "Xrandr: receive RRNotify_OutputChange event, refresh output/crtc info\n");
                driver_data->xrandr_update = 1;
                psb_xrandr_refresh(ctx);
                break;
            default:
                break;
            }
        }
        usleep(200000);
    }
}

Window psb_xrandr_create_full_screen_window(unsigned int destx, unsigned int desty, unsigned int destw, unsigned int desth)
{
    int x, y, width, height;
    Window win;

    x = psb_xrandr_info->extend_crtc->x;
    y = psb_xrandr_info->extend_crtc->y;
    width = psb_xrandr_info->extend_crtc->width;
    height = psb_xrandr_info->extend_crtc->height;

    if (destw == 0 || desth == 0) {
        destw = width;
        desth = height;
    }
    win = XCreateSimpleWindow(psb_xrandr_info->dpy, DefaultRootWindow(psb_xrandr_info->dpy), destx, desty, destw, desth, 0, 0, 0);

    MWMHints mwmhints;
    Atom MOTIF_WM_HINTS;

    mwmhints.flags = MWM_HINTS_DECORATIONS;
    mwmhints.decorations = 0; /* MWM_DECOR_BORDER */
    MOTIF_WM_HINTS = XInternAtom(psb_xrandr_info->dpy, "_MOTIF_WM_HINTS", False);
    XChangeProperty(psb_xrandr_info->dpy, win, MOTIF_WM_HINTS, MOTIF_WM_HINTS, sizeof(long) * 8,
                    PropModeReplace, (unsigned char*) &mwmhints, sizeof(mwmhints) / sizeof(long));

    XSetWindowAttributes attributes;
    attributes.override_redirect = 1;
    unsigned long valuemask;
    valuemask = CWOverrideRedirect ;
    XChangeWindowAttributes(psb_xrandr_info->dpy, win, valuemask, &attributes);

    XMapWindow(psb_xrandr_info->dpy, win);
    XFlush(psb_xrandr_info->dpy);
    return win;
}

int psb_xrandr_hdmi_enabled()
{
    int ret;
    pthread_mutex_lock(&psb_xrandr_info->psb_extvideo_mutex);
    ret = psb_xrandr_info->hdmi_enabled;
    pthread_mutex_unlock(&psb_xrandr_info->psb_extvideo_mutex);
    return ret;
}

int psb_xrandr_mipi0_enabled()
{
    int ret;
    pthread_mutex_lock(&psb_xrandr_info->psb_extvideo_mutex);
    ret = psb_xrandr_info->mipi0_enabled;
    pthread_mutex_unlock(&psb_xrandr_info->psb_extvideo_mutex);
    return ret;
}

int psb_xrandr_mipi1_enabled()
{
    int ret;
    pthread_mutex_lock(&psb_xrandr_info->psb_extvideo_mutex);
    ret = psb_xrandr_info->mipi1_enabled;
    pthread_mutex_unlock(&psb_xrandr_info->psb_extvideo_mutex);
    return ret;
}

int psb_xrandr_single_mode()
{
    int ret;
    pthread_mutex_lock(&psb_xrandr_info->psb_extvideo_mutex);
    ret = (psb_xrandr_info->hdmi_extvideo_prop->ExtDesktopMode == SINGLE) ? 1 : 0;
    pthread_mutex_unlock(&psb_xrandr_info->psb_extvideo_mutex);
    return ret;
}

int psb_xrandr_clone_mode()
{
    int ret;
    pthread_mutex_lock(&psb_xrandr_info->psb_extvideo_mutex);
    ret = (psb_xrandr_info->hdmi_extvideo_prop->ExtDesktopMode == CLONE) ? 1 : 0;
    pthread_mutex_unlock(&psb_xrandr_info->psb_extvideo_mutex);
    return ret;
}

int psb_xrandr_extend_mode()
{
    int ret;
    pthread_mutex_lock(&psb_xrandr_info->psb_extvideo_mutex);
    ret = (psb_xrandr_info->hdmi_extvideo_prop->ExtDesktopMode == EXTENDED) ? 1 : 0;
    pthread_mutex_unlock(&psb_xrandr_info->psb_extvideo_mutex);
    return ret;
}

int psb_xrandr_extvideo_mode()
{
    int ret;
    pthread_mutex_lock(&psb_xrandr_info->psb_extvideo_mutex);
    ret = (psb_xrandr_info->hdmi_extvideo_prop->ExtDesktopMode == EXTENDEDVIDEO) ? 1 : 0;
    pthread_mutex_unlock(&psb_xrandr_info->psb_extvideo_mutex);
    return ret;
}

int psb_xrandr_outputchanged()
{
    int ret;
    pthread_mutex_lock(&psb_xrandr_info->psb_extvideo_mutex);
    if (psb_xrandr_info->output_changed) {
        psb_xrandr_info->output_changed = 0;
        ret = 1;
    } else
        ret = 0;
    pthread_mutex_unlock(&psb_xrandr_info->psb_extvideo_mutex);
    return ret;
}

VAStatus psb_xrandr_extvideo_prop(unsigned int *xres, unsigned int *yres, unsigned int *xoffset,
                                  unsigned int *yoffset, psb_extvideo_center *center, psb_extvideo_subtitle *subtitle,
                                  unsigned int *overscanmode, unsigned int *pannelfitting)
{
    pthread_mutex_lock(&psb_xrandr_info->psb_extvideo_mutex);

    if (psb_xrandr_info->hdmi_extvideo_prop->ExtDesktopMode != EXTENDEDVIDEO) {
        pthread_mutex_unlock(&psb_xrandr_info->psb_extvideo_mutex);
        return VA_STATUS_ERROR_UNKNOWN;
    }

    *xres = psb_xrandr_info->hdmi_extvideo_prop->ExtVideoMode_XRes;
    *yres = psb_xrandr_info->hdmi_extvideo_prop->ExtVideoMode_YRes;
    *xoffset = psb_xrandr_info->hdmi_extvideo_prop->ExtVideoMode_X_Offset;
    *yoffset = psb_xrandr_info->hdmi_extvideo_prop->ExtVideoMode_Y_Offset;
    *center = psb_xrandr_info->hdmi_extvideo_prop->ExtVideoMode_Center;
    *subtitle = psb_xrandr_info->hdmi_extvideo_prop->ExtVideoMode_SubTitle;
    *pannelfitting = psb_xrandr_info->hdmi_extvideo_prop->PANELFITTING;
    *overscanmode = psb_xrandr_info->hdmi_extvideo_prop->OverscanMode;

    pthread_mutex_unlock(&psb_xrandr_info->psb_extvideo_mutex);
    return VA_STATUS_SUCCESS;
}

VAStatus psb_xrandr_local_crtc_coordinate(psb_output_device *local_device_enabled, int *x, int *y, int *width, int *height, Rotation *rotation)
{
    psb_xrandr_crtc_p p_crtc;
    pthread_mutex_lock(&psb_xrandr_info->psb_extvideo_mutex);

    switch (psb_xrandr_info->output_device_mode) {
    case SINGLE_LVDS0:
        *local_device_enabled = LVDS0;
        p_crtc = psb_xrandr_info->local_crtc[0];
        break;
    case SINGLE_MIPI0:
    case MIPI0_MIPI1:
    case MIPI0_HDMI:
    case MIPI0_MIPI1_HDMI:
        *local_device_enabled = MIPI0;
        p_crtc = psb_xrandr_info->local_crtc[0];
        break;
    case SINGLE_MIPI1:
    case MIPI1_HDMI:
        *local_device_enabled = MIPI1;
        p_crtc = psb_xrandr_info->local_crtc[1];
        break;
    case SINGLE_HDMI:
        *local_device_enabled = HDMI;
        p_crtc = psb_xrandr_info->extend_crtc;
        break;
    default:
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Xrandr: Unknown statue\n");
        pthread_mutex_unlock(&psb_xrandr_info->psb_extvideo_mutex);
        return VA_STATUS_ERROR_UNKNOWN;
        break;
    }

    if (p_crtc) {
        *x = p_crtc->x;
        *y = p_crtc->y;
        *width = p_crtc->width;
        *height = p_crtc->height;
        *rotation = p_crtc->rotation;
        pthread_mutex_unlock(&psb_xrandr_info->psb_extvideo_mutex);
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Xrandr: device %08x enabled, crtc %08x coordinate: x = %d, y = %d, widht = %d, height = %d, rotate = %08x\n",
                                 *local_device_enabled, p_crtc->crtc_id, *x, *y, *width + 1, *height + 1, *rotation);
        return VA_STATUS_SUCCESS;
    } else {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Xrandr: local device is not available\n");
        pthread_mutex_unlock(&psb_xrandr_info->psb_extvideo_mutex);
        return VA_STATUS_ERROR_UNKNOWN;
    }
}

VAStatus psb_xrandr_extend_crtc_coordinate(psb_output_device *extend_device_enabled, int *x, int *y, int *width, int *height, psb_xrandr_location *location, Rotation *rotation)
{
    psb_xrandr_crtc_p p_crtc;

    pthread_mutex_lock(&psb_xrandr_info->psb_extvideo_mutex);

    switch (psb_xrandr_info->output_device_mode) {
    case MIPI0_MIPI1:
        *extend_device_enabled = MIPI1;
        p_crtc = psb_xrandr_info->local_crtc[1];
        break;
    case MIPI0_HDMI:
    case MIPI0_MIPI1_HDMI:
    case MIPI1_HDMI:
        *extend_device_enabled = HDMI;
        p_crtc = psb_xrandr_info->extend_crtc;
        break;
    default:
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Xrandr: Unknown status, may be extend device is not available\n");
        pthread_mutex_unlock(&psb_xrandr_info->psb_extvideo_mutex);
        return VA_STATUS_ERROR_UNKNOWN;
        break;
    }

    if (p_crtc) {
        *x = p_crtc->x;
        *y = p_crtc->y;
        *width = p_crtc->width;
        *height = p_crtc->height;
        *location = p_crtc->location;
        *rotation = p_crtc->rotation;
        pthread_mutex_unlock(&psb_xrandr_info->psb_extvideo_mutex);
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Xrandr: extend device %08x enabled, crtc %08x coordinate: x = %d, y = %d, widht = %d, height = %d, location = %s, rotation = %08x\n",
                                 *extend_device_enabled, p_crtc->crtc_id, *x, *y, *width + 1, *height + 1, location2string(p_crtc->location), *rotation);
    } else {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Xrandr: extend device is not available\n");
        pthread_mutex_unlock(&psb_xrandr_info->psb_extvideo_mutex);
        return VA_STATUS_ERROR_UNKNOWN;
    }

    return VA_STATUS_SUCCESS;
}

VAStatus psb_xrandr_thread_exit()
{
    int ret;

    XSelectInput(psb_xrandr_info->dpy, psb_xrandr_info->root, StructureNotifyMask);
    XClientMessageEvent xevent;
    xevent.type = ClientMessage;
    xevent.message_type = psb_xrandr_info->psb_exit_atom;
    xevent.window = psb_xrandr_info->root;
    xevent.format = 32;
    ret = XSendEvent(psb_xrandr_info->dpy, psb_xrandr_info->root, 0, StructureNotifyMask, (XEvent*) & xevent);
    XFlush(psb_xrandr_info->dpy);
    if (!ret) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Xrandr: send thread exit event to drawable: failed\n");
        return VA_STATUS_ERROR_UNKNOWN;
    } else {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Xrandr: send thread exit event to drawable: success\n");
        return VA_STATUS_SUCCESS;
    }
}

VAStatus psb_xrandr_thread_create(VADriverContextP ctx)
{
    pthread_t id;
    INIT_DRIVER_DATA;

    psb_xrandr_info->psb_exit_atom = XInternAtom(psb_xrandr_info->dpy, "psb_exit_atom", 0);
    pthread_create(&id, NULL, (void*)psb_xrandr_thread, ctx);
    driver_data->xrandr_thread_id = id;
    return VA_STATUS_SUCCESS;
}

VAStatus psb_xrandr_deinit()
{
#ifdef _FOR_FPGA_
    return VA_STATUS_SUCCESS;
#endif
    pthread_mutex_lock(&psb_xrandr_info->psb_extvideo_mutex);
    //free crtc
    if (psb_xrandr_info->crtc_head) {
        while (psb_xrandr_info->crtc_head) {
            psb_xrandr_info->crtc_tail = psb_xrandr_info->crtc_head->next;

            free(psb_xrandr_info->crtc_head);

            psb_xrandr_info->crtc_head = psb_xrandr_info->crtc_tail;
        }
        psb_xrandr_info->crtc_head = psb_xrandr_info->crtc_tail = NULL;
    }

    //free output
    if (psb_xrandr_info->output_head) {
        while (psb_xrandr_info->output_head) {
            psb_xrandr_info->output_tail = psb_xrandr_info->output_head->next;

            free(psb_xrandr_info->output_head);

            psb_xrandr_info->output_head = psb_xrandr_info->output_tail;
        }
        psb_xrandr_info->output_head = psb_xrandr_info->output_tail = NULL;
    }

    if (psb_xrandr_info->hdmi_extvideo_prop) {
        free(psb_xrandr_info->hdmi_extvideo_prop);
    }

    pthread_mutex_unlock(&psb_xrandr_info->psb_extvideo_mutex);
    pthread_mutex_destroy(&psb_xrandr_info->psb_extvideo_mutex);

    free(psb_xrandr_info);
    return VA_STATUS_SUCCESS;
}

VAStatus psb_xrandr_init(VADriverContextP ctx)
{
    int major, minor;
    int screen;

    psb_xrandr_info = (psb_xrandr_info_p)calloc(1, sizeof(psb_xrandr_info_s));

    if (!psb_xrandr_info) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "output of memory\n");
        return VA_STATUS_ERROR_UNKNOWN;
    }
    memset(psb_xrandr_info, 0, sizeof(psb_xrandr_info_s));
    psb_xrandr_info->mipi0_rotation = RR_Rotate_0;
    psb_xrandr_info->mipi1_rotation = RR_Rotate_0;
    psb_xrandr_info->hdmi_rotation = RR_Rotate_0;

    psb_xrandr_info->hdmi_extvideo_prop = (psb_extvideo_prop_p)calloc(1, sizeof(psb_extvideo_prop_s));
    if (!psb_xrandr_info->hdmi_extvideo_prop) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "output of memory\n");
        return VA_STATUS_ERROR_ALLOCATION_FAILED;
    }
    memset(psb_xrandr_info->hdmi_extvideo_prop, 0, sizeof(psb_extvideo_prop_s));

    psb_xrandr_info->dpy = (Display *)ctx->native_dpy;
    screen = DefaultScreen(psb_xrandr_info->dpy);

    if (screen >= ScreenCount(psb_xrandr_info->dpy)) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Xrandr: Invalid screen number %d (display has %d)\n",
                           screen, ScreenCount(psb_xrandr_info->dpy));
        return VA_STATUS_ERROR_UNKNOWN;
    }

    psb_xrandr_info->root = RootWindow(psb_xrandr_info->dpy, screen);

    if (!XRRQueryVersion(psb_xrandr_info->dpy, &major, &minor)) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Xrandr: RandR extension missing\n");
        return VA_STATUS_ERROR_UNKNOWN;
    }

    psb_xrandr_info->res = XRRGetScreenResources(psb_xrandr_info->dpy, psb_xrandr_info->root);
    if (!psb_xrandr_info->res)
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Xrandr: failed to get screen resources\n");

    pthread_mutex_init(&psb_xrandr_info->psb_extvideo_mutex, NULL);

    psb_xrandr_refresh(ctx);

    return VA_STATUS_SUCCESS;
}
