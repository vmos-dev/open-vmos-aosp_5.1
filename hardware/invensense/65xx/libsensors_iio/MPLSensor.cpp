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

#define LOG_NDEBUG 0

//#define KLP 1 //Key Lime Pie Temporary Test Define
//see also the EXTRA_VERBOSE define in the MPLSensor.h header file

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <float.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/syscall.h>
#include <dlfcn.h>
#include <pthread.h>
#include <cutils/log.h>
#include <utils/KeyedVector.h>
#include <utils/String8.h>
#include <string.h>
#include <linux/input.h>
#include <utils/Atomic.h>

#include "MPLSensor.h"
#include "PressureSensor.IIO.secondary.h"
#include "MPLSupport.h"
#include "sensor_params.h"

#include "invensense.h"
#include "invensense_adv.h"
#include "ml_stored_data.h"
#include "ml_load_dmp.h"
#include "ml_sysfs_helper.h"

#define ENABLE_MULTI_RATE
// #define TESTING
// #define USE_LPQ_AT_FASTEST
#define ENABLE_PRESSSURE

#ifdef THIRD_PARTY_ACCEL
#pragma message("HAL:build third party accel support")
#define USE_THIRD_PARTY_ACCEL (1)
#else
#define USE_THIRD_PARTY_ACCEL (0)
#endif

#define MAX_SYSFS_ATTRB (sizeof(struct sysfs_attrbs) / sizeof(char*))

/******************************************************************************/
/*  MPL interface misc.                                                       */
/******************************************************************************/
static int hertz_request = 200;

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

// mask of virtual sensors that require gyro + accel + compass data
#define VIRTUAL_SENSOR_9AXES_MASK (         \
        (1 << Orientation)                  \
        | (1 << RotationVector)             \
        | (1 << LinearAccel)                \
        | (1 << Gravity)                    \
)
// mask of virtual sensors that require gyro + accel data (but no compass data)
#define VIRTUAL_SENSOR_GYRO_6AXES_MASK (    \
        (1 << GameRotationVector)           \
)
// mask of virtual sensors that require mag + accel data (but no gyro data)
#define VIRTUAL_SENSOR_MAG_6AXES_MASK (     \
        (1 << GeomagneticRotationVector)    \
)
// mask of all virtual sensors
#define VIRTUAL_SENSOR_ALL_MASK (           \
        VIRTUAL_SENSOR_9AXES_MASK           \
        | VIRTUAL_SENSOR_GYRO_6AXES_MASK    \
        | VIRTUAL_SENSOR_MAG_6AXES_MASK     \
)

static struct timespec mt_pre;
static struct sensor_t sSensorList[] =
{
    {"MPL Gyroscope", "Invensense", 1,
     SENSORS_GYROSCOPE_HANDLE,
     SENSOR_TYPE_GYROSCOPE, 2000.0f, 1.0f, 0.5f, 10000, 0, 64, 0, 0, 0, 0, {}},
    {"MPL Raw Gyroscope", "Invensense", 1,
     SENSORS_RAW_GYROSCOPE_HANDLE,
     SENSOR_TYPE_GYROSCOPE_UNCALIBRATED, 2000.0f, 1.0f, 0.5f, 10000, 0, 64, 0, 0, 0, 0, {}},
    {"MPL Accelerometer", "Invensense", 1,
     SENSORS_ACCELERATION_HANDLE,
     SENSOR_TYPE_ACCELEROMETER, 10240.0f, 1.0f, 0.5f, 10000, 0, 64, 0, 0, 0, 0, {}},
    {"MPL Magnetic Field", "Invensense", 1,
     SENSORS_MAGNETIC_FIELD_HANDLE,
     SENSOR_TYPE_MAGNETIC_FIELD, 10240.0f, 1.0f, 0.5f, 10000, 0, 64, 0, 0, 0, 0, {}},
    {"MPL Raw Magnetic Field", "Invensense", 1,
     SENSORS_RAW_MAGNETIC_FIELD_HANDLE,
     SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED, 10240.0f, 1.0f, 0.5f, 10000, 0, 64, 0, 0, 0, 0, {}},
#ifdef ENABLE_PRESSURE
    {"MPL Pressure", "Invensense", 1,
     SENSORS_PRESSURE_HANDLE,
     SENSOR_TYPE_PRESSURE, 10240.0f, 1.0f, 0.5f, 10000, 0, 64, 0, 0, 0, 0, {}},
#endif
    {"MPL Orientation", "Invensense", 1,
     SENSORS_ORIENTATION_HANDLE,
     SENSOR_TYPE_ORIENTATION, 360.0f, 1.0f, 9.7f, 10000, 0, 0, 0, 0, 0, 0, {}},
    {"MPL Rotation Vector", "Invensense", 1,
     SENSORS_ROTATION_VECTOR_HANDLE,
     SENSOR_TYPE_ROTATION_VECTOR, 10240.0f, 1.0f, 0.5f, 10000, 0, 0, 0, 0, 0, 0, {}},
    {"MPL Game Rotation Vector", "Invensense", 1,
     SENSORS_GAME_ROTATION_VECTOR_HANDLE,
     SENSOR_TYPE_GAME_ROTATION_VECTOR, 10240.0f, 1.0f, 0.5f, 10000, 0, 42, 0, 0, 0, 0, {}},
    {"MPL Linear Acceleration", "Invensense", 1,
     SENSORS_LINEAR_ACCEL_HANDLE,
     SENSOR_TYPE_LINEAR_ACCELERATION, 10240.0f, 1.0f, 0.5f, 10000, 0, 0, 0, 0, 0, 0, {}},
    {"MPL Gravity", "Invensense", 1,
     SENSORS_GRAVITY_HANDLE,
     SENSOR_TYPE_GRAVITY, 10240.0f, 1.0f, 0.5f, 10000, 0, 0, 0, 0, 0, 0, {}},
    {"MPL Significant Motion", "Invensense", 1,
     SENSORS_SIGNIFICANT_MOTION_HANDLE,
     SENSOR_TYPE_SIGNIFICANT_MOTION, 100.0f, 1.0f, 1.1f, 0, 0, 0, 0, 0, 0, 0, {}},
    {"MPL Step Detector", "Invensense", 1,
     SENSORS_PEDOMETER_HANDLE,
     SENSOR_TYPE_STEP_DETECTOR, 100.0f, 1.0f, 1.1f, 0, 0, 64, 0, 0, 0, 0, {}},
    {"MPL Step Counter", "Invensense", 1,
     SENSORS_STEP_COUNTER_HANDLE,
     SENSOR_TYPE_STEP_COUNTER, 100.0f, 1.0f, 1.1f, 0, 0, 0, 0, 0, 0, 0, {}},
    {"MPL Geomagnetic Rotation Vector", "Invensense", 1,
     SENSORS_GEOMAGNETIC_ROTATION_VECTOR_HANDLE,
     SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR, 10240.0f, 1.0f, 0.5f, 10000, 0, 0, 0, 0, 0, 0, {}},
#ifdef ENABLE_DMP_SCREEN_AUTO_ROTATION
    {"MPL Screen Orientation", "Invensense ", 1,
     SENSORS_SCREEN_ORIENTATION_HANDLE,
     SENSOR_TYPE_SCREEN_ORIENTATION, 100.0f, 1.0f, 1.1f, 0, 0, 0, 0, 0, 0, 0, {}},
#endif
};

MPLSensor *MPLSensor::gMPLSensor = NULL;

extern "C" {
void procData_cb_wrapper()
{
    if(MPLSensor::gMPLSensor) {
        MPLSensor::gMPLSensor->cbProcData();
    }
}

void setCallbackObject(MPLSensor* gbpt)
{
    MPLSensor::gMPLSensor = gbpt;
}

MPLSensor* getCallbackObject() {
    return MPLSensor::gMPLSensor;
}

} // end of extern C

//#define INV_PLAYBACK_DBG
#ifdef INV_PLAYBACK_DBG
static FILE *logfile = NULL;
#endif

/*******************************************************************************
 * MPLSensor class implementation
 ******************************************************************************/

// following extended initializer list would only be available with -std=c++11
//  or -std=gnu+11
MPLSensor::MPLSensor(CompassSensor *compass, int (*m_pt2AccelCalLoadFunc)(long *))
                       : SensorBase(NULL, NULL),
                         mNewData(0),
                         mMasterSensorMask(INV_ALL_SENSORS),
                         mLocalSensorMask(0),
                         mPollTime(-1),
                         mStepCountPollTime(-1),
                         mHaveGoodMpuCal(0),
                         mGyroAccuracy(0),
                         mAccelAccuracy(0),
                         mCompassAccuracy(0),
                         mSampleCount(0),
                         dmp_orient_fd(-1),
                         mDmpOrientationEnabled(0),
                         dmp_sign_motion_fd(-1),
                         mDmpSignificantMotionEnabled(0),
                         dmp_pedometer_fd(-1),
                         mDmpPedometerEnabled(0),
                         mDmpStepCountEnabled(0),
                         mEnabled(0),
                         mBatchEnabled(0),
                         mFlushEnabled(-1),
                         mOldBatchEnabledMask(0),
                         mAccelInputReader(4),
                         mGyroInputReader(32),
                         mTempScale(0),
                         mTempOffset(0),
                         mTempCurrentTime(0),
                         mAccelScale(2),
                         mAccelSelfTestScale(2),
                         mGyroScale(2000),
                         mGyroSelfTestScale(2000),
                         mCompassScale(0),
                         mFactoryGyroBiasAvailable(false),
                         mGyroBiasAvailable(false),
                         mGyroBiasApplied(false),
                         mFactoryAccelBiasAvailable(false),
                         mAccelBiasAvailable(false),
                         mAccelBiasApplied(false),
                         mPendingMask(0),
                         mSensorMask(0),
                         mMplFeatureActiveMask(0),
                         mFeatureActiveMask(0),
                         mDmpOn(0),
                         mPedUpdate(0),
                         mPressureUpdate(0),
                         mQuatSensorTimestamp(0),
                         mStepSensorTimestamp(0),
                         mLastStepCount(0),
                         mLeftOverBufferSize(0),
                         mInitial6QuatValueAvailable(0),
                         mFlushBatchSet(0),
                         mSkipReadEvents(0) {
    VFUNC_LOG;

    inv_error_t rv;
    int i, fd;
    char *port = NULL;
    char *ver_str;
    unsigned long mSensorMask;
    int res;
    FILE *fptr;

    mCompassSensor = compass;

    LOGV_IF(EXTRA_VERBOSE,
            "HAL:MPLSensor constructor : NumSensors = %d", NumSensors);

    pthread_mutex_init(&mMplMutex, NULL);
    pthread_mutex_init(&mHALMutex, NULL);
    memset(mGyroOrientation, 0, sizeof(mGyroOrientation));
    memset(mAccelOrientation, 0, sizeof(mAccelOrientation));

    /* setup sysfs paths */
    inv_init_sysfs_attributes();

    /* get chip name */
    if (inv_get_chip_name(chip_ID) != INV_SUCCESS) {
        LOGE("HAL:ERR- Failed to get chip ID\n");
    } else {
        LOGV_IF(PROCESS_VERBOSE, "HAL:Chip ID= %s\n", chip_ID);
    }

    enable_iio_sysfs();
    
    /* instantiate pressure sensor on secondary bus */
    if (strcmp(mSysfsPath, "") != 0) {
        mPressureSensor = new PressureSensor((const char*)mSysfsPath);
    } else {
        LOGE("HAL:ERR - Failed to instantiate pressure sensor class");
    }

    /* reset driver master enable */
    masterEnable(0);

    //Always load DMP for KLP
    /* Load DMP image if capable, ie. MPU6xxx/9xxx */
    loadDMP();

    /* open temperature fd for temp comp */
    LOGV_IF(EXTRA_VERBOSE, "HAL:gyro temperature path: %s", mpu.temperature);
    gyro_temperature_fd = open(mpu.temperature, O_RDONLY);
    if (gyro_temperature_fd == -1) {
        LOGE("HAL:could not open temperature node");
    } else {
        LOGV_IF(EXTRA_VERBOSE,
                "HAL:temperature_fd opened: %s", mpu.temperature);
    }

    /* read gyro FSR to calculate accel scale later */
    char gyroBuf[5];
    int count = 0;
         LOGV_IF(SYSFS_VERBOSE,
             "HAL:sysfs:cat %s (%lld)", mpu.gyro_fsr, getTimestamp());

    fd = open(mpu.gyro_fsr, O_RDONLY);
    if(fd < 0) {
        LOGE("HAL:Error opening gyro FSR");
    } else {
        memset(gyroBuf, 0, sizeof(gyroBuf));
        count = read_attribute_sensor(fd, gyroBuf, sizeof(gyroBuf));
        if(count < 1) {
            LOGE("HAL:Error reading gyro FSR");
        } else {
            count = sscanf(gyroBuf, "%ld", &mGyroScale);
            if(count)
                LOGV_IF(EXTRA_VERBOSE, "HAL:Gyro FSR used %ld", mGyroScale);
        }
        close(fd);
    }
    
    /* read gyro self test scale used to calculate factory cal bias later */
    char gyroScale[5]; 
         LOGV_IF(SYSFS_VERBOSE,
             "HAL:sysfs:cat %s (%lld)", mpu.in_gyro_self_test_scale, getTimestamp());
    fd = open(mpu.in_gyro_self_test_scale, O_RDONLY);
    if(fd < 0) {
        LOGE("HAL:Error opening gyro self test scale");
    } else {
        memset(gyroBuf, 0, sizeof(gyroBuf));
        count = read_attribute_sensor(fd, gyroScale, sizeof(gyroScale));
        if(count < 1) {
            LOGE("HAL:Error reading gyro self test scale");
        } else {
            count = sscanf(gyroScale, "%ld", &mGyroSelfTestScale);
            if(count)
                LOGV_IF(EXTRA_VERBOSE, "HAL:Gyro self test scale used %ld", mGyroSelfTestScale);
        }
        close(fd);
    }

    /* open Factory Gyro Bias fd */
    /* mFactoryGyBias contains bias values that will be used for device offset */
    memset(mFactoryGyroBias, 0, sizeof(mFactoryGyroBias));
    LOGV_IF(EXTRA_VERBOSE, "HAL:factory gyro x offset path: %s", mpu.in_gyro_x_offset);
    LOGV_IF(EXTRA_VERBOSE, "HAL:factory gyro y offset path: %s", mpu.in_gyro_y_offset);
    LOGV_IF(EXTRA_VERBOSE, "HAL:factory gyro z offset path: %s", mpu.in_gyro_z_offset);   
    gyro_x_offset_fd = open(mpu.in_gyro_x_offset, O_RDWR);
    gyro_y_offset_fd = open(mpu.in_gyro_y_offset, O_RDWR);
    gyro_z_offset_fd = open(mpu.in_gyro_z_offset, O_RDWR);
    if (gyro_x_offset_fd == -1 ||
             gyro_y_offset_fd == -1 || gyro_z_offset_fd == -1) {
            LOGE("HAL:could not open factory gyro calibrated bias");
    } else {
        LOGV_IF(EXTRA_VERBOSE,
             "HAL:gyro_offset opened");
    }

    /* open Gyro Bias fd */
    /* mGyroBias contains bias values that will be used for framework */
    /* mGyroChipBias contains bias values that will be used for dmp */
    memset(mGyroBias, 0, sizeof(mGyroBias));
    memset(mGyroChipBias, 0, sizeof(mGyroChipBias));
    LOGV_IF(EXTRA_VERBOSE, "HAL: gyro x dmp bias path: %s", mpu.in_gyro_x_dmp_bias);
    LOGV_IF(EXTRA_VERBOSE, "HAL: gyro y dmp bias path: %s", mpu.in_gyro_y_dmp_bias);
    LOGV_IF(EXTRA_VERBOSE, "HAL: gyro z dmp bias path: %s", mpu.in_gyro_z_dmp_bias);
    gyro_x_dmp_bias_fd = open(mpu.in_gyro_x_dmp_bias, O_RDWR);
    gyro_y_dmp_bias_fd = open(mpu.in_gyro_y_dmp_bias, O_RDWR);
    gyro_z_dmp_bias_fd = open(mpu.in_gyro_z_dmp_bias, O_RDWR);
    if (gyro_x_dmp_bias_fd == -1 ||
             gyro_y_dmp_bias_fd == -1 || gyro_z_dmp_bias_fd == -1) {
            LOGE("HAL:could not open gyro DMP calibrated bias");
    } else {
        LOGV_IF(EXTRA_VERBOSE,
             "HAL:gyro_dmp_bias opened");
    }

    /* read accel FSR to calcuate accel scale later */
    if (USE_THIRD_PARTY_ACCEL == 0) {
        char buf[3];
        int count = 0;
        LOGV_IF(SYSFS_VERBOSE,
                "HAL:sysfs:cat %s (%lld)", mpu.accel_fsr, getTimestamp());

        fd = open(mpu.accel_fsr, O_RDONLY);
        if(fd < 0) {
            LOGE("HAL:Error opening accel FSR");
        } else {
           memset(buf, 0, sizeof(buf));
           count = read_attribute_sensor(fd, buf, sizeof(buf));
           if(count < 1) {
               LOGE("HAL:Error reading accel FSR");
           } else {
               count = sscanf(buf, "%d", &mAccelScale);
               if(count)
                   LOGV_IF(EXTRA_VERBOSE, "HAL:Accel FSR used %d", mAccelScale);
           }
           close(fd);
        }

        /* read accel self test scale used to calculate factory cal bias later */
        char accelScale[5]; 
             LOGV_IF(SYSFS_VERBOSE,
                 "HAL:sysfs:cat %s (%lld)", mpu.in_accel_self_test_scale, getTimestamp());
        fd = open(mpu.in_accel_self_test_scale, O_RDONLY);
        if(fd < 0) {
            LOGE("HAL:Error opening gyro self test scale");
        } else {
            memset(buf, 0, sizeof(buf));
            count = read_attribute_sensor(fd, accelScale, sizeof(accelScale));
            if(count < 1) {
                LOGE("HAL:Error reading accel self test scale");
            } else {
                count = sscanf(accelScale, "%ld", &mAccelSelfTestScale);
                if(count)
                    LOGV_IF(EXTRA_VERBOSE, "HAL:Accel self test scale used %ld", mAccelSelfTestScale);
            }
            close(fd);
        }
 
        /* open Factory Accel Bias fd */
        /* mFactoryAccelBias contains bias values that will be used for device offset */
        memset(mFactoryAccelBias, 0, sizeof(mFactoryAccelBias));
        LOGV_IF(EXTRA_VERBOSE, "HAL:factory accel x offset path: %s", mpu.in_accel_x_offset);
        LOGV_IF(EXTRA_VERBOSE, "HAL:factory accel y offset path: %s", mpu.in_accel_y_offset);
        LOGV_IF(EXTRA_VERBOSE, "HAL:factory accel z offset path: %s", mpu.in_accel_z_offset);   
        accel_x_offset_fd = open(mpu.in_accel_x_offset, O_RDWR);
        accel_y_offset_fd = open(mpu.in_accel_y_offset, O_RDWR);
        accel_z_offset_fd = open(mpu.in_accel_z_offset, O_RDWR);
        if (accel_x_offset_fd == -1 ||
                 accel_y_offset_fd == -1 || accel_z_offset_fd == -1) {
                LOGE("HAL:could not open factory accel calibrated bias");
        } else {
            LOGV_IF(EXTRA_VERBOSE,
                 "HAL:accel_offset opened");
        }
        
        /* open Accel Bias fd */
        /* mAccelBias contains bias that will be used for dmp */
        memset(mAccelBias, 0, sizeof(mAccelBias));
        LOGV_IF(EXTRA_VERBOSE, "HAL:accel x dmp bias path: %s", mpu.in_accel_x_dmp_bias);
        LOGV_IF(EXTRA_VERBOSE, "HAL:accel y dmp bias path: %s", mpu.in_accel_y_dmp_bias);
        LOGV_IF(EXTRA_VERBOSE, "HAL:accel z dmp bias path: %s", mpu.in_accel_z_dmp_bias);
        accel_x_dmp_bias_fd = open(mpu.in_accel_x_dmp_bias, O_RDWR);
        accel_y_dmp_bias_fd = open(mpu.in_accel_y_dmp_bias, O_RDWR);
        accel_z_dmp_bias_fd = open(mpu.in_accel_z_dmp_bias, O_RDWR);
        if (accel_x_dmp_bias_fd == -1 ||
                 accel_y_dmp_bias_fd == -1 || accel_z_dmp_bias_fd == -1) {
            LOGE("HAL:could not open accel DMP calibrated bias");
        } else {
            LOGV_IF(EXTRA_VERBOSE,
                 "HAL:accel_dmp_bias opened");
        }
    }

    dmp_sign_motion_fd = open(mpu.event_smd, O_RDONLY | O_NONBLOCK);
    if (dmp_sign_motion_fd < 0) {
        LOGE("HAL:ERR couldn't open dmp_sign_motion node");
    } else {
        LOGV_IF(ENG_VERBOSE, 
                "HAL:dmp_sign_motion_fd opened : %d", dmp_sign_motion_fd);
    }

    dmp_pedometer_fd = open(mpu.event_pedometer, O_RDONLY | O_NONBLOCK);
    if (dmp_pedometer_fd < 0) {
        LOGE("HAL:ERR couldn't open dmp_pedometer node");
    } else {
        LOGV_IF(ENG_VERBOSE, 
                "HAL:dmp_pedometer_fd opened : %d", dmp_pedometer_fd);
    }

    initBias();

    (void)inv_get_version(&ver_str);
    LOGI("%s\n", ver_str);

    /* setup MPL */
    inv_constructor_init();

#ifdef INV_PLAYBACK_DBG
    LOGV_IF(PROCESS_VERBOSE, "HAL:inv_turn_on_data_logging");
    logfile = fopen("/data/playback.bin", "w+");
    if (logfile)
        inv_turn_on_data_logging(logfile);
#endif

    /* setup orientation matrix and scale */
    inv_set_device_properties();

    /* initialize sensor data */
    memset(mPendingEvents, 0, sizeof(mPendingEvents));

    mPendingEvents[RotationVector].version = sizeof(sensors_event_t);
    mPendingEvents[RotationVector].sensor = ID_RV;
    mPendingEvents[RotationVector].type = SENSOR_TYPE_ROTATION_VECTOR;
    mPendingEvents[RotationVector].acceleration.status
            = SENSOR_STATUS_ACCURACY_HIGH;

    mPendingEvents[GameRotationVector].version = sizeof(sensors_event_t);
    mPendingEvents[GameRotationVector].sensor = ID_GRV;
    mPendingEvents[GameRotationVector].type = SENSOR_TYPE_GAME_ROTATION_VECTOR;
    mPendingEvents[GameRotationVector].acceleration.status
            = SENSOR_STATUS_ACCURACY_HIGH;

    mPendingEvents[LinearAccel].version = sizeof(sensors_event_t);
    mPendingEvents[LinearAccel].sensor = ID_LA;
    mPendingEvents[LinearAccel].type = SENSOR_TYPE_LINEAR_ACCELERATION;
    mPendingEvents[LinearAccel].acceleration.status
            = SENSOR_STATUS_ACCURACY_HIGH;

    mPendingEvents[Gravity].version = sizeof(sensors_event_t);
    mPendingEvents[Gravity].sensor = ID_GR;
    mPendingEvents[Gravity].type = SENSOR_TYPE_GRAVITY;
    mPendingEvents[Gravity].acceleration.status = SENSOR_STATUS_ACCURACY_HIGH;

    mPendingEvents[Gyro].version = sizeof(sensors_event_t);
    mPendingEvents[Gyro].sensor = ID_GY;
    mPendingEvents[Gyro].type = SENSOR_TYPE_GYROSCOPE;
    mPendingEvents[Gyro].gyro.status = SENSOR_STATUS_ACCURACY_HIGH;

    mPendingEvents[RawGyro].version = sizeof(sensors_event_t);
    mPendingEvents[RawGyro].sensor = ID_RG;
    mPendingEvents[RawGyro].type = SENSOR_TYPE_GYROSCOPE_UNCALIBRATED;
    mPendingEvents[RawGyro].gyro.status = SENSOR_STATUS_ACCURACY_HIGH;

    mPendingEvents[Accelerometer].version = sizeof(sensors_event_t);
    mPendingEvents[Accelerometer].sensor = ID_A;
    mPendingEvents[Accelerometer].type = SENSOR_TYPE_ACCELEROMETER;
    mPendingEvents[Accelerometer].acceleration.status
            = SENSOR_STATUS_ACCURACY_HIGH;

    /* Invensense compass calibration */
    mPendingEvents[MagneticField].version = sizeof(sensors_event_t);
    mPendingEvents[MagneticField].sensor = ID_M;
    mPendingEvents[MagneticField].type = SENSOR_TYPE_MAGNETIC_FIELD;
    mPendingEvents[MagneticField].magnetic.status =
        SENSOR_STATUS_ACCURACY_HIGH;

    mPendingEvents[RawMagneticField].version = sizeof(sensors_event_t);
    mPendingEvents[RawMagneticField].sensor = ID_RM;
    mPendingEvents[RawMagneticField].type = SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED;
    mPendingEvents[RawMagneticField].magnetic.status =
        SENSOR_STATUS_ACCURACY_HIGH;
        
    mPendingEvents[Pressure].version = sizeof(sensors_event_t);
    mPendingEvents[Pressure].sensor = ID_PS;
    mPendingEvents[Pressure].type = SENSOR_TYPE_PRESSURE;
    mPendingEvents[Pressure].magnetic.status =
        SENSOR_STATUS_ACCURACY_HIGH;

    mPendingEvents[Orientation].version = sizeof(sensors_event_t);
    mPendingEvents[Orientation].sensor = ID_O;
    mPendingEvents[Orientation].type = SENSOR_TYPE_ORIENTATION;
    mPendingEvents[Orientation].orientation.status
            = SENSOR_STATUS_ACCURACY_HIGH;

    mPendingEvents[GeomagneticRotationVector].version = sizeof(sensors_event_t);
    mPendingEvents[GeomagneticRotationVector].sensor = ID_GMRV;
    mPendingEvents[GeomagneticRotationVector].type
            = SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR;
    mPendingEvents[GeomagneticRotationVector].acceleration.status
            = SENSOR_STATUS_ACCURACY_HIGH;

#ifndef KLP
    mHandlers[RotationVector] = &MPLSensor::rvHandler;
#else
    mHandlers[RotationVector] = &MPLSensor::grvHandler;
#endif
    mHandlers[GameRotationVector] = &MPLSensor::grvHandler;
    mHandlers[LinearAccel] = &MPLSensor::laHandler;
    mHandlers[Gravity] = &MPLSensor::gravHandler;
#ifndef KLP
    mHandlers[Gyro] = &MPLSensor::gyroHandler;
#else
    mHandlers[Gyro] = &MPLSensor::rawGyroHandler;
#endif
    mHandlers[RawGyro] = &MPLSensor::rawGyroHandler;
    mHandlers[Accelerometer] = &MPLSensor::accelHandler;
#ifndef KLP
    mHandlers[MagneticField] = &MPLSensor::compassHandler;
#else
    mHandlers[MagneticField] = &MPLSensor::rawCompassHandler;
#endif
    mHandlers[RawMagneticField] = &MPLSensor::rawCompassHandler;
    mHandlers[Orientation] = &MPLSensor::orienHandler;
    mHandlers[GeomagneticRotationVector] = &MPLSensor::gmHandler;
    mHandlers[Pressure] = &MPLSensor::psHandler;

    for (int i = 0; i < NumSensors; i++) {
        mDelays[i] = 1000000000LL;
        mBatchDelays[i] = 1000000000LL;
        mBatchTimeouts[i] = 100000000000LL;
    }

    /* initialize Compass Bias */
    memset(mCompassBias, 0, sizeof(mCompassBias));

    /* initialize Factory Accel Bias */
    memset(mFactoryAccelBias, 0, sizeof(mFactoryAccelBias));

    /* initialize Gyro Bias */
    memset(mGyroBias, 0, sizeof(mGyroBias));
    memset(mGyroChipBias, 0, sizeof(mGyroChipBias));    

    /* load calibration file from /data/inv_cal_data.bin */
    rv = inv_load_calibration();    
    if(rv == INV_SUCCESS) {
        LOGV_IF(PROCESS_VERBOSE, "HAL:Calibration file successfully loaded");
        /* Get initial values */
        getCompassBias();
        getGyroBias();
        if (mGyroBiasAvailable) {
            setGyroBias();
        }
        getAccelBias();
        getFactoryGyroBias();
        if (mFactoryGyroBiasAvailable) {
            setFactoryGyroBias();
        }
        getFactoryAccelBias();
        if (mFactoryAccelBiasAvailable) {
            setFactoryAccelBias();
        }
    }
    else
        LOGE("HAL:Could not open or load MPL calibration file (%d)", rv);

    /* takes external accel calibration load workflow */
    if( m_pt2AccelCalLoadFunc != NULL) {
        long accel_offset[3];
        long tmp_offset[3];
        int result = m_pt2AccelCalLoadFunc(accel_offset);
        if(result)
            LOGW("HAL:Vendor accelerometer calibration file load failed %d\n",
                 result);
        else {
            LOGW("HAL:Vendor accelerometer calibration file successfully "
                 "loaded");
            inv_get_mpl_accel_bias(tmp_offset, NULL);
            LOGV_IF(PROCESS_VERBOSE,
                    "HAL:Original accel offset, %ld, %ld, %ld\n",
               tmp_offset[0], tmp_offset[1], tmp_offset[2]);
            inv_set_accel_bias_mask(accel_offset, mAccelAccuracy,4);
            inv_get_mpl_accel_bias(tmp_offset, NULL);
            LOGV_IF(PROCESS_VERBOSE, "HAL:Set accel offset, %ld, %ld, %ld\n",
               tmp_offset[0], tmp_offset[1], tmp_offset[2]);
        }
    }
    /* end of external accel calibration load workflow */   

    /* disable driver master enable the first sensor goes on */
    masterEnable(0);
    enableGyro(0);
    enableLowPowerAccel(0);
    enableAccel(0);
    enableCompass(0,0);
    enablePressure(0);
    enableBatch(0);

    if (isLowPowerQuatEnabled()) {
        enableLPQuaternion(0);
    }

    if (isDmpDisplayOrientationOn()) {
        // open DMP Orient Fd
        openDmpOrientFd();
        enableDmpOrientation(!isDmpScreenAutoRotationEnabled());
    }
}

void MPLSensor::enable_iio_sysfs(void)
{
    VFUNC_LOG;

    char iio_device_node[MAX_CHIP_ID_LEN];
    FILE *tempFp = NULL;

    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo 1 > %s (%lld)",
            mpu.in_timestamp_en, getTimestamp());
    // Either fopen()/open() are okay for sysfs access
    // developers could choose what they want
    // with fopen(), the benefit is that fprintf()/fscanf() are available
    tempFp = fopen(mpu.in_timestamp_en, "w");
    if (tempFp == NULL) {
        LOGE("HAL:could not open timestamp enable");
    } else {
        if(fprintf(tempFp, "%d", 1) < 0 || fclose(tempFp) < 0) {
            LOGE("HAL:could not enable timestamp");
        }
    }

    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
            IIO_BUFFER_LENGTH, mpu.buffer_length, getTimestamp());
    tempFp = fopen(mpu.buffer_length, "w");
    if (tempFp == NULL) {
        LOGE("HAL:could not open buffer length");
    } else {
        if (fprintf(tempFp, "%d", IIO_BUFFER_LENGTH) < 0 || fclose(tempFp) < 0) {
            LOGE("HAL:could not write buffer length");
        }
    }

    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
            1, mpu.chip_enable, getTimestamp());
    tempFp = fopen(mpu.chip_enable, "w");
    if (tempFp == NULL) {
        LOGE("HAL:could not open chip enable");
    } else {
        if (fprintf(tempFp, "%d", 1) < 0 || fclose(tempFp) < 0) {
            LOGE("HAL:could not write chip enable");
        }
    }
    
    inv_get_iio_device_node(iio_device_node);
    iio_fd = open(iio_device_node, O_RDONLY);
    if (iio_fd < 0) {
        LOGE("HAL:could not open iio device node");
    } else {
        LOGV_IF(ENG_VERBOSE, "HAL:iio iio_fd opened : %d", iio_fd);
    }
}

int MPLSensor::inv_constructor_init(void)
{
    VFUNC_LOG;

    inv_error_t result = inv_init_mpl();
    if (result) {
        LOGE("HAL:inv_init_mpl() failed");
        return result;
    }
    result = inv_constructor_default_enable();
    result = inv_start_mpl();
    if (result) {
        LOGE("HAL:inv_start_mpl() failed");
        LOG_RESULT_LOCATION(result);
        return result;
    }

    return result;
}

int MPLSensor::inv_constructor_default_enable(void)
{
    VFUNC_LOG;

    inv_error_t result;

/*******************************************************************************

********************************************************************************

The InvenSense binary file (libmplmpu.so) is subject to Google's standard terms
and conditions as accepted in the click-through agreement required to download
this library.
The library includes, but is not limited to the following function calls:
inv_enable_quaternion().

ANY VIOLATION OF SUCH TERMS AND CONDITIONS WILL BE STRICTLY ENFORCED.

********************************************************************************

*******************************************************************************/

    result = inv_enable_quaternion();
    if (result) {
        LOGE("HAL:Cannot enable quaternion\n");
        return result;
    }

    result = inv_enable_in_use_auto_calibration();
    if (result) {
        return result;
    }

    result = inv_enable_fast_nomot();
    if (result) {
        return result;
    }

    result = inv_enable_gyro_tc();
    if (result) {
        return result;
    }

    result = inv_enable_hal_outputs();
    if (result) {
        return result;
    }

    if (!mCompassSensor->providesCalibration()) {
        /* Invensense compass calibration */
        LOGV_IF(ENG_VERBOSE, "HAL:Invensense vector compass cal enabled");
        result = inv_enable_vector_compass_cal();
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        } else {
            mMplFeatureActiveMask |= INV_COMPASS_CAL;
        }
        // specify MPL's trust weight, used by compass algorithms
        inv_vector_compass_cal_sensitivity(3);

        /* disabled by default
        result = inv_enable_compass_bias_w_gyro();
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        */

        result = inv_enable_heading_from_gyro();
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }

        result = inv_enable_magnetic_disturbance();
        if (result) {
            LOG_RESULT_LOCATION(result);
            return result;
        }
        //inv_enable_magnetic_disturbance_logging();
    }

    result = inv_enable_9x_sensor_fusion();
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    } else {
        // 9x sensor fusion enables Compass fit
        mMplFeatureActiveMask |= INV_COMPASS_FIT;
    }

    result = inv_enable_no_gyro_fusion();
    if (result) {
        LOG_RESULT_LOCATION(result);
        return result;
    }

    return result;
}

/* TODO: create function pointers to calculate scale */
void MPLSensor::inv_set_device_properties(void)
{
    VFUNC_LOG;

    unsigned short orient;

    inv_get_sensors_orientation();

    inv_set_gyro_sample_rate(DEFAULT_MPL_GYRO_RATE);
    inv_set_compass_sample_rate(DEFAULT_MPL_COMPASS_RATE);

    /* gyro setup */
    orient = inv_orientation_matrix_to_scalar(mGyroOrientation);
    inv_set_gyro_orientation_and_scale(orient, mGyroScale << 15);
    LOGI_IF(EXTRA_VERBOSE, "HAL: Set MPL Gyro Scale %ld", mGyroScale << 15);

    /* accel setup */
    orient = inv_orientation_matrix_to_scalar(mAccelOrientation);
    /* use for third party accel input subsystem driver
    inv_set_accel_orientation_and_scale(orient, 1LL << 22);
    */
    inv_set_accel_orientation_and_scale(orient, (long)mAccelScale << 15);
    LOGI_IF(EXTRA_VERBOSE,
            "HAL: Set MPL Accel Scale %ld", (long)mAccelScale << 15);

    /* compass setup */
    signed char orientMtx[9];
    mCompassSensor->getOrientationMatrix(orientMtx);
    orient =
        inv_orientation_matrix_to_scalar(orientMtx);
    long sensitivity;
    sensitivity = mCompassSensor->getSensitivity();
    inv_set_compass_orientation_and_scale(orient, sensitivity);
    mCompassScale = sensitivity;
    LOGI_IF(EXTRA_VERBOSE,
            "HAL: Set MPL Compass Scale %ld", mCompassScale);
}

void MPLSensor::loadDMP(void)
{
    VFUNC_LOG;

    int res, fd;
    FILE *fptr;

    if (isMpuNonDmp()) {
        //DMP support only for MPU6xxx/9xxx currently
        return;
    }

    /* load DMP firmware */
    LOGV_IF(SYSFS_VERBOSE,
            "HAL:sysfs:cat %s (%lld)", mpu.firmware_loaded, getTimestamp());
    fd = open(mpu.firmware_loaded, O_RDONLY);
    if(fd < 0) {
        LOGE("HAL:could not open dmp state");
    } else {
        if(inv_read_dmp_state(fd) == 0) {
            LOGV_IF(EXTRA_VERBOSE, "HAL:load dmp: %s", mpu.dmp_firmware);
            fptr = fopen(mpu.dmp_firmware, "w");
            if(fptr == NULL) {
                LOGE("HAL:could not open dmp_firmware");
            } else {
                if (inv_load_dmp(fptr) < 0 || fclose(fptr) < 0) {
                    LOGE("HAL:load DMP failed");
                } else {
                    LOGV_IF(PROCESS_VERBOSE, "HAL:DMP loaded");
                }
            }
        } else {
            LOGV_IF(ENG_VERBOSE, "HAL:DMP is already loaded");
        }
    }

    // onDmp(1);    //Can't enable here. See note onDmp()
}

void MPLSensor::inv_get_sensors_orientation(void)
{
    VFUNC_LOG;

    FILE *fptr;

    // get gyro orientation
    LOGV_IF(SYSFS_VERBOSE,
            "HAL:sysfs:cat %s (%lld)", mpu.gyro_orient, getTimestamp());
    fptr = fopen(mpu.gyro_orient, "r");
    if (fptr != NULL) {
        int om[9];
        if (fscanf(fptr, "%d,%d,%d,%d,%d,%d,%d,%d,%d",
               &om[0], &om[1], &om[2], &om[3], &om[4], &om[5],
               &om[6], &om[7], &om[8]) < 0 || fclose(fptr) < 0) {
            LOGE("HAL:Could not read gyro mounting matrix");
        } else {
            LOGV_IF(EXTRA_VERBOSE,
                    "HAL:gyro mounting matrix: "
                    "%+d %+d %+d %+d %+d %+d %+d %+d %+d",
                    om[0], om[1], om[2], om[3], om[4], om[5], om[6], om[7], om[8]);

            mGyroOrientation[0] = om[0];
            mGyroOrientation[1] = om[1];
            mGyroOrientation[2] = om[2];
            mGyroOrientation[3] = om[3];
            mGyroOrientation[4] = om[4];
            mGyroOrientation[5] = om[5];
            mGyroOrientation[6] = om[6];
            mGyroOrientation[7] = om[7];
            mGyroOrientation[8] = om[8];
        }
    }

    // get accel orientation
    LOGV_IF(SYSFS_VERBOSE,
            "HAL:sysfs:cat %s (%lld)", mpu.accel_orient, getTimestamp());
    fptr = fopen(mpu.accel_orient, "r");
    if (fptr != NULL) {
        int om[9];
        if (fscanf(fptr, "%d,%d,%d,%d,%d,%d,%d,%d,%d",
            &om[0], &om[1], &om[2], &om[3], &om[4], &om[5],
            &om[6], &om[7], &om[8]) < 0 || fclose(fptr) < 0) {
            LOGE("HAL:could not read accel mounting matrix");
        } else {
            LOGV_IF(EXTRA_VERBOSE,
           "HAL:accel mounting matrix: "
           "%+d %+d %+d %+d %+d %+d %+d %+d %+d",
           om[0], om[1], om[2], om[3], om[4], om[5], om[6], om[7], om[8]);

           mAccelOrientation[0] = om[0];
           mAccelOrientation[1] = om[1];
           mAccelOrientation[2] = om[2];
           mAccelOrientation[3] = om[3];
           mAccelOrientation[4] = om[4];
           mAccelOrientation[5] = om[5];
           mAccelOrientation[6] = om[6];
           mAccelOrientation[7] = om[7];
           mAccelOrientation[8] = om[8];
        }
    }
}

MPLSensor::~MPLSensor()
{
    VFUNC_LOG;

    /* Close open fds */
    if (iio_fd > 0)
        close(iio_fd);
    if( accel_fd > 0 )
        close(accel_fd );
    if (gyro_temperature_fd > 0)
        close(gyro_temperature_fd);
    if (sysfs_names_ptr)
        free(sysfs_names_ptr);

    closeDmpOrientFd();

    if (accel_x_dmp_bias_fd > 0) {
        close(accel_x_dmp_bias_fd);
    }
    if (accel_y_dmp_bias_fd > 0) {
        close(accel_y_dmp_bias_fd);
    }
    if (accel_z_dmp_bias_fd > 0) {
        close(accel_z_dmp_bias_fd);
    }

    if (gyro_x_dmp_bias_fd > 0) {
        close(gyro_x_dmp_bias_fd);
    }
    if (gyro_y_dmp_bias_fd > 0) {
        close(gyro_y_dmp_bias_fd);
    }
    if (gyro_z_dmp_bias_fd > 0) {
        close(gyro_z_dmp_bias_fd);
    }

    if (gyro_x_offset_fd > 0) {
        close(gyro_x_dmp_bias_fd);
    }
    if (gyro_y_offset_fd > 0) {
        close(gyro_y_offset_fd);
    }
    if (gyro_z_offset_fd > 0) {
        close(accel_z_offset_fd);
    }

    /* Turn off Gyro master enable          */
    /* A workaround until driver handles it */
    /* TODO: Turn off and close all sensors */
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
            0, mpu.master_enable, getTimestamp());
    write_sysfs_int(mpu.master_enable, 0);

#ifdef INV_PLAYBACK_DBG
    inv_turn_off_data_logging();
    if (fclose(logfile) < 0) {
        LOGE("cannot close debug log file");
    }
#endif
}

#define GY_ENABLED  ((1 << ID_GY) & enabled_sensors)
#define RGY_ENABLED ((1 << ID_RG) & enabled_sensors)
#define A_ENABLED   ((1 << ID_A)  & enabled_sensors)
#define M_ENABLED   ((1 << ID_M) & enabled_sensors)
#define RM_ENABLED  ((1 << ID_RM) & enabled_sensors)
#define PS_ENABLED  ((1 << ID_PS) & enabled_sensors)
#define O_ENABLED   ((1 << ID_O)  & enabled_sensors)
#define LA_ENABLED  ((1 << ID_LA) & enabled_sensors)
#define GR_ENABLED  ((1 << ID_GR) & enabled_sensors)
#define RV_ENABLED  ((1 << ID_RV) & enabled_sensors)
#define GRV_ENABLED ((1 << ID_GRV) & enabled_sensors)
#define GMRV_ENABLED ((1 << ID_GMRV) & enabled_sensors)

/* TODO: this step is optional, remove?  */
int MPLSensor::setGyroInitialState(void)
{
    VFUNC_LOG;

    int res = 0;

    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
            HW_GYRO_RATE_HZ, mpu.gyro_rate, getTimestamp());
    int fd = open(mpu.gyro_rate, O_RDWR);
    res = errno;
    if(fd < 0) {
        LOGE("HAL:open of %s failed with '%s' (%d)",
             mpu.gyro_rate, strerror(res), res);
        return res;
    }
    res = write_attribute_sensor(fd, HW_GYRO_RATE_HZ);
    if(res < 0) {
        LOGE("HAL:write_attribute_sensor : error writing %s with %d",
             mpu.gyro_rate, HW_GYRO_RATE_HZ);
        return res;
    }

    // Setting LPF is deprecated

    return 0;
}

/* this applies to BMA250 Input Subsystem Driver only */
int MPLSensor::setAccelInitialState()
{
    VFUNC_LOG;

    struct input_absinfo absinfo_x;
    struct input_absinfo absinfo_y;
    struct input_absinfo absinfo_z;
    float value;
    if (!ioctl(accel_fd, EVIOCGABS(EVENT_TYPE_ACCEL_X), &absinfo_x) &&
        !ioctl(accel_fd, EVIOCGABS(EVENT_TYPE_ACCEL_Y), &absinfo_y) &&
        !ioctl(accel_fd, EVIOCGABS(EVENT_TYPE_ACCEL_Z), &absinfo_z)) {
        value = absinfo_x.value;
        mPendingEvents[Accelerometer].data[0] = value * CONVERT_A_X;
        value = absinfo_y.value;
        mPendingEvents[Accelerometer].data[1] = value * CONVERT_A_Y;
        value = absinfo_z.value;
        mPendingEvents[Accelerometer].data[2] = value * CONVERT_A_Z;
        //mHasPendingEvent = true;
    }
    return 0;
}

int MPLSensor::onDmp(int en)
{
    VFUNC_LOG;

    int res = -1;
    int status;
    mDmpOn = en;

    //Sequence to enable DMP
    //1. Load DMP image if not already loaded
    //2. Either Gyro or Accel must be enabled/configured before next step
    //3. Enable DMP

    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:cat %s (%lld)",
            mpu.firmware_loaded, getTimestamp());
    if(read_sysfs_int(mpu.firmware_loaded, &status) < 0){
        LOGE("HAL:ERR can't get firmware_loaded status");
    } else if (status == 1) {
        //Write only if curr DMP state <> request
        LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:cat %s (%lld)",
                mpu.dmp_on, getTimestamp());
        if (read_sysfs_int(mpu.dmp_on, &status) < 0) {
            LOGE("HAL:ERR can't read DMP state");
        } else if (status != en) {
            LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                    en, mpu.dmp_on, getTimestamp());
            if (write_sysfs_int(mpu.dmp_on, en) < 0) {
                LOGE("HAL:ERR can't write dmp_on");
            } else {
                mDmpOn = en;
                res = 0;    //Indicate write successful
                if(!en) {
                    setAccelBias();
                }
            }
            //Enable DMP interrupt
            LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                    en, mpu.dmp_int_on, getTimestamp());
            if (write_sysfs_int(mpu.dmp_int_on, en) < 0) {
                LOGE("HAL:ERR can't en/dis DMP interrupt");
            }

            // disable DMP event interrupt if needed
            if (!en) {
                LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                        en, mpu.dmp_event_int_on, getTimestamp());
                if (write_sysfs_int(mpu.dmp_event_int_on, en) < 0) {
                    res = -1;
                    LOGE("HAL:ERR can't enable DMP event interrupt");
                }
            }
        } else {
            mDmpOn = en;
            res = 0;    //DMP already set as requested
            if(!en) {
                setAccelBias();
            }
        }
    } else {
        LOGE("HAL:ERR No DMP image");
    }
    return res;
}

/* called when batch and hw sensor enabled*/
int MPLSensor::enablePedIndicator(int en)
{
    VFUNC_LOG;
    
    int res = 0;
    if (en) {
        if (!(mFeatureActiveMask & INV_DMP_PED_QUATERNION)) { 
            //Disable DMP Pedometer Interrupt
            LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                        0, mpu.pedometer_int_on, getTimestamp());
            if (write_sysfs_int(mpu.pedometer_int_on, 0) < 0) {
               LOGE("HAL:ERR can't enable Android Pedometer Interrupt");
               res = -1;   // indicate an err
               return res;
            }
        }
    }
    
    LOGV_IF(ENG_VERBOSE, "HAL:Toggling step indicator to %d", en);
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                en, mpu.step_indicator_on, getTimestamp());
    if (write_sysfs_int(mpu.step_indicator_on, en) < 0) {
        res = -1;
        LOGE("HAL:ERR can't write to DMP step_indicator_on");
    }
    return res;
}

int MPLSensor::checkPedStandaloneEnabled(void)
{
    VFUNC_LOG;
    return ((mFeatureActiveMask & INV_DMP_PED_STANDALONE)? 1:0);
}

/* This feature is only used in batch mode */
/* Stand-alone Step Detector */
int MPLSensor::enablePedStandalone(int en)
{
    VFUNC_LOG;

    if (!en) {
        enablePedStandaloneData(0);
        mFeatureActiveMask &= ~INV_DMP_PED_STANDALONE;
        if (mFeatureActiveMask == 0) {
            onDmp(0);
        } else if(mFeatureActiveMask & INV_DMP_PEDOMETER) {
             //Re-enable DMP Pedometer Interrupt
             LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                     1, mpu.pedometer_int_on, getTimestamp());
             if (write_sysfs_int(mpu.pedometer_int_on, 1) < 0) {
                 LOGE("HAL:ERR can't enable Android Pedometer Interrupt");
                 return (-1);
             }
            //Disable data interrupt if no continuous data
            if (mEnabled == 0) {
                LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                       1, mpu.dmp_event_int_on, getTimestamp());
                if (write_sysfs_int(mpu.dmp_event_int_on, 1) < 0) {
                    LOGE("HAL:ERR can't enable DMP event interrupt");
                    return (-1);
                }
            }
        }
        LOGV_IF(ENG_VERBOSE, "HAL:Ped Standalone disabled");
    } else {
        if (enablePedStandaloneData(1) < 0 || onDmp(1) < 0) {
            LOGE("HAL:ERR can't enable Ped Standalone");
        } else {
            mFeatureActiveMask |= INV_DMP_PED_STANDALONE;
            //Disable DMP Pedometer Interrupt
            LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                    0, mpu.pedometer_int_on, getTimestamp());
            if (write_sysfs_int(mpu.pedometer_int_on, 0) < 0) {
                LOGE("HAL:ERR can't disable Android Pedometer Interrupt");
                return (-1);
            }
            //Enable Data Interrupt
            LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                       0, mpu.dmp_event_int_on, getTimestamp());
            if (write_sysfs_int(mpu.dmp_event_int_on, 0) < 0) {
                LOGE("HAL:ERR can't enable DMP event interrupt");
                return (-1);
            }
            LOGV_IF(ENG_VERBOSE, "HAL:Ped Standalone enabled");
        }
    }
    return 0;
}

int MPLSensor:: enablePedStandaloneData(int en)
{
    VFUNC_LOG;

    int res = 0;

    // Enable DMP Ped standalone
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
            en, mpu.step_detector_on, getTimestamp());
    if (write_sysfs_int(mpu.step_detector_on, en) < 0) {
        LOGE("HAL:ERR can't write DMP step_detector_on");
        res = -1;   //Indicate an err
    }

    // Disable DMP Step indicator
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
            en, mpu.step_indicator_on, getTimestamp());
    if (write_sysfs_int(mpu.step_indicator_on, en) < 0) {
        LOGE("HAL:ERR can't write DMP step_indicator_on");
        res = -1;   //Indicate an err
    }

    if (!en) {
      LOGV_IF(ENG_VERBOSE, "HAL:Disabling ped standalone");
      //Disable Accel if no sensor needs it
      if (!(mFeatureActiveMask & DMP_FEATURE_MASK)
                               && (!(mLocalSensorMask & mMasterSensorMask
                                                   & INV_THREE_AXIS_ACCEL))) {
          res = enableAccel(0);
          if (res < 0)
              return res;
      }
      if (!(mFeatureActiveMask & DMP_FEATURE_MASK)
                               && (!(mLocalSensorMask & mMasterSensorMask
                                                   & INV_THREE_AXIS_GYRO))) {
          res = enableGyro(0);
          if (res < 0)
              return res;
      }
    } else {
        LOGV_IF(ENG_VERBOSE, "HAL:Enabling ped standalone");
        // enable accel engine
        res = enableAccel(1);
        if (res < 0)
            return res;
        LOGV_IF(EXTRA_VERBOSE, "mLocalSensorMask=0x%lx", mLocalSensorMask);
        // disable accel FIFO
        if (!((mLocalSensorMask & mMasterSensorMask) & INV_THREE_AXIS_ACCEL)) {
            res = turnOffAccelFifo();
            if (res < 0)
                return res;           
        }
    }

    return res;
}

int MPLSensor::checkPedQuatEnabled(void)
{
    VFUNC_LOG;
    return ((mFeatureActiveMask & INV_DMP_PED_QUATERNION)? 1:0);
}

/* This feature is only used in batch mode */
/* Step Detector && Game Rotation Vector */
int MPLSensor::enablePedQuaternion(int en)
{
    VFUNC_LOG;

    if (!en) {
        enablePedQuaternionData(0);
        mFeatureActiveMask &= ~INV_DMP_PED_QUATERNION;
        if (mFeatureActiveMask == 0) {
            onDmp(0);
        } else if(mFeatureActiveMask & INV_DMP_PEDOMETER) {
             //Re-enable DMP Pedometer Interrupt
             LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                     1, mpu.pedometer_int_on, getTimestamp());
             if (write_sysfs_int(mpu.pedometer_int_on, 1) < 0) {
                 LOGE("HAL:ERR can't enable Android Pedometer Interrupt");
                 return (-1);
             }
            //Disable data interrupt if no continuous data
            if (mEnabled == 0) {
                LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                       1, mpu.dmp_event_int_on, getTimestamp());
                if (write_sysfs_int(mpu.dmp_event_int_on, en) < 0) {
                    LOGE("HAL:ERR can't enable DMP event interrupt");
                    return (-1);
                }
            }
        }
        LOGV_IF(ENG_VERBOSE, "HAL:Ped Quat disabled");
    } else {
        if (enablePedQuaternionData(1) < 0 || onDmp(1) < 0) {
            LOGE("HAL:ERR can't enable Ped Quaternion");
        } else {
            mFeatureActiveMask |= INV_DMP_PED_QUATERNION;
            //Disable DMP Pedometer Interrupt
            LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                    0, mpu.pedometer_int_on, getTimestamp());
            if (write_sysfs_int(mpu.pedometer_int_on, 0) < 0) {
                LOGE("HAL:ERR can't disable Android Pedometer Interrupt");
                return (-1);
            }
            //Enable Data Interrupt
            LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                       0, mpu.dmp_event_int_on, getTimestamp());
            if (write_sysfs_int(mpu.dmp_event_int_on, 0) < 0) {
                LOGE("HAL:ERR can't enable DMP event interrupt");
                return (-1);
            }
            LOGV_IF(ENG_VERBOSE, "HAL:Ped Quat enabled");
        }
    }
    return 0;
}

int MPLSensor::enablePedQuaternionData(int en)
{
    VFUNC_LOG;

    int res = 0;

    // Enable DMP quaternion
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
            en, mpu.ped_q_on, getTimestamp());
    if (write_sysfs_int(mpu.ped_q_on, en) < 0) {
        LOGE("HAL:ERR can't write DMP ped_q_on");
        res = -1;   //Indicate an err
    }

    // toggle DMP step indicator
    /*LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
            en, mpu.step_indicator_on, getTimestamp());
    if (write_sysfs_int(mpu.step_indicator_on, en) < 0) {
        LOGE("HAL:ERR can't write DMP step_indicator_on");
        res = -1;   //Indicate an err
    }*/

    if (!en) {
        LOGV_IF(ENG_VERBOSE, "HAL:Disabling ped quat");
        //Disable Accel if no sensor needs it
        if (!(mFeatureActiveMask & DMP_FEATURE_MASK)
                               && (!(mLocalSensorMask & mMasterSensorMask
                                                   & INV_THREE_AXIS_ACCEL))) {
          res = enableAccel(0);
          if (res < 0)
              return res;
        }
        if (!(mFeatureActiveMask & DMP_FEATURE_MASK)
                               && (!(mLocalSensorMask & mMasterSensorMask
                                                   & INV_THREE_AXIS_GYRO))) {
            res = enableGyro(0);
            if (res < 0)
                return res;
        }
        if (mFeatureActiveMask & INV_DMP_QUATERNION) {
            res = write_sysfs_int(mpu.gyro_fifo_enable, 1);
            res += write_sysfs_int(mpu.accel_fifo_enable, 1);
            if (res < 0)
                return res;
        }
        //LOGV_IF(ENG_VERBOSE, "before mLocalSensorMask=0x%lx", mLocalSensorMask);
        // reset global mask for buildMpuEvent()
        if (mEnabled & (1 << GameRotationVector)) {
            mLocalSensorMask |= INV_THREE_AXIS_GYRO;
            mLocalSensorMask |= INV_THREE_AXIS_ACCEL;
        } else if (mEnabled & (1 << Accelerometer)) {
            mLocalSensorMask |= INV_THREE_AXIS_ACCEL;
        } else if ((mEnabled & ( 1 << Gyro)) ||
            (mEnabled & (1 << RawGyro))) {
            mLocalSensorMask |= INV_THREE_AXIS_GYRO; 
        }
        //LOGV_IF(ENG_VERBOSE, "after mLocalSensorMask=0x%lx", mLocalSensorMask); 
    } else {
        LOGV_IF(PROCESS_VERBOSE, "HAL:Enabling ped quat");
        // enable accel engine
        res = enableAccel(1);
        if (res < 0)
            return res;

        // enable gyro engine
        res = enableGyro(1);
        if (res < 0)
            return res;
        LOGV_IF(EXTRA_VERBOSE, "mLocalSensorMask=0x%lx", mLocalSensorMask);
        // disable accel FIFO
        if ((!((mLocalSensorMask & mMasterSensorMask) & INV_THREE_AXIS_ACCEL)) ||
                !(mBatchEnabled & (1 << Accelerometer))) {
            res = turnOffAccelFifo();
            if (res < 0)
                return res;
            mLocalSensorMask &= ~INV_THREE_AXIS_ACCEL;
        }

        // disable gyro FIFO
        if ((!((mLocalSensorMask & mMasterSensorMask) & INV_THREE_AXIS_GYRO)) ||
                !((mBatchEnabled & (1 << Gyro)) || (mBatchEnabled & (1 << RawGyro)))) {
            res = turnOffGyroFifo();
            if (res < 0)
                return res;
            mLocalSensorMask &= ~INV_THREE_AXIS_GYRO;
        }
    }

    return res;
}

int MPLSensor::check6AxisQuatEnabled(void)
{
    VFUNC_LOG;
    return ((mFeatureActiveMask & INV_DMP_6AXIS_QUATERNION)? 1:0);
}

/* This is used for batch mode only */
/* GRV is batched but not along with ped */
int MPLSensor::enable6AxisQuaternion(int en)
{
    VFUNC_LOG;

    if (!en) {
        enable6AxisQuaternionData(0);
        mFeatureActiveMask &= ~INV_DMP_6AXIS_QUATERNION;
        if (mFeatureActiveMask == 0) {
            onDmp(0);
        }
        LOGV_IF(ENG_VERBOSE, "HAL:6 Axis Quat disabled");
    } else {
        if (enable6AxisQuaternionData(1) < 0 || onDmp(1) < 0) {
            LOGE("HAL:ERR can't enable 6 Axis Quaternion");
        } else {
            mFeatureActiveMask |= INV_DMP_6AXIS_QUATERNION;
            LOGV_IF(PROCESS_VERBOSE, "HAL:6 Axis Quat enabled");
        }
    }
    return 0;
}

int MPLSensor::enable6AxisQuaternionData(int en)
{
    VFUNC_LOG;

    int res = 0;

    // Enable DMP quaternion
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
            en, mpu.six_axis_q_on, getTimestamp());
    if (write_sysfs_int(mpu.six_axis_q_on, en) < 0) {
        LOGE("HAL:ERR can't write DMP six_axis_q_on");
        res = -1;   //Indicate an err
    }

    if (!en) {
        LOGV_IF(EXTRA_VERBOSE, "HAL:DMP six axis quaternion data was turned off");
        inv_quaternion_sensor_was_turned_off();
        if (!(mFeatureActiveMask & DMP_FEATURE_MASK)
                                 && (!(mLocalSensorMask & mMasterSensorMask
                                                     & INV_THREE_AXIS_ACCEL))) {
            res = enableAccel(0);
            if (res < 0)
                return res;
        }
        if (!(mFeatureActiveMask & DMP_FEATURE_MASK)
                                 && (!(mLocalSensorMask & mMasterSensorMask
                                                     & INV_THREE_AXIS_GYRO))) {
            res = enableGyro(0);
            if (res < 0)
                return res;
        }
        if (mFeatureActiveMask & INV_DMP_QUATERNION) {
            LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                    1, mpu.gyro_fifo_enable, getTimestamp());
            res = write_sysfs_int(mpu.gyro_fifo_enable, 1);
            LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                    1, mpu.accel_fifo_enable, getTimestamp());
            res += write_sysfs_int(mpu.accel_fifo_enable, 1);
            if (res < 0)
                return res;
        }
        LOGV_IF(ENG_VERBOSE, "  k=0x%lx", mLocalSensorMask);
        // reset global mask for buildMpuEvent()
        if (mEnabled & (1 << GameRotationVector)) {
            if (!(mFeatureActiveMask & INV_DMP_PED_QUATERNION)) {
                mLocalSensorMask |= INV_THREE_AXIS_GYRO;        
                mLocalSensorMask |= INV_THREE_AXIS_ACCEL;
                res = write_sysfs_int(mpu.gyro_fifo_enable, 1);
                res += write_sysfs_int(mpu.accel_fifo_enable, 1);
                if (res < 0)
                    return res;
            }
        } else if (mEnabled & (1 << Accelerometer)) {
            mLocalSensorMask |= INV_THREE_AXIS_ACCEL;
        } else if ((mEnabled & ( 1 << Gyro)) ||
                (mEnabled & (1 << RawGyro))) {
            mLocalSensorMask |= INV_THREE_AXIS_GYRO; 
        }
        LOGV_IF(ENG_VERBOSE, "after mLocalSensorMask=0x%lx", mLocalSensorMask);
    } else {
        LOGV_IF(PROCESS_VERBOSE, "HAL:Enabling six axis quat");
        if (mEnabled & ( 1 << GameRotationVector)) {
            // enable accel engine
            res = enableAccel(1);
            if (res < 0)
                return res;

            // enable gyro engine
            res = enableGyro(1);
            if (res < 0)
                return res;
            LOGV_IF(EXTRA_VERBOSE, "before: mLocalSensorMask=0x%lx", mLocalSensorMask);
            if ((!(mLocalSensorMask & mMasterSensorMask & INV_THREE_AXIS_ACCEL)) ||
                   (!(mBatchEnabled & (1 << Accelerometer)) ||
                       (!(mEnabled & (1 << Accelerometer))))) {
                res = turnOffAccelFifo();
                if (res < 0)
                    return res;
                mLocalSensorMask &= ~INV_THREE_AXIS_ACCEL;
            }
            
            if ((!(mLocalSensorMask & mMasterSensorMask & INV_THREE_AXIS_GYRO)) ||
                    (!(mBatchEnabled & (1 << Gyro)) ||
                       (!(mEnabled & (1 << Gyro))))) {
                if (!(mBatchEnabled & (1 << RawGyro)) ||
                        (!(mEnabled & (1 << RawGyro)))) {
                    res = turnOffGyroFifo();
                    if (res < 0)
                        return res;
                     mLocalSensorMask &= ~INV_THREE_AXIS_GYRO;
                     }
            }
            LOGV_IF(ENG_VERBOSE, "after: mLocalSensorMask=0x%lx", mLocalSensorMask);
        }
    }

    return res;
}

/* this is for batch  mode only */
int MPLSensor::checkLPQRateSupported(void)
{
    VFUNC_LOG;
    return ((mDelays[GameRotationVector] <= RATE_200HZ) ? 0 :1);
}

int MPLSensor::checkLPQuaternion(void)
{
    VFUNC_LOG;
    return ((mFeatureActiveMask & INV_DMP_QUATERNION)? 1:0);
}

int MPLSensor::enableLPQuaternion(int en)
{
    VFUNC_LOG;

    if (!en) {
        enableQuaternionData(0);
        mFeatureActiveMask &= ~INV_DMP_QUATERNION;
        if (mFeatureActiveMask == 0) {
            onDmp(0);
        }
        LOGV_IF(ENG_VERBOSE, "HAL:LP Quat disabled");
    } else {
        if (enableQuaternionData(1) < 0 || onDmp(1) < 0) {
            LOGE("HAL:ERR can't enable LP Quaternion");
        } else {
            mFeatureActiveMask |= INV_DMP_QUATERNION;
            LOGV_IF(ENG_VERBOSE, "HAL:LP Quat enabled");
        }
    }
    return 0;
}

int MPLSensor::enableQuaternionData(int en)
{
    VFUNC_LOG;

    int res = 0;

    // Enable DMP quaternion
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
            en, mpu.three_axis_q_on, getTimestamp());
    if (write_sysfs_int(mpu.three_axis_q_on, en) < 0) {
        LOGE("HAL:ERR can't write DMP three_axis_q__on");
        res = -1;   //Indicates an err
    }

    if (!en) {
        LOGV_IF(ENG_VERBOSE, "HAL:DMP quaternion data was turned off");
        inv_quaternion_sensor_was_turned_off();
    } else {
        LOGV_IF(ENG_VERBOSE, "HAL:Enabling three axis quat");
    }

    return res;
}

int MPLSensor::enableDmpPedometer(int en, int interruptMode)
{
    VFUNC_LOG;
    int res = 0;
    int enabled_sensors = mEnabled;

    if (isMpuNonDmp())
        return res;

    // reset master enable
    res = masterEnable(0);
    if (res < 0) {
        return res;
    }

    if (en == 1) {
        //Enable DMP Pedometer Function
        LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                en, mpu.pedometer_on, getTimestamp());
        if (write_sysfs_int(mpu.pedometer_on, en) < 0) {
            LOGE("HAL:ERR can't enable Android Pedometer");
            res = -1;   // indicate an err
            return res;
        }

        if (interruptMode || (mFeatureActiveMask & INV_DMP_PEDOMETER)) {
            //Enable DMP Pedometer Interrupt
            LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                    en, mpu.pedometer_int_on, getTimestamp());
            if (write_sysfs_int(mpu.pedometer_int_on, en) < 0) {
                LOGE("HAL:ERR can't enable Android Pedometer Interrupt");
                res = -1;   // indicate an err
                return res;
            }
        }
        // enable DMP
        res = onDmp(1);
        if (res < 0) {
            return res;
        }  

        // set DMP rate to 200Hz
        LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                200, mpu.accel_fifo_rate, getTimestamp());
        if (write_sysfs_int(mpu.accel_fifo_rate, 200) < 0) {
            res = -1;
            LOGE("HAL:ERR can't set rate to 200Hz");
            return res;
        }

        // enable accel engine
        res = enableAccel(1);
        if (res < 0) {
            return res;
        }

        // disable accel FIFO
        if (!(mLocalSensorMask & mMasterSensorMask & INV_THREE_AXIS_ACCEL)) {
            res = turnOffAccelFifo();
            if (res < 0)
                return res;
        }
        
        // disable data interrupt      
        //if (!batchPed && enabled_sensors == 0) {
        if (enabled_sensors == 0) {
            LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                        en, mpu.dmp_event_int_on, getTimestamp());
            if (write_sysfs_int(mpu.dmp_event_int_on, en) < 0) {
                res = -1;
                LOGE("HAL:ERR can't enable DMP event interrupt");
            }
        }
        if (interruptMode) {
            mFeatureActiveMask |= INV_DMP_PEDOMETER;
        }
        else {
            mFeatureActiveMask |= INV_DMP_PEDOMETER_STEP;
            mStepCountPollTime = 1000;
        }

        clock_gettime(CLOCK_MONOTONIC, &mt_pre);
    } else {
        if (interruptMode) {
            mFeatureActiveMask &= ~INV_DMP_PEDOMETER;
        }
        else {
            mFeatureActiveMask &= ~INV_DMP_PEDOMETER_STEP;
            mStepCountPollTime = -1;
        }

        /* if neither step detector or step count is on */
        if (!(mFeatureActiveMask & (INV_DMP_PEDOMETER | INV_DMP_PEDOMETER_STEP))) {
            //Disable DMP Pedometer Function
            LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                    en, mpu.pedometer_on, getTimestamp());
            if (write_sysfs_int(mpu.pedometer_on, en) < 0) {
                LOGE("HAL:ERR can't enable Android Pedometer");
                res = -1;   // indicate an err
                return res;
            }        
        }       

        if (mFeatureActiveMask == 0 ) {
            // disable DMP
            res = onDmp(0);
            if (res < 0) {
                return res;
            }

             // disable accel engine
             if (!(mLocalSensorMask & mMasterSensorMask
                        & INV_THREE_AXIS_ACCEL)) {
                res = enableAccel(0);
                if (res < 0) {
                    return res;
                }
            }
        }

        /* if feature is not step detector */
        if (!(mFeatureActiveMask & INV_DMP_PEDOMETER)) { 
            //Disable DMP Pedometer Interrupt
            LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                    en, mpu.pedometer_int_on, getTimestamp());
            if (write_sysfs_int(mpu.pedometer_int_on, en) < 0) {
                LOGE("HAL:ERR can't enable Android Pedometer Interrupt");
                res = -1;   // indicate an err
                return res;
            }
        }

        //enable data interrupts if applicable
        if (enabled_sensors) {
            LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                        en, mpu.dmp_event_int_on, getTimestamp());
            if (write_sysfs_int(mpu.dmp_event_int_on, en) < 0) {
                res = -1;
                LOGE("HAL:ERR can't enable DMP event interrupt");
            }
        }
    }
    
    if(en || enabled_sensors || mFeatureActiveMask) {
        res = masterEnable(1);
    }
    return res;
}

int MPLSensor::masterEnable(int en)
{
    VFUNC_LOG;

    int res = 0;
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
            en, mpu.master_enable, getTimestamp());
    res = write_sysfs_int(mpu.master_enable, en);
    return res;
}

int MPLSensor::enableGyro(int en)
{
    VFUNC_LOG;

    int res = 0;

    /* need to also turn on/off the master enable */
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
            en, mpu.gyro_enable, getTimestamp());
    res = write_sysfs_int(mpu.gyro_enable, en);
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
            en, mpu.gyro_fifo_enable, getTimestamp());
    res += write_sysfs_int(mpu.gyro_fifo_enable, en);

    if (!en) {
        LOGV_IF(EXTRA_VERBOSE, "HAL:MPL:inv_gyro_was_turned_off");
        inv_gyro_was_turned_off();
    }

    return res;
}

int MPLSensor::enableLowPowerAccel(int en)
{
    VFUNC_LOG;
    
    int res;
    
    /* need to also turn on/off the master enable */
    res = write_sysfs_int(mpu.motion_lpa_on, en);
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                        en, mpu.motion_lpa_on, getTimestamp());  
    return res;  
}

int MPLSensor::enableAccel(int en)
{
    VFUNC_LOG;

    int res;

    /* need to also turn on/off the master enable */
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
            en, mpu.accel_enable, getTimestamp());
    res = write_sysfs_int(mpu.accel_enable, en);
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
            en, mpu.accel_fifo_enable, getTimestamp());
    res += write_sysfs_int(mpu.accel_fifo_enable, en);

    if (!en) {
        LOGV_IF(EXTRA_VERBOSE, "HAL:MPL:inv_accel_was_turned_off");
        inv_accel_was_turned_off();
    }

    return res;
}

int MPLSensor::enableCompass(int en, int rawSensorRequested)
{
    VFUNC_LOG;

    int res = 0;
    /* handle ID_RM if third party compass cal is used */
    if (rawSensorRequested && mCompassSensor->providesCalibration()) {
        res = mCompassSensor->enable(ID_RM, en);
    } else {
        res = mCompassSensor->enable(ID_M, en);
    }
    if (en == 0 || res != 0) {
        LOGV_IF(EXTRA_VERBOSE, "HAL:MPL:inv_compass_was_turned_off %d", res);
        inv_compass_was_turned_off();
    }

    return res;
}

int MPLSensor::enablePressure(int en)
{
    VFUNC_LOG;

    int res = 0;

    if (mPressureSensor) 
        res = mPressureSensor->enable(ID_PS, en);
    
    return res;
}

/* use this function for initialization */
int MPLSensor::enableBatch(int64_t timeout)
{
    VFUNC_LOG;

    int res = 0;

    res = write_sysfs_int(mpu.batchmode_timeout, timeout);
    if (timeout == 0) {
        res = write_sysfs_int(mpu.six_axis_q_on, 0);
        res = write_sysfs_int(mpu.ped_q_on, 0);
        res = write_sysfs_int(mpu.step_detector_on, 0);
        res = write_sysfs_int(mpu.step_indicator_on, 0);
    }

    if (timeout == 0) {
        LOGV_IF(EXTRA_VERBOSE, "HAL:MPL:batchmode timeout is zero");      
    }

    return res;
}

void MPLSensor::computeLocalSensorMask(int enabled_sensors)
{
    VFUNC_LOG;

    do {
        /* Invensense Pressure on secondary bus */
        if (PS_ENABLED) {
            LOGV_IF(ENG_VERBOSE, "PS ENABLED");
            mLocalSensorMask |= INV_ONE_AXIS_PRESSURE;
        } else {
            LOGV_IF(ENG_VERBOSE, "PS DISABLED");
            mLocalSensorMask &= ~INV_ONE_AXIS_PRESSURE;
        }
        
        if (LA_ENABLED || GR_ENABLED || RV_ENABLED || O_ENABLED
                       || (GRV_ENABLED && GMRV_ENABLED)) {
            LOGV_IF(ENG_VERBOSE, "FUSION ENABLED");
            mLocalSensorMask |= ALL_MPL_SENSORS_NP;
            break;
        }

        if (GRV_ENABLED) {
            if (!(mBatchEnabled & (1 << GameRotationVector))) {
                LOGV_IF(ENG_VERBOSE, "6 Axis Fusion ENABLED");
                mLocalSensorMask |= INV_THREE_AXIS_GYRO;
                mLocalSensorMask |= INV_THREE_AXIS_ACCEL;
            } else {
                if (GY_ENABLED || RGY_ENABLED) {
                    LOGV_IF(ENG_VERBOSE, "G ENABLED");
                    mLocalSensorMask |= INV_THREE_AXIS_GYRO;
                } else {
                    LOGV_IF(ENG_VERBOSE, "G DISABLED");
                    mLocalSensorMask &= ~INV_THREE_AXIS_GYRO;
                }
                if (A_ENABLED) {
                    LOGV_IF(ENG_VERBOSE, "A ENABLED");
                    mLocalSensorMask |= INV_THREE_AXIS_ACCEL;
                } else {
                    LOGV_IF(ENG_VERBOSE, "A DISABLED");
                    mLocalSensorMask &= ~INV_THREE_AXIS_ACCEL;
                }
            }
            /* takes care of MAG case */
            if (M_ENABLED || RM_ENABLED) {
                LOGV_IF(1, "M ENABLED");
                mLocalSensorMask |= INV_THREE_AXIS_COMPASS;
            } else {
                LOGV_IF(1, "M DISABLED");
                mLocalSensorMask &= ~INV_THREE_AXIS_COMPASS;
            }
            break;
        }

        if (GMRV_ENABLED) {
            LOGV_IF(ENG_VERBOSE, "6 Axis Geomagnetic Fusion ENABLED");
            mLocalSensorMask |= INV_THREE_AXIS_ACCEL;
            mLocalSensorMask |= INV_THREE_AXIS_COMPASS;

            /* takes care of Gyro case */
            if (GY_ENABLED || RGY_ENABLED) {
                LOGV_IF(1, "G ENABLED");
                mLocalSensorMask |= INV_THREE_AXIS_GYRO;
            } else {
                LOGV_IF(1, "G DISABLED");
                mLocalSensorMask &= ~INV_THREE_AXIS_GYRO;
            }
            break;
        }

        if(!A_ENABLED && !M_ENABLED && !RM_ENABLED &&
               !GRV_ENABLED && !GMRV_ENABLED && !GY_ENABLED && !RGY_ENABLED &&
               !PS_ENABLED) {
            /* Invensense compass cal */
            LOGV_IF(ENG_VERBOSE, "ALL DISABLED");
            mLocalSensorMask = 0;
            break;
        }

        if (GY_ENABLED || RGY_ENABLED) {
            LOGV_IF(ENG_VERBOSE, "G ENABLED");
            mLocalSensorMask |= INV_THREE_AXIS_GYRO;
        } else {
            LOGV_IF(ENG_VERBOSE, "G DISABLED");
            mLocalSensorMask &= ~INV_THREE_AXIS_GYRO;
        }

        if (A_ENABLED) {
            LOGV_IF(ENG_VERBOSE, "A ENABLED");
            mLocalSensorMask |= INV_THREE_AXIS_ACCEL;
        } else {
            LOGV_IF(ENG_VERBOSE, "A DISABLED");
            mLocalSensorMask &= ~INV_THREE_AXIS_ACCEL;
        }

        /* Invensense compass calibration */
        if (M_ENABLED || RM_ENABLED) {
            LOGV_IF(ENG_VERBOSE, "M ENABLED");
            mLocalSensorMask |= INV_THREE_AXIS_COMPASS;
        } else {
            LOGV_IF(ENG_VERBOSE, "M DISABLED");
            mLocalSensorMask &= ~INV_THREE_AXIS_COMPASS;
        }        
    } while (0);
}

int MPLSensor::enableSensors(unsigned long sensors, int en, uint32_t changed)
{
    VFUNC_LOG;

    inv_error_t res = -1;
    int on = 1;
    int off = 0;
    int cal_stored = 0;

    // Sequence to enable or disable a sensor
    // 1. reset master enable (=0)
    // 2. enable or disable a sensor
    // 3. set master enable (=1)

    if (isLowPowerQuatEnabled() ||
        changed & ((1 << Gyro) | (1 << RawGyro) | (1 << Accelerometer) |
        (mCompassSensor->isIntegrated() << MagneticField) |
        (mCompassSensor->isIntegrated() << RawMagneticField) |
        (mPressureSensor->isIntegrated() << Pressure))) {

        /* reset master enable */
        res = masterEnable(0);
        if(res < 0) {
            return res;
        }
    }

    LOGV_IF(ENG_VERBOSE, "HAL:enableSensors - sensors: 0x%0x",
            (unsigned int)sensors);

    if (changed & ((1 << Gyro) | (1 << RawGyro))) {
        LOGV_IF(ENG_VERBOSE, "HAL:enableSensors - gyro %s",
                (sensors & INV_THREE_AXIS_GYRO? "enable": "disable"));
        res = enableGyro(!!(sensors & INV_THREE_AXIS_GYRO));
        if(res < 0) {
            return res;
        }

        if (!cal_stored && (!en && (changed & (1 << Gyro)))) {
            storeCalibration();
            cal_stored = 1;
        }
    }

    if (changed & (1 << Accelerometer)) {
        LOGV_IF(ENG_VERBOSE, "HAL:enableSensors - accel %s",
                (sensors & INV_THREE_AXIS_ACCEL? "enable": "disable"));
        res = enableAccel(!!(sensors & INV_THREE_AXIS_ACCEL));
        if(res < 0) {
            return res;
        }

        if (!(sensors & INV_THREE_AXIS_ACCEL) && !cal_stored) {
            storeCalibration();
            cal_stored = 1;
        }
    }

    if (changed & ((1 << MagneticField) | (1 << RawMagneticField))) {
        LOGV_IF(ENG_VERBOSE, "HAL:enableSensors - compass %s",
                (sensors & INV_THREE_AXIS_COMPASS? "enable": "disable"));
        res = enableCompass(!!(sensors & INV_THREE_AXIS_COMPASS), changed & (1 << RawMagneticField));
        if(res < 0) {
            return res;
        }

        if (!cal_stored && (!en && (changed & (1 << MagneticField)))) {
            storeCalibration();
            cal_stored = 1;
        }
    }
    
     if (changed & (1 << Pressure)) {
        LOGV_IF(ENG_VERBOSE, "HAL:enableSensors - pressure %s",
                (sensors & INV_ONE_AXIS_PRESSURE? "enable": "disable"));
        res = enablePressure(!!(sensors & INV_ONE_AXIS_PRESSURE));
        if(res < 0) {
            return res;
        }
    }

    if (isLowPowerQuatEnabled()) {
        // Enable LP Quat
        if ((mEnabled & VIRTUAL_SENSOR_9AXES_MASK)
                      || (mEnabled & VIRTUAL_SENSOR_GYRO_6AXES_MASK)) {
            LOGV_IF(ENG_VERBOSE, "HAL: 9 axis or game rot enabled");
            if (!(changed & ((1 << Gyro)
                           | (1 << RawGyro)
                           | (1 << Accelerometer)
                           | (mCompassSensor->isIntegrated() << MagneticField)
                           | (mCompassSensor->isIntegrated() << RawMagneticField)))
            ) {
                /* reset master enable */
                res = masterEnable(0);
                if(res < 0) {
                    return res;
                }
            }
            if (!checkLPQuaternion()) {
                enableLPQuaternion(1);
            } else {
                LOGV_IF(ENG_VERBOSE, "HAL:LP Quat already enabled");
            }
        } else if (checkLPQuaternion()) {
            enableLPQuaternion(0);
        }
    }

    /* apply accel/gyro bias to DMP bias                        */
    /* precondition: masterEnable(0), mGyroBiasAvailable=true   */
    /* postcondition: bias is applied upon masterEnable(1)      */
    if(!(sensors & INV_THREE_AXIS_GYRO)) {
        setGyroBias();
    }
    if(!(sensors & INV_THREE_AXIS_ACCEL)) {
        setAccelBias();
    }

    /* to batch or not to batch */
    int batchMode = computeBatchSensorMask(mEnabled, mBatchEnabled);
    /* skip setBatch if there is no need to */
    if(((int)mOldBatchEnabledMask != batchMode) || batchMode) {
        setBatch(batchMode,1);
    } 
    mOldBatchEnabledMask = batchMode;

    if (changed & ((1 << Gyro) | (1 << RawGyro) | (1 << Accelerometer) |
            (mCompassSensor->isIntegrated() << MagneticField) |
            (mCompassSensor->isIntegrated() << RawMagneticField) |
            (mPressureSensor->isIntegrated() << Pressure))) {
        LOGV_IF(ENG_VERBOSE,
                "HAL DEBUG: Gyro, Accel, Compass, Pressure changes");
        if ((checkSmdSupport() == 1) || (checkPedometerSupport() == 1) || (sensors &
            (INV_THREE_AXIS_GYRO
                | INV_THREE_AXIS_ACCEL
                | (INV_THREE_AXIS_COMPASS * mCompassSensor->isIntegrated())
                | (INV_ONE_AXIS_PRESSURE * mPressureSensor->isIntegrated())))) {
            LOGV_IF(ENG_VERBOSE, "SMD or Hardware sensors enabled");
            LOGV_IF(ENG_VERBOSE, 
                    "mFeatureActiveMask=%016llx", mFeatureActiveMask);
            if (mFeatureActiveMask & DMP_FEATURE_MASK) {
                LOGV_IF(ENG_VERBOSE, "HAL DEBUG: LPQ, SMD, SO enabled");
                // disable DMP event interrupt only (w/ data interrupt)
                LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                        0, mpu.dmp_event_int_on, getTimestamp());
                if (write_sysfs_int(mpu.dmp_event_int_on, 0) < 0) {
                    res = -1;
                    LOGE("HAL:ERR can't disable DMP event interrupt");
                    return res;
                }
            }
            LOGV_IF(ENG_VERBOSE, "mFeatureActiveMask=%lld", mFeatureActiveMask);
            LOGV_IF(ENG_VERBOSE, "DMP_FEATURE_MASK=%d", DMP_FEATURE_MASK);
        if ((mFeatureActiveMask & (long long)DMP_FEATURE_MASK) &&
                    (!(mFeatureActiveMask & INV_DMP_6AXIS_QUATERNION) ||
                     !(mFeatureActiveMask & INV_DMP_PED_STANDALONE) ||
                     !(mFeatureActiveMask & INV_DMP_PED_QUATERNION))) {
                // enable DMP
                onDmp(1);
                res = enableAccel(on);
                if(res < 0) {
                    return res;
                }
                LOGV_IF(ENG_VERBOSE, "mLocalSensorMask=0x%lx", mLocalSensorMask);
                if (((sensors | mLocalSensorMask) & INV_THREE_AXIS_ACCEL) == 0) {
                    res = turnOffAccelFifo();
                }
                if(res < 0) {
                    return res;
                }
            }
            res = masterEnable(1);
            if(res < 0) {
                return res;
            }
        } else { // all sensors idle -> reduce power
            LOGV_IF(ENG_VERBOSE, "HAL DEBUG: not SMD or Hardware sensors");
            if (isDmpDisplayOrientationOn()
                    && (mDmpOrientationEnabled
                            || !isDmpScreenAutoRotationEnabled())) {
                enableDmpOrientation(1);
            }

            if (!cal_stored) {
                storeCalibration();
                cal_stored = 1;
            }
        }
    } else if ((changed &
                    ((!mCompassSensor->isIntegrated()) << MagneticField) ||
                    ((!mCompassSensor->isIntegrated()) << RawMagneticField))
                    &&
              !(sensors & (INV_THREE_AXIS_GYRO | INV_THREE_AXIS_ACCEL
                | (INV_THREE_AXIS_COMPASS * (!mCompassSensor->isIntegrated()))))
    ) {
        LOGV_IF(ENG_VERBOSE, "HAL DEBUG: Gyro, Accel, Compass no change");
        if (!cal_stored) {
            storeCalibration();
            cal_stored = 1;
        }
    } else {
      LOGV_IF(ENG_VERBOSE, "HAL DEBUG: mEnabled");
      if (sensors &
            (INV_THREE_AXIS_GYRO
                | INV_THREE_AXIS_ACCEL
                | (INV_THREE_AXIS_COMPASS * mCompassSensor->isIntegrated()))) {
            res = masterEnable(1);
            if(res < 0)
                return res;
        }
    }

    return res;
}

/* check if batch mode should be turned on or not */
int MPLSensor::computeBatchSensorMask(int enableSensors, int tempBatchSensor)
{
    VFUNC_LOG;
    int batchMode = 1;
    mFeatureActiveMask &= ~INV_DMP_BATCH_MODE;

    LOGV_IF(ENG_VERBOSE,
            "HAL:computeBatchSensorMask: enableSensors=%d tempBatchSensor=%d",
            enableSensors, tempBatchSensor);

    // handle initialization case
    if (enableSensors == 0 && tempBatchSensor == 0)
        return 0;
            
    // check for possible continuous data mode
    for(int i = 0; i <= Pressure; i++) {
        if ((enableSensors & (1 << i)) && !(tempBatchSensor & (1 << i))) {
            LOGV_IF(ENG_VERBOSE, "HAL:computeBatchSensorMask: "
                    "hardware sensor on continuous mode:%d", i);
            // if any one of the hardware sensor is in continuous data mode
            // turn off batch mode.
            return 0;
        }
        if ((enableSensors & (1 << i)) && (tempBatchSensor & (1 << i))) {
            LOGV_IF(ENG_VERBOSE, 
                    "HAL:computeBatchSensorMask: hardware sensor is batch:%d", 
                    i);
            // if hardware sensor is batched, check if virtual sensor is batched
            if ((enableSensors & (1 << GameRotationVector))
                            && !(tempBatchSensor & (1 << GameRotationVector))) {
            LOGV_IF(ENG_VERBOSE, 
                    "HAL:computeBatchSensorMask: but virtual sensor is not:%d", 
                    i);
                return 0;
            }
        }
    }

    for(int i = Orientation; i <= GeomagneticRotationVector; i++) {
        if ((enableSensors & (1 << i)) && !(tempBatchSensor & (1 << i))) {
             LOGV_IF(ENG_VERBOSE, "HAL:computeBatchSensorMask: "
                     "composite sensor on continuous mode:%d", i);
            // if composite sensors are on but not batched
            // turn off batch mode.
            return 0;
        }
    }

    if ((mFeatureActiveMask & INV_DMP_PEDOMETER) && !(tempBatchSensor & (1 << StepDetector))) {
        LOGV("HAL:computeBatchSensorMask: step detector on continuous mode.");
        return 0;
    }
    
    mFeatureActiveMask |= INV_DMP_BATCH_MODE;
    LOGV_IF(EXTRA_VERBOSE,
            "HAL:computeBatchSensorMask: batchMode=%d, mBatchEnabled=%0x", 
            batchMode, tempBatchSensor);
    return (batchMode && tempBatchSensor);
}

/* This function is called by enable() */
int MPLSensor::setBatch(int en, int toggleEnable)
{
    VFUNC_LOG;

    int res = 0;
    int64_t wanted = 1000000000LL;
    int64_t timeout = 0;
    int timeoutInMs = 0;
    int featureMask = computeBatchDataOutput();

    // reset master enable
    res = masterEnable(0);
    if (res < 0) {
        return res;
    }

    if (en) {
        /* take the minimum batchmode timeout */
        int64_t timeout = 100000000000LL;
        int64_t ns;
        for (int i = 0; i < NumSensors; i++) {
            LOGV_IF(0, "mFeatureActiveMask=0x%016llx, mEnabled=0x%01x, mBatchEnabled=0x%x",
                            mFeatureActiveMask, mEnabled, mBatchEnabled);
            if (((mEnabled & (1 << i)) && (mBatchEnabled & (1 << i))) ||
                    (((featureMask & INV_DMP_PED_STANDALONE) && (mBatchEnabled & (1 << StepDetector))))) {
                LOGV_IF(ENG_VERBOSE, "sensor=%d, timeout=%lld", i, mBatchTimeouts[i]);
                ns = mBatchTimeouts[i];
                timeout = (ns < timeout) ? ns : timeout;
            }
        }
        /* Convert ns to millisecond */
        timeoutInMs = timeout / 1000000; 
    } else {
        timeoutInMs = 0;
    }    

    LOGV_IF(ENG_VERBOSE, "HAL: batch timeout set to %dms", timeoutInMs);

    /* step detector is enabled and */
    /* batch mode is standalone */
    if (en && (mFeatureActiveMask & INV_DMP_PEDOMETER) &&
            (featureMask & INV_DMP_PED_STANDALONE)) {
        LOGV_IF(ENG_VERBOSE, "ID_P only = 0x%x", mBatchEnabled);
        enablePedStandalone(1);
    } else {
        enablePedStandalone(0);
    }
    
    /* step detector and GRV are enabled and */
    /* batch mode is ped q */
    if (en && (mFeatureActiveMask & INV_DMP_PEDOMETER) &&
            (mEnabled & (1 << GameRotationVector)) &&
            (featureMask & INV_DMP_PED_QUATERNION)) {
        LOGV_IF(ENG_VERBOSE, "ID_P and GRV or ALL = 0x%x", mBatchEnabled);
        LOGV_IF(ENG_VERBOSE, "ID_P is enabled for batching, "
                "PED quat will be automatically enabled");
        enableLPQuaternion(0);
        enablePedQuaternion(1);
    } else if (!(featureMask & INV_DMP_PED_STANDALONE)){
        enablePedQuaternion(0);
    }

    /* step detector and hardware sensors enabled */
    if (en && (featureMask & INV_DMP_PED_INDICATOR) &&
            ((mEnabled) ||
            (mFeatureActiveMask & INV_DMP_PED_STANDALONE))) {
        enablePedIndicator(1);
    } else {
        enablePedIndicator(0);
    }
    
    /* GRV is enabled and */
    /* batch mode is 6axis q */
    if (en && (mEnabled & (1 << GameRotationVector)) &&
            (featureMask & INV_DMP_6AXIS_QUATERNION)) {
        LOGV_IF(ENG_VERBOSE, "GRV = 0x%x", mBatchEnabled);
        enableLPQuaternion(0);
        enable6AxisQuaternion(1);
        setInitial6QuatValue();
    } else if (!(featureMask & INV_DMP_PED_QUATERNION)){
        LOGV_IF(ENG_VERBOSE, "Toggle back to normal 6 axis");
        if (mEnabled & (1 << GameRotationVector) && checkLPQRateSupported()) {
            enableLPQuaternion(1);
        }
        enable6AxisQuaternion(0);
    }
    
    /* write required timeout to sysfs */
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
            timeoutInMs, mpu.batchmode_timeout, getTimestamp());
    if (write_sysfs_int(mpu.batchmode_timeout, timeoutInMs) < 0) {
        LOGE("HAL:ERR can't write batchmode_timeout");
    }

    if (en) {
        // enable DMP
        res = onDmp(1);
        if (res < 0) {
            return res;
        }

        // set batch rates
        if (setBatchDataRates() < 0) {
            LOGE("HAL:ERR can't set batch data rates");
        }
        
        // default fifo rate to 200Hz
        LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                200, mpu.gyro_fifo_rate, getTimestamp());
        if (write_sysfs_int(mpu.gyro_fifo_rate, 200) < 0) {
            res = -1;
            LOGE("HAL:ERR can't set rate to 200Hz");
            return res;
        }
    } else {
        if (mFeatureActiveMask == 0) {
            // disable DMP
            res = onDmp(0);
            if (res < 0) {
                return res;
            }
            /* reset sensor rate */
            if (resetDataRates() < 0) {
                LOGE("HAL:ERR can't reset output rate back to original setting");
            }
        }
        /* reset sensor rate */
        /*if (resetDataRates() < 0) {
            LOGE("HAL:ERR can't reset output rate back to original setting");
        }*/
    }
    if (toggleEnable == 1) {
        if (mFeatureActiveMask || mEnabled)
            masterEnable(1);
    }
    return res;
}

/* Store calibration file */
void MPLSensor::storeCalibration(void)
{
    VFUNC_LOG;

    if(mHaveGoodMpuCal == true
        || mAccelAccuracy >= 2
        || mCompassAccuracy >= 3) {
       int res = inv_store_calibration();
       if (res) {
           LOGE("HAL:Cannot store calibration on file");
       } else {
           LOGV_IF(PROCESS_VERBOSE, "HAL:Cal file updated");
       }
    }
}

void MPLSensor::cbProcData(void)
{
    VFUNC_LOG;

    mNewData = 1;
    mSampleCount++;
    LOGV_IF(EXTRA_VERBOSE, "HAL:new data");
}

/*  these handlers transform mpl data into one of the Android sensor types */
int MPLSensor::gyroHandler(sensors_event_t* s)
{
    VHANDLER_LOG;
    int update;
    update = inv_get_sensor_type_gyroscope(s->gyro.v, &s->gyro.status,
                                           &s->timestamp);
    LOGV_IF(HANDLER_DATA, "HAL:gyro data : %+f %+f %+f -- %lld - %d",
            s->gyro.v[0], s->gyro.v[1], s->gyro.v[2], s->timestamp, update);
    return update;
}

int MPLSensor::rawGyroHandler(sensors_event_t* s)
{
    VHANDLER_LOG;
    int update;
    update = inv_get_sensor_type_gyroscope_raw(s->uncalibrated_gyro.uncalib,
                                               &s->gyro.status, &s->timestamp);
    if(update) {
        memcpy(s->uncalibrated_gyro.bias, mGyroBias, sizeof(mGyroBias));
        LOGV_IF(HANDLER_DATA,"HAL:gyro bias data : %+f %+f %+f -- %lld - %d",
            s->uncalibrated_gyro.bias[0], s->uncalibrated_gyro.bias[1],
            s->uncalibrated_gyro.bias[2], s->timestamp, update);
    }
    s->gyro.status = SENSOR_STATUS_UNRELIABLE;
    LOGV_IF(HANDLER_DATA, "HAL:raw gyro data : %+f %+f %+f -- %lld - %d",
            s->uncalibrated_gyro.uncalib[0], s->uncalibrated_gyro.uncalib[1],
            s->uncalibrated_gyro.uncalib[2], s->timestamp, update);
    return update;
}

int MPLSensor::accelHandler(sensors_event_t* s)
{
    VHANDLER_LOG;
    int update;
    update = inv_get_sensor_type_accelerometer(
        s->acceleration.v, &s->acceleration.status, &s->timestamp);
    LOGV_IF(HANDLER_DATA, "HAL:accel data : %+f %+f %+f -- %lld - %d",
            s->acceleration.v[0], s->acceleration.v[1], s->acceleration.v[2],
            s->timestamp, update);
    mAccelAccuracy = s->acceleration.status;
    return update;
}

int MPLSensor::compassHandler(sensors_event_t* s)
{
    VHANDLER_LOG;
    int update;
    update = inv_get_sensor_type_magnetic_field(
        s->magnetic.v, &s->magnetic.status, &s->timestamp);
    LOGV_IF(HANDLER_DATA, "HAL:compass data: %+f %+f %+f -- %lld - %d",
            s->magnetic.v[0], s->magnetic.v[1], s->magnetic.v[2],
            s->timestamp, update);
    mCompassAccuracy = s->magnetic.status;
    return update;
}

int MPLSensor::rawCompassHandler(sensors_event_t* s)
{
    VHANDLER_LOG;
    int update;
    //TODO: need to handle uncalib data and bias for 3rd party compass
    if(mCompassSensor->providesCalibration()) {
        update = mCompassSensor->readRawSample(s->uncalibrated_magnetic.uncalib, &s->timestamp);
    }
    else {
        update = inv_get_sensor_type_magnetic_field_raw(s->uncalibrated_magnetic.uncalib,
                     &s->magnetic.status, &s->timestamp);
    }
    if(update) {
        memcpy(s->uncalibrated_magnetic.bias, mCompassBias, sizeof(mCompassBias));
        LOGV_IF(HANDLER_DATA, "HAL:compass bias data: %+f %+f %+f -- %lld - %d",
                s->uncalibrated_magnetic.bias[0], s->uncalibrated_magnetic.bias[1],
                s->uncalibrated_magnetic.bias[2], s->timestamp, update);
    }
    s->magnetic.status = SENSOR_STATUS_UNRELIABLE;
    LOGV_IF(HANDLER_DATA, "HAL:compass raw data: %+f %+f %+f %d -- %lld - %d",
        s->uncalibrated_magnetic.uncalib[0], s->uncalibrated_magnetic.uncalib[1],
                    s->uncalibrated_magnetic.uncalib[2], s->magnetic.status, s->timestamp, update);
    return update;
}

/*
    Rotation Vector handler.
    NOTE: rotation vector does not have an accuracy or status
*/
int MPLSensor::rvHandler(sensors_event_t* s)
{
    VHANDLER_LOG;
    int8_t status;
    int update;
    update = inv_get_sensor_type_rotation_vector(s->data, &status,
                                                 &s->timestamp);
    update |= isCompassDisabled();
    LOGV_IF(HANDLER_DATA, "HAL:rv data: %+f %+f %+f %+f %+f- %+lld - %d",
            s->data[0], s->data[1], s->data[2], s->data[3], s->data[4], s->timestamp,
            update);

    return update;
}

/*
    Game Rotation Vector handler.
    NOTE: rotation vector does not have an accuracy or status
*/
int MPLSensor::grvHandler(sensors_event_t* s)
{
    VHANDLER_LOG;
    int8_t status;
    int update;
    update = inv_get_sensor_type_rotation_vector_6_axis(s->data, &status,
                                                     &s->timestamp);
    /*hack*/
    /*s->data[0] =  mCached6AxisQuaternionData[0];
    s->data[1] =  mCached6AxisQuaternionData[1];
    s->data[2] =  mCached6AxisQuaternionData[2];
    update = 1;*/
    
                                                     
    LOGV_IF(HANDLER_DATA, "HAL:grv data: %+f %+f %+f %+f %+f - %+lld - %d",
            s->data[0], s->data[1], s->data[2], s->data[3], s->data[4], s->timestamp,
            update);
    return update;
}

int MPLSensor::laHandler(sensors_event_t* s)
{
    VHANDLER_LOG;
    int update;
    update = inv_get_sensor_type_linear_acceleration(
            s->gyro.v, &s->gyro.status, &s->timestamp);
    update |= isCompassDisabled();
    LOGV_IF(HANDLER_DATA, "HAL:la data: %+f %+f %+f - %lld - %d",
            s->gyro.v[0], s->gyro.v[1], s->gyro.v[2], s->timestamp, update);
    return update;
}

int MPLSensor::gravHandler(sensors_event_t* s)
{
    VHANDLER_LOG;
    int update;
    update = inv_get_sensor_type_gravity(s->gyro.v, &s->gyro.status,
                                         &s->timestamp);
    update |= isCompassDisabled();
    LOGV_IF(HANDLER_DATA, "HAL:gr data: %+f %+f %+f - %lld - %d",
            s->gyro.v[0], s->gyro.v[1], s->gyro.v[2], s->timestamp, update);
    return update;
}

int MPLSensor::orienHandler(sensors_event_t* s)
{
    VHANDLER_LOG;
    int update;
    update = inv_get_sensor_type_orientation(
            s->orientation.v, &s->orientation.status, &s->timestamp);
    update |= isCompassDisabled();
    LOGV_IF(HANDLER_DATA, "HAL:or data: %f %f %f - %lld - %d",
            s->orientation.v[0], s->orientation.v[1], s->orientation.v[2],
            s->timestamp, update);
    return update;
}

int MPLSensor::smHandler(sensors_event_t* s)
{
    VHANDLER_LOG;
    int update = 1;

    /* When event is triggered, set data to 1 */
    s->data[0] = 1.f;
    s->data[1] = 0.f;
    s->data[2] = 0.f;
    s->acceleration.status
            = SENSOR_STATUS_UNRELIABLE;

    /* Capture timestamp in HAL */
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    s->timestamp = (int64_t) ts.tv_sec * 1000000000 + ts.tv_nsec;

    /* Identify which sensor this event is for */
    s->version = sizeof(sensors_event_t);
    s->sensor = ID_SM;
    s->type = SENSOR_TYPE_SIGNIFICANT_MOTION;

    LOGV_IF(HANDLER_DATA, "HAL:sm data: %f - %lld - %d",
            s->data[0], s->timestamp, update);
    return update;
}

int MPLSensor::scHandler(sensors_event_t* s)
{
    VHANDLER_LOG;
    int update = 0;

    //update = readDmpPedometerEvents(s, 1);
    LOGV_IF(HANDLER_DATA, "HAL:sc data: %f - %lld - %d",
            s->data[0], s->timestamp, update);
    return update < 1 ? 0 :1;
}

int MPLSensor::gmHandler(sensors_event_t* s)
{
    VHANDLER_LOG;
    int8_t status;
    int update = 0;
    update = inv_get_sensor_type_geomagnetic_rotation_vector(s->data, &status,
                                                             &s->timestamp);

    LOGV_IF(HANDLER_DATA, "HAL:gm data: %+f %+f %+f %+f %+f- %+lld - %d",
            s->data[0], s->data[1], s->data[2], s->data[3], s->data[4], s->timestamp, update);
    return update < 1 ? 0 :1;

}

int MPLSensor::psHandler(sensors_event_t* s)
{
    VHANDLER_LOG;
    int8_t status;
    int update = 0;

    s->pressure = mCachedPressureData / 100.f; //hpa (millibar)
    s->data[1] = 0;
    s->data[2] = 0;
    s->timestamp = mPressureTimestamp;
    s->magnetic.status = 0;
    update = mPressureUpdate;
    mPressureUpdate = 0;

    LOGV_IF(HANDLER_DATA, "HAL:ps data: %+f %+f %+f %+f- %+lld - %d",
            s->data[0], s->data[1], s->data[2], s->data[3], s->timestamp, update);
    return update < 1 ? 0 :1;

}

int MPLSensor::metaHandler(sensors_event_t* s, int flags)
{
    VHANDLER_LOG;
    int update = 0;
    
    /* initalize SENSOR_TYPE_META_DATA */
    s->version = 0;
    s->sensor = 0;
    s->reserved0 = 0;
    s->timestamp = 0LL;
    
    switch(flags) {
    case META_DATA_FLUSH_COMPLETE:
        update = mFlushBatchSet;
        s->type = SENSOR_TYPE_META_DATA;
        s->meta_data.what = flags;
        s->meta_data.sensor = mFlushEnabled;
        mFlushBatchSet = 0;
        mFlushEnabled = -1;
        LOGV_IF(HANDLER_DATA,
                "HAL:flush complete data: type=%d what=%d, "
                "sensor=%d - %lld - %d",
                s->type, s->meta_data.what, s->meta_data.sensor,
                s->timestamp, update);
        break;
    
    default:
        LOGW("HAL: Meta flags not supported");
        break;
    }
    
    return update;
}

int MPLSensor::enable(int32_t handle, int en)
{
    VFUNC_LOG;

    android::String8 sname;
    int what = -1, err = 0;
    int batchMode = 0;

    switch (handle) {
    case ID_SC:
         what = StepCounter;
         sname = "Step Counter";
         LOGV_IF(PROCESS_VERBOSE, "HAL:enable - sensor %s (handle %d) %s -> %s",
                sname.string(), handle,
                (mDmpStepCountEnabled? "en": "dis"),
                (en? "en" : "dis"));
        enableDmpPedometer(en, 0);
        mDmpStepCountEnabled = !!en;
        return 0;
    case ID_P:
        sname = "StepDetector";
        LOGV_IF(PROCESS_VERBOSE, "HAL:enable - sensor %s (handle %d) %s -> %s",
                sname.string(), handle,
                (mDmpPedometerEnabled? "en": "dis"),
                (en? "en" : "dis"));
        enableDmpPedometer(en, 1);
        mDmpPedometerEnabled = !!en;
        batchMode = computeBatchSensorMask(mEnabled, mBatchEnabled);
        /* skip setBatch if there is no need to */
        if(((int)mOldBatchEnabledMask != batchMode) || batchMode) {
            setBatch(batchMode,1);
        }
        mOldBatchEnabledMask = batchMode;
        return 0;
    case ID_SM:
        sname = "Significant Motion";
        LOGV_IF(PROCESS_VERBOSE, "HAL:enable - sensor %s (handle %d) %s -> %s",
                sname.string(), handle,
                (mDmpSignificantMotionEnabled? "en": "dis"),
                (en? "en" : "dis"));
        enableDmpSignificantMotion(en);
        mDmpSignificantMotionEnabled = !!en;
        return 0;
    case ID_SO:
        sname = "Screen Orientation";
        LOGV_IF(PROCESS_VERBOSE, "HAL:enable - sensor %s (handle %d) %s -> %s",
                sname.string(), handle,
                (mDmpOrientationEnabled? "en": "dis"),
                (en? "en" : "dis"));
        enableDmpOrientation(en && isDmpDisplayOrientationOn());
        mDmpOrientationEnabled = !!en;
        return 0;
    case ID_A:
        what = Accelerometer;
        sname = "Accelerometer";
        break;
    case ID_M:
        what = MagneticField;
        sname = "MagneticField";
        break;
    case ID_RM:
        what = RawMagneticField;
        sname = "MagneticField Uncalibrated";
        break;
    case ID_O:
        what = Orientation;
        sname = "Orientation";
        break;
    case ID_GY:
        what = Gyro;
        sname = "Gyro";
        break;
    case ID_RG:
        what = RawGyro;
        sname = "Gyro Uncalibrated";
        break;
    case ID_GR:
        what = Gravity;
        sname = "Gravity";
        break;
    case ID_RV:
        what = RotationVector;
        sname = "RotationVector";
        break;
    case ID_GRV:
        what = GameRotationVector;
        sname = "GameRotationVector";
        break;
    case ID_LA:
        what = LinearAccel;
        sname = "LinearAccel";
        break;
    case ID_GMRV:
        what = GeomagneticRotationVector;
        sname = "GeomagneticRotationVector";
        break;
    case ID_PS:
        what = Pressure;
        sname = "Pressure";
        break;
    default: //this takes care of all the gestures
        what = handle;
        sname = "Others";
        break;
    }

    if (uint32_t(what) >= NumSensors)
        return -EINVAL;

    int newState = en ? 1 : 0;
    unsigned long sen_mask;

    LOGV_IF(PROCESS_VERBOSE, "HAL:enable - sensor %s (handle %d) %s -> %s",
            sname.string(), handle,
            ((mEnabled & (1 << what)) ? "en" : "dis"),
            ((uint32_t(newState) << what) ? "en" : "dis"));
    LOGV_IF(ENG_VERBOSE,
            "HAL:%s sensor state change what=%d", sname.string(), what);

    // pthread_mutex_lock(&mMplMutex);
    // pthread_mutex_lock(&mHALMutex);

    if ((uint32_t(newState) << what) != (mEnabled & (1 << what))) {
        uint32_t sensor_type;
        short flags = newState;
        uint32_t lastEnabled = mEnabled, changed = 0;

        mEnabled &= ~(1 << what);
        mEnabled |= (uint32_t(flags) << what);

        LOGV_IF(ENG_VERBOSE, "HAL:handle = %d", handle);
        LOGV_IF(ENG_VERBOSE, "HAL:flags = %d", flags);
        computeLocalSensorMask(mEnabled);
        LOGV_IF(ENG_VERBOSE, "HAL:enable : mEnabled = %d", mEnabled);
        LOGV_IF(ENG_VERBOSE, "HAL:last enable : lastEnabled = %d", lastEnabled);
        sen_mask = mLocalSensorMask & mMasterSensorMask;
        mSensorMask = sen_mask;
        LOGV_IF(ENG_VERBOSE, "HAL:sen_mask= 0x%0lx", sen_mask);

        switch (what) {
            case Gyro:
            case RawGyro:
            case Accelerometer:
                if ((!(mEnabled & VIRTUAL_SENSOR_GYRO_6AXES_MASK) &&
                    !(mEnabled & VIRTUAL_SENSOR_9AXES_MASK)) &&
                    ((lastEnabled & (1 << what)) != (mEnabled & (1 << what)))) {
                    changed |= (1 << what);
                }
                if (mFeatureActiveMask & INV_DMP_6AXIS_QUATERNION) {
                     changed |= (1 << what);
                }
                break;
            case MagneticField:
            case RawMagneticField:
                if (!(mEnabled & VIRTUAL_SENSOR_9AXES_MASK) &&
                    ((lastEnabled & (1 << what)) != (mEnabled & (1 << what)))) {
                    changed |= (1 << what);
                }
                break;
            case Pressure:
                if ((lastEnabled & (1 << what)) != (mEnabled & (1 << what))) {
                    changed |= (1 << what);
                }
                break;
            case GameRotationVector:
                if (!en)
                    storeCalibration();
                if ((en && !(lastEnabled & VIRTUAL_SENSOR_ALL_MASK))
                         ||
                    (en && !(lastEnabled & VIRTUAL_SENSOR_9AXES_MASK))
                         ||
                    (!en && !(mEnabled & VIRTUAL_SENSOR_ALL_MASK))
                         ||
                    (!en && (mEnabled & VIRTUAL_SENSOR_MAG_6AXES_MASK))) {
                    for (int i = Gyro; i <= RawMagneticField; i++) {
                        if (!(mEnabled & (1 << i))) {
                            changed |= (1 << i);
                        }
                    }
                }
                break;

            case Orientation:
            case RotationVector:
            case LinearAccel:
            case Gravity:
                if (!en)
                    storeCalibration();
                if ((en && !(lastEnabled & VIRTUAL_SENSOR_9AXES_MASK))
                         ||
                    (!en && !(mEnabled & VIRTUAL_SENSOR_9AXES_MASK))) {
                    for (int i = Gyro; i <= RawMagneticField; i++) {
                        if (!(mEnabled & (1 << i))) {
                            changed |= (1 << i);
                        }
                    }
                }
                break;
            case GeomagneticRotationVector:
                if (!en)
                    storeCalibration();
                if ((en && !(lastEnabled & VIRTUAL_SENSOR_ALL_MASK))
                        ||
                    (en && !(lastEnabled & VIRTUAL_SENSOR_9AXES_MASK))
                         ||
                   (!en && !(mEnabled & VIRTUAL_SENSOR_ALL_MASK))
                         ||
                   (!en && (mEnabled & VIRTUAL_SENSOR_GYRO_6AXES_MASK))) {
                   for (int i = Accelerometer; i <= RawMagneticField; i++) {
                       if (!(mEnabled & (1<<i))) {
                          changed |= (1 << i);
                       }
                   }
                }
                break;
        }
        LOGV_IF(ENG_VERBOSE, "HAL:changed = %d", changed);
        enableSensors(sen_mask, flags, changed);
    }

    // pthread_mutex_unlock(&mMplMutex);
    // pthread_mutex_unlock(&mHALMutex);

#ifdef INV_PLAYBACK_DBG
    /* apparently the logging needs to go through this sequence
       to properly flush the log file */
    inv_turn_off_data_logging();
    if (fclose(logfile) < 0) {
         LOGE("cannot close debug log file");
    }
    logfile = fopen("/data/playback.bin", "ab");
    if (logfile)
        inv_turn_on_data_logging(logfile);
#endif

    return err;
}

void MPLSensor::getHandle(int32_t handle, int &what, android::String8 &sname)
{
   VFUNC_LOG;

   what = -1;

   switch (handle) {
   case ID_P:
        what = StepDetector;
        sname = "StepDetector";
        break;
   case ID_SC:
        what = StepCounter;
        sname = "StepCounter";
        break;
   case ID_SM:
        what = SignificantMotion;
        sname = "SignificantMotion";
        break;
   case ID_SO:
        what = handle;
        sname = "ScreenOrienation";
   case ID_A:
        what = Accelerometer;
        sname = "Accelerometer";
        break;
   case ID_M:
        what = MagneticField;
        sname = "MagneticField";
        break;
   case ID_RM:
        what = RawMagneticField;
        sname = "MagneticField Uncalibrated";
        break;
   case ID_O:
        what = Orientation;
        sname = "Orientation";
        break;
   case ID_GY:
        what = Gyro;
        sname = "Gyro";
        break;
   case ID_RG:
        what = RawGyro;
        sname = "Gyro Uncalibrated";
        break;
   case ID_GR:
        what = Gravity;
        sname = "Gravity";
        break;
   case ID_RV:
        what = RotationVector;
        sname = "RotationVector";
        break;
   case ID_GRV:
        what = GameRotationVector;
        sname = "GameRotationVector";
        break;
   case ID_LA:
        what = LinearAccel;
        sname = "LinearAccel";
        break;
   case ID_PS:
        what = Pressure;
        sname = "Pressure";
        break;
   default: // this takes care of all the gestures
        what = handle;
        sname = "Others";
        break;
    }

    LOGI_IF(EXTRA_VERBOSE, "HAL:getHandle - what=%d, sname=%s", what, sname.string());
    return;
}

int MPLSensor::setDelay(int32_t handle, int64_t ns)
{
    VFUNC_LOG;

    android::String8 sname;
    int what = -1;

#if 0
    // skip the 1st call for enalbing sensors called by ICS/JB sensor service
    static int counter_delay = 0;
    if (!(mEnabled & (1 << what))) {
        counter_delay = 0;
    } else {
        if (++counter_delay == 1) {
            return 0;
        }
        else {
            counter_delay = 0;
        }
    }
#endif

    getHandle(handle, what, sname);
    if (uint32_t(what) >= NumSensors)
        return -EINVAL;

    if (ns < 0)
        return -EINVAL;

    LOGV_IF(PROCESS_VERBOSE,
            "setDelay : %llu ns, (%.2f Hz)", ns, 1000000000.f / ns);

    // limit all rates to reasonable ones */
    if (ns < 5000000LL) {
        ns = 5000000LL;
    }

    /* store request rate to mDelays arrary for each sensor */
    int64_t previousDelay = mDelays[what];
    mDelays[what] = ns;
    LOGV_IF(ENG_VERBOSE, "storing mDelays[%d] = %lld, previousDelay = %lld", what, ns, previousDelay);

    switch (what) {
        case ID_SC:
            /* set limits of delivery rate of events */
            mStepCountPollTime = ns;
            LOGV_IF(ENG_VERBOSE, "step count rate =%lld ns", ns);
            break; 
        case ID_P:
        case SignificantMotion:
        case ID_SO:
            update_delay();
            break;
        case Gyro:
        case RawGyro:
        case Accelerometer:
            // need to update delay since they are different
            // resetDataRates was called earlier
            //LOGV("what=%d mEnabled=%d mDelays[%d]=%lld previousDelay=%lld",
            //what, mEnabled, what, mDelays[what], previousDelay);
            if ((mEnabled & (1 << what)) && (previousDelay != mDelays[what])) {
                LOGV_IF(ENG_VERBOSE,
                    "HAL:need to update delay due to resetDataRates");
                break;
            }
            for (int i = Gyro;
                    i <= Accelerometer + mCompassSensor->isIntegrated();
                    i++) {
                if (i != what && (mEnabled & (1 << i)) && ns > mDelays[i]) {
                    LOGV_IF(ENG_VERBOSE,
                            "HAL:ignore delay set due to sensor %d", i);
                    return 0;
                }
            }
            break;

        case MagneticField:
        case RawMagneticField:
            // need to update delay since they are different
            // resetDataRates was called earlier
            if ((mEnabled & (1 << what)) && (previousDelay != mDelays[what])) {
                LOGV_IF(ENG_VERBOSE,
                    "HAL:need to update delay due to resetDataRates");
                break;
            }
            if (mCompassSensor->isIntegrated() &&
                    (((mEnabled & (1 << Gyro)) && ns > mDelays[Gyro]) ||
                    ((mEnabled & (1 << RawGyro)) && ns > mDelays[RawGyro]) ||
                    ((mEnabled & (1 << Accelerometer)) &&
                        ns > mDelays[Accelerometer])) &&
                        !checkBatchEnabled()) {
                 /* if request is slower rate, ignore request */
                 LOGV_IF(ENG_VERBOSE,
                         "HAL:ignore delay set due to gyro/accel");
                 return 0;
            }
            break;

        case Orientation:
        case RotationVector:
        case GameRotationVector:
        case GeomagneticRotationVector:
        case LinearAccel:
        case Gravity:
            if (isLowPowerQuatEnabled()) {
                LOGV_IF(ENG_VERBOSE,
                        "HAL:need to update delay due to LPQ");
                break;
            }

            for (int i = 0; i < NumSensors; i++) {
                if (i != what && (mEnabled & (1 << i)) && ns > mDelays[i]) {
                    LOGV_IF(ENG_VERBOSE,
                            "HAL:ignore delay set due to sensor %d", i);
                    return 0;
                }
            }
            break;
    }

    // pthread_mutex_lock(&mHALMutex);
    int res = update_delay();
    // pthread_mutex_unlock(&mHALMutex);
    return res;
}

int MPLSensor::update_delay(void)
{
    VFUNC_LOG;

    int res = 0;
    int64_t got;

    if (mEnabled) {
        int64_t wanted = 1000000000LL;
        int64_t wanted_3rd_party_sensor = 1000000000LL;

        // Sequence to change sensor's FIFO rate
        // 1. reset master enable
        // 2. Update delay
        // 3. set master enable

        // reset master enable
        masterEnable(0);

        int64_t gyroRate;
        int64_t accelRate;
        int64_t compassRate;
        int64_t pressureRate;
        
        int rateInus;
        int mplGyroRate;
        int mplAccelRate;
        int mplCompassRate;
        int tempRate = wanted;

        /* search the minimum delay requested across all enabled sensors */
        for (int i = 0; i < NumSensors; i++) {
            if (mEnabled & (1 << i)) {
                int64_t ns = mDelays[i];
                wanted = wanted < ns ? wanted : ns;
            }
        }
        
        if (mDmpOn) {
            gyroRate = mDelays[Gyro] < mDelays[RawGyro] ? mDelays[Gyro] : mDelays[RawGyro];
            accelRate = mDelays[Accelerometer];
            compassRate = mDelays[MagneticField] < mDelays[RawMagneticField] ? mDelays[MagneticField] : mDelays[RawMagneticField];
            pressureRate = mDelays[Pressure];
            
#ifdef ENABLE_MULTI_RATE
            gyroRate = wanted;
            accelRate = wanted;
            compassRate = wanted;
            pressureRate = wanted;   
            // same delay for 3rd party Accel or Compass
            wanted_3rd_party_sensor = wanted;
#endif
 
        }
        else {        
            gyroRate = wanted;
            accelRate = wanted;
            compassRate = wanted;
            pressureRate = wanted;   
            // same delay for 3rd party Accel or Compass
            wanted_3rd_party_sensor = wanted;
        }        

        int enabled_sensors = mEnabled;
        int tempFd = -1;

        if(mFeatureActiveMask & INV_DMP_BATCH_MODE) {
            // set batch rates
            LOGV_IF(ENG_VERBOSE, "HAL: mFeatureActiveMask=%016llx", mFeatureActiveMask);
            LOGV("HAL: batch mode is set, set batch data rates");
            if (setBatchDataRates() < 0) {                
                LOGE("HAL:ERR can't set batch data rates");
            }
        } else {
        /* set master sampling frequency */
        int64_t tempWanted = wanted;
        getDmpRate(&tempWanted);
        /* driver only looks at sampling frequency if DMP is off */
        LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %.0f > %s (%lld)",
                1000000000.f / tempWanted, mpu.gyro_fifo_rate, getTimestamp());
        tempFd = open(mpu.gyro_fifo_rate, O_RDWR);
        res = write_attribute_sensor(tempFd, 1000000000.f / tempWanted);
        LOGE_IF(res < 0, "HAL:sampling frequency update delay error");

        if (LA_ENABLED || GR_ENABLED || RV_ENABLED
                       || GRV_ENABLED || O_ENABLED || GMRV_ENABLED) {
            rateInus = (int)wanted / 1000LL;

            /* set rate in MPL */
            /* compass can only do 100Hz max */
            inv_set_gyro_sample_rate(rateInus);
            inv_set_accel_sample_rate(rateInus);
            inv_set_compass_sample_rate(rateInus);

            LOGV_IF(ENG_VERBOSE,
                    "HAL:MPL gyro sample rate: (mpl)=%d us (mpu)=%.2f Hz",
                    rateInus, 1000000000.f / gyroRate);
            LOGV_IF(ENG_VERBOSE,
                    "HAL:MPL accel sample rate: (mpl)=%d us (mpu)=%.2f Hz",
                    rateInus, 1000000000.f / accelRate);
            LOGV_IF(ENG_VERBOSE,
                    "HAL:MPL compass sample rate: (mpl)=%d us (mpu)=%.2f Hz",
                    rateInus, 1000000000.f / compassRate);

            LOGV_IF(ENG_VERBOSE, 
                    "mFeatureActiveMask=%016llx", mFeatureActiveMask);
            //TODO: may be able to combine DMP_FEATURE_MASK, DMP_SENSOR_MASK in the future
            if(mFeatureActiveMask & DMP_FEATURE_MASK) {
                bool setDMPrate= 0;
                gyroRate = wanted;
                accelRate = wanted;
                compassRate = wanted;      
                // same delay for 3rd party Accel or Compass
                wanted_3rd_party_sensor = wanted;
                rateInus = (int)wanted / 1000LL;
                
                 /* set rate in MPL */
                 /* compass can only do 100Hz max */
                 /*inv_set_gyro_sample_rate(rateInus);
                 inv_set_accel_sample_rate(rateInus);
                 inv_set_compass_sample_rate(rateInus);
                 
                 LOGV_IF(PROCESS_VERBOSE,
                "HAL:MPL gyro sample rate: (mpl)=%d us (mpu)=%.2f Hz", rateInus, 1000000000.f / gyroRate);
                 LOGV_IF(PROCESS_VERBOSE,
                "HAL:MPL accel sample rate: (mpl)=%d us (mpu)=%.2f Hz", rateInus, 1000000000.f / accelRate);
                 LOGV_IF(PROCESS_VERBOSE,
                "HAL:MPL compass sample rate: (mpl)=%d us (mpu)=%.2f Hz", rateInus, 1000000000.f / compassRate);
                 */
                // Set LP Quaternion sample rate if enabled
                if (checkLPQuaternion()) {
                    if (wanted <= RATE_200HZ) {
#ifndef USE_LPQ_AT_FASTEST 
                        enableLPQuaternion(0);
#endif
                    } else {
                        inv_set_quat_sample_rate(rateInus);
                        LOGV_IF(ENG_VERBOSE, "HAL:MPL quat sample rate: "
                                "(mpl)=%d us (mpu)=%.2f Hz", 
                                rateInus, 1000000000.f / wanted);
                        setDMPrate= 1;
                    }
                }
            }

            LOGV_IF(EXTRA_VERBOSE, "HAL:setDelay - Fusion");
            //nsToHz
            LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %.0f > %s (%lld)",
                    1000000000.f / gyroRate, mpu.gyro_rate,
                    getTimestamp());
            tempFd = open(mpu.gyro_rate, O_RDWR);
            res = write_attribute_sensor(tempFd, 1000000000.f / gyroRate);
            if(res < 0) {
                LOGE("HAL:GYRO update delay error");
            }

            if(USE_THIRD_PARTY_ACCEL == 1) {
                // 3rd party accelerometer - if applicable
                // nsToHz (BMA250)
                LOGV_IF(SYSFS_VERBOSE, "echo %lld > %s (%lld)",
                        wanted_3rd_party_sensor / 1000000L, mpu.accel_rate,
                        getTimestamp());
                tempFd = open(mpu.accel_rate, O_RDWR);
                res = write_attribute_sensor(tempFd,
                        wanted_3rd_party_sensor / 1000000L);
                LOGE_IF(res < 0, "HAL:ACCEL update delay error");
            } else {
                // mpu accel
               LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %.0f > %s (%lld)",
                        1000000000.f / accelRate, mpu.accel_rate,
                        getTimestamp());
                tempFd = open(mpu.accel_rate, O_RDWR);
                res = write_attribute_sensor(tempFd, 1000000000.f / accelRate);
                LOGE_IF(res < 0, "HAL:ACCEL update delay error");
            }

            if (!mCompassSensor->isIntegrated()) {
                // stand-alone compass - if applicable
                LOGV_IF(ENG_VERBOSE,
                        "HAL:Ext compass delay %lld", wanted_3rd_party_sensor);
                LOGV_IF(ENG_VERBOSE, "HAL:Ext compass rate %.2f Hz",
                        1000000000.f / wanted_3rd_party_sensor);
                if (wanted_3rd_party_sensor <
                        mCompassSensor->getMinDelay() * 1000LL) {
                    wanted_3rd_party_sensor =
                        mCompassSensor->getMinDelay() * 1000LL;
                }
                LOGV_IF(ENG_VERBOSE,
                        "HAL:Ext compass delay %lld", wanted_3rd_party_sensor);
                LOGV_IF(ENG_VERBOSE, "HAL:Ext compass rate %.2f Hz",
                        1000000000.f / wanted_3rd_party_sensor);
                mCompassSensor->setDelay(ID_M, wanted_3rd_party_sensor);
                got = mCompassSensor->getDelay(ID_M);
                inv_set_compass_sample_rate(got / 1000);
            } else {
                // compass on secondary bus
                if (compassRate < mCompassSensor->getMinDelay() * 1000LL) {
                    compassRate = mCompassSensor->getMinDelay() * 1000LL;
                }
                mCompassSensor->setDelay(ID_M, compassRate);
            }

            /*
            //nsTons - nothing to be done
            strcpy(&compass_sensor_sysfs_path[compass_sensor_sysfs_path_len],
                   COMPASS_SENSOR_DELAY);
            tempFd = open(compass_sensor_sysfs_path, O_RDWR);
            LOGV_IF(PROCESS_VERBOSE,
                    "setDelay - open path: %s", compass_sensor_sysfs_path);
            wanted = 20000000LLU;
            res = write_attribute_sensor(tempFd, wanted);
            if(res < 0) {
                LOGE("Compass update delay error");
            }
            */

        } else {

            if (GY_ENABLED || RGY_ENABLED) {
                wanted = (mDelays[Gyro] <= mDelays[RawGyro]?
                    (mEnabled & (1 << Gyro)? mDelays[Gyro]: mDelays[RawGyro]):
                    (mEnabled & (1 << RawGyro)? mDelays[RawGyro]: mDelays[Gyro]));
                LOGV_IF(ENG_VERBOSE, "mFeatureActiveMask=%016llx", mFeatureActiveMask);
                if (mFeatureActiveMask & DMP_FEATURE_MASK) {
                    //int64_t tempWanted;
                    //getDmpRate(&tempWanted);
                }

                inv_set_gyro_sample_rate((int)wanted/1000LL);
                LOGV_IF(ENG_VERBOSE,
                    "HAL:MPL gyro sample rate: (mpl)=%d us", int(wanted/1000LL));
                LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %.0f > %s (%lld)",
                        1000000000.f / wanted, mpu.gyro_rate, getTimestamp());
                tempFd = open(mpu.gyro_rate, O_RDWR);
                res = write_attribute_sensor(tempFd, 1000000000.f / wanted);
                LOGE_IF(res < 0, "HAL:GYRO update delay error");
            }

            if (A_ENABLED) { /* there is only 1 fifo rate for MPUxxxx */
#if (0)
                wanted = mDelays[Accelerometer];
#else
                if (GY_ENABLED && mDelays[Gyro] < mDelays[Accelerometer]) {
                    wanted = mDelays[Gyro];
                } else if (RGY_ENABLED && mDelays[RawGyro]
                            < mDelays[Accelerometer]) {
                    wanted = mDelays[RawGyro];
                } else {
                    wanted = mDelays[Accelerometer];
                }
#endif
                LOGV_IF(ENG_VERBOSE, "mFeatureActiveMask=%016llx", mFeatureActiveMask);
                if (mFeatureActiveMask & DMP_FEATURE_MASK) {
                    //int64_t tempWanted;
                    //getDmpRate(&tempWanted);
                }

                inv_set_accel_sample_rate((int)wanted/1000LL);
                LOGV_IF(ENG_VERBOSE, "HAL:MPL accel sample rate: (mpl)=%d us",
                        int(wanted/1000LL));
                /* TODO: use function pointers to calculate delay value specific
                   to vendor */
                LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %.0f > %s (%lld)",
                        1000000000.f / wanted, mpu.accel_rate,
                        getTimestamp());
                tempFd = open(mpu.accel_rate, O_RDWR);
                if(USE_THIRD_PARTY_ACCEL == 1) {
                    //BMA250 in ms
                    res = write_attribute_sensor(tempFd, wanted / 1000000L);
                }
                else {
                    //MPUxxxx in hz
                    res = write_attribute_sensor(tempFd, 1000000000.f/wanted);
                }
                LOGE_IF(res < 0, "HAL:ACCEL update delay error");
            }

            /* Invensense compass calibration */
            if (M_ENABLED || RM_ENABLED) {
                int64_t compassWanted = (mDelays[MagneticField] <= mDelays[RawMagneticField]?
                    (mEnabled & (1 << MagneticField)? mDelays[MagneticField]: mDelays[RawMagneticField]):
                    (mEnabled & (1 << RawMagneticField)? mDelays[RawMagneticField]: mDelays[MagneticField]));
                if (!mCompassSensor->isIntegrated()) {
                    wanted = compassWanted;
                } else {
#if (0)
                    wanted = compassWanted;
#else
                    if (GY_ENABLED
                        && (mDelays[Gyro] < compassWanted)) {
                        wanted = mDelays[Gyro];
                    } else if (RGY_ENABLED
                               && mDelays[RawGyro] < compassWanted) {
                        wanted = mDelays[RawGyro];
                    } else if (A_ENABLED && mDelays[Accelerometer]
                                < compassWanted) {
                        wanted = mDelays[Accelerometer];
                    } else {
                        wanted = compassWanted;
                    }
#endif
                    LOGV_IF(ENG_VERBOSE, "mFeatureActiveMask=%016llx", mFeatureActiveMask);
                    if (mFeatureActiveMask & DMP_FEATURE_MASK) {
                        //int64_t tempWanted;
                        //getDmpRate(&tempWanted);
                    }
                }

                mCompassSensor->setDelay(ID_M, wanted);
                got = mCompassSensor->getDelay(ID_M);
                inv_set_compass_sample_rate(got / 1000);
                LOGV_IF(ENG_VERBOSE, "HAL:MPL compass sample rate: (mpl)=%d us",
                        int(got/1000LL));
            }
            
            if (PS_ENABLED) {
                int64_t pressureRate = mDelays[Pressure];
                LOGV_IF(ENG_VERBOSE, "mFeatureActiveMask=%016llx", mFeatureActiveMask);
                if (mFeatureActiveMask & DMP_FEATURE_MASK) {
                    //int64_t tempWanted;
                    //getDmpRate(&tempWanted);
                }

                mPressureSensor->setDelay(ID_PS, pressureRate);
                LOGE_IF(res < 0, "HAL:PRESSURE update delay error");
            }
        }

        } //end of non batch mode

        unsigned long sensors = mLocalSensorMask & mMasterSensorMask;
        if (sensors &
            (INV_THREE_AXIS_GYRO
                | INV_THREE_AXIS_ACCEL
                | (INV_THREE_AXIS_COMPASS * mCompassSensor->isIntegrated()
                | (INV_ONE_AXIS_PRESSURE * mPressureSensor->isIntegrated())))) {
            LOGV_IF(ENG_VERBOSE, "sensors=%lu", sensors);
            res = masterEnable(1);
            if(res < 0)
                return res;
        } else { // all sensors idle -> reduce power, unless DMP is needed
            LOGV_IF(ENG_VERBOSE, "mFeatureActiveMask=%016llx", mFeatureActiveMask);
            if(mFeatureActiveMask & DMP_FEATURE_MASK) {
                res = masterEnable(1);
                if(res < 0)
                    return res;
            }
        }
    }

    return res;
}

/* For Third Party Accel Input Subsystem Drivers only */
int MPLSensor::readAccelEvents(sensors_event_t* data, int count)
{
    VHANDLER_LOG;

    if (count < 1)
        return -EINVAL;

    ssize_t n = mAccelInputReader.fill(accel_fd);
    if (n < 0) {
        LOGE("HAL:missed accel events, exit");
        return n;
    }

    int numEventReceived = 0;
    input_event const* event;
    int nb, done = 0;

    while (done == 0 && count && mAccelInputReader.readEvent(&event)) {
        int type = event->type;
        if (type == EV_ABS) {
            if (event->code == EVENT_TYPE_ACCEL_X) {
                mPendingMask |= 1 << Accelerometer;
                mCachedAccelData[0] = event->value;
            } else if (event->code == EVENT_TYPE_ACCEL_Y) {
                mPendingMask |= 1 << Accelerometer;
                mCachedAccelData[1] = event->value;
            } else if (event->code == EVENT_TYPE_ACCEL_Z) {
                mPendingMask |= 1 << Accelerometer;
                mCachedAccelData[2] =event-> value;
            }
        } else if (type == EV_SYN) {
            done = 1;
            if (mLocalSensorMask & INV_THREE_AXIS_ACCEL) {
                inv_build_accel(mCachedAccelData, 0, getTimestamp());
            }
        } else {
            LOGE("HAL:AccelSensor: unknown event (type=%d, code=%d)",
                    type, event->code);
        }
        mAccelInputReader.next();
    }

    return numEventReceived;
}

/**
 *  Should be called after reading at least one of gyro
 *  compass or accel data. (Also okay for handling all of them).
 *  @returns 0, if successful, error number if not.
 */
int MPLSensor::readEvents(sensors_event_t* data, int count)
{
    VHANDLER_LOG;
 
    inv_execute_on_data();

    int numEventReceived = 0;

    long msg;
    msg = inv_get_message_level_0(1);
    if (msg) {
        if (msg & INV_MSG_MOTION_EVENT) {
            LOGV_IF(PROCESS_VERBOSE, "HAL:**** Motion ****\n");
        }
        if (msg & INV_MSG_NO_MOTION_EVENT) {
            LOGV_IF(PROCESS_VERBOSE, "HAL:***** No Motion *****\n");
            /* after the first no motion, the gyro should be
               calibrated well */
            mGyroAccuracy = SENSOR_STATUS_ACCURACY_HIGH;
            /* if gyros are on and we got a no motion, set a flag
               indicating that the cal file can be written. */
            mHaveGoodMpuCal = true;
        }
        if(msg & INV_MSG_NEW_AB_EVENT) {
            LOGV_IF(EXTRA_VERBOSE, "HAL:***** New Accel Bias *****\n");
            getAccelBias();
            mAccelAccuracy = inv_get_accel_accuracy();
        }
        if(msg & INV_MSG_NEW_GB_EVENT) {
            LOGV_IF(EXTRA_VERBOSE, "HAL:***** New Gyro Bias *****\n");
            getGyroBias();
            setGyroBias();
        }
        if(msg & INV_MSG_NEW_FGB_EVENT) {
            LOGV_IF(EXTRA_VERBOSE, "HAL:***** New Factory Gyro Bias *****\n");
            getFactoryGyroBias();
        }
        if(msg & INV_MSG_NEW_FAB_EVENT) {
            LOGV_IF(EXTRA_VERBOSE, "HAL:***** New Factory Accel Bias *****\n");
            getFactoryAccelBias();
        }
        if(msg & INV_MSG_NEW_CB_EVENT) {
            LOGV_IF(EXTRA_VERBOSE, "HAL:***** New Compass Bias *****\n");
            getCompassBias();
            mCompassAccuracy = inv_get_mag_accuracy();
        }
    }

    // handle flush complete event
    if(mFlushBatchSet && mFlushEnabled != -1) {
        sensors_event_t temp;
        int sendEvent = metaHandler(&temp, META_DATA_FLUSH_COMPLETE);
        if(sendEvent == 1 && count > 0) {
            *data++ = temp;
            count--;
            numEventReceived++;
        }
    }

    // handle partial packet read
    if (mSkipReadEvents)
        return numEventReceived;

    for (int i = 0; i < NumSensors; i++) {
        int update = 0;

        // handle step detector when ped_q is enabled
        if(mPedUpdate) {
            if (i == StepDetector) {
                update = readDmpPedometerEvents(data, count, ID_P,
                                                SENSOR_TYPE_STEP_DETECTOR, 1);
                mPedUpdate = 0;            
                if(update == 1 && count > 0) {
                    data->timestamp = mStepSensorTimestamp;
                    count--;
                    numEventReceived++;
                    continue;
                }
            } else {
                if (mPedUpdate == DATA_FORMAT_STEP) {
                    continue;
                }
            }
        }

        // load up virtual sensors
        if (mEnabled & (1 << i)) {
            update = CALL_MEMBER_FN(this, mHandlers[i])(mPendingEvents + i);
            mPendingMask |= (1 << i);

            if (update && (count > 0)) {
                *data++ = mPendingEvents[i];
                count--;
                numEventReceived++;
            }
        }
    }

    return numEventReceived;
}

// collect data for MPL (but NOT sensor service currently), from driver layer
void MPLSensor::buildMpuEvent(void)
{
    VHANDLER_LOG;

    mSkipReadEvents = 0;
    int64_t mGyroSensorTimestamp=0, mAccelSensorTimestamp=0, latestTimestamp=0;
    int lp_quaternion_on = 0, sixAxis_quaternion_on = 0,
        ped_quaternion_on = 0, ped_standalone_on = 0;
    size_t nbyte;
    unsigned short data_format = 0;
    int i, nb, mask = 0,
        sensors = ((mLocalSensorMask & INV_THREE_AXIS_GYRO)? 1 : 0) +
            ((mLocalSensorMask & INV_THREE_AXIS_ACCEL)? 1 : 0) +
            (((mLocalSensorMask & INV_THREE_AXIS_COMPASS)
                && mCompassSensor->isIntegrated())? 1 : 0) +
            ((mLocalSensorMask & INV_ONE_AXIS_PRESSURE)? 1 : 0);
    //LOGV("mLocalSensorMask=0x%lx", mLocalSensorMask);
    char *rdata = mIIOBuffer;
    ssize_t rsize = 0;
    size_t readCounter = 0;
    char *rdataP = NULL;

    /* 2 Bytes header + 6 Bytes x,y,z data | 8 bytes timestamp */
    nbyte= (BYTES_PER_SENSOR + 8) * sensors * 1;

    /* special case for 6 Axis or LPQ */
    /* 2 Bytes header + 4 Bytes x data + 2 Bytes n/a */
    /* 4 Bytes y data | 4 Bytes z data */
    /* 8 Bytes timestamp */
    if (isLowPowerQuatEnabled()) {
        lp_quaternion_on = checkLPQuaternion();
        if (lp_quaternion_on == 1) {
            nbyte += BYTES_QUAT_DATA;
        }
    }

    if ((sixAxis_quaternion_on = check6AxisQuatEnabled())) {
        // sixAxis is mutually exclusive to LPQ
        // and it is also never enabled when continuous data is enabled
        // mLocalSensorMask does not need to be accounted for here
        // because accel/gyro fifo are always turned off
        nbyte += BYTES_QUAT_DATA;
    }

    if ((ped_quaternion_on = checkPedQuatEnabled())) {
        nbyte += BYTES_PER_SENSOR_PACKET;
    }

    if ((ped_standalone_on = checkPedStandaloneEnabled())) {
        nbyte += BYTES_PER_SENSOR_PACKET;
    }

    if (checkBatchEnabled()) {
        nbyte = 24 - mLeftOverBufferSize;
    }

    /* check previous copied buffer */
    /* append with just read data */
    if (mLeftOverBufferSize > 0) {
        LOGV_IF(0, "append old buffer size=%d", mLeftOverBufferSize);
        memcpy(rdata, mLeftOverBuffer, mLeftOverBufferSize);
            LOGV_IF(0,
            "HAL:input retrieve rdata=:%d, %d, %d, %d,%d, %d, %d, %d,%d, %d, "
            "%d, %d,%d, %d, %d, %d\n",           
            rdata[0], rdata[1], rdata[2], rdata[3],
            rdata[4], rdata[5], rdata[6], rdata[7],
            rdata[8], rdata[9], rdata[10], rdata[11],
            rdata[12], rdata[13], rdata[14], rdata[15]);
    }
    rdataP = rdata + mLeftOverBufferSize;
    
    /* read expected number of bytes */
    rsize = read(iio_fd, rdataP, nbyte);
    if(rsize < 0) {
        /* IIO buffer might have old data.
           Need to flush it if no sensor is on, to avoid infinite 
           read loop.*/
        LOGE("HAL:input data file descriptor not available - (%s)", 
             strerror(errno));
        if (sensors == 0) {
            rsize = read(iio_fd, rdata, MAX_SUSPEND_BATCH_PACKET_SIZE);
        }
        return;
    }
#if 1    
    if((rsize + mLeftOverBufferSize) == 8) {
        /* store packet then return */      
        memcpy(mLeftOverBuffer, rdataP, rsize);
        mLeftOverBufferSize += rsize;      
        LOGV_IF(1, "HAL:input data has partial packet");
        LOGV_IF(1, "HAL:input data mLeftOverBufferSize=%d", mLeftOverBufferSize);
        LOGV_IF(1,
            "HAL:input catch up retrieve rdata=:%d, %d, %d, %d, %d, %d, %d, %d",
            mLeftOverBuffer[0], mLeftOverBuffer[1], mLeftOverBuffer[2], mLeftOverBuffer[3],
            mLeftOverBuffer[4], mLeftOverBuffer[5], mLeftOverBuffer[6], mLeftOverBuffer[7]);
        mSkipReadEvents = 1;
        return;                
    }
#endif
    /* reset data and count pointer */
    rdataP = rdata;
    if (checkBatchEnabled()) {
        readCounter = 24;
    }
    else {
        readCounter = rsize + mLeftOverBufferSize;
    }

#ifdef TESTING
    LOGV_IF(INPUT_DATA,
         "HAL:input rdataP:r=%ld, n=%d,"
         "%d, %d, %d, %d,%d, %d, %d, %d,%d, %d, %d, %d,%d, %d, %d, %d\n",
         rsize, nbyte,
         rdataP[0], rdataP[1], rdataP[2], rdataP[3],
         rdataP[4], rdataP[5], rdataP[6], rdataP[7],
         rdataP[8], rdataP[9], rdataP[10], rdataP[11],
         rdataP[12], rdataP[13], rdataP[14], rdataP[15]);
#endif

    LOGV_IF(INPUT_DATA && ENG_VERBOSE, 
            "HAL:input b=%d rdata= %d nbyte= %d rsize= %d readCounter= %d",
            checkBatchEnabled(), *((short *) rdata), nbyte, (int)rsize, readCounter);
    LOGV_IF(INPUT_DATA && ENG_VERBOSE, 
            "HAL:input sensors= %d, lp_q_on= %d, 6axis_q_on= %d, "
            "ped_q_on= %d, ped_standalone_on= %d",
            sensors, lp_quaternion_on, sixAxis_quaternion_on, ped_quaternion_on, 
            ped_standalone_on); 
 
    while (readCounter > 0) {
        // since copied buffer is already accounted for, reset left over size
        mLeftOverBufferSize = 0;        
        // clear data format mask for parsing the next set of data
        mask = 0;
        data_format = *((short *)(rdata));
        LOGV_IF(INPUT_DATA && ENG_VERBOSE, 
                "HAL:input data_format=%x", data_format);

        if ((data_format & ~DATA_FORMAT_MASK) || (data_format == 0)) {
            LOGE("HAL:input invalid data_format 0x%02X", data_format);
            return;
        }

        if (data_format & DATA_FORMAT_STEP) {
            if (data_format == DATA_FORMAT_STEP) {
                rdata += BYTES_PER_SENSOR;
                latestTimestamp = *((long long*) (rdata));
                LOGV_IF(ENG_VERBOSE, "STEP DETECTED:0x%x - ts: %lld", data_format, latestTimestamp);
                // readCounter is decrement by 24 because DATA_FORMAT_STEP only applies in batch  mode
                readCounter -= BYTES_PER_SENSOR_PACKET;
            } else {
                LOGV_IF(0, "STEP DETECTED:0x%x", data_format);
            }
            mPedUpdate |= data_format;
            mask |= DATA_FORMAT_STEP;
            // cancels step bit
            data_format &= (~DATA_FORMAT_STEP);
        }

        if (data_format & DATA_FORMAT_MARKER) {
            LOGV_IF(ENG_VERBOSE, "MARKER DETECTED:0x%x", data_format);
            // cancels marker bit
            data_format &= (~DATA_FORMAT_MARKER);
            mFlushBatchSet = 1; 
        }
        
        if (data_format == DATA_FORMAT_QUAT) {
            mCachedQuaternionData[0] = *((int *) (rdata + 4));
            mCachedQuaternionData[1] = *((int *) (rdata + 8));
            mCachedQuaternionData[2] = *((int *) (rdata + 12));
            rdata += QUAT_ONLY_LAST_PACKET_OFFSET;
            mQuatSensorTimestamp = *((long long*) (rdata));
            mask |= DATA_FORMAT_QUAT;
            readCounter -= BYTES_QUAT_DATA;
        }
        else if (data_format == DATA_FORMAT_6_AXIS) {
            mCached6AxisQuaternionData[0] = *((int *) (rdata + 4));
            mCached6AxisQuaternionData[1] = *((int *) (rdata + 8));
            mCached6AxisQuaternionData[2] = *((int *) (rdata + 12));
            rdata += QUAT_ONLY_LAST_PACKET_OFFSET;
            mQuatSensorTimestamp = *((long long*) (rdata));
            mask |= DATA_FORMAT_6_AXIS;
            readCounter -= BYTES_QUAT_DATA;
        }
        else if (data_format == DATA_FORMAT_PED_QUAT) {
            mCachedPedQuaternionData[0] = *((short *) (rdata + 2));
            mCachedPedQuaternionData[1] = *((short *) (rdata + 4));
            mCachedPedQuaternionData[2] = *((short *) (rdata + 6));
            rdata += BYTES_PER_SENSOR;
            mQuatSensorTimestamp = *((long long*) (rdata));
            mask |= DATA_FORMAT_PED_QUAT;
            readCounter -= BYTES_PER_SENSOR_PACKET;
        }
        else if (data_format == DATA_FORMAT_PED_STANDALONE) {
            LOGV_IF(ENG_VERBOSE, "STEP DETECTED:0x%x", data_format);
            rdata += BYTES_PER_SENSOR;
            mStepSensorTimestamp = *((long long*) (rdata));
            mask |= DATA_FORMAT_PED_STANDALONE;
            readCounter -= BYTES_PER_SENSOR_PACKET;
            mPedUpdate |= data_format;
        }
        else if (data_format == DATA_FORMAT_GYRO) {
            mCachedGyroData[0] = *((short *) (rdata + 2));
            mCachedGyroData[1] = *((short *) (rdata + 4));
            mCachedGyroData[2] = *((short *) (rdata + 6));
            rdata += BYTES_PER_SENSOR;
            mGyroSensorTimestamp = *((long long*) (rdata));
            mask |= DATA_FORMAT_GYRO;
            readCounter -= BYTES_PER_SENSOR_PACKET;
        }
        else if (data_format == DATA_FORMAT_ACCEL) {
            mCachedAccelData[0] = *((short *) (rdata + 2));
            mCachedAccelData[1] = *((short *) (rdata + 4));
            mCachedAccelData[2] = *((short *) (rdata + 6));
            rdata += BYTES_PER_SENSOR;
            mAccelSensorTimestamp = *((long long*) (rdata));
            mask |= DATA_FORMAT_ACCEL;
            readCounter -= BYTES_PER_SENSOR_PACKET;
        }
        else  if (data_format == DATA_FORMAT_COMPASS) {
            if (mCompassSensor->isIntegrated()) {
                mCachedCompassData[0] = *((short *) (rdata + 2));
                mCachedCompassData[1] = *((short *) (rdata + 4));
                mCachedCompassData[2] = *((short *) (rdata + 6));
                rdata += BYTES_PER_SENSOR;
                mCompassTimestamp = *((long long*) (rdata));
                mask |= DATA_FORMAT_COMPASS;
                readCounter -= BYTES_PER_SENSOR_PACKET;
            }
        }
        else if (data_format == DATA_FORMAT_PRESSURE) {
            if (mPressureSensor->isIntegrated()) {
                mCachedPressureData = 
                    ((*((short *)(rdata + 4))) << 16) + 
                    (*((unsigned short *) (rdata + 6)));
                rdata += BYTES_PER_SENSOR;
                mPressureTimestamp = *((long long*) (rdata));
                if (mCachedPressureData != 0) {
                    mask |= DATA_FORMAT_PRESSURE;
                }
                readCounter -= BYTES_PER_SENSOR_PACKET;
            }
        }
        rdata += BYTES_PER_SENSOR;

        size_t storeBufferSize = 0;
        if (checkBatchEnabled()) {
            storeBufferSize = 8;
        } else {
            storeBufferSize = 24;
        }
        /* read ahead and store left over data if any */
        if ((readCounter != 0) && ((checkBatchEnabled() && (rsize == (ssize_t)nbyte)) ||
                                   (!checkBatchEnabled() && (rsize != (ssize_t)nbyte)))
                               &&(readCounter <= storeBufferSize)) {
            int currentBufferCounter = 0;
            LOGV_IF(0, "!!! not enough data readCounter=%d, expected nbyte=%d, rsize=%d", readCounter, nbyte, (int)rsize);
            memcpy(mLeftOverBuffer, rdata, readCounter);
            LOGV_IF(0, 
                    "HAL:input store rdata=:%d, %d, %d, %d,%d, %d, %d, %d,%d, "
                    "%d, %d, %d,%d, %d, %d, %d\n",
                    mLeftOverBuffer[0], mLeftOverBuffer[1], mLeftOverBuffer[2], mLeftOverBuffer[3],
                    mLeftOverBuffer[4], mLeftOverBuffer[5], mLeftOverBuffer[6], mLeftOverBuffer[7],
                    mLeftOverBuffer[8], mLeftOverBuffer[9], mLeftOverBuffer[10], mLeftOverBuffer[11],
                    mLeftOverBuffer[12],mLeftOverBuffer[13],mLeftOverBuffer[14], mLeftOverBuffer[15]);

            mLeftOverBufferSize = readCounter;
            readCounter = 0;
            LOGV_IF(0, "!!! stored number of bytes:%d", mLeftOverBufferSize);            
        } else if (checkBatchEnabled()) {
            /* reset count since this is the last packet for the data set */
            readCounter = 0;
            mLeftOverBufferSize = 0;
        }

        /* handle data read */
        if (mask & DATA_FORMAT_GYRO) {
            /* batch mode does not batch temperature */
            /* disable temperature read */
            if (!(mFeatureActiveMask & INV_DMP_BATCH_MODE)) {
                // send down temperature every 0.5 seconds
                // with timestamp measured in "driver" layer
                if(mGyroSensorTimestamp - mTempCurrentTime >= 500000000LL) {
                    mTempCurrentTime = mGyroSensorTimestamp;
                    long long temperature[2];
                    if(inv_read_temperature(temperature) == 0) {
                        LOGV_IF(INPUT_DATA,
                        "HAL:input inv_read_temperature = %lld, timestamp= %lld",
                        temperature[0], temperature[1]);
                        inv_build_temp(temperature[0], temperature[1]);
                     }
#ifdef TESTING
                    long bias[3], temp, temp_slope[3];
                    inv_get_mpl_gyro_bias(bias, &temp);
                    inv_get_gyro_ts(temp_slope);
                    LOGI("T: %.3f "
                     "GB: %+13f %+13f %+13f "
                     "TS: %+13f %+13f %+13f "
                     "\n",
                     (float)temperature[0] / 65536.f,
                     (float)bias[0] / 65536.f / 16.384f,
                     (float)bias[1] / 65536.f / 16.384f,
                     (float)bias[2] / 65536.f / 16.384f,
                     temp_slope[0] / 65536.f,
                     temp_slope[1] / 65536.f,
                     temp_slope[2] / 65536.f);
#endif
                }
            }
            mPendingMask |= 1 << Gyro;
            mPendingMask |= 1 << RawGyro;

            if (mLocalSensorMask & INV_THREE_AXIS_GYRO) {
                inv_build_gyro(mCachedGyroData, mGyroSensorTimestamp);
                LOGV_IF(INPUT_DATA, 
                        "HAL:input inv_build_gyro: %+8d %+8d %+8d - %lld",
                        mCachedGyroData[0], mCachedGyroData[1],
                        mCachedGyroData[2], mGyroSensorTimestamp);
           }
           latestTimestamp = mGyroSensorTimestamp;
        }

        if (mask & DATA_FORMAT_ACCEL) {
            mPendingMask |= 1 << Accelerometer;
            if (mLocalSensorMask & INV_THREE_AXIS_ACCEL) {
                inv_build_accel(mCachedAccelData, 0, mAccelSensorTimestamp);
                 LOGV_IF(INPUT_DATA,
                    "HAL:input inv_build_accel: %+8ld %+8ld %+8ld - %lld",
                    mCachedAccelData[0], mCachedAccelData[1],
                    mCachedAccelData[2], mAccelSensorTimestamp);
                /* remember inital 6 axis quaternion */
                inv_time_t tempTimestamp;
                inv_get_6axis_quaternion(mInitial6QuatValue, &tempTimestamp);
                if (mInitial6QuatValue[0] != 0 || mInitial6QuatValue[1] != 0 ||
                        mInitial6QuatValue[2] != 0 || mInitial6QuatValue[3] != 0) {
                    mInitial6QuatValueAvailable = 1;
                    LOGV_IF(INPUT_DATA && ENG_VERBOSE,
                        "HAL:input build 6q init: %+8ld %+8ld %+8ld %+8ld",
                        mInitial6QuatValue[0], mInitial6QuatValue[1],
                        mInitial6QuatValue[2], mInitial6QuatValue[3]);
                }
                        
            }
            latestTimestamp = mAccelSensorTimestamp;
        }

        if ((mask & DATA_FORMAT_COMPASS) && mCompassSensor->isIntegrated()) {
            int status = 0;
            if (mCompassSensor->providesCalibration()) {
                status = mCompassSensor->getAccuracy();
                status |= INV_CALIBRATED;
            }
            if (mLocalSensorMask & INV_THREE_AXIS_COMPASS) {
            inv_build_compass(mCachedCompassData, status,
                              mCompassTimestamp);
            LOGV_IF(INPUT_DATA,
                    "HAL:input inv_build_compass: %+8ld %+8ld %+8ld - %lld",
                    mCachedCompassData[0], mCachedCompassData[1],
                    mCachedCompassData[2], mCompassTimestamp);
            }
            latestTimestamp = mCompassTimestamp;
        }

        if (isLowPowerQuatEnabled() && lp_quaternion_on == 1
                                && (mask & DATA_FORMAT_QUAT)) {
            /* if bias was applied to DMP bias,
               set status bits to disable gyro bias cal */
            int status = 0;
            if (mGyroBiasApplied == true) {
                LOGV_IF(INPUT_DATA && ENG_VERBOSE, "HAL:input dmp bias is used");
                status |= INV_BIAS_APPLIED;
            }
            status |= 32 | INV_QUAT_3AXIS; /* default 32 (16/32bits) */
            inv_build_quat(mCachedQuaternionData,
                       status,
                       mQuatSensorTimestamp);
            LOGV_IF(INPUT_DATA,
                    "HAL:input inv_build_quat-3x: %+8ld %+8ld %+8ld - %lld",
                    mCachedQuaternionData[0], mCachedQuaternionData[1],
                    mCachedQuaternionData[2],
                    mQuatSensorTimestamp);
            latestTimestamp = mQuatSensorTimestamp;
        }

        if ((mask & DATA_FORMAT_6_AXIS) && check6AxisQuatEnabled()
                                    && (sixAxis_quaternion_on == 1)) {
            /* if bias was applied to DMP bias,
               set status bits to disable gyro bias cal */
            int status = 0;
            if (mGyroBiasApplied == true) {
                LOGV_IF(INPUT_DATA && ENG_VERBOSE, "HAL:input dmp bias is used");
                status |= INV_QUAT_6AXIS;
            }
            status |= 32 | INV_QUAT_3AXIS; /* default 32 (16/32bits) */
            inv_build_quat(mCached6AxisQuaternionData,
                       status,
                       mQuatSensorTimestamp);
            LOGV_IF(INPUT_DATA,
                    "HAL:input inv_build_quat-6x: %+8ld %+8ld %+8ld - %lld",
                    mCached6AxisQuaternionData[0], mCached6AxisQuaternionData[1],
                    mCached6AxisQuaternionData[2], mQuatSensorTimestamp);
            latestTimestamp = mQuatSensorTimestamp;
        }

        if ((mask & DATA_FORMAT_PED_QUAT) && checkPedQuatEnabled()
                                          && (ped_quaternion_on == 1)) {
            /* if bias was applied to DMP bias,
               set status bits to disable gyro bias cal */
            int status = 0;
            if (mGyroBiasApplied == true) {
                LOGV_IF(INPUT_DATA && ENG_VERBOSE, 
                        "HAL:input dmp bias is used");
                status |= INV_QUAT_6AXIS;
            }
            status |= 32 | INV_QUAT_3AXIS; /* default 32 (16/32bits) */
            mCachedPedQuaternionData[0] = mCachedPedQuaternionData[0] << 16;
            mCachedPedQuaternionData[1] = mCachedPedQuaternionData[1] << 16;
            mCachedPedQuaternionData[2] = mCachedPedQuaternionData[2] << 16;
            inv_build_quat(mCachedPedQuaternionData,
                       status,
                       mQuatSensorTimestamp);

            LOGV_IF(INPUT_DATA,
                    "HAL:HAL:input inv_build_quat-ped_6x: %+8ld %+8ld %+8ld - %lld",
                    mCachedPedQuaternionData[0], mCachedPedQuaternionData[1],
                    mCachedPedQuaternionData[2], mQuatSensorTimestamp);
            latestTimestamp = mQuatSensorTimestamp;
        }

        if ((mask & DATA_FORMAT_PRESSURE) && mPressureSensor->isIntegrated()) {
            int status = 0;
            if (mLocalSensorMask & INV_ONE_AXIS_PRESSURE) {
                latestTimestamp = mPressureTimestamp;
                mPressureUpdate = 1;
                inv_build_pressure(mCachedPressureData,
                            status,
                            mPressureTimestamp);           
                LOGV_IF(INPUT_DATA,
                    "HAL:input inv_build_pressure: %+8ld - %lld",
                    mCachedPressureData, mPressureTimestamp);                            
            }
        }

        /* take the latest timestamp */
        if (mask & DATA_FORMAT_STEP) {
        /* work around driver output duplicate step detector bit */
            if (latestTimestamp > mStepSensorTimestamp) {
                mStepSensorTimestamp = latestTimestamp;
                LOGV_IF(INPUT_DATA,
                    "HAL:input build step: 1 - %lld", mStepSensorTimestamp);                    
            } else {
                mPedUpdate = 0;
            }
        }
   }    //while end
}

/* use for both MPUxxxx and third party compass */
void MPLSensor::buildCompassEvent(void)
{
    VHANDLER_LOG;

    int done = 0;

    // pthread_mutex_lock(&mMplMutex);
    // pthread_mutex_lock(&mHALMutex);

    done = mCompassSensor->readSample(mCachedCompassData, &mCompassTimestamp);
    if(mCompassSensor->isYasCompass()) {
        if (mCompassSensor->checkCoilsReset() == 1) {
           //Reset relevant compass settings
           resetCompass();
        }
    }
    if (done > 0) {
        int status = 0;
        if (mCompassSensor->providesCalibration()) {
            status = mCompassSensor->getAccuracy();
            status |= INV_CALIBRATED;
        }
        if (mLocalSensorMask & INV_THREE_AXIS_COMPASS) {
            inv_build_compass(mCachedCompassData, status,
                              mCompassTimestamp);
            LOGV_IF(INPUT_DATA,
                    "HAL:input inv_build_compass: %+8ld %+8ld %+8ld - %lld",
                    mCachedCompassData[0], mCachedCompassData[1],
                    mCachedCompassData[2], mCompassTimestamp);
        }
    }

    // pthread_mutex_unlock(&mMplMutex);
    // pthread_mutex_unlock(&mHALMutex);
}

int MPLSensor::resetCompass(void)
{
    VFUNC_LOG;

    //Reset compass cal if enabled
    if (mMplFeatureActiveMask & INV_COMPASS_CAL) {
       LOGV_IF(EXTRA_VERBOSE, "HAL:Reset compass cal");
       inv_init_vector_compass_cal();
    }

    //Reset compass fit if enabled
    if (mMplFeatureActiveMask & INV_COMPASS_FIT) {
       LOGV_IF(EXTRA_VERBOSE, "HAL:Reset compass fit");
       inv_init_compass_fit();
    }

    return 0;
}

int MPLSensor::getFd(void) const
{
    VFUNC_LOG;
    LOGV_IF(EXTRA_VERBOSE, "getFd returning %d", iio_fd);
    return iio_fd;
}

int MPLSensor::getAccelFd(void) const
{
    VFUNC_LOG;
    LOGV_IF(EXTRA_VERBOSE, "getAccelFd returning %d", accel_fd);
    return accel_fd;
}

int MPLSensor::getCompassFd(void) const
{
    VFUNC_LOG;
    int fd = mCompassSensor->getFd();
    LOGV_IF(EXTRA_VERBOSE, "getCompassFd returning %d", fd);
    return fd;
}

int MPLSensor::turnOffAccelFifo(void)
{
    VFUNC_LOG;
    int i, res = 0, tempFd;
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                        0, mpu.accel_fifo_enable, getTimestamp());
    res += write_sysfs_int(mpu.accel_fifo_enable, 0);
    return res;
}

int MPLSensor::turnOffGyroFifo(void)
{
    VFUNC_LOG;
    int i, res = 0, tempFd;
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                        0, mpu.gyro_fifo_enable, getTimestamp());
    res += write_sysfs_int(mpu.gyro_fifo_enable, 0);
    return res;
}

int MPLSensor::enableDmpOrientation(int en)
{
    VFUNC_LOG;
    int res = 0;
    int enabled_sensors = mEnabled;

    if (isMpuNonDmp())
        return res;

    // reset master enable
    res = masterEnable(0);
    if (res < 0)
        return res;

    if (en == 1) {
        //Enable DMP orientation
        LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                en, mpu.display_orientation_on, getTimestamp());
        if (write_sysfs_int(mpu.display_orientation_on, en) < 0) {
            LOGE("HAL:ERR can't enable Android orientation");
            res = -1;	// indicate an err
            return res;
        }

        // enable DMP
        res = onDmp(1);
        if (res < 0)
            return res;

        // set rate to 200Hz
        LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                200, mpu.accel_fifo_rate, getTimestamp());
        if (write_sysfs_int(mpu.accel_fifo_rate, 200) < 0) {
            res = -1;
            LOGE("HAL:ERR can't set rate to 200Hz");
            return res;
        }

        // enable accel engine
        res = enableAccel(1);
        if (res < 0)
            return res;

        // disable accel FIFO
        if (!(mLocalSensorMask & mMasterSensorMask & INV_THREE_AXIS_ACCEL)) {
            res = turnOffAccelFifo();
            if (res < 0)
                return res;
        }

        if (!mEnabled){
            LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                       1, mpu.dmp_event_int_on, getTimestamp());
            if (write_sysfs_int(mpu.dmp_event_int_on, en) < 0) {
                res = -1;
                LOGE("HAL:ERR can't enable DMP event interrupt");
            }
        }

        mFeatureActiveMask |= INV_DMP_DISPL_ORIENTATION;
        LOGV_IF(ENG_VERBOSE, "mFeatureActiveMask=%016llx", mFeatureActiveMask);
    } else {
        mFeatureActiveMask &= ~INV_DMP_DISPL_ORIENTATION;
        // disable DMP
        if (mFeatureActiveMask == 0) {
            res = onDmp(0);
            if (res < 0)
                return res;

            // disable accel engine
            if (!(mLocalSensorMask & mMasterSensorMask
                    & INV_THREE_AXIS_ACCEL)) {
                res = enableAccel(0);
                if (res < 0)
                    return res;
            }
        }
        
        if (mEnabled){
            LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                       en, mpu.dmp_event_int_on, getTimestamp());
            if (write_sysfs_int(mpu.dmp_event_int_on, en) < 0) {
                res = -1;
                LOGE("HAL:ERR can't enable DMP event interrupt");
            }
        }
        LOGV_IF(ENG_VERBOSE, "mFeatureActiveMask=%016llx", mFeatureActiveMask);
    }

    if (en || mEnabled || mFeatureActiveMask) {
        res = masterEnable(1);
    }
    return res;
}

int MPLSensor::openDmpOrientFd(void)
{
    VFUNC_LOG;

    if (!isDmpDisplayOrientationOn() || dmp_orient_fd >= 0) {
        LOGV_IF(PROCESS_VERBOSE,
                "HAL:DMP display orientation disabled or file desc opened");
        return 0;
    }

    dmp_orient_fd = open(mpu.event_display_orientation, O_RDONLY| O_NONBLOCK);
    if (dmp_orient_fd < 0) {
        LOGE("HAL:ERR couldn't open dmpOrient node");
        return -1;
    } else {
        LOGV_IF(PROCESS_VERBOSE,
                "HAL:dmp_orient_fd opened : %d", dmp_orient_fd);
        return 0;
    }
}

int MPLSensor::closeDmpOrientFd(void)
{
    VFUNC_LOG;
    if (dmp_orient_fd >= 0)
        close(dmp_orient_fd);
    return 0;
}

int MPLSensor::dmpOrientHandler(int orient)
{
    VFUNC_LOG;
    LOGV_IF(PROCESS_VERBOSE, "HAL:orient %x", orient);
    return 0;
}

int MPLSensor::readDmpOrientEvents(sensors_event_t* data, int count)
{
    VFUNC_LOG;

    char dummy[4];
    int screen_orientation = 0;
    FILE *fp;

    fp = fopen(mpu.event_display_orientation, "r");
    if (fp == NULL) {
        LOGE("HAL:cannot open event_display_orientation");
        return 0;
    } else {
        if (fscanf(fp, "%d\n", &screen_orientation) < 0 || fclose(fp) < 0)
        {
            LOGE("HAL:cannot write event_display_orientation");
        }
    }

    int numEventReceived = 0;

    if (mDmpOrientationEnabled && count > 0) {
        sensors_event_t temp;

        temp.acceleration.x = 0;
        temp.acceleration.y = 0;
        temp.acceleration.z = 0;
        temp.version = sizeof(sensors_event_t);
        temp.sensor = ID_SO;
        temp.acceleration.status
            = SENSOR_STATUS_UNRELIABLE;
#ifdef ENABLE_DMP_SCREEN_AUTO_ROTATION
        temp.type = SENSOR_TYPE_SCREEN_ORIENTATION;
        temp.screen_orientation = screen_orientation;
#endif
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        temp.timestamp = (int64_t) ts.tv_sec * 1000000000 + ts.tv_nsec;

        *data++ = temp;
        count--;
        numEventReceived++;
    }

    // read dummy data per driver's request
    dmpOrientHandler(screen_orientation);
    read(dmp_orient_fd, dummy, 4);

    return numEventReceived;
}

int MPLSensor::getDmpOrientFd(void)
{
    VFUNC_LOG;

    LOGV_IF(EXTRA_VERBOSE, "getDmpOrientFd returning %d", dmp_orient_fd);
    return dmp_orient_fd;

}

int MPLSensor::checkDMPOrientation(void)
{
    VFUNC_LOG;
    return ((mFeatureActiveMask & INV_DMP_DISPL_ORIENTATION) ? 1 : 0);
}

int MPLSensor::getDmpRate(int64_t *wanted)
{
    VFUNC_LOG;

      // set DMP output rate to FIFO
      if(mDmpOn == 1) {
        LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                int(1000000000.f / *wanted), mpu.three_axis_q_rate,
                getTimestamp());
        write_sysfs_int(mpu.three_axis_q_rate, 1000000000.f / *wanted);
        LOGV_IF(PROCESS_VERBOSE,
                    "HAL:DMP three axis rate %.2f Hz", 1000000000.f / *wanted);
        if (mFeatureActiveMask & INV_DMP_BATCH_MODE) {
            LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                    int(1000000000.f / *wanted), mpu.six_axis_q_rate,
                    getTimestamp());
            write_sysfs_int(mpu.six_axis_q_rate, 1000000000.f / *wanted);
            LOGV_IF(PROCESS_VERBOSE,
                    "HAL:DMP six axis rate %.2f Hz", 1000000000.f / *wanted);
                    
            LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                    int(1000000000.f / *wanted), mpu.ped_q_rate,
                    getTimestamp());
            write_sysfs_int(mpu.ped_q_rate, 1000000000.f / *wanted);
            LOGV_IF(PROCESS_VERBOSE,
                    "HAL:DMP ped quaternion rate %.2f Hz", 1000000000.f / *wanted);
        }
        //DMP running rate must be @ 200Hz
        *wanted= RATE_200HZ;
        LOGV_IF(PROCESS_VERBOSE,
                "HAL:DMP rate= %.2f Hz", 1000000000.f / *wanted);
    }
    return 0;
}

int MPLSensor::getPollTime(void)
{
    VFUNC_LOG;
    return mPollTime;
}

int MPLSensor::getStepCountPollTime(void)
{
    VFUNC_LOG;
    if (mDmpStepCountEnabled) {
        /* clamped to 1ms?, still rather large */
        LOGV_IF(0/*EXTRA_VERBOSE*/, "Step Count poll time = %lld ms",
                mStepCountPollTime / 1000000LL);
        return (mStepCountPollTime / 1000000LL);
    }
    return 1000;
}

bool MPLSensor::hasStepCountPendingEvents(void)
{
    VFUNC_LOG;
    if (mDmpStepCountEnabled) {
        struct timespec t_now;
        int64_t interval = 0;

        clock_gettime(CLOCK_MONOTONIC, &t_now);
        interval = ((int64_t(t_now.tv_sec) * 1000000000LL + t_now.tv_nsec) -
                    (int64_t(mt_pre.tv_sec) * 1000000000LL + mt_pre.tv_nsec));

        if (interval < mStepCountPollTime) {
            LOGV_IF(0/*ENG_VERBOSE*/,
                    "Step Count interval elapsed: %lld, triggered: %d",
                    interval, mStepCountPollTime);
            return false;
        } else {
            clock_gettime(CLOCK_MONOTONIC, &mt_pre);
            LOGV_IF(0/*ENG_VERBOSE*/, "Step Count previous time: %ld ms",
                    mt_pre.tv_nsec / 1000);
            return true;
        }
    }
    return false;
}

bool MPLSensor::hasPendingEvents(void) const
{
    VFUNC_LOG;
    // if we are using the polling workaround, force the main
    // loop to check for data every time
    return (mPollTime != -1);
}

/* TODO: support resume suspend when we gain more info about them*/
void MPLSensor::sleepEvent(void)
{
    VFUNC_LOG;
}

void MPLSensor::wakeEvent(void)
{
    VFUNC_LOG;
}

int MPLSensor::inv_float_to_q16(float *fdata, long *ldata)
{
    //VFUNC_LOG;

    if (!fdata || !ldata)
        return -1;
    ldata[0] = (long)(fdata[0] * 65536.f);
    ldata[1] = (long)(fdata[1] * 65536.f);
    ldata[2] = (long)(fdata[2] * 65536.f);
    return 0;
}

int MPLSensor::inv_long_to_q16(long *fdata, long *ldata)
{
    //VFUNC_LOG;

    if (!fdata || !ldata)
        return -1;
    ldata[0] = (fdata[1] * 65536.f);
    ldata[1] = (fdata[2] * 65536.f);
    ldata[2] = (fdata[3] * 65536.f);
    return 0;
}

int MPLSensor::inv_float_to_round(float *fdata, long *ldata)
{
    VHANDLER_LOG;

    if (!fdata || !ldata)
            return -1;
    ldata[0] = (long)fdata[0];
    ldata[1] = (long)fdata[1];
    ldata[2] = (long)fdata[2];
    return 0;
}

int MPLSensor::inv_float_to_round2(float *fdata, short *ldata)
{
    //VFUNC_LOG;

    if (!fdata || !ldata)
        return -1;
    ldata[0] = (short)fdata[0];
    ldata[1] = (short)fdata[1];
    ldata[2] = (short)fdata[2];
    return 0;
}

int MPLSensor::inv_long_to_float(long *ldata, float *fdata)
{
    //VFUNC_LOG;

    if (!ldata || !fdata)
        return -1;
    fdata[0] = (float)ldata[0];
    fdata[1] = (float)ldata[1];
    fdata[2] = (float)ldata[2];
    return 0;
}

int MPLSensor::inv_read_temperature(long long *data)
{
    VHANDLER_LOG;

    int count = 0;
    char raw_buf[40];
    long raw = 0;

    long long timestamp = 0;

    memset(raw_buf, 0, sizeof(raw_buf));
    count = read_attribute_sensor(gyro_temperature_fd, raw_buf,
                                  sizeof(raw_buf));
    if(count < 1) {
        LOGE("HAL:error reading gyro temperature");
        return -1;
    }

    count = sscanf(raw_buf, "%ld%lld", &raw, &timestamp);

    if(count < 0) {
        return -1;
    }

    LOGV_IF(ENG_VERBOSE,
            "HAL:temperature raw = %ld, timestamp = %lld, count = %d",
            raw, timestamp, count);
    data[0] = raw;
    data[1] = timestamp;

    return 0;
}

int MPLSensor::inv_read_dmp_state(int fd)
{
    VFUNC_LOG;

    if(fd < 0)
        return -1;

    int count = 0;
    char raw_buf[10];
    short raw = 0;

    memset(raw_buf, 0, sizeof(raw_buf));
    count = read_attribute_sensor(fd, raw_buf, sizeof(raw_buf));
    if(count < 1) {
        LOGE("HAL:error reading dmp state");
        close(fd);
        return -1;
    }
    count = sscanf(raw_buf, "%hd", &raw);
    if(count < 0) {
        LOGE("HAL:dmp state data is invalid");
        close(fd);
        return -1;
    }
    LOGV_IF(EXTRA_VERBOSE, "HAL:dmp state = %d, count = %d", raw, count);
    close(fd);
    return (int)raw;
}

int MPLSensor::inv_read_sensor_bias(int fd, long *data)
{
    VFUNC_LOG;

    if(fd == -1) {
        return -1;
    }

    char buf[50];
    char x[15], y[15], z[15];

    memset(buf, 0, sizeof(buf));
    int count = read_attribute_sensor(fd, buf, sizeof(buf));
    if(count < 1) {
        LOGE("HAL:Error reading gyro bias");
        return -1;
    }
    count = sscanf(buf, "%[^','],%[^','],%[^',']", x, y, z);
    if(count) {
        /* scale appropriately for MPL */
        LOGV_IF(ENG_VERBOSE,
                "HAL:pre-scaled bias: X:Y:Z (%ld, %ld, %ld)",
                atol(x), atol(y), atol(z));

        data[0] = (long)(atol(x) / 10000 * (1L << 16));
        data[1] = (long)(atol(y) / 10000 * (1L << 16));
        data[2] = (long)(atol(z) / 10000 * (1L << 16));

        LOGV_IF(ENG_VERBOSE,
                "HAL:scaled bias: X:Y:Z (%ld, %ld, %ld)",
                data[0], data[1], data[2]);
    }
    return 0;
}

/** fill in the sensor list based on which sensors are configured.
 *  return the number of configured sensors.
 *  parameter list must point to a memory region of at least 7*sizeof(sensor_t)
 *  parameter len gives the length of the buffer pointed to by list
 */
int MPLSensor::populateSensorList(struct sensor_t *list, int len)
{
    VFUNC_LOG;

    int numsensors;

    if(len <
        (int)((sizeof(sSensorList) / sizeof(sensor_t)) * sizeof(sensor_t))) {
        LOGE("HAL:sensor list too small, not populating.");
        return -(sizeof(sSensorList) / sizeof(sensor_t));
    }

    /* fill in the base values */
    memcpy(list, sSensorList,
           sizeof (struct sensor_t) * (sizeof(sSensorList) / sizeof(sensor_t)));

    /* first add gyro, accel and compass to the list */

    /* fill in gyro/accel values */
    if(chip_ID == NULL) {
        LOGE("HAL:Can not get gyro/accel id");
    }
    fillGyro(chip_ID, list);
    fillAccel(chip_ID, list);

    // TODO: need fixes for unified HAL and 3rd-party solution
    mCompassSensor->fillList(&list[MagneticField]);
    mCompassSensor->fillList(&list[RawMagneticField]);

    if (mPressureSensor != NULL) {
        mPressureSensor->fillList(&list[Pressure]);
    }

    if(1) {
        numsensors = (sizeof(sSensorList) / sizeof(sensor_t));
        /* all sensors will be added to the list
           fill in orientation values */
        fillOrientation(list);
        /* fill in rotation vector values */
        fillRV(list);
        /* fill in game rotation vector values */
        fillGRV(list);
        /* fill in gravity values */
        fillGravity(list);
        /* fill in Linear accel values */
        fillLinearAccel(list);
        /* fill in Significant motion values */
        fillSignificantMotion(list);
#ifdef ENABLE_DMP_SCREEN_AUTO_ROTATION
        /* fill in screen orientation values */
        fillScreenOrientation(list);
#endif
    } else {
        /* no 9-axis sensors, zero fill that part of the list */
        numsensors = 3;
        memset(list + 3, 0, 4 * sizeof(struct sensor_t));
    }

    return numsensors;
}

void MPLSensor::fillAccel(const char* accel, struct sensor_t *list)
{
    VFUNC_LOG;

    if (accel) {
        if(accel != NULL && strcmp(accel, "BMA250") == 0) {
            list[Accelerometer].maxRange = ACCEL_BMA250_RANGE;
            list[Accelerometer].resolution = ACCEL_BMA250_RESOLUTION;
            list[Accelerometer].power = ACCEL_BMA250_POWER;
            list[Accelerometer].minDelay = ACCEL_BMA250_MINDELAY;
            return;
        } else if (accel != NULL && strcmp(accel, "MPU6050") == 0) {
            list[Accelerometer].maxRange = ACCEL_MPU6050_RANGE;
            list[Accelerometer].resolution = ACCEL_MPU6050_RESOLUTION;
            list[Accelerometer].power = ACCEL_MPU6050_POWER;
            list[Accelerometer].minDelay = ACCEL_MPU6050_MINDELAY;
            return;
        } else if (accel != NULL && strcmp(accel, "MPU6500") == 0) {
            list[Accelerometer].maxRange = ACCEL_MPU6500_RANGE;
            list[Accelerometer].resolution = ACCEL_MPU6500_RESOLUTION;
            list[Accelerometer].power = ACCEL_MPU6500_POWER;
            list[Accelerometer].minDelay = ACCEL_MPU6500_MINDELAY;
            return;
         } else if (accel != NULL && strcmp(accel, "MPU6515") == 0) {
            list[Accelerometer].maxRange = ACCEL_MPU6500_RANGE;
            list[Accelerometer].resolution = ACCEL_MPU6500_RESOLUTION;
            list[Accelerometer].power = ACCEL_MPU6500_POWER;
            list[Accelerometer].minDelay = ACCEL_MPU6500_MINDELAY;
            return;
        } else if (accel != NULL && strcmp(accel, "MPU6500") == 0) {
            list[Accelerometer].maxRange = ACCEL_MPU6500_RANGE;
            list[Accelerometer].resolution = ACCEL_MPU6500_RESOLUTION;
            list[Accelerometer].power = ACCEL_MPU6500_POWER;
            list[Accelerometer].minDelay = ACCEL_MPU6500_MINDELAY;
            return;
        } else if (accel != NULL && strcmp(accel, "MPU6500") == 0) {
            list[Accelerometer].maxRange = ACCEL_MPU6500_RANGE;
            list[Accelerometer].resolution = ACCEL_MPU6500_RESOLUTION;
            list[Accelerometer].power = ACCEL_MPU6500_POWER;
            list[Accelerometer].minDelay = ACCEL_MPU6500_MINDELAY;
            return;
        } else if (accel != NULL && strcmp(accel, "MPU9150") == 0) {
            list[Accelerometer].maxRange = ACCEL_MPU9150_RANGE;
            list[Accelerometer].resolution = ACCEL_MPU9150_RESOLUTION;
            list[Accelerometer].power = ACCEL_MPU9150_POWER;
            list[Accelerometer].minDelay = ACCEL_MPU9150_MINDELAY;
            return;
        } else if (accel != NULL && strcmp(accel, "MPU9250") == 0) {
            list[Accelerometer].maxRange = ACCEL_MPU9250_RANGE;
            list[Accelerometer].resolution = ACCEL_MPU9250_RESOLUTION;
            list[Accelerometer].power = ACCEL_MPU9250_POWER;
            list[Accelerometer].minDelay = ACCEL_MPU9250_MINDELAY;
            return;
        } else if (accel != NULL && strcmp(accel, "MPU9350") == 0) {
            list[Accelerometer].maxRange = ACCEL_MPU9350_RANGE;
            list[Accelerometer].resolution = ACCEL_MPU9350_RESOLUTION;
            list[Accelerometer].power = ACCEL_MPU9350_POWER;
            list[Accelerometer].minDelay = ACCEL_MPU9350_MINDELAY;
            return;
        }  else if (accel != NULL && strcmp(accel, "MPU3050") == 0) {
            list[Accelerometer].maxRange = ACCEL_BMA250_RANGE;
            list[Accelerometer].resolution = ACCEL_BMA250_RESOLUTION;
            list[Accelerometer].power = ACCEL_BMA250_POWER;
            list[Accelerometer].minDelay = ACCEL_BMA250_MINDELAY;
            return;
        }
    }

    LOGE("HAL:unknown accel id %s -- "
         "params default to bma250 and might be wrong.",
         accel);
    list[Accelerometer].maxRange = ACCEL_BMA250_RANGE;
    list[Accelerometer].resolution = ACCEL_BMA250_RESOLUTION;
    list[Accelerometer].power = ACCEL_BMA250_POWER;
    list[Accelerometer].minDelay = ACCEL_BMA250_MINDELAY;
}

void MPLSensor::fillGyro(const char* gyro, struct sensor_t *list)
{
    VFUNC_LOG;

    if ( gyro != NULL && strcmp(gyro, "MPU3050") == 0) {
        list[Gyro].maxRange = GYRO_MPU3050_RANGE;
        list[Gyro].resolution = GYRO_MPU3050_RESOLUTION;
        list[Gyro].power = GYRO_MPU3050_POWER;
        list[Gyro].minDelay = GYRO_MPU3050_MINDELAY;
    } else if( gyro != NULL && strcmp(gyro, "MPU6050") == 0) {
        list[Gyro].maxRange = GYRO_MPU6050_RANGE;
        list[Gyro].resolution = GYRO_MPU6050_RESOLUTION;
        list[Gyro].power = GYRO_MPU6050_POWER;
        list[Gyro].minDelay = GYRO_MPU6050_MINDELAY;
    } else if( gyro != NULL && strcmp(gyro, "MPU6500") == 0) {
        list[Gyro].maxRange = GYRO_MPU6500_RANGE;
        list[Gyro].resolution = GYRO_MPU6500_RESOLUTION;
        list[Gyro].power = GYRO_MPU6500_POWER;
        list[Gyro].minDelay = GYRO_MPU6500_MINDELAY;
     } else if( gyro != NULL && strcmp(gyro, "MPU6515") == 0) {
        list[Gyro].maxRange = GYRO_MPU6500_RANGE;
        list[Gyro].resolution = GYRO_MPU6500_RESOLUTION;
        list[Gyro].power = GYRO_MPU6500_POWER;
        list[Gyro].minDelay = GYRO_MPU6500_MINDELAY;
    } else if( gyro != NULL && strcmp(gyro, "MPU9150") == 0) {
        list[Gyro].maxRange = GYRO_MPU9150_RANGE;
        list[Gyro].resolution = GYRO_MPU9150_RESOLUTION;
        list[Gyro].power = GYRO_MPU9150_POWER;
        list[Gyro].minDelay = GYRO_MPU9150_MINDELAY;
    } else if( gyro != NULL && strcmp(gyro, "MPU9250") == 0) {
        list[Gyro].maxRange = GYRO_MPU9250_RANGE;
        list[Gyro].resolution = GYRO_MPU9250_RESOLUTION;
        list[Gyro].power = GYRO_MPU9250_POWER;
        list[Gyro].minDelay = GYRO_MPU9250_MINDELAY;
    } else if( gyro != NULL && strcmp(gyro, "MPU9350") == 0) {
        list[Gyro].maxRange = GYRO_MPU9350_RANGE;
        list[Gyro].resolution = GYRO_MPU9350_RESOLUTION;
        list[Gyro].power = GYRO_MPU9350_POWER;
        list[Gyro].minDelay = GYRO_MPU9350_MINDELAY;
    } else {
        LOGE("HAL:unknown gyro id -- gyro params will be wrong.");
        LOGE("HAL:default to use mpu3050 params");
        list[Gyro].maxRange = GYRO_MPU3050_RANGE;
        list[Gyro].resolution = GYRO_MPU3050_RESOLUTION;
        list[Gyro].power = GYRO_MPU3050_POWER;
        list[Gyro].minDelay = GYRO_MPU3050_MINDELAY;
    }

    list[RawGyro].maxRange = list[Gyro].maxRange;
    list[RawGyro].resolution = list[Gyro].resolution;
    list[RawGyro].power = list[Gyro].power;
    list[RawGyro].minDelay = list[Gyro].minDelay;

    return;
}

/* fillRV depends on values of gyro, accel and compass in the list */
void MPLSensor::fillRV(struct sensor_t *list)
{
    VFUNC_LOG;

    /* compute power on the fly */
    list[RotationVector].power = list[Gyro].power +
                                 list[Accelerometer].power +
                                 list[MagneticField].power;
    list[RotationVector].resolution = .00001;
    list[RotationVector].maxRange = 1.0;
    list[RotationVector].minDelay = 5000;

    return;
}

/* fillGMRV depends on values of accel and mag in the list */
void MPLSensor::fillGMRV(struct sensor_t *list)
{
    VFUNC_LOG;

    /* compute power on the fly */
    list[GeomagneticRotationVector].power = list[Accelerometer].power +
                                 list[MagneticField].power;
    list[GeomagneticRotationVector].resolution = .00001;
    list[GeomagneticRotationVector].maxRange = 1.0;
    list[GeomagneticRotationVector].minDelay = 5000;

    return;
}

/* fillGRV depends on values of gyro and accel in the list */
void MPLSensor::fillGRV(struct sensor_t *list)
{
    VFUNC_LOG;

    /* compute power on the fly */
    list[GameRotationVector].power = list[Gyro].power +
                                 list[Accelerometer].power;
    list[GameRotationVector].resolution = .00001;
    list[GameRotationVector].maxRange = 1.0;
    list[GameRotationVector].minDelay = 5000;

    return;
}

void MPLSensor::fillOrientation(struct sensor_t *list)
{
    VFUNC_LOG;

    list[Orientation].power = list[Gyro].power +
                              list[Accelerometer].power +
                              list[MagneticField].power;
    list[Orientation].resolution = .00001;
    list[Orientation].maxRange = 360.0;
    list[Orientation].minDelay = 5000;

    return;
}

void MPLSensor::fillGravity( struct sensor_t *list)
{
    VFUNC_LOG;

    list[Gravity].power = list[Gyro].power +
                          list[Accelerometer].power +
                          list[MagneticField].power;
    list[Gravity].resolution = .00001;
    list[Gravity].maxRange = 9.81;
    list[Gravity].minDelay = 5000;

    return;
}

void MPLSensor::fillLinearAccel(struct sensor_t *list)
{
    VFUNC_LOG;

    list[LinearAccel].power = list[Gyro].power +
                          list[Accelerometer].power +
                          list[MagneticField].power;
    list[LinearAccel].resolution = list[Accelerometer].resolution;
    list[LinearAccel].maxRange = list[Accelerometer].maxRange;
    list[LinearAccel].minDelay = 5000;

    return;
}

void MPLSensor::fillSignificantMotion(struct sensor_t *list)
{
    VFUNC_LOG;

    list[SignificantMotion].power = list[Accelerometer].power;
    list[SignificantMotion].resolution = 1;
    list[SignificantMotion].maxRange = 1;
    list[SignificantMotion].minDelay = -1;
}

#ifdef ENABLE_DMP_SCREEN_AUTO_ROTATION
void MPLSensor::fillScreenOrientation(struct sensor_t *list)
{
    VFUNC_LOG;

    list[NumSensors].power = list[Accelerometer].power;
    list[NumSensors].resolution = 1;
    list[NumSensors].maxRange = 3;
    list[NumSensors].minDelay = 0;
}
#endif

int MPLSensor::inv_init_sysfs_attributes(void)
{
    VFUNC_LOG;

    unsigned char i = 0;
    char sysfs_path[MAX_SYSFS_NAME_LEN];
    char tbuf[2];
    char *sptr;
    char **dptr;
    int num;

    memset(sysfs_path, 0, sizeof(sysfs_path));

    sysfs_names_ptr =
            (char*)malloc(sizeof(char[MAX_SYSFS_ATTRB][MAX_SYSFS_NAME_LEN]));
    sptr = sysfs_names_ptr;
    if (sptr != NULL) {
        dptr = (char**)&mpu;
        do {
            *dptr++ = sptr;
            memset(sptr, 0, sizeof(sptr));
            sptr += sizeof(char[MAX_SYSFS_NAME_LEN]);
        } while (++i < MAX_SYSFS_ATTRB);
    } else {
        LOGE("HAL:couldn't alloc mem for sysfs paths");
        return -1;
    }

    // get proper (in absolute) IIO path & build MPU's sysfs paths
    inv_get_sysfs_path(sysfs_path);

    if (strcmp(sysfs_path, "") == 0)
        return 0;

    memcpy(mSysfsPath, sysfs_path, sizeof(sysfs_path));
    sprintf(mpu.key, "%s%s", sysfs_path, "/key");
    sprintf(mpu.chip_enable, "%s%s", sysfs_path, "/buffer/enable");
    sprintf(mpu.buffer_length, "%s%s", sysfs_path, "/buffer/length");
    sprintf(mpu.master_enable, "%s%s", sysfs_path, "/master_enable");
    sprintf(mpu.power_state, "%s%s", sysfs_path, "/power_state");

    sprintf(mpu.in_timestamp_en, "%s%s", sysfs_path,
            "/scan_elements/in_timestamp_en");
    sprintf(mpu.in_timestamp_index, "%s%s", sysfs_path,
            "/scan_elements/in_timestamp_index");
    sprintf(mpu.in_timestamp_type, "%s%s", sysfs_path,
            "/scan_elements/in_timestamp_type");

    sprintf(mpu.dmp_firmware, "%s%s", sysfs_path, "/dmp_firmware");
    sprintf(mpu.firmware_loaded, "%s%s", sysfs_path, "/firmware_loaded");
    sprintf(mpu.dmp_on, "%s%s", sysfs_path, "/dmp_on");
    sprintf(mpu.dmp_int_on, "%s%s", sysfs_path, "/dmp_int_on");
    sprintf(mpu.dmp_event_int_on, "%s%s", sysfs_path, "/dmp_event_int_on");
    sprintf(mpu.tap_on, "%s%s", sysfs_path, "/tap_on");

    sprintf(mpu.self_test, "%s%s", sysfs_path, "/self_test");

    sprintf(mpu.temperature, "%s%s", sysfs_path, "/temperature");
    sprintf(mpu.gyro_enable, "%s%s", sysfs_path, "/gyro_enable");
    sprintf(mpu.gyro_fifo_rate, "%s%s", sysfs_path, "/sampling_frequency");
    sprintf(mpu.gyro_orient, "%s%s", sysfs_path, "/gyro_matrix");
    sprintf(mpu.gyro_fifo_enable, "%s%s", sysfs_path, "/gyro_fifo_enable");
    sprintf(mpu.gyro_fsr, "%s%s", sysfs_path, "/in_anglvel_scale");
    sprintf(mpu.gyro_fifo_enable, "%s%s", sysfs_path, "/gyro_fifo_enable");
    sprintf(mpu.gyro_rate, "%s%s", sysfs_path, "/gyro_rate");

    sprintf(mpu.accel_enable, "%s%s", sysfs_path, "/accel_enable");
    sprintf(mpu.accel_fifo_rate, "%s%s", sysfs_path, "/sampling_frequency");
    sprintf(mpu.accel_orient, "%s%s", sysfs_path, "/accel_matrix");
    sprintf(mpu.accel_fifo_enable, "%s%s", sysfs_path, "/accel_fifo_enable");
    sprintf(mpu.accel_rate, "%s%s", sysfs_path, "/accel_rate");

#ifndef THIRD_PARTY_ACCEL //MPUxxxx
    sprintf(mpu.accel_fsr, "%s%s", sysfs_path, "/in_accel_scale");

    // DMP uses these values
    sprintf(mpu.in_accel_x_dmp_bias, "%s%s", sysfs_path, "/in_accel_x_dmp_bias");
    sprintf(mpu.in_accel_y_dmp_bias, "%s%s", sysfs_path, "/in_accel_y_dmp_bias");
    sprintf(mpu.in_accel_z_dmp_bias, "%s%s", sysfs_path, "/in_accel_z_dmp_bias");
    
    // MPU uses these values 
    sprintf(mpu.in_accel_x_offset, "%s%s", sysfs_path, "/in_accel_x_offset");
    sprintf(mpu.in_accel_y_offset, "%s%s", sysfs_path, "/in_accel_y_offset");
    sprintf(mpu.in_accel_z_offset, "%s%s", sysfs_path, "/in_accel_z_offset");
    sprintf(mpu.in_accel_self_test_scale, "%s%s", sysfs_path, "/in_accel_self_test_scale");
#endif

    // DMP uses these bias values
    sprintf(mpu.in_gyro_x_dmp_bias, "%s%s", sysfs_path, "/in_anglvel_x_dmp_bias");
    sprintf(mpu.in_gyro_y_dmp_bias, "%s%s", sysfs_path, "/in_anglvel_y_dmp_bias");
    sprintf(mpu.in_gyro_z_dmp_bias, "%s%s", sysfs_path, "/in_anglvel_z_dmp_bias");

    // MPU uses these bias values
    sprintf(mpu.in_gyro_x_offset, "%s%s", sysfs_path, "/in_anglvel_x_offset");
    sprintf(mpu.in_gyro_y_offset, "%s%s", sysfs_path, "/in_anglvel_y_offset");
    sprintf(mpu.in_gyro_z_offset, "%s%s", sysfs_path, "/in_anglvel_z_offset");
    sprintf(mpu.in_gyro_self_test_scale, "%s%s", sysfs_path, "/in_anglvel_self_test_scale");

    sprintf(mpu.three_axis_q_on, "%s%s", sysfs_path, "/three_axes_q_on"); //formerly quaternion_on
    sprintf(mpu.three_axis_q_rate, "%s%s", sysfs_path, "/three_axes_q_rate");
    
    sprintf(mpu.ped_q_on, "%s%s", sysfs_path, "/ped_q_on");
    sprintf(mpu.ped_q_rate, "%s%s", sysfs_path, "/ped_q_rate");
    
    sprintf(mpu.six_axis_q_on, "%s%s", sysfs_path, "/six_axes_q_on");
    sprintf(mpu.six_axis_q_rate, "%s%s", sysfs_path, "/six_axes_q_rate");

    sprintf(mpu.six_axis_q_value, "%s%s", sysfs_path, "/six_axes_q_value");

    sprintf(mpu.step_detector_on, "%s%s", sysfs_path, "/step_detector_on");
    sprintf(mpu.step_indicator_on, "%s%s", sysfs_path, "/step_indicator_on");

    sprintf(mpu.display_orientation_on, "%s%s", sysfs_path,
            "/display_orientation_on");
    sprintf(mpu.event_display_orientation, "%s%s", sysfs_path,
            "/event_display_orientation");

    sprintf(mpu.event_smd, "%s%s", sysfs_path,
            "/event_smd");
    sprintf(mpu.smd_enable, "%s%s", sysfs_path,
            "/smd_enable");
    sprintf(mpu.smd_delay_threshold, "%s%s", sysfs_path,
            "/smd_delay_threshold");
    sprintf(mpu.smd_delay_threshold2, "%s%s", sysfs_path,
            "/smd_delay_threshold2");
    sprintf(mpu.smd_threshold, "%s%s", sysfs_path,
            "/smd_threshold");
    sprintf(mpu.batchmode_timeout, "%s%s", sysfs_path,
            "/batchmode_timeout");
    sprintf(mpu.batchmode_wake_fifo_full_on, "%s%s", sysfs_path,
            "/batchmode_wake_fifo_full_on");
    sprintf(mpu.flush_batch, "%s%s", sysfs_path,
            "/flush_batch");
    sprintf(mpu.pedometer_on, "%s%s", sysfs_path,
            "/pedometer_on");
    sprintf(mpu.pedometer_int_on, "%s%s", sysfs_path,
            "/pedometer_int_on");
    sprintf(mpu.event_pedometer, "%s%s", sysfs_path,
            "/event_pedometer");
    sprintf(mpu.pedometer_steps, "%s%s", sysfs_path,
            "/pedometer_steps");
    sprintf(mpu.motion_lpa_on, "%s%s", sysfs_path,
            "/motion_lpa_on");
    return 0;
}

bool MPLSensor::isMpuNonDmp(void)
{
    VFUNC_LOG;
    if (!strcmp(chip_ID, "mpu3050") || !strcmp(chip_ID, "MPU3050"))
        return true;
    else
        return false;
}

int MPLSensor::isLowPowerQuatEnabled(void)
{
    VFUNC_LOG;
#ifdef ENABLE_LP_QUAT_FEAT
    return !isMpuNonDmp();
#else
    return 0;
#endif
}

int MPLSensor::isDmpDisplayOrientationOn(void)
{
    VFUNC_LOG;
#ifdef ENABLE_DMP_DISPL_ORIENT_FEAT
    if (isMpuNonDmp())
        return 0;
    return 1;
#else
    return 0;
#endif
}

/* these functions can be consolidated
with inv_convert_to_body_with_scale */
void MPLSensor::getCompassBias()
{
    VFUNC_LOG;

    
    long bias[3];
    long compassBias[3];
    unsigned short orient;
    signed char orientMtx[9];
    mCompassSensor->getOrientationMatrix(orientMtx);
    orient = inv_orientation_matrix_to_scalar(orientMtx);
    /* Get Values from MPL */
    inv_get_compass_bias(bias);
    //inv_convert_to_body_with_scale(unsigned short orientation, long sensitivity, const long *input, long *output);
    inv_convert_to_body(orient, bias, compassBias);
    LOGV_IF(HANDLER_DATA, "Mpl Compass Bias (HW unit) %ld %ld %ld", bias[0], bias[1], bias[2]);
    LOGV_IF(HANDLER_DATA, "Mpl Compass Bias (HW unit) (body) %ld %ld %ld", compassBias[0], compassBias[1], compassBias[2]);
    long compassSensitivity = inv_get_compass_sensitivity();
    if (compassSensitivity == 0) {
        compassSensitivity = mCompassScale;
    }
    for(int i=0; i<3; i++) {
        /* convert to uT */
        float temp = (float) compassSensitivity / (1L << 30);
        mCompassBias[i] =(float) (compassBias[i] * temp / 65536.f);
    }

    return; 
}

void MPLSensor::getFactoryGyroBias()
{
    VFUNC_LOG;

    //TODO: mllite needs to add this function
    //if(inv_factory_bias_available) {
        /* Get Values from MPL */
        inv_get_gyro_bias(mFactoryGyroBias);
        LOGV_IF(ENG_VERBOSE, "Factory Gyro Bias %ld %ld %ld", mFactoryGyroBias[0], mFactoryGyroBias[1], mFactoryGyroBias[2]);
        mFactoryGyroBiasAvailable = true;
    //}

    return;
}

/* set bias from factory cal file to MPU offset (in chip frame)
   x = values store in cal file --> (v/1000 * 2^16 / (2000/250))
   offset = x/2^16 * (Gyro scale / self test scale used) * (-1) / offset scale
   i.e. self test default scale = 250 
         gyro scale default to = 2000
         offset scale = 4 //as spec by hardware
         offset = x/2^16 * (8) * (-1) / (4)
*/

void MPLSensor::setFactoryGyroBias()
{
    VFUNC_LOG;
    int scaleRatio = mGyroScale / mGyroSelfTestScale;
    int offsetScale = 4;
    LOGV_IF(ENG_VERBOSE, "HAL: scaleRatio used =%d", scaleRatio);
    LOGV_IF(ENG_VERBOSE, "HAL: offsetScale used =%d", offsetScale);

    /* Write to Driver */
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
            (((int) (((float) mFactoryGyroBias[0]) / 65536.f * scaleRatio)) * -1 / offsetScale),
            mpu.in_gyro_x_offset, getTimestamp());
    if(write_attribute_sensor_continuous(gyro_x_offset_fd,
        (((int) (((float) mFactoryGyroBias[0]) / 65536.f * scaleRatio)) * -1 / offsetScale)) < 0)
    {
        LOGE("HAL:Error writing to gyro_x_offset");
        return;
    }
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
            (((int) (((float) mFactoryGyroBias[1]) / 65536.f * scaleRatio)) * -1 / offsetScale),
            mpu.in_gyro_y_offset, getTimestamp());
    if(write_attribute_sensor_continuous(gyro_y_offset_fd,
        (((int) (((float) mFactoryGyroBias[1]) / 65536.f * scaleRatio)) * -1 / offsetScale)) < 0)
    {
        LOGE("HAL:Error writing to gyro_y_offset");
        return;
    }
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
            (((int) (((float) mFactoryGyroBias[2]) / 65536.f * scaleRatio)) * -1 / offsetScale),
            mpu.in_gyro_z_offset, getTimestamp());
    if(write_attribute_sensor_continuous(gyro_z_offset_fd,
        (((int) (((float) mFactoryGyroBias[2]) / 65536.f * scaleRatio)) * -1 / offsetScale)) < 0)
    {
        LOGE("HAL:Error writing to gyro_z_offset");
        return;
    }
    mFactoryGyroBiasAvailable = false;
    LOGV_IF(EXTRA_VERBOSE, "HAL:Factory Gyro Calibrated Bias Applied");

    return;
}

/* these functions can be consolidated
with inv_convert_to_body_with_scale */
void MPLSensor::getGyroBias()
{
    VFUNC_LOG;

    long *temp = NULL;
    long chipBias[3];
    long bias[3];
    unsigned short orient;

    /* Get Values from MPL */
    inv_get_mpl_gyro_bias(mGyroChipBias, temp);
    orient = inv_orientation_matrix_to_scalar(mGyroOrientation);
    //void inv_convert_to_body_with_scale(unsigned short orientation, long sensitivity, const long *input, long *output);
    inv_convert_to_body(orient, mGyroChipBias, bias);
    LOGV_IF(ENG_VERBOSE, "Mpl Gyro Bias (HW unit) %ld %ld %ld", mGyroChipBias[0], mGyroChipBias[1], mGyroChipBias[2]);
    LOGV_IF(ENG_VERBOSE, "Mpl Gyro Bias (HW unit) (body) %ld %ld %ld", bias[0], bias[1], bias[2]);
    long gyroSensitivity = inv_get_gyro_sensitivity();
    if(gyroSensitivity == 0) {
        gyroSensitivity = mGyroScale;
    }

    /* scale and convert to rad */
    for(int i=0; i<3; i++) {
        float temp = (float) gyroSensitivity / (1L << 30);
        mGyroBias[i] = (float) (bias[i] * temp / (1<<16) / 180 * M_PI);
        if (mGyroBias[i] != 0)
            mGyroBiasAvailable = true;
    }

    return;
}

void MPLSensor::setGyroBias()
{
    VFUNC_LOG;

    if(mGyroBiasAvailable == false)
        return;

    long bias[3];
    long gyroSensitivity = inv_get_gyro_sensitivity();

    if(gyroSensitivity == 0) {
        gyroSensitivity = mGyroScale;
    }
    
    inv_get_gyro_bias_dmp_units(bias);

    /* Write to Driver */
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %ld > %s (%lld)",
            bias[0], mpu.in_gyro_x_dmp_bias, getTimestamp());
    if(write_attribute_sensor_continuous(gyro_x_dmp_bias_fd, bias[0]) < 0) {
        LOGE("HAL:Error writing to gyro_x_dmp_bias");
        return;
    }
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %ld > %s (%lld)",
            bias[1], mpu.in_gyro_y_dmp_bias, getTimestamp());
    if(write_attribute_sensor_continuous(gyro_y_dmp_bias_fd, bias[1]) < 0) {
        LOGE("HAL:Error writing to gyro_y_dmp_bias");
        return;
    }
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %ld > %s (%lld)",
            bias[2], mpu.in_gyro_z_dmp_bias, getTimestamp());
    if(write_attribute_sensor_continuous(gyro_z_dmp_bias_fd, bias[2]) < 0) {
        LOGE("HAL:Error writing to gyro_z_dmp_bias");
        return;
    }
    mGyroBiasApplied = true;
    mGyroBiasAvailable = false;
    LOGV_IF(EXTRA_VERBOSE, "HAL:Gyro DMP Calibrated Bias Applied");

    return;
}

void MPLSensor::getFactoryAccelBias()
{
    VFUNC_LOG;

    long temp;

    /* Get Values from MPL */
    inv_get_accel_bias(mFactoryAccelBias);
    LOGV_IF(ENG_VERBOSE, "Factory Accel Bias (mg) %ld %ld %ld", mFactoryAccelBias[0], mFactoryAccelBias[1], mFactoryAccelBias[2]);
    mFactoryAccelBiasAvailable = true;

    return;
}

void MPLSensor::setFactoryAccelBias()
{
    VFUNC_LOG;

    if(mFactoryAccelBiasAvailable == false)
        return;

    /* add scaling here - depends on self test parameters */
    int scaleRatio = mAccelScale / mAccelSelfTestScale;
    int offsetScale = 16;
    long tempBias;

    LOGV_IF(ENG_VERBOSE, "HAL: scaleRatio used =%d", scaleRatio);
    LOGV_IF(ENG_VERBOSE, "HAL: offsetScale used =%d", offsetScale);

    /* Write to Driver */
    tempBias = -mFactoryAccelBias[0] / 65536.f * scaleRatio / offsetScale;
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %ld > %s (%lld)",
            tempBias, mpu.in_accel_x_offset, getTimestamp());
    if(write_attribute_sensor_continuous(accel_x_offset_fd, tempBias) < 0) {
        LOGE("HAL:Error writing to accel_x_offset");
        return;
    }
    tempBias = -mFactoryAccelBias[1] / 65536.f * scaleRatio / offsetScale;
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %ld > %s (%lld)",
            tempBias, mpu.in_accel_y_offset, getTimestamp());
    if(write_attribute_sensor_continuous(accel_y_offset_fd, tempBias) < 0) {
        LOGE("HAL:Error writing to accel_y_offset");
        return;
    }
    tempBias = -mFactoryAccelBias[2] / 65536.f * scaleRatio / offsetScale;
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %ld > %s (%lld)",
            tempBias, mpu.in_accel_z_offset, getTimestamp());
    if(write_attribute_sensor_continuous(accel_z_offset_fd, tempBias) < 0) {
        LOGE("HAL:Error writing to accel_z_offset");
        return;
    }
    mFactoryAccelBiasAvailable = false;
    LOGV_IF(EXTRA_VERBOSE, "HAL:Factory Accel Calibrated Bias Applied");

    return;
}

void MPLSensor::getAccelBias()
{
    VFUNC_LOG;
    long temp;

    /* Get Values from MPL */
    inv_get_mpl_accel_bias(mAccelBias, &temp);
    LOGV_IF(ENG_VERBOSE, "Accel Bias (mg) %ld %ld %ld", 
            mAccelBias[0], mAccelBias[1], mAccelBias[2]);
    mAccelBiasAvailable = true;

    return;
}

/*    set accel bias obtained from MPL
      bias is scaled by 65536 from MPL
      DMP expects: bias * 536870912 / 2^30 = bias / 2 (in body frame)
*/
void MPLSensor::setAccelBias()
{
    VFUNC_LOG;

    if(mAccelBiasAvailable == false) {
        LOGV_IF(ENG_VERBOSE, "HAL: setAccelBias - accel bias not available");
        return;
    }

    /*
    long bias[3];
    unsigned short orient = inv_orientation_matrix_to_scalar(mAccelOrientation);
    inv_convert_to_body(orient, mAccelBias, bias);
    */

    /* write to driver */
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %ld > %s (%lld)",
            (long) (mAccelBias[0] / 65536.f / 2), 
            mpu.in_accel_x_dmp_bias, getTimestamp());
    if(write_attribute_sensor_continuous(
            accel_x_dmp_bias_fd, (long)(mAccelBias[0] / 65536.f / 2)) < 0) {
        LOGE("HAL:Error writing to accel_x_dmp_bias");
        return;
    }
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %ld > %s (%lld)",
            (long)(mAccelBias[1] / 65536.f / 2), 
            mpu.in_accel_y_dmp_bias, getTimestamp());
    if(write_attribute_sensor_continuous(
            accel_y_dmp_bias_fd, (long)(mAccelBias[1] / 65536.f / 2)) < 0) {
        LOGE("HAL:Error writing to accel_y_dmp_bias");
        return;
    }
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %ld > %s (%lld)",
            (long)(mAccelBias[2] / 65536 / 2), 
            mpu.in_accel_z_dmp_bias, getTimestamp());
    if(write_attribute_sensor_continuous(
            accel_z_dmp_bias_fd, (long)(mAccelBias[2] / 65536 / 2)) < 0) {
        LOGE("HAL:Error writing to accel_z_dmp_bias");
        return;
    }

    mAccelBiasAvailable = false;
    mAccelBiasApplied = true;
    LOGV_IF(EXTRA_VERBOSE, "HAL:Accel DMP Calibrated Bias Applied");

    return;
}

int MPLSensor::isCompassDisabled(void)
{
    VFUNC_LOG;
    if(mCompassSensor->getFd() < 0 && !mCompassSensor->isIntegrated()) {
    LOGI_IF(EXTRA_VERBOSE, "HAL: Compass is disabled, Six-axis Sensor Fusion is used.");
        return 1;
    }
    return 0;
}

int MPLSensor::checkBatchEnabled(void)
{
    VFUNC_LOG;
    return ((mFeatureActiveMask & INV_DMP_BATCH_MODE)? 1:0);
}

/* precondition: framework disallows this case, ie enable continuous sensor, */
/* and enable batch sensor */
/* if one sensor is in continuous mode, HAL disallows enabling batch for this sensor */
/* or any other sensors */
#define DEBUG_BATCHING  (0)
int MPLSensor::batch(int handle, int flags, int64_t period_ns, int64_t timeout)
{
    VFUNC_LOG;

    int res = 0;

    if (isMpuNonDmp())
        return res;

    /* Enables batch mode and sets timeout for the given sensor */
    /* enum SENSORS_BATCH_DRY_RUN, SENSORS_BATCH_WAKE_UPON_FIFO_FULL */
    bool dryRun = false;
    android::String8 sname;
    int what = -1;
    int enabled_sensors = mEnabled;
    int batchMode = timeout > 0 ? 1 : 0;

    LOGI_IF(DEBUG_BATCHING || ENG_VERBOSE,
            "HAL:batch called - handle=%d, flags=%d, period=%lld, timeout=%lld",
            handle, flags, period_ns, timeout);

    if(flags & (1 << SENSORS_BATCH_DRY_RUN)) {
        dryRun = true;
        LOGI_IF(PROCESS_VERBOSE,
                "HAL:batch - dry run mode is set (%d)", SENSORS_BATCH_DRY_RUN);
    }

    getHandle(handle, what, sname);
    if(uint32_t(what) >= NumSensors) {
        LOGE("HAL:batch sensors %d not found", what);
        return -EINVAL;
    }

    switch (what) {
    case Gyro:
    case RawGyro:
    case Accelerometer:
    case MagneticField:
    case RawMagneticField:
    case Pressure:
    case GameRotationVector:
    case StepDetector:
        LOGV_IF(PROCESS_VERBOSE, "HAL: batch - select sensor (handle %d)", handle);
        break;
    default:
        if (timeout > 0) {
            LOGV_IF(PROCESS_VERBOSE, "sensor (handle %d) is not supported in batch mode", handle);
            return -EINVAL;
        }
    }

    int tempBatch = 0;
    if (timeout > 0) { 
        tempBatch = mBatchEnabled | (1 << what);
    } else {
        tempBatch = mBatchEnabled & ~(1 << what);
    }

    if (!computeBatchSensorMask(mEnabled, tempBatch)) {
        batchMode = 0;
    } else {
        batchMode = 1;
    }
    
    /* get maximum possible bytes to batch per sample */
    /* get minimum delay for each requested sensor    */
    ssize_t nBytes = 0;
    int64_t wanted = 1000000000LL, ns = 0;
    int64_t timeoutInMs = 0;
    for (int i = 0; i < NumSensors; i++) {
        if (batchMode == 1) {
            ns = mBatchDelays[i];
            LOGV_IF(DEBUG_BATCHING || EXTRA_VERBOSE,
                    "HAL:batch - requested sensor=0x%01x, batch delay=%lld", mEnabled & (1 << i), ns);
            // take the min delay ==> max rate
            wanted = (ns < wanted) ? ns : wanted;
            if (i <= RawMagneticField) {
                nBytes += 8;
            }
            if (i == Pressure) {
                nBytes += 6;
            }
            if ((i == StepDetector) || (i == GameRotationVector)) {
                nBytes += 16;
            }
        }
    }
    
    /* check if we can support issuing interrupt before FIFO fills-up */
    /* in the given timeout.                                          */
    if (flags & (1 << SENSORS_BATCH_WAKE_UPON_FIFO_FULL)) {
            LOGE("HAL: batch SENSORS_BATCH_WAKE_UPON_FIFO_FULL is not supported");
            return -EINVAL;
        /* provide messge if it exceeds hardware capability
        if (nSamples * nBytes >= 1024) {
            LOGE("HAL:batch - timeout - configuration is not supported, "
                 "cannot provide requested amount of buffering (%lld ms)",
                 timeout / 1000000LL);
        }*/
    }

    if(dryRun == true)
        return 0;

    /* starting from code below,  we will modify hardware */
    /* first edit global batch mode mask */

    if (!timeout) {
        mBatchEnabled &= ~(1 << what);
        mBatchDelays[what] = 1000000000L;
        mBatchTimeouts[what] = 100000000000LL;
    } else {
        mBatchEnabled |= (1 << what);
        mBatchDelays[what] = period_ns;
        mBatchTimeouts[what] = timeout;
    }

    /* set mask used by other functions */
    mOldBatchEnabledMask = batchMode;

    /* For these sensors, switch to different data output */
    /* These steps should be optimized at some point */
    int featureMask = computeBatchDataOutput();

    LOGV_IF(ENG_VERBOSE, "batchMode =%d, featureMask=0x%x, mEnabled=%d",
                batchMode, featureMask, mEnabled);
    if (DEBUG_BATCHING || EXTRA_VERBOSE) {
        LOGV("HAL:batch - sensor=0x%01x", mBatchEnabled);
        for (int d = 0; d < NumSensors; d++) {            
            LOGV("HAL:batch - sensor status=0x%01x batch status=0x%01x timeout=%lld delay=%lld",
                            mEnabled & (1 << d), (mBatchEnabled & (1 << d)), mBatchTimeouts[d],
                            mBatchDelays[d]);
        }
    }

    /* take the minimum batchmode timeout */
    if (batchMode == 1) {
        int64_t tempTimeout = 100000000000LL;
        for (int i = 0; i < NumSensors; i++) {
            if ((mEnabled & (1 << i) && mBatchEnabled & (1 << i)) ||
                    (((featureMask & INV_DMP_PED_STANDALONE) && (mBatchEnabled & (1 << StepDetector))))) {
                LOGV_IF(ENG_VERBOSE, "i=%d, timeout=%lld", i, mBatchTimeouts[i]);
                ns = mBatchTimeouts[i];
                tempTimeout = (ns < tempTimeout) ? ns : tempTimeout;
            }
        }
        timeout = tempTimeout;
        /* Convert ns to millisecond */
        timeoutInMs = timeout / 1000000;

        /* remember last timeout value */
        mBatchTimeoutInMs = timeoutInMs;
        
        /* TODO: Calculate nSamples */
        int nSamples = 0;
        nSamples = (unsigned long)(
            (1000000000.f / wanted) * ((float)timeout / 1000000000.f));
    } else {
        timeoutInMs = 0;    
    }
    
    LOGV_IF(DEBUG_BATCHING || EXTRA_VERBOSE,
                "HAL:batch - timeout - timeout=%lld ns, timeoutInMs=%lld, delay=%lld ns",
                timeout, timeoutInMs, wanted);

    // reset master enable
    res = masterEnable(0);
    if (res < 0) {
        return res;
    }

    /* case for Ped standalone */
    if ((batchMode == 1) && (featureMask & INV_DMP_PED_STANDALONE) &&
           (mFeatureActiveMask & INV_DMP_PEDOMETER)) {
        LOGI("ID_P only = 0x%x", mBatchEnabled);
        enablePedQuaternion(0);
        enablePedStandalone(1);
    } else {
        enablePedStandalone(0);
        if (featureMask & INV_DMP_PED_QUATERNION) {
            enableLPQuaternion(0);
            enablePedQuaternion(1);
        }
    }

    /* case for Ped Quaternion */
    if ((batchMode == 1) && (featureMask & INV_DMP_PED_QUATERNION) &&
            (mEnabled & (1 << GameRotationVector)) &&
            (mFeatureActiveMask & INV_DMP_PEDOMETER)) {
        LOGI("ID_P and GRV or ALL = 0x%x", mBatchEnabled);        
        LOGI("ID_P is enabled for batching, PED quat will be automatically enabled");
        enableLPQuaternion(0);
        enablePedQuaternion(1);
        
        wanted = mBatchDelays[GameRotationVector];
        /* set pedq rate */               
        LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                int(1000000000.f / wanted), mpu.ped_q_rate,
                getTimestamp());
        write_sysfs_int(mpu.ped_q_rate, 1000000000.f / wanted);
        LOGV_IF(PROCESS_VERBOSE,
                "HAL:DMP ped quaternion rate %.2f Hz", 1000000000.f / wanted);
    } else if (!(featureMask & INV_DMP_PED_STANDALONE)){
        LOGV_IF(ENG_VERBOSE, "Toggle back to normal 6 axis");
        if (mEnabled & (1 << GameRotationVector) && checkLPQRateSupported()) {
            enableLPQuaternion(1);
        }        
        enablePedQuaternion(0);
    }

     /* case for Ped indicator */
    if ((batchMode == 1) && ((featureMask & INV_DMP_PED_INDICATOR))) {
        enablePedIndicator(1);
    } else {
        enablePedIndicator(0);
    }    

    /* case for Six Axis Quaternion */
    if ((batchMode == 1) && (featureMask & INV_DMP_6AXIS_QUATERNION) &&
            (mEnabled & (1 << GameRotationVector))) {
        LOGI("GRV = 0x%x", mBatchEnabled);
        enableLPQuaternion(0);
        enable6AxisQuaternion(1);
        if (what == GameRotationVector) {
            setInitial6QuatValue();
        }

        wanted = mBatchDelays[GameRotationVector];
        /* set sixaxis rate */
        LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                int(1000000000.f / wanted), mpu.six_axis_q_rate,
                getTimestamp());
        write_sysfs_int(mpu.six_axis_q_rate, 1000000000.f / wanted);
        LOGV_IF(PROCESS_VERBOSE,
                "HAL:DMP six axis rate %.2f Hz", 1000000000.f / wanted);        
    } else if (!(featureMask & INV_DMP_PED_QUATERNION)){
        LOGV_IF(ENG_VERBOSE, "Toggle back to normal 6 axis");
        if (mEnabled & (1 << GameRotationVector) && checkLPQRateSupported()) {
            enableLPQuaternion(1);
        }
        enable6AxisQuaternion(0);
    } else {
        enable6AxisQuaternion(0);
    }

    /* TODO: This may make a come back some day */
    /* write not to overflow hardware FIFO if flag is set */
    /*if (flags & (1 << SENSORS_BATCH_WAKE_UPON_FIFO_FULL)) {
        LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                0, mpu.batchmode_wake_fifo_full_on, getTimestamp());
        if (write_sysfs_int(mpu.batchmode_wake_fifo_full_on, 0) < 0) {
            LOGE("HAL:ERR can't write batchmode_wake_fifo_full_on");
        }
    }*/

    /* write required timeout to sysfs */
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %lld > %s (%lld)",
            timeoutInMs, mpu.batchmode_timeout, getTimestamp());
    if (write_sysfs_int(mpu.batchmode_timeout, timeoutInMs) < 0) {
        LOGE("HAL:ERR can't write batchmode_timeout");
    }

    if (batchMode == 1) {
        // enable DMP
        res = onDmp(1);
        if (res < 0) {
            return res;
        }
        // set batch rates
        if (setBatchDataRates() < 0) {
            LOGE("HAL:ERR can't set batch data rates");
        }                  
        // default fifo rate to 200Hz
        LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                200, mpu.gyro_fifo_rate, getTimestamp());
        if (write_sysfs_int(mpu.gyro_fifo_rate, 200) < 0) {
            res = -1;
            LOGE("HAL:ERR can't set DMP rate to 200Hz");
            return res;
        }
    } else {
        if ((mFeatureActiveMask == 0) && !(mEnabled & VIRTUAL_SENSOR_ALL_MASK)) {
            // disable DMP
            res = onDmp(0);
            if (res < 0) {
                return res;
            }
            /*if (resetDataRates() < 0) {
                LOGE("HAL:ERR can't reset output rate back to original setting");
            }*/
        }
        /* reset sensor rate */
        if (resetDataRates() < 0) {
            LOGE("HAL:ERR can't reset output rate back to original setting");
        }
    }
    if (enabled_sensors || mFeatureActiveMask) {
        masterEnable(1);
    }
    return res;
}

int MPLSensor::flush(int handle)
{
    VFUNC_LOG;
    
    int res = 0;
    android::String8 sname;
    int what = -1;
   
    /* TODO: return zero if no data in FIFO */ 
    getHandle(handle, what, sname);
    if (uint32_t(what) >= NumSensors) {
        LOGE("HAL:flush - what=%d is invalid", what);
        return -EINVAL;
    }
       
    if (((what != StepDetector) && (!(mEnabled & (1 << what)))) ||
        ((what == StepDetector) && !(mFeatureActiveMask & INV_DMP_PEDOMETER))) {
         LOGE("HAL: flush - sensor %s not enabled", sname.string());
         return -EINVAL;
    }
    if(!(mBatchEnabled & (1 << what))) {
         LOGE("HAL:flush - batch mode not enabled for sensor %s", sname.string());
         return 0;
    }
    LOGV_IF(PROCESS_VERBOSE, "HAL:flush - sensor %s (handle %d)", sname.string(), handle);

    /*write sysfs */
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:cat %s (%lld)",
            mpu.flush_batch, getTimestamp());

    read_sysfs_int(mpu.flush_batch, &res);
    if (res < 0) {
        LOGE("HAL:ERR can't read flush_batch");
        return -1;
    } 
    
    if (res == 0) {
        LOGI("HAL: flush - no data in FIFO");
    }

    mFlushEnabled = handle;
    LOGV_IF(ENG_VERBOSE, "HAl:flush - mFlushEnabled=%d", mFlushEnabled);

    mFlushBatchSet = 0;
    return res;
}

int MPLSensor::computeBatchDataOutput()
{
    VFUNC_LOG;

    int featureMask = 0;
    if (mBatchEnabled == 0)
        return 0;//h
    
    uint32_t hardwareSensorMask = (1 << Gyro)
                                | (1 << RawGyro)
                                | (1 << Accelerometer)
                                | (1 << MagneticField)
                                | (1 << RawMagneticField)
                                | (1 << Pressure);
    LOGV_IF(ENG_VERBOSE, "hardwareSensorMask = 0x%0x, mBatchEnabled = 0x%0x", 
            hardwareSensorMask, mBatchEnabled);
   
    if (mBatchEnabled & (1 << StepDetector)) {         
        if (mBatchEnabled & (1 << GameRotationVector)) {
            if ((mBatchEnabled & hardwareSensorMask)) {
                featureMask |= INV_DMP_6AXIS_QUATERNION;//a
                featureMask |= INV_DMP_PED_INDICATOR;
            } else {
                featureMask |= INV_DMP_PED_QUATERNION; //b
                featureMask |= INV_DMP_PED_INDICATOR;  //always piggy back a bit
            }
        } else {
            if (mBatchEnabled & hardwareSensorMask) {
                featureMask |= INV_DMP_PED_INDICATOR; //c
            } else {
                featureMask |= INV_DMP_PED_STANDALONE; //d
                featureMask |= INV_DMP_PED_INDICATOR; //required for standalone
            }
        }
    } else if (mBatchEnabled & (1 << GameRotationVector)) {
        featureMask |= INV_DMP_6AXIS_QUATERNION; //e,f
    } else {
        LOGV_IF(ENG_VERBOSE, 
                "HAL:computeBatchDataOutput: featuerMask=0x%x", featureMask);
        return 0; //g
    }

    LOGV_IF(ENG_VERBOSE, 
            "HAL:computeBatchDataOutput: featuerMask=0x%x", featureMask);
    return featureMask;
}

int MPLSensor::getDmpPedometerFd()
{
    VFUNC_LOG;
    LOGV_IF(EXTRA_VERBOSE, "getDmpPedometerFd returning %d", dmp_pedometer_fd);
    return dmp_pedometer_fd;
}

/* @param [in] : outputType = 1 --derive from ped_q */
/* outputType = 0 --derive from IRQ          */
int MPLSensor::readDmpPedometerEvents(sensors_event_t* data, int count,
                                      int32_t id, int32_t type, int outputType)
{
    VFUNC_LOG;

    int res = 0;
    char dummy[4];
    FILE *fp;
    uint64_t stepCount = 0;
    int numEventReceived = 0;
    int update = 0;

    if((mDmpStepCountEnabled || mDmpPedometerEnabled) && count > 0) {
        /* handles return event */
        sensors_event_t temp;

        LOGI_IF(EXTRA_VERBOSE, "HAL: Read Pedometer Event ID=%d", id);
        temp.version = sizeof(sensors_event_t);
        temp.sensor = id;
        temp.type = type;
        temp.acceleration.status
            = SENSOR_STATUS_UNRELIABLE;
        /* sensors.h specified to return 1.0 */
        if(id == ID_P) {
             temp.data[0] = 1;
             temp.data[1] = 0.f;
             temp.data[2] = 0.f;
        } else {
            fp = fopen(mpu.pedometer_steps, "r");
            if (fp == NULL) {
                LOGE("HAL:cannot open pedometer_steps");
            } else{
                if (fscanf(fp, "%lld\n", &stepCount) < 0 || fclose(fp) < 0) {
                    LOGE("HAL:cannot read pedometer_steps");
                    return 0;
                }
            }
            /* return onChange only*/
            if (stepCount == mLastStepCount) {
                return 0;
            }
            /* TODO: framework needs to support 64-bit */
#ifdef TESTING
            temp.data[0] = (float)stepCount;
#else
            temp.u64.step_counter = stepCount;
#endif
            mLastStepCount = stepCount;
        }

        if (!outputType) {
            struct timespec ts;
            clock_gettime(CLOCK_MONOTONIC, &ts) ;
            temp.timestamp = (int64_t)ts.tv_sec * 1000000000 + ts.tv_nsec;
        }

        *data++ = temp;
        count--;
        numEventReceived++;
    }

    if (!outputType) {
        // read dummy data per driver's request
        // only required if actual irq is issued
        read(dmp_pedometer_fd, dummy, 4);
    } else {
        return 1;
    } 

    return numEventReceived;
}

int MPLSensor::getDmpSignificantMotionFd()
{
    VFUNC_LOG;

    LOGV_IF(EXTRA_VERBOSE, "getDmpSignificantMotionFd returning %d",
            dmp_sign_motion_fd);
    return dmp_sign_motion_fd;
}

int MPLSensor::readDmpSignificantMotionEvents(sensors_event_t* data, int count)
{
    VFUNC_LOG;

    int res = 0;
    char dummy[4];
    int significantMotion;
    FILE *fp;
    int sensors = mEnabled;
    int numEventReceived = 0;
    int update = 0;

    /* Technically this step is not necessary for now  */
    /* In the future, we may have meaningful values */
    fp = fopen(mpu.event_smd, "r");
    if (fp == NULL) {
        LOGE("HAL:cannot open event_smd");
        return 0;
    } else {
        if (fscanf(fp, "%d\n", &significantMotion) < 0 || fclose(fp) < 0) {
            LOGE("HAL:cannot read event_smd");
        }
    }

    if(mDmpSignificantMotionEnabled && count > 0) {
       /* By implementation, smd is disabled once an event is triggered */
        sensors_event_t temp;

        /* Handles return event */
        LOGI("HAL: SMD detected");
        int update = smHandler(&temp);
        if (update && count > 0) {
            *data++ = temp;
            count--;
            numEventReceived++;
            mDmpSignificantMotionEnabled = 0;
            mFeatureActiveMask &= ~INV_DMP_SIGNIFICANT_MOTION;
#if 0
            if(mFeatureActiveMask == 0 || sensors == 0) {
                LOGI("dmp off");
                // disable DMP
                masterEnable(0);
                res = onDmp(0);
                if (res < 0)
                    return res;

                // disable accel engine
                if (!(mLocalSensorMask & mMasterSensorMask
                        & INV_THREE_AXIS_ACCEL)) {
                    res = enableAccel(0);
                    if (res < 0)
                        return res;
                }
            }
#endif
        }
    }

    // read dummy data per driver's request
    read(dmp_sign_motion_fd, dummy, 4);

    return numEventReceived;
}

int MPLSensor::enableDmpSignificantMotion(int en)
{
    VFUNC_LOG;

    int res = 0;
    int enabled_sensors = mEnabled;

    if (isMpuNonDmp())
        return res;

    // reset master enable
    res = masterEnable(0);
    if (res < 0)
        return res;

    //Toggle significant montion detection
    if(en) {
        LOGV_IF(ENG_VERBOSE, "HAL:Enabling Significant Motion");
        LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                1, mpu.smd_enable, getTimestamp());
        if (write_sysfs_int(mpu.smd_enable, 1) < 0) {
            LOGE("HAL:ERR can't write DMP smd_enable");
            res = -1;   //Indicate an err
        }

        // enable DMP
        res = onDmp(1);
        if (res < 0)
            return res;

        // set DMP rate to 200Hz
        LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                200, mpu.accel_fifo_rate, getTimestamp());
        if (write_sysfs_int(mpu.accel_fifo_rate, 200) < 0) {
            res = -1;
            LOGE("HAL:ERR can't set rate to 200Hz");
            return res;
        }

        // enable accel engine
        res = enableAccel(1);
        if (res < 0)
            return res;

        // disable accel FIFO
        if (!(mLocalSensorMask & mMasterSensorMask & INV_THREE_AXIS_ACCEL)) {
            res = turnOffAccelFifo();
            if (res < 0)
                return res;
        }
        mFeatureActiveMask |= INV_DMP_SIGNIFICANT_MOTION;
    }
    else {
        LOGV_IF(ENG_VERBOSE, "HAL:Disabling Significant Motion");
        LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                0, mpu.smd_enable, getTimestamp());
        if (write_sysfs_int(mpu.smd_enable, 0) < 0) {
            LOGE("HAL:ERR write DMP smd_enable");
        }
        mFeatureActiveMask &= ~INV_DMP_SIGNIFICANT_MOTION;
        // disable DMP
        if (mFeatureActiveMask == 0) {
            res = onDmp(0);

            if (res < 0)
                return res;

            // disable accel engine
            if (!(mLocalSensorMask & mMasterSensorMask
                    & INV_THREE_AXIS_ACCEL)) {
                res = enableAccel(0);
                if (res < 0)
                    return res;
            }
        }
        if(enabled_sensors) {
            LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                en, mpu.dmp_event_int_on, getTimestamp());
            if (write_sysfs_int(mpu.dmp_event_int_on, en) < 0) {
                res = -1;
                LOGE("HAL:ERR can't enable DMP event interrupt");
            }
         }
    }
    if(en || enabled_sensors || mFeatureActiveMask) { 
        res = masterEnable(1);
    }
    return res;
}

void MPLSensor::setInitial6QuatValue()
{
    VFUNC_LOG;

    if (!mInitial6QuatValueAvailable)
        return;

    /* convert to unsigned char array */
    size_t length = 16;
    unsigned char quat[16];
    convert_long_to_hex_char(mInitial6QuatValue, quat, 4);

    /* write to sysfs */
    LOGV_IF(EXTRA_VERBOSE, "HAL:six axis q value: %s", mpu.six_axis_q_value);
    FILE* fptr = fopen(mpu.six_axis_q_value, "w");
    if(fptr == NULL) {
        LOGE("HAL:could not open six_axis_q_value");
    } else {
        if (fwrite(quat, 1, length, fptr) != length || fclose(fptr) < 0) {
           LOGE("HAL:write six axis q value failed");
        } else {
            LOGI("!!!! write dmp 6 axis !!!");
            mInitial6QuatValueAvailable = 0;
        }
    }

    return;
}
int MPLSensor::writeSignificantMotionParams(bool toggleEnable,
                                            uint32_t delayThreshold1,
                                            uint32_t delayThreshold2,
                                            uint32_t motionThreshold)
{
    VFUNC_LOG;

    int res = 0;

    // Turn off enable
    if (toggleEnable) {
        masterEnable(0);
    }

    // Write supplied values
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
            delayThreshold1, mpu.smd_delay_threshold, getTimestamp());
    res = write_sysfs_int(mpu.smd_delay_threshold, delayThreshold1);
    if (res == 0) {
        LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                delayThreshold2, mpu.smd_delay_threshold2, getTimestamp());
        res = write_sysfs_int(mpu.smd_delay_threshold2, delayThreshold2);
    }
    if (res == 0) {
        LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                motionThreshold, mpu.smd_threshold, getTimestamp());
        res = write_sysfs_int(mpu.smd_threshold, motionThreshold);
    }

    // Turn on enable
    if (toggleEnable) {
        masterEnable(1);
    }
    return res;
}

/* set batch data rate */
/* this function should be optimized */
int MPLSensor::setBatchDataRates()
{
    VFUNC_LOG;

    int res = 0;
    int tempFd = -1;

    int64_t gyroRate;
    int64_t accelRate;
    int64_t compassRate;
    int64_t pressureRate;
    int64_t quatRate;
    
    int mplGyroRate;
    int mplAccelRate;
    int mplCompassRate;
    int mplQuatRate;
            
#ifdef ENABLE_MULTI_RATE
    /* take care of case where only one type of gyro sensors or compass sensors is turned on */
    gyroRate = mBatchDelays[Gyro] < mBatchDelays[RawGyro] ? mBatchDelays[Gyro] : mBatchDelays[RawGyro];
    accelRate = mBatchDelays[Accelerometer];
    compassRate = mBatchDelays[MagneticField] < mBatchDelays[RawMagneticField] ? mBatchDelays[MagneticField] : mBatchDelays[RawMagneticField];
    pressureRate = mBatchDelays[Pressure];

    if ((mFeatureActiveMask & INV_DMP_PED_QUATERNION) || 
            (mFeatureActiveMask & INV_DMP_6AXIS_QUATERNION)) {
        quatRate = mBatchDelays[GameRotationVector];
        mplQuatRate = (int) quatRate / 1000LL;
        inv_set_quat_sample_rate(mplQuatRate);
        LOGV_IF(PROCESS_VERBOSE,
            "HAL:MPL quat sample rate: (mpl)=%d us (mpu)=%.2f Hz", mplQuatRate,
                1000000000.f / quatRate );
        getDmpRate(&quatRate);
    }

    mplGyroRate = (int) gyroRate / 1000LL;
    mplAccelRate = (int) accelRate / 1000LL;
    mplCompassRate = (int) compassRate / 1000LL;
    
     /* set rate in MPL */
     /* compass can only do 100Hz max */
    inv_set_gyro_sample_rate(mplGyroRate);
    inv_set_accel_sample_rate(mplAccelRate);
    inv_set_compass_sample_rate(mplCompassRate);
           
    LOGV_IF(PROCESS_VERBOSE,
            "HAL:MPL gyro sample rate: (mpl)=%d us (mpu)=%.2f Hz", mplGyroRate, 1000000000.f / gyroRate);
    LOGV_IF(PROCESS_VERBOSE,
            "HAL:MPL accel sample rate: (mpl)=%d us (mpu)=%.2f Hz", mplAccelRate, 1000000000.f / accelRate);
    LOGV_IF(PROCESS_VERBOSE,
            "HAL:MPL compass sample rate: (mpl)=%d us (mpu)=%.2f Hz", mplCompassRate, 1000000000.f / compassRate);

#else
    /* search the minimum delay requested across all enabled sensors */
    int64_t wanted = 1000000000LL;
    for (int i = 0; i < NumSensors; i++) {
        if (mBatchEnabled & (1 << i)) {
            int64_t ns = mBatchDelays[i];
            wanted = wanted < ns ? wanted : ns;
        }
    }
    gyroRate = wanted;
    accelRate = wanted;
    compassRate = wanted;
    pressureRate = wanted;
#endif

    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %.0f > %s (%lld)",
            1000000000.f / gyroRate, mpu.gyro_rate,
            getTimestamp());
    tempFd = open(mpu.gyro_rate, O_RDWR);
    res = write_attribute_sensor(tempFd, 1000000000.f / gyroRate);
    if(res < 0) {
        LOGE("HAL:GYRO update delay error");
    }
    
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %.0f > %s (%lld)",
            1000000000.f / accelRate, mpu.accel_rate,
            getTimestamp());
    tempFd = open(mpu.accel_rate, O_RDWR);
    res = write_attribute_sensor(tempFd, 1000000000.f / accelRate);
    LOGE_IF(res < 0, "HAL:ACCEL update delay error");
   
    if (compassRate < mCompassSensor->getMinDelay() * 1000LL) {
        compassRate = mCompassSensor->getMinDelay() * 1000LL;
    }
    mCompassSensor->setDelay(ID_M, compassRate);

    pressureRate = mDelays[Pressure];

    mPressureSensor->setDelay(ID_PS, pressureRate);
   
    return res;
}

/* Set sensor rate */
/* this function should be optimized */
int MPLSensor::resetDataRates()
{
    VFUNC_LOG;
    
    int res = 0;
    int tempFd = -1;
    int64_t wanted = 1000000000LL;

    int64_t gyroRate;
    int64_t accelRate;
    int64_t compassRate;
    int64_t pressureRate;
    
    /* TODO: support multi-rate in dmp off mode */

    if (!mEnabled) {
        LOGV_IF(ENG_VERBOSE, "skip resetDataRates");
        return 0;
    }
    /* search the minimum delay requested across all enabled sensors */
    /* skip setting rates if it is not changed */
    for (int i = 0; i < NumSensors; i++) {
        if (mEnabled & (1 << i)) {
            int64_t ns = mDelays[i];
            if ((wanted == ns) && (i != Pressure)) {
                LOGV_IF(ENG_VERBOSE, "skip resetDataRates");
                return 0;
            }
            LOGV_IF(ENG_VERBOSE, "resetDataRates - mDelays[%d]=%lld", i, mDelays[i]);
            wanted = wanted < ns ? wanted : ns;
        }
    }

    gyroRate = wanted;
    accelRate = wanted;
    compassRate = wanted;
    pressureRate = wanted;

    /* set mpl data rate */
    inv_set_gyro_sample_rate((int)gyroRate/1000LL);
    inv_set_accel_sample_rate((int)accelRate/1000LL);
    inv_set_compass_sample_rate((int)compassRate/1000LL);

    LOGV_IF(PROCESS_VERBOSE,
            "HAL:MPL gyro sample rate: (mpl)=%lld us (mpu)=%.2f Hz", 
            gyroRate/1000LL, 1000000000.f / gyroRate);
    LOGV_IF(PROCESS_VERBOSE,
            "HAL:MPL accel sample rate: (mpl)=%lld us (mpu)=%.2f Hz", 
            accelRate/1000LL, 1000000000.f / accelRate);
    LOGV_IF(PROCESS_VERBOSE,
            "HAL:MPL compass sample rate: (mpl)=%lld us (mpu)=%.2f Hz", 
            compassRate/1000LL, 1000000000.f / compassRate);

    /* reset dmp rate */
    getDmpRate (&wanted);

    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %.0f > %s (%lld)",
            1000000000.f / wanted, mpu.gyro_fifo_rate,
            getTimestamp());
    tempFd = open(mpu.gyro_fifo_rate, O_RDWR);
    res = write_attribute_sensor(tempFd, 1000000000.f / wanted);
    LOGE_IF(res < 0, "HAL:sampling frequency update delay error");
            
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %.0f > %s (%lld)",
            1000000000.f / gyroRate, mpu.gyro_rate,
            getTimestamp());
    tempFd = open(mpu.gyro_rate, O_RDWR);
    res = write_attribute_sensor(tempFd, 1000000000.f / gyroRate);
    if(res < 0) {
        LOGE("HAL:GYRO update delay error");
    }
    
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %.0f > %s (%lld)",
            1000000000.f / accelRate, mpu.accel_rate,
            getTimestamp());
    tempFd = open(mpu.accel_rate, O_RDWR);
    res = write_attribute_sensor(tempFd, 1000000000.f / accelRate);
    LOGE_IF(res < 0, "HAL:ACCEL update delay error");
   
    if (compassRate < mCompassSensor->getMinDelay() * 1000LL) {
        compassRate = mCompassSensor->getMinDelay() * 1000LL;
    }
    mCompassSensor->setDelay(ID_M, compassRate);
 
    mPressureSensor->setDelay(ID_PS, pressureRate);
   
    return res;
}

void MPLSensor::initBias()
{
    VFUNC_LOG;

    LOGV_IF(ENG_VERBOSE, "HAL:inititalize dmp and device offsets to 0");
    if(write_attribute_sensor_continuous(accel_x_dmp_bias_fd, 0) < 0) {
        LOGE("HAL:Error writing to accel_x_dmp_bias");
    }
    if(write_attribute_sensor_continuous(accel_y_dmp_bias_fd, 0) < 0) {
        LOGE("HAL:Error writing to accel_y_dmp_bias");
    }
    if(write_attribute_sensor_continuous(accel_z_dmp_bias_fd, 0) < 0) {
        LOGE("HAL:Error writing to accel_z_dmp_bias");
    }
    
    if(write_attribute_sensor_continuous(accel_x_offset_fd, 0) < 0) {
        LOGE("HAL:Error writing to accel_x_offset");
    }
    if(write_attribute_sensor_continuous(accel_y_offset_fd, 0) < 0) {
        LOGE("HAL:Error writing to accel_y_offset");
    }
    if(write_attribute_sensor_continuous(accel_z_offset_fd, 0) < 0) {
        LOGE("HAL:Error writing to accel_z_offset");
    }
    
    if(write_attribute_sensor_continuous(gyro_x_dmp_bias_fd, 0) < 0) {
        LOGE("HAL:Error writing to gyro_x_dmp_bias");
    }
    if(write_attribute_sensor_continuous(gyro_y_dmp_bias_fd, 0) < 0) {
        LOGE("HAL:Error writing to gyro_y_dmp_bias");
    }
    if(write_attribute_sensor_continuous(gyro_z_dmp_bias_fd, 0) < 0) {
        LOGE("HAL:Error writing to gyro_z_dmp_bias");
    }
    
    if(write_attribute_sensor_continuous(gyro_x_offset_fd, 0) < 0) {
        LOGE("HAL:Error writing to gyro_x_offset");
    }
    if(write_attribute_sensor_continuous(gyro_y_offset_fd, 0) < 0) {
        LOGE("HAL:Error writing to gyro_y_offset");
    }
    if(write_attribute_sensor_continuous(gyro_z_offset_fd, 0) < 0) {
        LOGE("HAL:Error writing to gyro_z_offset");
    }
    return;
}
 
/*TODO: reg_dump in a separate file*/
void MPLSensor::sys_dump(bool fileMode)
{
    VFUNC_LOG;

    char sysfs_path[MAX_SYSFS_NAME_LEN];
    char scan_element_path[MAX_SYSFS_NAME_LEN];

    memset(sysfs_path, 0, sizeof(sysfs_path));
    memset(scan_element_path, 0, sizeof(scan_element_path));
    inv_get_sysfs_path(sysfs_path);
    sprintf(scan_element_path, "%s%s", sysfs_path, "/scan_elements");

    read_sysfs_dir(fileMode, sysfs_path);
    read_sysfs_dir(fileMode, scan_element_path);

    dump_dmp_img("/data/local/read_img.h");
    return;
}
