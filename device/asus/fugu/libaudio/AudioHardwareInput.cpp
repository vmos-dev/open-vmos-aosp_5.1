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

#define LOG_TAG "AudioHAL:AudioHardwareInput"
#include <utils/Log.h>

#include <fcntl.h>
#include <sys/eventfd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <utils/String8.h>

#include "AudioHardwareInput.h"
#include "AudioHotplugThread.h"
#include "AudioStreamIn.h"

namespace android {

// Global singleton.
AudioHardwareInput gAudioHardwareInput;

AudioHardwareInput::AudioHardwareInput()
    : mMicMute(false)
{
    mHotplugThread = new AudioHotplugThread(*this);
    if (mHotplugThread == NULL) {
        ALOGE("Unable to create ATV Remote audio hotplug thread. "
              "Pluggable audio input devices will not function.");
    } else if (!mHotplugThread->start()) {
        ALOGE("Unable to start ATV Remote audio hotplug thread. "
              "Pluggable audio input devices will not function.");
        mHotplugThread.clear();
    }

    for (int i=0; i<kMaxDevices; i++) {
        mDeviceInfos[i].valid = false;
    }
}

AudioHardwareInput::~AudioHardwareInput()
{
    if (mHotplugThread != NULL) {
        mHotplugThread->shutdown();
        mHotplugThread.clear();
    }

    closeAllInputStreams();
}

status_t AudioHardwareInput::setMicMute(bool mute)
{
    mMicMute = mute;
    return NO_ERROR;
}

status_t AudioHardwareInput::getMicMute(bool* mute)
{
    *mute = mMicMute;
    return NO_ERROR;
}

// milliseconds per ALSA period
const uint32_t AudioHardwareInput::kPeriodMsec = 10;

size_t AudioHardwareInput::calculateInputBufferSize(uint32_t outputSampleRate,
                                                    audio_format_t format,
                                                    uint32_t channelCount)
{
    size_t size;

    // AudioFlinger expects audio buffers to be a multiple of 16 frames
    size = (kPeriodMsec * outputSampleRate) / 1000;
    size = ((size + 15) / 16) * 16;

    return size * channelCount * audio_bytes_per_sample(format);
}

status_t AudioHardwareInput::getInputBufferSize(const audio_config* config)
{
    size_t size = calculateInputBufferSize(config->sample_rate,
                                           config->format,
                                           audio_channel_count_from_in_mask(config->channel_mask));
    return size;
}

AudioStreamIn* AudioHardwareInput::openInputStream(uint32_t devices,
        audio_format_t* format, uint32_t* channelMask, uint32_t* sampleRate,
        status_t* status)
{
    (void) devices;
    Mutex::Autolock _l(mLock);

    AudioStreamIn* in;

    in = new AudioStreamIn(*this);
    if (in == NULL) {
        *status = NO_MEMORY;
        return NULL;
    }

    *status = in->set(format, channelMask, sampleRate);

    if (*status != NO_ERROR) {
        delete in;
        return NULL;
    }

    mInputStreams.add(in);

    return in;
}

void AudioHardwareInput::closeInputStream(AudioStreamIn* in)
{
    Mutex::Autolock _l(mLock);

    for (size_t i = 0; i < mInputStreams.size(); i++) {
        if (in == mInputStreams[i]) {
            mInputStreams.removeAt(i);
            in->standby();
            delete in;
            break;
        }
    }
}

void AudioHardwareInput::closeAllInputStreams()
{
    while (mInputStreams.size() != 0) {
        AudioStreamIn* in = mInputStreams[0];
        mInputStreams.removeAt(0);
        in->standby();
        delete in;
    }
}

void AudioHardwareInput::standbyAllInputStreams(const AudioHotplugThread::DeviceInfo* deviceInfo)
{
    for (size_t i = 0; i < mInputStreams.size(); i++) {
        if (deviceInfo == NULL || deviceInfo == mInputStreams[i]->getDeviceInfo()) {
            mInputStreams[i]->standby();
        }
    }
}

#define DUMP(a...) \
    snprintf(buffer, SIZE, a); \
    buffer[SIZE - 1] = 0; \
    result.append(buffer);
#define B2STR(b) b ? "true" : "false"

status_t AudioHardwareInput::dump(int fd)
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;

    DUMP("\nAudioHardwareInput::dump\n");

    for (int i=0; i<kMaxDevices; i++) {
        if (mDeviceInfos[i].valid) {
            DUMP("device[%d] is valid\n", i);
            DUMP("\tcapture card: %d\n", mDeviceInfos[i].pcmCard);
            DUMP("\tcapture device: %d\n", mDeviceInfos[i].pcmDevice);
        }
    }

    ::write(fd, result.string(), result.size());

    {
        Mutex::Autolock _l(mLock);
        for (size_t i = 0; i < mInputStreams.size(); i++) {
            mInputStreams[i]->dump(fd);
        }
    }

    return NO_ERROR;
}

#undef DUMP
#undef B2STR

// called on the audio hotplug thread
void AudioHardwareInput::onDeviceFound(
        const AudioHotplugThread::DeviceInfo& devInfo)
{
    bool foundSlot = false;
    Mutex::Autolock _l(mLock);

    ALOGD("AudioHardwareInput::onDeviceFound pcmCard = %d", devInfo.pcmCard);

    for (int i=0; i<kMaxDevices; i++) {
        if (mDeviceInfos[i].valid) {
            if ((mDeviceInfos[i].pcmCard == devInfo.pcmCard)
                && (mDeviceInfos[i].pcmDevice == devInfo.pcmDevice)) {
                ALOGW("AudioHardwareInput::onDeviceFound already has  %d:%d",
                    devInfo.pcmCard, devInfo.pcmDevice);
                return; // Got it already so no action needed.
            }
        }
    }

    // New device so find an empty slot and save it.
    for (int i=0; i<kMaxDevices; i++) {
        if (!mDeviceInfos[i].valid) {
            ALOGD("AudioHardwareInput::onDeviceFound saving as device #%d", i);
            mDeviceInfos[i] = devInfo;
            mDeviceInfos[i].valid = true;
            foundSlot = true;
            /* Restart any currently running streams. */
            standbyAllInputStreams(NULL);
            break;
        }
    }

    if (!foundSlot) {
        ALOGW("AudioHardwareInput::onDeviceFound found more devices than expected! Dropped");
    }
}

// called on the audio hotplug thread
void AudioHardwareInput::onDeviceRemoved(unsigned int pcmCard, unsigned int pcmDevice)
{
    Mutex::Autolock _l(mLock);

    ALOGD("AudioHardwareInput::onDeviceRemoved pcmCard = %d", pcmCard);
    // Find matching DeviceInfo.
    for (int i=0; i<kMaxDevices; i++) {
        if (mDeviceInfos[i].valid) {
            if ((mDeviceInfos[i].pcmCard == pcmCard) && (mDeviceInfos[i].pcmDevice == pcmDevice)) {
                ALOGD("AudioHardwareInput::onDeviceRemoved matches #%d", i);
                mDeviceInfos[i].valid = false;
                /* If currently active stream is using this device then restart. */
                standbyAllInputStreams(&mDeviceInfos[i]);
                break;
            }
        }
    }
}

const AudioHotplugThread::DeviceInfo* AudioHardwareInput::getBestDevice(int inputSource)
{
    bool doVoiceRecognition = (inputSource == AUDIO_SOURCE_VOICE_RECOGNITION);
    int chosenDeviceIndex = -1;
    Mutex::Autolock _l(mLock);

    ALOGD("AudioHardwareInput::getBestDevice inputSource = %d, doVoiceRecognition = %d",
        inputSource, (doVoiceRecognition ? 1 : 0));
    // RemoteControl is the only input device usable for voice recognition
    // and no other devices are used for voice recognition.
    // Currently the RemoteControl is the only device marked with forVoiceRecognition=true.
    // A connected USB mic could be used for anything but voice recognition.
    for (int i=0; i<kMaxDevices; i++) {
        if (mDeviceInfos[i].valid) {
            if (mDeviceInfos[i].forVoiceRecognition == doVoiceRecognition) {
                chosenDeviceIndex = i;
                break;
            }
        }
    }

    if (chosenDeviceIndex < 0) {
        ALOGE("ERROR AudioHardwareInput::getBestDevice, none for source %d", inputSource);
    } else {
        ALOGD("AudioHardwareInput::getBestDevice chose #%d", chosenDeviceIndex);
    }

    return (chosenDeviceIndex >= 0) ? &mDeviceInfos[chosenDeviceIndex] : NULL;
}

}; // namespace android
