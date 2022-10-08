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

#include <keymaster/google_keymaster_messages.h>
#include <keymaster/google_keymaster_utils.h>

namespace keymaster {

size_t KeymasterResponse::SerializedSize() const {
    if (error != KM_ERROR_OK)
        return sizeof(int32_t);
    else
        return sizeof(int32_t) + NonErrorSerializedSize();
}

uint8_t* KeymasterResponse::Serialize(uint8_t* buf, const uint8_t* end) const {
    buf = append_uint32_to_buf(buf, end, static_cast<uint32_t>(error));
    if (error == KM_ERROR_OK)
        buf = NonErrorSerialize(buf, end);
    return buf;
}

bool KeymasterResponse::Deserialize(const uint8_t** buf_ptr, const uint8_t* end) {
    if (!copy_uint32_from_buf(buf_ptr, end, &error))
        return false;
    if (error != KM_ERROR_OK)
        return true;
    return NonErrorDeserialize(buf_ptr, end);
}

size_t SupportedAlgorithmsResponse::NonErrorSerializedSize() const {
    return sizeof(uint32_t) + sizeof(uint32_t) * algorithms_length;
}

uint8_t* SupportedAlgorithmsResponse::NonErrorSerialize(uint8_t* buf, const uint8_t* end) const {
    return append_uint32_array_to_buf(buf, end, algorithms, algorithms_length);
}

bool SupportedAlgorithmsResponse::NonErrorDeserialize(const uint8_t** buf_ptr, const uint8_t* end) {
    delete[] algorithms;
    algorithms = NULL;
    UniquePtr<keymaster_algorithm_t[]> deserialized_algorithms;
    if (!copy_uint32_array_from_buf(buf_ptr, end, &deserialized_algorithms, &algorithms_length))
        return false;
    algorithms = deserialized_algorithms.release();
    return true;
}

GenerateKeyResponse::~GenerateKeyResponse() {
    delete[] key_blob.key_material;
}

size_t GenerateKeyResponse::NonErrorSerializedSize() const {
    return sizeof(uint32_t) /* key size */ + key_blob.key_material_size +
           enforced.SerializedSize() + unenforced.SerializedSize();
}

uint8_t* GenerateKeyResponse::NonErrorSerialize(uint8_t* buf, const uint8_t* end) const {
    buf = append_size_and_data_to_buf(buf, end, key_blob.key_material, key_blob.key_material_size);
    buf = enforced.Serialize(buf, end);
    return unenforced.Serialize(buf, end);
}

bool GenerateKeyResponse::NonErrorDeserialize(const uint8_t** buf_ptr, const uint8_t* end) {
    delete[] key_blob.key_material;
    key_blob.key_material = NULL;
    UniquePtr<uint8_t[]> deserialized_key_material;
    if (!copy_size_and_data_from_buf(buf_ptr, end, &key_blob.key_material_size,
                                     &deserialized_key_material) ||
        !enforced.Deserialize(buf_ptr, end) || !unenforced.Deserialize(buf_ptr, end))
        return false;
    key_blob.key_material = deserialized_key_material.release();
    return true;
}

GetKeyCharacteristicsRequest::~GetKeyCharacteristicsRequest() {
    delete[] key_blob.key_material;
}

void GetKeyCharacteristicsRequest::SetKeyMaterial(const void* key_material, size_t length) {
    delete[] key_blob.key_material;
    key_blob.key_material = dup_buffer(key_material, length);
    key_blob.key_material_size = length;
}

size_t GetKeyCharacteristicsRequest::SerializedSize() const {
    return sizeof(uint32_t) /* key blob size */ + key_blob.key_material_size +
           additional_params.SerializedSize();
}

uint8_t* GetKeyCharacteristicsRequest::Serialize(uint8_t* buf, const uint8_t* end) const {
    buf = append_size_and_data_to_buf(buf, end, key_blob.key_material, key_blob.key_material_size);
    return additional_params.Serialize(buf, end);
}

bool GetKeyCharacteristicsRequest::Deserialize(const uint8_t** buf_ptr, const uint8_t* end) {
    delete[] key_blob.key_material;
    key_blob.key_material = NULL;
    UniquePtr<uint8_t[]> deserialized_key_material;
    if (!copy_size_and_data_from_buf(buf_ptr, end, &key_blob.key_material_size,
                                     &deserialized_key_material) ||
        !additional_params.Deserialize(buf_ptr, end))
        return false;
    key_blob.key_material = deserialized_key_material.release();
    return true;
}

size_t GetKeyCharacteristicsResponse::NonErrorSerializedSize() const {
    return enforced.SerializedSize() + unenforced.SerializedSize();
}

uint8_t* GetKeyCharacteristicsResponse::NonErrorSerialize(uint8_t* buf, const uint8_t* end) const {
    buf = enforced.Serialize(buf, end);
    return unenforced.Serialize(buf, end);
}

bool GetKeyCharacteristicsResponse::NonErrorDeserialize(const uint8_t** buf_ptr,
                                                        const uint8_t* end) {
    return enforced.Deserialize(buf_ptr, end) && unenforced.Deserialize(buf_ptr, end);
}

void BeginOperationRequest::SetKeyMaterial(const void* key_material, size_t length) {
    delete[] key_blob.key_material;
    key_blob.key_material = dup_buffer(key_material, length);
    key_blob.key_material_size = length;
}

size_t BeginOperationRequest::SerializedSize() const {
    return sizeof(uint32_t) /* purpose */ + sizeof(uint32_t) /* key length */ +
           key_blob.key_material_size + additional_params.SerializedSize();
}

uint8_t* BeginOperationRequest::Serialize(uint8_t* buf, const uint8_t* end) const {
    buf = append_uint32_to_buf(buf, end, purpose);
    buf = append_size_and_data_to_buf(buf, end, key_blob.key_material, key_blob.key_material_size);
    return additional_params.Serialize(buf, end);
}

bool BeginOperationRequest::Deserialize(const uint8_t** buf_ptr, const uint8_t* end) {
    delete[] key_blob.key_material;
    key_blob.key_material = 0;
    UniquePtr<uint8_t[]> deserialized_key_material;
    if (!copy_uint32_from_buf(buf_ptr, end, &purpose) ||
        !copy_size_and_data_from_buf(buf_ptr, end, &key_blob.key_material_size,
                                     &deserialized_key_material) ||
        !additional_params.Deserialize(buf_ptr, end))
        return false;
    key_blob.key_material = deserialized_key_material.release();
    return true;
}

size_t BeginOperationResponse::NonErrorSerializedSize() const {
    return sizeof(op_handle);
}

uint8_t* BeginOperationResponse::NonErrorSerialize(uint8_t* buf, const uint8_t* end) const {
    return append_uint64_to_buf(buf, end, op_handle);
}

bool BeginOperationResponse::NonErrorDeserialize(const uint8_t** buf_ptr, const uint8_t* end) {
    return copy_uint64_from_buf(buf_ptr, end, &op_handle);
}

size_t UpdateOperationRequest::SerializedSize() const {
    return sizeof(op_handle) + input.SerializedSize();
}

uint8_t* UpdateOperationRequest::Serialize(uint8_t* buf, const uint8_t* end) const {
    buf = append_uint64_to_buf(buf, end, op_handle);
    return input.Serialize(buf, end);
}

bool UpdateOperationRequest::Deserialize(const uint8_t** buf_ptr, const uint8_t* end) {
    return copy_uint64_from_buf(buf_ptr, end, &op_handle) && input.Deserialize(buf_ptr, end);
}

size_t UpdateOperationResponse::NonErrorSerializedSize() const {
    return output.SerializedSize();
}

uint8_t* UpdateOperationResponse::NonErrorSerialize(uint8_t* buf, const uint8_t* end) const {
    return output.Serialize(buf, end);
}

bool UpdateOperationResponse::NonErrorDeserialize(const uint8_t** buf_ptr, const uint8_t* end) {
    return output.Deserialize(buf_ptr, end);
}

size_t FinishOperationRequest::SerializedSize() const {
    return sizeof(op_handle) + signature.SerializedSize();
}

uint8_t* FinishOperationRequest::Serialize(uint8_t* buf, const uint8_t* end) const {
    buf = append_uint64_to_buf(buf, end, op_handle);
    return signature.Serialize(buf, end);
}

bool FinishOperationRequest::Deserialize(const uint8_t** buf_ptr, const uint8_t* end) {
    return copy_uint64_from_buf(buf_ptr, end, &op_handle) && signature.Deserialize(buf_ptr, end);
}

size_t FinishOperationResponse::NonErrorSerializedSize() const {
    return output.SerializedSize();
}

uint8_t* FinishOperationResponse::NonErrorSerialize(uint8_t* buf, const uint8_t* end) const {
    return output.Serialize(buf, end);
}

bool FinishOperationResponse::NonErrorDeserialize(const uint8_t** buf_ptr, const uint8_t* end) {
    return output.Deserialize(buf_ptr, end);
}

void ImportKeyRequest::SetKeyMaterial(const void* key_material, size_t length) {
    delete[] key_data;
    key_data = dup_buffer(key_material, length);
    key_data_length = length;
}

size_t ImportKeyRequest::SerializedSize() const {
    return key_description.SerializedSize() + sizeof(uint32_t) /* key_format */ +
           sizeof(uint32_t) /* key_data_length */ + key_data_length;
}

uint8_t* ImportKeyRequest::Serialize(uint8_t* buf, const uint8_t* end) const {
    buf = key_description.Serialize(buf, end);
    buf = append_uint32_to_buf(buf, end, key_format);
    return append_size_and_data_to_buf(buf, end, key_data, key_data_length);
}

bool ImportKeyRequest::Deserialize(const uint8_t** buf_ptr, const uint8_t* end) {
    delete[] key_data;
    key_data = NULL;
    UniquePtr<uint8_t[]> deserialized_key_material;
    if (!key_description.Deserialize(buf_ptr, end) ||
        !copy_uint32_from_buf(buf_ptr, end, &key_format) ||
        !copy_size_and_data_from_buf(buf_ptr, end, &key_data_length, &deserialized_key_material))
        return false;
    key_data = deserialized_key_material.release();
    return true;
}

void ImportKeyResponse::SetKeyMaterial(const void* key_material, size_t length) {
    delete[] key_blob.key_material;
    key_blob.key_material = dup_buffer(key_material, length);
    key_blob.key_material_size = length;
}

size_t ImportKeyResponse::NonErrorSerializedSize() const {
    return sizeof(uint32_t) /* key_material length */ + key_blob.key_material_size +
           enforced.SerializedSize() + unenforced.SerializedSize();
}

uint8_t* ImportKeyResponse::NonErrorSerialize(uint8_t* buf, const uint8_t* end) const {
    buf = append_size_and_data_to_buf(buf, end, key_blob.key_material, key_blob.key_material_size);
    buf = enforced.Serialize(buf, end);
    return unenforced.Serialize(buf, end);
}

bool ImportKeyResponse::NonErrorDeserialize(const uint8_t** buf_ptr, const uint8_t* end) {
    delete[] key_blob.key_material;
    key_blob.key_material = NULL;
    UniquePtr<uint8_t[]> deserialized_key_material;
    if (!copy_size_and_data_from_buf(buf_ptr, end, &key_blob.key_material_size,
                                     &deserialized_key_material) ||
        !enforced.Deserialize(buf_ptr, end) || !unenforced.Deserialize(buf_ptr, end))
        return false;
    key_blob.key_material = deserialized_key_material.release();
    return true;
}

void ExportKeyRequest::SetKeyMaterial(const void* key_material, size_t length) {
    delete[] key_blob.key_material;
    key_blob.key_material = dup_buffer(key_material, length);
    key_blob.key_material_size = length;
}

size_t ExportKeyRequest::SerializedSize() const {
    return additional_params.SerializedSize() + sizeof(uint32_t) /* key_format */ +
           sizeof(uint32_t) /* key_material_size */ + key_blob.key_material_size;
}

uint8_t* ExportKeyRequest::Serialize(uint8_t* buf, const uint8_t* end) const {
    buf = additional_params.Serialize(buf, end);
    buf = append_uint32_to_buf(buf, end, key_format);
    return append_size_and_data_to_buf(buf, end, key_blob.key_material, key_blob.key_material_size);
}

bool ExportKeyRequest::Deserialize(const uint8_t** buf_ptr, const uint8_t* end) {
    delete[] key_blob.key_material;
    key_blob.key_material = NULL;
    UniquePtr<uint8_t[]> deserialized_key_material;
    if (!additional_params.Deserialize(buf_ptr, end) ||
        !copy_uint32_from_buf(buf_ptr, end, &key_format) ||
        !copy_size_and_data_from_buf(buf_ptr, end, &key_blob.key_material_size,
                                     &deserialized_key_material))
        return false;
    key_blob.key_material = deserialized_key_material.release();
    return true;
}

void ExportKeyResponse::SetKeyMaterial(const void* key_material, size_t length) {
    delete[] key_data;
    key_data = dup_buffer(key_material, length);
    key_data_length = length;
}

size_t ExportKeyResponse::NonErrorSerializedSize() const {
    return sizeof(uint32_t) /* key_data_length */ + key_data_length;
}

uint8_t* ExportKeyResponse::NonErrorSerialize(uint8_t* buf, const uint8_t* end) const {
    return append_size_and_data_to_buf(buf, end, key_data, key_data_length);
}

bool ExportKeyResponse::NonErrorDeserialize(const uint8_t** buf_ptr, const uint8_t* end) {
    delete[] key_data;
    key_data = NULL;
    UniquePtr<uint8_t[]> deserialized_key_material;
    if (!copy_size_and_data_from_buf(buf_ptr, end, &key_data_length, &deserialized_key_material))
        return false;
    key_data = deserialized_key_material.release();
    return true;
}

}  // namespace keymaster
