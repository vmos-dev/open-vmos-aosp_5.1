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
#include <common/base/Drm.h>
#include <Hwcomposer.h>
#include <ips/common/GrallocBufferMapperBase.h>

namespace android {
namespace intel {

GrallocBufferMapperBase::GrallocBufferMapperBase(DataBuffer& buffer)
    : BufferMapper(buffer)
{
    CTRACE();

    for (int i = 0; i < SUB_BUFFER_MAX; i++) {
        mGttOffsetInPage[i] = 0;
        mCpuAddress[i] = 0;
        mSize[i] = 0;
        mKHandle[i] = 0;
    }
}

GrallocBufferMapperBase::~GrallocBufferMapperBase()
{
    CTRACE();
}

uint32_t GrallocBufferMapperBase::getGttOffsetInPage(int subIndex) const
{
    if (subIndex >= 0 && subIndex < SUB_BUFFER_MAX)
        return mGttOffsetInPage[subIndex];
    return 0;
}

void* GrallocBufferMapperBase::getCpuAddress(int subIndex) const
{
    if (subIndex >=0 && subIndex < SUB_BUFFER_MAX)
        return mCpuAddress[subIndex];
    return 0;
}

uint32_t GrallocBufferMapperBase::getSize(int subIndex) const
{
    if (subIndex >= 0 && subIndex < SUB_BUFFER_MAX)
        return mSize[subIndex];
    return 0;
}

uint32_t GrallocBufferMapperBase::getKHandle(int subIndex)
{
    if (subIndex >= 0 && subIndex < SUB_BUFFER_MAX)
        return mKHandle[subIndex];
    return 0;
}


} // namespace intel
} // namespace android
