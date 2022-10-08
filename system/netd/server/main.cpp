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
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <dirent.h>

#define LOG_TAG "Netd"

#include "cutils/log.h"

#include "CommandListener.h"
#include "NetlinkManager.h"
#include "DnsProxyListener.h"
#include "MDnsSdListener.h"
#include "FwmarkServer.h"
#include <cutils/properties.h>

static void blockSigpipe();
static void remove_pid_file();
static bool write_pid_file();

const char* const PID_FILE_PATH = "/data/misc/net/netd_pid";
const int PID_FILE_FLAGS = O_CREAT | O_TRUNC | O_WRONLY | O_NOFOLLOW | O_CLOEXEC;
const mode_t PID_FILE_MODE = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;  // mode 0644, rw-r--r--

int main() {

    CommandListener *cl;
    NetlinkManager *nm;
    DnsProxyListener *dpl;
    MDnsSdListener *mdnsl;
    FwmarkServer* fwmarkServer;

    ALOGI("Netd 1.0 starting");
    remove_pid_file();

    blockSigpipe();
    property_set("svc.binderloop", "ready");

    if (!(nm = NetlinkManager::Instance())) {
        ALOGE("Unable to create NetlinkManager");
        exit(1);
    };

    cl = new CommandListener();
    nm->setBroadcaster((SocketListener *) cl);

    if (nm->start()) {
        ALOGE("Unable to start NetlinkManager (%s)", strerror(errno));
        exit(1);
    }

    // Set local DNS mode, to prevent bionic from proxying
    // back to this service, recursively.
    setenv("ANDROID_DNS_MODE", "local", 1);
    dpl = new DnsProxyListener(CommandListener::sNetCtrl);
    if (dpl->startListener()) {
        ALOGE("Unable to start DnsProxyListener (%s)", strerror(errno));
        exit(1);
    }

    mdnsl = new MDnsSdListener();
    if (mdnsl->startListener()) {
        ALOGE("Unable to start MDnsSdListener (%s)", strerror(errno));
        exit(1);
    }

    fwmarkServer = new FwmarkServer(CommandListener::sNetCtrl);
    if (fwmarkServer->startListener()) {
        ALOGE("Unable to start FwmarkServer (%s)", strerror(errno));
        exit(1);
    }

    /*
     * Now that we're up, we can respond to commands
     */
    if (cl->startListener()) {
        ALOGE("Unable to start CommandListener (%s)", strerror(errno));
        exit(1);
    }

    bool wrote_pid = write_pid_file();

    while(1) {
        sleep(30); // 30 sec
        if (!wrote_pid) {
            wrote_pid = write_pid_file();
        }
    }

    ALOGI("Netd exiting");
    remove_pid_file();
    exit(0);
}

static bool write_pid_file() {
    char pid_buf[20];  // current pid_max is 32768, so plenty of room
    snprintf(pid_buf, sizeof(pid_buf), "%ld\n", (long)getpid());

    int fd = open(PID_FILE_PATH, PID_FILE_FLAGS, PID_FILE_MODE);
    if (fd == -1) {
        ALOGE("Unable to create pid file (%s)", strerror(errno));
        return false;
    }

    // File creation is affected by umask, so make sure the right mode bits are set.
    if (fchmod(fd, PID_FILE_MODE) == -1) {
        ALOGE("failed to set mode 0%o on %s (%s)", PID_FILE_MODE, PID_FILE_PATH, strerror(errno));
        close(fd);
        remove_pid_file();
        return false;
    }

    if (write(fd, pid_buf, strlen(pid_buf)) != (ssize_t)strlen(pid_buf)) {
        ALOGE("Unable to write to pid file (%s)", strerror(errno));
        close(fd);
        remove_pid_file();
        return false;
    }
    close(fd);
    return true;
}

static void remove_pid_file() {
    unlink(PID_FILE_PATH);
}

static void blockSigpipe()
{
    sigset_t mask;

    sigemptyset(&mask);
    sigaddset(&mask, SIGPIPE);
    if (sigprocmask(SIG_BLOCK, &mask, NULL) != 0)
        ALOGW("WARNING: SIGPIPE not blocked\n");
}
