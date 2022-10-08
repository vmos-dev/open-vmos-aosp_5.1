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

#ifndef COMPASS_SENSOR_H
#define COMPASS_SENSOR_H

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>

// TODO fixme, need input_event
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

#define MAX_CHIP_ID_LEN (20)
#define COMPASS_ON_PRIMARY "in_magn_x_raw"

class CompassSensor : public SensorBase {

public:
    CompassSensor();
    virtual ~CompassSensor();

    virtual int getFd() const;
    virtual int enable(int32_t handle, int enabled);
    virtual int setDelay(int32_t handle, int64_t ns);
    virtual int getEnable(int32_t handle);
    virtual int64_t getDelay(int32_t handle);
    virtual int64_t getMinDelay() { return mMinDelay; }

    // unnecessary for MPL
    virtual int readEvents(sensors_event_t *data, int count) { return 0; }

    int readSample(long *data, int64_t *timestamp);
    int readRawSample(float *data, int64_t *timestamp);
    int providesCalibration() { return 0; }
    void getOrientationMatrix(signed char *orient);
    long getSensitivity();
    int getAccuracy() { return 0; }
    void fillList(struct sensor_t *list);
    int isIntegrated() { return (0); }
    int checkCoilsReset(void);
    int isYasCompass(void);

private:
    enum CompassBus {
        COMPASS_BUS_PRIMARY = 0,
        COMPASS_BUS_SECONDARY = 1
    } mI2CBus;

    struct sysfs_attrbs {
       char *chip_enable;
       char *in_timestamp_en;
       char *buffer_length;

       char *compass_enable;
       char *compass_x_fifo_enable;
       char *compass_y_fifo_enable;
       char *compass_z_fifo_enable;
       char *compass_rate;
       char *compass_scale;
       char *compass_orient;
       char *compass_attr_1;
    } compassSysFs;
    
    char dev_full_name[20];

    // implementation specific
    signed char mCompassOrientation[9];
    long mCachedCompassData[3];
    int64_t mCompassTimestamp;
    InputEventCircularReader mCompassInputReader;
    int compass_fd;
    int64_t mDelay;
    int64_t mMinDelay;
    int mEnable;
    char *pathP;

    char mIIOBuffer[(8 + 8) * IIO_BUFFER_LENGTH];

    int masterEnable(int en);
    void enable_iio_sysfs(void);
    void processCompassEvent(const input_event *event);
    int inv_init_sysfs_attributes(void);
    FILE *mCoilsResetFd;
    bool mYasCompass;
};

/*****************************************************************************/

#endif  // COMPASS_SENSOR_H
