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

#ifndef __WV_MOD_DRM_API_H_
#define __WV_MOD_DRM_API_H_

#include <inttypes.h>
#include "drm_common_api.h"
#include "wv_mod_oem_crypto.h"

/*!
 * Defines
 */
#define DRM_WV_MOD_CLEAR_CONTENT_FLAG  (1)
#define DRM_WV_MOD_AUDIO_CONTENT_FLAG  (1 << 1)
#define DRM_WV_MOD_SECURE_CONTENT_FLAG (1 << 2)

/*!
 * APIs
 */
uint32_t drm_wv_mod_open_session(uint32_t *session_id);

uint32_t drm_wv_mod_close_session(uint32_t session_id);

uint32_t drm_wv_mod_start_playback(uint32_t session_id);

uint32_t drm_wv_mod_stop_playback(uint32_t session_id);

uint32_t drm_wv_mod_generate_derived_keys(uint32_t session_id,
                                          const uint8_t *mac_key_context,
                                          uint32_t mac_key_context_length,
                                          const uint8_t *enc_key_context,
                                          uint32_t enc_key_context_length);

uint32_t drm_wv_mod_generate_nonce(uint32_t session_id,
                                   uint32_t *nonce);

uint32_t drm_wv_mod_generate_signature(uint32_t session_id,
                                       const uint8_t *message,
                                       uint32_t message_length,
                                       uint8_t *signature,
                                       uint32_t *signature_length);

uint32_t drm_wv_mod_load_keys(uint32_t session_id,
                              const uint8_t *message,
                              uint32_t message_length,
                              const uint8_t *signature,
                              uint32_t signature_length,
                              const uint8_t *enc_mac_keys_iv,
                              const uint8_t *enc_mac_keys,
                              uint32_t num_keys,
                              const struct drm_wv_mod_key_object *key_array);

uint32_t drm_wv_mod_refresh_keys(uint32_t session_id,
                                 const uint8_t *message,
                                 uint32_t message_length,
                                 const uint8_t *signature,
                                 uint32_t signature_length,
                                 uint32_t num_keys,
                                 const struct drm_wv_mod_key_refresh_object *key_array);

uint32_t drm_wv_mod_select_key(uint32_t session_id,
                               const uint8_t *key_id,
                               uint32_t key_id_length);

uint32_t drm_wv_mod_decrypt_ctr(uint32_t session_id,
                                const uint8_t *inp_data_buffer,
                                uint32_t inp_data_size,
                                uint8_t *out_data_buffer,
                                uint32_t out_data_size,
                                const uint8_t *iv,
                                uint8_t flags);

uint32_t drm_wv_mod_rewrap_device_rsa_key(uint32_t session_id,
                                          const uint8_t *message,
                                          uint32_t message_length,
                                          const uint8_t *signature,
                                          uint32_t signature_length,
                                          uint32_t *nonce,
                                          const uint8_t *enc_rsa_key,
                                          uint32_t enc_rsa_key_length,
                                          const uint8_t *enc_rsa_key_iv,
                                          uint8_t *wrapped_rsa_key,
                                          uint32_t *wrapped_rsa_key_length);

uint32_t drm_wv_mod_load_device_rsa_key(uint32_t session_id,
                                        const uint8_t *wrapped_rsa_key,
                                        uint32_t wrapped_rsa_key_length);

uint32_t drm_wv_mod_generate_rsa_signature(uint32_t session_id,
                                           const uint8_t *message,
                                           uint32_t message_length,
                                           uint8_t *signature,
                                           uint32_t *signature_length);

uint32_t drm_wv_mod_derive_keys_from_session_key(uint32_t session_id,
                                                 const uint8_t *enc_session_key,
                                                 uint32_t enc_session_key_length,
                                                 const uint8_t *mac_key_context,
                                                 uint32_t mac_key_context_length,
                                                 const uint8_t *enc_key_context,
                                                 uint32_t enc_key_context_length);

uint32_t drm_wv_mod_generic_encrypt(uint32_t session_id,
                                    const uint8_t *in_buffer,
                                    uint32_t buffer_size,
                                    const uint8_t *iv,
                                    enum drm_wv_mod_algorithm algorithm,
                                    uint8_t *out_buffer);

uint32_t drm_wv_mod_generic_decrypt(uint32_t session_id,
                                    const uint8_t *in_buffer,
                                    uint32_t buffer_size,
                                    const uint8_t *iv,
                                    enum drm_wv_mod_algorithm algorithm,
                                    uint8_t *out_buffer);

uint32_t drm_wv_mod_generic_sign(uint32_t session_id,
                                 const uint8_t *in_buffer,
                                 uint32_t buffer_size,
                                 enum drm_wv_mod_algorithm algorithm,
                                 uint8_t *signature,
                                 uint32_t *signature_size);

uint32_t drm_wv_mod_generic_verify(uint32_t session_id,
                                   const uint8_t *in_buffer,
                                   uint32_t buffer_size,
                                   enum drm_wv_mod_algorithm algorithm,
                                   const uint8_t *signature,
                                   uint32_t signature_size);

/*! Version 9 specific APIs */
uint32_t drm_wv_mod_v9_load_keys(uint32_t session_id,
                                 const uint8_t *message,
                                 uint32_t message_length,
                                 const uint8_t *signature,
                                 uint32_t signature_length,
                                 const uint8_t *enc_mac_keys_iv,
                                 const uint8_t *enc_mac_keys,
                                 uint32_t num_keys,
                                 const struct drm_wv_mod_key_object *key_array,
                                 const uint8_t *pst,
                                 uint32_t pst_length);

uint32_t drm_wv_mod_v9_generate_rsa_signature(uint32_t session_id,
                                              const uint8_t *message,
                                              uint32_t message_length,
                                              uint8_t *signature,
                                              uint32_t *signature_length,
                                              enum drm_wv_mod_rsa_padding_scheme padding_scheme);


/**
 * @brief Loads an existing usage table into chaabi secure memory
 *
 * This should be first called prior to load keys. Caller shall call
 * drm_wv_mod_update_usage_table after making this call.
 *
 * @param[in] usage_table_data
 *    Existing usage table blob to load. If NULL, chaabi will
 *    return required table size.
 * @param[in,out] data_size
 *    Size of the passed-in usage_table_data, in bytes. This
 *    will always be updated to the required table size.
 */
uint32_t drm_wv_mod_load_usage_table(const uint8_t *const usage_table_data,
                                     uint32_t *const data_size);

/* @brief Update usage table and return it
 *
 * Chaabi will update the usage table from its TEE memory and set the flag
 * is_updated.
 *
 * Upon returning DRM_WV_MOD_SUCCESS and is_updated == 1, caller should
 * write save the usage table to the file system.
 *
 * @param[out] usage_table_data
 *   Buffer where the usage table will be returned. Input is ignored.
 *   This will only contain data if is_updated == 1.
 * @param[in] data_size
 *   Size of the usage_table_data buffer, which must be large enough to
 *   hold the entire structure. This size can be obtained via the
 *   load_usage_table API or by re-using the size of a previous table blob.
 * @param[out] is_updated
 *   Flag indicating if the table has changed since the last
 *   update_usage_table or load_usage_table call.
 *
 * TODO: Return documentation
 */
uint32_t drm_wv_mod_update_usage_table(uint8_t *const usage_table_data,
                                       uint32_t data_size,
                                       uint8_t *const is_updated);

// NOTE: drm_wv_mod_update_usage_table shall be called after calling this
// function
// TODO: Documentation
uint32_t drm_wv_mod_deactivate_usage_entry(const uint8_t *const pst,
                                           uint32_t pst_length);

/**
 * @brief Returns the usage entry information for a particular pst
 *
 * Caller shall call drm_wv_mod_update_usage_table after making this call.
 *
 * @param[in] session_id
 *   Session ID to be associated with the pst entry
 * @param[in] pst
 *   Pointer to pst data used as an index into the usage table
 * @param[in] pst_length
 *   Length of pst buffer in bytes
 * @param[out] pst_report_buffer
 *   Pointer to caller-allocated memory where the usage report shall be placed
 * @param[in,out] pst_report_buffer_length
 *   Length of provided pst_report_buffer in bytes. Should be sizeof(pst) +
 *   sizeof(struct OEMCrypto_PST_Report) in length. If extra space is provided,
 *   this field will reflect the actual size of the returned report.
 *
 * TODO: Return documentation
 */
uint32_t drm_wv_mod_report_usage(uint32_t session_id,
                                 const uint8_t *const pst,
                                 uint32_t pst_length,
                                 uint8_t *const pst_report_buffer,
                                 uint32_t *const pst_report_buffer_length);

/**
 * @brief Deletes a previously-reported entry from the usage table
 *
 * Caller shall call drm_wv_mod_update_usage_table after making this call.
 *
 * @param[in] session_id
 *   Session ID previously associated with the pst
 * @param[in] pst
 *   Pointer to pst data used as an index into the usage table
 * @param[in] pst_length
 *   Length of pst buffer in bytes
 * @param[in] msg
 *   Pointer to message to be verified
 * @param[in] msg_len
 *   Length of msg buffer in bytes
 * @param[in] signature
 *   Pointer to signature to verify against
 * @param[in] signature_length
 *   Length of signature buffer in bytes
 *
 * TODO: Return Documentation
 */
uint32_t drm_wv_mod_delete_usage_entry(uint32_t session_id,
                                       const uint8_t *const pst,
                                       uint32_t pst_length,
                                       const uint8_t *const msg,
                                       uint32_t msg_length,
                                       const uint8_t *const signature,
                                       uint32_t signature_length);

// This will only clear Chaabi TEE memory. Caller is responsible for deleting
// usage table file from file system.
// TODO: Documentation
uint32_t drm_wv_mod_delete_usage_table(void);

/**
 * brief Clear session context
 *
 * This API is used to reset all sessions context.
 * Typically called to cleanup sessions resulting from a application crash.
 */
uint32_t drm_wv_mod_reset_session_context(void);

#endif /* __WV_MOD_DRM_API_H_ */
