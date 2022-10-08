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




#ifndef CAMERA_PROPERTIES_H
#define CAMERA_PROPERTIES_H

#include <utils/KeyedVector.h>
#include <utils/String8.h>
#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "cutils/properties.h"

#include "Common.h"

namespace Ti {
namespace Camera {

#define MAX_CAMERAS_SUPPORTED 3
#define MAX_SIMUL_CAMERAS_SUPPORTED 1
#define MAX_PROP_NAME_LENGTH 50
#define MAX_PROP_VALUE_LENGTH 2048

#define REMAINING_BYTES(buff) ((((int)sizeof(buff) - 1 - (int)strlen(buff)) < 0) ? 0 : (sizeof(buff) - 1 - strlen(buff)))

enum OperatingMode {
    MODE_HIGH_SPEED = 0,
    MODE_HIGH_QUALITY,
    MODE_ZEROSHUTTERLAG,
    MODE_VIDEO,
    MODE_STEREO,
    MODE_CPCAM,
    MODE_VIDEO_HIGH_QUALITY,
    MODE_MAX
};

// Class that handles the Camera Properties
class CameraProperties
{
public:
    static const char INVALID[];
    static const char CAMERA_NAME[];
    static const char CAMERA_SENSOR_INDEX[];
    static const char CAMERA_SENSOR_ID[];
    static const char ORIENTATION_INDEX[];
    static const char FACING_INDEX[];
    static const char SUPPORTED_PREVIEW_SIZES[];
    static const char SUPPORTED_PREVIEW_SUBSAMPLED_SIZES[];
    static const char SUPPORTED_PREVIEW_TOPBOTTOM_SIZES[];
    static const char SUPPORTED_PREVIEW_SIDEBYSIDE_SIZES[];
    static const char SUPPORTED_PREVIEW_FORMATS[];
    static const char SUPPORTED_PREVIEW_FRAME_RATES[];
    static const char SUPPORTED_PREVIEW_FRAME_RATES_EXT[];
    static const char SUPPORTED_PICTURE_SIZES[];
    static const char SUPPORTED_PICTURE_SUBSAMPLED_SIZES[];
    static const char SUPPORTED_PICTURE_TOPBOTTOM_SIZES[];
    static const char SUPPORTED_PICTURE_SIDEBYSIDE_SIZES[];
    static const char SUPPORTED_PICTURE_FORMATS[];
    static const char SUPPORTED_THUMBNAIL_SIZES[];
    static const char SUPPORTED_WHITE_BALANCE[];
    static const char SUPPORTED_EFFECTS[];
    static const char SUPPORTED_ANTIBANDING[];
    static const char SUPPORTED_EXPOSURE_MODES[];
    static const char SUPPORTED_MANUAL_EXPOSURE_MIN[];
    static const char SUPPORTED_MANUAL_EXPOSURE_MAX[];
    static const char SUPPORTED_MANUAL_EXPOSURE_STEP[];
    static const char SUPPORTED_MANUAL_GAIN_ISO_MIN[];
    static const char SUPPORTED_MANUAL_GAIN_ISO_MAX[];
    static const char SUPPORTED_MANUAL_GAIN_ISO_STEP[];
    static const char SUPPORTED_EV_MIN[];
    static const char SUPPORTED_EV_MAX[];
    static const char SUPPORTED_EV_STEP[];
    static const char SUPPORTED_ISO_VALUES[];
    static const char SUPPORTED_SCENE_MODES[];
    static const char SUPPORTED_FLASH_MODES[];
    static const char SUPPORTED_FOCUS_MODES[];
    static const char REQUIRED_PREVIEW_BUFS[];
    static const char REQUIRED_IMAGE_BUFS[];
    static const char SUPPORTED_ZOOM_RATIOS[];
    static const char SUPPORTED_ZOOM_STAGES[];
    static const char SUPPORTED_IPP_MODES[];
    static const char SMOOTH_ZOOM_SUPPORTED[];
    static const char ZOOM_SUPPORTED[];
    static const char PREVIEW_SIZE[];
    static const char PREVIEW_FORMAT[];
    static const char PREVIEW_FRAME_RATE[];
    static const char ZOOM[];
    static const char PICTURE_SIZE[];
    static const char PICTURE_FORMAT[];
    static const char JPEG_THUMBNAIL_SIZE[];
    static const char WHITEBALANCE[];
    static const char EFFECT[];
    static const char ANTIBANDING[];
    static const char EXPOSURE_MODE[];
    static const char EV_COMPENSATION[];
    static const char ISO_MODE[];
    static const char FOCUS_MODE[];
    static const char SCENE_MODE[];
    static const char FLASH_MODE[];
    static const char JPEG_QUALITY[];
    static const char BRIGHTNESS[];
    static const char SATURATION[];
    static const char SHARPNESS[];
    static const char CONTRAST[];
    static const char IPP[];
    static const char GBCE[];
    static const char SUPPORTED_GBCE[];
    static const char GLBCE[];
    static const char SUPPORTED_GLBCE[];
    static const char AUTOCONVERGENCE_MODE[];
    static const char AUTOCONVERGENCE_MODE_VALUES[];
    static const char MANUAL_CONVERGENCE[];
    static const char SUPPORTED_MANUAL_CONVERGENCE_MIN[];
    static const char SUPPORTED_MANUAL_CONVERGENCE_MAX[];
    static const char SUPPORTED_MANUAL_CONVERGENCE_STEP[];
    static const char SENSOR_ORIENTATION[];
    static const char SENSOR_ORIENTATION_VALUES[];
    static const char REVISION[];
    static const char FOCAL_LENGTH[];
    static const char HOR_ANGLE[];
    static const char VER_ANGLE[];
    static const char EXIF_MAKE[];
    static const char EXIF_MODEL[];
    static const char JPEG_THUMBNAIL_QUALITY[];
    static const char MAX_FOCUS_AREAS[];
    static const char MAX_FD_HW_FACES[];
    static const char MAX_FD_SW_FACES[];

    static const char MAX_PICTURE_WIDTH[];
    static const char MAX_PICTURE_HEIGHT[];

    static const char PARAMS_DELIMITER [];

    static const char S3D_PRV_FRAME_LAYOUT[];
    static const char S3D_PRV_FRAME_LAYOUT_VALUES[];
    static const char S3D_CAP_FRAME_LAYOUT[];
    static const char S3D_CAP_FRAME_LAYOUT_VALUES[];
    static const char VSTAB[];
    static const char VSTAB_SUPPORTED[];
    static const char VNF[];
    static const char VNF_SUPPORTED[];
    static const char FRAMERATE_RANGE[];
    static const char FRAMERATE_RANGE_SUPPORTED[];
    static const char FRAMERATE_RANGE_EXT_SUPPORTED[];

    static const char DEFAULT_VALUE[];

    static const char AUTO_EXPOSURE_LOCK[];
    static const char AUTO_EXPOSURE_LOCK_SUPPORTED[];
    static const char AUTO_WHITEBALANCE_LOCK[];
    static const char AUTO_WHITEBALANCE_LOCK_SUPPORTED[];
    static const char MAX_NUM_METERING_AREAS[];
    static const char METERING_AREAS[];
    static const char MAX_NUM_FOCUS_AREAS[];

    static const char VIDEO_SNAPSHOT_SUPPORTED[];

    static const char VIDEO_SIZE[];
    static const char SUPPORTED_VIDEO_SIZES[];

    static const char MECHANICAL_MISALIGNMENT_CORRECTION_SUPPORTED[];
    static const char MECHANICAL_MISALIGNMENT_CORRECTION[];

    static const char RAW_WIDTH[];
    static const char RAW_HEIGHT[];

    static const char CAP_MODE_VALUES[];

    CameraProperties();
    ~CameraProperties();

    // container class passed around for accessing properties
    class Properties
    {
        public:

            Properties()
            {
            }

            ~Properties()
            {
            }

            void set(const char *prop, const char *value);
            void set(const char *prop, int value);
            const char* get(const char * prop) const;
            int getInt(const char * prop) const;
            void setSensorIndex(int idx);
            void setMode(OperatingMode mode);
            OperatingMode getMode() const;
            void dump();

        protected:
            const char* keyAt(const unsigned int) const;
            const char* valueAt(const unsigned int) const;

        private:
            OperatingMode mCurrentMode;
            android::DefaultKeyedVector<android::String8, android::String8> mProperties[MODE_MAX];

    };

    ///Initializes the CameraProperties class
    status_t initialize();
    status_t loadProperties();
    int camerasSupported();
    int getProperties(int cameraIndex, Properties** properties);

private:

    int mCamerasSupported;
    int mInitialized;
    mutable android::Mutex mLock;

    Properties mCameraProps[MAX_CAMERAS_SUPPORTED];

};

} // namespace Camera
} // namespace Ti

#endif //CAMERA_PROPERTIES_H
