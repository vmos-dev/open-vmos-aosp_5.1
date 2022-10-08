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

#include <openssl/bn.h>

#include "dsa_operation.h"
#include "openssl_utils.h"

namespace keymaster {

DsaOperation::~DsaOperation() {
    if (dsa_key_ != NULL)
        DSA_free(dsa_key_);
}

keymaster_error_t DsaOperation::Update(const Buffer& input, Buffer* /* output */) {
    switch (purpose()) {
    default:
        return KM_ERROR_UNIMPLEMENTED;
    case KM_PURPOSE_SIGN:
    case KM_PURPOSE_VERIFY:
        return StoreData(input);
    }
}

keymaster_error_t DsaOperation::StoreData(const Buffer& input) {
    if (!data_.reserve(data_.available_read() + input.available_read()) ||
        !data_.write(input.peek_read(), input.available_read()))
        return KM_ERROR_INVALID_INPUT_LENGTH;
    return KM_ERROR_OK;
}

keymaster_error_t DsaSignOperation::Finish(const Buffer& /* signature */, Buffer* output) {
    output->Reinitialize(DSA_size(dsa_key_));
    unsigned int siglen;
    if (!DSA_sign(0 /* type -- ignored */, data_.peek_read(), data_.available_read(),
                  output->peek_write(), &siglen, dsa_key_))
        return KM_ERROR_UNKNOWN_ERROR;
    output->advance_write(siglen);
    return KM_ERROR_OK;
}

keymaster_error_t DsaVerifyOperation::Finish(const Buffer& signature, Buffer* /* output */) {
    if ((int)data_.available_read() != DSA_size(dsa_key_))
        return KM_ERROR_INVALID_INPUT_LENGTH;

    int result = DSA_verify(0 /* type -- ignored */, data_.peek_read(), data_.available_read(),
                            signature.peek_read(), signature.available_read(), dsa_key_);
    if (result < 0)
        return KM_ERROR_UNKNOWN_ERROR;
    else if (result == 0)
        return KM_ERROR_VERIFICATION_FAILED;
    else
        return KM_ERROR_OK;
}

}  // namespace keymaster
