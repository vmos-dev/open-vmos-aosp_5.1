/* AudioUtil.cpp
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

#define LOG_TAG "AudioUtil"
//#define LOG_NDEBUG 0
#include <utils/Log.h>

#include "AudioUtil.h"

int AudioUtil::printFormatFromEDID(unsigned char format) {
    switch (format) {
    case LPCM:
        ALOGV("Format:LPCM");
        break;
    case AC3:
        ALOGV("Format:AC-3");
        break;
    case MPEG1:
        ALOGV("Format:MPEG1 (Layers 1 & 2)");
        break;
    case MP3:
        ALOGV("Format:MP3 (MPEG1 Layer 3)");
        break;
    case MPEG2_MULTI_CHANNEL:
        ALOGV("Format:MPEG2 (multichannel)");
        break;
    case AAC:
        ALOGV("Format:AAC");
        break;
    case DTS:
        ALOGV("Format:DTS");
        break;
    case ATRAC:
        ALOGV("Format:ATRAC");
        break;
    case SACD:
        ALOGV("Format:One-bit audio aka SACD");
        break;
    case DOLBY_DIGITAL_PLUS:
        ALOGV("Format:Dolby Digital +");
        break;
    case DTS_HD:
        ALOGV("Format:DTS-HD");
        break;
    case MAT:
        ALOGV("Format:MAT (MLP)");
        break;
    case DST:
        ALOGV("Format:DST");
        break;
    case WMA_PRO:
        ALOGV("Format:WMA Pro");
        break;
    default:
        ALOGV("Invalid format ID....");
        break;
    }
    return format;
}

int AudioUtil::getSamplingFrequencyFromEDID(unsigned char byte) {
    int nFreq = 0;

    if (byte & BIT(6)) {
        ALOGV("192kHz");
        nFreq = 192000;
    } else if (byte & BIT(5)) {
        ALOGV("176kHz");
        nFreq = 176000;
    } else if (byte & BIT(4)) {
        ALOGV("96kHz");
        nFreq = 96000;
    } else if (byte & BIT(3)) {
        ALOGV("88.2kHz");
        nFreq = 88200;
    } else if (byte & BIT(2)) {
        ALOGV("48kHz");
        nFreq = 48000;
    } else if (byte & BIT(1)) {
        ALOGV("44.1kHz");
        nFreq = 44100;
    } else if (byte & BIT(0)) {
        ALOGV("32kHz");
        nFreq = 32000;
    }
    return nFreq;
}

int AudioUtil::getBitsPerSampleFromEDID(unsigned char byte,
    unsigned char format) {
    int nBitsPerSample = 0;
    if (format == 1) {
        if (byte & BIT(2)) {
            ALOGV("24bit");
            nBitsPerSample = 24;
        } else if (byte & BIT(1)) {
            ALOGV("20bit");
            nBitsPerSample = 20;
        } else if (byte & BIT(0)) {
            ALOGV("16bit");
            nBitsPerSample = 16;
        }
    } else {
        ALOGV("not lpcm format, return 0");
        return 0;
    }
    return nBitsPerSample;
}

bool AudioUtil::getHDMIAudioSinkCaps(EDID_AUDIO_INFO* pInfo) {
    unsigned char channels[16];
    unsigned char formats[16];
    unsigned char frequency[16];
    unsigned char bitrate[16];
    unsigned char* data = NULL;
    unsigned char* original_data_ptr = NULL;
    int count = 0;
    bool bRet = false;
    const char* file = "/sys/class/graphics/fb1/audio_data_block";
    FILE* fpaudiocaps = fopen(file, "rb");
    if (fpaudiocaps) {
        ALOGV("opened audio_caps successfully...");
        fseek(fpaudiocaps, 0, SEEK_END);
        long size = ftell(fpaudiocaps);
        ALOGV("audiocaps size is %ld\n",size);
        data = (unsigned char*) malloc(size);
        if (data) {
            fseek(fpaudiocaps, 0, SEEK_SET);
            original_data_ptr = data;
            fread(data, 1, size, fpaudiocaps);
        }
        fclose(fpaudiocaps);
    } else {
        ALOGE("failed to open audio_caps");
    }

    if (pInfo && data) {
        int length = 0;
        memcpy(&count,  data, sizeof(int));
        data+= sizeof(int);
        ALOGV("#Audio Block Count is %d",count);
        memcpy(&length, data, sizeof(int));
        data += sizeof(int);
        ALOGV("Total length is %d",length);
        unsigned int sad[MAX_SHORT_AUDIO_DESC_CNT];
        int nblockindex = 0;
        int nCountDesc = 0;
        while (length >= MIN_AUDIO_DESC_LENGTH && count < MAX_SHORT_AUDIO_DESC_CNT) {
            sad[nblockindex] = (unsigned int)data[0] + ((unsigned int)data[1] << 8)
                               + ((unsigned int)data[2] << 16);
            nblockindex+=1;
            nCountDesc++;
            length -= MIN_AUDIO_DESC_LENGTH;
            data += MIN_AUDIO_DESC_LENGTH;
        }
        memset(pInfo, 0, sizeof(EDID_AUDIO_INFO));
        pInfo->nAudioBlocks = nCountDesc;
        ALOGV("Total # of audio descriptors %d",nCountDesc);
        int nIndex = 0;
        while (nCountDesc--) {
              channels [nIndex]   = (sad[nIndex] & 0x7) + 1;
              formats  [nIndex]   = (sad[nIndex] & 0xFF) >> 3;
              frequency[nIndex]   = (sad[nIndex] >> 8) & 0xFF;
              bitrate  [nIndex]   = (sad[nIndex] >> 16) & 0xFF;
              nIndex++;
        }
        bRet = true;
        for (int i = 0; i < pInfo->nAudioBlocks; i++) {
            ALOGV("AUDIO DESC BLOCK # %d\n",i);

            pInfo->AudioBlocksArray[i].nChannels = channels[i];
            ALOGV("pInfo->AudioBlocksArray[i].nChannels %d\n", pInfo->AudioBlocksArray[i].nChannels);

            ALOGV("Format Byte %d\n", formats[i]);
            pInfo->AudioBlocksArray[i].nFormatId = (EDID_AUDIO_FORMAT_ID)printFormatFromEDID(formats[i]);
            ALOGV("pInfo->AudioBlocksArray[i].nFormatId %d",pInfo->AudioBlocksArray[i].nFormatId);

            ALOGV("Frequency Byte %d\n", frequency[i]);
            pInfo->AudioBlocksArray[i].nSamplingFreq = getSamplingFrequencyFromEDID(frequency[i]);
            ALOGV("pInfo->AudioBlocksArray[i].nSamplingFreq %d",pInfo->AudioBlocksArray[i].nSamplingFreq);

            ALOGV("BitsPerSample Byte %d\n", bitrate[i]);
            pInfo->AudioBlocksArray[i].nBitsPerSample = getBitsPerSampleFromEDID(bitrate[i],formats[i]);
            ALOGV("pInfo->AudioBlocksArray[i].nBitsPerSample %d",pInfo->AudioBlocksArray[i].nBitsPerSample);
        }
            getSpeakerAllocation(pInfo);
    }
    if (original_data_ptr)
        free(original_data_ptr);

    return bRet;
}

bool AudioUtil::getSpeakerAllocation(EDID_AUDIO_INFO* pInfo) {
    int count = 0;
    bool bRet = false;
    unsigned char* data = NULL;
    unsigned char* original_data_ptr = NULL;
    const char* spkrfile = "/sys/class/graphics/fb1/spkr_alloc_data_block";
    FILE* fpspkrfile = fopen(spkrfile, "rb");
    if(fpspkrfile) {
        ALOGV("opened spkr_alloc_data_block successfully...");
        fseek(fpspkrfile,0,SEEK_END);
        long size = ftell(fpspkrfile);
        ALOGV("fpspkrfile size is %ld\n",size);
        data = (unsigned char*)malloc(size);
        if(data) {
            original_data_ptr = data;
            fseek(fpspkrfile,0,SEEK_SET);
            fread(data,1,size,fpspkrfile);
        }
        fclose(fpspkrfile);
    } else {
        ALOGE("failed to open fpspkrfile");
    }

    if(pInfo && data) {
        int length = 0;
        memcpy(&count,  data, sizeof(int));
        ALOGV("Count is %d",count);
        data += sizeof(int);
        memcpy(&length, data, sizeof(int));
        ALOGV("Total length is %d",length);
        data+= sizeof(int);
        ALOGV("Total speaker allocation Block count # %d\n",count);
        bRet = true;
        for (int i = 0; i < count; i++) {
            ALOGV("Speaker Allocation BLOCK # %d\n",i);
            pInfo->nSpeakerAllocation[0] = data[0];
            pInfo->nSpeakerAllocation[1] = data[1];
            pInfo->nSpeakerAllocation[2] = data[2];
            ALOGV("pInfo->nSpeakerAllocation %x %x %x\n", data[0],data[1],data[2]);


            if (pInfo->nSpeakerAllocation[0] & BIT(7)) {
                 ALOGV("FLW/FRW");
            } else if (pInfo->nSpeakerAllocation[0] & BIT(6)) {
                 ALOGV("RLC/RRC");
            } else if (pInfo->nSpeakerAllocation[0] & BIT(5)) {
                 ALOGV("FLC/FRC");
            } else if (pInfo->nSpeakerAllocation[0] & BIT(4)) {
                ALOGV("RC");
            } else if (pInfo->nSpeakerAllocation[0] & BIT(3)) {
                ALOGV("RL/RR");
            } else if (pInfo->nSpeakerAllocation[0] & BIT(2)) {
                ALOGV("FC");
            } else if (pInfo->nSpeakerAllocation[0] & BIT(1)) {
                ALOGV("LFE");
            } else if (pInfo->nSpeakerAllocation[0] & BIT(0)) {
                ALOGV("FL/FR");
            }

            if (pInfo->nSpeakerAllocation[1] & BIT(2)) {
                ALOGV("FCH");
            } else if (pInfo->nSpeakerAllocation[1] & BIT(1)) {
                ALOGV("TC");
            } else if (pInfo->nSpeakerAllocation[1] & BIT(0)) {
                ALOGV("FLH/FRH");
            }
        }
    }
    if (original_data_ptr)
        free(original_data_ptr);
    return bRet;
}
