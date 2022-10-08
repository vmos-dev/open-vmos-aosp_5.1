/*
 * Copyright (C) 2010 The Android Open Source Project
 * Copyright (C)2012-2014, The Linux Foundation. All rights reserved.
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

#ifndef HWC_UTILS_H
#define HWC_UTILS_H

#define HWC_REMOVE_DEPRECATED_VERSIONS 1
#include <fcntl.h>
#include <math.h>
#include <hardware/hwcomposer.h>
#include <gr.h>
#include <gralloc_priv.h>
#include <utils/String8.h>
#include "qdMetaData.h"
#include <overlayUtils.h>
#include <EGL/egl.h>


#define ALIGN_TO(x, align)     (((x) + ((align)-1)) & ~((align)-1))
#define LIKELY( exp )       (__builtin_expect( (exp) != 0, true  ))
#define UNLIKELY( exp )     (__builtin_expect( (exp) != 0, false ))
#define MAX_NUM_APP_LAYERS 32
#define MIN_DISPLAY_XRES 200
#define MIN_DISPLAY_YRES 200
#define HWC_WFDDISPSYNC_LOG 0
#define STR(f) #f;
// Max number of PTOR layers handled
#define MAX_PTOR_LAYERS 2

//Fwrd decls
struct hwc_context_t;

namespace ovutils = overlay::utils;

namespace overlay {
class Overlay;
class Rotator;
class RotMgr;
}

namespace qhwc {
//fwrd decl
class QueuedBufferStore;
class ExternalDisplay;
class VirtualDisplay;
class IFBUpdate;
class IVideoOverlay;
class MDPComp;
class CopyBit;
class HwcDebug;
class AssertiveDisplay;
class HWCVirtualBase;


struct MDPInfo {
    int version;
    char panel;
    bool hasOverlay;
};

struct DisplayAttributes {
    uint32_t vsync_period; //nanos
    uint32_t xres;
    uint32_t yres;
    uint32_t stride;
    float xdpi;
    float ydpi;
    int fd;
    bool connected; //Applies only to pluggable disp.
    //Connected does not mean it ready to use.
    //It should be active also. (UNBLANKED)
    bool isActive;
    // In pause state, composition is bypassed
    // used for WFD displays only
    bool isPause;
    // To trigger padding round to clean up mdp
    // pipes
    bool isConfiguring;
    // External Display is in MDP Downscale mode indicator
    bool mDownScaleMode;
    // Ext dst Rect
    hwc_rect_t mDstRect;
    //Action safe attributes
    // Flag to indicate the presence of action safe dimensions for external
    bool mActionSafePresent;
    int mAsWidthRatio;
    int mAsHeightRatio;

    //If property fbsize set via adb shell debug.hwc.fbsize = XRESxYRES
    //following fields are used.
    bool customFBSize;
    uint32_t xres_new;
    uint32_t yres_new;

};

struct ListStats {
    int numAppLayers; //Total - 1, excluding FB layer.
    int skipCount;
    int fbLayerIndex; //Always last for now. = numAppLayers
    //Video specific
    int yuvCount;
    int yuvIndices[MAX_NUM_APP_LAYERS];
    int extOnlyLayerIndex;
    bool preMultipliedAlpha;
    int yuv4k2kIndices[MAX_NUM_APP_LAYERS];
    int yuv4k2kCount;
    // Notifies hwcomposer about the start and end of animation
    // This will be set to true during animation, otherwise false.
    bool isDisplayAnimating;
    bool secureUI; // Secure display layer
    bool isSecurePresent;
    hwc_rect_t lRoi;  //left ROI
    hwc_rect_t rRoi;  //right ROI. Unused in single DSI panels.
    //App Buffer Composition index
    int  renderBufIndexforABC;
};

//PTOR Comp info
struct PtorInfo {
    int count;
    int layerIndex[MAX_PTOR_LAYERS];
    int mRenderBuffOffset[MAX_PTOR_LAYERS];
    hwc_rect_t displayFrame[MAX_PTOR_LAYERS];
    bool isActive() { return (count>0); }
    int getPTORArrayIndex(int index) {
        int idx = -1;
        for(int i = 0; i < count; i++) {
            if(index == layerIndex[i])
                idx = i;
        }
        return idx;
    }
};

struct LayerProp {
    uint32_t mFlags; //qcom specific layer flags
    LayerProp():mFlags(0){};
};

struct VsyncState {
    bool enable;
    bool fakevsync;
    bool debug;
};

struct BwcPM {
    static void setBwc(const hwc_rect_t& crop,
            const hwc_rect_t& dst, const int& transform,
            ovutils::eMdpFlags& mdpFlags);
};

// LayerProp::flag values
enum {
    HWC_MDPCOMP = 0x00000001,
    HWC_COPYBIT = 0x00000002,
};

// HAL specific features
enum {
    HWC_COLOR_FILL = 0x00000008,
    HWC_FORMAT_RB_SWAP = 0x00000040,
};

/* External Display states */
enum {
    EXTERNAL_OFFLINE = 0,
    EXTERNAL_ONLINE,
    EXTERNAL_PAUSE,
    EXTERNAL_RESUME,
    EXTERNAL_MAXSTATES
};

class LayerRotMap {
public:
    LayerRotMap() { reset(); }
    enum { MAX_SESS = 3 };
    void add(hwc_layer_1_t* layer, overlay::Rotator *rot);
    //Resets the mapping of layer to rotator
    void reset();
    //Clears mappings and existing rotator fences
    //Intended to be used during errors
    void clear();
    uint32_t getCount() const;
    hwc_layer_1_t* getLayer(uint32_t index) const;
    overlay::Rotator* getRot(uint32_t index) const;
    void setReleaseFd(const int& fence);
private:
    hwc_layer_1_t* mLayer[MAX_SESS];
    overlay::Rotator* mRot[MAX_SESS];
    uint32_t mCount;
};

inline uint32_t LayerRotMap::getCount() const {
    return mCount;
}

inline hwc_layer_1_t* LayerRotMap::getLayer(uint32_t index) const {
    if(index >= mCount) return NULL;
    return mLayer[index];
}

inline overlay::Rotator* LayerRotMap::getRot(uint32_t index) const {
    if(index >= mCount) return NULL;
    return mRot[index];
}

inline hwc_rect_t integerizeSourceCrop(const hwc_frect_t& cropF) {
    hwc_rect_t cropI = {0,0,0,0};
    cropI.left = int(ceilf(cropF.left));
    cropI.top = int(ceilf(cropF.top));
    cropI.right = int(floorf(cropF.right));
    cropI.bottom = int(floorf(cropF.bottom));
    return cropI;
}

inline bool isNonIntegralSourceCrop(const hwc_frect_t& cropF) {
    if(cropF.left - roundf(cropF.left)     ||
       cropF.top - roundf(cropF.top)       ||
       cropF.right - roundf(cropF.right)   ||
       cropF.bottom - roundf(cropF.bottom))
        return true;
    else
        return false;
}

// -----------------------------------------------------------------------------
// Utility functions - implemented in hwc_utils.cpp
void dumpLayer(hwc_layer_1_t const* l);
void setListStats(hwc_context_t *ctx, hwc_display_contents_1_t *list,
        int dpy);
void initContext(hwc_context_t *ctx);
void closeContext(hwc_context_t *ctx);
//Crops source buffer against destination and FB boundaries
void calculate_crop_rects(hwc_rect_t& crop, hwc_rect_t& dst,
                         const hwc_rect_t& scissor, int orient);
void getNonWormholeRegion(hwc_display_contents_1_t* list,
                              hwc_rect_t& nwr);
bool isSecuring(hwc_context_t* ctx, hwc_layer_1_t const* layer);
bool isSecureModePolicy(int mdpVersion);
// Returns true, if the input layer format is supported by rotator
bool isRotatorSupportedFormat(private_handle_t *hnd);
//Returns true, if the layer is YUV or the layer has been rendered by CPU
bool isRotationDoable(hwc_context_t *ctx, private_handle_t *hnd);
bool isExternalActive(hwc_context_t* ctx);
bool isAlphaScaled(hwc_layer_1_t const* layer);
bool needsScaling(hwc_layer_1_t const* layer);
bool isDownscaleRequired(hwc_layer_1_t const* layer);
bool needsScalingWithSplit(hwc_context_t* ctx, hwc_layer_1_t const* layer,
                           const int& dpy);
void sanitizeSourceCrop(hwc_rect_t& cropL, hwc_rect_t& cropR,
                        private_handle_t *hnd);
bool isAlphaPresent(hwc_layer_1_t const* layer);
int hwc_vsync_control(hwc_context_t* ctx, int dpy, int enable);
int getBlending(int blending);
bool isGLESOnlyComp(hwc_context_t *ctx, const int& dpy);
void reset_layer_prop(hwc_context_t* ctx, int dpy, int numAppLayers);
bool isAbcInUse(hwc_context_t *ctx);

void dumpBuffer(private_handle_t *ohnd, char *bufferName);

//Helper function to dump logs
void dumpsys_log(android::String8& buf, const char* fmt, ...);

int getExtOrientation(hwc_context_t* ctx);
bool isValidRect(const hwc_rect_t& rect);
hwc_rect_t deductRect(const hwc_rect_t& rect1, const hwc_rect_t& rect2);
bool isSameRect(const hwc_rect& rect1, const hwc_rect& rect2);
hwc_rect_t moveRect(const hwc_rect_t& rect, const int& x_off, const int& y_off);
hwc_rect_t getIntersection(const hwc_rect_t& rect1, const hwc_rect_t& rect2);
hwc_rect_t getUnion(const hwc_rect_t& rect1, const hwc_rect_t& rect2);
void optimizeLayerRects(const hwc_display_contents_1_t *list);
bool areLayersIntersecting(const hwc_layer_1_t* layer1,
        const hwc_layer_1_t* layer2);
bool operator ==(const hwc_rect_t& lhs, const hwc_rect_t& rhs);

// returns true if Action safe dimensions are set and target supports Actionsafe
bool isActionSafePresent(hwc_context_t *ctx, int dpy);

/* Calculates the destination position based on the action safe rectangle */
void getActionSafePosition(hwc_context_t *ctx, int dpy, hwc_rect_t& dst);

void getAspectRatioPosition(hwc_context_t* ctx, int dpy, int extOrientation,
                                hwc_rect_t& inRect, hwc_rect_t& outRect);

bool isPrimaryPortrait(hwc_context_t *ctx);

bool isOrientationPortrait(hwc_context_t *ctx);

void calcExtDisplayPosition(hwc_context_t *ctx,
                               private_handle_t *hnd,
                               int dpy,
                               hwc_rect_t& sourceCrop,
                               hwc_rect_t& displayFrame,
                               int& transform,
                               ovutils::eTransform& orient);

// Returns the orientation that needs to be set on external for
// BufferMirrirMode(Sidesync)
int getMirrorModeOrientation(hwc_context_t *ctx);

/* Get External State names */
const char* getExternalDisplayState(uint32_t external_state);

// Resets display ROI to full panel resoluion
void resetROI(hwc_context_t *ctx, const int dpy);

// Aligns updating ROI to panel restrictions
hwc_rect_t getSanitizeROI(struct hwc_rect roi, hwc_rect boundary);

// Handles wfd Pause and resume events
void handle_pause(hwc_context_t *ctx, int dpy);
void handle_resume(hwc_context_t *ctx, int dpy);

//Close acquireFenceFds of all layers of incoming list
void closeAcquireFds(hwc_display_contents_1_t* list);

//Sync point impl.
int hwc_sync(hwc_context_t *ctx, hwc_display_contents_1_t* list, int dpy,
        int fd);

//Sets appropriate mdp flags for a layer.
void setMdpFlags(hwc_context_t *ctx, hwc_layer_1_t *layer,
        ovutils::eMdpFlags &mdpFlags,
        int rotDownscale, int transform);

int configRotator(overlay::Rotator *rot, ovutils::Whf& whf,
        hwc_rect_t& crop, const ovutils::eMdpFlags& mdpFlags,
        const ovutils::eTransform& orient, const int& downscale);

int configMdp(overlay::Overlay *ov, const ovutils::PipeArgs& parg,
        const ovutils::eTransform& orient, const hwc_rect_t& crop,
        const hwc_rect_t& pos, const MetaData_t *metadata,
        const ovutils::eDest& dest);

int configColorLayer(hwc_context_t *ctx, hwc_layer_1_t *layer, const int& dpy,
        ovutils::eMdpFlags& mdpFlags, ovutils::eZorder& z,
        ovutils::eIsFg& isFg, const ovutils::eDest& dest);

void updateSource(ovutils::eTransform& orient, ovutils::Whf& whf,
        hwc_rect_t& crop, overlay::Rotator *rot);

//Routine to configure low resolution panels (<= 2048 width)
int configureNonSplit(hwc_context_t *ctx, hwc_layer_1_t *layer, const int& dpy,
        ovutils::eMdpFlags& mdpFlags, ovutils::eZorder& z,
        ovutils::eIsFg& isFg, const ovutils::eDest& dest,
        overlay::Rotator **rot);

//Routine to configure high resolution panels (> 2048 width)
int configureSplit(hwc_context_t *ctx, hwc_layer_1_t *layer, const int& dpy,
        ovutils::eMdpFlags& mdpFlags, ovutils::eZorder& z,
        ovutils::eIsFg& isFg, const ovutils::eDest& lDest,
        const ovutils::eDest& rDest, overlay::Rotator **rot);

//Routine to split and configure high resolution YUV layer (> 2048 width)
int configureSourceSplit(hwc_context_t *ctx, hwc_layer_1_t *layer,
        const int& dpy,
        ovutils::eMdpFlags& mdpFlags, ovutils::eZorder& z,
        ovutils::eIsFg& isFg, const ovutils::eDest& lDest,
        const ovutils::eDest& rDest, overlay::Rotator **rot);

//On certain targets DMA pipes are used for rotation and they won't be available
//for line operations. On a per-target basis we can restrict certain use cases
//from using rotator, since we know before-hand that such scenarios can lead to
//extreme unavailability of pipes. This can also be done via hybrid calculations
//also involving many more variables like number of write-back interfaces etc,
//but the variety of scenarios is too high to warrant that.
bool canUseRotator(hwc_context_t *ctx, int dpy);

int getLeftSplit(hwc_context_t *ctx, const int& dpy);

bool isDisplaySplit(hwc_context_t* ctx, int dpy);

// Set the GPU hint flag to high for MIXED/GPU composition only for
// first frame after MDP to GPU/MIXED mode transition.
// Set the GPU hint to default if the current composition type is GPU
// due to idle fallback or MDP composition.
void setGPUHint(hwc_context_t* ctx, hwc_display_contents_1_t* list);

// Returns true if rect1 is peripheral to rect2, false otherwise.
bool isPeripheral(const hwc_rect_t& rect1, const hwc_rect_t& rect2);

// Inline utility functions
static inline bool isSkipLayer(const hwc_layer_1_t* l) {
    return (UNLIKELY(l && (l->flags & HWC_SKIP_LAYER)));
}

// Returns true if the buffer is yuv
static inline bool isYuvBuffer(const private_handle_t* hnd) {
    return (hnd && (hnd->bufferType == BUFFER_TYPE_VIDEO));
}

// Returns true if the buffer is yuv
static inline bool is4kx2kYuvBuffer(const private_handle_t* hnd) {
    return (hnd && (hnd->bufferType == BUFFER_TYPE_VIDEO) &&
            (hnd->width > 2048));
}

// Returns true if the buffer is secure
static inline bool isSecureBuffer(const private_handle_t* hnd) {
    return (hnd && (private_handle_t::PRIV_FLAGS_SECURE_BUFFER & hnd->flags));
}

static inline bool isTileRendered(const private_handle_t* hnd) {
    return (hnd && (private_handle_t::PRIV_FLAGS_TILE_RENDERED & hnd->flags));
}

static inline bool isCPURendered(const private_handle_t* hnd) {
    return (hnd && (private_handle_t::PRIV_FLAGS_CPU_RENDERED & hnd->flags));
}

//Return true if buffer is marked locked
static inline bool isBufferLocked(const private_handle_t* hnd) {
    return (hnd && (private_handle_t::PRIV_FLAGS_HWC_LOCK & hnd->flags));
}

//Return true if buffer is for external display only
static inline bool isExtOnly(const private_handle_t* hnd) {
    return (hnd && (hnd->flags & private_handle_t::PRIV_FLAGS_EXTERNAL_ONLY));
}

//Return true if the buffer is intended for Secure Display
static inline bool isSecureDisplayBuffer(const private_handle_t* hnd) {
    return (hnd && (hnd->flags & private_handle_t::PRIV_FLAGS_SECURE_DISPLAY));
}

static inline int getWidth(const private_handle_t* hnd) {
    if(isYuvBuffer(hnd)) {
        MetaData_t *metadata = reinterpret_cast<MetaData_t*>(hnd->base_metadata);
        if(metadata && metadata->operation & UPDATE_BUFFER_GEOMETRY) {
            return metadata->bufferDim.sliceWidth;
        }
    }
    return hnd->width;
}

static inline int getHeight(const private_handle_t* hnd) {
    if(isYuvBuffer(hnd)) {
        MetaData_t *metadata = reinterpret_cast<MetaData_t*>(hnd->base_metadata);
        if(metadata && metadata->operation & UPDATE_BUFFER_GEOMETRY) {
            return metadata->bufferDim.sliceHeight;
        }
    }
    return hnd->height;
}

template<typename T> inline T max(T a, T b) { return (a > b) ? a : b; }
template<typename T> inline T min(T a, T b) { return (a < b) ? a : b; }

// Initialize uevent thread
void init_uevent_thread(hwc_context_t* ctx);
// Initialize vsync thread
void init_vsync_thread(hwc_context_t* ctx);

inline void getLayerResolution(const hwc_layer_1_t* layer,
                               int& width, int& height) {
    hwc_rect_t displayFrame  = layer->displayFrame;
    width = displayFrame.right - displayFrame.left;
    height = displayFrame.bottom - displayFrame.top;
}

static inline int openFb(int dpy) {
    int fd = -1;
    const char *devtmpl = "/dev/graphics/fb%u";
    char name[64] = {0};
    snprintf(name, 64, devtmpl, dpy);
    fd = open(name, O_RDWR);
    return fd;
}

template <class T>
inline void swap(T& a, T& b) {
    T tmp = a;
    a = b;
    b = tmp;
}

}; //qhwc namespace

enum eAnimationState{
    ANIMATION_STOPPED,
    ANIMATION_STARTED,
};

enum eCompositionState {
    COMPOSITION_STATE_MDP = 0,        // Set if composition type is MDP
    COMPOSITION_STATE_GPU,            // Set if composition type is GPU or MIXED
    COMPOSITION_STATE_IDLE_FALLBACK,  // Set if it is idlefallback
};

// Structure holds the information about the GPU hint.
struct gpu_hint_info {
    // system level flag to enable gpu_perf_mode
    bool mGpuPerfModeEnable;
    // Stores the current GPU performance mode DEFAULT/HIGH
    bool mCurrGPUPerfMode;
    // Stores the compositon state GPU, MDP or IDLE_FALLBACK
    bool mCompositionState;
    // Stores the EGLContext of current process
    EGLContext mEGLContext;
    // Stores the EGLDisplay of current process
    EGLDisplay mEGLDisplay;
};

// -----------------------------------------------------------------------------
// HWC context
// This structure contains overall state
struct hwc_context_t {
    hwc_composer_device_1_t device;
    const hwc_procs_t* proc;

    //CopyBit objects
    qhwc::CopyBit *mCopyBit[HWC_NUM_DISPLAY_TYPES];

    //Overlay object - NULL for non overlay devices
    overlay::Overlay *mOverlay;
    //Holds a few rot objects
    overlay::RotMgr *mRotMgr;

    //Primary and external FB updater
    qhwc::IFBUpdate *mFBUpdate[HWC_NUM_DISPLAY_TYPES];
    // External display related information
    qhwc::ExternalDisplay *mExtDisplay;
    qhwc::VirtualDisplay *mVirtualDisplay;
    qhwc::MDPInfo mMDP;
    qhwc::VsyncState vstate;
    qhwc::DisplayAttributes dpyAttr[HWC_NUM_DISPLAY_TYPES];
    qhwc::ListStats listStats[HWC_NUM_DISPLAY_TYPES];
    qhwc::LayerProp *layerProp[HWC_NUM_DISPLAY_TYPES];
    qhwc::MDPComp *mMDPComp[HWC_NUM_DISPLAY_TYPES];
    qhwc::HwcDebug *mHwcDebug[HWC_NUM_DISPLAY_TYPES];
    hwc_rect_t mViewFrame[HWC_NUM_DISPLAY_TYPES];
    qhwc::AssertiveDisplay *mAD;
    eAnimationState mAnimationState[HWC_NUM_DISPLAY_TYPES];
    qhwc::HWCVirtualBase *mHWCVirtual;

    // stores the #numHwLayers of the previous frame
    // for each display device
    int mPrevHwLayerCount[HWC_NUM_DISPLAY_TYPES];

    // stores the primary device orientation
    int deviceOrientation;
    //Securing in progress indicator
    bool mSecuring;
    //WFD on proprietary stack
    bool mVirtualonExtActive;
    //Display in secure mode indicator
    bool mSecureMode;
    //Lock to protect drawing data structures
    mutable Locker mDrawLock;
    //Drawing round when we use GPU
    bool isPaddingRound;
    // External Orientation
    int mExtOrientation;
    //Flags the transition of a video session
    bool mVideoTransFlag;
    //Used for SideSync feature
    //which overrides the mExtOrientation
    bool mBufferMirrorMode;
    // Used to synchronize between WFD and Display modules
    mutable Locker mWfdSyncLock;

    qhwc::LayerRotMap *mLayerRotMap[HWC_NUM_DISPLAY_TYPES];
    // Panel reset flag will be set if BTA check fails
    bool mPanelResetStatus;
    // number of active Displays
    int numActiveDisplays;
    // Downscale feature switch, set via system property
    // sys.hwc.mdp_downscale_enabled
    bool mMDPDownscaleEnabled;
    // Is WFD enabled through VDS solution ?
    // This can be set via system property
    // persist.hwc.enable_vds
    bool mVDSEnabled;
    struct gpu_hint_info mGPUHintInfo;
    //App Buffer Composition
    bool enableABC;
    // PTOR Info
    qhwc::PtorInfo mPtorInfo;
};

namespace qhwc {
static inline bool isSkipPresent (hwc_context_t *ctx, int dpy) {
    return  ctx->listStats[dpy].skipCount;
}

static inline bool isYuvPresent (hwc_context_t *ctx, int dpy) {
    return  ctx->listStats[dpy].yuvCount;
}

static inline bool has90Transform(hwc_layer_1_t const* layer) {
    return ((layer->transform & HWC_TRANSFORM_ROT_90) &&
            !(layer->flags & HWC_COLOR_FILL));
}

inline bool isSecurePresent(hwc_context_t *ctx, int dpy) {
    return ctx->listStats[dpy].isSecurePresent;
}

static inline bool isSecondaryConfiguring(hwc_context_t* ctx) {
    return (ctx->dpyAttr[HWC_DISPLAY_EXTERNAL].isConfiguring ||
            ctx->dpyAttr[HWC_DISPLAY_VIRTUAL].isConfiguring);
}

static inline bool isSecondaryConnected(hwc_context_t* ctx) {
    return (ctx->dpyAttr[HWC_DISPLAY_EXTERNAL].connected ||
            ctx->dpyAttr[HWC_DISPLAY_VIRTUAL].connected);
}

};

#endif //HWC_UTILS_H
