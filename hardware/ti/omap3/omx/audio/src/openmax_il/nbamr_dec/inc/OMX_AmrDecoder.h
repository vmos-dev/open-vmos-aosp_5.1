
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
* @file OMX_AmrDecoder.h
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

#ifndef OMX_AMRDECODER_H
#define OMX_AMRDECODER_H

#include "LCML_DspCodec.h"
#include <OMX_Component.h>
#include <pthread.h>
#include <OMX_TI_Debug.h>
#include <cutils/log.h>

#ifdef RESOURCE_MANAGER_ENABLED
#include <ResourceManagerProxyAPI.h>
#endif

#ifdef __PERF_INSTRUMENTATION__
    #include "perf.h"
#endif

#ifdef DSP_RENDERING_ON
#include <AudioManagerAPI.h>
#endif


#ifndef ANDROID
    #define ANDROID
#endif

#ifdef ANDROID
    #undef LOG_TAG
    #define LOG_TAG "OMX_NBAMRDEC"

    /* PV opencore capability custom parameter index */
    #define PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX 0xFF7A347
#endif


/* ======================================================================= */
/**
 * @def    EXTRA_BUFFBYTES                Num of Extra Bytes to be allocated
 */
/* ======================================================================= */
#define EXTRA_BUFFBYTES (256)

/* ======================================================================= */
/**
 * @def    NBAMRD_TIMEOUT   Default timeout used to come out of blocking calls
 */
/* ======================================================================= */
#define NBAMRD_TIMEOUT (1000) /* millisecs */

/* ======================================================================= */
/**
 * @def    NUM_NBAMRDEC_INPUT_BUFFERS              Number of Input Buffers
 */
/* ======================================================================= */
#define NUM_NBAMRDEC_INPUT_BUFFERS 1

/* ======================================================================= */
/**
 * @def    NUM_NBAMRDEC_OUTPUT_BUFFERS              Number of Output Buffers
 */
/* ======================================================================= */
#define NUM_NBAMRDEC_OUTPUT_BUFFERS 2

/* ======================================================================= */
/**
 * @def    NUM_NBAMRDEC_OUTPUT_BUFFERS_DASF         Number of Output Buffers
 *                                                  on DASF mode
 */
/* ======================================================================= */
#define NUM_NBAMRDEC_OUTPUT_BUFFERS_DASF 2

/* ======================================================================= */
/**
 * @def    OUTPUT_NBAMRDEC_BUFFER_SIZE           Standart Output Buffer Size
 */
/* ======================================================================= */
#define OUTPUT_NBAMRDEC_BUFFER_SIZE 320
/* ======================================================================= */
/**
 * @def    INPUT_NBAMRDEC_BUFFER_SIZE_MIME       Mime Input Buffer Size
 */
/* ======================================================================= */
#define INPUT_NBAMRDEC_BUFFER_SIZE_MIME 34

/* ======================================================================= */
/**
 * @def    INPUT_BUFF_SIZE_EFR                  Input Buffer Size EFR
 */
/* ======================================================================= */
#define INPUT_BUFF_SIZE_EFR 120

/* @def    INPUT_NBAMRDEC_BUFFER_SIZE_MIME       IF2 Input Buffer Size*/
/* ======================================================================= */
#define INPUT_NBAMRDEC_BUFFER_SIZE_IF2 32


/* ======================================================================= */
/**
 * @def    STD_NBAMRDEC_BUF_SIZE                  Standart Input Buffer Size
 */
/* ======================================================================= */
#define STD_NBAMRDEC_BUF_SIZE 118

/* ======================================================================= */
/**
 * @def    FRAME_SIZE_x                          Size in Bytes of determined
 *                                               frame.
 */
/* ======================================================================= */
#define  FRAME_SIZE_13  13
#define  FRAME_SIZE_19  19
#define  FRAME_SIZE_26  26
#define  FRAME_SIZE_31  31
#define  FRAME_SIZE_14  14
#define  FRAME_SIZE_16  16
#define  FRAME_SIZE_18  18
#define  FRAME_SIZE_20  20
#define  FRAME_SIZE_21  21
#define  FRAME_SIZE_27  27
#define  FRAME_SIZE_32  32
#define  FRAME_SIZE_6   6
#define  FRAME_SIZE_1   1
#define  FRAME_SIZE_0   0

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
 * @def    NBAMRDEC_SAMPLING_FREQUENCY          Sampling Frequency
 */
/* ======================================================================= */
#define NBAMRDEC_SAMPLING_FREQUENCY 8000

/* ======================================================================= */
/**
 * @def    NBAMRDEC_CPU_LOAD                    CPU Load in MHz
 */
/* ======================================================================= */
#define NBAMRDEC_CPU_LOAD 10

/* ======================================================================= */
/**
 * @def    MAX_NUM_OF_BUFS                      Max Num of Bufs Allowed
 */
/* ======================================================================= */
#define MAX_NUM_OF_BUFS 12
/* ======================================================================= */
/**
 * @def    IP_BUFFERSIZE                      Input Port Buffer Size
 */
/* ======================================================================= */
#define IP_BUFFERSIZE 4096
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
 * @def    NBAMRDEC_DEBUGMEM   Turns memory leaks messaging on and off.
 *         APP_DEBUGMEM must be defined in Test App in order to get
 *         this functionality On.
 */
/* ======================================================================= */
#undef NBAMRDEC_DEBUGMEM
/*#define NBAMRDEC_DEBUGMEM*/


/*#define AMRDEC_DEBUG*/
#undef AMRDEC_DEBUG
#undef AMRDEC_MEMCHECK


#ifndef UNDER_CE

#define AMRDEC_EPRINT(...)  __android_log_print(ANDROID_LOG_VERBOSE, __FILE__,"%s %d:: ERROR    ",__FUNCTION__, __LINE__);\
                                    __android_log_print(ANDROID_LOG_VERBOSE, __FILE__, __VA_ARGS__);\
                                    __android_log_print(ANDROID_LOG_VERBOSE, __FILE__, "\n");

#ifdef  AMRDEC_DEBUG
        #define AMRDEC_DPRINT(...)    __android_log_print(ANDROID_LOG_VERBOSE, __FILE__,"%s %d::    ",__FUNCTION__, __LINE__);\
                                    __android_log_print(ANDROID_LOG_VERBOSE, __FILE__, __VA_ARGS__);\
                                    __android_log_print(ANDROID_LOG_VERBOSE, __FILE__, "\n");
#else
        #define AMRDEC_DPRINT(...)
#endif

#ifdef  AMRDEC_MEMCHECK
        #define AMRDEC_MEMPRINT(...)    fprintf(stderr,__VA_ARGS__)
#else
        #define AMRDEC_MEMPRINT(...)
#endif


#ifdef  AMRDEC_DEBUG_MCP
        #define AMRDEC_MCP_DPRINT(...)    __android_log_print(ANDROID_LOG_VERBOSE, __FILE__,"%s %d:: MCP    ",__FUNCTION__, __LINE__);\
                                    __android_log_print(ANDROID_LOG_VERBOSE, __FILE__, __VA_ARGS__);\
                                    __android_log_print(ANDROID_LOG_VERBOSE, __FILE__, "\n");
#else
        #define AMRDEC_MCP_DPRINT(...)
#endif
#else /*UNDER_CE*/
#define AMRDEC_EPRINT   printf
#ifdef  AMRDEC_DEBUG
 #define AMRDEC_DPRINT(STR, ARG...) printf()
#else
#endif

#ifdef AMRDEC_MEMCHECK
    #define AMRDEC_MEMPRINT(STR, ARG...) printf()
#else
#endif
#ifdef UNDER_CE

#ifdef DEBUG
    #define AMRDEC_DPRINT   printf
    #define AMRDEC_MEMPRINT   printf

#else
    #define AMRDEC_DPRINT
    #define AMRDEC_MEMPRINT
#endif

#endif  //UNDER_CE

#endif


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
 * @def    _ERROR_PROPAGATION__              Allow Logic to Detec Arm Errors
 */
/* ======================================================================= */
#define _ERROR_PROPAGATION__

/* ======================================================================= */
/**
* pthread variable to indicate OMX returned all buffers to app 
*/
/* ======================================================================= */
pthread_mutex_t bufferReturned_mutex; 
pthread_cond_t bufferReturned_condition; 

/* ======================================================================= */
/** NBAMRDEC_COMP_PORT_TYPE  Port Type
*
*  @param  NBAMRDEC_INPUT_PORT                  Port Type Input
*
*  @param  NBAMRDEC_OUTPUT_PORT                 Port Type Output
*
*/
/*  ==================================================================== */
typedef enum NBAMRDEC_COMP_PORT_TYPE {
    NBAMRDEC_INPUT_PORT = 0,
    NBAMRDEC_OUTPUT_PORT
}NBAMRDEC_COMP_PORT_TYPE;

/* ======================================================================= */
/** NBAMRDEC_StreamType  StreamType
*
*  @param  NBAMRDEC_DMM                 Stream Type DMM
*
*  @param  NBAMRDEC_INSTRM              Stream Type Input
*
*  @param  NBAMRDEC_OUTSTRM             Stream Type Output
*/
/*  ==================================================================== */
enum NBAMRDEC_StreamType
{
    NBAMRDEC_DMM,
    NBAMRDEC_INSTRM,
    NBAMRDEC_OUTSTRM
};

/* ======================================================================= */
/** NBAMRDEC_DecodeType  Decode Type Mode
*
*  @param  NBAMR                    OMX_AUDIO_AMRDTX
*
*  @param  NBAMRDEC_EFR             OMX_AUDIO_AMRDTX as EFR
*/
/*  ==================================================================== */
enum NBAMRDEC_DecodeType
{
    NBAMR,
    NBAMRDEC_EFR
};

/* ======================================================================= */
/** NBAMRDEC_MimeMode  Mime Mode
*
*  @param  NBAMRDEC_FORMATCONFORMANCE       Mime Mode and IF2 Off
*
*  @param  NBAMRDEC_MIMEMODE                Mime Mode On
*/
/*  ==================================================================== */
enum NBAMRDEC_MimeMode {
    NBAMRDEC_FORMATCONFORMANCE,
    NBAMRDEC_MIMEMODE,
        NBAMRDEC_IF2,
        NBAMRDEC_PADMIMEMODE
};

/* ======================================================================= */
/** NBAMRDEC_BUFFER_Dir  Direction of the Buffer
*
*  @param  NBAMRDEC_DIRECTION_INPUT             Direction Input
*
*  @param  NBAMRDEC_DIRECTION_INPUT             Direction Output
*/
/*  ==================================================================== */
typedef enum {
    NBAMRDEC_DIRECTION_INPUT,
    NBAMRDEC_DIRECTION_OUTPUT
}NBAMRDEC_BUFFER_Dir;

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

/* ======================================================================= */
/** LCML_MimeMode  modes
*
*  @param  MODE_MIME                    Mode MIME
*
*  @param  MODE_NONMIME                 Mode NONMIME
*/
/*  ==================================================================== */
typedef enum {
    MODE_MIME,
    MODE_NONMIME
}LCML_MimeMode;

/* =================================================================================== */
/**
* Socket node input parameters.
*/
/* ================================================================================== */
typedef struct AMRDEC_AudioCodecParams
{
    unsigned long iSamplingRate;
    unsigned long iStrmId;
    unsigned short iAudioFormat;
}AMRDEC_AudioCodecParams;

/* =================================================================================== */
/**
* Socket node alg parameters..
*/
/* ================================================================================== */
/*typedef struct {

        unsigned long usEndOfFile;
        unsigned long usFrameLost;
}AMRDEC_UAlgInBufParamStruct;*/

typedef struct {
        unsigned long int usLastFrame;
        unsigned long int usFrameLost;
}NBAMRDEC_FrameStruct;

typedef struct{
         unsigned long int usNbFrames;
         NBAMRDEC_FrameStruct *pParamElem;
}NBAMRDEC_ParamStruct;

/* =================================================================================== */
/**
* LCML_NBAMRDEC_BUFHEADERTYPE
*/
/* ================================================================================== */
typedef struct LCML_NBAMRDEC_BUFHEADERTYPE {
      NBAMRDEC_BUFFER_Dir  eDir;
      OMX_BUFFERHEADERTYPE* buffer;
      NBAMRDEC_FrameStruct *pFrameParam;
      NBAMRDEC_ParamStruct *pBufferParam;
      DMM_BUFFER_OBJ* pDmmBuf;
}LCML_NBAMRDEC_BUFHEADERTYPE;

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

OMX_ERRORTYPE NBAMRDEC_StartComponentThread(OMX_HANDLETYPE pHandle);
OMX_ERRORTYPE NBAMRDEC_StopComponentThread(OMX_HANDLETYPE pHandle);
OMX_ERRORTYPE NBAMRDEC_FreeCompResources(OMX_HANDLETYPE pComponent);

/* =================================================================================== */
/**
* Instrumentation info
*/
/* ================================================================================== */

typedef struct _NBAMRDEC_BUFFERLIST NBAMRDEC_BUFFERLIST;

/* =================================================================================== */
/**
* Structure for buffer list
*/
/* ================================================================================== */
struct _NBAMRDEC_BUFFERLIST{
    OMX_BUFFERHEADERTYPE *pBufHdr[MAX_NUM_OF_BUFS]; /* records buffer header send by client */
    OMX_U32 bufferOwner[MAX_NUM_OF_BUFS];
    OMX_U32 bBufferPending[MAX_NUM_OF_BUFS];
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
typedef struct NBAMRDEC_BUFDATA {
   OMX_U8 nFrames;     
}NBAMRDEC_BUFDATA;

/* =================================================================================== */
/**
* Component private data
*/
/* ================================================================================== */
typedef struct AMRDEC_COMPONENT_PRIVATE
{
    /** Array of pointers to BUFFERHEADERTYPE structues
       This pBufHeader[NBAMRDEC_INPUT_PORT] will point to all the
       BUFFERHEADERTYPE structures related to input port,
       not just one structure. Same is for output port
       also. */

#ifdef __PERF_INSTRUMENTATION__
    PERF_OBJHANDLE pPERF, pPERFcomp;
    OMX_U32 nLcml_nCntIp;
    OMX_U32 nLcml_nCntOpReceived;
#endif

    OMX_BUFFERHEADERTYPE* pBufHeader[NUM_OF_PORTS];

    BUFFERHEADERTYPE_INFO BufInfo[NUM_OF_PORTS];

    /** Structure of callback pointers */
    OMX_CALLBACKTYPE cbInfo;

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

    /** NBAMR Component Parameters */
    OMX_AUDIO_PARAM_AMRTYPE* amrParams[NUM_OF_PORTS]; /*amrParams[Output] = OMX_AUDIO_PARAM_PCMMODETYPE*/

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

    /** LCML Handle */
    OMX_HANDLETYPE pLcmlHandle;

    /** LCML Buffer Header */
    LCML_NBAMRDEC_BUFHEADERTYPE *pLcmlBufHeader[2];

    /** Flag for mime mode */
    OMX_S16 iAmrMimeFlag;

    /** Sampling Frequeny */
    OMX_S16 iAmrSamplingFrequeny;

    /** Number of channels */
    OMX_U32 iAmrChannels;

    /** Flag for Amr mode */
    OMX_S16 iAmrMode;

    /** Flag for DASF mode */
    OMX_S16 dasfmode;

    /** Flag for mime mode */
    OMX_S16 mimemode;

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

    /** Mark data */
    OMX_PTR pMarkData;

    /** Mark buffer */
    OMX_MARKTYPE *pMarkBuf;

    /** Mark target component */
   OMX_HANDLETYPE hMarkTargetComponent;

   /** Flag set when buffer should not be queued to the DSP */
   OMX_U32 bBypassDSP;

   /** Input buffer list */
   NBAMRDEC_BUFFERLIST *pInputBufferList;

   /** Output buffer list */
   NBAMRDEC_BUFFERLIST *pOutputBufferList;

   /** LCML stream attributes */
   LCML_STRMATTR *strmAttr;

   /** Component version */
   OMX_U32 nVersion;

   /** Play Complete Flag */
   OMX_U32 bPlayCompleteFlag;

   /** NBAMR Mime Bytes */
   OMX_U32 amrMimeBytes[NUM_MIME_BYTES_ARRAY];
   
   /**NBAMR IF2 Bytes**/
   OMX_U32 amrIF2Bytes[NUM_IF2_BYTES_ARRAY];

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
   AMRDEC_AudioCodecParams *pParams;

   /** Flag for Init Params Initialized */
   OMX_U32 bInitParamsInitialized;

   /** Flag for bIdleCommandPending */
 /*  OMX_U32 bIdleCommandPending;  */

   /** Array of Input Buffers that are pending to sent due State = Idle */
   OMX_BUFFERHEADERTYPE *pInputBufHdrPending[MAX_NUM_OF_BUFS];

   /** Number of Input Buffers that are pending to sent due State = Idle */
   OMX_U32 nNumInputBufPending;

   /** Array of Output Buffers that are pending to sent due State = Idle */
   OMX_BUFFERHEADERTYPE *pOutputBufHdrPending[MAX_NUM_OF_BUFS];

   /** Number of Output Buffers that are pending to sent due State = Idle */
   OMX_U32 nNumOutputBufPending;

   /** Flag for bDisableCommandPending*/
   OMX_U32 bDisableCommandPending;
   
   OMX_U32 bEnableCommandPending;

   /** Flag for bDisableCommandParam*/
   OMX_U32 bDisableCommandParam;
   
   OMX_U32 bEnableCommandParam;

    /** Flag to set when socket node stop callback should not transition
        component to OMX_StateIdle */
    OMX_U32 bNoIdleOnStop;

    /** Number of outstanding FillBufferDone() calls */
    OMX_U32 nOutStandingFillDones;

    /** Stop Codec Command Sent Flag*/
    OMX_U8 bStopSent;

    OMX_U32 nRuntimeInputBuffers;

    OMX_U32 nRuntimeOutputBuffers;

    /* Removing sleep() calls. Definition. */
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
    /* Removing sleep() calls. Definition. */

    OMX_U8 PendingPausedBufs;
    OMX_BUFFERHEADERTYPE *pOutputBufHdrPausedPending[MAX_NUM_OF_BUFS];

    OMX_BUFFERHEADERTYPE *LastOutbuf;

    OMX_BOOL bIsInvalidState;
    OMX_STRING* sDeviceString;

    void* ptrLibLCML;
    
    /** Circular array to keep buffer timestamps */
    OMX_S64 arrBufIndex[MAX_NUM_OF_BUFS]; 
    /** Circular array to keep buffer nTickCounts */
    OMX_S64 arrTickCount[MAX_NUM_OF_BUFS]; 
    /** Index to arrBufIndex[], used for input buffer timestamps */
    OMX_U8 IpBufindex;
    /** Index to arrBufIndex[], used for output buffer timestamps */
    OMX_U8 OpBufindex;

    /** Flag to flush SN after EOS in order to process more buffers after EOS**/
    OMX_U8 SendAfterEOS;    

    /** Flag to mark the first sent buffer**/
    OMX_U8 first_buff;
    /** First Time Stamp sent **/
    OMX_TICKS first_TS;

    /** Temporal time stamp **/
    OMX_TICKS temp_TS;

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

    /** Indicate when first output buffer received from DSP **/
    OMX_U32 first_output_buf_rcv;

} AMRDEC_COMPONENT_PRIVATE;

typedef enum OMX_NBAMRDEC_INDEXAUDIOTYPE {
        OMX_IndexCustomNbAmrDecModeEfrConfig = 0xFF000001,
        OMX_IndexCustomNbAmrDecModeAmrConfig,
        OMX_IndexCustomNbAmrDecModeAcdnConfig,
        OMX_IndexCustomNbAmrDecModeDasfConfig,
        OMX_IndexCustomNbAmrDecModeMimeConfig,
        OMX_IndexCustomNbAmrDecHeaderInfoConfig,
        OMX_IndexCustomNbAmrDecStreamIDConfig,
        OMX_IndexCustomNbAmrDecDataPath,
        OMX_IndexCustomNbAmrDecNextFrameLost,
        OMX_IndexCustomDebug
}OMX_NBAMRDEC_INDEXAUDIOTYPE;

/*=======================================================================*/
/*! @fn SignalIfAllBuffersAreReturned 

 * @brief Sends pthread signal to indicate OMX has returned all buffers to app 

 * @param  none 

 * @Return void 

 */
/*=======================================================================*/
void SignalIfAllBuffersAreReturned(AMRDEC_COMPONENT_PRIVATE *pComponentPrivate);

#endif /* OMX_AMRDECODER_H */
