/*
 * Copyright (C) 2012 The Android Open Source Project
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
#include <errno.h>
#include <string.h>
#include <stdint.h>

#include <keystore/keystore.h>

#include <hardware/hardware.h>
#include <hardware/keymaster.h>

#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/rsa.h>
#include <openssl/err.h>
#include <openssl/x509.h>

#include <UniquePtr.h>

// For debugging
// #define LOG_NDEBUG 0

#define LOG_TAG "OpenSSLKeyMaster"
#include <cutils/log.h>

struct BIGNUM_Delete {
    void operator()(BIGNUM* p) const {
        BN_free(p);
    }
};
typedef UniquePtr<BIGNUM, BIGNUM_Delete> Unique_BIGNUM;

struct EVP_PKEY_Delete {
    void operator()(EVP_PKEY* p) const {
        EVP_PKEY_free(p);
    }
};
typedef UniquePtr<EVP_PKEY, EVP_PKEY_Delete> Unique_EVP_PKEY;

struct PKCS8_PRIV_KEY_INFO_Delete {
    void operator()(PKCS8_PRIV_KEY_INFO* p) const {
        PKCS8_PRIV_KEY_INFO_free(p);
    }
};
typedef UniquePtr<PKCS8_PRIV_KEY_INFO, PKCS8_PRIV_KEY_INFO_Delete> Unique_PKCS8_PRIV_KEY_INFO;

struct DSA_Delete {
    void operator()(DSA* p) const {
        DSA_free(p);
    }
};
typedef UniquePtr<DSA, DSA_Delete> Unique_DSA;

struct EC_KEY_Delete {
    void operator()(EC_KEY* p) const {
        EC_KEY_free(p);
    }
};
typedef UniquePtr<EC_KEY, EC_KEY_Delete> Unique_EC_KEY;

struct EC_GROUP_Delete {
    void operator()(EC_GROUP* p) const {
        EC_GROUP_free(p);
    }
};
typedef UniquePtr<EC_GROUP, EC_GROUP_Delete> Unique_EC_GROUP;

struct RSA_Delete {
    void operator()(RSA* p) const {
        RSA_free(p);
    }
};
typedef UniquePtr<RSA, RSA_Delete> Unique_RSA;

struct Malloc_Free {
    void operator()(void* p) const {
        free(p);
    }
};

typedef UniquePtr<keymaster_device_t> Unique_keymaster_device_t;

/**
 * Many OpenSSL APIs take ownership of an argument on success but
 * don't free the argument on failure. This means we need to tell our
 * scoped pointers when we've transferred ownership, without
 * triggering a warning by not using the result of release().
 */
template <typename T, typename Delete_T>
inline void release_because_ownership_transferred(UniquePtr<T, Delete_T>& p) {
    T* val __attribute__((unused)) = p.release();
}

/*
 * Checks this thread's OpenSSL error queue and logs if
 * necessary.
 */
static void logOpenSSLError(const char* location) {
    int error = ERR_get_error();

    if (error != 0) {
        char message[256];
        ERR_error_string_n(error, message, sizeof(message));
        ALOGE("OpenSSL error in %s %d: %s", location, error, message);
    }

    ERR_clear_error();
    ERR_remove_state(0);
}

static int wrap_key(EVP_PKEY* pkey, int type, uint8_t** keyBlob, size_t* keyBlobLength) {
    /*
     * Find the length of each size. Public key is not needed anymore
     * but must be kept for alignment purposes.
     */
    int publicLen = 0;
    int privateLen = i2d_PrivateKey(pkey, NULL);

    if (privateLen <= 0) {
        ALOGE("private key size was too big");
        return -1;
    }

    /* int type + int size + private key data + int size + public key data */
    *keyBlobLength = get_softkey_header_size() + sizeof(type) + sizeof(publicLen) + privateLen +
                     sizeof(privateLen) + publicLen;

    // derData will be returned to the caller, so allocate it with malloc.
    UniquePtr<unsigned char, Malloc_Free> derData(
        static_cast<unsigned char*>(malloc(*keyBlobLength)));
    if (derData.get() == NULL) {
        ALOGE("could not allocate memory for key blob");
        return -1;
    }
    unsigned char* p = derData.get();

    /* Write the magic value for software keys. */
    p = add_softkey_header(p, *keyBlobLength);

    /* Write key type to allocated buffer */
    for (int i = sizeof(type) - 1; i >= 0; i--) {
        *p++ = (type >> (8 * i)) & 0xFF;
    }

    /* Write public key to allocated buffer */
    for (int i = sizeof(publicLen) - 1; i >= 0; i--) {
        *p++ = (publicLen >> (8 * i)) & 0xFF;
    }

    /* Write private key to allocated buffer */
    for (int i = sizeof(privateLen) - 1; i >= 0; i--) {
        *p++ = (privateLen >> (8 * i)) & 0xFF;
    }
    if (i2d_PrivateKey(pkey, &p) != privateLen) {
        logOpenSSLError("wrap_key");
        return -1;
    }

    *keyBlob = derData.release();

    return 0;
}

static EVP_PKEY* unwrap_key(const uint8_t* keyBlob, const size_t keyBlobLength) {
    long publicLen = 0;
    long privateLen = 0;
    const uint8_t* p = keyBlob;
    const uint8_t* const end = keyBlob + keyBlobLength;

    if (keyBlob == NULL) {
        ALOGE("supplied key blob was NULL");
        return NULL;
    }

    int type = 0;
    if (keyBlobLength < (get_softkey_header_size() + sizeof(type) + sizeof(publicLen) + 1 +
                         sizeof(privateLen) + 1)) {
        ALOGE("key blob appears to be truncated");
        return NULL;
    }

    if (!is_softkey(p, keyBlobLength)) {
        ALOGE("cannot read key; it was not made by this keymaster");
        return NULL;
    }
    p += get_softkey_header_size();

    for (size_t i = 0; i < sizeof(type); i++) {
        type = (type << 8) | *p++;
    }

    for (size_t i = 0; i < sizeof(type); i++) {
        publicLen = (publicLen << 8) | *p++;
    }
    if (p + publicLen > end) {
        ALOGE("public key length encoding error: size=%ld, end=%td", publicLen, end - p);
        return NULL;
    }

    p += publicLen;
    if (end - p < 2) {
        ALOGE("private key truncated");
        return NULL;
    }
    for (size_t i = 0; i < sizeof(type); i++) {
        privateLen = (privateLen << 8) | *p++;
    }
    if (p + privateLen > end) {
        ALOGE("private key length encoding error: size=%ld, end=%td", privateLen, end - p);
        return NULL;
    }

    Unique_EVP_PKEY pkey(EVP_PKEY_new());
    if (pkey.get() == NULL) {
        logOpenSSLError("unwrap_key");
        return NULL;
    }
    EVP_PKEY* tmp = pkey.get();

    if (d2i_PrivateKey(type, &tmp, &p, privateLen) == NULL) {
        logOpenSSLError("unwrap_key");
        return NULL;
    }

    return pkey.release();
}

static int generate_dsa_keypair(EVP_PKEY* pkey, const keymaster_dsa_keygen_params_t* dsa_params) {
    if (dsa_params->key_size < 512) {
        ALOGI("Requested DSA key size is too small (<512)");
        return -1;
    }

    Unique_DSA dsa(DSA_new());

    if (dsa_params->generator_len == 0 || dsa_params->prime_p_len == 0 ||
        dsa_params->prime_q_len == 0 || dsa_params->generator == NULL ||
        dsa_params->prime_p == NULL || dsa_params->prime_q == NULL) {
        if (DSA_generate_parameters_ex(dsa.get(), dsa_params->key_size, NULL, 0, NULL, NULL,
                                       NULL) != 1) {
            logOpenSSLError("generate_dsa_keypair");
            return -1;
        }
    } else {
        dsa->g = BN_bin2bn(dsa_params->generator, dsa_params->generator_len, NULL);
        if (dsa->g == NULL) {
            logOpenSSLError("generate_dsa_keypair");
            return -1;
        }

        dsa->p = BN_bin2bn(dsa_params->prime_p, dsa_params->prime_p_len, NULL);
        if (dsa->p == NULL) {
            logOpenSSLError("generate_dsa_keypair");
            return -1;
        }

        dsa->q = BN_bin2bn(dsa_params->prime_q, dsa_params->prime_q_len, NULL);
        if (dsa->q == NULL) {
            logOpenSSLError("generate_dsa_keypair");
            return -1;
        }
    }

    if (DSA_generate_key(dsa.get()) != 1) {
        logOpenSSLError("generate_dsa_keypair");
        return -1;
    }

    if (EVP_PKEY_assign_DSA(pkey, dsa.get()) == 0) {
        logOpenSSLError("generate_dsa_keypair");
        return -1;
    }
    release_because_ownership_transferred(dsa);

    return 0;
}

static int generate_ec_keypair(EVP_PKEY* pkey, const keymaster_ec_keygen_params_t* ec_params) {
    Unique_EC_GROUP group;
    switch (ec_params->field_size) {
    case 192:
        group.reset(EC_GROUP_new_by_curve_name(NID_X9_62_prime192v1));
        break;
    case 224:
        group.reset(EC_GROUP_new_by_curve_name(NID_secp224r1));
        break;
    case 256:
        group.reset(EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1));
        break;
    case 384:
        group.reset(EC_GROUP_new_by_curve_name(NID_secp384r1));
        break;
    case 521:
        group.reset(EC_GROUP_new_by_curve_name(NID_secp521r1));
        break;
    default:
        break;
    }

    if (group.get() == NULL) {
        logOpenSSLError("generate_ec_keypair");
        return -1;
    }

    EC_GROUP_set_point_conversion_form(group.get(), POINT_CONVERSION_UNCOMPRESSED);
    EC_GROUP_set_asn1_flag(group.get(), OPENSSL_EC_NAMED_CURVE);

    /* initialize EC key */
    Unique_EC_KEY eckey(EC_KEY_new());
    if (eckey.get() == NULL) {
        logOpenSSLError("generate_ec_keypair");
        return -1;
    }

    if (EC_KEY_set_group(eckey.get(), group.get()) != 1) {
        logOpenSSLError("generate_ec_keypair");
        return -1;
    }

    if (EC_KEY_generate_key(eckey.get()) != 1 || EC_KEY_check_key(eckey.get()) < 0) {
        logOpenSSLError("generate_ec_keypair");
        return -1;
    }

    if (EVP_PKEY_assign_EC_KEY(pkey, eckey.get()) == 0) {
        logOpenSSLError("generate_ec_keypair");
        return -1;
    }
    release_because_ownership_transferred(eckey);

    return 0;
}

static int generate_rsa_keypair(EVP_PKEY* pkey, const keymaster_rsa_keygen_params_t* rsa_params) {
    Unique_BIGNUM bn(BN_new());
    if (bn.get() == NULL) {
        logOpenSSLError("generate_rsa_keypair");
        return -1;
    }

    if (BN_set_word(bn.get(), rsa_params->public_exponent) == 0) {
        logOpenSSLError("generate_rsa_keypair");
        return -1;
    }

    /* initialize RSA */
    Unique_RSA rsa(RSA_new());
    if (rsa.get() == NULL) {
        logOpenSSLError("generate_rsa_keypair");
        return -1;
    }

    if (!RSA_generate_key_ex(rsa.get(), rsa_params->modulus_size, bn.get(), NULL) ||
        RSA_check_key(rsa.get()) < 0) {
        logOpenSSLError("generate_rsa_keypair");
        return -1;
    }

    if (EVP_PKEY_assign_RSA(pkey, rsa.get()) == 0) {
        logOpenSSLError("generate_rsa_keypair");
        return -1;
    }
    release_because_ownership_transferred(rsa);

    return 0;
}

__attribute__((visibility("default"))) int openssl_generate_keypair(
    const keymaster_device_t*, const keymaster_keypair_t key_type, const void* key_params,
    uint8_t** keyBlob, size_t* keyBlobLength) {
    Unique_EVP_PKEY pkey(EVP_PKEY_new());
    if (pkey.get() == NULL) {
        logOpenSSLError("openssl_generate_keypair");
        return -1;
    }

    if (key_params == NULL) {
        ALOGW("key_params == null");
        return -1;
    } else if (key_type == TYPE_DSA) {
        const keymaster_dsa_keygen_params_t* dsa_params =
            (const keymaster_dsa_keygen_params_t*)key_params;
        generate_dsa_keypair(pkey.get(), dsa_params);
    } else if (key_type == TYPE_EC) {
        const keymaster_ec_keygen_params_t* ec_params =
            (const keymaster_ec_keygen_params_t*)key_params;
        generate_ec_keypair(pkey.get(), ec_params);
    } else if (key_type == TYPE_RSA) {
        const keymaster_rsa_keygen_params_t* rsa_params =
            (const keymaster_rsa_keygen_params_t*)key_params;
        generate_rsa_keypair(pkey.get(), rsa_params);
    } else {
        ALOGW("Unsupported key type %d", key_type);
        return -1;
    }

    if (wrap_key(pkey.get(), EVP_PKEY_type(pkey->type), keyBlob, keyBlobLength)) {
        return -1;
    }

    return 0;
}

__attribute__((visibility("default"))) int openssl_import_keypair(const keymaster_device_t*,
                                                                  const uint8_t* key,
                                                                  const size_t key_length,
                                                                  uint8_t** key_blob,
                                                                  size_t* key_blob_length) {
    if (key == NULL) {
        ALOGW("input key == NULL");
        return -1;
    } else if (key_blob == NULL || key_blob_length == NULL) {
        ALOGW("output key blob or length == NULL");
        return -1;
    }

    Unique_PKCS8_PRIV_KEY_INFO pkcs8(d2i_PKCS8_PRIV_KEY_INFO(NULL, &key, key_length));
    if (pkcs8.get() == NULL) {
        logOpenSSLError("openssl_import_keypair");
        return -1;
    }

    /* assign to EVP */
    Unique_EVP_PKEY pkey(EVP_PKCS82PKEY(pkcs8.get()));
    if (pkey.get() == NULL) {
        logOpenSSLError("openssl_import_keypair");
        return -1;
    }
    release_because_ownership_transferred(pkcs8);

    if (wrap_key(pkey.get(), EVP_PKEY_type(pkey->type), key_blob, key_blob_length)) {
        return -1;
    }

    return 0;
}

__attribute__((visibility("default"))) int openssl_get_keypair_public(
    const struct keymaster_device*, const uint8_t* key_blob, const size_t key_blob_length,
    uint8_t** x509_data, size_t* x509_data_length) {

    if (x509_data == NULL || x509_data_length == NULL) {
        ALOGW("output public key buffer == NULL");
        return -1;
    }

    Unique_EVP_PKEY pkey(unwrap_key(key_blob, key_blob_length));
    if (pkey.get() == NULL) {
        return -1;
    }

    int len = i2d_PUBKEY(pkey.get(), NULL);
    if (len <= 0) {
        logOpenSSLError("openssl_get_keypair_public");
        return -1;
    }

    UniquePtr<uint8_t, Malloc_Free> key(static_cast<uint8_t*>(malloc(len)));
    if (key.get() == NULL) {
        ALOGE("Could not allocate memory for public key data");
        return -1;
    }

    unsigned char* tmp = reinterpret_cast<unsigned char*>(key.get());
    if (i2d_PUBKEY(pkey.get(), &tmp) != len) {
        logOpenSSLError("openssl_get_keypair_public");
        return -1;
    }

    ALOGV("Length of x509 data is %d", len);
    *x509_data_length = len;
    *x509_data = key.release();

    return 0;
}

static int sign_dsa(EVP_PKEY* pkey, keymaster_dsa_sign_params_t* sign_params, const uint8_t* data,
                    const size_t dataLength, uint8_t** signedData, size_t* signedDataLength) {
    if (sign_params->digest_type != DIGEST_NONE) {
        ALOGW("Cannot handle digest type %d", sign_params->digest_type);
        return -1;
    }

    Unique_DSA dsa(EVP_PKEY_get1_DSA(pkey));
    if (dsa.get() == NULL) {
        logOpenSSLError("openssl_sign_dsa");
        return -1;
    }

    unsigned int dsaSize = DSA_size(dsa.get());
    UniquePtr<uint8_t, Malloc_Free> signedDataPtr(reinterpret_cast<uint8_t*>(malloc(dsaSize)));
    if (signedDataPtr.get() == NULL) {
        logOpenSSLError("openssl_sign_dsa");
        return -1;
    }

    unsigned char* tmp = reinterpret_cast<unsigned char*>(signedDataPtr.get());
    if (DSA_sign(0, data, dataLength, tmp, &dsaSize, dsa.get()) <= 0) {
        logOpenSSLError("openssl_sign_dsa");
        return -1;
    }

    *signedDataLength = dsaSize;
    *signedData = signedDataPtr.release();

    return 0;
}

static int sign_ec(EVP_PKEY* pkey, keymaster_ec_sign_params_t* sign_params, const uint8_t* data,
                   const size_t dataLength, uint8_t** signedData, size_t* signedDataLength) {
    if (sign_params->digest_type != DIGEST_NONE) {
        ALOGW("Cannot handle digest type %d", sign_params->digest_type);
        return -1;
    }

    Unique_EC_KEY eckey(EVP_PKEY_get1_EC_KEY(pkey));
    if (eckey.get() == NULL) {
        logOpenSSLError("openssl_sign_ec");
        return -1;
    }

    unsigned int ecdsaSize = ECDSA_size(eckey.get());
    UniquePtr<uint8_t, Malloc_Free> signedDataPtr(reinterpret_cast<uint8_t*>(malloc(ecdsaSize)));
    if (signedDataPtr.get() == NULL) {
        logOpenSSLError("openssl_sign_ec");
        return -1;
    }

    unsigned char* tmp = reinterpret_cast<unsigned char*>(signedDataPtr.get());
    if (ECDSA_sign(0, data, dataLength, tmp, &ecdsaSize, eckey.get()) <= 0) {
        logOpenSSLError("openssl_sign_ec");
        return -1;
    }

    *signedDataLength = ecdsaSize;
    *signedData = signedDataPtr.release();

    return 0;
}

static int sign_rsa(EVP_PKEY* pkey, keymaster_rsa_sign_params_t* sign_params, const uint8_t* data,
                    const size_t dataLength, uint8_t** signedData, size_t* signedDataLength) {
    if (sign_params->digest_type != DIGEST_NONE) {
        ALOGW("Cannot handle digest type %d", sign_params->digest_type);
        return -1;
    } else if (sign_params->padding_type != PADDING_NONE) {
        ALOGW("Cannot handle padding type %d", sign_params->padding_type);
        return -1;
    }

    Unique_RSA rsa(EVP_PKEY_get1_RSA(pkey));
    if (rsa.get() == NULL) {
        logOpenSSLError("openssl_sign_rsa");
        return -1;
    }

    UniquePtr<uint8_t, Malloc_Free> signedDataPtr(reinterpret_cast<uint8_t*>(malloc(dataLength)));
    if (signedDataPtr.get() == NULL) {
        logOpenSSLError("openssl_sign_rsa");
        return -1;
    }

    unsigned char* tmp = reinterpret_cast<unsigned char*>(signedDataPtr.get());
    if (RSA_private_encrypt(dataLength, data, tmp, rsa.get(), RSA_NO_PADDING) <= 0) {
        logOpenSSLError("openssl_sign_rsa");
        return -1;
    }

    *signedDataLength = dataLength;
    *signedData = signedDataPtr.release();

    return 0;
}

__attribute__((visibility("default"))) int openssl_sign_data(
    const keymaster_device_t*, const void* params, const uint8_t* keyBlob,
    const size_t keyBlobLength, const uint8_t* data, const size_t dataLength, uint8_t** signedData,
    size_t* signedDataLength) {
    if (data == NULL) {
        ALOGW("input data to sign == NULL");
        return -1;
    } else if (signedData == NULL || signedDataLength == NULL) {
        ALOGW("output signature buffer == NULL");
        return -1;
    }

    Unique_EVP_PKEY pkey(unwrap_key(keyBlob, keyBlobLength));
    if (pkey.get() == NULL) {
        return -1;
    }

    int type = EVP_PKEY_type(pkey->type);
    if (type == EVP_PKEY_DSA) {
        const keymaster_dsa_sign_params_t* sign_params =
            reinterpret_cast<const keymaster_dsa_sign_params_t*>(params);
        return sign_dsa(pkey.get(), const_cast<keymaster_dsa_sign_params_t*>(sign_params), data,
                        dataLength, signedData, signedDataLength);
    } else if (type == EVP_PKEY_EC) {
        const keymaster_ec_sign_params_t* sign_params =
            reinterpret_cast<const keymaster_ec_sign_params_t*>(params);
        return sign_ec(pkey.get(), const_cast<keymaster_ec_sign_params_t*>(sign_params), data,
                       dataLength, signedData, signedDataLength);
    } else if (type == EVP_PKEY_RSA) {
        const keymaster_rsa_sign_params_t* sign_params =
            reinterpret_cast<const keymaster_rsa_sign_params_t*>(params);
        return sign_rsa(pkey.get(), const_cast<keymaster_rsa_sign_params_t*>(sign_params), data,
                        dataLength, signedData, signedDataLength);
    } else {
        ALOGW("Unsupported key type");
        return -1;
    }
}

static int verify_dsa(EVP_PKEY* pkey, keymaster_dsa_sign_params_t* sign_params,
                      const uint8_t* signedData, const size_t signedDataLength,
                      const uint8_t* signature, const size_t signatureLength) {
    if (sign_params->digest_type != DIGEST_NONE) {
        ALOGW("Cannot handle digest type %d", sign_params->digest_type);
        return -1;
    }

    Unique_DSA dsa(EVP_PKEY_get1_DSA(pkey));
    if (dsa.get() == NULL) {
        logOpenSSLError("openssl_verify_dsa");
        return -1;
    }

    if (DSA_verify(0, signedData, signedDataLength, signature, signatureLength, dsa.get()) <= 0) {
        logOpenSSLError("openssl_verify_dsa");
        return -1;
    }

    return 0;
}

static int verify_ec(EVP_PKEY* pkey, keymaster_ec_sign_params_t* sign_params,
                     const uint8_t* signedData, const size_t signedDataLength,
                     const uint8_t* signature, const size_t signatureLength) {
    if (sign_params->digest_type != DIGEST_NONE) {
        ALOGW("Cannot handle digest type %d", sign_params->digest_type);
        return -1;
    }

    Unique_EC_KEY eckey(EVP_PKEY_get1_EC_KEY(pkey));
    if (eckey.get() == NULL) {
        logOpenSSLError("openssl_verify_ec");
        return -1;
    }

    if (ECDSA_verify(0, signedData, signedDataLength, signature, signatureLength, eckey.get()) <=
        0) {
        logOpenSSLError("openssl_verify_ec");
        return -1;
    }

    return 0;
}

static int verify_rsa(EVP_PKEY* pkey, keymaster_rsa_sign_params_t* sign_params,
                      const uint8_t* signedData, const size_t signedDataLength,
                      const uint8_t* signature, const size_t signatureLength) {
    if (sign_params->digest_type != DIGEST_NONE) {
        ALOGW("Cannot handle digest type %d", sign_params->digest_type);
        return -1;
    } else if (sign_params->padding_type != PADDING_NONE) {
        ALOGW("Cannot handle padding type %d", sign_params->padding_type);
        return -1;
    } else if (signatureLength != signedDataLength) {
        ALOGW("signed data length must be signature length");
        return -1;
    }

    Unique_RSA rsa(EVP_PKEY_get1_RSA(pkey));
    if (rsa.get() == NULL) {
        logOpenSSLError("openssl_verify_data");
        return -1;
    }

    UniquePtr<uint8_t[]> dataPtr(new uint8_t[signedDataLength]);
    if (dataPtr.get() == NULL) {
        logOpenSSLError("openssl_verify_data");
        return -1;
    }

    unsigned char* tmp = reinterpret_cast<unsigned char*>(dataPtr.get());
    if (!RSA_public_decrypt(signatureLength, signature, tmp, rsa.get(), RSA_NO_PADDING)) {
        logOpenSSLError("openssl_verify_data");
        return -1;
    }

    int result = 0;
    for (size_t i = 0; i < signedDataLength; i++) {
        result |= tmp[i] ^ signedData[i];
    }

    return result == 0 ? 0 : -1;
}

__attribute__((visibility("default"))) int openssl_verify_data(
    const keymaster_device_t*, const void* params, const uint8_t* keyBlob,
    const size_t keyBlobLength, const uint8_t* signedData, const size_t signedDataLength,
    const uint8_t* signature, const size_t signatureLength) {

    if (signedData == NULL || signature == NULL) {
        ALOGW("data or signature buffers == NULL");
        return -1;
    }

    Unique_EVP_PKEY pkey(unwrap_key(keyBlob, keyBlobLength));
    if (pkey.get() == NULL) {
        return -1;
    }

    int type = EVP_PKEY_type(pkey->type);
    if (type == EVP_PKEY_DSA) {
        const keymaster_dsa_sign_params_t* sign_params =
            reinterpret_cast<const keymaster_dsa_sign_params_t*>(params);
        return verify_dsa(pkey.get(), const_cast<keymaster_dsa_sign_params_t*>(sign_params),
                          signedData, signedDataLength, signature, signatureLength);
    } else if (type == EVP_PKEY_RSA) {
        const keymaster_rsa_sign_params_t* sign_params =
            reinterpret_cast<const keymaster_rsa_sign_params_t*>(params);
        return verify_rsa(pkey.get(), const_cast<keymaster_rsa_sign_params_t*>(sign_params),
                          signedData, signedDataLength, signature, signatureLength);
    } else if (type == EVP_PKEY_EC) {
        const keymaster_ec_sign_params_t* sign_params =
            reinterpret_cast<const keymaster_ec_sign_params_t*>(params);
        return verify_ec(pkey.get(), const_cast<keymaster_ec_sign_params_t*>(sign_params),
                         signedData, signedDataLength, signature, signatureLength);
    } else {
        ALOGW("Unsupported key type %d", type);
        return -1;
    }
}
