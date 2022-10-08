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

#ifndef ANDROID_SENSORS_H
#define ANDROID_SENSORS_H

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <linux/input.h>

#include <hardware/hardware.h>
#include <hardware/sensors.h>

__BEGIN_DECLS

/*****************************************************************************/

#define I2C                 "/sys/bus/i2c/devices/3-0039/"
#define PROXIMITY_DATA      "Avago proximity sensor"
#define LIGHT_DATA          "Avago light sensor"
#define INPUT_EVENT_DEBUG   (0)
#define DEBUG               (0)

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#endif

enum {
    ID_GY = 0,
    ID_RG,
    ID_A,
    ID_M,
    ID_RM,
    ID_PS,
    ID_O,
    ID_RV,
    ID_GRV,
    ID_LA,
    ID_GR,
    ID_SM,
    ID_P,
    ID_SC,
    ID_GMRV,
    ID_SO,
    ID_L = 0x100,
    ID_PX
};

__END_DECLS

#endif  // ANDROID_SENSORS_H
