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
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <pthread.h>
#include <resolv_netid.h>
#include <net/if.h>

#define LOG_TAG "DnsProxyListener"
#define DBG 0
#define VDBG 0

#include <cutils/log.h>
#include <sysutils/SocketClient.h>

#include "Fwmark.h"
#include "DnsProxyListener.h"
#include "NetdConstants.h"
#include "NetworkController.h"
#include "ResponseCode.h"

DnsProxyListener::DnsProxyListener(const NetworkController* netCtrl) :
        FrameworkListener("dnsproxyd"), mNetCtrl(netCtrl) {
    registerCmd(new GetAddrInfoCmd(this));
    registerCmd(new GetHostByAddrCmd(this));
    registerCmd(new GetHostByNameCmd(this));
}

DnsProxyListener::GetAddrInfoHandler::GetAddrInfoHandler(SocketClient *c,
                                                         char* host,
                                                         char* service,
                                                         struct addrinfo* hints,
                                                         unsigned netId,
                                                         uint32_t mark)
        : mClient(c),
          mHost(host),
          mService(service),
          mHints(hints),
          mNetId(netId),
          mMark(mark) {
}

DnsProxyListener::GetAddrInfoHandler::~GetAddrInfoHandler() {
    free(mHost);
    free(mService);
    free(mHints);
}

void DnsProxyListener::GetAddrInfoHandler::start() {
    pthread_t thread;
    pthread_create(&thread, NULL,
                   DnsProxyListener::GetAddrInfoHandler::threadStart, this);
    pthread_detach(thread);
}

void* DnsProxyListener::GetAddrInfoHandler::threadStart(void* obj) {
    GetAddrInfoHandler* handler = reinterpret_cast<GetAddrInfoHandler*>(obj);
    handler->run();
    delete handler;
    pthread_exit(NULL);
    return NULL;
}

// Sends 4 bytes of big-endian length, followed by the data.
// Returns true on success.
static bool sendLenAndData(SocketClient *c, const int len, const void* data) {
    uint32_t len_be = htonl(len);
    return c->sendData(&len_be, 4) == 0 &&
        (len == 0 || c->sendData(data, len) == 0);
}

// Returns true on success
static bool sendhostent(SocketClient *c, struct hostent *hp) {
    bool success = true;
    int i;
    if (hp->h_name != NULL) {
        success &= sendLenAndData(c, strlen(hp->h_name)+1, hp->h_name);
    } else {
        success &= sendLenAndData(c, 0, "") == 0;
    }

    for (i=0; hp->h_aliases[i] != NULL; i++) {
        success &= sendLenAndData(c, strlen(hp->h_aliases[i])+1, hp->h_aliases[i]);
    }
    success &= sendLenAndData(c, 0, ""); // null to indicate we're done

    uint32_t buf = htonl(hp->h_addrtype);
    success &= c->sendData(&buf, sizeof(buf)) == 0;

    buf = htonl(hp->h_length);
    success &= c->sendData(&buf, sizeof(buf)) == 0;

    for (i=0; hp->h_addr_list[i] != NULL; i++) {
        success &= sendLenAndData(c, 16, hp->h_addr_list[i]);
    }
    success &= sendLenAndData(c, 0, ""); // null to indicate we're done
    return success;
}

void DnsProxyListener::GetAddrInfoHandler::run() {
    if (DBG) {
        ALOGD("GetAddrInfoHandler, now for %s / %s / %u / %u", mHost, mService, mNetId, mMark);
    }

    struct addrinfo* result = NULL;
    uint32_t rv = android_getaddrinfofornet(mHost, mService, mHints, mNetId, mMark, &result);
    if (rv) {
        // getaddrinfo failed
        mClient->sendBinaryMsg(ResponseCode::DnsProxyOperationFailed, &rv, sizeof(rv));
    } else {
        bool success = !mClient->sendCode(ResponseCode::DnsProxyQueryResult);
        struct addrinfo* ai = result;
        while (ai && success) {
            success = sendLenAndData(mClient, sizeof(struct addrinfo), ai)
                && sendLenAndData(mClient, ai->ai_addrlen, ai->ai_addr)
                && sendLenAndData(mClient,
                                  ai->ai_canonname ? strlen(ai->ai_canonname) + 1 : 0,
                                  ai->ai_canonname);
            ai = ai->ai_next;
        }
        success = success && sendLenAndData(mClient, 0, "");
        if (!success) {
            ALOGW("Error writing DNS result to client");
        }
    }
    if (result) {
        freeaddrinfo(result);
    }
    mClient->decRef();
}

DnsProxyListener::GetAddrInfoCmd::GetAddrInfoCmd(const DnsProxyListener* dnsProxyListener) :
    NetdCommand("getaddrinfo"),
    mDnsProxyListener(dnsProxyListener) {
}

int DnsProxyListener::GetAddrInfoCmd::runCommand(SocketClient *cli,
                                            int argc, char **argv) {
    if (DBG) {
        for (int i = 0; i < argc; i++) {
            ALOGD("argv[%i]=%s", i, argv[i]);
        }
    }
    if (argc != 8) {
        char* msg = NULL;
        asprintf( &msg, "Invalid number of arguments to getaddrinfo: %i", argc);
        ALOGW("%s", msg);
        cli->sendMsg(ResponseCode::CommandParameterError, msg, false);
        free(msg);
        return -1;
    }

    char* name = argv[1];
    if (strcmp("^", name) == 0) {
        name = NULL;
    } else {
        name = strdup(name);
    }

    char* service = argv[2];
    if (strcmp("^", service) == 0) {
        service = NULL;
    } else {
        service = strdup(service);
    }

    struct addrinfo* hints = NULL;
    int ai_flags = atoi(argv[3]);
    int ai_family = atoi(argv[4]);
    int ai_socktype = atoi(argv[5]);
    int ai_protocol = atoi(argv[6]);
    unsigned netId = strtoul(argv[7], NULL, 10);
    uid_t uid = cli->getUid();

    uint32_t mark = mDnsProxyListener->mNetCtrl->getNetworkForDns(&netId, uid);

    if (ai_flags != -1 || ai_family != -1 ||
        ai_socktype != -1 || ai_protocol != -1) {
        hints = (struct addrinfo*) calloc(1, sizeof(struct addrinfo));
        hints->ai_flags = ai_flags;
        hints->ai_family = ai_family;
        hints->ai_socktype = ai_socktype;
        hints->ai_protocol = ai_protocol;

        // Only implement AI_ADDRCONFIG if application is using default network since our
        // implementation only works on the default network.
        if ((hints->ai_flags & AI_ADDRCONFIG) &&
                netId != mDnsProxyListener->mNetCtrl->getDefaultNetwork()) {
            hints->ai_flags &= ~AI_ADDRCONFIG;
        }
    }

    if (DBG) {
        ALOGD("GetAddrInfoHandler for %s / %s / %u / %d / %u",
             name ? name : "[nullhost]",
             service ? service : "[nullservice]",
             netId, uid, mark);
    }

    cli->incRef();
    DnsProxyListener::GetAddrInfoHandler* handler =
            new DnsProxyListener::GetAddrInfoHandler(cli, name, service, hints, netId, mark);
    handler->start();

    return 0;
}

/*******************************************************
 *                  GetHostByName                      *
 *******************************************************/
DnsProxyListener::GetHostByNameCmd::GetHostByNameCmd(const DnsProxyListener* dnsProxyListener) :
      NetdCommand("gethostbyname"),
      mDnsProxyListener(dnsProxyListener) {
}

int DnsProxyListener::GetHostByNameCmd::runCommand(SocketClient *cli,
                                            int argc, char **argv) {
    if (DBG) {
        for (int i = 0; i < argc; i++) {
            ALOGD("argv[%i]=%s", i, argv[i]);
        }
    }
    if (argc != 4) {
        char* msg = NULL;
        asprintf(&msg, "Invalid number of arguments to gethostbyname: %i", argc);
        ALOGW("%s", msg);
        cli->sendMsg(ResponseCode::CommandParameterError, msg, false);
        free(msg);
        return -1;
    }

    uid_t uid = cli->getUid();
    unsigned netId = strtoul(argv[1], NULL, 10);
    char* name = argv[2];
    int af = atoi(argv[3]);

    if (strcmp(name, "^") == 0) {
        name = NULL;
    } else {
        name = strdup(name);
    }

    uint32_t mark = mDnsProxyListener->mNetCtrl->getNetworkForDns(&netId, uid);

    cli->incRef();
    DnsProxyListener::GetHostByNameHandler* handler =
            new DnsProxyListener::GetHostByNameHandler(cli, name, af, netId, mark);
    handler->start();

    return 0;
}

DnsProxyListener::GetHostByNameHandler::GetHostByNameHandler(SocketClient* c,
                                                             char* name,
                                                             int af,
                                                             unsigned netId,
                                                             uint32_t mark)
        : mClient(c),
          mName(name),
          mAf(af),
          mNetId(netId),
          mMark(mark) {
}

DnsProxyListener::GetHostByNameHandler::~GetHostByNameHandler() {
    free(mName);
}

void DnsProxyListener::GetHostByNameHandler::start() {
    pthread_t thread;
    pthread_create(&thread, NULL,
            DnsProxyListener::GetHostByNameHandler::threadStart, this);
    pthread_detach(thread);
}

void* DnsProxyListener::GetHostByNameHandler::threadStart(void* obj) {
    GetHostByNameHandler* handler = reinterpret_cast<GetHostByNameHandler*>(obj);
    handler->run();
    delete handler;
    pthread_exit(NULL);
    return NULL;
}

void DnsProxyListener::GetHostByNameHandler::run() {
    if (DBG) {
        ALOGD("DnsProxyListener::GetHostByNameHandler::run\n");
    }

    struct hostent* hp;

    hp = android_gethostbynamefornet(mName, mAf, mNetId, mMark);

    if (DBG) {
        ALOGD("GetHostByNameHandler::run gethostbyname errno: %s hp->h_name = %s, name_len = %zu\n",
                hp ? "success" : strerror(errno),
                (hp && hp->h_name) ? hp->h_name : "null",
                (hp && hp->h_name) ? strlen(hp->h_name) + 1 : 0);
    }

    bool success = true;
    if (hp) {
        success = mClient->sendCode(ResponseCode::DnsProxyQueryResult) == 0;
        success &= sendhostent(mClient, hp);
    } else {
        success = mClient->sendBinaryMsg(ResponseCode::DnsProxyOperationFailed, NULL, 0) == 0;
    }

    if (!success) {
        ALOGW("GetHostByNameHandler: Error writing DNS result to client\n");
    }
    mClient->decRef();
}


/*******************************************************
 *                  GetHostByAddr                      *
 *******************************************************/
DnsProxyListener::GetHostByAddrCmd::GetHostByAddrCmd(const DnsProxyListener* dnsProxyListener) :
        NetdCommand("gethostbyaddr"),
        mDnsProxyListener(dnsProxyListener) {
}

int DnsProxyListener::GetHostByAddrCmd::runCommand(SocketClient *cli,
                                            int argc, char **argv) {
    if (DBG) {
        for (int i = 0; i < argc; i++) {
            ALOGD("argv[%i]=%s", i, argv[i]);
        }
    }
    if (argc != 5) {
        char* msg = NULL;
        asprintf(&msg, "Invalid number of arguments to gethostbyaddr: %i", argc);
        ALOGW("%s", msg);
        cli->sendMsg(ResponseCode::CommandParameterError, msg, false);
        free(msg);
        return -1;
    }

    char* addrStr = argv[1];
    int addrLen = atoi(argv[2]);
    int addrFamily = atoi(argv[3]);
    uid_t uid = cli->getUid();
    unsigned netId = strtoul(argv[4], NULL, 10);

    void* addr = malloc(sizeof(struct in6_addr));
    errno = 0;
    int result = inet_pton(addrFamily, addrStr, addr);
    if (result <= 0) {
        char* msg = NULL;
        asprintf(&msg, "inet_pton(\"%s\") failed %s", addrStr, strerror(errno));
        ALOGW("%s", msg);
        cli->sendMsg(ResponseCode::OperationFailed, msg, false);
        free(addr);
        free(msg);
        return -1;
    }

    uint32_t mark = mDnsProxyListener->mNetCtrl->getNetworkForDns(&netId, uid);

    cli->incRef();
    DnsProxyListener::GetHostByAddrHandler* handler =
            new DnsProxyListener::GetHostByAddrHandler(cli, addr, addrLen, addrFamily, netId, mark);
    handler->start();

    return 0;
}

DnsProxyListener::GetHostByAddrHandler::GetHostByAddrHandler(SocketClient* c,
                                                             void* address,
                                                             int   addressLen,
                                                             int   addressFamily,
                                                             unsigned netId,
                                                             uint32_t mark)
        : mClient(c),
          mAddress(address),
          mAddressLen(addressLen),
          mAddressFamily(addressFamily),
          mNetId(netId),
          mMark(mark) {
}

DnsProxyListener::GetHostByAddrHandler::~GetHostByAddrHandler() {
    free(mAddress);
}

void DnsProxyListener::GetHostByAddrHandler::start() {
    pthread_t thread;
    pthread_create(&thread, NULL,
                   DnsProxyListener::GetHostByAddrHandler::threadStart, this);
    pthread_detach(thread);
}

void* DnsProxyListener::GetHostByAddrHandler::threadStart(void* obj) {
    GetHostByAddrHandler* handler = reinterpret_cast<GetHostByAddrHandler*>(obj);
    handler->run();
    delete handler;
    pthread_exit(NULL);
    return NULL;
}

void DnsProxyListener::GetHostByAddrHandler::run() {
    if (DBG) {
        ALOGD("DnsProxyListener::GetHostByAddrHandler::run\n");
    }
    struct hostent* hp;

    // NOTE gethostbyaddr should take a void* but bionic thinks it should be char*
    hp = android_gethostbyaddrfornet((char*)mAddress, mAddressLen, mAddressFamily, mNetId, mMark);

    if (DBG) {
        ALOGD("GetHostByAddrHandler::run gethostbyaddr errno: %s hp->h_name = %s, name_len = %zu\n",
                hp ? "success" : strerror(errno),
                (hp && hp->h_name) ? hp->h_name : "null",
                (hp && hp->h_name) ? strlen(hp->h_name) + 1 : 0);
    }

    bool success = true;
    if (hp) {
        success = mClient->sendCode(ResponseCode::DnsProxyQueryResult) == 0;
        success &= sendhostent(mClient, hp);
    } else {
        success = mClient->sendBinaryMsg(ResponseCode::DnsProxyOperationFailed, NULL, 0) == 0;
    }

    if (!success) {
        ALOGW("GetHostByAddrHandler: Error writing DNS result to client\n");
    }
    mClient->decRef();
}
