/*
// Copyright (c) 2014 Intel Corporation 
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#include <common/utils/HwcTrace.h>
#include <common/observers/VsyncEventObserver.h>
#include <PhysicalDevice.h>

namespace android {
namespace intel {

VsyncEventObserver::VsyncEventObserver(PhysicalDevice& disp)
    : mLock(),
      mCondition(),
      mDisplayDevice(disp),
      mVsyncControl(NULL),
      mDevice(IDisplayDevice::DEVICE_COUNT),
      mEnabled(false),
      mExitThread(false),
      mInitialized(false)
{
    CTRACE();
}

VsyncEventObserver::~VsyncEventObserver()
{
    WARN_IF_NOT_DEINIT();
}

bool VsyncEventObserver::initialize()
{
    if (mInitialized) {
        WLOGTRACE("object has been initialized");
        return true;
    }

    mExitThread = false;
    mEnabled = false;
    mDevice = mDisplayDevice.getType();
    mVsyncControl = mDisplayDevice.createVsyncControl();
    if (!mVsyncControl || !mVsyncControl->initialize()) {
        DEINIT_AND_RETURN_FALSE("failed to initialize vsync control");
    }

    mThread = new VsyncEventPollThread(this);
    if (!mThread.get()) {
        DEINIT_AND_RETURN_FALSE("failed to create vsync event poll thread.");
    }

    mThread->run("VsyncEventObserver", PRIORITY_URGENT_DISPLAY);

    mInitialized = true;
    return true;
}

void VsyncEventObserver::deinitialize()
{
    if (mEnabled) {
        WLOGTRACE("vsync is still enabled");
        control(false);
    }
    mInitialized = false;
    mExitThread = true;
    mEnabled = false;
    mCondition.signal();

    if (mThread.get()) {
        mThread->requestExitAndWait();
        mThread = NULL;
    }

    DEINIT_AND_DELETE_OBJ(mVsyncControl);
}

bool VsyncEventObserver::control(bool enabled)
{
    ALOGTRACE("enabled = %d on device %d", enabled, mDevice);
    if (enabled == mEnabled) {
        WLOGTRACE("vsync state %d is not changed", enabled);
        return true;
    }

    Mutex::Autolock _l(mLock);
    bool ret = mVsyncControl->control(mDevice, enabled);
    if (!ret) {
        ELOGTRACE("failed to control (%d) vsync on display %d", enabled, mDevice);
        return false;
    }

    mEnabled = enabled;
    mCondition.signal();
    return true;
}

bool VsyncEventObserver::threadLoop()
{
    do {
        // scope for lock
        Mutex::Autolock _l(mLock);
        while (!mEnabled) {
            mCondition.wait(mLock);
            if (mExitThread) {
                ILOGTRACE("exiting thread loop");
                return false;
            }
        }
    } while (0);

    if(mEnabled && mDisplayDevice.isConnected()) {
        int64_t timestamp;
        bool ret = mVsyncControl->wait(mDevice, timestamp);
        if (ret == false) {
            WLOGTRACE("failed to wait for vsync on display %d, vsync enabled %d", mDevice, mEnabled);
            usleep(16000);
            return true;
        }

        // notify device
        mDisplayDevice.onVsync(timestamp);
    }

    return true;
}

} // namespace intel
} // namesapce android
