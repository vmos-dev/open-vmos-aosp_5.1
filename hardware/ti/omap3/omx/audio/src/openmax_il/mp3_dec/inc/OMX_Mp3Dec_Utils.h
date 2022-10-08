
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
* @file OMX_Mp3Dec_Utils.h
*
* This is an header file for an audio MP3 decoder that is fully
* compliant with the OMX Audio specification.
* This the file is used internally by the component
* in its code.
*
* @path  $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\mp3_dec\inc\
*
* @rev 1.0
*/
/* --------------------------------------------------------------------------- */
#ifndef OMX_MP3DEC_UTILS__H
#define OMX_MP3DEC_UTILS__H

#include <OMX_Component.h>
#include "OMX_TI_Common.h"
#include <OMX_TI_Debug.h>
#include "LCML_DspCodec.h"

#ifdef UNDER_CE
#include <windows.h>
#include <oaf_osal.h>
#include <omx_core.h>
#include <stdlib.h>
#else
#include <pthread.h>
#endif

#ifndef UNDER_CE

#ifdef RESOURCE_MANAGER_ENABLED
#include <ResourceManagerProxyAPI.h>
#endif


#define _ERROR_PROPAGATION__ 
#ifdef __PERF_INSTRUMENTATION__
    #include "perf.h"
#endif

#ifndef ANDROID
    #define ANDROID
#endif

#ifdef ANDROID
    /* Log for Android system*/
    #include <utils/Log.h>
    #undef LOG_TAG
    #define LOG_TAG "OMX_MP3"
    
    /* PV opencore capability custom parameter index */ 
    #define PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX 0xFF7A347
#endif

#endif

#define MP3DEC_MAJOR_VER 0x1/* Major number of the component */
#define MP3DEC_MINOR_VER 0x1 /* Mnor number of the component */
#define NOT_USED 10 /* Value not used */
#define NORMAL_BUFFER 0 /* Marks a buffer as normal buffer */
#define OMX_MP3DEC_DEFAULT_SEGMENT (0) /* default segment ID of the component */
#define OMX_MP3DEC_SN_TIMEOUT (-1) /* tomeout value keep waiting until get the message */
#define OMX_MP3DEC_SN_PRIORITY (10) /* SN priority value */

#define MP3DEC_BUFHEADER_VERSION 0x0 /* Buffer Header structure version number */
#define MP3D_TIMEOUT (1000) /* default timeout in millisecs */
#define MP3_CPU 25

/* #define DSP_RENDERING_ON*/ /* Enable to use DASF functionality */
/* #define MP3DEC_MEMDEBUG */ /* Enable memory leaks debuf info */
//#define MP3DEC_DEBUG    /* See all debug statement of the component */
/* #define MP3DEC_MEMDETAILS */  /* See memory details of the component */
/* #define MP3DEC_BUFDETAILS */  /* See buffers details of the component */
// #define MP3DEC_STATEDETAILS /* See all state transitions of the component*/

#define MP3_APP_ID  100 /* Defines MP3 Dec App ID, App must use this value */
#define MP3D_MAX_NUM_OF_BUFS 10 /* Max number of buffers used */
#define MP3D_NUM_INPUT_BUFFERS 4  /* Default number of input buffers */
#define MP3D_NUM_OUTPUT_BUFFERS 4 /* Default number of output buffers */

#define MP3D_INPUT_BUFFER_SIZE  2000*4 /* Default size of input buffer */
#define MP3D_OUTPUT_BUFFER_SIZE 8192 /* Default size of output buffer */
#define MP3D_DEFAULT_FREQUENCY 44100 /* Default sample frequency*/

#define OUTPUT_PORT_MP3DEC 1
#define INPUT_PORT_MP3DEC 0

#define MP3D_MONO_STREAM  1 /* Mono stream index */
#define MP3D_STEREO_INTERLEAVED_STREAM  2 /* Stereo Interleaved stream index */
#define MP3D_STEREO_NONINTERLEAVED_STREAM  3 /* Stereo Non-Interleaved stream index */

#define MP3D_STEREO_STREAM  2

#define NUM_OF_PORTS 0x2 /* Number of ports of component */

#ifdef UNDER_CE
#define MP3DEC_USN_DLL_NAME "\\windows\\usn.dll64P"
#else
#define MP3DEC_USN_DLL_NAME "usn.dll64P"
#endif

#ifdef UNDER_CE
#define MP3DEC_DLL_NAME "\\windows\\mp3dec_sn.dll64P"
#else
#define MP3DEC_DLL_NAME "mp3dec_sn.dll64P"
#endif

#define DONT_CARE 0

#define EXIT_COMPONENT_THRD  10

#ifdef UNDER_CE /* For Windows */

#ifdef  MP3DEC_DEBUG
 #define MP3DEC_DPRINT(STR, ARG...) printf()
#else
#endif

#ifdef  MP3DEC_DEBUG
 #define MP3DEC_EPRINT(STR, ARG...) printf()
#else
#endif


#ifdef MP3DEC_MEMCHECK
        #define MP3DEC_MEMPRINT(STR, ARG...) printf()
#else
#endif

#ifdef MP3DEC_STATEDETAILS
        #define MP3DEC_STATEPRINT(STR, ARG...) printf()
#else
#endif


#ifdef MP3DEC_BUFDETAILS
        #define MP3DEC_BUFPRINT(STR, ARG...) printf()
#else
#endif

#ifdef MP3DEC_MEMDETAILS
        #define MP3DEC_MEMPRINT(STR, ARG...) printf()
#else
#endif


#ifdef DEBUG
        #define MP3DEC_DPRINT   printf
        #define MP3DEC_EPRINT   printf
        #define MP3DEC_MEMPRINT   printf
        #define MP3DEC_STATEPRINT   printf
        #define MP3DEC_BUFPRINT   printf
#else
        #define MP3DEC_DPRINT
        #define MP3DEC_EPRINT printf
        #define MP3DEC_MEMPRINT
        #define MP3DEC_STATEPRINT
        #define MP3DEC_BUFPRINT
#endif

#else /* for Linux */
#ifdef  MP3DEC_DEBUG

  #ifdef ANDROID
    #define MP3DEC_DPRINT  ALOGW
    #define MP3DEC_BUFPRINT ALOGW
    #define MP3DEC_MEMPRINT ALOGW
    #define MP3DEC_STATEPRINT ALOGW
  #else
    #define MP3DEC_DPRINT  printf
    #define MP3DEC_BUFPRINT printf
    #define MP3DEC_MEMPRINT printf
    #define MP3DEC_STATEPRINT printf
  #endif

#else
    #define MP3DEC_DPRINT(...)

    #ifdef MP3DEC_STATEDETAILS
        #define MP3DEC_STATEPRINT printf
    #else
        #define MP3DEC_STATEPRINT(...)
    #endif

    #ifdef MP3DEC_BUFDETAILS
        #define MP3DEC_BUFPRINT printf
    #else
        #define MP3DEC_BUFPRINT(...)
    #endif

    #ifdef MP3DEC_MEMDETAILS
        #define MP3DEC_MEMPRINT printf
    #else
        #define MP3DEC_MEMPRINT(...)
    #endif

#endif

#ifdef ANDROID
  #define MP3DEC_EPRINT ALOGE
#else
  #define MP3DEC_EPRINT printf
#endif

#endif

#define MP3D_OMX_ERROR_EXIT(_e_, _c_, _s_)\
    _e_ = _c_;\
    OMXDBG_PRINT(stderr, ERROR, 4, 0, "\n**************** OMX ERROR ************************\n");\
    OMXDBG_PRINT(stderr, ERROR, 4, 0, "%d : Error Name: %s : Error Num = %x",__LINE__, _s_, _e_);\
    OMXDBG_PRINT(stderr, ERROR, 4, 0, "\n**************** OMX ERROR ************************\n");\
    goto EXIT;


#define MP3D_OMX_CONF_CHECK_CMD(_ptr1, _ptr2, _ptr3) \
{                                               \
    if(!_ptr1 || !_ptr2 || !_ptr3){             \
        eError = OMX_ErrorBadParameter;         \
        goto EXIT;                 \
    }                                           \
}

#define OMX_CONF_INIT_STRUCT(_s_, _name_)       \
    memset((_s_), 0x0, sizeof(_name_)); \
    (_s_)->nSize = sizeof(_name_);              \
    (_s_)->nVersion.s.nVersionMajor = 0x1;      \
    (_s_)->nVersion.s.nVersionMinor = 0x1;      \
    (_s_)->nVersion.s.nRevision = 0x1;          \
    (_s_)->nVersion.s.nStep = 0x0

/* ======================================================================= */
/** PV_OMXComponentCapabilityFlagsType: this communicates capabilities to opencore client
* 
*/
/* ==================================================================== */
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
/** MP3D_COMP_PORT_TYPE: This enum is used by the OMX Component.
* 
* @param MP3D_INPUT_PORT: Specifies Input port.
*
* @param MP3D_OUTPUT_PORT: Specifies Output port.
*/
/* ==================================================================== */
typedef enum MP3D_COMP_PORT_TYPE {
    MP3D_INPUT_PORT = 0,
    MP3D_OUTPUT_PORT
}MP3D_COMP_PORT_TYPE;

/* ======================================================================= */
/**
 * pthread variable to indicate OMX returned all buffers to app 
 */
/* ======================================================================= */
    pthread_mutex_t bufferReturned_mutex; 
    pthread_cond_t bufferReturned_condition; 

/* ======================================================================= */
/** OMX_INDEXAUDIOTYPE: This enum is used by the TI OMX Component.
* 
* @param : 
*
* @param :
*/
/* ==================================================================== */

typedef enum OMX_INDEXAUDIOTYPE {
    MP3D_OMX_IndexCustomMode16_24bit = 0xFF000001,
    MP3D_OMX_IndexCustomModeDasfConfig,
    OMX_IndexCustomMp3DecHeaderInfoConfig,
    OMX_IndexCustomMp3DecStreamInfoConfig,
    OMX_IndexCustomMp3DecDataPath,
    OMX_IndexCustomDebug
}OMX_INDEXAUDIOTYPE;
/* ======================================================================= */
/** MP3DEC_BUFDATA
* 
* @param nFrames: Specifies the number of frames received by the SN.
*
*/
/* ==================================================================== */
typedef struct MP3DEC_BUFDATA {
   OMX_U8 nFrames;     
}MP3DEC_BUFDATA;

/* ======================================================================= */
/** IAUDIO_PcmFormat: This value is used by DSP.
* 
* @param IAUDIO_BLOCK: It is used in DASF mode.
*
* @param IAUDIO_INTERLEAVED: It specifies interleaved format of SN.
*/
/* ==================================================================== */
typedef enum {
    IAUDIO_BLOCK=0,
    IAUDIO_INTERLEAVED
} IAUDIO_PcmFormat;


/* ======================================================================= */
/** MP3DEC_UALGParams
 *
 * @param lOutputFormat - To set interleaved/Block format:Refer to IAUDIO_Mp3Format.
 * @param DownSampleSbr -
 */
/* ==================================================================== */
typedef struct {
  OMX_U32 size;
  unsigned long      lOutputFormat;
  unsigned long    lMonoToStereoCopy;
  unsigned long    lStereoToMonoCopy;
} MP3DEC_UALGParams;


/* ======================================================================= */
/** MP3D_IUALG_Cmd: This enum type describes the standard set of commands that 
* will be passed to iualg control API at DSP. This enum is taken as it is from
* DSP side USN source code.
* 
* @param IUALG_CMD_STOP: This command indicates that higher layer framework
* has received a stop command and no more process API will be called for the
* current data stream. The iualg layer is expected to ensure that all processed
* output as is put in the output IUALG_Buf buffers and the state of all buffers
* changed as to free or DISPATCH after this function call. 
*
* @param IUALG_CMD_PAUSE: This command indicates that higher layer framework
* has received a PAUSE command on the current data stream. The iualg layer 
* can change the state of some of its output IUALG_Bufs to DISPATCH to enable
* high level framework to use the processed data until the command was received.
*
* @param IUALG_CMD_GETSTATUS: This command indicates that some algo specific 
* status needs to be returned to the framework. The pointer to the status
* structure will be in IALG_status * variable passed to the control API. 
* The interpretation of the content of this pointer is left to IUALG layer.
*
* @param IUALG_CMD_SETSTATUS: This command indicates that some algo specific 
* status needs to be set. The pointer to the status structure will be in 
* IALG_status * variable passed to the control API. The interpretation of the
* content of this pointer is left to IUALG layer.
*
* @param IUALG_CMD_USERCMDSTART: The algorithm specific control commands can
* have the enum type set from this number.
*/
/* ==================================================================== */
typedef enum {
    IUALG_CMD_STOP          = 0,
    IUALG_CMD_PAUSE         = 1,
    IUALG_CMD_GETSTATUS     = 2,
    IUALG_CMD_SETSTATUS     = 3,
    IUALG_CMD_USERCMDSTART  = 100
}IUALG_Cmd;

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
/** IUALG_MP3DCmd: This enum specifies the command to DSP.
* 
* @param IULAG_CMD_SETSTREAMTYPE: Specifies the stream type to be sent to DSP.
*/
/* ==================================================================== */
typedef enum {
    IULAG_CMD_SETSTREAMTYPE = IUALG_CMD_USERCMDSTART
}IUALG_MP3DCmd;

/* ======================================================================= */
/** MP3DEC_UAlgInBufParamStruct: This struct is passed with input buffers that
 * are sent to DSP.
*/
/* ==================================================================== */
typedef struct {
    /* Set to 1 if buffer is last buffer */
    unsigned long bLastBuffer;
}MP3DEC_UAlgInBufParamStruct;

/* ======================================================================= */
/** MP3D_USN_AudioCodecParams: This contains the information which does to Codec
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
}USN_AudioCodecParams;

/* ======================================================================= */
/** MP3DEC_UAlgOutBufParamStruct: This is passed with output buffer to DSP.
*/
/* ==================================================================== */
typedef struct {
    /* Number of frames in a buffer */
    unsigned long ulFrameCount;
    unsigned long ulIsLastBuffer;
}MP3DEC_UAlgOutBufParamStruct;

/* ======================================================================= */
/** MP3D_LCML_BUFHEADERTYPE: This is LCML buffer header which is sent to LCML
 * for both input and output buffers.
*/
/* ==================================================================== */
typedef struct MP3D_LCML_BUFHEADERTYPE {
    /* Direction whether input or output buffer */
    OMX_DIRTYPE eDir;
    /* Pointer to OMX Buffer Header */
    OMX_BUFFERHEADERTYPE *pBufHdr;
    /* Other parameters, may be useful for enhancements */
    void *pOtherParams[10];
    /* Input Parameter Information structure */
    MP3DEC_UAlgInBufParamStruct *pIpParam;
    /* Output Parameter Information structure */
    MP3DEC_UAlgOutBufParamStruct *pOpParam;
}MP3D_LCML_BUFHEADERTYPE;

/* Component Port Context */
typedef struct MP3D_AUDIODEC_PORT_TYPE {
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
} MP3D_AUDIODEC_PORT_TYPE;


/* ======================================================================= */
/** _MP3D_BUFFERLIST: This contains information about a buffer's owner whether
 * it is application or component, number of buffers owned etc.
*
* @see OMX_BUFFERHEADERTYPE
*/
/* ==================================================================== */
struct _MP3D_BUFFERLIST{
    /* Array of pointer to OMX buffer headers */
    OMX_BUFFERHEADERTYPE *pBufHdr[MP3D_MAX_NUM_OF_BUFS];
    /* Array that tells about owner of each buffer */
    OMX_U32 bufferOwner[MP3D_MAX_NUM_OF_BUFS];
    /* Tracks pending buffers */
    OMX_U32 bBufferPending[MP3D_MAX_NUM_OF_BUFS];
    /* Number of buffers  */
    OMX_U32 numBuffers;
};

typedef struct _MP3D_BUFFERLIST MP3D_BUFFERLIST;

typedef struct StreamData{
    OMX_U32 nSyncWord;
    OMX_U32 nMpegVersion;
    OMX_U32 nLayer;
    OMX_U32 nBitRate;
    OMX_U32 nFrequency;
    OMX_U32 nChannelMode;
}StreamData;


/* ======================================================================= */
/** MP3DEC_COMPONENT_PRIVATE: This is the major and main structure of the
 * component which contains all type of information of buffers, ports etc
 * contained in the component.
*
* @see OMX_BUFFERHEADERTYPE
* @see OMX_AUDIO_PARAM_PORTFORMATTYPE
* @see OMX_PARAM_PORTDEFINITIONTYPE
* @see MP3D_LCML_BUFHEADERTYPE
* @see OMX_PORT_PARAM_TYPE
* @see OMX_PRIORITYMGMTTYPE
* @see MP3D_AUDIODEC_PORT_TYPE
* @see MP3D_BUFFERLIST
* @see MP3D_AUDIODEC_PORT_TYPE
* @see LCML_STRMATTR
* @see 
*/
/* ==================================================================== */
typedef struct MP3DEC_COMPONENT_PRIVATE
{
    /** Handle for use with async callbacks */
    OMX_CALLBACKTYPE cbInfo;
    /** Handle for use with async callbacks */

    /** Number of input buffers at runtime */
    OMX_U32 nRuntimeInputBuffers;
    /** Number of output buffers at runtime */
    OMX_U32 nRuntimeOutputBuffers;
    /** Parameters being passed to the SN */
    USN_AudioCodecParams *pParams;
    /**Dynamic parameters*/
    MP3DEC_UALGParams *ptAlgDynParams;

    OMX_PORT_PARAM_TYPE* sPortParam;
    /* Input port information */
    OMX_AUDIO_PARAM_PORTFORMATTYPE sInPortFormat;
    /* Output port information */
    OMX_AUDIO_PARAM_PORTFORMATTYPE sOutPortFormat;
    /* Buffer owner information */
    OMX_U32 bIsBufferOwned[NUM_OF_PORTS];

    /** This will contain info like how many buffers
        are there for input/output ports, their size etc, but not
        BUFFERHEADERTYPE POINTERS. */
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef[NUM_OF_PORTS];
    /* Contains information that come from application */
    OMX_AUDIO_PARAM_MP3TYPE* mp3Params;
    OMX_AUDIO_PARAM_PCMMODETYPE* pcmParams;
    /** This is component handle */
    OMX_COMPONENTTYPE* pHandle;

    /** Needed to free LCML lib dll **/
    void* ptrLibLCML;

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

#ifdef __PERF_INSTRUMENTATION__
    PERF_OBJHANDLE pPERF, pPERFcomp;
    OMX_U32 nLcml_nCntIp;         
    OMX_U32 nLcml_nCntOpReceived;
#endif

    /** Counts of number of output buffers reclaimed from lcml  */
    OMX_U32 num_Reclaimed_Op_Buff;
    /** Counts of number of input buffers sent to lcml  */
    OMX_U32 num_Sent_Ip_Buff;
    /** Counts of number of output buffers sent to lcml  */
    OMX_U32 num_Op_Issued;
    /** Holds the value of dasf mode, 1: DASF mode or 0: File Mode  */
    OMX_U32 dasfmode;
    /** Holds the value of frame mode, 1: frame mode or 0: non Frame Mode  */
    OMX_U32 frameMode;

    /** This is LCML handle  */
    OMX_HANDLETYPE pLcmlHandle;

    /** Contains pointers to LCML Buffer Headers */
    MP3D_LCML_BUFHEADERTYPE *pLcmlBufHeader[2];
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

#ifdef RESOURCE_MANAGER_ENABLED
    RMPROXY_CALLBACKTYPE rmproxyCallback;
#endif
    OMX_BOOL bPreempted;
    
    /** Contains the port related info of both the ports */
    MP3D_AUDIODEC_PORT_TYPE *pCompPort[NUM_OF_PORTS];
    /* Checks whether or not buffer were allocated by appliction */
    int bufAlloced;
    /** Flag to check about execution of component thread */
    OMX_U16 bExitCompThrd;
    /** Pointer to list of input buffers */
    MP3D_BUFFERLIST *pInputBufferList;
    /** Pointer to list of output buffers */
    MP3D_BUFFERLIST *pOutputBufferList;
    /** it is used for component's create phase arguments */
    LCML_STRMATTR  *strmAttr;
    /** Contains the version information */
    OMX_U32 nVersion;

    /** Audio Stream ID */
    OMX_U32 streamID;
    OMX_BOOL bIsInvalidState;
    OMX_STRING* sDeviceString;
    /** MPEG 1 Layer 2 custom flag **/
    OMX_BOOL mpeg1_layer2;

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
    MP3D_BUFFERLIST *pInputBufferListQueue;
    MP3D_BUFFERLIST *pOutputBufferListQueue;
    OMX_BUFFERHEADERTYPE *pInputBufHdrPending[MP3D_MAX_NUM_OF_BUFS];
    OMX_U32 nNumInputBufPending;
    OMX_BUFFERHEADERTYPE *pOutputBufHdrPending[MP3D_MAX_NUM_OF_BUFS];
    OMX_U32 nNumOutputBufPending;

    /** Store buffers received while paused **/
    OMX_BUFFERHEADERTYPE *pOutputBufHdrPause[MP3D_MAX_NUM_OF_BUFS];
    OMX_U32 nNumOutputBufPause;
    /** Flags to control port disable command **/
    OMX_U32 bDisableCommandPending;
    OMX_U32 bDisableCommandParam;
    /** Flags to control port enable command **/
    OMX_U32 bEnableCommandPending;
    OMX_U32 bEnableCommandParam;

    OMX_U32 nInvalidFrameCount;
    OMX_U32 numPendingBuffers;
    OMX_U32 bNoIdleOnStop;
    OMX_U32 bDspStoppedWhileExecuting;
    OMX_BOOL bLoadedCommandPending;

    /** Counts number of buffers received from client */
    OMX_U32 nHandledFillThisBuffers;
    /** Count number of buffers recieved from client */
    OMX_U32 nHandledEmptyThisBuffers;
    OMX_BOOL bFlushOutputPortCommandPending;
    OMX_BOOL bFlushInputPortCommandPending;

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
	
#else
    OMX_Event AlloBuf_event;
    OMX_U8 AlloBuf_waitingsignal;
    
    OMX_Event InLoaded_event;
    OMX_U8 InLoaded_readytoidle;
    
    OMX_Event InIdle_event;
    OMX_U8 InIdle_goingtoloaded; 
#endif    

    OMX_PARAM_COMPONENTROLETYPE componentRole;
    
    /** Keep buffer timestamps **/
    OMX_S64 arrBufIndex[MP3D_MAX_NUM_OF_BUFS];
	/**Keep buffer tickcounts*/
    OMX_U32 arrBufIndexTick[MP3D_MAX_NUM_OF_BUFS];
	
    /** Index to arrBufIndex[] and arrBufIndexTick[], used for input buffer timestamps */
    OMX_U8 IpBufindex;
    /** Index to arrBufIndex[] and arrBufIndexTick[], used for output buffer timestamps */
    OMX_U8 OpBufindex;

    OMX_U8 SendAfterEOS;
    
    /** Flag to mark the first buffer sent **/
    OMX_U8 first_buff;

    /** First Time Stamp sent **/
    OMX_S64 first_TS;
    /** Temp Time Stamp to store intermediate values **/
    OMX_S64 temp_TS;
    /** Last buffer received usind in PV-Android context **/
    OMX_BUFFERHEADERTYPE *lastout;

    PV_OMXComponentCapabilityFlagsType iPVCapabilityFlags;
    OMX_BOOL reconfigInputPort;
    OMX_BOOL reconfigOutputPort;
    OMX_BOOL bConfigData;

    StreamData pStreamData;

    struct OMX_TI_Debug dbg;

} MP3DEC_COMPONENT_PRIVATE;


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
*  @see          Mp3Dec_StartCompThread()
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
* Mp3Dec_StartCompThread() starts the component thread. This is internal
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
OMX_ERRORTYPE Mp3Dec_StartCompThread(OMX_HANDLETYPE pHandle);


/* ================================================================================= * */
/**
* MP3DEC_Fill_LCMLInitParams() fills the LCML initialization structure.
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
OMX_ERRORTYPE MP3DEC_Fill_LCMLInitParams(OMX_HANDLETYPE pHandle, LCML_DSP *plcml_Init, OMX_U16 arr[]);


/* ================================================================================= * */
/**
* MP3DEC_GetBufferDirection() function determines whether it is input buffer or
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
OMX_ERRORTYPE MP3DEC_GetBufferDirection(OMX_BUFFERHEADERTYPE *pBufHeader, OMX_DIRTYPE *eDir);

/* ================================================================================= * */
/**
* MP3DEC_LCML_Callback() function is callback which is called by LCML whenever
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
OMX_ERRORTYPE MP3DEC_LCML_Callback (TUsnCodecEvent event,void * args [10]);

/* ================================================================================= * */
/**
* MP3DEC_HandleCommand() function handles the command sent by the application.
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
OMX_U32 MP3DEC_HandleCommand (MP3DEC_COMPONENT_PRIVATE *pComponentPrivate);

/* ================================================================================= * */
/**
* MP3DEC_HandleDataBuf_FromApp() function handles the input and output buffers
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
OMX_ERRORTYPE MP3DEC_HandleDataBuf_FromApp(OMX_BUFFERHEADERTYPE *pBufHeader, MP3DEC_COMPONENT_PRIVATE *pComponentPrivate);

/* ================================================================================= * */
/**
* MP3DEC_GetLCMLHandle() function gets the LCML handle and interacts with LCML
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
OMX_HANDLETYPE MP3DEC_GetLCMLHandle(MP3DEC_COMPONENT_PRIVATE* pComponentPrivate);

/* ================================================================================= * */
/**
* MP3DEC_GetCorresponding_LCMLHeader() function gets the corresponding LCML
* header from the actual data buffer for required processing.
*
* @param *pBuffer This is the data buffer pointer. 
*
* @param eDir   This is direction of buffer. Input/Output.
*
* @param *MP3D_LCML_BUFHEADERTYPE  This is pointer to LCML Buffer Header.
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
OMX_ERRORTYPE MP3DEC_GetCorresponding_LCMLHeader(MP3DEC_COMPONENT_PRIVATE* pComponentPrivate,
                                                                                OMX_U8 *pBuffer,
                                        OMX_DIRTYPE eDir,
                                        MP3D_LCML_BUFHEADERTYPE **ppLcmlHdr);
/* ================================================================================= * */
/**
* MP3DEC_FreeCompResources() function frees the component resources.
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
OMX_ERRORTYPE MP3DEC_FreeCompResources(OMX_HANDLETYPE pComponent);


/* ================================================================================= * */
/**
* MP3DEC_CleanupInitParams() function frees only the initialization time
* memories allocated. For example, it will not close pipes, it will not free the
* memory allocated to the buffers etc. But it does free the memory of buffers
* utilized by the LCML etc. It is basically subset of MP3DEC_FreeResources()
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
void MP3DEC_CleanupInitParams(OMX_HANDLETYPE pComponent);
/* ================================================================================= * */
/**
* MP3DEC_CleanupInitParamsEx() function frees only the initialization time
* memories allocated. For example, it will not close pipes, it will not free the
* memory allocated to the buffers etc. But it does free the memory of buffers
* utilized by the LCML etc. It is basically subset of MP3DEC_FreeResources()
* function. Called while port disable when port reconfiguration takes place.
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
void MP3DEC_CleanupInitParamsEx(OMX_HANDLETYPE pComponent,OMX_U32 indexport);

#ifdef RESOURCE_MANAGER_ENABLED
/* =================================================================================== */
/**
*  MP3_ResourceManagerCallback() Callback from Resource Manager
*
*  @param cbData	RM Proxy command data
*
*  @return None
*/
/* =================================================================================== */
void MP3_ResourceManagerCallback(RMPROXY_COMMANDDATATYPE cbData);
#endif

OMX_ERRORTYPE MP3DECFill_LCMLInitParamsEx(OMX_HANDLETYPE pComponent,OMX_U32 indexport);
void MP3DEC_SetPending(MP3DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir, OMX_U32 lineNumber);
void MP3DEC_ClearPending(MP3DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir, OMX_U32 lineNumber) ;
OMX_U32 MP3DEC_IsPending(MP3DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir);
OMX_U32 MP3DEC_IsValid(MP3DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U8 *pBuffer, OMX_DIRTYPE eDir) ;
void* MP3DEC_ComponentThread (void* pThreadData);

/*  =========================================================================*/
/*  func    GetBits                                                          */
/*                                                                           */
/*  desc    Gets aBits number of bits from position aPosition of one buffer  */
/*            and returns the value in a TUint value.                        */
/*  =========================================================================*/
OMX_U32 MP3DEC_GetBits(OMX_U32* nPosition, OMX_U8 nBits, OMX_U8* pBuffer, OMX_BOOL bIcreasePosition);

/*=======================================================================*/
/*! @fn SignalIfAllBuffersAreReturned 

 * @brief Sends pthread signal to indicate OMX has returned all buffers to app 

 * @param  none 

 * @Return void 

 */
/*=======================================================================*/
void SignalIfAllBuffersAreReturned(MP3DEC_COMPONENT_PRIVATE *pComponentPrivate);

/*  =========================================================================*/
/*  func    MP3DEC_HandleUSNError
/*
/*  desc    Handles error messages returned by the dsp
/*
/*@return n/a
/*
/*  =========================================================================*/
void MP3DEC_HandleUSNError (MP3DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U32 arg);

#endif
