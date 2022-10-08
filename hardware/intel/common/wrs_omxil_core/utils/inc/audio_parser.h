/*
 * audio_parser.h, helper parser for audio codec data
 *
 * Copyright (c) 2009-2010 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __WRS_OMXIL_AUDIO_PARSER
#define ___WRS_OMXIL_AUDIO_PARSER

#ifdef __cplusplus
extern "C" {
#endif

/*
 * MP3
 */

#define MP3_HEADER_VERSION_25           0x0
#define MP3_HEADER_VERSION_2            0x2
#define MP3_HEADER_VERSION_1            0x3

#define MP3_HEADER_LAYER_3              0x1
#define MP3_HEADER_LAYER_2              0x2
#define MP3_HEADER_LAYER_1              0x3

#define MP3_HEADER_CRC_PROTECTED        0x0
#define MP3_HEADER_NOT_PROTECTED        0x1

#define MP3_HEADER_STEREO               0x0
#define MP3_HEADER_JOINT_STEREO         0x1
#define MP3_HEADER_DUAL_CHANNEL         0x2
#define MP3_HEADER_SINGLE_CHANNEL       0x3

int mp3_header_parse(const unsigned char *buffer,
                     int *version, int *layer, int *crc, int *bitrate,
                     int *frequency, int *channel, int *mode_extension,
                     int *frame_length, int *frame_duration);

/* end of MP3 */

/*
 * MP4
 */

int audio_specific_config_parse(const unsigned char *buffer,
                                int *aot, int *frequency, int *channel);

int audio_specific_config_bitcoding(unsigned char *buffer,
                                    int aot, int frequency, int channel);

/* end of MP4 */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ___WRS_OMXIL_AUDIO_PARSER */
