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

#ifndef QCOM_AUDIO_PLATFORM_H
#define QCOM_AUDIO_PLATFORM_H

/*
 * Below are the devices for which is back end is same, SLIMBUS_0_RX.
 * All these devices are handled by the internal HW codec. We can
 * enable any one of these devices at any time
 */
#define AUDIO_DEVICE_OUT_ALL_CODEC_BACKEND \
    (AUDIO_DEVICE_OUT_EARPIECE | AUDIO_DEVICE_OUT_SPEAKER | \
     AUDIO_DEVICE_OUT_SPEAKER_SAFE | \
     AUDIO_DEVICE_OUT_WIRED_HEADSET | AUDIO_DEVICE_OUT_WIRED_HEADPHONE | \
     AUDIO_DEVICE_OUT_LINE)

/* Sound devices specific to the platform
 * The DEVICE_OUT_* and DEVICE_IN_* should be mapped to these sound
 * devices to enable corresponding mixer paths
 */
enum {
    SND_DEVICE_NONE = 0,

    /* Playback devices */
    SND_DEVICE_MIN,
    SND_DEVICE_OUT_BEGIN = SND_DEVICE_MIN,
    SND_DEVICE_OUT_HANDSET = SND_DEVICE_OUT_BEGIN,
    SND_DEVICE_OUT_SPEAKER,
    SND_DEVICE_OUT_SPEAKER_REVERSE,
    SND_DEVICE_OUT_SPEAKER_SAFE,
    SND_DEVICE_OUT_HEADPHONES,
    SND_DEVICE_OUT_LINE,
    SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES,
    SND_DEVICE_OUT_SPEAKER_AND_LINE,
    SND_DEVICE_OUT_VOICE_HANDSET,
    SND_DEVICE_OUT_VOICE_SPEAKER,
    SND_DEVICE_OUT_VOICE_HEADPHONES,
    SND_DEVICE_OUT_VOICE_LINE,
    SND_DEVICE_OUT_HDMI,
    SND_DEVICE_OUT_SPEAKER_AND_HDMI,
    SND_DEVICE_OUT_BT_SCO,
    SND_DEVICE_OUT_BT_SCO_WB,
    SND_DEVICE_OUT_VOICE_HANDSET_TMUS,
    SND_DEVICE_OUT_VOICE_TTY_FULL_HEADPHONES,
    SND_DEVICE_OUT_VOICE_TTY_VCO_HEADPHONES,
    SND_DEVICE_OUT_VOICE_TTY_HCO_HANDSET,
    SND_DEVICE_OUT_VOICE_HAC_HANDSET,
    SND_DEVICE_OUT_VOICE_TX,
    SND_DEVICE_OUT_END,

    /*
     * Note: IN_BEGIN should be same as OUT_END because total number of devices
     * SND_DEVICES_MAX should not exceed MAX_RX + MAX_TX devices.
     */
    /* Capture devices */
    SND_DEVICE_IN_BEGIN = SND_DEVICE_OUT_END,
    SND_DEVICE_IN_HANDSET_MIC  = SND_DEVICE_IN_BEGIN,
    SND_DEVICE_IN_HANDSET_MIC_AEC,
    SND_DEVICE_IN_HANDSET_MIC_NS,
    SND_DEVICE_IN_HANDSET_MIC_AEC_NS,
    SND_DEVICE_IN_HANDSET_DMIC,
    SND_DEVICE_IN_HANDSET_DMIC_AEC,
    SND_DEVICE_IN_HANDSET_DMIC_NS,
    SND_DEVICE_IN_HANDSET_DMIC_AEC_NS,
    SND_DEVICE_IN_HANDSET_DMIC_STEREO,

    SND_DEVICE_IN_SPEAKER_MIC,
    SND_DEVICE_IN_SPEAKER_MIC_AEC,
    SND_DEVICE_IN_SPEAKER_MIC_NS,
    SND_DEVICE_IN_SPEAKER_MIC_AEC_NS,
    SND_DEVICE_IN_SPEAKER_DMIC,
    SND_DEVICE_IN_SPEAKER_DMIC_AEC,
    SND_DEVICE_IN_SPEAKER_DMIC_NS,
    SND_DEVICE_IN_SPEAKER_DMIC_AEC_NS,
    SND_DEVICE_IN_SPEAKER_DMIC_STEREO,

    SND_DEVICE_IN_HEADSET_MIC,
    SND_DEVICE_IN_HEADSET_MIC_AEC,

    SND_DEVICE_IN_HDMI_MIC,
    SND_DEVICE_IN_BT_SCO_MIC,
    SND_DEVICE_IN_BT_SCO_MIC_WB,

    SND_DEVICE_IN_CAMCORDER_MIC,

    SND_DEVICE_IN_VOICE_DMIC,
    SND_DEVICE_IN_VOICE_DMIC_TMUS,
    SND_DEVICE_IN_VOICE_SPEAKER_MIC,
    SND_DEVICE_IN_VOICE_SPEAKER_DMIC,
    SND_DEVICE_IN_VOICE_HEADSET_MIC,
    SND_DEVICE_IN_VOICE_TTY_FULL_HEADSET_MIC,
    SND_DEVICE_IN_VOICE_TTY_VCO_HANDSET_MIC,
    SND_DEVICE_IN_VOICE_TTY_HCO_HEADSET_MIC,

    SND_DEVICE_IN_VOICE_REC_MIC,
    SND_DEVICE_IN_VOICE_REC_MIC_NS,
    SND_DEVICE_IN_VOICE_REC_DMIC_STEREO,
    SND_DEVICE_IN_VOICE_REC_DMIC_FLUENCE,

    SND_DEVICE_IN_VOICE_RX,

    SND_DEVICE_IN_END,

    SND_DEVICE_MAX = SND_DEVICE_IN_END,

};

#define DEFAULT_OUTPUT_SAMPLING_RATE 48000

#define ALL_SESSION_VSID                0xFFFFFFFF
#define DEFAULT_MUTE_RAMP_DURATION_MS   20
#define DEFAULT_VOLUME_RAMP_DURATION_MS 20

#ifdef PLATFORM_MSM8084
#define ACDB_ID_VOICE_SPEAKER 66
#define ACDB_ID_VOICE_HANDSET 67
#define ACDB_ID_VOICE_HANDSET_TMUS 67
#define ACDB_ID_VOICE_DMIC_EF_TMUS 89
#define ACDB_ID_HEADSET_MIC_AEC 47
#else
#define ACDB_ID_VOICE_SPEAKER 15
#define ACDB_ID_VOICE_HANDSET 7
#define ACDB_ID_VOICE_HANDSET_TMUS 88
#define ACDB_ID_VOICE_DMIC_EF_TMUS 89
#define ACDB_ID_HEADSET_MIC_AEC 8
#endif

#define MAX_VOL_INDEX 5
#define MIN_VOL_INDEX 0
#define percent_to_index(val, min, max) \
            ((val) * ((max) - (min)) * 0.01 + (min) + .5)

/*
 * tinyAlsa library interprets period size as number of frames
 * one frame = channel_count * sizeof (pcm sample)
 * so if format = 16-bit PCM and channels = Stereo, frame size = 2 ch * 2 = 4 bytes
 * DEEP_BUFFER_OUTPUT_PERIOD_SIZE = 1024 means 1024 * 4 = 4096 bytes
 * We should take care of returning proper size when AudioFlinger queries for
 * the buffer size of an input/output stream
 */
#define DEEP_BUFFER_OUTPUT_PERIOD_SIZE 960
#define DEEP_BUFFER_OUTPUT_PERIOD_COUNT 8
#define LOW_LATENCY_OUTPUT_PERIOD_SIZE 240
#define LOW_LATENCY_OUTPUT_PERIOD_COUNT 2

#define HDMI_MULTI_PERIOD_SIZE  336
#define HDMI_MULTI_PERIOD_COUNT 8
#define HDMI_MULTI_DEFAULT_CHANNEL_COUNT 6
#define HDMI_MULTI_PERIOD_BYTES (HDMI_MULTI_PERIOD_SIZE * HDMI_MULTI_DEFAULT_CHANNEL_COUNT * 2)

#define AUDIO_CAPTURE_PERIOD_DURATION_MSEC 20
#define AUDIO_CAPTURE_PERIOD_COUNT 2

#define LOW_LATENCY_CAPTURE_SAMPLE_RATE 48000
#define LOW_LATENCY_CAPTURE_PERIOD_SIZE 240
#define LOW_LATENCY_CAPTURE_USE_CASE 1

#define DEEP_BUFFER_PCM_DEVICE 0
#define AUDIO_RECORD_PCM_DEVICE 0
#define MULTIMEDIA2_PCM_DEVICE 1
#define PLAYBACK_OFFLOAD_DEVICE 9
#define LOWLATENCY_PCM_DEVICE 15
#define VOICE_VSID  0x10C01000
#ifdef PLATFORM_MSM8084
#define VOICE_CALL_PCM_DEVICE 20
#define VOICE2_CALL_PCM_DEVICE 25
#define VOLTE_CALL_PCM_DEVICE 21
#define QCHAT_CALL_PCM_DEVICE 33
#define VOWLAN_CALL_PCM_DEVICE -1
#else
#define VOICE_CALL_PCM_DEVICE 2
#define VOICE2_CALL_PCM_DEVICE 22
#define VOLTE_CALL_PCM_DEVICE 14
#define QCHAT_CALL_PCM_DEVICE 20
#define VOWLAN_CALL_PCM_DEVICE 36
#endif

#define AFE_PROXY_PLAYBACK_PCM_DEVICE 7
#define AFE_PROXY_RECORD_PCM_DEVICE 8

#define HFP_PCM_RX 5
#ifdef PLATFORM_MSM8x26
#define HFP_SCO_RX 28
#define HFP_ASM_RX_TX 29
#else
#define HFP_SCO_RX 23
#define HFP_ASM_RX_TX 24
#endif

#define LIB_CSD_CLIENT "libcsd-client.so"
#define LIB_MDM_DETECT "libmdmdetect.so"

/* CSD-CLIENT related functions */
typedef int (*init_t)(bool);
typedef int (*deinit_t)();
typedef int (*disable_device_t)();
typedef int (*enable_device_config_t)(int, int);
typedef int (*enable_device_t)(int, int, uint32_t);
typedef int (*volume_t)(uint32_t, int, uint16_t);
typedef int (*mic_mute_t)(uint32_t, int, uint16_t);
typedef int (*slow_talk_t)(uint32_t, uint8_t);
typedef int (*start_voice_t)(uint32_t);
typedef int (*stop_voice_t)(uint32_t);
typedef int (*start_playback_t)(uint32_t);
typedef int (*stop_playback_t)(uint32_t);
typedef int (*start_record_t)(uint32_t, int);
typedef int (*stop_record_t)(uint32_t);
typedef int (*get_sample_rate_t)(uint32_t *);
/* CSD Client structure */
struct csd_data {
    void *csd_client;
    init_t init;
    deinit_t deinit;
    disable_device_t disable_device;
    enable_device_config_t enable_device_config;
    enable_device_t enable_device;
    volume_t volume;
    mic_mute_t mic_mute;
    slow_talk_t slow_talk;
    start_voice_t start_voice;
    stop_voice_t stop_voice;
    start_playback_t start_playback;
    stop_playback_t stop_playback;
    start_record_t start_record;
    stop_record_t stop_record;
    get_sample_rate_t get_sample_rate;
};

#endif // QCOM_AUDIO_PLATFORM_H
