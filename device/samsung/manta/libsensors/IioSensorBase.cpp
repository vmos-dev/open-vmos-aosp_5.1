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

#define LOG_TAG "Sensors"

#include <fcntl.h>
#include <errno.h>
#include <cutils/log.h>
#include <pthread.h>

#include "IioSensorBase.h"

char *IioSensorBase::makeSysfsName(const char *input_name,
                                       const char *file_name) {
    char *name;
    int ret;

    ret = asprintf(&name, "/sys/bus/iio/devices/%s/%s", input_name, file_name);
    if (ret < 0)
        return NULL;

    return name;
}

void IioSensorBase::handleData(int value) {
}

IioSensorBase::IioSensorBase(const char *dev_name,
                                     const char *data_name,
                                     const char *enable_name,
                                     const char *chan_name,
                                     int iio_chan_type)
    : SensorBase(dev_name, data_name),
      mEnabled(true),
      mHasPendingEvent(false),
      mInputReader(MAX_BUFFER_FOR_EVENT),
      mIioChanType(iio_chan_type),
      mIioSysfsChanFp(NULL)
{
    pthread_mutex_init(&mLock, NULL);
    ALOGV("%s(): dev_name=%s", __func__, dev_name);
    mPendingEvent.version = sizeof(sensors_event_t);
    memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));
    if (!mDataFd)
        return;

    mInputSysfsEnable = makeSysfsName(mInputName, enable_name);
    if (!mInputSysfsEnable) {
        ALOGE("%s: unable to allocate mem for %s:%s", __func__,
             data_name, enable_name);
        return;
    }
    mInputSysfsSamplingFrequency = makeSysfsName(mInputName, "sampling_frequency");
    if (!mInputSysfsSamplingFrequency) {
        ALOGE("%s: unable to allocate mem for %s:poll_delay", __func__,
             data_name);
        return;
    }

    mIioSysfsChan = makeSysfsName(mInputName, chan_name);
    if (!mIioSysfsChan) {
        ALOGE("%s: unable to allocate mem for %s:%s", __func__, data_name,
             chan_name);
        return;
    }

    mIioSysfsChanFp = fopen(mIioSysfsChan, "r");
    if (mIioSysfsChanFp == NULL) {
        ALOGE("%s: unable to open %s", __func__, mIioSysfsChan);
        return;
    }

    int flags = fcntl(mDataFd, F_GETFL, 0);
    fcntl(mDataFd, F_SETFL, flags | O_NONBLOCK);

    enable(0, 0);
}

IioSensorBase::~IioSensorBase() {
    if (mEnabled) {
        enable(0, 0);
    }
    if (mIioSysfsChanFp != NULL) {
        fclose(mIioSysfsChanFp);
    }
    free(mInputSysfsEnable);
    free(mInputSysfsSamplingFrequency);
    free(mIioSysfsChan);
}

bool IioSensorBase::readValue(int *value) {

    bool ret = true;

    if (mIioSysfsChanFp == NULL)
        return false;

    if (fscanf(mIioSysfsChanFp, "%d\n", value) != 1)
        ret = false;

    fseek(mIioSysfsChanFp, 0, SEEK_SET);

    return ret;
}

int IioSensorBase::enable(int32_t handle, int en)
{
    int err = 0;

    ALOGI("%s: %s %d", __func__,  mDevName, en);

    pthread_mutex_lock(&mLock);
    if (en != mEnabled) {
        int fd;
        fd = open(mInputSysfsEnable, O_RDWR);
        if (fd >= 0) {
            err = write(fd, en ? "1" : "0", 2);
            close(fd);
            if (err < 0) {
                goto cleanup;
            }
            mEnabled = en;
            err = 0;
        } else {
            err = -1;
        }
    }
cleanup:
    pthread_mutex_unlock(&mLock);
    return err;
}

int IioSensorBase::setDelay(int32_t handle, int64_t ns)
{
    int fd;
    int result = 0;
    char buf[21]; /* 21 = log10(max long long int) + 1 for sign + '\0' */
    pthread_mutex_lock(&mLock);
    if (!ns) {
        result = -1;
        goto done;
    }
    fd = open(mInputSysfsSamplingFrequency, O_RDWR);
    if (fd < 0) {
        result = -1;
        goto done;
    }
    /* round up ((NSEC_PER_SEC - 1) + ns) / ns */
    sprintf(buf, "%lld", ((1000000000 - 1) + ns) / ns);
    write(fd, buf, strlen(buf)+1);
    close(fd);
done:
    pthread_mutex_unlock(&mLock);
    return result;
}

int IioSensorBase::readEvents(sensors_event_t* data, int count)
{
    if (count < 1)
        return -EINVAL;

    pthread_mutex_lock(&mLock);
    int numEventReceived = 0;

    if (mHasPendingEvent) {
        mHasPendingEvent = false;
        if (mEnabled) {
            mPendingEvent.timestamp = getTimestamp();
            *data = mPendingEvent;
            numEventReceived++;
        }
        goto done;
    }

    iio_event_data const* event;

    while (count && mInputReader.readEvent(mDataFd, &event)) {
        int value;
        if (IIO_EVENT_CODE_EXTRACT_CHAN_TYPE(event->id) == mIioChanType) {
            if (mEnabled && readValue(&value)) {
                handleData(value);
                mPendingEvent.timestamp = event->timestamp;
                *data++ = mPendingEvent;
                count--;
                numEventReceived++;
            }
        }
        mInputReader.next();
    }

done:
    pthread_mutex_unlock(&mLock);
    return numEventReceived;
}
