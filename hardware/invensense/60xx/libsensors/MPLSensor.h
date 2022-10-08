/*
 * Copyright (C) 2011 Invensense, Inc.
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
/*************Removed the gesture related info for Google check in : Meenakshi Ramamoorthi: May 31st *********/

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

/*****************************************************************************/
/** MPLSensor implementation which fits into the HAL example for crespo provided
 * * by Google.
 * * WARNING: there may only be one instance of MPLSensor, ever.
 */

class MPLSensor: public SensorBase
{
    typedef void (MPLSensor::*hfunc_t)(sensors_event_t*, uint32_t*, int);

public:
    MPLSensor();
    virtual ~MPLSensor();

    enum
    {
        Gyro=0,
        Accelerometer,
        MagneticField,
        Orientation,
        RotationVector,
        LinearAccel,
        Gravity,
        numSensors
    };

    virtual int setDelay(int32_t handle, int64_t ns);
    virtual int enable(int32_t handle, int enabled);
    virtual int readEvents(sensors_event_t *data, int count);
    virtual int getFd() const;
    virtual int getAccelFd() const;
    virtual int getTimerFd() const;
    virtual int getPowerFd() const;
    virtual int getPollTime();
    virtual bool hasPendingEvents() const;
    virtual void handlePowerEvent();
    virtual void sleepEvent();
    virtual void wakeEvent();
    int populateSensorList(struct sensor_t *list, int len);
    void cbOnMotion(uint16_t);
    void cbProcData();

protected:

    void clearIrqData(bool* irq_set);
    void setPowerStates(int enabledsensor);
    void initMPL();
    void setupFIFO();
    void setupCallbacks();
    void gyroHandler(sensors_event_t *data, uint32_t *pendmask, int index);
    void accelHandler(sensors_event_t *data, uint32_t *pendmask, int index);
    void compassHandler(sensors_event_t *data, uint32_t *pendmask, int index);
    void rvHandler(sensors_event_t *data, uint32_t *pendmask, int index);
    void laHandler(sensors_event_t *data, uint32_t *pendmask, int index);
    void gravHandler(sensors_event_t *data, uint32_t *pendmask, int index);
    void orienHandler(sensors_event_t *data, uint32_t *pendmask, int index);
    void calcOrientationSensor(float *Rx, float *Val);
    int estimateCompassAccuracy();

    int mNewData; //flag indicating that the MPL calculated new output values
    int mDmpStarted;
    long mMasterSensorMask;
    long mLocalSensorMask;
    int mPollTime;
    int mCurFifoRate; //current fifo rate
    bool mHaveGoodMpuCal; //flag indicating that the cal file can be written
    bool mHaveGoodCompassCal;
    bool mUseTimerIrqAccel;
    bool mUsetimerIrqCompass;
    bool mUseTimerirq;
    struct pollfd mPollFds[4];
    int mSampleCount;
    pthread_mutex_t mMplMutex;
    int64_t now_ns();
    int64_t select_ns(unsigned long long time_set[]);    

    enum FILEHANDLES
    {
        MPUIRQ_FD, ACCELIRQ_FD, COMPASSIRQ_FD, TIMERIRQ_FD,
    };

private:

    int update_delay();
    int accel_fd;
    int timer_fd;

    uint32_t mEnabled;
    uint32_t mPendingMask;
    sensors_event_t mPendingEvents[numSensors];
    uint64_t mDelays[numSensors];
    hfunc_t mHandlers[numSensors];
    bool mForceSleep;
    long int mOldEnabledMask;
    android::KeyedVector<int, int> mIrqFds;

    /* added for dynamic get sensor list              */
    bool mNineAxisEnabled;
    void fillAccel(unsigned char accel, struct sensor_t *list);
    void fillCompass(unsigned char compass, struct sensor_t *list);
    void fillGyro(const char* gyro, struct sensor_t *list);
    void fillRV(struct sensor_t *list);
    void fillOrientation(struct sensor_t *list);
    void fillGravity(struct sensor_t *list);
    void fillLinearAccel(struct sensor_t *list);
};

void setCallbackObject(MPLSensor*);

/*****************************************************************************/

#endif  // ANDROID_MPL_SENSOR_H
