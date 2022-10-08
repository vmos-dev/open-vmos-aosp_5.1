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

#define LOG_TAG "legacy_audio_policy_hal"
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

struct legacy_ap_module {
    struct audio_policy_module module;
};

struct legacy_ap_device {
    struct audio_policy_device device;
};

struct legacy_audio_policy {
    struct audio_policy policy;

    void *service;
    struct audio_policy_service_ops *aps_ops;
    AudioPolicyCompatClient *service_client;
    AudioPolicyInterface *apm;
};

static inline struct legacy_audio_policy * to_lap(struct audio_policy *pol)
{
    return reinterpret_cast<struct legacy_audio_policy *>(pol);
}

static inline const struct legacy_audio_policy * to_clap(const struct audio_policy *pol)
{
    return reinterpret_cast<const struct legacy_audio_policy *>(pol);
}


static int ap_set_device_connection_state(struct audio_policy *pol,
                                          audio_devices_t device,
                                          audio_policy_dev_state_t state,
                                          const char *device_address)
{
    struct legacy_audio_policy *lap = to_lap(pol);
    return lap->apm->setDeviceConnectionState(
                    (AudioSystem::audio_devices)device,
                    (AudioSystem::device_connection_state)state,
                    device_address);
}

static audio_policy_dev_state_t ap_get_device_connection_state(
                                            const struct audio_policy *pol,
                                            audio_devices_t device,
                                            const char *device_address)
{
    const struct legacy_audio_policy *lap = to_clap(pol);
    return (audio_policy_dev_state_t)lap->apm->getDeviceConnectionState(
                    (AudioSystem::audio_devices)device,
                    device_address);
}

static void ap_set_phone_state(struct audio_policy *pol, audio_mode_t state)
{
    struct legacy_audio_policy *lap = to_lap(pol);
    // as this is the legacy API, don't change it to use audio_mode_t instead of int
    lap->apm->setPhoneState((int) state);
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
    struct legacy_audio_policy *lap = to_lap(pol);
    lap->apm->setForceUse((AudioSystem::force_use)usage,
                          (AudioSystem::forced_config)config);
}

    /* retreive current device category forced for a given usage */
static audio_policy_forced_cfg_t ap_get_force_use(
                                               const struct audio_policy *pol,
                                               audio_policy_force_use_t usage)
{
    const struct legacy_audio_policy *lap = to_clap(pol);
    return (audio_policy_forced_cfg_t)lap->apm->getForceUse(
                          (AudioSystem::force_use)usage);
}

/* if can_mute is true, then audio streams that are marked ENFORCED_AUDIBLE
 * can still be muted. */
static void ap_set_can_mute_enforced_audible(struct audio_policy *pol,
                                             bool can_mute)
{
    struct legacy_audio_policy *lap = to_lap(pol);
    lap->apm->setSystemProperty("ro.camera.sound.forced", can_mute ? "0" : "1");
}

static int ap_init_check(const struct audio_policy *pol)
{
    const struct legacy_audio_policy *lap = to_clap(pol);
    return lap->apm->initCheck();
}

static audio_io_handle_t ap_get_output(struct audio_policy *pol,
                                       audio_stream_type_t stream,
                                       uint32_t sampling_rate,
                                       audio_format_t format,
                                       audio_channel_mask_t channelMask,
                                       audio_output_flags_t flags,
                                       const audio_offload_info_t *offloadInfo)
{
    struct legacy_audio_policy *lap = to_lap(pol);

    ALOGV("%s: tid %d", __func__, gettid());
    return lap->apm->getOutput((AudioSystem::stream_type)stream,
                               sampling_rate, format, channelMask,
                               (AudioSystem::output_flags)flags,
                               offloadInfo);
}

static int ap_start_output(struct audio_policy *pol, audio_io_handle_t output,
                           audio_stream_type_t stream, int session)
{
    struct legacy_audio_policy *lap = to_lap(pol);
    return lap->apm->startOutput(output, (AudioSystem::stream_type)stream,
                                 session);
}

static int ap_stop_output(struct audio_policy *pol, audio_io_handle_t output,
                          audio_stream_type_t stream, int session)
{
    struct legacy_audio_policy *lap = to_lap(pol);
    return lap->apm->stopOutput(output, (AudioSystem::stream_type)stream,
                                session);
}

static void ap_release_output(struct audio_policy *pol,
                              audio_io_handle_t output)
{
    struct legacy_audio_policy *lap = to_lap(pol);
    lap->apm->releaseOutput(output);
}

static audio_io_handle_t ap_get_input(struct audio_policy *pol, audio_source_t inputSource,
                                      uint32_t sampling_rate,
                                      audio_format_t format,
                                      audio_channel_mask_t channelMask,
                                      audio_in_acoustics_t acoustics)
{
    struct legacy_audio_policy *lap = to_lap(pol);
    return lap->apm->getInput((int) inputSource, sampling_rate, format, channelMask,
                              (AudioSystem::audio_in_acoustics)acoustics);
}

static int ap_start_input(struct audio_policy *pol, audio_io_handle_t input)
{
    struct legacy_audio_policy *lap = to_lap(pol);
    return lap->apm->startInput(input);
}

static int ap_stop_input(struct audio_policy *pol, audio_io_handle_t input)
{
    struct legacy_audio_policy *lap = to_lap(pol);
    return lap->apm->stopInput(input);
}

static void ap_release_input(struct audio_policy *pol, audio_io_handle_t input)
{
    struct legacy_audio_policy *lap = to_lap(pol);
    lap->apm->releaseInput(input);
}

static void ap_init_stream_volume(struct audio_policy *pol,
                                  audio_stream_type_t stream, int index_min,
                                  int index_max)
{
    struct legacy_audio_policy *lap = to_lap(pol);
    lap->apm->initStreamVolume((AudioSystem::stream_type)stream, index_min,
                               index_max);
}

static int ap_set_stream_volume_index(struct audio_policy *pol,
                                      audio_stream_type_t stream,
                                      int index)
{
    struct legacy_audio_policy *lap = to_lap(pol);
    return lap->apm->setStreamVolumeIndex((AudioSystem::stream_type)stream,
                                          index,
                                          AUDIO_DEVICE_OUT_DEFAULT);
}

static int ap_get_stream_volume_index(const struct audio_policy *pol,
                                      audio_stream_type_t stream,
                                      int *index)
{
    const struct legacy_audio_policy *lap = to_clap(pol);
    return lap->apm->getStreamVolumeIndex((AudioSystem::stream_type)stream,
                                          index,
                                          AUDIO_DEVICE_OUT_DEFAULT);
}

static int ap_set_stream_volume_index_for_device(struct audio_policy *pol,
                                      audio_stream_type_t stream,
                                      int index,
                                      audio_devices_t device)
{
    struct legacy_audio_policy *lap = to_lap(pol);
    return lap->apm->setStreamVolumeIndex((AudioSystem::stream_type)stream,
                                          index,
                                          device);
}

static int ap_get_stream_volume_index_for_device(const struct audio_policy *pol,
                                      audio_stream_type_t stream,
                                      int *index,
                                      audio_devices_t device)
{
    const struct legacy_audio_policy *lap = to_clap(pol);
    return lap->apm->getStreamVolumeIndex((AudioSystem::stream_type)stream,
                                          index,
                                          device);
}

static uint32_t ap_get_strategy_for_stream(const struct audio_policy *pol,
                                           audio_stream_type_t stream)
{
    const struct legacy_audio_policy *lap = to_clap(pol);
    return lap->apm->getStrategyForStream((AudioSystem::stream_type)stream);
}

static audio_devices_t ap_get_devices_for_stream(const struct audio_policy *pol,
                                       audio_stream_type_t stream)
{
    const struct legacy_audio_policy *lap = to_clap(pol);
    return lap->apm->getDevicesForStream((AudioSystem::stream_type)stream);
}

static audio_io_handle_t ap_get_output_for_effect(struct audio_policy *pol,
                                            const struct effect_descriptor_s *desc)
{
    struct legacy_audio_policy *lap = to_lap(pol);
    return lap->apm->getOutputForEffect(desc);
}

static int ap_register_effect(struct audio_policy *pol,
                              const struct effect_descriptor_s *desc,
                              audio_io_handle_t io,
                              uint32_t strategy,
                              int session,
                              int id)
{
    struct legacy_audio_policy *lap = to_lap(pol);
    return lap->apm->registerEffect(desc, io, strategy, session, id);
}

static int ap_unregister_effect(struct audio_policy *pol, int id)
{
    struct legacy_audio_policy *lap = to_lap(pol);
    return lap->apm->unregisterEffect(id);
}

static int ap_set_effect_enabled(struct audio_policy *pol, int id, bool enabled)
{
    struct legacy_audio_policy *lap = to_lap(pol);
    return lap->apm->setEffectEnabled(id, enabled);
}

static bool ap_is_stream_active(const struct audio_policy *pol, audio_stream_type_t stream,
                                uint32_t in_past_ms)
{
    const struct legacy_audio_policy *lap = to_clap(pol);
    return lap->apm->isStreamActive((int) stream, in_past_ms);
}

static bool ap_is_stream_active_remotely(const struct audio_policy *pol, audio_stream_type_t stream,
                                uint32_t in_past_ms)
{
    const struct legacy_audio_policy *lap = to_clap(pol);
    return lap->apm->isStreamActiveRemotely((int) stream, in_past_ms);
}

static bool ap_is_source_active(const struct audio_policy *pol, audio_source_t source)
{
    const struct legacy_audio_policy *lap = to_clap(pol);
    return lap->apm->isSourceActive(source);
}

static int ap_dump(const struct audio_policy *pol, int fd)
{
    const struct legacy_audio_policy *lap = to_clap(pol);
    return lap->apm->dump(fd);
}

static bool ap_is_offload_supported(const struct audio_policy *pol,
                                    const audio_offload_info_t *info)
{
    const struct legacy_audio_policy *lap = to_clap(pol);
    return lap->apm->isOffloadSupported(*info);
}

static int create_legacy_ap(const struct audio_policy_device *device,
                            struct audio_policy_service_ops *aps_ops,
                            void *service,
                            struct audio_policy **ap)
{
    struct legacy_audio_policy *lap;
    int ret;

    if (!service || !aps_ops)
        return -EINVAL;

    lap = (struct legacy_audio_policy *)calloc(1, sizeof(*lap));
    if (!lap)
        return -ENOMEM;

    lap->policy.set_device_connection_state = ap_set_device_connection_state;
    lap->policy.get_device_connection_state = ap_get_device_connection_state;
    lap->policy.set_phone_state = ap_set_phone_state;
    lap->policy.set_ringer_mode = ap_set_ringer_mode;
    lap->policy.set_force_use = ap_set_force_use;
    lap->policy.get_force_use = ap_get_force_use;
    lap->policy.set_can_mute_enforced_audible =
        ap_set_can_mute_enforced_audible;
    lap->policy.init_check = ap_init_check;
    lap->policy.get_output = ap_get_output;
    lap->policy.start_output = ap_start_output;
    lap->policy.stop_output = ap_stop_output;
    lap->policy.release_output = ap_release_output;
    lap->policy.get_input = ap_get_input;
    lap->policy.start_input = ap_start_input;
    lap->policy.stop_input = ap_stop_input;
    lap->policy.release_input = ap_release_input;
    lap->policy.init_stream_volume = ap_init_stream_volume;
    lap->policy.set_stream_volume_index = ap_set_stream_volume_index;
    lap->policy.get_stream_volume_index = ap_get_stream_volume_index;
    lap->policy.set_stream_volume_index_for_device = ap_set_stream_volume_index_for_device;
    lap->policy.get_stream_volume_index_for_device = ap_get_stream_volume_index_for_device;
    lap->policy.get_strategy_for_stream = ap_get_strategy_for_stream;
    lap->policy.get_devices_for_stream = ap_get_devices_for_stream;
    lap->policy.get_output_for_effect = ap_get_output_for_effect;
    lap->policy.register_effect = ap_register_effect;
    lap->policy.unregister_effect = ap_unregister_effect;
    lap->policy.set_effect_enabled = ap_set_effect_enabled;
    lap->policy.is_stream_active = ap_is_stream_active;
    lap->policy.is_stream_active_remotely = ap_is_stream_active_remotely;
    lap->policy.is_source_active = ap_is_source_active;
    lap->policy.dump = ap_dump;
    lap->policy.is_offload_supported = ap_is_offload_supported;

    lap->service = service;
    lap->aps_ops = aps_ops;
    lap->service_client =
        new AudioPolicyCompatClient(aps_ops, service);
    if (!lap->service_client) {
        ret = -ENOMEM;
        goto err_new_compat_client;
    }

    lap->apm = createAudioPolicyManager(lap->service_client);
    if (!lap->apm) {
        ret = -ENOMEM;
        goto err_create_apm;
    }

    *ap = &lap->policy;
    return 0;

err_create_apm:
    delete lap->service_client;
err_new_compat_client:
    free(lap);
    *ap = NULL;
    return ret;
}

static int destroy_legacy_ap(const struct audio_policy_device *ap_dev,
                             struct audio_policy *ap)
{
    struct legacy_audio_policy *lap = to_lap(ap);

    if (!lap)
        return 0;

    if (lap->apm)
        destroyAudioPolicyManager(lap->apm);
    if (lap->service_client)
        delete lap->service_client;
    free(lap);
    return 0;
}

static int legacy_ap_dev_close(hw_device_t* device)
{
    if (device)
        free(device);
    return 0;
}

static int legacy_ap_dev_open(const hw_module_t* module, const char* name,
                                    hw_device_t** device)
{
    struct legacy_ap_device *dev;

    if (strcmp(name, AUDIO_POLICY_INTERFACE) != 0)
        return -EINVAL;

    dev = (struct legacy_ap_device *)calloc(1, sizeof(*dev));
    if (!dev)
        return -ENOMEM;

    dev->device.common.tag = HARDWARE_DEVICE_TAG;
    dev->device.common.version = 0;
    dev->device.common.module = const_cast<hw_module_t*>(module);
    dev->device.common.close = legacy_ap_dev_close;
    dev->device.create_audio_policy = create_legacy_ap;
    dev->device.destroy_audio_policy = destroy_legacy_ap;

    *device = &dev->device.common;

    return 0;
}

static struct hw_module_methods_t legacy_ap_module_methods = {
        .open = legacy_ap_dev_open
};

struct legacy_ap_module HAL_MODULE_INFO_SYM = {
    .module = {
        .common = {
            .tag = HARDWARE_MODULE_TAG,
            .version_major = 1,
            .version_minor = 0,
            .id = AUDIO_POLICY_HARDWARE_MODULE_ID,
            .name = "LEGACY Audio Policy HAL",
            .author = "The Android Open Source Project",
            .methods = &legacy_ap_module_methods,
            .dso = NULL,
            .reserved = {0},
        },
    },
};

}; // extern "C"

}; // namespace android_audio_legacy
