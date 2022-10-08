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
     AUDIO_DEVICE_OUT_WIRED_HEADSET | AUDIO_DEVICE_OUT_WIRED_HEADPHONE)

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
    SND_DEVICE_OUT_HEADPHONES,
    SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES,
    SND_DEVICE_OUT_VOICE_SPEAKER,
    SND_DEVICE_OUT_VOICE_HEADPHONES,
    SND_DEVICE_OUT_HDMI,
    SND_DEVICE_OUT_SPEAKER_AND_HDMI,
    SND_DEVICE_OUT_BT_SCO,
    SND_DEVICE_OUT_BT_SCO_WB,
    SND_DEVICE_OUT_VOICE_HANDSET_TMUS,
    SND_DEVICE_OUT_VOICE_TTY_FULL_HEADPHONES,
    SND_DEVICE_OUT_VOICE_TTY_VCO_HEADPHONES,
    SND_DEVICE_OUT_VOICE_TTY_HCO_HANDSET,
    SND_DEVICE_OUT_END,

    /*
     * Note: IN_BEGIN should be same as OUT_END because total number of devices
     * SND_DEVICES_MAX should not exceed MAX_RX + MAX_TX devices.
     */
    /* Capture devices */
    SND_DEVICE_IN_BEGIN = SND_DEVICE_OUT_END,
    SND_DEVICE_IN_HANDSET_MIC  = SND_DEVICE_IN_BEGIN,
    SND_DEVICE_IN_SPEAKER_MIC,
    SND_DEVICE_IN_HEADSET_MIC,
    SND_DEVICE_IN_HANDSET_MIC_AEC,
    SND_DEVICE_IN_SPEAKER_MIC_AEC,
    SND_DEVICE_IN_HEADSET_MIC_AEC,
    SND_DEVICE_IN_VOICE_SPEAKER_MIC,
    SND_DEVICE_IN_VOICE_HEADSET_MIC,
    SND_DEVICE_IN_HDMI_MIC,
    SND_DEVICE_IN_BT_SCO_MIC,
    SND_DEVICE_IN_BT_SCO_MIC_WB,
    SND_DEVICE_IN_CAMCORDER_MIC,
    SND_DEVICE_IN_VOICE_DMIC_EF,
    SND_DEVICE_IN_VOICE_DMIC_BS,
    SND_DEVICE_IN_VOICE_DMIC_EF_TMUS,
    SND_DEVICE_IN_VOICE_SPEAKER_DMIC_EF,
    SND_DEVICE_IN_VOICE_SPEAKER_DMIC_BS,
    SND_DEVICE_IN_VOICE_TTY_FULL_HEADSET_MIC,
    SND_DEVICE_IN_VOICE_TTY_VCO_HANDSET_MIC,
    SND_DEVICE_IN_VOICE_TTY_HCO_HEADSET_MIC,
    SND_DEVICE_IN_VOICE_REC_MIC,
    SND_DEVICE_IN_VOICE_REC_DMIC_EF,
    SND_DEVICE_IN_VOICE_REC_DMIC_BS,
    SND_DEVICE_IN_VOICE_REC_DMIC_EF_FLUENCE,
    SND_DEVICE_IN_VOICE_REC_DMIC_BS_FLUENCE,
    SND_DEVICE_IN_END,

    SND_DEVICE_MAX = SND_DEVICE_IN_END,

};

#define MIXER_CARD 0
#define SOUND_CARD 0

#define DEFAULT_OUTPUT_SAMPLING_RATE 48000

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
#define LOW_LATENCY_CAPTURE_USE_CASE 0

#define AFE_PROXY_PLAYBACK_PCM_DEVICE 7
#define AFE_PROXY_RECORD_PCM_DEVICE 8


#endif // QCOM_AUDIO_PLATFORM_H
