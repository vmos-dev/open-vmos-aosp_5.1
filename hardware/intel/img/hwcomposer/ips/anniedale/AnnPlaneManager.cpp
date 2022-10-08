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
#include <utils/String8.h>
#include <ips/anniedale/AnnPlaneManager.h>
#include <ips/anniedale/AnnRGBPlane.h>
#include <ips/anniedale/AnnOverlayPlane.h>
#include <ips/anniedale/AnnCursorPlane.h>
#include <PlaneCapabilities.h>

namespace android {
namespace intel {


struct PlaneDescription {
    char nickname;
    int type;
    int index;
};


static PlaneDescription PLANE_DESC[] =
{
    // nickname must be continous and start with 'A',
    // it is used to fast locate plane index and type
    {'A', DisplayPlane::PLANE_PRIMARY, 0},
    {'B', DisplayPlane::PLANE_PRIMARY, 1},
    {'C', DisplayPlane::PLANE_PRIMARY, 2},
    {'D', DisplayPlane::PLANE_SPRITE,  0},
    {'E', DisplayPlane::PLANE_SPRITE,  1},
    {'F', DisplayPlane::PLANE_SPRITE,  2},
    {'G', DisplayPlane::PLANE_OVERLAY, 0},  // nickname for Overlay A
    {'H', DisplayPlane::PLANE_OVERLAY, 1},  // nickname for Overlay C
    {'I', DisplayPlane::PLANE_CURSOR,  0},  // nickname for cursor A
    {'J', DisplayPlane::PLANE_CURSOR,  1},  // nickname for cursor B
    {'K', DisplayPlane::PLANE_CURSOR,  2}   // nickname for cursor C
};


struct ZOrderDescription {
    int index;  // based on overlay position
    const char *zorder;
};

// If overlay is in the bottom of Z order, two legitimate combinations are Oa, D, E, F
// and Oc, D, E, F. However, plane A has to be part of the blending chain as it can't
//  be disabled [HW bug]. The only legitimate combinations including overlay and plane A is:
// A, Oa, E, F
// A, Oc, E, F
#define OVERLAY_HW_WORKAROUND


// Cursor plane can be placed on top of any plane below and is intentionally ignored
// in the zorder table.

static ZOrderDescription PIPE_A_ZORDER_DESC[] =
{
    {0, "ADEF"},  // no overlay
#ifndef OVERLAY_HW_WORKAROUND
    {1, "GDEF"},  // overlay A at bottom (1 << 0)
    {1, "HDEF"},  // overlay C at bottom (1 << 0)
#else
    {1, "GEF"},  // overlay A at bottom (1 << 0)
    {1, "HEF"},  // overlay C at bottom (1 << 0)
#endif
    {2, "AGEF"},  // overlay A at next to bottom (1 << 1)
    {2, "AHEF"},  // overlay C at next to bottom (1 << 1)
#ifndef OVERLAY_HW_WORKAROUND
    {3, "GHEF"},  // overlay A, C at bottom
#else
    {3, "GHF"},   // overlay A, C at bottom
#endif
    {4, "ADGF"},  // overlay A at next to top (1 << 2)
    {4, "ADHF"},  // overlay C at next to top (1 << 2)
    {6, "AGHF"},  // overlay A, C in between
    {8, "ADEG"},  // overlay A at top (1 << 3)
    {8, "ADEH"},  // overlay C at top (1 <<3)
    {12, "ADGH"}  // overlay A, C at top
};

// use overlay C over overlay A if possible on pipe B
// workaround: use only overlay C on pipe B
static ZOrderDescription PIPE_B_ZORDER_DESC[] =
{
    {0, "BD"},    // no overlay
    {1, "GBD"},   // overlay A at bottom (1 << 0)
    {1, "HBD"},   // overlay C at bottom (1 << 0)
    {2, "BGD"},   // overlay A at middle (1 << 1)
    {2, "BHD"},   // overlay C at middle (1 << 1)
    {3, "GHBD"},  // overlay A and C at bottom ( 1 << 0 + 1 << 1)
    {4, "BDG"},   // overlay A at top (1 << 2)
    {4, "BDH"},   // overlay C at top (1 << 2)
    {6, "BGHD"},  // overlay A/C at middle  1 << 1 + 1 << 2)
    {12, "BDGH"}  // overlay A/C at top (1 << 2 + 1 << 3)
};

static const int PIPE_A_ZORDER_COMBINATIONS =
        sizeof(PIPE_A_ZORDER_DESC)/sizeof(ZOrderDescription);
static const int PIPE_B_ZORDER_COMBINATIONS =
        sizeof(PIPE_B_ZORDER_DESC)/sizeof(ZOrderDescription);

AnnPlaneManager::AnnPlaneManager()
    : DisplayPlaneManager()
{
}

AnnPlaneManager::~AnnPlaneManager()
{
}

bool AnnPlaneManager::initialize()
{
    mSpritePlaneCount = 3;  // Sprite D, E, F
    mOverlayPlaneCount = 2; // Overlay A, C
    mPrimaryPlaneCount = 3; // Primary A, B, C
    mCursorPlaneCount = 3;

    return DisplayPlaneManager::initialize();
}

void AnnPlaneManager::deinitialize()
{
    DisplayPlaneManager::deinitialize();
}

DisplayPlane* AnnPlaneManager::allocPlane(int index, int type)
{
    DisplayPlane *plane = NULL;

    switch (type) {
    case DisplayPlane::PLANE_PRIMARY:
        plane = new AnnRGBPlane(index, DisplayPlane::PLANE_PRIMARY, index/*disp*/);
        break;
    case DisplayPlane::PLANE_SPRITE:
        plane = new AnnRGBPlane(index, DisplayPlane::PLANE_SPRITE, 0/*disp*/);
        break;
    case DisplayPlane::PLANE_OVERLAY:
        plane = new AnnOverlayPlane(index, 0/*disp*/);
        break;
    case DisplayPlane::PLANE_CURSOR:
        plane = new AnnCursorPlane(index, index /*disp */);
        break;
    default:
        ELOGTRACE("unsupported type %d", type);
        break;
    }

    if (plane && !plane->initialize(DisplayPlane::MIN_DATA_BUFFER_COUNT)) {
        ELOGTRACE("failed to initialize plane.");
        DEINIT_AND_DELETE_OBJ(plane);
    }

    return plane;
}

bool AnnPlaneManager::isValidZOrder(int dsp, ZOrderConfig& config)
{
    int size = (int)config.size();

    if (size == 0 || size > 5) {
        VLOGTRACE("invalid z order config size %d", size);
        return false;
    }

    if (dsp == IDisplayDevice::DEVICE_PRIMARY) {
        int firstOverlay = -1;
        for (int i = 0; i < size; i++) {
            if (config[i]->planeType == DisplayPlane::PLANE_OVERLAY) {
                firstOverlay = i;
                break;
            }
        }

        int sprites = 0;
        for (int i = 0; i < size; i++) {
            if (config[i]->planeType != DisplayPlane::PLANE_OVERLAY &&
                config[i]->planeType != DisplayPlane::PLANE_CURSOR) {
                sprites++;
            }
        }

        if (firstOverlay < 0 && sprites > 4) {
            VLOGTRACE("not capable to support more than 4 sprite layers");
            return false;
        }

#ifdef OVERLAY_HW_WORKAROUND
        if (firstOverlay == 0 && sprites > 2) {
            VLOGTRACE("not capable to support 3 sprite layers on top of overlay");
            return false;
        }
#endif
    } else if (dsp == IDisplayDevice::DEVICE_EXTERNAL) {
        int sprites = 0;
        for (int i = 0; i < size; i++) {
            if (config[i]->planeType != DisplayPlane::PLANE_OVERLAY &&
                config[i]->planeType != DisplayPlane::PLANE_CURSOR) {
                sprites++;
            }
        }
        if (sprites > 2) {
            ELOGTRACE("number of sprite: %d, maximum 1 sprite and 1 primary supported on pipe 1", sprites);
            return false;
        }
    } else {
        ELOGTRACE("invalid display device %d", dsp);
        return false;
    }
    return true;
}

bool AnnPlaneManager::assignPlanes(int dsp, ZOrderConfig& config)
{
    if (dsp < 0 || dsp > IDisplayDevice::DEVICE_EXTERNAL) {
        ELOGTRACE("invalid display device %d", dsp);
        return false;
    }

    int size = (int)config.size();

    // calculate index based on overlay Z order position
    int index = 0;
    for (int i = 0; i < size; i++) {
        if (config[i]->planeType == DisplayPlane::PLANE_OVERLAY) {
            index += (1 << i);
        }
    }

    int combinations = 0;
    if (dsp == IDisplayDevice::DEVICE_PRIMARY)
        combinations = PIPE_A_ZORDER_COMBINATIONS;
    else
        combinations = PIPE_B_ZORDER_COMBINATIONS;

    ZOrderDescription *zorderDesc = NULL;
    for (int i = 0; i < combinations; i++) {
        if (dsp == IDisplayDevice::DEVICE_PRIMARY)
            zorderDesc = &PIPE_A_ZORDER_DESC[i];
        else
            zorderDesc = &PIPE_B_ZORDER_DESC[i];

        if (zorderDesc->index != index)
            continue;

        if (assignPlanes(dsp, config, zorderDesc->zorder)) {
            VLOGTRACE("zorder assigned %s", zorderDesc->zorder);
            return true;
        }
    }
    return false;
}

bool AnnPlaneManager::assignPlanes(int dsp, ZOrderConfig& config, const char *zorder)
{
    // zorder string does not include cursor plane, therefore cursor layer needs to be handled
    // in a special way. Cursor layer must be on top of zorder and no more than one cursor layer.

    int size = (int)config.size();

    if (zorder == NULL || size == 0) {
        //DLOGTRACE("invalid zorder or ZOrder config.");
        return false;
    }

    int zorderLen = (int)strlen(zorder);

    // test if plane is available
    for (int i = 0; i < size; i++) {
        if (config[i]->planeType == DisplayPlane::PLANE_CURSOR) {
            if (i != size - 1) {
                ELOGTRACE("invalid zorder of cursor layer");
                return false;
            }
            PlaneDescription& desc = PLANE_DESC['I' - 'A' + dsp];
            if (!isFreePlane(desc.type, desc.index)) {
                ELOGTRACE("cursor plane is not available");
                return false;
            }
            continue;
        }
        if (i >= zorderLen) {
            DLOGTRACE("index of ZOrderConfig is out of bound");
            return false;
        }
        char id = *(zorder + i);
        PlaneDescription& desc = PLANE_DESC[id - 'A'];
        if (!isFreePlane(desc.type, desc.index)) {
            DLOGTRACE("plane type %d index %d is not available", desc.type, desc.index);
            return false;
        }

#if 0
        // plane type check
        if (config[i]->planeType == DisplayPlane::PLANE_OVERLAY &&
            desc.type != DisplayPlane::PLANE_OVERLAY) {
            ELOGTRACE("invalid plane type %d, expected %d", desc.type, config[i]->planeType);
            return false;
        }

        if (config[i]->planeType != DisplayPlane::PLANE_OVERLAY) {
            if (config[i]->planeType != DisplayPlane::PLANE_PRIMARY &&
                config[i]->planeType != DisplayPlane::PLANE_SPRITE) {
                ELOGTRACE("invalid plane type %d,", config[i]->planeType);
                return false;
            }
            if (desc.type != DisplayPlane::PLANE_PRIMARY &&
                desc.type != DisplayPlane::PLANE_SPRITE) {
                ELOGTRACE("invalid plane type %d, expected %d", desc.type, config[i]->planeType);
                return false;
            }
        }
#endif

        if  (desc.type == DisplayPlane::PLANE_OVERLAY && desc.index == 1 &&
             config[i]->hwcLayer->getTransform() != 0) {
            DLOGTRACE("overlay C does not support transform");
            return false;
        }
    }

    bool primaryPlaneActive = false;
    // allocate planes
    for (int i = 0; i < size; i++) {
        if (config[i]->planeType == DisplayPlane::PLANE_CURSOR) {
            PlaneDescription& desc = PLANE_DESC['I' - 'A' + dsp];
            ZOrderLayer *zLayer = config.itemAt(i);
            zLayer->plane = getPlane(desc.type, desc.index);
            if (zLayer->plane == NULL) {
                ELOGTRACE("failed to get cursor plane, should never happen!");
            }
            continue;
        }
        char id = *(zorder + i);
        PlaneDescription& desc = PLANE_DESC[id - 'A'];
        ZOrderLayer *zLayer = config.itemAt(i);
        zLayer->plane = getPlane(desc.type, desc.index);
        if (zLayer->plane == NULL) {
            ELOGTRACE("failed to get plane, should never happen!");
        }
        // override type
        zLayer->planeType = desc.type;
        if (desc.type == DisplayPlane::PLANE_PRIMARY) {
            primaryPlaneActive = true;
        }
    }

    // setup Z order
    int slot = 0;
    for (int i = 0; i < size; i++) {
        slot = i;

#ifdef OVERLAY_HW_WORKAROUND
        if (!primaryPlaneActive && config[i]->planeType == DisplayPlane::PLANE_OVERLAY) {
            slot += 1;
        }
#endif

        config[i]->plane->setZOrderConfig(config, (void *)slot);
        config[i]->plane->enable();
    }

#if 0
    DLOGTRACE("config size %d, zorder %s", size, zorder);
    for (int i = 0; i < size; i++) {
        const ZOrderLayer *l = config.itemAt(i);
        ILOGTRACE("%d: plane type %d, index %d, zorder %d",
            i, l->planeType, l->plane->getIndex(), l->zorder);
    }
#endif

    return true;
}

void* AnnPlaneManager::getZOrderConfig() const
{
    return NULL;
}

int AnnPlaneManager::getFreePlanes(int dsp, int type)
{
    RETURN_NULL_IF_NOT_INIT();

    if (type != DisplayPlane::PLANE_SPRITE) {
        return DisplayPlaneManager::getFreePlanes(dsp, type);
    }

    if (dsp < 0 || dsp > IDisplayDevice::DEVICE_EXTERNAL) {
        ELOGTRACE("invalid display device %d", dsp);
        return 0;
    }

    uint32_t freePlanes = mFreePlanes[type] | mReclaimedPlanes[type];
    int start = 0;
    int stop = mSpritePlaneCount;
    if (dsp == IDisplayDevice::DEVICE_EXTERNAL) {
        // only Sprite D (index 0) can be assigned to pipe 1
        // Sprites E/F (index 1, 2) are fixed on pipe 0
        stop = 1;
    }
    int count = 0;
    for (int i = start; i < stop; i++) {
        if ((1 << i) & freePlanes) {
            count++;
        }
    }
    return count;
}

} // namespace intel
} // namespace android

