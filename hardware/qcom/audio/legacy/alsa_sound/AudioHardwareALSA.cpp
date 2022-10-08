/* AudioHardwareALSA.cpp
 **
 ** Copyright 2008-2010 Wind River Systems
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
#include <math.h>

#define LOG_TAG "AudioHardwareALSA"
//#define LOG_NDEBUG 0
#define LOG_NDDEBUG 0
#include <utils/Log.h>
#include <utils/String8.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <cutils/properties.h>
#include <media/AudioRecord.h>
#include <hardware_legacy/power.h>

#include "AudioHardwareALSA.h"
#ifdef QCOM_USBAUDIO_ENABLED
#include "AudioUsbALSA.h"
#endif
#include "AudioUtil.h"

extern "C"
{
    //
    // Function for dlsym() to look up for creating a new AudioHardwareInterface.
    //
    android_audio_legacy::AudioHardwareInterface *createAudioHardware(void) {
        return android_audio_legacy::AudioHardwareALSA::create();
    }
#ifdef QCOM_ACDB_ENABLED
    static int (*acdb_init)();
    static void (*acdb_deallocate)();
#endif
#ifdef QCOM_CSDCLIENT_ENABLED
    static int (*csd_client_init)();
    static int (*csd_client_deinit)();
    static int (*csd_start_playback)();
    static int (*csd_stop_playback)();
#endif
}         // extern "C"

namespace android_audio_legacy
{

// ----------------------------------------------------------------------------

AudioHardwareInterface *AudioHardwareALSA::create() {
    return new AudioHardwareALSA();
}

AudioHardwareALSA::AudioHardwareALSA() :
    mALSADevice(0),mVoipStreamCount(0),mVoipBitRate(0)
    ,mCallState(0),mAcdbHandle(NULL),mCsdHandle(NULL),mMicMute(0)
{
    FILE *fp;
    char soundCardInfo[200];
    hw_module_t *module;
    char platform[128], baseband[128];
    int err = hw_get_module(ALSA_HARDWARE_MODULE_ID,
            (hw_module_t const**)&module);
    int codec_rev = 2;
    ALOGD("hw_get_module(ALSA_HARDWARE_MODULE_ID) returned err %d", err);
    if (err == 0) {
        hw_device_t* device;
        err = module->methods->open(module, ALSA_HARDWARE_NAME, &device);
        if (err == 0) {
            mALSADevice = (alsa_device_t *)device;
            mALSADevice->init(mALSADevice, mDeviceList);
            mCSCallActive = 0;
            mVolteCallActive = 0;
            mIsFmActive = 0;
            mDevSettingsFlag = 0;
#ifdef QCOM_USBAUDIO_ENABLED
            mAudioUsbALSA = new AudioUsbALSA();
            musbPlaybackState = 0;
            musbRecordingState = 0;
#endif
#ifdef USES_FLUENCE_INCALL
            mDevSettingsFlag |= TTY_OFF | DMIC_FLAG;
#else
            mDevSettingsFlag |= TTY_OFF;
#endif
            mBluetoothVGS = false;
            mFusion3Platform = false;

#ifdef QCOM_ACDB_ENABLED
            mAcdbHandle = ::dlopen("/system/lib/libacdbloader.so", RTLD_NOW);
            if (mAcdbHandle == NULL) {
                ALOGE("AudioHardware: DLOPEN not successful for ACDBLOADER");
            } else {
                ALOGD("AudioHardware: DLOPEN successful for ACDBLOADER");
                acdb_init = (int (*)())::dlsym(mAcdbHandle,"acdb_loader_init_ACDB");
                if (acdb_init == NULL) {
                    ALOGE("dlsym:Error:%s Loading acdb_loader_init_ACDB", dlerror());
                }else {
                   acdb_init();
                   acdb_deallocate = (void (*)())::dlsym(mAcdbHandle,"acdb_loader_deallocate_ACDB");
                }
            }
#endif

#ifdef QCOM_CSDCLIENT_ENABLED
             mCsdHandle = ::dlopen("/system/lib/libcsd-client.so", RTLD_NOW);
             if (mCsdHandle == NULL) {
                 ALOGE("AudioHardware: DLOPEN not successful for CSD CLIENT");
             } else {
                 ALOGD("AudioHardware: DLOPEN successful for CSD CLIENT");
                 csd_client_init = (int (*)())::dlsym(mCsdHandle,"csd_client_init");
                 csd_client_deinit = (int (*)())::dlsym(mCsdHandle,"csd_client_deinit");
                 csd_start_playback = (int (*)())::dlsym(mCsdHandle,"csd_client_start_playback");
                 csd_stop_playback = (int (*)())::dlsym(mCsdHandle,"csd_client_stop_playback");

                 if (csd_client_init == NULL) {
                    ALOGE("dlsym: Error:%s Loading csd_client_init", dlerror());
                 } else {
                    csd_client_init();
                 }
             }
             mALSADevice->setCsdHandle(mCsdHandle);
#endif
            if((fp = fopen("/proc/asound/cards","r")) == NULL) {
                ALOGE("Cannot open /proc/asound/cards file to get sound card info");
            } else {
                while((fgets(soundCardInfo, sizeof(soundCardInfo), fp) != NULL)) {
                    ALOGV("SoundCardInfo %s", soundCardInfo);
                    if (strstr(soundCardInfo, "msm8960-tabla1x-snd-card")) {
                        codec_rev = 1;
                        break;
                    } else if (strstr(soundCardInfo, "msm-snd-card")) {
                        codec_rev = 2;
                        break;
                    } else if (strstr(soundCardInfo, "msm8930-sitar-snd-card")) {
                        codec_rev = 3;
                        break;
                    }
                }
                fclose(fp);
            }

            if (codec_rev == 1) {
                    ALOGV("Detected tabla 1.x sound card");
                    snd_use_case_mgr_open(&mUcMgr, "snd_soc_msm");
            } else if (codec_rev == 3) {
                    ALOGV("Detected sitar 1.x sound card");
                    snd_use_case_mgr_open(&mUcMgr, "snd_soc_msm_Sitar");
            } else {
                    property_get("ro.board.platform", platform, "");
                    property_get("ro.baseband", baseband, "");
                    if (!strcmp("msm8960", platform) && !strcmp("mdm", baseband)) {
                        ALOGV("Detected Fusion tabla 2.x");
                        mFusion3Platform = true;
                        snd_use_case_mgr_open(&mUcMgr, "snd_soc_msm_2x_Fusion3");
                    } else {
                        ALOGV("Detected tabla 2.x sound card");
                        snd_use_case_mgr_open(&mUcMgr, "snd_soc_msm_2x");
                    }
            }

            if (mUcMgr < 0) {
                ALOGE("Failed to open ucm instance: %d", errno);
            } else {
                ALOGI("ucm instance opened: %u", (unsigned)mUcMgr);
                mUcMgr->acdb_handle = NULL;
#ifdef QCOM_ACDB_ENABLED
                if (mAcdbHandle) {
                    mUcMgr->acdb_handle = static_cast<void*> (mAcdbHandle);
                    if (mFusion3Platform)
                        mUcMgr->isFusion3Platform = true;
                    else
                        mUcMgr->isFusion3Platform = false;
                }
#endif
            }
        } else {
            ALOGE("ALSA Module could not be opened!!!");
        }
    } else {
        ALOGE("ALSA Module not found!!!");
    }
}

AudioHardwareALSA::~AudioHardwareALSA()
{
    if (mUcMgr != NULL) {
        ALOGV("closing ucm instance: %u", (unsigned)mUcMgr);
        snd_use_case_mgr_close(mUcMgr);
    }
    if (mALSADevice) {
        mALSADevice->common.close(&mALSADevice->common);
    }
    for(ALSAHandleList::iterator it = mDeviceList.begin();
            it != mDeviceList.end(); ++it) {
        it->useCase[0] = 0;
        mDeviceList.erase(it);
    }
#ifdef QCOM_ACDB_ENABLED
     if (acdb_deallocate == NULL) {
        ALOGE("dlsym: Error:%s Loading acdb_deallocate_ACDB", dlerror());
     } else {
        acdb_deallocate();
     }
     if (mAcdbHandle) {
        ::dlclose(mAcdbHandle);
        mAcdbHandle = NULL;
     }
#endif
#ifdef QCOM_USBAUDIO_ENABLED
    delete mAudioUsbALSA;
#endif

#ifdef QCOM_CSDCLEINT_ENABLED
     if (mCsdHandle) {
         if (csd_client_deinit == NULL) {
             ALOGE("dlsym: Error:%s Loading csd_client_deinit", dlerror());
         } else {
             csd_client_deinit();
         }
         ::dlclose(mCsdHandle);
         mCsdHandle = NULL;
     }
#endif
}

status_t AudioHardwareALSA::initCheck()
{
    if (!mALSADevice)
        return NO_INIT;

    return NO_ERROR;
}

status_t AudioHardwareALSA::setVoiceVolume(float v)
{
    ALOGV("setVoiceVolume(%f)\n", v);
    if (v < 0.0) {
        ALOGW("setVoiceVolume(%f) under 0.0, assuming 0.0\n", v);
        v = 0.0;
    } else if (v > 1.0) {
        ALOGW("setVoiceVolume(%f) over 1.0, assuming 1.0\n", v);
        v = 1.0;
    }

    int newMode = mode();
    ALOGV("setVoiceVolume  newMode %d",newMode);
    int vol = lrint(v * 100.0);

    // Voice volume levels from android are mapped to driver volume levels as follows.
    // 0 -> 5, 20 -> 4, 40 ->3, 60 -> 2, 80 -> 1, 100 -> 0
    // So adjust the volume to get the correct volume index in driver
    vol = 100 - vol;

    if (mALSADevice) {
        if(newMode == AudioSystem::MODE_IN_COMMUNICATION) {
            mALSADevice->setVoipVolume(vol);
        } else if (newMode == AudioSystem::MODE_IN_CALL){
               if (mCSCallActive == CS_ACTIVE)
                   mALSADevice->setVoiceVolume(vol);
               if (mVolteCallActive == IMS_ACTIVE)
                   mALSADevice->setVoLTEVolume(vol);
        }
    }

    return NO_ERROR;
}

#ifdef QCOM_FM_ENABLED
status_t  AudioHardwareALSA::setFmVolume(float value)
{
    status_t status = NO_ERROR;

    int vol;

    if (value < 0.0) {
        ALOGW("setFmVolume(%f) under 0.0, assuming 0.0\n", value);
        value = 0.0;
    } else if (value > 1.0) {
        ALOGW("setFmVolume(%f) over 1.0, assuming 1.0\n", value);
        value = 1.0;
    }
    vol  = lrint((value * 0x2000) + 0.5);

    ALOGV("setFmVolume(%f)\n", value);
    ALOGV("Setting FM volume to %d (available range is 0 to 0x2000)\n", vol);

    mALSADevice->setFmVolume(vol);

    return status;
}
#endif

status_t AudioHardwareALSA::setMasterVolume(float volume)
{
    return NO_ERROR;
}

status_t AudioHardwareALSA::setMode(int mode)
{
    status_t status = NO_ERROR;

    if (mode != mMode) {
        status = AudioHardwareBase::setMode(mode);
    }

    if (mode == AudioSystem::MODE_IN_CALL) {
        mCallState = CS_ACTIVE;
    }else if (mode == AudioSystem::MODE_NORMAL) {
        mCallState = 0;
    }

    return status;
}

status_t AudioHardwareALSA::setParameters(const String8& keyValuePairs)
{
    AudioParameter param = AudioParameter(keyValuePairs);
    String8 key;
    String8 value;
    status_t status = NO_ERROR;
    int device;
    int btRate;
    int state;
    ALOGV("setParameters() %s", keyValuePairs.string());

    key = String8(TTY_MODE_KEY);
    if (param.get(key, value) == NO_ERROR) {
        mDevSettingsFlag &= TTY_CLEAR;
        if (value == "tty_full") {
            mDevSettingsFlag |= TTY_FULL;
        } else if (value == "tty_hco") {
            mDevSettingsFlag |= TTY_HCO;
        } else if (value == "tty_vco") {
            mDevSettingsFlag |= TTY_VCO;
        } else {
            mDevSettingsFlag |= TTY_OFF;
        }
        ALOGI("Changed TTY Mode=%s", value.string());
        mALSADevice->setFlags(mDevSettingsFlag);
        if(mMode != AudioSystem::MODE_IN_CALL){
           return NO_ERROR;
        }
        doRouting(0);
    }

    key = String8(FLUENCE_KEY);
    if (param.get(key, value) == NO_ERROR) {
        if (value == "quadmic") {
            mDevSettingsFlag |= QMIC_FLAG;
            mDevSettingsFlag &= (~DMIC_FLAG);
            ALOGV("Fluence quadMic feature Enabled");
        } else if (value == "dualmic") {
            mDevSettingsFlag |= DMIC_FLAG;
            mDevSettingsFlag &= (~QMIC_FLAG);
            ALOGV("Fluence dualmic feature Enabled");
        } else if (value == "none") {
            mDevSettingsFlag &= (~DMIC_FLAG);
            mDevSettingsFlag &= (~QMIC_FLAG);
            ALOGV("Fluence feature Disabled");
        }
        mALSADevice->setFlags(mDevSettingsFlag);
        doRouting(0);
    }

#ifdef QCOM_CSDCLIENT_ENABLED
    if (mFusion3Platform) {
        key = String8(INCALLMUSIC_KEY);
        if (param.get(key, value) == NO_ERROR) {
            if (value == "true") {
                ALOGV("Enabling Incall Music setting in the setparameter\n");
                if (csd_start_playback == NULL) {
                    ALOGE("dlsym: Error:%s Loading csd_client_start_playback", dlerror());
                } else {
                    csd_start_playback();
                }
            } else {
                ALOGV("Disabling Incall Music setting in the setparameter\n");
                if (csd_stop_playback == NULL) {
                    ALOGE("dlsym: Error:%s Loading csd_client_stop_playback", dlerror());
                } else {
                    csd_stop_playback();
                }
            }
        }
    }
#endif

    key = String8(ANC_KEY);
    if (param.get(key, value) == NO_ERROR) {
        if (value == "true") {
            ALOGV("Enabling ANC setting in the setparameter\n");
            mDevSettingsFlag |= ANC_FLAG;
        } else {
            ALOGV("Disabling ANC setting in the setparameter\n");
            mDevSettingsFlag &= (~ANC_FLAG);
        }
        mALSADevice->setFlags(mDevSettingsFlag);
        doRouting(0);
    }

    key = String8(AudioParameter::keyRouting);
    if (param.getInt(key, device) == NO_ERROR) {
        // Ignore routing if device is 0.
        if(device) {
            doRouting(device);
        }
        param.remove(key);
    }

    key = String8(BT_SAMPLERATE_KEY);
    if (param.getInt(key, btRate) == NO_ERROR) {
        mALSADevice->setBtscoRate(btRate);
        param.remove(key);
    }

    key = String8(BTHEADSET_VGS);
    if (param.get(key, value) == NO_ERROR) {
        if (value == "on") {
            mBluetoothVGS = true;
        } else {
            mBluetoothVGS = false;
        }
    }

    key = String8(WIDEVOICE_KEY);
    if (param.get(key, value) == NO_ERROR) {
        bool flag = false;
        if (value == "true") {
            flag = true;
        }
        if(mALSADevice) {
            mALSADevice->enableWideVoice(flag);
        }
        param.remove(key);
    }

    key = String8(VOIPRATE_KEY);
    if (param.get(key, value) == NO_ERROR) {
            mVoipBitRate = atoi(value);
        param.remove(key);
    }

    key = String8(FENS_KEY);
    if (param.get(key, value) == NO_ERROR) {
        bool flag = false;
        if (value == "true") {
            flag = true;
        }
        if(mALSADevice) {
            mALSADevice->enableFENS(flag);
        }
        param.remove(key);
    }

#ifdef QCOM_FM_ENABLED
    key = String8(AudioParameter::keyHandleFm);
    if (param.getInt(key, device) == NO_ERROR) {
        // Ignore if device is 0
        if(device) {
            handleFm(device);
        }
        param.remove(key);
    }
#endif

    key = String8(ST_KEY);
    if (param.get(key, value) == NO_ERROR) {
        bool flag = false;
        if (value == "true") {
            flag = true;
        }
        if(mALSADevice) {
            mALSADevice->enableSlowTalk(flag);
        }
        param.remove(key);
    }
    key = String8(MODE_CALL_KEY);
    if (param.getInt(key,state) == NO_ERROR) {
        if (mCallState != state) {
            mCallState = state;
            doRouting(0);
        }
        mCallState = state;
    }
    if (param.size()) {
        status = BAD_VALUE;
    }
    return status;
}

String8 AudioHardwareALSA::getParameters(const String8& keys)
{
    AudioParameter param = AudioParameter(keys);
    String8 value;

    String8 key = String8(DUALMIC_KEY);
    if (param.get(key, value) == NO_ERROR) {
        value = String8("false");
        param.add(key, value);
    }

    key = String8(FLUENCE_KEY);
    if (param.get(key, value) == NO_ERROR) {
    if ((mDevSettingsFlag & QMIC_FLAG) &&
                               (mDevSettingsFlag & ~DMIC_FLAG))
            value = String8("quadmic");
    else if ((mDevSettingsFlag & DMIC_FLAG) &&
                                (mDevSettingsFlag & ~QMIC_FLAG))
            value = String8("dualmic");
    else if ((mDevSettingsFlag & ~DMIC_FLAG) &&
                                (mDevSettingsFlag & ~QMIC_FLAG))
            value = String8("none");
        param.add(key, value);
    }

#ifdef QCOM_FM_ENABLED
    key = String8("Fm-radio");
    if ( param.get(key,value) == NO_ERROR ) {
        if ( mIsFmActive ) {
            param.addInt(String8("isFMON"), true );
        }
    }
#endif

    key = String8(BTHEADSET_VGS);
    if (param.get(key, value) == NO_ERROR) {
        if(mBluetoothVGS)
           param.addInt(String8("isVGS"), true);
    }

    ALOGV("AudioHardwareALSA::getParameters() %s", param.toString().string());
    return param.toString();
}

#ifdef QCOM_USBAUDIO_ENABLED
void AudioHardwareALSA::closeUSBPlayback()
{
    ALOGV("closeUSBPlayback, musbPlaybackState: %d", musbPlaybackState);
    musbPlaybackState = 0;
    mAudioUsbALSA->exitPlaybackThread(SIGNAL_EVENT_KILLTHREAD);
}

void AudioHardwareALSA::closeUSBRecording()
{
    ALOGV("closeUSBRecording");
    musbRecordingState = 0;
    mAudioUsbALSA->exitRecordingThread(SIGNAL_EVENT_KILLTHREAD);
}

void AudioHardwareALSA::closeUsbPlaybackIfNothingActive(){
    ALOGV("closeUsbPlaybackIfNothingActive, musbPlaybackState: %d", musbPlaybackState);
    if(!musbPlaybackState && mAudioUsbALSA != NULL) {
        mAudioUsbALSA->exitPlaybackThread(SIGNAL_EVENT_TIMEOUT);
    }
}

void AudioHardwareALSA::closeUsbRecordingIfNothingActive(){
    ALOGV("closeUsbRecordingIfNothingActive, musbRecordingState: %d", musbRecordingState);
    if(!musbRecordingState && mAudioUsbALSA != NULL) {
        ALOGD("Closing USB Recording Session as no stream is active");
        mAudioUsbALSA->setkillUsbRecordingThread(true);
    }
}

void AudioHardwareALSA::startUsbPlaybackIfNotStarted(){
    ALOGV("Starting the USB playback %d kill %d", musbPlaybackState,
             mAudioUsbALSA->getkillUsbPlaybackThread());
    if((!musbPlaybackState) || (mAudioUsbALSA->getkillUsbPlaybackThread() == true)) {
        mAudioUsbALSA->startPlayback();
    }
}

void AudioHardwareALSA::startUsbRecordingIfNotStarted(){
    ALOGV("Starting the recording musbRecordingState: %d killUsbRecordingThread %d",
          musbRecordingState, mAudioUsbALSA->getkillUsbRecordingThread());
    if((!musbRecordingState) || (mAudioUsbALSA->getkillUsbRecordingThread() == true)) {
        mAudioUsbALSA->startRecording();
    }
}
#endif

void AudioHardwareALSA::doRouting(int device)
{
    Mutex::Autolock autoLock(mLock);
    int newMode = mode();
    bool isRouted = false;

    if ((device == AudioSystem::DEVICE_IN_VOICE_CALL)
#ifdef QCOM_FM_ENABLED
        || (device == AudioSystem::DEVICE_IN_FM_RX)
        || (device == AudioSystem::DEVICE_OUT_DIRECTOUTPUT)
        || (device == AudioSystem::DEVICE_IN_FM_RX_A2DP)
#endif
        || (device == AudioSystem::DEVICE_IN_COMMUNICATION)
        ) {
        ALOGV("Ignoring routing for FM/INCALL/VOIP recording");
        return;
    }
    if (device == 0)
        device = mCurDevice;
    ALOGV("doRouting: device %d newMode %d mCSCallActive %d mVolteCallActive %d"
         "mIsFmActive %d", device, newMode, mCSCallActive, mVolteCallActive,
         mIsFmActive);

    isRouted = routeVoLTECall(device, newMode);
    isRouted |= routeVoiceCall(device, newMode);

    if(!isRouted) {
#ifdef QCOM_USBAUDIO_ENABLED
        if(!(device & AudioSystem::DEVICE_OUT_ANLG_DOCK_HEADSET) &&
            !(device & AudioSystem::DEVICE_OUT_DGTL_DOCK_HEADSET) &&
            !(device & AudioSystem::DEVICE_IN_ANLG_DOCK_HEADSET) &&
             (musbPlaybackState)){
                //USB unplugged
                device &= ~ AudioSystem::DEVICE_OUT_PROXY;
                device &= ~ AudioSystem::DEVICE_IN_PROXY;
                ALSAHandleList::iterator it = mDeviceList.end();
                it--;
                mALSADevice->route(&(*it), (uint32_t)device, newMode);
                ALOGD("USB UNPLUGGED, setting musbPlaybackState to 0");
                musbPlaybackState = 0;
                musbRecordingState = 0;
                closeUSBRecording();
                closeUSBPlayback();
        } else if((device & AudioSystem::DEVICE_OUT_ANLG_DOCK_HEADSET)||
                  (device & AudioSystem::DEVICE_OUT_DGTL_DOCK_HEADSET)){
                    ALOGD("Routing everything to prox now");
                    ALSAHandleList::iterator it = mDeviceList.end();
                    it--;
                    mALSADevice->route(&(*it), AudioSystem::DEVICE_OUT_PROXY,
                                       newMode);
                    for(it = mDeviceList.begin(); it != mDeviceList.end(); ++it) {
                         if((!strcmp(it->useCase, SND_USE_CASE_VERB_HIFI_LOW_POWER)) ||
                            (!strcmp(it->useCase, SND_USE_CASE_MOD_PLAY_LPA))) {
                                 ALOGV("doRouting: LPA device switch to proxy");
                                 startUsbPlaybackIfNotStarted();
                                 musbPlaybackState |= USBPLAYBACKBIT_LPA;
                                 break;
                         } else if((!strcmp(it->useCase, SND_USE_CASE_VERB_VOICECALL)) ||
                                   (!strcmp(it->useCase, SND_USE_CASE_MOD_PLAY_VOICE))) {
                                    ALOGV("doRouting: VOICE device switch to proxy");
                                    startUsbRecordingIfNotStarted();
                                    startUsbPlaybackIfNotStarted();
                                    musbPlaybackState |= USBPLAYBACKBIT_VOICECALL;
                                    musbRecordingState |= USBPLAYBACKBIT_VOICECALL;
                                    break;
                        }else if((!strcmp(it->useCase, SND_USE_CASE_VERB_DIGITAL_RADIO)) ||
                                 (!strcmp(it->useCase, SND_USE_CASE_MOD_PLAY_FM))) {
                                    ALOGV("doRouting: FM device switch to proxy");
                                    startUsbPlaybackIfNotStarted();
                                    musbPlaybackState |= USBPLAYBACKBIT_FM;
                                    break;
                         }
                    }
        } else
#endif
        {
             ALSAHandleList::iterator it = mDeviceList.end();
             it--;
             mALSADevice->route(&(*it), (uint32_t)device, newMode);
        }
    }
    mCurDevice = device;
}

uint32_t AudioHardwareALSA::getVoipMode(int format)
{
    switch(format) {
    case AudioSystem::PCM_16_BIT:
               return MODE_PCM;
         break;
    case AudioSystem::AMR_NB:
               return MODE_AMR;
         break;
    case AudioSystem::AMR_WB:
               return MODE_AMR_WB;
         break;

#ifdef QCOM_QCHAT_ENABLED
    case AudioSystem::EVRC:
               return MODE_IS127;
         break;

    case AudioSystem::EVRCB:
               return MODE_4GV_NB;
         break;
    case AudioSystem::EVRCWB:
               return MODE_4GV_WB;
         break;
#endif

    default:
               return MODE_PCM;
    }
}

AudioStreamOut *
AudioHardwareALSA::openOutputStream(uint32_t devices,
                                    int *format,
                                    uint32_t *channels,
                                    uint32_t *sampleRate,
                                    status_t *status)
{
    Mutex::Autolock autoLock(mLock);
    ALOGV("openOutputStream: devices 0x%x channels %d sampleRate %d",
         devices, *channels, *sampleRate);

    audio_output_flags_t flag = static_cast<audio_output_flags_t> (*status);

    status_t err = BAD_VALUE;
    *status = NO_ERROR;
    AudioStreamOutALSA *out = 0;
    ALSAHandleList::iterator it;

    if (devices & (devices - 1)) {
        if (status) *status = err;
        ALOGE("openOutputStream called with bad devices");
        return out;
    }


# if 0
    if((devices == AudioSystem::DEVICE_OUT_DIRECTOUTPUT) &&
       ((*sampleRate == VOIP_SAMPLING_RATE_8K) || (*sampleRate == VOIP_SAMPLING_RATE_16K))) {
        bool voipstream_active = false;
        for(it = mDeviceList.begin();
            it != mDeviceList.end(); ++it) {
                if((!strcmp(it->useCase, SND_USE_CASE_VERB_IP_VOICECALL)) ||
                   (!strcmp(it->useCase, SND_USE_CASE_MOD_PLAY_VOIP))) {
                    ALOGD("openOutput:  it->rxHandle %d it->handle %d",it->rxHandle,it->handle);
                    voipstream_active = true;
                    break;
                }
        }
      if(voipstream_active == false) {
         mVoipStreamCount = 0;
         alsa_handle_t alsa_handle;
         unsigned long bufferSize;
         if(*sampleRate == VOIP_SAMPLING_RATE_8K) {
             bufferSize = VOIP_BUFFER_SIZE_8K;
         }
         else if(*sampleRate == VOIP_SAMPLING_RATE_16K) {
             bufferSize = VOIP_BUFFER_SIZE_16K;
         }
         else {
             ALOGE("unsupported samplerate %d for voip",*sampleRate);
             if (status) *status = err;
                 return out;
          }
          alsa_handle.module = mALSADevice;
          alsa_handle.bufferSize = bufferSize;
          alsa_handle.devices = devices;
          alsa_handle.handle = 0;
          if(*format == AudioSystem::PCM_16_BIT)
              alsa_handle.format = SNDRV_PCM_FORMAT_S16_LE;
          else
              alsa_handle.format = *format;
          alsa_handle.channels = VOIP_DEFAULT_CHANNEL_MODE;
          alsa_handle.channelMask = AUDIO_CHANNEL_IN_MONO;
          alsa_handle.sampleRate = *sampleRate;
          alsa_handle.latency = VOIP_PLAYBACK_LATENCY;
          alsa_handle.rxHandle = 0;
          alsa_handle.ucMgr = mUcMgr;
          mALSADevice->setVoipConfig(getVoipMode(*format), mVoipBitRate);
          char *use_case;
          snd_use_case_get(mUcMgr, "_verb", (const char **)&use_case);
          if ((use_case == NULL) || (!strcmp(use_case, SND_USE_CASE_VERB_INACTIVE))) {
              strlcpy(alsa_handle.useCase, SND_USE_CASE_VERB_IP_VOICECALL, sizeof(alsa_handle.useCase));
          } else {
              strlcpy(alsa_handle.useCase, SND_USE_CASE_MOD_PLAY_VOIP, sizeof(alsa_handle.useCase));
          }
          free(use_case);
          mDeviceList.push_back(alsa_handle);
          it = mDeviceList.end();
          it--;
          ALOGV("openoutput: mALSADevice->route useCase %s mCurDevice %d mVoipStreamCount %d mode %d", it->useCase,mCurDevice,mVoipStreamCount, mode());
          if((mCurDevice & AudioSystem::DEVICE_OUT_ANLG_DOCK_HEADSET)||
             (mCurDevice & AudioSystem::DEVICE_OUT_DGTL_DOCK_HEADSET)||
             (mCurDevice & AudioSystem::DEVICE_OUT_PROXY)){
              ALOGD("Routing to proxy for normal voip call in openOutputStream");
              mCurDevice |= AudioSystem::DEVICE_OUT_PROXY;
              alsa_handle.devices = AudioSystem::DEVICE_OUT_PROXY;
              mALSADevice->route(&(*it), mCurDevice, AudioSystem::MODE_IN_COMMUNICATION);
              ALOGD("enabling VOIP in openoutputstream, musbPlaybackState: %d", musbPlaybackState);
              startUsbPlaybackIfNotStarted();
              musbPlaybackState |= USBPLAYBACKBIT_VOIPCALL;
              ALOGD("Starting recording in openoutputstream, musbRecordingState: %d", musbRecordingState);
              startUsbRecordingIfNotStarted();
              musbRecordingState |= USBRECBIT_VOIPCALL;
          } else{
              mALSADevice->route(&(*it), mCurDevice, AudioSystem::MODE_IN_COMMUNICATION);
          }
          if(!strcmp(it->useCase, SND_USE_CASE_VERB_IP_VOICECALL)) {
              snd_use_case_set(mUcMgr, "_verb", SND_USE_CASE_VERB_IP_VOICECALL);
          } else {
              snd_use_case_set(mUcMgr, "_enamod", SND_USE_CASE_MOD_PLAY_VOIP);
          }
          err = mALSADevice->startVoipCall(&(*it));
          if (err) {
              ALOGE("Device open failed");
              return NULL;
          }
      }
      out = new AudioStreamOutALSA(this, &(*it));
      err = out->set(format, channels, sampleRate, devices);
      if(err == NO_ERROR) {
          mVoipStreamCount++;   //increment VoipstreamCount only if success
          ALOGD("openoutput mVoipStreamCount %d",mVoipStreamCount);
      }
      if (status) *status = err;
      return out;
    } else
#endif
    if ((flag & AUDIO_OUTPUT_FLAG_DIRECT) &&
        (devices == AudioSystem::DEVICE_OUT_AUX_DIGITAL)) {
        ALOGD("Multi channel PCM");
        alsa_handle_t alsa_handle;
        EDID_AUDIO_INFO info = { 0 };

        alsa_handle.module = mALSADevice;
        alsa_handle.devices = devices;
        alsa_handle.handle = 0;
        alsa_handle.format = SNDRV_PCM_FORMAT_S16_LE;

        if (!AudioUtil::getHDMIAudioSinkCaps(&info)) {
            ALOGE("openOutputStream: Failed to get HDMI sink capabilities");
            return NULL;
        }
        if (0 == *channels) {
            alsa_handle.channels = info.AudioBlocksArray[info.nAudioBlocks-1].nChannels;
            if (alsa_handle.channels > 6) {
                alsa_handle.channels = 6;
            }
            *channels = audio_channel_out_mask_from_count(alsa_handle.channels);
        } else {
            alsa_handle.channels = AudioSystem::popCount(*channels);
        }
        alsa_handle.channelMask = *channels;

        if (6 == alsa_handle.channels) {
            alsa_handle.bufferSize = DEFAULT_MULTI_CHANNEL_BUF_SIZE;
        } else {
            alsa_handle.bufferSize = DEFAULT_BUFFER_SIZE;
        }
        if (0 == *sampleRate) {
            alsa_handle.sampleRate = info.AudioBlocksArray[info.nAudioBlocks-1].nSamplingFreq;
            *sampleRate = alsa_handle.sampleRate;
        } else {
            alsa_handle.sampleRate = *sampleRate;
        }
        alsa_handle.latency = PLAYBACK_LATENCY;
        alsa_handle.rxHandle = 0;
        alsa_handle.ucMgr = mUcMgr;
        ALOGD("alsa_handle.channels %d alsa_handle.sampleRate %d",alsa_handle.channels,alsa_handle.sampleRate);

        char *use_case;
        snd_use_case_get(mUcMgr, "_verb", (const char **)&use_case);
        if ((use_case == NULL) || (!strcmp(use_case, SND_USE_CASE_VERB_INACTIVE))) {
            strlcpy(alsa_handle.useCase, SND_USE_CASE_VERB_HIFI2 , sizeof(alsa_handle.useCase));
        } else {
            strlcpy(alsa_handle.useCase, SND_USE_CASE_MOD_PLAY_MUSIC2, sizeof(alsa_handle.useCase));
        }
        free(use_case);
        mDeviceList.push_back(alsa_handle);
        ALSAHandleList::iterator it = mDeviceList.end();
        it--;
        ALOGD("it->useCase %s", it->useCase);
        mALSADevice->route(&(*it), devices, mode());
        if(!strcmp(it->useCase, SND_USE_CASE_VERB_HIFI2)) {
            snd_use_case_set(mUcMgr, "_verb", SND_USE_CASE_VERB_HIFI2 );
        } else {
            snd_use_case_set(mUcMgr, "_enamod", SND_USE_CASE_MOD_PLAY_MUSIC2);
        }
        ALOGD("channels: %d", AudioSystem::popCount(*channels));
        err = mALSADevice->open(&(*it));

        if (err) {
            ALOGE("Device open failed err:%d",err);
        } else {
            out = new AudioStreamOutALSA(this, &(*it));
            err = out->set(format, channels, sampleRate, devices);
        }
        if (status) *status = err;
        return out;
    } else {

      alsa_handle_t alsa_handle;
      unsigned long bufferSize = DEFAULT_BUFFER_SIZE;

      for (size_t b = 1; (bufferSize & ~b) != 0; b <<= 1)
          bufferSize &= ~b;

      alsa_handle.module = mALSADevice;
      alsa_handle.bufferSize = bufferSize;
      alsa_handle.devices = devices;
      alsa_handle.handle = 0;
      alsa_handle.format = SNDRV_PCM_FORMAT_S16_LE;
      alsa_handle.channels = DEFAULT_CHANNEL_MODE;
      alsa_handle.channelMask = AUDIO_CHANNEL_OUT_STEREO;
      alsa_handle.sampleRate = DEFAULT_SAMPLING_RATE;
      alsa_handle.latency = PLAYBACK_LATENCY;
      alsa_handle.rxHandle = 0;
      alsa_handle.ucMgr = mUcMgr;
      alsa_handle.isDeepbufferOutput = false;

      char *use_case;
      snd_use_case_get(mUcMgr, "_verb", (const char **)&use_case);

      if (flag & AUDIO_OUTPUT_FLAG_DEEP_BUFFER) {
      ALOGD("openOutputStream: DeepBuffer Output");
          alsa_handle.isDeepbufferOutput = true;
          if ((use_case == NULL) || (!strcmp(use_case, SND_USE_CASE_VERB_INACTIVE))) {
               strlcpy(alsa_handle.useCase, SND_USE_CASE_VERB_HIFI, sizeof(alsa_handle.useCase));
          } else {
               strlcpy(alsa_handle.useCase, SND_USE_CASE_MOD_PLAY_MUSIC, sizeof(alsa_handle.useCase));
          }
      } else {
      ALOGD("openOutputStream: Lowlatency Output");
          alsa_handle.bufferSize = PLAYBACK_LOW_LATENCY_BUFFER_SIZE;
          alsa_handle.latency = PLAYBACK_LOW_LATENCY_MEASURED;
          if ((use_case == NULL) || (!strcmp(use_case, SND_USE_CASE_VERB_INACTIVE))) {
               strlcpy(alsa_handle.useCase, SND_USE_CASE_VERB_HIFI_LOWLATENCY_MUSIC, sizeof(alsa_handle.useCase));
          } else {
               strlcpy(alsa_handle.useCase, SND_USE_CASE_MOD_PLAY_LOWLATENCY_MUSIC, sizeof(alsa_handle.useCase));
          }
      }
      free(use_case);
      mDeviceList.push_back(alsa_handle);
      ALSAHandleList::iterator it = mDeviceList.end();
      it--;
      ALOGV("useCase %s", it->useCase);
#ifdef QCOM_USBAUDIO_ENABLED
      if((devices & AudioSystem::DEVICE_OUT_ANLG_DOCK_HEADSET)||
         (devices & AudioSystem::DEVICE_OUT_DGTL_DOCK_HEADSET)){
          ALOGD("Routing to proxy for normal playback in openOutputStream");
          devices |= AudioSystem::DEVICE_OUT_PROXY;
      }
#endif
      mALSADevice->route(&(*it), devices, mode());
      if (flag & AUDIO_OUTPUT_FLAG_DEEP_BUFFER) {
          if(!strcmp(it->useCase, SND_USE_CASE_VERB_HIFI)) {
             snd_use_case_set(mUcMgr, "_verb", SND_USE_CASE_VERB_HIFI);
          } else {
             snd_use_case_set(mUcMgr, "_enamod", SND_USE_CASE_MOD_PLAY_MUSIC);
          }
      } else {
          if(!strcmp(it->useCase, SND_USE_CASE_VERB_HIFI_LOWLATENCY_MUSIC)) {
             snd_use_case_set(mUcMgr, "_verb", SND_USE_CASE_VERB_HIFI_LOWLATENCY_MUSIC);
          } else {
             snd_use_case_set(mUcMgr, "_enamod", SND_USE_CASE_MOD_PLAY_LOWLATENCY_MUSIC);
          }
      }
      err = mALSADevice->open(&(*it));
      if (err) {
          ALOGE("Device open failed");
      } else {
          out = new AudioStreamOutALSA(this, &(*it));
          err = out->set(format, channels, sampleRate, devices);
      }

      if (status) *status = err;
      return out;
    }
}

void
AudioHardwareALSA::closeOutputStream(AudioStreamOut* out)
{
    delete out;
}

#ifdef QCOM_TUNNEL_LPA_ENABLED
AudioStreamOut *
AudioHardwareALSA::openOutputSession(uint32_t devices,
                                     int *format,
                                     status_t *status,
                                     int sessionId,
                                     uint32_t samplingRate,
                                     uint32_t channels)
{
    Mutex::Autolock autoLock(mLock);
    ALOGD("openOutputSession = %d" ,sessionId);
    AudioStreamOutALSA *out = 0;
    status_t err = BAD_VALUE;

    alsa_handle_t alsa_handle;
    unsigned long bufferSize = DEFAULT_BUFFER_SIZE;

    for (size_t b = 1; (bufferSize & ~b) != 0; b <<= 1)
        bufferSize &= ~b;

    alsa_handle.module = mALSADevice;
    alsa_handle.bufferSize = bufferSize;
    alsa_handle.devices = devices;
    alsa_handle.handle = 0;
    alsa_handle.format = SNDRV_PCM_FORMAT_S16_LE;
    alsa_handle.channels = DEFAULT_CHANNEL_MODE;
    alsa_handle.channelMask = AUDIO_CHANNEL_OUT_STEREO;
    alsa_handle.sampleRate = DEFAULT_SAMPLING_RATE;
    alsa_handle.latency = VOICE_LATENCY;
    alsa_handle.rxHandle = 0;
    alsa_handle.ucMgr = mUcMgr;

    char *use_case;
    if(sessionId == TUNNEL_SESSION_ID) {
        snd_use_case_get(mUcMgr, "_verb", (const char **)&use_case);
        if ((use_case == NULL) || (!strcmp(use_case, SND_USE_CASE_VERB_INACTIVE))) {
            strlcpy(alsa_handle.useCase, SND_USE_CASE_VERB_HIFI_TUNNEL, sizeof(alsa_handle.useCase));
        } else {
            strlcpy(alsa_handle.useCase, SND_USE_CASE_MOD_PLAY_TUNNEL, sizeof(alsa_handle.useCase));
        }
    } else {
        snd_use_case_get(mUcMgr, "_verb", (const char **)&use_case);
        if ((use_case == NULL) || (!strcmp(use_case, SND_USE_CASE_VERB_INACTIVE))) {
            strlcpy(alsa_handle.useCase, SND_USE_CASE_VERB_HIFI_LOW_POWER, sizeof(alsa_handle.useCase));
        } else {
            strlcpy(alsa_handle.useCase, SND_USE_CASE_MOD_PLAY_LPA, sizeof(alsa_handle.useCase));
        }
    }
    free(use_case);
    mDeviceList.push_back(alsa_handle);
    ALSAHandleList::iterator it = mDeviceList.end();
    it--;
    ALOGD("useCase %s", it->useCase);
#ifdef QCOM_USBAUDIO_ENABLED
    if((devices & AudioSystem::DEVICE_OUT_ANLG_DOCK_HEADSET)||
       (devices & AudioSystem::DEVICE_OUT_DGTL_DOCK_HEADSET)){
        ALOGD("Routing to proxy for LPA in openOutputSession");
        devices |= AudioSystem::DEVICE_OUT_PROXY;
        mALSADevice->route(&(*it), devices, mode());
        devices = AudioSystem::DEVICE_OUT_ANLG_DOCK_HEADSET;
        ALOGD("Starting USBPlayback for LPA");
        startUsbPlaybackIfNotStarted();
        musbPlaybackState |= USBPLAYBACKBIT_LPA;
    } else
#endif
    {
        mALSADevice->route(&(*it), devices, mode());
    }
    if(sessionId == TUNNEL_SESSION_ID) {
        if(!strcmp(it->useCase, SND_USE_CASE_VERB_HIFI_TUNNEL)) {
            snd_use_case_set(mUcMgr, "_verb", SND_USE_CASE_VERB_HIFI_TUNNEL);
        } else {
            snd_use_case_set(mUcMgr, "_enamod", SND_USE_CASE_MOD_PLAY_TUNNEL);
        }
    }
    else {
        if(!strcmp(it->useCase, SND_USE_CASE_VERB_HIFI_LOW_POWER)) {
            snd_use_case_set(mUcMgr, "_verb", SND_USE_CASE_VERB_HIFI_LOW_POWER);
        } else {
            snd_use_case_set(mUcMgr, "_enamod", SND_USE_CASE_MOD_PLAY_LPA);
        }
    }
    err = mALSADevice->open(&(*it));
    out = new AudioStreamOutALSA(this, &(*it));

    if (status) *status = err;
       return out;
}

void
AudioHardwareALSA::closeOutputSession(AudioStreamOut* out)
{
    delete out;
}
#endif

AudioStreamIn *
AudioHardwareALSA::openInputStream(uint32_t devices,
                                   int *format,
                                   uint32_t *channels,
                                   uint32_t *sampleRate,
                                   status_t *status,
                                   AudioSystem::audio_in_acoustics acoustics)
{
    Mutex::Autolock autoLock(mLock);
    char *use_case;
    int newMode = mode();
    uint32_t route_devices;

    status_t err = BAD_VALUE;
    AudioStreamInALSA *in = 0;
    ALSAHandleList::iterator it;

    ALOGD("openInputStream: devices 0x%x channels %d sampleRate %d", devices, *channels, *sampleRate);
    if (devices & (devices - 1)) {
        if (status) *status = err;
        return in;
    }

    if((devices == AudioSystem::DEVICE_IN_COMMUNICATION) &&
       ((*sampleRate == VOIP_SAMPLING_RATE_8K) || (*sampleRate == VOIP_SAMPLING_RATE_16K))) {
        bool voipstream_active = false;
        for(it = mDeviceList.begin();
            it != mDeviceList.end(); ++it) {
                if((!strcmp(it->useCase, SND_USE_CASE_VERB_IP_VOICECALL)) ||
                   (!strcmp(it->useCase, SND_USE_CASE_MOD_PLAY_VOIP))) {
                    ALOGD("openInput:  it->rxHandle %p it->handle %p",it->rxHandle,it->handle);
                    voipstream_active = true;
                    break;
                }
        }
        if(voipstream_active == false) {
           mVoipStreamCount = 0;
           alsa_handle_t alsa_handle;
           unsigned long bufferSize;
           if(*sampleRate == VOIP_SAMPLING_RATE_8K) {
               bufferSize = VOIP_BUFFER_SIZE_8K;
           }
           else if(*sampleRate == VOIP_SAMPLING_RATE_16K) {
               bufferSize = VOIP_BUFFER_SIZE_16K;
           }
           else {
               ALOGE("unsupported samplerate %d for voip",*sampleRate);
               if (status) *status = err;
               return in;
           }
           alsa_handle.module = mALSADevice;
           alsa_handle.bufferSize = bufferSize;
           alsa_handle.devices = devices;
           alsa_handle.handle = 0;
          if(*format == AudioSystem::PCM_16_BIT)
              alsa_handle.format = SNDRV_PCM_FORMAT_S16_LE;
          else
              alsa_handle.format = *format;
           alsa_handle.channels = VOIP_DEFAULT_CHANNEL_MODE;
           alsa_handle.channelMask = AUDIO_CHANNEL_IN_MONO;
           alsa_handle.sampleRate = *sampleRate;
           alsa_handle.latency = VOIP_RECORD_LATENCY;
           alsa_handle.rxHandle = 0;
           alsa_handle.ucMgr = mUcMgr;
          mALSADevice->setVoipConfig(getVoipMode(*format), mVoipBitRate);
           snd_use_case_get(mUcMgr, "_verb", (const char **)&use_case);
           if ((use_case != NULL) && (strcmp(use_case, SND_USE_CASE_VERB_INACTIVE))) {
                strlcpy(alsa_handle.useCase, SND_USE_CASE_MOD_PLAY_VOIP, sizeof(alsa_handle.useCase));
           } else {
                strlcpy(alsa_handle.useCase, SND_USE_CASE_VERB_IP_VOICECALL, sizeof(alsa_handle.useCase));
           }
           free(use_case);
           mDeviceList.push_back(alsa_handle);
           it = mDeviceList.end();
           it--;
           ALOGD("mCurrDevice: %d", mCurDevice);
#ifdef QCOM_USBAUDIO_ENABLED
           if((mCurDevice == AudioSystem::DEVICE_OUT_ANLG_DOCK_HEADSET)||
              (mCurDevice == AudioSystem::DEVICE_OUT_DGTL_DOCK_HEADSET)){
              ALOGD("Routing everything from proxy for voipcall");
              mALSADevice->route(&(*it), AudioSystem::DEVICE_IN_PROXY, AudioSystem::MODE_IN_COMMUNICATION);
              ALOGD("enabling VOIP in openInputstream, musbPlaybackState: %d", musbPlaybackState);
              startUsbPlaybackIfNotStarted();
              musbPlaybackState |= USBPLAYBACKBIT_VOIPCALL;
              ALOGD("Starting recording in openoutputstream, musbRecordingState: %d", musbRecordingState);
              startUsbRecordingIfNotStarted();
              musbRecordingState |= USBRECBIT_VOIPCALL;
           } else
#endif
           {
               mALSADevice->route(&(*it),mCurDevice, AudioSystem::MODE_IN_COMMUNICATION);
           }
           if(!strcmp(it->useCase, SND_USE_CASE_VERB_IP_VOICECALL)) {
               snd_use_case_set(mUcMgr, "_verb", SND_USE_CASE_VERB_IP_VOICECALL);
           } else {
               snd_use_case_set(mUcMgr, "_enamod", SND_USE_CASE_MOD_PLAY_VOIP);
           }
           if(sampleRate) {
               it->sampleRate = *sampleRate;
           }
           if(channels)
               it->channels = AudioSystem::popCount(*channels);
           err = mALSADevice->startVoipCall(&(*it));
           if (err) {
               ALOGE("Error opening pcm input device");
               return NULL;
           }
        }
        in = new AudioStreamInALSA(this, &(*it), acoustics);
        err = in->set(format, channels, sampleRate, devices);
        if(err == NO_ERROR) {
            mVoipStreamCount++;   //increment VoipstreamCount only if success
            ALOGD("OpenInput mVoipStreamCount %d",mVoipStreamCount);
        }
        ALOGD("openInput: After Get alsahandle");
        if (status) *status = err;
        return in;
      } else {
        alsa_handle_t alsa_handle;
        unsigned long bufferSize = MIN_CAPTURE_BUFFER_SIZE_PER_CH;

        alsa_handle.module = mALSADevice;
        alsa_handle.bufferSize = bufferSize;
        alsa_handle.devices = devices;
        alsa_handle.handle = 0;
        alsa_handle.format = SNDRV_PCM_FORMAT_S16_LE;
        alsa_handle.channels = VOICE_CHANNEL_MODE;
        alsa_handle.channelMask = AUDIO_CHANNEL_IN_MONO;
        alsa_handle.sampleRate = android::AudioRecord::DEFAULT_SAMPLE_RATE;
        alsa_handle.latency = RECORD_LATENCY;
        alsa_handle.rxHandle = 0;
        alsa_handle.ucMgr = mUcMgr;
        snd_use_case_get(mUcMgr, "_verb", (const char **)&use_case);
        if ((use_case != NULL) && (strcmp(use_case, SND_USE_CASE_VERB_INACTIVE))) {
            if ((devices == AudioSystem::DEVICE_IN_VOICE_CALL) &&
                (newMode == AudioSystem::MODE_IN_CALL)) {
                ALOGD("openInputStream: into incall recording, channels %d", *channels);
                mIncallMode = *channels;
                if ((*channels & AudioSystem::CHANNEL_IN_VOICE_UPLINK) &&
                    (*channels & AudioSystem::CHANNEL_IN_VOICE_DNLINK)) {
                    if (mFusion3Platform) {
                        mALSADevice->setVocRecMode(INCALL_REC_STEREO);
                        strlcpy(alsa_handle.useCase, SND_USE_CASE_MOD_CAPTURE_VOICE,
                                sizeof(alsa_handle.useCase));
                    } else {
                        strlcpy(alsa_handle.useCase, SND_USE_CASE_MOD_CAPTURE_VOICE_UL_DL,
                                sizeof(alsa_handle.useCase));
                    }
                } else if (*channels & AudioSystem::CHANNEL_IN_VOICE_DNLINK) {
                    if (mFusion3Platform) {
                        mALSADevice->setVocRecMode(INCALL_REC_MONO);
                        strlcpy(alsa_handle.useCase, SND_USE_CASE_MOD_CAPTURE_VOICE,
                                sizeof(alsa_handle.useCase));
                    } else {
                        strlcpy(alsa_handle.useCase, SND_USE_CASE_MOD_CAPTURE_VOICE_DL,
                                sizeof(alsa_handle.useCase));
                    }
                }
#ifdef QCOM_FM_ENABLED
            } else if((devices == AudioSystem::DEVICE_IN_FM_RX)) {
                strlcpy(alsa_handle.useCase, SND_USE_CASE_MOD_CAPTURE_FM, sizeof(alsa_handle.useCase));
            } else if(devices == AudioSystem::DEVICE_IN_FM_RX_A2DP) {
                strlcpy(alsa_handle.useCase, SND_USE_CASE_MOD_CAPTURE_A2DP_FM, sizeof(alsa_handle.useCase));
#endif
            } else {
        char value[128];
        property_get("persist.audio.lowlatency.rec",value,"0");
                if (!strcmp("true", value)) {
                    strlcpy(alsa_handle.useCase, SND_USE_CASE_MOD_CAPTURE_LOWLATENCY_MUSIC, sizeof(alsa_handle.useCase));
                } else {
                    strlcpy(alsa_handle.useCase, SND_USE_CASE_MOD_CAPTURE_MUSIC, sizeof(alsa_handle.useCase));
                }
            }
        } else {
            if ((devices == AudioSystem::DEVICE_IN_VOICE_CALL) &&
                (newMode == AudioSystem::MODE_IN_CALL)) {
                ALOGD("openInputStream: incall recording, channels %d", *channels);
                mIncallMode = *channels;
                if ((*channels & AudioSystem::CHANNEL_IN_VOICE_UPLINK) &&
                    (*channels & AudioSystem::CHANNEL_IN_VOICE_DNLINK)) {
                    if (mFusion3Platform) {
                        mALSADevice->setVocRecMode(INCALL_REC_STEREO);
                        strlcpy(alsa_handle.useCase, SND_USE_CASE_VERB_INCALL_REC,
                                sizeof(alsa_handle.useCase));
                    } else {
                        strlcpy(alsa_handle.useCase, SND_USE_CASE_VERB_UL_DL_REC,
                                sizeof(alsa_handle.useCase));
                    }
                } else if (*channels & AudioSystem::CHANNEL_IN_VOICE_DNLINK) {
                    if (mFusion3Platform) {
                        mALSADevice->setVocRecMode(INCALL_REC_MONO);
                        strlcpy(alsa_handle.useCase, SND_USE_CASE_VERB_INCALL_REC,
                                sizeof(alsa_handle.useCase));
                    } else {
                       strlcpy(alsa_handle.useCase, SND_USE_CASE_VERB_DL_REC,
                               sizeof(alsa_handle.useCase));
                    }
                }
#ifdef QCOM_FM_ENABLED
            } else if(devices == AudioSystem::DEVICE_IN_FM_RX) {
                strlcpy(alsa_handle.useCase, SND_USE_CASE_VERB_FM_REC, sizeof(alsa_handle.useCase));
            } else if (devices == AudioSystem::DEVICE_IN_FM_RX_A2DP) {
                strlcpy(alsa_handle.useCase, SND_USE_CASE_VERB_FM_A2DP_REC, sizeof(alsa_handle.useCase));
#endif
            } else {
                char value[128];
                property_get("persist.audio.lowlatency.rec",value,"0");
                if (!strcmp("true", value)) {
                    strlcpy(alsa_handle.useCase, SND_USE_CASE_VERB_HIFI_LOWLATENCY_REC, sizeof(alsa_handle.useCase));
                } else {
                    strlcpy(alsa_handle.useCase, SND_USE_CASE_VERB_HIFI_REC, sizeof(alsa_handle.useCase));
                }
            }
        }
        free(use_case);
        mDeviceList.push_back(alsa_handle);
        ALSAHandleList::iterator it = mDeviceList.end();
        it--;
        //update channel info before do routing
        if(channels) {
            it->channels = AudioSystem::popCount((*channels) &
                      (AudioSystem::CHANNEL_IN_STEREO
                       | AudioSystem::CHANNEL_IN_MONO
#ifdef QCOM_SSR_ENABLED
                       | AudioSystem::CHANNEL_IN_5POINT1
#endif
                       | AUDIO_CHANNEL_IN_FRONT_BACK));
            it->channelMask = *channels;
            ALOGV("updated channel info: channels=%d channelMask %08x",
                  it->channels, it->channelMask);
        }
        if (devices == AudioSystem::DEVICE_IN_VOICE_CALL){
           /* Add current devices info to devices to do route */
#ifdef QCOM_USBAUDIO_ENABLED
            if(mCurDevice == AudioSystem::DEVICE_OUT_ANLG_DOCK_HEADSET ||
               mCurDevice == AudioSystem::DEVICE_OUT_DGTL_DOCK_HEADSET){
                ALOGD("Routing everything from proxy for VOIP call");
                route_devices = devices | AudioSystem::DEVICE_IN_PROXY;
            } else
#endif
            {
            route_devices = devices | mCurDevice;
            }
            mALSADevice->route(&(*it), route_devices, mode());
        } else {
#ifdef QCOM_USBAUDIO_ENABLED
            if(devices & AudioSystem::DEVICE_IN_ANLG_DOCK_HEADSET ||
               devices & AudioSystem::DEVICE_IN_PROXY) {
                devices |= AudioSystem::DEVICE_IN_PROXY;
                ALOGD("routing everything from proxy");
            mALSADevice->route(&(*it), devices, mode());
            } else
#endif
            {
                mALSADevice->route(&(*it), devices, mode());
            }
        }
        if(!strcmp(it->useCase, SND_USE_CASE_VERB_HIFI_REC) ||
           !strcmp(it->useCase, SND_USE_CASE_VERB_HIFI_LOWLATENCY_REC) ||
#ifdef QCOM_FM_ENABLED
           !strcmp(it->useCase, SND_USE_CASE_VERB_FM_REC) ||
           !strcmp(it->useCase, SND_USE_CASE_VERB_FM_A2DP_REC) ||
#endif
           !strcmp(it->useCase, SND_USE_CASE_VERB_DL_REC) ||
           !strcmp(it->useCase, SND_USE_CASE_VERB_UL_DL_REC) ||
           !strcmp(it->useCase, SND_USE_CASE_VERB_INCALL_REC)) {
            snd_use_case_set(mUcMgr, "_verb", it->useCase);
        } else {
            snd_use_case_set(mUcMgr, "_enamod", it->useCase);
        }
        if(sampleRate) {
            it->sampleRate = *sampleRate;
        }
        if (!strncmp(it->useCase, SND_USE_CASE_VERB_HIFI_REC, strlen(SND_USE_CASE_VERB_HIFI_REC))
            || !strncmp(it->useCase, SND_USE_CASE_MOD_CAPTURE_MUSIC, strlen(SND_USE_CASE_MOD_CAPTURE_MUSIC))) {
            ALOGV("OpenInoutStream: Use larger buffer size for 5.1(%s) recording ", it->useCase);
            it->bufferSize = getInputBufferSize(it->sampleRate,*format,it->channels);
        }
        err = mALSADevice->open(&(*it));
        if (err) {
           ALOGE("Error opening pcm input device");
        } else {
           in = new AudioStreamInALSA(this, &(*it), acoustics);
           err = in->set(format, channels, sampleRate, devices);
        }
        if (status) *status = err;
        return in;
      }
}

void
AudioHardwareALSA::closeInputStream(AudioStreamIn* in)
{
    delete in;
}

status_t AudioHardwareALSA::setMicMute(bool state)
{
    if (mMicMute != state) {
        mMicMute = state;
        ALOGD("setMicMute: mMicMute %d", mMicMute);
        if(mALSADevice) {
            mALSADevice->setMicMute(state);
        }
    }
    return NO_ERROR;
}

status_t AudioHardwareALSA::getMicMute(bool *state)
{
    *state = mMicMute;
    return NO_ERROR;
}

status_t AudioHardwareALSA::dump(int fd, const Vector<String16>& args)
{
    return NO_ERROR;
}

size_t AudioHardwareALSA::getInputBufferSize(uint32_t sampleRate, int format, int channelCount)
{
    size_t bufferSize = 0;
    if (format == AudioSystem::PCM_16_BIT) {
        if(sampleRate == 8000 || sampleRate == 16000 || sampleRate == 32000) {
            bufferSize = (sampleRate * channelCount * 20 * sizeof(int16_t)) / 1000;
        } else if (sampleRate == 11025 || sampleRate == 12000) {
            bufferSize = 256 * sizeof(int16_t)  * channelCount;
        } else if (sampleRate == 22050 || sampleRate == 24000) {
            bufferSize = 512 * sizeof(int16_t)  * channelCount;
        } else if (sampleRate == 44100 || sampleRate == 48000) {
            bufferSize = 1024 * sizeof(int16_t) * channelCount;
        }
    } else {
        ALOGE("getInputBufferSize bad format: %d", format);
    }
    return bufferSize;
}

#ifdef QCOM_FM_ENABLED
void AudioHardwareALSA::handleFm(int device)
{
int newMode = mode();
    if(device & AudioSystem::DEVICE_OUT_FM && mIsFmActive == 0) {
        // Start FM Radio on current active device
        unsigned long bufferSize = FM_BUFFER_SIZE;
        alsa_handle_t alsa_handle;
        char *use_case;
        ALOGV("Start FM");
        snd_use_case_get(mUcMgr, "_verb", (const char **)&use_case);
        if ((use_case == NULL) || (!strcmp(use_case, SND_USE_CASE_VERB_INACTIVE))) {
            strlcpy(alsa_handle.useCase, SND_USE_CASE_VERB_DIGITAL_RADIO, sizeof(alsa_handle.useCase));
        } else {
            strlcpy(alsa_handle.useCase, SND_USE_CASE_MOD_PLAY_FM, sizeof(alsa_handle.useCase));
        }
        free(use_case);

        for (size_t b = 1; (bufferSize & ~b) != 0; b <<= 1)
        bufferSize &= ~b;
        alsa_handle.module = mALSADevice;
        alsa_handle.bufferSize = bufferSize;
        alsa_handle.devices = device;
        alsa_handle.handle = 0;
        alsa_handle.format = SNDRV_PCM_FORMAT_S16_LE;
        alsa_handle.channels = DEFAULT_CHANNEL_MODE;
        alsa_handle.channelMask = AUDIO_CHANNEL_OUT_STEREO;
        alsa_handle.sampleRate = DEFAULT_SAMPLING_RATE;
        alsa_handle.latency = VOICE_LATENCY;
        alsa_handle.rxHandle = 0;
        alsa_handle.ucMgr = mUcMgr;
        mIsFmActive = 1;
        mDeviceList.push_back(alsa_handle);
        ALSAHandleList::iterator it = mDeviceList.end();
        it--;
        if((device & AudioSystem::DEVICE_OUT_ANLG_DOCK_HEADSET)||
           (device & AudioSystem::DEVICE_OUT_DGTL_DOCK_HEADSET)){
            device |= AudioSystem::DEVICE_OUT_PROXY;
            alsa_handle.devices = AudioSystem::DEVICE_OUT_PROXY;
            ALOGD("Routing to proxy for FM case");
        }
        mALSADevice->route(&(*it), (uint32_t)device, newMode);
        if(!strcmp(it->useCase, SND_USE_CASE_VERB_DIGITAL_RADIO)) {
            snd_use_case_set(mUcMgr, "_verb", SND_USE_CASE_VERB_DIGITAL_RADIO);
        } else {
            snd_use_case_set(mUcMgr, "_enamod", SND_USE_CASE_MOD_PLAY_FM);
        }
        mALSADevice->startFm(&(*it));
        if((device & AudioSystem::DEVICE_OUT_ANLG_DOCK_HEADSET)||
           (device & AudioSystem::DEVICE_OUT_DGTL_DOCK_HEADSET)){
            ALOGD("Starting FM, musbPlaybackState %d", musbPlaybackState);
            startUsbPlaybackIfNotStarted();
            musbPlaybackState |= USBPLAYBACKBIT_FM;
        }
    } else if (!(device & AudioSystem::DEVICE_OUT_FM) && mIsFmActive == 1) {
        //i Stop FM Radio
        ALOGV("Stop FM");
        for(ALSAHandleList::iterator it = mDeviceList.begin();
            it != mDeviceList.end(); ++it) {
            if((!strcmp(it->useCase, SND_USE_CASE_VERB_DIGITAL_RADIO)) ||
              (!strcmp(it->useCase, SND_USE_CASE_MOD_PLAY_FM))) {
                mALSADevice->close(&(*it));
                //mALSADevice->route(&(*it), (uint32_t)device, newMode);
                mDeviceList.erase(it);
                break;
            }
        }
        mIsFmActive = 0;
        musbPlaybackState &= ~USBPLAYBACKBIT_FM;
        if((device & AudioSystem::DEVICE_OUT_ANLG_DOCK_HEADSET)||
           (device & AudioSystem::DEVICE_OUT_DGTL_DOCK_HEADSET)){
            closeUsbPlaybackIfNothingActive();
        }
    }
}
#endif

void AudioHardwareALSA::disableVoiceCall(char* verb, char* modifier, int mode, int device)
{
    for(ALSAHandleList::iterator it = mDeviceList.begin();
         it != mDeviceList.end(); ++it) {
        if((!strcmp(it->useCase, verb)) ||
           (!strcmp(it->useCase, modifier))) {
            ALOGV("Disabling voice call");
            mALSADevice->close(&(*it));
            mALSADevice->route(&(*it), (uint32_t)device, mode);
            mDeviceList.erase(it);
            break;
        }
    }
#ifdef QCOM_USBAUDIO_ENABLED
   if(musbPlaybackState & USBPLAYBACKBIT_VOICECALL) {
          ALOGD("Voice call ended on USB");
          musbPlaybackState &= ~USBPLAYBACKBIT_VOICECALL;
          musbRecordingState &= ~USBRECBIT_VOICECALL;
          closeUsbRecordingIfNothingActive();
          closeUsbPlaybackIfNothingActive();
   }
#endif
}
void AudioHardwareALSA::enableVoiceCall(char* verb, char* modifier, int mode, int device)
{
// Start voice call
unsigned long bufferSize = DEFAULT_VOICE_BUFFER_SIZE;
alsa_handle_t alsa_handle;
char *use_case;
    snd_use_case_get(mUcMgr, "_verb", (const char **)&use_case);
    if ((use_case == NULL) || (!strcmp(use_case, SND_USE_CASE_VERB_INACTIVE))) {
        strlcpy(alsa_handle.useCase, verb, sizeof(alsa_handle.useCase));
    } else {
        strlcpy(alsa_handle.useCase, modifier, sizeof(alsa_handle.useCase));
    }
    free(use_case);

    for (size_t b = 1; (bufferSize & ~b) != 0; b <<= 1)
    bufferSize &= ~b;
    alsa_handle.module = mALSADevice;
    alsa_handle.bufferSize = bufferSize;
    alsa_handle.devices = device;
    alsa_handle.handle = 0;
    alsa_handle.format = SNDRV_PCM_FORMAT_S16_LE;
    alsa_handle.channels = VOICE_CHANNEL_MODE;
    alsa_handle.channelMask = AUDIO_CHANNEL_IN_MONO;
    alsa_handle.sampleRate = VOICE_SAMPLING_RATE;
    alsa_handle.latency = VOICE_LATENCY;
    alsa_handle.rxHandle = 0;
    alsa_handle.ucMgr = mUcMgr;
    mDeviceList.push_back(alsa_handle);
    ALSAHandleList::iterator it = mDeviceList.end();
    it--;
#ifdef QCOM_USBAUDIO_ENABLED
    if((device & AudioSystem::DEVICE_OUT_ANLG_DOCK_HEADSET)||
       (device & AudioSystem::DEVICE_OUT_DGTL_DOCK_HEADSET)){
        device |= AudioSystem::DEVICE_OUT_PROXY;
        alsa_handle.devices = device;
    }
#endif
    mALSADevice->route(&(*it), (uint32_t)device, mode);
    if (!strcmp(it->useCase, verb)) {
        snd_use_case_set(mUcMgr, "_verb", verb);
    } else {
        snd_use_case_set(mUcMgr, "_enamod", modifier);
    }
    mALSADevice->startVoiceCall(&(*it));
#ifdef QCOM_USBAUDIO_ENABLED
    if((device & AudioSystem::DEVICE_OUT_ANLG_DOCK_HEADSET)||
       (device & AudioSystem::DEVICE_OUT_DGTL_DOCK_HEADSET)){
       startUsbRecordingIfNotStarted();
       startUsbPlaybackIfNotStarted();
       musbPlaybackState |= USBPLAYBACKBIT_VOICECALL;
       musbRecordingState |= USBRECBIT_VOICECALL;
    }
#endif
}

bool AudioHardwareALSA::routeVoiceCall(int device, int newMode)
{
int csCallState = mCallState&0xF;
 bool isRouted = false;
 switch (csCallState) {
    case CS_INACTIVE:
        if (mCSCallActive != CS_INACTIVE) {
            ALOGD("doRouting: Disabling voice call");
            disableVoiceCall((char *)SND_USE_CASE_VERB_VOICECALL,
                (char *)SND_USE_CASE_MOD_PLAY_VOICE, newMode, device);
            isRouted = true;
            mCSCallActive = CS_INACTIVE;
        }
    break;
    case CS_ACTIVE:
        if (mCSCallActive == CS_INACTIVE) {
            ALOGD("doRouting: Enabling CS voice call ");
            enableVoiceCall((char *)SND_USE_CASE_VERB_VOICECALL,
                (char *)SND_USE_CASE_MOD_PLAY_VOICE, newMode, device);
            isRouted = true;
            mCSCallActive = CS_ACTIVE;
        } else if (mCSCallActive == CS_HOLD) {
             ALOGD("doRouting: Resume voice call from hold state");
             ALSAHandleList::iterator vt_it;
             for(vt_it = mDeviceList.begin();
                 vt_it != mDeviceList.end(); ++vt_it) {
                 if((!strncmp(vt_it->useCase, SND_USE_CASE_VERB_VOICECALL,
                     strlen(SND_USE_CASE_VERB_VOICECALL))) ||
                     (!strncmp(vt_it->useCase, SND_USE_CASE_MOD_PLAY_VOICE,
                     strlen(SND_USE_CASE_MOD_PLAY_VOICE)))) {
                     alsa_handle_t *handle = (alsa_handle_t *)(&(*vt_it));
                     mCSCallActive = CS_ACTIVE;
                     if(ioctl((int)handle->handle->fd,SNDRV_PCM_IOCTL_PAUSE,0)<0)
                                   ALOGE("VoLTE resume failed");
                     break;
                 }
             }
        }
    break;
    case CS_HOLD:
        if (mCSCallActive == CS_ACTIVE) {
            ALOGD("doRouting: Voice call going to Hold");
             ALSAHandleList::iterator vt_it;
             for(vt_it = mDeviceList.begin();
                 vt_it != mDeviceList.end(); ++vt_it) {
                 if((!strncmp(vt_it->useCase, SND_USE_CASE_VERB_VOICECALL,
                     strlen(SND_USE_CASE_VERB_VOICECALL))) ||
                     (!strncmp(vt_it->useCase, SND_USE_CASE_MOD_PLAY_VOICE,
                         strlen(SND_USE_CASE_MOD_PLAY_VOICE)))) {
                         mCSCallActive = CS_HOLD;
                         alsa_handle_t *handle = (alsa_handle_t *)(&(*vt_it));
                         if(ioctl((int)handle->handle->fd,SNDRV_PCM_IOCTL_PAUSE,1)<0)
                                   ALOGE("Voice pause failed");
                         break;
                }
            }
        }
    break;
    }
    return isRouted;
}
bool AudioHardwareALSA::routeVoLTECall(int device, int newMode)
{
int volteCallState = mCallState&0xF0;
bool isRouted = false;
switch (volteCallState) {
    case IMS_INACTIVE:
        if (mVolteCallActive != IMS_INACTIVE) {
            ALOGD("doRouting: Disabling IMS call");
            disableVoiceCall((char *)SND_USE_CASE_VERB_VOLTE,
                (char *)SND_USE_CASE_MOD_PLAY_VOLTE, newMode, device);
            isRouted = true;
            mVolteCallActive = IMS_INACTIVE;
        }
    break;
    case IMS_ACTIVE:
        if (mVolteCallActive == IMS_INACTIVE) {
            ALOGD("doRouting: Enabling IMS voice call ");
            enableVoiceCall((char *)SND_USE_CASE_VERB_VOLTE,
                (char *)SND_USE_CASE_MOD_PLAY_VOLTE, newMode, device);
            isRouted = true;
            mVolteCallActive = IMS_ACTIVE;
        } else if (mVolteCallActive == IMS_HOLD) {
             ALOGD("doRouting: Resume IMS call from hold state");
             ALSAHandleList::iterator vt_it;
             for(vt_it = mDeviceList.begin();
                 vt_it != mDeviceList.end(); ++vt_it) {
                 if((!strncmp(vt_it->useCase, SND_USE_CASE_VERB_VOLTE,
                     strlen(SND_USE_CASE_VERB_VOLTE))) ||
                     (!strncmp(vt_it->useCase, SND_USE_CASE_MOD_PLAY_VOLTE,
                     strlen(SND_USE_CASE_MOD_PLAY_VOLTE)))) {
                     alsa_handle_t *handle = (alsa_handle_t *)(&(*vt_it));
                     mVolteCallActive = IMS_ACTIVE;
                     if(ioctl((int)handle->handle->fd,SNDRV_PCM_IOCTL_PAUSE,0)<0)
                                   ALOGE("VoLTE resume failed");
                     break;
                 }
             }
        }
    break;
    case IMS_HOLD:
        if (mVolteCallActive == IMS_ACTIVE) {
             ALOGD("doRouting: IMS ACTIVE going to HOLD");
             ALSAHandleList::iterator vt_it;
             for(vt_it = mDeviceList.begin();
                 vt_it != mDeviceList.end(); ++vt_it) {
                 if((!strncmp(vt_it->useCase, SND_USE_CASE_VERB_VOLTE,
                     strlen(SND_USE_CASE_VERB_VOLTE))) ||
                     (!strncmp(vt_it->useCase, SND_USE_CASE_MOD_PLAY_VOLTE,
                         strlen(SND_USE_CASE_MOD_PLAY_VOLTE)))) {
                          mVolteCallActive = IMS_HOLD;
                         alsa_handle_t *handle = (alsa_handle_t *)(&(*vt_it));
                         if(ioctl((int)handle->handle->fd,SNDRV_PCM_IOCTL_PAUSE,1)<0)
                                   ALOGE("VoLTE Pause failed");
                    break;
                }
            }
        }
    break;
    }
    return isRouted;
}

}       // namespace android_audio_legacy
