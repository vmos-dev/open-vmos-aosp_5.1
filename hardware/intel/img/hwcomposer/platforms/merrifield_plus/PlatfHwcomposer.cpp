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
#include <ips/tangier/TngDisplayContext.h>
#include <ips/anniedale/AnnPlaneManager.h>
#include <platforms/merrifield_plus/PlatfBufferManager.h>
#include <DummyDevice.h>
#include <IDisplayDevice.h>
#include <platforms/merrifield_plus/PlatfPrimaryDevice.h>
#include <platforms/merrifield_plus/PlatfExternalDevice.h>
#include <platforms/merrifield_plus/PlatfHwcomposer.h>



namespace android {
namespace intel {

PlatfHwcomposer::PlatfHwcomposer()
    : Hwcomposer()
{
    CTRACE();
}

PlatfHwcomposer::~PlatfHwcomposer()
{
    CTRACE();
}

DisplayPlaneManager* PlatfHwcomposer::createDisplayPlaneManager()
{
    CTRACE();
    return (new AnnPlaneManager());
}

BufferManager* PlatfHwcomposer::createBufferManager()
{
    CTRACE();
    return (new PlatfBufferManager());
}

IDisplayDevice* PlatfHwcomposer::createDisplayDevice(int disp,
                                                     DisplayPlaneManager& dpm)
{
    CTRACE();

    switch (disp) {
        case IDisplayDevice::DEVICE_PRIMARY:
#ifdef INTEL_SUPPORT_HDMI_PRIMARY
            return new PlatfExternalDevice(*this, dpm);
#else
            return new PlatfPrimaryDevice(*this, dpm);
#endif

        case IDisplayDevice::DEVICE_EXTERNAL:
#ifdef INTEL_SUPPORT_HDMI_PRIMARY
            return new DummyDevice((uint32_t)disp, *this);
#else
            return new PlatfExternalDevice(*this, dpm);
#endif

        case IDisplayDevice::DEVICE_VIRTUAL:
            return new DummyDevice((uint32_t)disp, *this);

        default:
            ELOGTRACE("invalid display device %d", disp);
            return NULL;
    }
}

IDisplayContext* PlatfHwcomposer::createDisplayContext()
{
    CTRACE();
    return new TngDisplayContext();
}

Hwcomposer* Hwcomposer::createHwcomposer()
{
    CTRACE();
    return new PlatfHwcomposer();
}

} //namespace intel
} //namespace android
