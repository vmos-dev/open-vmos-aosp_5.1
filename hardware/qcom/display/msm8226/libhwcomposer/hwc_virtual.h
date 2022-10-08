/*
 * Copyright (C) 2010 The Android Open Source Project
 * Copyright (C) 2012-2014, The Linux Foundation. All rights reserved.
 *
 * Not a Contribution, Apache license notifications and license are retained
 * for attribution purposes only.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef HWC_VIRTUAL
#define HWC_VIRTUAL

#include <hwc_utils.h>
#include <virtual.h>

namespace qhwc {
namespace ovutils = overlay::utils;

// Base and abstract class for VDS and V4L2 wfd design.
class HWCVirtualBase {
public:
    explicit HWCVirtualBase(){};
    virtual ~HWCVirtualBase(){};
    // instantiates and returns the pointer to VDS or V4L2 object.
    static HWCVirtualBase* getObject(bool isVDSEnabled);
    virtual int prepare(hwc_composer_device_1 *dev,
                          hwc_display_contents_1_t* list) = 0;
    virtual int set(hwc_context_t *ctx, hwc_display_contents_1_t *list) = 0;
    virtual void init(hwc_context_t *ctx) = 0;
    virtual void destroy(hwc_context_t *ctx, size_t numDisplays,
                       hwc_display_contents_1_t** displays) = 0;
    virtual void pause(hwc_context_t* ctx, int dpy) = 0;
    virtual void resume(hwc_context_t* ctx, int dpy) = 0;
};

class HWCVirtualVDS : public HWCVirtualBase {
public:
    explicit HWCVirtualVDS();
    virtual ~HWCVirtualVDS(){};
    // Chooses composition type and configures pipe for each layer in virtual
    // display list
    virtual int prepare(hwc_composer_device_1 *dev,
                          hwc_display_contents_1_t* list);
    // Queues the buffer for each layer in virtual display list and call display
    // commit.
    virtual int set(hwc_context_t *ctx, hwc_display_contents_1_t *list);
    // instantiates mdpcomp, copybit and fbupdate objects and initialize those
    // objects for virtual display during virtual display connect.
    virtual void init(hwc_context_t *ctx);
    // Destroys mdpcomp, copybit and fbupdate objects and for virtual display
    // during virtual display disconnect.
    virtual void destroy(hwc_context_t *ctx, size_t numDisplays,
                       hwc_display_contents_1_t** displays);
    virtual void pause(hwc_context_t* ctx, int dpy);
    virtual void resume(hwc_context_t* ctx, int dpy);
private:
    // If WFD is enabled through VDS solution
    // we can dump the frame buffer and WB
    // output buffer by setting the property
    // debug.hwc.enable_vds_dump
    bool mVDSDumpEnabled;
};

class HWCVirtualV4L2 : public HWCVirtualBase {
public:
    explicit HWCVirtualV4L2(){};
    virtual ~HWCVirtualV4L2(){};
    // Chooses composition type and configures pipe for each layer in virtual
    // display list
    virtual int prepare(hwc_composer_device_1 *dev,
                         hwc_display_contents_1_t* list);
    // Queues the buffer for each layer in virtual display list and call
    // display commit.
    virtual int set(hwc_context_t *ctx, hwc_display_contents_1_t *list);
    // instantiates mdpcomp, copybit and fbupdate objects and initialize those
    // objects for virtual display during virtual display connect. This function
    // is no-op for V4L2 design
    virtual void init(hwc_context_t *) {};
    // Destroys mdpcomp, copybit and fbupdate objects and for virtual display
    // during virtual display disconnect. This function is no-op for V4L2 design
    virtual void destroy(hwc_context_t *, size_t ,
                       hwc_display_contents_1_t** ) {};
    virtual void pause(hwc_context_t* ctx, int dpy);
    virtual void resume(hwc_context_t* ctx, int dpy);
};

}; //namespace
#endif
