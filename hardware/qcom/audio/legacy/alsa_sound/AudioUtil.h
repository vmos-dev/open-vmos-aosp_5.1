/* AudioUtil.h
 *
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ALSA_SOUND_AUDIO_UTIL_H
#define ALSA_SOUND_AUDIO_UTIL_H

#define BIT(nr)     (1UL << (nr))
#define MAX_EDID_BLOCKS 10
#define MAX_SHORT_AUDIO_DESC_CNT        30
#define MIN_AUDIO_DESC_LENGTH           3
#define MIN_SPKR_ALLOCATION_DATA_LENGTH 3

typedef enum EDID_AUDIO_FORMAT_ID {
    LPCM = 1,
    AC3,
    MPEG1,
    MP3,
    MPEG2_MULTI_CHANNEL,
    AAC,
    DTS,
    ATRAC,
    SACD,
    DOLBY_DIGITAL_PLUS,
    DTS_HD,
    MAT,
    DST,
    WMA_PRO
} EDID_AUDIO_FORMAT_ID;

typedef struct EDID_AUDIO_BLOCK_INFO {
    EDID_AUDIO_FORMAT_ID nFormatId;
    int nSamplingFreq;
    int nBitsPerSample;
    int nChannels;
} EDID_AUDIO_BLOCK_INFO;

typedef struct EDID_AUDIO_INFO {
    int nAudioBlocks;
    unsigned char nSpeakerAllocation[MIN_SPKR_ALLOCATION_DATA_LENGTH];
    EDID_AUDIO_BLOCK_INFO AudioBlocksArray[MAX_EDID_BLOCKS];
} EDID_AUDIO_INFO;

class AudioUtil {
public:

    //Parses EDID audio block when if HDMI is connected to determine audio sink capabilities.
    static bool getHDMIAudioSinkCaps(EDID_AUDIO_INFO*);

private:
    static int printFormatFromEDID(unsigned char format);
    static int getSamplingFrequencyFromEDID(unsigned char byte);
    static int getBitsPerSampleFromEDID(unsigned char byte,
        unsigned char format);
    static bool getSpeakerAllocation(EDID_AUDIO_INFO* pInfo);
};

#endif /* ALSA_SOUND_AUDIO_UTIL_H */
