/*
 * Copyright (C) 2012 Samsung Electronics Co., LTD
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

#include <hardware/hardware.h>
#include <hardware/keymaster.h>

#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/rsa.h>
#include <openssl/err.h>
#include <openssl/x509.h>

#include <UniquePtr.h>

#define LOG_TAG "ExynosKeyMaster"
#include <cutils/log.h>

#include <tlcTeeKeymaster_if.h>

#define RSA_KEY_BUFFER_SIZE   1536
#define RSA_KEY_MAX_SIZE      (2048 >> 3)

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

struct RSA_Delete {
    void operator()(RSA* p) const {
        RSA_free(p);
    }
};
typedef UniquePtr<RSA, RSA_Delete> Unique_RSA;

typedef UniquePtr<keymaster_device_t> Unique_keymaster_device_t;

/**
 * Many OpenSSL APIs take ownership of an argument on success but don't free the argument
 * on failure. This means we need to tell our scoped pointers when we've transferred ownership,
 * without triggering a warning by not using the result of release().
 */
#define OWNERSHIP_TRANSFERRED(obj) \
    typeof (obj.release()) _dummy __attribute__((unused)) = obj.release()

/*
 * Checks this thread's error queue and logs if necessary.
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

static int exynos_km_generate_keypair(const keymaster_device_t* dev,
        const keymaster_keypair_t key_type, const void* key_params,
        uint8_t** keyBlob, size_t* keyBlobLength) {
    teeResult_t ret = TEE_ERR_NONE;

    if (key_type != TYPE_RSA) {
        ALOGE("Unsupported key type %d", key_type);
        return -1;
    } else if (key_params == NULL) {
        ALOGE("key_params == null");
        return -1;
    }

    keymaster_rsa_keygen_params_t* rsa_params = (keymaster_rsa_keygen_params_t*) key_params;

    if ((rsa_params->modulus_size != 512) &&
        (rsa_params->modulus_size != 1024) &&
        (rsa_params->modulus_size != 2048)) {
        ALOGE("key size(%d) is not supported\n", rsa_params->modulus_size);
        return -1;
    }

    UniquePtr<uint8_t> keyDataPtr(reinterpret_cast<uint8_t*>(malloc(RSA_KEY_BUFFER_SIZE)));
    if (keyDataPtr.get() == NULL) {
        ALOGE("memory allocation is failed");
        return -1;
    }

    ret = TEE_RSAGenerateKeyPair(TEE_KEYPAIR_RSACRT, keyDataPtr.get(), RSA_KEY_BUFFER_SIZE,
				rsa_params->modulus_size, (uint32_t)rsa_params->public_exponent,
				keyBlobLength);
    if (ret != TEE_ERR_NONE) {
        ALOGE("TEE_RSAGenerateKeyPair() is failed: %d", ret);
        return -1;
    }

   *keyBlob = keyDataPtr.release();

    return 0;
}

static int exynos_km_import_keypair(const keymaster_device_t* dev,
        const uint8_t* key, const size_t key_length,
        uint8_t** key_blob, size_t* key_blob_length) {
    uint8_t kbuf[RSA_KEY_BUFFER_SIZE];
    teeRsaKeyMeta_t metadata;
    uint32_t key_len = 0;
    teeResult_t ret = TEE_ERR_NONE;

    if (key == NULL) {
        ALOGE("input key == NULL");
        return -1;
    } else if (key_blob == NULL || key_blob_length == NULL) {
        ALOGE("output key blob or length == NULL");
        return -1;
    }

    /* decoding */
    Unique_PKCS8_PRIV_KEY_INFO pkcs8(d2i_PKCS8_PRIV_KEY_INFO(NULL, &key, key_length));
    if (pkcs8.get() == NULL) {
        logOpenSSLError("pkcs4.get");
        return -1;
    }

    /* assign to EVP */
    Unique_EVP_PKEY pkey(EVP_PKCS82PKEY(pkcs8.get()));
    if (pkey.get() == NULL) {
        logOpenSSLError("pkey.get");
        return -1;
    }
    OWNERSHIP_TRANSFERRED(pkcs8);

    /* change key format */
    Unique_RSA rsa(EVP_PKEY_get1_RSA(pkey.get()));
    if (rsa.get() == NULL) {
        logOpenSSLError("get rsa key format");
	return -1;
    }

    key_len += sizeof(metadata);

    metadata.lenpubmod = BN_bn2bin(rsa->n, kbuf + key_len);
    key_len += metadata.lenpubmod;
    if (metadata.lenpubmod == (512 >> 3))
        metadata.keysize = TEE_RSA_KEY_SIZE_512;
    else if (metadata.lenpubmod == (1024 >> 3))
        metadata.keysize = TEE_RSA_KEY_SIZE_1024;
    else if (metadata.lenpubmod == (2048 >> 3))
        metadata.keysize = TEE_RSA_KEY_SIZE_2048;
    else {
        ALOGE("key size(%d) is not supported\n", metadata.lenpubmod << 3);
        return -1;
    }

    metadata.lenpubexp = BN_bn2bin(rsa->e, kbuf + key_len);
    key_len += metadata.lenpubexp;

    if ((rsa->p != NULL) && (rsa->q != NULL) && (rsa->dmp1 != NULL) &&
	(rsa->dmq1 != NULL) && (rsa->iqmp != NULL))
    {
           metadata.keytype = TEE_KEYPAIR_RSACRT;
	   metadata.rsacrtpriv.lenp = BN_bn2bin(rsa->p, kbuf + key_len);
	   key_len += metadata.rsacrtpriv.lenp;
	   metadata.rsacrtpriv.lenq = BN_bn2bin(rsa->q, kbuf + key_len);
	   key_len += metadata.rsacrtpriv.lenq;
	   metadata.rsacrtpriv.lendp = BN_bn2bin(rsa->dmp1, kbuf + key_len);
	   key_len += metadata.rsacrtpriv.lendp;
	   metadata.rsacrtpriv.lendq = BN_bn2bin(rsa->dmq1, kbuf + key_len);
	   key_len += metadata.rsacrtpriv.lendq;
	   metadata.rsacrtpriv.lenqinv = BN_bn2bin(rsa->iqmp, kbuf + key_len);
	   key_len += metadata.rsacrtpriv.lenqinv;
    } else {
           metadata.keytype = TEE_KEYPAIR_RSA;
	   metadata.rsapriv.lenpriexp = BN_bn2bin(rsa->p, kbuf + key_len);
	   key_len += metadata.rsapriv.lenprimod;
    }
    memcpy(kbuf, &metadata, sizeof(metadata));

    UniquePtr<uint8_t> outPtr(reinterpret_cast<uint8_t*>(malloc(RSA_KEY_BUFFER_SIZE)));
    if (outPtr.get() == NULL) {
        ALOGE("memory allocation is failed");
        return -1;
    }

    *key_blob_length = RSA_KEY_BUFFER_SIZE;

    ret = TEE_KeyImport(kbuf, key_len, outPtr.get(), key_blob_length);
    if (ret != TEE_ERR_NONE) {
        ALOGE("TEE_KeyImport() is failed: %d", ret);
        return -1;
    }

    *key_blob = outPtr.release();

    return 0;
}

static int exynos_km_get_keypair_public(const struct keymaster_device* dev,
        const uint8_t* key_blob, const size_t key_blob_length,
        uint8_t** x509_data, size_t* x509_data_length) {
    uint32_t bin_mod_len;
    uint32_t bin_exp_len;
    teeResult_t ret = TEE_ERR_NONE;

    if (x509_data == NULL || x509_data_length == NULL) {
        ALOGE("output public key buffer == NULL");
        return -1;
    }

    UniquePtr<uint8_t> binModPtr(reinterpret_cast<uint8_t*>(malloc(RSA_KEY_MAX_SIZE)));
    if (binModPtr.get() == NULL) {
        ALOGE("memory allocation is failed");
        return -1;
    }

    UniquePtr<uint8_t> binExpPtr(reinterpret_cast<uint8_t*>(malloc(sizeof(uint32_t))));
    if (binExpPtr.get() == NULL) {
        ALOGE("memory allocation is failed");
        return -1;
    }

    bin_mod_len = RSA_KEY_MAX_SIZE;
    bin_exp_len = sizeof(uint32_t);

    ret = TEE_GetPubKey(key_blob, key_blob_length, binModPtr.get(), &bin_mod_len, binExpPtr.get(),
			&bin_exp_len);
    if (ret != TEE_ERR_NONE) {
        ALOGE("TEE_GetPubKey() is failed: %d", ret);
        return -1;
    }

    Unique_BIGNUM bn_mod(BN_new());
    if (bn_mod.get() == NULL) {
        ALOGE("memory allocation is failed");
        return -1;
    }

    Unique_BIGNUM bn_exp(BN_new());
    if (bn_exp.get() == NULL) {
        ALOGE("memory allocation is failed");
        return -1;
    }

    BN_bin2bn(binModPtr.get(), bin_mod_len, bn_mod.get());
    BN_bin2bn(binExpPtr.get(), bin_exp_len, bn_exp.get());

    /* assign to RSA */
    Unique_RSA rsa(RSA_new());
    if (rsa.get() == NULL) {
        logOpenSSLError("rsa.get");
        return -1;
    }

    RSA* rsa_tmp = rsa.get();

    rsa_tmp->n = bn_mod.release();
    rsa_tmp->e = bn_exp.release();

    /* assign to EVP */
    Unique_EVP_PKEY pkey(EVP_PKEY_new());
    if (pkey.get() == NULL) {
        logOpenSSLError("allocate EVP_PKEY");
        return -1;
    }

    if (EVP_PKEY_assign_RSA(pkey.get(), rsa.get()) == 0) {
        logOpenSSLError("assing RSA to EVP_PKEY");
        return -1;
    }
    OWNERSHIP_TRANSFERRED(rsa);

    /* change to x.509 format */
    int len = i2d_PUBKEY(pkey.get(), NULL);
    if (len <= 0) {
        logOpenSSLError("i2d_PUBKEY");
        return -1;
    }

    UniquePtr<uint8_t> key(static_cast<uint8_t*>(malloc(len)));
    if (key.get() == NULL) {
        ALOGE("Could not allocate memory for public key data");
        return -1;
    }

    unsigned char* tmp = reinterpret_cast<unsigned char*>(key.get());
    if (i2d_PUBKEY(pkey.get(), &tmp) != len) {
        logOpenSSLError("Compare results");
        return -1;
    }

    *x509_data_length = len;
    *x509_data = key.release();

    return 0;
}

static int exynos_km_sign_data(const keymaster_device_t* dev,
        const void* params,
        const uint8_t* keyBlob, const size_t keyBlobLength,
        const uint8_t* data, const size_t dataLength,
        uint8_t** signedData, size_t* signedDataLength) {
    teeResult_t ret = TEE_ERR_NONE;

    if (data == NULL) {
        ALOGE("input data to sign == NULL");
        return -1;
    } else if (signedData == NULL || signedDataLength == NULL) {
        ALOGE("output signature buffer == NULL");
        return -1;
    }

    keymaster_rsa_sign_params_t* sign_params = (keymaster_rsa_sign_params_t*) params;
    if (sign_params->digest_type != DIGEST_NONE) {
        ALOGE("Cannot handle digest type %d", sign_params->digest_type);
        return -1;
    } else if (sign_params->padding_type != PADDING_NONE) {
        ALOGE("Cannot handle padding type %d", sign_params->padding_type);
        return -1;
    }

    UniquePtr<uint8_t> signedDataPtr(reinterpret_cast<uint8_t*>(malloc(RSA_KEY_MAX_SIZE)));
    if (signedDataPtr.get() == NULL) {
        ALOGE("memory allocation is failed");
        return -1;
    }

    *signedDataLength = RSA_KEY_MAX_SIZE;

    /* binder gives us read-only mappings we can't use with mobicore */
    void *tmpData = malloc(dataLength);
    memcpy(tmpData, data, dataLength);
    ret = TEE_RSASign(keyBlob, keyBlobLength, (const uint8_t *)tmpData, dataLength, signedDataPtr.get(),
			signedDataLength, TEE_RSA_NODIGEST_NOPADDING);
    free(tmpData);
    if (ret != TEE_ERR_NONE) {
        ALOGE("TEE_RSASign() is failed: %d", ret);
        return -1;
    }

    *signedData = signedDataPtr.release();

    return 0;
}

static int exynos_km_verify_data(const keymaster_device_t* dev,
        const void* params,
        const uint8_t* keyBlob, const size_t keyBlobLength,
        const uint8_t* signedData, const size_t signedDataLength,
        const uint8_t* signature, const size_t signatureLength) {
    bool result;
    teeResult_t ret = TEE_ERR_NONE;

    if (signedData == NULL || signature == NULL) {
        ALOGE("data or signature buffers == NULL");
        return -1;
    }

    keymaster_rsa_sign_params_t* sign_params = (keymaster_rsa_sign_params_t*) params;
    if (sign_params->digest_type != DIGEST_NONE) {
        ALOGE("Cannot handle digest type %d", sign_params->digest_type);
        return -1;
    } else if (sign_params->padding_type != PADDING_NONE) {
        ALOGE("Cannot handle padding type %d", sign_params->padding_type);
        return -1;
    } else if (signatureLength != signedDataLength) {
        ALOGE("signed data length must be signature length");
        return -1;
    }

    void *tmpSignedData = malloc(signedDataLength);
    memcpy(tmpSignedData, signedData, signedDataLength);
    void *tmpSig = malloc(signatureLength);
    memcpy(tmpSig, signature, signatureLength);
    ret = TEE_RSAVerify(keyBlob, keyBlobLength, (const uint8_t*)tmpSignedData, signedDataLength, (const uint8_t *)tmpSig,
			signatureLength, TEE_RSA_NODIGEST_NOPADDING, &result);
    free(tmpSignedData);
    free(tmpSig);
    if (ret != TEE_ERR_NONE) {
        ALOGE("TEE_RSAVerify() is failed: %d", ret);
        return -1;
    }

    return (result == true) ? 0 : -1;
}

/* Close an opened Exynos KM instance */
static int exynos_km_close(hw_device_t *dev) {
    free(dev);
    return 0;
}

/*
 * Generic device handling
 */
static int exynos_km_open(const hw_module_t* module, const char* name,
        hw_device_t** device) {
    if (strcmp(name, KEYSTORE_KEYMASTER) != 0)
        return -EINVAL;

    Unique_keymaster_device_t dev(new keymaster_device_t);
    if (dev.get() == NULL)
        return -ENOMEM;

    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 1;
    dev->common.module = (struct hw_module_t*) module;
    dev->common.close = exynos_km_close;

    dev->flags = 0;

    dev->generate_keypair = exynos_km_generate_keypair;
    dev->import_keypair = exynos_km_import_keypair;
    dev->get_keypair_public = exynos_km_get_keypair_public;
    dev->delete_keypair = NULL;
    dev->delete_all = NULL;
    dev->sign_data = exynos_km_sign_data;
    dev->verify_data = exynos_km_verify_data;

    ERR_load_crypto_strings();
    ERR_load_BIO_strings();

    *device = reinterpret_cast<hw_device_t*>(dev.release());

    return 0;
}

static struct hw_module_methods_t keystore_module_methods = {
    open: exynos_km_open,
};

struct keystore_module HAL_MODULE_INFO_SYM
__attribute__ ((visibility ("default"))) = {
    common: {
        tag: HARDWARE_MODULE_TAG,
        version_major: 1,
        version_minor: 0,
        id: KEYSTORE_HARDWARE_MODULE_ID,
        name: "Keymaster Exynos HAL",
        author: "Samsung S.LSI",
        methods: &keystore_module_methods,
        dso: 0,
        reserved: {},
    },
};
