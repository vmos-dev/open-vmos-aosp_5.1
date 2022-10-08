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

#ifndef ANDROID_AUDIOPOLICYCLIENTLEGACY_H
#define ANDROID_AUDIOPOLICYCLIENTLEGACY_H

#include <system/audio.h>
#include <system/audio_policy.h>
#include <hardware/audio_policy.h>

#include <hardware_legacy/AudioSystemLegacy.h>
#include <hardware_legacy/AudioPolicyInterface.h>

/************************************/
/* FOR BACKWARDS COMPATIBILITY ONLY */
/************************************/
namespace android_audio_legacy {

class AudioPolicyCompatClient : public AudioPolicyClientInterface {
public:
    AudioPolicyCompatClient(struct audio_policy_service_ops *serviceOps,
                            void *service) :
            mServiceOps(serviceOps) , mService(service) {}

    virtual audio_module_handle_t loadHwModule(const char *moduleName);

    virtual audio_io_handle_t openOutput(audio_module_handle_t module,
                                         audio_devices_t *pDevices,
                                         uint32_t *pSamplingRate,
                                         audio_format_t *pFormat,
                                         audio_channel_mask_t *pChannelMask,
                                         uint32_t *pLatencyMs,
                                         audio_output_flags_t flags,
                                         const audio_offload_info_t *offloadInfo);
    virtual audio_io_handle_t openDuplicateOutput(audio_io_handle_t output1,
                                                  audio_io_handle_t output2);
    virtual status_t closeOutput(audio_io_handle_t output);
    virtual status_t suspendOutput(audio_io_handle_t output);
    virtual status_t restoreOutput(audio_io_handle_t output);
    virtual audio_io_handle_t openInput(audio_module_handle_t module,
                                        audio_devices_t *pDevices,
                                        uint32_t *pSamplingRate,
                                        audio_format_t *pFormat,
                                        audio_channel_mask_t *pChannelMask);
    virtual status_t closeInput(audio_io_handle_t input);
    virtual status_t invalidateStream(AudioSystem::stream_type stream);
    virtual status_t moveEffects(int session,
                                 audio_io_handle_t srcOutput,
                                 audio_io_handle_t dstOutput);

    virtual String8 getParameters(audio_io_handle_t ioHandle, const String8& keys);
    virtual void setParameters(audio_io_handle_t ioHandle,
                               const String8& keyValuePairs,
                               int delayMs = 0);
    virtual status_t setStreamVolume(AudioSystem::stream_type stream,
                                     float volume,
                                     audio_io_handle_t output,
                                     int delayMs = 0);
    virtual status_t startTone(ToneGenerator::tone_type tone, AudioSystem::stream_type stream);
    virtual status_t stopTone();
    virtual status_t setVoiceVolume(float volume, int delayMs = 0);

private:
    struct audio_policy_service_ops* mServiceOps;
    void*                            mService;
};

}; // namespace android_audio_legacy

#endif // ANDROID_AUDIOPOLICYCLIENTLEGACY_H
