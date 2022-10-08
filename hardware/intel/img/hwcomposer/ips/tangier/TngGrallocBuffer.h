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
#ifndef TNG_GRALLOC_BUFFER_H
#define TNG_GRALLOC_BUFFER_H

#include <DataBuffer.h>
#include <hal_public.h>
#include <ips/common/GrallocSubBuffer.h>
#include <ips/common/GrallocBufferBase.h>

namespace android {
namespace intel {

typedef IMG_native_handle_t TngIMGGrallocBuffer;

class TngGrallocBuffer : public GrallocBufferBase {
public:
    TngGrallocBuffer(uint32_t handle);
    virtual ~TngGrallocBuffer();

    void resetBuffer(uint32_t handle);

private:
    void initBuffer(uint32_t handle);
};

} // namespace intel
} // namespace android


#endif /* TNG_GRALLOC_BUFFER_H */
