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

#define LOG_TAG "audio_hw_hfp"
/*#define LOG_NDEBUG 0*/
#define LOG_NDDEBUG 0

#include <errno.h>
#include <math.h>
#include <cutils/log.h>

#include "audio_hw.h"
#include "platform.h"
#include "platform_api.h"
#include <stdlib.h>
#include <cutils/str_parms.h>

#define AUDIO_PARAMETER_HFP_ENABLE      "hfp_enable"
#define AUDIO_PARAMETER_HFP_SET_SAMPLING_RATE "hfp_set_sampling_rate"
#define AUDIO_PARAMETER_KEY_HFP_VOLUME "hfp_volume"

static int32_t start_hfp(struct audio_device *adev,
                               struct str_parms *parms);

static int32_t stop_hfp(struct audio_device *adev);

struct hfp_module {
    struct pcm *hfp_sco_rx;
    struct pcm *hfp_sco_tx;
    struct pcm *hfp_pcm_rx;
    struct pcm *hfp_pcm_tx;
    float  hfp_volume;
    bool   is_hfp_running;
    audio_usecase_t ucid;
};

static struct hfp_module hfpmod = {
    .hfp_sco_rx = NULL,
    .hfp_sco_tx = NULL,
    .hfp_pcm_rx = NULL,
    .hfp_pcm_tx = NULL,
    .hfp_volume = 0,
    .is_hfp_running = 0,
    .ucid = USECASE_AUDIO_HFP_SCO,
};
static struct pcm_config pcm_config_hfp = {
    .channels = 1,
    .rate = 8000,
    .period_size = 240,
    .period_count = 2,
    .format = PCM_FORMAT_S16_LE,
    .start_threshold = 0,
    .stop_threshold = INT_MAX,
    .avail_min = 0,
};

static int32_t hfp_set_volume(struct audio_device *adev, float value)
{
    int32_t vol, ret = 0;
    struct mixer_ctl *ctl;
    const char *mixer_ctl_name = "Internal HFP RX Volume";

    ALOGV("%s: entry", __func__);
    ALOGD("%s: (%f)\n", __func__, value);

    if (value < 0.0) {
        ALOGW("%s: (%f) Under 0.0, assuming 0.0\n", __func__, value);
        value = 0.0;
    } else {
        value = ((value > 15.000000) ? 1.0 : (value / 15));
        ALOGW("%s: Volume brought with in range (%f)\n", __func__, value);
    }
    vol  = lrint((value * 0x2000) + 0.5);
    hfpmod.hfp_volume = value;

    if (!hfpmod.is_hfp_running) {
        ALOGV("%s: HFP not active, ignoring set_hfp_volume call", __func__);
        return -EIO;
    }

    ALOGD("%s: Setting HFP volume to %d \n", __func__, vol);
    ctl = mixer_get_ctl_by_name(adev->mixer, mixer_ctl_name);
    if (!ctl) {
        ALOGE("%s: Could not get ctl for mixer cmd - %s",
              __func__, mixer_ctl_name);
        return -EINVAL;
    }
    if(mixer_ctl_set_value(ctl, 0, vol) < 0) {
        ALOGE("%s: Couldn't set HFP Volume: [%d]", __func__, vol);
        return -EINVAL;
    }

    ALOGV("%s: exit", __func__);
    return ret;
}

static int32_t start_hfp(struct audio_device *adev,
                         struct str_parms *parms __unused)
{
    int32_t i, ret = 0;
    struct audio_usecase *uc_info;
    int32_t pcm_dev_rx_id, pcm_dev_tx_id, pcm_dev_asm_rx_id, pcm_dev_asm_tx_id;

    ALOGD("%s: enter", __func__);

    uc_info = (struct audio_usecase *)calloc(1, sizeof(struct audio_usecase));
    uc_info->id = hfpmod.ucid;
    uc_info->type = PCM_HFP_CALL;
    uc_info->stream.out = adev->primary_output;
    uc_info->devices = adev->primary_output->devices;
    uc_info->in_snd_device = SND_DEVICE_NONE;
    uc_info->out_snd_device = SND_DEVICE_NONE;

    list_add_tail(&adev->usecase_list, &uc_info->list);

    select_devices(adev, hfpmod.ucid);

    pcm_dev_rx_id = platform_get_pcm_device_id(uc_info->id, PCM_PLAYBACK);
    pcm_dev_tx_id = platform_get_pcm_device_id(uc_info->id, PCM_CAPTURE);
    pcm_dev_asm_rx_id = HFP_ASM_RX_TX;
    pcm_dev_asm_tx_id = HFP_ASM_RX_TX;
    if (pcm_dev_rx_id < 0 || pcm_dev_tx_id < 0 ||
        pcm_dev_asm_rx_id < 0 || pcm_dev_asm_tx_id < 0 ) {
        ALOGE("%s: Invalid PCM devices (rx: %d tx: %d asm: rx tx %d) for the usecase(%d)",
              __func__, pcm_dev_rx_id, pcm_dev_tx_id, pcm_dev_asm_rx_id, uc_info->id);
        ret = -EIO;
        goto exit;
    }

    ALOGV("%s: HFP PCM devices (hfp rx tx: %d pcm rx tx: %d) for the usecase(%d)",
              __func__, pcm_dev_rx_id, pcm_dev_tx_id, uc_info->id);

    ALOGV("%s: Opening PCM playback device card_id(%d) device_id(%d)",
          __func__, adev->snd_card, pcm_dev_rx_id);
    hfpmod.hfp_sco_rx = pcm_open(adev->snd_card,
                                  pcm_dev_asm_rx_id,
                                  PCM_OUT, &pcm_config_hfp);
    if (hfpmod.hfp_sco_rx && !pcm_is_ready(hfpmod.hfp_sco_rx)) {
        ALOGE("%s: %s", __func__, pcm_get_error(hfpmod.hfp_sco_rx));
        ret = -EIO;
        goto exit;
    }
    ALOGD("%s: Opening PCM capture device card_id(%d) device_id(%d)",
          __func__, adev->snd_card, pcm_dev_tx_id);
    hfpmod.hfp_pcm_rx = pcm_open(adev->snd_card,
                                   pcm_dev_rx_id,
                                   PCM_OUT, &pcm_config_hfp);
    if (hfpmod.hfp_pcm_rx && !pcm_is_ready(hfpmod.hfp_pcm_rx)) {
        ALOGE("%s: %s", __func__, pcm_get_error(hfpmod.hfp_pcm_rx));
        ret = -EIO;
        goto exit;
    }
    hfpmod.hfp_sco_tx = pcm_open(adev->snd_card,
                                  pcm_dev_asm_tx_id,
                                  PCM_IN, &pcm_config_hfp);
    if (hfpmod.hfp_sco_tx && !pcm_is_ready(hfpmod.hfp_sco_tx)) {
        ALOGE("%s: %s", __func__, pcm_get_error(hfpmod.hfp_sco_tx));
        ret = -EIO;
        goto exit;
    }
    ALOGV("%s: Opening PCM capture device card_id(%d) device_id(%d)",
          __func__, adev->snd_card, pcm_dev_tx_id);
    hfpmod.hfp_pcm_tx = pcm_open(adev->snd_card,
                                   pcm_dev_tx_id,
                                   PCM_IN, &pcm_config_hfp);
    if (hfpmod.hfp_pcm_tx && !pcm_is_ready(hfpmod.hfp_pcm_tx)) {
        ALOGE("%s: %s", __func__, pcm_get_error(hfpmod.hfp_pcm_tx));
        ret = -EIO;
        goto exit;
    }
    pcm_start(hfpmod.hfp_sco_rx);
    pcm_start(hfpmod.hfp_sco_tx);
    pcm_start(hfpmod.hfp_pcm_rx);
    pcm_start(hfpmod.hfp_pcm_tx);

    hfpmod.is_hfp_running = true;
    hfp_set_volume(adev, hfpmod.hfp_volume);

    ALOGD("%s: exit: status(%d)", __func__, ret);
    return 0;

exit:
    stop_hfp(adev);
    ALOGE("%s: Problem in HFP start: status(%d)", __func__, ret);
    return ret;
}

static int32_t stop_hfp(struct audio_device *adev)
{
    int32_t i, ret = 0;
    struct audio_usecase *uc_info;

    ALOGD("%s: enter", __func__);
    hfpmod.is_hfp_running = false;

    /* 1. Close the PCM devices */
    if (hfpmod.hfp_sco_rx) {
        pcm_close(hfpmod.hfp_sco_rx);
        hfpmod.hfp_sco_rx = NULL;
    }
    if (hfpmod.hfp_sco_tx) {
        pcm_close(hfpmod.hfp_sco_tx);
        hfpmod.hfp_sco_tx = NULL;
    }
    if (hfpmod.hfp_pcm_rx) {
        pcm_close(hfpmod.hfp_pcm_rx);
        hfpmod.hfp_pcm_rx = NULL;
    }
    if (hfpmod.hfp_pcm_tx) {
        pcm_close(hfpmod.hfp_pcm_tx);
        hfpmod.hfp_pcm_tx = NULL;
    }

    uc_info = get_usecase_from_list(adev, hfpmod.ucid);
    if (uc_info == NULL) {
        ALOGE("%s: Could not find the usecase (%d) in the list",
              __func__, hfpmod.ucid);
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

bool audio_extn_hfp_is_active(struct audio_device *adev)
{
    struct audio_usecase *hfp_usecase = NULL;
    hfp_usecase = get_usecase_from_list(adev, hfpmod.ucid);

    if (hfp_usecase != NULL)
        return true;
    else
        return false;
}

audio_usecase_t audio_extn_hfp_get_usecase()
{
    return hfpmod.ucid;
}

void audio_extn_hfp_set_parameters(struct audio_device *adev, struct str_parms *parms)
{
    int ret;
    int rate;
    int val;
    float vol;
    char value[32]={0};

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_HFP_ENABLE, value,
                            sizeof(value));
    if (ret >= 0) {
           if (!strncmp(value,"true",sizeof(value)))
               ret = start_hfp(adev,parms);
           else
               stop_hfp(adev);
    }
    memset(value, 0, sizeof(value));
    ret = str_parms_get_str(parms,AUDIO_PARAMETER_HFP_SET_SAMPLING_RATE, value,
                            sizeof(value));
    if (ret >= 0) {
           rate = atoi(value);
           if (rate == 8000){
               hfpmod.ucid = USECASE_AUDIO_HFP_SCO;
               pcm_config_hfp.rate = rate;
           } else if (rate == 16000){
               hfpmod.ucid = USECASE_AUDIO_HFP_SCO_WB;
               pcm_config_hfp.rate = rate;
           } else
               ALOGE("Unsupported rate..");
    }

    if (hfpmod.is_hfp_running) {
        memset(value, 0, sizeof(value));
        ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_ROUTING,
                                value, sizeof(value));
        if (ret >= 0) {
            val = atoi(value);
            if (val > 0)
                select_devices(adev, hfpmod.ucid);
        }
    }

    memset(value, 0, sizeof(value));
    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_HFP_VOLUME,
                            value, sizeof(value));
    if (ret >= 0) {
        if (sscanf(value, "%f", &vol) != 1){
            ALOGE("%s: error in retrieving hfp volume", __func__);
            ret = -EIO;
            goto exit;
        }
        ALOGD("%s: set_hfp_volume usecase, Vol: [%f]", __func__, vol);
        hfp_set_volume(adev, vol);
    }
exit:
    ALOGV("%s Exit",__func__);
}
