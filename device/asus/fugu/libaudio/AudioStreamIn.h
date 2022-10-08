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

#ifndef ANDROID_AUDIO_STREAM_IN_H
#define ANDROID_AUDIO_STREAM_IN_H

#include <audio_utils/resampler.h>
#include <hardware/audio.h>
#include <tinyalsa/asoundlib.h>
#include <utils/Errors.h>
#include <utils/threads.h>

#include "AudioHotplugThread.h"

namespace android {

class AudioHardwareInput;

class AudioStreamIn {
  public:
    AudioStreamIn(AudioHardwareInput& owner);
    ~AudioStreamIn();

    uint32_t          getSampleRate();
    status_t          setSampleRate(uint32_t rate);
    size_t            getBufferSize();
    uint32_t          getChannelMask();
    audio_format_t    getFormat();
    status_t          setFormat(audio_format_t format);
    status_t          standby();
    status_t          dump(int fd);
    status_t          setParameters(struct audio_stream* stream,
                                    const char* kvpairs);
    char*             getParameters(const char* keys);
    status_t          setGain(float gain);
    ssize_t           read(void* buffer, size_t bytes);
    uint32_t          getInputFramesLost();
    status_t          addAudioEffect(effect_handle_t effect);
    status_t          removeAudioEffect(effect_handle_t effect);

    status_t          set(audio_format_t *pFormat,
                          uint32_t       *pChannelMask,
                          uint32_t       *pRate);

    const AudioHotplugThread::DeviceInfo* getDeviceInfo() { return mCurrentDeviceInfo; };

  private:
    static const uint32_t kChannelMask;
    static const uint32_t kChannelCount;
    static const audio_format_t kAudioFormat;

    static uint32_t   getChannelCount() {
        return audio_channel_count_from_in_mask(kChannelMask);
    }

    static uint32_t   getFrameSize() {
        return getChannelCount() * audio_bytes_per_sample(kAudioFormat);
    }

    void              setRemoteControlMicEnabled(bool flag);

    status_t          startInputStream_l();
    status_t          standby_l();

    ssize_t           readFrames_l(void* buffer, ssize_t frames);

    // resampler buffer provider thunks
    static int        getNextBufferThunk(
            struct resampler_buffer_provider* bufferProvider,
            struct resampler_buffer* buffer);
    static void       releaseBufferThunk(
            struct resampler_buffer_provider* bufferProvider,
            struct resampler_buffer* buffer);

    // resampler buffer provider methods
    int               getNextBuffer(struct resampler_buffer* buffer);
    void              releaseBuffer(struct resampler_buffer* buffer);

    static const int  kPeriodCount;

    AudioHardwareInput& mOwnerHAL;
    const AudioHotplugThread::DeviceInfo* mCurrentDeviceInfo;

    Mutex mLock;

    uint32_t          mRequestedSampleRate;
    bool              mStandby;
    bool              mDisabled;

    struct pcm*       mPcm;
    struct pcm_config mPcmConfig;

    struct resampler_itfe*         mResampler;
    struct ResamplerBufferProviderWrapper {
        struct resampler_buffer_provider provider;
        AudioStreamIn* thiz;
    } mResamplerProviderWrapper;

    int16_t*          mBuffer;
    size_t            mBufferSize;
    int               mInputSource;
    int               mReadStatus;
    unsigned int      mFramesIn;
};

}; // namespace android

#endif  // ANDROID_AUDIO_STREAM_IN_H
