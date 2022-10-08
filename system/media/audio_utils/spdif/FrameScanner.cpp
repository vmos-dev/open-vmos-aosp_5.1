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

#define LOG_TAG "AudioSPDIF"

#include <utils/Log.h>
#include <audio_utils/spdif/FrameScanner.h>

namespace android {

#define SPDIF_DATA_TYPE_AC3     1
#define SPDIF_DATA_TYPE_E_AC3  21

#define AC3_SYNCWORD_SIZE  2

FrameScanner::FrameScanner(int dataType)
 : mSampleRate(0)
 , mRateMultiplier(1)
 , mFrameSizeBytes(0)
 , mDataType(dataType)
 , mDataTypeInfo(0)
{
}

FrameScanner::~FrameScanner()
{
}

// ------------------- AC3 -----------------------------------------------------
// These values are from the AC3 spec. Do not change them.
const uint8_t AC3FrameScanner::kAC3SyncByte1 = 0x0B;
const uint8_t AC3FrameScanner::kAC3SyncByte2 = 0x77;

const uint16_t AC3FrameScanner::kAC3SampleRateTable[AC3_NUM_SAMPLE_RATE_TABLE_ENTRIES]
    = { 48000, 44100, 32000 };

// Table contains number of 16-bit words in an AC3 frame.
const uint16_t AC3FrameScanner::kAC3FrameSizeTable[AC3_NUM_FRAME_SIZE_TABLE_ENTRIES]
        [AC3_NUM_SAMPLE_RATE_TABLE_ENTRIES] = {
    { 64, 69, 96 },
    { 64, 70, 96 },
    { 80, 87, 120 },
    { 80, 88, 120 },
    { 96, 104, 144 },
    { 96, 105, 144 },
    { 112, 121, 168 },
    { 112, 122, 168 },
    { 128, 139, 192 },
    { 128, 140, 192 },
    { 160, 174, 240 },
    { 160, 175, 240 },
    { 192, 208, 288 },
    { 192, 209, 288 },
    { 224, 243, 336 },
    { 224, 244, 336 },
    { 256, 278, 384 },
    { 256, 279, 384 },
    { 320, 348, 480 },
    { 320, 349, 480 },
    { 384, 417, 576 },
    { 384, 418, 576 },
    { 448, 487, 672 },
    { 448, 488, 672 },
    { 512, 557, 768 },
    { 512, 558, 768 },
    { 640, 696, 960 },
    { 640, 697, 960 },
    { 768, 835, 1152 },
    { 768, 836, 1152 },
    { 896, 975, 1344 },
    { 896, 976, 1344 },
    { 1024, 1114, 1536 },
    { 1024, 1115, 1536 },
    { 1152, 1253, 1728 },
    { 1152, 1254, 1728 },
    { 1280, 1393, 1920 },
    { 1280, 1394, 1920 }
};

const uint16_t AC3FrameScanner::kEAC3ReducedSampleRateTable[AC3_NUM_SAMPLE_RATE_TABLE_ENTRIES]
        = { 24000, 22050, 16000 };

const uint16_t
        AC3FrameScanner::kEAC3BlocksPerFrameTable[EAC3_NUM_BLOCKS_PER_FRAME_TABLE_ENTRIES]
        = { 1, 2, 3, 6 };
// -----------------------------------------------------------------------------

// Scanner for AC3 byte streams.
AC3FrameScanner::AC3FrameScanner()
 : FrameScanner(SPDIF_DATA_TYPE_AC3)
 , mState(STATE_EXPECTING_SYNC_1)
 , mBytesSkipped(0)
 , mAudioBlocksPerSyncFrame(6)
 , mCursor(AC3_SYNCWORD_SIZE) // past sync word
 , mStreamType(0)
 , mSubstreamID(0)
 , mFormatDumpCount(0)
{
    memset(mSubstreamBlockCounts, 0, sizeof(mSubstreamBlockCounts));
    // Define beginning of syncinfo for getSyncAddress()
    mHeaderBuffer[0] = kAC3SyncByte1;
    mHeaderBuffer[1] = kAC3SyncByte2;
}

AC3FrameScanner::~AC3FrameScanner()
{
}

int AC3FrameScanner::getSampleFramesPerSyncFrame() const
{
    return mRateMultiplier * AC3_MAX_BLOCKS_PER_SYNC_FRAME_BLOCK * AC3_PCM_FRAMES_PER_BLOCK;
}

void AC3FrameScanner::resetBurst()
{
    for (int i = 0; i < EAC3_MAX_SUBSTREAMS; i++) {
        if (mSubstreamBlockCounts[i] >= AC3_MAX_BLOCKS_PER_SYNC_FRAME_BLOCK) {
            mSubstreamBlockCounts[i] -= AC3_MAX_BLOCKS_PER_SYNC_FRAME_BLOCK;
        } else if (mSubstreamBlockCounts[i] > 0) {
            ALOGW("EAC3 substream[%d] has only %d audio blocks!",
                i, mSubstreamBlockCounts[i]);
            mSubstreamBlockCounts[i] = 0;
        }
    }
}

// per IEC 61973-3 Paragraph 5.3.3
// We have to send 6 audio blocks on all active substreams.
// Substream zero must be the first.
// We don't know if we have all the blocks we need until we see
// the 7th block of substream#0.
bool AC3FrameScanner::isFirstInBurst()
{
    if (mDataType == SPDIF_DATA_TYPE_E_AC3) {
        if (((mStreamType == 0) || (mStreamType == 2))
            && (mSubstreamID == 0)
            // The ">" is intentional. We have to see the beginning of the block
            // in the next burst before we can send the current burst.
            && (mSubstreamBlockCounts[0] > AC3_MAX_BLOCKS_PER_SYNC_FRAME_BLOCK)) {
            return true;
        }
    }
    return false;
}

bool AC3FrameScanner::isLastInBurst()
{
    // For EAC3 we don't know if we are the end until we see a
    // frame that must be at the beginning. See isFirstInBurst().
    return (mDataType != SPDIF_DATA_TYPE_E_AC3); // Just one AC3 frame per burst.
}

// Parse AC3 header.
// Detect whether the stream is AC3 or EAC3. Extract data depending on type.
// Sets mDataType, mFrameSizeBytes, mAudioBlocksPerSyncFrame,
//      mSampleRate, mRateMultiplier, mLengthCode.
//
// @return next state for scanner
AC3FrameScanner::State AC3FrameScanner::parseHeader()
{
    // Interpret bsid based on paragraph E2.3.1.6 of EAC3 spec.
    int bsid = mHeaderBuffer[5] >> 3; // bitstream ID
    // Check BSID to see if this is EAC3 or regular AC3
    if ((bsid >= 10) && (bsid <= 16)) {
        mDataType = SPDIF_DATA_TYPE_E_AC3;
    } else if ((bsid >= 0) && (bsid <= 8)) {
        mDataType = SPDIF_DATA_TYPE_AC3;
    } else {
        ALOGW("AC3 bsid = %d not supported", bsid);
        return STATE_EXPECTING_SYNC_1;
    }

    // The names fscod, frmsiz are from the AC3 spec.
    int fscod = mHeaderBuffer[4] >> 6;
    if (mDataType == SPDIF_DATA_TYPE_E_AC3) {
        mStreamType = mHeaderBuffer[2] >> 6; // strmtyp in spec
        mSubstreamID = (mHeaderBuffer[2] >> 3) & 0x07;

        // Frame size is explicit in EAC3. Paragraph E2.3.1.3
        int frmsiz = ((mHeaderBuffer[2] & 0x07) << 8) + mHeaderBuffer[3];
        mFrameSizeBytes = (frmsiz + 1) * sizeof(int16_t);

        int numblkscod = 3; // 6 blocks default
        if (fscod == 3) {
            int fscod2 = (mHeaderBuffer[4] >> 4) & 0x03;
            if (fscod2 >= AC3_NUM_SAMPLE_RATE_TABLE_ENTRIES) {
                ALOGW("Invalid EAC3 fscod2 = %d\n", fscod2);
                return STATE_EXPECTING_SYNC_1;
            } else {
                mSampleRate = kEAC3ReducedSampleRateTable[fscod2];
            }
        } else {
            mSampleRate = kAC3SampleRateTable[fscod];
            numblkscod = (mHeaderBuffer[4] >> 4) & 0x03;
        }
        mRateMultiplier = EAC3_RATE_MULTIPLIER; // per IEC 61973-3 Paragraph 5.3.3
        // Don't send data burst until we have 6 blocks per substream.
        mAudioBlocksPerSyncFrame = kEAC3BlocksPerFrameTable[numblkscod];
        // Keep track of how many audio blocks we have for each substream.
        // This should be safe because mSubstreamID is ANDed with 0x07 above.
        // And the array is allocated as [8].
        mSubstreamBlockCounts[mSubstreamID] += mAudioBlocksPerSyncFrame;

        // Print enough so we can see all the substreams.
        ALOGD_IF((mFormatDumpCount < 3*8 ),
                "EAC3 mStreamType = %d, mSubstreamID = %d",
                mStreamType, mSubstreamID);
    } else { // regular AC3
        // Extract sample rate and frame size from codes.
        unsigned int frmsizcod = mHeaderBuffer[4] & 0x3F; // frame size code

        if (fscod >= AC3_NUM_SAMPLE_RATE_TABLE_ENTRIES) {
            ALOGW("Invalid AC3 sampleRateCode = %d\n", fscod);
            return STATE_EXPECTING_SYNC_1;
        } else if (frmsizcod >= AC3_NUM_FRAME_SIZE_TABLE_ENTRIES) {
            ALOGW("Invalid AC3 frameSizeCode = %d\n", frmsizcod);
            return STATE_EXPECTING_SYNC_1;
        } else {
            mSampleRate = kAC3SampleRateTable[fscod];
            mRateMultiplier = 1;
            mFrameSizeBytes = sizeof(uint16_t)
                    * kAC3FrameSizeTable[frmsizcod][fscod];
        }
        mAudioBlocksPerSyncFrame = 6;
    }
    ALOGI_IF((mFormatDumpCount == 0),
            "AC3 frame rate = %d * %d, size = %d, audioBlocksPerSyncFrame = %d\n",
            mSampleRate, mRateMultiplier, mFrameSizeBytes, mAudioBlocksPerSyncFrame);
    mFormatDumpCount++;
    return STATE_GOT_HEADER;
}

// State machine that scans for AC3 headers in a byte stream.
// @return true if we have detected a complete and valid header.
bool AC3FrameScanner::scan(uint8_t byte)
{
    State nextState = mState;
    switch (mState) {
    case STATE_GOT_HEADER:
        nextState = STATE_EXPECTING_SYNC_1;
        // deliberately fall through
    case STATE_EXPECTING_SYNC_1:
        if (byte == kAC3SyncByte1) {
            nextState = STATE_EXPECTING_SYNC_2; // advance
        } else {
            mBytesSkipped += 1; // skip unsynchronized data
        }
        break;

    case STATE_EXPECTING_SYNC_2:
        if (byte == kAC3SyncByte2) {
            if (mBytesSkipped > 0) {
                ALOGI("WARNING AC3 skipped %u bytes looking for a valid header.\n", mBytesSkipped);
                mBytesSkipped = 0;
            }
            mCursor = AC3_SYNCWORD_SIZE;
            nextState = STATE_GATHERING; // advance
        } else if (byte == kAC3SyncByte1) {
            nextState = STATE_EXPECTING_SYNC_2; // resync
        } else {
            nextState = STATE_EXPECTING_SYNC_1; // restart
        }
        break;

    case STATE_GATHERING:
        mHeaderBuffer[mCursor++] = byte; // save for getSyncAddress()
        if (mCursor >= sizeof(mHeaderBuffer)) {
            nextState = parseHeader();
        }
        break;

    default:
        ALOGE("AC3FrameScanner: invalid state = %d\n", mState);
        nextState = STATE_EXPECTING_SYNC_1; // restart
        break;
    }
    mState = nextState;
    return mState == STATE_GOT_HEADER;
}

}  // namespace android
