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

#define LOG_TAG "AudioHAL:AudioStreamOut"

#include <utils/Log.h>

#include "AudioHardwareOutput.h"
#include "AudioStreamOut.h"

// Set to 1 to print timestamp data in CSV format.
#ifndef HAL_PRINT_TIMESTAMP_CSV
#define HAL_PRINT_TIMESTAMP_CSV 0
#endif

//#define VERY_VERBOSE_LOGGING
#ifdef VERY_VERBOSE_LOGGING
#define ALOGVV ALOGV
#else
#define ALOGVV(a...) do { } while(0)
#endif

namespace android {

AudioStreamOut::AudioStreamOut(AudioHardwareOutput& owner, bool mcOut)
    : mFramesPresented(0)
    , mFramesRendered(0)
    , mFramesWrittenRemainder(0)
    , mOwnerHAL(owner)
    , mFramesWritten(0)
    , mTgtDevices(0)
    , mAudioFlingerTgtDevices(0)
    , mIsMCOutput(mcOut)
    , mIsEncoded(false)
    , mInStandby(false)
    , mSPDIFEncoder(this)
{
    assert(mLocalClock.initCheck());

    mPhysOutputs.setCapacity(3);

    // Set some reasonable defaults for these.  All of this should be eventually
    // be overwritten by a specific audio flinger configuration, but it does not
    // hurt to have something here by default.
    mInputSampleRate = 48000;
    mInputChanMask = AUDIO_CHANNEL_OUT_STEREO;
    mInputFormat = AUDIO_FORMAT_PCM_16_BIT;
    mInputNominalChunksInFlight = 4;
    updateInputNums();

    mThrottleValid = false;

    memset(&mUSecToLocalTime, 0, sizeof(mUSecToLocalTime));
    mUSecToLocalTime.a_to_b_numer = mLocalClock.getLocalFreq();
    mUSecToLocalTime.a_to_b_denom = 1000000;
    LinearTransform::reduce(&mUSecToLocalTime.a_to_b_numer,
                            &mUSecToLocalTime.a_to_b_denom);
}

AudioStreamOut::~AudioStreamOut()
{
    releaseAllOutputs();
}

status_t AudioStreamOut::set(
        audio_format_t *pFormat,
        uint32_t *pChannels,
        uint32_t *pRate)
{
    Mutex::Autolock _l(mLock);
    audio_format_t lFormat   = pFormat ? *pFormat : AUDIO_FORMAT_DEFAULT;
    uint32_t       lChannels = pChannels ? *pChannels : 0;
    uint32_t       lRate     = pRate ? *pRate : 0;

    // fix up defaults
    if (lFormat == AUDIO_FORMAT_DEFAULT) lFormat = format();
    if (lChannels == 0)                  lChannels = chanMask();
    if (lRate == 0)                      lRate = sampleRate();

    if (pFormat)   *pFormat   = lFormat;
    if (pChannels) *pChannels = lChannels;
    if (pRate)     *pRate     = lRate;

    mIsEncoded = !audio_is_linear_pcm(lFormat);

    if (!mIsMCOutput && !mIsEncoded) {
        // If this is the primary stream out, then demand our defaults.
        if ((lFormat   != format()) ||
            (lChannels != chanMask()) ||
            (lRate     != sampleRate()))
            return BAD_VALUE;
    } else {
        // Else check to see if our HDMI sink supports this format before proceeding.
        if (!mOwnerHAL.getHDMIAudioCaps().supportsFormat(lFormat,
                                                     lRate,
                                                     audio_channel_count_from_out_mask(lChannels)))
            return BAD_VALUE;
    }

    mInputFormat = lFormat;
    mInputChanMask = lChannels;
    mInputSampleRate = lRate;
    ALOGI("AudioStreamOut::set: lRate = %u, mIsEncoded = %d\n", lRate, mIsEncoded);
    updateInputNums();

    return NO_ERROR;
}

void AudioStreamOut::setTgtDevices(uint32_t tgtDevices)
{
    Mutex::Autolock _l(mRoutingLock);
    if (mTgtDevices != tgtDevices) {
        mTgtDevices = tgtDevices;
    }
}

status_t AudioStreamOut::standby()
{
    mFramesRendered = 0;
    releaseAllOutputs();
    mOwnerHAL.standbyStatusUpdate(true, mIsMCOutput);
    mInStandby = true;

    return NO_ERROR;
}

void AudioStreamOut::releaseAllOutputs() {
    Mutex::Autolock _l(mRoutingLock);

    ALOGI("releaseAllOutputs: releasing %d mPhysOutputs", mPhysOutputs.size());
    AudioOutputList::iterator I;
    for (I = mPhysOutputs.begin(); I != mPhysOutputs.end(); ++I)
        mOwnerHAL.releaseOutput(*this, *I);

    mPhysOutputs.clear();
}

void AudioStreamOut::updateInputNums()
{
    assert(mLocalClock.initCheck());

    // mInputBufSize determines how many audio frames AudioFlinger is going to
    // mix at a time.  We also use the mInputBufSize to determine the ALSA
    // period_size, the number of of samples which need to play out (at most)
    // before low level ALSA driver code is required to wake up upper levels of
    // SW to fill a new buffer.  As it turns out, ALSA is going to apply some
    // rules and modify the period_size which we pass to it.  One of the things
    // ALSA seems to do is attempt to round the period_size up to a value which
    // will make the period an integral number of 0.5 mSec.  This round-up
    // behavior can cause the low levels of ALSA to consume more data per period
    // than the AudioFlinger mixer has been told to produce.  If there are only
    // two buffers in flight at any given point in time, this can lead to a
    // situation where the pipeline ends up slipping an extra buffer and
    // underflowing.  There are two approaches to mitigate this, both of which
    // are implemented in this HAL...
    //
    // 1) Try as hard as possible to make certain that the buffer size we choose
    //    results in a period_size which is not going to get rounded up by ALSA.
    //    This means that we want a buffer size which at the chosen sample rate
    //    and frame size will be an integral multiple of 1/2 mSec.
    // 2) Increate the number of chunks we keep in flight.  If the system slips
    //    a single period, its only really a problem if there is no data left in
    //    the pipeline waiting to be played out.  The mixer should going to mix
    //    as fast as possible until the buffer has been topped off.  By
    //    decreasing the buffer size and increasing the number of buffers in
    //    flight, we increase the number of interrups and mix events per second,
    //    but buy ourselves some insurance against the negative side effects of
    //    slipping one buffer in the schedule.  We end up using 4 buffers at
    //    10mSec, making the total audio latency somewhere between 40 and 50
    //    mSec, depending on when a sample begins playback relative to
    //    AudioFlinger's mixing schedule.
    //
    mInputChanCount = audio_channel_count_from_out_mask(mInputChanMask);

    // Picking a chunk duration 10mSec should satisfy #1 for both major families
    // of audio sample rates (the 44.1K and 48K families).  In the case of 44.1
    // (or higher) we will end up with a multiple of 441 frames of audio per
    // chunk, while for 48K, we will have a multiple of 480 frames of audio per
    // chunk.  This will not work well for lower sample rates in the 44.1 family
    // (22.05K and 11.025K); it is unlikely that we will ever be configured to
    // deliver those rates, and if we ever do, we will need to rely on having
    // extra chunks in flight to deal with the jitter problem described above.
    mInputChunkFrames = outputSampleRate() / 100;

    // FIXME: Currently, audio flinger demands an input buffer size which is a
    // multiple of 16 audio frames.  Right now, there is no good way to
    // reconcile this with ALSA round-up behavior described above when the
    // desired sample rate is a member of the 44.1 family.  For now, we just
    // round up to the nearest multiple of 16 frames and roll the dice, but
    // someday it would be good to fix one or the other halves of the problem
    // (either ALSA or AudioFlinger)
    mInputChunkFrames = (mInputChunkFrames + 0xF) & ~0xF;

    ALOGD("AudioStreamOut::updateInputNums: chunk size %u from output rate %u\n",
        mInputChunkFrames, outputSampleRate());

    // Buffer size is just the frame size multiplied by the number of
    // frames per chunk.
    mInputBufSize = mInputChunkFrames * getBytesPerOutputFrame();

    // The nominal latency is just the duration of a chunk * the number of
    // chunks we nominally keep in flight at any given point in time.
    mInputNominalLatencyUSec = static_cast<uint32_t>(((
                    static_cast<uint64_t>(mInputChunkFrames)
                    * 1000000 * mInputNominalChunksInFlight)
                    / mInputSampleRate));

    memset(&mLocalTimeToFrames, 0, sizeof(mLocalTimeToFrames));
    mLocalTimeToFrames.a_to_b_numer = mInputSampleRate;
    mLocalTimeToFrames.a_to_b_denom = mLocalClock.getLocalFreq();
    LinearTransform::reduce(
            &mLocalTimeToFrames.a_to_b_numer,
            &mLocalTimeToFrames.a_to_b_denom);
}

void AudioStreamOut::finishedWriteOp(size_t framesWritten,
                                     bool needThrottle)
{
    assert(mLocalClock.initCheck());

    int64_t now = mLocalClock.getLocalTime();

    if (!mThrottleValid || !needThrottle) {
        mThrottleValid = true;
        mWriteStartLT  = now;
        mFramesWritten = 0;
    }

    size_t framesWrittenAppRate;
    uint32_t multiplier = getRateMultiplier();
    if (multiplier != 1) {
        // Accumulate round-off error from previous call.
        framesWritten += mFramesWrittenRemainder;
        // Scale from device sample rate to application rate.
        framesWrittenAppRate = framesWritten / multiplier;
        ALOGV("finishedWriteOp() framesWrittenAppRate = %d = %d / %d\n",
            framesWrittenAppRate, framesWritten, multiplier);
        // Save remainder for next time to prevent error accumulation.
        mFramesWrittenRemainder = framesWritten - (framesWrittenAppRate * multiplier);
    } else {
        framesWrittenAppRate = framesWritten;
    }

    mFramesWritten += framesWrittenAppRate;
    mFramesPresented += framesWrittenAppRate;
    mFramesRendered += framesWrittenAppRate;

    if (needThrottle) {
        int64_t deltaLT;
        mLocalTimeToFrames.doReverseTransform(mFramesWritten, &deltaLT);
        deltaLT += mWriteStartLT;
        deltaLT -= now;

        int64_t deltaUSec;
        mUSecToLocalTime.doReverseTransform(deltaLT, &deltaUSec);

        if (deltaUSec > 0) {
            useconds_t sleep_time;

            // We should never be a full second ahead of schedule; sanity check
            // our throttle time and cap the max sleep time at 1 second.
            if (deltaUSec > 1000000) {
                ALOGW("throttle time clipped! deltaLT = %lld deltaUSec = %lld",
                    deltaLT, deltaUSec);
                sleep_time = 1000000;
            } else {
                sleep_time = static_cast<useconds_t>(deltaUSec);
            }
            usleep(sleep_time);
        }
    }
}

static const String8 keyRouting(AudioParameter::keyRouting);
static const String8 keySupSampleRates("sup_sampling_rates");
static const String8 keySupFormats("sup_formats");
static const String8 keySupChannels("sup_channels");
status_t AudioStreamOut::setParameters(__unused struct audio_stream *stream, const char *kvpairs)
{
    AudioParameter param = AudioParameter(String8(kvpairs));
    String8 key = String8(AudioParameter::keyRouting);
    int tmpInt;

    if (param.getInt(key, tmpInt) == NO_ERROR) {
        // The audio HAL handles routing to physical devices entirely
        // internally and mostly ignores what audio flinger tells it to do.  JiC
        // there is something (now or in the future) in audio flinger which
        // cares about the routing value in a call to getParameters, we hang on
        // to the last routing value set by audio flinger so we can at least be
        // consistent when we lie to the upper levels about doing what they told
        // us to do.
        mAudioFlingerTgtDevices = static_cast<uint32_t>(tmpInt);
    }

    return NO_ERROR;
}

char* AudioStreamOut::getParameters(const char* k)
{
    AudioParameter param = AudioParameter(String8(k));
    String8 value;

    if (param.get(keyRouting, value) == NO_ERROR) {
        param.addInt(keyRouting, (int)mAudioFlingerTgtDevices);
    }

    HDMIAudioCaps& hdmiCaps = mOwnerHAL.getHDMIAudioCaps();

    if (param.get(keySupSampleRates, value) == NO_ERROR) {
        if (mIsMCOutput) {
            hdmiCaps.getRatesForAF(value);
            param.add(keySupSampleRates, value);
        } else {
            param.add(keySupSampleRates, String8("48000"));
        }
    }

    if (param.get(keySupFormats, value) == NO_ERROR) {
        if (mIsMCOutput) {
            hdmiCaps.getFmtsForAF(value);
            param.add(keySupFormats, value);
        } else {
            param.add(keySupFormats, String8("AUDIO_FORMAT_PCM_16_BIT"));
        }
    }

    if (param.get(keySupChannels, value) == NO_ERROR) {
        if (mIsMCOutput) {
            hdmiCaps.getChannelMasksForAF(value);
            param.add(keySupChannels, value);
        } else {
            param.add(keySupChannels, String8("AUDIO_CHANNEL_OUT_STEREO"));
        }
    }

    return strdup(param.toString().string());
}

uint32_t AudioStreamOut::getRateMultiplier() const
{
    return (mIsEncoded) ? mSPDIFEncoder.getRateMultiplier() : 1;
}

uint32_t AudioStreamOut::outputSampleRate() const
{
    return mInputSampleRate * getRateMultiplier();
}

int AudioStreamOut::getBytesPerOutputFrame()
{
    return (mIsEncoded) ? mSPDIFEncoder.getBytesPerOutputFrame()
        : (mInputChanCount * sizeof(int16_t));
}

uint32_t AudioStreamOut::latency() const {
    uint32_t uSecLatency = mInputNominalLatencyUSec;
    uint32_t vcompDelay = mOwnerHAL.getVideoDelayCompUsec();

    if (uSecLatency < vcompDelay)
        return 0;

    return ((uSecLatency - vcompDelay) / 1000);
}

// Used to implement get_presentation_position() for Audio HAL.
// According to the prototype in audio.h, the frame count should not get
// reset on standby().
status_t AudioStreamOut::getPresentationPosition(uint64_t *frames,
        struct timespec *timestamp)
{
    Mutex::Autolock _l(mRoutingLock);
    status_t result = -ENODEV;
    // The presentation timestamp should be the same for all devices.
    // Also Molly only has one output device at the moment.
    // So just use the first one in the list.
    if (!mPhysOutputs.isEmpty()) {
        const unsigned int kInsaneAvail = 10 * 48000;
        unsigned int avail = 0;
        sp<AudioOutput> audioOutput = mPhysOutputs.itemAt(0);
        if (audioOutput->getHardwareTimestamp(&avail, timestamp) == 0) {
            if (avail < kInsaneAvail) {
                // FIXME av sync fudge factor
                // Use a fudge factor to account for hidden buffering in the
                // HDMI output path. This is a hack until we can determine the
                // actual buffer sizes.
                // Increasing kFudgeMSec will move the audio earlier in
                // relation to the video.
                const int kFudgeMSec = 50;
                int fudgeFrames = kFudgeMSec * sampleRate() / 1000;

                // Scale the frames in the driver because it might be running at
                // a higher rate for EAC3.
                int64_t framesInDriverBuffer =
                    (int64_t)audioOutput->getKernelBufferSize() - (int64_t)avail;
                framesInDriverBuffer = framesInDriverBuffer / getRateMultiplier();

                int64_t pendingFrames = framesInDriverBuffer + fudgeFrames;
                int64_t signedFrames = mFramesPresented - pendingFrames;
                if (pendingFrames < 0) {
                    ALOGE("getPresentationPosition: negative pendingFrames = %lld",
                        pendingFrames);
                } else if (signedFrames < 0) {
                    ALOGI("getPresentationPosition: playing silent preroll"
                        ", mFramesPresented = %llu, pendingFrames = %lld",
                        mFramesPresented, pendingFrames);
                } else {
#if HAL_PRINT_TIMESTAMP_CSV
                    // Print comma separated values for spreadsheet analysis.
                    uint64_t nanos = (((uint64_t)timestamp->tv_sec) * 1000000000L)
                            + timestamp->tv_nsec;
                    ALOGI("getPresentationPosition, %lld, %4u, %lld, %llu",
                            mFramesPresented, avail, signedFrames, nanos);
#endif
                    *frames = (uint64_t) signedFrames;
                    result = NO_ERROR;
                }
            } else {
                ALOGE("getPresentationPosition: avail too large = %u", avail);
            }
        } else {
            ALOGE("getPresentationPosition: getHardwareTimestamp returned non-zero");
        }
    } else {
        ALOGVV("getPresentationPosition: no physical outputs! This HAL is inactive!");
    }
    return result;
}

status_t AudioStreamOut::getRenderPosition(__unused uint32_t *dspFrames)
{
    if (dspFrames == NULL) {
        return -EINVAL;
    }
    if (mPhysOutputs.isEmpty()) {
        *dspFrames = 0;
        return -ENODEV;
    }
    *dspFrames = (uint32_t) mFramesRendered;
    return NO_ERROR;
}

void AudioStreamOut::updateTargetOutputs()
{
    Mutex::Autolock _l(mRoutingLock);

    AudioOutputList::iterator I;
    uint32_t cur_outputs = 0;

    for (I = mPhysOutputs.begin(); I != mPhysOutputs.end(); ++I)
        cur_outputs |= (*I)->devMask();

    if (cur_outputs == mTgtDevices)
        return;

    uint32_t outputsToObtain  = mTgtDevices & ~cur_outputs;
    uint32_t outputsToRelease = cur_outputs & ~mTgtDevices;

    // Start by releasing any outputs we should no longer have back to the HAL.
    if (outputsToRelease) {

        I = mPhysOutputs.begin();
        while (I != mPhysOutputs.end()) {
            if (!(outputsToRelease & (*I)->devMask())) {
                ++I;
                continue;
            }

            outputsToRelease &= ~((*I)->devMask());
            mOwnerHAL.releaseOutput(*this, *I);
            I = mPhysOutputs.erase(I);
        }
    }

    if (outputsToRelease) {
        ALOGW("Bookkeeping error!  Still have outputs to release (%08x), but"
              " none of them appear to be in the mPhysOutputs list!",
              outputsToRelease);
    }

    // Now attempt to obtain any outputs we should be using, but are not
    // currently.
    if (outputsToObtain) {
        uint32_t mask;

        // Buffer configuration may need updating now that we have decoded
        // the start of a stream. For example, EAC3, needs 4X sampleRate.
        updateInputNums();

        for (mask = 0x1; outputsToObtain; mask <<= 1) {
            if (!(mask & outputsToObtain))
                continue;

            sp<AudioOutput> newOutput;
            status_t res;

            res = mOwnerHAL.obtainOutput(*this, mask, &newOutput);
            outputsToObtain &= ~mask;

            if (OK != res) {
                // If we get an error back from obtain output, it means that
                // something went really wrong at a lower level (probably failed
                // to open the driver).  We should not try to obtain this output
                // again, at least until the next routing change.
                ALOGW("Failed to obtain output %08x for %s audio stream out."
                      " (res %d)", mask, getName(), res);
                mTgtDevices &= ~mask;
                continue;
            }

            if (newOutput != NULL) {
                // If we actually got an output, go ahead and add it to our list
                // of physical outputs.  The rest of the system will handle
                // starting it up.  If we didn't get an output, but also go no
                // error code, it just means that the output is currently busy
                // and should become available soon.
                ALOGI("updateTargetOutputs: adding output back to mPhysOutputs");
                mPhysOutputs.push_back(newOutput);
            }
        }
    }
}

void AudioStreamOut::adjustOutputs(int64_t maxTime)
{
    int64_t a_zero_original = mLocalTimeToFrames.a_zero;
    int64_t b_zero_original = mLocalTimeToFrames.b_zero;
    AudioOutputList::iterator I;

    // Check to see if any outputs are active and see what their buffer levels
    // are.
    for (I = mPhysOutputs.begin(); I != mPhysOutputs.end(); ++I) {
        if ((*I)->getState() == AudioOutput::DMA_START) {
            int64_t lastWriteTS = (*I)->getLastNextWriteTS();
            int64_t padAmt;

            mLocalTimeToFrames.a_zero = lastWriteTS;
            mLocalTimeToFrames.b_zero = 0;
            if (mLocalTimeToFrames.doForwardTransform(maxTime,
                                                      &padAmt)) {
                (*I)->adjustDelay(((int32_t)padAmt));
            }
        }
    }
    // Restore original offset so that the sleep time calculation for
    // throttling is not broken in finishedWriteOp().
    mLocalTimeToFrames.a_zero = a_zero_original;
    mLocalTimeToFrames.b_zero = b_zero_original;
}

ssize_t AudioStreamOut::write(const void* buffer, size_t bytes)
{
    uint8_t *data = (uint8_t *)buffer;
    ALOGVV("AudioStreamOut::write(%u)   0x%02X, 0x%02X, 0x%02X, 0x%02X,"
          " 0x%02X, 0x%02X, 0x%02X, 0x%02X,"
          " 0x%02X, 0x%02X, 0x%02X, 0x%02X,"
          " 0x%02X, 0x%02X, 0x%02X, 0x%02X ====",
        bytes, data[0], data[1], data[2], data[3],
        data[4], data[5], data[6], data[7],
        data[8], data[9], data[10], data[11],
        data[12], data[13], data[14], data[15]
        );
    if (mIsEncoded) {
        return mSPDIFEncoder.write(buffer, bytes);
    } else {
        return writeInternal(buffer, bytes);
    }
}

ssize_t AudioStreamOut::writeInternal(const void* buffer, size_t bytes)
{
    uint8_t *data = (uint8_t *)buffer;
    ALOGVV("AudioStreamOut::write_l(%u) 0x%02X, 0x%02X, 0x%02X, 0x%02X,"
          " 0x%02X, 0x%02X, 0x%02X, 0x%02X,"
          " 0x%02X, 0x%02X, 0x%02X, 0x%02X,"
          " 0x%02X, 0x%02X, 0x%02X, 0x%02X",
        bytes, data[0], data[1], data[2], data[3],
        data[4], data[5], data[6], data[7],
        data[8], data[9], data[10], data[11],
        data[12], data[13], data[14], data[15]
        );

    // Note: no lock is obtained here.  Calls to write and getNextWriteTimestamp
    // happen only on the AudioFlinger mixer thread which owns this particular
    // output stream, so there is no need to worry that there will be two
    // threads in this instance method concurrently.
    //
    // In addition, only calls to write change the contents of the mPhysOutputs
    // collection (during the call to updateTargetOutputs).  updateTargetOutputs
    // will hold the routing lock during the operation, as should any reader of
    // mPhysOutputs, unless the reader is a call to write or
    // getNextWriteTimestamp (we know that it is safe for write and gnwt to read
    // the collection because the only collection mutator is the same thread
    // which calls write and gnwt).

    // If the stream is in standby, then the first write should bring it out
    // of standby
    if (mInStandby) {
        mOwnerHAL.standbyStatusUpdate(false, mIsMCOutput);
        mInStandby = false;
    }

    updateTargetOutputs();

    // If any of our outputs is in the PRIMED state when ::write is called, it
    // means one of two things.  First, it could be that the DMA output really
    // has not started yet.  This is odd, but certainly not impossible.  The
    // other possibility is that AudioFlinger is in its silence-pushing mode and
    // is not calling getNextWriteTimestamp.  After an output is primed, its in
    // GNWTS where the amt of padding to compensate for different DMA start
    // times is taken into account.  Go ahead and force a call to GNWTS, just to
    // be certain that we have checked recently and are not stuck in silence
    // fill mode.  Failure to do this will cause the AudioOutput state machine
    // to eventually give up on DMA starting and reset the output over and over
    // again (spamming the log and producing general confusion).
    //
    // While we are in the process of checking our various output states, check
    // to see if any outputs have made it to the ACTIVE state.  Pass this
    // information along to the call to processOneChunk.  If any of our outputs
    // are waiting to be primed while other outputs have made it to steady
    // state, we need to change our priming behavior slightly.  Instead of
    // filling an output's buffer completely, we want to fill it to slightly
    // less than full and let the adjustDelay mechanism take care of the rest.
    //
    // Failure to do this during steady state operation will almost certainly
    // lead to the new output being over-filled relative to the other outputs
    // causing it to be slightly out of sync.
    AudioOutputList::iterator I;
    bool checkDMAStart = false;
    bool hasActiveOutputs = false;
    for (I = mPhysOutputs.begin(); I != mPhysOutputs.end(); ++I) {
        if (AudioOutput::PRIMED == (*I)->getState())
            checkDMAStart = true;

        if ((*I)->getState() == AudioOutput::ACTIVE)
            hasActiveOutputs = true;
    }

    if (checkDMAStart) {
        int64_t junk;
        getNextWriteTimestamp_internal(&junk);
    }

    // We always call processOneChunk on the outputs, as it is the
    // tick for their state machines.
    for (I = mPhysOutputs.begin(); I != mPhysOutputs.end(); ++I) {
        (*I)->processOneChunk((uint8_t *)buffer, bytes, hasActiveOutputs);
    }

    // If we don't actually have any physical outputs to write to, just sleep
    // for the proper amt of time in order to simulate the throttle that writing
    // to the hardware would impose.
    finishedWriteOp(bytes / getBytesPerOutputFrame(), (0 == mPhysOutputs.size()));

    return static_cast<ssize_t>(bytes);
}

status_t AudioStreamOut::getNextWriteTimestamp(int64_t *timestamp)
{
    return getNextWriteTimestamp_internal(timestamp);
}

status_t AudioStreamOut::getNextWriteTimestamp_internal(
        int64_t *timestamp)
{
    int64_t max_time = LLONG_MIN;
    bool    max_time_valid = false;
    bool    need_adjust = false;

    // Across all of our physical outputs, figure out the max time when
    // a write operation will hit the speakers.  Assume that if an
    // output cannot answer the question, its because it has never
    // started or because it has recently underflowed and needs to be
    // restarted.  If this is the case, we will need to prime the
    // pipeline with a chunk's worth of data before proceeding.
    // If any of the outputs indicate a discontinuity (meaning that the
    // DMA start time was valid and is now invalid, or was and is valid
    // but was different from before; almost certainly caused by a low
    // level underfow), then just stop now.  We will need to reset and
    // re-prime all of the outputs in order to make certain that the
    // lead-times on all of the outputs match.

    AudioOutputList::iterator I;
    bool discon = false;

    // Find the largest next write timestamp. The goal is to make EVERY
    // output have the same value, but we also need this to pass back
    // up the layers.
    for (I = mPhysOutputs.begin(); I != mPhysOutputs.end(); ++I) {
        int64_t tmp;
        if (OK == (*I)->getNextWriteTimestamp(&tmp, &discon)) {
            if (!max_time_valid || (max_time < tmp)) {
                max_time = tmp;
                max_time_valid = true;
            }
        }
    }

    // Check the state of each output and determine if we need to align them.
    // Make sure to do this after we have called each outputs'
    // getNextWriteTimestamp as the transition from PRIMED to DMA_START happens
    // there.
    for (I = mPhysOutputs.begin(); I != mPhysOutputs.end(); ++I) {
        if ((*I)->getState() == AudioOutput::DMA_START) {
            need_adjust = true;
            break;
        }
    }

    // At this point, if we still have not found at least one output
    // who knows when their data is going to hit the speakers, then we
    // just can't answer the getNextWriteTimestamp question and we
    // should give up.
    if (!max_time_valid) {
        return INVALID_OPERATION;
    }

    // Stuff silence into the non-aligned outputs so that the effective
    // timestamp is the same for all the outputs.
    if (need_adjust)
        adjustOutputs(max_time);

    // We are done. The time at which the next written audio should
    // hit the speakers is just max_time plus the maximum amt of delay
    // compensation in the system.
    *timestamp = max_time;
    return OK;
}

#define DUMP(a...) \
    snprintf(buffer, SIZE, a); \
    buffer[SIZE - 1] = 0; \
    result.append(buffer);
#define B2STR(b) b ? "true" : "false"

status_t AudioStreamOut::dump(int fd)
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;
    DUMP("\n%s AudioStreamOut::dump\n", getName());
    DUMP("\tsample rate            : %d\n", sampleRate());
    DUMP("\tbuffer size            : %d\n", bufferSize());
    DUMP("\tchannel mask           : 0x%04x\n", chanMask());
    DUMP("\tformat                 : %d\n", format());
    DUMP("\tdevice mask            : 0x%04x\n", mTgtDevices);
    DUMP("\tIn standby             : %s\n", mInStandby? "yes" : "no");

    mRoutingLock.lock();
    AudioOutputList outSnapshot(mPhysOutputs);
    mRoutingLock.unlock();

    AudioOutputList::iterator I;
    for (I = outSnapshot.begin(); I != outSnapshot.end(); ++I)
        (*I)->dump(result);

    ::write(fd, result.string(), result.size());

    return NO_ERROR;
}

#undef B2STR
#undef DUMP

}  // android
