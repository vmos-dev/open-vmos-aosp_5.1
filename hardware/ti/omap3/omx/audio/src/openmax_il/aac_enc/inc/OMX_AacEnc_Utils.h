
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
#ifndef OMX_AACENC_UTILS__H
#define OMX_AACENC_UTILS__H

#include <OMX_Component.h>
#include <OMX_TI_Common.h>
#include <OMX_TI_Debug.h>
#include "LCML_DspCodec.h"
#include "OMX_AacEncoder.h"

#ifdef RESOURCE_MANAGER_ENABLED
#include <ResourceManagerProxyAPI.h>
#endif

#define AACENC_MAJOR_VER 0x0001
#define AACENC_MINOR_VER 0x0001
#define NOT_USED 10
#define NORMAL_BUFFER 0
#define OMX_AACENC_DEFAULT_SEGMENT (0)
#define OMX_AACENC_SN_TIMEOUT (1000)
#define OMX_AACENC_SN_PRIORITY (10)
#define OMX_AACENC_NUM_DLLS (2)
#define AACENC_CPU_USAGE 45
#define _ERROR_PROPAGATION__ 
#define MPEG4AACENC_MAX_OUTPUT_FRAMES 24

#ifndef ANDROID
    #define ANDROID
#endif

#ifdef ANDROID
    #undef LOG_TAG
    #define LOG_TAG "OMX_AACENC"

/* PV opencore capability custom parameter index */
    #define PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX 0xFF7A347
#endif

#ifdef __PERF_INSTRUMENTATION__
    #include "perf.h"
#endif
#include <OMX_Component.h>

#define OMX_CONF_INIT_STRUCT(_s_, _name_)   \
    memset((_s_), 0x0, sizeof(_name_)); \
    (_s_)->nSize = sizeof(_name_);      \
    (_s_)->nVersion.s.nVersionMajor = AACENC_MAJOR_VER;  \
    (_s_)->nVersion.s.nVersionMinor = AACENC_MINOR_VER;  \
    (_s_)->nVersion.s.nRevision = 0x0001;      \
    (_s_)->nVersion.s.nStep = 0x0

#define OMX_CONF_CHECK_CMD(_ptr1, _ptr2, _ptr3) \
{                       \
    if(!_ptr1 || !_ptr2 || !_ptr3){     \
        OMXDBG_PRINT(stderr, ERROR, 4, 0, "%d :: Error bad parameter \n",__LINE__);\
        eError = OMX_ErrorBadParameter;     \
    goto EXIT;          \
    }                       \
}

#define OMX_CONF_SET_ERROR_BAIL(_eError, _eCode)\
{                       \
    _eError = _eCode;               \
    goto OMX_CONF_CMD_BAIL;         \
}

#define OMX_CLOSE_PIPE(_pStruct_,err)\
    OMXDBG_PRINT(stderr, PRINT, 2, OMX_DBG_BASEMASK, "%d :: CLOSING PIPE \n",__LINE__);\
    err = close (_pStruct_);\
    if(0 != err && OMX_ErrorNone == eError){\
        eError = OMX_ErrorHardware;\
        OMXDBG_PRINT(stderr, ERROR, 4, 0, "%d :: Error while closing pipe\n",__LINE__);\
        goto EXIT;\
    }

#define OMX_DPRINT_ADDRESS(_s_, _ptr_)  \
    OMXDBG_PRINT(stderr, PRINT, 2, 0, "%s = %p\n", _s_, _ptr_);


#undef SWAT_ANALYSIS

/* debug message */ 
#undef AACENC_DEBUG        
#define AACENC_ERROR


#ifndef UNDER_CE
#ifdef  AACENC_ERROR
     #define AACENC_EPRINT(...) fprintf(stderr,__VA_ARGS__)
#else
     #define AACENC_EPRINT(...)
#endif /* AAC_ERROR*/


#ifdef  AACENC_DEBUG
     #define AACENC_DPRINT(...) fprintf(stderr,__VA_ARGS__)
#else
     #define AACENC_DPRINT(...)
#endif

#else /*UNDER_CE*/ 

#ifdef  AACENC_DEBUG
 #define AACENC_DPRINT(STR, ARG...) printf()
#else
#endif

#endif /*UNDER_CE*/ 

/* ======================================================================= */
/**
 * @def    AACENC_USN_DLL_NAME   USN DLL name
 */
/* ======================================================================= */
#ifdef UNDER_CE
#define AACENC_USN_DLL_NAME "\\windows\\usn.dll64P"
#else
#define AACENC_USN_DLL_NAME "usn.dll64P"
#endif

/* ======================================================================= */
/**
 * @def    AACENC_DLL_NAME   AAC Enc Encoder socket node DLL name
 */
/* ======================================================================= */
#ifdef UNDER_CE
#define AACENC_DLL_NAME "\\windows\\mpeg4aacenc_sn.dll64P"
#else
#define AACENC_DLL_NAME "mpeg4aacenc_sn.dll64P"
#endif

typedef struct 
{
    OMX_BOOL bLastBuffer;
}AACENC_UAlgInBufParamStruct;

typedef struct 
{
    unsigned long unNumFramesEncoded;
    unsigned long unFrameSizes[MPEG4AACENC_MAX_OUTPUT_FRAMES];
}AACENC_UAlgOutBufParamStruct;


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

/*This enum must not be changed.*/
typedef enum COMP_PORT_TYPE 
{
    INPUT_PORT = 0,
    OUTPUT_PORT
}COMP_PORT_TYPE;

typedef enum 
{
    DIRECTION_INPUT,
    DIRECTION_OUTPUT
}BUFFER_Dir;

typedef struct BUFFS 
{
    char BufHeader;
    char Buffer;
}BUFFS;

typedef struct BUFFERHEADERTYPE_INFO 
{
    OMX_BUFFERHEADERTYPE* pBufHeader[MAX_NUM_OF_BUFS];
    BUFFS bBufOwner[MAX_NUM_OF_BUFS];
}BUFFERHEADERTYPE_INFO;

typedef OMX_ERRORTYPE (*fpo)(OMX_HANDLETYPE);

typedef struct AACENC_AudioCodecParams 
{
    unsigned long  iSamplingRate;
    unsigned long  iStrmId;
    unsigned short iAudioFormat;
}AACENC_AudioCodecParams;

/* enum AACENC_BOOL_TYPE                        */
/* brief Enumerated type for Boolean decision   */

typedef enum 
{
    AACENC_FALSE = 0,     /*!< To indicate False  */
    AACENC_TRUE  = 1      /*!< To indicate True   */
} AACENC_BOOL_TYPE;

/*! \enum AACENC_OBJ_TYP  */
/*! \brief enumerated type for output format */
typedef enum
{
  AACENC_OBJ_TYP_LC           = 2,   /*!< AAC Low Complexity  */
  AACENC_OBJ_TYP_HEAAC        = 5,   /*!< HE AAC              */
  AACENC_OBJ_TYP_PS           = 29   /*!< AAC LC + SBR + PS   */
} AACENC_OBJ_TYP;

/*! \enum  AACENC_TRANSPORT_TYPE */
/*! \brief Enumerated type for output file format */
typedef enum 
{
  AACENC_TT_RAW    = 0,              /*!< Raw Output Format   */
  AACENC_TT_ADIF   = 1,              /*!< ADIF File Format    */
  AACENC_TT_ADTS   = 2               /*!< ADTS File Format    */
} AACENC_TRANSPORT_TYPE;

/* ======================================================================= */
/** MPEG4AACENC_UALGParams
 *
 * @param lOutputFormat - To set interleaved/Block format:Refer to IAUDIO_PcmFormat.
 * @param DownSampleSbr -
 */
/* ==================================================================== */
typedef struct 
{
    long size;           /* size of this structure */
    long bitRate;        /* Average bit rate in bits per second */
    long sampleRate;     /* Samplling frequency in Hz */
    long numChannels;    /* Number of Channels: IAUDIO_ChannelId */

}MPEG4AUDENC_UALGdynamicparams;


typedef struct MPEG4AACENC_UALGParams 
{
    int size;
    MPEG4AUDENC_UALGdynamicparams audenc_dynamicparams; 
                                            /*!< generic encoder dynamic parameters  */

    /* For RESET Command */
    AACENC_BOOL_TYPE        useTns;         /*!< Flag for activating TNS feature     */
    AACENC_BOOL_TYPE        usePns;         /*!< Flag for activating PNS feature     */

    AACENC_OBJ_TYP          outObjectType;  /*!< Output Object Type LC/HE/HEv2     */
    AACENC_TRANSPORT_TYPE   outFileFormat;  /*!< Output File Format            */

} MPEG4AACENC_UALGParams;

typedef enum {
    IUALG_CMD_STOP          = 0,
    IUALG_CMD_PAUSE         = 1,
    IUALG_CMD_GETSTATUS     = 2,
    IUALG_CMD_SETSTATUS     = 3,    
    IUALG_CMD_USERCMDSTART  = 100
}IUALG_Cmd;

typedef enum {
    IAUDIO_BLOCK=0,
    IAUDIO_INTERLEAVED
} IAUDIO_AacFormat;

typedef struct LCML_AACENC_BUFHEADERTYPE 
{
      BUFFER_Dir eDir;
      OMX_BUFFERHEADERTYPE* buffer;
      AACENC_UAlgInBufParamStruct *pIpParam;
      AACENC_UAlgOutBufParamStruct *pOpParam;
}LCML_AACENC_BUFHEADERTYPE;



typedef struct _BUFFERLIST BUFFERLIST;

struct _BUFFERLIST
{
    OMX_BUFFERHEADERTYPE sBufHdr;                   /* actual buffer header */
    OMX_BUFFERHEADERTYPE *pBufHdr[MAX_NUM_OF_BUFS]; /* records buffer header send by client */
    OMX_U32 bufferOwner[MAX_NUM_OF_BUFS];
    OMX_U16 numBuffers;
    OMX_U32 bBufferPending[MAX_NUM_OF_BUFS];
    BUFFERLIST *pNextBuf;
    BUFFERLIST *pPrevBuf;
};

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

/* ======================================================================= */
/**
 * pthread variable to indicate OMX returned all buffers to app
 */
/* ======================================================================= */
pthread_mutex_t bufferReturned_mutex;
pthread_cond_t bufferReturned_condition;


typedef struct AACENC_COMPONENT_PRIVATE 
{
    /** Array of pointers to BUFFERHEADERTYPE structues
       This pBufHeader[INPUT_PORT] will point to all the
       BUFFERHEADERTYPE structures related to input port,
       not just one structure. Same is for output port
       also. */
    OMX_BUFFERHEADERTYPE* pBufHeader[NUM_OF_PORTS];

    BUFFERHEADERTYPE_INFO BufInfo[NUM_OF_PORTS];

    OMX_CALLBACKTYPE cbInfo;
    /** Handle for use with async callbacks */
    OMX_PORT_PARAM_TYPE sPortParam;
    
    OMX_PRIORITYMGMTTYPE* sPriorityMgmt;
    
#ifdef RESOURCE_MANAGER_ENABLED
    RMPROXY_CALLBACKTYPE rmproxyCallback;
#endif

    OMX_BOOL bPreempted;
    
    OMX_AUDIO_PARAM_PORTFORMATTYPE sInPortFormat;
    
    OMX_AUDIO_PARAM_PORTFORMATTYPE sOutPortFormat;
    
    OMX_U32 bIsBufferOwned[NUM_OF_PORTS];
    /** This will contain info like how many buffers
        are there for input/output ports, their size etc, but not
        BUFFERHEADERTYPE POINTERS. */
    OMX_U32 abc;
    
    OMX_U32 def;
    
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef[NUM_OF_PORTS];
    
    OMX_AUDIO_PARAM_AACPROFILETYPE* aacParams[NUM_OF_PORTS];

    OMX_AUDIO_PARAM_PCMMODETYPE* pcmParam[NUM_OF_PORTS];

    /** This is component handle */
    OMX_COMPONENTTYPE* pHandle;

    /** Current state of this component */
    OMX_STATETYPE curState;

    /** The component thread handle */
    pthread_t ComponentThread;

    /** The pipes for sending buffers to the thread */
    int dataPipe[2];

    /** The pipes for sending buffers to the thread */
    int cmdPipe[2];
    /** The pipes for sending buffers to the thread */
    int cmdDataPipe[2];

    /** Set to indicate component is stopping */
    OMX_U32 bIsStopping;

    /** Number of input buffers at runtime */
    OMX_U32 nRuntimeInputBuffers;

    /** Number of output buffers at runtime */
    OMX_U32 nRuntimeOutputBuffers;

    OMX_U32 bIsThreadstop;

    OMX_U32 bIsEOFSent;

    /** Count of number of buffers outstanding with bridge */
    OMX_U32 lcml_nIpBuf;

    OMX_BOOL CustomConfiguration;

#ifdef __PERF_INSTRUMENTATION__
        PERF_OBJHANDLE pPERF, pPERFcomp;
        OMX_U32 nLcml_nCntIp;         
        OMX_U32 nLcml_nCntOpReceived;
#endif

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

    OMX_HANDLETYPE pLcmlHandle;

    LCML_AACENC_BUFHEADERTYPE *pLcmlBufHeader[2];

    MPEG4AACENC_UALGParams* ptAlgDynParams;

    AACENC_AudioCodecParams* pParams;

    OMX_U16 ulSamplingRate;
    
    OMX_U16 unNumChannels;
    
    OMX_U32 unBitrate;
    
    OMX_U16 nObjectType;
    
    OMX_U32 bitRateMode;
    
    OMX_U16 File_Format;
    
    OMX_U32 dasfmode;

    OMX_U32 EmptybufferdoneCount;
    
    OMX_U32 EmptythisbufferCount;

    OMX_U32 FillbufferdoneCount;
    
    OMX_U32 FillthisbufferCount;
    
    OMX_U32 bPortDefsAllocated;
    
    OMX_U32 bCompThreadStarted;
    
    OMX_PTR pMarkData;
    
    OMX_MARKTYPE *pMarkBuf;
    
    OMX_HANDLETYPE hMarkTargetComponent;
    
    OMX_U32 bBypassDSP;
    
    BUFFERLIST *pInputBufferList;
    
    BUFFERLIST *pOutputBufferList;
    
    LCML_STRMATTR *strmAttr;
    
    OMX_U32 nVersion;
    
    OMX_STRING cComponentName;
    
    OMX_VERSIONTYPE ComponentVersion;
    
    OMX_U32 streamID;
    
    OMX_U32 bInputBufferHeaderAllocated;
    /** Stores input buffers while paused */
    OMX_BUFFERHEADERTYPE *pInputBufHdrPending[MAX_NUM_OF_BUFS];

    /** Number of input buffers received while paused */
    OMX_U32 nNumInputBufPending;

    /** Stores output buffers while paused */   
    OMX_BUFFERHEADERTYPE *pOutputBufHdrPending[MAX_NUM_OF_BUFS];

    /** Number of output buffers received while paused */
    OMX_U32 nNumOutputBufPending;


    OMX_U8 PendingInPausedBufs;
    OMX_BUFFERHEADERTYPE *pInBufHdrPausedPending[MAX_NUM_OF_BUFS];
    OMX_U8 PendingOutPausedBufs;
    OMX_BUFFERHEADERTYPE *pOutBufHdrPausedPending[MAX_NUM_OF_BUFS];



    OMX_U32 bPlayCompleteFlag;

    /** Flag set when a disable command is pending */
    OMX_U32 bDisableCommandPending;

    /** Parameter for pending disable command */
    OMX_U32 bDisableCommandParam;

    /** Flag set when a disable command is pending */
    OMX_U32 bEnableCommandPending;

    /** Parameter for pending disable command */
    OMX_U32 nEnableCommandParam;

    /** Flag to set when socket node stop callback should not transition
        component to OMX_StateIdle */
    OMX_U32 bNoIdleOnStop;

    /** Flag set when idle command is pending */
    /* OMX_U32 bIdleCommandPending; */

    /** Flag set when pause command is pending */
    OMX_U32 bPauseCommandPending;

    /** Flag set when socket node is stopped */
    OMX_U32 bDspStoppedWhileExecuting;

    /** Number of outstanding FillBufferDone() calls */
    OMX_S32 nOutStandingFillDones;

    /** Number of outstanding EmptyBufferDone() calls */
    OMX_S32 nOutStandingEmptyDones;

    OMX_BUFFERHEADERTYPE *LastOutputBufferHdrQueued;
    
#ifndef UNDER_CE
    pthread_mutex_t AlloBuf_mutex;    
    pthread_cond_t AlloBuf_threshold;
    OMX_U8 AlloBuf_waitingsignal;
    
    pthread_mutex_t codecStop_mutex;    
    pthread_cond_t codecStop_threshold;
    OMX_U8 codecStop_waitingsignal;

    pthread_mutex_t codecFlush_mutex;    
    pthread_cond_t codecFlush_threshold;
    OMX_U8 codecFlush_waitingsignal;

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
    
#else
    OMX_Event AlloBuf_event;
    OMX_U8 AlloBuf_waitingsignal;
    
    OMX_Event InLoaded_event;
    OMX_U8 InLoaded_readytoidle;
    
    OMX_Event InIdle_event;
    OMX_U8 InIdle_goingtoloaded; 

    
#endif
    OMX_BOOL bLoadedCommandPending;
    OMX_BOOL bIsInvalidState;
    void* PtrCollector[6];
    
    OMX_BUFFERHEADERTYPE *LastOutbuf;
    OMX_PARAM_COMPONENTROLETYPE componentRole;
    OMX_U16 FramesPer_OutputBuffer;

    /* backup pointer for LCML */
    void* ptrLibLCML;

    OMX_BOOL bCodecDestroyed;
    OMX_BOOL bGotLCML;
    
    OMX_STRING* sDeviceString;
    OMX_BOOL bFirstOutputBuffer;

    /** Keep buffer timestamps **/
    OMX_S64 timestampBufIndex[MAX_NUM_OF_BUFS];
    /** Index to arrBufIndex[], used for input buffer timestamps */
    OMX_U8 IpBufindex;
    /** Index to arrBufIndex[], used for output buffer timestamps */
    OMX_U8 OpBufindex;

    /** Keep buffer tickcount **/
    OMX_U32 tickcountBufIndex[MAX_NUM_OF_BUFS];
    
    PV_OMXComponentCapabilityFlagsType iPVCapabilityFlags;

    struct OMX_TI_Debug dbg;

    /* Reference count for pending state change requests */
    OMX_U32 nPendingStateChangeRequests;
    pthread_mutex_t mutexStateChangeRequest;
    pthread_cond_t StateChangeCondition;

} AACENC_COMPONENT_PRIVATE;

OMX_ERRORTYPE AACENCGetCorresponding_LCMLHeader(AACENC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U8 *pBuffer,
                                                  OMX_DIRTYPE eDir,
                                                  LCML_AACENC_BUFHEADERTYPE **ppLcmlHdr);

OMX_ERRORTYPE AACENCLCML_Callback(TUsnCodecEvent event,void * args [10]);

OMX_ERRORTYPE AACENCFill_LCMLInitParams(OMX_HANDLETYPE pHandle,
                                          LCML_DSP *plcml_Init,OMX_U16 arr[]);

OMX_ERRORTYPE AACENCGetBufferDirection(OMX_BUFFERHEADERTYPE *pBufHeader, OMX_DIRTYPE *eDir);

OMX_U32 AACENCHandleCommand(AACENC_COMPONENT_PRIVATE *pComponentPrivate);

OMX_ERRORTYPE AACENCHandleDataBuf_FromApp(OMX_BUFFERHEADERTYPE *pBufHeader,
                                            AACENC_COMPONENT_PRIVATE *pComponentPrivate);

int AACEnc_GetSampleRateIndexL( const int aRate);

OMX_HANDLETYPE AACENCGetLCMLHandle(AACENC_COMPONENT_PRIVATE *pComponentPrivate);

OMX_ERRORTYPE AACENC_CleanupInitParams(OMX_HANDLETYPE pHandle);

void AACENC_SetPending(AACENC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir, OMX_U32 lineNumber);

void AACENC_ClearPending(AACENC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir, OMX_U32 lineNumber) ;

OMX_U32 AACENC_IsPending(AACENC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir);

/* ===========================================================  */
/**
*  AACENC_TransitionToIdle() Transitions component to idle
* 
*
*  @param pComponentPrivate     Component private data
*
*  @return OMX_ErrorNone = No error
*          OMX Error code = Error
*/
/*================================================================== */

OMX_ERRORTYPE AACENC_TransitionToPause(AACENC_COMPONENT_PRIVATE *pComponentPrivate);

OMX_ERRORTYPE AACENCFill_LCMLInitParamsEx(OMX_HANDLETYPE pComponent);

OMX_ERRORTYPE AACENCWriteConfigHeader(AACENC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr);

#ifdef RESOURCE_MANAGER_ENABLED
void AACENC_ResourceManagerCallback(RMPROXY_COMMANDDATATYPE cbData);
#endif

#ifndef UNDER_CE
OMX_ERRORTYPE OMX_ComponentInit (OMX_HANDLETYPE hComp);
#else
/*  WinCE Implicit Export Syntax */
#define OMX_EXPORT __declspec(dllexport)
OMX_EXPORT OMX_ERRORTYPE OMX_ComponentInit (OMX_HANDLETYPE hComp);
#endif


OMX_ERRORTYPE AACENC_StartComponentThread(OMX_HANDLETYPE pHandle);

OMX_ERRORTYPE AACENC_StopComponentThread(OMX_HANDLETYPE pHandle);

OMX_ERRORTYPE AACENC_FreeCompResources(OMX_HANDLETYPE pComponent);

OMX_ERRORTYPE AddStateTransition(AACENC_COMPONENT_PRIVATE* pComponentPrivate);
OMX_ERRORTYPE RemoveStateTransition(AACENC_COMPONENT_PRIVATE* pComponentPrivate, OMX_BOOL bEnableSignal);

/*  =========================================================================*/
/**  func    AACENC_HandleUSNError
 *
 *  desc    Handles error messages returned by the dsp
 *
 *@return n/a
 */
/*  =========================================================================*/
void AACENC_HandleUSNError (AACENC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U32 arg);


#endif


/*=======================================================================*/
/** @fn SignalIfAllBuffersAreReturned
 * @brief Sends pthread signal to indicate OMX has returned all buffers to app
 *
 * @param  none
 *
 * @Return none
 *
 */
/*=======================================================================*/
void SignalIfAllBuffersAreReturned(AACENC_COMPONENT_PRIVATE *pComponentPrivate);

/* ====================================================================== */
/*@AACENC_IncrementBufferCounterByOne() This function is used by the component
 * to atomically increment some input or output buffer counter
 *
 * @param mutex pointer to mutex for synchronizing the value change on
 *              the counter
 * @param counter the buffer counter to be changed
 *
 * @post the buffer counter's value will be incremented by one.
 * @return None
 */
/* ====================================================================== */
void AACENC_IncrementBufferCounterByOne(pthread_mutex_t* mutex, OMX_U32 *counter);
