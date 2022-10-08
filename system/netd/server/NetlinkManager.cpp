/*
 * Copyright (C) 2008 The Android Open Source Project
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

#include <stdio.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>

#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#define LOG_TAG "Netd"

#include <cutils/log.h>

#include "NetlinkManager.h"
#include "NetlinkHandler.h"

const int NetlinkManager::NFLOG_QUOTA_GROUP = 1;

NetlinkManager *NetlinkManager::sInstance = NULL;

NetlinkManager *NetlinkManager::Instance() {
    if (!sInstance)
        sInstance = new NetlinkManager();
    return sInstance;
}

NetlinkManager::NetlinkManager() {
    mBroadcaster = NULL;
}

NetlinkManager::~NetlinkManager() {
}

NetlinkHandler *NetlinkManager::setupSocket(int *sock, int netlinkFamily,
    int groups, int format) {

    struct sockaddr_nl nladdr;
    int sz = 64 * 1024;
    int on = 1;

    memset(&nladdr, 0, sizeof(nladdr));
    nladdr.nl_family = AF_NETLINK;
    nladdr.nl_pid = getpid();
    nladdr.nl_groups = groups;

    if ((*sock = socket(PF_NETLINK, SOCK_DGRAM, netlinkFamily)) < 0) {
        ALOGE("Unable to create netlink socket: %s", strerror(errno));
        return NULL;
    }

    if (setsockopt(*sock, SOL_SOCKET, SO_RCVBUFFORCE, &sz, sizeof(sz)) < 0) {
        ALOGE("Unable to set uevent socket SO_RCVBUFFORCE option: %s", strerror(errno));
        close(*sock);
        return NULL;
    }

    if (setsockopt(*sock, SOL_SOCKET, SO_PASSCRED, &on, sizeof(on)) < 0) {
        SLOGE("Unable to set uevent socket SO_PASSCRED option: %s", strerror(errno));
        close(*sock);
        return NULL;
    }

    // if (bind(*sock, (struct sockaddr *) &nladdr, sizeof(nladdr)) < 0) {
    //     ALOGE("Unable to bind netlink socket: %s", strerror(errno));
    //     close(*sock);
    //     return NULL;
    // }

    NetlinkHandler *handler = new NetlinkHandler(this, *sock, format);
    if (handler->start()) {
        ALOGE("Unable to start NetlinkHandler: %s", strerror(errno));
        close(*sock);
        return NULL;
    }

    return handler;
}

int NetlinkManager::start() {
#if 0   // rm-root
    if ((mUeventHandler = setupSocket(&mUeventSock, NETLINK_KOBJECT_UEVENT,
         0xffffffff, NetlinkListener::NETLINK_FORMAT_ASCII)) == NULL) {
        return -1;
    }
#endif

    if ((mRouteHandler = setupSocket(&mRouteSock, NETLINK_ROUTE,
                                     RTMGRP_LINK |
                                     RTMGRP_IPV4_IFADDR |
                                     RTMGRP_IPV6_IFADDR |
                                     RTMGRP_IPV6_ROUTE |
                                     (1 << (RTNLGRP_ND_USEROPT - 1)),
         NetlinkListener::NETLINK_FORMAT_BINARY)) == NULL) {
        return -1;
    }

#if 0   // rm-root
    if ((mQuotaHandler = setupSocket(&mQuotaSock, NETLINK_NFLOG,
        NFLOG_QUOTA_GROUP, NetlinkListener::NETLINK_FORMAT_BINARY)) == NULL) {
        ALOGE("Unable to open quota2 logging socket");
        // TODO: return -1 once the emulator gets a new kernel.
    }
#endif

    return 0;
}

int NetlinkManager::stop() {
    int status = 0;

    if (mUeventHandler->stop()) {
        ALOGE("Unable to stop uevent NetlinkHandler: %s", strerror(errno));
        status = -1;
    }

    delete mUeventHandler;
    mUeventHandler = NULL;

    close(mUeventSock);
    mUeventSock = -1;

    if (mRouteHandler->stop()) {
        ALOGE("Unable to stop route NetlinkHandler: %s", strerror(errno));
        status = -1;
    }

    delete mRouteHandler;
    mRouteHandler = NULL;

    close(mRouteSock);
    mRouteSock = -1;

    if (mQuotaHandler) {
        if (mQuotaHandler->stop()) {
            ALOGE("Unable to stop quota NetlinkHandler: %s", strerror(errno));
            status = -1;
        }

        delete mQuotaHandler;
        mQuotaHandler = NULL;

        close(mQuotaSock);
        mQuotaSock = -1;
    }

    return status;
}
