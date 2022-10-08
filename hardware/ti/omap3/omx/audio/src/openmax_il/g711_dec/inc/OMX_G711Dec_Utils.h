
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
 * @file OMX_G711Dec_Utils.h
 *
 * This is an header file for an audio PCM decoder that is fully
 * compliant with the OMX Audio specification.
 * This the header file with the utils necesary to for the G711_DEC component.
 * in its code.
 *
 * @path $(CSLPATH)\
 *
 * @rev 0.1
 */
/* --------------------------------------------------------------------------- */

#ifndef OMX_G711DEC_UTILS__H
#define OMX_G711DEC_UTILS__H

#include "LCML_DspCodec.h"
#include <OMX_Component.h>
#include <pthread.h>

#ifdef RESOURCE_MANAGER_ENABLED
#include <ResourceManagerProxyAPI.h>
#endif


/* ======================================================================= */
/**
 * @def    G711DEC_MAJOR_VER              Define value for "major" version
 */
/* ======================================================================= */
#define  G711DEC_MAJOR_VER 0xF1

/* ======================================================================= */
/**
 * @def    G711DEC_MINOR_VER              Define value for "minor" version
 */
/* ======================================================================= */
#define  G711DEC_MINOR_VER 0xF2

/* ======================================================================= */
/**
 * @def    NOT_USED                            Define a not used value
 */
/* ======================================================================= */
#define NOT_USED 10

/* ======================================================================= */
/**
 * @def    NORMAL_BUFFER                       Define a normal buffer value
 */
/* ======================================================================= */
#define NORMAL_BUFFER 0

/* ======================================================================= */
/**
 * @def    OMX_G711DEC_DEFAULT_SEGMENT        Define the default segment
 */
/* ======================================================================= */
#define OMX_G711DEC_DEFAULT_SEGMENT (0)

/* ======================================================================= */
/**
 * @def    OMX_G711DEC_SN_TIMEOUT            Define a value for SN Timeout
 */
/* ======================================================================= */
#define OMX_G711DEC_SN_TIMEOUT (-1)

/* ======================================================================= */
/**
 * @def    OMX_G711DEC_SN_PRIORITY           Define a value for SN Priority
 */
/* ======================================================================= */
#define OMX_G711DEC_SN_PRIORITY (10)

/* ======================================================================= */
/**
 * @def    OMX_G711DEC_CPU   TBD, 50MHz for the moment
 */
/* ======================================================================= */
#define OMX_G711DEC_CPU (50)

/* ======================================================================= */
/**
 * @def    G711DEC_USN_DLL_NAME             Path & Name of USN DLL to be used
 *                                           at initialization
 */
/* ======================================================================= */
#ifdef UNDER_CE
#define G711DEC_USN_DLL_NAME "\\windows\\usn.dll64P"
#else
#define G711DEC_USN_DLL_NAME "usn.dll64P"
#endif

/* ======================================================================= */
/**
 * @def    G711DEC_USN_DLL_NAME             Path & Name of DLL to be useda
 *                                           at initialization
 */
/* ======================================================================= */
#ifdef UNDER_CE
#define G711DEC_DLL_NAME "\\windows\\g711dec_sn.dll64P"
#else
#define G711DEC_DLL_NAME "g711dec_sn.dll64P"
#endif



/* ======================================================================= */
/**
 * @def    EXTRA_BUFFBYTES                Num of Extra Bytes to be allocated
 */
/* ======================================================================= */
#define EXTRA_BUFFBYTES (256)

/* ======================================================================= */
/**
 * @def    G711D_TIMEOUT   Default timeout used to come out of blocking calls
 */
/* ======================================================================= */
#define G711D_TIMEOUT (1000) /* millisecs */

/* ======================================================================= */
/**
 * @def    NUM_G711DEC_INPUT_BUFFERS              Number of Input Buffers    
 */
/* ======================================================================= */
#define NUM_G711DEC_INPUT_BUFFERS 1

/* ======================================================================= */
/**
 * @def    NUM_G711DEC_OUTPUT_BUFFERS              Number of Output Buffers    
 */
/* ======================================================================= */
#define NUM_G711DEC_OUTPUT_BUFFERS 1

/* ======================================================================= */
/**
 * @def    NUM_G711DEC_OUTPUT_BUFFERS_DASF         Number of Output Buffers
 *                                                  on DASF mode
 */
/* ======================================================================= */
#define NUM_G711DEC_OUTPUT_BUFFERS_DASF 2

/* ======================================================================= */
/**
 * @def    OUTPUT_G711DEC_BUFFER_SIZE           Standart Output Buffer Size
 */
/* ======================================================================= */
#define OUTPUT_G711DEC_BUFFER_SIZE 160

/* ======================================================================= */
/**
 * @def    INPUT_G711DEC_BUFFER_SIZE             Standart Input Buffer Size
 *                                                
 */
/* ======================================================================= */
#define INPUT_G711DEC_BUFFER_SIZE 80
/* ========================================================================== */
/**
 * @def    RTP_Framesize                          Size in Bytes of determined
 *                                               frame. Not change it
 */
/* ========================================================================== */
#define RTP_Framesize 80
/* ========================================================================== */
/**
 * @def    STD_GSMFRDEC_BUF_SIZE                  Standart Input Buffer Size
 */
/* ========================================================================== */
#define STD_G711DEC_BUF_SIZE 80

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
 * @def    MAX_NUM_OF_BUFS                      Max Num of Bufs Allowed   
 */
/* ======================================================================= */
#define MAX_NUM_OF_BUFS 10
/*========================================================================*/
/** DEBUG PRINT's MACROS
 *@Memory check; printf's; component debug                                                           */
/*========================================================================*/
#undef G711DEC_DEBUG
#undef G711DEC_PRINT
#undef G711DEC_MEMCHECK
/*========================================================================*/
#define EXIT_COMPONENT_THRD  10
void* ComponentThread (void* pThreadData);
/*======================================================================*/
#ifndef UNDER_CE
#ifdef  G711DEC_DEBUG
#define G711DEC_DPRINT(...)    fprintf(stderr,__VA_ARGS__)
#else
#define G711DEC_DPRINT(...)
#endif

#ifdef  G711DEC_MEMCHECK
#define G711DEC_MEMPRINT(...)    fprintf(stderr,__VA_ARGS__)
#else
#define G711DEC_MEMPRINT(...)
#endif


#ifdef  G711DEC_DEBUG_MCP
#define G711DEC_MCP_DPRINT(...)    fprintf(stderr,__VA_ARGS__)
#else
#define G711DEC_MCP_DPRINT(...)
#endif

#else /*UNDER_CE*/
#ifdef  G711DEC_DEBUG
#define G711DEC_DPRINT(STR, ARG...) printf()
#else
#endif

#ifdef G711DEC_MEMCHECK
#define G711DEC_MEMPRINT(STR, ARG...) printf()
#else
#endif
#ifdef UNDER_CE

#ifdef DEBUG
#define G711DEC_DPRINT   printf
#define G711DEC_MEMPRINT   printf
#else
#define G711DEC_DPRINT
#define G711DEC_MEMPRINT
#endif

#endif  /*UNDER_CE*/

#endif
/**************************************************************/
#ifdef  G711DEC_PRINT
#define G711DEC_PRINT(...)    printf(stderr,__VA_ARGS__)
#else
#define G711DEC_PRINT(...)
#endif

/* ======================================================================= */
/**
 * @def    WMADEC_MEMDEBUG   Enable memory leaks debuf info
 */
/* ======================================================================= */
#undef G711DEC_MEMDEBUG 

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
/**
 *  M A C R O S FOR MALLOC and MEMORY FREE and CLOSING PIPES
 */
/* ======================================================================= */

#define OMX_NBCONF_INIT_STRUCT(_s_, _name_)     \
    memset((_s_), 0x0, sizeof(_name_));         \
    (_s_)->nSize = sizeof(_name_);              \
    (_s_)->nVersion.s.nVersionMajor = 0x1;      \
    (_s_)->nVersion.s.nVersionMinor = 0x0;      \
    (_s_)->nVersion.s.nRevision = 0x0;          \
    (_s_)->nVersion.s.nStep = 0x0

#define G711D_OMX_MALLOC(_pStruct_, _sName_)                        \
    _pStruct_ = (_sName_*)newmalloc(sizeof(_sName_));               \
    if(_pStruct_ == NULL){                                          \
        printf("***********************************\n");            \
        printf("%d :: Malloc Failed\n",__LINE__);                   \
        printf("***********************************\n");            \
        eError = OMX_ErrorInsufficientResources;                    \
        goto EXIT;                                                  \
    }                                                               \
    memset(_pStruct_,0,sizeof(_sName_));                            \
    G711DEC_MEMPRINT("%d :: Malloced = %p\n",__LINE__,_pStruct_);
    
#define G711D_OMX_MALLOC_SIZE(_ptr_, _size_,_name_)             \
    _ptr_ = (_name_ *)newmalloc(_size_);                        \
    if(_ptr_ == NULL){                                          \
        printf("***********************************\n");        \
        printf("%d :: Malloc Failed\n",__LINE__);               \
        printf("***********************************\n");        \
        eError = OMX_ErrorInsufficientResources;                \
        goto EXIT;                                              \
    }                                                           \
    memset(_ptr_,0,_size_);                                     \
    G711DEC_MEMPRINT("%d :: Malloced = %p\n",__LINE__,_ptr_);

#define OMX_G711DECMEMFREE_STRUCT(_pStruct_)                    \
    if(_pStruct_ != NULL){                                      \
    G711DEC_MEMPRINT("%d :: [FREE] %p\n",__LINE__,_pStruct_);   \
        newfree(_pStruct_);                                     \
        _pStruct_ = NULL;                                       \
    }

#define OMX_NBCLOSE_PIPE(_pStruct_,err)                         \
    G711DEC_DPRINT("%d :: CLOSING PIPE \n",__LINE__);           \
    err = close (_pStruct_);                                    \
    if(0 != err && OMX_ErrorNone == eError){                    \
        eError = OMX_ErrorHardware;                             \
        printf("%d :: Error while closing pipe\n",__LINE__);    \
        goto EXIT;                                              \
    }

/* ======================================================================= */
/** G711DEC_COMP_PORT_TYPE  Port Type
 *
 *  @param  G711DEC_INPUT_PORT                   Port Type Input
 *
 *  @param  G711DEC_OUTPUT_PORT                  Port Type Output
 *
 */
/*  ==================================================================== */
typedef enum G711DEC_COMP_PORT_TYPE {
    G711DEC_INPUT_PORT = 0,
    G711DEC_OUTPUT_PORT
}G711DEC_COMP_PORT_TYPE;

/* ======================================================================= */
/** G711DEC_StreamType  StreamType
 *
 *  @param  G711DEC_DMM                  Stream Type DMM
 *
 *  @param  G711DEC_INSTRM               Stream Type Input
 *
 *  @param  G711DEC_OUTSTRM             Stream Type Output
 */
/*  ==================================================================== */
enum G711DEC_StreamType
    {
        G711DEC_DMM,
        G711DEC_INSTRM,
        G711DEC_OUTSTRM
    };

/* ======================================================================= */
/** G711DEC_BUFFER_Dir  Direction of the Buffer
 *
 *  @param  G711DEC_DIRECTION_INPUT              Direction Input
 *
 *  @param  G711DEC_DIRECTION_INPUT              Direction Output
 */
/*  ==================================================================== */
typedef enum {
    G711DEC_DIRECTION_INPUT,
    G711DEC_DIRECTION_OUTPUT
}G711DEC_BUFFER_Dir;


/* =================================================================================== */
/** 
 * 
 */
/* ================================================================================== */
typedef struct G711DEC_FTYPES{
    OMX_S16     FrameSizeType;
    OMX_S16     NmuNLvl;
    OMX_S16     NoiseLp;
    OMX_S16     dBmNoise;
    OMX_S16     plc;
}G711DEC_FTYPES;
/* =================================================================================== */
/**
 * Socket node input parameters.
 */
/* ================================================================================== */
typedef struct G711DEC_AudioCodecParams
{
    unsigned long iSamplingRate;
    unsigned long iStrmId;
    unsigned short iAudioFormat;
}G711DEC_AudioCodecParams;

/* =================================================================================== */
/**
 * Socket node alg parameters..
 */
/* ================================================================================== */
typedef struct {
    unsigned long usEndOfFile;
    unsigned long usFrameLost;
}G711DEC_UAlgInBufParamStruct;

/* ========================================================================== */
/**
 * Socket node alg parameters..
 * frameType @ Type of packaging per frame number
 * usLastFrame @ End of decoding
 */
/* ========================================================================== */
typedef struct G711DEC_FrameStruct{
    unsigned long   frameType; /* 0: voice frame (80 bytes), 1: SID frame (22 bytes), 2: No Data (0 bytes), 3: Frame lost */ 
    unsigned long   usLastFrame;
} G711DEC_FrameStruct;  

typedef struct G711DEC_ParamStruct{
    unsigned long int usNbFrames;
    G711DEC_FrameStruct *pParamElem;
} G711DEC_ParamStruct;

/* =================================================================================== */
/**
 * LCML_G711DEC_BUFHEADERTYPE
 */
/* ================================================================================== */
typedef struct LCML_G711DEC_BUFHEADERTYPE {
    G711DEC_BUFFER_Dir eDir;
    OMX_BUFFERHEADERTYPE* buffer;
    G711DEC_UAlgInBufParamStruct *pIpParam;
    G711DEC_FrameStruct *pFrameParam;
    G711DEC_ParamStruct *pBufferParam;
    DMM_BUFFER_OBJ* pDmmBuf;
}LCML_G711DEC_BUFHEADERTYPE;

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

OMX_ERRORTYPE G711DEC_StartComponentThread(OMX_HANDLETYPE pHandle);
OMX_ERRORTYPE G711DEC_StopComponentThread(OMX_HANDLETYPE pHandle);
OMX_ERRORTYPE G711DEC_FreeCompResources(OMX_HANDLETYPE pComponent);

typedef struct _G711DEC_BUFFERLIST G711DEC_BUFFERLIST;

/* =================================================================================== */
/**
 * Structure for buffer list
 */
/* ================================================================================== */
struct _G711DEC_BUFFERLIST{
    OMX_BUFFERHEADERTYPE *pBufHdr[MAX_NUM_OF_BUFS]; /* records buffer header send by client */ 
    OMX_U32 bufferOwner[MAX_NUM_OF_BUFS];
    OMX_U32 bBufferPending[MAX_NUM_OF_BUFS];
    OMX_U32 numBuffers; 
};

/* =================================================================================== */
/**
 * G711ENC_PORT_TYPE Structure for PortFormat details
 */
/* =================================================================================== */
typedef struct G711DEC_PORT_TYPE {
    OMX_HANDLETYPE hTunnelComponent;
    OMX_U32 nTunnelPort;
    OMX_BUFFERSUPPLIERTYPE eSupplierSetting;
    OMX_U8 nBufferCnt;
    OMX_AUDIO_PARAM_PORTFORMATTYPE* pPortFormat;
} G711DEC_PORT_TYPE;

OMX_ERRORTYPE OMX_DmmMap(DSP_HPROCESSOR ProcHandle, int size, 
                         void* pArmPtr, DMM_BUFFER_OBJ* pDmmBuf);
OMX_ERRORTYPE OMX_DmmUnMap(DSP_HPROCESSOR ProcHandle, void* pMapPtr, 
                           void* pResPtr);                       
/* =================================================================================== */
/**
 * Component private data
 */
/* ================================================================================== */
typedef struct G711DEC_COMPONENT_PRIVATE
{
#ifdef UNDER_CE
    OMX_BUFFERHEADERTYPE* pBufHeader[NUM_OF_PORTS]; 
#endif

    /** Structure of callback pointers */
    OMX_CALLBACKTYPE cbInfo;

    G711DEC_PORT_TYPE *pCompPort[NUM_OF_PORTS];

    /** Handle for use with async callbacks */
    OMX_PORT_PARAM_TYPE sPortParam;
    
    /** Input port parameters */
    OMX_AUDIO_PARAM_PORTFORMATTYPE sInPortFormat;
    
    /** Output port parameters */
    OMX_AUDIO_PARAM_PORTFORMATTYPE sOutPortFormat;

    /** This will contain info like how many buffers
        are there for input/output ports, their size etc, but not
        BUFFERHEADERTYPE POINTERS. */
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef[NUM_OF_PORTS];
    
    /** G711 Component Parameters */
    OMX_AUDIO_PARAM_PCMMODETYPE* g711Params[NUM_OF_PORTS];

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

    /** Flag set when the EOS marker is sent */
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
    LCML_G711DEC_BUFHEADERTYPE *pLcmlBufHeader[2];
    
    /** Flag for G711 mode */
    OMX_S16 iG711Mode;
    
    /** Flag for DASF mode */
    OMX_S16 dasfmode;
    
    /** Flag for ACDN mode */
    OMX_S16 acdnmode;
    
    /** Flag for frame size type mode */
    OMX_S16 fsizemode;
    /** Flag for fram type mode */
    OMX_S16 ftype;
    /** Flag for Noise Level NMU */
    OMX_S16 nmulevel;
    /** Flag for Noise LP  */
    OMX_S16 noiselp;
    /** Flag for Noise level in dBm */
    OMX_S16 dbmnoise;
    /** Flag for Noise level in dBm */
    OMX_S16 packetlostc;
    
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
    
    /** Mark data */
    OMX_PTR pMarkData;   
    
    /** Mark buffer */       
    OMX_MARKTYPE *pMarkBuf;
    
    /** Mark target component */    
    OMX_HANDLETYPE hMarkTargetComponent; 
   
    /** Flag set when buffer should not be queued to the DSP */
    OMX_U32 bBypassDSP;
   
    /** Input buffer list */
    G711DEC_BUFFERLIST *pInputBufferList;
   
    /** Output buffer list */
    G711DEC_BUFFERLIST *pOutputBufferList;
   
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
    G711DEC_AudioCodecParams *pParams;
   
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
   
    /** Flag for bDisableCommandParam*/
    OMX_U32 bDisableCommandParam;

    /** Flag for Enabling the port*/  
    OMX_U32 bEnableCommandPending;
   
    /** Flag for Enabling the port*/
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

    OMX_BOOL bLoadedCommandPending;

    OMX_PARAM_COMPONENTROLETYPE componentRole;

    /* Device string */
    OMX_STRING* sDeviceString;
    /* Removing sleep() calls. Definition. */
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
    /**************************/
    OMX_U8 nUnhandledFillThisBuffers;

    /**Keep buffer tickcount*/
    OMX_U32 arrBufIndexTick[MAX_NUM_OF_BUFS];

    /** Keep buffer timestamps **/
    OMX_U32 arrBufIndex[MAX_NUM_OF_BUFS];

    /** Index to arrBufIndex[], used for input buffer timestamps */
    OMX_U8 IpBufindex;
    
    /** Index to arrBufIndex[], used for output buffer timestamps */
    OMX_U8 OpBufindex;

    /** Number of input buffers at runtime **/
    OMX_U32 nRuntimeInputBuffers;

    /** Pointer to RM callback **/
#ifdef RESOURCE_MANAGER_ENABLED
    RMPROXY_CALLBACKTYPE rmproxyCallback;
#endif

    OMX_BOOL bPreempted;

    
} G711DEC_COMPONENT_PRIVATE;

typedef enum OMX_G711DEC_INDEXAUDIOTYPE {
    OMX_IndexCustomG711DecModeAcdnConfig = 0xFF000001,
    OMX_IndexCustomG711DecModeDasfConfig,
    OMX_IndexCustomG711DecHeaderInfoConfig,
    OMX_IndexCustomG711DecFrameParams,
    OMX_IndexCustomG711DecDataPath
}OMX_G711DEC_INDEXAUDIOTYPE;

#ifdef RESOURCE_MANAGER_ENABLED
/*========================================================================*/
/** RESOURCE MANAGER CALLBACK                                                                    */
/*=======================================================================*/
void G711DEC_ResourceManagerCallback(RMPROXY_COMMANDDATATYPE cbData);
/* ========================================================================*/
#endif
 
OMX_ERRORTYPE G711DECGetCorresponding_LCMLHeader(OMX_U8 *pBuffer,
                                                 OMX_DIRTYPE eDir,
                                                 LCML_G711DEC_BUFHEADERTYPE **ppLcmlHdr);
/***********CALLBACK EVENT TO SN**********/
OMX_ERRORTYPE G711DECLCML_Callback (TUsnCodecEvent event,void * args [10]);

OMX_ERRORTYPE G711DECFill_LCMLInitParams(OMX_HANDLETYPE pHandle,
                                         LCML_DSP *plcml_Init,OMX_U16 arr[]);

OMX_ERRORTYPE G711DECGetBufferDirection(OMX_BUFFERHEADERTYPE *pBufHeader, OMX_DIRTYPE *eDir);

OMX_U32 G711DECHandleCommand (G711DEC_COMPONENT_PRIVATE *pComponentPrivate);

OMX_ERRORTYPE G711DECHandleDataBuf_FromApp(OMX_BUFFERHEADERTYPE *pBufHeader,
                                           G711DEC_COMPONENT_PRIVATE *pComponentPrivate);

OMX_ERRORTYPE G711DECHandleDataBuf_FromLCML(G711DEC_COMPONENT_PRIVATE* pComponentPrivate, 
                                            LCML_G711DEC_BUFHEADERTYPE* msgBuffer);

void  AddHeader(BYTE **pFileBuf);
void  ResetPtr(BYTE **pFileBuf);

OMX_HANDLETYPE G711DECGetLCMLHandle();
OMX_ERRORTYPE G711DECFreeLCMLHandle();
OMX_ERRORTYPE G711DEC_CleanupInitParams(OMX_HANDLETYPE pComponent);

void G711DEC_SetPending(G711DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir);
void G711DEC_ClearPending(G711DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir) ;

OMX_U32 G711DEC_IsPending(G711DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir);
OMX_ERRORTYPE G711DECFill_LCMLInitParamsEx(OMX_HANDLETYPE pComponent);
OMX_U32 G711DEC_IsValid(G711DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U8 *pBuffer, OMX_DIRTYPE eDir) ;
OMX_ERRORTYPE G711DEC_TransitionToIdle(G711DEC_COMPONENT_PRIVATE *pComponentPrivate);

#endif /* OMX_G711DECODER_H*/
