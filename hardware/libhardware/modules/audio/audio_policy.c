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

#define LOG_TAG "audio_policy_default"
//#define LOG_NDEBUG 0

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <hardware/hardware.h>
#include <system/audio.h>
#include <system/audio_policy.h>
#include <hardware/audio_policy.h>

struct default_ap_module {
    struct audio_policy_module module;
};

struct default_ap_device {
    struct audio_policy_device device;
};

struct default_audio_policy {
    struct audio_policy policy;

    struct audio_policy_service_ops *aps_ops;
    void *service;
};

static int ap_set_device_connection_state(struct audio_policy *pol,
                                          audio_devices_t device,
                                          audio_policy_dev_state_t state,
                                          const char *device_address)
{
    return -ENOSYS;
}

static audio_policy_dev_state_t ap_get_device_connection_state(
                                            const struct audio_policy *pol,
                                            audio_devices_t device,
                                            const char *device_address)
{
    return AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE;
}

static void ap_set_phone_state(struct audio_policy *pol, audio_mode_t state)
{
}

// deprecated, never called
static void ap_set_ringer_mode(struct audio_policy *pol, uint32_t mode,
                               uint32_t mask)
{
}

static void ap_set_force_use(struct audio_policy *pol,
                          audio_policy_force_use_t usage,
                          audio_policy_forced_cfg_t config)
{
}

    /* retreive current device category forced for a given usage */
static audio_policy_forced_cfg_t ap_get_force_use(
                                               const struct audio_policy *pol,
                                               audio_policy_force_use_t usage)
{
    return AUDIO_POLICY_FORCE_NONE;
}

/* if can_mute is true, then audio streams that are marked ENFORCED_AUDIBLE
 * can still be muted. */
static void ap_set_can_mute_enforced_audible(struct audio_policy *pol,
                                             bool can_mute)
{
}

static int ap_init_check(const struct audio_policy *pol)
{
    return 0;
}

static audio_io_handle_t ap_get_output(struct audio_policy *pol,
                                       audio_stream_type_t stream,
                                       uint32_t sampling_rate,
                                       audio_format_t format,
                                       audio_channel_mask_t channelMask,
                                       audio_output_flags_t flags,
                                       const audio_offload_info_t *info)
{
    return 0;
}

static int ap_start_output(struct audio_policy *pol, audio_io_handle_t output,
                           audio_stream_type_t stream, int session)
{
    return -ENOSYS;
}

static int ap_stop_output(struct audio_policy *pol, audio_io_handle_t output,
                          audio_stream_type_t stream, int session)
{
    return -ENOSYS;
}

static void ap_release_output(struct audio_policy *pol,
                              audio_io_handle_t output)
{
}

static audio_io_handle_t ap_get_input(struct audio_policy *pol, audio_source_t inputSource,
                                      uint32_t sampling_rate,
                                      audio_format_t format,
                                      audio_channel_mask_t channelMask,
                                      audio_in_acoustics_t acoustics)
{
    return 0;
}

static int ap_start_input(struct audio_policy *pol, audio_io_handle_t input)
{
    return -ENOSYS;
}

static int ap_stop_input(struct audio_policy *pol, audio_io_handle_t input)
{
    return -ENOSYS;
}

static void ap_release_input(struct audio_policy *pol, audio_io_handle_t input)
{
}

static void ap_init_stream_volume(struct audio_policy *pol,
                                  audio_stream_type_t stream, int index_min,
                                  int index_max)
{
}

static int ap_set_stream_volume_index(struct audio_policy *pol,
                                      audio_stream_type_t stream,
                                      int index)
{
    return -ENOSYS;
}

static int ap_get_stream_volume_index(const struct audio_policy *pol,
                                      audio_stream_type_t stream,
                                      int *index)
{
    return -ENOSYS;
}

static int ap_set_stream_volume_index_for_device(struct audio_policy *pol,
                                      audio_stream_type_t stream,
                                      int index,
                                      audio_devices_t device)
{
    return -ENOSYS;
}

static int ap_get_stream_volume_index_for_device(const struct audio_policy *pol,
                                      audio_stream_type_t stream,
                                      int *index,
                                      audio_devices_t device)
{
    return -ENOSYS;
}

static uint32_t ap_get_strategy_for_stream(const struct audio_policy *pol,
                                           audio_stream_type_t stream)
{
    return 0;
}

static audio_devices_t ap_get_devices_for_stream(const struct audio_policy *pol,
                                          audio_stream_type_t stream)
{
    return 0;
}

static audio_io_handle_t ap_get_output_for_effect(struct audio_policy *pol,
                                            const struct effect_descriptor_s *desc)
{
    return 0;
}

static int ap_register_effect(struct audio_policy *pol,
                              const struct effect_descriptor_s *desc,
                              audio_io_handle_t output,
                              uint32_t strategy,
                              int session,
                              int id)
{
    return -ENOSYS;
}

static int ap_unregister_effect(struct audio_policy *pol, int id)
{
    return -ENOSYS;
}

static int ap_set_effect_enabled(struct audio_policy *pol, int id, bool enabled)
{
    return -ENOSYS;
}

static bool ap_is_stream_active(const struct audio_policy *pol, audio_stream_type_t stream,
                                uint32_t in_past_ms)
{
    return false;
}

static int ap_dump(const struct audio_policy *pol, int fd)
{
    return -ENOSYS;
}

static bool ap_is_offload_supported(const struct audio_policy *pol,
                                   const audio_offload_info_t *info)
{
    return false;
}

static int create_default_ap(const struct audio_policy_device *device,
                             struct audio_policy_service_ops *aps_ops,
                             void *service,
                             struct audio_policy **ap)
{
    struct default_ap_device *dev;
    struct default_audio_policy *dap;
    int ret;

    *ap = NULL;

    if (!service || !aps_ops)
        return -EINVAL;

    dap = (struct default_audio_policy *)calloc(1, sizeof(*dap));
    if (!dap)
        return -ENOMEM;

    dap->policy.set_device_connection_state = ap_set_device_connection_state;
    dap->policy.get_device_connection_state = ap_get_device_connection_state;
    dap->policy.set_phone_state = ap_set_phone_state;
    dap->policy.set_ringer_mode = ap_set_ringer_mode;
    dap->policy.set_force_use = ap_set_force_use;
    dap->policy.get_force_use = ap_get_force_use;
    dap->policy.set_can_mute_enforced_audible =
        ap_set_can_mute_enforced_audible;
    dap->policy.init_check = ap_init_check;
    dap->policy.get_output = ap_get_output;
    dap->policy.start_output = ap_start_output;
    dap->policy.stop_output = ap_stop_output;
    dap->policy.release_output = ap_release_output;
    dap->policy.get_input = ap_get_input;
    dap->policy.start_input = ap_start_input;
    dap->policy.stop_input = ap_stop_input;
    dap->policy.release_input = ap_release_input;
    dap->policy.init_stream_volume = ap_init_stream_volume;
    dap->policy.set_stream_volume_index = ap_set_stream_volume_index;
    dap->policy.get_stream_volume_index = ap_get_stream_volume_index;
    dap->policy.set_stream_volume_index_for_device = ap_set_stream_volume_index_for_device;
    dap->policy.get_stream_volume_index_for_device = ap_get_stream_volume_index_for_device;
    dap->policy.get_strategy_for_stream = ap_get_strategy_for_stream;
    dap->policy.get_devices_for_stream = ap_get_devices_for_stream;
    dap->policy.get_output_for_effect = ap_get_output_for_effect;
    dap->policy.register_effect = ap_register_effect;
    dap->policy.unregister_effect = ap_unregister_effect;
    dap->policy.set_effect_enabled = ap_set_effect_enabled;
    dap->policy.is_stream_active = ap_is_stream_active;
    dap->policy.dump = ap_dump;

    dap->policy.is_offload_supported = ap_is_offload_supported;

    dap->service = service;
    dap->aps_ops = aps_ops;

    *ap = &dap->policy;
    return 0;
}

static int destroy_default_ap(const struct audio_policy_device *ap_dev,
                              struct audio_policy *ap)
{
    free(ap);
    return 0;
}

static int default_ap_dev_close(hw_device_t* device)
{
    free(device);
    return 0;
}

static int default_ap_dev_open(const hw_module_t* module, const char* name,
                               hw_device_t** device)
{
    struct default_ap_device *dev;

    *device = NULL;

    if (strcmp(name, AUDIO_POLICY_INTERFACE) != 0)
        return -EINVAL;

    dev = (struct default_ap_device *)calloc(1, sizeof(*dev));
    if (!dev)
        return -ENOMEM;

    dev->device.common.tag = HARDWARE_DEVICE_TAG;
    dev->device.common.version = 0;
    dev->device.common.module = (hw_module_t *)module;
    dev->device.common.close = default_ap_dev_close;
    dev->device.create_audio_policy = create_default_ap;
    dev->device.destroy_audio_policy = destroy_default_ap;

    *device = &dev->device.common;

    return 0;
}

static struct hw_module_methods_t default_ap_module_methods = {
    .open = default_ap_dev_open,
};

struct default_ap_module HAL_MODULE_INFO_SYM = {
    .module = {
        .common = {
            .tag            = HARDWARE_MODULE_TAG,
            .version_major  = 1,
            .version_minor  = 0,
            .id             = AUDIO_POLICY_HARDWARE_MODULE_ID,
            .name           = "Default audio policy HAL",
            .author         = "The Android Open Source Project",
            .methods        = &default_ap_module_methods,
        },
    },
};
