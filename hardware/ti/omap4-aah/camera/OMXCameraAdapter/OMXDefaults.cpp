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
* @file OMXDefaults.cpp
*
* This file contains definitions are OMX Camera defaults
*
*/

#include "CameraHal.h"
#include "OMXCameraAdapter.h"

namespace Ti {
namespace Camera {

#define __STRINGIFY(s) __STRING(s)

// OMX Camera defaults
const char OMXCameraAdapter::DEFAULT_ANTIBANDING[] = "auto";
const char OMXCameraAdapter::DEFAULT_BRIGHTNESS[] = "50";
const char OMXCameraAdapter::DEFAULT_CONTRAST[] = "100";
const char OMXCameraAdapter::DEFAULT_EFFECT[] = "none";
const char OMXCameraAdapter::DEFAULT_EV_COMPENSATION[] = "0";
const char OMXCameraAdapter::DEFAULT_EV_STEP[] = "0.1";
const char OMXCameraAdapter::DEFAULT_EXPOSURE_MODE[] = "auto";
const char OMXCameraAdapter::DEFAULT_FLASH_MODE[] = "off";
const char OMXCameraAdapter::DEFAULT_FOCUS_MODE_PREFERRED[] = "auto";
const char OMXCameraAdapter::DEFAULT_FOCUS_MODE[] = "infinity";
const char OMXCameraAdapter::DEFAULT_IPP[] = "ldc-nsf";
const char OMXCameraAdapter::DEFAULT_ISO_MODE[] = "auto";
const char OMXCameraAdapter::DEFAULT_JPEG_QUALITY[] = "95";
const char OMXCameraAdapter::DEFAULT_THUMBNAIL_QUALITY[] = "60";
const char OMXCameraAdapter::DEFAULT_THUMBNAIL_SIZE[] = "160x120";
const char OMXCameraAdapter::DEFAULT_PICTURE_FORMAT[] = "jpeg";
const char OMXCameraAdapter::DEFAULT_S3D_PICTURE_LAYOUT[] = "tb-full";
const char OMXCameraAdapter::DEFAULT_PICTURE_SIZE[] = "320x240";
const char OMXCameraAdapter::DEFAULT_PICTURE_SS_SIZE[] = "640x240";
const char OMXCameraAdapter::DEFAULT_PICTURE_TB_SIZE[] = "320x480";
const char OMXCameraAdapter::DEFAULT_PREVIEW_FORMAT[] = "yuv420sp";
const char OMXCameraAdapter::DEFAULT_FRAMERATE[] = "30";
const char OMXCameraAdapter::DEFAULT_S3D_PREVIEW_LAYOUT[] = "tb-subsampled";
const char OMXCameraAdapter::DEFAULT_PREVIEW_SIZE[] = "640x480";
const char OMXCameraAdapter::DEFAULT_PREVIEW_SS_SIZE[] = "1280x480";
const char OMXCameraAdapter::DEFAULT_PREVIEW_TB_SIZE[] = "640x960";
const char OMXCameraAdapter::DEFAULT_NUM_PREV_BUFS[] = "6";
const char OMXCameraAdapter::DEFAULT_NUM_PIC_BUFS[] = "1";
const char OMXCameraAdapter::DEFAULT_SATURATION[] = "100";
const char OMXCameraAdapter::DEFAULT_SCENE_MODE[] = "auto";
const char OMXCameraAdapter::DEFAULT_SHARPNESS[] = "100";
const char * OMXCameraAdapter::DEFAULT_VSTAB = android::CameraParameters::FALSE;
const char * OMXCameraAdapter::DEFAULT_VNF = android::CameraParameters::FALSE;
const char OMXCameraAdapter::DEFAULT_WB[] = "auto";
const char OMXCameraAdapter::DEFAULT_ZOOM[] = "0";
const char OMXCameraAdapter::DEFAULT_MAX_FD_HW_FACES[] = __STRINGIFY(MAX_NUM_FACES_SUPPORTED);
const char OMXCameraAdapter::DEFAULT_MAX_FD_SW_FACES[] = "0";
const char OMXCameraAdapter::DEFAULT_HOR_ANGLE[] = "54.8";
const char OMXCameraAdapter::DEFAULT_VER_ANGLE[] = "42.5";
const char * OMXCameraAdapter::DEFAULT_AE_LOCK = android::CameraParameters::FALSE;
const char * OMXCameraAdapter::DEFAULT_AWB_LOCK = android::CameraParameters::FALSE;
const char OMXCameraAdapter::DEFAULT_VIDEO_SIZE[] = "1920x1080";
const char OMXCameraAdapter::DEFAULT_SENSOR_ORIENTATION[] = "0";
const char OMXCameraAdapter::DEFAULT_AUTOCONVERGENCE_MODE[] = "frame";
const char OMXCameraAdapter::DEFAULT_MANUAL_CONVERGENCE[] = "0";
const char * OMXCameraAdapter::DEFAULT_MECHANICAL_MISALIGNMENT_CORRECTION_MODE = android::CameraParameters::TRUE;
const char OMXCameraAdapter::DEFAULT_EXIF_MAKE[] = "default_make";
const char OMXCameraAdapter::DEFAULT_EXIF_MODEL[] = "default_model";

} // namespace Camera
} // namespace Ti
