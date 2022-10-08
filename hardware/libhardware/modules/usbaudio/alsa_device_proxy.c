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

#define LOG_TAG "alsa_device_proxy"
/*#define LOG_NDEBUG 0*/
/*#define LOG_PCM_PARAMS 0*/

#include <log/log.h>

#include "alsa_device_proxy.h"

#include "logging.h"

#define DEFAULT_PERIOD_SIZE     1024
#define DEFAULT_PERIOD_COUNT    2

void proxy_prepare(alsa_device_proxy * proxy, alsa_device_profile* profile,
                   struct pcm_config * config)
{
    ALOGV("proxy_prepare()");

    proxy->profile = profile;

#ifdef LOG_PCM_PARAMS
    log_pcm_config(config, "proxy_setup()");
#endif

    proxy->alsa_config.format =
        config->format != PCM_FORMAT_INVALID && profile_is_format_valid(profile, config->format)
            ? config->format : profile->default_config.format;
    proxy->alsa_config.rate =
        config->rate != 0 && profile_is_sample_rate_valid(profile, config->rate)
            ? config->rate : profile->default_config.rate;
    proxy->alsa_config.channels =
        config->channels != 0 && profile_is_channel_count_valid(profile, config->channels)
            ? config->channels : profile->default_config.channels;

    proxy->alsa_config.period_count = profile->default_config.period_count;
    proxy->alsa_config.period_size =
            profile_get_period_size(proxy->profile, proxy->alsa_config.rate);

    // Hack for USB accessory audio.
    // Here we set the correct value for period_count if tinyalsa fails to get it from the
    // f_audio_source driver.
    if (proxy->alsa_config.period_count == 0) {
        proxy->alsa_config.period_count = 4;
    }

    proxy->pcm = NULL;
}

int proxy_open(alsa_device_proxy * proxy)
{
    alsa_device_profile* profile = proxy->profile;
    ALOGV("proxy_open(card:%d device:%d %s)", profile->card, profile->device,
          profile->direction == PCM_OUT ? "PCM_OUT" : "PCM_IN");

    proxy->pcm = pcm_open(profile->card, profile->device, profile->direction, &proxy->alsa_config);
    if (proxy->pcm == NULL) {
        return -ENOMEM;
    }

    if (!pcm_is_ready(proxy->pcm)) {
        ALOGE("[%s] proxy_open() pcm_open() failed: %s", LOG_TAG, pcm_get_error(proxy->pcm));
#ifdef LOG_PCM_PARAMS
        log_pcm_config(&proxy->alsa_config, "config");
#endif
        pcm_close(proxy->pcm);
        proxy->pcm = NULL;
        return -ENOMEM;
    }

    return 0;
}

void proxy_close(alsa_device_proxy * proxy)
{
    ALOGV("proxy_close() [pcm:%p]", proxy->pcm);

    if (proxy->pcm != NULL) {
        pcm_close(proxy->pcm);
        proxy->pcm = NULL;
    }
}

/*
 * Sample Rate
 */
unsigned proxy_get_sample_rate(const alsa_device_proxy * proxy)
{
    return proxy->alsa_config.rate;
}

/*
 * Format
 */
enum pcm_format proxy_get_format(const alsa_device_proxy * proxy)
{
    return proxy->alsa_config.format;
}

/*
 * Channel Count
 */
unsigned proxy_get_channel_count(const alsa_device_proxy * proxy)
{
    return proxy->alsa_config.channels;
}

/*
 * Other
 */
unsigned int proxy_get_period_size(const alsa_device_proxy * proxy)
{
    return proxy->alsa_config.period_size;
}

unsigned int proxy_get_period_count(const alsa_device_proxy * proxy)
{
    return proxy->alsa_config.period_count;
}

unsigned proxy_get_latency(const alsa_device_proxy * proxy)
{
    return (proxy_get_period_size(proxy) * proxy_get_period_count(proxy) * 1000)
               / proxy_get_sample_rate(proxy);
}

/*
 * I/O
 */
int proxy_write(const alsa_device_proxy * proxy, const void *data, unsigned int count)
{
    return pcm_write(proxy->pcm, data, count);
}

int proxy_read(const alsa_device_proxy * proxy, void *data, unsigned int count)
{
    return pcm_read(proxy->pcm, data, count);
}
