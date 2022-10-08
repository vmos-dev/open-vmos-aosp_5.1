/*
 * Copyright 2014 The Android Open Source Project
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

#include <openssl/x509.h>

#include <keymaster/key_blob.h>

#include "asymmetric_key.h"
#include "openssl_utils.h"

#include "key.h"

namespace keymaster {

struct PKCS8_PRIV_KEY_INFO_Delete {
    void operator()(PKCS8_PRIV_KEY_INFO* p) const { PKCS8_PRIV_KEY_INFO_free(p); }
};

Key::Key(const KeyBlob& blob, const Logger& logger) : logger_(logger) {
    authorizations_.push_back(blob.unenforced());
    authorizations_.push_back(blob.enforced());
}

/* static */
Key* Key::CreateKey(const KeyBlob& blob, const Logger& logger, keymaster_error_t* error) {
    switch (blob.algorithm()) {
    case KM_ALGORITHM_RSA:
        return new RsaKey(blob, logger, error);
    case KM_ALGORITHM_DSA:
        return new DsaKey(blob, logger, error);
    case KM_ALGORITHM_ECDSA:
        return new EcdsaKey(blob, logger, error);
    default:
        *error = KM_ERROR_UNSUPPORTED_ALGORITHM;
        return NULL;
    }
}

/* static */
Key* Key::GenerateKey(const AuthorizationSet& key_description, const Logger& logger,
                      keymaster_error_t* error) {
    keymaster_algorithm_t algorithm;
    if (!key_description.GetTagValue(TAG_ALGORITHM, &algorithm)) {
        *error = KM_ERROR_UNSUPPORTED_ALGORITHM;
        return NULL;
    }

    switch (algorithm) {
    case KM_ALGORITHM_RSA:
        return RsaKey::GenerateKey(key_description, logger, error);
    case KM_ALGORITHM_DSA:
        return DsaKey::GenerateKey(key_description, logger, error);
    case KM_ALGORITHM_ECDSA:
        return EcdsaKey::GenerateKey(key_description, logger, error);
    default:
        *error = KM_ERROR_UNSUPPORTED_ALGORITHM;
        return NULL;
    }
}

/* static */
Key* Key::ImportKey(const AuthorizationSet& key_description, keymaster_key_format_t key_format,
                    const uint8_t* key_data, size_t key_data_length, const Logger& logger,
                    keymaster_error_t* error) {
    *error = KM_ERROR_OK;

    if (key_data == NULL || key_data_length <= 0) {
        *error = KM_ERROR_INVALID_KEY_BLOB;
        return NULL;
    }

    if (key_format != KM_KEY_FORMAT_PKCS8) {
        *error = KM_ERROR_UNSUPPORTED_KEY_FORMAT;
        return NULL;
    }

    UniquePtr<PKCS8_PRIV_KEY_INFO, PKCS8_PRIV_KEY_INFO_Delete> pkcs8(
        d2i_PKCS8_PRIV_KEY_INFO(NULL, &key_data, key_data_length));
    if (pkcs8.get() == NULL) {
        *error = KM_ERROR_MEMORY_ALLOCATION_FAILED;
        return NULL;
    }

    UniquePtr<EVP_PKEY, EVP_PKEY_Delete> pkey(EVP_PKCS82PKEY(pkcs8.get()));
    if (pkey.get() == NULL) {
        *error = KM_ERROR_INVALID_KEY_BLOB;
        return NULL;
    }

    UniquePtr<Key> key;
    switch (EVP_PKEY_type(pkey->type)) {
    case EVP_PKEY_RSA:
        return RsaKey::ImportKey(key_description, pkey.get(), logger, error);
    case EVP_PKEY_DSA:
        return DsaKey::ImportKey(key_description, pkey.get(), logger, error);
    case EVP_PKEY_EC:
        return EcdsaKey::ImportKey(key_description, pkey.get(), logger, error);
    default:
        *error = KM_ERROR_UNSUPPORTED_ALGORITHM;
        return NULL;
    }

    *error = KM_ERROR_UNIMPLEMENTED;
    return NULL;
}

}  // namespace keymaster
