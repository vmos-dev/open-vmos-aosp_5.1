/*
 * Copyright (C) 2012 The Android Open Source Project
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

#define LOG_TAG "BubbleLevelImpl"
//#define LOG_NDEBUG 0

#include <utils/Log.h>
#include <binder/IServiceManager.h>
#include "BubbleLevelImpl.h"

namespace android {

static int sensor_callback(int fd, int events, void* data);

#define BL_SENSOR_POLL_INTERVAL_MS 20
#define BL_SENSOR_POLL_TIMEOUT_MS (BL_SENSOR_POLL_INTERVAL_MS * 5)
#define BL_SENSOR_POLL_COUNT 10
#define BL_SENSOR_LEVEL_THRESHOLD (2.0)

BubbleLevelImpl::BubbleLevelImpl()
    : Thread(false),
      mState(BL_STATE_IDLE), mCmd(BL_CMD_NONE),
      mPollIntervalSec(BL_POLL_INTERVAL_DEFAULT_SEC), mPollCount(0), mLevelCount(0),
      mCallBack(NULL), mUserData(NULL),
      mNumSensors(0), mAccelerometer(NULL),
      mInitStatus(-ENODEV)
{
    init();
}

int BubbleLevelImpl::init()
{
    if (mInitStatus == 0) {
        return 0;
    }

    if (defaultServiceManager()->checkService(String16("sensorservice")) == 0) {
        ALOGW("CSTOR sensorservice not ready yet");
        return mInitStatus;
    }

    SensorManager& mgr(SensorManager::getInstance());
    Sensor const* const* sensorList;

    mNumSensors = mgr.getSensorList(&sensorList);

    if (mNumSensors <= 0) {
        ALOGE("CSTOR mNumSensors error %d", mNumSensors);
        goto exit;
    }
    mAccelerometer = mgr.getDefaultSensor(Sensor::TYPE_ACCELEROMETER);
    if (mAccelerometer == NULL) {
        ALOGE("CSTOR mAccelerometer error NULL");
        goto exit;
    }

    mSensorEventQueue = mgr.createEventQueue();
    if (mSensorEventQueue == 0) {
        ALOGE("createEventQueue returned 0");
        goto exit;
    }

    mLooper = new Looper(false);
    mLooper->addFd(mSensorEventQueue->getFd(), 0, ALOOPER_EVENT_INPUT, sensor_callback, this);

    mInitStatus = 0;

exit:
    return mInitStatus;
}

BubbleLevelImpl::~BubbleLevelImpl()
{
    {
        Mutex::Autolock _l(mStateLock);
        mCmd = BL_CMD_EXIT;
        mLooper->wake();
        mCond.broadcast();
    }
    requestExitAndWait();
}

void BubbleLevelImpl::onFirstRef()
{
    if (mInitStatus == 0) {
        run("Acc Loop", ANDROID_PRIORITY_URGENT_AUDIO);
    }
}

bool BubbleLevelImpl::threadLoop() {
    bool isLevel;

    while(!exitPending()) {
        {
            Mutex::Autolock _l(mStateLock);

            isLevel = false;

            switch (mCmd) {
            case BL_CMD_POLL_ONCE:
            case BL_CMD_START_POLL:
                if (mState == BL_STATE_IDLE) {
                    mSensorEventQueue->enableSensor(mAccelerometer);
                    mSensorEventQueue->setEventRate(mAccelerometer,
                                                    ms2ns(BL_SENSOR_POLL_INTERVAL_MS));
                    mPollCount = 0;
                    mLevelCount = 0;
                    if (mCmd == BL_CMD_START_POLL) {
                        mState = BL_STATE_POLLING;
                    } else {
                        mState = BL_STATE_POLLING_ONCE;
                    }
                }
                if ((mCmd == BL_CMD_START_POLL) && (mState == BL_STATE_POLLING_ONCE)) {
                    mState = BL_STATE_POLLING;
                }
                break;
            case BL_CMD_STOP_POLL:
                if (mState == BL_STATE_POLLING ||
                        mState == BL_STATE_POLLING_ONCE ||
                        mState == BL_STATE_SLEEPING) {
                    mSensorEventQueue->disableSensor(mAccelerometer);
                    mState = BL_STATE_IDLE;
                }
                break;
            case BL_CMD_EXIT:
                continue;
            case BL_CMD_NONE:
                break;
            default:
                ALOGE("unknown command: %d", mCmd);
            }
            mCmd = BL_CMD_NONE;

            switch (mState) {
            case BL_STATE_IDLE:
                mCond.wait(mStateLock);
                continue;

            case BL_STATE_POLLING:
            case BL_STATE_POLLING_ONCE:
                if (mPollCount >= BL_SENSOR_POLL_COUNT) {
                    // majority vote
                    isLevel = (mLevelCount > (BL_SENSOR_POLL_COUNT / 2));
                    if (mState == BL_STATE_POLLING_ONCE) {
                        mCmd = BL_CMD_STOP_POLL;
                    }
                    mState = BL_STATE_SLEEPING;
                }
                break;
            case BL_STATE_SLEEPING:
                mCond.waitRelative(mStateLock, seconds(mPollIntervalSec));
                mPollCount = 0;
                mLevelCount = 0;
                mState = BL_STATE_POLLING;
                break;

            default:
                ALOGE("unknown state: %d", mState);
                mState = BL_STATE_IDLE;
                continue;
            }
        }

        if (mState == BL_STATE_SLEEPING) {
            Mutex::Autolock _l(mCallbackLock);
            if (mCallBack != NULL) {
                mCallBack(isLevel, mUserData);
            }
            continue;
        }
        mLooper->pollOnce(BL_SENSOR_POLL_TIMEOUT_MS);
    }
    ALOGV("threadLoop EXIT");
    return false;
}

int BubbleLevelImpl::setCallback(BubbleLevel_CallBack_t callback, void *userData)
{
    Mutex::Autolock _l(mCallbackLock);
    if (mInitStatus != 0) {
        return mInitStatus;
    }
    mCallBack = callback;
    mUserData = userData;
    return 0;
}

int BubbleLevelImpl::setPollInterval(unsigned int seconds)
{
    if (seconds < BL_POLL_INTERVAL_MIN_SEC) {
        return -EINVAL;
    }

    Mutex::Autolock _l(mStateLock);
    if (mInitStatus != 0) {
        return mInitStatus;
    }
    mPollIntervalSec = seconds;
    return 0;
}
int BubbleLevelImpl::startPolling()
{
    Mutex::Autolock _l(mStateLock);
    if (mInitStatus != 0) {
        return mInitStatus;
    }
    if (mCmd != BL_CMD_EXIT) {
        mCmd = BL_CMD_START_POLL;
        mCond.signal();
    }
    return 0;
}

int BubbleLevelImpl::stopPolling()
{
    Mutex::Autolock _l(mStateLock);
    if (mInitStatus != 0) {
        return mInitStatus;
    }
    if (mCmd != BL_CMD_EXIT) {
        mCmd = BL_CMD_STOP_POLL;
        mCond.signal();
    }
    return 0;
}

int BubbleLevelImpl::pollOnce()
{
    Mutex::Autolock _l(mStateLock);
    if (mInitStatus != 0) {
        return mInitStatus;
    }
    if (mCmd != BL_CMD_EXIT) {
        mCmd = BL_CMD_POLL_ONCE;
        mCond.signal();
    }
    return 0;
}

static int sensor_callback(int fd, int events, void* data)
{
    sp<BubbleLevelImpl> bl = sp<BubbleLevelImpl>((BubbleLevelImpl *)data);

    bl->lockState();
    if (((bl->state() != BubbleLevelImpl::BL_STATE_POLLING) &&
            (bl->state() != BubbleLevelImpl::BL_STATE_POLLING_ONCE)) ||
            (bl->pollCount() >= BL_SENSOR_POLL_COUNT)) {
        bl->unlockState();
        return 1;
    }
    bl->unlockState();

    sp<SensorEventQueue> sensorEventQueue = bl->sensorEventQueue();
    size_t numSensors = bl->numSensors();
    bool isLevel = false;
    ASensorEvent sensorEvents[numSensors];
    ssize_t ret = sensorEventQueue->read(sensorEvents, numSensors);
    if (ret > 0) {
        for (int i = 0; i < ret; i++) {
            if (sensorEvents[i].type == Sensor::TYPE_ACCELEROMETER) {
                ALOGV("sensor_callback() azimuth = %f pitch = %f roll = %f",
                      sensorEvents[i].vector.azimuth,
                      sensorEvents[i].vector.pitch,
                      sensorEvents[i].vector.roll);

                if ((sensorEvents[i].vector.roll > 0.0) &&
                        (sensorEvents[i].vector.azimuth < BL_SENSOR_LEVEL_THRESHOLD) &&
                        (sensorEvents[i].vector.azimuth > -BL_SENSOR_LEVEL_THRESHOLD) &&
                        (sensorEvents[i].vector.pitch < BL_SENSOR_LEVEL_THRESHOLD) &&
                        (sensorEvents[i].vector.pitch > -BL_SENSOR_LEVEL_THRESHOLD)) {
                    isLevel = true;
                }
                break;
            }
        }
    }

    bl->lockState();
    bl->incPollCount();
    if (isLevel) {
        bl->incLevelCount();
    }
    bl->unlockState();

    return 1;
}

}; // namespace android

extern "C" {

static int bl_set_callback(const struct bubble_level *bubble_level,
                 BubbleLevel_CallBack_t callback, void *userData)
{
    bubble_level_C_impl *bl = (bubble_level_C_impl *)bubble_level;
    return bl->bubble_level->setCallback(callback, userData);
}

static int bl_set_poll_interval(const struct bubble_level *bubble_level, unsigned int seconds)
{
    bubble_level_C_impl *bl = (bubble_level_C_impl *)bubble_level;
    return bl->bubble_level->setPollInterval(seconds);
}

static int bl_start_polling(const struct bubble_level *bubble_level)
{
    bubble_level_C_impl *bl = (bubble_level_C_impl *)bubble_level;
    return bl->bubble_level->startPolling();
}

static int bl_stop_polling(const struct bubble_level *bubble_level)
{
    bubble_level_C_impl *bl = (bubble_level_C_impl *)bubble_level;
    return bl->bubble_level->stopPolling();
}

static int bl_poll_once(const struct bubble_level *bubble_level)
{
    bubble_level_C_impl *bl = (bubble_level_C_impl *)bubble_level;
    return bl->bubble_level->pollOnce();
}


struct bubble_level *bubble_level_create()
{
    bubble_level_C_impl *bl = new bubble_level_C_impl();
    bl->bubble_level = new android::BubbleLevelImpl();
    if (bl->bubble_level->initStatus() != 0) {
        bubble_level_release((struct bubble_level *)bl);
        return NULL;
    }
    bl->interface.set_callback = bl_set_callback;
    bl->interface.set_poll_interval = bl_set_poll_interval;
    bl->interface.start_polling = bl_start_polling;
    bl->interface.stop_polling = bl_stop_polling;
    bl->interface.poll_once = bl_poll_once;

    return (struct bubble_level *)bl;
}

void bubble_level_release(const struct bubble_level *bubble_level)
{
    bubble_level_C_impl *bl = (bubble_level_C_impl *)bubble_level;

    if (bl == NULL)
        return;

    bl->bubble_level.clear();
    delete bubble_level;
}

};
