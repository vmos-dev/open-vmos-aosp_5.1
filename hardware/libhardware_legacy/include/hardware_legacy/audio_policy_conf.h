/*
 * Copyright (C) 2012 The Android Open Source Project
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


#ifndef ANDROID_AUDIO_POLICY_CONF_H
#define ANDROID_AUDIO_POLICY_CONF_H


/////////////////////////////////////////////////
//      Definitions for audio policy configuration file (audio_policy.conf)
/////////////////////////////////////////////////

#define AUDIO_HARDWARE_MODULE_ID_MAX_LEN 32

#define AUDIO_POLICY_CONFIG_FILE "/system/etc/audio_policy.conf"
#define AUDIO_POLICY_VENDOR_CONFIG_FILE "/vendor/etc/audio_policy.conf"

// global configuration
#define GLOBAL_CONFIG_TAG "global_configuration"

#define ATTACHED_OUTPUT_DEVICES_TAG "attached_output_devices"
#define DEFAULT_OUTPUT_DEVICE_TAG "default_output_device"
#define ATTACHED_INPUT_DEVICES_TAG "attached_input_devices"
#define SPEAKER_DRC_ENABLED_TAG "speaker_drc_enabled"

// hw modules descriptions
#define AUDIO_HW_MODULE_TAG "audio_hw_modules"

#define OUTPUTS_TAG "outputs"
#define INPUTS_TAG "inputs"

#define SAMPLING_RATES_TAG "sampling_rates"
#define FORMATS_TAG "formats"
#define CHANNELS_TAG "channel_masks"
#define DEVICES_TAG "devices"
#define FLAGS_TAG "flags"

#define DYNAMIC_VALUE_TAG "dynamic" // special value for "channel_masks", "sampling_rates" and
                                    // "formats" in outputs descriptors indicating that supported
                                    // values should be queried after opening the output.

#endif  // ANDROID_AUDIO_POLICY_CONF_H
