/*
 * Copyright (C) 2014 The Android Open Source Project
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

#define LOG_TAG "VoldCheckBattery"
#include <cutils/log.h>

#include <binder/IServiceManager.h>
#include <batteryservice/IBatteryPropertiesRegistrar.h>

using namespace android;

namespace
{
    // How often to check battery in seconds
    const int CHECK_PERIOD = 30;

    // How charged should the battery be (percent) to start encrypting
    const int START_THRESHOLD = 10;

    // How charged should the battery be (percent) to continue encrypting
    const int CONTINUE_THRESHOLD = 5;

    const String16 serviceName("batteryproperties");

    sp<IBinder> bs;
    sp<IBatteryPropertiesRegistrar> interface;

    bool singletonInitialized = false;
    time_t last_checked = {0};
    int last_result = 100;

    int is_battery_ok(int threshold)
    {
        time_t now = time(NULL);
        if (now == -1 || difftime(now, last_checked) < 5) {
            goto finish;
        }
        last_checked = now;

        if (!singletonInitialized) {
            bs = defaultServiceManager()->checkService(serviceName);
            if (bs == NULL) {
                SLOGE("No batteryproperties service!");
                goto finish;
            }

            interface = interface_cast<IBatteryPropertiesRegistrar>(bs);
            if (interface == NULL) {
                SLOGE("No IBatteryPropertiesRegistrar interface");
                goto finish;
            }

            singletonInitialized = true;
        }

        {
            BatteryProperty val;
            status_t status = interface
                ->getProperty(android::BATTERY_PROP_CAPACITY, &val);
            if (status == NO_ERROR) {
                SLOGD("Capacity is %d", (int)val.valueInt64);
                last_result = val.valueInt64;
            } else {
                SLOGE("Failed to get battery charge");
                last_result = 100;
            }
        }

    finish:
        return last_result >= threshold;
    }
}

extern "C"
{
    int is_battery_ok_to_start()
    {
      // Bug 16868177 exists to purge this code completely
      return true; //is_battery_ok(START_THRESHOLD);
    }

    int is_battery_ok_to_continue()
    {
      // Bug 16868177 exists to purge this code completely
      return true; //is_battery_ok(CONTINUE_THRESHOLD);
    }
}
