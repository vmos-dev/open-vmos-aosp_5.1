
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
* @file OMX_JpegDec_Utils.h
*
** This is a header file for a JPEG decoder.

* ================================================================================
*/

#ifndef OMX_JPEGDEC_UTILS__H
#define OMX_JPEGDEC_UTILS__H

#include <OMX_Component.h>
#include <OMX_IVCommon.h>
#include "LCML_DspCodec.h"
#include "LCML_Types.h"
#include "LCML_CodecInterface.h"
#include <pthread.h>
#include <OMX_Core.h>
#include <OMX_Types.h>
#include <OMX_Image.h>
#include <sys/time.h>
#include <stdio.h>
#include <errno.h>
#include <OMX_TI_Common.h>
#include <OMX_TI_Debug.h>

#include <utils/Log.h>
#define LOG_TAG "OMX_JPGDEC"

#ifdef RESOURCE_MANAGER_ENABLED
#include <ResourceManagerProxyAPI.h>
#endif

#ifdef __PERF_INSTRUMENTATION__
    #include "perf.h"
#endif


#ifdef UNDER_CE
    #include <oaf_debug.h>
#endif


#define JPEGDEC1MPImage 1000000
#define JPEGDEC2MPImage 2000000
#define JPEGDEC3MPImage 3000000
#define JPEGDEC4MPImage 4000000

#define COMP_MAX_NAMESIZE 127

/*Linked List */

typedef struct Node {
    struct Node *pNextNode;
    void *pValue;
} Node;

typedef struct LinkedList {
    Node *pRoot;
}   LinkedList;

LinkedList AllocList;

void LinkedList_Create(LinkedList *LinkedList);
void LinkedList_AddElement(LinkedList *LinkedList, void *pValue);
void LinkedList_FreeElement(LinkedList *LinkedList, void *pValue);
void LinkedList_FreeAll(LinkedList *LinkedList);
void LinkedList_DisplayAll(LinkedList *LinkedList);
void LinkedList_Destroy(LinkedList *LinkedList);

/*
 *     M A C R O S
 */
#define KHRONOS_1_1

#define OMX_CONF_INIT_STRUCT(_s_, _name_)   \
    memset((_s_), 0x0, sizeof(_name_)); \
    (_s_)->nSize = sizeof(_name_);      \
    (_s_)->nVersion.s.nVersionMajor = 0x1;  \
    (_s_)->nVersion.s.nVersionMinor = 0x0;  \
    (_s_)->nVersion.s.nRevision = 0x0;      \
    (_s_)->nVersion.s.nStep = 0x0


#define OMX_MEMCPY_CHECK(_s_)\
{\
    if (_s_ == NULL) { \
    eError = OMX_ErrorInsufficientResources;  \
    goto EXIT;   \
    } \
}
#define OMX_CHECK_PARAM(_ptr_)  \
{   \
    if(!_ptr_) {    \
    eError = OMX_ErrorBadParameter; \
    goto EXIT; \
    }   \
}

#define JPEGDEC_OMX_CONF_CHECK_CMD(_ptr1, _ptr2, _ptr3)\
do {					       \
    if(!_ptr1 || !_ptr2 || !_ptr3){	       \
        eError = OMX_ErrorBadParameter;        \
        goto EXIT;                             \
    }					       \
} while(0)

#define OMX_MALLOC(_pStruct_, _size_)   \
    _pStruct_ = malloc(_size_);  \
    if(_pStruct_ == NULL){  \
        eError = OMX_ErrorInsufficientResources;    \
        goto EXIT;  \
    } \
    memset(_pStruct_, 0, _size_);\
    LinkedList_AddElement(&AllocList, _pStruct_);

#define OMX_FREE(_ptr)   \
{                     \
    if (_ptr != NULL) { \
        LinkedList_FreeElement(&AllocList, _ptr);\
        _ptr = NULL; \
    }                \
}

#define OMX_FREEALL()   \
{                     \
        LinkedList_FreeAll(&AllocList);\
}

#define JPEGDEC_WAIT_PORT_POPULATION(_pComponentPrivate_)    \
{                                                                           \
    int nRet = 0x0;                                                         \
    struct timespec  ts;                                                    \
    struct timeval sTime;                                                   \
    struct timezone sTimeZone;                                              \
                                                                            \
    pthread_mutex_lock(&((_pComponentPrivate_)->mJpegDecMutex));     \
    gettimeofday(&sTime, &sTimeZone);                                       \
    ts.tv_sec = sTime.tv_sec;                                               \
    ts.tv_sec += JPEGDEC_TIMEOUT;                                      \
                                                                   \
    nRet = pthread_cond_timedwait(&((_pComponentPrivate_)->sPortPopulated_cond),\
                                  &((_pComponentPrivate_)->mJpegDecMutex), \
                                  &ts);                                 \
    if (nRet == ETIMEDOUT)                                              \
    {                                                                   \
        OMX_PRBUFFER4((_pComponentPrivate_)->dbg, "Wait for port to be Populated time-out"); \
        pthread_mutex_unlock(&((_pComponentPrivate_)->mJpegDecMutex));\
        \
        eError = OMX_ErrorPortUnresponsiveDuringAllocation;\
        \
        break;                                  \
    }                                                                   \
   pthread_mutex_unlock(&((_pComponentPrivate_)->mJpegDecMutex));     \
}

#define JPEGDEC_WAIT_PORT_UNPOPULATION(_pComponentPrivate_)    \
{                                                                           \
    int nRet = 0x0;                                                         \
    struct timespec  ts;                                                    \
    struct timeval sTime;                                                   \
    struct timezone sTimeZone;                                              \
                                                                            \
    pthread_mutex_lock(&((_pComponentPrivate_)->mJpegDecMutex));     \
    gettimeofday(&sTime, &sTimeZone);                                       \
    ts.tv_sec = sTime.tv_sec;                                               \
    ts.tv_sec += JPEGDEC_TIMEOUT;                                      \
                                                                   \
    nRet = pthread_cond_timedwait(&((_pComponentPrivate_)->sPortPopulated_cond),\
                                  &((_pComponentPrivate_)->mJpegDecMutex), \
                                  &ts);                                 \
    if (nRet == ETIMEDOUT)                                              \
    {                                                                   \
        OMX_PRBUFFER4((_pComponentPrivate_)->dbg, "Wait for port to be Unpopulated time-out"); \
        pthread_mutex_unlock(&((_pComponentPrivate_)->mJpegDecMutex));\
        \
        eError = OMX_ErrorPortUnresponsiveDuringDeallocation;\
        \
        break;                                  \
    }                                                                   \
   pthread_mutex_unlock(&((_pComponentPrivate_)->mJpegDecMutex));     \
}

#define JPEGDEC_WAIT_FLUSH(_pComponentPrivate_)     \
{                                                                                           \
    int nRet = 0x0;                                                         \
    struct timespec  ts;                                                    \
    struct timeval sTime;                                                   \
    struct timezone sTimeZone;                                              \
                                                                            \
    pthread_mutex_lock(&((_pComponentPrivate_)->mJpegDecFlushMutex));     \
    gettimeofday(&sTime, &sTimeZone);                                       \
    ts.tv_sec = sTime.tv_sec;                                               \
    ts.tv_sec += JPEGDEC_TIMEOUT;                                      \
                                                                   \
    nRet = pthread_cond_timedwait(&((_pComponentPrivate_)->sFlush_cond),\
                                  &((_pComponentPrivate_)->mJpegDecFlushMutex), \
                                  &ts);                                 \
    if (nRet == ETIMEDOUT)                                              \
    {                                                                   \
	OMX_PRBUFFER4((_pComponentPrivate_)->dbg, "Wait for port to be Unpopulated time-out"); \
        pthread_mutex_unlock(&((_pComponentPrivate_)->mJpegDecFlushMutex));\
        _pComponentPrivate_->cbInfo.EventHandler(_pComponentPrivate_->pHandle,\
                                               _pComponentPrivate_->pHandle->pApplicationPrivate,\
                                               OMX_EventError,\
                                               OMX_ErrorTimeout,\
                                               OMX_TI_ErrorMajor,\
                                               "TimeOut - whlie doing flush");\
        break;                                  \
    }                                                                   \
   pthread_mutex_unlock(&((_pComponentPrivate_)->mJpegDecFlushMutex));     \
}


#define OMX_DPRINT_ADDRESS(_s_, _ptr_)  \
    OMX_PRINT2(pComponentPrivate->dbg, "%s = %p\n", _s_, _ptr_);

#ifdef RESOURCE_MANAGER_ENABLED
#define OMX_GET_RM_VALUE(_Res_, _RM_) \
{   \
    if (_Res_ <= JPEGDEC1MPImage){  \
        _RM_ = 25;  \
        }   \
    else if (_Res_ <= JPEGDEC2MPImage){ \
        _RM_ = 40;  \
        }   \
    else if (_Res_ <= JPEGDEC3MPImage){ \
        _RM_ = 55;  \
        }   \
    else if (_Res_ <= JPEGDEC4MPImage){ \
        _RM_ = 70;  \
        }   \
    else {  \
        _RM_ = 90;  \
        }   \
        \
    OMX_PRMGR2(pComponentPrivate->dbg, "Value in MHz requested to RM = %d\n",_RM_); \
}
#endif

#define NUM_OF_BUFFERS 4
#define NUM_OF_PORTS 2


#define OMX_JPEGDEC_NUM_DLLS (3)
#ifdef UNDER_CE
    #define JPEG_DEC_NODE_DLL "/windows/jpegdec_sn.dll64P"
    #define JPEG_COMMON_DLL "/windows/usn.dll64P"
    #define USN_DLL "/windows/usn.dll64P"
#else
#define JPEG_DEC_NODE_DLL "jpegdec_sn.dll64P"
#define JPEG_COMMON_DLL "usn.dll64P"
#define USN_DLL "usn.dll64P"
#endif

#define JPGDEC_SNTEST_STRMCNT       2
#define JPGDEC_SNTEST_INSTRMID      0
#define JPGDEC_SNTEST_OUTSTRMID     1
#define JPGDEC_SNTEST_MAX_HEIGHT    3000
#define JPGDEC_SNTEST_MAX_WIDTH     4000
#define JPGDEC_SNTEST_PROG_FLAG     1
#define JPGDEC_SNTEST_INBUFCNT      4
#define JPGDEC_SNTEST_OUTBUFCNT     4

#define OMX_NOPORT 0xFFFFFFFE

#define JPEGD_DSPSTOP       0x01
#define JPEGD_BUFFERBACK    0x02
#define JPEGD_IDLEREADY     ( JPEGD_DSPSTOP | JPEGD_BUFFERBACK )

#define DSP_MMU_FAULT_HANDLING

#define OMX_CustomCommandStopThread (OMX_CommandMax - 1)

typedef OMX_ERRORTYPE (*jpegdec_fpo)(OMX_HANDLETYPE);

static const struct DSP_UUID JPEGDSOCKET_TI_UUID = {
    0x5D9CB711, 0x4645, 0x11d6, 0xb0, 0xdc, {
        0x00, 0xc0, 0x4f, 0x1f, 0xc0, 0x36
    }
};

static const struct DSP_UUID USN_UUID = {
    0x79A3C8B3, 0x95F2, 0x403F, 0x9A, 0x4B, {
        0xCF, 0x80, 0x57, 0x73, 0x05, 0x41
    }
};

typedef enum JPEGDEC_COMP_PORT_TYPE
{
    JPEGDEC_INPUT_PORT = 0,
    JPEGDEC_OUTPUT_PORT
}JPEGDEC_COMP_PORT_TYPE;


typedef enum JPEGDEC_BUFFER_OWNER
{
    JPEGDEC_BUFFER_CLIENT = 0x0,
    JPEGDEC_BUFFER_COMPONENT_IN,
    JPEGDEC_BUFFER_COMPONENT_OUT,
    JPEGDEC_BUFFER_DSP,
    JPEGDEC_BUFFER_TUNNEL_COMPONENT
} JPEGDEC_BUFFER_OWNER;

typedef struct _JPEGDEC_BUFFERFLAG_TRACK {
    OMX_U32 flag;
    OMX_U32 buffer_id;
    OMX_HANDLETYPE hMarkTargetComponent; 
    OMX_PTR pMarkData;
} JPEGDEC_BUFFERFLAG_TRACK;

typedef struct _JPEGDEC_BUFFERMARK_TRACK {
    OMX_U32 buffer_id;
    OMX_HANDLETYPE hMarkTargetComponent; 
    OMX_PTR pMarkData;
} JPEGDEC_BUFFERMARK_TRACK;

typedef struct JPEGDEC_BUFFER_PRIVATE {
    OMX_BUFFERHEADERTYPE* pBufferHdr;
    OMX_PTR pUALGParams;
    JPEGDEC_BUFFER_OWNER eBufferOwner;
    OMX_BOOL bAllocbyComponent;
    OMX_BOOL bReadFromPipe;
} JPEGDEC_BUFFER_PRIVATE;

typedef struct JPEGDEC_PORT_TYPE
{
    OMX_HANDLETYPE hTunnelComponent;
    OMX_U32 nTunnelPort;
    JPEGDEC_BUFFER_PRIVATE* pBufferPrivate[NUM_OF_BUFFERS];
    JPEGDEC_BUFFERFLAG_TRACK sBufferFlagTrack[NUM_OF_BUFFERS];
    JPEGDEC_BUFFERMARK_TRACK sBufferMarkTrack[NUM_OF_BUFFERS];
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef;
    OMX_PARAM_BUFFERSUPPLIERTYPE *pParamBufSupplier;
    OMX_IMAGE_PARAM_PORTFORMATTYPE* pPortFormat;
    OMX_U8 nBuffCount;
} JPEGDEC_PORT_TYPE;

typedef struct OMX_CUSTOM_IMAGE_DECODE_SECTION
{
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nMCURow;
    OMX_U32 nAU;
    OMX_BOOL bSectionsInput;
    OMX_BOOL bSectionsOutput;
}OMX_CUSTOM_IMAGE_DECODE_SECTION;

typedef struct OMX_CUSTOM_IMAGE_DECODE_SUBREGION
{
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nXOrg;         /*Sectional decoding: X origin*/
    OMX_U32 nYOrg;         /*Sectional decoding: Y origin*/
    OMX_U32 nXLength;      /*Sectional decoding: X lenght*/
    OMX_U32 nYLength;      /*Sectional decoding: Y lenght*/
}OMX_CUSTOM_IMAGE_DECODE_SUBREGION;


typedef struct OMX_CUSTOM_RESOLUTION 
{
	OMX_U32 nWidth;
	OMX_U32 nHeight;
} OMX_CUSTOM_RESOLUTION;


typedef struct JPEGDEC_COMPONENT_PRIVATE
{
    JPEGDEC_PORT_TYPE *pCompPort[NUM_OF_PORTS];
    OMX_PORT_PARAM_TYPE* pPortParamType;
    OMX_PRIORITYMGMTTYPE* pPriorityMgmt;
    OMX_CONFIG_SCALEFACTORTYPE* pScalePrivate;
    OMX_PORT_PARAM_TYPE* pAudioPortType;
    OMX_PORT_PARAM_TYPE* pVideoPortType;
    OMX_PORT_PARAM_TYPE* pOtherPortType;
    /* Handle for use with async callbacks */
    OMX_CALLBACKTYPE cbInfo;
    /*Component name OMX.TI.JPEG.decoder*/
    OMX_STRING      cComponentName;
    OMX_VERSIONTYPE ComponentVersion;
    OMX_VERSIONTYPE SpecVersion;
    
    OMX_U8 nSize;
    OMX_STATETYPE nToState;
    OMX_U8        ExeToIdleFlag;  /* StateCheck */
    /* This is component handle */
    OMX_COMPONENTTYPE* pHandle;
    /* Current state of this component */
    OMX_STATETYPE nCurState;
    /* The component thread handle */
    pthread_t pComponentThread;
    /* The pipes to maintain free buffers */
    int nFree_outBuf_Q[2];
    /* The pipes to maintain input buffers sent from app*/
    int nFilled_inpBuf_Q[2];
    /* The pipes for sending buffers to the thread */
    int nCmdPipe[2];
    int nCmdDataPipe[2];
    void* pLcmlHandle;
    void * pDllHandle;
    int nProgressive;
    int nProfileID;
    int nIsLCMLActive;
    OMX_PTR pMarkData;
    OMX_MARKTYPE *pMarkBuf;
    OMX_HANDLETYPE hMarkTargetComponent;
    LCML_DSP_INTERFACE* pLCML;
    OMX_BOOL bFlushComplete;
    OMX_U32 nInPortIn;
    OMX_U32 nOutPortOut;
    OMX_BOOL bInportDisableComplete;
    OMX_BOOL bOutportDisableComplete;
    OMX_U32 nMarkPort;

    pthread_mutex_t mJpegDecMutex;
    pthread_cond_t  sStop_cond;

    /* Condition to signal threads PortTransition */
    pthread_cond_t sPortPopulated_cond;
    /*Condition signal Flush & Mutex*/
    pthread_mutex_t mJpegDecFlushMutex;
    pthread_cond_t sFlush_cond;

#ifdef KHRONOS_1_1
    OMX_PARAM_COMPONENTROLETYPE* pCompRole;
    OMX_IMAGE_PARAM_QUANTIZATIONTABLETYPE* pQuantTable;
    OMX_IMAGE_PARAM_HUFFMANTTABLETYPE* pHuffmanTable;
#endif

    int nInputFrameWidth;
    int nOutputColorFormat;

#ifdef RESOURCE_MANAGER_ENABLED
    RMPROXY_CALLBACKTYPE rmproxyCallback;
#endif

    OMX_BOOL bPreempted;
    
#ifdef __PERF_INSTRUMENTATION__
    PERF_OBJHANDLE pPERF, pPERFcomp;
#endif
#ifdef KHRONOS_1_1
    OMX_PARAM_COMPONENTROLETYPE componentRole;
#endif
    OMX_CUSTOM_IMAGE_DECODE_SECTION* pSectionDecode;
    OMX_CUSTOM_IMAGE_DECODE_SUBREGION* pSubRegionDecode;
    OMX_CUSTOM_RESOLUTION sMaxResolution;
    struct OMX_TI_Debug dbg;
} JPEGDEC_COMPONENT_PRIVATE;


typedef struct
{
    long int lInBufCount;       /*set it to zero*/
    OMX_U32 ulInNumFrame;       /*set it to 1*/
    OMX_U32 ulInFrameAlign;     /*set it to 4*/
    OMX_U32 ulInFrameSize;      /*same as buffer size*/
    OMX_U32 ulInDisplayWidth;   /*Width of the buffer into which the image is to be decoded*/
    OMX_U32 ulInReserved0;      /*could be like thumbnail decode only*/
    OMX_U32 ulInReserved1;      /*could be output format � later. Let�s fix it to 422 always now*/
    OMX_U32 ulInReserved2;      /*could be post processing flag*/
    OMX_U32 ulInReserved3;
    OMX_U32 ulInResizeOption;   /*Rescale factor */
    OMX_U32 ulnumAU;            /*Number of MCUs to decode; set as DEFAULT for full image decoding*/
    OMX_U32 uldecodeHeader;     /*DECODE_AU = Decode at one shot; PARSE_HEADER = Parse header   */
    OMX_U32 maxHeight;          /*Maximum height of image that can be decoded*/
    OMX_U32 maxWidth;           /*Maximum width of image that can be decoded*/
    OMX_U32 maxScans;           /*Maximum number of frames in the input image*/
    OMX_U32 uldataEndianness;   /*Endianness of input data*/
    OMX_U32 forceChromaFormat;  /*Set to CHROMAFORMAT ; Set to DEFAULT to avoid resampling.*/
    OMX_U32 RGB_Format;         /* Set the output RGB format */
                                                    /* 0: BGR24                                 */
                                                    /* 1: BGR32                                 */
                                                    /* 2: RGB16                                 */
    OMX_U32 ulNumMCURow;    /*Slide decoding: Set the numbers of lines to decode*/
    OMX_U32 ulXOrg;         /*Sectional decoding: X origin*/
    OMX_U32 ulYOrg;         /*Sectional decoding: Y origin*/
    OMX_U32 ulXLength;      /*Sectional decoding: X lenght*/
    OMX_U32 ulYLength;      /*Sectional decoding: Y lenght*/
    OMX_U32 ulAlphaRGB;   /* Alpha RGB value, it only takes values from 0 to 255 */
}JPEGDEC_UAlgInBufParamStruct;

typedef struct
{
    long int lOutBufCount;      /*set it to zero*/
    OMX_U32 ulOutNumFrame;      /*set it to 1*/
    OMX_U32 ulOutFrameAlign;    /*set it to 4*/
    OMX_U32 ulOutFrameSize;     /*same as buffer size*/
    OMX_U32 ulOutImgFormat;     /*output format*/
    OMX_U32 ulOutImageWidth;    /*Width of the image*/
    OMX_U32 ulOutImageHeight;   /*Height of the image*/
    OMX_U32 ulOutnProgressiveFlag; /*nProgressive flag*/
    OMX_U32 ulOutErrorCode;     /*error code*/
    OMX_U32 ulOutReserved0;
    OMX_U32 ulOutReserved1;
    OMX_U32 ulOutReserved2;
    OMX_U32 lastMCU;            /* 1-Decoded all MCU�s0 - Decoding not completed*/
    OMX_U32 stride[3];          /*Stride values for Y, U, and V components*/
    OMX_U32 ulOutputHeight;     /* Output Height */
    OMX_U32 ulOutputWidth;      /* Output Width*/
    OMX_U32 ultotalAU;          /* Total number of Access unit(MCU)*/
    OMX_U32 ulbytesConsumed;    /* Total number of bytes consumed*/
    OMX_U32 ulcurrentAU;        /* current access unit number */
    OMX_U32 ulcurrentScan;      /*current scan number*/
}JPEGDEC_UAlgOutBufParamStruct;

typedef enum OMX_INDEXIMAGETYPE
{
    OMX_IndexCustomProgressiveFactor = 0xFF000001,
    OMX_IndexCustomInputFrameWidth,
    OMX_IndexCustomOutputColorFormat,
    OMX_IndexCustomSectionDecode,
    OMX_IndexCustomSubRegionDecode,
    OMX_IndexCustomSetMaxResolution,
    OMX_IndexCustomDebug
}OMX_INDEXIMAGETYPE;

typedef struct _JPEGDEC_CUSTOM_PARAM_DEFINITION
{
    OMX_U8 cCustomParamName[128];
    OMX_INDEXTYPE nCustomParamIndex;
} JPEGDEC_CUSTOM_PARAM_DEFINITION;

/* function declarations */
OMX_ERRORTYPE ComponentDeInit(OMX_HANDLETYPE hComponent);
OMX_U32 HandleCommandJpegDec(JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U32 nParam1);
OMX_U32 HandleCommandFlush(JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U32 nParam1);
OMX_ERRORTYPE DisablePortJpegDec(JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U32 nParam1);
OMX_ERRORTYPE EnablePortJpegDec(JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U32 nParam1);
OMX_ERRORTYPE Start_ComponentThreadJpegDec(OMX_HANDLETYPE pHandle);
OMX_ERRORTYPE HandleDataBuf_FromAppJpegDec(JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate );
OMX_ERRORTYPE HandleDataBuf_FromDspJpegDec(JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE* pBuffHead);
OMX_ERRORTYPE HandleFreeDataBufJpegDec(JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE* pBuffHead );
OMX_ERRORTYPE HandleFreeOutputBufferFromAppJpegDec( JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate );
OMX_ERRORTYPE JpegDec_AllocResources( JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate );
OMX_ERRORTYPE LCML_CallbackJpegDec(TUsnCodecEvent event,void * argsCb [10]);
OMX_ERRORTYPE Free_ComponentResourcesJpegDec(JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate);
OMX_ERRORTYPE Fill_LCMLInitParamsJpegDec(LCML_DSP *lcml_dsp, OMX_U16 arr[], OMX_HANDLETYPE pComponent);
OMX_ERRORTYPE GetLCMLHandleJpegDec(OMX_HANDLETYPE pComponent);
OMX_ERRORTYPE HandleInternalFlush(JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U32 nParam1);
OMX_BOOL IsTIOMXComponent(OMX_HANDLETYPE hComp);
void* OMX_JpegDec_Thread (void* pThreadData);

#ifdef RESOURCE_MANAGER_ENABLED
void ResourceManagerCallback(RMPROXY_COMMANDDATATYPE cbData);
#endif


#endif
