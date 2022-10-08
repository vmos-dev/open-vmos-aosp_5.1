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

#define LOG_TAG "ATVAudioPolicyManager"
//#define LOG_NDEBUG 0
#include <media/AudioParameter.h>
#include <media/mediarecorder.h>
#include <utils/Log.h>
#include <utils/String16.h>
#include <utils/String8.h>
#include <utils/StrongPointer.h>

#include "AudioHardwareOutput.h"
#include "ATVAudioPolicyManager.h"

#ifdef REMOTE_CONTROL_INTERFACE
#include <IRemoteControlService.h>
#endif


namespace android {
extern AudioHardwareOutput gAudioHardwareOutput;

// ----------------------------------------------------------------------------
// Common audio policy manager code is implemented in AudioPolicyManager class
// ----------------------------------------------------------------------------

// ---  class factory


extern "C" AudioPolicyInterface* createAudioPolicyManager(
        AudioPolicyClientInterface *clientInterface)
{
    return new ATVAudioPolicyManager(clientInterface);
}

extern "C" void destroyAudioPolicyManager(AudioPolicyInterface *interface)
{
    delete interface;
}

ATVAudioPolicyManager::ATVAudioPolicyManager(
        AudioPolicyClientInterface *clientInterface)
    : AudioPolicyManager(clientInterface), mForceSubmixInputSelection(false)
{
}

float ATVAudioPolicyManager::computeVolume(audio_stream_type_t stream,
                                           int index,
                                           audio_io_handle_t output,
                                           audio_devices_t device)
{
    // We only use master volume, so all audio flinger streams
    // should be set to maximum
    (void)stream;
    (void)index;
    (void)output;
    (void)device;
    return 1.0;
}

status_t ATVAudioPolicyManager::setDeviceConnectionState(audio_devices_t device,
                                                         audio_policy_dev_state_t state,
                                                         const char *device_address)
{
    audio_devices_t tmp = AUDIO_DEVICE_NONE;;
    ALOGE("setDeviceConnectionState %08x %x %s", device, state,
          device_address ? device_address : "(null)");

    // If the input device is the remote submix and an address starting with "force=" was
    // specified, enable "force=1" / disable "force=0" the forced selection of the remote submix
    // input device over hardware input devices (e.g RemoteControl).
    if (device == AUDIO_DEVICE_IN_REMOTE_SUBMIX && device_address) {
        AudioParameter parameters = AudioParameter(String8(device_address));
        int forceValue;
        if (parameters.getInt(String8("force"), forceValue) == OK) {
            mForceSubmixInputSelection = forceValue != 0;
        }
    }

    if (audio_is_output_device(device)) {
      switch (state) {
          case AUDIO_POLICY_DEVICE_STATE_AVAILABLE:
              tmp = mAvailableOutputDevices.types() | device;
              break;

          case AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE:
              tmp = mAvailableOutputDevices.types() & ~device;
              break;
          default:
              ALOGE("setDeviceConnectionState() invalid state: %x", state);
              return BAD_VALUE;
      }

      gAudioHardwareOutput.updateRouting(tmp);
      tmp = mAvailableOutputDevices.types();
    }

    status_t ret = 0;
    if (device != AUDIO_DEVICE_IN_REMOTE_SUBMIX) {
      ret = AudioPolicyManager::setDeviceConnectionState(
                    device, state, device_address);
    }

    if (audio_is_output_device(device)) {
      if (tmp != mAvailableOutputDevices.types())
          gAudioHardwareOutput.updateRouting(mAvailableOutputDevices.types());
    }

    return ret;
}

audio_devices_t ATVAudioPolicyManager::getDeviceForInputSource(audio_source_t inputSource)
{
    uint32_t device = AUDIO_DEVICE_NONE;
    bool usePhysRemote = true;

    if (inputSource == AUDIO_SOURCE_VOICE_RECOGNITION) {
#ifdef REMOTE_CONTROL_INTERFACE
      // Check if remote is actually connected or we should move on
      sp<IRemoteControlService> service = IRemoteControlService::getInstance();
      if (service == NULL) {
          ALOGV("getDeviceForInputSource No RemoteControl service detected, ignoring");
          usePhysRemote = false;
      } else if (!service->hasActiveRemote()) {
          ALOGV("getDeviceForInputSource No active connected device, passing onto submix");
          usePhysRemote = false;
      }
#endif
      ALOGV("getDeviceForInputSource %s %s", usePhysRemote ? "use physical" : "",
          mForceSubmixInputSelection ? "use virtual" : "");
      audio_devices_t availableDeviceTypes = mAvailableInputDevices.types() &
                                                ~AUDIO_DEVICE_BIT_IN;
      if (availableDeviceTypes & AUDIO_DEVICE_IN_WIRED_HEADSET &&
            usePhysRemote) {
          // User a wired headset (physical remote) if available, connected and active
          ALOGV("Wired Headset available");
          device = AUDIO_DEVICE_IN_WIRED_HEADSET;
      } else if (availableDeviceTypes & AUDIO_DEVICE_IN_REMOTE_SUBMIX &&
            mForceSubmixInputSelection) {
          // REMOTE_SUBMIX should always be avaible, let's make sure it's being forced at the moment
          ALOGV("Virtual remote available");
          device = AUDIO_DEVICE_IN_REMOTE_SUBMIX;
      }
    }

    ALOGV("getDeviceForInputSource() input source %d, device %08x", inputSource, device);
    return device;
}

}  // namespace android
