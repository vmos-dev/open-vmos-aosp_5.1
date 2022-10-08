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

#include "PhysicalNetwork.h"

#include "RouteController.h"

#define LOG_TAG "Netd"
#include "log/log.h"

namespace {

WARN_UNUSED_RESULT int addToDefault(unsigned netId, const std::string& interface,
                                    Permission permission, PhysicalNetwork::Delegate* delegate) {
    if (int ret = RouteController::addInterfaceToDefaultNetwork(interface.c_str(), permission)) {
        ALOGE("failed to add interface %s to default netId %u", interface.c_str(), netId);
        return ret;
    }
    if (int ret = delegate->addFallthrough(interface, permission)) {
        return ret;
    }
    return 0;
}

WARN_UNUSED_RESULT int removeFromDefault(unsigned netId, const std::string& interface,
                                         Permission permission,
                                         PhysicalNetwork::Delegate* delegate) {
    if (int ret = RouteController::removeInterfaceFromDefaultNetwork(interface.c_str(),
                                                                     permission)) {
        ALOGE("failed to remove interface %s from default netId %u", interface.c_str(), netId);
        return ret;
    }
    if (int ret = delegate->removeFallthrough(interface, permission)) {
        return ret;
    }
    return 0;
}

}  // namespace

PhysicalNetwork::Delegate::~Delegate() {
}

PhysicalNetwork::PhysicalNetwork(unsigned netId, PhysicalNetwork::Delegate* delegate) :
        Network(netId), mDelegate(delegate), mPermission(PERMISSION_NONE), mIsDefault(false) {
}

PhysicalNetwork::~PhysicalNetwork() {
}

Permission PhysicalNetwork::getPermission() const {
    return mPermission;
}

int PhysicalNetwork::setPermission(Permission permission) {
    if (permission == mPermission) {
        return 0;
    }
    for (const std::string& interface : mInterfaces) {
        if (int ret = RouteController::modifyPhysicalNetworkPermission(mNetId, interface.c_str(),
                                                                       mPermission, permission)) {
            ALOGE("failed to change permission on interface %s of netId %u from %x to %x",
                  interface.c_str(), mNetId, mPermission, permission);
            return ret;
        }
    }
    if (mIsDefault) {
        for (const std::string& interface : mInterfaces) {
            if (int ret = addToDefault(mNetId, interface, permission, mDelegate)) {
                return ret;
            }
            if (int ret = removeFromDefault(mNetId, interface, mPermission, mDelegate)) {
                return ret;
            }
        }
    }
    mPermission = permission;
    return 0;
}

int PhysicalNetwork::addAsDefault() {
    if (mIsDefault) {
        return 0;
    }
    for (const std::string& interface : mInterfaces) {
        if (int ret = addToDefault(mNetId, interface, mPermission, mDelegate)) {
            return ret;
        }
    }
    mIsDefault = true;
    return 0;
}

int PhysicalNetwork::removeAsDefault() {
    if (!mIsDefault) {
        return 0;
    }
    for (const std::string& interface : mInterfaces) {
        if (int ret = removeFromDefault(mNetId, interface, mPermission, mDelegate)) {
            return ret;
        }
    }
    mIsDefault = false;
    return 0;
}

Network::Type PhysicalNetwork::getType() const {
    return PHYSICAL;
}

int PhysicalNetwork::addInterface(const std::string& interface) {
    if (hasInterface(interface)) {
        return 0;
    }
    if (int ret = RouteController::addInterfaceToPhysicalNetwork(mNetId, interface.c_str(),
                                                                 mPermission)) {
        ALOGE("failed to add interface %s to netId %u", interface.c_str(), mNetId);
        return ret;
    }
    if (mIsDefault) {
        if (int ret = addToDefault(mNetId, interface, mPermission, mDelegate)) {
            return ret;
        }
    }
    mInterfaces.insert(interface);
    return 0;
}

int PhysicalNetwork::removeInterface(const std::string& interface) {
    if (!hasInterface(interface)) {
        return 0;
    }
    if (mIsDefault) {
        if (int ret = removeFromDefault(mNetId, interface, mPermission, mDelegate)) {
            return ret;
        }
    }
    // This step will flush the interface index from the cache in RouteController so it must be
    // done last as further requests to the RouteController regarding this interface will fail
    // to find the interface index in the cache in cases where the interface is already gone
    // (e.g. bt-pan).
    if (int ret = RouteController::removeInterfaceFromPhysicalNetwork(mNetId, interface.c_str(),
                                                                      mPermission)) {
        ALOGE("failed to remove interface %s from netId %u", interface.c_str(), mNetId);
        return ret;
    }
    mInterfaces.erase(interface);
    return 0;
}
