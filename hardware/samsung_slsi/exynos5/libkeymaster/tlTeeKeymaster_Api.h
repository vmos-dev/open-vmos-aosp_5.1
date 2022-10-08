/**
 * @file   tlTeeKeymaster_Api.h
 * @brief  Contains TCI command definitions and data structures
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

#ifndef __TLTEEKEYMASTERAPI_H__
#define __TLTEEKEYMASTERAPI_H__

#include "tci.h"



/**
 * Command ID's
 */
#define CMD_ID_TEE_RSA_GEN_KEY_PAIR   1
#define CMD_ID_TEE_RSA_SIGN           2
#define CMD_ID_TEE_RSA_VERIFY         3
#define CMD_ID_TEE_HMAC_GEN_KEY       4
#define CMD_ID_TEE_HMAC_SIGN          5
#define CMD_ID_TEE_HMAC_VERIFY        6
#define CMD_ID_TEE_KEY_IMPORT         7
#define CMD_ID_TEE_GET_PUB_KEY        8
/*... add more command ids when needed */


/**
 * Command message.
 *
 * @param len Length of the data to process.
 * @param data Data to be processed
 */
typedef struct {
    tciCommandHeader_t  header;     /**< Command header */
    uint32_t            len;        /**< Length of data to process */
} command_t;


/**
 * Response structure
 */
typedef struct {
    tciResponseHeader_t header;     /**< Response header */
    uint32_t            len;
} response_t;


/**
 * Generate key data
 * Response data contains generated RSA key pair data is
 * wrapped as below:
 *
 * |-- Key metadata --|-- Public key (plaintext) --|-- Private key (encrypted) --|
 */
typedef struct {
    uint32_t type;           /**< Key pair type. RSA or RSACRT */
    uint32_t keysize;        /**< Key size in bits, e.g. 1024, 2048,.. */
    uint32_t exponent;       /**< Exponent number */
    uint32_t keydata;        /**< Key data buffer passed by TLC  */
    uint32_t keydatalen;     /**< Length of key data buffer */
    uint32_t solen;          /**< Secure object length  (of key data) (provided by the trustlet)  */
} rsagenkey_t;


/**
 *  RSA sign data structure
 */
typedef struct {
    uint32_t keydata;           /**< Key data buffer */
    uint32_t keydatalen;        /**< Length of key data buffer */
    uint32_t plaindata;         /**< Plaintext data buffer */
    uint32_t plaindatalen;      /**< Length of plaintext data buffer */
    uint32_t signaturedata;     /**< Signature data buffer */
    uint32_t signaturedatalen;  /**< Length of signature data buffer */
    uint32_t algorithm;         /**< Signing algorithm */
} rsasign_t;


/**
 *  RSA signature verify data structure
 */
typedef struct {
    uint32_t keydata;           /**< Key data buffer */
    uint32_t keydatalen;        /**< Length of key data buffer */
    uint32_t plaindata;         /**< Plaintext data buffer */
    uint32_t plaindatalen;      /**< Length of plaintext data buffer */
    uint32_t signaturedata;     /**< Signature data buffer */
    uint32_t signaturedatalen;  /**< Length of signature data buffer */
    uint32_t algorithm;         /**< Signing algorithm */
    bool     validity;          /**< Signature validity */
} rsaverify_t;


/**
 * Generate HMAC key data
 * Response data contains generated HMAC key data that is
 * wrapped as below:
 *
 * |-- HMAC key (encrypted) --|
 */
typedef struct {
    uint32_t keydata;        /**< Key data buffer passed by TLC  */
    uint32_t keydatalen;     /**< Length of key data buffer */
    uint32_t solen;          /**< Secure object length  (of key data) (provided by the trustlet)  */
} hmacgenkey_t;


/**
 *  HMAC sign data structure
 */
typedef struct {
    uint32_t keydata;           /**< Key data buffer */
    uint32_t keydatalen;        /**< Length of key data buffer */
    uint32_t plaindata;         /**< Plaintext data buffer */
    uint32_t plaindatalen;      /**< Length of plaintext data buffer */
    uint32_t signaturedata;     /**< Signature data buffer */
    uint32_t signaturedatalen;  /**< Length of signature data buffer */
    uint32_t digest;            /**< Digest algorithm */
} hmacsign_t;


/**
 *  HMAC signature verify data structure
 */
typedef struct {
    uint32_t keydata;           /**< Key data buffer */
    uint32_t keydatalen;        /**< Length of key data buffer */
    uint32_t plaindata;         /**< Plaintext data buffer */
    uint32_t plaindatalen;      /**< Length of plaintext data buffer */
    uint32_t signaturedata;     /**< Signature data buffer */
    uint32_t signaturedatalen;  /**< Length of signature data buffer */
    uint32_t digest;            /**< Digest algorithm */
    bool     validity;          /**< Signature validity */
} hmacverify_t;

/**
 * RSA private key metadata
 */
typedef struct {
    uint32_t     lenpriexp;     /**< Private key exponent length */
} rsaprivkeymeta_t;


/**
 * RSA CRT private key metadata
 */
typedef struct {
    uint32_t     lenp;          /**< Prime p length */
    uint32_t     lenq;          /**< Prime q length */
    uint32_t     lendp;         /**< DP length */
    uint32_t     lendq;         /**< DQ length */
    uint32_t     lenqinv;       /**< QP length */
} rsacrtprivkeymeta_t;


/**
 * Key metadata (key size, modulus/exponent lengths, etc..)
 */
typedef struct {
    uint32_t     keytype;          /**< RSA key pair type. RSA or RSA CRT */
    uint32_t     keysize;          /**< RSA key size */
    uint32_t     lenpubmod;        /**< Public key modulus length */
    uint32_t     lenpubexp;        /**< Public key exponent length */
    union {
        rsaprivkeymeta_t    rsapriv;    /**< RSA private key */
        rsacrtprivkeymeta_t rsacrtpriv; /**< RSA CRT private key */
    };
    uint32_t     rfu;          /**< Reserved for future use */
    uint32_t     rfulen;       /**< Reserved for future use */
} rsakeymeta_t;

/**
 *  Key import data structure
 */
typedef struct {
    uint32_t     keydata;           /**< Key data buffer */
    uint32_t     keydatalen;        /**< Length of key data buffer */
    uint32_t     sodata;            /**< Wrapped buffer */
    uint32_t     sodatalen;         /**< Length of wrapped data buffer */
} keyimport_t;


/**
 *  Get public key data structure
 */
typedef struct {
    uint32_t type;              /**< Key type */
    uint32_t keydata;           /**< Key data buffer */
    uint32_t keydatalen;        /**< Length of key data buffer */
    uint32_t modulus;           /**< Modulus */
    uint32_t moduluslen;        /**< Modulus length */
    uint32_t exponent;          /**< Exponent */
    uint32_t exponentlen;       /**< Exponent length */
} getpubkey_t;


/**
 * TCI message data.
 */
typedef struct {
    union {
        command_t     command;
        response_t    response;
    };

    union {
        rsagenkey_t  rsagenkey;
        rsasign_t    rsasign;
        rsaverify_t  rsaverify;
        hmacgenkey_t hmacgenkey;
        hmacsign_t   hmacsign;
        hmacverify_t hmacverify;
        keyimport_t  keyimport;
        getpubkey_t  getpubkey;
    };

} tciMessage_t, *tciMessage_ptr;


/**
 * Overall TCI structure.
 */
typedef struct {
    tciMessage_t message;   /**< TCI message */
} tci_t;


/**
 * Trustlet UUID
 */
#define TEE_KEYMASTER_TL_UUID { { 7, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }


#endif // __TLTEEKEYMASTERAPI_H__
