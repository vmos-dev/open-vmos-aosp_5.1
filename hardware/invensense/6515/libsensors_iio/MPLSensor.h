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

#ifndef ANDROID_MPL_SENSOR_H
#define ANDROID_MPL_SENSOR_H

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <poll.h>
#include <time.h>
#include <utils/Vector.h>
#include <utils/KeyedVector.h>
#include <utils/String8.h>
#include "sensors.h"
#include "SensorBase.h"
#include "InputEventReader.h"

#ifndef INVENSENSE_COMPASS_CAL
#pragma message("unified HAL for AKM")
#include "CompassSensor.AKM.h"
#endif

#ifdef SENSOR_ON_PRIMARY_BUS
#pragma message("Sensor on Primary Bus")
#include "CompassSensor.IIO.primary.h"
#else
#pragma message("Sensor on Secondary Bus")
#include "CompassSensor.IIO.9150.h"
#endif

class PressureSensor;

/*****************************************************************************/
/* Sensors Enable/Disable Mask
 *****************************************************************************/
#define MAX_CHIP_ID_LEN             (20)

#define INV_THREE_AXIS_GYRO         (0x000F)
#define INV_THREE_AXIS_ACCEL        (0x0070)
#define INV_THREE_AXIS_COMPASS      (0x0380)
#define INV_ONE_AXIS_PRESSURE       (0x0400)
#define INV_ALL_SENSORS             (0x7FFF)

#ifdef INVENSENSE_COMPASS_CAL
#define ALL_MPL_SENSORS_NP          (INV_THREE_AXIS_ACCEL \
                                      | INV_THREE_AXIS_COMPASS \
                                      | INV_THREE_AXIS_GYRO)
#else
#define ALL_MPL_SENSORS_NP          (INV_THREE_AXIS_ACCEL \
                                      | INV_THREE_AXIS_COMPASS \
                                      | INV_THREE_AXIS_GYRO)
#endif

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

// bit mask of current MPL active features (mMplFeatureActiveMask)
#define INV_COMPASS_CAL              0x01
#define INV_COMPASS_FIT              0x02

// bit mask of current DMP active features (mFeatureActiveMask)
#define INV_DMP_QUATERNION           0x001 //3 elements without real part, 32 bit each
#define INV_DMP_DISPL_ORIENTATION    0x002 //screen orientation
#define INV_DMP_SIGNIFICANT_MOTION   0x004 //significant motion
#define INV_DMP_PEDOMETER            0x008 //interrupt-based pedometer
#define INV_DMP_PEDOMETER_STEP       0x010 //timer-based pedometer
#define INV_DMP_PED_STANDALONE       0x020 //timestamps only
#define INV_DMP_6AXIS_QUATERNION     0x040 //3 elements without real part, 32 bit each
#define INV_DMP_PED_QUATERNION       0x080 //3 elements without real part, 16 bit each
#define INV_DMP_PED_INDICATOR        0x100 //tag along header with step indciator
#define INV_DMP_BATCH_MODE           0x200 //batch mode

// bit mask of whether DMP should be turned on
#define DMP_FEATURE_MASK (                           \
        (INV_DMP_QUATERNION)                         \
        | (INV_DMP_DISPL_ORIENTATION)                \
        | (INV_DMP_SIGNIFICANT_MOTION)               \
        | (INV_DMP_PEDOMETER)                        \
        | (INV_DMP_PEDOMETER_STEP)                   \
        | (INV_DMP_6AXIS_QUATERNION)                 \
        | (INV_DMP_PED_QUATERNION)                   \
        | (INV_DMP_BATCH_MODE)                       \
)

// bit mask of DMP features as sensors
#define DMP_SENSOR_MASK (                            \
        (INV_DMP_DISPL_ORIENTATION)                  \
        | (INV_DMP_SIGNIFICANT_MOTION)               \
        | (INV_DMP_PEDOMETER)                        \
        | (INV_DMP_PEDOMETER_STEP)                   \
        | (INV_DMP_6AXIS_QUATERNION)                 \
)

// data header format used by kernel driver.
#define DATA_FORMAT_STEP           0x0001
#define DATA_FORMAT_MARKER         0x0010
#define DATA_FORMAT_EMPTY_MARKER   0x0020
#define DATA_FORMAT_PED_STANDALONE 0x0100
#define DATA_FORMAT_PED_QUAT       0x0200
#define DATA_FORMAT_6_AXIS         0x0400
#define DATA_FORMAT_QUAT           0x0800
#define DATA_FORMAT_COMPASS        0x1000
#define DATA_FORMAT_COMPASS_OF     0x1800
#define DATA_FORMAT_GYRO           0x2000
#define DATA_FORMAT_ACCEL          0x4000
#define DATA_FORMAT_PRESSURE       0x8000
#define DATA_FORMAT_MASK           0xffff

#define BYTES_PER_SENSOR                8
#define BYTES_PER_SENSOR_PACKET         16
#define QUAT_ONLY_LAST_PACKET_OFFSET    16
#define BYTES_QUAT_DATA                 24
#define MAX_READ_SIZE                   BYTES_QUAT_DATA
#define MAX_SUSPEND_BATCH_PACKET_SIZE   1024
#define MAX_PACKET_SIZE                 80 //8 * 4 + (2 * 24)

/* Uncomment to enable Low Power Quaternion */
#define ENABLE_LP_QUAT_FEAT

/* Enable Pressure sensor support */
#undef ENABLE_PRESSURE

/* Screen Orientation is not currently supported */
int isDmpScreenAutoRotationEnabled()
{
#ifdef ENABLE_DMP_SCREEN_AUTO_ROTATION
    return 1;
#else
    return 0;
#endif
}

int (*m_pt2AccelCalLoadFunc)(long *bias) = NULL;
/*****************************************************************************/
/** MPLSensor implementation which fits into the HAL example for crespo provided
 *  by Google.
 *  WARNING: there may only be one instance of MPLSensor, ever.
 */

class MPLSensor: public SensorBase
{
    typedef int (MPLSensor::*hfunc_t)(sensors_event_t*);

public:

    MPLSensor(CompassSensor *, int (*m_pt2AccelCalLoadFunc)(long*) = 0);
    virtual ~MPLSensor();

    virtual int setDelay(int32_t handle, int64_t ns);
    virtual int enable(int32_t handle, int enabled);
    virtual int batch(int handle, int flags, int64_t period_ns, int64_t timeout);
    virtual int flush(int handle);
    int selectAndSetQuaternion(int batchMode, int mEnabled, long long featureMask);
    int checkBatchEnabled();
    int setBatch(int en, int toggleEnable);
    int writeBatchTimeout(int en);
    int32_t getEnableMask() { return mEnabled; }
    void getHandle(int32_t handle, int &what, android::String8 &sname);

    virtual int readEvents(sensors_event_t *data, int count);
    virtual int getFd() const;
    virtual int getAccelFd() const;
    virtual int getCompassFd() const;
    virtual int getPollTime();
    virtual int getStepCountPollTime();
    virtual bool hasPendingEvents() const;
    virtual bool hasStepCountPendingEvents();
    int populateSensorList(struct sensor_t *list, int len);

    void buildCompassEvent();
    void buildMpuEvent();
    int checkValidHeader(unsigned short data_format);

    int turnOffAccelFifo();
    int turnOffGyroFifo();
    int enableDmpOrientation(int);
    int dmpOrientHandler(int);
    int readDmpOrientEvents(sensors_event_t* data, int count);
    int getDmpOrientFd();
    int openDmpOrientFd();
    int closeDmpOrientFd();

    int getDmpRate(int64_t *);
    int checkDMPOrientation();

    int getDmpSignificantMotionFd();
    int readDmpSignificantMotionEvents(sensors_event_t* data, int count);
    int enableDmpSignificantMotion(int);
    int significantMotionHandler(sensors_event_t* data);
    bool checkSmdSupport(){return (mDmpSignificantMotionEnabled);};

    int enableDmpPedometer(int, int);
    int readDmpPedometerEvents(sensors_event_t* data, int count, int32_t id, int outputType);
    int getDmpPedometerFd();
    bool checkPedometerSupport() {return (mDmpPedometerEnabled || mDmpStepCountEnabled);};
    bool checkOrientationSupport() {return ((isDmpDisplayOrientationOn()
                                       && (mDmpOrientationEnabled
                                       || !isDmpScreenAutoRotationEnabled())));};

protected:
    CompassSensor *mCompassSensor;
    PressureSensor *mPressureSensor;

    int gyroHandler(sensors_event_t *data);
    int rawGyroHandler(sensors_event_t *data);
    int accelHandler(sensors_event_t *data);
    int compassHandler(sensors_event_t *data);
    int rawCompassHandler(sensors_event_t *data);
    int rvHandler(sensors_event_t *data);
    int grvHandler(sensors_event_t *data);
    int laHandler(sensors_event_t *data);
    int gravHandler(sensors_event_t *data);
    int orienHandler(sensors_event_t *data);
    int smHandler(sensors_event_t *data);
    int pHandler(sensors_event_t *data);
    int gmHandler(sensors_event_t *data);
    int psHandler(sensors_event_t *data);
    int sdHandler(sensors_event_t *data);
    int scHandler(sensors_event_t *data);
    int metaHandler(sensors_event_t *data, int flags);
    void calcOrientationSensor(float *Rx, float *Val);
    virtual int update_delay();

    void inv_set_device_properties();
    int inv_constructor_init();
    int inv_constructor_default_enable();
    int setAccelInitialState();
    int masterEnable(int en);
    int enablePedStandalone(int en);
    int enablePedStandaloneData(int en);
    int enablePedQuaternion(int);
    int enablePedQuaternionData(int);
    int setPedQuaternionRate(int64_t wanted);
    int enable6AxisQuaternion(int);
    int enable6AxisQuaternionData(int);
    int set6AxisQuaternionRate(int64_t wanted);
    int enableLPQuaternion(int);
    int enableQuaternionData(int);
    int setQuaternionRate(int64_t wanted);
    int enableAccelPedometer(int);
    int enableAccelPedData(int);
    int onDmp(int);
    int enableGyro(int en);
    int enableLowPowerAccel(int en);
    int enableAccel(int en);
    int enableCompass(int en, int rawSensorOn);
    int enablePressure(int en);
    int enableBatch(int64_t timeout);
    void computeLocalSensorMask(int enabled_sensors);
    int computeBatchSensorMask(int enableSensor, int checkNewBatchSensor);
    int computeBatchDataOutput();
    int enableSensors(unsigned long sensors, int en, uint32_t changed);
    int inv_read_temperature(long long *data);
    int inv_read_dmp_state(int fd);
    int inv_read_sensor_bias(int fd, long *data);
    void inv_get_sensors_orientation(void);
    int inv_init_sysfs_attributes(void);
    int resetCompass(void);
    void setCompassDelay(int64_t ns);
    void enable_iio_sysfs(void);
    int setDmpFeature(int en);
    int computeAndSetDmpState(void);
    int enablePedometer(int);
    int enablePedIndicator(int en);
    int checkPedStandaloneBatched(void);
    int checkPedStandaloneEnabled(void);
    int checkPedQuatEnabled();
    int check6AxisQuatEnabled();
    int checkLPQRateSupported();
    int checkLPQuaternion();
    int checkAccelPed();
    void setInitial6QuatValue();
    int writeSignificantMotionParams(bool toggleEnable,
                                     uint32_t delayThreshold1, uint32_t delayThreshold2,
                                     uint32_t motionThreshold);
    long mMasterSensorMask;
    long mLocalSensorMask;
    int mPollTime;
    int64_t mStepCountPollTime;
    bool mHaveGoodMpuCal;   // flag indicating that the cal file can be written
    int mGyroAccuracy;      // value indicating the quality of the gyro calibr.
    int mAccelAccuracy;     // value indicating the quality of the accel calibr.
    int mCompassAccuracy;   // value indicating the quality of the compass calibr.
    struct pollfd mPollFds[5];
    pthread_mutex_t mMplMutex;
    pthread_mutex_t mHALMutex;

    char mIIOBuffer[(16 + 8 * 3 + 8) * IIO_BUFFER_LENGTH];

    int iio_fd;
    int accel_fd;
    int mpufifo_fd;
    int gyro_temperature_fd;
    int accel_x_offset_fd;
    int accel_y_offset_fd;
    int accel_z_offset_fd;

    int accel_x_dmp_bias_fd;
    int accel_y_dmp_bias_fd;
    int accel_z_dmp_bias_fd;

    int gyro_x_offset_fd;
    int gyro_y_offset_fd;
    int gyro_z_offset_fd;

    int gyro_x_dmp_bias_fd;
    int gyro_y_dmp_bias_fd;
    int gyro_z_dmp_bias_fd;

    int dmp_orient_fd;
    int mDmpOrientationEnabled;

    int dmp_sign_motion_fd;
    int mDmpSignificantMotionEnabled;

    int dmp_pedometer_fd;
    int mDmpPedometerEnabled;
    int mDmpStepCountEnabled;

    uint32_t mEnabled;
    uint32_t mBatchEnabled;
    android::Vector<int> mFlushSensorEnabledVector;
    uint32_t mOldBatchEnabledMask;
    int64_t mBatchTimeoutInMs;
    sensors_event_t mPendingEvents[NumSensors];
    sensors_event_t mPendingFlushEvents[NumSensors];
    sensors_event_t mSmEvents;
    sensors_event_t mSdEvents;
    sensors_event_t mScEvents;
    int64_t mDelays[NumSensors];
    int64_t mBatchDelays[NumSensors];
    int64_t mBatchTimeouts[NumSensors];
    hfunc_t mHandlers[NumSensors];
    short mCachedGyroData[3];
    long mCachedAccelData[3];
    long mCachedCompassData[3];
    long mCachedQuaternionData[3];
    long mCached6AxisQuaternionData[3];
    long mCachedPedQuaternionData[3];
    long mCachedPressureData;
    android::KeyedVector<int, int> mIrqFds;

    InputEventCircularReader mAccelInputReader;
    InputEventCircularReader mGyroInputReader;

    int mCompassOverFlow;
    bool mFirstRead;
    short mTempScale;
    short mTempOffset;
    int64_t mTempCurrentTime;
    int mAccelScale;
    long mAccelSelfTestScale;
    long mGyroScale;
    long mGyroSelfTestScale;
    long mCompassScale;
    float mCompassBias[3];
    bool mFactoryGyroBiasAvailable;
    long mFactoryGyroBias[3];
    bool mGyroBiasAvailable;
    bool mGyroBiasApplied;
    float mGyroBias[3];    //in body frame
    long mGyroChipBias[3]; //in chip frame
    bool mFactoryAccelBiasAvailable;
    long mFactoryAccelBias[3];
    bool mAccelBiasAvailable;
    bool mAccelBiasApplied;
    long mAccelBias[3];    //in chip frame

    uint32_t mPendingMask;
    unsigned long mSensorMask;

    char chip_ID[MAX_CHIP_ID_LEN];
    char mSysfsPath[MAX_SYSFS_NAME_LEN];

    signed char mGyroOrientation[9];
    signed char mAccelOrientation[9];

    int64_t mSensorTimestamp;
    int64_t mCompassTimestamp;
    int64_t mPressureTimestamp;

    struct sysfs_attrbs {
       char *chip_enable;
       char *power_state;
       char *master_enable;
       char *dmp_firmware;
       char *firmware_loaded;
       char *dmp_on;
       char *dmp_int_on;
       char *dmp_event_int_on;
       char *tap_on;
       char *key;
       char *self_test;
       char *temperature;

       char *gyro_enable;
       char *gyro_fifo_rate;
       char *gyro_fsr;
       char *gyro_orient;
       char *gyro_fifo_enable;
       char *gyro_rate;

       char *accel_enable;
       char *accel_fifo_rate;
       char *accel_fsr;
       char *accel_bias;
       char *accel_orient;
       char *accel_fifo_enable;
       char *accel_rate;

       char *three_axis_q_on; //formerly quaternion_on
       char *three_axis_q_rate;

       char *six_axis_q_on;
       char *six_axis_q_rate;

       char *six_axis_q_value;

       char *ped_q_on;
       char *ped_q_rate;

       char *step_detector_on;
       char *step_indicator_on;

       char *in_timestamp_en;
       char *in_timestamp_index;
       char *in_timestamp_type;

       char *buffer_length;

       char *display_orientation_on;
       char *event_display_orientation;

       char *in_accel_x_offset;
       char *in_accel_y_offset;
       char *in_accel_z_offset;
       char *in_accel_self_test_scale;

       char *in_accel_x_dmp_bias;
       char *in_accel_y_dmp_bias;
       char *in_accel_z_dmp_bias;

       char *in_gyro_x_offset;
       char *in_gyro_y_offset;
       char *in_gyro_z_offset;
       char *in_gyro_self_test_scale;

       char *in_gyro_x_dmp_bias;
       char *in_gyro_y_dmp_bias;
       char *in_gyro_z_dmp_bias;

       char *event_smd;
       char *smd_enable;
       char *smd_delay_threshold;
       char *smd_delay_threshold2;
       char *smd_threshold;
       char *batchmode_timeout;
       char *batchmode_wake_fifo_full_on;
       char *flush_batch;

       char *pedometer_on;
       char *pedometer_int_on;
       char *event_pedometer;
       char *pedometer_steps;
       char *pedometer_step_thresh;
       char *pedometer_counter;
       
       char *motion_lpa_on;
    } mpu;

    char *sysfs_names_ptr;
    int mMplFeatureActiveMask;
    uint64_t mFeatureActiveMask;
    bool mDmpOn;
    int mPedUpdate;
    int mPressureUpdate;
    int64_t mQuatSensorTimestamp;
    int64_t mStepSensorTimestamp;
    uint64_t mLastStepCount;
    int mLeftOverBufferSize;
    char mLeftOverBuffer[1024];
    bool mInitial6QuatValueAvailable;
    long mInitial6QuatValue[4];
    bool mFlushBatchSet;
    uint32_t mSkipReadEvents;
    bool mDataMarkerDetected;
    bool mEmptyDataMarkerDetected;

private:
    /* added for dynamic get sensor list */
    void fillAccel(const char* accel, struct sensor_t *list);
    void fillGyro(const char* gyro, struct sensor_t *list);
    void fillRV(struct sensor_t *list);
    void fillGMRV(struct sensor_t *list);
    void fillGRV(struct sensor_t *list);
    void fillOrientation(struct sensor_t *list);
    void fillGravity(struct sensor_t *list);
    void fillLinearAccel(struct sensor_t *list);
    void fillSignificantMotion(struct sensor_t *list);
#ifdef ENABLE_DMP_SCREEN_AUTO_ROTATION
    void fillScreenOrientation(struct sensor_t *list);
#endif
    void storeCalibration();
    void loadDMP();
    bool isMpuNonDmp();
    int isLowPowerQuatEnabled();
    int isDmpDisplayOrientationOn();
    void getCompassBias();
    void getFactoryGyroBias();
    void setFactoryGyroBias();
    void getGyroBias();
    void setGyroZeroBias();
    void setGyroBias();
    void getFactoryAccelBias();
    void setFactoryAccelBias();
    void getAccelBias();
    void setAccelBias();
    int isCompassDisabled();
    int setBatchDataRates();
    int resetDataRates();
    void initBias();
    void resetMplStates();
    void sys_dump(bool fileMode);
};

extern "C" {
    void setCallbackObject(MPLSensor*);
    MPLSensor *getCallbackObject();
}

#endif  // ANDROID_MPL_SENSOR_H
