/*
* Copyright (C) 2014 Invensense, Inc.
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

#undef ENABLE_PRESSURE

enum {
    ID_GY = 0,
    ID_RG,
    ID_A,
    ID_M,
    ID_RM,
#ifdef ENABLE_PRESSURE
    ID_PS,
#endif
    ID_O,
    ID_RV,
    ID_GRV,
    ID_LA,
    ID_GR,
    ID_SM,
    ID_P,
    ID_SC,
    ID_GMRV,
    ID_SO
};

enum {
    Gyro = 0,
    RawGyro,
    Accelerometer,
    MagneticField,
    RawMagneticField,
#ifdef ENABLE_PRESSURE
    Pressure,
#endif
    Orientation,
    RotationVector,
    GameRotationVector,
    LinearAccel,
    Gravity,
    SignificantMotion,
    StepDetector,
    StepCounter,
    GeomagneticRotationVector,
    NumSensors
};

#ifdef ENABLE_PRESSURE
#define LAST_HW_SENSOR	Pressure
#else
#define LAST_HW_SENSOR	RawMagneticField
#endif

/* Physical parameters of the sensors supported by Invensense MPL */
#define SENSORS_GYROSCOPE_HANDLE                   (ID_GY)
#define SENSORS_RAW_GYROSCOPE_HANDLE               (ID_RG)
#define SENSORS_ACCELERATION_HANDLE                (ID_A)
#define SENSORS_MAGNETIC_FIELD_HANDLE              (ID_M)
#define SENSORS_RAW_MAGNETIC_FIELD_HANDLE          (ID_RM)
#define SENSORS_PRESSURE_HANDLE                    (ID_PS)
#define SENSORS_ORIENTATION_HANDLE                 (ID_O)
#define SENSORS_ROTATION_VECTOR_HANDLE             (ID_RV)
#define SENSORS_GAME_ROTATION_VECTOR_HANDLE        (ID_GRV)
#define SENSORS_LINEAR_ACCEL_HANDLE                (ID_LA)
#define SENSORS_GRAVITY_HANDLE                     (ID_GR)
#define SENSORS_SIGNIFICANT_MOTION_HANDLE          (ID_SM)
#define SENSORS_PEDOMETER_HANDLE                   (ID_P)
#define SENSORS_STEP_COUNTER_HANDLE                (ID_SC)
#define SENSORS_GEOMAGNETIC_ROTATION_VECTOR_HANDLE (ID_GMRV)
#define SENSORS_SCREEN_ORIENTATION_HANDLE          (ID_SO)

/*****************************************************************************/

/*
   Android KitKat
   Populate sensor_t structure according to hardware sensors.h
   {    name, vendor, version, handle, type, maxRange, resolution, power, minDelay,
    fifoReservedEventCount, fifoMaxEventCount, reserved[]    }
*/
#if defined ANDROID_KITKAT
static struct sensor_t sBaseSensorList[] =
{
    {"MPL Gyroscope", "Invensense", 1, SENSORS_GYROSCOPE_HANDLE,
     SENSOR_TYPE_GYROSCOPE, 2000.0f, 1.0f, 0.5f, 10000, 0, 124,
     SENSOR_STRING_TYPE_GYROSCOPE, 0, 0, SENSOR_FLAG_CONTINUOUS_MODE, {}},
    {"MPL Raw Gyroscope", "Invensense", 1, SENSORS_RAW_GYROSCOPE_HANDLE,
     SENSOR_TYPE_GYROSCOPE_UNCALIBRATED, 2000.0f, 1.0f, 0.5f, 10000, 0, 124,
     SENSOR_STRING_TYPE_GYROSCOPE_UNCALIBRATED, 0, 0, SENSOR_FLAG_CONTINUOUS_MODE, {}},
    {"MPL Accelerometer", "Invensense", 1, SENSORS_ACCELERATION_HANDLE,
     SENSOR_TYPE_ACCELEROMETER, 10240.0f, 1.0f, 0.5f, 10000, 0, 124,
     SENSOR_STRING_TYPE_ACCELEROMETER, 0, 0, SENSOR_FLAG_CONTINUOUS_MODE, {}},
    {"MPL Magnetic Field", "Invensense", 1, SENSORS_MAGNETIC_FIELD_HANDLE,
     SENSOR_TYPE_MAGNETIC_FIELD, 10240.0f, 1.0f, 0.5f, 10000, 0, 124,
     SENSOR_STRING_TYPE_MAGNETIC_FIELD, 0, 0, SENSOR_FLAG_CONTINUOUS_MODE, {}},
    {"MPL Raw Magnetic Field", "Invensense", 1, SENSORS_RAW_MAGNETIC_FIELD_HANDLE,
     SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED, 10240.0f, 1.0f, 0.5f, 10000, 0, 124,
     SENSOR_STRING_TYPE_MAGNETIC_FIELD_UNCALIBRATED, 0, 0, SENSOR_FLAG_CONTINUOUS_MODE, {}},
#ifdef ENABLE_PRESSURE
    {"MPL Pressure", "Invensense", 1, SENSORS_PRESSURE_HANDLE,
     SENSOR_TYPE_PRESSURE, 10240.0f, 1.0f, 0.5f, 10000, 0, 165,
     SENSOR_STRING_TYPE_PRESSURE, 0, 0, SENSOR_FLAG_CONTINUOUS_MODE, {}},
#endif
    {"MPL Orientation", "Invensense", 1, SENSORS_ORIENTATION_HANDLE,
     SENSOR_TYPE_ORIENTATION, 360.0f, 1.0f, 9.7f, 10000, 0, 0,
     SENSOR_STRING_TYPE_ORIENTATION, 0, 0, SENSOR_FLAG_CONTINUOUS_MODE, {}},
    {"MPL Rotation Vector", "Invensense", 1, SENSORS_ROTATION_VECTOR_HANDLE,
     SENSOR_TYPE_ROTATION_VECTOR, 10240.0f, 1.0f, 0.5f, 10000, 0, 0,
     SENSOR_STRING_TYPE_ROTATION_VECTOR, 0, 0, SENSOR_FLAG_CONTINUOUS_MODE, {}},
    {"MPL Game Rotation Vector", "Invensense", 1, SENSORS_GAME_ROTATION_VECTOR_HANDLE,
     SENSOR_TYPE_GAME_ROTATION_VECTOR, 10240.0f, 1.0f, 0.5f, 10000, 0, 62,
     SENSOR_STRING_TYPE_GAME_ROTATION_VECTOR, 0, 0, SENSOR_FLAG_CONTINUOUS_MODE, {}},
    {"MPL Linear Acceleration", "Invensense", 1, SENSORS_LINEAR_ACCEL_HANDLE,
     SENSOR_TYPE_LINEAR_ACCELERATION, 10240.0f, 1.0f, 0.5f, 10000, 0, 0,
     SENSOR_STRING_TYPE_LINEAR_ACCELERATION, 0, 0, SENSOR_FLAG_CONTINUOUS_MODE, {}},
    {"MPL Gravity", "Invensense", 1, SENSORS_GRAVITY_HANDLE,
     SENSOR_TYPE_GRAVITY, 10240.0f, 1.0f, 0.5f, 10000, 0, 0,
     SENSOR_STRING_TYPE_GRAVITY, 0, 0, SENSOR_FLAG_CONTINUOUS_MODE, {}},
    {"MPL Significant Motion", "Invensense", 1, SENSORS_SIGNIFICANT_MOTION_HANDLE,
     SENSOR_TYPE_SIGNIFICANT_MOTION, 100.0f, 1.0f, 1.1f, 0, 0, 0,
     SENSOR_STRING_TYPE_SIGNIFICANT_MOTION, 0, 0,
     SENSOR_FLAG_ONE_SHOT_MODE | SENSOR_FLAG_WAKE_UP, {}},
    {"MPL Step Detector", "Invensense", 1, SENSORS_PEDOMETER_HANDLE,
     SENSOR_TYPE_STEP_DETECTOR, 100.0f, 1.0f, 1.1f, 0, 0, 124,
     SENSOR_STRING_TYPE_STEP_DETECTOR, 0, 0, SENSOR_FLAG_SPECIAL_REPORTING_MODE, {}},
    {"MPL Step Counter", "Invensense", 1, SENSORS_STEP_COUNTER_HANDLE,
     SENSOR_TYPE_STEP_COUNTER, 100.0f, 1.0f, 1.1f, 0, 0, 0,
     SENSOR_STRING_TYPE_STEP_COUNTER, 0, 0, SENSOR_FLAG_ON_CHANGE_MODE, {}},
    {"MPL Geomagnetic Rotation Vector", "Invensense", 1,
     SENSORS_GEOMAGNETIC_ROTATION_VECTOR_HANDLE,
     SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR, 10240.0f, 1.0f, 0.5f, 5000, 0, 0,
     SENSOR_STRING_TYPE_GEOMAGNETIC_ROTATION_VECTOR, 0, 0, SENSOR_FLAG_CONTINUOUS_MODE, {}},
#ifdef ENABLE_DMP_SCREEN_AUTO_ROTATION
    {"MPL Screen Orientation", "Invensense ", 1, SENSORS_SCREEN_ORIENTATION_HANDLE,
     SENSOR_TYPE_SCREEN_ORIENTATION, 100.0f, 1.0f, 1.1f, 0, 0, 0,
     SENSOR_STRING_TYPE_SCREEN_ORIENTATION, 0, 0, SENSOR_FLAG_ON_CHANGE_MODE, {}},
#endif
};
#else  //ANDROID_KITKAT END
static struct sensor_t sBaseSensorList[] =
{
    {"MPL Gyroscope", "Invensense", 1,
     SENSORS_GYROSCOPE_HANDLE,
     SENSOR_TYPE_GYROSCOPE, 2000.0f, 1.0f, 0.5f, 10000, {}},
    {"MPL Raw Gyroscope", "Invensense", 1,
     SENSORS_RAW_GYROSCOPE_HANDLE,
     SENSOR_TYPE_GYROSCOPE_UNCALIBRATED, 2000.0f, 1.0f, 0.5f, 10000, {}},
    {"MPL Accelerometer", "Invensense", 1,
     SENSORS_ACCELERATION_HANDLE,
     SENSOR_TYPE_ACCELEROMETER, 10240.0f, 1.0f, 0.5f, 10000, {}},
    {"MPL Magnetic Field", "Invensense", 1,
     SENSORS_MAGNETIC_FIELD_HANDLE,
     SENSOR_TYPE_MAGNETIC_FIELD, 10240.0f, 1.0f, 0.5f, 10000, {}},
    {"MPL Raw Magnetic Field", "Invensense", 1,
     SENSORS_RAW_MAGNETIC_FIELD_HANDLE,
     SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED, 10240.0f, 1.0f, 0.5f, 10000, {}},
#ifdef ENABLE_PRESSURE
    {"MPL Pressure", "Invensense", 1,
     SENSORS_PRESSURE_HANDLE,
     SENSOR_TYPE_PRESSURE, 10240.0f, 1.0f, 0.5f, 10000, {}},
#endif
    {"MPL Orientation", "Invensense", 1,
     SENSORS_ORIENTATION_HANDLE,
     SENSOR_TYPE_ORIENTATION, 360.0f, 1.0f, 9.7f, 10000, {}},
    {"MPL Rotation Vector", "Invensense", 1,
     SENSORS_ROTATION_VECTOR_HANDLE,
     SENSOR_TYPE_ROTATION_VECTOR, 10240.0f, 1.0f, 0.5f, 10000, {}},
    {"MPL Game Rotation Vector", "Invensense", 1,
     SENSORS_GAME_ROTATION_VECTOR_HANDLE,
     SENSOR_TYPE_GAME_ROTATION_VECTOR, 10240.0f, 1.0f, 0.5f, 10000, {}},
    {"MPL Linear Acceleration", "Invensense", 1,
     SENSORS_LINEAR_ACCEL_HANDLE,
     SENSOR_TYPE_LINEAR_ACCELERATION, 10240.0f, 1.0f, 0.5f, 10000, {}},
    {"MPL Gravity", "Invensense", 1,
     SENSORS_GRAVITY_HANDLE,
     SENSOR_TYPE_GRAVITY, 10240.0f, 1.0f, 0.5f, 10000, {}},
    {"MPL Significant Motion", "Invensense", 1,
     SENSORS_SIGNIFICANT_MOTION_HANDLE,
     SENSOR_TYPE_SIGNIFICANT_MOTION, 100.0f, 1.0f, 1.1f, 0, {}},
    {"MPL Step Detector", "Invensense", 1,
     SENSORS_PEDOMETER_HANDLE,
     SENSOR_TYPE_STEP_DETECTOR, 100.0f, 1.0f, 1.1f, 0, {}},
    {"MPL Step Counter", "Invensense", 1,
     SENSORS_STEP_COUNTER_HANDLE,
     SENSOR_TYPE_STEP_COUNTER, 100.0f, 1.0f, 1.1f, 0, {}},
    {"MPL Geomagnetic Rotation Vector", "Invensense", 1,
     SENSORS_GEOMAGNETIC_ROTATION_VECTOR_HANDLE,
     SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR, 10240.0f, 1.0f, 0.5f, 10000, {}},
#ifdef ENABLE_DMP_SCREEN_AUTO_ROTATION
    {"MPL Screen Orientation", "Invensense ", 1,
     SENSORS_SCREEN_ORIENTATION_HANDLE,
     SENSOR_TYPE_SCREEN_ORIENTATION, 100.0f, 1.0f, 1.1f, 0, {}},
#endif
};
#endif //ANDROID_JELLYBEAN END

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

/* AKM  compasses */
#define EVENT_TYPE_MAGV_X           ABS_RX
#define EVENT_TYPE_MAGV_Y           ABS_RY
#define EVENT_TYPE_MAGV_Z           ABS_RZ
#define EVENT_TYPE_MAGV_STATUS      ABS_RUDDER

/* conversion of magnetic data to uT units */
#define CONVERT_M                   (0.06f)

/* conversion of sensor rates */
#define hertz_request = 200;
#define DEFAULT_MPL_GYRO_RATE           (20000L)     //us
#define DEFAULT_MPL_COMPASS_RATE        (20000L)     //us

#define DEFAULT_HW_GYRO_RATE            (100)        //Hz
#define DEFAULT_HW_ACCEL_RATE           (20)         //ms
#define DEFAULT_HW_COMPASS_RATE         (20000000L)  //ns
#define DEFAULT_HW_AKMD_COMPASS_RATE    (200000000L) //ns

/* convert ns to hardware units */
#define HW_GYRO_RATE_NS                 (1000000000LL / rate_request) // to Hz
#define HW_ACCEL_RATE_NS                (rate_request / (1000000L))   // to ms
#define HW_COMPASS_RATE_NS              (rate_request)                // to ns

/* convert Hz to hardware units */
#define HW_GYRO_RATE_HZ                 (hertz_request)
#define HW_ACCEL_RATE_HZ                (1000 / hertz_request)
#define HW_COMPASS_RATE_HZ              (1000000000LL / hertz_request)

#define RATE_200HZ                      5000000LL
#define RATE_15HZ                       66667000LL
#define RATE_5HZ                        200000000LL
__END_DECLS

#endif  // ANDROID_SENSORS_H
