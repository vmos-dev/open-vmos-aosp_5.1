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
 * \file      ExynosCamera2.cpp
 * \brief     source file for static information of camera2
 * \author    Sungjoong Kang(sj3.kang@samsung.com)
 * \date      2012/08/06
 *
 * <b>Revision History: </b>
 * - 2012/08/06 : Sungjoong Kang(sj3.kang@samsung.com) \n
 *   Initial Release
 *
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "ExynosCamera2"
#include <utils/Log.h>

#include "ExynosCamera2.h"

#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))

namespace android {

int32_t SUPPORT_THUMBNAIL_REAR_SIZE[3][2] =
{
    {160, 120},
    {160, 90},
    {144, 96}
};

int32_t SUPPORT_THUMBNAIL_FRONT_SIZE[4][2] =
{
    {160, 120},
    {160, 160},
    {160, 90},
    {144, 96}
};

class Sensor {
public:
    /**
     * Static sensor characteristics
     */
    static const unsigned int kResolution[2][2];

    static const nsecs_t kExposureTimeRange[2];
    static const nsecs_t kFrameDurationRange[2];
    static const nsecs_t kMinVerticalBlank;

    static const uint8_t kColorFilterArrangement;

    // Output image data characteristics
    static const uint32_t kMaxRawValue;
    static const uint32_t kBlackLevel;
    // Sensor sensitivity, approximate

    static const float kSaturationVoltage;
    static const uint32_t kSaturationElectrons;
    static const float kVoltsPerLuxSecond;
    static const float kElectronsPerLuxSecond;

    static const float kBaseGainFactor;

    static const float kReadNoiseStddevBeforeGain; // In electrons
    static const float kReadNoiseStddevAfterGain;  // In raw digital units
    static const float kReadNoiseVarBeforeGain;
    static const float kReadNoiseVarAfterGain;

    // While each row has to read out, reset, and then expose, the (reset +
    // expose) sequence can be overlapped by other row readouts, so the final
    // minimum frame duration is purely a function of row readout time, at least
    // if there's a reasonable number of rows.
    static const nsecs_t kRowReadoutTime;

    static const int32_t kSensitivityRange[2];
    static const uint32_t kDefaultSensitivity;

};



const int32_t Sensor::kSensitivityRange[2] = {100, 1600};
const nsecs_t Sensor::kExposureTimeRange[2] =
    {1000L, 30000000000L} ; // 1 us - 30 sec
const nsecs_t Sensor::kFrameDurationRange[2] =
    {33331760L, 30000000000L}; // ~1/30 s - 30 sec

const uint8_t Sensor::kColorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB;

const uint32_t kAvailableFormats[5] = {
        HAL_PIXEL_FORMAT_RAW_SENSOR,
        HAL_PIXEL_FORMAT_BLOB,
        HAL_PIXEL_FORMAT_RGBA_8888,
        HAL_PIXEL_FORMAT_YV12,
        HAL_PIXEL_FORMAT_YCrCb_420_SP
};

// Output image data characteristics
const uint32_t Sensor::kMaxRawValue = 4000;
const uint32_t Sensor::kBlackLevel  = 1000;

const uint64_t kAvailableRawMinDurations[1] = {
    Sensor::kFrameDurationRange[0]
};

const uint64_t kAvailableProcessedMinDurations[1] = {
    Sensor::kFrameDurationRange[0]
};
const uint64_t kAvailableJpegMinDurations[1] = {
    Sensor::kFrameDurationRange[0]
};

const int32_t scalerResolutionS5K4E5[] =
{
    1920, 1080, // 16:9
    1440, 1080, // 4:3
    1440,  960, // 3:2
    1280, 1024, // 5:4
    1280,  720, // 16:9
     960,  720, // 4:3
     800,  480, // 5:3
     768,  576, // 4:3
     720,  576, // 5:4
     720,  480, // 3:2
     640,  480, // 4:3
     352,  288, // 11:9
     320,  240, // 4:3
     240,  160, // 3:2
     176,  144, // 6:5
     128,   96, // 4:3
};

const int32_t jpegResolutionS5K4E5[] =
{
    2560, 1920,
    2560, 1440,
    2160, 1440,
    2048, 1536,
    1600, 1200,
    1280, 1024,
    1280,  960,
    1152,  864,
     640,  480,
     320,  240,
};

const uint8_t availableAfModesS5K4E5[] =
{
    ANDROID_CONTROL_AF_MODE_OFF,
    ANDROID_CONTROL_AF_MODE_AUTO,
    ANDROID_CONTROL_AF_MODE_MACRO,
    ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE,
    ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO
};

const uint8_t sceneModeOverridesS5K4E5[] =
{
    // ANDROID_CONTROL_SCENE_MODE_ACTION
    ANDROID_CONTROL_AE_MODE_ON,
    ANDROID_CONTROL_AWB_MODE_AUTO,
    ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE,
    // ANDROID_CONTROL_SCENE_MODE_NIGHT
    ANDROID_CONTROL_AE_MODE_ON,
    ANDROID_CONTROL_AWB_MODE_AUTO,
    ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE,
    // ANDROID_CONTROL_SCENE_MODE_SUNSET
    ANDROID_CONTROL_AE_MODE_ON,
    ANDROID_CONTROL_AWB_MODE_DAYLIGHT,
    ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE,
    // ANDROID_CONTROL_SCENE_MODE_PARTY
    ANDROID_CONTROL_AE_MODE_ON_AUTO_FLASH,
    ANDROID_CONTROL_AWB_MODE_AUTO,
    ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE
};

const uint8_t availableAeModesS5K4E5[] =
{
    ANDROID_CONTROL_AE_MODE_OFF,
    ANDROID_CONTROL_AE_MODE_ON,
    ANDROID_CONTROL_AE_MODE_ON_AUTO_FLASH
};

ExynosCamera2InfoS5K4E5::ExynosCamera2InfoS5K4E5()
{
    sensorW             = 2560;
    sensorH             = 1920;
    sensorRawW          = (2560 + 16);
    sensorRawH          = (1920 + 10);
    numScalerResolution = ARRAY_SIZE(scalerResolutionS5K4E5)/2;
    scalerResolutions   = scalerResolutionS5K4E5;
    numJpegResolution   = ARRAY_SIZE(jpegResolutionS5K4E5)/2;
    jpegResolutions     = jpegResolutionS5K4E5;
    minFocusDistance    = 0.1f;
    focalLength         = 3.43f;
    aperture            = 2.7f;
    fnumber             = 2.7f;
    availableAfModes    = availableAfModesS5K4E5;
    numAvailableAfModes = ARRAY_SIZE(availableAfModesS5K4E5);
    sceneModeOverrides  = sceneModeOverridesS5K4E5;
    numSceneModeOverrides = ARRAY_SIZE(sceneModeOverridesS5K4E5);
    availableAeModes    = availableAeModesS5K4E5;
    numAvailableAeModes = ARRAY_SIZE(availableAeModesS5K4E5);
}

ExynosCamera2InfoS5K4E5::~ExynosCamera2InfoS5K4E5()
{
    ALOGV("%s", __FUNCTION__);
}
const int32_t scalerResolutionS5K6A3[] =
{
    1344,  896, // 3:2
    1280, 1024, // 5:4
    1024, 1024, // 1:1
    1280,  960, // 4:3
    1280,  720, // 16:9
     960,  720, // 4:3
     800,  480, // 5:3
     768,  576, // 4:3
     720,  576, // 5:4
     720,  480, // 3:2
     640,  480, // 4:3
     352,  288, // 11:9
     320,  240, // 4:3
     240,  160, // 3:2
     176,  144, // 6:5
     128,   96, // 4:3
};

const int32_t jpegResolutionS5K6A3[] =
{
    1392, 1392,
    1392, 1040,
    1392,  928,
    1392,  784,
    1280, 1024,
    1280,  960,
    1280,  720,
    1152,  864,
     640,  480,
     320,  240,
};

const uint8_t availableAfModesS5K6A3[] =
{
    ANDROID_CONTROL_AF_MODE_OFF
};

const uint8_t sceneModeOverridesS5K6A3[] =
{
    // ANDROID_CONTROL_SCENE_MODE_ACTION
    ANDROID_CONTROL_AE_MODE_ON,
    ANDROID_CONTROL_AWB_MODE_AUTO,
    ANDROID_CONTROL_AF_MODE_OFF,
    // ANDROID_CONTROL_SCENE_MODE_NIGHT
    ANDROID_CONTROL_AE_MODE_ON,
    ANDROID_CONTROL_AWB_MODE_AUTO,
    ANDROID_CONTROL_AF_MODE_OFF,
    // ANDROID_CONTROL_SCENE_MODE_SUNSET
    ANDROID_CONTROL_AE_MODE_ON,
    ANDROID_CONTROL_AWB_MODE_DAYLIGHT,
    ANDROID_CONTROL_AF_MODE_OFF,
    // ANDROID_CONTROL_SCENE_MODE_PARTY
    ANDROID_CONTROL_AE_MODE_ON,
    ANDROID_CONTROL_AWB_MODE_AUTO,
    ANDROID_CONTROL_AF_MODE_OFF
};

const uint8_t availableAeModesS5K6A3[] =
{
    ANDROID_CONTROL_AE_MODE_OFF,
    ANDROID_CONTROL_AE_MODE_ON
};

ExynosCamera2InfoS5K6A3::ExynosCamera2InfoS5K6A3()
{
    sensorW     = 1392;
    sensorH     = 1392;
    sensorRawW  = (1392 + 16);
    sensorRawH  = (1392 + 10);
    numScalerResolution = ARRAY_SIZE(scalerResolutionS5K6A3)/2;
    scalerResolutions   = scalerResolutionS5K6A3;
    numJpegResolution   = ARRAY_SIZE(jpegResolutionS5K6A3)/2;
    jpegResolutions     = jpegResolutionS5K6A3;
    minFocusDistance    = 0.0f;
    focalLength         = 2.73f;
    aperture            = 2.8f;
    fnumber             = 2.8f;
    availableAfModes    = availableAfModesS5K6A3;
    numAvailableAfModes = ARRAY_SIZE(availableAfModesS5K6A3);
    sceneModeOverrides  = sceneModeOverridesS5K6A3;
    numSceneModeOverrides = ARRAY_SIZE(sceneModeOverridesS5K6A3);
    availableAeModes    = availableAeModesS5K6A3;
    numAvailableAeModes = ARRAY_SIZE(availableAeModesS5K6A3);
}

ExynosCamera2InfoS5K6A3::~ExynosCamera2InfoS5K6A3()
{
    ALOGV("%s", __FUNCTION__);
}
ExynosCamera2::ExynosCamera2(int cameraId):
    m_cameraId(cameraId)
{
    if (cameraId == 0)
        m_curCameraInfo      = new ExynosCamera2InfoS5K4E5;
    else
        m_curCameraInfo      = new ExynosCamera2InfoS5K6A3;
}

ExynosCamera2::~ExynosCamera2()
{
    ALOGV("%s", __FUNCTION__);
	delete m_curCameraInfo;
    m_curCameraInfo = NULL;
}

int32_t ExynosCamera2::getSensorW()
{
    return m_curCameraInfo->sensorW;
}

int32_t ExynosCamera2::getSensorH()
{
    return m_curCameraInfo->sensorH;
}

int32_t ExynosCamera2::getSensorRawW()
{
    return m_curCameraInfo->sensorRawW;
}

int32_t ExynosCamera2::getSensorRawH()
{
    return m_curCameraInfo->sensorRawH;
}

bool ExynosCamera2::isSupportedResolution(int width, int height)
{
    int i;
    for (i = 0 ; i < m_curCameraInfo->numScalerResolution ; i++) {
        if (m_curCameraInfo->scalerResolutions[2*i] == width
                && m_curCameraInfo->scalerResolutions[2*i+1] == height) {
            return true;
        }
    }
    return false;
}

bool ExynosCamera2::isSupportedJpegResolution(int width, int height)
{
    int i;
    for (i = 0 ; i < m_curCameraInfo->numJpegResolution ; i++) {
        if (m_curCameraInfo->jpegResolutions[2*i] == width
                && m_curCameraInfo->jpegResolutions[2*i+1] == height) {
            return true;
        }
    }
    return false;
}

status_t addOrSize(camera_metadata_t *request,
        bool sizeRequest,
        size_t *entryCount,
        size_t *dataCount,
        uint32_t tag,
        const void *entryData,
        size_t entryDataCount) {
    status_t res;
    if (!sizeRequest) {
        return add_camera_metadata_entry(request, tag, entryData,
                entryDataCount);
    } else {
        int type = get_camera_metadata_tag_type(tag);
        if (type < 0 ) return BAD_VALUE;
        (*entryCount)++;
        (*dataCount) += calculate_camera_metadata_entry_data_size(type,
                entryDataCount);
        return OK;
    }
}

status_t ExynosCamera2::constructStaticInfo(camera_metadata_t **info,
        int cameraId, bool sizeRequest) {

    size_t entryCount = 0;
    size_t dataCount = 0;
    status_t ret;

#define ADD_OR_SIZE( tag, data, count ) \
    if ( ( ret = addOrSize(*info, sizeRequest, &entryCount, &dataCount, \
            tag, data, count) ) != OK ) return ret

    // android.info

    int32_t hardwareLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED;
    ADD_OR_SIZE(ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL,
            &hardwareLevel, 1);

    // android.lens

    ADD_OR_SIZE(ANDROID_LENS_INFO_MINIMUM_FOCUS_DISTANCE,
            &(m_curCameraInfo->minFocusDistance), 1);
    ADD_OR_SIZE(ANDROID_LENS_INFO_HYPERFOCAL_DISTANCE,
            &(m_curCameraInfo->minFocusDistance), 1);

    ADD_OR_SIZE(ANDROID_LENS_INFO_AVAILABLE_FOCAL_LENGTHS,
            &m_curCameraInfo->focalLength, 1);
    ADD_OR_SIZE(ANDROID_LENS_INFO_AVAILABLE_APERTURES,
            &m_curCameraInfo->aperture, 1);

    static const float filterDensity = 0;
    ADD_OR_SIZE(ANDROID_LENS_INFO_AVAILABLE_FILTER_DENSITIES,
            &filterDensity, 1);
    static const uint8_t availableOpticalStabilization =
            ANDROID_LENS_OPTICAL_STABILIZATION_MODE_OFF;
    ADD_OR_SIZE(ANDROID_LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION,
            &availableOpticalStabilization, 1);

    static const int32_t lensShadingMapSize[] = {1, 1};
    ADD_OR_SIZE(ANDROID_LENS_INFO_SHADING_MAP_SIZE, lensShadingMapSize,
            sizeof(lensShadingMapSize)/sizeof(int32_t));

    int32_t lensFacing = cameraId ?
            ANDROID_LENS_FACING_FRONT : ANDROID_LENS_FACING_BACK;
    ADD_OR_SIZE(ANDROID_LENS_FACING, &lensFacing, 1);

    // android.request
    static const int32_t maxNumOutputStreams[] = {1, 3, 1};
    ADD_OR_SIZE(ANDROID_REQUEST_MAX_NUM_OUTPUT_STREAMS, maxNumOutputStreams,
            sizeof(maxNumOutputStreams)/sizeof(int32_t));

    // android.sensor
    ADD_OR_SIZE(ANDROID_SENSOR_INFO_EXPOSURE_TIME_RANGE,
            Sensor::kExposureTimeRange, 2);

    ADD_OR_SIZE(ANDROID_SENSOR_INFO_MAX_FRAME_DURATION,
            &Sensor::kFrameDurationRange[1], 1);

    ADD_OR_SIZE(ANDROID_SENSOR_INFO_SENSITIVITY_RANGE,
            Sensor::kSensitivityRange,
            sizeof(Sensor::kSensitivityRange)
            /sizeof(int32_t));

    ADD_OR_SIZE(ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT,
            &Sensor::kColorFilterArrangement, 1);

    // Empirically derived to get correct FOV measurements
    static const float sensorPhysicalSize[2] = {3.50f, 2.625f}; // mm
    ADD_OR_SIZE(ANDROID_SENSOR_INFO_PHYSICAL_SIZE,
            sensorPhysicalSize, 2);

    int32_t pixelArraySize[2] = {
        m_curCameraInfo->sensorW, m_curCameraInfo->sensorH
    };
    ADD_OR_SIZE(ANDROID_SENSOR_INFO_PIXEL_ARRAY_SIZE, pixelArraySize, 2);

    int32_t activeArraySize[4] = { 0, 0, pixelArraySize[0], pixelArraySize[1]};
    ADD_OR_SIZE(ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE, activeArraySize,4);

    ADD_OR_SIZE(ANDROID_SENSOR_INFO_WHITE_LEVEL,
            &Sensor::kMaxRawValue, 1);

    static const int32_t blackLevelPattern[4] = {
            Sensor::kBlackLevel, Sensor::kBlackLevel,
            Sensor::kBlackLevel, Sensor::kBlackLevel
    };
    ADD_OR_SIZE(ANDROID_SENSOR_BLACK_LEVEL_PATTERN,
            blackLevelPattern, sizeof(blackLevelPattern)/sizeof(int32_t));

    static const int32_t orientation[1] = {0};
    ADD_OR_SIZE(ANDROID_SENSOR_ORIENTATION,
            orientation, 1);

    //TODO: sensor color calibration fields

    // android.flash
    uint8_t flashAvailable;
    if (cameraId == 0)
        flashAvailable = 1;
    else
        flashAvailable = 0;
    ADD_OR_SIZE(ANDROID_FLASH_INFO_AVAILABLE, &flashAvailable, 1);

    static const int64_t flashChargeDuration = 0;
    ADD_OR_SIZE(ANDROID_FLASH_INFO_CHARGE_DURATION, &flashChargeDuration, 1);

    // android.tonemap

    static const int32_t tonemapCurvePoints = 128;
    ADD_OR_SIZE(ANDROID_TONEMAP_MAX_CURVE_POINTS, &tonemapCurvePoints, 1);

    // android.scaler

    ADD_OR_SIZE(ANDROID_SCALER_AVAILABLE_FORMATS,
            kAvailableFormats,
            sizeof(kAvailableFormats)/sizeof(uint32_t));

    int32_t availableRawSizes[2] = {
        m_curCameraInfo->sensorRawW, m_curCameraInfo->sensorRawH
    };
    ADD_OR_SIZE(ANDROID_SCALER_AVAILABLE_RAW_SIZES,
            availableRawSizes, 2);

    ADD_OR_SIZE(ANDROID_SCALER_AVAILABLE_RAW_MIN_DURATIONS,
            kAvailableRawMinDurations,
            sizeof(kAvailableRawMinDurations)/sizeof(uint64_t));


    ADD_OR_SIZE(ANDROID_SCALER_AVAILABLE_PROCESSED_SIZES,
        m_curCameraInfo->scalerResolutions,
        (m_curCameraInfo->numScalerResolution)*2);
    ADD_OR_SIZE(ANDROID_SCALER_AVAILABLE_JPEG_SIZES,
        m_curCameraInfo->jpegResolutions,
        (m_curCameraInfo->numJpegResolution)*2);

    ADD_OR_SIZE(ANDROID_SCALER_AVAILABLE_PROCESSED_MIN_DURATIONS,
            kAvailableProcessedMinDurations,
            sizeof(kAvailableProcessedMinDurations)/sizeof(uint64_t));

    ADD_OR_SIZE(ANDROID_SCALER_AVAILABLE_JPEG_MIN_DURATIONS,
            kAvailableJpegMinDurations,
            sizeof(kAvailableJpegMinDurations)/sizeof(uint64_t));

    static const float maxZoom = 4;
    ADD_OR_SIZE(ANDROID_SCALER_AVAILABLE_MAX_DIGITAL_ZOOM, &maxZoom, 1);

    // android.jpeg

    size_t sizeOfThumbnailList = (cameraId) ? sizeof(SUPPORT_THUMBNAIL_FRONT_SIZE) /
            sizeof(int32_t) : sizeof(SUPPORT_THUMBNAIL_REAR_SIZE) / sizeof(int32_t);

    // Copy sizes from front or back camera
    int32_t jpegThumbnailSizes[sizeOfThumbnailList + 2];

    if (cameraId) {
        memcpy(jpegThumbnailSizes, SUPPORT_THUMBNAIL_FRONT_SIZE,
                sizeof(int32_t) * sizeOfThumbnailList);
    } else {
        memcpy(jpegThumbnailSizes, SUPPORT_THUMBNAIL_REAR_SIZE,
                sizeof(int32_t) * sizeOfThumbnailList);
    }

    // Always include 0,0 size in list
    jpegThumbnailSizes[sizeOfThumbnailList] = 0;
    jpegThumbnailSizes[sizeOfThumbnailList + 1] = 0;

    ADD_OR_SIZE(ANDROID_JPEG_AVAILABLE_THUMBNAIL_SIZES,
            jpegThumbnailSizes, sizeof(jpegThumbnailSizes)/sizeof(int32_t));

    static const int32_t jpegMaxSize = 10 * 1024 * 1024;
    ADD_OR_SIZE(ANDROID_JPEG_MAX_SIZE, &jpegMaxSize, 1);

    // android.stats

    static const uint8_t availableFaceDetectModes[] = {
            ANDROID_STATISTICS_FACE_DETECT_MODE_OFF,
            ANDROID_STATISTICS_FACE_DETECT_MODE_FULL
    };
    ADD_OR_SIZE(ANDROID_STATISTICS_INFO_AVAILABLE_FACE_DETECT_MODES,
            availableFaceDetectModes,
            sizeof(availableFaceDetectModes));

    m_curCameraInfo->maxFaceCount = CAMERA2_MAX_FACES;
    ADD_OR_SIZE(ANDROID_STATISTICS_INFO_MAX_FACE_COUNT,
            &(m_curCameraInfo->maxFaceCount), 1);

    static const int32_t histogramSize = 64;
    ADD_OR_SIZE(ANDROID_STATISTICS_INFO_HISTOGRAM_BUCKET_COUNT,
            &histogramSize, 1);

    static const int32_t maxHistogramCount = 1000;
    ADD_OR_SIZE(ANDROID_STATISTICS_INFO_MAX_HISTOGRAM_COUNT,
            &maxHistogramCount, 1);

    static const int32_t sharpnessMapSize[2] = {64, 64};
    ADD_OR_SIZE(ANDROID_STATISTICS_INFO_SHARPNESS_MAP_SIZE,
            sharpnessMapSize, sizeof(sharpnessMapSize)/sizeof(int32_t));

    static const int32_t maxSharpnessMapValue = 1000;
    ADD_OR_SIZE(ANDROID_STATISTICS_INFO_MAX_SHARPNESS_MAP_VALUE,
            &maxSharpnessMapValue, 1);

    // android.control

    static const uint8_t availableSceneModes[] = {
            ANDROID_CONTROL_SCENE_MODE_ACTION,
            ANDROID_CONTROL_SCENE_MODE_NIGHT,
            ANDROID_CONTROL_SCENE_MODE_SUNSET,
            ANDROID_CONTROL_SCENE_MODE_PARTY
    };
    ADD_OR_SIZE(ANDROID_CONTROL_AVAILABLE_SCENE_MODES,
            availableSceneModes, sizeof(availableSceneModes));

    static const uint8_t availableEffects[] = {
            ANDROID_CONTROL_EFFECT_MODE_OFF
    };
    ADD_OR_SIZE(ANDROID_CONTROL_AVAILABLE_EFFECTS,
            availableEffects, sizeof(availableEffects));

    static const int32_t max3aRegions[] = {/*AE*/ 1,/*AWB*/ 1,/*AF*/ 1};
    ADD_OR_SIZE(ANDROID_CONTROL_MAX_REGIONS,
            max3aRegions, sizeof(max3aRegions)/sizeof(max3aRegions[0]));

    ADD_OR_SIZE(ANDROID_CONTROL_AE_AVAILABLE_MODES,
            m_curCameraInfo->availableAeModes, m_curCameraInfo->numAvailableAeModes);

    static const camera_metadata_rational exposureCompensationStep = {
            1, 1
    };
    ADD_OR_SIZE(ANDROID_CONTROL_AE_COMPENSATION_STEP,
            &exposureCompensationStep, 1);

    int32_t exposureCompensationRange[] = {-3, 3};
    ADD_OR_SIZE(ANDROID_CONTROL_AE_COMPENSATION_RANGE,
            exposureCompensationRange,
            sizeof(exposureCompensationRange)/sizeof(int32_t));

    static const int32_t availableTargetFpsRanges[] = {
            15, 15, 24, 24, 25, 25, 15, 30, 30, 30
    };
    ADD_OR_SIZE(ANDROID_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES,
            availableTargetFpsRanges,
            sizeof(availableTargetFpsRanges)/sizeof(int32_t));

    static const uint8_t availableAntibandingModes[] = {
            ANDROID_CONTROL_AE_ANTIBANDING_MODE_OFF,
            ANDROID_CONTROL_AE_ANTIBANDING_MODE_AUTO
    };
    ADD_OR_SIZE(ANDROID_CONTROL_AE_AVAILABLE_ANTIBANDING_MODES,
            availableAntibandingModes, sizeof(availableAntibandingModes));

    static const uint8_t availableAwbModes[] = {
            ANDROID_CONTROL_AWB_MODE_OFF,
            ANDROID_CONTROL_AWB_MODE_AUTO,
            ANDROID_CONTROL_AWB_MODE_INCANDESCENT,
            ANDROID_CONTROL_AWB_MODE_FLUORESCENT,
            ANDROID_CONTROL_AWB_MODE_DAYLIGHT,
            ANDROID_CONTROL_AWB_MODE_CLOUDY_DAYLIGHT
    };
    ADD_OR_SIZE(ANDROID_CONTROL_AWB_AVAILABLE_MODES,
            availableAwbModes, sizeof(availableAwbModes));

    ADD_OR_SIZE(ANDROID_CONTROL_AF_AVAILABLE_MODES,
                m_curCameraInfo->availableAfModes, m_curCameraInfo->numAvailableAfModes);

    static const uint8_t availableVstabModes[] = {
            ANDROID_CONTROL_VIDEO_STABILIZATION_MODE_OFF,
            ANDROID_CONTROL_VIDEO_STABILIZATION_MODE_ON
    };
    ADD_OR_SIZE(ANDROID_CONTROL_AVAILABLE_VIDEO_STABILIZATION_MODES,
            availableVstabModes, sizeof(availableVstabModes));

    ADD_OR_SIZE(ANDROID_CONTROL_SCENE_MODE_OVERRIDES,
            m_curCameraInfo->sceneModeOverrides, m_curCameraInfo->numSceneModeOverrides);

    static const uint8_t quirkTriggerAuto = 1;
    ADD_OR_SIZE(ANDROID_QUIRKS_TRIGGER_AF_WITH_AUTO,
            &quirkTriggerAuto, 1);

    static const uint8_t quirkUseZslFormat = 1;
    ADD_OR_SIZE(ANDROID_QUIRKS_USE_ZSL_FORMAT,
            &quirkUseZslFormat, 1);

    static const uint8_t quirkMeteringCropRegion = 1;
    ADD_OR_SIZE(ANDROID_QUIRKS_METERING_CROP_REGION,
            &quirkMeteringCropRegion, 1);


#undef ADD_OR_SIZE
    /** Allocate metadata if sizing */
    if (sizeRequest) {
        ALOGV("Allocating %d entries, %d extra bytes for "
                "static camera info",
                entryCount, dataCount);
        *info = allocate_camera_metadata(entryCount, dataCount);
        if (*info == NULL) {
            ALOGE("Unable to allocate camera static info"
                    "(%d entries, %d bytes extra data)",
                    entryCount, dataCount);
            return NO_MEMORY;
        }
    }
    return OK;
}

status_t ExynosCamera2::constructDefaultRequest(
        int request_template,
        camera_metadata_t **request,
        bool sizeRequest) {

    size_t entryCount = 0;
    size_t dataCount = 0;
    status_t ret;

#define ADD_OR_SIZE( tag, data, count ) \
    if ( ( ret = addOrSize(*request, sizeRequest, &entryCount, &dataCount, \
            tag, data, count) ) != OK ) return ret

    static const int64_t USEC = 1000LL;
    static const int64_t MSEC = USEC * 1000LL;
    static const int64_t SEC = MSEC * 1000LL;

    /** android.request */

    static const uint8_t metadataMode = ANDROID_REQUEST_METADATA_MODE_NONE;
    ADD_OR_SIZE(ANDROID_REQUEST_METADATA_MODE, &metadataMode, 1);

    static const int32_t id = 0;
    ADD_OR_SIZE(ANDROID_REQUEST_ID, &id, 1);

    static const int32_t frameCount = 0;
    ADD_OR_SIZE(ANDROID_REQUEST_FRAME_COUNT, &frameCount, 1);

    // OUTPUT_STREAMS set by user
    entryCount += 1;
    dataCount += 5; // TODO: Should be maximum stream number

    /** android.lens */

    static const float focusDistance = 0;
    ADD_OR_SIZE(ANDROID_LENS_FOCUS_DISTANCE, &focusDistance, 1);

    ADD_OR_SIZE(ANDROID_LENS_APERTURE, &m_curCameraInfo->aperture, 1);

    ADD_OR_SIZE(ANDROID_LENS_FOCAL_LENGTH, &m_curCameraInfo->focalLength, 1);

    static const float filterDensity = 0;
    ADD_OR_SIZE(ANDROID_LENS_FILTER_DENSITY, &filterDensity, 1);

    static const uint8_t opticalStabilizationMode =
            ANDROID_LENS_OPTICAL_STABILIZATION_MODE_OFF;
    ADD_OR_SIZE(ANDROID_LENS_OPTICAL_STABILIZATION_MODE,
            &opticalStabilizationMode, 1);


    /** android.sensor */

    static const int64_t defaultExposureTime = 8000000LL; // 1/125 s
    ADD_OR_SIZE(ANDROID_SENSOR_EXPOSURE_TIME, &defaultExposureTime, 1);

    static const int64_t frameDuration = 33333333L; // 1/30 s
    ADD_OR_SIZE(ANDROID_SENSOR_FRAME_DURATION, &frameDuration, 1);


    /** android.flash */

    static const uint8_t flashMode = ANDROID_FLASH_MODE_OFF;
    ADD_OR_SIZE(ANDROID_FLASH_MODE, &flashMode, 1);

    static const uint8_t flashPower = 10;
    ADD_OR_SIZE(ANDROID_FLASH_FIRING_POWER, &flashPower, 1);

    static const int64_t firingTime = 0;
    ADD_OR_SIZE(ANDROID_FLASH_FIRING_TIME, &firingTime, 1);

    /** Processing block modes */
    uint8_t hotPixelMode = 0;
    uint8_t demosaicMode = 0;
    uint8_t noiseMode = 0;
    uint8_t shadingMode = 0;
    uint8_t colorMode = 0;
    uint8_t tonemapMode = 0;
    uint8_t edgeMode = 0;
    uint8_t vstabMode = ANDROID_CONTROL_VIDEO_STABILIZATION_MODE_OFF;

    switch (request_template) {
      case CAMERA2_TEMPLATE_VIDEO_SNAPSHOT:
        vstabMode = ANDROID_CONTROL_VIDEO_STABILIZATION_MODE_ON;
        // fall-through
      case CAMERA2_TEMPLATE_STILL_CAPTURE:
        // fall-through
      case CAMERA2_TEMPLATE_ZERO_SHUTTER_LAG:
        hotPixelMode = ANDROID_HOT_PIXEL_MODE_HIGH_QUALITY;
        demosaicMode = ANDROID_DEMOSAIC_MODE_HIGH_QUALITY;
        noiseMode = ANDROID_NOISE_REDUCTION_MODE_HIGH_QUALITY;
        shadingMode = ANDROID_SHADING_MODE_HIGH_QUALITY;
        colorMode = ANDROID_COLOR_CORRECTION_MODE_HIGH_QUALITY;
        tonemapMode = ANDROID_TONEMAP_MODE_HIGH_QUALITY;
        edgeMode = ANDROID_EDGE_MODE_HIGH_QUALITY;
        break;
      case CAMERA2_TEMPLATE_VIDEO_RECORD:
        vstabMode = ANDROID_CONTROL_VIDEO_STABILIZATION_MODE_ON;
        // fall-through
      case CAMERA2_TEMPLATE_PREVIEW:
        // fall-through
      default:
        hotPixelMode = ANDROID_HOT_PIXEL_MODE_FAST;
        demosaicMode = ANDROID_DEMOSAIC_MODE_FAST;
        noiseMode = ANDROID_NOISE_REDUCTION_MODE_FAST;
        shadingMode = ANDROID_SHADING_MODE_FAST;
        colorMode = ANDROID_COLOR_CORRECTION_MODE_FAST;
        tonemapMode = ANDROID_TONEMAP_MODE_FAST;
        edgeMode = ANDROID_EDGE_MODE_FAST;
        break;
    }
    ADD_OR_SIZE(ANDROID_HOT_PIXEL_MODE, &hotPixelMode, 1);
    ADD_OR_SIZE(ANDROID_DEMOSAIC_MODE, &demosaicMode, 1);
    ADD_OR_SIZE(ANDROID_NOISE_REDUCTION_MODE, &noiseMode, 1);
    ADD_OR_SIZE(ANDROID_SHADING_MODE, &shadingMode, 1);
    ADD_OR_SIZE(ANDROID_COLOR_CORRECTION_MODE, &colorMode, 1);
    ADD_OR_SIZE(ANDROID_TONEMAP_MODE, &tonemapMode, 1);
    ADD_OR_SIZE(ANDROID_EDGE_MODE, &edgeMode, 1);
    ADD_OR_SIZE(ANDROID_CONTROL_VIDEO_STABILIZATION_MODE, &vstabMode, 1);

    /** android.noise */
    static const uint8_t noiseStrength = 5;
    ADD_OR_SIZE(ANDROID_NOISE_REDUCTION_STRENGTH, &noiseStrength, 1);

    /** android.color */
    static const float colorTransform[9] = {
        1.0f, 0.f, 0.f,
        0.f, 1.f, 0.f,
        0.f, 0.f, 1.f
    };
    ADD_OR_SIZE(ANDROID_COLOR_CORRECTION_TRANSFORM, colorTransform, 9);

    /** android.tonemap */
    static const float tonemapCurve[4] = {
        0.f, 0.f,
        1.f, 1.f
    };
    ADD_OR_SIZE(ANDROID_TONEMAP_CURVE_RED, tonemapCurve, 32); // sungjoong
    ADD_OR_SIZE(ANDROID_TONEMAP_CURVE_GREEN, tonemapCurve, 32);
    ADD_OR_SIZE(ANDROID_TONEMAP_CURVE_BLUE, tonemapCurve, 32);

    /** android.edge */
    static const uint8_t edgeStrength = 5;
    ADD_OR_SIZE(ANDROID_EDGE_STRENGTH, &edgeStrength, 1);

    /** android.scaler */
    int32_t cropRegion[3] = {
        0, 0, m_curCameraInfo->sensorW
    };
    ADD_OR_SIZE(ANDROID_SCALER_CROP_REGION, cropRegion, 3);

    /** android.jpeg */
    static const int32_t jpegQuality = 100;
    ADD_OR_SIZE(ANDROID_JPEG_QUALITY, &jpegQuality, 1);

    static const int32_t thumbnailSize[2] = {
        160, 120
    };
    ADD_OR_SIZE(ANDROID_JPEG_THUMBNAIL_SIZE, thumbnailSize, 2);

    static const int32_t thumbnailQuality = 100;
    ADD_OR_SIZE(ANDROID_JPEG_THUMBNAIL_QUALITY, &thumbnailQuality, 1);

    static const double gpsCoordinates[3] = {
        0, 0, 0
    };
    ADD_OR_SIZE(ANDROID_JPEG_GPS_COORDINATES, gpsCoordinates, 3);

    static const uint8_t gpsProcessingMethod[32] = "None";
    ADD_OR_SIZE(ANDROID_JPEG_GPS_PROCESSING_METHOD, gpsProcessingMethod, 32);

    static const int64_t gpsTimestamp = 0;
    ADD_OR_SIZE(ANDROID_JPEG_GPS_TIMESTAMP, &gpsTimestamp, 1);

    static const int32_t jpegOrientation = 0;
    ADD_OR_SIZE(ANDROID_JPEG_ORIENTATION, &jpegOrientation, 1);

    /** android.stats */

    static const uint8_t faceDetectMode = ANDROID_STATISTICS_FACE_DETECT_MODE_FULL;
    ADD_OR_SIZE(ANDROID_STATISTICS_FACE_DETECT_MODE, &faceDetectMode, 1);

    static const uint8_t histogramMode = ANDROID_STATISTICS_HISTOGRAM_MODE_OFF;
    ADD_OR_SIZE(ANDROID_STATISTICS_HISTOGRAM_MODE, &histogramMode, 1);

    static const uint8_t sharpnessMapMode = ANDROID_STATISTICS_HISTOGRAM_MODE_OFF;
    ADD_OR_SIZE(ANDROID_STATISTICS_SHARPNESS_MAP_MODE, &sharpnessMapMode, 1);


    /** android.control */

    uint8_t controlIntent = 0;
    switch (request_template) {
      case CAMERA2_TEMPLATE_PREVIEW:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW;
        break;
      case CAMERA2_TEMPLATE_STILL_CAPTURE:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE;
        break;
      case CAMERA2_TEMPLATE_VIDEO_RECORD:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD;
        break;
      case CAMERA2_TEMPLATE_VIDEO_SNAPSHOT:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT;
        break;
      case CAMERA2_TEMPLATE_ZERO_SHUTTER_LAG:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_ZERO_SHUTTER_LAG;
        break;
      default:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_CUSTOM;
        break;
    }
    ADD_OR_SIZE(ANDROID_CONTROL_CAPTURE_INTENT, &controlIntent, 1);

    static const uint8_t controlMode = ANDROID_CONTROL_MODE_AUTO;
    ADD_OR_SIZE(ANDROID_CONTROL_MODE, &controlMode, 1);

    static const uint8_t effectMode = ANDROID_CONTROL_EFFECT_MODE_OFF;
    ADD_OR_SIZE(ANDROID_CONTROL_EFFECT_MODE, &effectMode, 1);

    static const uint8_t sceneMode = ANDROID_CONTROL_SCENE_MODE_DISABLED;
    ADD_OR_SIZE(ANDROID_CONTROL_SCENE_MODE, &sceneMode, 1);

    static const uint8_t aeMode = ANDROID_CONTROL_AE_MODE_ON;
    ADD_OR_SIZE(ANDROID_CONTROL_AE_MODE, &aeMode, 1);

    int32_t controlRegions[5] = {
        0, 0, m_curCameraInfo->sensorW, m_curCameraInfo->sensorH, 1000
    };
    ADD_OR_SIZE(ANDROID_CONTROL_AE_REGIONS, controlRegions, 5);

    static const int32_t aeExpCompensation = 0;
    ADD_OR_SIZE(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, &aeExpCompensation, 1);

    static const int32_t aeTargetFpsRange[2] = {
        15, 30
    };
    ADD_OR_SIZE(ANDROID_CONTROL_AE_TARGET_FPS_RANGE, aeTargetFpsRange, 2);

    static const uint8_t aeAntibandingMode =
            ANDROID_CONTROL_AE_ANTIBANDING_MODE_AUTO;
    ADD_OR_SIZE(ANDROID_CONTROL_AE_ANTIBANDING_MODE, &aeAntibandingMode, 1);

    static const uint8_t awbMode =
            ANDROID_CONTROL_AWB_MODE_AUTO;
    ADD_OR_SIZE(ANDROID_CONTROL_AWB_MODE, &awbMode, 1);

    ADD_OR_SIZE(ANDROID_CONTROL_AWB_REGIONS, controlRegions, 5);

    uint8_t afMode = 0;
    switch (request_template) {
      case CAMERA2_TEMPLATE_PREVIEW:
        afMode = ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE;
        break;
      case CAMERA2_TEMPLATE_STILL_CAPTURE:
        afMode = ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE;
        break;
      case CAMERA2_TEMPLATE_VIDEO_RECORD:
        afMode = ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO;
        break;
      case CAMERA2_TEMPLATE_VIDEO_SNAPSHOT:
        afMode = ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO;
        break;
      case CAMERA2_TEMPLATE_ZERO_SHUTTER_LAG:
        afMode = ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE;
        break;
      default:
        afMode = ANDROID_CONTROL_AF_MODE_AUTO;
        break;
    }
    ADD_OR_SIZE(ANDROID_CONTROL_AF_MODE, &afMode, 1);

    ADD_OR_SIZE(ANDROID_CONTROL_AF_REGIONS, controlRegions, 5);

    if (sizeRequest) {
        ALOGV("Allocating %d entries, %d extra bytes for "
                "request template type %d",
                entryCount, dataCount, request_template);
        *request = allocate_camera_metadata(entryCount, dataCount);
        if (*request == NULL) {
            ALOGE("Unable to allocate new request template type %d "
                    "(%d entries, %d bytes extra data)", request_template,
                    entryCount, dataCount);
            return NO_MEMORY;
        }
    }
    return OK;
#undef ADD_OR_SIZE
}

}
