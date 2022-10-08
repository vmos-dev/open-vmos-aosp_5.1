/*
 * Copyright (c) 2010 The Khronos Group Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/** @file OMX_IndexExt.h - OpenMax IL version 1.1.2
 * The OMX_IndexExt header file contains extensions to the definitions
 * for both applications and components .
 */

#ifndef OMX_IntelIndexExt_h
#define OMX_IntelIndexExt_h

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Each OMX header shall include all required header files to allow the
 * header to compile without errors.  The includes below are required
 * for this header file to compile successfully
 */
#include <OMX_Index.h>


/** Khronos standard extension indices.

This enum lists the current Khronos extension indices to OpenMAX IL.
*/
typedef enum OMX_INTELINDEXEXTTYPE {
    OMX_IndexIntelStartUsed = OMX_IndexVendorStartUnused + 1, 
    OMX_IndexParamVideoBytestream,                  /**< reference: OMX_VIDEO_PARAM_BYTESTREAMTYPE */
    OMX_IndexParamIntelBitrate,                     /**< reference: OMX_VIDEO_PARAM_INTEL_BITRATETYPE */
    OMX_IndexConfigIntelBitrate,                    /**< reference: OMX_VIDEO_CONFIG_INTEL_BITRATETYPE */
    OMX_IndexParamIntelAVCDecodeSettings,           /**< reference: OMX_VIDEO_PARAM_INTEL_AVC_DECODE_SETTINGS */
    OMX_IndexConfigIntelSliceNumbers,               /**< reference: OMX_VIDEO_CONFIG_INTEL_SLICE_NUMBERS */
    OMX_IndexConfigIntelAIR,                        /**< reference: OMX_VIDEO_CONFIG_INTEL_AIR */
    OMX_IndexParamIntelAVCVUI,                      /**< reference: OMX_VIDEO_PARAM_INTEL_AVCVUI */
    OMX_IndexParamIntelAdaptiveSliceControl,        /**< reference: OMX_VIDEO_PARAM_INTEL_ADAPTIVE_SLICE_CONTROL */
    OMX_IndexStoreMetaDataInBuffers,                /**< reference: StoreMetaDataInBuffersParams*/
    OMX_IndexIntelPrivateInfo,
    /* NativeWindow configurations */
    OMX_IndexExtEnableNativeBuffer,                 /**<reference: EnableNativeBuffer */
    OMX_IndexExtGetNativeBufferUsage,               /**<reference: GetNativeBufferUsage  */
    OMX_IndexExtUseNativeBuffer,                    /**<reference: UseNativeBuffer  */
    OMX_IndexExtRotationDegrees,                    /**<reference: Rotation for decode*/
    OMX_IndexExtSyncEncoding,                       /**<reference: Sync mode for encode*/
    OMX_IndexExtPrependSPSPPS,
    /* Error report by WebRTC */
    OMX_IndexExtEnableErrorReport,                  /**<reference: EnableErrorReport for decoder */
    OMX_IndexExtPrepareForAdaptivePlayback,         /**<reference: Prepare for AdaptivePlayback*/
    OMX_IndexExtVP8MaxFrameSizeRatio,               /**<reference: For VP8 Max Frame Size*/
    OMX_IndexExtTemporalLayer,                      /**<reference: For Temporal Layer*/
    OMX_IndexExtRequestBlackFramePointer,           /**<reference: OMX_VIDEO_INTEL_REQUEST_BALCK_FRAME_POINTER*/

    // Index for VPP must always be put at the end
#ifdef TARGET_HAS_ISV
    OMX_IndexExtVppBufferNum,                       /**<reference: vpp buffer number*/
#endif
    OMX_IntelIndexExtMax = 0x7FFFFFFF
} OMX_INTELINDEXEXTTYPE;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OMX_IndexExt_h */
/* File EOF */
