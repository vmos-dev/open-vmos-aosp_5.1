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

#define LOG_TAG "voice"
/*#define LOG_NDEBUG 0*/
#define LOG_NDDEBUG 0

#include <errno.h>
#include <math.h>
#include <cutils/log.h>
#include <cutils/str_parms.h>

#include "audio_hw.h"
#include "voice.h"
#include "voice_extn/voice_extn.h"
#include "platform.h"
#include "platform_api.h"

struct pcm_config pcm_config_voice_call = {
    .channels = 1,
    .rate = 8000,
    .period_size = 160,
    .period_count = 2,
    .format = PCM_FORMAT_S16_LE,
};

static struct voice_session *voice_get_session_from_use_case(struct audio_device *adev,
                              audio_usecase_t usecase_id)
{
    struct voice_session *session = NULL;
    int ret = 0;

    ret = voice_extn_get_session_from_use_case(adev, usecase_id, &session);
    if (ret == -ENOSYS) {
        session = &adev->voice.session[VOICE_SESS_IDX];
    }

    return session;
}

int voice_stop_usecase(struct audio_device *adev, audio_usecase_t usecase_id)
{
    int i, ret = 0;
    struct audio_usecase *uc_info;
    struct voice_session *session = NULL;

    ALOGD("%s: enter usecase:%s", __func__, use_case_table[usecase_id]);

    session = (struct voice_session *)voice_get_session_from_use_case(adev, usecase_id);
    session->state.current = CALL_INACTIVE;

    ret = platform_stop_voice_call(adev->platform, session->vsid);

    /* 1. Close the PCM devices */
    if (session->pcm_rx) {
        pcm_close(session->pcm_rx);
        session->pcm_rx = NULL;
    }
    if (session->pcm_tx) {
        pcm_close(session->pcm_tx);
        session->pcm_tx = NULL;
    }

    uc_info = get_usecase_from_list(adev, usecase_id);
    if (uc_info == NULL) {
        ALOGE("%s: Could not find the usecase (%d) in the list",
              __func__, usecase_id);
        return -EINVAL;
    }

    /* 2. Get and set stream specific mixer controls */
    disable_audio_route(adev, uc_info);

    /* 3. Disable the rx and tx devices */
    disable_snd_device(adev, uc_info->out_snd_device);
    disable_snd_device(adev, uc_info->in_snd_device);

    list_remove(&uc_info->list);
    free(uc_info);

    ALOGD("%s: exit: status(%d)", __func__, ret);
    return ret;
}

int voice_start_usecase(struct audio_device *adev, audio_usecase_t usecase_id)
{
    int i, ret = 0;
    struct audio_usecase *uc_info;
    int pcm_dev_rx_id, pcm_dev_tx_id;
    struct voice_session *session = NULL;
    struct pcm_config voice_config = pcm_config_voice_call;

    ALOGD("%s: enter usecase:%s", __func__, use_case_table[usecase_id]);

    session = (struct voice_session *)voice_get_session_from_use_case(adev, usecase_id);
    uc_info = (struct audio_usecase *)calloc(1, sizeof(struct audio_usecase));
    uc_info->id = usecase_id;
    uc_info->type = VOICE_CALL;
    uc_info->stream.out = adev->current_call_output ;
    uc_info->devices = adev->current_call_output ->devices;
    uc_info->in_snd_device = SND_DEVICE_NONE;
    uc_info->out_snd_device = SND_DEVICE_NONE;

    list_add_tail(&adev->usecase_list, &uc_info->list);

    select_devices(adev, usecase_id);

    pcm_dev_rx_id = platform_get_pcm_device_id(uc_info->id, PCM_PLAYBACK);
    pcm_dev_tx_id = platform_get_pcm_device_id(uc_info->id, PCM_CAPTURE);

    if (pcm_dev_rx_id < 0 || pcm_dev_tx_id < 0) {
        ALOGE("%s: Invalid PCM devices (rx: %d tx: %d) for the usecase(%d)",
              __func__, pcm_dev_rx_id, pcm_dev_tx_id, uc_info->id);
        ret = -EIO;
        goto error_start_voice;
    }

    ALOGV("%s: Opening PCM playback device card_id(%d) device_id(%d)",
          __func__, adev->snd_card, pcm_dev_rx_id);
    session->pcm_rx = pcm_open(adev->snd_card,
                               pcm_dev_rx_id,
                               PCM_OUT, &voice_config);
    if (session->pcm_rx && !pcm_is_ready(session->pcm_rx)) {
        ALOGE("%s: %s", __func__, pcm_get_error(session->pcm_rx));
        ret = -EIO;
        goto error_start_voice;
    }

    ALOGV("%s: Opening PCM capture device card_id(%d) device_id(%d)",
          __func__, adev->snd_card, pcm_dev_tx_id);
    session->pcm_tx = pcm_open(adev->snd_card,
                               pcm_dev_tx_id,
                               PCM_IN, &voice_config);
    if (session->pcm_tx && !pcm_is_ready(session->pcm_tx)) {
        ALOGE("%s: %s", __func__, pcm_get_error(session->pcm_tx));
        ret = -EIO;
        goto error_start_voice;
    }
    pcm_start(session->pcm_rx);
    pcm_start(session->pcm_tx);

    voice_set_volume(adev, adev->voice.volume);

    ret = platform_start_voice_call(adev->platform, session->vsid);
    if (ret < 0) {
        ALOGE("%s: platform_start_voice_call error %d\n", __func__, ret);
        goto error_start_voice;
    }

    session->state.current = CALL_ACTIVE;
    goto done;

error_start_voice:
    voice_stop_usecase(adev, usecase_id);

done:
    ALOGD("%s: exit: status(%d)", __func__, ret);
    return ret;
}

bool voice_is_call_state_active(struct audio_device *adev)
{
    bool call_state = false;
    int ret = 0;

    ret = voice_extn_is_call_state_active(adev, &call_state);
    if (ret == -ENOSYS) {
        call_state = (adev->voice.session[VOICE_SESS_IDX].state.current == CALL_ACTIVE) ? true : false;
    }

    return call_state;
}

bool voice_is_in_call(struct audio_device *adev)
{
    return adev->voice.in_call;
}

bool voice_is_in_call_rec_stream(struct stream_in *in)
{
    bool in_call_rec = false;
    int ret = 0;

    ret = voice_extn_is_in_call_rec_stream(in, &in_call_rec);
    if (ret == -ENOSYS) {
        in_call_rec = false;
    }

    return in_call_rec;
}

uint32_t voice_get_active_session_id(struct audio_device *adev)
{
    int ret = 0;
    uint32_t session_id;

    ret = voice_extn_get_active_session_id(adev, &session_id);
    if (ret == -ENOSYS) {
        session_id = VOICE_VSID;
    }
    return session_id;
}

int voice_check_and_set_incall_rec_usecase(struct audio_device *adev,
                                           struct stream_in *in)
{
    int ret = 0;
    uint32_t session_id;
    int usecase_id;
    int rec_mode = INCALL_REC_NONE;

    if (voice_is_call_state_active(adev)) {
        switch (in->source) {
        case AUDIO_SOURCE_VOICE_UPLINK:
            in->usecase = USECASE_INCALL_REC_UPLINK;
            rec_mode = INCALL_REC_UPLINK;
            break;
        case AUDIO_SOURCE_VOICE_DOWNLINK:
            in->usecase = USECASE_INCALL_REC_DOWNLINK;
            rec_mode = INCALL_REC_DOWNLINK;
            break;
        case AUDIO_SOURCE_VOICE_CALL:
            in->usecase = USECASE_INCALL_REC_UPLINK_AND_DOWNLINK;
            rec_mode = INCALL_REC_UPLINK_AND_DOWNLINK;
            break;
        default:
            ALOGV("%s: Source type %d doesnt match incall recording criteria",
                  __func__, in->source);
            return ret;
        }

        session_id = voice_get_active_session_id(adev);
        ret = platform_set_incall_recording_session_id(adev->platform,
                                                       session_id, rec_mode);
        ALOGV("%s: Update usecase to %d",__func__, in->usecase);
    } else {
        ALOGV("%s: voice call not active", __func__);
    }

    return ret;
}

int voice_check_and_stop_incall_rec_usecase(struct audio_device *adev,
                                            struct stream_in *in)
{
    int ret = 0;

    if (in->source == AUDIO_SOURCE_VOICE_UPLINK ||
        in->source == AUDIO_SOURCE_VOICE_DOWNLINK ||
        in->source == AUDIO_SOURCE_VOICE_CALL) {
        ret = platform_stop_incall_recording_usecase(adev->platform);
        ALOGV("%s: Stop In-call recording", __func__);
    }

    return ret;
}

int voice_check_and_set_incall_music_usecase(struct audio_device *adev,
                                             struct stream_out *out)
{
    int ret = 0;

    ret = voice_extn_check_and_set_incall_music_usecase(adev, out);
    if (ret == -ENOSYS) {
        /* Incall music delivery is used only for LCH call state */
        ret = -EINVAL;
    }

    return ret;
}

int voice_set_mic_mute(struct audio_device *adev, bool state)
{
    int err = 0;

    adev->voice.mic_mute = state;
    if (adev->mode == AUDIO_MODE_IN_CALL)
        err = platform_set_mic_mute(adev->platform, state);

    return err;
}

bool voice_get_mic_mute(struct audio_device *adev)
{
    return adev->voice.mic_mute;
}

int voice_set_volume(struct audio_device *adev, float volume)
{
    int vol, err = 0;

    adev->voice.volume = volume;
    if (adev->mode == AUDIO_MODE_IN_CALL) {
        if (volume < 0.0) {
            volume = 0.0;
        } else if (volume > 1.0) {
            volume = 1.0;
        }

        vol = lrint(volume * 100.0);

        // Voice volume levels from android are mapped to driver volume levels as follows.
        // 0 -> 5, 20 -> 4, 40 ->3, 60 -> 2, 80 -> 1, 100 -> 0
        // So adjust the volume to get the correct volume index in driver
        vol = 100 - vol;

        err = platform_set_voice_volume(adev->platform, vol);
    }

    return err;
}

int voice_start_call(struct audio_device *adev)
{
    int ret = 0;

    adev->voice.in_call = true;
    ret = voice_extn_start_call(adev);
    if (ret == -ENOSYS) {
        ret = voice_start_usecase(adev, USECASE_VOICE_CALL);
    }

    return ret;
}

int voice_stop_call(struct audio_device *adev)
{
    int ret = 0;

    adev->voice.in_call = false;
    ret = voice_extn_stop_call(adev);
    if (ret == -ENOSYS) {
        ret = voice_stop_usecase(adev, USECASE_VOICE_CALL);
    }

    return ret;
}

void voice_get_parameters(struct audio_device *adev,
                          struct str_parms *query,
                          struct str_parms *reply)
{
    voice_extn_get_parameters(adev, query, reply);
}

int voice_set_parameters(struct audio_device *adev, struct str_parms *parms)
{
    char *str;
    char value[32];
    int val;
    int ret = 0, err;
    char *kv_pairs = str_parms_to_str(parms);

    ALOGV_IF(kv_pairs != NULL, "%s: enter: %s", __func__, kv_pairs);

    ret = voice_extn_set_parameters(adev, parms);
    if (ret != 0) {
        if (ret == -ENOSYS) {
            ret = 0; /* ignore error */
        } else {
            goto done;
        }
    }

    err = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_TTY_MODE, value, sizeof(value));
    if (err >= 0) {
        int tty_mode;
        str_parms_del(parms, AUDIO_PARAMETER_KEY_TTY_MODE);
        if (strcmp(value, AUDIO_PARAMETER_VALUE_TTY_OFF) == 0)
            tty_mode = TTY_MODE_OFF;
        else if (strcmp(value, AUDIO_PARAMETER_VALUE_TTY_VCO) == 0)
            tty_mode = TTY_MODE_VCO;
        else if (strcmp(value, AUDIO_PARAMETER_VALUE_TTY_HCO) == 0)
            tty_mode = TTY_MODE_HCO;
        else if (strcmp(value, AUDIO_PARAMETER_VALUE_TTY_FULL) == 0)
            tty_mode = TTY_MODE_FULL;
        else {
            ret = -EINVAL;
            goto done;
        }

        if (tty_mode != adev->voice.tty_mode) {
            adev->voice.tty_mode = tty_mode;
            adev->acdb_settings = (adev->acdb_settings & TTY_MODE_CLEAR) | tty_mode;
            if (voice_is_call_state_active(adev))
               voice_update_devices_for_all_voice_usecases(adev);
        }
    }

    err = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_HAC,
                            value, sizeof(value));
    if (err >= 0) {
        bool hac = false;
        str_parms_del(parms, AUDIO_PARAMETER_KEY_HAC);
        if (strcmp(value, AUDIO_PARAMETER_VALUE_HAC_ON) == 0)
            hac = true;

        if (hac != adev->voice.hac) {
            adev->voice.hac = hac;
            if (voice_is_in_call(adev))
                voice_update_devices_for_all_voice_usecases(adev);
        }
     }

    err = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_INCALLMUSIC,
                            value, sizeof(value));
    if (err >= 0) {
        str_parms_del(parms, AUDIO_PARAMETER_KEY_INCALLMUSIC);
        if (strcmp(value, AUDIO_PARAMETER_VALUE_TRUE) == 0)
            platform_start_incall_music_usecase(adev->platform);
        else
            platform_stop_incall_music_usecase(adev->platform);
     }

done:
    ALOGV("%s: exit with code(%d)", __func__, ret);
    free(kv_pairs);
    return ret;
}

void voice_init(struct audio_device *adev)
{
    int i = 0;

    memset(&adev->voice, 0, sizeof(adev->voice));
    adev->voice.tty_mode = TTY_MODE_OFF;
    adev->voice.hac = false;
    adev->voice.volume = 1.0f;
    adev->voice.mic_mute = false;
    adev->voice.in_call = false;
    for (i = 0; i < MAX_VOICE_SESSIONS; i++) {
        adev->voice.session[i].pcm_rx = NULL;
        adev->voice.session[i].pcm_tx = NULL;
        adev->voice.session[i].state.current = CALL_INACTIVE;
        adev->voice.session[i].state.new = CALL_INACTIVE;
        adev->voice.session[i].vsid = VOICE_VSID;
    }

    voice_extn_init(adev);
}

void voice_update_devices_for_all_voice_usecases(struct audio_device *adev)
{
    struct listnode *node;
    struct audio_usecase *usecase;

    list_for_each(node, &adev->usecase_list) {
        usecase = node_to_item(node, struct audio_usecase, list);
        if (usecase->type == VOICE_CALL) {
            ALOGV("%s: updating device for usecase:%s", __func__,
                  use_case_table[usecase->id]);
            usecase->stream.out = adev->current_call_output;
            select_devices(adev, usecase->id);
        }
    }
}


