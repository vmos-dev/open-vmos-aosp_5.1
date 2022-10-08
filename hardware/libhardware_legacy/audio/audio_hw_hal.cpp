/*
 * Copyright (C) 2011 The Android Open Source Project
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

#define LOG_TAG "legacy_audio_hw_hal"
//#define LOG_NDEBUG 0

#include <stdint.h>

#include <hardware/hardware.h>
#include <system/audio.h>
#include <hardware/audio.h>

#include <hardware_legacy/AudioHardwareInterface.h>
#include <hardware_legacy/AudioSystemLegacy.h>

namespace android_audio_legacy {

extern "C" {

struct legacy_audio_module {
    struct audio_module module;
};

struct legacy_audio_device {
    struct audio_hw_device device;

    struct AudioHardwareInterface *hwif;
};

struct legacy_stream_out {
    struct audio_stream_out stream;

    AudioStreamOut *legacy_out;
};

struct legacy_stream_in {
    struct audio_stream_in stream;

    AudioStreamIn *legacy_in;
};


enum {
    HAL_API_REV_1_0,
    HAL_API_REV_2_0,
    HAL_API_REV_NUM
} hal_api_rev;

static uint32_t audio_device_conv_table[][HAL_API_REV_NUM] =
{
    /* output devices */
    { AudioSystem::DEVICE_OUT_EARPIECE, AUDIO_DEVICE_OUT_EARPIECE },
    { AudioSystem::DEVICE_OUT_SPEAKER, AUDIO_DEVICE_OUT_SPEAKER },
    { AudioSystem::DEVICE_OUT_WIRED_HEADSET, AUDIO_DEVICE_OUT_WIRED_HEADSET },
    { AudioSystem::DEVICE_OUT_WIRED_HEADPHONE, AUDIO_DEVICE_OUT_WIRED_HEADPHONE },
    { AudioSystem::DEVICE_OUT_BLUETOOTH_SCO, AUDIO_DEVICE_OUT_BLUETOOTH_SCO },
    { AudioSystem::DEVICE_OUT_BLUETOOTH_SCO_HEADSET, AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET },
    { AudioSystem::DEVICE_OUT_BLUETOOTH_SCO_CARKIT, AUDIO_DEVICE_OUT_BLUETOOTH_SCO_CARKIT },
    { AudioSystem::DEVICE_OUT_BLUETOOTH_A2DP, AUDIO_DEVICE_OUT_BLUETOOTH_A2DP },
    { AudioSystem::DEVICE_OUT_BLUETOOTH_A2DP_HEADPHONES, AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_HEADPHONES },
    { AudioSystem::DEVICE_OUT_BLUETOOTH_A2DP_SPEAKER, AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_SPEAKER },
    { AudioSystem::DEVICE_OUT_AUX_DIGITAL, AUDIO_DEVICE_OUT_AUX_DIGITAL },
    { AudioSystem::DEVICE_OUT_ANLG_DOCK_HEADSET, AUDIO_DEVICE_OUT_ANLG_DOCK_HEADSET },
    { AudioSystem::DEVICE_OUT_DGTL_DOCK_HEADSET, AUDIO_DEVICE_OUT_DGTL_DOCK_HEADSET },
    { AudioSystem::DEVICE_OUT_DEFAULT, AUDIO_DEVICE_OUT_DEFAULT },
    /* input devices */
    { AudioSystem::DEVICE_IN_COMMUNICATION, AUDIO_DEVICE_IN_COMMUNICATION },
    { AudioSystem::DEVICE_IN_AMBIENT, AUDIO_DEVICE_IN_AMBIENT },
    { AudioSystem::DEVICE_IN_BUILTIN_MIC, AUDIO_DEVICE_IN_BUILTIN_MIC },
    { AudioSystem::DEVICE_IN_BLUETOOTH_SCO_HEADSET, AUDIO_DEVICE_IN_BLUETOOTH_SCO_HEADSET },
    { AudioSystem::DEVICE_IN_WIRED_HEADSET, AUDIO_DEVICE_IN_WIRED_HEADSET },
    { AudioSystem::DEVICE_IN_AUX_DIGITAL, AUDIO_DEVICE_IN_AUX_DIGITAL },
    { AudioSystem::DEVICE_IN_VOICE_CALL, AUDIO_DEVICE_IN_VOICE_CALL },
    { AudioSystem::DEVICE_IN_BACK_MIC, AUDIO_DEVICE_IN_BACK_MIC },
    { AudioSystem::DEVICE_IN_DEFAULT, AUDIO_DEVICE_IN_DEFAULT },
};

static uint32_t convert_audio_device(uint32_t from_device, int from_rev, int to_rev)
{
    const uint32_t k_num_devices = sizeof(audio_device_conv_table)/sizeof(uint32_t)/HAL_API_REV_NUM;
    uint32_t to_device = AUDIO_DEVICE_NONE;
    uint32_t in_bit = 0;

    if (from_rev != HAL_API_REV_1_0) {
        in_bit = from_device & AUDIO_DEVICE_BIT_IN;
        from_device &= ~AUDIO_DEVICE_BIT_IN;
    }

    while (from_device) {
        uint32_t i = 31 - __builtin_clz(from_device);
        uint32_t cur_device = (1 << i) | in_bit;

        for (i = 0; i < k_num_devices; i++) {
            if (audio_device_conv_table[i][from_rev] == cur_device) {
                to_device |= audio_device_conv_table[i][to_rev];
                break;
            }
        }
        from_device &= ~cur_device;
    }
    return to_device;
}


/** audio_stream_out implementation **/
static uint32_t out_get_sample_rate(const struct audio_stream *stream)
{
    const struct legacy_stream_out *out =
        reinterpret_cast<const struct legacy_stream_out *>(stream);
    return out->legacy_out->sampleRate();
}

static int out_set_sample_rate(struct audio_stream *stream, uint32_t rate)
{
    struct legacy_stream_out *out =
        reinterpret_cast<struct legacy_stream_out *>(stream);

    ALOGE("(%s:%d) %s: Implement me!", __FILE__, __LINE__, __func__);
    /* TODO: implement this */
    return 0;
}

static size_t out_get_buffer_size(const struct audio_stream *stream)
{
    const struct legacy_stream_out *out =
        reinterpret_cast<const struct legacy_stream_out *>(stream);
    return out->legacy_out->bufferSize();
}

static audio_channel_mask_t out_get_channels(const struct audio_stream *stream)
{
    const struct legacy_stream_out *out =
        reinterpret_cast<const struct legacy_stream_out *>(stream);
    return (audio_channel_mask_t) out->legacy_out->channels();
}

static audio_format_t out_get_format(const struct audio_stream *stream)
{
    const struct legacy_stream_out *out =
        reinterpret_cast<const struct legacy_stream_out *>(stream);
    // legacy API, don't change return type
    return (audio_format_t) out->legacy_out->format();
}

static int out_set_format(struct audio_stream *stream, audio_format_t format)
{
    struct legacy_stream_out *out =
        reinterpret_cast<struct legacy_stream_out *>(stream);
    ALOGE("(%s:%d) %s: Implement me!", __FILE__, __LINE__, __func__);
    /* TODO: implement me */
    return 0;
}

static int out_standby(struct audio_stream *stream)
{
    struct legacy_stream_out *out =
        reinterpret_cast<struct legacy_stream_out *>(stream);
    return out->legacy_out->standby();
}

static int out_dump(const struct audio_stream *stream, int fd)
{
    const struct legacy_stream_out *out =
        reinterpret_cast<const struct legacy_stream_out *>(stream);
    Vector<String16> args;
    return out->legacy_out->dump(fd, args);
}

static int out_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    struct legacy_stream_out *out =
        reinterpret_cast<struct legacy_stream_out *>(stream);
    int val;
    String8 s8 = String8(kvpairs);
    AudioParameter parms = AudioParameter(String8(kvpairs));

    if (parms.getInt(String8(AUDIO_PARAMETER_STREAM_ROUTING), val) == NO_ERROR) {
        val = convert_audio_device(val, HAL_API_REV_2_0, HAL_API_REV_1_0);
        parms.remove(String8(AUDIO_PARAMETER_STREAM_ROUTING));
        parms.addInt(String8(AUDIO_PARAMETER_STREAM_ROUTING), val);
        s8 = parms.toString();
    }

    return out->legacy_out->setParameters(s8);
}

static char * out_get_parameters(const struct audio_stream *stream, const char *keys)
{
    const struct legacy_stream_out *out =
        reinterpret_cast<const struct legacy_stream_out *>(stream);
    String8 s8;
    int val;

    s8 = out->legacy_out->getParameters(String8(keys));

    AudioParameter parms = AudioParameter(s8);
    if (parms.getInt(String8(AUDIO_PARAMETER_STREAM_ROUTING), val) == NO_ERROR) {
        val = convert_audio_device(val, HAL_API_REV_1_0, HAL_API_REV_2_0);
        parms.remove(String8(AUDIO_PARAMETER_STREAM_ROUTING));
        parms.addInt(String8(AUDIO_PARAMETER_STREAM_ROUTING), val);
        s8 = parms.toString();
    }

    return strdup(s8.string());
}

static uint32_t out_get_latency(const struct audio_stream_out *stream)
{
    const struct legacy_stream_out *out =
        reinterpret_cast<const struct legacy_stream_out *>(stream);
    return out->legacy_out->latency();
}

static int out_set_volume(struct audio_stream_out *stream, float left,
                          float right)
{
    struct legacy_stream_out *out =
        reinterpret_cast<struct legacy_stream_out *>(stream);
    return out->legacy_out->setVolume(left, right);
}

static ssize_t out_write(struct audio_stream_out *stream, const void* buffer,
                         size_t bytes)
{
    struct legacy_stream_out *out =
        reinterpret_cast<struct legacy_stream_out *>(stream);
    return out->legacy_out->write(buffer, bytes);
}

static int out_get_render_position(const struct audio_stream_out *stream,
                                   uint32_t *dsp_frames)
{
    const struct legacy_stream_out *out =
        reinterpret_cast<const struct legacy_stream_out *>(stream);
    return out->legacy_out->getRenderPosition(dsp_frames);
}

static int out_get_next_write_timestamp(const struct audio_stream_out *stream,
                                        int64_t *timestamp)
{
    const struct legacy_stream_out *out =
        reinterpret_cast<const struct legacy_stream_out *>(stream);
    return out->legacy_out->getNextWriteTimestamp(timestamp);
}

static int out_add_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    return 0;
}

static int out_remove_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    return 0;
}

/** audio_stream_in implementation **/
static uint32_t in_get_sample_rate(const struct audio_stream *stream)
{
    const struct legacy_stream_in *in =
        reinterpret_cast<const struct legacy_stream_in *>(stream);
    return in->legacy_in->sampleRate();
}

static int in_set_sample_rate(struct audio_stream *stream, uint32_t rate)
{
    struct legacy_stream_in *in =
        reinterpret_cast<struct legacy_stream_in *>(stream);

    ALOGE("(%s:%d) %s: Implement me!", __FILE__, __LINE__, __func__);
    /* TODO: implement this */
    return 0;
}

static size_t in_get_buffer_size(const struct audio_stream *stream)
{
    const struct legacy_stream_in *in =
        reinterpret_cast<const struct legacy_stream_in *>(stream);
    return in->legacy_in->bufferSize();
}

static audio_channel_mask_t in_get_channels(const struct audio_stream *stream)
{
    const struct legacy_stream_in *in =
        reinterpret_cast<const struct legacy_stream_in *>(stream);
    return (audio_channel_mask_t) in->legacy_in->channels();
}

static audio_format_t in_get_format(const struct audio_stream *stream)
{
    const struct legacy_stream_in *in =
        reinterpret_cast<const struct legacy_stream_in *>(stream);
    // legacy API, don't change return type
    return (audio_format_t) in->legacy_in->format();
}

static int in_set_format(struct audio_stream *stream, audio_format_t format)
{
    struct legacy_stream_in *in =
        reinterpret_cast<struct legacy_stream_in *>(stream);
    ALOGE("(%s:%d) %s: Implement me!", __FILE__, __LINE__, __func__);
    /* TODO: implement me */
    return 0;
}

static int in_standby(struct audio_stream *stream)
{
    struct legacy_stream_in *in = reinterpret_cast<struct legacy_stream_in *>(stream);
    return in->legacy_in->standby();
}

static int in_dump(const struct audio_stream *stream, int fd)
{
    const struct legacy_stream_in *in =
        reinterpret_cast<const struct legacy_stream_in *>(stream);
    Vector<String16> args;
    return in->legacy_in->dump(fd, args);
}

static int in_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    struct legacy_stream_in *in =
        reinterpret_cast<struct legacy_stream_in *>(stream);
    int val;
    AudioParameter parms = AudioParameter(String8(kvpairs));
    String8 s8 = String8(kvpairs);

    if (parms.getInt(String8(AUDIO_PARAMETER_STREAM_ROUTING), val) == NO_ERROR) {
        val = convert_audio_device(val, HAL_API_REV_2_0, HAL_API_REV_1_0);
        parms.remove(String8(AUDIO_PARAMETER_STREAM_ROUTING));
        parms.addInt(String8(AUDIO_PARAMETER_STREAM_ROUTING), val);
        s8 = parms.toString();
    }

    return in->legacy_in->setParameters(s8);
}

static char * in_get_parameters(const struct audio_stream *stream,
                                const char *keys)
{
    const struct legacy_stream_in *in =
        reinterpret_cast<const struct legacy_stream_in *>(stream);
    String8 s8;
    int val;

    s8 = in->legacy_in->getParameters(String8(keys));

    AudioParameter parms = AudioParameter(s8);
    if (parms.getInt(String8(AUDIO_PARAMETER_STREAM_ROUTING), val) == NO_ERROR) {
        val = convert_audio_device(val, HAL_API_REV_1_0, HAL_API_REV_2_0);
        parms.remove(String8(AUDIO_PARAMETER_STREAM_ROUTING));
        parms.addInt(String8(AUDIO_PARAMETER_STREAM_ROUTING), val);
        s8 = parms.toString();
    }

    return strdup(s8.string());
}

static int in_set_gain(struct audio_stream_in *stream, float gain)
{
    struct legacy_stream_in *in =
        reinterpret_cast<struct legacy_stream_in *>(stream);
    return in->legacy_in->setGain(gain);
}

static ssize_t in_read(struct audio_stream_in *stream, void* buffer,
                       size_t bytes)
{
    struct legacy_stream_in *in =
        reinterpret_cast<struct legacy_stream_in *>(stream);
    return in->legacy_in->read(buffer, bytes);
}

static uint32_t in_get_input_frames_lost(struct audio_stream_in *stream)
{
    struct legacy_stream_in *in =
        reinterpret_cast<struct legacy_stream_in *>(stream);
    return in->legacy_in->getInputFramesLost();
}

static int in_add_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    const struct legacy_stream_in *in =
        reinterpret_cast<const struct legacy_stream_in *>(stream);
    return in->legacy_in->addAudioEffect(effect);
}

static int in_remove_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    const struct legacy_stream_in *in =
        reinterpret_cast<const struct legacy_stream_in *>(stream);
    return in->legacy_in->removeAudioEffect(effect);
}

/** audio_hw_device implementation **/
static inline struct legacy_audio_device * to_ladev(struct audio_hw_device *dev)
{
    return reinterpret_cast<struct legacy_audio_device *>(dev);
}

static inline const struct legacy_audio_device * to_cladev(const struct audio_hw_device *dev)
{
    return reinterpret_cast<const struct legacy_audio_device *>(dev);
}

static int adev_init_check(const struct audio_hw_device *dev)
{
    const struct legacy_audio_device *ladev = to_cladev(dev);

    return ladev->hwif->initCheck();
}

static int adev_set_voice_volume(struct audio_hw_device *dev, float volume)
{
    struct legacy_audio_device *ladev = to_ladev(dev);
    return ladev->hwif->setVoiceVolume(volume);
}

static int adev_set_master_volume(struct audio_hw_device *dev, float volume)
{
    struct legacy_audio_device *ladev = to_ladev(dev);
    return ladev->hwif->setMasterVolume(volume);
}

static int adev_get_master_volume(struct audio_hw_device *dev, float* volume)
{
    struct legacy_audio_device *ladev = to_ladev(dev);
    return ladev->hwif->getMasterVolume(volume);
}

static int adev_set_mode(struct audio_hw_device *dev, audio_mode_t mode)
{
    struct legacy_audio_device *ladev = to_ladev(dev);
    // as this is the legacy API, don't change it to use audio_mode_t instead of int
    return ladev->hwif->setMode((int) mode);
}

static int adev_set_mic_mute(struct audio_hw_device *dev, bool state)
{
    struct legacy_audio_device *ladev = to_ladev(dev);
    return ladev->hwif->setMicMute(state);
}

static int adev_get_mic_mute(const struct audio_hw_device *dev, bool *state)
{
    const struct legacy_audio_device *ladev = to_cladev(dev);
    return ladev->hwif->getMicMute(state);
}

static int adev_set_parameters(struct audio_hw_device *dev, const char *kvpairs)
{
    struct legacy_audio_device *ladev = to_ladev(dev);
    return ladev->hwif->setParameters(String8(kvpairs));
}

static char * adev_get_parameters(const struct audio_hw_device *dev,
                                  const char *keys)
{
    const struct legacy_audio_device *ladev = to_cladev(dev);
    String8 s8;

    s8 = ladev->hwif->getParameters(String8(keys));
    return strdup(s8.string());
}

static size_t adev_get_input_buffer_size(const struct audio_hw_device *dev,
                                         const struct audio_config *config)
{
    const struct legacy_audio_device *ladev = to_cladev(dev);
    return ladev->hwif->getInputBufferSize(config->sample_rate, (int) config->format,
                                           audio_channel_count_from_in_mask(config->channel_mask));
}

static int adev_open_output_stream(struct audio_hw_device *dev,
                                   audio_io_handle_t handle,
                                   audio_devices_t devices,
                                   audio_output_flags_t flags,
                                   struct audio_config *config,
                                   struct audio_stream_out **stream_out,
                                   const char *address __unused)
{
    struct legacy_audio_device *ladev = to_ladev(dev);
    status_t status;
    struct legacy_stream_out *out;
    int ret;

    out = (struct legacy_stream_out *)calloc(1, sizeof(*out));
    if (!out)
        return -ENOMEM;

    devices = convert_audio_device(devices, HAL_API_REV_2_0, HAL_API_REV_1_0);

    out->legacy_out = ladev->hwif->openOutputStreamWithFlags(devices, flags,
                                                    (int *) &config->format,
                                                    &config->channel_mask,
                                                    &config->sample_rate, &status);
    if (!out->legacy_out) {
        ret = status;
        goto err_open;
    }

    out->stream.common.get_sample_rate = out_get_sample_rate;
    out->stream.common.set_sample_rate = out_set_sample_rate;
    out->stream.common.get_buffer_size = out_get_buffer_size;
    out->stream.common.get_channels = out_get_channels;
    out->stream.common.get_format = out_get_format;
    out->stream.common.set_format = out_set_format;
    out->stream.common.standby = out_standby;
    out->stream.common.dump = out_dump;
    out->stream.common.set_parameters = out_set_parameters;
    out->stream.common.get_parameters = out_get_parameters;
    out->stream.common.add_audio_effect = out_add_audio_effect;
    out->stream.common.remove_audio_effect = out_remove_audio_effect;
    out->stream.get_latency = out_get_latency;
    out->stream.set_volume = out_set_volume;
    out->stream.write = out_write;
    out->stream.get_render_position = out_get_render_position;
    out->stream.get_next_write_timestamp = out_get_next_write_timestamp;

    *stream_out = &out->stream;
    return 0;

err_open:
    free(out);
    *stream_out = NULL;
    return ret;
}

static void adev_close_output_stream(struct audio_hw_device *dev,
                                     struct audio_stream_out* stream)
{
    struct legacy_audio_device *ladev = to_ladev(dev);
    struct legacy_stream_out *out = reinterpret_cast<struct legacy_stream_out *>(stream);

    ladev->hwif->closeOutputStream(out->legacy_out);
    free(out);
}

/** This method creates and opens the audio hardware input stream */
static int adev_open_input_stream(struct audio_hw_device *dev,
                                  audio_io_handle_t handle,
                                  audio_devices_t devices,
                                  struct audio_config *config,
                                  struct audio_stream_in **stream_in,
                                  audio_input_flags_t flags __unused,
                                  const char *address __unused,
                                  audio_source_t source __unused)
{
    struct legacy_audio_device *ladev = to_ladev(dev);
    status_t status;
    struct legacy_stream_in *in;
    int ret;

    in = (struct legacy_stream_in *)calloc(1, sizeof(*in));
    if (!in)
        return -ENOMEM;

    devices = convert_audio_device(devices, HAL_API_REV_2_0, HAL_API_REV_1_0);

    in->legacy_in = ladev->hwif->openInputStream(devices, (int *) &config->format,
                                                 &config->channel_mask, &config->sample_rate,
                                                 &status, (AudioSystem::audio_in_acoustics)0);
    if (!in->legacy_in) {
        ret = status;
        goto err_open;
    }

    in->stream.common.get_sample_rate = in_get_sample_rate;
    in->stream.common.set_sample_rate = in_set_sample_rate;
    in->stream.common.get_buffer_size = in_get_buffer_size;
    in->stream.common.get_channels = in_get_channels;
    in->stream.common.get_format = in_get_format;
    in->stream.common.set_format = in_set_format;
    in->stream.common.standby = in_standby;
    in->stream.common.dump = in_dump;
    in->stream.common.set_parameters = in_set_parameters;
    in->stream.common.get_parameters = in_get_parameters;
    in->stream.common.add_audio_effect = in_add_audio_effect;
    in->stream.common.remove_audio_effect = in_remove_audio_effect;
    in->stream.set_gain = in_set_gain;
    in->stream.read = in_read;
    in->stream.get_input_frames_lost = in_get_input_frames_lost;

    *stream_in = &in->stream;
    return 0;

err_open:
    free(in);
    *stream_in = NULL;
    return ret;
}

static void adev_close_input_stream(struct audio_hw_device *dev,
                               struct audio_stream_in *stream)
{
    struct legacy_audio_device *ladev = to_ladev(dev);
    struct legacy_stream_in *in =
        reinterpret_cast<struct legacy_stream_in *>(stream);

    ladev->hwif->closeInputStream(in->legacy_in);
    free(in);
}

static int adev_dump(const struct audio_hw_device *dev, int fd)
{
    const struct legacy_audio_device *ladev = to_cladev(dev);
    Vector<String16> args;

    return ladev->hwif->dumpState(fd, args);
}

static int legacy_adev_close(hw_device_t* device)
{
    struct audio_hw_device *hwdev =
                        reinterpret_cast<struct audio_hw_device *>(device);
    struct legacy_audio_device *ladev = to_ladev(hwdev);

    if (!ladev)
        return 0;

    if (ladev->hwif)
        delete ladev->hwif;

    free(ladev);
    return 0;
}

static int legacy_adev_open(const hw_module_t* module, const char* name,
                            hw_device_t** device)
{
    struct legacy_audio_device *ladev;
    int ret;

    if (strcmp(name, AUDIO_HARDWARE_INTERFACE) != 0)
        return -EINVAL;

    ladev = (struct legacy_audio_device *)calloc(1, sizeof(*ladev));
    if (!ladev)
        return -ENOMEM;

    ladev->device.common.tag = HARDWARE_DEVICE_TAG;
    ladev->device.common.version = AUDIO_DEVICE_API_VERSION_2_0;
    ladev->device.common.module = const_cast<hw_module_t*>(module);
    ladev->device.common.close = legacy_adev_close;

    ladev->device.init_check = adev_init_check;
    ladev->device.set_voice_volume = adev_set_voice_volume;
    ladev->device.set_master_volume = adev_set_master_volume;
    ladev->device.get_master_volume = adev_get_master_volume;
    ladev->device.set_mode = adev_set_mode;
    ladev->device.set_mic_mute = adev_set_mic_mute;
    ladev->device.get_mic_mute = adev_get_mic_mute;
    ladev->device.set_parameters = adev_set_parameters;
    ladev->device.get_parameters = adev_get_parameters;
    ladev->device.get_input_buffer_size = adev_get_input_buffer_size;
    ladev->device.open_output_stream = adev_open_output_stream;
    ladev->device.close_output_stream = adev_close_output_stream;
    ladev->device.open_input_stream = adev_open_input_stream;
    ladev->device.close_input_stream = adev_close_input_stream;
    ladev->device.dump = adev_dump;

    ladev->hwif = createAudioHardware();
    if (!ladev->hwif) {
        ret = -EIO;
        goto err_create_audio_hw;
    }

    *device = &ladev->device.common;

    return 0;

err_create_audio_hw:
    free(ladev);
    return ret;
}

static struct hw_module_methods_t legacy_audio_module_methods = {
        open: legacy_adev_open
};

struct legacy_audio_module HAL_MODULE_INFO_SYM = {
    module: {
        common: {
            tag: HARDWARE_MODULE_TAG,
            module_api_version: AUDIO_MODULE_API_VERSION_0_1,
            hal_api_version: HARDWARE_HAL_API_VERSION,
            id: AUDIO_HARDWARE_MODULE_ID,
            name: "LEGACY Audio HW HAL",
            author: "The Android Open Source Project",
            methods: &legacy_audio_module_methods,
            dso : NULL,
            reserved : {0},
        },
    },
};

}; // extern "C"

}; // namespace android_audio_legacy
