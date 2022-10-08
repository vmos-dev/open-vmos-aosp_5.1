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
#ifndef IDISPLAY_DEVICE_H
#define IDISPLAY_DEVICE_H

#include <common/utils/Dump.h>
#include <IDisplayContext.h>
#include <DisplayPlane.h>

namespace android {
namespace intel {

// display config
class DisplayConfig {
public:
    DisplayConfig(int rr, int w, int h, int dpix, int dpiy)
        : mRefreshRate(rr),
          mWidth(w),
          mHeight(h),
          mDpiX(dpix),
          mDpiY(dpiy)
    {}
public:
    int getRefreshRate() const { return mRefreshRate; }
    int getWidth() const { return mWidth; }
    int getHeight() const { return mHeight; }
    int getDpiX() const { return mDpiX; }
    int getDpiY() const { return mDpiY; }
private:
    int mRefreshRate;
    int mWidth;
    int mHeight;
    int mDpiX;
    int mDpiY;
};


//  display device interface
class IDisplayDevice {
public:
    // display device type
    enum {
        DEVICE_PRIMARY = HWC_DISPLAY_PRIMARY,
        DEVICE_EXTERNAL = HWC_DISPLAY_EXTERNAL,
        DEVICE_VIRTUAL = HWC_DISPLAY_VIRTUAL,
        DEVICE_COUNT,
    };
    enum {
        DEVICE_DISCONNECTED = 0,
        DEVICE_CONNECTED,
    };
    enum {
        DEVICE_DISPLAY_OFF = 0,
        DEVICE_DISPLAY_ON,
        DEVICE_DISPLAY_STANDBY,
    };
public:
    IDisplayDevice() {}
    virtual ~IDisplayDevice() {}
public:
    virtual bool prePrepare(hwc_display_contents_1_t *display) = 0;
    virtual bool prepare(hwc_display_contents_1_t *display) = 0;
    virtual bool commit(hwc_display_contents_1_t *display,
                          IDisplayContext *context) = 0;

    virtual bool vsyncControl(bool enabled) = 0;
    virtual bool blank(bool blank) = 0;
    virtual bool getDisplaySize(int *width, int *height) = 0;
    virtual bool getDisplayConfigs(uint32_t *configs,
                                       size_t *numConfigs) = 0;
    virtual bool getDisplayAttributes(uint32_t config,
                                          const uint32_t *attributes,
                                          int32_t *values) = 0;
    virtual bool compositionComplete() = 0;

    virtual bool setPowerMode(int mode) = 0;
    virtual int  getActiveConfig() = 0;
    virtual bool setActiveConfig(int index) = 0;

    virtual bool initialize() = 0;
    virtual void deinitialize() = 0;
    virtual bool isConnected() const = 0;
    virtual const char* getName() const = 0;
    virtual int getType() const = 0;
    virtual void onVsync(int64_t timestamp) = 0;
    virtual void dump(Dump& d) = 0;
};

}
}

#endif /* IDISPLAY_DEVICE_H */
