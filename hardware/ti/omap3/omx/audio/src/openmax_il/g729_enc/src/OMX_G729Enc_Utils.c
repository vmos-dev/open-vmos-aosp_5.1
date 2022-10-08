
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
 *             Texas Instruments OMAP (TM) Platform Software
 *  (c) Copyright Texas Instruments, Incorporated.  All Rights Reserved.
 *
 *  Use of this software is controlled by the terms and conditions found
 *  in the license agreement under which this software has been supplied.
 * =========================================================================== */
/**
 * @file OMX_G729Enc_Utils.c
 *
 * This file implements G729 Encoder Component Specific APIs and its functionality
 * that is fully compliant with the Khronos OpenMAX (TM) 1.0 Specification
 *
 * @path  $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\G729_enc\src
 *
 * @rev  1.0
 */
/* ----------------------------------------------------------------------------
 *!
 *! Revision History
 *! ===================================
 *! 21-sept-2006 bk: updated review findings for alpha release
 *! 24-Aug-2006 bk: Khronos OpenMAX (TM) 1.0 Conformance tests some more
 *! 18-July-2006 bk: Khronos OpenMAX (TM) 1.0 Conformance tests validated for few cases
 *! 21-Jun-2006 bk: Khronos OpenMAX (TM) 1.0 migration done
 *! 22-May-2006 bk: DASF recording quality improved
 *! 19-Apr-2006 bk: DASF recording speed issue resloved
 *! 23-Feb-2006 bk: DASF functionality added
 *! 18-Jan-2006 bk: Repated recording issue fixed and LCML changes taken care
 *! 14-Dec-2005 bk: Initial Version
 *! 16-Nov-2005 bk: Initial Version
 *! 23-Sept-2005 bk: Initial Version
 *! 10-Sept-2005 bk: Initial Version
 *! 10-Sept-2005 bk:
 *! This is newest file
 * =========================================================================== */

/* ------compilation control switches -------------------------*/
/****************************************************************
 *  INCLUDE FILES
 ****************************************************************/
/* ----- system and platform files ----------------------------*/
#ifdef UNDER_CE
#include <windows.h>
#include <oaf_osal.h>
#include <omx_core.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <malloc.h>
#include <memory.h>
#include <fcntl.h>
#include <errno.h>
#endif

#include <dbapi.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
/*-------program files ----------------------------------------*/
#include "OMX_G729Enc_Utils.h"

#include "g729encsocket_ti.h"
#include <encode_common_ti.h>
#include "OMX_G729Enc_ComponentThread.h"
#include "usn.h"


#ifdef UNDER_CE         
#define HASHINGENABLE 1
HINSTANCE g_hLcmlDllHandle = NULL;
#endif

#ifdef __PERF_INSTRUMENTATION__
#include "perf.h"
#endif

/* ========================================================================== */
/**
 * @G729ENC_FillLCMLInitParams () This function is used by the component thread to
 * fill the all of its initialization parameters, buffer deatils  etc
 * to LCML structure,
 *
 * @param pComponent  handle for this instance of the component
 * @param plcml_Init  pointer to LCML structure to be filled
 *
 * @pre
 *
 * @post
 *
 * @return none
 */
/* ========================================================================== */
static G729ENC_COMPONENT_PRIVATE *pComponentPrivate_CC; 

OMX_ERRORTYPE G729ENC_FillLCMLInitParams(OMX_HANDLETYPE pComponent,
                                         LCML_DSP *plcml_Init, OMX_U16 arr[])
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nIpBuf = 0,nIpBufSize = 0,nOpBuf = 0,nOpBufSize = 0;
    OMX_BUFFERHEADERTYPE *pTemp = NULL;
    LCML_DSP_INTERFACE *pHandle = (LCML_DSP_INTERFACE *)pComponent;
    G729ENC_COMPONENT_PRIVATE *pComponentPrivate = pHandle->pComponentPrivate;
    G729ENC_LCML_BUFHEADERTYPE *pTemp_lcml = NULL;
    OMX_U32 i = 0;
    OMX_U32 size_lcml = 0;
    
    G729ENC_DPRINT("Entering\n");
    nIpBuf = pComponentPrivate->pInputBufferList->numBuffers;
    nIpBufSize = pComponentPrivate->pPortDef[G729ENC_INPUT_PORT]->nBufferSize;
    pComponentPrivate->nRuntimeInputBuffers = nIpBuf;
    nOpBuf = pComponentPrivate->pOutputBufferList->numBuffers;
    nOpBufSize = pComponentPrivate->pPortDef[G729ENC_OUTPUT_PORT]->nBufferSize;
    pComponentPrivate->nRuntimeOutputBuffers = nOpBuf;
    G729ENC_DPRINT("------ Buffer Details -----------\n");
    G729ENC_DPRINT("Input  Buffer Count = %ld\n", nIpBuf);
    G729ENC_DPRINT("Input  Buffer Size = %ld\n", nIpBufSize);
    G729ENC_DPRINT("Output Buffer Count = %ld\n", nOpBuf);
    G729ENC_DPRINT("Output Buffer Size = %ld\n", nOpBufSize);
    G729ENC_DPRINT("------ Buffer Details ------------\n");
    /* Fill Input Buffers Info for LCML */
    plcml_Init->In_BufInfo.nBuffers = nIpBuf;
    plcml_Init->In_BufInfo.nSize = nIpBufSize;
    plcml_Init->In_BufInfo.DataTrMethod = DMM_METHOD;

    /* Fill Output Buffers Info for LCML */
    plcml_Init->Out_BufInfo.nBuffers = nOpBuf;
    plcml_Init->Out_BufInfo.nSize = nOpBufSize;
    plcml_Init->Out_BufInfo.DataTrMethod = DMM_METHOD;

    /*Copy the node information*/
    plcml_Init->NodeInfo.nNumOfDLLs = 3;

    plcml_Init->NodeInfo.AllUUIDs[0].uuid = &G729ENCSOCKET_TI_UUID;
    strcpy ((char *)plcml_Init->NodeInfo.AllUUIDs[0].DllName,G729ENC_DLL_NAME);
    plcml_Init->NodeInfo.AllUUIDs[0].eDllType = DLL_NODEOBJECT;

    plcml_Init->NodeInfo.AllUUIDs[1].uuid = &G729ENCSOCKET_TI_UUID;
    strcpy ((char *)plcml_Init->NodeInfo.AllUUIDs[1].DllName,G729ENC_DLL_NAME);
    plcml_Init->NodeInfo.AllUUIDs[1].eDllType = DLL_DEPENDENT;

    plcml_Init->NodeInfo.AllUUIDs[2].uuid = &USN_TI_UUID;
    strcpy ((char *)plcml_Init->NodeInfo.AllUUIDs[2].DllName,G729ENC_USN_DLL_NAME);
    plcml_Init->NodeInfo.AllUUIDs[2].eDllType = DLL_DEPENDENT;
    plcml_Init->DeviceInfo.TypeofDevice = 0;

    if(pComponentPrivate->dasfMode == 1)
    {
        G729ENC_DPRINT("Codec is configuring to DASF mode\n");
        OMX_G729MALLOC_STRUCT(pComponentPrivate->strmAttr, LCML_STRMATTR);
        pComponentPrivate->strmAttr->uSegid = G729ENC_DEFAULT_SEGMENT;
        pComponentPrivate->strmAttr->uAlignment = 0;
        pComponentPrivate->strmAttr->uTimeout = G729ENC_SN_TIMEOUT;
        pComponentPrivate->strmAttr->uBufsize = G729ENC_INPUT_BUFFER_SIZE_DASF;
        pComponentPrivate->strmAttr->uNumBufs = G729ENC_NUM_INPUT_BUFFERS_DASF;
        pComponentPrivate->strmAttr->lMode = STRMMODE_PROCCOPY;
        /* Device is Configuring to DASF Mode */
        plcml_Init->DeviceInfo.TypeofDevice = 1;
        /* Device is Configuring to Record Mode */
        plcml_Init->DeviceInfo.TypeofRender = 1;

        if(pComponentPrivate->acdnMode == 1)
        {
            /* ACDN mode */
            plcml_Init->DeviceInfo.AllUUIDs[0].uuid = &ACDN_TI_UUID;
        }
        else
        {
            /* DASF/TeeDN mode */
            plcml_Init->DeviceInfo.AllUUIDs[0].uuid = &DCTN_TI_UUID;
        }
        plcml_Init->DeviceInfo.DspStream = pComponentPrivate->strmAttr;
    }

    /*copy the other information*/
    plcml_Init->SegID = G729ENC_DEFAULT_SEGMENT;
    plcml_Init->Timeout = G729ENC_SN_TIMEOUT;
    plcml_Init->Alignment = 0;
    plcml_Init->Priority = G729ENC_SN_PRIORITY;
    plcml_Init->ProfileID = 0;
    
    /* Setting Creat Phase Parameters here */
    arr[0] = G729ENC_STREAM_COUNT;
    arr[1] = G729ENC_INPUT_PORT;

    if(pComponentPrivate->dasfMode == 1)
    {
        arr[2] = G729ENC_INSTRM;
        arr[3] = G729ENC_NUM_INPUT_BUFFERS_DASF;
    }
    else
    {
        arr[2] = G729ENC_DMM;
        if (pComponentPrivate->pInputBufferList->numBuffers)
        {
            arr[3] = (OMX_U16) pComponentPrivate->pInputBufferList->numBuffers;
        }
        else
        {
            arr[3] = 1;
        }
    }
    arr[4] = G729ENC_OUTPUT_PORT;
    arr[5] = G729ENC_DMM;
    if (pComponentPrivate->pOutputBufferList->numBuffers)
    {
        arr[6] = (OMX_U16) pComponentPrivate->pOutputBufferList->numBuffers;
    }
    else
    {
        arr[6] = 1;
    }
    arr[7] = END_OF_CR_PHASE_ARGS;
    plcml_Init->pCrPhArgs = arr;

    /* Allocate memory for all input buffer headers... This memory pointer will be sent to LCML */
    size_lcml = nIpBuf * sizeof(G729ENC_LCML_BUFHEADERTYPE);
    pTemp_lcml = (G729ENC_LCML_BUFHEADERTYPE *)malloc(size_lcml);
    G729ENC_MEMPRINT("%d :: [ALLOC]  %p\n",__LINE__,pTemp_lcml);
    if(pTemp_lcml == NULL)
    {
        eError = OMX_ErrorInsufficientResources;
        G729ENC_EPRINT("OMX_ErrorInsufficientResources.\n");
        goto EXIT;
    }
    memset(pTemp_lcml, 0x0, size_lcml);
    pComponentPrivate->pLcmlBufHeader[G729ENC_INPUT_PORT] = pTemp_lcml;
    for (i=0; i<nIpBuf; i++)
    {
        G729ENC_DPRINT("INPUT--------- Inside Ip Loop\n");
        pTemp = pComponentPrivate->pInputBufferList->pBufHdr[i];
        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pTemp->nAllocLen = nIpBufSize;
        pTemp->nFilledLen = nIpBufSize;
        pTemp->nVersion.s.nVersionMajor = G729ENC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = G729ENC_MINOR_VER;
        pTemp->nVersion.s.nRevision = 0;
        pTemp->nVersion.s.nStep = 0;
        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = G729ENC_NOT_USED;
        pTemp_lcml->buffer = pTemp;
        pTemp_lcml->eDir = OMX_DirInput;
        OMX_G729MALLOC_STRUCT(pTemp_lcml->pIpParam, G729ENC_UAlgInBufParamStruct);
        pTemp_lcml->pIpParam->usEndOfFile = 0;
        /* This means, it is not a last buffer. This flag is to be modified by
         * the application to indicate the last buffer */
        pTemp->nFlags = G729ENC_NORMAL_BUFFER;
        pTemp++;
        pTemp_lcml++;
    }

    /* Allocate memory for all output buffer headers..
     * This memory pointer will be sent to LCML */
    size_lcml = nOpBuf * sizeof(G729ENC_LCML_BUFHEADERTYPE);
    pTemp_lcml = (G729ENC_LCML_BUFHEADERTYPE *)malloc(size_lcml);
    G729ENC_MEMPRINT("%d :: [ALLOC]  %p\n",__LINE__,pTemp_lcml);
    if(pTemp_lcml == NULL)
    {
        eError = OMX_ErrorInsufficientResources;
        G729ENC_EPRINT("OMX_ErrorInsufficientResources.\n");
        goto EXIT;
    }
    memset(pTemp_lcml, 0x0, size_lcml);
    pComponentPrivate->pLcmlBufHeader[G729ENC_OUTPUT_PORT] = pTemp_lcml;
    for (i=0; i<nOpBuf; i++)
    {
        G729ENC_DPRINT("OUTPUT--------- Inside Op Loop\n");
        pTemp = pComponentPrivate->pOutputBufferList->pBufHdr[i];
        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pTemp->nAllocLen = nOpBufSize;
        pTemp->nFilledLen = nOpBufSize;
        pTemp->nVersion.s.nVersionMajor = G729ENC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = G729ENC_MINOR_VER;
        pTemp->nVersion.s.nRevision = 0;
        pTemp->nVersion.s.nStep = 0;
        pComponentPrivate->nVersion = pTemp->nVersion.nVersion;
        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = G729ENC_NOT_USED;
        pTemp_lcml->buffer = pTemp;
        G729ENC_DPRINT("pTemp_lcml->buffer->pBuffer = %p\n", pTemp_lcml->buffer->pBuffer);
        pTemp_lcml->eDir = OMX_DirOutput;
        OMX_G729MALLOC_STRUCT(pTemp_lcml->pOpParam, G729ENC_UAlgOutBufParamStruct);
        pTemp_lcml->pOpParam->ulFrameCount = 0;
        /* This means, it is not a last buffer. This flag is to be modified by
         * the application to indicate the last buffer */
        pTemp->nFlags = G729ENC_NORMAL_BUFFER;
        pTemp++;
        pTemp_lcml++;
    }
#ifdef __PERF_INSTRUMENTATION__
    pComponentPrivate->nLcml_nCntIp = 0;
    pComponentPrivate->nLcml_nCntOpReceived = 0;
#endif       
    pComponentPrivate->bPortDefsAllocated = 1;
    pComponentPrivate->bInitParamsInitialized = 1;
 EXIT:
    G729ENC_DPRINT("Exiting. Returning = 0x%x\n", eError);
    return eError;
}

/* ========================================================================== */
/**
 * @G729ENC_StartComponentThread() This function is called by the component to create
 * the component thread, command pipes, data pipes and LCML Pipes.
 *
 * @param pComponent  handle for this instance of the component
 *
 * @pre
 *
 * @post
 *
 * @return none
 */
/* ========================================================================== */

OMX_ERRORTYPE G729ENC_StartComponentThread(OMX_HANDLETYPE pComponent)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    G729ENC_COMPONENT_PRIVATE *pComponentPrivate =
        (G729ENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
#ifdef UNDER_CE         
    pthread_attr_t attr;
    memset(&attr, 0, sizeof(attr));
    attr.__inheritsched = PTHREAD_EXPLICIT_SCHED;
    attr.__schedparam.__sched_priority = OMX_AUDIO_ENCODER_THREAD_PRIORITY;
#endif

    G729ENC_DPRINT ("Entering\n");
    
    /* Initialize all the variables*/
    pComponentPrivate->bIsStopping = 0;
    pComponentPrivate->bIsThreadstop = 0;
    pComponentPrivate->lcml_nOpBuf = 0;
    pComponentPrivate->lcml_nIpBuf = 0;
    pComponentPrivate->app_nBuf = 0;
    pComponentPrivate->num_Op_Issued = 0;
    pComponentPrivate->num_Sent_Ip_Buff = 0;
    pComponentPrivate->num_Reclaimed_Op_Buff = 0;
    pComponentPrivate->bIsEOFSent = 0;
    /* create the pipe used to send buffers to the thread */
    eError = pipe (pComponentPrivate->cmdDataPipe);
    if (eError)
    {
        eError = OMX_ErrorInsufficientResources;
        G729ENC_EPRINT("while creating cmdDataPipe.\n");
        goto EXIT;
    }
    /* create the pipe used to send buffers to the thread */
    eError = pipe (pComponentPrivate->dataPipe);
    if (eError)
    {
        eError = OMX_ErrorInsufficientResources;
        G729ENC_EPRINT("while creating dataPipe.\n");
        goto EXIT;
    }

    /* create the pipe used to send commands to the thread */
    eError = pipe (pComponentPrivate->cmdPipe);
    if (eError)
    {
        eError = OMX_ErrorInsufficientResources;
        G729ENC_EPRINT("while creating cmdPipe.\n");
        goto EXIT;
    }


    /* Create the Component Thread */
#ifdef UNDER_CE         
    eError = pthread_create (&(pComponentPrivate->ComponentThread), &attr, G729ENC_CompThread, pComponentPrivate);
#else
    eError = pthread_create (&(pComponentPrivate->ComponentThread), NULL, G729ENC_CompThread, pComponentPrivate);
#endif  
    if (eError || !pComponentPrivate->ComponentThread)
    {
        eError = OMX_ErrorInsufficientResources;
        G729ENC_EPRINT("OMX_ErrorInsufficientResources.\n");
        goto EXIT;
    }
    pComponentPrivate_CC = pComponentPrivate; 

    pComponentPrivate->bCompThreadStarted = 1;
 EXIT:
    G729ENC_DPRINT("Exiting. Returning = 0x%x\n", eError);
    return eError;
}

/* ========================================================================== */
/**
 * @G729ENC_FreeCompResources() This function is called by the component during
 * de-init , to free Command pipe, data pipe & LCML pipe.
 *
 * @param pComponent  handle for this instance of the component
 *
 * @pre
 *
 * @post
 *
 * @return none
 */
/* ========================================================================== */

OMX_ERRORTYPE G729ENC_FreeCompResources(OMX_HANDLETYPE pComponent)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_ERRORTYPE err = OMX_ErrorNone;
    OMX_U32 nIpBuf = 0;
    OMX_U32 nOpBuf = 0;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    G729ENC_COMPONENT_PRIVATE *pComponentPrivate = (G729ENC_COMPONENT_PRIVATE *)
        pHandle->pComponentPrivate;
    OMX_U8* pAlgParmTemp = (OMX_U8*)pComponentPrivate->pAlgParam;
    OMX_U8* pParmsTemp = (OMX_U8*)pComponentPrivate->pParams;
    
    G729ENC_DPRINT("Entering\n");
    if (pComponentPrivate->bPortDefsAllocated)
    {
        nIpBuf = pComponentPrivate->pPortDef[G729ENC_INPUT_PORT]->nBufferCountActual;
        nOpBuf = pComponentPrivate->pPortDef[G729ENC_OUTPUT_PORT]->nBufferCountActual;
    }

    if (pComponentPrivate->bCompThreadStarted)
    {
        OMX_G729CLOSE_PIPE(pComponentPrivate->dataPipe[0],err);
        OMX_G729CLOSE_PIPE(pComponentPrivate->dataPipe[1],err);
        OMX_G729CLOSE_PIPE(pComponentPrivate->cmdPipe[0],err);
        OMX_G729CLOSE_PIPE(pComponentPrivate->cmdPipe[1],err);
        OMX_G729CLOSE_PIPE(pComponentPrivate->cmdDataPipe[0],err);
        OMX_G729CLOSE_PIPE(pComponentPrivate->cmdDataPipe[1],err);

    }

    if (pAlgParmTemp != NULL)
        pAlgParmTemp -= 128;
    pComponentPrivate->pAlgParam = (G729ENC_TALGCtrl*)pAlgParmTemp;
    OMX_G729MEMFREE_STRUCT(pComponentPrivate->pAlgParam);

    if (pParmsTemp != NULL)
        pParmsTemp -= 128;
    pComponentPrivate->pParams = (G729ENC_AudioCodecParams*)pParmsTemp;
    OMX_G729MEMFREE_STRUCT(pComponentPrivate->pParams);

    if (pComponentPrivate->bPortDefsAllocated)
    {
        OMX_G729MEMFREE_STRUCT(pComponentPrivate->pPortDef[G729ENC_INPUT_PORT]);
        OMX_G729MEMFREE_STRUCT(pComponentPrivate->pPortDef[G729ENC_OUTPUT_PORT]);
        OMX_G729MEMFREE_STRUCT(pComponentPrivate->pcmParams);
        OMX_G729MEMFREE_STRUCT(pComponentPrivate->g729Params);
        OMX_G729MEMFREE_STRUCT(pComponentPrivate->pCompPort[G729ENC_INPUT_PORT]->pPortFormat);
        OMX_G729MEMFREE_STRUCT(pComponentPrivate->pCompPort[G729ENC_OUTPUT_PORT]->pPortFormat);
        OMX_G729MEMFREE_STRUCT(pComponentPrivate->pCompPort[G729ENC_INPUT_PORT]);
        OMX_G729MEMFREE_STRUCT(pComponentPrivate->pCompPort[G729ENC_OUTPUT_PORT]);
        OMX_G729MEMFREE_STRUCT(pComponentPrivate->sPortParam);
        OMX_G729MEMFREE_STRUCT(pComponentPrivate->sPriorityMgmt);
        OMX_G729MEMFREE_STRUCT(pComponentPrivate->pInputBufferList);
        OMX_G729MEMFREE_STRUCT(pComponentPrivate->pOutputBufferList);
    }
    pComponentPrivate->bPortDefsAllocated = 0;
 EXIT:
    G729ENC_DPRINT("Exiting\n");
    return eError;
}

/* ========================================================================== */
/**
 * @G729ENC_CleanupInitParams() This function is called by the component during
 * de-init to free structues that are been allocated at intialization stage
 *
 * @param pComponent  handle for this instance of the component
 *
 * @pre
 *
 * @post
 *
 * @return none
 */
/* ========================================================================== */

OMX_ERRORTYPE G729ENC_CleanupInitParams(OMX_HANDLETYPE pComponent)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nIpBuf = 0;
    OMX_U32 nOpBuf = 0;
    OMX_U32 i = 0;
    G729ENC_LCML_BUFHEADERTYPE *pTemp_lcml = NULL;
    OMX_U8* pParmsTemp = NULL;
    OMX_U8* pAlgParmTemp = NULL;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    G729ENC_COMPONENT_PRIVATE *pComponentPrivate = (G729ENC_COMPONENT_PRIVATE *)
        pHandle->pComponentPrivate;

    G729ENC_DPRINT("Entering\n");
    if(pComponentPrivate->dasfMode == 1)
    {
        pParmsTemp = (OMX_U8*)pComponentPrivate->pParams;
        if (pParmsTemp != NULL)
        {
            pParmsTemp -= 128;
        }
        pComponentPrivate->pParams = (G729ENC_AudioCodecParams*)pParmsTemp;
        OMX_G729MEMFREE_STRUCT(pComponentPrivate->pParams);
        OMX_G729MEMFREE_STRUCT(pComponentPrivate->strmAttr);
    }

    pAlgParmTemp = (OMX_U8*)pComponentPrivate->pAlgParam;
    if (pAlgParmTemp != NULL)
    {
        pAlgParmTemp -= 128;
    }
    pComponentPrivate->pAlgParam = (G729ENC_TALGCtrl*)pAlgParmTemp;
    OMX_G729MEMFREE_STRUCT(pComponentPrivate->pAlgParam);
    if(pComponentPrivate->nMultiFrameMode == 1)
    {
        OMX_G729MEMFREE_STRUCT(pComponentPrivate->pHoldBuffer);
        OMX_G729MEMFREE_STRUCT(pComponentPrivate->iHoldBuffer);
        OMX_G729MEMFREE_STRUCT(pComponentPrivate->iMMFDataLastBuffer);
    }
    pTemp_lcml = pComponentPrivate->pLcmlBufHeader[G729ENC_INPUT_PORT];
    nIpBuf = pComponentPrivate->nRuntimeInputBuffers;
    for(i=0; i<nIpBuf; i++)
    {
        OMX_G729MEMFREE_STRUCT(pTemp_lcml->pIpParam);
        pTemp_lcml++;
    }
    pTemp_lcml = pComponentPrivate->pLcmlBufHeader[G729ENC_OUTPUT_PORT];
    nOpBuf = pComponentPrivate->nRuntimeOutputBuffers;
    for(i=0; i<nOpBuf; i++)
    {
        OMX_G729MEMFREE_STRUCT(pTemp_lcml->pOpParam);
        pTemp_lcml++;
    }
    OMX_G729MEMFREE_STRUCT(pComponentPrivate->pLcmlBufHeader[G729ENC_INPUT_PORT]);
    OMX_G729MEMFREE_STRUCT(pComponentPrivate->pLcmlBufHeader[G729ENC_OUTPUT_PORT]);
    G729ENC_DPRINT("Exiting\n");
    return eError;
}

/* ========================================================================== */
/**
 * @G729ENC_StopComponentThread() This function is called by the component during
 * de-init to close component thread.
 *
 * @param pComponent  handle for this instance of the component
 *
 * @pre
 *
 * @post
 *
 * @return none
 */
/* ========================================================================== */

OMX_ERRORTYPE G729ENC_StopComponentThread(OMX_HANDLETYPE pComponent)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_ERRORTYPE threadError = OMX_ErrorNone;
    int pthreadError = 0;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    G729ENC_COMPONENT_PRIVATE *pComponentPrivate = (G729ENC_COMPONENT_PRIVATE *)
        pHandle->pComponentPrivate;

    pComponentPrivate->bIsThreadstop = 1;

    pthreadError = pthread_join (pComponentPrivate->ComponentThread,
                                 (void*)&threadError);                                

    if (0 != pthreadError)
    {
        eError = OMX_ErrorHardware;
        G729ENC_EPRINT("OMX_ErrorHardware.\n");
        goto EXIT;
    }
    if (OMX_ErrorNone != threadError && OMX_ErrorNone != eError)
    {
        eError = OMX_ErrorInsufficientResources;
        G729ENC_EPRINT("OMX_ErrorInsufficientResources.\n");
        goto EXIT;
    }

 EXIT:
    G729ENC_DPRINT("Exiting StopComponentThread - Returning = 0x%x\n", eError);
    return eError;
}


/* ========================================================================== */
/**
 * @G729ENC_HandleCommand() This function is called by the component when ever it
 * receives the command from the application
 *
 * @param pComponentPrivate  Component private data
 *
 * @pre
 *
 * @post
 *
 * @return none
 */
/* ========================================================================== */

OMX_U32 G729ENC_HandleCommand (G729ENC_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMMANDTYPE command;
    OMX_STATETYPE commandedState = OMX_StateInvalid;
    OMX_HANDLETYPE pLcmlHandle;
    LCML_CALLBACKTYPE cb;
    LCML_DSP *pLcmlDsp = NULL;
    OMX_U32 cmdValues[4] = {0};
    OMX_U32 pValues[4] = {0};
    OMX_U32 commandData  = 0;
#ifdef RESOURCE_MANAGER_ENABLED
    OMX_ERRORTYPE rm_error = OMX_ErrorNone;
#endif    
    OMX_U16 arr[100] = {0};
    char *pArgs = "damedesuStr";
    char *p = "hello";
    OMX_U8* pParmsTemp = NULL;
    OMX_U8* pAlgParmTemp = NULL;
    OMX_U32 i = 0;
    OMX_U32 ret = 0;
    G729ENC_LCML_BUFHEADERTYPE *pLcmlHdr = NULL;

    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;
    pLcmlHandle = pComponentPrivate->pLcmlHandle;
    
    G729ENC_DPRINT("Entering\n");
    G729ENC_DPRINT("pComponentPrivate->curState = %d\n", pComponentPrivate->curState);
    ret = read(pComponentPrivate->cmdPipe[0], &command, sizeof (command));
    if (ret == -1)
    {
        eError = OMX_ErrorHardware;
        G729ENC_EPRINT("in Reading from the Command pipe.\n");
        goto EXIT;
    }
    ret = read(pComponentPrivate->cmdDataPipe[0], &commandData, sizeof (commandData));
    if (ret == -1)
    {
        eError = OMX_ErrorHardware;
        G729ENC_EPRINT("in Reading from the cmdData pipe.\n");
        goto EXIT;
    }

#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedCommand(pComponentPrivate->pPERFcomp, command, commandData,
                         PERF_ModuleLLMM);
#endif
    if (command == OMX_CommandStateSet)
    {
        commandedState = (OMX_STATETYPE)commandData;
        if (pComponentPrivate->curState == commandedState)
        {
            pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                   pHandle->pApplicationPrivate,
                                                   OMX_EventError,
                                                   OMX_ErrorSameState,
                                                   0,
                                                   NULL);
            G729ENC_EPRINT("OMX_ErrorSameState Given by Comp\n");
        }
        else 
        {
            switch(commandedState)
            {
            case OMX_StateIdle:
                G729ENC_DPRINT("Case OMX_StateIdle \n");
                if (pComponentPrivate->curState == OMX_StateLoaded || pComponentPrivate->curState == OMX_StateWaitForResources)
                {
#ifdef __PERF_INSTRUMENTATION__
                    PERF_Boundary(pComponentPrivate->pPERFcomp,
                                  PERF_BoundaryStart | PERF_BoundarySetup);
#endif
                    if (pComponentPrivate->dasfMode == 1)
                    {
                        if(pComponentPrivate->streamID == 0)
                        { 
                            eError = OMX_ErrorInsufficientResources; 
                            G729ENC_EPRINT("OMX_ErrorInsufficientResources.\n");
                            pComponentPrivate->curState = OMX_StateInvalid; 
                            pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                                   pHandle->pApplicationPrivate, 
                                                                   OMX_EventError,
                                                                   OMX_ErrorInvalidState,
                                                                   0,
                                                                   NULL);
                            goto EXIT; 
                        }
                    }
                    if (pComponentPrivate->pPortDef[G729ENC_INPUT_PORT]->bEnabled == OMX_TRUE
                        && pComponentPrivate->pPortDef[G729ENC_OUTPUT_PORT]->bEnabled == OMX_TRUE) 
                    {
                        if (!(pComponentPrivate->pPortDef[G729ENC_OUTPUT_PORT]->bPopulated)
                            && !(pComponentPrivate->pPortDef[G729ENC_OUTPUT_PORT]->bPopulated))
                        {
                            pComponentPrivate->InLoaded_readytoidle = 1;
#ifndef UNDER_CE
                            pthread_mutex_lock(&pComponentPrivate->InLoaded_mutex); 
                            pthread_cond_wait(&pComponentPrivate->InLoaded_threshold,
                                              &pComponentPrivate->InLoaded_mutex);
                            pthread_mutex_unlock(&pComponentPrivate->InLoaded_mutex);
#else
                            OMX_WaitForEvent(&(pComponentPrivate->InLoaded_event));
#endif
                        }
                    }
                    cb.LCML_Callback = (void *) G729ENC_LCMLCallback;
                    pLcmlHandle = (OMX_HANDLETYPE) G729ENC_GetLCMLHandle(pComponentPrivate); 
                    if (pLcmlHandle == NULL)
                    {
                        G729ENC_DPRINT("LCML Handle is NULL........exiting..\n");
                        goto EXIT;
                    }
                    /* Got handle of dsp via phandle filling information about DSP Specific things */
                    pLcmlDsp = (((LCML_DSP_INTERFACE*)pLcmlHandle)->dspCodec);
                    eError = G729ENC_FillLCMLInitParams(pHandle, pLcmlDsp, arr); 
                    if(eError != OMX_ErrorNone)
                    {
                        G729ENC_EPRINT("from G729ENCFill_LCMLInitParams().\n");
                        goto EXIT;
                    }
                    pComponentPrivate->pLcmlHandle = (LCML_DSP_INTERFACE *)pLcmlHandle;
                    cb.LCML_Callback = (void *) G729ENC_LCMLCallback;
#ifndef UNDER_CE
                    eError = LCML_InitMMCodecEx(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                p, &pLcmlHandle, (void *)p,
                                                &cb, (OMX_STRING)pComponentPrivate->sDeviceString);
#else
                    eError = LCML_InitMMCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                              p, &pLcmlHandle, (void *)p, 
                                              &cb);
#endif
                    if(eError != OMX_ErrorNone)
                    {
                        G729ENC_EPRINT("from LCML_Init().\n");
                        goto EXIT;
                    }
#ifdef HASHINGENABLE
                    /* Enable the Hashing Code */
                    eError = LCML_SetHashingState(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle, OMX_TRUE);
                    if (eError != OMX_ErrorNone) {
                        G729ENC_DPRINT("Failed to set Mapping State\n");
                        goto EXIT;
                    }
#endif                        
                        
                        
#ifdef __PERF_INSTRUMENTATION__
                    PERF_Boundary(pComponentPrivate->pPERFcomp,
                                  PERF_BoundaryComplete | PERF_BoundarySetup);
#endif                                                                                         

                    pComponentPrivate->curState = OMX_StateIdle;
                    pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventCmdComplete,
                                                           OMX_CommandStateSet,
                                                           pComponentPrivate->curState,
                                                           NULL);
#ifdef RESOURCE_MANAGER_ENABLED
                    /* need check the resource with RM */ 
                    pComponentPrivate->rmproxyCallback.RMPROXY_Callback = (void *) G729ENC_ResourceManagerCallback;
                    rm_error = RMProxy_NewSendCommand(pHandle, RMProxy_RequestResource,
                                                      OMX_G729_Encoder_COMPONENT,
                                                      G729ENC_CPU,
                                                      3456,
                                                      &(pComponentPrivate->rmproxyCallback)); 
                    if(rm_error == OMX_ErrorNone) {
                        /* resource is available */
                        rm_error = RMProxy_NewSendCommand(pHandle, RMProxy_StateSet, OMX_G729_Encoder_COMPONENT, OMX_StateIdle, 3456,NULL);
                                
                    }
                    else if(rm_error == OMX_ErrorInsufficientResources) {
                        /* resource is not available, need set state to OMX_StateWaitForResources */
                        pComponentPrivate->curState = OMX_StateWaitForResources;
                        pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                               pHandle->pApplicationPrivate,
                                                               OMX_EventCmdComplete,
                                                               OMX_CommandStateSet,
                                                               pComponentPrivate->curState,
                                                               NULL);
                        G729ENC_EPRINT("%d :: OMX_G729Enc_Utils.c :: Error - insufficient resources\n", __LINE__);
                    }


#endif                                                               
                    if(pComponentPrivate->acdnMode == 1)
                    {
                        G729ENC_EPRINT("Not implemented acdn mode.\n");
                    }
                }
                else if (pComponentPrivate->curState == OMX_StateExecuting)
                {
#ifdef HASHINGENABLE
                    /*Hashing Change*/
                    pLcmlHandle = (LCML_DSP_INTERFACE*)pComponentPrivate->pLcmlHandle;
                    eError = LCML_FlushHashes(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle);
                    if (eError != OMX_ErrorNone) {
                        G729ENC_DPRINT("Error occurred in Codec mapping flush!\n");
                        break;
                    }
#endif                    
                    G729ENC_DPRINT("Setting Component to OMX_StateIdle\n");
                    G729ENC_DPRINT("About to Call MMCodecControlStop\n");
#ifdef __PERF_INSTRUMENTATION__
                    PERF_Boundary(pComponentPrivate->pPERFcomp,
                                  PERF_BoundaryComplete | PERF_BoundarySteadyState);
#endif
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               MMCodecControlStop,(void *)pArgs);
                    pAlgParmTemp = (OMX_U8*)pComponentPrivate->pAlgParam;
                    if (pAlgParmTemp != NULL)
                    {
                        pAlgParmTemp -= 128;
                    }
                    pComponentPrivate->pAlgParam = (G729ENC_TALGCtrl*)pAlgParmTemp;
                    OMX_G729MEMFREE_STRUCT(pComponentPrivate->pAlgParam);
                    if(eError != OMX_ErrorNone)
                    {
                        G729ENC_EPRINT("from LCML_ControlCodec MMCodecControlStop.\n");
                        goto EXIT;
                    }
                }
                else if(pComponentPrivate->curState == OMX_StatePause)
                {
#ifdef HASHINGENABLE
                    /*Hashing Change*/
                    pLcmlHandle = (LCML_DSP_INTERFACE*)pComponentPrivate->pLcmlHandle;
                    eError = LCML_FlushHashes(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle);
                    if (eError != OMX_ErrorNone) {
                        G729ENC_DPRINT("Error occurred in Codec mapping flush!\n");
                        break;
                    }
#endif                    
                    pComponentPrivate->curState = OMX_StateIdle;
#ifdef __PERF_INSTRUMENTATION__
                    PERF_Boundary(pComponentPrivate->pPERFcomp,
                                  PERF_BoundaryComplete | PERF_BoundarySteadyState);
#endif
                    pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventCmdComplete,
                                                           OMX_CommandStateSet,
                                                           pComponentPrivate->curState,
                                                           NULL);
                }
                else
                {    /* This means, it is invalid state from application */
                    pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventError,
                                                           OMX_ErrorIncorrectStateTransition,
                                                           0,
                                                           NULL);
                    G729ENC_EPRINT("OMX_ErrorIncorrectStateTransition.\n");
                }
                break;
            case OMX_StateExecuting:
                G729ENC_DPRINT("Case OMX_StateExecuting\n");
                if (pComponentPrivate->curState == OMX_StateIdle)
                {
                    /* Sending commands to DSP via LCML_ControlCodec third argument is not used for time being */
                    pComponentPrivate->nNumInputBufPending = 0;
                    pComponentPrivate->nNumOutputBufPending = 0;
                    pAlgParmTemp = (OMX_U8*)malloc(sizeof(G729ENC_TALGCtrl) + 256);
                    if(pAlgParmTemp == NULL)
                    {
                        G729ENC_EPRINT("*************\n"); 
                        G729ENC_EPRINT("Malloc Failed\n"); 
                        G729ENC_EPRINT("*************\n"); 
                        eError = OMX_ErrorInsufficientResources; 
                        goto EXIT;      
                    }
                    memset(pAlgParmTemp, 0x0, sizeof(G729ENC_TALGCtrl) + 256);
                    G729ENC_MEMPRINT("%d :: [ALLOC] %p\n",__LINE__,pAlgParmTemp);
                    pComponentPrivate->pAlgParam = (G729ENC_TALGCtrl*)(pAlgParmTemp + 128);
                    pComponentPrivate->pAlgParam->vadFlag = pComponentPrivate->g729Params->bDTX;
                    pComponentPrivate->pAlgParam->size = sizeof( G729ENC_TALGCtrl );
                    pComponentPrivate->pAlgParam->frameSize = 0;
                    G729ENC_DPRINT("pAlgParam->vadFlag  = %d\n",
                                   pComponentPrivate->pAlgParam->vadFlag);
                    cmdValues[0] = 100;                 /* setting the VAD flag */
                    cmdValues[1] = (OMX_U32)pComponentPrivate->pAlgParam;
                    cmdValues[2] = sizeof (G729ENC_TALGCtrl);
                    p = (void *)&cmdValues;
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               EMMCodecControlAlgCtrl, (void *)p);
                    if (eError != OMX_ErrorNone)
                    {
                        G729ENC_EPRINT("from LCML_ControlCodec.\n");
                        goto EXIT;
                    }
                    if(pComponentPrivate->dasfMode == 1)
                    {
                        G729ENC_DPRINT("DASF Functionality is ON ---\n");
                        pParmsTemp = (OMX_U8*)malloc(sizeof(G729ENC_AudioCodecParams) + 256);
                        if(pParmsTemp == NULL)
                        {
                            G729ENC_EPRINT("*************\n"); 
                            G729ENC_EPRINT("Malloc Failed\n"); 
                            G729ENC_EPRINT("*************\n"); 
                            eError = OMX_ErrorInsufficientResources; 
                            goto EXIT;      
                        } 
                        memset(pParmsTemp, 0x0, sizeof(G729ENC_AudioCodecParams));
                        pComponentPrivate->pParams = (G729ENC_AudioCodecParams*)(pParmsTemp + 128);
                        G729ENC_MEMPRINT("%d :: [ALLOC] %p\n",__LINE__,pComponentPrivate->pParams);
                        pComponentPrivate->pParams->iAudioFormat = 1;
                        pComponentPrivate->pParams->iStrmId = pComponentPrivate->streamID;
                        pComponentPrivate->pParams->iSamplingRate = G729ENC_SAMPLING_FREQUENCY;
                        pValues[0] = USN_STRMCMD_SETCODECPARAMS;
                        pValues[1] = (OMX_U32)pComponentPrivate->pParams;
                        pValues[2] = sizeof(G729ENC_AudioCodecParams);
                        /* Sending STRMCTRL MESSAGE to DSP via LCML_ControlCodec*/
                        eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                   EMMCodecControlStrmCtrl,(void *)pValues);
                        if(eError != OMX_ErrorNone)
                        {
                            G729ENC_EPRINT("from LCML_ControlCodec.\n");
                            goto EXIT;
                        }
                    }
                    pComponentPrivate->bBypassDSP = 0;
                    /* Sending START MESSAGE to DSP via LCML_ControlCodec*/
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               EMMCodecControlStart, (void *)p);
                    if(eError != OMX_ErrorNone)
                    {
                        G729ENC_EPRINT("from LCML_ControlCodec.\n");
                        goto EXIT;
                    }
                } 
                else if (pComponentPrivate->curState == OMX_StatePause)
                {
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               EMMCodecControlStart, (void *)p);
                    if (eError != OMX_ErrorNone)
                    {
                        G729ENC_EPRINT("While Resuming the codec.\n");
                        goto EXIT;
                    }
                    if (pComponentPrivate->nNumInputBufPending < pComponentPrivate->pInputBufferList->numBuffers)
                    {
                        pComponentPrivate->nNumInputBufPending = pComponentPrivate->pInputBufferList->numBuffers;
                    }
                    for (i=0; i < pComponentPrivate->nNumInputBufPending; i++)
                    {
                        if (pComponentPrivate->pInputBufHdrPending[i])
                        {
                            G729ENC_GetCorrespondingLCMLHeader(pComponentPrivate->pInputBufHdrPending[i]->pBuffer,
                                                               OMX_DirInput,
                                                               &pLcmlHdr,
                                                               pComponentPrivate);
                            G729ENC_SetPending(pComponentPrivate,
                                               pComponentPrivate->pInputBufHdrPending[i],
                                               OMX_DirInput, __LINE__);
                            eError = LCML_QueueBuffer(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                      EMMCodecInputBuffer,
                                                      pComponentPrivate->pInputBufHdrPending[i]->pBuffer,
                                                      pComponentPrivate->pInputBufHdrPending[i]->nAllocLen,
                                                      pComponentPrivate->pInputBufHdrPending[i]->nFilledLen,
                                                      (OMX_U8 *) pLcmlHdr->pIpParam,
                                                      sizeof(G729ENC_UAlgInBufParamStruct),
                                                      NULL);
                        }
                    }
                    pComponentPrivate->nNumInputBufPending = 0;
                    if (pComponentPrivate->nNumOutputBufPending < pComponentPrivate->pOutputBufferList->numBuffers)
                    {
                        pComponentPrivate->nNumOutputBufPending = pComponentPrivate->pOutputBufferList->numBuffers;
                    }
                    for (i=0; i < pComponentPrivate->nNumOutputBufPending; i++)
                    {
                        if (pComponentPrivate->pOutputBufHdrPending[i])
                        {
                            G729ENC_GetCorrespondingLCMLHeader(pComponentPrivate->pOutputBufHdrPending[i]->pBuffer,
                                                               OMX_DirOutput,
                                                               &pLcmlHdr,
                                                               pComponentPrivate);
                            G729ENC_SetPending(pComponentPrivate,
                                               pComponentPrivate->pOutputBufHdrPending[i],
                                               OMX_DirOutput, __LINE__);
                            eError = LCML_QueueBuffer(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                      EMMCodecOuputBuffer,
                                                      pComponentPrivate->pOutputBufHdrPending[i]->pBuffer,
                                                      pComponentPrivate->pOutputBufHdrPending[i]->nAllocLen,
                                                      pComponentPrivate->pOutputBufHdrPending[i]->nFilledLen,
                                                      (OMX_U8 *) pLcmlHdr->pOpParam,
                                                      sizeof(G729ENC_UAlgInBufParamStruct),
                                                      NULL);
                        }
                    }
                    pComponentPrivate->nNumOutputBufPending = 0;
                } 
                else
                {
                    pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventError,
                                                           OMX_ErrorIncorrectStateTransition,
                                                           0, NULL);
                    G729ENC_DPRINT("OMX_ErrorIncorrectStateTransition Given by Comp.\n");
                    goto EXIT;
                }
                pComponentPrivate->curState = OMX_StateExecuting;
#ifdef __PERF_INSTRUMENTATION__
                PERF_Boundary(pComponentPrivate->pPERFcomp,
                              PERF_BoundaryStart | PERF_BoundarySteadyState);
#endif          
                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandStateSet,
                                                       pComponentPrivate->curState,
                                                       NULL);
#ifdef RESOURCE_MANAGER_ENABLED
                rm_error = RMProxy_NewSendCommand(pHandle, RMProxy_StateSet, OMX_G729_Encoder_COMPONENT, OMX_StateExecuting, 3456,NULL);
#endif
                G729ENC_DPRINT("OMX_CommandStateSet Given by Comp\n");
                break;

            case OMX_StateLoaded:
                G729ENC_DPRINT("Case OMX_StateLoaded\n");
                if (pComponentPrivate->curState == OMX_StateWaitForResources)
                {
                    G729ENC_DPRINT("OMX_StateWaitForResources\n");
#ifdef __PERF_INSTRUMENTATION__
                    PERF_Boundary(pComponentPrivate->pPERFcomp,
                                  PERF_BoundaryStart | PERF_BoundaryCleanup); 
#endif
                    pComponentPrivate->curState = OMX_StateLoaded;
#ifdef __PERF_INSTRUMENTATION__
                    PERF_Boundary(pComponentPrivate->pPERFcomp,
                                  PERF_BoundaryComplete | PERF_BoundaryCleanup);
#endif
                    pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventCmdComplete,
                                                           OMX_CommandStateSet,
                                                           pComponentPrivate->curState,
                                                           NULL);
                    G729ENC_DPRINT("OMX_CommandStateSet Given by Comp\n");
                    break;
                }
                if (pComponentPrivate->curState != OMX_StateIdle &&
                    pComponentPrivate->curState != OMX_StateWaitForResources)
                {
                    G729ENC_DPRINT("OMX_StateIdle && OMX_StateWaitForResources\n");
                    pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventError,
                                                           OMX_ErrorIncorrectStateTransition,
                                                           0, NULL);
                    G729ENC_EPRINT("OMX_ErrorIncorrectStateTransition Given by Comp.\n");
                    goto EXIT;
                }
#ifdef __PERF_INSTRUMENTATION__
                PERF_Boundary(pComponentPrivate->pPERFcomp,
                              PERF_BoundaryStart | PERF_BoundaryCleanup);
#endif
                if (pComponentPrivate->pInputBufferList->numBuffers &&
                    pComponentPrivate->pOutputBufferList->numBuffers)
                {
                    pComponentPrivate->InIdle_goingtoloaded = 1;
#ifndef UNDER_CE
                    pthread_mutex_lock(&pComponentPrivate->InIdle_mutex); 
                    pthread_cond_wait(&pComponentPrivate->InIdle_threshold,
                                      &pComponentPrivate->InIdle_mutex);
                    pthread_mutex_unlock(&pComponentPrivate->InIdle_mutex);
#else
                    OMX_WaitForEvent(&(pComponentPrivate->InIdle_event));
#endif
                }
                /* Now Deinitialize the component No error should be returned from
                 * this function. It should clean the system as much as possible */
                G729ENC_CleanupInitParams(pHandle);
                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                           EMMCodecControlDestroy, (void *)p);
                if (eError != OMX_ErrorNone)
                {
                    G729ENC_EPRINT("LCML_ControlCodec.\n");
                    goto EXIT;
                }
#ifdef UNDER_CE
                FreeLibrary(g_hLcmlDllHandle);
                g_hLcmlDllHandle = NULL;
#endif                    
#ifdef __PERF_INSTRUMENTATION__
                PERF_SendingCommand(pComponentPrivate->pPERF, -1, 0,
                                    PERF_ModuleComponent);
#endif
                eError = G729ENC_EXIT_COMPONENT_THRD;
                pComponentPrivate->bInitParamsInitialized = 0;
                pComponentPrivate->bLoadedCommandPending = OMX_FALSE;
                break;

            case OMX_StatePause:
                G729ENC_DPRINT("Case OMX_StatePause\n");
                if (pComponentPrivate->curState != OMX_StateExecuting &&
                    pComponentPrivate->curState != OMX_StateIdle)
                {
                    pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventError,
                                                           OMX_ErrorIncorrectStateTransition,
                                                           0, NULL);
                    G729ENC_EPRINT("OMX_ErrorIncorrectStateTransition Given by Comp.\n");
                    goto EXIT;
                }
#ifdef __PERF_INSTRUMENTATION__
                PERF_Boundary(pComponentPrivate->pPERFcomp,
                              PERF_BoundaryComplete | PERF_BoundarySteadyState);
#endif
                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                           EMMCodecControlPause, (void *)p);
                if (eError != OMX_ErrorNone)
                {
                    G729ENC_EPRINT("LCML_ControlCodec.\n");
                    goto EXIT;
                }
                G729ENC_DPRINT("OMX_CommandStateSet Given by Comp\n");
                break;

            case OMX_StateWaitForResources:
                G729ENC_DPRINT("Case OMX_StateWaitForResources\n");
                if (pComponentPrivate->curState == OMX_StateLoaded)
                {
#ifdef RESOURCE_MANAGER_ENABLED         
                    rm_error = RMProxy_NewSendCommand(pHandle, RMProxy_StateSet, OMX_G729_Encoder_COMPONENT, OMX_StateWaitForResources, 3456,NULL);
#endif 
                    pComponentPrivate->curState = OMX_StateWaitForResources;
                    pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventCmdComplete,
                                                           OMX_CommandStateSet,
                                                           pComponentPrivate->curState,
                                                           NULL);
                    G729ENC_DPRINT("%d :: Comp: OMX_CommandStateSet Given by Comp\n",__LINE__);
                }
                else
                {
                    pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventError,
                                                           OMX_ErrorIncorrectStateTransition,
                                                           0, NULL);
                    G729ENC_EPRINT("OMX_ErrorIncorrectStateTransition Given by Comp.\n");
                }
                break;

            case OMX_StateInvalid:
                G729ENC_DPRINT("Case OMX_StateInvalid\n");
                pComponentPrivate->curState = OMX_StateInvalid;
                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventError,
                                                       OMX_ErrorInvalidState,
                                                       0, NULL);
                G729ENC_EPRINT("OMX_ErrorInvalidState Given by Comp.\n");

                if (pComponentPrivate->curState != OMX_StateWaitForResources && 
                    pComponentPrivate->curState != OMX_StateLoaded &&
                    pComponentPrivate->curState != OMX_StateInvalid) {

                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               EMMCodecControlDestroy, (void *)p);
                }                                                                                                     
                G729ENC_CleanupInitParams(pHandle);
                    
                break;

            case OMX_StateMax:
                G729ENC_DPRINT("Case OMX_StateMax\n");
                break;
            } /* End of Switch */
        }
    } 
    else if (command == OMX_CommandMarkBuffer)
    {
        G729ENC_DPRINT("command OMX_CommandMarkBuffer received\n");
        if(!pComponentPrivate->pMarkBuf)
        {
            /* TODO Need to handle multiple marks */
            pComponentPrivate->pMarkBuf = (OMX_MARKTYPE *)(commandData);
        }
    } 
    else if (command == OMX_CommandPortDisable)
    {
        if (!pComponentPrivate->bDisableCommandPending)
        {
            if(commandData == 0x0 || commandData == -1)
            {
                pComponentPrivate->pPortDef[G729ENC_INPUT_PORT]->bEnabled = OMX_FALSE;
            }
            if(commandData == 0x1 || commandData == -1)
            {
                char *pArgs = "damedesuStr";
                pComponentPrivate->pPortDef[G729ENC_OUTPUT_PORT]->bEnabled = OMX_FALSE;
                if (pComponentPrivate->curState == OMX_StateExecuting)
                {
                    pComponentPrivate->bNoIdleOnStop = OMX_TRUE;
                    eError = LCML_ControlCodec(
                                               ((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               MMCodecControlStop,(void *)pArgs);
                }
            }
        }
        G729ENC_DPRINT("commandData = %d\n",commandData);
        G729ENC_DPRINT("pComponentPrivate->pPortDef[G729ENC_INPUT_PORT]->bPopulated = %d\n",
                       pComponentPrivate->pPortDef[G729ENC_INPUT_PORT]->bPopulated);
        G729ENC_DPRINT("pComponentPrivate->pPortDef[G729ENC_OUTPUT_PORT]->bPopulated = %d\n",
                       pComponentPrivate->pPortDef[G729ENC_OUTPUT_PORT]->bPopulated);
        if(commandData == 0x0)
        {
            if(!pComponentPrivate->pPortDef[G729ENC_INPUT_PORT]->bPopulated)
            {
                /* return cmdcomplete event if input unpopulated */ 
                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandPortDisable,
                                                       G729ENC_INPUT_PORT, NULL);
                pComponentPrivate->bDisableCommandPending = 0;
            }
            else
            {
                pComponentPrivate->bDisableCommandPending = 1;
                pComponentPrivate->bDisableCommandParam = commandData;
            }
        }
        if(commandData == 0x1)
        {
            if (!pComponentPrivate->pPortDef[G729ENC_OUTPUT_PORT]->bPopulated)
            {
                /* return cmdcomplete event if output unpopulated */ 
                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandPortDisable,
                                                       G729ENC_OUTPUT_PORT, NULL);
                pComponentPrivate->bDisableCommandPending = 0;
            }
            else
            {
                pComponentPrivate->bDisableCommandPending = 1;
                pComponentPrivate->bDisableCommandParam = commandData;
            }
        }
        if(commandData == -1)
        {
            if (!pComponentPrivate->pPortDef[G729ENC_INPUT_PORT]->bPopulated && 
                !pComponentPrivate->pPortDef[G729ENC_OUTPUT_PORT]->bPopulated)
            {
                /* return cmdcomplete event if inout & output unpopulated */ 
                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandPortDisable,
                                                       G729ENC_INPUT_PORT, NULL);
                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandPortDisable,
                                                       G729ENC_OUTPUT_PORT, NULL);
                pComponentPrivate->bDisableCommandPending = 0;
            }
            else
            {
                pComponentPrivate->bDisableCommandPending = 1;
                pComponentPrivate->bDisableCommandParam = commandData;
            }
        }
    } 
    else if (command == OMX_CommandPortEnable)
    {
        if(!pComponentPrivate->bEnableCommandPending) {
            if(commandData == 0x0 || commandData == -1)
            {
                /* enable in port */
                G729ENC_DPRINT("setting input port to enabled\n");
                pComponentPrivate->pPortDef[G729ENC_INPUT_PORT]->bEnabled = OMX_TRUE;
                G729ENC_DPRINT("pComponentPrivate->pPortDef[G729ENC_INPUT_PORT]->bEnabled = %d\n",
                               pComponentPrivate->pPortDef[G729ENC_INPUT_PORT]->bEnabled);
                if(pComponentPrivate->AlloBuf_waitingsignal)
                {
                    pComponentPrivate->AlloBuf_waitingsignal = 0;
#ifndef UNDER_CE
                    pthread_mutex_lock(&pComponentPrivate->AlloBuf_mutex); 
                    pthread_cond_signal(&pComponentPrivate->AlloBuf_threshold);
                    pthread_mutex_unlock(&pComponentPrivate->AlloBuf_mutex);    
#else
                    OMX_SignalEvent(&(pComponentPrivate->AlloBuf_event));
#endif
                }
            }
            if(commandData == 0x1 || commandData == -1)
            {
                /* enable out port */
                if(pComponentPrivate->AlloBuf_waitingsignal)
                {
                    pComponentPrivate->AlloBuf_waitingsignal = 0;
                }
                if (pComponentPrivate->curState == OMX_StateExecuting)
                {
                    char *pArgs = "damedesuStr";
                    pComponentPrivate->bDspStoppedWhileExecuting = OMX_FALSE;
                    eError = LCML_ControlCodec(
                                               ((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               EMMCodecControlStart,(void *)pArgs);
                }
                G729ENC_DPRINT("setting output port to enabled\n");
                pComponentPrivate->pPortDef[G729ENC_OUTPUT_PORT]->bEnabled = OMX_TRUE;
                G729ENC_DPRINT("pComponentPrivate->pPortDef[G729ENC_OUTPUT_PORT]->bEnabled = %d\n",
                               pComponentPrivate->pPortDef[G729ENC_OUTPUT_PORT]->bEnabled);
            }
 

            G729ENC_DPRINT("commandData = %d\n",commandData);
            G729ENC_DPRINT("pComponentPrivate->curState = %d\n",pComponentPrivate->curState);
            G729ENC_DPRINT("pComponentPrivate->pPortDef[G729ENC_INPUT_PORT]->bPopulated = %d\n",
                           pComponentPrivate->pPortDef[G729ENC_INPUT_PORT]->bPopulated);
        }
        if(commandData == 0x0){
            if (pComponentPrivate->curState == OMX_StateLoaded || 
                pComponentPrivate->pPortDef[G729ENC_INPUT_PORT]->bPopulated) {
                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandPortEnable,
                                                       G729ENC_INPUT_PORT,
                                                       NULL);
                pComponentPrivate->bEnableCommandPending = 0;
            }
            else {
                pComponentPrivate->bEnableCommandPending = 1;
                pComponentPrivate->bEnableCommandParam = commandData;
            }
        }
        else if(commandData == 0x1) {
            if (pComponentPrivate->curState == OMX_StateLoaded || 
                pComponentPrivate->pPortDef[G729ENC_OUTPUT_PORT]->bPopulated){
                pComponentPrivate->cbInfo.EventHandler( pHandle, 
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,
                                                        OMX_CommandPortEnable,
                                                        G729ENC_OUTPUT_PORT, 
                                                        NULL);
                pComponentPrivate->bEnableCommandPending = 0;
            }
            else {
                pComponentPrivate->bEnableCommandPending = 1;
                pComponentPrivate->bEnableCommandParam = commandData;
            }
        }
        else if(commandData == -1) {
            if (pComponentPrivate->curState == OMX_StateLoaded || 
                (pComponentPrivate->pPortDef[G729ENC_INPUT_PORT]->bPopulated
                 && pComponentPrivate->pPortDef[G729ENC_OUTPUT_PORT]->bPopulated)){
                pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete, 
                                                       OMX_CommandPortEnable,
                                                       G729ENC_INPUT_PORT, 
                                                       NULL);
                pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete, 
                                                       OMX_CommandPortEnable,
                                                       G729ENC_OUTPUT_PORT, 
                                                       NULL);
                pComponentPrivate->bEnableCommandPending = 0;
                G729ENC_FillLCMLInitParamsEx(pHandle);
            }
            else {
                pComponentPrivate->bEnableCommandPending = 1;
                pComponentPrivate->bEnableCommandParam = commandData;
            }
        }
       
#ifndef UNDER_CE
        pthread_mutex_lock(&pComponentPrivate->AlloBuf_mutex); 
        pthread_cond_signal(&pComponentPrivate->AlloBuf_threshold);
        pthread_mutex_unlock(&pComponentPrivate->AlloBuf_mutex);    
#else
        OMX_SignalEvent(&(pComponentPrivate->AlloBuf_event));
#endif
    } 

    else if (command == OMX_CommandFlush)
    {
        OMX_U32 aParam[3] = {0};
        if(commandData == 0x0 || commandData == -1)
        {
            if (pComponentPrivate->nUnhandledEmptyThisBuffers == 0) {
                aParam[0] = USN_STRMCMD_FLUSH; 
                aParam[1] = 0x0; 
                aParam[2] = 0x0; 
                G729ENC_DPRINT("Flushing input port\n");
                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                           EMMCodecControlStrmCtrl, (void*)aParam);
                if (eError != OMX_ErrorNone)
                {
                    G729ENC_DPRINT("Error flushing input port: %d\n", eError);
                    goto EXIT;
                }
            }
            else{
                pComponentPrivate->bFlushInputPortCommandPending = OMX_TRUE;
            }
        }
        if(commandData == 0x1 || commandData == -1)
        {
            if ((pComponentPrivate->nUnhandledFillThisBuffers) == 0)  {
                aParam[0] = USN_STRMCMD_FLUSH; 
                aParam[1] = 0x1; 
                aParam[2] = 0x0; 
                G729ENC_DPRINT("Flushing output port\n");
                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                           EMMCodecControlStrmCtrl, (void*)aParam);
                if (eError != OMX_ErrorNone)
                {
                    G729ENC_EPRINT("Error flushing output port: %d\n", eError);                
                    goto EXIT;
                }
            }
            else{
                pComponentPrivate->bFlushOutputPortCommandPending = OMX_TRUE; 
            }
        }
        else{
            eError = OMX_ErrorBadPortIndex;
        }
    }
 EXIT:
    G729ENC_DPRINT("Exiting. Returning = 0x%x\n", eError);
    return eError;
}

/* ========================================================================== */
/**
 * @G729ENC_HandleDataBufFromApp() This function is called by the component when ever it
 * receives the buffer from the application
 *
 * @param pComponentPrivate  Component private data
 * @param pBufHeader Buffer from the application
 *
 * @pre
 *
 * @post
 *
 * @return none
 */
/* ========================================================================== */
OMX_ERRORTYPE G729ENC_HandleDataBufFromApp(OMX_BUFFERHEADERTYPE* pBufHeader,
                                           G729ENC_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_DIRTYPE eDir;
    G729ENC_LCML_BUFHEADERTYPE *pLcmlHdr = NULL;
    LCML_DSP_INTERFACE *pLcmlHandle = (LCML_DSP_INTERFACE *)
        pComponentPrivate->pLcmlHandle;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefIn = NULL;    
    
    G729ENC_DPRINT ("Entering\n");
    /*Find the direction of the received buffer from buffer list */
    eError = G729ENC_GetBufferDirection(pBufHeader, &eDir);
    if (eError != OMX_ErrorNone)
    {
        G729ENC_EPRINT("The pBufHeader is not found in the list.\n");
        goto EXIT;
    }
    if (eDir == OMX_DirInput)
    {
        pComponentPrivate->nUnhandledEmptyThisBuffers--;
        pPortDefIn = pComponentPrivate->pPortDef[OMX_DirInput];             
        if (pBufHeader->nFilledLen > 0) /* || pBufHeader->nFlags == OMX_BUFFERFLAG_EOS) */
        {
            pComponentPrivate->bBypassDSP = 0;
            eError = G729ENC_GetCorrespondingLCMLHeader(pBufHeader->pBuffer, 
                                                        OMX_DirInput, &pLcmlHdr, pComponentPrivate);
            if (eError != OMX_ErrorNone)
            {
                G729ENC_EPRINT("Invalid Buffer Came.\n");
                goto EXIT;
            }
            pLcmlHdr->pIpParam->usEndOfFile = 0;
#ifdef  __PERF_INSTRUMENTATION__
            PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                              PREF(pBufHeader,pBuffer),
                              pPortDefIn->nBufferSize, 
                              PERF_ModuleCommonLayer);
#endif

            if(pBufHeader->nFlags == OMX_BUFFERFLAG_EOS)
            {
                pComponentPrivate->bIsEOFSent = 1;
                if(pComponentPrivate->dasfMode == 0)
                {
                    pComponentPrivate->pOutputBufferList->pBufHdr[0]->nFlags |= OMX_BUFFERFLAG_EOS;
                    pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                           pComponentPrivate->pHandle->pApplicationPrivate,
                                                           OMX_EventBufferFlag,
                                                           pComponentPrivate->pOutputBufferList->pBufHdr[0]->nOutputPortIndex,
                                                           pComponentPrivate->pOutputBufferList->pBufHdr[0]->nFlags,
                                                           NULL);
                }
                pBufHeader->nFlags = 0;
                pLcmlHdr->pIpParam->usEndOfFile=1;
            }
            /* Store time stamp information */
            pComponentPrivate->arrTimestamp[pComponentPrivate->IpBufindex] = pBufHeader->nTimeStamp;
            pComponentPrivate->arrTickCount[pComponentPrivate->IpBufindex] = pBufHeader->nTickCount;                
            pComponentPrivate->IpBufindex++;
            pComponentPrivate->IpBufindex %= pComponentPrivate->pPortDef[G729ENC_OUTPUT_PORT]->nBufferCountActual;
            
            if (pComponentPrivate->curState == OMX_StateExecuting)
            {
                if (!G729ENC_IsPending(pComponentPrivate,pBufHeader,OMX_DirInput))
                {
                    G729ENC_SetPending(pComponentPrivate,pBufHeader,OMX_DirInput,__LINE__);
                    eError = LCML_QueueBuffer(pLcmlHandle->pCodecinterfacehandle,
                                              EMMCodecInputBuffer,
                                              (OMX_U8 *)pBufHeader->pBuffer,
                                              pBufHeader->nAllocLen,
                                              pBufHeader->nFilledLen,
                                              (OMX_U8 *) pLcmlHdr->pIpParam,
                                              sizeof(G729ENC_UAlgInBufParamStruct),
                                              NULL);
                    if (eError != OMX_ErrorNone)
                    {
                        G729ENC_EPRINT("error hardware:: %d\n", __LINE__);
                        eError = OMX_ErrorHardware;
                        goto EXIT;
                    }
                    pComponentPrivate->lcml_nCntIp++;
                    pComponentPrivate->lcml_nIpBuf++;
                    pComponentPrivate->num_Sent_Ip_Buff++;
                }
            }
            else if (pComponentPrivate->curState == OMX_StatePause)
            {
                pComponentPrivate->pInputBufHdrPending[pComponentPrivate->nNumInputBufPending++] = pBufHeader;
            }
        } 
        else
        { 
            if(pBufHeader->nFlags == OMX_BUFFERFLAG_EOS)
            {
                G729ENC_DPRINT("Line %d\n",__LINE__);            
                pComponentPrivate->bIsEOFSent = 1;
                if(pComponentPrivate->dasfMode == 0)
                {
                    pComponentPrivate->pOutputBufferList->pBufHdr[0]->nFlags |= OMX_BUFFERFLAG_EOS;
                    
                    pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                           pComponentPrivate->pHandle->pApplicationPrivate,
                                                           OMX_EventBufferFlag,
                                                           pComponentPrivate->pOutputBufferList->pBufHdr[0]->nOutputPortIndex,
                                                           pComponentPrivate->pOutputBufferList->pBufHdr[0]->nFlags,
                                                           NULL);

                    pComponentPrivate->cbInfo.FillBufferDone (
                                                              pComponentPrivate->pHandle,
                                                              pComponentPrivate->pHandle->pApplicationPrivate,
                                                              pComponentPrivate->pOutputBufferList->pBufHdr[0]);
                                                               
                }
                pBufHeader->nFlags = 0;
            }
            G729ENC_DPRINT("Calling EmptyBufferDone\n");
#ifdef __PERF_INSTRUMENTATION__
            PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                              pComponentPrivate->pInputBufferList, 0,
                              PERF_ModuleHLMM);
#endif
            pComponentPrivate->cbInfo.EmptyBufferDone(pComponentPrivate->pHandle,
                                                      pComponentPrivate->pHandle->pApplicationPrivate,
                                                      pComponentPrivate->pInputBufferList->pBufHdr[0]);
            pComponentPrivate->nEmptyBufferDoneCount++;
        }
        if(pBufHeader->pMarkData)
        {
            /* copy mark to output buffer header */
            pComponentPrivate->pOutputBufferList->pBufHdr[0]->pMarkData = pBufHeader->pMarkData;
            pComponentPrivate->pOutputBufferList->pBufHdr[0]->hMarkTargetComponent = pBufHeader->hMarkTargetComponent;
            /* trigger event handler if we are supposed to */
            if(pBufHeader->hMarkTargetComponent == pComponentPrivate->pHandle && pBufHeader->pMarkData)
            {
                pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                                       OMX_EventMark, 0, 0,
                                                       pBufHeader->pMarkData);
            }
        }
        if (pComponentPrivate->bFlushInputPortCommandPending) {
            G729ENC_DPRINT("Sending command flush on INPUT:: %d\n", __LINE__);
            OMX_SendCommand(pComponentPrivate->pHandle,OMX_CommandFlush,0,NULL);
        }
    } 
    else if (eDir == OMX_DirOutput)
    {
        pComponentPrivate->nUnhandledFillThisBuffers--;
        /* Make sure that output buffer is issued to output stream only when
         * there is an outstanding input buffer already issued on input stream*/
        if (!(pComponentPrivate->bIsStopping))
        {
            if (pComponentPrivate->bBypassDSP == 0)
            {
                G729ENC_DPRINT ("Sending Empty OUTPUT BUFFER to Codec = %p\n", 
                                pBufHeader->pBuffer);
#ifdef __PERF_INSTRUMENTATION__
                PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                  PREF(pBufHeader,pBuffer), 
                                  pBufHeader->nFilledLen,
                                  PERF_ModuleCommonLayer);
#endif

                if (pComponentPrivate->curState == OMX_StateExecuting)
                {
                    if (!G729ENC_IsPending(pComponentPrivate,pBufHeader,OMX_DirOutput))
                    {
                        G729ENC_SetPending(pComponentPrivate,pBufHeader,OMX_DirOutput,__LINE__);
                        G729ENC_DPRINT("pLcmlHandle = %p\n",pLcmlHandle);
                        G729ENC_DPRINT("pLcmlHandle->pCodecinterfacehandle = %p\n",
                                       pLcmlHandle->pCodecinterfacehandle);
                        G729ENC_DPRINT("pBufHeader = %p\n",pBufHeader);
                        G729ENC_DPRINT("pBufHeader->pBuffer = %p\n",pBufHeader->pBuffer);
                        G729ENC_DPRINT("pBufHeader->nAllocLen = %d\n",pBufHeader->nAllocLen);

                        eError = LCML_QueueBuffer(pLcmlHandle->pCodecinterfacehandle,
                                                  EMMCodecOuputBuffer,
                                                  (OMX_U8 *)pBufHeader->pBuffer,
                                                  pBufHeader->nAllocLen,
                                                  pBufHeader->nAllocLen,
                                                  NULL, 0, NULL);
                                                  
                        G729ENC_DPRINT("After QueueBuffer Line %d\n",__LINE__);
                        if (eError != OMX_ErrorNone )
                        {
                            eError = OMX_ErrorHardware;
                            G729ENC_EPRINT("Issuing DSP OP: Error Occurred.\n");
                            goto EXIT;
                        }
                        pComponentPrivate->lcml_nOpBuf++;
                        pComponentPrivate->num_Op_Issued++;
                    }
                }
                else if(pComponentPrivate->curState == OMX_StatePause)
                {
                    pComponentPrivate->pOutputBufHdrPending[pComponentPrivate->nNumOutputBufPending++] = pBufHeader;
                }
            }
        }
        else
        {
            if (pComponentPrivate->curState == OMX_StateExecuting)
            {
                if (!G729ENC_IsPending(pComponentPrivate,pBufHeader,OMX_DirOutput))
                {
                    G729ENC_SetPending(pComponentPrivate,pBufHeader,OMX_DirOutput,__LINE__);
                    eError = LCML_QueueBuffer(pLcmlHandle->pCodecinterfacehandle,
                                              EMMCodecOuputBuffer,
                                              (OMX_U8 *)pBufHeader->pBuffer,
                                              pBufHeader->nAllocLen,
                                              pBufHeader->nFilledLen,
                                              NULL, 0, NULL);
                    if (eError != OMX_ErrorNone )
                    {
                        eError = OMX_ErrorHardware;
                        G729ENC_EPRINT("Issuing DSP OP: Error Occurred.\n");
                        goto EXIT;
                    }
                    pComponentPrivate->lcml_nOpBuf++;
                    pComponentPrivate->num_Op_Issued++;
                }
            }
            else if (pComponentPrivate->curState == OMX_StatePause)
            {
                pComponentPrivate->pOutputBufHdrPending[pComponentPrivate->nNumOutputBufPending++] = pBufHeader;
            }
        }
        if (pComponentPrivate->bFlushOutputPortCommandPending) 
        {
            G729ENC_DPRINT("Sending command flush on OUTPUT:: %d\n", __LINE__);
            OMX_SendCommand( pComponentPrivate->pHandle,
                             OMX_CommandFlush,
                             1,NULL);
        }

    }
    else
    {
        eError = OMX_ErrorBadParameter;
    }

 EXIT:
    G729ENC_DPRINT("Exiting. Returning error %d\n", eError);
    return eError;
}

/*-------------------------------------------------------------------*/
/**
 * G729ENC_GetBufferDirection () This function is used by the component
 * to get the direction of the buffer
 * @param eDir pointer will be updated with buffer direction
 * @param pBufHeader pointer to the buffer to be requested to be filled
 *
 * @retval none
 **/
/*-------------------------------------------------------------------*/

OMX_ERRORTYPE G729ENC_GetBufferDirection(OMX_BUFFERHEADERTYPE *pBufHeader,
                                         OMX_DIRTYPE *eDir)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    G729ENC_COMPONENT_PRIVATE *pComponentPrivate = pBufHeader->pPlatformPrivate;
    OMX_U32 nBuf = 0;
    OMX_BUFFERHEADERTYPE *pBuf = NULL;
    OMX_U32 flag = 1,i = 0;
    
    G729ENC_DPRINT("Entering\n");
    /*Search this buffer in input buffers list */
    nBuf = pComponentPrivate->pInputBufferList->numBuffers;
    for(i=0; i<nBuf; i++)
    {
        pBuf = pComponentPrivate->pInputBufferList->pBufHdr[i];
        if(pBufHeader == pBuf)
        {
            *eDir = OMX_DirInput;
            G729ENC_DPRINT("pBufHeader = %p is INPUT BUFFER pBuf = %p\n",
                           pBufHeader, pBuf);
            flag = 0;
            goto EXIT;
        }
    }
    /*Search this buffer in output buffers list */
    nBuf = pComponentPrivate->pOutputBufferList->numBuffers;
    for(i=0; i<nBuf; i++)
    {
        pBuf = pComponentPrivate->pOutputBufferList->pBufHdr[i];
        if(pBufHeader == pBuf)
        {
            *eDir = OMX_DirOutput;
            G729ENC_DPRINT("pBufHeader = %p is OUTPUT BUFFER pBuf = %p\n",
                           pBufHeader, pBuf);
            flag = 0;
            goto EXIT;
        }
    }

    if (flag == 1)
    {
        G729ENC_DPRINT("Buffer %p is Not Found in the List\n", pBufHeader);
        eError = OMX_ErrorUndefined;
        goto EXIT;
    }
 EXIT:
    G729ENC_DPRINT("Exiting. Returning = 0x%x\n", eError);
    return eError;
}

/* -------------------------------------------------------------------*/
/**
 * G729ENC_GetCorrespondingLCMLHeader() function will be called by LCML_Callback
 * component to write the msg
 * @param *pBuffer,          Event which gives to details about USN status
 * @param G729ENC_LCML_BUFHEADERTYPE **ppLcmlHdr
 * @param  OMX_DIRTYPE eDir this gives direction of the buffer
 * @retval OMX_NoError              Success, ready to roll
 *         OMX_Error_BadParameter   The input parameter pointer is null
 **/
/* -------------------------------------------------------------------*/
OMX_ERRORTYPE G729ENC_GetCorrespondingLCMLHeader(OMX_U8 *pBuffer,
                                                 OMX_DIRTYPE eDir,
                                                 G729ENC_LCML_BUFHEADERTYPE **ppLcmlHdr,
                                                 G729ENC_COMPONENT_PRIVATE *pComponentPrivate )
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    G729ENC_LCML_BUFHEADERTYPE *pLcmlBufHeader = NULL;
    OMX_U32 i = 0,nIpBuf = 0,nOpBuf = 0;
    nIpBuf = pComponentPrivate->pInputBufferList->numBuffers;
    nOpBuf = pComponentPrivate->pOutputBufferList->numBuffers;
    
    G729ENC_DPRINT("Entering\n");
    if(eDir == OMX_DirInput)
    {
        pLcmlBufHeader = pComponentPrivate->pLcmlBufHeader[G729ENC_INPUT_PORT];
        for(i = 0; i < nIpBuf; i++)
        {
            G729ENC_DPRINT("pLcmlBufHeader->buffer->pBuffer = %p\n",
                           pLcmlBufHeader->buffer->pBuffer);
            G729ENC_DPRINT("pBuffer = %p\n", pBuffer);
            G729ENC_DPRINT("pLcmlBufHeader->buffer->pBuffer = %p\n",
                           pLcmlBufHeader->buffer->pBuffer);
            if(pBuffer == pLcmlBufHeader->buffer->pBuffer)
            {
                *ppLcmlHdr = pLcmlBufHeader;
                G729ENC_DPRINT("Corresponding Input LCML Header Found = %p\n",
                               pLcmlBufHeader);
                eError = OMX_ErrorNone;
                goto EXIT;
            }
            pLcmlBufHeader++;
        }
    } else if (eDir == OMX_DirOutput)
    {
        pLcmlBufHeader = pComponentPrivate->pLcmlBufHeader[G729ENC_OUTPUT_PORT];
        for(i = 0; i < nOpBuf; i++)
        {
            G729ENC_DPRINT("pBuffer = %p\n", pBuffer);
            G729ENC_DPRINT("pLcmlBufHeader->buffer->pBuffer = %p\n",
                           pLcmlBufHeader->buffer->pBuffer);
            if(pBuffer == pLcmlBufHeader->buffer->pBuffer)
            {
                *ppLcmlHdr = pLcmlBufHeader;
                G729ENC_DPRINT("Corresponding Output LCML Header Found = %p\n",
                               pLcmlBufHeader);
                eError = OMX_ErrorNone;
                goto EXIT;
            }
            pLcmlBufHeader++;
        }
    }
    else
    {
        eError = OMX_ErrorUndefined;
        G729ENC_EPRINT("Invalid Buffer Type.\n");
    }

 EXIT:
    G729ENC_DPRINT("Exiting. Returning = 0x%x\n", eError);
    return eError;
}

/* -------------------------------------------------------------------*/
/**
 *  G729ENC_LCMLCallback() will be called LCML component to write the msg
 *
 * @param event                 Event which gives to details about USN status
 * @param void * args        //    args [0] //bufType;
 //    args [1] //arm address fpr buffer
 //    args [2] //BufferSize;
 //    args [3]  //arm address for param
 //    args [4] //ParamSize;
 //    args [6] //LCML Handle
 * @retval OMX_NoError              Success, ready to roll
 *         OMX_Error_BadParameter   The input parameter pointer is null
 **/
/*-------------------------------------------------------------------*/

OMX_ERRORTYPE G729ENC_LCMLCallback (TUsnCodecEvent event,void * args[10])
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U8 *pBuffer = args[1];
    G729ENC_LCML_BUFHEADERTYPE *pLcmlHdr = NULL;
    OMX_S16 i = 0;
    OMX_S16 numFrames = 0;
    OMX_U32 checkBeforeFilling = 0;
    OMX_U32 inputBufferSize =0, frameLength =0;
    G729ENC_COMPONENT_PRIVATE* pComponentPrivate = NULL;
    OMX_COMPONENTTYPE *pHandle = NULL;
    LCML_DSP_INTERFACE *pLcmlHandle = NULL;
#ifdef RESOURCE_MANAGER_ENABLED
    OMX_ERRORTYPE rm_error = OMX_ErrorNone;
#endif                                               

    pComponentPrivate = pComponentPrivate_CC;
    pHandle = (OMX_COMPONENTTYPE*)pComponentPrivate->pHandle;  
    pLcmlHandle = (LCML_DSP_INTERFACE *) pComponentPrivate->pLcmlHandle;    
    G729ENC_DPRINT("Entering G729ENC_LCMLCallback\n");

    switch(event)
    {
    case EMMCodecDspError:
        G729ENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecDspError\n");
        break;
    case EMMCodecInternalError:
        G729ENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecInternalError\n");
        break;
    case EMMCodecInitError:
        G729ENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecInitError\n");
        break;
    case EMMCodecDspMessageRecieved:
        G729ENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecDspMessageRecieved\n");
        break;
    case EMMCodecBufferProcessed:
        G729ENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecBufferProcessed\n");
        break;
    case EMMCodecProcessingStarted:
        G729ENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecProcessingStarted\n");
        break;
    case EMMCodecProcessingPaused:
        G729ENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecProcessingPaused\n");
        break;
    case EMMCodecProcessingStoped:
        G729ENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecProcessingStoped\n");
        break;
    case EMMCodecProcessingEof:
        G729ENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecProcessingEof\n");
        break;
    case EMMCodecBufferNotProcessed:
        G729ENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecBufferNotProcessed\n");
        break;
    case EMMCodecAlgCtrlAck:
        G729ENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecAlgCtrlAck\n");
        break;
    case EMMCodecStrmCtrlAck:
        G729ENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecStrmCtrlAck\n");
        break;
    }
    
    if(event == EMMCodecBufferProcessed)
    {
        if((OMX_U32)args[0] == EMMCodecInputBuffer)
        {
            G729ENC_DPRINT("INPUT: pBuffer = %p\n", pBuffer);
            eError = G729ENC_GetCorrespondingLCMLHeader(pBuffer, OMX_DirInput,
                                                        &pLcmlHdr, pComponentPrivate);
            if (eError != OMX_ErrorNone)
            {
                G729ENC_EPRINT("Invalid Buffer Came.\n");
                goto EXIT;
            }
#ifdef __PERF_INSTRUMENTATION__
            PERF_ReceivedFrame(pComponentPrivate->pPERFcomp,
                               PREF(pLcmlHdr->buffer, pBuffer), 0,
                               PERF_ModuleCommonLayer);
#endif
            G729ENC_ClearPending(pComponentPrivate, pLcmlHdr->buffer,
                                 OMX_DirInput, __LINE__);
            /*******************************************************************/
            inputBufferSize = G729ENC_INPUT_FRAME_SIZE;
            if(pComponentPrivate->pInputBufferList->numBuffers == 1) {
                checkBeforeFilling = inputBufferSize;
            } 
            else
            {
                checkBeforeFilling = inputBufferSize * (pComponentPrivate->pInputBufferList->numBuffers - 1);
            }
            G729ENC_DPRINT("pComponentPrivate->nHoldLength = %d\n",pComponentPrivate->nHoldLength);
            if(pComponentPrivate->nHoldLength < checkBeforeFilling)
            {
                G729ENC_DPRINT("pComponentPrivate->nHoldLength = %ld\n", pComponentPrivate->nHoldLength);
                G729ENC_DPRINT("checkBeforeFilling = %ld\n", checkBeforeFilling);
                if (pComponentPrivate->curState != OMX_StatePause)
                {
                    G729ENC_DPRINT("Calling EmptyBufferDone\n");
#ifdef __PERF_INSTRUMENTATION__
                    PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                      pLcmlHdr->buffer->pBuffer, 0,
                                      PERF_ModuleHLMM);
#endif
                    pComponentPrivate->cbInfo.EmptyBufferDone(pHandle,
                                                              pHandle->pApplicationPrivate,
                                                              pLcmlHdr->buffer);
                    pComponentPrivate->nEmptyBufferDoneCount++;
                    pComponentPrivate->lcml_nIpBuf--;
                    pComponentPrivate->app_nBuf++;
                } 
                else
                {
                    pComponentPrivate->pInputBufHdrPending[pComponentPrivate->nNumInputBufPending++] = pLcmlHdr->buffer;
                }
            } 
            else
            {              
                /*
                 * We possibly have enough data in pHoldBuffer.
                 * If we don't have enough data to be send, then we need to refill when last buffer is not set
                 * even though FillThisHwBuffer has already been sent.
                 * Send QueueBuffer from pHoldBuffer then reflushed the pHoldBuffer.
                 */
                if(pComponentPrivate->nHoldLength > 0)
                {
                    frameLength = G729ENC_INPUT_FRAME_SIZE;
                    if (pComponentPrivate->nHoldLength >= frameLength)
                    {
                        /* Copy the data from pHoldBuffer to dataPtr */
                        memcpy(pLcmlHdr->buffer->pBuffer, pComponentPrivate->pHoldBuffer, frameLength);
                        /* Remove the copied data from pHoldBuffer. */

                        /*OMAPS00101094*/
			if (pComponentPrivate->nHoldLength - frameLength < frameLength) {
                            memcpy(pComponentPrivate->pHoldBuffer, 
                                   pComponentPrivate->pHoldBuffer + frameLength,
                                   pComponentPrivate->nHoldLength - frameLength);
			}
			else {
                            memmove(pComponentPrivate->pHoldBuffer, 
                                   pComponentPrivate->pHoldBuffer + frameLength,
                                   pComponentPrivate->nHoldLength - frameLength);
			}
			
                        /*OMAPS00101094*/
                        pComponentPrivate->nHoldLength = pComponentPrivate->nHoldLength - frameLength;
                        G729ENC_DPRINT("pComponentPrivate->nHoldLength = %d\n",
                                       pComponentPrivate->nHoldLength);
                        eError = G729ENC_GetCorrespondingLCMLHeader(pLcmlHdr->buffer->pBuffer,
                                                                    OMX_DirInput,
                                                                    &pLcmlHdr,
                                                                    pComponentPrivate);
                        for (i=0; i < inputBufferSize; i++)
                        {
                            G729ENC_DPRINT("Queueing pLcmlHdr->buffer->pBuffer[%d] = %x\n",
                                           i , pLcmlHdr->buffer->pBuffer[i]);
                        }
                        G729ENC_SetPending(pComponentPrivate, pLcmlHdr->buffer,
                                           OMX_DirInput, __LINE__);
                        eError = LCML_QueueBuffer(pLcmlHandle->pCodecinterfacehandle,
                                                  EMMCodecInputBuffer,
                                                  (OMX_U8 *)pLcmlHdr->buffer->pBuffer,
                                                  frameLength, frameLength,
                                                  (OMX_U8 *) pLcmlHdr->pIpParam,
                                                  sizeof(G729ENC_UAlgInBufParamStruct),
                                                  NULL);
                    } 
                    else
                    {
                        /*We need to refill more since pHoldBuffer only has partial data.*/
                        if (pComponentPrivate->curState != OMX_StatePause)
                        {
                            G729ENC_DPRINT("Calling EmptyBufferDone\n");
#ifdef __PERF_INSTRUMENTATION__
                            PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                              pLcmlHdr->buffer->pBuffer, 0,
                                              PERF_ModuleHLMM);
#endif
                            pComponentPrivate->cbInfo.EmptyBufferDone(pHandle,
                                                                      pHandle->pApplicationPrivate,
                                                                      pLcmlHdr->buffer);
                            pComponentPrivate->nEmptyBufferDoneCount++;
                        } 
                        else
                        {
                            pComponentPrivate->pInputBufHdrPending[pComponentPrivate->nNumInputBufPending++] = pLcmlHdr->buffer;
                        }
                    }
                }
            }
            /*******************************************************************/
        } 
        else if((OMX_U32)args[0] == EMMCodecOuputBuffer)
        {
            G729ENC_DPRINT("OUTPUT: pBuffer = %p\n", pBuffer);
            eError = G729ENC_GetCorrespondingLCMLHeader(pBuffer, OMX_DirOutput,
                                                        &pLcmlHdr, pComponentPrivate);
            if (eError != OMX_ErrorNone)
            {
                G729ENC_EPRINT("Invalid Buffer Came.\n");
                goto EXIT;
            }
            G729ENC_DPRINT("Output: pLcmlHdr->buffer->pBuffer = %p\n",
                           pLcmlHdr->buffer->pBuffer);            
            /* The G729 Encoder returns a length in bits rather than bytes. 
               Therefore the value given from the socket node needs to be divided by 8 */
            numFrames = (OMX_U32)args[8] >> 24;
            pLcmlHdr->buffer->nFilledLen = 0;
            
#ifdef __PERF_INSTRUMENTATION__
            PERF_ReceivedFrame(pComponentPrivate->pPERFcomp,
                               PREF(pLcmlHdr->buffer,pBuffer),
                               PREF(pLcmlHdr->buffer,nFilledLen),
                               PERF_ModuleCommonLayer);

            pComponentPrivate->nLcml_nCntOpReceived++;
            if ((pComponentPrivate->nLcml_nCntIp >= 1) &&
                (pComponentPrivate->nLcml_nCntOpReceived == 1))
            {
                PERF_Boundary(pComponentPrivate->pPERFcomp,
                              PERF_BoundaryStart | PERF_BoundarySteadyState);
            }
#endif


            for (i=0; i < numFrames; i++)
            {
                pLcmlHdr->buffer->nFilledLen = pLcmlHdr->buffer->nFilledLen + (((OMX_U32)args[8] >> (20 - i*4)) & 0xf)  ;                        
            }
            pComponentPrivate->lcml_nCntOpReceived++;
            G729ENC_ClearPending(pComponentPrivate, pLcmlHdr->buffer,
                                 OMX_DirOutput, __LINE__);
            /*******************************************************************/
            pComponentPrivate->num_Reclaimed_Op_Buff++;
            if (pComponentPrivate->bIsEOFSent)
            {
                pLcmlHdr->buffer->nFlags |= OMX_BUFFERFLAG_EOS;
                pComponentPrivate->bIsEOFSent = 0;
            }
            frameLength = pLcmlHdr->buffer->nFilledLen;
            /* Multi Frame Mode has been tested here */
#ifdef __PERF_INSTRUMENTATION__
            PERF_SendingBuffer(pComponentPrivate->pPERFcomp,
                               pLcmlHdr->buffer->pBuffer,
                               pLcmlHdr->buffer->nFilledLen,
                               PERF_ModuleHLMM);
#endif

            /* Copying time stamp information to output buffer */
            pLcmlHdr->buffer->nTimeStamp = pComponentPrivate->arrTimestamp[pComponentPrivate->OpBufindex];
            pLcmlHdr->buffer->nTickCount = pComponentPrivate->arrTickCount[pComponentPrivate->OpBufindex];                    
            pComponentPrivate->OpBufindex++;
            pComponentPrivate->OpBufindex %= pComponentPrivate->pPortDef[G729ENC_OUTPUT_PORT]->nBufferCountActual;
        
            pComponentPrivate->cbInfo.FillBufferDone(pHandle,
                                                     pHandle->pApplicationPrivate,
                                                     pLcmlHdr->buffer);
            pComponentPrivate->lcml_nOpBuf--;
            pComponentPrivate->app_nBuf++;
            pComponentPrivate->nFillBufferDoneCount++;
            G729ENC_DPRINT("Incrementing app_nBuf = %ld\n", pComponentPrivate->app_nBuf);
            pLcmlHdr->buffer->nFlags = 0;             
            /*******************************************************************/                        
        }
    }
    else if (event == EMMCodecStrmCtrlAck)
    {
        G729ENC_DPRINT("GOT MESSAGE USN_DSPACK_STRMCTRL \n");
        if (args[1] == (void *)USN_STRMCMD_FLUSH)
        {
            G729ENC_DPRINT("revceived USN_STRMCMD_FLUSH\n");
            pHandle = pComponentPrivate->pHandle;
            if ( args[2] == (void *)EMMCodecInputBuffer)
            {
                if (args[0] == USN_ERR_NONE ) {
                    G729ENC_DPRINT("Flushing input port %d\n", __LINE__);
                    for (i=0; i < G729ENC_MAX_NUM_OF_BUFS; i++)
                    {
                        pComponentPrivate->pInputBufHdrPending[i] = NULL;
                    }
                    pComponentPrivate->nNumInputBufPending=0;
                    for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++)
                    {
#ifdef __PERF_INSTRUMENTATION__
                        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                          pComponentPrivate->pInputBufferList->pBufHdr[i]->pBuffer,
                                          0,
                                          PERF_ModuleHLMM);
#endif                        
                        pComponentPrivate->cbInfo.EmptyBufferDone (
                                                                   pComponentPrivate->pHandle,
                                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                                   pComponentPrivate->pInputBufferList->pBufHdr[i]
                                                                   );
                    }
                    pComponentPrivate->cbInfo.EventHandler(pHandle, pHandle->pApplicationPrivate,
                                                           OMX_EventCmdComplete,
                                                           OMX_CommandFlush,
                                                           G729ENC_INPUT_PORT,
                                                           NULL);
                }
                else 
                {
                    G729ENC_EPRINT("error flushing input port.\n");
                    goto EXIT;                            
                }
            }
            else if ( args[2] == (void *)EMMCodecOuputBuffer)
            {
                if (args[0] == USN_ERR_NONE )
                {
                    G729ENC_DPRINT("Flushing output port %d\n", __LINE__);
                    for (i=0; i < G729ENC_MAX_NUM_OF_BUFS; i++)
                    {
                        pComponentPrivate->pOutputBufHdrPending[i] = NULL;
                    }
                    pComponentPrivate->nNumOutputBufPending=0;
                    for (i=0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++)
                    {
#ifdef __PERF_INSTRUMENTATION__
                        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                          pComponentPrivate->pOutputBufferList->pBufHdr[i]->pBuffer,
                                          pComponentPrivate->pOutputBufferList->pBufHdr[i]->nFilledLen,
                                          PERF_ModuleHLMM);
#endif
                        pComponentPrivate->cbInfo.FillBufferDone (
                                                                  pComponentPrivate->pHandle,
                                                                  pComponentPrivate->pHandle->pApplicationPrivate,
                                                                  pComponentPrivate->pOutputBufferList->pBufHdr[i]
                                                                  );
                    }
                    pComponentPrivate->cbInfo.EventHandler(
                                                           pHandle, pHandle->pApplicationPrivate,
                                                           OMX_EventCmdComplete, OMX_CommandFlush,G729ENC_OUTPUT_PORT, NULL);
                }
                else
                {
                    G729ENC_DPRINT("error flushing output port.\n");
                    goto EXIT;                            
                }
            }
        }
    }
    else if(event == EMMCodecProcessingStoped)
    {
        G729ENC_DPRINT("GOT MESSAGE USN_DSPACK_STOP\n");
        if((pComponentPrivate->nMultiFrameMode == 1) &&
           (pComponentPrivate->mimeMode == 1))
        {
            /*Sending Last Buufer Data which on iHoldBuffer to App */
            G729ENC_DPRINT("Sending iMMFDataLastBuffer Data which on iHoldBuffer to App\n");
            pComponentPrivate->iMMFDataLastBuffer = malloc(G729ENC_OUTPUT_BUFFER_SIZE_MIME * (pComponentPrivate->pOutputBufferList->numBuffers + 1));
            G729ENC_MEMPRINT("%d :: [ALLOC]  = %p\n",__LINE__, pComponentPrivate->iMMFDataLastBuffer);
            if(pComponentPrivate->iMMFDataLastBuffer == NULL)
            {
                eError = OMX_ErrorInsufficientResources;
                G729ENC_EPRINT("Malloc Failed.\n");
                goto EXIT;
            }
            G729ENC_DPRINT("pComponentPrivate->iHoldLen = %ld \n",
                           pComponentPrivate->iHoldLen);
            /* Copy the data from iHoldBuffer to dataPtr */
            memcpy(pComponentPrivate->iMMFDataLastBuffer,
                   pComponentPrivate->iHoldBuffer,
                   pComponentPrivate->iHoldLen);
            pComponentPrivate->iMMFDataLastBuffer->nFilledLen = pComponentPrivate->iHoldLen;
            G729ENC_DPRINT("pComponentPrivate->iMMFDataLastBuffer->nFilledLen = %ld \n",
                           pComponentPrivate->iMMFDataLastBuffer->nFilledLen);
            /* Remove the copied data to dataPtr from iHoldBuffer. */
            /*memset(pComponentPrivate->iHoldBuffer, 0, pComponentPrivate->iHoldLen);*/
            pComponentPrivate->iHoldLen = 0;
#ifdef __PERF_INSTRUMENTATION__
            PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                              pComponentPrivate->iMMFDataLastBuffer->pBuffer,
                              pComponentPrivate->iMMFDataLastBuffer->nFilledLen,
                              PERF_ModuleHLMM);
#endif
            pComponentPrivate->cbInfo.FillBufferDone(pComponentPrivate->pHandle,
                                                     pComponentPrivate->pHandle->pApplicationPrivate,
                                                     pComponentPrivate->iMMFDataLastBuffer);
            pComponentPrivate->lcml_nOpBuf--;
            pComponentPrivate->app_nBuf++;
            pComponentPrivate->nFillBufferDoneCount++;
            G729ENC_DPRINT("Incrementing app_nBuf = %ld\n",
                           pComponentPrivate->app_nBuf);
        }
        if (!pComponentPrivate->bNoIdleOnStop)
        {
            
#ifdef RESOURCE_MANAGER_ENABLED                                                
            rm_error = RMProxy_NewSendCommand(pComponentPrivate->pHandle, RMProxy_StateSet, OMX_G729_Encoder_COMPONENT, OMX_StateIdle, 3456,NULL);                                       
#endif

            if(pComponentPrivate->bPreempted == 0){
                pComponentPrivate->curState = OMX_StateIdle;
                pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandStateSet,
                                                       pComponentPrivate->curState,
                                                       NULL);
            }
            else{
                pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                                       OMX_EventError,
                                                       OMX_ErrorResourcesPreempted,
                                                       0,
                                                       NULL);

            }
            
        }
        else
        {
            pComponentPrivate->bDspStoppedWhileExecuting = OMX_TRUE;
            pComponentPrivate->bNoIdleOnStop= OMX_FALSE;
        }

    }
    else if(event == EMMCodecDspMessageRecieved)
    {
        G729ENC_DPRINT("commandedState  = %ld\n", (OMX_U32)args[0]);
        G729ENC_DPRINT("arg1 = %ld\n", (OMX_U32)args[1]);
        G729ENC_DPRINT("arg2 = %ld\n", (OMX_U32)args[2]);
        if(0x0500 == (OMX_U32)args[2])
        {
            G729ENC_DPRINT("EMMCodecDspMessageRecieved\n");
        }
    }
    else if(event == EMMCodecAlgCtrlAck)
    {
        G729ENC_DPRINT("GOT MESSAGE USN_DSPACK_ALGCTRL \n");
    }
    else if (event == EMMCodecProcessingPaused) {
        pComponentPrivate->curState = OMX_StatePause;
                    
        pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                               pComponentPrivate->pHandle->pApplicationPrivate,
                                               OMX_EventCmdComplete,
                                               OMX_CommandStateSet,
                                               pComponentPrivate->curState,
                                               NULL);
    
    }
    else if (event == EMMCodecDspError)
    {
        if(((int)args[4] == USN_ERR_WARNING) && ((int)args[5] == IUALG_WARN_PLAYCOMPLETED))
        {
            G729ENC_DPRINT("GOT MESSAGE IUALG_WARN_PLAYCOMPLETED\n");
            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                   OMX_EventBufferFlag,
                                                   (OMX_U32)NULL,
                                                   OMX_BUFFERFLAG_EOS,
                                                   NULL);
            pComponentPrivate->bPlayCompleteFlag = 1;
        }
    }
 EXIT:
    G729ENC_DPRINT("Exiting. Returning = 0x%x\n", eError);
    return eError;
}

/* ================================================================================= */
/**
 *  G729ENC_GetLCMLHandle()
 *
 * @retval OMX_HANDLETYPE
 */
/* ================================================================================= */
#ifndef UNDER_CE
OMX_HANDLETYPE G729ENC_GetLCMLHandle(G729ENC_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_ERRORTYPE (*fpGetHandle)(OMX_HANDLETYPE);
    OMX_HANDLETYPE pHandle = NULL;
    void *handle = NULL;
    char *error = NULL;
    
    G729ENC_DPRINT("Entering\n");
    handle = dlopen("libLCML.so", RTLD_LAZY);
    if (!handle)
    {
        fputs(dlerror(), stderr);
        goto EXIT;
    }
    fpGetHandle = dlsym (handle, "GetHandle");
    if ((error = dlerror()) != NULL)
    {
        fputs(error, stderr);
        goto EXIT;
    }
    eError = (*fpGetHandle)(&pHandle);
    if(eError != OMX_ErrorNone)
    {
        eError = OMX_ErrorUndefined;
        G729ENC_EPRINT("OMX_ErrorUndefined.\n");
        pHandle = NULL;
        goto EXIT;
    }
 EXIT:
    G729ENC_DPRINT("Exiting.");
    return pHandle;
}

#else
/*WINDOWS Explicit dll load procedure*/
OMX_HANDLETYPE G729ENC_GetLCMLHandle(G729ENC_COMPONENT_PRIVATE *pComponentPrivate)
{
    typedef OMX_ERRORTYPE (*LPFNDLLFUNC1)(OMX_HANDLETYPE);
    OMX_HANDLETYPE pHandle = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    LPFNDLLFUNC1 fpGetHandle1;
        
    g_hLcmlDllHandle = LoadLibraryEx(TEXT("OAF_BML.dll"), NULL,0);
    if (g_hLcmlDllHandle == NULL) {
        //fputs(dlerror(), stderr);
        G729ENC_DPRINT("BML Load Failed!!!\n");
        return pHandle;
    }
    fpGetHandle1 = (LPFNDLLFUNC1)GetProcAddress(g_hLcmlDllHandle,TEXT("GetHandle"));
    if (!fpGetHandle1) 
    {
        // handle the error
        FreeLibrary(g_hLcmlDllHandle);
        g_hLcmlDllHandle = NULL;
        return pHandle;
    }
    // call the function
    eError = fpGetHandle1(&pHandle);
    if(eError != OMX_ErrorNone) 
    {
        eError = OMX_ErrorUndefined;
        G729ENC_DPRINT("eError != OMX_ErrorNone...\n");
        FreeLibrary(g_hLcmlDllHandle);
        g_hLcmlDllHandle = NULL;
        pHandle = NULL;
        return pHandle;
    }
    return pHandle;
}
#endif

/* ================================================================================= */
/**
 * @fn G729ENC_SetPending() description for G729ENC_SetPending
 G729ENC_SetPending().
 This component is called when a buffer is queued to the LCML
 * @param pComponent  handle for this instance of the component
 *
 * @pre
 *
 * @post
 *
 * @return OMX_ERRORTYPE
 */
/* ================================================================================ */
void G729ENC_SetPending(G729ENC_COMPONENT_PRIVATE *pComponentPrivate,
                        OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir,
                        OMX_U32 lineNumber)
{
    OMX_U32 i = 0;

    if (eDir == OMX_DirInput)
    {
        for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++)
        {
            if (pBufHdr == pComponentPrivate->pInputBufferList->pBufHdr[i])
            {
                pComponentPrivate->pInputBufferList->bBufferPending[i] = 1;
                G729ENC_DPRINT("****INPUT BUFFER %d IS PENDING Line %ld******\n",
                               i, lineNumber);
            }
        }
    }
    else
    {
        for (i=0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++)
        {
            if (pBufHdr == pComponentPrivate->pOutputBufferList->pBufHdr[i])
            {
                pComponentPrivate->pOutputBufferList->bBufferPending[i] = 1;
                G729ENC_DPRINT("****OUTPUT BUFFER %d IS PENDING Line %ld*****\n",
                               i, lineNumber);
            }
        }
    }
}
/* ================================================================================= */
/**
 * @fn G729ENC_ClearPending() description for G729ENC_ClearPending
 G729ENC_ClearPending().
 This component is called when a buffer is returned from the LCML
 * @param pComponent  handle for this instance of the component
 *
 * @pre
 *
 * @post
 *
 * @return OMX_ERRORTYPE
 */
/* ================================================================================ */
void G729ENC_ClearPending(G729ENC_COMPONENT_PRIVATE *pComponentPrivate,
                          OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir,
                          OMX_U32 lineNumber)
{
    OMX_U32 i = 0;

    if (eDir == OMX_DirInput)
    {
        for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++)
        {
            if (pBufHdr == pComponentPrivate->pInputBufferList->pBufHdr[i])
            {
                pComponentPrivate->pInputBufferList->bBufferPending[i] = 0;
                G729ENC_DPRINT("****INPUT BUFFER %d IS RECLAIMED Line %ld*****\n",
                               i, lineNumber);
            }
        }
    }
    else
    {
        for (i=0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++)
        {
            if (pBufHdr == pComponentPrivate->pOutputBufferList->pBufHdr[i])
            {
                pComponentPrivate->pOutputBufferList->bBufferPending[i] = 0;
                G729ENC_DPRINT("****OUTPUT BUFFER %d IS RECLAIMED Line %ld*****\n",
                               i, lineNumber);
            }
        }
    }
}
/* ================================================================================= */
/**
 * @fn G729ENC_IsPending() description for G729ENC_IsPending
 G729ENC_IsPending().
 This method returns the pending status to the buffer
 * @param pComponent  handle for this instance of the component
 *
 * @pre
 *
 * @post
 *
 * @return OMX_ERRORTYPE
 */
/* ================================================================================ */
OMX_U32 G729ENC_IsPending(G729ENC_COMPONENT_PRIVATE *pComponentPrivate,
                          OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir)
{
    OMX_U32 i = 0;

    if (eDir == OMX_DirInput)
    {
        for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++)
        {
            if (pBufHdr == pComponentPrivate->pInputBufferList->pBufHdr[i])
            {
                return pComponentPrivate->pInputBufferList->bBufferPending[i];
            }
        }
    }
    else
    {
        for (i=0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++)
        {
            G729ENC_DPRINT("pBufHdr = %p\n",pBufHdr);
            G729ENC_DPRINT("pOutputBufferList->pBufHdr[i] = %p\n",
                           pComponentPrivate->pOutputBufferList->pBufHdr[i]);
            if (pBufHdr == pComponentPrivate->pOutputBufferList->pBufHdr[i])
            {
                G729ENC_DPRINT("returning %d\n",
                               pComponentPrivate->pOutputBufferList->bBufferPending[i]);
                return pComponentPrivate->pOutputBufferList->bBufferPending[i];
            }
        }
    }
    return -1;
}
/* ================================================================================= */
/**
 * @fn G729ENC_IsValid() description for G729ENC_IsValid
 G729ENC_IsValid().
 This method checks to see if a buffer returned from the LCML is valid.
 * @param pComponent  handle for this instance of the component
 *
 * @pre
 *
 * @post
 *
 * @return OMX_ERRORTYPE
 */
/* ================================================================================ */
OMX_U32 G729ENC_IsValid(G729ENC_COMPONENT_PRIVATE *pComponentPrivate,
                        OMX_U8 *pBuffer, OMX_DIRTYPE eDir)
{
    OMX_U32 i = 0;
    int found=0;

    if (eDir == OMX_DirInput)
    {
        for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++)
        {
            if (pBuffer == pComponentPrivate->pInputBufferList->pBufHdr[i]->pBuffer)
            {
                found = 1;
            }
        }
    }
    else
    {
        for (i=0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++)
        {
            if (pBuffer == pComponentPrivate->pOutputBufferList->pBufHdr[i]->pBuffer)
            {
                found = 1;
            }
        }
    }
    return found;
}
/* ========================================================================== */
/**
 * @G729ENC_FillLCMLInitParamsEx() This function is used by the component thread to
 * fill the all of its initialization parameters, buffer deatils  etc
 * to LCML structure,
 *
 * @param pComponent  handle for this instance of the component
 * @param plcml_Init  pointer to LCML structure to be filled
 *
 * @pre
 *
 * @post
 *
 * @return none
 */
/* ========================================================================== */
OMX_ERRORTYPE G729ENC_FillLCMLInitParamsEx(OMX_HANDLETYPE pComponent)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nIpBuf = 0,nIpBufSize = 0,nOpBuf = 0,nOpBufSize = 0;
    OMX_BUFFERHEADERTYPE *pTemp = NULL;
    LCML_DSP_INTERFACE *pHandle = (LCML_DSP_INTERFACE *)pComponent;
    G729ENC_COMPONENT_PRIVATE *pComponentPrivate = pHandle->pComponentPrivate;
    G729ENC_LCML_BUFHEADERTYPE *pTemp_lcml = NULL;
    OMX_U32 i = 0;
    OMX_U32 size_lcml = 0;
    
    G729ENC_DPRINT("G729ENC_FillLCMLInitParamsEx\n");
    nIpBuf = pComponentPrivate->pInputBufferList->numBuffers;
    nIpBufSize = pComponentPrivate->pPortDef[G729ENC_INPUT_PORT]->nBufferSize;
    nOpBuf = pComponentPrivate->pOutputBufferList->numBuffers;
    nOpBufSize = pComponentPrivate->pPortDef[G729ENC_OUTPUT_PORT]->nBufferSize;
    G729ENC_DPRINT("------ Buffer Details -----------\n");
    G729ENC_DPRINT("Input  Buffer Count = %ld\n", nIpBuf);
    G729ENC_DPRINT("Input  Buffer Size = %ld\n", nIpBufSize);
    G729ENC_DPRINT("Output Buffer Count = %ld\n", nOpBuf);
    G729ENC_DPRINT("Output Buffer Size = %ld\n", nOpBufSize);
    G729ENC_DPRINT("------ Buffer Details ------------\n");
    /* Allocate memory for all input buffer headers This memory pointer will be sent to LCML */
    size_lcml = nIpBuf * sizeof(G729ENC_LCML_BUFHEADERTYPE);
    pTemp_lcml = (G729ENC_LCML_BUFHEADERTYPE *)malloc(size_lcml);
    G729ENC_MEMPRINT("%d :: [ALLOC] %p\n",__LINE__,pTemp_lcml);
    if(pTemp_lcml == NULL)
    {
        G729ENC_EPRINT("Memory Allocation Failed.\n");
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    memset(pTemp_lcml, 0x0, size_lcml);
    pComponentPrivate->pLcmlBufHeader[G729ENC_INPUT_PORT] = pTemp_lcml;
    for (i=0; i<nIpBuf; i++)
    {
        G729ENC_DPRINT("INPUT--------- Inside Ip Loop\n");
        pTemp = pComponentPrivate->pInputBufferList->pBufHdr[i];
        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pTemp->nAllocLen = nIpBufSize;
        pTemp->nFilledLen = nIpBufSize;
        pTemp->nVersion.s.nVersionMajor = G729ENC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = G729ENC_MINOR_VER;
        pTemp->nVersion.s.nRevision = 0;
        pTemp->nVersion.s.nStep = 0;
        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = G729ENC_NOT_USED;
        pTemp_lcml->buffer = pTemp;
        G729ENC_DPRINT("pTemp_lcml->buffer->pBuffer = %p \n", pTemp_lcml->buffer->pBuffer);
        pTemp_lcml->eDir = OMX_DirInput;
        OMX_G729MALLOC_STRUCT(pTemp_lcml->pIpParam, G729ENC_UAlgInBufParamStruct);
        pTemp_lcml->pIpParam->usEndOfFile = 0;
        /* This means, it is not a last buffer. This flag is to be modified by  the application to indicate the last buffer */
        pTemp->nFlags = G729ENC_NORMAL_BUFFER;
        pTemp++;
        pTemp_lcml++;
    }

    /* Allocate memory for all output buffer headers This memory pointer will be sent to LCML */
    size_lcml = nOpBuf * sizeof(G729ENC_LCML_BUFHEADERTYPE);
    pTemp_lcml = (G729ENC_LCML_BUFHEADERTYPE *)malloc(size_lcml);
    G729ENC_MEMPRINT("%d :: [ALLOC] %p\n",__LINE__,pTemp_lcml);
    if(pTemp_lcml == NULL)
    {
        eError = OMX_ErrorInsufficientResources;
        G729ENC_EPRINT("Memory Allocation Failed.\n");
        goto EXIT;
    }
    memset(pTemp_lcml, 0x0, size_lcml);
    pComponentPrivate->pLcmlBufHeader[G729ENC_OUTPUT_PORT] = pTemp_lcml;
    for (i=0; i<nOpBuf; i++)
    {
        G729ENC_DPRINT("OUTPUT--------- Inside Op Loop\n");
        pTemp = pComponentPrivate->pOutputBufferList->pBufHdr[i];
        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pTemp->nAllocLen = nOpBufSize;
        pTemp->nFilledLen = nOpBufSize;
        pTemp->nVersion.s.nVersionMajor = G729ENC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = G729ENC_MINOR_VER;
        pTemp->nVersion.s.nRevision = 0;
        pTemp->nVersion.s.nStep = 0;
        pComponentPrivate->nVersion = pTemp->nVersion.nVersion;
        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = G729ENC_NOT_USED;
        pTemp_lcml->buffer = pTemp;
        G729ENC_DPRINT("pTemp_lcml->buffer->pBuffer = %p \n", pTemp_lcml->buffer->pBuffer);
        pTemp_lcml->eDir = OMX_DirOutput;
        OMX_G729MALLOC_STRUCT(pTemp_lcml->pOpParam, G729ENC_UAlgOutBufParamStruct);
        pTemp_lcml->pOpParam->ulFrameCount = 0;
        /* This means, it is not a last buffer. This flag is to be modified by the application to indicate the last buffer */
        pTemp->nFlags = G729ENC_NORMAL_BUFFER;
        pTemp++;
        pTemp_lcml++;
    }
    pComponentPrivate->bPortDefsAllocated = 1;
    pComponentPrivate->bInitParamsInitialized = 1;
 EXIT:
    G729ENC_DPRINT("Exiting G729ENC_FillLCMLInitParamsEx\n",__LINE__);
    G729ENC_DPRINT("Returning = 0x%x\n",eError);
    return eError;
}

#ifdef RESOURCE_MANAGER_ENABLED
/***********************************
 *  Callback to the RM                                       *
 ***********************************/
void G729ENC_ResourceManagerCallback(RMPROXY_COMMANDDATATYPE cbData)
{
    OMX_COMMANDTYPE Cmd = OMX_CommandStateSet;
    OMX_STATETYPE state = OMX_StateIdle;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)cbData.hComponent;
    G729ENC_COMPONENT_PRIVATE *pCompPrivate = NULL;

    pCompPrivate = (G729ENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    if (*(cbData.RM_Error) == OMX_RmProxyCallback_ResourcesPreempted){
        if (pCompPrivate->curState == OMX_StateExecuting || 
            pCompPrivate->curState == OMX_StatePause) {

            write (pCompPrivate->cmdPipe[1], &Cmd, sizeof(Cmd));
            write (pCompPrivate->cmdDataPipe[1], &state ,sizeof(OMX_U32));

            pCompPrivate->bPreempted = 1;
        }
    }
    else if (*(cbData.RM_Error) == OMX_RmProxyCallback_ResourcesAcquired){
        pCompPrivate->cbInfo.EventHandler ( pHandle, 
                                            pHandle->pApplicationPrivate,
                                            OMX_EventResourcesAcquired, 
                                            0, 0, NULL);
    }
}
#endif
