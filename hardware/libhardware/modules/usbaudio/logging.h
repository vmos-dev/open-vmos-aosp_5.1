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

#ifndef ANDROID_HARDWARE_LIBHARDWARE_MODULES_USBAUDIO_LOGGING_H
#define ANDROID_HARDWARE_LIBHARDWARE_MODULES_USBAUDIO_LOGGING_H

#include <tinyalsa/asoundlib.h>

void log_pcm_mask(const char* mask_name, struct pcm_mask* mask);
void log_pcm_params(struct pcm_params * alsa_hw_params);
void log_pcm_config(struct pcm_config * config, const char* label);

#endif /* ANDROID_HARDWARE_LIBHARDWARE_MODULES_USBAUDIO_LOGGING_H */
