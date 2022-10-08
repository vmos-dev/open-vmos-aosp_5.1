/*
* Copyright (C) 2012 Invensense, Inc.
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

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <dirent.h>
#include <sys/select.h>
#include <cutils/log.h>
#include <linux/input.h>

#include "SensorBase.h"
#include "local_log_def.h"

/*****************************************************************************/

SensorBase::SensorBase(const char* dev_name,
                       const char* data_name) : dev_name(dev_name),
                                                data_name(data_name),
                                                dev_fd(-1),
                                                data_fd(-1)
{
    if (data_name) {
        data_fd = openInput(data_name);
    }
}

SensorBase::~SensorBase()
{
    if (data_fd >= 0) {
        close(data_fd);
    }
    if (dev_fd >= 0) {
        close(dev_fd);
    }
}

int SensorBase::open_device()
{
    if (dev_fd<0 && dev_name) {
        dev_fd = open(dev_name, O_RDONLY);
        LOGE_IF(dev_fd<0, "Couldn't open %s (%s)", dev_name, strerror(errno));
    }
    return 0;
}

int SensorBase::close_device()
{
    if (dev_fd >= 0) {
        close(dev_fd);
        dev_fd = -1;
    }
    return 0;
}

int SensorBase::getFd() const
{
    if (!data_name) {
        return dev_fd;
    }
    return data_fd;
}

int SensorBase::setDelay(int32_t /*handle*/, int64_t /*ns*/)
{
    return 0;
}

bool SensorBase::hasPendingEvents() const
{
    return false;
}

int SensorBase::openInput(const char *inputName)
{
    int fd = -1;
    const char *dirname = "/dev/input";
    char devname[PATH_MAX];
    char *filename;
    DIR *dir;
    struct dirent *de;
    dir = opendir(dirname);
    if(dir == NULL)
        return -1;
    strcpy(devname, dirname);
    filename = devname + strlen(devname);
    *filename++ = '/';
    while((de = readdir(dir))) {
        if(de->d_name[0] == '.' &&
                (de->d_name[1] == '\0' ||
                        (de->d_name[1] == '.' && de->d_name[2] == '\0')))
            continue;
        strcpy(filename, de->d_name);
        fd = open(devname, O_RDONLY);
        LOGV_IF(EXTRA_VERBOSE, "path open %s", devname);
        LOGI("path open %s", devname);
        if (fd >= 0) {
            char name[80];
            if (ioctl(fd, EVIOCGNAME(sizeof(name) - 1), &name) < 1) {
                name[0] = '\0';
            }
            LOGV_IF(EXTRA_VERBOSE, "name read %s", name);
            if (!strcmp(name, inputName)) {
                strcpy(input_name, filename);
                break;
            } else {
                close(fd);
                fd = -1;
            }
        }
    }
    closedir(dir);
    LOGE_IF(fd < 0, "couldn't find '%s' input device", inputName);
    return fd;
}

int SensorBase::enable(int32_t /*handle*/, int /*enabled*/)
{
    return 0;
}
