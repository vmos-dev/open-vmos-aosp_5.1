/* AudioUsbALSA.h

Copyright (c) 2012, Code Aurora Forum. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of Code Aurora Forum, Inc. nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.*/

#ifndef ANDROID_AUDIO_USB_ALSA_H
#define ANDROID_AUDIO_USB_ALSA_H

#include <utils/List.h>
#include <hardware_legacy/AudioHardwareBase.h>

#include <hardware_legacy/AudioHardwareInterface.h>
#include <hardware_legacy/AudioSystemLegacy.h>
#include <system/audio.h>
#include <hardware/audio.h>
#include <utils/threads.h>

#define DEFAULT_BUFFER_SIZE   2048
#define POLL_TIMEOUT   3000
#define DEFAULT_CHANNEL_MODE  2
#define CHANNEL_MODE_ONE  1
#define PROXY_DEFAULT_SAMPLING_RATE 48000
#define SIGNAL_EVENT_TIMEOUT 1
#define SIGNAL_EVENT_KILLTHREAD 2

#define BUFFSIZE 1000000

#define PATH "/proc/asound/card1/stream0"

extern "C" {
   #include <sound/asound.h>
   #include "alsa_audio.h"
   #include "msm8960_use_cases.h"
}

#include <hardware/hardware.h>

namespace android_audio_legacy
{
using android::List;
using android::Mutex;
class AudioUsbALSA;

class AudioUsbALSA
{
private:
    int mproxypfdPlayback;
    int musbpfdPlayback;
    int mnfdsPlayback;
    int mnfdsRecording;
    int mtimeOut;
    int mtimeOutRecording;
    struct pcm *mproxyRecordingHandle;
    struct pcm *musbRecordingHandle;
    struct pcm *mproxyPlaybackHandle;
    struct pcm *musbPlaybackHandle;
    u_int8_t *mdstUsb_addr;
    u_int8_t *msrcProxy_addr;
    bool mkillPlayBackThread;
    bool mkillRecordingThread;
    pthread_t mPlaybackUsb;
    pthread_t mRecordingUsb;
    snd_use_case_mgr_t *mUcMgr;

    //Helper functions
    struct pcm * configureDevice(unsigned flags, char* hw, int sampleRate, int channelCount, int periodSize, bool playback);
    status_t syncPtr(struct pcm *handle, bool *killThread);

    //playback
    void pollForProxyData();
    void pollForUsbData();

    //recording
    void pollForUsbDataForRecording();
    void pollForProxyDataForRecording();

    status_t startDevice(pcm *handle, bool *killThread);

    void PlaybackThreadEntry();
    static void *PlaybackThreadWrapper(void *me);

    void RecordingThreadEntry();
    static void *RecordingThreadWrapper(void *me);

    status_t setHardwareParams(pcm *local_handle, uint32_t sampleRate, uint32_t channels, int periodSize);

    status_t setSoftwareParams(pcm *pcm, bool playback);

    status_t closeDevice(pcm *handle);

    status_t getCap(char * type, int &channels, int &sampleRate);
    int         getnumOfRates(char *rateStr);
    int         mchannelsPlayback;
    int         msampleRatePlayback;
    int         mchannelsCapture;
    int         msampleRateCapture;

public:
    AudioUsbALSA();
    virtual            ~AudioUsbALSA();

    void exitPlaybackThread(uint64_t writeVal);
    void exitRecordingThread(uint64_t writeVal);
    void setkillUsbRecordingThread(bool val);
    bool getkillUsbPlaybackThread() {
        return mkillPlayBackThread;
    }
    bool getkillUsbRecordingThread() {
        return mkillRecordingThread;
    }
    //Playback
    void startPlayback();

    //Capture
    void startRecording();
};

};        // namespace android_audio_legacy
#endif    // ANDROID_AUDIO_USB_ALSA_H
