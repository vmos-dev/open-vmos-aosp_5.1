
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
* ============================================================================*/
/**
* @file OMX_VideoEnc_Utils.c
*
* This file implements OMX Component for MPEG-4 encoder that
* is fully compliant with the OMX specification 1.5.
*
* @path  $(CSLPATH)\src
*
* @rev  0.1
*/
/* ---------------------------------------------------------------------------*/
/* =============================================================================
*!
*! Revision History
*! =============================================================================
*!
*! 02-Feb-2006 mf: Revisions appear in reverse chronological order;
*! that is, newest first.  The date format is dd-Mon-yyyy.
* ============================================================================*/

/* ------compilation control switches ----------------------------------------*/
/******************************************************************************
*  INCLUDE FILES
*******************************************************************************/
/* ----- system and platform files -------------------------------------------*/
#ifdef UNDER_CE
    #include <windows.h>
    #include <omx_core.h>
    #include <stdlib.h>
    #include <pthread.h>
#else
    #include <wchar.h>
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <sys/types.h>
    #include <sys/select.h>
    #include <sys/stat.h>
    #include <dlfcn.h>
    #include <malloc.h>
    #include <memory.h>
    #include <fcntl.h>
#endif

#include <dbapi.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/*------- Program Header Files -----------------------------------------------*/
#include "OMX_VideoEnc_Utils.h"
#include "OMX_VideoEnc_Thread.h"
#include "OMX_VideoEnc_DSP.h"

#define DSP_MMU_FAULT_HANDLING

// We cannot request the same MHz for all resolutions.
// we have to change this implementation once we modify
// opencore to request the correct level based on resolution/bitrate/etc
#define VIDEO_ENCODER_MHZ (400 - 45 + 2) 

/* H264 Specific */
#define SPS_CODE_PREFIX 0x07
#define PPS_CODE_PREFIX 0x08

#ifdef UNDER_CE
    HINSTANCE g_hLcmlDllHandle = NULL;
#endif
#ifdef UNDER_CE
void sleep(DWORD Duration)
{
    Sleep(Duration);
}
#endif
/*******************************************************************************
*  EXTERNAL REFERENCES NOTE : only use if not found in header file
*******************************************************************************/
/*--------data declarations --------------------------------------------------*/
/*--------function prototypes ------------------------------------------------*/

/*******************************************************************************
*  PUBLIC DECLARATIONS Defined here, used elsewhere
*******************************************************************************/
/*--------data declarations --------------------------------------------------*/


/*--------function prototypes ------------------------------------------------*/

/*******************************************************************************
*  PRIVATE DECLARATIONS Defined here, used only here
*******************************************************************************/
/*--------data declarations --------------------------------------------------*/
struct DSP_UUID H264VESOCKET_TI_UUID = {
    0x63A3581A, 0x09D7, 0x4AD0, 0x80, 0xB8, {
    0x5F, 0x2C, 0x4D, 0x4D, 0x59, 0xC9
    }
};

struct DSP_UUID MP4VESOCKET_TI_UUID = {
    0x98c2e8d8, 0x4644, 0x11d6, 0x81, 0x18, {
        0x00, 0xb0, 0xd0, 0x8d, 0x72, 0x9f
    }
};

struct DSP_UUID USN_UUID = {
    0x79A3C8B3, 0x95F2, 0x403F, 0x9A, 0x4B, {
        0xCF, 0x80, 0x57, 0x73, 0x05, 0x41
    }
};

OMX_U32 VIDENC_STRUCT_H264DEFBITRATE [VIDENC_MAXBITRATES][2] = {
/*1*/    {176 * 144, 128000},     /*128KBps*/
/*2*/    {320 * 240, 400000},     /*400KBps*/
/*3*/    {352 * 288, 500000},     /*500kBps*/
/*4*/    {640 * 480, 1500000},    /*1.5MBps*/
/*5*/    {720 * 480, 2000000},    /*2MBps*/
/*6*/    {720 * 576, 3000000},    /*3MBps*/
/*7*/    {1280 * 720, 3000000},   /*3MBps*/
};

OMX_U32 VIDENC_STRUCT_MPEG4DEFBITRATE [VIDENC_MAXBITRATES][2] = {
/*1*/    {176 * 144, 128000},     /*128KBps*/
/*2*/    {320 * 240, 400000},     /*400KBps*/
/*3*/    {352 * 288, 500000},     /*500kBps*/
/*4*/    {640 * 480, 1500000},    /*1.5MBps*/
/*5*/    {720 * 480, 2000000},    /*2MBps*/
/*6*/    {720 * 576, 3000000},    /*3MBps*/
/*7*/    {1280 * 720, 3000000},   /*3MBps*/
};

OMX_U32 VIDENC_STRUCT_H263DEFBITRATE [VIDENC_MAXBITRATES][2] = {
/*1*/    {176 * 144, 128000},     /*128KBps*/
/*2*/    {320 * 240, 400000},     /*400KBps*/
/*3*/    {352 * 288, 500000},     /*500kBps*/
/*4*/    {640 * 480, 1500000},    /*1.5MBps*/
/*5*/    {720 * 480, 2000000},    /*2MBps*/
/*6*/    {720 * 576, 3000000},    /*3MBps*/
/*7*/    {1280 * 720, 3000000},   /*3MBps*/
};
/*--------macro definitions ---------------------------------------------------*/

static const int iQ16_Const = 1 << 16;
static const float fQ16_Const = (float)(1 << 16);

static float Q16Tof(int nQ16)
{
    return nQ16 / fQ16_Const;
}



/*-----------------------------------------------------------------------------*/
/**
  * ListCreate()
  *
  * Function call in OMX_ComponentInit(). Creates the List Head of the Component Memory List.
  *
  * @param pListHead VIDENC_NODE double pointer with the List Header of the Memory List.
  *
  * @retval OMX_ErrorNone
  *               OMX_ErrorInsufficientResources if the malloc fails
  *
  **/
/*-----------------------------------------------------------------------------*/
OMX_ERRORTYPE OMX_VIDENC_ListCreate(struct OMX_TI_Debug *dbg, struct VIDENC_NODE** pListHead)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    *pListHead = (VIDENC_NODE*)malloc(sizeof(VIDENC_NODE)); /* need to malloc!!! */
    if (*pListHead == NULL)
    {
        OMX_TRACE4(*dbg, "malloc() out of memory error\n");
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorInsufficientResources);
    }

    OMX_TRACE1(*dbg, "Create MemoryListHeader[%p]\n", *pListHead);
    memset(*pListHead, 0x0, sizeof(VIDENC_NODE));

OMX_CONF_CMD_BAIL:
    return eError;
}

/*-----------------------------------------------------------------------------*/
/**
  * ListAdd()
  *
  * Called inside VIDENC_MALLOC Macro to add a new node to Component Memory List
  *
  * @param pListHead VIDENC_NODE Points List Header of the Memory List.
  *                pData OMX_PTR points to the new allocated data.
  * @retval OMX_ErrorNone
  *               OMX_ErrorInsufficientResources if the malloc fails
  *
  **/
/*-----------------------------------------------------------------------------*/

OMX_ERRORTYPE OMX_VIDENC_ListAdd(struct OMX_TI_Debug *dbg, struct VIDENC_NODE* pListHead, OMX_PTR pData)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    VIDENC_NODE* pTmp = NULL;
    VIDENC_NODE* pNewNode = NULL;
    pNewNode = (VIDENC_NODE*)malloc(sizeof(VIDENC_NODE)); /* need to malloc!!! */
    if (pNewNode == NULL)
    {
        OMX_TRACE4(*dbg, "malloc() out of memory error\n");
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorInsufficientResources);
    }
    memset(pNewNode, 0x0, sizeof(VIDENC_NODE));
    pNewNode->pData = pData;
    pNewNode->pNext = NULL;
    OMX_TRACE1(*dbg, "Add MemoryNode[%p] -> [%p]\n", pNewNode, pNewNode->pData);
    pTmp = pListHead;

    while (pTmp->pNext != NULL)
    {
        pTmp = pTmp->pNext;
    }
    pTmp->pNext = pNewNode;

OMX_CONF_CMD_BAIL:
    return eError;
}

/*-----------------------------------------------------------------------------*/
/**
  * ListRemove()
  *
  * Called inside VIDENC_FREE Macro remove  node from Component Memory List and free the memory pointed by the node.
  *
  * @param pListHead VIDENC_NODE Points List Header of the Memory List.
  *                pData OMX_PTR points to the new allocated data.
  * @retval OMX_ErrorNone
  *
  *
  **/
/*-----------------------------------------------------------------------------*/

OMX_ERRORTYPE OMX_VIDENC_ListRemove(struct OMX_TI_Debug *dbg, struct VIDENC_NODE* pListHead,
                                    OMX_PTR pData)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    VIDENC_NODE* pNode = NULL;
    VIDENC_NODE* pTmp = NULL;

    pNode = pListHead;

    while (pNode->pNext != NULL)
    {
        if (pNode->pNext->pData == pData)
        {
            pTmp = pNode->pNext;
            pNode->pNext = pTmp->pNext;
            OMX_TRACE1(*dbg, "Remove MemoryNode[%p] -> [%p]\n", pTmp, pTmp->pData);
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
  * @param pListHead VIDENC_NODE Points List Header of the Memory List.
  *
  * @retval OMX_ErrorNone
  *
  *
  **/
/*-----------------------------------------------------------------------------*/

OMX_ERRORTYPE OMX_VIDENC_ListDestroy(struct OMX_TI_Debug *dbg, struct VIDENC_NODE* pListHead)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    VIDENC_NODE* pTmp = NULL;
    VIDENC_NODE* pNode = NULL;
    pNode = pListHead;

    while (pNode->pNext != NULL)
    {
        pTmp = pNode->pNext;
        pNode->pNext=pTmp->pNext;
        if (pTmp->pData != NULL)
        {
            OMX_TRACE0(*dbg, "Remove MemoryNode[%p] -> [%p]\n", pTmp, pTmp->pData);
            free(pTmp->pData);
            pTmp->pData = NULL;
        }
        free(pTmp);
        pTmp = NULL;
    }

    OMX_TRACE1(*dbg, "Destroy MemoryListHeader[%p]\n", pListHead);
    free(pListHead);
    return eError;
}



/*---------------------------------------------------------------------------------------*/
/**
  *  OMX_VIDENC_EmptyDataPipes Wait until all buffers are processed
  *
  * @param pComponentPrivate pointer to the private video encoder structure
  *
  * @return None
  *
 **/
/*---------------------------------------------------------------------------------------*/
void OMX_VIDENC_EmptyDataPipes (VIDENC_COMPONENT_PRIVATE *pComponentPrivate)
{
    pthread_mutex_lock(&bufferReturned_mutex);
    while (pComponentPrivate->EmptythisbufferCount != pComponentPrivate->EmptybufferdoneCount ||
           pComponentPrivate->FillthisbufferCount  != pComponentPrivate->FillbufferdoneCount) {
        pthread_cond_wait(&bufferReturned_condition, &bufferReturned_mutex);
    }
    pthread_mutex_unlock(&bufferReturned_mutex);
    ALOGI("Video encoder has returned all buffers");
}

void OMX_VIDENC_IncrementBufferCountByOne(OMX_U32 *count)
{
    pthread_mutex_lock(&bufferReturned_mutex);
    (*count)++;
    pthread_mutex_unlock(&bufferReturned_mutex);
}

void OMX_VIDENC_SignalIfAllBuffersAreReturned(VIDENC_COMPONENT_PRIVATE *pComponentPrivate)
{
    pthread_mutex_lock(&bufferReturned_mutex);
    if ((pComponentPrivate->EmptythisbufferCount == pComponentPrivate->EmptybufferdoneCount) &&
        (pComponentPrivate->FillthisbufferCount  == pComponentPrivate->FillbufferdoneCount)) {
        pthread_cond_broadcast(&bufferReturned_condition);
        ALOGI("Sending pthread signal that video encoder has returned all buffers to app");
    }
    pthread_mutex_unlock(&bufferReturned_mutex);
}

/*---------------------------------------------------------------------------------------*/
/**
  *  OMX_VIDENC_HandleError() will handle the error and pass the component to Invalid
  *  State, and send the event to the client.
  * @param eError - OMX_ERRORTYPE that occur.
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
 **/
/*---------------------------------------------------------------------------------------*/
OMX_ERRORTYPE OMX_VIDENC_HandleError(VIDENC_COMPONENT_PRIVATE* pComponentPrivate,
                                     OMX_ERRORTYPE eErrorCmp)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);

    pComponentPrivate->bHandlingFatalError = OMX_TRUE;

    OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                             OMX_EventError,
                             eErrorCmp,
                             OMX_TI_ErrorSevere,
                             NULL);

    switch (eErrorCmp)
    {
        case OMX_ErrorBadParameter:
        case OMX_ErrorPortUnresponsiveDuringAllocation:
        case OMX_ErrorUnsupportedIndex:
        case OMX_ErrorInsufficientResources:
            goto OMX_CONF_CMD_BAIL;
        default:
            ;
    }

    pComponentPrivate->bHideEvents = OMX_TRUE;

    eError = eErrorCmp;
    pComponentPrivate->eState = OMX_StateInvalid;

    OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                             OMX_EventError,
                             OMX_ErrorInvalidState,
                             OMX_TI_ErrorCritical,
                             NULL);

OMX_CONF_CMD_BAIL:
    if (pComponentPrivate)
        pComponentPrivate->bHandlingFatalError = OMX_FALSE;
    return eError;
}

/*---------------------------------------------------------------------------------------*/
/**
  *  OMX_VIDENC_HandleLcmlEvent() will handle the event from the LCML
 *  thread.
  * @param eError - OMX_ERRORTYPE that occur.
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
 **/
/*---------------------------------------------------------------------------------------*/
OMX_ERRORTYPE OMX_VIDENC_HandleLcmlEvent(VIDENC_COMPONENT_PRIVATE* pComponentPrivate,
                                         TUsnCodecEvent eEvent, void* argsCb [])
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);

    switch(eEvent)
    {
    case EMMCodecDspMessageRecieved:
        OMX_PRDSP1(pComponentPrivate->dbg, "EMMCodecDspMessageRecieved\n");
        break;
    case EMMCodecBufferProcessed:
        OMX_PRDSP1(pComponentPrivate->dbg, "EMMCodecBufferProcessed\n");
        break;
    case EMMCodecProcessingStarted:
        OMX_PRDSP1(pComponentPrivate->dbg, "EMMCodecProcessingStarted\n");
        break;
    case EMMCodecProcessingPaused:
        OMX_PRDSP1(pComponentPrivate->dbg, "EMMCodecProcessingPaused\n");
        break;
    case EMMCodecProcessingStoped:
        OMX_PRDSP1(pComponentPrivate->dbg, "EMMCodecProcessingStoped\n");
        break;
    case EMMCodecProcessingEof:
        OMX_PRDSP1(pComponentPrivate->dbg, "EMMCodecProcessingEof\n");
        break;
    case EMMCodecBufferNotProcessed:
        OMX_PRDSP1(pComponentPrivate->dbg, "EMMCodecBufferNotProcessed\n");
        break;
    case EMMCodecAlgCtrlAck:
        OMX_PRDSP1(pComponentPrivate->dbg, "EMMCodecAlgCtrlAck\n");
        break;
    case EMMCodecStrmCtrlAck:
        OMX_PRDSP1(pComponentPrivate->dbg, "EMMCodecStrmCtrlAck\n");
        break;
    case EMMCodecInternalError:
        OMX_PRDSP2(pComponentPrivate->dbg, "EMMCodecInternalError\n");
#ifdef DSP_MMU_FAULT_HANDLING
        if((argsCb[4] == (void *)USN_ERR_UNKNOWN_MSG) && (argsCb[5] == (void*)NULL))
        {
            OMX_VIDENC_SET_ERROR_BAIL(eError, OMX_ErrorInvalidState, pComponentPrivate);
        }
        else
        {
           OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                   OMX_EventError,
                                   OMX_ErrorHardware,
                                   OMX_TI_ErrorCritical,
                                   "Error Hardware\n");
            eError = OMX_ErrorHardware;
        }
#else
           OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                   OMX_EventError,
                                   OMX_ErrorHardware,
                                   OMX_TI_ErrorCritical,
                                   "Error Hardware\n");
           eError = OMX_ErrorHardware;
#endif
        break;
    case EMMCodecInitError:
        OMX_PRDSP2(pComponentPrivate->dbg, "EMMCodecInitError\n");
#ifdef DSP_MMU_FAULT_HANDLING
        if((argsCb[4] == (void *)USN_ERR_UNKNOWN_MSG) && (argsCb[5] == (void*)NULL))
        {
            OMX_VIDENC_SET_ERROR_BAIL(eError, OMX_ErrorInvalidState, pComponentPrivate);
        }
        else
        {
           OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                   OMX_EventError,
                                   OMX_ErrorHardware,
                                   OMX_TI_ErrorCritical,
                                   "Error Hardware\n");
            eError = OMX_ErrorHardware;
        }
#else
       OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                   OMX_EventError,
                                   OMX_ErrorHardware,
                                   OMX_TI_ErrorCritical,
                                   "Error Hardware\n");
        eError = OMX_ErrorHardware;
#endif
        break;
    case EMMCodecDspError:
        OMX_PRDSP2(pComponentPrivate->dbg, "EMMCodecDspError\n");
#ifdef DSP_MMU_FAULT_HANDLING
        if((argsCb[4] == (void *)NULL) && (argsCb[5] == (void*)NULL))
        {
            OMX_VIDENC_SET_ERROR_BAIL(eError, OMX_ErrorInvalidState, pComponentPrivate);
        }
        else
        {
       OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                   OMX_EventError,
                                   OMX_ErrorHardware,
                                   OMX_TI_ErrorCritical,
                                   "Error Hardware\n");
            eError = OMX_ErrorHardware;
        }
#else
       OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                   OMX_EventError,
                                   OMX_ErrorHardware,
                                   OMX_TI_ErrorCritical,
                                   "Error Hardware\n");
        eError = OMX_ErrorHardware;
#endif
        break;
    }

OMX_CONF_CMD_BAIL:
    return eError;
}

/*-----------------------------------------------------------------------------*/
/**
  * Disable Port()
  *
  * Called by component thread, handles commands sent by the app.
  *
  * @param
  *
  * @retval OMX_ErrorNone                  success, ready to roll
  *
  **/
/*-----------------------------------------------------------------------------*/
OMX_ERRORTYPE OMX_VIDENC_HandleCommandDisablePort (VIDENC_COMPONENT_PRIVATE* pComponentPrivate,
                                                   OMX_U32 nParam1)
{
    OMX_U8 i = 0;
    OMX_BOOL bFlushFlag;
    OMX_COMPONENTTYPE* pHandle = NULL;
    VIDENC_NODE* pMemoryListHead = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    VIDEOENC_PORT_TYPE* pCompPortIn = NULL;
    VIDEOENC_PORT_TYPE* pCompPortOut = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefIn = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefOut = NULL;

    OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);

    pHandle         = (OMX_COMPONENTTYPE*)pComponentPrivate->pHandle;
    pCompPortIn     = pComponentPrivate->pCompPort[VIDENC_INPUT_PORT];
    pCompPortOut    = pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT];
    pPortDefIn      = pComponentPrivate->pCompPort[VIDENC_INPUT_PORT]->pPortDef;
    pPortDefOut     = pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pPortDef;
    pMemoryListHead = pComponentPrivate->pMemoryListHead;

    OMX_DBG_CHECK_CMD(pComponentPrivate->dbg, pHandle, pPortDefIn, pPortDefOut);

    if (nParam1 == VIDENC_INPUT_PORT || nParam1 == (OMX_U32)-1)
    {
        /* Flush the DSP side before sending buffers back to the client */
        bFlushFlag = OMX_FALSE;
        for (i = 0; i < pPortDefIn->nBufferCountActual; i++)
        {
            if(pCompPortIn->pBufferPrivate[i]->eBufferOwner != VIDENC_BUFFER_WITH_CLIENT)
            {
                bFlushFlag = OMX_TRUE;
            }
        }

        if(bFlushFlag == OMX_TRUE)
        {
            eError = OMX_VIDENC_HandleCommandFlush(pComponentPrivate, VIDENC_INPUT_PORT, OMX_TRUE);
            OMX_DBG_BAIL_IF_ERROR(eError, pComponentPrivate->dbg, OMX_PRBUFFER3,
                                  "Flush command failed (%x)\n", eError);
            bFlushFlag = OMX_FALSE;
        }

        /*Return buffer to client*/
        if (pCompPortIn->hTunnelComponent == NULL)
        {
            for (i = 0; i < pPortDefIn->nBufferCountActual; i++)
            {
                if (pCompPortIn->pBufferPrivate[i]->eBufferOwner == VIDENC_BUFFER_WITH_COMPONENT ||
                    pCompPortIn->pBufferPrivate[i]->eBufferOwner == VIDENC_BUFFER_WITH_DSP)
                {
                    pCompPortIn->pBufferPrivate[i]->eBufferOwner = VIDENC_BUFFER_WITH_CLIENT;
#ifdef __PERF_INSTRUMENTATION__
                    PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                      PREF(pCompPortIn->pBufferPrivate[i]->pBufferHdr, pBuffer),
                                      0,
                                      PERF_ModuleHLMM);
#endif
                    pComponentPrivate->sCbData.EmptyBufferDone(pComponentPrivate->pHandle,
                                                               pComponentPrivate->pHandle->pApplicationPrivate,
                                                               pCompPortIn->pBufferPrivate[i]->pBufferHdr);
                    OMX_VIDENC_IncrementBufferCountByOne(&pComponentPrivate->EmptybufferdoneCount);
                    OMX_VIDENC_SignalIfAllBuffersAreReturned(pComponentPrivate);
                }
            }
        }
        else
        {
            for (i = 0; i < pPortDefIn->nBufferCountActual; i++)
            {
                if (pCompPortIn->pBufferPrivate[i]->eBufferOwner == VIDENC_BUFFER_WITH_COMPONENT ||
                    pCompPortIn->pBufferPrivate[i]->eBufferOwner == VIDENC_BUFFER_WITH_DSP)
                {
                    pCompPortIn->pBufferPrivate[i]->eBufferOwner = VIDENC_BUFFER_WITH_TUNNELEDCOMP;
#ifdef __PERF_INSTRUMENTATION__
                    PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                      PREF(pCompPortIn->pBufferPrivate[i]->pBufferHdr, pBuffer),
                                      0,
                                      PERF_ModuleLLMM);
#endif
                    eError = OMX_FillThisBuffer(pCompPortIn->hTunnelComponent,
                                                pCompPortIn->pBufferPrivate[i]->pBufferHdr);
                    OMX_DBG_BAIL_IF_ERROR(eError, pComponentPrivate->dbg, OMX_PRBUFFER4,
                                          "FillThisBuffer failed (%x)\n", eError);
                }
            }
        }
    }
    if (nParam1 == VIDENC_OUTPUT_PORT || nParam1 == (OMX_U32)-1)
    {
        /* Flush the DSP side before sending buffers back to the client */
        bFlushFlag = OMX_FALSE;
        for (i = 0; i < pPortDefOut->nBufferCountActual; i++)
        {
            if(pCompPortOut->pBufferPrivate[i]->eBufferOwner != VIDENC_BUFFER_WITH_CLIENT)
            {
                bFlushFlag = OMX_TRUE;
            }
        }

        if(bFlushFlag == OMX_TRUE)
        {
            eError = OMX_VIDENC_HandleCommandFlush(pComponentPrivate, VIDENC_OUTPUT_PORT, OMX_TRUE);
            OMX_DBG_BAIL_IF_ERROR(eError, pComponentPrivate->dbg, OMX_PRBUFFER3,
                                  "Flush command failed (%x)\n", eError);
            bFlushFlag = OMX_FALSE;
        }

        /*Return buffer to client*/
        if (pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->hTunnelComponent == NULL)
        {
            for (i = 0; i < pPortDefOut->nBufferCountActual; i++)
            {
                if (pCompPortOut->pBufferPrivate[i]->eBufferOwner == VIDENC_BUFFER_WITH_COMPONENT ||
                    pCompPortOut->pBufferPrivate[i]->eBufferOwner == VIDENC_BUFFER_WITH_DSP)
                {

                    pCompPortOut->pBufferPrivate[i]->eBufferOwner = VIDENC_BUFFER_WITH_CLIENT;
#ifdef __PERF_INSTRUMENTATION__
                    PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                      pCompPortOut->pBufferPrivate[i]->pBufferHdr ?
                                      pCompPortOut->pBufferPrivate[i]->pBufferHdr->pBuffer :
                                      NULL,
                                      pCompPortOut->pBufferPrivate[i]->pBufferHdr ?
                                      pCompPortOut->pBufferPrivate[i]->pBufferHdr->nFilledLen :
                                      0,
                                      PERF_ModuleHLMM);
#endif
                    OMX_CONF_CIRCULAR_BUFFER_MOVE_HEAD(pCompPortOut->pBufferPrivate[i]->pBufferHdr,
                                                       pComponentPrivate->sCircularBuffer,
                                                       pComponentPrivate);
                    /* trigger event handler if we are supposed to */
                    if (pCompPortOut->pBufferPrivate[i]->pBufferHdr->hMarkTargetComponent == pComponentPrivate->pHandle &&
                        pCompPortOut->pBufferPrivate[i]->pBufferHdr->pMarkData)
                    {
                        OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                                 OMX_EventMark,
                                                 0x0,
                                                 0x0,
                                                 pCompPortOut->pBufferPrivate[i]->pBufferHdr->pMarkData);
                    }
                    pComponentPrivate->sCbData.FillBufferDone(pComponentPrivate->pHandle,
                                                              pComponentPrivate->pHandle->pApplicationPrivate,
                                                              pCompPortOut->pBufferPrivate[i]->pBufferHdr);
                    OMX_VIDENC_IncrementBufferCountByOne(&pComponentPrivate->FillbufferdoneCount);
                    OMX_VIDENC_SignalIfAllBuffersAreReturned(pComponentPrivate);
                }
            }
        }
        else
        {
         /* If tunneled with VPP  - NOT Implemented*/
#ifdef __PERF_INSTRUMENTATION__
                PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                  PREF(pCompPortOut->pBufferPrivate[i]->pBufferHdr,pBuffer),
                                  PREF(pCompPortOut->pBufferPrivate[i]->pBufferHdr,nFilledLen),
                                  PERF_ModuleLLMM);
#endif
        }
    }

    OMX_PRBUFFER2(pComponentPrivate->dbg, "Flushing Pipes!\n");
    OMX_VIDENC_EmptyDataPipes (pComponentPrivate);

    /*while (1)
    {*/

        if (nParam1 == VIDENC_INPUT_PORT)
        {
            while ((pPortDefIn->bPopulated))
            {
                /*Send event*/
#ifndef UNDER_CE
    pthread_mutex_lock(&pComponentPrivate->videoe_mutex_app);
    pthread_cond_wait(&pComponentPrivate->unpopulate_cond, &pComponentPrivate->videoe_mutex_app);
    pthread_mutex_unlock(&pComponentPrivate->videoe_mutex_app);
#else
        OMX_WaitForEvent(&(pComponentPrivate->InIdle_event));
#endif
        break;
                }
                OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                             OMX_EventCmdComplete,
                             OMX_CommandPortDisable,
                             VIDENC_INPUT_PORT,
                             NULL);

            }

        else if (nParam1 == VIDENC_OUTPUT_PORT)
        {
            while ((pPortDefOut->bPopulated))
            {
                /*Send event*/
#ifndef UNDER_CE
    pthread_mutex_lock(&pComponentPrivate->videoe_mutex_app);
    pthread_cond_wait(&pComponentPrivate->unpopulate_cond, &pComponentPrivate->videoe_mutex_app);
    pthread_mutex_unlock(&pComponentPrivate->videoe_mutex_app);
#else
        OMX_WaitForEvent(&(pComponentPrivate->InIdle_event));
#endif
        break;
                }
        OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                             OMX_EventCmdComplete,
                             OMX_CommandPortDisable,
                             VIDENC_OUTPUT_PORT,
                             NULL);

            }

        else if (nParam1 == (OMX_U32)-1)
        {
            while ((pPortDefIn->bPopulated) || (pPortDefOut->bPopulated))
            {
        /*Send events*/
#ifndef UNDER_CE
    pthread_mutex_lock(&pComponentPrivate->videoe_mutex_app);
    pthread_cond_wait(&pComponentPrivate->unpopulate_cond, &pComponentPrivate->videoe_mutex_app);
    pthread_mutex_unlock(&pComponentPrivate->videoe_mutex_app);
#else
        OMX_WaitForEvent(&(pComponentPrivate->InIdle_event));
#endif
        break;
                }
        OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                             OMX_EventCmdComplete,
                             OMX_CommandPortDisable,
                             VIDENC_INPUT_PORT,
                             NULL);
                OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                             OMX_EventCmdComplete,
                             OMX_CommandPortDisable,
                             VIDENC_OUTPUT_PORT,
                             NULL);


        }


OMX_CONF_CMD_BAIL:
    return eError;
}

/*-----------------------------------------------------------------------------*/
/**
  * Enable Port()
  *
  * Called by component thread, handles commands sent by the app.
  *
  * @param
  *
  * @retval OMX_ErrorNone                  success, ready to roll
  *
  **/
/*-----------------------------------------------------------------------------*/

OMX_ERRORTYPE OMX_VIDENC_HandleCommandEnablePort (VIDENC_COMPONENT_PRIVATE* pComponentPrivate,
                                                  OMX_U32 nParam1)
{
    OMX_U32 nTimeout = 0x0;
    OMX_COMPONENTTYPE* pHandle = NULL;
    VIDENC_NODE* pMemoryListHead = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefIn = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefOut = NULL;

    OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);

    pHandle = (OMX_COMPONENTTYPE*)pComponentPrivate->pHandle;
    pPortDefIn      = pComponentPrivate->pCompPort[VIDENC_INPUT_PORT]->pPortDef;
    pPortDefOut     = pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pPortDef;
    pMemoryListHead = pComponentPrivate->pMemoryListHead;

    nTimeout = 0x0;
    /*while(1)
    {*/
        if(nParam1 == VIDENC_INPUT_PORT)
        {
        if (pComponentPrivate->eState != OMX_StateLoaded)
        {
        pthread_mutex_lock(&pComponentPrivate->videoe_mutex_app);
            while (!pPortDefIn->bPopulated)
               {
#ifndef UNDER_CE
        pthread_cond_wait(&pComponentPrivate->populate_cond, &pComponentPrivate->videoe_mutex_app);
#else
        OMX_WaitForEvent(&(pComponentPrivate->InIdle_event));
#endif

                }
    }
    pthread_mutex_unlock(&pComponentPrivate->videoe_mutex_app);
        OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                             OMX_EventCmdComplete,
                                             OMX_CommandPortEnable,
                                             VIDENC_INPUT_PORT,
                                             NULL);


        }
        else if(nParam1 == VIDENC_OUTPUT_PORT)
        {
        if (pComponentPrivate->eState != OMX_StateLoaded)
        {
            pthread_mutex_lock(&pComponentPrivate->videoe_mutex_app);
            while(!pPortDefOut->bPopulated)
            {
#ifndef UNDER_CE
        pthread_cond_wait(&pComponentPrivate->populate_cond, &pComponentPrivate->videoe_mutex_app);
#else
        OMX_WaitForEvent(&(pComponentPrivate->InIdle_event));
#endif

                }
        }
            pthread_mutex_unlock(&pComponentPrivate->videoe_mutex_app);
                OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                         OMX_EventCmdComplete,
                                         OMX_CommandPortEnable,
                                         VIDENC_OUTPUT_PORT,
                                         NULL);


        }
        else if(nParam1 == (OMX_U32)-1)
        {
        if (pComponentPrivate->eState != OMX_StateLoaded)
        {
            pthread_mutex_lock(&pComponentPrivate->videoe_mutex_app);
            while(!pPortDefOut->bPopulated && !pPortDefIn->bPopulated)
            {
#ifndef UNDER_CE
            pthread_cond_wait(&pComponentPrivate->populate_cond, &pComponentPrivate->videoe_mutex_app);
#else
        OMX_WaitForEvent(&(pComponentPrivate->InIdle_event));
#endif
      break;
                }
        }
            pthread_mutex_unlock(&pComponentPrivate->videoe_mutex_app);
                OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                         OMX_EventCmdComplete,
                                         OMX_CommandPortEnable,
                                         VIDENC_INPUT_PORT,
                                         NULL);
                OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                        OMX_EventCmdComplete,
                                        OMX_CommandPortEnable,
                                        VIDENC_OUTPUT_PORT,
                                        NULL);


        }


OMX_CONF_CMD_BAIL:
    return eError;
}

/*----------------------------------------------------------------------------*/
/**
  * OMX_OMX_VIDENC_HandleCommandFlush()
  *
  * Called by component thread, handles commands sent by the app.
  *
  * @param phandle LCML_DSP_INTERFACE handle for this instance of the component
  *
  * @retval OMX_ErrorNone                  success, ready to roll
  *         OMX_ErrorInsufficientResources if the malloc fails
  **/
/*----------------------------------------------------------------------------*/

OMX_ERRORTYPE OMX_VIDENC_HandleCommandFlush(VIDENC_COMPONENT_PRIVATE* pComponentPrivate,
                                            OMX_U32 nParam1,
                                            OMX_BOOL bInternalFlush)
{
    OMX_U16 i = 0;
    OMX_U32 aParam[3] = {0};
    OMX_COMPONENTTYPE* pHandle = NULL;
    VIDENC_NODE* pMemoryListHead = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    LCML_DSP_INTERFACE* pLcmlHandle = NULL;
    VIDEOENC_PORT_TYPE* pCompPortIn = NULL;
    VIDEOENC_PORT_TYPE* pCompPortOut = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefIn = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefOut = NULL;

    OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);

    pLcmlHandle     = (LCML_DSP_INTERFACE*)pComponentPrivate->pLCML;
    pCompPortIn     = pComponentPrivate->pCompPort[VIDENC_INPUT_PORT];
    pCompPortOut    = pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT];
    pPortDefIn      = pComponentPrivate->pCompPort[VIDENC_INPUT_PORT]->pPortDef;
    pPortDefOut     = pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pPortDef;
    pHandle         = (OMX_COMPONENTTYPE*)pComponentPrivate->pHandle;
    pMemoryListHead = pComponentPrivate->pMemoryListHead;

    if (nParam1 == VIDENC_INPUT_PORT || nParam1 == (OMX_U32)-1)
    {
        aParam[0] = USN_STRMCMD_FLUSH;
        aParam[1] = VIDENC_INPUT_PORT;
        aParam[2] = 0x0;

        eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                    EMMCodecControlStrmCtrl,
                                    (void*)aParam);
        OMX_DBG_BAIL_IF_ERROR(eError, pComponentPrivate->dbg, OMX_PRDSP4,
                              "DSP Input flush failed (%x).\n", eError);

#ifndef UNDER_CE
        pthread_mutex_lock(&pComponentPrivate->videoe_mutex_app);
        while (pComponentPrivate->bFlushComplete == OMX_FALSE)
        {
            pthread_cond_wait(&pComponentPrivate->flush_cond, &pComponentPrivate->videoe_mutex_app);
        }
        pthread_mutex_unlock(&pComponentPrivate->videoe_mutex_app);
#else
        while (pComponentPrivate->bFlushComplete == OMX_FALSE)
        {
            sched_yield();
        }
#endif

        pComponentPrivate->bFlushComplete = OMX_FALSE;

        if (pComponentPrivate->pCompPort[VIDENC_INPUT_PORT]->hTunnelComponent == NULL)
        {
            for (i = 0; i < pPortDefIn->nBufferCountActual; i++)
            {
                if (pCompPortIn->pBufferPrivate[i]->eBufferOwner == VIDENC_BUFFER_WITH_COMPONENT ||
                    pCompPortIn->pBufferPrivate[i]->eBufferOwner == VIDENC_BUFFER_WITH_DSP)
                {

                    pCompPortIn->pBufferPrivate[i]->eBufferOwner = VIDENC_BUFFER_WITH_CLIENT;
#ifdef __PERF_INSTRUMENTATION__
                    PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                      PREF(pCompPortIn->pBufferPrivate[i]->pBufferHdr,pBuffer),
                                      0,
                                      PERF_ModuleHLMM);
#endif
                    pComponentPrivate->sCbData.EmptyBufferDone(pComponentPrivate->pHandle,
                                                               pComponentPrivate->pHandle->pApplicationPrivate,
                                                               pCompPortIn->pBufferPrivate[i]->pBufferHdr);
                    OMX_VIDENC_IncrementBufferCountByOne(&pComponentPrivate->EmptybufferdoneCount);
                    OMX_VIDENC_SignalIfAllBuffersAreReturned(pComponentPrivate);
                }
            }
        }
        else
        {
            for (i = 0; i < pPortDefIn->nBufferCountActual; i++)
            {
                if (pCompPortIn->pBufferPrivate[i]->eBufferOwner == VIDENC_BUFFER_WITH_COMPONENT ||
                    pCompPortIn->pBufferPrivate[i]->eBufferOwner == VIDENC_BUFFER_WITH_DSP)
                {

                    pCompPortIn->pBufferPrivate[i]->eBufferOwner = VIDENC_BUFFER_WITH_TUNNELEDCOMP;
#ifdef __PERF_INSTRUMENTATION__
                    PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                      PREF(pCompPortOut->pBufferPrivate[i]->pBufferHdr,pBuffer),
                                      0,
                                      PERF_ModuleLLMM);
#endif
                    eError = OMX_FillThisBuffer(pCompPortIn->hTunnelComponent,
                                                pCompPortIn->pBufferPrivate[i]->pBufferHdr);
                    OMX_DBG_BAIL_IF_ERROR(eError, pComponentPrivate->dbg, OMX_PRBUFFER4,
                                          "FillThisBuffer failed (%x)\n", eError);
                }
            }
        }
        if (bInternalFlush == OMX_FALSE)
        {
            OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                     OMX_EventCmdComplete,
                                     OMX_CommandFlush,
                                     VIDENC_INPUT_PORT,
                                     NULL);
        }
    }
    if (nParam1 == VIDENC_OUTPUT_PORT || nParam1 == (OMX_U32)-1)
    {
        aParam[0] = USN_STRMCMD_FLUSH;
        aParam[1] = VIDENC_OUTPUT_PORT;
        aParam[2] = 0x0;

        eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                   EMMCodecControlStrmCtrl,
                                   (void*)aParam);
        OMX_DBG_BAIL_IF_ERROR(eError, pComponentPrivate->dbg, OMX_PRDSP4,
                              "DSP Output flush failed (%x).\n", eError);
#ifndef UNDER_CE
        pthread_mutex_lock(&pComponentPrivate->videoe_mutex_app);
        while (pComponentPrivate->bFlushComplete == OMX_FALSE)
        {
            pthread_cond_wait(&pComponentPrivate->flush_cond, &pComponentPrivate->videoe_mutex_app);
        }
        pthread_mutex_unlock(&pComponentPrivate->videoe_mutex_app);
#else
        while (pComponentPrivate->bFlushComplete == OMX_FALSE)
        {
            sched_yield();
        }
#endif

        pComponentPrivate->bFlushComplete = OMX_FALSE;

        if (pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->hTunnelComponent == NULL)
        {
            for (i = 0; i < pPortDefOut->nBufferCountActual; i++)
            {
                if (pCompPortOut->pBufferPrivate[i]->eBufferOwner == VIDENC_BUFFER_WITH_COMPONENT ||
                    pCompPortOut->pBufferPrivate[i]->eBufferOwner == VIDENC_BUFFER_WITH_DSP)
                {

                    pCompPortOut->pBufferPrivate[i]->eBufferOwner = VIDENC_BUFFER_WITH_CLIENT;
#ifdef __PERF_INSTRUMENTATION__
                    PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                      PREF(pCompPortOut->pBufferPrivate[i]->pBufferHdr,pBuffer),
                                      PREF(pCompPortOut->pBufferPrivate[i]->pBufferHdr,nFilledLen),
                                      PERF_ModuleHLMM);
#endif
                    /*Copy Buffer Data to be propagated*/
                    OMX_CONF_CIRCULAR_BUFFER_MOVE_HEAD(pCompPortOut->pBufferPrivate[i]->pBufferHdr,
                                                       pComponentPrivate->sCircularBuffer,
                                                       pComponentPrivate);
                     /* trigger event handler if we are supposed to */
                    if (pCompPortOut->pBufferPrivate[i]->pBufferHdr->hMarkTargetComponent == pComponentPrivate->pHandle &&
                        pCompPortOut->pBufferPrivate[i]->pBufferHdr->pMarkData)
                    {
                        OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                                 OMX_EventMark,
                                                 0x0,
                                                 0x0,
                                                 pCompPortOut->pBufferPrivate[i]->pBufferHdr->pMarkData);
                    }
                    pComponentPrivate->sCbData.FillBufferDone(pComponentPrivate->pHandle,
                                                              pComponentPrivate->pHandle->pApplicationPrivate,
                                                              pCompPortOut->pBufferPrivate[i]->pBufferHdr);
                    OMX_VIDENC_IncrementBufferCountByOne(&pComponentPrivate->FillbufferdoneCount);
                    OMX_VIDENC_SignalIfAllBuffersAreReturned(pComponentPrivate);
                }
            }
        }
        else
        {
            /* If tunneled with VPP  - NOT Implemented*/
        }
        if (bInternalFlush == OMX_FALSE)
        {
            OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                     OMX_EventCmdComplete,
                                     OMX_CommandFlush,
                                     VIDENC_OUTPUT_PORT,
                                     NULL);
        }
    }

OMX_CONF_CMD_BAIL:
    return eError;
}

/*----------------------------------------------------------------------------*/
/**
  * OMX_VIDENC_HandleCommand()
  *
  * Called by component thread, handles commands sent by the app.
  *
  * @param phandle LCML_DSP_INTERFACE handle for this instance of the component
  *
  * @retval OMX_ErrorNone                  success, ready to roll
  *         OMX_ErrorInsufficientResources if the malloc fails
  **/
/*----------------------------------------------------------------------------*/
OMX_ERRORTYPE OMX_VIDENC_HandleCommandStateSet (VIDENC_COMPONENT_PRIVATE* pComponentPrivate,
                                                OMX_U32 nParam1)
{
    VIDENC_NODE* pMemoryListHead = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefIn = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefOut = NULL;

    OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);

    pPortDefIn = pComponentPrivate->pCompPort[VIDENC_INPUT_PORT]->pPortDef;
    pPortDefOut = pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pPortDef;
    pMemoryListHead = pComponentPrivate->pMemoryListHead;

    switch (nParam1)
    {
    case OMX_StateIdle:
        eError = OMX_VIDENC_HandleCommandStateSetIdle (pComponentPrivate);
        OMX_DBG_BAIL_IF_ERROR(eError, pComponentPrivate->dbg, OMX_PRSTATE3,
                              "Failed to move to Idle state (%x).\n", eError);
        break;
    case OMX_StateExecuting:
        eError = OMX_VIDENC_HandleCommandStateSetExecuting (pComponentPrivate);
        OMX_DBG_BAIL_IF_ERROR(eError, pComponentPrivate->dbg, OMX_PRSTATE3,
                              "Failed to move to Execute state (%x).\n", eError);
        break;
    case OMX_StateLoaded:
        eError = OMX_VIDENC_HandleCommandStateSetLoaded (pComponentPrivate);
        OMX_DBG_BAIL_IF_ERROR(eError, pComponentPrivate->dbg, OMX_PRSTATE3,
                              "Failed to move to Loaded state (%x).\n", eError);
        break;
    case OMX_StatePause:
        eError = OMX_VIDENC_HandleCommandStateSetPause (pComponentPrivate);
        OMX_DBG_BAIL_IF_ERROR(eError, pComponentPrivate->dbg, OMX_PRSTATE3,
                              "Failed to move to Pause state (%x).\n", eError);
        break;
    case OMX_StateInvalid:
            if (pComponentPrivate->eState == OMX_StateInvalid)
            {
                OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                         OMX_EventError,
                                         OMX_ErrorSameState,
                                         OMX_TI_ErrorSevere,
                                         NULL);
            }
            else
            {
                pComponentPrivate->eState = OMX_StateInvalid;
                OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                         OMX_EventError,
                                         OMX_ErrorInvalidState,
                                         OMX_TI_ErrorSevere,
                                         NULL);
                OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                         OMX_EventCmdComplete,
                                         OMX_CommandStateSet,
                                         OMX_StateInvalid,
                                         NULL);
            }
            break;
        case OMX_StateWaitForResources:
            if (pComponentPrivate->eState == OMX_StateWaitForResources)
            {
                OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                         OMX_EventError,
                                         OMX_ErrorSameState,
                                         OMX_TI_ErrorMinor,
                                         NULL);
            }
            else if (pComponentPrivate->eState == OMX_StateLoaded)
            {
                pComponentPrivate->eState = OMX_StateWaitForResources;
                OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                         OMX_EventCmdComplete,
                                         OMX_CommandStateSet,
                                         OMX_StateWaitForResources,
                                         NULL);
            }
            else
            {
                OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                         OMX_EventError,
                                         OMX_ErrorIncorrectStateTransition,
                                         OMX_TI_ErrorMinor,
                                         NULL);
            }
            break;
        case OMX_StateMax:
            break;
    } /* End of Switch */

OMX_CONF_CMD_BAIL:
    return eError;
}

/*----------------------------------------------------------------------------*/
/**
  * OMX_VIDENC_HandleCommandStateSet()
  *
  * Called by component thread, handles commands sent by the app.
  *
  * @param phandle LCML_DSP_INTERFACE handle for this instance of the component
  *
  * @retval OMX_ErrorNone                  success, ready to roll
  *         OMX_ErrorInsufficientResources if the malloc fails
  **/
/*----------------------------------------------------------------------------*/
OMX_ERRORTYPE OMX_VIDENC_HandleCommandStateSetIdle(VIDENC_COMPONENT_PRIVATE* pComponentPrivate)
{
    OMX_U8 nCount = 0;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    LCML_DSP_INTERFACE* pLcmlHandle = NULL;
    VIDEOENC_PORT_TYPE* pCompPortIn = NULL;
    VIDEOENC_PORT_TYPE* pCompPortOut = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefIn = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefOut = NULL;


    OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);

    pCompPortIn = pComponentPrivate->pCompPort[VIDENC_INPUT_PORT];
    pCompPortOut = pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT];
    pPortDefIn = pComponentPrivate->pCompPort[VIDENC_INPUT_PORT]->pPortDef;
    pPortDefOut = pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pPortDef;


    switch (pComponentPrivate->eState)
    {
        case OMX_StateIdle:
            OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                     OMX_EventError,
                                     OMX_ErrorSameState,
                                     OMX_TI_ErrorMinor,
                                     NULL);
            break;
        case OMX_StateInvalid:
            OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                     OMX_EventError,
                                     OMX_ErrorIncorrectStateTransition,
                                     OMX_TI_ErrorMajor,
                                     NULL);
            break;
        case OMX_StateLoaded:
        case OMX_StateWaitForResources:
#ifdef __PERF_INSTRUMENTATION__
                PERF_Boundary(pComponentPrivate->pPERFcomp,
                              PERF_BoundaryStart | PERF_BoundarySetup);
#endif
        if ( pPortDefIn->bEnabled == OMX_TRUE || pPortDefOut->bEnabled == OMX_TRUE )
        {
            pthread_mutex_lock(&pComponentPrivate->videoe_mutex_app);
            while ( (!pPortDefIn->bPopulated) || (!pPortDefOut->bPopulated))
            {
#ifndef UNDER_CE
                pthread_cond_wait(&pComponentPrivate->populate_cond, &pComponentPrivate->videoe_mutex_app);
#else
            OMX_WaitForEvent(&(pComponentPrivate->InLoaded_event));
#endif
            }
            pthread_mutex_unlock(&pComponentPrivate->videoe_mutex_app);
        }
            /* Make sure the DSP node has been deleted first to cover
                   any idle->loaded->idle or idle->wfr->idle combinations
                   */
            if (pComponentPrivate->bCodecStarted == OMX_TRUE ||
                pComponentPrivate->bCodecLoaded == OMX_TRUE)
            {
            OMX_PRDSP2(pComponentPrivate->dbg, "Attempting to destroy the node...\n");
                pLcmlHandle = NULL;
                pLcmlHandle = (LCML_DSP_INTERFACE*)pComponentPrivate->pLCML;
                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                           EMMCodecControlDestroy,
                                           NULL);
            OMX_DBG_BAIL_IF_ERROR(eError, pComponentPrivate->dbg, OMX_PRDSP3,
                                  "Failed to destroy socket node (%x).\n", eError);


                /*Unload LCML */
                if(pComponentPrivate->pModLcml != NULL)
                {
#ifndef UNDER_CE
                    dlclose(pComponentPrivate->pModLcml);
#else
                    FreeLibrary(pComponentPrivate->pModLcml);
                FreeLibrary(g_hLcmlDllHandle);
                g_hLcmlDllHandle = NULL;

#endif
                   pComponentPrivate->pModLcml = NULL;
                   pComponentPrivate->pLCML = NULL;
                }

                if (pComponentPrivate->sps) {
                    free(pComponentPrivate->sps);
                    pComponentPrivate->sps = NULL;
                    pComponentPrivate->spsLen = 0;
                }

                pComponentPrivate->bCodecStarted = OMX_FALSE;
                pComponentPrivate->bCodecLoaded = OMX_FALSE;
            }

            eError = OMX_VIDENC_InitLCML(pComponentPrivate);
        OMX_DBG_BAIL_IF_ERROR(eError, pComponentPrivate->dbg, OMX_PRDSP4,
                              "Failed to initialize LCML (%x).\n", eError);

#ifdef __PERF_INSTRUMENTATION__
            pComponentPrivate->nLcml_nCntIp = 0;
            pComponentPrivate->nLcml_nCntOpReceived = 0;
#endif
            if (pPortDefOut->format.video.eCompressionFormat == OMX_VIDEO_CodingAVC)
            {
                eError = OMX_VIDENC_InitDSP_H264Enc(pComponentPrivate);
            OMX_DBG_BAIL_IF_ERROR(eError, pComponentPrivate->dbg, OMX_PRDSP4,
                                  "Failed to initialize H264 SN (%x).\n", eError);
            }
            else if (pPortDefOut->format.video.eCompressionFormat == OMX_VIDEO_CodingMPEG4 ||
                     pPortDefOut->format.video.eCompressionFormat == OMX_VIDEO_CodingH263)
            {
                eError = OMX_VIDENC_InitDSP_Mpeg4Enc(pComponentPrivate);
            OMX_DBG_BAIL_IF_ERROR(eError, pComponentPrivate->dbg, OMX_PRDSP4,
                                  "Failed to initialize MPEG4 SN (%x).\n", eError);
            }
            else
            {
            OMX_PRSTATE4(pComponentPrivate->dbg, "Unsupported compression format (%d)\n",
                         pPortDefOut->format.video.eCompressionFormat);
                OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorUnsupportedSetting);
            }

#ifdef RESOURCE_MANAGER_ENABLED

        OMX_PRMGR2(pComponentPrivate->dbg, "Setting CallBack In Video Encoder component\n");
             pComponentPrivate->cRMCallBack.RMPROXY_Callback = (void*)OMX_VIDENC_ResourceManagerCallBack;
            switch (pPortDefOut->format.video.eCompressionFormat)
            {
                case OMX_VIDEO_CodingAVC:
                     switch(pComponentPrivate->pH264->eLevel)
                     {
                        case OMX_VIDEO_AVCLevel1:
                        case OMX_VIDEO_AVCLevel1b:
                            eError = RMProxy_NewSendCommand(pComponentPrivate->pHandle,
                                                         RMProxy_RequestResource,
                                                         OMX_H264_Encode_COMPONENT,
                                                         VIDEO_ENCODER_MHZ,
                                                         3456,
                                                         &(pComponentPrivate->cRMCallBack));

                            break;
                        case OMX_VIDEO_AVCLevel11:
                        case OMX_VIDEO_AVCLevel12:
                            eError = RMProxy_NewSendCommand(pComponentPrivate->pHandle,
                                                         RMProxy_RequestResource,
                                                         OMX_H264_Encode_COMPONENT,
                                                         VIDEO_ENCODER_MHZ,
                                                         3456,
                                                         &(pComponentPrivate->cRMCallBack));
                            break;
                        case OMX_VIDEO_AVCLevel13:
                        case OMX_VIDEO_AVCLevel2:
                        case OMX_VIDEO_AVCLevel21:
                        case OMX_VIDEO_AVCLevel22:
                        case OMX_VIDEO_AVCLevel3:
                         default:
                            eError = RMProxy_NewSendCommand(pComponentPrivate->pHandle,
                                                            RMProxy_RequestResource,
                                                            OMX_H264_Encode_COMPONENT,
                                                            VIDEO_ENCODER_MHZ,
                                                            3456,
                                                            &(pComponentPrivate->cRMCallBack));
                    }
                    break;
                case OMX_VIDEO_CodingMPEG4:
                    switch(pComponentPrivate->pMpeg4->eLevel)
                    {
                        case 0:
                        case 1:
                        case 100:
                            eError = RMProxy_NewSendCommand(pComponentPrivate->pHandle,
                                                         RMProxy_RequestResource,
                                                         OMX_MPEG4_Encode_COMPONENT,
                                                         VIDEO_ENCODER_MHZ,
                                                         3456,
                                                         &(pComponentPrivate->cRMCallBack));
                            break;
                        case 2:
                        case 3:
                            eError = RMProxy_NewSendCommand(pComponentPrivate->pHandle,
                                                         RMProxy_RequestResource,
                                                         OMX_MPEG4_Encode_COMPONENT,
                                                         VIDEO_ENCODER_MHZ,
                                                         3456,
                                                         &(pComponentPrivate->cRMCallBack));
                            break;
                        case 4:
                        default:
                            eError = RMProxy_NewSendCommand(pComponentPrivate->pHandle,
                                                         RMProxy_RequestResource,
                                                         OMX_MPEG4_Encode_COMPONENT,
                                                         VIDEO_ENCODER_MHZ,
                                                         3456,
                                                         &(pComponentPrivate->cRMCallBack));
                    }
                    break;
                case OMX_VIDEO_CodingH263:
                    switch(pComponentPrivate->pH263->eLevel)
                    {
                        case OMX_VIDEO_H263Level10:
                        case OMX_VIDEO_H263Level40:
                            eError = RMProxy_NewSendCommand(pComponentPrivate->pHandle,
                                                         RMProxy_RequestResource,
                                                         OMX_H263_Encode_COMPONENT,
                                                         VIDEO_ENCODER_MHZ,
                                                         3456,
                                                         &(pComponentPrivate->cRMCallBack));
                            break;
                        case OMX_VIDEO_H263Level20:
                        case OMX_VIDEO_H263Level30:
                        default:
                            eError = RMProxy_NewSendCommand(pComponentPrivate->pHandle,
                                                         RMProxy_RequestResource,
                                                         OMX_H263_Encode_COMPONENT,
                                                         VIDEO_ENCODER_MHZ,
                                                         3456,
                                                         &(pComponentPrivate->cRMCallBack));
                    }
                    break;
                default:
                OMX_PRSTATE4(pComponentPrivate->dbg, "Unsupported compression format (%d)\n",
                             pPortDefOut->format.video.eCompressionFormat);
                    OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorUnsupportedSetting);
            }

            /* Resource Manager Proxy Calls */
                if (pCompPortOut->pPortDef->format.video.eCompressionFormat == OMX_VIDEO_CodingAVC)
                {
                    /* TODO: Disable RM Send for now */
                    /* eError = RMProxy_SendCommand(pHandle, RMProxy_RequestResource, OMX_H264_Encode_COMPONENT, 0); */
            OMX_PRMGR2(pComponentPrivate->dbg, "RMProxy_SendCommand: Setting state to Idle from Loaded\n");
                    eError = RMProxy_NewSendCommand(pComponentPrivate->pHandle,
                                                 RMProxy_StateSet,
                                                 OMX_H264_Encode_COMPONENT,
                                                 OMX_StateIdle,
                                                 3456,
                                                 NULL);
                }
                else if (pCompPortOut->pPortDef->format.video.eCompressionFormat == OMX_VIDEO_CodingMPEG4)
                {
                    /* TODO: Disable RM Send for now */
                    /* eError = RMProxy_SendCommand(pHandle, RMProxy_RequestResource, OMX_MPEG4_Encode_COMPONENT, 0); */
            OMX_PRMGR2(pComponentPrivate->dbg, "RMProxy_SendCommand: Setting state to Idle from Loaded\n");
                    eError = RMProxy_NewSendCommand(pComponentPrivate->pHandle,
                                                 RMProxy_StateSet,
                                                 OMX_MPEG4_Encode_COMPONENT,
                                                 OMX_StateIdle,
                                                 3456,
                                                 NULL);
                }
                else if (pCompPortOut->pPortDef->format.video.eCompressionFormat == OMX_VIDEO_CodingH263)
                {
                    /* TODO: Disable RM Send for now */
                    /* eError = RMProxy_SendCommand(pHandle, RMProxy_RequestResource, OMX_H263_Encode_COMPONENT, 0); */
            OMX_PRMGR2(pComponentPrivate->dbg, "RMProxy_SendCommand: Setting state to Idle from Loaded\n");
                    eError = RMProxy_NewSendCommand(pComponentPrivate->pHandle,
                                                 RMProxy_StateSet,
                                                 OMX_H263_Encode_COMPONENT,
                                                 OMX_StateIdle,
                                                 3456,
                                                 NULL);
                }
        if (eError != OMX_ErrorNone)
        {
                OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                             OMX_EventError,
                                             OMX_ErrorHardware,
                                             OMX_TI_ErrorSevere,
                                             NULL);
                }

           if (eError == OMX_ErrorNone) {

               pComponentPrivate->eState = OMX_StateIdle;
#ifdef __PERF_INSTRUMENTATION__
                PERF_Boundary(pComponentPrivate->pPERFcomp,
                            PERF_BoundaryComplete | PERF_BoundarySetup);
#endif

               /* Decrement reference count with signal enabled */
               if(RemoveStateTransition(pComponentPrivate, 1) != OMX_ErrorNone) {
                     return OMX_ErrorUndefined;
               }

               OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                         OMX_EventCmdComplete,
                                         OMX_CommandStateSet,
                                         OMX_StateIdle,
                                         NULL);
            }
            else if (eError == OMX_ErrorInsufficientResources)
            {
                pComponentPrivate->eState = OMX_StateWaitForResources;
                OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                         OMX_EventError,
                                         OMX_ErrorInsufficientResources,
                                         OMX_TI_ErrorMajor,
                                         NULL);
            }
#else /* WinCE MM will not use Linux RM, so do this... */
            pComponentPrivate->eState = OMX_StateIdle;
#ifdef __PERF_INSTRUMENTATION__
            PERF_Boundary(pComponentPrivate->pPERFcomp,
                          PERF_BoundaryComplete | PERF_BoundarySetup);
#endif

            /* Decrement reference count with signal enabled */
            if(RemoveStateTransition(pComponentPrivate, 1) != OMX_ErrorNone) {
                return OMX_ErrorUndefined;
            }

            OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                     OMX_EventCmdComplete,
                                     OMX_CommandStateSet,
                                     OMX_StateIdle,
                                     NULL);
#endif
            break;
        case OMX_StateExecuting:
        case OMX_StatePause:
            pLcmlHandle = NULL;
#ifdef __PERF_INSTRUMENTATION__
            PERF_Boundary(pComponentPrivate->pPERF,
                          PERF_BoundaryComplete | PERF_BoundarySteadyState);
#endif

            pLcmlHandle = (LCML_DSP_INTERFACE*)pComponentPrivate->pLCML;
            eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                       MMCodecControlStop,
                                       NULL);
        OMX_DBG_BAIL_IF_ERROR(eError, pComponentPrivate->dbg, OMX_PRDSP3,
                              "Failed to stop socket node (%x).\n", eError);
            pComponentPrivate->bCodecStarted = OMX_FALSE;
        OMX_PRDSP2(pComponentPrivate->dbg, "MMCodecControlStop called...\n");

#ifndef UNDER_CE
            pthread_mutex_lock(&pComponentPrivate->videoe_mutex_app);
        while (pComponentPrivate->bDSPStopAck == OMX_FALSE)
        {
                pthread_cond_wait(&pComponentPrivate->stop_cond, &pComponentPrivate->videoe_mutex_app);
            }
            pthread_mutex_unlock(&pComponentPrivate->videoe_mutex_app);
#else
        while (pComponentPrivate->bDSPStopAck == OMX_FALSE)
        {
                sched_yield();
            }
#endif
            pComponentPrivate->bDSPStopAck = OMX_FALSE;

            for (nCount = 0; nCount < pPortDefIn->nBufferCountActual; nCount++)
            {
                OMX_PRBUFFER2(pComponentPrivate->dbg, "Buffer[%d]:port[%d] -> %p [OWNER = %d]\n",
                              nCount,
                              VIDENC_INPUT_PORT,
                              pCompPortIn->pBufferPrivate[nCount]->pBufferHdr,
                              pCompPortIn->pBufferPrivate[nCount]->eBufferOwner);

                if (pCompPortIn->pBufferPrivate[nCount]->eBufferOwner == VIDENC_BUFFER_WITH_DSP ||
                    pCompPortIn->pBufferPrivate[nCount]->eBufferOwner == VIDENC_BUFFER_WITH_COMPONENT)
                {
                    OMX_PRBUFFER1(pComponentPrivate->dbg, "Buffer[%d]:port[%d] -> %p [SEND BACK TO SUPPLIER]\n",
                                  nCount,
                                  VIDENC_INPUT_PORT,
                                  pCompPortIn->pBufferPrivate[nCount]->pBufferHdr);

                    if (pCompPortIn->hTunnelComponent == NULL)
                    {

                        pCompPortIn->pBufferPrivate[nCount]->pBufferHdr->nFilledLen = 0;
                        OMX_PRBUFFER1(pComponentPrivate->dbg, "Buffer[%d]:port[%d] -> %p [memset %lu bytes]\n",
                                      nCount,
                                      VIDENC_INPUT_PORT,
                                      pCompPortIn->pBufferPrivate[nCount]->pBufferHdr,
                                      pCompPortIn->pBufferPrivate[nCount]->pBufferHdr->nAllocLen);

                        memset(pCompPortIn->pBufferPrivate[nCount]->pBufferHdr->pBuffer,
                               0x0,
                               pCompPortIn->pBufferPrivate[nCount]->pBufferHdr->nAllocLen);

                        pCompPortIn->pBufferPrivate[nCount]->eBufferOwner = VIDENC_BUFFER_WITH_CLIENT;
#ifdef __PERF_INSTRUMENTATION__
                        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                          pCompPortIn->pBufferPrivate[nCount]->pBufferHdr->pBuffer,
                                          0,
                                          PERF_ModuleHLMM);
#endif
                        pComponentPrivate->sCbData.EmptyBufferDone(pComponentPrivate->pHandle,
                                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                                   pCompPortIn->pBufferPrivate[nCount]->pBufferHdr);
                        OMX_VIDENC_IncrementBufferCountByOne(&pComponentPrivate->EmptybufferdoneCount);
                        OMX_VIDENC_SignalIfAllBuffersAreReturned(pComponentPrivate);
                    }
                    else
                    {
                        pCompPortIn->pBufferPrivate[nCount]->pBufferHdr->nFilledLen = 0;
                        pCompPortIn->pBufferPrivate[nCount]->eBufferOwner = VIDENC_BUFFER_WITH_TUNNELEDCOMP;
#ifdef __PERF_INSTRUMENTATION__
                        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                          pCompPortIn->pBufferPrivate[nCount]->pBufferHdr->pBuffer,
                                          0,
                                          PERF_ModuleLLMM);
#endif
                        eError = OMX_FillThisBuffer(pCompPortIn->hTunnelComponent,
                                                    pCompPortIn->pBufferPrivate[nCount]->pBufferHdr);
                    OMX_DBG_BAIL_IF_ERROR(eError, pComponentPrivate->dbg, OMX_PRBUFFER4,
                                          "FillThisBuffer failed (%x).\n", eError);
                    }
                }
            }

            for (nCount = 0; nCount < pPortDefOut->nBufferCountActual; nCount++)
            {
                OMX_PRBUFFER2(pComponentPrivate->dbg, "Buffer[%d]:port[%d] -> %p [OWNER = %d]\n",
                              nCount,
                              VIDENC_OUTPUT_PORT,
                              pCompPortOut->pBufferPrivate[nCount]->pBufferHdr,
                              pCompPortOut->pBufferPrivate[nCount]->eBufferOwner);

                if (pCompPortOut->pBufferPrivate[nCount]->eBufferOwner == VIDENC_BUFFER_WITH_DSP ||
                    pCompPortOut->pBufferPrivate[nCount]->eBufferOwner == VIDENC_BUFFER_WITH_COMPONENT)
                {

                    if (pCompPortOut->hTunnelComponent == NULL)
                    {
                        OMX_PRBUFFER1(pComponentPrivate->dbg, "Buffer[%d]:port[%d] -> %p [memset %lu bytes]\n",
                                      nCount,
                                      VIDENC_OUTPUT_PORT,
                                      pCompPortOut->pBufferPrivate[nCount]->pBufferHdr,
                                      pCompPortOut->pBufferPrivate[nCount]->pBufferHdr->nAllocLen);

                        memset(pCompPortOut->pBufferPrivate[nCount]->pBufferHdr->pBuffer,
                               0x0,
                               pCompPortOut->pBufferPrivate[nCount]->pBufferHdr->nAllocLen);
                    }

                    OMX_PRBUFFER1(pComponentPrivate->dbg, "Buffer[%d]:port[%d] -> %p [SEND BACK TO SUPPLIER]\n", nCount,
                                  VIDENC_OUTPUT_PORT,
                                  pCompPortOut->pBufferPrivate[nCount]->pBufferHdr);

                    pCompPortOut->pBufferPrivate[nCount]->pBufferHdr->nFilledLen = 0;
                    pCompPortOut->pBufferPrivate[nCount]->eBufferOwner = VIDENC_BUFFER_WITH_CLIENT;
#ifdef __PERF_INSTRUMENTATION__
                    PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                      pCompPortOut->pBufferPrivate[nCount]->pBufferHdr->pBuffer,
                                      pCompPortOut->pBufferPrivate[nCount]->pBufferHdr->nFilledLen,
                                      PERF_ModuleHLMM);
#endif
                    /*Propagate pBufferHeader Data*/
                    OMX_CONF_CIRCULAR_BUFFER_MOVE_HEAD(pCompPortOut->pBufferPrivate[nCount]->pBufferHdr,
                                                   pComponentPrivate->sCircularBuffer,
                                                   pComponentPrivate);
                    /* trigger event handler if we are supposed to */
                    if (pCompPortOut->pBufferPrivate[nCount]->pBufferHdr->hMarkTargetComponent == pComponentPrivate->pHandle &&
                        pCompPortOut->pBufferPrivate[nCount]->pBufferHdr->pMarkData)
                    {
                        OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                                 OMX_EventMark,
                                                 0x0,
                                                 0x0,
                                                 pCompPortOut->pBufferPrivate[nCount]->pBufferHdr->pMarkData);
                    }
                    pComponentPrivate->sCbData.FillBufferDone(pComponentPrivate->pHandle,
                                                              pComponentPrivate->pHandle->pApplicationPrivate,
                                                              pCompPortOut->pBufferPrivate[nCount]->pBufferHdr);
                    OMX_VIDENC_IncrementBufferCountByOne(&pComponentPrivate->FillbufferdoneCount);
                    OMX_VIDENC_SignalIfAllBuffersAreReturned(pComponentPrivate);
                }
            }

#ifdef RESOURCE_MANAGER_ENABLED /* Resource Manager Proxy Calls */
                if (pCompPortOut->pPortDef->format.video.eCompressionFormat == OMX_VIDEO_CodingAVC)
                {
                    /* TODO: Disable RM Send for now */
                    /* eError = RMProxy_SendCommand(pHandle, RMProxy_RequestResource, OMX_H264_Encode_COMPONENT, 0); */
                    eError = RMProxy_NewSendCommand(pComponentPrivate->pHandle,
                                                 RMProxy_StateSet,
                                                 OMX_H264_Encode_COMPONENT,
                                                 OMX_StateIdle,
                                                 3456,
                                                 NULL);
                }
                else if (pCompPortOut->pPortDef->format.video.eCompressionFormat == OMX_VIDEO_CodingMPEG4)
                {
                    /* TODO: Disable RM Send for now */
                    /* eError = RMProxy_SendCommand(pHandle, RMProxy_RequestResource, OMX_MPEG4_Encode_COMPONENT, 0); */
                    eError = RMProxy_NewSendCommand(pComponentPrivate->pHandle,
                                                 RMProxy_StateSet,
                                                 OMX_MPEG4_Encode_COMPONENT,
                                                 OMX_StateIdle,
                                                 3456,
                                                 NULL);
                }
                else if (pCompPortOut->pPortDef->format.video.eCompressionFormat == OMX_VIDEO_CodingH263)
                {
                    /* TODO: Disable RM Send for now */
                    /* eError = RMProxy_SendCommand(pHandle, RMProxy_RequestResource, OMX_H263_Encode_COMPONENT, 0); */
            OMX_PRMGR2(pComponentPrivate->dbg, "Setting Idle state from Executing to RMProxy\n");
                    eError = RMProxy_NewSendCommand(pComponentPrivate->pHandle,
                                                 RMProxy_StateSet,
                                                 OMX_H263_Encode_COMPONENT,
                                                 OMX_StateIdle,
                                                 3456,
                                                 NULL);
                }
        if (eError != OMX_ErrorNone)
        {
                    OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                             OMX_EventError,
                                             OMX_ErrorHardware,
                                             OMX_TI_ErrorMajor,
                                             NULL);
                }

#endif
        OMX_PRBUFFER2(pComponentPrivate->dbg, "Flushing Pipes!\n");
        OMX_VIDENC_EmptyDataPipes (pComponentPrivate);

        pComponentPrivate->eState = OMX_StateIdle;

        /* Decrement reference count with signal enabled */
        if(RemoveStateTransition(pComponentPrivate, 1) != OMX_ErrorNone) {
             return OMX_ErrorUndefined;
        }

        OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                 OMX_EventCmdComplete,
                                 OMX_CommandStateSet,
                                 OMX_StateIdle,
                                 NULL);
        break;
        default:
            OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                     OMX_EventError,
                                     OMX_ErrorIncorrectStateTransition,
                                     OMX_TI_ErrorMinor,
                                     NULL);
    }
OMX_CONF_CMD_BAIL:
    return eError;
}

/*----------------------------------------------------------------------------*/
/**
  * OMX_VIDENC_HandleCommandStateSet()
  *
  * Called by component thread, handles commands sent by the app.
  *
  * @param phandle LCML_DSP_INTERFACE handle for this instance of the component
  *
  * @retval OMX_ErrorNone                  success, ready to roll
  *         OMX_ErrorInsufficientResources if the malloc fails
  **/
/*----------------------------------------------------------------------------*/
OMX_ERRORTYPE OMX_VIDENC_HandleCommandStateSetExecuting(VIDENC_COMPONENT_PRIVATE* pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    LCML_DSP_INTERFACE* pLcmlHandle = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefIn = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefOut = NULL;

    OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);

    pPortDefIn = pComponentPrivate->pCompPort[VIDENC_INPUT_PORT]->pPortDef;
    pPortDefOut = pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pPortDef;

    switch (pComponentPrivate->eState)
    {
        case OMX_StateExecuting:
            OMX_VIDENC_EVENT_HANDLER(pComponentPrivate, OMX_EventError, OMX_ErrorSameState, OMX_TI_ErrorMinor, NULL);
            break;
        case OMX_StateIdle:
            OMX_CONF_CIRCULAR_BUFFER_RESTART(pComponentPrivate->sCircularBuffer);
        case OMX_StatePause:
            if (pComponentPrivate->bCodecStarted == OMX_FALSE)
            {
                pLcmlHandle = NULL;
                pLcmlHandle = (LCML_DSP_INTERFACE*)pComponentPrivate->pLCML;
                pLcmlHandle->pComponentPrivate = (VIDENC_COMPONENT_PRIVATE*)pComponentPrivate;

            OMX_PRDSP2(pComponentPrivate->dbg, "Starting the codec...\n");
                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                           EMMCodecControlStart,
                                           NULL);
            OMX_DBG_BAIL_IF_ERROR(eError, pComponentPrivate->dbg, OMX_PRDSP4,
                                  "Failed to start socket node (%x).\n", eError);

                pComponentPrivate->bCodecStarted = OMX_TRUE;
            }

#ifdef RESOURCE_MANAGER_ENABLED /* Resource Manager Proxy Calls */
            if (pPortDefOut->format.video.eCompressionFormat == OMX_VIDEO_CodingAVC)
            {
                /* TODO: Disable RM Send for now */
            OMX_PRMGR2(pComponentPrivate->dbg, "Setting executing state to RMProxy\n");
                /* eError = RMProxy_SendCommand(pHandle, RMProxy_RequestResource, OMX_H264_Encode_COMPONENT, 0); */
                eError = RMProxy_NewSendCommand(pComponentPrivate->pHandle,
                                             RMProxy_StateSet,
                                             OMX_H264_Encode_COMPONENT,
                                             OMX_StateExecuting,
                                             3456,
                                             NULL);
            }
            else if (pPortDefOut->format.video.eCompressionFormat == OMX_VIDEO_CodingMPEG4)
            {
                /* TODO: Disable RM Send for now */
            OMX_PRMGR2(pComponentPrivate->dbg, "Setting executing state to RMProxy\n");
                /* eError = RMProxy_SendCommand(pHandle, RMProxy_RequestResource, OMX_MPEG4_Encode_COMPONENT, 0); */
                eError = RMProxy_NewSendCommand(pComponentPrivate->pHandle,
                                             RMProxy_StateSet,
                                             OMX_MPEG4_Encode_COMPONENT,
                                             OMX_StateExecuting,
                                             3456,
                                             NULL);

            }
            else if (pPortDefOut->format.video.eCompressionFormat == OMX_VIDEO_CodingH263)
            {
                /* TODO: Disable RM Send for now */
                /* eError = RMProxy_SendCommand(pHandle, RMProxy_RequestResource, OMX_H263_Encode_COMPONENT, 0); */
            OMX_PRMGR2(pComponentPrivate->dbg, "Setting executing state to RMProxy\n");
                eError = RMProxy_NewSendCommand(pComponentPrivate->pHandle,
                                             RMProxy_StateSet,
                                             OMX_H263_Encode_COMPONENT,
                                             OMX_StateExecuting,
                                             3456,
                                             NULL);
            }
            if (eError != OMX_ErrorNone)
            {
                pComponentPrivate->eState = OMX_StateWaitForResources;
                OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                         OMX_EventError,
                                         OMX_ErrorHardware,
                                         OMX_TI_ErrorMajor,
                                         NULL);
            }
#endif
            pComponentPrivate->eState = OMX_StateExecuting;
#ifdef __PERF_INSTRUMENTATION__
            PERF_Boundary(pComponentPrivate->pPERFcomp,
                          PERF_BoundaryStart | PERF_BoundarySteadyState);
#endif

            /* Decrement reference count with signal enabled */
            if(RemoveStateTransition(pComponentPrivate, 1) != OMX_ErrorNone) {
                 return OMX_ErrorUndefined;
            }

            /*Send state change notificaiton to Application*/
            OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                     OMX_EventCmdComplete,
                                     OMX_CommandStateSet,
                                     OMX_StateExecuting,
                                     NULL);
            break;
        default:
            OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                     OMX_EventError,
                                     OMX_ErrorIncorrectStateTransition,
                                     OMX_TI_ErrorMinor,
                                     NULL);
    }
OMX_CONF_CMD_BAIL:
    return eError;
}

/*----------------------------------------------------------------------------*/
/**
  * OMX_VIDENC_HandleCommandStateSetPause()
  *
  * Called by component thread, handles commands sent by the app.
  *
  * @param phandle LCML_DSP_INTERFACE handle for this instance of the component
  *
  * @retval OMX_ErrorNone                  success, ready to roll
  *         OMX_ErrorInsufficientResources if the malloc fails
  **/
/*----------------------------------------------------------------------------*/
OMX_ERRORTYPE OMX_VIDENC_HandleCommandStateSetPause (VIDENC_COMPONENT_PRIVATE* pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefIn = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefOut = NULL;
    LCML_DSP_INTERFACE* pLcmlHandle = NULL;

    OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);

    pLcmlHandle     = (LCML_DSP_INTERFACE*)pComponentPrivate->pLCML;
    pPortDefIn = pComponentPrivate->pCompPort[VIDENC_INPUT_PORT]->pPortDef;
    pPortDefOut = pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pPortDef;


    switch (pComponentPrivate->eState)
    {
        case OMX_StatePause:
            OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                     OMX_EventError,
                                     OMX_ErrorSameState,
                                     OMX_TI_ErrorMinor,
                                     NULL);
            break;
        case OMX_StateIdle:
        case OMX_StateExecuting:
            pLcmlHandle = NULL;
#ifdef __PERF_INSTRUMENTATION__
            PERF_Boundary(pComponentPrivate->pPERFcomp,
                          PERF_BoundaryComplete | PERF_BoundarySteadyState);
#endif
            pLcmlHandle = (LCML_DSP_INTERFACE*)pComponentPrivate->pLCML;
            eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                       EMMCodecControlPause,
                                   NULL);
        OMX_DBG_BAIL_IF_ERROR(eError, pComponentPrivate->dbg, OMX_PRDSP4,
                              "Failed to pause socket node (%x).\n", eError);


        pComponentPrivate->bCodecStarted = OMX_FALSE;
        OMX_PRDSP2(pComponentPrivate->dbg, "MMCodecControlPaused called...\n");


#ifndef UNDER_CE
            pthread_mutex_lock(&pComponentPrivate->videoe_mutex_app);
        while (pComponentPrivate->bDSPStopAck == OMX_FALSE)
        {
                pthread_cond_wait(&pComponentPrivate->stop_cond, &pComponentPrivate->videoe_mutex_app);
            }
            pthread_mutex_unlock(&pComponentPrivate->videoe_mutex_app);
#else
        while (pComponentPrivate->bDSPStopAck == OMX_FALSE)
        {
                sched_yield();
            }
#endif

            pComponentPrivate->bDSPStopAck = OMX_FALSE;

            pComponentPrivate->eState = OMX_StatePause;

            /* Decrement reference count with signal enabled */
            if(RemoveStateTransition(pComponentPrivate, 1) != OMX_ErrorNone) {
                 return OMX_ErrorUndefined;
            }

            OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                     OMX_EventCmdComplete,
                                     OMX_CommandStateSet,
                                     OMX_StatePause,
                                     NULL);
            break;
        default:
            OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                     OMX_EventError,
                                     OMX_ErrorIncorrectStateTransition,
                                     OMX_TI_ErrorMinor,
                                     NULL);
    }

OMX_CONF_CMD_BAIL:
    return eError;
}

/*----------------------------------------------------------------------------*/
/**
  * OMX_VIDENC_HandleCommandStateSetLoaded()
  *
  * Called by component thread, handles commands sent by the app.
  *
  * @param phandle LCML_DSP_INTERFACE handle for this instance of the component
  *
  * @retval OMX_ErrorNone                  success, ready to roll
  *         OMX_ErrorInsufficientResources if the malloc fails
  **/
/*----------------------------------------------------------------------------*/
OMX_ERRORTYPE OMX_VIDENC_HandleCommandStateSetLoaded (VIDENC_COMPONENT_PRIVATE* pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    LCML_DSP_INTERFACE* pLcmlHandle = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefIn = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefOut = NULL;

    OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);

    pPortDefIn = pComponentPrivate->pCompPort[VIDENC_INPUT_PORT]->pPortDef;
    pPortDefOut = pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pPortDef;

    switch (pComponentPrivate->eState)
    {
        case OMX_StateLoaded:
            OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                     OMX_EventError,
                                     OMX_ErrorSameState,
                                     OMX_TI_ErrorMinor,
                                     NULL);
            break;
        case OMX_StateWaitForResources:
        OMX_PRSTATE2(pComponentPrivate->dbg, "Transitioning from WFR to Loaded\n");
#ifdef RESOURCE_MANAGER_ENABLED
            if (pPortDefOut->format.video.eCompressionFormat == OMX_VIDEO_CodingAVC)
            {
                /* TODO: Disable RM Send for now */
                /* eError = RMProxy_SendCommand(pHandle, RMProxy_RequestResource, OMX_H264_Encode_COMPONENT, 0); */
            OMX_PRMGR2(pComponentPrivate->dbg, "RMProxy_SendCommand: Setting state to Loaded from WFR\n");
                eError = RMProxy_NewSendCommand(pComponentPrivate->pHandle,
                                             RMProxy_StateSet,
                                             OMX_H264_Encode_COMPONENT,
                                             OMX_StateLoaded,
                                             3456,
                                             NULL);
            }
            else if (pPortDefOut->format.video.eCompressionFormat == OMX_VIDEO_CodingMPEG4)
            {
                /* TODO: Disable RM Send for now */
                /* eError = RMProxy_SendCommand(pHandle, RMProxy_RequestResource, OMX_MPEG4_Encode_COMPONENT, 0); */
            OMX_PRMGR2(pComponentPrivate->dbg, "RMProxy_SendCommand: Setting state to Loaded from WFR\n");
                eError = RMProxy_NewSendCommand(pComponentPrivate->pHandle,
                                             RMProxy_StateSet,
                                             OMX_MPEG4_Encode_COMPONENT,
                                             OMX_StateLoaded,
                                             3456,
                                             NULL);
            }
            else if (pPortDefOut->format.video.eCompressionFormat == OMX_VIDEO_CodingH263)
            {
                /* TODO: Disable RM Send for now */
                /* eError = RMProxy_SendCommand(pHandle, RMProxy_RequestResource, OMX_H263_Encode_COMPONENT, 0); */
            OMX_PRMGR2(pComponentPrivate->dbg, "RMProxy_SendCommand: Setting state to Loaded from WFR\n");
                eError = RMProxy_NewSendCommand(pComponentPrivate->pHandle,
                                             RMProxy_StateSet,
                                             OMX_H263_Encode_COMPONENT,
                                             OMX_StateLoaded,
                                             3456,
                                             NULL);
            }
            if (eError != OMX_ErrorNone)
            {
                OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                         OMX_EventError,
                                         OMX_ErrorHardware,
                                         OMX_TI_ErrorMajor,
                                         NULL);
                break;
            }
#endif
            pComponentPrivate->eState = OMX_StateLoaded;
            /* Decrement reference count with signal enabled */
            if(RemoveStateTransition(pComponentPrivate, 1) != OMX_ErrorNone) {
                return OMX_ErrorUndefined;
            }

            #ifdef __PERF_INSTRUMENTATION__
                PERF_Boundary(pComponentPrivate->pPERFcomp,
                              PERF_BoundaryComplete | PERF_BoundaryCleanup);
            #endif
            OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                     OMX_EventCmdComplete,
                                     OMX_CommandStateSet,
                                     OMX_StateLoaded,
                                     NULL);
            break;
        case OMX_StateIdle:
            OMX_PRSTATE2(pComponentPrivate->dbg, "Transitioning from Idle to Loaded\n");
            pLcmlHandle = (LCML_DSP_INTERFACE*)pComponentPrivate->pLCML;
    #ifdef __PERF_INSTRUMENTATION__
            PERF_Boundary(pComponentPrivate->pPERFcomp,
                          PERF_BoundaryStart | PERF_BoundaryCleanup);
    #endif
            pthread_mutex_lock(&pComponentPrivate->videoe_mutex_app);
            while ( (pPortDefIn->bPopulated) || (pPortDefOut->bPopulated))
            {
    #ifndef UNDER_CE
                    pthread_cond_wait(&pComponentPrivate->unpopulate_cond, &pComponentPrivate->videoe_mutex_app);
    #else
                    OMX_WaitForEvent(&(pComponentPrivate->InIdle_event));
    #endif
            }
            pthread_mutex_unlock(&pComponentPrivate->videoe_mutex_app);

    #ifdef RESOURCE_MANAGER_ENABLED /* Resource Manager Proxy Calls */
            if (pPortDefOut->format.video.eCompressionFormat == OMX_VIDEO_CodingAVC)
            {
                /* TODO: Disable RM Send for now */
                /* eError = RMProxy_SendCommand(pHandle, RMProxy_RequestResource, OMX_H264_Encode_COMPONENT, 0); */
                eError = RMProxy_NewSendCommand(pComponentPrivate->pHandle,
                                             RMProxy_FreeResource,
                                             OMX_H264_Encode_COMPONENT,
                                             0,
                                             3456,
                                             NULL);
            }
            else if (pPortDefOut->format.video.eCompressionFormat == OMX_VIDEO_CodingMPEG4)
            {
                /* TODO: Disable RM Send for now */
                /* eError = RMProxy_SendCommand(pHandle, RMProxy_RequestResource, OMX_MPEG4_Encode_COMPONENT, 0); */
                eError = RMProxy_NewSendCommand(pComponentPrivate->pHandle,
                                             RMProxy_FreeResource,
                                             OMX_MPEG4_Encode_COMPONENT,
                                             0,
                                             3456,
                                             NULL);
            }
            else if (pPortDefOut->format.video.eCompressionFormat == OMX_VIDEO_CodingH263)
            {
                /* TODO: Disable RM Send for now */
                /* eError = RMProxy_SendCommand(pHandle, RMProxy_RequestResource, OMX_H263_Encode_COMPONENT, 0); */
                eError = RMProxy_NewSendCommand(pComponentPrivate->pHandle,
                                             RMProxy_FreeResource,
                                             OMX_H263_Encode_COMPONENT,
                                             0,
                                             3456,
                                             NULL);
            }
            if (eError != OMX_ErrorNone)
            {
                OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                         OMX_EventError,
                                         OMX_ErrorHardware,
                                         OMX_TI_ErrorMajor,
                                         NULL);
            }
    #endif
             /* Make sure the DSP node has been deleted */
             if (pComponentPrivate->bCodecStarted == OMX_TRUE || pComponentPrivate->bCodecLoaded == OMX_TRUE)
             {
                 OMX_TRACE2(pComponentPrivate->dbg, "LCML_ControlCodec EMMCodecControlDestroy\n");
                 eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                             EMMCodecControlDestroy,
                                             NULL);
                 OMX_CONF_BAIL_IF_ERROR(eError);
                 OMX_TRACE2(pComponentPrivate->dbg,"Atempting to Unload LCML");
                 /*Unload LCML */
                 if(pComponentPrivate->pModLcml != NULL)
                 {
                     OMX_TRACE2(pComponentPrivate->dbg,"Unloading LCML");
                     dlclose(pComponentPrivate->pModLcml);
                     pComponentPrivate->pModLcml = NULL;
                     pComponentPrivate->pLCML = NULL;
                 }

                 pComponentPrivate->bCodecStarted = OMX_FALSE;
                 pComponentPrivate->bCodecLoaded = OMX_FALSE;
             }

             OMX_CONF_BAIL_IF_ERROR(eError);

#ifdef __KHRONOS_CONF__
            pComponentPrivate->bPassingIdleToLoaded = OMX_FALSE;
#endif
            pComponentPrivate->eState = OMX_StateLoaded;

#ifdef __PERF_INSTRUMENTATION__
                PERF_Boundary(pComponentPrivate->pPERFcomp,
                              PERF_BoundaryComplete | PERF_BoundaryCleanup);
#endif

            /* Decrement reference count with signal enabled */
            if(RemoveStateTransition(pComponentPrivate, 1) != OMX_ErrorNone) {
                 return OMX_ErrorUndefined;
            }

            OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                     OMX_EventCmdComplete,
                                     OMX_CommandStateSet,
                                     OMX_StateLoaded,
                                     NULL);
            break;
        default:
            OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                                     OMX_EventError,
                                     OMX_ErrorIncorrectStateTransition,
                                     OMX_TI_ErrorMinor,
                                     NULL);
    }
OMX_CONF_CMD_BAIL:
    return eError;
}


/*---------------------------------------------------------------------------------------*/
/**
  * OMX_OMX_VIDENC_Process_FreeOutBuf()
  *
  * Called by component thread, handles free output buffers from app.
  *
  * @param pComponentPrivate private component structure for this instance of the component
  *
  * @param phandle LCML_DSP_INTERFACE handle for this instance of the component
  *
  * @retval OMX_ErrorNone                  success, ready to roll
  *         OMX_ErrorInsufficientResources if the malloc fails
  **/
/*---------------------------------------------------------------------------------------*/

OMX_ERRORTYPE OMX_VIDENC_Process_FreeOutBuf(VIDENC_COMPONENT_PRIVATE* pComponentPrivate)
{
    int nRet = -1;
    void *pUalgOutParams = NULL;
    VIDENC_NODE* pMemoryListHead = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE* pBufHead = NULL;
    LCML_DSP_INTERFACE* pLcmlHandle = NULL;
     VIDENC_BUFFER_PRIVATE* pBufferPrivate = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefOut = NULL;

    OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);

    pPortDefOut = pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pPortDef;
    pLcmlHandle = (LCML_DSP_INTERFACE*)(((VIDENC_COMPONENT_PRIVATE*)pComponentPrivate)->pLCML);
    pMemoryListHead = pComponentPrivate->pMemoryListHead;

#ifndef UNDER_CE
    if (pthread_mutex_lock(&(pComponentPrivate->mVideoEncodeBufferMutex)) != 0)
    {
        OMX_TRACE4(pComponentPrivate->dbg, "pthread_mutex_lock() failed.\n");
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorHardware);
    }
    nRet = read(pComponentPrivate->nFree_oPipe[0], &pBufHead, sizeof(pBufHead));
    if ((nRet == -1) || !pBufHead || !pBufHead->pOutputPortPrivate)
    {
        pthread_mutex_unlock(&(pComponentPrivate->mVideoEncodeBufferMutex));
        OMX_ERROR4(pComponentPrivate->dbg, "Error while reading from the pipe\n");
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorHardware);
    }

    pBufferPrivate = pBufHead->pOutputPortPrivate;

    pBufferPrivate->bReadFromPipe = OMX_TRUE;
    if (pthread_mutex_unlock(&(pComponentPrivate->mVideoEncodeBufferMutex)) != 0)
    {
        OMX_TRACE4(pComponentPrivate->dbg, "pthread_mutex_unlock() failed.\n");
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorHardware);
    }
#else
    nRet = read(pComponentPrivate->nFree_oPipe[0], &pBufHead, sizeof(pBufHead));
    if ((nRet == -1) || (pBufHead == NULL) || (pBufHead->pOutputPortPrivate == NULL))
    {
        OMX_ERROR4(pComponentPrivate->dbg, "Error while reading from the pipe\n");
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorHardware);
    }
    if (pBufHead != NULL)
    {
        pBufferPrivate = pBufHead->pOutputPortPrivate;
    }
    pBufferPrivate->bReadFromPipe = OMX_TRUE;
#endif

#ifdef __PERF_INSTRUMENTATION__
    PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                      PREF(pBufHead,pBuffer),
                      0,
                      PERF_ModuleCommonLayer);
#endif

if (pBufferPrivate->eBufferOwner == VIDENC_BUFFER_WITH_DSP ||
        pBufferPrivate->eBufferOwner == VIDENC_BUFFER_WITH_CLIENT)
    {
goto EXIT;

}

    if(!pBufferPrivate || !pLcmlHandle || !pPortDefOut)
        goto EXIT;

    if (pPortDefOut->format.video.eCompressionFormat == OMX_VIDEO_CodingAVC)
    {
            pUalgOutParams =(H264VE_GPP_SN_UALGOutputParams*)pBufferPrivate->pUalgParam;
        OMX_PRBUFFER1(pComponentPrivate->dbg, " %p \n", (void*)pBufHead);
            pBufferPrivate->eBufferOwner = VIDENC_BUFFER_WITH_DSP;
            eError = LCML_QueueBuffer(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                      EMMCodecOuputBuffer,
                                      pBufHead->pBuffer,
                                      pBufHead->nAllocLen,
                                      0,
                                      (OMX_U8*)pUalgOutParams,
                                      sizeof(H264VE_GPP_SN_UALGOutputParams),
                                      (void*)pBufHead);
            if (eError != OMX_ErrorNone)
        {
            OMX_PRDSP4(pComponentPrivate->dbg, "LCML QueueBuffer failed: %x\n", eError);
            OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorHardware);
        }

    }
    else if (pPortDefOut->format.video.eCompressionFormat == OMX_VIDEO_CodingMPEG4 ||
             pPortDefOut->format.video.eCompressionFormat == OMX_VIDEO_CodingH263)
    {
        pUalgOutParams = (MP4VE_GPP_SN_UALGOutputParams*)pBufferPrivate->pUalgParam;
        OMX_PRBUFFER1(pComponentPrivate->dbg, " %p\n", (void*)pBufHead);
        pBufferPrivate->eBufferOwner = VIDENC_BUFFER_WITH_DSP;
        eError = LCML_QueueBuffer(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                  EMMCodecOuputBuffer,
                                  pBufHead->pBuffer,
                                  pBufHead->nAllocLen,
                                  0,
                                  (OMX_U8*)pUalgOutParams,
                                  sizeof(MP4VE_GPP_SN_UALGOutputParams),
                                  (void*)pBufHead);
        if (eError != OMX_ErrorNone)
        {
            OMX_PRDSP4(pComponentPrivate->dbg, "LCML QueueBuffer failed: %x\n", eError);
            OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorHardware);
        }
    }
    else
    {
        OMX_PRBUFFER4(pComponentPrivate->dbg, "Unsupported compression format (%d)\n",
                      pPortDefOut->format.video.eCompressionFormat);
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorUnsupportedSetting);
    }
    EXIT:
OMX_CONF_CMD_BAIL:
    return eError;
}

/*---------------------------------------------------------------------------------------*/
/**
  * OMX_VIDENC_Process_FilledInBuf()
  *
  * Called by component thread, handles filled input buffers from app.
  *
  * @param pComponentPrivate private component structure for this instance of the component
  *
  * @param phandle LCML_DSP_INTERFACE handle for this instance of the component
  *
  * @retval OMX_ErrorNone                  success, ready to roll
  *         OMX_ErrorInsufficientResources if the malloc fails
  **/
/*---------------------------------------------------------------------------------------*/

OMX_ERRORTYPE OMX_VIDENC_Process_FilledInBuf(VIDENC_COMPONENT_PRIVATE* pComponentPrivate)
{
    OMX_U8 i = 0;
    int nRet = -1;
    void* pUalgInpParams = NULL;
    VIDENC_NODE* pMemoryListHead = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE* pBufHead = NULL;
    LCML_DSP_INTERFACE* pLcmlHandle = NULL;
    VIDENC_BUFFER_PRIVATE* pBufferPrivate = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefIn = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefOut = NULL;
    VIDEOENC_PORT_TYPE* pCompPortOut            = NULL;

    OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);

    pPortDefIn = pComponentPrivate->pCompPort[VIDENC_INPUT_PORT]->pPortDef;
    pPortDefOut = pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pPortDef;
    pCompPortOut = pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT];
    pLcmlHandle = (LCML_DSP_INTERFACE*)pComponentPrivate->pLCML;
    pMemoryListHead = pComponentPrivate->pMemoryListHead;

    OMX_DBG_CHECK_CMD(pComponentPrivate->dbg, pLcmlHandle, pPortDefIn, 1);

#ifndef UNDER_CE
    if (pthread_mutex_lock(&(pComponentPrivate->mVideoEncodeBufferMutex)) != 0)
    {
        OMX_TRACE4(pComponentPrivate->dbg, "pthread_mutex_lock() failed.\n");
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorHardware);
    }
    nRet = read(pComponentPrivate->nFilled_iPipe[0], &(pBufHead), sizeof(pBufHead));
    if ((nRet == -1) || !pBufHead || !pBufHead->pInputPortPrivate)
    {
        pthread_mutex_unlock(&(pComponentPrivate->mVideoEncodeBufferMutex));
        OMX_TRACE4(pComponentPrivate->dbg, "Error while reading from the pipe\n");
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorHardware);
    }

    if (pBufHead != NULL)
    {
    pBufferPrivate = (VIDENC_BUFFER_PRIVATE*)pBufHead->pInputPortPrivate;
   	}
    OMX_DBG_CHECK_CMD(pComponentPrivate->dbg, pBufHead, pBufferPrivate, 1);
    pBufferPrivate->bReadFromPipe = OMX_TRUE;

    if (pthread_mutex_unlock(&(pComponentPrivate->mVideoEncodeBufferMutex)) != 0)
    {
        OMX_TRACE4(pComponentPrivate->dbg, "pthread_mutex_unlock() failed.\n");
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorHardware);
    }
#else
    nRet = read(pComponentPrivate->nFilled_iPipe[0], &(pBufHead), sizeof(pBufHead));
    if (nRet == -1)
    {
        OMX_TRACE4(pComponentPrivate->dbg, "Error while reading from the pipe\n");
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorHardware);
    }
    pBufferPrivate = (VIDENC_BUFFER_PRIVATE*)pBufHead->pInputPortPrivate;
    OMX_DBG_CHECK_CMD(pComponentPrivate->dbg, pBufHead, pBufferPrivate, 1);
    pBufferPrivate->bReadFromPipe = OMX_TRUE;
#endif

#ifdef __PERF_INSTRUMENTATION__
    /*For Steady State Instumentation*/
    #if 0
    if ((pComponentPrivate->nLcml_nCntIp == 1))
    {
        PERF_Boundary(pComponentPrivate->pPERFcomp,
                      PERF_BoundaryStart | PERF_BoundarySteadyState);
    }
    #endif
    PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                      PREF(pBufHead,pBuffer),
                      pPortDefIn->nBufferSize,
                      PERF_ModuleCommonLayer);
#endif

    if (pBufferPrivate->eBufferOwner == VIDENC_BUFFER_WITH_DSP ||
        pBufferPrivate->eBufferOwner == VIDENC_BUFFER_WITH_CLIENT)
    {
        goto EXIT;
    }



    if (pPortDefOut->format.video.eCompressionFormat == OMX_VIDEO_CodingAVC)
    {

        pUalgInpParams = (H264VE_GPP_SN_UALGInputParams*)pBufferPrivate->pUalgParam;
        OMX_DBG_CHECK_CMD(pComponentPrivate->dbg, pUalgInpParams, 1, 1);

        /*< must be followed for all video encoders*/
        /*  size of this structure             */
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.videncDynamicParams.size = sizeof(H264VE_GPP_SN_UALGInputParams);
        /*  Input frame height                 */
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.videncDynamicParams.inputHeight = pPortDefIn->format.video.nFrameHeight;
        /*  Input frame width                  */
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.videncDynamicParams.inputWidth  = pPortDefIn->format.video.nFrameWidth;
        /*  Reference or input frame rate*1000 */
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.videncDynamicParams.refFrameRate = (OMX_U32)(Q16Tof(pPortDefIn->format.video.xFramerate)*1000.0);
        /*  Target frame rate * 1000           */
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.videncDynamicParams.targetFrameRate = pComponentPrivate->nTargetFrameRate;
        /*  Target bit rate in bits per second */
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.videncDynamicParams.targetBitRate = pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pBitRateTypeConfig->nEncodeBitrate;
        /*  I frame interval e.g. 30           */
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.videncDynamicParams.intraFrameInterval = pComponentPrivate->nIntraFrameInterval;
        /*  XDM_ENCODE_AU, XDM_GENERATE_HEADER */
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.videncDynamicParams.generateHeader = 0;
        /*  DEFAULT(0): use imagewidth as pitch
        *  else use given capture width for
        *  pitch provided it is greater than
        *  image width.*/
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.videncDynamicParams.captureWidth = 0;
        /*  Force given frame as I or IDR (in H.264)  frame    */
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.videncDynamicParams.forceIFrame = pComponentPrivate->bForceIFrame;



        /*< initial QP of I frames Range[-1,51]. -1 is for auto initialization.*/
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.qpIntra = 0x0000001c;
        /*< initial QP of P frames Range[-1,51]. -1 is for auto initialization.*/
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.qpInter = 0x0000001c;
        /*< Maximum QP to be used  Range[0,51]*/
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.qpMax = 0x00000033;
        /*< Minimum QP to be used  Range[0,51]*/
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.qpMin = 0x00000000;
        /*< Controls enable/disable loop filter, See IH264VENC_LoopFilterParams for more details*/
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.lfDisableIdc = 0x00000000;
        /*< enable/disable Quarter Pel Interpolation*/
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.quartPelDisable = 0x00000000;
        /*< Adaptive Intra Refesh MB Period: Period at which intra macro blocks should be insterted in a frame*/
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.airMbPeriod = pComponentPrivate->nAIRRate;
        /*< Maximum number of macro block in a slice <minimum value is 8>*/
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.maxMBsPerSlice = 0;
        /*< Maximum number of bytes in a slice */
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.maxBytesPerSlice = 0;
        /*< Row number from which slice needs to be intra coded*/
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.sliceRefreshRowStartNumber = 0;
        /*< Number of rows to be coded as intra slice*/
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.sliceRefreshRowNumber = 0;
        /*< alpha offset for loop filter [-12, 12] even number*/
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.filterOffsetA = 0;
        /*< beta offset for loop filter [-12, 12] even number*/
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.filterOffsetB = 0 ;
        /*< Limits the maximum frame number in the bit-stream to (1<< (log2MaxFNumMinus4 + 4)) Range[0,12]*/
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.log2MaxFNumMinus4 = 0;
        /*< Specifies offset to be added to luma QP for addressing QPC values table for chroma components. Valid value is between -12 and 12, (inclusive)*/
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.chromaQPIndexOffset = 0;
        /*< Controls the intra macroblock coding in P slices [0,1]*/
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.constrainedIntraPredEnable = 0;
        /*< Picture Order count type Valid values 0, 2*/
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.picOrderCountType = 0;
        /*< enable/Disable Multiple Motion vector per MB, valid values are [1, 4] [For DM6446, allowed value is only 1]*/
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.maxMVperMB = pComponentPrivate->maxMVperMB;
        /*< See IH264VENC_Intra4x4Params for more details*/
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.intra4x4EnableIdc = pComponentPrivate->intra4x4EnableIdc;
        /*< enable/Disable Motion vector access*/
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.mvDataEnable = 0;
        /*< Enable/Disable Hierarchical P Frame (non-reference P frame) Coding. [Not useful for DM6446]*/
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.hierCodingEnable = 0; /* should be 1; */
        /*< Signals the type of stream generated with Call-back*/
        if (pComponentPrivate->AVCNALFormat == VIDENC_AVC_NAL_UNIT)
        {
            ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.streamFormat = IH264_BYTE_STREAM;
        }
        else
        {
           ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.streamFormat = IH264_NALU_STREAM;
        }
        /*< Mechanism to do intra Refresh, see IH264VENC_IntraRefreshMethods for valid values*/
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.intraRefreshMethod = IH264_INTRAREFRESH_NONE;
        /* Enable Perceptual Quantization a.k.a. Perceptual Rate Control*/
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.perceptualQuant = 0;
        /* Enable Scene Change Detection*/
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.sceneChangeDet = 0;
        /*< Function pointer of the call-back function to be used by Encoder*/
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.pfNalUnitCallBack = NULL;

        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.pContext = NULL;

        /*< Following Parameter are related to Arbitrary Slice Ordering (ASO)*/
        /*< Number of valid enteries in asoSliceOrder array valid range is [0,8],
        //!< where 0 and 1 doesn't have any effect*/
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.numSliceASO = pComponentPrivate->numSliceASO;
        /*!< Array containing the order of slices in which they should
        //!< be present in bit-stream. vaild enteries are [0, any entry lesser than numSlicesASO]*/
        for( i=0; i<MAXNUMSLCGPS;i++)
            ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.asoSliceOrder[i] = pComponentPrivate->asoSliceOrder[i];

        /*< Following Parameter are related to Flexible macro block ordering (FMO)*/
        /*< Total Number of slice groups, valid enteries are [0,8]*/
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.numSliceGroups = pComponentPrivate->numSliceGroups;
        /*< Slice GroupMapType : For Valid enteries see IH264VENC_SliceGroupMapType*/
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.sliceGroupMapType = pComponentPrivate->sliceGroupMapType;
        /*< Slice Group Change Direction Flag: Only valid when sliceGroupMapType
        //!< is equal to IH264_RASTER_SCAN_SLICE_GRP.
        //!< For valid values refer IH264VENC_SliceGroupChangeDirection*/
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.sliceGroupChangeDirectionFlag = pComponentPrivate->sliceGroupChangeDirectionFlag;
        /*< Slice Group Change Rate: Only valid when sliceGroupMapType
        //!< is equal to IH264_RASTER_SCAN_SLICE_GRP.
        //!< valid values are : [0, factor of number of Mbs in a row]*/
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.sliceGroupChangeRate = pComponentPrivate->sliceGroupChangeRate;
        /*< Slice Group Change Cycle: Only valid when sliceGroupMapType
        //!< is equal to IH264_RASTER_SCAN_SLICE_GRP.
        //!< Valid values can be 0 to numMbsRowsInPicture, also constrained
        //!< by sliceGroupChangeRate*sliceGroupChangeCycle < totalMbsInFrame*/
        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.sliceGroupChangeCycle = pComponentPrivate->sliceGroupChangeCycle;
        /*< This field is useful in case of sliceGroupMapType equal to either
        //!< IH264_INTERLEAVED_SLICE_GRP or IH264_FOREGRND_WITH_LEFTOVER_SLICE_GRP
        //!< In both cases it has different meaning:
        //!< In case of IH264_INTERLEAVED_SLICE_GRP:
        //!< The i-th entery in this array is used to specify the number of consecutive
        //!< slice group macroblocks to be assigned to the i-th slice group in
        //!< raster scan order of slice group macroblock units.
        //!< Valid values are 0 to totalMbsInFrame again constrained by sum of all the elements
        //!< shouldn't exceed totalMbsInFrame
        //!< In case of IH264_FOREGRND_WITH_LEFTOVER_SLICE_GRP:
        //!< First entry in the array specify the start position of foreground region in terms
        //!< of macroblock number, valid values are [0, totalMbsInFrame-1]
        //!< Second entry in the array specify the end position of foreground region in terms
        //!< of macroblock number, valid values are [0, totalMbsInFrame-1] with following constrains:
        //!< endPos > startPos && endPos%mbsInOneRow > startPos%mbsInOneRow*/
        for( i=0; i<MAXNUMSLCGPS;i++)
        {
            ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->H264VENC_TI_DYNAMICPARAMS.sliceGroupParams[i] = pComponentPrivate->sliceGroupParams[i];
        }

        ((H264VE_GPP_SN_UALGInputParams*)pUalgInpParams)->ulFrameIndex = pComponentPrivate->nFrameCnt;

        pComponentPrivate->bForceIFrame = 0;
        ++pComponentPrivate->nFrameCnt;

        printH264UAlgInParam(pUalgInpParams, 0, &pComponentPrivate->dbg);
        OMX_CONF_CIRCULAR_BUFFER_MOVE_TAIL(pBufHead,
                                           pComponentPrivate->sCircularBuffer,
                                           pComponentPrivate);

       /*Send Buffer to LCML*/
        OMX_PRBUFFER1(pComponentPrivate->dbg, " %p\n", (void*)pBufHead);
        pBufferPrivate->eBufferOwner = VIDENC_BUFFER_WITH_DSP;
        eError = LCML_QueueBuffer(pLcmlHandle->pCodecinterfacehandle,
                                  EMMCodecInputBuffer,
                                  pBufHead->pBuffer,
                                  pBufHead->nAllocLen,
                                  pBufHead->nFilledLen,
                                  (OMX_U8*)pUalgInpParams,
                                  sizeof(H264VE_GPP_SN_UALGInputParams),
                                  (OMX_U8*)pBufHead);
        if (eError != OMX_ErrorNone)
        {
            OMX_PRDSP4(pComponentPrivate->dbg, "LCML QueueBuffer failed: %x\n", eError);
            OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorHardware);
        }

    }
    else if (pPortDefOut->format.video.eCompressionFormat == OMX_VIDEO_CodingMPEG4 ||
             pPortDefOut->format.video.eCompressionFormat == OMX_VIDEO_CodingH263)
    {
        eError = OMX_VIDENC_Queue_Mpeg4_Buffer(pComponentPrivate, pBufHead);
    }
    else
    {
        OMX_PRBUFFER4(pComponentPrivate->dbg, "Unsupported compression format (%d)\n",
                      pPortDefOut->format.video.eCompressionFormat);
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorUnsupportedSetting);
    }
OMX_CONF_CMD_BAIL:
EXIT:
    return eError;
}

OMX_ERRORTYPE OMX_VIDENC_Queue_Mpeg4_Buffer(VIDENC_COMPONENT_PRIVATE* pComponentPrivate, OMX_BUFFERHEADERTYPE* pBufHead)
{
    MP4VE_GPP_SN_UALGInputParams* pUalgInpParams = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    VIDENC_BUFFER_PRIVATE* pBufferPrivate;
    LCML_DSP_INTERFACE* pLcmlHandle = NULL;
    VIDEOENC_PORT_TYPE* pCompPortOut = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefOut = NULL;

    pBufferPrivate = (VIDENC_BUFFER_PRIVATE*)pBufHead->pInputPortPrivate;
    pCompPortOut = pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT];
    pPortDefOut = pCompPortOut->pPortDef;
    pLcmlHandle = (LCML_DSP_INTERFACE*)pComponentPrivate->pLCML;

    pUalgInpParams = (MP4VE_GPP_SN_UALGInputParams*)pBufferPrivate->pUalgParam;
    OMX_DBG_CHECK_CMD(pComponentPrivate->dbg, pUalgInpParams, 1, 1);

    pUalgInpParams->ulFrameIndex         = pComponentPrivate->nFrameCnt;
    pUalgInpParams->ulTargetFrameRate    = pComponentPrivate->nTargetFrameRate;
    pUalgInpParams->ulTargetBitRate      = pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pBitRateTypeConfig->nEncodeBitrate;
    pUalgInpParams->ulGenerateHeader     = 0;
    pUalgInpParams->ulForceIFrame        = pComponentPrivate->bForceIFrame;
    pUalgInpParams->ulResyncInterval     = pCompPortOut->pErrorCorrectionType->nResynchMarkerSpacing;
    if(pCompPortOut->pErrorCorrectionType->bEnableHEC)
        pUalgInpParams->ulHecInterval    = 3;
    else
    pUalgInpParams->ulHecInterval        = 0;
    pUalgInpParams->ulAIRRate            = pCompPortOut->pIntraRefreshType->nAirRef;
    pUalgInpParams->ulMIRRate            = pComponentPrivate->nMIRRate;
    pUalgInpParams->ulfCode              = 5;
    pUalgInpParams->ulHalfPel            = 1;
    pUalgInpParams->ul4MV                = 0;
    pUalgInpParams->ulIntraFrameInterval = pComponentPrivate->nIntraFrameInterval;

    /*Set nQPI Value*/
    if (pUalgInpParams->ulForceIFrame == OMX_TRUE)
    {
        pUalgInpParams->ulQPIntra = pComponentPrivate->nQPI;
    }
    else
    {
        pUalgInpParams->ulQPIntra    = 0;
    }

    /*Set segment mode params*/
    if (pComponentPrivate->bMVDataEnable)
    {
        pUalgInpParams->ul4MV                 =1;
        pUalgInpParams->uluseUMV              =1;
        pUalgInpParams->ulMVDataEnable        =1;
    }
    else
    {
        pUalgInpParams->ul4MV                 =0;
        pUalgInpParams->uluseUMV              =0;
        pUalgInpParams->ulMVDataEnable        =0;
    }
    if (pComponentPrivate->bResyncDataEnable)
        pUalgInpParams->ulResyncDataEnable    =1;
    else
        pUalgInpParams->ulResyncDataEnable    =0;
    /* Reset bForceMPEG4IFrame to zero */
    pComponentPrivate->bForceIFrame = OMX_FALSE;

    /*Set ulACPred Value*/
    if (pPortDefOut->format.video.eCompressionFormat == OMX_VIDEO_CodingMPEG4)
    {
        pUalgInpParams->ulACPred         = pComponentPrivate->pMpeg4->bACPred;
    }
    else
    {
        pUalgInpParams->ulACPred        = 0;
    }
    pUalgInpParams->ulQPInter           = 8;
    pUalgInpParams->ulLastFrame         = 0;
    pUalgInpParams->ulcapturewidth      = 0;
    pUalgInpParams->ulQpMax             = 31;
    pUalgInpParams->ulQpMin             = 2;
    ++pComponentPrivate->nFrameCnt;

    if(pComponentPrivate->bRequestVOLHeader == OMX_TRUE)
    {
        /*In the case of Mpeg4 we have to send an extra Buffer to LCML requesting for VOL Header*/
        memcpy(pComponentPrivate->pTempUalgInpParams,pUalgInpParams,sizeof(MP4VE_GPP_SN_UALGInputParams));
        pComponentPrivate->pTempUalgInpParams->ulGenerateHeader = 1;
        pBufferPrivate->eBufferOwner = VIDENC_BUFFER_WITH_DSP;
        eError = LCML_QueueBuffer(pLcmlHandle->pCodecinterfacehandle,
                                  EMMCodecInputBuffer,
                                  pComponentPrivate->pTempUalgInpParams,/*send any buffer*/
                                  1,
                                  0,
                                  pComponentPrivate->pTempUalgInpParams,
                                  sizeof(MP4VE_GPP_SN_UALGInputParams),
                                  (OMX_U8*)pBufHead);
        if (eError != OMX_ErrorNone)
        {
            OMX_PRDSP2(pComponentPrivate->dbg, "LCML QueueBuffer failed: %x\n", eError);
        }
        pComponentPrivate->bRequestVOLHeader = OMX_FALSE;
    }

    OMX_PRDSP1(pComponentPrivate->dbg,
               "TargetFrameRate -> %d\n\
               TargetBitRate   -> %d\n\
               QPI             -> %d\n", pComponentPrivate->nTargetFrameRate,
               (int)pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pBitRateTypeConfig->nEncodeBitrate,
               pComponentPrivate->nQPI);

    printMpeg4UAlgInParam(pUalgInpParams, 0, &pComponentPrivate->dbg);
    OMX_CONF_CIRCULAR_BUFFER_MOVE_TAIL(pBufHead,
                                           pComponentPrivate->sCircularBuffer,
                                           pComponentPrivate);

        /*Send Buffer to LCML*/
        OMX_PRBUFFER1(pComponentPrivate->dbg, " %p\n", (void*)pBufHead);
        pBufferPrivate->eBufferOwner = VIDENC_BUFFER_WITH_DSP;
        eError = LCML_QueueBuffer(pLcmlHandle->pCodecinterfacehandle,
                                  EMMCodecInputBuffer,
                                  pBufHead->pBuffer,
                                  pBufHead->nAllocLen,
                                  pBufHead->nFilledLen,
                                  (OMX_U8*)pUalgInpParams,
                                  sizeof(MP4VE_GPP_SN_UALGInputParams),
                                  (OMX_U8*)pBufHead);
    if (eError != OMX_ErrorNone)
    {
        OMX_PRDSP4(pComponentPrivate->dbg, "LCML QueueBuffer failed: %x\n", eError);
    }

OMX_CONF_CMD_BAIL:

    return eError;
}


/*---------------------------------------------------------------------------------------*/
/**
  * OMX_VIDENC_Process_FilledOutBuf()
  *
  * Called by component thread, handles filled output buffers from DSP.
  *
  * @param pComponentPrivate private component structure for this instance of the component
  *
  * @retval OMX_ErrorNone                  success, ready to roll
  *         OMX_ErrorInsufficientResources if the malloc fails
  **/
/*---------------------------------------------------------------------------------------*/

OMX_ERRORTYPE OMX_VIDENC_Process_FilledOutBuf(VIDENC_COMPONENT_PRIVATE* pComponentPrivate, OMX_BUFFERHEADERTYPE* pBufHead)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    VIDENC_BUFFER_PRIVATE* pBufferPrivate = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefOut = NULL;
    OMX_OTHER_EXTRADATATYPE_1_1_2* pExtraDataType;
    H264VE_GPP_SN_UALGOutputParams* pSNPrivateParams;
    OMX_U8* pTemp;
    OMX_U32* pIndexNal;

    OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);

    pPortDefOut = pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pPortDef;

   if (pComponentPrivate->bCodecStarted == OMX_TRUE)
    {

        pBufferPrivate = pBufHead->pOutputPortPrivate;
        pSNPrivateParams = (H264VE_GPP_SN_UALGOutputParams*)(pBufferPrivate->pUalgParam);
        pBufHead->nFilledLen = ((H264VE_GPP_SN_UALGOutputParams*)pBufferPrivate->pUalgParam)->ulBitstreamSize;

        if (pPortDefOut->format.video.eCompressionFormat == OMX_VIDEO_CodingAVC)
        {
            pBufHead->nFlags &= ~OMX_BUFFERFLAG_CODECCONFIG;

        /*Copy Buffer Data to be propagated*/
        if((pComponentPrivate->AVCNALFormat == VIDENC_AVC_NAL_SLICE) &&
                (pSNPrivateParams->ulNALUnitsPerFrame != (pSNPrivateParams->ulNALUnitIndex+1)) &&
                (pSNPrivateParams->ulNALUnitsPerFrame != 0))
        {

            pBufHead->pMarkData = NULL;
            pBufHead->hMarkTargetComponent = NULL;
            pBufHead->nTickCount = pComponentPrivate->sCircularBuffer.pHead->nTickCount;
            pBufHead->nTimeStamp = pComponentPrivate->sCircularBuffer.pHead->nTimeStamp;
            pBufHead->nFlags = 0;
        }
            else
            {
                OMX_CONF_CIRCULAR_BUFFER_MOVE_HEAD(pBufHead, pComponentPrivate->sCircularBuffer,
                                                   pComponentPrivate);
            pBufHead->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
        }


            /* Set lFrameType*/
            if (((H264VE_GPP_SN_UALGOutputParams*)pBufferPrivate->pUalgParam)->lFrameType == OMX_LFRAMETYPE_H264 ||
                 ((H264VE_GPP_SN_UALGOutputParams*)pBufferPrivate->pUalgParam)->lFrameType == OMX_LFRAMETYPE_IDR_H264)
            {
                /* IDR Frame */
                OMX_S32 nalType = pBufHead->pBuffer[0] & 0x1F;
                if (nalType == SPS_CODE_PREFIX || nalType == PPS_CODE_PREFIX) {
                    /* Need to drop subsequent SPS or PPS NAL unit since opencore does not
                     * correctly handle storage */
                    if (!pComponentPrivate->bSentFirstSpsPps) {
                        if (nalType == SPS_CODE_PREFIX) {
                            // Save SPS and send it along with PPS later in a single buffer
                            // Workaround to send a 0-length buffer first.
                            // Ideally, we should not send a buffer at all.
                            pComponentPrivate->sps = malloc(4 + pBufHead->nFilledLen);
                            pComponentPrivate->spsLen = 4 + pBufHead->nFilledLen;
                            memcpy(pComponentPrivate->sps, "\x00\x00\x00\x01", 4);
                            memcpy(pComponentPrivate->sps + 4, pBufHead->pBuffer, pBufHead->nFilledLen);
                            pBufHead->nFilledLen = 0;
                        }

                        /* we can assume here that PPS always comes second */
                        if (nalType == PPS_CODE_PREFIX) {
                            pComponentPrivate->bSentFirstSpsPps = OMX_TRUE;
                            if (pComponentPrivate->sps == NULL ||
                                pComponentPrivate->spsLen == 0) {
                                OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorUndefined);
                            }
                            memmove(pBufHead->pBuffer + pComponentPrivate->spsLen + 4,
                                    pBufHead->pBuffer, pBufHead->nFilledLen);
                            memmove(pBufHead->pBuffer,
                                    pComponentPrivate->sps, pComponentPrivate->spsLen);
                            memcpy(pBufHead->pBuffer + pComponentPrivate->spsLen, "\x00\x00\x00\x01", 4);
                            pBufHead->nFilledLen += pComponentPrivate->spsLen + 4;
                            pBufHead->nFlags |= OMX_BUFFERFLAG_CODECCONFIG;
                            free(pComponentPrivate->sps);
                            pComponentPrivate->sps = NULL;
                            pComponentPrivate->spsLen = 0;
                        }
                    } else {
                        pBufHead->nFilledLen = 0;
                    }
                }

                pBufHead->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;
            }

            /* if NAL frame mode */
            if (pComponentPrivate->AVCNALFormat == VIDENC_AVC_NAL_FRAME)
            {

                /*H264VE_GPP_SN_UALGOutputParams* pSNPrivateParams;*/
                int nNalSlices;

                pBufHead->nFlags |= OMX_BUFFERFLAG_EXTRADATA;

                pTemp = pBufHead->pBuffer + pBufHead->nOffset + pBufHead->nFilledLen + 3;
                pExtraDataType = (OMX_OTHER_EXTRADATATYPE_1_1_2*) (((OMX_U32) pTemp) & ~3);
                pIndexNal = (OMX_U32*)(pExtraDataType->data);
                /*pSNPrivateParams = (H264VE_GPP_SN_UALGOutputParams*)(pBufferPrivate->pUalgParam);*/

                pExtraDataType->nVersion.s.nVersionMajor = 1;
                pExtraDataType->nVersion.s.nVersionMinor = 1;
                pExtraDataType->nVersion.s.nRevision = 2;
                pExtraDataType->nPortIndex = VIDENC_OUTPUT_PORT;
                pExtraDataType->eType = OMX_ExtraDataQuantization;
                pExtraDataType->nDataSize = (1+pSNPrivateParams->ulNALUnitsPerFrame)*sizeof(OMX_U32);

                *pIndexNal = pSNPrivateParams->ulNALUnitsPerFrame;
                pIndexNal++;
                for (nNalSlices = 0; (OMX_U32)nNalSlices < pSNPrivateParams->ulNALUnitsPerFrame; nNalSlices++, pIndexNal++)
                {

                    *pIndexNal = (OMX_U32)(pSNPrivateParams->ulNALUnitsSizes[nNalSlices]);
                }

                pTemp = (OMX_U8*)((((OMX_U32)pIndexNal)+3) & ~3);
                pExtraDataType->nSize = (OMX_U32)pTemp-(OMX_U32)pExtraDataType;

                pExtraDataType = (OMX_OTHER_EXTRADATATYPE_1_1_2*) pTemp;
                pExtraDataType->nSize = (sizeof(OMX_OTHER_EXTRADATATYPE_1_1_2)+3) & ~3;
                pExtraDataType->nVersion.s.nVersionMajor = 1;
                pExtraDataType->nVersion.s.nVersionMinor = 1;
                pExtraDataType->nVersion.s.nRevision = 2;
                pExtraDataType->nPortIndex = VIDENC_OUTPUT_PORT;
                pExtraDataType->eType = OMX_ExtraDataNone;
                pExtraDataType->nDataSize = 0;
            }
        }
        else if (pPortDefOut->format.video.eCompressionFormat == OMX_VIDEO_CodingMPEG4 ||
                 pPortDefOut->format.video.eCompressionFormat == OMX_VIDEO_CodingH263)
        {
            pBufHead->nFlags &= ~OMX_BUFFERFLAG_CODECCONFIG;

            /*We ignore the first Mpeg4 buffer which contains VOL Header since we did not add it to the circular list*/
            if(pComponentPrivate->bWaitingForVOLHeaderBuffer == OMX_FALSE)
            {
                OMX_CONF_CIRCULAR_BUFFER_MOVE_HEAD(pBufHead, pComponentPrivate->sCircularBuffer,
                                                   pComponentPrivate);
            }
            else
            {
                pComponentPrivate->bWaitingForVOLHeaderBuffer = OMX_FALSE;
                pBufHead->nFlags |= OMX_BUFFERFLAG_CODECCONFIG;
            }

            pBufHead->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;

            /* Set cFrameType*/
            if (((MP4VE_GPP_SN_UALGOutputParams*)pBufferPrivate->pUalgParam)->cFrameType == OMX_CFRAMETYPE_MPEG4)
            {
                /* I-VOP Frame */
                pBufHead->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;
            }
            VIDENC_MPEG4_SEGMENTMODE_METADATA* pMetaData;
            /*copy MPEG4 segment mode meta data */
            pMetaData=(VIDENC_MPEG4_SEGMENTMODE_METADATA*)pBufferPrivate->pMetaData;
            if (pComponentPrivate->bMVDataEnable==OMX_TRUE)
            {
               pMetaData->mvDataSize=((MP4VE_GPP_SN_UALGOutputParams*)pBufferPrivate->pUalgParam)->mvDataSize;
               pMetaData->pMVData=((MP4VE_GPP_SN_UALGOutputParams*)pBufferPrivate->pUalgParam)->MVData;
            }
            if (pComponentPrivate->bResyncDataEnable==OMX_TRUE)
            {
               pMetaData->pResyncData=((MP4VE_GPP_SN_UALGOutputParams*)pBufferPrivate->pUalgParam)->ResyncData;
               pMetaData->numPackets=((MP4VE_GPP_SN_UALGOutputParams*)pBufferPrivate->pUalgParam)->numPackets;
            }
        }
        else
        {
            OMX_PRBUFFER4(pComponentPrivate->dbg, "Unsupported compression format (%d)\n",
                          pPortDefOut->format.video.eCompressionFormat);
            OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorUnsupportedSetting);
        }

        if (pBufHead->nFlags & OMX_BUFFERFLAG_EOS)
        {
            /* trigger event handler */
            OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,  OMX_EventBufferFlag, 0x1, pBufHead->nFlags, NULL);
            /* clear flag */
            pComponentPrivate->nFlags = 0;
        }

        if (pBufHead->pMarkData != NULL)
        {
            /* trigger event handler if we are supposed to */
            if (pBufHead->hMarkTargetComponent == pComponentPrivate->pHandle &&
                pBufHead->pMarkData)
            {
                OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,  OMX_EventMark, 0x0, 0x0,
                                        pBufHead->pMarkData);
            }
        }
        pBufferPrivate->eBufferOwner = VIDENC_BUFFER_WITH_CLIENT;
#ifdef __PERF_INSTRUMENTATION__
        PERF_SendingBuffer(pComponentPrivate->pPERFcomp,
                           pBufHead->pBuffer,
                           pBufHead->nFilledLen,
                           PERF_ModuleHLMM);
#endif
        pComponentPrivate->sCbData.FillBufferDone(pComponentPrivate->pHandle,
                                                  pComponentPrivate->pHandle->pApplicationPrivate,
                                                  pBufHead);
        OMX_VIDENC_IncrementBufferCountByOne(&pComponentPrivate->FillbufferdoneCount);
        OMX_VIDENC_SignalIfAllBuffersAreReturned(pComponentPrivate);
    }
OMX_CONF_CMD_BAIL:
    return eError;
}

/*---------------------------------------------------------------------------------------*/
/**
  * OMX_VIDENC_Process_FreeInBuf()
  *
  * Called by component thread, handles free input buffers from DSP.
  *
  * @param pComponentPrivate private component structure for this instance of the component
  *
  * @retval OMX_ErrorNone                  success, ready to roll
  *         OMX_ErrorInsufficientResources if the malloc fails
  **/
/*---------------------------------------------------------------------------------------*/

OMX_ERRORTYPE OMX_VIDENC_Process_FreeInBuf(VIDENC_COMPONENT_PRIVATE* pComponentPrivate, OMX_BUFFERHEADERTYPE* pBufHead)
{
    VIDENC_NODE* pMemoryListHead = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_HANDLETYPE hTunnelComponent = NULL;
    VIDENC_BUFFER_PRIVATE* pBufferPrivate = NULL;

    OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);

    hTunnelComponent = pComponentPrivate->pCompPort[VIDENC_INPUT_PORT]->hTunnelComponent;
    pMemoryListHead = pComponentPrivate->pMemoryListHead;

    /*pBufHead is checked for NULL*/
    OMX_DBG_CHECK_CMD(pComponentPrivate->dbg, pBufHead, 1, 1);
    pBufferPrivate = pBufHead->pInputPortPrivate;

    if (hTunnelComponent != NULL)
    {
        pBufferPrivate->eBufferOwner = VIDENC_BUFFER_WITH_TUNNELEDCOMP;
#ifdef __PERF_INSTRUMENTATION__
        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                          PREF(pBufHead,pBuffer),
                          pBufHead->nFilledLen,
                          PERF_ModuleLLMM);
#endif

        eError = OMX_FillThisBuffer(hTunnelComponent, pBufHead);
        OMX_DBG_BAIL_IF_ERROR(eError, pComponentPrivate->dbg,
                              OMX_PRBUFFER4, "FillThisBuffer failed (%x)", eError);
    }
    else
    {
        pBufferPrivate->eBufferOwner = VIDENC_BUFFER_WITH_CLIENT;
#ifdef __PERF_INSTRUMENTATION__
        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                          PREF(pBufHead,pBuffer),
                          0,
                          PERF_ModuleHLMM);
#endif
        pComponentPrivate->sCbData.EmptyBufferDone(pComponentPrivate->pHandle,
                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                   pBufHead);
        OMX_VIDENC_IncrementBufferCountByOne(&pComponentPrivate->EmptybufferdoneCount);
        OMX_VIDENC_SignalIfAllBuffersAreReturned(pComponentPrivate);
   }

OMX_CONF_CMD_BAIL:
    return eError;
}
/*---------------------------------------------------------------------------------------*/
/**
  *  Function to initialize LCML
  *
  *
  *
  * @retval OMX_NoError              Success, ready to roll
  *
  **/
/*---------------------------------------------------------------------------------------*/

OMX_ERRORTYPE OMX_VIDENC_InitLCML(VIDENC_COMPONENT_PRIVATE* pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_HANDLETYPE hLCML = NULL;
    VIDENC_NODE* pMemoryListHead = NULL;
#ifdef UNDER_CE
    typedef OMX_ERRORTYPE (*LPFNDLLFUNC1)(OMX_HANDLETYPE);
    LPFNDLLFUNC1 fpGetHandle1;
#else
    void* pMyLCML = NULL;
    fpo   fpGetHandle = NULL;
    char* error = NULL;
#endif

    OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);

#ifndef UNDER_CE
    pMyLCML = dlopen("libLCML.so", RTLD_LAZY);
    pComponentPrivate->pModLcml = pMyLCML;
    if (!pMyLCML)
    {
        OMX_ERROR5(pComponentPrivate->dbg, "Could not open LCML library\n");
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorUndefined);
    }

    fpGetHandle = dlsym(pMyLCML, "GetHandle");
    if ((error = dlerror()) != NULL)
    {
        OMX_ERROR4(pComponentPrivate->dbg, "No GetHandle in LCML library\n");
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorUndefined);
    }

    eError = (*fpGetHandle)(&hLCML);
    if (eError != OMX_ErrorNone)
    {
        OMX_ERROR5(pComponentPrivate->dbg, "Error While Getting LCML Handle (%x)...\n", eError);
       OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorUndefined);
    }

    pComponentPrivate->pLCML = (LCML_DSP_INTERFACE*)hLCML;
    pMemoryListHead = pComponentPrivate->pMemoryListHead;
    pComponentPrivate->pLCML->pComponentPrivate = (VIDENC_COMPONENT_PRIVATE *)pComponentPrivate;

#else
    g_hLcmlDllHandle = LoadLibraryEx(TEXT("oaf_bml.dll"), NULL, 0);
    if (g_hLcmlDllHandle == NULL)
    {
        OMX_ERROR5(pComponentPrivate->dbg, "BML Load Failed!!!, %d\n", GetLastError());
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorUndefined);
    }
    fpGetHandle1 = (LPFNDLLFUNC1)GetProcAddress(g_hLcmlDllHandle,TEXT("GetHandle"));
    if (!fpGetHandle1)
    {
        FreeLibrary(g_hLcmlDllHandle);
        g_hLcmlDllHandle = NULL;
        OMX_ERROR4(pComponentPrivate->dbg, "No GetHandle in BML\n");
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorUndefined);
    }
    eError = fpGetHandle1(&hLCML);
    if (eError != OMX_ErrorNone)
    {
        FreeLibrary(g_hLcmlDllHandle);
        g_hLcmlDllHandle = NULL;
        OMX_ERROR5(pComponentPrivate->dbg, "Error While Getting LCML Handle (%x)...\n", eError);
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorUndefined);
    }
    (LCML_DSP_INTERFACE*)pComponentPrivate->pLCML = (LCML_DSP_INTERFACE*)hLCML;
    pComponentPrivate->pLCML->pComponentPrivate = (VIDENC_COMPONENT_PRIVATE *)pComponentPrivate;
#endif
OMX_CONF_CMD_BAIL:
    return eError;
}

/*---------------------------------------------------------------------------------------*/
/**
  *  Function to fill DSP structures via LCML
  *
  *
  *
  * @retval OMX_NoError              Success, ready to roll
  *
  **/
/*---------------------------------------------------------------------------------------*/

OMX_ERRORTYPE OMX_VIDENC_InitDSP_H264Enc(VIDENC_COMPONENT_PRIVATE* pComponentPrivate)
{
    OMX_U16 nArr[100];
    OMX_U32* pTmp = NULL;
    LCML_CALLBACKTYPE sCb;
    LCML_DSP* pLcmlDSP = NULL;
    VIDENC_NODE* pMemoryListHead = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_VIDEO_PARAM_AVCTYPE* pH264 = NULL;
    LCML_DSP_INTERFACE* pLcmlHandle = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefIn = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefOut = NULL;
    OMX_VIDEO_PARAM_BITRATETYPE* pVidParamBitrate = NULL;
    OMX_VIDEO_PARAM_QUANTIZATIONTYPE* pQuantization = NULL;
    H264VE_GPP_SN_Obj_CreatePhase* pCreatePhaseArgs = NULL;
    /* OMX_VIDEO_CONFIG_AVCINTRAPERIOD* pH264IntraPeriod = NULL; */
    OMX_VIDEO_PARAM_MOTIONVECTORTYPE* pMotionVector = NULL;
    OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);

    pPortDefIn = pComponentPrivate->pCompPort[VIDENC_INPUT_PORT]->pPortDef;
    pPortDefOut = pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pPortDef;
    pH264 = pComponentPrivate->pH264;
    pVidParamBitrate = pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pBitRateType;
    pQuantization = pComponentPrivate->pQuantization;
    pMemoryListHead = pComponentPrivate->pMemoryListHead;
    /* pH264IntraPeriod = pComponentPrivate->pH264IntraPeriod; */
    pMotionVector = pComponentPrivate->pMotionVector;
    pComponentPrivate->bErrorLcmlHandle = OMX_FALSE;

    pLcmlHandle = (LCML_DSP_INTERFACE*)pComponentPrivate->pLCML;
    pLcmlDSP = (((LCML_DSP_INTERFACE*)pLcmlHandle)->dspCodec);
    pLcmlDSP = (((LCML_DSP_INTERFACE*)pLcmlHandle)->dspCodec);

    pLcmlDSP->In_BufInfo.nBuffers           = pPortDefIn->nBufferCountActual;
    pLcmlDSP->In_BufInfo.nSize              = pComponentPrivate->nInBufferSize;
    pLcmlDSP->In_BufInfo.DataTrMethod       = DMM_METHOD;

    pLcmlDSP->Out_BufInfo.nBuffers          = pPortDefOut->nBufferCountActual;
    pLcmlDSP->Out_BufInfo.nSize             = pComponentPrivate->nOutBufferSize;
    pLcmlDSP->Out_BufInfo.DataTrMethod      = DMM_METHOD;

    pLcmlDSP->NodeInfo.nNumOfDLLs           = OMX_H264ENC_NUM_DLLS;
    pLcmlDSP->NodeInfo.AllUUIDs[0].uuid     = &H264VESOCKET_TI_UUID;
    strcpy ((char *)pLcmlDSP->NodeInfo.AllUUIDs[0].DllName,H264_ENC_NODE_DLL);
    pLcmlDSP->NodeInfo.AllUUIDs[0].eDllType = DLL_NODEOBJECT;

    pLcmlDSP->NodeInfo.AllUUIDs[1].uuid     = &H264VESOCKET_TI_UUID;
    strcpy ((char *)pLcmlDSP->NodeInfo.AllUUIDs[1].DllName,H264_ENC_NODE_DLL);
    pLcmlDSP->NodeInfo.AllUUIDs[1].eDllType = DLL_DEPENDENT;

    pLcmlDSP->NodeInfo.AllUUIDs[2].uuid     = &USN_UUID;
    strcpy ((char *)pLcmlDSP->NodeInfo.AllUUIDs[2].DllName,USN_DLL);
    pLcmlDSP->NodeInfo.AllUUIDs[2].eDllType = DLL_DEPENDENT;

    pLcmlDSP->SegID                         = 0;
    pLcmlDSP->Timeout                       = -1;
    pLcmlDSP->Alignment                     = 0;
    pLcmlDSP->Priority                      = 5;

    #ifdef GPP_PRIVATE_NODE_HEAP
        if ((pPortDefIn->format.video.nFrameWidth <= 176) &&
            (pPortDefIn->format.video.nFrameHeight <= 144))
        {
            pLcmlDSP->ProfileID = 0;
        }
        else if ((pPortDefIn->format.video.nFrameWidth <= 352) &&
                 (pPortDefIn->format.video.nFrameHeight <= 288))
        {
            pLcmlDSP->ProfileID = 1;
        }
        else
        {
            pLcmlDSP->ProfileID = 2;
        }
    #else
        pLcmlDSP->ProfileID = 0xff; /* Use DSP node heap */
    #endif

    /* pLcmlDSP->buffindx                      = 999; */

    VIDENC_MALLOC(pCreatePhaseArgs,
                  sizeof(H264VE_GPP_SN_Obj_CreatePhase),
                  H264VE_GPP_SN_Obj_CreatePhase,
                  pMemoryListHead,
                  pComponentPrivate->dbg);

    pCreatePhaseArgs->usNumStreams            = 2;
    pCreatePhaseArgs->usStreamId              = VIDENC_INPUT_PORT;
    pCreatePhaseArgs->usBuffTypeInStream      = 0;
    pCreatePhaseArgs->usMaxBuffsInStream      = (OMX_U16)pPortDefIn->nBufferCountActual;
    pCreatePhaseArgs->usStreamId2             = VIDENC_OUTPUT_PORT;
    pCreatePhaseArgs->usBuffTypeInStream2     = 0;
    pCreatePhaseArgs->usMaxBuffsInStream2     = (OMX_U16)pPortDefOut->nBufferCountActual;

    pCreatePhaseArgs->ulWidth                 = pPortDefIn->format.video.nFrameWidth;
    pCreatePhaseArgs->ulHeight                = pPortDefIn->format.video.nFrameHeight;
    pCreatePhaseArgs->ulTargetBitRate         = pPortDefOut->format.video.nBitrate;
    pCreatePhaseArgs->ulBitstreamBuffSize     = pComponentPrivate->nOutBufferSize;
    pCreatePhaseArgs->ulFrameRate             = (unsigned int)(Q16Tof(pPortDefIn->format.video.xFramerate)*1000.0);

    /* set run-time frame and bit rates to create-time values */
    pComponentPrivate->nTargetFrameRate       = pCreatePhaseArgs->ulFrameRate;
    pComponentPrivate->nPrevTargetFrameRate   = 0;
    pComponentPrivate->bSentFirstSpsPps       = OMX_FALSE;

    if (pPortDefIn->format.video.eColorFormat == OMX_COLOR_FormatYUV420Planar)
    {
        pCreatePhaseArgs->ucYUVFormat         = 0;
    }
    else if (pPortDefIn->format.video.eColorFormat == OMX_COLOR_FormatCbYCrY) /*422 LE UYVY*/
    {
        pCreatePhaseArgs->ucYUVFormat         = 2;
    }
    else if (pPortDefIn->format.video.eColorFormat == OMX_COLOR_FormatYCbYCr) /*422 BE YUYV */
    {
        pCreatePhaseArgs->ucYUVFormat         = 1;
    }
    else
    {
        OMX_PRDSP2(pComponentPrivate->dbg, "Unsupported YUV format.\n");
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorUnsupportedSetting);
    }

    pCreatePhaseArgs->ucUnrestrictedMV        = pComponentPrivate->ucUnrestrictedMV;
    pCreatePhaseArgs->ucNumRefFrames          = 1;

    if (pVidParamBitrate->eControlRate == OMX_Video_ControlRateVariable)
    {
        pCreatePhaseArgs->ucRateControlAlgorithm  = 0;
    }
    else if (pVidParamBitrate->eControlRate == OMX_Video_ControlRateConstant)
    {
        pCreatePhaseArgs->ucRateControlAlgorithm  = 1;
    }
    else if (pVidParamBitrate->eControlRate == OMX_Video_ControlRateDisable)
    {
        pCreatePhaseArgs->ucRateControlAlgorithm  = 2;
    }
    else
    {
        OMX_PRDSP2(pComponentPrivate->dbg, "Unsupported rate control algorithm.\n");
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorUnsupportedSetting);
    }

    pCreatePhaseArgs->ucIDREnable             = 1;

    if (pComponentPrivate->bDeblockFilter == OMX_FALSE)
    {
        pCreatePhaseArgs->ucDeblockingEnable  = 0;
    }
    else if (pComponentPrivate->bDeblockFilter == OMX_TRUE)
    {
        pCreatePhaseArgs->ucDeblockingEnable  = 1;
    }
    else
    {
        OMX_PRDSP2(pComponentPrivate->dbg, "Unsupported deblocking setting.\n");
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorBadParameter);
    }

    pCreatePhaseArgs->ucMVRange               = (pMotionVector->sXSearchRange > pMotionVector->sYSearchRange ? pMotionVector->sXSearchRange : pMotionVector->sYSearchRange);
    pCreatePhaseArgs->ucQPIFrame              = 28;
    pCreatePhaseArgs->ucProfile               = 66;
    pCreatePhaseArgs->ulIntraFramePeriod      = pCreatePhaseArgs->ulFrameRate > 15000 ? 29 : 14;

    if (pH264->eLevel == OMX_VIDEO_AVCLevel1b)
    {
        pCreatePhaseArgs->ucLevel = 9;
    }
    else if (pH264->eLevel == OMX_VIDEO_AVCLevel1)
    {
        pCreatePhaseArgs->ucLevel = 10;
    }
    else if (pH264->eLevel == OMX_VIDEO_AVCLevel11)
    {
        pCreatePhaseArgs->ucLevel = 11;
    }
    else if (pH264->eLevel == OMX_VIDEO_AVCLevel12)
    {
        pCreatePhaseArgs->ucLevel = 12;
    }
    else if (pH264->eLevel == OMX_VIDEO_AVCLevel13)
    {
        pCreatePhaseArgs->ucLevel = 13;
    }
    else if (pH264->eLevel == OMX_VIDEO_AVCLevel2)
    {
        pCreatePhaseArgs->ucLevel = 20;
    }
    else if (pH264->eLevel == OMX_VIDEO_AVCLevel21)
    {
        pCreatePhaseArgs->ucLevel = 21;
    }
    else if (pH264->eLevel == OMX_VIDEO_AVCLevel22)
    {
        pCreatePhaseArgs->ucLevel = 22;
    }
    else if (pH264->eLevel == OMX_VIDEO_AVCLevel3)
    {
        pCreatePhaseArgs->ucLevel = 30;
        if ((pPortDefIn->format.video.eColorFormat == OMX_COLOR_FormatYUV420Planar) &&
            (pPortDefIn->format.video.nFrameWidth == 320) &&
            (pPortDefIn->format.video.nFrameHeight == 240))
        {
            pCreatePhaseArgs->ucQPIFrame = 0;
        }
    }
    else
    {
        OMX_DBG_SET_ERROR_BAIL(eError, OMX_ErrorUnsupportedSetting,
                               pComponentPrivate->dbg, OMX_PRDSP2,
                               "Unsupported level.\n");
    }

    /* override parameters for VGA & D1 encoding */
    if ((pPortDefIn->format.video.nFrameWidth >= 640 ||
        pPortDefIn->format.video.nFrameHeight >= 480) &&
        pCreatePhaseArgs->ulFrameRate > 15000)
    {
        pComponentPrivate->maxMVperMB = 1;
        pComponentPrivate->intra4x4EnableIdc = INTRA4x4_ISLICES;
        pComponentPrivate->nIntraFrameInterval = 30;
        pComponentPrivate->nAIRRate = 0;
        /* Encoding preset = 4 enables DSP side optimizations for high resolutions */
        pComponentPrivate->nEncodingPreset = 4;
        pCreatePhaseArgs->ulIntraFramePeriod = 0;
        /* Constant bit rate control enabled */
        pCreatePhaseArgs->ucRateControlAlgorithm = 1;
        pCreatePhaseArgs->ucLevel = 30;
    }
    /* Ensure frame rate update interval, which forces IDR frames, is same as I-Slice interval */
    pComponentPrivate->nFrameRateUpdateInterval = pComponentPrivate->nIntraFrameInterval;
    pCreatePhaseArgs->usNalCallback = pComponentPrivate->AVCNALFormat;
    pCreatePhaseArgs->ulEncodingPreset = pComponentPrivate->nEncodingPreset;
    pCreatePhaseArgs->ulRcAlgo = 0;
    pCreatePhaseArgs->endArgs = END_OF_CR_PHASE_ARGS;
    printH264CreateParams(pCreatePhaseArgs, &pComponentPrivate->dbg);

    pTmp = memcpy (nArr, pCreatePhaseArgs, sizeof(H264VE_GPP_SN_Obj_CreatePhase));
    if (pTmp == NULL)
    {
        OMX_TRACE4(pComponentPrivate->dbg, "memcpy() out of memory error.\n");
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorInsufficientResources);
    }
    pLcmlDSP->pCrPhArgs = nArr;
    sCb.LCML_Callback = (void *)OMX_VIDENC_LCML_Callback;

    eError = LCML_InitMMCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                              NULL,
                              &pLcmlHandle,
                              NULL,
                              &sCb);
    if (eError != OMX_ErrorNone)
    {
        OMX_PRDSP4(pComponentPrivate->dbg, "LCML_InitMMCodec Failed!...\n");
        /*TODO: Validate eError from LCML_InitMMCodec for ResourceExhaustionTest */
        pComponentPrivate->bErrorLcmlHandle = OMX_TRUE;
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorInsufficientResources);
    }
    pComponentPrivate->bCodecLoaded = OMX_TRUE;
    VIDENC_FREE(pCreatePhaseArgs, pMemoryListHead,
                pComponentPrivate->dbg);
OMX_CONF_CMD_BAIL:
    return eError;
}

/*---------------------------------------------------------------------------------------*/
/**
  *  Function to fill DSP structures via LCML
  *
  *
  *
  * @retval OMX_NoError              Success, ready to roll
  *
  **/
/*---------------------------------------------------------------------------------------*/

OMX_ERRORTYPE OMX_VIDENC_InitDSP_Mpeg4Enc(VIDENC_COMPONENT_PRIVATE* pComponentPrivate)
{
    OMX_U16 nArr[100];
    OMX_U32* pTmp = NULL;
    LCML_CALLBACKTYPE sCb;
    LCML_DSP* pLcmlDSP = NULL;
    VIDENC_NODE* pMemoryListHead = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    LCML_DSP_INTERFACE* pLcmlHandle = NULL;
    OMX_VIDEO_PARAM_H263TYPE* pH263 = NULL;
    OMX_VIDEO_PARAM_MPEG4TYPE* pMpeg4 = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefIn = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefOut = NULL;
    MP4VE_GPP_SN_Obj_CreatePhase* pCreatePhaseArgs = NULL;
    OMX_VIDEO_PARAM_BITRATETYPE* pVidParamBitrate = NULL;
    OMX_VIDEO_PARAM_QUANTIZATIONTYPE* pQuantization = NULL;
    VIDEOENC_PORT_TYPE* pCompPortOut            = NULL;

    OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);

    pPortDefIn = pComponentPrivate->pCompPort[VIDENC_INPUT_PORT]->pPortDef;
    pPortDefOut = pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pPortDef;
    pCompPortOut = pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT];
    pVidParamBitrate = pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pBitRateType;
    pQuantization = pComponentPrivate->pQuantization;
    pH263 = pComponentPrivate->pH263;
    pMpeg4 = pComponentPrivate->pMpeg4;
    pMemoryListHead = pComponentPrivate->pMemoryListHead;

    pComponentPrivate->bErrorLcmlHandle = OMX_FALSE;

    pLcmlHandle = (LCML_DSP_INTERFACE*)pComponentPrivate->pLCML;
    pLcmlDSP    = (((LCML_DSP_INTERFACE*)pLcmlHandle)->dspCodec);

    pLcmlDSP->In_BufInfo.nBuffers           = pPortDefIn->nBufferCountActual;
    pLcmlDSP->In_BufInfo.nSize              = pComponentPrivate->nInBufferSize;
    pLcmlDSP->In_BufInfo.DataTrMethod       = DMM_METHOD;

    pLcmlDSP->Out_BufInfo.nBuffers          = pPortDefOut->nBufferCountActual;
    pLcmlDSP->Out_BufInfo.nSize             = pComponentPrivate->nOutBufferSize;
    pLcmlDSP->Out_BufInfo.DataTrMethod      = DMM_METHOD;

    pLcmlDSP->NodeInfo.nNumOfDLLs           = OMX_MP4ENC_NUM_DLLS;
    pLcmlDSP->NodeInfo.AllUUIDs[0].uuid     = &MP4VESOCKET_TI_UUID;
    strcpy ((char *)pLcmlDSP->NodeInfo.AllUUIDs[0].DllName,MP4_ENC_NODE_DLL);
    pLcmlDSP->NodeInfo.AllUUIDs[0].eDllType = DLL_NODEOBJECT;

    pLcmlDSP->NodeInfo.AllUUIDs[1].uuid     = &MP4VESOCKET_TI_UUID;
    strcpy ((char *)pLcmlDSP->NodeInfo.AllUUIDs[1].DllName,MP4_ENC_NODE_DLL);
    pLcmlDSP->NodeInfo.AllUUIDs[1].eDllType = DLL_DEPENDENT;

    pLcmlDSP->NodeInfo.AllUUIDs[2].uuid     = &USN_UUID;
    strcpy ((char *)pLcmlDSP->NodeInfo.AllUUIDs[2].DllName,USN_DLL);
    pLcmlDSP->NodeInfo.AllUUIDs[2].eDllType = DLL_DEPENDENT;

    pLcmlDSP->SegID     = 0;
    pLcmlDSP->Timeout   = -1;
    pLcmlDSP->Alignment = 0;
    pLcmlDSP->Priority  = 5;

    #ifdef GPP_PRIVATE_NODE_HEAP
        if ((pPortDefIn->format.video.nFrameWidth <= 176) &&
            (pPortDefIn->format.video.nFrameHeight <= 144))
        {
            pLcmlDSP->ProfileID = 0;
        }
        else if ((pPortDefIn->format.video.nFrameWidth <= 352) &&
                 (pPortDefIn->format.video.nFrameHeight <= 288))
        {
            pLcmlDSP->ProfileID = 1;
        }
        else if ((pPortDefIn->format.video.nFrameWidth <= 640) &&
                 (pPortDefIn->format.video.nFrameHeight <= 480))
        {
            pLcmlDSP->ProfileID = 2;
        }
        else if ((pPortDefIn->format.video.nFrameWidth <= 720) &&
                 (pPortDefIn->format.video.nFrameHeight <= 480))
        {
            pLcmlDSP->ProfileID = 3;
        }
        else if ((pPortDefIn->format.video.nFrameWidth <= 720) &&
                 (pPortDefIn->format.video.nFrameHeight <= 576))
        {
            pLcmlDSP->ProfileID = 4;
        }
        else
        {
            pLcmlDSP->ProfileID = 4;
        }
    #else
        pLcmlDSP->ProfileID = 0xff; /* Use DSP node heap */
    #endif

    /* pLcmlDSP->buffindx = 999; */

    VIDENC_MALLOC(pCreatePhaseArgs,
                  sizeof(MP4VE_GPP_SN_Obj_CreatePhase),
                  MP4VE_GPP_SN_Obj_CreatePhase,
                  pMemoryListHead,
                  pComponentPrivate->dbg);

    pCreatePhaseArgs->ucUnrestrictedMV        = pComponentPrivate->ucUnrestrictedMV;
    pCreatePhaseArgs->ucProfile               = 1;

    pCreatePhaseArgs->usNumStreams            = 2;
    pCreatePhaseArgs->usStreamId              = 0;
    pCreatePhaseArgs->usBuffTypeInStream      = 0;
    pCreatePhaseArgs->usMaxBuffsInStream      = (OMX_U16)pPortDefIn->nBufferCountActual;
    pCreatePhaseArgs->usStreamId2             = 1;
    pCreatePhaseArgs->usBuffTypeInStream2     = 0;
    pCreatePhaseArgs->usMaxBuffsInStream2     = (OMX_U16)pPortDefOut->nBufferCountActual;

    pCreatePhaseArgs->ulWidth                 = pPortDefIn->format.video.nFrameWidth;
    pCreatePhaseArgs->ulHeight                = pPortDefIn->format.video.nFrameHeight;
    pCreatePhaseArgs->ulTargetBitRate         = pPortDefOut->format.video.nBitrate;
    pCreatePhaseArgs->ulVBVSize               = pComponentPrivate->nVBVSize;

    if (pPortDefOut->format.video.eCompressionFormat == OMX_VIDEO_CodingH263)
    {
        pCreatePhaseArgs->ulGOBHeadersInterval    = pH263->nGOBHeaderInterval;
    }
    else if (pPortDefOut->format.video.eCompressionFormat == OMX_VIDEO_CodingMPEG4)
    {
        pCreatePhaseArgs->ulGOBHeadersInterval    = 0;
    }

    if (pPortDefOut->format.video.eCompressionFormat == OMX_VIDEO_CodingH263)
    {
        pCreatePhaseArgs->ucIsMPEG4 = 0;
    }
    else if (pPortDefOut->format.video.eCompressionFormat == OMX_VIDEO_CodingMPEG4)
    {
        pCreatePhaseArgs->ucIsMPEG4 = 1;
        /*Initialize variables for the generation of VOL Header*/
        pComponentPrivate->bRequestVOLHeader = OMX_TRUE;
        pComponentPrivate->bWaitingForVOLHeaderBuffer = OMX_TRUE;
        pComponentPrivate->bWaitingVOLHeaderCallback = OMX_TRUE;
    }
    else
    {
        OMX_PRDSP4(pComponentPrivate->dbg, "Unsupported video format (%d).\n",
                   pPortDefOut->format.video.eCompressionFormat);
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorUnsupportedSetting);
    }

    if (pPortDefIn->format.video.eColorFormat == OMX_COLOR_FormatYUV420Planar)
    {
        pCreatePhaseArgs->ucYUVFormat         = 0;
    }
    else if (pPortDefIn->format.video.eColorFormat == OMX_COLOR_FormatCbYCrY) /*422 LE UYVY*/
    {
        pCreatePhaseArgs->ucYUVFormat         = 2;
    }
    else if (pPortDefIn->format.video.eColorFormat == OMX_COLOR_FormatYCbYCr) /*422 BE YUYV */
    {
        pCreatePhaseArgs->ucYUVFormat         = 1;
    }
    else
    {
            OMX_PRDSP2(pComponentPrivate->dbg, "Unsupported YUV format.\n");
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorUnsupportedSetting);
    }
    if(pCompPortOut->pErrorCorrectionType->bEnableHEC)
        pCreatePhaseArgs->ucHEC                   = 1;
    else
        pCreatePhaseArgs->ucHEC                   = 0;/**/

    if(pCompPortOut->pErrorCorrectionType->bEnableResync)
        pCreatePhaseArgs->ucResyncMarker          = 1;
    else
        pCreatePhaseArgs->ucResyncMarker          = 0;/**/

    if(pCompPortOut->pErrorCorrectionType->bEnableDataPartitioning)
        pCreatePhaseArgs->ucDataPartitioning      = 1;
    else
        pCreatePhaseArgs->ucDataPartitioning      = 0;/**/

    if(pCompPortOut->pErrorCorrectionType->bEnableRVLC)
        pCreatePhaseArgs->ucReversibleVLC      = 1;
    else
        pCreatePhaseArgs->ucReversibleVLC         = 0;/**/

    pCreatePhaseArgs->ucFrameRate             = (OMX_U8) Q16Tof(pPortDefIn->format.video.xFramerate);

   /* set run-time frame and bit rates to create-time values */
    pComponentPrivate->nTargetFrameRate       = pCreatePhaseArgs->ucFrameRate;
    pComponentPrivate->nPrevTargetFrameRate   = 0;
    pComponentPrivate->nTargetBitRate         = pCreatePhaseArgs->ulTargetBitRate; 

     if (pVidParamBitrate->eControlRate == OMX_Video_ControlRateConstant)
    {
        pCreatePhaseArgs->ucRateControlAlgorithm  = IVIDEO_LOW_DELAY;
    }
    else if (pVidParamBitrate->eControlRate == OMX_Video_ControlRateVariable)
    {
        pCreatePhaseArgs->ucRateControlAlgorithm  = IVIDEO_STORAGE;
    }
    else if (pVidParamBitrate->eControlRate == OMX_Video_ControlRateDisable)
    {
        pCreatePhaseArgs->ucRateControlAlgorithm  = IVIDEO_NONE;
    }
    else
    {
        OMX_PRDSP2(pComponentPrivate->dbg, "Unsupported rate control algorithm.\n");
    OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorUnsupportedSetting);
    }

    pCreatePhaseArgs->ucQPFirstIFrame         = (OMX_U8)pQuantization->nQpI;

    if (pCreatePhaseArgs->ucIsMPEG4 == 1)
    {
#ifdef __KHRONOS_CONF_1_1__
        if (pMpeg4->eLevel == OMX_VIDEO_MPEG4Level0)
        {
            pCreatePhaseArgs->ucLevel = 0;
        }
        else if (pMpeg4->eLevel == OMX_VIDEO_MPEG4Level1)
        {
            pCreatePhaseArgs->ucLevel = 1;
        }
        else if (pMpeg4->eLevel == OMX_VIDEO_MPEG4Level2)
        {
            pCreatePhaseArgs->ucLevel = 2;
        }
        else if (pMpeg4->eLevel == OMX_VIDEO_MPEG4Level3)
        {
            pCreatePhaseArgs->ucLevel = 3;
        }
        else if (pMpeg4->eLevel == OMX_VIDEO_MPEG4Level4a ||
                 pMpeg4->eLevel == OMX_VIDEO_MPEG4Level4)
        {
            pCreatePhaseArgs->ucLevel = 4;
        }
        else if (pMpeg4->eLevel == OMX_VIDEO_MPEG4Level5)
        {
            pCreatePhaseArgs->ucLevel = 5;
        }
        else if (pMpeg4->eLevel == OMX_VIDEO_MPEG4Level0b)
        {
            pCreatePhaseArgs->ucLevel = 100;
        }
        else
        {
                OMX_PRDSP2(pComponentPrivate->dbg, "Unsupported level.\n");
            OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorUnsupportedSetting);
        }
#else
        pCreatePhaseArgs->ucLevel = pMpeg4->eLevel;
#endif
        pCreatePhaseArgs->enableH263AnnexI  = 0;
        pCreatePhaseArgs->enableH263AnnexJ  = 0;
        pCreatePhaseArgs->enableH263AnnexT  = 0;

    }
    else
    {
        if (pH263->eLevel == OMX_VIDEO_H263Level10)
        {
            pCreatePhaseArgs->ucLevel = 10;
        }
        else if (pH263->eLevel == OMX_VIDEO_H263Level20)
        {
            pCreatePhaseArgs->ucLevel = 20;
        }
        else if (pH263->eLevel == OMX_VIDEO_H263Level30)
        {
            pCreatePhaseArgs->ucLevel = 30;
        }
        else if (pH263->eLevel == OMX_VIDEO_H263Level40)
        {
            pCreatePhaseArgs->ucLevel = 40;
        }
        else if (pH263->eLevel == OMX_VIDEO_H263Level45)
        {
            pCreatePhaseArgs->ucLevel = 45;
        }
        else if (pH263->eLevel == OMX_VIDEO_H263Level50)
        {
            pCreatePhaseArgs->ucLevel = 50;
        }
        else if (pH263->eLevel == OMX_VIDEO_H263Level60)
        {
            pCreatePhaseArgs->ucLevel = 60;
        }
        else if (pH263->eLevel == OMX_VIDEO_H263Level70)
        {
            pCreatePhaseArgs->ucLevel = 70;
        }
        else
        {
            OMX_PRDSP2(pComponentPrivate->dbg, "Unsupported level.\n");
            OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorUnsupportedSetting);
        }

        pCreatePhaseArgs->enableH263AnnexI  = 0;
        pCreatePhaseArgs->enableH263AnnexJ  = 0;
        pCreatePhaseArgs->enableH263AnnexT  = 0;
    }
    pCreatePhaseArgs->ulMaxDelay              = 300;
    #ifndef MODE_3410
    pCreatePhaseArgs->ulVbvParamEnable        = 0;
    pCreatePhaseArgs->ulH263SliceMode         = 0;
    #endif
    pCreatePhaseArgs->ulUseGOV                = 0;
    if (pPortDefOut->format.video.eCompressionFormat == OMX_VIDEO_CodingMPEG4)
        pCreatePhaseArgs->ulUseVOS            = 1;//needed to generate VOL Header
    else
        pCreatePhaseArgs->ulUseVOS            = 0;
    pCreatePhaseArgs->endArgs                 = END_OF_CR_PHASE_ARGS;
    pTmp = memcpy(nArr, pCreatePhaseArgs, sizeof(MP4VE_GPP_SN_Obj_CreatePhase));
    if (pTmp == NULL)
    {
        OMX_TRACE4(pComponentPrivate->dbg, "memcpy() out of memory error.\n");
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorInsufficientResources);
    }

    pLcmlDSP->pCrPhArgs = nArr;
    printMpeg4Params(pCreatePhaseArgs, &pComponentPrivate->dbg);

    sCb.LCML_Callback = (void *)OMX_VIDENC_LCML_Callback;
    eError = LCML_InitMMCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                              NULL,
                              &pLcmlHandle,
                              NULL,
                              &sCb);

    if (eError != OMX_ErrorNone)
    {
        OMX_PRDSP4(pComponentPrivate->dbg, "LCML_InitMMCodec Failed!...\n");
        /*TODO: Validate eError from LCML_InitMMCodec for ResourceExhaustionTest */
        pComponentPrivate->bErrorLcmlHandle = OMX_TRUE;
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorInsufficientResources);
    }
    pComponentPrivate->bCodecLoaded = OMX_TRUE;
    VIDENC_FREE(pCreatePhaseArgs, pMemoryListHead,
                pComponentPrivate->dbg);

OMX_CONF_CMD_BAIL:
    return eError;
}
/*----------------------------------------------------------------------------*/
/**
  *  OMX_VIDENC_Allocate_DSPResources()
  *
  *
  *
  *
  * @param
  * @param
  * @param
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*----------------------------------------------------------------------------*/

OMX_ERRORTYPE OMX_VIDENC_Allocate_DSPResources(VIDENC_COMPONENT_PRIVATE* pComponentPrivate,
                                               OMX_IN OMX_U32 nPortIndex)
{
    char* pTemp = NULL;
    OMX_U32 nBufferCnt = -1;
    VIDENC_NODE* pMemoryListHead = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    VIDEOENC_PORT_TYPE* pCompPort = NULL;
    OMX_VIDEO_CODINGTYPE eCompressionFormat = -1;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefOut = NULL;

    OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);

    pMemoryListHead = pComponentPrivate->pMemoryListHead;
    pCompPort = pComponentPrivate->pCompPort[nPortIndex];
    nBufferCnt = pComponentPrivate->pCompPort[nPortIndex]->nBufferCnt;
    pPortDefOut = pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pPortDef;
    eCompressionFormat = pPortDefOut->format.video.eCompressionFormat;

    if (nPortIndex == VIDENC_INPUT_PORT)
    {
        if (eCompressionFormat == OMX_VIDEO_CodingAVC)
        {
            H264VE_GPP_SN_UALGInputParams* pUalgParam;

            VIDENC_MALLOC(pUalgParam,
                          sizeof(H264VE_GPP_SN_UALGInputParams) + 256,
                          H264VE_GPP_SN_UALGInputParams,
                          pMemoryListHead,
                          pComponentPrivate->dbg);

            pTemp = (char*)pUalgParam;
            pTemp += 128;
            pUalgParam = (H264VE_GPP_SN_UALGInputParams*)pTemp;
            pCompPort->pBufferPrivate[nBufferCnt]->pUalgParam = pUalgParam;
        }
        else if (eCompressionFormat == OMX_VIDEO_CodingMPEG4 ||
                 eCompressionFormat == OMX_VIDEO_CodingH263)
        {
            MP4VE_GPP_SN_UALGInputParams* pUalgParam;

            VIDENC_MALLOC(pUalgParam,
                          sizeof(MP4VE_GPP_SN_UALGInputParams) + 256,
                          MP4VE_GPP_SN_UALGInputParams,
                          pMemoryListHead,
                          pComponentPrivate->dbg);
            pTemp = (char*)pUalgParam;
            pTemp += 128;
            pUalgParam = (MP4VE_GPP_SN_UALGInputParams*)pTemp;
            pCompPort->pBufferPrivate[nBufferCnt]->pUalgParam = pUalgParam;
            if(eCompressionFormat == OMX_VIDEO_CodingMPEG4)
            {/*Structure needed to send the request for VOLHeader to SN*/
                VIDENC_MALLOC(pComponentPrivate->pTempUalgInpParams,
                              sizeof(MP4VE_GPP_SN_UALGInputParams) + 256,
                              MP4VE_GPP_SN_UALGInputParams,
                              pMemoryListHead,
                              pComponentPrivate->dbg);
                pTemp = (char*)pComponentPrivate->pTempUalgInpParams;
                pTemp += 128;
                pComponentPrivate->pTempUalgInpParams = (MP4VE_GPP_SN_UALGInputParams*)pTemp;
            }
        }
    }
    else if (nPortIndex == VIDENC_OUTPUT_PORT)
    {
        if (eCompressionFormat == OMX_VIDEO_CodingAVC)
        {
            H264VE_GPP_SN_UALGOutputParams* pUalgParam;

            VIDENC_MALLOC(pUalgParam,
                          sizeof(H264VE_GPP_SN_UALGOutputParams) + 256,
                          H264VE_GPP_SN_UALGOutputParams,
                          pMemoryListHead,
                          pComponentPrivate->dbg);
            pTemp = (char*)pUalgParam;
            pTemp += 128;
            pUalgParam = (H264VE_GPP_SN_UALGOutputParams*)pTemp;
            pCompPort->pBufferPrivate[nBufferCnt]->pUalgParam = pUalgParam;
        }
        else if (eCompressionFormat == OMX_VIDEO_CodingMPEG4 ||
                 eCompressionFormat == OMX_VIDEO_CodingH263)
        {
            MP4VE_GPP_SN_UALGOutputParams* pUalgParam;

            VIDENC_MALLOC(pUalgParam,
                          sizeof(MP4VE_GPP_SN_UALGOutputParams) + 256,
                          MP4VE_GPP_SN_UALGOutputParams,
                          pMemoryListHead,
                          pComponentPrivate->dbg);
            pTemp = (char*)pUalgParam;
            pTemp += 128;
            pUalgParam = (MP4VE_GPP_SN_UALGOutputParams*)pTemp;
            pCompPort->pBufferPrivate[nBufferCnt]->pUalgParam = pUalgParam;
        }
    }
OMX_CONF_CMD_BAIL:
    return eError;
}

/*---------------------------------------------------------------------------------------*/
/**
  *  Callback() function will be called LCML component to write the msg
  *
  * @param msgBuffer                 This buffer will be returned by the LCML
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
 **/
/*---------------------------------------------------------------------------------------*/
OMX_ERRORTYPE OMX_VIDENC_LCML_Callback(TUsnCodecEvent event,void* argsCb [10])
{
    int nRet = -1;
    OMX_COMPONENTTYPE* pHandle = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE* pBufHead = NULL;
    VIDENC_BUFFER_PRIVATE* pBufferPrivate = NULL;
    LCML_DSP_INTERFACE* pLcmlDspInterface = NULL;
    TUsnCodecEvent eEvent = (TUsnCodecEvent)event;
    VIDENC_COMPONENT_PRIVATE* pComponentPrivate = NULL;

    OMX_CONF_CHECK_CMD(argsCb, 1, 1);

    if (argsCb[6])
    {
        pLcmlDspInterface = (LCML_DSP_INTERFACE*)argsCb[6];
        pComponentPrivate = (VIDENC_COMPONENT_PRIVATE*)pLcmlDspInterface->pComponentPrivate;
        pHandle = (OMX_COMPONENTTYPE *)pComponentPrivate->pHandle;
    }
    else
    {
        OMXDBG_PRINT(stderr, DSP, 5, 0, "No LCML handle\n");
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorBadParameter);
    }


    if (eEvent == EMMCodecBufferProcessed)
    {
        if (((OMX_S32)argsCb[0]) == EMMCodecOuputBuffer)
        {
            pBufHead = (OMX_BUFFERHEADERTYPE*)argsCb[7];
            pBufferPrivate = (VIDENC_BUFFER_PRIVATE*)pBufHead->pOutputPortPrivate;
#ifdef __PERF_INSTRUMENTATION__
            PERF_ReceivedFrame(pComponentPrivate->pPERFcomp,
                               PREF(pBufHead,pBuffer),
                               PREF(pBufHead,nFilledLen),
                               PERF_ModuleCommonLayer);

            pComponentPrivate->nLcml_nCntOpReceived++;

            if ((pComponentPrivate->nLcml_nCntIp >= 1) &&
                (pComponentPrivate->nLcml_nCntOpReceived == 1))
            {
                PERF_Boundary(pComponentPrivate->pPERFcomp,
                              PERF_BoundaryStart | PERF_BoundarySteadyState);
            }
#endif
            OMX_PRDSP1(pComponentPrivate->dbg, " [OUT] -> %p\n", pBufHead);
            if(pBufHead->nFilledLen > pBufHead->nAllocLen) {
                ALOGD("VE Warning!!! Output buffer overflow.");
            }
            pBufferPrivate->eBufferOwner = VIDENC_BUFFER_WITH_COMPONENT;
            if (pComponentPrivate->bCodecStarted == OMX_TRUE)
            {
                OMX_PRDSP1(pComponentPrivate->dbg, "Enters OMX_VIDENC_Process_FilledOutBuf\n");
              eError = OMX_VIDENC_Process_FilledOutBuf(pComponentPrivate, pBufHead);
              if (eError != OMX_ErrorNone)
              {
                OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                           OMX_EventError,
                           OMX_ErrorUndefined,
                           OMX_TI_ErrorCritical,
                           NULL);
                OMX_VIDENC_BAIL_IF_ERROR(eError, pComponentPrivate);
              }
            }
        }
        if ((int)argsCb [0] == EMMCodecInputBuffer)
        {
            pBufHead = (OMX_BUFFERHEADERTYPE*)argsCb[7];
            pBufferPrivate = (VIDENC_BUFFER_PRIVATE*)pBufHead->pInputPortPrivate;

#ifdef __PERF_INSTRUMENTATION__
            PERF_ReceivedFrame(pComponentPrivate->pPERFcomp,
                               PREF(pBufHead,pBuffer),
                               0,
                               PERF_ModuleCommonLayer);
#endif

            OMX_PRDSP1(pComponentPrivate->dbg, " [IN] -> %p\n", pBufHead);
            pBufferPrivate->eBufferOwner = VIDENC_BUFFER_WITH_COMPONENT;
            /*we should ignore the callback asociated to the VOL Header request*/
            if (pComponentPrivate->bCodecStarted == OMX_TRUE && pComponentPrivate->bWaitingVOLHeaderCallback == OMX_FALSE)
            {
                OMX_PRDSP1(pComponentPrivate->dbg, "Enters OMX_VIDENC_Process_FreeInBuf\n");
               eError = OMX_VIDENC_Process_FreeInBuf(pComponentPrivate, pBufHead);
               if (eError != OMX_ErrorNone)
               {
                 OMX_VIDENC_EVENT_HANDLER(pComponentPrivate,
                            OMX_EventError,
                            OMX_ErrorUndefined,
                            OMX_TI_ErrorCritical,
                            NULL);
                 OMX_VIDENC_BAIL_IF_ERROR(eError, pComponentPrivate);
               }
                OMX_PRDSP1(pComponentPrivate->dbg, "Exits OMX_VIDENC_Process_FreeInBuf\n");
            }
            else if(pComponentPrivate->bWaitingVOLHeaderCallback == OMX_TRUE)
            {
                pComponentPrivate->bWaitingVOLHeaderCallback = OMX_FALSE;
            }
        }
    }

    if(eEvent == EMMCodecProcessingPaused ||
       eEvent == EMMCodecProcessingStoped)
    {
        if (pComponentPrivate != NULL)
        {
            pComponentPrivate->bDSPStopAck = OMX_TRUE;
            #ifndef UNDER_CE
                pthread_mutex_lock(&pComponentPrivate->videoe_mutex_app);
                pthread_cond_signal(&pComponentPrivate->stop_cond);
                pthread_mutex_unlock(&pComponentPrivate->videoe_mutex_app);
            #endif
        }
    }

    if(eEvent == EMMCodecStrmCtrlAck)
    {
        if ((int)argsCb [0] == USN_ERR_NONE)
        {
            pComponentPrivate->bFlushComplete = OMX_TRUE;
            #ifndef UNDER_CE
                pthread_mutex_lock(&pComponentPrivate->videoe_mutex_app);
                pthread_cond_signal(&pComponentPrivate->flush_cond);
                pthread_mutex_unlock(&pComponentPrivate->videoe_mutex_app);
            #endif
        }
    }
   nRet = OMX_VIDENC_HandleLcmlEvent(pComponentPrivate, eEvent, argsCb);
    if (nRet == -1)
    {
        OMX_ERROR4(pComponentPrivate->dbg, "LCML Event Handler failed.\n");
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorUndefined);
    }

OMX_CONF_CMD_BAIL:
    return eError;
}


#ifdef UNDER_CE
/* ================================================================================= */
/**
* @fns Sleep replace for WIN CE
*/
/* ================================================================================ */
int OMX_CreateEvent(OMX_Event *event){
    int ret = OMX_ErrorNone;
    HANDLE createdEvent = NULL;
    if (event == NULL)
    {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    event->event  = CreateEvent(NULL, TRUE, FALSE, NULL);
    if(event->event == NULL)
        ret = (int)GetLastError();
EXIT:
    return ret;
}

int OMX_SignalEvent(OMX_Event *event){
     int ret = OMX_ErrorNone;
    if (event == NULL)
    {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
     }
     SetEvent(event->event);
     ret = (int)GetLastError();
EXIT:
    return ret;
}

int OMX_WaitForEvent(OMX_Event *event) {
     int ret = OMX_ErrorNone;
    if (event == NULL)
    {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
     }
     WaitForSingleObject(event->event, INFINITE);
     ret = (int)GetLastError();
EXIT:
     return ret;
}

int OMX_DestroyEvent(OMX_Event *event) {
     int ret = OMX_ErrorNone;
    if (event == NULL)
    {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
     }
     CloseHandle(event->event);
EXIT:
     return ret;
}
#endif

#ifdef RESOURCE_MANAGER_ENABLED
/*-----------------------------------------------------------------------------*/
/**
  * OMX_VIDENC_ResourceManagerCallBack()
  *
  * Called from Resource Manager()
  *
 *
  *
  **/
/*-----------------------------------------------------------------------------*/
void OMX_VIDENC_ResourceManagerCallBack(RMPROXY_COMMANDDATATYPE cbData)
{
    OMX_COMMANDTYPE Cmd = OMX_CommandStateSet;
    OMX_STATETYPE state = OMX_StateIdle;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)cbData.hComponent;
    VIDENC_COMPONENT_PRIVATE *pCompPrivate = NULL;

    pCompPrivate = (VIDENC_COMPONENT_PRIVATE*)pHandle->pComponentPrivate;
    OMX_PRMGR2(pCompPrivate->dbg, "OMX_VIDENC_ResourceManagerCallBack\n");
    OMX_PRMGR2(pCompPrivate->dbg, "Arguments:\ncbData.RM_Error = %dcbData.RM_Cmd = %d\n", *(cbData.RM_Error), cbData.RM_Cmd);
    if (*(cbData.RM_Error) == OMX_ErrorResourcesPreempted)
    {
        if (pCompPrivate->eState== OMX_StateExecuting ||
            pCompPrivate->eState == OMX_StatePause)
        {

        pCompPrivate->sCbData.EventHandler (
                            pHandle, pHandle->pApplicationPrivate,
                            OMX_EventError,
                           OMX_ErrorResourcesPreempted,OMX_TI_ErrorMinor,
                            "Componentn Preempted\n");

            OMX_PRSTATE2(pCompPrivate->dbg, "Send command to Idle from RM CallBack\n");
        OMX_SendCommand(pHandle, Cmd, state, NULL);
        pCompPrivate->bPreempted = 1;

        }
    }
    else if (*(cbData.RM_Error) == OMX_RmProxyCallback_ResourcesAcquired)
    {
        pCompPrivate->sCbData.EventHandler (
                            pHandle, pHandle->pApplicationPrivate,
                            OMX_EventResourcesAcquired, 0,0,
                            NULL);
        OMX_PRSTATE2(pCompPrivate->dbg, "Send command to Executing from RM CallBack\n");
        OMX_SendCommand(pHandle, Cmd, OMX_StateExecuting, NULL);
    }
}
#endif

void CalculateBufferSize(OMX_PARAM_PORTDEFINITIONTYPE* pCompPort, VIDENC_COMPONENT_PRIVATE* pCompPrivate)
{

    if(pCompPort->nPortIndex == VIDENC_INPUT_PORT) {
        if (pCompPort->format.video.eColorFormat == OMX_COLOR_FormatYUV420Planar)
        {
            pCompPort->nBufferSize = pCompPort->format.video.nFrameWidth *
                                    pCompPort->format.video.nFrameHeight * 1.5;
        }
        else
        {
            pCompPort->nBufferSize = pCompPort->format.video.nFrameWidth *
                                    pCompPort->format.video.nFrameHeight * 2;
        }
    }
    else {
        if (pCompPort->format.video.eCompressionFormat == OMX_VIDEO_CodingAVC)
        {
            pCompPort->nBufferSize = GetMaxAVCBufferSize(pCompPort->format.video.nFrameWidth, pCompPort->format.video.nFrameHeight);
        }
        else
        {/*coding Mpeg4 or H263*/
            pCompPort->nBufferSize = pCompPort->format.video.nFrameWidth *
                                    pCompPort->format.video.nFrameHeight / 2;
        }
        pCompPort->nBufferSize += 256;
    }
}

OMX_U32 GetMaxAVCBufferSize(OMX_U32 width, OMX_U32 height)
{
    OMX_U32 MaxCPB;
    OMX_U32 nMacroBlocks;

    /* Calculate output buffer size based on max possible CPB for the resolution
       Output bitrate may not be set yet, so only resolution is taken into account */

    nMacroBlocks = (width * height) / 256;

    /* Following values are set based on Annex A of AVC Standard */
    if(nMacroBlocks <= 99) {
        MaxCPB = 500;
    }
    else if(nMacroBlocks <= 396) {
        MaxCPB = 2000;
    }
    else if(nMacroBlocks <= 792) {
        MaxCPB = 4000;
    }
    else if(nMacroBlocks <= 1620) {
        /* Note - Max bitrate in this case is assumed to max 4 Mbps to limit the buffer size 
           If bitrate in this particular case could be higher than 4 Mbps, increase MxCPB value */
        MaxCPB = 4000;
    }
    else
        MaxCPB = 14000;

    /* MaxCPB are in units of 1200 bits i.e. 150 bytes */
    /* Return  buffer size in bytes*/
    /*Last patch to improve the performance*/
    /*return (150 * MaxCPB);*/
    return (width * height) / 2;
}
OMX_U32 OMX_VIDENC_GetDefaultBitRate(VIDENC_COMPONENT_PRIVATE* pComponentPrivate)
{
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef;
    OMX_U32 bitrate;
    int nCount;

    pPortDef = pComponentPrivate->pCompPort[VIDENC_OUTPUT_PORT]->pPortDef;
    for ( nCount = 0; nCount < VIDENC_MAXBITRATES; nCount++ ) {
        if (pPortDef->format.video.eCompressionFormat == OMX_VIDEO_CodingAVC) {
            bitrate = VIDENC_STRUCT_H264DEFBITRATE [nCount][1];
            if ((pPortDef->format.video.nFrameWidth * pPortDef->format.video.nFrameHeight)
                <= VIDENC_STRUCT_H264DEFBITRATE[nCount][0]) {
                break;
            }
        }
        else if (pPortDef->format.video.eCompressionFormat == OMX_VIDEO_CodingMPEG4) {
            bitrate = VIDENC_STRUCT_MPEG4DEFBITRATE [nCount][1];
            if ((pPortDef->format.video.nFrameWidth * pPortDef->format.video.nFrameHeight)
                <= VIDENC_STRUCT_MPEG4DEFBITRATE[nCount][0]) {
                break;
            }
        }
        else {
            bitrate = VIDENC_STRUCT_H263DEFBITRATE [nCount][1];
            if ((pPortDef->format.video.nFrameWidth * pPortDef->format.video.nFrameHeight)
                <= VIDENC_STRUCT_H263DEFBITRATE[nCount][0]) {
                break;
            }
        }
    }

    return bitrate;
}


void printMpeg4Params(MP4VE_GPP_SN_Obj_CreatePhase* pCreatePhaseArgs,
                      struct OMX_TI_Debug *dbg)
{
    OMX_PRDSP2(*dbg, "\nusNumStreams = %d\n", pCreatePhaseArgs->usNumStreams);
    OMX_PRDSP2(*dbg, "usStreamId = %d\n", pCreatePhaseArgs->usStreamId);
    OMX_PRDSP2(*dbg, "usBuffTypeInStream = %d\n", pCreatePhaseArgs->usBuffTypeInStream);
    OMX_PRDSP2(*dbg, "usMaxBuffsInStream = %d\n", pCreatePhaseArgs->usMaxBuffsInStream);
    OMX_PRDSP2(*dbg, "usStreamId2 = %d\n", pCreatePhaseArgs->usStreamId2);
    OMX_PRDSP2(*dbg, "usBuffTypeInStream2 = %d\n", pCreatePhaseArgs->usBuffTypeInStream2);
    OMX_PRDSP2(*dbg, "usMaxBuffsInStream2 = %d\n", pCreatePhaseArgs->usMaxBuffsInStream2);

    OMX_PRDSP2(*dbg, "ulWidth = %d\n", pCreatePhaseArgs->ulWidth);
    OMX_PRDSP2(*dbg, "ulHeight = %d\n", pCreatePhaseArgs->ulHeight);
    OMX_PRDSP2(*dbg, "ulTargetBitRate = %d\n", pCreatePhaseArgs->ulTargetBitRate);
    OMX_PRDSP2(*dbg, "ulVBVSize = %d\n", pCreatePhaseArgs->ulVBVSize);
    OMX_PRDSP2(*dbg, "ulGOBHeadersInterval = %d\n", pCreatePhaseArgs->ulGOBHeadersInterval);

    OMX_PRDSP2(*dbg, "ucIsMPEG4 = %d\n", pCreatePhaseArgs->ucIsMPEG4);
    OMX_PRDSP2(*dbg, "ucYUVFormat = %d\n", pCreatePhaseArgs->ucYUVFormat);
    OMX_PRDSP2(*dbg, "ucHEC = %d\n", pCreatePhaseArgs->ucHEC);
    OMX_PRDSP2(*dbg, "ucResyncMarker = %d\n", pCreatePhaseArgs->ucResyncMarker);
    OMX_PRDSP2(*dbg, "ucDataPartitioning = %d\n", pCreatePhaseArgs->ucDataPartitioning);
    OMX_PRDSP2(*dbg, "ucReversibleVLC = %d\n", pCreatePhaseArgs->ucReversibleVLC);
    OMX_PRDSP2(*dbg, "ucUnrestrictedMV = %d\n", pCreatePhaseArgs->ucUnrestrictedMV);
    OMX_PRDSP2(*dbg, "ucFrameRate = %d\n", pCreatePhaseArgs->ucFrameRate);
    OMX_PRDSP2(*dbg, "ucRateControlAlgorithm = %d\n", pCreatePhaseArgs->ucRateControlAlgorithm);
    OMX_PRDSP2(*dbg, "ucQPFirstIFrame = %d\n", pCreatePhaseArgs->ucQPFirstIFrame);
    OMX_PRDSP2(*dbg, "ucProfile = %d\n", pCreatePhaseArgs->ucProfile);
    OMX_PRDSP2(*dbg, "ucLevel = %d\n", pCreatePhaseArgs->ucLevel);
    OMX_PRDSP2(*dbg, "ulMaxDelay = %d\n", pCreatePhaseArgs->ulMaxDelay);
    /*
    OMX_PRDSP2(*dbg, "ulVbvParamEnable = %d\n", pCreatePhaseArgs->ulVbvParamEnable);
    OMX_PRDSP2(*dbg, "ulH263SliceMode = %d\n", pCreatePhaseArgs->ulH263SliceMode);
    */
    OMX_PRDSP2(*dbg, "ulUseGOV = %d\n", pCreatePhaseArgs->ulUseGOV);
    OMX_PRDSP2(*dbg, "ulUseVOS = %d\n", pCreatePhaseArgs->ulUseVOS);
    OMX_PRDSP2(*dbg, "enableH263AnnexI = %d\n", pCreatePhaseArgs->enableH263AnnexI);
    OMX_PRDSP2(*dbg, "enableH263AnnexJ = %d\n", pCreatePhaseArgs->enableH263AnnexJ);
    OMX_PRDSP2(*dbg, "enableH263AnnexT = %d\n", pCreatePhaseArgs->enableH263AnnexT);
}
void printH264CreateParams(H264VE_GPP_SN_Obj_CreatePhase* pCreatePhaseArgs, struct OMX_TI_Debug *dbg)
{
    OMX_PRDSP2(*dbg, "\nusNumStreams = %d\n", pCreatePhaseArgs->usNumStreams);
    OMX_PRDSP2(*dbg, "usStreamId = %d\n", pCreatePhaseArgs->usStreamId);
    OMX_PRDSP2(*dbg, "usBuffTypeInStream = %d\n", pCreatePhaseArgs->usBuffTypeInStream);
    OMX_PRDSP2(*dbg, "usMaxBuffsInStream = %d\n", pCreatePhaseArgs->usMaxBuffsInStream);
    OMX_PRDSP2(*dbg, "usStreamId2 = %d\n", pCreatePhaseArgs->usStreamId2);
    OMX_PRDSP2(*dbg, "usBuffTypeInStream2 = %d\n", pCreatePhaseArgs->usBuffTypeInStream2);
    OMX_PRDSP2(*dbg, "usMaxBuffsInStream2 = %d\n", pCreatePhaseArgs->usMaxBuffsInStream2);

    OMX_PRDSP2(*dbg, "ulWidth = %d\n", pCreatePhaseArgs->ulWidth);
    OMX_PRDSP2(*dbg, "ulHeight = %d\n", pCreatePhaseArgs->ulHeight);
    OMX_PRDSP2(*dbg, "ulTargetBitRate = %d\n", pCreatePhaseArgs->ulTargetBitRate);
    OMX_PRDSP2(*dbg, "ulBitstreamBuffSize = %d\n", pCreatePhaseArgs->ulBitstreamBuffSize);
    OMX_PRDSP2(*dbg, "ulIntraFramePeriod = %d\n", pCreatePhaseArgs->ulIntraFramePeriod);
    OMX_PRDSP2(*dbg, "ulFrameRate = %d\n", pCreatePhaseArgs->ulFrameRate);

    OMX_PRDSP2(*dbg, "ucYUVFormat = %d\n", pCreatePhaseArgs->ucYUVFormat);
    OMX_PRDSP2(*dbg, "ucUnrestrictedMV = %d\n", pCreatePhaseArgs->ucUnrestrictedMV);
    OMX_PRDSP2(*dbg, "ucNumRefFrames = %d\n", pCreatePhaseArgs->ucNumRefFrames);
    OMX_PRDSP2(*dbg, "ucRateControlAlgorithm = %d\n", pCreatePhaseArgs->ucRateControlAlgorithm);
    OMX_PRDSP2(*dbg, "ucIDREnable = %d\n", pCreatePhaseArgs->ucIDREnable);
    OMX_PRDSP2(*dbg, "ucDeblockingEnable = %d\n", pCreatePhaseArgs->ucDeblockingEnable);
    OMX_PRDSP2(*dbg, "ucMVRange = %d\n", pCreatePhaseArgs->ucMVRange);
    OMX_PRDSP2(*dbg, "ucQPIFrame = %d\n", pCreatePhaseArgs->ucQPIFrame);
    OMX_PRDSP2(*dbg, "ucProfile = %d\n", pCreatePhaseArgs->ucProfile);
    OMX_PRDSP2(*dbg, "ucLevel = %d\n", pCreatePhaseArgs->ucLevel);

    OMX_PRDSP2(*dbg, "usNalCallback = %d\n", pCreatePhaseArgs->usNalCallback);
    OMX_PRDSP2(*dbg, "ulEncodingPreset = %d\n", pCreatePhaseArgs->ulEncodingPreset);
    OMX_PRDSP2(*dbg, "ulRcAlgo = %d\n", pCreatePhaseArgs->ulRcAlgo);
}

void printMpeg4UAlgInParam(MP4VE_GPP_SN_UALGInputParams* pUalgInpParams, int printAlways, struct OMX_TI_Debug *dbg)
{
    static int printed=0;

    if(printAlways || !printed)
    {
        printed++;
        OMX_PRDSP2(*dbg, "\nulFrameIndex = %u\n", pUalgInpParams->ulFrameIndex);
        OMX_PRDSP2(*dbg, "ulTargetFrameRate = %u\n", pUalgInpParams->ulTargetFrameRate);
        OMX_PRDSP2(*dbg, "ulTargetBitRate = %u\n", pUalgInpParams->ulTargetBitRate);
        OMX_PRDSP2(*dbg, "ulIntraFrameInterval = %u\n", pUalgInpParams->ulIntraFrameInterval);
        OMX_PRDSP2(*dbg, "ulGenerateHeader = %u\n", pUalgInpParams->ulGenerateHeader);
        OMX_PRDSP2(*dbg, "ulForceIFrame = %u\n", pUalgInpParams->ulForceIFrame);
        OMX_PRDSP2(*dbg, "ulResyncInterval = %u\n", pUalgInpParams->ulResyncInterval);
        OMX_PRDSP2(*dbg, "ulHecInterval = %u\n", pUalgInpParams->ulHecInterval);
        OMX_PRDSP2(*dbg, "ulAIRRate = %u\n", pUalgInpParams->ulAIRRate);
        OMX_PRDSP2(*dbg, "ulMIRRate = %u\n", pUalgInpParams->ulMIRRate);
        OMX_PRDSP2(*dbg, "ulQPIntra = %u\n", pUalgInpParams->ulQPIntra);
        OMX_PRDSP2(*dbg, "ulfCode = %u\n", pUalgInpParams->ulfCode);
        OMX_PRDSP2(*dbg, "ulHalfPel = %u\n", pUalgInpParams->ulHalfPel);
        OMX_PRDSP2(*dbg, "ulACPred = %u\n", pUalgInpParams->ulACPred);
        OMX_PRDSP2(*dbg, "ul4MV = %u\n", pUalgInpParams->ul4MV);
        OMX_PRDSP2(*dbg, "uluseUMV = %u\n", pUalgInpParams->uluseUMV);
        OMX_PRDSP2(*dbg, "ulMVDataEnable = %u\n", pUalgInpParams->ulMVDataEnable);
        OMX_PRDSP2(*dbg, "ulResyncDataEnable = %u\n", pUalgInpParams->ulResyncDataEnable);
        OMX_PRDSP2(*dbg, "ulQPInter = %u\n", pUalgInpParams->ulQPInter);
        OMX_PRDSP2(*dbg, "ulLastFrame = %u\n", pUalgInpParams->ulLastFrame);
        OMX_PRDSP2(*dbg, "ulcapturewidth = %u\n", pUalgInpParams->ulcapturewidth);
        OMX_PRDSP2(*dbg, "ulQpMax = %u\n", pUalgInpParams->ulQpMax);
        OMX_PRDSP2(*dbg, "ulQpMin = %u\n", pUalgInpParams->ulQpMin);
    }
}


void printH264UAlgInParam(H264VE_GPP_SN_UALGInputParams* pUalgInpParams, int printAlways, struct OMX_TI_Debug *dbg)
{
    static int printed=0;

    if(printAlways || !printed)
    {
        printed++;
        OMX_PRDSP2(*dbg, "\nqpIntra = %lu\n", pUalgInpParams->H264VENC_TI_DYNAMICPARAMS.qpIntra);
        OMX_PRDSP2(*dbg, "qpInter = %lu\n", pUalgInpParams->H264VENC_TI_DYNAMICPARAMS.qpInter);
        OMX_PRDSP2(*dbg, "qpMax = %lu\n", pUalgInpParams->H264VENC_TI_DYNAMICPARAMS.qpMax);
        OMX_PRDSP2(*dbg, "qpMin = %lu\n", pUalgInpParams->H264VENC_TI_DYNAMICPARAMS.qpMin);
        OMX_PRDSP2(*dbg, "lfDisableIdc = %lu\n", pUalgInpParams->H264VENC_TI_DYNAMICPARAMS.lfDisableIdc);
        OMX_PRDSP2(*dbg, "quartPelDisable = %lu\n", pUalgInpParams->H264VENC_TI_DYNAMICPARAMS.quartPelDisable);
        OMX_PRDSP2(*dbg, "airMbPeriod = %lu\n", pUalgInpParams->H264VENC_TI_DYNAMICPARAMS.airMbPeriod);
        OMX_PRDSP2(*dbg, "maxMBsPerSlice = %lu\n", pUalgInpParams->H264VENC_TI_DYNAMICPARAMS.maxMBsPerSlice);
        OMX_PRDSP2(*dbg, "maxBytesPerSlice = %lu\n", pUalgInpParams->H264VENC_TI_DYNAMICPARAMS.maxBytesPerSlice);
        OMX_PRDSP2(*dbg, "sliceRefreshRowStartNumber = %lu\n", pUalgInpParams->H264VENC_TI_DYNAMICPARAMS.sliceRefreshRowStartNumber);
        OMX_PRDSP2(*dbg, "sliceRefreshRowNumber = %lu\n", pUalgInpParams->H264VENC_TI_DYNAMICPARAMS.sliceRefreshRowNumber);
        OMX_PRDSP2(*dbg, "filterOffsetA = %lu\n", pUalgInpParams->H264VENC_TI_DYNAMICPARAMS.filterOffsetA);
        OMX_PRDSP2(*dbg, "filterOffsetB = %lu\n", pUalgInpParams->H264VENC_TI_DYNAMICPARAMS.filterOffsetB);
        OMX_PRDSP2(*dbg, "log2MaxFNumMinus4 = %lu\n", pUalgInpParams->H264VENC_TI_DYNAMICPARAMS.log2MaxFNumMinus4);
        OMX_PRDSP2(*dbg, "chromaQPIndexOffset = %lu\n", pUalgInpParams->H264VENC_TI_DYNAMICPARAMS.chromaQPIndexOffset);
        OMX_PRDSP2(*dbg, "constrainedIntraPredEnable = %lu\n", pUalgInpParams->H264VENC_TI_DYNAMICPARAMS.constrainedIntraPredEnable);
        OMX_PRDSP2(*dbg, "picOrderCountType = %lu\n", pUalgInpParams->H264VENC_TI_DYNAMICPARAMS.picOrderCountType);
        OMX_PRDSP2(*dbg, "maxMVperMB = %lu\n", pUalgInpParams->H264VENC_TI_DYNAMICPARAMS.maxMVperMB);
        OMX_PRDSP2(*dbg, "intra4x4EnableIdc = %lu\n", pUalgInpParams->H264VENC_TI_DYNAMICPARAMS.intra4x4EnableIdc);
        OMX_PRDSP2(*dbg, "mvDataEnable = %lu\n", pUalgInpParams->H264VENC_TI_DYNAMICPARAMS.mvDataEnable);
        OMX_PRDSP2(*dbg, "hierCodingEnable = %lu\n", pUalgInpParams->H264VENC_TI_DYNAMICPARAMS.hierCodingEnable);
        OMX_PRDSP2(*dbg, "streamFormat = %lu\n", pUalgInpParams->H264VENC_TI_DYNAMICPARAMS.streamFormat);
        OMX_PRDSP2(*dbg, "intraRefreshMethod = %lu\n", pUalgInpParams->H264VENC_TI_DYNAMICPARAMS.intraRefreshMethod);
        OMX_PRDSP2(*dbg, "perceptualQuant = %lu\n", pUalgInpParams->H264VENC_TI_DYNAMICPARAMS.perceptualQuant);
        OMX_PRDSP2(*dbg, "sceneChangeDet = %lu\n", pUalgInpParams->H264VENC_TI_DYNAMICPARAMS.sceneChangeDet);
        OMX_PRDSP2(*dbg, "numSliceASO = %lu\n", pUalgInpParams->H264VENC_TI_DYNAMICPARAMS.numSliceASO);
        OMX_PRDSP2(*dbg, "numSliceGroups = %lu\n", pUalgInpParams->H264VENC_TI_DYNAMICPARAMS.numSliceGroups);
        OMX_PRDSP2(*dbg, "sliceGroupMapType = %lu\n", pUalgInpParams->H264VENC_TI_DYNAMICPARAMS.sliceGroupMapType);
        OMX_PRDSP2(*dbg, "sliceGroupChangeDirectionFlag = %lu\n", pUalgInpParams->H264VENC_TI_DYNAMICPARAMS.sliceGroupChangeDirectionFlag);
        OMX_PRDSP2(*dbg, "sliceGroupChangeRate = %lu\n", pUalgInpParams->H264VENC_TI_DYNAMICPARAMS.sliceGroupChangeRate);
        OMX_PRDSP2(*dbg, "sliceGroupChangeCycle = %lu\n", pUalgInpParams->H264VENC_TI_DYNAMICPARAMS.sliceGroupChangeCycle);
        OMX_PRDSP2(*dbg, "ulFrameIndex = %lu\n", pUalgInpParams->ulFrameIndex);
    }
}

OMX_ERRORTYPE IsResolutionPlayable (OMX_U32 width, OMX_U32 height)
{
    if (width  > WVGA_MAX_WIDTH || height > WVGA_MAX_HEIGHT) 
    {
        return OMX_ErrorBadParameter;
    }
    return OMX_ErrorNone;

}

OMX_ERRORTYPE AddStateTransition(VIDENC_COMPONENT_PRIVATE* pComponentPrivate) {

    OMX_ERRORTYPE eError = OMX_ErrorNone;

    if(pthread_mutex_lock(&pComponentPrivate->mutexStateChangeRequest)) {
       return OMX_ErrorUndefined;
    }

    /* Increment state change request reference count */
    pComponentPrivate->nPendingStateChangeRequests++;

    if(pthread_mutex_unlock(&pComponentPrivate->mutexStateChangeRequest)) {
       return OMX_ErrorUndefined;
    }

    return eError;
}

OMX_ERRORTYPE RemoveStateTransition(VIDENC_COMPONENT_PRIVATE* pComponentPrivate, OMX_BOOL bEnableSignal) {
    OMX_ERRORTYPE eError = OMX_ErrorNone;

     /* Decrement state change request reference count*/
    if(pthread_mutex_lock(&pComponentPrivate->mutexStateChangeRequest)) {
       return OMX_ErrorUndefined;
    }

    pComponentPrivate->nPendingStateChangeRequests--;

   /* If there are no more pending requests, signal the thread waiting on this*/
    if(!pComponentPrivate->nPendingStateChangeRequests && bEnableSignal) {
       pthread_cond_signal(&(pComponentPrivate->StateChangeCondition));
    }

    if(pthread_mutex_unlock(&pComponentPrivate->mutexStateChangeRequest)) {
       return OMX_ErrorUndefined;
    }

    return eError;
}
