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

#define LOG_TAG "offload_effect_virtualizer"
//#define LOG_NDEBUG 0

#include <cutils/list.h>
#include <cutils/log.h>
#include <tinyalsa/asoundlib.h>
#include <sound/audio_effects.h>
#include <audio_effects/effect_virtualizer.h>

#include "effect_api.h"
#include "virtualizer.h"

/* Offload Virtualizer UUID: 509a4498-561a-4bea-b3b1-0002a5d5c51b */
const effect_descriptor_t virtualizer_descriptor = {
        {0x37cc2c00, 0xdddd, 0x11db, 0x8577, {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}},
        {0x509a4498, 0x561a, 0x4bea, 0xb3b1, {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}}, // uuid
        EFFECT_CONTROL_API_VERSION,
        (EFFECT_FLAG_TYPE_INSERT | EFFECT_FLAG_DEVICE_IND | EFFECT_FLAG_HW_ACC_TUNNEL),
        0, /* TODO */
        1,
        "MSM offload virtualizer",
        "The Android Open Source Project",
};

/*
 * Virtualizer operations
 */

int virtualizer_get_strength(virtualizer_context_t *context)
{
    ALOGV("%s: strength: %d", __func__, context->strength);
    return context->strength;
}

int virtualizer_set_strength(virtualizer_context_t *context, uint32_t strength)
{
    ALOGV("%s: strength: %d", __func__, strength);
    context->strength = strength;

    offload_virtualizer_set_strength(&(context->offload_virt), strength);
    if (context->ctl)
        offload_virtualizer_send_params(context->ctl, &context->offload_virt,
                                        OFFLOAD_SEND_VIRTUALIZER_ENABLE_FLAG |
                                        OFFLOAD_SEND_VIRTUALIZER_STRENGTH);
    return 0;
}

int virtualizer_get_parameter(effect_context_t *context, effect_param_t *p,
                              uint32_t *size)
{
    virtualizer_context_t *virt_ctxt = (virtualizer_context_t *)context;
    int voffset = ((p->psize - 1) / sizeof(int32_t) + 1) * sizeof(int32_t);
    int32_t *param_tmp = (int32_t *)p->data;
    int32_t param = *param_tmp++;
    void *value = p->data + voffset;
    int i;

    ALOGV("%s", __func__);

    p->status = 0;

    switch (param) {
    case VIRTUALIZER_PARAM_STRENGTH_SUPPORTED:
        if (p->vsize < sizeof(uint32_t))
           p->status = -EINVAL;
        p->vsize = sizeof(uint32_t);
        break;
    case VIRTUALIZER_PARAM_STRENGTH:
        if (p->vsize < sizeof(int16_t))
           p->status = -EINVAL;
        p->vsize = sizeof(int16_t);
        break;
    default:
        p->status = -EINVAL;
    }

    *size = sizeof(effect_param_t) + voffset + p->vsize;

    if (p->status != 0)
        return 0;

    switch (param) {
    case VIRTUALIZER_PARAM_STRENGTH_SUPPORTED:
	ALOGV("%s: VIRTUALIZER_PARAM_STRENGTH_SUPPORTED", __func__);
        *(uint32_t *)value = 1;
        break;

    case VIRTUALIZER_PARAM_STRENGTH:
	ALOGV("%s: VIRTUALIZER_PARAM_STRENGTH", __func__);
        *(int16_t *)value = virtualizer_get_strength(virt_ctxt);
        break;

    default:
        p->status = -EINVAL;
        break;
    }

    return 0;
}

int virtualizer_set_parameter(effect_context_t *context, effect_param_t *p,
                              uint32_t size __unused)
{
    virtualizer_context_t *virt_ctxt = (virtualizer_context_t *)context;
    int voffset = ((p->psize - 1) / sizeof(int32_t) + 1) * sizeof(int32_t);
    void *value = p->data + voffset;
    int32_t *param_tmp = (int32_t *)p->data;
    int32_t param = *param_tmp++;
    uint32_t strength;

    ALOGV("%s", __func__);

    p->status = 0;

    switch (param) {
    case VIRTUALIZER_PARAM_STRENGTH:
	ALOGV("%s VIRTUALIZER_PARAM_STRENGTH", __func__);
        strength = (uint32_t)(*(int16_t *)value);
        virtualizer_set_strength(virt_ctxt, strength);
        break;
    default:
        p->status = -EINVAL;
        break;
    }

    return 0;
}

int virtualizer_set_device(effect_context_t *context, uint32_t device)
{
    virtualizer_context_t *virt_ctxt = (virtualizer_context_t *)context;

    ALOGV("%s: device: %d", __func__, device);
    virt_ctxt->device = device;
    if ((device == AUDIO_DEVICE_OUT_SPEAKER) ||
       (device == AUDIO_DEVICE_OUT_BLUETOOTH_SCO_CARKIT) ||
       (device == AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_SPEAKER) ||
       (device == AUDIO_DEVICE_OUT_AUX_DIGITAL) ||
       (device == AUDIO_DEVICE_OUT_ANLG_DOCK_HEADSET)) {
        if (!virt_ctxt->temp_disabled) {
            if (effect_is_active(&virt_ctxt->common)) {
                offload_virtualizer_set_enable_flag(&(virt_ctxt->offload_virt), false);
                if (virt_ctxt->ctl)
                    offload_virtualizer_send_params(virt_ctxt->ctl,
                                                    &virt_ctxt->offload_virt,
                                                    OFFLOAD_SEND_VIRTUALIZER_ENABLE_FLAG);
            }
            virt_ctxt->temp_disabled = true;
        }
    } else {
        if (virt_ctxt->temp_disabled) {
            if (effect_is_active(&virt_ctxt->common)) {
                offload_virtualizer_set_enable_flag(&(virt_ctxt->offload_virt), true);
                if (virt_ctxt->ctl)
                    offload_virtualizer_send_params(virt_ctxt->ctl,
                                                    &virt_ctxt->offload_virt,
                                                    OFFLOAD_SEND_VIRTUALIZER_ENABLE_FLAG);
            }
            virt_ctxt->temp_disabled = false;
        }
    }
    offload_virtualizer_set_device(&(virt_ctxt->offload_virt), device);
    return 0;
}

int virtualizer_reset(effect_context_t *context)
{
    virtualizer_context_t *virt_ctxt = (virtualizer_context_t *)context;

    return 0;
}

int virtualizer_init(effect_context_t *context)
{
    ALOGV("%s", __func__);
    virtualizer_context_t *virt_ctxt = (virtualizer_context_t *)context;

    context->config.inputCfg.accessMode = EFFECT_BUFFER_ACCESS_READ;
    context->config.inputCfg.channels = AUDIO_CHANNEL_OUT_STEREO;
    context->config.inputCfg.format = AUDIO_FORMAT_PCM_16_BIT;
    context->config.inputCfg.samplingRate = 44100;
    context->config.inputCfg.bufferProvider.getBuffer = NULL;
    context->config.inputCfg.bufferProvider.releaseBuffer = NULL;
    context->config.inputCfg.bufferProvider.cookie = NULL;
    context->config.inputCfg.mask = EFFECT_CONFIG_ALL;
    context->config.outputCfg.accessMode = EFFECT_BUFFER_ACCESS_ACCUMULATE;
    context->config.outputCfg.channels = AUDIO_CHANNEL_OUT_STEREO;
    context->config.outputCfg.format = AUDIO_FORMAT_PCM_16_BIT;
    context->config.outputCfg.samplingRate = 44100;
    context->config.outputCfg.bufferProvider.getBuffer = NULL;
    context->config.outputCfg.bufferProvider.releaseBuffer = NULL;
    context->config.outputCfg.bufferProvider.cookie = NULL;
    context->config.outputCfg.mask = EFFECT_CONFIG_ALL;

    set_config(context, &context->config);

    virt_ctxt->temp_disabled = false;
    memset(&(virt_ctxt->offload_virt), 0, sizeof(struct virtualizer_params));

    return 0;
}

int virtualizer_enable(effect_context_t *context)
{
    virtualizer_context_t *virt_ctxt = (virtualizer_context_t *)context;

    ALOGV("%s", __func__);

    if (!offload_virtualizer_get_enable_flag(&(virt_ctxt->offload_virt)) &&
        !(virt_ctxt->temp_disabled)) {
        offload_virtualizer_set_enable_flag(&(virt_ctxt->offload_virt), true);
        if (virt_ctxt->ctl && virt_ctxt->strength)
            offload_virtualizer_send_params(virt_ctxt->ctl,
                                          &virt_ctxt->offload_virt,
                                          OFFLOAD_SEND_VIRTUALIZER_ENABLE_FLAG |
                                          OFFLOAD_SEND_BASSBOOST_STRENGTH);
    }
    return 0;
}

int virtualizer_disable(effect_context_t *context)
{
    virtualizer_context_t *virt_ctxt = (virtualizer_context_t *)context;

    ALOGV("%s", __func__);
    if (offload_virtualizer_get_enable_flag(&(virt_ctxt->offload_virt))) {
        offload_virtualizer_set_enable_flag(&(virt_ctxt->offload_virt), false);
        if (virt_ctxt->ctl)
            offload_virtualizer_send_params(virt_ctxt->ctl,
                                          &virt_ctxt->offload_virt,
                                          OFFLOAD_SEND_VIRTUALIZER_ENABLE_FLAG);
    }
    return 0;
}

int virtualizer_start(effect_context_t *context, output_context_t *output)
{
    virtualizer_context_t *virt_ctxt = (virtualizer_context_t *)context;

    ALOGV("%s", __func__);
    virt_ctxt->ctl = output->ctl;
    if (offload_virtualizer_get_enable_flag(&(virt_ctxt->offload_virt)))
        if (virt_ctxt->ctl)
            offload_virtualizer_send_params(virt_ctxt->ctl, &virt_ctxt->offload_virt,
                                          OFFLOAD_SEND_VIRTUALIZER_ENABLE_FLAG |
                                          OFFLOAD_SEND_VIRTUALIZER_STRENGTH);
    return 0;
}

int virtualizer_stop(effect_context_t *context, output_context_t *output __unused)
{
    virtualizer_context_t *virt_ctxt = (virtualizer_context_t *)context;

    ALOGV("%s", __func__);
    virt_ctxt->ctl = NULL;
    return 0;
}
