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
#include <hardware/hwcomposer.h>
#include <BufferManager.h>
#include <DrmConfig.h>

namespace android {
namespace intel {

BufferManager::BufferManager()
    : mGrallocModule(NULL),
      mAllocDev(NULL),
      mFrameBuffers(),
      mBufferPool(NULL),
      mDataBuffer(NULL),
      mDataBufferLock(),
      mInitialized(false)
{
    CTRACE();
}

BufferManager::~BufferManager()
{
    WARN_IF_NOT_DEINIT();
}

bool BufferManager::initCheck() const
{
    return mInitialized;
}

bool BufferManager::initialize()
{
    CTRACE();

    // create buffer pool
    mBufferPool = new BufferCache(DEFAULT_BUFFER_POOL_SIZE);
    if (!mBufferPool) {
        ELOGTRACE("failed to create gralloc buffer cache");
        return false;
    }

    // init gralloc module
    hw_module_t const* module;
    if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module)) {
        DEINIT_AND_RETURN_FALSE("failed to get gralloc module");
    }
    mGrallocModule = (gralloc_module_t*)module;

    gralloc_open(module, &mAllocDev);
    if (!mAllocDev) {
        WLOGTRACE("failed to open alloc device");
    }

    // create a dummy data buffer
    mDataBuffer = createDataBuffer(mGrallocModule, 0);
    if (!mDataBuffer) {
        DEINIT_AND_RETURN_FALSE("failed to create data buffer");
    }

    mInitialized = true;
    return true;
}

void BufferManager::deinitialize()
{
    mInitialized = false;

    if (mBufferPool) {
        // unmap & delete all cached buffer mappers
        for (size_t i = 0; i < mBufferPool->getCacheSize(); i++) {
            BufferMapper *mapper = mBufferPool->getMapper(i);
            mapper->unmap();
            delete mapper;
        }

        delete mBufferPool;
        mBufferPool = NULL;
    }

    for (size_t j = 0; j < mFrameBuffers.size(); j++) {
        BufferMapper *mapper = mFrameBuffers.valueAt(j);
        mapper->unmap();
        delete mapper;
    }
    mFrameBuffers.clear();

    if (mAllocDev) {
        gralloc_close(mAllocDev);
        mAllocDev = NULL;
    }

    if (mDataBuffer) {
        delete mDataBuffer;
        mDataBuffer = NULL;
    }
}

void BufferManager::dump(Dump& d)
{
    d.append("Buffer Manager status: pool size %d\n", mBufferPool->getCacheSize());
    d.append("-------------------------------------------------------------\n");
    for (size_t i = 0; i < mBufferPool->getCacheSize(); i++) {
        BufferMapper *mapper = mBufferPool->getMapper(i);
        d.append("Buffer %d: handle %#x, (%dx%d), format %d, refCount %d\n",
                 i,
                 mapper->getHandle(),
                 mapper->getWidth(),
                 mapper->getHeight(),
                 mapper->getFormat(),
                 mapper->getRef());
    }
    return;
}

DataBuffer* BufferManager::lockDataBuffer(uint32_t handle)
{
    mDataBufferLock.lock();
    mDataBuffer->resetBuffer(handle);
    return mDataBuffer;
}

void BufferManager::unlockDataBuffer(DataBuffer * /* buffer */)
{
    mDataBufferLock.unlock();
}

DataBuffer* BufferManager::get(uint32_t handle)
{
    return createDataBuffer(mGrallocModule, handle);
}

void BufferManager::put(DataBuffer *buffer)
{
    delete buffer;
}

BufferMapper* BufferManager::map(DataBuffer& buffer)
{
    bool ret;
    BufferMapper* mapper;

    CTRACE();
    Mutex::Autolock _l(mLock);
    //try to get mapper from pool
    mapper = mBufferPool->getMapper(buffer.getKey());
    if (mapper) {
        // increase mapper ref count
        mapper->incRef();
        return mapper;
    }

    // create a new buffer mapper and add it to pool
    do {
        VLOGTRACE("new buffer, will add it");
        mapper = createBufferMapper(mGrallocModule, buffer);
        if (!mapper) {
            ELOGTRACE("failed to allocate mapper");
            break;
        }
        ret = mapper->map();
        if (!ret) {
            ELOGTRACE("failed to map");
            delete mapper;
            mapper = NULL;
            break;
        }
        ret = mBufferPool->addMapper(buffer.getKey(), mapper);
        if (!ret) {
            ELOGTRACE("failed to add mapper");
            break;
        }
        // increase mapper ref count
        mapper->incRef();
        return mapper;
    } while (0);

    // error handling
    if (mapper) {
        mapper->unmap();
        delete mapper;
    }
    return NULL;
}

void BufferManager::unmap(BufferMapper *mapper)
{
    Mutex::Autolock _l(mLock);
    if (!mapper) {
        ELOGTRACE("invalid mapper");
        return;
    }

    // unmap & remove this mapper from buffer when refCount = 0
    int refCount = mapper->decRef();
    if (refCount < 0) {
        ELOGTRACE("invalid ref count");
    } else if (!refCount) {
        // remove mapper from buffer pool
        mBufferPool->removeMapper(mapper);
        mapper->unmap();
        delete mapper;
    }
}

uint32_t BufferManager::allocFrameBuffer(int width, int height, int *stride)
{
    RETURN_NULL_IF_NOT_INIT();

    if (!mAllocDev) {
        WLOGTRACE("Alloc device is not available");
        return 0;
    }

    if (!width || !height || !stride) {
        ELOGTRACE("invalid input parameter");
        return 0;
    }

    ILOGTRACE("size of frame buffer to create: %dx%d", width, height);
    uint32_t handle = 0;
    status_t err  = mAllocDev->alloc(
            mAllocDev,
            width,
            height,
            DrmConfig::getFrameBufferFormat(),
            0, // GRALLOC_USAGE_HW_FB
            (buffer_handle_t *)&handle,
            stride);

    if (err != 0) {
        ELOGTRACE("failed to allocate frame buffer, error = %d", err);
        return 0;
    }

    DataBuffer *buffer = NULL;
    BufferMapper *mapper = NULL;

    do {
        buffer = lockDataBuffer(handle);
        if (!buffer) {
            ELOGTRACE("failed to get data buffer, handle = %#x", handle);
            break;
        }

        mapper = createBufferMapper(mGrallocModule, *buffer);
        if (!mapper) {
            ELOGTRACE("failed to create buffer mapper");
            break;
        }

        uint32_t fbHandle;
         if (!(fbHandle = mapper->getFbHandle(0))) {
             ELOGTRACE("failed to get Fb handle");
             break;
         }

        mFrameBuffers.add(fbHandle, mapper);
        unlockDataBuffer(buffer);
        return fbHandle;
    } while (0);

    // error handling, release all allocated resources
    if (buffer) {
        unlockDataBuffer(buffer);
    }
    if (mapper) {
        delete mapper;
    }
    mAllocDev->free(mAllocDev, (buffer_handle_t)handle);
    return 0;
}

void BufferManager::freeFrameBuffer(uint32_t fbHandle)
{
    RETURN_VOID_IF_NOT_INIT();

    if (!mAllocDev) {
        WLOGTRACE("Alloc device is not available");
        return;
    }

    ssize_t index = mFrameBuffers.indexOfKey(fbHandle);
    if (index < 0) {
        ELOGTRACE("invalid kernel handle");
        return;
    }

    BufferMapper *mapper = mFrameBuffers.valueAt(index);
    uint32_t handle = mapper->getHandle();
    mapper->putFbHandle();
    delete mapper;
    mFrameBuffers.removeItem(fbHandle);
    mAllocDev->free(mAllocDev, (buffer_handle_t)handle);
}

uint32_t BufferManager::allocGrallocBuffer(uint32_t width, uint32_t height, uint32_t format, uint32_t usage)
{
    RETURN_NULL_IF_NOT_INIT();

    if (!mAllocDev) {
        WLOGTRACE("Alloc device is not available");
        return 0;
    }

    if (!width || !height) {
        ELOGTRACE("invalid input parameter");
        return 0;
    }

    ILOGTRACE("size of graphic buffer to create: %dx%d", width, height);
    uint32_t handle = 0;
    int stride;
    status_t err  = mAllocDev->alloc(
                mAllocDev,
                width,
                height,
                format,
                usage,
                (buffer_handle_t *)&handle,
                &stride);
    if (err != 0) {
        ELOGTRACE("failed to allocate gralloc buffer, error = %d", err);
        return 0;
    }

    return handle;
}

void BufferManager::freeGrallocBuffer(uint32_t handle)
{
    RETURN_VOID_IF_NOT_INIT();
    if (!mAllocDev) {
        WLOGTRACE("Alloc device is not available");
        return;
    }

    if (handle)
        mAllocDev->free(mAllocDev, (buffer_handle_t)handle);
}

} // namespace intel
} // namespace android
