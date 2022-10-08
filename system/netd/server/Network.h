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

#ifndef NETD_SERVER_NETWORK_H
#define NETD_SERVER_NETWORK_H

#include "NetdConstants.h"

#include <set>
#include <string>

// A Network represents a collection of interfaces participating as a single administrative unit.
class Network {
public:
    enum Type {
        LOCAL,
        PHYSICAL,
        VIRTUAL,
    };

    // You MUST ensure that no interfaces are still assigned to this network, say by calling
    // clearInterfaces(), before deleting it. This is because interface removal may fail. If we
    // automatically removed interfaces in the destructor, you wouldn't know if it failed.
    virtual ~Network();

    virtual Type getType() const = 0;
    unsigned getNetId() const;

    bool hasInterface(const std::string& interface) const;
    const std::set<std::string>& getInterfaces() const;

    // These return 0 on success or negative errno on failure.
    virtual int addInterface(const std::string& interface) WARN_UNUSED_RESULT = 0;
    virtual int removeInterface(const std::string& interface) WARN_UNUSED_RESULT = 0;
    int clearInterfaces() WARN_UNUSED_RESULT;

protected:
    explicit Network(unsigned netId);

    const unsigned mNetId;
    std::set<std::string> mInterfaces;
};

#endif  // NETD_SERVER_NETWORK_H
