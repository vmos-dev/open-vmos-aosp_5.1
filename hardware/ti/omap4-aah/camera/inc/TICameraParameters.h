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

#ifndef TI_CAMERA_PARAMETERS_H
#define TI_CAMERA_PARAMETERS_H

#include <utils/KeyedVector.h>
#include <utils/String8.h>

namespace Ti {
namespace Camera {

///TI Specific Camera Parameters
class TICameraParameters
{
public:

// Supported Camera indexes
// Example value: "0,1,2,3", where 0-primary, 1-secondary1, 2-secondary2, 3-sterocamera
static const  char KEY_SUPPORTED_CAMERAS[];
// Select logical Camera index
static const char KEY_CAMERA[];
static const char KEY_CAMERA_NAME[];
static const char  KEY_BURST[];
static const  char KEY_CAP_MODE[];
static const  char KEY_CAP_MODE_VALUES[];
static const  char KEY_VNF[];
static const  char KEY_VNF_SUPPORTED[];
static const  char KEY_SATURATION[];
static const  char KEY_BRIGHTNESS[];
static const  char KEY_SUPPORTED_EXPOSURE[];
static const  char KEY_EXPOSURE_MODE[];
static const  char KEY_SUPPORTED_MANUAL_EXPOSURE_MIN[];
static const  char KEY_SUPPORTED_MANUAL_EXPOSURE_MAX[];
static const  char KEY_SUPPORTED_MANUAL_EXPOSURE_STEP[];
static const  char KEY_SUPPORTED_MANUAL_GAIN_ISO_MIN[];
static const  char KEY_SUPPORTED_MANUAL_GAIN_ISO_MAX[];
static const  char KEY_SUPPORTED_MANUAL_GAIN_ISO_STEP[];
static const  char KEY_MANUAL_EXPOSURE[];
static const  char KEY_MANUAL_EXPOSURE_RIGHT[];
static const  char KEY_MANUAL_GAIN_ISO[];
static const  char KEY_MANUAL_GAIN_ISO_RIGHT[];
static const  char KEY_CONTRAST[];
static const  char KEY_SHARPNESS[];
static const  char KEY_ISO[];
static const  char KEY_SUPPORTED_ISO_VALUES[];
static const  char KEY_SUPPORTED_IPP[];
static const  char KEY_IPP[];
static const  char KEY_METERING_MODE[];
static const char  KEY_EXP_BRACKETING_RANGE[];
static const char  KEY_EXP_GAIN_BRACKETING_RANGE[];
static const char  KEY_ZOOM_BRACKETING_RANGE[];
static const char  KEY_TEMP_BRACKETING[];
static const char  KEY_TEMP_BRACKETING_RANGE_POS[];
static const char  KEY_TEMP_BRACKETING_RANGE_NEG[];
static const char  KEY_FLUSH_SHOT_CONFIG_QUEUE[];
static const char  KEY_SHUTTER_ENABLE[];
static const char  KEY_MEASUREMENT_ENABLE[];
static const char  KEY_INITIAL_VALUES[];
static const char  KEY_GBCE[];
static const char  KEY_GBCE_SUPPORTED[];
static const char  KEY_GLBCE[];
static const char  KEY_GLBCE_SUPPORTED[];
static const char  KEY_FRAMERATE_RANGES_EXT_SUPPORTED[];
static const char  KEY_FRAMERATES_EXT_SUPPORTED[];

// TI recording hint to notify camera adapters of possible recording
static const char  KEY_RECORDING_HINT[];
static const char  KEY_AUTO_FOCUS_LOCK[];
static const char  KEY_CURRENT_ISO[];

static const char KEY_SENSOR_ORIENTATION[];

//TI extensions for camera capabilies
static const char INITIAL_VALUES_TRUE[];
static const char INITIAL_VALUES_FALSE[];

//  TI extensions to add values for ManualConvergence and AutoConvergence mode
static const char KEY_AUTOCONVERGENCE_MODE[];
static const char KEY_AUTOCONVERGENCE_MODE_VALUES[];
static const char KEY_MANUAL_CONVERGENCE[];
static const char KEY_SUPPORTED_MANUAL_CONVERGENCE_MIN[];
static const char KEY_SUPPORTED_MANUAL_CONVERGENCE_MAX[];
static const char KEY_SUPPORTED_MANUAL_CONVERGENCE_STEP[];

// TI extensions to add Min frame rate Values
static const char VIDEO_MINFRAMERATE_5[];
static const char VIDEO_MINFRAMERATE_10[];
static const char VIDEO_MINFRAMERATE_15[];
static const char VIDEO_MINFRAMERATE_20[];
static const char VIDEO_MINFRAMERATE_24[];
static const char VIDEO_MINFRAMERATE_25[];
static const char VIDEO_MINFRAMERATE_30[];
static const char VIDEO_MINFRAMERATE_33[];

//TI extensions for setting EXIF tags
static const char KEY_EXIF_MODEL[];
static const char KEY_EXIF_MAKE[];

//TI extensions for additional GPS data
static const char  KEY_GPS_MAPDATUM[];
static const char  KEY_GPS_VERSION[];
static const char  KEY_GPS_DATESTAMP[];

// TI extensions for VTC
static const char KEY_VTC_HINT[];
static const char KEY_VIDEO_ENCODER_HANDLE[];
static const char KEY_VIDEO_ENCODER_SLICE_HEIGHT[];

static const char  RAW_WIDTH[];
static const char  RAW_HEIGHT[];

//TI extensions to Image post-processing
static const char IPP_LDCNSF[];
static const char IPP_LDC[];
static const char IPP_NSF[];
static const char IPP_NONE[];

//TI extensions to camera mode
static const char HIGH_PERFORMANCE_MODE[];
static const char HIGH_QUALITY_MODE[];
static const char HIGH_QUALITY_ZSL_MODE[];
static const char CP_CAM_MODE[];
static const char VIDEO_MODE[];
static const char VIDEO_MODE_HQ[];
static const char EXPOSURE_BRACKETING[];
static const char ZOOM_BRACKETING[];
static const char TEMP_BRACKETING[];

// TI extensions to standard android pixel formats
static const char PIXEL_FORMAT_UNUSED[];
static const char PIXEL_FORMAT_JPS[];
static const char PIXEL_FORMAT_MPO[];
static const char PIXEL_FORMAT_YUV422I_UYVY[];

// TI extensions to standard android scene mode settings
static const  char SCENE_MODE_CLOSEUP[];
static const  char SCENE_MODE_AQUA[];
static const  char SCENE_MODE_SNOWBEACH[];
static const  char SCENE_MODE_MOOD[];
static const  char SCENE_MODE_NIGHT_INDOOR[];
static const  char SCENE_MODE_DOCUMENT[];
static const  char SCENE_MODE_BARCODE[];
static const  char SCENE_MODE_VIDEO_SUPER_NIGHT[];
static const  char SCENE_MODE_VIDEO_CINE[];
static const  char SCENE_MODE_VIDEO_OLD_FILM[];

// TI extensions to standard android white balance settings.
static const  char WHITE_BALANCE_TUNGSTEN[];
static const  char WHITE_BALANCE_HORIZON[];
static const  char WHITE_BALANCE_SUNSET[];
static const  char WHITE_BALANCE_FACE[];

// TI extensions to add exposure preset modes to android api
static const  char EXPOSURE_MODE_MANUAL[];
static const  char EXPOSURE_MODE_AUTO[];
static const  char EXPOSURE_MODE_NIGHT[];
static const  char EXPOSURE_MODE_BACKLIGHT[];
static const  char EXPOSURE_MODE_SPOTLIGHT[];
static const  char EXPOSURE_MODE_SPORTS[];
static const  char EXPOSURE_MODE_SNOW[];
static const  char EXPOSURE_MODE_BEACH[];
static const  char EXPOSURE_MODE_APERTURE[];
static const  char EXPOSURE_MODE_SMALL_APERTURE[];
static const  char EXPOSURE_MODE_FACE[];

// TI extensions to standard android focus presets.
static const  char FOCUS_MODE_PORTRAIT[];
static const  char FOCUS_MODE_EXTENDED[];
static const char  FOCUS_MODE_FACE[];
static const char  FOCUS_MODE_OFF[];

// TI extensions to add iso values
static const char ISO_MODE_AUTO[];
static const char ISO_MODE_100[];
static const char ISO_MODE_200[];
static const char ISO_MODE_400[];
static const char ISO_MODE_800[];
static const char ISO_MODE_1000[];
static const char ISO_MODE_1200[];
static const char ISO_MODE_1600[];

//  TI extensions to add  values for effect settings.
static const char EFFECT_NATURAL[];
static const char EFFECT_VIVID[];
static const char EFFECT_COLOR_SWAP[];
static const char EFFECT_BLACKWHITE[];

//TI extensions for stereo frame layouts
static const char KEY_S3D_PRV_FRAME_LAYOUT[];
static const char KEY_S3D_PRV_FRAME_LAYOUT_VALUES[];
static const char KEY_S3D_CAP_FRAME_LAYOUT[];
static const char KEY_S3D_CAP_FRAME_LAYOUT_VALUES[];

//TI extensions for stereo frame layouts
static const char S3D_NONE[];
static const char S3D_TB_FULL[];
static const char S3D_SS_FULL[];
static const char S3D_TB_SUBSAMPLED[];
static const char S3D_SS_SUBSAMPLED[];

//TI extentions fo 3D resolutions
static const char KEY_SUPPORTED_PICTURE_SUBSAMPLED_SIZES[];
static const char KEY_SUPPORTED_PICTURE_TOPBOTTOM_SIZES[];
static const char KEY_SUPPORTED_PICTURE_SIDEBYSIDE_SIZES[];
static const char KEY_SUPPORTED_PREVIEW_SUBSAMPLED_SIZES[];
static const char KEY_SUPPORTED_PREVIEW_TOPBOTTOM_SIZES[];
static const char KEY_SUPPORTED_PREVIEW_SIDEBYSIDE_SIZES[];

//  TI extensions to add values for AutoConvergence settings.
static const char AUTOCONVERGENCE_MODE_DISABLE[];
static const char AUTOCONVERGENCE_MODE_FRAME[];
static const char AUTOCONVERGENCE_MODE_CENTER[];
static const char AUTOCONVERGENCE_MODE_TOUCH[];
static const char AUTOCONVERGENCE_MODE_MANUAL[];

//TI extensions for flash mode settings
static const char FLASH_MODE_FILL_IN[];

//TI extensions to add sensor orientation parameters
static const char ORIENTATION_SENSOR_NONE[];
static const char ORIENTATION_SENSOR_90[];
static const char ORIENTATION_SENSOR_180[];
static const char ORIENTATION_SENSOR_270[];


//TI values for camera direction
static const char FACING_FRONT[];
static const char FACING_BACK[];

static const char KEY_MECHANICAL_MISALIGNMENT_CORRECTION_SUPPORTED[];
static const char KEY_MECHANICAL_MISALIGNMENT_CORRECTION[];

//TI extensions for enable/disable algos
static const char KEY_ALGO_EXTERNAL_GAMMA[];
static const char KEY_ALGO_NSF1[];
static const char KEY_ALGO_NSF2[];
static const char KEY_ALGO_SHARPENING[];
static const char KEY_ALGO_THREELINCOLORMAP[];
static const char KEY_ALGO_GIC[];

//Gamma table
static const char KEY_GAMMA_TABLE[];

};

} // namespace Camera
} // namespace Ti

#endif
