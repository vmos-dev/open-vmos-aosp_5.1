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

#ifndef NETD_SERVER_UID_RANGES_H
#define NETD_SERVER_UID_RANGES_H

#include <sys/types.h>
#include <utility>
#include <vector>

class UidRanges {
public:
    typedef std::pair<uid_t, uid_t> Range;

    bool hasUid(uid_t uid) const;
    const std::vector<Range>& getRanges() const;

    bool parseFrom(int argc, char* argv[]);

    void add(const UidRanges& other);
    void remove(const UidRanges& other);

private:
    std::vector<Range> mRanges;
};

#endif  // NETD_SERVER_UID_RANGES_H
