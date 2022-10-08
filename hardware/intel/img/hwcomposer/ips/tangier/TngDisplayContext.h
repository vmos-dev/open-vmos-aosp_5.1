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
#ifndef TNG_DISPLAY_CONTEXT_H
#define TNG_DISPLAY_CONTEXT_H

#include <IDisplayContext.h>
#include <hal_public.h>

namespace android {
namespace intel {

class TngDisplayContext : public IDisplayContext {
public:
    TngDisplayContext();
    virtual ~TngDisplayContext();
public:
    bool initialize();
    void deinitialize();
    bool commitBegin(size_t numDisplays, hwc_display_contents_1_t **displays);
    bool commitContents(hwc_display_contents_1_t *display, HwcLayerList* layerList);
    bool commitEnd(size_t numDisplays, hwc_display_contents_1_t **displays);
    bool compositionComplete();
    bool setCursorPosition(int disp, int x, int y);

private:
    enum {
        MAXIMUM_LAYER_NUMBER = 20,
    };
    IMG_display_device_public_t *mIMGDisplayDevice;
    IMG_hwc_layer_t mImgLayers[MAXIMUM_LAYER_NUMBER];
    bool mInitialized;
    size_t mCount;
};

} // namespace intel
} // namespace android

#endif /* TNG_DISPLAY_CONTEXT_H */
