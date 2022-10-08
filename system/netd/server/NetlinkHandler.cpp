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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define LOG_TAG "Netd"

#include <cutils/log.h>

#include <netutils/ifc.h>
#include <sysutils/NetlinkEvent.h>
#include "NetlinkHandler.h"
#include "NetlinkManager.h"
#include "ResponseCode.h"

static const char *kUpdated = "updated";
static const char *kRemoved = "removed";

NetlinkHandler::NetlinkHandler(NetlinkManager *nm, int listenerSocket,
                               int format) :
                        NetlinkListener(listenerSocket, format) {
    mNm = nm;
}

NetlinkHandler::~NetlinkHandler() {
}

int NetlinkHandler::start() {
    return this->startListener();
}

int NetlinkHandler::stop() {
    return this->stopListener();
}

void NetlinkHandler::onEvent(NetlinkEvent *evt) {
    return ;
    const char *subsys = evt->getSubsystem();
    if (!subsys) {
        ALOGW("No subsystem found in netlink event");
        return;
    }

    if (!strcmp(subsys, "net")) {
        int action = evt->getAction();
        const char *iface = evt->findParam("INTERFACE");

        if (action == evt->NlActionAdd) {
            notifyInterfaceAdded(iface);
        } else if (action == evt->NlActionRemove) {
            notifyInterfaceRemoved(iface);
        } else if (action == evt->NlActionChange) {
            evt->dump();
            notifyInterfaceChanged("nana", true);
        } else if (action == evt->NlActionLinkUp) {
            notifyInterfaceLinkChanged(iface, true);
        } else if (action == evt->NlActionLinkDown) {
            notifyInterfaceLinkChanged(iface, false);
        } else if (action == evt->NlActionAddressUpdated ||
                   action == evt->NlActionAddressRemoved) {
            const char *address = evt->findParam("ADDRESS");
            const char *flags = evt->findParam("FLAGS");
            const char *scope = evt->findParam("SCOPE");
            if (action == evt->NlActionAddressRemoved && iface && address) {
                int resetMask = strchr(address, ':') ? RESET_IPV6_ADDRESSES : RESET_IPV4_ADDRESSES;
                resetMask |= RESET_IGNORE_INTERFACE_ADDRESS;
                if (int ret = ifc_reset_connections(iface, resetMask)) {
                    ALOGE("ifc_reset_connections failed on iface %s for address %s (%s)", iface,
                          address, strerror(ret));
                }
            }
            if (iface && flags && scope) {
                notifyAddressChanged(action, address, iface, flags, scope);
            }
        } else if (action == evt->NlActionRdnss) {
            const char *lifetime = evt->findParam("LIFETIME");
            const char *servers = evt->findParam("SERVERS");
            if (lifetime && servers) {
                notifyInterfaceDnsServers(iface, lifetime, servers);
            }
        } else if (action == evt->NlActionRouteUpdated ||
                   action == evt->NlActionRouteRemoved) {
            const char *route = evt->findParam("ROUTE");
            const char *gateway = evt->findParam("GATEWAY");
            const char *iface = evt->findParam("INTERFACE");
            if (route && (gateway || iface)) {
                notifyRouteChange(action, route, gateway, iface);
            }
        }

    } else if (!strcmp(subsys, "qlog")) {
        const char *alertName = evt->findParam("ALERT_NAME");
        const char *iface = evt->findParam("INTERFACE");
        notifyQuotaLimitReached(alertName, iface);

    } else if (!strcmp(subsys, "xt_idletimer")) {
        const char *label = evt->findParam("INTERFACE");
        const char *state = evt->findParam("STATE");
        const char *timestamp = evt->findParam("TIME_NS");
        if (state)
            notifyInterfaceClassActivity(label, !strcmp("active", state), timestamp);

#if !LOG_NDEBUG
    } else if (strcmp(subsys, "platform") && strcmp(subsys, "backlight")) {
        /* It is not a VSYNC or a backlight event */
        ALOGV("unexpected event from subsystem %s", subsys);
#endif
    }
}

void NetlinkHandler::notify(int code, const char *format, ...) {
    char *msg;
    va_list args;
    va_start(args, format);
    if (vasprintf(&msg, format, args) >= 0) {
        mNm->getBroadcaster()->sendBroadcast(code, msg, false);
        free(msg);
    } else {
        SLOGE("Failed to send notification: vasprintf: %s", strerror(errno));
    }
    va_end(args);
}

void NetlinkHandler::notifyInterfaceAdded(const char *name) {
    notify(ResponseCode::InterfaceChange, "Iface added %s", name);
}

void NetlinkHandler::notifyInterfaceRemoved(const char *name) {
    notify(ResponseCode::InterfaceChange, "Iface removed %s", name);
}

void NetlinkHandler::notifyInterfaceChanged(const char *name, bool isUp) {
    notify(ResponseCode::InterfaceChange,
           "Iface changed %s %s", name, (isUp ? "up" : "down"));
}

void NetlinkHandler::notifyInterfaceLinkChanged(const char *name, bool isUp) {
    notify(ResponseCode::InterfaceChange,
           "Iface linkstate %s %s", name, (isUp ? "up" : "down"));
}

void NetlinkHandler::notifyQuotaLimitReached(const char *name, const char *iface) {
    notify(ResponseCode::BandwidthControl, "limit alert %s %s", name, iface);
}

void NetlinkHandler::notifyInterfaceClassActivity(const char *name,
                                                  bool isActive, const char *timestamp) {
    if (timestamp == NULL)
        notify(ResponseCode::InterfaceClassActivity,
           "IfaceClass %s %s", isActive ? "active" : "idle", name);
    else
        notify(ResponseCode::InterfaceClassActivity,
           "IfaceClass %s %s %s", isActive ? "active" : "idle", name, timestamp);
}

void NetlinkHandler::notifyAddressChanged(int action, const char *addr,
                                          const char *iface, const char *flags,
                                          const char *scope) {
    notify(ResponseCode::InterfaceAddressChange,
           "Address %s %s %s %s %s",
           (action == NetlinkEvent::NlActionAddressUpdated) ? kUpdated : kRemoved,
           addr, iface, flags, scope);
}

void NetlinkHandler::notifyInterfaceDnsServers(const char *iface,
                                               const char *lifetime,
                                               const char *servers) {
    notify(ResponseCode::InterfaceDnsInfo, "DnsInfo servers %s %s %s",
           iface, lifetime, servers);
}

void NetlinkHandler::notifyRouteChange(int action, const char *route,
                                       const char *gateway, const char *iface) {
    notify(ResponseCode::RouteChange,
           "Route %s %s%s%s%s%s",
           (action == NetlinkEvent::NlActionRouteUpdated) ? kUpdated : kRemoved,
           route,
           *gateway ? " via " : "",
           gateway,
           *iface ? " dev " : "",
           iface);
}
