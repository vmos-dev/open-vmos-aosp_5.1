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
#include <common/base/DisplayAnalyzer.h>

namespace android {
namespace intel {

DisplayAnalyzer::DisplayAnalyzer()
    : mInitialized(false),
      mCachedNumDisplays(0),
      mCachedDisplays(0),
      mPendingEvents(),
      mEventMutex()
{
}

DisplayAnalyzer::~DisplayAnalyzer()
{
}

bool DisplayAnalyzer::initialize()
{
    mCachedNumDisplays = 0;
    mCachedDisplays = 0;
    mPendingEvents.clear();
    mInitialized = true;

    return true;
}

void DisplayAnalyzer::deinitialize()
{
    mPendingEvents.clear();
    mInitialized = false;
}

void DisplayAnalyzer::analyzeContents(
        size_t numDisplays, hwc_display_contents_1_t** displays)
{
    // cache and use them only in this context during analysis
    mCachedNumDisplays = numDisplays;
    mCachedDisplays = displays;

    handlePendingEvents();
}

void DisplayAnalyzer::postHotplugEvent(bool connected)
{
    // handle hotplug event (vsync switch) asynchronously
    Event e;
    e.type = HOTPLUG_EVENT;
    e.bValue = connected;
    postEvent(e);
    Hwcomposer::getInstance().invalidate();
}

void DisplayAnalyzer::postEvent(Event& e)
{
    Mutex::Autolock lock(mEventMutex);
    mPendingEvents.add(e);
}

bool DisplayAnalyzer::getEvent(Event& e)
{
    Mutex::Autolock lock(mEventMutex);
    if (mPendingEvents.size() == 0) {
        return false;
    }
    e = mPendingEvents[0];
    mPendingEvents.removeAt(0);
    return true;
}

void DisplayAnalyzer::handlePendingEvents()
{
    // handle one event per analysis to avoid blocking surface flinger
    // some event may take lengthy time to process
    Event e;
    if (!getEvent(e)) {
        return;
    }

    switch (e.type) {
    case HOTPLUG_EVENT:
        handleHotplugEvent(e.bValue);
        break;
    }
}

void DisplayAnalyzer::handleHotplugEvent(bool connected)
{
    if (connected) {
        for (int i = 0; i < mCachedNumDisplays; i++) {
            setCompositionType(i, HWC_FRAMEBUFFER, true);
        }
    }
}

void DisplayAnalyzer::setCompositionType(hwc_display_contents_1_t *display, int type)
{
    for (size_t i = 0; i < display->numHwLayers - 1; i++) {
        hwc_layer_1_t *layer = &display->hwLayers[i];
        if (layer) layer->compositionType = type;
    }
}

void DisplayAnalyzer::setCompositionType(int device, int type, bool reset)
{
    hwc_display_contents_1_t *content = mCachedDisplays[device];
    if (content == NULL) {
        ELOGTRACE("Invalid device %d", device);
        return;
    }

    // don't need to set geometry changed if layers are just needed to be marked
    if (reset) {
        content->flags |= HWC_GEOMETRY_CHANGED;
    }

    setCompositionType(content, type);
}

} // namespace intel
} // namespace android

