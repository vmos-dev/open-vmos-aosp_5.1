/*
 *
 * Copyright 2012 Samsung Electronics S.LSI Co. LTD
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

#ifndef _EXYNOS_VIDEO_API_H_
#define _EXYNOS_VIDEO_API_H_

#include "exynos_v4l2.h"

/* Fixed */
#define VIDEO_BUFFER_MAX_PLANES 3
#define VIDEO_BUFFER_MAX_NUM    32

typedef enum _ExynosVideoBoolType {
    VIDEO_FALSE = 0,
    VIDEO_TRUE  = 1,
} ExynosVideoBoolType;

typedef enum _ExynosVideoErrorType {
    VIDEO_ERROR_NONE      =  1,
    VIDEO_ERROR_BADPARAM  = -1,
    VIDEO_ERROR_OPENFAIL  = -2,
    VIDEO_ERROR_NOMEM     = -3,
    VIDEO_ERROR_APIFAIL   = -4,
    VIDEO_ERROR_MAPFAIL   = -5,
    VIDEO_ERROR_NOBUFFERS = -6,
    VIDEO_ERROR_POLL      = -7,
    VIDEO_ERROR_WRONGBUFFERSIZE = -8,
} ExynosVideoErrorType;

typedef enum _ExynosVideoCodingType {
    VIDEO_CODING_UNKNOWN = 0,
    VIDEO_CODING_MPEG2,
    VIDEO_CODING_H263,
    VIDEO_CODING_MPEG4,
    VIDEO_CODING_VC1,
    VIDEO_CODING_VC1_RCV,
    VIDEO_CODING_AVC,
    VIDEO_CODING_MVC,
    VIDEO_CODING_VP8,
    VIDEO_CODING_RESERVED,
} ExynosVideoCodingType;

typedef enum _ExynosVideoColorFormatType {
    VIDEO_COLORFORMAT_UNKNOWN = 0,
    VIDEO_COLORFORMAT_NV12,
    VIDEO_COLORFORMAT_NV21,
    VIDEO_COLORFORMAT_NV12_TILED,
    VIDEO_COLORFORMAT_RESERVED,
} ExynosVideoColorFormatType;

typedef enum _ExynosVideoFrameType {
    VIDEO_FRAME_NOT_CODED = 0,
    VIDEO_FRAME_I,
    VIDEO_FRAME_P,
    VIDEO_FRAME_B,
    VIDEO_FRAME_SKIPPED,
    VIDEO_FRAME_OTHERS,
} ExynosVideoFrameType;

typedef enum _ExynosVideoFrameStatusType {
    VIDEO_FRAME_STATUS_UNKNOWN = 0,
    VIDEO_FRAME_STATUS_DECODING_ONLY,
    VIDEO_FRAME_STATUS_DISPLAY_DECODING,
    VIDEO_FRAME_STATUS_DISPLAY_ONLY,
    VIDEO_FRAME_STATUS_DECODING_FINISHED,
    VIDEO_FRAME_STATUS_CHANGE_RESOL,
} ExynosVideoFrameStatusType;

typedef enum _ExynosVideoFrameSkipMode {
    VIDEO_FRAME_SKIP_DISABLED = 0,
    VIDEO_FRAME_SKIP_MODE_LEVEL_LIMIT,
    VIDEO_FRAME_SKIP_MODE_BUF_LIMIT,
} ExynosVideoFrameSkipMode;

typedef struct _ExynosVideoRect {
    unsigned int nTop;
    unsigned int nLeft;
    unsigned int nWidth;
    unsigned int nHeight;
} ExynosVideoRect;

typedef struct _ExynosVideoGeometry {
    unsigned int               nFrameWidth;
    unsigned int               nFrameHeight;
    unsigned int               nSizeImage;
    ExynosVideoRect            cropRect;
    ExynosVideoCodingType      eCompressionFormat;
    ExynosVideoColorFormatType eColorFormat;
} ExynosVideoGeometry;

typedef struct _ExynosVideoPlane {
    void          *addr;
    unsigned int   allocSize;
    unsigned int   dataSize;
    unsigned long  offset;
    int            fd;
} ExynosVideoPlane;

typedef struct _ReleaseDPB {
    int fd;
    int fd1;
    int fd2;
} ReleaseDPB;

typedef struct _PrivateDataShareBuffer {
    int index;
    ReleaseDPB dpbFD[VIDEO_BUFFER_MAX_NUM];
}PrivateDataShareBuffer;

typedef struct _ExynosVideoBuffer {
    ExynosVideoPlane            planes[VIDEO_BUFFER_MAX_PLANES];
    ExynosVideoGeometry        *pGeometry;
    ExynosVideoFrameStatusType  displayStatus;
    ExynosVideoFrameType        frameType;
    ExynosVideoBoolType         bQueued;
    ExynosVideoBoolType         bSlotUsed;
    ExynosVideoBoolType         bRegistered;
    void                       *pPrivate;
    PrivateDataShareBuffer      PDSB;
    int                         nIndexUseCnt;
} ExynosVideoBuffer;

typedef struct _ExynosVideoFramePacking{
    int           available;
    unsigned int  arrangement_id;
    int           arrangement_cancel_flag;
    unsigned char arrangement_type;
    int           quincunx_sampling_flag;
    unsigned char content_interpretation_type;
    int           spatial_flipping_flag;
    int           frame0_flipped_flag;
    int           field_views_flag;
    int           current_frame_is_frame0_flag;
    unsigned char frame0_grid_pos_x;
    unsigned char frame0_grid_pos_y;
    unsigned char frame1_grid_pos_x;
    unsigned char frame1_grid_pos_y;
} ExynosVideoFramePacking;

typedef struct _ExynosVideoEncInitParam{
    /* Initial parameters */
    ExynosVideoFrameSkipMode FrameSkip; /* [IN] frame skip mode */
    int FMO;
    int ASO;
}ExynosVideoEncInitParam;

typedef struct _ExynosVideoEncCommonParam{
    /* common parameters */
    int SourceWidth;                    /* [IN] width of video to be encoded */
    int SourceHeight;                   /* [IN] height of video to be encoded */
    int IDRPeriod;                      /* [IN] GOP number(interval of I-frame) */
    int SliceMode;                      /* [IN] Multi slice mode */
    int RandomIntraMBRefresh;           /* [IN] cyclic intra refresh */
    int EnableFRMRateControl;           /* [IN] frame based rate control enable */
    int EnableMBRateControl;            /* [IN] Enable macroblock-level rate control */
    int Bitrate;                        /* [IN] rate control parameter(bit rate) */
    int FrameQp;                        /* [IN] The quantization parameter of the frame */
    int FrameQp_P;                      /* [IN] The quantization parameter of the P frame */
    int QSCodeMax;                      /* [IN] Maximum Quantization value */
    int QSCodeMin;                      /* [IN] Minimum Quantization value */
    int CBRPeriodRf;                    /* [IN] Reaction coefficient parameter for rate control */
    int PadControlOn;                   /* [IN] Enable padding control */
    int LumaPadVal;                     /* [IN] Luma pel value used to fill padding area */
    int CbPadVal;                       /* [IN] CB pel value used to fill padding area */
    int CrPadVal;                       /* [IN] CR pel value used to fill padding area */
    int FrameMap;                       /* [IN] Encoding input mode(tile mode or linear mode) */
}ExynosVideoEncCommonParam;

typedef struct _ExynosVideoEncH264Param{
    /* H.264 specific parameters */
    int ProfileIDC;                     /* [IN] profile */
    int LevelIDC;                       /* [IN] level */
    int FrameQp_B;                      /* [IN] The quantization parameter of the B frame */
    int FrameRate;                      /* [IN] rate control parameter(frame rate) */
    int SliceArgument;                  /* [IN] MB number or byte number */
    int NumberBFrames;                  /* [IN] The number of consecutive B frame inserted */
    int NumberReferenceFrames;          /* [IN] The number of reference pictures used */
    int NumberRefForPframes;            /* [IN] The number of reference pictures used for encoding P pictures */
    int LoopFilterDisable;              /* [IN] disable the loop filter */
    int LoopFilterAlphaC0Offset;        /* [IN] Alpha & C0 offset for H.264 loop filter */
    int LoopFilterBetaOffset;           /* [IN] Beta offset for H.264 loop filter */
    int SymbolMode;                     /* [IN] The mode of entropy coding(CABAC, CAVLC) */
    int PictureInterlace;               /* [IN] Enables the interlace mode */
    int Transform8x8Mode;               /* [IN] Allow 8x8 transform(This is allowed only for high profile) */
    int DarkDisable;                    /* [IN] Disable adaptive rate control on dark region */
    int SmoothDisable;                  /* [IN] Disable adaptive rate control on smooth region */
    int StaticDisable;                  /* [IN] Disable adaptive rate control on static region */
    int ActivityDisable;                /* [IN] Disable adaptive rate control on high activity region */
} ExynosVideoEncH264Param;

typedef struct _ExynosVideoEncMpeg4Param {
    /* MPEG4 specific parameters */
    int ProfileIDC;                     /* [IN] profile */
    int LevelIDC;                       /* [IN] level */
    int FrameQp_B;                      /* [IN] The quantization parameter of the B frame */
    int TimeIncreamentRes;              /* [IN] frame rate */
    int VopTimeIncreament;              /* [IN] frame rate */
    int SliceArgument;                  /* [IN] MB number or byte number */
    int NumberBFrames;                  /* [IN] The number of consecutive B frame inserted */
    int DisableQpelME;                  /* [IN] disable quarter-pixel motion estimation */
} ExynosVideoEncMpeg4Param;

typedef struct _ExynosVideoEncH263Param {
    /* H.263 specific parameters */
    int FrameRate;                      /* [IN] rate control parameter(frame rate) */
} ExynosVideoEncH263Param;

typedef union _ExynosVideoEncCodecParam {
    ExynosVideoEncH264Param     h264;
    ExynosVideoEncMpeg4Param    mpeg4;
    ExynosVideoEncH263Param     h263;
} ExynosVideoEncCodecParam;

typedef struct _ExynosVideoEncParam {
    ExynosVideoCodingType       eCompressionFormat;
    ExynosVideoEncInitParam     initParam;
    ExynosVideoEncCommonParam   commonParam;
    ExynosVideoEncCodecParam    codecParam;
} ExynosVideoEncParam;

typedef struct _ExynosVideoDecOps {
    unsigned int            nSize;

    void *                (*Init)(int nMemoryType);
    ExynosVideoErrorType  (*Finalize)(void *pHandle);

    /* Add new ops at the end of structure, no order change */
    ExynosVideoErrorType  (*Set_FrameTag)(void *pHandle, int frameTag);
    int                   (*Get_FrameTag)(void *pHandle);
    int                   (*Get_ActualBufferCount)(void *pHandle);
    ExynosVideoErrorType  (*Set_DisplayDelay)(void *pHandle, int delay);
    ExynosVideoErrorType  (*Set_IFrameDecoding)(void *pHandle);
    ExynosVideoErrorType  (*Enable_PackedPB)(void *pHandle);
    ExynosVideoErrorType  (*Enable_LoopFilter)(void *pHandle);
    ExynosVideoErrorType  (*Enable_SliceMode)(void *pHandle);
    ExynosVideoErrorType  (*Enable_SEIParsing)(void *pHandle);
    ExynosVideoErrorType  (*Get_FramePackingInfo)(void *pHandle, ExynosVideoFramePacking *pFramepacking);
} ExynosVideoDecOps;

typedef struct _ExynosVideoEncOps {
    unsigned int           nSize;
    void *               (*Init)(int nMemoryType);
    ExynosVideoErrorType (*Finalize)(void *pHandle);

    /* Add new ops at the end of structure, no order change */
    ExynosVideoErrorType (*Set_EncParam)(void *pHandle, ExynosVideoEncParam*encParam);
    ExynosVideoErrorType (*Set_FrameTag)(void *pHandle, int frameTag);
    int (*Get_FrameTag)(void *pHandle);
    ExynosVideoErrorType (*Set_FrameType)(void *pHandle, int frameType);
    ExynosVideoErrorType (*Set_FrameRate)(void *pHandle, int frameRate);
    ExynosVideoErrorType (*Set_BitRate)(void *pHandle, int bitRate);
    ExynosVideoErrorType (*Set_FrameSkip)(void *pHandle, int frameSkip);
    ExynosVideoErrorType (*Set_IDRPeriod)(void *pHandle, int period);
    ExynosVideoErrorType (*Enable_PrependSpsPpsToIdr)(void *pHandle);
} ExynosVideoEncOps;

typedef struct _ExynosVideoDecBufferOps {
    unsigned int            nSize;

    /* Add new ops at the end of structure, no order change */
    ExynosVideoErrorType  (*Enable_Cacheable)(void *pHandle);
    ExynosVideoErrorType  (*Set_Shareable)(void *pHandle);
    ExynosVideoErrorType  (*Get_Buffer)(void *pHandle, int nIndex, ExynosVideoBuffer **pBuffer);
    ExynosVideoErrorType  (*Set_Geometry)(void *pHandle, ExynosVideoGeometry *bufferConf);
    ExynosVideoErrorType  (*Get_Geometry)(void *pHandle, ExynosVideoGeometry *bufferConf);
    ExynosVideoErrorType  (*Setup)(void *pHandle, unsigned int nBufferCount);
    ExynosVideoErrorType  (*Run)(void *pHandle);
    ExynosVideoErrorType  (*Stop)(void *pHandle);
    ExynosVideoErrorType  (*Enqueue)(void *pHandle, unsigned char *pBuffer[], unsigned int dataSize[], int nPlanes, void *pPrivate);
    ExynosVideoErrorType  (*Enqueue_All)(void *pHandle);
    ExynosVideoBuffer *   (*Dequeue)(void *pHandle);
    ExynosVideoErrorType  (*Register)(void *pHandle, ExynosVideoPlane *planes, int nPlanes);
    ExynosVideoErrorType  (*Clear_RegisteredBuffer)(void *pHandle);
    ExynosVideoErrorType  (*Clear_Queue)(void *pHandle);
    ExynosVideoErrorType  (*Cleanup_Buffer)(void *pHandle);
    ExynosVideoErrorType  (*ExtensionEnqueue)(void *pHandle, unsigned char *pBuffer[], unsigned int *pFd[], unsigned int allocLen[], unsigned int dataSize[], int nPlanes, void *pPrivate);
    ExynosVideoErrorType  (*ExtensionDequeue)(void *pHandle, ExynosVideoBuffer *pVideoBuffer);
    ExynosVideoErrorType  (*Enable_DynamicDPB)(void *pHandle);
} ExynosVideoDecBufferOps;

typedef struct _ExynosVideoEncBufferOps {
    unsigned int            nSize;

    /* Add new ops at the end of structure, no order change */
    ExynosVideoErrorType  (*Enable_Cacheable)(void *pHandle);
    ExynosVideoErrorType  (*Set_Shareable)(void *pHandle);
    ExynosVideoErrorType  (*Get_Buffer)(void *pHandle, int nIndex, ExynosVideoBuffer **pBuffer);
    ExynosVideoErrorType  (*Set_Geometry)(void *pHandle, ExynosVideoGeometry *bufferConf);
    ExynosVideoErrorType  (*Get_Geometry)(void *pHandle, ExynosVideoGeometry *bufferConf);
    ExynosVideoErrorType  (*Setup)(void *pHandle, unsigned int nBufferCount);
    ExynosVideoErrorType  (*Run)(void *pHandle);
    ExynosVideoErrorType  (*Stop)(void *pHandle);
    ExynosVideoErrorType  (*Enqueue)(void *pHandle, unsigned char *pBuffer[], unsigned int dataSize[], int nPlanes, void *pPrivate);
    ExynosVideoErrorType  (*Enqueue_All)(void *pHandle);
    ExynosVideoBuffer *   (*Dequeue)(void *pHandle);
    ExynosVideoErrorType  (*Register)(void *pHandle, ExynosVideoPlane *planes, int nPlanes);
    ExynosVideoErrorType  (*Clear_RegisteredBuffer)(void *pHandle);
    ExynosVideoErrorType  (*Clear_Queue)(void *pHandle);
    ExynosVideoErrorType  (*ExtensionEnqueue)(void *pHandle, unsigned char *pBuffer[], unsigned int *pFd[], unsigned int allocLen[], unsigned int dataSize[], int nPlanes, void *pPrivate);
    ExynosVideoErrorType  (*ExtensionDequeue)(void *pHandle, ExynosVideoBuffer *pVideoBuffer);
} ExynosVideoEncBufferOps;

int Exynos_Video_Register_Decoder(
    ExynosVideoDecOps       *pDecOps,
    ExynosVideoDecBufferOps *pInbufOps,
    ExynosVideoDecBufferOps *pOutbufOps);

int Exynos_Video_Register_Encoder(
    ExynosVideoEncOps       *pEncOps,
    ExynosVideoEncBufferOps *pInbufOps,
    ExynosVideoEncBufferOps *pOutbufOps);

#endif /* _EXYNOS_VIDEO_API_H_ */
