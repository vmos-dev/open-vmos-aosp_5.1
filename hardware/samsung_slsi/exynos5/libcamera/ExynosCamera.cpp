/*
 * Copyright 2008, The Android Open Source Project
 * Copyright 2010, Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed toggle an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*!
 * \file      ExynosCamera.cpp
 * \brief     source file for CAMERA HAL MODULE
 * \author    thun.hwang(thun.hwang@samsung.com)
 * \date      2010/06/03
 *
 * <b>Revision History: </b>
 * - 2011/12/31 : thun.hwang(thun.hwang@samsung.com) \n
 *   Initial version
 *
 * - 2012/01/18 : Sangwoo, Park(sw5771.park@samsung.com) \n
 *   Adjust Doxygen Document
 *
 * - 2012/02/01 : Sangwoo, Park(sw5771.park@samsung.com) \n
 *   Adjust libv4l2
 *   Adjust struct ExynosCameraInfo
 *   External ISP feature
 *
 * - 2012/03/14 : sangwoo.park(sw5771.park@samsung.com) \n
 *   Change file, class name to ExynosXXX.
 */

/**
 * @page ExynosCamera
 *
 * @section Introduction
 * ExynosCamera is for camera preview,takePicture and recording.
 * (Currently libseccamera is included in Android Camera HAL(libcamera.so).
 *
 * @section Copyright
 *  Copyright (c) 2008-2011 Samsung Electronics Co., Ltd.All rights reserved. \n
 *  Proprietary and Confidential
 *
 * @image html samsung.png
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "ExynosCamera"

/* FIXME: This define will be removed when functions are stable */
//#define USE_DIS
//#define USE_3DNR
//#define USE_ODC

#include <utils/Log.h>

#include "ExynosCamera.h"
#include "exynos_format.h"

using namespace android;

namespace android {

ExynosCameraInfo::ExynosCameraInfo()
{
    previewW = 2560;
    previewH = 1920;
    previewColorFormat = V4L2_PIX_FMT_NV21;
    videoW = 1920;
    videoH = 1080;
    prefVideoPreviewW = 640;
    prefVideoPreviewH = 360;
    videoColorFormat = V4L2_PIX_FMT_NV12M;
    pictureW = 2560;
    pictureH = 1920;
    pictureColorFormat = V4L2_PIX_FMT_YUYV;
    thumbnailW = 320;
    thumbnailH = 240;

    antiBandingList =
          ExynosCamera::ANTIBANDING_OFF
        | ExynosCamera::ANTIBANDING_50HZ
        | ExynosCamera::ANTIBANDING_60HZ
        | ExynosCamera::ANTIBANDING_OFF;
    antiBanding = ExynosCamera::ANTIBANDING_OFF;

    effectList =
          ExynosCamera::EFFECT_NONE
        | ExynosCamera::EFFECT_MONO
        | ExynosCamera::EFFECT_NEGATIVE
        | ExynosCamera::EFFECT_SOLARIZE
        | ExynosCamera::EFFECT_SEPIA
        | ExynosCamera::EFFECT_POSTERIZE
        | ExynosCamera::EFFECT_WHITEBOARD
        | ExynosCamera::EFFECT_BLACKBOARD
        | ExynosCamera::EFFECT_AQUA;
    effect = ExynosCamera::EFFECT_NONE;

    flashModeList =
          ExynosCamera::FLASH_MODE_OFF
        | ExynosCamera::FLASH_MODE_AUTO
        | ExynosCamera::FLASH_MODE_ON
        | ExynosCamera::FLASH_MODE_RED_EYE
        | ExynosCamera::FLASH_MODE_TORCH;
    flashMode = ExynosCamera::FLASH_MODE_OFF;

    focusModeList =
          ExynosCamera::FOCUS_MODE_AUTO
        | ExynosCamera::FOCUS_MODE_INFINITY
        | ExynosCamera::FOCUS_MODE_MACRO
        | ExynosCamera::FOCUS_MODE_FIXED
        | ExynosCamera::FOCUS_MODE_EDOF
        | ExynosCamera::FOCUS_MODE_CONTINUOUS_VIDEO
        | ExynosCamera::FOCUS_MODE_CONTINUOUS_PICTURE
        | ExynosCamera::FOCUS_MODE_TOUCH;
    focusMode = ExynosCamera::FOCUS_MODE_AUTO;

    sceneModeList =
          ExynosCamera::SCENE_MODE_AUTO
        | ExynosCamera::SCENE_MODE_ACTION
        | ExynosCamera::SCENE_MODE_PORTRAIT
        | ExynosCamera::SCENE_MODE_LANDSCAPE
        | ExynosCamera::SCENE_MODE_NIGHT
        | ExynosCamera::SCENE_MODE_NIGHT_PORTRAIT
        | ExynosCamera::SCENE_MODE_THEATRE
        | ExynosCamera::SCENE_MODE_BEACH
        | ExynosCamera::SCENE_MODE_SNOW
        | ExynosCamera::SCENE_MODE_SUNSET
        | ExynosCamera::SCENE_MODE_STEADYPHOTO
        | ExynosCamera::SCENE_MODE_FIREWORKS
        | ExynosCamera::SCENE_MODE_SPORTS
        | ExynosCamera::SCENE_MODE_PARTY
        | ExynosCamera::SCENE_MODE_CANDLELIGHT;
    sceneMode = ExynosCamera::SCENE_MODE_AUTO;

    whiteBalanceList =
          ExynosCamera::WHITE_BALANCE_AUTO
        | ExynosCamera::WHITE_BALANCE_INCANDESCENT
        | ExynosCamera::WHITE_BALANCE_FLUORESCENT
        | ExynosCamera::WHITE_BALANCE_WARM_FLUORESCENT
        | ExynosCamera::WHITE_BALANCE_DAYLIGHT
        | ExynosCamera::WHITE_BALANCE_CLOUDY_DAYLIGHT
        | ExynosCamera::WHITE_BALANCE_TWILIGHT
        | ExynosCamera::WHITE_BALANCE_SHADE;
    whiteBalance = ExynosCamera::WHITE_BALANCE_AUTO;

    autoWhiteBalanceLockSupported = false;
    autoWhiteBalanceLock = false;

    rotation = 0;
    minExposure = -2;
    maxExposure = 2;
    exposure = 0;

    autoExposureLockSupported = false;
    autoExposureLock = false;

    fps = 30;
    focalLengthNum = 9;
    focalLengthDen = 10;
    supportVideoStabilization = false;
    applyVideoStabilization = false;
    videoStabilization = false;
    maxNumMeteringAreas = 0;
    maxNumDetectedFaces = 0;
    maxNumFocusAreas = 0;
    maxZoom = ZOOM_LEVEL_MAX;
    hwZoomSupported = false;
    zoom = 0;
    gpsAltitude = 0;
    gpsLatitude = 0;
    gpsLongitude = 0;
    gpsTimestamp = 0;

    // Additional API default Value.
    angle = 0;
    antiShake = false;
    beautyShot = false;
    brightness = 0;
    contrast = ExynosCamera::CONTRAST_DEFAULT;
    gamma = false;
    hue = 2; // 2 is default;
    iso = 0;
    metering = ExynosCamera::METERING_MODE_CENTER;
    objectTracking = false;
    objectTrackingStart = false;
    saturation = 0;
    sharpness = 0;
    shotMode = ExynosCamera::SHOT_MODE_SINGLE;
    slowAE = false;
    smartAuto = false;
    touchAfStart = false;
    wdr = false;
    tdnr = false;
    odc = false;
}

ExynosCameraInfoM5M0::ExynosCameraInfoM5M0()
{
    previewW = 1280;
    previewH = 720;
    previewColorFormat = V4L2_PIX_FMT_YVU420M;
    videoW = 1280;
    videoH = 720;
    prefVideoPreviewW = 640;
    prefVideoPreviewH = 360;
    videoColorFormat = V4L2_PIX_FMT_NV12M;
    pictureW = 1280;
    pictureH = 720;
    pictureColorFormat = V4L2_PIX_FMT_YUYV;
    thumbnailW = 320;
    thumbnailH = 240;

    antiBandingList = ExynosCamera::ANTIBANDING_OFF;
    antiBanding = ExynosCamera::ANTIBANDING_OFF;

    effectList =
          ExynosCamera::EFFECT_NONE
        | ExynosCamera::EFFECT_MONO
        | ExynosCamera::EFFECT_NEGATIVE
        //| ExynosCamera::EFFECT_SOLARIZE
        | ExynosCamera::EFFECT_SEPIA
        //| ExynosCamera::EFFECT_POSTERIZE
        //| ExynosCamera::EFFECT_WHITEBOARD
        //| ExynosCamera::EFFECT_BLACKBOARD
        | ExynosCamera::EFFECT_AQUA;
    effect = ExynosCamera::EFFECT_NONE;

    flashModeList =
          ExynosCamera::FLASH_MODE_OFF
        | ExynosCamera::FLASH_MODE_AUTO
        | ExynosCamera::FLASH_MODE_ON
        | ExynosCamera::FLASH_MODE_RED_EYE
        | ExynosCamera::FLASH_MODE_TORCH;
    flashMode = ExynosCamera::FLASH_MODE_OFF;

    focusModeList =
          ExynosCamera::FOCUS_MODE_AUTO
        | ExynosCamera::FOCUS_MODE_INFINITY
        | ExynosCamera::FOCUS_MODE_MACRO
        //| ExynosCamera::FOCUS_MODE_FIXED
        //| ExynosCamera::FOCUS_MODE_EDOF
        //| ExynosCamera::FOCUS_MODE_CONTINUOUS_VIDEO
        //| ExynosCamera::FOCUS_MODE_CONTINUOUS_PICTURE
        //| ExynosCamera::FOCUS_MODE_TOUCH
        ;
    focusMode = ExynosCamera::FOCUS_MODE_AUTO;

    sceneModeList =
          ExynosCamera::SCENE_MODE_AUTO
        //| ExynosCamera::SCENE_MODE_ACTION
        | ExynosCamera::SCENE_MODE_PORTRAIT
        | ExynosCamera::SCENE_MODE_LANDSCAPE
        | ExynosCamera::SCENE_MODE_NIGHT
        //| ExynosCamera::SCENE_MODE_NIGHT_PORTRAIT
        //| ExynosCamera::SCENE_MODE_THEATRE
        | ExynosCamera::SCENE_MODE_BEACH
        | ExynosCamera::SCENE_MODE_SNOW
        | ExynosCamera::SCENE_MODE_SUNSET
        //| ExynosCamera::SCENE_MODE_STEADYPHOTO
        | ExynosCamera::SCENE_MODE_FIREWORKS
        | ExynosCamera::SCENE_MODE_SPORTS
        | ExynosCamera::SCENE_MODE_PARTY
        | ExynosCamera::SCENE_MODE_CANDLELIGHT;
    sceneMode = ExynosCamera::SCENE_MODE_AUTO;

    whiteBalanceList =
          ExynosCamera::WHITE_BALANCE_AUTO
        | ExynosCamera::WHITE_BALANCE_INCANDESCENT
        | ExynosCamera::WHITE_BALANCE_FLUORESCENT
        //| ExynosCamera::WHITE_BALANCE_WARM_FLUORESCENT
        | ExynosCamera::WHITE_BALANCE_DAYLIGHT
        | ExynosCamera::WHITE_BALANCE_CLOUDY_DAYLIGHT
        //| ExynosCamera::WHITE_BALANCE_TWILIGHT
        //| ExynosCamera::WHITE_BALANCE_SHADE
        ;
    whiteBalance = ExynosCamera::WHITE_BALANCE_AUTO;

    autoWhiteBalanceLockSupported = false;
    autoWhiteBalanceLock = false;

    rotation = 0;
    minExposure = -2;
    maxExposure = 2;
    exposure = 0;

    autoExposureLockSupported = false;
    autoExposureLock = false;

    fps = 30;
    focalLengthNum = 343;
    focalLengthDen = 100;
    supportVideoStabilization = false;
    applyVideoStabilization = false;
    videoStabilization = false;
    maxNumMeteringAreas = 64;
    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 2;
    maxZoom = ZOOM_LEVEL_MAX;
    hwZoomSupported = false;
    zoom = 0;
    gpsAltitude = 0;
    gpsLatitude = 0;
    gpsLongitude = 0;
    gpsTimestamp = 0;
}

ExynosCameraInfoS5K6A3::ExynosCameraInfoS5K6A3()
{
    previewW = 1280;
    previewH =  720;
    previewColorFormat = V4L2_PIX_FMT_YVU420M;
    videoW = 1280;
    videoH =  720;
    prefVideoPreviewW = 640;
    prefVideoPreviewH = 360;
    videoColorFormat = V4L2_PIX_FMT_NV12M;
    pictureW = 1280;
    pictureH =  720;
    pictureColorFormat = V4L2_PIX_FMT_YUYV;
    thumbnailW = 320;
    thumbnailH = 240;

    antiBandingList =
          ExynosCamera::ANTIBANDING_OFF
        | ExynosCamera::ANTIBANDING_50HZ
        | ExynosCamera::ANTIBANDING_60HZ
        | ExynosCamera::ANTIBANDING_OFF;
    antiBanding = ExynosCamera::ANTIBANDING_OFF;

    effectList =
          ExynosCamera::EFFECT_NONE
        | ExynosCamera::EFFECT_MONO
        | ExynosCamera::EFFECT_NEGATIVE
        //| ExynosCamera::EFFECT_SOLARIZE
        | ExynosCamera::EFFECT_SEPIA
        //| ExynosCamera::EFFECT_POSTERIZE
        //| ExynosCamera::EFFECT_WHITEBOARD
        //| ExynosCamera::EFFECT_BLACKBOARD
        //| ExynosCamera::EFFECT_AQUA
        ;
    effect = ExynosCamera::EFFECT_NONE;

    flashModeList =
          ExynosCamera::FLASH_MODE_OFF
        //| ExynosCamera::FLASH_MODE_AUTO
        //| ExynosCamera::FLASH_MODE_ON
        //| ExynosCamera::FLASH_MODE_RED_EYE
        //| ExynosCamera::FLASH_MODE_TORCH
        ;
    flashMode = ExynosCamera::FLASH_MODE_OFF;

    focusModeList =
        //  ExynosCamera::FOCUS_MODE_AUTO
        //| ExynosCamera::FOCUS_MODE_INFINITY
        //| ExynosCamera::FOCUS_MODE_MACRO
        //|
        ExynosCamera::FOCUS_MODE_FIXED
        //| ExynosCamera::FOCUS_MODE_EDOF
        //| ExynosCamera::FOCUS_MODE_CONTINUOUS_VIDEO
        //| ExynosCamera::FOCUS_MODE_CONTINUOUS_PICTURE
        //| ExynosCamera::FOCUS_MODE_TOUCH
        ;
    focusMode = ExynosCamera::FOCUS_MODE_FIXED;

    sceneModeList =
          ExynosCamera::SCENE_MODE_AUTO
        //| ExynosCamera::SCENE_MODE_ACTION
        | ExynosCamera::SCENE_MODE_PORTRAIT
        | ExynosCamera::SCENE_MODE_LANDSCAPE
        | ExynosCamera::SCENE_MODE_NIGHT
        //| ExynosCamera::SCENE_MODE_NIGHT_PORTRAIT
        //| ExynosCamera::SCENE_MODE_THEATRE
        | ExynosCamera::SCENE_MODE_BEACH
        | ExynosCamera::SCENE_MODE_SNOW
        | ExynosCamera::SCENE_MODE_SUNSET
        | ExynosCamera::SCENE_MODE_STEADYPHOTO
        | ExynosCamera::SCENE_MODE_FIREWORKS
        | ExynosCamera::SCENE_MODE_SPORTS
        | ExynosCamera::SCENE_MODE_PARTY
        | ExynosCamera::SCENE_MODE_CANDLELIGHT;
    sceneMode = ExynosCamera::SCENE_MODE_AUTO;

    whiteBalanceList =
          ExynosCamera::WHITE_BALANCE_AUTO
        | ExynosCamera::WHITE_BALANCE_INCANDESCENT
        | ExynosCamera::WHITE_BALANCE_FLUORESCENT
        //| ExynosCamera::WHITE_BALANCE_WARM_FLUORESCENT
        | ExynosCamera::WHITE_BALANCE_DAYLIGHT
        | ExynosCamera::WHITE_BALANCE_CLOUDY_DAYLIGHT
        //| ExynosCamera::WHITE_BALANCE_TWILIGHT
        //| ExynosCamera::WHITE_BALANCE_SHADE
        ;
    whiteBalance = ExynosCamera::WHITE_BALANCE_AUTO;

    autoWhiteBalanceLockSupported = true;
    autoWhiteBalanceLock = false;

    rotation = 0;
    minExposure = -2;
    maxExposure = 2;
    exposure = 0;

    autoExposureLockSupported = true;
    autoExposureLock = false;

    fps = 30;
    focalLengthNum = 9;
    focalLengthDen = 10;
    supportVideoStabilization = false;
    applyVideoStabilization = false;
    videoStabilization = false;
    maxNumMeteringAreas = 64;
    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 0;
    maxZoom = ZOOM_LEVEL_MAX;
    hwZoomSupported = false;
    zoom = 0;
    gpsAltitude = 0;
    gpsLatitude = 0;
    gpsLongitude = 0;
    gpsTimestamp = 0;
}

ExynosCameraInfoS5K4E5::ExynosCameraInfoS5K4E5()
{
    previewW = 1920;
    previewH = 1080;
    previewColorFormat = V4L2_PIX_FMT_YVU420M;
    videoW = 1920;
    videoH = 1080;
    prefVideoPreviewW = 640;
    prefVideoPreviewH = 360;
    videoColorFormat = V4L2_PIX_FMT_NV12M;
    pictureW = 2560;
    pictureH = 1920;
    pictureColorFormat = V4L2_PIX_FMT_YUYV;
    thumbnailW = 320;
    thumbnailH = 240;

    antiBandingList =
          ExynosCamera::ANTIBANDING_OFF
        | ExynosCamera::ANTIBANDING_50HZ
        | ExynosCamera::ANTIBANDING_60HZ
        | ExynosCamera::ANTIBANDING_OFF;
    antiBanding = ExynosCamera::ANTIBANDING_OFF;

    effectList =
          ExynosCamera::EFFECT_NONE
        | ExynosCamera::EFFECT_MONO
        | ExynosCamera::EFFECT_NEGATIVE
        //| ExynosCamera::EFFECT_SOLARIZE
        | ExynosCamera::EFFECT_SEPIA
        //| ExynosCamera::EFFECT_POSTERIZE
        //| ExynosCamera::EFFECT_WHITEBOARD
        //| ExynosCamera::EFFECT_BLACKBOARD
        //| ExynosCamera::EFFECT_AQUA
        ;
    effect = ExynosCamera::EFFECT_NONE;

    flashModeList =
          ExynosCamera::FLASH_MODE_OFF
        | ExynosCamera::FLASH_MODE_AUTO
        | ExynosCamera::FLASH_MODE_ON
        //| ExynosCamera::FLASH_MODE_RED_EYE
        | ExynosCamera::FLASH_MODE_TORCH;
    flashMode = ExynosCamera::FLASH_MODE_OFF;

    focusModeList =
          ExynosCamera::FOCUS_MODE_AUTO
        | ExynosCamera::FOCUS_MODE_INFINITY
        | ExynosCamera::FOCUS_MODE_MACRO
        //| ExynosCamera::FOCUS_MODE_FIXED
        //| ExynosCamera::FOCUS_MODE_EDOF
        | ExynosCamera::FOCUS_MODE_CONTINUOUS_VIDEO
    //    | ExynosCamera::FOCUS_MODE_CONTINUOUS_PICTURE
        | ExynosCamera::FOCUS_MODE_TOUCH
        ;
    focusMode = ExynosCamera::FOCUS_MODE_AUTO;

    sceneModeList =
          ExynosCamera::SCENE_MODE_AUTO
        //| ExynosCamera::SCENE_MODE_ACTION
        | ExynosCamera::SCENE_MODE_PORTRAIT
        | ExynosCamera::SCENE_MODE_LANDSCAPE
        | ExynosCamera::SCENE_MODE_NIGHT
        //| ExynosCamera::SCENE_MODE_NIGHT_PORTRAIT
        //| ExynosCamera::SCENE_MODE_THEATRE
        | ExynosCamera::SCENE_MODE_BEACH
        | ExynosCamera::SCENE_MODE_SNOW
        | ExynosCamera::SCENE_MODE_SUNSET
        | ExynosCamera::SCENE_MODE_STEADYPHOTO
        | ExynosCamera::SCENE_MODE_FIREWORKS
        | ExynosCamera::SCENE_MODE_SPORTS
        | ExynosCamera::SCENE_MODE_PARTY
        | ExynosCamera::SCENE_MODE_CANDLELIGHT;
    sceneMode = ExynosCamera::SCENE_MODE_AUTO;

    whiteBalanceList =
          ExynosCamera::WHITE_BALANCE_AUTO
        | ExynosCamera::WHITE_BALANCE_INCANDESCENT
        | ExynosCamera::WHITE_BALANCE_FLUORESCENT
        //| ExynosCamera::WHITE_BALANCE_WARM_FLUORESCENT
        | ExynosCamera::WHITE_BALANCE_DAYLIGHT
        | ExynosCamera::WHITE_BALANCE_CLOUDY_DAYLIGHT
        //| ExynosCamera::WHITE_BALANCE_TWILIGHT
        //| ExynosCamera::WHITE_BALANCE_SHADE
        ;
    whiteBalance = ExynosCamera::WHITE_BALANCE_AUTO;

    autoWhiteBalanceLockSupported = true;
    autoWhiteBalanceLock = false;

    rotation = 0;
    minExposure = -2;
    maxExposure = 2;
    exposure = 0;

    autoExposureLockSupported = true;
    autoExposureLock = false;

    fps = 30;
    focalLengthNum = 9;
    focalLengthDen = 10;
    supportVideoStabilization = true;
    applyVideoStabilization = false;
    videoStabilization = false;
    maxNumMeteringAreas = 64;
    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 2;
    maxZoom = ZOOM_LEVEL_MAX;
    hwZoomSupported = false;
    zoom = 0;
    gpsAltitude = 0;
    gpsLatitude = 0;
    gpsLongitude = 0;
    gpsTimestamp = 0;
}

//////////////////////////////////////////////////

#define PFX_NODE                            "/dev/video"

#define M5MOLS_ENTITY_NAME                  "M5MOLS 5-001f"
#define PFX_SUBDEV_ENTITY_MIPI_CSIS         "s5p-mipi-csis"
#define PFX_SUBDEV_ENTITY_FLITE             "flite-subdev"
#define PFX_SUBDEV_ENTITY_GSC_CAP           "gsc-cap-subdev"
#define PFX_VIDEODEV_ENTITY_FLITE           "exynos-fimc-lite"
#define PFX_VIDEODEV_ENTITY_GSC_CAP         "exynos-gsc"

#define MEDIA_DEV_INTERNAL_ISP              "/dev/media2"
#define MEDIA_DEV_EXTERNAL_ISP              "/dev/media1"
#define ISP_VD_NODE_OFFSET                  (40)              //INTERNAL_ISP
#define FLITE_VD_NODE_OFFSET                (36)              //External ISP

#define VIDEO_NODE_PREVIEW_ID               (3)
#define VIDEO_NODE_RECODING_ID              (2)
#define VIDEO_NODE_SNAPSHOT_ID              (1)

#define ISP_SENSOR_MAX_ENTITIES             1
#define ISP_SENSOR_PAD_SOURCE_FRONT         0
#define ISP_SENSOR_PADS_NUM                 1

#define ISP_FRONT_MAX_ENTITIES              1
#define ISP_FRONT_PAD_SINK                  0
#define ISP_FRONT_PAD_SOURCE_BACK           1
#define ISP_FRONT_PAD_SOURCE_BAYER          2
#define ISP_FRONT_PAD_SOURCE_SCALERC        3
#define ISP_FRONT_PADS_NUM                  4

#define ISP_BACK_MAX_ENTITIES               1
#define ISP_BACK_PAD_SINK                   0
#define ISP_BACK_PAD_SOURCE_3DNR            1
#define ISP_BACK_PAD_SOURCE_SCALERP         2
#define ISP_BACK_PADS_NUM                   3

#define ISP_MODULE_NAME                     "exynos5-fimc-is"
#define ISP_SENSOR_ENTITY_NAME              "exynos5-fimc-is-sensor"
#define ISP_FRONT_ENTITY_NAME               "exynos5-fimc-is-front"
#define ISP_BACK_ENTITY_NAME                "exynos5-fimc-is-back"
#define ISP_VIDEO_BAYER_NAME                "exynos5-fimc-is-bayer"
#define ISP_VIDEO_SCALERC_NAME              "exynos5-fimc-is-scalerc"
#define ISP_VIDEO_3DNR_NAME                 "exynos5-fimc-is-3dnr"
#define ISP_VIDEO_SCALERP_NAME              "exynos5-fimc-is-scalerp"

#define MIPI_NUM                            1
#define FLITE_NUM                           1
#define GSC_NUM                             0

#define PFX_SUBDEV_NODE                     "/dev/v4l-subdev"

/*
 * V 4 L 2   F I M C   E X T E N S I O N S
 *
 */
#define V4L2_CID_ROTATION                   (V4L2_CID_PRIVATE_BASE + 0)
#define V4L2_CID_PADDR_Y                    (V4L2_CID_PRIVATE_BASE + 1)
#define V4L2_CID_PADDR_CB                   (V4L2_CID_PRIVATE_BASE + 2)
#define V4L2_CID_PADDR_CR                   (V4L2_CID_PRIVATE_BASE + 3)
#define V4L2_CID_PADDR_CBCR                 (V4L2_CID_PRIVATE_BASE + 4)
#define V4L2_CID_STREAM_PAUSE               (V4L2_CID_PRIVATE_BASE + 53)

#define V4L2_CID_CAM_JPEG_MAIN_SIZE         (V4L2_CID_PRIVATE_BASE + 32)
#define V4L2_CID_CAM_JPEG_MAIN_OFFSET       (V4L2_CID_PRIVATE_BASE + 33)
#define V4L2_CID_CAM_JPEG_THUMB_SIZE        (V4L2_CID_PRIVATE_BASE + 34)
#define V4L2_CID_CAM_JPEG_THUMB_OFFSET      (V4L2_CID_PRIVATE_BASE + 35)
#define V4L2_CID_CAM_JPEG_POSTVIEW_OFFSET   (V4L2_CID_PRIVATE_BASE + 36)
#define V4L2_CID_CAM_JPEG_QUALITY           (V4L2_CID_PRIVATE_BASE + 37)

#define V4L2_PIX_FMT_YVYU           v4l2_fourcc('Y', 'V', 'Y', 'U')

/* FOURCC for FIMC specific */
#define V4L2_PIX_FMT_VYUY           v4l2_fourcc('V', 'Y', 'U', 'Y')
#define V4L2_PIX_FMT_NV16           v4l2_fourcc('N', 'V', '1', '6')
#define V4L2_PIX_FMT_NV61           v4l2_fourcc('N', 'V', '6', '1')
#define V4L2_PIX_FMT_NV12T          v4l2_fourcc('T', 'V', '1', '2')

///////////////////////////////////////////////////
// Google Official API : Camera.Parameters
// http://developer.android.com/reference/android/hardware/Camera.Parameters.html
///////////////////////////////////////////////////

ExynosCamera::ExynosCamera() :
        m_flagCreate(false),
        m_cameraId(CAMERA_ID_BACK),
        m_defaultCameraInfo(NULL),
        m_curCameraInfo(NULL),
        m_jpegQuality(100),
        m_jpegThumbnailQuality(100),
        m_currentZoom(-1)
{
    memset(&m_sensorDev, 0, sizeof(struct devInfo));
    memset(&m_mipiDev, 0, sizeof(struct devInfo));
    memset(&m_fliteDev, 0, sizeof(struct devInfo));
    memset(&m_gscPreviewDev, 0, sizeof(struct devInfo));
    memset(&m_gscVideoDev, 0, sizeof(struct devInfo));
    memset(&m_gscPictureDev, 0, sizeof(struct devInfo));

    m_previewDev = NULL;
    m_videoDev   = NULL;
    m_pictureDev = NULL;

    m_tryPreviewStop = true;
    m_tryVideoStop   = true;
    m_tryPictureStop = true;

    m_flagStartFaceDetection = false;
    m_flagAutoFocusRunning = false;

    m_sensorEntity = NULL;
    m_mipiEntity = NULL;
    m_fliteSdEntity = NULL;
    m_fliteVdEntity = NULL;
    m_gscSdEntity = NULL;
    m_gscVdEntity = NULL;
    m_ispSensorEntity = NULL;
    m_ispFrontEntity = NULL;
    m_ispBackEntity = NULL;
    m_ispScalercEntity = NULL;
    m_ispScalerpEntity = NULL;
    m_isp3dnrEntity = NULL;


    for (int i = 0; i < VIDEO_MAX_FRAME; i++) {
        m_validPreviewBuf[i] = false;
        m_validVideoBuf  [i] = false;
        m_validPictureBuf[i] = false;
    }

    memset((void *)m_cameraName, 0, 32);

    m_internalISP = true;
    m_media = NULL;

    memset(&mExifInfo, 0, sizeof(mExifInfo));
}

ExynosCamera::~ExynosCamera()
{
    if (m_flagCreate == true)
        destroy();
}

bool ExynosCamera::create(int cameraId)
{
    int ret = 0;
    unsigned int i;
    int devNum;
    char node[30];

    struct media_link   *links = NULL;

    if (m_flagCreate == true) {
        ALOGE("ERR(%s):Already created", __func__);
        return false;
    }

    m_cameraId = cameraId;

    ExynosBuffer nullBuf;

    for (int i = 0; i < VIDEO_MAX_FRAME; i++) {
        m_validPreviewBuf[i] = false;
        m_validVideoBuf  [i] = false;
        m_validPictureBuf[i] = false;

        m_previewBuf[i] = nullBuf;
        m_videoBuf[i]   = nullBuf;
        m_pictureBuf[i] = nullBuf;
    }

    if (m_cameraId == CAMERA_ID_BACK)
        m_internalISP = true;
        // m_internalISP = false; // external ISP.
    else
        m_internalISP = true;

    if (m_internalISP == true) {
        //////////////////////////////
        //  internal ISP
        //////////////////////////////
        // media device open
        m_media = exynos_media_open(MEDIA_DEV_INTERNAL_ISP);
        if (m_media == NULL) {
            ALOGE("ERR(%s):Cannot open media device (error : %s)", __func__, strerror(errno));
            goto err;
        }

        //////////////////
        // GET ENTITIES
        //////////////////
        // ISP sensor subdev
        memset(&node, 0x00, sizeof(node));
        strcpy(node, ISP_SENSOR_ENTITY_NAME);
        m_ispSensorEntity = exynos_media_get_entity_by_name(m_media, node, strlen(node));

        // ISP front subdev
        memset(&node, 0x00, sizeof(node));
        strcpy(node, ISP_FRONT_ENTITY_NAME);
        m_ispFrontEntity = exynos_media_get_entity_by_name(m_media, node, strlen(node));

        // ISP back subdev
        memset(&node, 0x00, sizeof(node));
        strcpy(node, ISP_BACK_ENTITY_NAME);
        m_ispBackEntity = exynos_media_get_entity_by_name(m_media, node, strlen(node));

        // ISP ScalerC video node
        memset(&node, 0x00, sizeof(node));
        strcpy(node, ISP_VIDEO_SCALERC_NAME);
        m_ispScalercEntity = exynos_media_get_entity_by_name(m_media, node, strlen(node));

        // ISP ScalerP video node
        memset(&node, 0x00, sizeof(node));
        strcpy(node, ISP_VIDEO_SCALERP_NAME);
        m_ispScalerpEntity = exynos_media_get_entity_by_name(m_media, node, strlen(node));

        // ISP 3DNR video node
        memset(&node, 0x00, sizeof(node));
        strcpy(node, ISP_VIDEO_3DNR_NAME);
        m_isp3dnrEntity = exynos_media_get_entity_by_name(m_media, node, strlen(node));

        ALOGV("DEBUG(%s):m_ispSensorEntity  : numlink : %d", __func__, m_ispSensorEntity->num_links);
        ALOGV("DEBUG(%s):m_ispFrontEntity   : numlink : %d", __func__, m_ispFrontEntity->num_links);
        ALOGV("DEBUG(%s):m_ispBackEntity    : numlink : %d", __func__, m_ispBackEntity->num_links);
        ALOGV("DEBUG(%s):m_ispScalercEntity : numlink : %d", __func__, m_ispScalercEntity->num_links);
        ALOGV("DEBUG(%s):m_ispScalerpEntity : numlink : %d", __func__, m_ispScalerpEntity->num_links);
        ALOGV("DEBUG(%s):m_isp3dnrEntity    : numlink : %d", __func__, m_isp3dnrEntity->num_links);

        //////////////////
        // SETUP LINKS
        //////////////////
        // SENSOR TO FRONT
        links = m_ispSensorEntity->links;
        if (links == NULL ||
            links->source->entity != m_ispSensorEntity ||
            links->sink->entity != m_ispFrontEntity) {
            ALOGE("ERR(%s):Can not make link isp_sensor to isp_front", __func__);
            goto err;
        } else if (exynos_media_setup_link(m_media, links->source, links->sink, MEDIA_LNK_FL_ENABLED) < 0) {
            ALOGE("ERR(%s):Can not make setup isp_sensor to isp_front", __func__);
            goto err;
        }
        ALOGV("DEBUG(%s):[LINK SUCCESS] Sensor to front", __func__);

        // FRONT TO BACK
        for (i = 0; i < m_ispFrontEntity->num_links; i++) {
            links = &m_ispFrontEntity->links[i];
            if (links == NULL ||
                links->source->entity != m_ispFrontEntity ||
                links->sink->entity != m_ispBackEntity) {
                ALOGV("DEBUG(%s):i=%d: links->source->entity : %p, m_ispFrontEntity : %p", __func__, i,
                    links->source->entity, m_ispFrontEntity);
                ALOGV("DEBUG(%s):i=%d: links->sink->entity : %p, m_ispBackEntity : %p", __func__, i,
                    links->sink->entity, m_ispBackEntity);
                continue;
            } else if (exynos_media_setup_link(m_media, links->source, links->sink, MEDIA_LNK_FL_ENABLED) < 0) {
                ALOGE("ERR(%s):Can not make setup isp_front to isp_back", __func__);
                goto err;
            }
        }
        ALOGV("DEBUG(%s):[LINK SUCCESS] front to back", __func__);

        // BACK TO ScalerP Video
        for (i = 0; i < m_ispBackEntity->num_links; i++) {
            links = &m_ispBackEntity->links[i];
            if (links == NULL ||
                links->source->entity != m_ispBackEntity ||
                links->sink->entity != m_ispScalerpEntity) {
                ALOGV("DEBUG(%s):i=%d: links->source->entity : %p, m_ispBackEntity : %p", __func__, i,
                    links->source->entity, m_ispBackEntity);
                ALOGV("DEBUG(%s):i=%d: links->sink->entity : %p, m_ispScalerpEntity : %p", __func__, i,
                    links->sink->entity, m_ispScalerpEntity);
                continue;
            } else if (exynos_media_setup_link(m_media, links->source, links->sink, MEDIA_LNK_FL_ENABLED) < 0) {
                ALOGE("ERR(%s):Can not make setup isp_back to scalerP", __func__);
                goto err;
            }
        }
        ALOGV("DEBUG(%s):[LINK SUCCESS] back to scalerP", __func__);

        sprintf(node, "%s%d", PFX_NODE, (ISP_VD_NODE_OFFSET + VIDEO_NODE_PREVIEW_ID));
        m_gscPreviewDev.fd = exynos_v4l2_open(node, O_RDWR, 0);
        if (m_gscPreviewDev.fd <= 0) {
            ALOGE("ERR(%s):exynos_v4l2_open(%s) fail (error : %s)", __func__, node, strerror(errno));
            goto err;
        }
        m_previewDev = &m_gscPreviewDev;

        sprintf(node, "%s%d", PFX_NODE, (ISP_VD_NODE_OFFSET + VIDEO_NODE_RECODING_ID));
        m_gscVideoDev.fd = exynos_v4l2_open(node, O_RDWR, 0);
        if (m_gscVideoDev.fd <= 0) {
            ALOGE("ERR(%s):exynos_v4l2_open(%s) fail (error : %s)", __func__, node, strerror(errno));
            goto err;
        }
        m_videoDev = &m_gscVideoDev;

        sprintf(node, "%s%d", PFX_NODE, (ISP_VD_NODE_OFFSET + VIDEO_NODE_SNAPSHOT_ID));
        m_gscPictureDev.fd = exynos_v4l2_open(node, O_RDWR, 0);
        if (m_gscPictureDev.fd <= 0) {
            ALOGE("ERR(%s):exynos_v4l2_open(%s) fail (error : %s)", __func__, node, strerror(errno));
            goto err;
        }
        m_pictureDev = &m_gscPictureDev;

    } else {
        //////////////////////////////
        //  external ISP
        //////////////////////////////
        // media device open
        m_media = exynos_media_open(MEDIA_DEV_EXTERNAL_ISP);
        if (m_media == NULL) {
            ALOGE("ERR(%s):Cannot open media device (error : %s)", __func__, strerror(errno));
            goto err;
        }

        //////////////////
        // GET ENTITIES
        //////////////////
        // camera subdev
        strcpy(node, M5MOLS_ENTITY_NAME);
        ALOGV("DEBUG(%s):node : %s", __func__, node);
        m_sensorEntity = exynos_media_get_entity_by_name(m_media, node, strlen(node));
        ALOGV("DEBUG(%s):m_sensorEntity : 0x%p", __func__, m_sensorEntity);

        // mipi subdev
        sprintf(node, "%s.%d", PFX_SUBDEV_ENTITY_MIPI_CSIS, MIPI_NUM);
        ALOGV("DEBUG(%s):node : %s", __func__, node);
        m_mipiEntity = exynos_media_get_entity_by_name(m_media, node, strlen(node));
        ALOGV("DEBUG(%s):m_mipiEntity : 0x%p", __func__, m_mipiEntity);

        // fimc-lite subdev
        sprintf(node, "%s.%d", PFX_SUBDEV_ENTITY_FLITE, FLITE_NUM);
        ALOGV("DEBUG(%s):node : %s", __func__, node);
        m_fliteSdEntity = exynos_media_get_entity_by_name(m_media, node, strlen(node));
        ALOGV("DEBUG(%s):m_fliteSdEntity : 0x%p", __func__, m_fliteSdEntity);

        // fimc-lite videodev
        sprintf(node, "%s.%d", PFX_VIDEODEV_ENTITY_FLITE, FLITE_NUM);
        ALOGV("DEBUG(%s):node : %s", __func__, node);
        m_fliteVdEntity = exynos_media_get_entity_by_name(m_media, node, strlen(node));
        ALOGV("DEBUG(%s):m_fliteVdEntity : 0x%p", __func__, m_fliteVdEntity);

        // gscaler subdev
        sprintf(node, "%s.%d", PFX_SUBDEV_ENTITY_GSC_CAP, GSC_NUM);
        ALOGV("DEBUG(%s):node : %s", __func__, node);
        m_gscSdEntity = exynos_media_get_entity_by_name(m_media, node, strlen(node));
        ALOGV("DEBUG(%s):m_gscSdEntity : 0x%p", __func__, m_gscSdEntity);

        // gscaler videodev
        sprintf(node, "%s.%d", PFX_VIDEODEV_ENTITY_GSC_CAP, GSC_NUM);
        ALOGV("DEBUG(%s):node : %s", __func__, node);
        m_gscVdEntity = exynos_media_get_entity_by_name(m_media, node, strlen(node));
        ALOGV("DEBUG(%s):m_gscVdEntity : 0x%p", __func__, m_gscVdEntity);

        ALOGV("DEBUG(%s):sensor_sd : numlink : %d", __func__, m_sensorEntity->num_links);
        ALOGV("DEBUG(%s):mipi_sd   : numlink : %d", __func__, m_mipiEntity->num_links);
        ALOGV("DEBUG(%s):flite_sd  : numlink : %d", __func__, m_fliteSdEntity->num_links);
        ALOGV("DEBUG(%s):flite_vd  : numlink : %d", __func__, m_fliteVdEntity->num_links);
        ALOGV("DEBUG(%s):gsc_sd    : numlink : %d", __func__, m_gscSdEntity->num_links);
        ALOGV("DEBUG(%s):gsc_vd    : numlink : %d", __func__, m_gscVdEntity->num_links);

        //////////////////
        // SETUP LINKS
        //////////////////
        // sensor subdev to mipi subdev
        links = m_sensorEntity->links;
        if (links == NULL ||
            links->source->entity != m_sensorEntity ||
            links->sink->entity != m_mipiEntity) {
            ALOGE("ERR(%s):Cannot make link camera sensor to mipi", __func__);
            goto err;
        }

        if (exynos_media_setup_link(m_media,  links->source,  links->sink, MEDIA_LNK_FL_ENABLED) < 0) {
            ALOGE("ERR(%s):Cannot make setup camera sensor to mipi", __func__);
            goto err;
        }
        ALOGV("DEBUG(%s):[LINK SUCCESS] sensor subdev to mipi subdev", __func__);

        // mipi subdev to fimc-lite subdev
        for (i = 0; i < m_mipiEntity->num_links; i++) {
            links = &m_mipiEntity->links[i];
            ALOGV("DEBUG(%s):i=%d: links->source->entity : %p, m_mipiEntity : %p", __func__, i,
                    links->source->entity, m_mipiEntity);
            ALOGV("DEBUG(%s):i=%d: links->sink->entity : %p, m_fliteSdEntity : %p", __func__, i,
                    links->sink->entity, m_fliteSdEntity);
            if (links == NULL ||
                links->source->entity != m_mipiEntity ||
                links->sink->entity != m_fliteSdEntity) {
                continue;
            } else if (exynos_media_setup_link(m_media,  links->source,  links->sink, MEDIA_LNK_FL_ENABLED) < 0) {
                ALOGE("ERR(%s):Cannot make setup mipi subdev to fimc-lite subdev", __func__);
                goto err;
            }
        }
        ALOGV("DEBUG(%s):[LINK SUCCESS] mipi subdev to fimc-lite subdev", __func__);

        // fimc-lite subdev TO fimc-lite video dev
        for (i = 0; i < m_fliteSdEntity->num_links; i++) {
            links = &m_fliteSdEntity->links[i];
            ALOGV("DEBUG(%s):i=%d: links->source->entity : %p, m_fliteSdEntity : %p", __func__, i,
                links->source->entity, m_fliteSdEntity);
            ALOGV("DEBUG(%s):i=%d: links->sink->entity : %p, m_fliteVdEntity : %p", __func__, i,
                links->sink->entity, m_fliteVdEntity);
            if (links == NULL ||
                links->source->entity != m_fliteSdEntity ||
                links->sink->entity != m_fliteVdEntity) {
                continue;
            } else if (exynos_media_setup_link(m_media,  links->source,  links->sink, MEDIA_LNK_FL_ENABLED) < 0) {
                ALOGE("ERR(%s):Cannot make setup fimc-lite subdev to fimc-lite video dev", __func__);
                goto err;
            }
        }
        ALOGV("DEBUG(%s):[LINK SUCCESS] fimc-lite subdev to fimc-lite video dev", __func__);

        // fimc-lite subdev to gscaler subdev
        for (i = 0; i < m_gscSdEntity->num_links; i++) {
            links = &m_gscSdEntity->links[i];
            ALOGV("DEBUG(%s):i=%d: links->source->entity : %p, m_fliteSdEntity : %p", __func__, i,
                    links->source->entity, m_fliteSdEntity);
            ALOGV("DEBUG(%s):i=%d: links->sink->entity : %p, m_gscSdEntity : %p", __func__, i,
                    links->sink->entity, m_gscSdEntity);
            if (links == NULL ||
                links->source->entity != m_fliteSdEntity ||
                links->sink->entity != m_gscSdEntity) {
                continue;
            } else if (exynos_media_setup_link(m_media, links->source, links->sink, MEDIA_LNK_FL_ENABLED) < 0) {
                ALOGE("ERR(%s):Cannot make setup fimc-lite subdev to gscaler subdev", __func__);
                goto err;
            }
        }
        ALOGV("DEBUG(%s):[LINK SUCCESS] fimc-lite subdev to gscaler subdev", __func__);

        // gscaler subdev to gscaler video dev
        for (i = 0; i < m_gscVdEntity->num_links; i++) {
            links = &m_gscVdEntity->links[i];
            ALOGV("DEBUG(%s):i=%d: links->source->entity : %p, m_gscSdEntity : %p", __func__, i,
                    links->source->entity, m_gscSdEntity);
            ALOGV("DEBUG(%s):i=%d: links->sink->entity : %p, m_gscVdEntity : %p", __func__, i,
                    links->sink->entity, m_gscVdEntity);
            if (links == NULL ||
                links->source->entity != m_gscSdEntity ||
                links->sink->entity != m_gscVdEntity) {
                continue;
            } else if (exynos_media_setup_link(m_media, links->source, links->sink, MEDIA_LNK_FL_ENABLED) < 0) {
                ALOGE("ERR(%s):Cannot make setup gscaler subdev to gscaler video dev", __func__);
                goto err;
            }
        }
        ALOGV("DEBUG(%s):[LINK SUCCESS] gscaler subdev to gscaler video dev", __func__);

        sprintf(node, "%s%d", PFX_NODE, (FLITE_VD_NODE_OFFSET + VIDEO_NODE_PREVIEW_ID));
        m_fliteDev.fd = exynos_v4l2_open(node, O_RDWR, 0);
        if (m_fliteDev.fd <= 0) {
            ALOGE("ERR(%s):exynos_v4l2_open(%s) fail (error : %s)", __func__, node, strerror(errno));
            goto err;
        }
        m_previewDev = &m_fliteDev;
        m_videoDev   = &m_fliteDev;
        m_pictureDev = &m_fliteDev;
    }

    m_previewDev->flagStart = false;
    m_videoDev->flagStart   = false;
    m_pictureDev->flagStart = false;

    m_tryPreviewStop = true;
    m_tryVideoStop   = true;
    m_tryPictureStop = true;

    m_flagStartFaceDetection = false;
    m_flagAutoFocusRunning = false;

    if (exynos_v4l2_enuminput(m_previewDev->fd, m_cameraId, m_cameraName) == false) {
        ALOGE("ERR(%s):exynos_v4l2_enuminput(%d, %s) fail", __func__, m_cameraId, m_cameraName);
        goto err;
    }

    // HACK
    if (m_cameraId == CAMERA_ID_BACK)
        strcpy(m_cameraName, "S5K4E5");
    else
        strcpy(m_cameraName, "S5K6A3");

    if (exynos_v4l2_s_input(m_previewDev->fd, m_cameraId) < 0) {
        ALOGE("ERR(%s):exynos_v4l2_s_input() fail", __func__);
        goto err;
    }

    if (strcmp((const char*)m_cameraName, "S5K4E5") == 0) {
        m_defaultCameraInfo  = new ExynosCameraInfoS5K4E5;
        m_curCameraInfo      = new ExynosCameraInfoS5K4E5;
    } else if (strcmp((const char*)m_cameraName, "S5K6A3") == 0) {
        m_defaultCameraInfo  = new ExynosCameraInfoS5K6A3;
        m_curCameraInfo      = new ExynosCameraInfoS5K6A3;
    } else if (strcmp((const char*)m_cameraName, "M5M0") == 0) {
        m_defaultCameraInfo  = new ExynosCameraInfoM5M0;
        m_curCameraInfo      = new ExynosCameraInfoM5M0;
    } else {
        ALOGE("ERR(%s):invalid camera Name (%s) fail", __func__, m_cameraName);
        goto err;
    }

    m_setExifFixedAttribute();

    m_flagCreate = true;
    return true;

err:
    if (m_defaultCameraInfo)
        delete m_defaultCameraInfo;
    m_defaultCameraInfo = NULL;

    if (m_curCameraInfo)
        delete m_curCameraInfo;
    m_curCameraInfo = NULL;

    if (0 < m_videoDev->fd)
        exynos_v4l2_close(m_videoDev->fd);
    m_videoDev->fd = 0;

    if (0 < m_pictureDev->fd)
        exynos_v4l2_close(m_pictureDev->fd);
    m_pictureDev->fd = 0;

    if (0 < m_previewDev->fd)
        exynos_v4l2_close(m_previewDev->fd);
    m_previewDev->fd = 0;

    if (m_media)
        exynos_media_close(m_media);
    m_media = NULL;

    return false;
}

bool ExynosCamera::destroy(void)
{
    if (m_flagCreate == false) {
        ALOGE("ERR(%s):Not yet created", __func__);
        return false;
    }

    if (m_pictureDev->flagStart == true)
        stopPicture();

    if (m_videoDev->flagStart == true)
        stopVideo();

    if (m_previewDev->flagStart == true)
        stopPreview();

    if (m_defaultCameraInfo)
        delete m_defaultCameraInfo;
    m_defaultCameraInfo = NULL;

    if (m_curCameraInfo)
        delete m_curCameraInfo;
    m_curCameraInfo = NULL;

    // close m_previewDev->fd after stopVideo() because stopVideo()
    // uses m_previewDev->fd to change frame rate
    if (0 < m_videoDev->fd)
        exynos_v4l2_close(m_videoDev->fd);
    m_videoDev->fd = 0;

    if (0 < m_pictureDev->fd)
        exynos_v4l2_close(m_pictureDev->fd);
    m_pictureDev->fd = 0;

    if (0 < m_previewDev->fd)
        exynos_v4l2_close(m_previewDev->fd);
    m_previewDev->fd = 0;

    if (m_media)
        exynos_media_close(m_media);
    m_media = NULL;

    m_flagCreate = false;

    return true;
}

bool ExynosCamera::flagCreate(void)
{
    return m_flagCreate;
}

int ExynosCamera::getCameraId(void)
{
    return m_cameraId;
}

char *ExynosCamera::getCameraName(void)
{
    return m_cameraName;
}

int ExynosCamera::getPreviewFd(void)
{
    return m_previewDev->fd;
}

int ExynosCamera::getPictureFd(void)
{
    return m_pictureDev->fd;
}

int ExynosCamera::getVideoFd(void)
{
    return m_videoDev->fd;
}

bool ExynosCamera::startPreview(void)
{
    if (m_flagCreate == false) {
        ALOGE("ERR(%s):Not yet Created", __func__);
        return false;
    }

    if (m_previewDev->flagStart == false) {
        if (m_setWidthHeight(PREVIEW_MODE,
                             m_previewDev->fd,
                             &m_previewDev->events,
                             m_curCameraInfo->previewW,
                             m_curCameraInfo->previewH,
                             m_curCameraInfo->previewColorFormat,
                             m_previewBuf,
                             m_validPreviewBuf) == false) {
            ALOGE("ERR(%s):m_setWidthHeight() fail", __func__);
            return false;
        }

        if (setPreviewFrameRate(m_curCameraInfo->fps) == false)
            ALOGE("ERR(%s):Fail toggle setPreviewFrameRate(%d)",
                __func__, m_curCameraInfo->fps);

        if (exynos_v4l2_streamon(m_previewDev->fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) < 0) {
            ALOGE("ERR(%s):exynos_v4l2_streamon() fail", __func__);
            return false;
        }

        if (m_curCameraInfo->focusMode == FOCUS_MODE_CONTINUOUS_VIDEO
            || m_curCameraInfo->focusMode == FOCUS_MODE_CONTINUOUS_PICTURE) {
            if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_CAMERA_CAF_START_STOP, CAF_START) < 0) {
                ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                return false;
            }
        }

		m_tryPreviewStop = false;
        m_previewDev->flagStart = true;

/* TODO */
/* DIS is only supported BACK camera(4E5) currently. */
#ifdef USE_DIS
        bool toggle = getVideoStabilization();

        if (setVideoStabilization(toggle) == false)
            ALOGE("ERR(%s):setVideoStabilization() fail", __func__);
#endif

#ifdef USE_3DNR
        if (m_recordingHint == true && getCameraId() == CAMERA_ID_BACK) {
            if (set3DNR(true) == false)
                ALOGE("ERR(%s):set3DNR() fail", __func__);
        }
#endif

#ifdef USE_ODC
        if (setODC(true) == false)
            ALOGE("ERR(%s):setODC() fail", __func__);
#endif
    }

    return true;
}

bool ExynosCamera::stopPreview(void)
{
    if (m_flagCreate == false) {
        ALOGE("ERR(%s):Not yet Created", __func__);
        return false;
    }

    if (m_previewDev->flagStart == true) {

        if (m_curCameraInfo->flashMode == FLASH_MODE_TORCH)
            setFlashMode(FLASH_MODE_OFF);

        m_tryPreviewStop = true;

        // skip stopPreview
        if (   (m_previewDev == m_videoDev   && m_tryVideoStop == false)
            || (m_previewDev == m_pictureDev && m_tryPictureStop == false))
            return true;

/* TODO */
/* Can not use 3DNR, ODC and DIS function because HW problem at exynos5250 EVT0 */
#ifdef USE_3DNR
        if (set3DNR(false) == false)
            ALOGE("ERR(%s):set3DNR() fail", __func__);
#endif

#ifdef USE_ODC
        if (setODC(false) == false)
            ALOGE("ERR(%s):setODC() fail", __func__);
#endif

        if (exynos_v4l2_streamoff(m_previewDev->fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) < 0) {
            ALOGE("ERR(%s):exynos_v4l2_streamoff() fail", __func__);
            return false;
        }

        struct v4l2_requestbuffers req;
        req.count  = 0;
        req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        req.memory = V4L2_MEMORY_DMABUF;

        if (exynos_v4l2_reqbufs(m_previewDev->fd, &req) < 0) {
            ALOGE("ERR(%s):exynos_v4l2_reqbufs() fail", __func__);
            return false;
        }

        m_previewDev->flagStart = false;

        m_flagStartFaceDetection = false;
    }

    return true;
}

bool ExynosCamera::flagStartPreview(void)
{
    return m_previewDev->flagStart;
}

int ExynosCamera::getPreviewMaxBuf(void)
{
    return VIDEO_MAX_FRAME;
}

bool ExynosCamera::setPreviewBuf(ExynosBuffer *buf)
{
    if (m_flagCreate == false) {
        ALOGE("ERR(%s):Not yet created fail", __func__);
        return false;
    }

    if (VIDEO_MAX_FRAME <= buf->reserved.p) {
        ALOGE("ERR(%s):index(%d) must smaller than %d", __func__, buf->reserved.p, VIDEO_MAX_FRAME);
        return false;
    }

    m_previewBuf[buf->reserved.p] = *buf;

    // HACK : Driver not yet support cb,cr of YV12
    m_previewBuf[buf->reserved.p].virt.extP[1] = buf->virt.extP[2];
    m_previewBuf[buf->reserved.p].virt.extP[2] = buf->virt.extP[1];

    return true;
}

bool ExynosCamera::getPreviewBuf(ExynosBuffer *buf)
{
    if (m_flagCreate == false) {
        ALOGE("ERR(%s):Not yet created fail", __func__);
        return false;
    }

    if (m_previewDev->flagStart == false) {
        ALOGE("ERR(%s):Not yet preview started fail", __func__);
        return false;
    }

    struct v4l2_buffer v4l2_buf;
    struct v4l2_plane  planes[VIDEO_MAX_PLANES];

    v4l2_buf.m.planes = planes;
    v4l2_buf.type     = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    v4l2_buf.memory   = V4L2_MEMORY_DMABUF;
    v4l2_buf.length   = 0;

    for (int i = 0; i < 3; i++) {
        if (m_previewBuf[0].size.extS[i] != 0)
            v4l2_buf.length++;
    }

    if (exynos_v4l2_dqbuf(m_previewDev->fd, &v4l2_buf) < 0) {
        ALOGE("ERR(%s):exynos_v4l2_dqbuf() fail", __func__);
        return false;
    }

    if (VIDEO_MAX_FRAME <= v4l2_buf.index) {
        ALOGE("ERR(%s):wrong index = %d", __func__, v4l2_buf.index);
        return false;
    }

    *buf = m_previewBuf[v4l2_buf.index];

    return true;
}

bool ExynosCamera::putPreviewBuf(ExynosBuffer *buf)
{
    if (m_flagCreate == false) {
        ALOGE("ERR(%s):Not yet created fail", __func__);
        return false;
    }

    if (m_validPreviewBuf[buf->reserved.p] == false) {
        ALOGE("ERR(%s):Invalid index(%d)", __func__, buf->reserved.p);
        return false;
    }

    struct v4l2_buffer v4l2_buf;
    struct v4l2_plane  planes[VIDEO_MAX_PLANES];

    v4l2_buf.m.planes = planes;
    v4l2_buf.type     = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    v4l2_buf.memory   = V4L2_MEMORY_DMABUF;
    v4l2_buf.index    = buf->reserved.p;
    v4l2_buf.length   = 0;

    for (int i = 0; i < 3; i++) {
        v4l2_buf.m.planes[i].m.fd= m_previewBuf[buf->reserved.p].fd.extFd[i];
        v4l2_buf.m.planes[i].length   = m_previewBuf[buf->reserved.p].size.extS[i];

        if (m_previewBuf[buf->reserved.p].size.extS[i] != 0)
            v4l2_buf.length++;
    }

    if (exynos_v4l2_qbuf(m_previewDev->fd, &v4l2_buf) < 0) {
        ALOGE("ERR(%s):exynos_v4l2_qbuf() fail", __func__);
        return false;
    }

    return true;
}

bool ExynosCamera::setVideoSize(int w, int h)
{
    m_curCameraInfo->videoW = w;
    m_curCameraInfo->videoH = h;

#ifdef USE_3DNR_DMAOUT
    // HACK : Video 3dnr port support resize. So, we must make max size video w, h
    m_curCameraInfo->videoW = m_defaultCameraInfo->videoW;
    m_curCameraInfo->videoH = m_defaultCameraInfo->videoH;
#endif
    return true;
}

bool ExynosCamera::getVideoSize(int *w, int *h)
{
    *w = m_curCameraInfo->videoW;
    *h = m_curCameraInfo->videoH;
    return true;
}

bool ExynosCamera::setVideoFormat(int colorFormat)
{
    m_curCameraInfo->videoColorFormat = colorFormat;
    return true;
}

int ExynosCamera::getVideoFormat(void)
{
    return m_curCameraInfo->videoColorFormat;
}

bool ExynosCamera::startVideo(void)
{
    if (m_flagCreate == false) {
        ALOGE("ERR(%s):Not yet Created", __func__);
        return false;
    }

#ifdef USE_3DNR_DMAOUT
    if (m_videoDev->flagStart == false) {
        if (m_setWidthHeight(VIDEO_MODE,
                             m_videoDev->fd,
                             &m_videoDev->events,
                             m_curCameraInfo->videoW,
                             m_curCameraInfo->videoH,
                             m_curCameraInfo->videoColorFormat,
                             m_videoBuf,
                             m_validVideoBuf) == false) {
            ALOGE("ERR(%s):m_setWidthHeight() fail", __func__);
            return false;
        }

        if (exynos_v4l2_streamon(m_videoDev->fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) < 0) {
            ALOGE("ERR(%s):exynos_v4l2_streamon() fail", __func__);
            return false;
        }

        m_tryVideoStop = false;
        m_videoDev->flagStart = true;
    }
#endif

    return true;
}

bool ExynosCamera::stopVideo(void)
{
    if (m_flagCreate == false) {
        ALOGE("ERR(%s):Not yet Created", __func__);
        return false;
    }

    if (m_videoDev->flagStart == true) {

        m_tryVideoStop = true;

        // skip stopVideo
        if (   (m_videoDev == m_previewDev && m_tryPreviewStop == false)
            || (m_videoDev == m_pictureDev && m_tryPictureStop == false))
            return true;

        if (exynos_v4l2_streamoff(m_videoDev->fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) < 0) {
            ALOGE("ERR(%s):exynos_v4l2_streamoff() fail", __func__);
            return false;
        }
        struct v4l2_requestbuffers req;
        req.count  = 0;
        req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        req.memory = V4L2_MEMORY_DMABUF;

        if (exynos_v4l2_reqbufs(m_videoDev->fd, &req) < 0) {
            ALOGE("ERR(%s):exynos_v4l2_reqbufs() fail", __func__);
            return false;
        }

        m_videoDev->flagStart = false;
    }

    return true;
}

bool ExynosCamera::flagStartVideo(void)
{
    return m_videoDev->flagStart;
}

int ExynosCamera::getVideoMaxBuf(void)
{
    return VIDEO_MAX_FRAME;
}

bool ExynosCamera::setVideoBuf(ExynosBuffer *buf)
{
    if (m_flagCreate == false) {
        ALOGE("ERR(%s):Not yet created fail", __func__);
        return false;
    }

    if (VIDEO_MAX_FRAME <= buf->reserved.p) {
        ALOGE("ERR(%s):index(%d) must smaller than %d", __func__, buf->reserved.p, VIDEO_MAX_FRAME);
        return false;
    }

    m_videoBuf[buf->reserved.p] = *buf;
    return true;
}

bool ExynosCamera::getVideoBuf(ExynosBuffer *buf)
{
    if (m_flagCreate == false) {
        ALOGE("ERR(%s):Not yet created fail", __func__);
        return false;
    }

    if (m_videoDev->flagStart == false) {
        ALOGE("ERR(%s):Not yet video started fail", __func__);
        return false;
    }

    struct v4l2_buffer v4l2_buf;
    struct v4l2_plane  planes[VIDEO_MAX_PLANES];

    v4l2_buf.m.planes = planes;
    v4l2_buf.type     = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    v4l2_buf.memory   = V4L2_MEMORY_DMABUF;
    v4l2_buf.length   = 0;

    for (int i = 0; i < 3; i++) {
        if (m_videoBuf[0].size.extS[i] != 0)
            v4l2_buf.length++;
    }

    if (exynos_v4l2_dqbuf(m_videoDev->fd, &v4l2_buf) < 0) {
        ALOGE("ERR(%s):exynos_v4l2_dqbuf() fail", __func__);
        return false;
    }

    if (VIDEO_MAX_FRAME <= v4l2_buf.index) {
        ALOGE("ERR(%s):wrong index = %d", __func__, v4l2_buf.index);
        return false;
    }

    *buf = m_videoBuf[v4l2_buf.index];

    return true;
}

bool ExynosCamera::putVideoBuf(ExynosBuffer *buf)
{
    if (m_flagCreate == false) {
        ALOGE("ERR(%s):Not yet created fail", __func__);
        return false;
    }

    if (m_videoDev->flagStart == false) {
        /* this can happen when recording frames are returned after
         * the recording is stopped at the driver level.  we don't
         * need to return the buffers in this case and we've seen
         * cases where fimc could crash if we called qbuf and it
         * wasn't expecting it.
         */
        ALOGV("DEBUG(%s):recording not in progress, ignoring", __func__);
        return true;
    }

    if (m_validVideoBuf[buf->reserved.p] == false) {
        ALOGE("ERR(%s):Invalid index(%d)", __func__, buf->reserved.p);
        return false;
    }

    struct v4l2_buffer v4l2_buf;
    struct v4l2_plane  planes[VIDEO_MAX_PLANES];

    v4l2_buf.m.planes = planes;
    v4l2_buf.type     = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    v4l2_buf.memory   = V4L2_MEMORY_DMABUF;
    v4l2_buf.index    = buf->reserved.p;
    v4l2_buf.length   = 0;

    for (int i = 0; i < 3; i++) {
        v4l2_buf.m.planes[i].m.fd = (unsigned long)m_videoBuf[buf->reserved.p].fd.extFd[i];
        v4l2_buf.m.planes[i].length   = m_videoBuf[buf->reserved.p].size.extS[i];

        if (m_videoBuf[buf->reserved.p].size.extS[i] != 0)
            v4l2_buf.length++;
    }

    if (exynos_v4l2_qbuf(m_videoDev->fd, &v4l2_buf) < 0) {
        ALOGE("ERR(%s):exynos_v4l2_qbuf() fail", __func__);
        return false;
    }

    return true;
}

bool ExynosCamera::startPicture(void)
{
    if (m_flagCreate == false) {
        ALOGE("ERR(%s):Not yet Created", __func__);
        return false;
    }

    if (m_pictureDev->flagStart == false) {
        if (m_setWidthHeight(PICTURE_MODE,
                             m_pictureDev->fd,
                             &m_pictureDev->events,
                             m_curCameraInfo->pictureW,
                             m_curCameraInfo->pictureH,
                             m_curCameraInfo->pictureColorFormat,
                             m_pictureBuf,
                             m_validPictureBuf) == false) {
            ALOGE("ERR(%s):m_setWidthHeight() fail", __func__);
            return false;
        }

        if (exynos_v4l2_streamon(m_pictureDev->fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) < 0) {
            ALOGE("ERR(%s):exynos_v4l2_streamon() fail", __func__);
            return false;
        }

        m_tryPictureStop = false;
        m_pictureDev->flagStart = true;
    }

    return true;
}

bool ExynosCamera::stopPicture(void)
{
    if (m_flagCreate == false) {
        ALOGE("ERR(%s):Not yet Created", __func__);
        return false;
    }

    if (m_pictureDev->flagStart == true) {

        m_tryPictureStop = true;

        // skip stopPicture
        if (   (m_pictureDev == m_previewDev && m_tryPreviewStop == false)
            || (m_pictureDev == m_videoDev   && m_tryVideoStop == false))
            return true;

        if (exynos_v4l2_streamoff(m_pictureDev->fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) < 0) {
            ALOGE("ERR(%s):exynos_v4l2_streamoff() fail", __func__);
            return false;
        }

        struct v4l2_requestbuffers req;
        req.count  = 0;
        req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        req.memory = V4L2_MEMORY_DMABUF;

        if (exynos_v4l2_reqbufs(m_pictureDev->fd, &req) < 0) {
            ALOGE("ERR(%s):exynos_v4l2_reqbufs() fail", __func__);
            return false;
        }

        m_pictureDev->flagStart = false;
    }

    return true;
}

bool ExynosCamera::flagStartPicture(void)
{
    return m_pictureDev->flagStart;
}

int ExynosCamera::getPictureMaxBuf(void)
{
    return VIDEO_MAX_FRAME;
}

bool ExynosCamera::setPictureBuf(ExynosBuffer *buf)
{
    if (m_flagCreate == false) {
        ALOGE("ERR(%s):Not yet created fail", __func__);
        return false;
    }

    if (VIDEO_MAX_FRAME <= buf->reserved.p) {
        ALOGE("ERR(%s):index(%d) must smaller than %d", __func__, buf->reserved.p, VIDEO_MAX_FRAME);
        return false;
    }

    m_pictureBuf[buf->reserved.p] = *buf;
    return true;
}

bool ExynosCamera::getPictureBuf(ExynosBuffer *buf)
{
    if (m_flagCreate == false) {
        ALOGE("ERR(%s):Not yet created fail", __func__);
        return false;
    }

    if (m_pictureDev->flagStart == false) {
        ALOGE("ERR(%s):Not yet picture started fail", __func__);
        return false;
    }

    struct v4l2_buffer v4l2_buf;
    struct v4l2_plane  planes[VIDEO_MAX_PLANES];

    v4l2_buf.m.planes = planes;
    v4l2_buf.type     = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    v4l2_buf.memory   = V4L2_MEMORY_DMABUF;
    v4l2_buf.length   = 0;

    for (int i = 0; i < 3; i++) {
        if (m_pictureBuf[0].size.extS[i] != 0)
            v4l2_buf.length++;
    }

    if (exynos_v4l2_dqbuf(m_pictureDev->fd, &v4l2_buf) < 0) {
        ALOGE("ERR(%s):exynos_v4l2_dqbuf() fail", __func__);
        return false;
    }

    if (VIDEO_MAX_FRAME <= v4l2_buf.index) {
        ALOGE("ERR(%s):wrong index = %d", __func__, v4l2_buf.index);
        return false;
    }

    *buf = m_pictureBuf[v4l2_buf.index];

    return true;
}

bool ExynosCamera::putPictureBuf(ExynosBuffer *buf)
{
    if (m_flagCreate == false) {
        ALOGE("ERR(%s):Not yet created fail", __func__);
        return false;
    }

    if (m_pictureDev->flagStart == false) {
        ALOGE("ERR(%s):Not yet picture started fail", __func__);
        return false;
    }

    if (m_validPictureBuf[buf->reserved.p] == false) {
        ALOGE("ERR(%s):Invalid index(%d)", __func__, buf->reserved.p);
        return false;
    }

    struct v4l2_buffer v4l2_buf;
    struct v4l2_plane  planes[VIDEO_MAX_PLANES];

    v4l2_buf.m.planes = planes;
    v4l2_buf.type     = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    v4l2_buf.memory   = V4L2_MEMORY_DMABUF;
    v4l2_buf.index    = buf->reserved.p;
    v4l2_buf.length   = 0;

    for (int i = 0; i < 3; i++) {
        v4l2_buf.m.planes[i].m.fd = (unsigned long)m_pictureBuf[buf->reserved.p].fd.extFd[i];
        v4l2_buf.m.planes[i].length   = m_pictureBuf[buf->reserved.p].size.extS[i];

        if (m_pictureBuf[buf->reserved.p].size.extS[i] != 0)
            v4l2_buf.length++;
    }

    if (exynos_v4l2_qbuf(m_pictureDev->fd, &v4l2_buf) < 0) {
        ALOGE("ERR(%s):exynos_v4l2_qbuf() fail", __func__);
        return false;
    }

    return true;
}

bool ExynosCamera::yuv2Jpeg(ExynosBuffer *yuvBuf,
                            ExynosBuffer *jpegBuf,
                            ExynosRect *rect)
{
    unsigned char *addr;

    ExynosJpegEncoderForCamera jpegEnc;
    bool ret = false;

    unsigned int *yuvSize = yuvBuf->size.extS;

    if (jpegEnc.create()) {
        ALOGE("ERR(%s):jpegEnc.create() fail", __func__);
        goto jpeg_encode_done;
    }

    if (jpegEnc.setQuality(m_jpegQuality)) {
        ALOGE("ERR(%s):jpegEnc.setQuality() fail", __func__);
        goto jpeg_encode_done;
    }

    if (jpegEnc.setSize(rect->w, rect->h)) {
        ALOGE("ERR(%s):jpegEnc.setSize() fail", __func__);
        goto jpeg_encode_done;
    }

    if (jpegEnc.setColorFormat(rect->colorFormat)) {
        ALOGE("ERR(%s):jpegEnc.setColorFormat() fail", __func__);
        goto jpeg_encode_done;
    }

    if (jpegEnc.setJpegFormat(V4L2_PIX_FMT_JPEG_422)) {
        ALOGE("ERR(%s):jpegEnc.setJpegFormat() fail", __func__);
        goto jpeg_encode_done;
    }

    if (m_curCameraInfo->thumbnailW != 0 && m_curCameraInfo->thumbnailH != 0) {
        mExifInfo.enableThumb = true;
        if (jpegEnc.setThumbnailSize(m_curCameraInfo->thumbnailW, m_curCameraInfo->thumbnailH)) {
            ALOGE("ERR(%s):jpegEnc.setThumbnailSize(%d, %d) fail", __func__, m_curCameraInfo->thumbnailW, m_curCameraInfo->thumbnailH);
            goto jpeg_encode_done;
        }

        if (0 < m_jpegThumbnailQuality && m_jpegThumbnailQuality <= 100) {
            if (jpegEnc.setThumbnailQuality(m_jpegThumbnailQuality)) {
                ALOGE("ERR(%s):jpegEnc.setThumbnailSize(%d, %d) fail", __func__, m_curCameraInfo->thumbnailW, m_curCameraInfo->thumbnailH);
                goto jpeg_encode_done;
            }
        }

        m_setExifChangedAttribute(&mExifInfo, rect);
    } else {
        mExifInfo.enableThumb = false;
    }

    if (jpegEnc.setInBuf((char **)&(yuvBuf->virt.p), (int *)yuvSize)) {
        ALOGE("ERR(%s):jpegEnc.setInBuf() fail", __func__);
        goto jpeg_encode_done;
    }

    if (jpegEnc.setOutBuf(jpegBuf->virt.p, jpegBuf->size.extS[0] + jpegBuf->size.extS[1] + jpegBuf->size.extS[2])) {
        ALOGE("ERR(%s):jpegEnc.setOutBuf() fail", __func__);
        goto jpeg_encode_done;
    }

    if (jpegEnc.updateConfig()) {
        ALOGE("ERR(%s):jpegEnc.updateConfig() fail", __func__);
        goto jpeg_encode_done;
    }

    if (jpegEnc.encode((int *)&jpegBuf->size.s, &mExifInfo)) {
        ALOGE("ERR(%s):jpegEnc.encode() fail", __func__);
        goto jpeg_encode_done;
    }

    ret = true;

jpeg_encode_done:

    if (jpegEnc.flagCreate() == true)
        jpegEnc.destroy();

    return ret;
}

bool ExynosCamera::autoFocus(void)
{
    if (m_previewDev->fd <= 0) {
        ALOGE("ERR(%s):Camera was closed", __func__);
        return false;
    }

    if (m_flagAutoFocusRunning == true) {
        ALOGD("DEBUG(%s):m_flagAutoFocusRunning == true", __func__);
        return true;
    }

    switch (m_curCameraInfo->focusMode) {
    case FOCUS_MODE_AUTO:
    case FOCUS_MODE_INFINITY:
    case FOCUS_MODE_MACRO:
        if (m_touchAFMode == true) {
            if (setFocusMode(FOCUS_MODE_TOUCH) == false) {
                ALOGE("ERR(%s): %d: setFocusMode(FOCUS_MODE_TOUCH) fail", __func__, __LINE__);
                return false;
            }
        } else {
            if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_CAMERA_SET_AUTO_FOCUS, AUTO_FOCUS_ON) < 0) {
                ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                return false;
            }
        }
        break;
    case FOCUS_MODE_CONTINUOUS_VIDEO:
    case FOCUS_MODE_CONTINUOUS_PICTURE:
        /* Doing nothing. Because we assume that continuous focus mode is
           always focused on. */
        break;
    case FOCUS_MODE_TOUCH:
        if (setFocusMode(FOCUS_MODE_TOUCH) == false) {
            ALOGE("ERR(%s): %d: setFocusMode(FOCUS_MODE_TOUCH) fail", __func__, __LINE__);
            return false;
        }
        break;
    case FOCUS_MODE_FIXED:
        break;
    case FOCUS_MODE_EDOF:
    default:
        ALOGE("ERR(%s):Unsupported value(%d)", __func__, m_curCameraInfo->focusMode);
        return false;
        break;
    }

    m_flagAutoFocusRunning = true;

    return true;
}

bool ExynosCamera::cancelAutoFocus(void)
{
    if (m_previewDev->fd <= 0) {
        ALOGE("ERR(%s):Camera was closed", __func__);
        return false;
    }

    if (m_flagAutoFocusRunning == false) {
        ALOGV("DEBUG(%s):m_flagAutoFocusRunning == false", __func__);
        return true;
    }

    switch (m_curCameraInfo->focusMode) {
    case FOCUS_MODE_AUTO:
    case FOCUS_MODE_INFINITY:
    case FOCUS_MODE_MACRO:
        if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_CAMERA_SET_AUTO_FOCUS, AUTO_FOCUS_OFF) < 0) {
            ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
            return false;
        }
        break;
    case FOCUS_MODE_CONTINUOUS_VIDEO:
    case FOCUS_MODE_CONTINUOUS_PICTURE:
        /* Doing nothing. Because we assume that continuous focus mode is
           always focused on. */
        break;
    case FOCUS_MODE_TOUCH:
        if (setFocusMode(FOCUS_MODE_TOUCH) == false) {
            ALOGE("ERR(%s): %d: setFocusMode(FOCUS_MODE_TOUCH) fail", __func__, __LINE__);
            return false;
        }
        m_touchAFMode = false;
        break;
    case FOCUS_MODE_FIXED:
        break;
    case FOCUS_MODE_EDOF:
    default:
        ALOGE("ERR(%s):Unsupported value(%d)", __func__, m_curCameraInfo->focusMode);
        return false;
        break;
    }

    m_flagAutoFocusRunning = false;

    return true;
}

int ExynosCamera::getFucusModeResult(void)
{
    int ret = 0;

#define AF_WATING_TIME       (100000)  //  100msec
#define TOTAL_AF_WATING_TIME (2000000) // 2000msec

    for (unsigned int i = 0; i < TOTAL_AF_WATING_TIME; i += AF_WATING_TIME) {

        if (m_flagAutoFocusRunning == false)
            return -1;

        if (exynos_v4l2_g_ctrl(m_previewDev->fd, V4L2_CID_CAMERA_AUTO_FOCUS_RESULT, &ret) < 0) {
            ALOGE("ERR(%s):exynos_v4l2_g_ctrl() fail", __func__);
            return -1;
        }

        if (strcmp((const char*)m_cameraName, "S5K4E5") == 0) {
            switch(ret) {
            case 0x00: // AF Running
                ret = 0;
                break;
            case 0x02: // AF succeed
                ret = 1;
                break;
            case 0x01:
            default :  // AF fail
                ret = -1;
                break;
            }

            if (ret != 0)
                break;

        } else if (strcmp((const char*)m_cameraName, "M5M0") == 0) {
            switch(ret) {
            case 0x00: // AF Running
                ret = 0;
                break;
            case 0x01: // AF succeed
                ret = 1;
                break;
            case 0x02: // AF cancel
                ret = 0;
                break;
            default:  // AF fail
                ret = -1;
                break;
            }

            if (ret != 0)
                break;
        } else {
            ret = -1;
            break;
        }

        usleep(AF_WATING_TIME);
    }

    return ret;
}

bool ExynosCamera::startFaceDetection(void)
{
    if (m_flagStartFaceDetection == true) {
        ALOGD("DEBUG(%s):Face detection already started..", __func__);
        return true;
    }

    if (m_previewDev->flagStart == true) {
        //if (this->setFocusMode(FOCUS_MODE_AUTO) == false)
        //    ALOGE("ERR(%s):Fail setFocusMode", __func__);

        if (m_internalISP == true) {
            if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_IS_FD_SET_MAX_FACE_NUMBER, m_defaultCameraInfo->maxNumDetectedFaces) < 0) {
                ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                return false;
            }

            if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_IS_CMD_FD, IS_FD_COMMAND_START) < 0) {
                ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                return false;
            }
        } else {
            if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_CAMERA_FACE_DETECTION, FACE_DETECTION_ON) < 0) {
                ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                return false;
            }
        }
        m_flagStartFaceDetection = true;
    }
    return true;
}

bool ExynosCamera::stopFaceDetection(void)
{
    if (m_flagStartFaceDetection == false) {
        ALOGD("DEBUG(%s):Face detection already stopped..", __func__);
        return true;
    }

    if (m_previewDev->flagStart == true) {
        if (m_internalISP == true) {
            if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_IS_CMD_FD, IS_FD_COMMAND_STOP) < 0) {
                ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                return false;
            }
        } else {
            if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_CAMERA_FACE_DETECTION, FACE_DETECTION_OFF) < 0) {
                ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                return false;
            }
        }
        m_flagStartFaceDetection = false;
    }
    return true;
}

bool ExynosCamera::flagStartFaceDetection(void)
{
    return m_flagStartFaceDetection;
}

bool ExynosCamera::setFaceDetectLock(bool toggle)
{
    int lock = (toggle == true) ? 1 : 0;

    if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_CAMERA_FACEDETECT_LOCKUNLOCK, lock) < 0) {
        ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
        return false;
    }
    return true;
}

bool ExynosCamera::startSmoothZoom(int value)
{
    if (m_defaultCameraInfo->hwZoomSupported == false) {
        ALOGE("ERR(%s):m_defaultCameraInfo->hwZoomSupported == false", __func__);
        return false;
    }

    return this->setZoom(value);
}

bool ExynosCamera::stopSmoothZoom(void)
{
    // TODO
    return true;
}

int ExynosCamera::getAntibanding(void)
{
    return m_curCameraInfo->antiBanding;
}

bool ExynosCamera::getAutoExposureLock(void)
{
    return m_curCameraInfo->autoExposureLock;
}

bool ExynosCamera::getAutoWhiteBalanceLock(void)
{
    return m_curCameraInfo->autoWhiteBalanceLock;
}

int ExynosCamera::getColorEffect(void)
{
    return m_curCameraInfo->effect;
}

int ExynosCamera::getDetectedFacesAreas(int num,
                                        int *id,
                                        int *score,
                                        ExynosRect *face,
                                        ExynosRect *leftEye,
                                        ExynosRect *rightEye,
                                        ExynosRect *mouth)
{
    if (m_defaultCameraInfo->maxNumDetectedFaces == 0) {
        ALOGE("ERR(%s):maxNumDetectedFaces == 0 fail", __func__);
        return -1;
    }

    if (m_flagStartFaceDetection == false) {
        ALOGD("DEBUG(%s):m_flagStartFaceDetection == false", __func__);
        return 0;
    }

    if (m_defaultCameraInfo->maxNumDetectedFaces < num)
        num = m_defaultCameraInfo->maxNumDetectedFaces;

    // width   : 0 ~ previewW
    // height  : 0 ~ previewH
    // if eye, mouth is not detectable : -1, -1
    ExynosRect2 *face2     = new ExynosRect2[num];
    ExynosRect2 *leftEye2  = new ExynosRect2[num];
    ExynosRect2 *rightEye2 = new ExynosRect2[num];
    ExynosRect2 *mouth2    = new ExynosRect2[num];

    num = getDetectedFacesAreas(num, id, score, face2, leftEye2, rightEye2, mouth2);

    for (int i = 0; i < num; i++) {

        m_secRect22SecRect(&face2[i], &face[i]);
        face[i].fullW = m_curCameraInfo->previewW;
        face[i].fullH = m_curCameraInfo->previewH;

        m_secRect22SecRect(&leftEye2[i], &leftEye[i]);
        leftEye[i].fullW = m_curCameraInfo->previewW;
        leftEye[i].fullH = m_curCameraInfo->previewH;

        m_secRect22SecRect(&rightEye2[i], &rightEye[i]);
        rightEye[i].fullW = m_curCameraInfo->previewW;
        rightEye[i].fullH = m_curCameraInfo->previewH;

        m_secRect22SecRect(&mouth2[i], &mouth[i]);
        mouth[i].fullW = m_curCameraInfo->previewW;
        mouth[i].fullH = m_curCameraInfo->previewH;
    }

    delete [] face2;
    delete [] leftEye2;
    delete [] rightEye2;
    delete [] mouth2;

    return num;
}

int ExynosCamera::getDetectedFacesAreas(int num,
                                     int *id,
                                     int *score,
                                     ExynosRect2 *face,
                                     ExynosRect2 *leftEye,
                                     ExynosRect2 *rightEye,
                                     ExynosRect2 *mouth)
{
    if (m_defaultCameraInfo->maxNumDetectedFaces == 0) {
        ALOGE("ERR(%s):maxNumDetectedFaces == 0 fail", __func__);
        return -1;
    }

    if (m_flagStartFaceDetection == false) {
        ALOGD("DEBUG(%s):m_flagStartFaceDetection == false", __func__);
        return 0;
    }

    int i = 0;

    if (m_defaultCameraInfo->maxNumDetectedFaces < num)
        num = m_defaultCameraInfo->maxNumDetectedFaces;

    const unsigned int numOfFDEntity = 1 + ((V4L2_CID_IS_FD_GET_NEXT - V4L2_CID_IS_FD_GET_FACE_FRAME_NUMBER) * num);

    // width   : 0 ~ previewW
    // height  : 0 ~ previewH
    // if eye, mouth is not detectable : -1, -1
    struct v4l2_ext_controls fd_ctrls;
    struct v4l2_ext_control *fd_ctrl = new struct v4l2_ext_control[numOfFDEntity];
    struct v4l2_ext_control *cur_ctrl;

    cur_ctrl = &fd_ctrl[0];
    cur_ctrl->id = V4L2_CID_IS_FD_GET_FACE_COUNT;
    cur_ctrl++;

    for (i = 0; i < num; i++) {
        cur_ctrl->id = V4L2_CID_IS_FD_GET_FACE_FRAME_NUMBER;
        cur_ctrl++;
        cur_ctrl->id = V4L2_CID_IS_FD_GET_FACE_CONFIDENCE;
        cur_ctrl++;
        cur_ctrl->id = V4L2_CID_IS_FD_GET_FACE_TOPLEFT_X;
        cur_ctrl++;
        cur_ctrl->id = V4L2_CID_IS_FD_GET_FACE_TOPLEFT_Y;
        cur_ctrl++;
        cur_ctrl->id = V4L2_CID_IS_FD_GET_FACE_BOTTOMRIGHT_X;
        cur_ctrl++;
        cur_ctrl->id = V4L2_CID_IS_FD_GET_FACE_BOTTOMRIGHT_Y;
        cur_ctrl++;
        cur_ctrl->id = V4L2_CID_IS_FD_GET_LEFT_EYE_TOPLEFT_X;
        cur_ctrl++;
        cur_ctrl->id = V4L2_CID_IS_FD_GET_LEFT_EYE_TOPLEFT_Y;
        cur_ctrl++;
        cur_ctrl->id = V4L2_CID_IS_FD_GET_LEFT_EYE_BOTTOMRIGHT_X;
        cur_ctrl++;
        cur_ctrl->id = V4L2_CID_IS_FD_GET_LEFT_EYE_BOTTOMRIGHT_Y;
        cur_ctrl++;
        cur_ctrl->id = V4L2_CID_IS_FD_GET_RIGHT_EYE_TOPLEFT_X;
        cur_ctrl++;
        cur_ctrl->id = V4L2_CID_IS_FD_GET_RIGHT_EYE_TOPLEFT_Y;
        cur_ctrl++;
        cur_ctrl->id = V4L2_CID_IS_FD_GET_RIGHT_EYE_BOTTOMRIGHT_X;
        cur_ctrl++;
        cur_ctrl->id = V4L2_CID_IS_FD_GET_RIGHT_EYE_BOTTOMRIGHT_Y;
        cur_ctrl++;
        cur_ctrl->id = V4L2_CID_IS_FD_GET_MOUTH_TOPLEFT_X;
        cur_ctrl++;
        cur_ctrl->id = V4L2_CID_IS_FD_GET_MOUTH_TOPLEFT_Y;
        cur_ctrl++;
        cur_ctrl->id = V4L2_CID_IS_FD_GET_MOUTH_BOTTOMRIGHT_X;
        cur_ctrl++;
        cur_ctrl->id = V4L2_CID_IS_FD_GET_MOUTH_BOTTOMRIGHT_Y;
        cur_ctrl++;
        cur_ctrl->id = V4L2_CID_IS_FD_GET_NEXT;
        cur_ctrl++;
    }

    fd_ctrls.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    fd_ctrls.count = i + 1;
    fd_ctrls.controls = fd_ctrl;

    if (exynos_v4l2_g_ext_ctrl(m_previewDev->fd, &fd_ctrls) < 0) {
        ALOGE("ERR(%s):exynos_v4l2_g_ext_ctrl() fail", __func__);
        num = -1;
        goto done;
    }

    cur_ctrl = &fd_ctrl[0];
    num = cur_ctrl->value;
    cur_ctrl++;

    for (i = 0; i < num; i++) {
        id[i] = cur_ctrl->value;
        cur_ctrl++;
        score[i] = cur_ctrl->value;
        cur_ctrl++;

        face[i].x1 = cur_ctrl->value;
        cur_ctrl++;
        face[i].y1 = cur_ctrl->value;
        cur_ctrl++;
        face[i].x2 = cur_ctrl->value;
        cur_ctrl++;
        face[i].y2 = cur_ctrl->value;
        cur_ctrl++;

        leftEye[i].x1 = cur_ctrl->value;
        cur_ctrl++;
        leftEye[i].y1 = cur_ctrl->value;
        cur_ctrl++;
        leftEye[i].x2 = cur_ctrl->value;
        cur_ctrl++;
        leftEye[i].y2 = cur_ctrl->value;
        cur_ctrl++;

        rightEye[i].x1 = cur_ctrl->value;
        cur_ctrl++;
        rightEye[i].y1 = cur_ctrl->value;
        cur_ctrl++;
        rightEye[i].x2 = cur_ctrl->value;
        cur_ctrl++;
        rightEye[i].y2 = cur_ctrl->value;
        cur_ctrl++;

        mouth[i].x1 = cur_ctrl->value;
        cur_ctrl++;
        mouth[i].y1 = cur_ctrl->value;
        cur_ctrl++;
        mouth[i].x2 = cur_ctrl->value;
        cur_ctrl++;
        mouth[i].y2 = cur_ctrl->value;
        cur_ctrl++;
    }

done:
    delete [] fd_ctrl;

    return num;
}

int ExynosCamera::getExposureCompensation(void)
{
    return m_curCameraInfo->exposure;
}

float ExynosCamera::getExposureCompensationStep(void)
{
    // CameraParameters.h
    // The exposure compensation step. Exposure compensation index multiply by
    // step eqals to EV. Ex: if exposure compensation index is 6 and step is
    // 0.3333, EV is -2.
    // Example value: "0.333333333" or "0.5". Read only.
    // -> But, this formula doesn't works in apps.
    return 1.0f;
}

int ExynosCamera::getFlashMode(void)
{
    return m_curCameraInfo->flashMode;
}

bool ExynosCamera::getFocalLength(int *num, int *den)
{
    *num = m_defaultCameraInfo->focalLengthNum;
    *num = m_defaultCameraInfo->focalLengthDen;
    return true;
}

int ExynosCamera::getFocusAreas(ExynosRect *rects)
{
    // TODO
    return 0;
}

int ExynosCamera::getFocusDistances(float *output)
{
    // TODO
    return 0;
}

int ExynosCamera::getFocusMode(void)
{
    return m_curCameraInfo->focusMode;
}

float ExynosCamera::getHorizontalViewAngle(void)
{
    //TODO
    return 51.2f;
}

int ExynosCamera::getJpegQuality(void)
{
    return m_jpegQuality;
}

int ExynosCamera::getJpegThumbnailQuality(void)
{
    return m_jpegThumbnailQuality;
}

bool ExynosCamera::getJpegThumbnailSize(int *w, int  *h)
{
    *w  = m_curCameraInfo->thumbnailW;
    *h  = m_curCameraInfo->thumbnailH;
    return true;
}

int ExynosCamera::getMaxExposureCompensation(void)
{
    return m_defaultCameraInfo->maxExposure;
}

int ExynosCamera::getMaxNumDetectedFaces(void)
{
    return m_defaultCameraInfo->maxNumDetectedFaces;
}

int ExynosCamera::getMaxNumFocusAreas(void)
{
    return m_defaultCameraInfo->maxNumFocusAreas;
}

int ExynosCamera::getMaxNumMeteringAreas(void)
{
    return m_defaultCameraInfo->maxNumMeteringAreas;
}

int ExynosCamera::getMaxZoom(void)
{
    return m_defaultCameraInfo->maxZoom;
}

int ExynosCamera::getMeteringAreas(ExynosRect *rects)
{
    // TODO
    return 0;
}

int ExynosCamera::getMinExposureCompensation(void)
{
    return m_defaultCameraInfo->minExposure;
}

int ExynosCamera::getPictureFormat(void)
{
    return m_curCameraInfo->pictureColorFormat;
}

bool ExynosCamera::getPictureSize(int *w, int *h)
{
    *w = m_curCameraInfo->pictureW;
    *h = m_curCameraInfo->pictureH;
    return true;
}

int ExynosCamera::getPreviewFormat(void)
{
    return m_curCameraInfo->previewColorFormat;
}

bool ExynosCamera::getPreviewFpsRange(int *min, int *max)
{
    *min = 1;
    *max = m_defaultCameraInfo->fps;
    return true;
}

int ExynosCamera::getPreviewFrameRate(void)
{
    return m_curCameraInfo->fps;
}

bool ExynosCamera::getPreviewSize(int *w, int *h)
{
    *w = m_curCameraInfo->previewW;
    *h = m_curCameraInfo->previewH;
    return true;
}

int ExynosCamera::getSceneMode(void)
{
    return m_curCameraInfo->sceneMode;
}

int ExynosCamera::getSupportedAntibanding(void)
{
    return m_defaultCameraInfo->antiBandingList;
}

int ExynosCamera::getSupportedColorEffects(void)
{
    return m_defaultCameraInfo->effectList;
}

int ExynosCamera::getSupportedFlashModes(void)
{
    return m_defaultCameraInfo->flashModeList;
}

int ExynosCamera::getSupportedFocusModes(void)
{
    return m_defaultCameraInfo->focusModeList;
}

bool ExynosCamera::getSupportedJpegThumbnailSizes(int *w, int *h)
{
    *w  = m_defaultCameraInfo->thumbnailW;
    *h  = m_defaultCameraInfo->thumbnailH;
    return true;
}

bool ExynosCamera::getSupportedPictureSizes(int *w, int *h)
{
    *w = m_defaultCameraInfo->pictureW;
    *h = m_defaultCameraInfo->pictureH;
    return true;
}

bool ExynosCamera::getSupportedPreviewSizes(int *w, int *h)
{
    *w = m_defaultCameraInfo->previewW;
    *h = m_defaultCameraInfo->previewH;
    return true;
}

int ExynosCamera::getSupportedSceneModes(void)
{
    return m_defaultCameraInfo->sceneModeList;
}

bool ExynosCamera::getSupportedVideoSizes(int *w, int *h)
{
    *w = m_defaultCameraInfo->videoW;
    *h = m_defaultCameraInfo->videoH;
    return true;
}

bool ExynosCamera::getPreferredPreivewSizeForVideo(int *w, int *h)
{
    *w = m_defaultCameraInfo->prefVideoPreviewW;
    *h = m_defaultCameraInfo->prefVideoPreviewH;
    return true;
}

int ExynosCamera::getSupportedWhiteBalance(void)
{
    return m_defaultCameraInfo->whiteBalanceList;
}

float ExynosCamera::getVerticalViewAngle(void)
{
    // TODO
    return 39.4f;
}

bool ExynosCamera::getVideoStabilization(void)
{
    return m_curCameraInfo->videoStabilization;
}

int ExynosCamera::getWhiteBalance(void)
{
    return m_curCameraInfo->whiteBalance;
}

int ExynosCamera::getZoom(void)
{
    return m_curCameraInfo->zoom;
}

int ExynosCamera::getMaxZoomRatio(void)
{
    return 400;
}

bool ExynosCamera::isAutoExposureLockSupported(void)
{
    return m_defaultCameraInfo->autoExposureLockSupported;
}

bool ExynosCamera::isAutoWhiteBalanceLockSupported(void)
{
    return m_defaultCameraInfo->autoWhiteBalanceLockSupported;
}

bool ExynosCamera::isSmoothZoomSupported(void)
{
    if (m_defaultCameraInfo->hwZoomSupported == true)
        return true;
    else
        return false;
}

bool ExynosCamera::isVideoSnapshotSupported(void)
{
    return true;
}

bool ExynosCamera::isVideoStabilizationSupported(void)
{
    return m_defaultCameraInfo->supportVideoStabilization;
}

bool ExynosCamera::isZoomSupported(void)
{
    return true;
}

bool ExynosCamera::setAntibanding(int value)
{
    int internalValue = -1;

    switch (value) {
    case ANTIBANDING_AUTO:
        internalValue = ::ANTI_BANDING_AUTO;
        break;
    case ANTIBANDING_50HZ:
        internalValue = ::ANTI_BANDING_50HZ;
        break;
    case ANTIBANDING_60HZ:
        internalValue = ::ANTI_BANDING_60HZ;
        break;
    case ANTIBANDING_OFF:
        internalValue = ::ANTI_BANDING_OFF;
        break;
    default:
        ALOGE("ERR(%s):Unsupported value(%d)", __func__, value);
        return false;
        break;
    }

    if (m_internalISP == true) {
        if (internalValue < ::IS_AFC_DISABLE || ::IS_AFC_MAX <= internalValue) {
            ALOGE("ERR(%s):Invalid value (%d)", __func__, value);
            return false;
        }
    } else {
        if (internalValue < ::ANTI_BANDING_AUTO || ::ANTI_BANDING_OFF < internalValue) {
            ALOGE("ERR(%s):Invalid internalValue (%d)", __func__, internalValue);
            return false;
        }
    }

    if (m_curCameraInfo->antiBanding != value) {
        m_curCameraInfo->antiBanding = value;
        if (m_flagCreate == true) {
            if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_IS_CAMERA_AFC_MODE, internalValue) < 0) {
                ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                return false;
            }
        }
    }

    return true;
}

bool ExynosCamera::setAutoExposureLock(bool toggle)
{
    int internalValue = -1;

    if (m_curCameraInfo->autoExposureLock == toggle)
        return true;

    m_curCameraInfo->autoExposureLock = toggle;

    if (m_curCameraInfo->autoExposureLock == true && m_curCameraInfo->autoWhiteBalanceLock == true)
        internalValue = AE_LOCK_AWB_LOCK;
    else if (m_curCameraInfo->autoExposureLock == true && m_curCameraInfo->autoWhiteBalanceLock == false)
        internalValue = AE_LOCK_AWB_UNLOCK;
    else if (m_curCameraInfo->autoExposureLock == false && m_curCameraInfo->autoWhiteBalanceLock == true)
        internalValue = AE_UNLOCK_AWB_LOCK;
    else // if (m_curCameraInfo->autoExposureLock == false && m_curCameraInfo->autoWhiteBalanceLock == false)
        internalValue = AE_UNLOCK_AWB_UNLOCK;

    if (m_flagCreate == true) {
        if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_CAMERA_AEAWB_LOCK_UNLOCK, internalValue) < 0) {
            ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
            return false;
        }
    }
    return true;
}

bool ExynosCamera::setAutoWhiteBalanceLock(bool toggle)
{
    int internalValue = -1;

    if (m_curCameraInfo->autoWhiteBalanceLock == toggle)
        return true;

    m_curCameraInfo->autoWhiteBalanceLock = toggle;

    if (m_curCameraInfo->autoExposureLock == true && m_curCameraInfo->autoWhiteBalanceLock == true)
        internalValue = AE_LOCK_AWB_LOCK;
    else if (m_curCameraInfo->autoExposureLock == true && m_curCameraInfo->autoWhiteBalanceLock == false)
        internalValue = AE_LOCK_AWB_UNLOCK;
    else if (m_curCameraInfo->autoExposureLock == false && m_curCameraInfo->autoWhiteBalanceLock == true)
        internalValue = AE_UNLOCK_AWB_LOCK;
    else // if (m_curCameraInfo->autoExposureLock == false && m_curCameraInfo->autoWhiteBalanceLock == false)
        internalValue = AE_UNLOCK_AWB_UNLOCK;

    if (m_flagCreate == true) {
        if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_CAMERA_AEAWB_LOCK_UNLOCK, internalValue) < 0) {
            ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
            return false;
        }
    }
    return true;
}

bool ExynosCamera::setColorEffect(int value)
{
    int internalValue = -1;

    switch (value) {
    case EFFECT_NONE:
        if (m_internalISP == true)
            internalValue = ::IS_IMAGE_EFFECT_DISABLE;
        else
            internalValue = ::IMAGE_EFFECT_NONE;
        break;
    case EFFECT_MONO:
        if (m_internalISP == true)
            internalValue = ::IS_IMAGE_EFFECT_MONOCHROME;
        else
            internalValue = ::IMAGE_EFFECT_BNW;
        break;
    case EFFECT_NEGATIVE:
        internalValue = IS_IMAGE_EFFECT_NEGATIVE_MONO;
        break;
    case EFFECT_SEPIA:
        if (m_internalISP == true)
            internalValue = ::IS_IMAGE_EFFECT_SEPIA;
        else
            internalValue = ::IMAGE_EFFECT_SEPIA;
        break;
    case EFFECT_AQUA:
    case EFFECT_SOLARIZE:
    case EFFECT_POSTERIZE:
    case EFFECT_WHITEBOARD:
    case EFFECT_BLACKBOARD:
    default:
        ALOGE("ERR(%s):Unsupported value(%d)", __func__, value);
        return false;
        break;
    }

    if (m_internalISP == true) {
        if (internalValue < ::IS_IMAGE_EFFECT_DISABLE || ::IS_IMAGE_EFFECT_MAX <= internalValue) {
            ALOGE("ERR(%s):Invalid internalValue(%d)", __func__, internalValue);
            return false;
        }
    } else {
        if (internalValue <= ::IMAGE_EFFECT_BASE || ::IMAGE_EFFECT_MAX <= internalValue) {
            ALOGE("ERR(%s):Invalid internalValue(%d)", __func__, internalValue);
            return false;
        }
    }

    if (m_curCameraInfo->effect != value) {
        m_curCameraInfo->effect = value;
        if (m_flagCreate == true) {
            if (m_internalISP == true) {
                if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_IS_CAMERA_IMAGE_EFFECT, internalValue) < 0) {
                    ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                    return false;
                }
            } else {
                if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_CAMERA_EFFECT, internalValue) < 0) {
                    ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                    return false;
                }
            }
        }
    }

    return true;
}

bool ExynosCamera::setExposureCompensation(int value)
{
    int internalValue = value;

    if (m_internalISP == true) {
        internalValue += IS_EXPOSURE_DEFAULT;
        if (internalValue < IS_EXPOSURE_MINUS_2 || IS_EXPOSURE_PLUS_2 < internalValue) {
            ALOGE("ERR(%s):Invalid internalValue(%d)", __func__, internalValue);
            return false;
        }
    } else {
        internalValue += EV_DEFAULT;
        if (internalValue < EV_MINUS_4 || EV_PLUS_4 < internalValue) {
            ALOGE("ERR(%s):Invalid internalValue(%d)", __func__, internalValue);
            return false;
        }
    }

    if (m_curCameraInfo->exposure != value) {
        m_curCameraInfo->exposure = value;
        if (m_flagCreate == true) {
            if (m_internalISP == true) {
                if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_IS_CAMERA_EXPOSURE, internalValue) < 0) {
                    ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                    return false;
                }
            } else {
                if (this->setBrightness(value) == false) {
                    ALOGE("ERR(%s):setBrightness() fail", __func__);
                    return false;
                }
            }
        }
    }

    return true;
}

bool ExynosCamera::setFlashMode(int value)
{
    int internalValue = -1;

    switch (value) {
    case FLASH_MODE_OFF:
        internalValue = ::FLASH_MODE_OFF;
        break;
    case FLASH_MODE_AUTO:
        internalValue = ::FLASH_MODE_AUTO;
        break;
    case FLASH_MODE_ON:
        internalValue = ::FLASH_MODE_ON;
        break;
    case FLASH_MODE_TORCH:
        internalValue = ::FLASH_MODE_TORCH;
        break;
    case FLASH_MODE_RED_EYE:
    default:
        ALOGE("ERR(%s):Unsupported value(%d)", __func__, value);
        return false;
        break;
    }

    if (internalValue <= ::FLASH_MODE_BASE || ::FLASH_MODE_MAX <= internalValue) {
        ALOGE("ERR(%s):Invalid value (%d)", __func__, value);
        return false;
    }

    if (m_curCameraInfo->flashMode != value) {
        m_curCameraInfo->flashMode = value;
        if (m_flagCreate == true) {
            if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_CAMERA_FLASH_MODE, internalValue) < 0) {
                ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                return false;
            }
        }
    }

    return true;
}

bool ExynosCamera::setFocusAreas(int num, ExynosRect* rects, int *weights)
{
    if (m_defaultCameraInfo->maxNumFocusAreas == 0) {
        ALOGV("DEBUG(%s):maxNumFocusAreas is 0. so, ignored", __func__);
        return true;
    }

    bool ret = true;

    ExynosRect2 *rect2s = new ExynosRect2[num];
    for (int i = 0; i < num; i++)
        m_secRect2SecRect2(&rects[i], &rect2s[i]);

    ret = setFocusAreas(num, rect2s, weights);

    delete [] rect2s;

    return ret;
}

bool ExynosCamera::setFocusAreas(int num, ExynosRect2* rect2s, int *weights)
{
    if (m_defaultCameraInfo->maxNumFocusAreas == 0) {
        ALOGV("DEBUG(%s):maxNumFocusAreas is 0. so, ignored", __func__);
        return true;
    }

    int new_x = 0;
    int new_y = 0;

    if (m_defaultCameraInfo->maxNumFocusAreas < num)
        num = m_defaultCameraInfo->maxNumFocusAreas;

    if (m_flagCreate == true) {
        for (int i = 0; i < num; i++) {
            if (   num == 1
                && rect2s[0].x1 == 0
                && rect2s[0].y1 == 0
                && rect2s[0].x2 == m_curCameraInfo->previewW
                && rect2s[0].y2 == m_curCameraInfo->previewH)  {
                // TODO : driver decide focus areas -> focus center.
                new_x = (m_curCameraInfo->previewW) / 2;
                new_y = (m_curCameraInfo->previewH) / 2;
            } else {
                new_x = (rect2s[i].x1 + rect2s[i].x2) / 2;
                new_y = (rect2s[i].y1 + rect2s[i].y2) / 2;
            }

            m_touchAFMode = true;
            if (   exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_CAMERA_OBJECT_POSITION_X, new_x) < 0
                && exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_CAMERA_OBJECT_POSITION_Y, new_y) < 0) {
                ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                return false;
            }
        }
    }

    return true;
}

bool ExynosCamera::setFocusMode(int value)
{
    int internalValue = -1;

    switch (value) {
    case FOCUS_MODE_AUTO:
        internalValue = ::FOCUS_MODE_AUTO;
        m_touchAFMode = false;
        break;
    case FOCUS_MODE_INFINITY:
        internalValue = ::FOCUS_MODE_INFINITY;
        m_touchAFMode = false;
        break;
    case FOCUS_MODE_MACRO:
        internalValue = ::FOCUS_MODE_MACRO;
        m_touchAFMode = false;
        break;
    case FOCUS_MODE_CONTINUOUS_VIDEO:
    case FOCUS_MODE_CONTINUOUS_PICTURE:
        internalValue = ::FOCUS_MODE_CONTINOUS;
        m_touchAFMode = false;
        break;
    case FOCUS_MODE_TOUCH:
        internalValue = ::FOCUS_MODE_TOUCH;
        m_touchAFMode = true;
        break;
    case FOCUS_MODE_FIXED:
        internalValue = ::FOCUS_MODE_FIXED;
        m_touchAFMode = false;
        break;
    case FOCUS_MODE_EDOF:
    default:
        m_touchAFMode = false;
        ALOGE("ERR(%s):Unsupported value(%d)", __func__, value);
        return false;
        break;
    }

    if (::FOCUS_MODE_MAX <= internalValue) {
        ALOGE("ERR(%s):Invalid internalValue (%d)", __func__, internalValue);
        return false;
    }

    if (m_curCameraInfo->focusMode != value) {
        m_curCameraInfo->focusMode = value;
        if (m_flagCreate == true) {
            if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_CAMERA_FOCUS_MODE, internalValue) < 0) {
                ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                return false;
            }
        }
    }

    return true;
}

bool ExynosCamera::setGpsAltitude(const char *gpsAltitude)
{
    double conveted_altitude = 0;

    if (gpsAltitude == NULL)
        m_curCameraInfo->gpsAltitude = 0;
    else {
        conveted_altitude = atof(gpsAltitude);
        m_curCameraInfo->gpsAltitude = (long)(conveted_altitude * 100 / 1);
    }

    return true;
}

bool ExynosCamera::setGpsLatitude(const char *gpsLatitude)
{
    double conveted_latitude = 0;

    if (gpsLatitude == NULL)
        m_curCameraInfo->gpsLatitude = 0;
    else {
        conveted_latitude = atof(gpsLatitude);
        m_curCameraInfo->gpsLatitude = (long)(conveted_latitude * 10000 / 1);
    }

    return true;
}

bool ExynosCamera::setGpsLongitude(const char *gpsLongitude)
{
    double conveted_longitude = 0;

    if (gpsLongitude == NULL)
        m_curCameraInfo->gpsLongitude = 0;
    else {
        conveted_longitude = atof(gpsLongitude);
        m_curCameraInfo->gpsLongitude = (long)(conveted_longitude * 10000 / 1);
    }

    return true;
}

bool ExynosCamera::setGpsProcessingMethod(const char *gpsProcessingMethod)
{
    memset(mExifInfo.gps_processing_method, 0, sizeof(mExifInfo.gps_processing_method));

    if (gpsProcessingMethod != NULL) {
        size_t len = strlen(gpsProcessingMethod);
        if (len > sizeof(mExifInfo.gps_processing_method)) {
            len = sizeof(mExifInfo.gps_processing_method);
        }
        memcpy(mExifInfo.gps_processing_method, gpsProcessingMethod, len);
    }

    return true;
}

bool ExynosCamera::setGpsTimeStamp(const char *gpsTimestamp)
{
    if (gpsTimestamp == NULL)
        m_curCameraInfo->gpsTimestamp = 0;
    else
        m_curCameraInfo->gpsTimestamp = atol(gpsTimestamp);

    return true;
}

bool ExynosCamera::setJpegQuality(int quality)
{
    if (quality < JPEG_QUALITY_MIN || JPEG_QUALITY_MAX < quality) {
        ALOGE("ERR(%s):Invalid quality (%d)", __func__, quality);
        return false;
    }

    m_jpegQuality = quality;

    return true;
}

bool ExynosCamera::setJpegThumbnailQuality(int quality)
{
    if (quality < JPEG_QUALITY_MIN || JPEG_QUALITY_MAX < quality) {
        ALOGE("ERR(%s):Invalid quality (%d)", __func__, quality);
        return false;
    }

    m_jpegThumbnailQuality = quality;

    return true;
}

bool ExynosCamera::setJpegThumbnailSize(int w, int h)
{
    m_curCameraInfo->thumbnailW = w;
    m_curCameraInfo->thumbnailH = h;
    return true;
}

bool ExynosCamera::setMeteringAreas(int num, ExynosRect *rects, int *weights)
{
    if (m_defaultCameraInfo->maxNumMeteringAreas == 0) {
        ALOGV("DEBUG(%s):maxNumMeteringAreas is 0. so, ignored", __func__);
        return true;
    }

    if (m_defaultCameraInfo->maxNumMeteringAreas < num)
        num = m_defaultCameraInfo->maxNumMeteringAreas;

    if (m_flagCreate == true) {
        for (int i = 0; i < num; i++) {
            if (   exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_IS_CAMERA_METERING_POSITION_X, rects[i].x) < 0
                && exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_IS_CAMERA_METERING_POSITION_Y, rects[i].y) < 0
                && exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_IS_CAMERA_METERING_WINDOW_X,   rects[i].w) < 0
                && exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_IS_CAMERA_METERING_WINDOW_Y,   rects[i].h) < 0) {
                ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                return false;
            }
        }
    }

    return true;
}

bool ExynosCamera::setMeteringAreas(int num, ExynosRect2 *rect2s, int *weights)
{
    if (m_defaultCameraInfo->maxNumMeteringAreas == 0) {
        ALOGV("DEBUG(%s):maxNumMeteringAreas is 0. so, ignored", __func__);
        return true;
    }

    bool ret = true;

    ExynosRect *rects = new ExynosRect[num];
    for (int i = 0; i < num; i++)
        m_secRect22SecRect(&rect2s[i], &rects[i]);

    /* FIXME: Currnetly HW dose not support metering area */
    //ret = setMeteringAreas(num, rects, weights);

    delete [] rects;

    return ret;
}

bool ExynosCamera::setPictureFormat(int colorFormat)
{
    m_curCameraInfo->pictureColorFormat = colorFormat;

#if defined(LOG_NDEBUG) && LOG_NDEBUG == 0
    m_printFormat(m_curCameraInfo->pictureColorFormat, "PictureFormat");
#endif
    return true;
}

bool ExynosCamera::setPictureSize(int w, int h)
{
    m_curCameraInfo->pictureW = w;
    m_curCameraInfo->pictureH = h;

    // HACK : Camera cannot support zoom. So, we must make max size picture w, h
    m_curCameraInfo->pictureW = m_defaultCameraInfo->pictureW;
    m_curCameraInfo->pictureH = m_defaultCameraInfo->pictureH;

    return true;
}

bool ExynosCamera::setPreviewFormat(int colorFormat)
{
    m_curCameraInfo->previewColorFormat = colorFormat;

#if defined(LOG_NDEBUG) && LOG_NDEBUG == 0
    m_printFormat(m_curCameraInfo->previewColorFormat, "PreviewtFormat");
#endif

    return true;
}

bool ExynosCamera::setPreviewFrameRate(int fps)
{
    if (fps < FRAME_RATE_AUTO || FRAME_RATE_MAX < fps)
        ALOGE("ERR(%s):Invalid fps(%d)", __func__, fps);

    if (m_flagCreate == true) {
        m_curCameraInfo->fps = fps;
        if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_CAMERA_FRAME_RATE, fps) < 0) {
            ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
            return false;
        }
    }

    return true;
}

bool ExynosCamera::setPreviewSize(int w, int h)
{
    m_curCameraInfo->previewW = w;
    m_curCameraInfo->previewH = h;
    return true;
}

bool ExynosCamera::setRecordingHint(bool hint)
{
    // TODO : fixed fps?
    /* DIS is only possible recording hint is true. */
    m_recordingHint = hint;
    return true;
}

bool ExynosCamera::setRotation(int rotation)
{
     if (rotation < 0) {
         ALOGE("ERR(%s):Invalid rotation (%d)", __func__, rotation);
         return false;
     }
     m_curCameraInfo->rotation = rotation;

     return true;
}

int ExynosCamera::getRotation(void)
{
    return m_curCameraInfo->rotation;
}

bool ExynosCamera::setSceneMode(int value)
{
    int internalValue = -1;

    switch (value) {
    case SCENE_MODE_AUTO:
        internalValue = ::SCENE_MODE_NONE;
        break;
    case SCENE_MODE_PORTRAIT:
        internalValue = ::SCENE_MODE_PORTRAIT;
        break;
    case SCENE_MODE_LANDSCAPE:
        internalValue = ::SCENE_MODE_LANDSCAPE;
        break;
    case SCENE_MODE_NIGHT:
        internalValue = ::SCENE_MODE_NIGHTSHOT;
        break;
    case SCENE_MODE_BEACH:
        internalValue = ::SCENE_MODE_BEACH_SNOW;
        break;
    case SCENE_MODE_SNOW:
        internalValue = ::SCENE_MODE_BEACH_SNOW;
        break;
    case SCENE_MODE_SUNSET:
        internalValue = ::SCENE_MODE_SUNSET;
        break;
    case SCENE_MODE_FIREWORKS:
        internalValue = ::SCENE_MODE_FIREWORKS;
        break;
    case SCENE_MODE_SPORTS:
        internalValue = ::SCENE_MODE_SPORTS;
        break;
    case SCENE_MODE_PARTY:
        internalValue = ::SCENE_MODE_PARTY_INDOOR;
        break;
    case SCENE_MODE_CANDLELIGHT:
        internalValue = ::SCENE_MODE_CANDLE_LIGHT;
        break;
    case SCENE_MODE_STEADYPHOTO:
        internalValue = ::SCENE_MODE_TEXT;
        break;
    case SCENE_MODE_ACTION:
    case SCENE_MODE_NIGHT_PORTRAIT:
    case SCENE_MODE_THEATRE:
    default:
        ALOGE("ERR(%s):Unsupported value(%d)", __func__, value);
        return false;
        break;
    }

    if (internalValue <= ::SCENE_MODE_BASE || ::SCENE_MODE_MAX <= internalValue) {
        ALOGE("ERR(%s):Invalid value (%d)", __func__, internalValue);
        return false;
    }

    if (m_curCameraInfo->sceneMode != value) {
        m_curCameraInfo->sceneMode = value;
        if (m_flagCreate == true) {
            if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_CAMERA_SCENE_MODE, internalValue) < 0) {
                ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                return false;
            }
        }
    }

    return true;
}

bool ExynosCamera::setVideoStabilization(bool toggle)
{
    m_curCameraInfo->videoStabilization = toggle;

    if (m_previewDev->flagStart == true) {
        if (m_curCameraInfo->applyVideoStabilization != toggle) {

            int dis = (toggle == true) ? CAMERA_DIS_ON : CAMERA_DIS_OFF;

            if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_CAMERA_SET_DIS, dis) < 0) {
                ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                return false;
            } else {
                m_curCameraInfo->applyVideoStabilization = toggle;
	    }
        }
    }
    return true;
}

bool ExynosCamera::setWhiteBalance(int value)
{
    int internalValue = -1;

    switch (value) {
    case WHITE_BALANCE_AUTO:
        if (m_internalISP == true)
            internalValue = ::IS_AWB_AUTO;
        else
            internalValue = ::WHITE_BALANCE_AUTO;
        break;
    case WHITE_BALANCE_INCANDESCENT:
        if (m_internalISP == true)
            internalValue = ::IS_AWB_TUNGSTEN;
        else
            internalValue = ::WHITE_BALANCE_TUNGSTEN;
        break;
    case WHITE_BALANCE_FLUORESCENT:
        if (m_internalISP == true)
            internalValue = ::IS_AWB_FLUORESCENT;
        else
            internalValue = ::WHITE_BALANCE_FLUORESCENT;
        break;
    case WHITE_BALANCE_DAYLIGHT:
        if (m_internalISP == true)
            internalValue = ::IS_AWB_DAYLIGHT;
        else
            internalValue = ::WHITE_BALANCE_SUNNY;
        break;
    case WHITE_BALANCE_CLOUDY_DAYLIGHT:
        if (m_internalISP == true)
            internalValue = ::IS_AWB_CLOUDY;
        else
            internalValue = ::WHITE_BALANCE_CLOUDY;
        break;
    case WHITE_BALANCE_WARM_FLUORESCENT:
    case WHITE_BALANCE_TWILIGHT:
    case WHITE_BALANCE_SHADE:
    default:
        ALOGE("ERR(%s):Unsupported value(%d)", __func__, value);
        return false;
        break;
    }

    if (m_internalISP == true) {
        if (internalValue < ::IS_AWB_AUTO || ::IS_AWB_MAX <= internalValue) {
            ALOGE("ERR(%s):Invalid internalValue(%d)", __func__, internalValue);
            return false;
        }
    } else {
        if (internalValue <= ::WHITE_BALANCE_BASE || ::WHITE_BALANCE_MAX <= internalValue) {
            ALOGE("ERR(%s):Invalid internalValue(%d)", __func__, internalValue);
            return false;
        }
    }

    if (m_curCameraInfo->whiteBalance != value) {
        m_curCameraInfo->whiteBalance = value;
        if (m_flagCreate == true) {
            if (m_internalISP == true) {
                if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_IS_CAMERA_AWB_MODE, internalValue) < 0) {
                    ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                    return false;
                }
            } else {
                if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_CAMERA_WHITE_BALANCE, internalValue) < 0) {
                    ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                    return false;
                }
            }
        }
    }

    return true;
}

bool ExynosCamera::setZoom(int value)
{
    if (value < ZOOM_LEVEL_0 || ZOOM_LEVEL_MAX <= value) {
        ALOGE("ERR(%s):Invalid value (%d)", __func__, value);
        return false;
    }

    if (m_curCameraInfo->zoom != value) {
        m_curCameraInfo->zoom = value;
        if (m_defaultCameraInfo->hwZoomSupported == true) {
            if (m_flagCreate == true) {
                if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_CAMERA_ZOOM, value) < 0) {
                    ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                    return false;
                }
            }
        } else {
            if (m_setZoom(m_previewDev->fd, m_curCameraInfo->zoom, m_curCameraInfo->previewW, m_curCameraInfo->previewH) == false) {
                ALOGE("ERR(%s):m_setZoom(%d) fail", __func__, m_curCameraInfo->zoom);
                return false;
            }
        }
    }

    return true;
}

bool ExynosCamera::m_setWidthHeight(int mode,
                                 int fd,
                                 struct pollfd *event,
                                 int w,
                                 int h,
                                 int colorFormat,
                                 struct ExynosBuffer *buf,
                                 bool *validBuf)
{
    // Get and throw away the first frame since it is often garbled.
    memset(event, 0, sizeof(struct pollfd));
    event->fd = fd;
    event->events = POLLIN | POLLERR;

    int numOfBuf = 0;

    for (int i = 0; i < VIDEO_MAX_FRAME; i++) {
        if (buf[i].virt.p != NULL || buf[i].phys.p != 0 || 
	    buf[i].fd.fd >= 0) {
            validBuf[i] = true;
            numOfBuf++;
        } else {
            validBuf[i] = false;
        }
    }

    struct v4l2_format v4l2_fmt;
    struct v4l2_pix_format pixfmt;
    unsigned int bpp;
    unsigned int planes;

    memset(&v4l2_fmt, 0, sizeof(struct v4l2_format));
    memset(&pixfmt, 0, sizeof(pixfmt));

    switch(mode) {
    case PREVIEW_MODE:
    case VIDEO_MODE:
        v4l2_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

        V4L2_PIX_2_YUV_INFO(colorFormat, &bpp, &planes);

        v4l2_fmt.fmt.pix_mp.width = w;
        v4l2_fmt.fmt.pix_mp.height = h;
        v4l2_fmt.fmt.pix_mp.pixelformat = colorFormat;
        v4l2_fmt.fmt.pix_mp.num_planes = planes;

        if (exynos_v4l2_s_fmt(fd, &v4l2_fmt) < 0) {
            ALOGE("ERR(%s):exynos_v4l2_s_fmt() fail", __func__);
            return false;
        }
        break;
    case PICTURE_MODE:
        v4l2_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

        pixfmt.width = w;
        pixfmt.height = h;
        pixfmt.pixelformat = colorFormat;
        if (pixfmt.pixelformat == V4L2_PIX_FMT_JPEG)
            pixfmt.colorspace = V4L2_COLORSPACE_JPEG;

        v4l2_fmt.fmt.pix = pixfmt;

        if (exynos_v4l2_s_fmt(fd, &v4l2_fmt) < 0) {
            ALOGE("ERR(%s):exynos_v4l2_s_fmt() fail", __func__);
            return false;
        }
        break;
    default:
        break;
    }

    struct v4l2_requestbuffers req;
    req.count  = numOfBuf;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    req.memory = V4L2_MEMORY_DMABUF;

    if (exynos_v4l2_reqbufs(fd, &req) < 0) {
        ALOGE("ERR(%s):exynos_v4l2_reqbufs(%d) fail", __func__, numOfBuf);
        return false;
    }

    for (int i = 0; i < VIDEO_MAX_FRAME; i++) {
        if (validBuf[i] == true) {

            struct v4l2_buffer v4l2_buf;
            struct v4l2_plane planes[VIDEO_MAX_PLANES];

            v4l2_buf.m.planes = planes;
            v4l2_buf.type     = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
            v4l2_buf.memory   = V4L2_MEMORY_DMABUF;
            v4l2_buf.index    = buf[i].reserved.p;
            v4l2_buf.length   = 0;

            for (int j = 0; j < 3; j++) {
                v4l2_buf.m.planes[j].m.fd = buf[i].fd.extFd[j];
                v4l2_buf.m.planes[j].length   = buf[i].size.extS[j];

                if (buf[i].size.extS[j] != 0)
                    v4l2_buf.length++;
            }

            if (exynos_v4l2_qbuf(fd, &v4l2_buf) < 0) {
                ALOGE("ERR(%s):exynos_v4l2_qbuf(%d) fail", __func__, i);
                return false;
            }
        }
    }

    /*
    m_currentZoom = -1;

    if (m_setZoom(fd, m_curCameraInfo->zoom, w, h) == false)
        ALOGE("ERR(%s):m_setZoom(%d, %d) fail", __func__, mode, m_curCameraInfo->zoom);
    */
    return true;
}

bool ExynosCamera::m_setZoom(int fd, int zoom, int w, int h)
{
    int ret = true;

    if (m_currentZoom != zoom) {
        m_currentZoom = zoom;

        int real_zoom = 0;

        if (m_defaultCameraInfo->hwZoomSupported == true)
            real_zoom = 0; // just adjust ratio, not digital zoom.
        else
            real_zoom = zoom; // adjust ratio, digital zoom

        ret = m_setCrop(fd, w, h, real_zoom);
        if (ret == false)
            ALOGE("ERR(%s):m_setCrop(%d, %d) fail", __func__, w, h);
    }

    return ret;
}

bool ExynosCamera::m_setCrop(int fd, int w, int h, int zoom)
{
    v4l2_cropcap cropcap;
    v4l2_crop crop;
    unsigned int crop_x = 0;
    unsigned int crop_y = 0;
    unsigned int crop_w = 0;
    unsigned int crop_h = 0;

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (exynos_v4l2_cropcap(fd, &cropcap) < 0)  {
        ALOGE("ERR(%s):exynos_v4l2_cropcap() fail)", __func__);
        return false;
    }

    m_getCropRect(cropcap.bounds.width, cropcap.bounds.height,
                  w,                    h,
                  &crop_x,              &crop_y,
                  &crop_w,              &crop_h,
                  zoom);

    cropcap.defrect.left   = crop_x;
    cropcap.defrect.top    = crop_y;
    cropcap.defrect.width  = crop_w;
    cropcap.defrect.height = crop_h;
    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    crop.c    = cropcap.defrect;

    if (exynos_v4l2_s_crop(fd, &crop) < 0) {
        ALOGE("ERR(%s):exynos_v4l2_s_crop() fail(%d))", __func__, zoom);
        return false;
    }

    /*
    ALOGD("## 1 w                     : %d", w);
    ALOGD("## 1 h                     : %d", h);
    ALOGD("## 1 zoom                  : %d", zoom);
    ALOGD("## 1 cropcap.bounds.w      : %d", cropcap.bounds.width);
    ALOGD("## 1 cropcap.bounds.h      : %d", cropcap.bounds.height);
    ALOGD("## 2 crop_x                : %d", crop_x);
    ALOGD("## 2 crop_y                : %d", crop_y);
    ALOGD("## 2 crop_w                : %d", crop_w);
    ALOGD("## 2 crop_h                : %d", crop_h);
    ALOGD("## 2 cropcap.defrect.left  : %d", cropcap.defrect.left);
    ALOGD("## 2 cropcap.defrect.top   : %d", cropcap.defrect.top);
    ALOGD("## 2 cropcap.defrect.width : %d", cropcap.defrect.width);
    ALOGD("## 2 cropcap.defrect.height: %d", cropcap.defrect.height);
    */

    return true;
}

bool ExynosCamera::m_getCropRect(unsigned int  src_w,  unsigned int   src_h,
                              unsigned int  dst_w,  unsigned int   dst_h,
                              unsigned int *crop_x, unsigned int *crop_y,
                              unsigned int *crop_w, unsigned int *crop_h,
                              int           zoom)
{
    #define DEFAULT_ZOOM_RATIO        (4) // 4x zoom
    #define DEFAULT_ZOOM_RATIO_SHIFT  (2)
    int max_zoom = m_defaultCameraInfo->maxZoom;

    *crop_w = src_w;
    *crop_h = src_h;

    if (   src_w != dst_w
        || src_h != dst_h) {
        float src_ratio = 1.0f;
        float dst_ratio = 1.0f;

        // ex : 1024 / 768
        src_ratio = (float)src_w / (float)src_h;

        // ex : 352  / 288
        dst_ratio = (float)dst_w / (float)dst_h;

        if (src_ratio != dst_ratio) {
            if (src_ratio <= dst_ratio) {
                // shrink h
                *crop_w = src_w;
                *crop_h = src_w / dst_ratio;
            } else  { //(src_ratio > dst_ratio)
                // shrink w
                *crop_w = src_h * dst_ratio;
                *crop_h = src_h;
            }
        }

        if (zoom != 0) {
            unsigned int zoom_w_step =
                        (*crop_w - (*crop_w  >> DEFAULT_ZOOM_RATIO_SHIFT)) / max_zoom;

            *crop_w  = *crop_w - (zoom_w_step * zoom);

            unsigned int zoom_h_step =
                        (*crop_h - (*crop_h >> DEFAULT_ZOOM_RATIO_SHIFT)) / max_zoom;

            *crop_h = *crop_h - (zoom_h_step * zoom);
        }
    }

    #define CAMERA_CROP_WIDTH_RESTRAIN_NUM  (0x10) // 16
    unsigned int w_align = (*crop_w & (CAMERA_CROP_WIDTH_RESTRAIN_NUM - 1));
    if (w_align != 0) {
        if (  (CAMERA_CROP_WIDTH_RESTRAIN_NUM >> 1) <= w_align
            && *crop_w + (CAMERA_CROP_WIDTH_RESTRAIN_NUM - w_align) <= dst_w) {
            *crop_w += (CAMERA_CROP_WIDTH_RESTRAIN_NUM - w_align);
        }
        else
            *crop_w -= w_align;
    }

    #define CAMERA_CROP_HEIGHT_RESTRAIN_NUM  (0x2) // 2
    unsigned int h_align = (*crop_h & (CAMERA_CROP_HEIGHT_RESTRAIN_NUM - 1));
    if (h_align != 0) {
        if (  (CAMERA_CROP_HEIGHT_RESTRAIN_NUM >> 1) <= h_align
            && *crop_h + (CAMERA_CROP_HEIGHT_RESTRAIN_NUM - h_align) <= dst_h) {
            *crop_h += (CAMERA_CROP_HEIGHT_RESTRAIN_NUM - h_align);
        }
        else
            *crop_h -= h_align;
    }

    *crop_x = (src_w - *crop_w) >> 1;
    *crop_y = (src_h - *crop_h) >> 1;

    return true;
}

void ExynosCamera::m_setExifFixedAttribute(void)
{
    char property[PROPERTY_VALUE_MAX];

    //2 0th IFD TIFF Tags
    //3 Maker
    property_get("ro.product.brand", property, EXIF_DEF_MAKER);
    strncpy((char *)mExifInfo.maker, property,
                sizeof(mExifInfo.maker) - 1);
    mExifInfo.maker[sizeof(mExifInfo.maker) - 1] = '\0';
    //3 Model
    property_get("ro.product.model", property, EXIF_DEF_MODEL);
    strncpy((char *)mExifInfo.model, property,
                sizeof(mExifInfo.model) - 1);
    mExifInfo.model[sizeof(mExifInfo.model) - 1] = '\0';
    //3 Software
    property_get("ro.build.id", property, EXIF_DEF_SOFTWARE);
    strncpy((char *)mExifInfo.software, property,
                sizeof(mExifInfo.software) - 1);
    mExifInfo.software[sizeof(mExifInfo.software) - 1] = '\0';

    //3 YCbCr Positioning
    mExifInfo.ycbcr_positioning = EXIF_DEF_YCBCR_POSITIONING;

    //2 0th IFD Exif Private Tags
    //3 F Number
    mExifInfo.fnumber.num = EXIF_DEF_FNUMBER_NUM;
    mExifInfo.fnumber.den = EXIF_DEF_FNUMBER_DEN;
    //3 Exposure Program
    mExifInfo.exposure_program = EXIF_DEF_EXPOSURE_PROGRAM;
    //3 Exif Version
    memcpy(mExifInfo.exif_version, EXIF_DEF_EXIF_VERSION, sizeof(mExifInfo.exif_version));
    //3 Aperture
    uint32_t av = APEX_FNUM_TO_APERTURE((double)mExifInfo.fnumber.num/mExifInfo.fnumber.den);
    mExifInfo.aperture.num = av*EXIF_DEF_APEX_DEN;
    mExifInfo.aperture.den = EXIF_DEF_APEX_DEN;
    //3 Maximum lens aperture
    mExifInfo.max_aperture.num = mExifInfo.aperture.num;
    mExifInfo.max_aperture.den = mExifInfo.aperture.den;
    //3 Lens Focal Length
    mExifInfo.focal_length.num = m_defaultCameraInfo->focalLengthNum;
    mExifInfo.focal_length.den = m_defaultCameraInfo->focalLengthDen;
    //3 User Comments
    strcpy((char *)mExifInfo.user_comment, EXIF_DEF_USERCOMMENTS);
    //3 Color Space information
    mExifInfo.color_space = EXIF_DEF_COLOR_SPACE;
    //3 Exposure Mode
    mExifInfo.exposure_mode = EXIF_DEF_EXPOSURE_MODE;

    //2 0th IFD GPS Info Tags
    unsigned char gps_version[4] = { 0x02, 0x02, 0x00, 0x00 };
    memcpy(mExifInfo.gps_version_id, gps_version, sizeof(gps_version));

    //2 1th IFD TIFF Tags
    mExifInfo.compression_scheme = EXIF_DEF_COMPRESSION;
    mExifInfo.x_resolution.num = EXIF_DEF_RESOLUTION_NUM;
    mExifInfo.x_resolution.den = EXIF_DEF_RESOLUTION_DEN;
    mExifInfo.y_resolution.num = EXIF_DEF_RESOLUTION_NUM;
    mExifInfo.y_resolution.den = EXIF_DEF_RESOLUTION_DEN;
    mExifInfo.resolution_unit = EXIF_DEF_RESOLUTION_UNIT;
}

void ExynosCamera::m_setExifChangedAttribute(exif_attribute_t *exifInfo, ExynosRect *rect)
{
    //2 0th IFD TIFF Tags
    //3 Width
    exifInfo->width = rect->w;
    //3 Height
    exifInfo->height = rect->h;
    //3 Orientation
    switch (m_curCameraInfo->rotation) {
    case 90:
        exifInfo->orientation = EXIF_ORIENTATION_90;
        break;
    case 180:
        exifInfo->orientation = EXIF_ORIENTATION_180;
        break;
    case 270:
        exifInfo->orientation = EXIF_ORIENTATION_270;
        break;
    case 0:
    default:
        exifInfo->orientation = EXIF_ORIENTATION_UP;
        break;
    }
    //3 Date time
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime((char *)exifInfo->date_time, 20, "%Y:%m:%d %H:%M:%S", timeinfo);

    //2 0th IFD Exif Private Tags
    //3 Exposure Time
    int shutterSpeed = 100;
    /* TBD - front camera needs to be fixed to support this g_ctrl,
       it current returns a negative err value, so avoid putting
       odd value into exif for now */
    if (   exynos_v4l2_g_ctrl(m_previewDev->fd, V4L2_CID_IS_CAMERA_EXIF_SHUTTERSPEED, &shutterSpeed) < 0
        || shutterSpeed < 0) {
        ALOGE("ERR(%s):exynos_v4l2_g_ctrl() fail, using 100", __func__);
        shutterSpeed = 100;
    }

    exifInfo->exposure_time.num = 1;
    // x us -> 1/x s */
    exifInfo->exposure_time.den = (uint32_t)(1000000 / shutterSpeed);

    //3 ISO Speed Rating
    int iso = m_curCameraInfo->iso;

    /* TBD - front camera needs to be fixed to support this g_ctrl,
       it current returns a negative err value, so avoid putting
       odd value into exif for now */
    if (   exynos_v4l2_g_ctrl(m_previewDev->fd, V4L2_CID_IS_CAMERA_EXIF_ISO, &iso) < 0
        || iso < 0) {
        ALOGE("ERR(%s):exynos_v4l2_g_ctrl() fail, using ISO_100", __func__);
        iso = ISO_100;
    }

    switch (iso) {
    case ISO_50:
        exifInfo->iso_speed_rating = 50;
        break;
    case ISO_100:
        exifInfo->iso_speed_rating = 100;
        break;
    case ISO_200:
        exifInfo->iso_speed_rating = 200;
        break;
    case ISO_400:
        exifInfo->iso_speed_rating = 400;
        break;
    case ISO_800:
        exifInfo->iso_speed_rating = 800;
        break;
    case ISO_1600:
        exifInfo->iso_speed_rating = 1600;
        break;
    default:
        exifInfo->iso_speed_rating = 100;
        break;
    }

    uint32_t av, tv, bv, sv, ev;
    av = APEX_FNUM_TO_APERTURE((double)exifInfo->fnumber.num / exifInfo->fnumber.den);
    tv = APEX_EXPOSURE_TO_SHUTTER((double)exifInfo->exposure_time.num / exifInfo->exposure_time.den);
    sv = APEX_ISO_TO_FILMSENSITIVITY(exifInfo->iso_speed_rating);
    bv = av + tv - sv;
    ev = av + tv;
    ALOGD("Shutter speed=%d us, iso=%d", shutterSpeed, exifInfo->iso_speed_rating);
    ALOGD("AV=%d, TV=%d, SV=%d", av, tv, sv);

    //3 Shutter Speed
    exifInfo->shutter_speed.num = tv * EXIF_DEF_APEX_DEN;
    exifInfo->shutter_speed.den = EXIF_DEF_APEX_DEN;
    //3 Brightness
    exifInfo->brightness.num = bv*EXIF_DEF_APEX_DEN;
    exifInfo->brightness.den = EXIF_DEF_APEX_DEN;
    //3 Exposure Bias
    if (m_curCameraInfo->sceneMode == SCENE_MODE_BEACH ||
        m_curCameraInfo->sceneMode == SCENE_MODE_SNOW) {
        exifInfo->exposure_bias.num = EXIF_DEF_APEX_DEN;
        exifInfo->exposure_bias.den = EXIF_DEF_APEX_DEN;
    } else {
        exifInfo->exposure_bias.num = 0;
        exifInfo->exposure_bias.den = 0;
    }
    //3 Metering Mode
    switch (m_curCameraInfo->metering) {
    case METERING_MODE_CENTER:
        exifInfo->metering_mode = EXIF_METERING_CENTER;
        break;
    case METERING_MODE_MATRIX:
        exifInfo->metering_mode = EXIF_METERING_MULTISPOT;
        break;
    case METERING_MODE_SPOT:
        exifInfo->metering_mode = EXIF_METERING_SPOT;
        break;
    case METERING_MODE_AVERAGE:
    default:
        exifInfo->metering_mode = EXIF_METERING_AVERAGE;
        break;
    }

    //3 Flash
    int flash = EXIF_DEF_FLASH;
    if (   m_curCameraInfo->flashMode == FLASH_MODE_OFF
        || exynos_v4l2_g_ctrl(m_previewDev->fd, V4L2_CID_IS_CAMERA_EXIF_FLASH, &flash) < 0
        || flash < 0)
        exifInfo->flash = EXIF_DEF_FLASH;
    else
        exifInfo->flash = flash;

    //3 White Balance
    if (m_curCameraInfo->whiteBalance == WHITE_BALANCE_AUTO)
        exifInfo->white_balance = EXIF_WB_AUTO;
    else
        exifInfo->white_balance = EXIF_WB_MANUAL;

    //3 Scene Capture Type
    switch (m_curCameraInfo->sceneMode) {
    case SCENE_MODE_PORTRAIT:
        exifInfo->scene_capture_type = EXIF_SCENE_PORTRAIT;
        break;
    case SCENE_MODE_LANDSCAPE:
        exifInfo->scene_capture_type = EXIF_SCENE_LANDSCAPE;
        break;
    case SCENE_MODE_NIGHT:
        exifInfo->scene_capture_type = EXIF_SCENE_NIGHT;
        break;
    default:
        exifInfo->scene_capture_type = EXIF_SCENE_STANDARD;
        break;
    }

    //2 0th IFD GPS Info Tags
    if (m_curCameraInfo->gpsLatitude != 0 && m_curCameraInfo->gpsLongitude != 0) {
        if (m_curCameraInfo->gpsLatitude > 0)
            strcpy((char *)exifInfo->gps_latitude_ref, "N");
        else
            strcpy((char *)exifInfo->gps_latitude_ref, "S");

        if (m_curCameraInfo->gpsLongitude > 0)
            strcpy((char *)exifInfo->gps_longitude_ref, "E");
        else
            strcpy((char *)exifInfo->gps_longitude_ref, "W");

        if (m_curCameraInfo->gpsAltitude > 0)
            exifInfo->gps_altitude_ref = 0;
        else
            exifInfo->gps_altitude_ref = 1;

        double latitude = fabs(m_curCameraInfo->gpsLatitude / 10000.0);
        double longitude = fabs(m_curCameraInfo->gpsLongitude / 10000.0);
        double altitude = fabs(m_curCameraInfo->gpsAltitude / 100.0);

        exifInfo->gps_latitude[0].num = (uint32_t)latitude;
        exifInfo->gps_latitude[0].den = 1;
        exifInfo->gps_latitude[1].num = (uint32_t)((latitude - exifInfo->gps_latitude[0].num) * 60);
        exifInfo->gps_latitude[1].den = 1;
        exifInfo->gps_latitude[2].num = (uint32_t)((((latitude - exifInfo->gps_latitude[0].num) * 60)
                                        - exifInfo->gps_latitude[1].num) * 60);
        exifInfo->gps_latitude[2].den = 1;

        exifInfo->gps_longitude[0].num = (uint32_t)longitude;
        exifInfo->gps_longitude[0].den = 1;
        exifInfo->gps_longitude[1].num = (uint32_t)((longitude - exifInfo->gps_longitude[0].num) * 60);
        exifInfo->gps_longitude[1].den = 1;
        exifInfo->gps_longitude[2].num = (uint32_t)((((longitude - exifInfo->gps_longitude[0].num) * 60)
                                        - exifInfo->gps_longitude[1].num) * 60);
        exifInfo->gps_longitude[2].den = 1;

        exifInfo->gps_altitude.num = (uint32_t)altitude;
        exifInfo->gps_altitude.den = 1;

        struct tm tm_data;
        gmtime_r(&m_curCameraInfo->gpsTimestamp, &tm_data);
        exifInfo->gps_timestamp[0].num = tm_data.tm_hour;
        exifInfo->gps_timestamp[0].den = 1;
        exifInfo->gps_timestamp[1].num = tm_data.tm_min;
        exifInfo->gps_timestamp[1].den = 1;
        exifInfo->gps_timestamp[2].num = tm_data.tm_sec;
        exifInfo->gps_timestamp[2].den = 1;
        snprintf((char*)exifInfo->gps_datestamp, sizeof(exifInfo->gps_datestamp),
                "%04d:%02d:%02d", tm_data.tm_year + 1900, tm_data.tm_mon + 1, tm_data.tm_mday);

        exifInfo->enableGps = true;
    } else {
        exifInfo->enableGps = false;
    }

    //2 1th IFD TIFF Tags
    exifInfo->widthThumb = m_curCameraInfo->thumbnailW;
    exifInfo->heightThumb = m_curCameraInfo->thumbnailH;
}

void ExynosCamera::m_secRect2SecRect2(ExynosRect *rect, ExynosRect2 *rect2)
{
    rect2->x1 = rect->x;
    rect2->y1 = rect->y;
    rect2->x2 = rect->x + rect->w;
    rect2->y2 = rect->y + rect->h;
}

void ExynosCamera::m_secRect22SecRect(ExynosRect2 *rect2, ExynosRect *rect)
{
    rect->x = rect2->x1;
    rect->y = rect2->y1;
    rect->w = rect2->x2 - rect2->x1;
    rect->h = rect2->y2 - rect2->y1;
}

void ExynosCamera::m_printFormat(int colorFormat, const char *arg)
{
    switch (colorFormat) {
    case V4L2_PIX_FMT_YUV420:
        ALOGV("DEBUG(%s):V4L2_PIX_FMT_YUV420", arg);
        break;
    case V4L2_PIX_FMT_YVU420:
        ALOGV("DEBUG(%s):V4L2_PIX_FMT_YVU420", arg);
        break;
    case V4L2_PIX_FMT_YVU420M:
        ALOGV("DEBUG(%s):V4L2_PIX_FMT_YVU420M", arg);
        break;
    case V4L2_PIX_FMT_NV12M:
        ALOGV("DEBUG(%s):V4L2_PIX_FMT_NV12M", arg);
        break;
    case V4L2_PIX_FMT_NV12:
        ALOGV("DEBUG(%s):V4L2_PIX_FMT_NV12", arg);
        break;
    case V4L2_PIX_FMT_NV12T:
        ALOGV("DEBUG(%s):V4L2_PIX_FMT_NV12T", arg);
        break;
    case V4L2_PIX_FMT_NV21:
        ALOGV("DEBUG(%s):V4L2_PIX_FMT_NV21", arg);
        break;
    case V4L2_PIX_FMT_YUV422P:
        ALOGV("DEBUG(%s):V4L2_PIX_FMT_YUV422PP", arg);
        break;
    case V4L2_PIX_FMT_YUYV:
        ALOGV("DEBUG(%s):V4L2_PIX_FMT_YUYV", arg);
        break;
    case V4L2_PIX_FMT_UYVY:
        ALOGV("DEBUG(%s):V4L2_PIX_FMT_UYVYI", arg);
        break;
    case V4L2_PIX_FMT_RGB565:
        ALOGV("DEBUG(%s):V4L2_PIX_FMT_RGB565", arg);
        break;
    default:
        ALOGV("DEBUG(%s):Unknown Format", arg);
        break;
    }
}

///////////////////////////////////////////////////
// Additional API.
///////////////////////////////////////////////////

bool ExynosCamera::setAngle(int angle)
{
    if (m_curCameraInfo->angle != angle) {
        switch (angle) {
        case -360:
        case    0:
        case  360:
            m_curCameraInfo->angle = 0;
            break;

        case -270:
        case   90:
            m_curCameraInfo->angle = 90;
            break;

        case -180:
        case  180:
            m_curCameraInfo->angle = 180;
            break;

        case  -90:
        case  270:
            m_curCameraInfo->angle = 270;
            break;

        default:
            ALOGE("ERR(%s):Invalid angle(%d)", __func__, angle);
            return false;
        }

        if (m_flagCreate == true) {
            if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_ROTATION, angle) < 0) {
                ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                return false;
            }
        }
    }

    return true;
}

int ExynosCamera::getAngle(void)
{
    return m_curCameraInfo->angle;
}

bool ExynosCamera::setISO(int iso)
{
    int internalValue = -1;

    switch (iso) {
    case 50:
        internalValue = ISO_50;
        break;
    case 100:
        internalValue = ISO_100;
        break;
    case 200:
        internalValue = ISO_200;
        break;
    case 400:
        internalValue = ISO_400;
        break;
    case 800:
        internalValue = ISO_800;
        break;
    case 1600:
        internalValue = ISO_1600;
        break;
    case 0:
    default:
        internalValue = ISO_AUTO;
        break;
    }

    if (internalValue < ISO_AUTO || ISO_MAX <= internalValue) {
        ALOGE("ERR(%s):Invalid internalValue (%d)", __func__, internalValue);
        return false;
    }

    if (m_curCameraInfo->iso != iso) {
        m_curCameraInfo->iso = iso;
        if (m_flagCreate == true) {
            if (m_internalISP == true) {
                if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_IS_CAMERA_ISO, internalValue) < 0) {
                    ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                    return false;
                }
            } else {
                if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_CAMERA_ISO, internalValue) < 0) {
                    ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                    return false;
                }
            }
        }
    }

    return true;
}

int ExynosCamera::getISO(void)
{
    return m_curCameraInfo->iso;
}

bool ExynosCamera::setContrast(int value)
{
    int internalValue = -1;

    switch (value) {
    case CONTRAST_AUTO:
        if (m_internalISP == true)
            internalValue = ::IS_CONTRAST_AUTO;
        else
            ALOGW("WARN(%s):Invalid contrast value (%d)", __func__, value);
            return true;
        break;
    case CONTRAST_MINUS_2:
        if (m_internalISP == true)
            internalValue = ::IS_CONTRAST_MINUS_2;
        else
            internalValue = ::CONTRAST_MINUS_2;
        break;
    case CONTRAST_MINUS_1:
        if (m_internalISP == true)
            internalValue = ::IS_CONTRAST_MINUS_1;
        else
            internalValue = ::CONTRAST_MINUS_1;
        break;
    case CONTRAST_DEFAULT:
        if (m_internalISP == true)
            internalValue = ::IS_CONTRAST_DEFAULT;
        else
            internalValue = ::CONTRAST_DEFAULT;
        break;
    case CONTRAST_PLUS_1:
        if (m_internalISP == true)
            internalValue = ::IS_CONTRAST_PLUS_1;
        else
            internalValue = ::CONTRAST_PLUS_1;
        break;
    case CONTRAST_PLUS_2:
        if (m_internalISP == true)
            internalValue = ::IS_CONTRAST_PLUS_2;
        else
            internalValue = ::CONTRAST_PLUS_2;
        break;
    default:
        ALOGE("ERR(%s):Unsupported value(%d)", __func__, value);
        return false;
        break;
    }

    if (m_internalISP == true) {
        if (internalValue < ::IS_CONTRAST_AUTO || ::IS_CONTRAST_MAX <= internalValue) {
            ALOGE("ERR(%s):Invalid internalValue (%d)", __func__, internalValue);
            return false;
        }
    } else {
        if (internalValue < ::CONTRAST_MINUS_2 || ::CONTRAST_MAX <= internalValue) {
            ALOGE("ERR(%s):Invalid internalValue (%d)", __func__, internalValue);
            return false;
        }
    }

    if (m_curCameraInfo->contrast != value) {
        m_curCameraInfo->contrast = value;
        if (m_flagCreate == true) {
            if (m_internalISP == true) {
                if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_IS_CAMERA_CONTRAST, internalValue) < 0) {
                    ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                    return false;
                }
            } else {
                if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_CAMERA_CONTRAST, internalValue) < 0) {
                    ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                    return false;
                }
            }
        }
    }

    return true;
}

int ExynosCamera::getContrast(void)
{
    return m_curCameraInfo->contrast;
}

bool ExynosCamera::setSaturation(int saturation)
{
    int internalValue = saturation + SATURATION_DEFAULT;
    if (internalValue < SATURATION_MINUS_2 || SATURATION_MAX <= internalValue) {
        ALOGE("ERR(%s):Invalid internalValue (%d)", __func__, internalValue);
        return false;
    }

    if (m_curCameraInfo->saturation != saturation) {
        m_curCameraInfo->saturation = saturation;
        if (m_flagCreate == true) {
            if (m_internalISP == true) {
                if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_IS_CAMERA_SATURATION, internalValue) < 0) {
                    ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                    return false;
                }
            } else {
                if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_CAMERA_SATURATION, internalValue) < 0) {
                    ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                    return false;
                }
            }
        }
    }

    return true;
}

int ExynosCamera::getSaturation(void)
{
    return m_curCameraInfo->saturation;
}

bool ExynosCamera::setSharpness(int sharpness)
{
    int internalValue = sharpness + SHARPNESS_DEFAULT;
    if (internalValue < SHARPNESS_MINUS_2 || SHARPNESS_MAX <= internalValue) {
        ALOGE("ERR(%s):Invalid internalValue (%d)", __func__, internalValue);
        return false;
    }

    if (m_curCameraInfo->sharpness != sharpness) {
        m_curCameraInfo->sharpness = sharpness;
        if (m_flagCreate == true) {
            if (m_internalISP == true) {
                if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_IS_CAMERA_SHARPNESS, internalValue) < 0) {
                    ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                    return false;
                }
            } else {
                if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_CAMERA_SHARPNESS, internalValue) < 0) {
                    ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                    return false;
                }
            }
        }
    }

    return true;
}

int ExynosCamera::getSharpness(void)
{
    return m_curCameraInfo->sharpness;
}

bool ExynosCamera::setHue(int hue)
{
    int internalValue = hue;

    if (m_internalISP == true) {
        internalValue += IS_HUE_DEFAULT;
        if (internalValue < IS_HUE_MINUS_2 || IS_HUE_MAX <= internalValue) {
            ALOGE("ERR(%s):Invalid hue (%d)", __func__, hue);
            return false;
        }
    } else {
            ALOGV("WARN(%s):Not supported hue setting", __func__);
            return true;
    }

    if (m_curCameraInfo->hue != hue) {
        m_curCameraInfo->hue = hue;
        if (m_flagCreate == true) {
            if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_IS_CAMERA_HUE, internalValue) < 0) {
                ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                return false;
            }
        }
    }

    return true;
}

int ExynosCamera::getHue(void)
{
    return m_curCameraInfo->hue;
}

bool ExynosCamera::setWDR(bool toggle)
{
    int internalWdr;

    if (toggle == true) {
        if (m_internalISP == true)
            internalWdr = IS_DRC_BYPASS_ENABLE;
        else
            internalWdr = IS_DRC_BYPASS_DISABLE;
    } else {
        if (m_internalISP == true)
            internalWdr = WDR_ON;
        else
            internalWdr = WDR_OFF;
    }

    if (m_curCameraInfo->wdr != toggle) {
        m_curCameraInfo->wdr = toggle;
        if (m_flagCreate == true) {
            if (m_internalISP == true) {
                if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_IS_SET_DRC, internalWdr) < 0) {
                    ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                    return false;
                }
            } else {
                if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_CAMERA_WDR, internalWdr) < 0) {
                    ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                    return false;
                }
            }
        }
    }

    return true;
}

bool ExynosCamera::getWDR(void)
{
    return m_curCameraInfo->wdr;
}

bool ExynosCamera::setAntiShake(bool toggle)
{
    int internalValue = ANTI_SHAKE_OFF;

    if (toggle == true)
        internalValue = ANTI_SHAKE_STILL_ON;

    if (m_curCameraInfo->antiShake != toggle) {
        m_curCameraInfo->antiShake = toggle;
        if (m_flagCreate == true) {
            if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_CAMERA_ANTI_SHAKE, internalValue) < 0) {
                ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                return false;
            }
        }
    }

    return true;
}

bool ExynosCamera::getAntiShake(void)
{
    return m_curCameraInfo->antiShake;
}

bool ExynosCamera::setMeteringMode(int value)
{
    int internalValue = -1;

    switch (value) {
    case METERING_MODE_AVERAGE:
        if (m_internalISP == true)
            internalValue = IS_METERING_AVERAGE;
        else
            internalValue = METERING_MATRIX;
        break;
    case METERING_MODE_MATRIX:
        if (m_internalISP == true)
            internalValue = IS_METERING_MATRIX;
        else
            internalValue = METERING_MATRIX;
        break;
    case METERING_MODE_CENTER:
        if (m_internalISP == true)
            internalValue = IS_METERING_CENTER;
        else
            internalValue = METERING_CENTER;
        break;
    case METERING_MODE_SPOT:
        if (m_internalISP == true)
            internalValue = IS_METERING_SPOT;
        else
            internalValue = METERING_SPOT;
        break;
    default:
        ALOGE("ERR(%s):Unsupported value(%d)", __func__, value);
        return false;
        break;
    }

    if (m_internalISP == true) {
        if (internalValue < IS_METERING_AVERAGE || IS_METERING_MAX <= internalValue) {
            ALOGE("ERR(%s):Invalid internalValue (%d)", __func__, internalValue);
            return false;
        }
    } else {
        if (internalValue <= METERING_BASE || METERING_MAX <= internalValue) {
            ALOGE("ERR(%s):Invalid internalValue (%d)", __func__, internalValue);
            return false;
        }
    }

    if (m_curCameraInfo->metering != value) {
        m_curCameraInfo->metering = value;
        if (m_flagCreate == true) {
            if (m_internalISP == true) {
                if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_IS_CAMERA_METERING, internalValue) < 0) {
                    ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                    return false;
                }
            } else {
                if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_CAMERA_METERING, internalValue) < 0) {
                    ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                    return false;
                }
            }
        }
    }

    return true;
}

int ExynosCamera::getMeteringMode(void)
{
    return m_curCameraInfo->metering;
}

bool ExynosCamera::setObjectTracking(bool toggle)
{
    m_curCameraInfo->objectTracking = toggle;
    return true;
}

bool ExynosCamera::getObjectTracking(void)
{
    return m_curCameraInfo->objectTracking;
}

bool ExynosCamera::setObjectTrackingStart(bool toggle)
{
    if (m_curCameraInfo->objectTrackingStart != toggle) {
        m_curCameraInfo->objectTrackingStart = toggle;

        int startStop = (toggle == true) ? 1 : 0;
        if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_CAMERA_OBJ_TRACKING_START_STOP, startStop) < 0) {
            ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
            return false;
        }
    }

    return true;
}

int ExynosCamera::getObjectTrackingStatus(void)
{
    int ret = 0;

    if (exynos_v4l2_g_ctrl(m_previewDev->fd, V4L2_CID_CAMERA_OBJ_TRACKING_STATUS, &ret) < 0) {
        ALOGE("ERR(%s):exynos_v4l2_g_ctrl() fail", __func__);
        return -1;
    }
    return ret;
}

bool ExynosCamera::setObjectPosition(int x, int y)
{
    if (m_curCameraInfo->previewW == 640)
        x = x - 80;

    if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_IS_CAMERA_OBJECT_POSITION_X, x) < 0) {
        ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
        return false;
    }

    if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_IS_CAMERA_OBJECT_POSITION_Y, y) < 0) {
        ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
        return false;
    }

    return true;
}

bool ExynosCamera::setTouchAFStart(bool toggle)
{
    if (m_curCameraInfo->touchAfStart != toggle) {
        m_curCameraInfo->touchAfStart = toggle;
        int startStop = (toggle == true) ? 1 : 0;

        if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_CAMERA_TOUCH_AF_START_STOP, startStop) < 0) {
            ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
            return false;
        }
    }

    return true;
}

bool ExynosCamera::setSmartAuto(bool toggle)
{
    if (m_curCameraInfo->smartAuto != toggle) {
        m_curCameraInfo->smartAuto = toggle;

        int smartAuto = (toggle == true) ? SMART_AUTO_ON : SMART_AUTO_OFF;

        if (m_flagCreate == true) {
            if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_CAMERA_SMART_AUTO, smartAuto) < 0) {
                ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                return false;
            }
        }
    }

    return true;
}

bool ExynosCamera::getSmartAuto(void)
{
    return m_curCameraInfo->smartAuto;
}

int ExynosCamera::getSmartAutoStatus(void)
{
    int autoscene_status = -1;

    if (m_curCameraInfo->smartAuto == true) {
        if (exynos_v4l2_g_ctrl(m_previewDev->fd, V4L2_CID_CAMERA_SMART_AUTO_STATUS, &autoscene_status) < 0) {
            ALOGE("ERR(%s):exynos_v4l2_g_ctrl() fail", __func__);
            return -1;
        }

        if ((autoscene_status < SMART_AUTO_STATUS_AUTO) || (autoscene_status > SMART_AUTO_STATUS_MAX)) {
            ALOGE("ERR(%s):Invalid getSmartAutoStatus (%d)", __func__, autoscene_status);
            return -1;
        }
    }
    return autoscene_status;
}

bool ExynosCamera::setBeautyShot(bool toggle)
{
    if (m_curCameraInfo->beautyShot != toggle) {
        m_curCameraInfo->beautyShot = toggle;
        int beautyShot = (toggle == true) ? BEAUTY_SHOT_ON : BEAUTY_SHOT_OFF;

        if (m_flagCreate == true) {
            if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_CAMERA_BEAUTY_SHOT, beautyShot) < 0) {
                ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                return false;
            }
        }
    }

    return true;
}

bool ExynosCamera::getBeautyShot(void)
{
    return m_curCameraInfo->beautyShot;
}

bool ExynosCamera::setTopDownMirror(void)
{
    if (m_previewDev->fd <= 0) {
        ALOGE("ERR(%s):Camera was closed", __func__);
        return false;
    }

    if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_VFLIP, 1) < 0) {
        ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
        return false;
    }

    return true;
}

bool ExynosCamera::setLRMirror(void)
{
    if (m_previewDev->fd <= 0) {
        ALOGE("ERR(%s):Camera was closed", __func__);
        return false;
    }

    if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_HFLIP, 1) < 0) {
        ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
        return false;
    }

    return true;
}

bool ExynosCamera::setBrightness(int brightness)
{
    int internalValue = brightness;

    if (m_internalISP == true) {
        internalValue += IS_BRIGHTNESS_DEFAULT;
        if (internalValue < IS_BRIGHTNESS_MINUS_2 || IS_BRIGHTNESS_PLUS_2 < internalValue) {
            ALOGE("ERR(%s):Invalid internalValue(%d)", __func__, internalValue);
            return false;
        }
    } else {
        internalValue += EV_DEFAULT;
        if (internalValue < EV_MINUS_4 || EV_PLUS_4 < internalValue) {
            ALOGE("ERR(%s):Invalid internalValue(%d)", __func__, internalValue);
            return false;
        }
    }

    if (m_curCameraInfo->brightness != brightness) {
        m_curCameraInfo->brightness = brightness;
        if (m_flagCreate == true) {
            if (m_internalISP == true) {
                if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_IS_CAMERA_BRIGHTNESS, internalValue) < 0) {
                    ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                    return false;
                }
            } else {
                if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_CAMERA_BRIGHTNESS, internalValue) < 0) {
                    ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                    return false;
                }
            }
        }
    }

    return true;
}

int ExynosCamera::getBrightness(void)
{
    return m_curCameraInfo->brightness;
}

bool ExynosCamera::setGamma(bool toggle)
{
     if (m_curCameraInfo->gamma != toggle) {
         m_curCameraInfo->gamma = toggle;

         int gamma = (toggle == true) ? GAMMA_ON : GAMMA_OFF;

        if (m_flagCreate == true) {
             if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_CAMERA_SET_GAMMA, gamma) < 0) {
                 ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                 return false;
             }
         }
     }

     return true;
}

bool ExynosCamera::getGamma(void)
{
    return m_curCameraInfo->gamma;
}

bool ExynosCamera::setODC(bool toggle)
{
    if (m_previewDev->flagStart == true) {
        if (m_curCameraInfo->odc != toggle) {
            m_curCameraInfo->odc = toggle;

            int odc = (toggle == true) ? CAMERA_ODC_ON : CAMERA_ODC_OFF;

            if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_CAMERA_SET_ODC, odc) < 0) {
                ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                return false;
            }
        }
    }

     return true;
}

bool ExynosCamera::getODC(void)
{
    return m_curCameraInfo->odc;
}

bool ExynosCamera::setSlowAE(bool toggle)
{
     if (m_curCameraInfo->slowAE != toggle) {
         m_curCameraInfo->slowAE = toggle;

         int slow_ae = (toggle == true) ? SLOW_AE_ON : SLOW_AE_OFF;

        if (m_flagCreate == true) {
            if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_CAMERA_SET_SLOW_AE, slow_ae) < 0) {
                ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                return false;
            }
         }
     }

     return true;
}

bool ExynosCamera::getSlowAE(void)
{
    return m_curCameraInfo->slowAE;
}

bool ExynosCamera::setShotMode(int shotMode)
{
    if (shotMode < SHOT_MODE_SINGLE || SHOT_MODE_SELF < shotMode) {
        ALOGE("ERR(%s):Invalid shotMode (%d)", __func__, shotMode);
        return false;
    }

    if (m_curCameraInfo->shotMode != shotMode) {
        m_curCameraInfo->shotMode = shotMode;

        if (m_flagCreate == true) {
            if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_IS_CAMERA_SHOT_MODE_NORMAL, shotMode) < 0) {
                ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                return false;
            }
        }
    }

    return true;
}

int ExynosCamera::getShotMode(void)
{
    return m_curCameraInfo->shotMode;
}

bool ExynosCamera::set3DNR(bool toggle)
{
    if (m_previewDev->flagStart == true) {
        if (m_curCameraInfo->tdnr != toggle) {
            m_curCameraInfo->tdnr = toggle;

            int tdnr = (toggle == true) ? CAMERA_3DNR_ON : CAMERA_3DNR_OFF;

            if (exynos_v4l2_s_ctrl(m_previewDev->fd, V4L2_CID_CAMERA_SET_3DNR, tdnr) < 0) {
                ALOGE("ERR(%s):exynos_v4l2_s_ctrl() fail", __func__);
                return false;
            }
        }
    }

     return true;
}

bool ExynosCamera::get3DNR(void)
{
    return m_curCameraInfo->tdnr;
}

}; // namespace android
