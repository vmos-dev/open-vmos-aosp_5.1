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

#ifndef _NETLINKMANAGER_H
#define _NETLINKMANAGER_H

#include <sysutils/SocketListener.h>
#include <sysutils/NetlinkListener.h>

class NetlinkHandler;

class NetlinkManager {
private:
    static NetlinkManager *sInstance;

private:
    SocketListener       *mBroadcaster;
    NetlinkHandler       *mHandler;
    int                  mSock;

public:
    virtual ~NetlinkManager();

    int start();
    int stop();

    void setBroadcaster(SocketListener *sl) { mBroadcaster = sl; }
    SocketListener *getBroadcaster() { return mBroadcaster; }

    static NetlinkManager *Instance();

private:
    NetlinkManager();
};
#endif
