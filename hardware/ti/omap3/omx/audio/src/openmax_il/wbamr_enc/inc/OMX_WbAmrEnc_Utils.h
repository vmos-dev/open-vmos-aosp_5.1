
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
*             Texas Instruments OMAP(TM) Platform Software
*  (c) Copyright Texas Instruments, Incorporated.  All Rights Reserved.
*
*  Use of this software is controlled by the terms and conditions found
*  in the license agreement under which this software has been supplied.
* ============================================================================ */
/**
* @file OMX_WbAmrEnc_Utils.h
*
* This is an header file for an WBAMR Encoder that is fully
* compliant with the OMX Audio specification 1.5.
* This the file that the application that uses OMX would include in its code.
*
* @path $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\wbamr_enc\inc
*
* @rev 1.0
*/
/* --------------------------------------------------------------------------- */
/* ----------------------------------------------------------------------------
*!
*! Revision History
*! ===================================
*! 21-sept-2006 bk: updated review findings for alpha release
*! 24-Aug-2006 bk: Khronos OpenMAX (TM) 1.0 Conformance tests some more
*! 18-July-2006 bk: Khronos OpenMAX (TM) 1.0 Conformance tests validated for few cases
*! 21-Jun-2006 bk: Khronos OpenMAX (TM) 1.0 migration done
*! 22-May-2006 bk: DASF recording quality improved
*! 19-Apr-2006 bk: DASF recording speed issue resloved
*! 23-Feb-2006 bk: DASF functionality added
*! 18-Jan-2006 bk: Repated recording issue fixed and LCML changes taken care
*! 14-Dec-2005 bk: Initial Version
*! 16-Nov-2005 bk: Initial Version
*! 23-Sept-2005 bk: Initial Version
*! 10-Sept-2005 bk: Initial Version
*! 10-Sept-2005 bk:
*! This is newest file
* =========================================================================== */
#ifndef OMX_WBAMRENC_UTILS__H
#define OMX_WBAMRENC_UTILS__H

#include "LCML_DspCodec.h"
#include <semaphore.h>

#ifdef RESOURCE_MANAGER_ENABLED
#include <ResourceManagerProxyAPI.h>
#endif

#ifdef __PERF_INSTRUMENTATION__
#include "perf.h"
#endif

#include <OMX_Component.h>
#include "OMX_TI_Common.h"
#include "OMX_TI_Debug.h"
#ifdef DSP_RENDERING_ON
#include <AudioManagerAPI.h>
#endif

#ifdef UNDER_CE
#define sleep Sleep
#endif

#ifndef ANDROID
#define ANDROID
#endif

#ifdef ANDROID
#undef LOG_TAG
#define LOG_TAG "OMX_WBAMRENC"

/* PV opencore capability custom parameter index */
#define PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX 0xFF7A347
#endif

/* ======================================================================= */
/**
 * @def    WBAMRENC_DEBUG   Turns debug messaging on and off
 */
/* ======================================================================= */
#undef WBAMRENC_DEBUG
/* ======================================================================= */
/**
 * @def    WBAMRENC_MEMCHECK   Turns memory messaging on and off
 */
/* ======================================================================= */
#undef WBAMRENC_MEMCHECK

/* ======================================================================= */
/**
 * @def    WBAMRENC_DEBUGMEM   Turns memory leaks messaging on and off.
 *         APP_DEBUGMEM must be defined in Test App in order to get
 *         this functionality On.
 */
/* ======================================================================= */
#undef WBAMRENC_DEBUGMEM
/*#define WBAMRENC_DEBUGMEM*/

/* ======================================================================= */
/**
 *  M A C R O S FOR MEMORY and CLOSING PIPES
 */
/* ======================================================================= */

#define OMX_WBCONF_INIT_STRUCT(_s_, _name_) \
    memset((_s_), 0x0, sizeof(_name_)); \
    (_s_)->nSize = sizeof(_name_);      \
    (_s_)->nVersion.s.nVersionMajor = 0x1;  \
    (_s_)->nVersion.s.nVersionMinor = 0x0;  \
    (_s_)->nVersion.s.nRevision = 0x0;      \
    (_s_)->nVersion.s.nStep = 0x0

#define OMX_WBCLOSE_PIPE(_pStruct_,err)\
    OMXDBG_PRINT(stderr, BUFFER, 2, OMX_DBG_BASEMASK, "Closing pipes = %d\n",_pStruct_);\
    err = close (_pStruct_);\
    if(0 != err && OMX_ErrorNone == eError){\
        eError = OMX_ErrorHardware;\
        OMXDBG_PRINT(stderr, ERROR, 4, OMX_DBG_BASEMASK, "Error while closing pipe\n");\
        goto EXIT;\
    }

#define WBAMRENC_OMX_ERROR_EXIT(_e_, _c_, _s_)\
    _e_ = _c_;\
    OMXDBG_PRINT(stderr, ERROR, 4, OMX_DBG_BASEMASK, "Error Name: %s : Error Num = %x", _s_, _e_);\
    goto EXIT;

/* ======================================================================= */
/**
 * @def    WBAMRENC_SAMPLING_FREQUENCY   Sampling frequency
 */
/* ======================================================================= */
#define WBAMRENC_SAMPLING_FREQUENCY 16000
/* ======================================================================= */
/**
 * @def    WBAMRENC_CPU_LOAD                    CPU Load in MHz
 */
/* ======================================================================= */
#define WBAMRENC_CPU_LOAD 20
/* ======================================================================= */
/**
 * @def    WBAMRENC_MAX_NUM_OF_BUFS   Maximum number of buffers
 */
/* ======================================================================= */
#define WBAMRENC_MAX_NUM_OF_BUFS 15
/* ======================================================================= */
/**
 * @def    WBAMRENC_NUM_OF_PORTS   Number of ports
 */
/* ======================================================================= */
#define WBAMRENC_NUM_OF_PORTS 2
/* ======================================================================= */
/**
 * @def    WBAMRENC_XXX_VER    Component version
 */
/* ======================================================================= */
#define WBAMRENC_MAJOR_VER 0xF1
#define WBAMRENC_MINOR_VER 0xF2
/* ======================================================================= */
/**
 * @def    WBAMRENC_NOT_USED    Defines a value for "don't care" parameters
 */
/* ======================================================================= */
#define WBAMRENC_NOT_USED 10
/* ======================================================================= */
/**
 * @def    WBAMRENC_NORMAL_BUFFER  Defines flag value with all flags off
 */
/* ======================================================================= */
#define WBAMRENC_NORMAL_BUFFER 0
/* ======================================================================= */
/**
 * @def    OMX_WBAMRENC_DEFAULT_SEGMENT    Default segment ID for the LCML
 */
/* ======================================================================= */
#define WBAMRENC_DEFAULT_SEGMENT (0)
/* ======================================================================= */
/**
 * @def    OMX_WBAMRENC_SN_TIMEOUT    Timeout value for the socket node
 */
/* ======================================================================= */
#define WBAMRENC_SN_TIMEOUT (-1)
/* ======================================================================= */
/**
 * @def    OMX_WBAMRENC_SN_PRIORITY   Priority for the socket node
 */
/* ======================================================================= */
#define WBAMRENC_SN_PRIORITY (10)
/* ======================================================================= */
/**
 * @def    OMX_WBAMRENC_NUM_DLLS   number of DLL's
 */
/* ======================================================================= */
#define WBAMRENC_NUM_DLLS (2)
/* ======================================================================= */
/**
 * @def    WBAMRENC_USN_DLL_NAME   USN DLL name
 */
/* ======================================================================= */
#ifdef UNDER_CE
#define WBAMRENC_USN_DLL_NAME "\\windows\\usn.dll64P"
#else
#define WBAMRENC_USN_DLL_NAME "usn.dll64P"
#endif

/* ======================================================================= */
/**
 * @def    WBAMRENC_DLL_NAME   WBAMR Encoder socket node dll name
 */
/* ======================================================================= */
#ifdef UNDER_CE
#define WBAMRENC_DLL_NAME "\\windows\\wbamrenc_sn.dll64P"
#else
#define WBAMRENC_DLL_NAME "wbamrenc_sn.dll64P"
#endif

/* ======================================================================= */
/** WBAMRENC_StreamType  Stream types
*
*  @param  WBAMRENC_DMM                 DMM
*
*  @param  WBAMRENC_INSTRM              Input stream
*
*  @param  WBAMRENC_OUTSTRM             Output stream
*/
/* ======================================================================= */
enum WBAMRENC_StreamType {
    WBAMRENC_DMM = 0,
    WBAMRENC_INSTRM,
    WBAMRENC_OUTSTRM
};
/* ======================================================================= */
/** WBAMRENC_EncodeType  coding types
*
*  @param  WBAMRENC_WBAMR           WBAMR mode
*
*  @param  WBAMRENC_EFR             EFR mode
*
*/
/* ======================================================================= */
enum WBAMRENC_EncodeType {
    WBAMRENC_WBAMR = 0,
    WBAMRENC_EFR
};
/* ======================================================================= */
/** WBAMRENC_MimeMode  format types
*
*  @param  WBAMRENC_MIMEMODE                MIME
*
*  @param  WBAMRENC_FORMATCONFORMANCE       WBAMR mode
*
*  @param  WBAMRENC_IF2                     IF2 mode
*
*/
/* ======================================================================= */
enum WBAMRENC_MimeMode {
    WBAMRENC_FORMATCONFORMANCE = 0,
    WBAMRENC_MIMEMODE,
    WBAMRENC_IF2
};

/* ======================================================================= */
/*
 * Different Frame sizes for different index in MIME Mode
 */
/* ======================================================================= */
#define WBAMRENC_FRAME_SIZE_0   0
#define WBAMRENC_FRAME_SIZE_1   1
#define WBAMRENC_FRAME_SIZE_6   6
#define WBAMRENC_FRAME_SIZE_18  18
#define WBAMRENC_FRAME_SIZE_23  23
#define WBAMRENC_FRAME_SIZE_24  24
#define WBAMRENC_FRAME_SIZE_33  33
#define WBAMRENC_FRAME_SIZE_37  37
#define WBAMRENC_FRAME_SIZE_41  41
#define WBAMRENC_FRAME_SIZE_47  47
#define WBAMRENC_FRAME_SIZE_51  51
#define WBAMRENC_FRAME_SIZE_59  59
#define WBAMRENC_FRAME_SIZE_61  61



/* ======================================================================= */
/**
 * @def WBAMRENC_TIMEOUT Default timeout used to come out of blocking calls
 */
/* ======================================================================= */
#define WBAMRENC_TIMEOUT 1000
/* ======================================================================= */
/*
 * @def WBAMRENC_OMX_MAX_TIMEOUTS   Max Time Outs
 * @def WBAMRENC_DONT_CARE          Dont Care Condition
 * @def WBAMRENC_NUM_CHANNELS       Number of Channels
 * @def WBAMRENC_APP_ID             App ID Value setting
 */
/* ======================================================================= */
#define WBAMRENC_OMX_MAX_TIMEOUTS 20
#define WBAMRENC_DONT_CARE 0
#define WBAMRENC_NUM_CHANNELS 1
/* ======================================================================= */
/**
 * @def    WBAMRENC_STREAM_COUNT    Number of streams
 *         WBAMRENC_INPUT_STREAM_ID Stream ID for Input Buffer
 */
/* ======================================================================= */
#define WBAMRENC_STREAM_COUNT 2
#define WBAMRENC_INPUT_STREAM_ID 0

/* ======================================================================= */
/**
 * @def _ERROR_PROPAGATION__     Allow Logic to Detec&Report Arm Errors
 */
/* ======================================================================= */
#define _ERROR_PROPAGATION__

typedef struct PV_OMXComponentCapabilityFlagsType {
    /* OMX COMPONENT CAPABILITY RELATED MEMBERS (for opencore compatability)*/
    OMX_BOOL iIsOMXComponentMultiThreaded;
    OMX_BOOL iOMXComponentSupportsExternalOutputBufferAlloc;
    OMX_BOOL iOMXComponentSupportsExternalInputBufferAlloc;
    OMX_BOOL iOMXComponentSupportsMovableInputBuffers;
    OMX_BOOL iOMXComponentSupportsPartialFrames;
    OMX_BOOL iOMXComponentNeedsNALStartCode;
    OMX_BOOL iOMXComponentCanHandleIncompleteFrames;
} PV_OMXComponentCapabilityFlagsType;

/** WBAMRENC_COMP_PORT_TYPE  Port types
 *
 *  @param  WBAMRENC_INPUT_PORT             Input port
 *
 *  @param  WBAMRENC_OUTPUT_PORT            Output port
 */
/*  ====================================================================== */
/*This enum must not be changed. */
typedef enum WBAMRENC_COMP_PORT_TYPE {
    WBAMRENC_INPUT_PORT = 0,
    WBAMRENC_OUTPUT_PORT
} WBAMRENC_COMP_PORT_TYPE;

/* ======================================================================= */
/** AUDIO_SN_WBAMRBANDMODETYPE
 *
 *  @SN_AUDIO_BRXXX This constant is used to determine the code to send
 *  to SN to set the BitRate.
 */
/*  ====================================================================== */
typedef enum AUDIO_SN_WBAMRBANDMODETYPE {
    SN_AUDIO_BR2385 = 8,
    SN_AUDIO_BR2305,
    SN_AUDIO_BR1985,
    SN_AUDIO_BR1825,
    SN_AUDIO_BR1585,
    SN_AUDIO_BR1425,
    SN_AUDIO_BR1265,
    SN_AUDIO_BR885,
    SN_AUDIO_BR660,
    SN_AUDIO_WBAMRBandModeMax = 0x7FFFFFFF
} AUDIO_SN_WBAMRBANDMODETYPE;

/* ======================================================================= */
/** WBAMRENC_BUFFER_Dir  Buffer Direction
*
*  @param  WBAMRENC_DIRECTION_INPUT     Input direction
*
*  @param  WBAMRENC_DIRECTION_OUTPUT    Output direction
*
*/
/* ======================================================================= */
typedef enum {
    WBAMRENC_DIRECTION_INPUT,
    WBAMRENC_DIRECTION_OUTPUT
} WBAMRENC_BUFFER_Dir;

/* ======================================================================= */
/** WBAMRENC_BUFFS  Buffer details
*
*  @param  BufHeader Buffer header
*
*  @param  Buffer   Buffer
*
*/
/* ======================================================================= */
typedef struct WBAMRENC_BUFFS {
    char BufHeader;
    char Buffer;
} WBAMRENC_BUFFS;

/* ======================================================================= */
/** WBAMRENC_BUFFERHEADERTYPE_INFO
*
*  @param  pBufHeader
*
*  @param  bBufOwner
*
*/
/* ======================================================================= */
typedef struct WBAMRENC_BUFFERHEADERTYPE_INFO {
    OMX_BUFFERHEADERTYPE* pBufHeader[WBAMRENC_MAX_NUM_OF_BUFS];
    WBAMRENC_BUFFS bBufOwner[WBAMRENC_MAX_NUM_OF_BUFS];
} WBAMRENC_BUFFERHEADERTYPE_INFO;


typedef OMX_ERRORTYPE (*WBAMRENC_fpo)(OMX_HANDLETYPE);

/* =================================================================================== */
/**
* Socket node Audio Codec Configuration parameters.
*/
/* =================================================================================== */
typedef struct WBAMRENC_AudioCodecParams {
    unsigned long  iSamplingRate;
    unsigned long  iStrmId;
    unsigned short iAudioFormat;
} WBAMRENC_AudioCodecParams;

/* =================================================================================== */
/**
* WBAMRENC_TALGCtrl                 Socket Node Alg Control parameters.
* WBAMRENC_TALGCtrlDTX              Socket Node Alg Control parameters (DTX).
* WBAMRENC_UAlgInBufParamStruct     Input Buffer Param Structure
* WBAMRENC_UAlgOutBufParamStruct    Output Buffer Param Structure
*/
/* =================================================================================== */
/* Algorithm specific command parameters */
typedef struct {
    int iSize;
    unsigned int iBitrate;
} WBAMRENC_TALGCtrl;

typedef struct {
    int iSize;
    unsigned int iVADFlag;

} WBAMRENC_TALGCtrlDTX;
/* =================================================================================== */
/**
* WBAMRENC_UAlgInBufParamStruct     Input Buffer Param Structure
* usLastFrame                       To Send Last Buufer Flag
*/
/* =================================================================================== */
typedef struct {
    unsigned long int usLastFrame;
} WBAMRENC_FrameStruct;

typedef struct {
    unsigned long int usNbFrames;
    WBAMRENC_FrameStruct *pParamElem;
} WBAMRENC_ParamStruct;

/* =================================================================================== */
/**
* WBAMRENC_UAlgOutBufParamStruct    Output Buffer Param Structure
* ulFrameCount No.of Frames Encoded
*/
/* =================================================================================== */
typedef struct {
    unsigned long ulFrameCount;
} WBAMRENC_UAlgOutBufParamStruct;

/* =================================================================================== */
/**
* WBAMRENC_LCML_BUFHEADERTYPE Buffer Header Type
*/
/* =================================================================================== */
typedef struct WBAMRENC_LCML_BUFHEADERTYPE {
    WBAMRENC_BUFFER_Dir eDir;
    WBAMRENC_FrameStruct *pFrameParam;
    WBAMRENC_ParamStruct *pBufferParam;
    DMM_BUFFER_OBJ* pDmmBuf;
    OMX_BUFFERHEADERTYPE* buffer;
} WBAMRENC_LCML_BUFHEADERTYPE;

typedef struct _WBAMRENC_BUFFERLIST WBAMRENC_BUFFERLIST;

/* =================================================================================== */
/**
* _WBAMRENC_BUFFERLIST Structure for buffer list
*/
/* ================================================================================== */
struct _WBAMRENC_BUFFERLIST {
    OMX_BUFFERHEADERTYPE sBufHdr;
    OMX_BUFFERHEADERTYPE *pBufHdr[WBAMRENC_MAX_NUM_OF_BUFS];
    OMX_U32 bufferOwner[WBAMRENC_MAX_NUM_OF_BUFS];
    OMX_U32 bBufferPending[WBAMRENC_MAX_NUM_OF_BUFS];
    OMX_U32 numBuffers;
    WBAMRENC_BUFFERLIST *pNextBuf;
    WBAMRENC_BUFFERLIST *pPrevBuf;
};

/* =================================================================================== */
/**
* WBAMRENC_PORT_TYPE Structure for PortFormat details
*/
/* =================================================================================== */
typedef struct WBAMRENC_PORT_TYPE {
    OMX_HANDLETYPE hTunnelComponent;
    OMX_U32 nTunnelPort;
    OMX_BUFFERSUPPLIERTYPE eSupplierSetting;
    OMX_U8 nBufferCnt;
    OMX_AUDIO_PARAM_PORTFORMATTYPE* pPortFormat;
} WBAMRENC_PORT_TYPE;

#ifdef UNDER_CE
#ifndef _OMX_EVENT_
#define _OMX_EVENT_
typedef struct OMX_Event {
    HANDLE event;
} OMX_Event;
#endif
int OMX_CreateEvent(OMX_Event *event);
int OMX_SignalEvent(OMX_Event *event);
int OMX_WaitForEvent(OMX_Event *event);
int OMX_DestroyEvent(OMX_Event *event);
#endif

typedef struct WBAMRENC_BUFDATA {
    OMX_U8 nFrames;
} WBAMRENC_BUFDATA;
/* =================================================================================== */
/**
* WBAMRENC_COMPONENT_PRIVATE Component private data Structure
*/
/* =================================================================================== */
typedef struct WBAMRENC_COMPONENT_PRIVATE {
    /** Array of pointers to BUFFERHEADERTYPE structues
        This pBufHeader[INPUT_PORT] will point to all the
        BUFFERHEADERTYPE structures related to input port,
        not just one structure. Same is the case for output
        port also. */
    OMX_BUFFERHEADERTYPE* pBufHeader[WBAMRENC_NUM_OF_PORTS];
    /** Number of input buffers at runtime */
    OMX_U32 nRuntimeInputBuffers;

    /** Number of output buffers at runtime */
    OMX_U32 nRuntimeOutputBuffers;

    OMX_CALLBACKTYPE cbInfo;
    OMX_PORT_PARAM_TYPE* sPortParam;
    OMX_PRIORITYMGMTTYPE* sPriorityMgmt;

#ifdef RESOURCE_MANAGER_ENABLED
    RMPROXY_CALLBACKTYPE rmproxyCallback;
#endif

    OMX_BOOL bPreempted;

    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef[WBAMRENC_NUM_OF_PORTS];
    OMX_PORT_PARAM_TYPE* pPortParamType;
    OMX_AUDIO_PARAM_AMRTYPE* amrParams;
    OMX_AUDIO_PARAM_PCMMODETYPE* pcmParams;

    WBAMRENC_BUFFERHEADERTYPE_INFO BufInfo[WBAMRENC_NUM_OF_PORTS];
    WBAMRENC_PORT_TYPE *pCompPort[WBAMRENC_NUM_OF_PORTS];
    WBAMRENC_LCML_BUFHEADERTYPE *pLcmlBufHeader[WBAMRENC_NUM_OF_PORTS];
    /** This is component handle */
    OMX_COMPONENTTYPE* pHandle;
    /** Current state of this component */
    OMX_STATETYPE curState;
    /** The component thread handle */
    pthread_t ComponentThread;
    /** The pipes for sending buffers to the thread */
    int dataPipe[2];
    /** The pipes for sending command to the thread */
    int cmdPipe[2];
    /** The pipes for sending cmd data to the thread */
    int cmdDataPipe[2];

    OMX_U32 efrMode;

    OMX_U32 amrMode;

    OMX_U32 dasfMode;

    OMX_U32 frameMode;

    OMX_U32 acdnMode;

    OMX_U32 nMultiFrameMode;

    OMX_S32 fdwrite;

    OMX_S32 fdread;

    /** Set to indicate component is stopping */
    OMX_U32 bIsThreadstop;

    /** Count of number of buffers outstanding with bridge */
    OMX_U32 lcml_nIpBuf;

    /** Count of number of buffers outstanding with bridge */
    OMX_U32 lcml_nOpBuf;

    OMX_U32 app_nBuf;

    OMX_U32 lcml_nCntIp;

    OMX_U32 lcml_nCntOpReceived;

    OMX_U32 bIsBufferOwned[WBAMRENC_NUM_OF_PORTS];

    OMX_U32 streamID;

    OMX_U32 bCompThreadStarted;

    OMX_U32 nVersion;

    OMX_U32 amrMimeBytes[16];

    OMX_U32 amrIf2Bytes[16];

    OMX_U32 iHoldLen;

    OMX_U32 nHoldLength;

    OMX_U32 nFillThisBufferCount;

    OMX_U32 nFillBufferDoneCount;

    OMX_U32 nEmptyThisBufferCount;

    OMX_U32 nEmptyBufferDoneCount;

    OMX_U32 bInitParamsInitialized;

    OMX_U32 nNumInputBufPending;

    OMX_U32 nNumOutputBufPending;

    OMX_U32 bDisableCommandPending;

    OMX_U32 bDisableCommandParam;

    OMX_U32 bEnableCommandPending;

    OMX_U32 bEnableCommandParam;

    OMX_HANDLETYPE pLcmlHandle;

    OMX_PTR pMarkData;

    OMX_MARKTYPE *pMarkBuf;

    OMX_HANDLETYPE hMarkTargetComponent;

    WBAMRENC_BUFFERLIST *pInputBufferList;

    WBAMRENC_BUFFERLIST *pOutputBufferList;

    LCML_STRMATTR *strmAttr;

    WBAMRENC_TALGCtrl *pAlgParam;

    WBAMRENC_TALGCtrlDTX *pAlgParamDTX;

    WBAMRENC_AudioCodecParams *pParams;

    OMX_STRING cComponentName;

    OMX_VERSIONTYPE ComponentVersion;

    OMX_BUFFERHEADERTYPE *pInputBufHdrPending[WBAMRENC_MAX_NUM_OF_BUFS];

    OMX_BUFFERHEADERTYPE *pOutputBufHdrPending[WBAMRENC_MAX_NUM_OF_BUFS];

    OMX_BUFFERHEADERTYPE *iMMFDataLastBuffer;

    OMX_U8* pHoldBuffer, *pHoldBuffer2;

    OMX_U8* iHoldBuffer;

    /** Flag to set when socket node stop callback should not transition
        component to OMX_StateIdle */
    OMX_U32 bNoIdleOnStop;

    /** Flag set when socket node is stopped */
    OMX_U32 bDspStoppedWhileExecuting;

    /** Number of outstanding FillBufferDone() calls */
    OMX_S32 nOutStandingFillDones;

    OMX_S32 nOutStandingEmptyDones;

#ifndef UNDER_CE
    pthread_mutex_t AlloBuf_mutex;
    pthread_cond_t AlloBuf_threshold;
    OMX_U8 AlloBuf_waitingsignal;

    pthread_mutex_t InLoaded_mutex;
    pthread_cond_t InLoaded_threshold;
    OMX_U8 InLoaded_readytoidle;

    pthread_mutex_t InIdle_mutex;
    pthread_cond_t InIdle_threshold;
    OMX_U8 InIdle_goingtoloaded;

    pthread_mutex_t ToLoaded_mutex;
    /*
          sem_t allobuf;
          sem_t inloaded;
          sem_t inidle;
    */
#else
    OMX_Event AlloBuf_event;
    OMX_U8 AlloBuf_waitingsignal;

    OMX_Event InLoaded_event;
    OMX_U8 InLoaded_readytoidle;

    OMX_Event InIdle_event;
    OMX_U8 InIdle_goingtoloaded;
#endif

    OMX_U8 nNumOfFramesSent;

    OMX_U8 InBuf_Eos_alreadysent;

#ifdef __PERF_INSTRUMENTATION__
    PERF_OBJHANDLE pPERF, pPERFcomp;
    OMX_U32 nLcml_nCntIp;
    OMX_U32 nLcml_nCntOpReceived;
#endif

    OMX_BUFFERHEADERTYPE *LastOutbuf;
    OMX_BOOL bIsInvalidState;

    OMX_STRING* sDeviceString;
    void* ptrLibLCML;

    /** Circular array to keep buffer timestamps */
    OMX_S64 arrBufIndex[WBAMRENC_MAX_NUM_OF_BUFS];
    /** Circular array to keep buffer nTickCounts */
    OMX_S64 arrTickCount[WBAMRENC_MAX_NUM_OF_BUFS];
    /** Index to arrBufIndex[], used for input buffer timestamps */
    OMX_U8 IpBufindex;
    /** Index to arrBufIndex[], used for output buffer timestamps */
    OMX_U8 OpBufindex;

    OMX_S8 ProcessingInputBuf;
    OMX_S8 ProcessingOutputBuf;

    OMX_BOOL bLoadedCommandPending;

    OMX_PARAM_COMPONENTROLETYPE componentRole;

    /* Pointer to OpenCore capabilities structure */
    PV_OMXComponentCapabilityFlagsType iPVCapabilityFlags;

    struct OMX_TI_Debug dbg;

    /* Reference count for pending state change requests */
    OMX_U32 nPendingStateChangeRequests;
    pthread_mutex_t mutexStateChangeRequest;
    pthread_cond_t StateChangeCondition;
    
} WBAMRENC_COMPONENT_PRIVATE;


/* =================================================================================== */
/**
*  OMX_ComponentInit()  Initializes component
*
*
*  @param hComp         OMX Handle
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*
*/
/* =================================================================================== */
OMX_ERRORTYPE OMX_ComponentInit (OMX_HANDLETYPE hComp);
/* =================================================================================== */
/**
*  WBAMRENC_StartComponentThread()  Starts component thread
*
*
*  @param hComp         OMX Handle
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*
*/
/* =================================================================================== */
OMX_ERRORTYPE WBAMRENC_StartComponentThread(OMX_HANDLETYPE pHandle);
/* =================================================================================== */
/**
*  WBAMRENC_StopComponentThread()  Stops component thread
*
*
*  @param hComp         OMX Handle
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*
*/
/* =================================================================================== */
OMX_ERRORTYPE WBAMRENC_StopComponentThread(OMX_HANDLETYPE pHandle);
/* =================================================================================== */
/**
*  WBAMRENC_FreeCompResources()  Frees allocated memory
*
*
*  @param hComp         OMX Handle
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*
*/
/* =================================================================================== */
OMX_ERRORTYPE WBAMRENC_FreeCompResources(OMX_HANDLETYPE pComponent);
/* =================================================================================== */
/**
*  WBAMRENC_GetCorrespondingLCMLHeader()  Returns LCML header
* that corresponds to the given buffer
*
*  @param pComponentPrivate Component private data
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*/
/* =================================================================================== */
OMX_ERRORTYPE WBAMRENC_GetCorrespondingLCMLHeader(WBAMRENC_COMPONENT_PRIVATE *pComponentPrivate,
        OMX_U8 *pBuffer,
        OMX_DIRTYPE eDir,
        WBAMRENC_LCML_BUFHEADERTYPE **ppLcmlHdr);
/* =================================================================================== */
/**
*  WBAMRENC_LCMLCallback() Callback from LCML
*
*  @param event     Codec Event
*
*  @param args      Arguments from LCML
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*/
/* =================================================================================== */
OMX_ERRORTYPE WBAMRENC_LCMLCallback(TUsnCodecEvent event,
                                    void * args [10]);
/* =================================================================================== */
/**
*  WBAMRENC_FillLCMLInitParams() Fills the parameters needed
* to initialize the LCML
*
*  @param pHandle OMX Handle
*
*  @param plcml_Init LCML initialization parameters
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*
*/
/* =================================================================================== */
OMX_ERRORTYPE WBAMRENC_FillLCMLInitParams(OMX_HANDLETYPE pHandle,
        LCML_DSP *plcml_Init,
        OMX_U16 arr[]);
/* =================================================================================== */
/**
*  WBAMRENC_GetBufferDirection() Returns direction of pBufHeader
*
*  @param pBufHeader        Buffer header
*
*  @param eDir              Buffer direction
*
*  @param pComponentPrivate Component private data
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*/
/* =================================================================================== */
OMX_ERRORTYPE WBAMRENC_GetBufferDirection(OMX_BUFFERHEADERTYPE *pBufHeader,
        OMX_DIRTYPE *eDir);
/* ===========================================================  */
/**
*  WBAMRENC_HandleCommand()  Handles commands sent via SendCommand()
*
*  @param pComponentPrivate Component private data
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*/
/* =================================================================================== */
OMX_U32 WBAMRENC_HandleCommand(WBAMRENC_COMPONENT_PRIVATE *pComponentPrivate,
                               OMX_COMMANDTYPE cmd,
                               OMX_U32 cmdData);
/* =================================================================================== */
/**
*  WBAMRENC_HandleDataBufFromApp()  Handles data buffers received
* from the IL Client
*
*  @param pComponentPrivate Component private data
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*/
/* =================================================================================== */
OMX_ERRORTYPE WBAMRENC_HandleDataBufFromApp(OMX_BUFFERHEADERTYPE *pBufHeader,
        WBAMRENC_COMPONENT_PRIVATE *pComponentPrivate);
/* =================================================================================== */
/**
*  WBAMRENC_GetLCMLHandle()  Get the handle to the LCML
*
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*/
/* =================================================================================== */
OMX_HANDLETYPE WBAMRENC_GetLCMLHandle(WBAMRENC_COMPONENT_PRIVATE *pComponentPrivate);
/* =================================================================================== */
/**
*  WBAMRENC_FreeLCMLHandle()  Frees the handle to the LCML
*
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*/
/* =================================================================================== */
OMX_ERRORTYPE WBAMRENC_FreeLCMLHandle(WBAMRENC_COMPONENT_PRIVATE *pComponentPrivate);
/* =================================================================================== */
/**
*  WBAMRENC_CleanupInitParams()  Starts component thread
*
*  @param pComponent        OMX Handle
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*/
/* =================================================================================== */
OMX_ERRORTYPE WBAMRENC_CleanupInitParams(OMX_HANDLETYPE pHandle);
/* =================================================================================== */
/**
*  WBAMRENC_SetPending()  Called when the component queues a buffer
* to the LCML
*
*  @param pComponentPrivate     Component private data
*
*  @param pBufHdr               Buffer header
*
*  @param eDir                  Direction of the buffer
*
*  @return None
*/
/* =================================================================================== */
void WBAMRENC_SetPending(WBAMRENC_COMPONENT_PRIVATE *pComponentPrivate,
                         OMX_BUFFERHEADERTYPE *pBufHdr,
                         OMX_DIRTYPE eDir,
                         OMX_U32 lineNumber);
/* =================================================================================== */
/**
*  WBAMRENC_ClearPending()  Called when a buffer is returned
* from the LCML
*
*  @param pComponentPrivate     Component private data
*
*  @param pBufHdr               Buffer header
*
*  @param eDir                  Direction of the buffer
*
*  @return None
*/
/* =================================================================================== */
void WBAMRENC_ClearPending(WBAMRENC_COMPONENT_PRIVATE *pComponentPrivate,
                           OMX_BUFFERHEADERTYPE *pBufHdr,
                           OMX_DIRTYPE eDir,
                           OMX_U32 lineNumber);
/* =================================================================================== */
/**
*  WBAMRENC_IsPending()
*
*
*  @param pComponentPrivate     Component private data
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*/
/* =================================================================================== */
OMX_U32 WBAMRENC_IsPending(WBAMRENC_COMPONENT_PRIVATE *pComponentPrivate,
                           OMX_BUFFERHEADERTYPE *pBufHdr,
                           OMX_DIRTYPE eDir);
/* =================================================================================== */
/**
*  WBAMRENC_FillLCMLInitParamsEx()  Fills the parameters needed
* to initialize the LCML without recreating the socket node
*
*  @param pComponent            OMX Handle
*
*  @return None
*/
/* =================================================================================== */
OMX_ERRORTYPE WBAMRENC_FillLCMLInitParamsEx(OMX_HANDLETYPE pComponent);
/* =================================================================================== */
/**
*  WBAMRENC_IsValid() Returns whether a buffer is valid
*
*
*  @param pComponentPrivate     Component private data
*
*  @param pBuffer               Data buffer
*
*  @param eDir                  Buffer direction
*
*  @return OMX_True = Valid
*          OMX_False= Invalid
*/
/* =================================================================================== */
OMX_U32 WBAMRENC_IsValid(WBAMRENC_COMPONENT_PRIVATE *pComponentPrivate,
                         OMX_U8 *pBuffer,
                         OMX_DIRTYPE eDir);

OMX_ERRORTYPE OMX_DmmMap(DSP_HPROCESSOR ProcHandle, int size, void* pArmPtr, DMM_BUFFER_OBJ* pDmmBuf, struct OMX_TI_Debug dbg);
OMX_ERRORTYPE OMX_DmmUnMap(DSP_HPROCESSOR ProcHandle, void* pMapPtr, void* pResPtr, struct OMX_TI_Debug dbg);

#ifdef RESOURCE_MANAGER_ENABLED
void WBAMRENC_ResourceManagerCallback(RMPROXY_COMMANDDATATYPE cbData);
#endif

void WBAMRENC_HandleUSNError (WBAMRENC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U32 arg);

/*===============================================================*/

typedef enum {
    IUALG_CMD_STOP             = 0,
    IUALG_CMD_PAUSE            = 1,
    IUALG_CMD_GETSTATUS        = 2,
    IUALG_CMD_SETSTATUS        = 3,
    IUALG_CMD_USERSETCMDSTART  = 100,
    IUALG_CMD_USERGETCMDSTART  = 150,
    IUALG_CMD_FLUSH            = 0x100
} IUALG_Cmd;

typedef enum {
    ALGCMD_BITRATE = IUALG_CMD_USERSETCMDSTART,
    ALGCMD_DTX

} eSPEECHENCODE_AlgCmd;

OMX_ERRORTYPE AddStateTransition(WBAMRENC_COMPONENT_PRIVATE* pComponentPrivate);
OMX_ERRORTYPE RemoveStateTransition(WBAMRENC_COMPONENT_PRIVATE* pComponentPrivate, OMX_BOOL bEnableSignal);


#endif  /* OMX_WBAMRENC_UTILS__H */
