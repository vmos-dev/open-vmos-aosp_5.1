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
* @file General3A_Settings.h
*
* This file maps the Camera Hardware Interface to OMX.
*
*/

#include "OMX_TI_IVCommon.h"
#include "OMX_TI_Common.h"
#include "OMX_TI_Index.h"
#include "TICameraParameters.h"

#ifndef GENERAL_3A_SETTINGS_H
#define GENERAL_3A_SETTINGS_H

namespace Ti {
namespace Camera {

struct userToOMX_LUT{
    const char * userDefinition;
    int         omxDefinition;
};

struct LUTtype{
    int size;
    const userToOMX_LUT *Table;
};

const userToOMX_LUT isoUserToOMX[] = {
    { TICameraParameters::ISO_MODE_AUTO, 0 },
    { TICameraParameters::ISO_MODE_100, 100 },
    { TICameraParameters::ISO_MODE_200, 200 },
    { TICameraParameters::ISO_MODE_400, 400 },
    { TICameraParameters::ISO_MODE_800, 800 },
    { TICameraParameters::ISO_MODE_1000, 1000 },
    { TICameraParameters::ISO_MODE_1200, 1200 },
    { TICameraParameters::ISO_MODE_1600, 1600 },
};

const userToOMX_LUT effects_UserToOMX [] = {
    { android::CameraParameters::EFFECT_NONE, OMX_ImageFilterNone },
    { android::CameraParameters::EFFECT_NEGATIVE, OMX_ImageFilterNegative },
    { android::CameraParameters::EFFECT_SOLARIZE,  OMX_ImageFilterSolarize },
    { android::CameraParameters::EFFECT_SEPIA, OMX_ImageFilterSepia },
    { android::CameraParameters::EFFECT_MONO, OMX_ImageFilterGrayScale },
    { android::CameraParameters::EFFECT_BLACKBOARD, OMX_TI_ImageFilterBlackBoard },
    { android::CameraParameters::EFFECT_WHITEBOARD, OMX_TI_ImageFilterWhiteBoard },
    { android::CameraParameters::EFFECT_AQUA, OMX_TI_ImageFilterAqua },
    { android::CameraParameters::EFFECT_POSTERIZE, OMX_TI_ImageFilterPosterize },
#ifdef OMAP_ENHANCEMENT
    { TICameraParameters::EFFECT_NATURAL, OMX_ImageFilterNatural },
    { TICameraParameters::EFFECT_VIVID, OMX_ImageFilterVivid },
    { TICameraParameters::EFFECT_COLOR_SWAP, OMX_ImageFilterColourSwap   },
    { TICameraParameters::EFFECT_BLACKWHITE, OMX_TI_ImageFilterBlackWhite }
#endif
};

const userToOMX_LUT scene_UserToOMX [] = {
    { android::CameraParameters::SCENE_MODE_AUTO, OMX_Manual },
    { android::CameraParameters::SCENE_MODE_LANDSCAPE, OMX_Landscape },
    { android::CameraParameters::SCENE_MODE_NIGHT_PORTRAIT, OMX_NightPortrait },
    { android::CameraParameters::SCENE_MODE_FIREWORKS, OMX_Fireworks },
    { android::CameraParameters::SCENE_MODE_ACTION, OMX_TI_Action },
    { android::CameraParameters::SCENE_MODE_BEACH, OMX_TI_Beach },
    { android::CameraParameters::SCENE_MODE_CANDLELIGHT, OMX_TI_Candlelight },
    { android::CameraParameters::SCENE_MODE_NIGHT, OMX_TI_Night },
    { android::CameraParameters::SCENE_MODE_PARTY, OMX_TI_Party },
    { android::CameraParameters::SCENE_MODE_PORTRAIT, OMX_TI_Portrait },
    { android::CameraParameters::SCENE_MODE_SNOW, OMX_TI_Snow },
    { android::CameraParameters::SCENE_MODE_STEADYPHOTO, OMX_TI_Steadyphoto },
    { android::CameraParameters::SCENE_MODE_SUNSET, OMX_TI_Sunset },
    { android::CameraParameters::SCENE_MODE_THEATRE, OMX_TI_Theatre },
    { android::CameraParameters::SCENE_MODE_SPORTS, OMX_Sport },
#ifdef OMAP_ENHANCEMENT
    { TICameraParameters::SCENE_MODE_CLOSEUP, OMX_Closeup },
    { TICameraParameters::SCENE_MODE_AQUA, OMX_Underwater },
    { TICameraParameters::SCENE_MODE_MOOD, OMX_Mood },
    { TICameraParameters::SCENE_MODE_NIGHT_INDOOR, OMX_NightIndoor },
    { TICameraParameters::SCENE_MODE_DOCUMENT, OMX_Document },
    { TICameraParameters::SCENE_MODE_BARCODE, OMX_Barcode },
    { TICameraParameters::SCENE_MODE_VIDEO_SUPER_NIGHT, OMX_SuperNight },
    { TICameraParameters::SCENE_MODE_VIDEO_CINE, OMX_Cine },
    { TICameraParameters::SCENE_MODE_VIDEO_OLD_FILM, OMX_OldFilm },
#endif
};

const userToOMX_LUT whiteBal_UserToOMX [] = {
    { android::CameraParameters::WHITE_BALANCE_AUTO, OMX_WhiteBalControlAuto },
    { android::CameraParameters::WHITE_BALANCE_DAYLIGHT, OMX_WhiteBalControlSunLight },
    { android::CameraParameters::WHITE_BALANCE_CLOUDY_DAYLIGHT, OMX_WhiteBalControlCloudy },
    { android::CameraParameters::WHITE_BALANCE_FLUORESCENT, OMX_WhiteBalControlFluorescent },
    { android::CameraParameters::WHITE_BALANCE_INCANDESCENT, OMX_WhiteBalControlIncandescent },
    { android::CameraParameters::WHITE_BALANCE_SHADE, OMX_TI_WhiteBalControlShade },
    { android::CameraParameters::WHITE_BALANCE_TWILIGHT, OMX_TI_WhiteBalControlTwilight },
    { android::CameraParameters::WHITE_BALANCE_WARM_FLUORESCENT, OMX_TI_WhiteBalControlWarmFluorescent },
#ifdef OMAP_ENHANCEMENT
    { TICameraParameters::WHITE_BALANCE_TUNGSTEN, OMX_WhiteBalControlTungsten },
    { TICameraParameters::WHITE_BALANCE_HORIZON, OMX_WhiteBalControlHorizon },
    { TICameraParameters::WHITE_BALANCE_SUNSET, OMX_TI_WhiteBalControlSunset }
#endif
};

const userToOMX_LUT antibanding_UserToOMX [] = {
    { android::CameraParameters::ANTIBANDING_OFF, OMX_FlickerCancelOff },
    { android::CameraParameters::ANTIBANDING_AUTO, OMX_FlickerCancelAuto },
    { android::CameraParameters::ANTIBANDING_50HZ, OMX_FlickerCancel50 },
    { android::CameraParameters::ANTIBANDING_60HZ, OMX_FlickerCancel60 }
};

const userToOMX_LUT focus_UserToOMX [] = {
    { android::CameraParameters::FOCUS_MODE_AUTO, OMX_IMAGE_FocusControlAutoLock },
    { android::CameraParameters::FOCUS_MODE_INFINITY, OMX_IMAGE_FocusControlAutoInfinity },
    { android::CameraParameters::FOCUS_MODE_INFINITY, OMX_IMAGE_FocusControlHyperfocal },
    { android::CameraParameters::FOCUS_MODE_MACRO, OMX_IMAGE_FocusControlAutoMacro },
    { android::CameraParameters::FOCUS_MODE_CONTINUOUS_VIDEO, OMX_IMAGE_FocusControlAuto },
    { android::CameraParameters::FOCUS_MODE_CONTINUOUS_PICTURE, OMX_IMAGE_FocusControlAuto },
#ifdef OMAP_ENHANCEMENT
    { TICameraParameters::FOCUS_MODE_FACE , OMX_IMAGE_FocusControlContinousFacePriority },
    { TICameraParameters::FOCUS_MODE_PORTRAIT, OMX_IMAGE_FocusControlPortrait },
    { TICameraParameters::FOCUS_MODE_EXTENDED, OMX_IMAGE_FocusControlExtended },
#endif
    { TICameraParameters::FOCUS_MODE_OFF , OMX_IMAGE_FocusControlOff }
};

const userToOMX_LUT exposure_UserToOMX [] = {
    { TICameraParameters::EXPOSURE_MODE_MANUAL, OMX_ExposureControlOff },
    { TICameraParameters::EXPOSURE_MODE_AUTO, OMX_ExposureControlAuto },
    { TICameraParameters::EXPOSURE_MODE_NIGHT, OMX_ExposureControlNight },
    { TICameraParameters::EXPOSURE_MODE_BACKLIGHT, OMX_ExposureControlBackLight },
    { TICameraParameters::EXPOSURE_MODE_SPOTLIGHT, OMX_ExposureControlSpotLight},
    { TICameraParameters::EXPOSURE_MODE_SPORTS, OMX_ExposureControlSports },
    { TICameraParameters::EXPOSURE_MODE_SNOW, OMX_ExposureControlSnow },
    { TICameraParameters::EXPOSURE_MODE_BEACH, OMX_ExposureControlBeach },
    { TICameraParameters::EXPOSURE_MODE_APERTURE, OMX_ExposureControlLargeAperture },
    { TICameraParameters::EXPOSURE_MODE_SMALL_APERTURE, OMX_ExposureControlSmallApperture },
};

const userToOMX_LUT flash_UserToOMX [] = {
    { android::CameraParameters::FLASH_MODE_OFF           ,OMX_IMAGE_FlashControlOff             },
    { android::CameraParameters::FLASH_MODE_ON            ,OMX_IMAGE_FlashControlOn              },
    { android::CameraParameters::FLASH_MODE_AUTO          ,OMX_IMAGE_FlashControlAuto            },
    { android::CameraParameters::FLASH_MODE_TORCH         ,OMX_IMAGE_FlashControlTorch           },
    { android::CameraParameters::FLASH_MODE_RED_EYE        ,OMX_IMAGE_FlashControlRedEyeReduction },
#ifdef OMAP_ENHANCEMENT
    { TICameraParameters::FLASH_MODE_FILL_IN        ,OMX_IMAGE_FlashControlFillin          }
#endif
};

const LUTtype ExpLUT =
    {
    sizeof(exposure_UserToOMX)/sizeof(exposure_UserToOMX[0]),
    exposure_UserToOMX
    };

const LUTtype WBalLUT =
    {
    sizeof(whiteBal_UserToOMX)/sizeof(whiteBal_UserToOMX[0]),
    whiteBal_UserToOMX
    };

const LUTtype FlickerLUT =
    {
    sizeof(antibanding_UserToOMX)/sizeof(antibanding_UserToOMX[0]),
    antibanding_UserToOMX
    };

const LUTtype SceneLUT =
    {
    sizeof(scene_UserToOMX)/sizeof(scene_UserToOMX[0]),
    scene_UserToOMX
    };

const LUTtype FlashLUT =
    {
    sizeof(flash_UserToOMX)/sizeof(flash_UserToOMX[0]),
    flash_UserToOMX
    };

const LUTtype EffLUT =
    {
    sizeof(effects_UserToOMX)/sizeof(effects_UserToOMX[0]),
    effects_UserToOMX
    };

const LUTtype FocusLUT =
    {
    sizeof(focus_UserToOMX)/sizeof(focus_UserToOMX[0]),
    focus_UserToOMX
    };

const LUTtype IsoLUT =
    {
    sizeof(isoUserToOMX)/sizeof(isoUserToOMX[0]),
    isoUserToOMX
    };

/*
*   class Gen3A_settings
*   stores the 3A settings
*   also defines the look up tables
*   for mapping settings from Hal to OMX
*/
class Gen3A_settings{
    public:

    int Exposure;
    int WhiteBallance;
    int Flicker;
    int SceneMode;
    int Effect;
    int Focus;
    int EVCompensation;
    int Contrast;
    int Saturation;
    int Sharpness;
    int ISO;
    int FlashMode;
    int ManualExposure;
    int ManualExposureRight;
    int ManualGain;
    int ManualGainRight;

    unsigned int Brightness;
    OMX_BOOL ExposureLock;
    OMX_BOOL FocusLock;
    OMX_BOOL WhiteBalanceLock;

    OMX_BOOL AlgoExternalGamma;
    OMX_BOOL AlgoNSF1;
    OMX_BOOL AlgoNSF2;
    OMX_BOOL AlgoSharpening;
    OMX_BOOL AlgoThreeLinColorMap;
    OMX_BOOL AlgoGIC;

    OMX_TI_CONFIG_GAMMATABLE_TYPE mGammaTable;

};

/*
*   Flags raised when a setting is changed
*/
enum E3ASettingsFlags
{
    SetSceneMode            = 1 << 0,
    SetEVCompensation       = 1 << 1,
    SetWhiteBallance        = 1 << 2,
    SetFlicker              = 1 << 3,
    SetExposure             = 1 << 4,
    SetSharpness            = 1 << 5,
    SetBrightness           = 1 << 6,
    SetContrast             = 1 << 7,
    SetISO                  = 1 << 8,
    SetSaturation           = 1 << 9,
    SetEffect               = 1 << 10,
    SetFocus                = 1 << 11,
    SetExpMode              = 1 << 14,
    SetFlash                = 1 << 15,
    SetExpLock              = 1 << 16,
    SetWBLock               = 1 << 17,
    SetMeteringAreas        = 1 << 18,
    SetManualExposure       = 1 << 19,

    SetAlgoExternalGamma    = 1 << 20,
    SetAlgoNSF1             = 1 << 21,
    SetAlgoNSF2             = 1 << 22,
    SetAlgoSharpening       = 1 << 23,
    SetAlgoThreeLinColorMap = 1 << 24,
    SetAlgoGIC              = 1 << 25,
    SetGammaTable           = 1 << 26,


    E3aSettingMax,
    E3AsettingsAll = ( ((E3aSettingMax -1 ) << 1) -1 ) /// all possible flags raised
};

} // namespace Camera
} // namespace Ti

#endif //GENERAL_3A_SETTINGS_H
