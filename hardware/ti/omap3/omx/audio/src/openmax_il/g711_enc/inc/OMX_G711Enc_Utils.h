
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
 * @file OMX_G711Enc_Utils.h
 *
 * This is an header file for an G711 Encoder that is fully
 * compliant with the OMX Audio specification 1.5.
 * This the file that the application that uses OMX would include in its code.
 *
 * @path $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\g711_enc\inc
 *
 * @rev 1.0
 */
/* --------------------------------------------------------------------------- */
/* ----------------------------------------------------------------------------
 *!
 *! Revision History
 *! ===================================
 *! 12-Dec-2006: Initial Version
 *! This is newest file
 * =========================================================================== */
#ifndef OMX_G711ENC_UTILS__H
#define OMX_G711ENC_UTILS__H
#include <OMX_Component.h>
#include <pthread.h>
#include "LCML_DspCodec.h"
#include <TIDspOmx.h>

#ifdef DSP_RENDERING_ON
#include <AudioManagerAPI.h>
#endif

#ifdef RESOURCE_MANAGER_ENABLED
#include <ResourceManagerProxyAPI.h>
#endif

#ifdef UNDER_CE
#define sleep Sleep
#endif
/* ======================================================================= */
/**
 * @def    G711ENC_DEBUG   Turns debug messaging on and off
 */
/* ======================================================================= */
#undef G711ENC_DEBUG
/* ======================================================================= */
/**
 * @def    G711ENC_MEMCHECK   Turns memory messaging on and off
 */
/* ======================================================================= */
#undef G711ENC_MEMCHECK
/* ======================================================================= */
/**
 * @def    G711ENC_PRINTS   Turns normal prints messaging on and off
 *implementation for substituing the normal printf
 */
/* ======================================================================= */
#undef G711ENC_PRINT
/*========================================================================*/

#ifndef UNDER_CE
/* ======================================================================= */
/**
 * @def    G711ENC_DEBUG   Debug print macro
 */
/* ======================================================================= */
#ifdef  G711ENC_DEBUG
#define G711ENC_DPRINT(...)    fprintf(stderr,__VA_ARGS__)
#else
#define G711ENC_DPRINT(...)
#endif
/* ======================================================================= */
/**
 * @def    G711ENC_PRINTS  print macro
 */
/* ======================================================================= */
#ifdef  G711ENC_PRINT
#define G711ENC_PRINT(...)    fprintf(stderr,__VA_ARGS__)
#else
#define G711ENC_PRINT(...)
#endif

/* ======================================================================= */
/**
 * @def    G711ENC_MEMCHECK   Memory print macro
 */
/* ======================================================================= */
#ifdef  G711ENC_MEMCHECK
#define G711ENC_MEMPRINT(...)    fprintf(stderr,__VA_ARGS__)
#else
#define G711ENC_MEMPRINT(...)
#endif

#else   /*UNDER_CE*/
/* ======================================================================= */
/**
 * @def    G711ENC_DEBUG   Debug print macro
 */
/* ======================================================================= */
#ifdef  G711ENC_DEBUG
#define G711ENC_DPRINT(STR, ARG...) printf()
#else

#endif
/* ======================================================================= */
/**
 * @def    G711ENC_MEMCHECK   Memory print macro
 */
/* ======================================================================= */
#ifdef  G711ENC_MEMCHECK
#define G711ENC_MEMPRINT(STR, ARG...) printf()
#else

#endif

#ifdef UNDER_CE

#ifdef DEBUG
#define G711ENC_DPRINT     printf
#define G711ENC_MEMPRINT   printf
#else
#define G711ENC_DPRINT
#define G711ENC_MEMPRINT
#endif

#endif  /*UNDER_CE*/

#endif

/* ======================================================================= */
/**
 *  M A C R O S FOR MALLOC and MEMORY FREE and CLOSING PIPES
 */
/* ======================================================================= */

#define OMX_G711ENC_INIT_STRUCT(_s_, _name_)    \
    memset((_s_), 0x0, sizeof(_name_));         \
    (_s_)->nSize = sizeof(_name_);              \
    (_s_)->nVersion.s.nVersionMajor = 0x1;      \
    (_s_)->nVersion.s.nVersionMinor = 0x0;      \
    (_s_)->nVersion.s.nRevision = 0x0;          \
    (_s_)->nVersion.s.nStep = 0x0

#define OMX_G711ENC_MEMFREE_STRUCT(_pStruct_)                   \
    if(_pStruct_ != NULL){                                      \
    G711ENC_MEMPRINT("%d :: [FREE] %p\n",__LINE__,_pStruct_);   \
        free(_pStruct_);                                        \
        _pStruct_ = NULL;                                       \
    }

#define OMX_G711ENC_CLOSE_PIPE(_pStruct_,err)                   \
    G711ENC_DPRINT("%d :: CLOSING PIPE \n",__LINE__);           \
    err = close (_pStruct_);                                    \
    if(0 != err && OMX_ErrorNone == eError){                    \
        eError = OMX_ErrorHardware;                             \
        printf("%d :: Error while closing pipe\n",__LINE__);    \
        goto EXIT;                                              \
    }

#define G711ENC_OMX_MALLOC_STRUCT(_pStruct_, _sName_)           \
    _pStruct_ = (_sName_*)malloc(sizeof(_sName_));              \
    if(_pStruct_ == NULL){                                      \
        printf("***********************************\n");        \
        printf("%d :: Malloc Failed\n",__LINE__);               \
        printf("***********************************\n");        \
        eError = OMX_ErrorInsufficientResources;                \
        goto EXIT;                                              \
    }                                                           \
    memset(_pStruct_,0,sizeof(_sName_));                        \
    G711ENC_MEMPRINT("%d :: [ALLOC] %p\n",__LINE__,_pStruct_);

#define G711ENC_OMX_MALLOC_SIZE(_ptr_, _size_,_name_)           \
    _ptr_ = (_name_ *)malloc(_size_);                           \
    if(_ptr_ == NULL){                                          \
        printf("***********************************\n");        \
        printf("%d :: Malloc Failed\n",__LINE__);               \
        printf("***********************************\n");        \
        eError = OMX_ErrorInsufficientResources;                \
        goto EXIT;                                              \
    }                                                           \
    memset(_ptr_,0,_size_);                                     \
    G711ENC_MEMPRINT("%d :: Malloced = %p\n",__LINE__,_ptr_);
  
/* ======================================================================= */
/**
 * @def G711ENC_NUM_INPUT_BUFFERS   Default number of input buffers
 */
/* ======================================================================= */
#define G711ENC_NUM_INPUT_BUFFERS 1
/* ======================================================================= */
/**
 * @def G711ENC_NUM_INPUT_BUFFERS_DASF  Default No.of input buffers DASF
 */
/* ======================================================================= */
#define G711ENC_NUM_INPUT_BUFFERS_DASF 2
/* ======================================================================= */
/**
 * @def G711ENC_NUM_OUTPUT_BUFFERS   Default number of output buffers
 */
/* ======================================================================= */
#define G711ENC_NUM_OUTPUT_BUFFERS 1
/* ======================================================================= */
/**
 * @def G711ENC_INPUT_BUFFER_SIZE_DASF  Default input buffer size DASF
 */
/* ======================================================================= */
#define G711ENC_INPUT_BUFFER_SIZE_DASF 160 /*80*/
/* ======================================================================= */
/**
 * @def G711ENC_OUTPUT_BUFFER_SIZE   Default output buffer size
 */
/* ======================================================================= */
#define G711ENC_OUTPUT_BUFFER_SIZE 80
/* ======================================================================= */
/**
 * @def G711ENC_INPUT_FRAME_SIZE   Default input buffer size
 */
/* ======================================================================= */
#define G711ENC_INPUT_FRAME_SIZE 160
#define G711ENC_INPUT_FRAME_SIZE_20MS 320
#define G711ENC_INPUT_FRAME_SIZE_30MS 480
/* ======================================================================= */
/**
 * @def G711ENC_OUTPUT_FRAME_SIZE   Default output buffer size
 */
/* ======================================================================= */
#define G711ENC_OUTPUT_FRAME_SIZE 80
#define G711ENC_OUTPUT_FRAME_SIZE_20MS 160
#define G711ENC_OUTPUT_FRAME_SIZE_30MS 240
/* ======================================================================= */
/**
 * @def G711ENC_APP_ID  App ID Value setting
 */
/* ======================================================================= */
#define G711ENC_APP_ID 100

/* ======================================================================= */
/**
 * @def    G711ENC_SAMPLING_FREQUENCY   Sampling frequency
 */
/* ======================================================================= */
#define G711ENC_SAMPLING_FREQUENCY 8000
/* ======================================================================= */
/**
 * @def    G711ENC_MAX_NUM_OF_BUFS   Maximum number of buffers
 */
/* ======================================================================= */
#define G711ENC_MAX_NUM_OF_BUFS 10
/* ======================================================================= */
/**
 * @def    G711ENC_NUM_OF_PORTS   Number of ports
 */
/* ======================================================================= */
#define G711ENC_NUM_OF_PORTS 2
/* ======================================================================= */
/**
 * @def    G711ENC_XXX_VER    Component version
 */
/* ======================================================================= */
#define G711ENC_MAJOR_VER 0xF1
#define G711ENC_MINOR_VER 0xF2
/* ======================================================================= */
/**
 * @def    G711ENC_NOT_USED    Defines a value for "don't care" parameters
 */
/* ======================================================================= */
#define G711ENC_NOT_USED 10
/* ======================================================================= */
/**
 * @def    G711ENC_NORMAL_BUFFER  Defines flag value with all flags off
 */
/* ======================================================================= */
#define G711ENC_NORMAL_BUFFER 0
/* ======================================================================= */
/**
 * @def    OMX_G711ENC_DEFAULT_SEGMENT    Default segment ID for the LCML
 */
/* ======================================================================= */
#define G711ENC_DEFAULT_SEGMENT (0)
/* ======================================================================= */
/**
 * @def    OMX_G711ENC_SN_TIMEOUT    Timeout value for the socket node
 */
/* ======================================================================= */
#define G711ENC_SN_TIMEOUT (-1)
/* ======================================================================= */
/**
 * @def    OMX_G711ENC_SN_PRIORITY   Priority for the socket node
 */
/* ======================================================================= */
#define G711ENC_SN_PRIORITY (10)
/* ======================================================================= */
/**
 * @def    G711ENC_CPU   TBD, 50MHz for the moment
 */
/* ======================================================================= */
#define G711ENC_CPU (50)
/* ======================================================================= */
/**
 * @def    OMX_G711ENC_NUM_DLLS   number of DLL's
 */
/* ======================================================================= */
#define G711ENC_NUM_DLLS (2)
/* ======================================================================= */
/**
 * @def    OMX_G711ENC_NUM_DLLS   number of DLL's
 */
/* ======================================================================= */
#define G711ENC_EXIT_COMPONENT_THRD  10
/* ======================================================================= */
/**
 * @def    DSP cache alignment number of bytes
 */
/* ======================================================================= */
#define DSP_CACHE_ALIGNMENT  256
/* ======================================================================= */
/**
 * @def    Extra buffer bytes used for DSP alignment
 */
/* ======================================================================= */
#define EXTRA_BYTES  128
/* ======================================================================= */
/**
 * @def    G711ENC_USN_DLL_NAME   USN DLL name
 */
/* ======================================================================= */
#ifdef UNDER_CE
#define G711ENC_USN_DLL_NAME "\\windows\\usn.dll64P"
#else
#define G711ENC_USN_DLL_NAME "usn.dll64P"
#endif

/* ======================================================================= */
/**
 * @def    G711ENC_DLL_NAME   G711 Encoder socket node dll name
 */
/* ======================================================================= */
#ifdef UNDER_CE
#define G711ENC_DLL_NAME "\\windows\\g711enc_sn.dll64P"
#else
#define G711ENC_DLL_NAME "g711enc_sn.dll64P"
#endif

/* ======================================================================= */
/** G711ENC_StreamType  Stream types
 *
 *  @param  G711ENC_DMM         DMM
 *
 *  @param  G711ENC_INSTRM        Input stream
 *
 *  @param  G711ENC_OUTSTRM       Output stream
 */
/* ======================================================================= */
enum G711ENC_StreamType {
    G711ENC_DMM = 0,
    G711ENC_INSTRM,
    G711ENC_OUTSTRM
};

/* ======================================================================= */
/**
 * @def G711ENC_TIMEOUTS Default timeout used to come out of blocking calls
 *@G711ENC_TIMEOUTS refered to time out in seconds
 *@G711ENC_TIMEOUTNS refered to time out in nano-seconds
 */
/* ======================================================================= */
#define G711ENC_TIMEOUTS 1
#define G711ENC_TIMEOUTNS 0
/* ======================================================================= */
/*
 * @def G711ENC_OMX_MAX_TIMEOUTS   Max Time Outs
 * @def G711ENC_DONT_CARE       Dont Care Condition
 * @def G711ENC_NUM_CHANNELS    Number of Channels
 * @def G711ENC_APP_ID      App ID Value setting
 */
/* ======================================================================= */
#define G711ENC_OMX_MAX_TIMEOUTS 20
#define G711ENC_DONT_CARE 0
#define G711ENC_NUM_CHANNELS 1
/* ======================================================================= */
/**
 * @def    G711ENC_STREAM_COUNT    Number of streams
 *       G711ENC_INPUT_STREAM_ID Stream ID for Input Buffer
 */
/* ======================================================================= */
#define G711ENC_STREAM_COUNT 2
#define G711ENC_INPUT_STREAM_ID 0

#define G711ENC_EXIT_COMPONENT_THRD  10

/* =================================================================================== */
/** 
 * 
 */
/* ================================================================================== */
typedef struct G711ENC_FTYPES{
    OMX_S16   FrameSizeType;
    OMX_S16   VAUMode;
    OMX_S16   VAUThresOffset;
    OMX_S16   VAUNum;
    OMX_S16   NMUNoise;
    OMX_S16   LPOrder;
}G711ENC_FTYPES;

/* ======================================================================= */
/** G711ENC_COMP_PORT_TYPE  Port types
 *
 *  @param  G711ENC_INPUT_PORT        Input port
 *
 *  @param  G711ENC_OUTPUT_PORT     Output port
 */
/*  ====================================================================== */
/*This enum must not be changed. */
typedef enum G711ENC_COMP_PORT_TYPE {
    G711ENC_INPUT_PORT = 0,
    G711ENC_OUTPUT_PORT
}G711ENC_COMP_PORT_TYPE;

/* ======================================================================= */
/** G711ENC_BUFFER_Dir  Buffer Direction
 *
 *  @param  G711ENC_DIRECTION_INPUT   Input direction
 *
 *  @param  G711ENC_DIRECTION_OUTPUT  Output direction
 *
 */
/* ======================================================================= */
typedef enum {
    G711ENC_DIRECTION_INPUT,
    G711ENC_DIRECTION_OUTPUT
}G711ENC_BUFFER_Dir;

/* ======================================================================= */
/** G711ENC_BUFFS  Buffer details
 *
 *  @param  BufHeader Buffer header
 *
 *  @param  Buffer  Buffer
 *
 */
/* ======================================================================= */
typedef struct G711ENC_BUFFS {
    char BufHeader;
    char Buffer;
}G711ENC_BUFFS;

/* ======================================================================= */
/** G711ENC_BUFFERHEADERTYPE_INFO
 *
 *  @param  pBufHeader
 *
 *  @param  bBufOwner
 *
 */
/* ======================================================================= */
typedef struct G711ENC_BUFFERHEADERTYPE_INFO {
    OMX_BUFFERHEADERTYPE* pBufHeader[G711ENC_MAX_NUM_OF_BUFS];
    G711ENC_BUFFS bBufOwner[G711ENC_MAX_NUM_OF_BUFS];
}G711ENC_BUFFERHEADERTYPE_INFO;


typedef OMX_ERRORTYPE (*G711ENC_fpo)(OMX_HANDLETYPE);

/* =================================================================================== */
/**
 * Socket node Audio Codec Configuration parameters.
 */
/* =================================================================================== */
typedef struct G711ENC_AudioCodecParams {
    unsigned long  iSamplingRate;
    unsigned long  iStrmId;
    unsigned short iAudioFormat;
}G711ENC_AudioCodecParams;

/* =================================================================================== */
/**
 * G711ENC_TALGCtrl         Socket Node Alg Control parameters.
 * G711ENC_UAlgInBufParamStruct   Input Buffer Param Structure
 * G711ENC_UAlgOutBufParamStruct  Output Buffer Param Structure
 */
/* =================================================================================== */
/* Algorithm specific command parameters */
typedef struct {
    unsigned int iSize;
    unsigned int iBitrate;
    unsigned int iDTX;
    unsigned int iMode;
    unsigned int iFrameSize;
    unsigned int iNoiseSuppressionMode;
    unsigned int ittyTddMode;
    unsigned int idtmfMode;
    unsigned int idataTransmit;
}G711ENC_TALGCtrl;

/* =================================================================================== */
/**
 * G711ENC_FrameStruct          Input Frame Structure
 * bLastBuffer            To Send Last Buufer Flag
 *frameType             Tio save the frame type
 */
/* =================================================================================== */
typedef struct G711ENC_FrameStruct{
    unsigned long   usLastFrame;
    unsigned long frameType; /* 0: voice frame (80 bytes), 1: SID frame (22 bytes), 2: No Data (0 bytes), 3: Frame lost */ 
} G711ENC_FrameStruct;  
/* =================================================================================== */
/**
 * G711ENC_ParamStruct    Input Buffer Param Structure
 * usEndOfFile        To Send Last Buufer Flag
 */
/* =================================================================================== */
typedef struct G711ENC_ParamStruct
{
    /*unsigned long usEndOfFile;*/
    unsigned long int usNbFrames;
    G711ENC_FrameStruct *pParamElem;  
} G711ENC_ParamStruct;
/* =================================================================================== */
/**
 * G711ENC_UAlgOutBufParamStruct  Output Buffer Param Structure
 * ulFrameCount No.of Frames Encoded
 */
/* =================================================================================== */
typedef struct {
    unsigned long ulFrameCount;
}G711ENC_UAlgOutBufParamStruct;

/* =================================================================================== */
/**
 * G711ENC_LCML_BUFHEADERTYPE Buffer Header Type
 */
/* =================================================================================== */
typedef struct G711ENC_LCML_BUFHEADERTYPE {
    G711ENC_BUFFER_Dir eDir;
    G711ENC_ParamStruct *pIpParam;
    G711ENC_UAlgOutBufParamStruct *pOpParam;
    OMX_BUFFERHEADERTYPE* buffer;
    G711ENC_FrameStruct *pFrameParam;
    G711ENC_ParamStruct *pBufferParam;
    DMM_BUFFER_OBJ* pDmmBuf;
}G711ENC_LCML_BUFHEADERTYPE;

typedef struct _G711ENC_BUFFERLIST G711ENC_BUFFERLIST;

/* =================================================================================== */
/**
 * _G711ENC_BUFFERLIST Structure for buffer list
 */
/* ================================================================================== */
struct _G711ENC_BUFFERLIST{
    OMX_BUFFERHEADERTYPE sBufHdr;
    OMX_BUFFERHEADERTYPE *pBufHdr[G711ENC_MAX_NUM_OF_BUFS];
    OMX_U32 bufferOwner[G711ENC_MAX_NUM_OF_BUFS];
    OMX_U32 bBufferPending[G711ENC_MAX_NUM_OF_BUFS];
    OMX_U32 numBuffers;
    G711ENC_BUFFERLIST *pNextBuf;
    G711ENC_BUFFERLIST *pPrevBuf;
};
/* =================================================================================== */
/**
 * G711ENC_BUFDATA
 */
/* =================================================================================== */
typedef struct G711ENC_BUFDATA {
    OMX_U8 nFrames;
}G711ENC_BUFDATA;

/* =================================================================================== */
/**
 * G711ENC_PORT_TYPE Structure for PortFormat details
 */
/* =================================================================================== */
typedef struct G711ENC_PORT_TYPE {
    OMX_HANDLETYPE hTunnelComponent;
    OMX_U32 nTunnelPort;
    OMX_BUFFERSUPPLIERTYPE eSupplierSetting;
    OMX_U8 nBufferCnt;
    OMX_AUDIO_PARAM_PORTFORMATTYPE* pPortFormat;
} G711ENC_PORT_TYPE;

OMX_ERRORTYPE OMX_DmmMap(DSP_HPROCESSOR ProcHandle, int size, 
                         void* pArmPtr, DMM_BUFFER_OBJ* pDmmBuf);
OMX_ERRORTYPE OMX_DmmUnMap(DSP_HPROCESSOR ProcHandle, void* pMapPtr, void* pResPtr);  
/* =================================================================================== */
/**
 * G711ENC_COMPONENT_PRIVATE Component private data Structure
 */
/* =================================================================================== */
typedef struct G711ENC_COMPONENT_PRIVATE
{
    /** Array of pointers to BUFFERHEADERTYPE structues
        This pBufHeader[INPUT_PORT] will point to all the
        BUFFERHEADERTYPE structures related to input port,
        not just one structure. Same is the case for output
        port also. */
    OMX_BUFFERHEADERTYPE* pBufHeader[G711ENC_NUM_OF_PORTS];
    OMX_CALLBACKTYPE cbInfo;
    OMX_PORT_PARAM_TYPE* sPortParam;
    OMX_PRIORITYMGMTTYPE* sPriorityMgmt;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef[G711ENC_NUM_OF_PORTS];
    OMX_PORT_PARAM_TYPE* pPortParamType;
    OMX_AUDIO_PARAM_PCMMODETYPE* G711Params[G711ENC_NUM_OF_PORTS];
    G711ENC_BUFFERHEADERTYPE_INFO BufInfo[G711ENC_NUM_OF_PORTS];
    G711ENC_PORT_TYPE *pCompPort[G711ENC_NUM_OF_PORTS];
    G711ENC_LCML_BUFHEADERTYPE *pLcmlBufHeader[G711ENC_NUM_OF_PORTS];
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

    OMX_U32 dasfMode;

    OMX_U32 acdnMode;

    OMX_U32 fdwrite;

    OMX_U32 fdread;

    OMX_U32 bLcmlHandleOpened;

    /** Set to indicate component is stopping */
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

    OMX_U32 streamID;

    OMX_U32 bPortDefsAllocated;

    OMX_U32 bCompThreadStarted;

    OMX_U32 bBypassDSP;

    OMX_U32 nVersion;

    OMX_U32 nFillThisBufferCount;

    OMX_U32 nFillBufferDoneCount;

    OMX_U32 nEmptyThisBufferCount;

    OMX_U32 nEmptyBufferDoneCount;

    OMX_U32 bInitParamsInitialized;

    OMX_U32 bIdleCommandPending;

    OMX_U32 nNumInputBufPending;

    OMX_U32 nNumOutputBufPending;

    OMX_U32 nInvalidFrameCount;

    OMX_U32 bDisableCommandPending;
  
    OMX_U32 bDisableCommandParam;
  
    OMX_U32 bEnableCommandParam;
  
    /** Flag for frame size type mode */
    OMX_S16 fsizemode;
    /** Flag for frame type  */
    OMX_S16 frametype;
    /** Flag for VAU mode */
    OMX_S16 vaumode;
    /** Flag for VAU Threshold offset */
    OMX_S16 vauthreshold;
    /** Flag for VAU number of ms   */
    OMX_S16 vaunumber;
    /** Flag for NMU TX Noise Mode */
    OMX_S16 nmunoise;
    /** Flag for Linear Prediction order*/
    OMX_S16 lporder;
  
    /**number of frames that has been sent*/
    OMX_U8 nNumOfFramesSent;

    /** Buffers unhandled during fill this buffer call*/
    OMX_S8 nUnhandledFillThisBuffers;
    OMX_S8 nUnhandledEmptyThisBuffers;
    OMX_BOOL bFlushInputPortCommandPending;
    OMX_BOOL bFlushOutputPortCommandPending;

    /** Set to indicate component is stopping */
    OMX_U32 bIsStopping;
    OMX_S8 ProcessingInputBuf;
    OMX_S8 ProcessingOutputBuf;
  
    /** Flag set when a disable command is pending */
    OMX_U32 bEnableCommandPending;

    /** Parameter for pending disable command */
    OMX_U32 nEnableCommandParam;

    OMX_HANDLETYPE pLcmlHandle;

    OMX_PTR pMarkData;

    OMX_MARKTYPE *pMarkBuf;

    OMX_HANDLETYPE hMarkTargetComponent;

    G711ENC_BUFFERLIST *pInputBufferList;

    G711ENC_BUFFERLIST *pOutputBufferList;

    LCML_STRMATTR *strmAttr;

    G711ENC_TALGCtrl *pAlgParam;

    G711ENC_AudioCodecParams *pParams;

    OMX_STRING cComponentName;

    OMX_VERSIONTYPE ComponentVersion;

    OMX_BUFFERHEADERTYPE *pInputBufHdrPending[G711ENC_MAX_NUM_OF_BUFS];

    OMX_BUFFERHEADERTYPE *pOutputBufHdrPending[G711ENC_MAX_NUM_OF_BUFS];

    TI_OMX_DSP_DEFINITION tiOmxDspDefinition;

    /** Flag to set when socket node stop callback should not transition
        component to OMX_StateIdle */
    OMX_U32 bNoIdleOnStop;

    /** Flag set when socket node is stopped */
    OMX_U32 bDspStoppedWhileExecuting;

    /** Number of outstanding FillBufferDone() calls */
    OMX_U32 nOutStandingFillDones;

    OMX_BOOL bLoadedCommandPending;

    OMX_PARAM_COMPONENTROLETYPE componentRole;

    OMX_STRING* sDeviceString;

    /* backup pointer for LCML */
    void* ptrLibLCML;

    OMX_U32 bIsThreadstop;
    
    /**Keep buffer tickcount*/
    OMX_U32 arrBufIndexTick[G711ENC_MAX_NUM_OF_BUFS]; 
    
    /** Keep buffer timestamps **/
    OMX_S64 arrBufIndex[G711ENC_MAX_NUM_OF_BUFS];

    /** Index to arrBufIndex[], used for input buffer timestamps */
    OMX_U8 IpBufindex;

    /** Index to arrBufIndex[], used for output buffer timestamps */
    OMX_U8 OpBufindex;

    /** Number of input buffers at runtime **/
    OMX_U32 nRuntimeInputBuffers;

    /** Number of output buffers at runtime **/
    OMX_U32 nRuntimeOutputBuffers;



#ifndef UNDER_CE
    pthread_mutex_t AlloBuf_mutex;    
    pthread_cond_t AlloBuf_threshold;
    OMX_U8 AlloBuf_waitingsignal;

    pthread_mutex_t InIdle_mutex;
    pthread_cond_t InIdle_threshold;
    OMX_U8 InIdle_goingtoloaded;

    pthread_mutex_t InLoaded_mutex;
    pthread_cond_t InLoaded_threshold;
    OMX_U8 InLoaded_readytoidle;
#endif

    OMX_BOOL bPreempted;

    /** Pointer to RM callback **/
#ifdef RESOURCE_MANAGER_ENABLED
    RMPROXY_CALLBACKTYPE rmproxyCallback;
#endif

} G711ENC_COMPONENT_PRIVATE;


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
 *  @param hComp      OMX Handle
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
 *  G711ENC_StartComponentThread()  Starts component thread
 *
 *
 *  @param hComp      OMX Handle
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 *
 */
/* =================================================================================== */
OMX_ERRORTYPE G711ENC_StartComponentThread(OMX_HANDLETYPE pHandle);
/* =================================================================================== */
/**
 *  G711ENC_StopComponentThread()  Stops component thread
 *
 *
 *  @param hComp      OMX Handle
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 *
 */
/* =================================================================================== */
OMX_ERRORTYPE G711ENC_StopComponentThread(OMX_HANDLETYPE pHandle);
/* =================================================================================== */
/**
 *  G711ENC_FreeCompResources()  Frees allocated memory
 *
 *
 *  @param hComp      OMX Handle
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 *
 */
/* =================================================================================== */
OMX_ERRORTYPE G711ENC_FreeCompResources(OMX_HANDLETYPE pComponent);
/* =================================================================================== */
/**
 *  G711ENC_GetCorrespondingLCMLHeader()  Returns LCML header
 * that corresponds to the given buffer
 *
 *  @param pComponentPrivate  Component private data
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 */
/* =================================================================================== */
OMX_ERRORTYPE G711ENC_GetCorrespondingLCMLHeader(G711ENC_COMPONENT_PRIVATE *pComponentPrivate,
                                                 OMX_U8 *pBuffer,
                                                 OMX_DIRTYPE eDir,
                                                 G711ENC_LCML_BUFHEADERTYPE **ppLcmlHdr);
/* =================================================================================== */
/**
 *  G711ENC_LCMLCallback() Callback from LCML
 *
 *  @param event    Codec Event
 *
 *  @param args   Arguments from LCML
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 */
/* =================================================================================== */
OMX_ERRORTYPE G711ENC_LCMLCallback(TUsnCodecEvent event,
                                   void * args [10]);
/* =================================================================================== */
/**
 *  G711ENC_FillLCMLInitParams() Fills the parameters needed
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
OMX_ERRORTYPE G711ENC_FillLCMLInitParams(OMX_HANDLETYPE pHandle,
                                         LCML_DSP *plcml_Init,
                                         OMX_U16 arr[]);
/* =================================================================================== */
/**
 *  G711ENC_GetBufferDirection() Returns direction of pBufHeader
 *
 *  @param pBufHeader   Buffer header
 *
 *  @param eDir       Buffer direction
 *
 *  @param pComponentPrivate  Component private data
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 */
/* =================================================================================== */
OMX_ERRORTYPE G711ENC_GetBufferDirection(OMX_BUFFERHEADERTYPE *pBufHeader,
                                         OMX_DIRTYPE *eDir);
/* ===========================================================  */
/**
 *  G711ENC_HandleCommand()  Handles commands sent via SendCommand()
 *
 *  @param pComponentPrivate  Component private data
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 */
/* =================================================================================== */
OMX_U32 G711ENC_HandleCommand(G711ENC_COMPONENT_PRIVATE *pComponentPrivate);
/* =================================================================================== */
/**
 *  G711ENC_HandleDataBufFromApp()  Handles data buffers received
 * from the IL Client
 *
 *  @param pComponentPrivate  Component private data
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 */
/* =================================================================================== */
OMX_ERRORTYPE G711ENC_HandleDataBufFromApp(OMX_BUFFERHEADERTYPE *pBufHeader,
                                           G711ENC_COMPONENT_PRIVATE *pComponentPrivate);
/* =================================================================================== */
/**
 *  G711ENC_HandleDataBufFromLCML()  Handles data buffers received
 *  from LCML
 *
 *  @param pComponentPrivate  Component private data
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 */
/* =================================================================================== */
OMX_ERRORTYPE G711ENC_HandleDataBufFromLCML(G711ENC_COMPONENT_PRIVATE* pComponentPrivate, G711ENC_LCML_BUFHEADERTYPE* msgBuffer);

/* =================================================================================== */
/**
 *  G711ENC_GetLCMLHandle()  Get the handle to the LCML
 *
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 */
/* =================================================================================== */
OMX_HANDLETYPE G711ENC_GetLCMLHandle(G711ENC_COMPONENT_PRIVATE *pComponentPrivate);
/* =================================================================================== */
/**
 *  G711ENC_FreeLCMLHandle()  Frees the handle to the LCML
 *
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 */
/* =================================================================================== */
OMX_ERRORTYPE G711ENC_FreeLCMLHandle();
/* =================================================================================== */
/**
 *  G711ENC_CleanupInitParams()  Starts component thread
 *
 *  @param pComponent   OMX Handle
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 */
/* =================================================================================== */
OMX_ERRORTYPE G711ENC_CleanupInitParams(OMX_HANDLETYPE pHandle);
/* =================================================================================== */
/**
 *  G711ENC_SetPending()  Called when the component queues a buffer
 * to the LCML
 *
 *  @param pComponentPrivate    Component private data
 *
 *  @param pBufHdr        Buffer header
 *
 *  @param eDir         Direction of the buffer
 *
 *  @return None
 */
/* =================================================================================== */
void G711ENC_SetPending(G711ENC_COMPONENT_PRIVATE *pComponentPrivate,
                        OMX_BUFFERHEADERTYPE *pBufHdr,
                        OMX_DIRTYPE eDir,
                        OMX_U32 lineNumber);
/* =================================================================================== */
/**
 *  G711ENC_ClearPending()  Called when a buffer is returned
 * from the LCML
 *
 *  @param pComponentPrivate    Component private data
 *
 *  @param pBufHdr        Buffer header
 *
 *  @param eDir         Direction of the buffer
 *
 *  @return None
 */
/* =================================================================================== */
void G711ENC_ClearPending(G711ENC_COMPONENT_PRIVATE *pComponentPrivate,
                          OMX_BUFFERHEADERTYPE *pBufHdr,
                          OMX_DIRTYPE eDir,
                          OMX_U32 lineNumber);
/* =================================================================================== */
/**
 *  G711ENC_IsPending()
 *
 *
 *  @param pComponentPrivate    Component private data
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 */
/* =================================================================================== */
OMX_U32 G711ENC_IsPending(G711ENC_COMPONENT_PRIVATE *pComponentPrivate,
                          OMX_BUFFERHEADERTYPE *pBufHdr,
                          OMX_DIRTYPE eDir);
/* =================================================================================== */
/**
 *  G711ENC_FillLCMLInitParamsEx()  Fills the parameters needed
 * to initialize the LCML without recreating the socket node
 *
 *  @param pComponent     OMX Handle
 *
 *  @return None
 */
/* =================================================================================== */
OMX_ERRORTYPE G711ENC_FillLCMLInitParamsEx(OMX_HANDLETYPE pComponent);
/* =================================================================================== */
/**
 *  G711ENC_IsValid() Returns whether a buffer is valid
 *
 *
 *  @param pComponentPrivate    Component private data
 *
 *  @param pBuffer        Data buffer
 *
 *  @param eDir         Buffer direction
 *
 *  @return OMX_True = Valid
 *          OMX_False= Invalid
 */
/* =================================================================================== */
OMX_U32 G711ENC_IsValid(G711ENC_COMPONENT_PRIVATE *pComponentPrivate,
                        OMX_U8 *pBuffer,
                        OMX_DIRTYPE eDir);


/* ======================================================================= */
/** OMX_G711ENC_INDEXAUDIOTYPE  Defines the custom configuration settings
 *                              for the component
 *
 *  @param  OMX_IndexCustomG711ENCModeConfig      Sets the DASF mode
 *
 *  
 */
/*  ==================================================================== */
typedef enum OMX_G711ENC_INDEXAUDIOTYPE {
    OMX_IndexCustomG711ENCModeConfig = 0xFF000001,
    OMX_IndexCustomG711EncFrameParams,
    OMX_IndexCustomG711EncDataPath  
}OMX_G711ENC_INDEXAUDIOTYPE;

/* ===========================================================  */
/**
 *  G711ENC_CompThread() Component Thread call
 * 
 *
 *  @param pComponentPrivate     Component private data
 *
 *  @return void
 */
/*================================================================== */
void* G711ENC_CompThread(void* pThreadData);

#ifdef RESOURCE_MANAGER_ENABLED
/***********************************
 *  Callback to the RM                                       *
 ***********************************/
void G711ENC_ResourceManagerCallback(RMPROXY_COMMANDDATATYPE cbData);
#endif

#endif  /* OMX_G711ENC_UTILS__H */
