/*
 * Copyright (C) 2012 The Android Open Source Project
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

#include <system/audio.h>
#include <hardware/audio_effect.h>

#define ES305_IO_HANDLE_NONE -1
#define ES305_SESSION_ID_NONE -1977

#define ES305_ON "1"
#define ES305_OFF "0"

#define ES305_NS_DEFAULT_ON  "4"
#define ES305_NS_VOICE_REC_HANDHELD_ON "6"
#define ES305_NS_VOICE_REC_SINGLE_MIC_ON "0"
#define ES305_NS_OFF "0"
#define ES305_AGC_ON  "1"
#define ES305_AGC_OFF "0"
#define ES305_AEC_ON  "1"
#define ES305_AEC_OFF "0"

enum {
    ES305_PRESET_INIT = -3,
    ES305_PRESET_CURRENT = -2,
    ES305_PRESET_OFF = -1,
    ES305_PRESET_VOIP_HANDHELD = 0,
    ES305_PRESET_ASRA_HANDHELD = 1,
    ES305_PRESET_VOIP_DESKTOP = 2,
    ES305_PRESET_ASRA_DESKTOP = 3,
    ES305_PRESET_VOIP_HEADSET = 4,
    ES305_PRESET_ASRA_HEADSET = 5,
    ES305_PRESET_VOIP_HEADPHONES = 6,
    ES305_PRESET_VOIP_HP_DESKTOP = 7,
    ES305_PRESET_CAMCORDER = 8,
};

#ifdef __cplusplus
extern "C" {
#endif

    int eS305_UsePreset(int preset);

    /*
     * Sets the IO handle for the current input stream, or ADNC_IO_HANDLE_NONE when stream is
     * stopped.
     */
    int eS305_SetActiveIoHandle(audio_io_handle_t handle);

    int eS305_AddEffect(effect_descriptor_t * descr, audio_io_handle_t handle);

    int eS305_RemoveEffect(effect_descriptor_t * descr, audio_io_handle_t handle);

    int eS305_Release();

#ifdef __cplusplus
}
#endif

