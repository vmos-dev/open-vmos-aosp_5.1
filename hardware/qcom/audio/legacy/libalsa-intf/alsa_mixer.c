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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>

#include <linux/ioctl.h>
#define __force
#define __bitwise
#define __user
#include <sound/asound.h>
#include <sound/tlv.h>

#include "alsa_audio.h"

#define LOG_TAG "alsa_mixer"
#define LOG_NDEBUG 1

#ifdef ANDROID
/* definitions for Android logging */
#include <utils/Log.h>
#else /* ANDROID */
#include <math.h>
#define ALOGI(...)      fprintf(stdout, __VA_ARGS__)
#define ALOGE(...)      fprintf(stderr, __VA_ARGS__)
#define ALOGV(...)      fprintf(stderr, __VA_ARGS__)
#endif /* ANDROID */

#define check_range(val, min, max) \
        (((val < min) ? (min) : (val > max) ? (max) : (val)))

/* .5 for rounding before casting to non-decmal value */
/* Should not be used if you need decmal values */
/* or are expecting negitive indexes */
#define percent_to_index(val, min, max) \
        ((val) * ((max) - (min)) * 0.01 + (min) + .5)

#define DEFAULT_TLV_SIZE 4096
#define SPDIF_CHANNEL_STATUS_SIZE 24

enum ctl_type {
	CTL_GLOBAL_VOLUME,
	CTL_PLAYBACK_VOLUME,
	CTL_CAPTURE_VOLUME,
};

static const struct suf {
        const char *suffix;
        snd_ctl_elem_iface_t type;
} suffixes[] = {
        {" Playback Volume", CTL_PLAYBACK_VOLUME},
        {" Capture Volume", CTL_CAPTURE_VOLUME},
        {" Volume", CTL_GLOBAL_VOLUME},
        {NULL, 0}
};

static int is_volume(const char *name, enum ctl_type *type)
{
        const struct suf *p;
        size_t nlen = strlen(name);
        p = suffixes;
        while (p->suffix) {
                size_t slen = strnlen(p->suffix, 44);
                size_t l;
                if (nlen > slen) {
                        l = nlen - slen;
                        if (strncmp(name + l, p->suffix, slen) == 0 &&
                            (l < 1 || name[l-1] != '-')) {      /* 3D Control - Switch */
                                *type = p->type;
                                return l;
                        }
                }
                p++;
        }
	return 0;
}

static const char *elem_iface_name(snd_ctl_elem_iface_t n)
{
    switch (n) {
    case SNDRV_CTL_ELEM_IFACE_CARD: return "CARD";
    case SNDRV_CTL_ELEM_IFACE_HWDEP: return "HWDEP";
    case SNDRV_CTL_ELEM_IFACE_MIXER: return "MIXER";
    case SNDRV_CTL_ELEM_IFACE_PCM: return "PCM";
    case SNDRV_CTL_ELEM_IFACE_RAWMIDI: return "MIDI";
    case SNDRV_CTL_ELEM_IFACE_TIMER: return "TIMER";
    case SNDRV_CTL_ELEM_IFACE_SEQUENCER: return "SEQ";
    default: return "???";
    }
}

static const char *elem_type_name(snd_ctl_elem_type_t n)
{
    switch (n) {
    case SNDRV_CTL_ELEM_TYPE_NONE: return "NONE";
    case SNDRV_CTL_ELEM_TYPE_BOOLEAN: return "BOOL";
    case SNDRV_CTL_ELEM_TYPE_INTEGER: return "INT32";
    case SNDRV_CTL_ELEM_TYPE_ENUMERATED: return "ENUM";
    case SNDRV_CTL_ELEM_TYPE_BYTES: return "BYTES";
    case SNDRV_CTL_ELEM_TYPE_IEC958: return "IEC958";
    case SNDRV_CTL_ELEM_TYPE_INTEGER64: return "INT64";
    default: return "???";
    }
}

void mixer_close(struct mixer *mixer)
{
    unsigned n,m;

    if (mixer->fd >= 0)
        close(mixer->fd);

    if (mixer->ctl) {
        for (n = 0; n < mixer->count; n++) {
            if (mixer->ctl[n].ename) {
                unsigned max = mixer->ctl[n].info->value.enumerated.items;
                for (m = 0; m < max; m++)
                    free(mixer->ctl[n].ename[m]);
                free(mixer->ctl[n].ename);
            }
        }
        free(mixer->ctl);
    }

    if (mixer->info)
        free(mixer->info);

    free(mixer);
}

struct mixer *mixer_open(const char *device)
{
    struct snd_ctl_elem_list elist;
    struct snd_ctl_elem_info tmp;
    struct snd_ctl_elem_id *eid = NULL;
    struct mixer *mixer = NULL;
    unsigned n, m;
    int fd;

    fd = open(device, O_RDWR);
    if (fd < 0) {
        ALOGE("Control open failed\n");
        return 0;
    }

    memset(&elist, 0, sizeof(elist));
    if (ioctl(fd, SNDRV_CTL_IOCTL_ELEM_LIST, &elist) < 0) {
        ALOGE("SNDRV_CTL_IOCTL_ELEM_LIST failed\n");
        goto fail;
    }

    mixer = calloc(1, sizeof(*mixer));
    if (!mixer)
        goto fail;

    mixer->ctl = calloc(elist.count, sizeof(struct mixer_ctl));
    mixer->info = calloc(elist.count, sizeof(struct snd_ctl_elem_info));
    if (!mixer->ctl || !mixer->info)
        goto fail;

    eid = calloc(elist.count, sizeof(struct snd_ctl_elem_id));
    if (!eid)
        goto fail;

    mixer->count = elist.count;
    mixer->fd = fd;
    elist.space = mixer->count;
    elist.pids = eid;
    if (ioctl(fd, SNDRV_CTL_IOCTL_ELEM_LIST, &elist) < 0)
        goto fail;

    for (n = 0; n < mixer->count; n++) {
        struct snd_ctl_elem_info *ei = mixer->info + n;
        ei->id.numid = eid[n].numid;
        if (ioctl(fd, SNDRV_CTL_IOCTL_ELEM_INFO, ei) < 0)
            goto fail;
        mixer->ctl[n].info = ei;
        mixer->ctl[n].mixer = mixer;
        if (ei->type == SNDRV_CTL_ELEM_TYPE_ENUMERATED) {
            char **enames = calloc(ei->value.enumerated.items, sizeof(char*));
            if (!enames)
                goto fail;
            mixer->ctl[n].ename = enames;
            for (m = 0; m < ei->value.enumerated.items; m++) {
                memset(&tmp, 0, sizeof(tmp));
                tmp.id.numid = ei->id.numid;
                tmp.value.enumerated.item = m;
                if (ioctl(fd, SNDRV_CTL_IOCTL_ELEM_INFO, &tmp) < 0)
                    goto fail;
                enames[m] = strdup(tmp.value.enumerated.name);
                if (!enames[m])
                    goto fail;
            }
        }
    }

    free(eid);
    return mixer;

fail:
    if (eid)
        free(eid);
    if (mixer)
        mixer_close(mixer);
    else if (fd >= 0)
        close(fd);
    return 0;
}

void mixer_dump(struct mixer *mixer)
{
    unsigned n, m;

    ALOGV("  id iface dev sub idx num perms     type   isvolume  name\n");
    for (n = 0; n < mixer->count; n++) {
	enum ctl_type type;
        struct snd_ctl_elem_info *ei = mixer->info + n;

        ALOGV("%4d %5s %3d %3d %3d %3d %c%c%c%c%c%c%c%c%c %-6s %8d  %s",
               ei->id.numid, elem_iface_name(ei->id.iface),
               ei->id.device, ei->id.subdevice, ei->id.index,
               ei->count,
               (ei->access & SNDRV_CTL_ELEM_ACCESS_READ) ? 'r' : ' ',
               (ei->access & SNDRV_CTL_ELEM_ACCESS_WRITE) ? 'w' : ' ',
               (ei->access & SNDRV_CTL_ELEM_ACCESS_VOLATILE) ? 'V' : ' ',
               (ei->access & SNDRV_CTL_ELEM_ACCESS_TIMESTAMP) ? 'T' : ' ',
               (ei->access & SNDRV_CTL_ELEM_ACCESS_TLV_READ) ? 'R' : ' ',
               (ei->access & SNDRV_CTL_ELEM_ACCESS_TLV_WRITE) ? 'W' : ' ',
               (ei->access & SNDRV_CTL_ELEM_ACCESS_TLV_COMMAND) ? 'C' : ' ',
               (ei->access & SNDRV_CTL_ELEM_ACCESS_INACTIVE) ? 'I' : ' ',
               (ei->access & SNDRV_CTL_ELEM_ACCESS_LOCK) ? 'L' : ' ',
               elem_type_name(ei->type),
	       (is_volume(ei->id.name, &type)) ? 1 : 0,
               ei->id.name);
        switch (ei->type) {
        case SNDRV_CTL_ELEM_TYPE_INTEGER:
            ALOGV(ei->value.integer.step ?
                   " { %ld-%ld, %ld }\n" : " { %ld-%ld }",
                   ei->value.integer.min,
                   ei->value.integer.max,
                   ei->value.integer.step);
            break;
        case SNDRV_CTL_ELEM_TYPE_INTEGER64:
            ALOGV(ei->value.integer64.step ?
                   " { %lld-%lld, %lld }\n" : " { %lld-%lld }",
                   ei->value.integer64.min,
                   ei->value.integer64.max,
                   ei->value.integer64.step);
            break;
        case SNDRV_CTL_ELEM_TYPE_ENUMERATED: {
            unsigned m;
            ALOGV(" { %s=0", mixer->ctl[n].ename[0]);
            for (m = 1; m < ei->value.enumerated.items; m++)
                ALOGV(", %s=%d", mixer->ctl[n].ename[m],m);
            ALOGV(" }");
            break;
        }
        }
        ALOGV("\n");
    }
}

struct mixer_ctl *mixer_get_control(struct mixer *mixer,
                                    const char *name, unsigned index)
{
    unsigned n;
    for (n = 0; n < mixer->count; n++) {
        if (mixer->info[n].id.index == index) {
            if (!strncmp(name, (char*) mixer->info[n].id.name,
			sizeof(mixer->info[n].id.name))) {
                return mixer->ctl + n;
            }
        }
    }
    return 0;
}

struct mixer_ctl *mixer_get_nth_control(struct mixer *mixer, unsigned n)
{
    if (n < mixer->count)
        return mixer->ctl + n;
    return 0;
}

static void print_dB(long dB)
{
        ALOGV("%li.%02lidB", dB / 100, (dB < 0 ? -dB : dB) % 100);
}

int mixer_ctl_read_tlv(struct mixer_ctl *ctl,
                    unsigned int *tlv,
		    long *min, long *max, unsigned int *tlv_type)
{
    unsigned int tlv_size = DEFAULT_TLV_SIZE;
    unsigned int type;
    unsigned int size;

    if(!!(ctl->info->access & SNDRV_CTL_ELEM_ACCESS_TLV_READ)) {
        struct snd_ctl_tlv *xtlv;
        tlv[0] = -1;
        tlv[1] = 0;
        xtlv = calloc(1, sizeof(struct snd_ctl_tlv) + tlv_size);
        if (xtlv == NULL)
                return -ENOMEM;
        xtlv->numid = ctl->info->id.numid;
        xtlv->length = tlv_size;
        memcpy(xtlv->tlv, tlv, tlv_size);
        if (ioctl(ctl->mixer->fd, SNDRV_CTL_IOCTL_TLV_READ, xtlv) < 0) {
            fprintf( stderr, "SNDRV_CTL_IOCTL_TLV_READ failed\n");
            free(xtlv);
            return -errno;
        }
        if (xtlv->tlv[1] + 2 * sizeof(unsigned int) > tlv_size) {
            free(xtlv);
            return -EFAULT;
        }
        memcpy(tlv, xtlv->tlv, xtlv->tlv[1] + 2 * sizeof(unsigned int));
        free(xtlv);

        type = tlv[0];
	*tlv_type = type;
        size = tlv[1];
        switch (type) {
        case SNDRV_CTL_TLVT_DB_SCALE: {
                int idx = 2;
                int step;
                ALOGV("dBscale-");
                if (size != 2 * sizeof(unsigned int)) {
                        while (size > 0) {
                                ALOGV("0x%08x,", tlv[idx++]);
                                size -= sizeof(unsigned int);
                        }
                } else {
                    ALOGV(" min=");
                    print_dB((int)tlv[2]);
                    *min = (long)tlv[2];
                    ALOGV(" step=");
                    step = (tlv[3] & 0xffff);
                    print_dB(tlv[3] & 0xffff);
                    ALOGV(" max=");
                    *max = (ctl->info->value.integer.max);
                    print_dB((long)ctl->info->value.integer.max);
                    ALOGV(" mute=%i\n", (tlv[3] >> 16) & 1);
                }
            break;
        }
        case SNDRV_CTL_TLVT_DB_LINEAR: {
                int idx = 2;
                ALOGV("dBLiner-");
                if (size != 2 * sizeof(unsigned int)) {
                        while (size > 0) {
                                ALOGV("0x%08x,", tlv[idx++]);
                                size -= sizeof(unsigned int);
                        }
                } else {
                    ALOGV(" min=");
                    *min = tlv[2];
                    print_dB(tlv[2]);
                    ALOGV(" max=");
                    *max = tlv[3];
                    print_dB(tlv[3]);
                }
            break;
        }
        default:
             break;
        }
        return 0;
    }
    return -EINVAL;
}

void mixer_ctl_get(struct mixer_ctl *ctl, unsigned *value)
{
    struct snd_ctl_elem_value ev;
    unsigned int n;
    unsigned int *tlv = NULL;
    enum ctl_type type;
    unsigned int *tlv_type;
    long min, max;

    if (is_volume(ctl->info->id.name, &type)) {
       ALOGV("capability: volume\n");
       tlv = calloc(1, DEFAULT_TLV_SIZE);
       if (tlv == NULL) {
           ALOGE("failed to allocate memory\n");
       } else {
	   mixer_ctl_read_tlv(ctl, tlv, &min, &max, &tlv_type);
           free(tlv);
       }
    }

    memset(&ev, 0, sizeof(ev));
    ev.id.numid = ctl->info->id.numid;
    if (ioctl(ctl->mixer->fd, SNDRV_CTL_IOCTL_ELEM_READ, &ev))
        return;
    ALOGV("%s:", ctl->info->id.name);

    switch (ctl->info->type) {
    case SNDRV_CTL_ELEM_TYPE_BOOLEAN:
        for (n = 0; n < ctl->info->count; n++)
            ALOGV(" %s", ev.value.integer.value[n] ? "on" : "off");
        *value = ev.value.integer.value[0];
        break;
    case SNDRV_CTL_ELEM_TYPE_INTEGER: {
        for (n = 0; n < ctl->info->count; n++)
            ALOGV(" %ld", ev.value.integer.value[n]);
        *value = ev.value.integer.value[0];
        break;
    }
    case SNDRV_CTL_ELEM_TYPE_INTEGER64:
        for (n = 0; n < ctl->info->count; n++)
            ALOGV(" %lld", ev.value.integer64.value[n]);
        *value = ev.value.integer64.value[0];
        break;
    case SNDRV_CTL_ELEM_TYPE_ENUMERATED:
        for (n = 0; n < ctl->info->count; n++) {
            unsigned v = ev.value.enumerated.item[n];
            ALOGV(" %d (%s)", v,
                   (v < ctl->info->value.enumerated.items) ? ctl->ename[v] : "???");
        *value = ev.value.enumerated.item[0];
        }
        break;
    default:
        ALOGV(" ???");
    }
    ALOGV("\n");
}

static long scale_int(struct snd_ctl_elem_info *ei, unsigned _percent)
{
    long percent;

    if (_percent > 100)
        percent = 100;
    else
        percent = (long) _percent;

    return (long)percent_to_index(percent, ei->value.integer.min, ei->value.integer.max);
}

static long long scale_int64(struct snd_ctl_elem_info *ei, unsigned _percent)
{
    long long percent;

    if (_percent > 100)
        percent = 100;
    else
        percent = (long) _percent;

    return (long long)percent_to_index(percent, ei->value.integer.min, ei->value.integer.max);
}

/*
 * Add support for controls taking more than one parameter as input value
 * This is useful for volume controls which take two parameters as input value.
 */
int mixer_ctl_mulvalues(struct mixer_ctl *ctl, int count, char ** argv)
{
    struct snd_ctl_elem_value ev;
    unsigned n;

    if (!ctl) {
        ALOGV("can't find control\n");
        return -1;
    }
    if (count < ctl->info->count || count > ctl->info->count)
        return -EINVAL;


    memset(&ev, 0, sizeof(ev));
    ev.id.numid = ctl->info->id.numid;
    switch (ctl->info->type) {
    case SNDRV_CTL_ELEM_TYPE_BOOLEAN:
        for (n = 0; n < ctl->info->count; n++)
            ev.value.integer.value[n] = !!atoi(argv[n]);
        break;
    case SNDRV_CTL_ELEM_TYPE_INTEGER: {
        for (n = 0; n < ctl->info->count; n++) {
             fprintf( stderr, "Value: %d idx:%d\n", atoi(argv[n]), n);
             ev.value.integer.value[n] = atoi(argv[n]);
        }
        break;
    }
    case SNDRV_CTL_ELEM_TYPE_INTEGER64: {
        for (n = 0; n < ctl->info->count; n++) {
             long long value_ll = scale_int64(ctl->info, atoi(argv[n]));
             fprintf( stderr, "ll_value = %lld\n", value_ll);
             ev.value.integer64.value[n] = value_ll;
        }
        break;
    }
    default:
        errno = EINVAL;
        return errno;
    }

    return ioctl(ctl->mixer->fd, SNDRV_CTL_IOCTL_ELEM_WRITE, &ev);
}

int mixer_ctl_set(struct mixer_ctl *ctl, unsigned percent)
{
    struct snd_ctl_elem_value ev;
    unsigned n;
    long min, max;
    unsigned int *tlv = NULL;
    enum ctl_type type;
    int volume = 0;
    unsigned int tlv_type;

    if (!ctl) {
        ALOGV("can't find control\n");
        return -1;
    }

    if (is_volume(ctl->info->id.name, &type)) {
        ALOGV("capability: volume\n");
        tlv = calloc(1, DEFAULT_TLV_SIZE);
        if (tlv == NULL) {
            ALOGE("failed to allocate memory\n");
        } else if (!mixer_ctl_read_tlv(ctl, tlv, &min, &max, &tlv_type)) {
            switch(tlv_type) {
            case SNDRV_CTL_TLVT_DB_LINEAR:
                ALOGV("tlv db linear: b4 %d\n", percent);

		if (min < 0) {
			max = max - min;
			min = 0;
		}
                percent = check_range(percent, min, max);
                ALOGV("tlv db linear: %d %d %d\n", percent, min, max);
                volume = 1;
                break;
            default:
                percent = (long)percent_to_index(percent, min, max);
                percent = check_range(percent, min, max);
                volume = 1;
                break;
            }
        } else
            ALOGV("mixer_ctl_read_tlv failed\n");
        free(tlv);
    }
    memset(&ev, 0, sizeof(ev));
    ev.id.numid = ctl->info->id.numid;
    switch (ctl->info->type) {
    case SNDRV_CTL_ELEM_TYPE_BOOLEAN:
        for (n = 0; n < ctl->info->count; n++)
            ev.value.integer.value[n] = !!percent;
        break;
    case SNDRV_CTL_ELEM_TYPE_INTEGER: {
        int value;
        if (!volume)
             value = scale_int(ctl->info, percent);
        else
             value = (int) percent;
        for (n = 0; n < ctl->info->count; n++)
            ev.value.integer.value[n] = value;
        break;
    }
    case SNDRV_CTL_ELEM_TYPE_INTEGER64: {
        long long value;
        if (!volume)
             value = scale_int64(ctl->info, percent);
        else
             value = (long long)percent;
        for (n = 0; n < ctl->info->count; n++)
            ev.value.integer64.value[n] = value;
        break;
    }
    case SNDRV_CTL_ELEM_TYPE_IEC958: {
        struct snd_aes_iec958 *iec958;
        iec958 = (struct snd_aes_iec958 *)percent;
        memcpy(ev.value.iec958.status,iec958->status,SPDIF_CHANNEL_STATUS_SIZE);
        break;
    }
    default:
        errno = EINVAL;
        return errno;
    }

    return ioctl(ctl->mixer->fd, SNDRV_CTL_IOCTL_ELEM_WRITE, &ev);
}

/* the api parses the mixer control input to extract
 * the value of volume in any one of the following format
 * <volume><%>
 * <volume><dB>
 * All remaining formats are currently ignored.
 */

static int set_volume_simple(struct mixer_ctl *ctl,
    char **ptr, long pmin, long pmax, int count)
{
    long val, orig;
    char *p = *ptr, *s;
    struct snd_ctl_elem_value ev;
    unsigned n;

    if (*p == ':')
        p++;
    if (*p == '\0' || (!isdigit(*p) && *p != '-'))
        goto skip;

    s = p;
    val = strtol(s, &p, 10);
    if (*p == '.') {
        p++;
        strtol(p, &p, 10);
    }
    if (*p == '%') {
        val = (long)percent_to_index(strtod(s, NULL), pmin, pmax);
        p++;
    } else if (p[0] == 'd' && p[1] == 'B') {
        val = (long)(strtod(s, NULL) * 100.0);
        p += 2;
    } else {
        if (pmin < 0) {
            pmax = pmax - pmin;
            pmin = 0;
        }
    }
    val = check_range(val, pmin, pmax);
    ALOGV("val = %x", val);

    if (!ctl) {
        ALOGV("can't find control\n");
        return -EPERM;
    }
    if (count < ctl->info->count || count > ctl->info->count)
        return -EINVAL;

    ALOGV("Value = ");

    memset(&ev, 0, sizeof(ev));
    ev.id.numid = ctl->info->id.numid;
    switch (ctl->info->type) {
    case SNDRV_CTL_ELEM_TYPE_BOOLEAN:
        for (n = 0; n < ctl->info->count; n++)
            ev.value.integer.value[n] = !!val;
        print_dB(val);
        break;
    case SNDRV_CTL_ELEM_TYPE_INTEGER: {
        for (n = 0; n < ctl->info->count; n++)
             ev.value.integer.value[n] = val;
        print_dB(val);
        break;
    }
    case SNDRV_CTL_ELEM_TYPE_INTEGER64: {
        for (n = 0; n < ctl->info->count; n++) {
             long long value_ll = scale_int64(ctl->info, val);
             print_dB(value_ll);
             ev.value.integer64.value[n] = value_ll;
        }
        break;
    }
    default:
        errno = EINVAL;
        return errno;
    }

    ALOGV("\n");
    return ioctl(ctl->mixer->fd, SNDRV_CTL_IOCTL_ELEM_WRITE, &ev);

skip:
        if (*p == ',')
                p++;
        *ptr = p;
        return 0;
}

int mixer_ctl_set_value(struct mixer_ctl *ctl, int count, char ** argv)
{
    unsigned int size;
    unsigned int *tlv = NULL;
    long min, max;
    enum ctl_type type;
    unsigned int tlv_type;

    if (is_volume(ctl->info->id.name, &type)) {
        ALOGV("capability: volume\n");
        tlv = calloc(1, DEFAULT_TLV_SIZE);
        if (tlv == NULL) {
            ALOGE("failed to allocate memory\n");
        } else if (!mixer_ctl_read_tlv(ctl, tlv, &min, &max, &tlv_type)) {
            ALOGV("min = %x max = %x", min, max);
            if (set_volume_simple(ctl, argv, min, max, count))
                mixer_ctl_mulvalues(ctl, count, argv);
        } else
            ALOGV("mixer_ctl_read_tlv failed\n");
        free(tlv);
    } else {
        mixer_ctl_mulvalues(ctl, count, argv);
    }
    return 0;
}


int mixer_ctl_select(struct mixer_ctl *ctl, const char *value)
{
    unsigned n, max;
    struct snd_ctl_elem_value ev;
    unsigned int  input_str_len, str_len;

    if (ctl->info->type != SNDRV_CTL_ELEM_TYPE_ENUMERATED) {
        errno = EINVAL;
        return -1;
    }

    input_str_len =  strnlen(value,64);

    max = ctl->info->value.enumerated.items;
    for (n = 0; n < max; n++) {

        str_len = strnlen(ctl->ename[n], 64);
        if (str_len < input_str_len)
            str_len = input_str_len;

        if (!strncmp(value, ctl->ename[n], str_len)) {
            memset(&ev, 0, sizeof(ev));
            ev.value.enumerated.item[0] = n;
            ev.id.numid = ctl->info->id.numid;
            if (ioctl(ctl->mixer->fd, SNDRV_CTL_IOCTL_ELEM_WRITE, &ev) < 0)
                return -1;
            return 0;
        }
    }

    errno = EINVAL;
    return errno;
}
