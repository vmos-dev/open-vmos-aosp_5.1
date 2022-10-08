/*
**
** Copyright 2008, The Android Open Source Project
** Copyright 2012, Samsung Electronics Co. LTD
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

/*!
 * \file      ExynosCamera2.h
 * \brief     header file for static information of camera2
 * \author    Sungjoong Kang(sj3.kang@samsung.com)
 * \date      2012/08/06
 *
 * <b>Revision History: </b>
 * - 2012/08/06 : Sungjoong Kang(sj3.kang@samsung.com) \n
 *   Initial Release
 *
 */

#ifndef EXYNOS_CAMERA_2_H
#define EXYNOS_CAMERA_2_H

#include <hardware/camera2.h>
#include <camera/Camera.h>
#include <camera/CameraParameters.h>
#include "exynos_format.h"
#include "fimc-is-metadata.h"

namespace android {

extern int32_t SUPPORT_THUMBNAIL_REAR_SIZE[3][2];
extern int32_t SUPPORT_THUMBNAIL_FRONT_SIZE[4][2];

struct ExynosCamera2Info
{
public:
    int32_t     sensorW;
    int32_t     sensorH;
    int32_t     sensorRawW;
    int32_t     sensorRawH;
    int         numScalerResolution;
    const int32_t     *scalerResolutions;
    int         numJpegResolution;
    const int32_t     *jpegResolutions;
    float       minFocusDistance;
    float       focalLength;
    float       aperture;
    float       fnumber;
    const uint8_t       *availableAfModes;
    const uint8_t       *sceneModeOverrides;
    const uint8_t       *availableAeModes;
    int         numAvailableAfModes;
    int         numSceneModeOverrides;
    int         numAvailableAeModes;

    /* FD information */
    int32_t    maxFaceCount;
};

struct ExynosCamera2InfoS5K4E5 : public ExynosCamera2Info
{
public:
    ExynosCamera2InfoS5K4E5();
	~ExynosCamera2InfoS5K4E5();
};

struct ExynosCamera2InfoS5K6A3 : public ExynosCamera2Info
{
public:
    ExynosCamera2InfoS5K6A3();
	~ExynosCamera2InfoS5K6A3();
};

class ExynosCamera2 {
public:
    ExynosCamera2(int cameraId);
    ~ExynosCamera2();

    ExynosCamera2Info  *m_curCameraInfo;

    int32_t getSensorW();
    int32_t getSensorH();
    int32_t getSensorRawW();
    int32_t getSensorRawH();

    bool isSupportedResolution(int width, int height);
    bool isSupportedJpegResolution(int width, int height);

    status_t constructStaticInfo(camera_metadata_t **info,
        int cameraId, bool sizeRequest);

    status_t constructDefaultRequest(int request_template,
        camera_metadata_t **request, bool sizeRequest);
    int     m_cameraId;
};
}

#endif
