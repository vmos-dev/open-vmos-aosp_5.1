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
#ifndef DUMMY_DEVICE_H
#define DUMMY_DEVICE_H

#include <IDisplayDevice.h>

namespace android {
namespace intel {

class Hwcomposer;
class SoftVsyncObserver;

class DummyDevice : public IDisplayDevice {
public:
    DummyDevice(uint32_t type, Hwcomposer& hwc);
    virtual ~DummyDevice();

public:
    virtual bool prePrepare(hwc_display_contents_1_t *display);
    virtual bool prepare(hwc_display_contents_1_t *display);
    virtual bool commit(hwc_display_contents_1_t *display,
                          IDisplayContext *context);

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

    virtual bool initialize();
    virtual void deinitialize();
    virtual bool isConnected() const;
    virtual const char* getName() const;
    virtual int getType() const;
    virtual void onVsync(int64_t timestamp);
    virtual void dump(Dump& d);

protected:
    bool mInitialized;
    bool mConnected;
    bool mBlank;
    uint32_t mType;
    Hwcomposer& mHwc;
    SoftVsyncObserver *mVsyncObserver;

    const char *mName;
};

}
}

#endif /* DUMMY_DEVICE_H */
