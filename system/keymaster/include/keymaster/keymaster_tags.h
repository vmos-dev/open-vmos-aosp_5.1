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

#ifndef SYSTEM_KEYMASTER_KEYMASTER_TAGS_H_
#define SYSTEM_KEYMASTER_KEYMASTER_TAGS_H_

/**
 * This header contains various definitions that make working with keymaster tags safer and easier.
 * It makes use of a fair amount of template metaprogramming, which is genarally a bad idea for
 * maintainability, but in this case all of the metaprogramming serves the purpose of making it
 * impossible to make certain classes of mistakes when operating on keymaster authorizations.  For
 * example, it's an error to create a keymaster_param_t with tag == KM_TAG_PURPOSE and then to
 * assign KM_ALGORITHM_RSA to the enumerated element of its union, but because "enumerated" is a
 * uint32_t, there's no way for the compiler, ordinarily, to diagnose it.  Also, generic functions
 * to manipulate authorizations of multiple types can't be written, because they need to know which
 * union parameter to modify.
 *
 * The machinery in this header solves these problems.  The core elements are two templated classes,
 * TypedTag and TypedEnumTag.  These classes are templated on a tag type and a tag value, and in the
 * case of TypedEnumTag, an enumeration type as well.  Specializations are created for each
 * keymaster tag, associating the tag type with the tag, and an instance of each specialization is
 * created, and named the same as the keymaster tag, but with the KM_ prefix omitted.  Because the
 * classes include a conversion operator to keymaster_tag_t, they can be used anywhere a
 * keymaster_tag_t is expected.
 *
 * They also define a "value_type" typedef, which specifies the type of values associated with that
 * particular tag.  This enables template functions to be written that check that the correct
 * parameter type is used for a given tag, and that use the correct union entry for the tag type.  A
 * very useful example is the overloaded "Authorization" function defined below, which takes tag and
 * value arguments and correctly constructs a keyamster_param_t struct.
 *
 * Because the classes have no data members and all of their methods are inline, they have ZERO
 * run-time cost in and of themselves.  The one way in which they can create code bloat is when
 * template functions using them are expanded multiple times.  The standard method of creating
 * trivial, inlined template functions which call non-templated functions which are compact but not
 * type-safe, allows the program to have both the type-safety of the templates and the compactness
 * of the non-templated functions, at the same time.
 *
 * For debugging purposes, one additional element of TypedTag and TypedEnumTag can be conditionally
 * compiled in.  If the "KEYMASTER_NAME_TAGS" macro symbol is defined, both classes will have a
 * name() method which returns a string equal to the tame of the tag (e.g. TAG_PURPOSE).  Activating
 * this option means the classes _do_ contain a data member, a pointer to the string, and also
 * causes static data space to be allocated for the strings.  So the run-time cost of these classes
 * is no longer zero.  Note that it can cause problems if KEYMASTER_NAME_TAGS is defined for some
 * compilation units and not others.
 */

#include <keymaster/keymaster_defs.h>

namespace keymaster {

// Until we have C++11, fake std::static_assert.
template <bool b> struct StaticAssert {};
template <> struct StaticAssert<true> {
    static void check() {}
};

// An unusable type that we can associate with tag types that don't have a simple value type.
// That will prevent the associated type from being used inadvertently.
class Void {
    Void();
    ~Void();
};

/**
 * A template that defines the association between non-enumerated tag types and their value
 * types.  For each tag type we define a specialized struct that contains a typedef "value_type".
 */
template <keymaster_tag_type_t tag_type> struct TagValueType {};
template <> struct TagValueType<KM_LONG> { typedef uint64_t value_type; };
template <> struct TagValueType<KM_DATE> { typedef uint64_t value_type; };
template <> struct TagValueType<KM_INT> { typedef uint32_t value_type; };
template <> struct TagValueType<KM_INT_REP> { typedef uint32_t value_type; };
template <> struct TagValueType<KM_INVALID> { typedef Void value_type; };
template <> struct TagValueType<KM_BOOL> { typedef bool value_type; };
template <> struct TagValueType<KM_BYTES> { typedef keymaster_blob_t value_type; };
template <> struct TagValueType<KM_BIGNUM> { typedef keymaster_blob_t value_type; };

/**
 * TypedTag is a templatized version of keymaster_tag_t, which provides compile-time checking of
 * keymaster tag types. Instances are convertible to keymaster_tag_t, so they can be used wherever
 * keymaster_tag_t is expected, and because they encode the tag type it's possible to create
 * function overloadings that only operate on tags with a particular type.
 */
template <keymaster_tag_type_t tag_type, keymaster_tag_t tag> class TypedTag {
  public:
    typedef typename TagValueType<tag_type>::value_type value_type;

#ifdef KEYMASTER_NAME_TAGS
    inline TypedTag(const char* name) : name_(name) {
#else
    inline TypedTag() {
#endif
        // Ensure that it's impossible to create a TypedTag instance whose 'tag' doesn't have type
        // 'tag_type'.  Attempting to instantiate a tag with the wrong type will result in a compile
        // error (no match for template specialization StaticAssert<false>), with no run-time cost.
        StaticAssert<(tag & tag_type) == tag_type>::check();
        StaticAssert<(tag_type != KM_ENUM) && (tag_type != KM_ENUM_REP)>::check();
    }
    inline operator keymaster_tag_t() { return tag; }
#ifdef KEYMASTER_NAME_TAGS
    const char* name() { return name_; }

  private:
    const char* name_;
#endif
};

template <keymaster_tag_type_t tag_type, keymaster_tag_t tag, typename KeymasterEnum>
class TypedEnumTag {
  public:
    typedef KeymasterEnum value_type;

#ifdef KEYMASTER_NAME_TAGS
    inline TypedEnumTag(const char* name) : name_(name) {
#else
    inline TypedEnumTag() {
#endif
        // Ensure that it's impossible to create a TypedTag instance whose 'tag' doesn't have type
        // 'tag_type'.  Attempting to instantiate a tag with the wrong type will result in a compile
        // error (no match for template specialization StaticAssert<false>), with no run-time cost.
        StaticAssert<(tag & tag_type) == tag_type>::check();
        StaticAssert<(tag_type == KM_ENUM) || (tag_type == KM_ENUM_REP)>::check();
    }
    inline operator keymaster_tag_t() { return tag; }
#ifdef KEYMASTER_NAME_TAGS
    const char* name() { return name_; }

  private:
    const char* name_;
#endif
};

// DEFINE_KEYMASTER_TAG is used to create TypedTag instances for each non-enum keymaster tag.
#ifdef KEYMASTER_NAME_TAGS
#define DEFINE_KEYMASTER_TAG(type, name) static TypedTag<type, KM_##name> name(#name)
#else
#define DEFINE_KEYMASTER_TAG(type, name) static TypedTag<type, KM_##name> name
#endif

DEFINE_KEYMASTER_TAG(KM_INVALID, TAG_INVALID);
DEFINE_KEYMASTER_TAG(KM_INT, TAG_KEY_SIZE);
DEFINE_KEYMASTER_TAG(KM_INT, TAG_MAC_LENGTH);
DEFINE_KEYMASTER_TAG(KM_INT, TAG_CHUNK_LENGTH);
DEFINE_KEYMASTER_TAG(KM_LONG, TAG_RSA_PUBLIC_EXPONENT);
DEFINE_KEYMASTER_TAG(KM_BIGNUM, TAG_DSA_GENERATOR);
DEFINE_KEYMASTER_TAG(KM_BIGNUM, TAG_DSA_P);
DEFINE_KEYMASTER_TAG(KM_BIGNUM, TAG_DSA_Q);
DEFINE_KEYMASTER_TAG(KM_DATE, TAG_ACTIVE_DATETIME);
DEFINE_KEYMASTER_TAG(KM_DATE, TAG_ORIGINATION_EXPIRE_DATETIME);
DEFINE_KEYMASTER_TAG(KM_DATE, TAG_USAGE_EXPIRE_DATETIME);
DEFINE_KEYMASTER_TAG(KM_INT, TAG_MIN_SECONDS_BETWEEN_OPS);
DEFINE_KEYMASTER_TAG(KM_BOOL, TAG_SINGLE_USE_PER_BOOT);
DEFINE_KEYMASTER_TAG(KM_BOOL, TAG_ALL_USERS);
DEFINE_KEYMASTER_TAG(KM_INT, TAG_USER_ID);
DEFINE_KEYMASTER_TAG(KM_BOOL, TAG_NO_AUTH_REQUIRED);
DEFINE_KEYMASTER_TAG(KM_INT_REP, TAG_USER_AUTH_ID);
DEFINE_KEYMASTER_TAG(KM_INT, TAG_AUTH_TIMEOUT);
DEFINE_KEYMASTER_TAG(KM_INT, TAG_RESCOPE_AUTH_TIMEOUT);
DEFINE_KEYMASTER_TAG(KM_BOOL, TAG_ALL_APPLICATIONS);
DEFINE_KEYMASTER_TAG(KM_BYTES, TAG_APPLICATION_ID);
DEFINE_KEYMASTER_TAG(KM_BYTES, TAG_APPLICATION_DATA);
DEFINE_KEYMASTER_TAG(KM_DATE, TAG_CREATION_DATETIME);
DEFINE_KEYMASTER_TAG(KM_BOOL, TAG_ROLLBACK_RESISTANT);
DEFINE_KEYMASTER_TAG(KM_BYTES, TAG_ROOT_OF_TRUST);
DEFINE_KEYMASTER_TAG(KM_BYTES, TAG_ADDITIONAL_DATA);

#ifdef KEYMASTER_NAME_TAGS
#define DEFINE_KEYMASTER_ENUM_TAG(type, name, enumtype)                                            \
    static TypedEnumTag<type, KM_##name, enumtype> name(#name)
#else
#define DEFINE_KEYMASTER_ENUM_TAG(type, name, enumtype)                                            \
    static TypedEnumTag<type, KM_##name, enumtype> name
#endif

DEFINE_KEYMASTER_ENUM_TAG(KM_ENUM_REP, TAG_PURPOSE, keymaster_purpose_t);
DEFINE_KEYMASTER_ENUM_TAG(KM_ENUM, TAG_ALGORITHM, keymaster_algorithm_t);
DEFINE_KEYMASTER_ENUM_TAG(KM_ENUM, TAG_BLOCK_MODE, keymaster_block_mode_t);
DEFINE_KEYMASTER_ENUM_TAG(KM_ENUM, TAG_DIGEST, keymaster_digest_t);
DEFINE_KEYMASTER_ENUM_TAG(KM_ENUM, TAG_PADDING, keymaster_padding_t);
DEFINE_KEYMASTER_ENUM_TAG(KM_ENUM_REP, TAG_RESCOPING_ADD, keymaster_tag_t);
DEFINE_KEYMASTER_ENUM_TAG(KM_ENUM_REP, TAG_RESCOPING_DEL, keymaster_tag_t);
DEFINE_KEYMASTER_ENUM_TAG(KM_ENUM, TAG_BLOB_USAGE_REQUIREMENTS,
                          keymaster_key_blob_usage_requirements_t);
DEFINE_KEYMASTER_ENUM_TAG(KM_ENUM, TAG_ORIGIN, keymaster_key_origin_t);

//
// Overloaded function "Authorization" to create keymaster_key_param_t objects for all of tags.
//

template <keymaster_tag_t Tag>
inline keymaster_key_param_t Authorization(TypedTag<KM_BOOL, Tag> tag) {
    return keymaster_param_bool(tag);
}

template <keymaster_tag_t Tag>
inline keymaster_key_param_t Authorization(TypedTag<KM_INT, Tag> tag, uint32_t value) {
    return keymaster_param_int(tag, value);
}

template <keymaster_tag_t Tag>
inline keymaster_key_param_t Authorization(TypedTag<KM_INT_REP, Tag> tag, uint32_t value) {
    return keymaster_param_int(tag, value);
}

template <keymaster_tag_t Tag>
inline keymaster_key_param_t Authorization(TypedTag<KM_LONG, Tag> tag, uint64_t value) {
    return keymaster_param_long(tag, value);
}

template <keymaster_tag_t Tag>
inline keymaster_key_param_t Authorization(TypedTag<KM_DATE, Tag> tag, uint64_t value) {
    return keymaster_param_date(tag, value);
}

template <keymaster_tag_t Tag>
inline keymaster_key_param_t Authorization(TypedTag<KM_BYTES, Tag> tag, const void* bytes,
                                           size_t bytes_len) {
    return keymaster_param_blob(tag, reinterpret_cast<const uint8_t*>(bytes), bytes_len);
}

template <keymaster_tag_t Tag>
inline keymaster_key_param_t Authorization(TypedTag<KM_BYTES, Tag> tag,
                                           const keymaster_blob_t& blob) {
    return keymaster_param_blob(tag, blob.data, blob.data_length);
}

template <keymaster_tag_t Tag>
inline keymaster_key_param_t Authorization(TypedTag<KM_BIGNUM, Tag> tag, const void* bytes,
                                           size_t bytes_len) {
    return keymaster_param_blob(tag, reinterpret_cast<const uint8_t*>(bytes), bytes_len);
}

template <keymaster_tag_t Tag>
inline keymaster_key_param_t Authorization(TypedTag<KM_BIGNUM, Tag> tag,
                                           const keymaster_blob_t& blob) {
    return keymaster_param_blob(tag, blob.data, blob.data_length);
}

template <keymaster_tag_t Tag, typename KeymasterEnum>
inline keymaster_key_param_t Authorization(TypedEnumTag<KM_ENUM, Tag, KeymasterEnum> tag,
                                           KeymasterEnum value) {
    return keymaster_param_enum(tag, value);
}

template <keymaster_tag_t Tag, typename KeymasterEnum>
inline keymaster_key_param_t Authorization(TypedEnumTag<KM_ENUM_REP, Tag, KeymasterEnum> tag,
                                           KeymasterEnum value) {
    return keymaster_param_enum(tag, value);
}

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_KEYMASTER_TAGS_H_
