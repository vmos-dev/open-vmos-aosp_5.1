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
#include <ips/common/GrallocBufferBase.h>
#include <DisplayQuery.h>
#include <khronos/openmax/OMX_IntelVideoExt.h>
#include <hal_public.h>

namespace android {
namespace intel {

GrallocBufferBase::GrallocBufferBase(uint32_t handle)
    : GraphicBuffer(handle)
{
    ALOGTRACE("handle = %#x", handle);
    initBuffer(handle);
}

void GrallocBufferBase::resetBuffer(uint32_t handle)
{
    GraphicBuffer::resetBuffer(handle);
    initBuffer(handle);
}

void GrallocBufferBase::initBuffer(uint32_t /* handle */)
{
    // nothing to initialize
}

void GrallocBufferBase::initStride()
{
    int yStride, uvStride;

    // setup stride
    switch (mFormat) {
    case HAL_PIXEL_FORMAT_YV12:
    case HAL_PIXEL_FORMAT_I420:
        uint32_t yStride_align;
        yStride_align = DisplayQuery::getOverlayLumaStrideAlignment(mFormat);
        if (yStride_align > 0)
        {
            yStride = align_to(align_to(mWidth, 32), yStride_align);
        }
        else
        {
            yStride = align_to(align_to(mWidth, 32), 64);
        }
        uvStride = align_to(yStride >> 1, 64);
        mStride.yuv.yStride = yStride;
        mStride.yuv.uvStride = uvStride;
        break;
    case HAL_PIXEL_FORMAT_NV12:
        yStride = align_to(align_to(mWidth, 32), 64);
        uvStride = yStride;
        mStride.yuv.yStride = yStride;
        mStride.yuv.uvStride = uvStride;
        break;
    case OMX_INTEL_COLOR_FormatYUV420PackedSemiPlanar:
    case OMX_INTEL_COLOR_FormatYUV420PackedSemiPlanar_Tiled:
        yStride = align_to(align_to(mWidth, 32), 64);
        uvStride = yStride;
        mStride.yuv.yStride = yStride;
        mStride.yuv.uvStride = uvStride;
        break;
    case HAL_PIXEL_FORMAT_YUY2:
    case HAL_PIXEL_FORMAT_UYVY:
        yStride = align_to((align_to(mWidth, 32) << 1), 64);
        uvStride = 0;
        mStride.yuv.yStride = yStride;
        mStride.yuv.uvStride = uvStride;
        break;
    default:
        mStride.rgb.stride = align_to(((mBpp >> 3) * align_to(mWidth, 32)), 64);
        break;
    }
}

}
}
