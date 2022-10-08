/*
 * Copyright (C) 2012 Intel Corporation.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef __ISV_BUFMANAGER_H
#define __ISV_BUFMANAGER_H

#include <utils/RefBase.h>
#include <utils/Mutex.h>
#include <utils/Errors.h>
#include <utils/Vector.h>
#include "isv_worker.h"
#ifndef TARGET_VPP_USE_GEN
#include "hal_public.h"
#endif

using namespace android;

#define ISV_BUFFER_MANAGER_DEBUG 0

class ISVWorker;

class ISVBuffer
{
public:
    typedef enum {
        ISV_BUFFER_GRALLOC,
        ISV_BUFFER_METADATA,
    } ISV_BUFFERTYPE;

    typedef enum {
        ISV_BUFFER_NEED_CLEAR       = 0x00000001,
        ISV_BUFFER_CROP_CHANGED     = 0x00000002,
    } ISV_BUFFERFLAG;
private:
    //FIX ME: copy from ufo gralloc.h
    typedef struct _ufo_buffer_details_t
    {
        int width;       // \see alloc_device_t::alloc
        int height;      // \see alloc_device_t::alloc
        int format;      // \see alloc_device_t::alloc
        int usage;       // \see alloc_device_t::alloc
        int name;        // flink
        uint32_t fb;     // framebuffer id
        int drmformat;   // drm format
        int pitch;       // buffer pitch (in bytes)
        int size;        // buffer size (in bytes)
        int allocWidth;  // allocated buffer width in pixels.
        int allocHeight; // allocated buffer height in lines.
        int allocOffsetX;// horizontal pixel offset to content origin within allocated buffer.
        int allocOffsetY;// vertical line offset to content origin within allocated buffer.
    } ufo_buffer_details_t;

    enum
    {
        INTEL_UFO_GRALLOC_MODULE_PERFORM_GET_BO_INFO = 6 // (buffer_handle_t, buffer_info_t*)
    };

public:
    ISVBuffer(sp<ISVWorker> worker,
            unsigned long buffer, unsigned long grallocHandle,
            uint32_t width, uint32_t height,
            uint32_t stride, uint32_t colorFormat,
            ISV_BUFFERTYPE type, uint32_t flag)
        :mWorker(worker),
        mBuffer(buffer),
        mGrallocHandle(grallocHandle),
        mWidth(width),
        mHeight(height),
        mSurfaceHeight(0),
        mStride(stride),
        mColorFormat(colorFormat),
        mType(type),
        mSurface(-1),
        mFlags(flag),
        mpGralloc(NULL) {}

    ISVBuffer(sp<ISVWorker> worker,
            unsigned long buffer,
            ISV_BUFFERTYPE type,
            uint32_t flag)
        :mWorker(worker),
        mBuffer(buffer),
        mGrallocHandle(0),
        mWidth(0),
        mHeight(0),
        mSurfaceHeight(0),
        mStride(0),
        mColorFormat(0),
        mType(type),
        mSurface(-1),
        mFlags(flag),
        mpGralloc(NULL) {}

    ~ISVBuffer();

    // init buffer info
    // FIXME: hackFormat is for VP9, should be removed in future
    status_t initBufferInfo(uint32_t hackFormat);

    // get va surface
    int32_t getSurface() { return mSurface; }
    // get buffer handle
    unsigned long getHandle() { return mBuffer; }
    // set/clear/get flag
    uint32_t getFlags() { return mFlags; }
    void setFlag(uint32_t flag) { mFlags |= flag; return; }
    void unsetFlag(uint32_t flag) { mFlags &= ~flag; return; }
    status_t clearIfNeed();

private:

    sp<ISVWorker> mWorker;
    unsigned long mBuffer;
    unsigned long mGrallocHandle;
    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mSurfaceHeight;
    uint32_t mStride;
    uint32_t mColorFormat;
    ISV_BUFFERTYPE mType;
    int32_t mSurface;
    uint32_t mFlags;
    gralloc_module_t* mpGralloc;
};

class ISVBufferManager: public RefBase
{
public:
    ISVBufferManager()
        :mWorker(NULL),
        mMetaDataMode(false),
        mNeedClearBuffers(false) {}

    ~ISVBufferManager() {}
    // set mBuffers size
    status_t setBufferCount(int32_t size);

    // register/unregister ISVBuffers to mBuffers
    status_t useBuffer(const sp<ANativeWindowBuffer> nativeBuffer);
    status_t useBuffer(unsigned long handle);
    status_t freeBuffer(unsigned long handle);

    // Map to ISVBuffer
    ISVBuffer* mapBuffer(unsigned long handle);
    // set isv worker
    void setWorker(sp<ISVWorker> worker) { mWorker = worker; }
    void setMetaDataMode(bool metaDataMode) { mMetaDataMode = metaDataMode; }
    // set buffer flag.
    status_t setBuffersFlag(uint32_t flag);
private:
    typedef enum {
        GRALLOC_BUFFER_MODE = 0,
        META_DATA_MODE = 1,
    } ISV_WORK_MODE;

    sp<ISVWorker> mWorker;
    bool mMetaDataMode;
    // VPP buffer queue
    Vector<ISVBuffer*> mBuffers;
    Mutex mBufferLock; // to protect access to mBuffers
    bool mNeedClearBuffers;
};


#endif //#define __ISV_BUFMANAGER_H
