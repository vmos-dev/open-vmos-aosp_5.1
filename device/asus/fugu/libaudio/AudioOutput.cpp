/*
**
** Copyright 2011, The Android Open Source Project
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

#define LOG_TAG "AudioHAL:AudioOutput"

#include <utils/Log.h>

#include <assert.h>
#include <limits.h>
#include <semaphore.h>
#include <sys/ioctl.h>

#include <common_time/local_clock.h>

#define __DO_FUNCTION_IMPL__
#include "alsa_utils.h"
#undef __DO_FUNCTION_IMPL__
#include "AudioOutput.h"

namespace android {

const uint32_t AudioOutput::kMaxDelayCompensationMSec = 300;
const uint32_t AudioOutput::kPrimeTimeoutChunks = 10; // 100ms

AudioOutput::AudioOutput(const char* alsa_name,
                         enum pcm_format alsa_pcm_format)
        : mState(OUT_OF_SYNC)
        , mFramesPerChunk(0)
        , mFramesPerSec(0)
        , mBufferChunks(0)
        , mChannelCnt(0)
        , mALSAName(alsa_name)
        , mALSAFormat(alsa_pcm_format)
        , mBytesPerFrame(0)
        , mBytesPerChunk(0)
        , mStagingBuf(NULL)
        , mPrimeTimeoutChunks(0)
        , mVolume(0.0)
        , mFixedLvl(0.0)
        , mMute(false)
        , mOutputFixed(false)
        , mVolParamsDirty(true)
{
    mLastNextWriteTimeValid = false;

    mMaxDelayCompFrames = 0;
    mExternalDelayUSec = 0;

    mDevice = NULL;
    mDeviceExtFd = -1;
    mALSACardID = -1;
    mFramesQueuedToDriver = 0;
}

AudioOutput::~AudioOutput() {
    cleanupResources();
    delete[] mStagingBuf;
}

status_t AudioOutput::initCheck() {
    if (!mDevice) {
        ALOGE("Unable to open PCM device for %s output.", getOutputName());
        return NO_INIT;
    }
    if (!pcm_is_ready(mDevice)) {
        ALOGE("PCM device %s is not ready.", getOutputName());
        ALOGE("PCM error: %s", pcm_get_error(mDevice));
        return NO_INIT;
    }

    return OK;
}

void AudioOutput::setupInternal() {
    LocalClock lc;

    mMaxDelayCompFrames = kMaxDelayCompensationMSec * mFramesPerSec / 1000;

#if 0
    mBytesPerSample = ((mALSAFormat == PCM_FORMAT_S32_LE) ? 4 : 2);
#else
    switch (mALSAFormat) {
    case PCM_FORMAT_S16_LE:
        mBytesPerSample = 2;
        break;
    case PCM_FORMAT_S24_LE:
        mBytesPerSample = 3;
        break;
    case PCM_FORMAT_S32_LE:
        mBytesPerSample = 4;
        break;
    default:
        ALOGE("Unexpected alsa format 0x%x, setting mBytesPerSample to 3", mALSAFormat);
        mBytesPerSample = 3;
        break;
    }
#endif
    mBytesPerFrame = mBytesPerSample * mChannelCnt;
    mBytesPerChunk = mBytesPerFrame * mFramesPerChunk;
    mStagingBuf = new uint8_t[mBytesPerChunk];

    memset(&mFramesToLocalTime, 0, sizeof(mFramesToLocalTime));
    mFramesToLocalTime.a_to_b_numer = lc.getLocalFreq();
    mFramesToLocalTime.a_to_b_denom = mFramesPerSec ? mFramesPerSec : 1;
    LinearTransform::reduce(
            &mFramesToLocalTime.a_to_b_numer,
            &mFramesToLocalTime.a_to_b_denom);

    openPCMDevice();
}

void AudioOutput::primeOutput(bool hasActiveOutputs) {
    ALOGI("primeOutput %s", getOutputName());

    if (hasFatalError())
        return;

    // See comments in AudioStreamOut::write for the reasons behind the
    // different priming levels.
    uint32_t primeAmt = mFramesPerChunk * mBufferChunks;
    if (hasActiveOutputs)
        primeAmt /= 2;

    pushSilence(primeAmt);
    mPrimeTimeoutChunks = 0;
    mState = PRIMED;
}

void AudioOutput::adjustDelay(int32_t nFrames) {
    if (hasFatalError())
        return;

    if (nFrames >= 0) {
        ALOGI("adjustDelay %s %d", getOutputName(), nFrames);
        pushSilence(nFrames);
        mState = ACTIVE;
    } else {
        ALOGW("adjustDelay %s %d, ignoring negative adjustment",
              getOutputName(), nFrames);
    }
}

void AudioOutput::pushSilence(uint32_t nFrames)
{
    if (hasFatalError())
        return;

    uint8_t sbuf[mBytesPerChunk];
    uint32_t primeAmount = mBytesPerFrame*nFrames;
    uint32_t zeroAmount = primeAmount < sizeof(sbuf)
                        ? primeAmount
                        : sizeof(sbuf);

    // Dispatch full buffer at a time if possible.
    memset(sbuf, 0, zeroAmount);
    while (primeAmount && !hasFatalError()) {
        uint32_t amt = (primeAmount < mBytesPerChunk) ?
                        primeAmount : mBytesPerChunk;
        doPCMWrite(sbuf, amt);
        primeAmount -= amt;
    }

    mFramesQueuedToDriver += nFrames;
}

void AudioOutput::stageChunk(const uint8_t* chunkData,
                             uint8_t* sbuf,
                             uint32_t inBytesPerSample,
                             uint32_t nSamples)
{
    memcpy(sbuf, chunkData, inBytesPerSample * nSamples);
}

void AudioOutput::cleanupResources() {

    Mutex::Autolock _l(mDeviceLock);

    if (NULL != mDevice)
        pcm_close(mDevice);

    mDevice = NULL;
    mDeviceExtFd = -1;
    mALSACardID = -1;
}

void AudioOutput::openPCMDevice() {

    Mutex::Autolock _l(mDeviceLock);
    if (NULL == mDevice) {
        struct pcm_config config;
        int dev_id = 0;
        int retry = 0;
        static const int MAX_RETRY_COUNT = 3;

        mALSACardID = find_alsa_card_by_name(mALSAName);
        if (mALSACardID < 0)
            return;

        memset(&config, 0, sizeof(config));
        config.channels        = mChannelCnt;
        config.rate            = mFramesPerSec;
        config.period_size     = mFramesPerChunk;
        config.period_count    = mBufferChunks;
        config.format          = mALSAFormat;
        // start_threshold is in audio frames. The default behavior
        // is to fill period_size*period_count frames before outputing
        // audio. Setting to 1 will start the DMA immediately. Our first
        // write is a full chunk, so we have 10ms to get back with the next
        // chunk before we underflow. This number could be increased if
        // problems arise.
        config.start_threshold = 1;

        ALOGI("calling pcm_open() for output, mALSACardID = %d, dev_id %d, rate = %u, "
            "%d channels, framesPerChunk = %d, alsaFormat = %d",
              mALSACardID, dev_id, config.rate, config.channels, config.period_size, config.format);
        while (1) {
            // Use PCM_MONOTONIC clock for get_presentation_position.
            mDevice = pcm_open(mALSACardID, dev_id,
                    PCM_OUT | PCM_NORESTART | PCM_MONOTONIC, &config);
            if (initCheck() == OK)
                break;
            if (retry++ >= MAX_RETRY_COUNT) {
                ALOGI("out of retries, giving up");
                break;
            }
            /* try again after a delay.  on hotplug, there appears to
             * be a race where the pcm device node isn't available on
             * first open try.
             */
            pcm_close(mDevice);
            mDevice = NULL;
            sleep(1);
            ALOGI("retrying pcm_open() after delay");
        }
        mDeviceExtFd = mDevice
                        ? *(reinterpret_cast<int*>(mDevice))
                        : -1;
        mState = OUT_OF_SYNC;
    }
}

status_t AudioOutput::getNextWriteTimestamp(int64_t* timestamp,
                                            bool* discon) {
    int64_t  dma_start_time;
    int64_t  frames_queued_to_driver;
    status_t ret;

    *discon = false;
    if (hasFatalError())
        return UNKNOWN_ERROR;

    ret = getDMAStartData(&dma_start_time,
                          &frames_queued_to_driver);
    if (OK != ret) {
        if (mLastNextWriteTimeValid) {
            if (!hasFatalError())
                ALOGE("Underflow detected for output \"%s\"", getOutputName());
            *discon = true;
        }

        goto bailout;
    }

    if (mLastNextWriteTimeValid && (mLastDMAStartTime != dma_start_time)) {
        *discon = true;
        ret = UNKNOWN_ERROR;

        ALOGE("Discontinuous DMA start time detected for output \"%s\"."
              "DMA start time is %lld, but last DMA start time was %lld.",
              getOutputName(), dma_start_time, mLastDMAStartTime);

        goto bailout;
    }

    mLastDMAStartTime = dma_start_time;

    mFramesToLocalTime.a_zero = 0;
    mFramesToLocalTime.b_zero = dma_start_time;

    if (!mFramesToLocalTime.doForwardTransform(frames_queued_to_driver,
                                               timestamp)) {
        ALOGE("Overflow when attempting to compute next write time for output"
              " \"%s\".  Frames Queued To Driver = %lld, DMA Start Time = %lld",
              getOutputName(), frames_queued_to_driver, dma_start_time);
        ret = UNKNOWN_ERROR;
        goto bailout;
    }

    mLastNextWriteTime = *timestamp;
    mLastNextWriteTimeValid = true;

    // If we have a valuid timestamp, DMA has started so advance the state.
    if (mState == PRIMED)
        mState = DMA_START;

    return OK;

bailout:
    mLastNextWriteTimeValid = false;
    // If we underflow, reset this output now.
    if (mState > PRIMED) {
        reset();
    }

    return ret;
}

bool AudioOutput::getLastNextWriteTSValid() const {
    return mLastNextWriteTimeValid;
}

int64_t AudioOutput::getLastNextWriteTS() const {
    return mLastNextWriteTime;
}

uint32_t AudioOutput::getExternalDelay_uSec() const {
    return mExternalDelayUSec;
}

void AudioOutput::setExternalDelay_uSec(uint32_t delay_usec) {
    mExternalDelayUSec = delay_usec;
}

void AudioOutput::reset() {
    if (hasFatalError())
        return;

    // Flush the driver level.
    cleanupResources();
    openPCMDevice();
    mFramesQueuedToDriver = 0;
    mLastNextWriteTimeValid = false;

    if (OK == initCheck()) {
        ALOGE("Reset %s", mALSAName);
    } else {
        ALOGE("Reset for %s failed, device is a zombie pending cleanup.", mALSAName);
        cleanupResources();
        mState = FATAL;
    }
}

status_t AudioOutput::getDMAStartData(
        int64_t* dma_start_time,
        int64_t* frames_queued_to_driver) {
    int ret;
#if 1 /* not implemented in driver yet, just fake it */
    *dma_start_time = mLastDMAStartTime;
    ret = 0;
#endif

    // If the get start time ioctl fails with an error of EBADFD, then our
    // underlying audio device is in the DISCONNECTED state.  The only reason
    // this should happen is that HDMI was unplugged while we were running, and
    // the audio driver needed to immediately shut down the driver without
    // involving the application level.  We should enter the fatal state, and
    // wait until the app level catches up to our view of the world (at which
    // point in time we will go through a plug/unplug cycle which should clean
    // things up).
    if (ret < 0) {
        if (EBADFD == errno) {
            ALOGI("Failed to ioctl to %s, output is probably disconnected."
                  " Going into zombie state to await cleanup.", mALSAName);
            cleanupResources();
            mState = FATAL;
        }

        return UNKNOWN_ERROR;
    }

    *frames_queued_to_driver = mFramesQueuedToDriver;
    return OK;
}

void AudioOutput::processOneChunk(const uint8_t* data, size_t len,
                                  bool hasActiveOutputs) {
    switch (mState) {
    case OUT_OF_SYNC:
        primeOutput(hasActiveOutputs);
        break;
    case PRIMED:
        if (mPrimeTimeoutChunks < kPrimeTimeoutChunks)
            mPrimeTimeoutChunks++;
        else
            // Uh-oh, DMA didn't start. Reset and try again.
            reset();

        break;
    case DMA_START:
        // Don't push data when primed and waiting for buffer alignment.
        // We need to align the ALSA buffers first.
        break;
    case ACTIVE:
        doPCMWrite(data, len);
        mFramesQueuedToDriver += len / mBytesPerFrame;
        break;
    default:
        // Do nothing.
        break;
    }

}

static int convert_16PCM_to_24PCM(const void* input, void *output, int ipbytes)
{
    int i = 0,outbytes = 0;
    const int *src = (const int*)input;
    int *dst = (int*)output;

    ALOGV("convert 16 to 24 bits for %d",ipbytes);
    /*convert 16 bit input to 24 bit output
       in a 32 bit sample*/
    if(0 == ipbytes)
        return outbytes;

    for(i = 0; i < (ipbytes/4); i++){
        int x = (int)((int*)src)[i];
        dst[i*2] = ((int)( x & 0x0000FFFF)) << 8;
        // trying to sign extend
        dst[i*2] = dst[i*2] << 8;
        dst[i*2] = dst[i*2] >> 8;
        //shift to middle
        dst[i*2 + 1] = (int)(( x & 0xFFFF0000) >> 8);
        dst[i*2 + 1] = dst[i*2 + 1] << 8;
        dst[i*2 + 1] = dst[i*2 + 1] >> 8;
    }
    outbytes= ipbytes * 2;
    return outbytes;
}

void AudioOutput::doPCMWrite(const uint8_t* data, size_t len) {
    if (hasFatalError())
        return;

    // If write fails with an error of EBADFD, then our underlying audio
    // device is in a pretty bad state.  This common cause of this is
    // that HDMI was unplugged while we were running, and the audio
    // driver needed to immediately shut down the driver without
    // involving the application level.  When this happens, the HDMI
    // audio device is put into the DISCONNECTED state, and calls to
    // write will return EBADFD.
#if 1
    /* Intel HDMI appears to be locked at 24bit PCM, but Android
     * only supports 16 or 32bit, so we have to convert to 24-bit
     * over 32 bit data type.
     */
    int32_t *dstbuff = (int32_t*)malloc(len * 2);
    if (!dstbuff) {
        ALOGE("%s: memory allocation for conversion buffer failed", __func__);
        return;
    }
    memset(dstbuff, 0, len*2);
    len = convert_16PCM_to_24PCM(data, dstbuff, len);
    int err = pcm_write(mDevice, dstbuff, len);
    free(dstbuff);
#else

    int err = pcm_write(mDevice, data, len);
#endif
    if ((err < 0) && (EBADFD == errno)) {
        ALOGI("Failed to write to %s, output is probably disconnected."
              " Going into zombie state to await cleanup.", mALSAName);
        cleanupResources();
        mState = FATAL;
    }
    else if (err < 0) {
        ALOGW("pcm_write failed err %d", err);
    }

#if 1 /* not implemented in driver yet, just fake it */
    else {
        LocalClock lc;
        mLastDMAStartTime = lc.getLocalTime();
    }
#endif
}

void AudioOutput::setVolume(float vol) {
    Mutex::Autolock _l(mVolumeLock);
    if (mVolume != vol) {
        mVolume = vol;
        mVolParamsDirty = true;
    }
}

void AudioOutput::setMute(bool mute) {
    Mutex::Autolock _l(mVolumeLock);
    if (mMute != mute) {
        mMute = mute;
        mVolParamsDirty = true;
    }
}

void AudioOutput::setOutputIsFixed(bool fixed) {
    Mutex::Autolock _l(mVolumeLock);
    if (mOutputFixed != fixed) {
        mOutputFixed = fixed;
        mVolParamsDirty = true;
    }
}

void AudioOutput::setFixedOutputLevel(float level) {
    Mutex::Autolock _l(mVolumeLock);
    if (mFixedLvl != level) {
        mFixedLvl = level;
        mVolParamsDirty = true;
    }
}

int  AudioOutput::getHardwareTimestamp(size_t *pAvail,
                            struct timespec *pTimestamp)
{
    Mutex::Autolock _l(mDeviceLock);
    if (!mDevice) {
       ALOGW("pcm device unavailable - reinitialize  timestamp");
       return -1;
    }
    return pcm_get_htimestamp(mDevice, pAvail, pTimestamp);
}

}  // namespace android
