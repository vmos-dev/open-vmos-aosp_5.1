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
/* =============================================================================
* Done            Texas Instruments OMAP(TM) Platform Software
*  (c) Copyright Texas Instruments, Incorporated.  All Rights Reserved.
*
*  Use of this software is controlled by the terms and conditions found
*  in the license agreement under which this software has been supplied.
* ============================================================================ */
/**
* @file OMX_JpegEnc_Utils.h
*
* This is a header file for a JPEG encoder that is fully
* compliant with the OMX Image specification.
* This the file that the application that uses OMX would include
* in its code.
*
* @path  $(CSLPATH)\inc
*
* @rev  0.1
*/
/* -------------------------------------------------------------------------------- */
/* ================================================================================
*!
*! Revision History
*! ===================================
*!
*! 22-May-2006 mf: Revisions appear in reverse chronological order;
*! that is, newest first.  The date format is dd-Mon-yyyy.
* ================================================================================= */

#ifndef OMX_JPEGENC_UTILS__H
#define OMX_JPEGENC_UTILS__H
#include <OMX_Component.h>
#include <OMX_IVCommon.h>


#ifndef UNDER_CE
#include <dlfcn.h>
#endif
#include "LCML_DspCodec.h"
#include "LCML_Types.h"
#include "LCML_CodecInterface.h"
#include <pthread.h>
#include <stdarg.h>
#include <OMX_Core.h>
#include <OMX_Types.h>
#include <OMX_Image.h>
#include<OMX_TI_Common.h>
#include <OMX_TI_Debug.h>
#ifdef RESOURCE_MANAGER_ENABLED
#include <ResourceManagerProxyAPI.h>
#endif
#include "OMX_JpegEnc_CustomCmd.h"

#include <utils/Log.h>
#define LOG_TAG "OMX_JPGENC"

#ifdef __PERF_INSTRUMENTATION__
#include "perf.h"
#endif

#define OMX_JPEGENC_NonMIME 1
#define OMX_NOPORT 0xFFFFFFFE
#define JPEGE_TIMEOUT (100000)
#define NUM_OF_PORTS  2
#define NUM_OF_BUFFERSJPEG 4

#define MAX_INPARAM_SIZE 1024

#define COMP_MAX_NAMESIZE 127

#define OMX_CustomCommandStopThread (OMX_CommandMax - 1)

#define PADDING_128_BYTE	128
#define PADDING_256_BYTE	256
#define JPEGENC_THUMBNAIL_ABSENT_WARNING 4

#ifdef UNDER_CE
    #include <oaf_debug.h>
#endif

#define KHRONOS_1_1

#ifndef FUNC
#define FUNC 1
#endif

#ifdef RESOURCE_MANAGER_ENABLED
#define JPEGENC1MPImage 1000000
#define JPEGENC2MPImage 2000000
#endif

#define DSP_MMU_FAULT_HANDLING

//JPEG Encoder Specific DSP Err Codes
#define IUALG_ERR_INSUFF_BUFFER 0x8401

/*Linked List */

typedef struct Node {
    struct Node *pNextNode;
    void *pValue;
} Node;

typedef struct LinkedList {
    Node *pRoot;
    pthread_mutex_t lock;
}   LinkedList;

LinkedList AllocList;

void LinkedList_Create(LinkedList *LinkedList);
void LinkedList_AddElement(LinkedList *LinkedList, void *pValue);
void LinkedList_FreeElement(LinkedList *LinkedList, void *pValue);
void LinkedList_FreeAll(LinkedList *LinkedList);
void LinkedList_DisplayAll(LinkedList *LinkedList);
void LinkedList_Destroy(LinkedList *LinkedList);

/*
 *     M A C R O S
 */

#define OMX_CONF_INIT_STRUCT(_s_, _name_)   \
    memset((_s_), 0x0, sizeof(_name_)); \
    (_s_)->nSize = sizeof(_name_);      \
    (_s_)->nVersion.s.nVersionMajor = 0x1;  \
    (_s_)->nVersion.s.nVersionMinor = 0x0;  \
    (_s_)->nVersion.s.nRevision = 0x0;      \
    (_s_)->nVersion.s.nStep = 0x0

#define OMX_CHECK_PARAM(_ptr_)  \
{   \
    if(!_ptr_) {    \
    eError = OMX_ErrorBadParameter; \
    goto EXIT; \
    }   \
}

#define OMX_CONF_SET_ERROR_BAIL(_eError, _eCode)\
{                       \
    _eError = _eCode;               \
    goto OMX_CONF_CMD_BAIL;         \
}

#define OMX_MALLOC(_pStruct_, _size_)   \
    _pStruct_ = malloc(_size_);  \
    if(_pStruct_ == NULL){  \
        eError = OMX_ErrorInsufficientResources;    \
        goto EXIT;  \
    } \
    memset(_pStruct_, 0, _size_);\
    LinkedList_AddElement(&AllocList, _pStruct_);

#define OMX_FREE(_ptr)   \
{                     \
    if (_ptr != NULL) { \
        LinkedList_FreeElement(&AllocList, _ptr);\
        _ptr = NULL; \
    }                \
}

#define OMX_FREEALL()   \
{                     \
        LinkedList_FreeAll(&AllocList);\
}

#define OMX_MEMCPY_CHECK(_p_)\
{\
    if (_p_ == NULL) { \
    eError = OMX_ErrorInsufficientResources;  \
    goto EXIT;   \
    } \
}


#ifdef RESOURCE_MANAGER_ENABLED
#define OMX_GET_RM_VALUE(_Res_, _RM_, _dbg_) \
{   \
    if ((_Res_) <= JPEGENC1MPImage){  \
        (_RM_) = 30;  \
        }   \
    else {  \
        (_RM_) = 60;  \
        }   \
    \
        \
    OMX_PRMGR2((_dbg_), "Value in MHz requested to RM = %d\n", (_RM_)); \
}
#endif

typedef struct IDMJPGE_TIGEM_Comment {
  OMX_U8 comment[256];
  OMX_U16 commentLen;
} IDMJPGE_TIGEM_Comment;


typedef struct IIMGENC_DynamicParams {
    OMX_U32 nSize;             /* nSize of this structure */
    OMX_U32 nNumAU;            /* Number of Access unit to encode,
                                  * set to XDM_DEFAULT in case of entire frame
                                  */
    OMX_U32 nInputChromaFormat;/* Input chroma format, Refer above comments regarding chroma                    */
    OMX_U32 nInputHeight;       /* Input nHeight*/
    OMX_U32 nInputWidth;        /* Input nWidth*/
    OMX_U32 nCaptureWidth;      /* 0: use imagewidth as pitch, otherwise:
                                   * use given display nWidth (if > imagewidth)
                                   * for pitch.
                                   */
    OMX_U32 nGenerateHeader;    /* XDM_ENCODE_AU or XDM_GENERATE_HEADER */
    OMX_U32 qValue;            /* Q value compression factor for encoder */
} IIMGENC_DynamicParams;


  typedef struct IDMJPGE_TIGEM_CustomQuantTables
  {
    /* The array "lum_quant_tab" defines the quantization table for the luma component. */
    OMX_U16 lum_quant_tab[64];
    /* The array "chm_quant_tab" defines the quantization table for the chroma component.  */
    OMX_U16 chm_quant_tab[64];
  } IDMJPGE_TIGEM_CustomQuantTables;


typedef struct IDMJPGE_TIGEM_DynamicParams {
    IIMGENC_DynamicParams  params;
    OMX_U32 captureHeight;       /* if set to 0 use image height
                                     else should set to actual Image height */
    OMX_U32 DRI_Interval ;
    JPEGENC_CUSTOM_HUFFMAN_TABLE *huffmanTable;
    IDMJPGE_TIGEM_CustomQuantTables *quantTable;
} IDMJPGE_TIGEM_DynamicParams;

/* PPLIB not needed if the the input to jpeg encoder is yuv. Uncomment the next line if PPLIB is needed */
/* #define __JPEG_OMX_PPLIB_ENABLED__ */

#ifdef __JPEG_OMX_PPLIB_ENABLED__
#define OMX_JPEGENC_NUM_DLLS (5)
#else
#define OMX_JPEGENC_NUM_DLLS (4)
#endif


#ifdef UNDER_CE
#define JPEG_ENC_NODE_DLL "/windows/jpegenc_sn.dll64P"
#define JPEG_COMMON_DLL "/windows/usn.dll64P"
#define USN_DLL "/windows/usn.dll64P"
#define CONVERSIONS_DLL "/windows/conversions.dll64P"
	#ifdef __JPEG_OMX_PPLIB_ENABLED__
		#define PPLIB_DLL "/windows/postprocessor_dualout.dll64P"
	#endif
#else
#define JPEG_ENC_NODE_DLL "jpegenc_sn.dll64P"
#define JPEG_COMMON_DLL "usn.dll64P"
#define USN_DLL "usn.dll64P"
#define CONVERSIONS_DLL "conversions.dll64P"
	#ifdef __JPEG_OMX_PPLIB_ENABLED__
		#define PPLIB_DLL "postprocessor_dualout.dll64P"
	#endif
#endif

#define JPGENC_SNTEST_STRMCNT          2
#define JPGENC_SNTEST_INSTRMID         0
#define JPGENC_SNTEST_OUTSTRMID        1
#define JPGENC_SNTEST_ARGLENGTH        20
#define JPGENC_SNTEST_INBUFCNT         4
#define JPGENC_SNTEST_OUTBUFCNT        4
#define JPGENC_SNTEST_MAX_HEIGHT       4096
#define JPGENC_SNTEST_MAX_WIDTH        4096
#define JPGENC_SNTEST_PROG_FLAG        1
#define M_COM   0xFE            /* COMment  */

#define JPEGE_DSPSTOP       0x01
#define JPEGE_BUFFERBACK    0x02
#define JPEGE_IDLEREADY     ( JPEGE_DSPSTOP | JPEGE_BUFFERBACK )

typedef enum Content_Type
{
    APP0_BUFFER = 0,
    APP1_BUFFER,
    APP13_BUFFER,
    COMMENT_BUFFER,
    APP0_NUMBUF,
    APP1_NUMBUF,
    APP13_NUMBUF,
    COMMENT_NUMBUF,
    APP0_THUMB_H,
    APP0_THUMB_W,
    APP1_THUMB_H,
    APP1_THUMB_W,
    APP13_THUMB_H,
    APP13_THUMB_W,
    APP0_THUMB_INDEX,
    APP1_THUMB_INDEX,
    APP13_THUMB_INDEX,
    DYNPARAMS_HUFFMANTABLE,
    DYNPARAMS_QUANTTABLE,
    APP5_BUFFER,
    APP5_NUMBUF,
    APP5_THUMB_H,
    APP5_THUMB_W,
    APP5_THUMB_INDEX    
} Content_Type;

/*This enum must not be changed.  */
typedef enum JPEG_PORT_TYPE_INDEX
    {
    JPEGENC_INP_PORT,
    JPEGENC_OUT_PORT
}JPEG_PORT_TYPE_INDEX;

typedef enum JPEGENC_BUFFER_OWNER {
    JPEGENC_BUFFER_CLIENT = 0x0,
    JPEGENC_BUFFER_COMPONENT_IN,
    JPEGENC_BUFFER_COMPONENT_OUT,
    JPEGENC_BUFFER_DSP,
    JPEGENC_BUFFER_TUNNEL_COMPONENT
} JPEGENC_BUFFER_OWNER;

typedef struct _JPEGENC_BUFFERFLAG_TRACK {
    OMX_U32 flag;
    OMX_U32 buffer_id;
    OMX_HANDLETYPE hMarkTargetComponent;
    OMX_PTR pMarkData;
} JPEGENC_BUFFERFLAG_TRACK;

typedef struct _JPEGENC_BUFFERMARK_TRACK {
    OMX_U32 buffer_id;
    OMX_HANDLETYPE hMarkTargetComponent;
    OMX_PTR pMarkData;
} JPEGENC_BUFFERMARK_TRACK;

typedef struct JPEGENC_BUFFER_PRIVATE {
    OMX_BUFFERHEADERTYPE* pBufferHdr;
    JPEGENC_BUFFER_OWNER eBufferOwner;
    OMX_BOOL bAllocByComponent;
    OMX_BOOL bReadFromPipe;
} JPEGENC_BUFFER_PRIVATE;

typedef struct JPEG_PORT_TYPE   {
    OMX_HANDLETYPE hTunnelComponent;
    OMX_U32 nTunnelPort;
    JPEGENC_BUFFER_PRIVATE* pBufferPrivate[NUM_OF_BUFFERSJPEG];
    JPEGENC_BUFFERFLAG_TRACK sBufferFlagTrack[NUM_OF_BUFFERSJPEG];
    JPEGENC_BUFFERMARK_TRACK sBufferMarkTrack[NUM_OF_BUFFERSJPEG];
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef;
    OMX_BUFFERSUPPLIERTYPE pBufSupplier;
    OMX_PARAM_BUFFERSUPPLIERTYPE* pParamBufSupplier;
    OMX_IMAGE_PARAM_PORTFORMATTYPE* pPortFormat;
    OMX_U8 nBuffCount;
}JPEG_PORT_TYPE;

typedef struct JPEGE_INPUT_PARAMS {
    OMX_U32 *pInParams;
    OMX_U32 size;
} JPEGE_INPUT_PARAMS;

typedef struct _JPEGENC_CUSTOM_PARAM_DEFINITION {
    OMX_U8 cCustomParamName[128];
    OMX_INDEXTYPE nCustomParamIndex;
} JPEGENC_CUSTOM_PARAM_DEFINITION;

typedef struct JPEGENC_COMPONENT_PRIVATE
{
    JPEG_PORT_TYPE* pCompPort[NUM_OF_PORTS];
    OMX_PORT_PARAM_TYPE* pPortParamType;
    OMX_PORT_PARAM_TYPE* pPortParamTypeAudio;
    OMX_PORT_PARAM_TYPE* pPortParamTypeVideo;
    OMX_PORT_PARAM_TYPE* pPortParamTypeOthers;
    OMX_PRIORITYMGMTTYPE* pPriorityMgmt;
    OMX_CALLBACKTYPE cbInfo;
    OMX_IMAGE_PARAM_QFACTORTYPE* pQualityfactor;
    OMX_CONFIG_RECTTYPE  *pCrop;
    /** This is component handle */
    OMX_COMPONENTTYPE* pHandle;
    /*Comonent Name& Version*/
    OMX_STRING cComponentName;
    OMX_VERSIONTYPE ComponentVersion;
    OMX_VERSIONTYPE SpecVersion;

    /** Current state of this component */
    OMX_STATETYPE   nCurState;
    OMX_STATETYPE   nToState;
    OMX_U8          ExeToIdleFlag;  /* StateCheck */

    OMX_U32 nInPortIn;
    OMX_U32 nInPortOut;
    OMX_U32 nOutPortIn;
    OMX_U32 nOutPortOut;
    OMX_BOOL bInportDisableIncomplete;
    OMX_BOOL bOutportDisableIncomplete;
    OMX_BOOL bSetLumaQuantizationTable;
    OMX_BOOL bSetChromaQuantizationTable;
    OMX_BOOL bSetHuffmanTable;
	OMX_BOOL bConvert420pTo422i;
	OMX_BOOL bPPLibEnable;
    OMX_IMAGE_PARAM_QUANTIZATIONTABLETYPE *pCustomLumaQuantTable;
    OMX_IMAGE_PARAM_QUANTIZATIONTABLETYPE *pCustomChromaQuantTable;
    JPEGENC_CUSTOM_HUFFMANTTABLETYPE *pHuffmanTable;


    /** The component thread handle */
    pthread_t ComponentThread;
    /** The pipes to maintain free buffers */
    int free_outBuf_Q[2];
    /** The pipes to maintain input buffers sent from app*/
    int filled_inpBuf_Q[2];
    /** The pipes for sending buffers to the thread */
    int nCmdPipe[2];
    int nCmdDataPipe[2];
    OMX_U32 nApp_nBuf;
    short int nNum_dspBuf;
    int nCommentFlag;
    OMX_U8 *pString_Comment;
    JPEG_APPTHUMB_MARKER sAPP0;
    JPEG_APPTHUMB_MARKER sAPP1;
    JPEG_APPTHUMB_MARKER sAPP5;
    JPEG_APP13_MARKER sAPP13;
    JPEGE_INPUT_PARAMS InParams;
#ifdef __JPEG_OMX_PPLIB_ENABLED__
    OMX_U32 *pOutParams;
#endif
#ifdef RESOURCE_MANAGER_ENABLED
    RMPROXY_CALLBACKTYPE rmproxyCallback;
#endif
    OMX_BOOL bPreempted;
    int nFlags;
    int nMarkPort;
    OMX_PTR pMarkData;
    OMX_HANDLETYPE hMarkTargetComponent;
    OMX_BOOL bDSPStopAck;
   OMX_BOOL bFlushComplete;
    OMX_BOOL bAckFromSetStatus;
    void* pLcmlHandle;   /* Review Utils.c */
    int isLCMLActive;
    LCML_DSP_INTERFACE* pLCML;
    void * pDllHandle;
    OMX_U8 nDRI_Interval;
#ifdef KHRONOS_1_1
    OMX_PARAM_COMPONENTROLETYPE componentRole;
#endif
    IDMJPGE_TIGEM_DynamicParams *pDynParams;

    pthread_mutex_t jpege_mutex;
    pthread_cond_t  stop_cond;
    pthread_cond_t  flush_cond;
    /* pthread_cond_t  control_cond; */
    pthread_mutex_t jpege_mutex_app;
    pthread_cond_t  populate_cond;
    pthread_cond_t  unpopulate_cond;


#ifdef __PERF_INSTRUMENTATION__
    PERF_OBJHANDLE pPERF, pPERFcomp;
#endif
    struct OMX_TI_Debug dbg;

    /* Reference count for pending state change requests */
    OMX_U32 nPendingStateChangeRequests;
    pthread_mutex_t mutexStateChangeRequest;
    pthread_cond_t StateChangeCondition;

} JPEGENC_COMPONENT_PRIVATE;


OMX_ERRORTYPE HandleJpegEncCommand (JPEGENC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U32 nParam1);
OMX_ERRORTYPE JpegEncDisablePort(JPEGENC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U32 nParam1);
OMX_ERRORTYPE JpegEncEnablePort(JPEGENC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U32 nParam1);
OMX_ERRORTYPE HandleJpegEncCommandFlush(JPEGENC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U32 nParam1);
OMX_ERRORTYPE JPEGEnc_Start_ComponentThread(OMX_HANDLETYPE pHandle);
OMX_ERRORTYPE HandleJpegEncDataBuf_FromApp(JPEGENC_COMPONENT_PRIVATE *pComponentPrivate );
OMX_ERRORTYPE HandleJpegEncDataBuf_FromDsp( JPEGENC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE* pBuffHead );
OMX_ERRORTYPE HandleJpegEncFreeDataBuf( JPEGENC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE* pBuffHead );
OMX_ERRORTYPE HandleJpegEncFreeOutputBufferFromApp( JPEGENC_COMPONENT_PRIVATE *pComponentPrivate );
OMX_ERRORTYPE AllocJpegEncResources( JPEGENC_COMPONENT_PRIVATE *pComponentPrivate );
OMX_ERRORTYPE JPEGEnc_Free_ComponentResources(JPEGENC_COMPONENT_PRIVATE *pComponentPrivate);
OMX_ERRORTYPE Fill_JpegEncLCMLInitParams(LCML_DSP *lcml_dsp, OMX_U16 arr[], OMX_HANDLETYPE pComponent);
OMX_ERRORTYPE GetJpegEncLCMLHandle(OMX_HANDLETYPE pComponent);
OMX_ERRORTYPE SetJpegEncInParams(JPEGENC_COMPONENT_PRIVATE *pComponentPrivate);
OMX_ERRORTYPE SendDynamicParam(JPEGENC_COMPONENT_PRIVATE *pComponentPrivate);
OMX_BOOL IsTIOMXComponent(OMX_HANDLETYPE hComp);

#ifdef __JPEG_OMX_PPLIB_ENABLED__
#define JPEGENC_PPLIB_CREATEPARAM_SIZE 28
#define JPEGENC_PPLIB_DYNPARM_SIZE 252
OMX_ERRORTYPE SendDynamicPPLibParam(JPEGENC_COMPONENT_PRIVATE *pComponentPrivate,OMX_U32 *ptInputParam);



typedef struct _PPLIB_UALGRunTimeParam_t
{
    OMX_U32 size;                            /**< Size of the structure in bytes. */
    OMX_U32 ulInWidth;                       /**< Input picture buffer width.  This value should be the same as the original decoded output width of the WMV9/VC1 stream. */
    OMX_U32 ulInHeight;                      /**< Input picture buffer height.  This value should be the same as the original decoded output height of the WMV9/VC1 stream. */
    OMX_U32 ulFrameEnabled[2];               /**< It is possible to run the VGPOP twice with two separate sets of configuration parameters using PPLIB.  This parameter specifies whether each set of configuration parameters is to be used when running PPLIB for this particular frame. */
    OMX_U32 ulEnableYUVOutput[2];            /**< Flag to enable YUV output */
    OMX_U32 ulEnableRGBOutput[2];            /**< Flag to enable RGB output. */
    OMX_U32 ulFrameInputStartYOffset[2];     /**< Offset from the start of the input buffer where the input Y data is located.  You can specify a different offset for each set of VGPOP parameters.  In most cases, this will be 0. */
    OMX_U32 ulFrameInputStartCOffset[2];     /**< Offset from the start of the input buffer where the input CrCb data is located.  You can specify a different offset for each set of VGPOP parameters.  In most cases, this will be the same as (input width * input height) + Y offset. */
    OMX_U32 ulFrameOutputStartYOffset[2];    /**< Offset from the start of the output buffer where the output Y data should be placed.  You can specify a different offset for each set of VGPOP parameters. */
    OMX_U32 ulFrameOutputStartCOffset[2];    /**< Offset from the start of the output buffer where the output CrCb data should be placed.  You can specify a different offset for each set of VGPOP parameters.  In most cases, this will be the same as (output width * output height) + Y offset. */
    OMX_U32 ulFrameOutputRGBOffset[2];       /**< Offset from the start of the output buffer where the output RGB data is located.  You can specify a different offset for each set of VGPOP parameters.  In most cases, this will be 0. */
    OMX_U32 ulFrameOutputHeight[2];          /**< Output picture buffer height for each VGPOP parameter set.*/
    OMX_U32 ulFrameOutputWidth[2];           /**< Output picture buffer width for each VGPOP parameter set. */
    OMX_U32 ulFrameContrast[2];              /**< Contrast Method for each VGPOP parameter set */
    OMX_U32 ulFrameInXStart[2];              /**< Horizontal cropping start position in the input buffer.  Set to 0 if no cropping is desired. */
    OMX_U32 ulFrameInYStart[2];              /**< Vertical cropping start position in the input buffer.  Set to 0 if no cropping is desired.*/
    OMX_U32 ulFrameInXSize[2];               /**< Horizontal cropping width.  Set to 0 if no cropping is desired */
    OMX_U32 ulFrameInYSize[2];               /**< Vertical cropping height.  Set to 0 if no cropping is desired.*/
    OMX_U32 ulFrameZoomFactor[2];            /**< Zooming ratio value, where ulZoomFactor = (Desired Zoom Ratio * 1024).  Set to 1024 if no zooming is desired.  Set above 1024 to enable zooming. */
    OMX_U32 ulFrameZoomLimit[2];             /**< Zooming ratio limit, where ulZoomLimit=(Desired Zoom Limit * 1024).*/
    OMX_U32 ulFrameZoomSpeed[2];             /**< Speed of ratio change.  Set to 0 to disable zoom variation.  The variation speed is proportional to the value while the direction (in/out) is given by the sign.*/
    OMX_U32 ulFrameEnableLightChroma[2];     /**< Light chrominance process.  */
    OMX_U32 ulFrameEnableAspectRatioLock[2]; /**< Locked H/V ratio */
    OMX_U32 ulFrameEnableMirroring[2];       /**< To mirror the picture: */
    OMX_U32 ulFrameRGBRotation[2];           /**< Rotation to apply to RGB Output. May be set to 0, 90, 180 or 270.*/
    OMX_U32 ulFrameYUVRotation[2];           /**< Rotation to apply to YUV Output. May be set to 0, 90, 180, or 270*/
    OMX_U32 ulFrameIORange[2];               /**< IO Video Range.   */
    OMX_U32 ulFrameEnableDithering[2];       /**< Dithering Enable */
    OMX_U32 ulFrameOutputPitch[2];           /**< Enable an output pitch */
    OMX_U32 ulAlphaRGB[2];                   /**< This is the default alpha values for ARGB32 or RGBA32. */
    OMX_U32 ulIsFrameGenerated[2];           /**< Flag to notify the user if a frame has been generated */
    OMX_U32 ulYUVFrameSize[2];               /**< YUV output size in bytes */
    OMX_U32 ulRGBFrameSize[2];               /**< RGB output size in bytes. */
} PPLIB_UALGRunTimeParam_t;



#endif

typedef OMX_ERRORTYPE (*fpo)(OMX_HANDLETYPE);

static const struct DSP_UUID JPEGESOCKET_TI_UUID = {
    0xCB70C0C1, 0x4C85, 0x11D6, 0xB1, 0x05, {
        0x00, 0xC0, 0x4F, 0x32, 0x90, 0x31
    }
};


static const struct DSP_UUID USN_UUID = {
    0x79A3C8B3, 0x95F2, 0x403F, 0x9A, 0x4B, {
        0xCF, 0x80, 0x57, 0x73, 0x05, 0x41
    }
};

static const struct DSP_UUID CONVERSIONS_UUID = {
	0x722DD0DA, 0xF532, 0x4238, 0xB8, 0x46, {
		0xAB, 0xFF, 0x5D, 0xA4, 0xBA, 0x02
	}
};

#ifdef __JPEG_OMX_PPLIB_ENABLED__
static const struct DSP_UUID PPLIB_UUID = {
	0xFC8CF948, 0xD3E9, 0x4B65, 0xBC, 0xA7, {
	0x08, 0x2E, 0xA0, 0xAD, 0x86, 0xF0
    }
};
#endif
void* OMX_JpegEnc_Thread (void* pThreadData);

typedef enum ThrCmdType
{
    SetState,
    Flush,
    StopPort,
    RestartPort,
    MarkBuf,
    Start,
    Stop,
    FillBuf,
    EmptyBuf
} ThrCmdType;

typedef enum OMX_JPEGE_INDEXTYPE  {

    OMX_IndexCustomCommentFlag = 0xFF000001,
    OMX_IndexCustomCommentString = 0xFF000002,
    OMX_IndexCustomInputFrameWidth,
    OMX_IndexCustomInputFrameHeight,
    OMX_IndexCustomAPP0,
    OMX_IndexCustomAPP1,
    OMX_IndexCustomAPP5,
    OMX_IndexCustomAPP13,
    OMX_IndexCustomQFactor,
    OMX_IndexCustomDRI,
    OMX_IndexCustomHuffmanTable,
    OMX_IndexCustomDebug,
	OMX_IndexCustomColorFormatConvertion_420pTo422i,
	OMX_IndexCustomPPLibEnable
}OMX_INDEXIMAGETYPE;

typedef struct IUALG_Buf {
    OMX_PTR            pBufAddr;
    unsigned long          ulBufSize;
    OMX_PTR            pParamAddr;
    unsigned long        ulParamSize;
    unsigned long          ulBufSizeUsed;
    //IUALG_BufState tBufState;
    OMX_BOOL           bBufActive;
    OMX_U32          unBufID;
    unsigned long          ulReserved;
} IUALG_Buf;

typedef enum {
    IUALG_CMD_STOP             = 0,
    IUALG_CMD_PAUSE            = 1,
    IUALG_CMD_GETSTATUS        = 2,
    IUALG_CMD_SETSTATUS        = 3,
    IUALG_CMD_USERSETCMDSTART  = 100,
    IUALG_CMD_USERGETCMDSTART  = 150,
    IUALG_CMD_FLUSH            = 0x100
}IUALG_Cmd;

OMX_ERRORTYPE AddStateTransition(JPEGENC_COMPONENT_PRIVATE* pComponentPrivate);
OMX_ERRORTYPE RemoveStateTransition(JPEGENC_COMPONENT_PRIVATE* pComponentPrivate, OMX_BOOL bEnableSignal);

#endif /*OMX_JPEGENC_UTILS__H*/
