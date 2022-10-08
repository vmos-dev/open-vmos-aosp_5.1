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
#include <DisplayPlane.h>
#include <hal_public.h>
#include <DisplayQuery.h>
#include <khronos/openmax/OMX_IntelVideoExt.h>
#include <Hwcomposer.h>


namespace android {
namespace intel {

bool DisplayQuery::isVideoFormat(uint32_t format)
{
    switch (format) {
    case OMX_INTEL_COLOR_FormatYUV420PackedSemiPlanar:
    case OMX_INTEL_COLOR_FormatYUV420PackedSemiPlanar_Tiled:
    // Expand format to support the case: Software decoder + HW rendering
    // Only VP9 use this foramt now
    case HAL_PIXEL_FORMAT_YV12:
        return true;
    default:
        return false;
    }
}

int DisplayQuery::getOverlayLumaStrideAlignment(uint32_t format)
{
    // both luma and chroma stride need to be 64-byte aligned for overlay
    switch (format) {
    case HAL_PIXEL_FORMAT_YV12:
    case HAL_PIXEL_FORMAT_I420:
        // for these two formats, chroma stride is calculated as half of luma stride
        // so luma stride needs to be 128-byte aligned.
        return 128;
    default:
        return 64;
    }
}

uint32_t DisplayQuery::queryNV12Format()
{
    return HAL_PIXEL_FORMAT_NV12;
}

bool DisplayQuery::forceFbScaling(int device)
{
    // RGB planes don't support scaling. Panel fitter can be used to scale frame buffer to device's resolution
    // if scaling factor is less than 1.5. Otherwise, GPU scaling is needed on RGB buffer, or hardware overlay
    // scaling is needed on NV12 buffer

    // this needs to work together with kernel driver. Panel fitter needs to be disabled if scaling factor is greater
    // than 1.5

    uint32_t width, height;
    if (!Hwcomposer::getInstance().getDrm()->getDisplayResolution(device, width, height)) {
        return false;
    }

    if (DEFAULT_DRM_FB_WIDTH / (float)width > 1.5) {
        return true;
    }

    if (DEFAULT_DRM_FB_HEIGHT / (float)height > 1.5) {
        return true;
    }

    return false;
}

} // namespace intel
} // namespace android

