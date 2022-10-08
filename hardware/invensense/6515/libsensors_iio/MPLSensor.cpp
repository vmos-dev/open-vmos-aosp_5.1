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

#define LOG_NDEBUG 0

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
#include <utils/Vector.h>
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
#define USE_LPQ_AT_FASTEST

#ifdef THIRD_PARTY_ACCEL
#pragma message("HAL:build third party accel support")
#define USE_THIRD_PARTY_ACCEL (1)
#else
#define USE_THIRD_PARTY_ACCEL (0)
#endif

#define MAX_SYSFS_ATTRB (sizeof(struct sysfs_attrbs) / sizeof(char*))

/******************************************************************************/
/*  MPL Interface                                                             */
/******************************************************************************/

//#define INV_PLAYBACK_DBG
#ifdef INV_PLAYBACK_DBG
static FILE *logfile = NULL;
#endif

/*******************************************************************************
 * MPLSensor class implementation
 ******************************************************************************/

static struct timespec mt_pre;

// following extended initializer list would only be available with -std=c++11
//  or -std=gnu+11
MPLSensor::MPLSensor(CompassSensor *compass, int (*m_pt2AccelCalLoadFunc)(long *))
                       : SensorBase(NULL, NULL),
                         mMasterSensorMask(INV_ALL_SENSORS),
                         mLocalSensorMask(0),
                         mPollTime(-1),
                         mStepCountPollTime(-1),
                         mHaveGoodMpuCal(0),
                         mGyroAccuracy(0),
                         mAccelAccuracy(0),
                         mCompassAccuracy(0),
                         dmp_orient_fd(-1),
                         mDmpOrientationEnabled(0),
                         dmp_sign_motion_fd(-1),
                         mDmpSignificantMotionEnabled(0),
                         dmp_pedometer_fd(-1),
                         mDmpPedometerEnabled(0),
                         mDmpStepCountEnabled(0),
                         mEnabled(0),
                         mBatchEnabled(0),
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
                         mLastStepCount(-1),
                         mLeftOverBufferSize(0),
                         mInitial6QuatValueAvailable(0),
                         mFlushBatchSet(0),
                         mSkipReadEvents(0),
                         mDataMarkerDetected(0),
                         mEmptyDataMarkerDetected(0) {
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
    memset(mInitial6QuatValue, 0, sizeof(mInitial6QuatValue));
    mFlushSensorEnabledVector.setCapacity(NumSensors);

    /* setup sysfs paths */
    inv_init_sysfs_attributes();

    /* get chip name */
    if (inv_get_chip_name(chip_ID) != INV_SUCCESS) {
        LOGE("HAL:ERR- Failed to get chip ID\n");
    } else {
        LOGV_IF(PROCESS_VERBOSE, "HAL:Chip ID= %s\n", chip_ID);
    }

    enable_iio_sysfs();

#ifdef ENABLE_PRESSURE
    /* instantiate pressure sensor on secondary bus */
    mPressureSensor = new PressureSensor((const char*)mSysfsPath);
#endif

    /* reset driver master enable */
    masterEnable(0);

    /* Load DMP image if capable, ie. MPU6515 */
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
#if 1
    /* the following threshold can be modified for SMD sensitivity */
    int motionThreshold = 3000;
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                motionThreshold, mpu.smd_threshold, getTimestamp());
        res = write_sysfs_int(mpu.smd_threshold, motionThreshold);
#endif
#if 0
    int StepCounterThreshold = 5;
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                StepCounterThreshold, mpu.pedometer_step_thresh, getTimestamp());
        res = write_sysfs_int(mpu.pedometer_step_thresh, StepCounterThreshold);
#endif

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
    memset(mPendingFlushEvents, 0, sizeof(mPendingEvents));

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

#ifdef ENABLE_PRESSURE
    mPendingEvents[Pressure].version = sizeof(sensors_event_t);
    mPendingEvents[Pressure].sensor = ID_PS;
    mPendingEvents[Pressure].type = SENSOR_TYPE_PRESSURE;
    mPendingEvents[Pressure].magnetic.status =
        SENSOR_STATUS_ACCURACY_HIGH;
#endif

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

    mSmEvents.version = sizeof(sensors_event_t);
    mSmEvents.sensor = ID_SM;
    mSmEvents.type = SENSOR_TYPE_SIGNIFICANT_MOTION;
    mSmEvents.acceleration.status = SENSOR_STATUS_UNRELIABLE;

    mSdEvents.version = sizeof(sensors_event_t);
    mSdEvents.sensor = ID_P;
    mSdEvents.type = SENSOR_TYPE_STEP_DETECTOR;
    mSdEvents.acceleration.status = SENSOR_STATUS_UNRELIABLE;

    mScEvents.version = sizeof(sensors_event_t);
    mScEvents.sensor = ID_SC;
    mScEvents.type = SENSOR_TYPE_STEP_COUNTER;
    mScEvents.acceleration.status = SENSOR_STATUS_UNRELIABLE;

    /* Event Handlers for HW and Virtual Sensors */
    mHandlers[RotationVector] = &MPLSensor::rvHandler;
    mHandlers[GameRotationVector] = &MPLSensor::grvHandler;
    mHandlers[LinearAccel] = &MPLSensor::laHandler;
    mHandlers[Gravity] = &MPLSensor::gravHandler;
    mHandlers[Gyro] = &MPLSensor::gyroHandler;
    mHandlers[RawGyro] = &MPLSensor::rawGyroHandler;
    mHandlers[Accelerometer] = &MPLSensor::accelHandler;
    mHandlers[MagneticField] = &MPLSensor::compassHandler;
    mHandlers[RawMagneticField] = &MPLSensor::rawCompassHandler;
    mHandlers[Orientation] = &MPLSensor::orienHandler;
    mHandlers[GeomagneticRotationVector] = &MPLSensor::gmHandler;
#ifdef ENABLE_PRESSURE
    mHandlers[Pressure] = &MPLSensor::psHandler;
#endif

    /* initialize delays to reasonable values */
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

    /* disable all sensors and features */
    masterEnable(0);
    enableGyro(0);
    enableLowPowerAccel(0);
    enableAccel(0);
    enableCompass(0,0);
#ifdef ENABLE_PRESSURE
    enablePressure(0);
#endif
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
        if(fprintf(tempFp, "%d", 1) < 0) {
            LOGE("HAL:could not enable timestamp");
        }
        if(fclose(tempFp) < 0) {
            LOGE("HAL:could not close timestamp");
        }
    }

    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
            IIO_BUFFER_LENGTH, mpu.buffer_length, getTimestamp());
    tempFp = fopen(mpu.buffer_length, "w");
    if (tempFp == NULL) {
        LOGE("HAL:could not open buffer length");
    } else {
        if (fprintf(tempFp, "%d", IIO_BUFFER_LENGTH) < 0) {
            LOGE("HAL:could not write buffer length");
        }
        if (fclose(tempFp) < 0) {
            LOGE("HAL:could not close buffer length");
        }
    }

    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
            1, mpu.chip_enable, getTimestamp());
    tempFp = fopen(mpu.chip_enable, "w");
    if (tempFp == NULL) {
        LOGE("HAL:could not open chip enable");
    } else {
        if (fprintf(tempFp, "%d", 1) < 0) {
            LOGE("HAL:could not write chip enable");
        }
        if (fclose(tempFp) < 0) {
            LOGE("HAL:could not close chip enable");
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
                if (inv_load_dmp(fptr) < 0) {
                    LOGE("HAL:load DMP failed");
                } else {
                    LOGV_IF(PROCESS_VERBOSE, "HAL:DMP loaded");
                }
                if (fclose(fptr) < 0) {
                    LOGE("HAL:could not close dmp firmware");
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
               &om[6], &om[7], &om[8]) < 0) {
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
        if (fclose(fptr) < 0) {
            LOGE("HAL:Could not close gyro mounting matrix");
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
            &om[6], &om[7], &om[8]) < 0) {
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
        if (fclose(fptr) < 0) {
            LOGE("HAL:could not close accel mounting matrix");
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
#define O_ENABLED   ((1 << ID_O)  & enabled_sensors)
#define LA_ENABLED  ((1 << ID_LA) & enabled_sensors)
#define GR_ENABLED  ((1 << ID_GR) & enabled_sensors)
#define RV_ENABLED  ((1 << ID_RV) & enabled_sensors)
#define GRV_ENABLED ((1 << ID_GRV) & enabled_sensors)
#define GMRV_ENABLED ((1 << ID_GMRV) & enabled_sensors)

#ifdef ENABLE_PRESSURE
#define PS_ENABLED  ((1 << ID_PS) & enabled_sensors)
#endif

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

int MPLSensor::setDmpFeature(int en)
{
    int res = 0;

    // set sensor engine and fifo
    if ((mFeatureActiveMask & DMP_FEATURE_MASK) || en) {
        if ((mFeatureActiveMask & INV_DMP_6AXIS_QUATERNION) ||
                (mFeatureActiveMask & INV_DMP_PED_QUATERNION) ||
                (mFeatureActiveMask & INV_DMP_QUATERNION)) {
            res = enableGyro(1);
            if (res < 0) {
                return res;
            }
            if (!(mLocalSensorMask & mMasterSensorMask & INV_THREE_AXIS_GYRO)) {
                res = turnOffGyroFifo();
                if (res < 0) {
                    return res;
                }
            }
        }
        res = enableAccel(1);
        if (res < 0) {
            return res;
        }
        if (!(mLocalSensorMask & mMasterSensorMask & INV_THREE_AXIS_ACCEL)) {
            res = turnOffAccelFifo();
            if (res < 0) {
                return res;
            }
        }
    } else {
        if (!(mLocalSensorMask & mMasterSensorMask & INV_THREE_AXIS_GYRO)) {
            res = enableGyro(0);
            if (res < 0) {
                return res;
            }
        }
        if (!(mLocalSensorMask & mMasterSensorMask & INV_THREE_AXIS_ACCEL)) {
            res = enableAccel(0);
            if (res < 0) {
                return res;
            }
        }
    }

    // set sensor data interrupt
    uint32_t dataInterrupt = (mEnabled || (mFeatureActiveMask & INV_DMP_BATCH_MODE));
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                !dataInterrupt, mpu.dmp_event_int_on, getTimestamp());
    if (write_sysfs_int(mpu.dmp_event_int_on, !dataInterrupt) < 0) {
        res = -1;
        LOGE("HAL:ERR can't enable DMP event interrupt");
    }
    return res;
}

int MPLSensor::computeAndSetDmpState()
{
    int res = 0;
    bool dmpState = 0;

    if (mFeatureActiveMask) {
        dmpState = 1;
        LOGV_IF(PROCESS_VERBOSE, "HAL:computeAndSetDmpState() mFeatureActiveMask = 1");
    } else if ((mEnabled & VIRTUAL_SENSOR_9AXES_MASK)
                        || (mEnabled & VIRTUAL_SENSOR_GYRO_6AXES_MASK)) {
            if (checkLPQuaternion() && checkLPQRateSupported()) {
                dmpState = 1;
                LOGV_IF(PROCESS_VERBOSE, "HAL:computeAndSetDmpState() Sensor Fusion = 1");
            }
    } /*else {
        unsigned long sensor = mLocalSensorMask & mMasterSensorMask;
        if (sensor & (INV_THREE_AXIS_ACCEL & INV_THREE_AXIS_GYRO)) {
            dmpState = 1;
            LOGV_IF(PROCESS_VERBOSE, "HAL:computeAndSetDmpState() accel and gyro = 1");
        }
    }*/

    // set Dmp state
    res = onDmp(dmpState);
    if (res < 0)
        return res;

    if (dmpState) {
        // set DMP rate to 200Hz
        LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                200, mpu.accel_fifo_rate, getTimestamp());
        if (write_sysfs_int(mpu.accel_fifo_rate, 200) < 0) {
            res = -1;
            LOGE("HAL:ERR can't set rate to 200Hz");
            return res;
        }
    }
    LOGV_IF(PROCESS_VERBOSE, "HAL:DMP is set %s", (dmpState ? "on" : "off"));
    return dmpState;
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
    } else {
        //Disable Accel if no sensor needs it
        if (!(mFeatureActiveMask & DMP_FEATURE_MASK)
                               && (!(mLocalSensorMask & mMasterSensorMask
                                                   & INV_THREE_AXIS_ACCEL))) {
            res = enableAccel(0);
                if (res < 0)
                    return res;
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

int MPLSensor::checkPedStandaloneBatched(void)
{
    VFUNC_LOG;
    int res = 0;

    if ((mFeatureActiveMask & INV_DMP_PEDOMETER) &&
            (mBatchEnabled & (1 << StepDetector))) {
        res = 1;
    } else
        res = 0;

    LOGV_IF(ENG_VERBOSE, "HAL:checkPedStandaloneBatched=%d", res);
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

    // Set DMP Ped standalone
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
            en, mpu.step_detector_on, getTimestamp());
    if (write_sysfs_int(mpu.step_detector_on, en) < 0) {
        LOGE("HAL:ERR can't write DMP step_detector_on");
        res = -1;   //Indicate an err
    }

    // Set DMP Step indicator
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
        // LOGV_IF(ENG_VERBOSE, "before mLocalSensorMask=0x%lx", mLocalSensorMask);
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

int MPLSensor::setPedQuaternionRate(int64_t wanted)
{
    VFUNC_LOG;
    int res = 0;

    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                int(1000000000.f / wanted), mpu.ped_q_rate,
                getTimestamp());
    res = write_sysfs_int(mpu.ped_q_rate, 1000000000.f / wanted);
    LOGV_IF(PROCESS_VERBOSE,
                "HAL:DMP ped quaternion rate %.2f Hz", 1000000000.f / wanted);

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

int MPLSensor::set6AxisQuaternionRate(int64_t wanted)
{
    VFUNC_LOG;
    int res = 0;

    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                int(1000000000.f / wanted), mpu.six_axis_q_rate,
                getTimestamp());
    res = write_sysfs_int(mpu.six_axis_q_rate, 1000000000.f / wanted);
    LOGV_IF(PROCESS_VERBOSE,
                "HAL:DMP six axis rate %.2f Hz", 1000000000.f / wanted);

    return res;
}

/* this is for batch  mode only */
int MPLSensor::checkLPQRateSupported(void)
{
    VFUNC_LOG;
#ifndef USE_LPQ_AT_FASTEST
    return ((mDelays[GameRotationVector] <= RATE_200HZ) ? 0 :1);
#else
    return 1;
#endif
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

int MPLSensor::setQuaternionRate(int64_t wanted)
{
    VFUNC_LOG;
    int res = 0;

    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
            int(1000000000.f / wanted), mpu.three_axis_q_rate,
            getTimestamp());
    res = write_sysfs_int(mpu.three_axis_q_rate, 1000000000.f / wanted);
    LOGV_IF(PROCESS_VERBOSE,
            "HAL:DMP three axis rate %.2f Hz", 1000000000.f / wanted);

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

        if (interruptMode) {
            if(!checkPedStandaloneBatched()) {
                //Enable DMP Pedometer Interrupt
                LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                        en, mpu.pedometer_int_on, getTimestamp());
                if (write_sysfs_int(mpu.pedometer_int_on, en) < 0) {
                    LOGE("HAL:ERR can't enable Android Pedometer Interrupt");
                    res = -1;   // indicate an err
                    return res;
                }
            }
        }

        if (interruptMode) {
            mFeatureActiveMask |= INV_DMP_PEDOMETER;
        }
        else {
            mFeatureActiveMask |= INV_DMP_PEDOMETER_STEP;
            mStepCountPollTime = 100000000LL;
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
            LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                    en, mpu.pedometer_on, getTimestamp());
            if (write_sysfs_int(mpu.pedometer_on, en) < 0) {
                LOGE("HAL:ERR can't enable Android Pedometer");
                res = -1;
                return res;
            }
        }

        /* if feature is not step detector */
        if (!(mFeatureActiveMask & INV_DMP_PEDOMETER)) {
            LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                    en, mpu.pedometer_int_on, getTimestamp());
            if (write_sysfs_int(mpu.pedometer_int_on, en) < 0) {
                LOGE("HAL:ERR can't enable Android Pedometer Interrupt");
                res = -1;
                return res;
            }
        }
    }

    if ((res = setDmpFeature(en)) < 0)
        return res;

    if ((res = computeAndSetDmpState()) < 0)
        return res;

    if (!mBatchEnabled && (resetDataRates() < 0))
        return res;

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
    /* TODO: handle ID_RM if third party compass cal is used */
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

#ifdef ENABLE_PRESSURE
int MPLSensor::enablePressure(int en)
{
    VFUNC_LOG;

    int res = 0;

    if (mPressureSensor)
        res = mPressureSensor->enable(ID_PS, en);

    return res;
}
#endif

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
#ifdef ENABLE_PRESSURE
        /* Invensense Pressure on secondary bus */
        if (PS_ENABLED) {
            LOGV_IF(ENG_VERBOSE, "PS ENABLED");
            mLocalSensorMask |= INV_ONE_AXIS_PRESSURE;
        } else {
            LOGV_IF(ENG_VERBOSE, "PS DISABLED");
            mLocalSensorMask &= ~INV_ONE_AXIS_PRESSURE;
        }
#else
        LOGV_IF(ENG_VERBOSE, "PS DISABLED");
        mLocalSensorMask &= ~INV_ONE_AXIS_PRESSURE;
#endif

        if (LA_ENABLED || GR_ENABLED || RV_ENABLED || O_ENABLED
                       || (GRV_ENABLED && GMRV_ENABLED)) {
            LOGV_IF(ENG_VERBOSE, "FUSION ENABLED");
            mLocalSensorMask |= ALL_MPL_SENSORS_NP;
            break;
        }

        if (GRV_ENABLED) {
            if (!(mFeatureActiveMask & INV_DMP_BATCH_MODE) ||
                !(mBatchEnabled & (1 << GameRotationVector))) {
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

#ifdef ENABLE_PRESSURE
        if(!A_ENABLED && !M_ENABLED && !RM_ENABLED &&
               !GRV_ENABLED && !GMRV_ENABLED && !GY_ENABLED && !RGY_ENABLED &&
               !PS_ENABLED) {
#else
        if(!A_ENABLED && !M_ENABLED && !RM_ENABLED &&
               !GRV_ENABLED && !GMRV_ENABLED && !GY_ENABLED && !RGY_ENABLED) {
#endif
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
#ifdef ENABLE_PRESSURE
        (mPressureSensor->isIntegrated() << Pressure) |
#endif
        (mCompassSensor->isIntegrated() << RawMagneticField))) {

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

#ifdef ENABLE_PRESSURE
     if (changed & (1 << Pressure)) {
        LOGV_IF(ENG_VERBOSE, "HAL:enableSensors - pressure %s",
                (sensors & INV_ONE_AXIS_PRESSURE? "enable": "disable"));
        res = enablePressure(!!(sensors & INV_ONE_AXIS_PRESSURE));
        if(res < 0) {
            return res;
        }
    }
#endif

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
        setBatch(batchMode,0);
    }
    mOldBatchEnabledMask = batchMode;

    /* check for invn hardware sensors change */
    if (changed & ((1 << Gyro) | (1 << RawGyro) | (1 << Accelerometer) |
            (mCompassSensor->isIntegrated() << MagneticField) |
#ifdef ENABLE_PRESSURE
            (mPressureSensor->isIntegrated() << Pressure) |
#endif
            (mCompassSensor->isIntegrated() << RawMagneticField))) {
        LOGV_IF(ENG_VERBOSE,
                "HAL DEBUG: Gyro, Accel, Compass, Pressure changes");
        if ((checkSmdSupport() == 1) || (checkPedometerSupport() == 1) || (sensors &
            (INV_THREE_AXIS_GYRO
                | INV_THREE_AXIS_ACCEL
#ifdef ENABLE_PRESSURE
                | (INV_ONE_AXIS_PRESSURE * mPressureSensor->isIntegrated())
#endif
                | (INV_THREE_AXIS_COMPASS * mCompassSensor->isIntegrated())))) {
            LOGV_IF(ENG_VERBOSE, "SMD or Hardware sensors enabled");
            LOGV_IF(ENG_VERBOSE,
                    "mFeatureActiveMask=0x%llx", mFeatureActiveMask);
                LOGV_IF(ENG_VERBOSE, "HAL DEBUG: LPQ, SMD, SO enabled");
                // disable DMP event interrupt only (w/ data interrupt)
                LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                        0, mpu.dmp_event_int_on, getTimestamp());
                if (write_sysfs_int(mpu.dmp_event_int_on, 0) < 0) {
                    res = -1;
                    LOGE("HAL:ERR can't disable DMP event interrupt");
                    return res;
                }
            LOGV_IF(ENG_VERBOSE, "mFeatureActiveMask=0x%llx", mFeatureActiveMask);
            LOGV_IF(ENG_VERBOSE, "DMP_FEATURE_MASK=0x%x", DMP_FEATURE_MASK);
        if ((mFeatureActiveMask & (long long)DMP_FEATURE_MASK) &&
                    !((mFeatureActiveMask & INV_DMP_6AXIS_QUATERNION) ||
                     (mFeatureActiveMask & INV_DMP_PED_STANDALONE) ||
                     (mFeatureActiveMask & INV_DMP_PED_QUATERNION) ||
                     (mFeatureActiveMask & INV_DMP_BATCH_MODE))) {
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
        } else { // all sensors idle
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
    } /*else {
      LOGV_IF(ENG_VERBOSE, "HAL DEBUG: mEnabled");
      if (sensors &
            (INV_THREE_AXIS_GYRO
                | INV_THREE_AXIS_ACCEL
                | (INV_THREE_AXIS_COMPASS * mCompassSensor->isIntegrated()))) {
            res = masterEnable(1);
            if(res < 0)
                return res;
        }
    }*/

    if (!batchMode && (resetDataRates() < 0)) {
        LOGE("HAL:ERR can't reset output rate back to original setting");
    }

    if(mFeatureActiveMask || sensors) {
        res = masterEnable(1);
        if(res < 0)
            return res;
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
    for(int i = 0; i <= LAST_HW_SENSOR; i++) {
        // if any one of the hardware sensor is in continuous data mode
        // turn off batch mode.
        if ((enableSensors & (1 << i)) && !(tempBatchSensor & (1 << i))) {
            LOGV_IF(ENG_VERBOSE, "HAL:computeBatchSensorMask: "
                    "hardware sensor on continuous mode:%d", i);
            return 0;
        }
        if ((enableSensors & (1 << i)) && (tempBatchSensor & (1 << i))) {
            LOGV_IF(ENG_VERBOSE,
                    "HAL:computeBatchSensorMask: hardware sensor is batch:%d",
                    i);
            // if hardware sensor is batched, check if virtual sensor is also batched
            if ((enableSensors & (1 << GameRotationVector))
                            && !(tempBatchSensor & (1 << GameRotationVector))) {
            LOGV_IF(ENG_VERBOSE,
                    "HAL:computeBatchSensorMask: but virtual sensor is not:%d",
                    i);
                return 0;
            }
        }
    }

    // if virtual sensors are on but not batched, turn off batch mode.
    for(int i = Orientation; i <= GeomagneticRotationVector; i++) {
        if ((enableSensors & (1 << i)) && !(tempBatchSensor & (1 << i))) {
             LOGV_IF(ENG_VERBOSE, "HAL:computeBatchSensorMask: "
                     "composite sensor on continuous mode:%d", i);
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
    if (toggleEnable == 1) {
        res = masterEnable(0);
        if (res < 0) {
            return res;
        }
    }

    /* step detector is enabled and */
    /* batch mode is standalone */
    if (en && (mFeatureActiveMask & INV_DMP_PEDOMETER) &&
            (featureMask & INV_DMP_PED_STANDALONE)) {
        LOGV_IF(ENG_VERBOSE, "setBatch: ID_P only = 0x%x", mBatchEnabled);
        enablePedStandalone(1);
    } else {
        enablePedStandalone(0);
    }

    /* step detector and GRV are enabled and */
    /* batch mode is ped q */
    if (en && (mFeatureActiveMask & INV_DMP_PEDOMETER) &&
            (mEnabled & (1 << GameRotationVector)) &&
            (featureMask & INV_DMP_PED_QUATERNION)) {
        LOGV_IF(ENG_VERBOSE, "setBatch: ID_P and GRV or ALL = 0x%x", mBatchEnabled);
        LOGV_IF(ENG_VERBOSE, "setBatch: ID_P is enabled for batching, "
                "PED quat will be automatically enabled");
        enableLPQuaternion(0);
        enablePedQuaternion(1);
    } else if (!(featureMask & INV_DMP_PED_STANDALONE)){
        enablePedQuaternion(0);
    } else {
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
        LOGV_IF(ENG_VERBOSE, "setBatch: GRV = 0x%x", mBatchEnabled);
        enableLPQuaternion(0);
        enable6AxisQuaternion(1);
        setInitial6QuatValue();
    } else if (!(featureMask & INV_DMP_PED_QUATERNION)){
        LOGV_IF(ENG_VERBOSE, "setBatch: Toggle back to normal 6 axis");
        if (mEnabled & (1 << GameRotationVector)) {
            enableLPQuaternion(checkLPQRateSupported());
        }
        enable6AxisQuaternion(0);
    } else {
        enable6AxisQuaternion(0);
    }

    writeBatchTimeout(en);

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
    }

    // set sensor data interrupt
    uint32_t dataInterrupt = (mEnabled || (mFeatureActiveMask & INV_DMP_BATCH_MODE));
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                !dataInterrupt, mpu.dmp_event_int_on, getTimestamp());
    if (write_sysfs_int(mpu.dmp_event_int_on, !dataInterrupt) < 0) {
        res = -1;
        LOGE("HAL:ERR can't enable DMP event interrupt");
    }

    if (toggleEnable == 1) {
        if (mFeatureActiveMask || mEnabled)
            masterEnable(1);
    }
    return res;
}

int MPLSensor::writeBatchTimeout(int en)
{
    VFUNC_LOG;

    int64_t timeoutInMs = 0;
    if (en) {
        /* take the minimum batchmode timeout */
        int64_t timeout = 100000000000LL;
        int64_t ns = 0;
        for (int i = 0; i < NumSensors; i++) {
            LOGV_IF(1, "mFeatureActiveMask=0x%016llx, mEnabled=0x%01x, mBatchEnabled=0x%x",
                            mFeatureActiveMask, mEnabled, mBatchEnabled);
            if (((mEnabled & (1 << i)) && (mBatchEnabled & (1 << i))) ||
                    (checkPedStandaloneBatched() && (i == StepDetector))) {
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

    LOGV_IF(PROCESS_VERBOSE,
                    "HAL: batch timeout set to %lld ms", timeoutInMs);

    if(mBatchTimeoutInMs != timeoutInMs) {
        /* write required timeout to sysfs */
        LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %lld > %s (%lld)",
                timeoutInMs, mpu.batchmode_timeout, getTimestamp());
        if (write_sysfs_int(mpu.batchmode_timeout, timeoutInMs) < 0) {
            LOGE("HAL:ERR can't write batchmode_timeout");
        }
    }
    /* remember last timeout value */
    mBatchTimeoutInMs = timeoutInMs;

    return 0;
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
    return update | mCompassOverFlow;
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
    return update | mCompassOverFlow;
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
    s->orientation.status = status;
    update |= isCompassDisabled();
    LOGV_IF(HANDLER_DATA, "HAL:rv data: %+f %+f %+f %+f %+f %d- %+lld - %d",
            s->data[0], s->data[1], s->data[2], s->data[3], s->data[4], s->orientation.status, s->timestamp,
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
    s->orientation.status = status;

    LOGV_IF(HANDLER_DATA, "HAL:grv data: %+f %+f %+f %+f %+f %d- %+lld - %d",
            s->data[0], s->data[1], s->data[2], s->data[3], s->data[4], s->orientation.status, s->timestamp,
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

    /* Capture timestamp in HAL */
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    s->timestamp = (int64_t) ts.tv_sec * 1000000000 + ts.tv_nsec;

    LOGV_IF(HANDLER_DATA, "HAL:sm data: %f - %lld - %d",
            s->data[0], s->timestamp, update);
    return update;
}

int MPLSensor::gmHandler(sensors_event_t* s)
{
    VHANDLER_LOG;
    int8_t status;
    int update = 0;
    update = inv_get_sensor_type_geomagnetic_rotation_vector(s->data, &status,
                                                             &s->timestamp);
    s->orientation.status = status;
    LOGV_IF(HANDLER_DATA, "HAL:gm data: %+f %+f %+f %+f %+f %d- %+lld - %d",
            s->data[0], s->data[1], s->data[2], s->data[3], s->data[4], s->orientation.status, s->timestamp, update);
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

int MPLSensor::sdHandler(sensors_event_t* s)
{
    VHANDLER_LOG;
    int update = 1;

    /* When event is triggered, set data to 1 */
    s->data[0] = 1;
    s->data[1] = 0.f;
    s->data[2] = 0.f;

    /* get current timestamp */
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts) ;
    s->timestamp = (int64_t)ts.tv_sec * 1000000000 + ts.tv_nsec;

    LOGV_IF(HANDLER_DATA, "HAL:sd data: %f - %lld - %d",
            s->data[0], s->timestamp, update);
    return update;
}

int MPLSensor::scHandler(sensors_event_t* s)
{
    VHANDLER_LOG;
    int update = 1;

    /* Set step count */
#if defined ANDROID_KITKAT
    s->u64.step_counter = mLastStepCount;
    LOGV_IF(HANDLER_DATA, "HAL:sc data: %lld - %lld - %d",
            s->u64.step_counter, s->timestamp, update);
#else
    s->step_counter = mLastStepCount;
    LOGV_IF(HANDLER_DATA, "HAL:sc data: %lld - %lld - %d",
            s->step_counter, s->timestamp, update);
#endif
    return update;
}

int MPLSensor::metaHandler(sensors_event_t* s, int flags)
{
    VHANDLER_LOG;
    int update = 1;

#if defined ANDROID_KITKAT
    /* initalize SENSOR_TYPE_META_DATA */
    s->version = 0;
    s->sensor = 0;
    s->reserved0 = 0;
    s->timestamp = 0LL;

    switch(flags) {
    case META_DATA_FLUSH_COMPLETE:
        s->type = SENSOR_TYPE_META_DATA;
        s->meta_data.what = flags;
        s->meta_data.sensor = mFlushSensorEnabledVector[0];
        mFlushSensorEnabledVector.removeAt(0);
        mFlushBatchSet = 0;
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
#endif
    return 1;
}

int MPLSensor::enable(int32_t handle, int en)
{
    VFUNC_LOG;

    android::String8 sname;
    int what = -1, err = 0;
    int batchMode = 0;

    if (uint32_t(handle) >= NumSensors)
        return -EINVAL;

    if (!en)
        mBatchEnabled &= ~(1 << handle);

    LOGV_IF(ENG_VERBOSE, "HAL:handle = %d", handle);

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
#ifdef ENABLE_PRESSURE
    case ID_PS:
        what = Pressure;
        sname = "Pressure";
        break;
#endif
    default:
        what = handle;
        sname = "Others";
        break;
    }

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
#ifdef ENABLE_PRESSURE
            case Pressure:
                if ((lastEnabled & (1 << what)) != (mEnabled & (1 << what))) {
                    changed |= (1 << what);
                }
                break;
#endif
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
        break;
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
#ifdef ENABLE_PRESSURE
   case ID_PS:
        what = Pressure;
        sname = "Pressure";
        break;
#endif
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
        case StepCounter:
            /* set limits of delivery rate of events */
            mStepCountPollTime = ns;
            LOGV_IF(ENG_VERBOSE, "step count rate =%lld ns", ns);
            break;
        case StepDetector:
        case SignificantMotion:
#ifdef ENABLE_DMP_SCREEN_AUTO_ROTATION
        case ID_SO:
#endif
            LOGV_IF(ENG_VERBOSE, "Step Detect, SMD, SO rate=%lld ns", ns);
            break;
        case Gyro:
        case RawGyro:
        case Accelerometer:
            // need to update delay since they are different
            // resetDataRates was called earlier
            // LOGV("what=%d mEnabled=%d mDelays[%d]=%lld previousDelay=%lld",
            // what, mEnabled, what, mDelays[what], previousDelay);
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
#ifdef ENABLE_PRESSURE
        int64_t pressureRate;
#endif

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

        /* initialize rate for each sensor */
        gyroRate = wanted;
        accelRate = wanted;
        compassRate = wanted;
#ifdef ENABLE_PRESSURE
        pressureRate = wanted;
#endif
        // same delay for 3rd party Accel or Compass
        wanted_3rd_party_sensor = wanted;

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
            inv_set_linear_acceleration_sample_rate(rateInus);
            inv_set_orientation_sample_rate(rateInus);
            inv_set_rotation_vector_sample_rate(rateInus);
            inv_set_gravity_sample_rate(rateInus);
            inv_set_orientation_geomagnetic_sample_rate(rateInus);
            inv_set_rotation_vector_6_axis_sample_rate(rateInus);
            inv_set_geomagnetic_rotation_vector_sample_rate(rateInus);

            LOGV_IF(ENG_VERBOSE,
                    "HAL:MPL virtual sensor sample rate: (mpl)=%d us (mpu)=%.2f Hz",
                    rateInus, 1000000000.f / wanted);
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
            if(mFeatureActiveMask & DMP_FEATURE_MASK) {
                bool setDMPrate= 0;
                gyroRate = wanted;
                accelRate = wanted;
                compassRate = wanted;
                // same delay for 3rd party Accel or Compass
                wanted_3rd_party_sensor = wanted;
                rateInus = (int)wanted / 1000LL;
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
        } else {

            if (GY_ENABLED || RGY_ENABLED) {
                wanted = (mDelays[Gyro] <= mDelays[RawGyro]?
                    (mEnabled & (1 << Gyro)? mDelays[Gyro]: mDelays[RawGyro]):
                    (mEnabled & (1 << RawGyro)? mDelays[RawGyro]: mDelays[Gyro]));
                LOGV_IF(ENG_VERBOSE, "mFeatureActiveMask=%016llx", mFeatureActiveMask);
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
                if (GY_ENABLED && mDelays[Gyro] < mDelays[Accelerometer]) {
                    wanted = mDelays[Gyro];
                } else if (RGY_ENABLED && mDelays[RawGyro]
                            < mDelays[Accelerometer]) {
                    wanted = mDelays[RawGyro];
                } else {
                    wanted = mDelays[Accelerometer];
                }
                LOGV_IF(ENG_VERBOSE, "mFeatureActiveMask=%016llx", mFeatureActiveMask);
                inv_set_accel_sample_rate((int)wanted/1000LL);
                LOGV_IF(ENG_VERBOSE, "HAL:MPL accel sample rate: (mpl)=%d us",
                        int(wanted/1000LL));
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
                    LOGV_IF(ENG_VERBOSE, "mFeatureActiveMask=%016llx", mFeatureActiveMask);
                }

                mCompassSensor->setDelay(ID_M, wanted);
                got = mCompassSensor->getDelay(ID_M);
                inv_set_compass_sample_rate(got / 1000);
                LOGV_IF(ENG_VERBOSE, "HAL:MPL compass sample rate: (mpl)=%d us",
                        int(got/1000LL));
            }
#ifdef ENABLE_PRESSURE
            if (PS_ENABLED) {
                int64_t pressureRate = mDelays[Pressure];
                LOGV_IF(ENG_VERBOSE, "mFeatureActiveMask=%016llx", mFeatureActiveMask);
                mPressureSensor->setDelay(ID_PS, pressureRate);
                LOGE_IF(res < 0, "HAL:PRESSURE update delay error");
            }
#endif
        }

        } //end of non batch mode

        unsigned long sensors = mLocalSensorMask & mMasterSensorMask;
        if (sensors &
            (INV_THREE_AXIS_GYRO
                | INV_THREE_AXIS_ACCEL
#ifdef ENABLE_PRESSURE
                | (INV_ONE_AXIS_PRESSURE * mPressureSensor->isIntegrated())
#endif
                | (INV_THREE_AXIS_COMPASS * mCompassSensor->isIntegrated()))) {
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

    // handle partial packet read and end marker
    // skip readEvents from hal_outputs
    if (!mFlushSensorEnabledVector.isEmpty()) {
        if (!mEmptyDataMarkerDetected) {
            // turn off sensors in data_builder
            resetMplStates();
        }
        mEmptyDataMarkerDetected = 0;
        mDataMarkerDetected = 0;

        // handle flush complete event
        for(int k = 0; k < mFlushSensorEnabledVector.size(); k++) {
            int sendEvent = metaHandler(&mPendingFlushEvents[k], META_DATA_FLUSH_COMPLETE);
            if(sendEvent && count > 0) {
                *data++ = mPendingFlushEvents[k];
                count--;
                numEventReceived++;
            }
        }
        return numEventReceived;
    }

    if (mSkipReadEvents) {
        return numEventReceived;
    }

    for (int i = 0; i < NumSensors; i++) {
        int update = 0;

        // handle step detector when ped_q is enabled
        if(mPedUpdate) {
            if (i == StepDetector) {
                update = readDmpPedometerEvents(data, count, ID_P, 1);
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
    mCompassOverFlow = 0;

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

    char *rdata = mIIOBuffer;
    ssize_t rsize = 0;
    ssize_t readCounter = 0;
    char *rdataP = NULL;
    bool doneFlag = 0;

    lp_quaternion_on = isLowPowerQuatEnabled() && checkLPQuaternion();
    sixAxis_quaternion_on = check6AxisQuatEnabled();
    ped_quaternion_on = checkPedQuatEnabled();
    ped_standalone_on = checkPedStandaloneEnabled();

    nbyte = MAX_READ_SIZE - mLeftOverBufferSize;

    /* check previous copied buffer */
    /* append with just read data */
    if (mLeftOverBufferSize > 0) {
        LOGV_IF(0, "append old buffer size=%d", mLeftOverBufferSize);
        memset(rdata, 0, sizeof(rdata));
        memcpy(rdata, mLeftOverBuffer, mLeftOverBufferSize);
            LOGV_IF(0,
            "HAL:input retrieve old buffer data=:%d, %d, %d, %d,%d, %d, %d, %d,%d, %d, "
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
            if(rsize > 0) {
                LOGV_IF(ENG_VERBOSE, "HAL:input data flush rsize=%d", (int)rsize);
#ifdef TESTING
                LOGV_IF(INPUT_DATA,
                     "HAL:input rdata:r=%d, n=%d,"
                     "%d, %d, %d, %d, %d, %d, %d, %d",
                     (int)rsize, nbyte,
                     rdataP[0], rdataP[1], rdataP[2], rdataP[3],
                     rdataP[4], rdataP[5], rdataP[6], rdataP[7]);
#endif
                mLeftOverBufferSize = 0;
            }
        }
        return;
    }

#ifdef TESTING
LOGV_IF(INPUT_DATA,
         "HAL:input just read rdataP:r=%d, n=%d,"
         "%d, %d, %d, %d,%d, %d, %d, %d,%d, %d, %d, %d,%d, %d, %d, %d,"
         "%d, %d, %d, %d,%d, %d, %d, %d\n",
         (int)rsize, nbyte,
         rdataP[0], rdataP[1], rdataP[2], rdataP[3],
         rdataP[4], rdataP[5], rdataP[6], rdataP[7],
         rdataP[8], rdataP[9], rdataP[10], rdataP[11],
         rdataP[12], rdataP[13], rdataP[14], rdataP[15],
         rdataP[16], rdataP[17], rdataP[18], rdataP[19],
         rdataP[20], rdataP[21], rdataP[22], rdataP[23]);

    LOGV_IF(INPUT_DATA,
         "HAL:input rdata:r=%d, n=%d,"
         "%d, %d, %d, %d,%d, %d, %d, %d,%d, %d, %d, %d,%d, %d, %d, %d,"
         "%d, %d, %d, %d,%d, %d, %d, %d\n",
         (int)rsize, nbyte,
         rdata[0], rdata[1], rdata[2], rdata[3],
         rdata[4], rdata[5], rdata[6], rdata[7],
         rdata[8], rdata[9], rdata[10], rdata[11],
         rdata[12], rdata[13], rdata[14], rdata[15],
         rdata[16], rdata[17], rdata[18], rdata[19],
         rdata[20], rdata[21], rdata[22], rdata[23]);
#endif
    /* reset data and count pointer */
    rdataP = rdata;
    readCounter = rsize + mLeftOverBufferSize;
    LOGV_IF(0, "HAL:input readCounter set=%d", (int)readCounter);

    if(readCounter < MAX_READ_SIZE) {
        // Handle standalone MARKER packet
        if (readCounter >= BYTES_PER_SENSOR) {
            data_format = *((short *)(rdata));
            if (data_format == DATA_FORMAT_MARKER) {
                LOGV_IF(ENG_VERBOSE && INPUT_DATA, "MARKER DETECTED:0x%x", data_format);
                readCounter -= BYTES_PER_SENSOR;
                rdata += BYTES_PER_SENSOR;
                if (!mFlushSensorEnabledVector.isEmpty()) {
                    mFlushBatchSet = 1;
                }
                mDataMarkerDetected = 1;
            }
            else if (data_format == DATA_FORMAT_EMPTY_MARKER) {
                LOGV_IF(ENG_VERBOSE && INPUT_DATA, "EMPTY MARKER DETECTED:0x%x", data_format);
                readCounter -= BYTES_PER_SENSOR;
                rdata += BYTES_PER_SENSOR;
                if (!mFlushSensorEnabledVector.isEmpty()) {
                    mFlushBatchSet = 1;
                }
                mEmptyDataMarkerDetected = 1;
            }
        }

        /* store packet then return */
        mLeftOverBufferSize = readCounter;
        memcpy(mLeftOverBuffer, rdata, mLeftOverBufferSize);

#ifdef TESTING
        LOGV_IF(1, "HAL:input data has batched partial packet");
        LOGV_IF(1, "HAL:input data batched mLeftOverBufferSize=%d", mLeftOverBufferSize);
        LOGV_IF(1,
            "HAL:input catch up batched retrieve buffer=:%d, %d, %d, %d, %d, %d, %d, %d,"
            "%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n",
            mLeftOverBuffer[0], mLeftOverBuffer[1], mLeftOverBuffer[2], mLeftOverBuffer[3],
            mLeftOverBuffer[4], mLeftOverBuffer[5], mLeftOverBuffer[6], mLeftOverBuffer[7],
            mLeftOverBuffer[8], mLeftOverBuffer[9], mLeftOverBuffer[10], mLeftOverBuffer[11],
            mLeftOverBuffer[12], mLeftOverBuffer[13], mLeftOverBuffer[14], mLeftOverBuffer[15]);
#endif
        mSkipReadEvents = 1;
        return;
    }

    LOGV_IF(INPUT_DATA && ENG_VERBOSE,
            "HAL:input b=%d rdata= %d nbyte= %d rsize= %d readCounter= %d",
            checkBatchEnabled(), *((short *) rdata), nbyte, (int)rsize, (int)readCounter);
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

        if(checkValidHeader(data_format) == 0) {
            LOGE("HAL:input invalid data_format 0x%02X", data_format);
            return;
        }

        if (data_format & DATA_FORMAT_STEP) {
            if (data_format == DATA_FORMAT_STEP) {
                rdata += BYTES_PER_SENSOR;
                latestTimestamp = *((long long*) (rdata));
                LOGV_IF(ENG_VERBOSE && INPUT_DATA, "STEP DETECTED:0x%x - ts: %lld", data_format, latestTimestamp);
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

        if (data_format == DATA_FORMAT_MARKER) {
            LOGV_IF(ENG_VERBOSE && INPUT_DATA, "MARKER DETECTED:0x%x", data_format);
            readCounter -= BYTES_PER_SENSOR;
            if (!mFlushSensorEnabledVector.isEmpty()) {
                mFlushBatchSet = 1;
            }
            mDataMarkerDetected = 1;
            mSkipReadEvents = 1;
        }
        else if (data_format == DATA_FORMAT_EMPTY_MARKER) {
            LOGV_IF(ENG_VERBOSE && INPUT_DATA, "EMPTY MARKER DETECTED:0x%x", data_format);
            readCounter -= BYTES_PER_SENSOR;
            if (!mFlushSensorEnabledVector.isEmpty()) {
                mFlushBatchSet = 1;
            }
            mEmptyDataMarkerDetected = 1;
            mSkipReadEvents = 1;
        }
        else if (data_format == DATA_FORMAT_QUAT) {
            LOGV_IF(ENG_VERBOSE && INPUT_DATA, "QUAT DETECTED:0x%x", data_format);
            if (readCounter >= BYTES_QUAT_DATA) {
                mCachedQuaternionData[0] = *((int *) (rdata + 4));
                mCachedQuaternionData[1] = *((int *) (rdata + 8));
                mCachedQuaternionData[2] = *((int *) (rdata + 12));
                rdata += QUAT_ONLY_LAST_PACKET_OFFSET;
                mQuatSensorTimestamp = *((long long*) (rdata));
                mask |= DATA_FORMAT_QUAT;
                readCounter -= BYTES_QUAT_DATA;
            }
            else {
                doneFlag = 1;
            }
        }
        else if (data_format == DATA_FORMAT_6_AXIS) {
            LOGV_IF(ENG_VERBOSE && INPUT_DATA, "6AXIS DETECTED:0x%x", data_format);
            if (readCounter >= BYTES_QUAT_DATA) {
                mCached6AxisQuaternionData[0] = *((int *) (rdata + 4));
                mCached6AxisQuaternionData[1] = *((int *) (rdata + 8));
                mCached6AxisQuaternionData[2] = *((int *) (rdata + 12));
                rdata += QUAT_ONLY_LAST_PACKET_OFFSET;
                mQuatSensorTimestamp = *((long long*) (rdata));
                mask |= DATA_FORMAT_6_AXIS;
                readCounter -= BYTES_QUAT_DATA;
            }
            else {
                doneFlag = 1;
            }
        }
        else if (data_format == DATA_FORMAT_PED_QUAT) {
            LOGV_IF(ENG_VERBOSE && INPUT_DATA, "PED QUAT DETECTED:0x%x", data_format);
            if (readCounter >= BYTES_PER_SENSOR_PACKET) {
                mCachedPedQuaternionData[0] = *((short *) (rdata + 2));
                mCachedPedQuaternionData[1] = *((short *) (rdata + 4));
                mCachedPedQuaternionData[2] = *((short *) (rdata + 6));
                rdata += BYTES_PER_SENSOR;
                mQuatSensorTimestamp = *((long long*) (rdata));
                mask |= DATA_FORMAT_PED_QUAT;
                readCounter -= BYTES_PER_SENSOR_PACKET;
            }
            else {
                doneFlag = 1;
            }
        }
        else if (data_format == DATA_FORMAT_PED_STANDALONE) {
            LOGV_IF(ENG_VERBOSE && INPUT_DATA, "STANDALONE STEP DETECTED:0x%x", data_format);
            if (readCounter >= BYTES_PER_SENSOR_PACKET) {
                rdata += BYTES_PER_SENSOR;
                mStepSensorTimestamp = *((long long*) (rdata));
                mask |= DATA_FORMAT_PED_STANDALONE;
                readCounter -= BYTES_PER_SENSOR_PACKET;
                mPedUpdate |= data_format;
            }
            else {
                doneFlag = 1;
            }
        }
        else if (data_format == DATA_FORMAT_GYRO) {
            LOGV_IF(ENG_VERBOSE && INPUT_DATA, "GYRO DETECTED:0x%x", data_format);
            if (readCounter >= BYTES_PER_SENSOR_PACKET) {
                mCachedGyroData[0] = *((short *) (rdata + 2));
                mCachedGyroData[1] = *((short *) (rdata + 4));
                mCachedGyroData[2] = *((short *) (rdata + 6));
                rdata += BYTES_PER_SENSOR;
                mGyroSensorTimestamp = *((long long*) (rdata));
                mask |= DATA_FORMAT_GYRO;
                readCounter -= BYTES_PER_SENSOR_PACKET;
            } else {
                doneFlag = 1;
            }
        }
        else if (data_format == DATA_FORMAT_ACCEL) {
            LOGV_IF(ENG_VERBOSE && INPUT_DATA, "ACCEL DETECTED:0x%x", data_format);
            if (readCounter >= BYTES_PER_SENSOR_PACKET) {
                mCachedAccelData[0] = *((short *) (rdata + 2));
                mCachedAccelData[1] = *((short *) (rdata + 4));
                mCachedAccelData[2] = *((short *) (rdata + 6));
                rdata += BYTES_PER_SENSOR;
                mAccelSensorTimestamp = *((long long*) (rdata));
                mask |= DATA_FORMAT_ACCEL;
                readCounter -= BYTES_PER_SENSOR_PACKET;
            }
            else {
                doneFlag = 1;
            }
        }
        else if (data_format == DATA_FORMAT_COMPASS) {
            LOGV_IF(ENG_VERBOSE && INPUT_DATA, "COMPASS DETECTED:0x%x", data_format);
            if (readCounter >= BYTES_PER_SENSOR_PACKET) {
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
            else {
                doneFlag = 1;
            }
        }
        else if (data_format == DATA_FORMAT_COMPASS_OF) {
            LOGV_IF(ENG_VERBOSE && INPUT_DATA, "COMPASS OF DETECTED:0x%x", data_format);            
            mask |= DATA_FORMAT_COMPASS_OF;
            mCompassOverFlow = 1;
#ifdef INV_PLAYBACK_DBG
            if (readCounter >= BYTES_PER_SENSOR_PACKET) {
                if (mCompassSensor->isIntegrated()) {
                    mCachedCompassData[0] = *((short *) (rdata + 2));
                    mCachedCompassData[1] = *((short *) (rdata + 4));
                    mCachedCompassData[2] = *((short *) (rdata + 6));
                    rdata += BYTES_PER_SENSOR;
                    mCompassTimestamp = *((long long*) (rdata));
                    readCounter -= BYTES_PER_SENSOR_PACKET;
                }
            }
#else
            readCounter -= BYTES_PER_SENSOR;
#endif
        }
#ifdef ENABLE_PRESSURE
        else if (data_format == DATA_FORMAT_PRESSURE) {
            LOGV_IF(ENG_VERBOSE && INPUT_DATA, "PRESSURE DETECTED:0x%x", data_format);
            if (readCounter >= BYTES_QUAT_DATA) {
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
            } else{
                doneFlag = 1;
            }
        }
#endif

        if(doneFlag == 0) {
            rdata += BYTES_PER_SENSOR;
            LOGV_IF(ENG_VERBOSE && INPUT_DATA, "HAL: input data doneFlag is zero, readCounter=%d", (int)readCounter);
        }
        else {
            LOGV_IF(ENG_VERBOSE && INPUT_DATA, "HAL: input data doneFlag is set, readCounter=%d", (int)readCounter);
        }

        /* read ahead and store left over data if any */
        if (readCounter != 0) {
            int currentBufferCounter = 0;
            LOGV_IF(0, "Not enough data readCounter=%d, expected nbyte=%d, rsize=%d", (int)readCounter, nbyte, (int)rsize);
            memset(mLeftOverBuffer, 0, sizeof(mLeftOverBuffer));
            /* check for end markers, don't save */
            data_format = *((short *) (rdata));
            if ((data_format == DATA_FORMAT_MARKER) || (data_format == DATA_FORMAT_EMPTY_MARKER)) {
				LOGV_IF(ENG_VERBOSE && INPUT_DATA, "s MARKER DETECTED:0x%x", data_format);
				rdata += BYTES_PER_SENSOR;
				readCounter -= BYTES_PER_SENSOR;
				if (!mFlushSensorEnabledVector.isEmpty()) {
					mFlushBatchSet = 1;
				}
				mDataMarkerDetected = 1;
				mSkipReadEvents = 1;
				if (readCounter == 0) {
					mLeftOverBufferSize = 0;
					if(doneFlag != 0) {
						return;
					}
				}
			}
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
            LOGV_IF(0, "Stored number of bytes:%d", mLeftOverBufferSize);
        } else {
            /* reset count since this is the last packet for the data set */
            readCounter = 0;
            mLeftOverBufferSize = 0;
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
            // cancels step bit
            mask &= (~DATA_FORMAT_STEP);
        }

        /* handle data read */
        if (mask == DATA_FORMAT_GYRO) {
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

            inv_build_gyro(mCachedGyroData, mGyroSensorTimestamp);
            LOGV_IF(INPUT_DATA,
                   "HAL:input inv_build_gyro: %+8d %+8d %+8d - %lld",
                    mCachedGyroData[0], mCachedGyroData[1],
                    mCachedGyroData[2], mGyroSensorTimestamp);
            latestTimestamp = mGyroSensorTimestamp;
        }

        if (mask == DATA_FORMAT_ACCEL) {
            mPendingMask |= 1 << Accelerometer;
            inv_build_accel(mCachedAccelData, 0, mAccelSensorTimestamp);
            LOGV_IF(INPUT_DATA,
               "HAL:input inv_build_accel: %+8ld %+8ld %+8ld - %lld",
                mCachedAccelData[0], mCachedAccelData[1],
                mCachedAccelData[2], mAccelSensorTimestamp);
                /* remember inital 6 axis quaternion */
                inv_time_t tempTimestamp;
                inv_get_6axis_quaternion(mInitial6QuatValue, &tempTimestamp);
                if (mInitial6QuatValue[0] != 0 && mInitial6QuatValue[1] != 0 &&
                        mInitial6QuatValue[2] != 0 && mInitial6QuatValue[3] != 0) {
                    mInitial6QuatValueAvailable = 1;
                    LOGV_IF(INPUT_DATA && ENG_VERBOSE,
                        "HAL:input build 6q init: %+8ld %+8ld %+8ld %+8ld",
                        mInitial6QuatValue[0], mInitial6QuatValue[1],
                        mInitial6QuatValue[2], mInitial6QuatValue[3]);
                }
            latestTimestamp = mAccelSensorTimestamp;
        }
        
        if (mask  == DATA_FORMAT_COMPASS_OF) {
            /* compass overflow detected */
            /* reset compass algorithm */
            int status = 0;
            inv_build_compass(mCachedCompassData, status,
                              mCompassTimestamp);
            LOGV_IF(INPUT_DATA,
                    "HAL:input inv_build_compass_of: %+8ld %+8ld %+8ld - %lld",
                    mCachedCompassData[0], mCachedCompassData[1],
                    mCachedCompassData[2], mCompassTimestamp);
            resetCompass();
        }
        
        if ((mask == DATA_FORMAT_COMPASS) && mCompassSensor->isIntegrated()) {
            int status = 0;
            if (mCompassSensor->providesCalibration()) {
                status = mCompassSensor->getAccuracy();
                status |= INV_CALIBRATED;
            }
            inv_build_compass(mCachedCompassData, status,
                              mCompassTimestamp);
            LOGV_IF(INPUT_DATA,
                    "HAL:input inv_build_compass: %+8ld %+8ld %+8ld - %lld",
                    mCachedCompassData[0], mCachedCompassData[1],
                    mCachedCompassData[2], mCompassTimestamp);
            latestTimestamp = mCompassTimestamp;
        }

        if (mask == DATA_FORMAT_QUAT) {
            /* if bias was applied to DMP bias,
               set status bits to disable gyro bias cal */
            int status = 0;
            if (mGyroBiasApplied == true) {
                LOGV_IF(INPUT_DATA && ENG_VERBOSE, "HAL:input dmp bias is used");
                status |= INV_BIAS_APPLIED;
            }
            status |= INV_CALIBRATED | INV_QUAT_3AXIS | INV_QUAT_3ELEMENT; /* default 32 (16/32bits) */
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

        if (mask == DATA_FORMAT_6_AXIS) {
            /* if bias was applied to DMP bias,
               set status bits to disable gyro bias cal */
            int status = 0;
            if (mGyroBiasApplied == true) {
                LOGV_IF(INPUT_DATA && ENG_VERBOSE, "HAL:input dmp bias is used");
                status |= INV_QUAT_6AXIS;
            }
            status |= INV_CALIBRATED | INV_QUAT_6AXIS | INV_QUAT_3ELEMENT; /* default 32 (16/32bits) */
            inv_build_quat(mCached6AxisQuaternionData,
                       status,
                       mQuatSensorTimestamp);
            LOGV_IF(INPUT_DATA,
                    "HAL:input inv_build_quat-6x: %+8ld %+8ld %+8ld - %lld",
                    mCached6AxisQuaternionData[0], mCached6AxisQuaternionData[1],
                    mCached6AxisQuaternionData[2], mQuatSensorTimestamp);
            latestTimestamp = mQuatSensorTimestamp;
        }

        if (mask == DATA_FORMAT_PED_QUAT) {
            /* if bias was applied to DMP bias,
               set status bits to disable gyro bias cal */
            int status = 0;
            if (mGyroBiasApplied == true) {
                LOGV_IF(INPUT_DATA && ENG_VERBOSE,
                        "HAL:input dmp bias is used");
                status |= INV_QUAT_6AXIS;
            }
            status |= INV_CALIBRATED | INV_QUAT_6AXIS | INV_QUAT_3ELEMENT; /* default 32 (16/32bits) */
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

#ifdef ENABLE_PRESSURE
        if ((mask ==DATA_FORMAT_PRESSURE) && mPressureSensor->isIntegrated()) {
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
#endif
   }    //while end
}

int MPLSensor::checkValidHeader(unsigned short data_format)
{
    LOGV_IF(ENG_VERBOSE && INPUT_DATA, "check data_format=%x", data_format);

    if(data_format == DATA_FORMAT_STEP)
        return 1;

    if(data_format & DATA_FORMAT_STEP) {
        data_format &= (~DATA_FORMAT_STEP);
    }

    if ((data_format == DATA_FORMAT_PED_STANDALONE) ||
        (data_format == DATA_FORMAT_PED_QUAT) ||
        (data_format == DATA_FORMAT_6_AXIS) ||
        (data_format == DATA_FORMAT_QUAT) ||
        (data_format == DATA_FORMAT_COMPASS) ||
        (data_format == DATA_FORMAT_GYRO) ||
        (data_format == DATA_FORMAT_ACCEL) ||
        (data_format == DATA_FORMAT_PRESSURE) ||
        (data_format == DATA_FORMAT_EMPTY_MARKER) ||
        (data_format == DATA_FORMAT_MARKER) ||
        (data_format == DATA_FORMAT_COMPASS_OF))
            return 1;
     else {
        LOGV_IF(ENG_VERBOSE, "bad data_format = %x", data_format);
        return 0;
    }
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
            mSkipReadEvents = 0;
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
       inv_init_magnetic_disturbance();
       inv_vector_compass_cal_sensitivity(3);
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
        // Enable DMP orientation
        LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                en, mpu.display_orientation_on, getTimestamp());
        if (write_sysfs_int(mpu.display_orientation_on, en) < 0) {
            LOGE("HAL:ERR can't enable Android orientation");
            res = -1;	// indicate an err
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

        if (mFeatureActiveMask == 0) {
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

    if ((res = computeAndSetDmpState()) < 0)
        return res;

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
        setQuaternionRate(*wanted);
        if (mFeatureActiveMask & INV_DMP_BATCH_MODE) {
            set6AxisQuaternionRate(*wanted);
            setPedQuaternionRate(*wanted);
        }
        // DMP running rate must be @ 200Hz
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
    /* clamped to 1ms? as spec, still rather large */
    return 100;
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
            LOGV_IF(0,
                    "Step Count interval elapsed: %lld, triggered: %lld",
                    interval, mStepCountPollTime);
            return false;
        } else {
            clock_gettime(CLOCK_MONOTONIC, &mt_pre);
            LOGV_IF(0, "Step Count previous time: %ld ms",
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

    LOGV_IF(ENG_VERBOSE && INPUT_DATA,
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
        (int)((sizeof(sBaseSensorList) / sizeof(sensor_t)) * sizeof(sensor_t))) {
        LOGE("HAL:sensor list too small, not populating.");
        return -(sizeof(sBaseSensorList) / sizeof(sensor_t));
    }

    /* fill in the base values */
    memcpy(list, sBaseSensorList,
           sizeof (struct sensor_t) * (sizeof(sBaseSensorList) / sizeof(sensor_t)));

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
#ifdef ENABLE_PRESSURE
    if (mPressureSensor != NULL) {
        mPressureSensor->fillList(&list[Pressure]);
    }
#endif

    if(1) {
        numsensors = (sizeof(sBaseSensorList) / sizeof(sensor_t));
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
         "params default to mpu6515 and might be wrong.",
         accel);
    list[Accelerometer].maxRange = ACCEL_MPU6500_RANGE;
    list[Accelerometer].resolution = ACCEL_MPU6500_RESOLUTION;
    list[Accelerometer].power = ACCEL_MPU6500_POWER;
    list[Accelerometer].minDelay = ACCEL_MPU6500_MINDELAY;
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
        LOGE("HAL:default to use mpu6515 params");
        list[Gyro].maxRange = GYRO_MPU6500_RANGE;
        list[Gyro].resolution = GYRO_MPU6500_RESOLUTION;
        list[Gyro].power = GYRO_MPU6500_POWER;
        list[Gyro].minDelay = GYRO_MPU6500_MINDELAY;
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

#ifndef THIRD_PARTY_ACCEL //MPU3050
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
    sprintf(mpu.pedometer_step_thresh, "%s%s", sysfs_path,
            "/pedometer_step_thresh");
    sprintf(mpu.pedometer_counter, "%s%s", sysfs_path,
            "/pedometer_counter");
    sprintf(mpu.motion_lpa_on, "%s%s", sysfs_path,
            "/motion_lpa_on");
    return 0;
}

//DMP support only for MPU6xxx/9xxx
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

    /* Get Values from MPL */
    inv_get_gyro_bias(mFactoryGyroBias);
    LOGV_IF(ENG_VERBOSE, "Factory Gyro Bias %ld %ld %ld", mFactoryGyroBias[0], mFactoryGyroBias[1], mFactoryGyroBias[2]);
    mFactoryGyroBiasAvailable = true;

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
    inv_convert_to_body(orient, mGyroChipBias, bias);
    LOGV_IF(ENG_VERBOSE && INPUT_DATA, "Mpl Gyro Bias (HW unit) %ld %ld %ld", mGyroChipBias[0], mGyroChipBias[1], mGyroChipBias[2]);
    LOGV_IF(ENG_VERBOSE && INPUT_DATA, "Mpl Gyro Bias (HW unit) (body) %ld %ld %ld", bias[0], bias[1], bias[2]);
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

void MPLSensor::setGyroZeroBias()
{
    VFUNC_LOG;

    /* Write to Driver */
    LOGV_IF(SYSFS_VERBOSE && INPUT_DATA, "HAL:sysfs:echo %d > %s (%lld)",
            0, mpu.in_gyro_x_dmp_bias, getTimestamp());
    if(write_attribute_sensor_continuous(gyro_x_dmp_bias_fd, 0) < 0) {
        LOGE("HAL:Error writing to gyro_x_dmp_bias");
        return;
    }
    LOGV_IF(SYSFS_VERBOSE && INPUT_DATA, "HAL:sysfs:echo %d > %s (%lld)",
            0, mpu.in_gyro_y_dmp_bias, getTimestamp());
    if(write_attribute_sensor_continuous(gyro_y_dmp_bias_fd, 0) < 0) {
        LOGE("HAL:Error writing to gyro_y_dmp_bias");
        return;
    }
    LOGV_IF(SYSFS_VERBOSE && INPUT_DATA, "HAL:sysfs:echo %d > %s (%lld)",
            0, mpu.in_gyro_z_dmp_bias, getTimestamp());
    if(write_attribute_sensor_continuous(gyro_z_dmp_bias_fd, 0) < 0) {
        LOGE("HAL:Error writing to gyro_z_dmp_bias");
        return;
    }
    LOGV_IF(EXTRA_VERBOSE, "HAL:Zero Gyro DMP Calibrated Bias Applied");

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
    LOGV_IF(SYSFS_VERBOSE && INPUT_DATA, "HAL:sysfs:echo %ld > %s (%lld)",
            bias[0], mpu.in_gyro_x_dmp_bias, getTimestamp());
    if(write_attribute_sensor_continuous(gyro_x_dmp_bias_fd, bias[0]) < 0) {
        LOGE("HAL:Error writing to gyro_x_dmp_bias");
        return;
    }
    LOGV_IF(SYSFS_VERBOSE && INPUT_DATA, "HAL:sysfs:echo %ld > %s (%lld)",
            bias[1], mpu.in_gyro_y_dmp_bias, getTimestamp());
    if(write_attribute_sensor_continuous(gyro_y_dmp_bias_fd, bias[1]) < 0) {
        LOGE("HAL:Error writing to gyro_y_dmp_bias");
        return;
    }
    LOGV_IF(SYSFS_VERBOSE && INPUT_DATA, "HAL:sysfs:echo %ld > %s (%lld)",
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

    /* write to driver */
    LOGV_IF(SYSFS_VERBOSE && INPUT_DATA, "HAL:sysfs:echo %ld > %s (%lld)",
            (long) (mAccelBias[0] / 65536.f / 2),
            mpu.in_accel_x_dmp_bias, getTimestamp());
    if(write_attribute_sensor_continuous(
            accel_x_dmp_bias_fd, (long)(mAccelBias[0] / 65536.f / 2)) < 0) {
        LOGE("HAL:Error writing to accel_x_dmp_bias");
        return;
    }
    LOGV_IF(SYSFS_VERBOSE && INPUT_DATA, "HAL:sysfs:echo %ld > %s (%lld)",
            (long)(mAccelBias[1] / 65536.f / 2),
            mpu.in_accel_y_dmp_bias, getTimestamp());
    if(write_attribute_sensor_continuous(
            accel_y_dmp_bias_fd, (long)(mAccelBias[1] / 65536.f / 2)) < 0) {
        LOGE("HAL:Error writing to accel_y_dmp_bias");
        return;
    }
    LOGV_IF(SYSFS_VERBOSE && INPUT_DATA, "HAL:sysfs:echo %ld > %s (%lld)",
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

    if(flags & SENSORS_BATCH_DRY_RUN) {
        dryRun = true;
        LOGI_IF(PROCESS_VERBOSE,
                "HAL:batch - dry run mode is set (%d)", SENSORS_BATCH_DRY_RUN);
    }

    /* check if we can support issuing interrupt before FIFO fills-up */
    /* in a given timeout.                                          */
    if (flags & SENSORS_BATCH_WAKE_UPON_FIFO_FULL) {
            LOGE("HAL: batch SENSORS_BATCH_WAKE_UPON_FIFO_FULL is not supported");
            return -EINVAL;
    }

    getHandle(handle, what, sname);
    if(uint32_t(what) >= NumSensors) {
        LOGE("HAL:batch sensors %d not found", what);
        return -EINVAL;
    }

    LOGV_IF(PROCESS_VERBOSE,
            "HAL:batch : %llu ns, (%.2f Hz)", period_ns, 1000000000.f / period_ns);

    // limit all rates to reasonable ones */
    if (period_ns < 5000000LL) {
        period_ns = 5000000LL;
    }

    switch (what) {
    case Gyro:
    case RawGyro:
    case Accelerometer:
#ifdef ENABLE_PRESSURE
    case Pressure:
#endif
    case GameRotationVector:
    case StepDetector:
        LOGV_IF(PROCESS_VERBOSE, "HAL: batch - select sensor (handle %d)", handle);
        break;
    case MagneticField:
    case RawMagneticField:
        if(timeout > 0 && !mCompassSensor->isIntegrated())
            return -EINVAL;
        else
            LOGV_IF(PROCESS_VERBOSE, "HAL: batch - select sensor (handle %d)", handle);
        break;
    default:
        if (timeout > 0) {
            LOGE("sensor (handle %d) is not supported in batch mode", handle);
            return -EINVAL;
        }
    }

    if(dryRun == true) {
        LOGI("HAL: batch Dry Run is complete");
        return 0;
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
            LOGV_IF(DEBUG_BATCHING && EXTRA_VERBOSE,
                    "HAL:batch - requested sensor=0x%01x, batch delay=%lld", mEnabled & (1 << i), ns);
            // take the min delay ==> max rate
            wanted = (ns < wanted) ? ns : wanted;
            if (i <= RawMagneticField) {
                nBytes += 8;
            }
#ifdef ENABLE_PRESSURE
            if (i == Pressure) {
                nBytes += 6;
            }
#endif
            if ((i == StepDetector) || (i == GameRotationVector)) {
                nBytes += 16;
            }
        }
    }

    /* starting from code below,  we will modify hardware */
    /* first edit global batch mode mask */

    if (!timeout) {
        mBatchEnabled &= ~(1 << what);
        mBatchDelays[what] = 1000000000L;
        mDelays[what] = period_ns;
        mBatchTimeouts[what] = 100000000000LL;
    } else {
        mBatchEnabled |= (1 << what);
        mBatchDelays[what] = period_ns;
        mDelays[what] = period_ns;
        mBatchTimeouts[what] = timeout;
    }

    // reset master enable
    res = masterEnable(0);
    if (res < 0) {
        return res;
    }

    if(((int)mOldBatchEnabledMask != batchMode) || batchMode) {

    /* remember batch mode that is set  */
    mOldBatchEnabledMask = batchMode;

    /* For these sensors, switch to different data output */
    int featureMask = computeBatchDataOutput();

    LOGV_IF(ENG_VERBOSE, "batchMode =%d, featureMask=0x%x, mEnabled=%d",
                batchMode, featureMask, mEnabled);
    if (DEBUG_BATCHING && EXTRA_VERBOSE) {
        LOGV("HAL:batch - sensor=0x%01x", mBatchEnabled);
        for (int d = 0; d < NumSensors; d++) {
            LOGV("HAL:batch - sensor status=0x%01x batch status=0x%01x timeout=%lld delay=%lld",
                            mEnabled & (1 << d), (mBatchEnabled & (1 << d)), mBatchTimeouts[d],
                            mBatchDelays[d]);
        }
    }

    /* case for Ped standalone */
    if ((batchMode == 1) && (featureMask & INV_DMP_PED_STANDALONE) &&
           (mFeatureActiveMask & INV_DMP_PEDOMETER)) {
        LOGI_IF(ENG_VERBOSE, "batch - ID_P only = 0x%x", mBatchEnabled);
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
        LOGI_IF(ENG_VERBOSE, "batch - ID_P and GRV or ALL = 0x%x", mBatchEnabled);
        LOGI_IF(ENG_VERBOSE, "batch - ID_P is enabled for batching, PED quat will be automatically enabled");
        enableLPQuaternion(0);
        enablePedQuaternion(1);

        /* set pedq rate */
        wanted = mBatchDelays[GameRotationVector];
        setPedQuaternionRate(wanted);
    } else if (!(featureMask & INV_DMP_PED_STANDALONE)){
        LOGV_IF(ENG_VERBOSE, "batch - PedQ Toggle back to normal 6 axis");
        if (mEnabled & (1 << GameRotationVector)) {
            enableLPQuaternion(checkLPQRateSupported());
        }
        enablePedQuaternion(0);
    } else {
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
        LOGI_IF(ENG_VERBOSE, "batch - GRV = 0x%x", mBatchEnabled);
        enableLPQuaternion(0);
        enable6AxisQuaternion(1);
        if (what == GameRotationVector) {
            setInitial6QuatValue();
        }

        /* set sixaxis rate */
        wanted = mBatchDelays[GameRotationVector];
        set6AxisQuaternionRate(wanted);
    } else if (!(featureMask & INV_DMP_PED_QUATERNION)){
        LOGV_IF(ENG_VERBOSE, "batch - 6Axis Toggle back to normal 6 axis");
        if (mEnabled & (1 << GameRotationVector)) {
            enableLPQuaternion(checkLPQRateSupported());
        }
        enable6AxisQuaternion(0);
    } else {
        enable6AxisQuaternion(0);
    }

    /* TODO: This may make a come back some day */
    /* Not to overflow hardware FIFO if flag is set */
    /*if (flags & (1 << SENSORS_BATCH_WAKE_UPON_FIFO_FULL)) {
        LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                0, mpu.batchmode_wake_fifo_full_on, getTimestamp());
        if (write_sysfs_int(mpu.batchmode_wake_fifo_full_on, 0) < 0) {
            LOGE("HAL:ERR can't write batchmode_wake_fifo_full_on");
        }
    }*/

    writeBatchTimeout(batchMode);

    if (computeAndSetDmpState() < 0) {
        LOGE("HAL:ERR can't compute dmp state");
    }

}//end of batch mode modify

    if (batchMode == 1) {
        /* set batch rates */
        if (setBatchDataRates() < 0) {
            LOGE("HAL:ERR can't set batch data rates");
        }
    } else {
        /* reset sensor rate */
        if (resetDataRates() < 0) {
            LOGE("HAL:ERR can't reset output rate back to original setting");
        }
    }

    // set sensor data interrupt
    uint32_t dataInterrupt = (mEnabled || (mFeatureActiveMask & INV_DMP_BATCH_MODE));
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                !dataInterrupt, mpu.dmp_event_int_on, getTimestamp());
    if (write_sysfs_int(mpu.dmp_event_int_on, !dataInterrupt) < 0) {
        res = -1;
        LOGE("HAL:ERR can't enable DMP event interrupt");
    }

    if (enabled_sensors || mFeatureActiveMask) {
        masterEnable(1);
    }
    return res;
}

/* Send empty event when:        */
/* 1. batch mode is not enabled  */
/* 2. no data in HW FIFO         */
/* return status zero if (2)     */
int MPLSensor::flush(int handle)
{
    VFUNC_LOG;

    int res = 0;
    int status = 0;
    android::String8 sname;
    int what = -1;

    getHandle(handle, what, sname);
    if (uint32_t(what) >= NumSensors) {
        LOGE("HAL:flush - what=%d is invalid", what);
        return -EINVAL;
    }

    LOGV_IF(PROCESS_VERBOSE, "HAL: flush - select sensor %s (handle %d)", sname.string(), handle);

    if (((what != StepDetector) && (!(mEnabled & (1 << what)))) ||
        ((what == StepDetector) && !(mFeatureActiveMask & INV_DMP_PEDOMETER))) {
        LOGV_IF(ENG_VERBOSE, "HAL: flush - sensor %s not enabled", sname.string());
         return -EINVAL;
    }

    if(!(mBatchEnabled & (1 << what))) {
         LOGV_IF(PROCESS_VERBOSE, "HAL:flush - batch mode not enabled for sensor %s (handle %d)", sname.string(), handle);
    }

    mFlushSensorEnabledVector.push_back(handle);

    /*write sysfs */
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:cat %s (%lld)",
            mpu.flush_batch, getTimestamp());

    status = read_sysfs_int(mpu.flush_batch, &res);

    if (status < 0)
        LOGE("HAL: flush - error invoking flush_batch");

    /* driver returns 0 if FIFO is empty */
    if (res == 0) {
        LOGI("HAL: flush - no data in FIFO");
    }

    LOGV_IF(ENG_VERBOSE, "HAl:flush - mFlushSensorEnabledVector=%d res=%d status=%d", handle, res, status);

    mFlushBatchSet = 0;
    return 0;
}

int MPLSensor::selectAndSetQuaternion(int batchMode, int mEnabled, long long featureMask)
{
    VFUNC_LOG;
    int res = 0;

    int64_t wanted;

    /* case for Ped Quaternion */
    if (batchMode == 1) {
        if ((featureMask & INV_DMP_PED_QUATERNION) &&
            (mEnabled & (1 << GameRotationVector)) &&
            (mFeatureActiveMask & INV_DMP_PEDOMETER)) {
                enableLPQuaternion(0);
                enable6AxisQuaternion(0);
                setInitial6QuatValue();
                enablePedQuaternion(1);

                /* set pedq rate */
                wanted = mBatchDelays[GameRotationVector];
                setPedQuaternionRate(wanted);
        } else if ((featureMask & INV_DMP_6AXIS_QUATERNION) &&
                   (mEnabled & (1 << GameRotationVector))) {
                enableLPQuaternion(0);
                enablePedQuaternion(0);
                setInitial6QuatValue();
                enable6AxisQuaternion(1);

                /* set sixaxis rate */
                wanted = mBatchDelays[GameRotationVector];
                set6AxisQuaternionRate(wanted);
        } else {
            enablePedQuaternion(0);
            enable6AxisQuaternion(0);
        }
    } else {
        if(mEnabled & (1 << GameRotationVector)) {
            enablePedQuaternion(0);
            enable6AxisQuaternion(0);
            enableLPQuaternion(checkLPQRateSupported());
        }
        else {
            enablePedQuaternion(0);
            enable6AxisQuaternion(0);
        }
    }

    return res;
}

/*
Select Quaternion and Options for Batching

    ID_P    ID_GRV     HW Batch     Type
a   1       1          1            PedQ, Ped Indicator, HW
b   1       1          0            PedQ
c   1       0          1            Ped Indicator, HW
d   1       0          0            Ped Standalone, Ped Indicator
e   0       1          1            6Q, HW
f   0       1          0            6Q
g   0       0          1            HW
h   0       0          0            LPQ <defualt>
*/
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
#ifdef ENABLE_PRESSURE
                                | (1 << Pressure)
#endif
                                | (1 << RawMagneticField);

    LOGV_IF(ENG_VERBOSE, "hardwareSensorMask = 0x%0x, mBatchEnabled = 0x%0x",
            hardwareSensorMask, mBatchEnabled);

    if (mBatchEnabled & (1 << StepDetector)) {
        if (mBatchEnabled & (1 << GameRotationVector)) {
            if ((mBatchEnabled & hardwareSensorMask)) {
                featureMask |= INV_DMP_6AXIS_QUATERNION;//a
                featureMask |= INV_DMP_PED_INDICATOR;
//LOGE("batch output: a");
            } else {
                featureMask |= INV_DMP_PED_QUATERNION;  //b
                featureMask |= INV_DMP_PED_INDICATOR;   //always piggy back a bit
//LOGE("batch output: b");
            }
        } else {
            if (mBatchEnabled & hardwareSensorMask) {
                featureMask |= INV_DMP_PED_INDICATOR;   //c
//LOGE("batch output: c");
            } else {
                featureMask |= INV_DMP_PED_STANDALONE;  //d
                featureMask |= INV_DMP_PED_INDICATOR;   //required for standalone
//LOGE("batch output: d");
            }
        }
    } else if (mBatchEnabled & (1 << GameRotationVector)) {
        featureMask |= INV_DMP_6AXIS_QUATERNION;        //e,f
//LOGE("batch output: e,f");
    } else {
        LOGV_IF(ENG_VERBOSE,
                "HAL:computeBatchDataOutput: featuerMask=0x%x", featureMask);
//LOGE("batch output: g");
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

/* @param [in] : outputType = 1 --event is from PED_Q        */
/*               outputType = 0 --event is from ID_SC, ID_P  */
int MPLSensor::readDmpPedometerEvents(sensors_event_t* data, int count,
                                      int32_t id, int outputType)
{
    VFUNC_LOG;

    int res = 0;
    char dummy[4];

    int numEventReceived = 0;
    int update = 0;

    LOGI_IF(0, "HAL: Read Pedometer Event ID=%d", id);
    switch (id) {
    case ID_P:
        if (mDmpPedometerEnabled && count > 0) {
            /* Handles return event */
            LOGI("HAL: Step detected");
            update = sdHandler(&mSdEvents);
        }

        if (update && count > 0) {
            *data++ = mSdEvents;
            count--;
            numEventReceived++;
        }
        break;
    case ID_SC:
        FILE *fp;
        uint64_t stepCount;
        uint64_t stepCountTs;

        if (mDmpStepCountEnabled && count > 0) {
            fp = fopen(mpu.pedometer_steps, "r");
            if (fp == NULL) {
                LOGE("HAL:cannot open pedometer_steps");
            } else {
                if (fscanf(fp, "%lld\n", &stepCount) < 0) {
                    LOGW("HAL:cannot read pedometer_steps");
                    if (fclose(fp) < 0) {
                       LOGW("HAL:cannot close pedometer_steps");
                    }
                    return 0;
                }
                if (fclose(fp) < 0) {
                       LOGW("HAL:cannot close pedometer_steps");
                }
            }

            /* return event onChange only */
            if (stepCount == mLastStepCount) {
                return 0;
            }

            mLastStepCount = stepCount;

            /* Read step count timestamp */
            fp = fopen(mpu.pedometer_counter, "r");
            if (fp == NULL) {
                LOGE("HAL:cannot open pedometer_counter");
            } else{
                if (fscanf(fp, "%lld\n", &stepCountTs) < 0) {
                    LOGE("HAL:cannot read pedometer_counter");
                    if (fclose(fp) < 0) {
                        LOGE("HAL:cannot close pedometer_counter");
                    }
                    return 0;
                }
                if (fclose(fp) < 0) {
                        LOGE("HAL:cannot close pedometer_counter");
                        return 0;
                }
            }
            mScEvents.timestamp = stepCountTs;

            /* Handles return event */
            update = scHandler(&mScEvents);
        }

        if (update && count > 0) {
            *data++ = mScEvents;
            count--;
            numEventReceived++;
        }
        break;
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
        if (fscanf(fp, "%d\n", &significantMotion) < 0) {
            LOGE("HAL:cannot read event_smd");
        }
        if (fclose(fp) < 0) {
            LOGE("HAL:cannot close event_smd");
        }
    }

    if(mDmpSignificantMotionEnabled && count > 0) {
       /* By implementation, smd is disabled once an event is triggered */
        sensors_event_t temp;

        /* Handles return event */
        LOGI("HAL: SMD detected");
        int update = smHandler(&mSmEvents);
        if (update && count > 0) {
            *data++ = mSmEvents;
            count--;
            numEventReceived++;

            /* reset smd state */
            mDmpSignificantMotionEnabled = 0;
            mFeatureActiveMask &= ~INV_DMP_SIGNIFICANT_MOTION;

            /* auto disable this sensor */
            enableDmpSignificantMotion(0);
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
    }

    if ((res = setDmpFeature(en)) < 0)
        return res;

    if ((res = computeAndSetDmpState()) < 0)
        return res;

    if (!mBatchEnabled && (resetDataRates() < 0))
        return res;

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
    LOGV_IF(EXTRA_VERBOSE, "HAL:sysfs:echo quat value > %s", mpu.six_axis_q_value);
    LOGV_IF(EXTRA_VERBOSE && ENG_VERBOSE, "quat=%ld,%ld,%ld,%ld", mInitial6QuatValue[0],
					            mInitial6QuatValue[1],
                                                    mInitial6QuatValue[2],
                                                    mInitial6QuatValue[3]);
    FILE* fptr = fopen(mpu.six_axis_q_value, "w");
    if(fptr == NULL) {
        LOGE("HAL:could not open six_axis_q_value");
    } else {
        if (fwrite(quat, 1, length, fptr) != length) {
           LOGE("HAL:write six axis q value failed");
        } else {
            mInitial6QuatValueAvailable = 0;
        }
        if (fclose(fptr) < 0) {
            LOGE("HAL:could not close six_axis_q_value");
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
#ifdef ENABLE_PRESSURE
    int64_t pressureRate;
#endif
    int64_t quatRate;

    int mplGyroRate;
    int mplAccelRate;
    int mplCompassRate;
    int mplQuatRate;

#ifdef ENABLE_MULTI_RATE
    gyroRate = mBatchDelays[Gyro];
    /* take care of case where only one type of gyro sensors or
       compass sensors is turned on */
    if (mBatchEnabled & (1 << Gyro) || mBatchEnabled & (1 << RawGyro)) {
        gyroRate = (mBatchDelays[Gyro] <= mBatchDelays[RawGyro]) ?
            (mBatchEnabled & (1 << Gyro) ? mBatchDelays[Gyro] : mBatchDelays[RawGyro]):
            (mBatchEnabled & (1 << RawGyro) ? mBatchDelays[RawGyro] : mBatchDelays[Gyro]);
    }
    compassRate = mBatchDelays[MagneticField];
    if (mBatchEnabled & (1 << MagneticField) || mBatchEnabled & (1 << RawMagneticField)) {
        compassRate = (mBatchDelays[MagneticField] <= mBatchDelays[RawMagneticField]) ?
                (mBatchEnabled & (1 << MagneticField) ? mBatchDelays[MagneticField] :
                    mBatchDelays[RawMagneticField]) :
                (mBatchEnabled & (1 << RawMagneticField) ? mBatchDelays[RawMagneticField] :
                    mBatchDelays[MagneticField]);
    }
    accelRate = mBatchDelays[Accelerometer];
#ifdef ENABLE_PRESSURE
    pressureRate = mBatchDelays[Pressure];
#endif //ENABLE_PRESSURE

    if ((mFeatureActiveMask & INV_DMP_PED_QUATERNION) ||
            (mFeatureActiveMask & INV_DMP_6AXIS_QUATERNION)) {
        quatRate = mBatchDelays[GameRotationVector];
        mplQuatRate = (int) quatRate / 1000LL;
        inv_set_quat_sample_rate(mplQuatRate);
        inv_set_rotation_vector_6_axis_sample_rate(mplQuatRate);
        LOGV_IF(PROCESS_VERBOSE,
            "HAL:MPL rv 6 axis sample rate: (mpl)=%d us (mpu)=%.2f Hz", mplQuatRate,
                1000000000.f / quatRate );
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
#ifdef ENABLE_PRESSURE
    pressureRate = wanted;
#endif
#endif

    /* takes care of gyro rate */
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %.0f > %s (%lld)",
            1000000000.f / gyroRate, mpu.gyro_rate,
            getTimestamp());
    tempFd = open(mpu.gyro_rate, O_RDWR);
    res = write_attribute_sensor(tempFd, 1000000000.f / gyroRate);
    if(res < 0) {
        LOGE("HAL:GYRO update delay error");
    }

    /* takes care of accel rate */
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %.0f > %s (%lld)",
            1000000000.f / accelRate, mpu.accel_rate,
            getTimestamp());
    tempFd = open(mpu.accel_rate, O_RDWR);
    res = write_attribute_sensor(tempFd, 1000000000.f / accelRate);
    LOGE_IF(res < 0, "HAL:ACCEL update delay error");

    /* takes care of compass rate */
    if (compassRate < mCompassSensor->getMinDelay() * 1000LL) {
        compassRate = mCompassSensor->getMinDelay() * 1000LL;
    }
    mCompassSensor->setDelay(ID_M, compassRate);

#ifdef ENABLE_PRESSURE
    /* takes care of pressure rate */
    mPressureSensor->setDelay(ID_PS, pressureRate);
#endif

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

    int64_t resetRate;
    int64_t gyroRate;
    int64_t accelRate;
    int64_t compassRate;
#ifdef ENABLE_PRESSURE
    int64_t pressureRate;
#endif

    if (!mEnabled) {
        LOGV_IF(ENG_VERBOSE, "skip resetDataRates");
        return 0;
    }
    LOGI("HAL:resetDataRates mEnabled=%d", mEnabled);
    /* search the minimum delay requested across all enabled sensors */
    /* skip setting rates if it is not changed */
    for (int i = 0; i < NumSensors; i++) {
        if (mEnabled & (1 << i)) {
            int64_t ns = mDelays[i];
#ifdef ENABLE_PRESSURE
            if ((wanted == ns) && (i != Pressure)) {
                LOGV_IF(ENG_VERBOSE, "skip resetDataRates : same delay mDelays[%d]=%lld", i,mDelays[i]);
                //return 0;
            }
#endif
            LOGV_IF(ENG_VERBOSE, "resetDataRates - mDelays[%d]=%lld", i, mDelays[i]);
            wanted = wanted < ns ? wanted : ns;
        }
    }

    resetRate = wanted;
    gyroRate = wanted;
    accelRate = wanted;
    compassRate = wanted;
#ifdef ENABLE_PRESSURE
    pressureRate = wanted;
#endif

    /* set mpl data rate */
   inv_set_gyro_sample_rate((int)gyroRate/1000LL);
   inv_set_accel_sample_rate((int)accelRate/1000LL);
   inv_set_compass_sample_rate((int)compassRate/1000LL);
   inv_set_linear_acceleration_sample_rate((int)resetRate/1000LL);
   inv_set_orientation_sample_rate((int)resetRate/1000LL);
   inv_set_rotation_vector_sample_rate((int)resetRate/1000LL);
   inv_set_gravity_sample_rate((int)resetRate/1000LL);
   inv_set_orientation_geomagnetic_sample_rate((int)resetRate/1000LL);
   inv_set_rotation_vector_6_axis_sample_rate((int)resetRate/1000LL);
   inv_set_geomagnetic_rotation_vector_sample_rate((int)resetRate/1000LL);

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

    /* takes care of gyro rate */
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %.0f > %s (%lld)",
            1000000000.f / gyroRate, mpu.gyro_rate,
            getTimestamp());
    tempFd = open(mpu.gyro_rate, O_RDWR);
    res = write_attribute_sensor(tempFd, 1000000000.f / gyroRate);
    if(res < 0) {
        LOGE("HAL:GYRO update delay error");
    }

    /* takes care of accel rate */
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %.0f > %s (%lld)",
            1000000000.f / accelRate, mpu.accel_rate,
            getTimestamp());
    tempFd = open(mpu.accel_rate, O_RDWR);
    res = write_attribute_sensor(tempFd, 1000000000.f / accelRate);
    LOGE_IF(res < 0, "HAL:ACCEL update delay error");

    /* takes care of compass rate */
    if (compassRate < mCompassSensor->getMinDelay() * 1000LL) {
        compassRate = mCompassSensor->getMinDelay() * 1000LL;
    }
    mCompassSensor->setDelay(ID_M, compassRate);

#ifdef ENABLE_PRESSURE
    /* takes care of pressure rate */
    mPressureSensor->setDelay(ID_PS, pressureRate);
#endif

    /* takes care of lpq case for data rate at 200Hz */
    if (checkLPQuaternion()) {
        if (resetRate <= RATE_200HZ) {
#ifndef USE_LPQ_AT_FASTEST
            enableLPQuaternion(0);
#endif
        }
    }

    return res;
}

void MPLSensor::resetMplStates()
{
    VFUNC_LOG;
    LOGV_IF(ENG_VERBOSE, "HAL:resetMplStates()");

    inv_gyro_was_turned_off();
    inv_accel_was_turned_off();
    inv_compass_was_turned_off();
    inv_quaternion_sensor_was_turned_off();

    return;
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
