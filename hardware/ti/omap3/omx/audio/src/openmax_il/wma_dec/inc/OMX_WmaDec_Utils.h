
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
 * @file OMX_WmaDecUtils.h
 *
 * This is an header file for an audio WMA Decoder that is fully
 * compliant with the Khronos OpenMAX 1.0 specification.
 * This the file that the application that uses OMX would include
 * in its code.
 *
 * @path $(CSLPATH)\
 *
 * @rev 1.0
 */
/* --------------------------------------------------------------------------- */

#ifndef OMX_WMADEC_UTILS__H
#define OMX_WMADEC_UTILS__H
#define OMX_WMADECODER_H
#include <pthread.h>

#include <OMX_TI_Common.h>
#include <TIDspOmx.h>
#include "LCML_DspCodec.h"
#define _ERROR_PROPAGATION__ 

#ifdef __PERF_INSTRUMENTATION__
#include "perf.h"
#endif

#ifdef RESOURCE_MANAGER_ENABLED
#include <ResourceManagerProxyAPI.h>
#endif

#include <OMX_Component.h> 

#ifndef ANDROID
    #define ANDROID
#endif

#ifdef ANDROID
    #undef LOG_TAG
    #define LOG_TAG "OMX_WMADEC"

/* PV opencore capability custom parameter index */
    #define PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX 0xFF7A347
#endif

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

typedef struct OMXBufferStatus /*BUFFERSTATUS*/
{
    DWORD EmptyBufferSent;
    DWORD FillBufferSent;
    DWORD EmptyBufferDone;
    DWORD FillBufferDone;
} OMXBufferStatus;

#endif

/* PV opencore capability custom parameter index */
#define PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX 0xFF7A347

#ifndef ANDROID
#define ANDROID
#endif

#define OBJECTTYPE_LC 2
#define OBJECTTYPE_HE 5
#define OBJECTTYPE_HE2 29


/* ======================================================================= */
/**
 * @def    WMADEC_COMPONENT_THREAD
 */
/* ======================================================================= */
#define EXIT_COMPONENT_THRD  10
/* ======================================================================= */
/**
 * @def    WMADEC_XXX_VER    Component version
 */
/* ======================================================================= */
#define  WMADEC_MAJOR_VER 0xF1
#define  WMADEC_MINOR_VER 0xF2
/* ======================================================================= */
/**
 * @def    INPUT_WMADEC_BUFFER_SIZE   Default input buffer size 
 *                                     
 */
/* ======================================================================= */
#define INPUT_WMADEC_BUFFER_SIZE 16384

/* ======================================================================= */
/**
 * @def    OUTPUT_WMADEC_BUFFER_SIZE   Default output buffer size          
 */
/* ======================================================================= */
#define OUTPUT_WMADEC_BUFFER_SIZE 40960
/* ======================================================================= */
/**
 * @def    NUM_WMADEC_INPUT_BUFFERS   Default number of input buffers               
 */
/* ======================================================================= */
#define NUM_WMADEC_INPUT_BUFFERS 2
/* ======================================================================= */
/**
 * @def    NUM_WMADEC_OUTPUT_BUFFERS   Default number of output buffers                                   
 */
/* ======================================================================= */
#ifdef UNDER_CE
#define NUM_WMADEC_OUTPUT_BUFFERS 4
#else
#define NUM_WMADEC_OUTPUT_BUFFERS 4
#endif
/* ======================================================================= */
/**
 * @def    NOT_USED    Defines a value for "don't care" parameters
 */
/* ======================================================================= */
#define NOT_USED 10
/* ======================================================================= */
/**
 * @def    NORMAL_BUFFER    Defines the flag value with all flags turned off
 */
/* ======================================================================= */
#define NORMAL_BUFFER 0
/* ======================================================================= */
/**
 * @def    OMX_WMADEC_DEFAULT_SEGMENT    Default segment ID for the LCML
 */
/* ======================================================================= */
#define OMX_WMADEC_DEFAULT_SEGMENT (0)
/* ======================================================================= */
/**
 * @def    OMX_WMADEC_SN_TIMEOUT    Timeout value for the socket node
 */
/* ======================================================================= */
#define OMX_WMADEC_SN_TIMEOUT (-1)
/* ======================================================================= */
/**
 * @def    OMX_WMADEC_SN_PRIORITY   Priority for the socket node
 */
/* ======================================================================= */
#define OMX_WMADEC_SN_PRIORITY (10)
/* ======================================================================= */
/**
 * @def    WMAD_TIMEOUT   Timeout value for the component thread
 */
/* ======================================================================= */
#define WMAD_TIMEOUT (1000) /* millisecs */
/* ======================================================================= */
/**
 * @def    NUM_WMADEC_OUTPUT_BUFFERS_DASF   Number of output buffers for DASF
 */
/* ======================================================================= */
#define NUM_WMADEC_OUTPUT_BUFFERS_DASF 2
/* ======================================================================= */
/**
 * @def    WMADEC_STREAM_COUNT   Number of streams
 */
/* ======================================================================= */
#define WMADEC_STREAM_COUNT 2
/* ======================================================================= */
/**
 * @def    WMADEC_DEFAULT_SAMPLING_FREQUENCY   Sampling frequency
 */
/* ======================================================================= */
#define WMADEC_DEFAULT_SAMPLING_FREQUENCY 8000
/* ======================================================================= */
/**
 * @def    WMADEC_MAX_NUM_OF_BUFS   Maximum number of buffers
 */
/* ======================================================================= */
#define MAX_NUM_OF_BUFS 10
#define WMA_CPU 45
/* ======================================================================= */
/**
 * @def    WMADEC_DEFAULT_PACKETS_DWLO  Default Packets Number
 */
/* ======================================================================= */
#define WMADEC_DEFAULT_PACKETS_DWLO 178
/* ======================================================================= */
/**
 * @def    WMADEC_DEFAULT_PLAYDURATION_DWLO   Default Play Duration
 */
/* ======================================================================= */
#define WMADEC_DEFAULT_PLAYDURATION_DWLO 917760000 
/* ======================================================================= */
/**
 * @def    WMADEC_DEFAULT_MAXPACKETSIZE   Default Maximum Packet Size
 */
/* ======================================================================= */
#define WMADEC_DEFAULT_MAXPACKETSIZE 349
/* ======================================================================= */
/**
 * @def    WMADEC_DEFAULT_STREAMTYPE_DATA1   Default Stream Data Type values
 */
/* ======================================================================= */
//Macros for WMA
#define GetUnalignedWord( pb, w )               \
    (w) = ((OMX_U16) *(pb + 1) << 8) + *pb;

#define GetUnalignedDword( pb, dw )             \
    (dw) = ((OMX_U32) *(pb + 3) << 24) +        \
        ((OMX_U32) *(pb + 2) << 16) +           \
        ((OMX_U16) *(pb + 1) << 8) + *pb;

#define GetUnalignedWordEx( pb, w )     GetUnalignedWord( pb, w ); (pb) += sizeof(OMX_U16);
#define GetUnalignedDwordEx( pb, dw )   GetUnalignedDword( pb, dw ); (pb) += sizeof(OMX_U32);
#define LoadWORD( w, p )    GetUnalignedWordEx( p, w )
#define LoadDWORD( dw, p )  GetUnalignedDwordEx( p, dw )


#define WMADEC_DEFAULT_STREAMTYPE_DATA1 -127295936 
#define WMADEC_DEFAULT_STREAMTYPE_DATA2 23373
#define WMADEC_DEFAULT_STREAMTYPE_DATA3 4559
#define WMADEC_DEFAULT_STREAMTYPE_DATA40 168
#define WMADEC_DEFAULT_STREAMTYPE_DATA41 253
#define WMADEC_DEFAULT_STREAMTYPE_DATA42 0
#define WMADEC_DEFAULT_STREAMTYPE_DATA43 128
#define WMADEC_DEFAULT_STREAMTYPE_DATA44 95
#define WMADEC_DEFAULT_STREAMTYPE_DATA45 92
#define WMADEC_DEFAULT_STREAMTYPE_DATA46 68
#define WMADEC_DEFAULT_STREAMTYPE_DATA47 43

/* ======================================================================= */
/**
 * @def    WMADEC_DEFAULT_TYPESPECIFIC    Default Specific Type
 */
/* ======================================================================= */
#define WMADEC_DEFAULT_TYPESPECIFIC 28
/* ======================================================================= */
/**
 * @def    WMADEC_DEFAULT_STREAMNUM     Default Stream Number
 */
/* ======================================================================= */
#define WMADEC_DEFAULT_STREAMNUM 1
/* ======================================================================= */
/**
 * @def    WMADEC_DEFAULT_FORMATTAG    Default Format tag
 */
/* ======================================================================= */
#define WMADEC_DEFAULT_FORMATTAG 353
/* ======================================================================= */
/**
 * @def    WMADEC_DEFAULT_SAMPLEPERSEC    Default samples per second
 */
/* ======================================================================= */
#define WMADEC_DEFAULT_SAMPLEPERSEC 8000
/* ======================================================================= */
/**
 * @def    WMADEC_DEFAULT_AVGBYTESPERSEC    Default average bytes per second
 */
/* ======================================================================= */
#define WMADEC_DEFAULT_AVGBYTESPERSEC 625
/* ======================================================================= */
/**
 * @def    WMADEC_DEFAULT_BLOCKALIGN    Default block alignment
 */
/* ======================================================================= */
#define WMADEC_DEFAULT_BLOCKALIGN 40
/* ======================================================================= */
/**
 * @def    WMADEC_DEFAULT_CHANNEL    Default channels number
 */
/* ======================================================================= */
#define WMADEC_DEFAULT_CHANNEL 1
/* ======================================================================= */
/**
 * @def    WMADEC_DEFAULT_VALIDBITSPERSAMPLE    Default valid bits per sample
 */
/* ======================================================================= */
#define WMADEC_DEFAULT_VALIDBITSPERSAMPLE 16
/* ======================================================================= */
/**
 * @def    WMADEC_DEFAULT_SIZEWAVEHEADER    Default wave header size
 */
/* ======================================================================= */
#define WMADEC_DEFAULT_SIZEWAVEHEADER 10
/* ======================================================================= */
/**
 * @def    WMADEC_DEFAULT_CHANNELMASK    Default channel mask
 */
/* ======================================================================= */
#define WMADEC_DEFAULT_CHANNELMASK 0
/* ======================================================================= */
/**
 * @def    WMADEC_DEFAULT_ENCODEOPTV    Default encode option
 */
/* ======================================================================= */
#define WMADEC_DEFAULT_ENCODEOPTV 0 
/* ======================================================================= */
/**
 * @def    WMADEC_DEFAULT_VALIDBITSPERSAMPLE    Default valid bits per sample
 */
/* ======================================================================= */
#define WMADEC_DEFAULT_VALIDBITSPERSAMPLE 16
/* ======================================================================= */
/**
 * @def    WMADEC_DEFAULT_SAMPLEPERBLOCK    Default samples per block
 */
/* ======================================================================= */
#define WMADEC_DEFAULT_SAMPLEPERBLOCK 8704
/**
 * @def    WMADEC_DEBUG   Turns debug messaging on and off
 */
/* ======================================================================= */
#undef WMADEC_DEBUG
/* ======================================================================= */
/**
 * @def    WMADEC_MEMCHECK   Turns memory messaging on and off
 */
/* ======================================================================= */
#undef WMADEC_MEMCHECK
/* ======================================================================= */
/**
 * @def    WMADEC_USN_DLL_NAME   USN DLL name
 */
/* ======================================================================= */
#ifdef UNDER_CE
#define WMADEC_USN_DLL_NAME "\\windows\\usn.dll64P"
#else
#define WMADEC_USN_DLL_NAME "usn.dll64P"
#endif
/* ======================================================================= */
/**
 * @def    WMADEC_DLL_NAME   WMA Decoder socket node dll name
 */
/* ======================================================================= */
#ifdef UNDER_CE
#define WMADEC_DLL_NAME "\\windows\\wmadec_sn.dll64P"
#else
#define WMADEC_DLL_NAME "wmadec_sn.dll64P"
#endif
/* ======================================================================= */
/**
 * @def    WMADEC_EPRINT   Error print macro
 */
/* ======================================================================= */
#ifndef UNDER_CE
#define WMADEC_EPRINT ALOGE
#else
#define WMADEC_EPRINT		  printf
#endif
/* ======================================================================= */
/**
 * @def    WMADEC_DPRINT   Debug print macro
 */
/* ======================================================================= */
#ifndef UNDER_CE
#ifdef  WMADEC_DEBUG
#define WMADEC_DPRINT ALOGI
#else
#define WMADEC_DPRINT(...)
#endif

#ifdef  WMADEC_MEMCHECK
#define WMADEC_MEMPRINT(...)    fprintf(stderr,__VA_ARGS__)
#else
#define WMADEC_MEMPRINT(...)
#endif


#ifdef  WMADEC_DEBUG_MCP
#define WMADEC_MCP_DPRINT(...)    fprintf(stderr,__VA_ARGS__)
#else
#define WMADEC_MCP_DPRINT(...)
#endif

#else /*UNDER_CE*/
#ifdef  WMADEC_DEBUG
#define WMADEC_DPRINT(STR, ARG...) printf()
#else
#define WMADEC_DPRINT 
#endif
/* ======================================================================= */
/**
 * @def    WMADEC_MEMCHECK   Memory print macro
 */
/* ======================================================================= */
#ifdef WMADEC_MEMCHECK
#define WMADEC_MEMPRINT(STR, ARG...) printf()
#else
#define WMADEC_MEMPRINT 
#endif

#endif
/* ======================================================================= */
/**
 * @def    WMADEC_NUM_OF_PORTS   Number of ports
 */
/* ======================================================================= */
#define WMADEC_NUM_OF_PORTS 2
/* ======================================================================= */
/**
 *  W M A       T Y P E S
 */
/* ======================================================================= */


#define WAVE_FORMAT_MSAUDIO1  0x0160
#define WAVE_FORMAT_WMAUDIO2  0x0161
#define WAVE_FORMAT_WMAUDIO3  0x0162
#define WAVE_FORMAT_WMAUDIO_LOSSLESS  0x0163
#define WAVE_FORMAT_WMAUDIO2_ES  0x0165
#define WAVE_FORMAT_WMASPDIF 0x164
#define WAVE_FORMAT_WMAUDIO3_ES  0x0166
#define WAVE_FORMAT_WMAUDIO_LOSSLESS_ES  0x0167
#define WAVE_FORMAT_MSSPEECH  10


/* According with the ASF Specification:*/
#define WAVE_FORMAT_MSAUDIO              0x0161  /*Versions 7,8 and 9 Series*/
#define WAVE_FORMAT_MSAUDIO_9            0x0162  /* 9 series                 */
#define WAVE_FORMAT_MSAUDIO_9_LOOSELESS  0x0163  /* 9 series                  */

/* ======================================================================= */
/** COMP_PORT_TYPE  Port types
 *
 *  @param  INPUT_PORT                   Input port
 *
 *  @param  OUTPUT_PORT                  Output port
 */
/*  ==================================================================== */
typedef enum COMP_PORT_TYPE {
    INPUT_PORT = 0,
    OUTPUT_PORT
}COMP_PORT_TYPE;
/* ======================================================================= */
/** StreamType  Stream types
 *
 *  @param  DMM                  DMM
 *
 *  @param  INSTRM               Input stream
 *
 *  @param  OUTSTRM              Output stream
 */
/*  ==================================================================== */
enum StreamType
    {
        DMM,
        INSTRM,
        OUTSTRM
    };

typedef OMX_ERRORTYPE (*fpo)(OMX_HANDLETYPE);

/* =================================================================================== */
/**
 * Socket node input parameters.
 */
/* ================================================================================== */
typedef struct WMADEC_AudioCodecParams
{
    unsigned long iSamplingRate;
    unsigned long iStrmId;
    unsigned short iAudioFormat;

}WMADEC_AudioCodecParams;

typedef enum {
    WMA_IAUDIO_BLOCK=0,
    WMA_IAUDIO_INTERLEAVED
} WMAAUDIO_PcmFormat;


#define WMA_MONO_CHANNEL                0x0001
#define WMA_STEREO_INTERLEAVED          0x0002
#define WMA_STEREO_NON_INTERLEAVED      0x0003
#define WMA_MONO_DUPLICATED             0x0004

typedef enum {
    WMA_IUALG_CMD_STOP          = 0,
    WMA_IUALG_CMD_PAUSE         = 1,
    WMA_IUALG_CMD_GETSTATUS     = 2,
    WMA_IUALG_CMD_SETSTATUS     = 3,
    WMA_IUALG_CMD_USERCMDSTART  = 100
}WMA_IUALGUALG_Cmd;


typedef struct {
    OMX_U16 bLastBuffer;
}WMADEC_UAlgInBufParamStruct;

typedef struct 
{   
    OMX_U32      size;              
    OMX_S32      iOutputFormat;     
} WMADEC_UALGParams;

typedef struct {
    /* Number of frames in a buffer */
    unsigned long ulFrameCount;
    bool ulIsLastBuffer;
}WMADEC_UAlgOutBufParamStruct;
/* =================================================================================== */
/**
 * WMA Buffer Header Type
 */
/* ================================================================================== */
typedef struct LCML_WMADEC_BUFHEADERTYPE {
    OMX_DIRTYPE eDir;
    OMX_BUFFERHEADERTYPE* buffer;
    WMADEC_UAlgInBufParamStruct *pIpParam;
    /* Output Parameter Information structure */
    WMADEC_UAlgOutBufParamStruct *pOpParam;
}LCML_WMADEC_BUFHEADERTYPE;

/* =================================================================================== */
/**
 * Structure for buffer list
 */
/* ================================================================================== */
typedef struct _BUFFERLIST BUFFERLIST;
struct _BUFFERLIST{
    OMX_U16 numBuffers; 
    OMX_BUFFERHEADERTYPE *pBufHdr[MAX_NUM_OF_BUFS]; /* records buffer header send by client */ 
    OMX_U32 bufferOwner[MAX_NUM_OF_BUFS];
    OMX_U32 bBufferPending[MAX_NUM_OF_BUFS];
    OMX_U8 EosFlagSent;
};


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
/**
 *  RCA_HEADER. Rca data that goes to SN with the first data packet received from test app.
 */
/* ================================================================================== */
typedef struct RCA_HEADER
{         
    QWORD					iPackets;	
    QWORD					iPlayDuration;
    OMX_U32 				iMaxPacketSize; 
    WMADECGUID				iStreamType;	
    OMX_U32					iTypeSpecific;  
    OMX_U16					iStreamNum;     
    OMX_U16					iFormatTag;     
    OMX_U16					iChannel;
    OMX_U32					iSamplePerSec;  
    OMX_U32					iAvgBytesPerSec;
    OMX_U16					iBlockAlign;    
    OMX_U16					iValidBitsPerSample;
    OMX_U16					iNotUsed;
    OMX_U32					iSamplesPerBlock;
    OMX_U16					iEncodeOptV;
    OMX_U32					iNotUsed2;    
    OMX_U8					iReplicatedDataSize;
    OMX_U32                 iPayload;
} RCA_HEADER;
/* =================================================================================== */
/**
 * Component private data
 */
/* ================================================================================== */
typedef struct WMADEC_COMPONENT_PRIVATE
{
    /** Input buffer list */
    BUFFERLIST *pInputBufferList;

    /** Number of input buffers at runtime */
    OMX_U32 nRuntimeInputBuffers;

    /** Number of output buffers at runtime */
    OMX_U32 nRuntimeOutputBuffers;

    /** Output buffer list */
    BUFFERLIST *pOutputBufferList;

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
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef[WMADEC_NUM_OF_PORTS];

    /** WMA Component Parameters */
    OMX_AUDIO_PARAM_WMATYPE* wmaParams[WMADEC_NUM_OF_PORTS];
    

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


    /** LCML input buffers received */
    OMX_U32 lcml_nCntIp;

    /** LCML output buffers received */
    OMX_U32 lcml_nCntOpReceived;

#ifdef __PERF_INSTRUMENTATION__
    PERF_OBJHANDLE pPERF, pPERFcomp;
    OMX_U32 nLcml_nCntIp;         
    OMX_U32 nLcml_nCntOpReceived;
#endif

    /** LCML Handle */
    OMX_HANDLETYPE pLcmlHandle;

    /** LCML Buffer Header */
    LCML_WMADEC_BUFHEADERTYPE *pLcmlBufHeader[2];

    /** Sampling frequency */
    int iWmaSamplingFrequeny;

    /** Number of channels */
    int iWmaChannels;

    /** Flag for DASF mode */
    int dasfmode;

    /** Flag set when port definitions are allocated */
    OMX_U32 bPortDefsAllocated;

    /** Flag set when component thread is started */
    OMX_U32 bCompThreadStarted;

    /** Mark data */
    OMX_PTR pMarkData;          

    /** Mark buffer */
    OMX_MARKTYPE *pMarkBuf;    

    /** Mark target component */
    OMX_HANDLETYPE hMarkTargetComponent; 

    /** Flag set when buffer should not be queued to the DSP */
    OMX_U32 bBypassDSP;


    /** LCML stream attributes */
    LCML_STRMATTR *strmAttr;

    /** Component version */
    OMX_U32 nVersion;

    /** WMA Header Info */
    WMA_HeadInfo* pHeaderInfo;

    /** WMA Header Info */
    TI_OMX_DSP_DEFINITION* pDspDefinition;

    /** Flag set when LCML handle is opened */
    int bLcmlHandleOpened;

    /** Keeps track of the number of EmptyThisBuffer() calls */
    OMX_U32 nEmptyThisBufferCount;

    /** Keeps track of the number of EmptyBufferDone() calls */
    OMX_U32 nEmptyBufferDoneCount;

    /** Flag set when init params have been initialized */
    OMX_U32 bInitParamsInitialized;

    /** Stores input buffers while paused */
    OMX_BUFFERHEADERTYPE *pInputBufHdrPending[MAX_NUM_OF_BUFS];

    /** Number of input buffers received while paused */
    OMX_U32 nNumInputBufPending;

    /** Stores output buffers while paused */   
    OMX_BUFFERHEADERTYPE *pOutputBufHdrPending[MAX_NUM_OF_BUFS];

    /** Number of output buffers received while paused */
    OMX_U32 nNumOutputBufPending;

    /** Keeps track of the number of invalid frames that come from the LCML */
    OMX_U32 nInvalidFrameCount;

    /** Flags to control port enable command **/
    OMX_U32 bEnableCommandPending;  

    /** Flag set when a disable command is pending */
    OMX_U32 bDisableCommandPending;

    /** Parameter for pending disable command */
    OMX_U32 bEnableCommandParam;

    /** Parameter for pending disable command */
    OMX_U32 bDisableCommandParam;

    /** Flag to set when socket node stop callback should not transition
        component to OMX_StateIdle */
    OMX_U32 bNoIdleOnStop;

    /** Flag set when idle command is pending */
    OMX_U32 bIdleCommandPending;

    /** Flag set when socket node is stopped */
    OMX_U32 bDspStoppedWhileExecuting;

    /** Number of outstanding FillBufferDone() calls */
    OMX_S32 nOutStandingFillDones;

    /** ID of stream */
    OMX_U32 streamID;

    /** Flag set when get status is pending */
    OMX_U32 bGetStatusPending;

    /** Pointer to the last output buffer header queued */
    OMX_BUFFERHEADERTYPE* LastOutputBufferHdrQueued;
	
    /** Pointer to WMADEC_AudioCodecParams */
    WMADEC_AudioCodecParams *pParams;
    
    /* Pointer to WMADEC_UALGParams */
    WMADEC_UALGParams *pDynParams;

    /* Device string */
    OMX_STRING* sDeviceString;

    /**Keep buffer tickcount*/
    OMX_U32 arrBufIndexTick[MAX_NUM_OF_BUFS]; 
    
    /** Keep buffer timestamps **/
    OMX_S64 arrBufIndex[MAX_NUM_OF_BUFS];

    /** Index to arrBufIndex[], used for input buffer timestamps */
    OMX_U8 IpBufindex;

    /** Index to arrBufIndex[], used for output buffer timestamps */
    OMX_U8 OpBufindex;

    /** Flag to flush SN after EOS in order to process more buffers after EOS**/
    OMX_U8 SendAfterEOS;		

    OMX_U8 InputEosSet;

    OMX_BOOL bPreempted;

#ifdef RESOURCE_MANAGER_ENABLED
    RMPROXY_CALLBACKTYPE rmproxyCallback;
#endif
	
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

    pthread_mutex_t codecStop_mutex;    
    pthread_cond_t codecStop_threshold;
    OMX_U8 codecStop_waitingsignal;

    pthread_mutex_t codecFlush_mutex;    
    pthread_cond_t codecFlush_threshold;
    OMX_U8 codecFlush_waitingsignal;
    
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
    OMX_BOOL bIsInvalidState;
    void* PtrCollector[6];        
    /* Removing sleep() calls. Definition. */
    OMX_BOOL bLoadedCommandPending;
    OMX_PARAM_COMPONENTROLETYPE componentRole;
    /** Count of number of buffers outstanding with bridge */
    OMX_U32 lcml_nIpBuf;
    /** Count of number of buffers outstanding with bridge */
    OMX_U32 lcml_nOpBuf;
    OMX_U32 app_nBuf;
    OMX_U32 num_Reclaimed_Op_Buff;
    
    PV_OMXComponentCapabilityFlagsType iPVCapabilityFlags;
    OMX_BOOL reconfigInputPort;     
    OMX_BOOL reconfigOutputPort;    
    OMX_BOOL bConfigData;           
    
    OMX_AUDIO_PARAM_WMATYPE *wma_ip;
    
    OMX_AUDIO_PARAM_PCMMODETYPE *wma_op;
    	
    OMX_U8 first_buffer;	
	
    RCA_HEADER *rcaheader;

    struct OMX_TI_Debug dbg;        

    OMX_BUFFERHEADERTYPE *lastout;

} WMADEC_COMPONENT_PRIVATE;
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

OMX_ERRORTYPE OMX_ComponentInit (OMX_HANDLETYPE hComp);

/* ===========================================================  */
/**
 *  WMADEC_StartComponentThread()  Starts component thread
 * 
 *
 *  @param hComp         OMX Handle
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 *
 */
/*================================================================== */
OMX_ERRORTYPE WMADEC_StartComponentThread(OMX_HANDLETYPE pHandle);

/* ===========================================================  */
/**
 *  WMADEC_StopComponentThread()  Stops component thread
 * 
 *
 *  @param hComp         OMX Handle
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 *
 */
/*================================================================== */
OMX_ERRORTYPE WMADEC_StopComponentThread(OMX_HANDLETYPE pHandle);

/* ===========================================================  */
/**
 *  WMADEC_FreeCompResources()  Frees allocated memory
 * 
 *
 *  @param hComp         OMX Handle
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 *
 */
/*================================================================== */
OMX_ERRORTYPE WMADEC_FreeCompResources(OMX_HANDLETYPE pComponent);

/* ===========================================================  */
/**
 *  WMADEC_GetCorresponding_LCMLHeader()  Returns LCML header
 * that corresponds to the given buffer
 *
 *  @param pComponentPrivate Component private data
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 */
/*================================================================== */
OMX_ERRORTYPE WMADECGetCorresponding_LCMLHeader(WMADEC_COMPONENT_PRIVATE *pComponentPrivate,
                                                OMX_U8 *pBuffer,
                                                OMX_DIRTYPE eDir,
                                                LCML_WMADEC_BUFHEADERTYPE **ppLcmlHdr);

/* ===========================================================  */
/**
 *  WMADEC_LCML_Callback() Callback from LCML
 *
 *  @param event     Codec Event
 *
 *  @param args      Arguments from LCML
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 */
/*================================================================== */
OMX_ERRORTYPE WMADECLCML_Callback (TUsnCodecEvent event,void * args [10]);

/* ===========================================================  */
/**
 *  WMADEC_Fill_LCMLInitParams() Fills the parameters needed
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
/*================================================================== */
OMX_ERRORTYPE WMADECFill_LCMLInitParams(OMX_COMPONENTTYPE* pComponent,
                                        LCML_DSP *plcml_Init, OMX_U16 arr[]);


/* ===========================================================  */
/**
 *  WMADEC_GetBufferDirection() Returns direction of pBufHeader
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
/*================================================================== */
OMX_ERRORTYPE WMADECGetBufferDirection(OMX_BUFFERHEADERTYPE *pBufHeader, OMX_DIRTYPE *eDir);


/* ===========================================================  */
/**
 *  WMADECHandleCommand()  Handles commands sent via SendCommand()
 *
 *  @param pComponentPrivate Component private data
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 */
/*================================================================== */
OMX_U32 WMADECHandleCommand (WMADEC_COMPONENT_PRIVATE *pComponentPrivate);

/* ===========================================================  */
/**
 *  WMADECFreeLCMLHandle()  Frees the handle to the LCML
 *
 *  @param pComponentPrivate Component private data
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 */
/*================================================================== */
OMX_ERRORTYPE WMADECFreeLCMLHandle(WMADEC_COMPONENT_PRIVATE *pComponentPrivate);

/* ===========================================================  */
/**
 *  WMADECHandleDataBuf_FromApp()  Handles data buffers received
 * from the IL Client
 *
 *  @param pComponentPrivate Component private data
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 */
/*================================================================== */
OMX_ERRORTYPE WMADECHandleDataBuf_FromApp(OMX_BUFFERHEADERTYPE *pBufHeader,
                                          WMADEC_COMPONENT_PRIVATE *pComponentPrivate);


/* ===========================================================  */
/**
 *  WMADECHandleDataBuf_FromLCML()  Handles data buffers received
 * from LCML
 *
 *  @param pComponentPrivate Component private data
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 */
/*================================================================== */
OMX_ERRORTYPE WMADECHandleDataBuf_FromLCML(WMADEC_COMPONENT_PRIVATE* pComponentPrivate, 
                                           LCML_WMADEC_BUFHEADERTYPE* msgBuffer);


/* ===========================================================  */
/**
 *  WMADEC_FreeLCMLHandle()  Frees the handle to the LCML
 *
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 */
/*================================================================== */
OMX_HANDLETYPE WMADECGetLCMLHandle(WMADEC_COMPONENT_PRIVATE *pComponentPrivate);

/* ===========================================================  */
/**
 *  WMADEC_CleanupInitParams()  Starts component thread
 *
 *  @param pComponent        OMX Handle
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 */
/*================================================================== */
OMX_ERRORTYPE WMADEC_CleanupInitParams(OMX_HANDLETYPE pComponent);

/* ===========================================================  */
/**
 *  WMADEC_SetPending()  Called when the component queues a buffer
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
/*================================================================== */
void WMADEC_SetPending(WMADEC_COMPONENT_PRIVATE *pComponentPrivate, 
                       OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir);

/* ===========================================================  */
/**
 *  WMADEC_ClearPending()  Called when a buffer is returned
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
/*================================================================== */
void WMADEC_ClearPending(WMADEC_COMPONENT_PRIVATE *pComponentPrivate, 
                         OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir) ;

/* ===========================================================  */
/**
 *  WMADEC_CommandToIdle()  Called when the component is commanded
 * to idle
 *
 *  @param pComponentPrivate     Component private data
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 */
/*================================================================== */
OMX_ERRORTYPE WMADEC_CommandToIdle(WMADEC_COMPONENT_PRIVATE *pComponentPrivate);

/* ===========================================================  */
/**
 *  WMADEC_CommandToIdle()  Called when the component is commanded
 * to idle
 *
 *  @param pComponentPrivate     Component private data
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 */
/*================================================================== */
OMX_ERRORTYPE WMADEC_CommandToLoaded(WMADEC_COMPONENT_PRIVATE *pComponentPrivate);

/* ===========================================================  */
/**
 *  WMADEC_CommandToExecuting()  Called when the component is commanded
 * to executing
 *
 *  @param pComponentPrivate     Component private data
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 */
/*================================================================== */
OMX_ERRORTYPE WMADEC_CommandToExecuting(WMADEC_COMPONENT_PRIVATE *pComponentPrivate);

/* ===========================================================  */
/**
 *  WMADEC_CommandToPause()  Called when the component is commanded
 * to paused
 *
 *  @param pComponentPrivate     Component private data
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 */
/*================================================================== */
OMX_ERRORTYPE WMADEC_CommandToPause(WMADEC_COMPONENT_PRIVATE *pComponentPrivate);

/* ===========================================================  */
/**
 *  WMADEC_CommandToWaitForResources()  Called when the component is commanded
 * to WaitForResources
 *
 *  @param pComponentPrivate     Component private data
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 */
/*================================================================== */
OMX_ERRORTYPE WMADEC_CommandToWaitForResources(WMADEC_COMPONENT_PRIVATE *pComponentPrivate);

/* ===========================================================  */
/**
 *  WMADEC_IsPending()  
 * 
 *
 *  @param pComponentPrivate     Component private data
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 */
/*================================================================== */
OMX_U32 WMADEC_IsPending(WMADEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir);

/* ===========================================================  */
/**
 *  WMADEC_Fill_LCMLInitParamsEx()  Fills the parameters needed
 * to initialize the LCML without recreating the socket node
 *
 *  @param pComponent            OMX Handle
 *
 *  @return None
 */

/*================================================================== */
OMX_ERRORTYPE WMADECFill_LCMLInitParamsEx(OMX_HANDLETYPE pComponent,OMX_U32 indexport);

/* ===========================================================  */
/**
 *  WMADEC_IsValid() Returns whether a buffer is valid
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
/*================================================================== */
OMX_U32 WMADEC_IsValid(WMADEC_COMPONENT_PRIVATE *pComponentPrivate, 
                       OMX_U8 *pBuffer, OMX_DIRTYPE eDir) ;
/* ===========================================================  */
/**
 *  WMADEC_TransitionToIdle() Transitions component to idle
 * 
 *
 *  @param pComponentPrivate     Component private data
 *
 *  @return OMX_ErrorNone = No error
 *          OMX Error code = Error
 */
/*================================================================== */
OMX_ERRORTYPE WMADEC_TransitionToIdle(WMADEC_COMPONENT_PRIVATE *pComponentPrivate);
/* ===========================================================  */
/**
 *  WMADEC_ComponentThread()  Component thread
 *
 *  @param pThreadData		Thread data
 *
 * @cbData		CallBack for ResourceManagerProxy
 *  @return None
 *
 */
/*================================================================== */
void* WMADEC_ComponentThread (void* pThreadData);
 

/* ======================================================================= */
/** OMX_WMADEC_INDEXAUDIOTYPE  Defines the custom configuration settings
 *                              for the component
 *pHeaderInfo
 *  @param  OMX_IndexCustomWMADECModeDasfConfig      Sets the DASF mode
 *
 *  
 */
/*  ==================================================================== */
typedef enum OMX_WMADEC_INDEXAUDIOTYPE {
    OMX_IndexCustomWMADECModeDasfConfig = 0xFF000001,
    OMX_IndexCustomWMADECHeaderInfoConfig,
    OMX_IndexCustomWmaDecLowLatencyConfig,
    OMX_IndexCustomWmaDecStreamIDConfig,
    OMX_IndexCustomWmaDecDataPath,
    OMX_IndexCustomDebug
}OMX_WMADEC_INDEXAUDIOTYPE;


/*  =========================================================================*/
/*  func    GetBits                                                          */
/*                                                                           */
/*  desc    Gets aBits number of bits from position aPosition of one buffer  */
/*            and returns the value in a TUint value.                        */
/*  =========================================================================*/
OMX_U32 WMADEC_GetBits(OMX_U32* nPosition, OMX_U8 nBits, OMX_U8* pBuffer, OMX_BOOL bIcreasePosition);

/*  =========================================================================*/
/*  func    WMADEC_Parser                                                    */
/*                                                                           */
/*  desc    Gets the info from the Buffer to build a RCA header              */
/*            and returns the RCA Header filled with the data.               */
/*@return OMX_ErrorNone = No error                                           */
/*          OMX Error code = Error                                           */
/*  =========================================================================*/
OMX_ERRORTYPE WMADEC_Parser(OMX_U8* pBuffer, RCA_HEADER *pStreamData, struct OMX_TI_Debug dbg);

/*  =========================================================================*/
/*  func    WMADEC_HandleUSNError                                                                                    */
/*                                                                                                                                              */
/*  desc    Handles error messages returned by the dsp                                                        */
/*                                                                                                                                              */
/*@return n/a                                                                                                                           */
/*                                                                                                                                              */
/*  =========================================================================*/
void WMADEC_HandleUSNError (WMADEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U32 arg);

#ifdef RESOURCE_MANAGER_ENABLED
void WMAD_ResourceManagerCallback(RMPROXY_COMMANDDATATYPE cbData);
#endif

#endif

