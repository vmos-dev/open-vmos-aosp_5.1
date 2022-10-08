/*
 * Copyright (C) 2012 The Android Open-Source Project
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
//#define FUNC_LOG ALOGV("%s", __PRETTY_FUNCTION__)
#define FUNC_LOG

#include <hardware/sensors.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <stdlib.h>

#include <utils/Atomic.h>
#include <utils/Log.h>

#include "sensors.h"

#include "MPLSensor.h"
#include "LightSensor.h"
#include "PressureSensor.h"


/*****************************************************************************/

#define DELAY_OUT_TIME 0x7FFFFFFF

#define SENSORS_ROTATION_VECTOR_HANDLE  (ID_RV)
#define SENSORS_LINEAR_ACCEL_HANDLE     (ID_LA)
#define SENSORS_GRAVITY_HANDLE          (ID_GR)
#define SENSORS_GYROSCOPE_HANDLE        (ID_GY)
#define SENSORS_RAW_GYROSCOPE_HANDLE    (ID_RG)
#define SENSORS_ACCELERATION_HANDLE     (ID_A)
#define SENSORS_MAGNETIC_FIELD_HANDLE   (ID_M)
#define SENSORS_ORIENTATION_HANDLE      (ID_O)
#define SENSORS_LIGHT_HANDLE            (ID_L)
#define SENSORS_PROXIMITY_HANDLE        (ID_P)
#define SENSORS_PRESSURE_HANDLE         (ID_PR)
#define AKM_FTRACE 0
#define AKM_DEBUG 0
#define AKM_DATA 0

/*****************************************************************************/

/* The SENSORS Module */
#define LOCAL_SENSORS 2
static struct sensor_t sSensorList[LOCAL_SENSORS + MPLSensor::numSensors] = {
      { "BH1721fvc Light sensor",
          "Rohm",
          1, SENSORS_LIGHT_HANDLE,
          SENSOR_TYPE_LIGHT, 65528.0f, 1.0f, 0.20f, 16000, 0, 0, 0, 0, 0, 0, { } },
      { "BMP182 Pressure sensor",
          "Bosch",
          1, SENSORS_PRESSURE_HANDLE,
          SENSOR_TYPE_PRESSURE, 1100.0f, 0.01f, 0.06f, 50000, 0, 0, 0, 0, 0, 0, { } },
};
static int numSensors = LOCAL_SENSORS;

static int open_sensors(const struct hw_module_t* module, const char* id,
                        struct hw_device_t** device);


static int sensors__get_sensors_list(struct sensors_module_t* module,
                                     struct sensor_t const** list)
{
    *list = sSensorList;
    return numSensors;
}

static struct hw_module_methods_t sensors_module_methods = {
        .open = open_sensors
};

struct sensors_module_t HAL_MODULE_INFO_SYM = {
        .common = {
                .tag = HARDWARE_MODULE_TAG,
                .version_major = 1,
                .version_minor = 0,
                .id = SENSORS_HARDWARE_MODULE_ID,
                .name = "Samsung Sensor module",
                .author = "Samsung Electronic Company",
                .methods = &sensors_module_methods,
                .dso = 0,
                .reserved = {},
        },
        .get_sensors_list = sensors__get_sensors_list,
};

struct sensors_poll_context_t {
    struct sensors_poll_device_t device; // must be first

        sensors_poll_context_t();
        ~sensors_poll_context_t();
    int activate(int handle, int enabled);
    int setDelay(int handle, int64_t ns);
    int pollEvents(sensors_event_t* data, int count);

    // Will return true if the constructor completed
    bool isValid() { return mInitialized; };

private:
    // Will be true if the constructor completed
    bool mInitialized;

    enum {
        mpl = 0,
        compass,
#ifdef ENABLE_DMP_DISPL_ORIENT_FEAT
        dmpOrient,
#endif
        light,
        pressure,
        numSensorDrivers,       // wake pipe goes here
        numFds,
    };

    static const size_t wake = numFds - 1;
    static const char WAKE_MESSAGE = 'W';
    struct pollfd mPollFds[numFds];
    int mWritePipeFd;
    SensorBase* mSensors[numSensorDrivers];

    int handleToDriver(int handle) const {
        switch (handle) {
            case ID_RV:
            case ID_LA:
            case ID_GR:
            case ID_GY:
            case ID_RG:
            case ID_A:
            case ID_M:
            case ID_O:
                return mpl;
#ifdef ENABLE_DMP_DISPL_ORIENT_FEAT
            case ID_SO:
                return dmpOrient;
#endif
            case ID_L:
                return light;
            case ID_PR:
                return pressure;
        }
        return -EINVAL;
    }
};

/*****************************************************************************/

sensors_poll_context_t::sensors_poll_context_t()
{
    FUNC_LOG;
    CompassSensor *p_compasssensor = new CompassSensor();
    MPLSensor *p_mplsen = new MPLSensor(p_compasssensor);
    mInitialized = false;
    // Must clean this up early or else the destructor will make a mess.
    memset(mSensors, 0, sizeof(mSensors));

    setCallbackObject(p_mplsen); //setup the callback object for handing mpl callbacks
    numSensors =
        LOCAL_SENSORS +
        p_mplsen->populateSensorList(sSensorList + LOCAL_SENSORS,
                                     sizeof(sSensorList[0]) * (ARRAY_SIZE(sSensorList) - LOCAL_SENSORS));

    mSensors[mpl] = p_mplsen;
    mPollFds[mpl].fd = mSensors[mpl]->getFd();
    mPollFds[mpl].events = POLLIN;
    mPollFds[mpl].revents = 0;

    mSensors[compass] = p_mplsen;
    mPollFds[compass].fd =  ((MPLSensor*)mSensors[mpl])->getCompassFd();
    mPollFds[compass].events = POLLIN;
    mPollFds[compass].revents = 0;

#ifdef ENABLE_DMP_DISPL_ORIENT_FEAT
    mPollFds[dmpOrient].fd = ((MPLSensor*)mSensors[mpl])->getDmpOrientFd();
    mPollFds[dmpOrient].events = POLLPRI;
    mPollFds[dmpOrient].revents = 0;
#endif
    mSensors[light] = new LightSensor();
    mPollFds[light].fd = mSensors[light]->getFd();
    mPollFds[light].events = POLLIN;
    mPollFds[light].revents = 0;

    mSensors[pressure] = new PressureSensor();
    mPollFds[pressure].fd = mSensors[pressure]->getFd();
    mPollFds[pressure].events = POLLIN;
    mPollFds[pressure].revents = 0;

    int wakeFds[2];
    int result = pipe(wakeFds);
    ALOGE_IF(result<0, "error creating wake pipe (%s)", strerror(errno));
    fcntl(wakeFds[0], F_SETFL, O_NONBLOCK);
    fcntl(wakeFds[1], F_SETFL, O_NONBLOCK);
    mWritePipeFd = wakeFds[1];

    mPollFds[wake].fd = wakeFds[0];
    mPollFds[wake].events = POLLIN;
    mPollFds[wake].revents = 0;
    mInitialized = true;
}

sensors_poll_context_t::~sensors_poll_context_t()
{
    FUNC_LOG;
    for (int i=0 ; i<numSensorDrivers ; i++) {
        delete mSensors[i];
    }
    close(mPollFds[wake].fd);
    close(mWritePipeFd);
    mInitialized = false;
}

int sensors_poll_context_t::activate(int handle, int enabled)
{
    FUNC_LOG;
    if (!mInitialized) return -EINVAL;
    int index = handleToDriver(handle);
    if (index < 0) return index;
    int err =  mSensors[index]->enable(handle, enabled);
    if (!err) {
        const char wakeMessage(WAKE_MESSAGE);
        int result = write(mWritePipeFd, &wakeMessage, 1);
        ALOGE_IF(result<0, "error sending wake message (%s)", strerror(errno));
    }
    return err;
}

int sensors_poll_context_t::setDelay(int handle, int64_t ns)
{
    FUNC_LOG;
    int index = handleToDriver(handle);
    if (index < 0) return index;
    return mSensors[index]->setDelay(handle, ns);
}

int sensors_poll_context_t::pollEvents(sensors_event_t* data, int count)
{
    //FUNC_LOG;
    int nbEvents = 0;
    int n = 0;
    int polltime = -1;
    do {
        for (int i=0 ; count && i<numSensorDrivers ; i++) {
            SensorBase* const sensor(mSensors[i]);
            // See if we have some pending events from the last poll()
            if ((mPollFds[i].revents & (POLLIN | POLLPRI)) || (sensor->hasPendingEvents())) {
                int nb;
                if (i == compass) {
                    /* result is hardcoded to 0 */
                    ((MPLSensor*) sensor)->readCompassEvents(NULL, count);
                    nb = ((MPLSensor*) mSensors[mpl])->executeOnData(data, count);
                }
                else if (i == mpl) {
                    /* result is hardcoded to 0 */
                    sensor->readEvents(NULL, count);
                    nb = ((MPLSensor*) mSensors[mpl])->executeOnData(data, count);
                    mPollFds[i].revents = 0;
                }
#ifdef ENABLE_DMP_DISPL_ORIENT_FEAT
                else if (i == dmpOrient) {
                    nb = ((MPLSensor*) mSensors[mpl])->readDmpOrientEvents(data, count);
                    mPollFds[dmpOrient].revents= 0;
                    if (!isDmpScreenAutoRotationEnabled()) {
                            /* ignore the data */
                            nb = 0;
                    }
                }
#endif
                else {
                    nb = sensor->readEvents(data, count);
                }
                if (nb < count) {
                    // no more data for this sensor
                    mPollFds[i].revents = 0;
                }
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
                ALOGE_IF(result<0, "error reading from wake pipe (%s)", strerror(errno));
                ALOGE_IF(msg != WAKE_MESSAGE, "unknown message on wake queue (0x%02x)", int(msg));
                mPollFds[wake].revents = 0;
            }
        }
        // if we have events and space, go read them
    } while (n && count);

    return nbEvents;
}

/*****************************************************************************/

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
    FUNC_LOG;
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->activate(handle, enabled);
}

static int poll__setDelay(struct sensors_poll_device_t *dev,
                          int handle, int64_t ns)
{
    FUNC_LOG;
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->setDelay(handle, ns);
}

static int poll__poll(struct sensors_poll_device_t *dev,
                      sensors_event_t* data, int count)
{
    FUNC_LOG;
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->pollEvents(data, count);
}

/*****************************************************************************/

/** Open a new instance of a sensor device using name */
static int open_sensors(const struct hw_module_t* module, const char* id,
                        struct hw_device_t** device)
{
    FUNC_LOG;
    int status = -EINVAL;
    sensors_poll_context_t *dev = new sensors_poll_context_t();

    if (!dev->isValid()) {
        ALOGE("Failed to open the sensors");
        return status;
    }

    memset(&dev->device, 0, sizeof(sensors_poll_device_t));

    dev->device.common.tag = HARDWARE_DEVICE_TAG;
    dev->device.common.version  = SENSORS_DEVICE_API_VERSION_1_0;
    dev->device.common.module   = const_cast<hw_module_t*>(module);
    dev->device.common.close    = poll__close;
    dev->device.activate        = poll__activate;
    dev->device.setDelay        = poll__setDelay;
    dev->device.poll            = poll__poll;

    *device = &dev->device.common;
    status = 0;

    return status;
}
