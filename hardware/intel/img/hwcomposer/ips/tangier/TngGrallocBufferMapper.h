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
#ifndef TNG_GRALLOC_BUFFER_MAPPER_H
#define TNG_GRALLOC_BUFFER_MAPPER_H

#include <BufferMapper.h>
#include <ips/common/GrallocBufferMapperBase.h>
#include <ips/tangier/TngGrallocBuffer.h>

namespace android {
namespace intel {

class TngGrallocBufferMapper : public GrallocBufferMapperBase {
public:
    TngGrallocBufferMapper(IMG_gralloc_module_public_t& module,
                               DataBuffer& buffer);
    virtual ~TngGrallocBufferMapper();
public:
    bool map();
    bool unmap();
    uint32_t getKHandle(int subIndex);
    uint32_t getFbHandle(int subIndex);
    void putFbHandle();
private:
    bool gttMap(void *vaddr, uint32_t size, uint32_t gttAlign, int *offset);
    bool gttUnmap(void *vaddr);
    bool mapKhandle();

private:
    IMG_gralloc_module_public_t& mIMGGrallocModule;
    void* mBufferObject;
    native_handle_t* mClonedHandle;
};

} // namespace intel
} // namespace android

#endif /* TNG_GRALLOC_BUFFER_MAPPER_H */
