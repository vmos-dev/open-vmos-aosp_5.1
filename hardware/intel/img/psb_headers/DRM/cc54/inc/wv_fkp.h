/*
 * Copyright (C) 2014 Intel Corporation. All rights reserved.
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

#ifndef WV_FKP_H_
#define WV_FKP_H_

#include <stdint.h>

#define KEYBOX_DEVID_SIZE       (32)    // 32 bytes
#define KEYBOX_DEVKEY_SIZE      (16)    // 16 bytes
#define KEYBOX_KEYDATA_SIZE     (72)    // 72 bytes
#define KEYBOX_MAGIC_SIZE       (4)     // 4 bytes
#define KEYBOX_CRC_SIZE         (4)     // 4 bytes
#define WIDE_VINE_KEYBOX_SIZE   (KEYBOX_DEVID_SIZE +    \
                                 KEYBOX_DEVKEY_SIZE +   \
                                 KEYBOX_KEYDATA_SIZE +  \
                                 KEYBOX_MAGIC_SIZE +    \
                                 KEYBOX_CRC_SIZE)

//#define WIDEVINE_MAGIC_WORD (0x6b626f78)

struct wv_keybox_info_length
{
    uint32_t key_data_size;     // default 72 bytes
    uint32_t device_id_sizee;     // default 32 bytes
};

//
// Widevine keybox provisioning key container structure.
//
struct wv_keybox
{
    uint8_t device_id[KEYBOX_DEVID_SIZE];     // 32 bytes
    uint8_t device_key[KEYBOX_DEVKEY_SIZE];     // 16 bytes
    uint8_t key_data[KEYBOX_KEYDATA_SIZE];     // 72 bytes
    uint8_t magic[KEYBOX_MAGIC_SIZE];     // 4 bytes
    uint8_t crc[KEYBOX_CRC_SIZE];     // 4 bytes
};


#endif /* WV_FKP_H_ */
