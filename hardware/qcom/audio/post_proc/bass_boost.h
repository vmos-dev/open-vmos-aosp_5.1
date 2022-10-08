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

#ifndef OFFLOAD_EFFECT_BASS_BOOST_H_
#define OFFLOAD_EFFECT_BASS_BOOST_H_

#include "bundle.h"

extern const effect_descriptor_t bassboost_descriptor;

typedef struct bassboost_context_s {
    effect_context_t common;

    int strength;

    // Offload vars
    struct mixer_ctl *ctl;
    bool temp_disabled;
    uint32_t device;
    struct bass_boost_params offload_bass;
} bassboost_context_t;

int bassboost_get_parameter(effect_context_t *context, effect_param_t *p,
                            uint32_t *size);

int bassboost_set_parameter(effect_context_t *context, effect_param_t *p,
                            uint32_t size);

int bassboost_set_device(effect_context_t *context,  uint32_t device);

int bassboost_reset(effect_context_t *context);

int bassboost_init(effect_context_t *context);

int bassboost_enable(effect_context_t *context);

int bassboost_disable(effect_context_t *context);

int bassboost_start(effect_context_t *context, output_context_t *output);

int bassboost_stop(effect_context_t *context, output_context_t *output);

#endif /* OFFLOAD_EFFECT_BASS_BOOST_H_ */
