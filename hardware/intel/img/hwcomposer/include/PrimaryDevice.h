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
#ifndef PRIMARY_DEVICE_H
#define PRIMARY_DEVICE_H

#include <DisplayPlane.h>
#include <IVsyncControl.h>
#include <IBlankControl.h>
#include <common/observers/VsyncEventObserver.h>
#include <common/base/HwcLayerList.h>
#include <PhysicalDevice.h>

namespace android {
namespace intel {


class PrimaryDevice : public PhysicalDevice {
public:
    PrimaryDevice(Hwcomposer& hwc, DisplayPlaneManager& dpm);
    virtual ~PrimaryDevice();
public:
    virtual bool initialize();
    virtual void deinitialize();

    bool blank(bool blank);
protected:
    virtual IVsyncControl* createVsyncControl() = 0;
    virtual IBlankControl* createBlankControl() = 0;
};

}
}

#endif /* PRIMARY_DEVICE_H */
