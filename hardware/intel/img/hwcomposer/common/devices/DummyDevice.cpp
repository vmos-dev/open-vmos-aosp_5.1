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
#include <Hwcomposer.h>
#include <DisplayQuery.h>
#include <common/observers/SoftVsyncObserver.h>
#include <DummyDevice.h>

namespace android {
namespace intel {
DummyDevice::DummyDevice(uint32_t type, Hwcomposer& hwc)
    : mInitialized(false),
      mConnected(false),
      mBlank(false),
      mType(type),
      mHwc(hwc),
      mVsyncObserver(NULL),
      mName("Dummy")
{
    CTRACE();
}

DummyDevice::~DummyDevice()
{
    WARN_IF_NOT_DEINIT();
}

bool DummyDevice::prePrepare(hwc_display_contents_1_t *display)
{
    RETURN_FALSE_IF_NOT_INIT();

    if (!display) {
        return true;
    }

    // nothing need to do for dummy display
    return true;
}

bool DummyDevice::prepare(hwc_display_contents_1_t *display)
{
    RETURN_FALSE_IF_NOT_INIT();

    if (!display || mType >= DEVICE_VIRTUAL) {
        return true;
    }

    // skip all layers composition on dummy display
    if (display->flags & HWC_GEOMETRY_CHANGED) {
        for (size_t i=0; i < display->numHwLayers-1; i++) {
            hwc_layer_1 * player = &display->hwLayers[i];
            player->compositionType = HWC_OVERLAY;
            player->flags &= ~HWC_SKIP_LAYER;
        }
    }

    return true;
}

bool DummyDevice::commit(hwc_display_contents_1_t *display, IDisplayContext *context)
{
    RETURN_FALSE_IF_NOT_INIT();

    if (!display || !context)
        return true;

    // nothing need to do for dummy display
    return true;
}

bool DummyDevice::vsyncControl(bool enabled)
{
    RETURN_FALSE_IF_NOT_INIT();
    return mVsyncObserver->control(enabled);
}

bool DummyDevice::blank(bool blank)
{
    RETURN_FALSE_IF_NOT_INIT();

    mBlank = blank;

    return true;
}

bool DummyDevice::getDisplaySize(int *width, int *height)
{
    RETURN_FALSE_IF_NOT_INIT();
    if (!width || !height) {
        ELOGTRACE("invalid parameters");
        return false;
    }

    // TODO: make this platform specifc
    *width = 1280;//720;
    *height = 720;//1280;
    return true;
}

bool DummyDevice::getDisplayConfigs(uint32_t *configs,
                                         size_t *numConfigs)
{
    RETURN_FALSE_IF_NOT_INIT();
    if (!configs || !numConfigs) {
        ELOGTRACE("invalid parameters");
        return false;
    }

    if (!mConnected) {
        ILOGTRACE("dummy device is not connected");
        return false;
    }

    *configs = 0;
    *numConfigs = 1;

    return true;
}

bool DummyDevice::getDisplayAttributes(uint32_t configs,
                                            const uint32_t *attributes,
                                            int32_t *values)
{
    RETURN_FALSE_IF_NOT_INIT();

    if ((configs > 0) || !attributes || !values) {
        ELOGTRACE("invalid parameters");
        return false;
    }

    if (!mConnected) {
        ILOGTRACE("dummy device is not connected");
        return false;
    }

    int i = 0;
    while (attributes[i] != HWC_DISPLAY_NO_ATTRIBUTE) {
        switch (attributes[i]) {
        case HWC_DISPLAY_VSYNC_PERIOD:
            values[i] = 1e9 / 60;
            break;
        case HWC_DISPLAY_WIDTH:
            values[i] = 1280;
            break;
        case HWC_DISPLAY_HEIGHT:
            values[i] = 720;
            break;
        case HWC_DISPLAY_DPI_X:
            values[i] = 0;
            break;
        case HWC_DISPLAY_DPI_Y:
            values[i] = 0;
            break;
        default:
            ELOGTRACE("unknown attribute %d", attributes[i]);
            break;
        }
        i++;
    }

    return true;
}

bool DummyDevice::compositionComplete()
{
    RETURN_FALSE_IF_NOT_INIT();
    return true;
}

bool DummyDevice::initialize()
{
    mInitialized = true;

    mVsyncObserver = new SoftVsyncObserver(*this);
    if (!mVsyncObserver || !mVsyncObserver->initialize()) {
        DEINIT_AND_RETURN_FALSE("Failed to create Soft Vsync Observer");
        mInitialized = false;
    }

    return mInitialized;
}

bool DummyDevice::isConnected() const
{
    return mConnected;
}

const char* DummyDevice::getName() const
{
    return "Dummy";
}

int DummyDevice::getType() const
{
    return mType;
}

void DummyDevice::onVsync(int64_t timestamp)
{
    if (!mConnected)
        return;

    mHwc.vsync(mType, timestamp);
}

void DummyDevice::dump(Dump& d)
{
    d.append("-------------------------------------------------------------\n");
    d.append("Device Name: %s (%s)\n", mName,
            mConnected ? "connected" : "disconnected");
}

void DummyDevice::deinitialize()
{
    DEINIT_AND_DELETE_OBJ(mVsyncObserver);
    mInitialized = false;
}

bool DummyDevice::setPowerMode(int /*mode*/)
{
    return true;
}

int DummyDevice::getActiveConfig()
{
    return 0;
}

bool DummyDevice::setActiveConfig(int /*index*/)
{
    return false;
}

} // namespace intel
} // namespace android
