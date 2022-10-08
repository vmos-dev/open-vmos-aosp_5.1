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

#include "PressureSensor.h"

/*
 * The BMP driver gives pascal values.
 * It needs to be changed into hectoPascal.
 */
#define PRESSURE_HECTO_PA (1.0f/100.0f)

PressureSensor::PressureSensor()
    : IioSensorBase(NULL, "barometer",
                        "events/in_pressure0_thresh_either_en",
                        "in_pressure0_input", IIO_PRESSURE)
{
    mPendingEvent.sensor = ID_PR;
    mPendingEvent.type = SENSOR_TYPE_PRESSURE;
}

void PressureSensor::handleData(int value) {
    ALOGV("PressureSensor::handleData value %d", value);
    mPendingEvent.pressure = value * PRESSURE_HECTO_PA;
}
