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

#include "SensorBase.h"

// TODO: include 3rd-party compass vendor's header file
//       p.s.: before using unified HAL, make sure 3rd-party compass
//       solution's driver/HAL work well by themselves
#include "AkmSensor.h"

/*****************************************************************************/

class CompassSensor : public SensorBase {

protected:

public:
            CompassSensor();
    virtual ~CompassSensor();

    // TODO: make sure either 3rd-party compass solution has following virtual
    //       functions, or SensorBase.cpp could provide equal functionalities
    virtual int getFd() const;
    virtual int getRawFd() {return 0;};
    virtual int enable(int32_t handle, int enabled);
    virtual int setDelay(int32_t handle, int64_t ns);
    virtual int getEnable(int32_t handle);
    virtual int64_t getDelay(int32_t handle);
    virtual int64_t getMinDelay() { return -1; } // stub

    // TODO: unnecessary for MPL solution (override 3rd-party solution)
    virtual int readEvents(sensors_event_t *data, int count) { return 0; }

    // TODO: following four APIs need further implementation for MPL's
    //       reference (look into .cpp for detailed information, also refer to
    //       3rd-party's readEvents() for relevant APIs)
    int readSample(long *data, int64_t *timestamp);
    int readRawSample(float *data, int64_t *timestamp);
    void fillList(struct sensor_t *list);
    void getOrientationMatrix(signed char *orient);
    int getAccuracy();
    virtual void getCompassBias(long *bias) {return;};

    // TODO: if 3rd-party provides calibrated compass data, just return 1
    int providesCalibration() { return 1; }

    // TODO: hard-coded for 3rd-party's sensitivity transformation
    long getSensitivity() { return (1L << 30); }
    
    /* all 3rd pary solution have compasses on the primary bus, hence they
       have no dependency on the MPU */
    int isIntegrated() { return 0; }

    int checkCoilsReset(void) { return 0; };
    int isYasCompass(void) { return 0; };

private:
    AkmSensor *mCompassSensor;
};

/*****************************************************************************/

#endif  // COMPASS_SENSOR_H
