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
 *    Jason Hu <jason.hu@intel.com>
 */

#ifndef _PSB_HDMIEXTMODE_H_
#define _PSB_HDMIEXTMODE_H_

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "va/va.h"
#include "va/va_backend.h"
#include "xf86drm.h"
#include "xf86drmMode.h"
#include "psb_output_android.h"

#define DRM_MODE_CONNECTOR_MIPI    15
#define HDMI_MODE_OFF           0
#define HDMI_MODE_CLONE         1
#define HDMI_MODE_EXT_VIDEO         2
#define HDMI_MODE_EXT_DESKTOP       3
#define HDMI_MODE_CLONE_ROTATION    4

typedef enum _psb_hdmi_mode {
    OFF,
    CLONE,
    EXTENDED_VIDEO,
    EXTENDED_DESKTOP,
    UNDEFINED,
} psb_hdmi_mode;

typedef struct _psb_extvideo_prop_s {
    psb_hdmi_mode ExtVideoMode;

    unsigned short ExtVideoMode_XRes;
    unsigned short ExtVideoMode_YRes;
    short ExtVideoMode_X_Offset;
    short ExtVideoMode_Y_Offset;
} psb_extvideo_prop_s, *psb_extvideo_prop_p;

typedef struct _psb_HDMIExt_info_s {
    /*MIPI infos*/
    uint32_t mipi_crtc_id;
    /*hdmi infos*/
    uint32_t hdmi_connector_id;
    uint32_t hdmi_encoder_id;
    uint32_t hdmi_crtc_id;

    psb_hdmi_mode hdmi_mode;
    int hdmi_state;
    psb_extvideo_prop_p hdmi_extvideo_prop;
} psb_HDMIExt_info_s, *psb_HDMIExt_info_p;

VAStatus psb_HDMIExt_get_prop(psb_android_output_p output, unsigned short *xres, unsigned short *yres);

psb_hdmi_mode psb_HDMIExt_get_mode(psb_android_output_p output);
VAStatus psb_HDMIExt_update(VADriverContextP ctx, psb_HDMIExt_info_p psb_HDMIExt_info);

psb_HDMIExt_info_p psb_HDMIExt_init(VADriverContextP ctx, psb_android_output_p output);
VAStatus psb_HDMIExt_deinit(psb_android_output_p output);

#endif /* _PSB_HDMIEXTMODE_H_*/
