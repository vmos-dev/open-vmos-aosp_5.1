
/*
 *  Copyright 2001-2008 Texas Instruments - http://www.ti.com/
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 * limitations under the License.
 */
/* =============================================================================
*             Texas Instruments OMAP(TM) Platform Software
*  (c) Copyright Texas Instruments, Incorporated.  All Rights Reserved.
*
*  Use of this software is controlled by the terms and conditions found
*  in the license agreement under which this software has been supplied.
* =========================================================================== */
/**
* @file VideoEncTest.c
*
* This file implements OMX Component for MPEG-4 encoder that
* is fully compliant with the OMX specification 1.5.
*
* @path  $(CSLPATH)\src
*
* @rev  0.1
*/
/* -------------------------------------------------------------------------- */
/* =============================================================================
*!
*! Revision History
*! ===================================
*!
*! 02-Feb-2006 mf: Revisions appear in reverse chronological order;
*! that is, newest first.  The date format is dd-Mon-yyyy.
* =========================================================================== */
#define _XOPEN_SOURCE 600

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sched.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/select.h>
#include <time.h>
#include <OMX_Component.h>
#include <getopt.h>
#include <sys/types.h>

#define DSP_MMU_FAULT_HANDLING

#ifdef DSP_MMU_FAULT_HANDLING
#include <dbapi.h>
#include <DSPManager.h>
#include <DSPProcessor.h>
#include <DSPProcessor_OEM.h>
#endif

/* For debug printing
   Add -DAPP_DEBUG to CFLAGS in test Makefile */
#define KHRONOS_1_2
#if VIDENCTEST_DEBUG
    #define VIDENCTEST_MAX_TIME_OUTS 1000000
    #define __VIDENCTEST_PRINT__
    #define __VIDENCTEST_DPRINT__
    #define __VIDENCTEST_MTRACE__
#else
    #define VIDENCTEST_MAX_TIME_OUTS 1000000
#endif
#define VIDENCTEST_UNUSED_ARG(arg) (void)(arg)
#if 0
    #define VIDENCTEST_COLOR
    #define VIDENCTEST_PRINT_PARAMS
#endif

#define VIDENCTEST_USE_DEFAULT_VALUE (OMX_U32)-1
#define VIDENCTEST_USE_DEFAULT_VALUE_UI (unsigned int)-1

#define VIDENCTEST_MALLOC(_p_, _s_, _c_, _h_)                           \
    _p_ = (_c_*)malloc(_s_);                                            \
    if (_p_ == NULL) {                                                  \
        VIDENCTEST_MTRACE("malloc() error.\n");                         \
        eError = OMX_ErrorInsufficientResources;                        \
    }                                                                   \
    else {                                                              \
        VIDENCTEST_MTRACE(": %d :malloc() -> %p\n", __LINE__, _p_);     \
        memset((_p_), 0x0, _s_);                                        \
        if ((_p_) == NULL) {                                            \
            VIDENCTEST_MTRACE("memset() error.\n");                     \
            eError = OMX_ErrorUndefined;                                \
        }                                                               \
        else{                                                           \
            eError = VIDENCTEST_ListAdd(_h_, _p_);                      \
            if (eError == OMX_ErrorInsufficientResources) {             \
                VIDENCTEST_MTRACE("malloc() error.\n");                 \
            }                                                           \
        }                                                               \
    }                                                                   \


#define VIDENCTEST_FREE(_p_, _h_)                           \
    VIDENCTEST_ListRemove(_h_, _p_);                        \
    _p_ = NULL;                                             \

#define VIDENCTEST_CHECK_ERROR(_e_, _s_)                    \
    if (_e_ != OMX_ErrorNone){                              \
        printf("\n------VIDENCTEST FATAL ERROR-------\n %x : %s \n", _e_, _s_);  \
        VIDENCTEST_HandleError(pAppData, _e_);               \
        goto EXIT;                                          \
    }                                                       \

#define VIDENCTEST_CHECK_EXIT(_e_, _s_)                     \
    if (_e_ != OMX_ErrorNone){                              \
        printf("\n------VIDENCTEST ERROR-------\n %x : %s \n", _e_, _s_);  \
        goto EXIT;                                          \
    }

#if 1
    #define CALC_TIME
#endif

#define MAX_UNRESPONSIVE_COUNT 50
#define NUM_OF_IN_BUFFERS 4
#define NUM_OF_OUT_BUFFERS 4
#define MAX_NUM_OF_PORTS 16
#define MAX_EVENTS 256
/*
 *  ANSI escape sequences for outputing text in various colors
 */
#ifdef APP_COLOR
    #define DBG_TEXT_WHITE   "\x1b[1;37;40m"
    #define DBG_TEXT_YELLOW  "\x1b[1;33;40m"
    #define DBG_TEXT_MAGENTA "\x1b[1;35;40m"
    #define DBG_TEXT_GREEN   "\x1b[1;32;40m"
    #define DBG_TEXT_CYAN    "\x1b[1;36;40m"
    #define DBG_TEXT_RED     "\x1b[1;31;40m"
#else
    #define DBG_TEXT_WHITE ""
    #define DBG_TEXT_YELLOW ""
    #define DBG_TEXT_MAGENTA ""
    #define DBG_TEXT_GREEN ""
    #define DBG_TEXT_CYAN ""
    #define DBG_TEXT_RED ""
#endif

#define APP_CONVERT_STATE(_s_, _p_)        \
    if (_p_ == 0) {                        \
        _s_ = "OMX_StateInvalid";          \
    }                                      \
    else if (_p_ == 1) {                   \
        _s_ = "OMX_StateLoaded";           \
    }                                      \
    else if (_p_ == 2) {                   \
        _s_ = "OMX_StateIdle";             \
    }                                      \
    else if (_p_ == 3) {                   \
        _s_ = "OMX_StateExecuting";        \
    }                                      \
    else if (_p_ == 4) {                   \
        _s_ = "OMX_StatePause";            \
    }                                      \
    else if (_p_ == 5) {                   \
        _s_ = "OMX_StateWaitForResources"; \
    }                                      \
    else {                                 \
        _s_ = "UnsupportedCommand";        \
    }


/*static const int iQ16_Const = 1 << 16;*/
static const float fQ16_Const = (float)(1 << 16);

/*static float Q16Tof(int nQ16)
{
    return nQ16 / fQ16_Const;
}*/

static int fToQ16(float f)
{
    return (int)(f*fQ16_Const);
}

#ifdef DSP_MMU_FAULT_HANDLING
static int bInvalid_state;
int LoadBaseImage();
#endif

void VIDENCTEST_Log(const char *szFileName, int iLineNum, const char *szFunctionName, const char *strFormat, ...)
{
    va_list list;
    VIDENCTEST_UNUSED_ARG(szFileName);
    VIDENCTEST_UNUSED_ARG(iLineNum);
    fprintf(stdout, "%s():", szFunctionName);
    va_start(list, strFormat);
    vfprintf(stdout, strFormat, list);
    va_end(list);
}

#ifdef __VIDENCTEST_DPRINT__
    #define VIDENCTEST_DPRINT(STR, ARG...) VIDENCTEST_Log(__FILE__, __LINE__, __FUNCTION__, STR, ##ARG)
#else
    #define VIDENCTEST_DPRINT(...)
#endif

#ifdef __VIDENCTEST_MTRACE__
    #define VIDENCTEST_MTRACE(STR, ARG...) VIDENCTEST_Log(__FILE__, __LINE__, __FUNCTION__, STR, ##ARG)
#else
    #define VIDENCTEST_MTRACE(...)
#endif

#ifdef __VIDENCTEST_PRINT__
    #define VIDENCTEST_PRINT(...) fprintf(stdout, __VA_ARGS__)
#else
    #define VIDENCTEST_PRINT(...)
#endif

OMX_STRING StrVideoEncoder= "OMX.TI.Video.encoder";

typedef enum VIDEOENC_PORT_INDEX {
    VIDENC_INPUT_PORT = 0x0,
    VIDENC_OUTPUT_PORT
} VIDEOENC_PORT_INDEX;

typedef enum VIDENCTEST_STATE {
    VIDENCTEST_StateLoaded = 0x0,
    VIDENCTEST_StateUnLoad,
    VIDENCTEST_StateReady,
    VIDENCTEST_StateStarting,
    VIDENCTEST_StateEncoding,
    VIDENCTEST_StateStopping,
    VIDENCTEST_StateConfirm,
    VIDENCTEST_StateWaitEvent,
    VIDENCTEST_StatePause,
    VIDENCTEST_StateStop,
    VIDENCTEST_StateError
} VIDENCTEST_STATE;

typedef enum VIDENCTEST_TEST_TYPE {
    VIDENCTEST_FullRecord = 0x0,
    VIDENCTEST_PartialRecord,
    VIDENCTEST_PauseResume,
    VIDENCTEST_StopRestart
} VIDENCTEST_TEST_TYPE;

typedef enum VIDENC_TEST_NAL_FORMAT {
    VIDENC_TEST_NAL_UNIT = 0,
    VIDENC_TEST_NAL_SLICE,
    VIDENC_TEST_NAL_FRAME
}VIDENC_TEST_NAL_FORMAT;

#ifndef KHRONOS_1_2
typedef enum OMX_EXTRADATATYPE {
    OMX_ExtraDataNone = 0,
    OMX_ExtraDataQuantization
} OMX_EXTRADATATYPE;
#endif

typedef struct OMX_OTHER_EXTRADATATYPE_1_1_2 {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_EXTRADATATYPE eType;
    OMX_U32 nDataSize;
    OMX_U8 data[1];
} OMX_OTHER_EXTRADATATYPE_1_1_2;

typedef struct APP_TIME{
    time_t rawTime;
    struct tm* pTimeInfo;
    int nHrs;
    int nMin;
    int nSec;
    OMX_BOOL bInitTime;
    int nTotalTime;
} APP_TIME;

/* Structure used for the Memory List (Link-List)*/
typedef struct VIDENCTEST_NODE {
    OMX_PTR pData;
    struct VIDENCTEST_NODE* pNext;
}VIDENCTEST_NODE;

typedef struct MYBUFFER_DATA{
    time_t rawTime;
    struct tm* pTimeInfo;
} MYBUFFER_DATA;

typedef struct MYDATATYPE {
    OMX_HANDLETYPE pHandle;
    char* szInFile;
    char* szOutFile;
    char* szOutFileNal;
    int nWidth;
    int nHeight;
    OMX_U8 eColorFormat;
    OMX_U32 nBitrate;
    OMX_U8 nFramerate;
    OMX_U8 eCompressionFormat;
    OMX_U8 eLevel;
    OMX_U32 nOutBuffSize;
    OMX_STATETYPE eState;
    OMX_PORT_PARAM_TYPE* pVideoInit;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef[MAX_NUM_OF_PORTS];
    OMX_PARAM_PORTDEFINITIONTYPE* pInPortDef;
    OMX_PARAM_PORTDEFINITIONTYPE* pOutPortDef;
    OMX_VIDEO_PARAM_AVCTYPE* pH264;
    OMX_VIDEO_AVCLEVELTYPE eLevelH264;
    OMX_VIDEO_PARAM_H263TYPE* pH263;
    OMX_VIDEO_H263LEVELTYPE eLevelH63;
    OMX_VIDEO_PARAM_MPEG4TYPE* pMpeg4;
    OMX_VIDEO_MPEG4LEVELTYPE eLevelMpeg4;
    int IpBuf_Pipe[2];
    int OpBuf_Pipe[2];
    int eventPipe[2];
    int fdmax;
    FILE* fIn;
    FILE* fOut;
    FILE* fNalnd;
    OMX_U32 nCurrentFrameIn;
    OMX_U32 nCurrentFrameOut;
    OMX_S32 nRetVal;
    OMX_CALLBACKTYPE* pCb;
    OMX_COMPONENTTYPE* pComponent;
    OMX_BUFFERHEADERTYPE* pInBuff[NUM_OF_IN_BUFFERS];
    OMX_BUFFERHEADERTYPE* pOutBuff[NUM_OF_OUT_BUFFERS];
    OMX_U8* pIBuffer[NUM_OF_IN_BUFFERS];
    OMX_U8* pOBuffer[NUM_OF_OUT_BUFFERS];
    OMX_VIDEO_PARAM_BITRATETYPE* pVidParamBitrate;
    OMX_VIDEO_CONTROLRATETYPE eControlRate;
    OMX_VIDEO_PARAM_QUANTIZATIONTYPE* pQuantization;
    OMX_U32 nQpI;
    OMX_BOOL bAllocateIBuf;
    OMX_BOOL bAllocateOBuf;
    OMX_INDEXTYPE nVideoEncodeCustomParamIndex;
    OMX_U32 nVBVSize;
    OMX_BOOL bDeblockFilter;
    OMX_BOOL bForceIFrame;
    OMX_U32 nIntraFrameInterval;
    OMX_U32 nGOBHeaderInterval;
    OMX_U32 nTargetFrameRate;
    OMX_U32 nAIRRate;
    OMX_U32 nTargetBitRate;
    OMX_U32 nStartPortNumber;
    OMX_U32 nPorts;
    OMX_U8 nInBufferCount;
    OMX_U8 nOutBufferCount;
    void* pEventArray[MAX_EVENTS];
    OMX_U8 nEventCount;
    OMX_BOOL bStop;
    OMX_BOOL bExit;
    OMX_U32 nSizeIn;
    OMX_U32 nSizeOut;

    OMX_U32 nReferenceFrame;
    OMX_U32 nNumberOfTimesTodo;
    OMX_U32 nNumberOfTimesDone;
    OMX_U32 nUnresponsiveCount;
    VIDENCTEST_NODE*  pMemoryListHead; /* Used in Memory List (Link-List) */
    VIDENCTEST_STATE eCurrentState;
    VIDENCTEST_TEST_TYPE eTypeOfTest;
    OMX_U32 nMIRRate;
    OMX_U32 nResynchMarkerSpacing;
    unsigned int nEncodingPreset;
    OMX_U8 nUnrestrictedMV;
    OMX_U8 NalFormat;
    OMX_U8 bLastOutBuffer;
    OMX_U32  nQPIoF;
} MYDATATYPE;

typedef struct EVENT_PRIVATE {
    OMX_EVENTTYPE eEvent;
    OMX_U32 nData1;
    OMX_U32 nData2;
    MYDATATYPE* pAppData;
    OMX_PTR pEventData;
} EVENT_PRIVATE;

/* safe routine to get the maximum of 2 integers */
/* inline int maxint(int a, int b) */
int maxint(int a, int b)
{
    return(a>b) ? a : b;
}

/*-----------------------------------------------------------------------------*/
/**
  * ListCreate()
  *
  * Creates the List Head of the Component Memory List.
  *
  * @param pListHead VIDENCTEST_NODE double pointer with the List Header of the Memory List.
  *
  * @retval OMX_ErrorNone
  *               OMX_ErrorInsufficientResources if the malloc fails
  *
  **/
/*-----------------------------------------------------------------------------*/
OMX_ERRORTYPE VIDENCTEST_ListCreate(struct VIDENCTEST_NODE** pListHead)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    *pListHead = (VIDENCTEST_NODE*)malloc(sizeof(VIDENCTEST_NODE)); /* need to malloc!!! */
    if (*pListHead == NULL) {
        VIDENCTEST_DPRINT("malloc() error.\n");
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    VIDENCTEST_DPRINT("Create MemoryListHeader[%p]\n", *pListHead);
    memset(*pListHead, 0x0, sizeof(VIDENCTEST_NODE));

EXIT:
    return eError;
}

/*-----------------------------------------------------------------------------*/
/**
  * ListAdd()
  *
  * Add a new node to Component Memory List
  *
  * @param pListHead VIDENCTEST_NODE Points List Header of the Memory List.
  *                pData OMX_PTR points to the new allocated data.
  * @retval OMX_ErrorNone
  *               OMX_ErrorInsufficientResources if the malloc fails
  *
  **/
/*-----------------------------------------------------------------------------*/

OMX_ERRORTYPE VIDENCTEST_ListAdd(struct VIDENCTEST_NODE* pListHead, OMX_PTR pData)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    VIDENCTEST_NODE* pTmp = NULL;
    VIDENCTEST_NODE* pNewNode = NULL;

    pNewNode = (VIDENCTEST_NODE*)malloc(sizeof(VIDENCTEST_NODE)); /* need to malloc!!! */
    if (pNewNode == NULL) {
        VIDENCTEST_DPRINT("malloc() error.\n");
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    memset(pNewNode, 0x0, sizeof(VIDENCTEST_NODE));
    pNewNode->pData = pData;
    pNewNode->pNext = NULL;
    VIDENCTEST_DPRINT("Add MemoryNode[%p] -> [%p]\n", pNewNode, pNewNode->pData);

    pTmp = pListHead;

    while (pTmp->pNext != NULL) {
        pTmp = pTmp->pNext;
    }
    pTmp->pNext = pNewNode;

EXIT:
    return eError;
}

/*-----------------------------------------------------------------------------*/
/**
  * ListRemove()
  *
  * Called inside VIDENC_FREE Macro remove  node from Component Memory List and free the memory pointed by the node.
  *
  * @param pListHead VIDENCTEST_NODE Points List Header of the Memory List.
  *                pData OMX_PTR points to the new allocated data.
  * @retval OMX_ErrorNone
  *
  *
  **/
/*-----------------------------------------------------------------------------*/

OMX_ERRORTYPE VIDENCTEST_ListRemove(struct VIDENCTEST_NODE* pListHead, OMX_PTR pData)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    VIDENCTEST_NODE* pNode = NULL;
    VIDENCTEST_NODE* pTmp = NULL;

    pNode = pListHead;

    while (pNode->pNext != NULL) {
        if (pNode->pNext->pData == pData) {
            pTmp = pNode->pNext;
            pNode->pNext = pTmp->pNext;
            VIDENCTEST_DPRINT("Remove MemoryNode[%p] -> [%p]\n", pTmp, pTmp->pData);
            free(pTmp->pData);
            free(pTmp);
            pTmp = NULL;
            break;
            /* VIDENC_ListPrint2(pListHead); */
        }
        pNode = pNode->pNext;
    }
    return eError;
}

/*-----------------------------------------------------------------------------*/
/**
  * ListDestroy()
  *
  * Called inside OMX_ComponentDeInit()  Remove all nodes and free all the memory in the Component Memory List.
  *
  * @param pListHead VIDENCTEST_NODE Points List Header of the Memory List.
  *
  * @retval OMX_ErrorNone
  *
  *
  **/
/*-----------------------------------------------------------------------------*/

OMX_ERRORTYPE VIDENCTEST_ListDestroy(struct VIDENCTEST_NODE* pListHead)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    VIDENCTEST_NODE* pTmp = NULL;
    VIDENCTEST_NODE* pNode = NULL;
    pNode = pListHead;

    while (pNode->pNext != NULL) {
        pTmp = pNode->pNext;
        if (pTmp->pData != NULL) {
            VIDENCTEST_FREE(pTmp->pData, pListHead);
        }
        pNode->pNext = pTmp->pNext;
        VIDENCTEST_FREE(pTmp, pListHead);
    }

    VIDENCTEST_DPRINT("Destroy MemoryListHeader[%p]\n", pListHead);
    free(pListHead);
    return eError;
}

/*-----------------------------------------------------------------------------*/
/**
  * EventHandler(  )
  *
  * Called inside OMX_ComponentDeInit()  Remove all nodes and free all the memory in the Component Memory List.
  *
  * @param pListHead VIDENCTEST_NODE Points List Header of the Memory List.
  *
  * @retval OMX_ErrorNone
  *
  *
  **/
/*-----------------------------------------------------------------------------*/
void VIDENCTEST_EventHandler(OMX_HANDLETYPE hComponent, MYDATATYPE* pAppData, OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData)
{
    EVENT_PRIVATE* pEventPrivate = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_HANDLETYPE pHandle;
    VIDENCTEST_NODE* pListHead;

    pHandle= pAppData->pHandle;
    pListHead = pAppData->pMemoryListHead;
    VIDENCTEST_UNUSED_ARG(hComponent);
    VIDENCTEST_MALLOC(pEventPrivate, sizeof(EVENT_PRIVATE), EVENT_PRIVATE, pListHead);

    /* TODO: Optimize using a linked list */
    pAppData->pEventArray[pAppData->nEventCount] = pEventPrivate;
    pAppData->nEventCount++;
    if(eError != OMX_ErrorNone)
        VIDENCTEST_DPRINT("Erro in function VIDENCTEST_EventHandler\n");
    else{
    pEventPrivate->pAppData = pAppData;
    pEventPrivate->eEvent = eEvent;
    pEventPrivate->nData1 = nData1;
    pEventPrivate->nData2 = nData2;
    pEventPrivate->pEventData = pEventData;

    write(pAppData->eventPipe[1], &pEventPrivate, sizeof(pEventPrivate));
    }
}

/*-----------------------------------------------------------------------------*/
/**
  * FillBufferDone()
  *
  * Callback function when output buffer is done fill with h.264/mpeg4/h.263 data
  *
  * @param
  *
  * @retval OMX_ErrorNone
  *
  *
  **/
/*-----------------------------------------------------------------------------*/
void VIDENCTEST_FillBufferDone(OMX_HANDLETYPE hComponent, MYDATATYPE* pAppData, OMX_BUFFERHEADERTYPE* pBuffer)
{
    VIDENCTEST_UNUSED_ARG(hComponent);
    VIDENCTEST_DPRINT("FillBufferDone :: %p \n", pBuffer);
    write(pAppData->OpBuf_Pipe[1], &pBuffer, sizeof(pBuffer));
}

/*-----------------------------------------------------------------------------*/
/**
  * EmptyBufferDone()
  *
  * Callback function when and input buffer has been encoded. Returns an Empty Buffer.
  *
  * @param
  *
  * @retval OMX_ErrorNone
  *
  *
  **/
/*-----------------------------------------------------------------------------*/
void VIDENCTEST_EmptyBufferDone(OMX_HANDLETYPE hComponent, MYDATATYPE* pAppData, OMX_BUFFERHEADERTYPE* pBuffer)
{
    VIDENCTEST_UNUSED_ARG(hComponent);
    VIDENCTEST_DPRINT("EmptyBufferDone :: %p \n", pBuffer);
    write(pAppData->IpBuf_Pipe[1], &pBuffer, sizeof(pBuffer));
}

/*-----------------------------------------------------------------------------*/
/**
  * fill_data()
  *
  * Fill buffer with data from the input file (YUV data 420/422 little endian/ 422 big endian).
  *
  * @param
  *
  * @retval OMX_ErrorNone
  *
  *
  **/
/*-----------------------------------------------------------------------------*/
int VIDENCTEST_fill_data(OMX_BUFFERHEADERTYPE *pBuf, FILE *fIn, int buffersize)
{
    int nRead = -1;
    int nError = 0;

    /* Input video frame format: YUV422 interleaved (1) or YUV420 (0) */
    nRead = fread(pBuf->pBuffer,1, buffersize, fIn);
    if (nRead == -1) {
        VIDENCTEST_DPRINT("Error Reading File!\n");
    }
    nError = ferror(fIn);
    if (nError != 0) {
        VIDENCTEST_DPRINT("ERROR: reading file\n");
    }
    nError = feof(fIn);
    if (nError != 0 ) {
        VIDENCTEST_DPRINT("EOS reached...\n");
    }

    pBuf->nFilledLen = nRead;
    if (feof(fIn)) {
        VIDENCTEST_DPRINT("Setting OMX_BUFFERFLAGE_EOS -> %p\n", pBuf);
        pBuf->nFlags = OMX_BUFFERFLAG_EOS;
    }
    return nRead;
}

/*-----------------------------------------------------------------------------*/
/**
  * HandleError()
  *
  * Function call when an error ocurrs. The function un-load and free all the resource
  * depending the eError recieved.
  * @param pHandle Handle of MYDATATYPE structure
  * @param eError Error ocurred.
 *
  * @retval OMX_ErrorNone
  *
  *
  **/
/*-----------------------------------------------------------------------------*/

OMX_ERRORTYPE VIDENCTEST_HandleError(MYDATATYPE* pAppData, OMX_ERRORTYPE eError)
{
    OMX_ERRORTYPE eErrorHandleError = OMX_ErrorNone;
    OMX_HANDLETYPE pHandle = pAppData->pHandle;
    OMX_U32 nCounter;
    VIDENCTEST_NODE* pListHead;
    OMX_ERRORTYPE eErr = OMX_ErrorNone;

    VIDENCTEST_DPRINT ("Enters to HandleError\n");
    pListHead = pAppData->pMemoryListHead;

    switch (pAppData->eCurrentState) {
        case VIDENCTEST_StateReady:
        case VIDENCTEST_StateStarting:
        case VIDENCTEST_StateEncoding:
        case VIDENCTEST_StateStopping:
        case VIDENCTEST_StateConfirm:
        case VIDENCTEST_StatePause:
        case VIDENCTEST_StateStop:
        case VIDENCTEST_StateWaitEvent:
         VIDENCTEST_DPRINT ("Free buffers\n");
            if (pAppData->bAllocateOBuf == OMX_TRUE) {
                for (nCounter = 0; nCounter < NUM_OF_OUT_BUFFERS; nCounter++) {
                    pAppData->pOBuffer[nCounter] -= 128;
                    pAppData->pOBuffer[nCounter] = (unsigned char*)pAppData->pOBuffer[nCounter];
                    VIDENCTEST_FREE(pAppData->pOBuffer[nCounter], pListHead);
                    pAppData->pOBuffer[nCounter] = NULL;
                }
            }
            for (nCounter = 0; nCounter < NUM_OF_OUT_BUFFERS; nCounter++) {
                eError = OMX_FreeBuffer(pHandle, pAppData->pOutPortDef->nPortIndex, pAppData->pOutBuff[nCounter]);
            }
            if (pAppData->bAllocateIBuf == OMX_TRUE) {
                for (nCounter = 0; nCounter < NUM_OF_IN_BUFFERS; nCounter++) {
                    pAppData->pIBuffer[nCounter] -= 128;
                    pAppData->pIBuffer[nCounter] = (unsigned char*)pAppData->pIBuffer[nCounter];
                    VIDENCTEST_FREE(pAppData->pIBuffer[nCounter], pListHead);
                    pAppData->pIBuffer[nCounter] = NULL;
                }
            }
            for (nCounter = 0; nCounter < NUM_OF_IN_BUFFERS; nCounter++) {
                eError = OMX_FreeBuffer(pHandle, pAppData->pInPortDef->nPortIndex, pAppData->pInBuff[nCounter]);
            }
        case VIDENCTEST_StateLoaded:
            VIDENCTEST_DPRINT ("DeInit Component\n");
            eErrorHandleError = TIOMX_FreeHandle(pHandle);
            VIDENCTEST_CHECK_EXIT(eErrorHandleError, "Error at TIOMX_FreeHandle function");
            eErrorHandleError = TIOMX_Deinit();
            VIDENCTEST_CHECK_EXIT(eErrorHandleError, "Error at TIOMX_Deinit function");
            fclose(pAppData->fIn);
            fclose(pAppData->fOut);
            if(pAppData->NalFormat == VIDENC_TEST_NAL_FRAME || pAppData->NalFormat == VIDENC_TEST_NAL_SLICE) {
                fclose(pAppData->fNalnd);
            }

            eErr = close(pAppData->IpBuf_Pipe[0]);
            if (0 != eErr && OMX_ErrorNone == eError) {
                eError = OMX_ErrorHardware;
                VIDENCTEST_DPRINT ("Error while closing data pipe\n");
            }

            eErr = close(pAppData->OpBuf_Pipe[0]);
            if (0 != eErr && OMX_ErrorNone == eError) {
                eError = OMX_ErrorHardware;
                VIDENCTEST_DPRINT ("Error while closing data pipe\n");
            }

            eErr = close(pAppData->eventPipe[0]);
            if (0 != eErr && OMX_ErrorNone == eError) {
                eError = OMX_ErrorHardware;
                VIDENCTEST_DPRINT ("Error while closing data pipe\n");
            }

            eErr = close(pAppData->IpBuf_Pipe[1]);
            if (0 != eErr && OMX_ErrorNone == eError) {
                eError = OMX_ErrorHardware;
                VIDENCTEST_DPRINT ("Error while closing data pipe\n");
            }

            eErr = close(pAppData->OpBuf_Pipe[1]);
            if (0 != eErr && OMX_ErrorNone == eError) {
                eError = OMX_ErrorHardware;
                VIDENCTEST_DPRINT ("Error while closing data pipe\n");
            }

            eErr = close(pAppData->eventPipe[1]);
            if (0 != eErr && OMX_ErrorNone == eError) {
                eError = OMX_ErrorHardware;
                VIDENCTEST_DPRINT ("Error while closing data pipe\n");
            }
            pAppData->fIn = NULL;
            pAppData->fOut = NULL;
            pAppData->fNalnd = NULL;
        case VIDENCTEST_StateUnLoad:
            VIDENCTEST_DPRINT ("Free Resources\n");
            VIDENCTEST_ListDestroy(pListHead);
        default:
            ;
    }

#ifdef DSP_MMU_FAULT_HANDLING
    if(bInvalid_state == OMX_TRUE)
    {
        LoadBaseImage();
    }
#endif

EXIT:
    return eErrorHandleError;
}

/*-----------------------------------------------------------------------------*/
/**
  * SetH264Parameter()
  *
  * Initialize H264 Parameters.
  *
  * @param pAppData
  *
  * @retval OMX_ErrorNone
  *
  *
  **/
/*-----------------------------------------------------------------------------*/
OMX_ERRORTYPE VIDENCTEST_SetH264Parameter(MYDATATYPE* pAppData)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_HANDLETYPE pHandle = pAppData->pHandle;

    /* Set the component's OMX_PARAM_PORTDEFINITIONTYPE structure (input) */
    /**********************************************************************/
    pAppData->pInPortDef->nBufferCountActual = NUM_OF_IN_BUFFERS;
    pAppData->pInPortDef->nBufferCountMin = 1;
    pAppData->pInPortDef->bEnabled = OMX_TRUE;
    pAppData->pInPortDef->bPopulated = OMX_FALSE;
    pAppData->pInPortDef->eDomain = OMX_PortDomainVideo;

    /* OMX_VIDEO_PORTDEFINITION values for input port */
    pAppData->pInPortDef->format.video.cMIMEType = "yuv";
    pAppData->pInPortDef->format.video.pNativeRender = NULL;
    pAppData->pInPortDef->format.video.nStride = -1;
    pAppData->pInPortDef->format.video.nSliceHeight = -1;
    pAppData->pInPortDef->format.video.xFramerate = fToQ16(pAppData->nFramerate);
    pAppData->pInPortDef->format.video.bFlagErrorConcealment = OMX_FALSE;
    pAppData->pInPortDef->format.video.eColorFormat = pAppData->eColorFormat;
    pAppData->pInPortDef->format.video.nFrameWidth = pAppData->nWidth;
    pAppData->pInPortDef->format.video.nFrameHeight = pAppData->nHeight;



    eError = OMX_SetParameter(pHandle, OMX_IndexParamPortDefinition, pAppData->pInPortDef);
    VIDENCTEST_CHECK_EXIT(eError, "Error at SetParameter");

    /* To get nBufferSize */
    eError = OMX_GetParameter(pHandle, OMX_IndexParamPortDefinition, pAppData->pInPortDef);
    VIDENCTEST_CHECK_EXIT(eError, "Error at GetParameter");

    pAppData->nSizeIn = pAppData->pInPortDef->nBufferSize;

    /* Set the component's OMX_VIDEO_PARAM_AVCTYPE structure (output) */
    /*************************************************************/
    memset(pAppData->pH264, 0x0, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
    pAppData->pH264->nSize = sizeof(OMX_VIDEO_PARAM_AVCTYPE);
    pAppData->pH264->nVersion.s.nVersionMajor = 0x1;
    pAppData->pH264->nVersion.s.nVersionMinor = 0x0;
    pAppData->pH264->nVersion.s.nRevision = 0x0;
    pAppData->pH264->nVersion.s.nStep = 0x0;
    pAppData->pH264->nPortIndex = VIDENC_OUTPUT_PORT;
    pAppData->pH264->nSliceHeaderSpacing = 0;
    pAppData->pH264->nPFrames = -1;
    pAppData->pH264->nBFrames = -1;
    pAppData->pH264->bUseHadamard = 0;
    pAppData->pH264->nRefFrames = -1;
    pAppData->pH264->nRefIdx10ActiveMinus1 = -1;
    pAppData->pH264->nRefIdx11ActiveMinus1 = -1;
    pAppData->pH264->bEnableUEP = OMX_FALSE;
    pAppData->pH264->bEnableFMO = OMX_FALSE;
    pAppData->pH264->bEnableASO = OMX_FALSE;
    pAppData->pH264->bEnableRS = OMX_FALSE;
    pAppData->pH264->eProfile = OMX_VIDEO_AVCProfileBaseline;
    pAppData->pH264->nAllowedPictureTypes = -1;
    pAppData->pH264->bFrameMBsOnly = OMX_FALSE;
    pAppData->pH264->bMBAFF = OMX_FALSE;
    pAppData->pH264->bEntropyCodingCABAC = OMX_FALSE;
    pAppData->pH264->bWeightedPPrediction = OMX_FALSE;
    pAppData->pH264->nWeightedBipredicitonMode = -1;
    pAppData->pH264->bconstIpred = OMX_FALSE;
    pAppData->pH264->bDirect8x8Inference = OMX_FALSE;
    pAppData->pH264->bDirectSpatialTemporal = OMX_FALSE;
    pAppData->pH264->nCabacInitIdc = -1;
    pAppData->pH264->eLoopFilterMode = OMX_VIDEO_AVCLoopFilterDisable;
    pAppData->pH264->eLevel = pAppData->eLevelH264;

    eError = OMX_SetParameter (pHandle, OMX_IndexParamVideoAvc, pAppData->pH264);
    VIDENCTEST_CHECK_EXIT(eError, "Error at SetParameter");

    /* Set the component's OMX_PARAM_PORTDEFINITIONTYPE structure (output) */
    /***********************************************************************/
    /*memset(pAppData->pOutPortDef, 0x1, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
    pAppData->pOutPortDef->nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
    pAppData->pOutPortDef->nVersion.s.nVersionMajor = 0x1;
    pAppData->pOutPortDef->nVersion.s.nVersionMinor = 0x0;
    pAppData->pOutPortDef->nVersion.s.nRevision = 0x0;
    pAppData->pOutPortDef->nVersion.s.nStep = 0x0;
    pAppData->pOutPortDef->nPortIndex = VIDENC_OUTPUT_PORT;
    pAppData->pOutPortDef->eDir = OMX_DirOutput;*/
    pAppData->pOutPortDef->nBufferCountActual = NUM_OF_OUT_BUFFERS;
    pAppData->pOutPortDef->nBufferCountMin = 1;
    /*pAppData->pOutPortDef->nBufferSize = pAppData->nOutBuffSize;*/
    pAppData->pOutPortDef->bEnabled = OMX_TRUE;
    pAppData->pOutPortDef->bPopulated = OMX_FALSE;
    pAppData->pOutPortDef->eDomain = OMX_PortDomainVideo;

    /* OMX_VIDEO_PORTDEFINITION values for input port */
    pAppData->pOutPortDef->format.video.cMIMEType = "264";
    pAppData->pOutPortDef->format.video.pNativeRender = NULL;
    pAppData->pOutPortDef->format.video.nFrameWidth = pAppData->nWidth;
    pAppData->pOutPortDef->format.video.nFrameHeight = pAppData->nHeight;
    pAppData->pOutPortDef->format.video.nStride = 0;
    pAppData->pOutPortDef->format.video.nSliceHeight = 0;
    pAppData->pOutPortDef->format.video.nBitrate = pAppData->nBitrate;
    pAppData->pOutPortDef->format.video.xFramerate = 0;
    pAppData->pOutPortDef->format.video.bFlagErrorConcealment = OMX_FALSE;
    pAppData->pOutPortDef->format.video.eCompressionFormat = pAppData->eCompressionFormat;

    eError = OMX_SetParameter (pHandle, OMX_IndexParamPortDefinition, pAppData->pOutPortDef);
    VIDENCTEST_CHECK_EXIT(eError, "Error at SetParameter");

    /* Retreive nBufferSize */
    eError = OMX_GetParameter (pHandle, OMX_IndexParamPortDefinition, pAppData->pOutPortDef);
    VIDENCTEST_CHECK_EXIT(eError, "Error at GetParameter");

    pAppData->nSizeOut = pAppData->pOutPortDef->nBufferSize;

    /* Set the component's OMX_VIDEO_PARAM_BITRATETYPE structure (output) */
    /*************************************************************/
    pAppData->pVidParamBitrate->nSize = sizeof(OMX_VIDEO_PARAM_BITRATETYPE);
    pAppData->pVidParamBitrate->nVersion.s.nVersionMajor = 0x1;
    pAppData->pVidParamBitrate->nVersion.s.nVersionMinor = 0x0;
    pAppData->pVidParamBitrate->nVersion.s.nRevision = 0x0;
    pAppData->pVidParamBitrate->nVersion.s.nStep = 0x0;
    pAppData->pVidParamBitrate->nPortIndex = VIDENC_OUTPUT_PORT;
    pAppData->pVidParamBitrate->eControlRate = pAppData->eControlRate;
    pAppData->pVidParamBitrate->nTargetBitrate = pAppData->pOutPortDef->format.video.nBitrate;

    /* TODO: need to convert what value DSP is expecting to equivalent OMX value */
    eError = OMX_SetParameter (pHandle, OMX_IndexParamVideoBitrate, pAppData->pVidParamBitrate);
    VIDENCTEST_CHECK_EXIT(eError, "Error at SetParameter");

    /* Set the component's OMX_VIDEO_PARAM_QUANTIZATIONTYPE structure (output) */
    /*************************************************************/
    pAppData->pQuantization->nSize = sizeof(OMX_VIDEO_PARAM_QUANTIZATIONTYPE);
    pAppData->pQuantization->nVersion.s.nVersionMajor = 0x1;
    pAppData->pQuantization->nVersion.s.nVersionMinor = 0x0;
    pAppData->pQuantization->nVersion.s.nRevision = 0x0;
    pAppData->pQuantization->nVersion.s.nStep = 0x0;
    pAppData->pQuantization->nPortIndex = VIDENC_OUTPUT_PORT;
    pAppData->pQuantization->nQpI = pAppData->nQpI;

    eError = OMX_SetParameter (pHandle, OMX_IndexParamVideoQuantization, pAppData->pQuantization);
    VIDENCTEST_CHECK_EXIT(eError, "Error at SetParameter");

    /* SR12020: Set H.264 encode Deblocking Filter using the custom OMX index */
    eError = OMX_GetExtensionIndex(pHandle,"OMX.TI.VideoEncode.Param.DeblockFilter", (OMX_INDEXTYPE*)(&(pAppData->nVideoEncodeCustomParamIndex)));
    VIDENCTEST_CHECK_EXIT(eError, "Error in OMX_GetExtensionIndex function");

    eError = OMX_SetParameter(pHandle, pAppData->nVideoEncodeCustomParamIndex, &(pAppData->bDeblockFilter));
    VIDENCTEST_CHECK_EXIT(eError, "Error at SetParameter");

    if(pAppData->nEncodingPreset!=VIDENCTEST_USE_DEFAULT_VALUE_UI && pAppData->nEncodingPreset<=4){
        printf("EncodingPreset %d selected\n", pAppData->nEncodingPreset);
        eError = OMX_GetExtensionIndex(pHandle,"OMX.TI.VideoEncode.Config.EncodingPreset", (OMX_INDEXTYPE*)(&(pAppData->nVideoEncodeCustomParamIndex)));
        VIDENCTEST_CHECK_EXIT(eError, "OMX_GetExtensionIndex function");
        eError = OMX_SetParameter(pHandle, pAppData->nVideoEncodeCustomParamIndex, &(pAppData->nEncodingPreset));
        VIDENCTEST_CHECK_EXIT(eError, "Error at SetConfig for nEncodingPreset");
    }

    if(pAppData->nUnrestrictedMV != (OMX_U8)VIDENCTEST_USE_DEFAULT_VALUE_UI){
        printf("nUnrestrictedMV %d selected\n", pAppData->nUnrestrictedMV);
        eError = OMX_GetExtensionIndex(pHandle,"OMX.TI.VideoEncode.Config.UnrestrictedMV", (OMX_INDEXTYPE*)(&(pAppData->nVideoEncodeCustomParamIndex)));
        VIDENCTEST_CHECK_EXIT(eError, "OMX_GetExtensionIndex function");
        eError = OMX_SetParameter(pHandle, pAppData->nVideoEncodeCustomParamIndex, &(pAppData->nUnrestrictedMV));
        VIDENCTEST_CHECK_EXIT(eError, "Error at SetConfig for nUnrestrictedMV");
    }

    eError = OMX_GetExtensionIndex(pHandle,"OMX.TI.VideoEncode.Config.NALFormat", (OMX_INDEXTYPE*)(&(pAppData->nVideoEncodeCustomParamIndex)));
    VIDENCTEST_CHECK_EXIT(eError, "OMX_GetExtensionIndex function");
    eError = OMX_SetParameter(pHandle, pAppData->nVideoEncodeCustomParamIndex, &(pAppData->NalFormat));
    VIDENCTEST_CHECK_EXIT(eError, "Error at SetConfig for NalFormat");

EXIT:
    return eError;
}

/*-----------------------------------------------------------------------------*/
/**
  * SetMpeg4Parameter()
  *
  * Intialize Mpeg4 structures.
  *
  * @param pAppData
  *
  * @retval OMX_ErrorNone
  *
  *
  **/
/*-----------------------------------------------------------------------------*/
OMX_ERRORTYPE VIDENCTEST_SetMpeg4Parameter(MYDATATYPE* pAppData)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_HANDLETYPE pHandle = pAppData->pHandle;

    /* Set the component's OMX_PARAM_PORTDEFINITIONTYPE structure (input) */
    /**********************************************************************/
    pAppData->pInPortDef->nBufferCountActual = NUM_OF_IN_BUFFERS;
    pAppData->pInPortDef->nBufferCountMin = 2;
    pAppData->pInPortDef->bEnabled = OMX_TRUE;
    pAppData->pInPortDef->bPopulated = OMX_FALSE;
    pAppData->pInPortDef->eDomain = OMX_PortDomainVideo;

    /* OMX_VIDEO_PORTDEFINITION values for input port */
    pAppData->pInPortDef->format.video.cMIMEType = "yuv";
    pAppData->pInPortDef->format.video.pNativeRender = NULL;
    pAppData->pInPortDef->format.video.nStride = -1;
    pAppData->pInPortDef->format.video.nSliceHeight = -1;
    pAppData->pInPortDef->format.video.xFramerate = fToQ16(pAppData->nFramerate);
    pAppData->pInPortDef->format.video.bFlagErrorConcealment = OMX_FALSE;
    pAppData->pInPortDef->format.video.eColorFormat = pAppData->eColorFormat;
    pAppData->pInPortDef->format.video.nFrameWidth = pAppData->nWidth;
    pAppData->pInPortDef->format.video.nFrameHeight = pAppData->nHeight;

    eError = OMX_SetParameter (pHandle, OMX_IndexParamPortDefinition, pAppData->pInPortDef);
    VIDENCTEST_CHECK_EXIT(eError, "Error at SetParameter");

    /* To get nBufferSize */
    eError = OMX_GetParameter(pHandle, OMX_IndexParamPortDefinition, pAppData->pInPortDef);
    VIDENCTEST_CHECK_EXIT(eError, "Error at GetParameter");

    pAppData->nSizeIn = pAppData->pInPortDef->nBufferSize;

    /* Set the component's OMX_PARAM_PORTDEFINITIONTYPE structure (output) */
    /*************************************************************/
    pAppData->pOutPortDef->nBufferCountActual = NUM_OF_OUT_BUFFERS;
    pAppData->pOutPortDef->nBufferCountMin = 1;
    pAppData->pOutPortDef->bEnabled = OMX_TRUE;
    pAppData->pOutPortDef->bPopulated = OMX_FALSE;
    pAppData->pOutPortDef->eDomain = OMX_PortDomainVideo;

    /* OMX_VIDEO_PORTDEFINITION values for input port */
    pAppData->pOutPortDef->format.video.cMIMEType = "mp4";
    pAppData->pOutPortDef->format.video.pNativeRender = NULL;
    pAppData->pOutPortDef->format.video.nFrameWidth = pAppData->nWidth;
    pAppData->pOutPortDef->format.video.nFrameHeight = pAppData->nHeight;
    pAppData->pOutPortDef->format.video.nStride = 0;
    pAppData->pOutPortDef->format.video.nSliceHeight = 0;
    pAppData->pOutPortDef->format.video.nBitrate = pAppData->nBitrate;
    pAppData->pOutPortDef->format.video.xFramerate = 0;
    pAppData->pOutPortDef->format.video.bFlagErrorConcealment = OMX_FALSE;
    pAppData->pOutPortDef->format.video.eCompressionFormat = pAppData->eCompressionFormat;

    eError = OMX_SetParameter (pHandle, OMX_IndexParamPortDefinition, pAppData->pOutPortDef);
    VIDENCTEST_CHECK_EXIT(eError, "Error at SetParameter");

    /* Retreive nBufferSize */
    eError = OMX_GetParameter (pHandle, OMX_IndexParamPortDefinition, pAppData->pOutPortDef);
    VIDENCTEST_CHECK_EXIT(eError, "Error at GetParameter");

    pAppData->nSizeOut = pAppData->pOutPortDef->nBufferSize;

    /* Set the component's OMX_VIDEO_PARAM_BITRATETYPE structure (output) */
    /*************************************************************/
    pAppData->pVidParamBitrate->nSize = sizeof(OMX_VIDEO_PARAM_BITRATETYPE);
    pAppData->pVidParamBitrate->nVersion.s.nVersionMajor = 0x1;
    pAppData->pVidParamBitrate->nVersion.s.nVersionMinor = 0x0;
    pAppData->pVidParamBitrate->nVersion.s.nRevision = 0x0;
    pAppData->pVidParamBitrate->nVersion.s.nStep = 0x0;
    pAppData->pVidParamBitrate->nPortIndex = VIDENC_OUTPUT_PORT;
    pAppData->pVidParamBitrate->eControlRate = pAppData->eControlRate;
    pAppData->pVidParamBitrate->nTargetBitrate = pAppData->pOutPortDef->format.video.nBitrate;

    /* TODO: need to convert what value DSP is expecting to equivalent OMX value */

    eError = OMX_SetParameter (pHandle, OMX_IndexParamVideoBitrate, pAppData->pVidParamBitrate);
    VIDENCTEST_CHECK_EXIT(eError, "Error at SetParameter");

    /* Set the component's OMX_VIDEO_PARAM_H263TYPE structure (output) */
    /*************************************************************/
    if (pAppData->eCompressionFormat == OMX_VIDEO_CodingH263) {
        pAppData->pH263->nSize = sizeof(OMX_VIDEO_PARAM_H263TYPE);
        pAppData->pH263->nVersion.s.nVersionMajor = 0x1;
        pAppData->pH263->nVersion.s.nVersionMinor = 0x0;
        pAppData->pH263->nVersion.s.nRevision = 0x0;
        pAppData->pH263->nVersion.s.nStep = 0x0;
        pAppData->pH263->nPortIndex = VIDENC_OUTPUT_PORT;
        pAppData->pH263->eLevel = pAppData->eLevelH63;
        pAppData->pH263->nGOBHeaderInterval = pAppData->nGOBHeaderInterval;

        eError = OMX_SetParameter (pHandle, OMX_IndexParamVideoH263, pAppData->pH263);
        VIDENCTEST_CHECK_EXIT(eError, "Error at SetParameter");
    }

    /* Set the component's OMX_VIDEO_PARAM_QUANTIZATIONTYPE structure (output) */
    /*************************************************************/
    pAppData->pQuantization->nSize = sizeof(OMX_VIDEO_PARAM_QUANTIZATIONTYPE);
    pAppData->pQuantization->nVersion.s.nVersionMajor = 0x1;
    pAppData->pQuantization->nVersion.s.nVersionMinor = 0x0;
    pAppData->pQuantization->nVersion.s.nRevision = 0x0;
    pAppData->pQuantization->nVersion.s.nStep = 0x0;
    pAppData->pQuantization->nPortIndex = VIDENC_OUTPUT_PORT;
    pAppData->pQuantization->nQpI = pAppData->nQpI;

    eError = OMX_SetParameter (pHandle, OMX_IndexParamVideoQuantization, pAppData->pQuantization);
    VIDENCTEST_CHECK_EXIT(eError, "Error at SetParameter");

    /* Set the component's OMX_VIDEO_PARAM_MPEG4TYPE structure (output) */
    /*************************************************************/
    if (pAppData->eCompressionFormat == OMX_VIDEO_CodingMPEG4) {
        pAppData->pMpeg4->nSize = sizeof(OMX_VIDEO_PARAM_MPEG4TYPE);
        pAppData->pMpeg4->nVersion.s.nVersionMajor = 0x1;
        pAppData->pMpeg4->nVersion.s.nVersionMinor = 0x0;
        pAppData->pMpeg4->nVersion.s.nRevision = 0x0;
        pAppData->pMpeg4->nVersion.s.nStep = 0x0;
        pAppData->pMpeg4->nPortIndex = VIDENC_OUTPUT_PORT;
        pAppData->pMpeg4->eLevel = pAppData->eLevelMpeg4;

        eError = OMX_SetParameter (pHandle, OMX_IndexParamVideoMpeg4, pAppData->pMpeg4);
        VIDENCTEST_CHECK_EXIT(eError, "Error at SetParameter");
    }

    /* SR10527: Set MPEG4/H263 encode VBV Size using the custom OMX index */
    eError = OMX_GetExtensionIndex(pHandle,"OMX.TI.VideoEncode.Param.VBVSize", (OMX_INDEXTYPE*)(&(pAppData->nVideoEncodeCustomParamIndex)));
    VIDENCTEST_CHECK_EXIT(eError, "OMX_GetExtensionIndex function");

    eError = OMX_SetParameter(pHandle, pAppData->nVideoEncodeCustomParamIndex, &(pAppData->nVBVSize));
    VIDENCTEST_CHECK_EXIT(eError, "Error at SetParameter");

    if(pAppData->nMIRRate != VIDENCTEST_USE_DEFAULT_VALUE){
        printf("MIRRate selected\n");
        eError = OMX_GetExtensionIndex(pHandle,"OMX.TI.VideoEncode.Config.MIRRate", (OMX_INDEXTYPE*)(&(pAppData->nVideoEncodeCustomParamIndex)));
        VIDENCTEST_CHECK_EXIT(eError, "OMX_GetExtensionIndex function");
        eError = OMX_SetConfig(pHandle, pAppData->nVideoEncodeCustomParamIndex, &(pAppData->nMIRRate));
        VIDENCTEST_CHECK_EXIT(eError, "Error at SetConfig for nMIRRate");
    }

    if(pAppData->nResynchMarkerSpacing != VIDENCTEST_USE_DEFAULT_VALUE){
        printf("ResynchMarkerSpacing selected\n");
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE ErrorCorrectionType;
        ErrorCorrectionType.nSize = sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE);
        ErrorCorrectionType.nVersion.s.nVersionMajor = 0x1;
        ErrorCorrectionType.nVersion.s.nVersionMinor = 0x0;
        ErrorCorrectionType.nVersion.s.nRevision     = 0x0;
        ErrorCorrectionType.nVersion.s.nStep         = 0x0;
        ErrorCorrectionType.nPortIndex= VIDENC_OUTPUT_PORT;
        ErrorCorrectionType.bEnableHEC= OMX_TRUE;
        ErrorCorrectionType.bEnableResync = OMX_TRUE;
        ErrorCorrectionType.bEnableDataPartitioning= OMX_FALSE;
        ErrorCorrectionType.bEnableRVLC= OMX_FALSE;
        ErrorCorrectionType.nResynchMarkerSpacing = pAppData->nResynchMarkerSpacing;

        eError = OMX_SetConfig(pHandle, OMX_IndexParamVideoErrorCorrection, &ErrorCorrectionType);
        VIDENCTEST_CHECK_EXIT(eError, "Error at SetConfig for ErrorCorrection");
    }
    if(pAppData->nUnrestrictedMV != (OMX_U8)VIDENCTEST_USE_DEFAULT_VALUE_UI){
        printf("nUnrestrictedMV %d selected\n", pAppData->nUnrestrictedMV);
        eError = OMX_GetExtensionIndex(pHandle,"OMX.TI.VideoEncode.Config.UnrestrictedMV", (OMX_INDEXTYPE*)(&(pAppData->nVideoEncodeCustomParamIndex)));
        VIDENCTEST_CHECK_EXIT(eError, "OMX_GetExtensionIndex function");
        eError = OMX_SetParameter(pHandle, pAppData->nVideoEncodeCustomParamIndex, &(pAppData->nUnrestrictedMV));
        VIDENCTEST_CHECK_EXIT(eError, "Error at SetConfig for nUnrestrictedMV");
    }
EXIT:
    return eError;
}

/*-----------------------------------------------------------------------------*/
/**
  * AllocateResources()
  *
  * Allocate necesary resources.
  *
  * @param pAppData
  *
  * @retval OMX_ErrorNone
  *
  *
  **/
/*-----------------------------------------------------------------------------*/
OMX_ERRORTYPE VIDENCTEST_AllocateResources(MYDATATYPE* pAppData)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    int retval = 0;
    VIDENCTEST_NODE* pListHead;

    pListHead = pAppData->pMemoryListHead;

    VIDENCTEST_MALLOC(pAppData->pCb, sizeof(OMX_CALLBACKTYPE), OMX_CALLBACKTYPE, pListHead);
    VIDENCTEST_MALLOC(pAppData->pInPortDef, sizeof(OMX_PARAM_PORTDEFINITIONTYPE), OMX_PARAM_PORTDEFINITIONTYPE, pListHead);
    VIDENCTEST_MALLOC(pAppData->pOutPortDef, sizeof(OMX_PARAM_PORTDEFINITIONTYPE), OMX_PARAM_PORTDEFINITIONTYPE, pListHead);

    if (pAppData->eCompressionFormat == OMX_VIDEO_CodingH263) {
        VIDENCTEST_MALLOC(pAppData->pVidParamBitrate, sizeof(OMX_VIDEO_PARAM_BITRATETYPE), OMX_VIDEO_PARAM_BITRATETYPE, pListHead);
        VIDENCTEST_MALLOC(pAppData->pH263, sizeof(OMX_VIDEO_PARAM_H263TYPE), OMX_VIDEO_PARAM_H263TYPE, pListHead);
    }
    else if (pAppData->eCompressionFormat == OMX_VIDEO_CodingMPEG4) {
        VIDENCTEST_MALLOC(pAppData->pVidParamBitrate, sizeof(OMX_VIDEO_PARAM_BITRATETYPE), OMX_VIDEO_PARAM_BITRATETYPE, pListHead);
        VIDENCTEST_MALLOC(pAppData->pMpeg4, sizeof(OMX_VIDEO_PARAM_MPEG4TYPE), OMX_VIDEO_PARAM_MPEG4TYPE, pListHead);
    }
    else if (pAppData->eCompressionFormat == OMX_VIDEO_CodingAVC) {
        VIDENCTEST_MALLOC(pAppData->pVidParamBitrate, sizeof(OMX_VIDEO_PARAM_BITRATETYPE), OMX_VIDEO_PARAM_BITRATETYPE, pListHead);
        VIDENCTEST_MALLOC(pAppData->pH264, sizeof(OMX_VIDEO_PARAM_AVCTYPE), OMX_VIDEO_PARAM_AVCTYPE, pListHead);
    }
    else {
        VIDENCTEST_DPRINT("Invalid compression format value.\n");
        eError = OMX_ErrorUnsupportedSetting;
        goto EXIT;
    }
    VIDENCTEST_MALLOC(pAppData->pQuantization, sizeof(OMX_VIDEO_PARAM_QUANTIZATIONTYPE), OMX_VIDEO_PARAM_QUANTIZATIONTYPE, pListHead);

    /* Create a pipe used to queue data from the callback. */
    retval = pipe(pAppData->IpBuf_Pipe);
    if (retval != 0) {
        VIDENCTEST_DPRINT("Error: Fill Data Pipe failed to open\n");
        goto EXIT;
    }
    retval = pipe(pAppData->OpBuf_Pipe);
    if (retval != 0) {
        VIDENCTEST_DPRINT("Error: Empty Data Pipe failed to open\n");
        goto EXIT;
    }
    retval = pipe(pAppData->eventPipe);
    if (retval != 0) {
        VIDENCTEST_DPRINT("Error: Empty Data Pipe failed to open\n");
        goto EXIT;
    }

EXIT:
    return eError;
}

/*-----------------------------------------------------------------------------*/
/**
  * AllocateBuffers()
  *
  * Allocate necesary resources.
  *
  * @param pAppData
  *
  * @retval OMX_ErrorNone
  *
  *
  **/
/*-----------------------------------------------------------------------------*/
OMX_ERRORTYPE VIDENCTEST_AllocateBuffers(MYDATATYPE* pAppData)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U8 nCounter = 0;
    VIDENCTEST_NODE* pListHead;

    pListHead = pAppData->pMemoryListHead;

    for (nCounter = 0; nCounter < NUM_OF_IN_BUFFERS; nCounter++) {
        if (pAppData->bAllocateIBuf == OMX_TRUE) {
            VIDENCTEST_MALLOC(pAppData->pIBuffer[nCounter], pAppData->nSizeIn + 256, OMX_U8, pListHead);
            pAppData->pIBuffer[nCounter] += 128;
            pAppData->pIBuffer[nCounter] = (unsigned char*)pAppData->pIBuffer[nCounter];
        }
    }
    for (nCounter = 0; nCounter < NUM_OF_OUT_BUFFERS; nCounter++) {
        if (pAppData->bAllocateOBuf == OMX_TRUE) {
            VIDENCTEST_MALLOC(pAppData->pOBuffer[nCounter], pAppData->nSizeOut + 256, OMX_U8, pListHead);
            pAppData->pOBuffer[nCounter] += 128;
            pAppData->pOBuffer[nCounter] = (unsigned char*)pAppData->pOBuffer[nCounter];
            }
    }

    return eError;
}

/*-----------------------------------------------------------------------------*/
/**
  * FreeResources()
  *
  * Free all allocated memory.
  *
  * @param pAppData
  *
  * @retval OMX_ErrorNone
  *
  *
  **/
/*-----------------------------------------------------------------------------*/
OMX_ERRORTYPE VIDENCTEST_FreeResources(MYDATATYPE* pAppData)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 i = 0;
    VIDENCTEST_NODE* pListHead;

    pListHead = pAppData->pMemoryListHead;

    VIDENCTEST_FREE(pAppData->pCb, pListHead);
    VIDENCTEST_FREE(pAppData->pInPortDef, pListHead);
    VIDENCTEST_FREE(pAppData->pOutPortDef, pListHead);

    if((pAppData->NalFormat == VIDENC_TEST_NAL_FRAME || pAppData->NalFormat == VIDENC_TEST_NAL_SLICE)
        && pAppData->szOutFileNal) {
        VIDENCTEST_FREE(pAppData->szOutFileNal, pListHead);
    }

    if (pAppData->pH264 != NULL) {
        VIDENCTEST_FREE(pAppData->pH264, pListHead);
    }

    VIDENCTEST_FREE(pAppData->pVidParamBitrate, pListHead);

    if (pAppData->pH263 != NULL) {
        VIDENCTEST_FREE(pAppData->pH263, pListHead);
    }

    VIDENCTEST_FREE(pAppData->pQuantization, pListHead);

    if (pAppData->pMpeg4 != NULL) {
        VIDENCTEST_FREE(pAppData->pMpeg4, pListHead);
    }

    for (i = 0; i < pAppData->nEventCount; i++) {
        VIDENCTEST_FREE(pAppData->pEventArray[i], pListHead);
    }

    return eError;
}

/*-----------------------------------------------------------------------------*/
/**
  * PrintCorrectArgs()
  *
  * Print the description of the input arguments. Also Prints a sample line.
  *
  * @param bPrintH264. IF set will print the H264 argument description.
  * @param bPrintMpeg4. If set will print the Mpeg4/H263  argument description.
  *
  * @retval OMX_ErrorNone
  *
  *
  **/
/*-----------------------------------------------------------------------------*/
OMX_ERRORTYPE VIDENCTEST_PrintCorrectArgs(OMX_BOOL bPrintH264, OMX_BOOL bPrintMpeg4)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    if (bPrintMpeg4 == OMX_TRUE) {
        printf("MPEG4/H.263 Encode Usage: ./VideoEncTest <arg01> <arg02> <arg03> ...");
        printf("\narg01 : <input.yuv>");
        printf("\narg02 : <output.mp4>");
        printf("\narg03 : <width>");
        printf("\narg04 : <height>");
        printf("\narg05 : <19:420/25:422-BE/26:422-LE>");
        printf("\narg06 : <bitrate>");
        printf("\narg07 : <framerate>");
        printf("\narg08 : <3:H263/4:MPEG4/7:H264>");
        printf("\narg09 : <rateCtrl/1:var/2:const>");
        printf("\narg10 : <GOB> --H.263 only--");
        printf("\narg11 : <QPI>");
        printf("\narg12 : <level>");
        printf("\narg13 : <allocateBufFlag>");
        printf("\narg14 : <VBVsize>");
        printf("\narg15 : <0:full-rec/1:partial-rec->stop/2:rec->pause->resume/3:rec->stop->restart>");
        printf("\narg16 : <stop/pause frame/iteration> -- use zero if arg15 = 0:full-rec <or> use a valid frame # if arg15 = 1, 2, 3 --\n");
        printf("\narg17 : <Number of Repetitions of test>");
        printf("\nSample OMX MPEG4 Encode test:\n");
        printf("/omx/VideoEncTest /omx/patterns/carphone_qcif_420p_short.yuv /omx/patterns/carphone_qcif_420p_short.mp4 176 144 19 64000 15 4 1 0 12 0 1 120 0 0 1\n");
        printf("\nSample OMX H.263 Encode test:\n");
        printf("/omx/VideoEncTest /omx/patterns/carphone_qcif_420p_short.yuv /omx/patterns/carphone_qcif_420p_short.263 176 144 19 64000 15 3 2 0 12 10 1 120 0 0 1\n\n");
    }
    if (bPrintH264 == OMX_TRUE) {
        printf("H.264 Encode Usage: ./VideoEncTest <arg01> <arg02> <arg03> ...");
        printf("\narg01 :: <input.yuv>");
        printf("\narg02 :: <output.H264>");
        printf("\narg03 :: <width>");
        printf("\narg04 :: <height>");
        printf("\narg05 :: <19:420/25:422-BE/26:422-LE>");
        printf("\narg06 :: <bitrate>");
        printf("\narg07 :: <framerate>");
        printf("\narg08 :: <3:H263/4:MPEG4/7:H264>");
        printf("\narg09 :: <level>");
        printf("\narg10 :: <out_buff_size>");
        printf("\narg11 :: <allocateBufFlag>");
        printf("\narg12 :: <deblockFilter>");
        printf("\narg13 :: <rateCtrl/0:none/1:var/2:const>");
        printf("\narg14 :: <QPI> --set rate control to zero--");
        printf("\narg15 :: <0:full-rec/1:partial-rec->stop/2:rec->pause->resume/3:rec->stop->restart>");
        printf("\narg16 :: <stop/pause frame/iteration> -- use zero if arg15 = 0:full-rec <or> use a valid frame # if arg15 = 1, 2, 3 --\n");
        printf("\narg17 :: <Number of Repetitions of test>");
        printf("\nSample OMX H.264 Encode test:\n");
        printf("/omx/VideoEncTest /omx/patterns/carphone_qcif_420p_short.yuv /omx/patterns/carphone_qcif_420p_short.264 176 144 19 64000 15 7 10 26250 1 1 1 0 0 0 1\n\n");
    }
    return eError;
}

/*-----------------------------------------------------------------------------*/
/**
  * Init()
  *
  *Initialize all the parameters for the VideoEncoder Structures
  *
  * @param  pAppData MYDATA handle
  *
  * @retval OMX_ErrorNone
  *
  *
  **/
/*-----------------------------------------------------------------------------*/
OMX_ERRORTYPE VIDENCTEST_PassToLoaded(MYDATATYPE* pAppData)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_HANDLETYPE pHandle;
    VIDENCTEST_NODE* pListHead;
    OMX_U32 i;
    OMX_CALLBACKTYPE sCb = {(void*)VIDENCTEST_EventHandler, (void*)VIDENCTEST_EmptyBufferDone, (void*)VIDENCTEST_FillBufferDone};

    pListHead = pAppData->pMemoryListHead;

    eError = VIDENCTEST_AllocateResources(pAppData);


    VIDENCTEST_CHECK_EXIT(eError, "Error at Allocation of Resources");

    pAppData->fdmax = maxint(pAppData->IpBuf_Pipe[0], pAppData->eventPipe[0]);

    pAppData->fdmax = maxint(pAppData->OpBuf_Pipe[0], pAppData->fdmax);



    /* Initialize OMX Core */

    eError = TIOMX_Init();
    VIDENCTEST_CHECK_EXIT(eError, "Error returned by TIOMX_Init()");

    *pAppData->pCb = sCb;
    /* Get VideoEncoder Component Handle */
    eError = TIOMX_GetHandle(&pHandle, StrVideoEncoder, pAppData, pAppData->pCb);
    VIDENCTEST_CHECK_EXIT(eError, "Error returned by TIOMX_Init()");
    if (pHandle == NULL) {
        VIDENCTEST_DPRINT("Error GetHandle() return Null Pointer\n");
        eError = OMX_ErrorUndefined;
        goto EXIT;
    }

    pAppData->pHandle = pHandle;

    /* Get starting port number and the number of ports */
    VIDENCTEST_MALLOC(pAppData->pVideoInit, sizeof(OMX_PORT_PARAM_TYPE), OMX_PORT_PARAM_TYPE, pListHead);

    eError = OMX_GetParameter(pHandle, OMX_IndexParamVideoInit, pAppData->pVideoInit);
    VIDENCTEST_CHECK_EXIT(eError, "Error returned from GetParameter()");

    pAppData->nStartPortNumber = pAppData->pVideoInit->nStartPortNumber;
    pAppData->nPorts           = pAppData->pVideoInit->nPorts;

    VIDENCTEST_FREE(pAppData->pVideoInit, pListHead);

    /* TODO: Optimize - We should use a linked list instead of an array */
    for (i = pAppData->nStartPortNumber; i < pAppData->nPorts; i++) {
        VIDENCTEST_MALLOC(pAppData->pPortDef[i], sizeof(OMX_PARAM_PORTDEFINITIONTYPE), OMX_PARAM_PORTDEFINITIONTYPE, pListHead);

        pAppData->pPortDef[i]->nPortIndex = i;
        eError = OMX_GetParameter(pHandle, OMX_IndexParamPortDefinition, pAppData->pPortDef[i]);
        VIDENCTEST_CHECK_EXIT(eError, "Error returned from GetParameter()");

        if (pAppData->pPortDef[i]->eDir == OMX_DirInput) {
            memcpy(pAppData->pInPortDef, pAppData->pPortDef[i], sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        }
        else {
            memcpy(pAppData->pOutPortDef, pAppData->pPortDef[i], sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        }
        VIDENCTEST_FREE(pAppData->pPortDef[i], pListHead);
    }

    switch (pAppData->eCompressionFormat) {
        case OMX_VIDEO_CodingH263:
            eError = VIDENCTEST_SetMpeg4Parameter(pAppData);
            VIDENCTEST_CHECK_EXIT(eError, "Error returned from SetMpeg4Parameter()");
            break;
        case OMX_VIDEO_CodingMPEG4:
            eError = VIDENCTEST_SetMpeg4Parameter(pAppData);
            VIDENCTEST_CHECK_EXIT(eError, "Error returned from SetMpeg4Parameter()");
            break;
        case OMX_VIDEO_CodingAVC:
            eError = VIDENCTEST_SetH264Parameter(pAppData);
            VIDENCTEST_CHECK_EXIT(eError, "Error returned from SetH264Parameter()");
            break;
        default:
            VIDENCTEST_DPRINT("Invalid compression format value.\n");
            eError = OMX_ErrorUnsupportedSetting;
            goto EXIT;
    }

    VIDENCTEST_AllocateBuffers(pAppData);
    VIDENCTEST_CHECK_EXIT(eError, "Error at Allocation of Resources");

    pAppData->eCurrentState = VIDENCTEST_StateLoaded;

 EXIT:
    return eError;

}

/*-----------------------------------------------------------------------------*/
/**
  * PassToReady()
  *
  *Pass the Component to Idle and allocate the buffers
  *
  * @param  pAppData MYDATA handle
  *
  * @retval OMX_ErrorNone
  *
  *
  **/
/*-----------------------------------------------------------------------------*/
OMX_ERRORTYPE VIDENCTEST_PassToReady(MYDATATYPE* pAppData)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_HANDLETYPE pHandle;
    OMX_U32 nCounter;

    pHandle = pAppData->pHandle;

    eError = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
    VIDENCTEST_CHECK_EXIT(eError, "Error from SendCommand-Idle(Init) State function");

    for (nCounter = 0; nCounter < NUM_OF_IN_BUFFERS; nCounter++) {
       if (pAppData->bAllocateIBuf == OMX_TRUE) {
            eError = OMX_UseBuffer(pHandle,
                                  &pAppData->pInBuff[nCounter],
                                  pAppData->pInPortDef->nPortIndex,
                                  pAppData,
                                  pAppData->pInPortDef->nBufferSize,
                                  pAppData->pIBuffer[nCounter]);
            VIDENCTEST_CHECK_EXIT(eError, "Error from OMX_UseBuffer function");
       }
       else {
            eError = OMX_AllocateBuffer(pHandle,
                                       &pAppData->pInBuff[nCounter],
                                       pAppData->pInPortDef->nPortIndex,
                                       pAppData,
                                       pAppData->pInPortDef->nBufferSize);
            VIDENCTEST_CHECK_EXIT(eError, "Error from OMX_AllocateBuffer State function");
       }
    }

    pAppData->nInBufferCount = NUM_OF_IN_BUFFERS;

    for (nCounter = 0; nCounter < NUM_OF_OUT_BUFFERS; nCounter++) {
        if (pAppData->bAllocateOBuf == OMX_TRUE) {
            eError = OMX_UseBuffer(pHandle,
                                   &pAppData->pOutBuff[nCounter],
                                   pAppData->pOutPortDef->nPortIndex,
                                   pAppData,
                                   pAppData->pOutPortDef->nBufferSize,
                                   pAppData->pOBuffer[nCounter]);
            VIDENCTEST_CHECK_EXIT(eError, "Error from OMX_UseBuffer function");
        }
        else {
            eError = OMX_AllocateBuffer(pHandle,
                                        &pAppData->pOutBuff[nCounter],
                                        pAppData->pOutPortDef->nPortIndex,
                                        pAppData,
                                        pAppData->pOutPortDef->nBufferSize);
            VIDENCTEST_CHECK_EXIT(eError, "Error from OMX_AllocateBuffer function");
        }
    }

    pAppData->nOutBufferCount = NUM_OF_OUT_BUFFERS;

    pAppData->bLastOutBuffer = 0;


EXIT:
    return eError;

}

/*-----------------------------------------------------------------------------*/
/**
  * PassToExecuting()
  *
  *Pass the component to executing  and fill the first 4 buffers.
  *
  * @param  pAppData MYDATA handle
  *
  * @retval OMX_ErrorNone
  *
  *
  **/
/*-----------------------------------------------------------------------------*/
OMX_ERRORTYPE VIDENCTEST_Starting(MYDATATYPE* pAppData)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_HANDLETYPE pHandle;
    OMX_U32 nCounter = 0;
    OMX_U32 nBuffersToSend;

    pHandle = pAppData->pHandle;
    pAppData->pComponent = (OMX_COMPONENTTYPE*)pHandle;

    /* SR10519/17735: Set the interval for the insertion of intra frames at run-time using a custom OMX index */
    eError = OMX_GetExtensionIndex(pHandle,"OMX.TI.VideoEncode.Config.IntraFrameInterval", (OMX_INDEXTYPE*)(&(pAppData->nVideoEncodeCustomParamIndex)));
    VIDENCTEST_CHECK_EXIT(eError, "Error in OMX_GetExtensionIndex function");

   /* MPEG4/H263 DSP socket node default value */
    if(pAppData->nIntraFrameInterval == VIDENCTEST_USE_DEFAULT_VALUE){
            pAppData->nIntraFrameInterval = 30;
    }
    eError = OMX_SetConfig(pHandle, pAppData->nVideoEncodeCustomParamIndex, &(pAppData->nIntraFrameInterval));
    VIDENCTEST_CHECK_EXIT(eError, "Error in OMX_SetConfig function");

    /* This test only applies to MPEG4/H.263 encoding */
    if (pAppData->eCompressionFormat == OMX_VIDEO_CodingMPEG4 ||
        pAppData->eCompressionFormat == OMX_VIDEO_CodingH263) {
        /* SR10540: Set the Target Frame Rate at run-time using a custom OMX index */
        eError = OMX_GetExtensionIndex(pHandle,
                                       "OMX.TI.VideoEncode.Config.TargetFrameRate",
                                       (OMX_INDEXTYPE*)(&(pAppData->nVideoEncodeCustomParamIndex)));
        VIDENCTEST_CHECK_EXIT(eError, "Error in OMX_GetExtensionIndex function");

        pAppData->nTargetFrameRate = pAppData->nFramerate; /* Refer to DSP socket node interface guide for usage */
        eError = OMX_SetConfig(pHandle, pAppData->nVideoEncodeCustomParamIndex, &(pAppData->nTargetFrameRate));
        VIDENCTEST_CHECK_EXIT(eError, "Error in OMX_SetConfig function");
    }

    /* SR10523: Set the AIRRate at run-time using a custom OMX index */
    eError = OMX_GetExtensionIndex(pHandle,
                                   "OMX.TI.VideoEncode.Config.AIRRate",
                                   (OMX_INDEXTYPE*)(&(pAppData->nVideoEncodeCustomParamIndex)));
    VIDENCTEST_CHECK_EXIT(eError, "Error in OMX_GetExtensionIndex function");
    pAppData->nAIRRate = 0; /* Refer to DSP socket node interface guide for usage */
    eError = OMX_SetConfig(pHandle, pAppData->nVideoEncodeCustomParamIndex, &(pAppData->nAIRRate));
    VIDENCTEST_CHECK_EXIT(eError, "Error in OMX_SetConfig function");

    /* SR12005: Set the Target Bit Rate at run-time using a custom OMX index */
    eError = OMX_GetExtensionIndex(pHandle,"OMX.TI.VideoEncode.Config.TargetBitRate", (OMX_INDEXTYPE*)(&(pAppData->nVideoEncodeCustomParamIndex)));
    VIDENCTEST_CHECK_EXIT(eError, "Error in OMX_GetExtensionIndex function");

    pAppData->nTargetBitRate = pAppData->nBitrate; /* Refer to DSP socket node interface guide for usage */
    eError = OMX_SetConfig(pHandle, pAppData->nVideoEncodeCustomParamIndex, &(pAppData->nTargetBitRate));
    VIDENCTEST_CHECK_EXIT(eError, "Error in OMX_SetConfig function");

    /*Initialize Frame Counter */
    pAppData->nCurrentFrameIn = 0;
    pAppData->nCurrentFrameOut = 0;


    /*Validation of stopframe in cases of Pause->Resume and Stop->Restart test cases*/
    if ( pAppData->eTypeOfTest == VIDENCTEST_StopRestart || pAppData->eTypeOfTest == VIDENCTEST_PauseResume) {
        if (pAppData->nReferenceFrame <= 1 ){
            nBuffersToSend = 1;
        }
        else if (pAppData->nReferenceFrame < NUM_OF_IN_BUFFERS){
            nBuffersToSend = pAppData->nReferenceFrame;
        }
        else {
            nBuffersToSend = NUM_OF_IN_BUFFERS;
        }
    }
    else {
        nBuffersToSend = NUM_OF_IN_BUFFERS;
    }
        /* Send FillThisBuffertoOMXVideoEncoder */

        if (pAppData->eTypeOfTest == VIDENCTEST_PartialRecord && pAppData->nReferenceFrame < NUM_OF_IN_BUFFERS) {
            nBuffersToSend = pAppData->nReferenceFrame;
            }



    for (nCounter = 0; nCounter < nBuffersToSend; nCounter++) {
        pAppData->pOutBuff[nCounter]->nFilledLen = 0;
        eError = pAppData->pComponent->FillThisBuffer(pHandle, pAppData->pOutBuff[nCounter]);
        VIDENCTEST_CHECK_EXIT(eError, "Error in FillThisBuffer");
        pAppData->nOutBufferCount--;
    }

    /* Send EmptyThisBuffer to OMX Video Encoder */
    for (nCounter = 0; nCounter < nBuffersToSend; nCounter++) {
        pAppData->pInBuff[nCounter]->nFilledLen = VIDENCTEST_fill_data(pAppData->pInBuff[nCounter], pAppData->fIn , pAppData->pInPortDef->nBufferSize);

        if (pAppData->pInBuff[nCounter]->nFlags == OMX_BUFFERFLAG_EOS && pAppData->pInBuff[nCounter]->nFilledLen == 0) {
            pAppData->eCurrentState = VIDENCTEST_StateStopping;
            eError = pAppData->pComponent->EmptyThisBuffer(pHandle, pAppData->pInBuff[nCounter]);
            VIDENCTEST_CHECK_ERROR(eError, "Error at EmptyThisBuffer function");
            pAppData->nInBufferCount--;
            goto EXIT;
        }
        else {
            pAppData->pComponent->EmptyThisBuffer(pHandle, pAppData->pInBuff[nCounter]);
            pAppData->nInBufferCount--;
            pAppData->nCurrentFrameIn++;
        }
    }

    pAppData->eCurrentState = VIDENCTEST_StateEncoding;

EXIT:
    return eError;
}

/*-----------------------------------------------------------------------------*/
/**
  * DeInit()
  *
  *Pass the component to executing  and fill the first 4 buffers.
  *
  * @param  pAppData MYDATA handle
  *
  * @retval OMX_ErrorNone
  *
  *
  **/
/*-----------------------------------------------------------------------------*/
 OMX_ERRORTYPE VIDENCTEST_DeInit(MYDATATYPE* pAppData)
 {
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_ERRORTYPE eErr = OMX_ErrorNone;
    VIDENCTEST_NODE* pListHead;
    OMX_HANDLETYPE pHandle = pAppData->pHandle;
    pListHead = pAppData->pMemoryListHead;

    /* Free Component Handle */
    eError = TIOMX_FreeHandle(pHandle);
    VIDENCTEST_CHECK_EXIT(eError, "Error in TIOMX_FreeHandle");

    /* De-Initialize OMX Core */
    eError = TIOMX_Deinit();
    VIDENCTEST_CHECK_EXIT(eError, "Error in TIOMX_Deinit");

    /* shutdown */
    fclose(pAppData->fIn);
    fclose(pAppData->fOut);
    if(pAppData->NalFormat == VIDENC_TEST_NAL_FRAME || pAppData->NalFormat == VIDENC_TEST_NAL_SLICE) {
        fclose(pAppData->fNalnd);
    }

    eErr = close(pAppData->IpBuf_Pipe[0]);
    if (0 != eErr && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
        VIDENCTEST_DPRINT ("Error while closing data pipe\n");
    }

    eErr = close(pAppData->OpBuf_Pipe[0]);
    if (0 != eErr && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
        VIDENCTEST_DPRINT ("Error while closing data pipe\n");
    }

    eErr = close(pAppData->eventPipe[0]);
    if (0 != eErr && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
        VIDENCTEST_DPRINT ("Error while closing data pipe\n");
    }

    eErr = close(pAppData->IpBuf_Pipe[1]);
    if (0 != eErr && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
        VIDENCTEST_DPRINT ("Error while closing data pipe\n");
    }

    eErr = close(pAppData->OpBuf_Pipe[1]);
    if (0 != eErr && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
        VIDENCTEST_DPRINT ("Error while closing data pipe\n");
    }

    eErr = close(pAppData->eventPipe[1]);
    if (0 != eErr && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
        VIDENCTEST_DPRINT ("Error while closing data pipe\n");
    }

    pAppData->fIn = NULL;
    pAppData->fOut = NULL;
    pAppData->fNalnd = NULL;

    VIDENCTEST_FreeResources(pAppData);
    VIDENCTEST_CHECK_EXIT(eError, "Error in FillThisBuffer");

    VIDENCTEST_FREE(pAppData, pListHead);
    VIDENCTEST_ListDestroy(pListHead);
    pAppData = NULL;

EXIT:
    return eError;

}

/*-----------------------------------------------------------------------------*/
/**
  * HandleState()
  *
  *StateTransitions Driven.
  *
  * @param  pAppData
  * @param  nData2 - State that has been set.
  *
  * @retval OMX_ErrorNone
  *
  *
  **/
/*-----------------------------------------------------------------------------*/
OMX_ERRORTYPE VIDENCTEST_HandleState(MYDATATYPE* pAppData, OMX_U32 eState)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_HANDLETYPE pHandle = pAppData->pHandle;
    OMX_U32 nCounter = 0;
    VIDENCTEST_NODE* pListHead;

    pListHead = pAppData->pMemoryListHead;

    switch (eState) {
        case OMX_StateLoaded:
            VIDENCTEST_PRINT("Component in OMX_StateLoaded\n");
            if(pAppData->eCurrentState == VIDENCTEST_StateLoaded) {
                pAppData->eCurrentState = VIDENCTEST_StateUnLoad;
            }
            break;
        case OMX_StateIdle:
            VIDENCTEST_PRINT("Component in OMX_StateIdle\n");
            if (pAppData->eCurrentState == VIDENCTEST_StateLoaded) {
                pAppData->eCurrentState = VIDENCTEST_StateReady;
                VIDENCTEST_PRINT("Send Component to OMX_StateExecuting\n");
                eError = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
                VIDENCTEST_CHECK_EXIT(eError, "Error from OMX_SendCommand function\n");
            }
            else if(pAppData->eCurrentState == VIDENCTEST_StateWaitEvent){
                if(pAppData->bStop == OMX_TRUE){
                    int count;
                    VIDENCTEST_PRINT("Component in OMX_StateStop\n");
                    pAppData->eCurrentState = VIDENCTEST_StateStop;
                    /*printf("Press any key to resume...");*/
                    /*getchar();*/
                    for(count = 5; count >= 0; count--) {
                        printf("App stopped: restart in %d seconds\n", count);
                        sleep(1);
                    }
                    pAppData->bStop = OMX_FALSE;
                }

                VIDENCTEST_PRINT("Disable Input Port\n");
                eError = OMX_SendCommand(pHandle, OMX_CommandPortDisable, VIDENC_INPUT_PORT, NULL);
                VIDENCTEST_CHECK_EXIT(eError, "Error from OMX_SendCommand function\n");

                if (pAppData->bAllocateIBuf == OMX_TRUE) {
                    for (nCounter = 0; nCounter < NUM_OF_IN_BUFFERS; nCounter++) {
                        pAppData->pIBuffer[nCounter] -= 128;
                        pAppData->pIBuffer[nCounter] = (unsigned char*)pAppData->pIBuffer[nCounter];
                        VIDENCTEST_FREE(pAppData->pIBuffer[nCounter], pListHead);
                        pAppData->pIBuffer[nCounter] = NULL;
                    }
                }
                for (nCounter = 0; nCounter < NUM_OF_IN_BUFFERS; nCounter++) {
                    eError = OMX_FreeBuffer(pHandle, pAppData->pInPortDef->nPortIndex, pAppData->pInBuff[nCounter]);
                }
            }
            break;
        case OMX_StateExecuting:
        VIDENCTEST_PRINT("Component in OMX_StateExecuting\n");
                pAppData->eCurrentState = VIDENCTEST_StateStarting;
                eError = VIDENCTEST_Starting(pAppData);
            break;
        case OMX_StatePause:
            VIDENCTEST_PRINT("Component in OMX_StatePause\n");
            pAppData->eCurrentState = VIDENCTEST_StatePause;
            /*printf("Press any key to resume...");*/
            /*getchar();*/
            int count;
            for(count = 5; count >= 0; count--) {
                printf("App paused: resume in %d seconds\n", count);
                sleep(1);
            }
            pAppData->bStop = OMX_FALSE;
            eError = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
            break;
        case OMX_StateWaitForResources:
        VIDENCTEST_PRINT("Component in OMX_StateWaitForResources\n");
            break;
        case OMX_StateInvalid:
        VIDENCTEST_PRINT("Component in OMX_StateInvalid\n");
             eError = OMX_ErrorInvalidState;
        }

EXIT:
    return eError;
}

/*-----------------------------------------------------------------------------*/
/**
  * CheckArgs_H263()
  *
  *Validates h.263 input parameters
  *
  * @param  pAppData
  * @param  argv
  *
  * @retval OMX_ErrorNone
  *
  *
  **/
/*-----------------------------------------------------------------------------*/
OMX_ERRORTYPE VIDENCTEST_CheckArgs_H263(MYDATATYPE* pAppData, char** argv)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    switch (atoi(argv[9])) {
        case 0:
            pAppData->eControlRate = OMX_Video_ControlRateDisable;
            break;
        case 1:
            pAppData->eControlRate = OMX_Video_ControlRateVariable;
            break;
        case 2:
            pAppData->eControlRate = OMX_Video_ControlRateConstant;
            break;
        default:
            eError = OMX_ErrorBadParameter;
            printf("*Error: Input Argument CONTROL RATE is not valid");
            goto EXIT;
    }

    pAppData->nGOBHeaderInterval = atoi(argv[10]);
    pAppData->nQpI = atoi(argv[11]);

    switch (atoi(argv[12])) {
        case 10:
            pAppData->eLevelH63 = OMX_VIDEO_H263Level10;
            break;
        case 20:
            pAppData->eLevelH63 = OMX_VIDEO_H263Level20;
            break;
        case 30:
            pAppData->eLevelH63 = OMX_VIDEO_H263Level30;
            break;
        case 40:
            pAppData->eLevelH63 = OMX_VIDEO_H263Level40;
            break;
        case 45:
            pAppData->eLevelH63 = OMX_VIDEO_H263Level45;
            break;
        case 50:
            pAppData->eLevelH63 = OMX_VIDEO_H263Level50;
            break;
        case 60:
            pAppData->eLevelH63 = OMX_VIDEO_H263Level60;
            break;
        case 70:
            pAppData->eLevelH63 = OMX_VIDEO_H263Level70;
            break;
        default:
            eError = OMX_ErrorBadParameter;
            printf("*Error: Input Argument LEVEL is not valid");
            goto EXIT;
    }

    if (atoi(argv[13]) == 0) {
        pAppData->bAllocateIBuf = OMX_FALSE;
        pAppData->bAllocateOBuf = OMX_FALSE;
    }
    else {
        pAppData->bAllocateIBuf = OMX_TRUE;
        pAppData->bAllocateOBuf = OMX_TRUE;
    }

    pAppData->nVBVSize = atoi(argv[14]);


EXIT:
    return eError;
}

/*-----------------------------------------------------------------------------*/
/**
  * CheckArgs_Mpeg4()
  *
  *Validates Mpeg4 input parameters
  *
  * @param  pAppData
  * @param  argv
  *
  * @retval OMX_ErrorNone
  *
  *
  **/
/*-----------------------------------------------------------------------------*/
OMX_ERRORTYPE VIDENCTEST_CheckArgs_Mpeg4(MYDATATYPE* pAppData, char** argv)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    switch (atoi(argv[9])) {
        case 0:
            pAppData->eControlRate = OMX_Video_ControlRateDisable;
            break;
        case 1:
            pAppData->eControlRate = OMX_Video_ControlRateVariable;
            break;
        case 2:
            pAppData->eControlRate = OMX_Video_ControlRateConstant;
            break;
        default:
            eError = OMX_ErrorBadParameter;
            printf("*Error: Input Argument CONTROL RATE is not valid");
            goto EXIT;
    }

    pAppData->nQpI = atoi(argv[11]);

#if 0
   pAppData->eLevelMpeg4 = atoi(argv[12]);

#else
    switch (atoi(argv[12])) {
        case 0:
            pAppData->eLevelMpeg4 = OMX_VIDEO_MPEG4Level0;
            break;
        case 1:
            pAppData->eLevelMpeg4 = OMX_VIDEO_MPEG4Level1;
            break;
        case 2:
            pAppData->eLevelMpeg4 = OMX_VIDEO_MPEG4Level2;
            break;
        case 3:
            pAppData->eLevelMpeg4 = OMX_VIDEO_MPEG4Level3;
            break;
        case 4:
            pAppData->eLevelMpeg4 = OMX_VIDEO_MPEG4Level4;
            break;
        case 5:
            pAppData->eLevelMpeg4 = OMX_VIDEO_MPEG4Level5;
            break;
        case 100:
            pAppData->eLevelMpeg4 = OMX_VIDEO_MPEG4Level0b;
            break;
        default:
            eError = OMX_ErrorBadParameter;
            printf("*Error: Input Argument LEVEL is not valid");
            goto EXIT;
        }
#endif


    if (atoi(argv[13]) == 0) {
        pAppData->bAllocateIBuf = OMX_FALSE;
        pAppData->bAllocateOBuf = OMX_FALSE;
    }
    else {
        pAppData->bAllocateIBuf = OMX_TRUE;
        pAppData->bAllocateOBuf = OMX_TRUE;
    }

    pAppData->nVBVSize = atoi(argv[14]);


EXIT:
    return eError;
}

/*-----------------------------------------------------------------------------*/
/**
  * CheckArgs_AVC()
  *
  *Validates Mpeg4 input parameters
  *
  * @param  pAppData
  * @param  argv
  *
  * @retval OMX_ErrorNone
  *
  *
  **/
/*-----------------------------------------------------------------------------*/
OMX_ERRORTYPE VIDENCTEST_CheckArgs_AVC(MYDATATYPE* pAppData, char** argv)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    switch (atoi(argv[9])) {
        case 10:
            pAppData->eLevelH264 = OMX_VIDEO_AVCLevel1;
            break;
        case 11:
            pAppData->eLevelH264 = OMX_VIDEO_AVCLevel11;
            break;
        case 12:
            pAppData->eLevelH264 = OMX_VIDEO_AVCLevel12;
            break;
        case 13:
            pAppData->eLevelH264 = OMX_VIDEO_AVCLevel13;
            break;
        case 20:
            pAppData->eLevelH264 = OMX_VIDEO_AVCLevel2;
            break;
        case 21:
            pAppData->eLevelH264 = OMX_VIDEO_AVCLevel21;
            break;
        case 22:
            pAppData->eLevelH264 = OMX_VIDEO_AVCLevel22;
            break;
        case 30:
            pAppData->eLevelH264 = OMX_VIDEO_AVCLevel3;
            break;
        case 9:
            pAppData->eLevelH264 = OMX_VIDEO_AVCLevel1b;
            break;
        default:
            eError = OMX_ErrorBadParameter;
            printf("*Error: Input Argument CONTROL RATE is not valid");
            goto EXIT;
    }

    pAppData->nOutBuffSize  = atoi(argv[10]);

    if (atoi(argv[11]) == 0) {
        pAppData->bAllocateIBuf = OMX_FALSE;
        pAppData->bAllocateOBuf = OMX_FALSE;
    }
    else {
        pAppData->bAllocateIBuf = OMX_TRUE;
        pAppData->bAllocateOBuf = OMX_TRUE;
    }

    if (atoi(argv[12]) == 0) {
            pAppData->bDeblockFilter = OMX_FALSE;
    }
    else if (atoi(argv[12]) == 1) {
        pAppData->bDeblockFilter = OMX_TRUE;
    }
    else {
        eError = OMX_ErrorBadParameter;
        printf("*Error: Input Argument DEBLOCK FILTER is not valid");
        goto EXIT;
    }


    switch (atoi(argv[13])) {
        case 0:
            pAppData->eControlRate = OMX_Video_ControlRateDisable;
            break;
        case 1:
            pAppData->eControlRate = OMX_Video_ControlRateVariable;
            break;
        case 2:
            pAppData->eControlRate = OMX_Video_ControlRateConstant;
            break;
        default:
            ;
    }

    pAppData->nQpI = atoi(argv[14]);

EXIT:
    return eError;
}

/*-----------------------------------------------------------------------------*/
/**
  * VIDENCTEST_CheckOptionalArgs()
  *
  *Validates input argument from user.
  *
  * @param  argc
  * @param  argv
  *
  * @retval OMX_ErrorNone
  *
  *
  **/
/*-----------------------------------------------------------------------------*/


OMX_ERRORTYPE VIDENCTEST_CheckOptionalArgs(MYDATATYPE* pAppData, int argc, char* argv []){
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    int next_option;
    const char* const short_options = "m:r:i:p:e:n";
    const struct option long_options[] = {
        {"nMIRRate",    1, NULL, 'm'},
        {"nResyncMarker",   1, NULL, 'r'},
        {"nIntraFrameInterval",   1, NULL, 'i'},
        {"nEncodingPreset",   1, NULL, 'p'},
        {"nRrker",   1, NULL, 'e'},
        {"NALFormat",   1, NULL, 'n'},
        {"nQPIoF", 1, NULL, 'q'},
        {"nUnrestrictedMV", 1, NULL, 'u'},
        {NULL,          0, NULL,   0}
    };

    VIDENCTEST_PRINT("checking for optional args\n");
    do
        {
        next_option = getopt_long(argc, argv, short_options, long_options, NULL);
        switch (next_option)
            {
            case 'm':
                printf("%d MIRRate found, Value= %s\n",next_option,optarg);
                pAppData->nMIRRate=atoi(optarg);
                break;
            case 'r':
                printf("%d ResyncMarker found, Value= %s\n",next_option,optarg);
                pAppData->nResynchMarkerSpacing=atoi(optarg);
                break;
            case 'i':
                printf("%d IntraFrameInterval found, Value= %s\n",next_option,optarg);
                pAppData->nIntraFrameInterval=atoi(optarg);
                break;
            case 'p':
                if(pAppData->eCompressionFormat == OMX_VIDEO_CodingAVC){
                    printf("%d EncodingPreset found, Value= %s\n",next_option,optarg);
                    pAppData->nEncodingPreset=atoi(optarg);
                    }
                else
                    printf("** Warning: EncodingPreset valid only for H264. Proceeding with test.\n");
                break;
            case 'n':
                printf("%d Nal Format found, Value= %s\n",next_option,optarg);
                pAppData->NalFormat=atoi(optarg);
                break;
            case 'q':
                printf("%d QPI value changed each %d frames\n",next_option,atoi(optarg));
                pAppData->nQPIoF=atoi(optarg);
            case 'u':
                printf("%d nUnrestrictedMV found, Value= %s\n",next_option,optarg);
                pAppData->nUnrestrictedMV=atoi(optarg);
                break;
            case -1:
                break;
            default:
                printf("** Warning: %d No valid param found. Proceeding with test. optarg= %s\n",next_option,optarg);

            }

    }
    while(next_option != -1);

    return eError;

}

/*-----------------------------------------------------------------------------*/
/**
  * CheckArgs()
  *
  *Validates input argument from user.
  *
  * @param  argc
  * @param  argv
  * @param pAppDataTmp
  *
  * @retval OMX_ErrorNone
  *
  *
  **/
/*-----------------------------------------------------------------------------*/
OMX_ERRORTYPE VIDENCTEST_CheckArgs(int argc, char** argv, MYDATATYPE** pAppDataTmp)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    VIDENCTEST_NODE*  pMemoryListHead;
    MYDATATYPE* pAppData;

    eError = VIDENCTEST_ListCreate(&pMemoryListHead);
    VIDENCTEST_CHECK_EXIT(eError, "Error at Creating Memory List");

    VIDENCTEST_MALLOC(pAppData, sizeof(MYDATATYPE), MYDATATYPE, pMemoryListHead);
    VIDENCTEST_CHECK_EXIT(eError, "Error at Allocating MYDATATYPE structure");
    *pAppDataTmp = pAppData;
    pAppData->pMemoryListHead = pMemoryListHead;
    pAppData->eCurrentState = VIDENCTEST_StateUnLoad;
    pAppData->nMIRRate=VIDENCTEST_USE_DEFAULT_VALUE;
    pAppData->nResynchMarkerSpacing=VIDENCTEST_USE_DEFAULT_VALUE;
    pAppData->nIntraFrameInterval=VIDENCTEST_USE_DEFAULT_VALUE;
    pAppData->nEncodingPreset=VIDENCTEST_USE_DEFAULT_VALUE_UI;
    pAppData->nUnrestrictedMV=(OMX_U8)VIDENCTEST_USE_DEFAULT_VALUE_UI;
    pAppData->NalFormat = 0;
    pAppData->nQPIoF = 0;
    pAppData->bForceIFrame = 0;


    if (argc < 15){
        printf("*Error: Input argument COLOR FORMAT is not valid");
        VIDENCTEST_PrintCorrectArgs(OMX_TRUE, OMX_TRUE);
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }
    else {

        pAppData->szInFile = argv[1];
        pAppData->fIn = fopen(pAppData->szInFile, "r");
        if (!pAppData->fIn) {
            printf("Error: failed to open the file <%s>", pAppData->szInFile);
            eError = OMX_ErrorBadParameter;
            goto EXIT;
        }

        pAppData->szOutFile = argv[2];
        pAppData->fOut = fopen(pAppData->szOutFile, "w");
        if (!pAppData->fOut) {
            VIDENCTEST_DPRINT("Error: failed to open the file <%s>", pAppData->szOutFile);
            eError = OMX_ErrorBadParameter;
            goto EXIT;
        }

        pAppData->nWidth = atoi(argv[3]);
        if (pAppData->nWidth & 15) {
            printf("**Warning: Input Argument WIDTH is not multiple of 16. \n");
        }

        pAppData->nHeight = atoi(argv[4]);
        if (pAppData->nHeight & 15) {
            printf("**Warning: Input Argument HEIGHT is not multiple of 16. \n");
        }

        switch (atoi(argv[5])) {
            case 19:
                pAppData->eColorFormat = OMX_COLOR_FormatYUV420Planar;/*420*/
                break;
            case 25:
                pAppData->eColorFormat = OMX_COLOR_FormatYCbYCr;/*422-BE*/
                break;
            case 26:
                pAppData->eColorFormat = OMX_COLOR_FormatCbYCrY;/*422-LE*/
                break;
            default:
                printf("*Error: Input Argument COLOR FORMAT is not valid");
                VIDENCTEST_PrintCorrectArgs(OMX_TRUE, OMX_TRUE);
                eError = OMX_ErrorBadParameter;
                goto EXIT;
        }

        pAppData->nBitrate = atoi(argv[6]);
        if(pAppData->nBitrate > 10000000) {
            printf("**Warning: Input argument BITRATE outside of tested range, behavior of component unknown.\n");
        }

        pAppData->nFramerate = atoi(argv[7]);
        if(pAppData->nFramerate < 7 || pAppData->nFramerate > 30) {
            printf("**Warning: Input argument FRAMERATE outside of tested range, behavior of component unknown.\n");
        }

        switch (atoi(argv[8])) {
            case 3:
                pAppData->eCompressionFormat = OMX_VIDEO_CodingH263;
                eError = VIDENCTEST_CheckArgs_H263(pAppData, argv);
                VIDENCTEST_CHECK_EXIT(eError, "Error at Checking arguments for H263");
                break;
            case 4:
                pAppData->eCompressionFormat = OMX_VIDEO_CodingMPEG4;
                eError = VIDENCTEST_CheckArgs_Mpeg4(pAppData, argv);
                VIDENCTEST_CHECK_EXIT(eError, "Error at Checking arguments for MPEG4");
                break;
            case 7:
                pAppData->eCompressionFormat = OMX_VIDEO_CodingAVC;
                eError = VIDENCTEST_CheckArgs_AVC(pAppData, argv);
                VIDENCTEST_CHECK_EXIT(eError, "Error at Checking arguments for H264");
                break;
            default:
                printf("*Error: Input argument COLOR FORMAT is not valid");
                VIDENCTEST_PrintCorrectArgs(OMX_TRUE, OMX_TRUE);
                eError = OMX_ErrorBadParameter;
                goto EXIT;
        }
    }

    if (argc < 16) {
        pAppData->eTypeOfTest = VIDENCTEST_FullRecord;
        printf("**Warning: Input Arguments TYPE OF TEST is not include input. Using Default value VIDENCTEST_FullRecord.\n");
    }
    else {
        switch (atoi(argv[15])){
            case 0:
                pAppData->eTypeOfTest = VIDENCTEST_FullRecord;
                pAppData->bStop = OMX_FALSE;
                break;
            case 1:
                pAppData->eTypeOfTest = VIDENCTEST_PartialRecord;
                pAppData->bStop = OMX_FALSE;
                break;
            case 2:
                pAppData->eTypeOfTest = VIDENCTEST_PauseResume;
                pAppData->bStop = OMX_TRUE;
                break;
            case 3:
                pAppData->eTypeOfTest = VIDENCTEST_StopRestart;
                pAppData->bStop = OMX_TRUE;
                break;
            default:
                pAppData->eTypeOfTest = VIDENCTEST_FullRecord;
                printf("**Warning: Input Argument TYPE OF TEST is out of range. Using Default value VIDENCTEST_FullRecord.\n");
        }
    }

    if (argc < 17) {
        pAppData->nReferenceFrame = 0;
        printf("**Warning: Input Arguments nReferenceFrame has not been specified. Using Default value 0.\n");
    }
    else{
        pAppData->nReferenceFrame = atoi(argv[16]);
    }

    if (argc < 18) {
        pAppData->nNumberOfTimesTodo = 1;
    }
    else{
        if(argv[17][0]<'0' || argv[17][0] > '9'){
            pAppData->nNumberOfTimesTodo = 1;
        }
        else{
            pAppData->nNumberOfTimesTodo  = atoi(argv[17]);
            VIDENCTEST_PRINT("nNumberOfTimesTodo %lu \n", pAppData->nNumberOfTimesTodo);
        }
    }

    VIDENCTEST_CheckOptionalArgs(pAppData, argc, argv);

    if(pAppData->NalFormat == VIDENC_TEST_NAL_FRAME || pAppData->NalFormat == VIDENC_TEST_NAL_SLICE) {
        VIDENCTEST_MALLOC(pAppData->szOutFileNal, strlen(pAppData->szOutFile)+3, char, pMemoryListHead);
        strcpy(pAppData->szOutFileNal, pAppData->szOutFile);
        strcat(pAppData->szOutFileNal, "nd");
        pAppData->fNalnd = fopen(pAppData->szOutFileNal, "w");
        if (!pAppData->szOutFileNal) {
            VIDENCTEST_DPRINT("Error: failed to open the file <%s>", pAppData->szOutFileNal);
            eError = OMX_ErrorBadParameter;
            goto EXIT;
        }
    }

EXIT:
    return eError;
}

/*-----------------------------------------------------------------------------*/
/**
  * Confirm()
  *
  *Check what type of test, repetions to be done and takes certain path.
  *
  * @param  argc
  * @param  argv
  * @param pAppDataTmp
  *
  * @retval OMX_ErrorNone
  *
  *
  **/
/*-----------------------------------------------------------------------------*/
OMX_ERRORTYPE VIDENCTEST_Confirm(MYDATATYPE* pAppData)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_HANDLETYPE pHandle = pAppData->pHandle;

    switch (pAppData->eTypeOfTest){
        case VIDENCTEST_FullRecord:
        case VIDENCTEST_PartialRecord:
            pAppData->nNumberOfTimesDone++;
            if(pAppData->nNumberOfTimesTodo == pAppData->nNumberOfTimesDone) {
                pAppData->bExit = OMX_TRUE;
            }
            else {
                pAppData->bExit = OMX_FALSE;
                VIDENCTEST_PRINT("\nTimes Done: %i of %i \n", pAppData->nNumberOfTimesDone, pAppData->nNumberOfTimesTodo);
                pAppData->fOut = fopen(pAppData->szOutFile, "w");
                if(pAppData->NalFormat == VIDENC_TEST_NAL_FRAME || pAppData->NalFormat == VIDENC_TEST_NAL_SLICE) {
                    pAppData->fNalnd = fopen(pAppData->szOutFileNal, "w");
                }
                if (!pAppData->fIn){
                    VIDENCTEST_CHECK_EXIT(OMX_ErrorUndefined, "Error at fopen function");
                }
                rewind(pAppData->fIn);
            }
            eError = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
            VIDENCTEST_CHECK_EXIT(eError, "Error at OMX_SendCommand function");
            break;
        case VIDENCTEST_PauseResume:
        case VIDENCTEST_StopRestart:
            if (pAppData->bStop == OMX_TRUE) {
                if(pAppData->eTypeOfTest == VIDENCTEST_StopRestart) {
                    rewind(pAppData->fIn);
                    eError = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
                    VIDENCTEST_CHECK_EXIT(eError, "Error at OMX_SendCommand function");
                }
                else if (pAppData->eTypeOfTest == VIDENCTEST_PauseResume) {
                    eError = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StatePause, NULL);
                    VIDENCTEST_CHECK_EXIT(eError, "Error at OMX_SendCommand function");
                }
            }
            else {
                pAppData->nNumberOfTimesDone++;
                if(pAppData->nNumberOfTimesTodo == pAppData->nNumberOfTimesDone) {
                    pAppData->bExit = OMX_TRUE;
                }
                else {
                    pAppData->bExit = OMX_FALSE;
                    fprintf(stdout, "\nTimes Done: %lx of %lx \n", pAppData->nNumberOfTimesDone, pAppData->nNumberOfTimesTodo);
                    if(pAppData->eTypeOfTest == VIDENCTEST_StopRestart || pAppData->eTypeOfTest == VIDENCTEST_PauseResume) {
                        pAppData->bStop = OMX_TRUE;
                    }
                    else{
                        pAppData->bStop = OMX_FALSE;
                    }

                    pAppData->fOut = fopen(pAppData->szOutFile, "w");
                    if (!pAppData->fOut){
                        VIDENCTEST_CHECK_EXIT(OMX_ErrorUndefined, "Error at fopen function");
                    }
                    if(pAppData->NalFormat == VIDENC_TEST_NAL_FRAME || pAppData->NalFormat == VIDENC_TEST_NAL_SLICE) {
                        pAppData->fNalnd = fopen(pAppData->szOutFileNal, "w");
                        if (!pAppData->fNalnd){
                            VIDENCTEST_CHECK_EXIT(OMX_ErrorUndefined, "Error at fopen function");
                        }
                    }
                    rewind(pAppData->fIn);
                }
                eError = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
                VIDENCTEST_CHECK_EXIT(eError, "Error at OMX_SendCommand function");
            }
        }

        pAppData->eCurrentState = VIDENCTEST_StateWaitEvent;

EXIT:
    return eError;
}

/*-----------------------------------------------------------------------------*/
/**
  * HandlePortDisable()
  *
  *Handles PortDisable Event
  *
  * @param  argc
  * @param  argv
  * @param pAppDataTmp
  *
  * @retval OMX_ErrorNone
  *
  *
  **/
/*-----------------------------------------------------------------------------*/
OMX_ERRORTYPE VIDENCTEST_HandlePortDisable(MYDATATYPE* pAppData, OMX_U32 ePort)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_HANDLETYPE pHandle = pAppData->pHandle;
    OMX_U32 nCounter;
    VIDENCTEST_NODE* pListHead;

    pAppData->nUnresponsiveCount = 0;
    pListHead = pAppData->pMemoryListHead;

    if (ePort == VIDENC_INPUT_PORT){
        eError = OMX_SendCommand(pHandle, OMX_CommandPortDisable, VIDENC_OUTPUT_PORT, NULL);
        VIDENCTEST_CHECK_EXIT(eError, "Error at OMX_SendCommand function");

        if (pAppData->bAllocateOBuf == OMX_TRUE) {
            for (nCounter = 0; nCounter < NUM_OF_OUT_BUFFERS; nCounter++) {
                pAppData->pOBuffer[nCounter] -= 128;
                pAppData->pOBuffer[nCounter] = (unsigned char*)pAppData->pOBuffer[nCounter];
                VIDENCTEST_FREE(pAppData->pOBuffer[nCounter], pListHead);
                pAppData->pOBuffer[nCounter] = NULL;
            }
        }
        for (nCounter = 0; nCounter < NUM_OF_OUT_BUFFERS; nCounter++) {
            eError = OMX_FreeBuffer(pHandle, pAppData->pOutPortDef->nPortIndex, pAppData->pOutBuff[nCounter]);
            VIDENCTEST_CHECK_EXIT(eError, "Error at OMX_FreeBuffer function");
        }
    }
    else if(ePort == VIDENC_OUTPUT_PORT) {
        if (pAppData->bExit == OMX_TRUE) {
            eError = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateLoaded, NULL);
            VIDENCTEST_CHECK_EXIT(eError, "Error at OMX_SendCommand function");
            pAppData->eCurrentState = VIDENCTEST_StateLoaded;
        }
        else {
            eError = OMX_SendCommand(pHandle, OMX_CommandPortEnable, VIDENC_INPUT_PORT, NULL);
            VIDENCTEST_CHECK_EXIT(eError, "Error at OMX_SendCommand function");

            for (nCounter = 0; nCounter < NUM_OF_IN_BUFFERS; nCounter++) {
                if (pAppData->bAllocateIBuf == OMX_TRUE) {
                    VIDENCTEST_MALLOC(pAppData->pIBuffer[nCounter], pAppData->nSizeIn + 256, OMX_U8, pListHead);
                    pAppData->pIBuffer[nCounter] += 128;
                    pAppData->pIBuffer[nCounter] = (unsigned char*)pAppData->pIBuffer[nCounter];
                }
            }

            for (nCounter = 0; nCounter < NUM_OF_IN_BUFFERS; nCounter++) {
                if (pAppData->bAllocateIBuf == OMX_TRUE) {
                eError = OMX_UseBuffer(pHandle,
                                        &pAppData->pInBuff[nCounter],
                                        pAppData->pInPortDef->nPortIndex,
                                        pAppData,
                                        pAppData->pInPortDef->nBufferSize,
                                        pAppData->pIBuffer[nCounter]);
                VIDENCTEST_CHECK_EXIT(eError, "Error at OMX_UseBuffer function");
                }
                else {
                    eError = OMX_AllocateBuffer(pHandle,
                                                &pAppData->pInBuff[nCounter],
                                                pAppData->pInPortDef->nPortIndex,
                                                pAppData,
                                                pAppData->pInPortDef->nBufferSize);
                    VIDENCTEST_CHECK_EXIT(eError, "Error at OMX_AllocateBuffer function");
                }
            }
        }
    }


EXIT:
    return eError;
}

/*-----------------------------------------------------------------------------*/
/**
  * HandlePortEnable()
  *
  *Handles PortEnable Event
  *
  * @param  pAppData
  * @param  nPort
  *
  * @retval OMX_ErrorNone
  *
  *
  **/
/*-----------------------------------------------------------------------------*/
OMX_ERRORTYPE VIDENCTEST_HandlePortEnable(MYDATATYPE* pAppData, OMX_U32 ePort)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_HANDLETYPE pHandle = pAppData->pHandle;
    OMX_U32 nCounter;
    VIDENCTEST_NODE* pListHead;

    pAppData->nUnresponsiveCount = 0;
    pListHead = pAppData->pMemoryListHead;

    if (ePort == VIDENC_INPUT_PORT){
        eError = OMX_SendCommand(pHandle, OMX_CommandPortEnable, VIDENC_OUTPUT_PORT, NULL);
        VIDENCTEST_CHECK_EXIT(eError, "Error at OMX_SendCommand function");

        for (nCounter = 0; nCounter < NUM_OF_OUT_BUFFERS; nCounter++) {
            if (pAppData->bAllocateOBuf == OMX_TRUE) {
                VIDENCTEST_MALLOC(pAppData->pOBuffer[nCounter], pAppData->nSizeOut + 256, OMX_U8, pListHead);
                pAppData->pOBuffer[nCounter] += 128;
                pAppData->pOBuffer[nCounter] = (unsigned char*)pAppData->pOBuffer[nCounter];
            }
        }

        for (nCounter = 0; nCounter < NUM_OF_OUT_BUFFERS; nCounter++) {
            if (pAppData->bAllocateOBuf == OMX_TRUE) {
                eError = OMX_UseBuffer(pHandle,
                                        &pAppData->pOutBuff[nCounter],
                                        pAppData->pOutPortDef->nPortIndex,
                                        pAppData,
                                        pAppData->pOutPortDef->nBufferSize,
                                        pAppData->pOBuffer[nCounter]);
                VIDENCTEST_CHECK_EXIT(eError, "Error at OMX_UseBuffer function");
            }
            else {
                eError = OMX_AllocateBuffer(pHandle,
                                            &pAppData->pOutBuff[nCounter],
                                            pAppData->pOutPortDef->nPortIndex,
                                            pAppData,
                                            pAppData->pOutPortDef->nBufferSize);
                VIDENCTEST_CHECK_EXIT(eError, "Error at OMX_AllocateBuffer function");
            }
        }
    }
    else if(ePort == VIDENC_OUTPUT_PORT){
        eError = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
        VIDENCTEST_CHECK_EXIT(eError, "Error at OMX_SendCommand function");
    }

EXIT:
    return eError;
}

/*-----------------------------------------------------------------------------*/
/**
  * HandlePortEnable()
  *
  *Handles PortEnable Event
  *
  * @param  pAppData
  * @param  nPort
  *
  * @retval OMX_ErrorNone
  *
  *
  **/
/*-----------------------------------------------------------------------------*/
OMX_ERRORTYPE VIDENCTEST_HandleEventError(MYDATATYPE* pAppData, OMX_U32 eErr, OMX_U32 nData2)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    VIDENCTEST_UNUSED_ARG(nData2);
    VIDENCTEST_PRINT("\n------VIDENCTEST ERROR-------\nOMX_EventError\n");
    VIDENCTEST_PRINT("eErr : %x \n", eErr);
    VIDENCTEST_PRINT("nData2 : %x \n", nData2);

    switch (eErr){
     case OMX_ErrorNone:
        break;
     case OMX_ErrorInsufficientResources:
     case OMX_ErrorUndefined:
     case OMX_ErrorInvalidComponentName:
     case OMX_ErrorComponentNotFound:
     case OMX_ErrorInvalidComponent:
        eError = eErr;
        break;
     case OMX_ErrorBadParameter:
     case OMX_ErrorNotImplemented:
     case OMX_ErrorUnderflow:
     case OMX_ErrorOverflow:
        break;
     case OMX_ErrorInvalidState:
#ifdef DSP_MMU_FAULT_HANDLING
        bInvalid_state = OMX_TRUE;
#endif
     case OMX_ErrorHardware:
     case OMX_ErrorStreamCorrupt:
     case OMX_ErrorPortsNotCompatible:
     case OMX_ErrorResourcesLost:
     case OMX_ErrorNoMore:
     case OMX_ErrorVersionMismatch:
     case OMX_ErrorNotReady:
     case OMX_ErrorTimeout:
     case OMX_ErrorSameState:
     case OMX_ErrorResourcesPreempted:
        eError = eErr;
        break;
     case OMX_ErrorPortUnresponsiveDuringAllocation:
     case OMX_ErrorPortUnresponsiveDuringDeallocation:
     case OMX_ErrorPortUnresponsiveDuringStop:
        if(pAppData->nUnresponsiveCount++ > MAX_UNRESPONSIVE_COUNT){
            eError = eErr;
        }
        break;
     case OMX_ErrorIncorrectStateTransition:
     case OMX_ErrorIncorrectStateOperation:
     case OMX_ErrorUnsupportedSetting:
     case OMX_ErrorUnsupportedIndex:
     case OMX_ErrorBadPortIndex:
     case OMX_ErrorMax:
     default:
        ;
    }

    return eError;
 }


/*-----------------------------------------------------------------------------*/
/**
  * Main()
  *
  *Control flow of the Stand Alone Test Application
  *
  * @param  argc
  * @param argv
  *
  * @retval OMX_ErrorNone
  *
  *
  **/
/*-----------------------------------------------------------------------------*/
int main(int argc, char** argv)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_HANDLETYPE pHandle;
    OMX_BUFFERHEADERTYPE* pBuffer;
    OMX_U32 nError;
    OMX_U32 nTimeCount;
    MYDATATYPE* pAppData;
    fd_set rfds;
    VIDENCTEST_NODE* pListHead;
    sigset_t set;

    nTimeCount = 0;
#ifdef DSP_MMU_FAULT_HANDLING
    bInvalid_state = OMX_FALSE;
#endif
    OMX_OTHER_EXTRADATATYPE_1_1_2 *pExtraDataType;
    OMX_U8* pTemp;
    OMX_U32* pIndexNal;
    OMX_U32 nNalSlices;


    VIDENCTEST_PRINT("Enter to CheckArgs\n");

    eError = VIDENCTEST_CheckArgs(argc, argv, &pAppData);

    VIDENCTEST_CHECK_ERROR(eError, "Invalid Arguments");
    VIDENCTEST_PRINT("Exit to CheckArgs\n");

    VIDENCTEST_PRINT("Enter to PassToLoaded\n");
    eError = VIDENCTEST_PassToLoaded(pAppData);
    VIDENCTEST_CHECK_ERROR(eError, "Error at Initialization of Component");
    VIDENCTEST_PRINT("Exit to PassToLoaded\n");

    pListHead = pAppData->pMemoryListHead;
    pHandle = pAppData->pHandle;

    VIDENCTEST_PRINT("Enter to PassToReady\n");
    eError = VIDENCTEST_PassToReady(pAppData);
    VIDENCTEST_CHECK_ERROR(eError, "Error at Passing to ComponentReady State");
    VIDENCTEST_PRINT("Exit to PassToReady\n");

    while(1){
        FD_ZERO(&rfds);
        FD_SET(pAppData->IpBuf_Pipe[0], &rfds);
        FD_SET(pAppData->OpBuf_Pipe[0], &rfds);
        FD_SET(pAppData->eventPipe[0], &rfds);

        sigemptyset(&set);
        sigaddset(&set,SIGALRM);
        pAppData->nRetVal = pselect(pAppData->fdmax+1, &rfds, NULL, NULL, NULL,&set);

        if (pAppData->nRetVal == -1) {
            perror("pselect()");
            VIDENCTEST_DPRINT("Error\n");
            break;
        }

        if (pAppData->nRetVal == 0) {
            if (nTimeCount++ > VIDENCTEST_MAX_TIME_OUTS) {
                VIDENCTEST_DPRINT("Application: Timeout!!!\n");
                printf("\n------VIDENCTEST FATAL ERROR-------\n Component Unresponsive \n");
                VIDENCTEST_HandleError(pAppData, OMX_ErrorUndefined);
                goto EXIT;
            }
            sched_yield();
        }
        else{
            nTimeCount = 0;
        }

        if (FD_ISSET(pAppData->eventPipe[0], &rfds)) {
            EVENT_PRIVATE* pEventPrivate = NULL;
            OMX_EVENTTYPE eEvent = -1;
            OMX_U32 nData1 = 0;
            OMX_U32 nData2 = 0;

            read(pAppData->eventPipe[0], &pEventPrivate, sizeof(pEventPrivate));
            eEvent = pEventPrivate->eEvent;
            nData1 = pEventPrivate->nData1;
            nData2 = pEventPrivate->nData2;

            switch (eEvent) {
                case OMX_EventCmdComplete:
                    switch (nData1){
                        case OMX_CommandStateSet:
                            VIDENCTEST_PRINT("Enter to HandleState\n");
                            eError = VIDENCTEST_HandleState(pAppData, nData2);
                            VIDENCTEST_CHECK_ERROR(eError, "Error at HandleState function");
                            VIDENCTEST_PRINT("Exit to HandleState\n");
                            break;
                        case OMX_CommandFlush:
                            break;
                        case OMX_CommandPortDisable:
                            VIDENCTEST_PRINT("Enter to HandlePortDisable\n");
                            eError = VIDENCTEST_HandlePortDisable(pAppData, nData2);
                            VIDENCTEST_CHECK_ERROR(eError, "Error at HandlePortDisable function");
                            VIDENCTEST_PRINT("Exits to HandlePortDisable\n");
                            break;
                        case OMX_CommandPortEnable:
                            VIDENCTEST_PRINT("Enter to HandlePortEnable\n");
                            eError = VIDENCTEST_HandlePortEnable(pAppData, nData2);
                            VIDENCTEST_CHECK_ERROR(eError, "Error at PortPortEnable function");
                            VIDENCTEST_PRINT("Exits to HandlePortEnable\n");
                            break;
                        case OMX_CommandMarkBuffer:
                            ;
                    }
                    break;
                case OMX_EventError:
                    eError = VIDENCTEST_HandleEventError(pAppData, nData1, nData2);
                    VIDENCTEST_CHECK_ERROR(eError, "Fatal EventError");
                    break;
                case OMX_EventMax:
                    VIDENCTEST_PRINT("OMX_EventMax recived, nothing to do\n");
                    break;
                case OMX_EventMark:
                    VIDENCTEST_PRINT("OMX_EventMark recived, nothing to do\n");
                    break;
                case OMX_EventPortSettingsChanged:
                    VIDENCTEST_PRINT("OMX_EventPortSettingsChanged recived, nothing to do\n");
                    break;
                case OMX_EventBufferFlag:
                    VIDENCTEST_PRINT("OMX_EventBufferFlag recived, nothing to do\n");
                    break;
                case OMX_EventResourcesAcquired:
                    VIDENCTEST_PRINT("OMX_EventResourcesAcquired recived, nothing to do\n");
                    break;
                case OMX_EventComponentResumed:
                    VIDENCTEST_PRINT("OMX_EventComponentResumed recived, nothing to do\n");
                    break;
                case OMX_EventDynamicResourcesAvailable:
                    VIDENCTEST_PRINT("OMX_EventDynamicResourcesAvailable recived, nothing to do\n");
                    break;
                case OMX_EventPortFormatDetected:
                    VIDENCTEST_PRINT("OMX_EventPortFormatDetected recived, nothing to do\n");
                    break;
                case OMX_EventKhronosExtensions:
                    VIDENCTEST_PRINT("OMX_EventKhronosExtensions recived, nothing to do\n");
                    break;
                case OMX_EventVendorStartUnused:
                    VIDENCTEST_PRINT("OMX_EventVendorStartUnused recived, nothing to do\n");
                    break;
                default:
                    VIDENCTEST_CHECK_ERROR(OMX_ErrorUndefined, "Error at EmptyThisBuffer function");
                    break;
            }

            VIDENCTEST_FREE(pEventPrivate, pListHead);
        }

        if(FD_ISSET(pAppData->IpBuf_Pipe[0], &rfds)){
            VIDENCTEST_PRINT("Input Pipe event receive\n");
            read(pAppData->IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
            pAppData->nInBufferCount++;

            if(pAppData->eCurrentState == VIDENCTEST_StateEncoding) {
                pBuffer->nFilledLen = VIDENCTEST_fill_data(pBuffer, pAppData->fIn , pAppData->pInPortDef->nBufferSize);

                if (pBuffer->nFlags == OMX_BUFFERFLAG_EOS && pBuffer->nFilledLen == 0) {
                    pAppData->eCurrentState = VIDENCTEST_StateStopping;
                    eError = pAppData->pComponent->EmptyThisBuffer(pHandle, pBuffer);
                    VIDENCTEST_CHECK_ERROR(eError, "Error at EmptyThisBuffer function");
                    pAppData->nInBufferCount--;
                }

                if(pAppData->eTypeOfTest != VIDENCTEST_FullRecord){
                    if(pAppData->nCurrentFrameIn == pAppData->nReferenceFrame){
                        pAppData->eCurrentState = VIDENCTEST_StateStopping;
                        if(pAppData->eTypeOfTest == VIDENCTEST_PauseResume) {
                            eError = pAppData->pComponent->EmptyThisBuffer(pHandle, pBuffer);
                            VIDENCTEST_CHECK_ERROR(eError, "Error at EmptyThisBuffer function");
                            pAppData->nInBufferCount--;
                            pAppData->nCurrentFrameIn++;
                        }
                    }
                }

                if(pAppData->eCurrentState == VIDENCTEST_StateEncoding){
                    if(pAppData->nQPIoF > 0) {
                        if(!(pAppData->nCurrentFrameIn % pAppData->nQPIoF)) {
                            pAppData->bForceIFrame = OMX_TRUE;

                            eError = OMX_GetExtensionIndex(pHandle,"OMX.TI.VideoEncode.Config.ForceIFrame", (OMX_INDEXTYPE*)(&(pAppData->nVideoEncodeCustomParamIndex)));
                            VIDENCTEST_CHECK_EXIT(eError, "Error in OMX_GetExtensionIndex function");
                            eError = OMX_SetConfig(pHandle, pAppData->nVideoEncodeCustomParamIndex, &(pAppData->bForceIFrame));
                            VIDENCTEST_CHECK_EXIT(eError, "Error at SetConfig for bForceIFrame");
                            eError = OMX_GetExtensionIndex(pHandle,"OMX.TI.VideoEncode.Config.QPI", (OMX_INDEXTYPE*)(&(pAppData->nVideoEncodeCustomParamIndex)));
                            VIDENCTEST_CHECK_EXIT(eError, "Error in OMX_GetExtensionIndex function");
                            eError = OMX_SetConfig(pHandle, pAppData->nVideoEncodeCustomParamIndex, &(pAppData->nQpI));
                            VIDENCTEST_CHECK_EXIT(eError, "Error at SetConfig for bForceIFrame");
                        }
                        else {
                            pAppData->bForceIFrame = OMX_FALSE;
                            eError = OMX_GetExtensionIndex(pHandle,"OMX.TI.VideoEncode.Config.ForceIFrame", (OMX_INDEXTYPE*)(&(pAppData->nVideoEncodeCustomParamIndex)));
                            VIDENCTEST_CHECK_EXIT(eError, "Error in OMX_GetExtensionIndex function");
                            eError = OMX_SetConfig(pHandle, pAppData->nVideoEncodeCustomParamIndex, &(pAppData->bForceIFrame));
                            VIDENCTEST_CHECK_EXIT(eError, "Error at SetConfig for bForceIFrame");
                        }
                    }
                    eError = pAppData->pComponent->EmptyThisBuffer(pHandle, pBuffer);
                    VIDENCTEST_CHECK_ERROR(eError, "Error at EmptyThisBuffer function");
                    pAppData->nInBufferCount--;
                    pAppData->nCurrentFrameIn++;
                }
            }

            if(pAppData->eCurrentState == VIDENCTEST_StateStopping) {
                if(pAppData->nInBufferCount == NUM_OF_IN_BUFFERS){
                    pAppData->eCurrentState = VIDENCTEST_StateConfirm;
                }
            }
        }

        if(FD_ISSET(pAppData->OpBuf_Pipe[0], &rfds)){
            VIDENCTEST_PRINT("Output Pipe event receive\n");
            read(pAppData->OpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
            pAppData->nOutBufferCount++;


            /* check is it is the last buffer */
            if((pBuffer->nFlags & OMX_BUFFERFLAG_EOS) ||
                (pAppData->eTypeOfTest != VIDENCTEST_FullRecord &&
                pAppData->nCurrentFrameIn == pAppData->nReferenceFrame)) {
                pAppData->bLastOutBuffer = 1;
            }

            /*App sends last buffer as null buffer, so buffer with EOS contains only garbage*/
            if(pBuffer->nFilledLen) {
                pAppData->nCurrentFrameOut++;
                fwrite(pBuffer->pBuffer, 1, pBuffer->nFilledLen, pAppData->fOut);
                nError = ferror(pAppData->fOut);
                if (nError != 0) {
                    VIDENCTEST_DPRINT("ERROR: writing to file\n");
                }
                nError = fflush(pAppData->fOut);
                if (nError != 0) {
                    VIDENCTEST_DPRINT("ERROR: flushing file\n");
                }

                if(pAppData->NalFormat == VIDENC_TEST_NAL_SLICE){
                    nNalSlices = 1;
                    fwrite(&nNalSlices, 1, sizeof(OMX_U32), pAppData->fNalnd);
                    fwrite(&(pBuffer->nFilledLen), 1, sizeof(OMX_U32), pAppData->fNalnd);
                    nError = ferror(pAppData->fNalnd);
                    if (nError != 0) {
                        VIDENCTEST_DPRINT("ERROR: writing to file\n");
                    }
                    nError = fflush(pAppData->fNalnd);
                    if (nError != 0) {
                        VIDENCTEST_DPRINT("ERROR: flushing file\n");
                    }
                }
                /* Check if it is Nal format and if it has extra data*/
                if((pAppData->NalFormat == VIDENC_TEST_NAL_FRAME) &&
                    (pBuffer->nFlags & OMX_BUFFERFLAG_EXTRADATA)){

                    pTemp = pBuffer->pBuffer + pBuffer->nOffset + pBuffer->nFilledLen + 3;
                    pExtraDataType = (OMX_OTHER_EXTRADATATYPE_1_1_2*) (((OMX_U32) pTemp) & ~3);
                    pIndexNal = (OMX_U32*)(pExtraDataType->data);

                    nNalSlices = *pIndexNal;
                    fwrite(pIndexNal, 1, sizeof(OMX_U32), pAppData->fNalnd);

                    while(nNalSlices--) {
                        pIndexNal++;
                        fwrite(pIndexNal, 1, sizeof(OMX_U32), pAppData->fNalnd);
                        nError = ferror(pAppData->fNalnd);
                        if (nError != 0) {
                            VIDENCTEST_DPRINT("ERROR: writing to file\n");
                        }
                        nError = fflush(pAppData->fNalnd);
                        if (nError != 0) {
                            VIDENCTEST_DPRINT("ERROR: flushing file\n");
                        }
                    }
                }
            }

            if(pAppData->eCurrentState == VIDENCTEST_StateEncoding ||
                pAppData->eCurrentState == VIDENCTEST_StateStopping) {
                pBuffer->nFilledLen = 0;
                eError = pAppData->pComponent->FillThisBuffer(pHandle, pBuffer);
                VIDENCTEST_CHECK_ERROR(eError, "Error at FillThisBuffer function");
                pAppData->nOutBufferCount--;
            }
        }

        if (pAppData->eCurrentState == VIDENCTEST_StateConfirm ) {
            if (pAppData->bLastOutBuffer){
                VIDENCTEST_PRINT("Number of Input  Buffer at Client Side : %i\n", pAppData->nInBufferCount);
                VIDENCTEST_PRINT("Number of Output Buffer at Client Side : %i\n", pAppData->nOutBufferCount);
                VIDENCTEST_PRINT("Frames Out: %i\n", pAppData->nCurrentFrameOut);
                VIDENCTEST_PRINT("Enter to Confirm Function\n");
                eError = VIDENCTEST_Confirm(pAppData);
                VIDENCTEST_CHECK_ERROR(eError, "Error at VIDENCTEST_Confirm function");
                VIDENCTEST_PRINT("Exits to Confirm Function\n");
            }
        }

        if (pAppData->eCurrentState == VIDENCTEST_StateUnLoad) {
            VIDENCTEST_PRINT("Exiting while\n");
            break;
        }
        sched_yield();
    }
    if(pAppData->nCurrentFrameIn != pAppData->nCurrentFrameOut)
    {
        printf("App: Warning!!! FrameIn: %d FrameOut: %d\n", (int)pAppData->nCurrentFrameIn, (int)pAppData->nCurrentFrameOut);
    }

    eError = VIDENCTEST_DeInit(pAppData);

EXIT:
    return eError;
}

#ifdef DSP_MMU_FAULT_HANDLING
int LoadBaseImage()
{
    unsigned int uProcId = 0;   /* default proc ID is 0. */
    unsigned int index = 0;

    struct DSP_PROCESSORINFO dspInfo;
    DSP_HPROCESSOR hProc;
    DSP_STATUS status = DSP_SOK;
    unsigned int numProcs;
    char* argv[2];

    argv[0] = "/lib/dsp/baseimage.dof";

    status = (DBAPI)DspManager_Open(0, NULL);
    if (DSP_FAILED(status)) {
        printf("DSPManager_Open failed \n");
        return -1;
    }
    while (DSP_SUCCEEDED(DSPManager_EnumProcessorInfo(index,&dspInfo,
        (unsigned int)sizeof(struct DSP_PROCESSORINFO),&numProcs))) {
        if ((dspInfo.uProcessorType == DSPTYPE_55) ||
            (dspInfo.uProcessorType == DSPTYPE_64)) {
            uProcId = index;
            status = DSP_SOK;
            break;
        }
        index++;
    }
    status = DSPProcessor_Attach(uProcId, NULL, &hProc);
    if (DSP_SUCCEEDED(status)) {
        status = DSPProcessor_Stop(hProc);
        if (DSP_SUCCEEDED(status)) {
            status = DSPProcessor_Load(hProc,1,(const char **)argv,NULL);
            if (DSP_SUCCEEDED(status)) {
                status = DSPProcessor_Start(hProc);
                if (DSP_SUCCEEDED(status)) {
                    fprintf(stderr,"Baseimage Loaded\n");
                }
                else {
                    fprintf(stderr,"APP: Baseimage start error!\n");
                }
            }
            else {
                fprintf(stderr,"APP: Baseimage load error!\n");
            }
            DSPProcessor_Detach(hProc);
        }
        else {
            fprintf(stderr,"APP: Baseimage stop error!\n");
        }
    }
    else {
        fprintf(stderr,"APP: Baseimage attach error!\n");
    }

    return 0;
}
#endif


