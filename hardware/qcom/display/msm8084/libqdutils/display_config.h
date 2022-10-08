/*
 * Copyright (c) 2013 The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation. nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <gralloc_priv.h>
#include <qdMetaData.h>
#include <mdp_version.h>
#include <hardware/hwcomposer.h>

// This header is for clients to use to set/get global display configuration
// The functions in this header run in the client process and wherever necessary
// do a binder call to HWC to get/set data.
// Only primary and external displays are supported here.
// WiFi/virtual displays are not supported.

namespace qdutils {

// Use this enum to specify the dpy parameters where needed
enum {
    DISPLAY_PRIMARY = 0,
    DISPLAY_EXTERNAL,
};

// Display Attributes that are available to clients of this library
// Not to be confused with a similar struct in hwc_utils (in the hwc namespace)
struct DisplayAttributes_t {
    uint32_t vsync_period; //nanoseconds
    uint32_t xres;
    uint32_t yres;
    float xdpi;
    float ydpi;
    char panel_type;
};

// Check if external display is connected. Useful to check before making
// calls for external displays
// Returns 1 if connected, 0 if disconnected, negative values on errors
int isExternalConnected(void);

// Get display vsync period which is in nanoseconds
// i.e vsync_period = 1000000000l / fps
// Returns 0 on success, negative values on errors
int getDisplayAttributes(int dpy, DisplayAttributes_t& dpyattr);

// Set HSIC data on a given display ID
// Returns 0 on success, negative values on errors
int setHSIC(int dpy, const HSICData_t& hsic_data);

// get the active visible region for the display
// Returns 0 on success, negative values on errors
int getDisplayVisibleRegion(int dpy, hwc_rect_t &rect);
}; //namespace
