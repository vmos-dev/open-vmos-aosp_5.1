
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
* @file OMX_G726Enc_Utils.h
*
* This is an header file for an G726 Encoder that is fully
* compliant with the OMX Audio specification 1.5.
* This the file that the application that uses OMX would include in its code.
*
* @path $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\g726_enc\inc
*
* @rev 1.0
*/
/* --------------------------------------------------------------------------- */
/* ----------------------------------------------------------------------------
*!
*! Revision History
*! ===================================
*! Gyancarlo Garcia: Initial Verision
*! 05-Oct-2007
*!
* =========================================================================== */
#ifndef OMX_G726ENC_UTILS__H
#define OMX_G726ENC_UTILS__H

#include <pthread.h>
#include "LCML_DspCodec.h"
#include <OMX_Component.h>
#include <TIDspOmx.h>

#ifdef RESOURCE_MANAGER_ENABLED
#include <ResourceManagerProxyAPI.h>
#endif

#ifdef UNDER_CE
	#define sleep Sleep
#endif


/* ======================================================================= */
/**
 * @def    G726ENC_DEBUG   Turns debug messaging on and off
 */
/* ======================================================================= */
#undef G726ENC_DEBUG
/*#define G726ENC_DEBUG*/
/* ======================================================================= */
/**
 * @def    G726ENC_MEMCHECK   Turns memory messaging on and off
 */
/* ======================================================================= */
#undef G726ENC_MEMCHECK

#ifndef UNDER_CE
/* ======================================================================= */
/**
 * @def    G726ENC_DEBUG   Debug print macro
 */
/* ======================================================================= */
#ifdef  G726ENC_DEBUG
        #define G726ENC_DPRINT(...)    fprintf(stderr,__VA_ARGS__)
#else
        #define G726ENC_DPRINT(...)
#endif
/* ======================================================================= */
/**
 * @def    G726ENC_MEMCHECK   Memory print macro
 */
/* ======================================================================= */
#ifdef  G726ENC_MEMCHECK
        #define G726ENC_MEMPRINT(...)    fprintf(stderr,__VA_ARGS__)
#else
        #define G726ENC_MEMPRINT(...)
#endif

#else   /*UNDER_CE*/
/* ======================================================================= */
/**
 * @def    G726ENC_DEBUG   Debug print macro
 */
/* ======================================================================= */
#ifdef  G726ENC_DEBUG
    #define G726ENC_DPRINT(STR, ARG...) printf()
#else

#endif

/* ======================================================================= */
/**
 * @def    G726ENC_MEMCHECK   Memory print macro
 */
/* ======================================================================= */
#ifdef  G726ENC_MEMCHECK
		#define G726ENC_MEMPRINT(STR, ARG...) printf()
#else

#endif

#endif

#ifdef DEBUG
		#define G726ENC_DPRINT(...)    fprintf(stderr,__VA_ARGS__)
		#define G726ENC_MEMPRINT(...)    fprintf(stderr,__VA_ARGS__)
		#define G726ENC_EPRINT(...)    fprintf(stderr,__VA_ARGS__)
#else
		#define G726ENC_DPRINT(...)
		#define G726ENC_MEMPRINT(...)
		#define G726ENC_EPRINT(...)
#endif

/* ======================================================================= */
/**
 * @def    G726ENC_DEBUGMEM   Turns memory leaks messaging on and off.
 *         APP_DEBUGMEM must be defined in Test App in order to get
 *         this functionality On.
 */
/* ======================================================================= */
#undef G726ENC_DEBUGMEM
/*#define G726ENC_DEBUGMEM*/

/* ======================================================================= */
/**
 *  M A C R O S FOR MALLOC and MEMORY FREE and CLOSING PIPES
 */
/* ======================================================================= */

#define OMX_NBCONF_INIT_STRUCT(_s_, _name_)	\
    memset((_s_), 0x0, sizeof(_name_));	\
    (_s_)->nSize = sizeof(_name_);		\
    (_s_)->nVersion.s.nVersionMajor = 0x1;	\
    (_s_)->nVersion.s.nVersionMinor = 0x0;	\
    (_s_)->nVersion.s.nRevision = 0x0;		\
    (_s_)->nVersion.s.nStep = 0x0

#define OMX_NBMEMFREE_STRUCT(_pStruct_)\
	G726ENC_MEMPRINT("%d :: [FREE] %p\n",__LINE__,_pStruct_);\
    if(_pStruct_ != NULL){\
        SafeFree(_pStruct_);\
	    _pStruct_ = NULL;\
	}

#define OMX_NBCLOSE_PIPE(_pStruct_,err)\
	G726ENC_DPRINT("%d :: CLOSING PIPE \n",__LINE__);\
	err = close (_pStruct_);\
    if(0 != err && OMX_ErrorNone == eError){\
		eError = OMX_ErrorHardware;\
		printf("%d :: Error while closing pipe\n",__LINE__);\
		goto EXIT;\
	}

#define OMX_NBMALLOC_STRUCT(_pStruct_, _sName_)   \
    _pStruct_ = (_sName_*)SafeMalloc(sizeof(_sName_));      \
    if(_pStruct_ == NULL){      \
        printf("***********************************\n"); \
        printf("%d :: Malloc Failed\n",__LINE__); \
        printf("***********************************\n"); \
        eError = OMX_ErrorInsufficientResources; \
        goto EXIT;      \
    } \
    G726ENC_MEMPRINT("%d :: [ALLOC] %p\n",__LINE__,_pStruct_);


/* ======================================================================= */
/**
 * @def G726ENC_NUM_INPUT_BUFFERS   Default number of input buffers
 */
/* ======================================================================= */
#define G726ENC_NUM_INPUT_BUFFERS 1
/* ======================================================================= */
/**
 * @def G726ENC_NUM_INPUT_BUFFERS_DASF  Default No.of input buffers DASF
 */
/* ======================================================================= */
#define G726ENC_NUM_INPUT_BUFFERS_DASF 2
/* ======================================================================= */
/**
 * @def G726ENC_NUM_OUTPUT_BUFFERS   Default number of output buffers
 */
/* ======================================================================= */
#define G726ENC_NUM_OUTPUT_BUFFERS 1
/* ======================================================================= */
/**
 * @def G726ENC_INPUT_BUFFER_SIZE_DASF  Default input buffer size DASF
 */
/* ======================================================================= */
#define G726ENC_INPUT_BUFFER_SIZE_DASF 80
/*16*/
/* ======================================================================= */
/**
 * @def G726ENC_INPUT_BUFFER_SIZE  Default input buffer size DASF
 */
/* ======================================================================= */
#define G726ENC_INPUT_BUFFER_SIZE 16
/* ======================================================================= */
/**
 * @def G726ENC_OUTPUT_BUFFER_SIZE   Default output buffer size
 */
/* ======================================================================= */
#define G726ENC_OUTPUT_BUFFER_SIZE 60

/* ======================================================================= */
/**
 * @def	G726ENC_APP_ID  App ID Value setting
 */
/* ======================================================================= */
#define G726ENC_APP_ID 100

/* ======================================================================= */
/**
 * @def    G726ENC_SAMPLING_FREQUENCY   Sampling frequency
 */
/* ======================================================================= */
#define G726ENC_SAMPLING_FREQUENCY 8000
/* ======================================================================= */
/**
 * @def    G726ENC_MAX_NUM_OF_BUFS   Maximum number of buffers
 */
/* ======================================================================= */
#define G726ENC_MAX_NUM_OF_BUFS 10
/* ======================================================================= */
/**
 * @def    G726ENC_NUM_OF_PORTS   Number of ports
 */
/* ======================================================================= */
#define G726ENC_NUM_OF_PORTS 2
/* ======================================================================= */
/**
 * @def    G726ENC_XXX_VER    Component version
 */
/* ======================================================================= */
#define G726ENC_MAJOR_VER 0xF1
#define G726ENC_MINOR_VER 0xF2
/* ======================================================================= */
/**
 * @def    G726ENC_NOT_USED    Defines a value for "don't care" parameters
 */
/* ======================================================================= */
#define G726ENC_NOT_USED 10
/* ======================================================================= */
/**
 * @def    G726ENC_NORMAL_BUFFER  Defines flag value with all flags off
 */
/* ======================================================================= */
#define G726ENC_NORMAL_BUFFER 0
/* ======================================================================= */
/**
 * @def    OMX_G726ENC_DEFAULT_SEGMENT    Default segment ID for the LCML
 */
/* ======================================================================= */
#define G726ENC_DEFAULT_SEGMENT (0)
/* ======================================================================= */
/**
 * @def    OMX_G726ENC_SN_TIMEOUT    Timeout value for the socket node
 */
/* ======================================================================= */
#define G726ENC_SN_TIMEOUT (-1)
/* ======================================================================= */
/**
 * @def    OMX_G726ENC_SN_PRIORITY   Priority for the socket node
 */
/* ======================================================================= */
#define G726ENC_SN_PRIORITY (10)
/* ======================================================================= */
/**
 * @def    G726ENC_CPU   TBD, 50MHz for the moment
 */
/* ======================================================================= */
#define G726ENC_CPU (50)
/* ======================================================================= */
/**
 * @def    OMX_G726ENC_NUM_DLLS   number of DLL's
 */
/* ======================================================================= */
#define G726ENC_NUM_DLLS (2)
/* ======================================================================= */
/**
 * @def    G726ENC_USN_DLL_NAME   USN DLL name
 */
/* ======================================================================= */
#ifdef UNDER_CE
	#define G726ENC_USN_DLL_NAME "\\windows\\usn.dll64P"
#else
	#define G726ENC_USN_DLL_NAME "usn.dll64P"
#endif

/* ======================================================================= */
/**
 * @def    G726ENC_DLL_NAME   G726 Encoder socket node dll name
 */
/* ======================================================================= */
#ifdef UNDER_CE
	#define G726ENC_DLL_NAME "\\windows\\g726enc_sn.dll64P"
#else
	#define G726ENC_DLL_NAME "g726enc_sn.dll64P"
#endif

/* ======================================================================= */
/** G726ENC_StreamType  Stream types
*
*  @param  G726ENC_DMM					DMM
*
*  @param  G726ENC_INSTRM				Input stream
*
*  @param  G726ENC_OUTSTRM				Output stream
*/
/* ======================================================================= */
enum G726ENC_StreamType {
    G726ENC_DMM = 0,
    G726ENC_INSTRM,
    G726ENC_OUTSTRM
};

/* ======================================================================= */
/**
 * @def G726ENC_TIMEOUT Default timeout used to come out of blocking calls
 */
/* ======================================================================= */
#define G726ENC_TIMEOUT 1000
/* ======================================================================= */
/*
 * @def	G726ENC_OMX_MAX_TIMEOUTS   Max Time Outs
 * @def	G726ENC_DONT_CARE 			Dont Care Condition
 * @def	G726ENC_NUM_CHANNELS 		Number of Channels
 * @def	G726ENC_APP_ID 			App ID Value setting
 */
/* ======================================================================= */
#define G726ENC_OMX_MAX_TIMEOUTS 20
#define G726ENC_DONT_CARE 0
#define G726ENC_NUM_CHANNELS 1
/* ======================================================================= */
/**
 * @def    G726ENC_STREAM_COUNT    Number of streams
 * 		   G726ENC_INPUT_STREAM_ID Stream ID for Input Buffer
 */
/* ======================================================================= */
#define G726ENC_STREAM_COUNT 2
#define G726ENC_INPUT_STREAM_ID 0

/* ======================================================================= */
/** G726ENC_COMP_PORT_TYPE  Port types
 *
 *  @param  G726ENC_INPUT_PORT				Input port
 *
 *  @param  G726ENC_OUTPUT_PORT			Output port
 */
/*  ====================================================================== */
/*This enum must not be changed. */
typedef enum G726ENC_COMP_PORT_TYPE {
    G726ENC_INPUT_PORT = 0,
    G726ENC_OUTPUT_PORT
}G726ENC_COMP_PORT_TYPE;

/* ======================================================================= */
/** G726ENC_BUFFER_Dir  Buffer Direction
*
*  @param  G726ENC_DIRECTION_INPUT		Input direction
*
*  @param  G726ENC_DIRECTION_OUTPUT	Output direction
*
*/
/* ======================================================================= */
typedef enum {
    G726ENC_DIRECTION_INPUT,
    G726ENC_DIRECTION_OUTPUT
}G726ENC_BUFFER_Dir;



/* ======================================================================= */
/** G726ENC_BUFFS  Buffer details
*
*  @param  BufHeader Buffer header
*
*  @param  Buffer	Buffer
*
*/
/* ======================================================================= */
typedef struct G726ENC_BUFFS {
    char BufHeader;
    char Buffer;
}G726ENC_BUFFS;

/* ======================================================================= */
/** G726ENC_BUFFERHEADERTYPE_INFO
*
*  @param  pBufHeader
*
*  @param  bBufOwner
*
*/
/* ======================================================================= */
typedef struct G726ENC_BUFFERHEADERTYPE_INFO {
    OMX_BUFFERHEADERTYPE* pBufHeader[G726ENC_MAX_NUM_OF_BUFS];
    G726ENC_BUFFS bBufOwner[G726ENC_MAX_NUM_OF_BUFS];
}G726ENC_BUFFERHEADERTYPE_INFO;


typedef OMX_ERRORTYPE (*G726ENC_fpo)(OMX_HANDLETYPE);

/* =================================================================================== */
/**
* Socket node Audio Codec Configuration parameters.
*/
/* =================================================================================== */
typedef struct G726ENC_AudioCodecParams {
	unsigned long  iSamplingRate;
	unsigned long  iStrmId;
	unsigned short iAudioFormat;
}G726ENC_AudioCodecParams; /*Especified as USN_AudioCodecParams on SN Guide*/

/* =================================================================================== */
/**
* G726ENC_ParamStruct		Input Buffer Param Structure
* bLastBuffer 						To Send Last Buufer Flag
*/
/* =================================================================================== */
typedef struct G726ENC_ParamStruct {
    /*unsigned long bLastBuffer;	*/
    unsigned short bLastBuffer;
} G726ENC_ParamStruct;

/* =================================================================================== */
/**
* G726ENC_LCML_BUFHEADERTYPE Buffer Header Type
*/
/* =================================================================================== */
typedef struct G726ENC_LCML_BUFHEADERTYPE {
      G726ENC_BUFFER_Dir eDir;
      G726ENC_ParamStruct *pIpParam;
/*      G726ENC_ParamStruct *pOpParam;*/ /*According SN guide, this should not be necessary*/
      OMX_BUFFERHEADERTYPE* buffer;
}G726ENC_LCML_BUFHEADERTYPE;

typedef struct _G726ENC_BUFFERLIST G726ENC_BUFFERLIST;

/* =================================================================================== */
/**
* _G726ENC_BUFFERLIST Structure for buffer list
*/
/* ================================================================================== */
struct _G726ENC_BUFFERLIST{
	OMX_BUFFERHEADERTYPE sBufHdr;
    OMX_BUFFERHEADERTYPE *pBufHdr[G726ENC_MAX_NUM_OF_BUFS];
	OMX_U32 bufferOwner[G726ENC_MAX_NUM_OF_BUFS];
	OMX_U32 bBufferPending[G726ENC_MAX_NUM_OF_BUFS];
	OMX_U32 numBuffers;
    G726ENC_BUFFERLIST *pNextBuf;
    G726ENC_BUFFERLIST *pPrevBuf;
};

/* =================================================================================== */
/**
* G726ENC_PORT_TYPE Structure for PortFormat details
*/
/* =================================================================================== */
typedef struct G726ENC_PORT_TYPE {
    OMX_HANDLETYPE hTunnelComponent;
    OMX_U32 nTunnelPort;
    OMX_BUFFERSUPPLIERTYPE eSupplierSetting;
	OMX_U8 nBufferCnt;
	OMX_AUDIO_PARAM_PORTFORMATTYPE* pPortFormat;
} G726ENC_PORT_TYPE;

/* =================================================================================== */
/**
* G726ENC_COMPONENT_PRIVATE Component private data Structure
*/
/* =================================================================================== */
typedef struct G726ENC_COMPONENT_PRIVATE
{
    /** callback Info */
    OMX_CALLBACKTYPE cbInfo;

    /** port parameters Info */
    OMX_PORT_PARAM_TYPE* sPortParam; /*Needed??*/

    /** priority management */
    OMX_PRIORITYMGMTTYPE* sPriorityMgmt;

    /** port definition structure */
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef[G726ENC_NUM_OF_PORTS];

    /** port parameter structure */
    OMX_AUDIO_PARAM_G726TYPE* G726Params[G726ENC_NUM_OF_PORTS];

    /** Buffer info */
    G726ENC_BUFFERHEADERTYPE_INFO BufInfo[G726ENC_NUM_OF_PORTS];

    /** Component ports */
    G726ENC_PORT_TYPE *pCompPort[G726ENC_NUM_OF_PORTS];

    /** LCML headers */
    G726ENC_LCML_BUFHEADERTYPE *pLcmlBufHeader[G726ENC_NUM_OF_PORTS];

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

    /** dasf mode flag */
    OMX_U32 dasfMode;

    /** acdn mode flag */
    OMX_U32 acdnMode;

    /** rtp mode flag */
    OMX_U32 rtpMode;

    /** Set to indicate component is stopping */
    OMX_U32 bIsStopping;

    /** stream ID */
    OMX_U32 streamID;

    /** port defaults allocated */
    OMX_U32 bPortDefsAllocated;

    /** thread started flag */
    OMX_U32 bCompThreadStarted;

    /** version number */
    OMX_U32 nVersion;

    /** FillThisBufferCount */
    OMX_U32 nFillThisBufferCount;

    /** FillBufferDoneCount */
    OMX_U32 nFillBufferDoneCount;

    /** EmptyThisBufferCount */
    OMX_U32 nEmptyThisBufferCount;

    /** EmptyBufferDoneCount */
    OMX_U32 nEmptyBufferDoneCount;

    /** InitParamsInitialized */
    OMX_U32 bInitParamsInitialized;

    /** NumInputBufPending */
    OMX_U32 nNumInputBufPending;

    /** NumOutputBufPending */
    OMX_U32 nNumOutputBufPending;

    /** DisableCommandPending */
    OMX_U32 bDisableCommandPending;

    /** DisableCommandParam */
    OMX_U32 bDisableCommandParam;

    /** Lcml Handle */
    OMX_HANDLETYPE pLcmlHandle;

    /** Mark Data pointer */
    OMX_PTR pMarkData;

    /** Mark Buffer pointer */
    OMX_MARKTYPE *pMarkBuf;

    /** Mark Target component */
    OMX_HANDLETYPE hMarkTargetComponent;

    /** pointer to Input Buffer List */
    G726ENC_BUFFERLIST *pInputBufferList;

    /** pointer to Output Buffer List */
    G726ENC_BUFFERLIST *pOutputBufferList;

    /** LCML stream attributes */
    LCML_STRMATTR *strmAttr;

    /** pointer to audio codec parameters */
    G726ENC_AudioCodecParams *pParams;

    /** component name */
    OMX_STRING cComponentName;

    /** component version */
    OMX_VERSIONTYPE ComponentVersion;

    /** pending input buffer headers */
    OMX_BUFFERHEADERTYPE *pInputBufHdrPending[G726ENC_MAX_NUM_OF_BUFS];

    /** pending output buffer headers */
    OMX_BUFFERHEADERTYPE *pOutputBufHdrPending[G726ENC_MAX_NUM_OF_BUFS];

    /** Flag to set when socket node stop callback should not transition
    component to OMX_StateIdle */
    OMX_U32 bNoIdleOnStop;

    /** Flag set when socket node is stopped */
    OMX_U32 bDspStoppedWhileExecuting;

    /** Number of outstanding FillBufferDone() calls */
    OMX_U32 nOutStandingFillDones;

#ifndef UNDER_CE
    /** sync mutexes and signals */
    pthread_mutex_t AlloBuf_mutex;
    pthread_cond_t AlloBuf_threshold;
    OMX_U8 AlloBuf_waitingsignal;

    pthread_mutex_t InLoaded_mutex;
    pthread_cond_t InLoaded_threshold;
    OMX_U8 InLoaded_readytoidle;

    pthread_mutex_t InIdle_mutex;
    pthread_cond_t InIdle_threshold;
    OMX_U8 InIdle_goingtoloaded;
#endif
    /** pointer to LCML lib */
    void* ptrLibLCML;

    /** frame size array */
    OMX_U8 G726FrameSize[4];

    /** component role */
    OMX_PARAM_COMPONENTROLETYPE componentRole;

    /** device string */
    OMX_STRING* sDeviceString;

    /** runtime input buffers */
    OMX_U8 nRuntimeInputBuffers;

    /** runtime output buffers */
    OMX_U8 nRuntimeOutputBuffers;

    /** hold buffer */
    OMX_U8 *pHoldBuffer;

    /** hold length */
    OMX_U16 nHoldLength;

    /** temp buffer */
    OMX_U8 *ptempBuffer;

    /** last out buffer arrived */
    OMX_BUFFERHEADERTYPE *lastOutBufArrived;

    /** last buffer sent */
    OMX_U8 LastBufSent;

    /** Keep buffer timestamps **/
    OMX_S64 arrTimestamp[G726ENC_MAX_NUM_OF_BUFS];
    /** Keep buffer nTickCounts **/
    OMX_S64 arrTickCount[G726ENC_MAX_NUM_OF_BUFS];
    /** Index to arrTimestamp[], used for input buffer timestamps */
    OMX_U8 IpBufindex;
    /** Index to arrTimestamp[], used for output buffer timestamps */
    OMX_U8 OpBufindex;

    /** preempted flag */
    OMX_BOOL bPreempted;

    /** Pointer to RM callback **/
#ifdef RESOURCE_MANAGER_ENABLED
    RMPROXY_CALLBACKTYPE rmproxyCallback;
#endif

} G726ENC_COMPONENT_PRIVATE;


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
*  @param hComp			OMX Handle
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*
*/
/* =================================================================================== */
OMX_EXPORT OMX_ERRORTYPE OMX_ComponentInit (OMX_HANDLETYPE hComp);
#endif

#define G726ENC_EXIT_COMPONENT_THRD  10
/* =================================================================================== */
/**
*  G726ENC_CompThread()  Component thread
*
*
*  @param pThreadData		void*
*
*  @return void*
*
*/
/* =================================================================================== */
void* G726ENC_CompThread(void* pThreadData);
/* =================================================================================== */
/**
*  G726ENC_StartComponentThread()  Starts component thread
*
*
*  @param hComp			OMX Handle
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*
*/
/* =================================================================================== */
OMX_ERRORTYPE G726ENC_StartComponentThread(OMX_HANDLETYPE pHandle);
/* =================================================================================== */
/**
*  G726ENC_StopComponentThread()  Stops component thread
*
*
*  @param hComp			OMX Handle
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*
*/
/* =================================================================================== */
OMX_ERRORTYPE G726ENC_StopComponentThread(OMX_HANDLETYPE pHandle);
/* =================================================================================== */
/**
*  G726ENC_FreeCompResources()  Frees allocated memory
*
*
*  @param hComp			OMX Handle
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*
*/
/* =================================================================================== */
OMX_ERRORTYPE G726ENC_FreeCompResources(OMX_HANDLETYPE pComponent);
/* =================================================================================== */
/**
*  G726ENC_GetCorrespondingLCMLHeader()  Returns LCML header
* that corresponds to the given buffer
*
*  @param pComponentPrivate	Component private data
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*/
/* =================================================================================== */
OMX_ERRORTYPE G726ENC_GetCorrespondingLCMLHeader( G726ENC_COMPONENT_PRIVATE *pComponentPrivate,
                                                OMX_U8 *pBuffer,
                                                OMX_DIRTYPE eDir,
                                                G726ENC_LCML_BUFHEADERTYPE **ppLcmlHdr);
/* =================================================================================== */
/**
*  G726ENC_LCMLCallback() Callback from LCML
*
*  @param event		Codec Event
*
*  @param args		Arguments from LCML
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*/
/* =================================================================================== */
OMX_ERRORTYPE G726ENC_LCMLCallback(TUsnCodecEvent event,
                                    void * args [10]);
/* =================================================================================== */
/**
*  G726ENC_FillLCMLInitParams() Fills the parameters needed
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
OMX_ERRORTYPE G726ENC_FillLCMLInitParams(OMX_HANDLETYPE pHandle,
                                          LCML_DSP *plcml_Init,
                                          OMX_U16 arr[]);
/* =================================================================================== */
/**
*  G726ENC_GetBufferDirection() Returns direction of pBufHeader
*
*  @param pBufHeader		Buffer header
*
*  @param eDir				Buffer direction
*
*  @param pComponentPrivate	Component private data
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*/
/* =================================================================================== */
OMX_ERRORTYPE G726ENC_GetBufferDirection(OMX_BUFFERHEADERTYPE *pBufHeader,
										  OMX_DIRTYPE *eDir);
/* ===========================================================  */
/**
*  G726ENC_HandleCommand()  Handles commands sent via SendCommand()
*
*  @param pComponentPrivate	Component private data
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*/
/* =================================================================================== */
OMX_U32 G726ENC_HandleCommand(G726ENC_COMPONENT_PRIVATE *pComponentPrivate);
/* =================================================================================== */
/**
*  G726ENC_HandleDataBufFromApp()  Handles data buffers received
* from the IL Client
*
*  @param pComponentPrivate	Component private data
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*/
/* =================================================================================== */
OMX_ERRORTYPE G726ENC_HandleDataBufFromApp(OMX_BUFFERHEADERTYPE *pBufHeader,
                                           G726ENC_COMPONENT_PRIVATE *pComponentPrivate);
/* =================================================================================== */
/**
*  G726ENC_GetLCMLHandle()  Get the handle to the LCML
*
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*/
/* =================================================================================== */
OMX_HANDLETYPE G726ENC_GetLCMLHandle(G726ENC_COMPONENT_PRIVATE *pComponentPrivate);
/* =================================================================================== */
/**
*  G726ENC_FreeLCMLHandle()  Frees the handle to the LCML
*
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*/
/* =================================================================================== */
OMX_ERRORTYPE G726ENC_FreeLCMLHandle();
/* =================================================================================== */
/**
*  G726ENC_CleanupInitParams()  Starts component thread
*
*  @param pComponent		OMX Handle
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*/
/* =================================================================================== */
OMX_ERRORTYPE G726ENC_CleanupInitParams(OMX_HANDLETYPE pHandle);
/* =================================================================================== */
/**
*  G726ENC_SetPending()  Called when the component queues a buffer
* to the LCML
*
*  @param pComponentPrivate		Component private data
*
*  @param pBufHdr				Buffer header
*
*  @param eDir					Direction of the buffer
*
*  @return None
*/
/* =================================================================================== */
void G726ENC_SetPending(G726ENC_COMPONENT_PRIVATE *pComponentPrivate,
						 OMX_BUFFERHEADERTYPE *pBufHdr,
						 OMX_DIRTYPE eDir,
						 OMX_U32 lineNumber);
/* =================================================================================== */
/**
*  G726ENC_ClearPending()  Called when a buffer is returned
* from the LCML
*
*  @param pComponentPrivate		Component private data
*
*  @param pBufHdr				Buffer header
*
*  @param eDir					Direction of the buffer
*
*  @return None
*/
/* =================================================================================== */
void G726ENC_ClearPending(G726ENC_COMPONENT_PRIVATE *pComponentPrivate,
						   OMX_BUFFERHEADERTYPE *pBufHdr,
						   OMX_DIRTYPE eDir,
						   OMX_U32 lineNumber);
/* =================================================================================== */
/**
*  G726ENC_IsPending()
*
*
*  @param pComponentPrivate		Component private data
*
*  @return OMX_ErrorNone = Successful
*          Other error code = fail
*/
/* =================================================================================== */
OMX_U32 G726ENC_IsPending(G726ENC_COMPONENT_PRIVATE *pComponentPrivate,
						   OMX_BUFFERHEADERTYPE *pBufHdr,
						   OMX_DIRTYPE eDir);
/* =================================================================================== */
/**
*  G726ENC_FillLCMLInitParamsEx()  Fills the parameters needed
* to initialize the LCML without recreating the socket node
*
*  @param pComponent			OMX Handle
*
*  @return None
*/
/* =================================================================================== */
OMX_ERRORTYPE G726ENC_FillLCMLInitParamsEx(OMX_HANDLETYPE pComponent);
/* =================================================================================== */
/**
*  G726ENC_IsValid() Returns whether a buffer is valid
*
*
*  @param pComponentPrivate		Component private data
*
*  @param pBuffer				Data buffer
*
*  @param eDir					Buffer direction
*
*  @return OMX_True = Valid
*          OMX_False= Invalid
*/
/* =================================================================================== */
OMX_U32 G726ENC_IsValid(G726ENC_COMPONENT_PRIVATE *pComponentPrivate,
						 OMX_U8 *pBuffer,
						 OMX_DIRTYPE eDir);


#ifdef RESOURCE_MANAGER_ENABLED
/***********************************
 *  Callback to the RM                                       *
 ***********************************/
void G726ENC_ResourceManagerCallback(RMPROXY_COMMANDDATATYPE cbData);
#endif

/* ======================================================================= */
/** OMX_G726ENC_INDEXAUDIOTYPE  Defines the custom configuration settings
*                              for the component
*
*  @param  OMX_IndexCustomG726ENCModeConfig      Sets the DASF mode
*
*
*/
/*  ==================================================================== */
typedef enum OMX_G726ENC_INDEXAUDIOTYPE {
	OMX_IndexCustomG726ENCModeConfig = 0xFF000001,
	OMX_IndexCustomG726ENCStreamIDConfig,
	OMX_IndexCustomG726ENCDataPath
}OMX_G726ENC_INDEXAUDIOTYPE;


#endif  /* OMX_G726ENC_UTILS__H */
