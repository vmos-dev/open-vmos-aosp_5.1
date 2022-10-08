/*
** Copyright 2011, The Android Open-Source Project
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

//#define LOG_NDEBUG 0
#define LOG_TAG "echo_reference"

#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdlib.h>

#include <log/log.h>
#include <system/audio.h>
#include <audio_utils/resampler.h>
#include <audio_utils/echo_reference.h>

// echo reference state: bit field indicating if read, write or both are active.
enum state {
    ECHOREF_IDLE = 0x00,        // idle
    ECHOREF_READING = 0x01,     // reading is active
    ECHOREF_WRITING = 0x02      // writing is active
};

struct echo_reference {
    struct echo_reference_itfe itfe;
    int status;                     // init status
    uint32_t state;                 // active state: reading, writing or both
    audio_format_t rd_format;       // read sample format
    uint32_t rd_channel_count;      // read number of channels
    uint32_t rd_sampling_rate;      // read sampling rate in Hz
    size_t rd_frame_size;           // read frame size (bytes per sample)
    audio_format_t wr_format;       // write sample format
    uint32_t wr_channel_count;      // write number of channels
    uint32_t wr_sampling_rate;      // write sampling rate in Hz
    size_t wr_frame_size;           // write frame size (bytes per sample)
    void *buffer;                   // main buffer
    size_t buf_size;                // main buffer size in frames
    size_t frames_in;               // number of frames in main buffer
    void *wr_buf;                   // buffer for input conversions
    size_t wr_buf_size;             // size of conversion buffer in frames
    size_t wr_frames_in;            // number of frames in conversion buffer
    size_t wr_curr_frame_size;      // number of frames given to current write() function
    void *wr_src_buf;               // resampler input buf (either wr_buf or buffer used by write())
    struct timespec wr_render_time; // latest render time indicated by write()
                                    // default ALSA gettimeofday() format
    int32_t  playback_delay;        // playback buffer delay indicated by last write()
    int16_t prev_delta_sign;        // sign of previous delay difference:
                                    //  1: positive, -1: negative, 0: unknown
    uint16_t delta_count;           // number of consecutive delay differences with same sign
    pthread_mutex_t lock;                      // mutex protecting read/write concurrency
    pthread_cond_t cond;                       // condition signaled when data is ready to read
    struct resampler_itfe *resampler;          // input resampler
    struct resampler_buffer_provider provider; // resampler buffer provider
};


int echo_reference_get_next_buffer(struct resampler_buffer_provider *buffer_provider,
                                   struct resampler_buffer* buffer)
{
    struct echo_reference *er;

    if (buffer_provider == NULL) {
        return -EINVAL;
    }

    er = (struct echo_reference *)((char *)buffer_provider -
                                      offsetof(struct echo_reference, provider));

    if (er->wr_src_buf == NULL || er->wr_frames_in == 0) {
        buffer->raw = NULL;
        buffer->frame_count = 0;
        return -ENODATA;
    }

    buffer->frame_count = (buffer->frame_count > er->wr_frames_in) ?
            er->wr_frames_in : buffer->frame_count;
    // this is er->rd_channel_count here as we resample after stereo to mono conversion if any
    buffer->i16 = (int16_t *)er->wr_src_buf + (er->wr_curr_frame_size - er->wr_frames_in) *
            er->rd_channel_count;

    return 0;
}

void echo_reference_release_buffer(struct resampler_buffer_provider *buffer_provider,
                                  struct resampler_buffer* buffer)
{
    struct echo_reference *er;

    if (buffer_provider == NULL) {
        return;
    }

    er = (struct echo_reference *)((char *)buffer_provider -
                                      offsetof(struct echo_reference, provider));

    er->wr_frames_in -= buffer->frame_count;
}

static void echo_reference_reset_l(struct echo_reference *er)
{
    ALOGV("echo_reference_reset_l()");
    free(er->buffer);
    er->buffer = NULL;
    er->buf_size = 0;
    er->frames_in = 0;
    free(er->wr_buf);
    er->wr_buf = NULL;
    er->wr_buf_size = 0;
    er->wr_render_time.tv_sec = 0;
    er->wr_render_time.tv_nsec = 0;
    er->delta_count = 0;
    er->prev_delta_sign = 0;
}

/* additional space in resampler buffer allowing for extra samples to be returned
 * by speex resampler when sample rates ratio is not an integer.
 */
#define RESAMPLER_HEADROOM_SAMPLES   10

static int echo_reference_write(struct echo_reference_itfe *echo_reference,
                         struct echo_reference_buffer *buffer)
{
    struct echo_reference *er = (struct echo_reference *)echo_reference;
    int status = 0;

    if (er == NULL) {
        return -EINVAL;
    }

    pthread_mutex_lock(&er->lock);

    if (buffer == NULL) {
        ALOGV("echo_reference_write() stop write");
        er->state &= ~ECHOREF_WRITING;
        echo_reference_reset_l(er);
        goto exit;
    }

    ALOGV("echo_reference_write() START trying to write %zu frames", buffer->frame_count);
    ALOGV("echo_reference_write() playbackTimestamp:[%d].[%d], er->playback_delay:[%" PRId32 "]",
            (int)buffer->time_stamp.tv_sec,
            (int)buffer->time_stamp.tv_nsec, er->playback_delay);

    //ALOGV("echo_reference_write() %d frames", buffer->frame_count);
    // discard writes until a valid time stamp is provided.

    if ((buffer->time_stamp.tv_sec == 0) && (buffer->time_stamp.tv_nsec == 0) &&
        (er->wr_render_time.tv_sec == 0) && (er->wr_render_time.tv_nsec == 0)) {
        goto exit;
    }

    if ((er->state & ECHOREF_WRITING) == 0) {
        ALOGV("echo_reference_write() start write");
        if (er->resampler != NULL) {
            er->resampler->reset(er->resampler);
        }
        er->state |= ECHOREF_WRITING;
    }

    if ((er->state & ECHOREF_READING) == 0) {
        goto exit;
    }

    er->wr_render_time.tv_sec  = buffer->time_stamp.tv_sec;
    er->wr_render_time.tv_nsec = buffer->time_stamp.tv_nsec;

    er->playback_delay = buffer->delay_ns;

    // this will be used in the get_next_buffer, to support variable input buffer sizes
    er->wr_curr_frame_size = buffer->frame_count;

    void *srcBuf;
    size_t inFrames;
    // do stereo to mono and down sampling if necessary
    if (er->rd_channel_count != er->wr_channel_count ||
            er->rd_sampling_rate != er->wr_sampling_rate) {
        size_t wrBufSize = buffer->frame_count;

        inFrames = buffer->frame_count;

        if (er->rd_sampling_rate != er->wr_sampling_rate) {
            inFrames = (buffer->frame_count * er->rd_sampling_rate) / er->wr_sampling_rate +
                                                    RESAMPLER_HEADROOM_SAMPLES;
            // wr_buf is not only used as resampler output but also for stereo to mono conversion
            // output so buffer size is driven by both write and read sample rates
            if (inFrames > wrBufSize) {
                wrBufSize = inFrames;
            }
        }

        if (er->wr_buf_size < wrBufSize) {
            ALOGV("echo_reference_write() increasing write buffer size from %zu to %zu",
                    er->wr_buf_size, wrBufSize);
            er->wr_buf_size = wrBufSize;
            er->wr_buf = realloc(er->wr_buf, er->wr_buf_size * er->rd_frame_size);
        }

        if (er->rd_channel_count != er->wr_channel_count) {
            // must be stereo to mono
            int16_t *src16 = (int16_t *)buffer->raw;
            int16_t *dst16 = (int16_t *)er->wr_buf;
            size_t frames = buffer->frame_count;
            while (frames--) {
                *dst16++ = (int16_t)(((int32_t)*src16 + (int32_t)*(src16 + 1)) >> 1);
                src16 += 2;
            }
        }
        if (er->wr_sampling_rate != er->rd_sampling_rate) {
            if (er->resampler == NULL) {
                int rc;
                ALOGV("echo_reference_write() new ReSampler(%d, %d)",
                      er->wr_sampling_rate, er->rd_sampling_rate);
                er->provider.get_next_buffer = echo_reference_get_next_buffer;
                er->provider.release_buffer = echo_reference_release_buffer;
                rc = create_resampler(er->wr_sampling_rate,
                                 er->rd_sampling_rate,
                                 er->rd_channel_count,
                                 RESAMPLER_QUALITY_DEFAULT,
                                 &er->provider,
                                 &er->resampler);
                if (rc != 0) {
                    er->resampler = NULL;
                    ALOGV("echo_reference_write() failure to create resampler %d", rc);
                    status = -ENODEV;
                    goto exit;
                }
            }
            // er->wr_src_buf and er->wr_frames_in are used by getNexBuffer() called by the
            // resampler to get new frames
            if (er->rd_channel_count != er->wr_channel_count) {
                er->wr_src_buf = er->wr_buf;
            } else {
                er->wr_src_buf = buffer->raw;
            }
            er->wr_frames_in = buffer->frame_count;
            // inFrames is always more than we need here to get frames remaining from previous runs
            // inFrames is updated by resample() with the number of frames produced
            ALOGV("echo_reference_write() ReSampling(%d, %d)",
                  er->wr_sampling_rate, er->rd_sampling_rate);
            er->resampler->resample_from_provider(er->resampler,
                                                     (int16_t *)er->wr_buf, &inFrames);
            ALOGV_IF(er->wr_frames_in != 0,
                    "echo_reference_write() er->wr_frames_in not 0 (%d) after resampler",
                    er->wr_frames_in);
        }
        srcBuf = er->wr_buf;
    } else {
        inFrames = buffer->frame_count;
        srcBuf = buffer->raw;
    }

    if (er->frames_in + inFrames > er->buf_size) {
        ALOGV("echo_reference_write() increasing buffer size from %zu to %zu",
                er->buf_size, er->frames_in + inFrames);
                er->buf_size = er->frames_in + inFrames;
                er->buffer = realloc(er->buffer, er->buf_size * er->rd_frame_size);
    }
    memcpy((char *)er->buffer + er->frames_in * er->rd_frame_size,
           srcBuf,
           inFrames * er->rd_frame_size);
    er->frames_in += inFrames;

    ALOGV("echo_reference_write() frames written:[%zu], frames total:[%zu] buffer size:[%zu]\n"
          "                       er->wr_render_time:[%d].[%d], er->playback_delay:[%" PRId32 "]",
          inFrames, er->frames_in, er->buf_size,
          (int)er->wr_render_time.tv_sec, (int)er->wr_render_time.tv_nsec, er->playback_delay);

    pthread_cond_signal(&er->cond);
exit:
    pthread_mutex_unlock(&er->lock);
    ALOGV("echo_reference_write() END");
    return status;
}

// delay jump threshold to update ref buffer: 6 samples at 8kHz in nsecs
#define MIN_DELAY_DELTA_NS (375000*2)
// number of consecutive delta with same sign between expected and actual delay before adjusting
// the buffer
#define MIN_DELTA_NUM 4


static int echo_reference_read(struct echo_reference_itfe *echo_reference,
                         struct echo_reference_buffer *buffer)
{
    struct echo_reference *er = (struct echo_reference *)echo_reference;

    if (er == NULL) {
        return -EINVAL;
    }

    pthread_mutex_lock(&er->lock);

    if (buffer == NULL) {
        ALOGV("echo_reference_read() stop read");
        er->state &= ~ECHOREF_READING;
        goto exit;
    }

    ALOGV("echo_reference_read() START, delayCapture:[%" PRId32 "], "
            "er->frames_in:[%zu],buffer->frame_count:[%zu]",
    buffer->delay_ns, er->frames_in, buffer->frame_count);

    if ((er->state & ECHOREF_READING) == 0) {
        ALOGV("echo_reference_read() start read");
        echo_reference_reset_l(er);
        er->state |= ECHOREF_READING;
    }

    if ((er->state & ECHOREF_WRITING) == 0) {
        memset(buffer->raw, 0, er->rd_frame_size * buffer->frame_count);
        buffer->delay_ns = 0;
        goto exit;
    }

//    ALOGV("echo_reference_read() %d frames", buffer->frame_count);

    // allow some time for new frames to arrive if not enough frames are ready for read
    if (er->frames_in < buffer->frame_count) {
        uint32_t timeoutMs = (uint32_t)((1000 * buffer->frame_count) / er->rd_sampling_rate / 2);
        struct timespec ts = {0, 0};

#ifndef HAVE_PTHREAD_COND_TIMEDWAIT_RELATIVE
        clock_gettime(CLOCK_REALTIME, &ts);
#endif

        ts.tv_sec  += timeoutMs/1000;
        ts.tv_nsec += (timeoutMs%1000) * 1000000;
        if (ts.tv_nsec >= 1000000000) {
            ts.tv_nsec -= 1000000000;
            ts.tv_sec  += 1;
        }

#ifdef HAVE_PTHREAD_COND_TIMEDWAIT_RELATIVE
        pthread_cond_timedwait_relative_np(&er->cond, &er->lock, &ts);
#else
        pthread_cond_timedwait(&er->cond, &er->lock, &ts);
#endif

        ALOGV_IF((er->frames_in < buffer->frame_count),
                 "echo_reference_read() waited %d ms but still not enough frames"\
                 " er->frames_in: %d, buffer->frame_count = %d",
                 timeoutMs, er->frames_in, buffer->frame_count);
    }

    int64_t timeDiff;
    struct timespec tmp;

    if ((er->wr_render_time.tv_sec == 0 && er->wr_render_time.tv_nsec == 0) ||
        (buffer->time_stamp.tv_sec == 0 && buffer->time_stamp.tv_nsec == 0)) {
        ALOGV("echo_reference_read(): NEW:timestamp is zero---------setting timeDiff = 0, "\
             "not updating delay this time");
        timeDiff = 0;
    } else {
        if (buffer->time_stamp.tv_nsec < er->wr_render_time.tv_nsec) {
            tmp.tv_sec = buffer->time_stamp.tv_sec - er->wr_render_time.tv_sec - 1;
            tmp.tv_nsec = 1000000000 + buffer->time_stamp.tv_nsec - er->wr_render_time.tv_nsec;
        } else {
            tmp.tv_sec = buffer->time_stamp.tv_sec - er->wr_render_time.tv_sec;
            tmp.tv_nsec = buffer->time_stamp.tv_nsec - er->wr_render_time.tv_nsec;
        }
        timeDiff = (((int64_t)tmp.tv_sec * 1000000000 + tmp.tv_nsec));

        int64_t expectedDelayNs =  er->playback_delay + buffer->delay_ns - timeDiff;

        if (er->resampler != NULL) {
            // Resampler already compensates part of the delay
            int32_t rsmp_delay = er->resampler->delay_ns(er->resampler);
            expectedDelayNs -= rsmp_delay;
        }

        ALOGV("echo_reference_read(): expectedDelayNs[%" PRId64 "] = "
                "er->playback_delay[%" PRId32 "] + delayCapture[%" PRId32
                "] - timeDiff[%" PRId64 "]",
                expectedDelayNs, er->playback_delay, buffer->delay_ns, timeDiff);

        if (expectedDelayNs > 0) {
            int64_t delayNs = ((int64_t)er->frames_in * 1000000000) / er->rd_sampling_rate;

            int64_t  deltaNs = delayNs - expectedDelayNs;

            ALOGV("echo_reference_read(): EchoPathDelayDeviation between reference and DMA [%" PRId64 "]",
                    deltaNs);
            if (abs(deltaNs) >= MIN_DELAY_DELTA_NS) {
                // smooth the variation and update the reference buffer only
                // if a deviation in the same direction is observed for more than MIN_DELTA_NUM
                // consecutive reads.
                int16_t delay_sign = (deltaNs >= 0) ? 1 : -1;
                if (delay_sign == er->prev_delta_sign) {
                    er->delta_count++;
                } else {
                    er->delta_count = 1;
                }
                er->prev_delta_sign = delay_sign;

                if (er->delta_count > MIN_DELTA_NUM) {
                    size_t previousFrameIn = er->frames_in;
                    er->frames_in = (size_t)((expectedDelayNs * er->rd_sampling_rate)/1000000000);
                    int offset = er->frames_in - previousFrameIn;

                    ALOGV("echo_reference_read(): deltaNs ENOUGH and %s: "
                            "er->frames_in: %zu, previousFrameIn = %zu",
                         delay_sign ? "positive" : "negative", er->frames_in, previousFrameIn);

                    if (deltaNs < 0) {
                        // Less data available in the reference buffer than expected
                        if (er->frames_in > er->buf_size) {
                            er->buf_size = er->frames_in;
                            er->buffer  = realloc(er->buffer, er->buf_size * er->rd_frame_size);
                            ALOGV("echo_reference_read(): increasing buffer size to %zu",
                                  er->buf_size);
                        }

                        if (offset > 0) {
                            memset((char *)er->buffer + previousFrameIn * er->rd_frame_size,
                                   0, offset * er->rd_frame_size);
                            ALOGV("echo_reference_read(): pushing ref buffer by [%d]", offset);
                        }
                    } else {
                        // More data available in the reference buffer than expected
                        offset = -offset;
                        if (offset > 0) {
                            memcpy(er->buffer, (char *)er->buffer + (offset * er->rd_frame_size),
                                   er->frames_in * er->rd_frame_size);
                            ALOGV("echo_reference_read(): shifting ref buffer by [%zu]",
                                  er->frames_in);
                        }
                    }
                }
            } else {
                er->delta_count = 0;
                er->prev_delta_sign = 0;
                ALOGV("echo_reference_read(): Constant EchoPathDelay - difference "
                        "between reference and DMA %" PRId64, deltaNs);
            }
        } else {
            ALOGV("echo_reference_read(): NEGATIVE expectedDelayNs[%" PRId64
                 "] = er->playback_delay[%" PRId32 "] + delayCapture[%" PRId32
                 "] - timeDiff[%" PRId64 "]",
                 expectedDelayNs, er->playback_delay, buffer->delay_ns, timeDiff);
        }
    }

    if (er->frames_in < buffer->frame_count) {
        if (buffer->frame_count > er->buf_size) {
            er->buf_size = buffer->frame_count;
            er->buffer  = realloc(er->buffer, er->buf_size * er->rd_frame_size);
            ALOGV("echo_reference_read(): increasing buffer size to %zu", er->buf_size);
        }
        // filling up the reference buffer with 0s to match the expected delay.
        memset((char *)er->buffer + er->frames_in * er->rd_frame_size,
            0, (buffer->frame_count - er->frames_in) * er->rd_frame_size);
        er->frames_in = buffer->frame_count;
    }

    memcpy(buffer->raw,
           (char *)er->buffer,
           buffer->frame_count * er->rd_frame_size);

    er->frames_in -= buffer->frame_count;
    memcpy(er->buffer,
           (char *)er->buffer + buffer->frame_count * er->rd_frame_size,
           er->frames_in * er->rd_frame_size);

    // As the reference buffer is now time aligned to the microphone signal there is a zero delay
    buffer->delay_ns = 0;

    ALOGV("echo_reference_read() END %zu frames, total frames in %zu",
          buffer->frame_count, er->frames_in);

    pthread_cond_signal(&er->cond);

exit:
    pthread_mutex_unlock(&er->lock);
    return 0;
}


int create_echo_reference(audio_format_t rdFormat,
                            uint32_t rdChannelCount,
                            uint32_t rdSamplingRate,
                            audio_format_t wrFormat,
                            uint32_t wrChannelCount,
                            uint32_t wrSamplingRate,
                            struct echo_reference_itfe **echo_reference)
{
    struct echo_reference *er;

    ALOGV("create_echo_reference()");

    if (echo_reference == NULL) {
        return -EINVAL;
    }

    *echo_reference = NULL;

    if (rdFormat != AUDIO_FORMAT_PCM_16_BIT ||
            rdFormat != wrFormat) {
        ALOGW("create_echo_reference bad format rd %d, wr %d", rdFormat, wrFormat);
        return -EINVAL;
    }
    if ((rdChannelCount != 1 && rdChannelCount != 2) ||
            wrChannelCount != 2) {
        ALOGW("create_echo_reference bad channel count rd %d, wr %d", rdChannelCount,
                wrChannelCount);
        return -EINVAL;
    }

    er = (struct echo_reference *)calloc(1, sizeof(struct echo_reference));

    er->itfe.read = echo_reference_read;
    er->itfe.write = echo_reference_write;

    er->state = ECHOREF_IDLE;
    er->rd_format = rdFormat;
    er->rd_channel_count = rdChannelCount;
    er->rd_sampling_rate = rdSamplingRate;
    er->wr_format = wrFormat;
    er->wr_channel_count = wrChannelCount;
    er->wr_sampling_rate = wrSamplingRate;
    er->rd_frame_size = audio_bytes_per_sample(rdFormat) * rdChannelCount;
    er->wr_frame_size = audio_bytes_per_sample(wrFormat) * wrChannelCount;
    *echo_reference = &er->itfe;
    return 0;
}

void release_echo_reference(struct echo_reference_itfe *echo_reference) {
    struct echo_reference *er = (struct echo_reference *)echo_reference;

    if (er == NULL) {
        return;
    }

    ALOGV("EchoReference dstor");
    echo_reference_reset_l(er);
    if (er->resampler != NULL) {
        release_resampler(er->resampler);
    }
    free(er);
}

