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
#ifndef SPRITE_PLANE_BASE_H
#define SPRITE_PLANE_BASE_H

#include <utils/KeyedVector.h>
#include <common/buffers/BufferCache.h>
#include <DisplayPlane.h>

namespace android {
namespace intel {

class SpritePlaneBase : public DisplayPlane {
public:
    SpritePlaneBase(int index, int disp);
    virtual ~SpritePlaneBase();
public:
    // hardware operations
    virtual bool flip(void *ctx);
    virtual bool enable();
    virtual bool disable();
    virtual bool isDisabled() = 0;

    // display device
    virtual void* getContext() const = 0;
protected:
    virtual bool setDataBuffer(BufferMapper& mapper) = 0;
    virtual bool enablePlane(bool enabled) = 0;
protected:
    bool mForceBottom;
    bool mAbovePrimary;
};

} // namespace intel
} // namespace android

#endif /* SPRITE_PLANE_BASE_H */

