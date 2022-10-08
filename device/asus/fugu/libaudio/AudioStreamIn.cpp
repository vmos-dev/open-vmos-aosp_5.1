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

#define LOG_TAG "AudioHAL:AudioStreamIn"
#include <utils/Log.h>

#include "AudioStreamIn.h"
#include "AudioHardwareInput.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <utils/String8.h>
#include <media/AudioParameter.h>

// for turning Remote mic on/off
#ifdef REMOTE_CONTROL_INTERFACE
#include <IRemoteControlService.h>
#endif

namespace android {

const audio_format_t AudioStreamIn::kAudioFormat = AUDIO_FORMAT_PCM_16_BIT;
const uint32_t AudioStreamIn::kChannelMask = AUDIO_CHANNEL_IN_MONO;

// number of periods in the ALSA buffer
const int AudioStreamIn::kPeriodCount = 4;

AudioStreamIn::AudioStreamIn(AudioHardwareInput& owner)
    : mOwnerHAL(owner)
    , mCurrentDeviceInfo(NULL)
    , mRequestedSampleRate(0)
    , mStandby(true)
    , mDisabled(false)
    , mPcm(NULL)
    , mResampler(NULL)
    , mBuffer(NULL)
    , mBufferSize(0)
    , mInputSource(AUDIO_SOURCE_DEFAULT)
    , mReadStatus(0)
    , mFramesIn(0)
{
    struct resampler_buffer_provider& provider =
            mResamplerProviderWrapper.provider;
    provider.get_next_buffer = getNextBufferThunk;
    provider.release_buffer = releaseBufferThunk;
    mResamplerProviderWrapper.thiz = this;
}

AudioStreamIn::~AudioStreamIn()
{
    Mutex::Autolock _l(mLock);
    standby_l();
}

// Perform stream initialization that may fail.
// Must only be called once at construction time.
status_t AudioStreamIn::set(audio_format_t *pFormat, uint32_t *pChannelMask,
                            uint32_t *pRate)
{
    Mutex::Autolock _l(mLock);

    assert(mRequestedSampleRate == 0);

    // Respond with a request for mono if a different format is given.
    if (*pChannelMask != kChannelMask) {
        *pChannelMask = kChannelMask;
        return BAD_VALUE;
    }

    if (*pFormat != kAudioFormat) {
        *pFormat = kAudioFormat;
        return BAD_VALUE;
    }

    mRequestedSampleRate = *pRate;

    return NO_ERROR;
}

uint32_t AudioStreamIn::getSampleRate()
{
    Mutex::Autolock _l(mLock);
    return mRequestedSampleRate;
}

status_t AudioStreamIn::setSampleRate(uint32_t rate)
{
    (void) rate;
    // this is a no-op in other audio HALs
    return NO_ERROR;
}

size_t AudioStreamIn::getBufferSize()
{
    Mutex::Autolock _l(mLock);

    size_t size = AudioHardwareInput::calculateInputBufferSize(
        mRequestedSampleRate, kAudioFormat, getChannelCount());
    return size;
}

uint32_t AudioStreamIn::getChannelMask()
{
    return kChannelMask;
}

audio_format_t AudioStreamIn::getFormat()
{
    return kAudioFormat;
}

status_t AudioStreamIn::setFormat(audio_format_t format)
{
    (void) format;
    // other audio HALs fail any call to this API (even if the format matches
    // the current format)
    return INVALID_OPERATION;
}

status_t AudioStreamIn::standby()
{
    Mutex::Autolock _l(mLock);
    return standby_l();
}

status_t AudioStreamIn::standby_l()
{
    if (mStandby) {
        return NO_ERROR;
    }
    if (mPcm) {
        ALOGD("AudioStreamIn::standby_l, call pcm_close()");
        pcm_close(mPcm);
        mPcm = NULL;
    }

    // Turn OFF Remote MIC if we were recording from Remote.
    if (mCurrentDeviceInfo != NULL) {
        if (mCurrentDeviceInfo->forVoiceRecognition) {
            setRemoteControlMicEnabled(false);
        }
    }

    if (mResampler) {
        release_resampler(mResampler);
        mResampler = NULL;
    }
    if (mBuffer) {
        delete [] mBuffer;
        mBuffer = NULL;
    }

    mCurrentDeviceInfo = NULL;
    mStandby = true;
    mDisabled = false;

    return NO_ERROR;
}

#define DUMP(a...) \
    snprintf(buffer, SIZE, a); \
    buffer[SIZE - 1] = 0; \
    result.append(buffer);

status_t AudioStreamIn::dump(int fd)
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;
    DUMP("\n AudioStreamIn::dump\n");

    {
        DUMP("\toutput sample rate: %d\n", mRequestedSampleRate);
        if (mPcm) {
            DUMP("\tinput sample rate: %d\n", mPcmConfig.rate);
            DUMP("\tinput channels: %d\n", mPcmConfig.channels);
        }
    }

    ::write(fd, result.string(), result.size());

    return NO_ERROR;
}

status_t AudioStreamIn::setParameters(struct audio_stream* stream,
                                      const char* kvpairs)
{
    (void) stream;
    AudioParameter param = AudioParameter(String8(kvpairs));
    status_t status = NO_ERROR;
    String8 keySource = String8(AudioParameter::keyInputSource);
    int intVal;

    if (param.getInt(keySource, intVal) == NO_ERROR) {
        ALOGI("AudioStreamIn::setParameters, mInputSource set to %d", intVal);
        mInputSource = intVal;
    }

    return status;
}

char* AudioStreamIn::getParameters(const char* keys)
{
    (void) keys;
    return strdup("");
}

status_t AudioStreamIn::setGain(float gain)
{
    (void) gain;
    // In other HALs, this is a no-op and returns success.
    return NO_ERROR;
}

uint32_t AudioStreamIn::getInputFramesLost()
{
    return 0;
}

status_t AudioStreamIn::addAudioEffect(effect_handle_t effect)
{
    (void) effect;
    // In other HALs, this is a no-op and returns success.
    return 0;
}

status_t AudioStreamIn::removeAudioEffect(effect_handle_t effect)
{
    (void) effect;
    // In other HALs, this is a no-op and returns success.
    return 0;
}

ssize_t AudioStreamIn::read(void* buffer, size_t bytes)
{
    Mutex::Autolock _l(mLock);

    status_t status = NO_ERROR;

    if (mStandby) {
        status = startInputStream_l();
        // Only try to start once to prevent pointless spew.
        // If mic is not available then read will return silence.
        // This is needed to prevent apps from hanging.
        mStandby = false;
        if (status != NO_ERROR) {
            mDisabled = true;
        }
    }

    if ((status == NO_ERROR) && !mDisabled) {
        int ret = readFrames_l(buffer, bytes / getFrameSize());
        status = (ret < 0) ? INVALID_OPERATION : NO_ERROR;
    }

    if ((status != NO_ERROR) || mDisabled) {
        memset(buffer, 0, bytes);

        // TODO: This code needs to project a timeline based on the number
        // of audio frames synthesized from the last time we returned data
        // from an actual audio device (or establish a fake timeline to obey
        // if we have never returned any data from an actual device and need
        // to synth on the first call to read)
        usleep(bytes * 1000000 / getFrameSize() / mRequestedSampleRate);
    } else {
        bool mute;
        mOwnerHAL.getMicMute(&mute);
        if (mute) {
            memset(buffer, 0, bytes);
        }
    }

    return bytes;
}

void AudioStreamIn::setRemoteControlMicEnabled(bool flag)
{
#ifdef REMOTE_CONTROL_INTERFACE
    sp<IRemoteControlService> service = IRemoteControlService::getInstance();
    if (service == NULL) {
        ALOGE("%s: No RemoteControl service detected, ignoring\n", __func__);
        return;
    }
    service->setMicEnabled(flag);
#else
    (void)flag;
#endif
}

status_t AudioStreamIn::startInputStream_l()
{

    ALOGI("AudioStreamIn::startInputStream_l, entry, built %s", __DATE__);

    // Get the most appropriate device for the given input source, eg VOICE_RECOGNITION
    const AudioHotplugThread::DeviceInfo *deviceInfo = mOwnerHAL.getBestDevice(mInputSource);
    if (deviceInfo == NULL) {
        return INVALID_OPERATION;
    }

    memset(&mPcmConfig, 0, sizeof(mPcmConfig));

    unsigned int requestedChannelCount = getChannelCount();

    // Clip to min/max available.
    if (requestedChannelCount < deviceInfo->minChannelCount ) {
        mPcmConfig.channels = deviceInfo->minChannelCount;
    } else if (requestedChannelCount > deviceInfo->maxChannelCount ) {
        mPcmConfig.channels = deviceInfo->maxChannelCount;
    } else {
        mPcmConfig.channels = requestedChannelCount;
    }

    ALOGD("AudioStreamIn::startInputStream_l, mRequestedSampleRate = %d",
        mRequestedSampleRate);

    // Clip to min/max available from driver.
    uint32_t chosenSampleRate = mRequestedSampleRate;
    if (chosenSampleRate < deviceInfo->minSampleRate) {
        chosenSampleRate = deviceInfo->minSampleRate;
    } else if (chosenSampleRate > deviceInfo->maxSampleRate) {
        chosenSampleRate = deviceInfo->maxSampleRate;
    }

    // Turn on RemoteControl MIC if we are recording from it.
    if (deviceInfo->forVoiceRecognition) {
        setRemoteControlMicEnabled(true);
    }

    mPcmConfig.rate = chosenSampleRate;

    mPcmConfig.period_size =
            AudioHardwareInput::kPeriodMsec * mPcmConfig.rate / 1000;
    mPcmConfig.period_count = kPeriodCount;
    mPcmConfig.format = PCM_FORMAT_S16_LE;

    ALOGD("AudioStreamIn::startInputStream_l, call pcm_open()");
    struct pcm* pcm = pcm_open(deviceInfo->pcmCard, deviceInfo->pcmDevice,
                               PCM_IN, &mPcmConfig);

    if (!pcm_is_ready(pcm)) {
        ALOGE("ERROR AudioStreamIn::startInputStream_l, pcm_open failed");
        pcm_close(pcm);
        if (deviceInfo->forVoiceRecognition) {
            setRemoteControlMicEnabled(false);
        }
        return NO_MEMORY;
    }

    mCurrentDeviceInfo = deviceInfo;

    mBufferSize = pcm_frames_to_bytes(pcm, mPcmConfig.period_size);
    if (mBuffer) {
        delete [] mBuffer;
    }
    mBuffer = new int16_t[mBufferSize / sizeof(uint16_t)];

    if (mResampler) {
        release_resampler(mResampler);
        mResampler = NULL;
    }
    if (mPcmConfig.rate != mRequestedSampleRate) {
        ALOGD("AudioStreamIn::startInputStream_l, call create_resampler( %d  to %d)",
            mPcmConfig.rate, mRequestedSampleRate);
        int ret = create_resampler(mPcmConfig.rate,
                                   mRequestedSampleRate,
                                   1,
                                   RESAMPLER_QUALITY_DEFAULT,
                                   &mResamplerProviderWrapper.provider,
                                   &mResampler);
        if (ret != 0) {
            ALOGW("AudioStreamIn: unable to create resampler");
            pcm_close(pcm);
            return static_cast<status_t>(ret);
        }
    }

    mPcm = pcm;

    return NO_ERROR;
}

// readFrames() reads frames from kernel driver, down samples to the capture
// rate if necessary and outputs the number of frames requested to the buffer
// specified
ssize_t AudioStreamIn::readFrames_l(void* buffer, ssize_t frames)
{
    ssize_t framesWr = 0;
    size_t frameSize = getFrameSize();

    while (framesWr < frames) {
        size_t framesRd = frames - framesWr;
        if (mResampler) {
            char* outFrame = static_cast<char*>(buffer) +
                    (framesWr * frameSize);
            mResampler->resample_from_provider(
                mResampler,
                reinterpret_cast<int16_t*>(outFrame),
                &framesRd);
        } else {
            struct resampler_buffer buf;
            buf.raw = NULL;
            buf.frame_count = framesRd;

            getNextBuffer(&buf);
            if (buf.raw != NULL) {
                memcpy(static_cast<char*>(buffer) + (framesWr * frameSize),
                       buf.raw,
                       buf.frame_count * frameSize);
                framesRd = buf.frame_count;
            }
            releaseBuffer(&buf);
        }
        // mReadStatus is updated by getNextBuffer(), which is called by the
        // resampler
        if (mReadStatus != 0)
            return mReadStatus;

        framesWr += framesRd;
    }
    return framesWr;
}

int AudioStreamIn::getNextBufferThunk(
        struct resampler_buffer_provider* bufferProvider,
        struct resampler_buffer* buffer)
{
    ResamplerBufferProviderWrapper* wrapper =
            reinterpret_cast<ResamplerBufferProviderWrapper*>(
                reinterpret_cast<char*>(bufferProvider) -
                offsetof(ResamplerBufferProviderWrapper, provider));

    return wrapper->thiz->getNextBuffer(buffer);
}

void AudioStreamIn::releaseBufferThunk(
        struct resampler_buffer_provider* bufferProvider,
        struct resampler_buffer* buffer)
{
    ResamplerBufferProviderWrapper* wrapper =
            reinterpret_cast<ResamplerBufferProviderWrapper*>(
                reinterpret_cast<char*>(bufferProvider) -
                offsetof(ResamplerBufferProviderWrapper, provider));

    wrapper->thiz->releaseBuffer(buffer);
}

// called while holding mLock
int AudioStreamIn::getNextBuffer(struct resampler_buffer* buffer)
{
    if (buffer == NULL) {
        return -EINVAL;
    }

    if (mPcm == NULL) {
        buffer->raw = NULL;
        buffer->frame_count = 0;
        mReadStatus = -ENODEV;
        return -ENODEV;
    }

    if (mFramesIn == 0) {
        mReadStatus = pcm_read(mPcm, mBuffer, mBufferSize);
        if (mReadStatus) {
            ALOGE("get_next_buffer() pcm_read error %d", mReadStatus);
            buffer->raw = NULL;
            buffer->frame_count = 0;
            return mReadStatus;
        }

        mFramesIn = mPcmConfig.period_size;
        if (mPcmConfig.channels == 2) {
            // Discard the right channel.
            // TODO: this is what other HALs are doing to handle stereo input
            // devices.  Need to verify if this is appropriate for ATV Remote.
            for (unsigned int i = 1; i < mFramesIn; i++) {
                mBuffer[i] = mBuffer[i * 2];
            }
        }
    }

    buffer->frame_count = (buffer->frame_count > mFramesIn) ?
            mFramesIn : buffer->frame_count;
    buffer->i16 = mBuffer + (mPcmConfig.period_size - mFramesIn);

    return mReadStatus;
}

// called while holding mLock
void AudioStreamIn::releaseBuffer(struct resampler_buffer* buffer)
{
    if (buffer == NULL) {
        return;
    }

    mFramesIn -= buffer->frame_count;
}

}; // namespace android
