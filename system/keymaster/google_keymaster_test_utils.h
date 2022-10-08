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

#ifndef SYSTEM_KEYMASTER_GOOGLE_KEYMASTER_TEST_UTILS_H_
#define SYSTEM_KEYMASTER_GOOGLE_KEYMASTER_TEST_UTILS_H_

/*
 * Utilities used to help with testing.  Not used in production code.
 */

#include <stdarg.h>

#include <ostream>

#include <keymaster/authorization_set.h>
#include <keymaster/keymaster_defs.h>
#include <keymaster/logger.h>

std::ostream& operator<<(std::ostream& os, const keymaster_key_param_t& param);
bool operator==(const keymaster_key_param_t& a, const keymaster_key_param_t& b);

namespace keymaster {

bool operator==(const AuthorizationSet& a, const AuthorizationSet& b);

std::ostream& operator<<(std::ostream& os, const AuthorizationSet& set);

namespace test {

template <keymaster_tag_t Tag, typename KeymasterEnum>
bool contains(const AuthorizationSet& set, TypedEnumTag<KM_ENUM, Tag, KeymasterEnum> tag,
              KeymasterEnum val) {
    int pos = set.find(tag);
    return pos != -1 && set[pos].enumerated == val;
}

template <keymaster_tag_t Tag, typename KeymasterEnum>
bool contains(const AuthorizationSet& set, TypedEnumTag<KM_ENUM_REP, Tag, KeymasterEnum> tag,
              KeymasterEnum val) {
    int pos = -1;
    while ((pos = set.find(tag, pos)) != -1)
        if (set[pos].enumerated == val)
            return true;
    return false;
}

template <keymaster_tag_t Tag>
bool contains(const AuthorizationSet& set, TypedTag<KM_INT, Tag> tag, uint32_t val) {
    int pos = set.find(tag);
    return pos != -1 && set[pos].integer == val;
}

template <keymaster_tag_t Tag>
bool contains(const AuthorizationSet& set, TypedTag<KM_INT_REP, Tag> tag, uint32_t val) {
    int pos = -1;
    while ((pos = set.find(tag, pos)) != -1)
        if (set[pos].integer == val)
            return true;
    return false;
}

template <keymaster_tag_t Tag>
bool contains(const AuthorizationSet& set, TypedTag<KM_LONG, Tag> tag, uint64_t val) {
    int pos = set.find(tag);
    return pos != -1 && set[pos].long_integer == val;
}

template <keymaster_tag_t Tag>
bool contains(const AuthorizationSet& set, TypedTag<KM_BYTES, Tag> tag, const std::string& val) {
    int pos = set.find(tag);
    return pos != -1 &&
           std::string(reinterpret_cast<const char*>(set[pos].blob.data),
                       set[pos].blob.data_length) == val;
}

inline bool contains(const AuthorizationSet& set, keymaster_tag_t tag) {
    return set.find(tag) != -1;
}

class StdoutLogger : public Logger {
  public:
    int debug(const char* fmt, ...) const {
        va_list args;
        va_start(args, fmt);
        int result = vprintf(fmt, args);
        result += printf("\n");
        va_end(args);
        return result;
    }

    int info(const char* fmt, ...) const {
        va_list args;
        va_start(args, fmt);
        int result = vprintf(fmt, args);
        result += printf("\n");
        va_end(args);
        return result;
    }

    int error(const char* fmt, ...) const {
        va_list args;
        va_start(args, fmt);
        int result = vfprintf(stderr, fmt, args);
        result += printf("\n");
        va_end(args);
        return result;
    }

    int severe(const char* fmt, ...) const {
        va_list args;
        va_start(args, fmt);
        int result = vfprintf(stderr, fmt, args);
        result += printf("\n");
        va_end(args);
        return result;
    }
};

}  // namespace test
}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_GOOGLE_KEYMASTER_TEST_UTILS_H_
