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
#include "LightSensor.h"
#include "ProximitySensor.h"

/*****************************************************************************/
/* The SENSORS Module */

#ifdef ENABLE_DMP_SCREEN_AUTO_ROTATION
#define GLOBAL_SENSORS (MPLSensor::NumSensors + 1)
#else
#define GLOBAL_SENSORS MPLSensor::NumSensors
#endif

#define LOCAL_SENSORS (2)

#define SENSORS_LIGHT_HANDLE        (ID_L)
#define SENSORS_PROXIMITY_HANDLE    (ID_PX)

static struct sensor_t sSensorList[GLOBAL_SENSORS + LOCAL_SENSORS] = {
    {
        .name       = "Light Sensor",
        .vendor     = "Avago Technologies",
        .version    = 1,
        .handle     = SENSORS_LIGHT_HANDLE,
        .type       = SENSOR_TYPE_LIGHT,
        .maxRange   = 30000.0f,
        .resolution = 1.0f,
        .power      = 0.5f,
        .minDelay   = 100000,
        .reserved   = {}
    },
    {
        .name       = "Proximity Sensor",
        .vendor     = "Avago Technologies",
        .version    = 1,
        .handle     = SENSORS_PROXIMITY_HANDLE,
        .type       = SENSOR_TYPE_PROXIMITY,
        .maxRange   = 5.0f,
        .resolution = 1.0f,
        .power      = 0.5f,
        .minDelay   = 100000,
        .reserved   = {}
    },
};
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
        name: "LGE Sensor module",
        author: "LG Electronics Inc.",
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
    int batch(int handle, int flags, int64_t period_ns, int64_t timeout);

    // return true if the constructor is completed
    bool isValid() { return mInitialized; };
    int flush(int handle);

private:
    enum {
        mpl = 0,
        compass,
        dmpOrient,
        dmpSign,
        dmpPed,
        light,
        proximity,
        numSensorDrivers,   // wake pipe goes here
        numFds,
    };

    static const size_t wake = numFds - 1;
    static const char WAKE_MESSAGE = 'W';
    struct pollfd mPollFds[numFds];
    int mWritePipeFd;
    SensorBase *mSensor[numSensorDrivers];
    CompassSensor *mCompassSensor;
    // return true if the constructor is completed
    bool mInitialized;

    int handleToDriver(int handle) const {
        switch (handle) {
            case ID_GY:
            case ID_RG:
            case ID_A:
            case ID_M:
            case ID_RM:
            case ID_PS:
            case ID_O:
            case ID_RV:
            case ID_GRV:
            case ID_LA:
            case ID_GR:
            case ID_SM:
            case ID_P:
            case ID_SC:
            case ID_GMRV:
            case ID_SO:
                return mpl;
            case ID_L:
                return light;
            case ID_PX:
                return proximity;
        }
        return -EINVAL;
    }
};

/******************************************************************************/

sensors_poll_context_t::sensors_poll_context_t() {
    /* TODO: Handle external pressure sensor */
    mCompassSensor = new CompassSensor();
    MPLSensor *mplSensor = new MPLSensor(mCompassSensor);
    mInitialized = false;
    // Must clean this up early or else the destructor will make a mess.
    memset(mSensor, 0, sizeof(mSensor));

    // setup the callback object for handing mpl callbacks
    setCallbackObject(mplSensor);

    // populate the sensor list
    sensors = LOCAL_SENSORS +
       mplSensor->populateSensorList(sSensorList + LOCAL_SENSORS,
               sizeof(sSensorList[0]) * (ARRAY_SIZE(sSensorList) - LOCAL_SENSORS));

    mSensor[mpl] = mplSensor;
    mPollFds[mpl].fd = mSensor[mpl]->getFd();
    mPollFds[mpl].events = POLLIN;
    mPollFds[mpl].revents = 0;

    mSensor[compass] = mplSensor;
    mPollFds[compass].fd = mCompassSensor->getFd();
    mPollFds[compass].events = POLLIN;
    mPollFds[compass].revents = 0;

    mSensor[dmpOrient] = mplSensor;
    mPollFds[dmpOrient].fd = ((MPLSensor*) mSensor[dmpOrient])->getDmpOrientFd();
    mPollFds[dmpOrient].events = POLLPRI;
    mPollFds[dmpOrient].revents = 0;

    mSensor[dmpSign] = mplSensor;
    mPollFds[dmpSign].fd = ((MPLSensor*) mSensor[dmpSign])->getDmpSignificantMotionFd();
    mPollFds[dmpSign].events = POLLPRI;
    mPollFds[dmpSign].revents = 0;

    mSensor[dmpPed] = mplSensor;
    mPollFds[dmpPed].fd = ((MPLSensor*) mSensor[dmpPed])->getDmpPedometerFd();
    mPollFds[dmpPed].events = POLLPRI;
    mPollFds[dmpPed].revents = 0;

    mSensor[light] = new LightSensor();
    mPollFds[light].fd = mSensor[light]->getFd();
    mPollFds[light].events = POLLIN;
    mPollFds[light].revents = 0;

    mSensor[proximity] = new ProximitySensor();
    mPollFds[proximity].fd = mSensor[proximity]->getFd();
    mPollFds[proximity].events = POLLIN;
    mPollFds[proximity].revents = 0;

    if (mPollFds[light].fd < 0 || mPollFds[proximity].fd < 0) {
        delete mCompassSensor;
        return;
    }

    /* Timer based sensor initialization */
    int wakeFds[2];
    int result = pipe(wakeFds);
    ALOGE_IF(result < 0, "error creating wake pipe (%s)", strerror(errno));
    fcntl(wakeFds[0], F_SETFL, O_NONBLOCK);
    fcntl(wakeFds[1], F_SETFL, O_NONBLOCK);
    mWritePipeFd = wakeFds[1];

    mPollFds[wake].fd = wakeFds[0];
    mPollFds[wake].events = POLLIN;
    mPollFds[wake].revents = 0;
    mInitialized = true;
}

sensors_poll_context_t::~sensors_poll_context_t() {
    for (int i=0 ; i<numSensorDrivers ; i++) {
        delete mSensor[i];
    }
    delete mCompassSensor;
    close(mPollFds[wake].fd);
    close(mWritePipeFd);
    mInitialized = false;
}

int sensors_poll_context_t::activate(int handle, int enabled) {
    if (!mInitialized) return -EINVAL;
    int index = handleToDriver(handle);
    if (index < 0) return index;
    int err =  mSensor[index]->enable(handle, enabled);
    if (!err) {
        const char wakeMessage(WAKE_MESSAGE);
        int result = write(mWritePipeFd, &wakeMessage, 1);
        ALOGE_IF(result < 0, "error sending wake message (%s)", strerror(errno));
    }
    return err;
}

int sensors_poll_context_t::setDelay(int handle, int64_t ns)
{
    int index = handleToDriver(handle);
    if (index < 0) return index;
    return mSensor[index]->setDelay(handle, ns);
}

int sensors_poll_context_t::pollEvents(sensors_event_t *data, int count)
{
    int nbEvents = 0;
    int n = 0;
    int nb, polltime = -1;

    do {
        for (int i = 0; count && i < numSensorDrivers; i++) {
            SensorBase* const sensor(mSensor[i]);
            if (mPollFds[i].revents & (POLLIN | POLLPRI)) {
                nb = 0;
                if (i == mpl) {
                    ((MPLSensor*) sensor)->buildMpuEvent();
                    mPollFds[i].revents = 0;
                    nb = ((MPLSensor*) sensor)->readEvents(data, count);
                    if (nb > 0) {
                        count -= nb;
                        nbEvents += nb;
                        data += nb;
                    }
                } else if (i == compass) {
                    ((MPLSensor*) sensor)->buildCompassEvent();
                    mPollFds[i].revents = 0;
                    nb = ((MPLSensor*) sensor)->readEvents(data, count);
                    if (nb > 0) {
                        count -= nb;
                        nbEvents += nb;
                        data += nb;
                    }
                } else if (i == dmpOrient) {
                    nb = ((MPLSensor*) sensor)->readDmpOrientEvents(data, count);
                    mPollFds[dmpOrient].revents= 0;
                    if (isDmpScreenAutoRotationEnabled() && nb > 0) {
                        count -= nb;
                        nbEvents += nb;
                        data += nb;
                    }
                } else if (i == dmpSign) {
                    ALOGI("HAL: dmpSign interrupt");
                    nb = ((MPLSensor*) sensor)->readDmpSignificantMotionEvents(data, count);
                    mPollFds[i].revents = 0;
                    count -= nb;
                    nbEvents += nb;
                    data += nb;
                } else if (i == dmpPed) {
                    ALOGI("HAL: dmpPed interrupt");
                    nb = ((MPLSensor*) sensor)->readDmpPedometerEvents(data, count, ID_P, SENSOR_TYPE_STEP_DETECTOR, 0);
                    mPollFds[i].revents = 0;
                    count -= nb;
                    nbEvents += nb;
                    data += nb;
                } else {
                    // LightSensor and ProximitySensor
                    nb = sensor->readEvents(data, count);
                    if (nb < count) {
                        // no more data for this sensor
                        mPollFds[i].revents = 0;
                    }
                    count -= nb;
                    nbEvents += nb;
                    data += nb;
                }
                ALOGI_IF(0, "sensors_mpl:readEvents() - nb=%d, count=%d, nbEvents=%d, data->timestamp=%lld, data->data[0]=%f,",
                        nb, count, nbEvents, data->timestamp, data->data[0]);
            }
        }

        /* to see if any step counter events */
        if (((MPLSensor*) mSensor[mpl])->hasStepCountPendingEvents() == true) {
            nb = 0;
            nb = ((MPLSensor*) mSensor[mpl])->readDmpPedometerEvents(data, count, ID_SC, SENSOR_TYPE_STEP_COUNTER, 0);
            ALOGI_IF(0, "sensors_mpl:readStepCount() - nb=%d, count=%d, nbEvents=%d, data->timestamp=%lld, data->data[0]=%f,",
                    nb, count, nbEvents, data->timestamp, data->data[0]);
            if (nb > 0) {
                count -= nb;
                nbEvents += nb;
                data += nb;
            }
        }
        if (count) {
            do {
                n = poll(mPollFds, numFds, nbEvents ? 0 : polltime);
            } while (n < 0 && errno == EINTR);
            if (n < 0) {
                ALOGE("poll() failed (%s)", strerror(errno));
                return -errno;
            }
            if (mPollFds[wake].revents & (POLLIN | POLLPRI)) {
                char msg;
                int result = read(mPollFds[wake].fd, &msg, 1);
                ALOGE_IF(result < 0, "error reading from wake pipe (%s)", strerror(errno));
                ALOGE_IF(msg != WAKE_MESSAGE, "unknown message on wake queue (0x%02x)", int(msg));
                mPollFds[wake].revents = 0;
            }
        }
    } while (n && count);

    return nbEvents;
}

int sensors_poll_context_t::batch(int handle, int flags, int64_t period_ns, int64_t timeout)
{
    int index = handleToDriver(handle);
    if (index < 0) return index;
    return mSensor[index]->batch(handle, flags, period_ns, timeout);
}

int sensors_poll_context_t::flush(int handle)
{
    int index = handleToDriver(handle);
    if (index < 0) return index;
    return mSensor[index]->flush(handle);
}
/******************************************************************************/

static int poll__close(struct hw_device_t *dev)
{
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
    int status = -EINVAL;
    sensors_poll_context_t *dev = new sensors_poll_context_t();

    if (!dev->isValid()) {
        ALOGE("Failed to open the sensors");
        return status;
    }

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
