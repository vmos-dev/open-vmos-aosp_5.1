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
#ifndef DISPLAYPLANE_H_
#define DISPLAYPLANE_H_

#include <utils/KeyedVector.h>
#include <BufferMapper.h>
#include <common/base/Drm.h>

namespace android {
namespace intel {

typedef struct {
    // align with android, using 'int' here
    int x;
    int y;
    int w;
    int h;
} PlanePosition;

enum {
    // support up to 4 overlays
    MAX_OVERLAY_COUNT = 4,
    MAX_SPRITE_COUNT = 4,
};

enum {
     // in version 1.3, HWC_FRAMEBUFFER_TARGET is defined as 3
     HWC_FORCE_FRAMEBUFFER = 255,
};

class ZOrderConfig;

class DisplayPlane {
public:
    // plane type
    enum {
        PLANE_SPRITE = 0,
        PLANE_OVERLAY,
        PLANE_PRIMARY,
        PLANE_CURSOR,
        PLANE_MAX,
    };

    enum {
        // one more than android's back buffer count to allow more space
        // to do map/unmap, as plane reallocation may unmap on-screen layer.
        // each plane will cache the latest MIN_DATA_BUFFER_COUNT buffers
        // in case that these buffers are still in-using by display device
        // other buffers will be released on cache invalidation
        MIN_DATA_BUFFER_COUNT = 4,
    };

protected:
    enum {
        PLANE_POSITION_CHANGED    = 0x00000001UL,
        PLANE_BUFFER_CHANGED      = 0x00000002UL,
        PLANE_SOURCE_CROP_CHANGED = 0x00000004UL,
        PLANE_TRANSFORM_CHANGED   = 0x00000008UL,
    };
public:
    DisplayPlane(int index, int type, int disp);
    virtual ~DisplayPlane();
public:
    virtual int getIndex() const { return mIndex; }
    virtual int getType() const { return mType; }
    virtual bool initCheck() const { return mInitialized; }

    // data destination
    virtual void setPosition(int x, int y, int w, int h);
    virtual void setSourceCrop(int x, int y, int w, int h);
    virtual void setTransform(int transform);
    virtual void setPlaneAlpha(uint8_t alpha, uint32_t blending);

    // data source
    virtual bool setDataBuffer(uint32_t handle);

    virtual void invalidateBufferCache();

    // display device
    virtual bool assignToDevice(int disp);

    // hardware operations
    virtual bool flip(void *ctx);
    virtual void postFlip();

    virtual bool reset();
    virtual bool enable() = 0;
    virtual bool disable() = 0;
    virtual bool isDisabled() = 0;

    // set z order config
    virtual void setZOrderConfig(ZOrderConfig& config,
                                     void *nativeConfig) = 0;

    virtual void setZOrder(int zorder);
    virtual int getZOrder() const;

    virtual void* getContext() const = 0;

    virtual bool initialize(uint32_t bufferCount);
    virtual void deinitialize();

protected:
    virtual void checkPosition(int& x, int& y, int& w, int& h);
    virtual bool setDataBuffer(BufferMapper& mapper) = 0;
private:
    inline BufferMapper* mapBuffer(DataBuffer *buffer);

    inline bool isActiveBuffer(BufferMapper *mapper);
    void updateActiveBuffers(BufferMapper *mapper);
    void invalidateActiveBuffers();
protected:
    int mIndex;
    int mType;
    int mZOrder;
    int mDevice;
    bool mInitialized;

    // cached data buffers
    KeyedVector<uint64_t, BufferMapper*> mDataBuffers;
    // holding the most recent buffers
    Vector<BufferMapper*> mActiveBuffers;
    int mCacheCapacity;

    PlanePosition mPosition;
    crop_t mSrcCrop;
    bool mIsProtectedBuffer;
    int mTransform;
    uint8_t mPlaneAlpha;
    uint32_t mBlending;
    uint32_t mCurrentDataBuffer;
    uint32_t mUpdateMasks;
    drmModeModeInfo mModeInfo;
    int mPanelOrientation;

protected:
    bool mForceScaling;
    uint32_t mDisplayWidth;
    uint32_t mDisplayHeight;
    crop_t mDisplayCrop;
    uint32_t mScalingSource;
    uint32_t mScalingTarget;

};

} // namespace intel
} // namespace android

#endif /* DISPLAYPLANE_H_ */
