
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
* @file OMX_WbAmrDecoder.h
*
* This is an header file for an audio PCM decoder that is fully
* compliant with the OMX Audio specification.
* This the file that the application that uses OMX would include
* in its code.
*
* @path $(CSLPATH)\
*
* @rev 0.1
*/
/* --------------------------------------------------------------------------- */

#ifndef OMX_WBAMR_DECODER_H
#define OMX_WBAMR_DECODER_H

#include "LCML_DspCodec.h"
#include <OMX_Component.h>
#include <pthread.h>
#include <OMX_TI_Debug.h>

#ifdef __PERF_INSTRUMENTATION__
    #include "perf.h"
#endif

#ifndef UNDER_CE
#ifdef DSP_RENDERING_ON
#include <AudioManagerAPI.h>
#endif
 
#ifdef RESOURCE_MANAGER_ENABLED
#include <ResourceManagerProxyAPI.h>
#endif
#endif

#ifndef ANDROID
    #define ANDROID
#endif

#ifdef ANDROID
    #undef LOG_TAG
    #define LOG_TAG "OMX_WBAMRDEC"

/* PV opencore capability custom parameter index */
    #define PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX 0xFF7A347
#endif

/* =======================================================================
 *
 ** Default timeout used to come out of blocking calls*
 *
 *
 ======================================================================= */
#define WBAMR_DEC_TIMEOUT (1000) /* millisecs */

/* ======================================================================= */
/**
 * @def    NUM_WMADEC_INPUT_BUFFERS   Default number of input buffers
 *
 */
/* ======================================================================= */
#define NUM_WBAMRDEC_INPUT_BUFFERS 1
/* ======================================================================= */
/**
 * @def    NUM_WMADEC_OUTPUT_BUFFERS   Default number of output buffers
 *
 */
/* ======================================================================= */
#define NUM_WBAMRDEC_OUTPUT_BUFFERS 2
/* ======================================================================= */
/**
 * @def    NUM_WMADEC_OUTPUT_BUFFERS   Default number of output buffers DASF
 *
 */
/* ======================================================================= */
#define NUM_WBAMRDEC_OUTPUT_BUFFERS_DASF 2
/* ======================================================================= */
/**
 * @def    INPUT_WBAMRDEC_BUFFER_SIZE   Default input buffer size
 *
 */
/* ======================================================================= */
#define INPUT_WBAMRDEC_BUFFER_SIZE 116
/* ======================================================================= */
/**
 * @def    OUTPUT_WBAMRDEC_BUFFER_SIZE   Default output buffer size
 *
 */
/* ======================================================================= */
#define OUTPUT_WBAMRDEC_BUFFER_SIZE 640
/* ======================================================================= */
/**
 * @def    INPUT_WBAMRDEC_BUFFER_SIZE_MIME   Default input buffer size MIME
 *
 */
/* ======================================================================= */
#define INPUT_WBAMRDEC_BUFFER_SIZE_MIME 61

/* ======================================================================= */
/**
 * @def    WBAMR_DEC_STREAM_COUNT   Number of streams
 */
/* ======================================================================= */
#define WBAMR_DEC_STREAM_COUNT 2
#define WBAMR_DEC_INPUT_STREAM_ID 0
/* ======================================================================= */
/**
 * @def    WBAMR_DEC_INPUT_BUFF_SIZE_EFR   Default input buffer size EFR
 *
 */
/* ======================================================================= */
#define WBAMR_DEC_INPUT_BUFF_SIZE_EFR 120

/* ======================================================================= */
/** WBAMR_DEC_COMP_PORT_TYPE  Port types
*
*  @param  WBAMR_DEC_INPUT_PORT                 Input port
*
*  @param  WBAMR_DEC_OUTPUT_PORT                Output port
*/
/*  ==================================================================== */
/*This enum must not be changed. */
typedef enum WBAMR_DEC_COMP_PORT_TYPE {
    WBAMR_DEC_INPUT_PORT = 0,
    WBAMR_DEC_OUTPUT_PORT
}WBAMR_DEC_COMP_PORT_TYPE;
/* ======================================================================= */
/**
 * @def    WBAMR_DEC_SAMPLING_FREQUENCY   Sampling frequency
 */
/* ======================================================================= */
#define WBAMR_DEC_SAMPLING_FREQUENCY 16000
/* ======================================================================= */
/**
 * @def    WBAMR_DEC_CPU_LOAD                    CPU Load in MHz
 */
/* ======================================================================= */
#define WBAMR_DEC_CPU_LOAD 10
/* ======================================================================= */
/**
 * @def    WBAMR_DEC_MAX_NUM_OF_BUFS   Maximum number of buffers
 */
/* ======================================================================= */
#define WBAMR_DEC_MAX_NUM_OF_BUFS 12
/* ======================================================================= */
/**
 * @def    IP_BUFFERSIZE                      Input Port Buffer Size
 */
/* ======================================================================= */
#define IP_WBAMRDEC_BUFFERSIZE 8192
/* ======================================================================= */
/**
 * @def    NUM_MIME_BYTES_ARRAY               amrMimeBytes array size
 */
/* ======================================================================= */
#define NUM_MIME_BYTES_ARRAY 16
/* ======================================================================= */
/**
 * @def    NUM_IF2_BYTES_ARRAY                amrIF2Bytes array size
 */
/* ======================================================================= */
#define NUM_IF2_BYTES_ARRAY 16

/* ======================================================================= */
/**
 * @def    WBAMR_DEC_DEBUG   Turns debug messaging on and off
 */
/* ======================================================================= */
#undef WBAMR_DEC_DEBUG
/* ======================================================================= */
/**
 * @def    WBAMR_DEC_MEMCHECK   Turns memory messaging on and off
 */
/* ======================================================================= */
#undef WBAMR_DEC_MEMCHECK

/* ======================================================================= */
/**
 * @def    WBAMRDEC_DEBUGMEM   Turns memory leaks messaging on and off.
 *         APP_DEBUGMEM must be defined in Test App in order to get
 *         this functionality On.
 */
/* ======================================================================= */
#undef WBAMRDEC_DEBUGMEM
/*#define WBAMRDEC_DEBUGMEM*/


#ifdef UNDER_CE
#define sleep Sleep
#endif

/* ======================================================================= */
/**
 * @def    WBAMR_DEC_NUM_OF_PORTS   Number of ports
 */
/* ======================================================================= */
#define WBAMR_DEC_NUM_OF_PORTS 2

/* ======================================================================= */
/**
 * @def    _ERROR_PROPAGATION__              Allow Logic to Detec Arm Errors 
 */
/* ======================================================================= */
#define _ERROR_PROPAGATION__ 

/* ======================================================================= */
/** OMX_INDEXAUDIOTYPE_WBAMRDEC  Defines the custom configuration settings
*                              for the component
*
*  @param  OMX_IndexCustomModeDasfConfig_WBAMRDEC  Sets the DASF mode
*
*  @param  OMX_IndexCustomModeAcdnConfig_WBAMRDEC  Sets the ACDN mode
*
*  @param  OMX_IndexCustomModeMimeConfig_WBAMRDEC  Sets the MIME mode
*/
/*  ==================================================================== */
typedef enum OMX_INDEXAUDIOTYPE_WBAMRDEC {
    OMX_IndexCustomModeEfrConfig_WBAMRDEC = 0xFF000001,
    OMX_IndexCustomModeAmrConfig_WBAMRDEC,
    OMX_IndexCustomModeAcdnConfig_WBAMRDEC,
    OMX_IndexCustomModeDasfConfig_WBAMRDEC,
    OMX_IndexCustomModeMimeConfig_WBAMRDEC,
    OMX_IndexCustomWbAmrDecHeaderInfoConfig,
    OMX_IndexCustomWbAmrDecStreamIDConfig,
    OMX_IndexCustomWbAmrDecDataPath,
    OMX_IndexCustomWbAmrDecNextFrameLost,
    OMX_IndexCustomDebug
}OMX_INDEXAUDIOTYPE_WBAMRDEC;

/* ======================================================================= */
/**
 * pthread variable to indicate OMX returned all buffers to app 
 */
/* ======================================================================= */
pthread_mutex_t bufferReturned_mutex; 
pthread_cond_t bufferReturned_condition; 

/* ======================================================================= */
/** WBAMR_DEC_StreamType  Stream types
*
*  @param  WBAMR_DEC_DMM                    DMM
*
*  @param  WBAMR_DEC_INSTRM                 Input stream
*
*  @param  WBAMR_DEC_OUTSTRM                Output stream
*/
/*  ==================================================================== */
enum WBAMR_DEC_StreamType
{
    WBAMR_DEC_DMM,
    WBAMR_DEC_INSTRM,
    WBAMR_DEC_OUTSTRM
};

enum WBAMR_DEC_DecodeType
{
    WBAMR,
    WBAMR_EFR
};

/* ======================================================================= */
/** WBAMR_DEC_MimeMode  Stream types
*
*  @param  WBAMR_DEC_MIMEMODE                   MIME
*
*  @param  WBAMR_DEC_NONMIMEMODE                NON MIME
*
*/
/*  ====================================================================== */
enum WBAMR_DEC_MimeMode {
    WBAMR_DEC_NONMIMEMODE,
    WBAMR_DEC_MIMEMODE
};

/* ======================================================================= */
/** WBAMR_DEC_BUFFER_Dir  Buffer Direction
*
*  @param  WBAMR_DEC_DIRECTION_INPUT                    INPUT
*
*  @param  WBAMR_DEC_DIRECTION_OUTPUT                   OUTPUT
*
*/
/*  ====================================================================== */
typedef enum {
    WBAMR_DEC_DIRECTION_INPUT,
    WBAMR_DEC_DIRECTION_OUTPUT
}WBAMR_DEC_BUFFER_Dir;

typedef struct WBAMR_DEC_BUFFS
{
    char WBAMR_DEC_BufHeader;
    char WBAMR_DEC_Buffer;
}WBAMR_DEC_BUFFS;

/* ======================================================================= */
/** WBAMR_DEC_BUFFERHEADERTYPE_INFO
*
*  @param  pBufHeader
*
*  @param  bBufOwner
*
*/
/*  ==================================================================== */
typedef struct WBAMR_DEC_BUFFERHEADERTYPE_INFO
{
    OMX_BUFFERHEADERTYPE* pBufHeader[WBAMR_DEC_MAX_NUM_OF_BUFS];
    WBAMR_DEC_BUFFS bBufOwner[WBAMR_DEC_MAX_NUM_OF_BUFS];
}WBAMR_DEC_BUFFERHEADERTYPE_INFO;

/* ======================================================================= */
/** WBAMR_DEC_LCML_MimeMode  Stream types
*
*  @param  WBAMR_DEC_MODE_MIME                  MIME
*
*  @param  WBAMR_DEC_MODE_NONMIME               NON MIME
*
*/
/*  ==================================================================== */
typedef enum {
    WBAMR_DEC_MODE_MIME,
    WBAMR_DEC_MODE_NONMIME
}WBAMR_DEC_LCML_MimeMode;

/* =================================================================================== */
/**
* Socket node input parameters.
*/
/* ================================================================================== */
typedef struct WBAMR_DEC_AudioCodecParams
{
    unsigned long  iSamplingRate;
    unsigned long  iStrmId;
    unsigned short iAudioFormat;

}WBAMR_DEC_AudioCodecParams;

/* =================================================================================== */
/**
* Socket node alg parameters.
*/
/* ================================================================================== */
typedef struct {
        unsigned long int usLastFrame;
        unsigned long int usFrameLost;
}WAMRDEC_FrameStruct;

typedef struct{
         unsigned long int usNbFrames;
         WAMRDEC_FrameStruct *pParamElem;
}WBAMRDEC_ParamStruct;

/* =================================================================================== */
/**
* WBAMR Buffer Header Type
*/
/* ================================================================================== */
typedef struct LCML_WBAMR_DEC_BUFHEADERTYPE {
      WBAMR_DEC_BUFFER_Dir eDir;
      OMX_BUFFERHEADERTYPE* buffer;
      WAMRDEC_FrameStruct *pFrameParam;
      WBAMRDEC_ParamStruct *pBufferParam;
      DMM_BUFFER_OBJ* pDmmBuf;
}LCML_WBAMR_DEC_BUFHEADERTYPE;

#ifndef UNDER_CE

OMX_ERRORTYPE OMX_ComponentInit (OMX_HANDLETYPE hComp);

#else
/*  WinCE Implicit Export Syntax */
#define OMX_EXPORT __declspec(dllexport)
/* ===========================================================  */
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
/*================================================================== */
OMX_EXPORT OMX_ERRORTYPE OMX_ComponentInit (OMX_HANDLETYPE hComp);

#endif

/* =================================================================================== */
/**
* Instrumentation info
*/
/* ================================================================================== */

typedef struct WBAMRDEC_BUFFERLIST WBAMR_DEC_BUFFERLIST;

/* =================================================================================== */
/**
* Structure for buffer list
*/
/* ================================================================================== */
struct WBAMRDEC_BUFFERLIST{
    OMX_BUFFERHEADERTYPE *pBufHdr[WBAMR_DEC_MAX_NUM_OF_BUFS];   /* records buffer header send by client */
    OMX_U32 bufferOwner[WBAMR_DEC_MAX_NUM_OF_BUFS];
    OMX_U32 bBufferPending[WBAMR_DEC_MAX_NUM_OF_BUFS];
    OMX_U16 numBuffers;
};
#ifdef UNDER_CE
    #ifndef _OMX_EVENT_
        #define _OMX_EVENT_
        typedef struct OMX_Event {
            HANDLE event;
        } OMX_Event;
    #endif
#endif

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


/* =================================================================================== */
/*
 * NBAMRDEC_BUFDATA
 */
/* =================================================================================== */
typedef struct WBAMRDEC_BUFDATA {
   OMX_U8 nFrames;     
}WBAMRDEC_BUFDATA;


/* =================================================================================== */
/**
* Component private data
*/
/* ================================================================================== */
typedef struct WBAMR_DEC_COMPONENT_PRIVATE
{
#ifdef __PERF_INSTRUMENTATION__
    PERF_OBJHANDLE pPERF, pPERFcomp;
    OMX_U32 nLcml_nCntIp;         
    OMX_U32 nLcml_nCntOpReceived;
#endif
    /** Array of pointers to BUFFERHEADERTYPE structues
       This pBufHeader[WBAMR_DEC_INPUT_PORT] will point to all the
       BUFFERHEADERTYPE structures related to input port,
       not just one structure. Same is for output port
       also. */

    OMX_BUFFERHEADERTYPE* pBufHeader[WBAMR_DEC_NUM_OF_PORTS];

    /** Number of input buffers at runtime */
    OMX_U32 nRuntimeInputBuffers;

    WBAMR_DEC_BUFFERHEADERTYPE_INFO BufInfo[WBAMR_DEC_NUM_OF_PORTS];

    OMX_CALLBACKTYPE cbInfo;
    /** Handle for use with async callbacks */

    OMX_PORT_PARAM_TYPE sPortParam;
    OMX_AUDIO_PARAM_PORTFORMATTYPE sInPortFormat;
    OMX_AUDIO_PARAM_PORTFORMATTYPE sOutPortFormat;

    /** This will contain info like how many buffers
        are there for input/output ports, their size etc, but not
        BUFFERHEADERTYPE POINTERS. */
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef[WBAMR_DEC_NUM_OF_PORTS];
    OMX_AUDIO_PARAM_AMRTYPE* wbamrParams[WBAMR_DEC_NUM_OF_PORTS];

    /** This is component handle */
    OMX_COMPONENTTYPE* pHandle;

    /** Current state of this component */
    OMX_STATETYPE curState;

    /** The component thread handle */
    pthread_t WBAMR_DEC_ComponentThread;

    /** The pipes for sending buffers to the thread */
    int dataPipe[2];

    /** The pipes for sending buffers to the thread */
    int cmdPipe[2];

    /** The pipes for sending buffers to the thread */
    int cmdDataPipe[2];

    /** The pipes for sending buffers to the thread */
  /*  int lcml_Pipe[2]; */

    /** Set to indicate component is stopping */
    OMX_U32 bIsStopping;

    /** Count of number of buffers outstanding with bridge */
    OMX_U32 lcml_nIpBuf;

    /** Count of number of buffers outstanding with bridge */
    OMX_U32 lcml_nOpBuf;

    OMX_U32 app_nBuf;
    OMX_U32 wbamrIf2Bytes[NUM_IF2_BYTES_ARRAY];                        /*Array With IF2 Lenght Information*/
    OMX_U32 lcml_nCntIp;
    OMX_U32 lcml_nCntOpReceived;
    OMX_U32 num_Reclaimed_Op_Buff;

    OMX_HANDLETYPE pLcmlHandle;
    LCML_WBAMR_DEC_BUFHEADERTYPE *pLcmlBufHeader[2];
    OMX_U32 iAmrMimeFlag;
    OMX_U32 iAmrSamplingFrequeny;
    OMX_U32 iAmrChannels;
    OMX_U32 iAmrMode;
    OMX_U32 dasfmode;
    OMX_U32 mimemode;
    OMX_U32 acdnmode;
    OMX_U32 fdwrite;
    OMX_U32 fdread;
    OMX_U32 streamID;

    OMX_U32 bPortDefsAllocated;
    OMX_U32 bCompThreadStarted;
    OMX_PTR pMarkData;
    OMX_MARKTYPE *pMarkBuf;
    OMX_HANDLETYPE hMarkTargetComponent;
    WBAMR_DEC_BUFFERLIST *pInputBufferList;
    WBAMR_DEC_BUFFERLIST *pOutputBufferList;
    LCML_STRMATTR *strmAttr;
    OMX_U32 nVersion;
    OMX_U32 wbamrMimeBytes[NUM_MIME_BYTES_ARRAY];
    OMX_U32 nHoldLength;
    OMX_U8* pHoldBuffer;
    OMX_U32 bLcmlHandleOpened;
    OMX_U32 nFillThisBufferCount;
    OMX_U32 nFillBufferDoneCount;
    OMX_U32 nEmptyThisBufferCount;
    OMX_U32 nEmptyBufferDoneCount;
    WBAMR_DEC_AudioCodecParams *pParams;
    OMX_U32 bInitParamsInitialized;
 /*     OMX_U32 bIdleCommandPending; */
    OMX_BUFFERHEADERTYPE *pInputBufHdrPending[WBAMR_DEC_MAX_NUM_OF_BUFS];
    OMX_U32 nNumInputBufPending;
    OMX_BUFFERHEADERTYPE *pOutputBufHdrPending[WBAMR_DEC_MAX_NUM_OF_BUFS];
    OMX_U32 nNumOutputBufPending;
    OMX_U32 bDisableCommandPending;
    OMX_U32 bEnableCommandPending;
    OMX_U32 bDisableCommandParam;
    OMX_U32 bEnableCommandParam;

    /** Flag to set when socket node stop callback should not transition
        component to OMX_StateIdle */
    OMX_U32 bNoIdleOnStop;

    /** Flag set when socket node is stopped */
    OMX_U32 bDspStoppedWhileExecuting;

    /** Number of outstanding FillBufferDone() calls */
    OMX_S32 nOutStandingFillDones;
    
    /** Stop Codec Command Sent Flag*/
    OMX_U8 bStopSent;
    
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
    
    OMX_S8 nUnhandledFillThisBuffers;
    OMX_S8 nUnhandledEmptyThisBuffers;
    OMX_BOOL bFlushOutputPortCommandPending;
    OMX_BOOL bFlushInputPortCommandPending;
#else
    OMX_Event AlloBuf_event;
    OMX_U8 AlloBuf_waitingsignal;
    
    OMX_Event InLoaded_event;
    OMX_U8 InLoaded_readytoidle;
    
    OMX_Event InIdle_event;
    OMX_U8 InIdle_goingtoloaded; 
#endif    
    OMX_U16 nRuntimeOutputBuffers;    
  
    OMX_U8 PendingPausedBufs;
    OMX_BUFFERHEADERTYPE *pOutputBufHdrPausedPending[WBAMR_DEC_MAX_NUM_OF_BUFS];
    
    OMX_BUFFERHEADERTYPE *LastOutbuf;

    OMX_BOOL bIsInvalidState;
    OMX_STRING* sDeviceString;
    
    void* ptrLibLCML;
    
    /** Circular array to keep buffer timestamps */
    OMX_S64 arrBufIndex[WBAMR_DEC_MAX_NUM_OF_BUFS]; 
    /** Circular array to keep buffer nTickCounts */
    OMX_S64 arrTickCount[WBAMR_DEC_MAX_NUM_OF_BUFS]; 
    /** Index to arrBufIndex[], used for input buffer timestamps */
    OMX_U8 IpBufindex;
    /** Index to arrBufIndex[], used for output buffer timestamps */
    OMX_U8 OpBufindex;  
    
    /** Flag to flush SN after EOS in order to process more buffers after EOS**/
    OMX_U8 SendAfterEOS;    

    OMX_BOOL bLoadedCommandPending;
    
    OMX_PARAM_COMPONENTROLETYPE componentRole;
    
    /** Pointer to port priority management structure */
    OMX_PRIORITYMGMTTYPE* pPriorityMgmt;

#ifdef RESOURCE_MANAGER_ENABLED
    RMPROXY_CALLBACKTYPE rmproxyCallback;
#endif

    OMX_BOOL bPreempted;
    OMX_BOOL bFrameLost;

    /** Flag to mark RTSP**/
    OMX_U8 using_rtsp;  
    
    PV_OMXComponentCapabilityFlagsType iPVCapabilityFlags;

    struct OMX_TI_Debug dbg;    

} WBAMR_DEC_COMPONENT_PRIVATE;

/*=======================================================================*/
/*! @fn SignalIfAllBuffersAreReturned 

 * @brief Sends pthread signal to indicate OMX has returned all buffers to app 

 * @param  none 

 * @Return void 

 */
/*=======================================================================*/
void SignalIfAllBuffersAreReturned(WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate);

#endif /* OMX_WBAMR_DECODER_H */
