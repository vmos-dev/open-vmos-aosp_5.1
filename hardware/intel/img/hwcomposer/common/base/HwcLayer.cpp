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
#include <common/base/Drm.h>
#include <common/base/HwcLayer.h>
#include <Hwcomposer.h>
#include <GraphicBuffer.h>
#include <IDisplayDevice.h>
#include <DisplayQuery.h>
#include <PlaneCapabilities.h>


namespace android {
namespace intel {

inline bool operator==(const hwc_rect_t& x, const hwc_rect_t& y)
{
    return (x.top == y.top &&
            x.bottom == y.bottom &&
            x.left == y.left &&
            x.right == y.right);
}

inline bool operator !=(const hwc_rect_t& x, const hwc_rect_t& y)
{
    return !operator==(x, y);
}

inline bool operator ==(const hwc_frect_t& x, const hwc_frect_t& y)
{
    return (x.top == y.top &&
            x.bottom == y.bottom &&
            x.left == y.left &&
            x.right == y.right);
}

inline bool operator !=(const hwc_frect_t& x, const hwc_frect_t& y)
{
    return !operator==(x, y);
}

HwcLayer::HwcLayer(int index, hwc_layer_1_t *layer)
    : mIndex(index),
      mZOrder(index + 1),  // 0 is reserved for frame buffer target
      mDevice(0),
      mLayer(layer),
      mPlane(0),
      mFormat(DataBuffer::FORMAT_INVALID),
      mWidth(0),
      mHeight(0),
      mUsage(0),
      mHandle(0),
      mIsProtected(false),
      mType(LAYER_FB),
      mPriority(0),
      mTransform(0),
      mUpdated(false)
{
    memset(&mSourceCropf, 0, sizeof(mSourceCropf));
    memset(&mDisplayFrame, 0, sizeof(mDisplayFrame));
    memset(&mStride, 0, sizeof(mStride));

    mPlaneCandidate = false;
    setupAttributes();
}

HwcLayer::~HwcLayer()
{
    if (mPlane) {
        WLOGTRACE("HwcLayer is not cleaned up");
    }

    mLayer = NULL;
    mPlane = NULL;
}

bool HwcLayer::attachPlane(DisplayPlane* plane, int device)
{
    if (mPlane) {
        ELOGTRACE("failed to attach plane, plane exists");
        return false;
    }

    if (!plane) {
        ELOGTRACE("Invalid plane");
        return false;
    }

    mDevice = device;
    //plane->setZOrder(mIndex);
    plane->assignToDevice(device);
    mPlane = plane;
    return true;
}

DisplayPlane* HwcLayer::detachPlane()
{
    // reset plane's z order
    if (mPlane)
        mPlane->setZOrder(-1);
    DisplayPlane *plane = mPlane;
    mPlane = 0;
    mDevice = 0;
    return plane;
}

void HwcLayer::setType(uint32_t type)
{
    if (!mLayer)
        return;

    switch (type) {
    case LAYER_OVERLAY:
    case LAYER_SKIPPED:
        mLayer->compositionType = HWC_OVERLAY;
        mLayer->hints |= HWC_HINT_CLEAR_FB;
        break;
    // NOTE: set compositionType to HWC_FRAMEBUFFER here so that we have
    // a chance to submit the primary changes to HW.
    // Upper layer HWComposer will reset the compositionType automatically.
    case LAYER_FB:
    case LAYER_FORCE_FB:
        mLayer->compositionType = HWC_FRAMEBUFFER;
        break;
    case LAYER_SIDEBAND:
        mLayer->compositionType = HWC_SIDEBAND;
        break;
    case LAYER_CURSOR_OVERLAY:
        mLayer->compositionType = HWC_CURSOR_OVERLAY;
        break;
    default:
        break;
    }

    mType = type;
}

uint32_t HwcLayer::getType() const
{
    return mType;
}

void HwcLayer::setCompositionType(int32_t type)
{
    mLayer->compositionType = type;
}

int32_t HwcLayer::getCompositionType() const
{
    return mLayer->compositionType;
}

int HwcLayer::getIndex() const
{
    return mIndex;
}

int HwcLayer::getZOrder() const
{
    return mZOrder;
}

uint32_t HwcLayer::getFormat() const
{
    return mFormat;
}

uint32_t HwcLayer::getBufferWidth() const
{
    return mWidth;
}

uint32_t HwcLayer::getBufferHeight() const
{
    return mHeight;
}

const stride_t& HwcLayer::getBufferStride() const
{
    return mStride;
}

uint32_t HwcLayer::getUsage() const
{
    return mUsage;
}

uint32_t HwcLayer::getHandle() const
{
    return mHandle;
}

uint32_t HwcLayer::getTransform() const
{
    return mTransform;
}

bool HwcLayer::isProtected() const
{
    return mIsProtected;
}

hwc_layer_1_t* HwcLayer::getLayer() const
{
    return mLayer;
}

DisplayPlane* HwcLayer::getPlane() const
{
    return mPlane;
}

void HwcLayer::setPriority(uint32_t priority)
{
    mPriority = priority;
}

uint32_t HwcLayer::getPriority() const
{
    return mPriority;
}

bool HwcLayer::update(hwc_layer_1_t *layer)
{
    // update layer
    mLayer = layer;
    setupAttributes();

    // if not a FB layer & a plane was attached update plane's data buffer
    if (mPlane) {
        mPlane->setPosition(layer->displayFrame.left,
                            layer->displayFrame.top,
                            layer->displayFrame.right - layer->displayFrame.left,
                            layer->displayFrame.bottom - layer->displayFrame.top);
        mPlane->setSourceCrop(layer->sourceCropf.left,
                              layer->sourceCropf.top,
                              layer->sourceCropf.right - layer->sourceCropf.left,
                              layer->sourceCropf.bottom - layer->sourceCropf.top);
        mPlane->setTransform(layer->transform);
        mPlane->setPlaneAlpha(layer->planeAlpha, layer->blending);
        bool ret = mPlane->setDataBuffer((uint32_t)layer->handle);
        if (ret == true) {
            return true;
        }
        DLOGTRACE("failed to set data buffer, reset handle to 0!!");
        mHandle = 0;
        if (!mIsProtected) {
            // typical case: rotated buffer is not ready or handle is null
            return false;
        } else {
            // protected video has to be rendered using overlay.
            // if buffer is not ready overlay will still be attached to this layer
            // but rendering needs to be skipped.
            WLOGTRACE("ignoring result of data buffer setting for protected video");
            return true;
        }
    }

    return true;
}

bool HwcLayer::isUpdated()
{
    return mUpdated;
}

void HwcLayer::postFlip()
{
    mUpdated = false;
    if (mPlane) {
        mPlane->postFlip();
    }
}

void HwcLayer::setupAttributes()
{
    if ((mLayer->flags & HWC_SKIP_LAYER) ||
        mTransform != mLayer->transform ||
        mSourceCropf != mLayer->sourceCropf ||
        mDisplayFrame != mLayer->displayFrame ||
        mHandle != (uint32_t)mLayer->handle ||
        DisplayQuery::isVideoFormat(mFormat)) {
        // TODO: same handle does not mean there is always no update
        mUpdated = true;
    }

    // update handle always as it can become "NULL"
    // if the given layer is not ready
    mTransform = mLayer->transform;
    mSourceCropf = mLayer->sourceCropf;
    mDisplayFrame = mLayer->displayFrame;
    mHandle = (uint32_t)mLayer->handle;

    if (mFormat != DataBuffer::FORMAT_INVALID) {
        // other attributes have been set.
        return;
    }

    if (mLayer->handle == NULL) {
        VLOGTRACE("invalid handle");
        return;
    }

    BufferManager *bm = Hwcomposer::getInstance().getBufferManager();
    if (bm == NULL) {
        // TODO: this check is redundant
        return;
    }

    DataBuffer *buffer = bm->lockDataBuffer((uint32_t)mLayer->handle);
     if (!buffer) {
         ELOGTRACE("failed to get buffer");
     } else {
        mFormat = buffer->getFormat();
        mWidth = buffer->getWidth();
        mHeight = buffer->getHeight();
        mStride = buffer->getStride();
        mPriority = (mSourceCropf.right - mSourceCropf.left) * (mSourceCropf.bottom - mSourceCropf.top);
        mPriority <<= LAYER_PRIORITY_SIZE_OFFSET;
        mPriority |= mIndex;
        GraphicBuffer *gBuffer = (GraphicBuffer*)buffer;
        mUsage = gBuffer->getUsage();
        mIsProtected = GraphicBuffer::isProtectedBuffer((GraphicBuffer*)buffer);
        if (mIsProtected) {
            mPriority |= LAYER_PRIORITY_PROTECTED;
        } else if (PlaneCapabilities::isFormatSupported(DisplayPlane::PLANE_OVERLAY, this)) {
            mPriority |= LAYER_PRIORITY_OVERLAY;
        }
        bm->unlockDataBuffer(buffer);
    }
}

} // namespace intel
} // namespace android
