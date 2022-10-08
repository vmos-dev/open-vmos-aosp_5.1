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
#ifndef BUFFERMAPPER_H__
#define BUFFERMAPPER_H__

#include <DataBuffer.h>

namespace android {
namespace intel {

class BufferMapper : public DataBuffer {
public:
    BufferMapper(DataBuffer& buffer)
        : DataBuffer(buffer),
          mRefCount(0)
    {
    }
    virtual ~BufferMapper() {}
public:
    int incRef()
    {
        mRefCount++;
        return mRefCount;
    }
    int decRef()
    {
        mRefCount--;
        return mRefCount;
    }

    int getRef() const
    {
        return mRefCount;
    }

    // map the given buffer into both DC & CPU MMU
    virtual bool map() = 0;
    // unmap the give buffer from both DC & CPU MMU
    virtual bool unmap() = 0;

    // return gtt page offset
    virtual uint32_t getGttOffsetInPage(int subIndex) const = 0;
    virtual void* getCpuAddress(int subIndex) const = 0;
    virtual uint32_t getSize(int subIndex) const = 0;
    virtual uint32_t getKHandle(int subIndex) = 0;
    virtual uint32_t getFbHandle(int subIndex) = 0;
    virtual void putFbHandle() = 0;
private:
    int mRefCount;
};

} // namespace intel
} // namespace android

#endif /* BUFFERMAPPER_H__ */
