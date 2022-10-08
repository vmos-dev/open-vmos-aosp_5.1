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

#define LOG_TAG "AudioPolicyCompatClient"
//#define LOG_NDEBUG 0

#include <stdint.h>

#include <hardware/hardware.h>
#include <system/audio.h>
#include <system/audio_policy.h>
#include <hardware/audio_policy.h>

#include <hardware_legacy/AudioSystemLegacy.h>

#include "AudioPolicyCompatClient.h"

namespace android_audio_legacy {

audio_module_handle_t AudioPolicyCompatClient::loadHwModule(const char *moduleName)
{
    return mServiceOps->load_hw_module(mService, moduleName);
}

audio_io_handle_t AudioPolicyCompatClient::openOutput(audio_module_handle_t module,
                                                      audio_devices_t *pDevices,
                                                      uint32_t *pSamplingRate,
                                                      audio_format_t *pFormat,
                                                      audio_channel_mask_t *pChannelMask,
                                                      uint32_t *pLatencyMs,
                                                      audio_output_flags_t flags,
                                                      const audio_offload_info_t *offloadInfo)
{
    return mServiceOps->open_output_on_module(mService, module, pDevices, pSamplingRate,
                                              pFormat, pChannelMask, pLatencyMs,
                                              flags, offloadInfo);
}

audio_io_handle_t AudioPolicyCompatClient::openDuplicateOutput(audio_io_handle_t output1,
                                                          audio_io_handle_t output2)
{
    return mServiceOps->open_duplicate_output(mService, output1, output2);
}

status_t AudioPolicyCompatClient::closeOutput(audio_io_handle_t output)
{
    return mServiceOps->close_output(mService, output);
}

status_t AudioPolicyCompatClient::suspendOutput(audio_io_handle_t output)
{
    return mServiceOps->suspend_output(mService, output);
}

status_t AudioPolicyCompatClient::restoreOutput(audio_io_handle_t output)
{
    return mServiceOps->restore_output(mService, output);
}

audio_io_handle_t AudioPolicyCompatClient::openInput(audio_module_handle_t module,
                                                     audio_devices_t *pDevices,
                                                     uint32_t *pSamplingRate,
                                                     audio_format_t *pFormat,
                                                     audio_channel_mask_t *pChannelMask)
{
    return mServiceOps->open_input_on_module(mService, module, pDevices,
                                             pSamplingRate, pFormat, pChannelMask);
}

status_t AudioPolicyCompatClient::closeInput(audio_io_handle_t input)
{
    return mServiceOps->close_input(mService, input);
}

status_t AudioPolicyCompatClient::invalidateStream(AudioSystem::stream_type stream)
{
    return mServiceOps->invalidate_stream(mService, (audio_stream_type_t)stream);
}

status_t AudioPolicyCompatClient::moveEffects(int session, audio_io_handle_t srcOutput,
                                               audio_io_handle_t dstOutput)
{
    return mServiceOps->move_effects(mService, session, srcOutput, dstOutput);
}

String8 AudioPolicyCompatClient::getParameters(audio_io_handle_t ioHandle, const String8& keys)
{
    char *str;
    String8 out_str8;

    str = mServiceOps->get_parameters(mService, ioHandle, keys.string());
    out_str8 = String8(str);
    free(str);

    return out_str8;
}

void AudioPolicyCompatClient::setParameters(audio_io_handle_t ioHandle,
                                            const String8& keyValuePairs,
                                            int delayMs)
{
    mServiceOps->set_parameters(mService, ioHandle, keyValuePairs.string(),
                           delayMs);
}

status_t AudioPolicyCompatClient::setStreamVolume(
                                             AudioSystem::stream_type stream,
                                             float volume,
                                             audio_io_handle_t output,
                                             int delayMs)
{
    return mServiceOps->set_stream_volume(mService, (audio_stream_type_t)stream,
                                          volume, output, delayMs);
}

status_t AudioPolicyCompatClient::startTone(ToneGenerator::tone_type tone,
                                       AudioSystem::stream_type stream)
{
    return mServiceOps->start_tone(mService,
                                   AUDIO_POLICY_TONE_IN_CALL_NOTIFICATION,
                                   (audio_stream_type_t)stream);
}

status_t AudioPolicyCompatClient::stopTone()
{
    return mServiceOps->stop_tone(mService);
}

status_t AudioPolicyCompatClient::setVoiceVolume(float volume, int delayMs)
{
    return mServiceOps->set_voice_volume(mService, volume, delayMs);
}

}; // namespace android_audio_legacy
