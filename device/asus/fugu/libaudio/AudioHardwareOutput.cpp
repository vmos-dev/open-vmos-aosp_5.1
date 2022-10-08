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

#define LOG_TAG "AudioHAL:AudioHardwareOutput"

#include <utils/Log.h>

#include <stdint.h>
#include <limits.h>
#include <math.h>

#include <common_time/local_clock.h>
#include <cutils/properties.h>

#include "AudioHardwareOutput.h"
#include "AudioStreamOut.h"
#include "HDMIAudioOutput.h"

namespace android {

// Global singleton.
AudioHardwareOutput gAudioHardwareOutput;

// HDMI options.
const String8 AudioHardwareOutput::kHDMIAllowedParamKey(
        "atv.hdmi_audio.allowed");
const String8 AudioHardwareOutput::kHDMIDelayCompParamKey(
        "atv.hdmi.audio_delay");
const String8 AudioHardwareOutput::kFixedHDMIOutputParamKey(
        "atv.hdmi.fixed_volume");
const String8 AudioHardwareOutput::kFixedHDMIOutputLevelParamKey(
        "atv.hdmi.fixed_level");

// Video delay comp hack options (not exposed to user level)
const String8 AudioHardwareOutput::kVideoDelayCompParamKey(
        "atv.video.delay_comp");

// Defaults for settings.
void AudioHardwareOutput::OutputSettings::setDefaults()
{
    allowed = true;
    delayCompUsec = 0;
    isFixed = false;
    fixedLvl = 0.0f;
}

void AudioHardwareOutput::Settings::setDefaults() {
    hdmi.setDefaults();

    masterVolume = 0.60;
    masterMute = false;

    // Default this to 16mSec or so.  Since audio start times are not sync'ed to
    // to the VBI, there should be a +/-0.5 output frame rate error in the AV
    // sync, even under the best of circumstances.
    //
    // In practice, the android core seems to have a hard time hitting its frame
    // cadence consistently.  Sometimes the frames are on time, and sometimes
    // they are even a little early, but more often than not, the frames are
    // late by about a full output frame time.
    //
    // ATV pretty much always uses a 60fps output rate, and the only thing
    // consuming the latency estimate provided by the HAL is the path handling
    // AV sync.  For now, we can fudge this number to move things back in the
    // direction of correct by providing a setting for video delay compensation
    // which will be subtracted from the latency estimate and defaulting it to
    // a reasonable middle gound (12mSec in this case).
    videoDelayCompUsec = 12000;
}

AudioHardwareOutput::AudioHardwareOutput()
  : mMainOutput(NULL)
  , mMCOutput(NULL)
  , mHDMIConnected(false)
  , mMaxDelayCompUsec(0)
{
    mSettings.setDefaults();
    mHDMICardID = find_alsa_card_by_name(kHDMI_ALSADeviceName);
}

AudioHardwareOutput::~AudioHardwareOutput()
{
    closeOutputStream(mMainOutput);
    closeOutputStream(mMCOutput);
}

status_t AudioHardwareOutput::initCheck() {
    return NO_ERROR;
}

AudioStreamOut* AudioHardwareOutput::openOutputStream(
        uint32_t devices,
        audio_format_t *format,
        uint32_t *channels,
        uint32_t *sampleRate,
        audio_output_flags_t flags,
        status_t *status) {
    (void) devices;
    AutoMutex lock(mStreamLock);

    AudioStreamOut** pp_out;
    AudioStreamOut* out;

    if (!(flags & AUDIO_OUTPUT_FLAG_DIRECT)) {
        pp_out = &mMainOutput;
        out = new AudioStreamOut(*this, false);
    } else {
        pp_out = &mMCOutput;
        out = new AudioStreamOut(*this, true);
    }

    if (out == NULL) {
        *status = NO_MEMORY;
        return NULL;
    }

    *status = out->set(format, channels, sampleRate);

    if (*status == NO_ERROR) {
        *pp_out = out;
        updateTgtDevices_l();
    } else {
        delete out;
    }

    return *pp_out;
}

void AudioHardwareOutput::closeOutputStream(AudioStreamOut* out) {
    if (out == NULL)
        return;

    // Putting the stream into "standby" should cause it to release all of its
    // physical outputs.
    out->standby();

    {
        Mutex::Autolock _l(mStreamLock);
        if (mMainOutput && out == mMainOutput) {
            delete mMainOutput;
            mMainOutput = NULL;
        } else if (mMCOutput && out == mMCOutput) {
            delete mMCOutput;
            mMCOutput = NULL;
        }

        updateTgtDevices_l();
    }
}

status_t AudioHardwareOutput::setMasterVolume(float volume)
{
    Mutex::Autolock _l1(mOutputLock);
    Mutex::Autolock _l2(mSettingsLock);

    mSettings.masterVolume = volume;

    AudioOutputList::iterator I;
    for (I = mPhysOutputs.begin(); I != mPhysOutputs.end(); ++I)
        (*I)->setVolume(mSettings.masterVolume);

    return NO_ERROR;
}

status_t AudioHardwareOutput::getMasterVolume(float* volume) {

    if (NULL == volume)
        return BAD_VALUE;

    // Explicit scope for auto-lock pattern.
    {
        Mutex::Autolock _l(mSettingsLock);
        *volume = mSettings.masterVolume;
    }

    return NO_ERROR;
}

status_t AudioHardwareOutput::setMasterMute(bool muted)
{
    Mutex::Autolock _l1(mOutputLock);
    Mutex::Autolock _l2(mSettingsLock);

    mSettings.masterMute = muted;

    AudioOutputList::iterator I;
    for (I = mPhysOutputs.begin(); I != mPhysOutputs.end(); ++I)
        (*I)->setMute(mSettings.masterMute);

    return NO_ERROR;
}

status_t AudioHardwareOutput::getMasterMute(bool* muted) {
    if (NULL == muted)
        return BAD_VALUE;

    // Explicit scope for auto-lock pattern.
    {
        Mutex::Autolock _l(mSettingsLock);
        *muted = mSettings.masterMute;
    }

    return NO_ERROR;
}

status_t AudioHardwareOutput::setParameters(const char* kvpairs) {
    AudioParameter param = AudioParameter(String8(kvpairs));
    status_t status = NO_ERROR;
    float floatVal;
    int intVal;
    Settings initial, s;

    {
        // Record the initial state of the settings from inside the lock.  Then
        // leave the lock in order to parse the changes to be made.
        Mutex::Autolock _l(mSettingsLock);
        initial = s = mSettings;
    }

    /***************************************************************
     *                     HDMI Audio Options                      *
     ***************************************************************/
    if (param.getInt(kHDMIAllowedParamKey, intVal) == NO_ERROR) {
        s.hdmi.allowed = (intVal != 0);
        param.remove(kHDMIAllowedParamKey);
    }

    if ((param.getFloat(kHDMIDelayCompParamKey, floatVal) == NO_ERROR) &&
        (floatVal >= 0.0) &&
        (floatVal <= AudioOutput::kMaxDelayCompensationMSec)) {
        uint32_t delay_comp = static_cast<uint32_t>(floatVal * 1000.0);
        s.hdmi.delayCompUsec = delay_comp;
        param.remove(kHDMIDelayCompParamKey);
    }

    if (param.getInt(kFixedHDMIOutputParamKey, intVal) == NO_ERROR) {
        s.hdmi.isFixed = (intVal != 0);
        param.remove(kFixedHDMIOutputParamKey);
    }

    if ((param.getFloat(kFixedHDMIOutputLevelParamKey, floatVal) == NO_ERROR)
        && (floatVal <= 0.0)) {
        s.hdmi.fixedLvl = floatVal;
        param.remove(kFixedHDMIOutputLevelParamKey);
    }

    /***************************************************************
     *                       Other Options                         *
     ***************************************************************/
    if ((param.getFloat(kVideoDelayCompParamKey, floatVal) == NO_ERROR) &&
        (floatVal >= 0.0) &&
        (floatVal <= AudioOutput::kMaxDelayCompensationMSec)) {
        s.videoDelayCompUsec = static_cast<uint32_t>(floatVal * 1000.0);
        param.remove(kVideoDelayCompParamKey);
    }

    if (param.size())
        status = BAD_VALUE;

    // If there was a change made to settings, go ahead and apply it now.
    bool allowedOutputsChanged = false;
    if (memcmp(&initial, &s, sizeof(initial)))  {
        Mutex::Autolock _l1(mOutputLock);
        Mutex::Autolock _l2(mSettingsLock);

        if (memcmp(&initial.hdmi, &s.hdmi, sizeof(initial.hdmi)))
            allowedOutputsChanged = allowedOutputsChanged ||
                applyOutputSettings_l(initial.hdmi, s.hdmi, mSettings.hdmi,
                                      HDMIAudioOutput::classDevMask());

        if (initial.videoDelayCompUsec != s.videoDelayCompUsec)
            mSettings.videoDelayCompUsec = s.videoDelayCompUsec;

        uint32_t tmp = 0;
        if (mSettings.hdmi.allowed && (tmp < mSettings.hdmi.delayCompUsec))
            tmp = mSettings.hdmi.delayCompUsec;
        if (mMaxDelayCompUsec != tmp)
            mMaxDelayCompUsec = tmp;
    }

    if (allowedOutputsChanged) {
        Mutex::Autolock _l(mStreamLock);
        updateTgtDevices_l();
    }

    return status;
}

bool AudioHardwareOutput::applyOutputSettings_l(
        const AudioHardwareOutput::OutputSettings& initial,
        const AudioHardwareOutput::OutputSettings& current,
        AudioHardwareOutput::OutputSettings& updateMe,
        uint32_t outDevMask) {
    // ASSERT(holding mOutputLock and mSettingsLock)
    sp<AudioOutput> out;

    // Check for a change in the allowed/not-allowed state.  Update if needed
    // and return true if there was a change made.
    bool ret = false;
    if (initial.allowed != current.allowed) {
        updateMe.allowed = current.allowed;
        ret = true;
    }

    // Look for an instance of the output to be updated in case other changes
    // were made.
    AudioOutputList::iterator I;
    for (I = mPhysOutputs.begin(); I != mPhysOutputs.end(); ++I) {
        if (outDevMask == (*I)->devMask()) {
            out = (*I);
            break;
        }
    }

    // Update the other settings, if needed.
    if (initial.delayCompUsec != current.delayCompUsec) {
        updateMe.delayCompUsec = current.delayCompUsec;
        if (out != NULL)
            out->setExternalDelay_uSec(current.delayCompUsec);
    }

    if (initial.isFixed != current.isFixed) {
        updateMe.isFixed = current.isFixed;
        if (out != NULL)
            out->setOutputIsFixed(current.isFixed);
    }

    if (initial.fixedLvl != current.fixedLvl) {
        updateMe.fixedLvl = current.fixedLvl;
        if (out != NULL)
            out->setFixedOutputLevel(current.fixedLvl);
    }

    return ret;
}


char* AudioHardwareOutput::getParameters(const char* keys) {
    Settings s;

    // Explicit scope for auto-lock pattern.
    {
        // Snapshot the current settings so we don't have to hold the settings
        // lock while formatting the results.
        Mutex::Autolock _l(mSettingsLock);
        s = mSettings;
    }

    AudioParameter param = AudioParameter(String8(keys));
    String8 tmp;

    /***************************************************************
     *                     HDMI Audio Options                      *
     ***************************************************************/
    if (param.get(kHDMIAllowedParamKey, tmp) == NO_ERROR)
        param.addInt(kHDMIAllowedParamKey, s.hdmi.allowed ? 1 : 0);

    if (param.get(kHDMIDelayCompParamKey, tmp) == NO_ERROR)
        param.addFloat(kHDMIDelayCompParamKey,
                       static_cast<float>(s.hdmi.delayCompUsec) / 1000.0);

    if (param.get(kFixedHDMIOutputParamKey, tmp) == NO_ERROR)
        param.addInt(kFixedHDMIOutputParamKey, s.hdmi.isFixed ? 1 : 0);

    if (param.get(kFixedHDMIOutputLevelParamKey, tmp) == NO_ERROR)
        param.addFloat(kFixedHDMIOutputLevelParamKey, s.hdmi.fixedLvl);

    /***************************************************************
     *                       Other Options                         *
     ***************************************************************/
    if (param.get(kVideoDelayCompParamKey, tmp) == NO_ERROR)
        param.addFloat(kVideoDelayCompParamKey,
                       static_cast<float>(s.videoDelayCompUsec) / 1000.0);

    return strdup(param.toString().string());
}

void AudioHardwareOutput::updateRouting(uint32_t devMask) {
    Mutex::Autolock _l(mStreamLock);

    bool hasHDMI = 0 != (devMask & HDMIAudioOutput::classDevMask());
    ALOGI("%s: hasHDMI = %d, mHDMIConnected = %d", __func__, hasHDMI, mHDMIConnected);
    if (mHDMIConnected != hasHDMI) {
        mHDMIConnected = hasHDMI;

        if (mHDMIConnected)
            mHDMIAudioCaps.loadCaps(mHDMICardID);
        else
            mHDMIAudioCaps.reset();

        updateTgtDevices_l();
    }
}

status_t AudioHardwareOutput::obtainOutput(const AudioStreamOut& tgtStream,
                                     uint32_t devMask,
                                     sp<AudioOutput>* newOutput) {
    Mutex::Autolock _l1(mOutputLock);

    // Sanity check the device mask passed to us.  There should exactly one bit
    // set, no less, no more.
    if (popcount(devMask) != 1) {
        ALOGW("bad device mask in obtainOutput, %08x", devMask);
        return INVALID_OPERATION;
    }

    // Start by checking to see if the requested output is currently busy.
    AudioOutputList::iterator I;
    for (I = mPhysOutputs.begin(); I != mPhysOutputs.end(); ++I)
        if (devMask & (*I)->devMask())
            return OK; // Yup; its busy.

    // Looks like we don't currently have an output of the requested type.
    // Figure out which type is being requested and try to construct one.
    OutputSettings* S = NULL;
    if (devMask & HDMIAudioOutput::classDevMask()) {
        *newOutput = new HDMIAudioOutput();
        S = &mSettings.hdmi;
    }
    else {
        ALOGW("%s stream out requested output of unknown type %08x",
                tgtStream.getName(), devMask);
        return BAD_VALUE;
    }

    if (*newOutput == NULL)
        return NO_MEMORY;

    status_t res = (*newOutput)->setupForStream(tgtStream);
    if (res != OK) {
        ALOGE("%s setupForStream() returned %d",
              tgtStream.getName(), res);
        *newOutput = NULL;
    } else {
        ALOGI("%s stream out adding %s output.",
                tgtStream.getName(), (*newOutput)->getOutputName());
        mPhysOutputs.push_back(*newOutput);

        {  // Apply current settings
            Mutex::Autolock _l2(mSettingsLock);
            (*newOutput)->setVolume(mSettings.masterVolume);
            (*newOutput)->setMute(mSettings.masterMute);
            (*newOutput)->setExternalDelay_uSec(S->delayCompUsec);
            (*newOutput)->setOutputIsFixed(S->isFixed);
            (*newOutput)->setFixedOutputLevel(S->fixedLvl);
        }
    }

    return res;
}

void AudioHardwareOutput::releaseOutput(const AudioStreamOut& tgtStream,
                                        const sp<AudioOutput>& releaseMe) {
    Mutex::Autolock _l(mOutputLock);

    ALOGI("%s stream out removing %s output.",
            tgtStream.getName(), releaseMe->getOutputName());

    // Immediately release any resources associated with this output (In
    // particular, make sure to close any ALSA device driver handles ASAP)
    releaseMe->cleanupResources();

    // Now, clear our internal bookkeeping.
    AudioOutputList::iterator I;
    for (I = mPhysOutputs.begin(); I != mPhysOutputs.end(); ++I) {
        if (releaseMe.get() == (*I).get()) {
            mPhysOutputs.erase(I);
            break;
        }
    }
}

void AudioHardwareOutput::updateTgtDevices_l() {
    // ASSERT(holding mStreamLock)
    uint32_t mcMask = 0;
    uint32_t mainMask = 0;

    {
        Mutex::Autolock _l(mSettingsLock);
        if (mSettings.hdmi.allowed && mHDMIConnected) {
            if (NULL != mMCOutput)
                mcMask |= HDMIAudioOutput::classDevMask();
            else
                mainMask |= HDMIAudioOutput::classDevMask();
        }
    }

    if (NULL != mMainOutput)
        mMainOutput->setTgtDevices(mainMask);

    if (NULL != mMCOutput)
        mMCOutput->setTgtDevices(mcMask);
}

void AudioHardwareOutput::standbyStatusUpdate(bool isInStandby, bool isMCStream) {

    Mutex::Autolock _l1(mStreamLock);
    bool hdmiAllowed;
    {
        Mutex::Autolock _l2(mSettingsLock);
        hdmiAllowed = mSettings.hdmi.allowed;
    }
    // If there is no HDMI, do nothing
    if (hdmiAllowed && mHDMIConnected) {
        // If a multi-channel stream goes to standy state, we must switch
        // to stereo stream. If MC comes out of standby, we must switch
        // back to MC. No special processing needed for main stream.
        // AudioStreamOut class handles that correctly
        if (isMCStream) {
            uint32_t mcMask;
            uint32_t mainMask;
            if (isInStandby) {
                mainMask = HDMIAudioOutput::classDevMask();
                mcMask = 0;
            } else {
                mainMask = 0;
                mcMask = HDMIAudioOutput::classDevMask();
            }

            if (NULL != mMainOutput)
                mMainOutput->setTgtDevices(mainMask);

            if (NULL != mMCOutput)
                mMCOutput->setTgtDevices(mcMask);
        }
    }
}

#define DUMP(a...) \
    snprintf(buffer, SIZE, a); \
    buffer[SIZE - 1] = 0; \
    result.append(buffer);
#define B2STR(b) b ? "true" : "false"

status_t AudioHardwareOutput::dump(int fd)
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;
    Settings s;

    // Explicit scope for auto-lock pattern.
    {
        // Snapshot the current settings so we don't have to hold the settings
        // lock while formatting the results.
        Mutex::Autolock _l(mSettingsLock);
        s = mSettings;
    }

    DUMP("AudioHardwareOutput::dump\n");
    DUMP("\tMaster Volume          : %0.3f\n", s.masterVolume);
    DUMP("\tMaster Mute            : %s\n", B2STR(s.masterMute));
    DUMP("\tHDMI Output Allowed    : %s\n", B2STR(s.hdmi.allowed));
    DUMP("\tHDMI Delay Comp        : %u uSec\n", s.hdmi.delayCompUsec);
    DUMP("\tHDMI Output Fixed      : %s\n", B2STR(s.hdmi.isFixed));
    DUMP("\tHDMI Fixed Level       : %.1f dB\n", s.hdmi.fixedLvl);
    DUMP("\tVideo Delay Comp       : %u uSec\n", s.videoDelayCompUsec);

    ::write(fd, result.string(), result.size());

    // Explicit scope for auto-lock pattern.
    {
        Mutex::Autolock _l(mOutputLock);
        if (mMainOutput)
            mMainOutput->dump(fd);

        if (mMCOutput)
            mMCOutput->dump(fd);
    }

    return NO_ERROR;
}

#undef B2STR
#undef DUMP

}; // namespace android
