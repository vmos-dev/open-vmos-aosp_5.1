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

#define DEFAULT_IDLE_TIME 2000
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
    /* dumpsys */
    void dump(android::String8& buf);

    static MDPComp* getObject(const int& width, const int dpy);
    /* Handler to invoke frame redraw on Idle Timer expiry */
    static void timeout_handler(void *udata);
    /* Initialize MDP comp*/
    static bool init(hwc_context_t *ctx);
    static void resetIdleFallBack() { sIdleFallBack = false; }

protected:
    enum ePipeType {
        MDPCOMP_OV_RGB = ovutils::OV_MDP_PIPE_RGB,
        MDPCOMP_OV_VG = ovutils::OV_MDP_PIPE_VG,
        MDPCOMP_OV_DMA = ovutils::OV_MDP_PIPE_DMA,
        MDPCOMP_OV_ANY,
    };

    /* mdp pipe data */
    struct MdpPipeInfo {
        int zOrder;
        virtual ~MdpPipeInfo(){};
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
        int mdpCount;
        int cacheCount;
        int fbZ;
        buffer_handle_t hnd[MAX_NUM_APP_LAYERS];

        /* c'tor */
        LayerCache();
        /* clear caching info*/
        void reset();
        void cacheAll(hwc_display_contents_1_t* list);
        void updateCounts(const FrameInfo&);
    };

    /* No of pipes needed for Framebuffer */
    virtual int pipesForFB() = 0;
    /* calculates pipes needed for the panel */
    virtual int pipesNeeded(hwc_context_t *ctx,
                            hwc_display_contents_1_t* list) = 0;
    /* allocates pipe from pipe book */
    virtual bool allocLayerPipes(hwc_context_t *ctx,
                                 hwc_display_contents_1_t* list) = 0;
    /* configures MPD pipes */
    virtual int configure(hwc_context_t *ctx, hwc_layer_1_t *layer,
                          PipeLayerPair& pipeLayerPair) = 0;

    /* set/reset flags for MDPComp */
    void setMDPCompLayerFlags(hwc_context_t *ctx,
                              hwc_display_contents_1_t* list);
    /* allocate MDP pipes from overlay */
    ovutils::eDest getMdpPipe(hwc_context_t *ctx, ePipeType type);

    /* checks for conditions where mdpcomp is not possible */
    bool isFrameDoable(hwc_context_t *ctx);
    /* checks for conditions where RGB layers cannot be bypassed */
    bool isFullFrameDoable(hwc_context_t *ctx, hwc_display_contents_1_t* list);
    /* checks if full MDP comp can be done */
    bool fullMDPComp(hwc_context_t *ctx, hwc_display_contents_1_t* list);
    /* check if we can use layer cache to do at least partial MDP comp */
    bool partialMDPComp(hwc_context_t *ctx, hwc_display_contents_1_t* list);
    /* checks for conditions where only video can be bypassed */
    bool isOnlyVideoDoable(hwc_context_t *ctx, hwc_display_contents_1_t* list);
    /* checks for conditions where YUV layers cannot be bypassed */
    bool isYUVDoable(hwc_context_t* ctx, hwc_layer_1_t* layer);

    /* set up Border fill as Base pipe */
    static bool setupBasePipe(hwc_context_t*);
    /* Is debug enabled */
    static bool isDebug() { return sDebugLogs ? true : false; };
    /* Is feature enabled */
    static bool isEnabled() { return sEnabled; };
    /* checks for mdp comp dimension limitation */
    bool isValidDimension(hwc_context_t *ctx, hwc_layer_1_t *layer);
    /* tracks non updating layers*/
    void updateLayerCache(hwc_context_t* ctx, hwc_display_contents_1_t* list);
    /* optimize layers for mdp comp*/
    void batchLayers();
    /* gets available pipes for mdp comp */
    int getAvailablePipes(hwc_context_t* ctx);
    /* updates cache map with YUV info */
    void updateYUV(hwc_context_t* ctx, hwc_display_contents_1_t* list);
    bool programMDP(hwc_context_t *ctx, hwc_display_contents_1_t* list);
    bool programYUV(hwc_context_t *ctx, hwc_display_contents_1_t* list);

    int mDpy;
    static bool sEnabled;
    static bool sDebugLogs;
    static bool sIdleFallBack;
    static int sMaxPipesPerMixer;
    static IdleInvalidator *idleInvalidator;
    struct FrameInfo mCurrentFrame;
    struct LayerCache mCachedFrame;
};

class MDPCompLowRes : public MDPComp {
public:
    explicit MDPCompLowRes(int dpy):MDPComp(dpy){};
    virtual ~MDPCompLowRes(){};
    virtual bool draw(hwc_context_t *ctx, hwc_display_contents_1_t *list);

private:
    struct MdpPipeInfoLowRes : public MdpPipeInfo {
        ovutils::eDest index;
        virtual ~MdpPipeInfoLowRes() {};
    };

    virtual int pipesForFB() { return 1; };
    /* configure's overlay pipes for the frame */
    virtual int configure(hwc_context_t *ctx, hwc_layer_1_t *layer,
                          PipeLayerPair& pipeLayerPair);

    /* allocates pipes to selected candidates */
    virtual bool allocLayerPipes(hwc_context_t *ctx,
                                 hwc_display_contents_1_t* list);

    virtual int pipesNeeded(hwc_context_t *ctx, hwc_display_contents_1_t* list);
};

class MDPCompHighRes : public MDPComp {
public:
    explicit MDPCompHighRes(int dpy):MDPComp(dpy){};
    virtual ~MDPCompHighRes(){};
    virtual bool draw(hwc_context_t *ctx, hwc_display_contents_1_t *list);
private:
    struct MdpPipeInfoHighRes : public MdpPipeInfo {
        ovutils::eDest lIndex;
        ovutils::eDest rIndex;
        virtual ~MdpPipeInfoHighRes() {};
    };

    bool acquireMDPPipes(hwc_context_t *ctx, hwc_layer_1_t* layer,
                         MdpPipeInfoHighRes& pipe_info, ePipeType type);

    virtual int pipesForFB() { return 2; };
    /* configure's overlay pipes for the frame */
    virtual int configure(hwc_context_t *ctx, hwc_layer_1_t *layer,
                          PipeLayerPair& pipeLayerPair);

    /* allocates pipes to selected candidates */
    virtual bool allocLayerPipes(hwc_context_t *ctx,
                                 hwc_display_contents_1_t* list);

    virtual int pipesNeeded(hwc_context_t *ctx, hwc_display_contents_1_t* list);
};
}; //namespace
#endif
