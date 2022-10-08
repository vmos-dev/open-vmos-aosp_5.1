/*
 * Copyright (C) 2014 The Android Open Source Project
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

#ifndef ANDROID_AUDIO_CHANNELS_H
#define ANDROID_AUDIO_CHANNELS_H

__BEGIN_DECLS

/*
 * Expands or contracts sample data from one interleaved channel format to another.
 * Expanded channels are filled with zeros and put at the end of each audio frame.
 * Contracted channels are omitted from the end of each audio frame.
 *   in_buff points to the buffer of samples
 *   in_buff_channels Specifies the number of channels in the input buffer.
 *   out_buff points to the buffer to receive converted samples.
 *   out_buff_channels Specifies the number of channels in the output buffer.
 *   sample_size_in_bytes Specifies the number of bytes per sample. 1, 2, 3, 4 are
 *     currently valid.
 *   num_in_bytes size of input buffer in BYTES
 * returns
 *   the number of BYTES of output data or 0 if an error occurs.
 * NOTE
 *   The out and sums buffers must either be completely separate (non-overlapping), or
 *   they must both start at the same address. Partially overlapping buffers are not supported.
 */
size_t adjust_channels(const void* in_buff, size_t in_buff_chans,
                       void* out_buff, size_t out_buff_chans,
                       unsigned sample_size_in_bytes, size_t num_in_bytes);
__END_DECLS

#endif
