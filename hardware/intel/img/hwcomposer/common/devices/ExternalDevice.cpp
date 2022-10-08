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
#include <DrmConfig.h>
#include <Hwcomposer.h>
#include <ExternalDevice.h>

namespace android {
namespace intel {

ExternalDevice::ExternalDevice(Hwcomposer& hwc, DisplayPlaneManager& dpm)
    : PhysicalDevice(DEVICE_EXTERNAL, hwc, dpm),
      mHdcpControl(NULL),
      mAbortModeSettingCond(),
      mPendingDrmMode(),
      mHotplugEventPending(false),
      mExpectedRefreshRate(0)
{
    CTRACE();
}

ExternalDevice::~ExternalDevice()
{
    CTRACE();
}

bool ExternalDevice::initialize()
{
    if (!PhysicalDevice::initialize()) {
        DEINIT_AND_RETURN_FALSE("failed to initialize physical device");
    }

    mHdcpControl = createHdcpControl();
    if (!mHdcpControl) {
        DEINIT_AND_RETURN_FALSE("failed to create HDCP control");
    }

    mHotplugEventPending = false;
    if (mConnected) {
        mHdcpControl->startHdcpAsync(HdcpLinkStatusListener, this);
    }

    UeventObserver *observer = Hwcomposer::getInstance().getUeventObserver();
    if (observer) {
        observer->registerListener(
            DrmConfig::getHotplugString(),
            hotplugEventListener,
            this);
    } else {
        ELOGTRACE("Uevent observer is NULL");
    }
    return true;
}

void ExternalDevice::deinitialize()
{
    // abort mode settings if it is in the middle
    mAbortModeSettingCond.signal();
    if (mThread.get()) {
        mThread->join();
        mThread = NULL;
    }

    if (mHdcpControl) {
        mHdcpControl->stopHdcp();
        delete mHdcpControl;
        mHdcpControl = 0;
    }

    mHotplugEventPending = false;
    PhysicalDevice::deinitialize();
}

bool ExternalDevice::blank(bool blank)
{
    if (!PhysicalDevice::blank(blank)) {
        return false;
    }

    if (blank) {
        mHdcpControl->stopHdcp();
    } else if (mConnected) {
        mHdcpControl->startHdcpAsync(HdcpLinkStatusListener, this);
    }
    return true;
}

bool ExternalDevice::setDrmMode(drmModeModeInfo& value)
{
    if (!mConnected) {
        WLOGTRACE("external device is not connected");
        return false;
    }

    if (mThread.get()) {
        mThread->join();
        mThread = NULL;
    }

    Drm *drm = Hwcomposer::getInstance().getDrm();
    drmModeModeInfo mode;
    drm->getModeInfo(mType, mode);
    if (drm->isSameDrmMode(&value, &mode))
        return true;

    // any issue here by faking connection status?
    mConnected = false;
    mPendingDrmMode = value;

    // setting mode in a working thread
    mThread = new ModeSettingThread(this);
    if (!mThread.get()) {
        ELOGTRACE("failed to create mode settings thread");
        return false;
    }

    mThread->run("ModeSettingsThread", PRIORITY_URGENT_DISPLAY);
    return true;
}

bool ExternalDevice::threadLoop()
{
    // one-time execution
    setDrmMode();
    return false;
}

void ExternalDevice::setDrmMode()
{
    ILOGTRACE("start mode setting...");

    Drm *drm = Hwcomposer::getInstance().getDrm();

    mConnected = false;
    mHwc.hotplug(mType, false);

    {
        Mutex::Autolock lock(mLock);
        // TODO: make timeout value flexible, or wait until surface flinger
        // acknowledges hot unplug event.
        status_t err = mAbortModeSettingCond.waitRelative(mLock, milliseconds(20));
        if (err != -ETIMEDOUT) {
            ILOGTRACE("Mode settings is interrupted");
            mHwc.hotplug(mType, true);
            return;
        }
    }

    // TODO: potential threading issue with onHotplug callback
    mHdcpControl->stopHdcp();
    if (!drm->setDrmMode(mType, mPendingDrmMode)) {
        ELOGTRACE("failed to set Drm mode");
        mHwc.hotplug(mType, true);
        return;
    }

    if (!PhysicalDevice::updateDisplayConfigs()) {
        ELOGTRACE("failed to update display configs");
        mHwc.hotplug(mType, true);
        return;
    }
    mConnected = true;
    mHotplugEventPending = true;
    // delay sending hotplug event until HDCP is authenticated
    if (mHdcpControl->startHdcpAsync(HdcpLinkStatusListener, this) == false) {
        ELOGTRACE("startHdcpAsync() failed; HDCP is not enabled");
        mHotplugEventPending = false;
        mHwc.hotplug(mType, true);
    }
    mExpectedRefreshRate = 0;
}


void ExternalDevice::HdcpLinkStatusListener(bool success, void *userData)
{
    if (userData == NULL) {
        return;
    }

    ExternalDevice *p = (ExternalDevice*)userData;
    p->HdcpLinkStatusListener(success);
}

void ExternalDevice::HdcpLinkStatusListener(bool success)
{
    if (mHotplugEventPending) {
        DLOGTRACE("HDCP authentication status %d, sending hotplug event...", success);
        mHwc.hotplug(mType, mConnected);
        mHotplugEventPending = false;
    }
}

void ExternalDevice::hotplugEventListener(void *data)
{
    ExternalDevice *pThis = (ExternalDevice*)data;
    if (pThis) {
        pThis->hotplugListener();
    }
}

void ExternalDevice::hotplugListener()
{
    bool ret;

    CTRACE();

    // abort mode settings if it is in the middle
    mAbortModeSettingCond.signal();

    // remember the current connection status before detection
    bool connected = mConnected;

    // detect display configs
    ret = detectDisplayConfigs();
    if (ret == false) {
        ELOGTRACE("failed to detect display config");
        return;
    }

    ILOGTRACE("hotpug event: %d", mConnected);

    if (connected == mConnected) {
        WLOGTRACE("same connection status detected, hotplug event ignored");
        return;
    }

    if (mConnected == false) {
        mHotplugEventPending = false;
        mHdcpControl->stopHdcp();
        mHwc.hotplug(mType, mConnected);
    } else {
        DLOGTRACE("start HDCP asynchronously...");
        // delay sending hotplug event till HDCP is authenticated.
        mHotplugEventPending = true;
        ret = mHdcpControl->startHdcpAsync(HdcpLinkStatusListener, this);
        if (ret == false) {
            ELOGTRACE("failed to start HDCP");
            mHotplugEventPending = false;
            mHwc.hotplug(mType, mConnected);
        }
    }
    mActiveDisplayConfig = 0;
}

void ExternalDevice::setRefreshRate(int hz)
{
    RETURN_VOID_IF_NOT_INIT();

    ILOGTRACE("setting refresh rate to %d", hz);

    if (mBlank) {
        WLOGTRACE("external device is blank");
        return;
    }

    Drm *drm = Hwcomposer::getInstance().getDrm();
    drmModeModeInfo mode;
    if (!drm->getModeInfo(IDisplayDevice::DEVICE_EXTERNAL, mode))
        return;

    if (hz == 0 && (mode.type & DRM_MODE_TYPE_PREFERRED))
        return;

    if (hz == (int)mode.vrefresh)
        return;

    if (mExpectedRefreshRate != 0 &&
            mExpectedRefreshRate == hz && mHotplugEventPending) {
        ILOGTRACE("Ignore a new refresh setting event because there is a same event is handling");
        return;
    }
    mExpectedRefreshRate = hz;

    ILOGTRACE("changing refresh rate from %d to %d", mode.vrefresh, hz);

    mHdcpControl->stopHdcp();

    drm->setRefreshRate(IDisplayDevice::DEVICE_EXTERNAL, hz);

    mHotplugEventPending = false;
    mHdcpControl->startHdcpAsync(HdcpLinkStatusListener, this);
}


bool ExternalDevice::getDisplaySize(int *width, int *height)
{
#ifndef INTEL_SUPPORT_HDMI_PRIMARY
    return PhysicalDevice::getDisplaySize(width, height);
#else
    if (mConnected)
        return PhysicalDevice::getDisplaySize(width, height);

    if (!width || !height)
        return false;

    *width = 1920;
    *height = 1080;
    return true;
#endif
}

bool ExternalDevice::getDisplayConfigs(uint32_t *configs, size_t *numConfigs)
{
#ifndef INTEL_SUPPORT_HDMI_PRIMARY
    return PhysicalDevice::getDisplayConfigs(configs, numConfigs);
#else
    if (mConnected)
        return PhysicalDevice::getDisplayConfigs(configs, numConfigs);

    if (!configs || !numConfigs)
        return false;

    *configs = 0;
    *numConfigs = 1;
    return true;
#endif
}

bool ExternalDevice::getDisplayAttributes(uint32_t config,
                                      const uint32_t *attributes,
                                      int32_t *values)
{
#ifndef INTEL_SUPPORT_HDMI_PRIMARY
    return PhysicalDevice::getDisplayAttributes(config, attributes, values);
#else
    if (mConnected)
        return PhysicalDevice::getDisplayAttributes(config, attributes, values);
    if (!attributes || !values)
        return false;
    int i = 0;
    while (attributes[i] != HWC_DISPLAY_NO_ATTRIBUTE) {
        switch (attributes[i]) {
        case HWC_DISPLAY_VSYNC_PERIOD:
            values[i] = 1e9 / 60;
            break;
        case HWC_DISPLAY_WIDTH:
            values[i] = 1920;
            break;
        case HWC_DISPLAY_HEIGHT:
            values[i] = 1080;
            break;
        case HWC_DISPLAY_DPI_X:
            values[i] = 1;
            break;
        case HWC_DISPLAY_DPI_Y:
            values[i] = 1;
            break;
        default:
            ELOGTRACE("unknown attribute %d", attributes[i]);
            break;
        }
        i++;
    }
    return true;
#endif
}

int ExternalDevice::getActiveConfig()
{
    if (!mConnected) {
        return 0;
    }
    return mActiveDisplayConfig;
}

bool ExternalDevice::setActiveConfig(int index)
{
    if (!mConnected) {
        if (index == 0)
            return true;
        else
            return false;
    }

    // for now we will only permit the frequency change.  In the future
    // we may need to set mode as well.
    if (index >= 0 && index < static_cast<int>(mDisplayConfigs.size())) {
        DisplayConfig *config = mDisplayConfigs.itemAt(index);
        setRefreshRate(config->getRefreshRate());
        mActiveDisplayConfig = index;
        return true;
    } else {
        return false;
    }
    return true;
}



} // namespace intel
} // namespace android
