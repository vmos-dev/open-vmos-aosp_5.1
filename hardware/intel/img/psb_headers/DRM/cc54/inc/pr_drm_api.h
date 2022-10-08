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

/**
 * @file drm_pr_api.h
 * @brief Header file for Playready DRM API
 */

#ifndef __DRM_PR_API_H__
#define __DRM_PR_API_H__

/*!
 * Defines
 */
#define PR_CLEAR_CONTENT_FLAG           (1)

#define DRM_SECURE_CLOCK_FLAG_RESET     (1)

/*!
 * Structs
 */

/*
 * This structure is used to provide necessary information for PlayReady video
 * ciphertext decryption.
 *
 * The members are:
 *
 *   iv                     - AES initialization vector.
 *   input_ciphertext_size  - Input ciphertext data size in bytes.
 *   p_input_ciphertext     - Pointer to the input ciphertext data.
 */
struct pr_drm_video_cipher
{
   uint32_t key_index;
   uint32_t key_type;
   uint64_t iv;
   uint64_t byte_offset;
   uint32_t input_ciphertext_size;
   uint8_t  *p_input_ciphertext;
   uint8_t  flags;
   uint8_t  *p_output_enc_ciphertext;
   uint32_t output_ciphertext_size;
};


struct drm_nalu_headers
{
   uint32_t frame_size;
   uint32_t parse_size;
   uint8_t  *p_enc_ciphertext;
   uint32_t hdrs_buf_len;
   uint8_t  *p_hdrs_buf;
};

struct pr_av_secure_input_data_buffer
{
    uint32_t  session_id;
    uint32_t  size;
    uint8_t   *data;
    uint32_t  clear;
};


/*
 *  Map to Oem_Hal_AllocateRegister
 */
uint32_t drm_pr_allocate_register(uint32_t key_type,
                                  uint32_t *key_reg_index);

/*
 *  Map to Oem_Hal_FreeRegister
 */
uint32_t drm_pr_free_register(uint32_t key_type,
                              uint32_t key_reg_index);

/*
 * Map to Oem_Hal_RegisterCount
 */
uint32_t drm_pr_register_count(uint32_t key_type,
                               uint32_t *total_regs,
                               uint32_t *allocated_regs);

/*
 * Map to Oem_Hal_GetPreloadedIndex
 */
uint32_t drm_pr_get_preloaded_index(uint32_t key_type,
                                    uint8_t *key_id,
                                    uint32_t key_id_len,
                                    uint32_t *key_index);
/*
 * Map to Oem_Hal_Initialize
 */
uint32_t drm_pr_initialize(void);

/*
 * Map to Oem_Hal_Deinitialize
 */
uint32_t drm_pr_deinitialize(void);

/*
 * Map to Oem_Hal_VerifyMessageSignature
 */
uint32_t drm_pr_verify_message_signature(uint8_t *msg,
                                         uint32_t msg_len,
                                         const uint8_t *signature,
                                         uint32_t signature_len,
                                         uint32_t hash_type,
                                         uint32_t signature_scheme,
                                         uint32_t integrity_key_index);

/*
 * Map to Oem_Hal_CreateMessageSignature
 */
uint32_t drm_pr_create_message_signature(uint8_t *msg,
                                         uint32_t msg_len,
                                         uint8_t *signature,
                                         uint32_t *signature_len,
                                         uint32_t hash_type,
                                         uint32_t signature_scheme,
                                         uint32_t key_type,
                                         uint32_t integrity_key_index);
/*
 * Map to Oem_Hal_VerifyOMAC1Signature
 */
 uint32_t drm_pr_verify_omac1_signature(uint8_t *msg,
                                        uint32_t msg_len,
                                        uint8_t *signature,
                                        uint32_t signature_len,
                                        uint32_t key_type,
                                        uint32_t index_key);

 /*
  * Map to Oem_Hal_CreateOMAC1Signature
  */
uint32_t drm_pr_create_omac1_signature(uint8_t *msg,
                                       uint32_t msg_len,
                                       uint8_t *signature,
                                       uint32_t *signature_len,
                                       uint32_t key_type,
                                       uint32_t index_key);

/*
 * Map to Oem_Hal_UnwrapKey
 */
uint32_t drm_pr_unwrap_get_keydata_type(uint32_t key_type,
                                        uint32_t key_index,
                                        uint32_t wrapping_key_type,
                                        uint32_t wrapping_key_index,
                                        uint32_t *key_data_type);

uint32_t drm_pr_unwrap_encrypted_key(uint32_t key_type,
                                     uint32_t key_index,
                                     uint32_t wrapping_key_type,
                                     uint32_t wrapping_key_index,
                                     uint8_t *encryption_key_info,
                                     uint32_t key_info_size,
                                     uint8_t *encrypted_key_data,
                                     uint32_t key_data_size);

uint32_t drm_pr_unwrap_xmr_license(uint32_t key_type,
                                   uint32_t key_index,
                                   uint32_t wrapping_key_type,
                                   uint32_t wrapping_key_index,
                                   uint8_t *xmr_license,
                                   uint32_t xmr_license_size,
                                   uint8_t xmr_ignore_cksum,
                                   uint8_t bb_ignore_cksum,
                                   uint8_t *bb_cksum_data,
                                   uint32_t bb_cksum_data_size,
                                   uint8_t *bb_kid,
                                   uint8_t bb_kid_size,
                                   uint8_t *bb_v1_kid,
                                   uint32_t bb_v1_kid_size);

uint32_t drm_pr_unwrap_key(uint32_t unwrap_key_type,
                           uint32_t unwrap_key_reg_index,
                           uint32_t wrapping_key_type,
                           uint32_t wrapping_key_index,
                           uint8_t *wrapped_key_data,
                           uint32_t wrapped_key_data_len,
                           const uint8_t *param_data,
                           uint32_t param_data_len);

/*
 * Map to Oem_Hal_WrapKey
 */
uint32_t drm_pr_wrap_get_encryption_type(uint32_t key_type,
                                         uint32_t key_index,
                                         uint32_t wrapping_key_type,
                                         uint32_t wrapping_key_index,
                                         uint32_t *encryption_type);

uint32_t drm_pr_wrap_encrypted_key(uint32_t key_type,
                                   uint32_t key_index,
                                   uint32_t wrapping_key_type,
                                   uint32_t wrapping_key_index,
                                   uint8_t *encryption_key_info,
                                   uint32_t key_info_size,
                                   uint8_t *encrypted_key_data,
                                   uint32_t encrypted_key_data_size);

uint32_t drm_pr_wrap_key(uint32_t wrap_key_type,
                         uint32_t wrap_key_reg_index,
                         uint32_t wrapping_key_type,
                         uint32_t wrapping_key_reg_index,
                         uint8_t *wrapped_key,
                         uint32_t *wrapped_key_len);

/*
 * Map to Oem_Hal_GenerateKey
 */
uint32_t drm_pr_generate_key(uint32_t key_type,
                             uint32_t key_reg_index,
                             uint32_t security_level);
/*
 * Map to Oem_Hal_LoadPlayReadyRevocationInfo
 */
uint32_t drm_pr_load_revoc_info(uint8_t *revoc_info,
                                uint32_t revoc_info_len,
                                uint32_t sign_key_reg_index);
/*
 * Map to Oem_Hal_LoadPlayReadyCrl
 */
uint32_t drm_pr_load_crl(uint8_t *crl,
                         uint32_t crl_len,
                         uint32_t sign_key_reg_index);
/*
 * Map to Oem_Hal_DecryptContentOpaque
 */
uint32_t drm_pr_decrypt_content_opaque(void *in_buffer,
                                       void *out_buffer,
                                       uint32_t data_len,
                                       uint32_t key_type,
                                       uint32_t key_index,
                                       uint64_t iv,
                                       uint64_t byte_offset);

uint32_t drm_pr_decrypt_content(uint8_t *out_buffer,
                                uint32_t data_len,
                                uint32_t key_type,
                                uint32_t key_index,
                                uint64_t iv,
                                uint64_t byte_offset);

/*!
 *@brief Create a PlayReady session
 *
 */
uint32_t drm_pr_create_session(uint32_t *session_id);

/*!
 *@brief Returns NALU header
 *
 */
uint32_t drm_pr_return_naluheaders(uint32_t session_id,
                                   struct drm_nalu_headers *nalu_info);

/*!
 *@brief Returns SRTC time
 *
 */
uint32_t drm_pr_get_srtc_time(uint32_t  *time,
                              uint32_t  *flags);
#endif
