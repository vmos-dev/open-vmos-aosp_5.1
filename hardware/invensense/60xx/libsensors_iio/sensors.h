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

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#endif

enum {
    ID_GY = 0,
    ID_RG,
    ID_A,
    ID_M,
    ID_O,
    ID_RV,
    ID_LA,
    ID_GR,
    ID_SO
};

/*****************************************************************************/

/*
 * The SENSORS Module
 */

/* ITG3500 */
#define EVENT_TYPE_GYRO_X          REL_X
#define EVENT_TYPE_GYRO_Y          REL_Y
#define EVENT_TYPE_GYRO_Z          REL_Z
/* MPU6050 MPU9150 */
#define EVENT_TYPE_IACCEL_X        REL_RX
#define EVENT_TYPE_IACCEL_Y        REL_RY
#define EVENT_TYPE_IACCEL_Z        REL_RZ
/* MPU6050 MPU9150 */
#define EVENT_TYPE_ICOMPASS_X      REL_X
#define EVENT_TYPE_ICOMPASS_Y      REL_Y
#define EVENT_TYPE_ICOMPASS_Z      REL_Z
/* MPUxxxx */
#define EVENT_TYPE_TIMESTAMP_HI    REL_MISC
#define EVENT_TYPE_TIMESTAMP_LO    REL_WHEEL

/* Accel BMA250 */
#define EVENT_TYPE_ACCEL_X          ABS_X
#define EVENT_TYPE_ACCEL_Y          ABS_Y
#define EVENT_TYPE_ACCEL_Z          ABS_Z
#define LSG                         (1000.0f)

// conversion of acceleration data to SI units (m/s^2)
#define RANGE_A                     (4*GRAVITY_EARTH)
#define RESOLUTION_A                (GRAVITY_EARTH / LSG)
#define CONVERT_A                   (GRAVITY_EARTH / LSG)
#define CONVERT_A_X                 (CONVERT_A)
#define CONVERT_A_Y                 (CONVERT_A)
#define CONVERT_A_Z                 (CONVERT_A)

/* Compass AKM8975 */
#define EVENT_TYPE_MAGV_X           ABS_RX
#define EVENT_TYPE_MAGV_Y           ABS_RY
#define EVENT_TYPE_MAGV_Z           ABS_RZ
#define EVENT_TYPE_MAGV_STATUS      ABS_RUDDER

// conversion of magnetic data to uT units
#define CONVERT_M                   (0.06f)

__END_DECLS

#endif  // ANDROID_SENSORS_H
