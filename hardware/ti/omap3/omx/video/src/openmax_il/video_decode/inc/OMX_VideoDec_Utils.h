/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef OMX_VIDDEC_UTILS__H
#define OMX_VIDDEC_UTILS__H

#define newmalloc(x) malloc(x)
#define newfree(z) free(z)
#ifdef ANDROID
/* Log for Android system*/
#include <utils/Log.h>
#define LOG_TAG "TI_Video_Decoder"
#endif
#include <cutils/properties.h>

#ifdef UNDER_CE
    #include <windows.h>
    #include <oaf_osal.h>
    #include <omx_core.h>
    #include <pthread.h>
    #include <stdlib.h>
#else
    #define _XOPEN_SOURCE 600
    #include <sys/select.h>
    #include <signal.h>
    #include <pthread.h>
    #include <wchar.h>
    #include <unistd.h>
    #include <sys/types.h>
    #include <malloc.h>
    #include <memory.h>
    #include <sys/wait.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <dbapi.h>
    #include <string.h>
    #include <stdio.h>
    #include <dlfcn.h>
    #include <stdarg.h>
    #include <errno.h>
    #include <sys/ioctl.h>
    #include <sys/time.h>
    #include <stdlib.h>
    #include <semaphore.h>
#endif

#ifndef KHRONOS_1_1
#define KHRONOS_1_1
#endif

#define KHRONOS_1_2
#ifndef VIDDEC_SPARK_CODE
    #define VIDDEC_SPARK_CODE
#endif

#ifndef VIDDEC_SN_R8_14
    #define VIDDEC_SN_R8_14
#endif

#define VIDDEC_WAIT_CODE() sched_yield()

#ifndef UNDER_CE
#ifndef __ENV_CHANGE__
#define __ENV_CHANGE__
#endif
#endif

#ifndef VIDDEC_ACTIVATEPARSER
    #define VIDDEC_ACTIVATEPARSER
#endif

#define VIDDEC_WMVPOINTERFIXED
#define VIDDEC_HANDLE_FULL_STRM_PROP_OBJ

#include <sched.h>
#include <OMX_Core.h>
#include <OMX_TI_Debug.h>
#include "OMX_VideoDecoder.h"
#include "OMX_VidDec_CustomCmd.h"
#include "OMX_TI_Common.h"


#ifdef KHRONOS_1_1
typedef struct OMX_CONFIG_MACROBLOCKERRORMAPTYPE_TI {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nErrMapSize;                /* Size of the Error Map in bytes*/
    OMX_U8  ErrMap[(864 * 480) / 256]; /* Error map hint   */
} OMX_CONFIG_MACROBLOCKERRORMAPTYPE_TI;
#endif

#define VIDDEC_MEMLEVELS 5
typedef enum VIDDEC_ENUM_MEMLEVELS{
    VIDDDEC_Enum_MemLevel0 = 0,
    VIDDDEC_Enum_MemLevel1,
    VIDDDEC_Enum_MemLevel2,
    VIDDDEC_Enum_MemLevel3,
    VIDDDEC_Enum_MemLevel4
}VIDDEC_ENUM_MEMLEVELS;

#include "LCML_DspCodec.h"
#include "LCML_Types.h"
#include "LCML_CodecInterface.h"

#ifdef __PERF_INSTRUMENTATION__
#include "perf.h"
#endif

/*#define RESOURCE_MANAGER_ENABLED*/
#ifdef RESOURCE_MANAGER_ENABLED
        #include <ResourceManagerProxyAPI.h>
#endif

#ifdef __ENV_CHANGE__
    #ifndef ENV_CHANGE_DEF_AUTO
        #define ENV_CHANGE_DEF_AUTO          "ON"
    #endif
    #ifndef ENV_CHANGE_DEF_VALUE
        #define ENV_CHANGE_DEF_VALUE         "H264"
    #endif
    #define ENV_CHANGE_VAL_ON                "ON"
    #define ENV_CHANGE_NAME_AUTO             "ENV_CHANGE_AUTO"
    #define ENV_CHANGE_NAME_VALUE            "ENV_CHANGE_VALUE"
    #define ENV_CHANGE_SET_H264              "H264"
    #define ENV_CHANGE_SET_AVC               "AVC"
    #define ENV_CHANGE_SET_H263              "H263"
    #define ENV_CHANGE_SET_MPEG2             "MPEG2"
    #define ENV_CHANGE_SET_MPEG4             "MPEG4"
    #define ENV_CHANGE_SET_WMV9              "WMV"
#ifdef VIDDEC_SPARK_CODE
    #define ENV_CHANGE_SET_SPARK             "SPARK"
#endif
#endif

#define VIDDEC_COMPONENTROLES_H263           "video_decoder.h263"
#define VIDDEC_COMPONENTROLES_H264           "video_decoder.avc"
#define VIDDEC_COMPONENTROLES_MPEG2          "video_decoder.mpeg2"
#define VIDDEC_COMPONENTROLES_MPEG4          "video_decoder.mpeg4"
#define VIDDEC_COMPONENTROLES_WMV9           "video_decoder.wmv"
#ifdef VIDDEC_SPARK_CODE
    #define VIDDEC_COMPONENTROLES_SPARK       "video_decoder.spark"
#endif

#define __STD_COMPONENT__

/*
 * MAX_PRIVATE_IN_BUFFERS and MAX_PRIVATE_OUT_BUFFERS must NOT be
 * greater than MAX_PRIVATE_BUFFERS. MAX_PRIVATE_BUFFERS is set
 * to 6 because 6 overlay buffers are currently being used for
 * playback
 */
#define MAX_PRIVATE_IN_BUFFERS              6
#define MAX_PRIVATE_OUT_BUFFERS             6
#define MAX_PRIVATE_BUFFERS                 6
#define NUM_OF_PORTS                        2
#define VIDDEC_MAX_NAMESIZE                 128
#define VIDDEC_NOPORT                       0xfffffffe
#define VIDDEC_MPU                          50

#define IUALG_CMD_SETSTATUS                 3

#define VERSION_MAJOR                       1
#ifdef KHRONOS_1_1
#define VERSION_MINOR                       0
#else
#define VERSION_MINOR                       0
#endif
#define VERSION_REVISION                    0
#define VERSION_STEP                        0

#define VIDDEC_COLORFORMAT422               OMX_COLOR_FormatCbYCrY
#define VIDDEC_COLORFORMAT420               OMX_COLOR_FormatYUV420Planar /*OMX_COLOR_FormatYUV420PackedPlanar is not working with OpenCore*/
#define VIDDEC_COLORFORMATUNUSED            OMX_COLOR_FormatUnused

#define VIDDEC_ZERO                         0
#define VIDDEC_ONE                          1
#define VIDDEC_MINUS                        -1
#define VIDDEC_WMVHEADER                    20

#define VIDDEC_BUFFERMINCOUNT                   VIDDEC_ONE
#define VIDDEC_PORT_ENABLED                     OMX_TRUE
#define VIDDEC_PORT_POPULATED                   OMX_FALSE
#define VIDDEC_PORT_DOMAIN                      OMX_PortDomainVideo

#define VIDDEC_DEFAULT_INPUT_BUFFER_SIZE        614400
#define VIDDEC_DEFAULT_OUTPUT_BUFFER_SIZE       614400
#define VIDDEC_DEFAULT_WIDTH                    640
#define VIDDEC_DEFAULT_HEIGHT                   480
#define VIDDEC_DEFAULT_PROCESSMODE              0      /* 0=frmmode; 1=strmmode */
#define VIDDEC_DEFAULT_H264BITSTRMFMT           0      /* 0=bytestrm; 1->4=NAL-bitstrm */
#define MAX_CCD_CNT                             128
#define MAX_NALUDATA_CNT                        128

#define VIDDEC_INPUT_PORT_COMPRESSIONFORMAT      OMX_VIDEO_CodingMPEG4
#define VIDDEC_OUTPUT_PORT_COMPRESSIONFORMAT      OMX_VIDEO_CodingUnused
#define VIDDEC_INPUT_PORT_BUFFERSUPPLIER        VIDDEC_ZERO
#define VIDDEC_OUTPUT_PORT_BUFFERSUPPLIER       VIDDEC_ZERO

#define VIDDEC_MIMETYPEH263                     "H263"
#define VIDDEC_MIMETYPEH264                     "H264"
#define VIDDEC_MIMETYPEMPEG4                    "MPEG4"
#define VIDDEC_MIMETYPEWMV                      "WMV"
#define VIDDEC_MIMETYPEYUV                      "YUV"
#ifdef VIDDEC_SPARK_CODE
    #define VIDDEC_MIMETYPEWMV_SPARK            "SPARK"
#endif

#define VIDDEC_INPUT_PORT_NATIVERENDER          NULL
#define VIDDEC_INPUT_PORT_STRIDE                VIDDEC_MINUS
#define VIDDEC_INPUT_PORT_SLICEHEIGHT           VIDDEC_MINUS

#ifdef __STD_COMPONENT__
    #define VIDDEC_INPUT_PORT_BITRATE           (64000)
    #define VIDDEC_INPUT_PORT_FRAMERATE         (15 << 16)
#else
    #define VIDDEC_INPUT_PORT_BITRATE            VIDDEC_MINUS
    #define VIDDEC_INPUT_PORT_FRAMERATE          VIDDEC_MINUS
#endif
#define VIDDEC_INPUT_PORT_FLAGERRORCONCEALMENT  OMX_FALSE

#define VIDDEC_OUTPUT_PORT_NATIVERENDER         NULL
#define VIDDEC_OUTPUT_PORT_STRIDE               VIDDEC_ZERO
#define VIDDEC_OUTPUT_PORT_SLICEHEIGHT          VIDDEC_ZERO
#define VIDDEC_OUTPUT_PORT_BITRATE              VIDDEC_ZERO
#define VIDDEC_OUTPUT_PORT_FRAMERATE            VIDDEC_ZERO
#define VIDDEC_OUTPUT_PORT_FLAGERRORCONCEALMENT OMX_FALSE

#define VIDDEC_FACTORFORMAT422                  2
#define VIDDEC_FACTORFORMAT420                  (1.5)

#define VIDDEC_DEFAULT_MPEG4_PORTINDEX              VIDDEC_INPUT_PORT
#define VIDDEC_DEFAULT_MPEG4_SLICEHEADERSPACING     VIDDEC_ZERO
#define VIDDEC_DEFAULT_MPEG4_SVH                    OMX_FALSE
#define VIDDEC_DEFAULT_MPEG4_GOV                    OMX_FALSE
#define VIDDEC_DEFAULT_MPEG4_PFRAMES                VIDDEC_MINUS
#define VIDDEC_DEFAULT_MPEG4_BFRAMES                VIDDEC_MINUS
#define VIDDEC_DEFAULT_MPEG4_IDCVLCTHRESHOLD        VIDDEC_MINUS
#define VIDDEC_DEFAULT_MPEG4_ACPRED                 OMX_FALSE
#define VIDDEC_DEFAULT_MPEG4_MAXPACKETSIZE          VIDDEC_DEFAULT_INPUT_BUFFER_SIZE
#define VIDDEC_DEFAULT_MPEG4_TIMEINCRES             VIDDEC_MINUS
#define VIDDEC_DEFAULT_MPEG4_PROFILE                OMX_VIDEO_MPEG4ProfileSimple
#define VIDDEC_DEFAULT_MPEG4_LEVEL                  OMX_VIDEO_MPEG4Level1
#define VIDDEC_DEFAULT_MPEG4_ALLOWEDPICTURETYPES    VIDDEC_MINUS
#define VIDDEC_DEFAULT_MPEG4_HEADEREXTENSION        VIDDEC_ONE
#define VIDDEC_DEFAULT_MPEG4_REVERSIBLEVLC          OMX_FALSE

#define VIDDEC_DEFAULT_MPEG2_PORTINDEX              VIDDEC_INPUT_PORT
#define VIDDEC_DEFAULT_MPEG2_PFRAMES                VIDDEC_MINUS
#define VIDDEC_DEFAULT_MPEG2_BFRAMES                VIDDEC_MINUS
#define VIDDEC_DEFAULT_MPEG2_PROFILE                OMX_VIDEO_MPEG2ProfileSimple
#define VIDDEC_DEFAULT_MPEG2_LEVEL                  OMX_VIDEO_MPEG2LevelLL
#define VIDDEC_DEFAULT_H264_PORTINDEX                 VIDDEC_INPUT_PORT
#define VIDDEC_DEFAULT_H264_SLICEHEADERSPACING        VIDDEC_ZERO
#define VIDDEC_DEFAULT_H264_PFRAMES                   VIDDEC_MINUS
#define VIDDEC_DEFAULT_H264_BFRAMES                   VIDDEC_MINUS
#define VIDDEC_DEFAULT_H264_USEHADAMARD               OMX_FALSE
#define VIDDEC_DEFAULT_H264_REFFRAMES                 VIDDEC_MINUS
#define VIDDEC_DEFAULT_H264_REFIDX10ACTIVEMINUS1      VIDDEC_MINUS
#define VIDDEC_DEFAULT_H264_REFIDX11ACTIVEMINUS1      VIDDEC_MINUS
#define VIDDEC_DEFAULT_H264_ENABLEUEP                 OMX_FALSE
#define VIDDEC_DEFAULT_H264_ENABLEFMO                 OMX_FALSE
#define VIDDEC_DEFAULT_H264_ENABLEASO                 OMX_FALSE
#define VIDDEC_DEFAULT_H264_ENABLERS                  OMX_FALSE
#define VIDDEC_DEFAULT_H264_PROFILE                   OMX_VIDEO_AVCProfileBaseline
#define VIDDEC_DEFAULT_H264_LEVEL                     OMX_VIDEO_AVCLevel1
#define VIDDEC_DEFAULT_H264_ALLOWEDPICTURETYPES       VIDDEC_MINUS
#define VIDDEC_DEFAULT_H264_FRAMEMBSONLY              OMX_FALSE
#define VIDDEC_DEFAULT_H264_MBAFF                     OMX_FALSE
#define VIDDEC_DEFAULT_H264_ENTROPYCODINGCABAC        OMX_FALSE
#define VIDDEC_DEFAULT_H264_WEIGHTEDPPREDICTION       OMX_FALSE
#define VIDDEC_DEFAULT_H264_WEIGHTEDBIPREDICITONMODE  VIDDEC_MINUS
#define VIDDEC_DEFAULT_H264_CONSTIPRED                OMX_FALSE
#define VIDDEC_DEFAULT_H264_DIRECT8X8INFERENCE        OMX_FALSE
#define VIDDEC_DEFAULT_H264_DIRECTSPATIALTEMPORAL     OMX_FALSE
#define VIDDEC_DEFAULT_H264_CABACINITIDC              VIDDEC_MINUS
#define VIDDEC_DEFAULT_H264_LOOPFILTERMODE            OMX_VIDEO_AVCLoopFilterDisable

#define VIDDEC_DEFAULT_H263_PORTINDEX                 VIDDEC_INPUT_PORT
#define VIDDEC_DEFAULT_H263_PFRAMES                   VIDDEC_MINUS
#define VIDDEC_DEFAULT_H263_BFRAMES                   VIDDEC_MINUS
#define VIDDEC_DEFAULT_H263_PROFILE                   OMX_VIDEO_H263ProfileBaseline
#define VIDDEC_DEFAULT_H263_LEVEL                     OMX_VIDEO_H263Level10
#define VIDDEC_DEFAULT_H263_PLUSPTYPEALLOWED          OMX_FALSE
#define VIDDEC_DEFAULT_H263_ALLOWEDPICTURETYPES       OMX_VIDEO_PictureTypeMax
#define VIDDEC_DEFAULT_H263_FORCEROUNDINGTYPETOZERO   OMX_TRUE
#define VIDDEC_DEFAULT_H263_PICTUREHEADERREPETITION   0
#define VIDDEC_DEFAULT_H263_GOBHEADERINTERVAL         1

#define VIDDEC_DEFAULT_WMV_PORTINDEX                  VIDDEC_INPUT_PORT
#define VIDDEC_DEFAULT_WMV_FORMAT                     OMX_VIDEO_WMVFormat9

#define VIDDEC_PIPE_WRITE                             VIDDEC_ONE
#define VIDDEC_PIPE_READ                              VIDDEC_ZERO

#define VIDDEC_PADDING_FULL                           256
#define VIDDEC_PADDING_HALF                           VIDDEC_PADDING_FULL / 2

#define VIDDEC_ALIGNMENT                              4

#define VIDDEC_CLEARFLAGS                             0
#define H264VDEC_SN_MAX_NALUNITS                      1200

#define VIDDEC_RM_FREC_MPEG4_QCIF                     30
#define VIDDEC_RM_FREC_MPEG4_CIF                      80
#define VIDDEC_RM_FREC_MPEG4_VGA                      165
#define VIDDEC_RM_FREC_MPEG4_720P                     401

#define VIDDEC_RM_FREC_MPEG2_QCIF                     30
#define VIDDEC_RM_FREC_MPEG2_CIF                      80
#define VIDDEC_RM_FREC_MPEG2_VGA                      165

#define VIDDEC_RM_FREC_H263_QCIF                      25
#define VIDDEC_RM_FREC_H263_CIF                       60
#define VIDDEC_RM_FREC_H263_VGA                       165

#define VIDDEC_RM_FREC_H264_QCIF                      85
#define VIDDEC_RM_FREC_H264_CIF                       160
#define VIDDEC_RM_FREC_H264_VGA                       260

#define VIDDEC_RM_FREC_WMV_QCIF                       55
#define VIDDEC_RM_FREC_WMV_CIF                        100
#define VIDDEC_RM_FREC_WMV_VGA                        300

#ifdef VIDDEC_SPARK_CODE
 #define VIDDEC_RM_FREC_SPARK_QCIF                    55
 #define VIDDEC_RM_FREC_SPARK_CIF                     100
 #define VIDDEC_RM_FREC_SPARK_VGA                     300
#endif

#define VIDDEC_MIN_WIDTH                              176
#define VIDDEC_MIN_HEIGHT                             144

#define VIDDEC_QCIF_WIDTH                             176
#define VIDDEC_QCIF_HEIGHT                            144

#define VIDDEC_QVGA_WIDTH                             320
#define VIDDEC_QVGA_HEIGHT                            240

#define VIDDEC_CIF_WIDTH                              352
#define VIDDEC_CIF_HEIGHT                             288

#define VIDDEC_VGA_WIDTH                              640
#define VIDDEC_VGA_HEIGHT                             480

#define VIDDEC_D1MAX_WIDTH                            864
#define VIDDEC_D1MAX_HEIGHT                           VIDDEC_D1MAX_WIDTH

/* In the current release the suport for : VIDDEC_MAX_FRAMERATE  & VIDDEC_MAX_BITRATE
 * is not provided by the algorithm. But is require to set this field to a non-zero value */
#define VIDDEC_MAX_FRAMERATE                        30000  /* Max frame rate to be suported * 1000 */
#define VIDDEC_MAX_BITRATE                        8000000  /* Max bit rate (in bits per second) to be suported */

#define VIDDEC_WMV_PROFILE_ID0                          0
#define VIDDEC_WMV_PROFILE_ID1                          1
#define VIDDEC_WMV_PROFILE_ID2                          2
#define VIDDEC_WMV_PROFILE_ID3                          3
#define VIDDEC_WMV_PROFILE_ID4                          4
#define VIDDEC_WMV_PROFILE_ID5                          5
#define VIDDEC_WMV_PROFILE_ID6                          6
#define VIDDEC_WMV_PROFILE_ID7                          7
#define VIDDEC_WMV_PROFILE_ID8                          8

#define VIDDEC_MAX_QUEUE_SIZE                           256
#define VIDDEC_WMV_BUFFER_OFFSET                        (255 - 4)
#define VIDDEC_WMV_ELEMSTREAM                           0
#define VIDDEC_WMV_RCVSTREAM                            1

#define VIDDEC_SN_WMV_ELEMSTREAM                        1
#define VIDDEC_SN_WMV_RCVSTREAM                         2

#define CSD_POSITION                                    51 /*Codec Specific Data position on the "stream propierties object"(ASF spec)*/

#ifndef KHRONOS_1_2
 #define OMX_BUFFERFLAG_CODECCONFIG 0x00000080
#endif

typedef struct VIDDEC_CUSTOM_PARAM
{
    unsigned char cCustomParamName[128];
    OMX_INDEXTYPE nCustomParamIndex;
} VIDDEC_CUSTOM_PARAM;

typedef enum VIDDEC_CUSTOM_PARAM_INDEX
{
#ifdef KHRONOS_1_2
    VideoDecodeCustomParamProcessMode = (OMX_IndexVendorStartUnused + 1),
#else
    VideoDecodeCustomParamProcessMode = (OMX_IndexIndexVendorStartUnused + 1),
#endif
    VideoDecodeCustomParamH264BitStreamFormat,
    VideoDecodeCustomParamWMVProfile,
    VideoDecodeCustomParamWMVFileType,
    VideoDecodeCustomParamParserEnabled,
    VideoDecodeCustomParamIsNALBigEndian,
#ifdef VIDDEC_SPARK_CODE
    VideoDecodeCustomParamIsSparkInput,
#endif
    VideoDecodeCustomConfigDebug

#ifdef ANDROID /*To be use by opencore multimedia framework*/
    ,
    PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX = 0xFF7A347
#endif
} VIDDEC_CUSTOM_PARAM_INDEX;

typedef enum VIDDEC_LCML_STATES
{
    VidDec_LCML_State_Unload = 0,
    VidDec_LCML_State_Load,
    VidDec_LCML_State_Init,
    VidDec_LCML_State_Pause,
    VidDec_LCML_State_Start,
    VidDec_LCML_State_Stop,
    VidDec_LCML_State_Destroy
} VIDDEC_LCML_STATES;

typedef enum VIDDEC_RMPROXY_STATES
{
    VidDec_RMPROXY_State_Unload = 0,
    VidDec_RMPROXY_State_Load,
    VidDec_RMPROXY_State_Registered,
} VIDDEC_RMPROXY_STATES;

/* ======================================================================= */
/**
* @def WMV9DEC_YUVFORMAT_XYZ : YUV ouput chroma format.
*/
/* ==================================================================== */
#define WMV9VIDDEC_YUVFORMAT_PLANAR420 (1)
#define WMV9VIDDEC_YUVFORMAT_INTERLEAVED422 (4)

/* ======================================================================= */
/**
* @def MP4VDEC_YUVFORMAT_XYZ : YUV ouput chroma format.
*/
/* ==================================================================== */
#define MP4VIDDEC_YUVFORMAT_PLANAR420 (1)
#define MP4VIDDEC_YUVFORMAT_INTERLEAVED422 (4)

/* ======================================================================= */
/**
* @def H264VDEC_YUVFORMAT_XYZ : YUV ouput chroma format.
*/
/* ==================================================================== */
#define H264VIDDEC_YUVFORMAT_PLANAR420 (0)
#define H264VIDDEC_YUVFORMAT_INTERLEAVED422 (1)

#define MP2VIDDEC_YUVFORMAT_PLANAR420 (1)
#define MP2VIDDEC_YUVFORMAT_INTERLEAVED422 (4)

/* ======================================================================= */
/**
* @def MP4VDEC_YUVFORMAT_XYZ : YUV ouput chroma format.
*/
/* ==================================================================== */
#define SPARKVIDDEC_YUVFORMAT_PLANAR420 (1)
#define SPARKVIDDEC_YUVFORMAT_INTERLEAVED422 (4)

typedef enum VIDDEC_PORT_INDEX
{
    VIDDEC_INPUT_PORT,
    VIDDEC_OUTPUT_PORT
}VIDDEC_PORT_INDEX;

typedef enum VIDDEC_DEFAULT_INPUT_INDEX
{
    VIDDEC_DEFAULT_INPUT_INDEX_H263,
    VIDDEC_DEFAULT_INPUT_INDEX_H264,
    VIDDEC_DEFAULT_INPUT_INDEX_MPEG4,
    VIDDEC_DEFAULT_INPUT_INDEX_WMV9,
    VIDDEC_DEFAULT_INPUT_INDEX_MPEG2,
#ifdef VIDDEC_SPARK_CODE
    VIDDEC_DEFAULT_INPUT_INDEX_SPARK,
#endif
    VIDDEC_DEFAULT_INPUT_INDEX_MAX = 0x7ffffff
}VIDDEC_DEFAULT_INPUT_INDEX;

typedef enum VIDDEC_DEFAULT_OUTPUT_INDEX
{
    VIDDEC_DEFAULT_OUTPUT_INDEX_INTERLEAVED422,
    VIDDEC_DEFAULT_OUTPUT_INDEX_PLANAR420,
    VIDDEC_DEFAULT_OUTPUT_INDEX_MAX = 0x7ffffff
}VIDDEC_DEFAULT_OUTPUT_INDEX;

typedef enum VIDDEC_BUFFER_OWNER
{
    VIDDEC_BUFFER_WITH_CLIENT = 0x0,
    VIDDEC_BUFFER_WITH_COMPONENT,
    VIDDEC_BUFFER_WITH_DSP,
    VIDDEC_BUFFER_WITH_TUNNELEDCOMP
} VIDDEC_BUFFER_OWNER;

typedef enum VIDDEC_TYPE_ALLOCATE
{
    VIDDEC_TALLOC_USEBUFFER,
    VIDDEC_TALLOC_ALLOCBUFFER
}VIDDEC_TYPE_ALLOCATE;

typedef enum VIDDEC_INIT_VALUE
{
    VIDDEC_INIT_ALL,
    VIDDEC_INIT_STRUCTS,
    VIDDEC_INIT_VARS,
    VIDDEC_INIT_H263,
    VIDDEC_INIT_H264,
    VIDDEC_INIT_MPEG2,
    VIDDEC_INIT_MPEG4,
    VIDDEC_INIT_WMV9,
#ifdef VIDDEC_SPARK_CODE
    VIDDEC_INIT_SPARK,
#endif
    VIDDEC_INIT_PLANAR420,
    VIDDEC_INIT_INTERLEAVED422,
    VIDDEC_INIT_IDLEEXECUTING,
    VIIDE_INIT_MAX = 0x7ffffff
}VIDDEC_INIT_VALUE;

typedef enum VIDDEC_WMV_PROFILES
{
    VIDDEC_WMV_PROFILE0,
    VIDDEC_WMV_PROFILE1,
    VIDDEC_WMV_PROFILE2,
    VIDDEC_WMV_PROFILE3,
    VIDDEC_WMV_PROFILE4,
    VIDDEC_WMV_PROFILE5,
    VIDDEC_WMV_PROFILE6,
    VIDDEC_WMV_PROFILE7,
    VIDDEC_WMV_PROFILE8,
    VIDDEC_WMV_PROFILEMAX
}VIDDEC_WMV_PROFILES;

typedef struct VIDDEC_BUFFER_PRIVATE
{
    OMX_BUFFERHEADERTYPE* pBufferHdr;
    OMX_PTR pUalgParam;
    OMX_U32 nUalgParamSize;
    VIDDEC_BUFFER_OWNER eBufferOwner;
    VIDDEC_TYPE_ALLOCATE bAllocByComponent;
    OMX_U32 nNumber;
    OMX_U8* pOriginalBuffer;
#ifdef VIDDEC_WMVPOINTERFIXED
     OMX_U8* pTempBuffer;
#endif
} VIDDEC_BUFFER_PRIVATE;

/*structures and defines for Circular Buffer*/
#define VIDDEC_CBUFFER_LOCK
#define MAX_MULTIPLY                        4
#define CBUFFER_SIZE                        MAX_PRIVATE_BUFFERS * MAX_MULTIPLY

typedef enum VIDDEC_QUEUE_TYPES {
    VIDDEC_QUEUE_OMX_U32,
    VIDDEC_QUEUE_OMX_MARKTYPE
} VIDDEC_QUEUE_TYPES;

typedef struct VIDDEC_QUEUE_TYPE {
    OMX_PTR Elements;
    OMX_U32 CounterElements[VIDDEC_MAX_QUEUE_SIZE];
    OMX_U32 nHead;
    OMX_S32 nTail;
    OMX_U32 nElements;
    OMX_U32 nErrorCount;
    pthread_mutex_t mMutex;
}VIDDEC_QUEUE_TYPE;

typedef enum VIDDEC_CBUFFER_TYPE {
    VIDDEC_CBUFFER_MARKDATA,
    VIDDEC_CBUFFER_TIMESTAMP,
    VIDDEC_CBUFFER_CMDMARKDATA
} VIDDEC_CBUFFER_TYPE;

typedef struct VIDDEC_CIRCULAR_BUFFER {
    OMX_PTR pElement[CBUFFER_SIZE];
    VIDDEC_CBUFFER_TYPE nType;
#ifdef VIDDEC_CBUFFER_LOCK
    pthread_mutex_t* m_lock;
#endif
    OMX_U8 nTail;
    OMX_U8 nHead;
    OMX_U8 nCount;
} VIDDEC_CIRCULAR_BUFFER;

typedef struct VIDDEC_CBUFFER_BUFFERFLAGS{
    OMX_TICKS       nTimeStamp;
    OMX_U32         nFlags;
    OMX_U32         nTickCount;
    OMX_PTR         pMarkData;
    OMX_HANDLETYPE  hMarkTargetComponent;
} VIDDEC_CBUFFER_BUFFERFLAGS;

typedef struct VIDDEC_PORT_TYPE
{
    OMX_HANDLETYPE hTunnelComponent;
    OMX_U32 nTunnelPort;
    OMX_BUFFERSUPPLIERTYPE eSupplierSetting;
    VIDDEC_BUFFER_PRIVATE* pBufferPrivate[MAX_PRIVATE_BUFFERS];
    OMX_U8 nBufferCnt;
    VIDDEC_CIRCULAR_BUFFER eTimeStamp;
} VIDDEC_PORT_TYPE;

typedef struct VIDDEC_MUTEX{
    OMX_BOOL bEnabled;
    OMX_BOOL bSignaled;
    OMX_BOOL bInitialized;
    OMX_S32 nErrorExist;
    pthread_mutex_t mutex;
    pthread_cond_t condition;
} VIDDEC_MUTEX;

typedef struct VIDDEC_SEMAPHORE{
    OMX_BOOL bEnabled;
    OMX_BOOL bSignaled;
    OMX_BOOL bInitialized;
    OMX_S32 nErrorExist;
    sem_t sSemaphore;
} VIDDEC_SEMAPHORE;

#ifdef VIDDEC_ACTIVATEPARSER
typedef struct VIDDEC_Point{
    OMX_S32 nX;
    OMX_S32 nY;
}VIDDEC_Point;

typedef struct VIDDEC_Rect{
    VIDDEC_Point sTl;
    VIDDEC_Point sBr;
}VIDDEC_Rect;

typedef struct VIDDEC_Size{
    OMX_S32 nWidth;
    OMX_S32 nHeight;
}VIDDEC_Size;

typedef struct VIDDEC_VideoPictureHeader{
    OMX_U32 nOptions;
    OMX_S32 nProfile;
    OMX_S32 nLevel;
    OMX_S32 nVersion;
    OMX_U8* cnOptional;
    VIDDEC_Size sSizeInMemory;
    VIDDEC_Rect sDisplayedRect;
    OMX_TICKS nPresentationTimestamp;
    OMX_TICKS nDecodingTimestamp;
    OMX_U32 nPreDecoderBufferSize;
    OMX_U32 nPostDecoderBufferSize;
    OMX_U32 nPictureNumber;
    OMX_U32 nPictureLayer;
    OMX_U8* pcSupplementalData;
    OMX_BOOL bIsRandomAccessible;
    OMX_S32 nRandomAccessBufferingPeriod;
    OMX_U32 nRandomAccessBufferOccupancy;
}VIDDEC_VideoPictureHeader;

typedef struct VIDDEC_MPEG4VisualVbvParams{
    OMX_TICKS nBitRate;
    OMX_U32 nVbvBufferSize;
    OMX_U32 nVbvOccupancy;
}VIDDEC_MPEG4VisualVbvParams;

typedef struct VIDDEC_MPEG4VisualVOLHeader{
    OMX_U32 nVideoObjectLayerId;
    OMX_BOOL bShortVideoHeader;
    OMX_BOOL bRandomAccessibleVOL;
    OMX_U32 nVideoObjectTypeIndication;
    OMX_U32 nVideoObjectLayerVerId;
    OMX_U32 nVideoObjectLayerPriority;
    OMX_U32 nAspectRatioNum;
    OMX_U32 nAspectRatioDenom;
    VIDDEC_MPEG4VisualVbvParams sVbvParams;
    OMX_U16 nVOPTimeIncrementResolution;
    OMX_BOOL bnFnXedVopRate;
    OMX_U16 nFnXedVOPTimeIncrement;
    OMX_BOOL bDataPartitioning;
    OMX_BOOL bReversibleVLC;
    OMX_U8* pcUserData;
}VIDDEC_MPEG4VisualVOLHeader ;

typedef struct VIDDEC_MPEG4YuvConversionMatrix{
    OMX_S32 iPostOffset;
    OMX_S32 iMatrix;
    OMX_S32 iPreOffset;
}VIDDEC_MPEG4YuvConversionMatrix;

typedef struct VIDDEC_MPEG4YuvFormat{
    OMX_U32 iCoefficients;
    OMX_U32 iPattern;
    OMX_U32 iDataLayout;
    VIDDEC_MPEG4YuvConversionMatrix * iYuv2RgbMatrix;
    VIDDEC_MPEG4YuvConversionMatrix * iRgb2YuvMatrix;
    OMX_U32 iAspectRatioNum;
    OMX_U32 iAspectRatioDenom;
}VIDDEC_MPEG4YuvFormat;

typedef struct VIDDEC_MPEG4UncompressedVideoFormat{
    OMX_U32 iDataFormat;
    VIDDEC_MPEG4YuvFormat iYuvFormat;
    OMX_U32 iRgbFormat;
}VIDDEC_MPEG4UncompressedVideoFormat;

typedef struct VIDDEC_AVC_ParserParam {
    OMX_U32 nBitPosTemp;
    OMX_U32 nForbiddenZeroBit;
    OMX_U32 nNalRefIdc;
    OMX_U32 nProfileIdc;
    OMX_U32 nConstraintSet0Flag;
    OMX_U32 nConstraintSet1Flag;
    OMX_U32 nConstraintSet2Flag;
    OMX_U32 nReservedZero5bits;
    OMX_U32 nLevelIdc;
    OMX_U32 nSeqParameterSetId;
    OMX_U32 nLog2MaxFrameNumMinus4;
    OMX_U32 nPicOrderCntType;
    OMX_U32 nLog2MaxPicOrderCntLsbMinus4;
    OMX_S32 nOffsetForNonRefPic;
    OMX_S32 nOffsetForTopToBottomField;
    OMX_U32 nNumRefFramesInPicOrderCntCycle;
    OMX_U32 nNumRefFrames;
    OMX_S32 nGapsInFrameNumValueAllowedFlag;
    OMX_U32 nPicWidthInMbsMinus1;
    OMX_U32 nPicHeightInMapUnitsMinus1;
    OMX_U32 nFrameMbsOnlyFlag;
    OMX_S32 nMBAdaptiveFrameFieldFlag ;
    OMX_U32 nDirect8x8InferenceFlag;
    OMX_U32 nFrameCroppingFlag;
    OMX_U32 nFrameCropLeftOffset;
    OMX_U32 nFrameCropRightOffset;
    OMX_U32 nFrameCropTopOffset;
    OMX_U32 nFrameCropBottomOffset;
}VIDDEC_AVC_ParserParam;

typedef struct VIDDEC_MPEG4_ParserParam {
    OMX_U32 nIsVisualObjectIdentifier;
    OMX_U32 nVisualObjectType;
    OMX_U32 nVideoSignalType;
    OMX_U32 nVideoFormat;
    OMX_U32 nVideoRange;
    OMX_U32 nColorDescription;
    OMX_U32 NBitZero;
    OMX_U32 nVideoObjectTypeIndication;
    OMX_U32 nIsVisualObjectLayerIdentifier;
    OMX_U32 nLayerVerId;
    OMX_U32 nLayerPriority;
    OMX_U32 nAspectRadio;
    OMX_U32 nParWidth;
    OMX_U32 nParHeight;
    OMX_U32 nControlParameters;
    OMX_U32 nChromaFormat;
    OMX_U32 nLowDelay;
    OMX_U32 nVbvParameters;
    OMX_U32 nBitRate;
    OMX_U32 nFirstHalfVbvBufferSize;
    OMX_U32 nLatterHalfVbvBufferSize;
    OMX_U32 nFirstHalfVbvOccupancy;
    OMX_U32 nLatterHalfVbvOccupancy;
    OMX_U32 nLayerShape;
    OMX_U32 nTimeIncrementResolution;
    OMX_U32 nFnXedVopRate;
    OMX_U32 nNum_bits;
    OMX_U32 nInterlaced;
    OMX_U32 nObmc;
    OMX_U32 nUFEP;
    OMX_U32 NSpriteNotSupported;
    OMX_U32 nNot8Bit;
    OMX_U32 nQuantPrecision;
    OMX_U32 nBitsPerPnXel;
    OMX_U32 nIsInverseQuantMethodFirst;
    OMX_U32 nComplexityEstimationDisable;
    OMX_U32 nIsResyncMarkerDisabled;
    OMX_U32 nIsDataPartitioned;
    OMX_U32 nRvlc;
    OMX_U32 nScalability;
    OMX_S32 nSourceFormat;
    OMX_BOOL nOutputFormat;
    OMX_U32 nCPM;
    OMX_U32 nPWI;
    OMX_U32 nPHI;
}VIDDEC_MPEG4_ParserParam;
#endif

#define VIDDEC_RCV_EXTHEADER_SIZE 4

typedef struct VIDDEC_WMV_RCV_struct {
    OMX_U32 nNumFrames : 24;
    OMX_U32 nFrameType : 8;
    OMX_U32 nID : 32;
    OMX_U32 nStructData : 32;
    OMX_U32 nVertSize;
    OMX_U32 nHorizSize;
} VIDDEC_WMV_RCV_struct;

typedef union VIDDEC_WMV_RCV_header {
    VIDDEC_WMV_RCV_struct sStructRCV;
    OMX_U8 pBuffer[sizeof(VIDDEC_WMV_RCV_struct)];
} VIDDEC_WMV_RCV_header;

typedef struct VIDDEC_SAVE_BUFFER{
    OMX_BOOL    bSaveFirstBuffer;
    OMX_PTR     pFirstBufferSaved;
    OMX_S32     nFilledLen;
}VIDDEC_SAVE_BUFFER;

#ifdef ANDROID 
/** Opencore specific, refer to OpenMax Call Sequences document section 3.2 **/
typedef struct PV_OMXComponentCapabilityFlagsType
{
    ////////////////// OMX COMPONENT CAPABILITY RELATED MEMBERS
    OMX_BOOL iIsOMXComponentMultiThreaded;
    OMX_BOOL iOMXComponentSupportsExternalOutputBufferAlloc;
    OMX_BOOL iOMXComponentSupportsExternalInputBufferAlloc;
    OMX_BOOL iOMXComponentSupportsMovableInputBuffers;
    OMX_BOOL iOMXComponentSupportsPartialFrames;
    OMX_BOOL iOMXComponentUsesNALStartCodes;
    OMX_BOOL iOMXComponentCanHandleIncompleteFrames;
    OMX_BOOL iOMXComponentUsesFullAVCFrames;
} PV_OMXComponentCapabilityFlagsType;
#endif

typedef struct VIDEO_PROFILE_LEVEL
{
    OMX_S32  nProfile;
    OMX_S32  nLevel;
} VIDEO_PROFILE_LEVEL_TYPE;

/**
 * Data structure used to ...
 *
 * STRUCT MEMBERS:
 *  pPortParamType        : Add desc here...
 *  pInPortDef            : Add desc here...
 *  pOutPortDef           : Add desc here...
 *  pInPortFormat         : Add desc here...
 */
typedef struct VIDDEC_COMPONENT_PRIVATE
{
    OMX_PARAM_PORTDEFINITIONTYPE* pInPortDef;
    OMX_PARAM_PORTDEFINITIONTYPE* pOutPortDef;
    OMX_VIDEO_PARAM_PORTFORMATTYPE* pInPortFormat;
    OMX_VIDEO_PARAM_PORTFORMATTYPE* pOutPortFormat;
    OMX_PRIORITYMGMTTYPE* pPriorityMgmt;
    OMX_PARAM_BUFFERSUPPLIERTYPE* pInBufSupplier;
    OMX_PARAM_BUFFERSUPPLIERTYPE* pOutBufSupplier;
    OMX_VIDEO_PARAM_AVCTYPE* pH264;
    OMX_VIDEO_PARAM_MPEG4TYPE* pMpeg4;
    OMX_VIDEO_PARAM_H263TYPE* pH263;
    OMX_VIDEO_PARAM_WMVTYPE* pWMV;
    OMX_VIDEO_PARAM_MPEG2TYPE* pMpeg2; /* OMX_IndexParamVideoMpeg2 */
    OMX_PORT_PARAM_TYPE* pPortParamType;
    OMX_PARAM_DEBLOCKINGTYPE* pDeblockingParamType;
#ifdef __STD_COMPONENT__
    OMX_PORT_PARAM_TYPE* pPortParamTypeAudio;
    OMX_PORT_PARAM_TYPE* pPortParamTypeImage;
    OMX_PORT_PARAM_TYPE* pPortParamTypeOthers;

#endif
    OMX_CALLBACKTYPE cbInfo;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef[NUM_OF_PORTS];
    OMX_U8 numPorts;
    OMX_COMPONENTTYPE* pHandle;
    OMX_STATETYPE eState;
    OMX_VERSIONTYPE pComponentVersion;
    OMX_VERSIONTYPE pSpecVersion;
    OMX_STRING cComponentName;
    pthread_t ComponentThread;
    int free_inpBuf_Q[2];
    int free_outBuf_Q[2];
    int filled_inpBuf_Q[2];
    int filled_outBuf_Q[2];
    int cmdPipe[2];
    int cmdDataPipe[2];
    OMX_U32 bIsStopping;
    OMX_U32 bIsPaused;
    OMX_U32 bTransPause;
    OMX_U32 ProcessMode;
    OMX_U32 H264BitStreamFormat;
    OMX_BOOL MPEG4Codec_IsTI;
    OMX_BUFFERHEADERTYPE pTempBuffHead;  /*Used for EOS logic*/
    OMX_U32 app_nBuf;
    OMX_U32 lcml_compID;
    void* pLcmlHandle;
    void* pModLCML;
    OMX_U16 arr[100];
    int frameCounter;
    LCML_DSP_INTERFACE* pLCML;
    VIDDEC_PORT_TYPE* pCompPort[NUM_OF_PORTS];
    VIDDEC_WMV_PROFILES wmvProfile;
#ifdef __PERF_INSTRUMENTATION__
    PERF_OBJHANDLE pPERF, pPERFcomp;
    OMX_U32 lcml_nCntOpReceived;
    OMX_U32 lcml_nCntIp;
#endif
#ifdef KHRONOS_1_1
    OMX_PARAM_COMPONENTROLETYPE componentRole;
    /*MBError Reporting code*/
    OMX_CONFIG_MBERRORREPORTINGTYPE eMBErrorReport;
    OMX_CONFIG_MACROBLOCKERRORMAPTYPE_TI eMBErrorMapType[MAX_PRIVATE_BUFFERS];
    OMX_U8 cMBErrorIndexIn;
    OMX_U8 cMBErrorIndexOut;
#endif
    OMX_U8 nInMarkBufIndex;                          /* for OMX_MARKTYPE */
    OMX_U8 nOutMarkBufIndex;                         /* for OMX_MARKTYPE */
    OMX_MARKTYPE arrMarkBufIndex[VIDDEC_MAX_QUEUE_SIZE]; /* for OMX_MARKTYPE */

    OMX_U8 nInCmdMarkBufIndex;                          /* for OMX_MARKTYPE */
    OMX_U8 nOutCmdMarkBufIndex;                         /* for OMX_MARKTYPE */
    OMX_MARKTYPE arrCmdMarkBufIndex[VIDDEC_MAX_QUEUE_SIZE]; /* for OMX_MARKTYPE */
    OMX_U8 nInBufIndex;                          /* for time stamps */
    OMX_U8 nOutBufIndex;                         /* for time stamps */
    OMX_U64 arrBufIndex[VIDDEC_MAX_QUEUE_SIZE]; /* for time stamps */
    OMX_MARKTYPE           MTbuffMark;
    VIDDEC_QUEUE_TYPE      qBuffMark;
    VIDDEC_QUEUE_TYPE      qCmdMarkData;
    VIDDEC_QUEUE_TYPE      qBytesSent;
    OMX_U32                nBytesConsumed;
    OMX_BOOL               bBuffMarkTaked;
    OMX_BOOL               bBuffalreadyMarked;

    OMX_STATETYPE eIdleToLoad;
    OMX_STATETYPE eExecuteToIdle;
    OMX_BOOL iEndofInputSent;
    OMX_BOOL bPipeCleaned;
    OMX_BOOL bFirstBuffer;

    OMX_BOOL bParserEnabled;
    OMX_BOOL bFlushOut;
    void* pUalgParams;
    OMX_BOOL bLCMLHalted;
    OMX_BOOL bMult16Size;
    OMX_BOOL bFirstHeader;
    OMX_BOOL bDynamicConfigurationInProgress;
    OMX_BOOL bInPortSettingsChanged;
    OMX_BOOL bOutPortSettingsChanged;
    VIDDEC_SAVE_BUFFER eFirstBuffer;


#ifndef UNDER_CE
    OMX_BOOL bLCMLOut;
#endif
    VIDDEC_RMPROXY_STATES eRMProxyState;

    OMX_U8 nCountInputBFromDsp;
    OMX_U8 nCountOutputBFromDsp;
    OMX_U8 nCountInputBFromApp;
    OMX_U8 nCountOutputBFromApp;

    VIDDEC_CBUFFER_BUFFERFLAGS aBufferFlags[CBUFFER_SIZE];
    VIDDEC_LCML_STATES eLCMLState;
    OMX_U32 nWMVFileType;
    OMX_BOOL bIsNALBigEndian;
#ifdef VIDDEC_SPARK_CODE
    OMX_BOOL bIsSparkInput;
#endif
    VIDDEC_MUTEX sMutex;
    pthread_mutex_t mutexInputBFromApp;
    pthread_mutex_t mutexOutputBFromApp;
    pthread_mutex_t mutexInputBFromDSP;
    pthread_mutex_t mutexOutputBFromDSP;
    VIDDEC_MUTEX inputFlushCompletionMutex;
    VIDDEC_MUTEX outputFlushCompletionMutex;
    OMX_BOOL bIsInputFlushPending;
    OMX_BOOL bIsOutputFlushPending;
    VIDDEC_MUTEX sDynConfigMutex;
    VIDDEC_SEMAPHORE sInSemaphore;
    VIDDEC_SEMAPHORE sOutSemaphore;
    /* used by RM callback */
#ifdef RESOURCE_MANAGER_ENABLED
    RMPROXY_CALLBACKTYPE rmproxyCallback;
#endif
    /* used to keep track of preempted state */
    OMX_BOOL bPreempted;
    /* desired state of this component */
    OMX_STATETYPE eDesiredState;
    VIDDEC_WMV_RCV_header pBufferRCV;
    OMX_BUFFERHEADERTYPE pBufferTemp;
    OMX_U32 pRCVExtendedHeader;
    OMX_U32 nMemUsage[VIDDEC_MEMLEVELS];
    OMX_U32 nDisplayWidth;
    OMX_U8* pCodecData; /* codec-specific data coming from the demuxer */
    OMX_U32 nCodecDataSize;
    OMX_BOOL bVC1Fix;
#ifdef ANDROID /* Specific flag for opencore mmframework */
    PV_OMXComponentCapabilityFlagsType* pPVCapabilityFlags;
#endif

    /* Used to handle config buffer fragmentation on AVC*/
    OMX_BOOL bConfigBufferCompleteAVC;
    OMX_PTR pInternalConfigBufferAVC;
    OMX_U32 nInternalConfigBufferFilledAVC;
    struct OMX_TI_Debug dbg;
    /* track number of codec config data (CCD) units and sizes */
    OMX_U32 aCCDsize[MAX_CCD_CNT];
    OMX_U32 nCCDcnt;

    /* indicate if codec config data (CCD)
     * buffer (e.g. SPS/PPS) has been copied
     * to the data buffer.  SPS,PPS,NAL1,...
     * */
    OMX_BOOL bCopiedCCDBuffer;

    /* Reference count for pending state change requests */
    OMX_U32 nPendingStateChangeRequests;
    pthread_mutex_t mutexStateChangeRequest;
    pthread_cond_t StateChangeCondition;

    // Signal first buffer after config data should have EOS flag
    OMX_BOOL firstBufferEos;

} VIDDEC_COMPONENT_PRIVATE;

/*****************macro definitions*********************/
/*----------------------------------------------------------------------------*/
/**
  * OMX_GET_DATABUFF_SIZE() Get the needed buffer data size base in the request.
  *
  * This method will give the needed data buffer size acording with
  * specific requirements from the codec and component.
  *
  * @param _nSizeBytes_     Requested size from client
  *
  **/
/*----------------------------------------------------------------------------*/

#define OMX_GET_DATABUFF_SIZE(_nSizeBytes_)                         \
         (_nSizeBytes_ + VIDDEC_PADDING_FULL + VIDDEC_WMV_BUFFER_OFFSET + VIDDEC_ALIGNMENT)


#define OMX_MALLOC_STRUCT(_pStruct_, _sName_, _memusage_)           \
    _pStruct_ = (_sName_*)malloc(sizeof(_sName_));                  \
    if(_pStruct_ == NULL){                                          \
            eError = OMX_ErrorInsufficientResources;                \
                goto EXIT;                                          \
    }                                                               \
    /*(_memusage_) += sizeof(_sName_);                               */ \
    memset((_pStruct_), 0x0, sizeof(_sName_));


#define OMX_MALLOC_STRUCT_SIZED(_pStruct_, _sName_, _nSize_, _memusage_)    \
    _pStruct_ = (_sName_*)malloc(_nSize_);                                  \
    if(_pStruct_ == NULL){                                                  \
            eError = OMX_ErrorInsufficientResources;                        \
                goto EXIT;                                                  \
    }                                                                       \
    /*(_memusage_) += _nSize_;                                               */ \
    memset((_pStruct_), 0x0, _nSize_);

#define VIDDEC_MEMUSAGE 0 /*\
    pComponentPrivate->nMemUsage[VIDDDEC_Enum_MemLevel0] + \
    pComponentPrivate->nMemUsage[VIDDDEC_Enum_MemLevel1] + \
    pComponentPrivate->nMemUsage[VIDDDEC_Enum_MemLevel2] + \
    pComponentPrivate->nMemUsage[VIDDDEC_Enum_MemLevel3] + \
    pComponentPrivate->nMemUsage[VIDDDEC_Enum_MemLevel4]*/


/*----------------------------------------------------------------------------*/
/**
  * OMX_ALIGN_BUFFER() Align the buffer to the desire number of bytes.
  *
  * This method will update the component function pointer to the handle
  *
  * @param _pBuffer_     Pointer to align
  * @param _nBytes_      # of byte to alignment desire
  *
  **/
/*----------------------------------------------------------------------------*/

#define OMX_ALIGN_BUFFER(_pBuffer_, _nBytes_)                  \
    while((OMX_U8)_pBuffer_ & (_nBytes_-1)){                   \
       _pBuffer_++;                                            \
    }



#define OMX_MALLOC_BUFFER_VIDDEC(_pBuffer_, _nSize_, _pOriginalBuffer_)	    \
    _pBuffer_ =  OMX_MALLOC_STRUCT_SIZED(_pBuffer_, OMX_U8, _nSize_ + VIDDEC_PADDING_FULL + VIDDEC_WMV_BUFFER_OFFSET + VIDDEC_ALIGNMENT, pComponentPrivate->nMemUsage[VIDDDEC_Enum_MemLevel1]);			\
    _pOriginalBuffer_ = _pBuffer_;					    \
    _pBuffer_ += VIDDEC_PADDING_HALF;					    \
    OMX_ALIGN_BUFFER(_pBuffer_, VIDDEC_ALIGNMENT);


/*----------------------------------------------------------------------------*/
/**
  * OMX_FREE() Free memory
  *
  * This method will free memory and set pointer to NULL
  *
  * @param _pBuffer_     Pointer to free
  *
  **/
/*----------------------------------------------------------------------------*/

#define OMX_FREE_VIDDEC(_pBuffer_)					    \
    if(_pBuffer_ != NULL){                                                  \
	free(_pBuffer_);                                                    \
	_pBuffer_ = NULL;                                                   \
    }



/*----------------------------------------------------------------------------*/
/**
  * OMX_FREE_BUFFER_VIDDEC() Free video decoder buffer
  *
  * This method will free video decoder buffer
  *
  * @param _pBuffHead_	    Buffer header pointer
  * @param _pCompPort_	    Component port will give us the reference to the
  *			    desire buffer to free
  *
  **/
/*----------------------------------------------------------------------------*/

#define OMX_FREE_BUFFER_VIDDEC(_pBuffHead_, _pCompPort_)					    \
    {												    \
	int _nBufferCount_ = 0;									    \
	OMX_U8* _pTemp_ = NULL;									    \
												    \
	for(_nBufferCount_ = 0; _nBufferCount_ < MAX_PRIVATE_BUFFERS; _nBufferCount_++){	    \
            if(_pCompPort_->pBufferPrivate[_nBufferCount_]->pBufferHdr != NULL){		    \
                _pTemp_ = (OMX_U8*)_pCompPort_->pBufferPrivate[_nBufferCount_]->pBufferHdr->pBuffer;	\
                if(_pBuffHead_->pBuffer == _pTemp_){						    \
                    break;									    \
                }										    \
	    }											    \
        }											    \
												    \
        if(_nBufferCount_ == MAX_PRIVATE_BUFFERS){						    \
            OMX_ERROR4(pComponentPrivate->dbg, "Error: Buffer NOT found to free: %p \n", _pBuffHead_->pBuffer);	    \
            goto EXIT;										    \
        }											    \
												    \
        _pBuffHead_->pBuffer = _pCompPort_->pBufferPrivate[_nBufferCount_]->pOriginalBuffer;		    \
        OMX_PRBUFFER1(pComponentPrivate->dbg, "Free original allocated buffer: %p\n", _pBuffHead_->pBuffer);	\
        OMX_FREE_VIDDEC(_pBuffHead_->pBuffer);							    \
    }
/*----------------------------------------------------------------------------*/
/**
  * OMX_WMV_INSERT_CODEC_DATA()
  *
  * This method will insert the codec data to the first frame to be sent to
  * queue in LCML
  *
  * @param _pBuffHead_    Pointer to free
  * @param _pComponentPrivate_  Component private structure to provide needed
  *                             references
  *
  **/
/*----------------------------------------------------------------------------*/

#define OMX_WMV_INSERT_CODEC_DATA(_pBuffHead_, _pComponentPrivate_)                     \
    {                                                                                   \
        OMX_U8* _pTempBuffer_ = NULL;                                                   \
        /* Copy frame data in a temporary buffer*/                                      \
        OMX_MALLOC_STRUCT_SIZED(_pTempBuffer_, OMX_U8, _pBuffHead_->nFilledLen, NULL);  \
        memcpy (_pTempBuffer_, _pBuffHead_->pBuffer, _pBuffHead_->nFilledLen);          \
                                                                                        \
        /*Copy configuration data at the begining of the buffer*/                       \
        memcpy (_pBuffHead_->pBuffer, _pComponentPrivate_->pCodecData, _pComponentPrivate_->nCodecDataSize);   \
        _pBuffHead_->pBuffer += _pComponentPrivate_->nCodecDataSize;                                           \
        /* Add frame start code */     \
        (*(_pBuffHead_->pBuffer++)) = 0x00;  \
        (*(_pBuffHead_->pBuffer++)) = 0x00;  \
        (*(_pBuffHead_->pBuffer++)) = 0x01;  \
        (*(_pBuffHead_->pBuffer++)) = 0x0d;  \
                                             \
        /* Insert again the frame buffer */  \
        memcpy (_pBuffHead_->pBuffer, _pTempBuffer_, _pBuffHead_->nFilledLen); \
        /* pTempBuffer no longer need*/                                        \
	OMX_FREE_VIDDEC(_pTempBuffer_);							\
                             \
        _pBuffHead_->pBuffer -= (pComponentPrivate->nCodecDataSize + 4);       \
        _pBuffHead_->nFilledLen += pComponentPrivate->nCodecDataSize + 4;      \
    }




#define OMX_CONF_INIT_STRUCT(_s_, _name_, dbg)       \
    memset((_s_), 0x0, sizeof(_name_));         \
    (_s_)->nSize = sizeof(_name_);              \
    (_s_)->nVersion.s.nVersionMajor = VERSION_MAJOR;      \
    (_s_)->nVersion.s.nVersionMinor = VERSION_MINOR;      \
    (_s_)->nVersion.s.nRevision = VERSION_REVISION;       \
    (_s_)->nVersion.s.nStep = VERSION_STEP;                                             \
    OMX_PRINT0(dbg, "INIT_STRUCT Major 0x%x Minor 0x%x nRevision 0x%x nStep 0x%x\n", \
        (_s_)->nVersion.s.nVersionMajor, (_s_)->nVersion.s.nVersionMinor, \
        (_s_)->nVersion.s.nRevision, (_s_)->nVersion.s.nStep);

#define OMX_CONF_CHK_VERSION(_s_, _name_, _e_, dbg)              \
    OMX_PRINT0(dbg, "CHK_VERSION Size 0x%lx Major 0x%x Minor 0x%x nRevision 0x%x nStep 0x%x\n", \
        (_s_)->nSize, (_s_)->nVersion.s.nVersionMajor, (_s_)->nVersion.s.nVersionMinor, \
        (_s_)->nVersion.s.nRevision, (_s_)->nVersion.s.nStep);              \
    if((_s_)->nSize != sizeof(_name_)) _e_ = OMX_ErrorBadParameter; \
    if(((_s_)->nVersion.s.nVersionMajor != VERSION_MAJOR)||         \
       ((_s_)->nVersion.s.nVersionMinor != VERSION_MINOR)||         \
       ((_s_)->nVersion.s.nRevision != VERSION_REVISION)||              \
       ((_s_)->nVersion.s.nStep != VERSION_STEP)) _e_ = OMX_ErrorVersionMismatch;\
    if(_e_ != OMX_ErrorNone) goto EXIT;

#define OMX_CONF_CHECK_CMD(_ptr1, _ptr2, _ptr3) \
{                                               \
    if(!_ptr1 || !_ptr2 || !_ptr3){             \
        eError = OMX_ErrorBadParameter;         \
        goto EXIT;                              \
    }                                           \
}

#define OMX_CONF_SET_ERROR_BAIL(_eError, _eCode)\
{                                               \
    _eError = _eCode;                           \
    goto EXIT;                                  \
}


#define OMX_PARSER_CHECKLIMIT(_total, _actual, _step) /*  \
    if(((_actual + _step) >> 3) >= _total){                \
    printf("_total %d _actual %d\n",_total,((_actual + _step)>>3)); \
        eError = OMX_ErrorStreamCorrupt;                \
        goto EXIT;                                      \
    }*/

/*sMutex*/
#define VIDDEC_PTHREAD_MUTEX_INIT(_mutex_)    \
    if(!((_mutex_).bInitialized)) {            \
        pthread_mutex_init (&((_mutex_).mutex), NULL);   \
        pthread_cond_init (&(_mutex_).condition, NULL);\
        (_mutex_).bInitialized = OMX_TRUE;   \
        (_mutex_).bSignaled = OMX_FALSE;     \
        (_mutex_).bEnabled = OMX_FALSE;      \
    }

#define VIDDEC_PTHREAD_MUTEX_DESTROY(_mutex_) \
    if((_mutex_).bInitialized) {             \
        pthread_mutex_destroy (&((_mutex_).mutex));     \
        pthread_cond_destroy (&(_mutex_).condition); \
        (_mutex_).bInitialized = OMX_FALSE;  \
        (_mutex_).bEnabled = OMX_FALSE;      \
    }

#define VIDDEC_PTHREAD_MUTEX_LOCK(_mutex_)    \
    VIDDEC_PTHREAD_MUTEX_INIT ((_mutex_));     \
    (_mutex_).bSignaled = OMX_FALSE;         \
    (_mutex_).nErrorExist = 0; \
    (_mutex_).nErrorExist = pthread_mutex_lock (&(_mutex_).mutex);

#define VIDDEC_PTHREAD_MUTEX_UNLOCK(_mutex_)  \
    VIDDEC_PTHREAD_MUTEX_INIT ((_mutex_));      \
    (_mutex_).nErrorExist = 0; \
    (_mutex_).nErrorExist = pthread_mutex_unlock (&(_mutex_).mutex);

#define VIDDEC_PTHREAD_MUTEX_TRYLOCK(_mutex_) \
    VIDDEC_PTHREAD_MUTEX_INIT ((_mutex_));      \
    (_mutex_).nErrorExist = 0; \
    (_mutex_).nErrorExist = pthread_mutex_trylock (&(_mutex_).mutex);

#define VIDDEC_PTHREAD_MUTEX_SIGNAL(_mutex_)  \
    VIDDEC_PTHREAD_MUTEX_INIT ((_mutex_));      \
    /*if( (_mutex_).bEnabled) {  */              \
    (_mutex_).nErrorExist = 0; \
    (_mutex_).nErrorExist = pthread_cond_signal (&(_mutex_).condition); \
        /*(__mutex.bSignaled = OMX_TRUE;*/  \
    /*}*/

#define VIDDEC_PTHREAD_MUTEX_WAIT(_mutex_)    \
    VIDDEC_PTHREAD_MUTEX_INIT ((_mutex_));      \
    (_mutex_).bEnabled = OMX_TRUE;           \
    /*if (!(__mutex.bSignaled){               */\
    (_mutex_).nErrorExist = 0; \
    (_mutex_).nErrorExist = pthread_cond_wait (&(_mutex_).condition, &(_mutex_).mutex);  \
        (_mutex_).bSignaled = OMX_FALSE;     \
        (_mutex_).bEnabled = OMX_FALSE;      \
    /*}*/

#define VIDDEC_PTHREAD_SEMAPHORE_INIT(_semaphore_)    \
    if(!((_semaphore_).bInitialized)) {            \
        sem_init (&((_semaphore_).sSemaphore), 0, 0);   \
        (_semaphore_).bInitialized = OMX_TRUE;   \
        (_semaphore_).bEnabled = OMX_FALSE;      \
        (_semaphore_).bSignaled = OMX_FALSE;        \
    }

#define VIDDEC_PTHREAD_SEMAPHORE_DESTROY(_semaphore_) \
    if((_semaphore_).bInitialized) {             \
        sem_destroy (&(_semaphore_).sSemaphore);     \
        (_semaphore_).bInitialized = OMX_FALSE;  \
        (_semaphore_).bSignaled = OMX_FALSE;     \
        (_semaphore_).bEnabled = OMX_FALSE;      \
    }
    /*
    printf("post signal %d Enable %d\n",(_semaphore_).bSignaled,(_semaphore_).bEnabled); \
    \
    printf("post out signal %d Enable %d\n",(_semaphore_).bSignaled,(_semaphore_).bEnabled); \
    \
    printf("wait out signal %d Enable %d\n",(_semaphore_).bSignaled,(_semaphore_).bEnabled); \
    printf("wait signal %d Enable %d\n",(_semaphore_).bSignaled,(_semaphore_).bEnabled); \

    */
#define VIDDEC_PTHREAD_SEMAPHORE_POST(_semaphore_)    \
    VIDDEC_PTHREAD_SEMAPHORE_INIT ((_semaphore_));     \
    if((_semaphore_).bEnabled) {     \
        sem_post (&(_semaphore_).sSemaphore);       \
        (_semaphore_).bEnabled = OMX_FALSE;      \
    }               \
    else {          \
        (_semaphore_).bSignaled = OMX_TRUE;      \
        (_semaphore_).bEnabled = OMX_FALSE;     \
    }

#define VIDDEC_PTHREAD_SEMAPHORE_WAIT(_semaphore_)  \
    VIDDEC_PTHREAD_SEMAPHORE_INIT ((_semaphore_));      \
    if(!(_semaphore_).bSignaled) {     \
        (_semaphore_).bEnabled = OMX_TRUE;     \
        sem_wait (&(_semaphore_).sSemaphore);   \
    }       \
    else {  \
        (_semaphore_).bEnabled = OMX_FALSE;     \
        (_semaphore_).bSignaled = OMX_FALSE;      \
    }

#define VIDDEC_EXECUTETOIDLE                                \
    (((pComponentPrivate->eState == OMX_StatePause) ||      \
    (pComponentPrivate->eState == OMX_StateExecuting)) &&   \
    (pComponentPrivate->eExecuteToIdle == OMX_StateIdle))

#define VIDDEC_IDLETOEXECUTE                                \
    (((pComponentPrivate->eState == OMX_StateIdle)) &&      \
    (pComponentPrivate->eExecuteToIdle == OMX_StateExecuting))

#ifdef VIDDEC_SPARK_CODE
 #define VIDDEC_SPARKCHECK \
    ((pComponentPrivate->bIsSparkInput) && \
    (pComponentPrivate->pInPortDef->format.video.eCompressionFormat == OMX_VIDEO_CodingUnused))
#else
 #define VIDDEC_SPARKCHECK (OMX_FALSE)
#endif


/* DEFINITIONS for parsing the config information & sequence header for WMV*/
 #define VIDDEC_GetUnalignedDword( pb, dw ) \
             (dw) = ((OMX_U32) *(pb + 3) << 24) + \
                    ((OMX_U32) *(pb + 2) << 16) + \
                    ((OMX_U16) *(pb + 1) << 8) + *pb;
 
 #define VIDDEC_GetUnalignedDwordEx( pb, dw )   VIDDEC_GetUnalignedDword( pb, dw ); (pb) += sizeof(OMX_U32);
 #define VIDDEC_LoadDWORD( dw, p )  VIDDEC_GetUnalignedDwordEx( p, dw )
 #ifndef VIDDEC_MAKEFOURCC
 #define VIDDEC_MAKEFOURCC(ch0, ch1, ch2, ch3) \
         ((OMX_U32)(OMX_U8)(ch0) | ((OMX_U32)(OMX_U8)(ch1) << 8) |   \
         ((OMX_U32)(OMX_U8)(ch2) << 16) | ((OMX_U32)(OMX_U8)(ch3) << 24 ))
 
 #define VIDDEC_FOURCC(ch0, ch1, ch2, ch3)  VIDDEC_MAKEFOURCC(ch0, ch1, ch2, ch3)
 #endif
 
 #define FOURCC_WMV3     VIDDEC_FOURCC('W','M','V','3')
 #define FOURCC_WMV2     VIDDEC_FOURCC('W','M','V','2')
 #define FOURCC_WMV1     VIDDEC_FOURCC('W','M','V','1')
 #define FOURCC_WVC1     VIDDEC_FOURCC('W','V','C','1')
 
 
/*-------function prototypes -------------------------------------------------*/
typedef OMX_ERRORTYPE (*VIDDEC_fpo)(OMX_HANDLETYPE);

OMX_ERRORTYPE OMX_ComponentInit (OMX_HANDLETYPE hComponent);
OMX_ERRORTYPE VIDDEC_Start_ComponentThread (OMX_HANDLETYPE pHandle);
OMX_ERRORTYPE VIDDEC_Stop_ComponentThread(OMX_HANDLETYPE pComponent);
OMX_ERRORTYPE VIDDEC_HandleCommand (OMX_HANDLETYPE pHandle, OMX_U32 nParam1);
OMX_ERRORTYPE VIDDEC_DisablePort (VIDDEC_COMPONENT_PRIVATE* pComponentPrivate, OMX_U32 nParam1);
OMX_ERRORTYPE VIDDEC_EnablePort (VIDDEC_COMPONENT_PRIVATE* pComponentPrivate, OMX_U32 nParam1);
OMX_ERRORTYPE VIDDEC_HandleDataBuf_FromApp (VIDDEC_COMPONENT_PRIVATE* pComponentPrivate);
OMX_ERRORTYPE VIDDEC_HandleDataBuf_FromDsp (VIDDEC_COMPONENT_PRIVATE* pComponentPrivate);
OMX_ERRORTYPE VIDDEC_HandleFreeDataBuf (VIDDEC_COMPONENT_PRIVATE* pComponentPrivate);
OMX_ERRORTYPE VIDDEC_LCML_Callback (TUsnCodecEvent event, void* argsCb [10]);
OMX_ERRORTYPE VIDDEC_HandleFreeOutputBufferFromApp (VIDDEC_COMPONENT_PRIVATE* pComponentPrivate);
OMX_ERRORTYPE VIDDEC_InitDSP_Mpeg4Dec (VIDDEC_COMPONENT_PRIVATE* pComponentPrivate);
OMX_ERRORTYPE VIDDEC_InitDSP_Mpeg2Dec (VIDDEC_COMPONENT_PRIVATE* pComponentPrivate);
OMX_ERRORTYPE VIDDEC_InitDSP_H264Dec (VIDDEC_COMPONENT_PRIVATE* pComponentPrivate);
OMX_ERRORTYPE VIDDEC_InitDSP_WMVDec (VIDDEC_COMPONENT_PRIVATE* pComponentPrivate);
#ifdef VIDDEC_SPARK_CODE
OMX_ERRORTYPE VIDDEC_InitDSP_SparkDec (VIDDEC_COMPONENT_PRIVATE* pComponentPrivate);
#endif
OMX_ERRORTYPE VIDDEC_ReturnBuffers (VIDDEC_COMPONENT_PRIVATE* pComponentPrivate, OMX_U32 nParam1, OMX_BOOL bRetDSP);
OMX_ERRORTYPE VIDDEC_HandleCommandMarkBuffer(VIDDEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U32 nParam1, OMX_PTR pCmdData);
OMX_ERRORTYPE VIDDEC_HandleCommandFlush(VIDDEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U32 nParam1, OMX_BOOL bPass);
OMX_ERRORTYPE VIDDEC_Load_Defaults (VIDDEC_COMPONENT_PRIVATE* pComponentPrivate, OMX_S32 nPassing);
OMX_U32 VIDDEC_GetRMFrecuency(VIDDEC_COMPONENT_PRIVATE* pComponentPrivate);
OMX_ERRORTYPE VIDDEC_Handle_InvalidState (VIDDEC_COMPONENT_PRIVATE* pComponentPrivate);

OMX_ERRORTYPE VIDDEC_CircBuf_Init(VIDDEC_COMPONENT_PRIVATE* pComponentPrivate, VIDDEC_CBUFFER_TYPE nTypeIndex, VIDDEC_PORT_INDEX nPortIndex);
OMX_ERRORTYPE VIDDEC_CircBuf_Flush(VIDDEC_COMPONENT_PRIVATE* pComponentPrivate, VIDDEC_CBUFFER_TYPE nTypeIndex, VIDDEC_PORT_INDEX nPortIndex);
OMX_ERRORTYPE VIDDEC_CircBuf_DeInit(VIDDEC_COMPONENT_PRIVATE* pComponentPrivate, VIDDEC_CBUFFER_TYPE nTypeIndex, VIDDEC_PORT_INDEX nPortIndex);
OMX_ERRORTYPE VIDDEC_CircBuf_Add(VIDDEC_COMPONENT_PRIVATE* pComponentPrivate, VIDDEC_CBUFFER_TYPE nTypeIndex, VIDDEC_PORT_INDEX nPortIndex, OMX_PTR pElement);
OMX_ERRORTYPE VIDDEC_CircBuf_Remove(VIDDEC_COMPONENT_PRIVATE* pComponentPrivate, VIDDEC_CBUFFER_TYPE nTypeIndex, VIDDEC_PORT_INDEX nPortIndex, OMX_PTR* pElement);
OMX_ERRORTYPE VIDDEC_CircBuf_Count(VIDDEC_COMPONENT_PRIVATE* pComponentPrivate, VIDDEC_CBUFFER_TYPE nTypeIndex, VIDDEC_PORT_INDEX nPortIndex, OMX_U8* pCount);
OMX_U8 VIDDEC_CircBuf_GetHead(VIDDEC_COMPONENT_PRIVATE* pComponentPrivate, VIDDEC_CBUFFER_TYPE nTypeIndex, VIDDEC_PORT_INDEX nPortIndex);
OMX_ERRORTYPE VIDDEC_Propagate_Mark(VIDDEC_COMPONENT_PRIVATE* pComponentPrivate, OMX_BUFFERHEADERTYPE *pBuffHead);
OMX_ERRORTYPE VIDDEC_Queue_Init(VIDDEC_QUEUE_TYPE *queue, VIDDEC_QUEUE_TYPES type);
OMX_ERRORTYPE VIDDEC_Queue_Flush(VIDDEC_QUEUE_TYPE *queue);
OMX_ERRORTYPE VIDDEC_Queue_Add(VIDDEC_QUEUE_TYPE *queue, OMX_PTR pElement, VIDDEC_QUEUE_TYPES type);
OMX_ERRORTYPE VIDDEC_Queue_Remove(VIDDEC_QUEUE_TYPE *queue, OMX_PTR pElement, VIDDEC_QUEUE_TYPES type);
OMX_ERRORTYPE VIDDEC_Queue_Replace_Tail(VIDDEC_QUEUE_TYPE *queue, OMX_PTR pElement, VIDDEC_QUEUE_TYPES type);
OMX_ERRORTYPE VIDDEC_Queue_Get_Tail(VIDDEC_QUEUE_TYPE *queue, OMX_PTR pElement,VIDDEC_QUEUE_TYPES type);
OMX_ERRORTYPE VIDDEC_Queue_Free(VIDDEC_QUEUE_TYPE *queue);
#ifdef RESOURCE_MANAGER_ENABLED
void VIDDEC_ResourceManagerCallback(RMPROXY_COMMANDDATATYPE cbData);
#endif
#ifdef ANDROID
OMX_ERRORTYPE VIDDEC_SaveBuffer(VIDDEC_COMPONENT_PRIVATE* pComponentPrivate, OMX_BUFFERHEADERTYPE* pBuffHead);
OMX_ERRORTYPE VIDDEC_CopyBuffer(VIDDEC_COMPONENT_PRIVATE* pComponentPrivate, OMX_BUFFERHEADERTYPE* pBuffHead);
#endif
OMX_ERRORTYPE VIDDEC_UnloadCodec(VIDDEC_COMPONENT_PRIVATE* pComponentPrivate);
OMX_ERRORTYPE VIDDEC_LoadCodec(VIDDEC_COMPONENT_PRIVATE* pComponentPrivate);
OMX_ERRORTYPE VIDDEC_Set_SN_StreamType(VIDDEC_COMPONENT_PRIVATE* pComponentPrivate);
OMX_ERRORTYPE VIDDEC_SetMpeg4_Parameters(VIDDEC_COMPONENT_PRIVATE* pComponentPrivate);

#ifdef VIDDEC_ACTIVATEPARSER
OMX_ERRORTYPE VIDDEC_ParseVideo_WMV9_VC1( OMX_S32* nWidth, OMX_S32* nHeight, OMX_BUFFERHEADERTYPE *pBuffHead);
OMX_ERRORTYPE VIDDEC_ParseVideo_WMV9_RCV( OMX_S32* nWidth, OMX_S32* nHeight, OMX_BUFFERHEADERTYPE *pBuffHead);
OMX_ERRORTYPE VIDDEC_ParseHeader(VIDDEC_COMPONENT_PRIVATE* pComponentPrivate, OMX_BUFFERHEADERTYPE *pBuffHead);
OMX_ERRORTYPE VIDDEC_ParseVideo_MPEG4( OMX_S32* nWidth, OMX_S32* nHeight, OMX_BUFFERHEADERTYPE *pBuffHead);
OMX_ERRORTYPE VIDDEC_ParseVideo_H264(VIDDEC_COMPONENT_PRIVATE* pComponentPrivate,
                                     OMX_BUFFERHEADERTYPE* pBuffHead,OMX_S32* nWidth,
                                     OMX_S32* nHeight, OMX_S32* nCropWidth, OMX_S32* nCropHeight, OMX_U32 nType);
OMX_ERRORTYPE VIDDEC_ParseVideo_MPEG2( OMX_S32* nWidth, OMX_S32* nHeight, OMX_BUFFERHEADERTYPE *pBuffHead);
OMX_U32 VIDDEC_GetBits(OMX_U32* nPosition, OMX_U8 nBits, OMX_U8* pBuffer, OMX_BOOL bIcreasePosition);
OMX_S32 VIDDEC_UVLC_dec(OMX_U32 *nPosition, OMX_U8* pBuffer);
OMX_ERRORTYPE AddStateTransition(VIDDEC_COMPONENT_PRIVATE* pComponentPrivate);
OMX_ERRORTYPE RemoveStateTransition(VIDDEC_COMPONENT_PRIVATE* pComponentPrivate, OMX_BOOL bEnableSignal);
OMX_ERRORTYPE IncrementCount (OMX_U8 * pCounter, pthread_mutex_t *pMutex);
OMX_ERRORTYPE DecrementCount (OMX_U8 * pCounter, pthread_mutex_t *pMutex);

#endif
#endif
