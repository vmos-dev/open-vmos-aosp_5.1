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

#define FUNC_LOG LOGV("%s", __PRETTY_FUNCTION__)

#include <hardware/sensors.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <stdlib.h>

#include <linux/input.h>

#include <utils/Atomic.h>
#include <utils/Log.h>

#include "sensors.h"
#include "MPLSensor.h"

/*****************************************************************************/
/* The SENSORS Module */

#ifdef ENABLE_DMP_SCREEN_AUTO_ROTATION
#define LOCAL_SENSORS (MPLSensor::numSensors + 1)
#else
#define LOCAL_SENSORS MPLSensor::numSensors
#endif

/* Vendor-defined Accel Load Calibration File Method
* @param[out] Accel bias, length 3.  In HW units scaled by 2^16 in body frame
* @return '0' for a successful load, '1' otherwise
* example: int AccelLoadConfig(long* offset);
* End of Vendor-defined Accel Load Cal Method
*/

static struct sensor_t sSensorList[LOCAL_SENSORS];
static int sensors = (sizeof(sSensorList) / sizeof(sensor_t));

static int open_sensors(const struct hw_module_t* module, const char* id,
                        struct hw_device_t** device);

static int sensors__get_sensors_list(struct sensors_module_t* module,
                                     struct sensor_t const** list)
{
    *list = sSensorList;
    return sensors;
}

static struct hw_module_methods_t sensors_module_methods = {
        open: open_sensors
};

struct sensors_module_t HAL_MODULE_INFO_SYM = {
        common: {
                tag: HARDWARE_MODULE_TAG,
                version_major: 1,
                version_minor: 0,
                id: SENSORS_HARDWARE_MODULE_ID,
                name: "Invensense module",
                author: "Invensense Inc.",
                methods: &sensors_module_methods,
        },
        get_sensors_list: sensors__get_sensors_list,
};

struct sensors_poll_context_t {
    struct sensors_poll_device_t device; // must be first

    sensors_poll_context_t();
    ~sensors_poll_context_t();
    int activate(int handle, int enabled);
    int setDelay(int handle, int64_t ns);
    int pollEvents(sensors_event_t* data, int count);

private:
    enum {
        mpl = 0,
        compass,
        dmpOrient,
        numSensorDrivers,   // wake pipe goes here
        numFds,
    };

    struct pollfd mPollFds[numSensorDrivers];
    SensorBase *mSensor;
};

/******************************************************************************/

sensors_poll_context_t::sensors_poll_context_t() {
    VFUNC_LOG;

    mCompassSensor = new CompassSensor();
    MPLSensor *mplSensor = new MPLSensor(mCompassSensor);

   /* For Vendor-defined Accel Calibration File Load
    * Use the Following Constructor and Pass Your Load Cal File Function
    *
    * MPLSensor *mplSensor = new MPLSensor(mCompassSensor, AccelLoadConfig);
    */

    // setup the callback object for handing mpl callbacks
    setCallbackObject(mplSensor);

    // populate the sensor list
    sensors =
            mplSensor->populateSensorList(sSensorList, sizeof(sSensorList));

    mSensor = mplSensor;
    mPollFds[mpl].fd = mSensor->getFd();
    mPollFds[mpl].events = POLLIN;
    mPollFds[mpl].revents = 0;

    mPollFds[compass].fd = mCompassSensor->getFd();
    mPollFds[compass].events = POLLIN;
    mPollFds[compass].revents = 0;

    mPollFds[dmpOrient].fd = ((MPLSensor*) mSensor)->getDmpOrientFd();
    mPollFds[dmpOrient].events = POLLPRI;
    mPollFds[dmpOrient].revents = 0;
}

sensors_poll_context_t::~sensors_poll_context_t() {
    FUNC_LOG;
    delete mSensor;
}

int sensors_poll_context_t::activate(int handle, int enabled) {
    FUNC_LOG;
    return mSensor->enable(handle, enabled);
}

int sensors_poll_context_t::setDelay(int handle, int64_t ns)
{
    FUNC_LOG;
    return mSensor->setDelay(handle, ns);
}

int sensors_poll_context_t::pollEvents(sensors_event_t *data, int count)
{
    VHANDLER_LOG;

    int nbEvents = 0;
    int nb, polltime = -1;

    // look for new events
    nb = poll(mPollFds, numSensorDrivers, polltime);

    if (nb > 0) {
        for (int i = 0; count && i < numSensorDrivers; i++) {
            if (mPollFds[i].revents & (POLLIN | POLLPRI)) {
                nb = 0;
                if (i == mpl) {
                    /* Ignore res */
                    mSensor->readEvents(NULL, 0);
                    mPollFds[i].revents = 0;
                }
                else if (i == compass) {
                    /* Ignore res */
                    ((MPLSensor*) mSensor)->readCompassEvents(NULL, count);
                    mPollFds[i].revents = 0;
                }
            }
        }
        nb = ((MPLSensor*) mSensor)->executeOnData(data, count);
        if (nb > 0) {
            count -= nb;
            nbEvents += nb;
            data += nb;
        }

        if (mPollFds[dmpOrient].revents & (POLLIN | POLLPRI)) {
            nb = ((MPLSensor*) mSensor)->readDmpOrientEvents(data, count);
            mPollFds[dmpOrient].revents= 0;
            if (isDmpScreenAutoRotationEnabled() && nb > 0) {
                count -= nb;
                nbEvents += nb;
                data += nb;
            }
        }
    }

    return nbEvents;
}

/******************************************************************************/

static int poll__close(struct hw_device_t *dev)
{
    FUNC_LOG;
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    if (ctx) {
        delete ctx;
    }
    return 0;
}

static int poll__activate(struct sensors_poll_device_t *dev,
                          int handle, int enabled)
{
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->activate(handle, enabled);
}

static int poll__setDelay(struct sensors_poll_device_t *dev,
                          int handle, int64_t ns)
{
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    int s= ctx->setDelay(handle, ns);
    return s;
}

static int poll__poll(struct sensors_poll_device_t *dev,
                      sensors_event_t* data, int count)
{
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->pollEvents(data, count);
}

/******************************************************************************/

/** Open a new instance of a sensor device using name */
static int open_sensors(const struct hw_module_t* module, const char* id,
                        struct hw_device_t** device)
{
    FUNC_LOG;
    int status = -EINVAL;
    sensors_poll_context_t *dev = new sensors_poll_context_t();

    memset(&dev->device, 0, sizeof(sensors_poll_device_t));

    dev->device.common.tag = HARDWARE_DEVICE_TAG;
    dev->device.common.version  = 0;
    dev->device.common.module   = const_cast<hw_module_t*>(module);
    dev->device.common.close    = poll__close;
    dev->device.activate        = poll__activate;
    dev->device.setDelay        = poll__setDelay;
    dev->device.poll            = poll__poll;

    *device = &dev->device.common;
    status = 0;

    return status;
}
