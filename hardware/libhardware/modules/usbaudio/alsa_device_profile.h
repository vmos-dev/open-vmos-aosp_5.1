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

#ifndef ANDROID_HARDWARE_LIBHARDWARE_MODULES_USBAUDIO_ALSA_DEVICE_PROFILE_H
#define ANDROID_HARDWARE_LIBHARDWARE_MODULES_USBAUDIO_ALSA_DEVICE_PROFILE_H

#include <stdbool.h>

#include <tinyalsa/asoundlib.h>

#define MAX_PROFILE_FORMATS         6  /* We long support the 5 standard formats defined
                                        * in asound.h, so we just need this to be 1 more
                                        * than that */
#define MAX_PROFILE_SAMPLE_RATES    10 /* this number needs to be 1 more than the number of
                                        * standard formats in std_sample_rates[]
                                        * (in alsa_device_profile.c) */
#define MAX_PROFILE_CHANNEL_COUNTS  5  /* this number need to be 1 more than the number of
                                        * standard channel formats in std_channel_counts[]
                                        * (in alsa_device_profile.c) */

#define DEFAULT_SAMPLE_RATE         44100
#define DEFAULT_SAMPLE_FORMAT       PCM_FORMAT_S16_LE
#define DEFAULT_CHANNEL_COUNT       2

typedef struct  {
    int card;
    int device;
    int direction; /* PCM_OUT or PCM_IN */

    enum pcm_format formats[MAX_PROFILE_FORMATS];

    unsigned sample_rates[MAX_PROFILE_SAMPLE_RATES];

    unsigned channel_counts[MAX_PROFILE_CHANNEL_COUNTS];

    bool is_valid;

    /* read from the hardware device */
    struct pcm_config default_config;

    unsigned min_period_size;
    unsigned max_period_size;

    unsigned min_channel_count;
    unsigned max_channel_count;
} alsa_device_profile;

void profile_init(alsa_device_profile* profile, int direction);
bool profile_is_initialized(alsa_device_profile* profile);
bool profile_is_valid(alsa_device_profile* profile);
bool profile_is_cached_for(alsa_device_profile* profile, int card, int device);
void profile_decache(alsa_device_profile* profile);

bool profile_read_device_info(alsa_device_profile* profile);

/* Audio Config Strings Methods */
char * profile_get_sample_rate_strs(alsa_device_profile* profile);
char * profile_get_format_strs(alsa_device_profile* profile);
char * profile_get_channel_count_strs(alsa_device_profile* profile);

/* Sample Rate Methods */
unsigned profile_get_default_sample_rate(alsa_device_profile* profile);
bool profile_is_sample_rate_valid(alsa_device_profile* profile, unsigned rate);

/* Format Methods */
enum pcm_format profile_get_default_format(alsa_device_profile* profile);
bool profile_is_format_valid(alsa_device_profile* profile, enum pcm_format fmt);

/* Channel Methods */
unsigned profile_get_default_channel_count(alsa_device_profile* profile);
bool profile_is_channel_count_valid(alsa_device_profile* profile, unsigned count);

/* Utility */
unsigned profile_calc_min_period_size(alsa_device_profile* profile, unsigned sample_rate);
unsigned int profile_get_period_size(alsa_device_profile* profile, unsigned sample_rate);

#endif /* ANDROID_HARDWARE_LIBHARDWARE_MODULES_USBAUDIO_ALSA_DEVICE_PROFILE_H */
