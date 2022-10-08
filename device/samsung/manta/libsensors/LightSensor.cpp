/*
 * Copyright (C) 2012 Samsung
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

#include <cutils/log.h>
#include <pthread.h>

#include "LightSensor.h"

LightSensor::LightSensor()
    : IioSensorBase(NULL, "lightsensor-level",
                        "events/in_illuminance0_thresh_either_en",
                        "in_illuminance0_input", IIO_LIGHT)
{
    mPendingEvent.sensor = ID_L;
    mPendingEvent.type = SENSOR_TYPE_LIGHT;
}

void LightSensor::handleData(int value) {
    ALOGV("LightSensor::handleData value %d", value);

    /* Measured raw values are 8.9 times lower than actual values*/
    mPendingEvent.light = value * 8.9;
}
