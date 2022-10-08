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

#include <utils/Log.h>

#include <string.h>
#include <stdlib.h>
#include <TICameraParameters.h>

#define TI_KEY_ALGO_PREFIX "ti-algo-"

namespace Ti {
namespace Camera {

//TI extensions to camera mode
const char TICameraParameters::HIGH_PERFORMANCE_MODE[] = "high-performance";
const char TICameraParameters::HIGH_QUALITY_MODE[] = "high-quality";
const char TICameraParameters::HIGH_QUALITY_ZSL_MODE[] = "high-quality-zsl";
const char TICameraParameters::CP_CAM_MODE[] = "cp-cam";
const char TICameraParameters::VIDEO_MODE[] = "video-mode";
const char TICameraParameters::VIDEO_MODE_HQ[] = "video-mode-hq";
const char TICameraParameters::EXPOSURE_BRACKETING[] = "exposure-bracketing";
const char TICameraParameters::ZOOM_BRACKETING[] = "zoom-bracketing";
const char TICameraParameters::TEMP_BRACKETING[] = "temporal-bracketing";

// TI extensions to standard android Parameters
const char TICameraParameters::KEY_SUPPORTED_CAMERAS[] = "camera-indexes";
const char TICameraParameters::KEY_CAMERA[] = "camera-index";
const char TICameraParameters::KEY_SHUTTER_ENABLE[] = "shutter-enable";
const char TICameraParameters::KEY_CAMERA_NAME[] = "camera-name";
const char TICameraParameters::KEY_BURST[] = "burst-capture";
const char TICameraParameters::KEY_CAP_MODE[] = "mode";
const char TICameraParameters::KEY_CAP_MODE_VALUES[] = "mode-values";
const char TICameraParameters::KEY_VNF[] = "vnf";
const char TICameraParameters::KEY_VNF_SUPPORTED[] = "vnf-supported";
const char TICameraParameters::KEY_SATURATION[] = "saturation";
const char TICameraParameters::KEY_BRIGHTNESS[] = "brightness";
const char TICameraParameters::KEY_SUPPORTED_EXPOSURE[] = "exposure-mode-values";
const char TICameraParameters::KEY_EXPOSURE_MODE[] = "exposure";
const char TICameraParameters::KEY_SUPPORTED_MANUAL_EXPOSURE_MIN[] = "supported-manual-exposure-min";
const char TICameraParameters::KEY_SUPPORTED_MANUAL_EXPOSURE_MAX[] = "supported-manual-exposure-max";
const char TICameraParameters::KEY_SUPPORTED_MANUAL_EXPOSURE_STEP[] = "supported-manual-exposure-step";
const char TICameraParameters::KEY_SUPPORTED_MANUAL_GAIN_ISO_MIN[] = "supported-manual-gain-iso-min";
const char TICameraParameters::KEY_SUPPORTED_MANUAL_GAIN_ISO_MAX[] = "supported-manual-gain-iso-max";
const char TICameraParameters::KEY_SUPPORTED_MANUAL_GAIN_ISO_STEP[] = "supported-manual-gain-iso-step";
const char TICameraParameters::KEY_MANUAL_EXPOSURE[] = "manual-exposure";
const char TICameraParameters::KEY_MANUAL_EXPOSURE_RIGHT[] = "manual-exposure-right";
const char TICameraParameters::KEY_MANUAL_GAIN_ISO[] = "manual-gain-iso";
const char TICameraParameters::KEY_MANUAL_GAIN_ISO_RIGHT[] = "manual-gain-iso-right";
const char TICameraParameters::KEY_CONTRAST[] = "contrast";
const char TICameraParameters::KEY_SHARPNESS[] = "sharpness";
const char TICameraParameters::KEY_ISO[] = "iso";
const char TICameraParameters::KEY_SUPPORTED_ISO_VALUES[] = "iso-mode-values";
const char TICameraParameters::KEY_SUPPORTED_IPP[] = "ipp-values";
const char TICameraParameters::KEY_IPP[] = "ipp";
const char TICameraParameters::KEY_METERING_MODE[] = "meter-mode";
const char TICameraParameters::KEY_EXP_BRACKETING_RANGE[] = "exp-bracketing-range";
const char TICameraParameters::KEY_EXP_GAIN_BRACKETING_RANGE[] = "exp-gain-bracketing-range";
const char TICameraParameters::KEY_ZOOM_BRACKETING_RANGE[] = "zoom-bracketing-range";
const char TICameraParameters::KEY_TEMP_BRACKETING[] = "temporal-bracketing";
const char TICameraParameters::KEY_TEMP_BRACKETING_RANGE_POS[] = "temporal-bracketing-range-positive";
const char TICameraParameters::KEY_TEMP_BRACKETING_RANGE_NEG[] = "temporal-bracketing-range-negative";
const char TICameraParameters::KEY_FLUSH_SHOT_CONFIG_QUEUE[] = "flush-shot-config-queue";
const char TICameraParameters::KEY_MEASUREMENT_ENABLE[] = "measurement";
const char TICameraParameters::KEY_GBCE[] = "gbce";
const char TICameraParameters::KEY_GBCE_SUPPORTED[] = "gbce-supported";
const char TICameraParameters::KEY_GLBCE[] = "glbce";
const char TICameraParameters::KEY_GLBCE_SUPPORTED[] = "glbce-supported";
const char TICameraParameters::KEY_CURRENT_ISO[] = "current-iso";
const char TICameraParameters::KEY_SENSOR_ORIENTATION[] = "sensor-orientation";
const char TICameraParameters::KEY_RECORDING_HINT[] = "internal-recording-hint";
const char TICameraParameters::KEY_AUTO_FOCUS_LOCK[] = "auto-focus-lock";
const char TICameraParameters::KEY_FRAMERATE_RANGES_EXT_SUPPORTED[] = "preview-fps-range-ext-values";
const char TICameraParameters::KEY_FRAMERATES_EXT_SUPPORTED[] = "preview-fps-ext-values";

const char TICameraParameters::RAW_WIDTH[] = "raw-width";
const char TICameraParameters::RAW_HEIGHT[] = "raw-height";

// TI extensions for Stereo Mode
const char TICameraParameters::KEY_S3D_PRV_FRAME_LAYOUT[] = "s3d-prv-frame-layout";
const char TICameraParameters::KEY_S3D_PRV_FRAME_LAYOUT_VALUES[] = "s3d-prv-frame-layout-values";
const char TICameraParameters::KEY_S3D_CAP_FRAME_LAYOUT[] = "s3d-cap-frame-layout";
const char TICameraParameters::KEY_S3D_CAP_FRAME_LAYOUT_VALUES[] = "s3d-cap-frame-layout-values";

//TI extentions fo 3D resolutions
const char TICameraParameters::KEY_SUPPORTED_PICTURE_SUBSAMPLED_SIZES[] = "supported-picture-subsampled-size-values";
const char TICameraParameters::KEY_SUPPORTED_PICTURE_TOPBOTTOM_SIZES[] = "supported-picture-topbottom-size-values";
const char TICameraParameters::KEY_SUPPORTED_PICTURE_SIDEBYSIDE_SIZES[] = "supported-picture-sidebyside-size-values";
const char TICameraParameters::KEY_SUPPORTED_PREVIEW_SUBSAMPLED_SIZES[] = "supported-preview-subsampled-size-values";
const char TICameraParameters::KEY_SUPPORTED_PREVIEW_TOPBOTTOM_SIZES[] = "supported-preview-topbottom-size-values";
const char TICameraParameters::KEY_SUPPORTED_PREVIEW_SIDEBYSIDE_SIZES[] = "supported-preview-sidebyside-size-values";

//TI extensions for SAC/SMC
const char TICameraParameters::KEY_AUTOCONVERGENCE_MODE[] = "auto-convergence-mode";
const char TICameraParameters::KEY_AUTOCONVERGENCE_MODE_VALUES[] = "auto-convergence-mode-values";
const char TICameraParameters::KEY_MANUAL_CONVERGENCE[] = "manual-convergence";
const char TICameraParameters::KEY_SUPPORTED_MANUAL_CONVERGENCE_MIN[] = "supported-manual-convergence-min";
const char TICameraParameters::KEY_SUPPORTED_MANUAL_CONVERGENCE_MAX[] = "supported-manual-convergence-max";
const char TICameraParameters::KEY_SUPPORTED_MANUAL_CONVERGENCE_STEP[] = "supported-manual-convergence-step";

//TI extensions for setting EXIF tags
const char TICameraParameters::KEY_EXIF_MODEL[] = "exif-model";
const char TICameraParameters::KEY_EXIF_MAKE[] = "exif-make";

//TI extensions for additiona GPS data
const char TICameraParameters::KEY_GPS_MAPDATUM[] = "gps-mapdatum";
const char TICameraParameters::KEY_GPS_VERSION[] = "gps-version";
const char TICameraParameters::KEY_GPS_DATESTAMP[] = "gps-datestamp";

// TI extensions for slice mode implementation for VTC
const char TICameraParameters::KEY_VTC_HINT[] = "internal-vtc-hint";
const char TICameraParameters::KEY_VIDEO_ENCODER_HANDLE[] = "encoder_handle";
const char TICameraParameters::KEY_VIDEO_ENCODER_SLICE_HEIGHT[] = "encoder_slice_height";

//TI extensions to Image post-processing
const char TICameraParameters::IPP_LDCNSF[] = "ldc-nsf";
const char TICameraParameters::IPP_LDC[] = "ldc";
const char TICameraParameters::IPP_NSF[] = "nsf";
const char TICameraParameters::IPP_NONE[] = "off";

// TI extensions to standard android pixel formats
const char TICameraParameters::PIXEL_FORMAT_UNUSED[] = "unused";
const char TICameraParameters::PIXEL_FORMAT_JPS[] = "jps";
const char TICameraParameters::PIXEL_FORMAT_MPO[] = "mpo";
const char TICameraParameters::PIXEL_FORMAT_YUV422I_UYVY[] = "yuv422i-uyvy";

// TI extensions to standard android scene mode settings
const char TICameraParameters::SCENE_MODE_CLOSEUP[] = "closeup";
const char TICameraParameters::SCENE_MODE_AQUA[] = "aqua";
const char TICameraParameters::SCENE_MODE_SNOWBEACH[] = "snow-beach";
const char TICameraParameters::SCENE_MODE_MOOD[] = "mood";
const char TICameraParameters::SCENE_MODE_NIGHT_INDOOR[] = "night-indoor";
const char TICameraParameters::SCENE_MODE_DOCUMENT[] = "document";
const char TICameraParameters::SCENE_MODE_BARCODE[] = "barcode";
const char TICameraParameters::SCENE_MODE_VIDEO_SUPER_NIGHT[] = "super-night";
const char TICameraParameters::SCENE_MODE_VIDEO_CINE[] = "cine";
const char TICameraParameters::SCENE_MODE_VIDEO_OLD_FILM[] = "old-film";

// TI extensions to standard android white balance values.
const char TICameraParameters::WHITE_BALANCE_TUNGSTEN[] = "tungsten";
const char TICameraParameters::WHITE_BALANCE_HORIZON[] = "horizon";
const char TICameraParameters::WHITE_BALANCE_SUNSET[] = "sunset";
const char TICameraParameters::WHITE_BALANCE_FACE[] = "face-priority";

// TI extensions to  standard android focus modes.
const char TICameraParameters::FOCUS_MODE_PORTRAIT[] = "portrait";
const char TICameraParameters::FOCUS_MODE_EXTENDED[] = "extended";
const char TICameraParameters::FOCUS_MODE_FACE[] = "face-priority";
const char TICameraParameters::FOCUS_MODE_OFF[] = "off";

//  TI extensions to add  values for effect settings.
const char TICameraParameters::EFFECT_NATURAL[] = "natural";
const char TICameraParameters::EFFECT_VIVID[] = "vivid";
const char TICameraParameters::EFFECT_COLOR_SWAP[] = "color-swap";
const char TICameraParameters::EFFECT_BLACKWHITE[] = "blackwhite";

// TI extensions to add exposure preset modes
const char TICameraParameters::EXPOSURE_MODE_MANUAL[] = "manual";
const char TICameraParameters::EXPOSURE_MODE_AUTO[] = "auto";
const char TICameraParameters::EXPOSURE_MODE_NIGHT[] = "night";
const char TICameraParameters::EXPOSURE_MODE_BACKLIGHT[] = "backlighting";
const char TICameraParameters::EXPOSURE_MODE_SPOTLIGHT[] = "spotlight";
const char TICameraParameters::EXPOSURE_MODE_SPORTS[] = "sports";
const char TICameraParameters::EXPOSURE_MODE_SNOW[] = "snow";
const char TICameraParameters::EXPOSURE_MODE_BEACH[] = "beach";
const char TICameraParameters::EXPOSURE_MODE_APERTURE[] = "aperture";
const char TICameraParameters::EXPOSURE_MODE_SMALL_APERTURE[] = "small-aperture";
const char TICameraParameters::EXPOSURE_MODE_FACE[] = "face-priority";

// TI extensions to add iso values
const char TICameraParameters::ISO_MODE_AUTO[] = "auto";
const char TICameraParameters::ISO_MODE_100[] = "100";
const char TICameraParameters::ISO_MODE_200[] = "200";
const char TICameraParameters::ISO_MODE_400[] = "400";
const char TICameraParameters::ISO_MODE_800[] = "800";
const char TICameraParameters::ISO_MODE_1000[] = "1000";
const char TICameraParameters::ISO_MODE_1200[] = "1200";
const char TICameraParameters::ISO_MODE_1600[] = "1600";

//TI extensions for stereo frame layouts
const char TICameraParameters::S3D_NONE[] = "none";
const char TICameraParameters::S3D_TB_FULL[] = "tb-full";
const char TICameraParameters::S3D_SS_FULL[] = "ss-full";
const char TICameraParameters::S3D_TB_SUBSAMPLED[] = "tb-subsampled";
const char TICameraParameters::S3D_SS_SUBSAMPLED[] = "ss-subsampled";

//  TI extensions to add auto convergence values
const char TICameraParameters::AUTOCONVERGENCE_MODE_DISABLE[] = "disable";
const char TICameraParameters::AUTOCONVERGENCE_MODE_FRAME[] = "frame";
const char TICameraParameters::AUTOCONVERGENCE_MODE_CENTER[] = "center";
const char TICameraParameters::AUTOCONVERGENCE_MODE_TOUCH[] = "touch";
const char TICameraParameters::AUTOCONVERGENCE_MODE_MANUAL[] = "manual";

//TI values for camera direction
const char TICameraParameters::FACING_FRONT[]="front";
const char TICameraParameters::FACING_BACK[]="back";

//TI extensions to flash settings
const char TICameraParameters::FLASH_MODE_FILL_IN[] = "fill-in";

//TI extensions to add sensor orientation parameters
const char TICameraParameters::ORIENTATION_SENSOR_NONE[] = "0";
const char TICameraParameters::ORIENTATION_SENSOR_90[] = "90";
const char TICameraParameters::ORIENTATION_SENSOR_180[] = "180";
const char TICameraParameters::ORIENTATION_SENSOR_270[] = "270";

const char TICameraParameters::KEY_MECHANICAL_MISALIGNMENT_CORRECTION_SUPPORTED[] = "mechanical-misalignment-correction-supported";
const char TICameraParameters::KEY_MECHANICAL_MISALIGNMENT_CORRECTION[] = "mechanical-misalignment-correction";

//TI extensions for enable/disable algos
const char TICameraParameters::KEY_ALGO_EXTERNAL_GAMMA[] = TI_KEY_ALGO_PREFIX "external-gamma";
const char TICameraParameters::KEY_ALGO_NSF1[] = TI_KEY_ALGO_PREFIX "nsf1";
const char TICameraParameters::KEY_ALGO_NSF2[] = TI_KEY_ALGO_PREFIX "nsf2";
const char TICameraParameters::KEY_ALGO_SHARPENING[] = TI_KEY_ALGO_PREFIX "sharpening";
const char TICameraParameters::KEY_ALGO_THREELINCOLORMAP[] = TI_KEY_ALGO_PREFIX "threelinecolormap";
const char TICameraParameters::KEY_ALGO_GIC[] = TI_KEY_ALGO_PREFIX "gic";

const char TICameraParameters::KEY_GAMMA_TABLE[] = "gamma-table";

} // namespace Camera
} // namespace Ti
