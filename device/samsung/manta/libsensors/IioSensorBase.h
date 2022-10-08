/*
 * Copyright (C) 2012 Samsung
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

#ifndef SAMSUNG_SENSORBASE_H
#define SAMSUNG_SENSORBASE_H

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include "sensors.h"
#include "SensorBase.h"
#include "InputEventReader.h"

/*****************************************************************************/
#define MAX_BUFFER_FOR_EVENT 4

class IioSensorBase:public SensorBase {
protected:
    bool mEnabled;
    bool mHasPendingEvent;
    InputEventCircularReader mInputReader;
    sensors_event_t mPendingEvent;
    char *mInputSysfsEnable;
    char *mInputSysfsSamplingFrequency;
    char *mIioSysfsChan;
    int mIioChanType;
    FILE *mIioSysfsChanFp;
    pthread_mutex_t mLock;

    char *makeSysfsName(const char *input_name,
                        const char *input_file);

    virtual bool readValue(int *value);
    virtual void handleData(int value);

public:
    IioSensorBase(const char* dev_name,
                      const char* data_name,
                      const char* enable_name,
                      const char* chan_name,
                      int iio_chan_type);

    virtual ~IioSensorBase();
    virtual int enable(int32_t handle, int en);
    virtual int setDelay(int32_t handle, int64_t ns);
    virtual int readEvents(sensors_event_t *data, int count);
};
#endif /* SAMSUNG_SENSORBASE_H */
