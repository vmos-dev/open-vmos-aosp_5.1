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

#include <keymaster/google_keymaster_utils.h>

namespace keymaster {

uint8_t* dup_buffer(const void* buf, size_t size) {
    uint8_t* retval = new uint8_t[size];
    if (retval != NULL)
        memcpy(retval, buf, size);
    return retval;
}

int memcmp_s(const void* p1, const void* p2, size_t length) {
    const uint8_t* s1 = static_cast<const uint8_t*>(p1);
    const uint8_t* s2 = static_cast<const uint8_t*>(p2);
    uint8_t result = 0;
    while (length-- > 0)
        result |= *s1++ ^ *s2++;
    return result == 0 ? 0 : 1;
}

}  // namespace keymaster
