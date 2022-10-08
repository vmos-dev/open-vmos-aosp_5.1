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

#ifndef NETD_INCLUDE_FWMARK_COMMAND_H
#define NETD_INCLUDE_FWMARK_COMMAND_H

#include <sys/types.h>

// Commands sent from clients to the fwmark server to mark sockets (i.e., set their SO_MARK).
struct FwmarkCommand {
    enum {
        ON_ACCEPT,
        ON_CONNECT,
        SELECT_NETWORK,
        PROTECT_FROM_VPN,
        SELECT_FOR_USER,
    } cmdId;
    unsigned netId;  // used only in the SELECT_NETWORK command; ignored otherwise.
    uid_t uid;  // used only in the SELECT_FOR_USER command; ignored otherwise.
};

#endif  // NETD_INCLUDE_FWMARK_COMMAND_H
