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
#include <linux/psb_drm.h>
#include <Hwcomposer.h>
#include <ips/common/DrmControl.h>

namespace android {
namespace intel {

DrmControl::DrmControl()
    : mVideoExtCommand(0)
{
}

DrmControl::~DrmControl()
{
}

int DrmControl::getVideoExtCommand()
{
    if (mVideoExtCommand) {
        return mVideoExtCommand;
    }

    int fd = Hwcomposer::getInstance().getDrm()->getDrmFd();

    union drm_psb_extension_arg video_getparam_arg;
    strncpy(video_getparam_arg.extension,
            "lnc_video_getparam", sizeof(video_getparam_arg.extension));
    int ret = drmCommandWriteRead(fd, DRM_PSB_EXTENSION,
            &video_getparam_arg, sizeof(video_getparam_arg));
    if (ret != 0) {
        VLOGTRACE("failed to get video extension command");
        return 0;
    }

    mVideoExtCommand = video_getparam_arg.rep.driver_ioctl_offset;

    return mVideoExtCommand;
}

} // namespace intel
} // namespace android
