/*
 * Copyright (C) 2009 The Android Open Source Project
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

#ifndef ANDROID_AUDIOPOLICYINTERFACE_H
#define ANDROID_AUDIOPOLICYINTERFACE_H

#include <media/AudioSystem.h>
#include <media/ToneGenerator.h>
#include <utils/String8.h>

#include <hardware_legacy/AudioSystemLegacy.h>
#include <hardware/audio_policy.h>

namespace android_audio_legacy {
    using android::Vector;
    using android::String8;
    using android::ToneGenerator;

// ----------------------------------------------------------------------------

// The AudioPolicyInterface and AudioPolicyClientInterface classes define the communication interfaces
// between the platform specific audio policy manager and Android generic audio policy manager.
// The platform specific audio policy manager must implement methods of the AudioPolicyInterface class.
// This implementation makes use of the AudioPolicyClientInterface to control the activity and
// configuration of audio input and output streams.
//
// The platform specific audio policy manager is in charge of the audio routing and volume control
// policies for a given platform.
// The main roles of this module are:
//   - keep track of current system state (removable device connections, phone state, user requests...).
//   System state changes and user actions are notified to audio policy manager with methods of the AudioPolicyInterface.
//   - process getOutput() queries received when AudioTrack objects are created: Those queries
//   return a handler on an output that has been selected, configured and opened by the audio policy manager and that
//   must be used by the AudioTrack when registering to the AudioFlinger with the createTrack() method.
//   When the AudioTrack object is released, a putOutput() query is received and the audio policy manager can decide
//   to close or reconfigure the output depending on other streams using this output and current system state.
//   - similarly process getInput() and putInput() queries received from AudioRecord objects and configure audio inputs.
//   - process volume control requests: the stream volume is converted from an index value (received from UI) to a float value
//   applicable to each output as a function of platform specific settings and current output route (destination device). It
//   also make sure that streams are not muted if not allowed (e.g. camera shutter sound in some countries).
//
// The platform specific audio policy manager is provided as a shared library by platform vendors (as for libaudio.so)
// and is linked with libaudioflinger.so


//    Audio Policy Manager Interface
class AudioPolicyInterface
{

public:
    virtual ~AudioPolicyInterface() {}
    //
    // configuration functions
    //

    // indicate a change in device connection status
    virtual status_t setDeviceConnectionState(audio_devices_t device,
                                          AudioSystem::device_connection_state state,
                                          const char *device_address) = 0;
    // retrieve a device connection status
    virtual AudioSystem::device_connection_state getDeviceConnectionState(audio_devices_t device,
                                                                          const char *device_address) = 0;
    // indicate a change in phone state. Valid phones states are defined by AudioSystem::audio_mode
    virtual void setPhoneState(int state) = 0;
    // force using a specific device category for the specified usage
    virtual void setForceUse(AudioSystem::force_use usage, AudioSystem::forced_config config) = 0;
    // retrieve current device category forced for a given usage
    virtual AudioSystem::forced_config getForceUse(AudioSystem::force_use usage) = 0;
    // set a system property (e.g. camera sound always audible)
    virtual void setSystemProperty(const char* property, const char* value) = 0;
    // check proper initialization
    virtual status_t initCheck() = 0;

    //
    // Audio routing query functions
    //

    // request an output appropriate for playback of the supplied stream type and parameters
    virtual audio_io_handle_t getOutput(AudioSystem::stream_type stream,
                                        uint32_t samplingRate,
                                        audio_format_t format,
                                        audio_channel_mask_t channelMask,
                                        AudioSystem::output_flags flags,
                                        const audio_offload_info_t *offloadInfo) = 0;
    // indicates to the audio policy manager that the output starts being used by corresponding stream.
    virtual status_t startOutput(audio_io_handle_t output,
                                 AudioSystem::stream_type stream,
                                 int session = 0) = 0;
    // indicates to the audio policy manager that the output stops being used by corresponding stream.
    virtual status_t stopOutput(audio_io_handle_t output,
                                AudioSystem::stream_type stream,
                                int session = 0) = 0;
    // releases the output.
    virtual void releaseOutput(audio_io_handle_t output) = 0;

    // request an input appropriate for record from the supplied device with supplied parameters.
    virtual audio_io_handle_t getInput(int inputSource,
                                    uint32_t samplingRate,
                                    audio_format_t format,
                                    audio_channel_mask_t channelMask,
                                    AudioSystem::audio_in_acoustics acoustics) = 0;
    // indicates to the audio policy manager that the input starts being used.
    virtual status_t startInput(audio_io_handle_t input) = 0;
    // indicates to the audio policy manager that the input stops being used.
    virtual status_t stopInput(audio_io_handle_t input) = 0;
    // releases the input.
    virtual void releaseInput(audio_io_handle_t input) = 0;

    //
    // volume control functions
    //

    // initialises stream volume conversion parameters by specifying volume index range.
    virtual void initStreamVolume(AudioSystem::stream_type stream,
                                      int indexMin,
                                      int indexMax) = 0;

    // sets the new stream volume at a level corresponding to the supplied index for the
    // supplied device. By convention, specifying AUDIO_DEVICE_OUT_DEFAULT means
    // setting volume for all devices
    virtual status_t setStreamVolumeIndex(AudioSystem::stream_type stream,
                                          int index,
                                          audio_devices_t device) = 0;

    // retrieve current volume index for the specified stream and the
    // specified device. By convention, specifying AUDIO_DEVICE_OUT_DEFAULT means
    // querying the volume of the active device.
    virtual status_t getStreamVolumeIndex(AudioSystem::stream_type stream,
                                          int *index,
                                          audio_devices_t device) = 0;

    // return the strategy corresponding to a given stream type
    virtual uint32_t getStrategyForStream(AudioSystem::stream_type stream) = 0;

    // return the enabled output devices for the given stream type
    virtual audio_devices_t getDevicesForStream(AudioSystem::stream_type stream) = 0;

    // Audio effect management
    virtual audio_io_handle_t getOutputForEffect(const effect_descriptor_t *desc) = 0;
    virtual status_t registerEffect(const effect_descriptor_t *desc,
                                    audio_io_handle_t io,
                                    uint32_t strategy,
                                    int session,
                                    int id) = 0;
    virtual status_t unregisterEffect(int id) = 0;
    virtual status_t setEffectEnabled(int id, bool enabled) = 0;

    virtual bool isStreamActive(int stream, uint32_t inPastMs = 0) const = 0;
    virtual bool isStreamActiveRemotely(int stream, uint32_t inPastMs = 0) const = 0;
    virtual bool isSourceActive(audio_source_t source) const = 0;

    //dump state
    virtual status_t    dump(int fd) = 0;

    virtual bool isOffloadSupported(const audio_offload_info_t& offloadInfo) = 0;
};


// Audio Policy client Interface
class AudioPolicyClientInterface
{
public:
    virtual ~AudioPolicyClientInterface() {}

    //
    // Audio HW module functions
    //

    // loads a HW module.
    virtual audio_module_handle_t loadHwModule(const char *name) = 0;

    //
    // Audio output Control functions
    //

    // opens an audio output with the requested parameters. The parameter values can indicate to use the default values
    // in case the audio policy manager has no specific requirements for the output being opened.
    // When the function returns, the parameter values reflect the actual values used by the audio hardware output stream.
    // The audio policy manager can check if the proposed parameters are suitable or not and act accordingly.
    virtual audio_io_handle_t openOutput(audio_module_handle_t module,
                                         audio_devices_t *pDevices,
                                         uint32_t *pSamplingRate,
                                         audio_format_t *pFormat,
                                         audio_channel_mask_t *pChannelMask,
                                         uint32_t *pLatencyMs,
                                         audio_output_flags_t flags,
                                         const audio_offload_info_t *offloadInfo = NULL) = 0;
    // creates a special output that is duplicated to the two outputs passed as arguments. The duplication is performed by
    // a special mixer thread in the AudioFlinger.
    virtual audio_io_handle_t openDuplicateOutput(audio_io_handle_t output1, audio_io_handle_t output2) = 0;
    // closes the output stream
    virtual status_t closeOutput(audio_io_handle_t output) = 0;
    // suspends the output. When an output is suspended, the corresponding audio hardware output stream is placed in
    // standby and the AudioTracks attached to the mixer thread are still processed but the output mix is discarded.
    virtual status_t suspendOutput(audio_io_handle_t output) = 0;
    // restores a suspended output.
    virtual status_t restoreOutput(audio_io_handle_t output) = 0;

    //
    // Audio input Control functions
    //

    // opens an audio input
    virtual audio_io_handle_t openInput(audio_module_handle_t module,
                                        audio_devices_t *pDevices,
                                        uint32_t *pSamplingRate,
                                        audio_format_t *pFormat,
                                        audio_channel_mask_t *pChannelMask) = 0;
    // closes an audio input
    virtual status_t closeInput(audio_io_handle_t input) = 0;
    //
    // misc control functions
    //

    // set a stream volume for a particular output. For the same user setting, a given stream type can have different volumes
    // for each output (destination device) it is attached to.
    virtual status_t setStreamVolume(AudioSystem::stream_type stream, float volume, audio_io_handle_t output, int delayMs = 0) = 0;

    // invalidate a stream type, causing a reroute to an unspecified new output
    virtual status_t invalidateStream(AudioSystem::stream_type stream) = 0;

    // function enabling to send proprietary informations directly from audio policy manager to audio hardware interface.
    virtual void setParameters(audio_io_handle_t ioHandle, const String8& keyValuePairs, int delayMs = 0) = 0;
    // function enabling to receive proprietary informations directly from audio hardware interface to audio policy manager.
    virtual String8 getParameters(audio_io_handle_t ioHandle, const String8& keys) = 0;

    // request the playback of a tone on the specified stream: used for instance to replace notification sounds when playing
    // over a telephony device during a phone call.
    virtual status_t startTone(ToneGenerator::tone_type tone, AudioSystem::stream_type stream) = 0;
    virtual status_t stopTone() = 0;

    // set down link audio volume.
    virtual status_t setVoiceVolume(float volume, int delayMs = 0) = 0;

    // move effect to the specified output
    virtual status_t moveEffects(int session,
                                     audio_io_handle_t srcOutput,
                                     audio_io_handle_t dstOutput) = 0;

};

extern "C" AudioPolicyInterface* createAudioPolicyManager(AudioPolicyClientInterface *clientInterface);
extern "C" void destroyAudioPolicyManager(AudioPolicyInterface *interface);


}; // namespace android

#endif // ANDROID_AUDIOPOLICYINTERFACE_H
