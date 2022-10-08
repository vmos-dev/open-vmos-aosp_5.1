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

#define LOG_TAG "AudioHAL:HDMIAudioOutput"

#include <utils/Log.h>

#include <stdint.h>

#include "AudioHardwareOutput.h"
#include "AudioStreamOut.h"
#include "HDMIAudioOutput.h"

namespace android {

extern AudioHardwareOutput gAudioHardwareOutput;

HDMIAudioOutput::HDMIAudioOutput()
    : AudioOutput(kHDMI_ALSADeviceName, PCM_FORMAT_S24_LE)
{
}

HDMIAudioOutput::~HDMIAudioOutput()
{
}

status_t HDMIAudioOutput::setupForStream(const AudioStreamOut& stream)
{
    mFramesPerChunk = stream.framesPerChunk();
    mFramesPerSec = stream.outputSampleRate();
    mBufferChunks = stream.nomChunksInFlight();
    mChannelCnt = audio_channel_count_from_out_mask(stream.chanMask());

    ALOGI("setupForStream format %08x, rate = %u", stream.format(), mFramesPerSec);

    if (!gAudioHardwareOutput.getHDMIAudioCaps().supportsFormat(
            stream.format(),
            stream.sampleRate(),
            mChannelCnt)) {
        ALOGE("HDMI Sink does not support format = 0x%0X, srate = %d, #channels = 0%d",
                stream.format(), mFramesPerSec, mChannelCnt);
        return BAD_VALUE;
    }

    if (stream.isEncoded()) {
        ALOGI("HDMIAudioOutput::setupForStream() use %d channels for playing encoded data!",
            SPDIF_ENCODED_CHANNEL_COUNT);
        mChannelCnt = SPDIF_ENCODED_CHANNEL_COUNT;
    }

    setupInternal();

    // TODO Maybe set SPDIF channel status to compressed mode.
    // Some receivers do not need the bit set. But some might.

    return initCheck();
}

#define IEC958_AES0_NONAUDIO      (1<<1)   /* 0 = audio, 1 = non-audio */

void HDMIAudioOutput::applyPendingVolParams()
{
}

void HDMIAudioOutput::dump(String8& result)
{
    const size_t SIZE = 256;
    char buffer[SIZE];

    snprintf(buffer, SIZE,
            "\t%s Audio Output\n"
            "\t\tSample Rate       : %d\n"
            "\t\tChannel Count     : %d\n"
            "\t\tState             : %d\n",
            getOutputName(),
            mFramesPerSec,
            mChannelCnt,
            mState);
    result.append(buffer);
}

} // namespace android
