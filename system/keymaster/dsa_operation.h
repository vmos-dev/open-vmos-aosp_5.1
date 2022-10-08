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

#ifndef SYSTEM_KEYMASTER_DSA_OPERATION_H_
#define SYSTEM_KEYMASTER_DSA_OPERATION_H_

#include <openssl/dsa.h>

#include <UniquePtr.h>

#include <keymaster/key_blob.h>

#include "operation.h"

namespace keymaster {

class DsaOperation : public Operation {
  public:
    DsaOperation(keymaster_purpose_t purpose, const Logger& logger, keymaster_digest_t digest,
                 keymaster_padding_t padding, DSA* key)
        : Operation(purpose, logger), dsa_key_(key), digest_(digest), padding_(padding) {}
    ~DsaOperation();

    virtual keymaster_error_t Begin() { return KM_ERROR_OK; }
    virtual keymaster_error_t Update(const Buffer& input, Buffer* output);
    virtual keymaster_error_t Abort() { return KM_ERROR_OK; }

  protected:
    keymaster_error_t StoreData(const Buffer& input);

    DSA* dsa_key_;
    keymaster_digest_t digest_;
    keymaster_padding_t padding_;
    Buffer data_;
};

class DsaSignOperation : public DsaOperation {
  public:
    DsaSignOperation(keymaster_purpose_t purpose, const Logger& logger, keymaster_digest_t digest,
                     keymaster_padding_t padding, DSA* key)
        : DsaOperation(purpose, logger, digest, padding, key) {}
    virtual keymaster_error_t Finish(const Buffer& signature, Buffer* output);
};

class DsaVerifyOperation : public DsaOperation {
  public:
    DsaVerifyOperation(keymaster_purpose_t purpose, const Logger& logger, keymaster_digest_t digest,
                       keymaster_padding_t padding, DSA* key)
        : DsaOperation(purpose, logger, digest, padding, key) {}
    virtual keymaster_error_t Finish(const Buffer& signature, Buffer* output);
};

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_DSA_OPERATION_H_
