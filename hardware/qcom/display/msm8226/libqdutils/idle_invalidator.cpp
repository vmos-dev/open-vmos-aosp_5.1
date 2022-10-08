/*
 * Copyright (c) 2012, The Linux Foundation. All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of The Linux Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "idle_invalidator.h"
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <fcntl.h>

#define II_DEBUG 0
#define IDLE_NOTIFY_PATH "/sys/devices/virtual/graphics/fb0/idle_notify"
#define IDLE_TIME_PATH "/sys/devices/virtual/graphics/fb0/idle_time"


static const char *threadName = "IdleInvalidator";
InvalidatorHandler IdleInvalidator::mHandler = NULL;
android::sp<IdleInvalidator> IdleInvalidator::sInstance(0);

IdleInvalidator::IdleInvalidator(): Thread(false), mHwcContext(0),
    mTimeoutEventFd(-1) {
    ALOGD_IF(II_DEBUG, "IdleInvalidator::%s", __FUNCTION__);
}

int IdleInvalidator::init(InvalidatorHandler reg_handler, void* user_data,
                         unsigned int idleSleepTime) {
    ALOGD_IF(II_DEBUG, "IdleInvalidator::%s idleSleepTime %d",
        __FUNCTION__, idleSleepTime);
    mHandler = reg_handler;
    mHwcContext = user_data;

    // Open a sysfs node to receive the timeout notification from driver.
    mTimeoutEventFd = open(IDLE_NOTIFY_PATH, O_RDONLY);
    if (mTimeoutEventFd < 0) {
        ALOGE ("%s:not able to open %s node %s",
                __FUNCTION__, IDLE_NOTIFY_PATH, strerror(errno));
        return -1;
    }

    // Open a sysfs node to send the timeout value to driver.
    int fd = open(IDLE_TIME_PATH, O_WRONLY);
    if (fd < 0) {
        ALOGE ("%s:not able to open %s node %s",
                __FUNCTION__, IDLE_TIME_PATH, strerror(errno));
        close(mTimeoutEventFd);
        mTimeoutEventFd = -1;
        return -1;
    }
    char strSleepTime[64];
    snprintf(strSleepTime, sizeof(strSleepTime), "%d", idleSleepTime);
    // Notify driver about the timeout value
    ssize_t len = pwrite(fd, strSleepTime, strlen(strSleepTime), 0);
    if(len < -1) {
        ALOGE ("%s:not able to write into %s node %s",
                __FUNCTION__, IDLE_TIME_PATH, strerror(errno));
        close(mTimeoutEventFd);
        mTimeoutEventFd = -1;
        close(fd);
        return -1;
    }
    close(fd);

    //Triggers the threadLoop to run, if not already running.
    run(threadName, android::PRIORITY_LOWEST);
    return 0;
}

bool IdleInvalidator::threadLoop() {
    ALOGD_IF(II_DEBUG, "IdleInvalidator::%s", __FUNCTION__);
    struct pollfd pFd;
    pFd.fd = mTimeoutEventFd;
    if (pFd.fd >= 0)
        pFd.events = POLLPRI | POLLERR;
    // Poll for an timeout event from driver
    int err = poll(&pFd, 1, -1);
    if(err > 0) {
        if (pFd.revents & POLLPRI) {
            char data[64];
            // Consume the node by reading it
            ssize_t len = pread(pFd.fd, data, 64, 0);
            ALOGD_IF(II_DEBUG, "IdleInvalidator::%s Idle Timeout fired len %zd",
                __FUNCTION__, len);
            mHandler((void*)mHwcContext);
        }
    }
    return true;
}

int IdleInvalidator::readyToRun() {
    ALOGD_IF(II_DEBUG, "IdleInvalidator::%s", __FUNCTION__);
    return 0; /*NO_ERROR*/
}

void IdleInvalidator::onFirstRef() {
    ALOGD_IF(II_DEBUG, "IdleInvalidator::%s", __FUNCTION__);
}

IdleInvalidator *IdleInvalidator::getInstance() {
    ALOGD_IF(II_DEBUG, "IdleInvalidator::%s", __FUNCTION__);
    if(sInstance.get() == NULL)
        sInstance = new IdleInvalidator();
    return sInstance.get();
}
