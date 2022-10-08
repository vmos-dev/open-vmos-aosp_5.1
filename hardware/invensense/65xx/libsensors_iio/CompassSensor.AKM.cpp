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

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>
#include <cutils/log.h>
#include <linux/input.h>

#include "sensor_params.h"
#include "MPLSupport.h"

// TODO: include corresponding header file for 3rd party compass sensor
#include "CompassSensor.AKM.h"

// TODO: specify this for "fillList()" API
#define COMPASS_NAME "AKM8963"

/*****************************************************************************/

CompassSensor::CompassSensor() 
                    : SensorBase(NULL, NULL)
{
    VFUNC_LOG;

    // TODO: initiate 3rd-party's class, and disable its funtionalities
    //       proper commands
    mCompassSensor = new AkmSensor();
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
            0, "/sys/class/compass/akm8963/enable_mag", getTimestamp());
    write_sysfs_int((char*)"/sys/class/compass/akm8963/enable_mag", 0);
}

CompassSensor::~CompassSensor()
{
    VFUNC_LOG;

    // TODO: disable 3rd-party's funtionalities and delete the object
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
            0, "/sys/class/compass/akm8963/enable_mag", getTimestamp());
    write_sysfs_int((char*)"/sys/class/compass/akm8963/enable_mag", 0);
    delete mCompassSensor;
}

int CompassSensor::getFd(void) const
{
    VFUNC_LOG;

    // TODO: return 3rd-party's file descriptor
    return mCompassSensor->getFd();
}

/**
 *  @brief        This function will enable/disable sensor.
 *  @param[in]    handle    which sensor to enable/disable.
 *  @param[in]    en        en=1 enable, en=0 disable
 *  @return       if the operation is successful.
 */
int CompassSensor::enable(int32_t handle, int en)
{
    VFUNC_LOG;

    // TODO: called 3rd-party's "set enable/disable" function
    return mCompassSensor->setEnable(handle, en);
}

int CompassSensor::setDelay(int32_t handle, int64_t ns)
{
    VFUNC_LOG;

    // TODO: called 3rd-party's "set delay" function
    return mCompassSensor->setDelay(handle, ns);
}

/**
    @brief      This function will return the state of the sensor.
    @return     1=enabled; 0=disabled
**/
int CompassSensor::getEnable(int32_t handle)
{
    VFUNC_LOG;

    // TODO: return if 3rd-party compass is enabled
    return mCompassSensor->getEnable(handle);
}

/**
    @brief      This function will return the current delay for this sensor.
    @return     delay in nanoseconds. 
**/
int64_t CompassSensor::getDelay(int32_t handle)
{
    VFUNC_LOG;

    // TODO: return 3rd-party's delay time (should be in ns)
    return mCompassSensor->getDelay(handle);
}

/**
    @brief         Integrators need to implement this function per 3rd-party solution
    @param[out]    data      sensor data is stored in this variable. Scaled such that
                             1 uT = 2^16
    @para[in]      timestamp data's timestamp
    @return        1, if 1   sample read, 0, if not, negative if error
**/
int CompassSensor::readSample(long *data, int64_t *timestamp)
{
    VFUNC_LOG;

    // TODO: need to implement "readSample()" for MPL in 3rd-party's .cpp file
    return mCompassSensor->readSample(data, timestamp);
}

/**
    @brief         Integrators need to implement this function per 3rd-party solution
    @param[out]    data      sensor data is stored in this variable. Scaled such that
                             1 uT = 2^16
    @para[in]      timestamp data's timestamp
    @return        1, if 1   sample read, 0, if not, negative if error
**/
int CompassSensor::readRawSample(float *data, int64_t *timestamp)
{
    VFUNC_LOG;
    long ldata[3];

    int res = mCompassSensor->readSample(ldata, timestamp);
    for(int i=0; i<3; i++) {
        data[i] = (float)ldata[i];
    }
    return res; 
}

void CompassSensor::fillList(struct sensor_t *list)
{
    VFUNC_LOG;

    const char *compass = COMPASS_NAME;

    if (compass) {
        if (!strcmp(compass, "AKM8963")) {
            list->maxRange = COMPASS_AKM8963_RANGE;
            list->resolution = COMPASS_AKM8963_RESOLUTION;
            list->power = COMPASS_AKM8963_POWER;
            list->minDelay = COMPASS_AKM8963_MINDELAY;
            return;
        }
        if (!strcmp(compass, "AKM8975")) {
            list->maxRange = COMPASS_AKM8975_RANGE;
            list->resolution = COMPASS_AKM8975_RESOLUTION;
            list->power = COMPASS_AKM8975_POWER;
            list->minDelay = COMPASS_AKM8975_MINDELAY;
            LOGW("HAL:support for AKM8975 is incomplete");
        }
    }

    LOGE("HAL:unsupported compass id %s -- "
         "this implementation only supports AKM compasses", compass);
    list->maxRange = COMPASS_AKM8975_RANGE;
    list->resolution = COMPASS_AKM8975_RESOLUTION;
    list->power = COMPASS_AKM8975_POWER;
    list->minDelay = COMPASS_AKM8975_MINDELAY;
}

// TODO: specify compass sensor's mounting matrix for MPL
void CompassSensor::getOrientationMatrix(signed char *orient)
{
    VFUNC_LOG;

    orient[0] = 1;
    orient[1] = 0;
    orient[2] = 0;
    orient[3] = 0;
    orient[4] = 1;
    orient[5] = 0;
    orient[6] = 0;
    orient[7] = 0;
    orient[8] = 1;
}

int CompassSensor::getAccuracy(void)
{
    VFUNC_LOG;

    // TODO: need to implement "getAccuracy()" for MPL in 3rd-party's .cpp file
    return mCompassSensor->getAccuracy();
}
