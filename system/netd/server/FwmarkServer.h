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

#ifndef NETD_SERVER_FWMARK_SERVER_H
#define NETD_SERVER_FWMARK_SERVER_H

#include "sysutils/SocketListener.h"

class NetworkController;

class FwmarkServer : public SocketListener {
public:
    explicit FwmarkServer(NetworkController* networkController);

private:
    // Overridden from SocketListener:
    bool onDataAvailable(SocketClient* client);

    // Returns 0 on success or a negative errno value on failure.
    int processClient(SocketClient* client, int* socketFd);

    NetworkController* const mNetworkController;
};

#endif  // NETD_SERVER_FWMARK_SERVER_H
