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
#ifndef GRALLOC_BUFFER_MAPPER_BASE_H
#define GRALLOC_BUFFER_MAPPER_BASE_H

#include <BufferMapper.h>
#include <ips/common/GrallocSubBuffer.h>
#include <ips/common/GrallocBufferBase.h>

namespace android {
namespace intel {

class GrallocBufferMapperBase : public BufferMapper {
public:
    GrallocBufferMapperBase(DataBuffer& buffer);
    virtual ~GrallocBufferMapperBase();
public:
    virtual bool map() = 0;
    virtual bool unmap() = 0;

    uint32_t getGttOffsetInPage(int subIndex) const;
    void* getCpuAddress(int subIndex) const;
    uint32_t getSize(int subIndex) const;
    virtual uint32_t getKHandle(int subIndex);
    virtual uint32_t getFbHandle(int subIndex) = 0;
    virtual void putFbHandle() = 0;

protected:
    // mapped info
    uint32_t mGttOffsetInPage[SUB_BUFFER_MAX];
    void* mCpuAddress[SUB_BUFFER_MAX];
    uint32_t mSize[SUB_BUFFER_MAX];
    uint32_t mKHandle[SUB_BUFFER_MAX];
};

} // namespace intel
} // namespace android

#endif /* TNG_GRALLOC_BUFFER_MAPPER_H */
