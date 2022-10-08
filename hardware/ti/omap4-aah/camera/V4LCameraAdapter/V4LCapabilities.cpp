/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
* @file V4LCapabilities.cpp
*
* This file implements the V4L Capabilities feature.
*
*/

#include "CameraHal.h"
#include "V4LCameraAdapter.h"
#include "ErrorUtils.h"
#include "TICameraParameters.h"

namespace Ti {
namespace Camera {

/************************************
 * global constants and variables
 *************************************/

#define ARRAY_SIZE(array) (sizeof((array)) / sizeof((array)[0]))
#define MAX_RES_STRING_LENGTH 10
#define DEFAULT_WIDTH 640
#define DEFAULT_HEIGHT 480

static const char PARAM_SEP[] = ",";

//Camera defaults
const char V4LCameraAdapter::DEFAULT_PICTURE_FORMAT[] = "jpeg";
const char V4LCameraAdapter::DEFAULT_PICTURE_SIZE[] = "640x480";
const char V4LCameraAdapter::DEFAULT_PREVIEW_FORMAT[] = "yuv422i-yuyv";
const char V4LCameraAdapter::DEFAULT_PREVIEW_SIZE[] = "640x480";
const char V4LCameraAdapter::DEFAULT_NUM_PREV_BUFS[] = "6";
const char V4LCameraAdapter::DEFAULT_FRAMERATE[] = "30";
const char V4LCameraAdapter::DEFAULT_FOCUS_MODE[] = "infinity";
const char * V4LCameraAdapter::DEFAULT_VSTAB = android::CameraParameters::FALSE;
const char * V4LCameraAdapter::DEFAULT_VNF = android::CameraParameters::FALSE;


const CapPixelformat V4LCameraAdapter::mPixelformats [] = {
    { V4L2_PIX_FMT_YUYV, android::CameraParameters::PIXEL_FORMAT_YUV422I },
    { V4L2_PIX_FMT_JPEG, android::CameraParameters::PIXEL_FORMAT_JPEG },
};

/*****************************************
 * internal static function declarations
 *****************************************/

/**** Utility functions to help translate V4L Caps to Parameter ****/

status_t V4LCameraAdapter::insertDefaults(CameraProperties::Properties* params, V4L_TI_CAPTYPE &caps)
{
    status_t ret = NO_ERROR;
    LOG_FUNCTION_NAME;

    params->set(CameraProperties::PREVIEW_FORMAT, DEFAULT_PREVIEW_FORMAT);

    params->set(CameraProperties::PICTURE_FORMAT, DEFAULT_PICTURE_FORMAT);
    params->set(CameraProperties::PICTURE_SIZE, DEFAULT_PICTURE_SIZE);
    params->set(CameraProperties::PREVIEW_SIZE, DEFAULT_PREVIEW_SIZE);
    params->set(CameraProperties::PREVIEW_FRAME_RATE, DEFAULT_FRAMERATE);
    params->set(CameraProperties::REQUIRED_PREVIEW_BUFS, DEFAULT_NUM_PREV_BUFS);
    params->set(CameraProperties::FOCUS_MODE, DEFAULT_FOCUS_MODE);

    params->set(CameraProperties::CAMERA_NAME, "USBCAMERA");
    params->set(CameraProperties::JPEG_THUMBNAIL_SIZE, "320x240");
    params->set(CameraProperties::JPEG_QUALITY, "90");
    params->set(CameraProperties::JPEG_THUMBNAIL_QUALITY, "50");
    params->set(CameraProperties::FRAMERATE_RANGE_SUPPORTED, "(30000,30000)");
    params->set(CameraProperties::FRAMERATE_RANGE, "30000,30000");
    params->set(CameraProperties::S3D_PRV_FRAME_LAYOUT, "none");
    params->set(CameraProperties::SUPPORTED_EXPOSURE_MODES, "auto");
    params->set(CameraProperties::SUPPORTED_ISO_VALUES, "auto");
    params->set(CameraProperties::SUPPORTED_ANTIBANDING, "auto");
    params->set(CameraProperties::SUPPORTED_EFFECTS, "none");
    params->set(CameraProperties::SUPPORTED_IPP_MODES, "ldc-nsf");
    params->set(CameraProperties::FACING_INDEX, TICameraParameters::FACING_FRONT);
    params->set(CameraProperties::ORIENTATION_INDEX, 0);
    params->set(CameraProperties::SENSOR_ORIENTATION, "0");
    params->set(CameraProperties::VSTAB, DEFAULT_VSTAB);
    params->set(CameraProperties::VNF, DEFAULT_VNF);


    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t V4LCameraAdapter::insertPreviewFormats(CameraProperties::Properties* params, V4L_TI_CAPTYPE &caps) {

    char supported[MAX_PROP_VALUE_LENGTH];

    memset(supported, '\0', MAX_PROP_VALUE_LENGTH);
    for (int i = 0; i < caps.ulPreviewFormatCount; i++) {
        for (unsigned int j = 0; j < ARRAY_SIZE(mPixelformats); j++) {
            if(caps.ePreviewFormats[i] == mPixelformats[j].pixelformat ) {
                strncat (supported, mPixelformats[j].param, MAX_PROP_VALUE_LENGTH-1 );
                strncat (supported, PARAM_SEP, 1 );
            }
        }
    }
    strncat(supported, android::CameraParameters::PIXEL_FORMAT_YUV420P, MAX_PROP_VALUE_LENGTH - 1);
    params->set(CameraProperties::SUPPORTED_PREVIEW_FORMATS, supported);
    return NO_ERROR;
}

status_t V4LCameraAdapter::insertPreviewSizes(CameraProperties::Properties* params, V4L_TI_CAPTYPE &caps) {

    char supported[MAX_PROP_VALUE_LENGTH];

    memset(supported, '\0', MAX_PROP_VALUE_LENGTH);
    for (int i = 0; i < caps.ulPreviewResCount; i++) {
        if (supported[0] != '\0') {
            strncat(supported, PARAM_SEP, 1);
        }
        strncat (supported, caps.tPreviewRes[i].param, MAX_PROP_VALUE_LENGTH-1 );
    }

    params->set(CameraProperties::SUPPORTED_PREVIEW_SIZES, supported);
    params->set(CameraProperties::SUPPORTED_PREVIEW_SUBSAMPLED_SIZES, supported);
    return NO_ERROR;
}

status_t V4LCameraAdapter::insertImageSizes(CameraProperties::Properties* params, V4L_TI_CAPTYPE &caps) {

    char supported[MAX_PROP_VALUE_LENGTH];

    memset(supported, '\0', MAX_PROP_VALUE_LENGTH);
    for (int i = 0; i < caps.ulCaptureResCount; i++) {
        if (supported[0] != '\0') {
            strncat(supported, PARAM_SEP, 1);
        }
        strncat (supported, caps.tCaptureRes[i].param, MAX_PROP_VALUE_LENGTH-1 );
    }
    params->set(CameraProperties::SUPPORTED_PICTURE_SIZES, supported);
    return NO_ERROR;
}

status_t V4LCameraAdapter::insertFrameRates(CameraProperties::Properties* params, V4L_TI_CAPTYPE &caps) {

    char supported[MAX_PROP_VALUE_LENGTH];
    char temp[10];

    memset(supported, '\0', MAX_PROP_VALUE_LENGTH);
    for (int i = 0; i < caps.ulFrameRateCount; i++) {
        snprintf (temp, 10, "%d", caps.ulFrameRates[i] );
        if (supported[0] != '\0') {
            strncat(supported, PARAM_SEP, 1);
        }
        strncat (supported, temp, MAX_PROP_VALUE_LENGTH-1 );
    }

    params->set(CameraProperties::SUPPORTED_PREVIEW_FRAME_RATES, supported);
    return NO_ERROR;
}

status_t V4LCameraAdapter::insertCapabilities(CameraProperties::Properties* params, V4L_TI_CAPTYPE &caps)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    if ( NO_ERROR == ret ) {
        ret = insertPreviewFormats(params, caps);
    }

    if ( NO_ERROR == ret ) {
        ret = insertImageSizes(params, caps);
    }

    if ( NO_ERROR == ret ) {
        ret = insertPreviewSizes(params, caps);
    }

    if ( NO_ERROR == ret ) {
        ret = insertFrameRates(params, caps);
    }

    //Insert Supported Focus modes.
    params->set(CameraProperties::SUPPORTED_FOCUS_MODES, "infinity");

    params->set(CameraProperties::SUPPORTED_PICTURE_FORMATS, "jpeg");

    if ( NO_ERROR == ret ) {
        ret = insertDefaults(params, caps);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t V4LCameraAdapter::sortAscend(V4L_TI_CAPTYPE &caps, uint16_t count) {
    size_t tempRes;
    size_t w, h, tmpW,tmpH;
    for (int i=0; i<count; i++) {
        w = caps.tPreviewRes[i].width;
        h = caps.tPreviewRes[i].height;
        tempRes = w*h;
        for (int j=i+1; j<count; j++) {
            tmpW = caps.tPreviewRes[j].width;
            tmpH = caps.tPreviewRes[j].height;

            if (tempRes > (tmpW * tmpH) ) {
                caps.tPreviewRes[j].width = w;
                caps.tPreviewRes[j].height = h;
                w = tmpW;
                h = tmpH;
                }
            }
        caps.tPreviewRes[i].width = w;
        caps.tPreviewRes[i].height = h;

        }
    return NO_ERROR;
}

/*****************************************
 * public exposed function declarations
 *****************************************/

status_t V4LCameraAdapter::getCaps(const int sensorId, CameraProperties::Properties* params,
                                   V4L_HANDLETYPE handle) {
     status_t status = NO_ERROR;
     V4L_TI_CAPTYPE caps;
     int i = 0;
     int j = 0;
     struct v4l2_fmtdesc fmtDesc;
     struct v4l2_frmsizeenum frmSizeEnum;
     struct v4l2_frmivalenum frmIvalEnum;

    //get supported pixel formats
    for ( i = 0; status == NO_ERROR; i++) {
        fmtDesc.index = i;
        fmtDesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        status = ioctl (handle, VIDIOC_ENUM_FMT, &fmtDesc);
        if (status == NO_ERROR) {
            CAMHAL_LOGDB("fmtDesc[%d].description::pixelformat::flags== (%s::%d::%d)",i, fmtDesc.description,fmtDesc.pixelformat,fmtDesc.flags);
            caps.ePreviewFormats[i] = fmtDesc.pixelformat;
        }
    }
    caps.ulPreviewFormatCount = i;

    //get preview sizes & capture image sizes
    status = NO_ERROR;
    for ( i = 0; status == NO_ERROR; i++) {
        frmSizeEnum.index = i;
        //Check for frame sizes for default pixel format
        //TODO: Check for frame sizes for all supported pixel formats
        frmSizeEnum.pixel_format = V4L2_PIX_FMT_YUYV;
        status = ioctl (handle, VIDIOC_ENUM_FRAMESIZES, &frmSizeEnum);
        if (status == NO_ERROR) {
            int width;
            int height;

            if(frmSizeEnum.type != V4L2_FRMSIZE_TYPE_DISCRETE) {
                CAMHAL_LOGDB("\nfrmSizeEnum.type = %d", frmSizeEnum.type);
                CAMHAL_LOGDB("\nmin_width x height = %d x %d ",frmSizeEnum.stepwise.min_width, frmSizeEnum.stepwise.min_height);
                CAMHAL_LOGDB("\nmax_width x height = %d x %d ",frmSizeEnum.stepwise.max_width, frmSizeEnum.stepwise.max_height);
                CAMHAL_LOGDB("\nstep width x height = %d x %d ",frmSizeEnum.stepwise.step_width,frmSizeEnum.stepwise.step_height);
                //TODO: validate populating the sizes when type = V4L2_FRMSIZE_TYPE_STEPWISE
                width = frmSizeEnum.stepwise.max_width;
                height = frmSizeEnum.stepwise.max_height;
            }
            else {
                CAMHAL_LOGDB("frmSizeEnum.index[%d].width x height == (%d x %d)", i, frmSizeEnum.discrete.width, frmSizeEnum.discrete.height);
                width = frmSizeEnum.discrete.width;
                height = frmSizeEnum.discrete.height;
            }

            caps.tCaptureRes[i].width = width;
            caps.tCaptureRes[i].height = height;
            caps.tPreviewRes[i].width =  width;
            caps.tPreviewRes[i].height = height;

            snprintf(caps.tPreviewRes[i].param, MAX_RES_STRING_LENGTH,"%dx%d",caps.tPreviewRes[i].width,caps.tPreviewRes[i].height);
            snprintf(caps.tCaptureRes[i].param, MAX_RES_STRING_LENGTH,"%dx%d",caps.tCaptureRes[i].width,caps.tCaptureRes[i].height);
        }
        else {
            caps.ulCaptureResCount = i;
            caps.ulPreviewResCount = i;
        }
    }

    //sort the preview sizes in ascending order
    sortAscend(caps, caps.ulPreviewResCount);

    //get supported frame rates
    bool fps30 = false;
    for ( j=caps.ulPreviewResCount-1; j >= 0; j--) {
        CAMHAL_LOGDB(" W x H = %d x %d", caps.tPreviewRes[j].width, caps.tPreviewRes[j].height);
        status = NO_ERROR;
        for ( i = 0; status == NO_ERROR; i++) {
            frmIvalEnum.index = i;
            //Check for supported frame rates for the default pixel format.
            frmIvalEnum.pixel_format = V4L2_PIX_FMT_YUYV;
            frmIvalEnum.width = caps.tPreviewRes[j].width;
            frmIvalEnum.height = caps.tPreviewRes[j].height;

            status = ioctl (handle, VIDIOC_ENUM_FRAMEINTERVALS, &frmIvalEnum);
            if (status == NO_ERROR) {
                if(frmIvalEnum.type != V4L2_FRMIVAL_TYPE_DISCRETE) {
                    CAMHAL_LOGDB("frmIvalEnum[%d].type = %d)", i, frmIvalEnum.type);
                    CAMHAL_LOGDB("frmIvalEnum[%d].stepwise.min = %d/%d)", i, frmIvalEnum.stepwise.min.denominator, frmIvalEnum.stepwise.min.numerator);
                    CAMHAL_LOGDB("frmIvalEnum[%d].stepwise.max = %d/%d)", i, frmIvalEnum.stepwise.max.denominator, frmIvalEnum.stepwise.max.numerator);
                    CAMHAL_LOGDB("frmIvalEnum[%d].stepwise.step = %d/%d)", i, frmIvalEnum.stepwise.step.denominator, frmIvalEnum.stepwise.step.numerator);
                    caps.ulFrameRates[i] = (frmIvalEnum.stepwise.max.denominator/frmIvalEnum.stepwise.max.numerator);
                }
                else {
                    CAMHAL_LOGDB("frmIvalEnum[%d].frame rate= %d)",i, (frmIvalEnum.discrete.denominator/frmIvalEnum.discrete.numerator));
                    caps.ulFrameRates[i] = (frmIvalEnum.discrete.denominator/frmIvalEnum.discrete.numerator);
                }

                if (caps.ulFrameRates[i] == 30) {
                    fps30 = true;
                }
            }
            else if (i == 0) {
                // Framerate reporting is not guaranteed in V4L2 implementation.
                caps.ulFrameRates[i] = 30;
                fps30 = true;
                caps.ulFrameRateCount = 1;
            } else {
                CAMHAL_LOGE("caps.ulFrameRateCount = %d",i);
                caps.ulFrameRateCount = i;
            }
        }
        if(fps30) {
            break;
        }
    }

    if(frmIvalEnum.type != V4L2_FRMIVAL_TYPE_DISCRETE) {
        //TODO: populate the frame rates when type = V4L2_FRMIVAL_TYPE_STEPWISE;
    }

    //update the preview resolution with the highest resolution which supports 30fps.
/*  // for video preview the application choose the resolution from the mediaprofiles.xml.
    // so populating all supported preview resolution is required for video mode.
    caps.tPreviewRes[0].width = caps.tPreviewRes[j].width;
    caps.tPreviewRes[0].height = caps.tPreviewRes[j].height;
    snprintf(caps.tPreviewRes[0].param, MAX_RES_STRING_LENGTH,"%dx%d",caps.tPreviewRes[j].width,caps.tPreviewRes[j].height);
    caps.ulPreviewResCount = 1;
*/
    insertCapabilities (params, caps);
    return NO_ERROR;
}



} // namespace Camera
} // namespace Ti
