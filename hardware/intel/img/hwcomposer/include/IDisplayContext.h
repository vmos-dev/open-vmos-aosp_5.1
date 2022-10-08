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
#ifndef IDISPLAY_CONTEXT_H
#define IDISPLAY_CONTEXT_H

#include <hardware/hwcomposer.h>

namespace android {
namespace intel {

class HwcLayerList;

class IDisplayContext {
public:
    IDisplayContext() {}
    virtual ~IDisplayContext() {}
public:
    virtual bool initialize() = 0;
    virtual void deinitialize() = 0;
    virtual bool commitBegin(size_t numDisplays, hwc_display_contents_1_t **displays) = 0;
    virtual bool commitContents(hwc_display_contents_1_t *display, HwcLayerList *layerList) = 0;
    virtual bool commitEnd(size_t numDisplays, hwc_display_contents_1_t **displays) = 0;
    virtual bool compositionComplete() = 0;
    virtual bool setCursorPosition(int disp, int x, int y) = 0;
};

}
}

#endif /* IDISPLAY_CONTEXT_H */
