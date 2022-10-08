
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
 * @file OMX_G729Enc_Utils.h
 *
 * This is an header file for an G729 Encoder that is fully
 * compliant with the OMX Audio specification 1.5.
 * This the file that the application that uses OMX would include in its code.
 *
 * @path $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\g729_enc\inc
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
#ifndef OMX_G729ENC_UTILS__H
#define OMX_G729ENC_UTILS__H

#include <pthread.h>
#include "LCML_DspCodec.h"
#include <OMX_Component.h>
#include <TIDspOmx.h>

#ifdef RESOURCE_MANAGER_ENABLED
#include <ResourceManagerProxyAPI.h>
#endif

#undef __G729_EPRINT__


#ifndef UNDER_CE
/* For printing errors */
#define __OMX_EPRINT__
#undef __G729_EPRINT__
#endif

#ifdef __PERF_INSTRUMENTATION__
#include "perf.h"
#endif

/* ======================================================================= */
/**
 * @def    G729ENC_DEBUG   Turns debug messaging on and off
 */
/* ======================================================================= */
#undef G729ENC_DEBUG
/* ======================================================================= */
/**
 * @def    G729ENC_MEMCHECK   Turns memory messaging on and off
 */
/* ======================================================================= */
#undef G729ENC_MEMCHECK

#ifndef UNDER_CE
/*
 *  ANSI escape sequences for outputing text in various colors
 */
#ifdef OMX_PRINT_COLOR
#define DBG_TEXT_WHITE   "\x1b[1;37;40m"
#define DBG_TEXT_YELLOW  "\x1b[1;33;40m" /* reserved for OMX_G729Enc_Thread.c */
#define DBG_TEXT_MAGENTA "\x1b[1;35;40m" 
#define DBG_TEXT_GREEN   "\x1b[1;32;40m" /* reserved for OMX_G729Encoder.c */
#define DBG_TEXT_CYAN    "\x1b[1;36;40m" /* reserved for OMX_G729Enc_Utils.c */
#define DBG_TEXT_RED     "\x1b[1;31;40m" /* reserved for test app */ 
#else
#define DBG_TEXT_WHITE   ""
#define DBG_TEXT_YELLOW  ""
#define DBG_TEXT_MAGENTA ""
#define DBG_TEXT_GREEN   ""
#define DBG_TEXT_CYAN    ""
#define DBG_TEXT_RED     ""
#endif

void G729ENC_eprint(int iLineNum, const char *szFunctionName, const char *strFormat, ...);
void G729ENC_Log(const char *szFileName, int iLineNum, const char *szFunctionName, const char *strFormat, ...);

/* ======================================================================= */
/**
 * @def    G729ENC_DEBUG   Debug print macro
 */
/* ======================================================================= */
#ifdef  G729ENC_DEBUG
#define G729ENC_DPRINT(STR, ARG...) G729ENC_Log(__FILE__, __LINE__, __FUNCTION__, STR, ##ARG)
#else
#define G729ENC_DPRINT(...)
#endif
/* ======================================================================= */
/**
 * @def    G729ENC_MEMCHECK   Memory print macro
 */
/* ======================================================================= */
#ifdef  G729ENC_MEMCHECK
#define G729ENC_MEMPRINT(...)    fprintf(stderr, __VA_ARGS__)
#else
#define G729ENC_MEMPRINT(...)
#endif
/* ======================================================================= */
/**
 * @def    OMX_EPRINT   Error print macro
 */
/* ======================================================================= */
#ifdef __OMX_EPRINT__
#define OMX_EPRINT(STR, ARG...) G729ENC_eprint(__LINE__, __FUNCTION__, STR, ##ARG)
#else
#define OMX_EPRINT(...)
#endif
/* ======================================================================= */
/**
 * @def    G729ENC_EPRINT   Error print macro
 */
/* ======================================================================= */
#ifdef __G729_EPRINT__
#define G729ENC_EPRINT(...)    fprintf(stderr,__VA_ARGS__)
#else
#define G729ENC_EPRINT(...)
#endif


#else   /*UNDER_CE*/
/* ======================================================================= */
/**
 * @def    G729ENC_DEBUG   Debug print macro
 */
/* ======================================================================= */
#ifdef  G729ENC_DEBUG
#define G729ENC_DPRINT(STR, ARG...) printf()
#endif
/* ======================================================================= */
/**
 * @def    G729ENC_MEMCHECK   Memory print macro
 */
/* ======================================================================= */
#ifdef  G729ENC_MEMCHECK
#define G729ENC_MEMPRINT(STR, ARG...) printf()
#endif

#define G729ENC_EPRINT         printf
#define OMX_EPRINT             G729ENC_EPRINT
    
#ifdef DEBUG
#define G729ENC_DPRINT     printf
#define G729ENC_MEMPRINT   printf
#else
#define G729ENC_DPRINT
#define G729ENC_MEMPRINT
#endif

#endif/*UNDER_CE*/

/* ======================================================================= */
/**
 *  M A C R O S FOR MALLOC and MEMORY FREE and CLOSING PIPES
 */
/* ======================================================================= */

#define OMX_G729CONF_INIT_STRUCT(_s_, _name_)   \
    memset((_s_), 0x0, sizeof(_name_));         \
    (_s_)->nSize = sizeof(_name_);              \
    (_s_)->nVersion.s.nVersionMajor = 0x1;      \
    (_s_)->nVersion.s.nVersionMinor = 0x1;      \
    (_s_)->nVersion.s.nRevision = 0x0;          \
    (_s_)->nVersion.s.nStep = 0x0

#define OMX_G729MEMFREE_STRUCT(_pStruct_)                       \
    if(_pStruct_ != NULL)                                       \
    {                                                           \
        G729ENC_MEMPRINT("%d :: [FREE] %p\n", __LINE__, _pStruct_); \
        free(_pStruct_);                                        \
        _pStruct_ = NULL;                                       \
    }

#define OMX_G729CLOSE_PIPE(_pStruct_,err)                       \
    G729ENC_DPRINT("%d :: CLOSING PIPE \n", __LINE__);          \
    err = close (_pStruct_);                                    \
    if(0 != err && OMX_ErrorNone == eError)                     \
    {                                                           \
        eError = OMX_ErrorHardware;                             \
        printf("%d :: Error while closing pipe\n", __LINE__);   \
        goto EXIT;                                              \
    }

#define OMX_G729MALLOC_STRUCT(_pStruct_, _sName_)                   \
    _pStruct_ = (_sName_*)malloc(sizeof(_sName_));                  \
    if(_pStruct_ == NULL)                                           \
    {                                                               \
        printf("***********************************\n");            \
        printf("%d :: Malloc Failed\n", __LINE__);                  \
        printf("***********************************\n");            \
        eError = OMX_ErrorInsufficientResources;                    \
        goto EXIT;                                                  \
    }                                                               \
    memset(_pStruct_, 0x0, sizeof(_sName_));                        \
    G729ENC_MEMPRINT("%d :: [ALLOC] %p\n", __LINE__, _pStruct_);


/* ======================================================================= */
/**
 * @def G729ENC_NUM_INPUT_BUFFERS   Default number of input buffers
 */
/* ======================================================================= */
#define G729ENC_NUM_INPUT_BUFFERS 1
/* ======================================================================= */
/**
 * @def G729ENC_NUM_INPUT_BUFFERS_DASF  Default No.of input buffers DASF
 */
/* ======================================================================= */
#define G729ENC_NUM_INPUT_BUFFERS_DASF 2
/* ======================================================================= */
/**
 * @def G729ENC_NUM_OUTPUT_BUFFERS   Default number of output buffers
 */
/* ======================================================================= */
#define G729ENC_NUM_OUTPUT_BUFFERS 1
/* ======================================================================= */
/**
 * @def G729ENC_INPUT_BUFFER_SIZE        Default input buffer size
 *      G729ENC_INPUT_BUFFER_SIZE_DASF  Default input buffer size DASF
 *      G729ENC_INPUT_FRAME_SIZE        Default input Frame Size
 */
/* ======================================================================= */
#define G729ENC_INPUT_BUFFER_SIZE       160
#define G729ENC_INPUT_BUFFER_SIZE_DASF  160
#define G729ENC_INPUT_FRAME_SIZE        160
/* ======================================================================= */
/**
 * @def G729ENC_OUTPUT_BUFFER_SIZE   Default output buffer size
 *      G729ENC_OUTPUT_FRAME_SIZE     Default output frame size
 */
/* ======================================================================= */
#define G729ENC_OUTPUT_BUFFER_SIZE              12
#define G729ENC_OUTPUT_FRAME_SIZE               12 
#define G729ENC_HEADER_SIZE                      2

/* ======================================================================= */
/**
 * @def G729ENC_OUTPUT_BUFFER_SIZE_MIME  Default input buffer size MIME
 */
/* ======================================================================= */
#define G729ENC_OUTPUT_BUFFER_SIZE_MIME 10 

/* ======================================================================= */
/*
 * @def G729ENC_APP_ID  App ID Value setting
 */
/* ======================================================================= */
#define G729ENC_APP_ID 100

/* ======================================================================= */
/**
 * @def    G729ENC_SAMPLING_FREQUENCY   Sampling frequency
 */
/* ======================================================================= */
#define G729ENC_SAMPLING_FREQUENCY 8000
/* ======================================================================= */
/**
 * @def    G729ENC_MAX_NUM_OF_BUFS   Maximum number of buffers
 */
/* ======================================================================= */
#define G729ENC_MAX_NUM_OF_BUFS 10
/* ======================================================================= */
/**
 * @def    G729ENC_NUM_OF_PORTS   Number of ports
 */
/* ======================================================================= */
#define G729ENC_NUM_OF_PORTS 2
/* ======================================================================= */
/**
 * @def    G729ENC_XXX_VER    Component version
 */
/* ======================================================================= */
#define G729ENC_MAJOR_VER 0x1
#define G729ENC_MINOR_VER 0x1
/* ======================================================================= */
/**
 * @def    G729ENC_NOT_USED    Defines a value for "don't care" parameters
 */
/* ======================================================================= */
#define G729ENC_NOT_USED 10
/* ======================================================================= */
/**
 * @def    G729ENC_NORMAL_BUFFER  Defines flag value with all flags off
 */
/* ======================================================================= */
#define G729ENC_NORMAL_BUFFER 0
/* ======================================================================= */
/**
 * @def    OMX_G729ENC_DEFAULT_SEGMENT    Default segment ID for the LCML
 */
/* ======================================================================= */
#define G729ENC_DEFAULT_SEGMENT (0)
/* ======================================================================= */
/**
 * @def    OMX_G729ENC_SN_TIMEOUT    Timeout value for the socket node
 */
/* ======================================================================= */
#define G729ENC_SN_TIMEOUT (-1)
/* ======================================================================= */
/**
 * @def    OMX_G729ENC_SN_PRIORITY   Priority for the socket node
 */
/* ======================================================================= */
#define G729ENC_SN_PRIORITY (10)
/* ======================================================================= */
/**
 * @def    OMX_G729ENC_NUM_DLLS   number of DLL's
 */
/* ======================================================================= */
#define G729ENC_NUM_DLLS (2)

#define G729ENC_CPU 50
/* ======================================================================= */
/**
 * @def    G729ENC_USN_DLL_NAME   USN DLL name
 */
/* ======================================================================= */
#ifdef UNDER_CE
#define G729ENC_USN_DLL_NAME "\\windows\\usn.dll64P"
#else
#define G729ENC_USN_DLL_NAME "usn.dll64P"
#endif

/* ======================================================================= */
/**
 * @def    G729ENC_DLL_NAME   G729 Encoder socket node dll name
 */
/* ======================================================================= */
#ifdef UNDER_CE
#define G729ENC_DLL_NAME "\\windows\\g729enc_sn.dll64P"
#else
#define G729ENC_DLL_NAME "g729enc_sn.dll64P"
#endif

/* ======================================================================= */
/**
 * @def _ERROR_PROPAGATION__     Allow Logic to Detec&Report Arm Errors
 */
/* ======================================================================= */
#define _ERROR_PROPAGATION__ 

/* ======================================================================= */
/** G729ENC_StreamType  Stream types
 *
 *  @param  G729ENC_DMM                                              DMM
 *
 *  @param  G729ENC_INSTRM                                         Input stream
 *
 *  @param  G729ENC_OUTSTRM                                       Output stream
 */
/* ======================================================================= */
enum G729ENC_StreamType
    {
        G729ENC_DMM = 0,
        G729ENC_INSTRM,
        G729ENC_OUTSTRM
    };
/* ======================================================================= */
/** G729ENC_EncodeType  coding types
 *
 *  @param  G729ENC_G729                             G729 mode
 *
 *  @param  G729ENC_EFR                                   EFR mode
 *
 */
/* ======================================================================= */
enum G729ENC_EncodeType
    {
        G729ENC_G729 = 0,
        G729ENC_EFR
    };
/* ======================================================================= */
/** G729ENC_MimeMode  format types
 *
 *  @param  G729ENC_MIMEMODE                                         MIME
 *
 *  @param  G729ENC_NONMIMEMODE                                  G729 mode
 *
 */
/* ======================================================================= */
enum G729ENC_MimeMode
    {
        G729ENC_NONMIMEMODE = 0,
        G729ENC_MIMEMODE
    };

/* ======================================================================= */
/*
 * Different Frame sizes for different index in MIME Mode
 */
/* ======================================================================= */
#define G729ENC_FRAME_SIZE_13  13
#define G729ENC_FRAME_SIZE_14  14
#define G729ENC_FRAME_SIZE_16  16
#define G729ENC_FRAME_SIZE_18  18
#define G729ENC_FRAME_SIZE_20  20
#define G729ENC_FRAME_SIZE_21  21
#define G729ENC_FRAME_SIZE_27  27
#define G729ENC_FRAME_SIZE_32  32
#define G729ENC_FRAME_SIZE_6    6
#define G729ENC_FRAME_SIZE_1    1
#define G729ENC_FRAME_SIZE_0    0
/* ======================================================================= */
/**
 * @def G729ENC_TIMEOUT Default timeout used to come out of blocking calls
 */
/* ======================================================================= */
#define G729ENC_TIMEOUT 1000
/* ======================================================================= */
/*
 * @def  G729ENC_OMX_MAX_TIMEOUTS   Max Time Outs
 * @def  G729ENC_DONT_CARE                   Dont Care Condition
 * @def  G729ENC_NUM_CHANNELS            Number of Channels
 * @def  G729ENC_APP_ID                           App ID Value setting
 */
/* ======================================================================= */
#define G729ENC_OMX_MAX_TIMEOUTS 20
#define G729ENC_DONT_CARE 0
#define G729ENC_NUM_CHANNELS 1
/* ======================================================================= */
/**
 * @def    G729ENC_STREAM_COUNT    Number of streams
 *    G729ENC_INPUT_STREAM_ID Stream ID for Input Buffer
 */
/* ======================================================================= */
#define G729ENC_STREAM_COUNT 2
#define G729ENC_INPUT_STREAM_ID 0

/* ======================================================================= */
/** G729ENC_COMP_PORT_TYPE  Port types
 *
 *  @param  G729ENC_INPUT_PORT                            Input port
 *
 *  @param  G729ENC_OUTPUT_PORT                        Output port
 */
/*  ====================================================================== */
/*This enum must not be changed. */
typedef enum G729ENC_COMP_PORT_TYPE
    {
        G729ENC_INPUT_PORT = 0,
        G729ENC_OUTPUT_PORT
    } G729ENC_COMP_PORT_TYPE;

/* ======================================================================= */
/** G729ENC_BUFFER_Dir  Buffer Direction
 *
 *  @param  G729ENC_DIRECTION_INPUT          Input direction
 *
 *  @param  G729ENC_DIRECTION_OUTPUT        Output direction
 *
 */
/* ======================================================================= */
typedef enum G729ENC_BUFFER_Dir
    {
        G729ENC_DIRECTION_INPUT,
        G729ENC_DIRECTION_OUTPUT
    } G729ENC_BUFFER_Dir;

/* ======================================================================= */
/** G729ENC_BUFFS  Buffer details
 *
 *  @param  BufHeader Buffer header
 *
 *  @param  Buffer      Buffer
 *
 */
/* ======================================================================= */
typedef struct G729ENC_BUFFS
{
    char BufHeader;
    char Buffer;
} G729ENC_BUFFS;

/* ======================================================================= */
/** G729ENC_BUFFERHEADERTYPE_INFO
 *
 *  @param  pBufHeader
 *
 *  @param  bBufOwner
 *
 */
/* ======================================================================= */
typedef struct G729ENC_BUFFERHEADERTYPE_INFO
{
    OMX_BUFFERHEADERTYPE* pBufHeader[G729ENC_MAX_NUM_OF_BUFS];
    G729ENC_BUFFS bBufOwner[G729ENC_MAX_NUM_OF_BUFS];
} G729ENC_BUFFERHEADERTYPE_INFO;


typedef OMX_ERRORTYPE (*G729ENC_fpo)(OMX_HANDLETYPE);

/* =================================================================================== */
/**
 * Socket node Audio Codec Configuration parameters.
 */
/* =================================================================================== */
typedef struct G729ENC_AudioCodecParams
{
    unsigned long  iSamplingRate;
    unsigned long  iStrmId;
    unsigned short iAudioFormat;
} G729ENC_AudioCodecParams;

/* =================================================================================== */
/**
 * G729ENC_TALGCtrl                                Socket Node Alg Control parameters.
 * G729ENC_UAlgInBufParamStruct             Input Buffer Param Structure
 * G729ENC_UAlgOutBufParamStruct           Output Buffer Param Structure
 */
/* =================================================================================== */
/* Algorithm specific command parameters */
typedef struct ISPHENC_DynamicParams
{
    int size;                                      /* not used */
    int frameSize;                                 /* not used*/
    int bitRate;                                   /* not used */
    int mode;                                      /* not used */
    int vadFlag;                                   /* VAD_On =1, VAD_OFF = 0 */
    int noiseSuppressionMode;                      /* not used */
    int ttyTddMode;                                /* not used */
    int dtmfMode;                                  /* not used */
    int dataTransmit;                              /* not used */
} G729ENC_TALGCtrl;

/* =================================================================================== */
/**
 * G729ENC_UAlgInBufParamStruct             Input Buffer Param Structure
 * usEndOfFile                                            To Send Last Buffer Flag
 */
/* =================================================================================== */
typedef struct G729ENC_UAlgInBufParamStruct
{
    unsigned long usEndOfFile;
} G729ENC_UAlgInBufParamStruct;

/* =================================================================================== */
/**
 * G729ENC_UAlgOutBufParamStruct Output Buffer Param Structure
 * ulFrameCount No.of Frames Encoded
 */
/* =================================================================================== */
typedef struct G729ENC_UAlgOutBufParamStruct
{
    unsigned long ulFrameCount;
} G729ENC_UAlgOutBufParamStruct;

/* =================================================================================== */
/**
 * G729ENC_LCML_BUFHEADERTYPE Buffer Header Type
 */
/* =================================================================================== */
typedef struct G729ENC_LCML_BUFHEADERTYPE {
    G729ENC_BUFFER_Dir eDir;
    G729ENC_UAlgInBufParamStruct *pIpParam;
    G729ENC_UAlgOutBufParamStruct *pOpParam;
    OMX_BUFFERHEADERTYPE* buffer;
} G729ENC_LCML_BUFHEADERTYPE;


typedef struct _G729ENC_BUFFERLIST G729ENC_BUFFERLIST;

/* =================================================================================== */
/**
 * _G729ENC_BUFFERLIST Structure for buffer list
 */
/* ================================================================================== */
struct _G729ENC_BUFFERLIST
{
    OMX_BUFFERHEADERTYPE sBufHdr;
    OMX_BUFFERHEADERTYPE *pBufHdr[G729ENC_MAX_NUM_OF_BUFS];
    OMX_U32 bufferOwner[G729ENC_MAX_NUM_OF_BUFS];
    OMX_U32 bBufferPending[G729ENC_MAX_NUM_OF_BUFS];
    OMX_U32 numBuffers;
    G729ENC_BUFFERLIST *pNextBuf;
    G729ENC_BUFFERLIST *pPrevBuf;
};

/* =================================================================================== */
/**
 * G729ENC_PORT_TYPE Structure for PortFormat details
 */
/* =================================================================================== */
typedef struct G729ENC_PORT_TYPE
{
    OMX_HANDLETYPE hTunnelComponent;
    OMX_U32 nTunnelPort;
    OMX_BUFFERSUPPLIERTYPE eSupplierSetting;
    OMX_U8 nBufferCnt;
    OMX_AUDIO_PARAM_PORTFORMATTYPE* pPortFormat;
} G729ENC_PORT_TYPE;

#ifdef UNDER_CE
/* =================================================================================== */
/**
 * OMX_Event Structure for Mutex application under WinCE
 */
/* =================================================================================== */
typedef struct OMX_Event
{
    HANDLE event;
} OMX_Event;

int OMX_CreateEvent(OMX_Event *event);
int OMX_SignalEvent(OMX_Event *event);
int OMX_WaitForEvent(OMX_Event *event);
int OMX_DestroyEvent(OMX_Event *event);
#endif

/* =================================================================================== */
/**
 * G729ENC_COMPONENT_PRIVATE Component private data Structure
 */
/* =================================================================================== */
typedef struct G729ENC_COMPONENT_PRIVATE
{
    /** Array of pointers to BUFFERHEADERTYPE structues
        This pBufHeader[INPUT_PORT] will point to all the
        BUFFERHEADERTYPE structures related to input port,
        not just one structure. Same is the case for output
        port also. */
    OMX_BUFFERHEADERTYPE* pBufHeader[G729ENC_NUM_OF_PORTS];
    OMX_CALLBACKTYPE cbInfo;
    OMX_PORT_PARAM_TYPE* sPortParam;
    OMX_PRIORITYMGMTTYPE* sPriorityMgmt;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef[G729ENC_NUM_OF_PORTS];
    OMX_PORT_PARAM_TYPE* pPortParamType;
    OMX_AUDIO_PARAM_G729TYPE* g729Params;
    OMX_AUDIO_PARAM_PCMMODETYPE* pcmParams;
    G729ENC_BUFFERHEADERTYPE_INFO BufInfo[G729ENC_NUM_OF_PORTS];
    G729ENC_PORT_TYPE *pCompPort[G729ENC_NUM_OF_PORTS];
    G729ENC_LCML_BUFHEADERTYPE *pLcmlBufHeader[G729ENC_NUM_OF_PORTS];
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
    /** The pipes for sending buffers to the thread */
    int lcml_Pipe[2];

    OMX_U32 efrMode;

    OMX_U32 g729Mode;

    OMX_U32 dasfMode;

    OMX_U32 mimeMode;

    OMX_U32 acdnMode;

    OMX_U32 nMultiFrameMode;

    OMX_U32 fdwrite;

    OMX_U32 fdread;

    OMX_U32 bLcmlHandleOpened;

    /** Set to indicate component is stopping */
    OMX_U32 bIsStopping;

    OMX_U32 bIsThreadstop;

    OMX_U32 bIsEOFSent;

    /** Count of number of buffers outstanding with bridge */
    OMX_U32 lcml_nIpBuf;

    /** Count of number of buffers outstanding with bridge */
    OMX_U32 lcml_nOpBuf;

    OMX_U32 app_nBuf;

    OMX_U32 lcml_nCntIp;

    OMX_U32 lcml_nCntOpReceived;

    OMX_U32 lcml_nCntApp;

    OMX_U32 lcml_compID;

    OMX_U32 num_Reclaimed_Op_Buff;

    OMX_U32 num_Sent_Ip_Buff;

    OMX_U32 num_Op_Issued;

    OMX_U32 bIsBufferOwned[G729ENC_NUM_OF_PORTS];

    OMX_U32 streamID;

    OMX_U32 bPortDefsAllocated;

    OMX_U32 bCompThreadStarted;

    OMX_U32 bBypassDSP;

    OMX_U32 nVersion;

    OMX_U32 bInputBufferHeaderAllocated;

    OMX_U32 bPlayCompleteFlag;

    OMX_U32 g729MimeBytes[16];

    OMX_U32 iHoldLen;

    OMX_U32 nHoldLength;

    OMX_U32 nFillThisBufferCount;

    OMX_U32 nFillBufferDoneCount;

    OMX_U32 nEmptyThisBufferCount;

    OMX_U32 nEmptyBufferDoneCount;

    OMX_U32 bInitParamsInitialized;

    OMX_U32 bIdleCommandPending;

    OMX_U32 nNumInputBufPending;

    OMX_U32 nNumOutputBufPending;

    OMX_U32 bJustReenabled;

    OMX_U32 nInvalidFrameCount;

    OMX_U32 nDataWritesWhilePaused;

    OMX_U32 bDisableCommandPending;

    OMX_U32 bDisableCommandParam;

    OMX_HANDLETYPE pLcmlHandle;

    OMX_PTR pMarkData;

    OMX_MARKTYPE *pMarkBuf;

    OMX_HANDLETYPE hMarkTargetComponent;

    G729ENC_BUFFERLIST *pInputBufferList;

    G729ENC_BUFFERLIST *pOutputBufferList;

    LCML_STRMATTR *strmAttr;

    G729ENC_TALGCtrl *pAlgParam;

    G729ENC_AudioCodecParams *pParams;

    OMX_STRING cComponentName;

    OMX_VERSIONTYPE ComponentVersion;

    OMX_BUFFERHEADERTYPE *pInputBufHdrPending[G729ENC_MAX_NUM_OF_BUFS];

    OMX_BUFFERHEADERTYPE *pOutputBufHdrPending[G729ENC_MAX_NUM_OF_BUFS];

    OMX_BUFFERHEADERTYPE *iMMFDataLastBuffer;

    OMX_U8* pHoldBuffer;

    OMX_U8* iHoldBuffer;

    OMX_U32 nRuntimeInputBuffers;

    OMX_U32 nRuntimeOutputBuffers;

    TI_OMX_DSP_DEFINITION tiOmxDspDefinition;

    /** Flag to set when socket node stop callback should not transition
        component to OMX_StateIdle */
    OMX_U32 bNoIdleOnStop;

    /** Flag set when socket node is stopped */
    OMX_U32 bDspStoppedWhileExecuting;

    /** Number of outstanding FillBufferDone() calls */
    OMX_U32 nOutStandingFillDones;

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
#else
    OMX_Event AlloBuf_event;
    OMX_U8 AlloBuf_waitingsignal;
    
    OMX_Event InLoaded_event;
    OMX_U8 InLoaded_readytoidle;
    
    OMX_Event InIdle_event;
    OMX_U8 InIdle_goingtoloaded; 
#endif
#ifdef __PERF_INSTRUMENTATION__
    PERF_OBJHANDLE pPERF, pPERFcomp;
    OMX_U32 nLcml_nCntIp;         
    OMX_U32 nLcml_nCntOpReceived;
#endif

    /** Holds the value of RT Mixer mode  */ 
    OMX_U32 rtmx; 

    /** Flags to control port enable command **/
    OMX_U32 bEnableCommandPending;
    OMX_U32 bEnableCommandParam;

    OMX_U8 nUnhandledFillThisBuffers;
    OMX_U8 nUnhandledEmptyThisBuffers;
    OMX_BOOL bFlushOutputPortCommandPending;
    OMX_BOOL bFlushInputPortCommandPending;
    
    OMX_BOOL bLoadedCommandPending;
    OMX_PARAM_COMPONENTROLETYPE componentRole;
    OMX_STRING* sDeviceString;

    /** Keep buffer timestamps **/
    OMX_S64 arrTimestamp[G729ENC_MAX_NUM_OF_BUFS];

    /** Keep buffer timestamps **/
    OMX_S64 arrTickCount[G729ENC_MAX_NUM_OF_BUFS];

    /** Index to arrBufIndex[], used for input buffer timestamps */
    OMX_U8 IpBufindex;

    /** Index to arrBufIndex[], used for output buffer timestamps */
    OMX_U8 OpBufindex; 

    OMX_BOOL bPreempted;

    /** Pointer to RM callback **/
#ifdef RESOURCE_MANAGER_ENABLED
    RMPROXY_CALLBACKTYPE rmproxyCallback;
#endif
    
} G729ENC_COMPONENT_PRIVATE;


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
 *  @param hComp                    OMX Handle
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
 *  G729ENC_StartComponentThread()  Starts component thread
 *
 *
 *  @param hComp                               OMX Handle
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 *
 */
/* =================================================================================== */
OMX_ERRORTYPE G729ENC_StartComponentThread(OMX_HANDLETYPE pHandle);
/* =================================================================================== */
/**
 *  G729ENC_StopComponentThread()  Stops component thread
 *
 *
 *  @param hComp                                OMX Handle
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 *
 */
/* =================================================================================== */
OMX_ERRORTYPE G729ENC_StopComponentThread(OMX_HANDLETYPE pHandle);
/* =================================================================================== */
/**
 *  G729ENC_FreeCompResources()  Frees allocated memory
 *
 *
 *  @param hComp                                 OMX Handle
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 *
 */
/* =================================================================================== */
OMX_ERRORTYPE G729ENC_FreeCompResources(OMX_HANDLETYPE pComponent);
/* =================================================================================== */
/**
 *  G729ENC_GetCorrespondingLCMLHeader()  Returns LCML header
 * that corresponds to the given buffer
 *
 *  @param pComponentPrivate                     Component private data
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 */
/* =================================================================================== */
OMX_ERRORTYPE G729ENC_GetCorrespondingLCMLHeader(OMX_U8 *pBuffer,
                                                 OMX_DIRTYPE eDir,
                                                 G729ENC_LCML_BUFHEADERTYPE **ppLcmlHdr,
                                                 G729ENC_COMPONENT_PRIVATE* pComponentPrivate);
/* =================================================================================== */
/**
 *  G729ENC_LCMLCallback() Callback from LCML
 *
 *  @param event             Codec Event
 *
 *  @param args               Arguments from LCML
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 */
/* =================================================================================== */
OMX_ERRORTYPE G729ENC_LCMLCallback(TUsnCodecEvent event,
                                   void * args [10]);
/* =================================================================================== */
/**
 *  G729ENC_FillLCMLInitParams() Fills the parameters needed
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
OMX_ERRORTYPE G729ENC_FillLCMLInitParams(OMX_HANDLETYPE pHandle,
                                         LCML_DSP *plcml_Init,
                                         OMX_U16 arr[]);
/* =================================================================================== */
/**
 *  G729ENC_GetBufferDirection() Returns direction of pBufHeader
 *
 *  @param pBufHeader                      Buffer header
 *
 *  @param eDir                                Buffer direction
 *
 *  @param pComponentPrivate             Component private data
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 */
/* =================================================================================== */
OMX_ERRORTYPE G729ENC_GetBufferDirection(OMX_BUFFERHEADERTYPE *pBufHeader,
                                         OMX_DIRTYPE *eDir);
/* ===========================================================  */
/**
 *  G729ENC_HandleCommand()  Handles commands sent via SendCommand()
 *
 *  @param pComponentPrivate Component private data
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 */
/* =================================================================================== */
OMX_U32 G729ENC_HandleCommand(G729ENC_COMPONENT_PRIVATE *pComponentPrivate);
/* =================================================================================== */
/**
 *  G729ENC_HandleDataBufFromApp()  Handles data buffers received
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
OMX_ERRORTYPE G729ENC_HandleDataBufFromApp(OMX_BUFFERHEADERTYPE *pBufHeader,
                                           G729ENC_COMPONENT_PRIVATE *pComponentPrivate);
/* =================================================================================== */
/**
 *  G729ENC_HandleDataBufFromLCML()  Handles data buffers received
 *  from LCML
 *
 *  @param pComponentPrivate    Component private data
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 */
/* =================================================================================== */
OMX_ERRORTYPE G729ENC_HandleDataBufFromLCML(G729ENC_COMPONENT_PRIVATE* pComponentPrivate);

/* =================================================================================== */
/**
 *  G729ENC_GetLCMLHandle()  Get the handle to the LCML
 *
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 */
/* =================================================================================== */
OMX_HANDLETYPE G729ENC_GetLCMLHandle(G729ENC_COMPONENT_PRIVATE* pComponentPrivate);
/* =================================================================================== */
/**
 *  G729ENC_FreeLCMLHandle()  Frees the handle to the LCML
 *
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 */
/* =================================================================================== */
OMX_ERRORTYPE G729ENC_FreeLCMLHandle(G729ENC_COMPONENT_PRIVATE* pComponentPrivate);
/* =================================================================================== */
/**
 *  G729ENC_CleanupInitParams()  Starts component thread
 *
 *  @param pComponent             OMX Handle
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 */
/* =================================================================================== */
OMX_ERRORTYPE G729ENC_CleanupInitParams(OMX_HANDLETYPE pHandle);
/* =================================================================================== */
/**
 *  G729ENC_SetPending()  Called when the component queues a buffer
 * to the LCML
 *
 *  @param pComponentPrivate                     Component private data
 *
 *  @param pBufHdr                                   Buffer header
 *
 *  @param eDir                                         Direction of the buffer
 *
 *  @return None
 */
/* =================================================================================== */
void G729ENC_SetPending(G729ENC_COMPONENT_PRIVATE *pComponentPrivate,
                        OMX_BUFFERHEADERTYPE *pBufHdr,
                        OMX_DIRTYPE eDir,
                        OMX_U32 lineNumber);
/* =================================================================================== */
/**
 *  G729ENC_ClearPending()  Called when a buffer is returned
 * from the LCML
 *
 *  @param pComponentPrivate                     Component private data
 *
 *  @param pBufHdr                                   Buffer header
 *
 *  @param eDir                                         Direction of the buffer
 *
 *  @return None
 */
/* =================================================================================== */
void G729ENC_ClearPending(G729ENC_COMPONENT_PRIVATE *pComponentPrivate,
                          OMX_BUFFERHEADERTYPE *pBufHdr,
                          OMX_DIRTYPE eDir,
                          OMX_U32 lineNumber);
/* =================================================================================== */
/**
 *  G729ENC_IsPending()
 *
 *
 *  @param pComponentPrivate             Component private data
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 */
/* =================================================================================== */
OMX_U32 G729ENC_IsPending(G729ENC_COMPONENT_PRIVATE *pComponentPrivate,
                          OMX_BUFFERHEADERTYPE *pBufHdr,
                          OMX_DIRTYPE eDir);
/* =================================================================================== */
/**
 *  G729ENC_FillLCMLInitParamsEx()  Fills the parameters needed
 * to initialize the LCML without recreating the socket node
 *
 *  @param pComponent                     OMX Handle
 *
 *  @return None
 */
/* =================================================================================== */
OMX_ERRORTYPE G729ENC_FillLCMLInitParamsEx(OMX_HANDLETYPE pComponent);
/* =================================================================================== */
/**
 *  G729ENC_IsValid() Returns whether a buffer is valid
 *
 *
 *  @param pComponentPrivate                 Component private data
 *
 *  @param pBuffer                                Data buffer
 *
 *  @param eDir                                     Buffer direction
 *
 *  @return OMX_True = Valid
 *          OMX_False= Invalid
 */
/* =================================================================================== */
OMX_U32 G729ENC_IsValid(G729ENC_COMPONENT_PRIVATE *pComponentPrivate,
                        OMX_U8 *pBuffer,
                        OMX_DIRTYPE eDir);


/* ======================================================================= */
/** OMX_G729ENC_INDEXAUDIOTYPE  Defines the custom configuration settings
 *                              for the component
 *
 *  @param  OMX_IndexCustomG729ENCModeConfig      Sets the DASF mode
 *
 *  
 */
/*  ==================================================================== */
typedef enum OMX_G729ENC_INDEXAUDIOTYPE
{
    OMX_IndexCustomG729ENCModeConfig = 0xFF000001,
    OMX_IndexCustomG729ENCHeaderInfoConfig,
    OMX_IndexCustomG729ENCStreamIDConfig,
    OMX_IndexCustomG729ENCDataPath  
    
} OMX_G729ENC_INDEXAUDIOTYPE;


/* ===========================================================  */
/**
 *  G729ENC_TransitionToIdle() Transitions component to idle
 * 
 *
 *  @param pComponentPrivate     Component private data
 *
 *  @return OMX_ErrorNone = No error
 *          OMX Error code = Error
 */
/*================================================================== */
OMX_ERRORTYPE G729ENC_TransitionToIdle(G729ENC_COMPONENT_PRIVATE *pComponentPrivate);

#ifdef RESOURCE_MANAGER_ENABLED
/***********************************
 *  Callback to the RM                                       *
 ***********************************/
void G729ENC_ResourceManagerCallback(RMPROXY_COMMANDDATATYPE cbData);
#endif

#endif  /* OMX_G729ENC_UTILS__H */

