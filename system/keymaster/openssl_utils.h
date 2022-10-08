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

#ifndef SYSTEM_KEYMASTER_OPENSSL_UTILS_H_
#define SYSTEM_KEYMASTER_OPENSSL_UTILS_H_

#include <openssl/evp.h>
#include <openssl/bn.h>

struct EVP_PKEY_Delete {
    void operator()(EVP_PKEY* p) const { EVP_PKEY_free(p); }
};

struct BIGNUM_Delete {
    void operator()(BIGNUM* p) const { BN_free(p); }
};

/**
 * Many OpenSSL APIs take ownership of an argument on success but don't free the argument on
 * failure. This means we need to tell our scoped pointers when we've transferred ownership, without
 * triggering a warning by not using the result of release().
 */
template <typename T, typename Delete_T>
inline void release_because_ownership_transferred(UniquePtr<T, Delete_T>& p) {
    T* val __attribute__((unused)) = p.release();
}

inline void convert_bn_to_blob(BIGNUM* bn, keymaster_blob_t* blob) {
    blob->data_length = BN_num_bytes(bn);
    blob->data = new uint8_t[blob->data_length];
    BN_bn2bin(bn, const_cast<uint8_t*>(blob->data));
}

#endif  // SYSTEM_KEYMASTER_OPENSSL_UTILS_H_
