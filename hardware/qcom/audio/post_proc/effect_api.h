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

#ifndef OFFLOAD_EFFECT_API_H_
#define OFFLOAD_EFFECT_API_H_

int offload_update_mixer_and_effects_ctl(int card, int device_id,
                                         struct mixer *mixer,
                                         struct mixer_ctl *ctl);
void offload_close_mixer(struct mixer *mixer);

#define OFFLOAD_SEND_BASSBOOST_ENABLE_FLAG      (1 << 0)
#define OFFLOAD_SEND_BASSBOOST_STRENGTH         \
                                          (OFFLOAD_SEND_BASSBOOST_ENABLE_FLAG << 1)
#define OFFLOAD_SEND_BASSBOOST_MODE             \
                                          (OFFLOAD_SEND_BASSBOOST_STRENGTH << 1)
void offload_bassboost_set_device(struct bass_boost_params *bassboost,
                                  uint32_t device);
void offload_bassboost_set_enable_flag(struct bass_boost_params *bassboost,
                                       bool enable);
int offload_bassboost_get_enable_flag(struct bass_boost_params *bassboost);
void offload_bassboost_set_strength(struct bass_boost_params *bassboost,
                                    int strength);
void offload_bassboost_set_mode(struct bass_boost_params *bassboost,
                                int mode);
int offload_bassboost_send_params(struct mixer_ctl *ctl,
                                  struct bass_boost_params *bassboost,
                                  unsigned param_send_flags);

#define OFFLOAD_SEND_VIRTUALIZER_ENABLE_FLAG    (1 << 0)
#define OFFLOAD_SEND_VIRTUALIZER_STRENGTH       \
                                          (OFFLOAD_SEND_VIRTUALIZER_ENABLE_FLAG << 1)
#define OFFLOAD_SEND_VIRTUALIZER_OUT_TYPE       \
                                          (OFFLOAD_SEND_VIRTUALIZER_STRENGTH << 1)
#define OFFLOAD_SEND_VIRTUALIZER_GAIN_ADJUST    \
                                          (OFFLOAD_SEND_VIRTUALIZER_OUT_TYPE << 1)
void offload_virtualizer_set_device(struct virtualizer_params *virtualizer,
                                    uint32_t device);
void offload_virtualizer_set_enable_flag(struct virtualizer_params *virtualizer,
                                         bool enable);
int offload_virtualizer_get_enable_flag(struct virtualizer_params *virtualizer);
void offload_virtualizer_set_strength(struct virtualizer_params *virtualizer,
                                      int strength);
void offload_virtualizer_set_out_type(struct virtualizer_params *virtualizer,
                                      int out_type);
void offload_virtualizer_set_gain_adjust(struct virtualizer_params *virtualizer,
                                         int gain_adjust);
int offload_virtualizer_send_params(struct mixer_ctl *ctl,
                                  struct virtualizer_params *virtualizer,
                                  unsigned param_send_flags);

#define OFFLOAD_SEND_EQ_ENABLE_FLAG             (1 << 0)
#define OFFLOAD_SEND_EQ_PRESET                  \
                                          (OFFLOAD_SEND_EQ_ENABLE_FLAG << 1)
#define OFFLOAD_SEND_EQ_BANDS_LEVEL             \
                                          (OFFLOAD_SEND_EQ_PRESET << 1)
void offload_eq_set_device(struct eq_params *eq, uint32_t device);
void offload_eq_set_enable_flag(struct eq_params *eq, bool enable);
int offload_eq_get_enable_flag(struct eq_params *eq);
void offload_eq_set_preset(struct eq_params *eq, int preset);
void offload_eq_set_bands_level(struct eq_params *eq, int num_bands,
                                const uint16_t *band_freq_list,
                                int *band_gain_list);
int offload_eq_send_params(struct mixer_ctl *ctl, struct eq_params *eq,
                           unsigned param_send_flags);

#define OFFLOAD_SEND_REVERB_ENABLE_FLAG         (1 << 0)
#define OFFLOAD_SEND_REVERB_MODE                \
                                          (OFFLOAD_SEND_REVERB_ENABLE_FLAG << 1)
#define OFFLOAD_SEND_REVERB_PRESET              \
                                          (OFFLOAD_SEND_REVERB_MODE << 1)
#define OFFLOAD_SEND_REVERB_WET_MIX             \
                                          (OFFLOAD_SEND_REVERB_PRESET << 1)
#define OFFLOAD_SEND_REVERB_GAIN_ADJUST	        \
                                          (OFFLOAD_SEND_REVERB_WET_MIX << 1)
#define OFFLOAD_SEND_REVERB_ROOM_LEVEL	        \
                                          (OFFLOAD_SEND_REVERB_GAIN_ADJUST << 1)
#define OFFLOAD_SEND_REVERB_ROOM_HF_LEVEL       \
                                          (OFFLOAD_SEND_REVERB_ROOM_LEVEL << 1)
#define OFFLOAD_SEND_REVERB_DECAY_TIME          \
                                          (OFFLOAD_SEND_REVERB_ROOM_HF_LEVEL << 1)
#define OFFLOAD_SEND_REVERB_DECAY_HF_RATIO      \
                                          (OFFLOAD_SEND_REVERB_DECAY_TIME << 1)
#define OFFLOAD_SEND_REVERB_REFLECTIONS_LEVEL   \
                                          (OFFLOAD_SEND_REVERB_DECAY_HF_RATIO << 1)
#define OFFLOAD_SEND_REVERB_REFLECTIONS_DELAY   \
                                          (OFFLOAD_SEND_REVERB_REFLECTIONS_LEVEL << 1)
#define OFFLOAD_SEND_REVERB_LEVEL               \
                                          (OFFLOAD_SEND_REVERB_REFLECTIONS_DELAY << 1)
#define OFFLOAD_SEND_REVERB_DELAY               \
                                          (OFFLOAD_SEND_REVERB_LEVEL << 1)
#define OFFLOAD_SEND_REVERB_DIFFUSION           \
                                          (OFFLOAD_SEND_REVERB_DELAY << 1)
#define OFFLOAD_SEND_REVERB_DENSITY             \
                                          (OFFLOAD_SEND_REVERB_DIFFUSION << 1)
void offload_reverb_set_device(struct reverb_params *reverb, uint32_t device);
void offload_reverb_set_enable_flag(struct reverb_params *reverb, bool enable);
int offload_reverb_get_enable_flag(struct reverb_params *reverb);
void offload_reverb_set_mode(struct reverb_params *reverb, int mode);
void offload_reverb_set_preset(struct reverb_params *reverb, int preset);
void offload_reverb_set_wet_mix(struct reverb_params *reverb, int wet_mix);
void offload_reverb_set_gain_adjust(struct reverb_params *reverb,
                                    int gain_adjust);
void offload_reverb_set_room_level(struct reverb_params *reverb,
                                   int room_level);
void offload_reverb_set_room_hf_level(struct reverb_params *reverb,
                                      int room_hf_level);
void offload_reverb_set_decay_time(struct reverb_params *reverb,
                                   int decay_time);
void offload_reverb_set_decay_hf_ratio(struct reverb_params *reverb,
                                       int decay_hf_ratio);
void offload_reverb_set_reflections_level(struct reverb_params *reverb,
                                          int reflections_level);
void offload_reverb_set_reflections_delay(struct reverb_params *reverb,
                                          int reflections_delay);
void offload_reverb_set_reverb_level(struct reverb_params *reverb,
                                     int reverb_level);
void offload_reverb_set_delay(struct reverb_params *reverb, int delay);
void offload_reverb_set_diffusion(struct reverb_params *reverb, int diffusion);
void offload_reverb_set_density(struct reverb_params *reverb, int density);
int offload_reverb_send_params(struct mixer_ctl *ctl,
                               struct reverb_params *reverb,
                               unsigned param_send_flags);

#endif /*OFFLOAD_EFFECT_API_H_*/
