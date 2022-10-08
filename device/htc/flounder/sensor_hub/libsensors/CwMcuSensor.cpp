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

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/select.h>
#include <unistd.h>

#define LOG_TAG "CwMcuSensor"
#include <cutils/log.h>
#include <cutils/properties.h>

#include "CwMcuSensor.h"


#define REL_Significant_Motion REL_WHEEL
#define LIGHTSENSOR_LEVEL 10
#define DEBUG_DATA 0
#define COMPASS_CALIBRATION_DATA_SIZE 26
#define G_SENSOR_CALIBRATION_DATA_SIZE 3
#define NS_PER_MS 1000000LL
#define EXHAUSTED_MAGIC 0x77

/*****************************************************************************/
#define IIO_MAX_BUFF_SIZE 4096
#define IIO_MAX_DATA_SIZE 24
#define IIO_MAX_NAME_LENGTH 30
#define IIO_BUF_SIZE_RETRY 8
#define INT32_CHAR_LEN 12

#define INIT_TRIGGER_RETRY 5

static const char iio_dir[] = "/sys/bus/iio/devices/";

static int min(int a, int b) {
    return (a < b) ? a : b;
}

static int chomp(char *buf, size_t len) {
    if (buf == NULL)
        return -1;

    while (len > 0 && isspace(buf[len-1])) {
        buf[len - 1] = '\0';
        len--;
    }

    return 0;
}

int CwMcuSensor::sysfs_set_input_attr(const char *attr, char *value, size_t len) {
    char fname[PATH_MAX];
    int fd;
    int rc;

    snprintf(fname, sizeof(fname), "%s/%s", mDevPath, attr);
    fname[sizeof(fname) - 1] = '\0';

    fd = open(fname, O_WRONLY);
    if (fd < 0) {
        ALOGE("%s: fname = %s, fd = %d, failed: %s\n", __func__, fname, fd, strerror(errno));
        return -EACCES;
    }

    rc = write(fd, value, (size_t)len);
    if (rc < 0) {
        ALOGE("%s: write failed: fd = %d, rc = %d, strerr = %s\n", __func__, fd, rc, strerror(errno));
        close(fd);
        return -EIO;
    }

    close(fd);

    return 0;
}

int CwMcuSensor::sysfs_set_input_attr_by_int(const char *attr, int value) {
    char buf[INT32_CHAR_LEN];

    size_t n = snprintf(buf, sizeof(buf), "%d", value);
    if (n > sizeof(buf)) {
        return -1;
    }

    return sysfs_set_input_attr(attr, buf, n);
}

static inline int find_type_by_name(const char *name, const char *type) {
    const struct dirent *ent;
    int number, numstrlen;

    DIR *dp;
    char thisname[IIO_MAX_NAME_LENGTH];
    char *filename;
    size_t size;
    size_t typeLen = strlen(type);
    size_t nameLen = strlen(name);

    if (nameLen >= sizeof(thisname) - 1) {
        return -ERANGE;
    }

    dp = opendir(iio_dir);
    if (dp == NULL) {
        return -ENODEV;
    }

    while (ent = readdir(dp), ent != NULL) {
        if (strcmp(ent->d_name, ".") != 0 &&
                strcmp(ent->d_name, "..") != 0 &&
                strlen(ent->d_name) > typeLen &&
                strncmp(ent->d_name, type, typeLen) == 0) {
            numstrlen = sscanf(ent->d_name + typeLen,
                               "%d", &number);

            /* verify the next character is not a colon */
            if (ent->d_name[strlen(type) + numstrlen] != ':') {
                size = sizeof(iio_dir) - 1 + typeLen + numstrlen + 6;
                filename = (char *)malloc(size);

                if (filename == NULL)
                    return -ENOMEM;

                snprintf(filename, size,
                         "%s%s%d/name",
                         iio_dir, type, number);

                int fd = open(filename, O_RDONLY);
                free(filename);
                if (fd < 0) {
                    continue;
                }
                size = read(fd, thisname, sizeof(thisname) - 1);
                close(fd);
                if (size < nameLen) {
                    continue;
                }
                thisname[size] = '\0';
                if (strncmp(name, thisname, nameLen)) {
                    continue;
                }
                // check for termination or whitespace
                if (!thisname[nameLen] || isspace(thisname[nameLen])) {
                    return number;
                }
            }
        }
    }
    return -ENODEV;
}

int fill_block_debug = 0;

pthread_mutex_t sys_fs_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sync_timestamp_algo_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t last_timestamp_mutex = PTHREAD_MUTEX_INITIALIZER;

void CwMcuSensor::sync_time_thread_in_class(void) {
    int fd;
    char buf[24];
    int err;
    uint64_t mcu_current_time;
    uint64_t cpu_current_time;
    int open_errno;

    ALOGV("sync_time_thread_in_class++:\n");

    pthread_mutex_lock(&sys_fs_mutex);

    strcpy(&fixed_sysfs_path[fixed_sysfs_path_len], "batch_enable");

    fd = open(fixed_sysfs_path, O_RDWR);
    open_errno = errno;
    pthread_mutex_unlock(&sys_fs_mutex);
    if (fd >= 0) {
        err = read(fd, buf, sizeof(buf) - 1);
        cpu_current_time = getTimestamp();
        if (err < 0) {
            ALOGE("sync_time_thread_in_class: read fail, err = %d\n", err);
        } else {
            buf[err] = '\0';
            mcu_current_time = strtoull(buf, NULL, 10) * NS_PER_US;
            if (errno == ERANGE) {
                ALOGE("sync_time_thread_in_class: strtoll fails, strerr = %s, buf = %s\n",
                      strerror(errno), buf);
            } else {
                pthread_mutex_lock(&sync_timestamp_algo_mutex);

                if (mcu_current_time == 0) {
                    // Do a recovery mechanism of timestamp estimation when the sensor_hub reset happened
                    ALOGE("Sync: sensor hub is on reset\n");
                    time_slope = 1;
                    memset(last_mcu_timestamp, 0, sizeof(last_mcu_timestamp));
                    memset(last_cpu_timestamp, 0, sizeof(last_cpu_timestamp));
                    for (int i=0; i<numSensors; i++) {
                        offset_reset[i] = true;
                    }
                } else if ((mcu_current_time <= last_mcu_sync_time) || (last_mcu_sync_time == 0)) {
                    ALOGV("Sync: time_slope was not estimated yet\n");
                    time_slope = 1;
                    time_offset = cpu_current_time - mcu_current_time;
                    for (int i=0; i<numSensors; i++) {
                        offset_reset[i] = true;
                    }
                } else {
                    time_slope = (float)(cpu_current_time - last_cpu_sync_time) /
                                 (float)(mcu_current_time - last_mcu_sync_time);
                    time_offset = cpu_current_time - mcu_current_time;
                }
                ALOGV("Sync: time_offset = %" PRId64 ", time_slope = %f\n", time_offset, time_slope);
                ALOGV("Sync: mcu_current_time = %" PRId64 ", last_mcu_sync_time = %" PRId64 "\n", mcu_current_time, last_mcu_sync_time);
                ALOGV("Sync: cpu_current_time = %" PRId64 ", last_cpu_sync_time = %" PRId64 "\n", cpu_current_time, last_cpu_sync_time);

                last_mcu_sync_time = mcu_current_time;
                last_cpu_sync_time = cpu_current_time;

                pthread_mutex_unlock(&sync_timestamp_algo_mutex);
            }
        }
        close(fd);
    } else {
        ALOGE("sync_time_thread_in_class: open failed, path = .../batch_enable, fd = %d,"
              " strerr = %s\n", fd, strerror(open_errno));
    }

    ALOGV("sync_time_thread_in_class--:\n");
}

void *sync_time_thread_run(void *context) {
    CwMcuSensor *myClass = (CwMcuSensor *)context;

    while (1) {
        ALOGV("sync_time_thread_run++:\n");
        myClass->sync_time_thread_in_class();
        sleep(PERIODIC_SYNC_TIME_SEC);
        ALOGV("sync_time_thread_run--:\n");
    }
    return NULL;
}

CwMcuSensor::CwMcuSensor()
    : SensorBase(NULL, "CwMcuSensor")
    , mEnabled(0)
    , mInputReader(IIO_MAX_BUFF_SIZE)
    , time_slope(1)
    , time_offset(0)
    , init_trigger_done(false) {

    int rc;

    memset(last_mcu_timestamp, 0, sizeof(last_mcu_timestamp));
    memset(last_cpu_timestamp, 0, sizeof(last_cpu_timestamp));
    for (int i=0; i<numSensors; i++) {
        offset_reset[i] = true;
    }

    mPendingEvents[CW_ACCELERATION].version = sizeof(sensors_event_t);
    mPendingEvents[CW_ACCELERATION].sensor = ID_A;
    mPendingEvents[CW_ACCELERATION].type = SENSOR_TYPE_ACCELEROMETER;
    mPendingEvents[CW_ACCELERATION].acceleration.status = SENSOR_STATUS_ACCURACY_HIGH;

    mPendingEvents[CW_MAGNETIC].version = sizeof(sensors_event_t);
    mPendingEvents[CW_MAGNETIC].sensor = ID_M;
    mPendingEvents[CW_MAGNETIC].type = SENSOR_TYPE_MAGNETIC_FIELD;

    mPendingEvents[CW_GYRO].version = sizeof(sensors_event_t);
    mPendingEvents[CW_GYRO].sensor = ID_GY;
    mPendingEvents[CW_GYRO].type = SENSOR_TYPE_GYROSCOPE;
    mPendingEvents[CW_GYRO].gyro.status = SENSOR_STATUS_ACCURACY_HIGH;

    mPendingEvents[CW_LIGHT].version = sizeof(sensors_event_t);
    mPendingEvents[CW_LIGHT].sensor = ID_L;
    mPendingEvents[CW_LIGHT].type = SENSOR_TYPE_LIGHT;
    memset(mPendingEvents[CW_LIGHT].data, 0, sizeof(mPendingEvents[CW_LIGHT].data));

    mPendingEvents[CW_PRESSURE].version = sizeof(sensors_event_t);
    mPendingEvents[CW_PRESSURE].sensor = ID_PS;
    mPendingEvents[CW_PRESSURE].type = SENSOR_TYPE_PRESSURE;
    memset(mPendingEvents[CW_PRESSURE].data, 0, sizeof(mPendingEvents[CW_PRESSURE].data));

    mPendingEvents[CW_ORIENTATION].version = sizeof(sensors_event_t);
    mPendingEvents[CW_ORIENTATION].sensor = ID_O;
    mPendingEvents[CW_ORIENTATION].type = SENSOR_TYPE_ORIENTATION;
    mPendingEvents[CW_ORIENTATION].orientation.status = SENSOR_STATUS_ACCURACY_HIGH;

    mPendingEvents[CW_ROTATIONVECTOR].version = sizeof(sensors_event_t);
    mPendingEvents[CW_ROTATIONVECTOR].sensor = ID_RV;
    mPendingEvents[CW_ROTATIONVECTOR].type = SENSOR_TYPE_ROTATION_VECTOR;

    mPendingEvents[CW_LINEARACCELERATION].version = sizeof(sensors_event_t);
    mPendingEvents[CW_LINEARACCELERATION].sensor = ID_LA;
    mPendingEvents[CW_LINEARACCELERATION].type = SENSOR_TYPE_LINEAR_ACCELERATION;

    mPendingEvents[CW_GRAVITY].version = sizeof(sensors_event_t);
    mPendingEvents[CW_GRAVITY].sensor = ID_G;
    mPendingEvents[CW_GRAVITY].type = SENSOR_TYPE_GRAVITY;

    mPendingEvents[CW_MAGNETIC_UNCALIBRATED].version = sizeof(sensors_event_t);
    mPendingEvents[CW_MAGNETIC_UNCALIBRATED].sensor = ID_CW_MAGNETIC_UNCALIBRATED;
    mPendingEvents[CW_MAGNETIC_UNCALIBRATED].type = SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED;

    mPendingEvents[CW_GYROSCOPE_UNCALIBRATED].version = sizeof(sensors_event_t);
    mPendingEvents[CW_GYROSCOPE_UNCALIBRATED].sensor = ID_CW_GYROSCOPE_UNCALIBRATED;
    mPendingEvents[CW_GYROSCOPE_UNCALIBRATED].type = SENSOR_TYPE_GYROSCOPE_UNCALIBRATED;

    mPendingEvents[CW_GAME_ROTATION_VECTOR].version = sizeof(sensors_event_t);
    mPendingEvents[CW_GAME_ROTATION_VECTOR].sensor = ID_CW_GAME_ROTATION_VECTOR;
    mPendingEvents[CW_GAME_ROTATION_VECTOR].type = SENSOR_TYPE_GAME_ROTATION_VECTOR;

    mPendingEvents[CW_GEOMAGNETIC_ROTATION_VECTOR].version = sizeof(sensors_event_t);
    mPendingEvents[CW_GEOMAGNETIC_ROTATION_VECTOR].sensor = ID_CW_GEOMAGNETIC_ROTATION_VECTOR;
    mPendingEvents[CW_GEOMAGNETIC_ROTATION_VECTOR].type = SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR;

    mPendingEvents[CW_SIGNIFICANT_MOTION].version = sizeof(sensors_event_t);
    mPendingEvents[CW_SIGNIFICANT_MOTION].sensor = ID_CW_SIGNIFICANT_MOTION;
    mPendingEvents[CW_SIGNIFICANT_MOTION].type = SENSOR_TYPE_SIGNIFICANT_MOTION;

    mPendingEvents[CW_STEP_DETECTOR].version = sizeof(sensors_event_t);
    mPendingEvents[CW_STEP_DETECTOR].sensor = ID_CW_STEP_DETECTOR;
    mPendingEvents[CW_STEP_DETECTOR].type = SENSOR_TYPE_STEP_DETECTOR;

    mPendingEvents[CW_STEP_COUNTER].version = sizeof(sensors_event_t);
    mPendingEvents[CW_STEP_COUNTER].sensor = ID_CW_STEP_COUNTER;
    mPendingEvents[CW_STEP_COUNTER].type = SENSOR_TYPE_STEP_COUNTER;


    mPendingEvents[CW_ACCELERATION_W].version = sizeof(sensors_event_t);
    mPendingEvents[CW_ACCELERATION_W].sensor = ID_A_W;
    mPendingEvents[CW_ACCELERATION_W].type = SENSOR_TYPE_ACCELEROMETER;
    mPendingEvents[CW_ACCELERATION_W].acceleration.status = SENSOR_STATUS_ACCURACY_HIGH;

    mPendingEvents[CW_MAGNETIC_W].version = sizeof(sensors_event_t);
    mPendingEvents[CW_MAGNETIC_W].sensor = ID_M_W;
    mPendingEvents[CW_MAGNETIC_W].type = SENSOR_TYPE_MAGNETIC_FIELD;

    mPendingEvents[CW_GYRO_W].version = sizeof(sensors_event_t);
    mPendingEvents[CW_GYRO_W].sensor = ID_GY_W;
    mPendingEvents[CW_GYRO_W].type = SENSOR_TYPE_GYROSCOPE;
    mPendingEvents[CW_GYRO_W].gyro.status = SENSOR_STATUS_ACCURACY_HIGH;

    mPendingEvents[CW_PRESSURE_W].version = sizeof(sensors_event_t);
    mPendingEvents[CW_PRESSURE_W].sensor = ID_PS_W;
    mPendingEvents[CW_PRESSURE_W].type = SENSOR_TYPE_PRESSURE;
    memset(mPendingEvents[CW_PRESSURE_W].data, 0, sizeof(mPendingEvents[CW_PRESSURE_W].data));

    mPendingEvents[CW_ORIENTATION_W].version = sizeof(sensors_event_t);
    mPendingEvents[CW_ORIENTATION_W].sensor = ID_O_W;
    mPendingEvents[CW_ORIENTATION_W].type = SENSOR_TYPE_ORIENTATION;
    mPendingEvents[CW_ORIENTATION_W].orientation.status = SENSOR_STATUS_ACCURACY_HIGH;

    mPendingEvents[CW_ROTATIONVECTOR_W].version = sizeof(sensors_event_t);
    mPendingEvents[CW_ROTATIONVECTOR_W].sensor = ID_RV_W;
    mPendingEvents[CW_ROTATIONVECTOR_W].type = SENSOR_TYPE_ROTATION_VECTOR;

    mPendingEvents[CW_LINEARACCELERATION_W].version = sizeof(sensors_event_t);
    mPendingEvents[CW_LINEARACCELERATION_W].sensor = ID_LA_W;
    mPendingEvents[CW_LINEARACCELERATION_W].type = SENSOR_TYPE_LINEAR_ACCELERATION;

    mPendingEvents[CW_GRAVITY_W].version = sizeof(sensors_event_t);
    mPendingEvents[CW_GRAVITY_W].sensor = ID_G_W;
    mPendingEvents[CW_GRAVITY_W].type = SENSOR_TYPE_GRAVITY;

    mPendingEvents[CW_MAGNETIC_UNCALIBRATED_W].version = sizeof(sensors_event_t);
    mPendingEvents[CW_MAGNETIC_UNCALIBRATED_W].sensor = ID_CW_MAGNETIC_UNCALIBRATED_W;
    mPendingEvents[CW_MAGNETIC_UNCALIBRATED_W].type = SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED;

    mPendingEvents[CW_GYROSCOPE_UNCALIBRATED_W].version = sizeof(sensors_event_t);
    mPendingEvents[CW_GYROSCOPE_UNCALIBRATED_W].sensor = ID_CW_GYROSCOPE_UNCALIBRATED_W;
    mPendingEvents[CW_GYROSCOPE_UNCALIBRATED_W].type = SENSOR_TYPE_GYROSCOPE_UNCALIBRATED;

    mPendingEvents[CW_GAME_ROTATION_VECTOR_W].version = sizeof(sensors_event_t);
    mPendingEvents[CW_GAME_ROTATION_VECTOR_W].sensor = ID_CW_GAME_ROTATION_VECTOR_W;
    mPendingEvents[CW_GAME_ROTATION_VECTOR_W].type = SENSOR_TYPE_GAME_ROTATION_VECTOR;

    mPendingEvents[CW_GEOMAGNETIC_ROTATION_VECTOR_W].version = sizeof(sensors_event_t);
    mPendingEvents[CW_GEOMAGNETIC_ROTATION_VECTOR_W].sensor = ID_CW_GEOMAGNETIC_ROTATION_VECTOR_W;
    mPendingEvents[CW_GEOMAGNETIC_ROTATION_VECTOR_W].type = SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR;

    mPendingEvents[CW_STEP_DETECTOR_W].version = sizeof(sensors_event_t);
    mPendingEvents[CW_STEP_DETECTOR_W].sensor = ID_CW_STEP_DETECTOR_W;
    mPendingEvents[CW_STEP_DETECTOR_W].type = SENSOR_TYPE_STEP_DETECTOR;

    mPendingEvents[CW_STEP_COUNTER_W].version = sizeof(sensors_event_t);
    mPendingEvents[CW_STEP_COUNTER_W].sensor = ID_CW_STEP_COUNTER_W;
    mPendingEvents[CW_STEP_COUNTER_W].type = SENSOR_TYPE_STEP_COUNTER;


    mPendingEventsFlush.version = META_DATA_VERSION;
    mPendingEventsFlush.sensor = 0;
    mPendingEventsFlush.type = SENSOR_TYPE_META_DATA;

    char buffer_access[PATH_MAX];
    const char *device_name = "CwMcuSensor";
    int rate = 20, dev_num, enabled = 0, i;

    dev_num = find_type_by_name(device_name, "iio:device");
    if (dev_num < 0)
        dev_num = 0;

    snprintf(buffer_access, sizeof(buffer_access),
            "/dev/iio:device%d", dev_num);

    data_fd = open(buffer_access, O_RDWR);
    if (data_fd < 0) {
        ALOGE("CwMcuSensor::CwMcuSensor: open file '%s' failed: %s\n",
              buffer_access, strerror(errno));
    }

    if (data_fd >= 0) {
        int i;
        int fd;
        int iio_buf_size;

        ALOGV("%s: 11 Before pthread_mutex_lock()\n", __func__);
        pthread_mutex_lock(&sys_fs_mutex);
        ALOGV("%s: 11 Acquired pthread_mutex_lock()\n", __func__);

        strcpy(fixed_sysfs_path,"/sys/class/htc_sensorhub/sensor_hub/");
        fixed_sysfs_path_len = strlen(fixed_sysfs_path);

        snprintf(mDevPath, sizeof(mDevPath), "%s%s", fixed_sysfs_path, "iio");

        snprintf(mTriggerName, sizeof(mTriggerName), "%s-dev%d",
                 device_name, dev_num);
        ALOGV("CwMcuSensor::CwMcuSensor: mTriggerName = %s\n", mTriggerName);

        if (sysfs_set_input_attr_by_int("buffer/enable", 0) < 0) {
            ALOGE("CwMcuSensor::CwMcuSensor: set IIO buffer enable failed00: %s\n",
                  strerror(errno));
        }

        // This is a piece of paranoia that retry for current_trigger
        for (i = 0; i < INIT_TRIGGER_RETRY; i++) {
            rc = sysfs_set_input_attr("trigger/current_trigger",
                                      mTriggerName, strlen(mTriggerName));
            if (rc < 0) {
                if (sysfs_set_input_attr_by_int("buffer/enable", 0) < 0) {
                    ALOGE("CwMcuSensor::CwMcuSensor: set IIO buffer enable failed11: %s\n",
                          strerror(errno));
                }
                ALOGE("CwMcuSensor::CwMcuSensor: set current trigger failed: rc = %d, strerr() = %s"
                      ", i = %d\n",
                      rc, strerror(errno), i);
            } else {
                init_trigger_done = true;
                break;
            }
        }

        iio_buf_size = IIO_MAX_BUFF_SIZE;
        for (i = 0; i < IIO_BUF_SIZE_RETRY; i++) {
            if (sysfs_set_input_attr_by_int("buffer/length", iio_buf_size) < 0) {
                ALOGE("CwMcuSensor::CwMcuSensor: set IIO buffer length (%d) failed: %s\n",
                      iio_buf_size, strerror(errno));
            } else {
                if (sysfs_set_input_attr_by_int("buffer/enable", 1) < 0) {
                    ALOGE("CwMcuSensor::CwMcuSensor: set IIO buffer enable failed22: %s, "
                          "i = %d, iio_buf_size = %d\n", strerror(errno), i, iio_buf_size);
                } else {
                    ALOGI("CwMcuSensor::CwMcuSensor: set IIO buffer length success: %d\n", iio_buf_size);
                    break;
                }
            }
            iio_buf_size /= 2;
        }

        strcpy(&fixed_sysfs_path[fixed_sysfs_path_len], "calibrator_en");
        fd = open(fixed_sysfs_path, O_RDWR);
        if (fd >= 0) {
            static const char buf[] = "12";

            rc = write(fd, buf, sizeof(buf) - 1);
            if (rc < 0) {
                ALOGE("%s: write buf = %s, failed: %s", __func__, buf, strerror(errno));
            }

            close(fd);
        } else {
            ALOGE("%s open %s failed: %s", __func__, fixed_sysfs_path, strerror(errno));
        }

        pthread_mutex_unlock(&sys_fs_mutex);

        ALOGV("%s: data_fd = %d", __func__, data_fd);
        ALOGV("%s: iio_device_path = %s", __func__, buffer_access);
        ALOGV("%s: ctrl sysfs_path = %s", __func__, fixed_sysfs_path);

        setEnable(0, 1); // Inside this function call, we use sys_fs_mutex
    }

    int gs_temp_data[G_SENSOR_CALIBRATION_DATA_SIZE] = {0};
    int compass_temp_data[COMPASS_CALIBRATION_DATA_SIZE] = {0};


    ALOGV("%s: 22 Before pthread_mutex_lock()\n", __func__);
    pthread_mutex_lock(&sys_fs_mutex);
    ALOGV("%s: 22 Acquired pthread_mutex_lock()\n", __func__);

    //Sensor Calibration init . Waiting for firmware ready
    rc = cw_read_calibrator_file(CW_MAGNETIC, SAVE_PATH_MAG, compass_temp_data);
    if (rc == 0) {
        ALOGD("Get compass calibration data from data/misc/ x is %d ,y is %d ,z is %d\n",
              compass_temp_data[0], compass_temp_data[1], compass_temp_data[2]);
        strcpy(&fixed_sysfs_path[fixed_sysfs_path_len], "calibrator_data_mag");
        cw_save_calibrator_file(CW_MAGNETIC, fixed_sysfs_path, compass_temp_data);
    } else {
        ALOGI("Compass calibration data does not exist\n");
    }

    rc = cw_read_calibrator_file(CW_ACCELERATION, SAVE_PATH_ACC, gs_temp_data);
    if (rc == 0) {
        ALOGD("Get g-sensor user calibration data from data/misc/ x is %d ,y is %d ,z is %d\n",
              gs_temp_data[0],gs_temp_data[1],gs_temp_data[2]);
        strcpy(&fixed_sysfs_path[fixed_sysfs_path_len], "calibrator_data_acc");
        if(!(gs_temp_data[0] == 0 && gs_temp_data[1] == 0 && gs_temp_data[2] == 0 )) {
            cw_save_calibrator_file(CW_ACCELERATION, fixed_sysfs_path, gs_temp_data);
        }
    } else {
        ALOGI("G-Sensor user calibration data does not exist\n");
    }

    pthread_mutex_unlock(&sys_fs_mutex);

    pthread_create(&sync_time_thread, (const pthread_attr_t *) NULL,
                    sync_time_thread_run, (void *)this);

}

CwMcuSensor::~CwMcuSensor() {
    if (!mEnabled.isEmpty()) {
        setEnable(0, 0);
    }
}

float CwMcuSensor::indexToValue(size_t index) const {
    static const float luxValues[LIGHTSENSOR_LEVEL] = {
        0.0, 10.0, 40.0, 90.0, 160.0,
        225.0, 320.0, 640.0, 1280.0,
        2600.0
    };

    const size_t maxIndex = (LIGHTSENSOR_LEVEL - 1);
    if (index > maxIndex) {
        index = maxIndex;
    }
    return luxValues[index];
}

int CwMcuSensor::find_handle(int32_t sensors_id) {
    switch (sensors_id) {
    case CW_ACCELERATION:
        return ID_A;
    case CW_MAGNETIC:
        return ID_M;
    case CW_GYRO:
        return ID_GY;
    case CW_PRESSURE:
        return ID_PS;
    case CW_ORIENTATION:
        return ID_O;
    case CW_ROTATIONVECTOR:
        return ID_RV;
    case CW_LINEARACCELERATION:
        return ID_LA;
    case CW_GRAVITY:
        return ID_G;
    case CW_MAGNETIC_UNCALIBRATED:
        return ID_CW_MAGNETIC_UNCALIBRATED;
    case CW_GYROSCOPE_UNCALIBRATED:
        return ID_CW_GYROSCOPE_UNCALIBRATED;
    case CW_GAME_ROTATION_VECTOR:
        return ID_CW_GAME_ROTATION_VECTOR;
    case CW_GEOMAGNETIC_ROTATION_VECTOR:
        return ID_CW_GEOMAGNETIC_ROTATION_VECTOR;
    case CW_LIGHT:
        return ID_L;
    case CW_SIGNIFICANT_MOTION:
        return ID_CW_SIGNIFICANT_MOTION;
    case CW_STEP_DETECTOR:
        return ID_CW_STEP_DETECTOR;
    case CW_STEP_COUNTER:
        return ID_CW_STEP_COUNTER;
    case CW_ACCELERATION_W:
        return ID_A_W;
    case CW_MAGNETIC_W:
        return ID_M_W;
    case CW_GYRO_W:
        return ID_GY_W;
    case CW_PRESSURE_W:
        return ID_PS_W;
    case CW_ORIENTATION_W:
        return ID_O_W;
    case CW_ROTATIONVECTOR_W:
        return ID_RV_W;
    case CW_LINEARACCELERATION_W:
        return ID_LA_W;
    case CW_GRAVITY_W:
        return ID_G_W;
    case CW_MAGNETIC_UNCALIBRATED_W:
        return ID_CW_MAGNETIC_UNCALIBRATED_W;
    case CW_GYROSCOPE_UNCALIBRATED_W:
        return ID_CW_GYROSCOPE_UNCALIBRATED_W;
    case CW_GAME_ROTATION_VECTOR_W:
        return ID_CW_GAME_ROTATION_VECTOR_W;
    case CW_GEOMAGNETIC_ROTATION_VECTOR_W:
        return ID_CW_GEOMAGNETIC_ROTATION_VECTOR_W;
    case CW_STEP_DETECTOR_W:
        return ID_CW_STEP_DETECTOR_W;
    case CW_STEP_COUNTER_W:
        return ID_CW_STEP_COUNTER_W;
    default:
        return 0xFF;
    }
}

bool CwMcuSensor::is_batch_wake_sensor(int32_t handle) {
    switch (handle) {
    case ID_A_W:
    case ID_M_W:
    case ID_GY_W:
    case ID_PS_W:
    case ID_O_W:
    case ID_RV_W:
    case ID_LA_W:
    case ID_G_W:
    case ID_CW_MAGNETIC_UNCALIBRATED_W:
    case ID_CW_GYROSCOPE_UNCALIBRATED_W:
    case ID_CW_GAME_ROTATION_VECTOR_W:
    case ID_CW_GEOMAGNETIC_ROTATION_VECTOR_W:
    case ID_CW_STEP_DETECTOR_W:
    case ID_CW_STEP_COUNTER_W:
        return true;
    default:
        return false;
    }
}

int CwMcuSensor::find_sensor(int32_t handle) {
    int what = -1;

    switch (handle) {
    case ID_A:
        what = CW_ACCELERATION;
        break;
    case ID_A_W:
        what = CW_ACCELERATION_W;
        break;
    case ID_M:
        what = CW_MAGNETIC;
        break;
    case ID_M_W:
        what = CW_MAGNETIC_W;
        break;
    case ID_GY:
        what = CW_GYRO;
        break;
    case ID_GY_W:
        what = CW_GYRO_W;
        break;
    case ID_PS:
        what = CW_PRESSURE;
        break;
    case ID_PS_W:
        what = CW_PRESSURE_W;
        break;
    case ID_O:
        what = CW_ORIENTATION;
        break;
    case ID_O_W:
        what = CW_ORIENTATION_W;
        break;
    case ID_RV:
        what = CW_ROTATIONVECTOR;
        break;
    case ID_RV_W:
        what = CW_ROTATIONVECTOR_W;
        break;
    case ID_LA:
        what = CW_LINEARACCELERATION;
        break;
    case ID_LA_W:
        what = CW_LINEARACCELERATION_W;
        break;
    case ID_G:
        what = CW_GRAVITY;
        break;
    case ID_G_W:
        what = CW_GRAVITY_W;
        break;
    case ID_CW_MAGNETIC_UNCALIBRATED:
        what = CW_MAGNETIC_UNCALIBRATED;
        break;
    case ID_CW_MAGNETIC_UNCALIBRATED_W:
        what = CW_MAGNETIC_UNCALIBRATED_W;
        break;
    case ID_CW_GYROSCOPE_UNCALIBRATED:
        what = CW_GYROSCOPE_UNCALIBRATED;
        break;
    case ID_CW_GYROSCOPE_UNCALIBRATED_W:
        what = CW_GYROSCOPE_UNCALIBRATED_W;
        break;
    case ID_CW_GAME_ROTATION_VECTOR:
        what = CW_GAME_ROTATION_VECTOR;
        break;
    case ID_CW_GAME_ROTATION_VECTOR_W:
        what = CW_GAME_ROTATION_VECTOR_W;
        break;
    case ID_CW_GEOMAGNETIC_ROTATION_VECTOR:
        what = CW_GEOMAGNETIC_ROTATION_VECTOR;
        break;
    case ID_CW_GEOMAGNETIC_ROTATION_VECTOR_W:
        what = CW_GEOMAGNETIC_ROTATION_VECTOR_W;
        break;
    case ID_CW_SIGNIFICANT_MOTION:
        what = CW_SIGNIFICANT_MOTION;
        break;
    case ID_CW_STEP_DETECTOR:
        what = CW_STEP_DETECTOR;
        break;
    case ID_CW_STEP_DETECTOR_W:
        what = CW_STEP_DETECTOR_W;
        break;
    case ID_CW_STEP_COUNTER:
        what = CW_STEP_COUNTER;
        break;
    case ID_CW_STEP_COUNTER_W:
        what = CW_STEP_COUNTER_W;
        break;
    case ID_L:
        what = CW_LIGHT;
        break;
    }

    return what;
}

int CwMcuSensor::getEnable(int32_t handle) {
    ALOGV("CwMcuSensor::getEnable: handle = %d\n", handle);
    return  0;
}

int CwMcuSensor::setEnable(int32_t handle, int en) {

    int what;
    int err = 0;
    int flags = !!en;
    int fd;
    char buf[10];
    int temp_data[COMPASS_CALIBRATION_DATA_SIZE];
    char value[PROPERTY_VALUE_MAX] = {0};
    int rc;

    ALOGV("%s: Before pthread_mutex_lock()\n", __func__);
    pthread_mutex_lock(&sys_fs_mutex);
    ALOGV("%s: Acquired pthread_mutex_lock()\n", __func__);

    property_get("debug.sensorhal.fill.block", value, "0");
    ALOGV("CwMcuSensor::setEnable: debug.sensorhal.fill.block= %s", value);
    fill_block_debug = atoi(value) == 1;

    what = find_sensor(handle);

    ALOGV("CwMcuSensor::setEnable: "
          "[v13-Dynamic adjust the IIO buffer], handle = %d, en = %d, what = %d\n",
          handle, en, what);

    if (uint32_t(what) >= numSensors) {
        pthread_mutex_unlock(&sys_fs_mutex);
        return -EINVAL;
    }

    offset_reset[what] = !!flags;

    strcpy(&fixed_sysfs_path[fixed_sysfs_path_len], "enable");
    fd = open(fixed_sysfs_path, O_RDWR);
    if (fd >= 0) {
        int n = snprintf(buf, sizeof(buf), "%d %d\n", what, flags);
        err = write(fd, buf, min(n, sizeof(buf)));
        if (err < 0) {
            ALOGE("%s: write failed: %s", __func__, strerror(errno));
        }

        close(fd);

        if (flags) {
            mEnabled.markBit(what);
        } else {
            mEnabled.clearBit(what);
        }

        if (mEnabled.isEmpty()) {
            if (sysfs_set_input_attr_by_int("buffer/enable", 0) < 0) {
                ALOGE("CwMcuSensor::setEnable: set buffer disable failed: %s\n", strerror(errno));
            } else {
                ALOGV("CwMcuSensor::setEnable: set IIO buffer enable = 0\n");
            }
        }
    } else {
        ALOGE("%s open failed: %s", __func__, strerror(errno));
    }


    // Sensor Calibration init. Waiting for firmware ready
    if (!flags &&
            ((what == CW_MAGNETIC) ||
             (what == CW_ORIENTATION) ||
             (what == CW_ROTATIONVECTOR))) {
        ALOGV("Save Compass calibration data");
        strcpy(&fixed_sysfs_path[fixed_sysfs_path_len], "calibrator_data_mag");
        rc = cw_read_calibrator_file(CW_MAGNETIC, fixed_sysfs_path, temp_data);
        if (rc== 0) {
            cw_save_calibrator_file(CW_MAGNETIC, SAVE_PATH_MAG, temp_data);
        } else {
            ALOGI("Compass calibration data from driver fails\n");
        }
    }

    pthread_mutex_unlock(&sys_fs_mutex);
    return 0;
}

int CwMcuSensor::batch(int handle, int flags, int64_t period_ns, int64_t timeout)
{
    int what;
    int fd;
    char buf[32] = {0};
    int err;
    int delay_ms;
    int timeout_ms;
    bool dryRun = false;

    ALOGV("CwMcuSensor::batch++: handle = %d, flags = %d, period_ns = %" PRId64 ", timeout = %" PRId64 "\n",
        handle, flags, period_ns, timeout);

    what = find_sensor(handle);
    delay_ms = period_ns/NS_PER_MS; // int64_t is being dropped to an int type
    timeout_ms = timeout/NS_PER_MS; // int64_t is being dropped to an int type

    if(flags & SENSORS_BATCH_DRY_RUN) {
        dryRun = true;
    }

    if (uint32_t(what) >= CW_SENSORS_ID_END) {
        return -EINVAL;
    }

    if(is_batch_wake_sensor(handle)) {
        flags |= SENSORS_BATCH_WAKE_UPON_FIFO_FULL;
        ALOGV("CwMcuSensor::batch: SENSORS_BATCH_WAKE_UPON_FIFO_FULL~!!\n");
    } else
        flags &= ~SENSORS_BATCH_WAKE_UPON_FIFO_FULL;

    switch (what) {
    case CW_LIGHT:
    case CW_SIGNIFICANT_MOTION:
        if (timeout > 0) {
            ALOGI("CwMcuSensor::batch: handle = %d, not support batch mode", handle);
            return -EINVAL;
        }
        break;
    default:
        break;
    }

    if (dryRun == true) {
        ALOGV("CwMcuSensor::batch: SENSORS_BATCH_DRY_RUN is set\n");
        return 0;
    }

    ALOGV("%s: Before pthread_mutex_lock()\n", __func__);
    pthread_mutex_lock(&sys_fs_mutex);
    ALOGV("%s: Acquired pthread_mutex_lock()\n", __func__);

    if (mEnabled.isEmpty()) {
        int i;
        int iio_buf_size;

        if (!init_trigger_done) {
            err = sysfs_set_input_attr("trigger/current_trigger",
                                      mTriggerName, strlen(mTriggerName));
            if (err < 0) {
                ALOGE("CwMcuSensor::batch: set current trigger failed: err = %d, strerr() = %s\n",
                      err, strerror(errno));
            } else {
                init_trigger_done = true;
            }
        }

        iio_buf_size = IIO_MAX_BUFF_SIZE;
        for (i = 0; i < IIO_BUF_SIZE_RETRY; i++) {
            if (sysfs_set_input_attr_by_int("buffer/length", iio_buf_size) < 0) {
                ALOGE("CwMcuSensor::batch: set IIO buffer length (%d) failed: %s\n",
                      iio_buf_size, strerror(errno));
            } else {
                if (sysfs_set_input_attr_by_int("buffer/enable", 1) < 0) {
                    ALOGE("CwMcuSensor::batch: set IIO buffer enable failed: %s, i = %d, "
                          "iio_buf_size = %d\n", strerror(errno), i , iio_buf_size);
                } else {
                    ALOGI("CwMcuSensor::batch: set IIO buffer length = %d, success\n", iio_buf_size);
                    break;
                }
            }
            iio_buf_size /= 2;
        }
    }

    strcpy(&fixed_sysfs_path[fixed_sysfs_path_len], "batch_enable");

    fd = open(fixed_sysfs_path, O_RDWR);
    if (fd < 0) {
        err = -errno;
    } else {
        int n = snprintf(buf, sizeof(buf), "%d %d %d %d\n", what, flags, delay_ms, timeout_ms);
        err = write(fd, buf, min(n, sizeof(buf)));
        if (err < 0) {
            err = -errno;
        } else {
            err = 0;
        }
        close(fd);
    }
    pthread_mutex_unlock(&sys_fs_mutex);

    ALOGV("CwMcuSensor::batch: fd = %d, sensors_id = %d, flags = %d, delay_ms= %d,"
          " timeout_ms = %d, path = %s, err = %d\n",
          fd , what, flags, delay_ms, timeout_ms, fixed_sysfs_path, err);

    return err;
}


int CwMcuSensor::flush(int handle)
{
    int what;
    int fd;
    char buf[10] = {0};
    int err;

    what = find_sensor(handle);

    if (uint32_t(what) >= CW_SENSORS_ID_END) {
        return -EINVAL;
    }

    ALOGV("%s: Before pthread_mutex_lock()\n", __func__);
    pthread_mutex_lock(&sys_fs_mutex);
    ALOGV("%s: Acquired pthread_mutex_lock()\n", __func__);

    strcpy(&fixed_sysfs_path[fixed_sysfs_path_len], "flush");

    fd = open(fixed_sysfs_path, O_RDWR);
    if (fd >= 0) {
        int n = snprintf(buf, sizeof(buf), "%d\n", what);
        err = write(fd, buf, min(n, sizeof(buf)));
        if (err < 0) {
            err = -errno;
        } else {
            err = 0;
        }
        close(fd);
    } else {
        ALOGI("CwMcuSensor::flush: flush not supported\n");
        err = -EINVAL;
    }

    pthread_mutex_unlock(&sys_fs_mutex);
    ALOGI("CwMcuSensor::flush: fd = %d, sensors_id = %d, path = %s, err = %d\n",
          fd, what, fixed_sysfs_path, err);
    return err;
}


bool CwMcuSensor::hasPendingEvents() const {
    return !mPendingMask.isEmpty();
}

int CwMcuSensor::setDelay(int32_t handle, int64_t delay_ns) {
    char buf[80];
    int fd;
    int what;
    int rc;

    ALOGV("%s: Before pthread_mutex_lock()\n", __func__);
    pthread_mutex_lock(&sys_fs_mutex);
    ALOGV("%s: Acquired pthread_mutex_lock()\n", __func__);

    ALOGV("CwMcuSensor::setDelay: handle = %" PRId32 ", delay_ns = %" PRId64 "\n",
            handle, delay_ns);

    what = find_sensor(handle);
    if (uint32_t(what) >= numSensors) {
        pthread_mutex_unlock(&sys_fs_mutex);
        return -EINVAL;
    }
    strcpy(&fixed_sysfs_path[fixed_sysfs_path_len], "delay_ms");
    fd = open(fixed_sysfs_path, O_RDWR);
    if (fd >= 0) {
        size_t n = snprintf(buf, sizeof(buf), "%d %lld\n", what, (delay_ns/NS_PER_MS));
        write(fd, buf, min(n, sizeof(buf)));
        close(fd);
    }

    pthread_mutex_unlock(&sys_fs_mutex);
    return 0;

}

void CwMcuSensor::calculate_rv_4th_element(int sensors_id) {
    switch (sensors_id) {
    case CW_ROTATIONVECTOR:
    case CW_GAME_ROTATION_VECTOR:
    case CW_GEOMAGNETIC_ROTATION_VECTOR:
    case CW_ROTATIONVECTOR_W:
    case CW_GAME_ROTATION_VECTOR_W:
    case CW_GEOMAGNETIC_ROTATION_VECTOR_W:
        float q0, q1, q2, q3;

        q1 = mPendingEvents[sensors_id].data[0];
        q2 = mPendingEvents[sensors_id].data[1];
        q3 = mPendingEvents[sensors_id].data[2];

        q0 = 1 - q1*q1 - q2*q2 - q3*q3;
        q0 = (q0 > 0) ? (float)sqrt(q0) : 0;

        mPendingEvents[sensors_id].data[3] = q0;
        break;
    default:
        break;
    }
}

int CwMcuSensor::readEvents(sensors_event_t* data, int count) {
    uint64_t mtimestamp;

    if (count < 1) {
        return -EINVAL;
    }

    ALOGD_IF(fill_block_debug == 1, "CwMcuSensor::readEvents: Before fill\n");
    ssize_t n = mInputReader.fill(data_fd);
    ALOGD_IF(fill_block_debug == 1, "CwMcuSensor::readEvents: After fill, n = %zd\n", n);
    if (n < 0) {
        return n;
    }

    cw_event const* event;
    uint8_t data_temp[24];
    int id;
    int numEventReceived = 0;

    while (count && mInputReader.readEvent(&event)) {

        memcpy(data_temp, event->data, sizeof(data_temp));

        id = processEvent(data_temp);
        if (id == CW_META_DATA) {
            *data++ = mPendingEventsFlush;
            count--;
            numEventReceived++;
            ALOGV("CwMcuSensor::readEvents: metadata = %d\n", mPendingEventsFlush.meta_data.sensor);
        } else if ((id == TIME_DIFF_EXHAUSTED) || (id == CW_TIME_BASE)) {
            ALOGV("readEvents: id = %d\n", id);
        } else {
            /*** The algorithm which parsed mcu_time into cpu_time for each event ***/
            uint64_t event_mcu_time = mPendingEvents[id].timestamp;
            uint64_t event_cpu_time;

            if (event_mcu_time < last_mcu_timestamp[id]) {
                ALOGE("Do syncronization due to wrong delta mcu_timestamp\n");
                ALOGE("curr_ts = %" PRIu64 " ns, last_ts = %" PRIu64 " ns",
                    event_mcu_time, last_mcu_timestamp[id]);
                sync_time_thread_in_class();
            }

            pthread_mutex_lock(&sync_timestamp_algo_mutex);

            if (offset_reset[id]) {
                ALOGV("offset changed, id = %d, offset = %" PRId64 "\n", id, time_offset);
                offset_reset[id] = false;
                event_cpu_time = event_mcu_time + time_offset;
            } else {
                int64_t event_mcu_diff = (event_mcu_time - last_mcu_timestamp[id]);
                int64_t event_cpu_diff = event_mcu_diff * time_slope;
                event_cpu_time = last_cpu_timestamp[id] + event_cpu_diff;
            }
            pthread_mutex_unlock(&sync_timestamp_algo_mutex);

            pthread_mutex_lock(&last_timestamp_mutex);

            mtimestamp = getTimestamp();
            ALOGV("readEvents: id = %d, accuracy = %d\n"
                  , id
                  , mPendingEvents[id].acceleration.status);
            ALOGV("readEvents: id = %d,"
                  " mcu_time = %" PRId64 " ms,"
                  " cpu_time = %" PRId64 " ns,"
                  " delta = %" PRId64 " us,"
                  " HALtime = %" PRId64 " ns\n",
                  id,
                  event_mcu_time / NS_PER_MS,
                  event_cpu_time,
                  (event_cpu_time - last_cpu_timestamp[id]) / NS_PER_US,
                  mtimestamp);
            event_cpu_time = (mtimestamp > event_cpu_time) ? event_cpu_time : mtimestamp;
            last_mcu_timestamp[id] = event_mcu_time;
            last_cpu_timestamp[id] = event_cpu_time;
            pthread_mutex_unlock(&last_timestamp_mutex);
            /*** The algorithm which parsed mcu_time into cpu_time for each event ***/

            mPendingEvents[id].timestamp = event_cpu_time;

            if (mEnabled.hasBit(id)) {
                if (id == CW_SIGNIFICANT_MOTION) {
                    setEnable(ID_CW_SIGNIFICANT_MOTION, 0);
                }
                calculate_rv_4th_element(id);
                *data++ = mPendingEvents[id];
                count--;
                numEventReceived++;
            }
        }

        mInputReader.next();
    }
    return numEventReceived;
}


int CwMcuSensor::processEvent(uint8_t *event) {
    int sensorsid = 0;
    int16_t data[3];
    int16_t bias[3];
    int64_t time;

    sensorsid = (int)event[0];
    memcpy(data, &event[1], 6);
    memcpy(bias, &event[7], 6);
    memcpy(&time, &event[13], 8);

    mPendingEvents[sensorsid].timestamp = time * NS_PER_MS;

    switch (sensorsid) {
    case CW_ORIENTATION:
    case CW_ORIENTATION_W:
        mPendingMask.markBit(sensorsid);
        if ((sensorsid == CW_ORIENTATION) || (sensorsid == CW_ORIENTATION_W)) {
            mPendingEvents[sensorsid].orientation.status = bias[0];
        }
        mPendingEvents[sensorsid].data[0] = (float)data[0] * CONVERT_10;
        mPendingEvents[sensorsid].data[1] = (float)data[1] * CONVERT_10;
        mPendingEvents[sensorsid].data[2] = (float)data[2] * CONVERT_10;
        break;
    case CW_ACCELERATION:
    case CW_MAGNETIC:
    case CW_GYRO:
    case CW_LINEARACCELERATION:
    case CW_GRAVITY:
    case CW_ACCELERATION_W:
    case CW_MAGNETIC_W:
    case CW_GYRO_W:
    case CW_LINEARACCELERATION_W:
    case CW_GRAVITY_W:
        mPendingMask.markBit(sensorsid);
        if ((sensorsid == CW_MAGNETIC) || (sensorsid == CW_MAGNETIC_W)) {
            mPendingEvents[sensorsid].magnetic.status = bias[0];
            ALOGV("CwMcuSensor::processEvent: magnetic accuracy = %d\n",
                  mPendingEvents[sensorsid].magnetic.status);
        }
        mPendingEvents[sensorsid].data[0] = (float)data[0] * CONVERT_100;
        mPendingEvents[sensorsid].data[1] = (float)data[1] * CONVERT_100;
        mPendingEvents[sensorsid].data[2] = (float)data[2] * CONVERT_100;
        break;
    case CW_PRESSURE:
    case CW_PRESSURE_W:
        mPendingMask.markBit(sensorsid);
        // .pressure is data[0] and the unit is hectopascal (hPa)
        mPendingEvents[sensorsid].pressure = ((float)*(int32_t *)(&data[0])) * CONVERT_100;
        // data[1] is not used, and data[2] is the temperature
        mPendingEvents[sensorsid].data[2] = ((float)data[2]) * CONVERT_100;
        break;
    case CW_ROTATIONVECTOR:
    case CW_GAME_ROTATION_VECTOR:
    case CW_GEOMAGNETIC_ROTATION_VECTOR:
    case CW_ROTATIONVECTOR_W:
    case CW_GAME_ROTATION_VECTOR_W:
    case CW_GEOMAGNETIC_ROTATION_VECTOR_W:
        mPendingMask.markBit(sensorsid);
        mPendingEvents[sensorsid].data[0] = (float)data[0] * CONVERT_10000;
        mPendingEvents[sensorsid].data[1] = (float)data[1] * CONVERT_10000;
        mPendingEvents[sensorsid].data[2] = (float)data[2] * CONVERT_10000;
        break;
    case CW_MAGNETIC_UNCALIBRATED:
    case CW_GYROSCOPE_UNCALIBRATED:
    case CW_MAGNETIC_UNCALIBRATED_W:
    case CW_GYROSCOPE_UNCALIBRATED_W:
        mPendingMask.markBit(sensorsid);
        mPendingEvents[sensorsid].data[0] = (float)data[0] * CONVERT_100;
        mPendingEvents[sensorsid].data[1] = (float)data[1] * CONVERT_100;
        mPendingEvents[sensorsid].data[2] = (float)data[2] * CONVERT_100;
        mPendingEvents[sensorsid].data[3] = (float)bias[0] * CONVERT_100;
        mPendingEvents[sensorsid].data[4] = (float)bias[1] * CONVERT_100;
        mPendingEvents[sensorsid].data[5] = (float)bias[2] * CONVERT_100;
        break;
    case CW_SIGNIFICANT_MOTION:
        mPendingMask.markBit(sensorsid);
        mPendingEvents[sensorsid].data[0] = 1.0;
        ALOGV("SIGNIFICANT timestamp = %" PRIu64 "\n", mPendingEvents[sensorsid].timestamp);
        break;
    case CW_LIGHT:
        mPendingMask.markBit(sensorsid);
        mPendingEvents[sensorsid].light = indexToValue(data[0]);
        break;
    case CW_STEP_DETECTOR:
    case CW_STEP_DETECTOR_W:
        mPendingMask.markBit(sensorsid);
        mPendingEvents[sensorsid].data[0] = data[0];
        ALOGV("STEP_DETECTOR, timestamp = %" PRIu64 "\n", mPendingEvents[sensorsid].timestamp);
        break;
    case CW_STEP_COUNTER:
    case CW_STEP_COUNTER_W:
        mPendingMask.markBit(sensorsid);
        // We use 4 bytes in SensorHUB
        mPendingEvents[sensorsid].u64.step_counter = *(uint32_t *)&data[0];
        mPendingEvents[sensorsid].u64.step_counter += 0x100000000LL * (*(uint32_t *)&bias[0]);
        ALOGV("processEvent: step counter = %" PRId64 "\n",
              mPendingEvents[sensorsid].u64.step_counter);
        break;
    case CW_META_DATA:
        mPendingEventsFlush.meta_data.what = META_DATA_FLUSH_COMPLETE;
        mPendingEventsFlush.meta_data.sensor = find_handle(data[0]);
        ALOGV("CW_META_DATA: meta_data.sensor = %d, data[0] = %d\n",
              mPendingEventsFlush.meta_data.sensor, data[0]);
        break;
    default:
        ALOGW("%s: Unknown sensorsid = %d\n", __func__, sensorsid);
        break;
    }

    return sensorsid;
}


void CwMcuSensor::cw_save_calibrator_file(int type, const char * path, int* str) {
    FILE *fp_file;
    int i;
    int rc;

    ALOGV("CwMcuSensor::cw_save_calibrator_file: path = %s\n", path);

    fp_file = fopen(path, "w+");
    if (!fp_file) {
        ALOGE("CwMcuSensor::cw_save_calibrator_file: open file '%s' failed: %s\n",
              path, strerror(errno));
        return;
    }

    if ((type == CW_GYRO) || (type == CW_ACCELERATION)) {
        fprintf(fp_file, "%d %d %d\n", str[0], str[1], str[2]);
    } else if(type == CW_MAGNETIC) {
        for (i = 0; i < COMPASS_CALIBRATION_DATA_SIZE; i++) {
            ALOGV("CwMcuSensor::cw_save_calibrator_file: str[%d] = %d\n", i, str[i]);
            rc = fprintf(fp_file, "%d%c", str[i], (i == (COMPASS_CALIBRATION_DATA_SIZE-1)) ? '\n' : ' ');
            if (rc < 0) {
                ALOGE("CwMcuSensor::cw_save_calibrator_file: fprintf fails, rc = %d\n", rc);
            }
        }
    }

    fclose(fp_file);
    return;
}

int CwMcuSensor::cw_read_calibrator_file(int type, const char * path, int* str) {
    FILE *fp;
    int readBytes;
    int data[COMPASS_CALIBRATION_DATA_SIZE] = {0};
    unsigned int i;
    int my_errno;

    ALOGV("CwMcuSensor::cw_read_calibrator_file: path = %s\n", path);

    fp = fopen(path, "r");
    if (!fp) {
        ALOGE("CwMcuSensor::cw_read_calibrator_file: open file '%s' failed: %s\n",
              path, strerror(errno));
        // errno is reset to 0 before return
        return -1;
    }

    if (type == CW_GYRO || type == CW_ACCELERATION) {
        readBytes = fscanf(fp, "%d %d %d\n", &str[0], &str[1], &str[2]);
        my_errno = errno;
        if (readBytes != 3) {
            ALOGE("CwMcuSensor::cw_read_calibrator_file: fscanf3, readBytes = %d, strerror = %s\n", readBytes, strerror(my_errno));
        }

    } else if (type == CW_MAGNETIC) {
        ALOGV("CwMcuSensor::cw_read_calibrator_file: COMPASS_CALIBRATION_DATA_SIZE = %d\n", COMPASS_CALIBRATION_DATA_SIZE);
        // COMPASS_CALIBRATION_DATA_SIZE is 26
        for (i = 0; i < COMPASS_CALIBRATION_DATA_SIZE; i++) {
            readBytes = fscanf(fp, "%d ", &str[i]);
            my_errno = errno;
            ALOGV("CwMcuSensor::cw_read_calibrator_file: str[%d] = %d\n", i, str[i]);
            if (readBytes < 1) {
                ALOGE("CwMcuSensor::cw_read_calibrator_file: fscanf26, readBytes = %d, strerror = %s\n", readBytes, strerror(my_errno));
                fclose(fp);
                return readBytes;
            }
        }
    }
    fclose(fp);
    return 0;
}
