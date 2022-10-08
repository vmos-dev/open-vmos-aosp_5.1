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
#include <fcntl.h>
#include <errno.h>
#include <common/utils/HwcTrace.h>
#include <IDisplayDevice.h>
#include <DrmConfig.h>
#include <common/base/Drm.h>
#include <Hwcomposer.h>

namespace android {
namespace intel {

Drm::Drm()
    : mDrmFd(0),
      mLock(),
      mInitialized(false)
{
    memset(&mOutputs, 0, sizeof(mOutputs));
}

Drm::~Drm()
{
    WARN_IF_NOT_DEINIT();
}

bool Drm::initialize()
{
    if (mInitialized) {
        WLOGTRACE("Drm object has been initialized");
        return true;
    }

    const char *path = DrmConfig::getDrmPath();
    mDrmFd = open(path, O_RDWR, 0);
    if (mDrmFd < 0) {
        ELOGTRACE("failed to open Drm, error: %s", strerror(errno));
        return false;
    }
    DLOGTRACE("mDrmFd = %d", mDrmFd);

    memset(&mOutputs, 0, sizeof(mOutputs));
    mInitialized = true;
    return true;
}

void Drm::deinitialize()
{
    for (int i = 0; i < OUTPUT_MAX; i++) {
        resetOutput(i);
    }

    if (mDrmFd) {
        close(mDrmFd);
        mDrmFd = 0;
    }
    mInitialized = false;
}

bool Drm::detect(int device)
{
    RETURN_FALSE_IF_NOT_INIT();

    Mutex::Autolock _l(mLock);
    int outputIndex = getOutputIndex(device);
    if (outputIndex < 0 ) {
        return false;
    }

    resetOutput(outputIndex);

    // get drm resources
    drmModeResPtr resources = drmModeGetResources(mDrmFd);
    if (!resources) {
        ELOGTRACE("fail to get drm resources, error: %s", strerror(errno));
        return false;
    }

    drmModeConnectorPtr connector = NULL;
    DrmOutput *output = &mOutputs[outputIndex];
    bool ret = false;

    // find connector for the given device
    for (int i = 0; i < resources->count_connectors; i++) {
        if (!resources->connectors || !resources->connectors[i]) {
            ELOGTRACE("fail to get drm resources connectors, error: %s", strerror(errno));
            continue;
        }

        connector = drmModeGetConnector(mDrmFd, resources->connectors[i]);
        if (!connector) {
            ELOGTRACE("drmModeGetConnector failed");
            continue;
        }

        if (connector->connector_type != DrmConfig::getDrmConnector(device)) {
            drmModeFreeConnector(connector);
            continue;
        }

        if (connector->connection != DRM_MODE_CONNECTED) {
            ILOGTRACE("device %d is not connected", device);
            drmModeFreeConnector(connector);
            ret = true;
            break;
        }

        output->connector = connector;
        output->connected = true;

        // get proper encoder for the given connector
        if (connector->encoder_id) {
            ILOGTRACE("Drm connector has encoder attached on device %d", device);
            output->encoder = drmModeGetEncoder(mDrmFd, connector->encoder_id);
            if (!output->encoder) {
                ELOGTRACE("failed to get encoder from a known encoder id");
                // fall through to get an encoder
            }
        }
        if (!output->encoder) {
            ILOGTRACE("getting encoder for device %d", device);
            drmModeEncoderPtr encoder;
            for (int j = 0; j < resources->count_encoders; j++) {
                if (!resources->encoders || !resources->encoders[j]) {
                    ELOGTRACE("fail to get drm resources encoders, error: %s", strerror(errno));
                    continue;
                }

                encoder = drmModeGetEncoder(mDrmFd, resources->encoders[i]);
                if (!encoder) {
                    ELOGTRACE("drmModeGetEncoder failed");
                    continue;
                }
                if (encoder->encoder_type == DrmConfig::getDrmEncoder(device)) {
                    output->encoder = encoder;
                    break;
                }
                drmModeFreeEncoder(encoder);
                encoder = NULL;
            }
        }
        if (!output->encoder) {
            ELOGTRACE("failed to get drm encoder");
            break;
        }

        // get an attached crtc or spare crtc
        if (output->encoder->crtc_id) {
            ILOGTRACE("Drm encoder has crtc attached on device %d", device);
            output->crtc = drmModeGetCrtc(mDrmFd, output->encoder->crtc_id);
            if (!output->crtc) {
                ELOGTRACE("failed to get crtc from a known crtc id");
                // fall through to get a spare crtc
            }
        }
        if (!output->crtc) {
            ILOGTRACE("getting crtc for device %d", device);
            drmModeCrtcPtr crtc;
            for (int j = 0; j < resources->count_crtcs; j++) {
                if (!resources->crtcs || !resources->crtcs[j]) {
                    ELOGTRACE("fail to get drm resources crtcs, error: %s", strerror(errno));
                    continue;
                }

                crtc = drmModeGetCrtc(mDrmFd, resources->crtcs[j]);
                if (!crtc) {
                    ELOGTRACE("drmModeGetCrtc failed");
                    continue;
                }
                // check if legal crtc to the encoder
                if (output->encoder->possible_crtcs & (1<<j)) {
                    if (crtc->buffer_id == 0) {
                        output->crtc = crtc;
                        break;
                    }
                }
                drmModeFreeCrtc(crtc);
            }
        }
        if (!output->crtc) {
            ELOGTRACE("failed to get drm crtc");
            break;
        }

        // current mode
        if (output->crtc->mode_valid) {
            ILOGTRACE("mode is valid, kernel mode settings");
            memcpy(&output->mode, &output->crtc->mode, sizeof(drmModeModeInfo));
            ret = true;
        } else {
            ELOGTRACE("mode is invalid. Kernel mode setting is not completed");
            ret = false;
        }

        if (outputIndex == OUTPUT_PRIMARY) {
            if (!readIoctl(DRM_PSB_PANEL_ORIENTATION, &output->panelOrientation, sizeof(int))) {
                ELOGTRACE("failed to get device %d orientation", device);
                output->panelOrientation = PANEL_ORIENTATION_0;
            }
        } else {
            output->panelOrientation = PANEL_ORIENTATION_0;
        }
        break;
    }

    if (!ret) {
        if (output->connector == NULL && outputIndex != OUTPUT_PRIMARY) {
            // a fatal failure on primary device
            // non fatal on secondary device
            WLOGTRACE("device %d is disabled?", device);
            ret = true;
        }
         resetOutput(outputIndex);
    } else if (output->connected) {
        ILOGTRACE("mode is: %dx%d@%dHz", output->mode.hdisplay, output->mode.vdisplay, output->mode.vrefresh);
    }

    drmModeFreeResources(resources);
    return ret;
}

bool Drm::isSameDrmMode(drmModeModeInfoPtr value,
        drmModeModeInfoPtr base) const
{
    if (base->hdisplay == value->hdisplay &&
        base->vdisplay == value->vdisplay &&
        base->vrefresh == value->vrefresh &&
        (base->flags & value->flags) == value->flags) {
        VLOGTRACE("Drm mode is not changed");
        return true;
    }

    return false;
}

bool Drm::setDrmMode(int device, drmModeModeInfo& value)
{
    RETURN_FALSE_IF_NOT_INIT();
    Mutex::Autolock _l(mLock);

    if (device != IDisplayDevice::DEVICE_EXTERNAL) {
        WLOGTRACE("Setting mode on invalid device %d", device);
        return false;
    }

    int outputIndex = getOutputIndex(device);
    if (outputIndex < 0 ) {
        ELOGTRACE("invalid device");
        return false;
    }

    DrmOutput *output= &mOutputs[outputIndex];
    if (!output->connected) {
        ELOGTRACE("device is not connected");
        return false;
    }

    if (output->connector->count_modes <= 0) {
        ELOGTRACE("invalid count of modes");
        return false;
    }

    drmModeModeInfoPtr mode;
    int index = 0;
    for (int i = 0; i < output->connector->count_modes; i++) {
        mode = &output->connector->modes[i];
        if (mode->type & DRM_MODE_TYPE_PREFERRED) {
            index = i;
        }
        if (isSameDrmMode(&value, mode)) {
            index = i;
            break;
        }
    }

    mode = &output->connector->modes[index];
    return setDrmMode(outputIndex, mode);
}

bool Drm::setRefreshRate(int device, int hz)
{
    RETURN_FALSE_IF_NOT_INIT();
    Mutex::Autolock _l(mLock);

    if (device != IDisplayDevice::DEVICE_EXTERNAL) {
        WLOGTRACE("Setting mode on invalid device %d", device);
        return false;
    }

    int outputIndex = getOutputIndex(device);
    if (outputIndex < 0 ) {
        ELOGTRACE("invalid device");
        return false;
    }

    DrmOutput *output= &mOutputs[outputIndex];
    if (!output->connected) {
        ELOGTRACE("device is not connected");
        return false;
    }

    if (output->connector->count_modes <= 0) {
        ELOGTRACE("invalid count of modes");
        return false;
    }

    drmModeModeInfoPtr mode;
    int index = 0;
    for (int i = 0; i < output->connector->count_modes; i++) {
        mode = &output->connector->modes[i];
        if (mode->type & DRM_MODE_TYPE_PREFERRED) {
            index = i;
        }
        if (mode->hdisplay == output->mode.hdisplay &&
            mode->vdisplay == output->mode.vdisplay &&
            mode->vrefresh == (uint32_t)hz) {
            index = i;
            break;
        }
    }

    mode = &output->connector->modes[index];
    return setDrmMode(outputIndex, mode);
}

bool Drm::writeReadIoctl(unsigned long cmd, void *data,
                           unsigned long size)
{
    int err;

    if (mDrmFd <= 0) {
        ELOGTRACE("drm is not initialized");
        return false;
    }

    if (!data || !size) {
        ELOGTRACE("invalid parameters");
        return false;
    }

    err = drmCommandWriteRead(mDrmFd, cmd, data, size);
    if (err) {
        WLOGTRACE("failed to call %ld ioctl with failure %d", cmd, err);
        return false;
    }

    return true;
}

bool Drm::writeIoctl(unsigned long cmd, void *data,
                       unsigned long size)
{
    int err;

    if (mDrmFd <= 0) {
        ELOGTRACE("drm is not initialized");
        return false;
    }

    if (!data || !size) {
        ELOGTRACE("invalid parameters");
        return false;
    }

    err = drmCommandWrite(mDrmFd, cmd, data, size);
    if (err) {
        WLOGTRACE("failed to call %ld ioctl with failure %d", cmd, err);
        return false;
    }

    return true;
}


bool Drm::readIoctl(unsigned long cmd, void *data,
                       unsigned long size)
{
    int err;

    if (mDrmFd <= 0) {
        ELOGTRACE("drm is not initialized");
        return false;
    }

    if (!data || !size) {
        ELOGTRACE("invalid parameters");
        return false;
    }

    err = drmCommandRead(mDrmFd, cmd, data, size);
    if (err) {
        WLOGTRACE("failed to call %ld ioctl with failure %d", cmd, err);
        return false;
    }

    return true;
}


int Drm::getDrmFd() const
{
    return mDrmFd;
}

bool Drm::getModeInfo(int device, drmModeModeInfo& mode)
{
    Mutex::Autolock _l(mLock);

    int outputIndex = getOutputIndex(device);
    if (outputIndex < 0 ) {
        return false;
    }

    DrmOutput *output= &mOutputs[outputIndex];
    if (output->connected == false) {
        ELOGTRACE("device is not connected");
        return false;
    }

    if (output->mode.hdisplay == 0 || output->mode.vdisplay == 0) {
        ELOGTRACE("invalid width or height");
        return false;
    }

    memcpy(&mode, &output->mode, sizeof(drmModeModeInfo));

#ifdef INTEL_SUPPORT_HDMI_PRIMARY
    // FIXME: use default fb size instead of hdmi mode, because to
    // support hdmi primary, we cannot report dynamic mode to SF.
    mode.hdisplay = DEFAULT_DRM_FB_WIDTH;
    mode.vdisplay = DEFAULT_DRM_FB_HEIGHT;
#endif

    return true;
}

bool Drm::getPhysicalSize(int device, uint32_t& width, uint32_t& height)
{
    Mutex::Autolock _l(mLock);

    int outputIndex = getOutputIndex(device);
    if (outputIndex < 0 ) {
        return false;
    }

    DrmOutput *output= &mOutputs[outputIndex];
    if (output->connected == false) {
        ELOGTRACE("device is not connected");
        return false;
    }

    width = output->connector->mmWidth;
    height = output->connector->mmHeight;
    return true;
}

bool Drm::getDisplayResolution(int device, uint32_t& width, uint32_t& height)
{
    Mutex::Autolock _l(mLock);

    int outputIndex = getOutputIndex(device);
    if (outputIndex < 0) {
        return false;
    }

    DrmOutput *output= &mOutputs[outputIndex];
    if (output->connected == false) {
        ELOGTRACE("device is not connected");
        return false;
    }

    width = output->mode.hdisplay;
    height = output->mode.vdisplay;

    if (!width || !height) {
        ELOGTRACE("invalid width or height");
        return false;
    }
    return true;
}

bool Drm::isConnected(int device)
{
    Mutex::Autolock _l(mLock);

    int output = getOutputIndex(device);
    if (output < 0 ) {
        return false;
    }

    return mOutputs[output].connected;
}

bool Drm::setDpmsMode(int device, int mode)
{
    Mutex::Autolock _l(mLock);

    int output = getOutputIndex(device);
    if (output < 0 ) {
        return false;
    }

    if (mode != IDisplayDevice::DEVICE_DISPLAY_OFF &&
            mode != IDisplayDevice::DEVICE_DISPLAY_STANDBY &&
            mode != IDisplayDevice::DEVICE_DISPLAY_ON) {
        ELOGTRACE("invalid mode %d", mode);
        return false;
    }

    DrmOutput *out = &mOutputs[output];
    if (!out->connected) {
        ELOGTRACE("device is not connected");
        return false;
    }

    drmModePropertyPtr props;
    for (int i = 0; i < out->connector->count_props; i++) {
        props = drmModeGetProperty(mDrmFd, out->connector->props[i]);
        if (!props) {
            continue;
        }

        if (strcmp(props->name, "DPMS") == 0) {
            int ret = drmModeConnectorSetProperty(
                mDrmFd,
                out->connector->connector_id,
                props->prop_id,
                (mode == IDisplayDevice::DEVICE_DISPLAY_ON) ? DRM_MODE_DPMS_ON :
                        IDisplayDevice::DEVICE_DISPLAY_STANDBY == mode ?
                        DRM_MODE_DPMS_STANDBY : DRM_MODE_DPMS_OFF);
            drmModeFreeProperty(props);
            if (ret != 0) {
                ELOGTRACE("unable to set DPMS %d", mode);
                return false;
            } else {
                return true;
            }
        }
        drmModeFreeProperty(props);
    }
    return false;
}

void Drm::resetOutput(int index)
{
    DrmOutput *output = &mOutputs[index];

    output->connected = false;
    memset(&output->mode, 0, sizeof(drmModeModeInfo));

    if (output->connector) {
        drmModeFreeConnector(output->connector);
        output->connector = 0;
    }
    if (output->encoder) {
        drmModeFreeEncoder(output->encoder);
        output->encoder = 0;
    }
    if (output->crtc) {
        drmModeFreeCrtc(output->crtc);
        output->crtc = 0;
    }
    if (output->fbId) {
        drmModeRmFB(mDrmFd, output->fbId);
        output->fbId = 0;
    }
    if (output->fbHandle) {
        Hwcomposer::getInstance().getBufferManager()->freeFrameBuffer(output->fbHandle);
        output->fbHandle = 0;
    }
}

bool Drm::initDrmMode(int outputIndex)
{
    DrmOutput *output= &mOutputs[outputIndex];
    if (output->connector->count_modes <= 0) {
        ELOGTRACE("invalid count of modes");
        return false;
    }

    drmModeModeInfoPtr mode;
    int index = 0;
    for (int i = 0; i < output->connector->count_modes; i++) {
        mode = &output->connector->modes[i];
        if (mode->type & DRM_MODE_TYPE_PREFERRED) {
            index = i;
            break;
        }
    }

    return setDrmMode(outputIndex, &output->connector->modes[index]);
}

bool Drm::setDrmMode(int index, drmModeModeInfoPtr mode)
{
    DrmOutput *output = &mOutputs[index];

    int oldFbId =0;
    int oldFbHandle = 0;

    drmModeModeInfo currentMode;
    memcpy(&currentMode, &output->mode, sizeof(drmModeModeInfo));

    if (isSameDrmMode(mode, &currentMode))
        return true;


    if (output->fbId) {
        oldFbId = output->fbId ;
        output->fbId = 0;
    }

    if (output->fbHandle) {
        oldFbHandle = output->fbHandle;
        output->fbHandle = 0;
    }

    // allocate frame buffer
    int stride = 0;
#ifdef INTEL_SUPPORT_HDMI_PRIMARY
    output->fbHandle = Hwcomposer::getInstance().getBufferManager()->allocFrameBuffer(
        DEFAULT_DRM_FB_WIDTH, DEFAULT_DRM_FB_HEIGHT, &stride);
#else
    output->fbHandle = Hwcomposer::getInstance().getBufferManager()->allocFrameBuffer(
        mode->hdisplay, mode->vdisplay, &stride);
#endif
    if (output->fbHandle == 0) {
        ELOGTRACE("failed to allocate frame buffer");
        return false;
    }

    int ret = 0;
    ret = drmModeAddFB(
        mDrmFd,
#ifdef INTEL_SUPPORT_HDMI_PRIMARY
        DEFAULT_DRM_FB_WIDTH,
        DEFAULT_DRM_FB_HEIGHT,
#else
        mode->hdisplay,
        mode->vdisplay,
#endif
        DrmConfig::getFrameBufferDepth(),
        DrmConfig::getFrameBufferBpp(),
        stride,
        output->fbHandle,
        &output->fbId);
    if (ret != 0) {
        ELOGTRACE("drmModeAddFB failed, error: %d", ret);
        return false;
    }

    ILOGTRACE("mode set: %dx%d@%dHz", mode->hdisplay, mode->vdisplay, mode->vrefresh);

    ret = drmModeSetCrtc(mDrmFd, output->crtc->crtc_id, output->fbId, 0, 0,
                   &output->connector->connector_id, 1, mode);
    if (ret == 0) {
        //save mode
        memcpy(&output->mode, mode, sizeof(drmModeModeInfo));
    } else {
        ELOGTRACE("drmModeSetCrtc failed. error: %d", ret);
    }

    if (oldFbId) {
        drmModeRmFB(mDrmFd, oldFbId);
    }

    if (oldFbHandle) {
        Hwcomposer::getInstance().getBufferManager()->freeFrameBuffer(oldFbHandle);
    }

    return ret == 0;
}

int Drm::getOutputIndex(int device)
{
    switch (device) {
    case IDisplayDevice::DEVICE_PRIMARY:
        return OUTPUT_PRIMARY;
    case IDisplayDevice::DEVICE_EXTERNAL:
        return OUTPUT_EXTERNAL;
    default:
        ELOGTRACE("invalid display device");
        break;
    }

    return -1;
}

int Drm::getPanelOrientation(int device)
{
    int outputIndex = getOutputIndex(device);
    if (outputIndex < 0) {
        ELOGTRACE("invalid device");
        return PANEL_ORIENTATION_0;
    }

    DrmOutput *output= &mOutputs[outputIndex];
    if (output->connected == false) {
        ELOGTRACE("device is not connected");
        return PANEL_ORIENTATION_0;
    }

    return output->panelOrientation;
}

// HWC 1.4 requires that we return all of the compatible configs in getDisplayConfigs
// this is needed so getActiveConfig/setActiveConfig work correctly.  It is up to the
// user space to decide what speed to send.
drmModeModeInfoPtr Drm::detectAllConfigs(int device, int *modeCount)
{
    RETURN_NULL_IF_NOT_INIT();
    Mutex::Autolock _l(mLock);

    if (modeCount != NULL)
        *modeCount = 0;
    else
        return NULL;

    int outputIndex = getOutputIndex(device);
    if (outputIndex < 0) {
        ELOGTRACE("invalid device");
        return NULL;
    }

    DrmOutput *output= &mOutputs[outputIndex];
    if (!output->connected) {
        ELOGTRACE("device is not connected");
        return NULL;
    }

    if (output->connector->count_modes <= 0) {
        ELOGTRACE("invalid count of modes");
        return NULL;
    }

    *modeCount = output->connector->count_modes;
    return output->connector->modes;
}

} // namespace intel
} // namespace android

