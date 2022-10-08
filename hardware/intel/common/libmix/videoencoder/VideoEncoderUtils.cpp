/*
* Copyright (c) 2009-2011 Intel Corporation.  All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "VideoEncoderLog.h"
#include "VideoEncoderUtils.h"
#include <va/va_android.h>
#include <va/va_drmcommon.h>

#ifdef IMG_GFX
#include <hal/hal_public.h>
#include <hardware/gralloc.h>

//#define GFX_DUMP

#define OMX_INTEL_COLOR_FormatYUV420PackedSemiPlanar 0x7FA00E00

static hw_module_t const *gModule = NULL;
static gralloc_module_t *gAllocMod = NULL; /* get by force hw_module_t */
static alloc_device_t  *gAllocDev = NULL;

static int gfx_init(void) {

    int err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &gModule);
    if (err) {
        LOG_E("FATAL: can't find the %s module", GRALLOC_HARDWARE_MODULE_ID);
        return -1;
    } else
        LOG_V("hw_get_module returned\n");
    gAllocMod = (gralloc_module_t *)gModule;

    return 0;
}

static int gfx_alloc(uint32_t w, uint32_t h, int format,
          int usage, buffer_handle_t* handle, int32_t* stride) {

    int err;

    if (!gAllocDev) {
        if (!gModule) {
            if (gfx_init()) {
                LOG_E("can't find the %s module", GRALLOC_HARDWARE_MODULE_ID);
                return -1;
            }
        }

        err = gralloc_open(gModule, &gAllocDev);
        if (err) {
            LOG_E("FATAL: gralloc open failed\n");
            return -1;
        }
    }

    err = gAllocDev->alloc(gAllocDev, w, h, format, usage, handle, stride);
    if (err) {
        LOG_E("alloc(%u, %u, %d, %08x, ...) failed %d (%s)\n",
               w, h, format, usage, err, strerror(-err));
    }

    return err;
}

static int gfx_free(buffer_handle_t handle) {

    int err;

    if (!gAllocDev) {
        if (!gModule) {
            if (gfx_init()) {
                LOG_E("can't find the %s module", GRALLOC_HARDWARE_MODULE_ID);
                return -1;
            }
        }

        err = gralloc_open(gModule, &gAllocDev);
        if (err) {
            LOG_E("FATAL: gralloc open failed\n");
            return -1;
        }
    }

    err = gAllocDev->free(gAllocDev, handle);
    if (err) {
        LOG_E("free(...) failed %d (%s)\n", err, strerror(-err));
    }

    return err;
}

static int gfx_lock(buffer_handle_t handle, int usage,
                      int left, int top, int width, int height, void** vaddr) {

    int err;

    if (!gAllocMod) {
        if (gfx_init()) {
            LOG_E("can't find the %s module", GRALLOC_HARDWARE_MODULE_ID);
            return -1;
        }
    }

    err = gAllocMod->lock(gAllocMod, handle, usage,
                          left, top, width, height, vaddr);
    LOG_V("gfx_lock: handle is %x, usage is %x, vaddr is %x.\n", (unsigned int)handle, usage, (unsigned int)*vaddr);

    if (err){
        LOG_E("lock(...) failed %d (%s).\n", err, strerror(-err));
        return -1;
    } else
        LOG_V("lock returned with address %p\n", *vaddr);

    return err;
}

static int gfx_unlock(buffer_handle_t handle) {

    int err;

    if (!gAllocMod) {
        if (gfx_init()) {
            LOG_E("can't find the %s module", GRALLOC_HARDWARE_MODULE_ID);
            return -1;
        }
    }

    err = gAllocMod->unlock(gAllocMod, handle);
    if (err) {
        LOG_E("unlock(...) failed %d (%s)", err, strerror(-err));
        return -1;
    } else
        LOG_V("unlock returned\n");

    return err;
}

static int gfx_Blit(buffer_handle_t src, buffer_handle_t dest,
                      int w, int h, int , int )
{
    int err;

    if (!gAllocMod) {
        if (gfx_init()) {
            LOG_E("can't find the %s module", GRALLOC_HARDWARE_MODULE_ID);
            return -1;
        }
    }

    IMG_gralloc_module_public_t* GrallocMod = (IMG_gralloc_module_public_t*)gModule;

#ifdef MRFLD_GFX
    err = GrallocMod->Blit(GrallocMod, src, dest, w, h, 0, 0, 0, 0);
#else
    err = GrallocMod->Blit2(GrallocMod, src, dest, w, h, 0, 0);
#endif

    if (err) {
        LOG_E("Blit(...) failed %d (%s)", err, strerror(-err));
        return -1;
    } else
        LOG_V("Blit returned\n");

    return err;
}

Encode_Status GetGfxBufferInfo(intptr_t handle, ValueInfo& vinfo){

    /* only support OMX_INTEL_COLOR_FormatYUV420PackedSemiPlanar
                                HAL_PIXEL_FORMAT_NV12
                                HAL_PIXEL_FORMAT_BGRA_8888
                                HAL_PIXEL_FORMAT_RGBA_8888
                                HAL_PIXEL_FORMAT_RGBX_8888
                                HAL_PIXEL_FORMAT_BGRX_8888 */
    IMG_native_handle_t* h = (IMG_native_handle_t*) handle;

    vinfo.width = h->iWidth;
    vinfo.height = h->iHeight;
    vinfo.lumaStride = h->iWidth;

    LOG_V("GetGfxBufferInfo: gfx iWidth=%d, iHeight=%d, iFormat=%x in handle structure\n", h->iWidth, h->iHeight, h->iFormat);

    if (h->iFormat == HAL_PIXEL_FORMAT_NV12) {
    #ifdef MRFLD_GFX
        if((h->usage & GRALLOC_USAGE_HW_CAMERA_READ) || (h->usage & GRALLOC_USAGE_HW_CAMERA_WRITE) )
            vinfo.lumaStride = (h->iWidth + 63) & ~63; //64 aligned
        else
            vinfo.lumaStride = (h->iWidth + 31) & ~31; //32 aligned
    #else //on CTP
        if (h->iWidth > 512)
            vinfo.lumaStride = (h->iWidth + 63) & ~63;  //64 aligned
        else
            vinfo.lumaStride = 512;
    #endif
    } else if ((h->iFormat == HAL_PIXEL_FORMAT_BGRA_8888)||
                  (h->iFormat == HAL_PIXEL_FORMAT_RGBA_8888)||
                  (h->iFormat == HAL_PIXEL_FORMAT_RGBX_8888)||
                  (h->iFormat == HAL_PIXEL_FORMAT_BGRX_8888)) {
        vinfo.lumaStride = (h->iWidth + 31) & ~31;
    } else if (h->iFormat == OMX_INTEL_COLOR_FormatYUV420PackedSemiPlanar) {
        //nothing to do
    } else
        return ENCODE_NOT_SUPPORTED;

    vinfo.format = h->iFormat;

    LOG_V("Actual Width=%d, Height=%d, Stride=%d\n\n", vinfo.width, vinfo.height, vinfo.lumaStride);
    return ENCODE_SUCCESS;
}

#ifdef GFX_DUMP
void DumpGfx(intptr_t handle, char* filename) {
    ValueInfo vinfo;
    void* vaddr[3];
    FILE* fp;
    int usage = GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_SW_READ_OFTEN;

    GetGfxBufferInfo(handle, vinfo);
    if (gfx_lock((buffer_handle_t)handle, usage, 0, 0, vinfo.width, vinfo.height, &vaddr[0]) != 0)
        return ENCODE_DRIVER_FAIL;
    fp = fopen(filename, "wb");
    fwrite(vaddr[0], 1, vinfo.lumaStride * vinfo.height * 4, fp);
    fclose(fp);
    LOG_I("dump %d bytes data to %s\n", vinfo.lumaStride * vinfo.height * 4, filename);
    gfx_unlock((buffer_handle_t)handle);

    return;
}
#endif

#endif

extern "C" {
VAStatus vaLockSurface(VADisplay dpy,
    VASurfaceID surface,
    unsigned int *fourcc,
    unsigned int *luma_stride,
    unsigned int *chroma_u_stride,
    unsigned int *chroma_v_stride,
    unsigned int *luma_offset,
    unsigned int *chroma_u_offset,
    unsigned int *chroma_v_offset,
    unsigned int *buffer_name,
    void **buffer
);

VAStatus vaUnlockSurface(VADisplay dpy,
    VASurfaceID surface
);
}

VASurfaceMap::VASurfaceMap(VADisplay display, int hwcap) {

    mVADisplay = display;
    mSupportedSurfaceMemType = hwcap;
    mValue = 0;
    mVASurface = VA_INVALID_SURFACE;
    mTracked = false;
    mAction = 0;
    memset(&mVinfo, 0, sizeof(ValueInfo));
#ifdef IMG_GFX
    mGfxHandle = NULL;
#endif
}

VASurfaceMap::~VASurfaceMap() {

    if (!mTracked && (mVASurface != VA_INVALID_SURFACE))
        vaDestroySurfaces(mVADisplay, &mVASurface, 1);

#ifdef IMG_GFX
    if (mGfxHandle)
        gfx_free(mGfxHandle);
#endif
}

Encode_Status VASurfaceMap::doMapping() {

    Encode_Status ret = ENCODE_SUCCESS;

    if (mVASurface == VA_INVALID_SURFACE) {

        int width = mVASurfaceWidth = mVinfo.width;
        int height = mVASurfaceHeight = mVinfo.height;
        int stride = mVASurfaceStride = mVinfo.lumaStride;

        if (mAction & MAP_ACTION_COLORCONVERT) {

            //only support gfx buffer
            if (mVinfo.mode != MEM_MODE_GFXHANDLE)
                return ENCODE_NOT_SUPPORTED;

        #ifdef IMG_GFX //only enable on IMG chip

            //do not trust valueinfo for gfx case, directly get from structure
            ValueInfo tmp;

            ret = GetGfxBufferInfo(mValue, tmp);
            CHECK_ENCODE_STATUS_RETURN("GetGfxBufferInfo");
            width = tmp.width;
            height = tmp.height;
            stride = tmp.lumaStride;

            if (HAL_PIXEL_FORMAT_NV12 == tmp.format || OMX_INTEL_COLOR_FormatYUV420PackedSemiPlanar == tmp.format)
                mAction &= ~MAP_ACTION_COLORCONVERT;
            else {
                //allocate new gfx buffer if format is not NV12
                int usage = GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_TEXTURE;

                //use same size with original and HAL_PIXEL_FORMAT_NV12 format
                if (gfx_alloc(width, height, HAL_PIXEL_FORMAT_NV12, usage, &mGfxHandle, &stride) != 0)
                    return ENCODE_DRIVER_FAIL;

                LOG_V("Create an new gfx buffer handle 0x%p for color convert, width=%d, height=%d, stride=%d\n",
                           mGfxHandle, width, height, stride);
            }

        #else
            return ENCODE_NOT_SUPPORTED;
        #endif
        }

        if (mAction & MAP_ACTION_ALIGN64 && stride % 64 != 0) {
            //check if stride is not 64 aligned, must allocate new 64 aligned vasurface
            stride = (stride + 63 ) & ~63;
            mAction |= MAP_ACTION_COPY;
        }

        if(mAction & MAP_ACTION_ALIGN64 && width <= 320 && height <= 240) {
            mAction |= MAP_ACTION_COPY;
        }

        if (mAction & MAP_ACTION_COPY) { //must allocate new vasurface(EXternalMemoryNULL, uncached)
            //allocate new vasurface
            mVASurface = CreateNewVASurface(mVADisplay, stride, height);
            if (mVASurface == VA_INVALID_SURFACE)
                return ENCODE_DRIVER_FAIL;
            mVASurfaceWidth = mVASurfaceStride = stride;
            mVASurfaceHeight = height;
            LOGI("create new vaSurface for MAP_ACTION_COPY\n");
        } else {
        #ifdef IMG_GFX
            if (mGfxHandle != NULL) {
                //map new gfx handle to vasurface
                ret = MappingGfxHandle((intptr_t)mGfxHandle);
                CHECK_ENCODE_STATUS_RETURN("MappingGfxHandle");
                LOGV("map new allocated gfx handle to vaSurface\n");
            } else
        #endif
            {
                //map original value to vasurface
                ret = MappingToVASurface();
                CHECK_ENCODE_STATUS_RETURN("MappingToVASurface");
            }
        }
    }

    if (mAction & MAP_ACTION_COLORCONVERT) {
        ret = doActionColConv();
        CHECK_ENCODE_STATUS_RETURN("doActionColConv");
    }

    if (mAction & MAP_ACTION_COPY) {
        //keep src color format is NV12, then do copy
        ret = doActionCopy();
        CHECK_ENCODE_STATUS_RETURN("doActionCopy");
    }

    return ENCODE_SUCCESS;
}

Encode_Status VASurfaceMap::MappingToVASurface() {

    Encode_Status ret = ENCODE_SUCCESS;

    if (mVASurface != VA_INVALID_SURFACE) {
        LOG_I("VASurface is already set before, nothing to do here\n");
        return ENCODE_SUCCESS;
    }
    LOG_V("MappingToVASurface mode=%d, value=%p\n", mVinfo.mode, (void*)mValue);

    const char *mode = NULL;
    switch (mVinfo.mode) {
        case MEM_MODE_SURFACE:
            mode = "SURFACE";
            ret = MappingSurfaceID(mValue);
            break;
        case MEM_MODE_GFXHANDLE:
            mode = "GFXHANDLE";
            ret = MappingGfxHandle(mValue);
            break;
        case MEM_MODE_KBUFHANDLE:
            mode = "KBUFHANDLE";
            ret = MappingKbufHandle(mValue);
            break;
        case MEM_MODE_MALLOC:
        case MEM_MODE_NONECACHE_USRPTR:
            mode = "MALLOC or NONCACHE_USRPTR";
            ret = MappingMallocPTR(mValue);
            break;
        case MEM_MODE_ION:
        case MEM_MODE_V4L2:
        case MEM_MODE_USRPTR:
        case MEM_MODE_CI:
        default:
            LOG_I("UnSupported memory mode 0x%08x", mVinfo.mode);
            return ENCODE_NOT_SUPPORTED;
    }

    LOG_V("%s: Format=%x, lumaStride=%d, width=%d, height=%d\n", mode, mVinfo.format, mVinfo.lumaStride, mVinfo.width, mVinfo.height);
    LOG_V("vaSurface 0x%08x is created for value = 0x%p\n", mVASurface, (void*)mValue);

    return ret;
}

Encode_Status VASurfaceMap::MappingSurfaceID(intptr_t value) {

    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VASurfaceID surface;

    //try to get kbufhandle from SurfaceID
    uint32_t fourCC = 0;
    uint32_t lumaStride = 0;
    uint32_t chromaUStride = 0;
    uint32_t chromaVStride = 0;
    uint32_t lumaOffset = 0;
    uint32_t chromaUOffset = 0;
    uint32_t chromaVOffset = 0;
    uint32_t kBufHandle = 0;

    vaStatus = vaLockSurface(
            (VADisplay)mVinfo.handle, (VASurfaceID)value,
            &fourCC, &lumaStride, &chromaUStride, &chromaVStride,
            &lumaOffset, &chromaUOffset, &chromaVOffset, &kBufHandle, NULL);

    CHECK_VA_STATUS_RETURN("vaLockSurface");
    LOG_V("Surface incoming = 0x%p\n", (void*)value);
    LOG_V("lumaStride = %d, chromaUStride = %d, chromaVStride=%d\n", lumaStride, chromaUStride, chromaVStride);
    LOG_V("lumaOffset = %d, chromaUOffset = %d, chromaVOffset = %d\n", lumaOffset, chromaUOffset, chromaVOffset);
    LOG_V("kBufHandle = 0x%08x, fourCC = %d\n", kBufHandle, fourCC);

    vaStatus = vaUnlockSurface((VADisplay)mVinfo.handle, (VASurfaceID)value);
    CHECK_VA_STATUS_RETURN("vaUnlockSurface");

    mVinfo.mode = MEM_MODE_KBUFHANDLE;
    mVinfo.size = mVinfo.lumaStride * mVinfo.height * 1.5;

    mVASurface = CreateSurfaceFromExternalBuf(kBufHandle, mVinfo);
    if (mVASurface == VA_INVALID_SURFACE)
        return ENCODE_INVALID_SURFACE;

    mVASurfaceWidth = mVinfo.width;
    mVASurfaceHeight = mVinfo.height;
    mVASurfaceStride = mVinfo.lumaStride;
    return ENCODE_SUCCESS;
}

Encode_Status VASurfaceMap::MappingGfxHandle(intptr_t value) {

    LOG_V("MappingGfxHandle %p......\n", (void*)value);
    LOG_V("format = 0x%08x, lumaStride = %d in ValueInfo\n", mVinfo.format, mVinfo.lumaStride);

    //default value for all HW platforms, maybe not accurate
    mVASurfaceWidth = mVinfo.width;
    mVASurfaceHeight = mVinfo.height;
    mVASurfaceStride = mVinfo.lumaStride;

#ifdef IMG_GFX
    Encode_Status ret;
    ValueInfo tmp;

    ret = GetGfxBufferInfo(value, tmp);
    CHECK_ENCODE_STATUS_RETURN("GetGfxBufferInfo");
    mVASurfaceWidth = tmp.width;
    mVASurfaceHeight = tmp.height;
    mVASurfaceStride = tmp.lumaStride;
#endif

    LOG_V("Mapping vasurface Width=%d, Height=%d, Stride=%d\n", mVASurfaceWidth, mVASurfaceHeight, mVASurfaceStride);

    ValueInfo vinfo;
    memset(&vinfo, 0, sizeof(ValueInfo));
    vinfo.mode = MEM_MODE_GFXHANDLE;
    vinfo.width = mVASurfaceWidth;
    vinfo.height = mVASurfaceHeight;
    vinfo.lumaStride = mVASurfaceStride;
    mVASurface = CreateSurfaceFromExternalBuf(value, vinfo);
    if (mVASurface == VA_INVALID_SURFACE)
        return ENCODE_INVALID_SURFACE;

    return ENCODE_SUCCESS;
}

Encode_Status VASurfaceMap::MappingKbufHandle(intptr_t value) {

    LOG_V("MappingKbufHandle value=%p\n", (void*)value);

    mVinfo.size = mVinfo.lumaStride * mVinfo.height * 1.5;
    mVASurface = CreateSurfaceFromExternalBuf(value, mVinfo);
    if (mVASurface == VA_INVALID_SURFACE)
        return ENCODE_INVALID_SURFACE;

    mVASurfaceWidth = mVinfo.width;
    mVASurfaceHeight = mVinfo.height;
    mVASurfaceStride = mVinfo.lumaStride;

    return ENCODE_SUCCESS;
}

Encode_Status VASurfaceMap::MappingMallocPTR(intptr_t value) {

    mVASurface = CreateSurfaceFromExternalBuf(value, mVinfo);
    if (mVASurface == VA_INVALID_SURFACE)
        return ENCODE_INVALID_SURFACE;

    mVASurfaceWidth = mVinfo.width;
    mVASurfaceHeight = mVinfo.height;
    mVASurfaceStride = mVinfo.lumaStride;

    return ENCODE_SUCCESS;
}

//always copy with same color format NV12
Encode_Status VASurfaceMap::doActionCopy() {

    VAStatus vaStatus = VA_STATUS_SUCCESS;

    uint32_t width = 0, height = 0, stride = 0;
    uint8_t *pSrcBuffer, *pDestBuffer;
    intptr_t handle = 0;

    LOG_V("Copying Src Buffer data to VASurface\n");

    if (mVinfo.mode != MEM_MODE_MALLOC && mVinfo.mode != MEM_MODE_GFXHANDLE) {
        LOG_E("Not support copy in mode %d", mVinfo.mode);
        return ENCODE_NOT_SUPPORTED;
    }

    LOG_V("Src Buffer information\n");
    LOG_V("Mode = %d, width = %d, stride = %d, height = %d\n",
           mVinfo.mode, mVinfo.width, mVinfo.lumaStride, mVinfo.height);

    uint32_t srcY_offset, srcUV_offset;
    uint32_t srcY_pitch, srcUV_pitch;

    if (mVinfo.mode == MEM_MODE_MALLOC) {
        width = mVinfo.width;
        height = mVinfo.height;
        stride = mVinfo.lumaStride;
        pSrcBuffer = (uint8_t*) mValue;
        srcY_offset = 0;
        srcUV_offset = stride * height;
        srcY_pitch = stride;
        srcUV_pitch = stride;
    } else {

    #ifdef IMG_GFX  //only enable on IMG chips
        int usage = GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_SW_READ_OFTEN;

        //do not trust valueinfo, directly get from structure
        Encode_Status ret;
        ValueInfo tmp;

        if (mGfxHandle)
            handle = (intptr_t) mGfxHandle;
        else
            handle = mValue;

        ret = GetGfxBufferInfo(handle, tmp);
        CHECK_ENCODE_STATUS_RETURN("GetGfxBufferInfo");
        width = tmp.width;
        height = tmp.height;
        stride = tmp.lumaStride;

        //only support HAL_PIXEL_FORMAT_NV12 & OMX_INTEL_COLOR_FormatYUV420PackedSemiPlanar
        if (HAL_PIXEL_FORMAT_NV12 != tmp.format && OMX_INTEL_COLOR_FormatYUV420PackedSemiPlanar != tmp.format) {
            LOG_E("Not support gfx buffer format %x", tmp.format);
            return ENCODE_NOT_SUPPORTED;
        }

        srcY_offset = 0;
        srcUV_offset = stride * height;
        srcY_pitch = stride;
        srcUV_pitch = stride;

        //lock gfx handle with buffer real size
        void* vaddr[3];
        if (gfx_lock((buffer_handle_t) handle, usage, 0, 0, width, height, &vaddr[0]) != 0)
            return ENCODE_DRIVER_FAIL;
        pSrcBuffer = (uint8_t*)vaddr[0];
    #else

        return ENCODE_NOT_SUPPORTED;
    #endif
    }


    VAImage destImage;
    vaStatus = vaDeriveImage(mVADisplay, mVASurface, &destImage);
    CHECK_VA_STATUS_RETURN("vaDeriveImage");
    vaStatus = vaMapBuffer(mVADisplay, destImage.buf, (void **)&pDestBuffer);
    CHECK_VA_STATUS_RETURN("vaMapBuffer");

    LOG_V("\nDest VASurface information\n");
    LOG_V("pitches[0] = %d\n", destImage.pitches[0]);
    LOG_V("pitches[1] = %d\n", destImage.pitches[1]);
    LOG_V("offsets[0] = %d\n", destImage.offsets[0]);
    LOG_V("offsets[1] = %d\n", destImage.offsets[1]);
    LOG_V("num_planes = %d\n", destImage.num_planes);
    LOG_V("width = %d\n", destImage.width);
    LOG_V("height = %d\n", destImage.height);

    if (width > destImage.width || height > destImage.height) {
        LOG_E("src buffer is bigger than destination buffer\n");
        return ENCODE_INVALID_PARAMS;
    }

    uint8_t *srcY, *dstY;
    uint8_t *srcU, *srcV;
    uint8_t *srcUV, *dstUV;

    srcY = pSrcBuffer + srcY_offset;
    dstY = pDestBuffer + destImage.offsets[0];
    srcUV = pSrcBuffer + srcUV_offset;
    dstUV = pDestBuffer + destImage.offsets[1];

    for (uint32_t i = 0; i < height; i++) {
        memcpy(dstY, srcY, width);
        srcY += srcY_pitch;
        dstY += destImage.pitches[0];
    }

    for (uint32_t i = 0; i < height / 2; i++) {
        memcpy(dstUV, srcUV, width);
        srcUV += srcUV_pitch;
        dstUV += destImage.pitches[1];
    }

    vaStatus = vaUnmapBuffer(mVADisplay, destImage.buf);
    CHECK_VA_STATUS_RETURN("vaUnmapBuffer");
    vaStatus = vaDestroyImage(mVADisplay, destImage.image_id);
    CHECK_VA_STATUS_RETURN("vaDestroyImage");

#ifdef IMG_GFX
    if (mVinfo.mode == MEM_MODE_GFXHANDLE) {
        //unlock gfx handle
        gfx_unlock((buffer_handle_t) handle);
    }
#endif
    LOG_V("Copying Src Buffer data to VASurface Complete\n");

    return ENCODE_SUCCESS;
}

Encode_Status VASurfaceMap::doActionColConv() {

#ifdef IMG_GFX
    if (mGfxHandle == NULL) {
        LOG_E("something wrong, why new gfxhandle is not allocated? \n");
        return ENCODE_FAIL;
    }

    LOG_V("doActionColConv gfx_Blit width=%d, height=%d\n", mVinfo.width, mVinfo.height);
    if (gfx_Blit((buffer_handle_t)mValue, mGfxHandle,
            mVinfo.width, mVinfo.height, 0, 0) != 0)
        return ENCODE_DRIVER_FAIL;

  #ifdef GFX_DUMP
    LOG_I("dumpping gfx data.....\n");
    DumpGfx(mValue, "/data/dump.rgb");
    DumpGfx((intptr_t)mGfxHandle, "/data/dump.yuv");
  #endif
    return ENCODE_SUCCESS;

#else
    return ENCODE_NOT_SUPPORTED;
#endif
}

VASurfaceID VASurfaceMap::CreateSurfaceFromExternalBuf(intptr_t value, ValueInfo& vinfo) {

    VAStatus vaStatus;
    VASurfaceAttribExternalBuffers extbuf;
    VASurfaceAttrib attribs[2];
    VASurfaceID surface = VA_INVALID_SURFACE;
    int type;
    unsigned long data = value;

    extbuf.pixel_format = VA_FOURCC_NV12;
    extbuf.width = vinfo.width;
    extbuf.height = vinfo.height;
    extbuf.data_size = vinfo.size;
    if (extbuf.data_size == 0)
        extbuf.data_size = vinfo.lumaStride * vinfo.height * 1.5;
    extbuf.num_buffers = 1;
    extbuf.num_planes = 3;
    extbuf.pitches[0] = vinfo.lumaStride;
    extbuf.pitches[1] = vinfo.lumaStride;
    extbuf.pitches[2] = vinfo.lumaStride;
    extbuf.pitches[3] = 0;
    extbuf.offsets[0] = 0;
    extbuf.offsets[1] = vinfo.lumaStride * vinfo.height;
    extbuf.offsets[2] = extbuf.offsets[1];
    extbuf.offsets[3] = 0;
    extbuf.buffers = &data;
    extbuf.flags = 0;
    extbuf.private_data = NULL;

    switch(vinfo.mode) {
        case MEM_MODE_GFXHANDLE:
            type = VA_SURFACE_ATTRIB_MEM_TYPE_ANDROID_GRALLOC;
            break;
        case MEM_MODE_KBUFHANDLE:
            type = VA_SURFACE_ATTRIB_MEM_TYPE_KERNEL_DRM;
            break;
        case MEM_MODE_MALLOC:
            type = VA_SURFACE_ATTRIB_MEM_TYPE_USER_PTR;
            break;
        case MEM_MODE_NONECACHE_USRPTR:
            type = VA_SURFACE_ATTRIB_MEM_TYPE_USER_PTR;
            extbuf.flags |= VA_SURFACE_EXTBUF_DESC_UNCACHED;
            break;
        case MEM_MODE_SURFACE:
        case MEM_MODE_ION:
        case MEM_MODE_V4L2:
        case MEM_MODE_USRPTR:
        case MEM_MODE_CI:
        default:
            //not support
            return VA_INVALID_SURFACE;
    }

    if (!(mSupportedSurfaceMemType & type))
        return VA_INVALID_SURFACE;

    attribs[0].type = (VASurfaceAttribType)VASurfaceAttribMemoryType;
    attribs[0].flags = VA_SURFACE_ATTRIB_SETTABLE;
    attribs[0].value.type = VAGenericValueTypeInteger;
    attribs[0].value.value.i = type;

    attribs[1].type = (VASurfaceAttribType)VASurfaceAttribExternalBufferDescriptor;
    attribs[1].flags = VA_SURFACE_ATTRIB_SETTABLE;
    attribs[1].value.type = VAGenericValueTypePointer;
    attribs[1].value.value.p = (void *)&extbuf;

    vaStatus = vaCreateSurfaces(mVADisplay, VA_RT_FORMAT_YUV420, vinfo.width,
                                 vinfo.height, &surface, 1, attribs, 2);
    if (vaStatus != VA_STATUS_SUCCESS){
        LOG_E("vaCreateSurfaces failed. vaStatus = %d\n", vaStatus);
        surface = VA_INVALID_SURFACE;
    }
    return surface;
}

VASurfaceID CreateNewVASurface(VADisplay display, int32_t width, int32_t height) {

    VAStatus vaStatus;
    VASurfaceID surface = VA_INVALID_SURFACE;
    VASurfaceAttrib attribs[2];
    VASurfaceAttribExternalBuffers extbuf;
    unsigned long data;

    extbuf.pixel_format = VA_FOURCC_NV12;
    extbuf.width = width;
    extbuf.height = height;
    extbuf.data_size = width * height * 3 / 2;
    extbuf.num_buffers = 1;
    extbuf.num_planes = 3;
    extbuf.pitches[0] = width;
    extbuf.pitches[1] = width;
    extbuf.pitches[2] = width;
    extbuf.pitches[3] = 0;
    extbuf.offsets[0] = 0;
    extbuf.offsets[1] = width * height;
    extbuf.offsets[2] = extbuf.offsets[1];
    extbuf.offsets[3] = 0;
    extbuf.buffers = &data;
    extbuf.flags = 0;
    extbuf.private_data = NULL;

    attribs[0].type = (VASurfaceAttribType)VASurfaceAttribMemoryType;
    attribs[0].flags = VA_SURFACE_ATTRIB_SETTABLE;
    attribs[0].value.type = VAGenericValueTypeInteger;
    attribs[0].value.value.i = VA_SURFACE_ATTRIB_MEM_TYPE_VA;

    attribs[1].type = (VASurfaceAttribType)VASurfaceAttribExternalBufferDescriptor;
    attribs[1].flags = VA_SURFACE_ATTRIB_SETTABLE;
    attribs[1].value.type = VAGenericValueTypePointer;
    attribs[1].value.value.p = (void *)&extbuf;

    vaStatus = vaCreateSurfaces(display, VA_RT_FORMAT_YUV420, width,
                                 height, &surface, 1, attribs, 2);
    if (vaStatus != VA_STATUS_SUCCESS)
        LOG_E("vaCreateSurfaces failed. vaStatus = %d\n", vaStatus);

    return surface;
}
