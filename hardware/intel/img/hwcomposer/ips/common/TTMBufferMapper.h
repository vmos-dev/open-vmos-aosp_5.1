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
#ifndef TTMBUFFERMAPPER_H_
#define TTMBUFFERMAPPER_H_

#include <DataBuffer.h>
#include <BufferMapper.h>
#include <ips/common/Wsbm.h>

namespace android {
namespace intel {

class TTMBufferMapper : public BufferMapper {
public:
    TTMBufferMapper(Wsbm& wsbm, DataBuffer& buffer);
    virtual ~TTMBufferMapper();
public:
    bool map();
    bool unmap();

    uint32_t getGttOffsetInPage(int /* subIndex */) const {
        return mGttOffsetInPage;
    }
    void* getCpuAddress(int /* subIndex */) const {
        return mCpuAddress;
    }
    uint32_t getSize(int /* subIndex */) const {
        return mSize;
    }
    uint32_t getKHandle(int /* subIndex */) {
        return 0;
    }
    uint32_t getFbHandle(int /* subIndex */) {
        return 0;
    }
    void putFbHandle() {
        return;
    }

    // wait idle
    bool waitIdle();
private:
    int mRefCount;
    Wsbm& mWsbm;
    void* mBufferObject;

    // mapped info
    uint32_t mGttOffsetInPage;
    void* mCpuAddress;
    uint32_t mSize;
};

} //namespace intel
} //namespace android


#endif /* TTMBUFFERMAPPER_H_ */
