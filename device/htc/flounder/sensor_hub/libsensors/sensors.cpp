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

#define LOG_TAG "Sensors"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <stdlib.h>

#include <utils/Atomic.h>
#include <utils/Log.h>

#include <hardware/sensors.h>

#include "sensors.h"
#include "CwMcuSensor.h"

/*****************************************************************************/

#define DELAY_OUT_TIME 0x7FFFFFFF

#define LIGHT_SENSOR_POLLTIME    2000000000

/*****************************************************************************/
static const struct sensor_t sSensorList[] = {
        {.name =       "Accelerometer Sensor",
         .vendor =     "HTC Group Ltd.",
         .version =    1,
         .handle =     ID_A,
         .type =       SENSOR_TYPE_ACCELEROMETER,
         .maxRange =   RANGE_A,
         .resolution = CONVERT_A,
         .power =      0.17f,
         .minDelay =   10000,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =   1220,
         .stringType =         0,
         .requiredPermission = 0,
         .maxDelay =      200000,
         .flags = SENSOR_FLAG_CONTINUOUS_MODE,
         .reserved =          {}
        },
        {.name =       "Magnetic field Sensor",
         .vendor =     "HTC Group Ltd.",
         .version =    1,
         .handle =     ID_M,
         .type =       SENSOR_TYPE_MAGNETIC_FIELD,
         .maxRange =   200.0f,
         .resolution = CONVERT_M,
         .power =      5.0f,
         .minDelay =   10000,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =   1220,
         .stringType =         0,
         .requiredPermission = 0,
         .maxDelay =      200000,
         .flags = SENSOR_FLAG_CONTINUOUS_MODE,
         .reserved =          {}
        },
        {.name =       "Gyroscope Sensor",
         .vendor =     "HTC Group Ltd.",
         .version =    1,
         .handle =     ID_GY,
         .type =       SENSOR_TYPE_GYROSCOPE,
         .maxRange =   2000.0f,
         .resolution = CONVERT_GYRO,
         .power =      6.1f,
         .minDelay =   10000,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =   1220,
         .stringType =         0,
         .requiredPermission = 0,
         .maxDelay =      200000,
         .flags = SENSOR_FLAG_CONTINUOUS_MODE,
         .reserved =          {}
        },
        {.name =       "CM32181 Light sensor",
         .vendor =     "Capella Microsystems",
         .version =    1,
         .handle =     ID_L,
         .type =       SENSOR_TYPE_LIGHT,
         .maxRange =   10240.0f,
         .resolution = 1.0f,
         .power =      0.15f,
         .minDelay =   0,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =      0,
         .stringType =         NULL,
         .requiredPermission = NULL,
         .maxDelay =           0,
         .flags = SENSOR_FLAG_ON_CHANGE_MODE,
         .reserved =          {}
        },
        {.name =       "Pressure Sensor",
         .vendor =     "HTC Group Ltd.",
         .version =    1,
         .handle =     ID_PS,
         .type =       SENSOR_TYPE_PRESSURE,
         .maxRange =   2000,
         .resolution = 1.0f,
         .power =      0.0027f,
         .minDelay =   10000,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =   1220,
         .stringType =         0,
         .requiredPermission = 0,
         .maxDelay =      200000,
         .flags = SENSOR_FLAG_CONTINUOUS_MODE,
         .reserved =          {}
        },
        {.name =       "CWGD Orientation Sensor",
         .vendor =     "HTC Group Ltd.",
         .version =    1,
         .handle =     ID_O,
         .type =       SENSOR_TYPE_ORIENTATION,
         .maxRange =   360.0f,
         .resolution = 0.1f,
         .power =      11.27f,
         .minDelay =   10000,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =   1220,
         .stringType =         0,
         .requiredPermission = 0,
         .maxDelay =      200000,
         .flags = SENSOR_FLAG_CONTINUOUS_MODE,
         .reserved =          {}
        },
        {.name =       "Rotation Vector",
         .vendor =     "HTC Group Ltd.",
         .version =    1,
         .handle =     ID_RV,
         .type =       SENSOR_TYPE_ROTATION_VECTOR,
         .maxRange =   1.0f,
         .resolution = 0.0001f,
         .power =      11.27f,
         .minDelay =   10000,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =   1220,
         .stringType =         0,
         .requiredPermission = 0,
         .maxDelay =      200000,
         .flags = SENSOR_FLAG_CONTINUOUS_MODE,
         .reserved =          {}
        },
        {.name =       "Linear Acceleration",
         .vendor =     "HTC Group Ltd.",
         .version =    1,
         .handle =     ID_LA,
         .type =       SENSOR_TYPE_LINEAR_ACCELERATION,
         .maxRange =   RANGE_A,
         .resolution = 0.01,
         .power =      11.27f,
         .minDelay =   10000,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =   1220,
         .stringType =         0,
         .requiredPermission = 0,
         .maxDelay =      200000,
         .flags = SENSOR_FLAG_CONTINUOUS_MODE,
         .reserved =          {}
        },
        {.name =       "Gravity",
         .vendor =     "HTC Group Ltd.",
         .version =    1,
         .handle =     ID_G,
         .type =       SENSOR_TYPE_GRAVITY,
         .maxRange =   GRAVITY_EARTH,
         .resolution = 0.01,
         .power =      11.27f,
         .minDelay =   10000,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =   1220,
         .stringType =         0,
         .requiredPermission = 0,
         .maxDelay =      200000,
         .flags = SENSOR_FLAG_CONTINUOUS_MODE,
         .reserved =          {}
        },
        {.name =       "Magnetic Uncalibrated",
         .vendor =     "hTC Corp.",
         .version =    1,
         .handle =     ID_CW_MAGNETIC_UNCALIBRATED,
         .type =       SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED,
         .maxRange =   200.0f,
         .resolution = CONVERT_M,
         .power =      5.0f,
         .minDelay =   10000,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =    610,
         .stringType =         0,
         .requiredPermission = 0,
         .maxDelay =      200000,
         .flags = SENSOR_FLAG_CONTINUOUS_MODE,
         .reserved =          {}
        },
        {.name =       "Gyroscope Uncalibrated",
         .vendor =     "hTC Corp.",
         .version =    1,
         .handle =     ID_CW_GYROSCOPE_UNCALIBRATED,
         .type =       SENSOR_TYPE_GYROSCOPE_UNCALIBRATED,
         .maxRange =   2000.0f,
         .resolution = CONVERT_GYRO,
         .power =      6.1f,
         .minDelay =   10000,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =    610,
         .stringType =         0,
         .requiredPermission = 0,
         .maxDelay =      200000,
         .flags = SENSOR_FLAG_CONTINUOUS_MODE,
         .reserved =          {}
        },
        {.name =       "Game Rotation Vector",
         .vendor =     "hTC Corp.",
         .version =    1,
         .handle =     ID_CW_GAME_ROTATION_VECTOR,
         .type =       SENSOR_TYPE_GAME_ROTATION_VECTOR,
         .maxRange =   1.0f,
         .resolution = 0.0001f,
         .power =      11.27f,
         .minDelay =   10000,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =   1220,
         .stringType =         0,
         .requiredPermission = 0,
         .maxDelay =      200000,
         .flags = SENSOR_FLAG_CONTINUOUS_MODE,
         .reserved =          {}
        },
        {.name =       "Geomagnetic Rotation Vector",
         .vendor =     "hTC Corp.",
         .version =    1,
         .handle =     ID_CW_GEOMAGNETIC_ROTATION_VECTOR,
         .type =       SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR,
         .maxRange =   1.0f,
         .resolution = 0.0001f,
         .power =      11.27f,
         .minDelay =   10000,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =   1220,
         .stringType =         0,
         .requiredPermission = 0,
         .maxDelay =      200000,
         .flags = SENSOR_FLAG_CONTINUOUS_MODE,
         .reserved =          {}
        },
        {.name =       "Significant Motion",
         .vendor =     "hTC Corp.",
         .version =    1,
         .handle =     ID_CW_SIGNIFICANT_MOTION,
         .type =       SENSOR_TYPE_SIGNIFICANT_MOTION,
         .maxRange =   200.0f,
         .resolution = 1.0f,
         .power =      0.17f,
         .minDelay =   -1,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =      0,
         .stringType =         0,
         .requiredPermission = 0,
         .maxDelay =           0,
         .flags = SENSOR_FLAG_ONE_SHOT_MODE | SENSOR_FLAG_WAKE_UP,
         .reserved =          {}
        },
        {.name =       "Step Detector",
         .vendor =     "hTC Corp.",
         .version =    1,
         .handle =     ID_CW_STEP_DETECTOR,
         .type =       SENSOR_TYPE_STEP_DETECTOR,
         .maxRange =   200.0f,
         .resolution = 1.0f,
         .power =      0.17f,
         .minDelay =   0,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =   1220,
         .stringType =         0,
         .requiredPermission = 0,
         .maxDelay =           0,
         .flags = SENSOR_FLAG_SPECIAL_REPORTING_MODE,
         .reserved =          {}
        },
        {.name =       "Step Counter",
         .vendor =     "hTC Corp.",
         .version =    1,
         .handle =     ID_CW_STEP_COUNTER,
         .type =       SENSOR_TYPE_STEP_COUNTER,
         .maxRange =   200.0f,
         .resolution = 1.0f,
         .power =      0.17f,
         .minDelay =   0,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =   1220,
         .stringType =         0,
         .requiredPermission = 0,
         .maxDelay =           0,
         .flags = SENSOR_FLAG_ON_CHANGE_MODE,
         .reserved =          {}
        },


        {.name =       "Accelerometer Sensor (WAKE_UP)",
         .vendor =     "HTC Group Ltd.",
         .version =    1,
         .handle =     ID_A_W,
         .type =       SENSOR_TYPE_ACCELEROMETER,
         .maxRange =   RANGE_A,
         .resolution = CONVERT_A,
         .power =      0.17f,
         .minDelay =   10000,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =   1220,
         .stringType =         0,
         .requiredPermission = 0,
         .maxDelay =      200000,
         .flags = SENSOR_FLAG_CONTINUOUS_MODE | SENSOR_FLAG_WAKE_UP,
         .reserved =          {}
        },
        {.name =       "Magnetic field Sensor (WAKE_UP)",
         .vendor =     "HTC Group Ltd.",
         .version =    1,
         .handle =     ID_M_W,
         .type =       SENSOR_TYPE_MAGNETIC_FIELD,
         .maxRange =   200.0f,
         .resolution = CONVERT_M,
         .power =      5.0f,
         .minDelay =   10000,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =   1220,
         .stringType =         0,
         .requiredPermission = 0,
         .maxDelay =      200000,
         .flags = SENSOR_FLAG_CONTINUOUS_MODE | SENSOR_FLAG_WAKE_UP,
         .reserved =          {}
        },
        {.name =       "Gyroscope Sensor (WAKE_UP)",
         .vendor =     "HTC Group Ltd.",
         .version =    1,
         .handle =     ID_GY_W,
         .type =       SENSOR_TYPE_GYROSCOPE,
         .maxRange =   2000.0f,
         .resolution = CONVERT_GYRO,
         .power =      6.1f,
         .minDelay =   10000,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =   1220,
         .stringType =         0,
         .requiredPermission = 0,
         .maxDelay =      200000,
         .flags = SENSOR_FLAG_CONTINUOUS_MODE | SENSOR_FLAG_WAKE_UP,
         .reserved =          {}
        },
        {.name =       "Pressure Sensor (WAKE_UP)",
         .vendor =     "HTC Group Ltd.",
         .version =    1,
         .handle =     ID_PS_W,
         .type =       SENSOR_TYPE_PRESSURE,
         .maxRange =   2000,
         .resolution = 1.0f,
         .power =      0.0027f,
         .minDelay =   10000,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =   1220,
         .stringType =         0,
         .requiredPermission = 0,
         .maxDelay =      200000,
         .flags = SENSOR_FLAG_CONTINUOUS_MODE | SENSOR_FLAG_WAKE_UP,
         .reserved =          {}
        },
        {.name =       "CWGD Orientation Sensor (WAKE_UP)",
         .vendor =     "HTC Group Ltd.",
         .version =    1,
         .handle =     ID_O_W,
         .type =       SENSOR_TYPE_ORIENTATION,
         .maxRange =   360.0f,
         .resolution = 0.1f,
         .power =      11.27f,
         .minDelay =   10000,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =   1220,
         .stringType =         0,
         .requiredPermission = 0,
         .maxDelay =      200000,
         .flags = SENSOR_FLAG_CONTINUOUS_MODE | SENSOR_FLAG_WAKE_UP,
         .reserved =          {}
        },
        {.name =       "Rotation Vector (WAKE_UP)",
         .vendor =     "HTC Group Ltd.",
         .version =    1,
         .handle =     ID_RV_W,
         .type =       SENSOR_TYPE_ROTATION_VECTOR,
         .maxRange =   1.0f,
         .resolution = 0.0001f,
         .power =      11.27f,
         .minDelay =   10000,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =   1220,
         .stringType =         0,
         .requiredPermission = 0,
         .maxDelay =      200000,
         .flags = SENSOR_FLAG_CONTINUOUS_MODE | SENSOR_FLAG_WAKE_UP,
         .reserved =          {}
        },
        {.name =       "Linear Acceleration (WAKE_UP)",
         .vendor =     "HTC Group Ltd.",
         .version =    1,
         .handle =     ID_LA_W,
         .type =       SENSOR_TYPE_LINEAR_ACCELERATION,
         .maxRange =   RANGE_A,
         .resolution = 0.01,
         .power =      11.27f,
         .minDelay =   10000,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =   1220,
         .stringType =         0,
         .requiredPermission = 0,
         .maxDelay =      200000,
         .flags = SENSOR_FLAG_CONTINUOUS_MODE | SENSOR_FLAG_WAKE_UP,
         .reserved =          {}
        },
        {.name =       "Gravity (WAKE_UP)",
         .vendor =     "HTC Group Ltd.",
         .version =    1,
         .handle =     ID_G_W,
         .type =       SENSOR_TYPE_GRAVITY,
         .maxRange =   GRAVITY_EARTH,
         .resolution = 0.01,
         .power =      11.27f,
         .minDelay =   10000,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =   1220,
         .stringType =         0,
         .requiredPermission = 0,
         .maxDelay =      200000,
         .flags = SENSOR_FLAG_CONTINUOUS_MODE | SENSOR_FLAG_WAKE_UP,
         .reserved =          {}
        },
        {.name =       "Magnetic Uncalibrated (WAKE_UP)",
         .vendor =     "hTC Corp.",
         .version =    1,
         .handle =     ID_CW_MAGNETIC_UNCALIBRATED_W,
         .type =       SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED,
         .maxRange =   200.0f,
         .resolution = CONVERT_M,
         .power =      5.0f,
         .minDelay =   10000,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =    610,
         .stringType =         0,
         .requiredPermission = 0,
         .maxDelay =      200000,
         .flags = SENSOR_FLAG_CONTINUOUS_MODE | SENSOR_FLAG_WAKE_UP,
         .reserved =          {}
        },
        {.name =       "Gyroscope Uncalibrated (WAKE_UP)",
         .vendor =     "hTC Corp.",
         .version =    1,
         .handle =     ID_CW_GYROSCOPE_UNCALIBRATED_W,
         .type =       SENSOR_TYPE_GYROSCOPE_UNCALIBRATED,
         .maxRange =   2000.0f,
         .resolution = CONVERT_GYRO,
         .power =      6.1f,
         .minDelay =   10000,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =    610,
         .stringType =         0,
         .requiredPermission = 0,
         .maxDelay =      200000,
         .flags = SENSOR_FLAG_CONTINUOUS_MODE | SENSOR_FLAG_WAKE_UP,
         .reserved =          {}
        },
        {.name =       "Game Rotation Vector (WAKE_UP)",
         .vendor =     "hTC Corp.",
         .version =    1,
         .handle =     ID_CW_GAME_ROTATION_VECTOR_W,
         .type =       SENSOR_TYPE_GAME_ROTATION_VECTOR,
         .maxRange =   1.0f,
         .resolution = 0.0001f,
         .power =      11.27f,
         .minDelay =   10000,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =   1220,
         .stringType =         0,
         .requiredPermission = 0,
         .maxDelay =      200000,
         .flags = SENSOR_FLAG_CONTINUOUS_MODE | SENSOR_FLAG_WAKE_UP,
         .reserved =          {}
        },
        {.name =       "Geomagnetic Rotation Vector (WAKE_UP)",
         .vendor =     "hTC Corp.",
         .version =    1,
         .handle =     ID_CW_GEOMAGNETIC_ROTATION_VECTOR_W,
         .type =       SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR,
         .maxRange =   1.0f,
         .resolution = 0.0001f,
         .power =      11.27f,
         .minDelay =   10000,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =   1220,
         .stringType =         0,
         .requiredPermission = 0,
         .maxDelay =      200000,
         .flags = SENSOR_FLAG_CONTINUOUS_MODE | SENSOR_FLAG_WAKE_UP,
         .reserved =          {}
        },
        {.name =       "Step Detector (WAKE_UP)",
         .vendor =     "hTC Corp.",
         .version =    1,
         .handle =     ID_CW_STEP_DETECTOR_W,
         .type =       SENSOR_TYPE_STEP_DETECTOR,
         .maxRange =   200.0f,
         .resolution = 1.0f,
         .power =      0.17f,
         .minDelay =   0,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =   1220,
         .stringType =         0,
         .requiredPermission = 0,
         .maxDelay =           0,
         .flags = SENSOR_FLAG_SPECIAL_REPORTING_MODE | SENSOR_FLAG_WAKE_UP,
         .reserved =          {}
        },
        {.name =       "Step Counter (WAKE_UP)",
         .vendor =     "hTC Corp.",
         .version =    1,
         .handle =     ID_CW_STEP_COUNTER_W,
         .type =       SENSOR_TYPE_STEP_COUNTER,
         .maxRange =   200.0f,
         .resolution = 1.0f,
         .power =      0.17f,
         .minDelay =   0,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =   1220,
         .stringType =         0,
         .requiredPermission = 0,
         .maxDelay =           0,
         .flags = SENSOR_FLAG_ON_CHANGE_MODE | SENSOR_FLAG_WAKE_UP,
         .reserved =          {}
        },
};

static int open_sensors(const struct hw_module_t* module, const char* id,
                        struct hw_device_t** device);

static int sensors__get_sensors_list(struct sensors_module_t*,
                                     struct sensor_t const** list)
{
    *list = sSensorList;
    return ARRAY_SIZE(sSensorList);
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
            name: "Sensor module",
            author: "Electronic Company",
            methods: &sensors_module_methods,
            dso: NULL,
            reserved: { },
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
    int flush(int handle);

private:
    enum {
        cwmcu            = 0,
        numSensorDrivers,
        numFds,
    };

    static const size_t wake = numFds - 1;
    static const char WAKE_MESSAGE = 'W';
    struct pollfd mPollFds[numFds];
    int mWritePipeFd;
    SensorBase* mSensors[numSensorDrivers];

int handleToDriver(int handle) const {
        switch (handle) {
                case ID_A:
                case ID_M:
                case ID_GY:
                case ID_L:
                case ID_PS:
                case ID_O:
                case ID_RV:
                case ID_LA:
                case ID_G:
                case ID_CW_MAGNETIC_UNCALIBRATED:
                case ID_CW_GYROSCOPE_UNCALIBRATED:
                case ID_CW_GAME_ROTATION_VECTOR:
                case ID_CW_GEOMAGNETIC_ROTATION_VECTOR:
                case ID_CW_SIGNIFICANT_MOTION:
                case ID_CW_STEP_DETECTOR:
                case ID_CW_STEP_COUNTER:
                case ID_A_W:
                case ID_M_W:
                case ID_GY_W:
                case ID_PS_W:
                case ID_O_W:
                case ID_RV_W:
                case ID_LA_W:
                case ID_G_W:
                case ID_CW_MAGNETIC_UNCALIBRATED_W:
                case ID_CW_GYROSCOPE_UNCALIBRATED_W:
                case ID_CW_GAME_ROTATION_VECTOR_W:
                case ID_CW_GEOMAGNETIC_ROTATION_VECTOR_W:
                case ID_CW_STEP_DETECTOR_W:
                case ID_CW_STEP_COUNTER_W:
                        return cwmcu;
        }
        return -EINVAL;
    }
};

/*****************************************************************************/

sensors_poll_context_t::sensors_poll_context_t()
{
    mSensors[cwmcu] = new CwMcuSensor();
    mPollFds[cwmcu].fd = mSensors[cwmcu]->getFd();
    mPollFds[cwmcu].events = POLLIN;
    mPollFds[cwmcu].revents = 0;

    int wakeFds[2];
    int result = pipe(wakeFds);
    ALOGE_IF(result<0, "error creating wake pipe (%s)", strerror(errno));
    fcntl(wakeFds[0], F_SETFL, O_NONBLOCK);
    fcntl(wakeFds[1], F_SETFL, O_NONBLOCK);
    mWritePipeFd = wakeFds[1];

    mPollFds[wake].fd = wakeFds[0];
    mPollFds[wake].events = POLLIN;
    mPollFds[wake].revents = 0;
}

sensors_poll_context_t::~sensors_poll_context_t() {
    for (int i=0 ; i<numSensorDrivers ; i++) {
        delete mSensors[i];
    }
    close(mPollFds[wake].fd);
    close(mWritePipeFd);
}

int sensors_poll_context_t::activate(int handle, int enabled) {
    int index = handleToDriver(handle);
    if (index < 0) return index;
    int err =  mSensors[index]->setEnable(handle, enabled);
    if (enabled && !err) {
        const char wakeMessage(WAKE_MESSAGE);
        int result = write(mWritePipeFd, &wakeMessage, 1);
        ALOGE_IF(result<0, "error sending wake message (%s)", strerror(errno));
    }
    return err;
}

int sensors_poll_context_t::setDelay(int handle, int64_t ns) {

    int index = handleToDriver(handle);
    if (index < 0) return index;
    return mSensors[index]->setDelay(handle, ns);
}

int sensors_poll_context_t::pollEvents(sensors_event_t* data, int count)
{
    int nbEvents = 0;
    int n = 0;
    do {
        // see if we have some leftover from the last poll()
        for (int i=0 ; count && i<numSensorDrivers ; i++) {
            SensorBase* const sensor(mSensors[i]);
            if ((mPollFds[i].revents & POLLIN) || (sensor->hasPendingEvents())) {
                int nb = sensor->readEvents(data, count);
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
            // we still have some room, so try to see if we can get
            // some events immediately or just wait if we don't have
            // anything to return
            do {
                TEMP_FAILURE_RETRY(n = poll(mPollFds, numFds, nbEvents ? 0 : -1));
            } while (n < 0 && errno == EINTR);
            if (n<0) {
                ALOGE("poll() failed (%s)", strerror(errno));
                return -errno;
            }
            if (mPollFds[wake].revents & POLLIN) {
                char msg(WAKE_MESSAGE);
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

int sensors_poll_context_t::batch(int handle, int flags, int64_t period_ns, int64_t timeout)
{
    int index = handleToDriver(handle);

    if (index < 0)
        return index;

    int err = mSensors[index]->batch(handle, flags, period_ns, timeout);

    return err;
}

int sensors_poll_context_t::flush(int handle)
{
    int index = handleToDriver(handle);

    if (index < 0)
        return index;

    int err = mSensors[index]->flush(handle);

    return err;
}


/*****************************************************************************/

static int poll__close(struct hw_device_t *dev)
{
    sensors_poll_context_t *ctx = reinterpret_cast<sensors_poll_context_t *>(dev);
    if (ctx) {
        delete ctx;
    }
    return 0;
}

static int poll__activate(struct sensors_poll_device_t *dev,
        int handle, int enabled) {
    sensors_poll_context_t *ctx = reinterpret_cast<sensors_poll_context_t *>(dev);
    return ctx->activate(handle, enabled);
}

static int poll__setDelay(struct sensors_poll_device_t *dev,
        int handle, int64_t ns) {
    sensors_poll_context_t *ctx = reinterpret_cast<sensors_poll_context_t *>(dev);
    return ctx->setDelay(handle, ns);
}

static int poll__poll(struct sensors_poll_device_t *dev,
        sensors_event_t* data, int count) {
    sensors_poll_context_t *ctx = reinterpret_cast<sensors_poll_context_t *>(dev);
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
/*****************************************************************************/

// Open a new instance of a sensor device using name
static int open_sensors(const struct hw_module_t* module, const char*,
                        struct hw_device_t** device)
{
    sensors_poll_context_t *dev = new sensors_poll_context_t();

    memset(&dev->device, 0, sizeof(sensors_poll_device_1_t));

    dev->device.common.tag = HARDWARE_DEVICE_TAG;
    dev->device.common.version  = SENSORS_DEVICE_API_VERSION_1_3;
    dev->device.common.module   = const_cast<hw_module_t*>(module);
    dev->device.common.close    = poll__close;
    dev->device.activate        = poll__activate;
    dev->device.setDelay        = poll__setDelay;
    dev->device.poll            = poll__poll;

    // Batch processing
    dev->device.batch           = poll__batch;
    dev->device.flush           = poll__flush;

    *device = &dev->device.common;

    return 0;
}

