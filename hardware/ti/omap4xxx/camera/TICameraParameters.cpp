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




#define LOG_TAG "CameraHAL"
#include <utils/Log.h>

#include <string.h>
#include <stdlib.h>
#include <TICameraParameters.h>
#include "CameraHal.h"

namespace android {

//TI extensions to camera mode
const char TICameraParameters::HIGH_PERFORMANCE_MODE[] = "high-performance";
const char TICameraParameters::HIGH_QUALITY_MODE[] = "high-quality";
const char TICameraParameters::HIGH_QUALITY_ZSL_MODE[] = "high-quality-zsl";
const char TICameraParameters::VIDEO_MODE[] = "video-mode";

// TI extensions to standard android Parameters
const char TICameraParameters::KEY_SUPPORTED_CAMERAS[] = "camera-indexes";
const char TICameraParameters::KEY_CAMERA[] = "camera-index";
const char TICameraParameters::KEY_SHUTTER_ENABLE[] = "shutter-enable";
const char TICameraParameters::KEY_CAMERA_NAME[] = "camera-name";
const char TICameraParameters::KEY_BURST[] = "burst-capture";
const char TICameraParameters::KEY_CAP_MODE[] = "mode";
const char TICameraParameters::KEY_VNF[] = "vnf";
const char TICameraParameters::KEY_SATURATION[] = "saturation";
const char TICameraParameters::KEY_BRIGHTNESS[] = "brightness";
const char TICameraParameters::KEY_EXPOSURE_MODE[] = "exposure";
const char TICameraParameters::KEY_SUPPORTED_EXPOSURE[] = "exposure-mode-values";
const char TICameraParameters::KEY_CONTRAST[] = "contrast";
const char TICameraParameters::KEY_SHARPNESS[] = "sharpness";
const char TICameraParameters::KEY_ISO[] = "iso";
const char TICameraParameters::KEY_SUPPORTED_ISO_VALUES[] = "iso-mode-values";
const char TICameraParameters::KEY_SUPPORTED_IPP[] = "ipp-values";
const char TICameraParameters::KEY_IPP[] = "ipp";
const char TICameraParameters::KEY_MAN_EXPOSURE[] = "manual-exposure";
const char TICameraParameters::KEY_METERING_MODE[] = "meter-mode";
const char TICameraParameters::KEY_PADDED_WIDTH[] = "padded-width";
const char TICameraParameters::KEY_PADDED_HEIGHT[] = "padded-height";
const char TICameraParameters::KEY_EXP_BRACKETING_RANGE[] = "exp-bracketing-range";
const char TICameraParameters::KEY_TEMP_BRACKETING[] = "temporal-bracketing";
const char TICameraParameters::KEY_TEMP_BRACKETING_RANGE_POS[] = "temporal-bracketing-range-positive";
const char TICameraParameters::KEY_TEMP_BRACKETING_RANGE_NEG[] = "temporal-bracketing-range-negative";
const char TICameraParameters::KEY_S3D_SUPPORTED[] = "s3d-supported";
const char TICameraParameters::KEY_MEASUREMENT_ENABLE[] = "measurement";
const char TICameraParameters::KEY_GBCE[] = "gbce";
const char TICameraParameters::KEY_GLBCE[] = "glbce";
const char TICameraParameters::KEY_CURRENT_ISO[] = "current-iso";
const char TICameraParameters::KEY_SENSOR_ORIENTATION[] = "sensor-orientation";
const char TICameraParameters::KEY_SENSOR_ORIENTATION_VALUES[] = "sensor-orientation-values";
const char TICameraParameters::KEY_MINFRAMERATE[] = "min-framerate";
const char TICameraParameters::KEY_MAXFRAMERATE[] = "max-framerate";
const char TICameraParameters::KEY_RECORDING_HINT[] = "internal-recording-hint";
const char TICameraParameters::KEY_AUTO_FOCUS_LOCK[] = "auto-focus-lock";

//TI extensions for enabling/disabling GLBCE
const char TICameraParameters::GLBCE_ENABLE[] = "enable";
const char TICameraParameters::GLBCE_DISABLE[] = "disable";

//TI extensions for enabling/disabling GBCE
const char TICameraParameters::GBCE_ENABLE[] = "enable";
const char TICameraParameters::GBCE_DISABLE[] = "disable";

//TI extensions for enabling/disabling measurement
const char TICameraParameters::MEASUREMENT_ENABLE[] = "enable";
const char TICameraParameters::MEASUREMENT_DISABLE[] = "disable";

//TI extensions for zoom
const char TICameraParameters::ZOOM_SUPPORTED[] = "true";
const char TICameraParameters::ZOOM_UNSUPPORTED[] = "false";

// TI extensions for 2D Preview in Stereo Mode
const char TICameraParameters::KEY_S3D2D_PREVIEW[] = "s3d2d-preview";
const char TICameraParameters::KEY_S3D2D_PREVIEW_MODE[] = "s3d2d-preview-values";

//TI extensions for SAC/SMC
const char TICameraParameters::KEY_AUTOCONVERGENCE[] = "auto-convergence";
const char TICameraParameters::KEY_AUTOCONVERGENCE_MODE[] = "auto-convergence-mode";
const char TICameraParameters::KEY_MANUALCONVERGENCE_VALUES[] = "manual-convergence-values";

//TI extensions for setting EXIF tags
const char TICameraParameters::KEY_EXIF_MODEL[] = "exif-model";
const char TICameraParameters::KEY_EXIF_MAKE[] = "exif-make";

//TI extensions for additiona GPS data
const char TICameraParameters::KEY_GPS_MAPDATUM[] = "gps-mapdatum";
const char TICameraParameters::KEY_GPS_VERSION[] = "gps-version";
const char TICameraParameters::KEY_GPS_DATESTAMP[] = "gps-datestamp";

//TI extensions for enabling/disabling shutter sound
const char TICameraParameters::SHUTTER_ENABLE[] = "true";
const char TICameraParameters::SHUTTER_DISABLE[] = "false";

//TI extensions for Temporal Bracketing
const char TICameraParameters::BRACKET_ENABLE[] = "enable";
const char TICameraParameters::BRACKET_DISABLE[] = "disable";

//TI extensions to Image post-processing
const char TICameraParameters::IPP_LDCNSF[] = "ldc-nsf";
const char TICameraParameters::IPP_LDC[] = "ldc";
const char TICameraParameters::IPP_NSF[] = "nsf";
const char TICameraParameters::IPP_NONE[] = "off";

// TI extensions to standard android pixel formats
const char TICameraParameters::PIXEL_FORMAT_RAW[] = "raw";
const char TICameraParameters::PIXEL_FORMAT_JPS[] = "jps";
const char TICameraParameters::PIXEL_FORMAT_MPO[] = "mpo";
const char TICameraParameters::PIXEL_FORMAT_RAW_JPEG[] = "raw+jpeg";
const char TICameraParameters::PIXEL_FORMAT_RAW_MPO[] = "raw+mpo";

// TI extensions to standard android scene mode settings
const char TICameraParameters::SCENE_MODE_SPORT[] = "sport";
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

//  TI extensions to add  values for effect settings.
const char TICameraParameters::EFFECT_NATURAL[] = "natural";
const char TICameraParameters::EFFECT_VIVID[] = "vivid";
const char TICameraParameters::EFFECT_COLOR_SWAP[] = "color-swap";
const char TICameraParameters::EFFECT_BLACKWHITE[] = "blackwhite";

// TI extensions to add exposure preset modes
const char TICameraParameters::EXPOSURE_MODE_OFF[] = "off";
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

//  TI extensions to add auto convergence values
const char TICameraParameters::AUTOCONVERGENCE_MODE_DISABLE[] = "mode-disable";
const char TICameraParameters::AUTOCONVERGENCE_MODE_FRAME[] = "mode-frame";
const char TICameraParameters::AUTOCONVERGENCE_MODE_CENTER[] = "mode-center";
const char TICameraParameters::AUTOCONVERGENCE_MODE_FFT[] = "mode-fft";
const char TICameraParameters::AUTOCONVERGENCE_MODE_MANUAL[] = "mode-manual";

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
};

