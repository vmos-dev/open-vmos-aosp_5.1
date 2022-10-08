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

#include "psb_HDMIExtMode.h"
#include "psb_output_android.h"
#include "pvr2d.h"
#include "psb_drv_video.h"
#include "psb_drv_debug.h"

#define INIT_DRIVER_DATA    psb_driver_data_p driver_data = (psb_driver_data_p) ctx->pDriverData

VAStatus psb_HDMIExt_get_prop(psb_android_output_p output,
                              unsigned short *xres, unsigned short *yres)
{
    psb_HDMIExt_info_p psb_HDMIExt_info = (psb_HDMIExt_info_p)output->psb_HDMIExt_info;

    if (!psb_HDMIExt_info || !psb_HDMIExt_info->hdmi_extvideo_prop ||
        (psb_HDMIExt_info->hdmi_extvideo_prop->ExtVideoMode == OFF)) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s : Failed to get HDMI prop\n", __FUNCTION__);
        return VA_STATUS_ERROR_UNKNOWN;
    }
    *xres = psb_HDMIExt_info->hdmi_extvideo_prop->ExtVideoMode_XRes;
    *yres = psb_HDMIExt_info->hdmi_extvideo_prop->ExtVideoMode_YRes;

    return VA_STATUS_SUCCESS;
}

psb_hdmi_mode psb_HDMIExt_get_mode(psb_android_output_p output)
{
    psb_HDMIExt_info_p psb_HDMIExt_info = (psb_HDMIExt_info_p)output->psb_HDMIExt_info;

    if (!psb_HDMIExt_info) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s : Failed to get HDMI mode\n", __FUNCTION__);
        return VA_STATUS_ERROR_UNKNOWN;
    }
    return psb_HDMIExt_info->hdmi_mode;
}

VAStatus psb_HDMIExt_update(VADriverContextP ctx, psb_HDMIExt_info_p psb_HDMIExt_info)
{
    INIT_DRIVER_DATA;
    drmModeCrtc *hdmi_crtc = NULL;
    drmModeConnector *hdmi_connector = NULL;
    drmModeEncoder *hdmi_encoder = NULL;
    char *strHeight = NULL;
    struct drm_lnc_video_getparam_arg arg;
    int hdmi_state = 0;
    static int hdmi_connected_frame = 0;

    arg.key = IMG_VIDEO_GET_HDMI_STATE;
    arg.value = (uint64_t)((unsigned int)&hdmi_state);
    drmCommandWriteRead(driver_data->drm_fd, driver_data->getParamIoctlOffset,
                        &arg, sizeof(arg));

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s : hdmi_state = %d\n", __FUNCTION__, hdmi_state);
    if (psb_HDMIExt_info->hdmi_state != hdmi_state) {
        psb_HDMIExt_info->hdmi_state = hdmi_state;
        switch (hdmi_state) {
            case HDMI_MODE_EXT_VIDEO:
                psb_HDMIExt_info->hdmi_extvideo_prop->ExtVideoMode = EXTENDED_VIDEO;
                psb_HDMIExt_info->hdmi_mode = EXTENDED_VIDEO;

                psb_extvideo_prop_p hdmi_extvideo_prop = psb_HDMIExt_info->hdmi_extvideo_prop;

                hdmi_connector = drmModeGetConnector(driver_data->drm_fd, psb_HDMIExt_info->hdmi_connector_id);
                if (!hdmi_connector) {
                    drv_debug_msg(VIDEO_DEBUG_ERROR, "%s : Failed to get hdmi connector\n", __FUNCTION__);
                    return VA_STATUS_ERROR_UNKNOWN;
                }

                hdmi_encoder = drmModeGetEncoder(driver_data->drm_fd, hdmi_connector->encoder_id);
                if (!hdmi_encoder) {
                    drv_debug_msg(VIDEO_DEBUG_ERROR, "%s : Failed to get hdmi encoder\n", __FUNCTION__);
                    return VA_STATUS_ERROR_UNKNOWN;
                }

                hdmi_crtc = drmModeGetCrtc(driver_data->drm_fd, hdmi_encoder->crtc_id);
                if (!hdmi_crtc) {
                    /* No CRTC attached to HDMI. */
                    drv_debug_msg(VIDEO_DEBUG_ERROR, "%s : Failed to get hdmi crtc\n", __FUNCTION__);
                    return VA_STATUS_ERROR_UNKNOWN;
                }

                strHeight = strstr(hdmi_crtc->mode.name, "x");
                hdmi_extvideo_prop->ExtVideoMode_XRes = (unsigned short)atoi(hdmi_crtc->mode.name);
                hdmi_extvideo_prop->ExtVideoMode_YRes = (unsigned short)atoi(strHeight + 1);
                drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s : size = %d x %d\n", __FUNCTION__,
                        hdmi_extvideo_prop->ExtVideoMode_XRes, hdmi_extvideo_prop->ExtVideoMode_YRes);
                drmModeFreeCrtc(hdmi_crtc);
                drmModeFreeEncoder(hdmi_encoder);
                drmModeFreeConnector(hdmi_connector);
                break;
            case HDMI_MODE_OFF:
                psb_HDMIExt_info->hdmi_extvideo_prop->ExtVideoMode = OFF;
                psb_HDMIExt_info->hdmi_mode = OFF;
                hdmi_connected_frame = 0;
                break;
            default:
                psb_HDMIExt_info->hdmi_extvideo_prop->ExtVideoMode = UNDEFINED;
                psb_HDMIExt_info->hdmi_mode = UNDEFINED;
        }
    }

    return VA_STATUS_SUCCESS;
}

psb_HDMIExt_info_p psb_HDMIExt_init(VADriverContextP ctx, psb_android_output_p output)
{
    INIT_DRIVER_DATA;
    drmModeRes *resources;
    drmModeConnector *connector = NULL;
    drmModeEncoder *mipi_encoder = NULL;
    int mipi_connector_id = 0, mipi_encoder_id = 0, mipi_crtc_id = 0, i;
    psb_HDMIExt_info_p psb_HDMIExt_info = NULL;

    psb_HDMIExt_info = (psb_HDMIExt_info_p)calloc(1, sizeof(psb_HDMIExt_info_s));
    if (!psb_HDMIExt_info) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s : Failed to create psb_HDMIExt_info.\n", __FUNCTION__);
        return NULL;
    }
    memset(psb_HDMIExt_info, 0, sizeof(psb_HDMIExt_info_s));

    psb_HDMIExt_info->hdmi_extvideo_prop = (psb_extvideo_prop_p)calloc(1, sizeof(psb_extvideo_prop_s));
    if (!psb_HDMIExt_info->hdmi_extvideo_prop) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s : Failed to create hdmi_extvideo_prop.\n", __FUNCTION__);
        free(psb_HDMIExt_info);
        return NULL;
    }
    memset(psb_HDMIExt_info->hdmi_extvideo_prop, 0, sizeof(psb_extvideo_prop_s));

    /*Get Resources.*/
    resources = drmModeGetResources(driver_data->drm_fd);
    if (!resources) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s : drmModeGetResources failed.\n", __FUNCTION__);
        goto exit;
    }

    /*Get MIPI and HDMI connector id.*/
    for (i = 0; i < resources->count_connectors; i++) {
        connector = drmModeGetConnector(driver_data->drm_fd, resources->connectors[i]);

        if (!connector) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "%s : Failed to get connector %i\n", __FUNCTION__,
                               resources->connectors[i]);
            continue;
        }

        if (connector->connector_type == DRM_MODE_CONNECTOR_DVID)
            psb_HDMIExt_info->hdmi_connector_id = connector->connector_id;

        if ((connector->connector_type == DRM_MODE_CONNECTOR_MIPI) &&
            (!mipi_connector_id)) {
            mipi_connector_id = connector->connector_id;
            mipi_encoder_id = connector->encoder_id;
        }

        drmModeFreeConnector(connector);
        connector = NULL;
    }

    if (!mipi_connector_id ||
        !psb_HDMIExt_info->hdmi_connector_id ||
        !mipi_encoder_id) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s : Failed to get connector id or mipi encoder id. mipi_connector_id=%d, hdmi_connector_id=%d, mipi_encoder_id=%d\n", __FUNCTION__,
                           mipi_connector_id, psb_HDMIExt_info->hdmi_connector_id, mipi_encoder_id);
        goto exit;
    }

    mipi_encoder = drmModeGetEncoder(driver_data->drm_fd, mipi_encoder_id);
    if (!mipi_encoder) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s : Failed to get mipi encoder %i\n", __FUNCTION__);
        goto exit;
    }

    psb_HDMIExt_info->mipi_crtc_id = mipi_encoder->crtc_id;
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s : mipi_crtc_id = %d\n", __FUNCTION__,
                             mipi_crtc_id);

    drmModeFreeEncoder(mipi_encoder);

    if (psb_HDMIExt_update(ctx, psb_HDMIExt_info))
        goto exit;

    if (resources)
        drmModeFreeResources(resources);

    return psb_HDMIExt_info;

exit:
    if (resources)
        drmModeFreeResources(resources);

    if (connector)
        drmModeFreeConnector(connector);

    return psb_HDMIExt_info;
}

VAStatus psb_HDMIExt_deinit(psb_android_output_p output)
{
    psb_HDMIExt_info_p psb_HDMIExt_info = (psb_HDMIExt_info_p)output->psb_HDMIExt_info;

    if (psb_HDMIExt_info->hdmi_extvideo_prop)
        free(psb_HDMIExt_info->hdmi_extvideo_prop);

    if (psb_HDMIExt_info)
        free(psb_HDMIExt_info);

    return VA_STATUS_SUCCESS;
}

