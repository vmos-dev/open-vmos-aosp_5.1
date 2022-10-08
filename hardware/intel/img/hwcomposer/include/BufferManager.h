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
#ifndef BUFFERMANAGER_H_
#define BUFFERMANAGER_H_

#include <common/utils/Dump.h>
#include <DataBuffer.h>
#include <BufferMapper.h>
#include <common/buffers/BufferCache.h>
#include <utils/Mutex.h>

namespace android {
namespace intel {

// Gralloc Buffer Manager
class BufferManager {
public:
    BufferManager();
    virtual ~BufferManager();

    bool initCheck() const;
    virtual bool initialize();
    virtual void deinitialize();

    // dump interface
    void dump(Dump& d);

    // lockDataBuffer and unlockDataBuffer must be used in serial
    // nested calling of them will cause a deadlock
    DataBuffer* lockDataBuffer(uint32_t handle);
    void unlockDataBuffer(DataBuffer *buffer);

    // get and put interfaces are deprecated
    // use lockDataBuffer and unlockDataBuffer instead
    DataBuffer* get(uint32_t handle);
    void put(DataBuffer *buffer);

    // map/unmap a data buffer into/from display memory
    BufferMapper* map(DataBuffer& buffer);
    void unmap(BufferMapper *mapper);

    // frame buffer management
    //return 0 if allocation fails
    virtual uint32_t allocFrameBuffer(int width, int height, int *stride);
    virtual void freeFrameBuffer(uint32_t kHandle);

    uint32_t allocGrallocBuffer(uint32_t width, uint32_t height, uint32_t format, uint32_t usage);
    void freeGrallocBuffer(uint32_t handle);
    virtual bool blitGrallocBuffer(uint32_t srcHandle, uint32_t dstHandle,
                                  crop_t& srcCrop, uint32_t async) = 0;
protected:
    virtual DataBuffer* createDataBuffer(gralloc_module_t *module,
                                             uint32_t handle) = 0;
    virtual BufferMapper* createBufferMapper(gralloc_module_t *module,
                                                 DataBuffer& buffer) = 0;

    gralloc_module_t *mGrallocModule;
private:
    enum {
        // make the buffer pool large enough
        DEFAULT_BUFFER_POOL_SIZE = 128,
    };

    alloc_device_t *mAllocDev;
    KeyedVector<uint32_t, BufferMapper*> mFrameBuffers;
    BufferCache *mBufferPool;
    DataBuffer *mDataBuffer;
    Mutex mDataBufferLock;
    Mutex mLock;
    bool mInitialized;
};

} // namespace intel
} // namespace android

#endif /* BUFFERMANAGER_H_ */
