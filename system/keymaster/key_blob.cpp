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

#include <assert.h>

#include <openssl/aes.h>
#include <openssl/sha.h>

#include <keymaster/google_keymaster_utils.h>
#include <keymaster/key_blob.h>

#include "ae.h"

namespace keymaster {

class KeyBlob::AeCtx {
  public:
    AeCtx() : ctx_(ae_allocate(NULL)) {}
    ~AeCtx() {
        ae_clear(ctx_);
        ae_free(ctx_);
    }

    ae_ctx* get() { return ctx_; }

  private:
    ae_ctx* ctx_;
};

const size_t KeyBlob::NONCE_LENGTH;
const size_t KeyBlob::TAG_LENGTH;

KeyBlob::KeyBlob(const AuthorizationSet& enforced, const AuthorizationSet& unenforced,
                 const AuthorizationSet& hidden, const keymaster_key_blob_t& key,
                 const keymaster_key_blob_t& master_key, const uint8_t nonce[NONCE_LENGTH])
    : error_(KM_ERROR_OK), nonce_(new uint8_t[NONCE_LENGTH]), tag_(new uint8_t[TAG_LENGTH]),
      enforced_(enforced), unenforced_(unenforced), hidden_(hidden) {
    if (!nonce_.get() || !tag_.get()) {
        error_ = KM_ERROR_MEMORY_ALLOCATION_FAILED;
        return;
    }
    error_ = KM_ERROR_OK;

    if (enforced_.is_valid() == AuthorizationSet::ALLOCATION_FAILURE ||
        unenforced_.is_valid() == AuthorizationSet::ALLOCATION_FAILURE ||
        hidden_.is_valid() == AuthorizationSet::ALLOCATION_FAILURE) {
        error_ = KM_ERROR_MEMORY_ALLOCATION_FAILED;
        return;
    }

    if (enforced_.is_valid() != AuthorizationSet::OK ||
        unenforced_.is_valid() != AuthorizationSet::OK ||
        hidden_.is_valid() != AuthorizationSet::OK) {
        error_ = KM_ERROR_UNKNOWN_ERROR;
        return;
    }

    if (!ExtractKeyCharacteristics())
        return;

    key_material_length_ = key.key_material_size;
    key_material_.reset(new uint8_t[key_material_length_]);
    encrypted_key_material_.reset(new uint8_t[key_material_length_]);

    if (!key_material_.get() || !encrypted_key_material_.get() || !nonce_.get() || !tag_.get()) {
        error_ = KM_ERROR_MEMORY_ALLOCATION_FAILED;
        return;
    }

    memcpy(nonce_.get(), nonce, NONCE_LENGTH);
    memcpy(key_material_.get(), key.key_material, key_material_length_);
    EncryptKey(master_key);
}

KeyBlob::KeyBlob(const keymaster_key_blob_t& key, const AuthorizationSet& hidden,
                 const keymaster_key_blob_t& master_key)
    : nonce_(new uint8_t[NONCE_LENGTH]), tag_(new uint8_t[TAG_LENGTH]), hidden_(hidden) {
    if (!nonce_.get() || !tag_.get()) {
        error_ = KM_ERROR_MEMORY_ALLOCATION_FAILED;
        return;
    }
    error_ = KM_ERROR_OK;

    const uint8_t* p = key.key_material;
    if (!Deserialize(&p, key.key_material + key.key_material_size))
        return;
    DecryptKey(master_key);
}

KeyBlob::KeyBlob(const uint8_t* key_blob, size_t blob_size)
    : nonce_(new uint8_t[NONCE_LENGTH]), tag_(new uint8_t[TAG_LENGTH]) {
    if (!nonce_.get() || !tag_.get()) {
        error_ = KM_ERROR_MEMORY_ALLOCATION_FAILED;
        return;
    }
    error_ = KM_ERROR_OK;

    if (!Deserialize(&key_blob, key_blob + blob_size))
        return;
}

size_t KeyBlob::SerializedSize() const {
    return NONCE_LENGTH + sizeof(uint32_t) + key_material_length() + TAG_LENGTH +
           enforced_.SerializedSize() + unenforced_.SerializedSize();
}

uint8_t* KeyBlob::Serialize(uint8_t* buf, const uint8_t* end) const {
    const uint8_t* start = buf;
    buf = append_to_buf(buf, end, nonce(), NONCE_LENGTH);
    buf = append_size_and_data_to_buf(buf, end, encrypted_key_material(), key_material_length());
    buf = append_to_buf(buf, end, tag(), TAG_LENGTH);
    buf = enforced_.Serialize(buf, end);
    buf = unenforced_.Serialize(buf, end);
    assert(buf - start == static_cast<ptrdiff_t>(SerializedSize()));
    return buf;
}

bool KeyBlob::Deserialize(const uint8_t** buf_ptr, const uint8_t* end) {
    UniquePtr<uint8_t[]> tmp_key_ptr;
    if (!copy_from_buf(buf_ptr, end, nonce_.get(), NONCE_LENGTH) ||
        !copy_size_and_data_from_buf(buf_ptr, end, &key_material_length_, &tmp_key_ptr) ||
        !copy_from_buf(buf_ptr, end, tag_.get(), TAG_LENGTH) ||
        !enforced_.Deserialize(buf_ptr, end) || !unenforced_.Deserialize(buf_ptr, end)) {
        error_ = KM_ERROR_INVALID_KEY_BLOB;
        return false;
    }

    if (!ExtractKeyCharacteristics())
        return false;

    encrypted_key_material_.reset(tmp_key_ptr.release());
    key_material_.reset(new uint8_t[key_material_length_]);
    return true;
}

void KeyBlob::EncryptKey(const keymaster_key_blob_t& master_key) {
    UniquePtr<AeCtx> ctx(InitializeKeyWrappingContext(master_key, &error_));
    if (error_ != KM_ERROR_OK)
        return;

    int ae_err = ae_encrypt(ctx->get(), nonce_.get(), key_material(), key_material_length(),
                            NULL /* additional data */, 0 /* additional data length */,
                            encrypted_key_material_.get(), tag_.get(), 1 /* final */);
    if (ae_err < 0) {
        error_ = KM_ERROR_UNKNOWN_ERROR;
        return;
    }
    assert(ae_err == static_cast<int>(key_material_length_));
    error_ = KM_ERROR_OK;
}

void KeyBlob::DecryptKey(const keymaster_key_blob_t& master_key) {
    UniquePtr<AeCtx> ctx(InitializeKeyWrappingContext(master_key, &error_));
    if (error_ != KM_ERROR_OK)
        return;

    int ae_err =
        ae_decrypt(ctx->get(), nonce_.get(), encrypted_key_material(), key_material_length(),
                   NULL /* additional data */, 0 /* additional data length */, key_material_.get(),
                   tag_.get(), 1 /* final */);
    if (ae_err == AE_INVALID) {
        // Authentication failed!  Decryption probably succeeded(ish), but we don't want to return
        // any data when the authentication fails, so clear it.
        memset_s(key_material_.get(), 0, key_material_length());
        error_ = KM_ERROR_INVALID_KEY_BLOB;
        return;
    } else if (ae_err < 0) {
        error_ = KM_ERROR_UNKNOWN_ERROR;
        return;
    }
    assert(ae_err == static_cast<int>(key_material_length()));
    error_ = KM_ERROR_OK;
}

KeyBlob::AeCtx* KeyBlob::InitializeKeyWrappingContext(const keymaster_key_blob_t& master_key,
                                                      keymaster_error_t* error) const {
    size_t derivation_data_length;
    UniquePtr<const uint8_t[]> derivation_data(BuildDerivationData(&derivation_data_length));
    if (derivation_data.get() == NULL) {
        *error = KM_ERROR_MEMORY_ALLOCATION_FAILED;
        return NULL;
    }

    *error = KM_ERROR_OK;
    UniquePtr<AeCtx> ctx(new AeCtx);

    SHA256_CTX sha256_ctx;
    UniquePtr<uint8_t[]> hash_buf(new uint8_t[SHA256_DIGEST_LENGTH]);
    Eraser hash_eraser(hash_buf.get(), SHA256_DIGEST_LENGTH);
    UniquePtr<uint8_t[]> derived_key(new uint8_t[AES_BLOCK_SIZE]);
    Eraser derived_key_eraser(derived_key.get(), AES_BLOCK_SIZE);

    if (ctx.get() == NULL || hash_buf.get() == NULL || derived_key.get() == NULL) {
        *error = KM_ERROR_MEMORY_ALLOCATION_FAILED;
        return NULL;
    }

    Eraser sha256_ctx_eraser(sha256_ctx);

    // Hash derivation data.
    SHA256_Init(&sha256_ctx);
    SHA256_Update(&sha256_ctx, derivation_data.get(), derivation_data_length);
    SHA256_Final(hash_buf.get(), &sha256_ctx);

    // Encrypt hash with master key to build derived key.
    AES_KEY aes_key;
    Eraser aes_key_eraser(AES_KEY);
    if (AES_set_encrypt_key(master_key.key_material, master_key.key_material_size * 8, &aes_key) !=
        0) {
        *error = KM_ERROR_UNKNOWN_ERROR;
        return NULL;
    }
    AES_encrypt(hash_buf.get(), derived_key.get(), &aes_key);

    // Set up AES OCB context using derived key.
    if (ae_init(ctx->get(), derived_key.get(), AES_BLOCK_SIZE, NONCE_LENGTH, TAG_LENGTH) ==
        AE_SUCCESS)
        return ctx.release();
    else {
        memset_s(ctx.get(), 0, ae_ctx_sizeof());
        return NULL;
    }
}

const uint8_t* KeyBlob::BuildDerivationData(size_t* derivation_data_length) const {
    *derivation_data_length =
        hidden_.SerializedSize() + enforced_.SerializedSize() + unenforced_.SerializedSize();
    uint8_t* derivation_data = new uint8_t[*derivation_data_length];
    if (derivation_data != NULL) {
        uint8_t* buf = derivation_data;
        uint8_t* end = derivation_data + *derivation_data_length;
        buf = hidden_.Serialize(buf, end);
        buf = enforced_.Serialize(buf, end);
        buf = unenforced_.Serialize(buf, end);
    }
    return derivation_data;
}

bool KeyBlob::ExtractKeyCharacteristics() {
    if (!enforced_.GetTagValue(TAG_ALGORITHM, &algorithm_) &&
        !unenforced_.GetTagValue(TAG_ALGORITHM, &algorithm_)) {
        error_ = KM_ERROR_UNSUPPORTED_ALGORITHM;
        return false;
    }
    if (!enforced_.GetTagValue(TAG_KEY_SIZE, &key_size_bits_) &&
        !unenforced_.GetTagValue(TAG_KEY_SIZE, &key_size_bits_)) {
        error_ = KM_ERROR_UNSUPPORTED_KEY_SIZE;
        return false;
    }
    return true;
}

}  // namespace keymaster
