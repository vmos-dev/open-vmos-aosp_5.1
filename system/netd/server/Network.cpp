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

#include "Network.h"

#define LOG_TAG "Netd"
#include "log/log.h"

Network::~Network() {
    if (!mInterfaces.empty()) {
        ALOGE("deleting network with netId %u without clearing its interfaces", mNetId);
    }
}

unsigned Network::getNetId() const {
    return mNetId;
}

bool Network::hasInterface(const std::string& interface) const {
    return mInterfaces.find(interface) != mInterfaces.end();
}

const std::set<std::string>& Network::getInterfaces() const {
    return mInterfaces;
}

int Network::clearInterfaces() {
    while (!mInterfaces.empty()) {
        // Make a copy of the string, so removeInterface() doesn't lose its parameter when it
        // removes the string from the set.
        std::string interface = *mInterfaces.begin();
        if (int ret = removeInterface(interface)) {
            return ret;
        }
    }
    return 0;
}

Network::Network(unsigned netId) : mNetId(netId) {
}
