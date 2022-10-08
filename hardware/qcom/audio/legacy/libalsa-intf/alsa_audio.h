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

#ifndef _AUDIO_H_
#define _AUDIO_H_

#include <sound/asound.h>
#define PCM_ERROR_MAX 128

struct pcm {
    int fd;
    int timer_fd;
    unsigned rate;
    unsigned channels;
    unsigned flags;
    unsigned format;
    unsigned running:1;
    int underruns;
    unsigned buffer_size;
    unsigned period_size;
    unsigned period_cnt;
    char error[PCM_ERROR_MAX];
    struct snd_pcm_hw_params *hw_p;
    struct snd_pcm_sw_params *sw_p;
    struct snd_pcm_sync_ptr *sync_ptr;
    struct snd_pcm_channel_info ch[2];
    void *addr;
    int card_no;
    int device_no;
    int start;
};

enum decoder_alias {
    FORMAT_MP3,
    FORMAT_AC3_PASS_THROUGH = 2,
};

#define FORMAT(v) SNDRV_PCM_FORMAT_##v

#define PCM_OUT        0x00000000
#define PCM_IN         0x10000000

#define PCM_STEREO     0x00000000
#define PCM_MONO       0x01000000
#define PCM_QUAD       0x02000000
#define PCM_5POINT1    0x04000000

#define PCM_44100HZ    0x00000000
#define PCM_48000HZ    0x00100000
#define PCM_8000HZ     0x00200000
#define PCM_RATE_MASK  0x00F00000

#define PCM_MMAP       0x00010000
#define PCM_NMMAP      0x00000000

#define DEBUG_ON       0x00000001
#define DEBUG_OFF      0x00000000

#define PCM_PERIOD_CNT_MIN 2
#define PCM_PERIOD_CNT_SHIFT 16
#define PCM_PERIOD_CNT_MASK (0xF << PCM_PERIOD_CNT_SHIFT)
#define PCM_PERIOD_SZ_MIN 128
#define PCM_PERIOD_SZ_SHIFT 12
#define PCM_PERIOD_SZ_MASK (0xF << PCM_PERIOD_SZ_SHIFT)

#define TIMEOUT_INFINITE  -1

/* Acquire/release a pcm channel.
 * Returns non-zero on error
 */

struct mixer_ctl {
    struct mixer *mixer;
    struct snd_ctl_elem_info *info;
    char **ename;
};

#define __snd_alloca(ptr,type) do { *ptr = (type *) alloca(sizeof(type)); memset(*ptr, 0, sizeof(type)); } while (0)
#define snd_ctl_elem_id_alloca(ptr) __snd_alloca(ptr, snd_ctl_elem_id)
#define snd_ctl_card_info_alloca(ptr) __snd_alloca(ptr, snd_ctl_card_info)
#define snd_ctl_event_alloca(ptr) __snd_alloca(ptr, snd_ctl_event)
#define snd_ctl_elem_list_alloca(ptr) __snd_alloca(ptr, snd_ctl_elem_list)
#define snd_ctl_elem_info_alloca(ptr) __snd_alloca(ptr, snd_ctl_elem_info)
#define snd_ctl_elem_value_alloca(ptr) __snd_alloca(ptr, snd_ctl_elem_value)


enum snd_pcm_stream_t {
        /** Playback stream */
        SND_PCM_STREAM_PLAYBACK = 0,
        /** Capture stream */
        SND_PCM_STREAM_CAPTURE,
        SND_PCM_STREAM_LAST = SND_PCM_STREAM_CAPTURE
};

enum _snd_ctl_elem_iface {
        /** Card level */
        SND_CTL_ELEM_IFACE_CARD = 0,
        /** Hardware dependent device */
        SND_CTL_ELEM_IFACE_HWDEP,
        /** Mixer */
        SND_CTL_ELEM_IFACE_MIXER,
        /** PCM */
        SND_CTL_ELEM_IFACE_PCM,
        /** RawMidi */
        SND_CTL_ELEM_IFACE_RAWMIDI,
        /** Timer */
        SND_CTL_ELEM_IFACE_TIMER,
        /** Sequencer */
        SND_CTL_ELEM_IFACE_SEQUENCER,
        SND_CTL_ELEM_IFACE_LAST = SND_CTL_ELEM_IFACE_SEQUENCER
};

struct mixer {
    int fd;
    struct snd_ctl_elem_info *info;
    struct mixer_ctl *ctl;
    unsigned count;
};

int get_format(const char* name);
const char *get_format_name(int format);
const char *get_format_desc(int format);
struct pcm *pcm_open(unsigned flags, char *device);
int pcm_close(struct pcm *pcm);
int pcm_ready(struct pcm *pcm);
int mmap_buffer(struct pcm *pcm);
u_int8_t *dst_address(struct pcm *pcm);
int sync_ptr(struct pcm *pcm);

void param_init(struct snd_pcm_hw_params *p);
void param_set_mask(struct snd_pcm_hw_params *p, int n, unsigned bit);
void param_set_min(struct snd_pcm_hw_params *p, int n, unsigned val);
void param_set_int(struct snd_pcm_hw_params *p, int n, unsigned val);
void param_set_max(struct snd_pcm_hw_params *p, int n, unsigned val);
int param_set_hw_refine(struct pcm *pcm, struct snd_pcm_hw_params *params);
int param_set_hw_params(struct pcm *pcm, struct snd_pcm_hw_params *params);
int param_set_sw_params(struct pcm *pcm, struct snd_pcm_sw_params *sparams);
void param_dump(struct snd_pcm_hw_params *p);
int pcm_prepare(struct pcm *pcm);
long pcm_avail(struct pcm *pcm);

/* Returns a human readable reason for the last error. */
const char *pcm_error(struct pcm *pcm);

/* Returns the buffer size (int bytes) that should be used for pcm_write.
 * This will be 1/2 of the actual fifo size.
 */
int pcm_buffer_size(struct snd_pcm_hw_params *params);
int pcm_period_size(struct snd_pcm_hw_params *params);

/* Write data to the fifo.
 * Will start playback on the first write or on a write that
 * occurs after a fifo underrun.
 */
int pcm_write(struct pcm *pcm, void *data, unsigned count);
int pcm_read(struct pcm *pcm, void *data, unsigned count);

struct mixer;
struct mixer_ctl;

struct mixer *mixer_open(const char *device);
void mixer_close(struct mixer *mixer);
void mixer_dump(struct mixer *mixer);

struct mixer_ctl *mixer_get_control(struct mixer *mixer,
                                    const char *name, unsigned index);
struct mixer_ctl *mixer_get_nth_control(struct mixer *mixer, unsigned n);

int mixer_ctl_set(struct mixer_ctl *ctl, unsigned percent);
int mixer_ctl_select(struct mixer_ctl *ctl, const char *value);
void mixer_ctl_get(struct mixer_ctl *ctl, unsigned *value);
int mixer_ctl_set_value(struct mixer_ctl *ctl, int count, char ** argv);


#define MAX_NUM_CODECS 32

/* compressed audio support */
#ifdef QCOM_COMPRESSED_AUDIO_ENABLED
struct snd_compr_caps {
        __u32 num_codecs;
        __u32 min_fragment_size;
        __u32 max_fragment_size;
        __u32 min_fragments;
        __u32 max_fragments;
        __u32 codecs[MAX_NUM_CODECS];
        __u32 reserved[11];
};

struct snd_enc_wma {
        __u32 super_block_align; /* WMA Type-specific data */
	__u32 bits_per_sample;
	__u32 channelmask;
	__u32 encodeopt;
};

struct snd_enc_vorbis {
        int quality;
        __u32 managed;
        __u32 max_bit_rate;
        __u32 min_bit_rate;
        __u32 downmix;
};

struct snd_enc_real {
        __u32 quant_bits;
        __u32 start_region;
        __u32 num_regions;
};

struct snd_enc_flac {
        __u32 num;
        __u32 gain;
};

struct snd_enc_generic {
        __u32 bw;       /* encoder bandwidth */
        int reserved[15];
};

union snd_codec_options {
        struct snd_enc_wma wma;
        struct snd_enc_vorbis vorbis;
        struct snd_enc_real real;
        struct snd_enc_flac flac;
        struct snd_enc_generic generic;
};

struct snd_codec {
        __u32 id;
        __u32 ch_in;
        __u32 ch_out;
        __u32 sample_rate;
        __u32 bit_rate;
        __u32 rate_control;
        __u32 profile;
        __u32 level;
        __u32 ch_mode;
        __u32 format;
        __u32 align;
        union snd_codec_options options;
        __u32 reserved[3];
};

struct snd_compressed_buffer {
        size_t fragment_size;
        int fragments;
};

/* */
struct snd_compr_params {
        struct snd_compressed_buffer buffer;
        struct snd_codec codec;
};

struct snd_compr_tstamp {
        size_t copied_bytes;
        size_t copied_total;
        size_t decoded;
        size_t rendered;
        __u32 sampling_rate;
        uint64_t timestamp;
};

#define SNDRV_COMPRESS_GET_CAPS         _IOWR('C', 0x00, struct snd_compr_caps *)
#define SNDRV_COMPRESS_GET_CODEC_CAPS   _IOWR('C', 0x01, struct snd_compr_codec_caps *)
#define SNDRV_COMPRESS_SET_PARAMS       _IOW('C', 0x02, struct snd_compr_params *)
#define SNDRV_COMPRESS_GET_PARAMS       _IOR('C', 0x03, struct snd_compr_params *)
#define SNDRV_COMPRESS_TSTAMP           _IOR('C', 0x10, struct snd_compr_tstamp *)
#define SNDRV_COMPRESS_AVAIL            _IOR('C', 0x11, struct snd_compr_avail *)
#define SNDRV_COMPRESS_PAUSE            _IO('C', 0x20)
#define SNDRV_COMPRESS_RESUME           _IO('C', 0x21)
#define SNDRV_COMPRESS_START            _IO('C', 0x22)
#define SNDRV_COMPRESS_STOP             _IO('C', 0x23)
#define SNDRV_COMPRESS_DRAIN            _IO('C', 0x24)
#endif

#endif
