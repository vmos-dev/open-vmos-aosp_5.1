/*
 * Copyright (C) 2010 The Android Open Source Project
 * Copyright (C) 2012, The Linux Foundation. All rights reserved.
 *
 * Not a Contribution, Apache license notifications and license are
 * retained for attribution purposes only.
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
#ifndef HWC_FBUPDATE_H
#define HWC_FBUPDATE_H
#include "hwc_utils.h"
#include "overlay.h"

#define LIKELY( exp )       (__builtin_expect( (exp) != 0, true  ))
#define UNLIKELY( exp )     (__builtin_expect( (exp) != 0, false ))

namespace qhwc {
namespace ovutils = overlay::utils;

//Framebuffer update Interface
class IFBUpdate {
public:
    explicit IFBUpdate(const int& dpy) : mDpy(dpy) {}
    virtual ~IFBUpdate() {};
    // Sets up members and prepares overlay if conditions are met
    virtual bool prepare(hwc_context_t *ctx, hwc_display_contents_1 *list,
                                                       int fbZorder) = 0;
    // Draws layer
    virtual bool draw(hwc_context_t *ctx, private_handle_t *hnd) = 0;
    //Reset values
    virtual void reset();
    //Factory method that returns a low-res or high-res version
    static IFBUpdate *getObject(const int& width, const int& dpy);

protected:
    const int mDpy; // display to update
    bool mModeOn; // if prepare happened
};

//Low resolution (<= 2048) panel handler.
class FBUpdateLowRes : public IFBUpdate {
public:
    explicit FBUpdateLowRes(const int& dpy);
    virtual ~FBUpdateLowRes() {};
    bool prepare(hwc_context_t *ctx, hwc_display_contents_1 *list,
                                                          int fbZorder);
    bool draw(hwc_context_t *ctx, private_handle_t *hnd);
    void reset();
private:
    bool configure(hwc_context_t *ctx, hwc_display_contents_1 *list,
                                                               int fbZorder);
    ovutils::eDest mDest; //pipe to draw on
};

//High resolution (> 2048) panel handler.
class FBUpdateHighRes : public IFBUpdate {
public:
    explicit FBUpdateHighRes(const int& dpy);
    virtual ~FBUpdateHighRes() {};
    bool prepare(hwc_context_t *ctx, hwc_display_contents_1 *list,
                                                             int fbZorder);
    bool draw(hwc_context_t *ctx, private_handle_t *hnd);
    void reset();
private:
    bool configure(hwc_context_t *ctx, hwc_display_contents_1 *list,
                                                            int fbZorder);
    ovutils::eDest mDestLeft; //left pipe to draw on
    ovutils::eDest mDestRight; //right pipe to draw on
};

}; //namespace qhwc

#endif //HWC_FBUPDATE_H
