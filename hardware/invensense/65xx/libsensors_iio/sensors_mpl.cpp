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

/* 
 * Vendor-defined Accel Load Calibration File Method 
 * @param[out] Accel bias, length 3.  In HW units scaled by 2^16 in body frame
 * @return '0' for a successful load, '1' otherwise
 * example: int AccelLoadConfig(long* offset);
 * End of Vendor-defined Accel Load Cal Method 
 */

/*****************************************************************************/
/* The SENSORS Module */

#ifdef ENABLE_DMP_SCREEN_AUTO_ROTATION
#define LOCAL_SENSORS (MPLSensor::NumSensors + 1)
#else
#define LOCAL_SENSORS MPLSensor::NumSensors
#endif

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
                dso: NULL,
                reserved: {0}
        },
        get_sensors_list: sensors__get_sensors_list,
};

struct sensors_poll_context_t {
    sensors_poll_device_1_t device; // must be first

    sensors_poll_context_t();
    ~sensors_poll_context_t();
    int activate(int handle, int enabled);
    int setDelay(int handle, int64_t ns);
    int pollEvents(sensors_event_t* data, int count);
    int query(int what, int *value);
    int batch(int handle, int flags, int64_t period_ns, int64_t timeout);
    int flush(int handle);

private:
    enum {
        mpl = 0,
        compass,
        dmpOrient,
        dmpSign,
        dmpPed,
        numSensorDrivers,   // wake pipe goes here
        numFds,
    };

    struct pollfd mPollFds[numFds];
    SensorBase *mSensor;
    CompassSensor *mCompassSensor;

    static const size_t wake = numSensorDrivers;
    static const char WAKE_MESSAGE = 'W';
    int mWritePipeFd;
};

/******************************************************************************/

sensors_poll_context_t::sensors_poll_context_t() {
    VFUNC_LOG;

    /* TODO: Handle external pressure sensor */
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

    mPollFds[dmpSign].fd = ((MPLSensor*) mSensor)->getDmpSignificantMotionFd();
    mPollFds[dmpSign].events = POLLPRI;
    mPollFds[dmpSign].revents = 0;

    mPollFds[dmpPed].fd = ((MPLSensor*) mSensor)->getDmpPedometerFd();
    mPollFds[dmpPed].events = POLLPRI;
    mPollFds[dmpPed].revents = 0;

    /* Timer based sensor initialization */
    int wakeFds[2];
    int result = pipe(wakeFds);
    LOGE_IF(result<0, "error creating wake pipe (%s)", strerror(errno));
    fcntl(wakeFds[0], F_SETFL, O_NONBLOCK);
    fcntl(wakeFds[1], F_SETFL, O_NONBLOCK);
    mWritePipeFd = wakeFds[1];

    mPollFds[numSensorDrivers].fd = wakeFds[0];
    mPollFds[numSensorDrivers].events = POLLIN;
    mPollFds[numSensorDrivers].revents = 0;
}

sensors_poll_context_t::~sensors_poll_context_t() {
    FUNC_LOG;
    delete mSensor;
    delete mCompassSensor;
    for (int i = 0; i < numSensorDrivers; i++) {
        close(mPollFds[i].fd);
    }
    close(mWritePipeFd);
}

int sensors_poll_context_t::activate(int handle, int enabled) {
    FUNC_LOG;

    int err;
    err = mSensor->enable(handle, enabled);
    if (!err) {
        const char wakeMessage(WAKE_MESSAGE);
        int result = write(mWritePipeFd, &wakeMessage, 1);
        LOGE_IF(result < 0, 
                "error sending wake message (%s)", strerror(errno));
    }
    return err;
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

    polltime = ((MPLSensor*) mSensor)->getStepCountPollTime();

    // look for new events
    nb = poll(mPollFds, numSensorDrivers, polltime);
    LOGI_IF(0, "poll nb=%d, count=%d, pt=%d", nb, count, polltime);
    if (nb > 0) {
        for (int i = 0; count && i < numSensorDrivers; i++) {
            if (mPollFds[i].revents & (POLLIN | POLLPRI)) {
                nb = 0;
                if (i == mpl) {
                    ((MPLSensor*) mSensor)->buildMpuEvent();
                    mPollFds[i].revents = 0;
                } else if (i == compass) {
                    ((MPLSensor*) mSensor)->buildCompassEvent();
                    mPollFds[i].revents = 0;
                } else if (i == dmpOrient) {
                    nb = ((MPLSensor*)mSensor)->
                                        readDmpOrientEvents(data, count);
                    mPollFds[dmpOrient].revents= 0;
                    if (isDmpScreenAutoRotationEnabled() && nb > 0) {
                        count -= nb;
                        nbEvents += nb;
                        data += nb;
                    }
                } else if (i == dmpSign) {
                    LOGI("HAL: dmpSign interrupt");
                    nb = ((MPLSensor*) mSensor)->
                                    readDmpSignificantMotionEvents(data, count);
                    mPollFds[i].revents = 0;
                    count -= nb;
                    nbEvents += nb;
                    data += nb; 
                } else if (i == dmpPed) {
                    LOGI("HAL: dmpPed interrupt");
                    nb = ((MPLSensor*) mSensor)->readDmpPedometerEvents(
                            data, count, ID_P, SENSOR_TYPE_STEP_DETECTOR, 0);
                    mPollFds[i].revents = 0;
                    count -= nb;
                    nbEvents += nb;
                    data += nb;
                }
                #if 1
                if(nb == 0) {
                    nb = ((MPLSensor*) mSensor)->readEvents(data, count);
                    LOGI_IF(0, "sensors_mpl:readEvents() - "
                            "i=%d, nb=%d, count=%d, nbEvents=%d, "
                            "data->timestamp=%lld, data->data[0]=%f,",
                            i, nb, count, nbEvents, data->timestamp, 
                            data->data[0]);
                    if (nb > 0) {
                        count -= nb;
                        nbEvents += nb;
                        data += nb;
                    }
                }
                #endif
            }
        }

        /* to see if any step counter events */
        if(((MPLSensor*) mSensor)->hasStepCountPendingEvents() == true) {
            nb = 0;
            nb = ((MPLSensor*) mSensor)->readDmpPedometerEvents(
                            data, count, ID_SC, SENSOR_TYPE_STEP_COUNTER, 0);
            LOGI_IF(SensorBase::HANDLER_DATA, "sensors_mpl:readStepCount() - "
                    "nb=%d, count=%d, nbEvents=%d, data->timestamp=%lld, "
                    "data->data[0]=%f,",
                    nb, count, nbEvents, data->timestamp, data->data[0]);
            if (nb > 0) {
                count -= nb;
                nbEvents += nb;
                data += nb;
            }
        }
    } else if(nb == 0) {
        /* to see if any step counter events */
        if(((MPLSensor*) mSensor)->hasStepCountPendingEvents() == true) {
            nb = 0;
            nb = ((MPLSensor*) mSensor)->readDmpPedometerEvents(
                            data, count, ID_SC, SENSOR_TYPE_STEP_COUNTER, 0);
            LOGI_IF(SensorBase::HANDLER_DATA, "sensors_mpl:readStepCount() - "
                    "nb=%d, count=%d, nbEvents=%d, data->timestamp=%lld, "
                    "data->data[0]=%f,",
                    nb, count, nbEvents, data->timestamp, data->data[0]);
            if (nb > 0) {
                count -= nb;
                nbEvents += nb;
                data += nb;
            }
        }

        if (mPollFds[numSensorDrivers].revents & POLLIN) {
            char msg;
            int result = read(mPollFds[numSensorDrivers].fd, &msg, 1);
            LOGE_IF(result < 0, 
                    "error reading from wake pipe (%s)", strerror(errno));
            mPollFds[numSensorDrivers].revents = 0;
        }
    }
    return nbEvents;
}

int sensors_poll_context_t::query(int what, int* value)
{
    FUNC_LOG;
    return mSensor->query(what, value);
}

int sensors_poll_context_t::batch(int handle, int flags, int64_t period_ns, 
                                  int64_t timeout)
{
    FUNC_LOG;
    return mSensor->batch(handle, flags, period_ns, timeout);
}

int sensors_poll_context_t::flush(int handle)
{
    FUNC_LOG;
    return mSensor->flush(handle);
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

static int poll__query(struct sensors_poll_device_1 *dev,
                      int what, int *value)
{
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->query(what, value);
}

static int poll__batch(struct sensors_poll_device_1 *dev,
                      int handle, int flags, int64_t period_ns, int64_t timeout)
{
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->batch(handle, flags, period_ns, timeout);
}

static int poll__flush(struct sensors_poll_device_1 *dev,
                      int handle)
{
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->flush(handle);
}
/******************************************************************************/

/** Open a new instance of a sensor device using name */
static int open_sensors(const struct hw_module_t* module, const char* id,
                        struct hw_device_t** device)
{
    FUNC_LOG;
    int status = -EINVAL;
    sensors_poll_context_t *dev = new sensors_poll_context_t();

    memset(&dev->device, 0, sizeof(sensors_poll_device_1));

    dev->device.common.tag = HARDWARE_DEVICE_TAG;
    dev->device.common.version  = SENSORS_DEVICE_API_VERSION_1_0;
    dev->device.common.module   = const_cast<hw_module_t*>(module);
    dev->device.common.close    = poll__close;
    dev->device.activate        = poll__activate;
    dev->device.setDelay        = poll__setDelay;
    dev->device.poll            = poll__poll;

    /* Batch processing */
    dev->device.batch           = poll__batch; 
    dev->device.flush           = poll__flush;

    *device = &dev->device.common;
    status = 0;

    return status;
}
