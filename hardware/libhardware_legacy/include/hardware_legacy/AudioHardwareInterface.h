/*
 * Copyright (C) 2007 The Android Open Source Project
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

#ifndef ANDROID_AUDIO_HARDWARE_INTERFACE_H
#define ANDROID_AUDIO_HARDWARE_INTERFACE_H

#include <stdint.h>
#include <sys/types.h>

#include <utils/Errors.h>
#include <utils/Vector.h>
#include <utils/String16.h>
#include <utils/String8.h>

#include <media/IAudioFlinger.h>
#include <hardware_legacy/AudioSystemLegacy.h>

#include <system/audio.h>
#include <hardware/audio.h>

#include <cutils/bitops.h>

namespace android_audio_legacy {
    using android::Vector;
    using android::String16;
    using android::String8;

// ----------------------------------------------------------------------------

/**
 * AudioStreamOut is the abstraction interface for the audio output hardware.
 *
 * It provides information about various properties of the audio output hardware driver.
 */
class AudioStreamOut {
public:
    virtual             ~AudioStreamOut() = 0;

    /** return audio sampling rate in hz - eg. 44100 */
    virtual uint32_t    sampleRate() const = 0;

    /** returns size of output buffer - eg. 4800 */
    virtual size_t      bufferSize() const = 0;

    /**
     * returns the output channel mask
     */
    virtual uint32_t    channels() const = 0;

    /**
     * return audio format in 8bit or 16bit PCM format -
     * eg. AudioSystem:PCM_16_BIT
     */
    virtual int         format() const = 0;

    /**
     * return the frame size (number of bytes per sample).
     */
    uint32_t    frameSize() const { return audio_channel_count_from_out_mask(channels())*
                            ((format()==AUDIO_FORMAT_PCM_16_BIT)?sizeof(int16_t):sizeof(int8_t)); }

    /**
     * return the audio hardware driver latency in milli seconds.
     */
    virtual uint32_t    latency() const = 0;

    /**
     * Use this method in situations where audio mixing is done in the
     * hardware. This method serves as a direct interface with hardware,
     * allowing you to directly set the volume as apposed to via the framework.
     * This method might produce multiple PCM outputs or hardware accelerated
     * codecs, such as MP3 or AAC.
     */
    virtual status_t    setVolume(float left, float right) = 0;

    /** write audio buffer to driver. Returns number of bytes written */
    virtual ssize_t     write(const void* buffer, size_t bytes) = 0;

    /**
     * Put the audio hardware output into standby mode. Returns
     * status based on include/utils/Errors.h
     */
    virtual status_t    standby() = 0;

    /** dump the state of the audio output device */
    virtual status_t dump(int fd, const Vector<String16>& args) = 0;

    // set/get audio output parameters. The function accepts a list of parameters
    // key value pairs in the form: key1=value1;key2=value2;...
    // Some keys are reserved for standard parameters (See AudioParameter class).
    // If the implementation does not accept a parameter change while the output is
    // active but the parameter is acceptable otherwise, it must return INVALID_OPERATION.
    // The audio flinger will put the output in standby and then change the parameter value.
    virtual status_t    setParameters(const String8& keyValuePairs) = 0;
    virtual String8     getParameters(const String8& keys) = 0;

    // return the number of audio frames written by the audio dsp to DAC since
    // the output has exited standby
    virtual status_t    getRenderPosition(uint32_t *dspFrames) = 0;

    /**
     * get the local time at which the next write to the audio driver will be
     * presented
     */
    virtual status_t    getNextWriteTimestamp(int64_t *timestamp);

    /**
     * Return a recent count of the number of audio frames presented to an external observer.
     */
    virtual status_t    getPresentationPosition(uint64_t *frames, struct timespec *timestamp);

};

/**
 * AudioStreamIn is the abstraction interface for the audio input hardware.
 *
 * It defines the various properties of the audio hardware input driver.
 */
class AudioStreamIn {
public:
    virtual             ~AudioStreamIn() = 0;

    /** return audio sampling rate in hz - eg. 44100 */
    virtual uint32_t    sampleRate() const = 0;

    /** return the input buffer size allowed by audio driver */
    virtual size_t      bufferSize() const = 0;

    /** return input channel mask */
    virtual uint32_t    channels() const = 0;

    /**
     * return audio format in 8bit or 16bit PCM format -
     * eg. AudioSystem:PCM_16_BIT
     */
    virtual int         format() const = 0;

    /**
     * return the frame size (number of bytes per sample).
     */
    uint32_t    frameSize() const { return audio_channel_count_from_in_mask(channels())*
                            ((format()==AudioSystem::PCM_16_BIT)?sizeof(int16_t):sizeof(int8_t)); }

    /** set the input gain for the audio driver. This method is for
     *  for future use */
    virtual status_t    setGain(float gain) = 0;

    /** read audio buffer in from audio driver */
    virtual ssize_t     read(void* buffer, ssize_t bytes) = 0;

    /** dump the state of the audio input device */
    virtual status_t dump(int fd, const Vector<String16>& args) = 0;

    /**
     * Put the audio hardware input into standby mode. Returns
     * status based on include/utils/Errors.h
     */
    virtual status_t    standby() = 0;

    // set/get audio input parameters. The function accepts a list of parameters
    // key value pairs in the form: key1=value1;key2=value2;...
    // Some keys are reserved for standard parameters (See AudioParameter class).
    // If the implementation does not accept a parameter change while the output is
    // active but the parameter is acceptable otherwise, it must return INVALID_OPERATION.
    // The audio flinger will put the input in standby and then change the parameter value.
    virtual status_t    setParameters(const String8& keyValuePairs) = 0;
    virtual String8     getParameters(const String8& keys) = 0;


    // Return the number of input frames lost in the audio driver since the last call of this function.
    // Audio driver is expected to reset the value to 0 and restart counting upon returning the current value by this function call.
    // Such loss typically occurs when the user space process is blocked longer than the capacity of audio driver buffers.
    // Unit: the number of input audio frames
    virtual unsigned int  getInputFramesLost() const = 0;

    virtual status_t addAudioEffect(effect_handle_t effect) = 0;
    virtual status_t removeAudioEffect(effect_handle_t effect) = 0;
};

/**
 * AudioHardwareInterface.h defines the interface to the audio hardware abstraction layer.
 *
 * The interface supports setting and getting parameters, selecting audio routing
 * paths, and defining input and output streams.
 *
 * AudioFlinger initializes the audio hardware and immediately opens an output stream.
 * You can set Audio routing to output to handset, speaker, Bluetooth, or a headset.
 *
 * The audio input stream is initialized when AudioFlinger is called to carry out
 * a record operation.
 */
class AudioHardwareInterface
{
public:
    virtual ~AudioHardwareInterface() {}

    /**
     * check to see if the audio hardware interface has been initialized.
     * return status based on values defined in include/utils/Errors.h
     */
    virtual status_t    initCheck() = 0;

    /** set the audio volume of a voice call. Range is between 0.0 and 1.0 */
    virtual status_t    setVoiceVolume(float volume) = 0;

    /**
     * set the audio volume for all audio activities other than voice call.
     * Range between 0.0 and 1.0. If any value other than NO_ERROR is returned,
     * the software mixer will emulate this capability.
     */
    virtual status_t    setMasterVolume(float volume) = 0;

    /**
     * Get the current master volume value for the HAL, if the HAL supports
     * master volume control.  AudioFlinger will query this value from the
     * primary audio HAL when the service starts and use the value for setting
     * the initial master volume across all HALs.
     */
    virtual status_t    getMasterVolume(float *volume) = 0;

    /**
     * setMode is called when the audio mode changes. NORMAL mode is for
     * standard audio playback, RINGTONE when a ringtone is playing, and IN_CALL
     * when a call is in progress.
     */
    virtual status_t    setMode(int mode) = 0;

    // mic mute
    virtual status_t    setMicMute(bool state) = 0;
    virtual status_t    getMicMute(bool* state) = 0;

    // set/get global audio parameters
    virtual status_t    setParameters(const String8& keyValuePairs) = 0;
    virtual String8     getParameters(const String8& keys) = 0;

    // Returns audio input buffer size according to parameters passed or 0 if one of the
    // parameters is not supported
    virtual size_t    getInputBufferSize(uint32_t sampleRate, int format, int channelCount) = 0;

    /** This method creates and opens the audio hardware output stream */
    virtual AudioStreamOut* openOutputStream(
                                uint32_t devices,
                                int *format=0,
                                uint32_t *channels=0,
                                uint32_t *sampleRate=0,
                                status_t *status=0) = 0;
    virtual AudioStreamOut* openOutputStreamWithFlags(
                                uint32_t devices,
                                audio_output_flags_t flags=(audio_output_flags_t)0,
                                int *format=0,
                                uint32_t *channels=0,
                                uint32_t *sampleRate=0,
                                status_t *status=0) = 0;
    virtual    void        closeOutputStream(AudioStreamOut* out) = 0;

    /** This method creates and opens the audio hardware input stream */
    virtual AudioStreamIn* openInputStream(
                                uint32_t devices,
                                int *format,
                                uint32_t *channels,
                                uint32_t *sampleRate,
                                status_t *status,
                                AudioSystem::audio_in_acoustics acoustics) = 0;
    virtual    void        closeInputStream(AudioStreamIn* in) = 0;

    /**This method dumps the state of the audio hardware */
    virtual status_t dumpState(int fd, const Vector<String16>& args) = 0;

    virtual status_t setMasterMute(bool muted) = 0;

    static AudioHardwareInterface* create();

    virtual int createAudioPatch(unsigned int num_sources,
                               const struct audio_port_config *sources,
                               unsigned int num_sinks,
                               const struct audio_port_config *sinks,
                               audio_patch_handle_t *handle) = 0;

    virtual int releaseAudioPatch(audio_patch_handle_t handle) = 0;

    virtual int getAudioPort(struct audio_port *port) = 0;

    virtual int setAudioPortConfig(const struct audio_port_config *config) = 0;

protected:

    virtual status_t dump(int fd, const Vector<String16>& args) = 0;
};

// ----------------------------------------------------------------------------

extern "C" AudioHardwareInterface* createAudioHardware(void);

}; // namespace android

#endif // ANDROID_AUDIO_HARDWARE_INTERFACE_H
