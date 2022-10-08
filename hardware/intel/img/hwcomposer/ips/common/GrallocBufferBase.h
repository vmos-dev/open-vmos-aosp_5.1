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
#ifndef GRALLOC_BUFFER_BASE_H
#define GRALLOC_BUFFER_BASE_H

#include <GraphicBuffer.h>

namespace android {
namespace intel {

class GrallocBufferBase : public GraphicBuffer {
public:
    GrallocBufferBase(uint32_t handle);
    virtual ~GrallocBufferBase() {}
    virtual void resetBuffer(uint32_t handle);

protected:
    // helper function to be invoked by the derived class
    void initStride();

private:
    void initBuffer(uint32_t handle);
};

} // namespace intel
} // namespace android

#endif /* GRALLOC_BUFFER_BASE_H */
