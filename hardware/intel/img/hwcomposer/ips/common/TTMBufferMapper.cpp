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
#include <ips/common/TTMBufferMapper.h>

namespace android {
namespace intel {

TTMBufferMapper::TTMBufferMapper(Wsbm& wsbm, DataBuffer& buffer)
    : BufferMapper(buffer),
      mRefCount(0),
      mWsbm(wsbm),
      mBufferObject(0),
      mGttOffsetInPage(0),
      mCpuAddress(0),
      mSize(0)
{
    CTRACE();
}

TTMBufferMapper::~TTMBufferMapper()
{
    CTRACE();
}

bool TTMBufferMapper::map()
{
    void *wsbmBufferObject = 0;
    uint32_t handle;
    void *virtAddr;
    uint32_t gttOffsetInPage;

    CTRACE();

    handle = getHandle();

    bool ret = mWsbm.wrapTTMBuffer(handle, &wsbmBufferObject);
    if (ret == false) {
        ELOGTRACE("failed to map TTM buffer");
        return false;
    }

    // TODO: review this later
    ret = mWsbm.waitIdleTTMBuffer(wsbmBufferObject);
    if (ret == false) {
        ELOGTRACE("failed to wait ttm buffer idle");
        return false;
    }

    virtAddr = mWsbm.getCPUAddress(wsbmBufferObject);
    gttOffsetInPage = mWsbm.getGttOffset(wsbmBufferObject);

    if (!gttOffsetInPage || !virtAddr) {
        WLOGTRACE("offset = %#x, addr = %p.", gttOffsetInPage, virtAddr);
        return false;
    }

    // update parameters
    mBufferObject = wsbmBufferObject;
    mGttOffsetInPage = gttOffsetInPage;
    mCpuAddress = virtAddr;
    mSize = 0;
    return true;
}

bool TTMBufferMapper::unmap()
{
    CTRACE();

    if (!mBufferObject)
        return false;

    mWsbm.unreferenceTTMBuffer(mBufferObject);

    mGttOffsetInPage = 0;
    mCpuAddress = 0;
    mSize = 0;
    mBufferObject = 0;
    return true;
}

bool TTMBufferMapper::waitIdle()
{
    return mWsbm.waitIdleTTMBuffer(mBufferObject);
}

} // namespace intel
} // namespace android


