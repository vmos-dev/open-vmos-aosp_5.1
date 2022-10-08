/*
 * Copyright (C) 2010 The Android Open Source Project
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

#ifndef _DNSPROXYLISTENER_H__
#define _DNSPROXYLISTENER_H__

#include <sysutils/FrameworkListener.h>

#include "NetdCommand.h"

class NetworkController;

class DnsProxyListener : public FrameworkListener {
public:
    explicit DnsProxyListener(const NetworkController* netCtrl);
    virtual ~DnsProxyListener() {}

private:
    const NetworkController *mNetCtrl;
    class GetAddrInfoCmd : public NetdCommand {
    public:
        GetAddrInfoCmd(const DnsProxyListener* dnsProxyListener);
        virtual ~GetAddrInfoCmd() {}
        int runCommand(SocketClient *c, int argc, char** argv);
    private:
        const DnsProxyListener* mDnsProxyListener;
    };

    class GetAddrInfoHandler {
    public:
        // Note: All of host, service, and hints may be NULL
        GetAddrInfoHandler(SocketClient *c,
                           char* host,
                           char* service,
                           struct addrinfo* hints,
                           unsigned netId,
                           uint32_t mark);
        ~GetAddrInfoHandler();

        static void* threadStart(void* handler);
        void start();

    private:
        void run();
        SocketClient* mClient;  // ref counted
        char* mHost;    // owned
        char* mService; // owned
        struct addrinfo* mHints;  // owned
        unsigned mNetId;
        uint32_t mMark;
    };

    /* ------ gethostbyname ------*/
    class GetHostByNameCmd : public NetdCommand {
    public:
        GetHostByNameCmd(const DnsProxyListener* dnsProxyListener);
        virtual ~GetHostByNameCmd() {}
        int runCommand(SocketClient *c, int argc, char** argv);
    private:
        const DnsProxyListener* mDnsProxyListener;
    };

    class GetHostByNameHandler {
    public:
        GetHostByNameHandler(SocketClient *c,
                            char *name,
                            int af,
                            unsigned netId,
                            uint32_t mark);
        ~GetHostByNameHandler();
        static void* threadStart(void* handler);
        void start();
    private:
        void run();
        SocketClient* mClient; //ref counted
        char* mName; // owned
        int mAf;
        unsigned mNetId;
        uint32_t mMark;
    };

    /* ------ gethostbyaddr ------*/
    class GetHostByAddrCmd : public NetdCommand {
    public:
        GetHostByAddrCmd(const DnsProxyListener* dnsProxyListener);
        virtual ~GetHostByAddrCmd() {}
        int runCommand(SocketClient *c, int argc, char** argv);
    private:
        const DnsProxyListener* mDnsProxyListener;
    };

    class GetHostByAddrHandler {
    public:
        GetHostByAddrHandler(SocketClient *c,
                            void* address,
                            int addressLen,
                            int addressFamily,
                            unsigned netId,
                            uint32_t mark);
        ~GetHostByAddrHandler();

        static void* threadStart(void* handler);
        void start();

    private:
        void run();
        SocketClient* mClient;  // ref counted
        void* mAddress;    // address to lookup; owned
        int mAddressLen; // length of address to look up
        int mAddressFamily;  // address family
        unsigned mNetId;
        uint32_t mMark;
    };
};

#endif
