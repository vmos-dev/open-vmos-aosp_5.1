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

#ifndef PRESSURE_SENSOR_H
#define PRESSURE_SENSOR_H

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>

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

class PressureSensor : public SensorBase {

public:
    PressureSensor(const char *sysfs_path);
    virtual ~PressureSensor();

    virtual int getFd() const;
    virtual int enable(int32_t handle, int enabled);
    virtual int setDelay(int32_t handle, int64_t ns);
    virtual int getEnable(int32_t handle);
    virtual int64_t getDelay(int32_t handle);
    virtual int64_t getMinDelay() { return mMinDelay; }
    // only applicable to primary
    virtual int readEvents(sensors_event_t *data, int count) { return 0; }
            
    void fillList(struct sensor_t *list);    
    // default is integrated for secondary bus
    int isIntegrated() { return (1); }

private:
    char sensor_name[200];
    
    struct sysfs_attrbs {
       char *pressure_enable;
       char *pressure_rate;
    } pressureSysFs;

    int pressure_fd; 
    int64_t mDelay;
    int64_t mMinDelay;
    int mEnable;
    char* pathP;
    const char* mSysfsPath;

    int inv_init_sysfs_attributes(void);
};

/*****************************************************************************/

#endif  // PRESSURE_SENSOR_H
