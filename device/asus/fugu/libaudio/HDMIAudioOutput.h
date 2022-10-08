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

#ifndef ANDROID_HDMI_AUDIO_OUTPUT_H
#define ANDROID_HDMI_AUDIO_OUTPUT_H

#include <hardware/audio.h>

#include "AudioOutput.h"

namespace android {

class AudioStreamOut;

class HDMIAudioOutput : public AudioOutput {
  public:
                        HDMIAudioOutput();
    virtual            ~HDMIAudioOutput();
    virtual status_t    setupForStream(const AudioStreamOut& stream);
    virtual const char* getOutputName() { return "HDMI"; }
    static  uint32_t    classDevMask() { return AUDIO_DEVICE_OUT_AUX_DIGITAL; }
    virtual uint32_t    devMask() const { return classDevMask(); }
    virtual void        dump(String8& result);

protected:
    virtual void        applyPendingVolParams();
};

}  // namespace android
#endif  // ANDROID_HDMI_AUDIO_OUTPUT_H
