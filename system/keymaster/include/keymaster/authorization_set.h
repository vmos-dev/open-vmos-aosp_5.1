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

#ifndef SYSTEM_KEYMASTER_AUTHORIZATION_SET_H_
#define SYSTEM_KEYMASTER_AUTHORIZATION_SET_H_

#include <UniquePtr.h>

#include <keymaster/keymaster_defs.h>
#include <keymaster/keymaster_tags.h>
#include <keymaster/serializable.h>

namespace keymaster {

/**
 * A container that manages a set of keymaster_key_param_t objects, providing serialization,
 * de-serialization and accessors.
 */
class AuthorizationSet : public Serializable {
  public:
    /**
     * Construct an empty, dynamically-allocated, growable AuthorizationSet.  Does not actually
     * allocate any storage until elements are added, so there is no cost to creating an
     * AuthorizationSet with this constructor and then reinitializing it to point at pre-allocated
     * buffers, with \p Reinitialize.
     */
    AuthorizationSet()
        : elems_(NULL), elems_size_(0), elems_capacity_(0), indirect_data_(NULL),
          indirect_data_size_(0), indirect_data_capacity_(0), error_(OK) {}

    /**
     * Construct an AuthorizationSet from the provided array.  The AuthorizationSet copies the data
     * from the provided array (and the data referenced by its embedded pointers, if any) into
     * dynamically-allocated storage.  If allocation of the needed storage fails, \p is_valid() will
     * return ALLOCATION_FAILURE. It is the responsibility of the caller to check before using the
     * set, if allocations might fail.
     */
    AuthorizationSet(const keymaster_key_param_t* elems, size_t count)
        : elems_(NULL), indirect_data_(NULL) {
        Reinitialize(elems, count);
    }

    AuthorizationSet(const uint8_t* serialized_set, size_t serialized_size)
        : elems_(NULL), indirect_data_(NULL) {
        Deserialize(&serialized_set, serialized_set + serialized_size);
    }

    // Copy constructor.
    AuthorizationSet(const AuthorizationSet&);

    /**
     * Reinitialize an AuthorizationSet as a dynamically-allocated, growable copy of the data in the
     * provided array (and the data referenced by its embedded pointers, if any).  If the allocation
     * of the needed storage fails this method will return false and \p is_valid() will return
     * ALLOCATION_FAILURE.
     */
    bool Reinitialize(const keymaster_key_param_t* elems, size_t count);

    bool Reinitialize(const AuthorizationSet& set) {
        return Reinitialize(set.elems_, set.elems_size_);
    }

    ~AuthorizationSet();

    enum Error {
        OK,
        ALLOCATION_FAILURE,
        MALFORMED_DATA,
    };

    Error is_valid() const { return error_; }

    /**
     * Returns the size of the set.
     */
    size_t size() const { return elems_size_; }

    /**
     * Returns the total size of all indirect data referenced by set elements.
     */
    size_t indirect_size() const { return indirect_data_size_; }

    /**
     * Returns the data in the set, directly. Be careful with this.
     */
    const keymaster_key_param_t* data() const;

    /**
     * Returns the offset of the next entry that matches \p tag, starting from the element after \p
     * begin.  If not found, returns -1.
     */
    int find(keymaster_tag_t tag, int begin = -1) const;

    /**
     * Returns the nth element of the set.
     */
    keymaster_key_param_t operator[](int n) const;

    /**
     * If the specified integer-typed \p tag exists, places its value in \p val and returns true.
     * If \p tag is not present, leaves \p val unmodified and returns false.
     */
    template <keymaster_tag_t T>
    inline bool GetTagValue(TypedTag<KM_INT, T> tag, uint32_t* val) const {
        return GetTagValueInt(tag, val);
    }

    /**
     * If the specified instance of the specified integer-typed \p tag exists, places its value
     * in \p val and returns true.  If \p tag is not present, leaves \p val unmodified and returns
     * false.
     */
    template <keymaster_tag_t Tag>
    bool GetTagValue(TypedTag<KM_INT_REP, Tag> tag, size_t instance, uint32_t* val) const {
        return GetTagValueIntRep(tag, instance, val);
    }

    /**
     * If the specified long-typed \p tag exists, places its value in \p val and returns true.
     * If \p tag is not present, leaves \p val unmodified and returns false.
     */
    template <keymaster_tag_t T>
    inline bool GetTagValue(TypedTag<KM_LONG, T> tag, uint64_t* val) const {
        return GetTagValueLong(tag, val);
    }

    /**
     * If the specified enumeration-typed \p tag exists, places its value in \p val and returns
     * true.  If \p tag is not present, leaves \p val unmodified and returns false.
     */
    template <keymaster_tag_t Tag, typename T>
    bool GetTagValue(TypedEnumTag<KM_ENUM, Tag, T> tag, T* val) const {
        return GetTagValueEnum(tag, reinterpret_cast<uint32_t*>(val));
    }

    /**
     * If the specified instance of the specified enumeration-typed \p tag exists, places its value
     * in \p val and returns true.  If \p tag is not present, leaves \p val unmodified and returns
     * false.
     */
    template <keymaster_tag_t Tag, typename T>
    bool GetTagValue(TypedEnumTag<KM_ENUM_REP, Tag, T> tag, size_t instance, T* val) const {
        return GetTagValueEnumRep(tag, instance, reinterpret_cast<uint32_t*>(val));
    }

    /**
     * If the specified date-typed \p tag exists, places its value in \p val and returns
     * true.  If \p tag is not present, leaves \p val unmodified and returns false.
     */
    template <keymaster_tag_t Tag>
    bool GetTagValue(TypedTag<KM_INT_REP, Tag> tag, size_t instance,
                     typename TypedTag<KM_INT_REP, Tag>::value_type* val) const {
        return GetTagValueIntRep(tag, instance, val);
    }

    /**
     * If the specified bytes-typed \p tag exists, places its value in \p val and returns
     * true.  If \p tag is not present, leaves \p val unmodified and returns false.
     */
    template <keymaster_tag_t Tag>
    bool GetTagValue(TypedTag<KM_BYTES, Tag> tag, keymaster_blob_t* val) const {
        return GetTagValueBlob(tag, val);
    }

    /**
     * If the specified bignum-typed \p tag exists, places its value in \p val and returns
     * true.  If \p tag is not present, leaves \p val unmodified and returns false.
     */
    template <keymaster_tag_t Tag>
    bool GetTagValue(TypedTag<KM_BIGNUM, Tag> tag, keymaster_blob_t* val) const {
        return GetTagValueBlob(tag, val);
    }

    /**
     * If the specified \p tag exists, places its value in \p val and returns true.  If \p tag is
     * not present, leaves \p val unmodified and returns false.
     */
    template <keymaster_tag_t Tag, keymaster_tag_type_t Type>
    bool GetTagValue(TypedTag<Type, Tag> tag, typename TagValueType<Type>::value_type* val) const {
        return GetTagValueLong(tag, val);
    }

    bool push_back(keymaster_key_param_t elem);

    /**
     * Grow the elements array to ensure it can contain \p count entries.  Preserves any existing
     * entries.
     */
    bool reserve_elems(size_t count);

    /**
     * Grow the indirect data array to ensure it can contain \p length bytes.  Preserves any
     * existing indirect data.
     */
    bool reserve_indirect(size_t length);

    bool push_back(const AuthorizationSet& set);

    template <keymaster_tag_t Tag, keymaster_tag_type_t Type, typename KeymasterEnum>
    bool push_back(TypedEnumTag<Type, Tag, KeymasterEnum> tag, KeymasterEnum val) {
        return push_back(Authorization(tag, val));
    }

    template <keymaster_tag_t Tag> bool push_back(TypedTag<KM_BOOL, Tag> tag) {
        return push_back(Authorization(tag));
    }

    template <keymaster_tag_t Tag>
    bool push_back(TypedTag<KM_BYTES, Tag> tag, const void* bytes, size_t bytes_len) {
        return push_back(keymaster_param_blob(tag, static_cast<const uint8_t*>(bytes), bytes_len));
    }

    template <keymaster_tag_t Tag>
    bool push_back(TypedTag<KM_BIGNUM, Tag> tag, const void* bytes, size_t bytes_len) {
        return push_back(keymaster_param_blob(tag, static_cast<const uint8_t*>(bytes), bytes_len));
    }

    template <keymaster_tag_t Tag, keymaster_tag_type_t Type>
    bool push_back(TypedTag<Type, Tag> tag, typename TypedTag<Type, Tag>::value_type val) {
        return push_back(Authorization(tag, val));
    }

    template <keymaster_tag_t Tag, keymaster_tag_type_t Type>
    bool push_back(TypedTag<Type, Tag> tag, const void* bytes, size_t bytes_len) {
        return push_back(Authorization(tag, bytes, bytes_len));
    }

    /* Virtual methods from Serializable */
    size_t SerializedSize() const;
    uint8_t* Serialize(uint8_t* serialized_set, const uint8_t* end) const;
    bool Deserialize(const uint8_t** buf_ptr, const uint8_t* end);

    size_t SerializedSizeOfElements() const;

  private:
    // Disallow assignment
    void operator=(const AuthorizationSet&);

    void FreeData();
    void set_invalid(Error err);

    static size_t ComputeIndirectDataSize(const keymaster_key_param_t* elems, size_t count);
    void CopyIndirectData();
    bool CheckIndirectData();

    bool DeserializeIndirectData(const uint8_t** buf_ptr, const uint8_t* end);
    bool DeserializeElementsData(const uint8_t** buf_ptr, const uint8_t* end);

    bool GetTagValueEnum(keymaster_tag_t tag, uint32_t* val) const;
    bool GetTagValueEnumRep(keymaster_tag_t tag, size_t instance, uint32_t* val) const;
    bool GetTagValueInt(keymaster_tag_t tag, uint32_t* val) const;
    bool GetTagValueIntRep(keymaster_tag_t tag, size_t instance, uint32_t* val) const;
    bool GetTagValueLong(keymaster_tag_t tag, uint64_t* val) const;
    bool GetTagValueDate(keymaster_tag_t tag, uint64_t* val) const;
    bool GetTagValueBlob(keymaster_tag_t tag, keymaster_blob_t* val) const;

    keymaster_key_param_t* elems_;
    size_t elems_size_;
    size_t elems_capacity_;
    uint8_t* indirect_data_;
    size_t indirect_data_size_;
    size_t indirect_data_capacity_;
    Error error_;
};

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_KEY_AUTHORIZATION_SET_H_
