/*
 * Copyright (C) 2008-2014 The Android Open Source Project
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

#ifndef ANDROID_CWMCU_SENSOR_H
#define ANDROID_CWMCU_SENSOR_H

#include <errno.h>
#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <utils/BitSet.h>

#include "InputEventReader.h"
#include "sensors.h"
#include "SensorBase.h"

/*****************************************************************************/

// Must match driver copy of .../linux/include/linux/CwMcuSensor.h file
typedef enum {
    CW_ACCELERATION                = 0,
    CW_MAGNETIC                    = 1,
    CW_GYRO                        = 2,
    CW_LIGHT                       = 3,
    CW_PRESSURE                    = 5,
    CW_ORIENTATION                 = 6,
    CW_ROTATIONVECTOR              = 7,
    CW_LINEARACCELERATION          = 8,
    CW_GRAVITY                     = 9,
    CW_MAGNETIC_UNCALIBRATED       = 16,
    CW_GYROSCOPE_UNCALIBRATED      = 17,
    CW_GAME_ROTATION_VECTOR        = 18,
    CW_GEOMAGNETIC_ROTATION_VECTOR = 19,
    CW_SIGNIFICANT_MOTION          = 20,
    CW_STEP_DETECTOR               = 21,
    CW_STEP_COUNTER                = 22,
    HTC_ANY_MOTION                 = 28,
    //Above are Firmware supported sensors,
    CW_ACCELERATION_W                = 32,
    CW_MAGNETIC_W                    = 33,
    CW_GYRO_W                        = 34,
    CW_PRESSURE_W                    = 37,
    CW_ORIENTATION_W                 = 38,
    CW_ROTATIONVECTOR_W              = 39,
    CW_LINEARACCELERATION_W          = 40,
    CW_GRAVITY_W                     = 41,
    HTC_WAKE_UP_GESTURE_W            = 42,
    CW_MAGNETIC_UNCALIBRATED_W       = 48,
    CW_GYROSCOPE_UNCALIBRATED_W      = 49,
    CW_GAME_ROTATION_VECTOR_W        = 50,
    CW_GEOMAGNETIC_ROTATION_VECTOR_W = 51,
    CW_STEP_DETECTOR_W               = 53,
    CW_STEP_COUNTER_W                = 54,
    CW_SENSORS_ID_END,
    TIME_DIFF_EXHAUSTED              = 97,
    CW_TIME_BASE                     = 98,
    CW_META_DATA                     = 99,
    CW_MAGNETIC_UNCALIBRATED_BIAS    = 100,
    CW_GYROSCOPE_UNCALIBRATED_BIAS   = 101
} CW_SENSORS_ID;

#define        SAVE_PATH_ACC                                "/data/misc/AccOffset.txt"
#define        SAVE_PATH_MAG                                "/data/misc/cw_calibrator_mag.ini"
#define        SAVE_PATH_GYRO                                "/data/system/cw_calibrator_gyro.ini"

#define        BOOT_MODE_PATH                                "sys/class/htc_sensorhub/sensor_hub/boot_mode"

#define        numSensors        CW_SENSORS_ID_END

#define TIMESTAMP_SYNC_CODE        (98)

#define PERIODIC_SYNC_TIME_SEC     (5)

class CwMcuSensor : public SensorBase {

        android::BitSet64 mEnabled;
        InputEventCircularReader mInputReader;
        sensors_event_t mPendingEvents[numSensors];
        sensors_event_t mPendingEventsFlush;
        android::BitSet64 mPendingMask;
        char fixed_sysfs_path[PATH_MAX];
        int fixed_sysfs_path_len;

        float indexToValue(size_t index) const;
        char mDevPath[PATH_MAX];
        char mTriggerName[PATH_MAX];

        float time_slope;
        int64_t time_offset;
        uint64_t last_mcu_sync_time;
        uint64_t last_cpu_sync_time;

        bool offset_reset[numSensors];
        uint64_t last_mcu_timestamp[numSensors];
        uint64_t last_cpu_timestamp[numSensors];
        pthread_t sync_time_thread;

        bool init_trigger_done;

        int sysfs_set_input_attr(const char *attr, char *value, size_t len);
        int sysfs_set_input_attr_by_int(const char *attr, int value);
public:
        CwMcuSensor();
        virtual ~CwMcuSensor();
        virtual int readEvents(sensors_event_t* data, int count);
        virtual bool hasPendingEvents() const;
        virtual int setDelay(int32_t handle, int64_t ns);
        virtual int setEnable(int32_t handle, int enabled);
        virtual int getEnable(int32_t handle);
        virtual int batch(int handle, int flags, int64_t period_ns, int64_t timeout);
        virtual int flush(int handle);
        bool is_batch_wake_sensor(int32_t handle);
        int find_sensor(int32_t handle);
        int find_handle(int32_t sensors_id);
        void cw_save_calibrator_file(int type, const char * path, int* str);
        int cw_read_calibrator_file(int type, const char * path, int* str);
        int processEvent(uint8_t *event);
        void calculate_rv_4th_element(int sensors_id);
        void sync_time_thread_in_class(void);
};

/*****************************************************************************/

#endif  // ANDROID_CWMCU_SENSOR_H
