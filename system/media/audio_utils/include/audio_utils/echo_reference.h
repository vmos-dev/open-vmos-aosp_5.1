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

#ifndef ANDROID_ECHO_REFERENCE_H
#define ANDROID_ECHO_REFERENCE_H

#include <stdint.h>
#include <sys/time.h>

__BEGIN_DECLS

/* Buffer descriptor used by read() and write() methods, including the time stamp and delay. */
struct echo_reference_buffer {
    void *raw;                  // pointer to audio frame
    size_t frame_count;         // number of frames in buffer
    int32_t delay_ns;           // delay for this buffer (see comment below)
    struct timespec time_stamp; // time stamp for this buffer (see comment below)
                                // default ALSA gettimeofday() format
};
/**
 * + as input:
 *      - delay_ns is the delay introduced by playback buffers
 *      - time_stamp is the time stamp corresponding to the delay calculation
 * + as output:
 *      unused
 * when used for EchoReference::read():
 * + as input:
 *      - delay_ns is the delay introduced by capture buffers
 *      - time_stamp is the time stamp corresponding to the delay calculation
 * + as output:
 *      - delay_ns is the delay between the returned frames and the capture time derived from
 *      delay and time stamp indicated as input. This delay is to be communicated to the AEC.
 *      - frame_count is updated with the actual number of frames returned
 */

struct echo_reference_itfe {
    int (*read)(struct echo_reference_itfe *echo_reference, struct echo_reference_buffer *buffer);
    int (*write)(struct echo_reference_itfe *echo_reference, struct echo_reference_buffer *buffer);
};

int create_echo_reference(audio_format_t rdFormat,
                          uint32_t rdChannelCount,
                          uint32_t rdSamplingRate,
                          audio_format_t wrFormat,
                          uint32_t wrChannelCount,
                          uint32_t wrSamplingRate,
                          struct echo_reference_itfe **);

void release_echo_reference(struct echo_reference_itfe *echo_reference);

__END_DECLS

#endif // ANDROID_ECHO_REFERENCE_H
