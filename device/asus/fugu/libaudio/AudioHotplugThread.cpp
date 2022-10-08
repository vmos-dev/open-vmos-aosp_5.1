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

#define LOG_TAG "AudioHAL:AudioHotplugThread"
#include <utils/Log.h>

#include <assert.h>
#include <dirent.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <sys/inotify.h>
#include <sys/ioctl.h>

// Bionic's copy of asound.h contains references to these kernel macros.
// They need to be removed in order to include the file from userland.
#define __force
#define __bitwise
#define __user
#include <sound/asound.h>
#undef __force
#undef __bitwise
#undef __user

#include <utils/misc.h>
#include <utils/String8.h>

#include "AudioHotplugThread.h"

// This name is used to recognize the AndroidTV Remote mic so we can
// use it for voice recognition.
#define ANDROID_TV_REMOTE_AUDIO_DEVICE_NAME "ATVRAudio"

namespace android {

/*
 * ALSA parameter manipulation routines
 *
 * TODO: replace this when TinyAlsa offers a suitable API
 */

static inline int param_is_mask(int p)
{
    return (p >= SNDRV_PCM_HW_PARAM_FIRST_MASK) &&
        (p <= SNDRV_PCM_HW_PARAM_LAST_MASK);
}

static inline int param_is_interval(int p)
{
    return (p >= SNDRV_PCM_HW_PARAM_FIRST_INTERVAL) &&
        (p <= SNDRV_PCM_HW_PARAM_LAST_INTERVAL);
}

static inline struct snd_interval *param_to_interval(
        struct snd_pcm_hw_params *p, int n)
{
    assert(p->intervals);
    assert(param_is_interval(n));
    return &(p->intervals[n - SNDRV_PCM_HW_PARAM_FIRST_INTERVAL]);
}

static inline struct snd_mask *param_to_mask(struct snd_pcm_hw_params *p, int n)
{
    assert(p->masks);
    assert(param_is_mask(n));
    return &(p->masks[n - SNDRV_PCM_HW_PARAM_FIRST_MASK]);
}

static inline void snd_mask_any(struct snd_mask *mask)
{
    memset(mask, 0xff, sizeof(struct snd_mask));
}

static inline void snd_interval_any(struct snd_interval *i)
{
    i->min = 0;
    i->openmin = 0;
    i->max = UINT_MAX;
    i->openmax = 0;
    i->integer = 0;
    i->empty = 0;
}

static void param_init(struct snd_pcm_hw_params *p)
{
    int n, k;

    memset(p, 0, sizeof(*p));
    for (n = SNDRV_PCM_HW_PARAM_FIRST_MASK;
         n <= SNDRV_PCM_HW_PARAM_LAST_MASK; n++) {
        struct snd_mask *m = param_to_mask(p, n);
        snd_mask_any(m);
    }
    for (n = SNDRV_PCM_HW_PARAM_FIRST_INTERVAL;
         n <= SNDRV_PCM_HW_PARAM_LAST_INTERVAL; n++) {
        struct snd_interval *i = param_to_interval(p, n);
        snd_interval_any(i);
    }
    p->rmask = 0xFFFFFFFF;
}

/*
 * Hotplug thread
 */

const char* AudioHotplugThread::kThreadName = "ATVRemoteAudioHotplug";

// directory where ALSA device nodes appear
const char* AudioHotplugThread::kAlsaDeviceDir = "/dev/snd";

// filename suffix for ALSA nodes representing capture devices
const char  AudioHotplugThread::kDeviceTypeCapture = 'c';

AudioHotplugThread::AudioHotplugThread(Callback& callback)
    : mCallback(callback)
    , mShutdownEventFD(-1)
{
}

AudioHotplugThread::~AudioHotplugThread()
{
    if (mShutdownEventFD != -1) {
        ::close(mShutdownEventFD);
    }
}

bool AudioHotplugThread::start()
{
    mShutdownEventFD = eventfd(0, EFD_NONBLOCK);
    if (mShutdownEventFD == -1) {
        return false;
    }

    return (run(kThreadName) == NO_ERROR);
}

void AudioHotplugThread::shutdown()
{
    requestExit();
    uint64_t tmp = 1;
    ::write(mShutdownEventFD, &tmp, sizeof(tmp));
    join();
}

bool AudioHotplugThread::parseCaptureDeviceName(const char* name,
                                                unsigned int* card,
                                                unsigned int* device)
{
    char deviceType;
    int ret = sscanf(name, "pcmC%uD%u%c", card, device, &deviceType);
    return (ret == 3 && deviceType == kDeviceTypeCapture);
}

static inline void getAlsaParamInterval(const struct snd_pcm_hw_params& params,
                                        int n, unsigned int* min,
                                        unsigned int* max)
{
    struct snd_interval* interval = param_to_interval(
        const_cast<struct snd_pcm_hw_params*>(&params), n);
    *min = interval->min;
    *max = interval->max;
}

// This was hacked out of "alsa_utils.cpp".
static int s_get_alsa_card_name(char *name, size_t len, int card_id)
{
        int fd;
        int amt = -1;
        snprintf(name, len, "/proc/asound/card%d/id", card_id);
        fd = open(name, O_RDONLY);
        if (fd >= 0) {
            amt = read(fd, name, len - 1);
            if (amt > 0) {
                // replace the '\n' at the end of the proc file with '\0'
                name[amt - 1] = 0;
            }
            close(fd);
        }
        return amt;
}

bool AudioHotplugThread::getDeviceInfo(unsigned int pcmCard,
                                       unsigned int pcmDevice,
                                       DeviceInfo* info)
{
    bool result = false;
    int ret;
    int len;
    char cardName[64] = "";

    String8 devicePath = String8::format("%s/pcmC%dD%d%c",
            kAlsaDeviceDir, pcmCard, pcmDevice, kDeviceTypeCapture);

    ALOGD("AudioHotplugThread::getDeviceInfo opening %s", devicePath.string());
    int alsaFD = open(devicePath.string(), O_RDONLY);
    if (alsaFD == -1) {
        ALOGE("AudioHotplugThread::getDeviceInfo open failed for %s", devicePath.string());
        goto done;
    }

    // query the device's ALSA configuration space
    struct snd_pcm_hw_params params;
    param_init(&params);
    ret = ioctl(alsaFD, SNDRV_PCM_IOCTL_HW_REFINE, &params);
    if (ret == -1) {
        ALOGE("AudioHotplugThread: refine ioctl failed");
        goto done;
    }

    info->pcmCard = pcmCard;
    info->pcmDevice = pcmDevice;
    getAlsaParamInterval(params, SNDRV_PCM_HW_PARAM_SAMPLE_BITS,
                         &info->minSampleBits, &info->maxSampleBits);
    getAlsaParamInterval(params, SNDRV_PCM_HW_PARAM_CHANNELS,
                         &info->minChannelCount, &info->maxChannelCount);
    getAlsaParamInterval(params, SNDRV_PCM_HW_PARAM_RATE,
                         &info->minSampleRate, &info->maxSampleRate);

    // Ugly hack to recognize Remote mic and mark it for voice recognition
    info->forVoiceRecognition = false;
    len = s_get_alsa_card_name(cardName, sizeof(cardName), pcmCard);
    ALOGD("AudioHotplugThread get_alsa_card_name returned %d, %s", len, cardName);
    if (len > 0) {
        if (strcmp(ANDROID_TV_REMOTE_AUDIO_DEVICE_NAME, cardName) == 0) {
            ALOGD("AudioHotplugThread found Android TV remote mic on Card %d, for VOICE_RECOGNITION", pcmCard);
            info->forVoiceRecognition = true;
        }
    }

    result = true;

done:
    if (alsaFD != -1) {
        close(alsaFD);
    }
    return result;
}

// scan the ALSA device directory for a usable capture device
void AudioHotplugThread::scanForDevice()
{
    DIR* alsaDir;
    DeviceInfo deviceInfo;

    alsaDir = opendir(kAlsaDeviceDir);
    if (alsaDir == NULL)
        return;

    while (true) {
        struct dirent entry, *result;
        int ret = readdir_r(alsaDir, &entry, &result);
        if (ret != 0 || result == NULL)
            break;
        unsigned int pcmCard, pcmDevice;
        if (parseCaptureDeviceName(entry.d_name, &pcmCard, &pcmDevice)) {
            if (getDeviceInfo(pcmCard, pcmDevice, &deviceInfo)) {
                mCallback.onDeviceFound(deviceInfo);
            }
        }
    }

    closedir(alsaDir);
}

bool AudioHotplugThread::threadLoop()
{
    int inotifyFD = -1;
    int watchFD = -1;
    int flags;

    // watch for changes to the ALSA device directory
    inotifyFD = inotify_init();
    if (inotifyFD == -1) {
        ALOGE("AudioHotplugThread: inotify_init failed");
        goto done;
    }
    flags = fcntl(inotifyFD, F_GETFL, 0);
    if (flags == -1) {
        ALOGE("AudioHotplugThread: F_GETFL failed");
        goto done;
    }
    if (fcntl(inotifyFD, F_SETFL, flags | O_NONBLOCK) == -1) {
        ALOGE("AudioHotplugThread: F_SETFL failed");
        goto done;
    }

    watchFD = inotify_add_watch(inotifyFD, kAlsaDeviceDir,
                                IN_CREATE | IN_DELETE);
    if (watchFD == -1) {
        ALOGE("AudioHotplugThread: inotify_add_watch failed");
        goto done;
    }

    // check for any existing capture devices
    scanForDevice();

    while (!exitPending()) {
        // wait for a change to the ALSA directory or a shutdown signal
        struct pollfd fds[2] = {
            { inotifyFD, POLLIN, 0 },
            { mShutdownEventFD, POLLIN, 0 }
        };
        int ret = poll(fds, NELEM(fds), -1);
        if (ret == -1) {
            ALOGE("AudioHotplugThread: poll failed");
            break;
        } else if (fds[1].revents & POLLIN) {
            // shutdown requested
            break;
        }

        if (!(fds[0].revents & POLLIN)) {
            continue;
        }

        // parse the filesystem change events
        char eventBuf[256];
        ret = read(inotifyFD, eventBuf, sizeof(eventBuf));
        if (ret == -1) {
            ALOGE("AudioHotplugThread: read failed");
            break;
        }

        for (int i = 0; i < ret;) {
            if ((ret - i) < (int)sizeof(struct inotify_event)) {
                ALOGE("AudioHotplugThread: read an invalid inotify_event");
                break;
            }

            struct inotify_event *event =
                    reinterpret_cast<struct inotify_event*>(eventBuf + i);

            if ((ret - i) < (int)(sizeof(struct inotify_event) + event->len)) {
                ALOGE("AudioHotplugThread: read a bad inotify_event length");
                break;
            }

            char *name = ((char *) event) +
                    offsetof(struct inotify_event, name);

            unsigned int pcmCard, pcmDevice;
            if (parseCaptureDeviceName(name, &pcmCard, &pcmDevice)) {
                if (event->mask & IN_CREATE) {
                    // Some devices can not be opened immediately after the
                    // inotify event occurs.  Add a delay to avoid these
                    // races.  (50ms was chosen arbitrarily)
                    const int kOpenTimeoutMs = 50;
                    struct pollfd pfd = {mShutdownEventFD, POLLIN, 0};
                    if (poll(&pfd, 1, kOpenTimeoutMs) == -1) {
                        ALOGE("AudioHotplugThread: poll failed");
                        break;
                    } else if (pfd.revents & POLLIN) {
                        // shutdown requested
                        break;
                    }

                    DeviceInfo deviceInfo;
                    if (getDeviceInfo(pcmCard, pcmDevice, &deviceInfo)) {
                        mCallback.onDeviceFound(deviceInfo);
                    }
                } else if (event->mask & IN_DELETE) {
                    mCallback.onDeviceRemoved(pcmCard, pcmDevice);
                }
            }

            i += sizeof(struct inotify_event) + event->len;
        }
    }

done:
    if (watchFD != -1) {
        inotify_rm_watch(inotifyFD, watchFD);
    }
    if (inotifyFD != -1) {
        close(inotifyFD);
    }

    return false;
}

}; // namespace android
