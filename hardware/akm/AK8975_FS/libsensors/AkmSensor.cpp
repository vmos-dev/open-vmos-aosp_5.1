/*
 * Copyright (C) 2008 The Android Open Source Project
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

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>
#include <dlfcn.h>

#include <cutils/log.h>

#include "AkmSensor.h"

#define AKMD_DEFAULT_INTERVAL	200000000

/*****************************************************************************/

AkmSensor::AkmSensor()
: SensorBase(NULL, "compass"),
      mPendingMask(0),
      mInputReader(32)
{
	for (int i=0; i<numSensors; i++) {
		mEnabled[i] = 0;
		mDelay[i] = -1;
	}
    memset(mPendingEvents, 0, sizeof(mPendingEvents));

    mPendingEvents[Accelerometer].version = sizeof(sensors_event_t);
    mPendingEvents[Accelerometer].sensor = ID_A;
    mPendingEvents[Accelerometer].type = SENSOR_TYPE_ACCELEROMETER;
    mPendingEvents[Accelerometer].acceleration.status = SENSOR_STATUS_ACCURACY_HIGH;

    mPendingEvents[MagneticField].version = sizeof(sensors_event_t);
    mPendingEvents[MagneticField].sensor = ID_M;
    mPendingEvents[MagneticField].type = SENSOR_TYPE_MAGNETIC_FIELD;
    mPendingEvents[MagneticField].magnetic.status = SENSOR_STATUS_ACCURACY_HIGH;

    mPendingEvents[Orientation  ].version = sizeof(sensors_event_t);
    mPendingEvents[Orientation  ].sensor = ID_O;
    mPendingEvents[Orientation  ].type = SENSOR_TYPE_ORIENTATION;
    mPendingEvents[Orientation  ].orientation.status = SENSOR_STATUS_ACCURACY_HIGH;

    if (data_fd) {
		strcpy(input_sysfs_path, "/sys/class/compass/akm8975/");
		input_sysfs_path_len = strlen(input_sysfs_path);
	} else {
		input_sysfs_path[0] = '\0';
		input_sysfs_path_len = 0;
	}
}

AkmSensor::~AkmSensor()
{
	for (int i=0; i<numSensors; i++) {
		setEnable(i, 0);
	}
}

int AkmSensor::setEnable(int32_t handle, int enabled)
{
	int id = handle2id(handle);
	int err = 0;
	char buffer[2];

	switch (id) {
	case Accelerometer:
		strcpy(&input_sysfs_path[input_sysfs_path_len], "enable_acc");
		break;
	case MagneticField:
		strcpy(&input_sysfs_path[input_sysfs_path_len], "enable_mag");
		break;
	case Orientation:
		strcpy(&input_sysfs_path[input_sysfs_path_len], "enable_ori");
		break;
	default:
		ALOGE("AkmSensor: unknown handle (%d)", handle);
		return -EINVAL;
	}

	buffer[0] = '\0';
	buffer[1] = '\0';

	if (mEnabled[id] <= 0) {
		if(enabled) buffer[0] = '1';
	} else if (mEnabled[id] == 1) {
		if(!enabled) buffer[0] = '0';
	}

    if (buffer[0] != '\0') {
		err = write_sys_attribute(input_sysfs_path, buffer, 1);
		if (err != 0) {
			return err;
		}
		ALOGD("AkmSensor: set %s to %s",
			&input_sysfs_path[input_sysfs_path_len], buffer);

		/* for AKMD specification */
		if (buffer[0] == '1') {
			setDelay(handle, AKMD_DEFAULT_INTERVAL);
		} else {
			setDelay(handle, -1);
		}
    }

	if (enabled) {
		(mEnabled[id])++;
		if (mEnabled[id] > 32767) mEnabled[id] = 32767;
	} else {
		(mEnabled[id])--;
		if (mEnabled[id] < 0) mEnabled[id] = 0;
	}
	ALOGD("AkmSensor: mEnabled[%d] = %d", id, mEnabled[id]);

    return err;
}

int AkmSensor::setDelay(int32_t handle, int64_t ns)
{
	int id = handle2id(handle);
	int err = 0;
	char buffer[32];
	int bytes;

    if (ns < -1 || 2147483647 < ns) {
		ALOGE("AkmSensor: invalid delay (%lld)", ns);
        return -EINVAL;
	}

    switch (id) {
        case Accelerometer:
			strcpy(&input_sysfs_path[input_sysfs_path_len], "delay_acc");
			break;
        case MagneticField:
			strcpy(&input_sysfs_path[input_sysfs_path_len], "delay_mag");
			break;
        case Orientation:
			strcpy(&input_sysfs_path[input_sysfs_path_len], "delay_ori");
			break;
		default:
			ALOGE("AkmSensor: unknown handle (%d)", handle);
			return -EINVAL;
    }

	if (ns != mDelay[id]) {
   		bytes = sprintf(buffer, "%lld", ns);
		err = write_sys_attribute(input_sysfs_path, buffer, bytes);
		if (err == 0) {
			mDelay[id] = ns;
			ALOGD("AkmSensor: set %s to %f ms.",
				&input_sysfs_path[input_sysfs_path_len], ns/1000000.0f);
		}
	}

    return err;
}

int64_t AkmSensor::getDelay(int32_t handle)
{
	int id = handle2id(handle);
	if (id > 0) {
		return mDelay[id];
	} else {
		return 0;
	}
}

int AkmSensor::getEnable(int32_t handle)
{
	int id = handle2id(handle);
	if (id >= 0) {
		return mEnabled[id];
	} else {
		return 0;
	}
}

int AkmSensor::readEvents(sensors_event_t* data, int count)
{
    if (count < 1)
        return -EINVAL;

    ssize_t n = mInputReader.fill(data_fd);
    if (n < 0)
        return n;

    int numEventReceived = 0;
    input_event const* event;

    while (count && mInputReader.readEvent(&event)) {
        int type = event->type;
        if (type == EV_ABS) {
            processEvent(event->code, event->value);
            mInputReader.next();
        } else if (type == EV_SYN) {
            int64_t time = timevalToNano(event->time);
            for (int j=0 ; count && mPendingMask && j<numSensors ; j++) {
                if (mPendingMask & (1<<j)) {
                    mPendingMask &= ~(1<<j);
                    mPendingEvents[j].timestamp = time;
					//ALOGD("data=%8.5f,%8.5f,%8.5f",
						//mPendingEvents[j].data[0],
						//mPendingEvents[j].data[1],
						//mPendingEvents[j].data[2]);
                    if (mEnabled[j]) {
                        *data++ = mPendingEvents[j];
                        count--;
                        numEventReceived++;
                    }
                }
            }
            if (!mPendingMask) {
                mInputReader.next();
            }
        } else {
            ALOGE("AkmSensor: unknown event (type=%d, code=%d)",
                    type, event->code);
            mInputReader.next();
        }
    }
    return numEventReceived;
}

int AkmSensor::setAccel(sensors_event_t* data)
{
	int err;
	int16_t acc[3];

	acc[0] = (int16_t)(data->acceleration.x / GRAVITY_EARTH * AKSC_LSG);
	acc[1] = (int16_t)(data->acceleration.y / GRAVITY_EARTH * AKSC_LSG);
	acc[2] = (int16_t)(data->acceleration.z / GRAVITY_EARTH * AKSC_LSG);

	strcpy(&input_sysfs_path[input_sysfs_path_len], "accel");
	err = write_sys_attribute(input_sysfs_path, (char*)acc, 6);
	if (err < 0) {
		ALOGD("AkmSensor: %s write failed.",
			&input_sysfs_path[input_sysfs_path_len]);
	}
	return err;
}

int AkmSensor::handle2id(int32_t handle)
{
    switch (handle) {
        case ID_A:
			return Accelerometer;
        case ID_M:
			return MagneticField;
        case ID_O:
			return Orientation;
		default:
			ALOGE("AkmSensor: unknown handle (%d)", handle);
			return -EINVAL;
    }
}

void AkmSensor::processEvent(int code, int value)
{
    switch (code) {
        case EVENT_TYPE_ACCEL_X:
            mPendingMask |= 1<<Accelerometer;
            mPendingEvents[Accelerometer].acceleration.x = value * CONVERT_A;
            break;
        case EVENT_TYPE_ACCEL_Y:
            mPendingMask |= 1<<Accelerometer;
            mPendingEvents[Accelerometer].acceleration.y = value * CONVERT_A;
            break;
        case EVENT_TYPE_ACCEL_Z:
            mPendingMask |= 1<<Accelerometer;
            mPendingEvents[Accelerometer].acceleration.z = value * CONVERT_A;
            break;

        case EVENT_TYPE_MAGV_X:
            mPendingMask |= 1<<MagneticField;
            mPendingEvents[MagneticField].magnetic.x = value * CONVERT_M;
            break;
        case EVENT_TYPE_MAGV_Y:
            mPendingMask |= 1<<MagneticField;
            mPendingEvents[MagneticField].magnetic.y = value * CONVERT_M;
            break;
        case EVENT_TYPE_MAGV_Z:
            mPendingMask |= 1<<MagneticField;
            mPendingEvents[MagneticField].magnetic.z = value * CONVERT_M;
            break;
        case EVENT_TYPE_MAGV_STATUS:
            mPendingMask |= 1<<MagneticField;
            mPendingEvents[MagneticField].magnetic.status = value;
            break;

        case EVENT_TYPE_YAW:
            mPendingMask |= 1<<Orientation;
            mPendingEvents[Orientation].orientation.azimuth = value * CONVERT_O;
            break;
        case EVENT_TYPE_PITCH:
            mPendingMask |= 1<<Orientation;
            mPendingEvents[Orientation].orientation.pitch = value * CONVERT_O;
            break;
        case EVENT_TYPE_ROLL:
            mPendingMask |= 1<<Orientation;
            mPendingEvents[Orientation].orientation.roll = value * CONVERT_O;
            break;
        case EVENT_TYPE_ORIENT_STATUS:
            mPendingMask |= 1<<Orientation;
            mPendingEvents[Orientation].orientation.status = value;
            break;
    }
}
