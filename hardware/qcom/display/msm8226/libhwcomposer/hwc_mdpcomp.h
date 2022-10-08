/*
 * Copyright (C) 2012-2013, The Linux Foundation. All rights reserved.
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

#ifndef HWC_MDP_COMP
#define HWC_MDP_COMP

#include <hwc_utils.h>
#include <idle_invalidator.h>
#include <cutils/properties.h>
#include <overlay.h>

#define DEFAULT_IDLE_TIME 70
#define MAX_PIPES_PER_MIXER 4

namespace overlay {
class Rotator;
};

namespace qhwc {
namespace ovutils = overlay::utils;

class MDPComp {
public:
    explicit MDPComp(int);
    virtual ~MDPComp(){};
    /*sets up mdp comp for the current frame */
    int prepare(hwc_context_t *ctx, hwc_display_contents_1_t* list);
    /* draw */
    virtual bool draw(hwc_context_t *ctx, hwc_display_contents_1_t *list) = 0;
    //Reset values
    void reset();
    /* dumpsys */
    void dump(android::String8& buf, hwc_context_t *ctx);
    bool isGLESOnlyComp() { return (mCurrentFrame.mdpCount == 0); }
    int drawOverlap(hwc_context_t *ctx, hwc_display_contents_1_t* list);
    static MDPComp* getObject(hwc_context_t *ctx, const int& dpy);
    /* Handler to invoke frame redraw on Idle Timer expiry */
    static void timeout_handler(void *udata);
    /* Initialize MDP comp*/
    static bool init(hwc_context_t *ctx);
    static void resetIdleFallBack() { sIdleFallBack = false; }
    static bool isIdleFallback() { return sIdleFallBack; }
    static void dynamicDebug(bool enable){ sDebugLogs = enable; }

protected:
    enum { MAX_SEC_LAYERS = 1 }; //TODO add property support

    enum ePipeType {
        MDPCOMP_OV_RGB = ovutils::OV_MDP_PIPE_RGB,
        MDPCOMP_OV_VG = ovutils::OV_MDP_PIPE_VG,
        MDPCOMP_OV_DMA = ovutils::OV_MDP_PIPE_DMA,
        MDPCOMP_OV_ANY,
    };

    //Simulation flags
    enum {
        MDPCOMP_AVOID_FULL_MDP = 0x001,
        MDPCOMP_AVOID_CACHE_MDP = 0x002,
        MDPCOMP_AVOID_LOAD_MDP = 0x004,
        MDPCOMP_AVOID_VIDEO_ONLY = 0x008,
    };

    /* mdp pipe data */
    struct MdpPipeInfo {
        int zOrder;
        virtual ~MdpPipeInfo(){};
    };

    struct MdpYUVPipeInfo : public MdpPipeInfo{
        ovutils::eDest lIndex;
        ovutils::eDest rIndex;
        virtual ~MdpYUVPipeInfo(){};
    };

    /* per layer data */
    struct PipeLayerPair {
        MdpPipeInfo *pipeInfo;
        overlay::Rotator* rot;
        int listIndex;
    };

    /* per frame data */
    struct FrameInfo {
        /* maps layer list to mdp list */
        int layerCount;
        int layerToMDP[MAX_NUM_APP_LAYERS];

        /* maps mdp list to layer list */
        int mdpCount;
        struct PipeLayerPair mdpToLayer[MAX_PIPES_PER_MIXER];

        /* layer composing on FB? */
        int fbCount;
        bool isFBComposed[MAX_NUM_APP_LAYERS];
        /* layers lying outside ROI. Will
         * be dropped off from the composition */
        int dropCount;
        bool drop[MAX_NUM_APP_LAYERS];

        bool needsRedraw;
        int fbZ;

        /* c'tor */
        FrameInfo();
        /* clear old frame data */
        void reset(const int& numLayers);
        void map();
    };

    /* cached data */
    struct LayerCache {
        int layerCount;
        buffer_handle_t hnd[MAX_NUM_APP_LAYERS];
        bool isFBComposed[MAX_NUM_APP_LAYERS];
        bool drop[MAX_NUM_APP_LAYERS];

        /* c'tor */
        LayerCache();
        /* clear caching info*/
        void reset();
        void cacheAll(hwc_display_contents_1_t* list);
        void updateCounts(const FrameInfo&);
        bool isSameFrame(const FrameInfo& curFrame,
                         hwc_display_contents_1_t* list);
    };

    /* allocates pipe from pipe book */
    virtual bool allocLayerPipes(hwc_context_t *ctx,
                                 hwc_display_contents_1_t* list) = 0;
    /* configures MPD pipes */
    virtual int configure(hwc_context_t *ctx, hwc_layer_1_t *layer,
                          PipeLayerPair& pipeLayerPair) = 0;
    /* Increments mdpCount if 4k2k yuv layer split is enabled.
     * updates framebuffer z order if fb lies above source-split layer */
    virtual void adjustForSourceSplit(hwc_context_t *ctx,
            hwc_display_contents_1_t* list) = 0;
    /* configures 4kx2k yuv layer*/
    virtual int configure4k2kYuv(hwc_context_t *ctx, hwc_layer_1_t *layer,
            PipeLayerPair& PipeLayerPair) = 0;
    /* generates ROI based on the modified area of the frame */
    virtual void generateROI(hwc_context_t *ctx,
            hwc_display_contents_1_t* list) = 0;
    /* validates the ROI generated for fallback conditions */
    virtual bool validateAndApplyROI(hwc_context_t *ctx,
            hwc_display_contents_1_t* list) = 0;
    /* Trims fbRect calculated against ROI generated */
    virtual void trimAgainstROI(hwc_context_t *ctx, hwc_rect_t& fbRect) = 0;

    /* set/reset flags for MDPComp */
    void setMDPCompLayerFlags(hwc_context_t *ctx,
                              hwc_display_contents_1_t* list);
    void setRedraw(hwc_context_t *ctx,
            hwc_display_contents_1_t* list);
    /* checks for conditions where mdpcomp is not possible */
    bool isFrameDoable(hwc_context_t *ctx);
    /* checks for conditions where RGB layers cannot be bypassed */
    bool tryFullFrame(hwc_context_t *ctx, hwc_display_contents_1_t* list);
    /* checks if full MDP comp can be done */
    bool fullMDPComp(hwc_context_t *ctx, hwc_display_contents_1_t* list);
    /* Full MDP Composition with Peripheral Tiny Overlap Removal */
    bool fullMDPCompWithPTOR(hwc_context_t *ctx,hwc_display_contents_1_t* list);
    /* check if we can use layer cache to do at least partial MDP comp */
    bool partialMDPComp(hwc_context_t *ctx, hwc_display_contents_1_t* list);
    /* Partial MDP comp that uses caching to save power as primary goal */
    bool cacheBasedComp(hwc_context_t *ctx, hwc_display_contents_1_t* list);
    /* Partial MDP comp that balances the load between MDP and GPU such that
     * MDP is loaded to the max of its capacity. The lower z order layers are
     * fed to MDP, whereas the upper ones to GPU, because the upper ones have
     * lower number of pixels and can reduce GPU processing time */
    bool loadBasedComp(hwc_context_t *ctx, hwc_display_contents_1_t* list);
    /* Checks if its worth doing load based partial comp */
    bool isLoadBasedCompDoable(hwc_context_t *ctx);
    /* checks for conditions where only video can be bypassed */
    bool tryVideoOnly(hwc_context_t *ctx, hwc_display_contents_1_t* list);
    bool videoOnlyComp(hwc_context_t *ctx, hwc_display_contents_1_t* list,
            bool secureOnly);
    /* checks for conditions where YUV layers cannot be bypassed */
    bool isYUVDoable(hwc_context_t* ctx, hwc_layer_1_t* layer);
    /* checks if MDP/MDSS can process current list w.r.to HW limitations
     * All peculiar HW limitations should go here */
    bool hwLimitationsCheck(hwc_context_t* ctx, hwc_display_contents_1_t* list);
    /* Is debug enabled */
    static bool isDebug() { return sDebugLogs ? true : false; };
    /* Is feature enabled */
    static bool isEnabled() { return sEnabled; };
    /* checks for mdp comp dimension limitation */
    bool isValidDimension(hwc_context_t *ctx, hwc_layer_1_t *layer);
    /* tracks non updating layers*/
    void updateLayerCache(hwc_context_t* ctx, hwc_display_contents_1_t* list);
    /* optimize layers for mdp comp*/
    bool markLayersForCaching(hwc_context_t* ctx,
            hwc_display_contents_1_t* list);
    int getBatch(hwc_display_contents_1_t* list,
            int& maxBatchStart, int& maxBatchEnd,
            int& maxBatchCount);
    bool canPushBatchToTop(const hwc_display_contents_1_t* list,
            int fromIndex, int toIndex);
    bool intersectingUpdatingLayers(const hwc_display_contents_1_t* list,
            int fromIndex, int toIndex, int targetLayerIndex);

        /* updates cache map with YUV info */
    void updateYUV(hwc_context_t* ctx, hwc_display_contents_1_t* list,
            bool secureOnly);
    /* Validates if the GPU/MDP layer split chosen by a strategy is supported
     * by MDP.
     * Sets up MDP comp data structures to reflect covnversion from layers to
     * overlay pipes.
     * Configures overlay.
     * Configures if GPU should redraw.
     */
    bool postHeuristicsHandling(hwc_context_t *ctx,
            hwc_display_contents_1_t* list);
    void reset(hwc_context_t *ctx);
    bool isSupportedForMDPComp(hwc_context_t *ctx, hwc_layer_1_t* layer);
    bool resourceCheck(hwc_context_t* ctx, hwc_display_contents_1_t* list);
    hwc_rect_t getUpdatingFBRect(hwc_context_t *ctx,
            hwc_display_contents_1_t* list);
    /* checks for conditions to enable partial udpate */
    bool canPartialUpdate(hwc_context_t *ctx, hwc_display_contents_1_t* list);

    int mDpy;
    static bool sEnabled;
    static bool sEnableMixedMode;
    static int sSimulationFlags;
    static bool sDebugLogs;
    static bool sIdleFallBack;
    /* Handles the timeout event from kernel, if the value is set to true */
    static bool sHandleTimeout;
    static int sMaxPipesPerMixer;
    static bool sSrcSplitEnabled;
    static IdleInvalidator *idleInvalidator;
    struct FrameInfo mCurrentFrame;
    struct LayerCache mCachedFrame;
    //Enable 4kx2k yuv layer split
    static bool sEnable4k2kYUVSplit;
    bool mModeOn; // if prepare happened
    bool allocSplitVGPipesfor4k2k(hwc_context_t *ctx, int index);
};

class MDPCompNonSplit : public MDPComp {
public:
    explicit MDPCompNonSplit(int dpy):MDPComp(dpy){};
    virtual ~MDPCompNonSplit(){};
    virtual bool draw(hwc_context_t *ctx, hwc_display_contents_1_t *list);

private:
    struct MdpPipeInfoNonSplit : public MdpPipeInfo {
        ovutils::eDest index;
        virtual ~MdpPipeInfoNonSplit() {};
    };

    /* configure's overlay pipes for the frame */
    virtual int configure(hwc_context_t *ctx, hwc_layer_1_t *layer,
                          PipeLayerPair& pipeLayerPair);

    /* allocates pipes to selected candidates */
    virtual bool allocLayerPipes(hwc_context_t *ctx,
                                 hwc_display_contents_1_t* list);

    /* Increments mdpCount if 4k2k yuv layer split is enabled.
     * updates framebuffer z order if fb lies above source-split layer */
    virtual void adjustForSourceSplit(hwc_context_t *ctx,
            hwc_display_contents_1_t* list);

    /* configures 4kx2k yuv layer to 2 VG pipes*/
    virtual int configure4k2kYuv(hwc_context_t *ctx, hwc_layer_1_t *layer,
            PipeLayerPair& PipeLayerPair);
    /* generates ROI based on the modified area of the frame */
    virtual void generateROI(hwc_context_t *ctx,
            hwc_display_contents_1_t* list);
    /* validates the ROI generated for fallback conditions */
    virtual bool validateAndApplyROI(hwc_context_t *ctx,
            hwc_display_contents_1_t* list);
    /* Trims fbRect calculated against ROI generated */
    virtual void trimAgainstROI(hwc_context_t *ctx, hwc_rect_t& fbRect);
};

class MDPCompSplit : public MDPComp {
public:
    explicit MDPCompSplit(int dpy):MDPComp(dpy){};
    virtual ~MDPCompSplit(){};
    virtual bool draw(hwc_context_t *ctx, hwc_display_contents_1_t *list);

protected:
    struct MdpPipeInfoSplit : public MdpPipeInfo {
        ovutils::eDest lIndex;
        ovutils::eDest rIndex;
        virtual ~MdpPipeInfoSplit() {};
    };

    virtual bool acquireMDPPipes(hwc_context_t *ctx, hwc_layer_1_t* layer,
                         MdpPipeInfoSplit& pipe_info);

    /* configure's overlay pipes for the frame */
    virtual int configure(hwc_context_t *ctx, hwc_layer_1_t *layer,
                          PipeLayerPair& pipeLayerPair);

    /* allocates pipes to selected candidates */
    virtual bool allocLayerPipes(hwc_context_t *ctx,
                                 hwc_display_contents_1_t* list);
private:
    /* Increments mdpCount if 4k2k yuv layer split is enabled.
     * updates framebuffer z order if fb lies above source-split layer */
    virtual void adjustForSourceSplit(hwc_context_t *ctx,
            hwc_display_contents_1_t* list);

    /* configures 4kx2k yuv layer*/
    virtual int configure4k2kYuv(hwc_context_t *ctx, hwc_layer_1_t *layer,
            PipeLayerPair& PipeLayerPair);
    /* generates ROI based on the modified area of the frame */
    virtual void generateROI(hwc_context_t *ctx,
            hwc_display_contents_1_t* list);
    /* validates the ROI generated for fallback conditions */
    virtual bool validateAndApplyROI(hwc_context_t *ctx,
            hwc_display_contents_1_t* list);
    /* Trims fbRect calculated against ROI generated */
    virtual void trimAgainstROI(hwc_context_t *ctx, hwc_rect_t& fbRect);
};

class MDPCompSrcSplit : public MDPCompSplit {
public:
    explicit MDPCompSrcSplit(int dpy) : MDPCompSplit(dpy){};
    virtual ~MDPCompSrcSplit(){};
private:
    virtual bool acquireMDPPipes(hwc_context_t *ctx, hwc_layer_1_t* layer,
            MdpPipeInfoSplit& pipe_info);

    virtual int configure(hwc_context_t *ctx, hwc_layer_1_t *layer,
            PipeLayerPair& pipeLayerPair);
};

}; //namespace
#endif
