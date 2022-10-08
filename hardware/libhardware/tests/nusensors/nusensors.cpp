/*
 * Copyright (C) 2008 The Android Open Source Project
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

#include <string.h>
#include <stdint.h>
#include <string.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <cutils/log.h>

#include <hardware/sensors.h>
#include <utils/Timers.h>

char const* getSensorName(int type) {
    switch(type) {
        case SENSOR_TYPE_ACCELEROMETER:
            return "Acc";
        case SENSOR_TYPE_MAGNETIC_FIELD:
            return "Mag";
        case SENSOR_TYPE_ORIENTATION:
            return "Ori";
        case SENSOR_TYPE_GYROSCOPE:
            return "Gyr";
        case SENSOR_TYPE_LIGHT:
            return "Lux";
        case SENSOR_TYPE_PRESSURE:
            return "Bar";
        case SENSOR_TYPE_TEMPERATURE:
            return "Tmp";
        case SENSOR_TYPE_PROXIMITY:
            return "Prx";
        case SENSOR_TYPE_GRAVITY:
            return "Grv";
        case SENSOR_TYPE_LINEAR_ACCELERATION:
            return "Lac";
        case SENSOR_TYPE_ROTATION_VECTOR:
            return "Rot";
        case SENSOR_TYPE_RELATIVE_HUMIDITY:
            return "Hum";
        case SENSOR_TYPE_AMBIENT_TEMPERATURE:
            return "Tam";
    }
    return "ukn";
}

int main(int argc, char** argv)
{
    int err;
    struct sensors_poll_device_t* device;
    struct sensors_module_t* module;

    err = hw_get_module(SENSORS_HARDWARE_MODULE_ID, (hw_module_t const**)&module);
    if (err != 0) {
        printf("hw_get_module() failed (%s)\n", strerror(-err));
        return 0;
    }

    err = sensors_open(&module->common, &device);
    if (err != 0) {
        printf("sensors_open() failed (%s)\n", strerror(-err));
        return 0;
    }

    struct sensor_t const* list;
    int count = module->get_sensors_list(module, &list);
    printf("%d sensors found:\n", count);
    for (int i=0 ; i<count ; i++) {
        printf("%s\n"
                "\tvendor: %s\n"
                "\tversion: %d\n"
                "\thandle: %d\n"
                "\ttype: %d\n"
                "\tmaxRange: %f\n"
                "\tresolution: %f\n"
                "\tpower: %f mA\n",
                list[i].name,
                list[i].vendor,
                list[i].version,
                list[i].handle,
                list[i].type,
                list[i].maxRange,
                list[i].resolution,
                list[i].power);
    }

    static const size_t numEvents = 16;
    sensors_event_t buffer[numEvents];

    for (int i=0 ; i<count ; i++) {
        err = device->activate(device, list[i].handle, 0);
        if (err != 0) {
            printf("deactivate() for '%s'failed (%s)\n",
                    list[i].name, strerror(-err));
            return 0;
        }
    }

    for (int i=0 ; i<count ; i++) {
        err = device->activate(device, list[i].handle, 1);
        if (err != 0) {
            printf("activate() for '%s'failed (%s)\n",
                    list[i].name, strerror(-err));
            return 0;
        }
        device->setDelay(device, list[i].handle, ms2ns(10));
    }

    do {
        int n = device->poll(device, buffer, numEvents);
        if (n < 0) {
            printf("poll() failed (%s)\n", strerror(-err));
            break;
        }

        printf("read %d events:\n", n);
        for (int i=0 ; i<n ; i++) {
            const sensors_event_t& data = buffer[i];

            if (data.version != sizeof(sensors_event_t)) {
                printf("incorrect event version (version=%d, expected=%d",
                        data.version, sizeof(sensors_event_t));
                break;
            }

            switch(data.type) {
                case SENSOR_TYPE_ACCELEROMETER:
                case SENSOR_TYPE_MAGNETIC_FIELD:
                case SENSOR_TYPE_ORIENTATION:
                case SENSOR_TYPE_GYROSCOPE:
                case SENSOR_TYPE_GRAVITY:
                case SENSOR_TYPE_LINEAR_ACCELERATION:
                case SENSOR_TYPE_ROTATION_VECTOR:
                    printf("sensor=%s, time=%lld, value=<%5.1f,%5.1f,%5.1f>\n",
                            getSensorName(data.type),
                            data.timestamp,
                            data.data[0],
                            data.data[1],
                            data.data[2]);
                    break;

                case SENSOR_TYPE_LIGHT:
                case SENSOR_TYPE_PRESSURE:
                case SENSOR_TYPE_TEMPERATURE:
                case SENSOR_TYPE_PROXIMITY:
                case SENSOR_TYPE_RELATIVE_HUMIDITY:
                case SENSOR_TYPE_AMBIENT_TEMPERATURE:
                    printf("sensor=%s, time=%lld, value=%f\n",
                            getSensorName(data.type),
                            data.timestamp,
                            data.data[0]);
                    break;

                default:
                    printf("sensor=%d, time=%lld, value=<%f,%f,%f, ...>\n",
                            data.type,
                            data.timestamp,
                            data.data[0],
                            data.data[1],
                            data.data[2]);
                    break;
            }
        }
    } while (1); // fix that


    for (int i=0 ; i<count ; i++) {
        err = device->activate(device, list[i].handle, 0);
        if (err != 0) {
            printf("deactivate() for '%s'failed (%s)\n",
                    list[i].name, strerror(-err));
            return 0;
        }
    }

    err = sensors_close(device);
    if (err != 0) {
        printf("sensors_close() failed (%s)\n", strerror(-err));
    }
    return 0;
}
