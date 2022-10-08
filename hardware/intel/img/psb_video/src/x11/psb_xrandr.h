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


#ifndef _PSB_XRANDR_H_
#define _PSB_XRANDR_H_
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xlibint.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/Xrender.h>     /* we share subpixel information */
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <va/va_backend.h>

typedef enum _psb_output_device {
    MIPI0,
    MIPI1,
    LVDS0,
    HDMI,
} psb_output_device;

typedef enum _psb_output_device_mode {
    SINGLE_MIPI0,
    SINGLE_MIPI1,
    SINGLE_HDMI,
    SINGLE_LVDS0,
    MIPI0_MIPI1,
    MIPI0_HDMI,
    MIPI0_MIPI1_HDMI,
    MIPI1_HDMI,
} psb_output_device_mode;

typedef enum _psb_hdmi_mode {
    CLONE,
    EXTENDED,
    EXTENDEDVIDEO,
    SINGLE,
    UNKNOWNVIDEOMODE,
} psb_hdmi_mode;

typedef enum _psb_extvideo_center {
    NOCENTER,
    CENTER,
    UNKNOWNCENTER,
} psb_extvideo_center;

typedef enum _psb_extvideo_subtitle {
    BOTH,
    ONLY_HDMI,
    NOSUBTITLE,
} psb_extvideo_subtitle;

typedef enum _psb_xrandr_location {
    NORMAL, ABOVE, BELOW, LEFT_OF, RIGHT_OF,
} psb_xrandr_location;

typedef struct _psb_extvideo_prop_s {
    psb_hdmi_mode ExtVideoMode;
    psb_hdmi_mode ExtDesktopMode;

    unsigned int ExtVideoMode_XRes;
    unsigned int ExtVideoMode_YRes;
    unsigned int ExtVideoMode_X_Offset;
    unsigned int ExtVideoMode_Y_Offset;
    unsigned int OverscanMode;
    unsigned int PANELFITTING;

    psb_extvideo_center ExtVideoMode_Center;
    psb_extvideo_subtitle ExtVideoMode_SubTitle;

} psb_extvideo_prop_s, *psb_extvideo_prop_p;

typedef struct _psb_xrandr_crtc_s {
    struct _psb_xrandr_crtc_s *next;

    RRCrtc crtc_id;
    RRMode crtc_mode;

    unsigned int width;
    unsigned int height;
    unsigned int x;
    unsigned int y;

    Rotation rotation;
    psb_xrandr_location location;

    int noutput;

} psb_xrandr_crtc_s, *psb_xrandr_crtc_p;

typedef struct _psb_xrandr_output_s {
    RROutput output_id;
    char name[10];
    struct _psb_xrandr_output_s *next;

    Connection connection;

    psb_xrandr_crtc_p crtc;

} psb_xrandr_output_s, *psb_xrandr_output_p;

typedef struct _psb_xrandr_info_s {
    psb_xrandr_crtc_p local_crtc[2];
    psb_xrandr_crtc_p extend_crtc;

    psb_xrandr_output_p local_output[2];
    psb_xrandr_output_p extend_output;

    unsigned int nconnected_output;

    psb_extvideo_prop_p hdmi_extvideo_prop;

    int lvds0_enabled;
    int mipi0_enabled;
    int mipi1_enabled;
    int hdmi_enabled;

    Rotation mipi0_rotation;
    Rotation mipi1_rotation;
    Rotation hdmi_rotation;

    int output_changed;
    psb_output_device_mode output_device_mode;

    psb_xrandr_crtc_p crtc_head, crtc_tail;
    psb_xrandr_output_p output_head, output_tail;

    pthread_mutex_t psb_extvideo_mutex;
    XRRScreenResources *res;
    Display *dpy;
    Window root;
    Atom psb_exit_atom;
} psb_xrandr_info_s, *psb_xrandr_info_p;


int psb_xrandr_hdmi_enabled();
int psb_xrandr_mipi1_enabled();
int psb_xrandr_single_mode();
int psb_xrandr_clone_mode();
int psb_xrandr_extend_mode();
int psb_xrandr_extvideo_mode();
int psb_xrandr_outputchanged();

Window psb_xrandr_create_full_screen_window(unsigned int destx, unsigned int desty, unsigned int destw, unsigned int desth);
VAStatus psb_xrandr_extvideo_prop(unsigned int *xres, unsigned int *yres, unsigned int *xoffset,
                                  unsigned int *yoffset, psb_extvideo_center *center, psb_extvideo_subtitle *subtitle,
                                  unsigned int *overscanmode, unsigned int *pannelfitting);
VAStatus psb_xrandr_local_crtc_coordinate(psb_output_device *local_device_enabled, int *x, int *y, int *width, int *height, Rotation *rotation);
VAStatus psb_xrandr_extend_crtc_coordinate(psb_output_device *extend_device_enabled, int *x, int *y, int *width, int *height, psb_xrandr_location *location, Rotation *rotation);

void psb_xrandr_refresh(VADriverContextP ctx);
void psb_xrandr_thread();
VAStatus psb_xrandr_thread_create(VADriverContextP ctx);
VAStatus psb_xrandr_thread_exit();
VAStatus psb_xrandr_init(VADriverContextP ctx);
VAStatus psb_xrandr_deinit();

#endif /* _PSB_XRANDR_H_ */
