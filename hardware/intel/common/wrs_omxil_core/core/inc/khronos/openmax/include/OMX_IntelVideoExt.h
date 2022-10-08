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

/** OMX_VideoExt.h - OpenMax IL version 1.1.2
 * The OMX_VideoExt header file contains extensions to the
 * definitions used by both the application and the component to
 * access video items.
 */

#ifndef OMX_IntelVideoExt_h
#define OMX_IntelVideoExt_h

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Each OMX header shall include all required header files to allow the
 * header to compile without errors.  The includes below are required
 * for this header file to compile successfully
 */
#include <OMX_Core.h>
#include <OMX_Video.h>

/** NALU Formats */
typedef enum OMX_INTEL_NALUFORMATSTYPE {
    OMX_NaluFormatZeroByteInterleaveLength = 32,
    OMX_NaluFormatStartCodesSeparateFirstHeader = 64,
    OMX_NaluFormatLengthPrefixedSeparateFirstHeader = 128,
} OMX_INTEL_NALUFORMATSTYPE;


typedef struct OMX_VIDEO_PARAM_BYTESTREAMTYPE {
     OMX_U32 nSize;                 // Size of the structure
     OMX_VERSIONTYPE nVersion;      // OMX specification version
     OMX_U32 nPortIndex;            // Port that this structure applies to
     OMX_BOOL bBytestream;          // Enable/disable bytestream support
} OMX_VIDEO_PARAM_BYTESTREAMTYPE;

typedef struct OMX_VIDEO_CONFIG_INTEL_BITRATETYPE {
     OMX_U32 nSize;
     OMX_VERSIONTYPE nVersion;
     OMX_U32 nPortIndex;
     OMX_U32 nMaxEncodeBitrate;    // Maximum bitrate
     OMX_U32 nTargetPercentage;    // Target bitrate as percentage of maximum bitrate; e.g. 95 is 95%
     OMX_U32 nWindowSize;          // Window size in milliseconds allowed for bitrate to reach target
     OMX_U32 nInitialQP;           // Initial QP for I frames
     OMX_U32 nMinQP;
     OMX_U32 nMaxQP;
     OMX_U32 nFrameRate;
     OMX_U32 nTemporalID;
} OMX_VIDEO_CONFIG_INTEL_BITRATETYPE;

enum  {
    OMX_Video_Intel_ControlRateVideoConferencingMode = OMX_Video_ControlRateVendorStartUnused + 1
};

typedef struct OMX_VIDEO_PARAM_INTEL_AVC_DECODE_SETTINGS {
     OMX_U32 nSize;                       // Size of the structure
     OMX_VERSIONTYPE nVersion;            // OMX specification version
     OMX_U32 nPortIndex;                  // Port that this structure applies to
     OMX_U32 nMaxNumberOfReferenceFrame;  // Maximum number of reference frames
     OMX_U32 nMaxWidth;                   // Maximum width of video
     OMX_U32 nMaxHeight;                  // Maximum height of video
} OMX_VIDEO_PARAM_INTEL_AVC_DECODE_SETTINGS;


typedef struct OMX_VIDEO_CONFIG_INTEL_SLICE_NUMBERS {
     OMX_U32 nSize;                       // Size of the structure
     OMX_VERSIONTYPE nVersion;            // OMX specification version
     OMX_U32 nPortIndex;                  // Port that this structure applies to
     OMX_U32 nISliceNumber;               // I frame slice number
     OMX_U32 nPSliceNumber;               // P frame slice number
} OMX_VIDEO_CONFIG_INTEL_SLICE_NUMBERS;


typedef struct OMX_VIDEO_CONFIG_INTEL_AIR {
     OMX_U32 nSize;                       // Size of the structure
     OMX_VERSIONTYPE nVersion;            // OMX specification version
     OMX_U32 nPortIndex;                  // Port that this structure applies to
     OMX_BOOL bAirEnable;                 // Enable AIR
     OMX_BOOL bAirAuto;                   // AIR auto
     OMX_U32 nAirMBs;                     // Number of AIR MBs
     OMX_U32 nAirThreshold;               // AIR Threshold

} OMX_VIDEO_CONFIG_INTEL_AIR;

typedef struct OMX_VIDEO_PARAM_INTEL_AVCVUI {
     OMX_U32 nSize;                       // Size of the structure
     OMX_VERSIONTYPE nVersion;            // OMX specification version
     OMX_U32 nPortIndex;                  // Port that this structure applies to
     OMX_BOOL  bVuiGeneration;            // Enable/disable VUI generation

} OMX_VIDEO_PARAM_INTEL_AVCVUI;

typedef struct OMX_VIDEO_PARAM_INTEL_ADAPTIVE_SLICE_CONTROL {
     OMX_U32 nSize;                       // Size of the structure
     OMX_VERSIONTYPE nVersion;            // OMX specification version
     OMX_U32 nPortIndex;                  // Port that this structure applies to
     OMX_BOOL bEnable;                    // enable adaptive slice control
     OMX_U32 nMinPSliceNumber;            // minimum number of P slices
     OMX_U32 nNumPFramesToSkip;           // number of P frames after an I frame to skip before kicking in adaptive slice control
     OMX_U32 nSliceSizeThreshold;         // Slice size threshold for adaptive slice control to start a new slice
     OMX_U32 nSliceSizeSkipThreshold;     // Slice size skip threshold for adaptive slice control to start a new slice
} OMX_VIDEO_PARAM_INTEL_ADAPTIVE_SLICE_CONTROL;

/**
 * Vendor Private Configs
 *
 * STRUCT MEMBERS:
 *  nSize      : Size of the structure in bytes
 *  nVersion   : OMX specification version information
 *  nPortIndex : Port that this structure applies to
 *  nCapacity  : Specifies the private unit size
 *  nHolder    : Pointer to private unit address 
 */
typedef struct OMX_VIDEO_CONFIG_PRI_INFOTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nCapacity;
    OMX_PTR nHolder;
} OMX_VIDEO_CONFIG_PRI_INFOTYPE;

// Error reporting data structure
typedef struct OMX_VIDEO_CONFIG_INTEL_ERROR_REPORT {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bEnable;
} OMX_VIDEO_CONFIG_INTEL_ERROR_REPORT;

#define MAX_ERR_NUM 10

typedef enum
{
    OMX_Decode_HeaderError   = 0,
    OMX_Decode_MBError       = 1,
    OMX_Decode_SliceMissing  = 2,
    OMX_Decode_RefMissing    = 3,
} OMX_VIDEO_DECODE_ERRORTYPE;

typedef struct OMX_VIDEO_ERROR_INFO {
    OMX_VIDEO_DECODE_ERRORTYPE type;
    OMX_U32 num_mbs;
    union {
        struct {OMX_U32 start_mb; OMX_U32 end_mb;} mb_pos;
    } error_data;
} OMX_VIDEO_ERROR_INFO;

typedef struct OMX_VIDEO_ERROR_BUFFER {
    OMX_U32 errorNumber;   // Error number should be no more than MAX_ERR_NUM
    OMX_S64 timeStamp;      // presentation time stamp
    OMX_VIDEO_ERROR_INFO errorArray[MAX_ERR_NUM];
} OMX_VIDEO_ERROR_BUFFER;

// Force K frame for VP8 encode
typedef struct OMX_VIDEO_CONFIG_INTEL_VP8_FORCE_KFRAME {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bForceKFrame;
} OMX_VIDEO_CONFIG_INTEL_VP8_FORCE_KFRAME;

// max frame size for VP8 encode during WebRTC feature
typedef struct OMX_VIDEO_CONFIG_INTEL_VP8_MAX_FRAME_SIZE_RATIO {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nMaxFrameSizeRatio;
} OMX_VIDEO_CONFIG_INTEL_VP8_MAX_FRAME_SIZE_RATIO;

// temporal layer for Sand
typedef struct OMX_VIDEO_PARAM_INTEL_TEMPORAL_LAYER {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nNumberOfTemporalLayer;
    OMX_U32 nPeriodicity;
    OMX_U32 nLayerID[32];
} OMX_VIDEO_PARAM_INTEL_TEMPORAL_LAYER;


// Request OMX to allocate a black frame to video mute feature
typedef struct OMX_VIDEO_INTEL_REQUEST_BALCK_FRAME_POINTER {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nFramePointer;
} OMX_VIDEO_INTEL_REQUEST_BALCK_FRAME_POINTER;

#define OMX_INTEL_COLOR_FormatYUV420PackedSemiPlanar (OMX_COLOR_FORMATTYPE)0x7FA00E00
#define OMX_INTEL_COLOR_FormatYUV420PackedSemiPlanar_Tiled (OMX_COLOR_FORMATTYPE)0x7FA00F00

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OMX_VideoExt_h */
/* File EOF */
