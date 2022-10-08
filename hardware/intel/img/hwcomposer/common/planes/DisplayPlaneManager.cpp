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
#include <IDisplayDevice.h>
#include <DisplayPlaneManager.h>

namespace android {
namespace intel {

DisplayPlaneManager::DisplayPlaneManager()
    : mTotalPlaneCount(0),
      mPrimaryPlaneCount(DEFAULT_PRIMARY_PLANE_COUNT),
      mSpritePlaneCount(0),
      mOverlayPlaneCount(0),
      mInitialized(false)
{
    int i;

    for (i = 0; i < DisplayPlane::PLANE_MAX; i++) {
        mPlaneCount[i] = 0;
        mFreePlanes[i] = 0;
        mReclaimedPlanes[i] = 0;
    }
}

DisplayPlaneManager::~DisplayPlaneManager()
{
    WARN_IF_NOT_DEINIT();
}

void DisplayPlaneManager::deinitialize()
{
    int i;
    size_t j;

    for (i = 0; i < DisplayPlane::PLANE_MAX; i++) {
        for (j = 0; j < mPlanes[i].size(); j++) {
            // reset plane
            DisplayPlane *plane = mPlanes[i].itemAt(j);
            plane->reset();

            DEINIT_AND_DELETE_OBJ(plane);
        }
        mPlanes[i].clear();
    }

    mInitialized = false;
}

bool DisplayPlaneManager::initialize()
{
    int i, j;

    if (mInitialized) {
        WLOGTRACE("object has been initialized");
        return true;
    }


    // calculate total plane number and free plane bitmaps
    mPlaneCount[DisplayPlane::PLANE_SPRITE] = mSpritePlaneCount;
    mPlaneCount[DisplayPlane::PLANE_OVERLAY] = mOverlayPlaneCount;
    mPlaneCount[DisplayPlane::PLANE_PRIMARY] = mPrimaryPlaneCount;
    mPlaneCount[DisplayPlane::PLANE_CURSOR] = mCursorPlaneCount;

    mTotalPlaneCount = mSpritePlaneCount+ mOverlayPlaneCount+ mPrimaryPlaneCount + mCursorPlaneCount;
    if (mTotalPlaneCount == 0) {
        ELOGTRACE("plane count is not initialized");
        return false;
    }

    for (i = 0; i < DisplayPlane::PLANE_MAX; i++) {
        mFreePlanes[i] = ((1 << mPlaneCount[i]) - 1);
    }

    // allocate plane pools
    for (i = 0; i < DisplayPlane::PLANE_MAX; i++) {
        if (mPlaneCount[i]) {
            mPlanes[i].setCapacity(mPlaneCount[i]);

            for (j = 0; j < mPlaneCount[i]; j++) {
                DisplayPlane* plane = allocPlane(j, i);
                if (!plane) {
                    ELOGTRACE("failed to allocate plane %d, type %d", j, i);
                    DEINIT_AND_RETURN_FALSE();
                }
                mPlanes[i].push_back(plane);
            }
        }
    }

    mInitialized = true;
    return true;
}

int DisplayPlaneManager::getPlane(uint32_t& mask)
{
    if (!mask)
        return -1;

    for (int i = 0; i < 32; i++) {
        int bit = (1 << i);
        if (bit & mask) {
            mask &= ~bit;
            return i;
        }
    }

    return -1;
}

void DisplayPlaneManager::putPlane(int index, uint32_t& mask)
{
    if (index < 0 || index >= 32)
        return;

    int bit = (1 << index);

    if (bit & mask) {
        WLOGTRACE("bit %d was set", index);
        return;
    }

    mask |= bit;
}

int DisplayPlaneManager::getPlane(uint32_t& mask, int index)
{
    if (!mask || index < 0 || index > mTotalPlaneCount)
        return -1;

    int bit = (1 << index);
    if (bit & mask) {
        mask &= ~bit;
        return index;
    }

    return -1;
}

DisplayPlane* DisplayPlaneManager::getPlane(int type, int index)
{
    RETURN_NULL_IF_NOT_INIT();

    if (type < 0 || type >= DisplayPlane::PLANE_MAX) {
        ELOGTRACE("Invalid plane type %d", type);
        return 0;
    }

    int freePlaneIndex = getPlane(mReclaimedPlanes[type], index);
    if (freePlaneIndex >= 0)
        return mPlanes[type].itemAt(freePlaneIndex);

    freePlaneIndex = getPlane(mFreePlanes[type], index);
    if (freePlaneIndex >= 0)
        return mPlanes[type].itemAt(freePlaneIndex);

    return 0;
}

DisplayPlane* DisplayPlaneManager::getAnyPlane(int type)
{
    RETURN_NULL_IF_NOT_INIT();

    if (type < 0 || type >= DisplayPlane::PLANE_MAX) {
        ELOGTRACE("Invalid plane type %d", type);
        return 0;
    }

    int freePlaneIndex = getPlane(mReclaimedPlanes[type]);
    if (freePlaneIndex >= 0)
        return mPlanes[type].itemAt(freePlaneIndex);

    freePlaneIndex = getPlane(mFreePlanes[type]);
    if (freePlaneIndex >= 0)
        return mPlanes[type].itemAt(freePlaneIndex);

    return 0;
}

void DisplayPlaneManager::putPlane(int /* dsp */, DisplayPlane& plane)
{
    int index;
    int type;

    RETURN_VOID_IF_NOT_INIT();

    index = plane.getIndex();
    type = plane.getType();

    if (type < 0 || type >= DisplayPlane::PLANE_MAX) {
        ELOGTRACE("Invalid plane type %d", type);
        return;
    }

    putPlane(index, mFreePlanes[type]);
}

bool DisplayPlaneManager::isFreePlane(int type, int index)
{
    if (type < 0 || type >= DisplayPlane::PLANE_MAX) {
        ELOGTRACE("Invalid plane type %d", type);
        return false;
    }

    int freePlanes = mFreePlanes[type] | mReclaimedPlanes[type];
    if ((freePlanes & (1 << index)) == 0)
        return false;

    return true;
}

int DisplayPlaneManager::getFreePlanes(int dsp, int type)
{
    RETURN_NULL_IF_NOT_INIT();

    if (dsp < 0 || dsp > IDisplayDevice::DEVICE_EXTERNAL) {
        ELOGTRACE("Invalid display device %d", dsp);
        return 0;
    }

    if (type < 0 || type >= DisplayPlane::PLANE_MAX) {
        ELOGTRACE("Invalid plane type %d", type);
        return 0;
    }


    uint32_t freePlanes = mFreePlanes[type] | mReclaimedPlanes[type];
    if (type == DisplayPlane::PLANE_PRIMARY ||
        type == DisplayPlane::PLANE_CURSOR) {
        return ((freePlanes & (1 << dsp)) == 0) ? 0 : 1;
    } else {
        int count = 0;
        for (int i = 0; i < 32; i++) {
            if ((1 << i) & freePlanes) {
                count++;
            }
        }
        return count;
    }
    return 0;
}

void DisplayPlaneManager::reclaimPlane(int /* dsp */, DisplayPlane& plane)
{
    RETURN_VOID_IF_NOT_INIT();

    int index = plane.getIndex();
    int type = plane.getType();

    ALOGTRACE("reclaimPlane = %d, type = %d", index, plane.getType());

    if (type < 0 || type >= DisplayPlane::PLANE_MAX) {
        ELOGTRACE("Invalid plane type %d", type);
        return;
    }

    putPlane(index, mReclaimedPlanes[type]);

    // NOTE: don't invalidate plane's data cache here because the reclaimed
    // plane might be re-assigned to the same layer later
}

void DisplayPlaneManager::disableReclaimedPlanes()
{
    int i, j;
    bool ret;

    RETURN_VOID_IF_NOT_INIT();

    for (i = 0; i < DisplayPlane::PLANE_MAX; i++) {
        // disable reclaimed planes
        if (mReclaimedPlanes[i]) {
            for (j = 0; j < mPlaneCount[i]; j++) {
                int bit = (1 << j);
                if (mReclaimedPlanes[i] & bit) {
                    DisplayPlane* plane = mPlanes[i].itemAt(j);
                    // check plane state first
                    ret = plane->isDisabled();
                    // reset plane
                    if (ret)
                        ret = plane->reset();
                    if (ret) {
                        // only merge into free bitmap if it is successfully disabled and reset
                        // otherwise, plane will be disabled and reset again.
                        mFreePlanes[i] |=bit;
                        mReclaimedPlanes[i] &= ~bit;
                    }
                }
            }
        }
    }
}

bool DisplayPlaneManager::isOverlayPlanesDisabled()
{
    for (int i = 0; i < DisplayPlane::PLANE_MAX; i++) {
        for (int j = 0; j < mPlaneCount[i]; j++) {
            DisplayPlane* plane = (DisplayPlane *)mPlanes[i][j];
            if (plane && plane->getType() == DisplayPlane::PLANE_OVERLAY) {
                if (!plane->isDisabled())
                    return false;
            }
        }
    }

    return true;
}

void DisplayPlaneManager::dump(Dump& d)
{
    d.append("Display Plane Manager state:\n");
    d.append("-------------------------------------------------------------\n");
    d.append(" PLANE TYPE | COUNT |   FREE   | RECLAIMED \n");
    d.append("------------+-------+----------+-----------\n");
    d.append("    SPRITE  |  %2d   | %08x | %08x\n",
             mPlaneCount[DisplayPlane::PLANE_SPRITE],
             mFreePlanes[DisplayPlane::PLANE_SPRITE],
             mReclaimedPlanes[DisplayPlane::PLANE_SPRITE]);
    d.append("   OVERLAY  |  %2d   | %08x | %08x\n",
             mPlaneCount[DisplayPlane::PLANE_OVERLAY],
             mFreePlanes[DisplayPlane::PLANE_OVERLAY],
             mReclaimedPlanes[DisplayPlane::PLANE_OVERLAY]);
    d.append("   PRIMARY  |  %2d   | %08x | %08x\n",
             mPlaneCount[DisplayPlane::PLANE_PRIMARY],
             mFreePlanes[DisplayPlane::PLANE_PRIMARY],
             mReclaimedPlanes[DisplayPlane::PLANE_PRIMARY]);
    d.append("   CURSOR   |  %2d   | %08x | %08x\n",
             mPlaneCount[DisplayPlane::PLANE_CURSOR],
             mFreePlanes[DisplayPlane::PLANE_CURSOR],
             mReclaimedPlanes[DisplayPlane::PLANE_CURSOR]);
}

} // namespace intel
} // namespace android


