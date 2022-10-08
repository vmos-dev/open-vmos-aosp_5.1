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
#ifndef HWC_LAYER_H
#define HWC_LAYER_H

#include <hardware/hwcomposer.h>
#include <DisplayPlane.h>


namespace android {
namespace intel {

class HwcLayer {
public:
    enum {
        // LAYER_FB layers are marked as HWC_FRAMEBUFFER.
        // And a LAYER_FB can become HWC_OVERLAY layers during
        // revisiting layer list.
        LAYER_FB = 0,
        // LAYER_FORCE_FB layers are marked as HWC_FRAMEBUFFER.
        // And a LAYER_FORCE_FB can never become HWC_OVERLAY layers during
        // revisiting layer list.
        LAYER_FORCE_FB,
        // LAYER_OVERLAY layers are marked as HWC_OVERLAY
        LAYER_OVERLAY,
        // LAYER_SKIPPED layers are marked as HWC_OVERLAY with no plane attached
        LAYER_SKIPPED,
        // LAYER_FRAMEBUFFER_TARGET layers are marked as HWC_FRAMEBUFFER_TARGET
        LAYER_FRAMEBUFFER_TARGET,
        // LAYER_SIDEBAND layers have alternate path bypassing HWC after setup
        LAYER_SIDEBAND,
        // LAYER_CURSOR_OVERLAY layers support hardware cursor planes
        LAYER_CURSOR_OVERLAY,
    };

    enum {
        LAYER_PRIORITY_OVERLAY = 0x60000000UL,
        LAYER_PRIORITY_PROTECTED = 0x70000000UL,
        LAYER_PRIORITY_SIZE_OFFSET = 4,
    };
public:
    HwcLayer(int index, hwc_layer_1_t *layer);
    virtual ~HwcLayer();

    // plane operations
    bool attachPlane(DisplayPlane *plane, int device);
    DisplayPlane* detachPlane();

    void setType(uint32_t type);
    uint32_t getType() const;
    int32_t getCompositionType() const;
    void setCompositionType(int32_t type);

    int getIndex() const;
    int getZOrder() const;
    uint32_t getFormat() const;
    uint32_t getBufferWidth() const;
    uint32_t getBufferHeight() const;
    const stride_t& getBufferStride() const;
    uint32_t getUsage() const;
    uint32_t getHandle() const;
    uint32_t getTransform() const;
    bool isProtected() const;
    hwc_layer_1_t* getLayer() const;
    DisplayPlane* getPlane() const;

    void setPriority(uint32_t priority);
    uint32_t getPriority() const;

    bool update(hwc_layer_1_t *layer);
    void postFlip();
    bool isUpdated();

public:
    // temporary solution for plane assignment
    bool mPlaneCandidate;

private:
    void setupAttributes();

private:
    const int mIndex;
    int mZOrder;
    int mDevice;
    hwc_layer_1_t *mLayer;
    DisplayPlane *mPlane;
    uint32_t mFormat;
    uint32_t mWidth;
    uint32_t mHeight;
    stride_t mStride;
    uint32_t mUsage;
    uint32_t mHandle;
    bool mIsProtected;
    uint32_t mType;
    uint32_t mPriority;
    uint32_t mTransform;

    // for smart composition
    hwc_frect_t mSourceCropf;
    hwc_rect_t mDisplayFrame;
    bool mUpdated;
};


} // namespace intel
} // namespace android


#endif /* HWC_LAYER_H */
