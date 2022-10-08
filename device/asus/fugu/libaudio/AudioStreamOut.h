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

#ifndef ANDROID_AUDIO_STREAM_OUT_H
#define ANDROID_AUDIO_STREAM_OUT_H

#include <stdint.h>
#include <sys/types.h>

#include <common_time/local_clock.h>
#include <hardware/audio.h>
#include <media/AudioParameter.h>
#include <audio_utils/spdif/SPDIFEncoder.h>

#include "AudioOutput.h"

namespace android {

class AudioHardwareOutput;

class AudioStreamOut {
  public:
    AudioStreamOut(AudioHardwareOutput& owner, bool mcOut);
    ~AudioStreamOut();

    uint32_t            latency() const;
    status_t            getRenderPosition(uint32_t *dspFrames);
    status_t            getPresentationPosition(uint64_t *frames, struct timespec *timestamp);
    status_t            getNextWriteTimestamp(int64_t *timestamp);
    status_t            standby();
    status_t            dump(int fd);

    uint32_t            sampleRate()        const { return mInputSampleRate; }
    uint32_t            outputSampleRate()  const;
    uint32_t            getRateMultiplier() const;

    size_t              bufferSize()        const { return mInputBufSize; }
    uint32_t            chanMask()          const { return mInputChanMask; }
    audio_format_t      format()            const { return mInputFormat; }
    uint32_t            framesPerChunk()    const { return mInputChunkFrames; }
    uint32_t            nomChunksInFlight() const { return mInputNominalChunksInFlight; }

    status_t            set(audio_format_t *pFormat,
                            uint32_t       *pChannels,
                            uint32_t       *pRate);
    void                setTgtDevices(uint32_t tgtDevices);

    status_t            setParameters(struct audio_stream *stream,
                                      const char *kvpairs);
    char*               getParameters(const char* keys);
    const char*         getName() const { return mIsMCOutput ? "Multi-channel"
                                                             : "Main"; }

    ssize_t             write(const void* buffer, size_t bytes);

    bool                isEncoded() const { return mIsEncoded; }

    class MySPDIFEncoder : public SPDIFEncoder
    {
    public:
        MySPDIFEncoder(AudioStreamOut *streamOut)
          : mStreamOut(streamOut)
        {};

        virtual ssize_t writeOutput(const void* buffer, size_t bytes)
        {
            return mStreamOut->writeInternal(buffer, bytes);
        }
    protected:
        AudioStreamOut * const mStreamOut;
    };

protected:
    Mutex           mLock;
    Mutex           mRoutingLock;

    // Used to implment get_presentation_position()
    int64_t         mFramesPresented; // application rate frames, not device rate frames
    // Used to implement get_render_position()
    int64_t         mFramesRendered; // application rate frames, not device rate frames
    uint32_t        mFramesWrittenRemainder; // needed when device rate > app rate

    // Our HAL, used as the middle-man to collect and trade AudioOutputs.
    AudioHardwareOutput&  mOwnerHAL;

    // Details about the format of the audio we have been configured to receive
    // from audio flinger.
    uint32_t        mInputSampleRate;
    uint32_t        mInputChanMask;
    audio_format_t  mInputFormat;
    uint32_t        mInputNominalChunksInFlight;

    // Handy values pre-computed from the audio configuration.
    uint32_t        mInputBufSize;
    uint32_t        mInputChanCount;
    uint32_t        mInputChunkFrames;
    uint32_t        mInputNominalLatencyUSec;
    LinearTransform mLocalTimeToFrames;

    // Bookkeeping used to throttle audio flinger when this audio stream has no
    // actual physical outputs.
    LocalClock      mLocalClock;
    bool            mThrottleValid;
    int64_t         mWriteStartLT;
    int64_t         mFramesWritten; // application rate frames, not device rate frames
    LinearTransform mUSecToLocalTime;

    // State to track which actual outputs are assigned to this output stream.
    AudioOutputList mPhysOutputs;
    uint32_t        mTgtDevices;
    bool            mTgtDevicesDirty;
    uint32_t        mAudioFlingerTgtDevices;

    // Flag to track if this StreamOut was created to sink a direct output
    // multichannel stream.
    bool            mIsMCOutput;
    // Is the audio data encoded, eg. AC3?
    bool            mIsEncoded;
    // Is the stream on standby?
    bool            mInStandby;

    MySPDIFEncoder  mSPDIFEncoder;

    void            releaseAllOutputs();
    void            updateTargetOutputs();
    void            updateInputNums();
    void            finishedWriteOp(size_t framesWritten, bool needThrottle);
    void            resetThrottle() { mThrottleValid = false; }
    status_t        getNextWriteTimestamp_internal(int64_t *timestamp);
    void            adjustOutputs(int64_t maxTime);
    ssize_t         writeInternal(const void* buffer, size_t bytes);
    int             getBytesPerOutputFrame();
};

}  // android
#endif  // ANDROID_AUDIO_STREAM_OUT_H
