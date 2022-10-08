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
#include <ips/common/VsyncControl.h>

namespace android {
namespace intel {

VsyncControl::VsyncControl()
    : IVsyncControl(),
      mInitialized(false)
{
}

VsyncControl::~VsyncControl()
{
    WARN_IF_NOT_DEINIT();
}

bool VsyncControl::initialize()
{
    mInitialized = true;
    return true;
}

void VsyncControl::deinitialize()
{
    mInitialized = false;
}

bool VsyncControl::control(int disp, bool enabled)
{
    ALOGTRACE("disp = %d, enabled = %d", disp, enabled);

    struct drm_psb_vsync_set_arg arg;
    memset(&arg, 0, sizeof(struct drm_psb_vsync_set_arg));

    // pipe equals to disp
    arg.vsync.pipe = disp;

    if (enabled) {
        arg.vsync_operation_mask = VSYNC_ENABLE;
    } else {
        arg.vsync_operation_mask = VSYNC_DISABLE;
    }
    Drm *drm = Hwcomposer::getInstance().getDrm();
    return drm->writeReadIoctl(DRM_PSB_VSYNC_SET, &arg, sizeof(arg));
}

bool VsyncControl::wait(int disp, int64_t& timestamp)
{
    ALOGTRACE("disp = %d", disp);

    struct drm_psb_vsync_set_arg arg;
    memset(&arg, 0, sizeof(struct drm_psb_vsync_set_arg));

    arg.vsync_operation_mask = VSYNC_WAIT;

    // pipe equals to disp
    arg.vsync.pipe = disp;

    Drm *drm = Hwcomposer::getInstance().getDrm();
    bool ret = drm->writeReadIoctl(DRM_PSB_VSYNC_SET, &arg, sizeof(arg));
    timestamp = (int64_t)arg.vsync.timestamp;
    return ret;
}

} // namespace intel
} // namespace android
