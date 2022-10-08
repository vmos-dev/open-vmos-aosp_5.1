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

#ifndef ANDROID_AUDIO_HARDWARE_BASE_H
#define ANDROID_AUDIO_HARDWARE_BASE_H

#include <hardware_legacy/AudioHardwareInterface.h>

#include <system/audio.h>

namespace android_audio_legacy {

// ----------------------------------------------------------------------------

/**
 * AudioHardwareBase is a convenient base class used for implementing the
 * AudioHardwareInterface interface.
 */
class AudioHardwareBase : public AudioHardwareInterface
{
public:
                        AudioHardwareBase();
    virtual             ~AudioHardwareBase() { }

    /**
     * setMode is called when the audio mode changes. NORMAL mode is for
     * standard audio playback, RINGTONE when a ringtone is playing, IN_CALL
     * when a telephony call is in progress, IN_COMMUNICATION when a VoIP call is in progress.
     */
    virtual status_t    setMode(int mode);

    virtual status_t    setParameters(const String8& keyValuePairs);
    virtual String8     getParameters(const String8& keys);

    virtual  size_t     getInputBufferSize(uint32_t sampleRate, int format, int channelCount);
    virtual status_t    getMasterVolume(float *volume);

    /**This method dumps the state of the audio hardware */
    virtual status_t dumpState(int fd, const Vector<String16>& args);

protected:
    /** returns true if the given mode maps to a telephony or VoIP call is in progress */
    virtual bool     isModeInCall(int mode)
                        { return ((mode == AudioSystem::MODE_IN_CALL)
                                || (mode == AudioSystem::MODE_IN_COMMUNICATION)); };
    /** returns true if a telephony or VoIP call is in progress */
    virtual bool     isInCall() { return isModeInCall(mMode); };
    int              mMode;
};

}; // namespace android

#endif // ANDROID_AUDIO_HARDWARE_BASE_H
