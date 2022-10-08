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

#ifndef ANDROID_BUBBLE_LEVEL_IMPL_
#define ANDROID_BUBBLE_LEVEL_IMPL_

#include "BubbleLevel.h"
#include <utils/Looper.h>
#include <gui/Sensor.h>
#include <gui/SensorManager.h>
#include <gui/SensorEventQueue.h>


namespace android {

class BubbleLevelImpl : public Thread
{
public:

    BubbleLevelImpl();
    virtual ~BubbleLevelImpl();

    int initStatus() const { return mInitStatus; }

    // BubbleLevel interface
    int setCallback(BubbleLevel_CallBack_t callback, void *userData);
    int setPollInterval(unsigned int seconds);
    int startPolling();
    int stopPolling();
    int pollOnce();

    // RefBase
    virtual     void        onFirstRef();
    // Thread virtuals
    virtual     bool        threadLoop();

    enum state {
        BL_STATE_IDLE,
        BL_STATE_POLLING,
        BL_STATE_POLLING_ONCE,
        BL_STATE_SLEEPING,
    };

    uint32_t state() const { return mState; }
    void lockState() { mStateLock.lock(); }
    void unlockState() { mStateLock.unlock(); }
    uint32_t pollCount() { return mPollCount; }
    void incPollCount() { mPollCount++; }
    void incLevelCount() { mLevelCount++; }
    sp<SensorEventQueue> sensorEventQueue() const { return mSensorEventQueue; }
    size_t numSensors() const { return mNumSensors; }

private:
    enum command {
        BL_CMD_NONE,
        BL_CMD_START_POLL,
        BL_CMD_STOP_POLL,
        BL_CMD_POLL_ONCE,
        BL_CMD_EXIT,
    };

    int init();

    Mutex  mStateLock;
    Mutex  mCallbackLock;
    Condition mCond;
    uint32_t mState;
    uint32_t mCmd;
    uint32_t mPollIntervalSec;
    uint32_t mPollCount;
    uint32_t mLevelCount;

    BubbleLevel_CallBack_t mCallBack;
    void *mUserData;

    size_t mNumSensors;
    Sensor const* mAccelerometer;
    sp<SensorEventQueue> mSensorEventQueue;
    sp<Looper> mLooper;
    int mInitStatus;
};

};

class BubbleLevelBase: public BubbleLevel
{
public:
    BubbleLevelBase() { mBubbleLevel = new android::BubbleLevelImpl(); }
    virtual ~BubbleLevelBase() {}

    int initStatus() {
        return mBubbleLevel->initStatus();
    }

    // BubbleLevel interface
    virtual int setCallback(BubbleLevel_CallBack_t callback, void *userData) {
        return mBubbleLevel->setCallback(callback, userData);
    }
    virtual int setPollInterval(unsigned int seconds) {
        return mBubbleLevel->setPollInterval(seconds);
    }
    virtual int startPolling() {
        return mBubbleLevel->startPolling();
    }
    virtual int stopPolling() {
        return mBubbleLevel->stopPolling();
    }
    virtual int pollOnce() {
        return mBubbleLevel->pollOnce();
    }

private:
    android::sp<android::BubbleLevelImpl> mBubbleLevel;
};

BubbleLevel *BubbleLevel::create() {
    BubbleLevelBase *bl = new BubbleLevelBase();

    if (bl->initStatus() != 0) {
        delete bl;
        bl = NULL;
    }
    return static_cast<BubbleLevel *>(bl);
}

struct bubble_level_C_impl
{
    struct bubble_level interface;
    android::sp<android::BubbleLevelImpl> bubble_level;
};

#endif /*ANDROID_BUBBLE_LEVEL_IMPL_*/
