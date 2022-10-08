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

#ifndef ANDROID_AUDIO_OUTPUT_H
#define ANDROID_AUDIO_OUTPUT_H

#include <semaphore.h>
#include <tinyalsa/asoundlib.h>
#include <utils/LinearTransform.h>
#include <utils/String16.h>
#include <utils/String8.h>
#include <utils/threads.h>
#include <utils/Vector.h>

namespace android {

class AudioStreamOut;

class AudioOutput : public RefBase {
  public:

    // Audio ouput state machine states.
    enum State {
        // Ouput not yet started or synchronized.
        OUT_OF_SYNC,

        // Silence primed to output to start DMA.
        PRIMED,

        // DMA started, ready to align to other inputs.
        DMA_START,

        // DMA active.
        ACTIVE,

        // Fatal, unrecoverable error.
        FATAL,
    };

                        AudioOutput(const char* alsa_name,
                                    enum pcm_format alsa_pcm_format);
    virtual            ~AudioOutput();

    virtual status_t    initCheck();
    virtual status_t    setupForStream(const AudioStreamOut& stream) = 0;

    // State machine transition functions.
    State               getState() { return mState; };
    bool                hasFatalError() { return mState == FATAL; }

    // Prime data to output device, go to PRIMED state.
    void                primeOutput(bool hasActiveOutputs);

    // Adjust for write timestamp difference, go to ACTIVE state.
    void                adjustDelay(int32_t nFrames);

    // Send one chunk of data to ALSA, if state machine permits. This is called
    // for every chunk sent down, regardless of the state of the output.
    void                processOneChunk(const uint8_t* data, size_t len,
                                        bool hasActiveOutputs);

    status_t            getNextWriteTimestamp(int64_t* timestamp,
                                              bool* discon);
    bool                getLastNextWriteTSValid() const;
    int64_t             getLastNextWriteTS() const;

    uint32_t            getExternalDelay_uSec() const;
    void                setExternalDelay_uSec(uint32_t delay);
    void                setDelayComp_uSec(uint32_t delay_usec);

    void                setVolume(float vol);
    void                setMute(bool mute);
    void                setOutputIsFixed(bool fixed);
    void                setFixedOutputLevel(float level);

    float               getVolume()           const { return mVolume; }
    bool                getMute()             const { return mMute; }
    bool                getOutputIsFixed()    const { return mOutputFixed; }
    float               getFixedOutputLevel() const { return mFixedLvl; }

    int                 getHardwareTimestamp(unsigned int *pAvail,
                                struct timespec *pTimestamp);
    uint32_t            getKernelBufferSize() { return mFramesPerChunk * mBufferChunks; }

    virtual void        dump(String8& result) = 0;

    virtual const char* getOutputName() = 0;
    virtual uint32_t    devMask() const = 0;

    virtual void        cleanupResources();

    static const uint32_t kMaxDelayCompensationMSec;
    static const uint32_t kPrimeTimeoutChunks;

  protected:

    void                pushSilence(uint32_t nFrames);
    // Take nBytes of chunkData, convert to output format and write at
    // sbuf. sbuf WILL point to enough space to convert from 16 to 32 bit
    // if needed.
    virtual void        stageChunk(const uint8_t* chunkData,
                                   uint8_t* sbuf,
                                   uint32_t inBytesPerSample,
                                   uint32_t nSamples);
    virtual void        openPCMDevice();
    virtual void        reset();
    virtual status_t    getDMAStartData(int64_t* dma_start_time,
                                        int64_t* frames_queued_to_driver);
    void                doPCMWrite(const uint8_t* data, size_t len);
    void                setupInternal();

    // Current state machine state.
    State               mState;

    // Output format
    uint32_t            mFramesPerChunk;
    uint32_t            mFramesPerSec;
    uint32_t            mBufferChunks;
    uint32_t            mChannelCnt;
    const char*         mALSAName;
    enum pcm_format     mALSAFormat;

    // These numbers are relative to the ALSA output.
    uint32_t            mBytesPerSample;
    uint32_t            mBytesPerFrame;
    uint32_t            mBytesPerChunk;
    uint8_t*            mStagingBuf;

    // Get next write time stuff.
    bool                mLastNextWriteTimeValid;
    int64_t             mLastNextWriteTime;
    int64_t             mLastDMAStartTime;

    // External delay compensation.
    uint32_t            mMaxDelayCompFrames;
    uint32_t            mExternalDelayUSec;
    uint32_t            mExternalDelayLocalTicks;

    LinearTransform     mFramesToLocalTime;

    // ALSA device stuff.
    Mutex               mDeviceLock;
    struct pcm*         mDevice;
    int                 mDeviceExtFd;
    int                 mALSACardID;
    uint64_t            mFramesQueuedToDriver;
    uint32_t            mPrimeTimeoutChunks;

    // Volume stuff
    Mutex               mVolumeLock;
    float               mVolume;
    float               mFixedLvl;
    bool                mMute;
    bool                mOutputFixed;
    bool                mVolParamsDirty;
    virtual void        applyPendingVolParams() = 0;
};

typedef Vector< sp<AudioOutput> > AudioOutputList;

}  // namespace android
#endif  // ANDROID_AUDIO_OUTPUT_H
