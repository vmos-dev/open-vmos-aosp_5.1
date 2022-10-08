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
#include <GraphicBuffer.h>

namespace android {
namespace intel {

GraphicBuffer::GraphicBuffer(uint32_t handle)
    : DataBuffer(handle)
{
    initBuffer(handle);
}

void GraphicBuffer::resetBuffer(uint32_t handle)
{
    DataBuffer::resetBuffer(handle);
    initBuffer(handle);
}

bool GraphicBuffer::isProtectedUsage(uint32_t usage)
{
    if (usage == USAGE_INVALID) {
        return false;
    }

    return (usage & GRALLOC_USAGE_PROTECTED) != 0;
}

bool GraphicBuffer::isProtectedBuffer(GraphicBuffer *buffer)
{
    if (buffer == NULL) {
        return false;
    }

    return isProtectedUsage(buffer->mUsage);
}

bool GraphicBuffer::isCompressionUsage(uint32_t usage)
{
    if (usage == USAGE_INVALID) {
        return false;
    }

    return false;
}

bool GraphicBuffer::isCompressionBuffer(GraphicBuffer *buffer)
{
    if (buffer == NULL) {
        return false;
    }

    return isCompressionUsage(buffer->mUsage);
}

void GraphicBuffer::initBuffer(uint32_t /*handle*/)
{
    mUsage = USAGE_INVALID;
    mBpp = 0;
}

}
}
