
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
* @file OMX_AmrEnc_Utils.h
*
* This is an header file for an NBAMR Encoder that is fully
* compliant with the OMX Audio specification 1.5.
* This the file that the application that uses OMX would include in its code.
*
* @path $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\nbamr_enc\inc
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
#ifndef OMX_AMRENC_UTILS__H
#define OMX_AMRENC_UTILS__H

#include <pthread.h>
#include "LCML_DspCodec.h"
#include <OMX_Component.h>
#include "OMX_TI_Common.h"
#include "OMX_TI_Debug.h"
#include <TIDspOmx.h>

#ifdef RESOURCE_MANAGER_ENABLED
#include <ResourceManagerProxyAPI.h>
#endif

#ifdef __PERF_INSTRUMENTATION__
    #include "perf.h"
#endif

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
    #define LOG_TAG "OMX_NBAMRENC"
//    #define LOG_TAG "nbamr_enc"
    /* PV opencore capability custom parameter index */
    #define PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX 0xFF7A347
#endif

/* ======================================================================= */
/**
 * @def    AMRENC_DEBUG   Turns debug messaging on and off
 */
/* ======================================================================= */
#define AMRENC_DEBUG
/* ======================================================================= */
/**
 * @def    AMRENC_MEMCHECK   Turns memory messaging on and off
 */
/* ======================================================================= */
#undef AMRENC_MEMCHECK

/* ======================================================================= */
/**
 * @def    NBAMRENC_DEBUGMEM   Turns memory leaks messaging on and off.
 *         APP_DEBUGMEM must be defined in Test App in order to get
 *         this functionality On.
 */
/* ======================================================================= */
#undef NBAMRENC_DEBUGMEM

/* ======================================================================= */
/**
 * @def    AMRENC_EPRINT   Error print macro
 */
/* ======================================================================= */
#ifndef UNDER_CE
    #ifdef ANDROID
        #define AMRENC_EPRINT ALOGE
    #else   
        #define AMRENC_EPRINT(...)    fprintf(stderr,__VA_ARGS__)
    #endif
#else
        #define AMRENC_EPRINT
#endif



/* ======================================================================= */
/**
 * @def    AMRENC_DEBUG   Debug print macro
 */
/* ======================================================================= */
#ifndef UNDER_CE

#ifdef  AMRENC_DEBUG
        #define AMRENC_DPRINT(...)    fprintf(stderr,__VA_ARGS__)

    #ifdef ANDROID
        #undef AMRENC_DPRINT
        #define AMRENC_DPRINT ALOGW
    #endif

#else
        #define AMRENC_DPRINT(...)
#endif


/* ======================================================================= */
/**
 * @def    AMRENC_MEMCHECK   Memory print macro
 */
/* ======================================================================= */
#ifdef  AMRENC_MEMCHECK
    #define AMRENC_MEMPRINT(...)    fprintf(stderr,__VA_ARGS__)

    #ifdef ANDROID
        #undef AMRENC_MEMPRINT
        #define AMRENC_MEMPRINT ALOGW
    #endif

#else
        #define AMRENC_MEMPRINT(...)    printf
#endif

#else   /*UNDER_CE*/
/* ======================================================================= */
/**
 * @def    AMRENC_DEBUG   Debug print macro
 */
/* ======================================================================= */
#ifdef  AMRENC_DEBUG
        #define AMRENC_DPRINT(STR, ARG...) printf()
#else

#endif
/* ======================================================================= */
/**
 * @def    AMRENC_MEMCHECK   Memory print macro
 */
/* ======================================================================= */
#ifdef  AMRENC_MEMCHECK
        #define AMRENC_MEMPRINT(STR, ARG...) printf()
#else

#endif

#ifdef UNDER_CE

#ifdef DEBUG
    #ifdef ANDROID
        #define AMRENC_DPRINT     ALOGW
        #define AMRENC_MEMPRINT   ALOGW
    #else
        #define AMRENC_DPRINT     printf
        #define AMRENC_MEMPRINT   printf
    #endif
#else
        #define AMRENC_DPRINT
        #define AMRENC_MEMPRINT
#endif

#endif  /*UNDER_CE*/

#endif

/* ======================================================================= */
/**
 *  M A C R O S FOR MALLOC and MEMORY FREE and CLOSING PIPES
 */
/* ======================================================================= */

#define OMX_NBCONF_INIT_STRUCT(_s_, _name_) \
    memset((_s_), 0x0, sizeof(_name_)); \
    (_s_)->nSize = sizeof(_name_);      \
    (_s_)->nVersion.s.nVersionMajor = 0x1;  \
    (_s_)->nVersion.s.nVersionMinor = 0x0;  \
    (_s_)->nVersion.s.nRevision = 0x0;      \
    (_s_)->nVersion.s.nStep = 0x0

#define OMX_NBCLOSE_PIPE(_pStruct_,err)\
    OMXDBG_PRINT(stderr, COMM, 2, OMX_DBG_BASEMASK, "%d :: CLOSING PIPE \n",__LINE__); \
    err = close (_pStruct_);\
    if(0 != err && OMX_ErrorNone == eError){\
        eError = OMX_ErrorHardware;\
        OMXDBG_PRINT(stderr, ERROR, 4, 0, "%d :: Error while closing pipe\n",__LINE__);\
        goto EXIT;\
    }

#define NBAMRENC_OMX_ERROR_EXIT(_e_, _c_, _s_)\
    _e_ = _c_;\
    OMXDBG_PRINT(stderr, ERROR, 4, 0, "\n**************** OMX ERROR ************************\n");\
    OMXDBG_PRINT(stderr, ERROR, 4, 0, "%d : Error Name: %s : Error Num = %x",__LINE__, _s_, _e_);\
    OMXDBG_PRINT(stderr, ERROR, 4, 0, "\n**************** OMX ERROR ************************\n");\
    goto EXIT;

/* ======================================================================= */
/**
 * @def NBAMRENC_NUM_INPUT_BUFFERS   Default number of input buffers
 */
/* ======================================================================= */
#define NBAMRENC_NUM_INPUT_BUFFERS 1
/* ======================================================================= */
/**
 * @def NBAMRENC_NUM_INPUT_BUFFERS_DASF  Default No.of input buffers DASF
 */
/* ======================================================================= */
#define NBAMRENC_NUM_INPUT_BUFFERS_DASF 2
/* ======================================================================= */
/**
 * @def NBAMRENC_NUM_OUTPUT_BUFFERS   Default number of output buffers
 */
/* ======================================================================= */
#define NBAMRENC_NUM_OUTPUT_BUFFERS 1
/* ======================================================================= */
/**
 * @def NBAMRENC_INPUT_BUFFER_SIZE       Default input buffer size
 *      NBAMRENC_INPUT_BUFFER_SIZE_DASF  Default input buffer size DASF
 *      NBAMRENC_INPUT_FRAME_SIZE        Default input Frame Size
 */
/* ======================================================================= */
#define NBAMRENC_INPUT_BUFFER_SIZE 320
#define NBAMRENC_INPUT_BUFFER_SIZE_DASF 320
#define NBAMRENC_INPUT_FRAME_SIZE 320
/* ======================================================================= */
/**
 * @def NBAMRENC_OUTPUT_BUFFER_SIZE   Default output buffer size
 *      NBAMRENC_OUTPUT_FRAME_SIZE    Default output frame size
 */
/* ======================================================================= */
#define NBAMRENC_OUTPUT_BUFFER_SIZE 118
#define NBAMRENC_OUTPUT_FRAME_SIZE 118
/* ======================================================================= */
/**
 * @def NBAMRENC_OUTPUT_BUFFER_SIZE_MIME  Default input buffer size MIME
 */
/* ======================================================================= */
#define NBAMRENC_OUTPUT_BUFFER_SIZE_MIME 34

/* ======================================================================= */
/**
 * @def NBAMRENC_OUTPUT_BUFFER_SIZE_MIME  Default input buffer size IF2
 */
/* ======================================================================= */
#define NBAMRENC_OUTPUT_BUFFER_SIZE_IF2 32

/* ======================================================================= */
/**
 * @def NBAMRENC_OUTPUT_BUFFER_SIZE_EFR  Default input buffer size EFR
 */
/* ======================================================================= */
#define NBAMRENC_OUTPUT_BUFFER_SIZE_EFR 120

/* ======================================================================= */
/*
 * @def NBAMRENC_APP_ID  App ID Value setting
 */
/* ======================================================================= */
#define NBAMRENC_APP_ID 100

/* ======================================================================= */
/**
 * @def    NBAMRENC_SAMPLING_FREQUENCY   Sampling frequency
 */
/* ======================================================================= */
#define NBAMRENC_SAMPLING_FREQUENCY 8000
/* ======================================================================= */
/**
 * @def    NBAMRENC_CPU_LOAD                    CPU Load in MHz
 */
/* ======================================================================= */
#define NBAMRENC_CPU_LOAD 12
/* ======================================================================= */
/**
 * @def    NBAMRENC_MAX_NUM_OF_BUFS   Maximum number of buffers
 */
/* ======================================================================= */
#define NBAMRENC_MAX_NUM_OF_BUFS 15
/* ======================================================================= */
/**
 * @def    NBAMRENC_NUM_OF_PORTS   Number of ports
 */
/* ======================================================================= */
#define NBAMRENC_NUM_OF_PORTS 2
/* ======================================================================= */
/**
 * @def    NBAMRENC_XXX_VER    Component version
 */
/* ======================================================================= */
#define NBAMRENC_MAJOR_VER 0x1
#define NBAMRENC_MINOR_VER 0x1
/* ======================================================================= */
/**
 * @def    NBAMRENC_NOT_USED    Defines a value for "don't care" parameters
 */
/* ======================================================================= */
#define NBAMRENC_NOT_USED 10
/* ======================================================================= */
/**
 * @def    NBAMRENC_NORMAL_BUFFER  Defines flag value with all flags off
 */
/* ======================================================================= */
#define NBAMRENC_NORMAL_BUFFER 0
/* ======================================================================= */
/**
 * @def    OMX_NBAMRENC_DEFAULT_SEGMENT    Default segment ID for the LCML
 */
/* ======================================================================= */
#define NBAMRENC_DEFAULT_SEGMENT (0)
/* ======================================================================= */
/**
 * @def    OMX_NBAMRENC_SN_TIMEOUT    Timeout value for the socket node
 */
/* ======================================================================= */
#define NBAMRENC_SN_TIMEOUT (-1)
/* ======================================================================= */
/**
 * @def    OMX_NBAMRENC_SN_PRIORITY   Priority for the socket node
 */
/* ======================================================================= */
#define NBAMRENC_SN_PRIORITY (10)
/* ======================================================================= */
/**
 * @def    OMX_NBAMRENC_NUM_DLLS   number of DLL's
 */
/* ======================================================================= */
#define NBAMRENC_NUM_DLLS (2)
/* ======================================================================= */
/**
 * @def    NBAMRENC_USN_DLL_NAME   USN DLL name
 */
/* ======================================================================= */
#ifdef UNDER_CE
    #define NBAMRENC_USN_DLL_NAME "\\windows\\usn.dll64P"
#else
    #define NBAMRENC_USN_DLL_NAME "usn.dll64P"
#endif

/* ======================================================================= */
/**
 * @def    NBAMRENC_DLL_NAME   NBAMR Encoder socket node dll name
 */
/* ======================================================================= */
#ifdef UNDER_CE
    #define NBAMRENC_DLL_NAME "\\windows\\nbamrenc_sn.dll64P"
#else
    #define NBAMRENC_DLL_NAME "nbamrenc_sn.dll64P"
#endif

/* ======================================================================= */
/** NBAMRENC_StreamType  Stream types
*
*  @param  NBAMRENC_DMM                 DMM
*
*  @param  NBAMRENC_INSTRM              Input stream
*
*  @param  NBAMRENC_OUTSTRM             Output stream
*/
/* ======================================================================= */
enum NBAMRENC_StreamType {
    NBAMRENC_DMM = 0,
    NBAMRENC_INSTRM,
    NBAMRENC_OUTSTRM
};
/* ======================================================================= */
/** NBAMRENC_EncodeType  coding types
*
*  @param  NBAMRENC_NBAMR           NBAMR mode
*
*  @param  NBAMRENC_EFR             EFR mode
*
*/
/* ======================================================================= */
enum NBAMRENC_EncodeType {
    NBAMRENC_NBAMR = 0,
    NBAMRENC_EFR
};
/* ======================================================================= */
/** NBAMRENC_MimeMode  format types
*
*  @param  NBAMRENC_MIMEMODE                MIME
*
*  @param  NBAMRENC_FORMATCONFORMANCE       NBAMR mode
*
*  @param  NBAMRENC_IF2                     IF2
*
*/
/* ======================================================================= */
enum NBAMRENC_MimeMode {
    NBAMRENC_FORMATCONFORMANCE = 0,
    NBAMRENC_MIMEMODE, 
    NBAMRENC_IF2
};

/* ======================================================================= */
/*
 * Different Frame sizes for different index in MIME Mode
 */
/* ======================================================================= */
#define NBAMRENC_FRAME_SIZE_0   0
#define NBAMRENC_FRAME_SIZE_1   1
#define NBAMRENC_FRAME_SIZE_6   6
#define NBAMRENC_FRAME_SIZE_13  13
#define NBAMRENC_FRAME_SIZE_14  14
#define NBAMRENC_FRAME_SIZE_16  16
#define NBAMRENC_FRAME_SIZE_18  18
#define NBAMRENC_FRAME_SIZE_19  19
#define NBAMRENC_FRAME_SIZE_20  20
#define NBAMRENC_FRAME_SIZE_21  21
#define NBAMRENC_FRAME_SIZE_26  26
#define NBAMRENC_FRAME_SIZE_27  27
#define NBAMRENC_FRAME_SIZE_31  31
#define NBAMRENC_FRAME_SIZE_32  32



/* ======================================================================= */
/**
 * @def NBAMRENC_TIMEOUT Default timeout used to come out of blocking calls
 */
/* ======================================================================= */
#define NBAMRENC_TIMEOUT 1000
/* ======================================================================= */
/*
 * @def NBAMRENC_OMX_MAX_TIMEOUTS   Max Time Outs
 * @def NBAMRENC_DONT_CARE          Dont Care Condition
 * @def NBAMRENC_NUM_CHANNELS       Number of Channels
 * @def NBAMRENC_APP_ID             App ID Value setting
 */
/* ======================================================================= */
#define NBAMRENC_OMX_MAX_TIMEOUTS 20
#define NBAMRENC_DONT_CARE 0
#define NBAMRENC_NUM_CHANNELS 1
/* ======================================================================= */
/**
 * @def    NBAMRENC_STREAM_COUNT    Number of streams
 *         NBAMRENC_INPUT_STREAM_ID Stream ID for Input Buffer
 */
/* ======================================================================= */
#define NBAMRENC_STREAM_COUNT 2
#define NBAMRENC_INPUT_STREAM_ID 0

/* ======================================================================= */
/**
 * @def _ERROR_PROPAGATION__     Allow Logic to Detec&Report Arm Errors
 */
/* ======================================================================= */
#define _ERROR_PROPAGATION__


typedef struct PV_OMXComponentCapabilityFlagsType
{
        ////////////////// OMX COMPONENT CAPABILITY RELATED MEMBERS (for opencore compatability)
        OMX_BOOL iIsOMXComponentMultiThreaded;
        OMX_BOOL iOMXComponentSupportsExternalOutputBufferAlloc;
        OMX_BOOL iOMXComponentSupportsExternalInputBufferAlloc;
        OMX_BOOL iOMXComponentSupportsMovableInputBuffers;
        OMX_BOOL iOMXComponentSupportsPartialFrames;
        OMX_BOOL iOMXComponentNeedsNALStartCode;
        OMX_BOOL iOMXComponentCanHandleIncompleteFrames;
} PV_OMXComponentCapabilityFlagsType;
/* ======================================================================= */
/** NBAMRENC_COMP_PORT_TYPE  Port types
 *
 *  @param  NBAMRENC_INPUT_PORT             Input port
 *
 *  @param  NBAMRENC_OUTPUT_PORT            Output port
 */
/*  ====================================================================== */
/*This enum must not be changed. */
typedef enum NBAMRENC_COMP_PORT_TYPE {
    NBAMRENC_INPUT_PORT = 0,
    NBAMRENC_OUTPUT_PORT
}NBAMRENC_COMP_PORT_TYPE;

/* ======================================================================= */
/** NBAMRENC_BUFFER_Dir  Buffer Direction
*
*  @param  NBAMRENC_DIRECTION_INPUT     Input direction
*
*  @param  NBAMRENC_DIRECTION_OUTPUT    Output direction
*
*/
/* ======================================================================= */
typedef enum {
    NBAMRENC_DIRECTION_INPUT,
    NBAMRENC_DIRECTION_OUTPUT
}NBAMRENC_BUFFER_Dir;

/* ======================================================================= */
/** AUDIO_SN_AMRBANDMODETYPE            BitRate Enum on the format used by
*                                       the SN
*  @param  SN_AUDIO_BR_X
*/
/* ======================================================================= */
typedef enum AUDIO_SN_AMRBANDMODETYPE {
     SN_AUDIO_BR122 = 0,
     SN_AUDIO_BR102,
     SN_AUDIO_BR795,
     SN_AUDIO_BR74,
     SN_AUDIO_BR67,
     SN_AUDIO_BR59,
     SN_AUDIO_BR515,
     SN_AUDIO_475,
     SN_AUDIO_AMRBandModeMax = 0x7FFFFFFF
}AUDIO_SN_AMRBANDMODETYPE;

/* ======================================================================= */
/** NBAMRENC_BUFFS  Buffer details
*
*  @param  BufHeader Buffer header
*
*  @param  Buffer   Buffer
*
*/
/* ======================================================================= */
typedef struct NBAMRENC_BUFFS {
    char BufHeader;
    char Buffer;
}NBAMRENC_BUFFS;

/* ======================================================================= */
/** NBAMRENC_BUFFERHEADERTYPE_INFO
*
*  @param  pBufHeader
*
*  @param  bBufOwner
*
*/
/* ======================================================================= */
typedef struct NBAMRENC_BUFFERHEADERTYPE_INFO {
    OMX_BUFFERHEADERTYPE* pBufHeader[NBAMRENC_MAX_NUM_OF_BUFS];
    NBAMRENC_BUFFS bBufOwner[NBAMRENC_MAX_NUM_OF_BUFS];
}NBAMRENC_BUFFERHEADERTYPE_INFO;


typedef OMX_ERRORTYPE (*NBAMRENC_fpo)(OMX_HANDLETYPE);

/* =================================================================================== */
/**
* Socket node Audio Codec Configuration parameters.
*/
/* =================================================================================== */
typedef struct NBAMRENC_AudioCodecParams {
    unsigned long  iSamplingRate;
    unsigned long  iStrmId;
    unsigned short iAudioFormat;
}NBAMRENC_AudioCodecParams;

/* =================================================================================== */
/**
* NBAMRENC_TALGCtrl                 Socket Node Alg Control parameters.
* NBAMRENC_TALGCtrlDTX                 Socket Node Alg Control parameters (DTX).
* NBAMRENC_UAlgInBufParamStruct     Input Buffer Param Structure
* NBAMRENC_UAlgOutBufParamStruct    Output Buffer Param Structure
*/
/* =================================================================================== */
/* Algorithm specific command parameters */
typedef struct {
    int iSize;
    unsigned int iBitrate;

}NBAMRENC_TALGCtrl;

typedef struct {
    int iSize;
    unsigned int iVADFlag;

}NBAMRENC_TALGCtrlDTX;
/* =================================================================================== */
/**
* NBAMRENC_UAlgInBufParamStruct     Input Buffer Param Structure
* usLastFrame                       To Send Last Buufer Flag
*/
/* =================================================================================== */
typedef struct {
        unsigned long int usLastFrame;
}NBAMRENC_FrameStruct;

typedef struct{
         unsigned long int usNbFrames;
         NBAMRENC_FrameStruct *pParamElem;
}NBAMRENC_ParamStruct;

/* =================================================================================== */
/**
* NBAMRENC_LCML_BUFHEADERTYPE Buffer Header Type
*/
/* =================================================================================== */
typedef struct NBAMRENC_LCML_BUFHEADERTYPE {
      NBAMRENC_BUFFER_Dir eDir;
      NBAMRENC_FrameStruct *pFrameParam;
      NBAMRENC_ParamStruct *pBufferParam;
      DMM_BUFFER_OBJ* pDmmBuf;
      OMX_BUFFERHEADERTYPE* buffer;
}NBAMRENC_LCML_BUFHEADERTYPE;

typedef struct _NBAMRENC_BUFFERLIST NBAMRENC_BUFFERLIST;

/* =================================================================================== */
/**
* _NBAMRENC_BUFFERLIST Structure for buffer list
*/
/* ================================================================================== */
struct _NBAMRENC_BUFFERLIST{
    OMX_BUFFERHEADERTYPE sBufHdr;
    OMX_BUFFERHEADERTYPE *pBufHdr[NBAMRENC_MAX_NUM_OF_BUFS];
    OMX_U32 bufferOwner[NBAMRENC_MAX_NUM_OF_BUFS];
    OMX_U32 bBufferPending[NBAMRENC_MAX_NUM_OF_BUFS];
    OMX_U16 numBuffers;
    NBAMRENC_BUFFERLIST *pNextBuf;
    NBAMRENC_BUFFERLIST *pPrevBuf;
};

/* =================================================================================== */
/**
* NBAMRENC_PORT_TYPE Structure for PortFormat details
*/
/* =================================================================================== */
typedef struct NBAMRENC_PORT_TYPE {
    OMX_HANDLETYPE hTunnelComponent;
    OMX_U32 nTunnelPort;
    OMX_BUFFERSUPPLIERTYPE eSupplierSetting;
    OMX_U8 nBufferCnt;
    OMX_AUDIO_PARAM_PORTFORMATTYPE* pPortFormat;
} NBAMRENC_PORT_TYPE;

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

/* =================================================================================== */
/**
* NBAMRENC_BUFDATA
*/
/* =================================================================================== */
typedef struct NBAMRENC_BUFDATA {
   OMX_U8 nFrames;     
}NBAMRENC_BUFDATA;

/* =================================================================================== */
/**
* AMRENC_COMPONENT_PRIVATE Component private data Structure
*/
/* =================================================================================== */
typedef struct AMRENC_COMPONENT_PRIVATE
{
    /** Array of pointers to BUFFERHEADERTYPE structues
       This pBufHeader[INPUT_PORT] will point to all the
       BUFFERHEADERTYPE structures related to input port,
       not just one structure. Same is the case for output
       port also. */
    OMX_BUFFERHEADERTYPE* pBufHeader[NBAMRENC_NUM_OF_PORTS];
    OMX_U32 nRuntimeInputBuffers;

    OMX_U32 nRuntimeOutputBuffers;
    OMX_CALLBACKTYPE cbInfo;
    OMX_PORT_PARAM_TYPE* sPortParam;
    OMX_PRIORITYMGMTTYPE* sPriorityMgmt;
    
#ifdef RESOURCE_MANAGER_ENABLED
    RMPROXY_CALLBACKTYPE rmproxyCallback;
#endif

    OMX_BOOL bPreempted;
    
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef[NBAMRENC_NUM_OF_PORTS];
    OMX_PORT_PARAM_TYPE* pPortParamType;
    OMX_AUDIO_PARAM_AMRTYPE* amrParams;
    OMX_AUDIO_PARAM_PCMMODETYPE* pcmParams;
    NBAMRENC_BUFFERHEADERTYPE_INFO BufInfo[NBAMRENC_NUM_OF_PORTS];
    NBAMRENC_PORT_TYPE *pCompPort[NBAMRENC_NUM_OF_PORTS];
    NBAMRENC_LCML_BUFHEADERTYPE *pLcmlBufHeader[NBAMRENC_NUM_OF_PORTS];
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
    OMX_U32 bIsStopping;

    OMX_U32 bIsThreadstop;

    /** Count of number of buffers outstanding with bridge */
    OMX_U32 lcml_nIpBuf;

    /** Count of number of buffers outstanding with bridge */
    OMX_U32 lcml_nOpBuf;

    OMX_U32 app_nBuf;

    OMX_U32 num_Op_Issued;

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
    
    OMX_U32 bEnableCommandPending;

    OMX_U32 bDisableCommandParam;
    
    OMX_U32 bEnableCommandParam;

    OMX_HANDLETYPE pLcmlHandle;

    OMX_PTR pMarkData;

    OMX_MARKTYPE *pMarkBuf;

    OMX_HANDLETYPE hMarkTargetComponent;

    NBAMRENC_BUFFERLIST *pInputBufferList;

    NBAMRENC_BUFFERLIST *pOutputBufferList;

    LCML_STRMATTR *strmAttr;

    NBAMRENC_TALGCtrl *pAlgParam;

    NBAMRENC_TALGCtrlDTX *pAlgParamDTX;

    NBAMRENC_AudioCodecParams *pParams;

    OMX_STRING cComponentName;

    OMX_VERSIONTYPE ComponentVersion;

    OMX_BUFFERHEADERTYPE *pInputBufHdrPending[NBAMRENC_MAX_NUM_OF_BUFS];

    OMX_BUFFERHEADERTYPE *pOutputBufHdrPending[NBAMRENC_MAX_NUM_OF_BUFS];

    OMX_BUFFERHEADERTYPE *iMMFDataLastBuffer;

    OMX_U8 *pHoldBuffer,*pHoldBuffer2;

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

    pthread_mutex_t codecStop_mutex;    
    pthread_cond_t codecStop_threshold;
    OMX_U8 codecStop_waitingsignal;

    pthread_mutex_t InLoaded_mutex;
    pthread_cond_t InLoaded_threshold;
    OMX_U8 InLoaded_readytoidle;

    pthread_mutex_t InIdle_mutex;
    pthread_cond_t InIdle_threshold;
    OMX_U8 InIdle_goingtoloaded;
    
    OMX_U8 nUnhandledFillThisBuffers;
    OMX_U8 nUnhandledEmptyThisBuffers;
    OMX_BOOL bFlushOutputPortCommandPending;
    OMX_BOOL bFlushInputPortCommandPending;

    pthread_mutex_t ToLoaded_mutex;
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

    OMX_U8 PendingPausedBufs;
    OMX_BUFFERHEADERTYPE *pOutputBufHdrPausedPending[NBAMRENC_MAX_NUM_OF_BUFS];

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
    OMX_S64 arrBufIndex[NBAMRENC_MAX_NUM_OF_BUFS]; 
    /** Circular array to keep buffer nTickCounts */
    OMX_S64 arrTickCount[NBAMRENC_MAX_NUM_OF_BUFS]; 
    /** Index to arrBufIndex[], used for input buffer timestamps */
    OMX_U8 IpBufindex;
    /** Index to arrBufIndex[], used for output buffer timestamps */
    OMX_U8 OpBufindex;  
    OMX_TICKS TimeStamp;
    OMX_BOOL bFirstInputBufReceived;

    OMX_S8 ProcessingInputBuf;
    OMX_S8 ProcessingOutputBuf; 
    
    OMX_BOOL bLoadedCommandPending;
    OMX_BOOL bLoadedWaitingFreeBuffers;
    
    OMX_PARAM_COMPONENTROLETYPE componentRole;
    OMX_U32 teeMode;
    PV_OMXComponentCapabilityFlagsType iPVCapabilityFlags;

    struct OMX_TI_Debug dbg;

    /* Reference count for pending state change requests */
    OMX_U32 nPendingStateChangeRequests;
    pthread_mutex_t mutexStateChangeRequest;
    pthread_cond_t StateChangeCondition;
} AMRENC_COMPONENT_PRIVATE;


#ifndef UNDER_CE
    OMX_ERRORTYPE OMX_ComponentInit (OMX_HANDLETYPE hComp);
#else
/*  WinCE Implicit Export Syntax */
#define OMX_EXPORT __declspec(dllexport)
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
OMX_EXPORT OMX_ERRORTYPE OMX_ComponentInit (OMX_HANDLETYPE hComp);
#endif
/* =================================================================================== */
/**
*  NBAMRENC_StartComponentThread()  Starts component thread
*
*
*  @param hComp         OMX Handle
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*
*/
/* =================================================================================== */
OMX_ERRORTYPE NBAMRENC_StartComponentThread(OMX_HANDLETYPE pHandle);
/* =================================================================================== */
/**
*  NBAMRENC_StopComponentThread()  Stops component thread
*
*
*  @param hComp         OMX Handle
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*
*/
/* =================================================================================== */
OMX_ERRORTYPE NBAMRENC_StopComponentThread(OMX_HANDLETYPE pHandle);
/* =================================================================================== */
/**
*  NBAMRENC_FreeCompResources()  Frees allocated memory
*
*
*  @param hComp         OMX Handle
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*
*/
/* =================================================================================== */
OMX_ERRORTYPE NBAMRENC_FreeCompResources(OMX_HANDLETYPE pComponent);
/* =================================================================================== */
/**
*  NBAMRENC_GetCorrespondingLCMLHeader()  Returns LCML header
* that corresponds to the given buffer
*
*  @param pComponentPrivate Component private data
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*/
/* =================================================================================== */
OMX_ERRORTYPE NBAMRENC_GetCorrespondingLCMLHeader(AMRENC_COMPONENT_PRIVATE *pComponentPrivate,
                                                  OMX_U8 *pBuffer,
                                                  OMX_DIRTYPE eDir,
                                                  NBAMRENC_LCML_BUFHEADERTYPE **ppLcmlHdr);
/* =================================================================================== */
/**
*  NBAMRENC_LCMLCallback() Callback from LCML
*
*  @param event     Codec Event
*
*  @param args      Arguments from LCML
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*/
/* =================================================================================== */
OMX_ERRORTYPE NBAMRENC_LCMLCallback(TUsnCodecEvent event,
                                    void * args [10]);
/* =================================================================================== */
/**
*  NBAMRENC_FillLCMLInitParams() Fills the parameters needed
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
OMX_ERRORTYPE NBAMRENC_FillLCMLInitParams(OMX_HANDLETYPE pHandle,
                                          LCML_DSP *plcml_Init,
                                          OMX_U16 arr[]);
/* =================================================================================== */
/**
*  NBAMRENC_GetBufferDirection() Returns direction of pBufHeader
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
OMX_ERRORTYPE NBAMRENC_GetBufferDirection(OMX_BUFFERHEADERTYPE *pBufHeader,
                                          OMX_DIRTYPE *eDir);
/* ===========================================================  */
/**
*  NBAMRENC_HandleCommand()  Handles commands sent via SendCommand()
*
*  @param pComponentPrivate Component private data
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*/
/* =================================================================================== */
OMX_U32 NBAMRENC_HandleCommand(AMRENC_COMPONENT_PRIVATE *pComponentPrivate);
/* =================================================================================== */
/**
*  NBAMRENC_HandleDataBufFromApp()  Handles data buffers received
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
OMX_ERRORTYPE NBAMRENC_HandleDataBufFromApp(OMX_BUFFERHEADERTYPE *pBufHeader,
                                            AMRENC_COMPONENT_PRIVATE *pComponentPrivate);
/* =================================================================================== */
/**
*  NBAMRENC_GetLCMLHandle()  Get the handle to the LCML
*
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*/
/* =================================================================================== */
OMX_HANDLETYPE NBAMRENC_GetLCMLHandle(AMRENC_COMPONENT_PRIVATE *pComponentPrivate);
/* =================================================================================== */
/**
*  NBAMRENC_FreeLCMLHandle()  Frees the handle to the LCML
*
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*/
/* =================================================================================== */
OMX_ERRORTYPE NBAMRENC_FreeLCMLHandle(AMRENC_COMPONENT_PRIVATE *pComponentPrivate);
/* =================================================================================== */
/**
*  NBAMRENC_CleanupInitParams()  Starts component thread
*
*  @param pComponent        OMX Handle
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*/
/* =================================================================================== */
OMX_ERRORTYPE NBAMRENC_CleanupInitParams(OMX_HANDLETYPE pHandle);
/* =================================================================================== */
/**
*  NBAMRENC_SetPending()  Called when the component queues a buffer
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
void NBAMRENC_SetPending(AMRENC_COMPONENT_PRIVATE *pComponentPrivate,
                         OMX_BUFFERHEADERTYPE *pBufHdr,
                         OMX_DIRTYPE eDir,
                         OMX_U32 lineNumber);
/* =================================================================================== */
/**
*  NBAMRENC_ClearPending()  Called when a buffer is returned
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
void NBAMRENC_ClearPending(AMRENC_COMPONENT_PRIVATE *pComponentPrivate,
                           OMX_BUFFERHEADERTYPE *pBufHdr,
                           OMX_DIRTYPE eDir,
                           OMX_U32 lineNumber);
/* =================================================================================== */
/**
*  NBAMRENC_IsPending()
*
*
*  @param pComponentPrivate     Component private data
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*/
/* =================================================================================== */
OMX_U32 NBAMRENC_IsPending(AMRENC_COMPONENT_PRIVATE *pComponentPrivate,
                           OMX_BUFFERHEADERTYPE *pBufHdr,
                           OMX_DIRTYPE eDir);
/* =================================================================================== */
/**
*  NBAMRENC_FillLCMLInitParamsEx()  Fills the parameters needed
* to initialize the LCML without recreating the socket node
*
*  @param pComponent            OMX Handle
*
*  @return None
*/
/* =================================================================================== */
OMX_ERRORTYPE NBAMRENC_FillLCMLInitParamsEx(OMX_HANDLETYPE pComponent);
/* =================================================================================== */
/**
*  NBAMRENC_IsValid() Returns whether a buffer is valid
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
OMX_U32 NBAMRENC_IsValid(AMRENC_COMPONENT_PRIVATE *pComponentPrivate,
                         OMX_U8 *pBuffer,
                         OMX_DIRTYPE eDir);

#ifdef RESOURCE_MANAGER_ENABLED
void NBAMRENC_ResourceManagerCallback(RMPROXY_COMMANDDATATYPE cbData);
#endif
/* ======================================================================= */
/** OMX_NBAMRENC_INDEXAUDIOTYPE  Defines the custom configuration settings
*                              for the component
*
*  @param  OMX_IndexCustomNBAMRENCModeConfig      Sets the DASF mode
*
*
*/
/*  ==================================================================== */
typedef enum OMX_NBAMRENC_INDEXAUDIOTYPE {
    OMX_IndexCustomNBAMRENCModeConfig = 0xFF000001,
    OMX_IndexCustomNBAMRENCStreamIDConfig,
    OMX_IndexCustomNBAMRENCDataPath,
    OMX_IndexCustomDebug
}OMX_NBAMRENC_INDEXAUDIOTYPE;

OMX_ERRORTYPE OMX_DmmMap(DSP_HPROCESSOR ProcHandle, int size, void* pArmPtr, DMM_BUFFER_OBJ* pDmmBuf, struct OMX_TI_Debug dbg);
OMX_ERRORTYPE OMX_DmmUnMap(DSP_HPROCESSOR ProcHandle, void* pMapPtr, void* pResPtr, struct OMX_TI_Debug dbg);

void NBAMRENC_HandleUSNError (AMRENC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U32 arg);
OMX_ERRORTYPE AddStateTransition(AMRENC_COMPONENT_PRIVATE *pComponentPrivate);
OMX_ERRORTYPE RemoveStateTransition(AMRENC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BOOL bEnableSignal);

/*===============================================================*/

typedef enum {
    IUALG_CMD_STOP             = 0,
    IUALG_CMD_PAUSE            = 1,
    IUALG_CMD_GETSTATUS        = 2,
    IUALG_CMD_SETSTATUS        = 3,
    IUALG_CMD_USERSETCMDSTART  = 100,
    IUALG_CMD_USERGETCMDSTART  = 150,
    IUALG_CMD_FLUSH            = 0x100
}IUALG_Cmd;

typedef enum
{
    ALGCMD_BITRATE = IUALG_CMD_USERSETCMDSTART,
    ALGCMD_DTX

} eSPEECHENCODE_AlgCmd;

#endif  /* OMX_AMRENC_UTILS__H */
