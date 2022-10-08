/*
 * Copyright (C) 2012 Intel Corporation.  All rights reserved.
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
 *
 */

#ifndef __ISVWorker_H_
#define __ISVWorker_H_

#include <va/va.h>
#include <va/va_vpp.h>
#include <va/va_android.h>
#include <OMX_Component.h>
#include <utils/RefBase.h>
#include "isv_profile.h"
#include "isv_bufmanager.h"

#define ANDROID_DISPLAY_HANDLE 0x18C34078
#define Display unsigned int

//FIXME: copy from OMX_Core.h

/* interlaced frame flag: This flag is set to indicate the buffer contains a
 * top and bottom field and display ordering is top field first.
 * @ingroup buf
 */
#define OMX_BUFFERFLAG_TFF 0x00010000

/* interlaced frame flag: This flag is set to indicate the buffer contains a 
 * top and bottom field and display ordering is bottom field first.
 * @ingroup buf
 */
#define OMX_BUFFERFLAG_BFF 0x00020000

using namespace android;

typedef enum
{
    STATUS_OK = 0,
    STATUS_NOT_SUPPORT,
    STATUS_ALLOCATION_ERROR,
    STATUS_ERROR,
    STATUS_DATA_RENDERING
} vpp_status;

typedef enum
{
    DEINTERLACE_BOB = 0,                   // BOB DI
    DEINTERLACE_ADI = 1,                   // ADI
} deinterlace_t;

//Avaiable filter types
typedef enum
{
    FILTER_HQ   = 0,                      // high-quality filter (AVS scaling)
    FILTER_FAST = 1,                      // fast filter (bilinear scaling)
} filter_t;

typedef struct {
    uint32_t srcWidth;
    uint32_t srcHeight;
    uint32_t dstWidth;
    uint32_t dstHeight;
    uint32_t denoiseLevel;
    uint32_t sharpnessLevel;
    uint32_t colorBalanceLevel;
    deinterlace_t deinterlaceType;
    filter_t scalarType;
    uint32_t frameRate;
    uint32_t hasEncoder;
    FRC_RATE frcRate;
} FilterParam;

class ISVBuffer;

class ISVWorker : public RefBase
{

    public:
        // config filters on or off based on video info
        status_t configFilters(uint32_t filters, const FilterParam* filterParam);

        // Initialize: setupVA()->setupFilters()->setupPipelineCaps()
        status_t init(uint32_t width, uint32_t height);
        status_t deinit();

        // Get output buffer number needed for processing
        uint32_t getProcBufCount();

        // Get output buffer number needed for filling
        uint32_t getFillBufCount();

        // Send input and output buffers to VSP to begin processing
        status_t process(ISVBuffer* input, Vector<ISVBuffer*> output, uint32_t outputCount, bool isEOS, uint32_t flags);

        // Fill output buffers given, it's a blocking call
        status_t fill(Vector<ISVBuffer*> output, uint32_t outputCount);

        // reset index
        status_t reset();

        // set video display mode
        void setDisplayMode(int32_t mode);

        // get video display mode
        int32_t getDisplayMode();

        // check HDMI connection status
        bool isHdmiConnected();

        uint32_t getVppOutputFps();

        // alloc/free VA surface
        status_t allocSurface(uint32_t* width, uint32_t* height,
                uint32_t stride, uint32_t format, unsigned long handle, int32_t* surfaceId);
        status_t freeSurface(int32_t* surfaceId);

        ISVWorker();
        ~ISVWorker() {}

    private:
        // Check if VPP is supported
        bool isSupport() const;

        // Get output buffer number needed for processing
        uint32_t getOutputBufCount(uint32_t index);

        // Check filter caps and create filter buffers
        status_t setupFilters();

        // Setup pipeline caps
        status_t setupPipelineCaps();

        //check if the input fps is suportted in array fpsSet.
        bool isFpsSupport(int32_t fps, int32_t *fpsSet, int32_t fpsSetCnt);

        // Debug only
        // Dump YUV frame
        status_t dumpYUVFrameData(VASurfaceID surfaceID);
        status_t writeNV12(int width, int height, unsigned char *out_buf, int y_pitch, int uv_pitch);

        ISVWorker(const ISVWorker &);
        ISVWorker &operator=(const ISVWorker &);

    public:
        uint32_t mNumForwardReferences;

    private:
        // VA common variables
        VAContextID mVAContext;
        uint32_t mWidth;
        uint32_t mHeight;
        Display * mDisplay;
        VADisplay mVADisplay;
        VAConfigID mVAConfig;

        // Forward References Surfaces
        Vector<VABufferID> mPipelineBuffers;
        Mutex mPipelineBufferLock; // to protect access to mPipelineBuffers
        VASurfaceID *mForwardReferences;
        VASurfaceID mPrevInput;
        VASurfaceID mPrevOutput;

        // VPP Filters Buffers
        uint32_t mNumFilterBuffers;
        VABufferID mFilterBuffers[VAProcFilterCount];

        // VPP filter configuration
        VABufferID mFilterFrc;

        // VPP filter configuration
        uint32_t mFilters;
        FilterParam mFilterParam;

        // status
        uint32_t mInputIndex;
        uint32_t mOutputIndex;
        uint32_t mOutputCount;

        // FIXME: not very sure how to check color standard
        VAProcColorStandardType in_color_standards[VAProcColorStandardCount];
        VAProcColorStandardType out_color_standards[VAProcColorStandardCount];
};

#endif //__ISVWorker_H_
