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

#define LOG_TAG "offload_effect_bundle"
//#define LOG_NDEBUG 0

#include <cutils/list.h>
#include <cutils/log.h>
#include <system/thread_defs.h>
#include <tinyalsa/asoundlib.h>
#include <hardware/audio_effect.h>

#include "bundle.h"
#include "equalizer.h"
#include "bass_boost.h"
#include "virtualizer.h"
#include "reverb.h"

enum {
    EFFECT_STATE_UNINITIALIZED,
    EFFECT_STATE_INITIALIZED,
    EFFECT_STATE_ACTIVE,
};

const effect_descriptor_t *descriptors[] = {
        &equalizer_descriptor,
        &bassboost_descriptor,
        &virtualizer_descriptor,
        &aux_env_reverb_descriptor,
        &ins_env_reverb_descriptor,
        &aux_preset_reverb_descriptor,
        &ins_preset_reverb_descriptor,
        NULL,
};

pthread_once_t once = PTHREAD_ONCE_INIT;
int init_status;
/*
 * list of created effects.
 * Updated by offload_effects_bundle_hal_start_output()
 * and offload_effects_bundle_hal_stop_output()
 */
struct listnode created_effects_list;
/*
 * list of active output streams.
 * Updated by offload_effects_bundle_hal_start_output()
 * and offload_effects_bundle_hal_stop_output()
 */
struct listnode active_outputs_list;
/*
 * lock must be held when modifying or accessing
 * created_effects_list or active_outputs_list
 */
pthread_mutex_t lock;


/*
 *  Local functions
 */
static void init_once() {
    list_init(&created_effects_list);
    list_init(&active_outputs_list);

    pthread_mutex_init(&lock, NULL);

    init_status = 0;
}

int lib_init()
{
    pthread_once(&once, init_once);
    return init_status;
}

bool effect_exists(effect_context_t *context)
{
    struct listnode *node;

    list_for_each(node, &created_effects_list) {
        effect_context_t *fx_ctxt = node_to_item(node,
                                                 effect_context_t,
                                                 effects_list_node);
        if (fx_ctxt == context) {
            return true;
        }
    }
    return false;
}

output_context_t *get_output(audio_io_handle_t output)
{
    struct listnode *node;

    list_for_each(node, &active_outputs_list) {
        output_context_t *out_ctxt = node_to_item(node,
                                                  output_context_t,
                                                  outputs_list_node);
        if (out_ctxt->handle == output)
            return out_ctxt;
    }
    return NULL;
}

void add_effect_to_output(output_context_t * output, effect_context_t *context)
{
    struct listnode *fx_node;

    list_for_each(fx_node, &output->effects_list) {
        effect_context_t *fx_ctxt = node_to_item(fx_node,
                                                 effect_context_t,
                                                 output_node);
        if (fx_ctxt == context)
            return;
    }
    list_add_tail(&output->effects_list, &context->output_node);
    if (context->ops.start)
        context->ops.start(context, output);

}

void remove_effect_from_output(output_context_t * output,
                               effect_context_t *context)
{
    struct listnode *fx_node;

    list_for_each(fx_node, &output->effects_list) {
        effect_context_t *fx_ctxt = node_to_item(fx_node,
                                                 effect_context_t,
                                                 output_node);
        if (fx_ctxt == context) {
            if (context->ops.stop)
                context->ops.stop(context, output);
            list_remove(&context->output_node);
            return;
        }
    }
}

bool effects_enabled()
{
    struct listnode *out_node;

    list_for_each(out_node, &active_outputs_list) {
        struct listnode *fx_node;
        output_context_t *out_ctxt = node_to_item(out_node,
                                                  output_context_t,
                                                  outputs_list_node);

        list_for_each(fx_node, &out_ctxt->effects_list) {
            effect_context_t *fx_ctxt = node_to_item(fx_node,
                                                     effect_context_t,
                                                     output_node);
            if ((fx_ctxt->state == EFFECT_STATE_ACTIVE) &&
                (fx_ctxt->ops.process != NULL))
                return true;
        }
    }
    return false;
}


/*
 * Interface from audio HAL
 */
__attribute__ ((visibility ("default")))
int offload_effects_bundle_hal_start_output(audio_io_handle_t output, int pcm_id)
{
    int ret = 0;
    struct listnode *node;
    char mixer_string[128];
    output_context_t * out_ctxt = NULL;

    ALOGV("%s output %d pcm_id %d", __func__, output, pcm_id);

    if (lib_init() != 0)
        return init_status;

    pthread_mutex_lock(&lock);
    if (get_output(output) != NULL) {
        ALOGW("%s output already started", __func__);
        ret = -ENOSYS;
        goto exit;
    }

    out_ctxt = (output_context_t *)
                                 malloc(sizeof(output_context_t));
    out_ctxt->handle = output;
    out_ctxt->pcm_device_id = pcm_id;

    /* populate the mixer control to send offload parameters */
    snprintf(mixer_string, sizeof(mixer_string),
             "%s %d", "Audio Effects Config", out_ctxt->pcm_device_id);
    out_ctxt->mixer = mixer_open(MIXER_CARD);
    if (!out_ctxt->mixer) {
        ALOGE("Failed to open mixer");
        out_ctxt->ctl = NULL;
        ret = -EINVAL;
        free(out_ctxt);
        goto exit;
    } else {
        out_ctxt->ctl = mixer_get_ctl_by_name(out_ctxt->mixer, mixer_string);
        if (!out_ctxt->ctl) {
            ALOGE("mixer_get_ctl_by_name failed");
            mixer_close(out_ctxt->mixer);
            out_ctxt->mixer = NULL;
            ret = -EINVAL;
            free(out_ctxt);
            goto exit;
        }
    }

    list_init(&out_ctxt->effects_list);

    list_for_each(node, &created_effects_list) {
        effect_context_t *fx_ctxt = node_to_item(node,
                                                 effect_context_t,
                                                 effects_list_node);
        if (fx_ctxt->out_handle == output) {
            if (fx_ctxt->ops.start)
                fx_ctxt->ops.start(fx_ctxt, out_ctxt);
            list_add_tail(&out_ctxt->effects_list, &fx_ctxt->output_node);
        }
    }
    list_add_tail(&active_outputs_list, &out_ctxt->outputs_list_node);
exit:
    pthread_mutex_unlock(&lock);
    return ret;
}

__attribute__ ((visibility ("default")))
int offload_effects_bundle_hal_stop_output(audio_io_handle_t output, int pcm_id)
{
    int ret;
    struct listnode *node;
    struct listnode *fx_node;
    output_context_t *out_ctxt;

    ALOGV("%s output %d pcm_id %d", __func__, output, pcm_id);

    if (lib_init() != 0)
        return init_status;

    pthread_mutex_lock(&lock);

    out_ctxt = get_output(output);
    if (out_ctxt == NULL) {
        ALOGW("%s output not started", __func__);
        ret = -ENOSYS;
        goto exit;
    }

    if (out_ctxt->mixer)
        mixer_close(out_ctxt->mixer);

    list_for_each(fx_node, &out_ctxt->effects_list) {
        effect_context_t *fx_ctxt = node_to_item(fx_node,
                                                 effect_context_t,
                                                 output_node);
        if (fx_ctxt->ops.stop)
            fx_ctxt->ops.stop(fx_ctxt, out_ctxt);
    }

    list_remove(&out_ctxt->outputs_list_node);

    free(out_ctxt);

exit:
    pthread_mutex_unlock(&lock);
    return ret;
}


/*
 * Effect operations
 */
int set_config(effect_context_t *context, effect_config_t *config)
{
    context->config = *config;

    if (context->ops.reset)
        context->ops.reset(context);

    return 0;
}

void get_config(effect_context_t *context, effect_config_t *config)
{
    *config = context->config;
}


/*
 * Effect Library Interface Implementation
 */
int effect_lib_create(const effect_uuid_t *uuid,
                         int32_t sessionId,
                         int32_t ioId,
                         effect_handle_t *pHandle) {
    int ret;
    int i;

    ALOGV("%s: sessionId: %d, ioId: %d", __func__, sessionId, ioId);
    if (lib_init() != 0)
        return init_status;

    if (pHandle == NULL || uuid == NULL)
        return -EINVAL;

    for (i = 0; descriptors[i] != NULL; i++) {
        if (memcmp(uuid, &descriptors[i]->uuid, sizeof(effect_uuid_t)) == 0)
            break;
    }

    if (descriptors[i] == NULL)
        return -EINVAL;

    effect_context_t *context;
    if (memcmp(uuid, &equalizer_descriptor.uuid,
        sizeof(effect_uuid_t)) == 0) {
        equalizer_context_t *eq_ctxt = (equalizer_context_t *)
                                       calloc(1, sizeof(equalizer_context_t));
        context = (effect_context_t *)eq_ctxt;
        context->ops.init = equalizer_init;
        context->ops.reset = equalizer_reset;
        context->ops.set_parameter = equalizer_set_parameter;
        context->ops.get_parameter = equalizer_get_parameter;
        context->ops.set_device = equalizer_set_device;
        context->ops.enable = equalizer_enable;
        context->ops.disable = equalizer_disable;
        context->ops.start = equalizer_start;
        context->ops.stop = equalizer_stop;

        context->desc = &equalizer_descriptor;
        eq_ctxt->ctl = NULL;
    } else if (memcmp(uuid, &bassboost_descriptor.uuid,
               sizeof(effect_uuid_t)) == 0) {
        bassboost_context_t *bass_ctxt = (bassboost_context_t *)
                                         calloc(1, sizeof(bassboost_context_t));
        context = (effect_context_t *)bass_ctxt;
        context->ops.init = bassboost_init;
        context->ops.reset = bassboost_reset;
        context->ops.set_parameter = bassboost_set_parameter;
        context->ops.get_parameter = bassboost_get_parameter;
        context->ops.set_device = bassboost_set_device;
        context->ops.enable = bassboost_enable;
        context->ops.disable = bassboost_disable;
        context->ops.start = bassboost_start;
        context->ops.stop = bassboost_stop;

        context->desc = &bassboost_descriptor;
        bass_ctxt->ctl = NULL;
    } else if (memcmp(uuid, &virtualizer_descriptor.uuid,
               sizeof(effect_uuid_t)) == 0) {
        virtualizer_context_t *virt_ctxt = (virtualizer_context_t *)
                                           calloc(1, sizeof(virtualizer_context_t));
        context = (effect_context_t *)virt_ctxt;
        context->ops.init = virtualizer_init;
        context->ops.reset = virtualizer_reset;
        context->ops.set_parameter = virtualizer_set_parameter;
        context->ops.get_parameter = virtualizer_get_parameter;
        context->ops.set_device = virtualizer_set_device;
        context->ops.enable = virtualizer_enable;
        context->ops.disable = virtualizer_disable;
        context->ops.start = virtualizer_start;
        context->ops.stop = virtualizer_stop;

        context->desc = &virtualizer_descriptor;
        virt_ctxt->ctl = NULL;
    } else if ((memcmp(uuid, &aux_env_reverb_descriptor.uuid,
                sizeof(effect_uuid_t)) == 0) ||
               (memcmp(uuid, &ins_env_reverb_descriptor.uuid,
                sizeof(effect_uuid_t)) == 0) ||
               (memcmp(uuid, &aux_preset_reverb_descriptor.uuid,
                sizeof(effect_uuid_t)) == 0) ||
               (memcmp(uuid, &ins_preset_reverb_descriptor.uuid,
                sizeof(effect_uuid_t)) == 0)) {
        reverb_context_t *reverb_ctxt = (reverb_context_t *)
                                        calloc(1, sizeof(reverb_context_t));
        context = (effect_context_t *)reverb_ctxt;
        context->ops.init = reverb_init;
        context->ops.reset = reverb_reset;
        context->ops.set_parameter = reverb_set_parameter;
        context->ops.get_parameter = reverb_get_parameter;
        context->ops.set_device = reverb_set_device;
        context->ops.enable = reverb_enable;
        context->ops.disable = reverb_disable;
        context->ops.start = reverb_start;
        context->ops.stop = reverb_stop;

        if (memcmp(uuid, &aux_env_reverb_descriptor.uuid,
                   sizeof(effect_uuid_t)) == 0) {
            context->desc = &aux_env_reverb_descriptor;
            reverb_auxiliary_init(reverb_ctxt);
        } else if (memcmp(uuid, &ins_env_reverb_descriptor.uuid,
                   sizeof(effect_uuid_t)) == 0) {
            context->desc = &ins_env_reverb_descriptor;
            reverb_insert_init(reverb_ctxt);
        } else if (memcmp(uuid, &aux_preset_reverb_descriptor.uuid,
                   sizeof(effect_uuid_t)) == 0) {
            context->desc = &aux_preset_reverb_descriptor;
            reverb_auxiliary_init(reverb_ctxt);
        } else if (memcmp(uuid, &ins_preset_reverb_descriptor.uuid,
                   sizeof(effect_uuid_t)) == 0) {
            context->desc = &ins_preset_reverb_descriptor;
            reverb_preset_init(reverb_ctxt);
        }
        reverb_ctxt->ctl = NULL;
    } else {
        return -EINVAL;
    }

    context->itfe = &effect_interface;
    context->state = EFFECT_STATE_UNINITIALIZED;
    context->out_handle = (audio_io_handle_t)ioId;

    ret = context->ops.init(context);
    if (ret < 0) {
        ALOGW("%s init failed", __func__);
        free(context);
        return ret;
    }

    context->state = EFFECT_STATE_INITIALIZED;

    pthread_mutex_lock(&lock);
    list_add_tail(&created_effects_list, &context->effects_list_node);
    output_context_t *out_ctxt = get_output(ioId);
    if (out_ctxt != NULL)
        add_effect_to_output(out_ctxt, context);
    pthread_mutex_unlock(&lock);

    *pHandle = (effect_handle_t)context;

    ALOGV("%s created context %p", __func__, context);

    return 0;

}

int effect_lib_release(effect_handle_t handle)
{
    effect_context_t *context = (effect_context_t *)handle;
    int status;

    if (lib_init() != 0)
        return init_status;

    ALOGV("%s context %p", __func__, handle);
    pthread_mutex_lock(&lock);
    status = -EINVAL;
    if (effect_exists(context)) {
        output_context_t *out_ctxt = get_output(context->out_handle);
        if (out_ctxt != NULL)
            remove_effect_from_output(out_ctxt, context);
        list_remove(&context->effects_list_node);
        if (context->ops.release)
            context->ops.release(context);
        free(context);
        status = 0;
    }
    pthread_mutex_unlock(&lock);

    return status;
}

int effect_lib_get_descriptor(const effect_uuid_t *uuid,
                              effect_descriptor_t *descriptor)
{
    int i;

    if (lib_init() != 0)
        return init_status;

    if (descriptor == NULL || uuid == NULL) {
        ALOGV("%s called with NULL pointer", __func__);
        return -EINVAL;
    }

    for (i = 0; descriptors[i] != NULL; i++) {
        if (memcmp(uuid, &descriptors[i]->uuid, sizeof(effect_uuid_t)) == 0) {
            *descriptor = *descriptors[i];
            return 0;
        }
    }

    return  -EINVAL;
}


/*
 * Effect Control Interface Implementation
 */

/* Stub function for effect interface: never called for offloaded effects */
int effect_process(effect_handle_t self,
                       audio_buffer_t *inBuffer __unused,
                       audio_buffer_t *outBuffer __unused)
{
    effect_context_t * context = (effect_context_t *)self;
    int status = 0;

    ALOGW("%s Called ?????", __func__);

    pthread_mutex_lock(&lock);
    if (!effect_exists(context)) {
        status = -ENOSYS;
        goto exit;
    }

    if (context->state != EFFECT_STATE_ACTIVE) {
        status = -ENODATA;
        goto exit;
    }

exit:
    pthread_mutex_unlock(&lock);
    return status;
}

int effect_command(effect_handle_t self, uint32_t cmdCode, uint32_t cmdSize,
                   void *pCmdData, uint32_t *replySize, void *pReplyData)
{

    effect_context_t * context = (effect_context_t *)self;
    int retsize;
    int status = 0;

    pthread_mutex_lock(&lock);

    if (!effect_exists(context)) {
        status = -ENOSYS;
        goto exit;
    }

    if (context == NULL || context->state == EFFECT_STATE_UNINITIALIZED) {
        status = -ENOSYS;
        goto exit;
    }

    switch (cmdCode) {
    case EFFECT_CMD_INIT:
        if (pReplyData == NULL || *replySize != sizeof(int)) {
            status = -EINVAL;
            goto exit;
        }
        if (context->ops.init)
            *(int *) pReplyData = context->ops.init(context);
        else
            *(int *) pReplyData = 0;
        break;
    case EFFECT_CMD_SET_CONFIG:
        if (pCmdData == NULL || cmdSize != sizeof(effect_config_t)
                || pReplyData == NULL || *replySize != sizeof(int)) {
            status = -EINVAL;
            goto exit;
        }
        *(int *) pReplyData = set_config(context, (effect_config_t *) pCmdData);
        break;
    case EFFECT_CMD_GET_CONFIG:
        if (pReplyData == NULL ||
            *replySize != sizeof(effect_config_t)) {
            status = -EINVAL;
            goto exit;
        }
        if (!context->offload_enabled) {
            status = -EINVAL;
            goto exit;
        }

        get_config(context, (effect_config_t *)pReplyData);
        break;
    case EFFECT_CMD_RESET:
        if (context->ops.reset)
            context->ops.reset(context);
        break;
    case EFFECT_CMD_ENABLE:
        if (pReplyData == NULL || *replySize != sizeof(int)) {
            status = -EINVAL;
            goto exit;
        }
        if (context->state != EFFECT_STATE_INITIALIZED) {
            status = -ENOSYS;
            goto exit;
        }
        context->state = EFFECT_STATE_ACTIVE;
        if (context->ops.enable)
            context->ops.enable(context);
        ALOGV("%s EFFECT_CMD_ENABLE", __func__);
        *(int *)pReplyData = 0;
        break;
    case EFFECT_CMD_DISABLE:
        if (pReplyData == NULL || *replySize != sizeof(int)) {
            status = -EINVAL;
            goto exit;
        }
        if (context->state != EFFECT_STATE_ACTIVE) {
            status = -ENOSYS;
            goto exit;
        }
        context->state = EFFECT_STATE_INITIALIZED;
        if (context->ops.disable)
            context->ops.disable(context);
        ALOGV("%s EFFECT_CMD_DISABLE", __func__);
        *(int *)pReplyData = 0;
        break;
    case EFFECT_CMD_GET_PARAM: {
        if (pCmdData == NULL ||
            cmdSize < (int)(sizeof(effect_param_t) + sizeof(uint32_t)) ||
            pReplyData == NULL ||
            *replySize < (int)(sizeof(effect_param_t) + sizeof(uint32_t) + sizeof(uint16_t)) ||
            // constrain memcpy below
            ((effect_param_t *)pCmdData)->psize > *replySize - sizeof(effect_param_t)) {
            status = -EINVAL;
            ALOGV("EFFECT_CMD_GET_PARAM invalid command cmdSize %d *replySize %d",
                  cmdSize, *replySize);
            goto exit;
        }
        if (!context->offload_enabled) {
            status = -EINVAL;
            goto exit;
        }
        effect_param_t *q = (effect_param_t *)pCmdData;
        memcpy(pReplyData, pCmdData, sizeof(effect_param_t) + q->psize);
        effect_param_t *p = (effect_param_t *)pReplyData;
        if (context->ops.get_parameter)
            context->ops.get_parameter(context, p, replySize);
        } break;
    case EFFECT_CMD_SET_PARAM: {
        if (pCmdData == NULL ||
            cmdSize < (int)(sizeof(effect_param_t) + sizeof(uint32_t) +
                            sizeof(uint16_t)) ||
            pReplyData == NULL || *replySize != sizeof(int32_t)) {
            status = -EINVAL;
            ALOGV("EFFECT_CMD_SET_PARAM invalid command cmdSize %d *replySize %d",
                  cmdSize, *replySize);
            goto exit;
        }
        *(int32_t *)pReplyData = 0;
        effect_param_t *p = (effect_param_t *)pCmdData;
        if (context->ops.set_parameter)
            *(int32_t *)pReplyData = context->ops.set_parameter(context, p,
                                                                *replySize);

        } break;
    case EFFECT_CMD_SET_DEVICE: {
        uint32_t device;
        ALOGV("\t EFFECT_CMD_SET_DEVICE start");
        if (pCmdData == NULL || cmdSize < sizeof(uint32_t)) {
            status = -EINVAL;
            ALOGV("EFFECT_CMD_SET_DEVICE invalid command cmdSize %d", cmdSize);
            goto exit;
        }
        device = *(uint32_t *)pCmdData;
        if (context->ops.set_device)
            context->ops.set_device(context, device);
        } break;
    case EFFECT_CMD_SET_VOLUME:
    case EFFECT_CMD_SET_AUDIO_MODE:
        break;

    case EFFECT_CMD_OFFLOAD: {
        output_context_t *out_ctxt;

        if (cmdSize != sizeof(effect_offload_param_t) || pCmdData == NULL
                || pReplyData == NULL || *replySize != sizeof(int)) {
            ALOGV("%s EFFECT_CMD_OFFLOAD bad format", __func__);
            status = -EINVAL;
            break;
        }

        effect_offload_param_t* offload_param = (effect_offload_param_t*)pCmdData;

        ALOGV("%s EFFECT_CMD_OFFLOAD offload %d output %d", __func__,
              offload_param->isOffload, offload_param->ioHandle);

        *(int *)pReplyData = 0;

        context->offload_enabled = offload_param->isOffload;
        if (context->out_handle == offload_param->ioHandle)
            break;

        out_ctxt = get_output(context->out_handle);
        if (out_ctxt != NULL)
            remove_effect_from_output(out_ctxt, context);

        context->out_handle = offload_param->ioHandle;
        out_ctxt = get_output(context->out_handle);
        if (out_ctxt != NULL)
            add_effect_to_output(out_ctxt, context);

        } break;


    default:
        if (cmdCode >= EFFECT_CMD_FIRST_PROPRIETARY && context->ops.command)
            status = context->ops.command(context, cmdCode, cmdSize,
                                          pCmdData, replySize, pReplyData);
        else {
            ALOGW("%s invalid command %d", __func__, cmdCode);
            status = -EINVAL;
        }
        break;
    }

exit:
    pthread_mutex_unlock(&lock);

    return status;
}

/* Effect Control Interface Implementation: get_descriptor */
int effect_get_descriptor(effect_handle_t   self,
                          effect_descriptor_t *descriptor)
{
    effect_context_t *context = (effect_context_t *)self;

    if (!effect_exists(context) || (descriptor == NULL))
        return -EINVAL;

    *descriptor = *context->desc;

    return 0;
}

bool effect_is_active(effect_context_t * ctxt) {
    return ctxt->state == EFFECT_STATE_ACTIVE;
}

/* effect_handle_t interface implementation for offload effects */
const struct effect_interface_s effect_interface = {
    effect_process,
    effect_command,
    effect_get_descriptor,
    NULL,
};

__attribute__ ((visibility ("default")))
audio_effect_library_t AUDIO_EFFECT_LIBRARY_INFO_SYM = {
    tag : AUDIO_EFFECT_LIBRARY_TAG,
    version : EFFECT_LIBRARY_API_VERSION,
    name : "Offload Effects Bundle Library",
    implementor : "The Android Open Source Project",
    create_effect : effect_lib_create,
    release_effect : effect_lib_release,
    get_descriptor : effect_lib_get_descriptor,
};
