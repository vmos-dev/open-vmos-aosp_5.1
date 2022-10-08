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
#include <common/base/Drm.h>
#include <PhysicalDevice.h>

namespace android {
namespace intel {

PhysicalDevice::PhysicalDevice(uint32_t type, Hwcomposer& hwc, DisplayPlaneManager& dpm)
    : mType(type),
      mHwc(hwc),
      mDisplayPlaneManager(dpm),
      mActiveDisplayConfig(-1),
      mBlankControl(NULL),
      mVsyncObserver(NULL),
      mLayerList(NULL),
      mConnected(false),
      mBlank(false),
      mDisplayState(DEVICE_DISPLAY_ON),
      mInitialized(false)
{
    CTRACE();

    switch (type) {
    case DEVICE_PRIMARY:
        mName = "Primary";
        break;
    case DEVICE_EXTERNAL:
        mName = "External";
        break;
    default:
        mName = "Unknown";
    }

    mDisplayConfigs.setCapacity(DEVICE_COUNT);
}

PhysicalDevice::~PhysicalDevice()
{
    WARN_IF_NOT_DEINIT();
}

void PhysicalDevice::onGeometryChanged(hwc_display_contents_1_t *list)
{
    if (!list) {
        ELOGTRACE("list is NULL");
        return;
    }

    ALOGTRACE("disp = %d, layer number = %d", mType, list->numHwLayers);

    // NOTE: should NOT be here
    if (mLayerList) {
        WLOGTRACE("mLayerList exists");
        DEINIT_AND_DELETE_OBJ(mLayerList);
    }

    // create a new layer list
    mLayerList = new HwcLayerList(list, mType);
    if (!mLayerList) {
        WLOGTRACE("failed to create layer list");
    }
}

bool PhysicalDevice::prePrepare(hwc_display_contents_1_t *display)
{
    RETURN_FALSE_IF_NOT_INIT();

    // for a null list, delete hwc list
    if (!mConnected || !display || mBlank) {
        if (mLayerList) {
            DEINIT_AND_DELETE_OBJ(mLayerList);
        }
        return true;
    }

    // check if geometry is changed, if changed delete list
    if ((display->flags & HWC_GEOMETRY_CHANGED) && mLayerList) {
        DEINIT_AND_DELETE_OBJ(mLayerList);
    }
    return true;
}

bool PhysicalDevice::prepare(hwc_display_contents_1_t *display)
{
    RETURN_FALSE_IF_NOT_INIT();

    if (!mConnected || !display || mBlank)
        return true;

    // check if geometry is changed
    if (display->flags & HWC_GEOMETRY_CHANGED) {
        onGeometryChanged(display);
    }
    if (!mLayerList) {
        WLOGTRACE("null HWC layer list");
        return true;
    }

    // update list with new list
    return mLayerList->update(display);
}


bool PhysicalDevice::commit(hwc_display_contents_1_t *display, IDisplayContext *context)
{
    RETURN_FALSE_IF_NOT_INIT();

    if (!display || !context || !mLayerList || mBlank) {
        return true;
    }
    return context->commitContents(display, mLayerList);
}

bool PhysicalDevice::vsyncControl(bool enabled)
{
    RETURN_FALSE_IF_NOT_INIT();

    ALOGTRACE("disp = %d, enabled = %d", mType, enabled);
    return mVsyncObserver->control(enabled);
}

bool PhysicalDevice::blank(bool blank)
{
    RETURN_FALSE_IF_NOT_INIT();

    mBlank = blank;
    bool ret = mBlankControl->blank(mType, blank);
    if (ret == false) {
        ELOGTRACE("failed to blank device");
        return false;
    }

    return true;
}

bool PhysicalDevice::getDisplaySize(int *width, int *height)
{
    RETURN_FALSE_IF_NOT_INIT();
    Mutex::Autolock _l(mLock);
    if (!width || !height) {
        ELOGTRACE("invalid parameters");
        return false;
    }

    *width = 0;
    *height = 0;
    drmModeModeInfo mode;
    Drm *drm = Hwcomposer::getInstance().getDrm();
    bool ret = drm->getModeInfo(mType, mode);
    if (!ret) {
        return false;
    }

    *width = mode.hdisplay;
    *height = mode.vdisplay;
    return true;
}

template <typename T>
static inline T min(T a, T b) {
    return a<b ? a : b;
}

bool PhysicalDevice::getDisplayConfigs(uint32_t *configs,
                                         size_t *numConfigs)
{
    RETURN_FALSE_IF_NOT_INIT();

    Mutex::Autolock _l(mLock);

    if (!mConnected) {
        ILOGTRACE("device is not connected");
        return false;
    }

    if (!configs || !numConfigs || *numConfigs < 1) {
        ELOGTRACE("invalid parameters");
        return false;
    }

    // fill in all config handles
    *numConfigs = min(*numConfigs, mDisplayConfigs.size());
    for (int i = 0; i < static_cast<int>(*numConfigs); i++) {
        configs[i] = i;
    }

    return true;
}

bool PhysicalDevice::getDisplayAttributes(uint32_t config,
        const uint32_t *attributes,
        int32_t *values)
{
    RETURN_FALSE_IF_NOT_INIT();

    Mutex::Autolock _l(mLock);

    if (!mConnected) {
        ILOGTRACE("device is not connected");
        return false;
    }

    if (!attributes || !values) {
        ELOGTRACE("invalid parameters");
        return false;
    }

    DisplayConfig *configChosen = mDisplayConfigs.itemAt(config);
    if  (!configChosen) {
        WLOGTRACE("failed to get display config");
        return false;
    }

    int i = 0;
    while (attributes[i] != HWC_DISPLAY_NO_ATTRIBUTE) {
        switch (attributes[i]) {
        case HWC_DISPLAY_VSYNC_PERIOD:
            if (configChosen->getRefreshRate()) {
                values[i] = 1e9 / configChosen->getRefreshRate();
            } else {
                ELOGTRACE("refresh rate is 0!!!");
                values[i] = 0;
            }
            break;
        case HWC_DISPLAY_WIDTH:
            values[i] = configChosen->getWidth();
            break;
        case HWC_DISPLAY_HEIGHT:
            values[i] = configChosen->getHeight();
            break;
        case HWC_DISPLAY_DPI_X:
            values[i] = configChosen->getDpiX() * 1000.0f;
            break;
        case HWC_DISPLAY_DPI_Y:
            values[i] = configChosen->getDpiY() * 1000.0f;
            break;
        default:
            ELOGTRACE("unknown attribute %d", attributes[i]);
            break;
        }
        i++;
    }

    return true;
}

bool PhysicalDevice::compositionComplete()
{
    CTRACE();
    // do nothing by default
    return true;
}

void PhysicalDevice::removeDisplayConfigs()
{
    for (size_t i = 0; i < mDisplayConfigs.size(); i++) {
        DisplayConfig *config = mDisplayConfigs.itemAt(i);
        delete config;
    }

    mDisplayConfigs.clear();
    mActiveDisplayConfig = -1;
}

bool PhysicalDevice::detectDisplayConfigs()
{
    Mutex::Autolock _l(mLock);

    Drm *drm = Hwcomposer::getInstance().getDrm();
    if (!drm->detect(mType)) {
        ELOGTRACE("drm detection on device %d failed ", mType);
        return false;
    }
    return updateDisplayConfigs();
}

bool PhysicalDevice::updateDisplayConfigs()
{
    bool ret;
    Drm *drm = Hwcomposer::getInstance().getDrm();

    // reset display configs
    removeDisplayConfigs();

    // update device connection status
    mConnected = drm->isConnected(mType);
    if (!mConnected) {
        return true;
    }

    // reset the number of display configs
    mDisplayConfigs.setCapacity(1);

    drmModeModeInfo mode;
    ret = drm->getModeInfo(mType, mode);
    if (!ret) {
        ELOGTRACE("failed to get mode info");
        mConnected = false;
        return false;
    }

    uint32_t mmWidth, mmHeight;
    ret = drm->getPhysicalSize(mType, mmWidth, mmHeight);
    if (!ret) {
        ELOGTRACE("failed to get physical size");
        mConnected = false;
        return false;
    }

    float physWidthInch = (float)mmWidth * 0.039370f;
    float physHeightInch = (float)mmHeight * 0.039370f;

    // use current drm mode, likely it's preferred mode
    int dpiX = 0;
    int dpiY = 0;
    if (physWidthInch && physHeightInch) {
        dpiX = mode.hdisplay / physWidthInch;
        dpiY = mode.vdisplay / physHeightInch;
    } else {
        ELOGTRACE("invalid physical size, EDID read error?");
        // don't bail out as it is not a fatal error
    }
    // use active fb dimension as config width/height
    DisplayConfig *config = new DisplayConfig(mode.vrefresh,
                                              mode.hdisplay,
                                              mode.vdisplay,
                                              dpiX, dpiY);
    // add it to the front of other configs
    mDisplayConfigs.push_front(config);

    // init the active display config
    mActiveDisplayConfig = 0;

    drmModeModeInfoPtr modes;
    drmModeModeInfoPtr compatMode;
    int modeCount = 0;

    modes = drm->detectAllConfigs(mType, &modeCount);

    for (int i = 0; i < modeCount; i++) {
        if (modes) {
            compatMode = &modes[i];
            if (!compatMode)
                continue;
            if (compatMode->hdisplay == mode.hdisplay &&
                compatMode->vdisplay == mode.vdisplay &&
                compatMode->vrefresh != mode.vrefresh) {

                bool found = false;
                for (size_t j = 0; j < mDisplayConfigs.size(); j++) {
                     DisplayConfig *config = mDisplayConfigs.itemAt(j);
                     if (config->getRefreshRate() == (int)compatMode->vrefresh) {
                         found = true;
                         break;
                     }
                }

                if (found) {
                    continue;
                }

                DisplayConfig *config = new DisplayConfig(compatMode->vrefresh,
                                              compatMode->hdisplay,
                                              compatMode->vdisplay,
                                              dpiX, dpiY);
                // add it to the end of configs
                mDisplayConfigs.push_back(config);
            }
        }
    }

    return true;
}

bool PhysicalDevice::initialize()
{
    CTRACE();

    if (mType != DEVICE_PRIMARY && mType != DEVICE_EXTERNAL) {
        ELOGTRACE("invalid device type");
        return false;
    }

    // detect display configs
    bool ret = detectDisplayConfigs();
    if (ret == false) {
        DEINIT_AND_RETURN_FALSE("failed to detect display config");
    }

    // create blank control
    mBlankControl = createBlankControl();
    if (!mBlankControl) {
        DEINIT_AND_RETURN_FALSE("failed to create blank control");
    }

    // create vsync event observer
    mVsyncObserver = new VsyncEventObserver(*this);
    if (!mVsyncObserver || !mVsyncObserver->initialize()) {
        DEINIT_AND_RETURN_FALSE("failed to create vsync observer");
    }

    mInitialized = true;
    return true;
}

void PhysicalDevice::deinitialize()
{
    if (mLayerList) {
        DEINIT_AND_DELETE_OBJ(mLayerList);
    }

    DEINIT_AND_DELETE_OBJ(mVsyncObserver);

    // destroy blank control
    if (mBlankControl) {
        delete mBlankControl;
        mBlankControl = 0;
    }

    // remove configs
    removeDisplayConfigs();

    mInitialized = false;
}

bool PhysicalDevice::isConnected() const
{
    RETURN_FALSE_IF_NOT_INIT();

    return mConnected;
}

const char* PhysicalDevice::getName() const
{
    return mName;
}

int PhysicalDevice::getType() const
{
    return mType;
}

void PhysicalDevice::onVsync(int64_t timestamp)
{
    RETURN_VOID_IF_NOT_INIT();
    ALOGTRACE("timestamp = %lld", timestamp);

    if (!mConnected)
        return;

    // notify hwc
    mHwc.vsync(mType, timestamp);
}

void PhysicalDevice::dump(Dump& d)
{
    d.append("-------------------------------------------------------------\n");
    d.append("Device Name: %s (%s)\n", mName,
            mConnected ? "connected" : "disconnected");
    d.append("Display configs (count = %d):\n", mDisplayConfigs.size());
    d.append(" CONFIG | VSYNC_PERIOD | WIDTH | HEIGHT | DPI_X | DPI_Y \n");
    d.append("--------+--------------+-------+--------+-------+-------\n");
    for (size_t i = 0; i < mDisplayConfigs.size(); i++) {
        DisplayConfig *config = mDisplayConfigs.itemAt(i);
        if (config) {
            d.append("%s %2d   |     %4d     | %5d |  %4d  |  %3d  |  %3d  \n",
                     (i == (size_t)mActiveDisplayConfig) ? "* " : "  ",
                     i,
                     config->getRefreshRate(),
                     config->getWidth(),
                     config->getHeight(),
                     config->getDpiX(),
                     config->getDpiY());
        }
    }
    // dump layer list
    if (mLayerList)
        mLayerList->dump(d);
}

bool PhysicalDevice::setPowerMode(int mode)
{
    // TODO: set proper blanking modes for HWC 1.4 modes
    switch (mode) {
        case HWC_POWER_MODE_OFF:
        case HWC_POWER_MODE_DOZE:
            return blank(true);
        case HWC_POWER_MODE_NORMAL:
        case HWC_POWER_MODE_DOZE_SUSPEND:
            return blank(false);
        default:
            return false;
    }
    return false;
}

int PhysicalDevice::getActiveConfig()
{
    return mActiveDisplayConfig;
}

bool PhysicalDevice::setActiveConfig(int index)
{
    // TODO: for now only implement in external
    if (index == 0)
        return true;
    return false;
}

} // namespace intel
} // namespace android
