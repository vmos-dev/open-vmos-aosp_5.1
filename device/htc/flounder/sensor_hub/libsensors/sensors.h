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

#ifndef ANDROID_SENSORS_H
#define ANDROID_SENSORS_H

#include <errno.h>
#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <linux/input.h>

#include <hardware/hardware.h>
#include <hardware/sensors.h>

__BEGIN_DECLS

/*****************************************************************************/

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define ID_A                                        0//CW_ACCELERATION
#define ID_M                                        1//CW_MAGNETIC
#define ID_GY                                       2//CW_GYRO
#define ID_L                                        3//CW_LIGHT
#define ID_PS                                       4//CW_PRESSURE
#define ID_O                                        5//CW_ORIENTATION
#define ID_RV                                       6//CW_ROTATIONVECTOR
#define ID_LA                                       7//CW_LINEARACCELERATION
#define ID_G                                        8//CW_GRAVITY

#define ID_CW_MAGNETIC_UNCALIBRATED                 9//CW_MAGNETIC_UNCALIBRATED
#define ID_CW_GYROSCOPE_UNCALIBRATED               10//CW_GYROSCOPE_UNCALIBRATED
#define ID_CW_GAME_ROTATION_VECTOR                 11//CW_GAME_ROTATION_VECTOR
#define ID_CW_GEOMAGNETIC_ROTATION_VECTOR          12//CW_GEOMAGNETIC_ROTATION_VECTOR
#define ID_CW_SIGNIFICANT_MOTION                   13//CW_SIGNIFICANT_MOTION
#define ID_CW_STEP_DETECTOR                        14//CW_STEP_DETECTOR
#define ID_CW_STEP_COUNTER                         15//CW_STEP_COUNTER

#define ID_A_W                                     16//CW_ACCELERATION_WAKE_UP
#define ID_M_W                                     17//CW_MAGNETIC_WAKE_UP
#define ID_GY_W                                    18//CW_GYRO_WAKE_UP
#define ID_PS_W                                    19//CW_PRESSURE_WAKE_UP
#define ID_O_W                                     20//CW_ORIENTATION_WAKE_UP
#define ID_RV_W                                    21//CW_ROTATIONVECTOR_WAKE_UP
#define ID_LA_W                                    22//CW_LINEARACCELERATION_WAKE_UP
#define ID_G_W                                     23//CW_GRAVITY_WAKE_UP
#define ID_CW_MAGNETIC_UNCALIBRATED_W              24//CW_MAGNETIC_UNCALIBRATED_WAKE_UP
#define ID_CW_GYROSCOPE_UNCALIBRATED_W             25//CW_GYROSCOPE_UNCALIBRATED_WAKE_UP
#define ID_CW_GAME_ROTATION_VECTOR_W               26//CW_GAME_ROTATION_VECTOR_WAKE_UP
#define ID_CW_GEOMAGNETIC_ROTATION_VECTOR_W        27//CW_GEOMAGNETIC_ROTATION_VECTOR_WAKE_UP
#define ID_CW_STEP_DETECTOR_W                      28//CW_STEP_DETECTOR_WAKE_UP
#define ID_CW_STEP_COUNTER_W                       29//CW_STEP_COUNTER_WAKE_UP
/*****************************************************************************/

// The SENSORS Module
#define EVENT_TYPE_LIGHT           ABS_MISC

enum ABS_status {
	CW_ABS_X = 0x01,
	CW_ABS_Y,
	CW_ABS_Z,
	CW_ABS_X1,
	CW_ABS_Y1,
	CW_ABS_Z1,
	CW_ABS_TIMEDIFF,
	ABS_MAG_ACCURACY = 0x0A,
	ABS_ORI_ACCURACY = 0x0B,
	ABS_PRESSURE_X = 0x10,
	ABS_PRESSURE_Y = 0x11,
	ABS_PRESSURE_Z = 0x12,
	ABS_STEP_DETECTOR = 0x23,
	ABS_STEP_COUNTER = 0x24,
	ABS_TRANSPORT_BUFFER_FULL = 0x2E,
};

#define CONVERT_A        0.01f
#define CONVERT_M        0.01f
#define CONVERT_GYRO     0.01f
#define CONVERT_PS       1.0f
#define CONVERT_O        0.1f
#define CONVERT_ALL      0.01f
#define CONVERT_PRESSURE 100
#define CONVERT_RV       10000

#define CONVERT_1		1.0f
#define CONVERT_10		0.1f
#define CONVERT_100		0.01f
#define CONVERT_1000		0.001f
#define CONVERT_10000		0.0001f

#define RANGE_A                     (4*GRAVITY_EARTH)

#define SENSOR_STATE_MASK           (0x7FFF)

/*****************************************************************************/

__END_DECLS

#endif  // ANDROID_SENSORS_H
