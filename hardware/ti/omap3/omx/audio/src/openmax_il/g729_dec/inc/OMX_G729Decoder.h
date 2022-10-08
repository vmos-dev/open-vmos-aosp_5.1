
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
 *             Texas Instruments OMAP (TM) Platform Software
 *  (c) Copyright Texas Instruments, Incorporated.  All Rights Reserved.
 *
 *  Use of this software is controlled by the terms and conditions found
 *  in the license agreement under which this software has been supplied.
 * =========================================================================== */
/**
 * @file OMX_G729Decoder.h
 *
 * This header file contains data and function prototypes for G729 DECODER OMX 
 *
 * @path  $(OMAPSW_MPU)\linux\audio\src\openmax_il\g729_dec\inc
 *
 * @rev  0.3
 */
/* ----------------------------------------------------------------------------- 
 *! 
 *! Revision History 
 *! ===================================
 *! Date         Author(s)            Version  Description
 *! ---------    -------------------  -------  ---------------------------------
 *! 03-Jan-2007  A.Donjon             0.1      Code update for G729 DECODER
 *! 16-Feb-2007  A.Donjon             0.2      Frame Lost
 *! 08-Jun-2007  A.Donjon             0.3      Variable input buffer size
 *! 
 *!
 * ================================================================================= */
/* ------compilation control switches -------------------------*/
#ifndef OMX_G729DECODER_H
#define OMX_G729DECODER_H

#include <LCML_DspCodec.h>
#include <OMX_Component.h>
#include <pthread.h>

#ifdef RESOURCE_MANAGER_ENABLED
#include <ResourceManagerProxyAPI.h>
#endif

#ifdef __PERF_INSTRUMENTATION__
#include "perf.h"
#endif

#undef G729DEC_PRINT
#undef G729DEC_DEBUG 
#undef G729DEC_MEMCHECK 

/* ======================================================================= */
/**
 * @def    G729D_TIMEOUT   Default timeout used to come out of blocking calls
 */
/* ======================================================================= */
#define G729D_TIMEOUT (1000) /* millisecs */


/* ======================================================================= */
/**
 * @def    NUM_G729DEC_INPUT_BUFFERS              Number of Input Buffers    
 */
/* ======================================================================= */
#define NUM_G729DEC_INPUT_BUFFERS 1

/* ======================================================================= */
/**
 * @def    NUM_G729DEC_OUTPUT_BUFFERS              Number of Output Buffers    
 */
/* ======================================================================= */
#define NUM_G729DEC_OUTPUT_BUFFERS 1

/* ======================================================================= */
/**
 * @def    G729DEC_PACKETS_PER_BUFFER             Number of PACKETS PER BUFFER   
 */
/* ======================================================================= */
#define G729DEC_PACKETS_PER_BUFFER 1

/* ======================================================================= */
/**
 * @def    NUM_G729DEC_OUTPUT_BUFFERS_DASF         Number of Output Buffers
 *                                                  in DASF mode
 */
/* ======================================================================= */
#define NUM_G729DEC_OUTPUT_BUFFERS_DASF 2

/* ======================================================================= */
/**
 * @def    INPUT_G729DEC_BUFFER_SIZE             Standart Input Buffer Size
 *                                                (1 packet)
 */
/* ======================================================================= */
#define INPUT_G729DEC_BUFFER_SIZE_MIN 11

/* ======================================================================= */
/**
 * @def    OUTPUT_G729DEC_BUFFER_SIZE           Standart Output Buffer Size
 *                                               (1 frame)
 */
/* ======================================================================= */
#define OUTPUT_G729DEC_BUFFER_SIZE_MIN 80<<1

/* ======================================================================= */
/**
 * @def    G729DEC_SHIFT_OFFSET           Shift Amount to move low 8 bits to high 8 bits in 32 bit field
 */
/* ======================================================================= */
#define G729DEC_SHIFT_OFFSET 24

/* ======================================================================= */
/**
 * @def    STREAM_COUNT                         Stream Count value for
 *                                              LCML init.
 */
/* ======================================================================= */
#define STREAM_COUNT 2

/* ======================================================================= */
/**
 * @def    INPUT_STREAM_ID                      Input Stream ID
 */
/* ======================================================================= */
#define INPUT_STREAM_ID 0

/* ======================================================================= */
/**
 * @def    G729DEC_SAMPLING_FREQUENCY          Sampling Frequency   
 */
/* ======================================================================= */
#define G729DEC_SAMPLING_FREQUENCY 8000

/* ======================================================================= */
/**
 * @def    MAX_NUM_OF_BUFS                      Max Num of Bufs Allowed   
 */
/* ======================================================================= */
#define MAX_NUM_OF_BUFS 10

/* ======================================================================= */
/**
 * @def    EXTRA_BUFFBYTES                Num of Extra Bytes to be allocated
 */
/* ======================================================================= */
#define EXTRA_BUFFBYTES (256)

/* ======================================================================= */
/**
 * @def  CACHE_ALIGNMENT                           Buffer Cache Alignment
 */
/* ======================================================================= */
#define CACHE_ALIGNMENT 128

/* ======================================================================= */
/**
 * @def    NUM_OF_PORTS                       Number of Comunication Port
 */
/* ======================================================================= */
#define NUM_OF_PORTS 2

/* ======================================================================= */
#define G729DEC_CPU 50 /* TBD, 50MHz for the moment */
/* ======================================================================= 
 * @def OMX_G729MALLOC_STRUCT                          structure allocation macro
 */
/* ======================================================================= */
#define OMX_G729MALLOC_STRUCT(_pStruct_, _sName_)               \
    _pStruct_ = (_sName_*)malloc(sizeof(_sName_));              \
    if(_pStruct_ == NULL){                                      \
        printf("***********************************\n");        \
        printf("%d :: Malloc Failed\n",__LINE__);               \
        printf("***********************************\n");        \
        eError = OMX_ErrorInsufficientResources;                \
        goto EXIT;                                              \
    }                                                           \
    memset(_pStruct_, 0x0, sizeof(_sName_));                    \
    G729DEC_MEMPRINT("%d :: [ALLOC] %p\n",__LINE__,_pStruct_);

#define OMX_G729CONF_INIT_STRUCT(_s_, _name_)   \
    memset((_s_), 0x0, sizeof(_name_));         \
    (_s_)->nSize = sizeof(_name_);              \
    (_s_)->nVersion.s.nVersionMajor = 0x1;      \
    (_s_)->nVersion.s.nVersionMinor = 0x1;      \
    (_s_)->nVersion.s.nRevision = 0x0;          \
    (_s_)->nVersion.s.nStep = 0x0;

#define OMX_G729MEMFREE_STRUCT(_pStruct_)                       \
    if(_pStruct_ != NULL)                                       \
    {                                                           \
        G729DEC_MEMPRINT("%d :: [FREE] %p\n", __LINE__, _pStruct_); \
        free(_pStruct_);                                        \
        _pStruct_ = NULL;                                       \
    }
    
/****************************************************************
 *  INCLUDE FILES                                                 
 ****************************************************************/
/* ----- system and platform files ----------------------------*/
/*-------program files ----------------------------------------*/

/****************************************************************
 * EXTERNAL REFERENCES NOTE : only use if not found in header file
 ****************************************************************/
/*--------data declarations -----------------------------------*/
/*--------function prototypes ---------------------------------*/

/****************************************************************
 * PUBLIC DECLARATIONS Defined here, used elsewhere
 ****************************************************************/
/*--------data declarations -----------------------------------*/

/*--------function prototypes ---------------------------------*/

/****************************************************************
 * PRIVATE DECLARATIONS Defined here, used only here
 ****************************************************************/
/*--------data declarations -----------------------------------*/
/* ======================================================================= */
/** G729DEC_COMP_PORT_TYPE  Port Type
 *
 *  @param  G729DEC_INPUT_PORT                   Port Type Input
 *
 *  @param  G729DEC_OUTPUT_PORT                  Port Type Output
 *
 */
/*  ==================================================================== */
typedef enum G729DEC_COMP_PORT_TYPE {
    G729DEC_INPUT_PORT = 0,
    G729DEC_OUTPUT_PORT
}G729DEC_COMP_PORT_TYPE;

/* ======================================================================= */
/** G729DEC_StreamType  StreamType
 *
 *  @param  G729DEC_DMM                  Stream Type DMM
 *
 *  @param  G729DEC_INSTRM               Stream Type Input
 *
 *  @param  G729DEC_OUTSTRM              Stream Type Output
 */
/*  ==================================================================== */
enum G729DEC_StreamType
    {
        G729DEC_DMM,
        G729DEC_INSTRM,
        G729DEC_OUTSTRM
    };


/* ======================================================================= */
/** G729DEC_BUFFER_Dir  Direction of the Buffer
 *
 *  @param  G729DEC_DIRECTION_INPUT              Direction Input
 *
 *  @param  G729DEC_DIRECTION_INPUT              Direction Output
 */
/*  ==================================================================== */
typedef enum {
    G729DEC_DIRECTION_INPUT,
    G729DEC_DIRECTION_OUTPUT
}G729DEC_BUFFER_Dir;

/* =================================================================================== */
/**
 *  Buffer Information.
 */
/* ================================================================================== */
typedef struct BUFFS
{
    OMX_S8 BufHeader;
    OMX_S8 Buffer;
}BUFFS;

/* =================================================================================== */
/**
 * NBAMR Buffer Header Type Info.
 */
/* ================================================================================== */
typedef struct BUFFERHEADERTYPE_INFO
{
    OMX_BUFFERHEADERTYPE* pBufHeader[MAX_NUM_OF_BUFS];
    BUFFS bBufOwner[MAX_NUM_OF_BUFS];
}BUFFERHEADERTYPE_INFO;

/* =================================================================================== */
/**
 * Socket node input parameters.
 */
/* ================================================================================== */
typedef struct G729DEC_AudioCodecParams
{
    unsigned long iSamplingRate;
    unsigned long iStrmId;
    unsigned short iAudioFormat;
}G729DEC_AudioCodecParams;

/* =================================================================================== */
/**
 * Socket node alg parameters..
 */
/* ================================================================================== */
typedef struct G729DEC_UAlgBufParamStruct
{
    unsigned long usLastFrame;      /* Last frame to decode */
    unsigned long usFrameLost;      /* Lost frame flag */
}G729DEC_UAlgBufParamStruct;

/* =================================================================================== */
/**
 * LCML_G729DEC_BUFHEADERTYPE
 */
/* ================================================================================== */
typedef struct LCML_G729DEC_BUFHEADERTYPE {
    G729DEC_BUFFER_Dir eDir;
    OMX_BUFFERHEADERTYPE* buffer;
    G729DEC_UAlgBufParamStruct *pIpParam;
}LCML_G729DEC_BUFHEADERTYPE;


typedef struct _G729DEC_BUFFERLIST G729DEC_BUFFERLIST;

/* =================================================================================== */
/**
 * Structure for buffer list
 */
/* ================================================================================== */
struct _G729DEC_BUFFERLIST{
    OMX_BUFFERHEADERTYPE sBufHdr;
    OMX_BUFFERHEADERTYPE *pBufHdr[MAX_NUM_OF_BUFS];  /* records buffer header send by client */ 
    OMX_U32 bufferOwner[MAX_NUM_OF_BUFS];
    OMX_U32 bBufferPending[MAX_NUM_OF_BUFS];
    OMX_U32 numBuffers; 
    G729DEC_BUFFERLIST *pNextBuf;
    G729DEC_BUFFERLIST *pPrevBuf;
};

/* =================================================================================== */
/**
 * Component private data
 */
/* ================================================================================== */
typedef struct G729DEC_COMPONENT_PRIVATE
{
    /** Array of pointers to BUFFERHEADERTYPE structures
        This pBufHeader[G729DEC_INPUT_PORT] will point to all the
        BUFFERHEADERTYPE structures related to input port,
        not just one structure. Same is for output port
        also. */

    OMX_BUFFERHEADERTYPE* pBufHeader[NUM_OF_PORTS];

    BUFFERHEADERTYPE_INFO BufInfo[NUM_OF_PORTS];

    /** Structure of callback pointers */
    OMX_CALLBACKTYPE cbInfo;
    
    /** Handle for use with async callbacks */
    OMX_PORT_PARAM_TYPE sPortParam;

    OMX_PRIORITYMGMTTYPE* sPriorityMgmt;
        
    /** Input port parameters */
    OMX_AUDIO_PARAM_PORTFORMATTYPE* pInPortFormat;
    
    /** Output port parameters */
    OMX_AUDIO_PARAM_PORTFORMATTYPE* pOutPortFormat;

    /** This will contain info like how many buffers
        are there for input/output ports, their size etc, but not
        BUFFERHEADERTYPE POINTERS. */
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef[NUM_OF_PORTS];
    
    /** G729 Component Parameters */
    OMX_AUDIO_PARAM_G729TYPE* g729Params;
    OMX_AUDIO_PARAM_PCMMODETYPE* pcmParams; 

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

    /** The pipes for sending buffers to the thread */
    int lcml_Pipe[2];

    /** Set to indicate component is stopping */
    OMX_U32 bIsStopping;

    /** Flag set when the EOF marker is sent */
    OMX_U32 bIsEOFSent;

    /** Count of number of buffers outstanding with bridge */
    OMX_U32 lcml_nIpBuf;

    /** Count of number of buffers outstanding with bridge */
    OMX_U32 lcml_nOpBuf;

    /** Number of Buffers In the Application*/
    OMX_U32 app_nBuf;

    /** LCML Number Input Buffer Received*/
    OMX_U32 lcml_nCntIp;
    
    /** LCML Number Output Buffer Received*/
    OMX_U32 lcml_nCntOpReceived;
    
    /** Num Reclaimed OutPut Buff    */
    OMX_U32 num_Reclaimed_Op_Buff;
    
    /** Num Sent Input Buff   */
    OMX_U32 num_Sent_Ip_Buff;
    
    /** Num Sent Output Buff Issued   */
    OMX_U32 num_Op_Issued;

    /** LCML Handle */
    OMX_HANDLETYPE pLcmlHandle;
    
    /** LCML Buffer Header */
    LCML_G729DEC_BUFHEADERTYPE *pLcmlBufHeader[2];   
    
    /** Sampling Frequeny */
    OMX_S16 iG729SamplingFrequeny;
    
    /** Number of channels */
    OMX_S16 iG729Channels;
    
    /** Flag for Post Filter mode */
    OMX_S16 iPostFilt;
    
    /** Flag for DASF mode */
    OMX_S16 dasfmode;

    /** Flag for ACDN mode */
    OMX_S16 acdnmode;

    /** Writing pipe Used for DSP_RENDERING_ON */
    int fdwrite;

    /** Reading pipe Used for DSP_RENDERING_ON */   
    int fdread;

    /* ID stream ID*/
    OMX_U32 streamID;
 
    /* Flag for Port Defs Allocated*/
    OMX_U32 bPortDefsAllocated;
    
    /* Flag for Component Thread Started*/
    OMX_U32 bCompThreadStarted;

    /* Flag for Component Thread Stopping*/
    OMX_U32 bCompThreadStop;
    
    /** Mark data */
    OMX_PTR pMarkData;   
    
    /** Mark buffer */       
    OMX_MARKTYPE *pMarkBuf;
    
    /** Mark target component */    
    OMX_HANDLETYPE hMarkTargetComponent; 
   
    /** Flag set when buffer should not be queued to the DSP */
    OMX_U32 bBypassDSP;
   
    /** Input buffer list */
    G729DEC_BUFFERLIST *pInputBufferList;
   
    /** Output buffer list */
    G729DEC_BUFFERLIST *pOutputBufferList;
   
    /** LCML stream attributes */
    LCML_STRMATTR *strmAttr;
   
    /** Component version */
    OMX_U32 nVersion;
   
    /** Play Complete Flag */
    OMX_U32 bPlayCompleteFlag;
   
    /** Number of Bytes holding to be sent*/ 
    OMX_U32 nHoldLength;

    /** Pointer to the data holding to be sent*/ 
    OMX_U8* pHoldBuffer;
   
    /** Flag set when LCML handle is opened */
    OMX_S16 bLcmlHandleOpened;
   
    /** Keeps track of the number of nFillThisBufferCount() calls */
    OMX_U32 nFillThisBufferCount;
   
    /** Keeps track of the number of nFillBufferDoneCount() calls */  
    OMX_U32 nFillBufferDoneCount;

    /** Keeps track of the number of nEmptyThisBufferCount() calls */    
    OMX_U32 nEmptyThisBufferCount;

    /** Keeps track of the number of nEmptyBufferDoneCount() calls */        
    OMX_U32 nEmptyBufferDoneCount;

    /** Parameters for the Audio Codec */        
    G729DEC_AudioCodecParams *pParams;
   
    /** Flag for Init Params Initialized */          
    OMX_U32 bInitParamsInitialized;
   
    /** Flag for bIdleCommandPending */  
    OMX_U32 bIdleCommandPending;
   
    /** Array of Input Buffers that are pending to sent due State = Idle */   
    OMX_BUFFERHEADERTYPE *pInputBufHdrPending[MAX_NUM_OF_BUFS];
   
    /** Number of Input Buffers that are pending to sent due State = Idle */   
    OMX_U32 nNumInputBufPending;
   
    /** Array of Output Buffers that are pending to sent due State = Idle */   
    OMX_BUFFERHEADERTYPE *pOutputBufHdrPending[MAX_NUM_OF_BUFS];
   
    /** Number of Output Buffers that are pending to sent due State = Idle */      
    OMX_U32 nNumOutputBufPending;
   
    /** Flag for Reenabling Ports*/         
    OMX_U32 bJustReenabled;
   
    /** Flag for Invalid Frame Count*/         
    OMX_U32 nInvalidFrameCount;
   
    /** Flag for Writes While Paused   */
    OMX_U32 nDataWritesWhilePaused;
   
    /** Flag for bDisableCommandPending*/  
    OMX_U32 bDisableCommandPending;
    OMX_U32 bEnableCommandPending;
   
    /** Flag for bDisableCommandParam*/
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

    OMX_U32 nRuntimeInputBuffers;
    OMX_U32 nRuntimeOutputBuffers;

    OMX_STRING* sDeviceString;

    OMX_U32 nPacketsPerBuffer;

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

    /** Holds the value of RT Mixer mode  */
    OMX_U32 rtmx;

    OMX_BOOL bLoadedCommandPending;
    OMX_BOOL bFlushEventPending;

    OMX_PARAM_COMPONENTROLETYPE componentRole;

    /** Keep buffer timestamps **/
    OMX_S64 arrTimestamp[MAX_NUM_OF_BUFS];

    /** Keep buffer timestamps **/
    OMX_S64 arrTickCount[MAX_NUM_OF_BUFS];

    /** Index to arrBufIndex[], used for input buffer timestamps */
    OMX_U8 IpBufindex;

    /** Index to arrBufIndex[], used for output buffer timestamps */
    OMX_U8 OpBufindex;

    OMX_U8 nUnhandledFillThisBuffers;
    OMX_U8 nUnhandledEmptyThisBuffers;
    OMX_BOOL bFlushOutputPortCommandPending;
    OMX_BOOL bFlushInputPortCommandPending;

    /* array to hold buffer parameters */
    unsigned long int* bufParamsArray;

    OMX_BOOL bPreempted;

    /** Pointer to RM callback **/
#ifdef RESOURCE_MANAGER_ENABLED
    RMPROXY_CALLBACKTYPE rmproxyCallback;
#endif


#ifdef __PERF_INSTRUMENTATION__
    PERF_OBJHANDLE pPERF, pPERFcomp;
    OMX_U32 nLcml_nCntIp;         
    OMX_U32 nLcml_nCntOpReceived;
#endif
    
    
} G729DEC_COMPONENT_PRIVATE;

/* ===========================================================================*/
/** 
 *   Private data that application associates with buffer 
 */
/* ===========================================================================*/

typedef struct G729DEC_BufParamStruct
{
    OMX_U16 frameLost;      /* Lost frame flag from GPP */
    unsigned long int numPackets;
    unsigned long int packetLength[6]; 
    OMX_BOOL bNoUseDefaults; 
    
} G729DEC_BufParamStruct;


typedef enum OMX_G729DEC_INDEXAUDIOTYPE {
    OMX_IndexCustomG729DecModeAcdnConfig = 0xFF000001,
    OMX_IndexCustomG729DecModeDasfConfig,
    OMX_IndexCustomG729DecHeaderInfoConfig,
    OMX_IndexCustomG729DecDataPath
}OMX_G729DEC_INDEXAUDIOTYPE;


/*--------function prototypes ---------------------------------*/
#ifndef UNDER_CE

OMX_ERRORTYPE OMX_ComponentInit (OMX_HANDLETYPE hComp);

#else
/* =================================================================================== */
/**
 *   OMX_EXPORT                                           WinCE Implicit Export Syntax 
 */
/* ================================================================================== */
#define OMX_EXPORT __declspec(dllexport)

OMX_EXPORT OMX_ERRORTYPE OMX_ComponentInit (OMX_HANDLETYPE hComp);

#endif


OMX_ERRORTYPE G729DEC_StartComponentThread(OMX_HANDLETYPE pHandle);
OMX_ERRORTYPE G729DEC_StopComponentThread(OMX_HANDLETYPE pHandle);
OMX_ERRORTYPE G729DEC_FreeCompResources(OMX_HANDLETYPE pComponent);
void SendFlushCompleteEvent(G729DEC_COMPONENT_PRIVATE *pComponentPrivate, int port);

#ifdef RESOURCE_MANAGER_ENABLED
/***********************************
 *  Callback to the RM                                       *
 ***********************************/
void G729DEC_ResourceManagerCallback(RMPROXY_COMMANDDATATYPE cbData);
#endif

/*--------macros ----------------------------------------------*/
#ifndef UNDER_CE
#ifdef  G729DEC_DEBUG
#define G729DEC_DPRINT(...)    fprintf(stderr,__VA_ARGS__)
#define G729DEC_EPRINT(...)    fprintf(stderr,__VA_ARGS__)
#else
#define G729DEC_DPRINT(...)
#define G729DEC_EPRINT(...)
#endif

#ifdef  G729DEC_MEMCHECK
#define G729DEC_MEMPRINT(...)    fprintf(stderr,__VA_ARGS__)
#else
#define G729DEC_MEMPRINT(...)
#endif


#ifdef  G729DEC_DEBUG_MCP
#define G729DEC_MCP_DPRINT(...)    fprintf(stderr,__VA_ARGS__)
#else
#define G729DEC_MCP_DPRINT(...)
#endif

#ifdef  G729DEC_PRINT
#define G729DEC_PRINT_INFO(...)    fprintf(stderr,__VA_ARGS__)
#else
#define G729DEC_PRINT_INFO(...)
#endif


#else /*UNDER_CE*/
#ifdef DEBUG
#define G729DEC_DPRINT   printf
#define G729DEC_EPRINT   printf     
#define G729DEC_MEMPRINT   printf
#define G729DEC_PRINT_INFO printf
#else
#define G729DEC_DPRINT
#define G729DEC_EPRINT      
#define G729DEC_MEMPRINT
#define G729DEC_PRINT_INFO
#endif

#endif




#endif /* OMX_G729DECODER_H */
