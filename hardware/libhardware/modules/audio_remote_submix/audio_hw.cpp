/*
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

#define LOG_TAG "r_submix"
//#define LOG_NDEBUG 0

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/limits.h>

#include <cutils/log.h>
#include <cutils/properties.h>
#include <cutils/str_parms.h>

#include <hardware/audio.h>
#include <hardware/hardware.h>
#include <system/audio.h>

#include <media/AudioParameter.h>
#include <media/AudioBufferProvider.h>
#include <media/nbaio/MonoPipe.h>
#include <media/nbaio/MonoPipeReader.h>

#include <utils/String8.h>

#define LOG_STREAMS_TO_FILES 0
#if LOG_STREAMS_TO_FILES
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#endif // LOG_STREAMS_TO_FILES

extern "C" {

namespace android {

// Set to 1 to enable extremely verbose logging in this module.
#define SUBMIX_VERBOSE_LOGGING 0
#if SUBMIX_VERBOSE_LOGGING
#define SUBMIX_ALOGV(...) ALOGV(__VA_ARGS__)
#define SUBMIX_ALOGE(...) ALOGE(__VA_ARGS__)
#else
#define SUBMIX_ALOGV(...)
#define SUBMIX_ALOGE(...)
#endif // SUBMIX_VERBOSE_LOGGING

// NOTE: This value will be rounded up to the nearest power of 2 by MonoPipe().
#define DEFAULT_PIPE_SIZE_IN_FRAMES  (1024*8)
// Value used to divide the MonoPipe() buffer into segments that are written to the source and
// read from the sink.  The maximum latency of the device is the size of the MonoPipe's buffer
// the minimum latency is the MonoPipe buffer size divided by this value.
#define DEFAULT_PIPE_PERIOD_COUNT    4
// The duration of MAX_READ_ATTEMPTS * READ_ATTEMPT_SLEEP_MS must be stricly inferior to
//   the duration of a record buffer at the current record sample rate (of the device, not of
//   the recording itself). Here we have:
//      3 * 5ms = 15ms < 1024 frames * 1000 / 48000 = 21.333ms
#define MAX_READ_ATTEMPTS            3
#define READ_ATTEMPT_SLEEP_MS        5 // 5ms between two read attempts when pipe is empty
#define DEFAULT_SAMPLE_RATE_HZ       48000 // default sample rate
// See NBAIO_Format frameworks/av/include/media/nbaio/NBAIO.h.
#define DEFAULT_FORMAT               AUDIO_FORMAT_PCM_16_BIT
// A legacy user of this device does not close the input stream when it shuts down, which
// results in the application opening a new input stream before closing the old input stream
// handle it was previously using.  Setting this value to 1 allows multiple clients to open
// multiple input streams from this device.  If this option is enabled, each input stream returned
// is *the same stream* which means that readers will race to read data from these streams.
#define ENABLE_LEGACY_INPUT_OPEN     1
// Whether channel conversion (16-bit signed PCM mono->stereo, stereo->mono) is enabled.
#define ENABLE_CHANNEL_CONVERSION    1
// Whether resampling is enabled.
#define ENABLE_RESAMPLING            1
#if LOG_STREAMS_TO_FILES
// Folder to save stream log files to.
#define LOG_STREAM_FOLDER "/data/misc/media"
// Log filenames for input and output streams.
#define LOG_STREAM_OUT_FILENAME LOG_STREAM_FOLDER "/r_submix_out.raw"
#define LOG_STREAM_IN_FILENAME LOG_STREAM_FOLDER "/r_submix_in.raw"
// File permissions for stream log files.
#define LOG_STREAM_FILE_PERMISSIONS (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#endif // LOG_STREAMS_TO_FILES
// limit for number of read error log entries to avoid spamming the logs
#define MAX_READ_ERROR_LOGS 5

// Common limits macros.
#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif // min
#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif // max

// Set *result_variable_ptr to true if value_to_find is present in the array array_to_search,
// otherwise set *result_variable_ptr to false.
#define SUBMIX_VALUE_IN_SET(value_to_find, array_to_search, result_variable_ptr) \
    { \
        size_t i; \
        *(result_variable_ptr) = false; \
        for (i = 0; i < sizeof(array_to_search) / sizeof((array_to_search)[0]); i++) { \
          if ((value_to_find) == (array_to_search)[i]) { \
                *(result_variable_ptr) = true; \
                break; \
            } \
        } \
    }

// Configuration of the submix pipe.
struct submix_config {
    // Channel mask field in this data structure is set to either input_channel_mask or
    // output_channel_mask depending upon the last stream to be opened on this device.
    struct audio_config common;
    // Input stream and output stream channel masks.  This is required since input and output
    // channel bitfields are not equivalent.
    audio_channel_mask_t input_channel_mask;
    audio_channel_mask_t output_channel_mask;
#if ENABLE_RESAMPLING
    // Input stream and output stream sample rates.
    uint32_t input_sample_rate;
    uint32_t output_sample_rate;
#endif // ENABLE_RESAMPLING
    size_t pipe_frame_size;  // Number of bytes in each audio frame in the pipe.
    size_t buffer_size_frames; // Size of the audio pipe in frames.
    // Maximum number of frames buffered by the input and output streams.
    size_t buffer_period_size_frames;
};

#define MAX_ROUTES 10
typedef struct route_config {
    struct submix_config config;
    char address[AUDIO_DEVICE_MAX_ADDRESS_LEN];
    // Pipe variables: they handle the ring buffer that "pipes" audio:
    //  - from the submix virtual audio output == what needs to be played
    //    remotely, seen as an output for AudioFlinger
    //  - to the virtual audio source == what is captured by the component
    //    which "records" the submix / virtual audio source, and handles it as needed.
    // A usecase example is one where the component capturing the audio is then sending it over
    // Wifi for presentation on a remote Wifi Display device (e.g. a dongle attached to a TV, or a
    // TV with Wifi Display capabilities), or to a wireless audio player.
    sp<MonoPipe> rsxSink;
    sp<MonoPipeReader> rsxSource;
    // Pointers to the current input and output stream instances.  rsxSink and rsxSource are
    // destroyed if both and input and output streams are destroyed.
    struct submix_stream_out *output;
    struct submix_stream_in *input;
#if ENABLE_RESAMPLING
    // Buffer used as temporary storage for resampled data prior to returning data to the output
    // stream.
    int16_t resampler_buffer[DEFAULT_PIPE_SIZE_IN_FRAMES];
#endif // ENABLE_RESAMPLING
} route_config_t;

struct submix_audio_device {
    struct audio_hw_device device;
    route_config_t routes[MAX_ROUTES];
    // Device lock, also used to protect access to submix_audio_device from the input and output
    // streams.
    pthread_mutex_t lock;
};

struct submix_stream_out {
    struct audio_stream_out stream;
    struct submix_audio_device *dev;
    int route_handle;
    bool output_standby;
#if LOG_STREAMS_TO_FILES
    int log_fd;
#endif // LOG_STREAMS_TO_FILES
};

struct submix_stream_in {
    struct audio_stream_in stream;
    struct submix_audio_device *dev;
    int route_handle;
    bool input_standby;
    bool output_standby_rec_thr; // output standby state as seen from record thread

    // wall clock when recording starts
    struct timespec record_start_time;
    // how many frames have been requested to be read
    int64_t read_counter_frames;

#if ENABLE_LEGACY_INPUT_OPEN
    // Number of references to this input stream.
    volatile int32_t ref_count;
#endif // ENABLE_LEGACY_INPUT_OPEN
#if LOG_STREAMS_TO_FILES
    int log_fd;
#endif // LOG_STREAMS_TO_FILES

    volatile int16_t read_error_count;
};

// Determine whether the specified sample rate is supported by the submix module.
static bool sample_rate_supported(const uint32_t sample_rate)
{
    // Set of sample rates supported by Format_from_SR_C() frameworks/av/media/libnbaio/NAIO.cpp.
    static const unsigned int supported_sample_rates[] = {
        8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000,
    };
    bool return_value;
    SUBMIX_VALUE_IN_SET(sample_rate, supported_sample_rates, &return_value);
    return return_value;
}

// Determine whether the specified sample rate is supported, if it is return the specified sample
// rate, otherwise return the default sample rate for the submix module.
static uint32_t get_supported_sample_rate(uint32_t sample_rate)
{
  return sample_rate_supported(sample_rate) ? sample_rate : DEFAULT_SAMPLE_RATE_HZ;
}

// Determine whether the specified channel in mask is supported by the submix module.
static bool channel_in_mask_supported(const audio_channel_mask_t channel_in_mask)
{
    // Set of channel in masks supported by Format_from_SR_C()
    // frameworks/av/media/libnbaio/NAIO.cpp.
    static const audio_channel_mask_t supported_channel_in_masks[] = {
        AUDIO_CHANNEL_IN_MONO, AUDIO_CHANNEL_IN_STEREO,
    };
    bool return_value;
    SUBMIX_VALUE_IN_SET(channel_in_mask, supported_channel_in_masks, &return_value);
    return return_value;
}

// Determine whether the specified channel in mask is supported, if it is return the specified
// channel in mask, otherwise return the default channel in mask for the submix module.
static audio_channel_mask_t get_supported_channel_in_mask(
        const audio_channel_mask_t channel_in_mask)
{
    return channel_in_mask_supported(channel_in_mask) ? channel_in_mask :
            static_cast<audio_channel_mask_t>(AUDIO_CHANNEL_IN_STEREO);
}

// Determine whether the specified channel out mask is supported by the submix module.
static bool channel_out_mask_supported(const audio_channel_mask_t channel_out_mask)
{
    // Set of channel out masks supported by Format_from_SR_C()
    // frameworks/av/media/libnbaio/NAIO.cpp.
    static const audio_channel_mask_t supported_channel_out_masks[] = {
        AUDIO_CHANNEL_OUT_MONO, AUDIO_CHANNEL_OUT_STEREO,
    };
    bool return_value;
    SUBMIX_VALUE_IN_SET(channel_out_mask, supported_channel_out_masks, &return_value);
    return return_value;
}

// Determine whether the specified channel out mask is supported, if it is return the specified
// channel out mask, otherwise return the default channel out mask for the submix module.
static audio_channel_mask_t get_supported_channel_out_mask(
        const audio_channel_mask_t channel_out_mask)
{
    return channel_out_mask_supported(channel_out_mask) ? channel_out_mask :
        static_cast<audio_channel_mask_t>(AUDIO_CHANNEL_OUT_STEREO);
}

// Get a pointer to submix_stream_out given an audio_stream_out that is embedded within the
// structure.
static struct submix_stream_out * audio_stream_out_get_submix_stream_out(
        struct audio_stream_out * const stream)
{
    ALOG_ASSERT(stream);
    return reinterpret_cast<struct submix_stream_out *>(reinterpret_cast<uint8_t *>(stream) -
                offsetof(struct submix_stream_out, stream));
}

// Get a pointer to submix_stream_out given an audio_stream that is embedded within the structure.
static struct submix_stream_out * audio_stream_get_submix_stream_out(
        struct audio_stream * const stream)
{
    ALOG_ASSERT(stream);
    return audio_stream_out_get_submix_stream_out(
            reinterpret_cast<struct audio_stream_out *>(stream));
}

// Get a pointer to submix_stream_in given an audio_stream_in that is embedded within the
// structure.
static struct submix_stream_in * audio_stream_in_get_submix_stream_in(
        struct audio_stream_in * const stream)
{
    ALOG_ASSERT(stream);
    return reinterpret_cast<struct submix_stream_in *>(reinterpret_cast<uint8_t *>(stream) -
            offsetof(struct submix_stream_in, stream));
}

// Get a pointer to submix_stream_in given an audio_stream that is embedded within the structure.
static struct submix_stream_in * audio_stream_get_submix_stream_in(
        struct audio_stream * const stream)
{
    ALOG_ASSERT(stream);
    return audio_stream_in_get_submix_stream_in(
            reinterpret_cast<struct audio_stream_in *>(stream));
}

// Get a pointer to submix_audio_device given a pointer to an audio_device that is embedded within
// the structure.
static struct submix_audio_device * audio_hw_device_get_submix_audio_device(
        struct audio_hw_device *device)
{
    ALOG_ASSERT(device);
    return reinterpret_cast<struct submix_audio_device *>(reinterpret_cast<uint8_t *>(device) -
        offsetof(struct submix_audio_device, device));
}

// Compare an audio_config with input channel mask and an audio_config with output channel mask
// returning false if they do *not* match, true otherwise.
static bool audio_config_compare(const audio_config * const input_config,
        const audio_config * const output_config)
{
#if !ENABLE_CHANNEL_CONVERSION
    const uint32_t input_channels = audio_channel_count_from_in_mask(input_config->channel_mask);
    const uint32_t output_channels = audio_channel_count_from_out_mask(output_config->channel_mask);
    if (input_channels != output_channels) {
        ALOGE("audio_config_compare() channel count mismatch input=%d vs. output=%d",
              input_channels, output_channels);
        return false;
    }
#endif // !ENABLE_CHANNEL_CONVERSION
#if ENABLE_RESAMPLING
    if (input_config->sample_rate != output_config->sample_rate &&
            audio_channel_count_from_in_mask(input_config->channel_mask) != 1) {
#else
    if (input_config->sample_rate != output_config->sample_rate) {
#endif // ENABLE_RESAMPLING
        ALOGE("audio_config_compare() sample rate mismatch %ul vs. %ul",
              input_config->sample_rate, output_config->sample_rate);
        return false;
    }
    if (input_config->format != output_config->format) {
        ALOGE("audio_config_compare() format mismatch %x vs. %x",
              input_config->format, output_config->format);
        return false;
    }
    // This purposely ignores offload_info as it's not required for the submix device.
    return true;
}

// If one doesn't exist, create a pipe for the submix audio device rsxadev of size
// buffer_size_frames and optionally associate "in" or "out" with the submix audio device.
// Must be called with lock held on the submix_audio_device
static void submix_audio_device_create_pipe_l(struct submix_audio_device * const rsxadev,
                                            const struct audio_config * const config,
                                            const size_t buffer_size_frames,
                                            const uint32_t buffer_period_count,
                                            struct submix_stream_in * const in,
                                            struct submix_stream_out * const out,
                                            const char *address,
                                            int route_idx)
{
    ALOG_ASSERT(in || out);
    ALOG_ASSERT(route_idx > -1);
    ALOG_ASSERT(route_idx < MAX_ROUTES);
    ALOGD("submix_audio_device_create_pipe_l(addr=%s, idx=%d)", address, route_idx);

    // Save a reference to the specified input or output stream and the associated channel
    // mask.
    if (in) {
        in->route_handle = route_idx;
        rsxadev->routes[route_idx].input = in;
        rsxadev->routes[route_idx].config.input_channel_mask = config->channel_mask;
#if ENABLE_RESAMPLING
        rsxadev->routes[route_idx].config.input_sample_rate = config->sample_rate;
        // If the output isn't configured yet, set the output sample rate to the maximum supported
        // sample rate such that the smallest possible input buffer is created, and put a default
        // value for channel count
        if (!rsxadev->routes[route_idx].output) {
            rsxadev->routes[route_idx].config.output_sample_rate = 48000;
            rsxadev->routes[route_idx].config.output_channel_mask = AUDIO_CHANNEL_OUT_STEREO;
        }
#endif // ENABLE_RESAMPLING
    }
    if (out) {
        out->route_handle = route_idx;
        rsxadev->routes[route_idx].output = out;
        rsxadev->routes[route_idx].config.output_channel_mask = config->channel_mask;
#if ENABLE_RESAMPLING
        rsxadev->routes[route_idx].config.output_sample_rate = config->sample_rate;
#endif // ENABLE_RESAMPLING
    }
    // Save the address
    strncpy(rsxadev->routes[route_idx].address, address, AUDIO_DEVICE_MAX_ADDRESS_LEN);
    ALOGD("  now using address %s for route %d", rsxadev->routes[route_idx].address, route_idx);
    // If a pipe isn't associated with the device, create one.
    if (rsxadev->routes[route_idx].rsxSink == NULL || rsxadev->routes[route_idx].rsxSource == NULL)
    {
        struct submix_config * const device_config = &rsxadev->routes[route_idx].config;
        uint32_t channel_count;
        if (out)
            channel_count = audio_channel_count_from_out_mask(config->channel_mask);
        else
            channel_count = audio_channel_count_from_in_mask(config->channel_mask);
#if ENABLE_CHANNEL_CONVERSION
        // If channel conversion is enabled, allocate enough space for the maximum number of
        // possible channels stored in the pipe for the situation when the number of channels in
        // the output stream don't match the number in the input stream.
        const uint32_t pipe_channel_count = max(channel_count, 2);
#else
        const uint32_t pipe_channel_count = channel_count;
#endif // ENABLE_CHANNEL_CONVERSION
        const NBAIO_Format format = Format_from_SR_C(config->sample_rate, pipe_channel_count,
            config->format);
        const NBAIO_Format offers[1] = {format};
        size_t numCounterOffers = 0;
        // Create a MonoPipe with optional blocking set to true.
        MonoPipe* sink = new MonoPipe(buffer_size_frames, format, true /*writeCanBlock*/);
        // Negotiation between the source and sink cannot fail as the device open operation
        // creates both ends of the pipe using the same audio format.
        ssize_t index = sink->negotiate(offers, 1, NULL, numCounterOffers);
        ALOG_ASSERT(index == 0);
        MonoPipeReader* source = new MonoPipeReader(sink);
        numCounterOffers = 0;
        index = source->negotiate(offers, 1, NULL, numCounterOffers);
        ALOG_ASSERT(index == 0);
        ALOGV("submix_audio_device_create_pipe_l(): created pipe");

        // Save references to the source and sink.
        ALOG_ASSERT(rsxadev->routes[route_idx].rsxSink == NULL);
        ALOG_ASSERT(rsxadev->routes[route_idx].rsxSource == NULL);
        rsxadev->routes[route_idx].rsxSink = sink;
        rsxadev->routes[route_idx].rsxSource = source;
        // Store the sanitized audio format in the device so that it's possible to determine
        // the format of the pipe source when opening the input device.
        memcpy(&device_config->common, config, sizeof(device_config->common));
        device_config->buffer_size_frames = sink->maxFrames();
        device_config->buffer_period_size_frames = device_config->buffer_size_frames /
                buffer_period_count;
        if (in) device_config->pipe_frame_size = audio_stream_in_frame_size(&in->stream);
        if (out) device_config->pipe_frame_size = audio_stream_out_frame_size(&out->stream);
#if ENABLE_CHANNEL_CONVERSION
        // Calculate the pipe frame size based upon the number of channels.
        device_config->pipe_frame_size = (device_config->pipe_frame_size * pipe_channel_count) /
                channel_count;
#endif // ENABLE_CHANNEL_CONVERSION
        SUBMIX_ALOGV("submix_audio_device_create_pipe_l(): pipe frame size %zd, pipe size %zd, "
                     "period size %zd", device_config->pipe_frame_size,
                     device_config->buffer_size_frames, device_config->buffer_period_size_frames);
    }
}

// Release references to the sink and source.  Input and output threads may maintain references
// to these objects via StrongPointer (sp<MonoPipe> and sp<MonoPipeReader>) which they can use
// before they shutdown.
// Must be called with lock held on the submix_audio_device
static void submix_audio_device_release_pipe_l(struct submix_audio_device * const rsxadev,
        int route_idx)
{
    ALOG_ASSERT(route_idx > -1);
    ALOG_ASSERT(route_idx < MAX_ROUTES);
    ALOGD("submix_audio_device_release_pipe_l(idx=%d) addr=%s", route_idx,
            rsxadev->routes[route_idx].address);
    if (rsxadev->routes[route_idx].rsxSink != 0) {
        rsxadev->routes[route_idx].rsxSink.clear();
        rsxadev->routes[route_idx].rsxSink = 0;
    }
    if (rsxadev->routes[route_idx].rsxSource != 0) {
        rsxadev->routes[route_idx].rsxSource.clear();
        rsxadev->routes[route_idx].rsxSource = 0;
    }
    memset(rsxadev->routes[route_idx].address, 0, AUDIO_DEVICE_MAX_ADDRESS_LEN);
#ifdef ENABLE_RESAMPLING
    memset(rsxadev->routes[route_idx].resampler_buffer, 0,
            sizeof(int16_t) * DEFAULT_PIPE_SIZE_IN_FRAMES);
#endif
}

// Remove references to the specified input and output streams.  When the device no longer
// references input and output streams destroy the associated pipe.
// Must be called with lock held on the submix_audio_device
static void submix_audio_device_destroy_pipe_l(struct submix_audio_device * const rsxadev,
                                             const struct submix_stream_in * const in,
                                             const struct submix_stream_out * const out)
{
    MonoPipe* sink;
    ALOGV("submix_audio_device_destroy_pipe_l()");
    int route_idx = -1;
    if (in != NULL) {
#if ENABLE_LEGACY_INPUT_OPEN
        const_cast<struct submix_stream_in*>(in)->ref_count--;
        route_idx = in->route_handle;
        ALOG_ASSERT(rsxadev->routes[route_idx].input == in);
        if (in->ref_count == 0) {
            rsxadev->routes[route_idx].input = NULL;
        }
        ALOGV("submix_audio_device_destroy_pipe_l(): input ref_count %d", in->ref_count);
#else
        rsxadev->input = NULL;
#endif // ENABLE_LEGACY_INPUT_OPEN
    }
    if (out != NULL) {
        route_idx = out->route_handle;
        ALOG_ASSERT(rsxadev->routes[route_idx].output == out);
        rsxadev->routes[route_idx].output = NULL;
    }
    if (route_idx != -1 &&
            rsxadev->routes[route_idx].input == NULL && rsxadev->routes[route_idx].output == NULL) {
        submix_audio_device_release_pipe_l(rsxadev, route_idx);
        ALOGD("submix_audio_device_destroy_pipe_l(): pipe destroyed");
    }
}

// Sanitize the user specified audio config for a submix input / output stream.
static void submix_sanitize_config(struct audio_config * const config, const bool is_input_format)
{
    config->channel_mask = is_input_format ? get_supported_channel_in_mask(config->channel_mask) :
            get_supported_channel_out_mask(config->channel_mask);
    config->sample_rate = get_supported_sample_rate(config->sample_rate);
    config->format = DEFAULT_FORMAT;
}

// Verify a submix input or output stream can be opened.
// Must be called with lock held on the submix_audio_device
static bool submix_open_validate_l(const struct submix_audio_device * const rsxadev,
                                 int route_idx,
                                 const struct audio_config * const config,
                                 const bool opening_input)
{
    bool input_open;
    bool output_open;
    audio_config pipe_config;

    // Query the device for the current audio config and whether input and output streams are open.
    output_open = rsxadev->routes[route_idx].output != NULL;
    input_open = rsxadev->routes[route_idx].input != NULL;
    memcpy(&pipe_config, &rsxadev->routes[route_idx].config.common, sizeof(pipe_config));

    // If the stream is already open, don't open it again.
    if (opening_input ? !ENABLE_LEGACY_INPUT_OPEN && input_open : output_open) {
        ALOGE("submix_open_validate_l(): %s stream already open.", opening_input ? "Input" :
                "Output");
        return false;
    }

    SUBMIX_ALOGV("submix_open_validate_l(): sample rate=%d format=%x "
                 "%s_channel_mask=%x", config->sample_rate, config->format,
                 opening_input ? "in" : "out", config->channel_mask);

    // If either stream is open, verify the existing audio config the pipe matches the user
    // specified config.
    if (input_open || output_open) {
        const audio_config * const input_config = opening_input ? config : &pipe_config;
        const audio_config * const output_config = opening_input ? &pipe_config : config;
        // Get the channel mask of the open device.
        pipe_config.channel_mask =
            opening_input ? rsxadev->routes[route_idx].config.output_channel_mask :
                rsxadev->routes[route_idx].config.input_channel_mask;
        if (!audio_config_compare(input_config, output_config)) {
            ALOGE("submix_open_validate_l(): Unsupported format.");
            return false;
        }
    }
    return true;
}

// Must be called with lock held on the submix_audio_device
static status_t submix_get_route_idx_for_address_l(const struct submix_audio_device * const rsxadev,
                                                 const char* address, /*in*/
                                                 int *idx /*out*/)
{
    // Do we already have a route for this address
    int route_idx = -1;
    int route_empty_idx = -1; // index of an empty route slot that can be used if needed
    for (int i=0 ; i < MAX_ROUTES ; i++) {
        if (strcmp(rsxadev->routes[i].address, "") == 0) {
            route_empty_idx = i;
        }
        if (strncmp(rsxadev->routes[i].address, address, AUDIO_DEVICE_MAX_ADDRESS_LEN) == 0) {
            route_idx = i;
            break;
        }
    }

    if ((route_idx == -1) && (route_empty_idx == -1)) {
        ALOGE("Cannot create new route for address %s, max number of routes reached", address);
        return -ENOMEM;
    }
    if (route_idx == -1) {
        route_idx = route_empty_idx;
    }
    *idx = route_idx;
    return OK;
}


// Calculate the maximum size of the pipe buffer in frames for the specified stream.
static size_t calculate_stream_pipe_size_in_frames(const struct audio_stream *stream,
                                                   const struct submix_config *config,
                                                   const size_t pipe_frames,
                                                   const size_t stream_frame_size)
{
    const size_t pipe_frame_size = config->pipe_frame_size;
    const size_t max_frame_size = max(stream_frame_size, pipe_frame_size);
    return (pipe_frames * config->pipe_frame_size) / max_frame_size;
}

/* audio HAL functions */

static uint32_t out_get_sample_rate(const struct audio_stream *stream)
{
    const struct submix_stream_out * const out = audio_stream_get_submix_stream_out(
            const_cast<struct audio_stream *>(stream));
#if ENABLE_RESAMPLING
    const uint32_t out_rate = out->dev->routes[out->route_handle].config.output_sample_rate;
#else
    const uint32_t out_rate = out->dev->routes[out->route_handle].config.common.sample_rate;
#endif // ENABLE_RESAMPLING
    SUBMIX_ALOGV("out_get_sample_rate() returns %u for addr %s",
            out_rate, out->dev->routes[out->route_handle].address);
    return out_rate;
}

static int out_set_sample_rate(struct audio_stream *stream, uint32_t rate)
{
    struct submix_stream_out * const out = audio_stream_get_submix_stream_out(stream);
#if ENABLE_RESAMPLING
    // The sample rate of the stream can't be changed once it's set since this would change the
    // output buffer size and hence break playback to the shared pipe.
    if (rate != out->dev->routes[out->route_handle].config.output_sample_rate) {
        ALOGE("out_set_sample_rate() resampling enabled can't change sample rate from "
              "%u to %u for addr %s",
              out->dev->routes[out->route_handle].config.output_sample_rate, rate,
              out->dev->routes[out->route_handle].address);
        return -ENOSYS;
    }
#endif // ENABLE_RESAMPLING
    if (!sample_rate_supported(rate)) {
        ALOGE("out_set_sample_rate(rate=%u) rate unsupported", rate);
        return -ENOSYS;
    }
    SUBMIX_ALOGV("out_set_sample_rate(rate=%u)", rate);
    out->dev->routes[out->route_handle].config.common.sample_rate = rate;
    return 0;
}

static size_t out_get_buffer_size(const struct audio_stream *stream)
{
    const struct submix_stream_out * const out = audio_stream_get_submix_stream_out(
            const_cast<struct audio_stream *>(stream));
    const struct submix_config * const config = &out->dev->routes[out->route_handle].config;
    const size_t stream_frame_size =
                            audio_stream_out_frame_size((const struct audio_stream_out *)stream);
    const size_t buffer_size_frames = calculate_stream_pipe_size_in_frames(
        stream, config, config->buffer_period_size_frames, stream_frame_size);
    const size_t buffer_size_bytes = buffer_size_frames * stream_frame_size;
    SUBMIX_ALOGV("out_get_buffer_size() returns %zu bytes, %zu frames",
                 buffer_size_bytes, buffer_size_frames);
    return buffer_size_bytes;
}

static audio_channel_mask_t out_get_channels(const struct audio_stream *stream)
{
    const struct submix_stream_out * const out = audio_stream_get_submix_stream_out(
            const_cast<struct audio_stream *>(stream));
    uint32_t channel_mask = out->dev->routes[out->route_handle].config.output_channel_mask;
    SUBMIX_ALOGV("out_get_channels() returns %08x", channel_mask);
    return channel_mask;
}

static audio_format_t out_get_format(const struct audio_stream *stream)
{
    const struct submix_stream_out * const out = audio_stream_get_submix_stream_out(
            const_cast<struct audio_stream *>(stream));
    const audio_format_t format = out->dev->routes[out->route_handle].config.common.format;
    SUBMIX_ALOGV("out_get_format() returns %x", format);
    return format;
}

static int out_set_format(struct audio_stream *stream, audio_format_t format)
{
    const struct submix_stream_out * const out = audio_stream_get_submix_stream_out(stream);
    if (format != out->dev->routes[out->route_handle].config.common.format) {
        ALOGE("out_set_format(format=%x) format unsupported", format);
        return -ENOSYS;
    }
    SUBMIX_ALOGV("out_set_format(format=%x)", format);
    return 0;
}

static int out_standby(struct audio_stream *stream)
{
    ALOGI("out_standby()");
    struct submix_stream_out * const out = audio_stream_get_submix_stream_out(stream);
    struct submix_audio_device * const rsxadev = out->dev;

    pthread_mutex_lock(&rsxadev->lock);

    out->output_standby = true;

    pthread_mutex_unlock(&rsxadev->lock);

    return 0;
}

static int out_dump(const struct audio_stream *stream, int fd)
{
    (void)stream;
    (void)fd;
    return 0;
}

static int out_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    int exiting = -1;
    AudioParameter parms = AudioParameter(String8(kvpairs));
    SUBMIX_ALOGV("out_set_parameters() kvpairs='%s'", kvpairs);

    // FIXME this is using hard-coded strings but in the future, this functionality will be
    //       converted to use audio HAL extensions required to support tunneling
    if ((parms.getInt(String8("exiting"), exiting) == NO_ERROR) && (exiting > 0)) {
        struct submix_audio_device * const rsxadev =
                audio_stream_get_submix_stream_out(stream)->dev;
        pthread_mutex_lock(&rsxadev->lock);
        { // using the sink
            sp<MonoPipe> sink =
                    rsxadev->routes[audio_stream_get_submix_stream_out(stream)->route_handle]
                                    .rsxSink;
            if (sink == NULL) {
                pthread_mutex_unlock(&rsxadev->lock);
                return 0;
            }

            ALOGD("out_set_parameters(): shutting down MonoPipe sink");
            sink->shutdown(true);
        } // done using the sink
        pthread_mutex_unlock(&rsxadev->lock);
    }
    return 0;
}

static char * out_get_parameters(const struct audio_stream *stream, const char *keys)
{
    (void)stream;
    (void)keys;
    return strdup("");
}

static uint32_t out_get_latency(const struct audio_stream_out *stream)
{
    const struct submix_stream_out * const out = audio_stream_out_get_submix_stream_out(
            const_cast<struct audio_stream_out *>(stream));
    const struct submix_config * const config = &out->dev->routes[out->route_handle].config;
    const size_t stream_frame_size =
                            audio_stream_out_frame_size(stream);
    const size_t buffer_size_frames = calculate_stream_pipe_size_in_frames(
            &stream->common, config, config->buffer_size_frames, stream_frame_size);
    const uint32_t sample_rate = out_get_sample_rate(&stream->common);
    const uint32_t latency_ms = (buffer_size_frames * 1000) / sample_rate;
    SUBMIX_ALOGV("out_get_latency() returns %u ms, size in frames %zu, sample rate %u",
                 latency_ms, buffer_size_frames, sample_rate);
    return latency_ms;
}

static int out_set_volume(struct audio_stream_out *stream, float left,
                          float right)
{
    (void)stream;
    (void)left;
    (void)right;
    return -ENOSYS;
}

static ssize_t out_write(struct audio_stream_out *stream, const void* buffer,
                         size_t bytes)
{
    SUBMIX_ALOGV("out_write(bytes=%zd)", bytes);
    ssize_t written_frames = 0;
    const size_t frame_size = audio_stream_out_frame_size(stream);
    struct submix_stream_out * const out = audio_stream_out_get_submix_stream_out(stream);
    struct submix_audio_device * const rsxadev = out->dev;
    const size_t frames = bytes / frame_size;

    pthread_mutex_lock(&rsxadev->lock);

    out->output_standby = false;

    sp<MonoPipe> sink = rsxadev->routes[out->route_handle].rsxSink;
    if (sink != NULL) {
        if (sink->isShutdown()) {
            sink.clear();
            pthread_mutex_unlock(&rsxadev->lock);
            SUBMIX_ALOGV("out_write(): pipe shutdown, ignoring the write.");
            // the pipe has already been shutdown, this buffer will be lost but we must
            //   simulate timing so we don't drain the output faster than realtime
            usleep(frames * 1000000 / out_get_sample_rate(&stream->common));
            return bytes;
        }
    } else {
        pthread_mutex_unlock(&rsxadev->lock);
        ALOGE("out_write without a pipe!");
        ALOG_ASSERT("out_write without a pipe!");
        return 0;
    }

    // If the write to the sink would block when no input stream is present, flush enough frames
    // from the pipe to make space to write the most recent data.
    {
        const size_t availableToWrite = sink->availableToWrite();
        sp<MonoPipeReader> source = rsxadev->routes[out->route_handle].rsxSource;
        if (rsxadev->routes[out->route_handle].input == NULL && availableToWrite < frames) {
            static uint8_t flush_buffer[64];
            const size_t flushBufferSizeFrames = sizeof(flush_buffer) / frame_size;
            size_t frames_to_flush_from_source = frames - availableToWrite;
            SUBMIX_ALOGV("out_write(): flushing %d frames from the pipe to avoid blocking",
                         frames_to_flush_from_source);
            while (frames_to_flush_from_source) {
                const size_t flush_size = min(frames_to_flush_from_source, flushBufferSizeFrames);
                frames_to_flush_from_source -= flush_size;
                // read does not block
                source->read(flush_buffer, flush_size, AudioBufferProvider::kInvalidPTS);
            }
        }
    }

    pthread_mutex_unlock(&rsxadev->lock);

    written_frames = sink->write(buffer, frames);

#if LOG_STREAMS_TO_FILES
    if (out->log_fd >= 0) write(out->log_fd, buffer, written_frames * frame_size);
#endif // LOG_STREAMS_TO_FILES

    if (written_frames < 0) {
        if (written_frames == (ssize_t)NEGOTIATE) {
            ALOGE("out_write() write to pipe returned NEGOTIATE");

            pthread_mutex_lock(&rsxadev->lock);
            sink.clear();
            pthread_mutex_unlock(&rsxadev->lock);

            written_frames = 0;
            return 0;
        } else {
            // write() returned UNDERRUN or WOULD_BLOCK, retry
            ALOGE("out_write() write to pipe returned unexpected %zd", written_frames);
            written_frames = sink->write(buffer, frames);
        }
    }

    pthread_mutex_lock(&rsxadev->lock);
    sink.clear();
    pthread_mutex_unlock(&rsxadev->lock);

    if (written_frames < 0) {
        ALOGE("out_write() failed writing to pipe with %zd", written_frames);
        return 0;
    }
    const ssize_t written_bytes = written_frames * frame_size;
    SUBMIX_ALOGV("out_write() wrote %zd bytes %zd frames", written_bytes, written_frames);
    return written_bytes;
}

static int out_get_render_position(const struct audio_stream_out *stream,
                                   uint32_t *dsp_frames)
{
    (void)stream;
    (void)dsp_frames;
    return -EINVAL;
}

static int out_add_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    (void)stream;
    (void)effect;
    return 0;
}

static int out_remove_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    (void)stream;
    (void)effect;
    return 0;
}

static int out_get_next_write_timestamp(const struct audio_stream_out *stream,
                                        int64_t *timestamp)
{
    (void)stream;
    (void)timestamp;
    return -EINVAL;
}

/** audio_stream_in implementation **/
static uint32_t in_get_sample_rate(const struct audio_stream *stream)
{
    const struct submix_stream_in * const in = audio_stream_get_submix_stream_in(
        const_cast<struct audio_stream*>(stream));
#if ENABLE_RESAMPLING
    const uint32_t rate = in->dev->routes[in->route_handle].config.input_sample_rate;
#else
    const uint32_t rate = in->dev->routes[in->route_handle].config.common.sample_rate;
#endif // ENABLE_RESAMPLING
    SUBMIX_ALOGV("in_get_sample_rate() returns %u", rate);
    return rate;
}

static int in_set_sample_rate(struct audio_stream *stream, uint32_t rate)
{
    const struct submix_stream_in * const in = audio_stream_get_submix_stream_in(stream);
#if ENABLE_RESAMPLING
    // The sample rate of the stream can't be changed once it's set since this would change the
    // input buffer size and hence break recording from the shared pipe.
    if (rate != in->dev->routes[in->route_handle].config.input_sample_rate) {
        ALOGE("in_set_sample_rate() resampling enabled can't change sample rate from "
              "%u to %u", in->dev->routes[in->route_handle].config.input_sample_rate, rate);
        return -ENOSYS;
    }
#endif // ENABLE_RESAMPLING
    if (!sample_rate_supported(rate)) {
        ALOGE("in_set_sample_rate(rate=%u) rate unsupported", rate);
        return -ENOSYS;
    }
    in->dev->routes[in->route_handle].config.common.sample_rate = rate;
    SUBMIX_ALOGV("in_set_sample_rate() set %u", rate);
    return 0;
}

static size_t in_get_buffer_size(const struct audio_stream *stream)
{
    const struct submix_stream_in * const in = audio_stream_get_submix_stream_in(
            const_cast<struct audio_stream*>(stream));
    const struct submix_config * const config = &in->dev->routes[in->route_handle].config;
    const size_t stream_frame_size =
                            audio_stream_in_frame_size((const struct audio_stream_in *)stream);
    size_t buffer_size_frames = calculate_stream_pipe_size_in_frames(
        stream, config, config->buffer_period_size_frames, stream_frame_size);
#if ENABLE_RESAMPLING
    // Scale the size of the buffer based upon the maximum number of frames that could be returned
    // given the ratio of output to input sample rate.
    buffer_size_frames = (size_t)(((float)buffer_size_frames *
                                   (float)config->input_sample_rate) /
                                  (float)config->output_sample_rate);
#endif // ENABLE_RESAMPLING
    const size_t buffer_size_bytes = buffer_size_frames * stream_frame_size;
    SUBMIX_ALOGV("in_get_buffer_size() returns %zu bytes, %zu frames", buffer_size_bytes,
                 buffer_size_frames);
    return buffer_size_bytes;
}

static audio_channel_mask_t in_get_channels(const struct audio_stream *stream)
{
    const struct submix_stream_in * const in = audio_stream_get_submix_stream_in(
            const_cast<struct audio_stream*>(stream));
    const audio_channel_mask_t channel_mask =
            in->dev->routes[in->route_handle].config.input_channel_mask;
    SUBMIX_ALOGV("in_get_channels() returns %x", channel_mask);
    return channel_mask;
}

static audio_format_t in_get_format(const struct audio_stream *stream)
{
    const struct submix_stream_in * const in = audio_stream_get_submix_stream_in(
            const_cast<struct audio_stream*>(stream));
    const audio_format_t format = in->dev->routes[in->route_handle].config.common.format;
    SUBMIX_ALOGV("in_get_format() returns %x", format);
    return format;
}

static int in_set_format(struct audio_stream *stream, audio_format_t format)
{
    const struct submix_stream_in * const in = audio_stream_get_submix_stream_in(stream);
    if (format != in->dev->routes[in->route_handle].config.common.format) {
        ALOGE("in_set_format(format=%x) format unsupported", format);
        return -ENOSYS;
    }
    SUBMIX_ALOGV("in_set_format(format=%x)", format);
    return 0;
}

static int in_standby(struct audio_stream *stream)
{
    ALOGI("in_standby()");
    struct submix_stream_in * const in = audio_stream_get_submix_stream_in(stream);
    struct submix_audio_device * const rsxadev = in->dev;

    pthread_mutex_lock(&rsxadev->lock);

    in->input_standby = true;

    pthread_mutex_unlock(&rsxadev->lock);

    return 0;
}

static int in_dump(const struct audio_stream *stream, int fd)
{
    (void)stream;
    (void)fd;
    return 0;
}

static int in_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    (void)stream;
    (void)kvpairs;
    return 0;
}

static char * in_get_parameters(const struct audio_stream *stream,
                                const char *keys)
{
    (void)stream;
    (void)keys;
    return strdup("");
}

static int in_set_gain(struct audio_stream_in *stream, float gain)
{
    (void)stream;
    (void)gain;
    return 0;
}

static ssize_t in_read(struct audio_stream_in *stream, void* buffer,
                       size_t bytes)
{
    struct submix_stream_in * const in = audio_stream_in_get_submix_stream_in(stream);
    struct submix_audio_device * const rsxadev = in->dev;
    struct audio_config *format;
    const size_t frame_size = audio_stream_in_frame_size(stream);
    const size_t frames_to_read = bytes / frame_size;

    SUBMIX_ALOGV("in_read bytes=%zu", bytes);
    pthread_mutex_lock(&rsxadev->lock);

    const bool output_standby = rsxadev->routes[in->route_handle].output == NULL
            ? true : rsxadev->routes[in->route_handle].output->output_standby;
    const bool output_standby_transition = (in->output_standby_rec_thr != output_standby);
    in->output_standby_rec_thr = output_standby;

    if (in->input_standby || output_standby_transition) {
        in->input_standby = false;
        // keep track of when we exit input standby (== first read == start "real recording")
        // or when we start recording silence, and reset projected time
        int rc = clock_gettime(CLOCK_MONOTONIC, &in->record_start_time);
        if (rc == 0) {
            in->read_counter_frames = 0;
        }
    }

    in->read_counter_frames += frames_to_read;
    size_t remaining_frames = frames_to_read;

    {
        // about to read from audio source
        sp<MonoPipeReader> source = rsxadev->routes[in->route_handle].rsxSource;
        if (source == NULL) {
            in->read_error_count++;// ok if it rolls over
            ALOGE_IF(in->read_error_count < MAX_READ_ERROR_LOGS,
                    "no audio pipe yet we're trying to read! (not all errors will be logged)");
            pthread_mutex_unlock(&rsxadev->lock);
            usleep(frames_to_read * 1000000 / in_get_sample_rate(&stream->common));
            memset(buffer, 0, bytes);
            return bytes;
        }

        pthread_mutex_unlock(&rsxadev->lock);

        // read the data from the pipe (it's non blocking)
        int attempts = 0;
        char* buff = (char*)buffer;
#if ENABLE_CHANNEL_CONVERSION
        // Determine whether channel conversion is required.
        const uint32_t input_channels = audio_channel_count_from_in_mask(
            rsxadev->routes[in->route_handle].config.input_channel_mask);
        const uint32_t output_channels = audio_channel_count_from_out_mask(
            rsxadev->routes[in->route_handle].config.output_channel_mask);
        if (input_channels != output_channels) {
            SUBMIX_ALOGV("in_read(): %d output channels will be converted to %d "
                         "input channels", output_channels, input_channels);
            // Only support 16-bit PCM channel conversion from mono to stereo or stereo to mono.
            ALOG_ASSERT(rsxadev->routes[in->route_handle].config.common.format ==
                    AUDIO_FORMAT_PCM_16_BIT);
            ALOG_ASSERT((input_channels == 1 && output_channels == 2) ||
                        (input_channels == 2 && output_channels == 1));
        }
#endif // ENABLE_CHANNEL_CONVERSION

#if ENABLE_RESAMPLING
        const uint32_t input_sample_rate = in_get_sample_rate(&stream->common);
        const uint32_t output_sample_rate =
                rsxadev->routes[in->route_handle].config.output_sample_rate;
        const size_t resampler_buffer_size_frames =
            sizeof(rsxadev->routes[in->route_handle].resampler_buffer) /
                sizeof(rsxadev->routes[in->route_handle].resampler_buffer[0]);
        float resampler_ratio = 1.0f;
        // Determine whether resampling is required.
        if (input_sample_rate != output_sample_rate) {
            resampler_ratio = (float)output_sample_rate / (float)input_sample_rate;
            // Only support 16-bit PCM mono resampling.
            // NOTE: Resampling is performed after the channel conversion step.
            ALOG_ASSERT(rsxadev->routes[in->route_handle].config.common.format ==
                    AUDIO_FORMAT_PCM_16_BIT);
            ALOG_ASSERT(audio_channel_count_from_in_mask(
                    rsxadev->routes[in->route_handle].config.input_channel_mask) == 1);
        }
#endif // ENABLE_RESAMPLING

        while ((remaining_frames > 0) && (attempts < MAX_READ_ATTEMPTS)) {
            ssize_t frames_read = -1977;
            size_t read_frames = remaining_frames;
#if ENABLE_RESAMPLING
            char* const saved_buff = buff;
            if (resampler_ratio != 1.0f) {
                // Calculate the number of frames from the pipe that need to be read to generate
                // the data for the input stream read.
                const size_t frames_required_for_resampler = (size_t)(
                    (float)read_frames * (float)resampler_ratio);
                read_frames = min(frames_required_for_resampler, resampler_buffer_size_frames);
                // Read into the resampler buffer.
                buff = (char*)rsxadev->routes[in->route_handle].resampler_buffer;
            }
#endif // ENABLE_RESAMPLING
#if ENABLE_CHANNEL_CONVERSION
            if (output_channels == 1 && input_channels == 2) {
                // Need to read half the requested frames since the converted output
                // data will take twice the space (mono->stereo).
                read_frames /= 2;
            }
#endif // ENABLE_CHANNEL_CONVERSION

            SUBMIX_ALOGV("in_read(): frames available to read %zd", source->availableToRead());

            frames_read = source->read(buff, read_frames, AudioBufferProvider::kInvalidPTS);

            SUBMIX_ALOGV("in_read(): frames read %zd", frames_read);

#if ENABLE_CHANNEL_CONVERSION
            // Perform in-place channel conversion.
            // NOTE: In the following "input stream" refers to the data returned by this function
            // and "output stream" refers to the data read from the pipe.
            if (input_channels != output_channels && frames_read > 0) {
                int16_t *data = (int16_t*)buff;
                if (output_channels == 2 && input_channels == 1) {
                    // Offset into the output stream data in samples.
                    ssize_t output_stream_offset = 0;
                    for (ssize_t input_stream_frame = 0; input_stream_frame < frames_read;
                         input_stream_frame++, output_stream_offset += 2) {
                        // Average the content from both channels.
                        data[input_stream_frame] = ((int32_t)data[output_stream_offset] +
                                                    (int32_t)data[output_stream_offset + 1]) / 2;
                    }
                } else if (output_channels == 1 && input_channels == 2) {
                    // Offset into the input stream data in samples.
                    ssize_t input_stream_offset = (frames_read - 1) * 2;
                    for (ssize_t output_stream_frame = frames_read - 1; output_stream_frame >= 0;
                         output_stream_frame--, input_stream_offset -= 2) {
                        const short sample = data[output_stream_frame];
                        data[input_stream_offset] = sample;
                        data[input_stream_offset + 1] = sample;
                    }
                }
            }
#endif // ENABLE_CHANNEL_CONVERSION

#if ENABLE_RESAMPLING
            if (resampler_ratio != 1.0f) {
                SUBMIX_ALOGV("in_read(): resampling %zd frames", frames_read);
                const int16_t * const data = (int16_t*)buff;
                int16_t * const resampled_buffer = (int16_t*)saved_buff;
                // Resample with *no* filtering - if the data from the ouptut stream was really
                // sampled at a different rate this will result in very nasty aliasing.
                const float output_stream_frames = (float)frames_read;
                size_t input_stream_frame = 0;
                for (float output_stream_frame = 0.0f;
                     output_stream_frame < output_stream_frames &&
                     input_stream_frame < remaining_frames;
                     output_stream_frame += resampler_ratio, input_stream_frame++) {
                    resampled_buffer[input_stream_frame] = data[(size_t)output_stream_frame];
                }
                ALOG_ASSERT(input_stream_frame <= (ssize_t)resampler_buffer_size_frames);
                SUBMIX_ALOGV("in_read(): resampler produced %zd frames", input_stream_frame);
                frames_read = input_stream_frame;
                buff = saved_buff;
            }
#endif // ENABLE_RESAMPLING

            if (frames_read > 0) {
#if LOG_STREAMS_TO_FILES
                if (in->log_fd >= 0) write(in->log_fd, buff, frames_read * frame_size);
#endif // LOG_STREAMS_TO_FILES

                remaining_frames -= frames_read;
                buff += frames_read * frame_size;
                SUBMIX_ALOGV("  in_read (att=%d) got %zd frames, remaining=%zu",
                             attempts, frames_read, remaining_frames);
            } else {
                attempts++;
                SUBMIX_ALOGE("  in_read read returned %zd", frames_read);
                usleep(READ_ATTEMPT_SLEEP_MS * 1000);
            }
        }
        // done using the source
        pthread_mutex_lock(&rsxadev->lock);
        source.clear();
        pthread_mutex_unlock(&rsxadev->lock);
    }

    if (remaining_frames > 0) {
        const size_t remaining_bytes = remaining_frames * frame_size;
        SUBMIX_ALOGV("  clearing remaining_frames = %zu", remaining_frames);
        memset(((char*)buffer)+ bytes - remaining_bytes, 0, remaining_bytes);
    }

    // compute how much we need to sleep after reading the data by comparing the wall clock with
    //   the projected time at which we should return.
    struct timespec time_after_read;// wall clock after reading from the pipe
    struct timespec record_duration;// observed record duration
    int rc = clock_gettime(CLOCK_MONOTONIC, &time_after_read);
    const uint32_t sample_rate = in_get_sample_rate(&stream->common);
    if (rc == 0) {
        // for how long have we been recording?
        record_duration.tv_sec  = time_after_read.tv_sec - in->record_start_time.tv_sec;
        record_duration.tv_nsec = time_after_read.tv_nsec - in->record_start_time.tv_nsec;
        if (record_duration.tv_nsec < 0) {
            record_duration.tv_sec--;
            record_duration.tv_nsec += 1000000000;
        }

        // read_counter_frames contains the number of frames that have been read since the
        // beginning of recording (including this call): it's converted to usec and compared to
        // how long we've been recording for, which gives us how long we must wait to sync the
        // projected recording time, and the observed recording time.
        long projected_vs_observed_offset_us =
                ((int64_t)(in->read_counter_frames
                            - (record_duration.tv_sec*sample_rate)))
                        * 1000000 / sample_rate
                - (record_duration.tv_nsec / 1000);

        SUBMIX_ALOGV("  record duration %5lds %3ldms, will wait: %7ldus",
                record_duration.tv_sec, record_duration.tv_nsec/1000000,
                projected_vs_observed_offset_us);
        if (projected_vs_observed_offset_us > 0) {
            usleep(projected_vs_observed_offset_us);
        }
    }

    SUBMIX_ALOGV("in_read returns %zu", bytes);
    return bytes;

}

static uint32_t in_get_input_frames_lost(struct audio_stream_in *stream)
{
    (void)stream;
    return 0;
}

static int in_add_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    (void)stream;
    (void)effect;
    return 0;
}

static int in_remove_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    (void)stream;
    (void)effect;
    return 0;
}

static int adev_open_output_stream(struct audio_hw_device *dev,
                                   audio_io_handle_t handle,
                                   audio_devices_t devices,
                                   audio_output_flags_t flags,
                                   struct audio_config *config,
                                   struct audio_stream_out **stream_out,
                                   const char *address)
{
    struct submix_audio_device * const rsxadev = audio_hw_device_get_submix_audio_device(dev);
    ALOGD("adev_open_output_stream(address=%s)", address);
    struct submix_stream_out *out;
    bool force_pipe_creation = false;
    (void)handle;
    (void)devices;
    (void)flags;

    *stream_out = NULL;

    // Make sure it's possible to open the device given the current audio config.
    submix_sanitize_config(config, false);

    int route_idx = -1;

    pthread_mutex_lock(&rsxadev->lock);

    status_t res = submix_get_route_idx_for_address_l(rsxadev, address, &route_idx);
    if (res != OK) {
        ALOGE("Error %d looking for address=%s in adev_open_output_stream", res, address);
        pthread_mutex_unlock(&rsxadev->lock);
        return res;
    }

    if (!submix_open_validate_l(rsxadev, route_idx, config, false)) {
        ALOGE("adev_open_output_stream(): Unable to open output stream for address %s", address);
        pthread_mutex_unlock(&rsxadev->lock);
        return -EINVAL;
    }

    out = (struct submix_stream_out *)calloc(1, sizeof(struct submix_stream_out));
    if (!out) {
        pthread_mutex_unlock(&rsxadev->lock);
        return -ENOMEM;
    }

    // Initialize the function pointer tables (v-tables).
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

#if ENABLE_RESAMPLING
    // Recreate the pipe with the correct sample rate so that MonoPipe.write() rate limits
    // writes correctly.
    force_pipe_creation = rsxadev->routes[route_idx].config.common.sample_rate
            != config->sample_rate;
#endif // ENABLE_RESAMPLING

    // If the sink has been shutdown or pipe recreation is forced (see above), delete the pipe so
    // that it's recreated.
    if ((rsxadev->routes[route_idx].rsxSink != NULL
            && rsxadev->routes[route_idx].rsxSink->isShutdown()) || force_pipe_creation) {
        submix_audio_device_release_pipe_l(rsxadev, route_idx);
    }

    // Store a pointer to the device from the output stream.
    out->dev = rsxadev;
    // Initialize the pipe.
    ALOGV("adev_open_output_stream(): about to create pipe at index %d", route_idx);
    submix_audio_device_create_pipe_l(rsxadev, config, DEFAULT_PIPE_SIZE_IN_FRAMES,
            DEFAULT_PIPE_PERIOD_COUNT, NULL, out, address, route_idx);
#if LOG_STREAMS_TO_FILES
    out->log_fd = open(LOG_STREAM_OUT_FILENAME, O_CREAT | O_TRUNC | O_WRONLY,
                       LOG_STREAM_FILE_PERMISSIONS);
    ALOGE_IF(out->log_fd < 0, "adev_open_output_stream(): log file open failed %s",
             strerror(errno));
    ALOGV("adev_open_output_stream(): log_fd = %d", out->log_fd);
#endif // LOG_STREAMS_TO_FILES
    // Return the output stream.
    *stream_out = &out->stream;

    pthread_mutex_unlock(&rsxadev->lock);
    return 0;
}

static void adev_close_output_stream(struct audio_hw_device *dev,
                                     struct audio_stream_out *stream)
{
    struct submix_audio_device * rsxadev = audio_hw_device_get_submix_audio_device(
                    const_cast<struct audio_hw_device*>(dev));
    struct submix_stream_out * const out = audio_stream_out_get_submix_stream_out(stream);

    pthread_mutex_lock(&rsxadev->lock);
    ALOGD("adev_close_output_stream() addr = %s", rsxadev->routes[out->route_handle].address);
    submix_audio_device_destroy_pipe_l(audio_hw_device_get_submix_audio_device(dev), NULL, out);
#if LOG_STREAMS_TO_FILES
    if (out->log_fd >= 0) close(out->log_fd);
#endif // LOG_STREAMS_TO_FILES

    pthread_mutex_unlock(&rsxadev->lock);
    free(out);
}

static int adev_set_parameters(struct audio_hw_device *dev, const char *kvpairs)
{
    (void)dev;
    (void)kvpairs;
    return -ENOSYS;
}

static char * adev_get_parameters(const struct audio_hw_device *dev,
                                  const char *keys)
{
    (void)dev;
    (void)keys;
    return strdup("");;
}

static int adev_init_check(const struct audio_hw_device *dev)
{
    ALOGI("adev_init_check()");
    (void)dev;
    return 0;
}

static int adev_set_voice_volume(struct audio_hw_device *dev, float volume)
{
    (void)dev;
    (void)volume;
    return -ENOSYS;
}

static int adev_set_master_volume(struct audio_hw_device *dev, float volume)
{
    (void)dev;
    (void)volume;
    return -ENOSYS;
}

static int adev_get_master_volume(struct audio_hw_device *dev, float *volume)
{
    (void)dev;
    (void)volume;
    return -ENOSYS;
}

static int adev_set_master_mute(struct audio_hw_device *dev, bool muted)
{
    (void)dev;
    (void)muted;
    return -ENOSYS;
}

static int adev_get_master_mute(struct audio_hw_device *dev, bool *muted)
{
    (void)dev;
    (void)muted;
    return -ENOSYS;
}

static int adev_set_mode(struct audio_hw_device *dev, audio_mode_t mode)
{
    (void)dev;
    (void)mode;
    return 0;
}

static int adev_set_mic_mute(struct audio_hw_device *dev, bool state)
{
    (void)dev;
    (void)state;
    return -ENOSYS;
}

static int adev_get_mic_mute(const struct audio_hw_device *dev, bool *state)
{
    (void)dev;
    (void)state;
    return -ENOSYS;
}

static size_t adev_get_input_buffer_size(const struct audio_hw_device *dev,
                                         const struct audio_config *config)
{
    if (audio_is_linear_pcm(config->format)) {
        size_t max_buffer_period_size_frames = 0;
        struct submix_audio_device * rsxadev = audio_hw_device_get_submix_audio_device(
                const_cast<struct audio_hw_device*>(dev));
        // look for the largest buffer period size
        for (int i = 0 ; i < MAX_ROUTES ; i++) {
            if (rsxadev->routes[i].config.buffer_period_size_frames > max_buffer_period_size_frames)
            {
                max_buffer_period_size_frames = rsxadev->routes[i].config.buffer_period_size_frames;
            }
        }
        const size_t frame_size_in_bytes = audio_channel_count_from_in_mask(config->channel_mask) *
                audio_bytes_per_sample(config->format);
        const size_t buffer_size = max_buffer_period_size_frames * frame_size_in_bytes;
        SUBMIX_ALOGV("adev_get_input_buffer_size() returns %zu bytes, %zu frames",
                 buffer_size, buffer_period_size_frames);
        return buffer_size;
    }
    return 0;
}

static int adev_open_input_stream(struct audio_hw_device *dev,
                                  audio_io_handle_t handle,
                                  audio_devices_t devices,
                                  struct audio_config *config,
                                  struct audio_stream_in **stream_in,
                                  audio_input_flags_t flags __unused,
                                  const char *address,
                                  audio_source_t source __unused)
{
    struct submix_audio_device *rsxadev = audio_hw_device_get_submix_audio_device(dev);
    struct submix_stream_in *in;
    ALOGD("adev_open_input_stream(addr=%s)", address);
    (void)handle;
    (void)devices;

    *stream_in = NULL;

    // Do we already have a route for this address
    int route_idx = -1;

    pthread_mutex_lock(&rsxadev->lock);

    status_t res = submix_get_route_idx_for_address_l(rsxadev, address, &route_idx);
    if (res != OK) {
        ALOGE("Error %d looking for address=%s in adev_open_output_stream", res, address);
        pthread_mutex_unlock(&rsxadev->lock);
        return res;
    }

    // Make sure it's possible to open the device given the current audio config.
    submix_sanitize_config(config, true);
    if (!submix_open_validate_l(rsxadev, route_idx, config, true)) {
        ALOGE("adev_open_input_stream(): Unable to open input stream.");
        pthread_mutex_unlock(&rsxadev->lock);
        return -EINVAL;
    }

#if ENABLE_LEGACY_INPUT_OPEN
    in = rsxadev->routes[route_idx].input;
    if (in) {
        in->ref_count++;
        sp<MonoPipe> sink = rsxadev->routes[route_idx].rsxSink;
        ALOG_ASSERT(sink != NULL);
        // If the sink has been shutdown, delete the pipe.
        if (sink != NULL) {
            if (sink->isShutdown()) {
                ALOGD(" Non-NULL shut down sink when opening input stream, releasing, refcount=%d",
                        in->ref_count);
                submix_audio_device_release_pipe_l(rsxadev, in->route_handle);
            } else {
                ALOGD(" Non-NULL sink when opening input stream, refcount=%d", in->ref_count);
            }
        } else {
            ALOGE("NULL sink when opening input stream, refcount=%d", in->ref_count);
        }
    }
#else
    in = NULL;
#endif // ENABLE_LEGACY_INPUT_OPEN

    if (!in) {
        in = (struct submix_stream_in *)calloc(1, sizeof(struct submix_stream_in));
        if (!in) return -ENOMEM;
        in->ref_count = 1;

        // Initialize the function pointer tables (v-tables).
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

        in->dev = rsxadev;
#if LOG_STREAMS_TO_FILES
        in->log_fd = -1;
#endif
    }

    // Initialize the input stream.
    in->read_counter_frames = 0;
    in->input_standby = true;
    if (rsxadev->routes[route_idx].output != NULL) {
        in->output_standby_rec_thr = rsxadev->routes[route_idx].output->output_standby;
    } else {
        in->output_standby_rec_thr = true;
    }

    in->read_error_count = 0;
    // Initialize the pipe.
    ALOGV("adev_open_input_stream(): about to create pipe");
    submix_audio_device_create_pipe_l(rsxadev, config, DEFAULT_PIPE_SIZE_IN_FRAMES,
                                    DEFAULT_PIPE_PERIOD_COUNT, in, NULL, address, route_idx);
#if LOG_STREAMS_TO_FILES
    if (in->log_fd >= 0) close(in->log_fd);
    in->log_fd = open(LOG_STREAM_IN_FILENAME, O_CREAT | O_TRUNC | O_WRONLY,
                      LOG_STREAM_FILE_PERMISSIONS);
    ALOGE_IF(in->log_fd < 0, "adev_open_input_stream(): log file open failed %s",
             strerror(errno));
    ALOGV("adev_open_input_stream(): log_fd = %d", in->log_fd);
#endif // LOG_STREAMS_TO_FILES
    // Return the input stream.
    *stream_in = &in->stream;

    pthread_mutex_unlock(&rsxadev->lock);
    return 0;
}

static void adev_close_input_stream(struct audio_hw_device *dev,
                                    struct audio_stream_in *stream)
{
    struct submix_audio_device * const rsxadev = audio_hw_device_get_submix_audio_device(dev);

    struct submix_stream_in * const in = audio_stream_in_get_submix_stream_in(stream);
    ALOGD("adev_close_input_stream()");
    pthread_mutex_lock(&rsxadev->lock);
    submix_audio_device_destroy_pipe_l(rsxadev, in, NULL);
#if LOG_STREAMS_TO_FILES
    if (in->log_fd >= 0) close(in->log_fd);
#endif // LOG_STREAMS_TO_FILES
#if ENABLE_LEGACY_INPUT_OPEN
    if (in->ref_count == 0) free(in);
#else
    free(in);
#endif // ENABLE_LEGACY_INPUT_OPEN

    pthread_mutex_unlock(&rsxadev->lock);
}

static int adev_dump(const audio_hw_device_t *device, int fd)
{
    const struct submix_audio_device * rsxadev = //audio_hw_device_get_submix_audio_device(device);
            reinterpret_cast<const struct submix_audio_device *>(
                    reinterpret_cast<const uint8_t *>(device) -
                            offsetof(struct submix_audio_device, device));
    char msg[100];
    int n = sprintf(msg, "\nReroute submix audio module:\n");
    write(fd, &msg, n);
    for (int i=0 ; i < MAX_ROUTES ; i++) {
        n = sprintf(msg, " route[%d] rate in=%d out=%d, addr=[%s]\n", i,
                rsxadev->routes[i].config.input_sample_rate,
                rsxadev->routes[i].config.output_sample_rate,
                rsxadev->routes[i].address);
        write(fd, &msg, n);
    }
    return 0;
}

static int adev_close(hw_device_t *device)
{
    ALOGI("adev_close()");
    free(device);
    return 0;
}

static int adev_open(const hw_module_t* module, const char* name,
                     hw_device_t** device)
{
    ALOGI("adev_open(name=%s)", name);
    struct submix_audio_device *rsxadev;

    if (strcmp(name, AUDIO_HARDWARE_INTERFACE) != 0)
        return -EINVAL;

    rsxadev = (submix_audio_device*) calloc(1, sizeof(struct submix_audio_device));
    if (!rsxadev)
        return -ENOMEM;

    rsxadev->device.common.tag = HARDWARE_DEVICE_TAG;
    rsxadev->device.common.version = AUDIO_DEVICE_API_VERSION_2_0;
    rsxadev->device.common.module = (struct hw_module_t *) module;
    rsxadev->device.common.close = adev_close;

    rsxadev->device.init_check = adev_init_check;
    rsxadev->device.set_voice_volume = adev_set_voice_volume;
    rsxadev->device.set_master_volume = adev_set_master_volume;
    rsxadev->device.get_master_volume = adev_get_master_volume;
    rsxadev->device.set_master_mute = adev_set_master_mute;
    rsxadev->device.get_master_mute = adev_get_master_mute;
    rsxadev->device.set_mode = adev_set_mode;
    rsxadev->device.set_mic_mute = adev_set_mic_mute;
    rsxadev->device.get_mic_mute = adev_get_mic_mute;
    rsxadev->device.set_parameters = adev_set_parameters;
    rsxadev->device.get_parameters = adev_get_parameters;
    rsxadev->device.get_input_buffer_size = adev_get_input_buffer_size;
    rsxadev->device.open_output_stream = adev_open_output_stream;
    rsxadev->device.close_output_stream = adev_close_output_stream;
    rsxadev->device.open_input_stream = adev_open_input_stream;
    rsxadev->device.close_input_stream = adev_close_input_stream;
    rsxadev->device.dump = adev_dump;

    for (int i=0 ; i < MAX_ROUTES ; i++) {
            memset(&rsxadev->routes[i], 0, sizeof(route_config));
            strcpy(rsxadev->routes[i].address, "");
        }

    *device = &rsxadev->device.common;

    return 0;
}

static struct hw_module_methods_t hal_module_methods = {
    /* open */ adev_open,
};

struct audio_module HAL_MODULE_INFO_SYM = {
    /* common */ {
        /* tag */                HARDWARE_MODULE_TAG,
        /* module_api_version */ AUDIO_MODULE_API_VERSION_0_1,
        /* hal_api_version */    HARDWARE_HAL_API_VERSION,
        /* id */                 AUDIO_HARDWARE_MODULE_ID,
        /* name */               "Wifi Display audio HAL",
        /* author */             "The Android Open Source Project",
        /* methods */            &hal_module_methods,
        /* dso */                NULL,
        /* reserved */           { 0 },
    },
};

} //namespace android

} //extern "C"
