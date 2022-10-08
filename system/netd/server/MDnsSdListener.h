/*
 * Copyright (C) 2012 The Android Open Source Project
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

#ifndef _MDNSSDLISTENER_H__
#define _MDNSSDLISTENER_H__

#include <pthread.h>
#include <sysutils/FrameworkListener.h>
#include <dns_sd.h>

#include "NetdCommand.h"

// callbacks
void MDnsSdListenerDiscoverCallback(DNSServiceRef sdRef, DNSServiceFlags flags,
        uint32_t interfaceIndex, DNSServiceErrorType errorCode,
        const char *serviceName, const char *regType, const char *replyDomain,
        void *inContext);

void MDnsSdListenerRegisterCallback(DNSServiceRef sdRef, DNSServiceFlags flags,
        DNSServiceErrorType errorCode, const char *serviceName, const char *regType,
        const char *domain, void *inContext);

void MDnsSdListenerResolveCallback(DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interface,
        DNSServiceErrorType errorCode, const char *fullname, const char *hosttarget, uint16_t port,
        uint16_t txtLen, const unsigned char *txtRecord, void *inContext);

void MDnsSdListenerSetHostnameCallback(DNSServiceRef, DNSServiceFlags flags,
        DNSServiceErrorType errorCode, const char *hostname, void *inContext);

void MDnsSdListenerGetAddrInfoCallback(DNSServiceRef sdRef, DNSServiceFlags flags,
        uint32_t interface, DNSServiceErrorType errorCode, const char *hostname,
        const struct sockaddr *const sa, uint32_t ttl, void *inContext);

#define RESCAN "1"

class MDnsSdListener : public FrameworkListener {
public:
    MDnsSdListener();
    virtual ~MDnsSdListener() {}

    class Context {
    public:
        MDnsSdListener *mListener;
        int mRefNumber;

        Context(int refNumber, MDnsSdListener *m) {
            mRefNumber = refNumber;
            mListener = m;
        }

        ~Context() {
        }
    };

    class Monitor {
    public:
        Monitor();
        virtual ~Monitor() {}
        DNSServiceRef *allocateServiceRef(int id, Context *c);
        void startMonitoring(int id);
        DNSServiceRef *lookupServiceRef(int id);
        void freeServiceRef(int id);
        static void *threadStart(void *handler);
        int startService();
        int stopService();
    private:
        void run();
        int rescan(); // returns the number of elements in the poll
        class Element {
        public:
            int mId;
            Element *mNext;
            DNSServiceRef mRef;
            Context *mContext;
            int mReady;
            Element(int id, Context *context)
                    : mId(id), mNext(NULL), mContext(context), mReady(0) {}
            virtual ~Element() { delete(mContext); }
        };
        Element *mHead;
        int mLiveCount;
        struct pollfd *mPollFds;
        DNSServiceRef **mPollRefs;
        int mPollSize;
        pthread_t mThread;
        int mCtrlSocketPair[2];
        pthread_mutex_t mHeadMutex;
    };

    class Handler : public NetdCommand {
    public:
        Handler(Monitor *m, MDnsSdListener *listener);
        virtual ~Handler();
        int runCommand(SocketClient *c, int argc, char** argv);

        MDnsSdListener *mListener; // needed for broadcast purposes
    private:
        void stop(SocketClient *cli, int argc, char **argv, const char *str);

        void discover(SocketClient *cli, const char *iface, const char *regType,
                const char *domain, const int requestNumber,
                const int requestFlags);

        void serviceRegister(SocketClient *cli, int requestId, const char *interfaceName,
                const char *serviceName, const char *serviceType, const char *domain,
                const char *host, int port, int textLen, void *txtRecord);

        void resolveService(SocketClient *cli, int requestId,
                const char *interfaceName, const char *serviceName, const char *regType,
                const char *domain);

        void setHostname(SocketClient *cli, int requestId, const char *hostname);

        void getAddrInfo(SocketClient *cli, int requestId, const char *interfaceName,
                uint32_t protocol, const char *hostname);

        int ifaceNameToI(const char *iface);
        const char *iToIfaceName(int i);
        DNSServiceFlags iToFlags(int i);
        int flagsToI(DNSServiceFlags flags);
        Monitor *mMonitor;
    };
};

#endif
