/*
// Copyright (c) 2014 Intel Corporation 
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/queue.h>
#include <linux/netlink.h>
#include <sys/types.h>
#include <unistd.h>
#include <DrmConfig.h>
#include <common/utils/HwcTrace.h>
#include <UeventObserver.h>

namespace android {
namespace intel {

UeventObserver::UeventObserver()
    : mUeventFd(-1),
      mExitRDFd(-1),
      mExitWDFd(-1),
      mListeners()
{
}

UeventObserver::~UeventObserver()
{
    deinitialize();
}

bool UeventObserver::initialize()
{
    mListeners.clear();

    if (mUeventFd != -1) {
        return true;
    }

    mThread = new UeventObserverThread(this);
    if (!mThread.get()) {
        ELOGTRACE("failed to create uevent observer thread");
        return false;
    }

    // init uevent socket
    struct sockaddr_nl addr;
    // set the socket receive buffer to 64K
    // NOTE: this is only called for once
    int sz = 64 * 1024;

    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid =  pthread_self() | getpid();
    addr.nl_groups = 0xffffffff;

    mUeventFd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if (mUeventFd < 0) {
        DEINIT_AND_RETURN_FALSE("failed to create uevent socket");
    }

    if (setsockopt(mUeventFd, SOL_SOCKET, SO_RCVBUFFORCE, &sz, sizeof(sz))) {
        WLOGTRACE("setsockopt() failed");
        //return false;
    }

    if (bind(mUeventFd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        DEINIT_AND_RETURN_FALSE("failed to bind scoket");
        return false;
    }

    memset(mUeventMessage, 0, UEVENT_MSG_LEN);

    int exitFds[2];
    if (pipe(exitFds) < 0) {
        ELOGTRACE("failed to make pipe");
        deinitialize();
        return false;
    }
    mExitRDFd = exitFds[0];
    mExitWDFd = exitFds[1];

    return true;
}

void UeventObserver::deinitialize()
{
    if (mUeventFd != -1) {
        if (mExitWDFd != -1) {
            close(mExitWDFd);
            mExitWDFd = -1;
        }
        close(mUeventFd);
        mUeventFd = -1;
    }

    if (mThread.get()) {
        mThread->requestExitAndWait();
        mThread = NULL;
    }

    while (!mListeners.isEmpty()) {
        UeventListener *listener = mListeners.valueAt(0);
        mListeners.removeItemsAt(0);
        delete listener;
    }
}

void UeventObserver::start()
{
    if (mThread.get()) {
        mThread->run("UeventObserver", PRIORITY_URGENT_DISPLAY);
    }
}


void UeventObserver::registerListener(const char *event, UeventListenerFunc func, void *data)
{
    if (!event || !func) {
        ELOGTRACE("invalid event string or listener to register");
        return;
    }

    String8 key(event);
    if (mListeners.indexOfKey(key) >= 0) {
        ELOGTRACE("listener for uevent %s exists", event);
        return;
    }

    UeventListener *listener = new UeventListener;
    if (!listener) {
        ELOGTRACE("failed to create Uevent Listener");
        return;
    }
    listener->func = func;
    listener->data = data;

    mListeners.add(key, listener);
}

bool UeventObserver::threadLoop()
{
    if (mUeventFd == -1) {
        ELOGTRACE("invalid uEvent file descriptor");
        return false;
    }

    struct pollfd fds[2];
    int nr;

    fds[0].fd = mUeventFd;
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    fds[1].fd = mExitRDFd;
    fds[1].events = POLLIN;
    fds[1].revents = 0;
    nr = poll(fds, 2, -1);

    if (nr > 0 && fds[0].revents == POLLIN) {
        int count = recv(mUeventFd, mUeventMessage, UEVENT_MSG_LEN - 2, 0);
        if (count > 0) {
            onUevent();
        }
    } else if (fds[1].revents) {
        close(mExitRDFd);
        mExitRDFd = -1;
        ILOGTRACE("exiting wait");
        return false;
    }
    // always looping
    return true;
}

void UeventObserver::onUevent()
{
    char *msg = mUeventMessage;
    const char *envelope = DrmConfig::getUeventEnvelope();
    if (strncmp(msg, envelope, strlen(envelope)) != 0)
        return;

    msg += strlen(msg) + 1;

    UeventListener *listener;
    String8 key;
    while (*msg) {
        key = String8(msg);
        if (mListeners.indexOfKey(key) >= 0) {
            DLOGTRACE("received Uevent: %s", msg);
            listener = mListeners.valueFor(key);
            if (listener) {
                listener->func(listener->data);
            } else {
                ELOGTRACE("no listener for uevent %s", msg);
            }
        }
        msg += strlen(msg) + 1;
    }
}

} // namespace intel
} // namespace android

