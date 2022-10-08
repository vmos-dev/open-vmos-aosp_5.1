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
#include <Hwcomposer.h>
#include <DisplayPlane.h>
#include <GraphicBuffer.h>
#include <DisplayQuery.h>

namespace android {
namespace intel {

DisplayPlane::DisplayPlane(int index, int type, int disp)
    : mIndex(index),
      mType(type),
      mZOrder(-1),
      mDevice(disp),
      mInitialized(false),
      mDataBuffers(),
      mActiveBuffers(),
      mCacheCapacity(0),
      mIsProtectedBuffer(false),
      mTransform(0),
      mPlaneAlpha(0),
      mBlending(HWC_BLENDING_NONE),
      mCurrentDataBuffer(0),
      mUpdateMasks(0),
      mForceScaling(false),
      mDisplayWidth(0),
      mDisplayHeight(0),
      mScalingSource(0),
      mScalingTarget(0)
{
    CTRACE();
    memset(&mPosition, 0, sizeof(mPosition));
    memset(&mSrcCrop, 0, sizeof(mSrcCrop));
}

DisplayPlane::~DisplayPlane()
{
    WARN_IF_NOT_DEINIT();
}

bool DisplayPlane::initialize(uint32_t bufferCount)
{
    CTRACE();

    if (bufferCount < MIN_DATA_BUFFER_COUNT) {
        WLOGTRACE("buffer count %d is too small", bufferCount);
        bufferCount = MIN_DATA_BUFFER_COUNT;
    }

    // create buffer cache, adding few extra slots as buffer rendering is async
    // buffer could still be queued in the display pipeline such that they
    // can't be unmapped]
    mCacheCapacity = bufferCount;
    mDataBuffers.setCapacity(bufferCount);
    mActiveBuffers.setCapacity(MIN_DATA_BUFFER_COUNT);
    mInitialized = true;
    return true;
}

void DisplayPlane::deinitialize()
{
    // invalidate cached data buffers
    if (mDataBuffers.size()) {
        // invalidateBufferCache will assert if object is not initialized
        // so invoking it only there is buffer to invalidate.
        invalidateBufferCache();
    }

    // invalidate active buffers
    if (mActiveBuffers.size()) {
        invalidateActiveBuffers();
    }

    mCurrentDataBuffer = 0;
    mInitialized = false;
}

void DisplayPlane::checkPosition(int& x, int& y, int& w, int& h)
{
    drmModeModeInfoPtr mode = &mModeInfo;

    if (mode->hdisplay == 0 || mode->vdisplay == 0)
        return;

    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;
    if ((x + w) > mode->hdisplay)
        w = mode->hdisplay - x;
    if ((y + h) > mode->vdisplay)
        h = mode->vdisplay - y;
}

void DisplayPlane::setPosition(int x, int y, int w, int h)
{
    ALOGTRACE("Position = %d, %d - %dx%d", x, y, w, h);

    if (mForceScaling) {
        // set in assignToDevice
        mPosition.x = (int) (((float)x/DEFAULT_DRM_FB_WIDTH)*mDisplayWidth);
        mPosition.y = (int) (((float)y/DEFAULT_DRM_FB_HEIGHT)*mDisplayHeight);
        mPosition.w = (int) (((float)w/DEFAULT_DRM_FB_WIDTH)*mDisplayWidth);
        mPosition.h = (int) (((float)h/DEFAULT_DRM_FB_HEIGHT)*mDisplayHeight);

        mDisplayCrop.x = 0;
        mDisplayCrop.y = 0;
        mDisplayCrop.w = mDisplayWidth;
        mDisplayCrop.h = mDisplayHeight;

        return;
    }

    if (mPosition.x != x || mPosition.y != y ||
        mPosition.w != w || mPosition.h != h) {
        mUpdateMasks |= PLANE_POSITION_CHANGED;
        mPosition.x = x;
        mPosition.y = y;
        mPosition.w = w;
        mPosition.h = h;
    }
}

void DisplayPlane::setSourceCrop(int x, int y, int w, int h)
{
    ALOGTRACE("Source crop = %d, %d - %dx%d", x, y, w, h);

    if (mSrcCrop.x != x || mSrcCrop.y != y ||
        mSrcCrop.w != w || mSrcCrop.h != h) {
        mUpdateMasks |= PLANE_SOURCE_CROP_CHANGED;
        mSrcCrop.x = x;
        mSrcCrop.y = y;
        mSrcCrop.w = w;
        mSrcCrop.h = h;
    }
}

void DisplayPlane::setTransform(int trans)
{
    ALOGTRACE("transform = %d", trans);

    if (mTransform == trans) {
        return;
    }

    mTransform = trans;

    mUpdateMasks |= PLANE_TRANSFORM_CHANGED;
}

void DisplayPlane::setPlaneAlpha(uint8_t alpha, uint32_t blending)
{
    ALOGTRACE("plane alpha = 0x%x", alpha);

    if (mPlaneAlpha != alpha) {
        mPlaneAlpha = alpha;
        mUpdateMasks |= PLANE_BUFFER_CHANGED;
    }

    if (mBlending != blending) {
        mBlending = blending;
        mUpdateMasks |= PLANE_BUFFER_CHANGED;
    }
}

bool DisplayPlane::setDataBuffer(uint32_t handle)
{
    DataBuffer *buffer;
    BufferMapper *mapper;
    ssize_t index;
    bool ret;
    bool isCompression;
    BufferManager *bm = Hwcomposer::getInstance().getBufferManager();

    RETURN_FALSE_IF_NOT_INIT();
    ALOGTRACE("handle = %#x", handle);

    if (!handle) {
        WLOGTRACE("invalid buffer handle");
        return false;
    }

    // do not need to update the buffer handle
    if (mCurrentDataBuffer != handle)
        mUpdateMasks |= PLANE_BUFFER_CHANGED;

    // if no update then do Not need set data buffer
    if (!mUpdateMasks)
        return true;

    buffer = bm->lockDataBuffer(handle);
    if (!buffer) {
        ELOGTRACE("failed to get buffer");
        return false;
    }

    mIsProtectedBuffer = GraphicBuffer::isProtectedBuffer((GraphicBuffer*)buffer);
    isCompression = GraphicBuffer::isCompressionBuffer((GraphicBuffer*)buffer);

    // map buffer if it's not in cache
    index = mDataBuffers.indexOfKey(buffer->getKey());
    if (index < 0) {
        VLOGTRACE("unmapped buffer, mapping...");
        mapper = mapBuffer(buffer);
        if (!mapper) {
            ELOGTRACE("failed to map buffer %#x", handle);
            bm->unlockDataBuffer(buffer);
            return false;
        }
    } else {
        VLOGTRACE("got mapper in saved data buffers and update source Crop");
        mapper = mDataBuffers.valueAt(index);
    }

    // always update source crop to mapper
    mapper->setCrop(mSrcCrop.x, mSrcCrop.y, mSrcCrop.w, mSrcCrop.h);

    mapper->setIsCompression(isCompression);

    // unlock buffer after getting mapper
    bm->unlockDataBuffer(buffer);
    buffer = NULL;

    ret = setDataBuffer(*mapper);
    if (ret) {
        mCurrentDataBuffer = handle;
        // update active buffers
        updateActiveBuffers(mapper);
    }
    return ret;
}

BufferMapper* DisplayPlane::mapBuffer(DataBuffer *buffer)
{
    BufferManager *bm = Hwcomposer::getInstance().getBufferManager();

    // invalidate buffer cache  if cache is full
    if ((int)mDataBuffers.size() >= mCacheCapacity) {
        invalidateBufferCache();
    }

    BufferMapper *mapper = bm->map(*buffer);
    if (!mapper) {
        ELOGTRACE("failed to map buffer");
        return NULL;
    }

    // add it to data buffers
    ssize_t index = mDataBuffers.add(buffer->getKey(), mapper);
    if (index < 0) {
        ELOGTRACE("failed to add mapper");
        bm->unmap(mapper);
        return NULL;
    }

    return mapper;
}

bool DisplayPlane::isActiveBuffer(BufferMapper *mapper)
{
    for (size_t i = 0; i < mActiveBuffers.size(); i++) {
        BufferMapper *activeMapper = mActiveBuffers.itemAt(i);
        if (!activeMapper)
            continue;
        if (activeMapper->getKey() == mapper->getKey())
            return true;
    }

    return false;
}

void DisplayPlane::updateActiveBuffers(BufferMapper *mapper)
{
    BufferManager *bm = Hwcomposer::getInstance().getBufferManager();

    // unmap the first entry (oldest buffer)
    if (mActiveBuffers.size() >= MIN_DATA_BUFFER_COUNT) {
        BufferMapper *oldest = mActiveBuffers.itemAt(0);
        bm->unmap(oldest);
        mActiveBuffers.removeAt(0);
    }

    // queue it to active buffers
    if (!isActiveBuffer(mapper)) {
        mapper->incRef();
        mActiveBuffers.push_back(mapper);
    }
}

void DisplayPlane::invalidateActiveBuffers()
{
    BufferManager *bm = Hwcomposer::getInstance().getBufferManager();
    BufferMapper* mapper;

    RETURN_VOID_IF_NOT_INIT();

    VLOGTRACE("invalidating active buffers");

    for (size_t i = 0; i < mActiveBuffers.size(); i++) {
        mapper = mActiveBuffers.itemAt(i);
        // unmap it
        bm->unmap(mapper);
    }

    // clear recorded data buffers
    mActiveBuffers.clear();
}

void DisplayPlane::invalidateBufferCache()
{
    BufferManager *bm = Hwcomposer::getInstance().getBufferManager();
    BufferMapper* mapper;

    RETURN_VOID_IF_NOT_INIT();

    for (size_t i = 0; i < mDataBuffers.size(); i++) {
        mapper = mDataBuffers.valueAt(i);
        bm->unmap(mapper);
    }

    mDataBuffers.clear();
    // reset current buffer
    mCurrentDataBuffer = 0;
}

bool DisplayPlane::assignToDevice(int disp)
{
    RETURN_FALSE_IF_NOT_INIT();
    ALOGTRACE("disp = %d", disp);

    mDevice = disp;

    Drm *drm = Hwcomposer::getInstance().getDrm();
    if (!drm->getModeInfo(mDevice, mModeInfo)) {
        ELOGTRACE("failed to get mode info");
    }

    mPanelOrientation = drm->getPanelOrientation(mDevice);

    mForceScaling = DisplayQuery::forceFbScaling(disp);
    drm->getDisplayResolution(disp, mDisplayWidth, mDisplayHeight);

    if (mForceScaling) {
        mModeInfo.hdisplay = mDisplayWidth;
        mModeInfo.vdisplay = mDisplayHeight;

    }

    return true;
}

bool DisplayPlane::flip(void * /* ctx */)
{
    RETURN_FALSE_IF_NOT_INIT();

    // always flip
    return true;
}

void DisplayPlane::postFlip()
{
    mUpdateMasks = 0;
}

bool DisplayPlane::reset()
{
    // reclaim all allocated resources
    if (mDataBuffers.size() > 0) {
        invalidateBufferCache();
    }

    if (mActiveBuffers.size() > 0) {
        invalidateActiveBuffers();
    }

    return true;
}

void DisplayPlane::setZOrder(int zorder)
{
    mZOrder = zorder;
}

int DisplayPlane::getZOrder() const
{
    return mZOrder;
}

} // namespace intel
} // namespace android
