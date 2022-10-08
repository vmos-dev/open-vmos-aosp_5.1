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

#define LOG_TAG "usb_profile"
/*#define LOG_NDEBUG 0*/

#include "format.h"

#include <tinyalsa/asoundlib.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

/*
 * Maps from bit position in pcm_mask to AUDIO_ format constants.
 */
static audio_format_t const format_value_map[] = {
    AUDIO_FORMAT_PCM_8_BIT,           /* 00 - SNDRV_PCM_FORMAT_S8 */
    AUDIO_FORMAT_PCM_8_BIT,           /* 01 - SNDRV_PCM_FORMAT_U8 */
    AUDIO_FORMAT_PCM_16_BIT,          /* 02 - SNDRV_PCM_FORMAT_S16_LE */
    AUDIO_FORMAT_INVALID,             /* 03 - SNDRV_PCM_FORMAT_S16_BE */
    AUDIO_FORMAT_INVALID,             /* 04 - SNDRV_PCM_FORMAT_U16_LE */
    AUDIO_FORMAT_INVALID,             /* 05 - SNDRV_PCM_FORMAT_U16_BE */
    AUDIO_FORMAT_INVALID,             /* 06 - SNDRV_PCM_FORMAT_S24_LE */
    AUDIO_FORMAT_INVALID,             /* 07 - SNDRV_PCM_FORMAT_S24_BE */
    AUDIO_FORMAT_INVALID,             /* 08 - SNDRV_PCM_FORMAT_U24_LE */
    AUDIO_FORMAT_INVALID,             /* 09 - SNDRV_PCM_FORMAT_U24_BE */
    AUDIO_FORMAT_PCM_32_BIT,          /* 10 - SNDRV_PCM_FORMAT_S32_LE */
    AUDIO_FORMAT_INVALID,             /* 11 - SNDRV_PCM_FORMAT_S32_BE */
    AUDIO_FORMAT_INVALID,             /* 12 - SNDRV_PCM_FORMAT_U32_LE */
    AUDIO_FORMAT_INVALID,             /* 13 - SNDRV_PCM_FORMAT_U32_BE */
    AUDIO_FORMAT_PCM_FLOAT,           /* 14 - SNDRV_PCM_FORMAT_FLOAT_LE */
    AUDIO_FORMAT_INVALID,             /* 15 - SNDRV_PCM_FORMAT_FLOAT_BE */
    AUDIO_FORMAT_INVALID,             /* 16 - SNDRV_PCM_FORMAT_FLOAT64_LE */
    AUDIO_FORMAT_INVALID,             /* 17 - SNDRV_PCM_FORMAT_FLOAT64_BE */
    AUDIO_FORMAT_INVALID,             /* 18 - SNDRV_PCM_FORMAT_IEC958_SUBFRAME_LE */
    AUDIO_FORMAT_INVALID,             /* 19 - SNDRV_PCM_FORMAT_IEC958_SUBFRAME_BE */
    AUDIO_FORMAT_INVALID,             /* 20 - SNDRV_PCM_FORMAT_MU_LAW */
    AUDIO_FORMAT_INVALID,             /* 21 - SNDRV_PCM_FORMAT_A_LAW */
    AUDIO_FORMAT_INVALID,             /* 22 - SNDRV_PCM_FORMAT_IMA_ADPCM */
    AUDIO_FORMAT_INVALID,             /* 23 - SNDRV_PCM_FORMAT_MPEG */
    AUDIO_FORMAT_INVALID,             /* 24 - SNDRV_PCM_FORMAT_GSM */
    AUDIO_FORMAT_INVALID,             /* 25 -> 30 (not assigned) */
    AUDIO_FORMAT_INVALID,
    AUDIO_FORMAT_INVALID,
    AUDIO_FORMAT_INVALID,
    AUDIO_FORMAT_INVALID,
    AUDIO_FORMAT_INVALID,
    AUDIO_FORMAT_INVALID,             /* 31 - SNDRV_PCM_FORMAT_SPECIAL */
    AUDIO_FORMAT_PCM_24_BIT_PACKED,   /* 32 - SNDRV_PCM_FORMAT_S24_3LE */
    AUDIO_FORMAT_INVALID,             /* 33 - SNDRV_PCM_FORMAT_S24_3BE */
    AUDIO_FORMAT_INVALID,             /* 34 - SNDRV_PCM_FORMAT_U24_3LE */
    AUDIO_FORMAT_INVALID,             /* 35 - SNDRV_PCM_FORMAT_U24_3BE */
    AUDIO_FORMAT_INVALID,             /* 36 - SNDRV_PCM_FORMAT_S20_3LE */
    AUDIO_FORMAT_INVALID,             /* 37 - SNDRV_PCM_FORMAT_S20_3BE */
    AUDIO_FORMAT_INVALID,             /* 38 - SNDRV_PCM_FORMAT_U20_3LE */
    AUDIO_FORMAT_INVALID,             /* 39 - SNDRV_PCM_FORMAT_U20_3BE */
    AUDIO_FORMAT_INVALID,             /* 40 - SNDRV_PCM_FORMAT_S18_3LE */
    AUDIO_FORMAT_INVALID,             /* 41 - SNDRV_PCM_FORMAT_S18_3BE */
    AUDIO_FORMAT_INVALID,             /* 42 - SNDRV_PCM_FORMAT_U18_3LE */
    AUDIO_FORMAT_INVALID,             /* 43 - SNDRV_PCM_FORMAT_U18_3BE */
    AUDIO_FORMAT_INVALID,             /* 44 - SNDRV_PCM_FORMAT_G723_24 */
    AUDIO_FORMAT_INVALID,             /* 45 - SNDRV_PCM_FORMAT_G723_24_1B */
    AUDIO_FORMAT_INVALID,             /* 46 - SNDRV_PCM_FORMAT_G723_40 */
    AUDIO_FORMAT_INVALID,             /* 47 - SNDRV_PCM_FORMAT_G723_40_1B */
    AUDIO_FORMAT_INVALID,             /* 48 - SNDRV_PCM_FORMAT_DSD_U8 */
    AUDIO_FORMAT_INVALID              /* 49 - SNDRV_PCM_FORMAT_DSD_U16_LE */
};

audio_format_t get_format_for_mask(struct pcm_mask* mask)
{
    int num_slots = sizeof(mask->bits) / sizeof(mask->bits[0]);
    int bits_per_slot = sizeof(mask->bits[0]) * 8;

    int table_size = sizeof(format_value_map) / sizeof(format_value_map[0]);

    int slot_index, bit_index, table_index;
    table_index = 0;
    int num_written = 0;
    for (slot_index = 0; slot_index < num_slots; slot_index++) {
        unsigned bit_mask = 1;
        for (bit_index = 0; bit_index < bits_per_slot; bit_index++) {
            /* don't return b-bit formats even if they are supported */
            if (table_index >= 2 && (mask->bits[slot_index] & bit_mask) != 0) {
                /* just return the first one */
                return table_index < table_size
                           ? format_value_map[table_index]
                           : AUDIO_FORMAT_INVALID;
            }
            bit_mask <<= 1;
            table_index++;
        }
    }

    return AUDIO_FORMAT_INVALID;
}

/*
 * Maps from bit position in pcm_mask to PCM_ format constants.
 */
int8_t const pcm_format_value_map[50] = {
    PCM_FORMAT_S8,          /* 00 - SNDRV_PCM_FORMAT_S8 */
    PCM_FORMAT_INVALID,     /* 01 - SNDRV_PCM_FORMAT_U8 */
    PCM_FORMAT_S16_LE,      /* 02 - SNDRV_PCM_FORMAT_S16_LE */
    PCM_FORMAT_INVALID,     /* 03 - SNDRV_PCM_FORMAT_S16_BE */
    PCM_FORMAT_INVALID,     /* 04 - SNDRV_PCM_FORMAT_U16_LE */
    PCM_FORMAT_INVALID,     /* 05 - SNDRV_PCM_FORMAT_U16_BE */
    PCM_FORMAT_S24_3LE,     /* 06 - SNDRV_PCM_FORMAT_S24_LE */
    PCM_FORMAT_INVALID,     /* 07 - SNDRV_PCM_FORMAT_S24_BE */
    PCM_FORMAT_INVALID,     /* 08 - SNDRV_PCM_FORMAT_U24_LE */
    PCM_FORMAT_INVALID,     /* 09 - SNDRV_PCM_FORMAT_U24_BE */
    PCM_FORMAT_S32_LE,      /* 10 - SNDRV_PCM_FORMAT_S32_LE */
    PCM_FORMAT_INVALID,     /* 11 - SNDRV_PCM_FORMAT_S32_BE */
    PCM_FORMAT_INVALID,     /* 12 - SNDRV_PCM_FORMAT_U32_LE */
    PCM_FORMAT_INVALID,     /* 13 - SNDRV_PCM_FORMAT_U32_BE */
    PCM_FORMAT_INVALID,     /* 14 - SNDRV_PCM_FORMAT_FLOAT_LE */
    PCM_FORMAT_INVALID,     /* 15 - SNDRV_PCM_FORMAT_FLOAT_BE */
    PCM_FORMAT_INVALID,     /* 16 - SNDRV_PCM_FORMAT_FLOAT64_LE */
    PCM_FORMAT_INVALID,     /* 17 - SNDRV_PCM_FORMAT_FLOAT64_BE */
    PCM_FORMAT_INVALID,     /* 18 - SNDRV_PCM_FORMAT_IEC958_SUBFRAME_LE */
    PCM_FORMAT_INVALID,     /* 19 - SNDRV_PCM_FORMAT_IEC958_SUBFRAME_BE */
    PCM_FORMAT_INVALID,     /* 20 - SNDRV_PCM_FORMAT_MU_LAW */
    PCM_FORMAT_INVALID,     /* 21 - SNDRV_PCM_FORMAT_A_LAW */
    PCM_FORMAT_INVALID,     /* 22 - SNDRV_PCM_FORMAT_IMA_ADPCM */
    PCM_FORMAT_INVALID,     /* 23 - SNDRV_PCM_FORMAT_MPEG */
    PCM_FORMAT_INVALID,     /* 24 - SNDRV_PCM_FORMAT_GSM */
    PCM_FORMAT_INVALID,     /* 25 -> 30 (not assigned) */
    PCM_FORMAT_INVALID,
    PCM_FORMAT_INVALID,
    PCM_FORMAT_INVALID,
    PCM_FORMAT_INVALID,
    PCM_FORMAT_INVALID,
    PCM_FORMAT_INVALID,     /* 31 - SNDRV_PCM_FORMAT_SPECIAL */
    PCM_FORMAT_S24_3LE,     /* 32 - SNDRV_PCM_FORMAT_S24_3LE */ /* ??? */
    PCM_FORMAT_INVALID,     /* 33 - SNDRV_PCM_FORMAT_S24_3BE */
    PCM_FORMAT_INVALID,     /* 34 - SNDRV_PCM_FORMAT_U24_3LE */
    PCM_FORMAT_INVALID,     /* 35 - SNDRV_PCM_FORMAT_U24_3BE */
    PCM_FORMAT_INVALID,     /* 36 - SNDRV_PCM_FORMAT_S20_3LE */
    PCM_FORMAT_INVALID,     /* 37 - SNDRV_PCM_FORMAT_S20_3BE */
    PCM_FORMAT_INVALID,     /* 38 - SNDRV_PCM_FORMAT_U20_3LE */
    PCM_FORMAT_INVALID,     /* 39 - SNDRV_PCM_FORMAT_U20_3BE */
    PCM_FORMAT_INVALID,     /* 40 - SNDRV_PCM_FORMAT_S18_3LE */
    PCM_FORMAT_INVALID,     /* 41 - SNDRV_PCM_FORMAT_S18_3BE */
    PCM_FORMAT_INVALID,     /* 42 - SNDRV_PCM_FORMAT_U18_3LE */
    PCM_FORMAT_INVALID,     /* 43 - SNDRV_PCM_FORMAT_U18_3BE */
    PCM_FORMAT_INVALID,     /* 44 - SNDRV_PCM_FORMAT_G723_24 */
    PCM_FORMAT_INVALID,     /* 45 - SNDRV_PCM_FORMAT_G723_24_1B */
    PCM_FORMAT_INVALID,     /* 46 - SNDRV_PCM_FORMAT_G723_40 */
    PCM_FORMAT_INVALID,     /* 47 - SNDRV_PCM_FORMAT_G723_40_1B */
    PCM_FORMAT_INVALID,     /* 48 - SNDRV_PCM_FORMAT_DSD_U8 */
    PCM_FORMAT_INVALID      /* 49 - SNDRV_PCM_FORMAT_DSD_U16_LE */
};

/*
 * Scans the provided format mask and returns the first non-8 bit sample
 * format supported by the devices.
 */
enum pcm_format get_pcm_format_for_mask(struct pcm_mask* mask)
{
    int num_slots = ARRAY_SIZE(mask->bits);
    int bits_per_slot = sizeof(mask->bits[0]) * 8;

    int table_size = ARRAY_SIZE(pcm_format_value_map);

    int slot_index, bit_index, table_index;
    table_index = 0;
    int num_written = 0;
    for (slot_index = 0; slot_index < num_slots && table_index < table_size; slot_index++) {
        unsigned bit_mask = 1;
        for (bit_index = 0; bit_index < bits_per_slot && table_index < table_size; bit_index++) {
            /* skip any 8-bit formats */
            if (table_index >= 2 && (mask->bits[slot_index] & bit_mask) != 0) {
                /* just return the first one which will be at least 16-bit */
                return (int)pcm_format_value_map[table_index];
            }
            bit_mask <<= 1;
            table_index++;
        }
    }

    return PCM_FORMAT_INVALID;
}
