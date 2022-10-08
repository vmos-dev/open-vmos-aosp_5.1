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

#ifndef ANDROID_SENSOR_BASE_H
#define ANDROID_SENSOR_BASE_H

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>

/*****************************************************************************/
struct sensors_event_t;

class SensorBase {
protected:
    const char* mDevName;
    const char* mDataName;
    char        mInputName[PATH_MAX];
    int         mDevFd;
    int         mDataFd;

    int findTypeByName(const char *name, const char *type);
    int openInput(const char* inputName);
    static int64_t getTimestamp();
    int openDevice();
    int closeDevice();

public:
    SensorBase(const char* dev_name,
               const char* data_name);

    virtual ~SensorBase();

    virtual int readEvents(sensors_event_t* data, int count) = 0;
    virtual bool hasPendingEvents() const;
    virtual int getFd() const;
    virtual int setDelay(int32_t handle, int64_t ns);
    virtual int enable(int32_t handle, int enabled) = 0;
};

/*****************************************************************************/

#endif  // ANDROID_SENSOR_BASE_H
