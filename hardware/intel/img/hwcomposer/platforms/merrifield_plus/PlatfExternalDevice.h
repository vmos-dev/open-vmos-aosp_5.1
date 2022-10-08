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
#ifndef PLATF_EXTERNAL_DEVICE_H
#define PLATF_EXTERNAL_DEVICE_H

#include <ExternalDevice.h>

namespace android {
namespace intel {

class PlatfExternalDevice : public ExternalDevice {
public:
    PlatfExternalDevice(Hwcomposer& hwc,
                        DisplayPlaneManager& dpm);
    virtual ~PlatfExternalDevice();

protected:
    IVsyncControl* createVsyncControl();
    IBlankControl* createBlankControl();
    IHdcpControl* createHdcpControl();
};

}
}
#endif /* PLATF_EXTERNAL_DEVICE_H */
