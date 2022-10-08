/**
 * @file   tlcTeeKeymaster_if.h
 * @brief  Contains TEE Keymaster trustlet connector interface definitions
 *
 * Copyright Giesecke & Devrient GmbH 2012
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __TLCTEEKEYMASTERIF_H__
#define __TLCTEEKEYMASTERIF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>


/**
 * Key sizes
 */
#define TEE_RSA_KEY_SIZE_512   512
#define TEE_RSA_KEY_SIZE_1024  1024
#define TEE_RSA_KEY_SIZE_2048  2048


/* error codes */
typedef enum
{
    TEE_ERR_NONE             = 0,
    TEE_ERR_FAIL             = 1,
    TEE_ERR_INVALID_BUFFER   = 2,
    TEE_ERR_BUFFER_TOO_SMALL = 3,
    TEE_ERR_NOT_IMPLEMENTED  = 4,
    TEE_ERR_SESSION          = 5,
    TEE_ERR_MC_DEVICE        = 6,
    TEE_ERR_NOTIFICATION     = 7,
    TEE_ERR_MEMORY           = 8,
    TEE_ERR_MAP              = 9
    /* more can be added as required */
} teeResult_t;


/* RSA key pair types */
typedef enum {
    TEE_KEYPAIR_RSA       = 1,   /**< RSA public and RSA private key. */
    TEE_KEYPAIR_RSACRT    = 2    /**< RSA public and RSA CRT private key. */
} teeRsaKeyPairType_t;


/* Supported RSA signature algorithms */
typedef enum
{
    /* RSA */
    TEE_RSA_SHA_ISO9796           = 1, /**< 20-byte SHA-1 digest, padded according to the ISO 9796-2 scheme as specified in EMV '96 and EMV 2000, encrypted using RSA. */
    TEE_RSA_SHA_ISO9796_MR        = 2, /**< 20-byte SHA-1 digest, padded according to the ISO9796-2 specification and encrypted using RSA. */
    TEE_RSA_SHA_PKCS1             = 3, /**< 20-byte SHA-1 digest, padded according to the PKCS#1 (v1.5) scheme, and encrypted using RSA. */
    TEE_RSA_SHA256_PSS            = 4, /**< SHA-256 digest and PSS padding */
    TEE_RSA_SHA1_PSS              = 5, /**< SHA-256 digest and PSS padding */
    TEE_RSA_NODIGEST_NOPADDING    = 6, /**< No digest and padding */
} teeRsaSigAlg_t;


/* Digest types */
typedef enum
{
    TEE_DIGEST_SHA1,
    TEE_DIGEST_SHA256
} teeDigest_t;


/**
 * RSA private key metadata (Private modulus and exponent lengths)
 */
typedef struct {
    uint32_t     lenprimod;     /**< Private key modulus length */
    uint32_t     lenpriexp;     /**< Private key exponent length */
} teeRsaPrivKeyMeta_t;


/**
 * RSA CRT private key metadata (Private modulus and exponent lengths)
 */
typedef struct {
    uint32_t     lenprimod;     /**< Private key modulus length */
    uint32_t     lenp;          /**< Prime p length */
    uint32_t     lenq;          /**< Prime q length */
    uint32_t     lendp;         /**< DP length */
    uint32_t     lendq;         /**< DQ length */
    uint32_t     lenqinv;       /**< QP length */
} teeRsaCrtPrivKeyMeta_t;


/**
 * Key metadata (public key hash, key size, modulus/exponent lengths, etc..)
 */
typedef struct {
    uint32_t     keytype;       /**< Key type, e.g. RSA */
    uint32_t     keysize;       /**< Key size, e.g. 1024, 2048 */
    uint32_t     lenpubmod;     /**< Public key modulus length */
    uint32_t     lenpubexp;     /**< Public key exponent length */
    union {
        teeRsaPrivKeyMeta_t rsapriv;       /**< RSA private key */
        teeRsaCrtPrivKeyMeta_t rsacrtpriv; /**< RSA CRT private key */
    };
    uint32_t     rfu;          /**< Reserved for future use */
    uint32_t     rfulen;       /**< Reserved for future use */
} teeRsaKeyMeta_t;

/**
 * TEE_RSAGenerateKeyPair
 *
 * Generates RSA key pair and returns key pair data as wrapped object
 *
 * @param  keyType        [in]  Key pair type. RSA or RSACRT
 * @param  keyData        [in]  Pointer to the key data buffer
 * @param  keyDataLength  [in]  Key data buffer length
 * @param  keySize        [in]  Key size
 * @param  exponent       [in]  Exponent number
 * @param  soLen          [out] Key data secure object length
 */
teeResult_t TEE_RSAGenerateKeyPair(
    teeRsaKeyPairType_t keyType,
    uint8_t*            keyData,
    uint32_t            keyDataLength,
    uint32_t            keySize,
    uint32_t            exponent,
    uint32_t*           soLen);


/**
 * TEE_RSASign
 *
 * Signs given plain data and returns signature data
 *
 * @param  keyData          [in]  Pointer to key data buffer
 * @param  keyDataLength    [in]  Key data buffer length
 * @param  plainData        [in]  Pointer to plain data to be signed
 * @param  plainDataLength  [in]  Plain data length
 * @param  signatureData    [out] Pointer to signature data
 * @param  signatureDataLength  [out] Signature data length
 * @param  algorithm        [in]  RSA signature algorithm
 */
teeResult_t TEE_RSASign(
    const uint8_t*  keyData,
    const uint32_t  keyDataLength,
    const uint8_t*  plainData,
    const uint32_t  plainDataLength,
    uint8_t*        signatureData,
    uint32_t*       signatureDataLength,
    teeRsaSigAlg_t  algorithm);


/**
 * TEE_RSAVerify
 *
 * Verifies given data with RSA public key and return status
 *
 * @param  keyData          [in]  Pointer to key data buffer
 * @param  keyDataLength    [in]  Key data buffer length
 * @param  plainData        [in]  Pointer to plain data to be signed
 * @param  plainDataLength  [in]  Plain data length
 * @param  signatureData    [in]  Pointer to signed data
 * @param  signatureData    [in]  Plain  data length
 * @param  algorithm        [in]  RSA signature algorithm
 * @param  validity         [out] Signature validity
 */
teeResult_t TEE_RSAVerify(
    const uint8_t*  keyData,
    const uint32_t  keyDataLength,
    const uint8_t*  plainData,
    const uint32_t  plainDataLength,
    const uint8_t*  signatureData,
    const uint32_t  signatureDataLength,
    teeRsaSigAlg_t  algorithm,
    bool            *validity);


/**
 * TEE_HMACKeyGenerate
 *
 * Generates random key for HMAC calculation and returns key data as wrapped object
 * (key is encrypted)
 *
 * @param  keyData        [out] Pointer to key data
 * @param  keyDataLength  [in]  Key data buffer length
 * @param  soLen          [out] Key data secure object length
 */
teeResult_t TEE_HMACKeyGenerate(
    uint8_t*  keyData,
    uint32_t  keyDataLength,
    uint32_t* soLen);


/**
 * TEE_HMACSign
 *
 * Signs given plain data and returns HMAC signature data
 *
 * @param  keyData          [in]  Pointer to key data buffer
 * @param  keyDataLength    [in]  Key data buffer length
 * @param  plainData        [in]  Pointer to plain data to be signed
 * @param  plainDataLength  [in]  Plain data length
 * @param  signatureData    [out] Pointer to signature data
 * @param  signatureDataLength  [out] Signature data length
 * @param  digest           [in]  Digest type
 */
teeResult_t TEE_HMACSign(
    const uint8_t*  keyData,
    const uint32_t  keyDataLength,
    const uint8_t*  plainData,
    const uint32_t  plainDataLength,
    uint8_t*        signatureData,
    uint32_t*       signatureDataLength,
    teeDigest_t     digest);


/**
 * TEE_HMACVerify
 *
 * Verifies given data HMAC key data and return status
 *
 * @param  keyData          [in]  Pointer to key data buffer
 * @param  keyDataLength    [in]  Key data buffer length
 * @param  plainData        [in]  Pointer to plain data to be signed
 * @param  plainDataLength  [in]  Plain data length
 * @param  signatureData    [in]  Pointer to signed data
 * @param  signatureData    [in]  Plain  data length
 * @param  digest           [in]  Digest type
 * @param  validity         [out] Signature validity
 */
teeResult_t TEE_HMACVerify(
    const uint8_t*  keyData,
    const uint32_t  keyDataLength,
    const uint8_t*  plainData,
    const uint32_t  plainDataLength,
    const uint8_t*  signatureData,
    const uint32_t  signatureDataLength,
    teeDigest_t     digest,
    bool            *validity);


/**
 * TEE_KeyImport
 *
 * Imports key data and returns key data as secure object
 *
 * Key data needs to be in the following format
 *
 * RSA key data:
 * |--key metadata--|--public modulus--|--public exponent--|--private exponent--|
 *
 * RSA CRT key data:
 * |--key metadata--|--public modulus--|--public exponent--|--P--|--Q--|--DP--|--DQ--|--Qinv--|
 *
 * Where:
 * P:     secret prime factor
 * Q:     secret prime factor
 * DP:    d mod (p-1)
 * DQ:    d mod (q-1)
 * Qinv:  q^-1 mod p
 *
 * @param  keyData          [in]  Pointer to key data
 * @param  keyDataLength    [in]  Key data length
 * @param  soData           [out] Pointer to wrapped key data
 * @param  soDataLength     [out] Wrapped key data length
 */
teeResult_t TEE_KeyImport(
    const uint8_t*  keyData,
    const uint32_t  keyDataLength,
    uint8_t*        soData,
    uint32_t*       soDataLength);


/**
 * TEE_GetPubKey
 *
 * Retrieves public key daya (modulus and exponent) from wrapped key data
 *
 * @param  keyData          [in]  Pointer to key data
 * @param  keyDataLength    [in]  Key data length
 * @param  modulus          [out] Pointer to public key modulus data
 * @param  modulusLength    [out] Modulus data length
 * @param  exponent         [out] Pointer to public key exponent data
 * @param  exponentLength   [out] Exponent data length
 */
teeResult_t TEE_GetPubKey(
    const uint8_t*  keyData,
    const uint32_t  keyDataLength,
    uint8_t*        modulus,
    uint32_t*       modulusLength,
    uint8_t*        exponent,
    uint32_t*       exponentLength);


#ifdef __cplusplus
}
#endif

#endif // __TLCTEEKEYMASTERIF_H__
