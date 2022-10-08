/*
 *  Copyright (C) 2012 The Android Open Source Project
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you
 *  may not use this file except in compliance with the License.  You may
 *  obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 *  implied.  See the License for the specific language governing
 *  permissions and limitations under the License.
 */

#ifndef ANDROID_HARDWARE_QCOM_KEYMASTER_H
#define ANDROID_HARDWARE_QCOM_KEYMASTER_H

#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/types.h>

__BEGIN_DECLS

#ifdef _ION_HEAP_MASK_COMPATIBILITY_WA
#define ION_HEAP_MASK heap_mask
#else
#define ION_HEAP_MASK heap_id_mask
#endif

/**
 * The id of this module
 */
#define QCOM_KEYSTORE_KEYMASTER "qcom_keymaster"
/**
 * Operation result
 */
#define KEYMATER_SUCCESS  0
#define KEYMASTER_FAILURE  -1

/**
 * The API level of this version of the header. The allows the implementing
 * module to recognize which API level of the client it is dealing with in
 * the case of pre-compiled binary clients.
 */
#define QCOM_KEYMASTER_API_VERSION KEYMASTER_MODULE_API_VERSION_0_3

#define KM_MAGIC_NUM     (0x4B4D4B42)    /* "KMKB" Key Master Key Blob in hex */
#define KM_KEY_SIZE_MAX  (512)           /* 4096 bits */
#define KM_IV_LENGTH     (16)            /* AES128 CBC IV */
#define KM_HMAC_LENGTH   (32)            /* SHA2 will be used for HMAC  */

struct  qcom_km_key_blob {
  uint32_t magic_num;
  uint32_t version_num;
  uint8_t  modulus[KM_KEY_SIZE_MAX];
  uint32_t modulus_size;
  uint8_t  public_exponent[KM_KEY_SIZE_MAX];
  uint32_t public_exponent_size;
  uint8_t  iv[KM_IV_LENGTH];
  uint8_t  encrypted_private_exponent[KM_KEY_SIZE_MAX];
  uint32_t encrypted_private_exponent_size;
  uint8_t  hmac[KM_HMAC_LENGTH];
};
typedef struct  qcom_km_key_blob qcom_km_key_blob_t;
/**
 * Commands supported
 */
enum  keymaster_cmd_t {
    /*
     * List the commands supportedin by the hardware.
     */
    KEYMASTER_GENERATE_KEYPAIR = 0x00000001,
    KEYMASTER_IMPORT_KEYPAIR = 0x00000002,
    KEYMASTER_SIGN_DATA = 0x00000003,
    KEYMASTER_VERIFY_DATA = 0x00000004,
};


/**
 * Command to Generate a public and private key. The key data returned
 * (by secure app) is in shared buffer at offset of "key_blob" and is opaque
 *
 * cmd_id       : Command issue to secure app
 * key_type     : Currently on RSA_TYPE is supported
 * rsa_params   : Parameters needed to generate an RSA key
 */
 struct keymaster_gen_keypair_cmd {
      keymaster_cmd_t               cmd_id;
      keymaster_keypair_t           key_type;
      keymaster_rsa_keygen_params_t rsa_params;
};
typedef struct keymaster_gen_keypair_cmd keymaster_gen_keypair_cmd_t;

/**
 * Response to Generate a public and private key. The key data returned
 * (by secure app) is in shared buffer at offset of "key_blob" and is opaque
 *
 * cmd_id       : Command issue to secure app
 * key_blob     : key blob data
 * key_blob_len : Total length of key blob information
 * status       : Result (success 0, or failure -1)
 */
struct keymaster_gen_keypair_resp {
      keymaster_cmd_t     cmd_id;
      qcom_km_key_blob_t  key_blob;
      size_t              key_blob_len;
      int32_t             status;
};
typedef struct keymaster_gen_keypair_resp keymaster_gen_keypair_resp_t;


/**
 * Command to import a public and private key pair. The imported keys
 * will be in PKCS#8 format with DER encoding (Java standard). The key
 * data returned (by secure app) is in shared buffer at offset of
 * "key_blob" and is opaque
 *
 * cmd_id       : Command issue to secure app
 * pkcs8_key    : Pointer to  pkcs8 formatted key information
 * pkcs8_key_len: PKCS8 formatted key length
 */
struct keymaster_import_keypair_cmd {
      keymaster_cmd_t cmd_id;
      uint32_t        pkcs8_key;
      size_t          pkcs8_key_len;
};
typedef struct keymaster_import_keypair_cmd keymaster_import_keypair_cmd_t;

/**
 * Response to import a public and private key. The key data returned
 * (by secure app) is in shared buffer at offset of "key_blob" and is opaque
 *
 * cmd_id       : Command issue to secure app
 * key_blob     : key blob data
 * key_blob_len : Total length of key blob information
 * status       : Result (success 0, or failure -1)
 */
struct keymaster_import_keypair_resp {
      keymaster_cmd_t     cmd_id;
      qcom_km_key_blob_t  key_blob;
      size_t              key_blob_len;
      int32_t             status;
};
typedef struct keymaster_import_keypair_resp keymaster_import_keypair_resp_t;

/**
 * Command to sign data using a key info generated before. This can use either
 * an asymmetric key or a secret key.
 * The signed data is returned (by secure app) at offset of data + dlen.
 *
 * cmd_id      : Command issue to secure app
 * sign_param  :
 * key_blob    : Key data information (in shared buffer)
 * data        : Pointer to plain data buffer
 * dlen        : Plain data length
 */
struct keymaster_sign_data_cmd {
      keymaster_cmd_t               cmd_id;
      keymaster_rsa_sign_params_t   sign_param;
      qcom_km_key_blob_t            key_blob;
      uint32_t                      data;
      size_t                        dlen;
};
typedef struct keymaster_sign_data_cmd keymaster_sign_data_cmd_t;

/**
 * Response to sign data response
 *
 * cmd_id      : Command issue to secure app
 * signed_data : signature
 * sig_len     : Signed data length
 * status      : Result (success 0, or failure -1)
 */
struct keymaster_sign_data_resp {
      keymaster_cmd_t     cmd_id;
      uint8_t             signed_data[KM_KEY_SIZE_MAX];
      size_t              sig_len;
      int32_t             status;
};

typedef struct keymaster_sign_data_resp keymaster_sign_data_resp_t;

/**
 * Command to verify data using a key info generated before. This can use either
 * an asymmetric key or a secret key.
 *
 * cmd_id      : Command issue to secure app
 * sign_param  :
 * key_blob    : Key data information (in shared buffer)
 * key_blob_len: Total key length
 * signed_data : Pointer to signed data buffer
 * signed_dlen : Signed data length
 * signature   : Offset to the signature data buffer (from signed data buffer)
 * slen        : Signature data length
 */
struct keymaster_verify_data_cmd {
      keymaster_cmd_t cmd_id;
      keymaster_rsa_sign_params_t   sign_param;
      qcom_km_key_blob_t            key_blob;
      uint32_t                      signed_data;
      size_t                        signed_dlen;
      uint32_t                      signature;
      size_t                        slen;
};
typedef struct keymaster_verify_data_cmd keymaster_verify_data_cmd_t;
/**
 * Response to verify data
 *
 * cmd_id      : Command issue to secure app
 * status      : Result (success 0, or failure -1)
 */
struct  keymaster_verify_data_resp {
      keymaster_cmd_t     cmd_id;
      int32_t             status;
};
typedef struct keymaster_verify_data_resp keymaster_verify_data_resp_t;

__END_DECLS

#endif  // ANDROID_HARDWARE_QCOM_KEYMASTER_H
