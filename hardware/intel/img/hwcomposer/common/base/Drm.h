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
#ifndef __DRM_H__
#define __DRM_H__

#include <utils/Mutex.h>
#include <linux/psb_drm.h>

extern "C" {
#include "xf86drm.h"
#include "xf86drmMode.h"
}

namespace android {
namespace intel {

enum {
    PANEL_ORIENTATION_0 = 0,
    PANEL_ORIENTATION_180
};

#ifdef INTEL_SUPPORT_HDMI_PRIMARY
enum {
    DEFAULT_DRM_FB_WIDTH = 1920,
    DEFAULT_DRM_FB_HEIGHT = 1080,
};
#endif

class Drm {
public:
    Drm();
    virtual ~Drm();
public:
    bool initialize();
    void deinitialize();
    bool detect(int device);
    bool setDrmMode(int device, drmModeModeInfo& value);
    bool setRefreshRate(int device, int hz);
    bool writeReadIoctl(unsigned long cmd, void *data,
                      unsigned long size);
    bool writeIoctl(unsigned long cmd, void *data,
                      unsigned long size);
    bool readIoctl(unsigned long cmd, void *data,
                      unsigned long size);

    bool isConnected(int device);
    bool setDpmsMode(int device, int mode);
    int getDrmFd() const;
    bool getModeInfo(int device, drmModeModeInfo& mode);
    bool getPhysicalSize(int device, uint32_t& width, uint32_t& height);
    bool getDisplayResolution(int device, uint32_t& width, uint32_t& height);
    bool isSameDrmMode(drmModeModeInfoPtr mode, drmModeModeInfoPtr base) const;
    int getPanelOrientation(int device);

    drmModeModeInfoPtr detectAllConfigs(int device, int *modeCount);

private:
    bool initDrmMode(int index);
    bool setDrmMode(int index, drmModeModeInfoPtr mode);
    void resetOutput(int index);

    // map device type to output index, return -1 if not mapped
    inline int getOutputIndex(int device);

private:
    // DRM object index
    enum {
        OUTPUT_PRIMARY = 0,
        OUTPUT_EXTERNAL,
        OUTPUT_MAX,
    };

    struct DrmOutput {
        drmModeConnectorPtr connector;
        drmModeEncoderPtr encoder;
        drmModeCrtcPtr crtc;
        drmModeModeInfo mode;
        uint32_t fbHandle;
        uint32_t fbId;
        int connected;
        int panelOrientation;
    } mOutputs[OUTPUT_MAX];

    int mDrmFd;
    Mutex mLock;
    bool mInitialized;
};

} // namespace intel
} // namespace android



#endif /* __DRM_H__ */
