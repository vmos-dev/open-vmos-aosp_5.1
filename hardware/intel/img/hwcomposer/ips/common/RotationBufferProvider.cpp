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

#include <common/utils/HwcTrace.h>
#include <ips/common/RotationBufferProvider.h>

namespace android {
namespace intel {

#define CHECK_VA_STATUS_RETURN(FUNC) \
if (vaStatus != VA_STATUS_SUCCESS) {\
    ELOGTRACE(FUNC" failed. vaStatus = %#x", vaStatus);\
    return false;\
}

#define CHECK_VA_STATUS_BREAK(FUNC) \
if (vaStatus != VA_STATUS_SUCCESS) {\
    ELOGTRACE(FUNC" failed. vaStatus = %#x", vaStatus);\
    break;\
}

// With this display value, VA will hook VED driver insead of VSP driver for buffer rotation
#define DISPLAYVALUE  0x56454450

RotationBufferProvider::RotationBufferProvider(Wsbm* wsbm)
    : mWsbm(wsbm),
      mVaInitialized(false),
      mVaDpy(0),
      mVaCfg(0),
      mVaCtx(0),
      mVaBufFilter(0),
      mSourceSurface(0),
      mDisplay(DISPLAYVALUE),
      mWidth(0),
      mHeight(0),
      mTransform(0),
      mRotatedWidth(0),
      mRotatedHeight(0),
      mRotatedStride(0),
      mTargetIndex(0),
      mTTMWrappers(),
      mBobDeinterlace(0)
{
    for (int i = 0; i < MAX_SURFACE_NUM; i++) {
        mKhandles[i] = 0;
        mRotatedSurfaces[i] = 0;
        mDrmBuf[i] = NULL;
    }
}

RotationBufferProvider::~RotationBufferProvider()
{
}

uint32_t RotationBufferProvider::getMilliseconds()
{
    struct timeval ptimeval;
    gettimeofday(&ptimeval, NULL);
    return (uint32_t)((ptimeval.tv_sec * 1000) + (ptimeval.tv_usec / 1000));
}

bool RotationBufferProvider::initialize()
{
    if (NULL == mWsbm)
        return false;
    mTTMWrappers.setCapacity(TTM_WRAPPER_COUNT);
    return true;
}

void RotationBufferProvider::deinitialize()
{
    stopVA();
    reset();
}

void RotationBufferProvider::reset()
{
    if (mTTMWrappers.size()) {
        invalidateCaches();
    }
}

void RotationBufferProvider::invalidateCaches()
{
    void *buf;

    for (size_t i = 0; i < mTTMWrappers.size(); i++) {
        buf = mTTMWrappers.valueAt(i);
        if (!mWsbm->destroyTTMBuffer(buf))
            WLOGTRACE("failed to free TTMBuffer");
    }
    mTTMWrappers.clear();
}

int RotationBufferProvider::transFromHalToVa(int transform)
{
    if (transform == HAL_TRANSFORM_ROT_90)
        return VA_ROTATION_90;
    if (transform == HAL_TRANSFORM_ROT_180)
        return VA_ROTATION_180;
    if (transform == HAL_TRANSFORM_ROT_270)
        return VA_ROTATION_270;
    return 0;
}

int RotationBufferProvider::getStride(bool isTarget, int width)
{
    int stride = 0;
    if (width <= 512)
        stride = 512;
    else if (width <= 1024)
        stride = 1024;
    else if (width <= 1280) {
        stride = 1280;
        if (isTarget)
            stride = 2048;
    } else if (width <= 2048)
        stride = 2048;
    else if (width <= 4096)
        stride = 4096;
    else
        stride = (width + 0x3f) & ~0x3f;
    return stride;
}

uint32_t RotationBufferProvider::createWsbmBuffer(int width, int height, void **buf)
{
    int size = width * height * 3 / 2; // YUV420 NV12 format
    int allignment = 16 * 2048; // tiling row stride aligned
    bool ret = mWsbm->allocateTTMBuffer(size, allignment, buf);

    if (ret == false) {
        ELOGTRACE("failed to allocate TTM buffer");
        return 0;
    }

    return mWsbm->getKBufHandle(*buf);
}

bool RotationBufferProvider::createVaSurface(VideoPayloadBuffer *payload, int transform, bool isTarget)
{
    VAStatus vaStatus;
    VASurfaceAttributeTPI attribTpi;
    VASurfaceAttributeTPI *vaSurfaceAttrib = &attribTpi;
    int stride;
    unsigned long buffers;
    VASurfaceID *surface;
    int width = 0, height = 0, bufferHeight = 0;

    if (isTarget) {
        if (transFromHalToVa(transform) == VA_ROTATION_180) {
            width = payload->width;
            height = payload->height;
        } else {
            width = payload->height;
            height = payload->width;
        }
        mRotatedWidth = width;
        mRotatedHeight = height;
        bufferHeight = (height + 0x1f) & ~0x1f;
        stride = getStride(isTarget, width);
    } else {
        width = payload->width;
        height = payload->height;
        bufferHeight = payload->height;
        stride = payload->luma_stride; /* NV12 srouce buffer */
    }

    if (!stride) {
        ELOGTRACE("invalid stride value");
        return false;
    }

    // adjust source target for Bob deinterlace
    if (!isTarget && mBobDeinterlace) {
        height >>= 1;
        bufferHeight >>= 1;
        stride <<= 1;
    }

    vaSurfaceAttrib->count = 1;
    vaSurfaceAttrib->width = width;
    vaSurfaceAttrib->height = height;
    vaSurfaceAttrib->pixel_format = payload->format;
    vaSurfaceAttrib->type = VAExternalMemoryKernelDRMBufffer;
    vaSurfaceAttrib->tiling = payload->tiling;
    vaSurfaceAttrib->size = (stride * bufferHeight * 3) / 2;
    vaSurfaceAttrib->luma_offset = 0;
    vaSurfaceAttrib->chroma_v_offset = stride * bufferHeight;
    vaSurfaceAttrib->luma_stride = vaSurfaceAttrib->chroma_u_stride
                                 = vaSurfaceAttrib->chroma_v_stride
                                 = stride;
    vaSurfaceAttrib->chroma_u_offset = vaSurfaceAttrib->chroma_v_offset;
    vaSurfaceAttrib->buffers = &buffers;

    if (isTarget) {
        int khandle = createWsbmBuffer(stride, bufferHeight, &mDrmBuf[mTargetIndex]);
        if (khandle == 0) {
            ELOGTRACE("failed to create buffer by wsbm");
            return false;
        }

        mKhandles[mTargetIndex] = khandle;
        vaSurfaceAttrib->buffers[0] = khandle;
        mRotatedStride = stride;
        surface = &mRotatedSurfaces[mTargetIndex];
    } else {
        vaSurfaceAttrib->buffers[0] = payload->khandle;
        surface = &mSourceSurface;
        /* set src surface width/height to video crop size */
        if (payload->crop_width && payload->crop_height) {
            width = payload->crop_width;
            height = (payload->crop_height >> mBobDeinterlace);
        } else {
            VLOGTRACE("Invalid cropping width or height");
            payload->crop_width = width;
            payload->crop_height = height;
        }
    }

    vaStatus = vaCreateSurfacesWithAttribute(mVaDpy,
                                             width,
                                             height,
                                             VA_RT_FORMAT_YUV420,
                                             1,
                                             surface,
                                             vaSurfaceAttrib);
    if (vaStatus != VA_STATUS_SUCCESS) {
        ELOGTRACE("vaCreateSurfacesWithAttribute returns %d", vaStatus);
        ELOGTRACE("Attributes: target: %d, width: %d, height %d, bufferHeight %d, tiling %d",
                isTarget, width, height, bufferHeight, payload->tiling);
        *surface = 0;
        return false;
    }

    return true;
}

bool RotationBufferProvider::startVA(VideoPayloadBuffer *payload, int transform)
{
    bool ret = true;
    VAStatus vaStatus;
    VAEntrypoint *entryPoint;
    VAConfigAttrib attribDummy;
    int numEntryPoints;
    bool supportVideoProcessing = false;
    int majorVer = 0, minorVer = 0;

    // VA will hold a copy of the param pointer, so local varialbe doesn't work
    mVaDpy = vaGetDisplay(&mDisplay);
    if (NULL == mVaDpy) {
        ELOGTRACE("failed to get VADisplay");
        return false;
    }

    vaStatus = vaInitialize(mVaDpy, &majorVer, &minorVer);
    CHECK_VA_STATUS_RETURN("vaInitialize");

    numEntryPoints = vaMaxNumEntrypoints(mVaDpy);

    if (numEntryPoints <= 0) {
        ELOGTRACE("numEntryPoints value is invalid");
        return false;
    }

    entryPoint = (VAEntrypoint*)malloc(sizeof(VAEntrypoint) * numEntryPoints);
    if (NULL == entryPoint) {
        ELOGTRACE("failed to malloc memory for entryPoint");
        return false;
    }

    vaStatus = vaQueryConfigEntrypoints(mVaDpy,
                                        VAProfileNone,
                                        entryPoint,
                                        &numEntryPoints);
    CHECK_VA_STATUS_RETURN("vaQueryConfigEntrypoints");

    for (int i = 0; i < numEntryPoints; i++)
        if (entryPoint[i] == VAEntrypointVideoProc)
            supportVideoProcessing = true;

    free(entryPoint);
    entryPoint = NULL;

    if (!supportVideoProcessing) {
        ELOGTRACE("VAEntrypointVideoProc is not supported");
        return false;
    }

    vaStatus = vaCreateConfig(mVaDpy,
                              VAProfileNone,
                              VAEntrypointVideoProc,
                              &attribDummy,
                              0,
                              &mVaCfg);
    CHECK_VA_STATUS_RETURN("vaCreateConfig");

    // create first target surface
    ret = createVaSurface(payload, transform, true);
    if (ret == false) {
        ELOGTRACE("failed to create target surface with attribute");
        return false;
    }

    vaStatus = vaCreateContext(mVaDpy,
                               mVaCfg,
                               payload->width,
                               payload->height,
                               0,
                               &mRotatedSurfaces[0],
                               1,
                               &mVaCtx);
    CHECK_VA_STATUS_RETURN("vaCreateContext");

    VAProcFilterType filters[VAProcFilterCount];
    unsigned int numFilters = VAProcFilterCount;
    vaStatus = vaQueryVideoProcFilters(mVaDpy, mVaCtx, filters, &numFilters);
    CHECK_VA_STATUS_RETURN("vaQueryVideoProcFilters");

    bool supportVideoProcFilter = false;
    for (unsigned int j = 0; j < numFilters; j++)
        if (filters[j] == VAProcFilterNone)
            supportVideoProcFilter = true;

    if (!supportVideoProcFilter) {
        ELOGTRACE("VAProcFilterNone is not supported");
        return false;
    }

    VAProcFilterParameterBuffer filter;
    filter.type = VAProcFilterNone;
    filter.value = 0;

    vaStatus = vaCreateBuffer(mVaDpy,
                              mVaCtx,
                              VAProcFilterParameterBufferType,
                              sizeof(filter),
                              1,
                              &filter,
                              &mVaBufFilter);
    CHECK_VA_STATUS_RETURN("vaCreateBuffer");

    VAProcPipelineCaps pipelineCaps;
    unsigned int numCaps = 1;
    vaStatus = vaQueryVideoProcPipelineCaps(mVaDpy,
                                            mVaCtx,
                                            &mVaBufFilter,
                                            numCaps,
                                            &pipelineCaps);
    CHECK_VA_STATUS_RETURN("vaQueryVideoProcPipelineCaps");

    if (!(pipelineCaps.rotation_flags & (1 << transFromHalToVa(transform)))) {
        ELOGTRACE("VA_ROTATION_xxx: 0x%08x is not supported by the filter",
             transFromHalToVa(transform));
        return false;
    }

    mBobDeinterlace = payload->bob_deinterlace;
    mVaInitialized = true;

    return true;
}

bool RotationBufferProvider::setupRotationBuffer(VideoPayloadBuffer *payload, int transform)
{
#ifdef DEBUG_ROTATION_PERFROMANCE
    uint32_t setup_Begin = getMilliseconds();
#endif
    VAStatus vaStatus;
    int stride;
    bool ret = false;

    if (payload->format != VA_FOURCC_NV12 || payload->width == 0 || payload->height == 0) {
        WLOGTRACE("payload data is not correct: format %#x, width %d, height %d",
            payload->format, payload->width, payload->height);
        return ret;
    }

    if (payload->width > 1280) {
        payload->tiling = 1;
    }

    do {
        if (isContextChanged(payload->width, payload->height, transform)) {
            DLOGTRACE("VA is restarted as rotation context changes");

            if (mVaInitialized) {
                stopVA(); // need to re-initialize VA for new rotation config
            }
            mTransform = transform;
            mWidth = payload->width;
            mHeight = payload->height;
        }

        if (!mVaInitialized) {
            ret = startVA(payload, transform);
            if (ret == false) {
                vaStatus = VA_STATUS_ERROR_OPERATION_FAILED;
                break;
            }
        }

        // start to create next target surface
        if (!mRotatedSurfaces[mTargetIndex]) {
            ret = createVaSurface(payload, transform, true);
            if (ret == false) {
                ELOGTRACE("failed to create target surface with attribute");
                vaStatus = VA_STATUS_ERROR_OPERATION_FAILED;
                break;
            }
        }

        // create source surface
        ret = createVaSurface(payload, transform, false);
        if (ret == false) {
            ELOGTRACE("failed to create source surface with attribute");
            vaStatus = VA_STATUS_ERROR_OPERATION_FAILED;
            break;
        }

#ifdef DEBUG_ROTATION_PERFROMANCE
        uint32_t beginPicture = getMilliseconds();
#endif
        vaStatus = vaBeginPicture(mVaDpy, mVaCtx, mRotatedSurfaces[mTargetIndex]);
        CHECK_VA_STATUS_BREAK("vaBeginPicture");

        VABufferID pipelineBuf;
        void *p;
        VAProcPipelineParameterBuffer *pipelineParam;
        vaStatus = vaCreateBuffer(mVaDpy,
                                  mVaCtx,
                                  VAProcPipelineParameterBufferType,
                                  sizeof(*pipelineParam),
                                  1,
                                  NULL,
                                  &pipelineBuf);
        CHECK_VA_STATUS_BREAK("vaCreateBuffer");

        vaStatus = vaMapBuffer(mVaDpy, pipelineBuf, &p);
        CHECK_VA_STATUS_BREAK("vaMapBuffer");

        pipelineParam = (VAProcPipelineParameterBuffer*)p;
        pipelineParam->surface = mSourceSurface;
        pipelineParam->rotation_state = transFromHalToVa(transform);
        pipelineParam->filters = &mVaBufFilter;
        pipelineParam->num_filters = 1;
        vaStatus = vaUnmapBuffer(mVaDpy, pipelineBuf);
        CHECK_VA_STATUS_BREAK("vaUnmapBuffer");

        vaStatus = vaRenderPicture(mVaDpy, mVaCtx, &pipelineBuf, 1);
        CHECK_VA_STATUS_BREAK("vaRenderPicture");

        vaStatus = vaEndPicture(mVaDpy, mVaCtx);
        CHECK_VA_STATUS_BREAK("vaEndPicture");

        vaStatus = vaSyncSurface(mVaDpy, mRotatedSurfaces[mTargetIndex]);
        CHECK_VA_STATUS_BREAK("vaSyncSurface");

#ifdef DEBUG_ROTATION_PERFROMANCE
        ILOGTRACE("time spent %dms from vaBeginPicture to vaSyncSurface",
             getMilliseconds() - beginPicture);
#endif

        // Populate payload fields so that overlayPlane can flip the buffer
        payload->rotated_width = mRotatedStride;
        payload->rotated_height = mRotatedHeight;
        payload->rotated_buffer_handle = mKhandles[mTargetIndex];
        // setting client transform to 0 to force re-generating rotated buffer whenever needed.
        payload->client_transform = 0;
        mTargetIndex++;
        if (mTargetIndex >= MAX_SURFACE_NUM)
            mTargetIndex = 0;

    } while (0);

#ifdef DEBUG_ROTATION_PERFROMANCE
    ILOGTRACE("time spent %dms for setupRotationBuffer",
         getMilliseconds() - setup_Begin);
#endif

    if (mSourceSurface > 0) {
        vaStatus = vaDestroySurfaces(mVaDpy, &mSourceSurface, 1);
        if (vaStatus != VA_STATUS_SUCCESS)
            WLOGTRACE("vaDestroySurfaces failed, vaStatus = %d", vaStatus);
        mSourceSurface = 0;
    }

    if (vaStatus != VA_STATUS_SUCCESS) {
        stopVA();
        return false; // To not block HWC, just abort instead of retry
    }

    if (!payload->khandle) {
        WLOGTRACE("khandle is reset by decoder, surface is invalid!");
        return false;
    }

    return true;
}

bool RotationBufferProvider::prepareBufferInfo(int w, int h, int stride, VideoPayloadBuffer *payload, void *user_pt)
{
    int chroma_offset, size;
    void *buf = NULL;

    payload->width = payload->crop_width = w;
    payload->height = payload->crop_height = h;
    payload->format = VA_FOURCC_NV12;
    payload->tiling = 1;
    payload->luma_stride = stride;
    payload->chroma_u_stride = stride;
    payload->chroma_v_stride = stride;
    payload->client_transform = 0;

    chroma_offset = stride * h;
    size = stride * h + stride * h / 2;

    ssize_t index;
    index = mTTMWrappers.indexOfKey((uint64_t)user_pt);
    if (index < 0) {
        VLOGTRACE("wrapped userPt as wsbm buffer");
        bool ret = mWsbm->allocateTTMBufferUB(size, 0, &buf, user_pt);
        if (ret == false) {
            ELOGTRACE("failed to allocate TTM buffer");
            return ret;
        }

        if (mTTMWrappers.size() >= TTM_WRAPPER_COUNT) {
            WLOGTRACE("mTTMWrappers is unexpectedly full. Invalidate caches");
            invalidateCaches();
        }

        index = mTTMWrappers.add((uint64_t)user_pt, buf);
    } else {
        VLOGTRACE("got wsbmBuffer in saved caches");
        buf = mTTMWrappers.valueAt(index);
    }

    payload->khandle = mWsbm->getKBufHandle(buf);
    return true;
}

void RotationBufferProvider::freeVaSurfaces()
{
    bool ret;
    VAStatus vaStatus;

    for (int i = 0; i < MAX_SURFACE_NUM; i++) {
        if (NULL != mDrmBuf[i]) {
            ret = mWsbm->destroyTTMBuffer(mDrmBuf[i]);
            if (!ret)
                WLOGTRACE("failed to free TTMBuffer");
            mDrmBuf[i] = NULL;
        }
    }

    // remove wsbm buffer ref from VA
    for (int j = 0; j < MAX_SURFACE_NUM; j++) {
        if (0 != mRotatedSurfaces[j]) {
            vaStatus = vaDestroySurfaces(mVaDpy, &mRotatedSurfaces[j], 1);
            if (vaStatus != VA_STATUS_SUCCESS)
                WLOGTRACE("vaDestroySurfaces failed, vaStatus = %d", vaStatus);
        }
        mRotatedSurfaces[j] = 0;
    }
}

void RotationBufferProvider::stopVA()
{
    freeVaSurfaces();

    if (0 != mVaBufFilter)
        vaDestroyBuffer(mVaDpy, mVaBufFilter);
    if (0 != mVaCfg)
        vaDestroyConfig(mVaDpy,mVaCfg);
    if (0 != mVaCtx)
        vaDestroyContext(mVaDpy, mVaCtx);
    if (0 != mVaDpy)
        vaTerminate(mVaDpy);

    mVaInitialized = false;

    // reset VA variable
    mVaDpy = 0;
    mVaCfg = 0;
    mVaCtx = 0;
    mVaBufFilter = 0;
    mSourceSurface = 0;

    mWidth = 0;
    mHeight = 0;
    mRotatedWidth = 0;
    mRotatedHeight = 0;
    mRotatedStride = 0;
    mTargetIndex = 0;
}

bool RotationBufferProvider::isContextChanged(int width, int height, int transform)
{
    // check rotation config
    if (height == mHeight &&
        width == mWidth &&
        transform == mTransform) {
        return false;
    }

    return true;
}

} // name space intel
} // name space android
