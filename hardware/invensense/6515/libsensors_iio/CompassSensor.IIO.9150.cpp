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

#define LOG_NDEBUG 0

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>
#include <cutils/log.h>
#include <linux/input.h>
#include <string.h>

#include "CompassSensor.IIO.9150.h"
#include "sensors.h"
#include "MPLSupport.h"
#include "sensor_params.h"
#include "ml_sysfs_helper.h"

#define COMPASS_MAX_SYSFS_ATTRB sizeof(compassSysFs) / sizeof(char*)
#define COMPASS_NAME "USE_SYSFS"

#if defined COMPASS_YAS53x
#pragma message("HAL:build Invensense compass cal with YAS53x IIO on secondary bus")
#define USE_MPL_COMPASS_HAL (1)
#define COMPASS_NAME        "INV_YAS530"

#elif defined COMPASS_AK8975
#pragma message("HAL:build Invensense compass cal with AK8975 on primary bus")
#define USE_MPL_COMPASS_HAL (1)
#define COMPASS_NAME        "INV_AK8975"

#elif defined INVENSENSE_COMPASS_CAL
#   define COMPASS_NAME                 "USE_SYSFS"
#pragma message("HAL:build Invensense compass cal with compass IIO on secondary bus")
#define USE_MPL_COMPASS_HAL (1)
#else
#pragma message("HAL:build third party compass cal HAL")
#define USE_MPL_COMPASS_HAL (0)
// TODO: change to vendor's name
#define COMPASS_NAME        "AKM8975"

#endif

/*****************************************************************************/

CompassSensor::CompassSensor()
                  : SensorBase(NULL, NULL),
                    compass_fd(-1),
                    mCompassTimestamp(0),
                    mCompassInputReader(8)
{
    VFUNC_LOG;

    if(!strcmp(COMPASS_NAME, "USE_SYSFS")) {
        int result = find_name_by_sensor_type("in_magn_scale", "iio:device", 
                                              sensor_name);
        if(result) {
            LOGE("HAL:Cannot read secondary device name - (%d)", result);
        }
        dev_name = sensor_name;
    }
    LOGI_IF(PROCESS_VERBOSE, "HAL:Secondary Chip Id: %s", dev_name);

    if(inv_init_sysfs_attributes()) {
        LOGE("Error Instantiating Compass\n");
        return;
    }

    memset(mCachedCompassData, 0, sizeof(mCachedCompassData));

    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:cat %s (%lld)", 
            compassSysFs.compass_orient, getTimestamp());
    FILE *fptr;
    fptr = fopen(compassSysFs.compass_orient, "r");
    if (fptr != NULL) {
        int om[9];
        if (fscanf(fptr, "%d,%d,%d,%d,%d,%d,%d,%d,%d", 
               &om[0], &om[1], &om[2], &om[3], &om[4], &om[5],
               &om[6], &om[7], &om[8]) < 0 || fclose(fptr) < 0) {
            LOGE("HAL:Could not read compass mounting matrix");
        } else {
            LOGV_IF(EXTRA_VERBOSE, "HAL:compass mounting matrix: "
                    "%+d %+d %+d %+d %+d %+d %+d %+d %+d", om[0], om[1], om[2], 
                    om[3], om[4], om[5], om[6], om[7], om[8]);
            mCompassOrientation[0] = om[0];
            mCompassOrientation[1] = om[1];
            mCompassOrientation[2] = om[2];
            mCompassOrientation[3] = om[3];
            mCompassOrientation[4] = om[4];
            mCompassOrientation[5] = om[5];
            mCompassOrientation[6] = om[6];
            mCompassOrientation[7] = om[7];
            mCompassOrientation[8] = om[8];
        }
    }

    if (!isIntegrated()) {
        enable(ID_M, 0);
    }
}

CompassSensor::~CompassSensor()
{
    VFUNC_LOG;
    free(pathP);
    if( compass_fd > 0)
        close(compass_fd);
}

int CompassSensor::getFd() const
{
    VFUNC_LOG;
    return compass_fd;
}

/**
 *  @brief        This function will enable/disable sensor.
 *  @param[in]    handle
 *                  which sensor to enable/disable.
 *  @param[in]    en
 *                  en=1, enable; 
 *                  en=0, disable
 *  @return       if the operation is successful.
 */
int CompassSensor::enable(int32_t handle, int en) 
{
    VFUNC_LOG;
    int res = 0;
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
            en, compassSysFs.compass_enable, getTimestamp());
    res = write_sysfs_int(compassSysFs.compass_enable, en);
    return res;
}

int CompassSensor::setDelay(int32_t handle, int64_t ns) 
{
    VFUNC_LOG;
    int tempFd;
    int res;

    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %.0f > %s (%lld)", 
            1000000000.f / ns, compassSysFs.compass_rate, getTimestamp());
    mDelay = ns;
    if (ns == 0)
        return -1;
    tempFd = open(compassSysFs.compass_rate, O_RDWR);
    res = write_attribute_sensor(tempFd, 1000000000.f / ns);
    if(res < 0) {
        LOGE("HAL:Compass update delay error");
    }
    return res;
}

int CompassSensor::turnOffCompassFifo(void)
{
    int i, res = 0, tempFd;
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                        0, compassSysFs.compass_fifo_enable, getTimestamp());
    res += write_sysfs_int(compassSysFs.compass_fifo_enable, 0);
    return res;
}

int CompassSensor::turnOnCompassFifo(void)
{
    int i, res = 0, tempFd;
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:echo %d > %s (%lld)",
                        1, compassSysFs.compass_fifo_enable, getTimestamp());
    res += write_sysfs_int(compassSysFs.compass_fifo_enable, 1);
    return res;
}

/**
    @brief      This function will return the state of the sensor.
    @return     1=enabled; 0=disabled
**/
int CompassSensor::getEnable(int32_t handle)
{
    VFUNC_LOG;
    return mEnable;
}

/* use for Invensense compass calibration */
#define COMPASS_EVENT_DEBUG (0)
void CompassSensor::processCompassEvent(const input_event *event)
{
    VHANDLER_LOG;

    switch (event->code) {
    case EVENT_TYPE_ICOMPASS_X:
        LOGV_IF(COMPASS_EVENT_DEBUG, "EVENT_TYPE_ICOMPASS_X\n");
        mCachedCompassData[0] = event->value;
        break;
    case EVENT_TYPE_ICOMPASS_Y:
        LOGV_IF(COMPASS_EVENT_DEBUG, "EVENT_TYPE_ICOMPASS_Y\n");
        mCachedCompassData[1] = event->value;
        break;
    case EVENT_TYPE_ICOMPASS_Z:
        LOGV_IF(COMPASS_EVENT_DEBUG, "EVENT_TYPE_ICOMPASS_Z\n");
        mCachedCompassData[2] = event->value;
        break;
    }
    
    mCompassTimestamp = 
        (int64_t)event->time.tv_sec * 1000000000L + event->time.tv_usec * 1000L;
}

void CompassSensor::getOrientationMatrix(signed char *orient)
{
    VFUNC_LOG;
    memcpy(orient, mCompassOrientation, sizeof(mCompassOrientation));
}

long CompassSensor::getSensitivity()
{
    VFUNC_LOG;

    long sensitivity;
    LOGV_IF(SYSFS_VERBOSE, "HAL:sysfs:cat %s (%lld)", 
            compassSysFs.compass_scale, getTimestamp());
    inv_read_data(compassSysFs.compass_scale, &sensitivity);
    return sensitivity;
}

/**
    @brief         This function is called by sensors_mpl.cpp
                   to read sensor data from the driver.
    @param[out]    data      sensor data is stored in this variable. Scaled such that
                             1 uT = 2^16
    @para[in]      timestamp data's timestamp
    @return        1, if 1   sample read, 0, if not, negative if error
 */
int CompassSensor::readSample(long *data, int64_t *timestamp)
{
    VHANDLER_LOG;

    int numEventReceived = 0, done = 0;

    ssize_t n = mCompassInputReader.fill(compass_fd);
    if (n < 0) {
        LOGE("HAL:no compass events read");
        return n;
    }

    input_event const* event;

    while (done == 0 && mCompassInputReader.readEvent(&event)) {
        int type = event->type;
        if (type == EV_REL) {
            processCompassEvent(event);
        } else if (type == EV_SYN) {
            *timestamp = mCompassTimestamp;
            memcpy(data, mCachedCompassData, sizeof(mCachedCompassData));
            done = 1;
        } else {
            LOGE("HAL:Compass Sensor: unknown event (type=%d, code=%d)",
                 type, event->code);
        }
        mCompassInputReader.next();
    }

    return done;
}

/**
 *  @brief  This function will return the current delay for this sensor.
 *  @return delay in nanoseconds. 
 */
int64_t CompassSensor::getDelay(int32_t handle)
{
    VFUNC_LOG;
    return mDelay;
}

void CompassSensor::fillList(struct sensor_t *list)
{
    VFUNC_LOG;

    const char *compass = sensor_name;

    if (compass) {
        if(!strcmp(compass, "INV_COMPASS")) {
            list->maxRange = COMPASS_MPU9150_RANGE;
            list->resolution = COMPASS_MPU9150_RESOLUTION;
            list->power = COMPASS_MPU9150_POWER;
            list->minDelay = COMPASS_MPU9150_MINDELAY;
            return;
        }
        if(!strcmp(compass, "compass")
                || !strcmp(compass, "INV_AK8975")
                || !strcmp(compass, "AK8975")
                || !strcmp(compass, "ak8975")) {
            list->maxRange = COMPASS_AKM8975_RANGE;
            list->resolution = COMPASS_AKM8975_RESOLUTION;
            list->power = COMPASS_AKM8975_POWER;
            list->minDelay = COMPASS_AKM8975_MINDELAY;
            return;
        }
        if(!strcmp(compass, "compass")
                || !strcmp(compass, "INV_AK8963")
                || !strcmp(compass, "AK8963")
                || !strcmp(compass, "ak8963")) {
            list->maxRange = COMPASS_AKM8963_RANGE;
            list->resolution = COMPASS_AKM8963_RESOLUTION;
            list->power = COMPASS_AKM8963_POWER;
            list->minDelay = COMPASS_AKM8963_MINDELAY;
            return;
        }
        if(!strcmp(compass, "compass")
                || !strcmp(compass, "INV_AK09911")
                || !strcmp(compass, "AK09911")
                || !strcmp(compass, "ak09911")) {
            list->maxRange = COMPASS_AKM9911_RANGE;
            list->resolution = COMPASS_AKM9911_RESOLUTION;
            list->power = COMPASS_AKM9911_POWER;
            list->minDelay = COMPASS_AKM9911_MINDELAY;
            return;
        }
        if(!strcmp(compass, "compass")
                || !strncmp(compass, "mlx90399",3)
                || !strncmp(compass, "MLX90399",3)) {
            list->maxRange = COMPASS_MPU9350_RANGE;
            list->resolution = COMPASS_MPU9350_RESOLUTION;
            list->power = COMPASS_MPU9350_POWER;
            list->minDelay = COMPASS_MPU9350_MINDELAY;
            return;
        }
        if(!strcmp(compass, "INV_YAS530")) {
            list->maxRange = COMPASS_YAS53x_RANGE;
            list->resolution = COMPASS_YAS53x_RESOLUTION;
            list->power = COMPASS_YAS53x_POWER;
            list->minDelay = COMPASS_YAS53x_MINDELAY;
            return;
        }
        if(!strcmp(compass, "INV_AMI306")) {
            list->maxRange = COMPASS_AMI306_RANGE;
            list->resolution = COMPASS_AMI306_RESOLUTION;
            list->power = COMPASS_AMI306_POWER;
            list->minDelay = COMPASS_AMI306_MINDELAY;
            return;
        }
    }
    LOGE("HAL:unknown compass id %s -- "
         "params default to ak8975 and might be wrong.",
         compass);
    list->maxRange = COMPASS_AKM8975_RANGE;
    list->resolution = COMPASS_AKM8975_RESOLUTION;
    list->power = COMPASS_AKM8975_POWER;
    list->minDelay = COMPASS_AKM8975_MINDELAY;
}

int CompassSensor::inv_init_sysfs_attributes(void)
{
    VFUNC_LOG;

    unsigned char i = 0;
    char sysfs_path[MAX_SYSFS_NAME_LEN];
    char iio_trigger_path[MAX_SYSFS_NAME_LEN], tbuf[2];
    char *sptr;
    char **dptr;
    int num;

    pathP = (char*)malloc(
                    sizeof(char[COMPASS_MAX_SYSFS_ATTRB][MAX_SYSFS_NAME_LEN]));
    sptr = pathP;
    dptr = (char**)&compassSysFs;
    if (sptr == NULL)
        return -1;

    memset(sysfs_path, 0, sizeof(sysfs_path));
    memset(iio_trigger_path, 0, sizeof(iio_trigger_path));

    do {
        *dptr++ = sptr;
        memset(sptr, 0, sizeof(sptr));
        sptr += sizeof(char[MAX_SYSFS_NAME_LEN]);
    } while (++i < COMPASS_MAX_SYSFS_ATTRB);

    // get proper (in absolute/relative) IIO path & build MPU's sysfs paths
    // inv_get_sysfs_abs_path(sysfs_path);
    inv_get_sysfs_path(sysfs_path);
    inv_get_iio_trigger_path(iio_trigger_path);

#if defined COMPASS_AK8975
    inv_get_input_number(dev_name, &num);
    tbuf[0] = num + 0x30;
    tbuf[1] = 0;
    sprintf(sysfs_path, "%s%s", "sys/class/input/input", tbuf);
    strcat(sysfs_path, "/ak8975");

    sprintf(compassSysFs.compass_enable, "%s%s", sysfs_path, "/enable");
    sprintf(compassSysFs.compass_rate, "%s%s", sysfs_path, "/rate");
    sprintf(compassSysFs.compass_scale, "%s%s", sysfs_path, "/scale");
    sprintf(compassSysFs.compass_orient, "%s%s", sysfs_path, "/compass_matrix");
#else
    sprintf(compassSysFs.compass_enable, "%s%s", sysfs_path, "/compass_enable");
    sprintf(compassSysFs.compass_fifo_enable, "%s%s", sysfs_path, "/compass_fifo_enable");
    sprintf(compassSysFs.compass_rate, "%s%s", sysfs_path, "/compass_rate");
    sprintf(compassSysFs.compass_scale, "%s%s", sysfs_path, "/in_magn_scale");
    sprintf(compassSysFs.compass_orient, "%s%s", sysfs_path, "/compass_matrix");
#endif

#if 0
    // test print sysfs paths   
    dptr = (char**)&compassSysFs;
    for (i = 0; i < COMPASS_MAX_SYSFS_ATTRB; i++) {
        LOGE("HAL:sysfs path: %s", *dptr++);
    }
#endif
    return 0;
}
