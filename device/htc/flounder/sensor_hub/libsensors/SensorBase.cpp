/*
 * Copyright (C) 2008-2014 The Android Open Source Project
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

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <math.h>
#include <poll.h>
#include <sys/select.h>
#include <unistd.h>

#include <cutils/log.h>

#include "SensorBase.h"

/*****************************************************************************/

#undef LOG_TAG
#define LOG_TAG "CwMcuSensor"

SensorBase::SensorBase(
        const char* dev_name,
        const char* data_name)
    : dev_name(dev_name)
    , data_name(data_name)
    , dev_fd(-1)
    , data_fd(-1)
{
}

SensorBase::~SensorBase() {
    if (data_fd >= 0) {
        close(data_fd);
    }
    if (dev_fd >= 0) {
        close(dev_fd);
    }
}

int SensorBase::open_device() {
    if (dev_fd<0 && dev_name) {
        dev_fd = open(dev_name, O_RDONLY);
        ALOGE_IF(dev_fd<0, "Couldn't open %s (%s)", dev_name, strerror(errno));
    }
    return 0;
}

int SensorBase::close_device() {
    if (dev_fd >= 0) {
        close(dev_fd);
        dev_fd = -1;
    }
    return 0;
}

int SensorBase::write_sys_attribute(
        const char *path, const char *value, int bytes)
{
    int fd, amt;

    fd = open(path, O_WRONLY);
    if (fd < 0) {
        ALOGE("SensorBase: write_attr failed to open %s (%s)",
                path, strerror(errno));
        return -1;
    }

    amt = write(fd, value, bytes);
    amt = ((amt == -1) ? -errno : 0);
    ALOGE_IF(amt < 0, "SensorBase: write_int failed to write %s (%s)",
                path, strerror(errno));
    close(fd);
    return amt;
}

int SensorBase::getFd() const {
    if (!data_name) {
        return dev_fd;
    }
    return data_fd;
}

int SensorBase::setDelay(int32_t, int64_t) {
    return 0;
}

int64_t SensorBase::getDelay(int32_t) {
    return 0;
}

bool SensorBase::hasPendingEvents() const {
    return false;
}

int64_t SensorBase::getTimestamp() {
    struct timespec t;
    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(CLOCK_BOOTTIME, &t);
    return int64_t(t.tv_sec)*NS_PER_SEC + t.tv_nsec;
}

