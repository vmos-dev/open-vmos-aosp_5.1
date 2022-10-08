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

#ifndef NETD_SERVER_PHYSICAL_NETWORK_H
#define NETD_SERVER_PHYSICAL_NETWORK_H

#include "Network.h"
#include "Permission.h"

class PhysicalNetwork : public Network {
public:
    class Delegate {
    public:
        virtual ~Delegate();

        virtual int addFallthrough(const std::string& physicalInterface,
                                   Permission permission) WARN_UNUSED_RESULT = 0;
        virtual int removeFallthrough(const std::string& physicalInterface,
                                      Permission permission) WARN_UNUSED_RESULT = 0;
    };

    PhysicalNetwork(unsigned netId, Delegate* delegate);
    virtual ~PhysicalNetwork();

    // These refer to permissions that apps must have in order to use this network.
    Permission getPermission() const;
    int setPermission(Permission permission) WARN_UNUSED_RESULT;

    int addAsDefault() WARN_UNUSED_RESULT;
    int removeAsDefault() WARN_UNUSED_RESULT;

private:
    Type getType() const override;
    int addInterface(const std::string& interface) override WARN_UNUSED_RESULT;
    int removeInterface(const std::string& interface) override WARN_UNUSED_RESULT;

    Delegate* const mDelegate;
    Permission mPermission;
    bool mIsDefault;
};

#endif  // NETD_SERVER_PHYSICAL_NETWORK_H
