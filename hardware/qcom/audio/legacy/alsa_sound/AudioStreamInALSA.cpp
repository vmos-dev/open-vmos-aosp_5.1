/* AudioStreamInALSA.cpp
 **
 ** Copyright 2008-2009 Wind River Systems
 ** Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
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

#include <errno.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>

#define LOG_TAG "AudioStreamInALSA"
//#define LOG_NDEBUG 0
#define LOG_NDDEBUG 0
#include <utils/Log.h>
#include <utils/String8.h>

#include <cutils/properties.h>
#include <media/AudioRecord.h>
#include <hardware_legacy/power.h>

#include "AudioHardwareALSA.h"

extern "C" {
#ifdef QCOM_CSDCLIENT_ENABLED
static int (*csd_start_record)(int);
static int (*csd_stop_record)(void);
#endif

#ifdef QCOM_SSR_ENABLED
#include "surround_filters_interface.h"
#endif
}

namespace android_audio_legacy
{
#ifdef QCOM_SSR_ENABLED
#define SURROUND_FILE_1R "/system/etc/surround_sound/filter1r.pcm"
#define SURROUND_FILE_2R "/system/etc/surround_sound/filter2r.pcm"
#define SURROUND_FILE_3R "/system/etc/surround_sound/filter3r.pcm"
#define SURROUND_FILE_4R "/system/etc/surround_sound/filter4r.pcm"

#define SURROUND_FILE_1I "/system/etc/surround_sound/filter1i.pcm"
#define SURROUND_FILE_2I "/system/etc/surround_sound/filter2i.pcm"
#define SURROUND_FILE_3I "/system/etc/surround_sound/filter3i.pcm"
#define SURROUND_FILE_4I "/system/etc/surround_sound/filter4i.pcm"

// Use AAC/DTS channel mapping as default channel mapping: C,FL,FR,Ls,Rs,LFE
const int chanMap[] = { 1, 2, 4, 3, 0, 5 };
#endif

AudioStreamInALSA::AudioStreamInALSA(AudioHardwareALSA *parent,
        alsa_handle_t *handle,
        AudioSystem::audio_in_acoustics audio_acoustics) :
    ALSAStreamOps(parent, handle),
    mFramesLost(0),
    mAcoustics(audio_acoustics),
    mParent(parent)
#ifdef QCOM_SSR_ENABLED
    , mFp_4ch(NULL),
    mFp_6ch(NULL),
    mRealCoeffs(NULL),
    mImagCoeffs(NULL),
    mSurroundObj(NULL),
    mSurroundOutputBuffer(NULL),
    mSurroundInputBuffer(NULL),
    mSurroundOutputBufferIdx(0),
    mSurroundInputBufferIdx(0)
#endif
{
#ifdef QCOM_SSR_ENABLED
    char c_multi_ch_dump[128] = {0};
    status_t err = NO_ERROR;

    // Call surround sound library init if device is Surround Sound
    if ( handle->channels == 6) {
        if (!strncmp(handle->useCase, SND_USE_CASE_VERB_HIFI_REC, strlen(SND_USE_CASE_VERB_HIFI_REC))
            || !strncmp(handle->useCase, SND_USE_CASE_MOD_CAPTURE_MUSIC, strlen(SND_USE_CASE_MOD_CAPTURE_MUSIC))) {

            err = initSurroundSoundLibrary(handle->bufferSize);
            if ( NO_ERROR != err) {
                ALOGE("initSurroundSoundLibrary failed: %d  handle->bufferSize:%d", err,handle->bufferSize);
            }

            property_get("ssr.pcmdump",c_multi_ch_dump,"0");
            if (0 == strncmp("true",c_multi_ch_dump, sizeof("ssr.dump-pcm"))) {
                //Remember to change file system permission of data(e.g. chmod 777 data/),
                //otherwise, fopen may fail.
                if ( !mFp_4ch)
                    mFp_4ch = fopen("/data/4ch_ssr.pcm", "wb");
                if ( !mFp_6ch)
                    mFp_6ch = fopen("/data/6ch_ssr.pcm", "wb");
                if ((!mFp_4ch) || (!mFp_6ch))
                    ALOGE("mfp_4ch or mfp_6ch open failed: mfp_4ch:%p mfp_6ch:%p",mFp_4ch,mFp_6ch);
            }
        }
    }
#endif
}

AudioStreamInALSA::~AudioStreamInALSA()
{
    close();
}

status_t AudioStreamInALSA::setGain(float gain)
{
    return 0; //mixer() ? mixer()->setMasterGain(gain) : (status_t)NO_INIT;
}

ssize_t AudioStreamInALSA::read(void *buffer, ssize_t bytes)
{
    int period_size;

    ALOGV("read:: buffer %p, bytes %d", buffer, bytes);

    int n;
    status_t          err;
    ssize_t            read = 0;
    char *use_case;
    int newMode = mParent->mode();

    if((mHandle->handle == NULL) && (mHandle->rxHandle == NULL) &&
         (strcmp(mHandle->useCase, SND_USE_CASE_VERB_IP_VOICECALL)) &&
         (strcmp(mHandle->useCase, SND_USE_CASE_MOD_PLAY_VOIP))) {
        mParent->mLock.lock();
        snd_use_case_get(mHandle->ucMgr, "_verb", (const char **)&use_case);
        if ((use_case != NULL) && (strcmp(use_case, SND_USE_CASE_VERB_INACTIVE))) {
            if ((mHandle->devices == AudioSystem::DEVICE_IN_VOICE_CALL) &&
                (newMode == AudioSystem::MODE_IN_CALL)) {
                ALOGD("read:: mParent->mIncallMode=%d", mParent->mIncallMode);
                if ((mParent->mIncallMode & AudioSystem::CHANNEL_IN_VOICE_UPLINK) &&
                    (mParent->mIncallMode & AudioSystem::CHANNEL_IN_VOICE_DNLINK)) {
#ifdef QCOM_CSDCLIENT_ENABLED
                    if (mParent->mFusion3Platform) {
                        mParent->mALSADevice->setVocRecMode(INCALL_REC_STEREO);
                        strlcpy(mHandle->useCase, SND_USE_CASE_MOD_CAPTURE_VOICE,
                                sizeof(mHandle->useCase));
                        start_csd_record(INCALL_REC_STEREO);
                    } else
#endif
                    {
                        strlcpy(mHandle->useCase, SND_USE_CASE_MOD_CAPTURE_VOICE_UL_DL,
                                sizeof(mHandle->useCase));
                    }
                } else if (mParent->mIncallMode & AudioSystem::CHANNEL_IN_VOICE_DNLINK) {
#ifdef QCOM_CSDCLIENT_ENABLED
                    if (mParent->mFusion3Platform) {
                        mParent->mALSADevice->setVocRecMode(INCALL_REC_MONO);
                        strlcpy(mHandle->useCase, SND_USE_CASE_MOD_CAPTURE_VOICE,
                                sizeof(mHandle->useCase));
                        start_csd_record(INCALL_REC_MONO);
                    } else
#endif
                    {
                        strlcpy(mHandle->useCase, SND_USE_CASE_MOD_CAPTURE_VOICE_DL,
                                sizeof(mHandle->useCase));
                    }
                }
#ifdef QCOM_FM_ENABLED
            } else if(mHandle->devices == AudioSystem::DEVICE_IN_FM_RX) {
                strlcpy(mHandle->useCase, SND_USE_CASE_MOD_CAPTURE_FM, sizeof(mHandle->useCase));
            } else if (mHandle->devices == AudioSystem::DEVICE_IN_FM_RX_A2DP) {
                strlcpy(mHandle->useCase, SND_USE_CASE_MOD_CAPTURE_A2DP_FM, sizeof(mHandle->useCase));
#endif
            } else if(!strcmp(mHandle->useCase, SND_USE_CASE_MOD_PLAY_VOIP)) {
                strlcpy(mHandle->useCase, SND_USE_CASE_MOD_PLAY_VOIP, sizeof(mHandle->useCase));
            } else {
                    char value[128];
                    property_get("persist.audio.lowlatency.rec",value,"0");
                    if (!strcmp("true", value)) {
                        strlcpy(mHandle->useCase, SND_USE_CASE_MOD_CAPTURE_LOWLATENCY_MUSIC, sizeof(mHandle->useCase));
                    } else {
                        strlcpy(mHandle->useCase, SND_USE_CASE_MOD_CAPTURE_MUSIC, sizeof(mHandle->useCase));
                    }
            }
        } else {
            if ((mHandle->devices == AudioSystem::DEVICE_IN_VOICE_CALL) &&
                (newMode == AudioSystem::MODE_IN_CALL)) {
                ALOGD("read:: ---- mParent->mIncallMode=%d", mParent->mIncallMode);
                if ((mParent->mIncallMode & AudioSystem::CHANNEL_IN_VOICE_UPLINK) &&
                    (mParent->mIncallMode & AudioSystem::CHANNEL_IN_VOICE_DNLINK)) {
#ifdef QCOM_CSDCLIENT_ENABLED
                    if (mParent->mFusion3Platform) {
                        mParent->mALSADevice->setVocRecMode(INCALL_REC_STEREO);
                        strlcpy(mHandle->useCase, SND_USE_CASE_VERB_INCALL_REC,
                                sizeof(mHandle->useCase));
                        start_csd_record(INCALL_REC_STEREO);
                    } else
#endif
                    {
                        strlcpy(mHandle->useCase, SND_USE_CASE_VERB_UL_DL_REC,
                                sizeof(mHandle->useCase));
                    }
                } else if (mParent->mIncallMode & AudioSystem::CHANNEL_IN_VOICE_DNLINK) {
#ifdef QCOM_CSDCLIENT_ENABLED
                   if (mParent->mFusion3Platform) {
                       mParent->mALSADevice->setVocRecMode(INCALL_REC_MONO);
                       strlcpy(mHandle->useCase, SND_USE_CASE_VERB_INCALL_REC,
                               sizeof(mHandle->useCase));
                       start_csd_record(INCALL_REC_MONO);
                   } else
#endif
                   {
                       strlcpy(mHandle->useCase, SND_USE_CASE_VERB_DL_REC,
                               sizeof(mHandle->useCase));
                   }
                }
#ifdef QCOM_FM_ENABLED
            } else if(mHandle->devices == AudioSystem::DEVICE_IN_FM_RX) {
                strlcpy(mHandle->useCase, SND_USE_CASE_VERB_FM_REC, sizeof(mHandle->useCase));
        } else if (mHandle->devices == AudioSystem::DEVICE_IN_FM_RX_A2DP) {
                strlcpy(mHandle->useCase, SND_USE_CASE_VERB_FM_A2DP_REC, sizeof(mHandle->useCase));
#endif
            } else if(!strcmp(mHandle->useCase, SND_USE_CASE_VERB_IP_VOICECALL)){
                    strlcpy(mHandle->useCase, SND_USE_CASE_VERB_IP_VOICECALL, sizeof(mHandle->useCase));
            } else {
                    char value[128];
                    property_get("persist.audio.lowlatency.rec",value,"0");
                    if (!strcmp("true", value)) {
                        strlcpy(mHandle->useCase, SND_USE_CASE_VERB_HIFI_LOWLATENCY_REC, sizeof(mHandle->useCase));
                    } else {
                        strlcpy(mHandle->useCase, SND_USE_CASE_VERB_HIFI_REC, sizeof(mHandle->useCase));
                    }
            }
        }
        if (mHandle->channelMask == AUDIO_CHANNEL_IN_FRONT_BACK) {
            mHandle->module->setFlags(mParent->mDevSettingsFlag | DMIC_FLAG);
        }
        free(use_case);
        if((!strcmp(mHandle->useCase, SND_USE_CASE_VERB_IP_VOICECALL)) ||
            (!strcmp(mHandle->useCase, SND_USE_CASE_MOD_PLAY_VOIP))) {
#ifdef QCOM_USBAUDIO_ENABLED
            if((mDevices & AudioSystem::DEVICE_IN_ANLG_DOCK_HEADSET) ||
               (mDevices & AudioSystem::DEVICE_OUT_ANLG_DOCK_HEADSET)) {
                mHandle->module->route(mHandle, (mDevices | AudioSystem::DEVICE_IN_PROXY) , AudioSystem::MODE_IN_COMMUNICATION);
            }else
#endif
            {
                mHandle->module->route(mHandle, mDevices , AudioSystem::MODE_IN_COMMUNICATION);
            }
        } else {
#ifdef QCOM_USBAUDIO_ENABLED
            if((mHandle->devices == AudioSystem::DEVICE_IN_ANLG_DOCK_HEADSET)||
               (mHandle->devices == AudioSystem::DEVICE_OUT_ANLG_DOCK_HEADSET)){
                mHandle->module->route(mHandle, AudioSystem::DEVICE_IN_PROXY , mParent->mode());
            } else
#endif
            {

                mHandle->module->route(mHandle, mDevices , mParent->mode());
            }
        }
        if (!strcmp(mHandle->useCase, SND_USE_CASE_VERB_HIFI_REC) ||
            !strcmp(mHandle->useCase, SND_USE_CASE_VERB_HIFI_LOWLATENCY_REC) ||
            !strcmp(mHandle->useCase, SND_USE_CASE_VERB_FM_REC) ||
            !strcmp(mHandle->useCase, SND_USE_CASE_VERB_IP_VOICECALL) ||
            !strcmp(mHandle->useCase, SND_USE_CASE_VERB_FM_A2DP_REC) ||
            !strcmp(mHandle->useCase, SND_USE_CASE_VERB_UL_DL_REC) ||
            !strcmp(mHandle->useCase, SND_USE_CASE_VERB_DL_REC) ||
            !strcmp(mHandle->useCase, SND_USE_CASE_VERB_INCALL_REC)) {
            snd_use_case_set(mHandle->ucMgr, "_verb", mHandle->useCase);
        } else {
            snd_use_case_set(mHandle->ucMgr, "_enamod", mHandle->useCase);
        }
       if((!strcmp(mHandle->useCase, SND_USE_CASE_VERB_IP_VOICECALL)) ||
           (!strcmp(mHandle->useCase, SND_USE_CASE_MOD_PLAY_VOIP))) {
            err = mHandle->module->startVoipCall(mHandle);
        }
        else
            mHandle->module->open(mHandle);
        if(mHandle->handle == NULL) {
            ALOGE("read:: PCM device open failed");
            mParent->mLock.unlock();

            return 0;
        }
#ifdef QCOM_USBAUDIO_ENABLED
        if((mHandle->devices == AudioSystem::DEVICE_IN_ANLG_DOCK_HEADSET)||
           (mHandle->devices == AudioSystem::DEVICE_OUT_ANLG_DOCK_HEADSET)){
            if((!strcmp(mHandle->useCase, SND_USE_CASE_VERB_IP_VOICECALL)) ||
               (!strcmp(mHandle->useCase, SND_USE_CASE_MOD_PLAY_VOIP))) {
                mParent->musbRecordingState |= USBRECBIT_VOIPCALL;
            } else {
                mParent->startUsbRecordingIfNotStarted();
                mParent->musbRecordingState |= USBRECBIT_REC;
            }
        }
#endif
        mParent->mLock.unlock();
    }
#ifdef QCOM_USBAUDIO_ENABLED
    if(((mDevices & AudioSystem::DEVICE_IN_ANLG_DOCK_HEADSET) ||
       (mDevices & AudioSystem::DEVICE_OUT_ANLG_DOCK_HEADSET)) &&
       (!mParent->musbRecordingState)) {
        mParent->mLock.lock();
        ALOGD("Starting UsbRecording thread");
        mParent->startUsbRecordingIfNotStarted();
        if(!strcmp(mHandle->useCase, SND_USE_CASE_VERB_IP_VOICECALL) ||
           !strcmp(mHandle->useCase, SND_USE_CASE_MOD_PLAY_VOIP)) {
            ALOGD("Enabling voip recording bit");
            mParent->musbRecordingState |= USBRECBIT_VOIPCALL;
        }else{
            ALOGD("Enabling HiFi Recording bit");
            mParent->musbRecordingState |= USBRECBIT_REC;
        }
        mParent->mLock.unlock();
    }
#endif
    period_size = mHandle->periodSize;
    int read_pending = bytes;

#ifdef QCOM_SSR_ENABLED
    if (mSurroundObj) {
        int processed = 0;
        int processed_pending;
        int samples = bytes >> 1;
        void *buffer_start = buffer;
        int period_bytes = mHandle->handle->period_size;
        int period_samples = period_bytes >> 1;

        do {
            if (mSurroundOutputBufferIdx > 0) {
                ALOGV("AudioStreamInALSA::read() - copy processed output "
                     "to buffer, mSurroundOutputBufferIdx = %d",
                     mSurroundOutputBufferIdx);
                // Copy processed output to buffer
                processed_pending = mSurroundOutputBufferIdx;
                if (processed_pending > (samples - processed)) {
                    processed_pending = (samples - processed);
                }
                memcpy(buffer, mSurroundOutputBuffer, processed_pending * sizeof(Word16));
                buffer += processed_pending * sizeof(Word16);
                processed += processed_pending;
                if (mSurroundOutputBufferIdx > processed_pending) {
                    // Shift leftover samples to beginning of the buffer
                    memcpy(&mSurroundOutputBuffer[0],
                           &mSurroundOutputBuffer[processed_pending],
                           (mSurroundOutputBufferIdx - processed_pending) * sizeof(Word16));
                }
                mSurroundOutputBufferIdx -= processed_pending;
            }

            if (processed >= samples) {
                ALOGV("AudioStreamInALSA::read() - done processing buffer, "
                     "processed = %d", processed);
                // Done processing this buffer
                break;
            }

            // Fill input buffer until there is enough to process
            read_pending = SSR_INPUT_FRAME_SIZE - mSurroundInputBufferIdx;
            read = mSurroundInputBufferIdx;
            while (mHandle->handle && read_pending > 0) {
                n = pcm_read(mHandle->handle, &mSurroundInputBuffer[read],
                             period_bytes);
                ALOGV("pcm_read() returned n = %d buffer:%p size:%d", n, &mSurroundInputBuffer[read], period_bytes);
                if (n && n != -EAGAIN) {
                    //Recovery part of pcm_read. TODO:split recovery.
                    return static_cast<ssize_t>(n);
                }
                else if (n < 0) {
                    // Recovery is part of pcm_write. TODO split is later.
                    return static_cast<ssize_t>(n);
                }
                else {
                    read_pending -= period_samples;
                    read += period_samples;
                }
            }


            if (mFp_4ch) {
                fwrite( mSurroundInputBuffer, 1,
                        SSR_INPUT_FRAME_SIZE * sizeof(Word16), mFp_4ch);
            }

            //apply ssr libs to conver 4ch to 6ch
            surround_filters_intl_process(mSurroundObj,
                &mSurroundOutputBuffer[mSurroundOutputBufferIdx],
                (Word16 *)mSurroundInputBuffer);

            // Shift leftover samples to beginning of input buffer
            if (read_pending < 0) {
                memcpy(&mSurroundInputBuffer[0],
                       &mSurroundInputBuffer[SSR_INPUT_FRAME_SIZE],
                       (-read_pending) * sizeof(Word16));
            }
            mSurroundInputBufferIdx = -read_pending;

            if (mFp_6ch) {
                fwrite( &mSurroundOutputBuffer[mSurroundOutputBufferIdx],
                        1, SSR_OUTPUT_FRAME_SIZE * sizeof(Word16), mFp_6ch);
            }

            mSurroundOutputBufferIdx += SSR_OUTPUT_FRAME_SIZE;
            ALOGV("do_while loop: processed=%d, samples=%d\n", processed, samples);
        } while (mHandle->handle && processed < samples);
        read = processed * sizeof(Word16);
        buffer = buffer_start;
    } else
#endif
    {

        do {
            if (read_pending < period_size) {
                read_pending = period_size;
            }

            n = pcm_read(mHandle->handle, buffer,
                period_size);
            ALOGV("pcm_read() returned n = %d", n);
            if (n && (n == -EIO || n == -EAGAIN || n == -EPIPE || n == -EBADFD)) {
                mParent->mLock.lock();
                ALOGW("pcm_read() returned error n %d, Recovering from error\n", n);
                pcm_close(mHandle->handle);
                mHandle->handle = NULL;
                if((!strncmp(mHandle->useCase, SND_USE_CASE_VERB_IP_VOICECALL, strlen(SND_USE_CASE_VERB_IP_VOICECALL))) ||
                (!strncmp(mHandle->useCase, SND_USE_CASE_MOD_PLAY_VOIP, strlen(SND_USE_CASE_MOD_PLAY_VOIP)))) {
                    pcm_close(mHandle->rxHandle);
                    mHandle->rxHandle = NULL;
                    mHandle->module->startVoipCall(mHandle);
                }
                else
                    mHandle->module->open(mHandle);

                if(mHandle->handle == NULL) {
                   ALOGE("read:: PCM device re-open failed");
                   mParent->mLock.unlock();
                   return 0;
                }

                mParent->mLock.unlock();
                continue;
            }
            else if (n < 0) {
                ALOGD("pcm_read() returned n < 0");
                return static_cast<ssize_t>(n);
            }
            else {
                read += static_cast<ssize_t>((period_size));
                read_pending -= period_size;
                //Set mute by cleanning buffers read
                if (mParent->mMicMute) {
                    memset(buffer, 0, period_size);
                }
                buffer = ((uint8_t *)buffer) + period_size;
            }

        } while (mHandle->handle && read < bytes);
    }

    return read;
}

status_t AudioStreamInALSA::dump(int fd, const Vector<String16>& args)
{
    return NO_ERROR;
}

status_t AudioStreamInALSA::open(int mode)
{
    Mutex::Autolock autoLock(mParent->mLock);

    status_t status = ALSAStreamOps::open(mode);

    return status;
}

status_t AudioStreamInALSA::close()
{
    Mutex::Autolock autoLock(mParent->mLock);

    ALOGD("close");
    if((!strcmp(mHandle->useCase, SND_USE_CASE_VERB_IP_VOICECALL)) ||
        (!strcmp(mHandle->useCase, SND_USE_CASE_MOD_PLAY_VOIP))) {
        if((mParent->mVoipStreamCount)) {
#ifdef QCOM_USBAUDIO_ENABLED
            ALOGD("musbRecordingState: %d, mVoipStreamCount:%d",mParent->musbRecordingState,
                  mParent->mVoipStreamCount );
            if(mParent->mVoipStreamCount == 1) {
                ALOGD("Deregistering VOIP Call bit, musbPlaybackState:%d,"
                       "musbRecordingState:%d", mParent->musbPlaybackState, mParent->musbRecordingState);
                mParent->musbPlaybackState &= ~USBPLAYBACKBIT_VOIPCALL;
                mParent->musbRecordingState &= ~USBRECBIT_VOIPCALL;
                mParent->closeUsbRecordingIfNothingActive();
                mParent->closeUsbPlaybackIfNothingActive();
            }
#endif
               return NO_ERROR;
        }
        mParent->mVoipStreamCount = 0;
#ifdef QCOM_USBAUDIO_ENABLED
    } else {
        ALOGD("Deregistering REC bit, musbRecordingState:%d", mParent->musbRecordingState);
        mParent->musbRecordingState &= ~USBRECBIT_REC;
#endif
     }
#ifdef QCOM_CSDCLIENT_ENABLED
    if (mParent->mFusion3Platform) {
       if((!strcmp(mHandle->useCase, SND_USE_CASE_VERB_INCALL_REC)) ||
           (!strcmp(mHandle->useCase, SND_USE_CASE_MOD_CAPTURE_VOICE))) {
           stop_csd_record();
       }
    }
#endif
    ALOGD("close");
#ifdef QCOM_USBAUDIO_ENABLED
    mParent->closeUsbRecordingIfNothingActive();
#endif

    ALSAStreamOps::close();

#ifdef QCOM_SSR_ENABLED
    if (mSurroundObj) {
        surround_filters_release(mSurroundObj);
        if (mSurroundObj)
            free(mSurroundObj);
        mSurroundObj = NULL;
        if (mRealCoeffs){
            for (int i =0; i<COEFF_ARRAY_SIZE; i++ ) {
                if (mRealCoeffs[i]) {
                    free(mRealCoeffs[i]);
                    mRealCoeffs[i] = NULL;
                }
            }
            free(mRealCoeffs);
            mRealCoeffs = NULL;
        }
        if (mImagCoeffs){
            for (int i =0; i<COEFF_ARRAY_SIZE; i++ ) {
                if (mImagCoeffs[i]) {
                    free(mImagCoeffs[i]);
                    mImagCoeffs[i] = NULL;
                }
            }
            free(mImagCoeffs);
            mImagCoeffs = NULL;
        }
        if (mSurroundOutputBuffer){
            free(mSurroundOutputBuffer);
            mSurroundOutputBuffer = NULL;
        }
        if (mSurroundInputBuffer) {
            free(mSurroundInputBuffer);
            mSurroundInputBuffer = NULL;
        }

        if ( mFp_4ch ) fclose(mFp_4ch);
        if ( mFp_6ch ) fclose(mFp_6ch);

    }
#endif

    return NO_ERROR;
}

status_t AudioStreamInALSA::standby()
{
    Mutex::Autolock autoLock(mParent->mLock);

    ALOGD("standby");

    if((!strcmp(mHandle->useCase, SND_USE_CASE_VERB_IP_VOICECALL)) ||
        (!strcmp(mHandle->useCase, SND_USE_CASE_MOD_PLAY_VOIP))) {
         return NO_ERROR;
    }

#ifdef QCOM_CSDCLIENT_ENABLED
    ALOGD("standby");
    if (mParent->mFusion3Platform) {
       if((!strcmp(mHandle->useCase, SND_USE_CASE_VERB_INCALL_REC)) ||
           (!strcmp(mHandle->useCase, SND_USE_CASE_MOD_CAPTURE_VOICE))) {
           ALOGD(" into standby, stop record");
           stop_csd_record();
       }
    }
#endif
    mHandle->module->standby(mHandle);

#ifdef QCOM_USBAUDIO_ENABLED
    ALOGD("Checking for musbRecordingState %d", mParent->musbRecordingState);
    mParent->musbRecordingState &= ~USBRECBIT_REC;
    mParent->closeUsbRecordingIfNothingActive();
#endif

    if (mHandle->channelMask == AUDIO_CHANNEL_IN_FRONT_BACK) {
        mHandle->module->setFlags(mParent->mDevSettingsFlag);
    }

    return NO_ERROR;
}

void AudioStreamInALSA::resetFramesLost()
{
    mFramesLost = 0;
}

unsigned int AudioStreamInALSA::getInputFramesLost() const
{
    unsigned int count = mFramesLost;
    // Stupid interface wants us to have a side effect of clearing the count
    // but is defined as a const to prevent such a thing.
    ((AudioStreamInALSA *)this)->resetFramesLost();
    return count;
}

status_t AudioStreamInALSA::setAcousticParams(void *params)
{
    Mutex::Autolock autoLock(mParent->mLock);

    return (status_t)NO_ERROR;
}

#ifdef QCOM_SSR_ENABLED
status_t AudioStreamInALSA::initSurroundSoundLibrary(unsigned long buffersize)
{
    int subwoofer = 0;  // subwoofer channel assignment: default as first microphone input channel
    int low_freq = 4;   // frequency upper bound for subwoofer: frequency=(low_freq-1)/FFT_SIZE*samplingRate, default as 4
    int high_freq = 100;    // frequency upper bound for spatial processing: frequency=(high_freq-1)/FFT_SIZE*samplingRate, default as 100
    int ret = 0;

    mSurroundInputBufferIdx = 0;
    mSurroundOutputBufferIdx = 0;

    if ( mSurroundObj ) {
        ALOGE("ola filter library is already initialized");
        return ALREADY_EXISTS;
    }

    // Allocate memory for input buffer
    mSurroundInputBuffer = (Word16 *) calloc(2 * SSR_INPUT_FRAME_SIZE,
                                              sizeof(Word16));
    if ( !mSurroundInputBuffer ) {
       ALOGE("Memory allocation failure. Not able to allocate memory for surroundInputBuffer");
       goto init_fail;
    }

    // Allocate memory for output buffer
    mSurroundOutputBuffer = (Word16 *) calloc(2 * SSR_OUTPUT_FRAME_SIZE,
                                               sizeof(Word16));
    if ( !mSurroundOutputBuffer ) {
       ALOGE("Memory allocation failure. Not able to allocate memory for surroundOutputBuffer");
       goto init_fail;
    }

    // Allocate memory for real and imag coeffs array
    mRealCoeffs = (Word16 **) calloc(COEFF_ARRAY_SIZE, sizeof(Word16 *));
    if ( !mRealCoeffs ) {
        ALOGE("Memory allocation failure during real Coefficient array");
        goto init_fail;
    }

    mImagCoeffs = (Word16 **) calloc(COEFF_ARRAY_SIZE, sizeof(Word16 *));
    if ( !mImagCoeffs ) {
        ALOGE("Memory allocation failure during imaginary Coefficient array");
        goto init_fail;
    }

    if( readCoeffsFromFile() != NO_ERROR) {
        ALOGE("Error while loading coeffs from file");
        goto init_fail;
    }

    //calculate the size of data to allocate for mSurroundObj
    ret = surround_filters_init(NULL,
                  6, // Num output channel
                  4,     // Num input channel
                  mRealCoeffs,       // Coeffs hardcoded in header
                  mImagCoeffs,       // Coeffs hardcoded in header
                  subwoofer,
                  low_freq,
                  high_freq,
                  NULL);

    if ( ret > 0 ) {
        ALOGV("Allocating surroundObj size is %d", ret);
        mSurroundObj = (void *)malloc(ret);
        memset(mSurroundObj,0,ret);
        if (NULL != mSurroundObj) {
            //initialize after allocating the memory for mSurroundObj
            ret = surround_filters_init(mSurroundObj,
                        6,
                        4,
                        mRealCoeffs,
                        mImagCoeffs,
                        subwoofer,
                        low_freq,
                        high_freq,
                        NULL);
            if (0 != ret) {
               ALOGE("surround_filters_init failed with ret:%d",ret);
               surround_filters_release(mSurroundObj);
               goto init_fail;
            }
        } else {
            ALOGE("Allocationg mSurroundObj failed");
            goto init_fail;
        }
    } else {
        ALOGE("surround_filters_init(mSurroundObj=Null) failed with ret: %d",ret);
        goto init_fail;
    }

    (void) surround_filters_set_channel_map(mSurroundObj, chanMap);

    return NO_ERROR;

init_fail:
    if (mSurroundObj) {
        free(mSurroundObj);
        mSurroundObj = NULL;
    }
    if (mSurroundOutputBuffer) {
        free(mSurroundOutputBuffer);
        mSurroundOutputBuffer = NULL;
    }
    if (mSurroundInputBuffer) {
        free(mSurroundInputBuffer);
        mSurroundInputBuffer = NULL;
    }
    if (mRealCoeffs){
        for (int i =0; i<COEFF_ARRAY_SIZE; i++ ) {
            if (mRealCoeffs[i]) {
                free(mRealCoeffs[i]);
                mRealCoeffs[i] = NULL;
            }
        }
        free(mRealCoeffs);
        mRealCoeffs = NULL;
    }
    if (mImagCoeffs){
        for (int i =0; i<COEFF_ARRAY_SIZE; i++ ) {
            if (mImagCoeffs[i]) {
                free(mImagCoeffs[i]);
                mImagCoeffs[i] = NULL;
            }
        }
        free(mImagCoeffs);
        mImagCoeffs = NULL;
    }

    return NO_MEMORY;

}


// Helper function to read coeffs from File and updates real and imaginary
// coeff array member variable
status_t AudioStreamInALSA::readCoeffsFromFile()
{
    FILE    *flt1r;
    FILE    *flt2r;
    FILE    *flt3r;
    FILE    *flt4r;
    FILE    *flt1i;
    FILE    *flt2i;
    FILE    *flt3i;
    FILE    *flt4i;

    if ( (flt1r = fopen(SURROUND_FILE_1R, "rb")) == NULL ) {
        ALOGE("Cannot open filter co-efficient file %s", SURROUND_FILE_1R);
        return NAME_NOT_FOUND;
    }

    if ( (flt2r = fopen(SURROUND_FILE_2R, "rb")) == NULL ) {
        ALOGE("Cannot open filter co-efficient file %s", SURROUND_FILE_2R);
        return NAME_NOT_FOUND;
    }

    if ( (flt3r = fopen(SURROUND_FILE_3R, "rb")) == NULL ) {
        ALOGE("Cannot open filter co-efficient file %s", SURROUND_FILE_3R);
        return  NAME_NOT_FOUND;
    }

    if ( (flt4r = fopen(SURROUND_FILE_4R, "rb")) == NULL ) {
        ALOGE("Cannot open filter co-efficient file %s", SURROUND_FILE_4R);
        return  NAME_NOT_FOUND;
    }

    if ( (flt1i = fopen(SURROUND_FILE_1I, "rb")) == NULL ) {
        ALOGE("Cannot open filter co-efficient file %s", SURROUND_FILE_1I);
        return NAME_NOT_FOUND;
    }

    if ( (flt2i = fopen(SURROUND_FILE_2I, "rb")) == NULL ) {
        ALOGE("Cannot open filter co-efficient file %s", SURROUND_FILE_2I);
        return NAME_NOT_FOUND;
    }

    if ( (flt3i = fopen(SURROUND_FILE_3I, "rb")) == NULL ) {
        ALOGE("Cannot open filter co-efficient file %s", SURROUND_FILE_3I);
        return NAME_NOT_FOUND;
    }

    if ( (flt4i = fopen(SURROUND_FILE_4I, "rb")) == NULL ) {
        ALOGE("Cannot open filter co-efficient file %s", SURROUND_FILE_4I);
        return NAME_NOT_FOUND;
    }
    ALOGV("readCoeffsFromFile all filter files opened");

    for (int i=0; i<COEFF_ARRAY_SIZE; i++) {
        mRealCoeffs[i] = (Word16 *)calloc(FILT_SIZE, sizeof(Word16));
    }
    for (int i=0; i<COEFF_ARRAY_SIZE; i++) {
        mImagCoeffs[i] = (Word16 *)calloc(FILT_SIZE, sizeof(Word16));
    }

    // Read real co-efficients
    if (NULL != mRealCoeffs[0]) {
        fread(mRealCoeffs[0], sizeof(int16), FILT_SIZE, flt1r);
    }
    if (NULL != mRealCoeffs[0]) {
        fread(mRealCoeffs[1], sizeof(int16), FILT_SIZE, flt2r);
    }
    if (NULL != mRealCoeffs[0]) {
        fread(mRealCoeffs[2], sizeof(int16), FILT_SIZE, flt3r);
    }
    if (NULL != mRealCoeffs[0]) {
        fread(mRealCoeffs[3], sizeof(int16), FILT_SIZE, flt4r);
    }

    // read imaginary co-efficients
    if (NULL != mImagCoeffs[0]) {
        fread(mImagCoeffs[0], sizeof(int16), FILT_SIZE, flt1i);
    }
    if (NULL != mImagCoeffs[0]) {
        fread(mImagCoeffs[1], sizeof(int16), FILT_SIZE, flt2i);
    }
    if (NULL != mImagCoeffs[0]) {
        fread(mImagCoeffs[2], sizeof(int16), FILT_SIZE, flt3i);
    }
    if (NULL != mImagCoeffs[0]) {
        fread(mImagCoeffs[3], sizeof(int16), FILT_SIZE, flt4i);
    }

    fclose(flt1r);
    fclose(flt2r);
    fclose(flt3r);
    fclose(flt4r);
    fclose(flt1i);
    fclose(flt2i);
    fclose(flt3i);
    fclose(flt4i);

    return NO_ERROR;
}
#endif

#ifdef QCOM_CSDCLIENT_ENABLED
int AudioStreamInALSA::start_csd_record(int param)
{
    int err = NO_ERROR;

    if (mParent->mCsdHandle != NULL) {
        csd_start_record = (int (*)(int))::dlsym(mParent->mCsdHandle,"csd_client_start_record");
        if (csd_start_record == NULL) {
            ALOGE("dlsym:Error:%s Loading csd_client_start_record", dlerror());
        } else {
            err = csd_start_record(param);
        }
    }
    return err;
}

int AudioStreamInALSA::stop_csd_record()
{
    int err = NO_ERROR;
    if (mParent->mCsdHandle != NULL) {
        csd_stop_record = (int (*)())::dlsym(mParent->mCsdHandle,"csd_client_stop_record");
        if (csd_start_record == NULL) {
            ALOGE("dlsym:Error:%s Loading csd_client_start_record", dlerror());
        } else {
            csd_stop_record();
        }
    }
    return err;
}
#endif

}       // namespace android_audio_legacy
