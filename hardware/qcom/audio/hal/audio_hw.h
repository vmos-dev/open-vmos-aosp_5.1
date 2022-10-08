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

#ifndef QCOM_AUDIO_HW_H
#define QCOM_AUDIO_HW_H

#include <cutils/str_parms.h>
#include <cutils/list.h>
#include <hardware/audio.h>

#include <tinyalsa/asoundlib.h>
#include <tinycompress/tinycompress.h>

#include <audio_route/audio_route.h>
#include "voice.h"

#define VISUALIZER_LIBRARY_PATH "/system/lib/soundfx/libqcomvisualizer.so"
#define OFFLOAD_EFFECTS_BUNDLE_LIBRARY_PATH "/system/lib/soundfx/libqcompostprocbundle.so"

/* Flags used to initialize acdb_settings variable that goes to ACDB library */
#define DMIC_FLAG       0x00000002
#define TTY_MODE_OFF    0x00000010
#define TTY_MODE_FULL   0x00000020
#define TTY_MODE_VCO    0x00000040
#define TTY_MODE_HCO    0x00000080
#define TTY_MODE_CLEAR  0xFFFFFF0F

#define ACDB_DEV_TYPE_OUT 1
#define ACDB_DEV_TYPE_IN 2

#define MAX_SUPPORTED_CHANNEL_MASKS 2
#define DEFAULT_HDMI_OUT_CHANNELS   2

typedef int snd_device_t;

/* These are the supported use cases by the hardware.
 * Each usecase is mapped to a specific PCM device.
 * Refer to pcm_device_table[].
 */
enum {
    USECASE_INVALID = -1,
    /* Playback usecases */
    USECASE_AUDIO_PLAYBACK_DEEP_BUFFER = 0,
    USECASE_AUDIO_PLAYBACK_LOW_LATENCY,
    USECASE_AUDIO_PLAYBACK_MULTI_CH,
    USECASE_AUDIO_PLAYBACK_OFFLOAD,

    /* HFP Use case*/
    USECASE_AUDIO_HFP_SCO,
    USECASE_AUDIO_HFP_SCO_WB,

    /* Capture usecases */
    USECASE_AUDIO_RECORD,
    USECASE_AUDIO_RECORD_LOW_LATENCY,

    USECASE_VOICE_CALL,

    /* Voice extension usecases */
    USECASE_VOICE2_CALL,
    USECASE_VOLTE_CALL,
    USECASE_QCHAT_CALL,
    USECASE_VOWLAN_CALL,
    USECASE_INCALL_REC_UPLINK,
    USECASE_INCALL_REC_DOWNLINK,
    USECASE_INCALL_REC_UPLINK_AND_DOWNLINK,

    USECASE_AUDIO_PLAYBACK_AFE_PROXY,
    USECASE_AUDIO_RECORD_AFE_PROXY,

    AUDIO_USECASE_MAX
};

const char * const use_case_table[AUDIO_USECASE_MAX];

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

/*
 * tinyAlsa library interprets period size as number of frames
 * one frame = channel_count * sizeof (pcm sample)
 * so if format = 16-bit PCM and channels = Stereo, frame size = 2 ch * 2 = 4 bytes
 * DEEP_BUFFER_OUTPUT_PERIOD_SIZE = 1024 means 1024 * 4 = 4096 bytes
 * We should take care of returning proper size when AudioFlinger queries for
 * the buffer size of an input/output stream
 */

enum {
    OFFLOAD_CMD_EXIT,               /* exit compress offload thread loop*/
    OFFLOAD_CMD_DRAIN,              /* send a full drain request to DSP */
    OFFLOAD_CMD_PARTIAL_DRAIN,      /* send a partial drain request to DSP */
    OFFLOAD_CMD_WAIT_FOR_BUFFER,    /* wait for buffer released by DSP */
};

enum {
    OFFLOAD_STATE_IDLE,
    OFFLOAD_STATE_PLAYING,
    OFFLOAD_STATE_PAUSED,
};

struct offload_cmd {
    struct listnode node;
    int cmd;
    int data[];
};

struct stream_out {
    struct audio_stream_out stream;
    pthread_mutex_t lock; /* see note below on mutex acquisition order */
    pthread_cond_t  cond;
    struct pcm_config config;
    struct compr_config compr_config;
    struct pcm *pcm;
    struct compress *compr;
    int standby;
    int pcm_device_id;
    unsigned int sample_rate;
    audio_channel_mask_t channel_mask;
    audio_format_t format;
    audio_devices_t devices;
    audio_output_flags_t flags;
    audio_usecase_t usecase;
    /* Array of supported channel mask configurations. +1 so that the last entry is always 0 */
    audio_channel_mask_t supported_channel_masks[MAX_SUPPORTED_CHANNEL_MASKS + 1];
    bool muted;
    uint64_t written; /* total frames written, not cleared when entering standby */
    audio_io_handle_t handle;

    int non_blocking;
    int playback_started;
    int offload_state;
    pthread_cond_t offload_cond;
    pthread_t offload_thread;
    struct listnode offload_cmd_list;
    bool offload_thread_blocked;

    stream_callback_t offload_callback;
    void *offload_cookie;
    struct compr_gapless_mdata gapless_mdata;
    int send_new_metadata;

    struct audio_device *dev;
};

struct stream_in {
    struct audio_stream_in stream;
    pthread_mutex_t lock; /* see note below on mutex acquisition order */
    struct pcm_config config;
    struct pcm *pcm;
    int standby;
    int source;
    int pcm_device_id;
    audio_devices_t device;
    audio_channel_mask_t channel_mask;
    audio_usecase_t usecase;
    bool enable_aec;
    bool enable_ns;

    struct audio_device *dev;
};

typedef enum {
    PCM_PLAYBACK,
    PCM_CAPTURE,
    VOICE_CALL,
    PCM_HFP_CALL
} usecase_type_t;

union stream_ptr {
    struct stream_in *in;
    struct stream_out *out;
};

struct audio_usecase {
    struct listnode list;
    audio_usecase_t id;
    usecase_type_t  type;
    audio_devices_t devices;
    snd_device_t out_snd_device;
    snd_device_t in_snd_device;
    union stream_ptr stream;
};

struct audio_device {
    struct audio_hw_device device;
    pthread_mutex_t lock; /* see note below on mutex acquisition order */
    struct mixer *mixer;
    audio_mode_t mode;
    struct stream_in *active_input;
    struct stream_out *primary_output;
    struct stream_out *voice_tx_output;
    struct stream_out *current_call_output;
    bool bluetooth_nrec;
    bool screen_off;
    int *snd_dev_ref_cnt;
    struct listnode usecase_list;
    struct audio_route *audio_route;
    int acdb_settings;
    bool speaker_lr_swap;
    struct voice voice;
    unsigned int cur_hdmi_channels;
    bool bt_wb_speech_enabled;
    bool mic_muted;
    bool enable_voicerx;

    int snd_card;
    void *platform;
    void *extspk;

    void *visualizer_lib;
    int (*visualizer_start_output)(audio_io_handle_t, int);
    int (*visualizer_stop_output)(audio_io_handle_t, int);

    /* The pcm_params use_case_table is loaded by adev_verify_devices() upon
     * calling adev_open().
     *
     * If an entry is not NULL, it can be used to determine if extended precision
     * or other capabilities are present for the device corresponding to that usecase.
     */
    struct pcm_params *use_case_table[AUDIO_USECASE_MAX];
    void *offload_effects_lib;
    int (*offload_effects_start_output)(audio_io_handle_t, int);
    int (*offload_effects_stop_output)(audio_io_handle_t, int);
};

int select_devices(struct audio_device *adev,
                   audio_usecase_t uc_id);

int disable_audio_route(struct audio_device *adev,
                        struct audio_usecase *usecase);

int disable_snd_device(struct audio_device *adev,
                       snd_device_t snd_device);

int enable_snd_device(struct audio_device *adev,
                      snd_device_t snd_device);

int enable_audio_route(struct audio_device *adev,
                       struct audio_usecase *usecase);

struct audio_usecase *get_usecase_from_list(struct audio_device *adev,
                                            audio_usecase_t uc_id);

#define LITERAL_TO_STRING(x) #x
#define CHECK(condition) LOG_ALWAYS_FATAL_IF(!(condition), "%s",\
            __FILE__ ":" LITERAL_TO_STRING(__LINE__)\
            " ASSERT_FATAL(" #condition ") failed.")

/*
 * NOTE: when multiple mutexes have to be acquired, always take the
 * stream_in or stream_out mutex first, followed by the audio_device mutex.
 */

#endif // QCOM_AUDIO_HW_H
