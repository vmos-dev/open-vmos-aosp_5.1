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
#include <ips/common/SpritePlaneBase.h>
#include <ips/common/PixelFormat.h>

namespace android {
namespace intel {

SpritePlaneBase::SpritePlaneBase(int index, int disp)
    : DisplayPlane(index, PLANE_SPRITE, disp),
      mForceBottom(false),
      mAbovePrimary(true)
{
    CTRACE();
}

SpritePlaneBase::~SpritePlaneBase()
{
    CTRACE();
}

bool SpritePlaneBase::flip(void *ctx)
{
    CTRACE();
    return DisplayPlane::flip(ctx);
}

bool SpritePlaneBase::enable()
{
    return enablePlane(true);
}

bool SpritePlaneBase::disable()
{
    return enablePlane(false);
}

} // namespace intel
} // namespace android
