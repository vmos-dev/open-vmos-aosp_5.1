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

#ifndef SYSTEM_KEYMASTER_ASYMMETRIC_KEY_H
#define SYSTEM_KEYMASTER_ASYMMETRIC_KEY_H

#include <openssl/rsa.h>
#include <openssl/dsa.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>

#include "key.h"

namespace keymaster {

class AsymmetricKey : public Key {
  public:
  protected:
    AsymmetricKey(const KeyBlob& blob, const Logger& logger) : Key(blob, logger) {}
    keymaster_error_t LoadKey(const KeyBlob& blob);

    /**
     * Return a copy of raw key material, in the key's preferred binary format.
     */
    virtual keymaster_error_t key_material(UniquePtr<uint8_t[]>* material, size_t* size) const;

    /**
     * Return a copy of raw key material, in the specified format.
     */
    virtual keymaster_error_t formatted_key_material(keymaster_key_format_t format,
                                                     UniquePtr<uint8_t[]>* material,
                                                     size_t* size) const;

    virtual Operation* CreateOperation(keymaster_purpose_t purpose, keymaster_error_t* error);

  protected:
    AsymmetricKey(const AuthorizationSet& auths, const Logger& logger) : Key(auths, logger) {}

  private:
    virtual int evp_key_type() = 0;
    virtual bool InternalToEvp(EVP_PKEY* pkey) const = 0;
    virtual bool EvpToInternal(const EVP_PKEY* pkey) = 0;
    virtual Operation* CreateOperation(keymaster_purpose_t purpose, keymaster_digest_t digest,
                                       keymaster_padding_t padding, keymaster_error_t* error) = 0;
};

class RsaKey : public AsymmetricKey {
  public:
    static RsaKey* GenerateKey(const AuthorizationSet& key_description, const Logger& logger,
                               keymaster_error_t* error);
    static RsaKey* ImportKey(const AuthorizationSet& key_description, EVP_PKEY* pkey,
                             const Logger& logger, keymaster_error_t* error);
    RsaKey(const KeyBlob& blob, const Logger& logger, keymaster_error_t* error);

    virtual Operation* CreateOperation(keymaster_purpose_t purpose, keymaster_digest_t digest,
                                       keymaster_padding_t padding, keymaster_error_t* error);

  private:
    RsaKey(RSA* rsa_key, const AuthorizationSet& auths, const Logger& logger)
        : AsymmetricKey(auths, logger), rsa_key_(rsa_key) {}

    virtual int evp_key_type() { return EVP_PKEY_RSA; }
    virtual bool InternalToEvp(EVP_PKEY* pkey) const;
    virtual bool EvpToInternal(const EVP_PKEY* pkey);

    struct RSA_Delete {
        void operator()(RSA* p) { RSA_free(p); }
    };

    UniquePtr<RSA, RSA_Delete> rsa_key_;
};

class DsaKey : public AsymmetricKey {
  public:
    static DsaKey* GenerateKey(const AuthorizationSet& key_description, const Logger& logger,
                               keymaster_error_t* error);
    static DsaKey* ImportKey(const AuthorizationSet& key_description, EVP_PKEY* pkey,
                             const Logger& logger, keymaster_error_t* error);
    DsaKey(const KeyBlob& blob, const Logger& logger, keymaster_error_t* error);

    virtual Operation* CreateOperation(keymaster_purpose_t purpose, keymaster_digest_t digest,
                                       keymaster_padding_t padding, keymaster_error_t* error);
    static size_t key_size_bits(DSA* dsa_key);

  private:

    DsaKey(DSA* dsa_key, const AuthorizationSet auths, const Logger& logger)
        : AsymmetricKey(auths, logger), dsa_key_(dsa_key) {}

    virtual int evp_key_type() { return EVP_PKEY_DSA; }
    virtual bool InternalToEvp(EVP_PKEY* pkey) const;
    virtual bool EvpToInternal(const EVP_PKEY* pkey);

    struct DSA_Delete {
        void operator()(DSA* p) { DSA_free(p); }
    };

    UniquePtr<DSA, DSA_Delete> dsa_key_;
};

class EcdsaKey : public AsymmetricKey {
  public:
    static EcdsaKey* GenerateKey(const AuthorizationSet& key_description, const Logger& logger,
                                 keymaster_error_t* error);
    static EcdsaKey* ImportKey(const AuthorizationSet& key_description, EVP_PKEY* pkey,
                             const Logger& logger, keymaster_error_t* error);
    EcdsaKey(const KeyBlob& blob, const Logger& logger, keymaster_error_t* error);

    virtual Operation* CreateOperation(keymaster_purpose_t purpose, keymaster_digest_t digest,
                                       keymaster_padding_t padding, keymaster_error_t* error);

  private:
    EcdsaKey(EC_KEY* ecdsa_key, const AuthorizationSet auths, const Logger& logger)
        : AsymmetricKey(auths, logger), ecdsa_key_(ecdsa_key) {}

    static EC_GROUP* choose_group(size_t key_size_bits);
    static keymaster_error_t get_group_size(const EC_GROUP& group, size_t* key_size_bits);

    virtual int evp_key_type() { return EVP_PKEY_EC; }
    virtual bool InternalToEvp(EVP_PKEY* pkey) const;
    virtual bool EvpToInternal(const EVP_PKEY* pkey);

    struct ECDSA_Delete {
        void operator()(EC_KEY* p) { EC_KEY_free(p); }
    };

    struct EC_GROUP_Delete {
        void operator()(EC_GROUP* p) { EC_GROUP_free(p); }
    };

    UniquePtr<EC_KEY, ECDSA_Delete> ecdsa_key_;
};

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_ASYMMETRIC_KEY_H
