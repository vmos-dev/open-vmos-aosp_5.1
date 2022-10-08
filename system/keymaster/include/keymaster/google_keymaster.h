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

#ifndef SYSTEM_KEYMASTER_GOOGLE_KEYMASTER_H_
#define SYSTEM_KEYMASTER_GOOGLE_KEYMASTER_H_

#include <keymaster/authorization_set.h>
#include <keymaster/google_keymaster_messages.h>
#include <keymaster/logger.h>

namespace keymaster {

class Key;
class KeyBlob;
class Operation;

/**
 * OpenSSL-based Keymaster backing implementation, for use as a pure software implmentation
 * (softkeymaster) and in a trusted execution environment (TEE), like ARM TrustZone.  This class
 * doesn't actually implement the Keymaster HAL interface, instead it implements an alternative API
 * which is similar to and based upon the HAL, but uses C++ "message" classes which support
 * serialization.
 *
 * For non-secure, pure software implementation there is a HAL translation layer that converts the
 * HAL's parameters to and from the message representations, which are then passed in to this
 * API.
 *
 * For secure implementation there is another HAL translation layer that serializes the messages to
 * the TEE. In the TEE implementation there's another component which deserializes the messages,
 * extracts the relevant parameters and calls this API.
 */
class GoogleKeymaster {
  public:
    GoogleKeymaster(size_t operation_table_size, Logger* logger);
    virtual ~GoogleKeymaster();

    void SupportedAlgorithms(SupportedResponse<keymaster_algorithm_t>* response) const;
    void SupportedBlockModes(keymaster_algorithm_t algorithm,
                             SupportedResponse<keymaster_block_mode_t>* response) const;
    void SupportedPaddingModes(keymaster_algorithm_t algorithm,
                               SupportedResponse<keymaster_padding_t>* response) const;
    void SupportedDigests(keymaster_algorithm_t algorithm,
                          SupportedResponse<keymaster_digest_t>* response) const;
    void SupportedImportFormats(keymaster_algorithm_t algorithm,
                                SupportedResponse<keymaster_key_format_t>* response) const;
    void SupportedExportFormats(keymaster_algorithm_t algorithm,
                                SupportedResponse<keymaster_key_format_t>* response) const;

    virtual keymaster_error_t AddRngEntropy(AddEntropyRequest& /* request */) {
        // Not going to implement until post-L.
        return KM_ERROR_UNIMPLEMENTED;
    }
    void GenerateKey(const GenerateKeyRequest& request, GenerateKeyResponse* response);
    void GetKeyCharacteristics(const GetKeyCharacteristicsRequest& request,
                               GetKeyCharacteristicsResponse* response);
    void Rescope(const RescopeRequest& /* request */, RescopeResponse* response) {
        // Not going to implement until post-L.
        response->error = KM_ERROR_UNIMPLEMENTED;
    }
    void ImportKey(const ImportKeyRequest& request, ImportKeyResponse* response);
    void ExportKey(const ExportKeyRequest& request, ExportKeyResponse* response);
    void BeginOperation(const BeginOperationRequest& request, BeginOperationResponse* response);
    void UpdateOperation(const UpdateOperationRequest& request, UpdateOperationResponse* response);
    void FinishOperation(const FinishOperationRequest& request, FinishOperationResponse* response);
    keymaster_error_t AbortOperation(const keymaster_operation_handle_t op_handle);

    const Logger& logger() const { return *logger_; }

  private:
    virtual bool is_enforced(keymaster_tag_t tag) = 0;
    virtual keymaster_key_origin_t origin() = 0;
    virtual keymaster_key_param_t RootOfTrustTag() = 0;
    virtual keymaster_key_blob_t MasterKey() = 0;
    virtual void GenerateNonce(uint8_t* nonce, size_t length) = 0;

    keymaster_error_t SerializeKey(const Key* key, keymaster_key_origin_t origin,
                                   keymaster_key_blob_t* keymaster_blob, AuthorizationSet* enforced,
                                   AuthorizationSet* unenforced);
    Key* LoadKey(const keymaster_key_blob_t& key, const AuthorizationSet& client_params,
                 keymaster_error_t* error);
    KeyBlob* LoadKeyBlob(const keymaster_key_blob_t& key, const AuthorizationSet& client_params,
                         keymaster_error_t* error);

    keymaster_error_t SetAuthorizations(const AuthorizationSet& key_description,
                                        keymaster_key_origin_t origin, AuthorizationSet* enforced,
                                        AuthorizationSet* unenforced);
    keymaster_error_t BuildHiddenAuthorizations(const AuthorizationSet& input_set,
                                                AuthorizationSet* hidden);

    void AddAuthorization(const keymaster_key_param_t& auth, AuthorizationSet* enforced,
                          AuthorizationSet* unenforced);
    bool GenerateRsa(const AuthorizationSet& key_auths, GenerateKeyResponse* response,
                     AuthorizationSet* hidden_auths);
    bool GenerateDsa(const AuthorizationSet& key_auths, GenerateKeyResponse* response,
                     AuthorizationSet* hidden_auths);
    bool GenerateEcdsa(const AuthorizationSet& key_auths, GenerateKeyResponse* response,
                       AuthorizationSet* hidden_auths);
    keymaster_error_t WrapKey(const uint8_t* key_material, size_t key_material_length,
                              KeyBlob* blob);
    keymaster_error_t UnwrapKey(const KeyBlob* blob, uint8_t* key, size_t key_length);

    struct OpTableEntry {
        OpTableEntry() {
            handle = 0;
            operation = NULL;
        }
        keymaster_operation_handle_t handle;
        Operation* operation;
    };

    keymaster_error_t AddOperation(Operation* operation, keymaster_operation_handle_t* op_handle);
    OpTableEntry* FindOperation(keymaster_operation_handle_t op_handle);
    void DeleteOperation(OpTableEntry* entry);
    bool is_supported_export_format(keymaster_key_format_t test_format);
    bool is_supported_import_format(keymaster_key_format_t test_format);

    UniquePtr<OpTableEntry[]> operation_table_;
    size_t operation_table_size_;
    UniquePtr<Logger> logger_;
};

}  // namespace keymaster

#endif  //  SYSTEM_KEYMASTER_GOOGLE_KEYMASTER_H_
