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

#include <openssl/rsa.h>
#include <openssl/evp.h>

#include "rsa_operation.h"
#include "openssl_utils.h"

namespace keymaster {

struct RSA_Delete {
    void operator()(RSA* p) const { RSA_free(p); }
};

RsaOperation::~RsaOperation() {
    if (rsa_key_ != NULL)
        RSA_free(rsa_key_);
}

keymaster_error_t RsaOperation::Update(const Buffer& input, Buffer* /* output */) {
    switch (purpose()) {
    default:
        return KM_ERROR_UNIMPLEMENTED;
    case KM_PURPOSE_SIGN:
    case KM_PURPOSE_VERIFY:
        return StoreData(input);
    }
}

keymaster_error_t RsaOperation::StoreData(const Buffer& input) {
    if (!data_.reserve(data_.available_read() + input.available_read()) ||
        !data_.write(input.peek_read(), input.available_read()))
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    return KM_ERROR_OK;
}

keymaster_error_t RsaSignOperation::Finish(const Buffer& /* signature */, Buffer* output) {
    output->Reinitialize(RSA_size(rsa_key_));
    int bytes_encrypted = RSA_private_encrypt(data_.available_read(), data_.peek_read(),
                                              output->peek_write(), rsa_key_, RSA_NO_PADDING);
    if (bytes_encrypted < 0)
        return KM_ERROR_UNKNOWN_ERROR;
    assert(bytes_encrypted == RSA_size(rsa_key_));
    output->advance_write(bytes_encrypted);
    return KM_ERROR_OK;
}

keymaster_error_t RsaVerifyOperation::Finish(const Buffer& signature, Buffer* /* output */) {

    if ((int)data_.available_read() != RSA_size(rsa_key_))
        return KM_ERROR_INVALID_INPUT_LENGTH;
    if (data_.available_read() != signature.available_read())
        return KM_ERROR_VERIFICATION_FAILED;

    UniquePtr<uint8_t[]> decrypted_data(new uint8_t[RSA_size(rsa_key_)]);
    int bytes_decrypted = RSA_public_decrypt(signature.available_read(), signature.peek_read(),
                                             decrypted_data.get(), rsa_key_, RSA_NO_PADDING);
    if (bytes_decrypted < 0)
        return KM_ERROR_UNKNOWN_ERROR;
    assert(bytes_decrypted == RSA_size(rsa_key_));

    if (memcmp_s(decrypted_data.get(), data_.peek_read(), data_.available_read()) == 0)
        return KM_ERROR_OK;
    return KM_ERROR_VERIFICATION_FAILED;
}

}  // namespace keymaster
