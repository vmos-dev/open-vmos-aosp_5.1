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

#ifndef __WV_MOD_OEM_CRYPTO_H_
#define __WV_MOD_OEM_CRYPTO_H_

#include <inttypes.h>

enum drm_wv_mod_algorithm
{
    DRM_WV_MOD_AES_CBC_128_NO_PADDING = 0,
    HMAC_SHA256
};

enum drm_wv_mod_rsa_padding_scheme
{
    DRM_WV_MOD_RSA_PADDING_SCHEME_RSASSA_PSS_SHA1 = 1,
    DRM_WV_MOD_RSA_PADDING_SCHEME_RSASSA_PKCS1v15,
    DRM_WV_MOD_RSA_PADDING_SCHEME_MAX_VALUE
};

struct drm_wv_mod_key_object
{
    const uint8_t *key_id;
    uint32_t key_id_len;
    const uint8_t *key_data_iv;
    const uint8_t *key_data;
    uint32_t key_data_length;
    const uint8_t *key_control_iv;
    const uint8_t *key_control;
};

struct drm_wv_mod_key_refresh_object
{
    const uint8_t *key_id;
    uint32_t key_id_len;
    const uint8_t *key_control_iv;
    const uint8_t *key_control;
};

#endif /* __WV_MOD_OEM_CRYPTO_H_ */
