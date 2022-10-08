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

#ifndef ANDROID_AUDIO_FRAME_SCANNER_H
#define ANDROID_AUDIO_FRAME_SCANNER_H

#include <stdint.h>

namespace android {


/**
 * Scan a byte stream looking for the start of an encoded frame.
 * Parse the sample rate and the size of the encoded frame.
 * Buffer the sync header so it can be prepended to the remaining data.
 *
 * This is used directly by the SPDIFEncoder. External clients will
 * generally not call this class.
 */
class FrameScanner {
public:

    FrameScanner(int dataType);
    virtual ~FrameScanner();

    /**
     * Pass each byte of the encoded stream to this scanner.
     * @return true if a complete and valid header was detected
     */
    virtual bool scan(uint8_t) = 0;

    /**
     * @return address of where the sync header was stored by scan()
     */
    virtual const uint8_t *getHeaderAddress() const = 0;

    /**
     * @return number of bytes in sync header stored by scan()
     */
    virtual size_t getHeaderSizeBytes() const = 0;

    /**
     * @return sample rate of the encoded audio
     */
    uint32_t getSampleRate()   const { return mSampleRate; }

    /**
     * Some formats, for example EAC3, are wrapped in data bursts that have
     * a sample rate that is a multiple of the encoded sample rate.
     * The default multiplier is 1.
     * @return sample rate multiplier for the SP/DIF PCM data bursts
     */
    uint32_t getRateMultiplier()   const { return mRateMultiplier; }

    size_t getFrameSizeBytes()     const { return mFrameSizeBytes; }

    /**
     * dataType is defined by the SPDIF standard for each format
     */
    virtual int getDataType()      const { return mDataType; }
    virtual int getDataTypeInfo()  const { return mDataTypeInfo; }

    virtual int getMaxChannels() const = 0;

    virtual void resetBurst() = 0;

    /**
     * @return the number of pcm frames that correspond to one encoded frame
     */
    virtual int getMaxSampleFramesPerSyncFrame() const = 0;
    virtual int getSampleFramesPerSyncFrame()    const = 0;

    /**
     * @return true if this parsed frame must be the first frame in a data burst.
     */
    virtual bool isFirstInBurst() = 0;

    /**
     * If this returns false then the previous frame may or may not be the last frame.
     * @return true if this parsed frame is definitely the last frame in a data burst.
     */
    virtual bool isLastInBurst()  = 0;

protected:
    uint32_t mSampleRate;
    uint32_t mRateMultiplier;
    size_t   mFrameSizeBytes;
    int      mDataType;
    int      mDataTypeInfo;
};

#define AC3_NUM_SAMPLE_RATE_TABLE_ENTRIES          3
#define AC3_NUM_FRAME_SIZE_TABLE_ENTRIES          38
#define AC3_PCM_FRAMES_PER_BLOCK                 256
#define AC3_MAX_BLOCKS_PER_SYNC_FRAME_BLOCK        6
#define EAC3_RATE_MULTIPLIER                       4
#define EAC3_NUM_SAMPLE_RATE_TABLE_ENTRIES         3
#define EAC3_NUM_BLOCKS_PER_FRAME_TABLE_ENTRIES   38
#define EAC3_MAX_SUBSTREAMS                        8

class AC3FrameScanner : public FrameScanner
{
public:
    AC3FrameScanner();
    virtual ~AC3FrameScanner();

    virtual bool scan(uint8_t);

    virtual const uint8_t *getHeaderAddress() const { return mHeaderBuffer; }
    virtual size_t getHeaderSizeBytes() const { return sizeof(mHeaderBuffer); }

    virtual int getDataType()      const { return mDataType; }
    virtual int getDataTypeInfo()  const { return 0; }
    virtual int getMaxChannels()   const { return 5 + 1; }

    virtual int getMaxSampleFramesPerSyncFrame() const { return EAC3_RATE_MULTIPLIER
            * AC3_MAX_BLOCKS_PER_SYNC_FRAME_BLOCK * AC3_PCM_FRAMES_PER_BLOCK; }
    virtual int getSampleFramesPerSyncFrame() const;

    virtual bool isFirstInBurst();
    virtual bool isLastInBurst();
    virtual void resetBurst();

protected:

    // Preamble state machine states.
    enum State {
         STATE_EXPECTING_SYNC_1,
         STATE_EXPECTING_SYNC_2,
         STATE_GATHERING,
         STATE_GOT_HEADER,
    };

    State parseHeader(void);

    State    mState;
    uint32_t mBytesSkipped;
    uint8_t  mHeaderBuffer[6];
    uint8_t  mSubstreamBlockCounts[EAC3_MAX_SUBSTREAMS];
    int      mAudioBlocksPerSyncFrame;
    uint     mCursor;
    uint     mStreamType;
    uint     mSubstreamID;
    uint     mFormatDumpCount;

    static const uint8_t kAC3SyncByte1;
    static const uint8_t kAC3SyncByte2;
    static const uint16_t   kAC3SampleRateTable[AC3_NUM_SAMPLE_RATE_TABLE_ENTRIES];
    static const uint16_t kAC3FrameSizeTable[AC3_NUM_FRAME_SIZE_TABLE_ENTRIES]
            [AC3_NUM_SAMPLE_RATE_TABLE_ENTRIES];

    static const uint16_t   kEAC3ReducedSampleRateTable[AC3_NUM_SAMPLE_RATE_TABLE_ENTRIES];
    static const uint16_t kEAC3BlocksPerFrameTable[EAC3_NUM_BLOCKS_PER_FRAME_TABLE_ENTRIES];

};

}  // namespace android
#endif  // ANDROID_AUDIO_FRAME_SCANNER_H
