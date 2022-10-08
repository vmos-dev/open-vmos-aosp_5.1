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

#ifndef ANDROID_HARDWARE_LIBHARDWARE_MODULES_USBAUDIO_ALSA_DEVICE_PROXY_H
#define ANDROID_HARDWARE_LIBHARDWARE_MODULES_USBAUDIO_ALSA_DEVICE_PROXY_H

#include <tinyalsa/asoundlib.h>

#include "alsa_device_profile.h"

typedef struct {
    alsa_device_profile* profile;

    struct pcm_config alsa_config;

    struct pcm * pcm;
} alsa_device_proxy;

void proxy_prepare(alsa_device_proxy * proxy, alsa_device_profile * profile,
                   struct pcm_config * config);

unsigned proxy_get_sample_rate(const alsa_device_proxy * proxy);
enum pcm_format proxy_get_format(const alsa_device_proxy * proxy);
unsigned proxy_get_channel_count(const alsa_device_proxy * proxy);

unsigned int proxy_get_period_size(const alsa_device_proxy * proxy);

unsigned proxy_get_latency(const alsa_device_proxy * proxy);

int proxy_open(alsa_device_proxy * proxy);
void proxy_close(alsa_device_proxy * proxy);

int proxy_write(const alsa_device_proxy * proxy, const void *data, unsigned int count);
int proxy_read(const alsa_device_proxy * proxy, void *data, unsigned int count);

#endif /* ANDROID_HARDWARE_LIBHARDWARE_MODULES_USBAUDIO_ALSA_DEVICE_PROXY_H */
