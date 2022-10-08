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

#ifndef SYSTEM_KEYMASTER_KEY_BLOB_H_
#define SYSTEM_KEYMASTER_KEY_BLOB_H_

#include <cstddef>

#include <stdint.h>

#include <UniquePtr.h>

#include <keymaster/authorization_set.h>
#include <keymaster/google_keymaster_utils.h>
#include <keymaster/keymaster_defs.h>
#include <keymaster/serializable.h>

namespace keymaster {

/**
 * This class represents a Keymaster key blob, including authorization sets and key material, both
 * encrypted and unencrypted.  It's primary purpose is to serialize and deserialize blob arrays, and
 * provide access to the data in the blob.
 */
class KeyBlob : public Serializable {
  public:
    static const size_t NONCE_LENGTH = 12;
    static const size_t TAG_LENGTH = 128 / 8;

    /**
     * Create a KeyBlob containing the specified authorization data and key material.  The copy of
     * \p key will be encrypted with key derived from \p master_key, using OCB authenticated
     * encryption with \p nonce.  It is critically important that nonces NEVER be reused.  The most
     * convenient way to accomplish that is to choose them randomly (assuming good randomness, that
     * means there's a probability of reuse of one in 2^96).
     *
     * Note that this interface abuses \p keymaster_key_blob_t a bit.  Normally, that struct is used
     * to contain a full Keymaster blob, i.e. what KeyBlob is designed to create and manage.  In
     * this case we're using it to hold pure key material without any of the additional structure
     * needed for a true Keymaster key.
     *
     * IMPORTANT: After constructing a KeyBlob, call error() to verify that the blob is usable.
     */
    KeyBlob(const AuthorizationSet& enforced, const AuthorizationSet& unenforced,
            const AuthorizationSet& hidden, const keymaster_key_blob_t& key,
            const keymaster_key_blob_t& master_key, const uint8_t nonce[NONCE_LENGTH]);

    /**
     * Create a KeyBlob, reconstituting it from the encrypted material in \p encrypted_key,
     * decrypted with key derived from \p master_key.  The KeyBlob takes ownership of the \p
     * keymaster_blob.key_material.
     *
     * Note, again, that \p master_key here is an abuse of \p keymaster_key_blob_t, since it
     * is just key material, not a full Keymaster blob.
     *
     * IMPORTANT: After constructing a KeyBlob, call error() to verify that the blob is usable.
     */
    KeyBlob(const keymaster_key_blob_t& keymaster_blob, const AuthorizationSet& hidden,
            const keymaster_key_blob_t& master_key);

    /**
     * Create a KeyBlob, extracting the enforced and unenforced sets, but not decrypting the key, or
     * even keeping it.  The KeyBlob does *not* take ownership of key_blob.
     *
     * IMPORTANT: After constructing a KeyBlob, call error() to verify that the blob is usable.
     */
    KeyBlob(const uint8_t* key_blob, size_t blob_size);

    ~KeyBlob() {
        memset_s(key_material_.get(), 0, key_material_length_);
        // The following aren't sensitive, but clear them anyway.
        memset_s(encrypted_key_material_.get(), 0, key_material_length_);
        memset_s(nonce_.get(), 0, NONCE_LENGTH);
        memset_s(tag_.get(), 0, TAG_LENGTH);
        // AuthorizationSets clear themselves.
    }

    size_t SerializedSize() const;
    uint8_t* Serialize(uint8_t* buf, const uint8_t* end) const;
    bool Deserialize(const uint8_t** buf_ptr, const uint8_t* end);

    /**
     * Decrypt encrypted key.  Call this after calling "Deserialize". Until it's called,
     * key_material() will return a pointer to an uninitialized buffer.  Sets error if there is a
     * problem.
     */
    void DecryptKey(const keymaster_key_blob_t& master_key);

    /**
     * Returns KM_ERROR_OK if all is well, or an appropriate error code if there is a problem.  This
     * error code should be checked after constructing or deserializing/decrypting, and if it does
     * not return KM_ERROR_OK, then don't call any other methods.
     */
    inline keymaster_error_t error() { return error_; }

    inline const uint8_t* nonce() const { return nonce_.get(); }
    inline const uint8_t* key_material() const { return key_material_.get(); }
    inline const uint8_t* encrypted_key_material() const { return encrypted_key_material_.get(); }
    inline size_t key_material_length() const { return key_material_length_; }
    inline const uint8_t* tag() const { return tag_.get(); }

    inline const AuthorizationSet& enforced() const { return enforced_; }
    inline const AuthorizationSet& unenforced() const { return unenforced_; }
    inline const AuthorizationSet& hidden() const { return hidden_; }
    inline keymaster_algorithm_t algorithm() const { return algorithm_; }
    inline size_t key_size_bits() const { return key_size_bits_; }

  private:
    void EncryptKey(const keymaster_key_blob_t& master_key);
    bool ExtractKeyCharacteristics();

    /**
     * Create an AES_OCB context initialized with a key derived using \p master_key and the
     * authorizations.
     */
    class AeCtx;
    AeCtx* InitializeKeyWrappingContext(const keymaster_key_blob_t& master_key,
                                        keymaster_error_t* error) const;

    const uint8_t* BuildDerivationData(size_t* derivation_data_len) const;

    keymaster_error_t error_;
    UniquePtr<uint8_t[]> nonce_;
    UniquePtr<uint8_t[]> key_material_;
    UniquePtr<uint8_t[]> encrypted_key_material_;
    UniquePtr<uint8_t[]> tag_;
    size_t key_material_length_;
    AuthorizationSet enforced_;
    AuthorizationSet unenforced_;
    AuthorizationSet hidden_;
    keymaster_algorithm_t algorithm_;
    uint32_t key_size_bits_;
};

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_KEY_BLOB_H_
