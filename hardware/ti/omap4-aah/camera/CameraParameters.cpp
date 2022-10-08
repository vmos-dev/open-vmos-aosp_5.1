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
* @file CameraProperties.cpp
*
* This file maps the CameraHardwareInterface to the Camera interfaces on OMAP4 (mainly OMX).
*
*/

#include "CameraProperties.h"

namespace Ti {
namespace Camera {

const char CameraProperties::INVALID[]="prop-invalid-key";
const char CameraProperties::CAMERA_NAME[]="prop-camera-name";
const char CameraProperties::CAMERA_SENSOR_INDEX[]="prop-sensor-index";
const char CameraProperties::CAMERA_SENSOR_ID[] = "prop-sensor-id";
const char CameraProperties::ORIENTATION_INDEX[]="prop-orientation";
const char CameraProperties::FACING_INDEX[]="prop-facing";
const char CameraProperties::SUPPORTED_PREVIEW_SIZES[] = "prop-preview-size-values";
const char CameraProperties::SUPPORTED_PREVIEW_SUBSAMPLED_SIZES[] = "prop-preview-subsampled-size-values";
const char CameraProperties::SUPPORTED_PREVIEW_TOPBOTTOM_SIZES[] = "prop-preview-topbottom-size-values";
const char CameraProperties::SUPPORTED_PREVIEW_SIDEBYSIDE_SIZES[] = "prop-preview-sidebyside-size-values";
const char CameraProperties::SUPPORTED_PREVIEW_FORMATS[] = "prop-preview-format-values";
const char CameraProperties::SUPPORTED_PREVIEW_FRAME_RATES[] = "prop-preview-frame-rate-values";
const char CameraProperties::SUPPORTED_PREVIEW_FRAME_RATES_EXT[] = "prop-preview-frame-rate-ext-values";
const char CameraProperties::SUPPORTED_PICTURE_SIZES[] = "prop-picture-size-values";
const char CameraProperties::SUPPORTED_PICTURE_SUBSAMPLED_SIZES[] = "prop-picture-subsampled-size-values";
const char CameraProperties::SUPPORTED_PICTURE_TOPBOTTOM_SIZES[] = "prop-picture-topbottom-size-values";
const char CameraProperties::SUPPORTED_PICTURE_SIDEBYSIDE_SIZES[] = "prop-picture-sidebyside-size-values";
const char CameraProperties::SUPPORTED_PICTURE_FORMATS[] = "prop-picture-format-values";
const char CameraProperties::SUPPORTED_THUMBNAIL_SIZES[] = "prop-jpeg-thumbnail-size-values";
const char CameraProperties::SUPPORTED_WHITE_BALANCE[] = "prop-whitebalance-values";
const char CameraProperties::SUPPORTED_EFFECTS[] = "prop-effect-values";
const char CameraProperties::SUPPORTED_ANTIBANDING[] = "prop-antibanding-values";
const char CameraProperties::SUPPORTED_EXPOSURE_MODES[] = "prop-exposure-mode-values";
const char CameraProperties::SUPPORTED_MANUAL_EXPOSURE_MIN[] = "prop-manual-exposure-min";
const char CameraProperties::SUPPORTED_MANUAL_EXPOSURE_MAX[] = "prop-manual-exposure-max";
const char CameraProperties::SUPPORTED_MANUAL_EXPOSURE_STEP[] = "prop-manual-exposure-step";
const char CameraProperties::SUPPORTED_MANUAL_GAIN_ISO_MIN[] = "prop-manual-gain-iso-min";
const char CameraProperties::SUPPORTED_MANUAL_GAIN_ISO_MAX[] = "prop-manual-gain-iso-max";
const char CameraProperties::SUPPORTED_MANUAL_GAIN_ISO_STEP[] = "prop-manual-gain-iso-step";
const char CameraProperties::SUPPORTED_EV_MAX[] = "prop-ev-compensation-max";
const char CameraProperties::SUPPORTED_EV_MIN[] = "prop-ev-compensation-min";
const char CameraProperties::SUPPORTED_EV_STEP[] = "prop-ev-compensation-step";
const char CameraProperties::SUPPORTED_ISO_VALUES[] = "prop-iso-mode-values";
const char CameraProperties::SUPPORTED_SCENE_MODES[] = "prop-scene-mode-values";
const char CameraProperties::SUPPORTED_FLASH_MODES[] = "prop-flash-mode-values";
const char CameraProperties::SUPPORTED_FOCUS_MODES[] = "prop-focus-mode-values";
const char CameraProperties::REQUIRED_PREVIEW_BUFS[] = "prop-required-preview-bufs";
const char CameraProperties::REQUIRED_IMAGE_BUFS[] = "prop-required-image-bufs";
const char CameraProperties::SUPPORTED_ZOOM_RATIOS[] = "prop-zoom-ratios";
const char CameraProperties::SUPPORTED_ZOOM_STAGES[] = "prop-zoom-stages";
const char CameraProperties::SUPPORTED_IPP_MODES[] = "prop-ipp-values";
const char CameraProperties::SMOOTH_ZOOM_SUPPORTED[] = "prop-smooth-zoom-supported";
const char CameraProperties::ZOOM_SUPPORTED[] = "prop-zoom-supported";
const char CameraProperties::PREVIEW_SIZE[] = "prop-preview-size-default";
const char CameraProperties::PREVIEW_FORMAT[] = "prop-preview-format-default";
const char CameraProperties::PREVIEW_FRAME_RATE[] = "prop-preview-frame-rate-default";
const char CameraProperties::ZOOM[] = "prop-zoom-default";
const char CameraProperties::PICTURE_SIZE[] = "prop-picture-size-default";
const char CameraProperties::PICTURE_FORMAT[] = "prop-picture-format-default";
const char CameraProperties::JPEG_THUMBNAIL_SIZE[] = "prop-jpeg-thumbnail-size-default";
const char CameraProperties::WHITEBALANCE[] = "prop-whitebalance-default";
const char CameraProperties::EFFECT[] = "prop-effect-default";
const char CameraProperties::ANTIBANDING[] = "prop-antibanding-default";
const char CameraProperties::EXPOSURE_MODE[] = "prop-exposure-mode-default";
const char CameraProperties::EV_COMPENSATION[] = "prop-ev-compensation-default";
const char CameraProperties::ISO_MODE[] = "prop-iso-mode-default";
const char CameraProperties::FOCUS_MODE[] = "prop-focus-mode-default";
const char CameraProperties::SCENE_MODE[] = "prop-scene-mode-default";
const char CameraProperties::FLASH_MODE[] = "prop-flash-mode-default";
const char CameraProperties::JPEG_QUALITY[] = "prop-jpeg-quality-default";
const char CameraProperties::CONTRAST[] = "prop-contrast-default";
const char CameraProperties::BRIGHTNESS[] = "prop-brightness-default";
const char CameraProperties::SATURATION[] = "prop-saturation-default";
const char CameraProperties::SHARPNESS[] = "prop-sharpness-default";
const char CameraProperties::IPP[] = "prop-ipp-default";
const char CameraProperties::GBCE[] = "prop-gbce-default";
const char CameraProperties::SUPPORTED_GBCE[] = "prop-gbce-supported";
const char CameraProperties::GLBCE[] = "prop-glbce-default";
const char CameraProperties::SUPPORTED_GLBCE[] = "prop-glbce-supported";
const char CameraProperties::S3D_PRV_FRAME_LAYOUT[] = "prop-s3d-prv-frame-layout";
const char CameraProperties::S3D_PRV_FRAME_LAYOUT_VALUES[] = "prop-s3d-prv-frame-layout-values";
const char CameraProperties::S3D_CAP_FRAME_LAYOUT[] = "prop-s3d-cap-frame-layout";
const char CameraProperties::S3D_CAP_FRAME_LAYOUT_VALUES[] = "prop-s3d-cap-frame-layout-values";
const char CameraProperties::AUTOCONVERGENCE_MODE[] = "prop-auto-convergence-mode";
const char CameraProperties::AUTOCONVERGENCE_MODE_VALUES[] = "prop-auto-convergence-mode-values";
const char CameraProperties::MANUAL_CONVERGENCE[] = "prop-manual-convergence";
const char CameraProperties::SUPPORTED_MANUAL_CONVERGENCE_MIN[] = "prop-supported-manual-convergence-min";
const char CameraProperties::SUPPORTED_MANUAL_CONVERGENCE_MAX[] = "prop-supported-manual-convergence-max";
const char CameraProperties::SUPPORTED_MANUAL_CONVERGENCE_STEP[] = "prop-supported-manual-convergence-step";
const char CameraProperties::VSTAB[] = "prop-vstab-default";
const char CameraProperties::VSTAB_SUPPORTED[] = "prop-vstab-supported";
const char CameraProperties::VNF[] = "prop-vnf-default";
const char CameraProperties::VNF_SUPPORTED[] = "prop-vnf-supported";
const char CameraProperties::REVISION[] = "prop-revision";
const char CameraProperties::FOCAL_LENGTH[] = "prop-focal-length";
const char CameraProperties::HOR_ANGLE[] = "prop-horizontal-angle";
const char CameraProperties::VER_ANGLE[] = "prop-vertical-angle";
const char CameraProperties::FRAMERATE_RANGE[] = "prop-framerate-range-default";
const char CameraProperties::FRAMERATE_RANGE_SUPPORTED[]="prop-framerate-range-values";
const char CameraProperties::FRAMERATE_RANGE_EXT_SUPPORTED[]="prop-framerate-range-ext-values";
const char CameraProperties::SENSOR_ORIENTATION[]= "sensor-orientation";
const char CameraProperties::SENSOR_ORIENTATION_VALUES[]= "sensor-orientation-values";
const char CameraProperties::EXIF_MAKE[] = "prop-exif-make";
const char CameraProperties::EXIF_MODEL[] = "prop-exif-model";
const char CameraProperties::JPEG_THUMBNAIL_QUALITY[] = "prop-jpeg-thumbnail-quality-default";
const char CameraProperties::MAX_FOCUS_AREAS[] = "prop-max-focus-areas";
const char CameraProperties::MAX_FD_HW_FACES[] = "prop-max-fd-hw-faces";
const char CameraProperties::MAX_FD_SW_FACES[] = "prop-max-fd-sw-faces";
const char CameraProperties::AUTO_EXPOSURE_LOCK[] = "prop-auto-exposure-lock";
const char CameraProperties::AUTO_EXPOSURE_LOCK_SUPPORTED[] = "prop-auto-exposure-lock-supported";
const char CameraProperties::AUTO_WHITEBALANCE_LOCK[] = "prop-auto-whitebalance-lock";
const char CameraProperties::AUTO_WHITEBALANCE_LOCK_SUPPORTED[] = "prop-auto-whitebalance-lock-supported";
const char CameraProperties::MAX_NUM_METERING_AREAS[] = "prop-max-num-metering-areas";
const char CameraProperties::METERING_AREAS[] = "prop-metering-areas";
const char CameraProperties::VIDEO_SNAPSHOT_SUPPORTED[] = "prop-video-snapshot-supported";
const char CameraProperties::VIDEO_SIZE[] = "video-size";
const char CameraProperties::SUPPORTED_VIDEO_SIZES[] = "video-size-values";
const char CameraProperties::MECHANICAL_MISALIGNMENT_CORRECTION_SUPPORTED[] = "prop-mechanical-misalignment-correction-supported";
const char CameraProperties::MECHANICAL_MISALIGNMENT_CORRECTION[] = "prop-mechanical-misalignment-correction";
const char CameraProperties::CAP_MODE_VALUES[] = "prop-mode-values";
const char CameraProperties::RAW_WIDTH[] = "prop-raw-width-values";
const char CameraProperties::RAW_HEIGHT[] = "prop-raw-height-values";
const char CameraProperties::MAX_PICTURE_WIDTH[] = "prop-max-picture-width";
const char CameraProperties::MAX_PICTURE_HEIGHT[] = "prop-max-picture-height";

const char CameraProperties::DEFAULT_VALUE[] = "";

const char CameraProperties::PARAMS_DELIMITER []= ",";

// Returns the properties class for a specific Camera
// Each value is indexed by the CameraProperties::CameraPropertyIndex enum
int CameraProperties::getProperties(int cameraIndex, CameraProperties::Properties** properties)
{
    LOG_FUNCTION_NAME;

    if(cameraIndex >= mCamerasSupported)
    {
        LOG_FUNCTION_NAME_EXIT;
        return -EINVAL;
    }

    *properties = mCameraProps+cameraIndex;

    LOG_FUNCTION_NAME_EXIT;
    return 0;
}

void CameraProperties::Properties::set(const char * const prop, const char * const value) {
    CAMHAL_ASSERT(prop);

    if ( !value ) {
        mProperties[mCurrentMode].removeItem(android::String8(prop));
    } else {
        mProperties[mCurrentMode].replaceValueFor(android::String8(prop), android::String8(value));
    }
}

void CameraProperties::Properties::set(const char * const prop, const int value) {
    char s_val[30];
    sprintf(s_val, "%d", value);
    set(prop, s_val);
}

const char* CameraProperties::Properties::get(const char * prop) const {
    return mProperties[mCurrentMode].valueFor(android::String8(prop)).string();
}

int CameraProperties::Properties::getInt(const char * prop) const {
    android::String8 value = mProperties[mCurrentMode].valueFor(android::String8(prop));
    if (value.isEmpty()) {
        return -1;
    }
    return strtol(value, 0, 0);
}

void CameraProperties::Properties::setSensorIndex(int idx) {
    OperatingMode originalMode = getMode();
    for ( int i = 0 ; i < MODE_MAX ; i++ ) {
        setMode(static_cast<OperatingMode>(i));
        set(CAMERA_SENSOR_INDEX, idx);
    }
    setMode(originalMode);
}

void CameraProperties::Properties::setMode(OperatingMode mode) {
    CAMHAL_ASSERT(mode >= 0 && mode < MODE_MAX);
    mCurrentMode = mode;
}

OperatingMode CameraProperties::Properties::getMode() const {
    return mCurrentMode;
}

void CameraProperties::Properties::dump() {
    CAMHAL_LOGD("================================");
    CAMHAL_LOGD("Dumping properties for camera: %d", getInt("prop-sensor-index"));

    for (size_t i = 0; i < mProperties[mCurrentMode].size(); i++) {
        CAMHAL_LOGD("%s = %s",
                mProperties[mCurrentMode].keyAt(i).string(),
                mProperties[mCurrentMode].valueAt(i).string());
    }

    CAMHAL_LOGD("--------------------------------");
}

const char* CameraProperties::Properties::keyAt(const unsigned int index) const {
    if (index < mProperties[mCurrentMode].size()) {
        return mProperties[mCurrentMode].keyAt(index).string();
    }
    return NULL;
}

const char* CameraProperties::Properties::valueAt(const unsigned int index) const {
    if (index < mProperties[mCurrentMode].size()) {
        return mProperties[mCurrentMode].valueAt(index).string();
    }
    return NULL;
}

} // namespace Camera
} // namespace Ti
