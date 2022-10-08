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
#include <common/base/Drm.h>
#include <Hwcomposer.h>
#include <DrmConfig.h>
#include <PrimaryDevice.h>

namespace android {
namespace intel {

PrimaryDevice::PrimaryDevice(Hwcomposer& hwc, DisplayPlaneManager& dpm)
    : PhysicalDevice(DEVICE_PRIMARY, hwc, dpm)
{
    CTRACE();
}

PrimaryDevice::~PrimaryDevice()
{
    CTRACE();
}

bool PrimaryDevice::initialize()
{
    if (!PhysicalDevice::initialize()) {
        DEINIT_AND_RETURN_FALSE("failed to initialize physical device");
    }

    return true;
}

void PrimaryDevice::deinitialize()
{
    PhysicalDevice::deinitialize();
}

bool PrimaryDevice::blank(bool blank)
{
    if (!mConnected)
        return true;

    return PhysicalDevice::blank(blank);
}

} // namespace intel
} // namespace android
