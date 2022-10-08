/*
 * Copyright (c) 2012-2014, Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Linux Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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

#ifndef LOG_TAG
#define LOG_TAG "qsfdump"
#endif
#define LOG_NDEBUG 0
#include <hwc_utils.h>
#include <hwc_dump_layers.h>
#include <cutils/log.h>
#include <sys/stat.h>
#include <comptype.h>
#ifdef QCOM_BSP
#include <SkBitmap.h>
#include <SkImageEncoder.h>
#endif
#ifdef STDC_FORMAT_MACROS
#include <inttypes.h>
#endif

namespace qhwc {

// MAX_ALLOWED_FRAMEDUMPS must be capped to (LONG_MAX - 1)
// 60fps => 216000 frames per hour
// Below setting of 216000 * 24 * 7 => 1 week or 168 hours of capture.
  enum {
    MAX_ALLOWED_FRAMEDUMPS = (216000 * 24 * 7)
  };

bool HwcDebug::sDumpEnable = false;

HwcDebug::HwcDebug(uint32_t dpy):
  mDumpCntLimRaw(0),
  mDumpCntrRaw(1),
  mDumpCntLimPng(0),
  mDumpCntrPng(1),
  mDpy(dpy) {
    char dumpPropStr[PROPERTY_VALUE_MAX];
    if(mDpy) {
        strlcpy(mDisplayName, "external", sizeof(mDisplayName));
    } else {
        strlcpy(mDisplayName, "primary", sizeof(mDisplayName));
    }
    snprintf(mDumpPropKeyDisplayType, sizeof(mDumpPropKeyDisplayType),
             "debug.sf.dump.%s", (char *)mDisplayName);

    if ((property_get("debug.sf.dump.enable", dumpPropStr, NULL) > 0)) {
        if(!strncmp(dumpPropStr, "true", strlen("true"))) {
            sDumpEnable = true;
        }
    }
}

void HwcDebug::dumpLayers(hwc_display_contents_1_t* list)
{
    // Check need for dumping layers for debugging.
    if (UNLIKELY(sDumpEnable) && UNLIKELY(needToDumpLayers()) && LIKELY(list)) {
        logHwcProps(list->flags);
        for (size_t i = 0; i < list->numHwLayers; i++) {
            logLayer(i, list->hwLayers);
            dumpLayer(i, list->hwLayers);
        }
    }
}

bool HwcDebug::needToDumpLayers()
{
    bool bDumpLayer = false;
    char dumpPropStr[PROPERTY_VALUE_MAX];
    // Enable primary dump and disable external dump by default.
    bool bDumpEnable = !mDpy;
    time_t timeNow;
    tm dumpTime;

    // Override the bDumpEnable based on the property value, if the property
    // is present in the build.prop file.
    if ((property_get(mDumpPropKeyDisplayType, dumpPropStr, NULL) > 0)) {
        if(!strncmp(dumpPropStr, "true", strlen("true")))
            bDumpEnable = true;
        else
            bDumpEnable = false;
    }

    if (false == bDumpEnable)
        return false;

    time(&timeNow);
    localtime_r(&timeNow, &dumpTime);

    if ((property_get("debug.sf.dump.png", dumpPropStr, NULL) > 0) &&
            (strncmp(dumpPropStr, mDumpPropStrPng, PROPERTY_VALUE_MAX - 1))) {
        // Strings exist & not equal implies it has changed, so trigger a dump
        strlcpy(mDumpPropStrPng, dumpPropStr, sizeof(mDumpPropStrPng));
        mDumpCntLimPng = atoi(dumpPropStr);
        if (mDumpCntLimPng > MAX_ALLOWED_FRAMEDUMPS) {
            ALOGW("Warning: Using debug.sf.dump.png %d (= max)",
                MAX_ALLOWED_FRAMEDUMPS);
            mDumpCntLimPng = MAX_ALLOWED_FRAMEDUMPS;
        }
        mDumpCntLimPng = (mDumpCntLimPng < 0) ? 0: mDumpCntLimPng;
        if (mDumpCntLimPng) {
            snprintf(mDumpDirPng, sizeof(mDumpDirPng),
                    "/data/sfdump.png.%04d.%02d.%02d.%02d.%02d.%02d",
                    dumpTime.tm_year + 1900, dumpTime.tm_mon + 1,
                    dumpTime.tm_mday, dumpTime.tm_hour,
                    dumpTime.tm_min, dumpTime.tm_sec);
            if (0 == mkdir(mDumpDirPng, 0777))
                mDumpCntrPng = 0;
            else {
                ALOGE("Error: %s. Failed to create sfdump directory: %s",
                    strerror(errno), mDumpDirPng);
                mDumpCntrPng = mDumpCntLimPng + 1;
            }
        }
    }

    if (mDumpCntrPng <= mDumpCntLimPng)
        mDumpCntrPng++;

    if ((property_get("debug.sf.dump", dumpPropStr, NULL) > 0) &&
            (strncmp(dumpPropStr, mDumpPropStrRaw, PROPERTY_VALUE_MAX - 1))) {
        // Strings exist & not equal implies it has changed, so trigger a dump
        strlcpy(mDumpPropStrRaw, dumpPropStr, sizeof(mDumpPropStrRaw));
        mDumpCntLimRaw = atoi(dumpPropStr);
        if (mDumpCntLimRaw > MAX_ALLOWED_FRAMEDUMPS) {
            ALOGW("Warning: Using debug.sf.dump %d (= max)",
                MAX_ALLOWED_FRAMEDUMPS);
            mDumpCntLimRaw = MAX_ALLOWED_FRAMEDUMPS;
        }
        mDumpCntLimRaw = (mDumpCntLimRaw < 0) ? 0: mDumpCntLimRaw;
        if (mDumpCntLimRaw) {
            snprintf(mDumpDirRaw, sizeof(mDumpDirRaw),
                    "/data/sfdump.raw.%04d.%02d.%02d.%02d.%02d.%02d",
                    dumpTime.tm_year + 1900, dumpTime.tm_mon + 1,
                    dumpTime.tm_mday, dumpTime.tm_hour,
                    dumpTime.tm_min, dumpTime.tm_sec);
            if (0 == mkdir(mDumpDirRaw, 0777))
                mDumpCntrRaw = 0;
            else {
                ALOGE("Error: %s. Failed to create sfdump directory: %s",
                    strerror(errno), mDumpDirRaw);
                mDumpCntrRaw = mDumpCntLimRaw + 1;
            }
        }
    }

    if (mDumpCntrRaw <= mDumpCntLimRaw)
        mDumpCntrRaw++;

    bDumpLayer = (mDumpCntLimPng || mDumpCntLimRaw)? true : false;
    return bDumpLayer;
}

void HwcDebug::logHwcProps(uint32_t listFlags)
{
    static int hwcModuleCompType = -1;
    static int sMdpCompMaxLayers = 0;
    static String8 hwcModuleCompTypeLog("");
    if (-1 == hwcModuleCompType) {
        // One time stuff
        char mdpCompPropStr[PROPERTY_VALUE_MAX];
        if (property_get("debug.mdpcomp.maxlayer", mdpCompPropStr, NULL) > 0) {
            sMdpCompMaxLayers = atoi(mdpCompPropStr);
        }
        hwcModuleCompType =
            qdutils::QCCompositionType::getInstance().getCompositionType();
        hwcModuleCompTypeLog.appendFormat("%s%s%s%s%s%s",
            // Is hwc module composition type now a bit-field?!
            (hwcModuleCompType == qdutils::COMPOSITION_TYPE_GPU)?
                "[GPU]": "",
            (hwcModuleCompType & qdutils::COMPOSITION_TYPE_MDP)?
                "[MDP]": "",
            (hwcModuleCompType & qdutils::COMPOSITION_TYPE_C2D)?
                "[C2D]": "",
            (hwcModuleCompType & qdutils::COMPOSITION_TYPE_CPU)?
                "[CPU]": "",
            (hwcModuleCompType & qdutils::COMPOSITION_TYPE_DYN)?
                "[DYN]": "",
            (hwcModuleCompType >= (qdutils::COMPOSITION_TYPE_DYN << 1))?
                "[???]": "");
    }
    ALOGI("Display[%s] Layer[*] %s-HwcModuleCompType, %d-layer MdpComp %s",
         mDisplayName, hwcModuleCompTypeLog.string(), sMdpCompMaxLayers,
        (listFlags & HWC_GEOMETRY_CHANGED)? "[HwcList Geometry Changed]": "");
}

void HwcDebug::logLayer(size_t layerIndex, hwc_layer_1_t hwLayers[])
{
    if (NULL == hwLayers) {
        ALOGE("Display[%s] Layer[%zu] Error. No hwc layers to log.",
            mDisplayName, layerIndex);
        return;
    }

    hwc_layer_1_t *layer = &hwLayers[layerIndex];
    hwc_rect_t sourceCrop = integerizeSourceCrop(layer->sourceCropf);
    hwc_rect_t displayFrame = layer->displayFrame;
    size_t numHwcRects = layer->visibleRegionScreen.numRects;
    hwc_rect_t const *hwcRects = layer->visibleRegionScreen.rects;
    private_handle_t *hnd = (private_handle_t *)layer->handle;

    char pixFormatStr[32] = "None";
    String8 hwcVisRegsScrLog("[None]");

    for (size_t i = 0 ; (hwcRects && (i < numHwcRects)); i++) {
        if (0 == i)
            hwcVisRegsScrLog.clear();
        hwcVisRegsScrLog.appendFormat("[%dl, %dt, %dr, %db]",
                                        hwcRects[i].left, hwcRects[i].top,
                                        hwcRects[i].right, hwcRects[i].bottom);
    }

    if (hnd)
        getHalPixelFormatStr(hnd->format, pixFormatStr);

    // Log Line 1
    ALOGI("Display[%s] Layer[%zu] SrcBuff[%dx%d] SrcCrop[%dl, %dt, %dr, %db] "
        "DispFrame[%dl, %dt, %dr, %db] VisRegsScr%s", mDisplayName, layerIndex,
        (hnd)? getWidth(hnd) : -1, (hnd)? getHeight(hnd) : -1,
        sourceCrop.left, sourceCrop.top,
        sourceCrop.right, sourceCrop.bottom,
        displayFrame.left, displayFrame.top,
        displayFrame.right, displayFrame.bottom,
        hwcVisRegsScrLog.string());
    // Log Line 2
    ALOGI("Display[%s] Layer[%zu] LayerCompType = %s, Format = %s, "
        "Orientation = %s, Flags = %s%s%s, Hints = %s%s%s, "
        "Blending = %s%s%s", mDisplayName, layerIndex,
        (layer->compositionType == HWC_FRAMEBUFFER)? "Framebuffer(GPU)":
            (layer->compositionType == HWC_OVERLAY)? "Overlay":
            (layer->compositionType == HWC_BACKGROUND)? "Background":"???",
         pixFormatStr,
         (layer->transform == 0)? "ROT_0":
             (layer->transform == HWC_TRANSFORM_FLIP_H)? "FLIP_H":
             (layer->transform == HWC_TRANSFORM_FLIP_V)? "FLIP_V":
             (layer->transform == HWC_TRANSFORM_ROT_90)? "ROT_90":
                                                        "ROT_INVALID",
         (layer->flags)? "": "[None]",
         (layer->flags & HWC_SKIP_LAYER)? "[Skip layer]":"",
         (layer->flags & qhwc::HWC_MDPCOMP)? "[MDP Comp]":"",
         (layer->hints)? "":"[None]",
         (layer->hints & HWC_HINT_TRIPLE_BUFFER)? "[Triple Buffer]":"",
         (layer->hints & HWC_HINT_CLEAR_FB)? "[Clear FB]":"",
         (layer->blending == HWC_BLENDING_NONE)? "[None]":"",
         (layer->blending == HWC_BLENDING_PREMULT)? "[PreMult]":"",
         (layer->blending == HWC_BLENDING_COVERAGE)? "[Coverage]":"");
}

void HwcDebug::dumpLayer(size_t layerIndex, hwc_layer_1_t hwLayers[])
{
    char dumpLogStrPng[128] = "";
    char dumpLogStrRaw[128] = "";
    bool needDumpPng = (mDumpCntrPng <= mDumpCntLimPng)? true:false;
    bool needDumpRaw = (mDumpCntrRaw <= mDumpCntLimRaw)? true:false;

    if (needDumpPng) {
        snprintf(dumpLogStrPng, sizeof(dumpLogStrPng),
            "[png-dump-frame: %03d of %03d]", mDumpCntrPng,
            mDumpCntLimPng);
    }
    if (needDumpRaw) {
        snprintf(dumpLogStrRaw, sizeof(dumpLogStrRaw),
            "[raw-dump-frame: %03d of %03d]", mDumpCntrRaw,
            mDumpCntLimRaw);
    }

    if (!(needDumpPng || needDumpRaw))
        return;

    if (NULL == hwLayers) {
        ALOGE("Display[%s] Layer[%zu] %s%s Error: No hwc layers to dump.",
            mDisplayName, layerIndex, dumpLogStrRaw, dumpLogStrPng);
        return;
    }

    hwc_layer_1_t *layer = &hwLayers[layerIndex];
    private_handle_t *hnd = (private_handle_t *)layer->handle;
    char pixFormatStr[32] = "None";

    if (NULL == hnd) {
        ALOGI("Display[%s] Layer[%zu] %s%s Skipping dump: Bufferless layer.",
            mDisplayName, layerIndex, dumpLogStrRaw, dumpLogStrPng);
        return;
    }

    getHalPixelFormatStr(hnd->format, pixFormatStr);
#ifdef QCOM_BSP
    if (needDumpPng && hnd->base) {
        bool bResult = false;
        char dumpFilename[PATH_MAX];
        SkBitmap *tempSkBmp = new SkBitmap();
        SkBitmap::Config tempSkBmpConfig = SkBitmap::kNo_Config;
        snprintf(dumpFilename, sizeof(dumpFilename),
            "%s/sfdump%03d.layer%zu.%s.png", mDumpDirPng,
            mDumpCntrPng, layerIndex, mDisplayName);

        switch (hnd->format) {
            case HAL_PIXEL_FORMAT_RGBA_8888:
            case HAL_PIXEL_FORMAT_RGBX_8888:
            case HAL_PIXEL_FORMAT_BGRA_8888:
                tempSkBmpConfig = SkBitmap::kARGB_8888_Config;
                break;
            case HAL_PIXEL_FORMAT_RGB_565:
                tempSkBmpConfig = SkBitmap::kRGB_565_Config;
                break;
            case HAL_PIXEL_FORMAT_RGB_888:
            default:
                tempSkBmpConfig = SkBitmap::kNo_Config;
                break;
        }
        if (SkBitmap::kNo_Config != tempSkBmpConfig) {
            tempSkBmp->setConfig(tempSkBmpConfig, getWidth(hnd), getHeight(hnd));
            tempSkBmp->setPixels((void*)hnd->base);
            bResult = SkImageEncoder::EncodeFile(dumpFilename,
                                    *tempSkBmp, SkImageEncoder::kPNG_Type, 100);
            ALOGI("Display[%s] Layer[%zu] %s Dump to %s: %s",
                mDisplayName, layerIndex, dumpLogStrPng,
                dumpFilename, bResult ? "Success" : "Fail");
        } else {
            ALOGI("Display[%s] Layer[%zu] %s Skipping dump: Unsupported layer"
                " format %s for png encoder",
                mDisplayName, layerIndex, dumpLogStrPng, pixFormatStr);
        }
        delete tempSkBmp; // Calls SkBitmap::freePixels() internally.
    }
#endif
    if (needDumpRaw && hnd->base) {
        char dumpFilename[PATH_MAX];
        bool bResult = false;
        snprintf(dumpFilename, sizeof(dumpFilename),
            "%s/sfdump%03d.layer%zu.%dx%d.%s.%s.raw",
            mDumpDirRaw, mDumpCntrRaw,
            layerIndex, getWidth(hnd), getHeight(hnd),
            pixFormatStr, mDisplayName);
        FILE* fp = fopen(dumpFilename, "w+");
        if (NULL != fp) {
            bResult = (bool) fwrite((void*)hnd->base, hnd->size, 1, fp);
            fclose(fp);
        }
        ALOGI("Display[%s] Layer[%zu] %s Dump to %s: %s",
            mDisplayName, layerIndex, dumpLogStrRaw,
            dumpFilename, bResult ? "Success" : "Fail");
    }
}

void HwcDebug::getHalPixelFormatStr(int format, char pixFormatStr[])
{
    if (!pixFormatStr)
        return;

    switch(format) {
        case HAL_PIXEL_FORMAT_RGBA_8888:
            strlcpy(pixFormatStr, "RGBA_8888", sizeof(pixFormatStr));
            break;
        case HAL_PIXEL_FORMAT_RGBX_8888:
            strlcpy(pixFormatStr, "RGBX_8888", sizeof(pixFormatStr));
            break;
        case HAL_PIXEL_FORMAT_RGB_888:
            strlcpy(pixFormatStr, "RGB_888", sizeof(pixFormatStr));
            break;
        case HAL_PIXEL_FORMAT_RGB_565:
            strlcpy(pixFormatStr, "RGB_565", sizeof(pixFormatStr));
            break;
        case HAL_PIXEL_FORMAT_BGRA_8888:
            strlcpy(pixFormatStr, "BGRA_8888", sizeof(pixFormatStr));
            break;
        case HAL_PIXEL_FORMAT_YV12:
            strlcpy(pixFormatStr, "YV12", sizeof(pixFormatStr));
            break;
        case HAL_PIXEL_FORMAT_YCbCr_422_SP:
            strlcpy(pixFormatStr, "YCbCr_422_SP_NV16", sizeof(pixFormatStr));
            break;
        case HAL_PIXEL_FORMAT_YCrCb_420_SP:
            strlcpy(pixFormatStr, "YCrCb_420_SP_NV21", sizeof(pixFormatStr));
            break;
        case HAL_PIXEL_FORMAT_YCbCr_422_I:
            strlcpy(pixFormatStr, "YCbCr_422_I_YUY2", sizeof(pixFormatStr));
            break;
        case HAL_PIXEL_FORMAT_YCrCb_422_I:
            strlcpy(pixFormatStr, "YCrCb_422_I_YVYU", sizeof(pixFormatStr));
            break;
        case HAL_PIXEL_FORMAT_NV12_ENCODEABLE:
            strlcpy(pixFormatStr, "NV12_ENCODEABLE", sizeof(pixFormatStr));
            break;
        case HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED:
            strlcpy(pixFormatStr, "YCbCr_420_SP_TILED_TILE_4x2",
                   sizeof(pixFormatStr));
            break;
        case HAL_PIXEL_FORMAT_YCbCr_420_SP:
            strlcpy(pixFormatStr, "YCbCr_420_SP", sizeof(pixFormatStr));
            break;
        case HAL_PIXEL_FORMAT_YCrCb_420_SP_ADRENO:
            strlcpy(pixFormatStr, "YCrCb_420_SP_ADRENO", sizeof(pixFormatStr));
            break;
        case HAL_PIXEL_FORMAT_YCrCb_422_SP:
            strlcpy(pixFormatStr, "YCrCb_422_SP", sizeof(pixFormatStr));
            break;
        case HAL_PIXEL_FORMAT_R_8:
            strlcpy(pixFormatStr, "R_8", sizeof(pixFormatStr));
            break;
        case HAL_PIXEL_FORMAT_RG_88:
            strlcpy(pixFormatStr, "RG_88", sizeof(pixFormatStr));
            break;
        case HAL_PIXEL_FORMAT_INTERLACE:
            strlcpy(pixFormatStr, "INTERLACE", sizeof(pixFormatStr));
            break;
        case HAL_PIXEL_FORMAT_YCbCr_420_SP_VENUS:
            strlcpy(pixFormatStr, "YCbCr_420_SP_VENUS", sizeof(pixFormatStr));
            break;
        default:
            size_t len = sizeof(pixFormatStr);
            snprintf(pixFormatStr, len, "Unknown0x%X", format);
            break;
    }
}

} // namespace qhwc

