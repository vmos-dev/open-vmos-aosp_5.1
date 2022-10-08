/*
**
** Copyright 2008, The Android Open Source Project
** Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
** Not a Contribution. Apache license notifications and license are
** retained for attribution purposes only.
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
#ifndef ANDROID_HARDWARE_QCAMERA_PARAMETERS_H
#define ANDROID_HARDWARE_QCAMERA_PARAMETERS_H

#include <camera/CameraParameters.h>
#include <cutils/properties.h>
#include <hardware/camera.h>
#include <stdlib.h>
#include <utils/Errors.h>
#include "cam_intf.h"
#include "QCameraMem.h"
#include "QCameraThermalAdapter.h"

extern "C" {
#include <mm_jpeg_interface.h>
}

using namespace android;

namespace qcamera {

//EXIF globals
static const char ExifAsciiPrefix[] = { 0x41, 0x53, 0x43, 0x49, 0x49, 0x0, 0x0, 0x0 };          // "ASCII\0\0\0"
static const char ExifUndefinedPrefix[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };   // "\0\0\0\0\0\0\0\0"

#define GPS_PROCESSING_METHOD_SIZE       101
#define EXIF_ASCII_PREFIX_SIZE           8   //(sizeof(ExifAsciiPrefix))
#define FOCAL_LENGTH_DECIMAL_PRECISION   100

class QCameraParameters: public CameraParameters
{
public:
    QCameraParameters();
    QCameraParameters(const String8 &params);
    ~QCameraParameters();

    // Supported PREVIEW/RECORDING SIZES IN HIGH FRAME RATE recording, sizes in pixels.
    // Example value: "800x480,432x320". Read only.
    static const char KEY_QC_SUPPORTED_HFR_SIZES[];
    // The mode of preview frame rate.
    // Example value: "frame-rate-auto, frame-rate-fixed".
    static const char KEY_QC_PREVIEW_FRAME_RATE_MODE[];
    static const char KEY_QC_SUPPORTED_PREVIEW_FRAME_RATE_MODES[];
    static const char KEY_QC_PREVIEW_FRAME_RATE_AUTO_MODE[];
    static const char KEY_QC_PREVIEW_FRAME_RATE_FIXED_MODE[];
    static const char KEY_QC_SUPPORTED_SKIN_TONE_ENHANCEMENT_MODES[] ;

    // Supported live snapshot sizes
    static const char KEY_QC_SUPPORTED_LIVESNAPSHOT_SIZES[];

    // Supported Raw formats
    static const char KEY_QC_SUPPORTED_RAW_FORMATS[];
    static const char KEY_QC_RAW_FORMAT[];

    //Touch Af/AEC settings.
    static const char KEY_QC_TOUCH_AF_AEC[];
    static const char KEY_QC_SUPPORTED_TOUCH_AF_AEC[];
    //Touch Index for AEC.
    static const char KEY_QC_TOUCH_INDEX_AEC[];
    //Touch Index for AF.
    static const char KEY_QC_TOUCH_INDEX_AF[];
    // Current auto scene detection mode.
    // Example value: "off" or "on" constants. Read/write.
    static const char KEY_QC_SCENE_DETECT[];
    // Supported auto scene detection settings.
    // Example value: "off,on". Read only.
    static const char KEY_QC_SUPPORTED_SCENE_DETECT[];
    static const char KEY_QC_SELECTABLE_ZONE_AF[];

    static const char KEY_QC_ISO_MODE[];
    static const char KEY_QC_SUPPORTED_ISO_MODES[];
    static const char KEY_QC_LENSSHADE[] ;
    static const char KEY_QC_SUPPORTED_LENSSHADE_MODES[] ;
    static const char KEY_QC_AUTO_EXPOSURE[];
    static const char KEY_QC_SUPPORTED_AUTO_EXPOSURE[];

    static const char KEY_QC_GPS_LATITUDE_REF[];
    static const char KEY_QC_GPS_LONGITUDE_REF[];
    static const char KEY_QC_GPS_ALTITUDE_REF[];
    static const char KEY_QC_GPS_STATUS[];
    static const char KEY_QC_MEMORY_COLOR_ENHANCEMENT[];
    static const char KEY_QC_SUPPORTED_MEM_COLOR_ENHANCE_MODES[];
    static const char KEY_QC_DIS[];
    static const char KEY_QC_SUPPORTED_DIS_MODES[];

    static const char KEY_QC_ZSL[];
    static const char KEY_QC_SUPPORTED_ZSL_MODES[];
    static const char KEY_QC_ZSL_BURST_INTERVAL[];
    static const char KEY_QC_ZSL_BURST_LOOKBACK[];
    static const char KEY_QC_ZSL_QUEUE_DEPTH[];

    static const char KEY_QC_CAMERA_MODE[];
    static const char KEY_QC_ORIENTATION[];

    static const char KEY_QC_VIDEO_HIGH_FRAME_RATE[];
    static const char KEY_QC_SUPPORTED_VIDEO_HIGH_FRAME_RATE_MODES[];
    static const char KEY_QC_HIGH_DYNAMIC_RANGE_IMAGING[];
    static const char KEY_QC_SUPPORTED_HDR_IMAGING_MODES[];
    static const char KEY_QC_AE_BRACKET_HDR[];
    static const char KEY_QC_SUPPORTED_AE_BRACKET_MODES[];
    static const char KEY_QC_CAPTURE_BURST_EXPOSURE[];
    static const char KEY_QC_NUM_SNAPSHOT_PER_SHUTTER[];
    static const char KEY_QC_NO_DISPLAY_MODE[];
    static const char KEY_QC_RAW_PICUTRE_SIZE[];

    // DENOISE
    static const char KEY_QC_DENOISE[];
    static const char KEY_QC_SUPPORTED_DENOISE[];

    //Selectable zone AF.
    static const char KEY_QC_FOCUS_ALGO[];
    static const char KEY_QC_SUPPORTED_FOCUS_ALGOS[];

    //Face Detection
    static const char KEY_QC_FACE_DETECTION[];
    static const char KEY_QC_SUPPORTED_FACE_DETECTION[];

    //Face Recognition
    static const char KEY_QC_FACE_RECOGNITION[];
    static const char KEY_QC_SUPPORTED_FACE_RECOGNITION[];

    // supported camera features to be queried by Snapdragon SDK
    //Read only
    static const char KEY_QC_SUPPORTED_CAMERA_FEATURES[];

    //Indicates number of faces requested by the application.
    //This value will be rejected if the requested faces
    //greater than supported by hardware.
    //Write only.
    static const char KEY_QC_MAX_NUM_REQUESTED_FACES[];

    //preview flip
    static const char KEY_QC_PREVIEW_FLIP[];
    //video flip
    static const char KEY_QC_VIDEO_FLIP[];
    //snapshot picture flip
    static const char KEY_QC_SNAPSHOT_PICTURE_FLIP[];

    static const char KEY_QC_SUPPORTED_FLIP_MODES[];

    //Redeye Reduction
    static const char KEY_QC_REDEYE_REDUCTION[];
    static const char KEY_QC_SUPPORTED_REDEYE_REDUCTION[];
    static const char EFFECT_EMBOSS[];
    static const char EFFECT_SKETCH[];
    static const char EFFECT_NEON[];

    // Values for Touch AF/AEC
    static const char TOUCH_AF_AEC_OFF[];
    static const char TOUCH_AF_AEC_ON[];

    // Values for Scene mode
    static const char SCENE_MODE_ASD[];
    static const char SCENE_MODE_BACKLIGHT[];
    static const char SCENE_MODE_FLOWERS[];
    static const char SCENE_MODE_AR[];
    static const char SCENE_MODE_HDR[];
    static const char PIXEL_FORMAT_YUV420SP_ADRENO[]; // ADRENO
    static const char PIXEL_FORMAT_YV12[]; // NV12
    static const char PIXEL_FORMAT_NV12[]; //NV12

    // Values for raw picture format
    static const char QC_PIXEL_FORMAT_YUV_RAW_8BIT_YUYV[];
    static const char QC_PIXEL_FORMAT_YUV_RAW_8BIT_YVYU[];
    static const char QC_PIXEL_FORMAT_YUV_RAW_8BIT_UYVY[];
    static const char QC_PIXEL_FORMAT_YUV_RAW_8BIT_VYUY[];
    static const char QC_PIXEL_FORMAT_BAYER_QCOM_RAW_8GBRG[];
    static const char QC_PIXEL_FORMAT_BAYER_QCOM_RAW_8GRBG[];
    static const char QC_PIXEL_FORMAT_BAYER_QCOM_RAW_8RGGB[];
    static const char QC_PIXEL_FORMAT_BAYER_QCOM_RAW_8BGGR[];
    static const char QC_PIXEL_FORMAT_BAYER_QCOM_RAW_10GBRG[];
    static const char QC_PIXEL_FORMAT_BAYER_QCOM_RAW_10GRBG[];
    static const char QC_PIXEL_FORMAT_BAYER_QCOM_RAW_10RGGB[];
    static const char QC_PIXEL_FORMAT_BAYER_QCOM_RAW_10BGGR[];
    static const char QC_PIXEL_FORMAT_BAYER_QCOM_RAW_12GBRG[];
    static const char QC_PIXEL_FORMAT_BAYER_QCOM_RAW_12GRBG[];
    static const char QC_PIXEL_FORMAT_BAYER_QCOM_RAW_12RGGB[];
    static const char QC_PIXEL_FORMAT_BAYER_QCOM_RAW_12BGGR[];
    static const char QC_PIXEL_FORMAT_BAYER_MIPI_RAW_8GBRG[];
    static const char QC_PIXEL_FORMAT_BAYER_MIPI_RAW_8GRBG[];
    static const char QC_PIXEL_FORMAT_BAYER_MIPI_RAW_8RGGB[];
    static const char QC_PIXEL_FORMAT_BAYER_MIPI_RAW_8BGGR[];
    static const char QC_PIXEL_FORMAT_BAYER_MIPI_RAW_10GBRG[];
    static const char QC_PIXEL_FORMAT_BAYER_MIPI_RAW_10GRBG[];
    static const char QC_PIXEL_FORMAT_BAYER_MIPI_RAW_10RGGB[];
    static const char QC_PIXEL_FORMAT_BAYER_MIPI_RAW_10BGGR[];
    static const char QC_PIXEL_FORMAT_BAYER_MIPI_RAW_12GBRG[];
    static const char QC_PIXEL_FORMAT_BAYER_MIPI_RAW_12GRBG[];
    static const char QC_PIXEL_FORMAT_BAYER_MIPI_RAW_12RGGB[];
    static const char QC_PIXEL_FORMAT_BAYER_MIPI_RAW_12BGGR[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_8GBRG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_8GRBG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_8RGGB[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_8BGGR[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_10GBRG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_10GRBG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_10RGGB[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_10BGGR[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_12GBRG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_12GRBG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_12RGGB[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_QCOM_12BGGR[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_8GBRG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_8GRBG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_8RGGB[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_8BGGR[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_10GBRG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_10GRBG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_10RGGB[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_10BGGR[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_12GBRG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_12GRBG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_12RGGB[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_MIPI_12BGGR[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN8_8GBRG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN8_8GRBG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN8_8RGGB[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN8_8BGGR[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_8GBRG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_8GRBG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_8RGGB[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_8BGGR[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_10GBRG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_10GRBG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_10RGGB[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_10BGGR[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_12GBRG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_12GRBG[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_12RGGB[];
    static const char QC_PIXEL_FORMAT_BAYER_IDEAL_PLAIN16_12BGGR[];

    // ISO values
    static const char ISO_AUTO[];
    static const char ISO_HJR[];
    static const char ISO_100[];
    static const char ISO_200[];
    static const char ISO_400[];
    static const char ISO_800[];
    static const char ISO_1600[];

    // Values for auto exposure settings.
    static const char AUTO_EXPOSURE_FRAME_AVG[];
    static const char AUTO_EXPOSURE_CENTER_WEIGHTED[];
    static const char AUTO_EXPOSURE_SPOT_METERING[];
    static const char AUTO_EXPOSURE_SMART_METERING[];
    static const char AUTO_EXPOSURE_USER_METERING[];
    static const char AUTO_EXPOSURE_SPOT_METERING_ADV[];
    static const char AUTO_EXPOSURE_CENTER_WEIGHTED_ADV[];

    static const char KEY_QC_SHARPNESS[];
    static const char KEY_QC_MIN_SHARPNESS[];
    static const char KEY_QC_MAX_SHARPNESS[];
    static const char KEY_QC_SHARPNESS_STEP[];
    static const char KEY_QC_CONTRAST[];
    static const char KEY_QC_MIN_CONTRAST[];
    static const char KEY_QC_MAX_CONTRAST[];
    static const char KEY_QC_CONTRAST_STEP[];
    static const char KEY_QC_SATURATION[];
    static const char KEY_QC_MIN_SATURATION[];
    static const char KEY_QC_MAX_SATURATION[];
    static const char KEY_QC_SATURATION_STEP[];
    static const char KEY_QC_BRIGHTNESS[];
    static const char KEY_QC_MIN_BRIGHTNESS[];
    static const char KEY_QC_MAX_BRIGHTNESS[];
    static const char KEY_QC_BRIGHTNESS_STEP[];
    static const char KEY_QC_SCE_FACTOR[];
    static const char KEY_QC_MIN_SCE_FACTOR[];
    static const char KEY_QC_MAX_SCE_FACTOR[];
    static const char KEY_QC_SCE_FACTOR_STEP[];

    static const char KEY_QC_HISTOGRAM[] ;
    static const char KEY_QC_SUPPORTED_HISTOGRAM_MODES[] ;
    static const char KEY_QC_HDR_NEED_1X[];
    static const char KEY_QC_VIDEO_HDR[];
    static const char KEY_QC_SUPPORTED_VIDEO_HDR_MODES[];

    // Values for SKIN TONE ENHANCEMENT
    static const char SKIN_TONE_ENHANCEMENT_ENABLE[] ;
    static const char SKIN_TONE_ENHANCEMENT_DISABLE[] ;

    // Values for Denoise
    static const char DENOISE_OFF[] ;
    static const char DENOISE_ON[] ;

    // Values for auto exposure settings.
    static const char FOCUS_ALGO_AUTO[];
    static const char FOCUS_ALGO_SPOT_METERING[];
    static const char FOCUS_ALGO_CENTER_WEIGHTED[];
    static const char FOCUS_ALGO_FRAME_AVERAGE[];

    // Values for AE Bracketing settings.
    static const char AE_BRACKET_OFF[];
    static const char AE_BRACKET[];

    // Values for HFR settings.
    static const char VIDEO_HFR_OFF[];
    static const char VIDEO_HFR_2X[];
    static const char VIDEO_HFR_3X[];
    static const char VIDEO_HFR_4X[];
    static const char VIDEO_HFR_5X[];

    // Values for feature on/off settings.
    static const char VALUE_OFF[];
    static const char VALUE_ON[];

    // Values for feature enable/disable settings.
    static const char VALUE_ENABLE[];
    static const char VALUE_DISABLE[];

    // Values for feature true/false settings.
    static const char VALUE_FALSE[];
    static const char VALUE_TRUE[];

    //Values for flip settings
    static const char FLIP_MODE_OFF[];
    static const char FLIP_MODE_V[];
    static const char FLIP_MODE_H[];
    static const char FLIP_MODE_VH[];

    enum {
        CAMERA_ORIENTATION_UNKNOWN = 0,
        CAMERA_ORIENTATION_PORTRAIT = 1,
        CAMERA_ORIENTATION_LANDSCAPE = 2,
    };
    typedef struct {
        const char *const desc;
        int val;
    } QCameraMap;

    void getSupportedHfrSizes(Vector<Size> &sizes);
    void setPreviewFrameRateMode(const char *mode);
    const char *getPreviewFrameRateMode() const;
    void setTouchIndexAec(int x, int y);
    void getTouchIndexAec(int *x, int *y);
    void setTouchIndexAf(int x, int y);
    void getTouchIndexAf(int *x, int *y);

    int32_t init(cam_capability_t *, mm_camera_vtbl_t *);
    void deinit();
    int32_t assign(QCameraParameters& params);
    int32_t initDefaultParameters();
    int32_t updateParameters(QCameraParameters&, bool &needRestart);
    int32_t commitParameters();
    int getPreviewHalPixelFormat() const;
    int32_t getStreamFormat(cam_stream_type_t streamType,
                             cam_format_t &format);
    int32_t getStreamDimension(cam_stream_type_t streamType,
                                cam_dimension_t &dim);
    void getThumbnailSize(int *width, int *height) const;

    int getZSLBurstInterval();
    int getZSLQueueDepth();
    int getZSLBackLookCount();
    int getMaxUnmatchedFramesInQueue();
    bool isZSLMode() {return m_bZslMode;};
    bool isNoDisplayMode() {return m_bNoDisplayMode;};
    bool isWNREnabled() {return m_bWNROn;};
    uint8_t getNumOfSnapshots();
    uint8_t getNumOfExtraHDRBufsIfNeeded();
    uint8_t getNumOfHDRBufsIfNeeded();
    int getBurstNum();
    bool getRecordingHintValue() {return m_bRecordingHint;}; // return local copy of video hint
    int setRecordingHintValue(int32_t value); // set local copy of video hint and send to server
                                              // no change in parameters value
    int getJpegQuality();
    int getJpegRotation();

    int32_t getExifDateTime(char *dateTime, uint32_t &count);
    int32_t getExifFocalLength(rat_t *focalLenght);
    uint16_t getExifIsoSpeed();
    int32_t getExifGpsProcessingMethod(char *gpsProcessingMethod, uint32_t &count);
    int32_t getExifLatitude(rat_t *latitude, char *latRef);
    int32_t getExifLongitude(rat_t *longitude, char *lonRef);
    int32_t getExifAltitude(rat_t *altitude, char *altRef);
    int32_t getExifGpsDateTimeStamp(char *gpsDateStamp, uint32_t bufLen, rat_t *gpsTimeStamp);
    int32_t updateFocusDistances(cam_focus_distances_info_t *focusDistances);

    bool isFpsDebugEnabled() {return m_bDebugFps;};
    bool isHistogramEnabled() {return m_bHistogramEnabled;};
    bool isFaceDetectionEnabled() {return ((m_nFaceProcMask & CAM_FACE_PROCESS_MASK_DETECTION) != 0);};
    int32_t setHistogram(bool enabled);
    int32_t setFaceDetection(bool enabled);
    int32_t setLockCAF(bool bLock);
    int32_t setFrameSkip(enum msm_vfe_frame_skip_pattern pattern);
    qcamera_thermal_mode getThermalMode() {return m_ThermalMode;};
    int32_t updateRecordingHintValue(int32_t value);

    cam_focus_mode_type getFocusMode() const {return mFocusMode;};
    int32_t setNumOfSnapshot();
    int32_t adjustPreviewFpsRange(cam_fps_range_t *fpsRange);
    bool isJpegPictureFormat() {return (mPictureFormat == CAM_FORMAT_JPEG);};
    bool isNV16PictureFormat() {return (mPictureFormat == CAM_FORMAT_YUV_422_NV16);};
    cam_denoise_process_type_t getWaveletDenoiseProcessPlate();
    int32_t getLiveSnapshotSize(cam_dimension_t &dim) {dim = m_LiveSnapshotSize; return NO_ERROR;};
    int getFlipMode(cam_stream_type_t streamType);

    void setLockCAFNeeded(bool bNeedflag) {m_bNeedLockCAF = bNeedflag;};
    bool isLockCAFNeeded() {return m_bNeedLockCAF;};
    bool isCAFLocked() {return m_bCAFLocked;};
    void setAFRunning(bool bflag) {m_bAFRunning = bflag;};
    bool isAFRunning() {return m_bAFRunning;};

private:
    int32_t setPreviewSize(const QCameraParameters& );
    int32_t setVideoSize(const QCameraParameters& );
    int32_t setPictureSize(const QCameraParameters& );
    int32_t setLiveSnapshotSize(const QCameraParameters& );
    int32_t setPreviewFormat(const QCameraParameters& );
    int32_t setPictureFormat(const QCameraParameters& );
    int32_t setOrientation(const QCameraParameters& );
    int32_t setJpegThumbnailSize(const QCameraParameters& );
    int32_t setJpegQuality(const QCameraParameters& );
    int32_t setPreviewFpsRange(const QCameraParameters& );
    int32_t setPreviewFrameRate(const QCameraParameters& );
    int32_t setAutoExposure(const QCameraParameters& );
    int32_t setEffect(const QCameraParameters& );
    int32_t setBrightness(const QCameraParameters& );
    int32_t setFocusMode(const QCameraParameters& );
    int32_t setSharpness(const QCameraParameters& );
    int32_t setSaturation(const QCameraParameters& );
    int32_t setContrast(const QCameraParameters& );
    int32_t setSkinToneEnhancement(const QCameraParameters& );
    int32_t setSceneDetect(const QCameraParameters& );
    int32_t setVideoHDR(const QCameraParameters& );
    int32_t setZoom(const QCameraParameters& );
    int32_t setISOValue(const QCameraParameters& );
    int32_t setRotation(const QCameraParameters& );
    int32_t setFlash(const QCameraParameters& );
    int32_t setAecLock(const QCameraParameters& );
    int32_t setAwbLock(const QCameraParameters& );
    int32_t setMCEValue(const QCameraParameters& );
    int32_t setDISValue(const QCameraParameters& params);
    int32_t setHighFrameRate(const QCameraParameters& );
    int32_t setLensShadeValue(const QCameraParameters& );
    int32_t setExposureCompensation(const QCameraParameters& );
    int32_t setWhiteBalance(const QCameraParameters& );
    int32_t setAntibanding(const QCameraParameters& );
    int32_t setFocusAreas(const QCameraParameters& );
    int32_t setMeteringAreas(const QCameraParameters& );
    int32_t setSceneMode(const QCameraParameters& );
    int32_t setSelectableZoneAf(const QCameraParameters& );
    int32_t setAEBracket(const QCameraParameters& );
    int32_t setRedeyeReduction(const QCameraParameters& );
    int32_t setGpsLocation(const QCameraParameters& );
    int32_t setRecordingHint(const QCameraParameters& );
    int32_t setNoDisplayMode(const QCameraParameters& );
    int32_t setWaveletDenoise(const QCameraParameters& );
    int32_t setZslMode(const QCameraParameters& );
    int32_t setZslAttributes(const QCameraParameters& );
    int32_t setCameraMode(const QCameraParameters& );
    int32_t setFaceRecognition(const QCameraParameters& );
    int32_t setFlip(const QCameraParameters& );

    int32_t setAutoExposure(const char *autoExp);
    int32_t setPreviewFpsRange(int minFPS,int maxFPS);
    int32_t setEffect(const char *effect);
    int32_t setBrightness(int brightness);
    int32_t setFocusMode(const char *focusMode);
    int32_t setSharpness(int sharpness);
    int32_t setSaturation(int saturation);
    int32_t setContrast(int contrast);
    int32_t setSkinToneEnhancement(int sceFactor);
    int32_t setSceneDetect(const char *scendDetect);
    int32_t setVideoHDR(const char *videoHDR);
    int32_t setZoom(int zoom_level);
    int32_t setISOValue(const char *isoValue);
    int32_t setFlash(const char *flashStr);
    int32_t setAecLock(const char *aecStr);
    int32_t setAwbLock(const char *awbStr);
    int32_t setMCEValue(const char *mceStr);
    int32_t setDISValue(const char *disStr);
    int32_t setHighFrameRate(const char *hfrStr);
    int32_t setLensShadeValue(const char *lensShadeStr);
    int32_t setExposureCompensation(int expComp);
    int32_t setWhiteBalance(const char *wbStr);
    int32_t setAntibanding(const char *antiBandingStr);
    int32_t setFocusAreas(const char *focusAreasStr);
    int32_t setMeteringAreas(const char *meteringAreasStr);
    int32_t setSceneMode(const char *sceneModeStr);
    int32_t setSelectableZoneAf(const char *selZoneAFStr);
    int32_t setAEBracket(const char *aecBracketStr);
    int32_t setRedeyeReduction(const char *redeyeStr);
    int32_t setWaveletDenoise(const char *wnrStr);
    int32_t setFaceRecognition(const char *faceRecog, int maxFaces);

    int32_t parse_pair(const char *str, int *first, int *second,
                       char delim, char **endptr);
    void parseSizesList(const char *sizesStr, Vector<Size> &sizes);
    int32_t parseNDimVector(const char *str, int *num, int N, char delim);
    int32_t parseCameraAreaString(const char *str, int max_num_areas,
                                  cam_area_t *pAreas, int& num_areas_found);
    bool validateCameraAreas(cam_area_t *areas, int num_areas);
    int parseGPSCoordinate(const char *coord_str, rat_t *coord);
    int32_t getRational(rat_t *rat, int num, int denom);
    String8 createSizesString(const cam_dimension_t *sizes, int len);
    String8 createValuesString(const int *values, int len,
                               const QCameraMap *map, int map_len);
    String8 createValuesStringFromMap(const QCameraMap *map,
                                      int map_len);
    String8 createHfrValuesString(const cam_hfr_info_t *values, int len,
                                  const QCameraMap *map, int map_len);
    String8 createHfrSizesString(const cam_hfr_info_t *values, int len);
    String8 createFpsRangeString(const cam_fps_range_t *fps,
                                 int len,
                                 int &default_fps_index);
    static int compareFPSValues(const void *p1, const void *p2);
    String8 createFpsString(const cam_fps_range_t *fps, int len);
    String8 createZoomRatioValuesString(int *zoomRatios, int length);
    int lookupAttr(const QCameraMap arr[], int len, const char *name);
    const char *lookupNameByValue(const QCameraMap arr[], int len, int value);

    // ops for batch set/get params with server
    int32_t initBatchUpdate(parm_buffer_t *p_table);
    int32_t AddSetParmEntryToBatch(parm_buffer_t *p_table,
                                   cam_intf_parm_type_t paramType,
                                   uint32_t paramLength,
                                   void *paramValue);
    int32_t commitSetBatch();
    int32_t AddGetParmEntryToBatch(parm_buffer_t *p_table,
                                   cam_intf_parm_type_t paramType);
    int32_t commitGetBatch();

    // ops to tempororily update parameter entries and commit
    int32_t updateParamEntry(const char *key, const char *value);
    int32_t commitParamChanges();

    // Map from strings to values
    static const cam_dimension_t THUMBNAIL_SIZES_MAP[];
    static const QCameraMap AUTO_EXPOSURE_MAP[];
    static const QCameraMap PREVIEW_FORMATS_MAP[];
    static const QCameraMap PICTURE_TYPES_MAP[];
    static const QCameraMap RAW_FORMATS_MAP[];
    static const QCameraMap FOCUS_MODES_MAP[];
    static const QCameraMap EFFECT_MODES_MAP[];
    static const QCameraMap SCENE_MODES_MAP[];
    static const QCameraMap FLASH_MODES_MAP[];
    static const QCameraMap FOCUS_ALGO_MAP[];
    static const QCameraMap WHITE_BALANCE_MODES_MAP[];
    static const QCameraMap ANTIBANDING_MODES_MAP[];
    static const QCameraMap ISO_MODES_MAP[];
    static const QCameraMap HFR_MODES_MAP[];
    static const QCameraMap BRACKETING_MODES_MAP[];
    static const QCameraMap ON_OFF_MODES_MAP[];
    static const QCameraMap ENABLE_DISABLE_MODES_MAP[];
    static const QCameraMap DENOISE_ON_OFF_MODES_MAP[];
    static const QCameraMap TRUE_FALSE_MODES_MAP[];
    static const QCameraMap TOUCH_AF_AEC_MODES_MAP[];
    static const QCameraMap FLIP_MODES_MAP[];

    cam_capability_t *m_pCapability;
    mm_camera_vtbl_t *m_pCamOpsTbl;
    QCameraHeapMemory *m_pParamHeap;
    parm_buffer_t     *m_pParamBuf;  // ptr to param buf in m_pParamHeap

    bool m_bZslMode;                // if ZSL is enabled
    bool m_bZslMode_new;
    bool m_bRecordingHint;          // local copy of recording hint
    bool m_bRecordingHint_new;
    bool m_bHistogramEnabled;       // if histogram is enabled
    int  m_nFaceProcMask;           // face process mask
    bool m_bDebugFps;               // if FPS need to be logged
    cam_focus_mode_type mFocusMode;
    cam_format_t mPreviewFormat;
    int32_t mPictureFormat;         // could be CAMERA_PICTURE_TYPE_JPEG or cam_format_t
    bool m_bNeedRestart;            // if preview needs restart after parameters updated
    bool m_bNoDisplayMode;
    bool m_bWNROn;
    bool m_bNeedLockCAF;
    bool m_bCAFLocked;
    bool m_bAFRunning;
    qcamera_thermal_mode m_ThermalMode; // adjust fps vs adjust frameskip
    cam_dimension_t m_LiveSnapshotSize; // live snapshot size

    DefaultKeyedVector<String8,String8> m_tempMap; // map for temororily store parameters to be set
};

}; // namespace qcamera

#endif
