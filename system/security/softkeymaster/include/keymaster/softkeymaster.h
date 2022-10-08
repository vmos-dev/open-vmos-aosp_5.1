/*
 * Copyright 2013 The Android Open Source Project
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

#include <hardware/keymaster.h>

#ifndef SOFTKEYMASTER_INCLUDE_KEYMASTER_SOFTKEYMASTER_H_
#define SOFTKEYMASTER_INCLUDE_KEYMASTER_SOFTKEYMASTER_H_

int openssl_generate_keypair(const keymaster_device_t* dev, const keymaster_keypair_t key_type,
                             const void* key_params, uint8_t** keyBlob, size_t* keyBlobLength);

int openssl_import_keypair(const keymaster_device_t* dev, const uint8_t* key,
                           const size_t key_length, uint8_t** key_blob, size_t* key_blob_length);

int openssl_get_keypair_public(const struct keymaster_device* dev, const uint8_t* key_blob,
                               const size_t key_blob_length, uint8_t** x509_data,
                               size_t* x509_data_length);

int openssl_sign_data(const keymaster_device_t* dev, const void* params, const uint8_t* keyBlob,
                      const size_t keyBlobLength, const uint8_t* data, const size_t dataLength,
                      uint8_t** signedData, size_t* signedDataLength);

int openssl_verify_data(const keymaster_device_t* dev, const void* params, const uint8_t* keyBlob,
                        const size_t keyBlobLength, const uint8_t* signedData,
                        const size_t signedDataLength, const uint8_t* signature,
                        const size_t signatureLength);

#endif  // SOFTKEYMASTER_INCLUDE_KEYMASTER_SOFTKEYMASTER_H_
