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
#include <IDisplayDevice.h>
#include <common/base/Drm.h>
#include <DrmConfig.h>


namespace android {
namespace intel {

const char* DrmConfig::getDrmPath()
{
    return "/dev/card0";
}

uint32_t DrmConfig::getDrmConnector(int device)
{
    if (device == IDisplayDevice::DEVICE_PRIMARY)
        return DRM_MODE_CONNECTOR_MIPI;
    else if (device == IDisplayDevice::DEVICE_EXTERNAL)
        return DRM_MODE_CONNECTOR_DVID;
    return DRM_MODE_CONNECTOR_Unknown;
}

uint32_t DrmConfig::getDrmEncoder(int device)
{
    if (device == IDisplayDevice::DEVICE_PRIMARY)
        return DRM_MODE_ENCODER_MIPI;
    else if (device == IDisplayDevice::DEVICE_EXTERNAL)
        return DRM_MODE_ENCODER_TMDS;
    return DRM_MODE_ENCODER_NONE;
}

uint32_t DrmConfig::getFrameBufferFormat()
{
    return HAL_PIXEL_FORMAT_RGBX_8888;
}

uint32_t DrmConfig::getFrameBufferDepth()
{
    return 24;
}

uint32_t DrmConfig::getFrameBufferBpp()
{
    return 32;
}

const char* DrmConfig::getUeventEnvelope()
{
    return "change@/devices/pci0000:00/0000:00:02.0/drm/card0";
}

const char* DrmConfig::getHotplugString()
{
    return "HOTPLUG=1";
}

const char* DrmConfig::getRepeatedFrameString()
{
    return "REPEATED_FRAME";
}

} // namespace intel
} // namespace android
