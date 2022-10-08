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
#include <string.h>

#include <cstddef>

#include <openssl/rand.h>
#include <openssl/x509.h>

#include <UniquePtr.h>

#include <keymaster/google_keymaster.h>
#include <keymaster/google_keymaster_utils.h>
#include <keymaster/key_blob.h>

#include "ae.h"
#include "key.h"
#include "operation.h"

namespace keymaster {

GoogleKeymaster::GoogleKeymaster(size_t operation_table_size, Logger* logger)
    : operation_table_(new OpTableEntry[operation_table_size]),
      operation_table_size_(operation_table_size), logger_(logger) {
    if (operation_table_.get() == NULL)
        operation_table_size_ = 0;
}
GoogleKeymaster::~GoogleKeymaster() {
    for (size_t i = 0; i < operation_table_size_; ++i)
        if (operation_table_[i].operation != NULL)
            delete operation_table_[i].operation;
}

struct AE_CTX_Delete {
    void operator()(ae_ctx* ctx) const { ae_free(ctx); }
};
typedef UniquePtr<ae_ctx, AE_CTX_Delete> Unique_ae_ctx;

keymaster_algorithm_t supported_algorithms[] = {
    KM_ALGORITHM_RSA, KM_ALGORITHM_DSA, KM_ALGORITHM_ECDSA,
};

template <typename T>
bool check_supported(keymaster_algorithm_t algorithm, SupportedResponse<T>* response) {
    if (!array_contains(supported_algorithms, algorithm)) {
        response->error = KM_ERROR_UNSUPPORTED_ALGORITHM;
        return false;
    }
    return true;
}

void
GoogleKeymaster::SupportedAlgorithms(SupportedResponse<keymaster_algorithm_t>* response) const {
    if (response == NULL)
        return;
    response->SetResults(supported_algorithms);
}

void
GoogleKeymaster::SupportedBlockModes(keymaster_algorithm_t algorithm,
                                     SupportedResponse<keymaster_block_mode_t>* response) const {
    if (response == NULL || !check_supported(algorithm, response))
        return;
    response->error = KM_ERROR_OK;
}

keymaster_padding_t supported_padding[] = {KM_PAD_NONE};
void
GoogleKeymaster::SupportedPaddingModes(keymaster_algorithm_t algorithm,
                                       SupportedResponse<keymaster_padding_t>* response) const {
    if (response == NULL || !check_supported(algorithm, response))
        return;

    response->error = KM_ERROR_OK;
    switch (algorithm) {
    case KM_ALGORITHM_RSA:
    case KM_ALGORITHM_DSA:
    case KM_ALGORITHM_ECDSA:
        response->SetResults(supported_padding);
        break;
    default:
        response->results_length = 0;
        break;
    }
}

keymaster_digest_t supported_digests[] = {KM_DIGEST_NONE};
void GoogleKeymaster::SupportedDigests(keymaster_algorithm_t algorithm,
                                       SupportedResponse<keymaster_digest_t>* response) const {
    if (response == NULL || !check_supported(algorithm, response))
        return;

    response->error = KM_ERROR_OK;
    switch (algorithm) {
    case KM_ALGORITHM_RSA:
    case KM_ALGORITHM_DSA:
    case KM_ALGORITHM_ECDSA:
        response->SetResults(supported_digests);
        break;
    default:
        response->results_length = 0;
        break;
    }
}

keymaster_key_format_t supported_import_formats[] = {KM_KEY_FORMAT_PKCS8};
void
GoogleKeymaster::SupportedImportFormats(keymaster_algorithm_t algorithm,
                                        SupportedResponse<keymaster_key_format_t>* response) const {
    if (response == NULL || !check_supported(algorithm, response))
        return;

    response->error = KM_ERROR_OK;
    switch (algorithm) {
    case KM_ALGORITHM_RSA:
    case KM_ALGORITHM_DSA:
    case KM_ALGORITHM_ECDSA:
        response->SetResults(supported_import_formats);
        break;
    default:
        response->results_length = 0;
        break;
    }
}

keymaster_key_format_t supported_export_formats[] = {KM_KEY_FORMAT_X509};
void
GoogleKeymaster::SupportedExportFormats(keymaster_algorithm_t algorithm,
                                        SupportedResponse<keymaster_key_format_t>* response) const {
    if (response == NULL || !check_supported(algorithm, response))
        return;

    response->error = KM_ERROR_OK;
    switch (algorithm) {
    case KM_ALGORITHM_RSA:
    case KM_ALGORITHM_DSA:
    case KM_ALGORITHM_ECDSA:
        response->SetResults(supported_export_formats);
        break;
    default:
        response->results_length = 0;
        break;
    }
}

void GoogleKeymaster::GenerateKey(const GenerateKeyRequest& request,
                                  GenerateKeyResponse* response) {
    if (response == NULL)
        return;

    UniquePtr<Key> key(Key::GenerateKey(request.key_description, logger(), &response->error));
    if (response->error != KM_ERROR_OK)
        return;

    response->error = SerializeKey(key.get(), origin(), &response->key_blob, &response->enforced,
                                   &response->unenforced);
}

void GoogleKeymaster::GetKeyCharacteristics(const GetKeyCharacteristicsRequest& request,
                                            GetKeyCharacteristicsResponse* response) {
    if (response == NULL)
        return;
    response->error = KM_ERROR_UNKNOWN_ERROR;

    UniquePtr<KeyBlob> blob(
        LoadKeyBlob(request.key_blob, request.additional_params, &(response->error)));
    if (blob.get() == NULL)
        return;

    response->enforced.Reinitialize(blob->enforced());
    response->unenforced.Reinitialize(blob->unenforced());
    response->error = KM_ERROR_OK;
}

void GoogleKeymaster::BeginOperation(const BeginOperationRequest& request,
                                     BeginOperationResponse* response) {
    if (response == NULL)
        return;
    response->op_handle = 0;

    UniquePtr<Key> key(LoadKey(request.key_blob, request.additional_params, &response->error));
    if (key.get() == NULL)
        return;

    UniquePtr<Operation> operation(key->CreateOperation(request.purpose, &response->error));
    if (operation.get() == NULL)
        return;

    response->error = operation->Begin();
    if (response->error != KM_ERROR_OK)
        return;

    response->error = AddOperation(operation.release(), &response->op_handle);
}

void GoogleKeymaster::UpdateOperation(const UpdateOperationRequest& request,
                                      UpdateOperationResponse* response) {
    OpTableEntry* entry = FindOperation(request.op_handle);
    if (entry == NULL) {
        response->error = KM_ERROR_INVALID_OPERATION_HANDLE;
        return;
    }

    response->error = entry->operation->Update(request.input, &response->output);
    if (response->error != KM_ERROR_OK) {
        // Any error invalidates the operation.
        DeleteOperation(entry);
    }
}

void GoogleKeymaster::FinishOperation(const FinishOperationRequest& request,
                                      FinishOperationResponse* response) {
    OpTableEntry* entry = FindOperation(request.op_handle);
    if (entry == NULL) {
        response->error = KM_ERROR_INVALID_OPERATION_HANDLE;
        return;
    }

    response->error = entry->operation->Finish(request.signature, &response->output);
    DeleteOperation(entry);
}

keymaster_error_t GoogleKeymaster::AbortOperation(const keymaster_operation_handle_t op_handle) {
    OpTableEntry* entry = FindOperation(op_handle);
    if (entry == NULL)
        return KM_ERROR_INVALID_OPERATION_HANDLE;
    DeleteOperation(entry);
    return KM_ERROR_OK;
}

bool GoogleKeymaster::is_supported_export_format(keymaster_key_format_t test_format) {
    unsigned int index;
    for (index = 0; index < array_length(supported_export_formats); index++) {
        if (test_format == supported_export_formats[index]) {
            return true;
        }
    }

    return false;
}

bool GoogleKeymaster::is_supported_import_format(keymaster_key_format_t test_format) {
    unsigned int index;
    for (index = 0; index < array_length(supported_import_formats); index++) {
        if (test_format == supported_import_formats[index]) {
            return true;
        }
    }

    return false;
}

void GoogleKeymaster::ExportKey(const ExportKeyRequest& request, ExportKeyResponse* response) {
    if (response == NULL)
        return;

    UniquePtr<Key> to_export(
        LoadKey(request.key_blob, request.additional_params, &response->error));
    if (to_export.get() == NULL)
        return;

    UniquePtr<uint8_t[]> out_key;
    size_t size;
    response->error = to_export->formatted_key_material(request.key_format, &out_key, &size);
    if (response->error == KM_ERROR_OK) {
        response->key_data = out_key.release();
        response->key_data_length = size;
    }
}

void GoogleKeymaster::ImportKey(const ImportKeyRequest& request, ImportKeyResponse* response) {
    if (response == NULL)
        return;

    UniquePtr<Key> key(Key::ImportKey(request.key_description, request.key_format, request.key_data,
                                      request.key_data_length, logger(), &response->error));
    if (response->error != KM_ERROR_OK)
        return;

    response->error = SerializeKey(key.get(), KM_ORIGIN_IMPORTED, &response->key_blob,
                                   &response->enforced, &response->unenforced);
}

keymaster_error_t GoogleKeymaster::SerializeKey(const Key* key, keymaster_key_origin_t origin,
                                                keymaster_key_blob_t* keymaster_blob,
                                                AuthorizationSet* enforced,
                                                AuthorizationSet* unenforced) {
    keymaster_error_t error;

    error = SetAuthorizations(key->authorizations(), origin, enforced, unenforced);
    if (error != KM_ERROR_OK)
        return error;

    AuthorizationSet hidden_auths;
    error = BuildHiddenAuthorizations(key->authorizations(), &hidden_auths);
    if (error != KM_ERROR_OK)
        return error;

    UniquePtr<uint8_t[]> key_material;
    size_t key_material_size;
    error = key->key_material(&key_material, &key_material_size);
    if (error != KM_ERROR_OK)
        return error;

    uint8_t nonce[KeyBlob::NONCE_LENGTH];
    GenerateNonce(nonce, array_size(nonce));

    keymaster_key_blob_t key_data = {key_material.get(), key_material_size};
    UniquePtr<KeyBlob> blob(
        new KeyBlob(*enforced, *unenforced, hidden_auths, key_data, MasterKey(), nonce));
    if (blob.get() == NULL)
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    if (blob->error() != KM_ERROR_OK)
        return blob->error();

    size_t size = blob->SerializedSize();
    UniquePtr<uint8_t[]> blob_bytes(new uint8_t[size]);
    if (blob_bytes.get() == NULL)
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;

    blob->Serialize(blob_bytes.get(), blob_bytes.get() + size);
    keymaster_blob->key_material_size = size;
    keymaster_blob->key_material = blob_bytes.release();

    return KM_ERROR_OK;
}

Key* GoogleKeymaster::LoadKey(const keymaster_key_blob_t& key,
                              const AuthorizationSet& client_params, keymaster_error_t* error) {
    UniquePtr<KeyBlob> blob(LoadKeyBlob(key, client_params, error));
    if (*error != KM_ERROR_OK)
        return NULL;
    return Key::CreateKey(*blob, logger(), error);
}

KeyBlob* GoogleKeymaster::LoadKeyBlob(const keymaster_key_blob_t& key,
                                      const AuthorizationSet& client_params,
                                      keymaster_error_t* error) {
    AuthorizationSet hidden;
    BuildHiddenAuthorizations(client_params, &hidden);
    UniquePtr<KeyBlob> blob(new KeyBlob(key, hidden, MasterKey()));
    if (blob.get() == NULL) {
        *error = KM_ERROR_MEMORY_ALLOCATION_FAILED;
        return NULL;
    } else if (blob->error() != KM_ERROR_OK) {
        *error = blob->error();
        return NULL;
    }
    *error = KM_ERROR_OK;
    return blob.release();
}

static keymaster_error_t TranslateAuthorizationSetError(AuthorizationSet::Error err) {
    switch (err) {
    case AuthorizationSet::OK:
        return KM_ERROR_OK;
    case AuthorizationSet::ALLOCATION_FAILURE:
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    case AuthorizationSet::MALFORMED_DATA:
        return KM_ERROR_UNKNOWN_ERROR;
    }
    return KM_ERROR_OK;
}

keymaster_error_t GoogleKeymaster::SetAuthorizations(const AuthorizationSet& key_description,
                                                     keymaster_key_origin_t origin,
                                                     AuthorizationSet* enforced,
                                                     AuthorizationSet* unenforced) {
    for (size_t i = 0; i < key_description.size(); ++i) {
        switch (key_description[i].tag) {
        // These cannot be specified by the client.
        case KM_TAG_ROOT_OF_TRUST:
        case KM_TAG_ORIGIN:
            return KM_ERROR_INVALID_TAG;

        // These don't work.
        case KM_TAG_ROLLBACK_RESISTANT:
            return KM_ERROR_UNSUPPORTED_TAG;

        // These are hidden.
        case KM_TAG_APPLICATION_ID:
        case KM_TAG_APPLICATION_DATA:
            break;

        // Everything else we just copy into the appropriate set.
        default:
            AddAuthorization(key_description[i], enforced, unenforced);
            break;
        }
    }

    AddAuthorization(Authorization(TAG_CREATION_DATETIME, java_time(time(NULL))), enforced,
                     unenforced);
    AddAuthorization(Authorization(TAG_ORIGIN, origin), enforced, unenforced);

    if (enforced->is_valid() != AuthorizationSet::OK)
        return TranslateAuthorizationSetError(enforced->is_valid());

    return TranslateAuthorizationSetError(unenforced->is_valid());
}

keymaster_error_t GoogleKeymaster::BuildHiddenAuthorizations(const AuthorizationSet& input_set,
                                                             AuthorizationSet* hidden) {
    keymaster_blob_t entry;
    if (input_set.GetTagValue(TAG_APPLICATION_ID, &entry))
        hidden->push_back(TAG_APPLICATION_ID, entry.data, entry.data_length);
    if (input_set.GetTagValue(TAG_APPLICATION_DATA, &entry))
        hidden->push_back(TAG_APPLICATION_DATA, entry.data, entry.data_length);
    hidden->push_back(RootOfTrustTag());

    return TranslateAuthorizationSetError(hidden->is_valid());
}

void GoogleKeymaster::AddAuthorization(const keymaster_key_param_t& auth,
                                       AuthorizationSet* enforced, AuthorizationSet* unenforced) {
    if (is_enforced(auth.tag))
        enforced->push_back(auth);
    else
        unenforced->push_back(auth);
}

keymaster_error_t GoogleKeymaster::AddOperation(Operation* operation,
                                                keymaster_operation_handle_t* op_handle) {
    UniquePtr<Operation> op(operation);
    if (RAND_bytes(reinterpret_cast<uint8_t*>(op_handle), sizeof(*op_handle)) == 0)
        return KM_ERROR_UNKNOWN_ERROR;
    if (*op_handle == 0) {
        // Statistically this is vanishingly unlikely, which means if it ever happens in practice,
        // it indicates a broken RNG.
        return KM_ERROR_UNKNOWN_ERROR;
    }
    for (size_t i = 0; i < operation_table_size_; ++i) {
        if (operation_table_[i].operation == NULL) {
            operation_table_[i].operation = op.release();
            operation_table_[i].handle = *op_handle;
            return KM_ERROR_OK;
        }
    }
    return KM_ERROR_TOO_MANY_OPERATIONS;
}

GoogleKeymaster::OpTableEntry*
GoogleKeymaster::FindOperation(keymaster_operation_handle_t op_handle) {
    if (op_handle == 0)
        return NULL;

    for (size_t i = 0; i < operation_table_size_; ++i) {
        if (operation_table_[i].handle == op_handle)
            return operation_table_.get() + i;
    }
    return NULL;
}

void GoogleKeymaster::DeleteOperation(OpTableEntry* entry) {
    delete entry->operation;
    entry->operation = NULL;
    entry->handle = 0;
}

}  // namespace keymaster
