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

#define LOG_TAG "Sensors"

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>

#include <cutils/log.h>

#include "SensorBase.h"
#include "iio/events.h"

#define IIO_MAX_NAME_LENGTH 30

const char *iio_dir = "/sys/bus/iio/devices/";

/*****************************************************************************/

SensorBase::SensorBase(
        const char* dev_name,
        const char* data_name)
    : mDevName(dev_name), mDataName(data_name),
      mDevFd(-1), mDataFd(-1)
{
    ALOGV("%s(): dev_name=%s", __func__, dev_name);
    if (mDataName) {
        mDataFd = openInput(mDataName);
    }
}

SensorBase::~SensorBase() {
    if (mDataFd >= 0) {
        close(mDataFd);
    }
    if (mDevFd >= 0) {
        close(mDevFd);
    }
}

int SensorBase::openDevice() {
    if ((mDevFd < 0) && mDevName) {
        mDevFd = open(mDevName, O_RDONLY);
        ALOGE_IF(mDevFd < 0, "Couldn't open %s (%s)", mDevName, strerror(errno));
    }
    return 0;
}

int SensorBase::closeDevice() {
    if (mDevFd >= 0) {
        close(mDevFd);
        mDevFd = -1;
    }
    return 0;
}

int SensorBase::getFd() const {
    if (!mDataName) {
        return mDevFd;
    }
    return mDataFd;
}

int SensorBase::setDelay(int32_t handle, int64_t ns) {
    return 0;
}

bool SensorBase::hasPendingEvents() const {
    return false;
}

int64_t SensorBase::getTimestamp() {
    struct timespec t;
    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return int64_t(t.tv_sec)*1000000000LL + t.tv_nsec;
}

/*
 * find_type_by_name() - function to match top level types by name
 * @name: top level type instance name
 * @type: the type of top level instance being sort
 *
 * Typical types this is used for are device and trigger.
 *
 * NOTE: This function is copied from drivers/staging/iio/Documentation/iio_utils.h
 * and modified.
 */
int SensorBase::findTypeByName(const char *name, const char *type)
{
    const struct dirent *ent;
    int iio_id;
    int ret = -ENODEV;

    FILE *nameFile;
    DIR *dp;
    char thisname[IIO_MAX_NAME_LENGTH];
    char filename[PATH_MAX];

    dp = opendir(iio_dir);
    if (dp == NULL) {
        ALOGE("No industrialio devices available");
        return ret;
    }

    while (ent = readdir(dp), ent != NULL) {
        if (strcmp(ent->d_name, ".") != 0 &&
            strcmp(ent->d_name, "..") != 0 &&
            strlen(ent->d_name) > strlen(type) &&
            strncmp(ent->d_name, type, strlen(type)) == 0) {
            if (sscanf(ent->d_name + strlen(type), "%d", &iio_id) != 1)
                continue;

            sprintf(filename, "%s%s%d/name", iio_dir, type, iio_id);
            nameFile = fopen(filename, "r");
            if (!nameFile)
                continue;

            if (fscanf(nameFile, "%s", thisname) == 1) {
                if (strcmp(name, thisname) == 0) {
                    fclose(nameFile);
                    ret = iio_id;
                    break;
                }
            }
            fclose(nameFile);
        }
    }
    closedir(dp);
    return ret;
}

int SensorBase::openInput(const char* inputName) {
    int event_fd = -1;
    char devname[PATH_MAX];
    int dev_num;

    dev_num =  findTypeByName(inputName, "iio:device");
    if (dev_num >= 0) {
        int fd;
        sprintf(devname, "/dev/iio:device%d", dev_num);
        fd = open(devname, O_RDONLY);
        if (fd >= 0) {
            if (ioctl(fd, IIO_GET_EVENT_FD_IOCTL, &event_fd) >= 0)
                strcpy(mInputName, devname + 5);
            else
                ALOGE("couldn't get a event fd from %s", devname);
            close(fd); /* close /dev/iio:device* */
        } else {
            ALOGE("couldn't open %s (%s)", devname, strerror(errno));
        }
    } else {
       ALOGE("couldn't find the device %s", inputName);
    }

    return event_fd;
}
