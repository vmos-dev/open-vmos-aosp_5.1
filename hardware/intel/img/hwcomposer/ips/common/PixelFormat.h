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
#ifndef PIXEL_FORMAT_H
#define PIXEL_FORMAT_H

namespace android {
namespace intel {

class PixelFormat
{
public:
    enum {
        PLANE_PIXEL_FORMAT_BGRX565  = 0x14000000UL,
        PLANE_PIXEL_FORMAT_BGRX8888 = 0x18000000UL,
        PLANE_PIXEL_FORMAT_BGRA8888 = 0x1c000000UL,
        PLANE_PIXEL_FORMAT_RGBX8888 = 0x38000000UL,
        PLANE_PIXEL_FORMAT_RGBA8888 = 0x3c000000UL,
    };

    // convert gralloc color format to IP specific sprite pixel format.
    // See DSPACNTR (Display A Primary Sprite Control Register for more information)
    static bool convertFormat(uint32_t grallocFormat, uint32_t& spriteFormat, int& bpp);
};

} // namespace intel
} // namespace android

#endif /*PIXEL_FORMAT_H*/
