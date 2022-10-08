/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0/
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * This code has modified by Intel Corporation
 */

/*
 * Copyright (c) 2014, Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define LOG_TAG "tiny_hdmi_audio_hw"
//#define LOG_NDEBUG 0

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/time.h>
#include <stdlib.h>

#include <cutils/log.h>
#include <cutils/str_parms.h>
#include <cutils/properties.h>

#include <hardware/hardware.h>
#include <system/audio.h>
#include <hardware/audio.h>

#include <sound/asound.h>
#include <tinyalsa/asoundlib.h>

#define UNUSED_PARAMETER(x)        (void)(x)

#define DEFAULT_CARD               0
#define DEFAULT_DEVICE             0

/*this is used to avoid starvation*/
#define LATENCY_TO_BUFFER_SIZE_RATIO 2

/*Playback Channel Map*/
#define CHANNEL_MAP_REQUEST      2

/*global - keep track of the active device.
This is needed since we are supporting more
than one profile for HDMI. The Flinger
assumes we can suport multiple streams
at the same time. This makes sure only one stream
is active at a time.*/
struct pcm * activePcm = NULL;
/*TODO - move active channel inside activepcm*/
static unsigned int activeChannel;

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define STRING_TO_ENUM(string) { #string, string }

struct channel_list {
    const char *name;
    uint32_t value;
};

const struct channel_list channel_list_table[] = {
    STRING_TO_ENUM(AUDIO_CHANNEL_OUT_STEREO),
    STRING_TO_ENUM(AUDIO_CHANNEL_OUT_5POINT1),
    STRING_TO_ENUM(AUDIO_CHANNEL_OUT_7POINT1),
};

struct pcm_config pcm_config_default = {
    .channels = 2,
    .rate = 44100,
    .period_size = 1024,
    .period_count = 4,
    .format = PCM_FORMAT_S24_LE,
};

#define CHANNEL_MASK_MAX 3
struct audio_device {
    struct audio_hw_device hw_device;

    pthread_mutex_t lock;
    int card;
    int device;
    bool standby;
    int sink_sup_channels;
    audio_channel_mask_t sup_channel_masks[CHANNEL_MASK_MAX];
};

struct stream_out {
    struct audio_stream_out stream;

    pthread_mutex_t lock;
    struct pcm *pcm;
    bool standby;

/* PCM Stream Configurations */
    struct pcm_config pcm_config;
    uint32_t   channel_mask;

 /* ALSA PCM Configurations */
    uint32_t   sample_rate;
    uint32_t   buffer_size;
    uint32_t   channels;
    uint32_t   latency;

    struct audio_device *dev;
};

/**
 * NOTE: when multiple mutexes have to be acquired, always respect the
 * following order: hw device > out stream
 */

/* Helper functions */

// This function return the card number associated with the card ID (name)
// passed as argument
static int get_card_number_by_name(const char* name)
{
    char id_filepath[PATH_MAX] = {0};
    char number_filepath[PATH_MAX] = {0};
    ssize_t written;

    snprintf(id_filepath, sizeof(id_filepath), "/proc/asound/%s", name);

    written = readlink(id_filepath, number_filepath, sizeof(number_filepath));
    if (written < 0) {
        ALOGE("Sound card %s does not exist - setting default", name);
        return DEFAULT_CARD;
    } else if (written >= (ssize_t)sizeof(id_filepath)) {
        ALOGE("Sound card %s name is too long - setting default", name);
        return DEFAULT_CARD;
    }

    // We are assured, because of the check in the previous elseif, that this
    // buffer is null-terminated.  So this call is safe.
    // 4 == strlen("card")
    return atoi(number_filepath + 4);
}

static enum pcm_format Get_SinkSupported_format()
{
   /*TODO : query sink supported formats*/
   return PCM_FORMAT_S24_LE;
}

static int format_to_bits(enum pcm_format pcmformat)
{
  switch (pcmformat) {
    case PCM_FORMAT_S32_LE:
         return 32;
    case PCM_FORMAT_S24_LE:
         return 24;
    default:
    case PCM_FORMAT_S16_LE:
         return 16;
  };
}

static int make_sinkcompliant_buffers(void* input, void *output, int ipbytes)
{
  int i = 0,outbytes = 0;
  enum pcm_format in_pcmformat;
  enum pcm_format out_pcmformat;
  int *src = (int*)input;
  int *dst = (int*)output;

  /*by default android currently support only
    16 bit signed PCM*/
  in_pcmformat = PCM_FORMAT_S16_LE;
  out_pcmformat = Get_SinkSupported_format();

  switch (out_pcmformat) {
    default:
    case PCM_FORMAT_S24_LE:
    {
       ALOGV("convert 16 to 24 bits for %d",ipbytes);
       /*convert 16 bit input to 24 bit output
         in a 32 bit sample*/
       if(0 == ipbytes)
          break;

       for(i = 0; i < (ipbytes/4); i++){
          int x = (int)((int*)src)[i];
          dst[i*2] = ((int)( x & 0x0000FFFF)) << 8;
          // trying to sign exdent
          dst[i*2] = dst[i*2] << 8;
          dst[i*2] = dst[i*2] >> 8;
          //shift to middle
          dst[i*2 + 1] = (int)(( x & 0xFFFF0000) >> 8);
          dst[i*2 + 1] = dst[i*2 + 1] << 8;
          dst[i*2 + 1] = dst[i*2 + 1] >> 8;
        }
        outbytes=ipbytes * 2;

    }//case
  };//switch

  return outbytes;
}

/* must be called with hw device and output stream mutexes locked */
static int start_output_stream(struct stream_out *out)
{
    struct audio_device *adev = out->dev;
    int hdmicard = 0;

    ALOGV("%s enter",__func__);

    if ((adev->card < 0) || (adev->device < 0)){
        /*this will be updated once the hot plug intent
          sends these information.*/
        adev->card = DEFAULT_CARD;
        adev->device = DEFAULT_DEVICE;
        ALOGV("%s : Setting default card/ device %d,%d",__func__,adev->card,adev->device);
    }

    ALOGV("%s enter %d,%d,%d,%d,%d",__func__,
          out->pcm_config.channels,
          out->pcm_config.rate,
          out->pcm_config.period_size,
          out->pcm_config.period_count,
          out->pcm_config.format);

    out->pcm_config.start_threshold = 0;
    out->pcm_config.stop_threshold = 0;
    out->pcm_config.silence_threshold = 0;

    if(activePcm){
      ALOGV("Closing already open tiny alsa stream running state %d",(int)(activePcm));
      pcm_close(activePcm);
      activePcm = NULL;
    }

    /*TODO - this needs to be updated once the device connect intent sends
      card, device id*/
    adev->card = get_card_number_by_name("IntelHDMI");
    ALOGD("%s: HDMI card number = %d, device = %d",__func__,adev->card,adev->device);

    out->pcm = pcm_open(adev->card, adev->device, PCM_OUT, &out->pcm_config);

    if (out->pcm && !pcm_is_ready(out->pcm)) {
        ALOGE("pcm_open() failed: %s", pcm_get_error(out->pcm));
        pcm_close(out->pcm);
        activePcm = NULL;
        return -ENOMEM;
    }

    activePcm = out->pcm;
    activeChannel = out->pcm_config.channels;

    ALOGV("Initialized PCM device for channels %d handle = %d",out->pcm_config.channels, (int)activePcm);
    ALOGV("%s exit",__func__);
    return 0;
}

/* API functions */

static uint32_t out_get_sample_rate(const struct audio_stream *stream)
{
    struct stream_out *out = (struct stream_out *)stream;
    return out->pcm_config.rate;
}

static int out_set_sample_rate(struct audio_stream *stream, uint32_t rate)
{
    UNUSED_PARAMETER(stream);
    UNUSED_PARAMETER(rate);

    return 0;
}

static size_t out_get_buffer_size(const struct audio_stream *stream)
{
    struct stream_out *out = (struct stream_out *)stream;
    size_t buf_size;

    if(out->channel_mask > 2){
       buf_size = out->pcm_config.period_size *
                  audio_stream_out_frame_size((struct audio_stream_out *)stream);
    }
    else{
       buf_size = out->pcm_config.period_size *
                  out->pcm_config.period_count *
                  audio_stream_out_frame_size((struct audio_stream_out *)stream);

       /*latency of audio flinger is based on this
         buffer size. modifying the buffer size to avoid
         starvation*/
       buf_size/=LATENCY_TO_BUFFER_SIZE_RATIO;
    }

    ALOGV("%s : %d, period_size : %d, frame_size : %d",
        __func__,
        buf_size,
        out->pcm_config.period_size,
        audio_stream_out_frame_size((struct audio_stream_out *)stream));

    return buf_size;

}

static uint32_t out_get_channels(const struct audio_stream *stream)
{
    struct stream_out *out = (struct stream_out *)stream;
    ALOGV("%s channel mask : %x",__func__,out->channel_mask);
    return out->channel_mask;
}

static audio_format_t out_get_format(const struct audio_stream *stream)
{
    UNUSED_PARAMETER(stream);

    return AUDIO_FORMAT_PCM_16_BIT;
}

static int out_set_format(struct audio_stream *stream, audio_format_t format)
{
    UNUSED_PARAMETER(stream);
    UNUSED_PARAMETER(format);

    return 0;
}

static int out_standby(struct audio_stream *stream)
{
    struct stream_out *out = (struct stream_out *)stream;

    ALOGV("%s enter standby = %d",__func__,out->standby);

    pthread_mutex_lock(&out->dev->lock);
    pthread_mutex_lock(&out->lock);

    if (!out->standby && activePcm) {
        pcm_close(activePcm);
        out->pcm = NULL;
        out->standby = true;
        activePcm = NULL;
        ALOGV("%s PCM device closed",__func__);
    }

    pthread_mutex_unlock(&out->lock);
    pthread_mutex_unlock(&out->dev->lock);

    ALOGV("%s exit",__func__);
    return 0;
}

static int out_dump(const struct audio_stream *stream, int fd)
{
    UNUSED_PARAMETER(stream);
    UNUSED_PARAMETER(fd);
    return 0;
}

static int out_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    struct stream_out *out = (struct stream_out *)stream;
    struct audio_device *adev = out->dev;
    struct str_parms *parms;
    char value[32];
    int ret;
    int routing = 0;
    ALOGV("%s enter",__func__);

    parms = str_parms_create_str(kvpairs);

    pthread_mutex_lock(&adev->lock);

    if (parms == NULL) {
        ALOGE("couldn't extract string params from key value pairs");
        pthread_mutex_unlock(&adev->lock);
        return 0;
    }

    ret = str_parms_get_str(parms, "card", value, sizeof(value));
    if (ret >= 0)
        adev->card = atoi(value);

    ret = str_parms_get_str(parms, "device", value, sizeof(value));
    if (ret >= 0)
        adev->device = atoi(value);

    pthread_mutex_unlock(&adev->lock);
    str_parms_destroy(parms);

    ALOGV("%s exit",__func__);
    return 0;
}

static int parse_channel_map()
{
    struct mixer *mixer;
    int card = 0;
    struct mixer_ctl *ctl;
    enum mixer_ctl_type type;
    unsigned int num_values;
    unsigned int i,id;
    int chcount=0, chmap=0;

    card = get_card_number_by_name("IntelHDMI");
    mixer = mixer_open(card);
    if (!mixer) {
        ALOGE("[EDID] Failed to open mixer\n");
        goto chmap_error;
    }

    id = CHANNEL_MAP_REQUEST;
    if (id >= mixer_get_num_ctls(mixer)) {
        ALOGE("[EDID] Invalid request for channel map %d",id);
        goto chmap_error;
    }

    ctl = mixer_get_ctl_by_name(mixer, "Playback Channel Map");

    //ctl = mixer_get_ctl(mixer, id);

    type = mixer_ctl_get_type(ctl);
    num_values = mixer_ctl_get_num_values(ctl);

    ALOGV("[EDID]id = %d",id);
    ALOGV("[EDID]type = %d",type);
    ALOGV("[EDID]count = %d",num_values);

    for (i = 0; i < num_values; i++) {
      switch (type)
      {
       case MIXER_CTL_TYPE_INT:
            chmap = mixer_ctl_get_value(ctl, i);
            ALOGD("[EDID]chmap = %d", chmap);
            if(chmap > 0)  ++chcount;
            break;
       default:
            printf(" unknown");
            break;
      };
    }//for

    ALOGD("[EDID]valid number of channels supported by sink = %d",chcount);

    mixer_close(mixer);

    return chcount;

chmap_error:
    mixer_close(mixer);
    return 2;//stereo by default

}

static int out_read_edid(const struct stream_out *stream)
{
    struct stream_out *out = (struct stream_out *)stream;
    struct audio_device *adev = out->dev;

    /**read the channel max param from the sink*/
    adev->sink_sup_channels = parse_channel_map();

    if(adev->sink_sup_channels == 8) {
      adev->sup_channel_masks[0] = AUDIO_CHANNEL_OUT_5POINT1;
      adev->sup_channel_masks[1] = AUDIO_CHANNEL_OUT_7POINT1;
    }
    else if((adev->sink_sup_channels == 6) || (adev->sink_sup_channels > 2)) {
      adev->sup_channel_masks[0] = AUDIO_CHANNEL_OUT_5POINT1;
    }
    else {
      adev->sup_channel_masks[0] = AUDIO_CHANNEL_OUT_STEREO;
    }

    ALOGV("%s sink supports 0x%x max channels", __func__,adev->sink_sup_channels);
    return 0;
}

static char * out_get_parameters(const struct audio_stream *stream, const char *keys)
{
    struct stream_out *out = (struct stream_out *)stream;
    struct audio_device *adev = out->dev;
    struct str_parms *params_in = str_parms_create_str(keys);
    char *str = NULL;
    char value[256] = {0};
    int ret;
    size_t i, j;
    bool append = false;

    struct str_parms *params_out = str_parms_create();

    ALOGV("%s Entered %s", __func__,keys);

    if (params_in) {
        ret = str_parms_get_str(params_in, AUDIO_PARAMETER_STREAM_SUP_CHANNELS, value, sizeof(value));
        if (ret >= 0) {
            /*read the channel support from sink*/
            out_read_edid(out);

            value[0] = '\0';
            for (i = 0; i < CHANNEL_MASK_MAX; i++) {
               for (j = 0; j < ARRAY_SIZE(channel_list_table); j++) {
                   if (channel_list_table[j].value == adev->sup_channel_masks[i]) {
                      if (append) {
                          strcat(value, "|");
                      }
                      strcat(value, channel_list_table[j].name);
                      append = true;
                      break;
                   }
               }
            }
        }
    }
    if (params_out) {
        str_parms_add_str(params_out, AUDIO_PARAMETER_STREAM_SUP_CHANNELS, value);
        str = str_parms_to_str(params_out);
    } else {
        str = strdup(keys);
    }

    ALOGV("%s AUDIO_PARAMETER_STREAM_SUP_CHANNELS %s", __func__,str);
    if (params_in) {
        str_parms_destroy(params_in);
    }
    if (params_out) {
        str_parms_destroy(params_out);
    }

    return str;
}

static uint32_t out_get_latency(const struct audio_stream_out *stream)
{
    struct stream_out *out = (struct stream_out *)stream;
    return (out->pcm_config.period_size * out->pcm_config.period_count * 1000) /
            out_get_sample_rate(&stream->common);
}

static int out_set_volume(struct audio_stream_out *stream, float left,
                          float right)
{
    UNUSED_PARAMETER(stream);
    UNUSED_PARAMETER(left);
    UNUSED_PARAMETER(right);

    return -ENOSYS;
}

static ssize_t out_write(struct audio_stream_out *stream, const void* buffer,
                         size_t bytes)
{
    int ret = 0;
    struct stream_out *out = (struct stream_out *)stream;
    int32_t* dstbuff = NULL;
    int outbytes = 0;

    ALOGV("%s enter for bytes = %d channels = %d",__func__,bytes, out->pcm_config.channels);

    pthread_mutex_lock(&out->dev->lock);
    pthread_mutex_lock(&out->lock);

    if(activePcm == NULL) {
       ALOGV("%s: previous stream closed- open again",__func__);
       out->standby = true;
    }

    if (out->standby) {
        ret = start_output_stream(out);
        if (ret != 0) {
            goto err;
        }
        out->standby = false;
    }

    if((!out->pcm) || (activeChannel != out->pcm_config.channels)){
       ALOGD("%s: null handle to write - device already closed",__func__);
       goto err;
    }

    if(Get_SinkSupported_format() == out->pcm_config.format){

       /*16 bit data will be converted to 24 bit over 32 bit data type
       hence the multiplier 2*/
       dstbuff = (int32_t*)malloc(bytes* 2);
       if (!dstbuff) {
           pthread_mutex_unlock(&out->lock);
           pthread_mutex_unlock(&out->dev->lock);
           ALOGE("%s : memory allocation failed",__func__);
           return -ENOMEM;
       }

       memset(dstbuff,0,bytes * 2);

       outbytes = make_sinkcompliant_buffers((void*)buffer, (void*)dstbuff,bytes);
     } //if()for conversion

    if(dstbuff){
      ret = pcm_write(out->pcm, (void *)dstbuff, outbytes);
    }
    else
      ret = pcm_write(out->pcm, (void *)buffer, bytes);

    ALOGV("pcm_write: %s done for %d input bytes, output bytes = %d ", pcm_get_error(out->pcm),bytes,outbytes);

    free(dstbuff);

err:
    pthread_mutex_unlock(&out->lock);
    pthread_mutex_unlock(&out->dev->lock);

   if(ret !=0){
    uint64_t duration_ms = ((bytes * 1000)/
                            (audio_stream_out_frame_size(stream)) /
                            (out_get_sample_rate(&stream->common)));
    ALOGV("%s : silence written", __func__);
    usleep(duration_ms * 1000);
   }

    ALOGV("%s exit",__func__);
    return bytes;
}

static int out_get_render_position(const struct audio_stream_out *stream,
                                   uint32_t *dsp_frames)
{
    UNUSED_PARAMETER(stream);
    UNUSED_PARAMETER(dsp_frames);

    return -EINVAL;
}

static int out_add_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    UNUSED_PARAMETER(stream);
    UNUSED_PARAMETER(effect);

    return 0;
}

static int out_remove_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    UNUSED_PARAMETER(stream);
    UNUSED_PARAMETER(effect);

    return 0;
}

static int out_get_next_write_timestamp(const struct audio_stream_out *stream,
                                        int64_t *timestamp)
{
    UNUSED_PARAMETER(stream);
    UNUSED_PARAMETER(timestamp);

    return -EINVAL;
}

static int adev_open_output_stream(struct audio_hw_device *dev,
                                   audio_io_handle_t handle,
                                   audio_devices_t devices,
                                   audio_output_flags_t flags,
                                   struct audio_config *config,
                                   struct audio_stream_out **stream_out,
                                   const char *address)
{
    UNUSED_PARAMETER(devices);
    UNUSED_PARAMETER(handle);
    UNUSED_PARAMETER(address);

    struct audio_device *adev = (struct audio_device *)dev;
    struct stream_out *out;
    int ret;
    ALOGV("%s enter",__func__);

    out = (struct stream_out *)calloc(1, sizeof(struct stream_out));
    if (!out)
        return -ENOMEM;


    out->dev = adev;
    out->channel_mask = AUDIO_CHANNEL_OUT_STEREO;
    adev->sup_channel_masks[0] = AUDIO_CHANNEL_OUT_STEREO;

    if (flags & AUDIO_OUTPUT_FLAG_DIRECT) {
        ALOGV("%s: HDMI Multichannel",__func__);
        if (config->sample_rate == 0)
            config->sample_rate = pcm_config_default.rate;
        if (config->channel_mask == 0){
            /*read the channel support from sink*/
            out_read_edid(out);
            if(config->channel_mask == 0)
               config->channel_mask = AUDIO_CHANNEL_OUT_5POINT1;
        }
    } else {
        ALOGV("%s: HDMI Stereo",__func__);
        if (config->sample_rate == 0)
            config->sample_rate = pcm_config_default.rate;
        if (config->channel_mask == 0)
            config->channel_mask = AUDIO_CHANNEL_OUT_STEREO;
    }

    out->channel_mask                      = config->channel_mask;

    out->pcm_config.channels               = popcount(config->channel_mask);
    out->pcm_config.rate                   = config->sample_rate;
    out->pcm_config.period_size            = pcm_config_default.period_size;
    out->pcm_config.period_count           = pcm_config_default.period_count;
    out->pcm_config.format                 = pcm_config_default.format;

    out->stream.common.get_sample_rate     = out_get_sample_rate;
    out->stream.common.set_sample_rate     = out_set_sample_rate;
    out->stream.common.get_buffer_size     = out_get_buffer_size;
    out->stream.common.get_channels        = out_get_channels;
    out->stream.common.get_format          = out_get_format;
    out->stream.common.set_format          = out_set_format;
    out->stream.common.standby             = out_standby;
    out->stream.common.dump                = out_dump;
    out->stream.common.set_parameters      = out_set_parameters;
    out->stream.common.get_parameters      = out_get_parameters;
    out->stream.common.add_audio_effect    = out_add_audio_effect;
    out->stream.common.remove_audio_effect = out_remove_audio_effect;
    out->stream.get_latency                = out_get_latency;
    out->stream.set_volume                 = out_set_volume;
    out->stream.write                      = out_write;
    out->stream.get_render_position        = out_get_render_position;
    out->stream.get_next_write_timestamp   = out_get_next_write_timestamp;

    config->format = out_get_format(&out->stream.common);
    config->channel_mask = out_get_channels(&out->stream.common);
    config->sample_rate = out_get_sample_rate(&out->stream.common);

    out->standby = true;

    adev->card = -1;
    adev->device = -1;

    pthread_mutex_lock(&out->dev->lock);
    pthread_mutex_lock(&out->lock);

    if(activePcm){
      ALOGV("Closing already open tiny alsa stream %d",(int)out->pcm);
      pcm_close(activePcm);
      activePcm = NULL;
    }
    ret = start_output_stream(out);
    if (ret != 0) {
        ALOGV("%s: stream start failed", __func__);
        goto err_open;
    }

    out->standby = false;

    *stream_out = &out->stream;

    pthread_mutex_unlock(&out->lock);
    pthread_mutex_unlock(&out->dev->lock);

    ALOGV("%s exit",__func__);
    return 0;

err_open:
    ALOGE("%s exit with error",__func__);
    pthread_mutex_unlock(&out->lock);
    pthread_mutex_unlock(&out->dev->lock);
    free(out);
    *stream_out = NULL;
    return ret;
}

static void adev_close_output_stream(struct audio_hw_device *dev,
                                     struct audio_stream_out *stream)
{
    UNUSED_PARAMETER(dev);

    struct stream_out *out = (struct stream_out *)stream;

    ALOGV("%s enter",__func__);
    out->standby = false;
    out_standby(&stream->common);
    free(stream);
    ALOGV("%s exit",__func__);
}

static int adev_set_parameters(struct audio_hw_device *dev, const char *kvpairs)
{
    UNUSED_PARAMETER(dev);
    UNUSED_PARAMETER(kvpairs);

    return 0;
}

static char * adev_get_parameters(const struct audio_hw_device *dev,
                                  const char *keys)
{
    UNUSED_PARAMETER(dev);
    UNUSED_PARAMETER(keys);

    return strdup("");
}

static int adev_init_check(const struct audio_hw_device *dev)
{
    UNUSED_PARAMETER(dev);

    return 0;
}

static int adev_set_voice_volume(struct audio_hw_device *dev, float volume)
{
    UNUSED_PARAMETER(dev);
    UNUSED_PARAMETER(volume);

    return -ENOSYS;
}

static int adev_set_master_volume(struct audio_hw_device *dev, float volume)
{
    UNUSED_PARAMETER(dev);
    UNUSED_PARAMETER(volume);

    return -ENOSYS;
}

static int adev_set_mode(struct audio_hw_device *dev, audio_mode_t mode)
{
    UNUSED_PARAMETER(dev);
    UNUSED_PARAMETER(mode);

    return 0;
}

static int adev_set_mic_mute(struct audio_hw_device *dev, bool state)
{
    UNUSED_PARAMETER(dev);
    UNUSED_PARAMETER(state);

    return -ENOSYS;
}

static int adev_get_mic_mute(const struct audio_hw_device *dev, bool *state)
{
    UNUSED_PARAMETER(dev);
    UNUSED_PARAMETER(state);

    return -ENOSYS;
}

static size_t adev_get_input_buffer_size(const struct audio_hw_device *dev,
                                         const struct audio_config *config)
{
    UNUSED_PARAMETER(dev);
    UNUSED_PARAMETER(config);

    return 0;
}

static int adev_open_input_stream(struct audio_hw_device *dev,
                                  audio_io_handle_t handle,
                                  audio_devices_t devices,
                                  struct audio_config *config,
                                  struct audio_stream_in **stream_in,
                                  audio_input_flags_t flags,
                                  const char *address,
                                  audio_source_t source)
{
    UNUSED_PARAMETER(dev);
    UNUSED_PARAMETER(handle);
    UNUSED_PARAMETER(devices);
    UNUSED_PARAMETER(config);
    UNUSED_PARAMETER(stream_in);
    UNUSED_PARAMETER(flags);
    UNUSED_PARAMETER(address);
    UNUSED_PARAMETER(source);

    return -ENOSYS;
}

static void adev_close_input_stream(struct audio_hw_device *dev,
                                   struct audio_stream_in *stream)
{
    UNUSED_PARAMETER(dev);
    UNUSED_PARAMETER(stream);
}

static int adev_dump(const audio_hw_device_t *device, int fd)
{
    UNUSED_PARAMETER(device);
    UNUSED_PARAMETER(fd);

    return 0;
}

static int adev_close(hw_device_t *device)
{
    struct audio_device *adev = (struct audio_device *)device;

    free(device);
    return 0;
}

static int adev_open(const hw_module_t* module, const char* name,
                     hw_device_t** device)
{
    struct audio_device *adev;
    int ret;

    ALOGV("%s enter",__func__);

    if (strcmp(name, AUDIO_HARDWARE_INTERFACE) != 0)
        return -EINVAL;

    adev = calloc(1, sizeof(struct audio_device));
    if (!adev)
        return -ENOMEM;

    adev->hw_device.common.tag = HARDWARE_DEVICE_TAG;
    adev->hw_device.common.version = AUDIO_DEVICE_API_VERSION_2_0;
    adev->hw_device.common.module = (struct hw_module_t *) module;
    adev->hw_device.common.close = adev_close;

    adev->hw_device.init_check = adev_init_check;
    adev->hw_device.set_voice_volume = adev_set_voice_volume;
    adev->hw_device.set_master_volume = adev_set_master_volume;
    adev->hw_device.set_mode = adev_set_mode;
    adev->hw_device.set_mic_mute = adev_set_mic_mute;
    adev->hw_device.get_mic_mute = adev_get_mic_mute;
    adev->hw_device.set_parameters = adev_set_parameters;
    adev->hw_device.get_parameters = adev_get_parameters;
    adev->hw_device.get_input_buffer_size = adev_get_input_buffer_size;
    adev->hw_device.open_output_stream = adev_open_output_stream;
    adev->hw_device.close_output_stream = adev_close_output_stream;
    adev->hw_device.open_input_stream = adev_open_input_stream;
    adev->hw_device.close_input_stream = adev_close_input_stream;
    adev->hw_device.dump = adev_dump;

    *device = &adev->hw_device.common;

    ALOGV("%s exit",__func__);

    return 0;
}

static struct hw_module_methods_t hal_module_methods = {
    .open = adev_open,
};

struct audio_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = AUDIO_MODULE_API_VERSION_0_1,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = AUDIO_HARDWARE_MODULE_ID,
        .name = "tiny_hdmi audio HW HAL",
        .author = "The Android Open Source Project",
        .methods = &hal_module_methods,
    },
};
