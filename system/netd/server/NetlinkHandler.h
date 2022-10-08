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

#ifndef _NETLINKHANDLER_H
#define _NETLINKHANDLER_H

#include <sysutils/NetlinkListener.h>
#include "NetlinkManager.h"

class NetlinkHandler: public NetlinkListener {
    NetlinkManager *mNm;

public:
    NetlinkHandler(NetlinkManager *nm, int listenerSocket, int format);
    virtual ~NetlinkHandler();

    int start(void);
    int stop(void);

protected:
    virtual void onEvent(NetlinkEvent *evt);

    void notify(int code, const char *format, ...);
    void notifyInterfaceAdded(const char *name);
    void notifyInterfaceRemoved(const char *name);
    void notifyInterfaceChanged(const char *name, bool isUp);
    void notifyInterfaceLinkChanged(const char *name, bool isUp);
    void notifyQuotaLimitReached(const char *name, const char *iface);
    void notifyInterfaceClassActivity(const char *name, bool isActive,
                                      const char *timestamp);
    void notifyAddressChanged(int action, const char *addr, const char *iface,
                              const char *flags, const char *scope);
    void notifyInterfaceDnsServers(const char *iface, const char *lifetime,
                                   const char *servers);
    void notifyRouteChange(int action, const char *route, const char *gateway, const char *iface);
};
#endif
