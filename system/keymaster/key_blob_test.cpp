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

#include <algorithm>

#include <gtest/gtest.h>

#include <openssl/engine.h>

#include <keymaster/authorization_set.h>
#include <keymaster/google_keymaster_utils.h>
#include <keymaster/keymaster_tags.h>

#include <keymaster/key_blob.h>

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();
    // Clean up stuff OpenSSL leaves around, so Valgrind doesn't complain.
    CRYPTO_cleanup_all_ex_data();
    ERR_free_strings();
    return result;
}

namespace keymaster {

namespace test {

const uint8_t master_key_data[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const uint8_t key_data[5] = {21, 22, 23, 24, 25};
const uint8_t nonce[KeyBlob::NONCE_LENGTH]{12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};

class KeyBlobTest : public testing::Test {
  protected:
    KeyBlobTest() {
        key_.key_material = const_cast<uint8_t*>(key_data);
        key_.key_material_size = array_size(key_data);
        master_key_.key_material = const_cast<uint8_t*>(master_key_data);
        master_key_.key_material_size = array_size(master_key_data);

        enforced_.push_back(TAG_ALGORITHM, KM_ALGORITHM_RSA);
        enforced_.push_back(TAG_KEY_SIZE, 256);
        enforced_.push_back(TAG_BLOB_USAGE_REQUIREMENTS, KM_BLOB_STANDALONE);
        enforced_.push_back(TAG_MIN_SECONDS_BETWEEN_OPS, 10);
        enforced_.push_back(TAG_ALL_USERS);
        enforced_.push_back(TAG_NO_AUTH_REQUIRED);
        enforced_.push_back(TAG_ORIGIN, KM_ORIGIN_HARDWARE);

        unenforced_.push_back(TAG_ACTIVE_DATETIME, 10);
        unenforced_.push_back(TAG_ORIGINATION_EXPIRE_DATETIME, 100);
        unenforced_.push_back(TAG_CREATION_DATETIME, 10);
        unenforced_.push_back(TAG_CHUNK_LENGTH, 10);

        hidden_.push_back(TAG_ROOT_OF_TRUST, "foo", 3);
        hidden_.push_back(TAG_APPLICATION_ID, "my_app", 6);

        blob_.reset(new KeyBlob(enforced_, unenforced_, hidden_, key_, master_key_, nonce));
    }

    AuthorizationSet enforced_;
    AuthorizationSet unenforced_;
    AuthorizationSet hidden_;

    UniquePtr<KeyBlob> blob_;

    keymaster_key_blob_t key_;
    keymaster_key_blob_t master_key_;
};

TEST_F(KeyBlobTest, EncryptDecrypt) {
    size_t size = blob_->SerializedSize();
    UniquePtr<uint8_t[]> serialized_blob(new uint8_t[size]);
    blob_->Serialize(serialized_blob.get(), serialized_blob.get() + size);

    // key_data shouldn't be anywhere in the blob.
    uint8_t* begin = serialized_blob.get();
    uint8_t* end = begin + size;
    EXPECT_EQ(end, std::search(begin, end, key_data, key_data + array_size(key_data)));

    // Recover the key material.
    keymaster_key_blob_t encrypted_blob = {serialized_blob.get(), size};
    KeyBlob deserialized(encrypted_blob, hidden_, master_key_);
    EXPECT_EQ(KM_ERROR_OK, deserialized.error());
    EXPECT_EQ(0, memcmp(deserialized.key_material(), key_data, array_size(key_data)));
}

TEST_F(KeyBlobTest, WrongKeyLength) {
    size_t size = blob_->SerializedSize();
    UniquePtr<uint8_t[]> serialized_blob(new uint8_t[size]);
    blob_->Serialize(serialized_blob.get(), serialized_blob.get() + size);

    // Modify the key length
    serialized_blob[KeyBlob::NONCE_LENGTH]++;

    // Decrypting with wrong nonce should fail.
    keymaster_key_blob_t encrypted_blob = {serialized_blob.get(), size};
    KeyBlob deserialized(encrypted_blob, hidden_, master_key_);
    EXPECT_EQ(KM_ERROR_INVALID_KEY_BLOB, deserialized.error());
}

TEST_F(KeyBlobTest, WrongNonce) {
    size_t size = blob_->SerializedSize();
    UniquePtr<uint8_t[]> serialized_blob(new uint8_t[size]);
    blob_->Serialize(serialized_blob.get(), serialized_blob.get() + size);

    // Find the nonce, then modify it.
    uint8_t* begin = serialized_blob.get();
    uint8_t* end = begin + size;
    auto nonce_ptr = std::search(begin, end, nonce, nonce + array_size(nonce));
    ASSERT_NE(nonce_ptr, end);
    EXPECT_EQ(end, std::search(nonce_ptr + 1, end, nonce, nonce + array_size(nonce)));
    (*nonce_ptr)++;

    // Decrypting with wrong nonce should fail.
    keymaster_key_blob_t encrypted_blob = {serialized_blob.get(), size};
    KeyBlob deserialized(encrypted_blob, hidden_, master_key_);
    EXPECT_EQ(KM_ERROR_INVALID_KEY_BLOB, deserialized.error());
    EXPECT_NE(0, memcmp(deserialized.key_material(), key_data, array_size(key_data)));
}

TEST_F(KeyBlobTest, WrongTag) {
    size_t size = blob_->SerializedSize();
    UniquePtr<uint8_t[]> serialized_blob(new uint8_t[size]);
    blob_->Serialize(serialized_blob.get(), serialized_blob.get() + size);

    // Find the tag, them modify it.
    uint8_t* begin = serialized_blob.get();
    uint8_t* end = begin + size;
    auto tag_ptr = std::search(begin, end, blob_->tag(), blob_->tag() + KeyBlob::TAG_LENGTH);
    ASSERT_NE(tag_ptr, end);
    EXPECT_EQ(end, std::search(tag_ptr + 1, end, blob_->tag(), blob_->tag() + KeyBlob::TAG_LENGTH));
    (*tag_ptr)++;

    // Decrypting with wrong tag should fail.
    keymaster_key_blob_t encrypted_blob = {serialized_blob.get(), size};
    KeyBlob deserialized(encrypted_blob, hidden_, master_key_);
    EXPECT_EQ(KM_ERROR_INVALID_KEY_BLOB, deserialized.error());
    EXPECT_NE(0, memcmp(deserialized.key_material(), key_data, array_size(key_data)));
}

TEST_F(KeyBlobTest, WrongCiphertext) {
    size_t size = blob_->SerializedSize();
    UniquePtr<uint8_t[]> serialized_blob(new uint8_t[size]);
    blob_->Serialize(serialized_blob.get(), serialized_blob.get() + size);

    // Find the ciphertext, them modify it.
    uint8_t* begin = serialized_blob.get();
    uint8_t* end = begin + size;
    auto ciphertext_ptr =
        std::search(begin, end, blob_->encrypted_key_material(),
                    blob_->encrypted_key_material() + blob_->key_material_length());
    ASSERT_NE(ciphertext_ptr, end);
    EXPECT_EQ(end, std::search(ciphertext_ptr + 1, end, blob_->encrypted_key_material(),
                               blob_->encrypted_key_material() + blob_->key_material_length()));
    (*ciphertext_ptr)++;

    // Decrypting with wrong tag should fail.
    keymaster_key_blob_t encrypted_blob = {serialized_blob.get(), size};
    KeyBlob deserialized(encrypted_blob, hidden_, master_key_);
    EXPECT_EQ(KM_ERROR_INVALID_KEY_BLOB, deserialized.error());
    EXPECT_NE(0, memcmp(deserialized.key_material(), key_data, array_size(key_data)));
}

TEST_F(KeyBlobTest, WrongMasterKey) {
    size_t size = blob_->SerializedSize();
    UniquePtr<uint8_t[]> serialized_blob(new uint8_t[size]);
    blob_->Serialize(serialized_blob.get(), serialized_blob.get() + size);

    uint8_t wrong_master_data[] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    keymaster_key_blob_t wrong_master;
    wrong_master.key_material = wrong_master_data;
    wrong_master.key_material_size = array_size(wrong_master_data);

    // Decrypting with wrong master key should fail.
    keymaster_key_blob_t encrypted_blob = {serialized_blob.get(), size};
    KeyBlob deserialized(encrypted_blob, hidden_, wrong_master);
    EXPECT_EQ(KM_ERROR_INVALID_KEY_BLOB, deserialized.error());
    EXPECT_NE(0, memcmp(deserialized.key_material(), key_data, array_size(key_data)));
}

TEST_F(KeyBlobTest, WrongEnforced) {
    size_t size = blob_->SerializedSize();
    UniquePtr<uint8_t[]> serialized_blob(new uint8_t[size]);
    blob_->Serialize(serialized_blob.get(), serialized_blob.get() + size);
    uint8_t* begin = serialized_blob.get();
    uint8_t* end = begin + size;

    // Find enforced serialization data and modify it.
    size_t enforced_size = enforced_.SerializedSize();
    UniquePtr<uint8_t[]> enforced_data(new uint8_t[enforced_size]);
    enforced_.Serialize(enforced_data.get(), enforced_data.get() + enforced_size);

    auto enforced_ptr =
        std::search(begin, end, enforced_data.get(), enforced_data.get() + enforced_size);
    ASSERT_NE(end, enforced_ptr);
    EXPECT_EQ(end, std::search(enforced_ptr + 1, end, enforced_data.get(),
                               enforced_data.get() + enforced_size));
    (*(enforced_ptr + enforced_size - 1))++;

    // Decrypting with wrong unenforced data should fail.
    keymaster_key_blob_t encrypted_blob = {serialized_blob.get(), size};
    KeyBlob deserialized(encrypted_blob, hidden_, master_key_);
    EXPECT_EQ(KM_ERROR_INVALID_KEY_BLOB, deserialized.error());
}

TEST_F(KeyBlobTest, WrongUnenforced) {
    size_t size = blob_->SerializedSize();
    UniquePtr<uint8_t[]> serialized_blob(new uint8_t[size]);
    blob_->Serialize(serialized_blob.get(), serialized_blob.get() + size);
    uint8_t* begin = serialized_blob.get();
    uint8_t* end = begin + size;

    // Find unenforced serialization data and modify it.
    size_t unenforced_size = unenforced_.SerializedSize();
    UniquePtr<uint8_t[]> unenforced_data(new uint8_t[unenforced_size]);
    unenforced_.Serialize(unenforced_data.get(), unenforced_data.get() + unenforced_size);

    auto unenforced_ptr =
        std::search(begin, end, unenforced_data.get(), unenforced_data.get() + unenforced_size);
    ASSERT_NE(end, unenforced_ptr);
    EXPECT_EQ(end, std::search(unenforced_ptr + 1, end, unenforced_data.get(),
                               unenforced_data.get() + unenforced_size));
    (*(unenforced_ptr + unenforced_size - 1))++;

    // Decrypting with wrong unenforced data should fail.
    keymaster_key_blob_t encrypted_blob = {serialized_blob.get(), size};
    KeyBlob deserialized(encrypted_blob, hidden_, master_key_);
    EXPECT_EQ(KM_ERROR_INVALID_KEY_BLOB, deserialized.error());
}

TEST_F(KeyBlobTest, EmptyHidden) {
    size_t size = blob_->SerializedSize();
    UniquePtr<uint8_t[]> serialized_blob(new uint8_t[size]);
    blob_->Serialize(serialized_blob.get(), serialized_blob.get() + size);
    uint8_t* begin = serialized_blob.get();
    uint8_t* end = begin + size;

    AuthorizationSet wrong_hidden;

    // Decrypting with wrong hidden data should fail.
    keymaster_key_blob_t encrypted_blob = {serialized_blob.get(), size};
    KeyBlob deserialized(encrypted_blob, wrong_hidden, master_key_);
    EXPECT_EQ(KM_ERROR_INVALID_KEY_BLOB, deserialized.error());
}

TEST_F(KeyBlobTest, WrongRootOfTrust) {
    size_t size = blob_->SerializedSize();
    UniquePtr<uint8_t[]> serialized_blob(new uint8_t[size]);
    blob_->Serialize(serialized_blob.get(), serialized_blob.get() + size);
    uint8_t* begin = serialized_blob.get();
    uint8_t* end = begin + size;

    AuthorizationSet wrong_hidden;
    wrong_hidden.push_back(TAG_ROOT_OF_TRUST, "bar", 3);
    wrong_hidden.push_back(TAG_APPLICATION_ID, "my_app", 6);

    // Decrypting with wrong hidden data should fail.
    keymaster_key_blob_t encrypted_blob = {serialized_blob.get(), size};
    KeyBlob deserialized(encrypted_blob, wrong_hidden, master_key_);
    EXPECT_EQ(KM_ERROR_INVALID_KEY_BLOB, deserialized.error());
}

TEST_F(KeyBlobTest, WrongAppId) {
    size_t size = blob_->SerializedSize();
    UniquePtr<uint8_t[]> serialized_blob(new uint8_t[size]);
    blob_->Serialize(serialized_blob.get(), serialized_blob.get() + size);
    uint8_t* begin = serialized_blob.get();
    uint8_t* end = begin + size;

    AuthorizationSet wrong_hidden;
    wrong_hidden.push_back(TAG_ROOT_OF_TRUST, "foo", 3);
    wrong_hidden.push_back(TAG_APPLICATION_ID, "your_app", 7);

    // Decrypting with wrong hidden data should fail.
    keymaster_key_blob_t encrypted_blob = {serialized_blob.get(), size};
    KeyBlob deserialized(encrypted_blob, wrong_hidden, master_key_);
    EXPECT_EQ(KM_ERROR_INVALID_KEY_BLOB, deserialized.error());
}

}  // namespace test
}  // namespace keymaster
