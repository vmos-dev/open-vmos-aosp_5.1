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

#include "FwmarkServer.h"

#include "Fwmark.h"
#include "FwmarkCommand.h"
#include "NetworkController.h"
#include "resolv_netid.h"

#include <sys/socket.h>
#include <unistd.h>

FwmarkServer::FwmarkServer(NetworkController* networkController) :
        SocketListener("fwmarkd", true), mNetworkController(networkController) {
}

bool FwmarkServer::onDataAvailable(SocketClient* client) {
    int socketFd = -1;
    int error = processClient(client, &socketFd);
    if (socketFd >= 0) {
        close(socketFd);
    }

    // Always send a response even if there were connection errors or read errors, so that we don't
    // inadvertently cause the client to hang (which always waits for a response).
    client->sendData(&error, sizeof(error));

    // Always close the client connection (by returning false). This prevents a DoS attack where
    // the client issues multiple commands on the same connection, never reading the responses,
    // causing its receive buffer to fill up, and thus causing our client->sendData() to block.
    return false;
}

int FwmarkServer::processClient(SocketClient* client, int* socketFd) {
    FwmarkCommand command;

    iovec iov;
    iov.iov_base = &command;
    iov.iov_len = sizeof(command);

    msghdr message;
    memset(&message, 0, sizeof(message));
    message.msg_iov = &iov;
    message.msg_iovlen = 1;

    union {
        cmsghdr cmh;
        char cmsg[CMSG_SPACE(sizeof(*socketFd))];
    } cmsgu;

    memset(cmsgu.cmsg, 0, sizeof(cmsgu.cmsg));
    message.msg_control = cmsgu.cmsg;
    message.msg_controllen = sizeof(cmsgu.cmsg);

    int messageLength = TEMP_FAILURE_RETRY(recvmsg(client->getSocket(), &message, 0));
    if (messageLength <= 0) {
        return -errno;
    }

    if (messageLength != sizeof(command)) {
        return -EBADMSG;
    }

    cmsghdr* const cmsgh = CMSG_FIRSTHDR(&message);
    if (cmsgh && cmsgh->cmsg_level == SOL_SOCKET && cmsgh->cmsg_type == SCM_RIGHTS &&
        cmsgh->cmsg_len == CMSG_LEN(sizeof(*socketFd))) {
        memcpy(socketFd, CMSG_DATA(cmsgh), sizeof(*socketFd));
    }

    if (*socketFd < 0) {
        return -EBADF;
    }

    Fwmark fwmark;
    socklen_t fwmarkLen = sizeof(fwmark.intValue);
    if (getsockopt(*socketFd, SOL_SOCKET, SO_MARK, &fwmark.intValue, &fwmarkLen) == -1) {
        return -errno;
    }

    Permission permission = mNetworkController->getPermissionForUser(client->getUid());

    switch (command.cmdId) {
        case FwmarkCommand::ON_ACCEPT: {
            // Called after a socket accept(). The kernel would've marked the NetId and necessary
            // permissions bits, so we just add the rest of the user's permissions here.
            permission = static_cast<Permission>(permission | fwmark.permission);
            break;
        }

        case FwmarkCommand::ON_CONNECT: {
            // Called before a socket connect() happens. Set an appropriate NetId into the fwmark so
            // that the socket routes consistently over that network. Do this even if the socket
            // already has a NetId, so that calling connect() multiple times still works.
            //
            // But if the explicit bit was set, the existing NetId was explicitly preferred (and not
            // a case of connect() being called multiple times). Don't reset the NetId in that case.
            //
            // An "appropriate" NetId is the NetId of a bypassable VPN that applies to the user, or
            // failing that, the default network. We'll never set the NetId of a secure VPN here.
            // See the comments in the implementation of getNetworkForConnect() for more details.
            //
            // If the protect bit is set, this could be either a system proxy (e.g.: the dns proxy
            // or the download manager) acting on behalf of another user, or a VPN provider. If it's
            // a proxy, we shouldn't reset the NetId. If it's a VPN provider, we should set the
            // default network's NetId.
            //
            // There's no easy way to tell the difference between a proxy and a VPN app. We can't
            // use PERMISSION_SYSTEM to identify the proxy because a VPN app may also have those
            // permissions. So we use the following heuristic:
            //
            // If it's a proxy, but the existing NetId is not a VPN, that means the user (that the
            // proxy is acting on behalf of) is not subject to a VPN, so the proxy must have picked
            // the default network's NetId. So, it's okay to replace that with the current default
            // network's NetId (which in all likelihood is the same).
            //
            // Conversely, if it's a VPN provider, the existing NetId cannot be a VPN. The only time
            // we set a VPN's NetId into a socket without setting the explicit bit is here, in
            // ON_CONNECT, but we won't do that if the socket has the protect bit set. If the VPN
            // provider connect()ed (and got the VPN NetId set) and then called protect(), we
            // would've unset the NetId in PROTECT_FROM_VPN below.
            //
            // So, overall (when the explicit bit is not set but the protect bit is set), if the
            // existing NetId is a VPN, don't reset it. Else, set the default network's NetId.
            if (!fwmark.explicitlySelected) {
                if (!fwmark.protectedFromVpn) {
                    fwmark.netId = mNetworkController->getNetworkForConnect(client->getUid());
                } else if (!mNetworkController->isVirtualNetwork(fwmark.netId)) {
                    fwmark.netId = mNetworkController->getDefaultNetwork();
                }
            }
            break;
        }

        case FwmarkCommand::SELECT_NETWORK: {
            fwmark.netId = command.netId;
            if (command.netId == NETID_UNSET) {
                fwmark.explicitlySelected = false;
                fwmark.protectedFromVpn = false;
                permission = PERMISSION_NONE;
            } else {
                if (int ret = mNetworkController->checkUserNetworkAccess(client->getUid(),
                                                                         command.netId)) {
                    return ret;
                }
                fwmark.explicitlySelected = true;
                fwmark.protectedFromVpn = mNetworkController->canProtect(client->getUid());
            }
            break;
        }

        case FwmarkCommand::PROTECT_FROM_VPN: {
            if (!mNetworkController->canProtect(client->getUid())) {
                return -EPERM;
            }
            // If a bypassable VPN's provider app calls connect() and then protect(), it will end up
            // with a socket that looks like that of a system proxy but is not (see comments for
            // ON_CONNECT above). So, reset the NetId.
            //
            // In any case, it's appropriate that if the socket has an implicit VPN NetId mark, the
            // PROTECT_FROM_VPN command should unset it.
            if (!fwmark.explicitlySelected && mNetworkController->isVirtualNetwork(fwmark.netId)) {
                fwmark.netId = mNetworkController->getDefaultNetwork();
            }
            fwmark.protectedFromVpn = true;
            permission = static_cast<Permission>(permission | fwmark.permission);
            break;
        }

        case FwmarkCommand::SELECT_FOR_USER: {
            if ((permission & PERMISSION_SYSTEM) != PERMISSION_SYSTEM) {
                return -EPERM;
            }
            fwmark.netId = mNetworkController->getNetworkForUser(command.uid);
            fwmark.protectedFromVpn = true;
            break;
        }

        default: {
            // unknown command
            return -EPROTO;
        }
    }

    fwmark.permission = permission;

    if (setsockopt(*socketFd, SOL_SOCKET, SO_MARK, &fwmark.intValue,
                   sizeof(fwmark.intValue)) == -1) {
        return -errno;
    }

    return 0;
}
