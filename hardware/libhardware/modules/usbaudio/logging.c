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

#define LOG_TAG "usb_logging"
/*#define LOG_NDEBUG 0*/

#include <string.h>

#include <log/log.h>

#include "logging.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

/*
 * Logging
 */
void log_pcm_mask(const char* mask_name, struct pcm_mask* mask)
{
    const size_t num_slots = ARRAY_SIZE(mask->bits);
    const size_t bits_per_slot = (sizeof(mask->bits[0]) * 8);
    const size_t chars_per_slot = (bits_per_slot + 1); /* comma */

    const size_t BUFF_SIZE =
            (num_slots * chars_per_slot + 2 + 1);  /* brackets and null-terminator */
    char buff[BUFF_SIZE];
    buff[0] = '\0';

    size_t slot_index, bit_index;
    strcat(buff, "[");
    for (slot_index = 0; slot_index < num_slots; slot_index++) {
        unsigned bit_mask = 1;
        for (bit_index = 0; bit_index < bits_per_slot; bit_index++) {
            strcat(buff, (mask->bits[slot_index] & bit_mask) != 0 ? "1" : "0");
            bit_mask <<= 1;
        }
        if (slot_index < num_slots - 1) {
            strcat(buff, ",");
        }
    }
    strcat(buff, "]");

    ALOGV("%s: mask:%s", mask_name, buff);
}

void log_pcm_params(struct pcm_params * alsa_hw_params)
{
    ALOGV("usb:audio_hw - PCM_PARAM_SAMPLE_BITS min:%u, max:%u",
          pcm_params_get_min(alsa_hw_params, PCM_PARAM_SAMPLE_BITS),
          pcm_params_get_max(alsa_hw_params, PCM_PARAM_SAMPLE_BITS));
    ALOGV("usb:audio_hw - PCM_PARAM_FRAME_BITS min:%u, max:%u",
          pcm_params_get_min(alsa_hw_params, PCM_PARAM_FRAME_BITS),
          pcm_params_get_max(alsa_hw_params, PCM_PARAM_FRAME_BITS));
    log_pcm_mask("PCM_PARAM_FORMAT",
                 pcm_params_get_mask(alsa_hw_params, PCM_PARAM_FORMAT));
    log_pcm_mask("PCM_PARAM_SUBFORMAT",
                 pcm_params_get_mask(alsa_hw_params, PCM_PARAM_SUBFORMAT));
    ALOGV("usb:audio_hw - PCM_PARAM_CHANNELS min:%u, max:%u",
          pcm_params_get_min(alsa_hw_params, PCM_PARAM_CHANNELS),
          pcm_params_get_max(alsa_hw_params, PCM_PARAM_CHANNELS));
    ALOGV("usb:audio_hw - PCM_PARAM_RATE min:%u, max:%u",
          pcm_params_get_min(alsa_hw_params, PCM_PARAM_RATE),
          pcm_params_get_max(alsa_hw_params, PCM_PARAM_RATE));
    ALOGV("usb:audio_hw - PCM_PARAM_PERIOD_TIME min:%u, max:%u",
          pcm_params_get_min(alsa_hw_params, PCM_PARAM_PERIOD_TIME),
          pcm_params_get_max(alsa_hw_params, PCM_PARAM_PERIOD_TIME));
    ALOGV("usb:audio_hw - PCM_PARAM_PERIOD_SIZE min:%u, max:%u",
          pcm_params_get_min(alsa_hw_params, PCM_PARAM_PERIOD_SIZE),
          pcm_params_get_max(alsa_hw_params, PCM_PARAM_PERIOD_SIZE));
    ALOGV("usb:audio_hw - PCM_PARAM_PERIOD_BYTES min:%u, max:%u",
          pcm_params_get_min(alsa_hw_params, PCM_PARAM_PERIOD_BYTES),
          pcm_params_get_max(alsa_hw_params, PCM_PARAM_PERIOD_BYTES));
    ALOGV("usb:audio_hw - PCM_PARAM_PERIODS min:%u, max:%u",
          pcm_params_get_min(alsa_hw_params, PCM_PARAM_PERIODS),
          pcm_params_get_max(alsa_hw_params, PCM_PARAM_PERIODS));
    ALOGV("usb:audio_hw - PCM_PARAM_BUFFER_TIME min:%u, max:%u",
          pcm_params_get_min(alsa_hw_params, PCM_PARAM_BUFFER_TIME),
          pcm_params_get_max(alsa_hw_params, PCM_PARAM_BUFFER_TIME));
    ALOGV("usb:audio_hw - PCM_PARAM_BUFFER_SIZE min:%u, max:%u",
          pcm_params_get_min(alsa_hw_params, PCM_PARAM_BUFFER_SIZE),
          pcm_params_get_max(alsa_hw_params, PCM_PARAM_BUFFER_SIZE));
    ALOGV("usb:audio_hw - PCM_PARAM_BUFFER_BYTES min:%u, max:%u",
          pcm_params_get_min(alsa_hw_params, PCM_PARAM_BUFFER_BYTES),
          pcm_params_get_max(alsa_hw_params, PCM_PARAM_BUFFER_BYTES));
    ALOGV("usb:audio_hw - PCM_PARAM_TICK_TIME min:%u, max:%u",
          pcm_params_get_min(alsa_hw_params, PCM_PARAM_TICK_TIME),
          pcm_params_get_max(alsa_hw_params, PCM_PARAM_TICK_TIME));
}

void log_pcm_config(struct pcm_config * config, const char* label) {
    ALOGV("log_pcm_config() - %s", label);
    ALOGV("  channels:%d", config->channels);
    ALOGV("  rate:%d", config->rate);
    ALOGV("  period_size:%d", config->period_size);
    ALOGV("  period_count:%d", config->period_count);
    ALOGV("  format:%d", config->format);
#if 0
    /* Values to use for the ALSA start, stop and silence thresholds.  Setting
     * any one of these values to 0 will cause the default tinyalsa values to be
     * used instead.  Tinyalsa defaults are as follows.
     *
     * start_threshold   : period_count * period_size
     * stop_threshold    : period_count * period_size
     * silence_threshold : 0
     */
    unsigned int start_threshold;
    unsigned int stop_threshold;
    unsigned int silence_threshold;

    /* Minimum number of frames available before pcm_mmap_write() will actually
     * write into the kernel buffer. Only used if the stream is opened in mmap mode
     * (pcm_open() called with PCM_MMAP flag set).   Use 0 for default.
     */
    int avail_min;
#endif
}
