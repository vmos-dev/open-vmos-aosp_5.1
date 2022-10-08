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
#ifndef HWC_LAYER_LIST_H
#define HWC_LAYER_LIST_H

#include <common/utils/Dump.h>
#include <hardware/hwcomposer.h>
#include <utils/SortedVector.h>
#include <DataBuffer.h>
#include <DisplayPlane.h>
#include <DisplayPlaneManager.h>
#include <common/base/HwcLayer.h>

namespace android {
namespace intel {


class HwcLayerList {
public:
    HwcLayerList(hwc_display_contents_1_t *list, int disp);
    virtual ~HwcLayerList();

public:
    virtual bool initialize();
    virtual void deinitialize();

    virtual bool update(hwc_display_contents_1_t *list);
    virtual DisplayPlane* getPlane(uint32_t index) const;

    void postFlip();

    // dump interface
    virtual void dump(Dump& d);


private:
    bool checkSupported(int planeType, HwcLayer *hwcLayer);
    bool checkRgbOverlaySupported(HwcLayer *hwcLayer);
    bool checkCursorSupported(HwcLayer *hwcLayer);
    bool allocatePlanes();
    bool assignCursorPlanes();
    bool assignCursorPlanes(int index, int planeNumber);
    bool assignOverlayPlanes();
    bool assignOverlayPlanes(int index, int planeNumber);
    bool assignSpritePlanes();
    bool assignSpritePlanes(int index, int planeNumber);
    bool assignPrimaryPlane();
    bool assignPrimaryPlaneHelper(HwcLayer *hwcLayer, int zorder = -1);
    bool attachPlanes();
    bool useAsFrameBufferTarget(HwcLayer *target);
    bool hasIntersection(HwcLayer *la, HwcLayer *lb);
    ZOrderLayer* addZOrderLayer(int type, HwcLayer *hwcLayer, int zorder = -1);
    void removeZOrderLayer(ZOrderLayer *layer);
    void setupSmartComposition();
    void dump();

private:
    class HwcLayerVector : public SortedVector<HwcLayer*> {
    public:
        HwcLayerVector() {}
        virtual int do_compare(const void* lhs, const void* rhs) const {
            const HwcLayer* l = *(HwcLayer**)lhs;
            const HwcLayer* r = *(HwcLayer**)rhs;
            // sorted from index 0 to n
            return l->getIndex() - r->getIndex();
        }
    };

    class PriorityVector : public SortedVector<HwcLayer*> {
    public:
        PriorityVector() {}
        virtual int do_compare(const void* lhs, const void* rhs) const {
            const HwcLayer* l = *(HwcLayer**)lhs;
            const HwcLayer* r = *(HwcLayer**)rhs;
            return r->getPriority() - l->getPriority();
        }
    };

    hwc_display_contents_1_t *mList;
    int mLayerCount;

    HwcLayerVector mLayers;
    HwcLayerVector mFBLayers;
    PriorityVector mSpriteCandidates;
    PriorityVector mOverlayCandidates;
    PriorityVector mCursorCandidates;
    ZOrderConfig mZOrderConfig;
    HwcLayer *mFrameBufferTarget;
    int mDisplayIndex;
};

} // namespace intel
} // namespace android


#endif /* HWC_LAYER_LIST_H */
