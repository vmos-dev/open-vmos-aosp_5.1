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

/*!
 * NOTE: Don't include this file. It is recommended to include sepdrm.h
 */
#ifndef __WV_DRM_API_H__
#define __WV_DRM_API_H__

#include <inttypes.h>
#include "drm_common_api.h"
#include "wv_fkp.h"

/*!
 * Defines
 */
#define NEW_FRAME_FLAG                  (1)
#define PREV_PACKET_PARTIAL_BLOCK_FLAG  (1 << 1)
#define CLEAR_CONTENT_FLAG              (1 << 2)

/*!
 * Structs
 */

/*
 * This structure is used to provide necessary information for Widevine audio
 * ciphertext decryption.
 *
 * The members are:
 *
 *   iv                     - AES initialization vector.
 *   input_ciphertext_size  - Input ciphertext data size in bytes.
 *   p_input_ciphertext     - Pointer to the input ciphertext data buffer.
 *   output_plaintext_size  - Output plaintext data size in bytes.
 *   p_output_plaintext     - Pointer to the output plaintext data buffer.
 */
struct drm_wv_audio_data
{
    uint8_t iv[AES_IV_COUNTER_SIZE_IN_BYTES];
    uint32_t input_ciphertext_size;
    uint8_t *p_input_ciphertext;
    uint32_t output_plaintext_size;
    uint8_t *p_output_plaintext;
};


/*
 * This structure is used to provide necessary information for Widevine video
 * ciphertext decryption.
 *
 * The members are:
 *
 *   iv                     - AES initialization vector.
 *   input_ciphertext_size  - Input ciphertext data size in bytes.
 *   p_input_ciphertext     - Pointer to the input ciphertext data.
 */
struct drm_wv_video_cipher
{
    uint8_t iv[AES_IV_COUNTER_SIZE_IN_BYTES];
    uint32_t input_ciphertext_size;
    uint8_t *p_input_ciphertext;
    uint8_t flags;
    uint8_t *p_output_enc_ciphertext;
    uint32_t output_ciphertext_size;
};

struct drm_wv_nalu_headers
{
    uint32_t frame_size;
    uint32_t parse_size;
    uint8_t *p_enc_ciphertext;
    uint32_t hdrs_buf_len;
    uint8_t *p_hdrs_buf;
};

/*!
 * Functions
 */


/*
 * Set Widevine Asset Key
 */
uint32_t drm_wv_set_entitlementkey(
    uint8_t *emm_keyptr,
    uint32_t emm_keylen);

/*
 * Derive Widevine Control Word
 */
uint32_t drm_wv_derive_controlword(
    uint8_t *cw_ptr,
    uint32_t cw_len,
    uint32_t *flags_ptr);

/*
 * Get Widevine Keybox infomation
 * Retrieve the encypted kbox data and decrypt the encrypted kbox data
 */
uint32_t drm_wv_get_keyboxinfo(
    uint8_t *key_data,
    uint32_t *key_data_size,
    uint8_t *device_id,
    uint32_t *device_id_size);


/*!
   *@brief Create a widevine session
 *
 */
uint32_t drm_wv_create_session(
    uint32_t *session_id);


/*
 * @brief Creates a Widevine session for HLS content.
 * @param[out] pSessionID Pointer to a variable that contains the session's ID
 *             number.
 */
uint32_t drm_wv_hls_create_session(
    uint32_t *session_id);

/*
 * @brief Decrypts Widevine encrypted audio data
 *
 * @param session_id DRM Session ID number
 * @param audio_info Pointer to a buffer containing Widevine audio information
 *
 * @return DRM_SUCCESSFUL The Widevine audio ciphertext was decrypted
 */
uint32_t drm_wv_decrypt_audio(
    uint32_t session_id,
    struct drm_wv_audio_data *audio_info);


/*
 * @brief Decrypts Widevine video ciphertext data into the IMR decryption buffer
 * @param session_id DRM Session ID number
 * @param video_info Pointer to the Widevine video data
 */
uint32_t drm_wv_decrypt_video(
    uint32_t session_id,
    struct drm_wv_video_cipher *video_info);


uint32_t drm_wv_return_naluheaders(
    uint32_t session_id,
    struct drm_wv_nalu_headers *nalu_info);

uint32_t drm_wv_keybox_provision(
    struct wv_keybox *buf);


/*
 * @brief Encrypts a Widevine keybox with a device specific key.
 *
 * @param[in] p_keybox Pointer to a Widevine keybox. May be NULL if
 *            keybox_length is zero.
 *
 * @param[in] keybox_length Length of the Widevine keybox in bytes. If zero then
 *            only the size of an encrypted Widevine keybox is returned.
 *
 * @param[out] p_wrapped_keybox Pointer to a buffer for the returned encrypted
 *             Widevine keybox. May be NULL if keybox_length is zero.
 *
 * @param[out] p_wrapped_keybox_length Length of the encrypted Widevine keybox
 *             in bytes.
 *
 * @param[in] p_transport_key Pointer to a transport key. May be NULL if
 *            transport_key_length is zero.
 *
 * @param[in] transport_key_length Length of the transport key in bytes.
 *
 * A Widevine keybox encrypted with the transport key is not supported.
 */
uint32_t drm_wv_wrap_keybox(const uint8_t * const p_keybox,
                            const uint32_t keybox_length,
                            uint8_t * const p_wrapped_keybox,
                            uint32_t * const p_wrapped_keybox_length,
                            const uint8_t * const p_transport_key,
                            const uint32_t transport_key_length);


/*
 * @brief Decrypts a wrapped Widevine keybox and installs it into the device.
 *
 * @param[in] p_keybox Pointer to a wrapped Widevine keybox.
 *
 * @param[in] keybox_length Length of the Widevine keybox in bytes.
 */
uint32_t drm_wv_install_keybox(const uint8_t * const p_keybox,
                               const uint32_t keybox_length);




#endif //__WV_DRM_API_H__
