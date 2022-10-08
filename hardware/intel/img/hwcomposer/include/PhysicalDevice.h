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
#ifndef PHYSICAL_DEVICE_H
#define PHYSICAL_DEVICE_H

#include <DisplayPlane.h>
#include <IVsyncControl.h>
#include <IBlankControl.h>
#include <common/observers/VsyncEventObserver.h>
#include <common/base/HwcLayerList.h>
#include <common/base/Drm.h>
#include <IDisplayDevice.h>

namespace android {
namespace intel {

class Hwcomposer;

// Base class for primary and external devices
class PhysicalDevice : public IDisplayDevice {
public:
    PhysicalDevice(uint32_t type, Hwcomposer& hwc, DisplayPlaneManager& dpm);
    virtual ~PhysicalDevice();
public:
    virtual bool prePrepare(hwc_display_contents_1_t *display);
    virtual bool prepare(hwc_display_contents_1_t *display);
    virtual bool commit(hwc_display_contents_1_t *display, IDisplayContext *context);

    virtual bool vsyncControl(bool enabled);
    virtual bool blank(bool blank);
    virtual bool getDisplaySize(int *width, int *height);
    virtual bool getDisplayConfigs(uint32_t *configs,
                                       size_t *numConfigs);
    virtual bool getDisplayAttributes(uint32_t config,
                                          const uint32_t *attributes,
                                          int32_t *values);
    virtual bool compositionComplete();

    virtual bool setPowerMode(int mode);
    virtual int  getActiveConfig();
    virtual bool setActiveConfig(int index);

    // display config operations
    virtual void removeDisplayConfigs();
    virtual bool detectDisplayConfigs();

    // device related operations
    virtual bool initCheck() const { return mInitialized; }
    virtual bool initialize();
    virtual void deinitialize();
    virtual bool isConnected() const;
    virtual const char* getName() const;
    virtual int getType() const;

    //events
    virtual void onVsync(int64_t timestamp);

    virtual void dump(Dump& d);

protected:
    void onGeometryChanged(hwc_display_contents_1_t *list);
    bool updateDisplayConfigs();
    virtual IVsyncControl* createVsyncControl() = 0;
    virtual IBlankControl* createBlankControl() = 0;
    friend class VsyncEventObserver;

protected:
    uint32_t mType;
    const char *mName;

    Hwcomposer& mHwc;
    DisplayPlaneManager& mDisplayPlaneManager;

    // display configs
    Vector<DisplayConfig*> mDisplayConfigs;
    int mActiveDisplayConfig;


    IBlankControl *mBlankControl;
    VsyncEventObserver *mVsyncObserver;

    // layer list
    HwcLayerList *mLayerList;
    bool mConnected;
    bool mBlank;

    // lock
    Mutex mLock;

    // DPMS on (1) or off (0)
    int mDisplayState;
    bool mInitialized;
};

}
}

#endif /* PHYSICAL_DEVICE_H */
