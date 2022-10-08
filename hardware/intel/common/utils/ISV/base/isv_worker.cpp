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
#include <cutils/properties.h>
#include <system/graphics.h>
#include "isv_worker.h"
#ifndef TARGET_VPP_USE_GEN
#include <hal_public.h>
#else
#include <ufo/graphics.h>
#endif

//#define LOG_NDEBUG 0
#undef LOG_TAG
#define LOG_TAG "isv-omxil"

#define CHECK_VASTATUS(str) \
    do { \
        if (vaStatus != VA_STATUS_SUCCESS) { \
                ALOGE("%s failed\n", str); \
                return STATUS_ERROR;}   \
        }while(0);

enum STRENGTH {
    STRENGTH_LOW = 0,
    STRENGTH_MEDIUM,
    STRENGTH_HIGH
};

#define DENOISE_DEBLOCK_STRENGTH STRENGTH_MEDIUM
#define COLOR_STRENGTH STRENGTH_MEDIUM
#ifdef TARGET_VPP_USE_GEN
#define COLOR_NUM 4
#else
#define COLOR_NUM 2
#endif

#define MAX_FRC_OUTPUT 4 /*for frcx4*/

using namespace android;

ISVWorker::ISVWorker()
    :mNumForwardReferences(0),
    mVAContext(VA_INVALID_ID),
    mWidth(0), mHeight(0),
    mDisplay(NULL), mVADisplay(NULL),
    mVAConfig(VA_INVALID_ID),
    mForwardReferences(NULL),
    mPrevInput(0), mPrevOutput(0),
    mNumFilterBuffers(0),
    mFilterFrc(VA_INVALID_ID), mFilters(0),
    mInputIndex(0), mOutputIndex(0),
    mOutputCount(0) {
    memset(&mFilterBuffers, VA_INVALID_ID, VAProcFilterCount * sizeof(VABufferID));
    memset(&mFilterParam, 0, sizeof(mFilterParam));
}

bool ISVWorker::isSupport() const {
    bool support = false;

    int num_entrypoints = vaMaxNumEntrypoints(mVADisplay);
    VAEntrypoint * entrypoints = (VAEntrypoint *)malloc(num_entrypoints * sizeof(VAEntrypoint));
    if (entrypoints == NULL) {
        ALOGE("failed to malloc entrypoints array\n");
        return false;
    }

    // check if it contains VPP entry point VAEntrypointVideoProc
    VAStatus vaStatus = vaQueryConfigEntrypoints(mVADisplay, VAProfileNone, entrypoints, &num_entrypoints);
    if (vaStatus != VA_STATUS_SUCCESS) {
        ALOGE("vaQueryConfigEntrypoints failed");
        return false;
    }
    for (int i = 0; !support && i < num_entrypoints; i++) {
        support = entrypoints[i] == VAEntrypointVideoProc;
    }
    free(entrypoints);
    entrypoints = NULL;

    return support;
}

uint32_t ISVWorker::getProcBufCount() {
    return getOutputBufCount(mInputIndex);
}

uint32_t ISVWorker::getFillBufCount() {
        return getOutputBufCount(mOutputIndex);
}

uint32_t ISVWorker::getOutputBufCount(uint32_t index) {
    uint32_t bufCount = 1;
    if (((mFilters & FilterFrameRateConversion) != 0)
            && index > 0)
            bufCount = mFilterParam.frcRate - (((mFilterParam.frcRate == FRC_RATE_2_5X) ? (index & 1): 0));
    return bufCount;
}


status_t ISVWorker::init(uint32_t width, uint32_t height) {
    ALOGV("init");

    if (mDisplay != NULL) {
        ALOGE("VA is particially started");
        return STATUS_ERROR;
    }
    mDisplay = new Display;
    *mDisplay = ANDROID_DISPLAY_HANDLE;

    mVADisplay = vaGetDisplay(mDisplay);
    if (mVADisplay == NULL) {
        ALOGE("vaGetDisplay failed");
        return STATUS_ERROR;
    }

    int majorVersion, minorVersion;
    VAStatus vaStatus = vaInitialize(mVADisplay, &majorVersion, &minorVersion);
    CHECK_VASTATUS("vaInitialize");

    // Check if VPP entry point is supported
    if (!isSupport()) {
        ALOGE("VPP is not supported on current platform");
        return STATUS_NOT_SUPPORT;
    }

    // Find out the format for the target
    VAConfigAttrib attrib;
    attrib.type = VAConfigAttribRTFormat;
    vaStatus = vaGetConfigAttributes(mVADisplay, VAProfileNone, VAEntrypointVideoProc, &attrib, 1);
    CHECK_VASTATUS("vaGetConfigAttributes");

    if ((attrib.value & VA_RT_FORMAT_YUV420) == 0) {
        ALOGE("attribute is %x vs wanted %x", attrib.value, VA_RT_FORMAT_YUV420);
        return STATUS_NOT_SUPPORT;
    }

    ALOGV("ready to create config");
    // Create the configuration
    vaStatus = vaCreateConfig(mVADisplay, VAProfileNone, VAEntrypointVideoProc, &attrib, 1, &mVAConfig);
    CHECK_VASTATUS("vaCreateConfig");


    // Create Context
    ALOGV("ready to create context");
    mWidth = width;
    mHeight = height;
    vaStatus = vaCreateContext(mVADisplay, mVAConfig, mWidth, mHeight, 0, NULL, 0, &mVAContext);
    CHECK_VASTATUS("vaCreateContext");

    ALOGV("VA has been successfully started");
    return STATUS_OK;
}

status_t ISVWorker::deinit() {
    {
        Mutex::Autolock autoLock(mPipelineBufferLock);
        while (!mPipelineBuffers.isEmpty()) {
            VABufferID pipelineBuffer = mPipelineBuffers.itemAt(0);
            if (VA_STATUS_SUCCESS != vaDestroyBuffer(mVADisplay, pipelineBuffer))
                ALOGW("%s: failed to destroy va buffer id %d", __func__, pipelineBuffer);
            mPipelineBuffers.removeAt(0);
        }
    }

    if (mNumFilterBuffers != 0) {
        for (uint32_t i = 0; i < mNumFilterBuffers; i++) {
            if(VA_STATUS_SUCCESS != vaDestroyBuffer(mVADisplay, mFilterBuffers[i]))
                ALOGW("%s: failed to destroy va buffer id %d", __func__, mFilterBuffers[i]);
        }
        mNumFilterBuffers = 0;
        memset(&mFilterBuffers, VA_INVALID_ID, VAProcFilterCount * sizeof(VABufferID));
        mFilterFrc = VA_INVALID_ID;
    }

    if (mForwardReferences != NULL) {
        free(mForwardReferences);
        mForwardReferences = NULL;
        mNumForwardReferences = 0;
    }

    if (mVAContext != VA_INVALID_ID) {
         vaDestroyContext(mVADisplay, mVAContext);
         mVAContext = VA_INVALID_ID;
    }

    if (mVAConfig != VA_INVALID_ID) {
        vaDestroyConfig(mVADisplay, mVAConfig);
        mVAConfig = VA_INVALID_ID;
    }

    if (mVADisplay) {
        vaTerminate(mVADisplay);
        mVADisplay = NULL;
    }

    if (mDisplay) {
        delete mDisplay;
        mDisplay = NULL;
    }

    return STATUS_OK;
}

status_t ISVWorker::allocSurface(uint32_t* width, uint32_t* height,
        uint32_t stride, uint32_t format, unsigned long handle, int32_t* surfaceId)
{
    if (mWidth == 0 || mHeight == 0) {
        ALOGE("%s: isv worker has not been initialized.", __func__);
        return STATUS_ERROR;
    }

#ifndef TARGET_VPP_USE_GEN
    *width = mWidth;
    *height = mHeight;
#endif
    // Create VASurfaces
    VASurfaceAttrib attribs[3];
    VASurfaceAttribExternalBuffers vaExtBuf;

    memset(&vaExtBuf, 0, sizeof(VASurfaceAttribExternalBuffers));
    switch(format) {
        case HAL_PIXEL_FORMAT_YV12:
            vaExtBuf.pixel_format = VA_FOURCC_YV12;
            vaExtBuf.num_planes = 3;
            vaExtBuf.pitches[0] = stride;
            vaExtBuf.pitches[1] = stride / 2;
            vaExtBuf.pitches[2] = stride / 2;
            vaExtBuf.pitches[3] = 0;
            vaExtBuf.offsets[0] = 0;
            vaExtBuf.offsets[1] = stride * *height;
            vaExtBuf.offsets[2] = vaExtBuf.offsets[1] + (stride / 2) * (*height / 2);
            vaExtBuf.offsets[3] = 0;
            break;
#ifdef TARGET_VPP_USE_GEN
        case HAL_PIXEL_FORMAT_NV12_Y_TILED_INTEL:
        case HAL_PIXEL_FORMAT_NV12_X_TILED_INTEL:
        //it will be removed in future, it indicate the same format with HAL_PIXEL_FORMAT_NV12_Y_TILED_INTEL
        case HAL_PIXEL_FORMAT_YUV420PackedSemiPlanar_Tiled_INTEL:
#else
        case HAL_PIXEL_FORMAT_NV12_VED:
        case HAL_PIXEL_FORMAT_NV12_VEDT:
#endif
            vaExtBuf.pixel_format = VA_FOURCC_NV12;
            vaExtBuf.num_planes = 2;
            vaExtBuf.pitches[0] = stride;
            vaExtBuf.pitches[1] = stride;
            vaExtBuf.pitches[2] = 0;
            vaExtBuf.pitches[3] = 0;
            vaExtBuf.offsets[0] = 0;
            vaExtBuf.offsets[1] = stride * *height;
            vaExtBuf.offsets[2] = 0;
            vaExtBuf.offsets[3] = 0;
            break;
        default:
            ALOGE("%s: can't support this format 0x%08x", __func__, format);
            return STATUS_ERROR;
    }
    vaExtBuf.width = *width;
    vaExtBuf.height = *height;
    vaExtBuf.data_size = stride * *height * 1.5;
    vaExtBuf.num_buffers = 1;
#ifndef TARGET_VPP_USE_GEN
    if (format == HAL_PIXEL_FORMAT_NV12_VEDT) {
        ALOGV("set TILING flag");
        vaExtBuf.flags |= VA_SURFACE_EXTBUF_DESC_ENABLE_TILING;
    }
#endif
    vaExtBuf.flags |= VA_SURFACE_ATTRIB_MEM_TYPE_ANDROID_GRALLOC;
    vaExtBuf.buffers = (long unsigned int*)&handle;

    attribs[0].type = (VASurfaceAttribType)VASurfaceAttribMemoryType;
    attribs[0].flags = VA_SURFACE_ATTRIB_SETTABLE;
    attribs[0].value.type = VAGenericValueTypeInteger;
    attribs[0].value.value.i = VA_SURFACE_ATTRIB_MEM_TYPE_ANDROID_GRALLOC;

    attribs[1].type = (VASurfaceAttribType)VASurfaceAttribExternalBufferDescriptor;
    attribs[1].flags = VA_SURFACE_ATTRIB_SETTABLE;
    attribs[1].value.type = VAGenericValueTypePointer;
    attribs[1].value.value.p = &vaExtBuf;

    attribs[2].type = (VASurfaceAttribType)VASurfaceAttribUsageHint;
    attribs[2].flags = VA_SURFACE_ATTRIB_SETTABLE;
    attribs[2].value.type = VAGenericValueTypeInteger;
    attribs[2].value.value.i = VA_SURFACE_ATTRIB_USAGE_HINT_VPP_READ;

    ALOGV("%s: Ext buffer: width %d, height %d, data_size %d, pitch %d", __func__,
            vaExtBuf.width, vaExtBuf.height, vaExtBuf.data_size, vaExtBuf.pitches[0]);
    VAStatus vaStatus = vaCreateSurfaces(mVADisplay, VA_RT_FORMAT_YUV420, vaExtBuf.width,
                                 vaExtBuf.height, (VASurfaceID*)surfaceId, 1, attribs, 3);
    CHECK_VASTATUS("vaCreateSurfaces");

    return (vaStatus == VA_STATUS_SUCCESS) ? STATUS_OK : STATUS_ERROR;
}

status_t ISVWorker::freeSurface(int32_t* surfaceId)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    vaDestroySurfaces(mVADisplay, (VASurfaceID*)surfaceId, 1);
    CHECK_VASTATUS("vaDestroySurfaces");

    return (vaStatus == VA_STATUS_SUCCESS) ? STATUS_OK : STATUS_ERROR;
}

status_t ISVWorker::configFilters(uint32_t filters,
                                  const FilterParam* filterParam)
{
    status_t ret = STATUS_OK;

    if (!filterParam) {
        ALOGE("%s: invalid filterParam", __func__);
        return STATUS_ERROR;
    }

    if (filters != 0) {
        mFilterParam.srcWidth = filterParam->srcWidth;
        mFilterParam.srcHeight = filterParam->srcHeight;
        mFilterParam.dstWidth = filterParam->dstWidth;
        mFilterParam.dstHeight = filterParam->dstHeight;
        mFilterParam.frameRate = filterParam->frameRate;
        mFilterParam.frcRate = filterParam->frcRate;
    }

    if (mFilters != filters) {
        mFilters = filters;
        ALOGI("%s: mFilters 0x%x, fps %d, frc rate %d", __func__, mFilters, mFilterParam.frameRate, mFilterParam.frcRate);
        ret = setupFilters();
    }

    return ret;
}

bool ISVWorker::isFpsSupport(int32_t fps, int32_t *fpsSet, int32_t fpsSetCnt) {
    bool ret = false;
    for (int32_t i = 0; i < fpsSetCnt; i++) {
        if (fps == fpsSet[i]) {
            ret = true;
            break;
        }
    }

    return ret;
}

status_t ISVWorker::setupFilters() {
    ALOGV("setupFilters");
    VAProcFilterParameterBuffer deblock, denoise, sharpen, stde;
    VAProcFilterParameterBufferDeinterlacing deint;
    VAProcFilterParameterBufferColorBalance color[COLOR_NUM];
    VAProcFilterParameterBufferFrameRateConversion frc;
    VABufferID deblockId, denoiseId, deintId, sharpenId, colorId, frcId, stdeId;
    uint32_t numCaps;
    VAProcFilterCap deblockCaps, denoiseCaps, sharpenCaps, frcCaps, stdeCaps;
    VAProcFilterCapDeinterlacing deinterlacingCaps[VAProcDeinterlacingCount];
    VAProcFilterCapColorBalance colorCaps[COLOR_NUM];
    VAStatus vaStatus;
    uint32_t numSupportedFilters = VAProcFilterCount;
    VAProcFilterType supportedFilters[VAProcFilterCount];

    if (mNumFilterBuffers != 0) {
        for (uint32_t i = 0; i < mNumFilterBuffers; i++) {
            if (VA_STATUS_SUCCESS != vaDestroyBuffer(mVADisplay, mFilterBuffers[i]))
                ALOGW("%s: failed to destroy va buffer %d", __func__, mFilterBuffers[i]);
                //return STATUS_ERROR;
        }
        memset(&mFilterBuffers, VA_INVALID_ID, VAProcFilterCount * sizeof(VABufferID));
        mFilterFrc = VA_INVALID_ID;
        mNumFilterBuffers = 0;
    }

    // query supported filters
    vaStatus = vaQueryVideoProcFilters(mVADisplay, mVAContext, supportedFilters, &numSupportedFilters);
    CHECK_VASTATUS("vaQueryVideoProcFilters");

    // create filter buffer for each filter
    for (uint32_t i = 0; i < numSupportedFilters; i++) {
        switch (supportedFilters[i]) {
            case VAProcFilterDeblocking:
                if ((mFilters & FilterDeblocking) != 0) {
                    // check filter caps
                    numCaps = 1;
                    vaStatus = vaQueryVideoProcFilterCaps(mVADisplay, mVAContext,
                            VAProcFilterDeblocking,
                            &deblockCaps,
                            &numCaps);
                    CHECK_VASTATUS("vaQueryVideoProcFilterCaps for deblocking");
                    // create parameter buffer
                    deblock.type = VAProcFilterDeblocking;
                    deblock.value = deblockCaps.range.min_value + DENOISE_DEBLOCK_STRENGTH * deblockCaps.range.step;
                    vaStatus = vaCreateBuffer(mVADisplay, mVAContext,
                        VAProcFilterParameterBufferType, sizeof(deblock), 1,
                        &deblock, &deblockId);
                    CHECK_VASTATUS("vaCreateBuffer for deblocking");
                    mFilterBuffers[mNumFilterBuffers] = deblockId;
                    mNumFilterBuffers++;
                }
                break;
            case VAProcFilterNoiseReduction:
                if((mFilters & FilterNoiseReduction) != 0) {
                    // check filter caps
                    numCaps = 1;
                    vaStatus = vaQueryVideoProcFilterCaps(mVADisplay, mVAContext,
                            VAProcFilterNoiseReduction,
                            &denoiseCaps,
                            &numCaps);
                    CHECK_VASTATUS("vaQueryVideoProcFilterCaps for denoising");
                    // create parameter buffer
                    denoise.type = VAProcFilterNoiseReduction;
#ifdef TARGET_VPP_USE_GEN
                    char propValueString[PROPERTY_VALUE_MAX];

                    // placeholder for vpg driver: can't support denoise factor auto adjust, so leave config to user.
                    property_get("vpp.filter.denoise.factor", propValueString, "64.0");
                    denoise.value = atof(propValueString);
                    denoise.value = (denoise.value < 0.0f) ? 0.0f : denoise.value;
                    denoise.value = (denoise.value > 64.0f) ? 64.0f : denoise.value;
#else
                    denoise.value = denoiseCaps.range.min_value + DENOISE_DEBLOCK_STRENGTH * denoiseCaps.range.step;
#endif
                    vaStatus = vaCreateBuffer(mVADisplay, mVAContext,
                        VAProcFilterParameterBufferType, sizeof(denoise), 1,
                        &denoise, &denoiseId);
                    CHECK_VASTATUS("vaCreateBuffer for denoising");
                    mFilterBuffers[mNumFilterBuffers] = denoiseId;
                    mNumFilterBuffers++;
                }
                break;
            case VAProcFilterDeinterlacing:
                if ((mFilters & FilterDeinterlacing) != 0) {
                    numCaps = VAProcDeinterlacingCount;
                    vaStatus = vaQueryVideoProcFilterCaps(mVADisplay, mVAContext,
                            VAProcFilterDeinterlacing,
                            &deinterlacingCaps[0],
                            &numCaps);
                    CHECK_VASTATUS("vaQueryVideoProcFilterCaps for deinterlacing");
                    for (uint32_t i = 0; i < numCaps; i++)
                    {
                        VAProcFilterCapDeinterlacing * const cap = &deinterlacingCaps[i];
                        if (cap->type != VAProcDeinterlacingBob) // desired Deinterlacing Type
                            continue;

                        deint.type = VAProcFilterDeinterlacing;
                        deint.algorithm = VAProcDeinterlacingBob;
                        vaStatus = vaCreateBuffer(mVADisplay,
                                mVAContext,
                                VAProcFilterParameterBufferType,
                                sizeof(deint), 1,
                                &deint, &deintId);
                        CHECK_VASTATUS("vaCreateBuffer for deinterlacing");
                        mFilterBuffers[mNumFilterBuffers] = deintId;
                        mNumFilterBuffers++;
                    }
                }
                break;
            case VAProcFilterSharpening:
                if((mFilters & FilterSharpening) != 0) {
                    // check filter caps
                    numCaps = 1;
                    vaStatus = vaQueryVideoProcFilterCaps(mVADisplay, mVAContext,
                            VAProcFilterSharpening,
                            &sharpenCaps,
                            &numCaps);
                    CHECK_VASTATUS("vaQueryVideoProcFilterCaps for sharpening");
                    // create parameter buffer
                    sharpen.type = VAProcFilterSharpening;
#ifdef TARGET_VPP_USE_GEN
                    char propValueString[PROPERTY_VALUE_MAX];

                    // placeholder for vpg driver: can't support sharpness factor auto adjust, so leave config to user.
                    property_get("vpp.filter.sharpen.factor", propValueString, "8.0");
                    sharpen.value = atof(propValueString);
                    sharpen.value = (sharpen.value < 0.0f) ? 0.0f : sharpen.value;
                    sharpen.value = (sharpen.value > 64.0f) ? 64.0f : sharpen.value;
#else
                    sharpen.value = sharpenCaps.range.default_value;
#endif
                    vaStatus = vaCreateBuffer(mVADisplay, mVAContext,
                        VAProcFilterParameterBufferType, sizeof(sharpen), 1,
                        &sharpen, &sharpenId);
                    CHECK_VASTATUS("vaCreateBuffer for sharpening");
                    mFilterBuffers[mNumFilterBuffers] = sharpenId;
                    mNumFilterBuffers++;
                }
                break;
            case VAProcFilterColorBalance:
                if((mFilters & FilterColorBalance) != 0) {
                    uint32_t featureCount = 0;
                    // check filter caps
                    // FIXME: it's not used at all!
                    numCaps = COLOR_NUM;
                    vaStatus = vaQueryVideoProcFilterCaps(mVADisplay, mVAContext,
                            VAProcFilterColorBalance,
                            colorCaps,
                            &numCaps);
                    CHECK_VASTATUS("vaQueryVideoProcFilterCaps for color balance");
                    // create parameter buffer
                    for (uint32_t i = 0; i < numCaps; i++) {
                        if (colorCaps[i].type == VAProcColorBalanceAutoSaturation) {
                            color[i].type = VAProcFilterColorBalance;
                            color[i].attrib = VAProcColorBalanceAutoSaturation;
                            color[i].value = colorCaps[i].range.min_value + COLOR_STRENGTH * colorCaps[i].range.step;
                            featureCount++;
                        }
                        else if (colorCaps[i].type == VAProcColorBalanceAutoBrightness) {
                            color[i].type = VAProcFilterColorBalance;
                            color[i].attrib = VAProcColorBalanceAutoBrightness;
                            color[i].value = colorCaps[i].range.min_value + COLOR_STRENGTH * colorCaps[i].range.step;
                            featureCount++;
                        }
                    }
#ifdef TARGET_VPP_USE_GEN
                    //TODO: VPG need to support check input value by colorCaps.
                    enum {kHue = 0, kSaturation, kBrightness, kContrast};
                    char propValueString[PROPERTY_VALUE_MAX];
                    color[kHue].type = VAProcFilterColorBalance;
                    color[kHue].attrib = VAProcColorBalanceHue;

                    // placeholder for vpg driver: can't support auto color balance, so leave config to user.
                    property_get("vpp.filter.procamp.hue", propValueString, "179.0");
                    color[kHue].value = atof(propValueString);
                    color[kHue].value = (color[kHue].value < -180.0f) ? -180.0f : color[kHue].value;
                    color[kHue].value = (color[kHue].value > 180.0f) ? 180.0f : color[kHue].value;
                    featureCount++;

                    color[kSaturation].type   = VAProcFilterColorBalance;
                    color[kSaturation].attrib = VAProcColorBalanceSaturation;
                    property_get("vpp.filter.procamp.saturation", propValueString, "1.0");
                    color[kSaturation].value = atof(propValueString);
                    color[kSaturation].value = (color[kSaturation].value < 0.0f) ? 0.0f : color[kSaturation].value;
                    color[kSaturation].value = (color[kSaturation].value > 10.0f) ? 10.0f : color[kSaturation].value;
                    featureCount++;

                    color[kBrightness].type   = VAProcFilterColorBalance;
                    color[kBrightness].attrib = VAProcColorBalanceBrightness;
                    property_get("vpp.filter.procamp.brightness", propValueString, "0.0");
                    color[kBrightness].value = atof(propValueString);
                    color[kBrightness].value = (color[kBrightness].value < -100.0f) ? -100.0f : color[kBrightness].value;
                    color[kBrightness].value = (color[kBrightness].value > 100.0f) ? 100.0f : color[kBrightness].value;
                    featureCount++;

                    color[kContrast].type   = VAProcFilterColorBalance;
                    color[kContrast].attrib = VAProcColorBalanceContrast;
                    property_get("vpp.filter.procamp.contrast", propValueString, "1.0");
                    color[kContrast].value = atof(propValueString);
                    color[kContrast].value = (color[kContrast].value < 0.0f) ? 0.0f : color[kContrast].value;
                    color[kContrast].value = (color[kContrast].value > 10.0f) ? 10.0f : color[kContrast].value;
                    featureCount++;
#endif
                    vaStatus = vaCreateBuffer(mVADisplay, mVAContext,
                        VAProcFilterParameterBufferType, sizeof(*color), featureCount,
                        color, &colorId);
                    CHECK_VASTATUS("vaCreateBuffer for color balance");
                    mFilterBuffers[mNumFilterBuffers] = colorId;
                    mNumFilterBuffers++;
                }
                break;
            case VAProcFilterFrameRateConversion:
                if((mFilters & FilterFrameRateConversion) != 0) {
                    frc.type = VAProcFilterFrameRateConversion;
                    frc.input_fps = mFilterParam.frameRate;
                    switch (mFilterParam.frcRate){
                        case FRC_RATE_1X:
                            frc.output_fps = frc.input_fps;
                            break;
                        case FRC_RATE_2X:
                            frc.output_fps = frc.input_fps * 2;
                            break;
                        case FRC_RATE_2_5X:
                            frc.output_fps = frc.input_fps * 5/2;
                            break;
                        case FRC_RATE_4X:
                            frc.output_fps = frc.input_fps * 4;
                            break;
                    }
                    vaStatus = vaCreateBuffer(mVADisplay, mVAContext,
                        VAProcFilterParameterBufferType, sizeof(frc), 1,
                        &frc, &frcId);
                    CHECK_VASTATUS("vaCreateBuffer for frc");
                    mFilterBuffers[mNumFilterBuffers] = frcId;
                    mNumFilterBuffers++;
                    mFilterFrc = frcId;
                }
                break;
            case VAProcFilterSkinToneEnhancement:
                if((mFilters & FilterSkinToneEnhancement) != 0) {
                    // check filter caps
                    numCaps = 1;
                    vaStatus = vaQueryVideoProcFilterCaps(mVADisplay, mVAContext,
                            VAProcFilterSkinToneEnhancement,
                            &stdeCaps,
                            &numCaps);
                    CHECK_VASTATUS("vaQueryVideoProcFilterCaps for skintone");
                    // create parameter buffer
                    stde.type = VAProcFilterSkinToneEnhancement;
#ifdef TARGET_VPP_USE_GEN
                    char propValueString[PROPERTY_VALUE_MAX];

                    // placeholder for vpg driver: can't support skintone factor auto adjust, so leave config to user.
                    property_get("vpp.filter.skintone.factor", propValueString, "8.0");
                    stde.value = atof(propValueString);
                    stde.value = (stde.value < 0.0f) ? 0.0f : stde.value;
                    stde.value = (stde.value > 8.0f) ? 8.0f : stde.value;
#else
                    stde.value = stdeCaps.range.default_value;
#endif
                    vaStatus = vaCreateBuffer(mVADisplay, mVAContext,
                        VAProcFilterParameterBufferType, sizeof(stde), 1,
                        &stde, &stdeId);
                    CHECK_VASTATUS("vaCreateBuffer for skintone");
                    mFilterBuffers[mNumFilterBuffers] = stdeId;
                    mNumFilterBuffers++;
                }
                break;
            default:
                ALOGW("%s: Not supported filter 0x%08x", __func__, supportedFilters[i]);
                break;
        }
    }

    return setupPipelineCaps();
}

status_t ISVWorker::setupPipelineCaps() {
    ALOGV("setupPipelineCaps");
    //TODO color standards
    VAProcPipelineCaps pipelineCaps;
    VAStatus vaStatus;
    pipelineCaps.input_color_standards = in_color_standards;
    pipelineCaps.num_input_color_standards = VAProcColorStandardCount;
    pipelineCaps.output_color_standards = out_color_standards;
    pipelineCaps.num_output_color_standards = VAProcColorStandardCount;

    vaStatus = vaQueryVideoProcPipelineCaps(mVADisplay, mVAContext,
        mFilterBuffers, mNumFilterBuffers,
        &pipelineCaps);
    CHECK_VASTATUS("vaQueryVideoProcPipelineCaps");

    if (mForwardReferences != NULL) {
        free(mForwardReferences);
        mForwardReferences = NULL;
        mNumForwardReferences = 0;
    }

    mNumForwardReferences = pipelineCaps.num_forward_references;
    if (mNumForwardReferences > 0) {
        mForwardReferences = (VASurfaceID*)malloc(mNumForwardReferences * sizeof(VASurfaceID));
        if (mForwardReferences == NULL)
            return STATUS_ALLOCATION_ERROR;
        memset(mForwardReferences, 0, mNumForwardReferences * sizeof(VASurfaceID));
    }
    return STATUS_OK;
}

status_t ISVWorker::process(ISVBuffer* inputBuffer, Vector<ISVBuffer*> outputBuffer,
                             uint32_t outputCount, bool isEOS, uint32_t flags) {
    ALOGV("process: outputCount=%d, mInputIndex=%d", outputCount, mInputIndex);
    VASurfaceID input;
    VASurfaceID output[MAX_FRC_OUTPUT];
    VABufferID pipelineId;
    VAProcPipelineParameterBuffer *pipeline;
    VAProcFilterParameterBufferFrameRateConversion *frc;
    VAStatus vaStatus = STATUS_OK;
    uint32_t i = 0;

    if (isEOS) {
        if (mInputIndex == 0) {
            ALOGV("%s: don't need to flush VSP", __func__);
            return STATUS_OK;
        }
        input = VA_INVALID_SURFACE;
        outputCount = 1;
        output[0] = mPrevOutput;
    } else {
        if (!inputBuffer || outputBuffer.size() != outputCount) {
            ALOGE("%s: invalid input/output buffer", __func__);
            return STATUS_ERROR;
        }

        if (outputCount < 1 || outputCount > 4) {
            ALOGE("%s: invalid outputCount", __func__);
            return STATUS_ERROR;
        }

        input = inputBuffer->getSurface();
        for (i = 0; i < outputCount; i++) {
            output[i] = outputBuffer[i]->getSurface();
            if (output[i] == VA_INVALID_SURFACE) {
                ALOGE("invalid output buffer");
                return STATUS_ERROR;
            }
        }
    }

    // reference frames setting
    if (mNumForwardReferences > 0) {
        /* add previous frame into reference array*/
        for (i = 1; i < mNumForwardReferences; i++) {
            mForwardReferences[i - 1] = mForwardReferences[i];
        }

        //make last reference to input
        mForwardReferences[mNumForwardReferences - 1] = mPrevInput;
    }

    mPrevInput = input;

    // create pipeline parameter buffer
    vaStatus = vaCreateBuffer(mVADisplay,
            mVAContext,
            VAProcPipelineParameterBufferType,
            sizeof(VAProcPipelineParameterBuffer),
            1,
            NULL,
            &pipelineId);
    CHECK_VASTATUS("vaCreateBuffer for VAProcPipelineParameterBufferType");

    ALOGV("before vaBeginPicture");
    vaStatus = vaBeginPicture(mVADisplay, mVAContext, output[0]);
    CHECK_VASTATUS("vaBeginPicture");

    // map pipeline paramter buffer
    vaStatus = vaMapBuffer(mVADisplay, pipelineId, (void**)&pipeline);
    CHECK_VASTATUS("vaMapBuffer for pipeline parameter buffer");

    // frc pamameter setting
    if ((mFilters & FilterFrameRateConversion) != 0) {
        vaStatus = vaMapBuffer(mVADisplay, mFilterFrc, (void **)&frc);
        CHECK_VASTATUS("vaMapBuffer for frc parameter buffer");
        if (isEOS)
            frc->num_output_frames = 0;
        else
            frc->num_output_frames = outputCount - 1;
        frc->output_frames = output + 1;
    }

    // pipeline parameter setting
    VARectangle dst_region;
    dst_region.x = 0;
    dst_region.y = 0;
    dst_region.width = mFilterParam.dstWidth;
    dst_region.height = mFilterParam.dstHeight;

    VARectangle src_region;
    src_region.x = 0;
    src_region.y = 0;
    src_region.width = mFilterParam.srcWidth;
    src_region.height = mFilterParam.srcHeight;

    if (isEOS) {
        pipeline->surface = 0;
        pipeline->pipeline_flags = VA_PIPELINE_FLAG_END;
    }
    else {
        pipeline->surface = input;
        pipeline->pipeline_flags = 0;
    }
#ifdef TARGET_VPP_USE_GEN
    pipeline->surface_region = &src_region;
    pipeline->output_region = &dst_region;
    pipeline->surface_color_standard = VAProcColorStandardBT601;
    pipeline->output_color_standard = VAProcColorStandardBT601;
#else
    pipeline->surface_region = NULL;
    pipeline->output_region = NULL;//&output_region;
    pipeline->surface_color_standard = VAProcColorStandardNone;
    pipeline->output_color_standard = VAProcColorStandardNone;
    /* real rotate state will be decided in psb video */
    pipeline->rotation_state = 0;
#endif
    /* FIXME: set more meaningful background color */
    pipeline->output_background_color = 0;
    pipeline->filters = mFilterBuffers;
    pipeline->num_filters = mNumFilterBuffers;
    pipeline->forward_references = mForwardReferences;
    pipeline->num_forward_references = mNumForwardReferences;
    pipeline->backward_references = NULL;
    pipeline->num_backward_references = 0;

    //currently, we only transfer TOP field to frame, no frame rate change.
    if (flags & (OMX_BUFFERFLAG_TFF | OMX_BUFFERFLAG_BFF)) {
        pipeline->filter_flags = VA_TOP_FIELD;
    } else {
        pipeline->filter_flags = VA_FRAME_PICTURE;
    }

    if ((mFilters & FilterFrameRateConversion) != 0) {
        vaStatus = vaUnmapBuffer(mVADisplay, mFilterFrc);
        CHECK_VASTATUS("vaUnmapBuffer for frc parameter buffer");
    }

    vaStatus = vaUnmapBuffer(mVADisplay, pipelineId);
    CHECK_VASTATUS("vaUnmapBuffer for pipeline parameter buffer");

    ALOGV("before vaRenderPicture");
    // Send parameter to driver
    vaStatus = vaRenderPicture(mVADisplay, mVAContext, &pipelineId, 1);
    CHECK_VASTATUS("vaRenderPicture");

    ALOGV("before vaEndPicture");
    vaStatus = vaEndPicture(mVADisplay, mVAContext);
    CHECK_VASTATUS("vaEndPicture");
    
    if (isEOS) {
        vaStatus = vaSyncSurface(mVADisplay, mPrevOutput);
        CHECK_VASTATUS("vaSyncSurface");
        if (VA_STATUS_SUCCESS != vaDestroyBuffer(mVADisplay, pipelineId)) {
            ALOGE("%s: failed to destroy va buffer %d", __func__, pipelineId);
            return STATUS_ERROR;
        }
        return STATUS_OK;
    }

    mPrevOutput = output[0];
    mInputIndex++;

    Mutex::Autolock autoLock(mPipelineBufferLock);
    mPipelineBuffers.push_back(pipelineId);

    ALOGV("process, exit");
    return STATUS_OK;
}

status_t ISVWorker::fill(Vector<ISVBuffer*> outputBuffer, uint32_t outputCount) {
    ALOGV("fill, outputCount=%d, mOutputIndex=%d",outputCount, mOutputIndex);
    // get output surface
    VASurfaceID output[MAX_FRC_OUTPUT];
    VAStatus vaStatus;
    VASurfaceStatus surStatus;

    if (outputCount < 1)
        return STATUS_ERROR;
    // map GraphicBuffer to VASurface
    for (uint32_t i = 0; i < outputCount; i++) {

        output[i] = outputBuffer[i]->getSurface();
        if (output[i] == VA_INVALID_SURFACE) {
            ALOGE("invalid output buffer");
            return STATUS_ERROR;
        }
        //FIXME: only enable sync mode
#if 0
        vaStatus = vaQuerySurfaceStatus(mVADisplay, output[i],&surStatus);
        CHECK_VASTATUS("vaQuerySurfaceStatus");
        if (surStatus == VASurfaceRendering) {
            ALOGV("Rendering %d", i);
            /* The behavior of driver is: all output of one process task are return in one interruption.
               The whole outputs of one FRC task are all ready or none of them is ready.
               If the behavior changed, it hurts the performance.
            */
            if (0 != i) {
                ALOGW("*****Driver behavior changed. The performance is hurt.");
                ALOGW("Please check driver behavior: all output of one task return in one interruption.");
            }
            vaStatus = STATUS_DATA_RENDERING;
            break;
        }

        if ((surStatus != VASurfaceRendering) && (surStatus != VASurfaceReady)) {
            ALOGE("surface statu Error %d", surStatus);
            vaStatus = STATUS_ERROR;
        }
#endif
        vaStatus = vaSyncSurface(mVADisplay, output[i]);
        CHECK_VASTATUS("vaSyncSurface");
        vaStatus = STATUS_OK;
        mOutputCount++;
        //dumpYUVFrameData(output[i]);
    }

    {
        Mutex::Autolock autoLock(mPipelineBufferLock);
        if (vaStatus == STATUS_OK) {
            VABufferID pipelineBuffer = mPipelineBuffers.itemAt(0);
            if (VA_STATUS_SUCCESS != vaDestroyBuffer(mVADisplay, pipelineBuffer)) {
                ALOGE("%s: failed to destroy va buffer %d", __func__, pipelineBuffer);
                return STATUS_ERROR;
            }
            mPipelineBuffers.removeAt(0);
            mOutputIndex++;
        }
    }

    ALOGV("fill, exit");
    return vaStatus;
}

// Debug only
#define FRAME_OUTPUT_FILE_NV12 "/storage/sdcard0/vpp_output.nv12"
status_t ISVWorker::dumpYUVFrameData(VASurfaceID surfaceID) {
    status_t ret;
    if (surfaceID == VA_INVALID_SURFACE)
        return STATUS_ERROR;

    VAStatus vaStatus;
    VAImage image;
    unsigned char *data_ptr;

    vaStatus = vaDeriveImage(mVADisplay,
            surfaceID,
            &image);
    CHECK_VASTATUS("vaDeriveImage");

    vaStatus = vaMapBuffer(mVADisplay, image.buf, (void **)&data_ptr);
    CHECK_VASTATUS("vaMapBuffer");

    ret = writeNV12(mFilterParam.srcWidth, mFilterParam.srcHeight, data_ptr, image.pitches[0], image.pitches[1]);
    if (ret != STATUS_OK) {
        ALOGV("writeNV12 error");
        return STATUS_ERROR;
    }

    vaStatus = vaUnmapBuffer(mVADisplay, image.buf);
    CHECK_VASTATUS("vaUnMapBuffer");

    vaStatus = vaDestroyImage(mVADisplay,image.image_id);
    CHECK_VASTATUS("vaDestroyImage");

    return STATUS_OK;
}

status_t ISVWorker::reset() {
    status_t ret;
    ALOGV("reset");
    if (mOutputCount > 0) {
        ALOGI("======mVPPInputCount=%d, mVPPRenderCount=%d======",
                mInputIndex, mOutputCount);
    }
    mInputIndex = 0;
    mOutputIndex = 0;
    mOutputCount = 0;

    {
        Mutex::Autolock autoLock(mPipelineBufferLock);
        while (!mPipelineBuffers.isEmpty()) {
            VABufferID pipelineBuffer = mPipelineBuffers.itemAt(0);
            if (VA_STATUS_SUCCESS != vaDestroyBuffer(mVADisplay, pipelineBuffer)) {
                ALOGE("%s: failed to destroy va buffer %d", __func__, pipelineBuffer);
                return STATUS_ERROR;
            }
            mPipelineBuffers.removeAt(0);
        }
    }

    if (mNumFilterBuffers != 0) {
        for (uint32_t i = 0; i < mNumFilterBuffers; i++) {
            if (VA_STATUS_SUCCESS != vaDestroyBuffer(mVADisplay, mFilterBuffers[i]))
                ALOGW("%s: failed to destroy va buffer %d", __func__, mFilterBuffers[i]);
                //return STATUS_ERROR;
        }
        mNumFilterBuffers = 0;
        memset(&mFilterBuffers, VA_INVALID_ID, VAProcFilterCount * sizeof(VABufferID));
        mFilterFrc = VA_INVALID_ID;
    }

    // we need to clear the cache for reference surfaces
    if (mForwardReferences != NULL) {
        free(mForwardReferences);
        mForwardReferences = NULL;
        mNumForwardReferences = 0;
    }

    if (mVAContext != VA_INVALID_ID) {
         vaDestroyContext(mVADisplay, mVAContext);
         mVAContext = VA_INVALID_ID;
    }
    VAStatus vaStatus = vaCreateContext(mVADisplay, mVAConfig, mWidth, mHeight, 0, NULL, 0, &mVAContext);
    CHECK_VASTATUS("vaCreateContext");

    return setupFilters();
}

uint32_t ISVWorker::getVppOutputFps() {
    uint32_t outputFps;
    //mFilterParam.frcRate is 1 if FRC is disabled or input FPS is not changed by VPP.
    if (FRC_RATE_2_5X == mFilterParam.frcRate) {
        outputFps = mFilterParam.frameRate * 5 / 2;
    } else {
        outputFps = mFilterParam.frameRate * mFilterParam.frcRate;
    }

    ALOGV("vpp is on in settings %d %d %d", outputFps,  mFilterParam.frameRate, mFilterParam.frcRate);
    return outputFps;
}


status_t ISVWorker::writeNV12(int width, int height, unsigned char *out_buf, int y_pitch, int uv_pitch) {
    size_t result;
    int frame_size;
    unsigned char *y_start, *uv_start;
    int h;

    FILE *ofile = fopen(FRAME_OUTPUT_FILE_NV12, "ab");
    if(ofile == NULL) {
        ALOGE("Open %s failed!", FRAME_OUTPUT_FILE_NV12);
        return STATUS_ERROR;
    }

    if (out_buf == NULL)
    {
        fclose(ofile);
        return STATUS_ERROR;
    }
    if ((width % 2) || (height % 2))
    {
        fclose(ofile);
        return STATUS_ERROR;
    }
    // Set frame size
    frame_size = height * width * 3/2;

    /* write y */
    y_start = out_buf;
    for (h = 0; h < height; ++h) {
        result = fwrite(y_start, sizeof(unsigned char), width, ofile);
        y_start += y_pitch;
    }

    /* write uv */
    uv_start = out_buf + uv_pitch * height;
    for (h = 0; h < height / 2; ++h) {
        result = fwrite(uv_start, sizeof(unsigned char), width, ofile);
        uv_start += uv_pitch;
    }
    // Close file
    fclose(ofile);
    return STATUS_OK;
}

