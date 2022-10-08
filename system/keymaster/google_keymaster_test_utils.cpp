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

#include "google_keymaster_test_utils.h"

std::ostream& operator<<(std::ostream& os, const keymaster_key_param_t& param) {
    os << "Tag: " << keymaster_tag_mask_type(param.tag);
    switch (keymaster_tag_get_type(param.tag)) {
    case KM_INVALID:
        os << " Invalid";
        break;
    case KM_INT_REP:
        os << " (Rep)";
    /* Falls through */
    case KM_INT:
        os << " Int: " << param.integer;
        break;
    case KM_ENUM_REP:
        os << " (Rep)";
    /* Falls through */
    case KM_ENUM:
        os << " Enum: " << param.enumerated;
        break;
    case KM_LONG:
        os << " Long: " << param.long_integer;
        break;
    case KM_DATE:
        os << " Date: " << param.date_time;
        break;
    case KM_BOOL:
        os << " Bool: " << param.boolean;
        break;
    case KM_BIGNUM:
        os << " Bignum: ";
        break;
    case KM_BYTES:
        os << " Bytes: ";
        break;
    }
    return os;
}

bool operator==(const keymaster_key_param_t& a, const keymaster_key_param_t& b) {
    if (a.tag != b.tag) {
        return false;
    }

    switch (keymaster_tag_get_type(a.tag)) {
    default:
        return false;
    case KM_INVALID:
        return true;
    case KM_INT_REP:
    case KM_INT:
        return a.integer == b.integer;
    case KM_ENUM_REP:
    case KM_ENUM:
        return a.enumerated == b.enumerated;
    case KM_LONG:
        return a.long_integer == b.long_integer;
    case KM_DATE:
        return a.date_time == b.date_time;
    case KM_BOOL:
        return a.boolean == b.boolean;
    case KM_BIGNUM:
    case KM_BYTES:
        if ((a.blob.data == NULL || b.blob.data == NULL) && a.blob.data != b.blob.data)
            return false;
        return a.blob.data_length == b.blob.data_length &&
               (memcmp(a.blob.data, b.blob.data, a.blob.data_length) == 0);
    }
}

namespace keymaster {

bool operator==(const AuthorizationSet& a, const AuthorizationSet& b) {
    if (a.size() != b.size())
        return false;

    for (size_t i = 0; i < a.size(); ++i)
        if (!(a[i] == b[i]))
            return false;
    return true;
}

std::ostream& operator<<(std::ostream& os, const AuthorizationSet& set) {
    if (set.size() == 0)
        os << "(Empty)" << std::endl;
    for (size_t i = 0; i < set.size(); ++i) {
        os << set[i] << std::endl;
    }
    return os;
}

}  // namespace keymaster
