/*
 * Copyright (C) 2011 The Android Open Source Project
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

#define LOG_TAG "qcom_audio_policy_hal"
//#define LOG_NDEBUG 0

#include <stdint.h>

#include <hardware/hardware.h>
#include <system/audio.h>
#include <system/audio_policy.h>
#include <hardware/audio_policy.h>

#include <hardware_legacy/AudioPolicyInterface.h>
#include <hardware_legacy/AudioSystemLegacy.h>

#include "AudioPolicyCompatClient.h"

namespace android_audio_legacy {

extern "C" {

struct qcom_ap_module {
    struct audio_policy_module module;
};

struct qcom_ap_device {
    struct audio_policy_device device;
};

struct qcom_audio_policy {
    struct audio_policy policy;

    void *service;
    struct audio_policy_service_ops *aps_ops;
    AudioPolicyCompatClient *service_client;
    AudioPolicyInterface *apm;
};

static inline struct qcom_audio_policy * to_qap(struct audio_policy *pol)
{
    return reinterpret_cast<struct qcom_audio_policy *>(pol);
}

static inline const struct qcom_audio_policy * to_cqap(const struct audio_policy *pol)
{
    return reinterpret_cast<const struct qcom_audio_policy *>(pol);
}


static int ap_set_device_connection_state(struct audio_policy *pol,
                                          audio_devices_t device,
                                          audio_policy_dev_state_t state,
                                          const char *device_address)
{
    struct qcom_audio_policy *qap = to_qap(pol);
    return qap->apm->setDeviceConnectionState(
                    (AudioSystem::audio_devices)device,
                    (AudioSystem::device_connection_state)state,
                    device_address);
}

static audio_policy_dev_state_t ap_get_device_connection_state(
                                            const struct audio_policy *pol,
                                            audio_devices_t device,
                                            const char *device_address)
{
    const struct qcom_audio_policy *qap = to_cqap(pol);
    return (audio_policy_dev_state_t)qap->apm->getDeviceConnectionState(
                    (AudioSystem::audio_devices)device,
                    device_address);
}

static void ap_set_phone_state(struct audio_policy *pol, audio_mode_t state)
{
    struct qcom_audio_policy *qap = to_qap(pol);
    // as this is the legacy API, don't change it to use audio_mode_t instead of int
    qap->apm->setPhoneState((int) state);
}

    /* indicate a change in ringer mode */
static void ap_set_ringer_mode(struct audio_policy *pol, uint32_t mode,
                               uint32_t mask)
{
    // deprecated, never called
}

    /* force using a specific device category for the specified usage */
static void ap_set_force_use(struct audio_policy *pol,
                          audio_policy_force_use_t usage,
                          audio_policy_forced_cfg_t config)
{
    struct qcom_audio_policy *qap = to_qap(pol);
    qap->apm->setForceUse((AudioSystem::force_use)usage,
                          (AudioSystem::forced_config)config);
}

    /* retreive current device category forced for a given usage */
static audio_policy_forced_cfg_t ap_get_force_use(
                                               const struct audio_policy *pol,
                                               audio_policy_force_use_t usage)
{
    const struct qcom_audio_policy *qap = to_cqap(pol);
    return (audio_policy_forced_cfg_t)qap->apm->getForceUse(
                          (AudioSystem::force_use)usage);
}

/* if can_mute is true, then audio streams that are marked ENFORCED_AUDIBLE
 * can still be muted. */
static void ap_set_can_mute_enforced_audible(struct audio_policy *pol,
                                             bool can_mute)
{
    struct qcom_audio_policy *qap = to_qap(pol);
    qap->apm->setSystemProperty("ro.camera.sound.forced", can_mute ? "0" : "1");
}

static int ap_init_check(const struct audio_policy *pol)
{
    const struct qcom_audio_policy *qap = to_cqap(pol);
    return qap->apm->initCheck();
}

static audio_io_handle_t ap_get_output(struct audio_policy *pol,
                                       audio_stream_type_t stream,
                                       uint32_t sampling_rate,
                                       audio_format_t format,
                                       audio_channel_mask_t channelMask,
                                       audio_output_flags_t flags)
{
    struct qcom_audio_policy *qap = to_qap(pol);

    ALOGV("%s: tid %d", __func__, gettid());
    return qap->apm->getOutput((AudioSystem::stream_type)stream,
                               sampling_rate, (int) format, channelMask,
                               (AudioSystem::output_flags)flags);
}

static int ap_start_output(struct audio_policy *pol, audio_io_handle_t output,
                           audio_stream_type_t stream, int session)
{
    struct qcom_audio_policy *qap = to_qap(pol);
    return qap->apm->startOutput(output, (AudioSystem::stream_type)stream,
                                 session);
}

static int ap_stop_output(struct audio_policy *pol, audio_io_handle_t output,
                          audio_stream_type_t stream, int session)
{
    struct qcom_audio_policy *qap = to_qap(pol);
    return qap->apm->stopOutput(output, (AudioSystem::stream_type)stream,
                                session);
}

static void ap_release_output(struct audio_policy *pol,
                              audio_io_handle_t output)
{
    struct qcom_audio_policy *qap = to_qap(pol);
    qap->apm->releaseOutput(output);
}

static audio_io_handle_t ap_get_input(struct audio_policy *pol, audio_source_t inputSource,
                                      uint32_t sampling_rate,
                                      audio_format_t format,
                                      audio_channel_mask_t channelMask,
                                      audio_in_acoustics_t acoustics)
{
    struct qcom_audio_policy *qap = to_qap(pol);
    return qap->apm->getInput((int) inputSource, sampling_rate, (int) format, channelMask,
                              (AudioSystem::audio_in_acoustics)acoustics);
}

static int ap_start_input(struct audio_policy *pol, audio_io_handle_t input)
{
    struct qcom_audio_policy *qap = to_qap(pol);
    return qap->apm->startInput(input);
}

static int ap_stop_input(struct audio_policy *pol, audio_io_handle_t input)
{
    struct qcom_audio_policy *qap = to_qap(pol);
    return qap->apm->stopInput(input);
}

static void ap_release_input(struct audio_policy *pol, audio_io_handle_t input)
{
    struct qcom_audio_policy *qap = to_qap(pol);
    qap->apm->releaseInput(input);
}

static void ap_init_stream_volume(struct audio_policy *pol,
                                  audio_stream_type_t stream, int index_min,
                                  int index_max)
{
    struct qcom_audio_policy *qap = to_qap(pol);
    qap->apm->initStreamVolume((AudioSystem::stream_type)stream, index_min,
                               index_max);
}

static int ap_set_stream_volume_index(struct audio_policy *pol,
                                      audio_stream_type_t stream,
                                      int index)
{
    struct qcom_audio_policy *qap = to_qap(pol);
    return qap->apm->setStreamVolumeIndex((AudioSystem::stream_type)stream,
                                          index,
                                          AUDIO_DEVICE_OUT_DEFAULT);
}

static int ap_get_stream_volume_index(const struct audio_policy *pol,
                                      audio_stream_type_t stream,
                                      int *index)
{
    const struct qcom_audio_policy *qap = to_cqap(pol);
    return qap->apm->getStreamVolumeIndex((AudioSystem::stream_type)stream,
                                          index,
                                          AUDIO_DEVICE_OUT_DEFAULT);
}

static int ap_set_stream_volume_index_for_device(struct audio_policy *pol,
                                      audio_stream_type_t stream,
                                      int index,
                                      audio_devices_t device)
{
   const struct qcom_audio_policy *qap = to_cqap(pol);
   return qap->apm->setStreamVolumeIndex((AudioSystem::stream_type)stream,
                                          index,
                                          device);
}

static int ap_get_stream_volume_index_for_device(const struct audio_policy *pol,
                                      audio_stream_type_t stream,
                                      int *index,
                                      audio_devices_t device)
{
   const struct qcom_audio_policy *qap = to_cqap(pol);
   return qap->apm->getStreamVolumeIndex((AudioSystem::stream_type)stream,
                                          index,
                                          device);
}

static uint32_t ap_get_strategy_for_stream(const struct audio_policy *pol,
                                           audio_stream_type_t stream)
{
    const struct qcom_audio_policy *qap = to_cqap(pol);
    return qap->apm->getStrategyForStream((AudioSystem::stream_type)stream);
}

static audio_devices_t ap_get_devices_for_stream(const struct audio_policy *pol,
                                       audio_stream_type_t stream)
{
    const struct qcom_audio_policy *qap = to_cqap(pol);
    return qap->apm->getDevicesForStream((AudioSystem::stream_type)stream);
}

static audio_io_handle_t ap_get_output_for_effect(struct audio_policy *pol,
                                            const struct effect_descriptor_s *desc)
{
    struct qcom_audio_policy *qap = to_qap(pol);
    return qap->apm->getOutputForEffect(desc);
}

static int ap_register_effect(struct audio_policy *pol,
                              const struct effect_descriptor_s *desc,
                              audio_io_handle_t io,
                              uint32_t strategy,
                              int session,
                              int id)
{
    struct qcom_audio_policy *qap = to_qap(pol);
    return qap->apm->registerEffect(desc, io, strategy, session, id);
}

static int ap_unregister_effect(struct audio_policy *pol, int id)
{
    struct qcom_audio_policy *qap = to_qap(pol);
    return qap->apm->unregisterEffect(id);
}

static int ap_set_effect_enabled(struct audio_policy *pol, int id, bool enabled)
{
    struct qcom_audio_policy *qap = to_qap(pol);
    return qap->apm->setEffectEnabled(id, enabled);
}

static bool ap_is_stream_active(const struct audio_policy *pol, 
                                audio_stream_type_t stream,
                                uint32_t in_past_ms)
{
    const struct qcom_audio_policy *qap = to_cqap(pol);
    return qap->apm->isStreamActive((int) stream, in_past_ms);
}

static bool ap_is_stream_active_remotely(const struct audio_policy *pol,
        audio_stream_type_t stream, uint32_t in_past_ms)
{
    const struct qcom_audio_policy *qap = to_cqap(pol);
    return qap->apm->isStreamActiveRemotely((int) stream, in_past_ms);
}

static bool ap_is_source_active(const struct audio_policy *pol, audio_source_t source)
{
    const struct qcom_audio_policy *qap = to_cqap(pol);
    return qap->apm->isSourceActive(source);
}

static int ap_dump(const struct audio_policy *pol, int fd)
{
    const struct qcom_audio_policy *qap = to_cqap(pol);
    return qap->apm->dump(fd);
}

static int create_qcom_ap(const struct audio_policy_device *device,
                            struct audio_policy_service_ops *aps_ops,
                            void *service,
                            struct audio_policy **ap)
{
    struct qcom_audio_policy *qap;
    int ret;

    if (!service || !aps_ops)
        return -EINVAL;

    qap = (struct qcom_audio_policy *)calloc(1, sizeof(*qap));
    if (!qap)
        return -ENOMEM;

    qap->policy.set_device_connection_state = ap_set_device_connection_state;
    qap->policy.get_device_connection_state = ap_get_device_connection_state;
    qap->policy.set_phone_state = ap_set_phone_state;
    qap->policy.set_ringer_mode = ap_set_ringer_mode;
    qap->policy.set_force_use = ap_set_force_use;
    qap->policy.get_force_use = ap_get_force_use;
    qap->policy.set_can_mute_enforced_audible =
        ap_set_can_mute_enforced_audible;
    qap->policy.init_check = ap_init_check;
    qap->policy.get_output = ap_get_output;
    qap->policy.start_output = ap_start_output;
    qap->policy.stop_output = ap_stop_output;
    qap->policy.release_output = ap_release_output;
    qap->policy.get_input = ap_get_input;
    qap->policy.start_input = ap_start_input;
    qap->policy.stop_input = ap_stop_input;
    qap->policy.release_input = ap_release_input;
    qap->policy.init_stream_volume = ap_init_stream_volume;
    qap->policy.set_stream_volume_index = ap_set_stream_volume_index;
    qap->policy.get_stream_volume_index = ap_get_stream_volume_index;
    qap->policy.set_stream_volume_index_for_device = ap_set_stream_volume_index_for_device;
    qap->policy.get_stream_volume_index_for_device = ap_get_stream_volume_index_for_device;
    qap->policy.get_strategy_for_stream = ap_get_strategy_for_stream;
    qap->policy.get_devices_for_stream = ap_get_devices_for_stream;
    qap->policy.get_output_for_effect = ap_get_output_for_effect;
    qap->policy.register_effect = ap_register_effect;
    qap->policy.unregister_effect = ap_unregister_effect;
    qap->policy.set_effect_enabled = ap_set_effect_enabled;
    qap->policy.is_stream_active = ap_is_stream_active;
    qap->policy.is_stream_active_remotely = ap_is_stream_active_remotely;
    qap->policy.is_source_active = ap_is_source_active;
    qap->policy.dump = ap_dump;

    qap->service = service;
    qap->aps_ops = aps_ops;
    qap->service_client =
        new AudioPolicyCompatClient(aps_ops, service);
    if (!qap->service_client) {
        ret = -ENOMEM;
        goto err_new_compat_client;
    }

    qap->apm = createAudioPolicyManager(qap->service_client);
    if (!qap->apm) {
        ret = -ENOMEM;
        goto err_create_apm;
    }

    *ap = &qap->policy;
    return 0;

err_create_apm:
    delete qap->service_client;
err_new_compat_client:
    free(qap);
    *ap = NULL;
    return ret;
}

static int destroy_qcom_ap(const struct audio_policy_device *ap_dev,
                             struct audio_policy *ap)
{
    struct qcom_audio_policy *qap = to_qap(ap);

    if (!qap)
        return 0;

    if (qap->apm)
        destroyAudioPolicyManager(qap->apm);
    if (qap->service_client)
        delete qap->service_client;
    free(qap);
    return 0;
}

static int qcom_ap_dev_close(hw_device_t* device)
{
    if (device)
        free(device);
    return 0;
}

static int qcom_ap_dev_open(const hw_module_t* module, const char* name,
                                    hw_device_t** device)
{
    struct qcom_ap_device *dev;

    if (strcmp(name, AUDIO_POLICY_INTERFACE) != 0)
        return -EINVAL;

    dev = (struct qcom_ap_device *)calloc(1, sizeof(*dev));
    if (!dev)
        return -ENOMEM;

    dev->device.common.tag = HARDWARE_DEVICE_TAG;
    dev->device.common.version = 0;
    dev->device.common.module = const_cast<hw_module_t*>(module);
    dev->device.common.close = qcom_ap_dev_close;
    dev->device.create_audio_policy = create_qcom_ap;
    dev->device.destroy_audio_policy = destroy_qcom_ap;

    *device = &dev->device.common;

    return 0;
}

static struct hw_module_methods_t qcom_ap_module_methods = {
        open: qcom_ap_dev_open
};

struct qcom_ap_module HAL_MODULE_INFO_SYM = {
    module: {
        common: {
            tag: HARDWARE_MODULE_TAG,
            version_major: 1,
            version_minor: 0,
            id: AUDIO_POLICY_HARDWARE_MODULE_ID,
            name: "QCOM Audio Policy HAL",
            author: "Code Aurora Forum",
            methods: &qcom_ap_module_methods,
            dso : NULL,
            reserved : {0},
        },
    },
};

}; // extern "C"

}; // namespace android_audio_legacy
