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
#ifndef DISPLAYPLANEMANAGER_H_
#define DISPLAYPLANEMANAGER_H_

#include <common/utils/Dump.h>
#include <DisplayPlane.h>
#include <common/base/HwcLayer.h>
#include <utils/Vector.h>

namespace android {
namespace intel {

struct ZOrderLayer
{
    ZOrderLayer() {
        memset(this, 0, sizeof(ZOrderLayer));
    }

    inline bool operator<(const ZOrderLayer& rhs) const {
        return zorder < rhs.zorder;
    }

    int planeType;
    int zorder;
    DisplayPlane *plane;
    HwcLayer *hwcLayer;
};

class ZOrderConfig : public SortedVector<ZOrderLayer*> {
public:
    ZOrderConfig() {}

    int do_compare(const void* lhs, const void* rhs) const {
        const ZOrderLayer *l = *(ZOrderLayer**)lhs;
        const ZOrderLayer *r = *(ZOrderLayer**)rhs;

        // sorted from z order 0 to n
        return l->zorder - r->zorder;
    }
};


class DisplayPlaneManager {
public:
    DisplayPlaneManager();
    virtual ~DisplayPlaneManager();

public:
    virtual bool initialize();
    virtual void deinitialize();

    virtual bool isValidZOrder(int dsp, ZOrderConfig& config) = 0;
    virtual bool assignPlanes(int dsp, ZOrderConfig& config) = 0;
    // TODO: remove this API
    virtual void* getZOrderConfig() const = 0;
    virtual int getFreePlanes(int dsp, int type);
    virtual void reclaimPlane(int dsp, DisplayPlane& plane);
    virtual void disableReclaimedPlanes();
    virtual bool isOverlayPlanesDisabled();
    // dump interface
    virtual void dump(Dump& d);

protected:
    // plane allocation & free
    int getPlane(uint32_t& mask);
    int getPlane(uint32_t& mask, int index);
    DisplayPlane* getPlane(int type, int index);
    DisplayPlane* getAnyPlane(int type);
    void putPlane(int index, uint32_t& mask);
    void putPlane(int dsp, DisplayPlane& plane);
    bool isFreePlane(int type, int index);
    virtual DisplayPlane* allocPlane(int index, int type) = 0;

protected:
    int mPlaneCount[DisplayPlane::PLANE_MAX];
    int mTotalPlaneCount;
    int mPrimaryPlaneCount;
    int mSpritePlaneCount;
    int mOverlayPlaneCount;
    int mCursorPlaneCount;

    Vector<DisplayPlane*> mPlanes[DisplayPlane::PLANE_MAX];

    // Bitmap of free planes. Bit0 - plane A, bit 1 - plane B, etc.
    uint32_t mFreePlanes[DisplayPlane::PLANE_MAX];
    uint32_t mReclaimedPlanes[DisplayPlane::PLANE_MAX];

    bool mInitialized;

enum {
    DEFAULT_PRIMARY_PLANE_COUNT = 3
};
};

} // namespace intel
} // namespace android

#endif /* DISPLAYPLANEMANAGER_H_ */
