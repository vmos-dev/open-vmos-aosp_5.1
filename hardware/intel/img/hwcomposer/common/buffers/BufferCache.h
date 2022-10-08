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
#ifndef BUFFERCACHE_H_
#define BUFFERCACHE_H_

#include <utils/KeyedVector.h>
#include <BufferMapper.h>

namespace android {
namespace intel {

// Generic buffer cache
class BufferCache {
public:
    BufferCache(int size);
    virtual ~BufferCache();
    // add a new mapper into buffer cache
    virtual bool addMapper(uint64_t handle, BufferMapper* mapper);
    //remove mapper
    virtual bool removeMapper(BufferMapper* mapper);
    // get a buffer mapper
    virtual BufferMapper* getMapper(uint64_t handle);
    // get cache size
    virtual size_t getCacheSize() const;
    // get mapper with an index
    virtual BufferMapper* getMapper(size_t index);
private:
    KeyedVector<uint64_t, BufferMapper*> mBufferPool;
};

}
}


#endif /* BUFFERCACHE_H_ */
