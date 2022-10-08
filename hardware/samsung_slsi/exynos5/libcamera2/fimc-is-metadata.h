/****************************************************************************
 ****************************************************************************
 ***
 ***   This header was automatically generated from a Linux kernel header
 ***   of the same name, to make information necessary for userspace to
 ***   call into the kernel available to libc.  It contains only constants,
 ***   structures, and macros generated from the original header, and thus,
 ***   contains no copyrightable information.
 ***
 ***   To edit the content of this header, modify the corresponding
 ***   source file (e.g. under external/kernel-headers/original/) then
 ***   run bionic/libc/kernel/tools/update_all.py
 ***
 ***   Any manual change here will be lost the next time this script will
 ***   be run. You've been warned!
 ***
 ****************************************************************************
 ****************************************************************************/
#ifndef FIMC_IS_METADATA_H_
#define FIMC_IS_METADATA_H_
struct rational {
 uint32_t num;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t den;
};
#define CAMERA2_MAX_AVAILABLE_MODE 21
#define CAMERA2_MAX_FACES 16
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define CAMERA2_FACE_DETECTION_THRESHOLD 35
enum metadata_mode {
 METADATA_MODE_NONE,
 METADATA_MODE_FULL
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct camera2_request_ctl {
 uint32_t id;
 enum metadata_mode metadataMode;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint8_t outputStreams[16];
 uint32_t frameCount;
};
struct camera2_request_dm {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t id;
 enum metadata_mode metadataMode;
 uint32_t frameCount;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum optical_stabilization_mode {
 OPTICAL_STABILIZATION_MODE_OFF,
 OPTICAL_STABILIZATION_MODE_ON
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum lens_facing {
 LENS_FACING_BACK,
 LENS_FACING_FRONT
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct camera2_lens_ctl {
 uint32_t focusDistance;
 float aperture;
 float focalLength;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 float filterDensity;
 enum optical_stabilization_mode opticalStabilizationMode;
};
struct camera2_lens_dm {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t focusDistance;
 float aperture;
 float focalLength;
 float filterDensity;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 enum optical_stabilization_mode opticalStabilizationMode;
 float focusRange[2];
};
struct camera2_lens_sm {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 float minimumFocusDistance;
 float hyperfocalDistance;
 float availableFocalLength[2];
 float availableApertures;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 float availableFilterDensities;
 enum optical_stabilization_mode availableOpticalStabilization;
 uint32_t shadingMapSize;
 float shadingMap[3][40][30];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t geometricCorrectionMapSize;
 float geometricCorrectionMap[2][3][40][30];
 enum lens_facing facing;
 float position[2];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
enum sensor_colorfilterarrangement {
 SENSOR_COLORFILTERARRANGEMENT_RGGB,
 SENSOR_COLORFILTERARRANGEMENT_GRBG,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 SENSOR_COLORFILTERARRANGEMENT_GBRG,
 SENSOR_COLORFILTERARRANGEMENT_BGGR,
 SENSOR_COLORFILTERARRANGEMENT_RGB
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum sensor_ref_illuminant {
 SENSOR_ILLUMINANT_DAYLIGHT = 1,
 SENSOR_ILLUMINANT_FLUORESCENT = 2,
 SENSOR_ILLUMINANT_TUNGSTEN = 3,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 SENSOR_ILLUMINANT_FLASH = 4,
 SENSOR_ILLUMINANT_FINE_WEATHER = 9,
 SENSOR_ILLUMINANT_CLOUDY_WEATHER = 10,
 SENSOR_ILLUMINANT_SHADE = 11,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 SENSOR_ILLUMINANT_DAYLIGHT_FLUORESCENT = 12,
 SENSOR_ILLUMINANT_DAY_WHITE_FLUORESCENT = 13,
 SENSOR_ILLUMINANT_COOL_WHITE_FLUORESCENT = 14,
 SENSOR_ILLUMINANT_WHITE_FLUORESCENT = 15,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 SENSOR_ILLUMINANT_STANDARD_A = 17,
 SENSOR_ILLUMINANT_STANDARD_B = 18,
 SENSOR_ILLUMINANT_STANDARD_C = 19,
 SENSOR_ILLUMINANT_D55 = 20,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 SENSOR_ILLUMINANT_D65 = 21,
 SENSOR_ILLUMINANT_D75 = 22,
 SENSOR_ILLUMINANT_D50 = 23,
 SENSOR_ILLUMINANT_ISO_STUDIO_TUNGSTEN = 24
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct camera2_sensor_ctl {
 uint64_t exposureTime;
 uint64_t frameDuration;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t sensitivity;
};
struct camera2_sensor_dm {
 uint64_t exposureTime;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint64_t frameDuration;
 uint32_t sensitivity;
 uint64_t timeStamp;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct camera2_sensor_sm {
 uint32_t exposureTimeRange[2];
 uint32_t maxFrameDuration;
 uint32_t availableSensitivities[10];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 enum sensor_colorfilterarrangement colorFilterArrangement;
 float physicalSize[2];
 uint32_t pixelArraySize[2];
 uint32_t activeArraySize[4];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t whiteLevel;
 uint32_t blackLevelPattern[4];
 struct rational colorTransform1[9];
 struct rational colorTransform2[9];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 enum sensor_ref_illuminant referenceIlluminant1;
 enum sensor_ref_illuminant referenceIlluminant2;
 struct rational forwardMatrix1[9];
 struct rational forwardMatrix2[9];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct rational calibrationTransform1[9];
 struct rational calibrationTransform2[9];
 struct rational baseGainFactor;
 uint32_t maxAnalogSensitivity;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 float noiseModelCoefficients[2];
 uint32_t orientation;
};
enum flash_mode {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 CAM2_FLASH_MODE_NOP = 0,
 CAM2_FLASH_MODE_OFF = 1,
 CAM2_FLASH_MODE_SINGLE,
 CAM2_FLASH_MODE_TORCH,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 CAM2_FLASH_MODE_BEST
};
struct camera2_flash_ctl {
 enum flash_mode flashMode;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t firingPower;
 uint64_t firingTime;
};
struct camera2_flash_dm {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 enum flash_mode flashMode;
 uint32_t firingPower;
 uint64_t firingTime;
 uint32_t firingStable;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t decision;
};
struct camera2_flash_sm {
 uint32_t available;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint64_t chargeDuration;
};
enum processing_mode {
 PROCESSING_MODE_OFF = 1,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 PROCESSING_MODE_FAST,
 PROCESSING_MODE_HIGH_QUALITY
};
struct camera2_hotpixel_ctl {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 enum processing_mode mode;
};
struct camera2_hotpixel_dm {
 enum processing_mode mode;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct camera2_demosaic_ctl {
 enum processing_mode mode;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct camera2_demosaic_dm {
 enum processing_mode mode;
};
struct camera2_noisereduction_ctl {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 enum processing_mode mode;
 uint32_t strength;
};
struct camera2_noisereduction_dm {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 enum processing_mode mode;
 uint32_t strength;
};
struct camera2_shading_ctl {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 enum processing_mode mode;
};
struct camera2_shading_dm {
 enum processing_mode mode;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct camera2_geometric_ctl {
 enum processing_mode mode;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct camera2_geometric_dm {
 enum processing_mode mode;
};
enum colorcorrection_mode {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 COLORCORRECTION_MODE_FAST = 1,
 COLORCORRECTION_MODE_HIGH_QUALITY,
 COLORCORRECTION_MODE_TRANSFORM_MATRIX,
 COLORCORRECTION_MODE_EFFECT_MONO,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 COLORCORRECTION_MODE_EFFECT_NEGATIVE,
 COLORCORRECTION_MODE_EFFECT_SOLARIZE,
 COLORCORRECTION_MODE_EFFECT_SEPIA,
 COLORCORRECTION_MODE_EFFECT_POSTERIZE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 COLORCORRECTION_MODE_EFFECT_WHITEBOARD,
 COLORCORRECTION_MODE_EFFECT_BLACKBOARD,
 COLORCORRECTION_MODE_EFFECT_AQUA
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct camera2_colorcorrection_ctl {
 enum colorcorrection_mode mode;
 float transform[9];
 uint32_t hue;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t saturation;
 uint32_t brightness;
};
struct camera2_colorcorrection_dm {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 enum colorcorrection_mode mode;
 float transform[9];
 uint32_t hue;
 uint32_t saturation;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t brightness;
};
struct camera2_colorcorrection_sm {
 uint8_t availableModes[CAMERA2_MAX_AVAILABLE_MODE];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t hueRange[2];
 uint32_t saturationRange[2];
 uint32_t brightnessRange[2];
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum tonemap_mode {
 TONEMAP_MODE_FAST = 1,
 TONEMAP_MODE_HIGH_QUALITY,
 TONEMAP_MODE_CONTRAST_CURVE
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct camera2_tonemap_ctl {
 enum tonemap_mode mode;
 float curveRed[64];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 float curveGreen[64];
 float curveBlue[64];
};
struct camera2_tonemap_dm {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 enum tonemap_mode mode;
 float curveRed[64];
 float curveGreen[64];
 float curveBlue[64];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct camera2_tonemap_sm {
 uint32_t maxCurvePoints;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct camera2_edge_ctl {
 enum processing_mode mode;
 uint32_t strength;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct camera2_edge_dm {
 enum processing_mode mode;
 uint32_t strength;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum scaler_availableformats {
 SCALER_FORMAT_BAYER_RAW,
 SCALER_FORMAT_YV12,
 SCALER_FORMAT_NV21,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 SCALER_FORMAT_JPEG,
 SCALER_FORMAT_UNKNOWN
};
struct camera2_scaler_ctl {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t cropRegion[3];
};
struct camera2_scaler_dm {
 uint32_t cropRegion[3];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct camera2_scaler_sm {
 enum scaler_availableformats availableFormats[4];
 uint32_t availableRawSizes;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint64_t availableRawMinDurations;
 uint32_t availableProcessedSizes[8];
 uint64_t availableProcessedMinDurations[8];
 uint32_t availableJpegSizes[8][2];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint64_t availableJpegMinDurations[8];
 uint32_t availableMaxDigitalZoom[8];
};
struct camera2_jpeg_ctl {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t quality;
 uint32_t thumbnailSize[2];
 uint32_t thumbnailQuality;
 double gpsCoordinates[3];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t gpsProcessingMethod;
 uint64_t gpsTimestamp;
 uint32_t orientation;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct camera2_jpeg_dm {
 uint32_t quality;
 uint32_t thumbnailSize[2];
 uint32_t thumbnailQuality;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 double gpsCoordinates[3];
 uint32_t gpsProcessingMethod;
 uint64_t gpsTimestamp;
 uint32_t orientation;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct camera2_jpeg_sm {
 uint32_t availableThumbnailSizes[8][2];
 uint32_t maxSize;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
enum facedetect_mode {
 FACEDETECT_MODE_OFF = 1,
 FACEDETECT_MODE_SIMPLE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 FACEDETECT_MODE_FULL
};
enum stats_mode {
 STATS_MODE_OFF = 1,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 STATS_MODE_ON
};
struct camera2_stats_ctl {
 enum facedetect_mode faceDetectMode;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 enum stats_mode histogramMode;
 enum stats_mode sharpnessMapMode;
};
struct camera2_stats_dm {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 enum facedetect_mode faceDetectMode;
 uint32_t faceRectangles[CAMERA2_MAX_FACES][4];
 uint8_t faceScores[CAMERA2_MAX_FACES];
 uint32_t faceLandmarks[CAMERA2_MAX_FACES][6];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t faceIds[CAMERA2_MAX_FACES];
 enum stats_mode histogramMode;
 uint32_t histogram[3 * 256];
 enum stats_mode sharpnessMapMode;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct camera2_stats_sm {
 uint8_t availableFaceDetectModes[CAMERA2_MAX_AVAILABLE_MODE];
 uint32_t maxFaceCount;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t histogramBucketCount;
 uint32_t maxHistogramCount;
 uint32_t sharpnessMapSize[2];
 uint32_t maxSharpnessMapValue;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
enum aa_capture_intent {
 AA_CAPTURE_INTENT_CUSTOM = 0,
 AA_CAPTURE_INTENT_PREVIEW,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 AA_CAPTURE_INTENT_STILL_CAPTURE,
 AA_CAPTURE_INTENT_VIDEO_RECORD,
 AA_CAPTURE_INTENT_VIDEO_SNAPSHOT,
 AA_CAPTURE_INTENT_ZERO_SHUTTER_LAG
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
enum aa_mode {
 AA_CONTROL_NONE = 0,
 AA_CONTROL_OFF,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 AA_CONTROL_AUTO,
 AA_CONTROL_USE_SCENE_MODE
};
enum aa_scene_mode {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 AA_SCENE_MODE_UNSUPPORTED = 1,
 AA_SCENE_MODE_FACE_PRIORITY,
 AA_SCENE_MODE_ACTION,
 AA_SCENE_MODE_PORTRAIT,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 AA_SCENE_MODE_LANDSCAPE,
 AA_SCENE_MODE_NIGHT,
 AA_SCENE_MODE_NIGHT_PORTRAIT,
 AA_SCENE_MODE_THEATRE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 AA_SCENE_MODE_BEACH,
 AA_SCENE_MODE_SNOW,
 AA_SCENE_MODE_SUNSET,
 AA_SCENE_MODE_STEADYPHOTO,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 AA_SCENE_MODE_FIREWORKS,
 AA_SCENE_MODE_SPORTS,
 AA_SCENE_MODE_PARTY,
 AA_SCENE_MODE_CANDLELIGHT,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 AA_SCENE_MODE_BARCODE,
 AA_SCENE_MODE_NIGHT_CAPTURE,
 AA_SCENE_MODE_MAX
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum aa_effect_mode {
 AA_EFFECT_OFF = 1,
 AA_EFFECT_MONO,
 AA_EFFECT_NEGATIVE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 AA_EFFECT_SOLARIZE,
 AA_EFFECT_SEPIA,
 AA_EFFECT_POSTERIZE,
 AA_EFFECT_WHITEBOARD,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 AA_EFFECT_BLACKBOARD,
 AA_EFFECT_AQUA
};
enum aa_aemode {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 AA_AEMODE_OFF = 1,
 AA_AEMODE_LOCKED,
 AA_AEMODE_ON,
 AA_AEMODE_ON_AUTO_FLASH,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 AA_AEMODE_ON_ALWAYS_FLASH,
 AA_AEMODE_ON_AUTO_FLASH_REDEYE
};
enum aa_ae_flashmode {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 AA_FLASHMODE_NOP = 0,
 AA_FLASHMODE_OFF = 1,
 AA_FLASHMODE_ON,
 AA_FLASHMODE_AUTO,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 AA_FLASHMODE_CAPTURE,
 AA_FLASHMODE_ON_ALWAYS
};
enum aa_ae_antibanding_mode {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 AA_AE_ANTIBANDING_OFF = 1,
 AA_AE_ANTIBANDING_50HZ,
 AA_AE_ANTIBANDING_60HZ,
 AA_AE_ANTIBANDING_AUTO
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
enum aa_awbmode {
 AA_AWBMODE_OFF = 1,
 AA_AWBMODE_LOCKED,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 AA_AWBMODE_WB_AUTO,
 AA_AWBMODE_WB_INCANDESCENT,
 AA_AWBMODE_WB_FLUORESCENT,
 AA_AWBMODE_WB_WARM_FLUORESCENT,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 AA_AWBMODE_WB_DAYLIGHT,
 AA_AWBMODE_WB_CLOUDY_DAYLIGHT,
 AA_AWBMODE_WB_TWILIGHT,
 AA_AWBMODE_WB_SHADE
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
enum aa_afmode {
 NO_CHANGE = 0,
 AA_AFMODE_OFF = 1,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 AA_AFMODE_AUTO,
 AA_AFMODE_MACRO,
 AA_AFMODE_CONTINUOUS_VIDEO,
 AA_AFMODE_CONTINUOUS_PICTURE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 AA_AFMODE_INFINITY,
 AA_AFMODE_AUTO_FACE,
 AA_AFMODE_CONTINUOUS_VIDEO_FACE,
 AA_AFMODE_CONTINUOUS_PICTURE_FACE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 AA_AFMODE_MANUAL,
 AA_AFMODE_EDOF
};
enum aa_afstate {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 AA_AFSTATE_INACTIVE = 1,
 AA_AFSTATE_PASSIVE_SCAN,
 AA_AFSTATE_ACTIVE_SCAN,
 AA_AFSTATE_AF_ACQUIRED_FOCUS,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 AA_AFSTATE_AF_FAILED_FOCUS
};
enum ae_state {
 AE_STATE_INACTIVE = 1,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 AE_STATE_SEARCHING,
 AE_STATE_CONVERGED,
 AE_STATE_LOCKED,
 AE_STATE_FLASH_REQUIRED,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 AE_STATE_PRECAPTURE
};
enum awb_state {
 AWB_STATE_INACTIVE = 1,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 AWB_STATE_SEARCHING,
 AWB_STATE_CONVERGED,
 AWB_STATE_LOCKED
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum aa_isomode {
 AA_ISOMODE_AUTO = 1,
 AA_ISOMODE_MANUAL,
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum ae_lockmode {
 AEMODE_LOCK_OFF = 0,
 AEMODE_LOCK_ON,
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum awb_lockmode {
 AWBMODE_LOCK_OFF = 0,
 AWBMODE_LOCK_ON,
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct camera2_aa_ctl {
 enum aa_capture_intent captureIntent;
 enum aa_mode mode;
 enum aa_scene_mode sceneMode;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t videoStabilizationMode;
 enum aa_aemode aeMode;
 uint32_t aeRegions[5];
 int32_t aeExpCompensation;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t aeTargetFpsRange[2];
 enum aa_ae_antibanding_mode aeAntibandingMode;
 enum aa_ae_flashmode aeflashMode;
 enum aa_awbmode awbMode;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t awbRegions[5];
 enum aa_afmode afMode;
 uint32_t afRegions[5];
 uint32_t afTrigger;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 enum aa_isomode isoMode;
 uint32_t isoValue;
};
struct camera2_aa_dm {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 enum aa_mode mode;
 enum aa_effect_mode effectMode;
 enum aa_scene_mode sceneMode;
 uint32_t videoStabilizationMode;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 enum aa_aemode aeMode;
 uint32_t aeRegions[5];
 enum ae_state aeState;
 enum aa_ae_flashmode aeflashMode;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 enum aa_awbmode awbMode;
 uint32_t awbRegions[5];
 enum awb_state awbState;
 enum aa_afmode afMode;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t afRegions[5];
 enum aa_afstate afState;
 enum aa_isomode isoMode;
 uint32_t isoValue;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct camera2_aa_sm {
 uint8_t availableSceneModes[CAMERA2_MAX_AVAILABLE_MODE];
 uint8_t availableEffects[CAMERA2_MAX_AVAILABLE_MODE];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t maxRegions;
 uint8_t aeAvailableModes[CAMERA2_MAX_AVAILABLE_MODE];
 struct rational aeCompensationStep;
 int32_t aeCompensationRange[2];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t aeAvailableTargetFpsRanges[CAMERA2_MAX_AVAILABLE_MODE][2];
 uint8_t aeAvailableAntibandingModes[CAMERA2_MAX_AVAILABLE_MODE];
 uint8_t awbAvailableModes[CAMERA2_MAX_AVAILABLE_MODE];
 uint8_t afAvailableModes[CAMERA2_MAX_AVAILABLE_MODE];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint8_t availableVideoStabilizationModes[4];
 uint32_t isoRange[2];
};
struct camera2_lens_usm {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t focusDistanceFrameDelay;
};
struct camera2_sensor_usm {
 uint32_t exposureTimeFrameDelay;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t frameDurationFrameDelay;
 uint32_t sensitivityFrameDelay;
};
struct camera2_flash_usm {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t flashModeFrameDelay;
 uint32_t firingPowerFrameDelay;
 uint64_t firingTimeFrameDelay;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct camera2_ctl {
 struct camera2_request_ctl request;
 struct camera2_lens_ctl lens;
 struct camera2_sensor_ctl sensor;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct camera2_flash_ctl flash;
 struct camera2_hotpixel_ctl hotpixel;
 struct camera2_demosaic_ctl demosaic;
 struct camera2_noisereduction_ctl noise;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct camera2_shading_ctl shading;
 struct camera2_geometric_ctl geometric;
 struct camera2_colorcorrection_ctl color;
 struct camera2_tonemap_ctl tonemap;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct camera2_edge_ctl edge;
 struct camera2_scaler_ctl scaler;
 struct camera2_jpeg_ctl jpeg;
 struct camera2_stats_ctl stats;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct camera2_aa_ctl aa;
};
struct camera2_dm {
 struct camera2_request_dm request;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct camera2_lens_dm lens;
 struct camera2_sensor_dm sensor;
 struct camera2_flash_dm flash;
 struct camera2_hotpixel_dm hotpixel;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct camera2_demosaic_dm demosaic;
 struct camera2_noisereduction_dm noise;
 struct camera2_shading_dm shading;
 struct camera2_geometric_dm geometric;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct camera2_colorcorrection_dm color;
 struct camera2_tonemap_dm tonemap;
 struct camera2_edge_dm edge;
 struct camera2_scaler_dm scaler;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct camera2_jpeg_dm jpeg;
 struct camera2_stats_dm stats;
 struct camera2_aa_dm aa;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct camera2_sm {
 struct camera2_lens_sm lens;
 struct camera2_sensor_sm sensor;
 struct camera2_flash_sm flash;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct camera2_colorcorrection_sm color;
 struct camera2_tonemap_sm tonemap;
 struct camera2_scaler_sm scaler;
 struct camera2_jpeg_sm jpeg;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct camera2_stats_sm stats;
 struct camera2_aa_sm aa;
 struct camera2_lens_usm lensUd;
 struct camera2_sensor_usm sensorUd;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct camera2_flash_usm flashUd;
};
struct camera2_lens_uctl {
 struct camera2_lens_ctl ctl;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t maxPos;
 uint32_t slewRate;
};
struct camera2_lens_udm {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t maxPos;
 uint32_t slewRate;
};
struct camera2_sensor_uctl {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct camera2_sensor_ctl ctl;
 uint64_t dynamicFrameDuration;
};
struct camera2_scaler_uctl {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t sccTargetAddress[4];
 uint32_t scpTargetAddress[4];
};
struct camera2_flash_uctl {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct camera2_flash_ctl ctl;
};
struct camera2_uctl {
 uint32_t uUpdateBitMap;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t uFrameNumber;
 struct camera2_lens_uctl lensUd;
 struct camera2_sensor_uctl sensorUd;
 struct camera2_flash_uctl flashUd;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct camera2_scaler_uctl scalerUd;
};
struct camera2_udm {
 struct camera2_lens_udm lens;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct camera2_shot {
 struct camera2_ctl ctl;
 struct camera2_dm dm;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct camera2_uctl uctl;
 struct camera2_udm udm;
 uint32_t magicNumber;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct camera2_shot_ext {
 uint32_t setfile;
 uint32_t request_sensor;
 uint32_t request_scc;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t request_scp;
 uint32_t drc_bypass;
 uint32_t dis_bypass;
 uint32_t dnr_bypass;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t fd_bypass;
 uint32_t reserved[20];
 uint32_t timeZone[10][2];
 struct camera2_shot shot;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint8_t gpsProcessingMethod[32];
 uint8_t isReprocessing;
 uint8_t reprocessInput;
 enum ae_lockmode ae_lock;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 enum awb_lockmode awb_lock;
 enum aa_awbmode awb_mode_dm;
};
struct camera2_stream {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t address;
 uint32_t fcount;
 uint32_t rcount;
 uint32_t findex;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#define CAM_LENS_CMD (0x1 << 0x0)
#define CAM_SENSOR_CMD (0x1 << 0x1)
#define CAM_FLASH_CMD (0x1 << 0x2)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#endif

