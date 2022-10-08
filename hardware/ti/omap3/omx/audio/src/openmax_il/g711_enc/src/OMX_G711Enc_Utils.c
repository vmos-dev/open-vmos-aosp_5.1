
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
 * @file OMX_G711Enc_Utils.c
 *
 * This file implements G711 Encoder Component Specific APIs and its functionality
 * that is fully compliant with the Khronos OpenMAX (TM) 1.0 Specification
 *
 * @path  $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\g711_enc\src
 *
 * @rev  1.0
 */
/* ----------------------------------------------------------------------------
 *!
 *! Revision History
 *! ===================================
 *! 12-Dec-2006: Initial Version
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
#include <memory.h>
#include <fcntl.h>
#include <errno.h>

#endif

#include <dbapi.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
/*-------program files ----------------------------------------*/


#include "OMX_G711Enc_Utils.h"
#include <encode_common_ti.h>
#include <g711enc_sn_uuid.h>
#include <usn.h>

 

#ifdef UNDER_CE
#define HASHINGENABLE 1
#endif


/* ========================================================================== */
/**
 * @G711ENC_FillLCMLInitParams () This function is used by the component thread to
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
OMX_ERRORTYPE G711ENC_FillLCMLInitParams(OMX_HANDLETYPE pComponent,
                                         LCML_DSP *plcml_Init, OMX_U16 arr[])
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nIpBuf = 0,nIpBufSize = 0,nOpBuf = 0,nOpBufSize = 0;
    OMX_BUFFERHEADERTYPE *pTemp = NULL;
    LCML_DSP_INTERFACE *pHandle = (LCML_DSP_INTERFACE *)pComponent;
    G711ENC_COMPONENT_PRIVATE *pComponentPrivate = pHandle->pComponentPrivate;
    G711ENC_LCML_BUFHEADERTYPE *pTemp_lcml = NULL;
    OMX_U32 i = 0;
    OMX_U32 size_lcml = 0;
    OMX_U8 *pBufferParamTemp = NULL;
    
    G711ENC_DPRINT("%d :: Entering G711ENC_FillLCMLInitParams\n",__LINE__);

    nIpBuf = pComponentPrivate->pInputBufferList->numBuffers;
    nIpBufSize = pComponentPrivate->pPortDef[G711ENC_INPUT_PORT]->nBufferSize;
    pComponentPrivate->nRuntimeInputBuffers = nIpBuf;

    nOpBuf = pComponentPrivate->pOutputBufferList->numBuffers;
    nOpBufSize = pComponentPrivate->pPortDef[G711ENC_OUTPUT_PORT]->nBufferSize;
    pComponentPrivate->nRuntimeOutputBuffers = nOpBuf;

    G711ENC_DPRINT("%d :: ------ Buffer Details -----------\n",__LINE__);
    G711ENC_DPRINT("%d :: Input  Buffer Count = %ld\n",__LINE__,nIpBuf);
    G711ENC_DPRINT("%d :: Input  Buffer Size = %ld\n",__LINE__,nIpBufSize);
    G711ENC_DPRINT("%d :: Output Buffer Count = %ld\n",__LINE__,nOpBuf);
    G711ENC_DPRINT("%d :: Output Buffer Size = %ld\n",__LINE__,nOpBufSize);
    G711ENC_DPRINT("%d :: ------ Buffer Details ------------\n",__LINE__);

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
    
    plcml_Init->NodeInfo.AllUUIDs[0].uuid = &G711ENCSOCKET_TI_UUID;
    strcpy ((char*)plcml_Init->NodeInfo.AllUUIDs[0].DllName,G711ENC_DLL_NAME);
    plcml_Init->NodeInfo.AllUUIDs[0].eDllType = DLL_NODEOBJECT;

    plcml_Init->NodeInfo.AllUUIDs[1].uuid = &G711ENCSOCKET_TI_UUID;
    strcpy ((char*)plcml_Init->NodeInfo.AllUUIDs[1].DllName,G711ENC_DLL_NAME);
    plcml_Init->NodeInfo.AllUUIDs[1].eDllType = DLL_DEPENDENT;

    plcml_Init->NodeInfo.AllUUIDs[2].uuid = &USN_TI_UUID;
    strcpy ((char*)plcml_Init->NodeInfo.AllUUIDs[2].DllName,G711ENC_USN_DLL_NAME);
    plcml_Init->NodeInfo.AllUUIDs[2].eDllType = DLL_DEPENDENT;

    plcml_Init->DeviceInfo.TypeofDevice = 0;
    if(pComponentPrivate->dasfMode == 1) {
        G711ENC_DPRINT("%d :: Codec is configuring to DASF mode\n",__LINE__);
        G711ENC_OMX_MALLOC_STRUCT(pComponentPrivate->strmAttr, LCML_STRMATTR);
        pComponentPrivate->strmAttr->uSegid = G711ENC_DEFAULT_SEGMENT;
        pComponentPrivate->strmAttr->uAlignment = 0;
        pComponentPrivate->strmAttr->uTimeout = G711ENC_SN_TIMEOUT;
        pComponentPrivate->strmAttr->uBufsize = nIpBufSize;
        pComponentPrivate->strmAttr->uNumBufs = G711ENC_NUM_INPUT_BUFFERS_DASF;
        pComponentPrivate->strmAttr->lMode = STRMMODE_PROCCOPY;
        /* Device is Configuring to DASF Mode */
        plcml_Init->DeviceInfo.TypeofDevice = 1;
        /* Device is Configuring to Record Mode */
        plcml_Init->DeviceInfo.TypeofRender = 1;

        if(pComponentPrivate->acdnMode == 1) {
            /* ACDN mode */
            plcml_Init->DeviceInfo.AllUUIDs[0].uuid = &ACDN_TI_UUID;
        }
        else {
            /* DASF/TeeDN mode */
            plcml_Init->DeviceInfo.AllUUIDs[0].uuid = &DCTN_TI_UUID;
        }
        plcml_Init->DeviceInfo.DspStream = pComponentPrivate->strmAttr;
    }

    /*copy the other information*/
    plcml_Init->SegID = G711ENC_DEFAULT_SEGMENT;
    plcml_Init->Timeout = G711ENC_SN_TIMEOUT;
    plcml_Init->Alignment = 0;
    plcml_Init->Priority = G711ENC_SN_PRIORITY;

    /* Setting Creat Phase Parameters here */
    arr[0] = G711ENC_STREAM_COUNT;
    arr[1] = G711ENC_INPUT_PORT;

    if(pComponentPrivate->dasfMode == 1) {
        arr[2] = G711ENC_INSTRM;
        arr[3] = G711ENC_NUM_INPUT_BUFFERS_DASF;
    }
    else {
        arr[2] = G711ENC_DMM;
        if (pComponentPrivate->pInputBufferList->numBuffers) {
            arr[3] = (OMX_U16) pComponentPrivate->pInputBufferList->numBuffers;
        }
        else {
            arr[3] = 1;
        }
    }

    arr[4] = G711ENC_OUTPUT_PORT;
    arr[5] = G711ENC_DMM;
    if (pComponentPrivate->pOutputBufferList->numBuffers) {
        arr[6] = (OMX_U16) pComponentPrivate->pOutputBufferList->numBuffers;
    }
    else {
        arr[6] = 1;
    }

    /* set companding mode (A-Law or Mu-Law) */
    arr[7] = (OMX_U16)pComponentPrivate->G711Params[G711ENC_OUTPUT_PORT]->ePCMMode;
 
    arr[8] = pComponentPrivate->frametype;
    arr[9] = pComponentPrivate->vaumode;
    arr[10] = pComponentPrivate->vauthreshold;
    arr[11] = pComponentPrivate->vaunumber;
    arr[12] = pComponentPrivate->nmunoise;
    arr[13] = pComponentPrivate->lporder;
                                
    arr[14] = END_OF_CR_PHASE_ARGS;
    
    plcml_Init->pCrPhArgs = arr;

    /* Allocate memory for all input buffer headers..
     * This memory pointer will be sent to LCML */
    size_lcml = nIpBuf * sizeof(G711ENC_LCML_BUFHEADERTYPE);
    G711ENC_OMX_MALLOC_SIZE(pTemp_lcml,size_lcml,G711ENC_LCML_BUFHEADERTYPE);

    pComponentPrivate->pLcmlBufHeader[G711ENC_INPUT_PORT] = pTemp_lcml;
    for (i=0; i<nIpBuf; i++) {
        G711ENC_DPRINT("%d :: INPUT--------- Inside Ip Loop\n",__LINE__);
        pTemp = pComponentPrivate->pInputBufferList->pBufHdr[i];
        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pTemp->nAllocLen = nIpBufSize;
        pTemp->nFilledLen = nIpBufSize;
        pTemp->nVersion.s.nVersionMajor = G711ENC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = G711ENC_MINOR_VER;
        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = G711ENC_NOT_USED;
        pTemp_lcml->buffer = pTemp;
        G711ENC_DPRINT("%d :: pTemp_lcml->buffer->pBuffer = %p \n",__LINE__,pTemp_lcml->buffer->pBuffer);
        pTemp_lcml->eDir = OMX_DirInput;
        
        G711ENC_OMX_MALLOC_STRUCT(pTemp_lcml->pIpParam, G711ENC_ParamStruct);
        /* pTemp_lcml->pIpParam->usEndOfFile = 0; */
        
        G711ENC_OMX_MALLOC_SIZE(pBufferParamTemp, sizeof(G711ENC_ParamStruct) + DSP_CACHE_ALIGNMENT,OMX_U8);
        pTemp_lcml->pBufferParam =  (G711ENC_ParamStruct*)(pBufferParamTemp + EXTRA_BYTES);
        pTemp_lcml->pBufferParam->usNbFrames=0;
        pTemp_lcml->pBufferParam->pParamElem=NULL;
        pTemp_lcml->pFrameParam=NULL;
        G711ENC_OMX_MALLOC_STRUCT(pTemp_lcml->pDmmBuf, DMM_BUFFER_OBJ);
        
        /* This means, it is not a last buffer. This flag is to be modified by the application to indicate the last buffer */
        pTemp->nFlags = G711ENC_NORMAL_BUFFER;
        pTemp++;
        pTemp_lcml++;
    }

    /* Allocate memory for all output buffer headers..
     * This memory pointer will be sent to LCML */
    size_lcml = nOpBuf * sizeof(G711ENC_LCML_BUFHEADERTYPE);
    G711ENC_OMX_MALLOC_SIZE(pTemp_lcml,size_lcml,G711ENC_LCML_BUFHEADERTYPE);

    pComponentPrivate->pLcmlBufHeader[G711ENC_OUTPUT_PORT] = pTemp_lcml;
    
    for (i=0; i<nOpBuf; i++) {
        G711ENC_DPRINT("%d :: OUTPUT--------- Inside Op Loop\n",__LINE__);
        pTemp = pComponentPrivate->pOutputBufferList->pBufHdr[i];
        G711ENC_DPRINT("%d :: pBuffer from outputbuflist %p\n",__LINE__, pTemp->pBuffer);
        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        /*pTemp->nAllocLen = nOpBufSize;*/
        pTemp->nFilledLen = nOpBufSize;
        G711ENC_DPRINT("%d :: pTemp Filled Len %d\n",__LINE__, pTemp->nFilledLen);
        pTemp->nVersion.s.nVersionMajor = G711ENC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = G711ENC_MINOR_VER;
        pComponentPrivate->nVersion = pTemp->nVersion.nVersion;
        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = G711ENC_NOT_USED;
        pTemp_lcml->buffer = pTemp;
        G711ENC_DPRINT("%d :: pTemp_lcml->buffer->pBuffer = %p \n",__LINE__,pTemp_lcml->buffer->pBuffer);
        pTemp_lcml->eDir = OMX_DirOutput;
        G711ENC_OMX_MALLOC_STRUCT(pTemp_lcml->pOpParam, G711ENC_UAlgOutBufParamStruct);
        pTemp_lcml->pOpParam->ulFrameCount = 0;
        
        G711ENC_OMX_MALLOC_STRUCT(pTemp_lcml->pBufferParam, G711ENC_ParamStruct);
        pTemp_lcml->pBufferParam->usNbFrames=0;
        pTemp_lcml->pBufferParam->pParamElem=NULL;
        pTemp_lcml->pFrameParam=NULL;
        G711ENC_OMX_MALLOC_STRUCT(pTemp_lcml->pDmmBuf, DMM_BUFFER_OBJ);
        
        /* This means, it is not a last buffer. This flag is to be modified by
         * the application to indicate the last buffer */
        pTemp->nFlags = G711ENC_NORMAL_BUFFER;
        pTemp++;
        pTemp_lcml++;
    }

    pComponentPrivate->bPortDefsAllocated = 1;
    pComponentPrivate->bInitParamsInitialized = 1;
 EXIT:
    G711ENC_DPRINT("%d :: Exiting G711ENC_FillLCMLInitParams\n",__LINE__);
    G711ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}

/* ========================================================================== */
/**
 * @G711ENC_StartComponentThread() This function is called by the component to create
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
OMX_ERRORTYPE G711ENC_StartComponentThread(OMX_HANDLETYPE pComponent)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    G711ENC_COMPONENT_PRIVATE *pComponentPrivate =
        (G711ENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
#ifdef UNDER_CE
    pthread_attr_t attr;
    memset(&attr, 0, sizeof(attr));
    attr.__inheritsched = PTHREAD_EXPLICIT_SCHED;
    attr.__schedparam.__sched_priority = OMX_AUDIO_ENCODER_THREAD_PRIORITY;
#endif

    G711ENC_DPRINT ("%d :: Enetering  G711ENC_StartComponentThread\n", __LINE__);
    /* Initialize all the variables*/
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
    if (eError) {
        eError = OMX_ErrorInsufficientResources;
        G711ENC_DPRINT("%d :: Error while creating cmdDataPipe\n",__LINE__);
        goto EXIT;
    }
    /* create the pipe used to send buffers to the thread */
    eError = pipe (pComponentPrivate->dataPipe);
    if (eError) {
        eError = OMX_ErrorInsufficientResources;
        G711ENC_DPRINT("%d :: Error while creating dataPipe\n",__LINE__);
        goto EXIT;
    }

    /* create the pipe used to send commands to the thread */
    eError = pipe (pComponentPrivate->cmdPipe);
    if (eError) {
        eError = OMX_ErrorInsufficientResources;
        G711ENC_DPRINT("%d :: Error while creating cmdPipe\n",__LINE__);
        goto EXIT;
    }

    /* Create the Component Thread */
#ifdef UNDER_CE
    eError = pthread_create (&(pComponentPrivate->ComponentThread), &attr,
                             G711ENC_CompThread, pComponentPrivate);
#else
    eError = pthread_create (&(pComponentPrivate->ComponentThread), NULL,
                             G711ENC_CompThread, pComponentPrivate);
#endif
    if (eError || !pComponentPrivate->ComponentThread) {
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    pComponentPrivate->bCompThreadStarted = 1;
 EXIT:
    G711ENC_DPRINT("%d :: Exiting G711ENC_StartComponentThread\n", __LINE__);
    G711ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}

/* ========================================================================== */
/**
 * @G711ENC_FreeCompResources() This function is called by the component during
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

OMX_ERRORTYPE G711ENC_FreeCompResources(OMX_HANDLETYPE pComponent)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_ERRORTYPE err = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    G711ENC_COMPONENT_PRIVATE *pComponentPrivate = (G711ENC_COMPONENT_PRIVATE *)
        pHandle->pComponentPrivate;
    G711ENC_DPRINT("%d :: Entering G711ENC_FreeCompResources\n",__LINE__);

    if (pComponentPrivate->bCompThreadStarted) {
        OMX_G711ENC_CLOSE_PIPE(pComponentPrivate->dataPipe[0],err);
        OMX_G711ENC_CLOSE_PIPE(pComponentPrivate->dataPipe[1],err);
        OMX_G711ENC_CLOSE_PIPE(pComponentPrivate->cmdPipe[0],err);
        OMX_G711ENC_CLOSE_PIPE(pComponentPrivate->cmdPipe[1],err);
        OMX_G711ENC_CLOSE_PIPE(pComponentPrivate->cmdDataPipe[0],err);
        OMX_G711ENC_CLOSE_PIPE(pComponentPrivate->cmdDataPipe[1],err);
    }

    if (pComponentPrivate->bPortDefsAllocated) {
        OMX_G711ENC_MEMFREE_STRUCT(pComponentPrivate->pPortDef[G711ENC_INPUT_PORT]);
        OMX_G711ENC_MEMFREE_STRUCT(pComponentPrivate->pPortDef[G711ENC_OUTPUT_PORT]);
        OMX_G711ENC_MEMFREE_STRUCT(pComponentPrivate->G711Params[G711ENC_INPUT_PORT]);
        OMX_G711ENC_MEMFREE_STRUCT(pComponentPrivate->G711Params[G711ENC_OUTPUT_PORT]);

        OMX_G711ENC_MEMFREE_STRUCT(pComponentPrivate->pCompPort[G711ENC_INPUT_PORT]->pPortFormat);
        OMX_G711ENC_MEMFREE_STRUCT(pComponentPrivate->pCompPort[G711ENC_OUTPUT_PORT]->pPortFormat);
        OMX_G711ENC_MEMFREE_STRUCT(pComponentPrivate->pCompPort[G711ENC_INPUT_PORT]);
        OMX_G711ENC_MEMFREE_STRUCT(pComponentPrivate->pCompPort[G711ENC_OUTPUT_PORT]);

        OMX_G711ENC_MEMFREE_STRUCT(pComponentPrivate->sPortParam);
        OMX_G711ENC_MEMFREE_STRUCT(pComponentPrivate->sPriorityMgmt);
        OMX_G711ENC_MEMFREE_STRUCT(pComponentPrivate->pInputBufferList);
        OMX_G711ENC_MEMFREE_STRUCT(pComponentPrivate->pOutputBufferList);
    }

#ifndef UNDER_CE
    pthread_mutex_destroy(&pComponentPrivate->AlloBuf_mutex);
    pthread_cond_destroy(&pComponentPrivate->AlloBuf_threshold);

    pthread_mutex_destroy(&pComponentPrivate->InIdle_mutex);
    pthread_cond_destroy(&pComponentPrivate->InIdle_threshold);

    pthread_mutex_destroy(&pComponentPrivate->InLoaded_mutex);
    pthread_cond_destroy(&pComponentPrivate->InLoaded_threshold);
#endif

    pComponentPrivate->bPortDefsAllocated = 0;
 EXIT:
    G711ENC_DPRINT("%d :: Exiting G711ENC_FreeCompResources()\n",__LINE__);
    G711ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}

/* ========================================================================== */
/**
 * @G711ENC_CleanupInitParams() This function is called by the component during
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

OMX_ERRORTYPE G711ENC_CleanupInitParams(OMX_HANDLETYPE pComponent)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nIpBuf = 0;
    OMX_U32 nOpBuf = 0;
    OMX_U32 i = 0;
    OMX_U8* pParmsTemp = NULL;
    G711ENC_LCML_BUFHEADERTYPE *pTemp_lcml = NULL;
    OMX_U8 *pBufParmsTemp = NULL;
    OMX_U8 *pFrameParmsTemp = NULL;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    G711ENC_COMPONENT_PRIVATE *pComponentPrivate = (G711ENC_COMPONENT_PRIVATE *)
        pHandle->pComponentPrivate;
    G711ENC_DPRINT("%d :: Entering G711ENC_CleanupInitParams()\n", __LINE__);

    if(pComponentPrivate->dasfMode == 1) {
        pParmsTemp = (OMX_U8*)pComponentPrivate->pParams;
        if (pParmsTemp != NULL){
            pParmsTemp -= EXTRA_BYTES;
        }
        pComponentPrivate->pParams = (G711ENC_AudioCodecParams*)pParmsTemp;
        OMX_G711ENC_MEMFREE_STRUCT(pComponentPrivate->pParams);
        OMX_G711ENC_MEMFREE_STRUCT(pComponentPrivate->strmAttr);
    }
    /*    OMX_G711ENC_MEMFREE_STRUCT(pComponentPrivate->pAlgParam); */          /*Not yet used */
    pTemp_lcml = pComponentPrivate->pLcmlBufHeader[G711ENC_INPUT_PORT];
    nIpBuf = pComponentPrivate->nRuntimeInputBuffers;
    for(i=0; i<nIpBuf; i++) {
        OMX_G711ENC_MEMFREE_STRUCT(pTemp_lcml->pIpParam);
        pBufParmsTemp = (OMX_U8*)pTemp_lcml->pBufferParam; 
        pBufParmsTemp -=EXTRA_BYTES;
        OMX_G711ENC_MEMFREE_STRUCT(pBufParmsTemp);
        OMX_G711ENC_MEMFREE_STRUCT(pTemp_lcml->pDmmBuf);
        pFrameParmsTemp = (OMX_U8*)pTemp_lcml->pFrameParam; 
        pFrameParmsTemp -=EXTRA_BYTES;
        OMX_G711ENC_MEMFREE_STRUCT(pFrameParmsTemp); 
        pTemp_lcml++;
    }

    pTemp_lcml = pComponentPrivate->pLcmlBufHeader[G711ENC_OUTPUT_PORT];
    nOpBuf =  pComponentPrivate->nRuntimeOutputBuffers;
    for(i=0; i<nOpBuf; i++) {
        OMX_G711ENC_MEMFREE_STRUCT(pTemp_lcml->pOpParam);
        OMX_G711ENC_MEMFREE_STRUCT(pTemp_lcml->pBufferParam);
        OMX_G711ENC_MEMFREE_STRUCT(pTemp_lcml->pDmmBuf);
        pBufParmsTemp = (OMX_U8*)pTemp_lcml->pFrameParam; 
        pBufParmsTemp -=EXTRA_BYTES;
        OMX_G711ENC_MEMFREE_STRUCT(pBufParmsTemp);
        pTemp_lcml++;
    }

    OMX_G711ENC_MEMFREE_STRUCT(pComponentPrivate->pLcmlBufHeader[G711ENC_INPUT_PORT]);
    OMX_G711ENC_MEMFREE_STRUCT(pComponentPrivate->pLcmlBufHeader[G711ENC_OUTPUT_PORT]);

    G711ENC_DPRINT("%d :: Exiting G711ENC_CleanupInitParams()\n",__LINE__);
    G711ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}

/* ========================================================================== */
/**
 * @G711ENC_StopComponentThread() This function is called by the component during
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

OMX_ERRORTYPE G711ENC_StopComponentThread(OMX_HANDLETYPE pComponent)
{
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    G711ENC_COMPONENT_PRIVATE *pComponentPrivate = (G711ENC_COMPONENT_PRIVATE *)
        pHandle->pComponentPrivate;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_ERRORTYPE threadError = OMX_ErrorNone;
    int pthreadError = 0;
    
    G711ENC_DPRINT("%d :: Entering G711ENC_StopComponentThread\n",__LINE__);
    pComponentPrivate->bIsThreadstop = 1;
    G711ENC_DPRINT("%d :: About to call pthread_join\n",__LINE__);
    pthreadError = pthread_join (pComponentPrivate->ComponentThread,(void*)&threadError);
    
    if (0 != pthreadError) {
        eError = OMX_ErrorHardware;
        G711ENC_DPRINT("%d :: Error closing ComponentThread - pthreadError = %d\n",__LINE__,pthreadError);
        goto EXIT;
    }
    if (OMX_ErrorNone != threadError && OMX_ErrorNone != eError) {
        eError = OMX_ErrorInsufficientResources;
        G711ENC_DPRINT("%d :: Error while closing Component Thread\n",__LINE__);
        goto EXIT;
    }
    
 EXIT:
    G711ENC_DPRINT("%d :: Exiting G711ENC_StopComponentThread\n",__LINE__);
    G711ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}
/* ========================================================================== */
/**
 * @G711ENC_HandleCommand() This function is called by the component when ever it
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
OMX_U32 G711ENC_HandleCommand (G711ENC_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMMANDTYPE command;
    OMX_STATETYPE commandedState = OMX_StateInvalid;
    OMX_HANDLETYPE pLcmlHandle;
    LCML_CALLBACKTYPE cb;
    LCML_DSP *pLcmlDsp = NULL;
    OMX_U32 pValues[4] = {0};
    OMX_U32 commandData = 0;
    OMX_U16 arr[100] = {0};
    char *p = "damedesuStr";
    OMX_U8* pParmsTemp = NULL;
    OMX_U32 i = 0;
    OMX_U32 ret = 0;
    OMX_U8 inputPortFlag=0,outputPortFlag=0;    

    G711ENC_LCML_BUFHEADERTYPE *pLcmlHdr = NULL;
#ifdef RESOURCE_MANAGER_ENABLED
    OMX_ERRORTYPE rm_error = OMX_ErrorNone;
#endif

    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;
    pLcmlHandle = pComponentPrivate->pLcmlHandle;

    G711ENC_DPRINT("%d :: Entering G711ENCHandleCommand Function \n",__LINE__);
    G711ENC_DPRINT("%d :: pComponentPrivate->curState = %d\n",__LINE__,pComponentPrivate->curState);
    ret = read(pComponentPrivate->cmdPipe[0], &command, sizeof (command));
    if (ret == -1) {
        G711ENC_DPRINT("%d :: Error in Reading from the Data pipe\n", __LINE__);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }
    ret = read(pComponentPrivate->cmdDataPipe[0], &commandData, sizeof (commandData));
    if (ret == -1) {
        G711ENC_DPRINT("%d :: Error in Reading from the Data pipe\n", __LINE__);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }

    if (command == OMX_CommandStateSet) {
        commandedState = (OMX_STATETYPE)commandData;
        if ( pComponentPrivate->curState==commandedState){
            pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                   pHandle->pApplicationPrivate,
                                                   OMX_EventError,
                                                   OMX_ErrorSameState,
                                                   0, NULL);
            G711ENC_PRINT("%d :: Error: Same State Given by Application\n",__LINE__);
        }
        else{
            switch(commandedState) {
            case OMX_StateIdle:
                G711ENC_DPRINT("%d :: G711ENC_HandleCommand :: OMX_StateIdle \n",__LINE__);
                G711ENC_DPRINT("%d :: pComponentPrivate->curState = %d\n",__LINE__,pComponentPrivate->curState);
                if (pComponentPrivate->curState == OMX_StateLoaded || 
                    pComponentPrivate->curState == OMX_StateWaitForResources) {
                    if (pComponentPrivate->dasfMode == 1) {
                        pComponentPrivate->pPortDef[G711ENC_INPUT_PORT]->bEnabled= FALSE;
                        pComponentPrivate->pPortDef[G711ENC_INPUT_PORT]->bPopulated= FALSE;
                        if(pComponentPrivate->streamID == 0) { 
                            G711ENC_DPRINT("**************************************\n");
                            G711ENC_DPRINT(":: Error = OMX_ErrorInsufficientResources\n");
                            G711ENC_DPRINT("**************************************\n");
                            eError = OMX_ErrorInsufficientResources; 
                            pComponentPrivate->curState = OMX_StateInvalid; 
                            pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                                   pHandle->pApplicationPrivate, 
                                                                   OMX_EventError, 
                                                                   OMX_ErrorInvalidState,
                                                                   0, NULL);              
                            goto EXIT; 
                        }                                                       
                    }

                    if (pComponentPrivate->pPortDef[G711ENC_INPUT_PORT]->bPopulated &&  
                        pComponentPrivate->pPortDef[G711ENC_INPUT_PORT]->bEnabled)  {
                        inputPortFlag = 1;
                    }
                    if (pComponentPrivate->pPortDef[G711ENC_OUTPUT_PORT]->bPopulated && 
                        pComponentPrivate->pPortDef[G711ENC_OUTPUT_PORT]->bEnabled) {
                        outputPortFlag = 1;
                    }
                    if (!pComponentPrivate->pPortDef[G711ENC_INPUT_PORT]->bPopulated &&  
                        !pComponentPrivate->pPortDef[G711ENC_INPUT_PORT]->bEnabled)  {
                        inputPortFlag = 1;
                    }
                    if (!pComponentPrivate->pPortDef[G711ENC_OUTPUT_PORT]->bPopulated && 
                        !pComponentPrivate->pPortDef[G711ENC_OUTPUT_PORT]->bEnabled) {
                        outputPortFlag = 1;
                    }

                    if(!(inputPortFlag && outputPortFlag)){ 
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

                    cb.LCML_Callback = (void *) G711ENC_LCMLCallback;
                    pLcmlHandle = (OMX_HANDLETYPE) G711ENC_GetLCMLHandle(pComponentPrivate);
                    if (pLcmlHandle == NULL) {
                        G711ENC_DPRINT("%d :: LCML Handle is NULL........exiting..\n",__LINE__);
                        goto EXIT;
                    }
                    pComponentPrivate->pLcmlHandle = (LCML_DSP_INTERFACE *)pLcmlHandle;

                    /* Got handle of dsp via phandle filling information about DSP Specific things */
                    pLcmlDsp = (((LCML_DSP_INTERFACE*)pLcmlHandle)->dspCodec);
                    eError = G711ENC_FillLCMLInitParams(pHandle, pLcmlDsp, arr);
                    if(eError != OMX_ErrorNone) {
                        G711ENC_DPRINT("%d :: Error from G711ENCFill_LCMLInitParams()\n",__LINE__);
                        goto EXIT;
                    }

                    G711ENC_DPRINT("%d :: Calling LCML_InitMMCodecEx...\n",__LINE__);
#ifndef UNDER_CE
                    eError = LCML_InitMMCodecEx(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                p,
                                                &pLcmlHandle, 
                                                (void *)p, 
                                                &cb,
                                                (OMX_STRING)pComponentPrivate->sDeviceString);
#else
                    eError = LCML_InitMMCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                              p,
                                              &pLcmlHandle, 
                                              (void *)p, 
                                              &cb);
#endif
                    if(eError != OMX_ErrorNone) {
                        G711ENC_DPRINT("%d :: Error returned from LCML_Init()\n",__LINE__);
                        goto EXIT;
                    }
#ifdef HASHINGENABLE
                    /* Enable the Hashing Code */
                    eError = LCML_SetHashingState(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle, OMX_TRUE);
                    if (eError != OMX_ErrorNone){
                        G711ENC_DPRINT("Failed to set Mapping State\n");
                        goto EXIT;
                    }
#endif

#ifdef RESOURCE_MANAGER_ENABLED
                    /* Need check the resource with RM */
                    pComponentPrivate->rmproxyCallback.RMPROXY_Callback = 
                        (void *) G711ENC_ResourceManagerCallback;
                    if (pComponentPrivate->curState != OMX_StateWaitForResources) {
                        rm_error = RMProxy_NewSendCommand(pHandle, RMProxy_RequestResource, 
                                                          OMX_G711_Encoder_COMPONENT, 
                                                          G711ENC_CPU, 
                                                          1234, 
                                                          &(pComponentPrivate->rmproxyCallback));
                        if(rm_error == OMX_ErrorNone) {
                            /* resource is available */
                            pComponentPrivate->curState = OMX_StateIdle;
                            pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                                    pHandle->pApplicationPrivate,
                                                                    OMX_EventCmdComplete,
                                                                    OMX_CommandStateSet,
                                                                    pComponentPrivate->curState,
                                                                    NULL);
                            rm_error = RMProxy_NewSendCommand(pHandle, 
                                                              RMProxy_StateSet, 
                                                              OMX_G711_Encoder_COMPONENT, 
                                                              OMX_StateIdle, 1234, NULL);
                        }
                        else if(rm_error == OMX_ErrorInsufficientResources) {
                            /* resource is not available, need set state to OMX_StateWaitForResources */
                            pComponentPrivate->curState = OMX_StateWaitForResources;
                            pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                                    pHandle->pApplicationPrivate,
                                                                    OMX_EventCmdComplete,
                                                                    OMX_CommandStateSet,
                                                                    pComponentPrivate->curState,
                                                                    NULL);
                            G711ENC_DPRINT("%d :: Comp: OMX_ErrorInsufficientResources\n", __LINE__);
                        }
                    }
                    else{
                        rm_error = RMProxy_NewSendCommand(pHandle, RMProxy_StateSet,
                                                          OMX_G711_Encoder_COMPONENT,
                                                          OMX_StateIdle, 1234, NULL);
                       
                        pComponentPrivate->curState = OMX_StateIdle;
                        pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                               pHandle->pApplicationPrivate,
                                                               OMX_EventCmdComplete, 
                                                               OMX_CommandStateSet,
                                                               pComponentPrivate->curState,
                                                               NULL);
                    }
                
#else
                    pComponentPrivate->curState = OMX_StateIdle;
                    pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                            pHandle->pApplicationPrivate,
                                                            OMX_EventCmdComplete,
                                                            OMX_CommandStateSet,
                                                            pComponentPrivate->curState,
                                                            NULL);
#endif
                }
                else if (pComponentPrivate->curState == OMX_StateExecuting) {
                    G711ENC_DPRINT("%d :: Setting Component to OMX_StateIdle\n",__LINE__);
#ifdef HASHINGENABLE
                    /*Hashing Change*/
                    pLcmlHandle = (LCML_DSP_INTERFACE*)pComponentPrivate->pLcmlHandle;
                    eError = LCML_FlushHashes(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle);
                    if (eError != OMX_ErrorNone) {
                        G711ENC_DPRINT("Error occurred in Codec mapping flush!\n");
                        break;
                    }
#endif
                    G711ENC_DPRINT("%d :: G711ENC: About to Call MMCodecControlStop\n", __LINE__);
                    pComponentPrivate->bDspStoppedWhileExecuting = OMX_TRUE;                    
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               MMCodecControlStop,(void *)p);
                    if(eError != OMX_ErrorNone) {
                        G711ENC_DPRINT("%d :: Error from LCML_ControlCodec MMCodecControlStop..\n",__LINE__);
                        goto EXIT;
                    }
                }
                else if(pComponentPrivate->curState == OMX_StatePause) {

#ifdef HASHINGENABLE
                    /*Hashing Change*/
                    pLcmlHandle = (LCML_DSP_INTERFACE*)pComponentPrivate->pLcmlHandle;
                    /* clear out any mappings that might have accumulated */
                    eError = LCML_FlushHashes(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle);
                    if (eError != OMX_ErrorNone) {
                        G711ENC_DPRINT("Error occurred in Codec mapping flush!\n");
                        break;
                    }
#endif              
                    pComponentPrivate->curState = OMX_StateIdle;
#ifdef RESOURCE_MANAGER_ENABLED
                    rm_error = RMProxy_NewSendCommand(pHandle,
                                                      RMProxy_StateSet,
                                                      OMX_G711_Encoder_COMPONENT,
                                                      OMX_StateIdle, 1234, NULL);
#endif
                    pComponentPrivate->cbInfo.EventHandler ( pHandle,
                                                             pHandle->pApplicationPrivate,
                                                             OMX_EventCmdComplete,
                                                             OMX_CommandStateSet,
                                                             pComponentPrivate->curState,
                                                             NULL);
                }
                else {   /* This means, it is invalid state from application */
                    pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                            pHandle->pApplicationPrivate,
                                                            OMX_EventError,
                                                            OMX_ErrorIncorrectStateTransition,
                                                            0,
                                                            NULL);
                    G711ENC_DPRINT("%d :: Comp: OMX_ErrorIncorrectStateTransition\n",__LINE__);
                }
                break;

            case OMX_StateExecuting:
                G711ENC_DPRINT("%d :: G711ENC_HandleCommand :: OMX_StateExecuting \n",__LINE__);
                if (pComponentPrivate->curState == OMX_StateIdle) {
                    if(pComponentPrivate->dasfMode == 1) {
                        G711ENC_DPRINT("%d :: ---- Comp: DASF Functionality is ON ---\n",__LINE__);
                        G711ENC_OMX_MALLOC_SIZE(pComponentPrivate->pParams,
                                                (sizeof(G711ENC_AudioCodecParams)+DSP_CACHE_ALIGNMENT),
                                                G711ENC_AudioCodecParams);
                        /* cache aligment */
                        pParmsTemp = (OMX_U8*)pComponentPrivate->pParams;
                        pParmsTemp+= EXTRA_BYTES;
                        pComponentPrivate->pParams = (G711ENC_AudioCodecParams*)pParmsTemp;         
                        
                        pComponentPrivate->pParams->iAudioFormat = 1;
                        pComponentPrivate->pParams->iStrmId = pComponentPrivate->streamID;
                        pComponentPrivate->pParams->iSamplingRate = G711ENC_SAMPLING_FREQUENCY;

                        pValues[0] = USN_STRMCMD_SETCODECPARAMS;
                        pValues[1] = (OMX_U32)pComponentPrivate->pParams;
                        pValues[2] = sizeof(G711ENC_AudioCodecParams);
                        /* Sending STRMCTRL MESSAGE to DSP via LCML_ControlCodec*/
                        eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                   EMMCodecControlStrmCtrl,(void *)pValues);
                        if(eError != OMX_ErrorNone) {
                            G711ENC_DPRINT("%d :: Error from LCML_ControlCodec EMMCodecControlStrmCtrl = %x\n",__LINE__,eError);
                            goto EXIT;
                        }
                    }
                    pComponentPrivate->bBypassDSP = 0; 
                    /* Sending START MESSAGE to DSP via LCML_ControlCodec*/
                    pComponentPrivate->bDspStoppedWhileExecuting = OMX_FALSE;
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               EMMCodecControlStart, (void *)p);
                    if(eError != OMX_ErrorNone) {
                        G711ENC_PRINT("%d :: Error from LCML_ControlCodec EMMCodecControlStart = %x\n",__LINE__,eError);
                        goto EXIT;
                    }
                }
                else if (pComponentPrivate->curState == OMX_StatePause) {
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               EMMCodecControlStart, (void *)p);
                    if (eError != OMX_ErrorNone) {
                        G711ENC_DPRINT("%d :: Error While Resuming the codec = %x\n",__LINE__,eError);
                        goto EXIT;
                    }
                    for (i=0; i < pComponentPrivate->nNumInputBufPending; i++) {
                        if (pComponentPrivate->pInputBufHdrPending[i]) {
                            G711ENC_GetCorrespondingLCMLHeader(pComponentPrivate,pComponentPrivate->pInputBufHdrPending[i]->pBuffer, OMX_DirInput, &pLcmlHdr);
                            G711ENC_SetPending(pComponentPrivate,pComponentPrivate->pInputBufHdrPending[i],OMX_DirInput,__LINE__);

                            eError = LCML_QueueBuffer(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                      EMMCodecInputBuffer,
                                                      pComponentPrivate->pInputBufHdrPending[i]->pBuffer,
                                                      pComponentPrivate->pInputBufHdrPending[i]->nAllocLen,
                                                      pComponentPrivate->pInputBufHdrPending[i]->nFilledLen,
                                                      (OMX_U8 *) pLcmlHdr->pIpParam,
                                                      sizeof(G711ENC_ParamStruct),
                                                      NULL);
                                                        
                            pComponentPrivate->pInputBufHdrPending[i] = NULL;
                        }
                    }
                    pComponentPrivate->nNumInputBufPending = 0;

                    for (i=0; i < pComponentPrivate->nNumOutputBufPending; i++) {
                        if (pComponentPrivate->pOutputBufHdrPending[i]) {
                            G711ENC_GetCorrespondingLCMLHeader(pComponentPrivate,pComponentPrivate->pOutputBufHdrPending[i]->pBuffer, OMX_DirOutput, &pLcmlHdr);
                            G711ENC_SetPending(pComponentPrivate,pComponentPrivate->pOutputBufHdrPending[i],OMX_DirOutput,__LINE__);
                            
                            eError = LCML_QueueBuffer(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                      EMMCodecOuputBuffer,
                                                      pComponentPrivate->pOutputBufHdrPending[i]->pBuffer,
                                                      pComponentPrivate->pOutputBufHdrPending[i]->nAllocLen,
                                                      pComponentPrivate->pOutputBufHdrPending[i]->nFilledLen,
                                                      (OMX_U8 *) pLcmlHdr->pIpParam,
                                                      sizeof(G711ENC_ParamStruct),
                                                      NULL);
                                                        
                            pComponentPrivate->pOutputBufHdrPending[i] = NULL;
                        }
                    }
                    pComponentPrivate->nNumOutputBufPending = 0;
                }
                else {
                    pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                            pHandle->pApplicationPrivate,
                                                            OMX_EventError,
                                                            OMX_ErrorIncorrectStateTransition,
                                                            0, NULL);
                    G711ENC_DPRINT("%d :: Comp: OMX_ErrorIncorrectStateTransition Given by Comp\n",__LINE__);
                    goto EXIT;

                }
                pComponentPrivate->curState = OMX_StateExecuting; /* Change to Executing */
#ifdef RESOURCE_MANAGER_ENABLED
                rm_error = RMProxy_NewSendCommand(pHandle,
                                                  RMProxy_StateSet,
                                                  OMX_G711_Encoder_COMPONENT,
                                                  OMX_StateExecuting, 1234, NULL);
#endif
                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,
                                                        OMX_CommandStateSet,
                                                        pComponentPrivate->curState,
                                                        NULL);
                G711ENC_DPRINT("%d :: Comp: OMX_CommandStateSet Given by Comp\n",__LINE__);
                break;

            case OMX_StateLoaded:
                G711ENC_DPRINT("%d :: G711ENC_HandleCommand :: OMX_StateLoaded\n",__LINE__);
                if (pComponentPrivate->curState == OMX_StateWaitForResources){
                    G711ENC_DPRINT("%d :: G711ENC_HandleCommand :: OMX_StateWaitForResources\n",__LINE__);
                    pComponentPrivate->curState = OMX_StateLoaded;
                    pComponentPrivate->cbInfo.EventHandler ( pHandle,
                                                             pHandle->pApplicationPrivate,
                                                             OMX_EventCmdComplete,
                                                             OMX_CommandStateSet,
                                                             pComponentPrivate->curState,
                                                             NULL);
                    G711ENC_DPRINT("%d :: Comp: OMX_CommandStateSet Given by Comp\n",__LINE__);
                    break;
                }
                if (pComponentPrivate->curState != OMX_StateIdle){
                    G711ENC_DPRINT("%d :: G711ENC_HandleCommand :: OMX_StateIdle && OMX_StateWaitForResources\n",__LINE__);
                    pComponentPrivate->cbInfo.EventHandler ( pHandle,
                                                             pHandle->pApplicationPrivate,
                                                             OMX_EventError,
                                                             OMX_ErrorIncorrectStateTransition,
                                                             0, NULL);
                    G711ENC_DPRINT("%d :: Error: OMX_ErrorIncorrectStateTransition Given by Comp\n",__LINE__);
                    goto EXIT;
                }

                if (pComponentPrivate->pInputBufferList->numBuffers ||
                    pComponentPrivate->pOutputBufferList->numBuffers){
                    pComponentPrivate->InIdle_goingtoloaded = 1;
#ifndef UNDER_CE
                    pthread_mutex_lock(&pComponentPrivate->InIdle_mutex); 
                    pthread_cond_wait(&pComponentPrivate->InIdle_threshold, 
                                      &pComponentPrivate->InIdle_mutex);
                    pthread_mutex_unlock(&pComponentPrivate->InIdle_mutex);
#else
                    OMX_WaitForEvent(&(pComponentPrivate->InIdle_event));
#endif
                    pComponentPrivate->bLoadedCommandPending = OMX_FALSE;
                }

                /* Now Deinitialize the component No error should be returned from
                 * this function. It should clean the system as much as possible */
                eError = G711ENC_CleanupInitParams(pHandle);
                if(eError != OMX_ErrorNone) {
                    G711ENC_PRINT("%d :: G711ENC_CleanupInitParams returned error\n",__LINE__);
                    goto EXIT;
                }               
                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                           EMMCodecControlDestroy, (void *)p);
                if (eError != OMX_ErrorNone) {
                    G711ENC_PRINT("%d :: Error: LCML_ControlCodec EMMCodecControlDestroy = %x\n",__LINE__, eError);
                    goto EXIT;
                }

#ifndef UNDER_CE
                /*Closing LCML Lib*/
                if (pComponentPrivate->ptrLibLCML != NULL){
                    G711ENC_DPRINT("%d :: About to Close LCML %p \n",__LINE__,pComponentPrivate->ptrLibLCML);
                    dlclose( pComponentPrivate->ptrLibLCML  );  
                    pComponentPrivate->ptrLibLCML = NULL;    
                    G711ENC_DPRINT("%d :: Closed LCML \n",__LINE__);
                }
#endif
                
                eError = G711ENC_EXIT_COMPONENT_THRD;
                pComponentPrivate->bInitParamsInitialized = 0;
                pComponentPrivate->bLoadedCommandPending = OMX_FALSE;
                break;

            case OMX_StatePause:
                G711ENC_DPRINT("%d :: G711ENC_HandleCommand :: OMX_StatePause\n",__LINE__);
                if (pComponentPrivate->curState != OMX_StateExecuting &&
                    pComponentPrivate->curState != OMX_StateIdle) {
                    pComponentPrivate->cbInfo.EventHandler ( pHandle,
                                                             pHandle->pApplicationPrivate,
                                                             OMX_EventError,
                                                             OMX_ErrorIncorrectStateTransition,
                                                             0,
                                                             NULL);
                    G711ENC_PRINT("%d :: Error: OMX_ErrorIncorrectStateTransition Given by Comp\n",__LINE__);
                    goto EXIT;
                }
                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                           EMMCodecControlPause, (void *)p);
                if (eError != OMX_ErrorNone) {
                    G711ENC_DPRINT("%d :: Error: LCML_ControlCodec EMMCodecControlPause = %x\n",__LINE__,eError);
                    goto EXIT;
                }
                G711ENC_DPRINT("%d :: Comp: OMX_CommandStateSet Given by Comp\n",__LINE__);
                break;

            case OMX_StateWaitForResources:
                G711ENC_DPRINT("%d :: G711ENC_HandleCommand :: OMX_StateWaitForResources\n",__LINE__);
                if (pComponentPrivate->curState == OMX_StateLoaded) {
                    pComponentPrivate->curState = OMX_StateWaitForResources;
                    pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                            pHandle->pApplicationPrivate,
                                                            OMX_EventCmdComplete,
                                                            OMX_CommandStateSet,
                                                            pComponentPrivate->curState,
                                                            NULL);
                    G711ENC_DPRINT("%d :: Comp: OMX_CommandStateSet Given by Comp\n",__LINE__);
                } else {
                    pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                            pHandle->pApplicationPrivate,
                                                            OMX_EventError,
                                                            OMX_ErrorIncorrectStateTransition,
                                                            0,
                                                            NULL);
                    G711ENC_DPRINT("%d :: Error: OMX_ErrorIncorrectStateTransition Given by Comp\n",__LINE__);
                }
                break;

            case OMX_StateInvalid:
                G711ENC_DPRINT("%d :: G711ENC_HandleCommand :: OMX_StateInvalid\n",__LINE__);

                if (pComponentPrivate->curState != OMX_StateWaitForResources && 
                    pComponentPrivate->curState != OMX_StateInvalid && 
                    pComponentPrivate->curState != OMX_StateLoaded) {
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               EMMCodecControlDestroy, (void *)p);
                }

                pComponentPrivate->curState = OMX_StateInvalid;
                pComponentPrivate->cbInfo.EventHandler( pHandle, 
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventError, 
                                                        OMX_ErrorInvalidState,
                                                        0, 
                                                        NULL);
                G711ENC_CleanupInitParams(pHandle);
                break;

            case OMX_StateMax:
                G711ENC_DPRINT("%d :: G711ENC_HandleCommand :: Cmd OMX_StateMax\n",__LINE__);
                break;
            } /* End of Switch */
        }
    }
    
    else if (command == OMX_CommandMarkBuffer) {
        G711ENC_DPRINT("%d :: command OMX_CommandMarkBuffer received\n",__LINE__);
        if(!pComponentPrivate->pMarkBuf){
            /* TODO Need to handle multiple marks */
            pComponentPrivate->pMarkBuf = (OMX_MARKTYPE *)(commandData);
        }
    } 
    
    else if (command == OMX_CommandPortDisable) {
        G711ENC_DPRINT("%d :: Inside command Port disabled \n",__LINE__);
        if (!pComponentPrivate->bDisableCommandPending) {
            if(commandData == 0x0 || commandData == -1){
                /* disable port */
                pComponentPrivate->pPortDef[G711ENC_INPUT_PORT]->bEnabled = OMX_FALSE;
                G711ENC_DPRINT("%d :: command disabled input port\n",__LINE__);
            }
            if(commandData == 0x1 || commandData == -1){
                /* disable output port */
                char *p = "damedesuStr";
                pComponentPrivate->pPortDef[G711ENC_OUTPUT_PORT]->bEnabled = OMX_FALSE;
                G711ENC_DPRINT("%d :: command disabled output port\n",__LINE__);
                if (pComponentPrivate->curState == OMX_StateExecuting) {
                    pComponentPrivate->bNoIdleOnStop = OMX_TRUE;
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               MMCodecControlStop,(void *)p);
                }
            }
        }
        G711ENC_DPRINT("commandData = %ld\n",commandData);
        G711ENC_DPRINT("pComponentPrivate->pPortDef[INPUT_PORT]->bPopulated = %d\n",pComponentPrivate->pPortDef[G711ENC_INPUT_PORT]->bPopulated);
        G711ENC_DPRINT("pComponentPrivate->pPortDef[OUTPUT_PORT]->bPopulated = %d\n",pComponentPrivate->pPortDef[G711ENC_OUTPUT_PORT]->bPopulated);
        if(commandData == 0x0) {
            if(!pComponentPrivate->pPortDef[G711ENC_INPUT_PORT]->bPopulated){
                /* return cmdcomplete event if input unpopulated */
                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,
                                                        OMX_CommandPortDisable,
                                                        G711ENC_INPUT_PORT,
                                                        NULL);
                pComponentPrivate->bDisableCommandPending = 0;
            } else {
                pComponentPrivate->bDisableCommandPending = 1;
                pComponentPrivate->bDisableCommandParam = commandData;
            }
        }
        if(commandData == 0x1) {
            if (!pComponentPrivate->pPortDef[G711ENC_OUTPUT_PORT]->bPopulated){
                /* return cmdcomplete event if output unpopulated */
                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,
                                                        OMX_CommandPortDisable,
                                                        G711ENC_OUTPUT_PORT,
                                                        NULL);
                pComponentPrivate->bDisableCommandPending = 0;
            } else {
                pComponentPrivate->bDisableCommandPending = 1;
                pComponentPrivate->bDisableCommandParam = commandData;
            }
        }
        if(commandData == -1) {
            if (!pComponentPrivate->pPortDef[G711ENC_INPUT_PORT]->bPopulated &&
                !pComponentPrivate->pPortDef[G711ENC_OUTPUT_PORT]->bPopulated){
                /* return cmdcomplete event if inout & output unpopulated */
                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,
                                                        OMX_CommandPortDisable,
                                                        G711ENC_INPUT_PORT,
                                                        NULL);
                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,
                                                        OMX_CommandPortDisable,
                                                        G711ENC_OUTPUT_PORT,
                                                        NULL);
                pComponentPrivate->bDisableCommandPending = 0;
            }
            else {
                pComponentPrivate->bDisableCommandPending = 1;
                pComponentPrivate->bDisableCommandParam = commandData;
            }
        }
    }


    else if (command == OMX_CommandPortEnable) {
        if (!pComponentPrivate->bEnableCommandPending) {
            if(commandData == 0x0 || commandData == -1){
                /* enable in port */
                G711ENC_DPRINT("%d :: setting input port to enabled\n",__LINE__);
                pComponentPrivate->pPortDef[G711ENC_INPUT_PORT]->bEnabled = OMX_TRUE;
                
                if(pComponentPrivate->AlloBuf_waitingsignal){
                    pComponentPrivate->AlloBuf_waitingsignal = 0;
                }
            }
            G711ENC_DPRINT("pComponentPrivate->pPortDef[G711ENC_INPUT_PORT]->bEnabled = %d\n",pComponentPrivate->pPortDef[G711ENC_INPUT_PORT]->bEnabled);

            if(commandData == 0x1 || commandData == -1){
                /* enable out port */
                if(pComponentPrivate->AlloBuf_waitingsignal){
                    pComponentPrivate->AlloBuf_waitingsignal = 0;
#ifndef UNDER_CE
                    pthread_mutex_lock(&pComponentPrivate->AlloBuf_mutex); 
                    pthread_cond_signal(&pComponentPrivate->AlloBuf_threshold);
                    pthread_mutex_unlock(&pComponentPrivate->AlloBuf_mutex);     
#endif
                }

                if (pComponentPrivate->curState == OMX_StateExecuting) {
                    char *p = "damedesuStr";
                    pComponentPrivate->bDspStoppedWhileExecuting = OMX_FALSE;
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               EMMCodecControlStart,(void *)p);
                }
                G711ENC_DPRINT("%d :: setting output port to enabled\n",__LINE__);
                pComponentPrivate->pPortDef[G711ENC_OUTPUT_PORT]->bEnabled = OMX_TRUE;
                G711ENC_DPRINT("pComponentPrivate->pPortDef[G711ENC_OUTPUT_PORT]->bEnabled = %d\n",pComponentPrivate->pPortDef[G711ENC_OUTPUT_PORT]->bEnabled);
            }
        }
        
        if(commandData == 0x0){
            if (pComponentPrivate->curState == OMX_StateLoaded ||
                pComponentPrivate->pPortDef[G711ENC_INPUT_PORT]->bPopulated){
                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,
                                                        OMX_CommandPortEnable,
                                                        G711ENC_INPUT_PORT,
                                                        NULL);
                G711ENC_DPRINT("%d :: setting Input port to enabled\n",__LINE__);
                pComponentPrivate->bEnableCommandPending = 0;
            }
            else {
                pComponentPrivate->bEnableCommandPending = 1;
                pComponentPrivate->nEnableCommandParam = commandData;
            }
        }
        else if(commandData == 0x1){
            if (pComponentPrivate->curState == OMX_StateLoaded || 
                pComponentPrivate->pPortDef[G711ENC_OUTPUT_PORT]->bPopulated){
                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,
                                                        OMX_CommandPortEnable,
                                                        G711ENC_OUTPUT_PORT,
                                                        NULL);
                G711ENC_DPRINT("%d :: setting output port to enabled\n",__LINE__);
                pComponentPrivate->bEnableCommandPending = 0;
            }
            
            else {
                pComponentPrivate->bEnableCommandPending = 1;
                pComponentPrivate->nEnableCommandParam = commandData;
            }
        }
        else if(commandData == -1){
            if (pComponentPrivate->curState == OMX_StateLoaded || 
                (pComponentPrivate->pPortDef[G711ENC_INPUT_PORT]->bPopulated
                 && pComponentPrivate->pPortDef[G711ENC_OUTPUT_PORT]->bPopulated)){
                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,
                                                        OMX_CommandPortEnable,
                                                        G711ENC_INPUT_PORT,
                                                        NULL);
                                                        
                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,
                                                        OMX_CommandPortEnable,
                                                        G711ENC_OUTPUT_PORT,
                                                        NULL);
                                                        
                G711ENC_FillLCMLInitParamsEx(pComponentPrivate->pHandle);
                G711ENC_DPRINT("%d :: setting Input & Output port to enabled\n",__LINE__);
                pComponentPrivate->bEnableCommandPending = 0;
            }
            else {
                pComponentPrivate->bEnableCommandPending = 1;
                pComponentPrivate->nEnableCommandParam = commandData;
            }
        }
#ifndef UNDER_CE
        pthread_mutex_lock(&pComponentPrivate->AlloBuf_mutex); 
        pthread_cond_signal(&pComponentPrivate->AlloBuf_threshold);
        pthread_mutex_unlock(&pComponentPrivate->AlloBuf_mutex);     
#endif

    }
    else if (command == OMX_CommandFlush) {
#if 0
        if(commandData == 0x0 || commandData == -1){
            for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
                pComponentPrivate->cbInfo.EmptyBufferDone (pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           pComponentPrivate->pInputBufferList->pBufHdr[i]);
                                                           
                pComponentPrivate->pInputBufHdrPending[i] = NULL;
            }
            pComponentPrivate->nNumInputBufPending=0;
            pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                    pHandle->pApplicationPrivate,
                                                    OMX_EventCmdComplete,
                                                    OMX_CommandFlush,
                                                    G711ENC_INPUT_PORT,
                                                    NULL);
        }
        if(commandData == 0x1 || commandData == -1){
            for (i=0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {
                pComponentPrivate->cbInfo.FillBufferDone (pHandle,
                                                          pHandle->pApplicationPrivate,
                                                          pComponentPrivate->pOutputBufferList->pBufHdr[i]);
                                                        
                pComponentPrivate->pOutputBufHdrPending[i] = NULL;
            }
            pComponentPrivate->nNumOutputBufPending=0;
            /* return all output buffers */
            pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                    pHandle->pApplicationPrivate,
                                                    OMX_EventCmdComplete,
                                                    OMX_CommandFlush,
                                                    G711ENC_OUTPUT_PORT,
                                                    NULL);
        }
#endif
        OMX_U32 aParam[3] = {0};
        if(commandData == 0x0 || commandData == -1) {
            if (pComponentPrivate->nUnhandledEmptyThisBuffers == 0)  {
                pComponentPrivate->bFlushInputPortCommandPending = OMX_FALSE;

                aParam[0] = USN_STRMCMD_FLUSH; 
                aParam[1] = 0x0; 
                aParam[2] = 0x0; 

                G711ENC_DPRINT("Flushing input port\n");
                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                           EMMCodecControlStrmCtrl, (void*)aParam);
                 
                if (eError != OMX_ErrorNone) {
                    goto EXIT;
                }
            }else {
                pComponentPrivate->bFlushInputPortCommandPending = OMX_TRUE;
            }
        }
        if(commandData == 0x1 || commandData == -1){
            if (pComponentPrivate->nUnhandledFillThisBuffers == 0)  {
                pComponentPrivate->bFlushOutputPortCommandPending = OMX_FALSE;

                aParam[0] = USN_STRMCMD_FLUSH; 
                aParam[1] = 0x1; 
                aParam[2] = 0x0; 

                G711ENC_DPRINT("Flushing output port\n");
                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                           EMMCodecControlStrmCtrl, (void*)aParam);
                
                if (eError != OMX_ErrorNone) {
                    goto EXIT;
                }
            } else {
                pComponentPrivate->bFlushOutputPortCommandPending = OMX_TRUE; 
            }
        }

    }

 EXIT:
    G711ENC_DPRINT("%d :: Exiting G711ENC_HandleCommand Function\n",__LINE__);
    G711ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}

/* ========================================================================== */
/**
 * @G711ENC_HandleDataBufFromApp() This function is called by the component when ever it
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
OMX_ERRORTYPE G711ENC_HandleDataBufFromApp(OMX_BUFFERHEADERTYPE* pBufHeader,
                                           G711ENC_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_DIRTYPE eDir;
    G711ENC_LCML_BUFHEADERTYPE *pLcmlHdr = NULL;
    
    OMX_U8 nFrames = 0,i = 0;
    LCML_DSP_INTERFACE * phandle = NULL;
    OMX_U8* pBufParmsTemp = NULL;

    OMX_U32 frameLength_in;
    OMX_U32 frameLength_out;

    LCML_DSP_INTERFACE *pLcmlHandle = (LCML_DSP_INTERFACE *) pComponentPrivate->pLcmlHandle;
    G711ENC_DPRINT ("%d :: Entering G711ENC_HandleDataBufFromApp Function\n",__LINE__);
    /*Find the direction of the received buffer from buffer list */

    eError = G711ENC_GetBufferDirection(pBufHeader, &eDir);
    if (eError != OMX_ErrorNone) {
        G711ENC_DPRINT ("%d :: The pBufHeader is not found in the list\n", __LINE__);
        goto EXIT;
    }

    if (eDir == OMX_DirInput) {
        if(pComponentPrivate->dasfMode == 0) {
            pComponentPrivate->nUnhandledEmptyThisBuffers--;
            if ((pBufHeader->nFilledLen > 0) || (pBufHeader->nFlags == OMX_BUFFERFLAG_EOS)) {
                pComponentPrivate->bBypassDSP = 0;          /* buffer is not  Empty  */
                eError = G711ENC_GetCorrespondingLCMLHeader(pComponentPrivate,
                                                            pBufHeader->pBuffer, 
                                                            OMX_DirInput, 
                                                            &pLcmlHdr);
                if (eError != OMX_ErrorNone) {
                    G711ENC_DPRINT("%d :: Error: Invalid Buffer Came ...\n",__LINE__);
                    goto EXIT;
                }
                switch (pComponentPrivate->frametype) {
                case 0:
                  frameLength_in = G711ENC_INPUT_FRAME_SIZE;
                  break;
                case 1:
                  frameLength_in = G711ENC_INPUT_FRAME_SIZE_20MS;
                  break;
                case 2:
                  frameLength_in = G711ENC_INPUT_FRAME_SIZE_30MS;
                  break;
                default:
                  frameLength_in = G711ENC_INPUT_FRAME_SIZE;
                  break;
                }

                nFrames = (OMX_U8)(pBufHeader->nFilledLen / frameLength_in);
                pComponentPrivate->nNumOfFramesSent = nFrames;
                phandle = (LCML_DSP_INTERFACE *)(((LCML_CODEC_INTERFACE *)pLcmlHandle->pCodecinterfacehandle)->pCodec);

                if( (pLcmlHdr->pBufferParam->usNbFrames < nFrames) && 
                    (pLcmlHdr->pFrameParam!=NULL) ){
                    /*This means that more memory need to be used*/
                    pBufParmsTemp = (OMX_U8*)pLcmlHdr->pFrameParam; 
                    pBufParmsTemp -=EXTRA_BYTES;
                    OMX_G711ENC_MEMFREE_STRUCT(pBufParmsTemp);
                    pLcmlHdr->pFrameParam = NULL;
                    pBufParmsTemp = NULL;
                    OMX_DmmUnMap(phandle->dspCodec->hProc, /*Unmap DSP memory used*/
                                 (void*)pLcmlHdr->pBufferParam->pParamElem,
                                 pLcmlHdr->pDmmBuf->pReserved);
                    pLcmlHdr->pBufferParam->pParamElem = NULL;
                }

                if(pLcmlHdr->pFrameParam == NULL ){
                    G711ENC_OMX_MALLOC_SIZE(pBufParmsTemp, (sizeof(G711ENC_FrameStruct)*nFrames) + DSP_CACHE_ALIGNMENT,OMX_U8);
                    pLcmlHdr->pFrameParam =  (G711ENC_FrameStruct*)(pBufParmsTemp + EXTRA_BYTES);
                    eError = OMX_DmmMap(phandle->dspCodec->hProc,
                                        nFrames*sizeof(G711ENC_FrameStruct),
                                        (void*)pLcmlHdr->pFrameParam,
                                        (pLcmlHdr->pDmmBuf));
                    if (eError != OMX_ErrorNone){
                        G711ENC_DPRINT("%d OMX_DmmMap ERRROR!!!!\n",__LINE__);
                        goto EXIT;
                    }
                    G711ENC_DPRINT("%d OMX_DmmMap Success first !!!!\n",__LINE__);
                    pLcmlHdr->pBufferParam->pParamElem = (G711ENC_FrameStruct *)pLcmlHdr->pDmmBuf->pMapped; /*DSP Address*/
                }

                for(i=0;i<nFrames;i++){
                    (pLcmlHdr->pFrameParam+i)->usLastFrame = 0;
                }

                if(pBufHeader->nFlags == OMX_BUFFERFLAG_EOS) {
                    (pLcmlHdr->pFrameParam+(nFrames-1))->usLastFrame = OMX_BUFFERFLAG_EOS;
                    pComponentPrivate->bIsEOFSent = 1;
                    pBufHeader->nFlags = 0;
                }
                pLcmlHdr->pBufferParam->usNbFrames = nFrames;
                /*Store tick count information*/
                pComponentPrivate->arrBufIndexTick[pComponentPrivate->IpBufindex] = pBufHeader->nTickCount;

                /* Store time stamp information */
                pComponentPrivate->arrBufIndex[pComponentPrivate->IpBufindex] = pBufHeader->nTimeStamp;
                pComponentPrivate->IpBufindex++;
                pComponentPrivate->IpBufindex %= pComponentPrivate->pPortDef[G711ENC_OUTPUT_PORT]->nBufferCountActual;
                G711ENC_DPRINT("%d :: Output Buffer TimeStamp %lld\n", __LINE__, 
                               pComponentPrivate->arrBufIndex[pComponentPrivate->IpBufindex]);

                if (pComponentPrivate->curState == OMX_StateExecuting) {
                    if(!pComponentPrivate->bDspStoppedWhileExecuting) {
                        if (!G711ENC_IsPending(pComponentPrivate,pBufHeader,OMX_DirInput)) {
                            G711ENC_SetPending(pComponentPrivate,pBufHeader,OMX_DirInput,__LINE__);

                            eError = LCML_QueueBuffer( pLcmlHandle->pCodecinterfacehandle,
                                                       EMMCodecInputBuffer,
                                                       (OMX_U8 *)pBufHeader->pBuffer,
                                                       pBufHeader->nAllocLen,
                                                       pBufHeader->nFilledLen,
                                                       (OMX_U8 *) pLcmlHdr->pBufferParam,
                                                       sizeof(G711ENC_ParamStruct),
                                                       NULL);
                                                       
                            if (eError != OMX_ErrorNone) {
                                eError = OMX_ErrorHardware;
                                goto EXIT;
                            }
                            pComponentPrivate->num_Sent_Ip_Buff++;
                            pComponentPrivate->lcml_nCntIp++;
                            pComponentPrivate->lcml_nIpBuf++;
                        }
                    }
                    else{
                        pComponentPrivate->cbInfo.EmptyBufferDone (pComponentPrivate->pHandle,
                                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                                   pBufHeader);
                    }
                }
                else if(pComponentPrivate->curState == OMX_StatePause) {
                    pComponentPrivate->pInputBufHdrPending[pComponentPrivate->nNumInputBufPending++] = pBufHeader;
                }
            }
            else {
                pComponentPrivate->bBypassDSP = 1;

                G711ENC_DPRINT("%d :: Calling EmptyBufferDone\n",__LINE__);
                pComponentPrivate->cbInfo.EmptyBufferDone( pComponentPrivate->pHandle,
                                                           pComponentPrivate->pHandle->pApplicationPrivate,
                                                           pComponentPrivate->pInputBufferList->pBufHdr[0]);
                                                           
                pComponentPrivate->nEmptyBufferDoneCount++;

            }
            if(pBufHeader->pMarkData){
                /* copy mark to output buffer header */
                pComponentPrivate->pOutputBufferList->pBufHdr[0]->pMarkData = pBufHeader->pMarkData;
                pComponentPrivate->pOutputBufferList->pBufHdr[0]->hMarkTargetComponent = pBufHeader->hMarkTargetComponent;
                /* trigger event handler if we are supposed to */
                if(pBufHeader->hMarkTargetComponent == pComponentPrivate->pHandle && pBufHeader->pMarkData){
                    
                    pComponentPrivate->cbInfo.EventHandler( pComponentPrivate->pHandle,
                                                            pComponentPrivate->pHandle->pApplicationPrivate,
                                                            OMX_EventMark, 0, 0,
                                                            pBufHeader->pMarkData);
                }
            }
        }
        if (pComponentPrivate->bFlushInputPortCommandPending) {
            OMX_SendCommand(pComponentPrivate->pHandle,
                            OMX_CommandFlush,
                            0,
                            NULL);
        }

    } 
    else if (eDir == OMX_DirOutput) {
        pComponentPrivate->nUnhandledFillThisBuffers--;
        nFrames = pComponentPrivate->nNumOfFramesSent;

        if(nFrames == 0)
            nFrames = 1;

        /*OutputFrames = pBufHeader->pOutputPortPrivate;
          OutputFrames->nFrames = nFrames;*/

        eError = G711ENC_GetCorrespondingLCMLHeader(pComponentPrivate,
                                                    pBufHeader->pBuffer, 
                                                    OMX_DirOutput, 
                                                    &pLcmlHdr);
        if (eError != OMX_ErrorNone) {
            G711ENC_DPRINT("%d :: Error: Invalid Buffer Came ...\n",__LINE__);
            goto EXIT;
        }

        switch (pComponentPrivate->frametype) {
        case 0:
          frameLength_out = G711ENC_OUTPUT_FRAME_SIZE;
          break;
        case 1:
          frameLength_out = G711ENC_OUTPUT_FRAME_SIZE_20MS;
          break;
        case 2:
          frameLength_out = G711ENC_OUTPUT_FRAME_SIZE_30MS;
          break;
        default:
          frameLength_out = G711ENC_OUTPUT_FRAME_SIZE;
          break;
        }

        phandle = (LCML_DSP_INTERFACE *)(((LCML_CODEC_INTERFACE *)pLcmlHandle->pCodecinterfacehandle)->pCodec);

        if( (pLcmlHdr->pBufferParam->usNbFrames < nFrames) && 
            (pLcmlHdr->pFrameParam!=NULL) ){
            /*This means that more memory need to be used*/
            pBufParmsTemp = (OMX_U8*)pLcmlHdr->pFrameParam; 
            pBufParmsTemp -=EXTRA_BYTES;
            OMX_G711ENC_MEMFREE_STRUCT(pBufParmsTemp);
            pLcmlHdr->pFrameParam = NULL;
            pBufParmsTemp = NULL;
#ifndef UNDER_CE
            OMX_DmmUnMap(phandle->dspCodec->hProc,
                         (void*)pLcmlHdr->pBufferParam->pParamElem,
                         pLcmlHdr->pDmmBuf->pReserved);
#endif
            pLcmlHdr->pBufferParam->pParamElem = NULL;
        }

        if(pLcmlHdr->pFrameParam==NULL ){
            G711ENC_OMX_MALLOC_SIZE(pBufParmsTemp, (sizeof(G711ENC_FrameStruct)*nFrames ) + DSP_CACHE_ALIGNMENT,OMX_U8);
            pLcmlHdr->pFrameParam =  (G711ENC_FrameStruct*)(pBufParmsTemp + EXTRA_BYTES);
            pLcmlHdr->pBufferParam->pParamElem = NULL;
#ifndef UNDER_CE
            eError = OMX_DmmMap(phandle->dspCodec->hProc,
                                nFrames*sizeof(G711ENC_FrameStruct),
                                (void*)pLcmlHdr->pFrameParam,
                                (pLcmlHdr->pDmmBuf));

            if (eError != OMX_ErrorNone){
                G711ENC_DPRINT("%d OMX_DmmMap ERRROR!!!!\n",__LINE__);
                goto EXIT;
            }
            G711ENC_DPRINT("%d OMX_DmmMap Success second!!!!\n",__LINE__);
            pLcmlHdr->pBufferParam->pParamElem = (G711ENC_FrameStruct *)pLcmlHdr->pDmmBuf->pMapped; /*DSP Address*/
#endif
        }

        pLcmlHdr->pBufferParam->usNbFrames = nFrames;

        if (pComponentPrivate->bBypassDSP == 0) {
            G711ENC_DPRINT ("%d: Sending Empty OUTPUT BUFFER to Codec = %p to get %d frames\n",
                            __LINE__,
                            pBufHeader->pBuffer, 
                            pLcmlHdr->pBufferParam->usNbFrames);
            if (pComponentPrivate->curState == OMX_StateExecuting) {
                if (!G711ENC_IsPending(pComponentPrivate,pBufHeader,OMX_DirOutput)) {
                    G711ENC_SetPending(pComponentPrivate,pBufHeader,OMX_DirOutput,__LINE__);
                    if (!pComponentPrivate->bDspStoppedWhileExecuting){

                        eError = LCML_QueueBuffer( pLcmlHandle->pCodecinterfacehandle,
                                                   EMMCodecOuputBuffer,
                                                   (OMX_U8 *)pBufHeader->pBuffer,
                                                   frameLength_out * nFrames,
                                                   0,
                                                   (OMX_U8 *) pLcmlHdr->pBufferParam,
                                                   sizeof(G711ENC_ParamStruct),
                                                   NULL);
                        if (eError != OMX_ErrorNone ) {
                            G711ENC_DPRINT ("%d :: IssuingDSP OP: Error Occurred\n",__LINE__);
                            eError = OMX_ErrorHardware;
                            goto EXIT;
                        }
                        pComponentPrivate->lcml_nOpBuf++;
                        pComponentPrivate->num_Op_Issued++;
                    }
                }
            } 
            else if(pComponentPrivate->curState == OMX_StatePause) {
                pComponentPrivate->pOutputBufHdrPending[pComponentPrivate->nNumOutputBufPending++] = pBufHeader;
            }
        }else{
            G711ENC_DPRINT("%d :: Calling FillBufferDone\n",__LINE__);
            pComponentPrivate->cbInfo.FillBufferDone( pComponentPrivate->pHandle,
                                                      pComponentPrivate->pHandle->pApplicationPrivate,
                                                      pComponentPrivate->pOutputBufferList->pBufHdr[0]);
                                                          
            pComponentPrivate->nFillBufferDoneCount++;
        }
        if (pComponentPrivate->bFlushOutputPortCommandPending) {
            OMX_SendCommand( pComponentPrivate->pHandle, 
                             OMX_CommandFlush, 
                             1, 
                             NULL);
        }
    } else {
        eError = OMX_ErrorBadParameter;
    }

 EXIT:
    G711ENC_DPRINT("%d :: Exiting from  G711ENC_HandleDataBufFromApp \n",__LINE__);
    G711ENC_DPRINT("%d :: Returning error %d\n",__LINE__,eError);
    return eError;
}

/*-------------------------------------------------------------------*/
/**
 * G711ENC_HandleDataBufFromLCML () This function is used by the component thread to
 * request a buffer from the application.  Since it was called from 2 places,
 * it made sense to turn this into a small function.
 *
 * @param pData pointer to G711 Decoder Context Structure
 * @param pCur pointer to the buffer to be requested to be filled
 *
 * @retval none
 **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE G711ENC_HandleDataBufFromLCML(G711ENC_COMPONENT_PRIVATE* pComponentPrivate, 
                                            G711ENC_LCML_BUFHEADERTYPE* msgBuffer)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE* pHandle = (OMX_COMPONENTTYPE*)pComponentPrivate->pHandle;
    G711ENC_DPRINT ("%d :: Entering G711ENC_HandleDataBufFromLCML Function\n",__LINE__);

    if(msgBuffer->eDir == OMX_DirInput) {
        pComponentPrivate->lcml_nIpBuf--;
        pComponentPrivate->cbInfo.EmptyBufferDone (pHandle,
                                                   pHandle->pApplicationPrivate,
                                                   msgBuffer->buffer);
        pComponentPrivate->app_nBuf++;
        pComponentPrivate->nEmptyBufferDoneCount++;
    }
    else if(msgBuffer->eDir == OMX_DirOutput) {
        pComponentPrivate->num_Reclaimed_Op_Buff++;
        if (pComponentPrivate->bIsEOFSent){
            msgBuffer->buffer->nFlags |= OMX_BUFFERFLAG_EOS;
            pComponentPrivate->bIsEOFSent = 0;
            pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                    pHandle->pApplicationPrivate,
                                                    OMX_EventBufferFlag,
                                                    msgBuffer->buffer->nOutputPortIndex,
                                                    OMX_BUFFERFLAG_EOS, 
                                                    NULL);
        }
        /*Copying tick count information to output buffer*/
        msgBuffer->buffer->nTickCount = (OMX_U32)pComponentPrivate->arrBufIndexTick[pComponentPrivate->OpBufindex];

        /* Copying time stamp information to output buffer */
        msgBuffer->buffer->nTimeStamp = (OMX_TICKS)pComponentPrivate->arrBufIndex[pComponentPrivate->OpBufindex];
        pComponentPrivate->OpBufindex++;
        pComponentPrivate->OpBufindex %= pComponentPrivate->pPortDef[G711ENC_OUTPUT_PORT]->nBufferCountActual;

        pComponentPrivate->cbInfo.FillBufferDone( pHandle,
                                                  pHandle->pApplicationPrivate,
                                                  msgBuffer->buffer);
        pComponentPrivate->lcml_nOpBuf--;
        pComponentPrivate->app_nBuf++;
        pComponentPrivate->nFillBufferDoneCount++;
        G711ENC_DPRINT("%d :: Incrementing app_nBuf = %ld\n",__LINE__,pComponentPrivate->app_nBuf);
        pComponentPrivate->nOutStandingFillDones--;
    }
    else {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }
 EXIT:
    G711ENC_DPRINT("%d :: Exiting G711ENC_HandleDataBufFromLCML Function\n",__LINE__);
    return eError;
}

/*-------------------------------------------------------------------*/
/**
 * G711ENC_GetBufferDirection () This function is used by the component
 * to get the direction of the buffer
 * @param eDir pointer will be updated with buffer direction
 * @param pBufHeader pointer to the buffer to be requested to be filled
 *
 * @retval none
 **/
/*-------------------------------------------------------------------*/

OMX_ERRORTYPE G711ENC_GetBufferDirection(OMX_BUFFERHEADERTYPE *pBufHeader,
                                         OMX_DIRTYPE *eDir)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    G711ENC_COMPONENT_PRIVATE *pComponentPrivate = pBufHeader->pPlatformPrivate;
    OMX_U32 nBuf = 0;
    OMX_BUFFERHEADERTYPE *pBuf = NULL;
    OMX_U32 flag = 1,i = 0;
    G711ENC_DPRINT("%d :: Entering G711ENC_GetBufferDirection Function\n",__LINE__);
    /*Search this buffer in input buffers list */
    nBuf = pComponentPrivate->pInputBufferList->numBuffers;
    for(i=0; i<nBuf; i++) {
        pBuf = pComponentPrivate->pInputBufferList->pBufHdr[i];
        if(pBufHeader == pBuf) {
            *eDir = OMX_DirInput;
            G711ENC_DPRINT("%d :: pBufHeader = %p is INPUT BUFFER pBuf = %p\n",__LINE__,pBufHeader,pBuf);
            flag = 0;
            goto EXIT;
        }
    }
    /*Search this buffer in output buffers list */
    nBuf = pComponentPrivate->pOutputBufferList->numBuffers;
    
    for(i=0; i<nBuf; i++) {
        pBuf = pComponentPrivate->pOutputBufferList->pBufHdr[i];
        if(pBufHeader == pBuf) {
            *eDir = OMX_DirOutput;
            G711ENC_DPRINT("%d :: pBufHeader = %p is OUTPUT BUFFER pBuf = %p\n",__LINE__,pBufHeader,pBuf);
            flag = 0;
            goto EXIT;
        }
    }

    if (flag == 1) {
        G711ENC_DPRINT("%d :: Buffer %p is Not Found in the List\n",__LINE__, pBufHeader);
        eError = OMX_ErrorUndefined;
        goto EXIT;
    }
    
 EXIT:
    G711ENC_DPRINT("%d :: Exiting G711ENC_GetBufferDirection Function\n",__LINE__);
    G711ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}

/* -------------------------------------------------------------------*/
/**
 * G711ENC_GetCorrespondingLCMLHeader() function will be called by LCML_Callback
 * component to write the msg
 * @param *pBuffer,          Event which gives to details about USN status
 * @param G711ENC_LCML_BUFHEADERTYPE **ppLcmlHdr
 * @param  OMX_DIRTYPE eDir this gives direction of the buffer
 * @retval OMX_NoError              Success, ready to roll
 *         OMX_Error_BadParameter   The input parameter pointer is null
 **/
/* -------------------------------------------------------------------*/
OMX_ERRORTYPE G711ENC_GetCorrespondingLCMLHeader(G711ENC_COMPONENT_PRIVATE *pComponentPrivate,
                                                 OMX_U8 *pBuffer,
                                                 OMX_DIRTYPE eDir,
                                                 G711ENC_LCML_BUFHEADERTYPE **ppLcmlHdr)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    G711ENC_LCML_BUFHEADERTYPE *pLcmlBufHeader = NULL;
    OMX_U32 i = 0,nIpBuf = 0,nOpBuf = 0;

    nIpBuf = pComponentPrivate->pInputBufferList->numBuffers;
    nOpBuf = pComponentPrivate->pOutputBufferList->numBuffers;
    G711ENC_DPRINT("%d :: Entering G711ENC_GetCorrespondingLCMLHeader..\n",__LINE__);

    while (!pComponentPrivate->bInitParamsInitialized) {
        G711ENC_DPRINT("%d :: Waiting for init to complete........\n",__LINE__);
#ifndef UNDER_CE
        sched_yield();
#else
        Sleep(0);
#endif
    }
    
    if(eDir == OMX_DirInput) {
        G711ENC_DPRINT("%d :: Entering OMX_DIRINPUT\n",__LINE__);
        G711ENC_DPRINT("%d :: Entering G711ENC_GetCorrespondingLCMLHeader..\n",__LINE__);
        pLcmlBufHeader = pComponentPrivate->pLcmlBufHeader[G711ENC_INPUT_PORT];
        
        for(i = 0; i < nIpBuf; i++) {
            G711ENC_DPRINT("%d :: pBuffer = %p\n",__LINE__,pBuffer);
            G711ENC_DPRINT("%d :: pLcmlBufHeader->buffer->pBuffer = %p\n",__LINE__,pLcmlBufHeader->buffer->pBuffer);
            
            if(pBuffer == pLcmlBufHeader->buffer->pBuffer) {
                *ppLcmlHdr = pLcmlBufHeader;
                G711ENC_DPRINT("%d :: Corresponding Input LCML Header Found = %p\n",__LINE__,pLcmlBufHeader);
                eError = OMX_ErrorNone;
                goto EXIT;
            }
            
            pLcmlBufHeader++;
        }
        
    } else if (eDir == OMX_DirOutput) {
        G711ENC_DPRINT("%d :: Entering OMX_DIROUTPUT \n",__LINE__);
        pLcmlBufHeader = pComponentPrivate->pLcmlBufHeader[G711ENC_OUTPUT_PORT];
        
        for(i = 0; i < nOpBuf; i++) {
            G711ENC_DPRINT("%d :: pBuffer = %p\n",__LINE__,pBuffer);
            G711ENC_DPRINT("%d :: pLcmlBufHeader->buffer->pBuffer = %p\n",__LINE__,pLcmlBufHeader->buffer->pBuffer);
            
            if(pBuffer == pLcmlBufHeader->buffer->pBuffer) {
                *ppLcmlHdr = pLcmlBufHeader;
                G711ENC_DPRINT("%d :: Corresponding Output LCML Header Found = %p\n",__LINE__,pLcmlBufHeader);
                eError = OMX_ErrorNone;
                goto EXIT;
            }
            
            pLcmlBufHeader++;
        }
        
    } else {
        G711ENC_DPRINT("%d :: Invalid Buffer Type :: exiting...\n",__LINE__);
        eError = OMX_ErrorUndefined;
    }

 EXIT:
    G711ENC_DPRINT("%d :: Exiting G711ENC_GetCorrespondingLCMLHeader..\n",__LINE__);
    G711ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}

/* -------------------------------------------------------------------*/
/**
 *  G711ENC_LCMLCallback() will be called LCML component to write the msg
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
OMX_ERRORTYPE G711ENC_LCMLCallback (TUsnCodecEvent event,void * args[10])
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U8 *pBuffer = args[1];
    G711ENC_LCML_BUFHEADERTYPE *pLcmlHdr = NULL;
    OMX_U16 i = 0;

    G711ENC_COMPONENT_PRIVATE* pComponentPrivate = NULL;
    pComponentPrivate = (G711ENC_COMPONENT_PRIVATE*)((LCML_DSP_INTERFACE *)args[6])->pComponentPrivate; 
    
    G711ENC_DPRINT("%d :: Entering the G711ENC_LCMLCallback Function\n",__LINE__);

    switch(event) {

    case EMMCodecDspError:
        G711ENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecDspError\n");
        break;

    case EMMCodecInternalError:
        G711ENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecInternalError\n");
        break;

    case EMMCodecInitError:
        G711ENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecInitError\n");
        break;

    case EMMCodecDspMessageRecieved:
        G711ENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecDspMessageRecieved\n");
        break;

    case EMMCodecBufferProcessed:
        G711ENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecBufferProcessed\n");
        break;

    case EMMCodecProcessingStarted:
        G711ENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecProcessingStarted\n");
        break;

    case EMMCodecProcessingPaused:
        G711ENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecProcessingPaused\n");
        break;

    case EMMCodecProcessingStoped:
        G711ENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecProcessingStoped\n");
        break;

    case EMMCodecProcessingEof:
        G711ENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecProcessingEof\n");
        break;

    case EMMCodecBufferNotProcessed:
        G711ENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecBufferNotProcessed\n");
        break;

    case EMMCodecAlgCtrlAck:
        G711ENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecAlgCtrlAck\n");
        break;

    case EMMCodecStrmCtrlAck:
        G711ENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecStrmCtrlAck\n");
        break;
    }

    if(event == EMMCodecBufferProcessed){
        if((OMX_U32)args[0] == EMMCodecInputBuffer) {
            G711ENC_DPRINT("%d :: INPUT: pBuffer = %p\n",__LINE__, pBuffer);
            eError = G711ENC_GetCorrespondingLCMLHeader(pComponentPrivate,pBuffer, OMX_DirInput, &pLcmlHdr);
            if (eError != OMX_ErrorNone) {
                G711ENC_DPRINT("%d :: Error: Invalid Buffer Came ...\n",__LINE__);
                goto EXIT;
            }
            G711ENC_ClearPending(pComponentPrivate,pLcmlHdr->buffer,OMX_DirInput,__LINE__);
            eError =  G711ENC_HandleDataBufFromLCML(pComponentPrivate, pLcmlHdr);
            if (eError != OMX_ErrorNone) {
                G711ENC_DPRINT("%d :: Error in Sending Buffer to App\n", __LINE__);
                eError = OMX_ErrorHardware;
                goto EXIT;
            }
        } else if((OMX_U32)args[0] == EMMCodecOuputBuffer) {
            G711ENC_DPRINT("%d :: OUTPUT: pBuffer = %p %d\n",__LINE__, pBuffer);
            pComponentPrivate->nOutStandingFillDones++;
            eError = G711ENC_GetCorrespondingLCMLHeader(pComponentPrivate,pBuffer, OMX_DirOutput, &pLcmlHdr);
            if (eError != OMX_ErrorNone) {
                G711ENC_DPRINT("%d :: Error: Invalid Buffer Came ...\n",__LINE__);
                goto EXIT;
            }
            G711ENC_DPRINT("%d :: Output: pLcmlHdr->buffer->pBuffer = %p\n",__LINE__, pLcmlHdr->buffer->pBuffer);
            pLcmlHdr->buffer->nFilledLen = (OMX_U32)args[8];
            G711ENC_DPRINT("%d :: Output: pBuffer = %ld or %ld\n",__LINE__, pLcmlHdr->buffer->nFilledLen, args[2]);
            pComponentPrivate->lcml_nCntOpReceived++;

            G711ENC_ClearPending(pComponentPrivate,pLcmlHdr->buffer,OMX_DirOutput,__LINE__);
            eError =  G711ENC_HandleDataBufFromLCML(pComponentPrivate, pLcmlHdr);
            if (eError != OMX_ErrorNone) {
                G711ENC_DPRINT("%d :: Error in Sending Buffer to App\n", __LINE__);
                eError = OMX_ErrorHardware;
                goto EXIT;
            }
            
            if(!pLcmlHdr->pBufferParam->usNbFrames){
                pLcmlHdr->pBufferParam->usNbFrames++;
            }
        }
    }
    else if (event == EMMCodecStrmCtrlAck) {
        G711ENC_DPRINT("%d :: GOT MESSAGE USN_DSPACK_STRMCTRL \n",__LINE__);
        if (args[1] == (void *)USN_STRMCMD_FLUSH) {
            if ( args[2] == (void *)EMMCodecInputBuffer) {
                if (args[0] == (void *)USN_ERR_NONE ) {
                    G711ENC_DPRINT("Flushing input port %d\n",__LINE__);
                    for (i=0; i < pComponentPrivate->nNumInputBufPending; i++) {
                        pComponentPrivate->cbInfo.EmptyBufferDone (pComponentPrivate->pHandle,
                                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                                   pComponentPrivate->pInputBufHdrPending[i]);
                        pComponentPrivate->pInputBufHdrPending[i] = NULL;
                    }
                    pComponentPrivate->nNumInputBufPending=0;
                    pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle, 
                                                           pComponentPrivate->pHandle->pApplicationPrivate,
                                                           OMX_EventCmdComplete, 
                                                           OMX_CommandFlush,
                                                           G711ENC_INPUT_PORT, 
                                                           NULL);    
                } else {
                    G711ENC_DPRINT ("LCML reported error while flushing input port\n");
                    goto EXIT;                            
                }
            }
            else if ( args[2] == (void *)EMMCodecOuputBuffer) { 
                if (args[0] == (void *)USN_ERR_NONE ) {                      
                    G711ENC_DPRINT("Flushing output port %d\n",__LINE__);
                    for (i=0; i < pComponentPrivate->nNumOutputBufPending; i++) {
                        pComponentPrivate->cbInfo.FillBufferDone (pComponentPrivate->pHandle,
                                                                  pComponentPrivate->pHandle->pApplicationPrivate,
                                                                  pComponentPrivate->pOutputBufHdrPending[i]
                                                                  );
                        pComponentPrivate->nOutStandingFillDones--;
                        pComponentPrivate->pOutputBufHdrPending[i] = NULL;
                    }
                    pComponentPrivate->nNumOutputBufPending=0;
                    pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle, 
                                                           pComponentPrivate->pHandle->pApplicationPrivate,
                                                           OMX_EventCmdComplete, 
                                                           OMX_CommandFlush,
                                                           G711ENC_OUTPUT_PORT, 
                                                           NULL);
                } else {
                    G711ENC_DPRINT("LCML reported error while flushing output port\n");
                    goto EXIT;                            
                }
            }
        }       

    }
    else if(event == EMMCodecProcessingStoped) {
        G711ENC_DPRINT("%d :: GOT MESSAGE USN_DSPACK_STOP \n",__LINE__);
        for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
            if (pComponentPrivate->pInputBufferList->bBufferPending[i]) {
                pComponentPrivate->cbInfo.EmptyBufferDone (pComponentPrivate->pHandle,
                                                           pComponentPrivate->pHandle->pApplicationPrivate,
                                                           pComponentPrivate->pInputBufferList->pBufHdr[i]);
                G711ENC_ClearPending(pComponentPrivate, 
                                     pComponentPrivate->pInputBufferList->pBufHdr[i], 
                                     OMX_DirInput,
                                     __LINE__);
            }
        }

        for (i=0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {

            if (pComponentPrivate->pOutputBufferList->bBufferPending[i]) {
                pComponentPrivate->cbInfo.FillBufferDone (pComponentPrivate->pHandle,
                                                          pComponentPrivate->pHandle->pApplicationPrivate,
                                                          pComponentPrivate->pOutputBufferList->pBufHdr[i]);

                G711ENC_ClearPending(pComponentPrivate, 
                                     pComponentPrivate->pOutputBufferList->pBufHdr[i], 
                                     OMX_DirOutput,
                                     __LINE__);
            }
        }
        if (!pComponentPrivate->bNoIdleOnStop) {
            pComponentPrivate->curState = OMX_StateIdle;
#ifdef RESOURCE_MANAGER_ENABLED
            eError = RMProxy_NewSendCommand(pComponentPrivate->pHandle,
                                            RMProxy_StateSet,
                                            OMX_G711_Encoder_COMPONENT,
                                            OMX_StateIdle, 
                                            1234, 
                                            NULL);
#endif
            if(pComponentPrivate->bPreempted == 0) {
                pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandStateSet,
                                                       pComponentPrivate->curState,
                                                       NULL);
            }else{
                pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                                       OMX_EventError,
                                                       OMX_ErrorResourcesPreempted,
                                                       0,
                                                       NULL);

            }
        }
        else {
            pComponentPrivate->bDspStoppedWhileExecuting = OMX_TRUE;
            pComponentPrivate->bNoIdleOnStop= OMX_FALSE;
        }
        
    }
    else if(event == EMMCodecDspMessageRecieved) {
        G711ENC_DPRINT("%d :: commandedState  = %ld\n",__LINE__,(OMX_U32)args[0]);
        G711ENC_DPRINT("%d :: arg1 = %ld\n",__LINE__,(OMX_U32)args[1]);
        G711ENC_DPRINT("%d :: arg2 = %ld\n",__LINE__,(OMX_U32)args[2]);

        if(0x0500 == (OMX_U32)args[2]) {
            G711ENC_DPRINT("%d :: EMMCodecDspMessageRecieved\n",__LINE__);
        }
    }
    else if(event == EMMCodecAlgCtrlAck) {
        G711ENC_DPRINT("%d :: GOT MESSAGE USN_DSPACK_ALGCTRL \n",__LINE__);
    }
    else if (event == EMMCodecDspError) {
        if(((int)args[4] == USN_ERR_WARNING) && ((int)args[5] == IUALG_WARN_PLAYCOMPLETED)) {
            G711ENC_DPRINT("%d :: GOT MESSAGE IUALG_WARN_PLAYCOMPLETED\n",__LINE__);
            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                   OMX_EventBufferFlag,
                                                   (OMX_U32)NULL,
                                                   OMX_BUFFERFLAG_EOS, 
                                                   NULL);
        }
    }

    /* Since the event is called by SN,  buffers must have returned, so we can call the EventHandler for app */
    else if (event == EMMCodecProcessingPaused){
        pComponentPrivate->nUnhandledFillThisBuffers = 0;
        pComponentPrivate->nUnhandledEmptyThisBuffers = 0;

        pComponentPrivate->curState = OMX_StatePause;
        pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                               pComponentPrivate->pHandle->pApplicationPrivate,
                                               OMX_EventCmdComplete,
                                               OMX_CommandStateSet,
                                               pComponentPrivate->curState,
                                               NULL);
    }
        
    
 EXIT:
    G711ENC_DPRINT("%d :: Exiting the G711ENC_LCMLCallback Function\n",__LINE__);
    G711ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}

/* ================================================================================= */
/**
 *  G711ENC_GetLCMLHandle()
 *
 * @retval OMX_HANDLETYPE
 */
/* ================================================================================= */
#ifndef UNDER_CE
OMX_HANDLETYPE G711ENC_GetLCMLHandle(G711ENC_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_ERRORTYPE (*fpGetHandle)(OMX_HANDLETYPE);
    OMX_HANDLETYPE pHandle = NULL;
    void *handle = NULL;
    char *error = NULL;
    G711ENC_DPRINT("%d :: Entering G711ENC_GetLCMLHandle..\n",__LINE__);
    handle = dlopen("libLCML.so", RTLD_LAZY);
    if (!handle) {
        fputs(dlerror(), stderr);
        goto EXIT;
    }
    fpGetHandle = dlsym (handle, "GetHandle");
    if ((error = dlerror()) != NULL) {
        fputs(error, stderr);
        goto EXIT;
    }
    eError = (*fpGetHandle)(&pHandle);
    if(eError != OMX_ErrorNone) {
        eError = OMX_ErrorUndefined;
        G711ENC_DPRINT("%d :: OMX_ErrorUndefined...\n",__LINE__);
        pHandle = NULL;
        goto EXIT;
    }
    ((LCML_DSP_INTERFACE*)pHandle)->pComponentPrivate= pComponentPrivate;
    pComponentPrivate->ptrLibLCML = handle; /* saving LCML  backup lib pointer  */
    
 EXIT:
    G711ENC_DPRINT("%d :: Exiting G711ENC_GetLCMLHandle..\n",__LINE__);
    G711ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
    return pHandle;
}
/*WINDOWS Explicit dll load procedure*/
#else
/*WINDOWS Explicit dll load procedure*/
OMX_HANDLETYPE G711ENC_GetLCMLHandle()
{
    typedef OMX_ERRORTYPE (*LPFNDLLFUNC1)(OMX_HANDLETYPE);
    OMX_HANDLETYPE pHandle = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    HINSTANCE hDLL;               // Handle to DLL
    LPFNDLLFUNC1 fpGetHandle1;
    hDLL = LoadLibraryEx(TEXT("OAF_BML.dll"), NULL,0);
    if (hDLL == NULL) {
        //fputs(dlerror(), stderr);
        G711ENC_DPRINT("BML Load Failed!!!\n");
        return pHandle;
    }
    fpGetHandle1 = (LPFNDLLFUNC1)GetProcAddress(hDLL,TEXT("GetHandle"));
    if (!fpGetHandle1) {
        // handle the error
        FreeLibrary(hDLL);
        return pHandle;
    }
    // call the function
    eError = fpGetHandle1(&pHandle);
    if(eError != OMX_ErrorNone) {
        eError = OMX_ErrorUndefined;
        G711ENC_DPRINT("eError != OMX_ErrorNone...\n");
        pHandle = NULL;
        return pHandle;
    }
    return pHandle;
}
#endif

/* ================================================================================= */
/**
 * @fn G711ENC_SetPending() description for G711ENC_SetPending
 G711ENC_SetPending().
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
void G711ENC_SetPending(G711ENC_COMPONENT_PRIVATE *pComponentPrivate,
                        OMX_BUFFERHEADERTYPE *pBufHdr, 
                        OMX_DIRTYPE eDir, OMX_U32 lineNumber)
{
    OMX_U16 i = 0;

    if (eDir == OMX_DirInput) {
        for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pInputBufferList->pBufHdr[i]) {
                pComponentPrivate->pInputBufferList->bBufferPending[i] = 1;
                G711ENC_DPRINT("****INPUT BUFFER %d IS PENDING Line %ld******\n",i,lineNumber);
            }
        }
    }
    else {
        for (i=0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pOutputBufferList->pBufHdr[i]) {
                pComponentPrivate->pOutputBufferList->bBufferPending[i] = 1;
                G711ENC_DPRINT("****OUTPUT BUFFER %d IS PENDING Line %ld*****\n",i,lineNumber);
            }
        }
    }
}
/* ================================================================================= */
/**
 * @fn G711ENC_ClearPending() description for G711ENC_ClearPending
 G711ENC_ClearPending().
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
void G711ENC_ClearPending(G711ENC_COMPONENT_PRIVATE *pComponentPrivate,
                          OMX_BUFFERHEADERTYPE *pBufHdr, 
                          OMX_DIRTYPE eDir, OMX_U32 lineNumber)
{
    OMX_U16 i = 0;

    if (eDir == OMX_DirInput) {
        for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pInputBufferList->pBufHdr[i]) {
                pComponentPrivate->pInputBufferList->bBufferPending[i] = 0;
                G711ENC_DPRINT("****INPUT BUFFER %d IS RECLAIMED Line %ld*****\n",i,lineNumber);
            }
        }
    }
    else {
        for (i=0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pOutputBufferList->pBufHdr[i]) {
                pComponentPrivate->pOutputBufferList->bBufferPending[i] = 0;
                G711ENC_DPRINT("****OUTPUT BUFFER %d IS RECLAIMED Line %ld*****\n",i,lineNumber);
            }
        }
    }
}
/* ================================================================================= */
/**
 * @fn G711ENC_IsPending() description for G711ENC_IsPending
 G711ENC_IsPending().
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
OMX_U32 G711ENC_IsPending(G711ENC_COMPONENT_PRIVATE *pComponentPrivate,
                          OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir)
{
    OMX_U16 i = 0;

    if (eDir == OMX_DirInput) {
        for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pInputBufferList->pBufHdr[i]) {
                return pComponentPrivate->pInputBufferList->bBufferPending[i];
            }
        }
    }
    else {
        for (i=0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pOutputBufferList->pBufHdr[i]) {
                return pComponentPrivate->pOutputBufferList->bBufferPending[i];
            }
        }
    }
    return -1;
}
/* ================================================================================= */
/**
 * @fn G711ENC_IsValid() description for G711ENC_IsValid
 G711ENC_IsValid().
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
OMX_U32 G711ENC_IsValid(G711ENC_COMPONENT_PRIVATE *pComponentPrivate,
                        OMX_U8 *pBuffer, OMX_DIRTYPE eDir)
{
    OMX_U16 i = 0;
    OMX_U32 found=0;

    if (eDir == OMX_DirInput) {
        for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
            if (pBuffer == pComponentPrivate->pInputBufferList->pBufHdr[i]->pBuffer) {
                found = 1;
            }
        }
    }
    else {
        for (i=0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {
            if (pBuffer == pComponentPrivate->pOutputBufferList->pBufHdr[i]->pBuffer) {
                found = 1;
            }
        }
    }
    return found;
}
/* ========================================================================== */
/**
 * @G711ENC_FillLCMLInitParamsEx() This function is used by the component thread to
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
OMX_ERRORTYPE G711ENC_FillLCMLInitParamsEx(OMX_HANDLETYPE pComponent)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nIpBuf = 0,nIpBufSize = 0,nOpBuf = 0,nOpBufSize = 0;
    OMX_BUFFERHEADERTYPE *pTemp = NULL;
    LCML_DSP_INTERFACE *pHandle = (LCML_DSP_INTERFACE *)pComponent;
    G711ENC_COMPONENT_PRIVATE *pComponentPrivate = pHandle->pComponentPrivate;
    G711ENC_LCML_BUFHEADERTYPE *pTemp_lcml = NULL;
    OMX_U32 i = 0;
    OMX_U8  *pBufferParamTemp = NULL;
    OMX_U32 size_lcml = 0;
    G711ENC_DPRINT("%d :: G711ENC_FillLCMLInitParamsEx\n",__LINE__);
    nIpBuf = pComponentPrivate->pInputBufferList->numBuffers;
    nIpBufSize = pComponentPrivate->pPortDef[G711ENC_INPUT_PORT]->nBufferSize;
    nOpBuf = pComponentPrivate->pOutputBufferList->numBuffers;
    nOpBufSize = pComponentPrivate->pPortDef[G711ENC_OUTPUT_PORT]->nBufferSize;
    G711ENC_DPRINT("%d :: ------ Buffer Details -----------\n",__LINE__);
    G711ENC_DPRINT("%d :: Input  Buffer Count = %ld\n",__LINE__,nIpBuf);
    G711ENC_DPRINT("%d :: Input  Buffer Size = %ld\n",__LINE__,nIpBufSize);
    G711ENC_DPRINT("%d :: Output Buffer Count = %ld\n",__LINE__,nOpBuf);
    G711ENC_DPRINT("%d :: Output Buffer Size = %ld\n",__LINE__,nOpBufSize);
    G711ENC_DPRINT("%d :: ------ Buffer Details ------------\n",__LINE__);

    /* Allocate memory for all input buffer headers..
     * This memory pointer will be sent to LCML */
    size_lcml = nIpBuf * sizeof(G711ENC_LCML_BUFHEADERTYPE);
    G711ENC_OMX_MALLOC_SIZE(pTemp_lcml,size_lcml,G711ENC_LCML_BUFHEADERTYPE);

    pComponentPrivate->pLcmlBufHeader[G711ENC_INPUT_PORT] = pTemp_lcml;
    
    for (i=0; i<nIpBuf; i++) {
        G711ENC_DPRINT("%d :: INPUT--------- Inside Ip Loop\n",__LINE__);
        pTemp = pComponentPrivate->pInputBufferList->pBufHdr[i];
        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pTemp->nAllocLen = nIpBufSize;
        pTemp->nFilledLen = nIpBufSize;
        pTemp->nVersion.s.nVersionMajor = G711ENC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = G711ENC_MINOR_VER;
        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = G711ENC_NOT_USED;
        pTemp_lcml->buffer = pTemp;
        G711ENC_DPRINT("%d :: pTemp_lcml->buffer->pBuffer = %p \n",__LINE__,pTemp_lcml->buffer->pBuffer);
        pTemp_lcml->eDir = OMX_DirInput;
        G711ENC_OMX_MALLOC_STRUCT(pTemp_lcml->pIpParam, G711ENC_ParamStruct);
        /* pTemp_lcml->pIpParam->usEndOfFile = 0; */
        /* This means, it is not a last buffer. This flag is to be modified by
         * the application to indicate the last buffer */
         
        G711ENC_OMX_MALLOC_SIZE(pBufferParamTemp, sizeof(G711ENC_ParamStruct) + DSP_CACHE_ALIGNMENT,OMX_U8);
        pTemp_lcml->pBufferParam =  (G711ENC_ParamStruct*)(pBufferParamTemp + EXTRA_BYTES);

        pTemp_lcml->pBufferParam->usNbFrames=0;
        pTemp_lcml->pBufferParam->pParamElem=NULL;
        pTemp_lcml->pFrameParam=NULL;
        G711ENC_OMX_MALLOC_STRUCT(pTemp_lcml->pDmmBuf, DMM_BUFFER_OBJ);
         
        pTemp->nFlags = G711ENC_NORMAL_BUFFER;
        pTemp++;
        pTemp_lcml++;
    }

    /* Allocate memory for all output buffer headers..
     * This memory pointer will be sent to LCML */
    size_lcml = nOpBuf * sizeof(G711ENC_LCML_BUFHEADERTYPE);
    G711ENC_OMX_MALLOC_SIZE(pTemp_lcml,size_lcml,G711ENC_LCML_BUFHEADERTYPE);

    pComponentPrivate->pLcmlBufHeader[G711ENC_OUTPUT_PORT] = pTemp_lcml;
    
    for (i=0; i<nOpBuf; i++) {
        G711ENC_DPRINT("%d :: OUTPUT--------- Inside Op Loop\n",__LINE__);
        pTemp = pComponentPrivate->pOutputBufferList->pBufHdr[i];
        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pTemp->nAllocLen = nOpBufSize;
        pTemp->nFilledLen = nOpBufSize;
        pTemp->nVersion.s.nVersionMajor = G711ENC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = G711ENC_MINOR_VER;
        pComponentPrivate->nVersion = pTemp->nVersion.nVersion;
        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = G711ENC_NOT_USED;
        pTemp_lcml->buffer = pTemp;
        G711ENC_DPRINT("%d :: pTemp_lcml->buffer->pBuffer = %p \n",__LINE__,pTemp_lcml->buffer->pBuffer);
        pTemp_lcml->eDir = OMX_DirOutput;
        G711ENC_OMX_MALLOC_STRUCT(pTemp_lcml->pOpParam, G711ENC_UAlgOutBufParamStruct);
        pTemp_lcml->pOpParam->ulFrameCount = 0;
        
        G711ENC_OMX_MALLOC_STRUCT(pTemp_lcml->pBufferParam, G711ENC_ParamStruct);
        pTemp_lcml->pBufferParam->usNbFrames=0;
        pTemp_lcml->pBufferParam->pParamElem=NULL;
        pTemp_lcml->pFrameParam=NULL;
        G711ENC_OMX_MALLOC_STRUCT(pTemp_lcml->pDmmBuf, DMM_BUFFER_OBJ);
        
        /* This means, it is not a last buffer. This flag is to be modified by
         * the application to indicate the last buffer */
        pTemp->nFlags = G711ENC_NORMAL_BUFFER;
        pTemp++;
        pTemp_lcml++;
    }
    
    pComponentPrivate->bPortDefsAllocated = 1;
    pComponentPrivate->bInitParamsInitialized = 1;
    
 EXIT:
    G711ENC_DPRINT("%d :: Exiting G711ENC_FillLCMLInitParamsEx\n",__LINE__);
    G711ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}

/** ========================================================================
 *  OMX_DmmMap () method is used to allocate the memory using DMM.
 *
 *  @param ProcHandle -  Component identification number
 *  @param size  - Buffer header address, that needs to be sent to codec
 *  @param pArmPtr - Message used to send the buffer to codec
 *  @param pDmmBuf - buffer id
 *
 *  @retval OMX_ErrorNone  - Success
 *          OMX_ErrorHardware  -  Hardware Error
 ** ==========================================================================*/
OMX_ERRORTYPE OMX_DmmMap(DSP_HPROCESSOR ProcHandle, int size, void* pArmPtr,
                         DMM_BUFFER_OBJ* pDmmBuf)
{
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    DSP_STATUS status = DSP_SOK;
    int nSizeReserved = 0;

    if(pDmmBuf == NULL)
    {
        G711ENC_DPRINT("pBuf is NULL\n");
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    if(pArmPtr == NULL)
    {
        G711ENC_DPRINT("pBuf is NULL\n");
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    if (ProcHandle == NULL)
    {
        G711ENC_DPRINT ("ProcHandle is NULL\n");
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }
    
    /* Allocate */
    pDmmBuf->pAllocated = pArmPtr;
    /* Reserve */
    nSizeReserved = ROUND_TO_PAGESIZE(size) + 2*DMM_PAGE_SIZE ;
    status = DSPProcessor_ReserveMemory(ProcHandle, nSizeReserved, &(pDmmBuf->pReserved));
                          
    if(DSP_FAILED(status))
    {
        G711ENC_DPRINT("DSPProcessor_ReserveMemory() failed - error 0x%x", (int)status);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }
    pDmmBuf->nSize = size;
    G711ENC_DPRINT(" OMX DMM MAP Reserved: %p, size 0x%x (%d)\n", pDmmBuf->pReserved,nSizeReserved,nSizeReserved);
    
    /* Map */
    status = DSPProcessor_Map(ProcHandle,
                              pDmmBuf->pAllocated,/* Arm addres of data to Map on DSP*/
                              size , /* size to Map on DSP*/
                              pDmmBuf->pReserved, /* reserved space */
                              &(pDmmBuf->pMapped), /* returned map pointer */
                              0); /* final param is reserved.  set to zero. */
    
    if(DSP_FAILED(status))
    {
        G711ENC_DPRINT("DSPProcessor_Map() failed - error 0x%x", (int)status);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }
    G711ENC_DPRINT("OMX DMM Mapped: %p, size 0x%x (%d)\n",pDmmBuf->pMapped, size,size);

    /* Issue an initial memory flush to ensure cache coherency */
    status = DSPProcessor_FlushMemory(ProcHandle, pDmmBuf->pAllocated, size, 0);
    if(DSP_FAILED(status))
    {
        G711ENC_DPRINT("Unable to flush mapped buffer: error 0x%x",(int)status);
        goto EXIT;
    }
    eError = OMX_ErrorNone;
    
 EXIT:
    return eError;
}

/** ========================================================================
 *  OMX_DmmUnMap () method is used to de-allocate the memory using DMM.
 *
 *  @param ProcHandle -  Component identification number
 *  @param pMapPtr  - Map address
 *  @param pResPtr - reserve adress
 *
 *  @retval OMX_ErrorNone  - Success
 *          OMX_ErrorHardware  -  Hardware Error
 ** ==========================================================================*/
OMX_ERRORTYPE OMX_DmmUnMap(DSP_HPROCESSOR ProcHandle, void* pMapPtr, void* pResPtr)
{
    G711ENC_DPRINT("OMX_DmmUnMap %d\n",__LINE__);
    DSP_STATUS status = DSP_SOK;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    G711ENC_DPRINT("\nOMX UnReserve DSP: %p\n",pResPtr);

    if(pMapPtr == NULL)
    {
        G711ENC_DPRINT("pMapPtr is NULL\n");
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if(pResPtr == NULL)
    {
        G711ENC_DPRINT("pResPtr is NULL\n");
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if(ProcHandle == NULL)
    {
        G711ENC_DPRINT("--ProcHandle is NULL\n");
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    status = DSPProcessor_UnMap(ProcHandle,pMapPtr);
    if(DSP_FAILED(status))
    {
        G711ENC_DPRINT("DSPProcessor_UnMap() failed - error 0x%x",(int)status);
    }

    G711ENC_DPRINT("unreserving  structure =0x%p\n",pResPtr );
    status = DSPProcessor_UnReserveMemory(ProcHandle,pResPtr);
    if(DSP_FAILED(status))
    {
        G711ENC_DPRINT("DSPProcessor_UnReserveMemory() failed - error 0x%x", (int)status);
    }

 EXIT:
    return eError;
}

#ifdef RESOURCE_MANAGER_ENABLED
/***********************************
 *  Callback to the RM                                       *
 ***********************************/
void G711ENC_ResourceManagerCallback(RMPROXY_COMMANDDATATYPE cbData)
{
    OMX_COMMANDTYPE Cmd = OMX_CommandStateSet;
    OMX_STATETYPE state = OMX_StateIdle;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)cbData.hComponent;
    G711ENC_COMPONENT_PRIVATE *pCompPrivate = NULL;

    pCompPrivate = (G711ENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

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

