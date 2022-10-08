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
#ifndef DATABUFFER_H__
#define DATABUFFER_H__

#include <hardware/hwcomposer.h>

namespace android {
namespace intel {

typedef struct crop {
    // align with android, using 'int' here
    int x;
    int y;
    int w;
    int h;
} crop_t;

typedef struct stride {
    union {
        struct {
            uint32_t stride;
        } rgb;
        struct {
            uint32_t yStride;
            uint32_t uvStride;
        } yuv;
    };
}stride_t;

class DataBuffer {
public:
    enum {
        FORMAT_INVALID = 0xffffffff,
    };
public:
    DataBuffer(uint32_t handle)
    {
        initBuffer(handle);
    }
    virtual ~DataBuffer() {}

public:
    virtual void resetBuffer(uint32_t handle) {
        // nothing to reset, just do initialization
        initBuffer(handle);
    }

    uint32_t getHandle() const { return mHandle; }

    void setStride(stride_t& stride) { mStride = stride; }
    stride_t& getStride() { return mStride; }

    void setWidth(uint32_t width) { mWidth = width; }
    uint32_t getWidth() const { return mWidth; }

    void setHeight(uint32_t height) { mHeight = height; }
    uint32_t getHeight() const { return mHeight; }

    void setCrop(int x, int y, int w, int h) {
        mCrop.x = x; mCrop.y = y; mCrop.w = w; mCrop.h = h; }
    crop_t& getCrop() { return mCrop; }

    void setFormat(uint32_t format) { mFormat = format; }
    uint32_t getFormat() const { return mFormat; }

    uint64_t getKey() const { return mKey; }

    void setIsCompression(bool isCompressed) { mIsCompression = isCompressed; }
    bool isCompression() { return mIsCompression; }

private:
    void initBuffer(uint32_t handle) {
        mHandle = handle;
        mFormat = 0;
        mWidth = 0;
        mHeight = 0;
        mKey = handle;
        memset(&mStride, 0, sizeof(stride_t));
        memset(&mCrop, 0, sizeof(crop_t));
    }
protected:
    uint32_t mHandle;
    stride_t mStride;
    crop_t mCrop;
    uint32_t mFormat;
    uint32_t mWidth;
    uint32_t mHeight;
    uint64_t mKey;
    bool mIsCompression;
};

static inline uint32_t align_to(uint32_t arg, uint32_t align)
{
    return ((arg + (align - 1)) & (~(align - 1)));
}

} // namespace intel
} // namespace android

#endif /* DATABUFFER_H__ */
