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
#include <ips/tangier/TngGrallocBuffer.h>

namespace android {
namespace intel {

TngGrallocBuffer::TngGrallocBuffer(uint32_t handle)
    :GrallocBufferBase(handle)
{
    initBuffer(handle);
}

TngGrallocBuffer::~TngGrallocBuffer()
{
}

void TngGrallocBuffer::resetBuffer(uint32_t handle)
{
    GrallocBufferBase::resetBuffer(handle);
    initBuffer(handle);
}

void TngGrallocBuffer::initBuffer(uint32_t handle)
{
    TngIMGGrallocBuffer *grallocHandle = (TngIMGGrallocBuffer *)handle;

    CTRACE();

    if (!grallocHandle) {
        ELOGTRACE("gralloc handle is null");
        return;
    }

    mFormat = grallocHandle->iFormat;
    mWidth = grallocHandle->iWidth;
    mHeight = grallocHandle->iHeight;
    mUsage = grallocHandle->usage;
    mKey = grallocHandle->ui64Stamp;
    mBpp = grallocHandle->uiBpp;

    // stride can only be initialized after format is set
    initStride();
}


}
}
