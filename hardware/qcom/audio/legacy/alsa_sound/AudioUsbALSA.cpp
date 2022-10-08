/* AudioUsbALSA.cpp
Copyright (c) 2012, Code Aurora Forum. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of Code Aurora Forum, Inc. nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.*/

#define LOG_TAG "AudioUsbALSA"
#define LOG_NDEBUG 0
#define LOG_NDDEBUG 0
#include <utils/Log.h>
#include <utils/String8.h>

#include <cutils/properties.h>
#include <media/AudioRecord.h>
#include <hardware_legacy/power.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <jni.h>
#include <stdio.h>
#include <sys/eventfd.h>


#include "AudioUsbALSA.h"
struct pollfd pfdProxyPlayback[2];
struct pollfd pfdUsbPlayback[2];
struct pollfd pfdProxyRecording[1];
struct pollfd pfdUsbRecording[1];

#define USB_PERIOD_SIZE 2048
#define PROXY_PERIOD_SIZE 3072

namespace android_audio_legacy
{
AudioUsbALSA::AudioUsbALSA()
{
    mproxypfdPlayback = -1;
    musbpfdPlayback = -1;
    mkillPlayBackThread = false;
    mkillRecordingThread = false;
}

AudioUsbALSA::~AudioUsbALSA()
{
    mkillPlayBackThread = true;
    mkillRecordingThread = true;
}


int AudioUsbALSA::getnumOfRates(char *ratesStr){
    int i, size = 0;
    char *nextSRString, *temp_ptr;
    nextSRString = strtok_r(ratesStr, " ,", &temp_ptr);
    if (nextSRString == NULL) {
        ALOGE("ERROR: getnumOfRates: could not find rates string");
        return NULL;
    }
    for (i = 1; nextSRString != NULL; i++) {
        size ++;
        nextSRString = strtok_r(NULL, " ,.-", &temp_ptr);
    }
    return size;
}


status_t AudioUsbALSA::getCap(char * type, int &channels, int &sampleRate)
{
    ALOGD("getCap for %s",type);
    long unsigned fileSize;
    FILE *fp;
    char *buffer;
    int err = 1;
    int size = 0;
    int fd, i, lchannelsPlayback;
    char *read_buf, *str_start, *channel_start, *ratesStr, *ratesStrForVal,
    *ratesStrStart, *chString, *nextSRStr, *test, *nextSRString, *temp_ptr;
    struct stat st;
    memset(&st, 0x0, sizeof(struct stat));
    sampleRate = 0;
    fd = open(PATH, O_RDONLY);
    if (fd <0) {
        ALOGE("ERROR: failed to open config file %s error: %d\n", PATH, errno);
        close(fd);
        return UNKNOWN_ERROR;
    }

    if (fstat(fd, &st) < 0) {
        ALOGE("ERROR: failed to stat %s error %d\n", PATH, errno);
        close(fd);
        return UNKNOWN_ERROR;
    }

    fileSize = st.st_size;

    read_buf = (char *)malloc(BUFFSIZE);
    memset(read_buf, 0x0, BUFFSIZE);
    err = read(fd, read_buf, BUFFSIZE);
    str_start = strstr(read_buf, type);
    if (str_start == NULL) {
        ALOGE("ERROR:%s section not found in usb config file", type);
        close(fd);
        free(read_buf);
        return UNKNOWN_ERROR;
    }

    channel_start = strstr(str_start, "Channels:");
    if (channel_start == NULL) {
        ALOGE("ERROR: Could not find Channels information");
        close(fd);
        free(read_buf);
        return UNKNOWN_ERROR;
    }
    channel_start = strstr(channel_start, " ");
    if (channel_start == NULL) {
        ALOGE("ERROR: Channel section not found in usb config file");
        close(fd);
        free(read_buf);
        return UNKNOWN_ERROR;
    }

    lchannelsPlayback = atoi(channel_start);
    if (lchannelsPlayback == 1) {
        channels = 1;
    } else {
        channels = 2;
    }
    ALOGD("channels supported by device: %d", lchannelsPlayback);
    ratesStrStart = strstr(str_start, "Rates:");
    if (ratesStrStart == NULL) {
        ALOGE("ERROR: Cant find rates information");
        close(fd);
        free(read_buf);
        return UNKNOWN_ERROR;
    }

    ratesStrStart = strstr(ratesStrStart, " ");
    if (ratesStrStart == NULL) {
        ALOGE("ERROR: Channel section not found in usb config file");
        close(fd);
        free(read_buf);
        return UNKNOWN_ERROR;
    }

    //copy to ratesStr, current line.
    char *target = strchr(ratesStrStart, '\n');
    if (target == NULL) {
        ALOGE("ERROR: end of line not found");
        close(fd);
        free(read_buf);
        return UNKNOWN_ERROR;
    }
    size = target - ratesStrStart;
    ratesStr = (char *)malloc(size + 1) ;
    ratesStrForVal = (char *)malloc(size + 1) ;
    memcpy(ratesStr, ratesStrStart, size);
    memcpy(ratesStrForVal, ratesStrStart, size);
    ratesStr[size] = '\0';
    ratesStrForVal[size] = '\0';

    size = getnumOfRates(ratesStr);
    if (!size) {
        ALOGE("ERROR: Could not get rate size, returning");
        close(fd);
        free(ratesStrForVal);
        free(ratesStr);
        free(read_buf);
        return UNKNOWN_ERROR;
    }

    //populate playback rates array
    int ratesSupported[size];
    nextSRString = strtok_r(ratesStrForVal, " ,", &temp_ptr);
    if (nextSRString == NULL) {
        ALOGE("ERROR: Could not get first rate val");
        close(fd);
        free(ratesStrForVal);
        free(ratesStr);
        free(read_buf);
        return UNKNOWN_ERROR;
    }

    ratesSupported[0] = atoi(nextSRString);
    for (i = 1; i<size; i++) {
        nextSRString = strtok_r(NULL, " ,.-", &temp_ptr);
        ratesSupported[i] = atoi(nextSRString);
        ALOGV("ratesSupported[%d] for playback: %d",i, ratesSupported[i]);
    }

    for (i = 0; i<=size; i++) {
        if (ratesSupported[i] <= 48000) {
            sampleRate = ratesSupported[i];
            break;
        }
    }
    ALOGD("sampleRate: %d", sampleRate);

    close(fd);
    free(ratesStrForVal);
    free(ratesStr);
    free(read_buf);
    ratesStrForVal = NULL;
    ratesStr = NULL;
    read_buf = NULL;
    return NO_ERROR;
}

void AudioUsbALSA::exitPlaybackThread(uint64_t writeVal)
{
    ALOGD("exitPlaybackThread, mproxypfdPlayback: %d", mproxypfdPlayback);
    if (writeVal == SIGNAL_EVENT_KILLTHREAD) {
        int err;

        err = closeDevice(mproxyPlaybackHandle);
        if (err) {
            ALOGE("Info: Could not close proxy %p", mproxyPlaybackHandle);
        }
        err = closeDevice(musbPlaybackHandle);
        if (err) {
            ALOGE("Info: Could not close USB device %p", musbPlaybackHandle);
        }
    }
    if ((mproxypfdPlayback != -1) && (musbpfdPlayback != -1)) {
        write(mproxypfdPlayback, &writeVal, sizeof(uint64_t));
        write(musbpfdPlayback, &writeVal, sizeof(uint64_t));
        mkillPlayBackThread = true;
        pthread_join(mPlaybackUsb,NULL);
    }
}

void AudioUsbALSA::exitRecordingThread(uint64_t writeVal)
{
    ALOGD("exitRecordingThread");
    if (writeVal == SIGNAL_EVENT_KILLTHREAD) {
        int err;

        err = closeDevice(mproxyRecordingHandle);
        if (err) {
            ALOGE("Info: Could not close proxy for recording %p", mproxyRecordingHandle);
        }
        err = closeDevice(musbRecordingHandle);
        if (err) {
            ALOGE("Info: Could not close USB recording device %p", musbRecordingHandle);
        }
    }
    mkillRecordingThread = true;
}

void AudioUsbALSA::setkillUsbRecordingThread(bool val){
    ALOGD("setkillUsbRecordingThread");
    mkillRecordingThread = val;
}

status_t AudioUsbALSA::setHardwareParams(pcm *txHandle, uint32_t sampleRate, uint32_t channels, int periodBytes)
{
    ALOGD("setHardwareParams");
    struct snd_pcm_hw_params *params;
    unsigned long bufferSize, reqBuffSize;
    unsigned int periodTime, bufferTime;
    unsigned int requestedRate = sampleRate;
    int status = 0;

    params = (snd_pcm_hw_params*) calloc(1, sizeof(struct snd_pcm_hw_params));
    if (!params) {
        return NO_INIT;
    }

    param_init(params);
    param_set_mask(params, SNDRV_PCM_HW_PARAM_ACCESS,
                   SNDRV_PCM_ACCESS_MMAP_INTERLEAVED);
    param_set_mask(params, SNDRV_PCM_HW_PARAM_FORMAT,
                   SNDRV_PCM_FORMAT_S16_LE);
    param_set_mask(params, SNDRV_PCM_HW_PARAM_SUBFORMAT,
                   SNDRV_PCM_SUBFORMAT_STD);
    ALOGV("Setting period size:%d samplerate:%d, channels: %d",periodBytes,sampleRate, channels);
    param_set_min(params, SNDRV_PCM_HW_PARAM_PERIOD_BYTES, periodBytes);
    param_set_int(params, SNDRV_PCM_HW_PARAM_SAMPLE_BITS, 16);
    param_set_int(params, SNDRV_PCM_HW_PARAM_FRAME_BITS,
                  channels - 1 ? 32 : 16);
    param_set_int(params, SNDRV_PCM_HW_PARAM_CHANNELS,
                  channels);
    param_set_int(params, SNDRV_PCM_HW_PARAM_RATE, sampleRate);
    param_set_hw_refine(txHandle, params);

    if (param_set_hw_params(txHandle, params)) {
        ALOGE("ERROR: cannot set hw params");
        return NO_INIT;
    }

    param_dump(params);

    txHandle->period_size = pcm_period_size(params);
    txHandle->buffer_size = pcm_buffer_size(params);
    txHandle->period_cnt = txHandle->buffer_size/txHandle->period_size;

    ALOGD("setHardwareParams: buffer_size %d, period_size %d, period_cnt %d",
         txHandle->buffer_size, txHandle->period_size,
         txHandle->period_cnt);

    return NO_ERROR;
}

status_t AudioUsbALSA::setSoftwareParams(pcm *pcm, bool playback)
{
    ALOGD("setSoftwareParams");
    struct snd_pcm_sw_params* params;

    params = (snd_pcm_sw_params*) calloc(1, sizeof(struct snd_pcm_sw_params));
    if (!params) {
        LOG_ALWAYS_FATAL("Failed to allocate ALSA software parameters!");
        return NO_INIT;
    }

    params->tstamp_mode = SNDRV_PCM_TSTAMP_NONE;
    params->period_step = 1;

    params->avail_min = (pcm->flags & PCM_MONO) ? pcm->period_size/2 : pcm->period_size/4;

    if (playback) {
        params->start_threshold = (pcm->flags & PCM_MONO) ? pcm->period_size*8 : pcm->period_size*4;
        params->xfer_align = (pcm->flags & PCM_MONO) ? pcm->period_size*8 : pcm->period_size*4;
    } else {
        params->start_threshold = (pcm->flags & PCM_MONO) ? pcm->period_size/2 : pcm->period_size/4;
        params->xfer_align = (pcm->flags & PCM_MONO) ? pcm->period_size/2 : pcm->period_size/4;
    }
    params->stop_threshold = pcm->buffer_size;

    params->xfer_align = (pcm->flags & PCM_MONO) ? pcm->period_size/2 : pcm->period_size/4;
    params->silence_size = 0;
    params->silence_threshold = 0;

    if (param_set_sw_params(pcm, params)) {
        ALOGE("ERROR: cannot set sw params");
        return NO_INIT;
    }

    return NO_ERROR;
}

status_t AudioUsbALSA::closeDevice(pcm *handle)
{
    ALOGD("closeDevice handle %p", handle);
    status_t err = NO_ERROR;
    if (handle) {
        err = pcm_close(handle);
        if (err != NO_ERROR) {
            ALOGE("INFO: closeDevice: pcm_close failed with err %d", err);
        }
    }
    handle = NULL;
    return err;
}

void AudioUsbALSA::RecordingThreadEntry() {
    ALOGD("Inside RecordingThreadEntry");
    int nfds = 1;
    mtimeOutRecording = TIMEOUT_INFINITE;
    int fd;
    long frames;
    static int start = 0;
    struct snd_xferi x;
    int filed;
    unsigned avail, bufsize;
    int bytes_written;
    uint32_t sampleRate;
    uint32_t channels;
    u_int8_t *srcUsb_addr = NULL;
    u_int8_t *dstProxy_addr = NULL;
    int err;
    const char *fn = "/data/RecordPcm.pcm";
    filed = open(fn, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0664);

    err = getCap((char *)"Capture:", mchannelsCapture, msampleRateCapture);
    if (err) {
        ALOGE("ERROR: Could not get capture capabilities from usb device");
        return;
    }
    int channelFlag = PCM_MONO;
    if (mchannelsCapture >= 2) {
        channelFlag = PCM_STEREO;
    }

    musbRecordingHandle = configureDevice(PCM_IN|channelFlag|PCM_MMAP, (char *)"hw:1,0",
                                         msampleRateCapture, mchannelsCapture,768,false);
    if (!musbRecordingHandle) {
        ALOGE("ERROR: Could not configure USB device for recording");
        return;
    } else {
        ALOGD("USB device Configured for recording");
    }

    pfdUsbRecording[0].fd = musbRecordingHandle->fd;                           //DEBUG
    pfdUsbRecording[0].events = POLLIN;

    mproxyRecordingHandle = configureDevice(PCM_OUT|channelFlag|PCM_MMAP, (char *)"hw:0,7",
                                            msampleRateCapture, mchannelsCapture,768,false);
    if (!mproxyRecordingHandle) {
        ALOGE("ERROR: Could not configure Proxy for recording");
        closeDevice(musbRecordingHandle);
        return;
    } else {
        ALOGD("Proxy Configured for recording");
    }

    bufsize = musbRecordingHandle->period_size;
    pfdProxyRecording[0].fd = mproxyRecordingHandle->fd;
    pfdProxyRecording[0].events = POLLOUT;
    frames = (musbRecordingHandle->flags & PCM_MONO) ? (bufsize / 2) : (bufsize / 4);
    x.frames = (musbRecordingHandle->flags & PCM_MONO) ? (bufsize / 2) : (bufsize / 4);

    /***********************keep reading from usb and writing to proxy******************************************/
    while (mkillRecordingThread != true) {
        if (!musbRecordingHandle->running) {
            if (pcm_prepare(musbRecordingHandle)) {
                ALOGE("ERROR: pcm_prepare failed for usb device for recording");
                mkillRecordingThread = true;
                break;;
            }
        }
        if (!mproxyRecordingHandle->running) {
            if (pcm_prepare(mproxyRecordingHandle)) {
                ALOGE("ERROR: pcm_prepare failed for proxy device for recording");
                mkillRecordingThread = true;
                break;;
            }
        }

        /********** USB syncing before write **************/
        if (!musbRecordingHandle->start && !mkillRecordingThread) {
            err = startDevice(musbRecordingHandle, &mkillRecordingThread);
            if (err == EPIPE) {
                continue;
            } else if (err != NO_ERROR) {
                mkillRecordingThread = true;
                break;
            }
        }
        for (;;) {
            if (!musbRecordingHandle->running) {
                if (pcm_prepare(musbRecordingHandle)) {
                    ALOGE("ERROR: pcm_prepare failed for proxy device for recording");
                    mkillRecordingThread = true;
                    break;
                }
            }
            /* Sync the current Application pointer from the kernel */
            musbRecordingHandle->sync_ptr->flags = SNDRV_PCM_SYNC_PTR_APPL |
                                                   SNDRV_PCM_SYNC_PTR_AVAIL_MIN;

            err = syncPtr(musbRecordingHandle, &mkillRecordingThread);
            if (err == EPIPE) {
                continue;
            } else if (err != NO_ERROR) {
                break;
            }

            avail = pcm_avail(musbRecordingHandle);
            if (avail < musbRecordingHandle->sw_p->avail_min) {
                poll(pfdUsbRecording, nfds, TIMEOUT_INFINITE);
                continue;
            } else {
                break;
            }
        }
        if (mkillRecordingThread) {
            break;
        }
        if (x.frames > avail)
            frames = avail;

        srcUsb_addr = dst_address(musbRecordingHandle);
        /**********End USB syncing before write**************/

        /*************Proxy syncing before write ******************/

        for (;;) {
            if (!mproxyRecordingHandle->running) {
                if (pcm_prepare(mproxyRecordingHandle)) {
                    ALOGE("ERROR: pcm_prepare failed for proxy device for recording");
                    mkillRecordingThread = true;
                    break;
                }
            }
            mproxyRecordingHandle->sync_ptr->flags = SNDRV_PCM_SYNC_PTR_APPL |
                                                     SNDRV_PCM_SYNC_PTR_AVAIL_MIN;

            err = syncPtr(mproxyRecordingHandle, &mkillRecordingThread);
            if (err == EPIPE) {
                continue;
            } else if (err != NO_ERROR) {
                break;
            }
            avail = pcm_avail(mproxyRecordingHandle);
            if (avail < mproxyRecordingHandle->sw_p->avail_min) {
                poll(pfdProxyRecording, nfds, TIMEOUT_INFINITE);
                continue;
            } else {
                break;
            }
        }
        if (mkillRecordingThread) {
            break;
        }

        dstProxy_addr = dst_address(mproxyRecordingHandle);
        memset(dstProxy_addr, 0x0, bufsize);

        /**************End Proxy syncing before write *************/

        memcpy(dstProxy_addr, srcUsb_addr, bufsize );

        /************* sync up after write -- USB  *********************/
        musbRecordingHandle->sync_ptr->c.control.appl_ptr += frames;
        musbRecordingHandle->sync_ptr->flags = 0;
        err = syncPtr(musbRecordingHandle, &mkillRecordingThread);
        if (err == EPIPE) {
            continue;
        } else if (err != NO_ERROR) {
            break;
        }

        /************* end sync up after write -- USB *********************/

        /**************** sync up after write -- Proxy  ************************/
        mproxyRecordingHandle->sync_ptr->c.control.appl_ptr += frames;
        mproxyRecordingHandle->sync_ptr->flags = 0;

        err = syncPtr(mproxyRecordingHandle, &mkillRecordingThread);
        if (err == EPIPE) {
            continue;
        } else if (err != NO_ERROR) {
            break;
        }

        bytes_written = mproxyRecordingHandle->sync_ptr->c.control.appl_ptr - mproxyRecordingHandle->sync_ptr->s.status.hw_ptr;
        if ((bytes_written >= mproxyRecordingHandle->sw_p->start_threshold) && (!mproxyRecordingHandle->start)) {
            if (!mkillPlayBackThread) {
                err = startDevice(mproxyRecordingHandle, &mkillRecordingThread);
                if (err == EPIPE) {
                    continue;
                } else if (err != NO_ERROR) {
                    mkillRecordingThread = true;
                    break;
                }
            }
        }
    }
    /***************  End sync up after write -- Proxy *********************/
    if (mkillRecordingThread) {
        closeDevice(mproxyRecordingHandle);
        closeDevice(musbRecordingHandle);
    }
    ALOGD("Exiting USB Recording thread");
}

void *AudioUsbALSA::PlaybackThreadWrapper(void *me) {
    static_cast<AudioUsbALSA *>(me)->PlaybackThreadEntry();
    return NULL;
}

void *AudioUsbALSA::RecordingThreadWrapper(void *me) {
    static_cast<AudioUsbALSA *>(me)->RecordingThreadEntry();
    return NULL;
}

struct pcm * AudioUsbALSA::configureDevice(unsigned flags, char* hw, int sampleRate, int channelCount, int periodSize, bool playback){
    int err = NO_ERROR;
    struct pcm * handle = NULL;
    handle = pcm_open(flags, hw);
    if (!handle || handle->fd < 0) {
        ALOGE("ERROR: pcm_open failed");
        return NULL;
    }

    if (!pcm_ready(handle)) {
        ALOGE("ERROR: pcm_ready failed");
        closeDevice(handle);
        return NULL;
    }

    ALOGD("Setting hardware params: sampleRate:%d, channels: %d",sampleRate, channelCount);
    err = setHardwareParams(handle, sampleRate, channelCount,periodSize);
    if (err != NO_ERROR) {
        ALOGE("ERROR: setHardwareParams failed");
        closeDevice(handle);
        return NULL;
    }

    err = setSoftwareParams(handle, playback);
    if (err != NO_ERROR) {
        ALOGE("ERROR: setSoftwareParams failed");
        closeDevice(handle);
        return NULL;
    }

    err = mmap_buffer(handle);
    if (err) {
        ALOGE("ERROR: mmap_buffer failed");
        closeDevice(handle);
        return NULL;
    }

    err = pcm_prepare(handle);
    if (err) {
        ALOGE("ERROR: pcm_prepare failed");
        closeDevice(handle);
        return NULL;
    }

    return handle;
}

status_t AudioUsbALSA::startDevice(pcm *handle, bool *killThread) {
    int err = NO_ERROR;;
    if (ioctl(handle->fd, SNDRV_PCM_IOCTL_START)) {
        err = -errno;
        if (errno == EPIPE) {
            ALOGE("ERROR: SNDRV_PCM_IOCTL_START returned EPIPE for usb recording case");
            handle->underruns++;
            handle->running = 0;
            handle->start = 0;
            return errno;
        } else {
            ALOGE("ERROR: SNDRV_PCM_IOCTL_START failed for usb recording case errno:%d", errno);
            *killThread = true;
            return errno;
        }
    }
    handle->start = 1;
    if (handle == musbRecordingHandle) {
        ALOGD("Usb Driver started for recording");
    } else if (handle == mproxyRecordingHandle) {
        ALOGD("Proxy Driver started for recording");
    } else if (handle == musbPlaybackHandle) {
        ALOGD("Usb Driver started for playback");
    } else if (handle == mproxyPlaybackHandle) {
        ALOGD("proxy Driver started for playback");
    }
    return NO_ERROR;
}

status_t AudioUsbALSA::syncPtr(struct pcm *handle, bool *killThread) {
    int err;
    err = sync_ptr(handle);
    if (err == EPIPE) {
        ALOGE("ERROR: Failed in sync_ptr \n");
        handle->running = 0;
        handle->underruns++;
        handle->start = 0;
    } else if (err == ENODEV) {
        ALOGE("Info: Device not available");
    } else if (err != NO_ERROR) {
        ALOGE("ERROR: Sync ptr returned %d", err);
        *killThread = true;
    }
    return err;
}

void AudioUsbALSA::pollForProxyData(){
    int err_poll = poll(pfdProxyPlayback, mnfdsPlayback, mtimeOut);
    if (err_poll == 0 ) {
        ALOGD("POLL timedout");
        mkillPlayBackThread = true;
        pfdProxyPlayback[0].revents = 0;
        pfdProxyPlayback[1].revents = 0;
        return;
    }

    if (pfdProxyPlayback[1].revents & POLLIN) {
        ALOGD("Signalled from HAL about timeout");
        uint64_t u;
        read(mproxypfdPlayback, &u, sizeof(uint64_t));
        pfdProxyPlayback[1].revents = 0;
        if (u == SIGNAL_EVENT_KILLTHREAD) {
            ALOGD("kill thread event");
            mkillPlayBackThread = true;
            pfdProxyPlayback[0].revents = 0;
            pfdProxyPlayback[1].revents = 0;
            return;
        } else if (u == SIGNAL_EVENT_TIMEOUT) {
            ALOGD("Setting timeout for 3 sec");
            mtimeOut = POLL_TIMEOUT;
        }
    } else if (pfdProxyPlayback[1].revents & POLLERR || pfdProxyPlayback[1].revents & POLLHUP ||
               pfdProxyPlayback[1].revents & POLLNVAL) {
        ALOGE("Info: proxy throwing error from location 1");
        mkillPlayBackThread = true;
        pfdProxyPlayback[0].revents = 0;
        pfdProxyPlayback[1].revents = 0;
        return;
    }

    if (pfdProxyPlayback[0].revents & POLLERR || pfdProxyPlayback[0].revents & POLLHUP ||
        pfdProxyPlayback[0].revents & POLLNVAL) {
        ALOGE("Info: proxy throwing error");
        mkillPlayBackThread = true;
        pfdProxyPlayback[0].revents = 0;
        pfdProxyPlayback[1].revents = 0;
    }
}

void AudioUsbALSA::pollForUsbData(){
    int err_poll = poll(pfdUsbPlayback, mnfdsPlayback, mtimeOut);
    if (err_poll == 0 ) {
        ALOGD("POLL timedout");
        mkillPlayBackThread = true;
        pfdUsbPlayback[0].revents = 0;
        pfdUsbPlayback[1].revents = 0;
        return;
    }

    if (pfdUsbPlayback[1].revents & POLLIN) {
        ALOGD("Info: Signalled from HAL about an event");
        uint64_t u;
        read(musbpfdPlayback, &u, sizeof(uint64_t));
        pfdUsbPlayback[0].revents = 0;
        pfdUsbPlayback[1].revents = 0;
        if (u == SIGNAL_EVENT_KILLTHREAD) {
            ALOGD("kill thread");
            mkillPlayBackThread = true;
            return;
        } else if (u == SIGNAL_EVENT_TIMEOUT) {
            ALOGD("Setting timeout for 3 sec");
            mtimeOut = POLL_TIMEOUT;
        }
    } else if (pfdUsbPlayback[1].revents & POLLERR || pfdUsbPlayback[1].revents & POLLHUP ||
               pfdUsbPlayback[1].revents & POLLNVAL) {
        ALOGE("Info: usb throwing error from location 1");
        mkillPlayBackThread = true;
        pfdUsbPlayback[0].revents = 0;
        pfdUsbPlayback[1].revents = 0;
        return;
    }

    if (pfdUsbPlayback[0].revents & POLLERR || pfdProxyPlayback[0].revents & POLLHUP ||
        pfdUsbPlayback[0].revents & POLLNVAL) {
        ALOGE("Info: usb throwing error");
        mkillPlayBackThread = true;
        pfdUsbPlayback[0].revents = 0;
        return;
    }
}

void AudioUsbALSA::PlaybackThreadEntry() {
    ALOGD("PlaybackThreadEntry");
    mnfdsPlayback = 2;
    mtimeOut = TIMEOUT_INFINITE;
    long frames;
    static int fd;
    struct snd_xferi x;
    int bytes_written;
    unsigned avail, xfer, bufsize;
    unsigned proxyPeriod, usbPeriod;
    uint32_t sampleRate;
    uint32_t channels;
    unsigned int tmp;
    int numOfBytesWritten;
    int err;
    int filed;
    const char *fn = "/data/test.pcm";
    mdstUsb_addr = NULL;
    msrcProxy_addr = NULL;

    int proxySizeRemaining = 0;
    int usbSizeFilled = 0;

    pid_t tid  = gettid();
    androidSetThreadPriority(tid, ANDROID_PRIORITY_URGENT_AUDIO);

    err = getCap((char *)"Playback:", mchannelsPlayback, msampleRatePlayback);
    if (err) {
        ALOGE("ERROR: Could not get playback capabilities from usb device");
        return;
    }

    musbPlaybackHandle = configureDevice(PCM_OUT|PCM_STEREO|PCM_MMAP, (char *)"hw:1,0",
                                         msampleRatePlayback, mchannelsPlayback, USB_PERIOD_SIZE, true);
    if (!musbPlaybackHandle) {
        ALOGE("ERROR: configureUsbDevice failed, returning");
        closeDevice(musbPlaybackHandle);
        return;
    } else {
        ALOGD("USB Configured for playback");
    }

    if (!mkillPlayBackThread) {
        pfdUsbPlayback[0].fd = musbPlaybackHandle->timer_fd;
        pfdUsbPlayback[0].events = POLLIN;
        musbpfdPlayback = eventfd(0,0);
        pfdUsbPlayback[1].fd = musbpfdPlayback;
        pfdUsbPlayback[1].events = (POLLIN | POLLOUT | POLLERR | POLLNVAL | POLLHUP);
    }

    mproxyPlaybackHandle = configureDevice(PCM_IN|PCM_STEREO|PCM_MMAP, (char *)"hw:0,8",
                               msampleRatePlayback, mchannelsPlayback, PROXY_PERIOD_SIZE, false);
    if (!mproxyPlaybackHandle) {
        ALOGE("ERROR: Could not configure Proxy, returning");
        closeDevice(musbPlaybackHandle);
        return;
    } else {
        ALOGD("Proxy Configured for playback");
    }

    proxyPeriod = mproxyPlaybackHandle->period_size;
    usbPeriod = musbPlaybackHandle->period_size;

    if (!mkillPlayBackThread) {
        pfdProxyPlayback[0].fd = mproxyPlaybackHandle->fd;
        pfdProxyPlayback[0].events = (POLLIN);                                 // | POLLERR | POLLNVAL);
        mproxypfdPlayback = eventfd(0,0);
        pfdProxyPlayback[1].fd = mproxypfdPlayback;
        pfdProxyPlayback[1].events = (POLLIN | POLLOUT| POLLERR | POLLNVAL);
    }

    frames = (mproxyPlaybackHandle->flags & PCM_MONO) ? (proxyPeriod / 2) : (proxyPeriod / 4);
    x.frames = (mproxyPlaybackHandle->flags & PCM_MONO) ? (proxyPeriod / 2) : (proxyPeriod / 4);
    int usbframes = (musbPlaybackHandle->flags & PCM_MONO) ? (usbPeriod / 2) : (usbPeriod / 4);

    u_int8_t *proxybuf = ( u_int8_t *) malloc(PROXY_PERIOD_SIZE);
    u_int8_t *usbbuf = ( u_int8_t *) malloc(USB_PERIOD_SIZE);
    memset(proxybuf, 0x0, PROXY_PERIOD_SIZE);
    memset(usbbuf, 0x0, USB_PERIOD_SIZE);


    /***********************keep reading from proxy and writing to USB******************************************/
    while (mkillPlayBackThread != true) {
        if (!mproxyPlaybackHandle->running) {
            if (pcm_prepare(mproxyPlaybackHandle)) {
                ALOGE("ERROR: pcm_prepare failed for proxy");
                mkillPlayBackThread = true;
                break;
            }
        }
        if (!musbPlaybackHandle->running) {
            if (pcm_prepare(musbPlaybackHandle)) {
                ALOGE("ERROR: pcm_prepare failed for usb");
                mkillPlayBackThread = true;
                break;
            }
        }

        /********** Proxy syncing before write **************/
        if (!mkillPlayBackThread && (!mproxyPlaybackHandle->start)) {
            err = startDevice(mproxyPlaybackHandle, &mkillPlayBackThread);
            if (err == EPIPE) {
                continue;
            } else if (err != NO_ERROR) {
                mkillPlayBackThread = true;
                break;
            }
        }
        if (proxySizeRemaining == 0) {
            for (;;) {
                if (!mproxyPlaybackHandle->running) {
                    if (pcm_prepare(mproxyPlaybackHandle)) {
                        ALOGE("ERROR: pcm_prepare failed for proxy");
                        mkillPlayBackThread = true;
                        break;
                    }
                }
                /* Sync the current Application pointer from the kernel */
                mproxyPlaybackHandle->sync_ptr->flags = SNDRV_PCM_SYNC_PTR_APPL |
                                                        SNDRV_PCM_SYNC_PTR_AVAIL_MIN;

                if (mtimeOut == TIMEOUT_INFINITE && !mkillPlayBackThread) {
                    err = syncPtr(mproxyPlaybackHandle, &mkillPlayBackThread);
                    if (err == EPIPE) {
                        continue;
                    } else if (err != NO_ERROR) {
                        break;
                    }
                    avail = pcm_avail(mproxyPlaybackHandle);
                }
                if (avail < mproxyPlaybackHandle->sw_p->avail_min && !mkillPlayBackThread) {
                    pollForProxyData();
                    //if polling returned some error
                    if (!mkillPlayBackThread) {
                        continue;
                    } else {
                        break;
                    }
                } else {                                                           //Got some data or mkillPlayBackThread is true
                    break;
                }
            }
            if (mkillPlayBackThread) {
                break;
            }

            if (x.frames > avail)
                frames = avail;

            if (!mkillPlayBackThread) {
                msrcProxy_addr = dst_address(mproxyPlaybackHandle);
                memcpy(proxybuf, msrcProxy_addr, proxyPeriod );

                x.frames -= frames;
                mproxyPlaybackHandle->sync_ptr->c.control.appl_ptr += frames;
                mproxyPlaybackHandle->sync_ptr->flags = 0;
                proxySizeRemaining = proxyPeriod;
            }

            if (!mkillPlayBackThread) {
                err = syncPtr(mproxyPlaybackHandle, &mkillPlayBackThread);
                if (err == EPIPE) {
                    continue;
                } else if (err != NO_ERROR) {
                    break;
                }
            }
        }
        //ALOGE("usbSizeFilled %d, proxySizeRemaining %d ",usbSizeFilled,proxySizeRemaining);
        if (usbPeriod - usbSizeFilled <= proxySizeRemaining) {
            memcpy(usbbuf + usbSizeFilled, proxybuf + proxyPeriod - proxySizeRemaining, usbPeriod - usbSizeFilled);
            proxySizeRemaining -= (usbPeriod - usbSizeFilled);
            usbSizeFilled = usbPeriod;
        }
        else {
            memcpy(usbbuf + usbSizeFilled, proxybuf + proxyPeriod - proxySizeRemaining,proxySizeRemaining);
            usbSizeFilled += proxySizeRemaining;
            proxySizeRemaining = 0;
        }

        if (usbSizeFilled == usbPeriod) {
            for (;;) {
                if (!musbPlaybackHandle->running) {
                    if (pcm_prepare(musbPlaybackHandle)) {
                        ALOGE("ERROR: pcm_prepare failed for usb");
                        mkillPlayBackThread = true;
                        break;
                    }
                }
                /*************USB syncing before write ******************/
                musbPlaybackHandle->sync_ptr->flags = SNDRV_PCM_SYNC_PTR_APPL |
                                                      SNDRV_PCM_SYNC_PTR_AVAIL_MIN;
                if (mtimeOut == TIMEOUT_INFINITE && !mkillPlayBackThread) {
                    err = syncPtr(musbPlaybackHandle, &mkillPlayBackThread);
                    if (err == EPIPE) {
                        continue;
                    } else if (err != NO_ERROR) {
                        break;
                    }
                    avail = pcm_avail(musbPlaybackHandle);
                    //ALOGV("Avail USB is: %d", avail);
                }

                if (avail < musbPlaybackHandle->sw_p->avail_min && !mkillPlayBackThread) {
                    pollForUsbData();
                    if (!mkillPlayBackThread) {
                        continue;
                    } else {
                        break;
                    }
                } else {
                    break;
                }
            }
            if (mkillPlayBackThread) {
                break;
            }

            if (!mkillPlayBackThread) {
                mdstUsb_addr = dst_address(musbPlaybackHandle);

                /**************End USB syncing before write *************/

                memcpy(mdstUsb_addr, usbbuf, usbPeriod );
                usbSizeFilled = 0;
                memset(usbbuf, 0x0, usbPeriod);
            }

            /**************** sync up after write -- USB  ************************/
            musbPlaybackHandle->sync_ptr->c.control.appl_ptr += usbframes;
            musbPlaybackHandle->sync_ptr->flags = 0;
            if (!mkillPlayBackThread) {
                err = syncPtr(musbPlaybackHandle, &mkillPlayBackThread);
                if (err == EPIPE) {
                    continue;
                } else if (err != NO_ERROR) {
                    break;
                }
            }

            bytes_written = musbPlaybackHandle->sync_ptr->c.control.appl_ptr - musbPlaybackHandle->sync_ptr->s.status.hw_ptr;
            ALOGE("Appl ptr %d , hw_ptr %d, difference %d",musbPlaybackHandle->sync_ptr->c.control.appl_ptr, musbPlaybackHandle->sync_ptr->s.status.hw_ptr, bytes_written);

            /*
                Following is the check to prevent USB from going to bad state.
                This happens in case of an underrun where there is not enough
                data from the proxy
            */
	    if (bytes_written <= usbPeriod && musbPlaybackHandle->start) {
                ioctl(musbPlaybackHandle->fd, SNDRV_PCM_IOCTL_PAUSE,1);
                pcm_prepare(musbPlaybackHandle);
                musbPlaybackHandle->start = false;
                continue;
            }
            if ((bytes_written >= musbPlaybackHandle->sw_p->start_threshold) && (!musbPlaybackHandle->start)) {
                if (!mkillPlayBackThread) {
                    err = startDevice(musbPlaybackHandle, &mkillPlayBackThread);
                    if (err == EPIPE) {
                        continue;
                    } else if (err != NO_ERROR) {
                        mkillPlayBackThread = true;
                        break;
                    }
                }
            }
            /***************  End sync up after write -- USB *********************/
        }
    }
    if (mkillPlayBackThread) {
        if (proxybuf)
            free(proxybuf);
        if (usbbuf)
            free(usbbuf);
        mproxypfdPlayback = -1;
        musbpfdPlayback = -1;
        closeDevice(mproxyPlaybackHandle);
        closeDevice(musbPlaybackHandle);
    }
    ALOGD("Exiting USB Playback Thread");
}

void AudioUsbALSA::startPlayback()
{
    mkillPlayBackThread = false;
    ALOGD("Creating USB Playback Thread");
    pthread_create(&mPlaybackUsb, NULL, PlaybackThreadWrapper, this);
}

void AudioUsbALSA::startRecording()
{
    //create Thread
    mkillRecordingThread = false;
    ALOGV("Creating USB recording Thread");
    pthread_create(&mRecordingUsb, NULL, RecordingThreadWrapper, this);
}
}
