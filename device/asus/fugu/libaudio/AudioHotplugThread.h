/*
**
** Copyright 2012, The Android Open Source Project
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

#ifndef ANDROID_AUDIO_HOTPLUG_THREAD_H
#define ANDROID_AUDIO_HOTPLUG_THREAD_H

#include <utils/threads.h>

namespace android {

class AudioHotplugThread : public Thread {
  public:
    struct DeviceInfo {
        unsigned int pcmCard;
        unsigned int pcmDevice;
        unsigned int minSampleBits, maxSampleBits;
        unsigned int minChannelCount, maxChannelCount;
        unsigned int minSampleRate, maxSampleRate;
        bool valid;
        bool forVoiceRecognition;
    };

    class Callback {
      public:
        virtual ~Callback() {}
        virtual void onDeviceFound(const DeviceInfo& devInfo) = 0;
        virtual void onDeviceRemoved(unsigned int pcmCard, unsigned int pcmDevice) = 0;
    };

    AudioHotplugThread(Callback& callback);
    virtual ~AudioHotplugThread();

    bool        start();
    void        shutdown();

  private:
    static const char* kThreadName;
    static const char* kAlsaDeviceDir;
    static const char  kDeviceTypeCapture;
    static bool parseCaptureDeviceName(const char *name, unsigned int *pcmCard,
                                       unsigned int *pcmDevice);
    static bool getDeviceInfo(unsigned int pcmCard, unsigned int pcmDevice,
                              DeviceInfo* info);

    virtual bool threadLoop();

    void scanForDevice();

    Callback& mCallback;
    int mShutdownEventFD;
};

}; // namespace android

#endif  // ANDROID_AUDIO_HOTPLUG_THREAD_H
