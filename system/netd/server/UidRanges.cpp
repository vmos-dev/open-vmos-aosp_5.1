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

#include "UidRanges.h"

#include "NetdConstants.h"

#include <stdlib.h>

bool UidRanges::hasUid(uid_t uid) const {
    auto iter = std::lower_bound(mRanges.begin(), mRanges.end(), Range(uid, uid));
    return (iter != mRanges.end() && iter->first == uid) ||
           (iter != mRanges.begin() && (--iter)->second >= uid);
}

const std::vector<UidRanges::Range>& UidRanges::getRanges() const {
    return mRanges;
}

bool UidRanges::parseFrom(int argc, char* argv[]) {
    mRanges.clear();
    for (int i = 0; i < argc; ++i) {
        if (!*argv[i]) {
            // The UID string is empty.
            return false;
        }
        char* endPtr;
        uid_t uidStart = strtoul(argv[i], &endPtr, 0);
        uid_t uidEnd;
        if (!*endPtr) {
            // Found a single UID. The range contains just the one UID.
            uidEnd = uidStart;
        } else if (*endPtr == '-') {
            if (!*++endPtr) {
                // Unexpected end of string.
                return false;
            }
            uidEnd = strtoul(endPtr, &endPtr, 0);
            if (*endPtr) {
                // Illegal trailing chars.
                return false;
            }
            if (uidEnd < uidStart) {
                // Invalid order.
                return false;
            }
        } else {
            // Not a single uid, not a range. Found some other illegal char.
            return false;
        }
        if (uidStart == INVALID_UID || uidEnd == INVALID_UID) {
            // Invalid UIDs.
            return false;
        }
        mRanges.push_back(Range(uidStart, uidEnd));
    }
    std::sort(mRanges.begin(), mRanges.end());
    return true;
}

void UidRanges::add(const UidRanges& other) {
    auto middle = mRanges.insert(mRanges.end(), other.mRanges.begin(), other.mRanges.end());
    std::inplace_merge(mRanges.begin(), middle, mRanges.end());
}

void UidRanges::remove(const UidRanges& other) {
    auto end = std::set_difference(mRanges.begin(), mRanges.end(), other.mRanges.begin(),
                                   other.mRanges.end(), mRanges.begin());
    mRanges.erase(end, mRanges.end());
}
