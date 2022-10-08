/*
 * Copyright (C) 2014 The Android Open Source Project
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

#define LOG_TAG "offload_effect_api"
//#define LOG_NDEBUG 0

#include <stdbool.h>
#include <cutils/log.h>
#include <tinyalsa/asoundlib.h>
#include <sound/audio_effects.h>

#include "effect_api.h"

#define ARRAY_SIZE(array) (sizeof array / sizeof array[0])

#define OFFLOAD_PRESET_START_OFFSET_FOR_OPENSL 19
const int map_eq_opensl_preset_2_offload_preset[] = {
    OFFLOAD_PRESET_START_OFFSET_FOR_OPENSL,   /* Normal Preset */
    OFFLOAD_PRESET_START_OFFSET_FOR_OPENSL+1, /* Classical Preset */
    OFFLOAD_PRESET_START_OFFSET_FOR_OPENSL+2, /* Dance Preset */
    OFFLOAD_PRESET_START_OFFSET_FOR_OPENSL+3, /* Flat Preset */
    OFFLOAD_PRESET_START_OFFSET_FOR_OPENSL+4, /* Folk Preset */
    OFFLOAD_PRESET_START_OFFSET_FOR_OPENSL+5, /* Heavy Metal Preset */
    OFFLOAD_PRESET_START_OFFSET_FOR_OPENSL+6, /* Hip Hop Preset */
    OFFLOAD_PRESET_START_OFFSET_FOR_OPENSL+7, /* Jazz Preset */
    OFFLOAD_PRESET_START_OFFSET_FOR_OPENSL+8, /* Pop Preset */
    OFFLOAD_PRESET_START_OFFSET_FOR_OPENSL+9, /* Rock Preset */
    OFFLOAD_PRESET_START_OFFSET_FOR_OPENSL+10 /* FX Booster */
};

const int map_reverb_opensl_preset_2_offload_preset
                  [NUM_OSL_REVERB_PRESETS_SUPPORTED][2] = {
    {1, 15},
    {2, 16},
    {3, 17},
    {4, 18},
    {5, 3},
    {6, 20}
};

int offload_update_mixer_and_effects_ctl(int card, int device_id,
                                         struct mixer *mixer,
                                         struct mixer_ctl *ctl)
{
    char mixer_string[128];

    snprintf(mixer_string, sizeof(mixer_string),
             "%s %d", "Audio Effects Config", device_id);
    ALOGV("%s: mixer_string: %s", __func__, mixer_string);
    mixer = mixer_open(card);
    if (!mixer) {
        ALOGE("Failed to open mixer");
        ctl = NULL;
        return -EINVAL;
    } else {
        ctl = mixer_get_ctl_by_name(mixer, mixer_string);
        if (!ctl) {
            ALOGE("mixer_get_ctl_by_name failed");
            mixer_close(mixer);
            mixer = NULL;
            return -EINVAL;
        }
    }
    ALOGV("mixer: %p, ctl: %p", mixer, ctl);
    return 0;
}

void offload_close_mixer(struct mixer *mixer)
{
    mixer_close(mixer);
}

void offload_bassboost_set_device(struct bass_boost_params *bassboost,
                                  uint32_t device)
{
    ALOGV("%s", __func__);
    bassboost->device = device;
}

void offload_bassboost_set_enable_flag(struct bass_boost_params *bassboost,
                                       bool enable)
{
    ALOGV("%s", __func__);
    bassboost->enable_flag = enable;
}

int offload_bassboost_get_enable_flag(struct bass_boost_params *bassboost)
{
    ALOGV("%s", __func__);
    return bassboost->enable_flag;
}

void offload_bassboost_set_strength(struct bass_boost_params *bassboost,
                                    int strength)
{
    ALOGV("%s", __func__);
    bassboost->strength = strength;
}

void offload_bassboost_set_mode(struct bass_boost_params *bassboost,
                                int mode)
{
    ALOGV("%s", __func__);
    bassboost->mode = mode;
}

int offload_bassboost_send_params(struct mixer_ctl *ctl,
                                  struct bass_boost_params *bassboost,
                                  unsigned param_send_flags)
{
    int param_values[128] = {0};
    int *p_param_values = param_values;

    ALOGV("%s", __func__);
    *p_param_values++ = BASS_BOOST_MODULE;
    *p_param_values++ = bassboost->device;
    *p_param_values++ = 0; /* num of commands*/
    if (param_send_flags & OFFLOAD_SEND_BASSBOOST_ENABLE_FLAG) {
        *p_param_values++ = BASS_BOOST_ENABLE;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = BASS_BOOST_ENABLE_PARAM_LEN;
        *p_param_values++ = bassboost->enable_flag;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_BASSBOOST_STRENGTH) {
        *p_param_values++ = BASS_BOOST_STRENGTH;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = BASS_BOOST_STRENGTH_PARAM_LEN;
        *p_param_values++ = bassboost->strength;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_BASSBOOST_MODE) {
        *p_param_values++ = BASS_BOOST_MODE;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = BASS_BOOST_MODE_PARAM_LEN;
        *p_param_values++ = bassboost->mode;
        param_values[2] += 1;
    }

    if (param_values[2] && ctl)
        mixer_ctl_set_array(ctl, param_values, ARRAY_SIZE(param_values));

    return 0;
}

void offload_virtualizer_set_device(struct virtualizer_params *virtualizer,
                                    uint32_t device)
{
    ALOGV("%s", __func__);
    virtualizer->device = device;
}

void offload_virtualizer_set_enable_flag(struct virtualizer_params *virtualizer,
                                         bool enable)
{
    ALOGV("%s", __func__);
    virtualizer->enable_flag = enable;
}

int offload_virtualizer_get_enable_flag(struct virtualizer_params *virtualizer)
{
    ALOGV("%s", __func__);
    return virtualizer->enable_flag;
}

void offload_virtualizer_set_strength(struct virtualizer_params *virtualizer,
                                      int strength)
{
    ALOGV("%s", __func__);
    virtualizer->strength = strength;
}

void offload_virtualizer_set_out_type(struct virtualizer_params *virtualizer,
                                      int out_type)
{
    ALOGV("%s", __func__);
    virtualizer->out_type = out_type;
}

void offload_virtualizer_set_gain_adjust(struct virtualizer_params *virtualizer,
                                         int gain_adjust)
{
    ALOGV("%s", __func__);
    virtualizer->gain_adjust = gain_adjust;
}

int offload_virtualizer_send_params(struct mixer_ctl *ctl,
                                    struct virtualizer_params *virtualizer,
                                    unsigned param_send_flags)
{
    int param_values[128] = {0};
    int *p_param_values = param_values;

    ALOGV("%s", __func__);
    *p_param_values++ = VIRTUALIZER_MODULE;
    *p_param_values++ = virtualizer->device;
    *p_param_values++ = 0; /* num of commands*/
    if (param_send_flags & OFFLOAD_SEND_VIRTUALIZER_ENABLE_FLAG) {
        *p_param_values++ = VIRTUALIZER_ENABLE;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = VIRTUALIZER_ENABLE_PARAM_LEN;
        *p_param_values++ = virtualizer->enable_flag;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_VIRTUALIZER_STRENGTH) {
        *p_param_values++ = VIRTUALIZER_STRENGTH;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = VIRTUALIZER_STRENGTH_PARAM_LEN;
        *p_param_values++ = virtualizer->strength;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_VIRTUALIZER_OUT_TYPE) {
        *p_param_values++ = VIRTUALIZER_OUT_TYPE;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = VIRTUALIZER_OUT_TYPE_PARAM_LEN;
        *p_param_values++ = virtualizer->out_type;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_VIRTUALIZER_GAIN_ADJUST) {
        *p_param_values++ = VIRTUALIZER_GAIN_ADJUST;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = VIRTUALIZER_GAIN_ADJUST_PARAM_LEN;
        *p_param_values++ = virtualizer->gain_adjust;
        param_values[2] += 1;
    }

    if (param_values[2] && ctl)
        mixer_ctl_set_array(ctl, param_values, ARRAY_SIZE(param_values));

    return 0;
}

void offload_eq_set_device(struct eq_params *eq, uint32_t device)
{
    ALOGV("%s", __func__);
    eq->device = device;
}

void offload_eq_set_enable_flag(struct eq_params *eq, bool enable)
{
    ALOGV("%s", __func__);
    eq->enable_flag = enable;
}

int offload_eq_get_enable_flag(struct eq_params *eq)
{
    ALOGV("%s", __func__);
    return eq->enable_flag;
}

void offload_eq_set_preset(struct eq_params *eq, int preset)
{
    ALOGV("%s", __func__);
    eq->config.preset_id = preset;
    eq->config.eq_pregain = Q27_UNITY;
}

void offload_eq_set_bands_level(struct eq_params *eq, int num_bands,
                                const uint16_t *band_freq_list,
                                int *band_gain_list)
{
    int i;
    ALOGV("%s", __func__);
    eq->config.num_bands = num_bands;
    for (i=0; i<num_bands; i++) {
        eq->per_band_cfg[i].band_idx = i;
        eq->per_band_cfg[i].filter_type = EQ_BAND_BOOST;
        eq->per_band_cfg[i].freq_millihertz = band_freq_list[i] * 1000;
        eq->per_band_cfg[i].gain_millibels = band_gain_list[i] * 100;
        eq->per_band_cfg[i].quality_factor = Q8_UNITY;
    }
}

int offload_eq_send_params(struct mixer_ctl *ctl, struct eq_params *eq,
                           unsigned param_send_flags)
{
    int param_values[128] = {0};
    int *p_param_values = param_values;
    uint32_t i;

    ALOGV("%s", __func__);
    if (eq->config.preset_id < -1 ) {
        ALOGV("No Valid preset to set");
        return 0;
    }
    *p_param_values++ = EQ_MODULE;
    *p_param_values++ = eq->device;
    *p_param_values++ = 0; /* num of commands*/
    if (param_send_flags & OFFLOAD_SEND_EQ_ENABLE_FLAG) {
        *p_param_values++ = EQ_ENABLE;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = EQ_ENABLE_PARAM_LEN;
        *p_param_values++ = eq->enable_flag;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_EQ_PRESET) {
        *p_param_values++ = EQ_CONFIG;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = EQ_CONFIG_PARAM_LEN;
        *p_param_values++ = eq->config.eq_pregain;
        *p_param_values++ =
                     map_eq_opensl_preset_2_offload_preset[eq->config.preset_id];
        *p_param_values++ = 0;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_EQ_BANDS_LEVEL) {
        *p_param_values++ = EQ_CONFIG;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = EQ_CONFIG_PARAM_LEN +
                            eq->config.num_bands * EQ_CONFIG_PER_BAND_PARAM_LEN;
        *p_param_values++ = eq->config.eq_pregain;
        *p_param_values++ = CUSTOM_OPENSL_PRESET;
        *p_param_values++ = eq->config.num_bands;
        for (i=0; i<eq->config.num_bands; i++) {
            *p_param_values++ = eq->per_band_cfg[i].band_idx;
            *p_param_values++ = eq->per_band_cfg[i].filter_type;
	    *p_param_values++ = eq->per_band_cfg[i].freq_millihertz;
            *p_param_values++ = eq->per_band_cfg[i].gain_millibels;
            *p_param_values++ = eq->per_band_cfg[i].quality_factor;
        }
        param_values[2] += 1;
    }

    if (param_values[2] && ctl)
        mixer_ctl_set_array(ctl, param_values, ARRAY_SIZE(param_values));

    return 0;
}

void offload_reverb_set_device(struct reverb_params *reverb, uint32_t device)
{
    ALOGV("%s", __func__);
    reverb->device = device;
}

void offload_reverb_set_enable_flag(struct reverb_params *reverb, bool enable)
{
    ALOGV("%s", __func__);
    reverb->enable_flag = enable;
}

int offload_reverb_get_enable_flag(struct reverb_params *reverb)
{
    ALOGV("%s", __func__);
    return reverb->enable_flag;
}

void offload_reverb_set_mode(struct reverb_params *reverb, int mode)
{
    ALOGV("%s", __func__);
    reverb->mode = mode;
}

void offload_reverb_set_preset(struct reverb_params *reverb, int preset)
{
    ALOGV("%s", __func__);
    if (preset && (preset <= NUM_OSL_REVERB_PRESETS_SUPPORTED))
        reverb->preset = map_reverb_opensl_preset_2_offload_preset[preset-1][1];
}

void offload_reverb_set_wet_mix(struct reverb_params *reverb, int wet_mix)
{
    ALOGV("%s", __func__);
    reverb->wet_mix = wet_mix;
}

void offload_reverb_set_gain_adjust(struct reverb_params *reverb,
                                    int gain_adjust)
{
    ALOGV("%s", __func__);
    reverb->gain_adjust = gain_adjust;
}

void offload_reverb_set_room_level(struct reverb_params *reverb, int room_level)
{
    ALOGV("%s", __func__);
    reverb->room_level = room_level;
}

void offload_reverb_set_room_hf_level(struct reverb_params *reverb,
                                      int room_hf_level)
{
    ALOGV("%s", __func__);
    reverb->room_hf_level = room_hf_level;
}

void offload_reverb_set_decay_time(struct reverb_params *reverb, int decay_time)
{
    ALOGV("%s", __func__);
    reverb->decay_time = decay_time;
}

void offload_reverb_set_decay_hf_ratio(struct reverb_params *reverb,
                                       int decay_hf_ratio)
{
    ALOGV("%s", __func__);
    reverb->decay_hf_ratio = decay_hf_ratio;
}

void offload_reverb_set_reflections_level(struct reverb_params *reverb,
                                          int reflections_level)
{
    ALOGV("%s", __func__);
    reverb->reflections_level = reflections_level;
}

void offload_reverb_set_reflections_delay(struct reverb_params *reverb,
                                          int reflections_delay)
{
    ALOGV("%s", __func__);
    reverb->reflections_delay = reflections_delay;
}

void offload_reverb_set_reverb_level(struct reverb_params *reverb,
                                     int reverb_level)
{
    ALOGV("%s", __func__);
    reverb->level = reverb_level;
}

void offload_reverb_set_delay(struct reverb_params *reverb, int delay)
{
    ALOGV("%s", __func__);
    reverb->delay = delay;
}

void offload_reverb_set_diffusion(struct reverb_params *reverb, int diffusion)
{
    ALOGV("%s", __func__);
    reverb->diffusion = diffusion;
}

void offload_reverb_set_density(struct reverb_params *reverb, int density)
{
    ALOGV("%s", __func__);
    reverb->density = density;
}

int offload_reverb_send_params(struct mixer_ctl *ctl,
                               struct reverb_params *reverb,
                               unsigned param_send_flags)
{
    int param_values[128] = {0};
    int *p_param_values = param_values;

    ALOGV("%s", __func__);
    *p_param_values++ = REVERB_MODULE;
    *p_param_values++ = reverb->device;
    *p_param_values++ = 0; /* num of commands*/

    if (param_send_flags & OFFLOAD_SEND_REVERB_ENABLE_FLAG) {
        *p_param_values++ = REVERB_ENABLE;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = REVERB_ENABLE_PARAM_LEN;
        *p_param_values++ = reverb->enable_flag;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_REVERB_MODE) {
        *p_param_values++ = REVERB_MODE;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = REVERB_MODE_PARAM_LEN;
        *p_param_values++ = reverb->mode;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_REVERB_PRESET) {
        *p_param_values++ = REVERB_PRESET;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = REVERB_PRESET_PARAM_LEN;
        *p_param_values++ = reverb->preset;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_REVERB_WET_MIX) {
        *p_param_values++ = REVERB_WET_MIX;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = REVERB_WET_MIX_PARAM_LEN;
        *p_param_values++ = reverb->wet_mix;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_REVERB_GAIN_ADJUST) {
        *p_param_values++ = REVERB_GAIN_ADJUST;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = REVERB_GAIN_ADJUST_PARAM_LEN;
        *p_param_values++ = reverb->gain_adjust;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_REVERB_ROOM_LEVEL) {
        *p_param_values++ = REVERB_ROOM_LEVEL;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = REVERB_ROOM_LEVEL_PARAM_LEN;
        *p_param_values++ = reverb->room_level;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_REVERB_ROOM_HF_LEVEL) {
        *p_param_values++ = REVERB_ROOM_HF_LEVEL;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = REVERB_ROOM_HF_LEVEL_PARAM_LEN;
        *p_param_values++ = reverb->room_hf_level;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_REVERB_DECAY_TIME) {
        *p_param_values++ = REVERB_DECAY_TIME;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = REVERB_DECAY_TIME_PARAM_LEN;
        *p_param_values++ = reverb->decay_time;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_REVERB_DECAY_HF_RATIO) {
        *p_param_values++ = REVERB_DECAY_HF_RATIO;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = REVERB_DECAY_HF_RATIO_PARAM_LEN;
        *p_param_values++ = reverb->decay_hf_ratio;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_REVERB_REFLECTIONS_LEVEL) {
        *p_param_values++ = REVERB_REFLECTIONS_LEVEL;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = REVERB_REFLECTIONS_LEVEL_PARAM_LEN;
        *p_param_values++ = reverb->reflections_level;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_REVERB_REFLECTIONS_DELAY) {
        *p_param_values++ = REVERB_REFLECTIONS_DELAY;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = REVERB_REFLECTIONS_DELAY_PARAM_LEN;
        *p_param_values++ = reverb->reflections_delay;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_REVERB_LEVEL) {
        *p_param_values++ = REVERB_LEVEL;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = REVERB_LEVEL_PARAM_LEN;
        *p_param_values++ = reverb->level;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_REVERB_DELAY) {
        *p_param_values++ = REVERB_DELAY;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = REVERB_DELAY_PARAM_LEN;
        *p_param_values++ = reverb->delay;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_REVERB_DIFFUSION) {
        *p_param_values++ = REVERB_DIFFUSION;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = REVERB_DIFFUSION_PARAM_LEN;
        *p_param_values++ = reverb->diffusion;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_REVERB_DENSITY) {
        *p_param_values++ = REVERB_DENSITY;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = REVERB_DENSITY_PARAM_LEN;
        *p_param_values++ = reverb->density;
        param_values[2] += 1;
    }

    if (param_values[2] && ctl)
        mixer_ctl_set_array(ctl, param_values, ARRAY_SIZE(param_values));

    return 0;
}
