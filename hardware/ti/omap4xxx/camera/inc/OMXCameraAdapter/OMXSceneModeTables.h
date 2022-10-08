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
* @file OMXSceneModeTables.h
*
* This holds scene mode settings for different omx cameras.
*
*/

#include "OMX_TI_IVCommon.h"
#include "OMX_TI_Common.h"
#include "OMX_TI_Index.h"

#ifndef OMXCAMERAADAPTER_SCENEMODES_H
#define OMXCAMERAADAPTER_SCENEMODES_H

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof((array)) / sizeof((array)[0]))
#endif

struct SceneModesEntry {
    OMX_SCENEMODETYPE scene;
    OMX_IMAGE_FLASHCONTROLTYPE flash;
    int focus;
    OMX_WHITEBALCONTROLTYPE wb;
};

struct CameraToSensorModesLUTEntry {
    const char* name;
    const SceneModesEntry* Table;
    const unsigned int size;
};

static const SceneModesEntry S5K4E1GA_SceneModesLUT [] = {
    { OMX_Closeup,
      OMX_IMAGE_FlashControlAuto,
      OMX_IMAGE_FocusControlAutoMacro,
      OMX_WhiteBalControlAuto },
    { OMX_Landscape,
      OMX_IMAGE_FlashControlOff,
      OMX_IMAGE_FocusControlHyperfocal,
      OMX_WhiteBalControlAuto },
    { OMX_Underwater,
      OMX_IMAGE_FlashControlAuto,
      OMX_IMAGE_FocusControlAutoLock,
      OMX_WhiteBalControlSunLight },
    { OMX_Sport,
      OMX_IMAGE_FlashControlOff,
      OMX_IMAGE_FocusControlHyperfocal,
      OMX_WhiteBalControlAuto },
    { OMX_Mood,
       OMX_IMAGE_FlashControlAuto,
      OMX_IMAGE_FocusControlAutoLock,
      OMX_WhiteBalControlAuto },
    { OMX_NightPortrait,
      OMX_IMAGE_FlashControlAuto,
      OMX_IMAGE_FocusControlPortrait,
      OMX_WhiteBalControlAuto },
    { OMX_NightIndoor,
      OMX_IMAGE_FlashControlAuto,
      OMX_IMAGE_FocusControlHyperfocal,
      OMX_WhiteBalControlAuto },
    { OMX_Fireworks,
      OMX_IMAGE_FlashControlOn,
      OMX_IMAGE_FocusControlHyperfocal,
      OMX_WhiteBalControlAuto },
    { OMX_Document,
      OMX_IMAGE_FlashControlOff,
      OMX_IMAGE_FocusControlAutoMacro,
      OMX_WhiteBalControlAuto },
    { OMX_Barcode,
      OMX_IMAGE_FlashControlAuto,
      OMX_IMAGE_FocusControlAutoMacro,
      OMX_WhiteBalControlAuto },
    { OMX_SuperNight,
      OMX_IMAGE_FlashControlAuto,
      OMX_IMAGE_FocusControlHyperfocal,
      OMX_WhiteBalControlAuto },
    { OMX_Cine,
      OMX_IMAGE_FlashControlAuto,
      OMX_IMAGE_FocusControlHyperfocal,
      OMX_WhiteBalControlAuto },
    { OMX_OldFilm,
      OMX_IMAGE_FlashControlAuto,
      OMX_IMAGE_FocusControlHyperfocal,
      OMX_WhiteBalControlAuto },
    { OMX_TI_Action,
      OMX_IMAGE_FlashControlOff,
      OMX_IMAGE_FocusControlAuto,
      OMX_WhiteBalControlAuto },
    { OMX_TI_Beach,
      OMX_IMAGE_FlashControlOff,
      OMX_IMAGE_FocusControlAutoLock,
      OMX_WhiteBalControlAuto },
    { OMX_TI_Candlelight,
      OMX_IMAGE_FlashControlAuto,
      OMX_IMAGE_FocusControlHyperfocal,
      OMX_WhiteBalControlIncandescent },
    { OMX_TI_Night,
      OMX_IMAGE_FlashControlOff,
      OMX_IMAGE_FocusControlAuto,
      OMX_WhiteBalControlAuto },
    { OMX_TI_Party,
      OMX_IMAGE_FlashControlAuto,
      OMX_IMAGE_FocusControlAuto,
      OMX_WhiteBalControlAuto },
    { OMX_TI_Portrait,
      OMX_IMAGE_FlashControlAuto,
      OMX_IMAGE_FocusControlPortrait,
      OMX_WhiteBalControlAuto },
    { OMX_TI_Snow,
      OMX_IMAGE_FlashControlOff,
      OMX_IMAGE_FocusControlAutoLock,
      OMX_WhiteBalControlAuto },
    { OMX_TI_Steadyphoto,
      OMX_IMAGE_FlashControlAuto,
      OMX_IMAGE_FocusControlHyperfocal,
      OMX_WhiteBalControlAuto },
    { OMX_TI_Sunset,
      OMX_IMAGE_FlashControlOff,
      OMX_IMAGE_FocusControlAuto,
      OMX_WhiteBalControlSunLight },
    { OMX_TI_Theatre,
      OMX_IMAGE_FlashControlAuto,
      OMX_IMAGE_FocusControlHyperfocal,
      OMX_WhiteBalControlAuto },
};

static const SceneModesEntry S5K6A1GX03_SceneModesLUT [] = {
    { OMX_Closeup,
      OMX_IMAGE_FlashControlOff,
      OMX_IMAGE_FocusControlHyperfocal,
      OMX_WhiteBalControlAuto },
    { OMX_Landscape,
      OMX_IMAGE_FlashControlOff,
      OMX_IMAGE_FocusControlHyperfocal,
      OMX_WhiteBalControlAuto },
    { OMX_Underwater,
      OMX_IMAGE_FlashControlOff,
      OMX_IMAGE_FocusControlHyperfocal,
      OMX_WhiteBalControlSunLight },
    { OMX_Sport,
      OMX_IMAGE_FlashControlOff,
      OMX_IMAGE_FocusControlHyperfocal,
      OMX_WhiteBalControlAuto },
    { OMX_SnowBeach,
      OMX_IMAGE_FlashControlOff,
      OMX_IMAGE_FocusControlHyperfocal,
      OMX_WhiteBalControlAuto },
    { OMX_Mood,
      OMX_IMAGE_FlashControlOff,
      OMX_IMAGE_FocusControlHyperfocal,
      OMX_WhiteBalControlAuto },
    { OMX_NightPortrait,
      OMX_IMAGE_FlashControlOff,
      OMX_IMAGE_FocusControlHyperfocal,
      OMX_WhiteBalControlAuto },
    { OMX_NightIndoor,
      OMX_IMAGE_FlashControlOff,
      OMX_IMAGE_FocusControlHyperfocal,
      OMX_WhiteBalControlAuto },
    { OMX_Fireworks,
      OMX_IMAGE_FlashControlOff,
      OMX_IMAGE_FocusControlHyperfocal,
      OMX_WhiteBalControlAuto },
    { OMX_Document,
      OMX_IMAGE_FlashControlOff,
      OMX_IMAGE_FocusControlHyperfocal,
      OMX_WhiteBalControlAuto },
    { OMX_Barcode,
      OMX_IMAGE_FlashControlOff,
      OMX_IMAGE_FocusControlHyperfocal,
      OMX_WhiteBalControlAuto },
    { OMX_SuperNight,
      OMX_IMAGE_FlashControlOff,
      OMX_IMAGE_FocusControlHyperfocal,
      OMX_WhiteBalControlAuto },
    { OMX_Cine,
      OMX_IMAGE_FlashControlOff,
      OMX_IMAGE_FocusControlHyperfocal,
      OMX_WhiteBalControlAuto },
    { OMX_OldFilm,
      OMX_IMAGE_FlashControlOff,
      OMX_IMAGE_FocusControlHyperfocal,
      OMX_WhiteBalControlAuto },
    { OMX_TI_Action,
      OMX_IMAGE_FlashControlOff,
      OMX_IMAGE_FocusControlHyperfocal,
      OMX_WhiteBalControlAuto },
    { OMX_TI_Beach,
      OMX_IMAGE_FlashControlOff,
      OMX_IMAGE_FocusControlHyperfocal,
      OMX_WhiteBalControlAuto },
    { OMX_TI_Candlelight,
      OMX_IMAGE_FlashControlOff,
      OMX_IMAGE_FocusControlHyperfocal,
      OMX_WhiteBalControlIncandescent },
    { OMX_TI_Night,
      OMX_IMAGE_FlashControlOff,
      OMX_IMAGE_FocusControlHyperfocal,
      OMX_WhiteBalControlAuto },
    { OMX_TI_Party,
      OMX_IMAGE_FlashControlOff,
      OMX_IMAGE_FocusControlHyperfocal,
      OMX_WhiteBalControlAuto },
    { OMX_TI_Portrait,
      OMX_IMAGE_FlashControlOff,
      OMX_IMAGE_FocusControlHyperfocal,
      OMX_WhiteBalControlAuto },
    { OMX_TI_Snow,
      OMX_IMAGE_FlashControlOff,
      OMX_IMAGE_FocusControlHyperfocal,
      OMX_WhiteBalControlAuto },
    { OMX_TI_Steadyphoto,
      OMX_IMAGE_FlashControlOff,
      OMX_IMAGE_FocusControlHyperfocal,
      OMX_WhiteBalControlAuto },
    { OMX_TI_Sunset,
      OMX_IMAGE_FlashControlOff,
      OMX_IMAGE_FocusControlHyperfocal,
      OMX_WhiteBalControlSunLight },
    { OMX_TI_Theatre,
      OMX_IMAGE_FlashControlOff,
      OMX_IMAGE_FocusControlHyperfocal,
      OMX_WhiteBalControlAuto },
};

static const CameraToSensorModesLUTEntry CameraToSensorModesLUT [] = {
    { "S5K4E1GA", S5K4E1GA_SceneModesLUT, ARRAY_SIZE(S5K4E1GA_SceneModesLUT)},
    { "S5K6A1GX03", S5K6A1GX03_SceneModesLUT, ARRAY_SIZE(S5K6A1GX03_SceneModesLUT)},
};

#endif
