
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
 * @file OMX_AacDec_Utils.h
 *
 * This is an header file for an audio AAC decoder that is fully
 * compliant with the OMX Audio specification.
 * This the file is used internally by the component
 * in its code.
 *
 * @path  $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\aac_dec\inc\
 *
 * @rev 1.0
 */
/* --------------------------------------------------------------------------- */
#ifndef OMX_AACDEC_UTILS__H
#define OMX_AACDEC_UTILS__H

#include <OMX_Component.h>
#include <OMX_TI_Common.h>
#include <OMX_TI_Debug.h>
#include "LCML_DspCodec.h"
#include <pthread.h>
#include <sched.h>

#ifdef RESOURCE_MANAGER_ENABLED
#include <ResourceManagerProxyAPI.h>
#endif

#ifdef UNDER_CE
#include <windows.h>
#include <oaf_osal.h>
#include <omx_core.h>
#include <stdlib.h>
#endif
#ifndef UNDER_CE
#define AUDIO_MANAGER
#else
#undef AUDIO_MANAGER
#endif

#ifdef __PERF_INSTRUMENTATION__
#include "perf.h"
#endif

#ifndef ANDROID
    #define ANDROID
#endif

#ifdef ANDROID
    /* Log for Android system*/
    #undef LOG_TAG
    #define LOG_TAG "OMX_AACDEC"

    /* PV opencore capability custom parameter index */
    #define PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX 0xFF7A347
#endif
 
#define OBJECTTYPE_LC 2
#define OBJECTTYPE_LTP 4
#define OBJECTTYPE_HE 5
#define OBJECTTYPE_HE2 29

#define EXIT_COMPONENT_THRD  10


/* ======================================================================= */
/**
 * @def    AAC_DEC__XXX_VER    Component version
 */
/* ======================================================================= */
#define AACDEC_MAJOR_VER 1
#define AACDEC_MINOR_VER 1
/* ======================================================================= */
/**
 * @def    NOT_USED_AACDEC    Defines a value for "don't care" parameters
 */
/* ======================================================================= */
#define NOT_USED_AACDEC 0
/* ======================================================================= */
/**
 * @def    NORMAL_BUFFER_AACDEC    Defines the flag value with all flags turned off
 */
/* ======================================================================= */
#define NORMAL_BUFFER_AACDEC 0
/* ======================================================================= */
/**
 * @def    OMX_AACDEC_DEFAULT_SEGMENT    Default segment ID for the LCML
 */
/* ======================================================================= */
#define OMX_AACDEC_DEFAULT_SEGMENT (0)
/* ======================================================================= */
/**
 * @def    OMX_AACDEC_SN_TIMEOUT    Timeout value for the socket node
 */
/* ======================================================================= */
#define OMX_AACDEC_SN_TIMEOUT (-1)
/* ======================================================================= */
/**
 * @def    OMX_AACDEC_SN_PRIORITY   Priority for the socket node
 */
/* ======================================================================= */
#define OMX_AACDEC_SN_PRIORITY (10)
/* ======================================================================= */
/**
 * @def    OMX_AACDEC_NUM_DLLS   number of DLL's
 */
/* ======================================================================= */
#define OMX_AACDEC_NUM_DLLS (2)

#define AACDEC_BUFHEADER_VERSION 0x1
/* ======================================================================= */
/**
 ** Default timeout used to come out of blocking calls*
 *
 */
/* ======================================================================= */
#define AACD_TIMEOUT (1000) /* millisecs */

/* ======================================================================= */
/**
 * Wince #define
 *
 */
/* ======================================================================= */
#ifdef UNDER_CE
#define sleep Sleep
#endif
/* ======================================================================= */
/**
 * @def    AACDEC_USN_DLL_NAME   USN DLL name
 */
/* ======================================================================= */
#ifdef UNDER_CE
#define AACDEC_USN_DLL_NAME "\\windows\\usn.dll64P"
#else
#define AACDEC_USN_DLL_NAME "usn.dll64P"
#endif

/* ======================================================================= */
/**
 * @def    AACDEC_DLL_NAME   AAC Dec Decoder socket node DLL name
 */
/* ======================================================================= */
#ifdef UNDER_CE
#define AACDEC_DLL_NAME "\\windows\\mpeg4aacdec_sn.dll64P"
#else
#define AACDEC_DLL_NAME "mpeg4aacdec_sn.dll64P"
#endif

#define DONT_CARE 0

/* ======================================================================= */
/**
 * @def    AACDEC_CPU_USAGE for Resource Mannager (MHZ)
 */
/* ======================================================================= */
#define AACDEC_CPU_USAGE 50


/* ======================================================================= */
/**
 * @def    AACDEC_SBR_CONTENT  flag detection
 */
/* ======================================================================= */

#define AACDEC_SBR_CONTENT 0x601


/* ======================================================================= */
/**
 * @def    AACDEC_PS_CONTENT flag  detection
 */
/* ======================================================================= */

#define  AACDEC_PS_CONTENT 0x602


/* ======================================================================= */
/**
 * @def    AACDEC_DEBUG   Debug print macro
 */
/* ======================================================================= */

#undef AACDEC_DEBUG 
#define _ERROR_PROPAGATION__

#ifdef UNDER_CE

/* ======================================================================= */
/**
 * @def    DEBUG   Memory print macro
 */
/* ======================================================================= */
#if DEBUG
#define AACDEC_DPRINT printf
#define AACDEC_MEMPRINT printf
#define AACDEC_STATEPRINT printf
#define AACDEC_BUFPRINT printf
#define AACDEC_MEMPRINT printf
#define AACDEC_EPRINT printf
#else
#define AACDEC_DPRINT
#define AACDEC_MEMPRINT
#define AACDEC_STATEPRINT
#define AACDEC_BUFPRINT
#define AACDEC_MEMPRINT
#define AACDEC_EPRINT
#endif

#else /* for Linux */

#ifdef  AACDEC_DEBUG
    #define AACDEC_DPRINT printf
    #undef AACDEC_BUFPRINT printf
    #undef AACDEC_MEMPRINT printf
    #define AACDEC_STATEPRINT printf
#else
    #define AACDEC_DPRINT(...)
#endif

#ifdef AACDEC_STATEDETAILS
    #define AACDEC_STATEPRINT printf
#else
    #define AACDEC_STATEPRINT(...)
#endif

#ifdef AACDEC_BUFDETAILS
    #define AACDEC_BUFPRINT printf
#else
    #define AACDEC_BUFPRINT(...)
#endif

#ifdef AACDEC_MEMDETAILS
    #define AACDEC_MEMPRINT(...)  fprintf(stdout, "%s %d::  ",__FUNCTION__, __LINE__); \
                                  fprintf(stdout, __VA_ARGS__); \
                                  fprintf(stdout, "\n");
#else
    #define AACDEC_MEMPRINT(...)
#endif

#define AACDEC_EPRINT ALOGE

#endif


/* ======================================================================= */
/**
 * @def    AACDEC_OMX_ERROR_EXIT   Exit print and return macro
 */
/* ======================================================================= */
#define AACDEC_OMX_ERROR_EXIT(_e_, _c_, _s_)                            \
    _e_ = _c_;                                                          \
    OMXDBG_PRINT(stderr, ERROR, 4, 0, "\n**************** OMX ERROR ************************\n");  \
    OMXDBG_PRINT(stderr, ERROR, 4, 0, "%d : Error Name: %s : Error Num = %x",__LINE__, _s_, _e_);  \
    OMXDBG_PRINT(stderr, ERROR, 4, 0, "\n**************** OMX ERROR ************************\n");  \
    goto EXIT;

/* ======================================================================= */
/**
 * @def    AACDEC_OMX_CONF_CHECK_CMD   Command check Macro
 */
/* ======================================================================= */
#define AACDEC_OMX_CONF_CHECK_CMD(_ptr1, _ptr2, _ptr3)  \
    {                                                   \
        if(!_ptr1 || !_ptr2 || !_ptr3){                 \
            eError = OMX_ErrorBadParameter;             \
            goto EXIT;                                  \
        }                                               \
    }

/* ======================================================================= */
/**
 * @def    OMX_CONF_INIT_STRUCT   Macro to Initialise the structure variables
 */
/* ======================================================================= */
#define OMX_CONF_INIT_STRUCT(_s_, _name_)       \
    memset((_s_), 0x0, sizeof(_name_));         \
    (_s_)->nSize = sizeof(_name_);              \
    (_s_)->nVersion.s.nVersionMajor = 1;      \
    (_s_)->nVersion.s.nVersionMinor = 1;      \
    (_s_)->nVersion.s.nRevision = 0x0;          \
    (_s_)->nVersion.s.nStep = 0x0

/* ======================================================================= */
/**
 * @def    AACDEC_BUFDETAILS   Turns buffer messaging on and off
 */
/* ======================================================================= */
#undef AACDEC_BUFDETAILS
/* ======================================================================= */
/**
 * @def    AACDEC_STATEDETAILS   Turns state messaging on and off
 */
/* ======================================================================= */
#undef AACDEC_STATEDETAILS
/* ======================================================================= */
/**
 * @def    AACDEC_MEMDETAILS   Turns memory messaging on and off
 */
/* ======================================================================= */
#undef AACDEC_MEMDETAILS

#define AACDEC_OUTPUT_PORT 1
#define AACDEC_INPUT_PORT 0
#define AACDEC_APP_ID  100
#define MAX_NUM_OF_BUFS_AACDEC 15
#define PARAMETRIC_STEREO_AACDEC 1
#define NON_PARAMETRIC_STEREO_AACDEC 0
/* ======================================================================= */
/**
 * @def    NUM_OF_PORTS_AACDEC   Number of ports
 */
/* ======================================================================= */
#define NUM_OF_PORTS_AACDEC 2
/* ======================================================================= */
/**
 * @def    STREAM_COUNT_AACDEC   Number of streams
 */
/* ======================================================================= */
#define STREAM_COUNT_AACDEC 2

/** Default timeout used to come out of blocking calls*/

/* ======================================================================= */
/**
 * @def    AACD_NUM_INPUT_BUFFERS   Default number of input buffers
 *
 */
/* ======================================================================= */
#define AACD_NUM_INPUT_BUFFERS 4
/* ======================================================================= */
/**
 * @def    AACD_NUM_OUTPUT_BUFFERS   Default number of output buffers
 *
 */
/* ======================================================================= */
#define AACD_NUM_OUTPUT_BUFFERS 4

/* ======================================================================= */
/**
 * @def    AACD_INPUT_BUFFER_SIZE   Default input buffer size
 *
 */
/* ======================================================================= */
#define AACD_INPUT_BUFFER_SIZE 1536*4
/* ======================================================================= */
/**
 * @def    AACD_OUTPUT_BUFFER_SIZE   Default output buffer size
 *
 */
/* ======================================================================= */
#define AACD_OUTPUT_BUFFER_SIZE 8192*2
/* ======================================================================= */
/**
 * @def    AACD_SAMPLING_FREQUENCY   Sampling frequency
 */
/* ======================================================================= */
#define AACD_SAMPLING_FREQUENCY 44100

/* ======================================================================= */
/**
 * @def    AACDec macros for MONO,STEREO_INTERLEAVED,STEREO_NONINTERLEAVED
 */
/* ======================================================================= */
/*#define AACD_STEREO_INTERLEAVED_STREAM     2
  #define AACD_STEREO_NONINTERLEAVED_STREAM  3*/
/* ======================================================================= */
/**
 * @def    AACDec macros for MONO,STEREO_INTERLEAVED,STEREO_NONINTERLEAVED
 */
/* ======================================================================= */
/* Stream types supported*/
#define MONO_STREAM_AACDEC                   1
#define STEREO_INTERLEAVED_STREAM_AACDEC     2
#define STEREO_NONINTERLEAVED_STREAM_AACDEC  3

/* ======================================================================= */
/**
 * pthread variable to indicate OMX returned all buffers to app 
 */
/* ======================================================================= */
pthread_mutex_t bufferReturned_mutex; 
pthread_cond_t bufferReturned_condition;

/**
 *
 * AAC Decoder Profile:0 - MAIN, 1 - LC, 2 - SSR, 3 - LTP.
 */
typedef enum {
    EProfileMain,
    EProfileLC,
    EProfileSSR,
    EProfileLTP
}AACProfile;
/* ======================================================================= */
/** COMP_PORT_TYPE_AACDEC  Port types
 *
 *  @param  INPUT_PORT_AACDEC                    Input port
 *
 *  @param  OUTPUT_PORT_AACDEC               Output port
 */
/*  ==================================================================== */
/*This enum must not be changed. */
typedef enum COMP_PORT_TYPE_AACDEC {
    INPUT_PORT_AACDEC = 0,
    OUTPUT_PORT_AACDEC
}COMP_PORT_TYPE_AACDEC;
/* ======================================================================= */
/** OMX_INDEXAUDIOTYPE_AACDEC  Defines the custom configuration settings
 *                              for the component
 *
 *  @param  OMX_IndexCustomMode16_24bit_AACDEC  Sets the 16/24 mode
 *
 *  @param  OMX_IndexCustomModeProfile_AACDEC  Sets the Profile mode
 *
 *  @param  OMX_IndexCustomModeSBR_AACDEC  Sets the SBR mode
 *
 *  @param  OMX_IndexCustomModeDasfConfig_AACDEC  Sets the DASF mode
 *
 *  @param  OMX_IndexCustomModeRAW_AACDEC  Sets the RAW mode
 *
 *  @param  OMX_IndexCustomModePS_AACDEC  Sets the ParametricStereo mode
 *
 */
/*  ==================================================================== */
typedef enum OMX_INDEXAUDIOTYPE_AACDEC {
    OMX_IndexCustomAacDecHeaderInfoConfig = 0xFF000001,
    OMX_IndexCustomAacDecStreamIDConfig,
    OMX_IndexCustomAacDecDataPath,
    OMX_IndexCustomDebug
}OMX_INDEXAUDIOTYPE_AACDEC;

/* ======================================================================= */
/** IAUDIO_PcmFormat: This value is used by DSP.
 *
 * @param IAUDIO_BLOCK: It is used in DASF mode.
 *
 * @param IAUDIO_INTERLEAVED: It specifies interleaved format of SN.
 */
/* ==================================================================== */
typedef enum {
    EAUDIO_BLOCK =0,
    EAUDIO_INTERLEAVED
}TAUDIO_AacFormat;

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
/** MPEG4AACDEC_UALGParams
 *
 * @param lOutputFormat - To set interleaved/Block format:Refer to IAUDIO_AacFormat.
 * @param DownSampleSbr -
 */
/* ==================================================================== */
typedef struct {
    OMX_U32    size;
    long       lOutputFormat;
    long       DownSampleSbr;
    long       iEnablePS;
    long       lSamplingRateIdx;
    long       bRawFormat;
    long       dualMonoMode;
} MPEG4AACDEC_UALGParams;

/* ======================================================================= */
/** IUALG_Cmd_AAC_DEC: This enum type describes the standard set of commands that
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
    IUALG_CMD_USERCMDSTART_AACDEC  = 100
}IUALG_Cmd_AAC_DEC;

typedef enum{
  IAAC_WARN_DATA_CORRUPT = 0x0804
}IAAC_WARN_MSG;

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
/** IUALG_PCMDCmd: This enum specifies the command to DSP.
 *
 * @param IULAG_CMD_SETSTREAMTYPE: Specifies the stream type to be sent to DSP.
 */
/* ==================================================================== */
typedef enum {
    IULAG_CMD_SETSTREAMTYPE = IUALG_CMD_USERCMDSTART_AACDEC
}IUALG_PCMDCmd;

/* ======================================================================= */
/** AACDEC_UAlgInBufParamStruct: This struct is passed with input buffers that
 * are sent to DSP.
 */
/* ==================================================================== */
typedef struct {
    /* Set to 1 if buffer is last buffer */
    unsigned short bLastBuffer;
    unsigned short bConcealBuffer;
}AACDEC_UAlgInBufParamStruct;

/* ======================================================================= */
/** USN_AudioCodecParams: This contains the information which does to Codec
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
/** AACDEC_UAlgOutBufParamStruct: This is passed with output buffer to DSP.
 */
/* ==================================================================== */
typedef struct {
    unsigned long ulFrameCount;
    unsigned long isLastBuffer;
}AACDEC_UAlgOutBufParamStruct;

typedef struct AACDEC_UALGParams{
    unsigned long    lOutputFormat;
    unsigned long    lMonoToStereoCopy;
} AACDEC_UALGParams;

/* ======================================================================= */
/** AACD_LCML_BUFHEADERTYPE: This is LCML buffer header which is sent to LCML
 * for both input and output buffers.
 */
/* ==================================================================== */
typedef struct AACD_LCML_BUFHEADERTYPE {
    /* Direction whether input or output buffer */
    OMX_DIRTYPE eDir;
    /* Pointer to OMX Buffer Header */
    OMX_BUFFERHEADERTYPE *pBufHdr;
    /* Other parameters, may be useful for enhancements */
    void *pOtherParams[10];
    /* Input Parameter Information structure */
    AACDEC_UAlgInBufParamStruct *pIpParam;
    /* Output Parameter Information structure */
    AACDEC_UAlgOutBufParamStruct *pOpParam;
}AACD_LCML_BUFHEADERTYPE;

/* Component Port Context */
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
} AUDIODEC_PORT_TYPE;


/* ======================================================================= */
/** AAC_DEC_BUFFERLIST: This contains information about a buffer's owner whether
 * it is application or component, number of buffers owned etc.
 *
 * @see OMX_BUFFERHEADERTYPE
 */
/* ==================================================================== */
struct AAC_DEC_BUFFERLIST{
    /* Array of pointer to OMX buffer headers */
    OMX_BUFFERHEADERTYPE *pBufHdr[MAX_NUM_OF_BUFS_AACDEC];
    /* Array that tells about owner of each buffer */
    OMX_U32 bufferOwner[MAX_NUM_OF_BUFS_AACDEC];
    /* Tracks pending buffers */
    OMX_U32 bBufferPending[MAX_NUM_OF_BUFS_AACDEC];
    /* Number of buffers  */
    OMX_U32 numBuffers;
};

typedef struct AAC_DEC_BUFFERLIST AACDEC_BUFFERLIST;

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
/** AACDEC_COMPONENT_PRIVATE: This is the major and main structure of the
 * component which contains all type of information of buffers, ports etc
 * contained in the component.
 *
 * @see OMX_BUFFERHEADERTYPE
 * @see OMX_AUDIO_PARAM_PORTFORMATTYPE
 * @see OMX_PARAM_PORTDEFINITIONTYPE
 * @see AACD_LCML_BUFHEADERTYPE
 * @see OMX_PORT_PARAM_TYPE
 * @see OMX_PRIORITYMGMTTYPE
 * @see AUDIODEC_PORT_TYPE
 * @see AACDEC_BUFFERLIST
 * @see LCML_STRMATTR
 * @see
 */
/* ==================================================================== */

typedef struct AACDEC_COMPONENT_PRIVATE
{

    OMX_CALLBACKTYPE cbInfo;
    /** Handle for use with async callbacks */
    OMX_PORT_PARAM_TYPE* sPortParam;
    /* Input port information */
    OMX_AUDIO_PARAM_PORTFORMATTYPE sInPortFormat;
    /* Output port information */
    OMX_AUDIO_PARAM_PORTFORMATTYPE sOutPortFormat;
    /* Buffer owner information */
    OMX_U32 bIsBufferOwned[NUM_OF_PORTS_AACDEC];

    /** This will contain info like how many buffers
        are there for input/output ports, their size etc, but not
        BUFFERHEADERTYPE POINTERS. */
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef[NUM_OF_PORTS_AACDEC];
    /* Contains information that come from application */
    OMX_AUDIO_PARAM_AACPROFILETYPE* aacParams;

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

    OMX_U32 lcml_compID;
    /** Counts of number of output buffers reclaimed from lcml  */
    OMX_U32 num_Reclaimed_Op_Buff;
    /** Counts of number of input buffers sent to lcml  */
    OMX_U32 num_Sent_Ip_Buff;
    /** Counts of number of output buffers sent to lcml  */
    OMX_U32 num_Op_Issued;
    /** Holds the value of dasf mode, 1: DASF mode or 0: File Mode  */
    OMX_U32 dasfmode;

    /** This is LCML handle  */
    OMX_HANDLETYPE pLcmlHandle;

    /** ID stream ID**/
    OMX_U32 streamID;
    /** Contains pointers to LCML Buffer Headers */
    AACD_LCML_BUFHEADERTYPE *pLcmlBufHeader[2];

#ifdef __PERF_INSTRUMENTATION__
    PERF_OBJHANDLE pPERF, pPERFcomp;
    OMX_U32 nLcml_nCntIp;         
    OMX_U32 nLcml_nCntOpReceived;
#endif

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
    OMX_U32 ipPortEnableFlag;
    /** Input port disble flag */
    OMX_U32 ipPortDisableFlag;
    /** Pointer to port parameter structure */
    OMX_PORT_PARAM_TYPE* pPortParamType;
    /** Pointer to port priority management structure */
    OMX_PRIORITYMGMTTYPE* pPriorityMgmt;

#ifdef RESOURCE_MANAGER_ENABLED
    RMPROXY_CALLBACKTYPE rmproxyCallback;
#endif

    OMX_BOOL bPreempted;

	
    /** Contains the port related info of both the ports */
    AUDIODEC_PORT_TYPE *pCompPort[NUM_OF_PORTS_AACDEC];
    /* Checks whether or not buffer were allocated by appliction */
    OMX_U32 bufAlloced;
    /** Flag to check about execution of component thread */
    OMX_U16 bExitCompThrd;
    /** Pointer to list of input buffers */
    AACDEC_BUFFERLIST *pInputBufferList;
    /** Pointer to list of output buffers */
    AACDEC_BUFFERLIST *pOutputBufferList;
    /** it is used for component's create phase arguments */
    LCML_STRMATTR  *strmAttr;
    /** Contains the version information */
    OMX_U32 nVersion;

    /** Number of input buffers at runtime */
    OMX_U32 nRuntimeInputBuffers;

    /** Number of output buffers at runtime */
    OMX_U32 nRuntimeOutputBuffers;

    /** Parameters being passed to the SN */
    USN_AudioCodecParams *pParams;

    /** Dynamic Parameters being passed to the SN */
    MPEG4AACDEC_UALGParams * AACDEC_UALGParam;

    OMX_U16 framemode;

    OMX_STRING cComponentName;

    OMX_VERSIONTYPE ComponentVersion;

    OMX_U32 nOpBit;
    OMX_U32 parameteric_stereo;
    OMX_U32 dualMonoMode;
    OMX_U32 SBR;
    OMX_U32 RAW;
    OMX_U32 nFillThisBufferCount;
    OMX_U32 nFillBufferDoneCount;
    OMX_U32 nEmptyThisBufferCount;
    OMX_U32 nEmptyBufferDoneCount;
    OMX_U32 bInitParamsInitialized;
    AACDEC_BUFFERLIST *pInputBufferListQueue;
    AACDEC_BUFFERLIST *pOutputBufferListQueue;
    /** To store input buffers recieved while in paused state **/
    OMX_BUFFERHEADERTYPE *pInputBufHdrPending[MAX_NUM_OF_BUFS_AACDEC];
    OMX_U32 nNumInputBufPending;
    
    /** To store out buffers received while in puased state **/
    OMX_BUFFERHEADERTYPE *pOutputBufHdrPending[MAX_NUM_OF_BUFS_AACDEC];
    OMX_U32 nNumOutputBufPending;
    
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
    /* bIdleCommandPending;*/
    OMX_S32 nOutStandingFillDones;
    OMX_BOOL bIsInvalidState;
    OMX_STRING* sDeviceString;
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

    OMX_U32 nUnhandledFillThisBuffers;
    OMX_U32 nHandledFillThisBuffers;
    OMX_U32 nUnhandledEmptyThisBuffers;
    OMX_U32 nHandledEmptyThisBuffers;
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
    OMX_PARAM_COMPONENTROLETYPE *componentRole;

    OMX_U8 PendingInPausedBufs;
    OMX_BUFFERHEADERTYPE *pInBufHdrPausedPending[MAX_NUM_OF_BUFS_AACDEC];
    OMX_U8 PendingOutPausedBufs;
    OMX_BUFFERHEADERTYPE *pOutBufHdrPausedPending[MAX_NUM_OF_BUFS_AACDEC];
    
    /** Keep buffer timestamps **/
    OMX_S64 arrBufIndex[MAX_NUM_OF_BUFS_AACDEC];
	/**Keep buffer tickcount*/
	OMX_U32 arrBufIndexTick[MAX_NUM_OF_BUFS_AACDEC];
	
    /** Index to arrBufIndex[] and arrBufIndexTick[], used for input buffer timestamps */
    OMX_U8 IpBufindex;
    /** Index to arrBufIndex[] and arrBufIndexTick[], used for input buffer timestamps  */
    OMX_U8 OpBufindex;

    /** Flag to flush SN after EOS in order to process more buffers after EOS**/
    OMX_U8 SendAfterEOS;

    /** Flag to mark the first sent buffer**/
    OMX_U8 first_buff;
    /** First Time Stamp sent **/
    OMX_TICKS first_TS;
    /** Temporal time stamp **/
    OMX_TICKS temp_TS;

    PV_OMXComponentCapabilityFlagsType iPVCapabilityFlags;
    OMX_BOOL bConfigData;
    OMX_BOOL reconfigInputPort;
    OMX_BOOL reconfigOutputPort;
    OMX_U8 OutPendingPR;

    struct OMX_TI_Debug dbg;

    /** Indicate when first output buffer received from DSP **/
    OMX_U32 first_output_buf_rcv;

} AACDEC_COMPONENT_PRIVATE;

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
 *  @see          AacDec_StartCompThread()
 */
/* ================================================================================ * */
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
 *  @param hComp            OMX Handle
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 *
 */
/*================================================================== */
OMX_EXPORT OMX_ERRORTYPE OMX_ComponentInit (OMX_HANDLETYPE hComp);
#endif

/* ================================================================================= * */
/**
 * AacDec_StartCompThread() starts the component thread. This is internal
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
OMX_ERRORTYPE AacDec_StartCompThread(OMX_HANDLETYPE pHandle);
/* ================================================================================= * */
/**
 * AACDEC_Fill_LCMLInitParams() fills the LCML initialization structure.
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
OMX_ERRORTYPE AACDEC_Fill_LCMLInitParams(OMX_HANDLETYPE pHandle, LCML_DSP *plcml_Init, OMX_U16 arr[]);
/* ================================================================================= * */
/**
 * AACDEC_GetBufferDirection() function determines whether it is input buffer or
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
OMX_ERRORTYPE AACDEC_GetBufferDirection(OMX_BUFFERHEADERTYPE *pBufHeader,
                                        OMX_DIRTYPE *eDir);
/* ================================================================================= * */
/**
 * AACDEC_LCML_Callback() function is callback which is called by LCML whenever
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
OMX_ERRORTYPE AACDEC_LCML_Callback (TUsnCodecEvent event,void * args [10]);
/* ================================================================================= * */
/**
 * AACDEC_HandleCommand() function handles the command sent by the application.
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
OMX_U32 AACDEC_HandleCommand (AACDEC_COMPONENT_PRIVATE *pComponentPrivate);
/* ================================================================================= * */
/**
 * AACDEC_HandleDataBuf_FromApp() function handles the input and output buffers
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
OMX_ERRORTYPE AACDEC_HandleDataBuf_FromApp(OMX_BUFFERHEADERTYPE *pBufHeader,
                                           AACDEC_COMPONENT_PRIVATE *pComponentPrivate);
/* ================================================================================= * */
/**
 * AACDEC_GetLCMLHandle() function gets the LCML handle and interacts with LCML
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
OMX_HANDLETYPE AACDEC_GetLCMLHandle(AACDEC_COMPONENT_PRIVATE* pComponentPrivate);
/* ================================================================================= * */
/**
 * AACDEC_GetCorresponding_LCMLHeader() function gets the corresponding LCML
 * header from the actual data buffer for required processing.
 *
 * @param *pBuffer This is the data buffer pointer.
 *
 * @param eDir   This is direction of buffer. Input/Output.
 *
 * @param *AACD_LCML_BUFHEADERTYPE  This is pointer to LCML Buffer Header.
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
OMX_ERRORTYPE AACDEC_GetCorresponding_LCMLHeader(AACDEC_COMPONENT_PRIVATE* pComponentPrivate,
                                                 OMX_U8 *pBuffer,
                                                 OMX_DIRTYPE eDir,
                                                 AACD_LCML_BUFHEADERTYPE **ppLcmlHdr);
/* ================================================================================= * */
/**
 * AACDEC_FreeCompResources() function frees the component resources.
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
OMX_ERRORTYPE AACDEC_FreeCompResources(OMX_HANDLETYPE pComponent);
/* ================================================================================= * */
/**
 * AACDEC_CleanupInitParams() function frees only the initialization time
 * memories allocated. For example, it will not close pipes, it will not free the
 * memory allocated to the buffers etc. But it does free the memory of buffers
 * utilized by the LCML etc. It is basically subset of AACDEC_FreeCompResources()
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
void AACDEC_CleanupInitParams(OMX_HANDLETYPE pComponent);
/* ================================================================================= * */
/**
 * AACDEC_CleanupInitParamsEx() function frees only the initialization time
 * memories allocated. For example, it will not close pipes, it will not free the
 * memory allocated to the buffers etc. But it does free the memory of buffers
 * utilized by the LCML etc. It is basically subset of AACDEC_FreeCompResources()
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
void AACDEC_CleanupInitParamsEx(OMX_HANDLETYPE pComponent,OMX_U32 indexport);
/* ===========================================================  */
/**
 *  AACDEC_SetPending()  Called when the component queues a buffer
 * to the LCML
 *
 *  @param pComponentPrivate        Component private data
 *
 *  @param pBufHdr                Buffer header
 *
 *  @param eDir                    Direction of the buffer
 *
 *  @return None
 */
/*================================================================== */
void AACDEC_SetPending(AACDEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir, OMX_U32 lineNumber);
/* ===========================================================  */
/**
 *  AACDEC_ClearPending()  Called when a buffer is returned
 * from the LCML
 *
 *  @param pComponentPrivate        Component private data
 *
 *  @param pBufHdr                Buffer header
 *
 *  @param eDir                    Direction of the buffer
 *
 *  @return None
 */
/*================================================================== */
void AACDEC_ClearPending(AACDEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir, OMX_U32 lineNumber) ;
/* ===========================================================  */
/**
 *  AACDEC_IsPending()
 *
 *
 *  @param pComponentPrivate        Component private data
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 */
/*================================================================== */
OMX_U32 AACDEC_IsPending(AACDEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir);
/* ===========================================================  */
/**
 *  AACDECFill_LCMLInitParamsEx()  Fills the parameters needed
 * to initialize the LCML without recreating the socket node
 *
 *  @param pComponent            OMX Handle
 *
 *  @return None
 */

/*================================================================== */
OMX_ERRORTYPE AACDECFill_LCMLInitParamsEx(OMX_HANDLETYPE pComponent,OMX_U32 indexport);
/* ===========================================================  */
/**
 *  AACDEC_IsValid()
 *
 *
 *  @param pComponentPrivate        Component private data
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 */
/*================================================================== */
OMX_U32 AACDEC_IsValid(AACDEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U8 *pBuffer, OMX_DIRTYPE eDir) ;

#ifdef RESOURCE_MANAGER_ENABLED
void AACDEC_ResourceManagerCallback(RMPROXY_COMMANDDATATYPE cbData);
#endif
/*=======================================================================*/
/*! @fn AACDec_GetSampleRateIndexL

 * @brief Gets the sample rate index

 * @param  aRate : Actual Sampling Freq

 * @Return  Index

 */
/*=======================================================================*/
int AACDec_GetSampleRateIndexL( const int aRate);
int AACDec_GetSampleRatebyIndex( const int index);
void* AACDEC_ComponentThread (void* pThreadData);

OMX_U32 AACDEC_ParseHeader(OMX_BUFFERHEADERTYPE* pBufHeader,
                           AACDEC_COMPONENT_PRIVATE *pComponentPrivate);

/*  =========================================================================*/
/*  func    GetBits                                                          */
/*                                                                           */
/*  desc    Gets aBits number of bits from position aPosition of one buffer  */
/*            and returns the value in a TUint value.                        */
/*  =========================================================================*/
OMX_U32 AACDEC_GetBits(OMX_U32* nPosition, OMX_U8 nBits, OMX_U8* pBuffer, OMX_BOOL bIcreasePosition);

/*  =========================================================================*/
/*  func    AACDEC_HandleUSNError
 *
 *  desc    Handles error messages returned by the dsp
 *
 * @Return n/a
 *
 *  =========================================================================*/
void AACDEC_HandleUSNError (AACDEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U32 arg);

/*=======================================================================*/
/*! @fn SignalIfAllBuffersAreReturned 
 * @brief Sends pthread signal to indicate OMX has returned all buffers to app 
 * @param  none 
 * @Return void 
 */
/*=======================================================================*/
void SignalIfAllBuffersAreReturned(AACDEC_COMPONENT_PRIVATE *pComponentPrivate);

#endif
