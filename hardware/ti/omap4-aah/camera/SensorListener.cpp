/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
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

/**
* @file SensorListener.cpp
*
* This file listens and propogates sensor events to CameraHal.
*
*/

#include "SensorListener.h"

#include <stdint.h>
#include <math.h>
#include <sys/types.h>

namespace Ti {
namespace Camera {

/*** static declarations ***/
static const float RADIANS_2_DEG = (float) (180 / M_PI);
// measured values on device...might need tuning
static const int DEGREES_90_THRESH = 50;
static const int DEGREES_180_THRESH = 170;
static const int DEGREES_270_THRESH = 250;

static int sensor_events_listener(int fd, int events, void* data)
{
    SensorListener* listener = (SensorListener*) data;
    ssize_t num_sensors;
    ASensorEvent sen_events[8];
    while ((num_sensors = listener->mSensorEventQueue->read(sen_events, 8)) > 0) {
        for (int i = 0; i < num_sensors; i++) {
            if (sen_events[i].type == android::Sensor::TYPE_ACCELEROMETER) {
                float x = sen_events[i].vector.azimuth;
                float y = sen_events[i].vector.pitch;
                float z = sen_events[i].vector.roll;
                float radius = 0;
                int tilt = 0, orient = 0;

                CAMHAL_LOGVA("ACCELEROMETER EVENT");
                CAMHAL_LOGVB(" azimuth = %f pitch = %f roll = %f",
                             sen_events[i].vector.azimuth,
                             sen_events[i].vector.pitch,
                             sen_events[i].vector.roll);
                // see http://en.wikipedia.org/wiki/Spherical_coordinate_system#Cartesian_coordinates
                // about conversion from cartesian to spherical for orientation calculations
                radius = (float) sqrt(x * x + y * y + z * z);
                tilt = (int) asinf(z / radius) * RADIANS_2_DEG;
                orient = (int) atan2f(-x, y) * RADIANS_2_DEG;

                if (orient < 0) {
                    orient += 360;
                }

                if (orient >= DEGREES_270_THRESH) {
                    orient = 270;
                } else if (orient >= DEGREES_180_THRESH) {
                    orient = 180;
                } else if (orient >= DEGREES_90_THRESH) {
                    orient = 90;
                } else {
                    orient = 0;
                }
                listener->handleOrientation(orient, tilt);
                CAMHAL_LOGVB(" tilt = %d orientation = %d", tilt, orient);
            } else if (sen_events[i].type == android::Sensor::TYPE_GYROSCOPE) {
                CAMHAL_LOGVA("GYROSCOPE EVENT");
            }
        }
    }

    if (num_sensors < 0 && num_sensors != -EAGAIN) {
        CAMHAL_LOGEB("reading events failed: %s", strerror(-num_sensors));
    }

    return 1;
}

/****** public - member functions ******/
SensorListener::SensorListener() {
    LOG_FUNCTION_NAME;

    sensorsEnabled = 0;
    mOrientationCb = NULL;
    mSensorEventQueue = NULL;
    mSensorLooperThread = NULL;

    LOG_FUNCTION_NAME_EXIT;
}

SensorListener::~SensorListener() {
    LOG_FUNCTION_NAME;

    CAMHAL_LOGDA("Kill looper thread");
    if (mSensorLooperThread.get()) {
        // 1. Request exit
        // 2. Wake up looper which should be polling for an event
        // 3. Wait for exit
        mSensorLooperThread->requestExit();
        mSensorLooperThread->wake();
        mSensorLooperThread->join();
        mSensorLooperThread.clear();
        mSensorLooperThread = NULL;
    }

    CAMHAL_LOGDA("Kill looper");
    if (mLooper.get()) {
        mLooper->removeFd(mSensorEventQueue->getFd());
        mLooper.clear();
        mLooper = NULL;
    }
    CAMHAL_LOGDA("SensorListener destroyed");

    LOG_FUNCTION_NAME_EXIT;
}

status_t SensorListener::initialize() {
    status_t ret = NO_ERROR;
    android::SensorManager& mgr(android::SensorManager::getInstance());

    LOG_FUNCTION_NAME;

    android::sp<android::Looper> mLooper;

    mSensorEventQueue = mgr.createEventQueue();
    if (mSensorEventQueue == NULL) {
        CAMHAL_LOGEA("createEventQueue returned NULL");
        ret = NO_INIT;
        goto out;
    }

    mLooper = new android::Looper(false);
    mLooper->addFd(mSensorEventQueue->getFd(), 0, ALOOPER_EVENT_INPUT, sensor_events_listener, this);

    if (mSensorLooperThread.get() == NULL)
            mSensorLooperThread = new SensorLooperThread(mLooper.get());

    if (mSensorLooperThread.get() == NULL) {
        CAMHAL_LOGEA("Couldn't create sensor looper thread");
        ret = NO_MEMORY;
        goto out;
    }

    ret = mSensorLooperThread->run("sensor looper thread", android::PRIORITY_URGENT_DISPLAY);
    if (ret == INVALID_OPERATION){
        CAMHAL_LOGDA("thread already running ?!?");
    } else if (ret != NO_ERROR) {
        CAMHAL_LOGEA("couldn't run thread");
        goto out;
    }

 out:
    LOG_FUNCTION_NAME_EXIT;
    return ret;
}

void SensorListener::setCallbacks(orientation_callback_t orientation_cb, void *cookie) {
    LOG_FUNCTION_NAME;

    if (orientation_cb) {
        mOrientationCb = orientation_cb;
    }
    mCbCookie = cookie;

    LOG_FUNCTION_NAME_EXIT;
}

void SensorListener::handleOrientation(uint32_t orientation, uint32_t tilt) {
    LOG_FUNCTION_NAME;

    android::AutoMutex lock(&mLock);

    if (mOrientationCb && (sensorsEnabled & SENSOR_ORIENTATION)) {
        mOrientationCb(orientation, tilt, mCbCookie);
    }

    LOG_FUNCTION_NAME_EXIT;
}

void SensorListener::enableSensor(sensor_type_t type) {
    android::Sensor const* sensor;
    android::SensorManager& mgr(android::SensorManager::getInstance());

    LOG_FUNCTION_NAME;

    android::AutoMutex lock(&mLock);

    if ((type & SENSOR_ORIENTATION) && !(sensorsEnabled & SENSOR_ORIENTATION)) {
        sensor = mgr.getDefaultSensor(android::Sensor::TYPE_ACCELEROMETER);
        if(sensor) {
            CAMHAL_LOGDB("orientation = %p (%s)", sensor, sensor->getName().string());
            mSensorEventQueue->enableSensor(sensor);
            mSensorEventQueue->setEventRate(sensor, ms2ns(100));
            sensorsEnabled |= SENSOR_ORIENTATION;
        } else {
            CAMHAL_LOGDB("not enabling absent orientation sensor");
        }
    }

    LOG_FUNCTION_NAME_EXIT;
}

void SensorListener::disableSensor(sensor_type_t type) {
    android::Sensor const* sensor;
    android::SensorManager& mgr(android::SensorManager::getInstance());

    LOG_FUNCTION_NAME;

    android::AutoMutex lock(&mLock);

    if ((type & SENSOR_ORIENTATION) && (sensorsEnabled & SENSOR_ORIENTATION)) {
        sensor = mgr.getDefaultSensor(android::Sensor::TYPE_ACCELEROMETER);
        CAMHAL_LOGDB("orientation = %p (%s)", sensor, sensor->getName().string());
        mSensorEventQueue->disableSensor(sensor);
        sensorsEnabled &= ~SENSOR_ORIENTATION;
    }

    LOG_FUNCTION_NAME_EXIT;
}

} // namespace Camera
} // namespace Ti
