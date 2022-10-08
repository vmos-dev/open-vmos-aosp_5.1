/*
**
** Copyright 2014, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef ANDROID_AUDIO_HARDWARE_OUTPUT_H
#define ANDROID_AUDIO_HARDWARE_OUTPUT_H

#include <stdint.h>
#include <sys/types.h>

#include <hardware/audio.h>
#include <utils/String8.h>
#include <utils/threads.h>

#include "alsa_utils.h"
#include "AudioOutput.h"

namespace android {

class AudioStreamOut;
class AudioOutput;

class AudioHardwareOutput {
  public:
                AudioHardwareOutput();
    virtual    ~AudioHardwareOutput();
    status_t    initCheck();
    status_t    setMasterVolume(float volume);
    status_t    getMasterVolume(float* volume);
    status_t    setMasterMute(bool mute);
    status_t    getMasterMute(bool* mute);
    status_t    setParameters(const char* kvpairs);
    char*       getParameters(const char* keys);
    status_t    dump(int fd);
    void        updateRouting(uint32_t devMask);
    uint32_t    getMaxDelayCompUsec() const { return mMaxDelayCompUsec; }
    uint32_t    getVideoDelayCompUsec() const {
        return mSettings.videoDelayCompUsec;
    }
    HDMIAudioCaps& getHDMIAudioCaps() { return mHDMIAudioCaps; }

    // Interface to allow streams to obtain and release various physical
    // outputs.
    status_t       obtainOutput(const AudioStreamOut& tgtStream,
                             uint32_t devMask,
                             sp<AudioOutput>* newOutput);
    void           releaseOutput(const AudioStreamOut& tgtStream,
                              const sp<AudioOutput>& releaseMe);


    // create I/O streams
    AudioStreamOut* openOutputStream(uint32_t  devices,
                                     audio_format_t *format,
                                     uint32_t *channels,
                                     uint32_t *sampleRate,
                                     audio_output_flags_t flags,
                                     status_t *status);
    void           closeOutputStream(AudioStreamOut* out);

    void           standbyStatusUpdate(bool isInStandby, bool isMCStream);

  private:
    struct OutputSettings {
        bool        allowed;
        uint32_t    delayCompUsec;
        bool        isFixed;
        float       fixedLvl;
        void        setDefaults();
    };

    struct Settings {
        OutputSettings hdmi;
        uint32_t       videoDelayCompUsec;
        float          masterVolume;
        bool           masterMute;
        void           setDefaults();
    };

    void     updateTgtDevices_l();
    bool     applyOutputSettings_l(const OutputSettings& initial,
                                   const OutputSettings& current,
                                   OutputSettings& updateMe,
                                   uint32_t outDevMask);

    // Notes on locking:
    // There are 3 locks in the AudioHardware class; mStreamLock, mOutputLock
    // and mSettingsLock.
    //
    // mStreamLock is held when interacting with AudioStreamOuts, in particular
    // during creation and destruction of streams, and during routing changes
    // (HDMI connecting and disconnecting) which (potentially) end up effecting
    // the target device masks of the output streams.
    //
    // mOutputLock is held while interacting with AudioOutputs (which represent
    // the physical outputs of the system).  AudioStreamOuts grab this lock
    // during calls to (obtain|release)Output which can trigger instantiation
    // and destruction of AudioOutputs.  During this operation, the
    // AudioStreamOut instance will be holding its own "routing" lock.  Care
    // should be taken to never hold the output lock or setting lock while making
    // a call into an AudioStreamOut which may obtain the routing lock.
    // Currently, the set of publicly accessible calls in AudioStreamOut which
    // may obtain the routing lock are...
    // 1) ~AudioStreamOut (calls releaseAllOutputs)
    // 2) standby (calls releaseAllOutputs)
    // 3) setTgtDevices
    //
    // mSettingsLock is held while reading settings and while writing/applying
    // settings to existing outputs.  Lock ordering is important when applying
    // settings to outputs as the both the output and settings lock need to be
    // held at the same time.  Whenever settings need to be applied to outputs,
    // the output lock should always obtained first, followed by the settings
    // lock.

    Mutex            mStreamLock;
    AudioStreamOut  *mMainOutput;
    AudioStreamOut  *mMCOutput;
    bool             mHDMIConnected;

    Mutex            mOutputLock;
    AudioOutputList  mPhysOutputs;

    Mutex            mSettingsLock;
    Settings         mSettings;
    uint32_t         mMaxDelayCompUsec;

    HDMIAudioCaps    mHDMIAudioCaps;
    int              mHDMICardID;

    static const String8 kHDMIAllowedParamKey;
    static const String8 kHDMIDelayCompParamKey;
    static const String8 kFixedHDMIOutputParamKey;
    static const String8 kFixedHDMIOutputLevelParamKey;
    static const String8 kVideoDelayCompParamKey;
    static const float   kDefaultMasterVol;

};

// ----------------------------------------------------------------------------

}; // namespace android

#endif  // ANDROID_AUDIO_HARDWARE_OUTPUT_H
