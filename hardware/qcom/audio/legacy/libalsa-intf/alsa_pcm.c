/*
** Copyright 2010, The Android Open-Source Project
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

#define LOG_TAG "alsa_pcm"
#define LOG_NDEBUG 1
#ifdef ANDROID
/* definitions for Android logging */
#include <utils/Log.h>
#include <cutils/properties.h>
#else /* ANDROID */
#define strlcat g_strlcat
#define strlcpy g_strlcpy
#define ALOGI(...)      fprintf(stdout, __VA_ARGS__)
#define ALOGE(...)      fprintf(stderr, __VA_ARGS__)
#define ALOGV(...)      fprintf(stderr, __VA_ARGS__)
#endif /* ANDROID */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <linux/ioctl.h>
#include <linux/types.h>

#include "alsa_audio.h"

#define __force
#define __bitwise
#define __user

#define DEBUG 1

enum format_alias {
      S8 = 0,
      U8,
      S16_LE,
      S16_BE,
      U16_LE,
      U16_BE,
      S24_LE,
      S24_BE,
      U24_LE,
      U24_BE,
      S32_LE,
      S32_BE,
      U32_LE,
      U32_BE,
      FLOAT_LE,
      FLOAT_BE,
      FLOAT64_LE,
      FLOAT64_BE,
      IEC958_SUBFRAME_LE,
      IEC958_SUBFRAME_BE,
      MU_LAW,
      A_LAW,
      IMA_ADPCM,
      MPEG,
      GSM,
      SPECIAL = 31, 
      S24_3LE,
      S24_3BE,
      U24_3LE,
      U24_3BE,
      S20_3LE,
      S20_3BE,
      U20_3LE,
      U20_3BE,
      S18_3LE,
      S18_3BE,
      U18_3LE,
      U18_3BE,
      FORMAT_LAST,
};
const char *formats_list[][2] = {
        {"S8", "Signed 8 bit"},
        {"U8", "Unsigned 8 bit"},
        {"S16_LE", "Signed 16 bit Little Endian"},
        {"S16_BE", "Signed 16 bit Big Endian"},
        {"U16_LE", "Unsigned 16 bit Little Endian"},
        {"U16_BE", "Unsigned 16 bit Big Endian"},
        {"S24_LE", "Signed 24 bit Little Endian"},
        {"S24_BE", "Signed 24 bit Big Endian"},
        {"U24_LE", "Unsigned 24 bit Little Endian"},
        {"U24_BE", "Unsigned 24 bit Big Endian"},
        {"S32_LE", "Signed 32 bit Little Endian"},
        {"S32_BE", "Signed 32 bit Big Endian"},
        {"U32_LE", "Unsigned 32 bit Little Endian"},
        {"U32_BE", "Unsigned 32 bit Big Endian"},
        {"FLOAT_LE", "Float 32 bit Little Endian"},
        {"FLOAT_BE", "Float 32 bit Big Endian"},
        {"FLOAT64_LE", "Float 64 bit Little Endian"},
        {"FLOAT64_BE", "Float 64 bit Big Endian"},
        {"IEC958_SUBFRAME_LE", "IEC-958 Little Endian"},
        {"IEC958_SUBFRAME_BE", "IEC-958 Big Endian"},
        {"MU_LAW", "Mu-Law"},
        {"A_LAW", "A-Law"},
        {"IMA_ADPCM", "Ima-ADPCM"},
        {"MPEG", "MPEG"},
        {"GSM", "GSM"}, 
        [31] = {"SPECIAL", "Special"},
        {"S24_3LE", "Signed 24 bit Little Endian in 3bytes"},
        {"S24_3BE", "Signed 24 bit Big Endian in 3bytes"},
        {"U24_3LE", "Unsigned 24 bit Little Endian in 3bytes"},
        {"U24_3BE", "Unsigned 24 bit Big Endian in 3bytes"},
        {"S20_3LE", "Signed 20 bit Little Endian in 3bytes"},
        {"S20_3BE", "Signed 20 bit Big Endian in 3bytes"},
        {"U20_3LE", "Unsigned 20 bit Little Endian in 3bytes"},
        {"U20_3BE", "Unsigned 20 bit Big Endian in 3bytes"},
        {"S18_3LE", "Signed 18 bit Little Endian in 3bytes"},
        {"S18_3BE", "Signed 18 bit Big Endian in 3bytes"},
        {"U18_3LE", "Unsigned 18 bit Little Endian in 3bytes"},
        {"U18_3BE", "Unsigned 18 bit Big Endian in 3bytes"},
};

int get_compressed_format(const char *format)
{
        const char *ch = format;
        if (strcmp(ch, "MP3") == 0) {
                printf("MP3 is selected\n");
                return FORMAT_MP3;
        } else if (strcmp(ch, "AC3_PASS_THROUGH") == 0) {
                printf("AC3 PASS THROUGH is selected\n");
                return FORMAT_AC3_PASS_THROUGH;
        } else {
                printf("invalid format\n");
                return -1;
        }
        return 0;
}

int get_format(const char* name)
{
        int format;
        for (format = 0; format < FORMAT_LAST; format++) {
                if (formats_list[format][0] &&
                    strcasecmp(name, formats_list[format][0]) == 0) {
                        ALOGV("format_names %s", name);
                        return  format;
                }
        }
        return -EINVAL;
}

const char *get_format_name(int format)
{
        if ((format < FORMAT_LAST) &&
             formats_list[format][0])
            return formats_list[format][0];
        return NULL;
}

const char *get_format_desc(int format)
{
        if ((format < FORMAT_LAST) &&
             formats_list[format][1])
            return formats_list[format][1];
        return NULL;
}

/* alsa parameter manipulation cruft */

#define PARAM_MAX SNDRV_PCM_HW_PARAM_LAST_INTERVAL
static int oops(struct pcm *pcm, int e, const char *fmt, ...);

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

static inline struct snd_interval *param_to_interval(struct snd_pcm_hw_params *p, int n)
{
    return &(p->intervals[n - SNDRV_PCM_HW_PARAM_FIRST_INTERVAL]);
}

static inline struct snd_mask *param_to_mask(struct snd_pcm_hw_params *p, int n)
{
    return &(p->masks[n - SNDRV_PCM_HW_PARAM_FIRST_MASK]);
}

void param_set_mask(struct snd_pcm_hw_params *p, int n, unsigned bit)
{
    if (bit >= SNDRV_MASK_MAX)
        return;
    if (param_is_mask(n)) {
        struct snd_mask *m = param_to_mask(p, n);
        m->bits[0] = 0;
        m->bits[1] = 0;
        m->bits[bit >> 5] |= (1 << (bit & 31));
    }
}

void param_set_min(struct snd_pcm_hw_params *p, int n, unsigned val)
{
    if (param_is_interval(n)) {
        struct snd_interval *i = param_to_interval(p, n);
        i->min = val;
    }
}

void param_set_max(struct snd_pcm_hw_params *p, int n, unsigned val)
{
    if (param_is_interval(n)) {
        struct snd_interval *i = param_to_interval(p, n);
        i->max = val;
    }
}

void param_set_int(struct snd_pcm_hw_params *p, int n, unsigned val)
{
    if (param_is_interval(n)) {
        struct snd_interval *i = param_to_interval(p, n);
        i->min = val;
        i->max = val;
        i->integer = 1;
    }
}

void param_init(struct snd_pcm_hw_params *p)
{
    int n;
    memset(p, 0, sizeof(*p));
    for (n = SNDRV_PCM_HW_PARAM_FIRST_MASK;
         n <= SNDRV_PCM_HW_PARAM_LAST_MASK; n++) {
            struct snd_mask *m = param_to_mask(p, n);
            m->bits[0] = ~0;
            m->bits[1] = ~0;
    }
    for (n = SNDRV_PCM_HW_PARAM_FIRST_INTERVAL;
         n <= SNDRV_PCM_HW_PARAM_LAST_INTERVAL; n++) {
            struct snd_interval *i = param_to_interval(p, n);
            i->min = 0;
            i->max = ~0;
    }
}

/* debugging gunk */

#if DEBUG
static const char *param_name[PARAM_MAX+1] = {
    [SNDRV_PCM_HW_PARAM_ACCESS] = "access",
    [SNDRV_PCM_HW_PARAM_FORMAT] = "format",
    [SNDRV_PCM_HW_PARAM_SUBFORMAT] = "subformat",

    [SNDRV_PCM_HW_PARAM_SAMPLE_BITS] = "sample_bits",
    [SNDRV_PCM_HW_PARAM_FRAME_BITS] = "frame_bits",
    [SNDRV_PCM_HW_PARAM_CHANNELS] = "channels",
    [SNDRV_PCM_HW_PARAM_RATE] = "rate",
    [SNDRV_PCM_HW_PARAM_PERIOD_TIME] = "period_time",
    [SNDRV_PCM_HW_PARAM_PERIOD_SIZE] = "period_size",
    [SNDRV_PCM_HW_PARAM_PERIOD_BYTES] = "period_bytes",
    [SNDRV_PCM_HW_PARAM_PERIODS] = "periods",
    [SNDRV_PCM_HW_PARAM_BUFFER_TIME] = "buffer_time",
    [SNDRV_PCM_HW_PARAM_BUFFER_SIZE] = "buffer_size",
    [SNDRV_PCM_HW_PARAM_BUFFER_BYTES] = "buffer_bytes",
    [SNDRV_PCM_HW_PARAM_TICK_TIME] = "tick_time",
};

void param_dump(struct snd_pcm_hw_params *p)
{
    int n;

    for (n = SNDRV_PCM_HW_PARAM_FIRST_MASK;
         n <= SNDRV_PCM_HW_PARAM_LAST_MASK; n++) {
            struct snd_mask *m = param_to_mask(p, n);
            ALOGV("%s = %08x%08x\n", param_name[n],
                   m->bits[1], m->bits[0]);
    }
    for (n = SNDRV_PCM_HW_PARAM_FIRST_INTERVAL;
         n <= SNDRV_PCM_HW_PARAM_LAST_INTERVAL; n++) {
            struct snd_interval *i = param_to_interval(p, n);
            ALOGV("%s = (%d,%d) omin=%d omax=%d int=%d empty=%d\n",
                   param_name[n], i->min, i->max, i->openmin,
                   i->openmax, i->integer, i->empty);
    }
    ALOGV("info = %08x\n", p->info);
    ALOGV("msbits = %d\n", p->msbits);
    ALOGV("rate = %d/%d\n", p->rate_num, p->rate_den);
    ALOGV("fifo = %d\n", (int) p->fifo_size);
}

static void info_dump(struct snd_pcm_info *info)
{
    ALOGV("device = %d\n", info->device);
    ALOGV("subdevice = %d\n", info->subdevice);
    ALOGV("stream = %d\n", info->stream);
    ALOGV("card = %d\n", info->card);
    ALOGV("id = '%s'\n", info->id);
    ALOGV("name = '%s'\n", info->name);
    ALOGV("subname = '%s'\n", info->subname);
    ALOGV("dev_class = %d\n", info->dev_class);
    ALOGV("dev_subclass = %d\n", info->dev_subclass);
    ALOGV("subdevices_count = %d\n", info->subdevices_count);
    ALOGV("subdevices_avail = %d\n", info->subdevices_avail);
}
#else
void param_dump(struct snd_pcm_hw_params *p) {}
static void info_dump(struct snd_pcm_info *info) {}
#endif

int param_set_hw_refine(struct pcm *pcm, struct snd_pcm_hw_params *params)
{
    if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_HW_REFINE, params)) {
        ALOGE("SNDRV_PCM_IOCTL_HW_REFINE failed");
        return -EPERM;
    }
    return 0;
}

int param_set_hw_params(struct pcm *pcm, struct snd_pcm_hw_params *params)
{
    if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_HW_PARAMS, params)) {
        return -EPERM;
    }
    pcm->hw_p = params;
    return 0;
}

int param_set_sw_params(struct pcm *pcm, struct snd_pcm_sw_params *sparams)
{
    if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_SW_PARAMS, sparams)) {
        return -EPERM;
    }
    pcm->sw_p = sparams;
    return 0;
}

int pcm_buffer_size(struct snd_pcm_hw_params *params)
{
    struct snd_interval *i = param_to_interval(params, SNDRV_PCM_HW_PARAM_BUFFER_BYTES);
            ALOGV("%s = (%d,%d) omin=%d omax=%d int=%d empty=%d\n",
                   param_name[SNDRV_PCM_HW_PARAM_BUFFER_BYTES],
                   i->min, i->max, i->openmin,
                   i->openmax, i->integer, i->empty);
    return i->min;
}

int pcm_period_size(struct snd_pcm_hw_params *params)
{
    struct snd_interval *i = param_to_interval(params, SNDRV_PCM_HW_PARAM_PERIOD_BYTES);
            ALOGV("%s = (%d,%d) omin=%d omax=%d int=%d empty=%d\n",
                   param_name[SNDRV_PCM_HW_PARAM_PERIOD_BYTES],
                   i->min, i->max, i->openmin,
                   i->openmax, i->integer, i->empty);
    return i->min;
}

const char* pcm_error(struct pcm *pcm)
{
    return pcm->error;
}

static int oops(struct pcm *pcm, int e, const char *fmt, ...)
{
    va_list ap;
    int sz;

    va_start(ap, fmt);
    vsnprintf(pcm->error, PCM_ERROR_MAX, fmt, ap);
    va_end(ap);
    sz = strnlen(pcm->error, PCM_ERROR_MAX);

    if (errno)
        snprintf(pcm->error + sz, PCM_ERROR_MAX - sz,
                 ": %s", strerror(e));
    return -1;
}

long pcm_avail(struct pcm *pcm)
{
     struct snd_pcm_sync_ptr *sync_ptr = pcm->sync_ptr;
     if (pcm->flags & DEBUG_ON) {
        ALOGV("hw_ptr = %d buf_size = %d appl_ptr = %d\n",
                sync_ptr->s.status.hw_ptr,
                pcm->buffer_size,
                sync_ptr->c.control.appl_ptr);
     }
     if (pcm->flags & PCM_IN) {
          long avail = sync_ptr->s.status.hw_ptr - sync_ptr->c.control.appl_ptr;
        if (avail < 0)
                avail += pcm->sw_p->boundary;
        return avail;
     } else {
         long avail = sync_ptr->s.status.hw_ptr - sync_ptr->c.control.appl_ptr + ((pcm->flags & PCM_MONO) ? pcm->buffer_size/2 : pcm->buffer_size/4);
         if (avail < 0)
              avail += pcm->sw_p->boundary;
         else if ((unsigned long) avail >= pcm->sw_p->boundary)
              avail -= pcm->sw_p->boundary;
         return avail;
     }
}

int sync_ptr(struct pcm *pcm)
{
    int err;
    err = ioctl(pcm->fd, SNDRV_PCM_IOCTL_SYNC_PTR, pcm->sync_ptr);
    if (err < 0) {
        err = errno;
        ALOGE("SNDRV_PCM_IOCTL_SYNC_PTR failed %d \n", err);
        return err;
    }

    return 0;
}

int mmap_buffer(struct pcm *pcm)
{
    int err, i;
    char *ptr;
    unsigned size;
    struct snd_pcm_channel_info ch;
    int channels = (pcm->flags & PCM_MONO) ? 1 : 2;

    size = pcm->buffer_size;
    if (pcm->flags & DEBUG_ON)
        ALOGV("size = %d\n", size);
    pcm->addr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED,
                           pcm->fd, 0);
    if (pcm->addr)
         return 0;
    else
         return -errno;
}

/*
 * Destination offset would be mod of total data written
 * (application pointer) and the buffer size of the driver.
 * Hence destination address would be base address(pcm->addr) +
 * destination offset.
 */
u_int8_t *dst_address(struct pcm *pcm)
{
    unsigned long pcm_offset = 0;
    struct snd_pcm_sync_ptr *sync_ptr = pcm->sync_ptr;
    unsigned int appl_ptr = 0;

    appl_ptr = (pcm->flags & PCM_MONO) ? sync_ptr->c.control.appl_ptr*2 : sync_ptr->c.control.appl_ptr*4;
    pcm_offset = (appl_ptr % (unsigned long)pcm->buffer_size);
    return pcm->addr + pcm_offset;

}

int mmap_transfer(struct pcm *pcm, void *data, unsigned offset,
                  long frames)
{
    struct snd_pcm_sync_ptr *sync_ptr = pcm->sync_ptr;
    unsigned size;
    u_int8_t *dst_addr, *mmaped_addr;
    u_int8_t *src_addr = data;
    int channels = (pcm->flags & PCM_MONO) ? 1 : 2;

    dst_addr = dst_address(pcm);

    frames = frames * channels *2 ;

    while (frames-- > 0) {
        *(u_int8_t*)dst_addr = *(const u_int8_t*)src_addr;
         src_addr++;
         dst_addr++;
    }
    return 0;
}

int mmap_transfer_capture(struct pcm *pcm, void *data, unsigned offset,
                          long frames)
{
    struct snd_pcm_sync_ptr *sync_ptr = pcm->sync_ptr;
    unsigned long pcm_offset = 0;
    unsigned size;
    u_int8_t *dst_addr, *mmaped_addr;
    u_int8_t *src_addr;
    int channels = (pcm->flags & PCM_MONO) ? 1 : 2;
    unsigned int tmp = (pcm->flags & PCM_MONO) ? sync_ptr->c.control.appl_ptr*2 : sync_ptr->c.control.appl_ptr*4;

    pcm_offset = (tmp % (unsigned long)pcm->buffer_size);
    dst_addr = data;
    src_addr = pcm->addr + pcm_offset;
    frames = frames * channels *2 ;

    while (frames-- > 0) {
        *(u_int8_t*)dst_addr = *(const u_int8_t*)src_addr;
         src_addr++;
         dst_addr++;
    }
    return 0;
}

int pcm_prepare(struct pcm *pcm)
{
    if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_PREPARE)) {
           ALOGE("cannot prepare channel: errno =%d\n", -errno);
           return -errno;
    }
    pcm->running = 1;
    return 0;
}

static int pcm_write_mmap(struct pcm *pcm, void *data, unsigned count)
{
    long frames;
    int err;
    int bytes_written;

    frames = (pcm->flags & PCM_MONO) ? (count / 2) : (count / 4);

    pcm->sync_ptr->flags = SNDRV_PCM_SYNC_PTR_APPL | SNDRV_PCM_SYNC_PTR_AVAIL_MIN;
    err = sync_ptr(pcm);
    if (err == EPIPE) {
        ALOGE("Failed in sync_ptr\n");
        /* we failed to make our window -- try to restart */
        pcm->underruns++;
        pcm->running = 0;
        pcm_prepare(pcm);
    }
    pcm->sync_ptr->c.control.appl_ptr += frames;
    pcm->sync_ptr->flags = 0;

    err = sync_ptr(pcm);
    if (err == EPIPE) {
        ALOGE("Failed in sync_ptr 2 \n");
        /* we failed to make our window -- try to restart */
        pcm->underruns++;
        pcm->running = 0;
        pcm_prepare(pcm);
    }
    bytes_written = pcm->sync_ptr->c.control.appl_ptr - pcm->sync_ptr->s.status.hw_ptr;
    if ((bytes_written >= pcm->sw_p->start_threshold) && (!pcm->start)) {
        if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_START)) {
            err = -errno;
            if (errno == EPIPE) {
                ALOGE("Failed in SNDRV_PCM_IOCTL_START\n");
                /* we failed to make our window -- try to restart */
                pcm->underruns++;
                pcm->running = 0;
                pcm_prepare(pcm);
            } else {
                ALOGE("Error no %d \n", errno);
                return -errno;
            }
        } else {
             ALOGD(" start\n");
             pcm->start = 1;
        }
    }
    return 0;
}

static int pcm_write_nmmap(struct pcm *pcm, void *data, unsigned count)
{
    struct snd_xferi x;
    int channels = (pcm->flags & PCM_MONO) ? 1 : ((pcm->flags & PCM_5POINT1)? 6 : 2 );

    if (pcm->flags & PCM_IN)
        return -EINVAL;
    x.buf = data;
    x.frames =  (count / (channels * 2)) ;

    for (;;) {
        if (!pcm->running) {
            if (pcm_prepare(pcm))
                return -errno;
        }
        if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_WRITEI_FRAMES, &x)) {
            if (errno == EPIPE) {
                    /* we failed to make our window -- try to restart */
                ALOGE("Underrun Error\n");
                pcm->underruns++;
                pcm->running = 0;
                continue;
            }
            return -errno;
        }
        if (pcm->flags & DEBUG_ON)
          ALOGV("Sent frame\n");
        return 0;
    }
}

int pcm_write(struct pcm *pcm, void *data, unsigned count)
{
     if (pcm->flags & PCM_MMAP)
         return pcm_write_mmap(pcm, data, count);
     else
         return pcm_write_nmmap(pcm, data, count);
}

int pcm_read(struct pcm *pcm, void *data, unsigned count)
{
    struct snd_xferi x;

    if (!(pcm->flags & PCM_IN))
        return -EINVAL;

    x.buf = data;
    if (pcm->flags & PCM_MONO) {
        x.frames = (count / 2);
    } else if (pcm->flags & PCM_QUAD) {
        x.frames = (count / 8);
    } else if (pcm->flags & PCM_5POINT1) {
        x.frames = (count / 12);
    } else {
        x.frames = (count / 4);
    }

    for (;;) {
        if (!pcm->running) {
            if (pcm_prepare(pcm))
                return -errno;
            if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_START)) {
                ALOGE("Arec:SNDRV_PCM_IOCTL_START failed\n");
                return -errno;
            }
            pcm->running = 1;
        }
        if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_READI_FRAMES, &x)) {
            if (errno == EPIPE) {
                /* we failed to make our window -- try to restart */
                ALOGE("Arec:Overrun Error\n");
                pcm->underruns++;
                pcm->running = 0;
                continue;
            }
            ALOGE("Arec: error%d\n", errno);
            return -errno;
        }
        return 0;
    }
}

static struct pcm bad_pcm = {
    .fd = -1,
};

static int enable_timer(struct pcm *pcm) {

    pcm->timer_fd = open("/dev/snd/timer", O_RDWR | O_NONBLOCK);
    if (pcm->timer_fd < 0) {
       close(pcm->fd);
       ALOGE("cannot open timer device 'timer'");
       return &bad_pcm;
    }
    int arg = 1;
    struct snd_timer_params timer_param;
    struct snd_timer_select sel;
    if (ioctl(pcm->timer_fd, SNDRV_TIMER_IOCTL_TREAD, &arg) < 0) {
           ALOGE("extended read is not supported (SNDRV_TIMER_IOCTL_TREAD)\n");
    }
    memset(&sel, 0, sizeof(sel));
    sel.id.dev_class = SNDRV_TIMER_CLASS_PCM;
    sel.id.dev_sclass = SNDRV_TIMER_SCLASS_NONE;
    sel.id.card = pcm->card_no;
    sel.id.device = pcm->device_no;
    if (pcm->flags & PCM_IN)
        sel.id.subdevice = 1;
    else
        sel.id.subdevice = 0;

    if (pcm->flags & DEBUG_ON) {
        ALOGD("sel.id.dev_class= %d\n", sel.id.dev_class);
        ALOGD("sel.id.dev_sclass = %d\n", sel.id.dev_sclass);
        ALOGD("sel.id.card = %d\n", sel.id.card);
        ALOGD("sel.id.device = %d\n", sel.id.device);
        ALOGD("sel.id.subdevice = %d\n", sel.id.subdevice);
    }
    if (ioctl(pcm->timer_fd, SNDRV_TIMER_IOCTL_SELECT, &sel) < 0) {
          ALOGE("SNDRV_TIMER_IOCTL_SELECT failed.\n");
          close(pcm->timer_fd);
          close(pcm->fd);
          return &bad_pcm;
    }
    memset(&timer_param, 0, sizeof(struct snd_timer_params));
    timer_param.flags |= SNDRV_TIMER_PSFLG_AUTO;
    timer_param.ticks = 1;
    timer_param.filter = (1<<SNDRV_TIMER_EVENT_MSUSPEND) | (1<<SNDRV_TIMER_EVENT_MRESUME) | (1<<SNDRV_TIMER_EVENT_TICK);

    if (ioctl(pcm->timer_fd, SNDRV_TIMER_IOCTL_PARAMS, &timer_param)< 0) {
           ALOGE("SNDRV_TIMER_IOCTL_PARAMS failed\n");
    }
    if (ioctl(pcm->timer_fd, SNDRV_TIMER_IOCTL_START) < 0) {
           close(pcm->timer_fd);
           ALOGE("SNDRV_TIMER_IOCTL_START failed\n");
    }
    return 0;
}

static int disable_timer(struct pcm *pcm) {
     if (pcm == &bad_pcm)
         return 0;
     if (ioctl(pcm->timer_fd, SNDRV_TIMER_IOCTL_STOP) < 0)
         ALOGE("SNDRV_TIMER_IOCTL_STOP failed\n");
     return close(pcm->timer_fd);
}

int pcm_close(struct pcm *pcm)
{
    if (pcm == &bad_pcm)
        return 0;

    if (pcm->flags & PCM_MMAP) {
        disable_timer(pcm);
        if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_DROP) < 0) {
            ALOGE("Reset failed");
        }

        if (munmap(pcm->addr, pcm->buffer_size))
            ALOGE("munmap failed");

        if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_HW_FREE) < 0) {
            ALOGE("HW_FREE failed");
        }
    }

    if (pcm->fd >= 0)
        close(pcm->fd);
    pcm->running = 0;
    pcm->buffer_size = 0;
    pcm->fd = -1;
    if (pcm->sw_p)
        free(pcm->sw_p);
    if (pcm->hw_p)
        free(pcm->hw_p);
    if (pcm->sync_ptr)
        free(pcm->sync_ptr);
    free(pcm);
    return 0;
}

struct pcm *pcm_open(unsigned flags, char *device)
{
    char dname[19];
    struct pcm *pcm;
    struct snd_pcm_info info;
    struct snd_pcm_hw_params params;
    struct snd_pcm_sw_params sparams;
    unsigned period_sz;
    unsigned period_cnt;
    char *tmp;

    if (flags & DEBUG_ON) {
        ALOGV("pcm_open(0x%08x)",flags);
        ALOGV("device %s\n",device);
    }

    pcm = calloc(1, sizeof(struct pcm));
    if (!pcm)
        return &bad_pcm;

    tmp = device+4;
    if ((strncmp(device, "hw:",3) != 0) || (strncmp(tmp, ",",1) != 0)){
        ALOGE("Wrong device fromat\n");
        free(pcm);
        return -EINVAL;
    }

    if (flags & PCM_IN) {
        strlcpy(dname, "/dev/snd/pcmC", sizeof(dname));
        tmp = device+3;
        strlcat(dname, tmp, (2+strlen(dname))) ;
        pcm->card_no = atoi(tmp);
        strlcat(dname, "D", (sizeof("D")+strlen(dname)));
        tmp = device+5;
        pcm->device_no = atoi(tmp);
	/* should be safe to assume pcm dev ID never exceed 99 */
        if (pcm->device_no > 9)
            strlcat(dname, tmp, (3+strlen(dname)));
        else
            strlcat(dname, tmp, (2+strlen(dname)));
        strlcat(dname, "c", (sizeof("c")+strlen(dname)));
    } else {
        strlcpy(dname, "/dev/snd/pcmC", sizeof(dname));
        tmp = device+3;
        strlcat(dname, tmp, (2+strlen(dname))) ;
        pcm->card_no = atoi(tmp);
        strlcat(dname, "D", (sizeof("D")+strlen(dname)));
        tmp = device+5;
        pcm->device_no = atoi(tmp);
	/* should be safe to assume pcm dev ID never exceed 99 */
        if (pcm->device_no > 9)
            strlcat(dname, tmp, (3+strlen(dname)));
        else
            strlcat(dname, tmp, (2+strlen(dname)));
        strlcat(dname, "p", (sizeof("p")+strlen(dname)));
    }
    if (pcm->flags & DEBUG_ON)
        ALOGV("Device name %s\n", dname);

    pcm->sync_ptr = calloc(1, sizeof(struct snd_pcm_sync_ptr));
    if (!pcm->sync_ptr) {
         free(pcm);
         return &bad_pcm;
    }
    pcm->flags = flags;

    pcm->fd = open(dname, O_RDWR|O_NONBLOCK);
    if (pcm->fd < 0) {
        free(pcm->sync_ptr);
        free(pcm);
        ALOGE("cannot open device '%s', errno %d", dname, errno);
        return &bad_pcm;
    }

    if (fcntl(pcm->fd, F_SETFL, fcntl(pcm->fd, F_GETFL) &
            ~O_NONBLOCK) < 0) {
        close(pcm->fd);
        free(pcm->sync_ptr);
        free(pcm);
        ALOGE("failed to change the flag, errno %d", errno);
        return &bad_pcm;
    }

    if (pcm->flags & PCM_MMAP)
        enable_timer(pcm);

    if (pcm->flags & DEBUG_ON)
        ALOGV("pcm_open() %s\n", dname);
    if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_INFO, &info)) {
        ALOGE("cannot get info - %s", dname);
    }
    if (pcm->flags & DEBUG_ON)
       info_dump(&info);

    return pcm;
}

int pcm_ready(struct pcm *pcm)
{
    return pcm->fd >= 0;
}
