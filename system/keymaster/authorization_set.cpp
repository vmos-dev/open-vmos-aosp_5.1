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

#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include <assert.h>

#include <keymaster/authorization_set.h>
#include <keymaster/google_keymaster_utils.h>

namespace keymaster {

static inline bool is_blob_tag(keymaster_tag_t tag) {
    return (keymaster_tag_get_type(tag) == KM_BYTES || keymaster_tag_get_type(tag) == KM_BIGNUM);
}

const size_t STARTING_ELEMS_CAPACITY = 8;

AuthorizationSet::AuthorizationSet(const AuthorizationSet& set)
    : Serializable(), elems_(NULL), indirect_data_(NULL) {
    Reinitialize(set.elems_, set.elems_size_);
}

AuthorizationSet::~AuthorizationSet() {
    FreeData();
}

bool AuthorizationSet::reserve_elems(size_t count) {
    if (is_valid() != OK)
        return false;

    if (count >= elems_capacity_) {
        keymaster_key_param_t* new_elems = new keymaster_key_param_t[count];
        if (new_elems == NULL) {
            set_invalid(ALLOCATION_FAILURE);
            return false;
        }
        memcpy(new_elems, elems_, sizeof(*elems_) * elems_size_);
        delete[] elems_;
        elems_ = new_elems;
        elems_capacity_ = count;
    }
    return true;
}

bool AuthorizationSet::reserve_indirect(size_t length) {
    if (is_valid() != OK)
        return false;

    if (length > indirect_data_capacity_) {
        uint8_t* new_data = new uint8_t[length];
        if (new_data == NULL) {
            set_invalid(ALLOCATION_FAILURE);
            return false;
        }
        memcpy(new_data, indirect_data_, indirect_data_size_);

        // Fix up the data pointers to point into the new region.
        for (size_t i = 0; i < elems_size_; ++i) {
            if (is_blob_tag(elems_[i].tag))
                elems_[i].blob.data = new_data + (elems_[i].blob.data - indirect_data_);
        }
        delete[] indirect_data_;
        indirect_data_ = new_data;
        indirect_data_capacity_ = length;
    }
    return true;
}

bool AuthorizationSet::Reinitialize(const keymaster_key_param_t* elems, const size_t count) {
    FreeData();

    if (!reserve_elems(count))
        return false;

    if (!reserve_indirect(ComputeIndirectDataSize(elems, count)))
        return false;

    memcpy(elems_, elems, sizeof(keymaster_key_param_t) * count);
    elems_size_ = count;
    CopyIndirectData();
    error_ = OK;
    return true;
}

void AuthorizationSet::set_invalid(Error error) {
    FreeData();
    error_ = error;
}

int AuthorizationSet::find(keymaster_tag_t tag, int begin) const {
    if (is_valid() != OK)
        return -1;

    int i = ++begin;
    while (i < (int)elems_size_ && elems_[i].tag != tag)
        ++i;
    if (i == (int)elems_size_)
        return -1;
    else
        return i;
}

keymaster_key_param_t empty;
keymaster_key_param_t AuthorizationSet::operator[](int at) const {
    if (is_valid() == OK && at < (int)elems_size_) {
        return elems_[at];
    }
    memset(&empty, 0, sizeof(empty));
    return empty;
}

template <typename T> int comparator(const T& a, const T& b) {
    if (a < b)
        return -1;
    else if (a > b)
        return 1;
    else
        return 0;
}

static int param_comparator(const void* a, const void* b) {
    const keymaster_key_param_t* lhs = static_cast<const keymaster_key_param_t*>(a);
    const keymaster_key_param_t* rhs = static_cast<const keymaster_key_param_t*>(b);

    if (lhs->tag < rhs->tag)
        return -1;
    else if (lhs->tag > rhs->tag)
        return 1;
    else
        switch (keymaster_tag_get_type(lhs->tag)) {
        default:
        case KM_INVALID:
            return 0;
        case KM_ENUM:
        case KM_ENUM_REP:
            return comparator(lhs->enumerated, rhs->enumerated);
        case KM_INT:
        case KM_INT_REP:
            return comparator(lhs->integer, rhs->integer);
        case KM_LONG:
            return comparator(lhs->long_integer, rhs->long_integer);
        case KM_DATE:
            return comparator(lhs->date_time, rhs->date_time);
        case KM_BOOL:
            return comparator(lhs->boolean, rhs->boolean);
        case KM_BIGNUM:
        case KM_BYTES: {
            size_t min_len = lhs->blob.data_length;
            if (rhs->blob.data_length < min_len)
                min_len = rhs->blob.data_length;

            if (lhs->blob.data_length == rhs->blob.data_length && min_len > 0)
                return memcmp(lhs->blob.data, rhs->blob.data, min_len);
            int cmp_result = memcmp(lhs->blob.data, rhs->blob.data, min_len);
            if (cmp_result == 0) {
                // The blobs are equal up to the length of the shortest (which may have length 0),
                // so the shorter is less, the longer is greater and if they have the same length
                // they're identical.
                return comparator(lhs->blob.data_length, rhs->blob.data_length);
            }
            return cmp_result;
        } break;
        }
}

bool AuthorizationSet::push_back(const AuthorizationSet& set) {
    if (is_valid() != OK)
        return false;

    if (!reserve_elems(elems_size_ + set.elems_size_))
        return false;

    if (!reserve_indirect(indirect_data_size_ + set.indirect_data_size_))
        return false;

    for (size_t i = 0; i < set.size(); ++i)
        if (!push_back(set[i]))
            return false;

    return true;
}

bool AuthorizationSet::push_back(keymaster_key_param_t elem) {
    if (is_valid() != OK)
        return false;

    if (elems_size_ >= elems_capacity_)
        if (!reserve_elems(elems_capacity_ ? elems_capacity_ * 2 : STARTING_ELEMS_CAPACITY))
            return false;

    if (is_blob_tag(elem.tag)) {
        if (indirect_data_capacity_ - indirect_data_size_ < elem.blob.data_length)
            if (!reserve_indirect(2 * (indirect_data_capacity_ + elem.blob.data_length)))
                return false;

        memcpy(indirect_data_ + indirect_data_size_, elem.blob.data, elem.blob.data_length);
        elem.blob.data = indirect_data_ + indirect_data_size_;
        indirect_data_size_ += elem.blob.data_length;
    }

    elems_[elems_size_++] = elem;
    return true;
}

static size_t serialized_size(const keymaster_key_param_t& param) {
    switch (keymaster_tag_get_type(param.tag)) {
    case KM_INVALID:
    default:
        return sizeof(uint32_t);
    case KM_ENUM:
    case KM_ENUM_REP:
    case KM_INT:
    case KM_INT_REP:
        return sizeof(uint32_t) * 2;
    case KM_LONG:
    case KM_DATE:
        return sizeof(uint32_t) + sizeof(uint64_t);
    case KM_BOOL:
        return sizeof(uint32_t) + 1;
        break;
    case KM_BIGNUM:
    case KM_BYTES:
        return sizeof(uint32_t) * 3;
    }
}

static uint8_t* serialize(const keymaster_key_param_t& param, uint8_t* buf, const uint8_t* end,
                          const uint8_t* indirect_base) {
    buf = append_uint32_to_buf(buf, end, param.tag);
    switch (keymaster_tag_get_type(param.tag)) {
    case KM_INVALID:
        break;
    case KM_ENUM:
    case KM_ENUM_REP:
        buf = append_uint32_to_buf(buf, end, param.enumerated);
        break;
    case KM_INT:
    case KM_INT_REP:
        buf = append_uint32_to_buf(buf, end, param.integer);
        break;
    case KM_LONG:
        buf = append_uint64_to_buf(buf, end, param.long_integer);
        break;
    case KM_DATE:
        buf = append_uint64_to_buf(buf, end, param.date_time);
        break;
    case KM_BOOL:
        if (buf < end)
            *buf = static_cast<uint8_t>(param.boolean);
        buf++;
        break;
    case KM_BIGNUM:
    case KM_BYTES:
        buf = append_uint32_to_buf(buf, end, param.blob.data_length);
        buf = append_uint32_to_buf(buf, end, param.blob.data - indirect_base);
        break;
    }
    return buf;
}

static bool deserialize(keymaster_key_param_t* param, const uint8_t** buf_ptr, const uint8_t* end,
                        const uint8_t* indirect_base, const uint8_t* indirect_end) {
    if (!copy_uint32_from_buf(buf_ptr, end, &param->tag))
        return false;

    switch (keymaster_tag_get_type(param->tag)) {
    default:
    case KM_INVALID:
        return false;
    case KM_ENUM:
    case KM_ENUM_REP:
        return copy_uint32_from_buf(buf_ptr, end, &param->enumerated);
    case KM_INT:
    case KM_INT_REP:
        return copy_uint32_from_buf(buf_ptr, end, &param->integer);
    case KM_LONG:
        return copy_uint64_from_buf(buf_ptr, end, &param->long_integer);
    case KM_DATE:
        return copy_uint64_from_buf(buf_ptr, end, &param->date_time);
        break;
    case KM_BOOL:
        if (*buf_ptr < end) {
            param->boolean = static_cast<bool>(**buf_ptr);
            (*buf_ptr)++;
            return true;
        }
        return false;

    case KM_BIGNUM:
    case KM_BYTES: {
        uint32_t offset;
        if (!copy_uint32_from_buf(buf_ptr, end, &param->blob.data_length) ||
            !copy_uint32_from_buf(buf_ptr, end, &offset))
            return false;
        if (static_cast<ptrdiff_t>(offset) > indirect_end - indirect_base ||
            static_cast<ptrdiff_t>(offset + param->blob.data_length) > indirect_end - indirect_base)
            return false;
        param->blob.data = indirect_base + offset;
        return true;
    }
    }
}

size_t AuthorizationSet::SerializedSizeOfElements() const {
    size_t size = 0;
    for (size_t i = 0; i < elems_size_; ++i) {
        size += serialized_size(elems_[i]);
    }
    return size;
}

size_t AuthorizationSet::SerializedSize() const {
    return sizeof(uint32_t) +           // Size of indirect_data_
           indirect_data_size_ +        // indirect_data_
           sizeof(uint32_t) +           // Number of elems_
           sizeof(uint32_t) +           // Size of elems_
           SerializedSizeOfElements();  // elems_
}

uint8_t* AuthorizationSet::Serialize(uint8_t* buf, const uint8_t* end) const {
    buf = append_size_and_data_to_buf(buf, end, indirect_data_, indirect_data_size_);
    buf = append_uint32_to_buf(buf, end, elems_size_);
    buf = append_uint32_to_buf(buf, end, SerializedSizeOfElements());
    for (size_t i = 0; i < elems_size_; ++i) {
        buf = serialize(elems_[i], buf, end, indirect_data_);
    }
    return buf;
}

bool AuthorizationSet::DeserializeIndirectData(const uint8_t** buf_ptr, const uint8_t* end) {
    UniquePtr<uint8_t[]> indirect_buf;
    if (!copy_size_and_data_from_buf(buf_ptr, end, &indirect_data_size_, &indirect_buf)) {
        set_invalid(MALFORMED_DATA);
        return false;
    }
    indirect_data_ = indirect_buf.release();
    return true;
}

bool AuthorizationSet::DeserializeElementsData(const uint8_t** buf_ptr, const uint8_t* end) {
    uint32_t elements_count;
    uint32_t elements_size;
    if (!copy_uint32_from_buf(buf_ptr, end, &elements_count) ||
        !copy_uint32_from_buf(buf_ptr, end, &elements_size)) {
        set_invalid(MALFORMED_DATA);
        return false;
    }

    // Note that the following validation of elements_count is weak, but it prevents allocation of
    // elems_ arrays which are clearly too large to be reasonable.
    if (static_cast<ptrdiff_t>(elements_size) > end - *buf_ptr ||
        elements_count * sizeof(uint32_t) > elements_size) {
        set_invalid(MALFORMED_DATA);
        return false;
    }

    if (!reserve_elems(elements_count))
        return false;

    uint8_t* indirect_end = indirect_data_ + indirect_data_size_;
    const uint8_t* elements_end = *buf_ptr + elements_size;
    for (size_t i = 0; i < elements_count; ++i) {
        if (!deserialize(elems_ + i, buf_ptr, elements_end, indirect_data_, indirect_end)) {
            set_invalid(MALFORMED_DATA);
            return false;
        }
    }
    elems_size_ = elements_count;
    return true;
}

bool AuthorizationSet::Deserialize(const uint8_t** buf_ptr, const uint8_t* end) {
    FreeData();

    if (!DeserializeIndirectData(buf_ptr, end) || !DeserializeElementsData(buf_ptr, end))
        return false;

    if (indirect_data_size_ != ComputeIndirectDataSize(elems_, elems_size_)) {
        set_invalid(MALFORMED_DATA);
        return false;
    }
    return true;
}

void AuthorizationSet::FreeData() {
    if (elems_ != NULL)
        memset_s(elems_, 0, elems_size_ * sizeof(keymaster_key_param_t));
    if (indirect_data_ != NULL)
        memset_s(indirect_data_, 0, indirect_data_size_);

    delete[] elems_;
    delete[] indirect_data_;

    elems_ = NULL;
    indirect_data_ = NULL;
    elems_size_ = 0;
    elems_capacity_ = 0;
    indirect_data_size_ = 0;
    indirect_data_capacity_ = 0;
    error_ = OK;
}

/* static */
size_t AuthorizationSet::ComputeIndirectDataSize(const keymaster_key_param_t* elems, size_t count) {
    size_t size = 0;
    for (size_t i = 0; i < count; ++i) {
        if (is_blob_tag(elems[i].tag)) {
            size += elems[i].blob.data_length;
        }
    }
    return size;
}

void AuthorizationSet::CopyIndirectData() {
    memset_s(indirect_data_, 0, indirect_data_capacity_);

    uint8_t* indirect_data_pos = indirect_data_;
    for (size_t i = 0; i < elems_size_; ++i) {
        assert(indirect_data_pos <= indirect_data_ + indirect_data_capacity_);
        if (is_blob_tag(elems_[i].tag)) {
            memcpy(indirect_data_pos, elems_[i].blob.data, elems_[i].blob.data_length);
            elems_[i].blob.data = indirect_data_pos;
            indirect_data_pos += elems_[i].blob.data_length;
        }
    }
    assert(indirect_data_pos == indirect_data_ + indirect_data_capacity_);
    indirect_data_size_ = indirect_data_pos - indirect_data_;
}

bool AuthorizationSet::GetTagValueEnum(keymaster_tag_t tag, uint32_t* val) const {
    int pos = find(tag);
    if (pos == -1) {
        return false;
    }
    *val = elems_[pos].enumerated;
    return true;
}

bool AuthorizationSet::GetTagValueEnumRep(keymaster_tag_t tag, size_t instance,
                                          uint32_t* val) const {
    size_t count = 0;
    int pos = -1;
    while (count <= instance) {
        pos = find(tag, pos);
        if (pos == -1) {
            return false;
        }
        ++count;
    }
    *val = elems_[pos].enumerated;
    return true;
}

bool AuthorizationSet::GetTagValueInt(keymaster_tag_t tag, uint32_t* val) const {
    int pos = find(tag);
    if (pos == -1) {
        return false;
    }
    *val = elems_[pos].integer;
    return true;
}

bool AuthorizationSet::GetTagValueIntRep(keymaster_tag_t tag, size_t instance,
                                         uint32_t* val) const {
    size_t count = 0;
    int pos = -1;
    while (count <= instance) {
        pos = find(tag, pos);
        if (pos == -1) {
            return false;
        }
        ++count;
    }
    *val = elems_[pos].integer;
    return true;
}

bool AuthorizationSet::GetTagValueLong(keymaster_tag_t tag, uint64_t* val) const {
    int pos = find(tag);
    if (pos == -1) {
        return false;
    }
    *val = elems_[pos].long_integer;
    return true;
}

bool AuthorizationSet::GetTagValueDate(keymaster_tag_t tag, uint64_t* val) const {
    int pos = find(tag);
    if (pos == -1) {
        return false;
    }
    *val = elems_[pos].date_time;
    return true;
}

bool AuthorizationSet::GetTagValueBlob(keymaster_tag_t tag, keymaster_blob_t* val) const {
    int pos = find(tag);
    if (pos == -1) {
        return false;
    }
    *val = elems_[pos].blob;
    return true;
}

}  // namespace keymaster
