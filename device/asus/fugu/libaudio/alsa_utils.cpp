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

#define LOG_TAG "AudioHAL:alsa_utils"

#include "alsa_utils.h"

#ifndef ALSA_UTILS_PRINT_FORMATS
#define ALSA_UTILS_PRINT_FORMATS  1
#endif

int find_alsa_card_by_name(const char* name) {
    int card_id = 0;
    int ret = -1;
    int fd;

    do {
        int fd;
        int amt;
        char tmp[256];

        snprintf(tmp, sizeof(tmp), "/proc/asound/card%d/id", card_id);
        tmp[sizeof(tmp) - 1] = 0;
        fd = open(tmp, O_RDONLY);
        if (fd < 0)
            break;

        amt = read(fd, tmp, sizeof(tmp) - 1);
        if (amt > 0) {
            // replace the '\n' at the end of the proc file with '\0'
            tmp[amt - 1] = 0;
            if (!strcmp(name, tmp))
                ret = card_id;
        }

        close(fd);

        card_id++;
    } while (ret < 0);

    ALOGI("%s: returning card %d for name %s", __func__, ret, name);
    return ret;
}

#ifdef __cplusplus
#include <tinyalsa/asoundlib.h>
#include <utils/misc.h>

namespace android {

static const char *kCtrlNames[] = {
    "Basic Audio Supported",
    "Speaker Allocation",
    "Audio Mode Count",
    "Audio Mode To Query",
    "Query Mode : Format",
    "Query Mode : Max Ch Count",
    "Query Mode : Sample Rate Mask",
    "Query Mode : PCM Bits/Sample Mask",
    "Query Mode : Max Compressed Bitrate"
};
static const size_t kCtrlCount    = sizeof(kCtrlNames)/sizeof(*kCtrlNames);
static const size_t kBasicAudNdx  = 0;
static const size_t kSpeakerAlloc = 1;
static const size_t kModeCntNdx   = 2;
static const size_t kModeSelNdx   = 3;
static const size_t kFmtNdx       = 4;
static const size_t kMaxChCntNdx  = 5;
static const size_t kSampRateNdx  = 6;
static const size_t kBPSNdx       = 7;
static const size_t kMaxCompBRNdx = 8;

HDMIAudioCaps::HDMIAudioCaps()
{
    // Its unlikely we will need storage for more than 16 modes, but if we do,
    // the vector will resize for us.
    mModes.setCapacity(16);
    reset();
}

bool HDMIAudioCaps::loadCaps(int ALSADeviceID) {
    bool ret = false;
    struct mixer* mixer = NULL;
    struct mixer_ctl* ctrls[kCtrlCount] = {NULL};
    int tmp, mode_cnt;
    Mutex::Autolock _l(mLock);

    ALOGE("%s: start", __func__);

    reset_l();

    // Open the mixer for the chosen ALSA device
    if (NULL == (mixer = mixer_open(ALSADeviceID))) {
        ALOGE("%s: mixer_open(%d) failed", __func__, ALSADeviceID);
        goto bailout;
    }

    // Gather handles to all of the controls we will need in order to enumerate
    // the audio capabilities of this HDMI link.  No need to free/release these
    // later, they are just pointers into the tinyalsa mixer structure itself.
    for (size_t i = 0; i < kCtrlCount; ++i) {
        ctrls[i] = mixer_get_ctl_by_name(mixer, kCtrlNames[i]);
        if (NULL == ctrls[i]) {
            ALOGE("%s: mixer_get_ctrl_by_name(%s) failed", __func__, kCtrlNames[i]);
            goto bailout;
	}
    }

    // Start by checking to see if this HDMI connection supports even basic
    // audio.  If it does not, there is no point in proceeding.
    if ((tmp = mixer_ctl_get_value(ctrls[kBasicAudNdx], 0)) <= 0) {
        ALOGI("%s: Basic audio not supported by attached device", __func__);
        goto bailout;
    }

    // Looks like we support basic audio.  Get a count of the available
    // non-basic modes.
    mBasicAudioSupported = true;
    if ((mode_cnt = mixer_ctl_get_value(ctrls[kModeCntNdx], 0)) < 0)
        goto bailout;

    // Fetch the speaker allocation data block, if available.
    if ((tmp = mixer_ctl_get_value(ctrls[kSpeakerAlloc], 0)) < 0)
        goto bailout;
    mSpeakerAlloc = static_cast<uint16_t>(tmp);
    ALOGI("%s: Speaker Allocation Map for attached device is: 0x%hx", __func__, mSpeakerAlloc);

    // If there are no non-basic modes available, then we are done.  Be sure to
    // flag this as a successful operation.
    if (!mode_cnt) {
        ret = true;
        goto bailout;
    }

    // Now enumerate the non-basic modes.  Any errors at this point in time
    // should indicate that the HDMI cable was unplugged and we should just
    // abort with an empty set of audio capabilities.
    for (int i = 0; i < mode_cnt; ++i) {
        Mode m;

        // Pick the mode we want to fetch info for.
        if (mixer_ctl_set_value(ctrls[kModeSelNdx], 0, i) < 0)
            goto bailout;

        // Now fetch the common fields.
        if ((tmp = mixer_ctl_get_value(ctrls[kFmtNdx], 0)) < 0)
            goto bailout;
        m.fmt = static_cast<AudFormat>(tmp);
        ALOGI("Got mode %d from ALSA driver.", m.fmt);

        if ((tmp = mixer_ctl_get_value(ctrls[kMaxChCntNdx], 0)) < 0)
            goto bailout;
        m.max_ch = static_cast<uint32_t>(tmp);

        if ((tmp = mixer_ctl_get_value(ctrls[kSampRateNdx], 0)) < 0)
            goto bailout;
        m.sr_bitmask = static_cast<uint32_t>(tmp);

        // Now for the mode dependent fields.  Only LPCM has the bits-per-sample
        // mask.  Only AC3 through ATRAC have the compressed bitrate field.
        m.bps_bitmask = 0;
        m.comp_bitrate = 0;

        if (m.fmt == kFmtLPCM) {
            if ((tmp = mixer_ctl_get_value(ctrls[kBPSNdx], 0)) < 0)
                goto bailout;
            m.bps_bitmask = static_cast<uint32_t>(tmp);
        } else if ((m.fmt >= kFmtAC3) && (m.fmt <= kFmtATRAC)) { // FIXME ATRAC is not last format!?
            if ((tmp = mixer_ctl_get_value(ctrls[kMaxCompBRNdx], 0)) < 0)
                goto bailout;
            m.comp_bitrate = static_cast<uint32_t>(tmp);
        }

        // Finally, sanity check the info.  If it passes, add it to the vector
        // of available modes.
        if (sanityCheckMode(m))  {
            ALOGI("Passed sanity check for mode %d from ALSA driver.", m.fmt);
            mModes.add(m);
        }
    }

    // Looks like we managed to enumerate all of the modes before someone
    // unplugged the HDMI cable.  Signal success and get out.
    ret = true;

bailout:
    if (NULL != mixer)
        mixer_close(mixer);

    if (!ret)
        reset_l();

    return ret;
}

void HDMIAudioCaps::reset() {
    Mutex::Autolock _l(mLock);
    reset_l();
}

void HDMIAudioCaps::reset_l() {
    mBasicAudioSupported = false;
    mSpeakerAlloc = 0;
    mModes.clear();
}

void HDMIAudioCaps::getRatesForAF(String8& rates) {
    Mutex::Autolock _l(mLock);
    rates.clear();

    // If the sink does not support basic audio, then it supports no audio.
    if (!mBasicAudioSupported)
        return;

    // Basic audio always supports from 32k through 38k.
    uint32_t tmp = kSR_32000 | kSR_44100 | kSR_48000;

    // To keep things simple, only report mode information for the PCM mode
    // which supports the maximum number of channels.
    ssize_t ndx = getMaxChModeNdx_l();
    if (ndx >= 0)
        tmp |= mModes[ndx].sr_bitmask;

    bool first = true;
    for (uint32_t i = 1; tmp; i <<= 1) {
        if (i & tmp) {
            rates.appendFormat(first ? "%d" : "|%d", srMaskToSR(i));
            first = false;
            tmp &= ~i;
        }
    }
}

void HDMIAudioCaps::getFmtsForAF(String8& fmts) {
    Mutex::Autolock _l(mLock);
    fmts.clear();

    // If the sink does not support basic audio, then it supports no audio.
    if (!mBasicAudioSupported) {
        ALOGI("ALSAFORMATS: basic audio not supported");
        return;
    }

    fmts.append("AUDIO_FORMAT_PCM_16_BIT");
    // TODO: when we can start to expect 20 and 24 bit audio modes coming from
    // AF, we need to implement support to enumerate those modes.

    // These names must match formats in android.media.AudioFormat
    for (size_t i = 0; i < mModes.size(); ++i) {
        switch (mModes[i].fmt) {
            case kFmtAC3:
                fmts.append("|AUDIO_FORMAT_AC3");
                break;
            case kFmtEAC3:
                fmts.append("|AUDIO_FORMAT_E_AC3");
                break;
            default:
                break;
        }
    }

#if ALSA_UTILS_PRINT_FORMATS
    ALOGI("ALSAFORMATS: formats = %s", fmts.string());

    for (size_t i = 0; i < mModes.size(); ++i) {
        ALOGI("ALSAFORMATS: ------- fmt[%d] = 0x%08X = %s",
            i, mModes[i].fmt, fmtToString(mModes[i].fmt));
        ALOGI("ALSAFORMATS:   comp_bitrate[%d] = 0x%08X = %d",
            i, mModes[i].comp_bitrate, mModes[i].comp_bitrate);
        ALOGI("ALSAFORMATS:   max_ch[%d] = 0x%08X = %d",
            i, mModes[i].max_ch, mModes[i].max_ch);
        ALOGI("ALSAFORMATS:   bps_bitmask[%d] = 0x%08X", i, mModes[i].bps_bitmask);
        uint32_t bpsm = mModes[i].bps_bitmask;
        while(bpsm) {
            uint32_t bpsm_next = bpsm & (bpsm - 1);
            uint32_t bpsm_single = bpsm ^ bpsm_next;
            if (bpsm_single) {
                ALOGI("ALSAFORMATS:      bits  = %d", bpsMaskToBPS(bpsm_single));
            }
            bpsm = bpsm_next;
        }
        ALOGI("ALSAFORMATS:   sr_bitmask[%d] = 0x%08X", i, mModes[i].sr_bitmask);
        uint32_t srs = mModes[i].sr_bitmask;
        while(srs) {
            uint32_t srs_next = srs & (srs - 1);
            uint32_t srs_single = srs ^ srs_next;
            if (srs_single) {
                ALOGI("ALSAFORMATS:      srate = %d", srMaskToSR(srs_single));
            }
            srs = srs_next;
        }
    }
#endif /* ALSA_UTILS_PRINT_FORMATS */
}

void HDMIAudioCaps::getChannelMasksForAF(String8& masks) {
    Mutex::Autolock _l(mLock);
    masks.clear();

    // If the sink does not support basic audio, then it supports no audio.
    if (!mBasicAudioSupported)
        return;

    masks.append("AUDIO_CHANNEL_OUT_STEREO");

    // To keep things simple, only report mode information for the mode
    // which supports the maximum number of channels.
    ssize_t ndx = getMaxChModeNdx_l();
    if (ndx < 0)
        return;

    if (mModes[ndx].max_ch >= 6) {
        if (masks.length())
            masks.append("|");

        masks.append((mModes[ndx].max_ch >= 8)
                ? "AUDIO_CHANNEL_OUT_5POINT1|AUDIO_CHANNEL_OUT_7POINT1"
                : "AUDIO_CHANNEL_OUT_5POINT1");
    }
}

ssize_t HDMIAudioCaps::getMaxChModeNdx_l() {
    ssize_t max_ch_ndx = -1;
    uint32_t max_ch = 0;

    for (size_t i = 0; i < mModes.size(); ++i) {
        if (max_ch < mModes[i].max_ch) {
            max_ch = mModes[i].max_ch;
            max_ch_ndx = i;
        }
    }

    return max_ch_ndx;
}

bool HDMIAudioCaps::supportsFormat(audio_format_t format,
                                      uint32_t sampleRate,
                                      uint32_t channelCount) {
    Mutex::Autolock _l(mLock);

    // If the sink does not support basic audio, then it supports no audio.
    if (!mBasicAudioSupported)
        return false;

    AudFormat alsaFormat;
    switch (format & AUDIO_FORMAT_MAIN_MASK) {
        case AUDIO_FORMAT_PCM: alsaFormat = kFmtLPCM; break;
        case AUDIO_FORMAT_AC3: alsaFormat = kFmtAC3; break;
        case AUDIO_FORMAT_E_AC3: alsaFormat = kFmtAC3; break;
        default: return false;
    }

    SRMask srMask;
    switch (sampleRate) {
        case 32000:  srMask = kSR_32000;  break;
        case 44100:  srMask = kSR_44100;  break;
        case 48000:  srMask = kSR_48000;  break;
        case 88200:  srMask = kSR_88200;  break;
        case 96000:  srMask = kSR_96000;  break;
        case 176400: srMask = kSR_176400; break;
        case 192000: srMask = kSR_192000; break;
        default: return false;
    }

    // if PCM then determine actual bits per sample.
    if (alsaFormat == kFmtLPCM) {
        uint32_t subFormat = format & AUDIO_FORMAT_SUB_MASK;
        BPSMask bpsMask;
        switch (subFormat) {
            case AUDIO_FORMAT_PCM_SUB_16_BIT: bpsMask = kBPS_16bit; break;
            default: return false;
        }

        // Is the caller requesting basic audio?  If so, we should be good to go.
        // Otherwise, we need to check the mode table.
        if ((2 == channelCount) && (sampleRate <= 48000))
            return true;

        // Check the modes in the table to see if there is one which
        // supports the caller's format.
        for (size_t i = 0; i < mModes.size(); ++i) {
            const Mode& m = mModes[i];
            if ((m.fmt == kFmtLPCM) &&
                (m.max_ch >= channelCount) &&
                (m.sr_bitmask & srMask) &&
                (m.bps_bitmask & bpsMask))
                return true;
        }
    } else {
        // Check the modes in the table to see if there is one which
        // supports the caller's format.
        for (size_t i = 0; i < mModes.size(); ++i) {
            const Mode& m = mModes[i];
            // ignore bps_bitmask
            if ((m.fmt == alsaFormat) &&
                (m.max_ch >= channelCount) &&
                (m.sr_bitmask & srMask))
                return true;
        }
    }

    // Looks like no compatible modes were found.
    return false;
}

bool HDMIAudioCaps::sanityCheckMode(const Mode& m) {
    if ((m.fmt < kFmtLPCM) || (m.fmt > kFmtMPGSUR))
        return false;

    if (m.max_ch > 8)
        return false;

    if (m.sr_bitmask & ~(kSR_32000 | kSR_44100 | kSR_48000 | kSR_88200 |
                         kSR_96000 | kSR_176400 | kSR_192000))
        return false;

    if (m.bps_bitmask & ~(kBPS_16bit | kBPS_20bit | kBPS_24bit))
        return false;

    return true;
}

const char* HDMIAudioCaps::fmtToString(AudFormat fmt) {
    static const char* fmts[] = {
        "invalid", "LPCM", "AC-3", "MPEG-1", "MPEG-1 Layer 3",
        "MPEG-2", "AAC-LC", "DTS", "ATRAC", "DSD", "E-AC3",
        "DTS-HD", "MLP", "DST", "WMA Pro", "Extended" };

    if (fmt >= NELEM(fmts))
        return "invalid";

    return fmts[fmt];
}

uint32_t HDMIAudioCaps::srMaskToSR(uint32_t mask) {
    switch (mask) {
        case kSR_32000: return 32000;
        case kSR_44100: return 44100;
        case kSR_48000: return 48000;
        case kSR_88200: return 88200;
        case kSR_96000: return 96000;
        case kSR_176400: return 176400;
        case kSR_192000: return 192000;
        default: return 0;
    }
}

uint32_t HDMIAudioCaps::bpsMaskToBPS(uint32_t mask) {
    switch (mask) {
        case kBPS_16bit: return 16;
        case kBPS_20bit: return 20;
        case kBPS_24bit: return 24;
        default: return 0;
    }
}

const char* HDMIAudioCaps::saMaskToString(uint32_t mask) {
    switch (mask) {
        case kSA_FLFR:   return "Front Left/Right";
        case kSA_LFE:    return "LFE";
        case kSA_FC:     return "Front Center";
        case kSA_RLRR:   return "Rear Left/Right";
        case kSA_RC:     return "Rear Center";
        case kSA_FLCFRC: return "Front Left/Right Center";
        case kSA_RLCRRC: return "Rear Left/Right Center";
        case kSA_FLWFRW: return "Front Left/Right Wide";
        case kSA_FLHFRH: return "Front Left/Right High";
        case kSA_TC:     return "Top Center (overhead)";
        case kSA_FCH:    return "Front Center High";
        default: return "unknown";
    }
}

}  // namespace android
#endif  // __cplusplus
