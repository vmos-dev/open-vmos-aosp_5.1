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

#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <linux/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>

#define LOG_TAG "MDnsDS"
#define DBG 1
#define VDBG 1

#include <cutils/log.h>
#include <cutils/properties.h>
#include <sysutils/SocketClient.h>

#include "MDnsSdListener.h"
#include "ResponseCode.h"

#define MDNS_SERVICE_NAME "mdnsd"
#define MDNS_SERVICE_STATUS "init.svc.mdnsd"

MDnsSdListener::MDnsSdListener() :
                 FrameworkListener("mdns", true) {
    Monitor *m = new Monitor();
    registerCmd(new Handler(m, this));
}

MDnsSdListener::Handler::Handler(Monitor *m, MDnsSdListener *listener) :
   NetdCommand("mdnssd") {
   if (DBG) ALOGD("MDnsSdListener::Hander starting up");
   mMonitor = m;
   mListener = listener;
}

MDnsSdListener::Handler::~Handler() {}

void MDnsSdListener::Handler::discover(SocketClient *cli,
        const char *iface,
        const char *regType,
        const char *domain,
        const int requestId,
        const int requestFlags) {
    if (VDBG) {
        ALOGD("discover(%s, %s, %s, %d, %d)", iface, regType, domain, requestId,
                requestFlags);
    }
    Context *context = new Context(requestId, mListener);
    DNSServiceRef *ref = mMonitor->allocateServiceRef(requestId, context);
    if (ref == NULL) {
        ALOGE("requestId %d already in use during discover call", requestId);
        cli->sendMsg(ResponseCode::CommandParameterError,
                "RequestId already in use during discover call", false);
        return;
    }
    if (VDBG) ALOGD("using ref %p", ref);
    DNSServiceFlags nativeFlags = iToFlags(requestFlags);
    int interfaceInt = ifaceNameToI(iface);

    DNSServiceErrorType result = DNSServiceBrowse(ref, nativeFlags, interfaceInt, regType,
            domain, &MDnsSdListenerDiscoverCallback, context);
    if (result != kDNSServiceErr_NoError) {
        ALOGE("Discover request %d got an error from DNSServiceBrowse %d", requestId, result);
        mMonitor->freeServiceRef(requestId);
        cli->sendMsg(ResponseCode::CommandParameterError,
                "Discover request got an error from DNSServiceBrowse", false);
        return;
    }
    mMonitor->startMonitoring(requestId);
    if (VDBG) ALOGD("discover successful");
    cli->sendMsg(ResponseCode::CommandOkay, "Discover operation started", false);
    return;
}

void MDnsSdListenerDiscoverCallback(DNSServiceRef /* sdRef */, DNSServiceFlags flags,
        uint32_t /* interfaceIndex */, DNSServiceErrorType errorCode, const char *serviceName,
        const char *regType, const char *replyDomain, void *inContext) {
    MDnsSdListener::Context *context = reinterpret_cast<MDnsSdListener::Context *>(inContext);
    char *msg;
    int refNumber = context->mRefNumber;

    if (errorCode != kDNSServiceErr_NoError) {
        asprintf(&msg, "%d %d", refNumber, errorCode);
        context->mListener->sendBroadcast(ResponseCode::ServiceDiscoveryFailed, msg, false);
        if (DBG) ALOGE("discover failure for %d, error= %d", refNumber, errorCode);
    } else {
        int respCode;
        char *quotedServiceName = SocketClient::quoteArg(serviceName);
        if (flags & kDNSServiceFlagsAdd) {
            if (VDBG) {
                ALOGD("Discover found new serviceName %s, regType %s and domain %s for %d",
                        serviceName, regType, replyDomain, refNumber);
            }
            respCode = ResponseCode::ServiceDiscoveryServiceAdded;
        } else {
            if (VDBG) {
                ALOGD("Discover lost serviceName %s, regType %s and domain %s for %d",
                        serviceName, regType, replyDomain, refNumber);
            }
            respCode = ResponseCode::ServiceDiscoveryServiceRemoved;
        }
        asprintf(&msg, "%d %s %s %s", refNumber, quotedServiceName, regType, replyDomain);
        free(quotedServiceName);
        context->mListener->sendBroadcast(respCode, msg, false);
    }
    free(msg);
}

void MDnsSdListener::Handler::stop(SocketClient *cli, int argc, char **argv, const char *str) {
    if (argc != 3) {
        char *msg;
        asprintf(&msg, "Invalid number of arguments to %s", str);
        cli->sendMsg(ResponseCode::CommandParameterError, msg, false);
        free(msg);
        return;
    }
    int requestId = atoi(argv[2]);
    DNSServiceRef *ref = mMonitor->lookupServiceRef(requestId);
    if (ref == NULL) {
        if (DBG) ALOGE("%s stop used unknown requestId %d", str, requestId);
        cli->sendMsg(ResponseCode::CommandParameterError, "Unknown requestId", false);
        return;
    }
    if (VDBG) ALOGD("Stopping %s with ref %p", str, ref);
    DNSServiceRefDeallocate(*ref);
    mMonitor->freeServiceRef(requestId);
    char *msg;
    asprintf(&msg, "%s stopped", str);
    cli->sendMsg(ResponseCode::CommandOkay, msg, false);
    free(msg);
}

void MDnsSdListener::Handler::serviceRegister(SocketClient *cli, int requestId,
        const char *interfaceName, const char *serviceName, const char *serviceType,
        const char *domain, const char *host, int port, int txtLen, void *txtRecord) {
    if (VDBG) {
        ALOGD("serviceRegister(%d, %s, %s, %s, %s, %s, %d, %d, <binary>)", requestId,
                interfaceName, serviceName, serviceType, domain, host, port, txtLen);
    }
    Context *context = new Context(requestId, mListener);
    DNSServiceRef *ref = mMonitor->allocateServiceRef(requestId, context);
    port = htons(port);
    if (ref == NULL) {
        ALOGE("requestId %d already in use during register call", requestId);
        cli->sendMsg(ResponseCode::CommandParameterError,
                "RequestId already in use during register call", false);
        return;
    }
    DNSServiceFlags nativeFlags = 0;
    int interfaceInt = ifaceNameToI(interfaceName);
    DNSServiceErrorType result = DNSServiceRegister(ref, interfaceInt, nativeFlags, serviceName,
            serviceType, domain, host, port, txtLen, txtRecord, &MDnsSdListenerRegisterCallback,
            context);
    if (result != kDNSServiceErr_NoError) {
        ALOGE("service register request %d got an error from DNSServiceRegister %d", requestId,
                result);
        mMonitor->freeServiceRef(requestId);
        cli->sendMsg(ResponseCode::CommandParameterError,
                "serviceRegister request got an error from DNSServiceRegister", false);
        return;
    }
    mMonitor->startMonitoring(requestId);
    if (VDBG) ALOGD("serviceRegister successful");
    cli->sendMsg(ResponseCode::CommandOkay, "serviceRegister started", false);
    return;
}

void MDnsSdListenerRegisterCallback(DNSServiceRef /* sdRef */, DNSServiceFlags /* flags */,
        DNSServiceErrorType errorCode, const char *serviceName, const char * /* regType */,
        const char * /* domain */, void *inContext) {
    MDnsSdListener::Context *context = reinterpret_cast<MDnsSdListener::Context *>(inContext);
    char *msg;
    int refNumber = context->mRefNumber;
    if (errorCode != kDNSServiceErr_NoError) {
        asprintf(&msg, "%d %d", refNumber, errorCode);
        context->mListener->sendBroadcast(ResponseCode::ServiceRegistrationFailed, msg, false);
        if (DBG) ALOGE("register failure for %d, error= %d", refNumber, errorCode);
    } else {
        char *quotedServiceName = SocketClient::quoteArg(serviceName);
        asprintf(&msg, "%d %s", refNumber, quotedServiceName);
        free(quotedServiceName);
        context->mListener->sendBroadcast(ResponseCode::ServiceRegistrationSucceeded, msg, false);
        if (VDBG) ALOGD("register succeeded for %d as %s", refNumber, serviceName);
    }
    free(msg);
}


void MDnsSdListener::Handler::resolveService(SocketClient *cli, int requestId,
        const char *interfaceName, const char *serviceName, const char *regType,
        const char *domain) {
    if (VDBG) {
        ALOGD("resolveService(%d, %s, %s, %s, %s)", requestId, interfaceName,
                serviceName, regType, domain);
    }
    Context *context = new Context(requestId, mListener);
    DNSServiceRef *ref = mMonitor->allocateServiceRef(requestId, context);
    if (ref == NULL) {
        ALOGE("request Id %d already in use during resolve call", requestId);
        cli->sendMsg(ResponseCode::CommandParameterError,
                "RequestId already in use during resolve call", false);
        return;
    }
    DNSServiceFlags nativeFlags = 0;
    int interfaceInt = ifaceNameToI(interfaceName);
    DNSServiceErrorType result = DNSServiceResolve(ref, nativeFlags, interfaceInt, serviceName,
            regType, domain, &MDnsSdListenerResolveCallback, context);
    if (result != kDNSServiceErr_NoError) {
        ALOGE("service resolve request %d got an error from DNSServiceResolve %d", requestId,
                result);
        mMonitor->freeServiceRef(requestId);
        cli->sendMsg(ResponseCode::CommandParameterError,
                "resolveService got an error from DNSServiceResolve", false);
        return;
    }
    mMonitor->startMonitoring(requestId);
    if (VDBG) ALOGD("resolveService successful");
    cli->sendMsg(ResponseCode::CommandOkay, "resolveService started", false);
    return;
}

void MDnsSdListenerResolveCallback(DNSServiceRef /* sdRef */, DNSServiceFlags /* flags */,
        uint32_t /* interface */, DNSServiceErrorType errorCode, const char *fullname,
        const char *hosttarget, uint16_t port, uint16_t txtLen,
        const unsigned char * /* txtRecord */, void *inContext) {
    MDnsSdListener::Context *context = reinterpret_cast<MDnsSdListener::Context *>(inContext);
    char *msg;
    int refNumber = context->mRefNumber;
    port = ntohs(port);
    if (errorCode != kDNSServiceErr_NoError) {
        asprintf(&msg, "%d %d", refNumber, errorCode);
        context->mListener->sendBroadcast(ResponseCode::ServiceResolveFailed, msg, false);
        if (DBG) ALOGE("resolve failure for %d, error= %d", refNumber, errorCode);
    } else {
        char *quotedFullName = SocketClient::quoteArg(fullname);
        char *quotedHostTarget = SocketClient::quoteArg(hosttarget);
        asprintf(&msg, "%d %s %s %d %d", refNumber, quotedFullName, quotedHostTarget, port, txtLen);
        free(quotedFullName);
        free(quotedHostTarget);
        context->mListener->sendBroadcast(ResponseCode::ServiceResolveSuccess, msg, false);
        if (VDBG) {
            ALOGD("resolve succeeded for %d finding %s at %s:%d with txtLen %d",
                    refNumber, fullname, hosttarget, port, txtLen);
        }
    }
    free(msg);
}

void MDnsSdListener::Handler::getAddrInfo(SocketClient *cli, int requestId,
        const char *interfaceName, uint32_t protocol, const char *hostname) {
    if (VDBG) ALOGD("getAddrInfo(%d, %s %d, %s)", requestId, interfaceName, protocol, hostname);
    Context *context = new Context(requestId, mListener);
    DNSServiceRef *ref = mMonitor->allocateServiceRef(requestId, context);
    if (ref == NULL) {
        ALOGE("request ID %d already in use during getAddrInfo call", requestId);
        cli->sendMsg(ResponseCode::CommandParameterError,
                "RequestId already in use during getAddrInfo call", false);
        return;
    }
    DNSServiceFlags nativeFlags = 0;
    int interfaceInt = ifaceNameToI(interfaceName);
    DNSServiceErrorType result = DNSServiceGetAddrInfo(ref, nativeFlags, interfaceInt, protocol,
            hostname, &MDnsSdListenerGetAddrInfoCallback, context);
    if (result != kDNSServiceErr_NoError) {
        ALOGE("getAddrInfo request %d got an error from DNSServiceGetAddrInfo %d", requestId,
                result);
        mMonitor->freeServiceRef(requestId);
        cli->sendMsg(ResponseCode::CommandParameterError,
                "getAddrInfo request got an error from DNSServiceGetAddrInfo", false);
        return;
    }
    mMonitor->startMonitoring(requestId);
    if (VDBG) ALOGD("getAddrInfo successful");
    cli->sendMsg(ResponseCode::CommandOkay, "getAddrInfo started", false);
    return;
}

void MDnsSdListenerGetAddrInfoCallback(DNSServiceRef /* sdRef */, DNSServiceFlags /* flags */,
        uint32_t /* interface */, DNSServiceErrorType errorCode, const char *hostname,
        const struct sockaddr *const sa, uint32_t ttl, void *inContext) {
    MDnsSdListener::Context *context = reinterpret_cast<MDnsSdListener::Context *>(inContext);
    int refNumber = context->mRefNumber;

    if (errorCode != kDNSServiceErr_NoError) {
        char *msg;
        asprintf(&msg, "%d %d", refNumber, errorCode);
        context->mListener->sendBroadcast(ResponseCode::ServiceGetAddrInfoFailed, msg, false);
        if (DBG) ALOGE("getAddrInfo failure for %d, error= %d", refNumber, errorCode);
        free(msg);
    } else {
        char addr[INET6_ADDRSTRLEN];
        char *msg;
        char *quotedHostname = SocketClient::quoteArg(hostname);
        if (sa->sa_family == AF_INET) {
            inet_ntop(sa->sa_family, &(((struct sockaddr_in *)sa)->sin_addr), addr, sizeof(addr));
        } else {
            inet_ntop(sa->sa_family, &(((struct sockaddr_in6 *)sa)->sin6_addr), addr, sizeof(addr));
        }
        asprintf(&msg, "%d %s %d %s", refNumber, quotedHostname, ttl, addr);
        free(quotedHostname);
        context->mListener->sendBroadcast(ResponseCode::ServiceGetAddrInfoSuccess, msg, false);
        if (VDBG) {
            ALOGD("getAddrInfo succeeded for %d: %s", refNumber, msg);
        }
        free(msg);
    }
}

void MDnsSdListener::Handler::setHostname(SocketClient *cli, int requestId,
        const char *hostname) {
    if (VDBG) ALOGD("setHostname(%d, %s)", requestId, hostname);
    Context *context = new Context(requestId, mListener);
    DNSServiceRef *ref = mMonitor->allocateServiceRef(requestId, context);
    if (ref == NULL) {
        ALOGE("request Id %d already in use during setHostname call", requestId);
        cli->sendMsg(ResponseCode::CommandParameterError,
                "RequestId already in use during setHostname call", false);
        return;
    }
    DNSServiceFlags nativeFlags = 0;
    DNSServiceErrorType result = DNSSetHostname(ref, nativeFlags, hostname,
            &MDnsSdListenerSetHostnameCallback, context);
    if (result != kDNSServiceErr_NoError) {
        ALOGE("setHostname request %d got an error from DNSSetHostname %d", requestId, result);
        mMonitor->freeServiceRef(requestId);
        cli->sendMsg(ResponseCode::CommandParameterError,
                "setHostname got an error from DNSSetHostname", false);
        return;
    }
    mMonitor->startMonitoring(requestId);
    if (VDBG) ALOGD("setHostname successful");
    cli->sendMsg(ResponseCode::CommandOkay, "setHostname started", false);
    return;
}

void MDnsSdListenerSetHostnameCallback(DNSServiceRef /* sdRef */, DNSServiceFlags /* flags */,
        DNSServiceErrorType errorCode, const char *hostname, void *inContext) {
    MDnsSdListener::Context *context = reinterpret_cast<MDnsSdListener::Context *>(inContext);
    char *msg;
    int refNumber = context->mRefNumber;
    if (errorCode != kDNSServiceErr_NoError) {
        asprintf(&msg, "%d %d", refNumber, errorCode);
        context->mListener->sendBroadcast(ResponseCode::ServiceSetHostnameFailed, msg, false);
        if (DBG) ALOGE("setHostname failure for %d, error= %d", refNumber, errorCode);
    } else {
        char *quotedHostname = SocketClient::quoteArg(hostname);
        asprintf(&msg, "%d %s", refNumber, quotedHostname);
        free(quotedHostname);
        context->mListener->sendBroadcast(ResponseCode::ServiceSetHostnameSuccess, msg, false);
        if (VDBG) ALOGD("setHostname succeeded for %d.  Set to %s", refNumber, hostname);
    }
    free(msg);
}


int MDnsSdListener::Handler::ifaceNameToI(const char * /* iface */) {
    return 0;
}

const char *MDnsSdListener::Handler::iToIfaceName(int /* i */) {
    return NULL;
}

DNSServiceFlags MDnsSdListener::Handler::iToFlags(int /* i */) {
    return 0;
}

int MDnsSdListener::Handler::flagsToI(DNSServiceFlags /* flags */) {
    return 0;
}

int MDnsSdListener::Handler::runCommand(SocketClient *cli,
                                        int argc, char **argv) {
    if (argc < 2) {
        char* msg = NULL;
        asprintf( &msg, "Invalid number of arguments to mdnssd: %i", argc);
        ALOGW("%s", msg);
        cli->sendMsg(ResponseCode::CommandParameterError, msg, false);
        free(msg);
        return -1;
    }

    char* cmd = argv[1];

    if (strcmp(cmd, "discover") == 0) {
        if (argc != 4) {
            cli->sendMsg(ResponseCode::CommandParameterError,
                    "Invalid number of arguments to mdnssd discover", false);
            return 0;
        }
        int requestId = atoi(argv[2]);
        char *serviceType = argv[3];

        discover(cli, NULL, serviceType, NULL, requestId, 0);
    } else if (strcmp(cmd, "stop-discover") == 0) {
        stop(cli, argc, argv, "discover");
    } else if (strcmp(cmd, "register") == 0) {
        if (argc < 6) {
            cli->sendMsg(ResponseCode::CommandParameterError,
                    "Invalid number of arguments to mdnssd register", false);
            return 0;
        }
        int requestId = atoi(argv[2]);
        char *serviceName = argv[3];
        char *serviceType = argv[4];
        int port = atoi(argv[5]);
        char *interfaceName = NULL; // will use all
        char *domain = NULL;        // will use default
        char *host = NULL;          // will use default hostname
        unsigned char txtRecord[2048] = "";
        unsigned char *ptr = txtRecord;
        for (int i = 6; i < argc; ++i) {
          int dataLength = strlen(argv[i]);
          if (dataLength < 1) {
            continue;
          }
          if (dataLength > 255) {
            cli->sendMsg(ResponseCode::CommandParameterError,
                    "TXT record fields must not be longer than 255 characters", false);
            return 0;
          }
          if (ptr + dataLength + 1 > txtRecord + sizeof(txtRecord)) {
            cli->sendMsg(ResponseCode::CommandParameterError,
                    "Total length of TXT record must be smaller than 2048 bytes", false);
            return 0;
          }
          *ptr++ = dataLength;
          strcpy( (char*) ptr, argv[i]);
          ptr += dataLength;
        }
        serviceRegister(cli, requestId, interfaceName, serviceName,
                serviceType, domain, host, port, ptr - txtRecord, txtRecord);
    } else if (strcmp(cmd, "stop-register") == 0) {
        stop(cli, argc, argv, "register");
    } else if (strcmp(cmd, "resolve") == 0) {
        if (argc != 6) {
            cli->sendMsg(ResponseCode::CommandParameterError,
                    "Invalid number of arguments to mdnssd resolve", false);
            return 0;
        }
        int requestId = atoi(argv[2]);
        char *interfaceName = NULL;  // will use all
        char *serviceName = argv[3];
        char *regType = argv[4];
        char *domain = argv[5];
        resolveService(cli, requestId, interfaceName, serviceName, regType, domain);
    } else if (strcmp(cmd, "stop-resolve") == 0) {
        stop(cli, argc, argv, "resolve");
    } else if (strcmp(cmd, "start-service") == 0) {
        if (mMonitor->startService()) {
            cli->sendMsg(ResponseCode::CommandOkay, "Service Started", false);
        } else {
            cli->sendMsg(ResponseCode::ServiceStartFailed, "Service already running", false);
        }
    } else if (strcmp(cmd, "stop-service") == 0) {
        if (mMonitor->stopService()) {
            cli->sendMsg(ResponseCode::CommandOkay, "Service Stopped", false);
        } else {
            cli->sendMsg(ResponseCode::ServiceStopFailed, "Service still in use", false);
        }
    } else if (strcmp(cmd, "sethostname") == 0) {
        if (argc != 4) {
            cli->sendMsg(ResponseCode::CommandParameterError,
                    "Invalid number of arguments to mdnssd sethostname", false);
            return 0;
        }
        int requestId = atoi(argv[2]);
        char *hostname = argv[3];
        setHostname(cli, requestId, hostname);
    } else if (strcmp(cmd, "stop-sethostname") == 0) {
        stop(cli, argc, argv, "sethostname");
    } else if (strcmp(cmd, "getaddrinfo") == 0) {
        if (argc != 4) {
            cli->sendMsg(ResponseCode::CommandParameterError,
                    "Invalid number of arguments to mdnssd getaddrinfo", false);
            return 0;
        }
        int requestId = atoi(argv[2]);
        char *hostname = argv[3];
        char *interfaceName = NULL;  // default
        int protocol = 0;            // intelligient heuristic (both v4 + v6)
        getAddrInfo(cli, requestId, interfaceName, protocol, hostname);
    } else if (strcmp(cmd, "stop-getaddrinfo") == 0) {
        stop(cli, argc, argv, "getaddrinfo");
    } else {
        if (VDBG) ALOGE("Unknown cmd %s", cmd);
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Unknown mdnssd cmd", false);
        return 0;
    }
    return 0;
}

MDnsSdListener::Monitor::Monitor() {
    mHead = NULL;
    mLiveCount = 0;
    mPollFds = NULL;
    mPollRefs = NULL;
    mPollSize = 10;
    socketpair(AF_LOCAL, SOCK_STREAM, 0, mCtrlSocketPair);
    pthread_mutex_init(&mHeadMutex, NULL);

    pthread_create(&mThread, NULL, MDnsSdListener::Monitor::threadStart, this);
    pthread_detach(mThread);
}

void *MDnsSdListener::Monitor::threadStart(void *obj) {
    Monitor *monitor = reinterpret_cast<Monitor *>(obj);

    monitor->run();
    delete monitor;
    pthread_exit(NULL);
    return NULL;
}

#define NAP_TIME 200  // 200 ms between polls
static int wait_for_property(const char *name, const char *desired_value, int maxwait)
{
    char value[PROPERTY_VALUE_MAX] = {'\0'};
    int maxnaps = (maxwait * 1000) / NAP_TIME;

    if (maxnaps < 1) {
        maxnaps = 1;
    }

    while (maxnaps-- > 0) {
        usleep(NAP_TIME * 1000);
        if (property_get(name, value, NULL)) {
            if (desired_value == NULL || strcmp(value, desired_value) == 0) {
                return 0;
            }
        }
    }
    return -1; /* failure */
}

int MDnsSdListener::Monitor::startService() {
    int result = 0;
    char property_value[PROPERTY_VALUE_MAX];
    pthread_mutex_lock(&mHeadMutex);
    property_get(MDNS_SERVICE_STATUS, property_value, "");
    if (strcmp("running", property_value) != 0) {
        ALOGD("Starting MDNSD");
        property_set("ctl.start", MDNS_SERVICE_NAME);
        wait_for_property(MDNS_SERVICE_STATUS, "running", 5);
        result = -1;
    } else {
        result = 0;
    }
    pthread_mutex_unlock(&mHeadMutex);
    return result;
}

int MDnsSdListener::Monitor::stopService() {
    int result = 0;
    pthread_mutex_lock(&mHeadMutex);
    if (mHead == NULL) {
        ALOGD("Stopping MDNSD");
        property_set("ctl.stop", MDNS_SERVICE_NAME);
        wait_for_property(MDNS_SERVICE_STATUS, "stopped", 5);
        result = -1;
    } else {
        result = 0;
    }
    pthread_mutex_unlock(&mHeadMutex);
    return result;
}

void MDnsSdListener::Monitor::run() {
    int pollCount = 1;

    mPollFds = (struct pollfd *)calloc(sizeof(struct pollfd), mPollSize);
    mPollRefs = (DNSServiceRef **)calloc(sizeof(DNSServiceRef *), mPollSize);
    LOG_ALWAYS_FATAL_IF((mPollFds == NULL), "initial calloc failed on mPollFds with a size of %d",
            ((int)sizeof(struct pollfd)) * mPollSize);
    LOG_ALWAYS_FATAL_IF((mPollRefs == NULL), "initial calloc failed on mPollRefs with a size of %d",
            ((int)sizeof(DNSServiceRef *)) * mPollSize);

    mPollFds[0].fd = mCtrlSocketPair[0];
    mPollFds[0].events = POLLIN;

    if (VDBG) ALOGD("MDnsSdListener starting to monitor");
    while (1) {
        if (VDBG) ALOGD("Going to poll with pollCount %d", pollCount);
        int pollResults = poll(mPollFds, pollCount, 10000000);
        if (pollResults < 0) {
            ALOGE("Error in poll - got %d", errno);
        } else if (pollResults > 0) {
            if (VDBG) ALOGD("Monitor poll got data pollCount = %d, %d", pollCount, pollResults);
            for(int i = 1; i < pollCount; i++) {
                if (mPollFds[i].revents != 0) {
                    if (VDBG) {
                        ALOGD("Monitor found [%d].revents = %d - calling ProcessResults",
                                i, mPollFds[i].revents);
                    }
                    DNSServiceProcessResult(*(mPollRefs[i]));
                    mPollFds[i].revents = 0;
                }
            }
            if (VDBG) ALOGD("controlSocket shows revent= %d", mPollFds[0].revents);
            switch (mPollFds[0].revents) {
                case POLLIN: {
                    char readBuf[2];
                    read(mCtrlSocketPair[0], &readBuf, 1);
                    if (DBG) ALOGD("MDnsSdListener::Monitor got %c", readBuf[0]);
                    if (memcmp(RESCAN, readBuf, 1) == 0) {
                        pollCount = rescan();
                    }
                }
            }
            mPollFds[0].revents = 0;
        } else {
            if (VDBG) ALOGD("MDnsSdListener::Monitor poll timed out");
        }
    }
    free(mPollFds);
    free(mPollRefs);
}

#define DBG_RESCAN 0

int MDnsSdListener::Monitor::rescan() {
// rescan the list from mHead and make new pollfds and serviceRefs
    if (VDBG) {
        ALOGD("MDnsSdListener::Monitor poll rescanning - size=%d, live=%d", mPollSize, mLiveCount);
    }
    pthread_mutex_lock(&mHeadMutex);
    Element **prevPtr = &mHead;
    int i = 1;
    if (mPollSize <= mLiveCount) {
        mPollSize = mLiveCount + 5;
        free(mPollFds);
        free(mPollRefs);
        mPollFds = (struct pollfd *)calloc(sizeof(struct pollfd), mPollSize);
        mPollRefs = (DNSServiceRef **)calloc(sizeof(DNSServiceRef *), mPollSize);
        LOG_ALWAYS_FATAL_IF((mPollFds == NULL), "calloc failed on mPollFds with a size of %d",
                ((int)sizeof(struct pollfd)) * mPollSize);
        LOG_ALWAYS_FATAL_IF((mPollRefs == NULL), "calloc failed on mPollRefs with a size of %d",
                ((int)sizeof(DNSServiceRef *)) * mPollSize);
    } else {
        memset(mPollFds, 0, sizeof(struct pollfd) * mPollSize);
        memset(mPollRefs, 0, sizeof(DNSServiceRef *) * mPollSize);
    }
    mPollFds[0].fd = mCtrlSocketPair[0];
    mPollFds[0].events = POLLIN;
    if (DBG_RESCAN) ALOGD("mHead = %p", mHead);
    while (*prevPtr != NULL) {
        if (DBG_RESCAN) ALOGD("checking %p, mReady = %d", *prevPtr, (*prevPtr)->mReady);
        if ((*prevPtr)->mReady == 1) {
            int fd = DNSServiceRefSockFD((*prevPtr)->mRef);
            if (fd != -1) {
                if (DBG_RESCAN) ALOGD("  adding FD %d", fd);
                mPollFds[i].fd = fd;
                mPollFds[i].events = POLLIN;
                mPollRefs[i] = &((*prevPtr)->mRef);
                i++;
            } else {
                ALOGE("Error retreving socket FD for live ServiceRef");
            }
            prevPtr = &((*prevPtr)->mNext); // advance to the next element
        } else if ((*prevPtr)->mReady == -1) {
            if (DBG_RESCAN) ALOGD("  removing %p from  play", *prevPtr);
            Element *cur = *prevPtr;
            *prevPtr = (cur)->mNext; // change our notion of this element and don't advance
            delete cur;
        } else if ((*prevPtr)->mReady == 0) {
            // Not ready so just skip this node and continue on
            if (DBG_RESCAN) ALOGD("%p not ready.  Continuing.", *prevPtr);
            prevPtr = &((*prevPtr)->mNext);
        }
    }
    pthread_mutex_unlock(&mHeadMutex);
    return i;
}

DNSServiceRef *MDnsSdListener::Monitor::allocateServiceRef(int id, Context *context) {
    if (lookupServiceRef(id) != NULL) {
        delete(context);
        return NULL;
    }
    Element *e = new Element(id, context);
    pthread_mutex_lock(&mHeadMutex);
    e->mNext = mHead;
    mHead = e;
    pthread_mutex_unlock(&mHeadMutex);
    return &(e->mRef);
}

DNSServiceRef *MDnsSdListener::Monitor::lookupServiceRef(int id) {
    pthread_mutex_lock(&mHeadMutex);
    Element *cur = mHead;
    while (cur != NULL) {
        if (cur->mId == id) {
            DNSServiceRef *result = &(cur->mRef);
            pthread_mutex_unlock(&mHeadMutex);
            return result;
        }
        cur = cur->mNext;
    }
    pthread_mutex_unlock(&mHeadMutex);
    return NULL;
}

void MDnsSdListener::Monitor::startMonitoring(int id) {
    if (VDBG) ALOGD("startMonitoring %d", id);
    pthread_mutex_lock(&mHeadMutex);
    Element *cur = mHead;
    while (cur != NULL) {
        if (cur->mId == id) {
            if (DBG_RESCAN) ALOGD("marking %p as ready to be added", cur);
            mLiveCount++;
            cur->mReady = 1;
            pthread_mutex_unlock(&mHeadMutex);
            write(mCtrlSocketPair[1], RESCAN, 1);  // trigger a rescan for a fresh poll
            if (VDBG) ALOGD("triggering rescan");
            return;
        }
        cur = cur->mNext;
    }
    pthread_mutex_unlock(&mHeadMutex);
}

void MDnsSdListener::Monitor::freeServiceRef(int id) {
    if (VDBG) ALOGD("freeServiceRef %d", id);
    pthread_mutex_lock(&mHeadMutex);
    Element **prevPtr = &mHead;
    Element *cur;
    while (*prevPtr != NULL) {
        cur = *prevPtr;
        if (cur->mId == id) {
            if (DBG_RESCAN) ALOGD("marking %p as ready to be removed", cur);
            mLiveCount--;
            if (cur->mReady == 1) {
                cur->mReady = -1; // tell poll thread to delete
                write(mCtrlSocketPair[1], RESCAN, 1); // trigger a rescan for a fresh poll
                if (VDBG) ALOGD("triggering rescan");
            } else {
                *prevPtr = cur->mNext;
                delete cur;
            }
            pthread_mutex_unlock(&mHeadMutex);
            return;
        }
        prevPtr = &(cur->mNext);
    }
    pthread_mutex_unlock(&mHeadMutex);
}
