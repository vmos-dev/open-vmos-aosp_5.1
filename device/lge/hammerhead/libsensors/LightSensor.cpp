/*
 * Copyright 2013 The Android Open Source Project
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
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>
#include <cutils/log.h>

#include "sensors.h"
#include "LightSensor.h"

#define ABS_LIGHT   0x29

LightSensor::LightSensor()
    : SensorBase(NULL, LIGHT_DATA),
      mEnabled(0),
      mEventsSinceEnable(0),
      mInputReader(4),
      mHasPendingEvent(false)
{
    mPendingEvent.sensor = ID_L;
    mPendingEvent.type = SENSOR_TYPE_LIGHT;
    memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));
}

LightSensor::~LightSensor()
{
}

int LightSensor::setInitialState()
{
    return 0;
}

int LightSensor::setDelay(int32_t handle, int64_t ns)
{
    int fd;
    char sysfs[PATH_MAX];

    strcpy(sysfs, I2C);
    strcat(sysfs, "als_poll_delay");

    fd = open(sysfs, O_RDWR);
    if (fd >= 0) {
        char buf[16] = {0,};
        snprintf(buf, sizeof(buf), "%lld", ns);
        write(fd, buf, sizeof(buf));
        close(fd);
        return 0;
    }
    return -1;
}

int LightSensor::enable(int32_t handle, int en)
{
    int newState = en ? 1 : 0;
    int err = 0;

    if (newState != mEnabled) {
        if (!mEnabled && dev_name != NULL) {
            open_device();
        }

        char sysfs[PATH_MAX];

        strcpy(sysfs, I2C);
        strcat(sysfs, "enable_als_sensor");

        ALOGI_IF(DEBUG, "enable.open(%s), en(%d)", sysfs, en);

        int fd = open(sysfs, O_RDWR);
        if (fd < 0) {
            ALOGE("couldn't open '%s' input device", sysfs);
            err = -1;
        } else {
            char buf[2];

            buf[0] = newState ? '1' : '0';
            buf[1] = '\0';

            write(fd, buf, sizeof(buf));
            close(fd);
            setInitialState();
        }

        mEnabled = newState;

        if (!mEnabled && dev_name != NULL) {
            close_device();
        }
    }
    return err;
}

bool LightSensor::hasPendingEvents() const
{
    return mHasPendingEvent;
}

int LightSensor::readEvents(sensors_event_t* data, int count)
{
    if (count < 1)
        return -EINVAL;

    if (mHasPendingEvent) {
        mHasPendingEvent = false;
        mPendingEvent.timestamp = getTimestamp();
        *data = mPendingEvent;
        return mEnabled ? 1 : 0;
    }

    ssize_t n = mInputReader.fill(data_fd);
    if (n < 0)
        return n;

    int numEventReceived = 0;
    input_event const* event;

    while (count && mInputReader.readEvent(&event)) {
        int type = event->type;
        if (type == EV_ABS) {
            if (event->code == ABS_LIGHT) {
                mPendingEvent.sensor = ID_L;
                mPendingEvent.type = SENSOR_TYPE_LIGHT;
                mPendingEvent.light = (float)event->value;
            }
        } else if (type == EV_SYN) {
            mPendingEvent.timestamp = timevalToNano(event->time);
            if (mEnabled && (mPendingEvent.light != mPreviousLight) ) {
                *data++ = mPendingEvent;
                count--;
                numEventReceived++;
                mPreviousLight = mPendingEvent.light;
            }
        } else {
            ALOGE("LightSensor: unknown event (type=%d, code=%d)",
                    type, event->code);
        }
        mInputReader.next();
    }

    return numEventReceived;
}

float LightSensor::indexToValue(size_t index) const
{
    return 0.0;
}
