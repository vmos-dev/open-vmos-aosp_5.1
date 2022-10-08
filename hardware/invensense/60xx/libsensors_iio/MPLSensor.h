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

#ifndef ANDROID_MPL_SENSOR_H
#define ANDROID_MPL_SENSOR_H

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <poll.h>
#include <utils/Vector.h>
#include <utils/KeyedVector.h>
#include "sensors.h"
#include "SensorBase.h"
#include "InputEventReader.h"

#ifdef INVENSENSE_COMPASS_CAL

#ifdef COMPASS_YAS53x
#warning "unified HAL for YAS53x"
#include "CompassSensor.IIO.primary.h"
#elif COMPASS_AMI306
#warning "unified HAL for AMI306"
#include "CompassSensor.IIO.primary.h"
#else
#warning "unified HAL for MPU9150"
#include "CompassSensor.IIO.9150.h"
#endif
#else
#warning "unified HAL for AKM"
#include "CompassSensor.AKM.h"
#endif

/*****************************************************************************/
/* Sensors Enable/Disable Mask
 *****************************************************************************/
#define MAX_CHIP_ID_LEN             (20)

#define INV_THREE_AXIS_GYRO         (0x000F)
#define INV_THREE_AXIS_ACCEL        (0x0070)
#define INV_THREE_AXIS_COMPASS      (0x0380)
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

// bit mask of current active features (mFeatureActiveMask)
#define INV_COMPASS_CAL              0x01
#define INV_COMPASS_FIT              0x02
#define INV_DMP_QUATERNION           0x04
#define INV_DMP_DISPL_ORIENTATION    0x08

/* Uncomment to enable Low Power Quaternion */
#define ENABLE_LP_QUAT_FEAT

/* Uncomment to enable DMP display orientation 
   (within the HAL, see below for Java framework) */
// #define ENABLE_DMP_DISPL_ORIENT_FEAT

#ifdef ENABLE_DMP_DISPL_ORIENT_FEAT
/* Uncomment following to expose the SENSOR_TYPE_SCREEN_ORIENTATION 
   sensor type (DMP screen orientation) to the Java framework.
   NOTE:
       need Invensense customized 
       'hardware/libhardware/include/hardware/sensors.h' to compile correctly.
   NOTE:
       need Invensense java framework changes to:
       'frameworks/base/core/java/android/view/WindowOrientationListener.java'
       'frameworks/base/core/java/android/hardware/Sensor.java'
       'frameworks/base/core/java/android/hardware/SensorEvent.java'
       for the 'Auto-rotate screen' to use this feature.
*/
#define ENABLE_DMP_SCREEN_AUTO_ROTATION
#warning "ENABLE_DMP_DISPL_ORIENT_FEAT is defined, framework changes are necessary for HAL to work properly"
#endif

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

    enum {
        Gyro = 0,
        RawGyro,
        Accelerometer,
        MagneticField,
        Orientation,
        RotationVector,
        LinearAccel,
        Gravity,
        numSensors
    };

    MPLSensor(CompassSensor *, int (*m_pt2AccelCalLoadFunc)(long*) = 0);
    virtual ~MPLSensor();

    virtual int setDelay(int32_t handle, int64_t ns);
    virtual int enable(int32_t handle, int enabled);
    int32_t getEnableMask() { return mEnabled; }

    virtual int readEvents(sensors_event_t *data, int count);
    virtual int getFd() const;
    virtual int getAccelFd() const;
    virtual int getCompassFd() const;
    virtual int getPollTime();
    virtual bool hasPendingEvents() const;
    virtual void sleepEvent();
    virtual void wakeEvent();
    int populateSensorList(struct sensor_t *list, int len);
    void cbProcData();

    //static pointer to the object that will handle callbacks
    static MPLSensor* gMPLSensor;

    //AKM HAL Integration
    //void set_compass(long ready, long x, long y, long z, long accuracy);
    int executeOnData(sensors_event_t* data, int count);
    int readAccelEvents(sensors_event_t* data, int count);
    int readCompassEvents(sensors_event_t* data, int count);

    int turnOffAccelFifo();
    int enableDmpOrientation(int);
    int dmpOrientHandler(int);
    int readDmpOrientEvents(sensors_event_t* data, int count);
    int getDmpOrientFd();
    int openDmpOrientFd();
    int closeDmpOrientFd();

    int getDmpRate(int64_t *);
    int checkDMPOrientation();

protected:
    CompassSensor *mCompassSensor;

    int gyroHandler(sensors_event_t *data);
    int rawGyroHandler(sensors_event_t *data);
    int accelHandler(sensors_event_t *data);
    int compassHandler(sensors_event_t *data);
    int rvHandler(sensors_event_t *data);
    int laHandler(sensors_event_t *data);
    int gravHandler(sensors_event_t *data);
    int orienHandler(sensors_event_t *data);
    void calcOrientationSensor(float *Rx, float *Val);
    virtual int update_delay();

    void inv_set_device_properties();
    int inv_constructor_init();
    int inv_constructor_default_enable();
    int setGyroInitialState();
    int setAccelInitialState();
    int masterEnable(int en);
    int onPower(int en);
    int enableLPQuaternion(int);
    int enableQuaternionData(int);
    int onDMP(int);
    int enableOneSensor(int en, const char *name, int (MPLSensor::*enabler)(int));
    int enableGyro(int en);
    int enableAccel(int en);
    int enableCompass(int en);
    void computeLocalSensorMask(int enabled_sensors);
    int enableSensors(unsigned long sensors, int en, uint32_t changed);
    int inv_read_gyro_buffer(int fd, short *data, long long *timestamp);
    int inv_float_to_q16(float *fdata, long *ldata);
    int inv_long_to_q16(long *fdata, long *ldata);
    int inv_float_to_round(float *fdata, long *ldata);
    int inv_float_to_round2(float *fdata, short *sdata);
    int inv_read_temperature(long long *data);
    int inv_read_dmp_state(int fd);
    int inv_read_sensor_bias(int fd, long *data);
    void inv_get_sensors_orientation(void);
    int inv_init_sysfs_attributes(void);
#ifdef COMPASS_YAS53x
    int resetCompass(void);
#endif
    void setCompassDelay(int64_t ns);
    void enable_iio_sysfs(void);
    int enableTap(int);
    int enableFlick(int);
    int enablePedometer(int);
    int checkLPQuaternion();

    int mNewData;   // flag indicating that the MPL calculated new output values
    int mDmpStarted;
    long mMasterSensorMask;
    long mLocalSensorMask;
    int mPollTime;
    bool mHaveGoodMpuCal;   // flag indicating that the cal file can be written
    int mGyroAccuracy;      // value indicating the quality of the gyro calibr.
    int mAccelAccuracy;     // value indicating the quality of the accel calibr.
    int mCompassAccuracy;     // value indicating the quality of the compass calibr.
    struct pollfd mPollFds[5];
    int mSampleCount;
    pthread_mutex_t mMplMutex;
    pthread_mutex_t mHALMutex;

    char mIIOBuffer[(16 + 8 * 3 + 8) * IIO_BUFFER_LENGTH];

    int iio_fd;
    int accel_fd;
    int mpufifo_fd;
    int gyro_temperature_fd;
    int dmp_orient_fd;

    int mDmpOrientationEnabled;


    uint32_t mEnabled;
    uint32_t mOldEnabledMask;
    sensors_event_t mPendingEvents[numSensors];
    int64_t mDelays[numSensors];
    hfunc_t mHandlers[numSensors];
    short mCachedGyroData[3];
    long mCachedAccelData[3];
    long mCachedCompassData[3];
    long mCachedQuaternionData[4];
    android::KeyedVector<int, int> mIrqFds;

    InputEventCircularReader mAccelInputReader;
    InputEventCircularReader mGyroInputReader;

    bool mFirstRead;
    short mTempScale;
    short mTempOffset;
    int64_t mTempCurrentTime;
    int mAccelScale;

    uint32_t mPendingMask;
    unsigned long mSensorMask;

    char chip_ID[MAX_CHIP_ID_LEN];

    signed char mGyroOrientation[9];
    signed char mAccelOrientation[9];

    int64_t mSensorTimestamp;
    int64_t mCompassTimestamp;

    struct sysfs_attrbs {
       char *chip_enable;
       char *power_state;
       char *dmp_firmware;
       char *firmware_loaded;
       char *dmp_on;
       char *dmp_int_on;
       char *dmp_event_int_on;
       char *dmp_output_rate;
       char *tap_on;
       char *key;
       char *self_test;
       char *temperature;

       char *gyro_enable;
       char *gyro_fifo_rate;
       char *gyro_orient;
       char *gyro_x_fifo_enable;
       char *gyro_y_fifo_enable;
       char *gyro_z_fifo_enable;

       char *accel_enable;
       char *accel_fifo_rate;
       char *accel_fsr;
       char *accel_bias;
       char *accel_orient;
       char *accel_x_fifo_enable;
       char *accel_y_fifo_enable;
       char *accel_z_fifo_enable;

       char *quaternion_on;
       char *in_quat_r_en;
       char *in_quat_x_en;
       char *in_quat_y_en;
       char *in_quat_z_en;

       char *in_timestamp_en;
       char *trigger_name;
       char *current_trigger;
       char *buffer_length;

       char *display_orientation_on;
       char *event_display_orientation;
    } mpu;

    char *sysfs_names_ptr;
    int mFeatureActiveMask;

private:
    /* added for dynamic get sensor list */
    void fillAccel(const char* accel, struct sensor_t *list);
    void fillGyro(const char* gyro, struct sensor_t *list);
    void fillRV(struct sensor_t *list);
    void fillOrientation(struct sensor_t *list);
    void fillGravity(struct sensor_t *list);
    void fillLinearAccel(struct sensor_t *list);
    void storeCalibration();
    void loadDMP();
    bool isMpu3050();
    int isLowPowerQuatEnabled();
    int isDmpDisplayOrientationOn();


};

extern "C" {
    void setCallbackObject(MPLSensor*);
    MPLSensor *getCallbackObject();
}

#endif  // ANDROID_MPL_SENSOR_H
