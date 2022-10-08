/*
 * Copyright (C) 2013-2014 The Android Open Source Project
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

#define LOG_TAG "audio_hw_primary"
/*#define LOG_NDEBUG 0*/
/*#define VERY_VERY_VERBOSE_LOGGING*/
#ifdef VERY_VERY_VERBOSE_LOGGING
#define ALOGVV ALOGV
#else
#define ALOGVV(a...) do { } while(0)
#endif

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/time.h>
#include <stdlib.h>
#include <math.h>
#include <dlfcn.h>
#include <sys/resource.h>
#include <sys/prctl.h>

#include <cutils/log.h>
#include <cutils/str_parms.h>
#include <cutils/properties.h>
#include <cutils/atomic.h>
#include <cutils/sched_policy.h>

#include <hardware/audio_effect.h>
#include <hardware/audio_alsaops.h>
#include <system/thread_defs.h>
#include <audio_effects/effect_aec.h>
#include <audio_effects/effect_ns.h>
#include "audio_hw.h"
#include "audio_extn.h"
#include "platform_api.h"
#include <platform.h>
#include "voice_extn.h"

#include "sound/compress_params.h"

#define COMPRESS_OFFLOAD_FRAGMENT_SIZE (32 * 1024)
#define COMPRESS_OFFLOAD_NUM_FRAGMENTS 4
/* ToDo: Check and update a proper value in msec */
#define COMPRESS_OFFLOAD_PLAYBACK_LATENCY 96
#define COMPRESS_PLAYBACK_VOLUME_MAX 0x2000

#define PROXY_OPEN_RETRY_COUNT           100
#define PROXY_OPEN_WAIT_TIME             20

static unsigned int configured_low_latency_capture_period_size =
        LOW_LATENCY_CAPTURE_PERIOD_SIZE;

/* This constant enables extended precision handling.
 * TODO The flag is off until more testing is done.
 */
static const bool k_enable_extended_precision = false;

struct pcm_config pcm_config_deep_buffer = {
    .channels = 2,
    .rate = DEFAULT_OUTPUT_SAMPLING_RATE,
    .period_size = DEEP_BUFFER_OUTPUT_PERIOD_SIZE,
    .period_count = DEEP_BUFFER_OUTPUT_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
    .start_threshold = DEEP_BUFFER_OUTPUT_PERIOD_SIZE / 4,
    .stop_threshold = INT_MAX,
    .avail_min = DEEP_BUFFER_OUTPUT_PERIOD_SIZE / 4,
};

struct pcm_config pcm_config_low_latency = {
    .channels = 2,
    .rate = DEFAULT_OUTPUT_SAMPLING_RATE,
    .period_size = LOW_LATENCY_OUTPUT_PERIOD_SIZE,
    .period_count = LOW_LATENCY_OUTPUT_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
    .start_threshold = LOW_LATENCY_OUTPUT_PERIOD_SIZE / 4,
    .stop_threshold = INT_MAX,
    .avail_min = LOW_LATENCY_OUTPUT_PERIOD_SIZE / 4,
};

struct pcm_config pcm_config_hdmi_multi = {
    .channels = HDMI_MULTI_DEFAULT_CHANNEL_COUNT, /* changed when the stream is opened */
    .rate = DEFAULT_OUTPUT_SAMPLING_RATE, /* changed when the stream is opened */
    .period_size = HDMI_MULTI_PERIOD_SIZE,
    .period_count = HDMI_MULTI_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
    .start_threshold = 0,
    .stop_threshold = INT_MAX,
    .avail_min = 0,
};

struct pcm_config pcm_config_audio_capture = {
    .channels = 2,
    .period_count = AUDIO_CAPTURE_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
};

#define AFE_PROXY_CHANNEL_COUNT 2
#define AFE_PROXY_SAMPLING_RATE 48000

#define AFE_PROXY_PLAYBACK_PERIOD_SIZE  768
#define AFE_PROXY_PLAYBACK_PERIOD_COUNT 4

struct pcm_config pcm_config_afe_proxy_playback = {
    .channels = AFE_PROXY_CHANNEL_COUNT,
    .rate = AFE_PROXY_SAMPLING_RATE,
    .period_size = AFE_PROXY_PLAYBACK_PERIOD_SIZE,
    .period_count = AFE_PROXY_PLAYBACK_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
    .start_threshold = AFE_PROXY_PLAYBACK_PERIOD_SIZE,
    .stop_threshold = INT_MAX,
    .avail_min = AFE_PROXY_PLAYBACK_PERIOD_SIZE,
};

#define AFE_PROXY_RECORD_PERIOD_SIZE  768
#define AFE_PROXY_RECORD_PERIOD_COUNT 4

struct pcm_config pcm_config_afe_proxy_record = {
    .channels = AFE_PROXY_CHANNEL_COUNT,
    .rate = AFE_PROXY_SAMPLING_RATE,
    .period_size = AFE_PROXY_RECORD_PERIOD_SIZE,
    .period_count = AFE_PROXY_RECORD_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
    .start_threshold = AFE_PROXY_RECORD_PERIOD_SIZE,
    .stop_threshold = INT_MAX,
    .avail_min = AFE_PROXY_RECORD_PERIOD_SIZE,
};

const char * const use_case_table[AUDIO_USECASE_MAX] = {
    [USECASE_AUDIO_PLAYBACK_DEEP_BUFFER] = "deep-buffer-playback",
    [USECASE_AUDIO_PLAYBACK_LOW_LATENCY] = "low-latency-playback",
    [USECASE_AUDIO_PLAYBACK_MULTI_CH] = "multi-channel-playback",
    [USECASE_AUDIO_PLAYBACK_OFFLOAD] = "compress-offload-playback",

    [USECASE_AUDIO_RECORD] = "audio-record",
    [USECASE_AUDIO_RECORD_LOW_LATENCY] = "low-latency-record",

    [USECASE_AUDIO_HFP_SCO] = "hfp-sco",
    [USECASE_AUDIO_HFP_SCO_WB] = "hfp-sco-wb",

    [USECASE_VOICE_CALL] = "voice-call",
    [USECASE_VOICE2_CALL] = "voice2-call",
    [USECASE_VOLTE_CALL] = "volte-call",
    [USECASE_QCHAT_CALL] = "qchat-call",
    [USECASE_VOWLAN_CALL] = "vowlan-call",

    [USECASE_AUDIO_PLAYBACK_AFE_PROXY] = "afe-proxy-playback",
    [USECASE_AUDIO_RECORD_AFE_PROXY] = "afe-proxy-record",
};


#define STRING_TO_ENUM(string) { #string, string }

struct string_to_enum {
    const char *name;
    uint32_t value;
};

static const struct string_to_enum out_channels_name_to_enum_table[] = {
    STRING_TO_ENUM(AUDIO_CHANNEL_OUT_STEREO),
    STRING_TO_ENUM(AUDIO_CHANNEL_OUT_5POINT1),
    STRING_TO_ENUM(AUDIO_CHANNEL_OUT_7POINT1),
};

static int set_voice_volume_l(struct audio_device *adev, float volume);

static bool is_supported_format(audio_format_t format)
{
    switch (format) {
        case AUDIO_FORMAT_MP3:
        case AUDIO_FORMAT_AAC_LC:
        case AUDIO_FORMAT_AAC_HE_V1:
        case AUDIO_FORMAT_AAC_HE_V2:
            return true;
        default:
            break;
    }
    return false;
}

static int get_snd_codec_id(audio_format_t format)
{
    int id = 0;

    switch (format & AUDIO_FORMAT_MAIN_MASK) {
    case AUDIO_FORMAT_MP3:
        id = SND_AUDIOCODEC_MP3;
        break;
    case AUDIO_FORMAT_AAC:
        id = SND_AUDIOCODEC_AAC;
        break;
    default:
        ALOGE("%s: Unsupported audio format", __func__);
    }

    return id;
}

int enable_audio_route(struct audio_device *adev,
                       struct audio_usecase *usecase)
{
    snd_device_t snd_device;
    char mixer_path[50];

    if (usecase == NULL)
        return -EINVAL;

    ALOGV("%s: enter: usecase(%d)", __func__, usecase->id);

    if (usecase->type == PCM_CAPTURE)
        snd_device = usecase->in_snd_device;
    else
        snd_device = usecase->out_snd_device;

    strcpy(mixer_path, use_case_table[usecase->id]);
    platform_add_backend_name(adev->platform, mixer_path, snd_device);
    ALOGD("%s: apply and update mixer path: %s", __func__, mixer_path);
    audio_route_apply_and_update_path(adev->audio_route, mixer_path);

    ALOGV("%s: exit", __func__);
    return 0;
}

int disable_audio_route(struct audio_device *adev,
                        struct audio_usecase *usecase)
{
    snd_device_t snd_device;
    char mixer_path[50];

    if (usecase == NULL)
        return -EINVAL;

    ALOGV("%s: enter: usecase(%d)", __func__, usecase->id);
    if (usecase->type == PCM_CAPTURE)
        snd_device = usecase->in_snd_device;
    else
        snd_device = usecase->out_snd_device;
    strcpy(mixer_path, use_case_table[usecase->id]);
    platform_add_backend_name(adev->platform, mixer_path, snd_device);
    ALOGD("%s: reset and update mixer path: %s", __func__, mixer_path);
    audio_route_reset_and_update_path(adev->audio_route, mixer_path);

    ALOGV("%s: exit", __func__);
    return 0;
}

int enable_snd_device(struct audio_device *adev,
                      snd_device_t snd_device)
{
    if (snd_device < SND_DEVICE_MIN ||
        snd_device >= SND_DEVICE_MAX) {
        ALOGE("%s: Invalid sound device %d", __func__, snd_device);
        return -EINVAL;
    }

    adev->snd_dev_ref_cnt[snd_device]++;
    if (adev->snd_dev_ref_cnt[snd_device] > 1) {
        ALOGV("%s: snd_device(%d: %s) is already active",
              __func__, snd_device, platform_get_snd_device_name(snd_device));
        return 0;
    }

    if (platform_send_audio_calibration(adev->platform, snd_device) < 0) {
        adev->snd_dev_ref_cnt[snd_device]--;
        return -EINVAL;
    }

    const char * dev_path = platform_get_snd_device_name(snd_device);
    ALOGD("%s: snd_device(%d: %s)", __func__, snd_device, dev_path);
    audio_route_apply_and_update_path(adev->audio_route, dev_path);

    return 0;
}

int disable_snd_device(struct audio_device *adev,
                       snd_device_t snd_device)
{
    if (snd_device < SND_DEVICE_MIN ||
        snd_device >= SND_DEVICE_MAX) {
        ALOGE("%s: Invalid sound device %d", __func__, snd_device);
        return -EINVAL;
    }
    if (adev->snd_dev_ref_cnt[snd_device] <= 0) {
        ALOGE("%s: device ref cnt is already 0", __func__);
        return -EINVAL;
    }
    adev->snd_dev_ref_cnt[snd_device]--;
    if (adev->snd_dev_ref_cnt[snd_device] == 0) {
        const char * dev_path = platform_get_snd_device_name(snd_device);
        ALOGD("%s: snd_device(%d: %s)", __func__, snd_device, dev_path);
        audio_route_reset_and_update_path(adev->audio_route, dev_path);
    }
    return 0;
}

static void check_usecases_codec_backend(struct audio_device *adev,
                                          struct audio_usecase *uc_info,
                                          snd_device_t snd_device)
{
    struct listnode *node;
    struct audio_usecase *usecase;
    bool switch_device[AUDIO_USECASE_MAX];
    int i, num_uc_to_switch = 0;

    /*
     * This function is to make sure that all the usecases that are active on
     * the hardware codec backend are always routed to any one device that is
     * handled by the hardware codec.
     * For example, if low-latency and deep-buffer usecases are currently active
     * on speaker and out_set_parameters(headset) is received on low-latency
     * output, then we have to make sure deep-buffer is also switched to headset,
     * because of the limitation that both the devices cannot be enabled
     * at the same time as they share the same backend.
     */
    /* Disable all the usecases on the shared backend other than the
       specified usecase */
    for (i = 0; i < AUDIO_USECASE_MAX; i++)
        switch_device[i] = false;

    list_for_each(node, &adev->usecase_list) {
        usecase = node_to_item(node, struct audio_usecase, list);
        if (usecase->type != PCM_CAPTURE &&
                usecase != uc_info &&
                usecase->out_snd_device != snd_device &&
                usecase->devices & AUDIO_DEVICE_OUT_ALL_CODEC_BACKEND) {
            ALOGV("%s: Usecase (%s) is active on (%s) - disabling ..",
                  __func__, use_case_table[usecase->id],
                  platform_get_snd_device_name(usecase->out_snd_device));
            disable_audio_route(adev, usecase);
            switch_device[usecase->id] = true;
            num_uc_to_switch++;
        }
    }

    if (num_uc_to_switch) {
        list_for_each(node, &adev->usecase_list) {
            usecase = node_to_item(node, struct audio_usecase, list);
            if (switch_device[usecase->id]) {
                disable_snd_device(adev, usecase->out_snd_device);
            }
        }

        list_for_each(node, &adev->usecase_list) {
            usecase = node_to_item(node, struct audio_usecase, list);
            if (switch_device[usecase->id]) {
                enable_snd_device(adev, snd_device);
            }
        }

        /* Re-route all the usecases on the shared backend other than the
           specified usecase to new snd devices */
        list_for_each(node, &adev->usecase_list) {
            usecase = node_to_item(node, struct audio_usecase, list);
            /* Update the out_snd_device only before enabling the audio route */
            if (switch_device[usecase->id] ) {
                usecase->out_snd_device = snd_device;
                enable_audio_route(adev, usecase);
            }
        }
    }
}

static void check_and_route_capture_usecases(struct audio_device *adev,
                                             struct audio_usecase *uc_info,
                                             snd_device_t snd_device)
{
    struct listnode *node;
    struct audio_usecase *usecase;
    bool switch_device[AUDIO_USECASE_MAX];
    int i, num_uc_to_switch = 0;

    /*
     * This function is to make sure that all the active capture usecases
     * are always routed to the same input sound device.
     * For example, if audio-record and voice-call usecases are currently
     * active on speaker(rx) and speaker-mic (tx) and out_set_parameters(earpiece)
     * is received for voice call then we have to make sure that audio-record
     * usecase is also switched to earpiece i.e. voice-dmic-ef,
     * because of the limitation that two devices cannot be enabled
     * at the same time if they share the same backend.
     */
    for (i = 0; i < AUDIO_USECASE_MAX; i++)
        switch_device[i] = false;

    list_for_each(node, &adev->usecase_list) {
        usecase = node_to_item(node, struct audio_usecase, list);
        if (usecase->type != PCM_PLAYBACK &&
                usecase != uc_info &&
                usecase->in_snd_device != snd_device) {
            ALOGV("%s: Usecase (%s) is active on (%s) - disabling ..",
                  __func__, use_case_table[usecase->id],
                  platform_get_snd_device_name(usecase->in_snd_device));
            disable_audio_route(adev, usecase);
            switch_device[usecase->id] = true;
            num_uc_to_switch++;
        }
    }

    if (num_uc_to_switch) {
        list_for_each(node, &adev->usecase_list) {
            usecase = node_to_item(node, struct audio_usecase, list);
            if (switch_device[usecase->id]) {
                disable_snd_device(adev, usecase->in_snd_device);
            }
        }

        list_for_each(node, &adev->usecase_list) {
            usecase = node_to_item(node, struct audio_usecase, list);
            if (switch_device[usecase->id]) {
                enable_snd_device(adev, snd_device);
            }
        }

        /* Re-route all the usecases on the shared backend other than the
           specified usecase to new snd devices */
        list_for_each(node, &adev->usecase_list) {
            usecase = node_to_item(node, struct audio_usecase, list);
            /* Update the in_snd_device only before enabling the audio route */
            if (switch_device[usecase->id] ) {
                usecase->in_snd_device = snd_device;
                enable_audio_route(adev, usecase);
            }
        }
    }
}

/* must be called with hw device mutex locked */
static int read_hdmi_channel_masks(struct stream_out *out)
{
    int ret = 0;
    int channels = platform_edid_get_max_channels(out->dev->platform);

    switch (channels) {
        /*
         * Do not handle stereo output in Multi-channel cases
         * Stereo case is handled in normal playback path
         */
    case 6:
        ALOGV("%s: HDMI supports 5.1", __func__);
        out->supported_channel_masks[0] = AUDIO_CHANNEL_OUT_5POINT1;
        break;
    case 8:
        ALOGV("%s: HDMI supports 5.1 and 7.1 channels", __func__);
        out->supported_channel_masks[0] = AUDIO_CHANNEL_OUT_5POINT1;
        out->supported_channel_masks[1] = AUDIO_CHANNEL_OUT_7POINT1;
        break;
    default:
        ALOGE("HDMI does not support multi channel playback");
        ret = -ENOSYS;
        break;
    }
    return ret;
}

static audio_usecase_t get_voice_usecase_id_from_list(struct audio_device *adev)
{
    struct audio_usecase *usecase;
    struct listnode *node;

    list_for_each(node, &adev->usecase_list) {
        usecase = node_to_item(node, struct audio_usecase, list);
        if (usecase->type == VOICE_CALL) {
            ALOGV("%s: usecase id %d", __func__, usecase->id);
            return usecase->id;
        }
    }
    return USECASE_INVALID;
}

struct audio_usecase *get_usecase_from_list(struct audio_device *adev,
                                            audio_usecase_t uc_id)
{
    struct audio_usecase *usecase;
    struct listnode *node;

    list_for_each(node, &adev->usecase_list) {
        usecase = node_to_item(node, struct audio_usecase, list);
        if (usecase->id == uc_id)
            return usecase;
    }
    return NULL;
}

int select_devices(struct audio_device *adev,
                   audio_usecase_t uc_id)
{
    snd_device_t out_snd_device = SND_DEVICE_NONE;
    snd_device_t in_snd_device = SND_DEVICE_NONE;
    struct audio_usecase *usecase = NULL;
    struct audio_usecase *vc_usecase = NULL;
    struct audio_usecase *hfp_usecase = NULL;
    audio_usecase_t hfp_ucid;
    struct listnode *node;
    int status = 0;

    usecase = get_usecase_from_list(adev, uc_id);
    if (usecase == NULL) {
        ALOGE("%s: Could not find the usecase(%d)", __func__, uc_id);
        return -EINVAL;
    }

    if ((usecase->type == VOICE_CALL) ||
        (usecase->type == PCM_HFP_CALL)) {
        out_snd_device = platform_get_output_snd_device(adev->platform,
                                                        usecase->stream.out->devices);
        in_snd_device = platform_get_input_snd_device(adev->platform, usecase->stream.out->devices);
        usecase->devices = usecase->stream.out->devices;
    } else {
        /*
         * If the voice call is active, use the sound devices of voice call usecase
         * so that it would not result any device switch. All the usecases will
         * be switched to new device when select_devices() is called for voice call
         * usecase. This is to avoid switching devices for voice call when
         * check_usecases_codec_backend() is called below.
         */
        if (voice_is_in_call(adev)) {
            vc_usecase = get_usecase_from_list(adev,
                                               get_voice_usecase_id_from_list(adev));
            if ((vc_usecase != NULL) &&
                ((vc_usecase->devices & AUDIO_DEVICE_OUT_ALL_CODEC_BACKEND) ||
                (usecase->devices == AUDIO_DEVICE_IN_VOICE_CALL))) {
                in_snd_device = vc_usecase->in_snd_device;
                out_snd_device = vc_usecase->out_snd_device;
            }
        } else if (audio_extn_hfp_is_active(adev)) {
            hfp_ucid = audio_extn_hfp_get_usecase();
            hfp_usecase = get_usecase_from_list(adev, hfp_ucid);
            if (hfp_usecase->devices & AUDIO_DEVICE_OUT_ALL_CODEC_BACKEND) {
                   in_snd_device = hfp_usecase->in_snd_device;
                   out_snd_device = hfp_usecase->out_snd_device;
            }
        }
        if (usecase->type == PCM_PLAYBACK) {
            usecase->devices = usecase->stream.out->devices;
            in_snd_device = SND_DEVICE_NONE;
            if (out_snd_device == SND_DEVICE_NONE) {
                out_snd_device = platform_get_output_snd_device(adev->platform,
                                            usecase->stream.out->devices);
                if (usecase->stream.out == adev->primary_output &&
                        adev->active_input &&
                        adev->active_input->source == AUDIO_SOURCE_VOICE_COMMUNICATION &&
                        out_snd_device != usecase->out_snd_device) {
                    select_devices(adev, adev->active_input->usecase);
                }
            }
        } else if (usecase->type == PCM_CAPTURE) {
            usecase->devices = usecase->stream.in->device;
            out_snd_device = SND_DEVICE_NONE;
            if (in_snd_device == SND_DEVICE_NONE) {
                audio_devices_t out_device = AUDIO_DEVICE_NONE;
                if (adev->active_input->source == AUDIO_SOURCE_VOICE_COMMUNICATION &&
                        adev->primary_output && !adev->primary_output->standby) {
                    out_device = adev->primary_output->devices;
                    platform_set_echo_reference(adev, false, AUDIO_DEVICE_NONE);
                } else if (usecase->id == USECASE_AUDIO_RECORD_AFE_PROXY) {
                    out_device = AUDIO_DEVICE_OUT_TELEPHONY_TX;
                }
                in_snd_device = platform_get_input_snd_device(adev->platform, out_device);
            }
        }
    }

    if (out_snd_device == usecase->out_snd_device &&
        in_snd_device == usecase->in_snd_device) {
        return 0;
    }

    ALOGD("%s: out_snd_device(%d: %s) in_snd_device(%d: %s)", __func__,
          out_snd_device, platform_get_snd_device_name(out_snd_device),
          in_snd_device,  platform_get_snd_device_name(in_snd_device));

    /*
     * Limitation: While in call, to do a device switch we need to disable
     * and enable both RX and TX devices though one of them is same as current
     * device.
     */
    if ((usecase->type == VOICE_CALL) &&
        (usecase->in_snd_device != SND_DEVICE_NONE) &&
        (usecase->out_snd_device != SND_DEVICE_NONE)) {
        status = platform_switch_voice_call_device_pre(adev->platform);
    }

    /* Disable current sound devices */
    if (usecase->out_snd_device != SND_DEVICE_NONE) {
        disable_audio_route(adev, usecase);
        disable_snd_device(adev, usecase->out_snd_device);
    }

    if (usecase->in_snd_device != SND_DEVICE_NONE) {
        disable_audio_route(adev, usecase);
        disable_snd_device(adev, usecase->in_snd_device);
    }

    /* Applicable only on the targets that has external modem.
     * New device information should be sent to modem before enabling
     * the devices to reduce in-call device switch time.
     */
    if ((usecase->type == VOICE_CALL) &&
        (usecase->in_snd_device != SND_DEVICE_NONE) &&
        (usecase->out_snd_device != SND_DEVICE_NONE)) {
        status = platform_switch_voice_call_enable_device_config(adev->platform,
                                                                 out_snd_device,
                                                                 in_snd_device);
    }

    /* Enable new sound devices */
    if (out_snd_device != SND_DEVICE_NONE) {
        if (usecase->devices & AUDIO_DEVICE_OUT_ALL_CODEC_BACKEND)
            check_usecases_codec_backend(adev, usecase, out_snd_device);
        enable_snd_device(adev, out_snd_device);
    }

    if (in_snd_device != SND_DEVICE_NONE) {
        check_and_route_capture_usecases(adev, usecase, in_snd_device);
        enable_snd_device(adev, in_snd_device);
    }

    if (usecase->type == VOICE_CALL)
        status = platform_switch_voice_call_device_post(adev->platform,
                                                        out_snd_device,
                                                        in_snd_device);

    usecase->in_snd_device = in_snd_device;
    usecase->out_snd_device = out_snd_device;

    enable_audio_route(adev, usecase);

    /* Applicable only on the targets that has external modem.
     * Enable device command should be sent to modem only after
     * enabling voice call mixer controls
     */
    if (usecase->type == VOICE_CALL)
        status = platform_switch_voice_call_usecase_route_post(adev->platform,
                                                               out_snd_device,
                                                               in_snd_device);

    return status;
}

static int stop_input_stream(struct stream_in *in)
{
    int i, ret = 0;
    struct audio_usecase *uc_info;
    struct audio_device *adev = in->dev;

    adev->active_input = NULL;

    ALOGV("%s: enter: usecase(%d: %s)", __func__,
          in->usecase, use_case_table[in->usecase]);
    uc_info = get_usecase_from_list(adev, in->usecase);
    if (uc_info == NULL) {
        ALOGE("%s: Could not find the usecase (%d) in the list",
              __func__, in->usecase);
        return -EINVAL;
    }

    /* 1. Disable stream specific mixer controls */
    disable_audio_route(adev, uc_info);

    /* 2. Disable the tx device */
    disable_snd_device(adev, uc_info->in_snd_device);

    list_remove(&uc_info->list);
    free(uc_info);

    ALOGV("%s: exit: status(%d)", __func__, ret);
    return ret;
}

int start_input_stream(struct stream_in *in)
{
    /* 1. Enable output device and stream routing controls */
    int ret = 0;
    struct audio_usecase *uc_info;
    struct audio_device *adev = in->dev;

    ALOGV("%s: enter: usecase(%d)", __func__, in->usecase);
    in->pcm_device_id = platform_get_pcm_device_id(in->usecase, PCM_CAPTURE);
    if (in->pcm_device_id < 0) {
        ALOGE("%s: Could not find PCM device id for the usecase(%d)",
              __func__, in->usecase);
        ret = -EINVAL;
        goto error_config;
    }

    adev->active_input = in;
    uc_info = (struct audio_usecase *)calloc(1, sizeof(struct audio_usecase));
    uc_info->id = in->usecase;
    uc_info->type = PCM_CAPTURE;
    uc_info->stream.in = in;
    uc_info->devices = in->device;
    uc_info->in_snd_device = SND_DEVICE_NONE;
    uc_info->out_snd_device = SND_DEVICE_NONE;

    list_add_tail(&adev->usecase_list, &uc_info->list);
    select_devices(adev, in->usecase);

    ALOGV("%s: Opening PCM device card_id(%d) device_id(%d), channels %d",
          __func__, adev->snd_card, in->pcm_device_id, in->config.channels);

    unsigned int flags = PCM_IN;
    unsigned int pcm_open_retry_count = 0;

    if (in->usecase == USECASE_AUDIO_RECORD_AFE_PROXY) {
        flags |= PCM_MMAP | PCM_NOIRQ;
        pcm_open_retry_count = PROXY_OPEN_RETRY_COUNT;
    }

    while (1) {
        in->pcm = pcm_open(adev->snd_card, in->pcm_device_id,
                           flags, &in->config);
        if (in->pcm == NULL || !pcm_is_ready(in->pcm)) {
            ALOGE("%s: %s", __func__, pcm_get_error(in->pcm));
            if (in->pcm != NULL) {
                pcm_close(in->pcm);
                in->pcm = NULL;
            }
            if (pcm_open_retry_count-- == 0) {
                ret = -EIO;
                goto error_open;
            }
            usleep(PROXY_OPEN_WAIT_TIME * 1000);
            continue;
        }
        break;
    }

    ALOGV("%s: exit", __func__);
    return ret;

error_open:
    stop_input_stream(in);

error_config:
    adev->active_input = NULL;
    ALOGD("%s: exit: status(%d)", __func__, ret);

    return ret;
}

/* must be called with out->lock locked */
static int send_offload_cmd_l(struct stream_out* out, int command)
{
    struct offload_cmd *cmd = (struct offload_cmd *)calloc(1, sizeof(struct offload_cmd));

    ALOGVV("%s %d", __func__, command);

    cmd->cmd = command;
    list_add_tail(&out->offload_cmd_list, &cmd->node);
    pthread_cond_signal(&out->offload_cond);
    return 0;
}

/* must be called iwth out->lock locked */
static void stop_compressed_output_l(struct stream_out *out)
{
    out->offload_state = OFFLOAD_STATE_IDLE;
    out->playback_started = 0;
    out->send_new_metadata = 1;
    if (out->compr != NULL) {
        compress_stop(out->compr);
        while (out->offload_thread_blocked) {
            pthread_cond_wait(&out->cond, &out->lock);
        }
    }
}

static void *offload_thread_loop(void *context)
{
    struct stream_out *out = (struct stream_out *) context;
    struct listnode *item;

    out->offload_state = OFFLOAD_STATE_IDLE;
    out->playback_started = 0;

    setpriority(PRIO_PROCESS, 0, ANDROID_PRIORITY_AUDIO);
    set_sched_policy(0, SP_FOREGROUND);
    prctl(PR_SET_NAME, (unsigned long)"Offload Callback", 0, 0, 0);

    ALOGV("%s", __func__);
    pthread_mutex_lock(&out->lock);
    for (;;) {
        struct offload_cmd *cmd = NULL;
        stream_callback_event_t event;
        bool send_callback = false;

        ALOGVV("%s offload_cmd_list %d out->offload_state %d",
              __func__, list_empty(&out->offload_cmd_list),
              out->offload_state);
        if (list_empty(&out->offload_cmd_list)) {
            ALOGV("%s SLEEPING", __func__);
            pthread_cond_wait(&out->offload_cond, &out->lock);
            ALOGV("%s RUNNING", __func__);
            continue;
        }

        item = list_head(&out->offload_cmd_list);
        cmd = node_to_item(item, struct offload_cmd, node);
        list_remove(item);

        ALOGVV("%s STATE %d CMD %d out->compr %p",
               __func__, out->offload_state, cmd->cmd, out->compr);

        if (cmd->cmd == OFFLOAD_CMD_EXIT) {
            free(cmd);
            break;
        }

        if (out->compr == NULL) {
            ALOGE("%s: Compress handle is NULL", __func__);
            pthread_cond_signal(&out->cond);
            continue;
        }
        out->offload_thread_blocked = true;
        pthread_mutex_unlock(&out->lock);
        send_callback = false;
        switch(cmd->cmd) {
        case OFFLOAD_CMD_WAIT_FOR_BUFFER:
            compress_wait(out->compr, -1);
            send_callback = true;
            event = STREAM_CBK_EVENT_WRITE_READY;
            break;
        case OFFLOAD_CMD_PARTIAL_DRAIN:
            compress_next_track(out->compr);
            compress_partial_drain(out->compr);
            send_callback = true;
            event = STREAM_CBK_EVENT_DRAIN_READY;
            /* Resend the metadata for next iteration */
            out->send_new_metadata = 1;
            break;
        case OFFLOAD_CMD_DRAIN:
            compress_drain(out->compr);
            send_callback = true;
            event = STREAM_CBK_EVENT_DRAIN_READY;
            break;
        default:
            ALOGE("%s unknown command received: %d", __func__, cmd->cmd);
            break;
        }
        pthread_mutex_lock(&out->lock);
        out->offload_thread_blocked = false;
        pthread_cond_signal(&out->cond);
        if (send_callback) {
            ALOGVV("%s: sending offload_callback event %d", __func__, event);
            out->offload_callback(event, NULL, out->offload_cookie);
        }
        free(cmd);
    }

    pthread_cond_signal(&out->cond);
    while (!list_empty(&out->offload_cmd_list)) {
        item = list_head(&out->offload_cmd_list);
        list_remove(item);
        free(node_to_item(item, struct offload_cmd, node));
    }
    pthread_mutex_unlock(&out->lock);

    return NULL;
}

static int create_offload_callback_thread(struct stream_out *out)
{
    pthread_cond_init(&out->offload_cond, (const pthread_condattr_t *) NULL);
    list_init(&out->offload_cmd_list);
    pthread_create(&out->offload_thread, (const pthread_attr_t *) NULL,
                    offload_thread_loop, out);
    return 0;
}

static int destroy_offload_callback_thread(struct stream_out *out)
{
    pthread_mutex_lock(&out->lock);
    stop_compressed_output_l(out);
    send_offload_cmd_l(out, OFFLOAD_CMD_EXIT);

    pthread_mutex_unlock(&out->lock);
    pthread_join(out->offload_thread, (void **) NULL);
    pthread_cond_destroy(&out->offload_cond);

    return 0;
}

static bool allow_hdmi_channel_config(struct audio_device *adev)
{
    struct listnode *node;
    struct audio_usecase *usecase;
    bool ret = true;

    list_for_each(node, &adev->usecase_list) {
        usecase = node_to_item(node, struct audio_usecase, list);
        if (usecase->devices & AUDIO_DEVICE_OUT_AUX_DIGITAL) {
            /*
             * If voice call is already existing, do not proceed further to avoid
             * disabling/enabling both RX and TX devices, CSD calls, etc.
             * Once the voice call done, the HDMI channels can be configured to
             * max channels of remaining use cases.
             */
            if (usecase->id == USECASE_VOICE_CALL) {
                ALOGD("%s: voice call is active, no change in HDMI channels",
                      __func__);
                ret = false;
                break;
            } else if (usecase->id == USECASE_AUDIO_PLAYBACK_MULTI_CH) {
                ALOGD("%s: multi channel playback is active, "
                      "no change in HDMI channels", __func__);
                ret = false;
                break;
            }
        }
    }
    return ret;
}

static int check_and_set_hdmi_channels(struct audio_device *adev,
                                       unsigned int channels)
{
    struct listnode *node;
    struct audio_usecase *usecase;

    /* Check if change in HDMI channel config is allowed */
    if (!allow_hdmi_channel_config(adev))
        return 0;

    if (channels == adev->cur_hdmi_channels) {
        ALOGD("%s: Requested channels are same as current", __func__);
        return 0;
    }

    platform_set_hdmi_channels(adev->platform, channels);
    adev->cur_hdmi_channels = channels;

    /*
     * Deroute all the playback streams routed to HDMI so that
     * the back end is deactivated. Note that backend will not
     * be deactivated if any one stream is connected to it.
     */
    list_for_each(node, &adev->usecase_list) {
        usecase = node_to_item(node, struct audio_usecase, list);
        if (usecase->type == PCM_PLAYBACK &&
                usecase->devices & AUDIO_DEVICE_OUT_AUX_DIGITAL) {
            disable_audio_route(adev, usecase);
        }
    }

    /*
     * Enable all the streams disabled above. Now the HDMI backend
     * will be activated with new channel configuration
     */
    list_for_each(node, &adev->usecase_list) {
        usecase = node_to_item(node, struct audio_usecase, list);
        if (usecase->type == PCM_PLAYBACK &&
                usecase->devices & AUDIO_DEVICE_OUT_AUX_DIGITAL) {
            enable_audio_route(adev, usecase);
        }
    }

    return 0;
}

static int stop_output_stream(struct stream_out *out)
{
    int i, ret = 0;
    struct audio_usecase *uc_info;
    struct audio_device *adev = out->dev;

    ALOGV("%s: enter: usecase(%d: %s)", __func__,
          out->usecase, use_case_table[out->usecase]);
    uc_info = get_usecase_from_list(adev, out->usecase);
    if (uc_info == NULL) {
        ALOGE("%s: Could not find the usecase (%d) in the list",
              __func__, out->usecase);
        return -EINVAL;
    }

    if (out->usecase == USECASE_AUDIO_PLAYBACK_OFFLOAD) {
        if (adev->visualizer_stop_output != NULL)
            adev->visualizer_stop_output(out->handle, out->pcm_device_id);
        if (adev->offload_effects_stop_output != NULL)
            adev->offload_effects_stop_output(out->handle, out->pcm_device_id);
    }

    /* 1. Get and set stream specific mixer controls */
    disable_audio_route(adev, uc_info);

    /* 2. Disable the rx device */
    disable_snd_device(adev, uc_info->out_snd_device);

    list_remove(&uc_info->list);
    free(uc_info);

    audio_extn_extspk_update(adev->extspk);

    /* Must be called after removing the usecase from list */
    if (out->devices & AUDIO_DEVICE_OUT_AUX_DIGITAL)
        check_and_set_hdmi_channels(adev, DEFAULT_HDMI_OUT_CHANNELS);

    ALOGV("%s: exit: status(%d)", __func__, ret);
    return ret;
}

int start_output_stream(struct stream_out *out)
{
    int ret = 0;
    struct audio_usecase *uc_info;
    struct audio_device *adev = out->dev;

    ALOGV("%s: enter: usecase(%d: %s) devices(%#x)",
          __func__, out->usecase, use_case_table[out->usecase], out->devices);
    out->pcm_device_id = platform_get_pcm_device_id(out->usecase, PCM_PLAYBACK);
    if (out->pcm_device_id < 0) {
        ALOGE("%s: Invalid PCM device id(%d) for the usecase(%d)",
              __func__, out->pcm_device_id, out->usecase);
        ret = -EINVAL;
        goto error_config;
    }

    uc_info = (struct audio_usecase *)calloc(1, sizeof(struct audio_usecase));
    uc_info->id = out->usecase;
    uc_info->type = PCM_PLAYBACK;
    uc_info->stream.out = out;
    uc_info->devices = out->devices;
    uc_info->in_snd_device = SND_DEVICE_NONE;
    uc_info->out_snd_device = SND_DEVICE_NONE;

    /* This must be called before adding this usecase to the list */
    if (out->devices & AUDIO_DEVICE_OUT_AUX_DIGITAL)
        check_and_set_hdmi_channels(adev, out->config.channels);

    list_add_tail(&adev->usecase_list, &uc_info->list);

    select_devices(adev, out->usecase);

    audio_extn_extspk_update(adev->extspk);

    ALOGV("%s: Opening PCM device card_id(%d) device_id(%d) format(%#x)",
          __func__, adev->snd_card, out->pcm_device_id, out->config.format);
    if (out->usecase != USECASE_AUDIO_PLAYBACK_OFFLOAD) {
        unsigned int flags = PCM_OUT;
        unsigned int pcm_open_retry_count = 0;
        if (out->usecase == USECASE_AUDIO_PLAYBACK_AFE_PROXY) {
            flags |= PCM_MMAP | PCM_NOIRQ;
            pcm_open_retry_count = PROXY_OPEN_RETRY_COUNT;
        } else
            flags |= PCM_MONOTONIC;

        while (1) {
            out->pcm = pcm_open(adev->snd_card, out->pcm_device_id,
                               flags, &out->config);
            if (out->pcm == NULL || !pcm_is_ready(out->pcm)) {
                ALOGE("%s: %s", __func__, pcm_get_error(out->pcm));
                if (out->pcm != NULL) {
                    pcm_close(out->pcm);
                    out->pcm = NULL;
                }
                if (pcm_open_retry_count-- == 0) {
                    ret = -EIO;
                    goto error_open;
                }
                usleep(PROXY_OPEN_WAIT_TIME * 1000);
                continue;
            }
            break;
        }
    } else {
        out->pcm = NULL;
        out->compr = compress_open(adev->snd_card, out->pcm_device_id,
                                   COMPRESS_IN, &out->compr_config);
        if (out->compr && !is_compress_ready(out->compr)) {
            ALOGE("%s: %s", __func__, compress_get_error(out->compr));
            compress_close(out->compr);
            out->compr = NULL;
            ret = -EIO;
            goto error_open;
        }
        if (out->offload_callback)
            compress_nonblock(out->compr, out->non_blocking);

        if (adev->visualizer_start_output != NULL)
            adev->visualizer_start_output(out->handle, out->pcm_device_id);
        if (adev->offload_effects_start_output != NULL)
            adev->offload_effects_start_output(out->handle, out->pcm_device_id);
    }
    ALOGV("%s: exit", __func__);
    return 0;
error_open:
    stop_output_stream(out);
error_config:
    return ret;
}

static int check_input_parameters(uint32_t sample_rate,
                                  audio_format_t format,
                                  int channel_count)
{
    if (format != AUDIO_FORMAT_PCM_16_BIT) return -EINVAL;

    if ((channel_count < 1) || (channel_count > 2)) return -EINVAL;

    switch (sample_rate) {
    case 8000:
    case 11025:
    case 12000:
    case 16000:
    case 22050:
    case 24000:
    case 32000:
    case 44100:
    case 48000:
        break;
    default:
        return -EINVAL;
    }

    return 0;
}

static size_t get_input_buffer_size(uint32_t sample_rate,
                                    audio_format_t format,
                                    int channel_count,
                                    bool is_low_latency)
{
    size_t size = 0;

    if (check_input_parameters(sample_rate, format, channel_count) != 0)
        return 0;

    size = (sample_rate * AUDIO_CAPTURE_PERIOD_DURATION_MSEC) / 1000;
    if (is_low_latency)
        size = configured_low_latency_capture_period_size;
    /* ToDo: should use frame_size computed based on the format and
       channel_count here. */
    size *= sizeof(short) * channel_count;

    /* make sure the size is multiple of 32 bytes
     * At 48 kHz mono 16-bit PCM:
     *  5.000 ms = 240 frames = 15*16*1*2 = 480, a whole multiple of 32 (15)
     *  3.333 ms = 160 frames = 10*16*1*2 = 320, a whole multiple of 32 (10)
     */
    size += 0x1f;
    size &= ~0x1f;

    return size;
}

static uint32_t out_get_sample_rate(const struct audio_stream *stream)
{
    struct stream_out *out = (struct stream_out *)stream;

    return out->sample_rate;
}

static int out_set_sample_rate(struct audio_stream *stream __unused, uint32_t rate __unused)
{
    return -ENOSYS;
}

static size_t out_get_buffer_size(const struct audio_stream *stream)
{
    struct stream_out *out = (struct stream_out *)stream;

    if (out->usecase == USECASE_AUDIO_PLAYBACK_OFFLOAD) {
        return out->compr_config.fragment_size;
    }
    return out->config.period_size *
                audio_stream_out_frame_size((const struct audio_stream_out *)stream);
}

static uint32_t out_get_channels(const struct audio_stream *stream)
{
    struct stream_out *out = (struct stream_out *)stream;

    return out->channel_mask;
}

static audio_format_t out_get_format(const struct audio_stream *stream)
{
    struct stream_out *out = (struct stream_out *)stream;

    return out->format;
}

static int out_set_format(struct audio_stream *stream __unused, audio_format_t format __unused)
{
    return -ENOSYS;
}

static int out_standby(struct audio_stream *stream)
{
    struct stream_out *out = (struct stream_out *)stream;
    struct audio_device *adev = out->dev;

    ALOGV("%s: enter: usecase(%d: %s)", __func__,
          out->usecase, use_case_table[out->usecase]);

    pthread_mutex_lock(&out->lock);
    if (!out->standby) {
        pthread_mutex_lock(&adev->lock);
        out->standby = true;
        if (out->usecase != USECASE_AUDIO_PLAYBACK_OFFLOAD) {
            if (out->pcm) {
                pcm_close(out->pcm);
                out->pcm = NULL;
            }
        } else {
            stop_compressed_output_l(out);
            out->gapless_mdata.encoder_delay = 0;
            out->gapless_mdata.encoder_padding = 0;
            if (out->compr != NULL) {
                compress_close(out->compr);
                out->compr = NULL;
            }
        }
        stop_output_stream(out);
        pthread_mutex_unlock(&adev->lock);
    }
    pthread_mutex_unlock(&out->lock);
    ALOGV("%s: exit", __func__);
    return 0;
}

static int out_dump(const struct audio_stream *stream __unused, int fd __unused)
{
    return 0;
}

static int parse_compress_metadata(struct stream_out *out, struct str_parms *parms)
{
    int ret = 0;
    char value[32];
    struct compr_gapless_mdata tmp_mdata;

    if (!out || !parms) {
        return -EINVAL;
    }

    ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_DELAY_SAMPLES, value, sizeof(value));
    if (ret >= 0) {
        tmp_mdata.encoder_delay = atoi(value); //whats a good limit check?
    } else {
        return -EINVAL;
    }

    ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_PADDING_SAMPLES, value, sizeof(value));
    if (ret >= 0) {
        tmp_mdata.encoder_padding = atoi(value);
    } else {
        return -EINVAL;
    }

    out->gapless_mdata = tmp_mdata;
    out->send_new_metadata = 1;
    ALOGV("%s new encoder delay %u and padding %u", __func__,
          out->gapless_mdata.encoder_delay, out->gapless_mdata.encoder_padding);

    return 0;
}

static bool output_drives_call(struct audio_device *adev, struct stream_out *out)
{
    return out == adev->primary_output || out == adev->voice_tx_output;
}

static int out_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    struct stream_out *out = (struct stream_out *)stream;
    struct audio_device *adev = out->dev;
    struct audio_usecase *usecase;
    struct listnode *node;
    struct str_parms *parms;
    char value[32];
    int ret, val = 0;
    bool select_new_device = false;
    int status = 0;

    ALOGD("%s: enter: usecase(%d: %s) kvpairs: %s",
          __func__, out->usecase, use_case_table[out->usecase], kvpairs);
    parms = str_parms_create_str(kvpairs);
    ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_ROUTING, value, sizeof(value));
    if (ret >= 0) {
        val = atoi(value);
        pthread_mutex_lock(&out->lock);
        pthread_mutex_lock(&adev->lock);

        /*
         * When HDMI cable is unplugged the music playback is paused and
         * the policy manager sends routing=0. But the audioflinger
         * continues to write data until standby time (3sec).
         * As the HDMI core is turned off, the write gets blocked.
         * Avoid this by routing audio to speaker until standby.
         */
        if (out->devices == AUDIO_DEVICE_OUT_AUX_DIGITAL &&
                val == AUDIO_DEVICE_NONE) {
            val = AUDIO_DEVICE_OUT_SPEAKER;
        }

        /*
         * select_devices() call below switches all the usecases on the same
         * backend to the new device. Refer to check_usecases_codec_backend() in
         * the select_devices(). But how do we undo this?
         *
         * For example, music playback is active on headset (deep-buffer usecase)
         * and if we go to ringtones and select a ringtone, low-latency usecase
         * will be started on headset+speaker. As we can't enable headset+speaker
         * and headset devices at the same time, select_devices() switches the music
         * playback to headset+speaker while starting low-lateny usecase for ringtone.
         * So when the ringtone playback is completed, how do we undo the same?
         *
         * We are relying on the out_set_parameters() call on deep-buffer output,
         * once the ringtone playback is ended.
         * NOTE: We should not check if the current devices are same as new devices.
         *       Because select_devices() must be called to switch back the music
         *       playback to headset.
         */
        if (val != 0) {
            out->devices = val;

            if (!out->standby)
                select_devices(adev, out->usecase);

            if (output_drives_call(adev, out)) {
                if (!voice_is_in_call(adev)) {
                    if (adev->mode == AUDIO_MODE_IN_CALL) {
                        adev->current_call_output = out;
                        ret = voice_start_call(adev);
                    }
                } else {
                    adev->current_call_output = out;
                    voice_update_devices_for_all_voice_usecases(adev);
                }
            }
        }

        pthread_mutex_unlock(&adev->lock);
        pthread_mutex_unlock(&out->lock);

        /*handles device and call state changes*/
        audio_extn_extspk_update(adev->extspk);
    }

    if (out->usecase == USECASE_AUDIO_PLAYBACK_OFFLOAD) {
        parse_compress_metadata(out, parms);
    }

    str_parms_destroy(parms);
    ALOGV("%s: exit: code(%d)", __func__, status);
    return status;
}

static char* out_get_parameters(const struct audio_stream *stream, const char *keys)
{
    struct stream_out *out = (struct stream_out *)stream;
    struct str_parms *query = str_parms_create_str(keys);
    char *str;
    char value[256];
    struct str_parms *reply = str_parms_create();
    size_t i, j;
    int ret;
    bool first = true;
    ALOGV("%s: enter: keys - %s", __func__, keys);
    ret = str_parms_get_str(query, AUDIO_PARAMETER_STREAM_SUP_CHANNELS, value, sizeof(value));
    if (ret >= 0) {
        value[0] = '\0';
        i = 0;
        while (out->supported_channel_masks[i] != 0) {
            for (j = 0; j < ARRAY_SIZE(out_channels_name_to_enum_table); j++) {
                if (out_channels_name_to_enum_table[j].value == out->supported_channel_masks[i]) {
                    if (!first) {
                        strcat(value, "|");
                    }
                    strcat(value, out_channels_name_to_enum_table[j].name);
                    first = false;
                    break;
                }
            }
            i++;
        }
        str_parms_add_str(reply, AUDIO_PARAMETER_STREAM_SUP_CHANNELS, value);
        str = str_parms_to_str(reply);
    } else {
        str = strdup(keys);
    }
    str_parms_destroy(query);
    str_parms_destroy(reply);
    ALOGV("%s: exit: returns - %s", __func__, str);
    return str;
}

static uint32_t out_get_latency(const struct audio_stream_out *stream)
{
    struct stream_out *out = (struct stream_out *)stream;

    if (out->usecase == USECASE_AUDIO_PLAYBACK_OFFLOAD)
        return COMPRESS_OFFLOAD_PLAYBACK_LATENCY;

    return (out->config.period_count * out->config.period_size * 1000) /
           (out->config.rate);
}

static int out_set_volume(struct audio_stream_out *stream, float left,
                          float right)
{
    struct stream_out *out = (struct stream_out *)stream;
    int volume[2];

    if (out->usecase == USECASE_AUDIO_PLAYBACK_MULTI_CH) {
        /* only take left channel into account: the API is for stereo anyway */
        out->muted = (left == 0.0f);
        return 0;
    } else if (out->usecase == USECASE_AUDIO_PLAYBACK_OFFLOAD) {
        const char *mixer_ctl_name = "Compress Playback Volume";
        struct audio_device *adev = out->dev;
        struct mixer_ctl *ctl;
        ctl = mixer_get_ctl_by_name(adev->mixer, mixer_ctl_name);
        if (!ctl) {
            /* try with the control based on device id */
            int pcm_device_id = platform_get_pcm_device_id(out->usecase,
                                                       PCM_PLAYBACK);
            char ctl_name[128] = {0};
            snprintf(ctl_name, sizeof(ctl_name),
                     "Compress Playback %d Volume", pcm_device_id);
            ctl = mixer_get_ctl_by_name(adev->mixer, ctl_name);
            if (!ctl) {
                ALOGE("%s: Could not get volume ctl mixer cmd", __func__);
                return -EINVAL;
            }
        }
        volume[0] = (int)(left * COMPRESS_PLAYBACK_VOLUME_MAX);
        volume[1] = (int)(right * COMPRESS_PLAYBACK_VOLUME_MAX);
        mixer_ctl_set_array(ctl, volume, sizeof(volume)/sizeof(volume[0]));
        return 0;
    }

    return -ENOSYS;
}

static ssize_t out_write(struct audio_stream_out *stream, const void *buffer,
                         size_t bytes)
{
    struct stream_out *out = (struct stream_out *)stream;
    struct audio_device *adev = out->dev;
    ssize_t ret = 0;

    pthread_mutex_lock(&out->lock);
    if (out->standby) {
        out->standby = false;
        pthread_mutex_lock(&adev->lock);
        ret = start_output_stream(out);
        pthread_mutex_unlock(&adev->lock);
        /* ToDo: If use case is compress offload should return 0 */
        if (ret != 0) {
            out->standby = true;
            goto exit;
        }
    }

    if (out->usecase == USECASE_AUDIO_PLAYBACK_OFFLOAD) {
        ALOGVV("%s: writing buffer (%d bytes) to compress device", __func__, bytes);
        if (out->send_new_metadata) {
            ALOGVV("send new gapless metadata");
            compress_set_gapless_metadata(out->compr, &out->gapless_mdata);
            out->send_new_metadata = 0;
        }

        ret = compress_write(out->compr, buffer, bytes);
        ALOGVV("%s: writing buffer (%d bytes) to compress device returned %d", __func__, bytes, ret);
        if (ret >= 0 && ret < (ssize_t)bytes) {
            send_offload_cmd_l(out, OFFLOAD_CMD_WAIT_FOR_BUFFER);
        }
        if (!out->playback_started) {
            compress_start(out->compr);
            out->playback_started = 1;
            out->offload_state = OFFLOAD_STATE_PLAYING;
        }
        pthread_mutex_unlock(&out->lock);
        return ret;
    } else {
        if (out->pcm) {
            if (out->muted)
                memset((void *)buffer, 0, bytes);
            ALOGVV("%s: writing buffer (%d bytes) to pcm device", __func__, bytes);
            if (out->usecase == USECASE_AUDIO_PLAYBACK_AFE_PROXY) {
                ret = pcm_mmap_write(out->pcm, (void *)buffer, bytes);
            }
            else
                ret = pcm_write(out->pcm, (void *)buffer, bytes);
            if (ret == 0)
                out->written += bytes / (out->config.channels * sizeof(short));
        }
    }

exit:
    pthread_mutex_unlock(&out->lock);

    if (ret != 0) {
        if (out->pcm)
            ALOGE("%s: error %d - %s", __func__, ret, pcm_get_error(out->pcm));
        out_standby(&out->stream.common);
        usleep(bytes * 1000000 / audio_stream_out_frame_size(stream) /
               out_get_sample_rate(&out->stream.common));
    }
    return bytes;
}

static int out_get_render_position(const struct audio_stream_out *stream,
                                   uint32_t *dsp_frames)
{
    struct stream_out *out = (struct stream_out *)stream;
    *dsp_frames = 0;
    if ((out->usecase == USECASE_AUDIO_PLAYBACK_OFFLOAD) && (dsp_frames != NULL)) {
        pthread_mutex_lock(&out->lock);
        if (out->compr != NULL) {
            compress_get_tstamp(out->compr, (unsigned long *)dsp_frames,
                    &out->sample_rate);
            ALOGVV("%s rendered frames %d sample_rate %d",
                   __func__, *dsp_frames, out->sample_rate);
        }
        pthread_mutex_unlock(&out->lock);
        return 0;
    } else
        return -EINVAL;
}

static int out_add_audio_effect(const struct audio_stream *stream __unused,
                                effect_handle_t effect __unused)
{
    return 0;
}

static int out_remove_audio_effect(const struct audio_stream *stream __unused,
                                   effect_handle_t effect __unused)
{
    return 0;
}

static int out_get_next_write_timestamp(const struct audio_stream_out *stream __unused,
                                        int64_t *timestamp __unused)
{
    return -EINVAL;
}

static int out_get_presentation_position(const struct audio_stream_out *stream,
                                   uint64_t *frames, struct timespec *timestamp)
{
    struct stream_out *out = (struct stream_out *)stream;
    int ret = -1;
    unsigned long dsp_frames;

    pthread_mutex_lock(&out->lock);

    if (out->usecase == USECASE_AUDIO_PLAYBACK_OFFLOAD) {
        if (out->compr != NULL) {
            compress_get_tstamp(out->compr, &dsp_frames,
                    &out->sample_rate);
            ALOGVV("%s rendered frames %ld sample_rate %d",
                   __func__, dsp_frames, out->sample_rate);
            *frames = dsp_frames;
            ret = 0;
            /* this is the best we can do */
            clock_gettime(CLOCK_MONOTONIC, timestamp);
        }
    } else {
        if (out->pcm) {
            size_t avail;
            if (pcm_get_htimestamp(out->pcm, &avail, timestamp) == 0) {
                size_t kernel_buffer_size = out->config.period_size * out->config.period_count;
                int64_t signed_frames = out->written - kernel_buffer_size + avail;
                // This adjustment accounts for buffering after app processor.
                // It is based on estimated DSP latency per use case, rather than exact.
                signed_frames -=
                    (platform_render_latency(out->usecase) * out->sample_rate / 1000000LL);

                // It would be unusual for this value to be negative, but check just in case ...
                if (signed_frames >= 0) {
                    *frames = signed_frames;
                    ret = 0;
                }
            }
        }
    }

    pthread_mutex_unlock(&out->lock);

    return ret;
}

static int out_set_callback(struct audio_stream_out *stream,
            stream_callback_t callback, void *cookie)
{
    struct stream_out *out = (struct stream_out *)stream;

    ALOGV("%s", __func__);
    pthread_mutex_lock(&out->lock);
    out->offload_callback = callback;
    out->offload_cookie = cookie;
    pthread_mutex_unlock(&out->lock);
    return 0;
}

static int out_pause(struct audio_stream_out* stream)
{
    struct stream_out *out = (struct stream_out *)stream;
    int status = -ENOSYS;
    ALOGV("%s", __func__);
    if (out->usecase == USECASE_AUDIO_PLAYBACK_OFFLOAD) {
        pthread_mutex_lock(&out->lock);
        if (out->compr != NULL && out->offload_state == OFFLOAD_STATE_PLAYING) {
            status = compress_pause(out->compr);
            out->offload_state = OFFLOAD_STATE_PAUSED;
        }
        pthread_mutex_unlock(&out->lock);
    }
    return status;
}

static int out_resume(struct audio_stream_out* stream)
{
    struct stream_out *out = (struct stream_out *)stream;
    int status = -ENOSYS;
    ALOGV("%s", __func__);
    if (out->usecase == USECASE_AUDIO_PLAYBACK_OFFLOAD) {
        status = 0;
        pthread_mutex_lock(&out->lock);
        if (out->compr != NULL && out->offload_state == OFFLOAD_STATE_PAUSED) {
            status = compress_resume(out->compr);
            out->offload_state = OFFLOAD_STATE_PLAYING;
        }
        pthread_mutex_unlock(&out->lock);
    }
    return status;
}

static int out_drain(struct audio_stream_out* stream, audio_drain_type_t type )
{
    struct stream_out *out = (struct stream_out *)stream;
    int status = -ENOSYS;
    ALOGV("%s", __func__);
    if (out->usecase == USECASE_AUDIO_PLAYBACK_OFFLOAD) {
        pthread_mutex_lock(&out->lock);
        if (type == AUDIO_DRAIN_EARLY_NOTIFY)
            status = send_offload_cmd_l(out, OFFLOAD_CMD_PARTIAL_DRAIN);
        else
            status = send_offload_cmd_l(out, OFFLOAD_CMD_DRAIN);
        pthread_mutex_unlock(&out->lock);
    }
    return status;
}

static int out_flush(struct audio_stream_out* stream)
{
    struct stream_out *out = (struct stream_out *)stream;
    ALOGV("%s", __func__);
    if (out->usecase == USECASE_AUDIO_PLAYBACK_OFFLOAD) {
        pthread_mutex_lock(&out->lock);
        stop_compressed_output_l(out);
        pthread_mutex_unlock(&out->lock);
        return 0;
    }
    return -ENOSYS;
}

/** audio_stream_in implementation **/
static uint32_t in_get_sample_rate(const struct audio_stream *stream)
{
    struct stream_in *in = (struct stream_in *)stream;

    return in->config.rate;
}

static int in_set_sample_rate(struct audio_stream *stream __unused, uint32_t rate __unused)
{
    return -ENOSYS;
}

static size_t in_get_buffer_size(const struct audio_stream *stream)
{
    struct stream_in *in = (struct stream_in *)stream;

    return in->config.period_size *
                audio_stream_in_frame_size((const struct audio_stream_in *)stream);
}

static uint32_t in_get_channels(const struct audio_stream *stream)
{
    struct stream_in *in = (struct stream_in *)stream;

    return in->channel_mask;
}

static audio_format_t in_get_format(const struct audio_stream *stream __unused)
{
    return AUDIO_FORMAT_PCM_16_BIT;
}

static int in_set_format(struct audio_stream *stream __unused, audio_format_t format __unused)
{
    return -ENOSYS;
}

static int in_standby(struct audio_stream *stream)
{
    struct stream_in *in = (struct stream_in *)stream;
    struct audio_device *adev = in->dev;
    int status = 0;
    ALOGV("%s: enter", __func__);
    pthread_mutex_lock(&in->lock);
    if (!in->standby) {
        pthread_mutex_lock(&adev->lock);
        in->standby = true;
        if (in->pcm) {
            pcm_close(in->pcm);
            in->pcm = NULL;
        }
        adev->enable_voicerx = false;
        platform_set_echo_reference(adev, false, AUDIO_DEVICE_NONE );
        status = stop_input_stream(in);
        pthread_mutex_unlock(&adev->lock);
    }
    pthread_mutex_unlock(&in->lock);
    ALOGV("%s: exit:  status(%d)", __func__, status);
    return status;
}

static int in_dump(const struct audio_stream *stream __unused, int fd __unused)
{
    return 0;
}

static int in_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    struct stream_in *in = (struct stream_in *)stream;
    struct audio_device *adev = in->dev;
    struct str_parms *parms;
    char *str;
    char value[32];
    int ret, val = 0;
    int status = 0;

    ALOGV("%s: enter: kvpairs=%s", __func__, kvpairs);
    parms = str_parms_create_str(kvpairs);

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_INPUT_SOURCE, value, sizeof(value));

    pthread_mutex_lock(&in->lock);
    pthread_mutex_lock(&adev->lock);
    if (ret >= 0) {
        val = atoi(value);
        /* no audio source uses val == 0 */
        if ((in->source != val) && (val != 0)) {
            in->source = val;
        }
    }

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_ROUTING, value, sizeof(value));

    if (ret >= 0) {
        val = atoi(value);
        if (((int)in->device != val) && (val != 0)) {
            in->device = val;
            /* If recording is in progress, change the tx device to new device */
            if (!in->standby)
                status = select_devices(adev, in->usecase);
        }
    }

    pthread_mutex_unlock(&adev->lock);
    pthread_mutex_unlock(&in->lock);

    str_parms_destroy(parms);
    ALOGV("%s: exit: status(%d)", __func__, status);
    return status;
}

static char* in_get_parameters(const struct audio_stream *stream __unused,
                               const char *keys __unused)
{
    return strdup("");
}

static int in_set_gain(struct audio_stream_in *stream __unused, float gain __unused)
{
    return 0;
}

static ssize_t in_read(struct audio_stream_in *stream, void *buffer,
                       size_t bytes)
{
    struct stream_in *in = (struct stream_in *)stream;
    struct audio_device *adev = in->dev;
    int i, ret = -1;

    pthread_mutex_lock(&in->lock);
    if (in->standby) {
        pthread_mutex_lock(&adev->lock);
        ret = start_input_stream(in);
        pthread_mutex_unlock(&adev->lock);
        if (ret != 0) {
            goto exit;
        }
        in->standby = 0;
    }

    if (in->pcm) {
        if (in->usecase == USECASE_AUDIO_RECORD_AFE_PROXY) {
            ret = pcm_mmap_read(in->pcm, buffer, bytes);
        } else
            ret = pcm_read(in->pcm, buffer, bytes);
    }

    /*
     * Instead of writing zeroes here, we could trust the hardware
     * to always provide zeroes when muted.
     * No need to acquire adev->lock to read mic_muted here as we don't change its state.
     */
    if (ret == 0 && adev->mic_muted && in->usecase != USECASE_AUDIO_RECORD_AFE_PROXY)
        memset(buffer, 0, bytes);

exit:
    pthread_mutex_unlock(&in->lock);

    if (ret != 0) {
        in_standby(&in->stream.common);
        ALOGV("%s: read failed - sleeping for buffer duration", __func__);
        usleep(bytes * 1000000 / audio_stream_in_frame_size(stream) /
               in_get_sample_rate(&in->stream.common));
    }
    return bytes;
}

static uint32_t in_get_input_frames_lost(struct audio_stream_in *stream __unused)
{
    return 0;
}

static int add_remove_audio_effect(const struct audio_stream *stream,
                                   effect_handle_t effect,
                                   bool enable)
{
    struct stream_in *in = (struct stream_in *)stream;
    struct audio_device *adev = in->dev;
    int status = 0;
    effect_descriptor_t desc;

    status = (*effect)->get_descriptor(effect, &desc);
    if (status != 0)
        return status;

    pthread_mutex_lock(&in->lock);
    pthread_mutex_lock(&in->dev->lock);
    if ((in->source == AUDIO_SOURCE_VOICE_COMMUNICATION) &&
            in->enable_aec != enable &&
            (memcmp(&desc.type, FX_IID_AEC, sizeof(effect_uuid_t)) == 0)) {
        in->enable_aec = enable;
        if (!enable)
            platform_set_echo_reference(in->dev, enable, AUDIO_DEVICE_NONE);
        adev->enable_voicerx = enable;
        struct audio_usecase *usecase;
        struct listnode *node;
        list_for_each(node, &adev->usecase_list) {
            usecase = node_to_item(node, struct audio_usecase, list);
            if (usecase->type == PCM_PLAYBACK) {
                select_devices(adev, usecase->id);
            break;
            }
        }
        if (!in->standby)
            select_devices(in->dev, in->usecase);
    }
    if (in->enable_ns != enable &&
            (memcmp(&desc.type, FX_IID_NS, sizeof(effect_uuid_t)) == 0)) {
        in->enable_ns = enable;
        if (!in->standby)
            select_devices(in->dev, in->usecase);
    }
    pthread_mutex_unlock(&in->dev->lock);
    pthread_mutex_unlock(&in->lock);

    return 0;
}

static int in_add_audio_effect(const struct audio_stream *stream,
                               effect_handle_t effect)
{
    ALOGV("%s: effect %p", __func__, effect);
    return add_remove_audio_effect(stream, effect, true);
}

static int in_remove_audio_effect(const struct audio_stream *stream,
                                  effect_handle_t effect)
{
    ALOGV("%s: effect %p", __func__, effect);
    return add_remove_audio_effect(stream, effect, false);
}

static int adev_open_output_stream(struct audio_hw_device *dev,
                                   audio_io_handle_t handle,
                                   audio_devices_t devices,
                                   audio_output_flags_t flags,
                                   struct audio_config *config,
                                   struct audio_stream_out **stream_out,
                                   const char *address __unused)
{
    struct audio_device *adev = (struct audio_device *)dev;
    struct stream_out *out;
    int i, ret;

    ALOGV("%s: enter: sample_rate(%d) channel_mask(%#x) devices(%#x) flags(%#x)",
          __func__, config->sample_rate, config->channel_mask, devices, flags);
    *stream_out = NULL;
    out = (struct stream_out *)calloc(1, sizeof(struct stream_out));

    if (devices == AUDIO_DEVICE_NONE)
        devices = AUDIO_DEVICE_OUT_SPEAKER;

    out->flags = flags;
    out->devices = devices;
    out->dev = adev;
    out->format = config->format;
    out->sample_rate = config->sample_rate;
    out->channel_mask = AUDIO_CHANNEL_OUT_STEREO;
    out->supported_channel_masks[0] = AUDIO_CHANNEL_OUT_STEREO;
    out->handle = handle;

    /* Init use case and pcm_config */
    if (out->flags & AUDIO_OUTPUT_FLAG_DIRECT &&
            !(out->flags & AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD) &&
        out->devices & AUDIO_DEVICE_OUT_AUX_DIGITAL) {
        pthread_mutex_lock(&adev->lock);
        ret = read_hdmi_channel_masks(out);
        pthread_mutex_unlock(&adev->lock);
        if (ret != 0)
            goto error_open;

        if (config->sample_rate == 0)
            config->sample_rate = DEFAULT_OUTPUT_SAMPLING_RATE;
        if (config->channel_mask == 0)
            config->channel_mask = AUDIO_CHANNEL_OUT_5POINT1;

        out->channel_mask = config->channel_mask;
        out->sample_rate = config->sample_rate;
        out->usecase = USECASE_AUDIO_PLAYBACK_MULTI_CH;
        out->config = pcm_config_hdmi_multi;
        out->config.rate = config->sample_rate;
        out->config.channels = audio_channel_count_from_out_mask(out->channel_mask);
        out->config.period_size = HDMI_MULTI_PERIOD_BYTES / (out->config.channels * 2);
    } else if (out->flags & AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD) {
        if (config->offload_info.version != AUDIO_INFO_INITIALIZER.version ||
            config->offload_info.size != AUDIO_INFO_INITIALIZER.size) {
            ALOGE("%s: Unsupported Offload information", __func__);
            ret = -EINVAL;
            goto error_open;
        }
        if (!is_supported_format(config->offload_info.format)) {
            ALOGE("%s: Unsupported audio format", __func__);
            ret = -EINVAL;
            goto error_open;
        }

        out->compr_config.codec = (struct snd_codec *)
                                    calloc(1, sizeof(struct snd_codec));

        out->usecase = USECASE_AUDIO_PLAYBACK_OFFLOAD;
        if (config->offload_info.channel_mask)
            out->channel_mask = config->offload_info.channel_mask;
        else if (config->channel_mask)
            out->channel_mask = config->channel_mask;
        out->format = config->offload_info.format;
        out->sample_rate = config->offload_info.sample_rate;

        out->stream.set_callback = out_set_callback;
        out->stream.pause = out_pause;
        out->stream.resume = out_resume;
        out->stream.drain = out_drain;
        out->stream.flush = out_flush;

        out->compr_config.codec->id =
                get_snd_codec_id(config->offload_info.format);
        out->compr_config.fragment_size = COMPRESS_OFFLOAD_FRAGMENT_SIZE;
        out->compr_config.fragments = COMPRESS_OFFLOAD_NUM_FRAGMENTS;
        out->compr_config.codec->sample_rate = config->offload_info.sample_rate;
        out->compr_config.codec->bit_rate =
                    config->offload_info.bit_rate;
        out->compr_config.codec->ch_in =
                audio_channel_count_from_out_mask(config->channel_mask);
        out->compr_config.codec->ch_out = out->compr_config.codec->ch_in;

        if (flags & AUDIO_OUTPUT_FLAG_NON_BLOCKING)
            out->non_blocking = 1;

        out->send_new_metadata = 1;
        create_offload_callback_thread(out);
        ALOGV("%s: offloaded output offload_info version %04x bit rate %d",
                __func__, config->offload_info.version,
                config->offload_info.bit_rate);
    } else  if (out->devices == AUDIO_DEVICE_OUT_TELEPHONY_TX) {
        if (config->sample_rate == 0)
            config->sample_rate = AFE_PROXY_SAMPLING_RATE;
        if (config->sample_rate != 48000 && config->sample_rate != 16000 &&
                config->sample_rate != 8000) {
            config->sample_rate = AFE_PROXY_SAMPLING_RATE;
            ret = -EINVAL;
            goto error_open;
        }
        out->sample_rate = config->sample_rate;
        out->config.rate = config->sample_rate;
        if (config->format == AUDIO_FORMAT_DEFAULT)
            config->format = AUDIO_FORMAT_PCM_16_BIT;
        if (config->format != AUDIO_FORMAT_PCM_16_BIT) {
            config->format = AUDIO_FORMAT_PCM_16_BIT;
            ret = -EINVAL;
            goto error_open;
        }
        out->format = config->format;
        out->usecase = USECASE_AUDIO_PLAYBACK_AFE_PROXY;
        out->config = pcm_config_afe_proxy_playback;
        adev->voice_tx_output = out;
    } else {
        if (out->flags & AUDIO_OUTPUT_FLAG_DEEP_BUFFER) {
            out->usecase = USECASE_AUDIO_PLAYBACK_DEEP_BUFFER;
            out->config = pcm_config_deep_buffer;
        } else {
            out->usecase = USECASE_AUDIO_PLAYBACK_LOW_LATENCY;
            out->config = pcm_config_low_latency;
        }
        if (config->format != audio_format_from_pcm_format(out->config.format)) {
            if (k_enable_extended_precision
                    && pcm_params_format_test(adev->use_case_table[out->usecase],
                            pcm_format_from_audio_format(config->format))) {
                out->config.format = pcm_format_from_audio_format(config->format);
                /* out->format already set to config->format */
            } else {
                /* deny the externally proposed config format
                 * and use the one specified in audio_hw layer configuration.
                 * Note: out->format is returned by out->stream.common.get_format()
                 * and is used to set config->format in the code several lines below.
                 */
                out->format = audio_format_from_pcm_format(out->config.format);
            }
        }
        out->sample_rate = out->config.rate;
    }
    ALOGV("%s: Usecase(%s) config->format %#x  out->config.format %#x\n",
            __func__, use_case_table[out->usecase], config->format, out->config.format);

    if (flags & AUDIO_OUTPUT_FLAG_PRIMARY) {
        if (adev->primary_output == NULL)
            adev->primary_output = out;
        else {
            ALOGE("%s: Primary output is already opened", __func__);
            ret = -EEXIST;
            goto error_open;
        }
    }

    /* Check if this usecase is already existing */
    pthread_mutex_lock(&adev->lock);
    if (get_usecase_from_list(adev, out->usecase) != NULL) {
        ALOGE("%s: Usecase (%d) is already present", __func__, out->usecase);
        pthread_mutex_unlock(&adev->lock);
        ret = -EEXIST;
        goto error_open;
    }
    pthread_mutex_unlock(&adev->lock);

    out->stream.common.get_sample_rate = out_get_sample_rate;
    out->stream.common.set_sample_rate = out_set_sample_rate;
    out->stream.common.get_buffer_size = out_get_buffer_size;
    out->stream.common.get_channels = out_get_channels;
    out->stream.common.get_format = out_get_format;
    out->stream.common.set_format = out_set_format;
    out->stream.common.standby = out_standby;
    out->stream.common.dump = out_dump;
    out->stream.common.set_parameters = out_set_parameters;
    out->stream.common.get_parameters = out_get_parameters;
    out->stream.common.add_audio_effect = out_add_audio_effect;
    out->stream.common.remove_audio_effect = out_remove_audio_effect;
    out->stream.get_latency = out_get_latency;
    out->stream.set_volume = out_set_volume;
    out->stream.write = out_write;
    out->stream.get_render_position = out_get_render_position;
    out->stream.get_next_write_timestamp = out_get_next_write_timestamp;
    out->stream.get_presentation_position = out_get_presentation_position;

    out->standby = 1;
    /* out->muted = false; by calloc() */
    /* out->written = 0; by calloc() */

    pthread_mutex_init(&out->lock, (const pthread_mutexattr_t *) NULL);
    pthread_cond_init(&out->cond, (const pthread_condattr_t *) NULL);

    config->format = out->stream.common.get_format(&out->stream.common);
    config->channel_mask = out->stream.common.get_channels(&out->stream.common);
    config->sample_rate = out->stream.common.get_sample_rate(&out->stream.common);

    *stream_out = &out->stream;
    ALOGV("%s: exit", __func__);
    return 0;

error_open:
    free(out);
    *stream_out = NULL;
    ALOGD("%s: exit: ret %d", __func__, ret);
    return ret;
}

static void adev_close_output_stream(struct audio_hw_device *dev __unused,
                                     struct audio_stream_out *stream)
{
    struct stream_out *out = (struct stream_out *)stream;
    struct audio_device *adev = out->dev;

    ALOGV("%s: enter", __func__);
    out_standby(&stream->common);
    if (out->usecase == USECASE_AUDIO_PLAYBACK_OFFLOAD) {
        destroy_offload_callback_thread(out);

        if (out->compr_config.codec != NULL)
            free(out->compr_config.codec);
    }

    if (adev->voice_tx_output == out)
        adev->voice_tx_output = NULL;

    pthread_cond_destroy(&out->cond);
    pthread_mutex_destroy(&out->lock);
    free(stream);
    ALOGV("%s: exit", __func__);
}

static int adev_set_parameters(struct audio_hw_device *dev, const char *kvpairs)
{
    struct audio_device *adev = (struct audio_device *)dev;
    struct str_parms *parms;
    char *str;
    char value[32];
    int val;
    int ret;
    int status = 0;

    ALOGD("%s: enter: %s", __func__, kvpairs);

    pthread_mutex_lock(&adev->lock);

    parms = str_parms_create_str(kvpairs);
    status = voice_set_parameters(adev, parms);
    if (status != 0) {
        goto done;
    }

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_BT_NREC, value, sizeof(value));
    if (ret >= 0) {
        /* When set to false, HAL should disable EC and NS
         * But it is currently not supported.
         */
        if (strcmp(value, AUDIO_PARAMETER_VALUE_ON) == 0)
            adev->bluetooth_nrec = true;
        else
            adev->bluetooth_nrec = false;
    }

    ret = str_parms_get_str(parms, "screen_state", value, sizeof(value));
    if (ret >= 0) {
        if (strcmp(value, AUDIO_PARAMETER_VALUE_ON) == 0)
            adev->screen_off = false;
        else
            adev->screen_off = true;
    }

    ret = str_parms_get_int(parms, "rotation", &val);
    if (ret >= 0) {
        bool reverse_speakers = false;
        switch(val) {
        // FIXME: note that the code below assumes that the speakers are in the correct placement
        //   relative to the user when the device is rotated 90deg from its default rotation. This
        //   assumption is device-specific, not platform-specific like this code.
        case 270:
            reverse_speakers = true;
            break;
        case 0:
        case 90:
        case 180:
            break;
        default:
            ALOGE("%s: unexpected rotation of %d", __func__, val);
            status = -EINVAL;
        }
        if (status == 0) {
            if (adev->speaker_lr_swap != reverse_speakers) {
                adev->speaker_lr_swap = reverse_speakers;
                // only update the selected device if there is active pcm playback
                struct audio_usecase *usecase;
                struct listnode *node;
                list_for_each(node, &adev->usecase_list) {
                    usecase = node_to_item(node, struct audio_usecase, list);
                    if (usecase->type == PCM_PLAYBACK) {
                        select_devices(adev, usecase->id);
                        break;
                    }
                }
            }
        }
    }

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_BT_SCO_WB, value, sizeof(value));
    if (ret >= 0) {
        adev->bt_wb_speech_enabled = !strcmp(value, AUDIO_PARAMETER_VALUE_ON);
    }

    audio_extn_hfp_set_parameters(adev, parms);
done:
    str_parms_destroy(parms);
    pthread_mutex_unlock(&adev->lock);
    ALOGV("%s: exit with code(%d)", __func__, status);
    return status;
}

static char* adev_get_parameters(const struct audio_hw_device *dev,
                                 const char *keys)
{
    struct audio_device *adev = (struct audio_device *)dev;
    struct str_parms *reply = str_parms_create();
    struct str_parms *query = str_parms_create_str(keys);
    char *str;

    pthread_mutex_lock(&adev->lock);

    voice_get_parameters(adev, query, reply);
    str = str_parms_to_str(reply);
    str_parms_destroy(query);
    str_parms_destroy(reply);

    pthread_mutex_unlock(&adev->lock);
    ALOGV("%s: exit: returns - %s", __func__, str);
    return str;
}

static int adev_init_check(const struct audio_hw_device *dev __unused)
{
    return 0;
}

static int adev_set_voice_volume(struct audio_hw_device *dev, float volume)
{
    int ret;
    struct audio_device *adev = (struct audio_device *)dev;

    audio_extn_extspk_set_voice_vol(adev->extspk, volume);

    pthread_mutex_lock(&adev->lock);
    ret = voice_set_volume(adev, volume);
    pthread_mutex_unlock(&adev->lock);

    return ret;
}

static int adev_set_master_volume(struct audio_hw_device *dev __unused, float volume __unused)
{
    return -ENOSYS;
}

static int adev_get_master_volume(struct audio_hw_device *dev __unused,
                                  float *volume __unused)
{
    return -ENOSYS;
}

static int adev_set_master_mute(struct audio_hw_device *dev __unused, bool muted __unused)
{
    return -ENOSYS;
}

static int adev_get_master_mute(struct audio_hw_device *dev __unused, bool *muted __unused)
{
    return -ENOSYS;
}

static int adev_set_mode(struct audio_hw_device *dev, audio_mode_t mode)
{
    struct audio_device *adev = (struct audio_device *)dev;

    pthread_mutex_lock(&adev->lock);
    if (adev->mode != mode) {
        ALOGD("%s: mode %d\n", __func__, mode);
        adev->mode = mode;
        if ((mode == AUDIO_MODE_NORMAL || mode == AUDIO_MODE_IN_COMMUNICATION) &&
                voice_is_in_call(adev)) {
            voice_stop_call(adev);
            adev->current_call_output = NULL;
        }
    }
    pthread_mutex_unlock(&adev->lock);

    audio_extn_extspk_set_mode(adev->extspk, mode);

    return 0;
}

static int adev_set_mic_mute(struct audio_hw_device *dev, bool state)
{
    int ret;
    struct audio_device *adev = (struct audio_device *)dev;

    ALOGD("%s: state %d\n", __func__, state);
    pthread_mutex_lock(&adev->lock);
    ret = voice_set_mic_mute(adev, state);
    adev->mic_muted = state;
    pthread_mutex_unlock(&adev->lock);

    return ret;
}

static int adev_get_mic_mute(const struct audio_hw_device *dev, bool *state)
{
    *state = voice_get_mic_mute((struct audio_device *)dev);
    return 0;
}

static size_t adev_get_input_buffer_size(const struct audio_hw_device *dev __unused,
                                         const struct audio_config *config)
{
    int channel_count = audio_channel_count_from_in_mask(config->channel_mask);

    return get_input_buffer_size(config->sample_rate, config->format, channel_count,
            false /* is_low_latency: since we don't know, be conservative */);
}

static int adev_open_input_stream(struct audio_hw_device *dev,
                                  audio_io_handle_t handle __unused,
                                  audio_devices_t devices,
                                  struct audio_config *config,
                                  struct audio_stream_in **stream_in,
                                  audio_input_flags_t flags,
                                  const char *address __unused,
                                  audio_source_t source )
{
    struct audio_device *adev = (struct audio_device *)dev;
    struct stream_in *in;
    int ret = 0, buffer_size, frame_size;
    int channel_count = audio_channel_count_from_in_mask(config->channel_mask);
    bool is_low_latency = false;

    ALOGV("%s: enter", __func__);
    *stream_in = NULL;
    if (check_input_parameters(config->sample_rate, config->format, channel_count) != 0)
        return -EINVAL;

    in = (struct stream_in *)calloc(1, sizeof(struct stream_in));

    pthread_mutex_init(&in->lock, (const pthread_mutexattr_t *) NULL);

    in->stream.common.get_sample_rate = in_get_sample_rate;
    in->stream.common.set_sample_rate = in_set_sample_rate;
    in->stream.common.get_buffer_size = in_get_buffer_size;
    in->stream.common.get_channels = in_get_channels;
    in->stream.common.get_format = in_get_format;
    in->stream.common.set_format = in_set_format;
    in->stream.common.standby = in_standby;
    in->stream.common.dump = in_dump;
    in->stream.common.set_parameters = in_set_parameters;
    in->stream.common.get_parameters = in_get_parameters;
    in->stream.common.add_audio_effect = in_add_audio_effect;
    in->stream.common.remove_audio_effect = in_remove_audio_effect;
    in->stream.set_gain = in_set_gain;
    in->stream.read = in_read;
    in->stream.get_input_frames_lost = in_get_input_frames_lost;

    in->device = devices;
    in->source = source;
    in->dev = adev;
    in->standby = 1;
    in->channel_mask = config->channel_mask;

    /* Update config params with the requested sample rate and channels */
    if (in->device == AUDIO_DEVICE_IN_TELEPHONY_RX) {
        if (config->sample_rate == 0)
            config->sample_rate = AFE_PROXY_SAMPLING_RATE;
        if (config->sample_rate != 48000 && config->sample_rate != 16000 &&
                config->sample_rate != 8000) {
            config->sample_rate = AFE_PROXY_SAMPLING_RATE;
            ret = -EINVAL;
            goto err_open;
        }
        if (config->format == AUDIO_FORMAT_DEFAULT)
            config->format = AUDIO_FORMAT_PCM_16_BIT;
        if (config->format != AUDIO_FORMAT_PCM_16_BIT) {
            config->format = AUDIO_FORMAT_PCM_16_BIT;
            ret = -EINVAL;
            goto err_open;
        }

        in->usecase = USECASE_AUDIO_RECORD_AFE_PROXY;
        in->config = pcm_config_afe_proxy_record;
    } else {
        in->usecase = USECASE_AUDIO_RECORD;
        if (config->sample_rate == LOW_LATENCY_CAPTURE_SAMPLE_RATE &&
                (flags & AUDIO_INPUT_FLAG_FAST) != 0) {
            is_low_latency = true;
#if LOW_LATENCY_CAPTURE_USE_CASE
            in->usecase = USECASE_AUDIO_RECORD_LOW_LATENCY;
#endif
        }
        in->config = pcm_config_audio_capture;

        frame_size = audio_stream_in_frame_size(&in->stream);
        buffer_size = get_input_buffer_size(config->sample_rate,
                                            config->format,
                                            channel_count,
                                            is_low_latency);
        in->config.period_size = buffer_size / frame_size;
    }
    in->config.channels = channel_count;
    in->config.rate = config->sample_rate;


    *stream_in = &in->stream;
    ALOGV("%s: exit", __func__);
    return 0;

err_open:
    free(in);
    *stream_in = NULL;
    return ret;
}

static void adev_close_input_stream(struct audio_hw_device *dev __unused,
                                    struct audio_stream_in *stream)
{
    ALOGV("%s", __func__);

    in_standby(&stream->common);
    free(stream);

    return;
}

static int adev_dump(const audio_hw_device_t *device __unused, int fd __unused)
{
    return 0;
}

/* verifies input and output devices and their capabilities.
 *
 * This verification is required when enabling extended bit-depth or
 * sampling rates, as not all qcom products support it.
 *
 * Suitable for calling only on initialization such as adev_open().
 * It fills the audio_device use_case_table[] array.
 *
 * Has a side-effect that it needs to configure audio routing / devices
 * in order to power up the devices and read the device parameters.
 * It does not acquire any hw device lock. Should restore the devices
 * back to "normal state" upon completion.
 */
static int adev_verify_devices(struct audio_device *adev)
{
    /* enumeration is a bit difficult because one really wants to pull
     * the use_case, device id, etc from the hidden pcm_device_table[].
     * In this case there are the following use cases and device ids.
     *
     * [USECASE_AUDIO_PLAYBACK_DEEP_BUFFER] = {0, 0},
     * [USECASE_AUDIO_PLAYBACK_LOW_LATENCY] = {15, 15},
     * [USECASE_AUDIO_PLAYBACK_MULTI_CH] = {1, 1},
     * [USECASE_AUDIO_PLAYBACK_OFFLOAD] = {9, 9},
     * [USECASE_AUDIO_RECORD] = {0, 0},
     * [USECASE_AUDIO_RECORD_LOW_LATENCY] = {15, 15},
     * [USECASE_VOICE_CALL] = {2, 2},
     *
     * USECASE_AUDIO_PLAYBACK_OFFLOAD, USECASE_AUDIO_PLAYBACK_MULTI_CH omitted.
     * USECASE_VOICE_CALL omitted, but possible for either input or output.
     */

    /* should be the usecases enabled in adev_open_input_stream() */
    static const int test_in_usecases[] = {
             USECASE_AUDIO_RECORD,
             USECASE_AUDIO_RECORD_LOW_LATENCY, /* does not appear to be used */
    };
    /* should be the usecases enabled in adev_open_output_stream()*/
    static const int test_out_usecases[] = {
            USECASE_AUDIO_PLAYBACK_DEEP_BUFFER,
            USECASE_AUDIO_PLAYBACK_LOW_LATENCY,
    };
    static const usecase_type_t usecase_type_by_dir[] = {
            PCM_PLAYBACK,
            PCM_CAPTURE,
    };
    static const unsigned flags_by_dir[] = {
            PCM_OUT,
            PCM_IN,
    };

    size_t i;
    unsigned dir;
    const unsigned card_id = adev->snd_card;
    char info[512]; /* for possible debug info */

    for (dir = 0; dir < 2; ++dir) {
        const usecase_type_t usecase_type = usecase_type_by_dir[dir];
        const unsigned flags_dir = flags_by_dir[dir];
        const size_t testsize =
                dir ? ARRAY_SIZE(test_in_usecases) : ARRAY_SIZE(test_out_usecases);
        const int *testcases =
                dir ? test_in_usecases : test_out_usecases;
        const audio_devices_t audio_device =
                dir ? AUDIO_DEVICE_IN_BUILTIN_MIC : AUDIO_DEVICE_OUT_SPEAKER;

        for (i = 0; i < testsize; ++i) {
            const audio_usecase_t audio_usecase = testcases[i];
            int device_id;
            snd_device_t snd_device;
            struct pcm_params **pparams;
            struct stream_out out;
            struct stream_in in;
            struct audio_usecase uc_info;
            int retval;

            pparams = &adev->use_case_table[audio_usecase];
            pcm_params_free(*pparams); /* can accept null input */
            *pparams = NULL;

            /* find the device ID for the use case (signed, for error) */
            device_id = platform_get_pcm_device_id(audio_usecase, usecase_type);
            if (device_id < 0)
                continue;

            /* prepare structures for device probing */
            memset(&uc_info, 0, sizeof(uc_info));
            uc_info.id = audio_usecase;
            uc_info.type = usecase_type;
            if (dir) {
                adev->active_input = &in;
                memset(&in, 0, sizeof(in));
                in.device = audio_device;
                in.source = AUDIO_SOURCE_VOICE_COMMUNICATION;
                uc_info.stream.in = &in;
            }  else {
                adev->active_input = NULL;
            }
            memset(&out, 0, sizeof(out));
            out.devices = audio_device; /* only field needed in select_devices */
            uc_info.stream.out = &out;
            uc_info.devices = audio_device;
            uc_info.in_snd_device = SND_DEVICE_NONE;
            uc_info.out_snd_device = SND_DEVICE_NONE;
            list_add_tail(&adev->usecase_list, &uc_info.list);

            /* select device - similar to start_(in/out)put_stream() */
            retval = select_devices(adev, audio_usecase);
            if (retval >= 0) {
                *pparams = pcm_params_get(card_id, device_id, flags_dir);
#if LOG_NDEBUG == 0
                if (*pparams) {
                    ALOGV("%s: (%s) card %d  device %d", __func__,
                            dir ? "input" : "output", card_id, device_id);
                    pcm_params_to_string(*pparams, info, ARRAY_SIZE(info));
                    ALOGV(info); /* print parameters */
                } else {
                    ALOGV("%s: cannot locate card %d  device %d", __func__, card_id, device_id);
                }
#endif
            }

            /* deselect device - similar to stop_(in/out)put_stream() */
            /* 1. Get and set stream specific mixer controls */
            retval = disable_audio_route(adev, &uc_info);
            /* 2. Disable the rx device */
            retval = disable_snd_device(adev,
                    dir ? uc_info.in_snd_device : uc_info.out_snd_device);
            list_remove(&uc_info.list);
        }
    }
    adev->active_input = NULL; /* restore adev state */
    return 0;
}

static int adev_close(hw_device_t *device)
{
    size_t i;
    struct audio_device *adev = (struct audio_device *)device;
    audio_route_free(adev->audio_route);
    free(adev->snd_dev_ref_cnt);
    platform_deinit(adev->platform);
    audio_extn_extspk_deinit(adev->extspk);
    for (i = 0; i < ARRAY_SIZE(adev->use_case_table); ++i) {
        pcm_params_free(adev->use_case_table[i]);
    }
    free(device);
    return 0;
}

/* This returns 1 if the input parameter looks at all plausible as a low latency period size,
 * or 0 otherwise.  A return value of 1 doesn't mean the value is guaranteed to work,
 * just that it _might_ work.
 */
static int period_size_is_plausible_for_low_latency(int period_size)
{
    switch (period_size) {
    case 160:
    case 240:
    case 320:
    case 480:
        return 1;
    default:
        return 0;
    }
}

static int adev_open(const hw_module_t *module, const char *name,
                     hw_device_t **device)
{
    struct audio_device *adev;
    int i, ret;

    ALOGD("%s: enter", __func__);
    if (strcmp(name, AUDIO_HARDWARE_INTERFACE) != 0) return -EINVAL;

    adev = calloc(1, sizeof(struct audio_device));

    pthread_mutex_init(&adev->lock, (const pthread_mutexattr_t *) NULL);

    adev->device.common.tag = HARDWARE_DEVICE_TAG;
    adev->device.common.version = AUDIO_DEVICE_API_VERSION_2_0;
    adev->device.common.module = (struct hw_module_t *)module;
    adev->device.common.close = adev_close;

    adev->device.init_check = adev_init_check;
    adev->device.set_voice_volume = adev_set_voice_volume;
    adev->device.set_master_volume = adev_set_master_volume;
    adev->device.get_master_volume = adev_get_master_volume;
    adev->device.set_master_mute = adev_set_master_mute;
    adev->device.get_master_mute = adev_get_master_mute;
    adev->device.set_mode = adev_set_mode;
    adev->device.set_mic_mute = adev_set_mic_mute;
    adev->device.get_mic_mute = adev_get_mic_mute;
    adev->device.set_parameters = adev_set_parameters;
    adev->device.get_parameters = adev_get_parameters;
    adev->device.get_input_buffer_size = adev_get_input_buffer_size;
    adev->device.open_output_stream = adev_open_output_stream;
    adev->device.close_output_stream = adev_close_output_stream;
    adev->device.open_input_stream = adev_open_input_stream;
    adev->device.close_input_stream = adev_close_input_stream;
    adev->device.dump = adev_dump;

    /* Set the default route before the PCM stream is opened */
    pthread_mutex_lock(&adev->lock);
    adev->mode = AUDIO_MODE_NORMAL;
    adev->active_input = NULL;
    adev->primary_output = NULL;
    adev->bluetooth_nrec = true;
    adev->acdb_settings = TTY_MODE_OFF;
    /* adev->cur_hdmi_channels = 0;  by calloc() */
    adev->snd_dev_ref_cnt = calloc(SND_DEVICE_MAX, sizeof(int));
    voice_init(adev);
    list_init(&adev->usecase_list);
    pthread_mutex_unlock(&adev->lock);

    /* Loads platform specific libraries dynamically */
    adev->platform = platform_init(adev);
    if (!adev->platform) {
        free(adev->snd_dev_ref_cnt);
        free(adev);
        ALOGE("%s: Failed to init platform data, aborting.", __func__);
        *device = NULL;
        return -EINVAL;
    }

    adev->extspk = audio_extn_extspk_init(adev);

    if (access(VISUALIZER_LIBRARY_PATH, R_OK) == 0) {
        adev->visualizer_lib = dlopen(VISUALIZER_LIBRARY_PATH, RTLD_NOW);
        if (adev->visualizer_lib == NULL) {
            ALOGE("%s: DLOPEN failed for %s", __func__, VISUALIZER_LIBRARY_PATH);
        } else {
            ALOGV("%s: DLOPEN successful for %s", __func__, VISUALIZER_LIBRARY_PATH);
            adev->visualizer_start_output =
                        (int (*)(audio_io_handle_t, int))dlsym(adev->visualizer_lib,
                                                        "visualizer_hal_start_output");
            adev->visualizer_stop_output =
                        (int (*)(audio_io_handle_t, int))dlsym(adev->visualizer_lib,
                                                        "visualizer_hal_stop_output");
        }
    }

    if (access(OFFLOAD_EFFECTS_BUNDLE_LIBRARY_PATH, R_OK) == 0) {
        adev->offload_effects_lib = dlopen(OFFLOAD_EFFECTS_BUNDLE_LIBRARY_PATH, RTLD_NOW);
        if (adev->offload_effects_lib == NULL) {
            ALOGE("%s: DLOPEN failed for %s", __func__,
                  OFFLOAD_EFFECTS_BUNDLE_LIBRARY_PATH);
        } else {
            ALOGV("%s: DLOPEN successful for %s", __func__,
                  OFFLOAD_EFFECTS_BUNDLE_LIBRARY_PATH);
            adev->offload_effects_start_output =
                        (int (*)(audio_io_handle_t, int))dlsym(adev->offload_effects_lib,
                                         "offload_effects_bundle_hal_start_output");
            adev->offload_effects_stop_output =
                        (int (*)(audio_io_handle_t, int))dlsym(adev->offload_effects_lib,
                                         "offload_effects_bundle_hal_stop_output");
        }
    }

    adev->bt_wb_speech_enabled = false;
    adev->enable_voicerx = false;

    *device = &adev->device.common;
    if (k_enable_extended_precision)
        adev_verify_devices(adev);

    char value[PROPERTY_VALUE_MAX];
    int trial;
    if (property_get("audio_hal.period_size", value, NULL) > 0) {
        trial = atoi(value);
        if (period_size_is_plausible_for_low_latency(trial)) {
            pcm_config_low_latency.period_size = trial;
            pcm_config_low_latency.start_threshold = trial / 4;
            pcm_config_low_latency.avail_min = trial / 4;
            configured_low_latency_capture_period_size = trial;
        }
    }
    if (property_get("audio_hal.in_period_size", value, NULL) > 0) {
        trial = atoi(value);
        if (period_size_is_plausible_for_low_latency(trial)) {
            configured_low_latency_capture_period_size = trial;
        }
    }

    ALOGV("%s: exit", __func__);
    return 0;
}

static struct hw_module_methods_t hal_module_methods = {
    .open = adev_open,
};

struct audio_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = AUDIO_MODULE_API_VERSION_0_1,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = AUDIO_HARDWARE_MODULE_ID,
        .name = "QCOM Audio HAL",
        .author = "Code Aurora Forum",
        .methods = &hal_module_methods,
    },
};
