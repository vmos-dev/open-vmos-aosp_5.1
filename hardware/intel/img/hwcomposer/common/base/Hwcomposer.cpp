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
#include <common/utils/Dump.h>
#include <UeventObserver.h>

namespace android {
namespace intel {

Hwcomposer* Hwcomposer::sInstance(0);

Hwcomposer::Hwcomposer()
    : mProcs(0),
      mDrm(0),
      mPlaneManager(0),
      mBufferManager(0),
      mDisplayAnalyzer(0),
      mDisplayContext(0),
      mUeventObserver(0),
      mInitialized(false)
{
    CTRACE();

    mDisplayDevices.setCapacity(IDisplayDevice::DEVICE_COUNT);
    mDisplayDevices.clear();
}

Hwcomposer::~Hwcomposer()
{
    CTRACE();
    deinitialize();
}

bool Hwcomposer::initCheck() const
{
    return mInitialized;
}

bool Hwcomposer::prepare(size_t numDisplays,
                          hwc_display_contents_1_t** displays)
{
    bool ret = true;

    RETURN_FALSE_IF_NOT_INIT();
    ALOGTRACE("display count = %d", numDisplays);

    if (!numDisplays || !displays) {
        ELOGTRACE("invalid parameters");
        return false;
    }

    mDisplayAnalyzer->analyzeContents(numDisplays, displays);

    // disable reclaimed planes
    mPlaneManager->disableReclaimedPlanes();

    // reclaim all allocated planes if possible
    for (size_t i = 0; i < numDisplays; i++) {
        if (i >= mDisplayDevices.size()) {
            continue;
        }
        IDisplayDevice *device = mDisplayDevices.itemAt(i);
        if (!device) {
            VLOGTRACE("device %d doesn't exist", i);
            continue;
        }
        device->prePrepare(displays[i]);
    }

    for (size_t i = 0; i < numDisplays; i++) {
        if (i >= mDisplayDevices.size()) {
            continue;
        }
        IDisplayDevice *device = mDisplayDevices.itemAt(i);
        if (!device) {
            VLOGTRACE("device %d doesn't exist", i);
            continue;
        }
        ret = device->prepare(displays[i]);
        if (ret == false) {
            ELOGTRACE("failed to do prepare for device %d", i);
            continue;
        }
    }

    return ret;
}

bool Hwcomposer::commit(size_t numDisplays,
                         hwc_display_contents_1_t **displays)
{
    bool ret = true;

    RETURN_FALSE_IF_NOT_INIT();
    ALOGTRACE("display count = %d", numDisplays);

    if (!numDisplays || !displays) {
        ELOGTRACE("invalid parameters");
        return false;
    }

    mDisplayContext->commitBegin(numDisplays, displays);

    for (size_t i = 0; i < numDisplays; i++) {
        if (i >= mDisplayDevices.size()) {
            continue;
        }
        IDisplayDevice *device = mDisplayDevices.itemAt(i);
        if (!device) {
            VLOGTRACE("device %d doesn't exist", i);
            continue;
        }

        if (!device->isConnected()) {
            VLOGTRACE("device %d is disconnected", i);
            continue;
        }

        ret = device->commit(displays[i], mDisplayContext);
        if (ret == false) {
            ELOGTRACE("failed to do commit for device %d", i);
            continue;
        }
    }

    mDisplayContext->commitEnd(numDisplays, displays);
    // return true always
    return true;
}

bool Hwcomposer::vsyncControl(int disp, int enabled)
{
    RETURN_FALSE_IF_NOT_INIT();
    ALOGTRACE("disp = %d, enabled = %d", disp, enabled);

    if (disp < 0 || disp >= IDisplayDevice::DEVICE_COUNT) {
        ELOGTRACE("invalid disp %d", disp);
        return false;
    }
    if (disp >= (int) mDisplayDevices.size()) {
        return false;
    }
    IDisplayDevice *device = mDisplayDevices.itemAt(disp);
    if (!device) {
        ELOGTRACE("no device found");
        return false;
    }

    return device->vsyncControl(enabled ? true : false);
}

bool Hwcomposer::blank(int disp, int blank)
{
    RETURN_FALSE_IF_NOT_INIT();
    ALOGTRACE("disp = %d, blank = %d", disp, blank);

    if (disp < 0 || disp >= IDisplayDevice::DEVICE_COUNT) {
        ELOGTRACE("invalid disp %d", disp);
        return false;
    }
    if (disp >= (int) mDisplayDevices.size()) {
        return false;
    }
    IDisplayDevice *device = mDisplayDevices.itemAt(disp);
    if (!device) {
        ELOGTRACE("no device found");
        return false;
    }

    return device->blank(blank ? true : false);
}

bool Hwcomposer::getDisplayConfigs(int disp,
                                      uint32_t *configs,
                                      size_t *numConfigs)
{
    RETURN_FALSE_IF_NOT_INIT();

    if (disp < 0 || disp >= IDisplayDevice::DEVICE_COUNT) {
        ELOGTRACE("invalid disp %d", disp);
        return false;
    }
    if (disp >= (int) mDisplayDevices.size()) {
        return false;
    }
    IDisplayDevice *device = mDisplayDevices.itemAt(disp);
    if (!device) {
        ELOGTRACE("no device %d found", disp);
        return false;
    }

    return device->getDisplayConfigs(configs, numConfigs);
}

bool Hwcomposer::getDisplayAttributes(int disp,
                                         uint32_t config,
                                         const uint32_t *attributes,
                                         int32_t *values)
{
    RETURN_FALSE_IF_NOT_INIT();

    if (disp < 0 || disp >= IDisplayDevice::DEVICE_COUNT) {
        ELOGTRACE("invalid disp %d", disp);
        return false;
    }
    if (disp >= (int) mDisplayDevices.size()) {
        return false;
    }
    IDisplayDevice *device = mDisplayDevices.itemAt(disp);
    if (!device) {
        ELOGTRACE("no device found");
        return false;
    }

    return device->getDisplayAttributes(config, attributes, values);
}

bool Hwcomposer::compositionComplete(int disp)
{
    RETURN_FALSE_IF_NOT_INIT();

    if (disp < 0 || disp >= IDisplayDevice::DEVICE_COUNT) {
        ELOGTRACE("invalid disp %d", disp);
        return false;
    }

    mDisplayContext->compositionComplete();
    if (disp >= (int) mDisplayDevices.size()) {
        return false;
    }

    IDisplayDevice *device = mDisplayDevices.itemAt(disp);
    if (!device) {
        ELOGTRACE("no device found");
        return false;
    }

    return device->compositionComplete();
}

bool Hwcomposer::setPowerMode(int disp, int mode)
{
    RETURN_FALSE_IF_NOT_INIT();

    if (disp < 0 || disp >= IDisplayDevice::DEVICE_COUNT) {
        ELOGTRACE("invalid disp %d", disp);
        return false;
    }

    IDisplayDevice *device = mDisplayDevices.itemAt(disp);
    if (!device) {
        ELOGTRACE("no device found");
        return false;
    }

    return device->setPowerMode(mode);
}

int Hwcomposer::getActiveConfig(int disp)
{
    RETURN_NULL_IF_NOT_INIT();

    if (disp < 0 || disp >= IDisplayDevice::DEVICE_COUNT) {
        ELOGTRACE("invalid disp %d", disp);
        return -1;
    }

    IDisplayDevice *device = mDisplayDevices.itemAt(disp);
    if (!device) {
        ELOGTRACE("no device found");
        return -1;
    }

    return device->getActiveConfig();
}

bool Hwcomposer::setActiveConfig(int disp, int index)
{
    RETURN_FALSE_IF_NOT_INIT();

    if (disp < 0 || disp >= IDisplayDevice::DEVICE_COUNT) {
        ELOGTRACE("invalid disp %d", disp);
        return false;
    }

    IDisplayDevice *device = mDisplayDevices.itemAt(disp);
    if (!device) {
        ELOGTRACE("no device found");
        return false;
    }

    return device->setActiveConfig(index);
}

bool Hwcomposer::setCursorPositionAsync(int disp, int x, int y)
{
    RETURN_FALSE_IF_NOT_INIT();

    if (disp != HWC_DISPLAY_PRIMARY && disp != HWC_DISPLAY_EXTERNAL) {
        ELOGTRACE("invalid disp %d", disp);
        return false;
    }

    return mDisplayContext->setCursorPosition(disp, x, y);
}

void Hwcomposer::vsync(int disp, int64_t timestamp)
{
    RETURN_VOID_IF_NOT_INIT();

    if (mProcs && mProcs->vsync) {
        VLOGTRACE("report vsync on disp %d, timestamp %llu", disp, timestamp);
        // workaround to pretend vsync is from primary display
        // Display will freeze if vsync is from external display.
        mProcs->vsync(const_cast<hwc_procs_t*>(mProcs), IDisplayDevice::DEVICE_PRIMARY, timestamp);
    }
}

void Hwcomposer::hotplug(__attribute__((unused))int disp, bool connected)
{
    RETURN_VOID_IF_NOT_INIT();

#ifndef INTEL_SUPPORT_HDMI_PRIMARY
    if (mProcs && mProcs->hotplug) {
        DLOGTRACE("report hotplug on disp %d, connected %d", disp, connected);
        mProcs->hotplug(const_cast<hwc_procs_t*>(mProcs), disp, connected);
        DLOGTRACE("hotplug callback processed and returned!");
    }
#endif

    mDisplayAnalyzer->postHotplugEvent(connected);
}

void Hwcomposer::invalidate()
{
    RETURN_VOID_IF_NOT_INIT();

    if (mProcs && mProcs->invalidate) {
        DLOGTRACE("invalidating screen...");
        mProcs->invalidate(const_cast<hwc_procs_t*>(mProcs));
    }
}

bool Hwcomposer::release()
{
    RETURN_FALSE_IF_NOT_INIT();

    return true;
}

bool Hwcomposer::dump(char *buff, int buff_len, int * /* cur_len */)
{
    RETURN_FALSE_IF_NOT_INIT();

    Dump d(buff, buff_len);

    // dump composer status
    d.append("Hardware Composer state:");
    // dump device status
    for (size_t i= 0; i < mDisplayDevices.size(); i++) {
        IDisplayDevice *device = mDisplayDevices.itemAt(i);
        if (device)
            device->dump(d);
    }

    // dump plane manager status
    if (mPlaneManager)
        mPlaneManager->dump(d);

    // dump buffer manager status
    if (mBufferManager)
        mBufferManager->dump(d);

    return true;
}

void Hwcomposer::registerProcs(hwc_procs_t const *procs)
{
    CTRACE();

    if (!procs) {
        WLOGTRACE("procs is NULL");
    }
    mProcs = procs;
}

bool Hwcomposer::initialize()
{
    CTRACE();

    // create drm
    mDrm = new Drm();
    if (!mDrm || !mDrm->initialize()) {
        DEINIT_AND_RETURN_FALSE("failed to create DRM");
    }

    // create buffer manager
    mBufferManager = createBufferManager();
    if (!mBufferManager || !mBufferManager->initialize()) {
        DEINIT_AND_RETURN_FALSE("failed to create buffer manager");
    }

    // create display plane manager
    mPlaneManager = createDisplayPlaneManager();
    if (!mPlaneManager || !mPlaneManager->initialize()) {
        DEINIT_AND_RETURN_FALSE("failed to create display plane manager");
    }

    mDisplayContext = createDisplayContext();
    if (!mDisplayContext || !mDisplayContext->initialize()) {
        DEINIT_AND_RETURN_FALSE("failed to create display context");
    }

    mUeventObserver = new UeventObserver();
    if (!mUeventObserver || !mUeventObserver->initialize()) {
        DEINIT_AND_RETURN_FALSE("failed to initialize uevent observer");
    }

    // create display device
    for (int i = 0; i < IDisplayDevice::DEVICE_COUNT; i++) {
        IDisplayDevice *device = createDisplayDevice(i, *mPlaneManager);
        if (!device || !device->initialize()) {
            DEINIT_AND_DELETE_OBJ(device);
            DEINIT_AND_RETURN_FALSE("failed to create device %d", i);
        }
        // add this device
        mDisplayDevices.insertAt(device, i, 1);
    }

    mDisplayAnalyzer = new DisplayAnalyzer();
    if (!mDisplayAnalyzer || !mDisplayAnalyzer->initialize()) {
        DEINIT_AND_RETURN_FALSE("failed to initialize display analyzer");
    }

    // all initialized, starting uevent observer
    mUeventObserver->start();

    mInitialized = true;
    return true;
}

void Hwcomposer::deinitialize()
{
    DEINIT_AND_DELETE_OBJ(mDisplayAnalyzer);

    DEINIT_AND_DELETE_OBJ(mUeventObserver);
    // destroy display devices
    for (size_t i = 0; i < mDisplayDevices.size(); i++) {
        IDisplayDevice *device = mDisplayDevices.itemAt(i);
        DEINIT_AND_DELETE_OBJ(device);
    }
    mDisplayDevices.clear();

    DEINIT_AND_DELETE_OBJ(mDisplayContext);
    DEINIT_AND_DELETE_OBJ(mPlaneManager);
    DEINIT_AND_DELETE_OBJ(mBufferManager);
    DEINIT_AND_DELETE_OBJ(mDrm);
    mInitialized = false;
}

Drm* Hwcomposer::getDrm()
{
    return mDrm;
}

DisplayPlaneManager* Hwcomposer::getPlaneManager()
{
    return mPlaneManager;
}

BufferManager* Hwcomposer::getBufferManager()
{
    return mBufferManager;
}

IDisplayContext* Hwcomposer::getDisplayContext()
{
    return mDisplayContext;
}

DisplayAnalyzer* Hwcomposer::getDisplayAnalyzer()
{
    return mDisplayAnalyzer;
}

IDisplayDevice* Hwcomposer::getDisplayDevice(int disp)
{
    if (disp < 0 || disp >= IDisplayDevice::DEVICE_COUNT) {
        ELOGTRACE("invalid disp %d", disp);
        return NULL;
    }
    if (disp >= (int) mDisplayDevices.size()) {
        return NULL;
    }
    return mDisplayDevices.itemAt(disp);
}

UeventObserver* Hwcomposer::getUeventObserver()
{
    return mUeventObserver;
}


} // namespace intel
} // namespace android
