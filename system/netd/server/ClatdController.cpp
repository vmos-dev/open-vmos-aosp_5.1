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
#include <map>
#include <string>

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#define LOG_TAG "ClatdController"
#include <cutils/log.h>

#include <resolv_netid.h>

#include "NetdConstants.h"
#include "ClatdController.h"
#include "Fwmark.h"
#include "NetdConstants.h"
#include "NetworkController.h"

static const char* kClatdPath = "/system/bin/clatd";

ClatdController::ClatdController(NetworkController* controller)
        : mNetCtrl(controller) {
}

ClatdController::~ClatdController() {
}

// Returns the PID of the clatd running on interface |interface|, or 0 if clatd is not running on
// |interface|.
pid_t ClatdController::getClatdPid(char* interface) {
    auto it = mClatdPids.find(interface);
    return (it == mClatdPids.end() ? 0 : it->second);
}

int ClatdController::startClatd(char* interface) {
    pid_t pid = getClatdPid(interface);

    if (pid != 0) {
        ALOGE("clatd pid=%d already started on %s", pid, interface);
        errno = EBUSY;
        return -1;
    }

    // Pass in the interface, a netid to use for DNS lookups, and a fwmark for outgoing packets.
    unsigned netId = mNetCtrl->getNetworkForInterface(interface);
    if (netId == NETID_UNSET) {
        ALOGE("interface %s not assigned to any netId", interface);
        errno = ENODEV;
        return -1;
    }

    char netIdString[UINT32_STRLEN];
    snprintf(netIdString, sizeof(netIdString), "%u", netId);

    Fwmark fwmark;
    fwmark.netId = netId;
    fwmark.explicitlySelected = true;
    fwmark.protectedFromVpn = true;
    fwmark.permission = PERMISSION_SYSTEM;

    char fwmarkString[UINT32_HEX_STRLEN];
    snprintf(fwmarkString, sizeof(fwmarkString), "0x%x", fwmark.intValue);

    ALOGD("starting clatd on %s", interface);

    std::string progname("clatd-");
    progname += interface;

    if ((pid = fork()) < 0) {
        ALOGE("fork failed (%s)", strerror(errno));
        return -1;
    }

    if (!pid) {
        char *args[] = {
            (char *) progname.c_str(),
            (char *) "-i",
            interface,
            (char *) "-n",
            netIdString,
            (char *) "-m",
            fwmarkString,
            NULL
        };

        if (execv(kClatdPath, args)) {
            ALOGE("execv failed (%s)", strerror(errno));
            _exit(1);
        }
        ALOGE("Should never get here!");
        _exit(1);
    } else {
        mClatdPids[interface] = pid;
        ALOGD("clatd started on %s", interface);
    }

    return 0;
}

int ClatdController::stopClatd(char* interface) {
    pid_t pid = getClatdPid(interface);

    if (pid == 0) {
        ALOGE("clatd already stopped");
        return -1;
    }

    ALOGD("Stopping clatd pid=%d on %s", pid, interface);

    kill(pid, SIGTERM);
    waitpid(pid, NULL, 0);
    mClatdPids.erase(interface);

    ALOGD("clatd on %s stopped", interface);

    return 0;
}

bool ClatdController::isClatdStarted(char* interface) {
    pid_t waitpid_status;
    pid_t pid = getClatdPid(interface);
    if (pid == 0) {
        return false;
    }
    waitpid_status = waitpid(pid, NULL, WNOHANG);
    if (waitpid_status != 0) {
        mClatdPids.erase(interface);  // child exited, don't call waitpid on it again
    }
    return waitpid_status == 0; // 0 while child is running
}
