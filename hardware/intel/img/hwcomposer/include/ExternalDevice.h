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
#ifndef EXTERNAL_DEVICE_H
#define EXTERNAL_DEVICE_H

#include <PhysicalDevice.h>
#include <IHdcpControl.h>
#include <common/base/SimpleThread.h>

namespace android {
namespace intel {


class ExternalDevice : public PhysicalDevice {

public:
    ExternalDevice(Hwcomposer& hwc, DisplayPlaneManager& dpm);
    virtual ~ExternalDevice();
public:
    virtual bool initialize();
    virtual void deinitialize();
    virtual bool blank(bool blank);
    virtual bool setDrmMode(drmModeModeInfo& value);
    virtual void setRefreshRate(int hz);
    virtual bool getDisplaySize(int *width, int *height);
    virtual bool getDisplayConfigs(uint32_t *configs,
                                       size_t *numConfigs);
    virtual bool getDisplayAttributes(uint32_t config,
                                          const uint32_t *attributes,
                                          int32_t *values);
    virtual int  getActiveConfig();
    virtual bool setActiveConfig(int index);

private:
    static void HdcpLinkStatusListener(bool success, void *userData);
    void HdcpLinkStatusListener(bool success);
    void setDrmMode();

protected:
    virtual IHdcpControl* createHdcpControl() = 0;

protected:
    IHdcpControl *mHdcpControl;

private:
    static void hotplugEventListener(void *data);
    void hotplugListener();

private:
    Condition mAbortModeSettingCond;
    drmModeModeInfo mPendingDrmMode;
    bool mHotplugEventPending;
    int mExpectedRefreshRate;

private:
    DECLARE_THREAD(ModeSettingThread, ExternalDevice);
};

}
}

#endif /* EXTERNAL_DEVICE_H */
