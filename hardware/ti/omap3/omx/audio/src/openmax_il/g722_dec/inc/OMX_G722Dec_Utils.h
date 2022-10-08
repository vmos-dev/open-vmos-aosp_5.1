
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
 * @file OMX_G722Decoder.h
 *
 * This is an header file for an audio G722 decoder that is fully
 * compliant with the OMX Audio specification.
 * This the file is used internally by the component
 * in its code.
 *
 * @path  $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\g722_dec\inc\
 *
 * @rev 1.0
 */
/* --------------------------------------------------------------------------- */
#ifndef OMX_G722DEC_UTILS__H
#define OMX_G722DEC_UTILS__H

#include <OMX_Component.h>

#include "LCML_DspCodec.h"

    #ifdef UNDER_CE
#include <windows.h>
#include <oaf_osal.h>
#include <omx_core.h>
#include <stdlib.h>
#else
#include <pthread.h>
#endif

/*#define G722DEC_DEBUG  **/         /* See all debug statement of the component */
/*#define G722DEC_MEMDETAILS */     /* See memory details of the component */
/*#define G722DEC_BUFDETAILS */     /* See buffers details of the component */
/*#define G722DEC_STATEDETAILS */   /* See all state transitions of the component */

#define MAX_NUM_OF_BUFS 10 /* Max number of buffers used */
#define G722D_NUM_INPUT_BUFFERS 1  /* Default number of input buffers */
#define G722D_NUM_OUTPUT_BUFFERS 1 /* Default number of output buffers */
#define G722D_INPUT_BUFFER_SIZE  4096 /* Default size of input buffer */
#define G722D_OUTPUT_BUFFER_SIZE 320 /* Default size of output buffer */

#define NUM_OF_PORTS 2 /* Number of ports of component */
#define INVALID_SAMPLING_FREQ  51

#define EXIT_COMPONENT_THRD  10


#define G722DEC_MAJOR_VER           0xF1 /* Majer version of the component */
#define G722DEC_MINOR_VER           0xF2 /* Minor version of the component */
#define G722DEC_BUFHEADER_VERSION   0x0 /* Version of the buffer header struct */
#define NOT_USED                    10 /* To specify that a particulat field is not used */
#define NORMAL_BUFFER               0 /* Marks a buffer as normal buffer i.e. not last buffer */
#define OMX_G722DEC_DEFAULT_SEGMENT (0) /* Default segment ID */
#define OMX_G722DEC_SN_TIMEOUT      (-1) /* timeout, wait until ack is received */
#define OMX_G722DEC_SN_PRIORITY     (10) /* Priority used by DSP */
#define G722DEC_CPU                 50 /* TBD, 50MHz for the moment */

#ifdef UNDER_CE
#define USN_DLL_NAME "\\windows\\usn.dll64P" /* Path of USN DLL */
#define G722DEC_DLL_NAME "\\windows\\g722dec_sn.dll64P" /* Path of G722 SN DLL */
#else
#define USN_DLL_NAME "usn.dll64P" /* Path of USN DLL */
#define G722DEC_DLL_NAME "g722dec_sn.dll64P" /* Path of G722 SN DLL */
#endif

#define DONT_CARE 0 /* Value unused or ignored */

/** Default timeout used to come out of blocking calls*/
#define G722D_TIMEOUT (1000) /* millisecs */

#ifdef UNDER_CE

#ifdef DEBUG
#define G722DEC_DPRINT       printf
#define G722DEC_EPRINT       printf
#define G722DEC_MEMPRINT     printf
#define G722DEC_STATEPRINT   printf
#define G722DEC_BUFPRINT     printf
#else
#define G722DEC_DPRINT
#define G722DEC_EPRINT
#define G722DEC_MEMPRINT
#define G722DEC_STATEPRINT
#define G722DEC_BUFPRINT
#endif

#else /* for Linux */

#ifdef  G722DEC_DEBUG

#define G722DEC_DPRINT(...)  fprintf(stdout, "%s %d::  ",__FUNCTION__, __LINE__); \
    fprintf(stdout, __VA_ARGS__);                                       \
    fprintf(stdout, "\n");

#define G722DEC_BUFPRINT printf
#define G722DEC_MEMPRINT printf
#define G722DEC_STATEPRINT printf

#else
#define G722DEC_DPRINT(...)

#ifdef G722DEC_STATEDETAILS
#define G722DEC_STATEPRINT printf
#else
#define G722DEC_STATEPRINT(...)
#endif

#ifdef G722DEC_BUFDETAILS
#define G722DEC_BUFPRINT printf
#else
#define G722DEC_BUFPRINT(...)
#endif

#ifdef G722DEC_MEMDETAILS
#define G722DEC_MEMPRINT(...)  fprintf(stdout, "%s %d::  ",__FUNCTION__, __LINE__); \
    fprintf(stdout, __VA_ARGS__);                                       \
    fprintf(stdout, "\n");
#else
#define G722DEC_MEMPRINT(...)
#endif

#endif

#endif /*for UNDER_CE*/

#define G722D_OMX_MALLOC(_pStruct_, _sName_)                        \
    _pStruct_ = (_sName_*)malloc(sizeof(_sName_));                  \
    if(_pStruct_ == NULL){                                          \
        printf("***********************************\n");            \
        printf("%d :: Malloc Failed\n",__LINE__);                   \
        printf("***********************************\n");            \
        eError = OMX_ErrorInsufficientResources;                    \
        goto EXIT;                                                  \
    }                                                               \
    memset(_pStruct_,0,sizeof(_sName_));                            \
    G722DEC_MEMPRINT("%d :: Malloced = %p\n",__LINE__,_pStruct_);



#define G722D_OMX_MALLOC_SIZE(_ptr_, _size_,_name_)             \
    _ptr_ = (_name_ *)malloc(_size_);                           \
    if(_ptr_ == NULL){                                          \
        printf("***********************************\n");        \
        printf("%d :: Malloc Failed\n",__LINE__);               \
        printf("***********************************\n");        \
        eError = OMX_ErrorInsufficientResources;                \
        goto EXIT;                                              \
    }                                                           \
    memset(_ptr_,0,_size_);                                     \
    G722DEC_MEMPRINT("%d :: Malloced = %p\n",__LINE__,_ptr_);

#define G722D_OMX_ERROR_EXIT(_e_, _c_, _s_)                             \
    _e_ = _c_;                                                          \
    printf("\n**************** OMX ERROR ************************\n");  \
    printf("%d : Error Name: %s : Error Num = %x",__LINE__, _s_, _e_);  \
    printf("\n**************** OMX ERROR ************************\n");  \
    goto EXIT;


#define G722D_OMX_CONF_CHECK_CMD(_ptr1, _ptr2, _ptr3)   \
    {                                                   \
        if(!_ptr1 || !_ptr2 || !_ptr3){                 \
            eError = OMX_ErrorBadParameter;             \
            goto EXIT;                                  \
        }                                               \
    }

#define G722D_OMX_FREE(ptr)                                             \
    if(NULL != ptr) {                                                   \
        G722DEC_MEMPRINT("%d :: Freeing Address = %p\n",__LINE__,ptr);  \
        free(ptr);                                                      \
        ptr = NULL;                                                     \
    }

#define OMX_CONF_INIT_STRUCT(_s_, _name_)       \
    memset((_s_), 0x0, sizeof(_name_));         \
    (_s_)->nSize = sizeof(_name_);              \
    (_s_)->nVersion.s.nVersionMajor = 0x1;      \
    (_s_)->nVersion.s.nVersionMinor = 0x0;      \
    (_s_)->nVersion.s.nRevision = 0x0;          \
    (_s_)->nVersion.s.nStep = 0x0

    
/* ======================================================================= */
/** OMX_G722DEC_INDEXAUDIOTYPE  Defines the custom configuration settings
 *                              for the component
 *
 *  @param  OMX_IndexCustomG722DecModeDasfConfig      Sets the DASF mode
 *
 */
/*  ==================================================================== */
typedef enum OMX_G722DEC_INDEXAUDIOTYPE {
    /*OMX_IndexCustomG722DecModeDasfConfig = OMX_IndexIndexVendorStartUnused + 1,*/
    OMX_IndexCustomG722DecModeDasfConfig = 0xFF000001,
    OMX_IndexCustomG722DecHeaderInfoConfig,
    OMX_IndexCustomG722DecStreamIDConfig,
    OMX_IndexCustomG722DecDataPath
}OMX_G722DEC_INDEXAUDIOTYPE;


/* ============================================================================== * */
/** G722D_COMP_PORT_TYPE  describes the input and output port of indices of the
 * component.
 *
 * @param  G722D_INPUT_PORT  Input port index
 *
 * @param  G722D_OUTPUT_PORT Output port index
 */
/* ============================================================================ * */
typedef enum G722D_COMP_PORT_TYPE {
    G722D_INPUT_PORT = 0,
    G722D_OUTPUT_PORT
}G722D_COMP_PORT_TYPE;


/* ======================================================================= */
/** G722DEC_ParamStruct: This struct is passed with input buffers that
 * are sent to DSP.
 */
/* ==================================================================== */
typedef struct {
    /* Set to 1 if buffer is last buffer */
    unsigned long usLastFrame;
    unsigned long usFrameLost;
}G722DEC_ParamStruct;

/* ======================================================================= */
/** G722DEC_UAlgInBufParamStruct: This struct is passed with input buffers that
 * are sent to DSP.
 */
/* ==================================================================== */
typedef struct {
    /* Set to 1 if buffer is last buffer */
    unsigned long bLastBuffer;
}G722DEC_UAlgInBufParamStruct;


/* ======================================================================= */
/** G722D_USN_AudioCodecParams: This contains the information which does to Codec
 * on DSP
 * are sent to DSP.
 */
/* ==================================================================== */
typedef struct USN_AudioCodecParams{
    /* Specifies the sample frequency */
    unsigned long ulSamplingFreq;
    /* Specifies the UUID */
    unsigned long unUUID;
    /* Specifies the audio format */
    unsigned short unAudioFormat;
}G722D_USN_AudioCodecParams;

/* ======================================================================= */
/** G722DEC_UAlgOutBufParamStruct: This is passed with output buffer to DSP.
 */
/* ==================================================================== */
typedef struct {
    /* Number of frames in a buffer */
    unsigned long ulFrameCount;
}G722DEC_UAlgOutBufParamStruct;

/* ======================================================================= */
/** G722D_LCML_BUFHEADERTYPE: This is LCML buffer header which is sent to LCML
 * for both input and output buffers.
 */
/* ==================================================================== */
typedef struct G722D_LCML_BUFHEADERTYPE {
    /* Direction whether input or output buffer */
    OMX_DIRTYPE eDir;
    /* Pointer to OMX Buffer Header */
    OMX_BUFFERHEADERTYPE *pBufHdr;
    /* Other parameters, may be useful for enhancements */
    void *pOtherParams[10];
    /* Input Parameter Information structure */
    /*G722DEC_UAlgInBufParamStruct *pIpParam;*/
    G722DEC_ParamStruct *pIpParam;
    /* Output Parameter Information structure */
    G722DEC_UAlgOutBufParamStruct *pOpParam;

    OMX_BUFFERHEADERTYPE* buffer;
}G722D_LCML_BUFHEADERTYPE;

/* ======================================================================= */
/** G722D_AUDIODEC_PORT_TYPE: This contains component port information.
 *
 * @see OMX_AUDIO_PARAM_PORTFORMATTYPE
 */
/* ==================================================================== */
typedef struct AUDIODEC_PORT_TYPE {
    /* Used in tunneling, this is handle of tunneled component */
    OMX_HANDLETYPE hTunnelComponent;
    /* Port which has to be tunneled */
    OMX_U32 nTunnelPort;
    /* Buffer Supplier Information */
    OMX_BUFFERSUPPLIERTYPE eSupplierSetting;
    /* Number of buffers */
    OMX_U8 nBufferCnt;
    /* Port format information */
    OMX_AUDIO_PARAM_PORTFORMATTYPE* pPortFormat;
} G722D_AUDIODEC_PORT_TYPE;


/* ======================================================================= */
/** _G722D_BUFFERLIST: This contains information about a buffer's owner whether
 * it is application or component, number of buffers owned etc.
 *
 * @see OMX_BUFFERHEADERTYPE
 */
/* ==================================================================== */
struct _G722D_BUFFERLIST{
    /* Array of pointer to OMX buffer headers */
    OMX_BUFFERHEADERTYPE *pBufHdr[MAX_NUM_OF_BUFS];
    /* Array that tells about owner of each buffer */
    OMX_U32 bufferOwner[MAX_NUM_OF_BUFS];
    OMX_U32 bBufferPending[MAX_NUM_OF_BUFS];
    /* Number of buffers  */
    OMX_U32 numBuffers;
};

typedef struct _G722D_BUFFERLIST G722D_BUFFERLIST;

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
/** G722DEC_COMPONENT_PRIVATE: This is the major and main structure of the
 * component which contains all type of information of buffers, ports etc
 * contained in the component.
 *
 * @see OMX_BUFFERHEADERTYPE
 * @see OMX_AUDIO_PARAM_PORTFORMATTYPE
 * @see OMX_PARAM_PORTDEFINITIONTYPE
 * @see G722D_LCML_BUFHEADERTYPE
 * @see OMX_PORT_PARAM_TYPE
 * @see OMX_PRIORITYMGMTTYPE
 * @see G722D_AUDIODEC_PORT_TYPE
 * @see G722D_BUFFERLIST
 * @see G722D_AUDIODEC_PORT_TYPE
 * @see LCML_STRMATTR
 * @see
 */
/* ==================================================================== */
typedef struct G722DEC_COMPONENT_PRIVATE
{
    /** Handle for use with async callbacks */
    OMX_CALLBACKTYPE cbInfo;
    /* Component port information */
    OMX_PORT_PARAM_TYPE *sPortParam;
    /* Input port information */
    OMX_AUDIO_PARAM_PORTFORMATTYPE sInPortFormat;
    /* Output port information */
    OMX_AUDIO_PARAM_PORTFORMATTYPE sOutPortFormat;
    /* Buffer owner information */
    OMX_U32 bIsBufferOwned[NUM_OF_PORTS];

    /** Number of input buffers at runtime */
    OMX_U32 nRuntimeInputBuffers;

    /** Number of output buffers at runtime */
    OMX_U32 nRuntimeOutputBuffers;

    /* Audio codec parameters structure */
    G722D_USN_AudioCodecParams *pParams;

    /** This will contain info like how many buffers
        are there for input/output ports, their size etc, but not
        BUFFERHEADERTYPE POINTERS. */
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef[NUM_OF_PORTS];
    /* Contains information that come from application */
    OMX_AUDIO_PARAM_PCMMODETYPE* G722Params[NUM_OF_PORTS];
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

    /** The pipes for sending command data to the thread */
    int cmdDataPipe[2];

    /*    int portcmdPipe[2]; */

    /** The pipes for sending buffers to the thread */
    int lcml_Pipe[2];

    /** Set to indicate component is stopping */
    OMX_U32 bIsEOFSent;

    /** Count of number of buffers outstanding with bridge */
    OMX_U32 lcml_nIpBuf;

    /** Count of number of buffers outstanding with bridge */
    OMX_U32 lcml_nOpBuf;

    /** Counts of number of input buffers sent to LCML */
    OMX_U32 lcml_nCntIp;
    /** Counts of number of input buffers received from LCML */
    OMX_U32 lcml_nCntIpRes;
    /** Counts of number of output buffers sent to LCML */
    OMX_U32 lcml_nCntOp;
    /** Counts of number of output buffers received from LCML */
    OMX_U32 lcml_nCntOpReceived;
    /** Counts of number of buffers sent to App  */
    OMX_U32 lcml_nCntApp;
    /** Counts of number of buffers received from App  */
    OMX_U32 app_nBuf;

    /** Counts of number of output buffers reclaimed from lcml  */
    OMX_U32 num_Reclaimed_Op_Buff;
    /** Counts of number of input buffers sent to lcml  */
    OMX_U32 num_Sent_Ip_Buff;
    /** Counts of number of output buffers sent to lcml  */
    OMX_U32 num_Op_Issued;
    /** Holds the value of dasf mode, 1: DASF mode or 0: File Mode  */
    OMX_U32 dasfmode;

    OMX_STRING* sDeviceString;

    /** This is LCML handle  */
    OMX_HANDLETYPE pLcmlHandle;

    /** Contains pointers to LCML Buffer Headers */
    G722D_LCML_BUFHEADERTYPE *pLcmlBufHeader[2];
    OMX_U32 bBufferIsAllocated;

    /** Tells whether buffers on ports have been allocated */
    OMX_U32 bPortDefsAllocated;
    /** Tells whether component thread has started */
    OMX_U32 bCompThreadStarted;
    /** Marks the buffer data  */
    OMX_PTR pMarkData;
    /** Marks the buffer */
    OMX_MARKTYPE *pMarkBuf;
    /** Marks the target component */
    OMX_HANDLETYPE hMarkTargetComponent;
    /** Flag to track when input buffer's filled length is 0 */
    OMX_U32 bBypassDSP;
    /** Input port enable flag */
    int ipPortEnableFlag;
    /** Input port disble flag */
    int ipPortDisableFlag;
    /** Pointer to port parameter structure */
    OMX_PORT_PARAM_TYPE* pPortParamType;
    /** Pointer to port priority management structure */
    OMX_PRIORITYMGMTTYPE* pPriorityMgmt;
    /** Contains the port related info of both the ports */
    G722D_AUDIODEC_PORT_TYPE *pCompPort[NUM_OF_PORTS];
    /* Checks whether or not buffer were allocated by appliction */
    int bufAlloced;
    /** Flag to check about execution of component thread */
    OMX_U16 bExitCompThrd;
    /** Pointer to list of input buffers */
    G722D_BUFFERLIST *pInputBufferList;
    /** Pointer to list of output buffers */
    G722D_BUFFERLIST *pOutputBufferList;
    /** it is used for component's create phase arguments */
    LCML_STRMATTR  *strmAttr;
    /** Contains the version information */
    OMX_U32 nVersion;

    /** ACDN mode flag */
    OMX_U32 acdnmode;

    /** Audio Stream ID */
    OMX_U32 streamID;

    int nOpBit;
    int bLcmlHandleOpened;
    OMX_U32 nFillThisBufferCount;
    /** Counts number of FillBufferDone calls*/
    OMX_U32 nFillBufferDoneCount;
    /** Counts number of EmptyThisBuffer calls*/
    OMX_U32 nEmptyThisBufferCount;
    /** Counts number of EmptyBufferDone calls*/
    OMX_U32 nEmptyBufferDoneCount;
    /** Checks if component Init Params have been initialized */
    OMX_U32 bInitParamsInitialized;
    G722D_BUFFERLIST *pInputBufferListQueue;
    G722D_BUFFERLIST *pOutputBufferListQueue;
    OMX_BUFFERHEADERTYPE *pInputBufHdrPending[MAX_NUM_OF_BUFS];
    OMX_U32 nNumInputBufPending;
    OMX_BUFFERHEADERTYPE *pOutputBufHdrPending[MAX_NUM_OF_BUFS];
    OMX_U32 nNumOutputBufPending;
    OMX_BUFFERHEADERTYPE *pOutBufHdrWhilePaused[MAX_NUM_OF_BUFS];
    OMX_U32 nPendingOutPausedBufs;

    OMX_U32 bDisableCommandPending;
    OMX_U32 bDisableCommandParam;
    OMX_U32 nInvalidFrameCount;
    OMX_U32 numPendingBuffers;
    OMX_U32 bNoIdleOnStop;
    OMX_U32 bDspStoppedWhileExecuting;
    OMX_U32 bIdleCommandPending;
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

    OMX_BOOL bLoadedCommandPending;
    OMX_PARAM_COMPONENTROLETYPE *componentRole;
    OMX_VERSIONTYPE ComponentVersion;
    OMX_STRING cComponentName;

    /** Keep buffer timestamps **/ 
    OMX_S64 arrTimestamp[MAX_NUM_OF_BUFS]; 
    /** Keep buffer nTickCounts **/ 
    OMX_S64 arrTickCount[MAX_NUM_OF_BUFS]; 
    /** Index to arrTimestamp[], used for input buffer timestamps */ 
    OMX_U8 IpBufindex; 
    /** Index to arrTimestamp[], used for output buffer timestamps */ 
    OMX_U8 OpBufindex; 
} G722DEC_COMPONENT_PRIVATE;



/* ================================================================================= * */
/**
 * OMX_ComponentInit() function is called by OMX Core to initialize the component
 * with default values of the component. Before calling this function OMX_Init
 * must have been called.
 *
 * @param *hComp This is component handle allocated by the OMX core.
 *
 * @pre          OMX_Init should be called by application.
 *
 * @post         Component has initialzed with default values.
 *
 *  @return      OMX_ErrorNone = Successful Inirialization of the component\n
 *               OMX_ErrorInsufficientResources = Not enough memory
 *
 *  @see          G722Dec_StartCompThread()
 */
/* ================================================================================ * */
#ifndef UNDER_CE
OMX_ERRORTYPE OMX_ComponentInit (OMX_HANDLETYPE hComp);
#else
/*  WinCE Implicit Export Syntax */
#define OMX_EXPORT __declspec(dllexport)
OMX_EXPORT OMX_ERRORTYPE OMX_ComponentInit (OMX_HANDLETYPE hComp);
#endif

/* ================================================================================= * */
/**
 * G722Dec_StartCompThread() starts the component thread. This is internal
 * function of the component.
 *
 * @param pHandle This is component handle allocated by the OMX core.
 *
 * @pre          None
 *
 * @post         None
 *
 *  @return      OMX_ErrorNone = Successful Inirialization of the component\n
 *               OMX_ErrorInsufficientResources = Not enough memory
 *
 *  @see         None
 */
/* ================================================================================ * */
OMX_ERRORTYPE G722Dec_StartCompThread(OMX_HANDLETYPE pHandle);

/* ================================================================================= * */
/**
 * G722DEC_Fill_LCMLInitParams() fills the LCML initialization structure.
 *
 * @param pHandle This is component handle allocated by the OMX core.
 *
 * @param plcml_Init This structure is filled and sent to LCML.
 *
 * @pre          None
 *
 * @post         None
 *
 *  @return      OMX_ErrorNone = Successful Inirialization of the LCML struct.
 *               OMX_ErrorInsufficientResources = Not enough memory
 *
 *  @see         None
 */
/* ================================================================================ * */
OMX_ERRORTYPE G722DEC_Fill_LCMLInitParams(OMX_HANDLETYPE pHandle,
                                          LCML_DSP *plcml_Init, OMX_U16 arr[]);

/* ================================================================================= * */
/**
 * G722DEC_GetBufferDirection() function determines whether it is input buffer or
 * output buffer.
 *
 * @param *pBufHeader This is pointer to buffer header whose direction needs to
 *                    be determined.
 *
 * @param *eDir  This is output argument which stores the direction of buffer.
 *
 * @pre          None
 *
 * @post         None
 *
 *  @return      OMX_ErrorNone = Successful processing.
 *               OMX_ErrorBadParameter = In case of invalid buffer
 *
 *  @see         None
 */
/* ================================================================================ * */
OMX_ERRORTYPE G722DEC_GetBufferDirection(OMX_BUFFERHEADERTYPE *pBufHeader,
                                         OMX_DIRTYPE *eDir);

/* ================================================================================= * */
/**
 * G722DEC_LCML_Callback() function is callback which is called by LCML whenever
 * there is an even generated for the component.
 *
 * @param event  This is event that was generated.
 *
 * @param arg    This has other needed arguments supplied by LCML like handles
 *               etc.
 *
 * @pre          None
 *
 * @post         None
 *
 *  @return      OMX_ErrorNone = Successful processing.
 *               OMX_ErrorInsufficientResources = Not enough memory
 *
 *  @see         None
 */
/* ================================================================================ * */
OMX_ERRORTYPE G722DEC_LCML_Callback (TUsnCodecEvent event,void * args [10]);

/* ================================================================================= * */
/**
 * G722DEC_HandleCommand() function handles the command sent by the application.
 * All the state transitions, except from nothing to loaded state, of the
 * component are done by this function.
 *
 * @param pComponentPrivate  This is component's private date structure.
 *
 * @pre          None
 *
 * @post         None
 *
 *  @return      OMX_ErrorNone = Successful processing.
 *               OMX_ErrorInsufficientResources = Not enough memory
 *               OMX_ErrorHardware = Hardware error has occured lile LCML failed
 *               to do any said operartion.
 *
 *  @see         None
 */
/* ================================================================================ * */
OMX_U32 G722DEC_HandleCommand (G722DEC_COMPONENT_PRIVATE *pComponentPrivate);

/* ================================================================================= * */
/**
 * G722DEC_HandleDataBuf_FromApp() function handles the input and output buffers
 * that come from the application. It is not direct function wich gets called by
 * the application rather, it gets called eventually.
 *
 * @param *pBufHeader This is the buffer header that needs to be processed.
 *
 * @param *pComponentPrivate  This is component's private date structure.
 *
 * @pre          None
 *
 * @post         None
 *
 *  @return      OMX_ErrorNone = Successful processing.
 *               OMX_ErrorInsufficientResources = Not enough memory
 *               OMX_ErrorHardware = Hardware error has occured lile LCML failed
 *               to do any said operartion.
 *
 *  @see         None
 */
/* ================================================================================ * */
OMX_ERRORTYPE G722DEC_HandleDataBuf_FromApp(OMX_BUFFERHEADERTYPE *pBufHeader,
                                            G722DEC_COMPONENT_PRIVATE *pComponentPrivate);

/* ================================================================================= * */
/**
 * G722DEC_GetLCMLHandle() function gets the LCML handle and interacts with LCML
 * by using this LCML Handle.
 *
 * @param *pBufHeader This is the buffer header that needs to be processed.
 *
 * @param *pComponentPrivate  This is component's private date structure.
 *
 * @pre          None
 *
 * @post         None
 *
 *  @return      OMX_HANDLETYPE = Successful loading of LCML library.
 *               OMX_ErrorHardware = Hardware error has occured.
 *
 *  @see         None
 */
/* ================================================================================ * */
OMX_HANDLETYPE G722DEC_GetLCMLHandle(G722DEC_COMPONENT_PRIVATE *pComponentPrivate);

/* ================================================================================= * */
/**
 * G722DEC_GetCorresponding_LCMLHeader() function gets the corresponding LCML
 * header from the actual data buffer for required processing.
 *
 * @param *pBuffer This is the data buffer pointer.
 *
 * @param eDir   This is direction of buffer. Input/Output.
 *
 * @param *G722D_LCML_BUFHEADERTYPE  This is pointer to LCML Buffer Header.
 *
 * @pre          None
 *
 * @post         None
 *
 *  @return      OMX_ErrorNone = Successful Inirialization of the component\n
 *               OMX_ErrorHardware = Hardware error has occured.
 *
 *  @see         None
 */
/* ================================================================================ * */
OMX_ERRORTYPE G722DEC_GetCorresponding_LCMLHeader(G722DEC_COMPONENT_PRIVATE *pComponentPrivate,
                                                  OMX_U8 *pBuffer,
                                                  OMX_DIRTYPE eDir,
                                                  G722D_LCML_BUFHEADERTYPE **ppLcmlHdr);

/* ================================================================================= * */
/**
 * G722DEC_FreeCompResources() function frees the component resources.
 *
 * @param pComponent This is the component handle.
 *
 * @pre          None
 *
 * @post         None
 *
 *  @return      OMX_ErrorNone = Successful Inirialization of the component\n
 *               OMX_ErrorHardware = Hardware error has occured.
 *
 *  @see         None
 */
/* ================================================================================ * */
OMX_ERRORTYPE G722DEC_FreeCompResources(OMX_HANDLETYPE pComponent);

/* ================================================================================= * */
/**
 * G722DEC_CleanupInitParams() function frees only the initialization time
 * memories allocated. For example, it will not close pipes, it will not free the
 * memory allocated to the buffers etc. But it does free the memory of buffers
 * utilized by the LCML etc. It is basically subset of G722DEC_FreeResources()
 * function.
 *
 * @param pComponent This is the component handle.
 *
 * @pre          None
 *
 * @post         None
 *
 *  @return      OMX_ErrorNone = Successful Inirialization of the component\n
 *
 *  @see         None
 */
/* ================================================================================ * */
void G722DEC_CleanupInitParams(OMX_HANDLETYPE pComponent);

/* ================================================================================= * */
/**
 * G722DEC_ComponentThread() This is component thread of the component which keeps
 * running or lsitening from the application unless component is deinitialized
 * from by the application i.e. component is transitioned from Idle to Loaded
 * state.
 *
 * @param pHandle This is component handle allocated by the OMX core. 
 *
 * @pre          None
 *
 * @post         None
 *
 *  @return      OMX_ErrorNone = Successful Inirialization of the component\n
 *               OMX_ErrorInsufficientResources = Not enough memory
 *
 *  @see         None
 */
/* ================================================================================ * */
void* G722DEC_ComponentThread (void* pThreadData);

OMX_ERRORTYPE   G722DECFill_LCMLInitParamsEx    (OMX_HANDLETYPE pComponent);
void            G722DEC_SetPending              (G722DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir, OMX_U32 lineNumber);
void            G722DEC_ClearPending            (G722DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir, OMX_U32 lineNumber) ;
OMX_U32         G722DEC_IsPending               (G722DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir);
OMX_U32         G722DEC_IsValid                 (G722DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U8 *pBuffer, OMX_DIRTYPE eDir) ;
void            printEmmEvent                   (TUsnCodecEvent);
void*           G722DEC_ComponentThread         (void* pThreadData);

#ifndef OMX_G722_COMPONENTTHREAD__H
#define OMX_G722_COMPONENTTHREAD__H

#define EXIT_COMPONENT_THRD  10
#endif

#endif
