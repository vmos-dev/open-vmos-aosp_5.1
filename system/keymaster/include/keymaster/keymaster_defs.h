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

#ifndef ANDROID_HARDWARE_KEYMASTER_DEFS_H
#define ANDROID_HARDWARE_KEYMASTER_DEFS_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(__cplusplus)
extern "C" {
#endif  // defined(__cplusplus)

/**
 * Authorization tags each have an associated type.  This enumeration facilitates tagging each with
 * a type, by using the high four bits (of an implied 32-bit unsigned enum value) to specify up to
 * 16 data types.  These values are ORed with tag IDs to generate the final tag ID values.
 */
typedef enum {
    KM_INVALID = 0 << 28, /* Invalid type, used to designate a tag as uninitialized */
    KM_ENUM = 1 << 28,
    KM_ENUM_REP = 2 << 28, /* Repeatable enumeration value. */
    KM_INT = 3 << 28,
    KM_INT_REP = 4 << 28, /* Repeatable integer value */
    KM_LONG = 5 << 28,
    KM_DATE = 6 << 28,
    KM_BOOL = 7 << 28,
    KM_BIGNUM = 8 << 28,
    KM_BYTES = 9 << 28,
} keymaster_tag_type_t;

typedef enum {
    KM_TAG_INVALID = KM_INVALID | 0,

    /*
     * Tags that must be semantically enforced by hardware and software implementations.
     */

    /* Crypto parameters */
    KM_TAG_PURPOSE = KM_ENUM_REP | 1, /* keymaster_purpose_t. */
    KM_TAG_ALGORITHM = KM_ENUM | 2,   /* keymaster_algorithm_t. */
    KM_TAG_KEY_SIZE = KM_INT | 3,     /* Key size in bits. */
    KM_TAG_BLOCK_MODE = KM_ENUM | 4,  /* keymaster_block_mode_t. */
    KM_TAG_DIGEST = KM_ENUM | 5,      /* keymaster_digest_t. */
    KM_TAG_MAC_LENGTH = KM_INT | 6,   /* MAC length in bits. */
    KM_TAG_PADDING = KM_ENUM | 7,     /* keymaster_padding_t. */
    KM_TAG_CHUNK_LENGTH = KM_INT | 8, /* AEAD mode minimum decryption chunk size, in bytes. */

    /* Other hardware-enforced. */
    KM_TAG_RESCOPING_ADD = KM_ENUM_REP | 101, /* Tags authorized for addition via rescoping. */
    KM_TAG_RESCOPING_DEL = KM_ENUM_REP | 102, /* Tags authorized for removal via rescoping. */
    KM_TAG_BLOB_USAGE_REQUIREMENTS = KM_ENUM | 705, /* keymaster_key_blob_usage_requirements_t */

    /* Algorithm-specific. */
    KM_TAG_RSA_PUBLIC_EXPONENT = KM_LONG | 200, /* Defaults to 2^16+1 */
    KM_TAG_DSA_GENERATOR = KM_BIGNUM | 201,
    KM_TAG_DSA_P = KM_BIGNUM | 202,
    KM_TAG_DSA_Q = KM_BIGNUM | 203,
    /* Note there are no EC-specific params.  Field size is defined by KM_TAG_KEY_SIZE, and the
       curve is chosen from NIST recommendations for field size */

    /*
     * Tags that should be semantically enforced by hardware if possible and will otherwise be
     * enforced by software (keystore).
     */

    /* Key validity period */
    KM_TAG_ACTIVE_DATETIME = KM_DATE | 400,             /* Start of validity */
    KM_TAG_ORIGINATION_EXPIRE_DATETIME = KM_DATE | 401, /* Date when new "messages" should no
                                                           longer be created. */
    KM_TAG_USAGE_EXPIRE_DATETIME = KM_DATE | 402,       /* Date when existing "messages" should no
                                                           longer be trusted. */
    KM_TAG_MIN_SECONDS_BETWEEN_OPS = KM_INT | 403,      /* Minimum elapsed time between
                                                           cryptographic operations with the key. */
    KM_TAG_SINGLE_USE_PER_BOOT = KM_BOOL | 404,         /* If true, the key can only be used once
                                                           per boot. */

    /* User authentication */
    KM_TAG_ALL_USERS = KM_BOOL | 500, /* If key is usable by all users. */
    KM_TAG_USER_ID = KM_INT | 501,    /* ID of authorized user.  Disallowed if KM_TAG_ALL_USERS is
                                         present. */
    KM_TAG_NO_AUTH_REQUIRED = KM_BOOL | 502,    /* If key is usable without authentication. */
    KM_TAG_USER_AUTH_ID = KM_INT_REP | 503,     /* ID of the authenticator to use (e.g. password,
                                                   fingerprint, etc.).  Repeatable to support
                                                   multi-factor auth.  Disallowed if
                                                   KM_TAG_NO_AUTH_REQUIRED is present. */
    KM_TAG_AUTH_TIMEOUT = KM_INT | 504,         /* Required freshness of user authentication for
                                                   private/secret key operations, in seconds.
                                                   Public key operations require no authentication.
                                                   If absent, authentication is required for every
                                                   use.  Authentication state is lost when the
                                                   device is powered off. */
    KM_TAG_RESCOPE_AUTH_TIMEOUT = KM_INT | 505, /* Required freshness of user authentication for key
                                                   rescoping operations, in seconds.  Public key
                                                   operations require no authentication.  If absent,
                                                   authentication required for every rescoping. */

    /* Application access control */
    KM_TAG_ALL_APPLICATIONS = KM_BOOL | 600, /* If key is usable by all applications. */
    KM_TAG_APPLICATION_ID = KM_BYTES | 601,  /* ID of authorized application. Disallowed if
                                                KM_TAG_ALL_APPLICATIONS is present. */

    /*
     * Semantically unenforceable tags, either because they have no specific meaning or because
     * they're informational only.
     */
    KM_TAG_APPLICATION_DATA = KM_BYTES | 700,  /* Data provided by authorized application. */
    KM_TAG_CREATION_DATETIME = KM_DATE | 701,  /* Key creation time */
    KM_TAG_ORIGIN = KM_ENUM | 702,             /* keymaster_key_origin_t. */
    KM_TAG_ROLLBACK_RESISTANT = KM_BOOL | 703, /* Whether key is rollback-resistant. */
    KM_TAG_ROOT_OF_TRUST = KM_BYTES | 704,     /* Root of trust ID.  Empty array means usable by all
                                                  roots. */

    /* Tags used only to provide data to operations */
    KM_TAG_ADDITIONAL_DATA = KM_BYTES | 1000, /* Used to provide additional data for AEAD modes. */
} keymaster_tag_t;

/**
 * Algorithms that may be provided by keymaster implementations.  Those that must be provided by all
 * implementations are tagged as "required".  Note that where the values in this enumeration overlap
 * with the values for the deprecated keymaster_keypair_t, the same algorithm must be
 * specified. This type is new in 0_4 and replaces the deprecated keymaster_keypair_t.
 */
typedef enum {
    /* Asymmetric algorithms. */
    KM_ALGORITHM_RSA = 1,   /* required */
    KM_ALGORITHM_DSA = 2,   /* required */
    KM_ALGORITHM_ECDSA = 3, /* required */
    KM_ALGORITHM_ECIES = 4,
    /* FIPS Approved Ciphers */
    KM_ALGORITHM_AES = 32, /* required */
    KM_ALGORITHM_3DES = 33,
    KM_ALGORITHM_SKIPJACK = 34,
    /* AES Finalists */
    KM_ALGORITHM_MARS = 48,
    KM_ALGORITHM_RC6 = 49,
    KM_ALGORITHM_SERPENT = 50,
    KM_ALGORITHM_TWOFISH = 51,
    /* Other common block ciphers */
    KM_ALGORITHM_IDEA = 52,
    KM_ALGORITHM_RC5 = 53,
    KM_ALGORITHM_CAST5 = 54,
    KM_ALGORITHM_BLOWFISH = 55,
    /* Common stream ciphers */
    KM_ALGORITHM_RC4 = 64,
    KM_ALGORITHM_CHACHA20 = 65,
    /* MAC algorithms */
    KM_ALGORITHM_HMAC = 128, /* required */
} keymaster_algorithm_t;

/**
 * Symmetric block cipher modes that may be provided by keymaster implementations.  Those that must
 * be provided by all implementations are tagged as "required".  This type is new in 0_4.
 */
typedef enum {
    /* Unauthenticated modes, usable only for encryption/decryption and not generally recommended
     * except for compatibility with existing other protocols. */
    KM_MODE_ECB = 1,     /* required */
    KM_MODE_CBC = 2,     /* required */
    KM_MODE_CBC_CTS = 3, /* recommended */
    KM_MODE_CTR = 4,     /* recommended */
    KM_MODE_OFB = 5,
    KM_MODE_CFB = 6,
    KM_MODE_XTS = 7, /* Note: requires double-length keys */
    /* Authenticated modes, usable for encryption/decryption and signing/verification.  Recommended
     * over unauthenticated modes for all purposes.  One of KM_MODE_GCM and KM_MODE_OCB is
     * required. */
    KM_MODE_GCM = 32,
    KM_MODE_OCB = 33,
    KM_MODE_CCM = 34,
    /* MAC modes -- only for signing/verification */
    KM_MODE_CMAC = 128,
    KM_MODE_POLY1305 = 129,
} keymaster_block_mode_t;

/**
 * Padding modes that may be applied to plaintext for encryption operations.  This list includes
 * padding modes for both symmetric and asymmetric algorithms.  Note that implementations should not
 * provide all possible combinations of algorithm and padding, only the
 * cryptographically-appropriate pairs.
 */
typedef enum {
    KM_PAD_NONE = 1,     /* required, deprecated */
    KM_PAD_RSA_OAEP = 2, /* required */
    KM_PAD_RSA_PSS = 3,  /* required */
    KM_PAD_RSA_PKCS1_1_5_ENCRYPT = 4,
    KM_PAD_RSA_PKCS1_1_5_SIGN = 5,
    KM_PAD_ANSI_X923 = 32,
    KM_PAD_ISO_10126 = 33,
    KM_PAD_ZERO = 64,  /* required */
    KM_PAD_PKCS7 = 65, /* required */
    KM_PAD_ISO_7816_4 = 66,
} keymaster_padding_t;

#ifndef ANDROID_HARDWARE_KEYMASTER_H

/**
 * Digests that may be provided by keymaster implementations.  Those that must be provided by all
 * implementations are tagged as "required".  Those that have been added since version 0_2 of the
 * API are tagged as "new".
 */
typedef enum {
    KM_DIGEST_NONE = 0,           /* new, required */
    DIGEST_NONE = KM_DIGEST_NONE, /* For 0_2 compatibility */
    KM_DIGEST_MD5 = 1,            /* new, for compatibility with old protocols only */
    KM_DIGEST_SHA1 = 2,           /* new */
    KM_DIGEST_SHA_2_224 = 3,      /* new */
    KM_DIGEST_SHA_2_256 = 4,      /* new, required */
    KM_DIGEST_SHA_2_384 = 5,      /* new, recommended */
    KM_DIGEST_SHA_2_512 = 6,      /* new, recommended */
    KM_DIGEST_SHA_3_256 = 7,      /* new */
    KM_DIGEST_SHA_3_384 = 8,      /* new */
    KM_DIGEST_SHA_3_512 = 9,      /* new */
} keymaster_digest_t;

#endif  // ANDROID_HARDWARE_KEYMASTER_H

/**
 * The origin of a key (or pair), i.e. where it was generated.  Origin and can be used together to
 * determine whether a key may have existed outside of secure hardware.  This type is new in 0_4.
 */
typedef enum {
    KM_ORIGIN_HARDWARE = 0, /* Generated in secure hardware */
    KM_ORIGIN_SOFTWARE = 1, /* Generated in non-secure software */
    KM_ORIGIN_IMPORTED = 2, /* Imported, origin unknown */
} keymaster_key_origin_t;

/**
 * Usability requirements of key blobs.  This defines what system functionality must be available
 * for the key to function.  For example, key "blobs" which are actually handles referencing
 * encrypted key material stored in the file system cannot be used until the file system is
 * available, and should have BLOB_REQUIRES_FILE_SYSTEM.  Other requirements entries will be added
 * as needed for implementations.  This type is new in 0_4.
 */
typedef enum {
    KM_BLOB_STANDALONE = 0,
    KM_BLOB_REQUIRES_FILE_SYSTEM = 1,
} keymaster_key_blob_usage_requirements_t;

/**
 * Possible purposes of a key (or pair). This type is new in 0_4.
 */
typedef enum {
    KM_PURPOSE_ENCRYPT = 0,
    KM_PURPOSE_DECRYPT = 1,
    KM_PURPOSE_SIGN = 2,
    KM_PURPOSE_VERIFY = 3,
} keymaster_purpose_t;

typedef struct {
    const uint8_t* data;
    size_t data_length;
} keymaster_blob_t;

typedef struct {
    keymaster_tag_t tag;
    union {
        uint32_t enumerated;   /* KM_ENUM and KM_ENUM_REP */
        bool boolean;          /* KM_BOOL */
        uint32_t integer;      /* KM_INT and KM_INT_REP */
        uint64_t long_integer; /* KM_LONG */
        uint64_t date_time;    /* KM_DATE */
        keymaster_blob_t blob; /* KM_BIGNUM and KM_BYTES*/
    };
} keymaster_key_param_t;

/**
 * Parameters that define a key's characteristics, including authorized modes of usage and access
 * control restrictions.  The parameters are divided into two categories, those that are enforced by
 * secure hardware, and those that are not.  For a software-only keymaster implementation the
 * enforced array must NULL.  Hardware implementations must enforce everything in the enforced
 * array.
 */
typedef struct {
    keymaster_key_param_t* enforced; /* NULL if enforced_length == 0 */
    size_t enforced_length;
    keymaster_key_param_t* unenforced; /* NULL if unenforced_length == 0 */
    size_t unenforced_length;
} keymaster_key_characteristics_t;

typedef struct {
    const uint8_t* key_material;
    size_t key_material_size;
} keymaster_key_blob_t;

/**
 * Formats for key import and export.  At present, only asymmetric key import/export is supported.
 * In the future this list will expand greatly to accommodate asymmetric key import/export.
 */
typedef enum {
    KM_KEY_FORMAT_X509,   /* for public key export, required */
    KM_KEY_FORMAT_PKCS8,  /* for asymmetric key pair import, required */
    KM_KEY_FORMAT_PKCS12, /* for asymmetric key pair import, not required */
} keymaster_key_format_t;

/**
 * The keymaster operation API consists of begin, update, finish and abort. This is the type of the
 * handle used to tie the sequence of calls together.  A 64-bit value is used because it's important
 * that handles not be predictable.  Implementations must use strong random numbers for handle
 * values.
 */
typedef uint64_t keymaster_operation_handle_t;

typedef enum {
    KM_ERROR_OK = 0,
    KM_ERROR_ROOT_OF_TRUST_ALREADY_SET = -1,
    KM_ERROR_UNSUPPORTED_PURPOSE = -2,
    KM_ERROR_INCOMPATIBLE_PURPOSE = -3,
    KM_ERROR_UNSUPPORTED_ALGORITHM = -4,
    KM_ERROR_INCOMPATIBLE_ALGORITHM = -5,
    KM_ERROR_UNSUPPORTED_KEY_SIZE = -6,
    KM_ERROR_UNSUPPORTED_BLOCK_MODE = -7,
    KM_ERROR_INCOMPATIBLE_BLOCK_MODE = -8,
    KM_ERROR_UNSUPPORTED_TAG_LENGTH = -9,
    KM_ERROR_UNSUPPORTED_PADDING_MODE = -10,
    KM_ERROR_INCOMPATIBLE_PADDING_MODE = -11,
    KM_ERROR_UNSUPPORTED_DIGEST = -12,
    KM_ERROR_INCOMPATIBLE_DIGEST = -13,
    KM_ERROR_INVALID_EXPIRATION_TIME = -14,
    KM_ERROR_INVALID_USER_ID = -15,
    KM_ERROR_INVALID_AUTHORIZATION_TIMEOUT = -16,
    KM_ERROR_UNSUPPORTED_KEY_FORMAT = -17,
    KM_ERROR_INCOMPATIBLE_KEY_FORMAT = -18,
    KM_ERROR_UNSUPPORTED_KEY_ENCRYPTION_ALGORITHM = -19,   /* For PKCS8 & PKCS12 */
    KM_ERROR_UNSUPPORTED_KEY_VERIFICATION_ALGORITHM = -20, /* For PKCS8 & PKCS12 */
    KM_ERROR_INVALID_INPUT_LENGTH = -21,
    KM_ERROR_KEY_EXPORT_OPTIONS_INVALID = -22,
    KM_ERROR_DELEGATION_NOT_ALLOWED = -23,
    KM_ERROR_KEY_NOT_YET_VALID = -24,
    KM_ERROR_KEY_EXPIRED = -25,
    KM_ERROR_KEY_USER_NOT_AUTHENTICATED = -26,
    KM_ERROR_OUTPUT_PARAMETER_NULL = -27,
    KM_ERROR_INVALID_OPERATION_HANDLE = -28,
    KM_ERROR_INSUFFICIENT_BUFFER_SPACE = -29,
    KM_ERROR_VERIFICATION_FAILED = -30,
    KM_ERROR_TOO_MANY_OPERATIONS = -31,
    KM_ERROR_UNEXPECTED_NULL_POINTER = -32,
    KM_ERROR_INVALID_KEY_BLOB = -33,
    KM_ERROR_IMPORTED_KEY_NOT_ENCRYPTED = -34,
    KM_ERROR_IMPORTED_KEY_DECRYPTION_FAILED = -35,
    KM_ERROR_IMPORTED_KEY_NOT_SIGNED = -36,
    KM_ERROR_IMPORTED_KEY_VERIFICATION_FAILED = -37,
    KM_ERROR_INVALID_ARGUMENT = -38,
    KM_ERROR_UNSUPPORTED_TAG = -39,
    KM_ERROR_INVALID_TAG = -40,
    KM_ERROR_MEMORY_ALLOCATION_FAILED = -41,
    KM_ERROR_INVALID_RESCOPING = -42,
    KM_ERROR_INVALID_DSA_PARAMS = -43,
    KM_ERROR_IMPORT_PARAMETER_MISMATCH =-44,
    KM_ERROR_SECURE_HW_ACCESS_DENIED = -45,
    KM_ERROR_OPERATION_CANCELLED =-46,
    KM_ERROR_CONCURRENT_ACCESS_CONFLICT = -47,
    KM_ERROR_SECURE_HW_BUSY =-48,
    KM_ERROR_SECURE_HW_COMMUNICATION_FAILED = -49,
    KM_ERROR_UNSUPPORTED_EC_FIELD = -50,
    KM_ERROR_UNIMPLEMENTED = -100,

    /* Additional error codes may be added by implementations, but implementers should coordinate
     * with Google to avoid code collision. */
    KM_ERROR_UNKNOWN_ERROR = -1000,
} keymaster_error_t;

/* Convenience functions for manipulating keymaster tag types */

static inline keymaster_tag_type_t keymaster_tag_get_type(keymaster_tag_t tag) {
    return (keymaster_tag_type_t)(tag & (0xF << 28));
}

static inline uint32_t keymaster_tag_mask_type(keymaster_tag_t tag) {
    return tag & 0x0FFFFFFF;
}

static inline bool keymaster_tag_type_repeatable(keymaster_tag_type_t type) {
    switch (type) {
    case KM_INT_REP:
    case KM_ENUM_REP:
        return true;
    default:
        return false;
    }
}

static inline bool keymaster_tag_repeatable(keymaster_tag_t tag) {
    return keymaster_tag_type_repeatable(keymaster_tag_get_type(tag));
}

/* Convenience functions for manipulating keymaster_key_param_t structs */

inline keymaster_key_param_t keymaster_param_enum(keymaster_tag_t tag, uint32_t value) {
    // assert(keymaster_tag_get_type(tag) == KM_ENUM || keymaster_tag_get_type(tag) == KM_ENUM_REP);
    keymaster_key_param_t param;
    memset(&param, 0, sizeof(param));
    param.tag = tag;
    param.enumerated = value;
    return param;
}

inline keymaster_key_param_t keymaster_param_int(keymaster_tag_t tag, uint32_t value) {
    // assert(keymaster_tag_get_type(tag) == KM_INT || keymaster_tag_get_type(tag) == KM_INT_REP);
    keymaster_key_param_t param;
    memset(&param, 0, sizeof(param));
    param.tag = tag;
    param.integer = value;
    return param;
}

inline keymaster_key_param_t keymaster_param_long(keymaster_tag_t tag, uint64_t value) {
    // assert(keymaster_tag_get_type(tag) == KM_LONG);
    keymaster_key_param_t param;
    memset(&param, 0, sizeof(param));
    param.tag = tag;
    param.long_integer = value;
    return param;
}

inline keymaster_key_param_t keymaster_param_blob(keymaster_tag_t tag, const uint8_t* bytes,
                                                  size_t bytes_len) {
    // assert(keymaster_tag_get_type(tag) == KM_BYTES || keymaster_tag_get_type(tag) == KM_BIGNUM);
    keymaster_key_param_t param;
    memset(&param, 0, sizeof(param));
    param.tag = tag;
    param.blob.data = bytes;
    param.blob.data_length = bytes_len;
    return param;
}

inline keymaster_key_param_t keymaster_param_bool(keymaster_tag_t tag) {
    // assert(keymaster_tag_get_type(tag) == KM_BOOL);
    keymaster_key_param_t param;
    memset(&param, 0, sizeof(param));
    param.tag = tag;
    param.boolean = true;
    return param;
}

inline keymaster_key_param_t keymaster_param_date(keymaster_tag_t tag, uint64_t value) {
    // assert(keymaster_tag_get_type(tag) == KM_DATE);
    keymaster_key_param_t param;
    memset(&param, 0, sizeof(param));
    param.tag = tag;
    param.date_time = value;
    return param;
}

static inline void keymaster_free_param_values(keymaster_key_param_t* param, size_t param_count) {
    while (param_count-- > 0) {
        switch (keymaster_tag_get_type(param->tag)) {
        case KM_BIGNUM:
        case KM_BYTES:
            free((void*)param->blob.data);
            break;
        default:
            // NOP
            break;
        }
        ++param;
    }
}

#if defined(__cplusplus)
}  // extern "C"
#endif  // defined(__cplusplus)

#endif  // ANDROID_HARDWARE_KEYMASTER_DEFS_H
