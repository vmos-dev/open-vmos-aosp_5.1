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

#ifndef ANDROID_ALSA_UTILS_H
#define ANDROID_ALSA_UTILS_H

#include <fcntl.h>
#include <stdint.h>
#include <hardware/audio.h>

#define kHDMI_ALSADeviceName    "IntelHDMI"

#ifdef __cplusplus
extern "C"
#endif
int find_alsa_card_by_name(const char* name);

#ifdef __cplusplus
#include <utils/Vector.h>
#include <utils/Mutex.h>
#include <utils/String8.h>

namespace android {

class HDMIAudioCaps {
  public:
    enum AudFormat {
        kFmtInvalid = 0,
        kFmtLPCM,
        kFmtAC3,
        kFmtMP1,
        kFmtMP1L3,
        kFmtMP2,
        kFmtAACLC,
        kFmtDTS,
        kFmtATRAC,
        kFmtDSD,
        kFmtEAC3,
        kFmtDTSHD,
        kFmtMLP,
        kFmtDST,
        kFmtWMAPRO,
        kFmtRefCxt,
        kFmtHEAAC,
        kFmtHEAAC2,
        kFmtMPGSUR
    };

    enum SRMask {
        kSR_32000 = (1 << 5),
        kSR_44100 = (1 << 6),
        kSR_48000 = (1 << 7),
        kSR_88200 = (1 << 9),
        kSR_96000 = (1 << 10),
        kSR_176400 = (1 << 11),
        kSR_192000 = (1 << 12),
    };

    enum BPSMask {
        kBPS_16bit = (1 << 17),
        kBPS_20bit = (1 << 18),
        kBPS_24bit = (1 << 19),
    };

    enum SAMask {
        kSA_FLFR   = (1 <<  0), // Front Left/Right
        kSA_LFE    = (1 <<  1), // LFE (aka, subwoofer)
        kSA_FC     = (1 <<  2), // Front Center
        kSA_RLRR   = (1 <<  3), // Rear Left/Right
        kSA_RC     = (1 <<  4), // Rear Center
        kSA_FLCFRC = (1 <<  5), // Front Left/Right Center
        kSA_RLCRRC = (1 <<  6), // Rear Left/Right Center
        kSA_FLWFRW = (1 <<  7), // Front Left/Right Wide
        kSA_FLHFRH = (1 <<  8), // Front Left/Right High
        kSA_TC     = (1 <<  9), // Top Center (overhead)
        kSA_FCH    = (1 << 10), // Front Center High
    };

    typedef struct {
        AudFormat fmt;
        uint32_t  max_ch;
        uint32_t  sr_bitmask;
        uint32_t  bps_bitmask;
        uint32_t  comp_bitrate;
    } Mode;

    HDMIAudioCaps();
    ~HDMIAudioCaps() { reset(); }

    bool loadCaps(int ALSADeviceID);
    void reset();
    void getRatesForAF(String8& rates);
    void getFmtsForAF(String8& fmts);
    void getChannelMasksForAF(String8& masks);
    bool supportsFormat(audio_format_t format,
                                      uint32_t sampleRate,
                                      uint32_t channelCount);

    bool basicAudioSupport() const { return mBasicAudioSupported; }
    uint16_t speakerAllocation() const { return mSpeakerAlloc; }
    size_t modeCnt() const { return mModes.size(); }
    const Mode& getMode(size_t ndx) const { return mModes[ndx]; }

    static const char* fmtToString(AudFormat fmt);
    static uint32_t srMaskToSR(uint32_t mask);
    static uint32_t bpsMaskToBPS(uint32_t mask);
    static const char* saMaskToString(uint32_t mask);

  private:
    Mutex mLock;
    bool mBasicAudioSupported;
    uint16_t mSpeakerAlloc;
    Vector<Mode> mModes;

    void reset_l();
    ssize_t getMaxChModeNdx_l();
    static bool sanityCheckMode(const Mode& m);
};
}  // namespace android
#endif  // __cplusplus
#endif  // ANDROID_ALSA_UTILS_H
