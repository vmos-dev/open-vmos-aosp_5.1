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
#include <system/graphics.h>
#include <common/utils/HwcTrace.h>
#include <ips/common/PixelFormat.h>
#include <hal_public.h>

namespace android {
namespace intel {

bool PixelFormat::convertFormat(uint32_t grallocFormat, uint32_t& spriteFormat, int& bpp)
{
    switch (grallocFormat) {
    case HAL_PIXEL_FORMAT_RGBA_8888:
        spriteFormat = PLANE_PIXEL_FORMAT_RGBA8888;
        bpp = 4;
        break;
    case HAL_PIXEL_FORMAT_RGBX_8888:
        spriteFormat = PLANE_PIXEL_FORMAT_RGBX8888;
        bpp = 4;
        break;
    case HAL_PIXEL_FORMAT_BGRX_8888:
        spriteFormat = PLANE_PIXEL_FORMAT_BGRX8888;
        bpp = 4;
        break;
    case HAL_PIXEL_FORMAT_BGRA_8888:
        spriteFormat = PLANE_PIXEL_FORMAT_BGRA8888;
        bpp = 4;
        break;
    case HAL_PIXEL_FORMAT_RGB_565:
        spriteFormat = PLANE_PIXEL_FORMAT_BGRX565;
        bpp = 2;
        break;
    default:
        return false;
    }

    return true;
}


} // namespace intel
} // namespace android
