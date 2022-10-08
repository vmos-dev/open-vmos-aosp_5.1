
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
 * @file OMX_G729Dec_Utils.c
 *
 * This file implements OMX Component for G729 decoder that
 * is fully compliant with the OMX Audio specification .
 *
 * @path  $(OMAPSW_MPU)\linux\audio\src\openmax_il\g729_dec\src
 *
 * @rev  0.5
 */
/* ----------------------------------------------------------------------------- 
 *! 
 *! Revision History 
 *! ===================================
 *! Date         Author(s)            Version  Description
 *! ---------    -------------------  -------  ---------------------------------
 *! 03-Jan-2007  A.Donjon                         0.1      Code update for G729 DECODER
 *! 16-Feb-2007  A.Donjon                         0.2      Frame Lost
 *!                                                                                        Input buffer size used for buffer check in SN
 *! 01-Mar-2007  A.Donjon                         0.3      RM, DVFS changes 
 *! 08-Jun-2007  A.Donjon                         0.4      Variable input buffer size
 *! 04-Jul-2007  A.Donjon                         0.5      Last output frame reset for repeated play wo deleting component               
 *! 
 *!
 * ================================================================================= */

/* ------compilation control switches -------------------------*/

/****************************************************************
 *  INCLUDE FILES
 ****************************************************************/
/* ----- system and platform files ----------------------------*/
#ifdef UNDER_CE 
#include <windows.h>
#include <oaf_osal.h>
#include <omx_core.h>
#include <stdlib.h>
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
#include <dlfcn.h>
#endif
#include <dbapi.h>
#include <string.h>
#include <stdio.h>
/*-------program files ----------------------------------------*/
#include "OMX_G729Decoder.h"
#include "OMX_G729Dec_Utils.h"
#include "g729decsocket_ti.h"
#include "decode_common_ti.h"
#include "OMX_G729Dec_ComponentThread.h"
#include "usn.h"
#ifdef __PERF_INSTRUMENTATION__
#include "perf.h"
#endif

/* ======================================================================= */
/**
 * @def    DASF    Defines the value for identify DASF ON
 */
/* ======================================================================= */
#define DASF 1

#ifdef UNDER_CE
#define HASHINGENABLE 1
HINSTANCE g_hLcmlDllHandle = NULL;

#endif

static G729DEC_COMPONENT_PRIVATE *pComponentPrivate_CC; 
/* ========================================================================== */
/**
 * @G729DECFill_LCMLInitParams () This function is used by the component thread to
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

OMX_ERRORTYPE G729DECFill_LCMLInitParams(OMX_HANDLETYPE pComponent,
                                         LCML_DSP *plcml_Init, OMX_U16 arr[])
{

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nIpBuf=0,nIpBufSize=0,nOpBuf=0,nOpBufSize=0;
    OMX_U32 i=0;
    OMX_BUFFERHEADERTYPE *pTemp = NULL;
    OMX_S16 size_lcml=0;
    LCML_STRMATTR *strmAttr = NULL;

    LCML_DSP_INTERFACE *pHandle = (LCML_DSP_INTERFACE *)pComponent;
    G729DEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    LCML_G729DEC_BUFHEADERTYPE *pTemp_lcml = NULL;

    G729DEC_DPRINT("%d :: G729DECFill_LCMLInitParams\n ",__LINE__);
    G729DEC_DPRINT("%d :: pHandle = %p\n",__LINE__,pHandle);
    G729DEC_DPRINT("%d :: pHandle->pComponentPrivate = %p\n",__LINE__,pHandle->pComponentPrivate);
    pComponentPrivate = pHandle->pComponentPrivate;

    pComponentPrivate->bufParamsArray = malloc((10 * sizeof(unsigned long int)) + 256);
    pComponentPrivate->bufParamsArray += 128;
    memset(pComponentPrivate->bufParamsArray, 0, 9 * sizeof(unsigned long int));
    nIpBuf = pComponentPrivate->pInputBufferList->numBuffers;
    pComponentPrivate->nRuntimeInputBuffers = nIpBuf;
    nOpBuf = pComponentPrivate->pOutputBufferList->numBuffers;
    pComponentPrivate->nRuntimeOutputBuffers = nOpBuf;
    nIpBufSize = pComponentPrivate->pPortDef[G729DEC_INPUT_PORT]->nBufferSize;
    nOpBuf = pComponentPrivate->pOutputBufferList->numBuffers;
    nOpBufSize = pComponentPrivate->pPortDef[G729DEC_OUTPUT_PORT]->nBufferSize; 

    G729DEC_DPRINT("------ Buffer Details -----------\n");
    G729DEC_DPRINT("Input  Buffer Count = %ld\n", nIpBuf);
    G729DEC_DPRINT("Input  Buffer Size = %ld\n", nIpBufSize);
    G729DEC_DPRINT("Output Buffer Count = %ld\n", nOpBuf);
    G729DEC_DPRINT("Output Buffer Size = %ld\n", nOpBufSize);
    G729DEC_DPRINT("------ Buffer Details ------------\n");
    
    /* Fill Input Buffers Info for LCML */
    plcml_Init->In_BufInfo.nBuffers = nIpBuf;
    plcml_Init->In_BufInfo.nSize = nIpBufSize;
    plcml_Init->In_BufInfo.DataTrMethod = DMM_METHOD;


    /* Fill Output Buffers Info for LCML */
    plcml_Init->Out_BufInfo.nBuffers = nOpBuf;
    plcml_Init->Out_BufInfo.nSize = nOpBufSize;
    plcml_Init->Out_BufInfo.DataTrMethod = DMM_METHOD;

    /*Copy the node information */
    plcml_Init->NodeInfo.nNumOfDLLs = 3;
    
    plcml_Init->NodeInfo.AllUUIDs[0].uuid = &G729DECSOCKET_TI_UUID;
    strcpy ((char *)plcml_Init->NodeInfo.AllUUIDs[0].DllName,G729DEC_DLL_NAME);
    plcml_Init->NodeInfo.AllUUIDs[0].eDllType = DLL_NODEOBJECT;
    
    plcml_Init->NodeInfo.AllUUIDs[1].uuid = &G729DECSOCKET_TI_UUID;
    strcpy ((char *)plcml_Init->NodeInfo.AllUUIDs[1].DllName,G729DEC_DLL_NAME);
    plcml_Init->NodeInfo.AllUUIDs[1].eDllType = DLL_DEPENDENT;
    
    plcml_Init->NodeInfo.AllUUIDs[2].uuid = &DECODE_COMMON_TI_UUID;
    strcpy ((char *)plcml_Init->NodeInfo.AllUUIDs[2].DllName,G729DEC_USN_DLL_NAME);
    plcml_Init->NodeInfo.AllUUIDs[2].eDllType = DLL_DEPENDENT;


    if(pComponentPrivate->dasfmode == 1) {
        G729DEC_DPRINT("pComponentPrivate->dasfmode = %d\n",pComponentPrivate->dasfmode);
        OMX_G729MALLOC_STRUCT(strmAttr, LCML_STRMATTR);

        pComponentPrivate->strmAttr = strmAttr;
        G729DEC_MEMPRINT("%d:[ALLOC] %p\n",__LINE__,strmAttr);

        strmAttr->uSegid = 0;
        strmAttr->uAlignment = 0;
        strmAttr->uTimeout = G729D_TIMEOUT;
        strmAttr->uBufsize = nOpBufSize;
        strmAttr->uNumBufs = 2;
        strmAttr->lMode = STRMMODE_PROCCOPY;
        plcml_Init->DeviceInfo.TypeofDevice =1;
        plcml_Init->DeviceInfo.TypeofRender =0;
        if(pComponentPrivate->acdnmode == 1)
        {
            /* DASF/TeeDN mode */
            plcml_Init->DeviceInfo.AllUUIDs[0].uuid = &ACDN_TI_UUID;
        }
        else
        {
            /* ACDN mode */
            plcml_Init->DeviceInfo.AllUUIDs[0].uuid = &DCTN_TI_UUID;
        }
        plcml_Init->DeviceInfo.DspStream = strmAttr;
    }
    else {
        pComponentPrivate->strmAttr = NULL;
    }


    /*copy the other information */
    plcml_Init->SegID = OMX_G729DEC_DEFAULT_SEGMENT;
    plcml_Init->Timeout = OMX_G729DEC_SN_TIMEOUT;
    plcml_Init->Alignment = 0;
    plcml_Init->Priority = OMX_G729DEC_SN_PRIORITY;
    plcml_Init->ProfileID = -1;


    /* TODO: Set this using SetParameter() */
    pComponentPrivate->iG729SamplingFrequeny = G729DEC_SAMPLING_FREQUENCY;

    /*Accessing these 2 has the problem/creates problem in state transition tests*/
    pComponentPrivate->iG729Channels =
        (OMX_U16)pComponentPrivate->pcmParams->nChannels;

    /* Set G729 SN create phase arguments */

    arr[0] = STREAM_COUNT;
    arr[1] = G729DEC_INPUT_PORT;
    arr[2] = G729DEC_DMM;
    if (pComponentPrivate->pInputBufferList->numBuffers) {
        arr[3] = (OMX_U16)pComponentPrivate->pInputBufferList->numBuffers;

    }
    else {
        arr[3] = 1;
    }

    arr[4] = G729DEC_OUTPUT_PORT;

    if(pComponentPrivate->dasfmode == 1) {
        G729DEC_DPRINT("Setting up create phase params for DASF mode\n");
        arr[5] = G729DEC_OUTSTRM;
        arr[6] = NUM_G729DEC_OUTPUT_BUFFERS_DASF;
    }
    else {

        G729DEC_DPRINT("Setting up create phase params for FILE mode\n");
        arr[5] = G729DEC_DMM;
        if (pComponentPrivate->pOutputBufferList->numBuffers) {
            arr[6] = (OMX_U16)pComponentPrivate->pOutputBufferList->numBuffers;
        }
        else {
            arr[6] = 1;
        }

    }

    arr[7] = END_OF_CR_PHASE_ARGS;
    plcml_Init->pCrPhArgs = arr;  
    size_lcml = (OMX_U16)(nIpBuf * sizeof(LCML_G729DEC_BUFHEADERTYPE));
    pTemp_lcml = (LCML_G729DEC_BUFHEADERTYPE *)malloc(size_lcml);
    G729DEC_MEMPRINT("%d:[ALLOC] %p\n",__LINE__,pTemp_lcml);
    if(pTemp_lcml == NULL) {
        G729DEC_DPRINT("%d :: Memory Allocation Failed\n",__LINE__);
        /* Free previously allocated memory before bailing */
        if (strmAttr) {
            free(strmAttr);
            strmAttr = NULL;
        }
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    pComponentPrivate->pLcmlBufHeader[G729DEC_INPUT_PORT] = pTemp_lcml;
    
    for (i=0; i<nIpBuf; i++) {
        pTemp = pComponentPrivate->pInputBufferList->pBufHdr[i];
        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pTemp->nAllocLen = nIpBufSize;
        pTemp->nFilledLen = nIpBufSize;
        pTemp->nVersion.s.nVersionMajor = G729DEC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = G729DEC_MINOR_VER;
        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp_lcml->buffer = pTemp;
        pTemp_lcml->eDir = OMX_DirInput;
        OMX_G729MALLOC_STRUCT(pTemp_lcml->pIpParam, G729DEC_UAlgBufParamStruct);
        pTemp_lcml->pIpParam->usFrameLost = 0;
        pTemp_lcml->pIpParam->usLastFrame = 0;
        /* This means, it is not a last buffer. This flag is to be modified by
         * the application to indicate the last buffer */
        pTemp->nFlags = NORMAL_BUFFER;
        pTemp++;
        pTemp_lcml++;
    }

    /* Allocate memory for all output buffer headers..
     * This memory pointer will be sent to LCML */
    size_lcml = (OMX_U16)(nOpBuf * sizeof(LCML_G729DEC_BUFHEADERTYPE));
    pTemp_lcml = (LCML_G729DEC_BUFHEADERTYPE *)malloc(size_lcml);
    G729DEC_MEMPRINT("%d:[ALLOC] %p\n",__LINE__,pTemp_lcml);
    if(pTemp_lcml == NULL) {
        /* Free previously allocated memory before bailing */
        if (strmAttr) {
            free(strmAttr);
            strmAttr = NULL;
        }
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    memset(pTemp_lcml, 0x0, size_lcml);
    pComponentPrivate->pLcmlBufHeader[G729DEC_OUTPUT_PORT] = pTemp_lcml;
     
    for (i=0; i<nOpBuf; i++) {  
        pTemp = pComponentPrivate->pOutputBufferList->pBufHdr[i]; 
        pTemp->nSize = (OMX_U16)sizeof(OMX_BUFFERHEADERTYPE);            
        pTemp->nAllocLen = nOpBufSize; 
        pTemp->nFilledLen = nOpBufSize; 
        pTemp->nVersion.s.nVersionMajor = G729DEC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = G729DEC_MINOR_VER;
        pComponentPrivate->nVersion = pTemp->nVersion.nVersion;
        pTemp->pPlatformPrivate = pHandle->pComponentPrivate; 
        pTemp->nTickCount = 0;
        pTemp->nTimeStamp = 0;
        /* This means, it is not a last buffer. This flag is to be modified by
         * the application to indicate the last buffer */ 
        pTemp_lcml->buffer = pTemp; 
        pTemp_lcml->eDir = OMX_DirOutput;
        OMX_G729MALLOC_STRUCT(pTemp_lcml->pIpParam, G729DEC_UAlgBufParamStruct); 
        pTemp_lcml->pIpParam->usFrameLost = 0;
        pTemp_lcml->pIpParam->usLastFrame = 0;
        pTemp->nFlags = NORMAL_BUFFER;
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
    G729DEC_DPRINT("%d :: Exiting G729DECFill_LCMLInitParams",__LINE__);
    return eError;
}


/* ========================================================================== */
/**
 * @G729DEC_StartComponentThread() This function is called by the component to create
 * the component thread, command pipe and data pipe.
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



OMX_ERRORTYPE G729DEC_StartComponentThread(OMX_HANDLETYPE pComponent)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    G729DEC_COMPONENT_PRIVATE *pComponentPrivate =
        (G729DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
#ifdef UNDER_CE
    pthread_attr_t attr;
    memset(&attr, 0, sizeof(attr));
    attr.__inheritsched = PTHREAD_EXPLICIT_SCHED;
    attr.__schedparam.__sched_priority = OMX_AUDIO_DECODER_THREAD_PRIORITY;
#endif
    G729DEC_DPRINT ("%d :: Inside  G729DEC_StartComponentThread\n", __LINE__);

    /* Initialize all the variables*/
    pComponentPrivate->bIsStopping = 0;
    pComponentPrivate->bCompThreadStop = 0;    
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
        goto EXIT;
    }

    /* create the pipe used to send buffers to the thread */
    eError = pipe (pComponentPrivate->dataPipe);
    if (eError) {
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    /* create the pipe used to send commands to the thread */
    eError = pipe (pComponentPrivate->cmdPipe);
    if (eError) {
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    /* Create the Component Thread */
#ifdef UNDER_CE
    eError = pthread_create (&(pComponentPrivate->ComponentThread), &attr,
                             G729DEC_ComponentThread, pComponentPrivate);
#else
    eError = pthread_create (&(pComponentPrivate->ComponentThread), NULL,
                             G729DEC_ComponentThread, pComponentPrivate);
#endif
    if (eError || !pComponentPrivate->ComponentThread) {
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    pComponentPrivate_CC = pComponentPrivate; 

    pComponentPrivate->bCompThreadStarted = 1;
 EXIT:
    return eError;
}

/* ========================================================================== */
/**
 * @G729DEC_FreeCompResources() This function is called by the component during
 * de-init to close component thread, Command pipe, data pipe & LCML pipe.
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

OMX_ERRORTYPE G729DEC_FreeCompResources(OMX_HANDLETYPE pComponent)
{
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    G729DEC_COMPONENT_PRIVATE *pComponentPrivate = (G729DEC_COMPONENT_PRIVATE *)
        pHandle->pComponentPrivate;

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_ERRORTYPE err = OMX_ErrorNone;
    OMX_U32 nIpBuf = 0;
    OMX_U32 nOpBuf = 0;

    G729DEC_DPRINT ("%d :: G729DEC_FreeCompResources\n", __LINE__);

    if (pComponentPrivate->bPortDefsAllocated) {
        nIpBuf = pComponentPrivate->pPortDef[G729DEC_INPUT_PORT]->nBufferCountActual;
        nOpBuf = pComponentPrivate->pPortDef[G729DEC_OUTPUT_PORT]->nBufferCountActual;
    }

    if (pComponentPrivate->bCompThreadStarted) {
        err = close (pComponentPrivate->dataPipe[0]);

        if (0 != err && OMX_ErrorNone == eError) {
            eError = OMX_ErrorHardware;
        }

        err = close (pComponentPrivate->dataPipe[1]);
        if (0 != err && OMX_ErrorNone == eError) {
            eError = OMX_ErrorHardware;
        }

        err = close (pComponentPrivate->cmdPipe[0]);
        if (0 != err && OMX_ErrorNone == eError) {
            eError = OMX_ErrorHardware;
        }

        err = close (pComponentPrivate->cmdPipe[1]);
        if (0 != err && OMX_ErrorNone == eError) {
            eError = OMX_ErrorHardware;
        }

        err = close (pComponentPrivate->cmdDataPipe[0]);
        if (0 != err && OMX_ErrorNone == eError) {
            eError = OMX_ErrorHardware;
        }

        err = close (pComponentPrivate->cmdDataPipe[1]);
        if (0 != err && OMX_ErrorNone == eError) {
            eError = OMX_ErrorHardware;
        }

    }

    if (pComponentPrivate->bPortDefsAllocated) {
        if (pComponentPrivate->pPortDef[G729DEC_INPUT_PORT]) {
            G729DEC_MEMPRINT("%d:[FREE] %p\n",__LINE__,pComponentPrivate->pPortDef[G729DEC_INPUT_PORT]);
            free(pComponentPrivate->pPortDef[G729DEC_INPUT_PORT]);
            pComponentPrivate->pPortDef[G729DEC_INPUT_PORT] = NULL;
        }
        pComponentPrivate->pPortDef[G729DEC_INPUT_PORT] = NULL;

        if (pComponentPrivate->pPortDef[G729DEC_OUTPUT_PORT]) {
            G729DEC_MEMPRINT("%d:[FREE] %p\n",__LINE__,pComponentPrivate->pPortDef[G729DEC_OUTPUT_PORT]);
            free (pComponentPrivate->pPortDef[G729DEC_OUTPUT_PORT]);
            pComponentPrivate->pPortDef[G729DEC_OUTPUT_PORT] = NULL;
        }
        pComponentPrivate->pPortDef[G729DEC_OUTPUT_PORT] = NULL;

        if (pComponentPrivate->g729Params) {
            G729DEC_MEMPRINT("%d:[FREE] %p\n",__LINE__,pComponentPrivate->g729Params);
            free(pComponentPrivate->g729Params);
            pComponentPrivate->g729Params = NULL;
        }

        if (pComponentPrivate->pcmParams) {
            G729DEC_MEMPRINT("%d:[FREE] %p\n",__LINE__,pComponentPrivate->pcmParams);
            free (pComponentPrivate->pcmParams);
            pComponentPrivate->pcmParams = NULL;
        }
    }
    pComponentPrivate->bPortDefsAllocated = 0;
    return eError;
}



OMX_ERRORTYPE G729DEC_CleanupInitParams(OMX_HANDLETYPE pComponent)
{
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    G729DEC_COMPONENT_PRIVATE *pComponentPrivate = (G729DEC_COMPONENT_PRIVATE *)
        pHandle->pComponentPrivate;

    LCML_G729DEC_BUFHEADERTYPE *pTemp_lcml = NULL;

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nIpBuf = 0;
    OMX_U32 nOpBuf = 0;
    OMX_U32 i=0;

    G729DEC_DPRINT ("%d :: G729DEC_CleanupInitParams()\n", __LINE__);

    if (pComponentPrivate->strmAttr) {
        free(pComponentPrivate->strmAttr);
        pComponentPrivate->strmAttr = NULL;
    }
    if(pComponentPrivate->pParams!=NULL){
        free(pComponentPrivate->pParams);
        pComponentPrivate->pParams = NULL;
    }
   
    nIpBuf = pComponentPrivate->nRuntimeInputBuffers;
    pTemp_lcml = pComponentPrivate->pLcmlBufHeader[G729DEC_INPUT_PORT];

    for(i=0; i<nIpBuf; i++) {
        G729DEC_MEMPRINT("%d:[FREE] %p\n",__LINE__,pTemp_lcml->pIpParam);
        free(pTemp_lcml->pIpParam);
        pTemp_lcml->pIpParam = NULL;
        pTemp_lcml++;
    }


    nOpBuf = pComponentPrivate->nRuntimeOutputBuffers;
    pTemp_lcml = pComponentPrivate->pLcmlBufHeader[G729DEC_OUTPUT_PORT];
    for(i=0; i<nOpBuf; i++) {
        G729DEC_MEMPRINT("%d:[FREE] %p\n",__LINE__,pTemp_lcml->pIpParam);
        free(pTemp_lcml->pIpParam);
        pTemp_lcml->pIpParam = NULL;
        pTemp_lcml++;
    } 

    if(pComponentPrivate->bufParamsArray != NULL){
        pComponentPrivate->bufParamsArray -= 128;
        free(pComponentPrivate->bufParamsArray);
        pComponentPrivate->bufParamsArray = NULL;
    }
    G729DEC_MEMPRINT("%d:[FREE] %p\n",__LINE__,pComponentPrivate->pLcmlBufHeader[G729DEC_INPUT_PORT]);
    free(pComponentPrivate->pLcmlBufHeader[G729DEC_INPUT_PORT]);
    pComponentPrivate->pLcmlBufHeader[G729DEC_INPUT_PORT] = NULL;

    G729DEC_MEMPRINT("%d:[FREE] %p\n",__LINE__,pComponentPrivate->pLcmlBufHeader[G729DEC_OUTPUT_PORT]);
    free(pComponentPrivate->pLcmlBufHeader[G729DEC_OUTPUT_PORT]);
    pComponentPrivate->pLcmlBufHeader[G729DEC_OUTPUT_PORT] = NULL;
    return eError;
}

/* ========================================================================== */
/**
 * @G729DEC_StopComponentThread() This function is called by the component during
 * de-init to close component thread, Command pipe, data pipe & LCML pipe.
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

OMX_ERRORTYPE G729DEC_StopComponentThread(OMX_HANDLETYPE pComponent)
{
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    G729DEC_COMPONENT_PRIVATE *pComponentPrivate = (G729DEC_COMPONENT_PRIVATE *)
        pHandle->pComponentPrivate;

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_ERRORTYPE threadError = OMX_ErrorNone;
    OMX_S16 pthreadError = 0;

    /*Join the component thread */
    pComponentPrivate->bIsStopping = 1;
    pComponentPrivate->bCompThreadStop = 1;
    pthreadError = pthread_join (pComponentPrivate->ComponentThread,
                                 (void*)&threadError);
    if (0 != pthreadError) {
        eError = OMX_ErrorHardware;
    }

    /*Check for the errors */
    if (OMX_ErrorNone != threadError && OMX_ErrorNone != eError) {
        eError = OMX_ErrorInsufficientResources;
        G729DEC_DPRINT ("%d :: Error while closing Component Thread\n",__LINE__);
    }
    return eError;
}


/* ========================================================================== */
/**
 * @G729DECHandleCommand() This function is called by the component when ever it
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

OMX_U32 G729DECHandleCommand (G729DEC_COMPONENT_PRIVATE *pComponentPrivate)
{

    OMX_COMPONENTTYPE *pHandle = NULL;
    OMX_COMMANDTYPE command;
    OMX_STATETYPE commandedState = OMX_StateInvalid;
    OMX_U32 commandData  = 0;
    OMX_HANDLETYPE pLcmlHandle = pComponentPrivate->pLcmlHandle;
    OMX_ERRORTYPE rm_error = OMX_ErrorNone;
    OMX_U16 i = 0;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nBuf = 0;
    OMX_U16 arr[100] = {0};
    OMX_S8 *p = (OMX_S8 *)"hello";
    LCML_CALLBACKTYPE cb;
    LCML_DSP *pLcmlDsp = NULL;
    G729DEC_AudioCodecParams *pParams = NULL;
    OMX_S16 ret = 0;
    LCML_G729DEC_BUFHEADERTYPE *pLcmlHdr = NULL;
    int inputPortFlag=0,outputPortFlag=0;
    OMX_U8 frameType = 0;
    OMX_U32 nOpBuf = 0;

    G729DEC_DPRINT ("%d :: Entering G729DECHandleCommand Function\n", __LINE__);

    pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;
    G729DEC_DPRINT("pComponentPrivate->pHandle = %p at HandleCommand\n", pComponentPrivate->pHandle);
    
    ret = (OMX_U16)(read (pComponentPrivate->cmdPipe[0], &command, sizeof (command)));
    if (ret == -1) {
        G729DEC_EPRINT ("%d :: Error While reading from the Pipe\n",__LINE__);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }

    ret = (OMX_U16)(read (pComponentPrivate->cmdDataPipe[0], &commandData, sizeof (commandData)));
    if (ret == -1) {
        G729DEC_EPRINT ("%d :: Error While reading from the Pipe\n",__LINE__);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }

#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedCommand(pComponentPrivate->pPERFcomp, command, commandData,
                         PERF_ModuleLLMM);
#endif

    if (command == OMX_CommandStateSet) {
        commandedState = (OMX_STATETYPE)commandData;
        if (pComponentPrivate->curState == commandedState){
            pComponentPrivate->cbInfo.EventHandler (
                                                    pHandle, pHandle->pApplicationPrivate,
                                                    OMX_EventError, OMX_ErrorSameState,0,
                                                    NULL);
            goto EXIT;
        }
        
        switch(commandedState) {
        case OMX_StateIdle:

#ifdef __PERF_INSTRUMENTATION__
            PERF_Boundary(pComponentPrivate->pPERFcomp,
                          PERF_BoundaryStart | PERF_BoundarySetup);
#endif                          
            if (pComponentPrivate->curState == OMX_StateLoaded || pComponentPrivate->curState == OMX_StateWaitForResources) {

                if (pComponentPrivate->pPortDef[G729DEC_INPUT_PORT]->bPopulated &&  
                    pComponentPrivate->pPortDef[G729DEC_INPUT_PORT]->bEnabled)  {
                    inputPortFlag = 1;
                }
                if (!pComponentPrivate->pPortDef[G729DEC_INPUT_PORT]->bPopulated && 
                    !pComponentPrivate->pPortDef[G729DEC_INPUT_PORT]->bEnabled) {
                    inputPortFlag = 1;
                }
                if (pComponentPrivate->pPortDef[G729DEC_OUTPUT_PORT]->bPopulated && 
                    pComponentPrivate->pPortDef[G729DEC_OUTPUT_PORT]->bEnabled) {
                    outputPortFlag = 1;
                }
                if (!pComponentPrivate->pPortDef[G729DEC_OUTPUT_PORT]->bPopulated && 
                    !pComponentPrivate->pPortDef[G729DEC_OUTPUT_PORT]->bEnabled) {
                    outputPortFlag = 1;
                }
                if (!(inputPortFlag && outputPortFlag)) {
                    pComponentPrivate->InLoaded_readytoidle = 1;

#ifndef UNDER_CE
                    pthread_mutex_lock(&pComponentPrivate->InLoaded_mutex); 
                    pthread_cond_wait(&pComponentPrivate->InLoaded_threshold, &pComponentPrivate->InLoaded_mutex);
                    pthread_mutex_unlock(&pComponentPrivate->InLoaded_mutex);
#else
                    OMX_WaitForEvent(&(pComponentPrivate->InLoaded_event));
#endif
                }
 
                G729DEC_DPRINT ("%d :: Inside G729DECHandleCommand\n",__LINE__);
                cb.LCML_Callback = (void *) G729DECLCML_Callback;
                pLcmlHandle = (OMX_HANDLETYPE) G729DECGetLCMLHandle(pComponentPrivate);
                if (pLcmlHandle == NULL) {
                    G729DEC_DPRINT("%d :: LCML Handle is NULL........exiting..\n",__LINE__);
                    goto EXIT;
                }
                G729DEC_DPRINT("G729DECHandleCommand %d\n",__LINE__);
                G729DEC_DPRINT("pLcmlHandle = %p\n",pLcmlHandle);

                /* Got handle of dsp via phandle filling information about DSP
                   specific things */
                pLcmlDsp = (((LCML_DSP_INTERFACE*)pLcmlHandle)->dspCodec);
                G729DEC_DPRINT("pLcmlDsp = %p\n",pLcmlDsp);
                eError = G729DECFill_LCMLInitParams(pHandle, pLcmlDsp, arr);
                if(eError != OMX_ErrorNone) {
                    G729DEC_DPRINT("%d :: Error returned from\
                                                            G729DECFill_LCMLInitParams()\n",__LINE__);
                    goto EXIT;
                }
                G729DEC_DPRINT("%d :: Comp: OMX_G729DecUtils.c\n",__LINE__);
                pComponentPrivate->pLcmlHandle = (LCML_DSP_INTERFACE *)pLcmlHandle;
                /*filling create phase params */
                /*         cb.LCML_Callback = (void *) G729DECLCML_Callback;
                           G729DEC_DPRINT("%d :: Calling LCML_InitMMCodec...\n",__LINE__); */

#ifndef UNDER_CE
                /* TeeDN will be default for decoder component */
                G729DEC_DPRINT("G729 decoder support TeeDN\n");
                eError = LCML_InitMMCodecEx(((LCML_DSP_INTERFACE *)pLcmlHandle)->pCodecinterfacehandle,
                                            (void *)p,&pLcmlHandle,(void *)p,&cb, (OMX_STRING)pComponentPrivate->sDeviceString);
#else
                eError = LCML_InitMMCodec(((LCML_DSP_INTERFACE *)pLcmlHandle)->pCodecinterfacehandle,
                                          (void *)p,&pLcmlHandle,(void *)p,&cb);
#endif
                if(eError != OMX_ErrorNone) {
                    G729DEC_EPRINT("%d :: Error returned from LCML_Init()\n",__LINE__);
                    goto EXIT;
                }
#ifdef HASHINGENABLE
                /* Enable the Hashing Code */
                eError = LCML_SetHashingState(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle, OMX_TRUE);
                if (eError != OMX_ErrorNone) {
                    G729DEC_DPRINT("Failed to set Mapping State\n");
                    goto EXIT;
                }
#endif

                    

#ifdef RESOURCE_MANAGER_ENABLED
                /* need check the resource with RM */
                pComponentPrivate->rmproxyCallback.RMPROXY_Callback = (void *) G729DEC_ResourceManagerCallback;
                rm_error = RMProxy_NewSendCommand(pHandle,
                                                  RMProxy_RequestResource,
                                                  OMX_G729_Decoder_COMPONENT,
                                                  G729DEC_CPU,
                                                  3456,
                                                  &(pComponentPrivate->rmproxyCallback));
                if(rm_error == OMX_ErrorNone) {
                    /* resource is available */
                    rm_error = RMProxy_NewSendCommand(pHandle, 
                                                      RMProxy_StateSet,
                                                      OMX_G729_Decoder_COMPONENT,
                                                      OMX_StateIdle,
                                                      3456,NULL);
                    pComponentPrivate->curState = OMX_StateIdle;
                    pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventCmdComplete,
                                                           OMX_CommandStateSet,
                                                           pComponentPrivate->curState,
                                                           NULL);                                                                                         
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
                    G729DEC_DPRINT("%d :: OMX_G729Dec_Utils.c ::  Error - insufficient resources\n", __LINE__);
                }             
#else
                pComponentPrivate->curState = OMX_StateIdle;
                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandStateSet,
                                                       pComponentPrivate->curState,
                                                       NULL);
#endif

#ifdef __PERF_INSTRUMENTATION__
                PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryComplete | PERF_BoundarySetup);
#endif
                G729DEC_DPRINT("%d :: G729DEC: State has been Set to Idle\n",
                               __LINE__);

                if(pComponentPrivate->dasfmode == 1) {
                    OMX_U32 pValues[4];
                    G729DEC_DPRINT("%d :: ---- Comp: DASF Functionality is ON ---\n",__LINE__);
                    OMX_G729MALLOC_STRUCT(pParams, G729DEC_AudioCodecParams);
                    G729DEC_DPRINT("Line %d:::pParams  = 0x%x\n",__LINE__,pParams );
                    pComponentPrivate->pParams = pParams;
                    /* TODO: Configure from test app */
                    pParams->iAudioFormat = 1;
                    pParams->iSamplingRate = 8000;

                    /*TODO: get this value from the audio manager */
                    pParams->iStrmId = pComponentPrivate->streamID;

                    pValues[0] = USN_STRMCMD_SETCODECPARAMS;
                    pValues[1] = (OMX_U32)pParams;
                    pValues[2] = sizeof(G729DEC_AudioCodecParams);
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               EMMCodecControlStrmCtrl,(void *)pValues);

                    if(eError != OMX_ErrorNone) {
                        G729DEC_EPRINT("%d: Error Occurred in Codec StreamControl..\n",__LINE__);
                        goto EXIT;
                    }
                }
            } 
            else if (pComponentPrivate->curState == OMX_StateExecuting) {
                OMX_S8 *pArgs = (OMX_S8*)"damedesuStr";
                /*Set the bIsStopping bit */
                G729DEC_DPRINT("%d :: G729DEC: About to set bIsStopping bit\n", __LINE__);

#ifdef __PERF_INSTRUMENTATION__
                PERF_Boundary(pComponentPrivate->pPERFcomp,
                              PERF_BoundaryComplete | PERF_BoundarySteadyState);
#endif
                G729DEC_DPRINT("About to call LCML_ControlCodec(STOP)\n");
                eError = LCML_ControlCodec(
                                           ((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                           MMCodecControlStop,(void *)pArgs);
                if(eError != OMX_ErrorNone) {
                    G729DEC_EPRINT("%d: Error Occurred in Codec Stop..\n", __LINE__);
                    goto EXIT;
                }

#ifdef HASHINGENABLE
                /*Hashing Change*/
                pLcmlHandle = (LCML_DSP_INTERFACE*)pComponentPrivate->pLcmlHandle;
                eError = LCML_FlushHashes(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle);
                if (eError != OMX_ErrorNone) {
                    G729DEC_EPRINT("Error occurred in Codec mapping flush!\n");
                    break;
                }
#endif

                pComponentPrivate->bStopSent=1;
                if (pComponentPrivate->pHoldBuffer) {
                    free(pComponentPrivate->pHoldBuffer);
                    pComponentPrivate->pHoldBuffer = NULL;
                } 
                pComponentPrivate->nHoldLength = 0; 
            } 
            else if(pComponentPrivate->curState == OMX_StatePause) {
                G729DEC_DPRINT("%d :: Comp: Stop Command Received\n",__LINE__);

#ifdef HASHINGENABLE
                /*Hashing Change*/
                pLcmlHandle = (LCML_DSP_INTERFACE*)pComponentPrivate->pLcmlHandle;
                eError = LCML_FlushHashes(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle);
                if (eError != OMX_ErrorNone) {
                    G729DEC_EPRINT("Error occurred in Codec mapping flush!\n");
                    break;
                }
#endif
                G729DEC_DPRINT("Setting to OMX_StateIdle - Line %d\n",__LINE__);
                pComponentPrivate->curState = OMX_StateIdle;

#ifdef RESOURCE_MANAGER_ENABLED
                rm_error = RMProxy_NewSendCommand(pHandle, RMProxy_StateSet, OMX_G729_Decoder_COMPONENT, OMX_StateIdle, 3456,NULL);
#endif
                G729DEC_DPRINT ("%d :: The component is stopped\n",__LINE__);
                pComponentPrivate->cbInfo.EventHandler (
                                                        pHandle,pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,OMX_CommandStateSet,pComponentPrivate->curState,
                                                        NULL);
            } 
            else {
                /* This means, it is invalid state from application */
                G729DEC_DPRINT("%d :: Comp: OMX_G729DecUtils.c\n",__LINE__);
                pComponentPrivate->cbInfo.EventHandler(
                                                       pHandle,
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventError,
                                                       OMX_ErrorIncorrectStateTransition,0,
                                                       NULL);
            }
            break;

        case OMX_StateExecuting:
            G729DEC_DPRINT("%d: G729DECHandleCommand: Cmd Executing \n",__LINE__);            
            
            if (pComponentPrivate->curState == OMX_StateIdle) {
                G729DEC_DPRINT("%d :: Comp: OMX_G729DecUtils.c\n",__LINE__);
                /* Sending commands to DSP via LCML_ControlCodec third argument
                   is not used for time being */
                pComponentPrivate->nFillBufferDoneCount = 0;  
                pComponentPrivate->bStopSent=0;
                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                           EMMCodecControlStart, NULL);
                if(eError != OMX_ErrorNone) {
                    G729DEC_EPRINT("%d: Error Occurred in Codec Start..\n",__LINE__);
                    goto EXIT;
                }
                /* Send input buffers to application */
                nBuf = pComponentPrivate->pInputBufferList->numBuffers;
                                

                G729DEC_DPRINT ("nBuf =  %d\n",nBuf);
                /* Send output buffers to codec */
            }
            else if (pComponentPrivate->curState == OMX_StatePause) {
                G729DEC_DPRINT("%d :: Comp: OMX_G729DecUtils.c\n",__LINE__);
                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                           EMMCodecControlStart, (void *)p);
                if (eError != OMX_ErrorNone) {
                    G729DEC_EPRINT ("Error While Resuming the codec\n");
                    goto EXIT;
                }
                
                for (i=0; i < pComponentPrivate->nNumInputBufPending; i++) {
                    if (pComponentPrivate->pInputBufHdrPending[i]) {
                        frameType = *(pComponentPrivate->pInputBufHdrPending[i]->pBuffer);
                        G729DECGetCorresponding_LCMLHeader(pComponentPrivate, pComponentPrivate->pInputBufHdrPending[i]->pBuffer, OMX_DirInput, &pLcmlHdr);
                        G729DEC_SetPending(pComponentPrivate,pComponentPrivate->pInputBufHdrPending[i],OMX_DirInput);
                        eError = LCML_QueueBuffer(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                  EMMCodecInputBuffer,  
                                                  pComponentPrivate->pInputBufHdrPending[i]->pBuffer,
                                                  pComponentPrivate->pInputBufHdrPending[i]->nAllocLen,
                                                  pComponentPrivate->pInputBufHdrPending[i]->nFilledLen,
                                                  (OMX_U8 *) pLcmlHdr->pIpParam,
                                                  sizeof(G729DEC_UAlgBufParamStruct),
                                                  NULL); 
                                           
                    }
                }
                pComponentPrivate->nNumInputBufPending = 0;

                for (i=0; i < pComponentPrivate->nNumOutputBufPending; i++) {
                    if (pComponentPrivate->pOutputBufHdrPending[i]) {
                        G729DECGetCorresponding_LCMLHeader(pComponentPrivate, pComponentPrivate->pOutputBufHdrPending[i]->pBuffer, OMX_DirOutput, &pLcmlHdr);
                        G729DEC_SetPending(pComponentPrivate,pComponentPrivate->pOutputBufHdrPending[i],OMX_DirOutput);
                        eError = LCML_QueueBuffer(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                  EMMCodecOuputBuffer,  
                                                  pComponentPrivate->pOutputBufHdrPending[i]->pBuffer,
                                                  pComponentPrivate->pOutputBufHdrPending[i]->nAllocLen,
                                                  pComponentPrivate->pOutputBufHdrPending[i]->nFilledLen,
                                                  (OMX_U8 *) pLcmlHdr->pIpParam,
                                                  sizeof(G729DEC_UAlgBufParamStruct),
                                                  NULL);
                    }
                }
                pComponentPrivate->nNumOutputBufPending = 0;
                
            }
            else {
                pComponentPrivate->cbInfo.EventHandler (
                                                        pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventError, OMX_ErrorIncorrectStateTransition,0,
                                                        NULL);
                G729DEC_EPRINT("%d :: Error: Invalid State Given by Application\n",__LINE__);
                goto EXIT;

            }

#ifdef __PERF_INSTRUMENTATION__
            PERF_Boundary(pComponentPrivate->pPERFcomp,
                          PERF_BoundaryStart | PERF_BoundarySteadyState);
#endif
            pComponentPrivate->curState = OMX_StateExecuting;
            /*Send state change notificaiton to Application */
            pComponentPrivate->cbInfo.EventHandler(
                                                   pHandle,
                                                   pHandle->pApplicationPrivate,
                                                   OMX_EventCmdComplete, OMX_CommandStateSet,pComponentPrivate->curState, NULL);

#ifdef RESOURCE_MANAGER_ENABLED
            rm_error = RMProxy_NewSendCommand(pHandle, RMProxy_StateSet, OMX_G729_Decoder_COMPONENT, OMX_StateExecuting, 3456,NULL);
#endif                     

            break;

        case OMX_StateLoaded:

            G729DEC_DPRINT("%d: G729DECHandleCommand: Cmd Loaded - curState = %d\n",__LINE__,pComponentPrivate->curState);
            if (pComponentPrivate->curState == OMX_StateWaitForResources){
                G729DEC_DPRINT("%d: G729DECHandleCommand: Cmd Loaded\n",__LINE__);
                
#ifdef __PERF_INSTRUMENTATION__
                PERF_Boundary(pComponentPrivate->pPERFcomp,
                              PERF_BoundaryStart | PERF_BoundaryCleanup); 
#endif           

                pComponentPrivate->curState = OMX_StateLoaded;

#ifdef __PERF_INSTRUMENTATION__
                PERF_Boundary(pComponentPrivate->pPERFcomp,
                              PERF_BoundaryComplete | PERF_BoundaryCleanup);
#endif                    

                pComponentPrivate->cbInfo.EventHandler (
                                                        pHandle, pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete, OMX_CommandStateSet,pComponentPrivate->curState,
                                                        NULL);
                break;

            }

            if (pComponentPrivate->curState != OMX_StateIdle &&
                pComponentPrivate->curState != OMX_StateWaitForResources)
            {
                pComponentPrivate->cbInfo.EventHandler (pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventError,
                                                        OMX_ErrorIncorrectStateTransition,0,
                                                        NULL);
                G729DEC_EPRINT("%d :: Error: Invalid State Given by Application\n",__LINE__);
                goto EXIT;
            }
                                        
#ifdef __PERF_INSTRUMENTATION__
            PERF_Boundary(pComponentPrivate->pPERFcomp,
                          PERF_BoundaryStart | PERF_BoundaryCleanup);
#endif

            G729DEC_DPRINT("pComponentPrivate->pInputBufferList->numBuffers = %d\n",pComponentPrivate->pInputBufferList->numBuffers);
            G729DEC_DPRINT("pComponentPrivate->pOutputBufferList->numBuffers = %d\n",pComponentPrivate->pOutputBufferList->numBuffers);
            nOpBuf = pComponentPrivate->pOutputBufferList->numBuffers;
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
            G729DEC_DPRINT("%d :: In side OMX_StateLoaded State: \n",__LINE__);
            eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                       EMMCodecControlDestroy, (void *)p);
                                                                                 
            if (eError != OMX_ErrorNone) {
                G729DEC_EPRINT("%d : Error: in Destroying the codec: no.  %x\n",__LINE__, eError);
                goto EXIT;
            }
            
#ifdef __PERF_INSTRUMENTATION__
            PERF_SendingCommand(pComponentPrivate->pPERF, -1, 0,
                                PERF_ModuleComponent);
#endif

            eError = EXIT_COMPONENT_THRD; 
            pComponentPrivate->bInitParamsInitialized = 0;
            /* Send StateChangeNotification to application */
            pComponentPrivate->bLoadedCommandPending = OMX_FALSE;
            break;

        case OMX_StatePause:
            G729DEC_DPRINT("%d: G729DECHandleCommand: Cmd Pause\n",__LINE__);
        
            if (pComponentPrivate->curState != OMX_StateExecuting &&
                pComponentPrivate->curState != OMX_StateIdle) {
                pComponentPrivate->cbInfo.EventHandler (
                                                        pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventError, OMX_ErrorIncorrectStateTransition,0,
                                                        NULL);
                G729DEC_EPRINT("%d :: Error: Invalid State Given by \
                       Application\n",__LINE__);
                goto EXIT;
            }
            eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                       EMMCodecControlPause, (void *)p);

            if (eError != OMX_ErrorNone) {
                G729DEC_EPRINT("%d : Error: in Pausing the codec\n",__LINE__);
                goto EXIT;
            }
            break;

        case OMX_StateWaitForResources:

            if (pComponentPrivate->curState == OMX_StateLoaded) {
#ifdef RESOURCE_MANAGER_ENABLED         
                rm_error = RMProxy_NewSendCommand(pHandle, RMProxy_StateSet, OMX_G729_Decoder_COMPONENT, OMX_StateWaitForResources, 3456,NULL);
#endif
                pComponentPrivate->curState = OMX_StateWaitForResources;
                pComponentPrivate->cbInfo.EventHandler(
                                                       pHandle, pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete, OMX_CommandStateSet,pComponentPrivate->curState,NULL);
            }
            else{
                pComponentPrivate->cbInfo.EventHandler(
                                                       pHandle, pHandle->pApplicationPrivate,
                                                       OMX_EventError, OMX_ErrorIncorrectStateTransition,0, "NULL");
            }
            break;


        case OMX_StateInvalid:
            G729DEC_DPRINT("%d: G729DECHandleCommand: Cmd OMX_StateInvalid:\n",__LINE__);
        
            pComponentPrivate->curState = OMX_StateInvalid;

            pComponentPrivate->cbInfo.EventHandler(
                                                   pHandle, pHandle->pApplicationPrivate,
                                                   OMX_EventError, OMX_ErrorInvalidState,0, NULL);
            break;

        case OMX_StateMax:
            G729DEC_DPRINT("%d: G729DECHandleCommand: Cmd OMX_StateMax::\n",__LINE__);
            break;
        } /* End of Switch */
    }
    else if (command == OMX_CommandMarkBuffer) {
        G729DEC_DPRINT("command OMX_CommandMarkBuffer received %d\n",__LINE__);
        if(!pComponentPrivate->pMarkBuf){
            /* TODO Need to handle multiple marks */
            pComponentPrivate->pMarkBuf = (OMX_MARKTYPE *)(commandData);
        }
    }
    else if (command == OMX_CommandPortDisable) {
        if (!pComponentPrivate->bDisableCommandPending) {
                                                        
            if(commandData == 0x0 || commandData == -1){
                pComponentPrivate->pPortDef[G729DEC_INPUT_PORT]->bEnabled = OMX_FALSE;                         
            }
           
                        
                    
        }
        if(commandData == 0x1 || commandData == -1){
            char *pArgs = "damedesuStr";
            pComponentPrivate->pPortDef[G729DEC_OUTPUT_PORT]->bEnabled = OMX_FALSE;
            if (pComponentPrivate->curState == OMX_StateExecuting) {
                pComponentPrivate->bNoIdleOnStop = OMX_TRUE;
                G729DEC_DPRINT("Calling LCML_ControlCodec() Line %d\n",__LINE__);
                eError = LCML_ControlCodec(
                                           ((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                           MMCodecControlStop,(void *)pArgs);
            }
        }
                
        if(commandData == 0x0) {
            if(!pComponentPrivate->pPortDef[G729DEC_INPUT_PORT]->bPopulated){
                /* return cmdcomplete event if input unpopulated */ 
                pComponentPrivate->cbInfo.EventHandler(pHandle, pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandPortDisable,
                                                       G729DEC_INPUT_PORT, NULL);
                                                                                           
                G729DEC_DPRINT("Clearing bDisableCommandPending Line %d\n",__LINE__);
                pComponentPrivate->bDisableCommandPending = 0;
            }
            else{
                pComponentPrivate->bDisableCommandPending = 1;
                pComponentPrivate->bDisableCommandParam = commandData;
            }
        }

        if(commandData == 0x1) {
            if (!pComponentPrivate->pPortDef[G729DEC_OUTPUT_PORT]->bPopulated){
                /* return cmdcomplete event if output unpopulated */ 
                pComponentPrivate->cbInfo.EventHandler(pHandle, pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandPortDisable,
                                                       G729DEC_OUTPUT_PORT, NULL);
                                                                                           
                G729DEC_DPRINT("Clearing bDisableCommandPending Line %d\n",__LINE__);
                pComponentPrivate->bDisableCommandPending = 0;
            }
            else {
                pComponentPrivate->bDisableCommandPending = 1;
                pComponentPrivate->bDisableCommandParam = commandData;
            }
        }

        if(commandData == -1) {
            if (!pComponentPrivate->pPortDef[G729DEC_INPUT_PORT]->bPopulated && 
                !pComponentPrivate->pPortDef[G729DEC_OUTPUT_PORT]->bPopulated){

                /* return cmdcomplete event if inout & output unpopulated */ 
                pComponentPrivate->cbInfo.EventHandler(
                                                       pHandle, pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete, OMX_CommandPortDisable,G729DEC_INPUT_PORT, NULL);

                pComponentPrivate->cbInfo.EventHandler(
                                                       pHandle, pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete, OMX_CommandPortDisable,G729DEC_OUTPUT_PORT, NULL);
                G729DEC_DPRINT("Clearing bDisableCommandPending Line %d\n",__LINE__);
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
                G729DEC_DPRINT("setting input port to enabled\n");
                pComponentPrivate->pPortDef[G729DEC_INPUT_PORT]->bEnabled = OMX_TRUE;
                G729DEC_DPRINT("pComponentPrivate->pPortDef[G729DEC_INPUT_PORT]->bEnabled = %d\n",pComponentPrivate->pPortDef[G729DEC_INPUT_PORT]->bEnabled);

                if(pComponentPrivate->AlloBuf_waitingsignal)
                {
                    pComponentPrivate->AlloBuf_waitingsignal = 0;
                }
            }    
            if(commandData == 0x1 || commandData == -1){
                /* enable out port */
                G729DEC_DPRINT("setting output port to enabled\n");

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
                if (pComponentPrivate->curState == OMX_StateExecuting) {
                    char *pArgs = "damedesuStr";
                    pComponentPrivate->bDspStoppedWhileExecuting = OMX_FALSE;
                    eError = LCML_ControlCodec(
                                               ((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               EMMCodecControlStart,(void *)pArgs);
                }

                pComponentPrivate->pPortDef[G729DEC_OUTPUT_PORT]->bEnabled = OMX_TRUE;
                G729DEC_DPRINT("pComponentPrivate->pPortDef[G729DEC_OUTPUT_PORT]->bEnabled = %d\n",
                               pComponentPrivate->pPortDef[G729DEC_OUTPUT_PORT]->bEnabled);
            }
        }

        if(commandData == 0x0){ 
            if(pComponentPrivate->curState == OMX_StateLoaded ||
               pComponentPrivate->pPortDef[G729DEC_INPUT_PORT]->bPopulated){

                pComponentPrivate->cbInfo.EventHandler( pHandle, 
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,
                                                        OMX_CommandPortEnable,
                                                        G729DEC_INPUT_PORT, 
                                                        NULL);
                pComponentPrivate->bEnableCommandPending = 0;
            }
            else {
                pComponentPrivate->bEnableCommandPending = 1;
                pComponentPrivate->bEnableCommandParam = commandData;
            }
        }
        else if(commandData == 0x1){
            if(pComponentPrivate->curState == OMX_StateLoaded ||
               pComponentPrivate->pPortDef[G729DEC_OUTPUT_PORT]->bPopulated){

                pComponentPrivate->cbInfo.EventHandler( pHandle, 
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete, OMX_CommandPortEnable,
                                                        G729DEC_OUTPUT_PORT, 
                                                        NULL);
                pComponentPrivate->bEnableCommandPending = 0;
            }
            else {
                pComponentPrivate->bEnableCommandPending = 1;
                pComponentPrivate->bEnableCommandParam = commandData;
            }
        }
        else if(commandData == -1){ 
            if(pComponentPrivate->curState == OMX_StateLoaded || 
               (pComponentPrivate->pPortDef[G729DEC_INPUT_PORT]->bPopulated 
                && pComponentPrivate->pPortDef[G729DEC_OUTPUT_PORT]->bPopulated)){

                pComponentPrivate->cbInfo.EventHandler(
                                                       pHandle, pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete, OMX_CommandPortEnable,G729DEC_INPUT_PORT, NULL);

                pComponentPrivate->cbInfo.EventHandler(
                                                       pHandle, pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete, OMX_CommandPortEnable,G729DEC_OUTPUT_PORT, NULL);

                pComponentPrivate->bEnableCommandPending = 0;
                G729DECFill_LCMLInitParamsEx(pComponentPrivate->pHandle);
                pComponentPrivate->bJustReenabled = 1;
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
    else if (command == OMX_CommandFlush) {
        G729DEC_DPRINT("%d:: OMX_CommandFlush\n", __LINE__);
        OMX_U32 aParam[3] = {0};
        if(commandData == 0x0 || commandData == -1)
        {
            if (pComponentPrivate->nUnhandledEmptyThisBuffers == 0) {
                aParam[0] = USN_STRMCMD_FLUSH; 
                aParam[1] = 0x0; 
                aParam[2] = 0x0; 
                G729DEC_DPRINT("Flushing input port\n");
                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                           EMMCodecControlStrmCtrl, (void*)aParam);
                if (eError != OMX_ErrorNone)
                {
                    G729DEC_EPRINT("Error flushing input port: %d\n", eError);
                    goto EXIT;
                }
            }
            else{
                pComponentPrivate->bFlushInputPortCommandPending = OMX_TRUE;
            }
        }
        if(commandData == 0x1 || commandData == -1)
        {
            if (pComponentPrivate->nUnhandledFillThisBuffers == 0)  {
                aParam[0] = USN_STRMCMD_FLUSH; 
                aParam[1] = 0x1; 
                aParam[2] = 0x0; 
                G729DEC_DPRINT("Flushing output port\n");
                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                           EMMCodecControlStrmCtrl, (void*)aParam);
                if (eError != OMX_ErrorNone)
                {
                    G729DEC_DPRINT("Error flushing output port: %d\n", eError);                
                    goto EXIT;
                }
            }
            else{
                pComponentPrivate->bFlushOutputPortCommandPending = OMX_TRUE; 
            }                
        }
    }
 EXIT:
    G729DEC_DPRINT ("%d :: Exiting G729DECHandleCommand Function\n",__LINE__);
    return eError;
}


/* ========================================================================== */
/**
 * @G729DECHandleDataBuf_FromApp() This function is called by the component when ever it
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
OMX_ERRORTYPE G729DECHandleDataBuf_FromApp(OMX_BUFFERHEADERTYPE* pBufHeader,
                                           G729DEC_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_DIRTYPE eDir;
    LCML_G729DEC_BUFHEADERTYPE *pLcmlHdr = NULL;
    LCML_DSP_INTERFACE *pLcmlHandle = (LCML_DSP_INTERFACE *)
        pComponentPrivate->pLcmlHandle;
    G729DEC_BufParamStruct* pInBufStruct=NULL;
    /*  unsigned long int* bufParamsArray = NULL;    */

    G729DEC_DPRINT ("%d :: Entering G729DECHandleDataBuf_FromApp Function\n",__LINE__);
    /*Find the direction of the received buffer from buffer list */
    eError = G729DECGetBufferDirection(pBufHeader, &eDir);
    if (eError != OMX_ErrorNone) {
        G729DEC_DPRINT ("%d :: The PBufHeader is not found in the list\n",
                        __LINE__);
        goto EXIT;
    }
    if (eDir == OMX_DirInput) {
        pComponentPrivate->nUnhandledEmptyThisBuffers--;
        /*   bufParamsArray = malloc((10 * sizeof(unsigned long int)) + 256);
             bufParamsArray += 128;
             memset(bufParamsArray, 0, 9 * sizeof(unsigned long int)); */
        pInBufStruct = (G729DEC_BufParamStruct*)pBufHeader->pInputPortPrivate;

	 if (pInBufStruct == NULL) {
	     G729DEC_EPRINT("%d :: Error: input port NULL ...\n", __LINE__);
	     goto EXIT;
	 }
        /* fill array for SN params */
        if(pInBufStruct->bNoUseDefaults == OMX_TRUE){ /*indicates that khronos conformance tests are NOT running */
            pComponentPrivate->bufParamsArray[0] = 0;
            pComponentPrivate->bufParamsArray[1] = pInBufStruct->frameLost;
            pComponentPrivate->bufParamsArray[2] = pInBufStruct->numPackets;
            pComponentPrivate->bufParamsArray[3] = pInBufStruct->packetLength[0];
            pComponentPrivate->bufParamsArray[4] = pInBufStruct->packetLength[1]; 
            pComponentPrivate->bufParamsArray[5] = pInBufStruct->packetLength[2]; 
            pComponentPrivate->bufParamsArray[6] = pInBufStruct->packetLength[3]; 
            pComponentPrivate->bufParamsArray[7] = pInBufStruct->packetLength[4]; 
            pComponentPrivate->bufParamsArray[8] = pInBufStruct->packetLength[5]; 
        }
        else
        {   
            pComponentPrivate->bufParamsArray[0] = 0; /* last frame flag */
            pComponentPrivate->bufParamsArray[1] = 0; /* frame lost flag */
            pComponentPrivate->bufParamsArray[2] = 1; /* number of frames in this buffer */
            pComponentPrivate->bufParamsArray[3] = 11; /*lengths of each frame in bytes */
            pComponentPrivate->bufParamsArray[4] = 0;
            pComponentPrivate->bufParamsArray[5] = 0;
            pComponentPrivate->bufParamsArray[6] = 0;
            pComponentPrivate->bufParamsArray[7] = 0;
            pComponentPrivate->bufParamsArray[8] = 0;
        }              
        eError = G729DECGetCorresponding_LCMLHeader(pComponentPrivate, pBufHeader->pBuffer, OMX_DirInput, &pLcmlHdr);
        if (eError != OMX_ErrorNone) {
            G729DEC_EPRINT("%d :: Error: Invalid Buffer Came ...\n",__LINE__);
            goto EXIT;
        }                
        if(pBufHeader->nFlags == OMX_BUFFERFLAG_EOS){ /* Last input buffer from App. */
            pBufHeader->nFlags = NORMAL_BUFFER; 
            pLcmlHdr->pIpParam->usLastFrame = 1;
            pComponentPrivate->bufParamsArray[0] = 1;
            pComponentPrivate->bIsEOFSent = 1;
        }
        else{
            pLcmlHdr->pIpParam->usLastFrame = 0;
            pComponentPrivate->bufParamsArray[0] = 0;
        }
	 pLcmlHdr->pIpParam->usFrameLost = pInBufStruct->frameLost;
	 if (pInBufStruct->frameLost == 1) {
	     G729DEC_PRINT_INFO("Frame LOST event\n");
        }
        /* Store time stamp information */
        pComponentPrivate->arrTimestamp[pComponentPrivate->IpBufindex] = pBufHeader->nTimeStamp;
        pComponentPrivate->arrTickCount[pComponentPrivate->IpBufindex] = pBufHeader->nTickCount;                
        pComponentPrivate->IpBufindex++;
        pComponentPrivate->IpBufindex %= pComponentPrivate->pPortDef[G729DEC_INPUT_PORT]->nBufferCountActual; 
        if (pComponentPrivate->curState == OMX_StateExecuting) {
            if (!G729DEC_IsPending(pComponentPrivate,pBufHeader,OMX_DirInput)) {
                G729DEC_SetPending(pComponentPrivate,pBufHeader,OMX_DirInput);
                eError = LCML_QueueBuffer(pLcmlHandle->pCodecinterfacehandle,
                                          EMMCodecInputBuffer,  
                                          (OMX_U8 *)pBufHeader->pBuffer, 
                                          pBufHeader->nAllocLen,
                                          pBufHeader->nFilledLen,
                                          (OMX_U8 *) pComponentPrivate->bufParamsArray,
                                          sizeof(pComponentPrivate->bufParamsArray),
                                          NULL);
                if (eError != OMX_ErrorNone) {
                    eError = OMX_ErrorHardware;
                    goto EXIT;
                }
                pComponentPrivate->lcml_nCntIp++;
                pComponentPrivate->lcml_nIpBuf++;
                pComponentPrivate->num_Sent_Ip_Buff++;
            }
        }
        else if (pComponentPrivate->curState == OMX_StatePause)  {
            pComponentPrivate->pInputBufHdrPending[pComponentPrivate->nNumInputBufPending++] = pBufHeader;
        }
        if(pBufHeader->pMarkData){
            /* copy mark to output buffer header */ 
            pComponentPrivate->pOutputBufferList->pBufHdr[0]->pMarkData = pBufHeader->pMarkData;
            pComponentPrivate->pOutputBufferList->pBufHdr[0]->hMarkTargetComponent = pBufHeader->hMarkTargetComponent;

            /* trigger event handler if we are supposed to */ 
            if(pBufHeader->hMarkTargetComponent == pComponentPrivate->pHandle && pBufHeader->pMarkData){
                pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle, 
                                                       pComponentPrivate->pHandle->pApplicationPrivate, 
                                                       OMX_EventMark, 
                                                       0, 
                                                       0, 
                                                       pBufHeader->pMarkData);
            }
        }
        if (pComponentPrivate->bFlushInputPortCommandPending) {
            OMX_SendCommand(pComponentPrivate->pHandle,OMX_CommandFlush,0,NULL);
        }
    } 

    else if (eDir == OMX_DirOutput) {
        /* Make sure that output buffer is issued to output stream only when
         * there is an outstanding input buffer already issued on input stream
         */
        pComponentPrivate->nUnhandledFillThisBuffers--;
        if (!(pComponentPrivate->bIsStopping)) {
            G729DEC_DPRINT ("%d: Sending Empty OUTPUT BUFFER to Codec = %p\n",__LINE__,pBufHeader->pBuffer);
            if (pComponentPrivate->curState == OMX_StateExecuting) 
            {
                if (!G729DEC_IsPending(pComponentPrivate,pBufHeader,OMX_DirOutput)) {
                    G729DEC_SetPending(pComponentPrivate,pBufHeader,OMX_DirOutput);
                    eError = G729DECGetCorresponding_LCMLHeader(pComponentPrivate, pBufHeader->pBuffer, OMX_DirOutput, &pLcmlHdr);
                    if (eError != OMX_ErrorNone) {
                        G729DEC_DPRINT("%d :: Error: Invalid Buffer Came ...\n",__LINE__);
                        goto EXIT;
                    }
                    G729DEC_DPRINT("%d\n", __LINE__);
                    pLcmlHdr->pIpParam->usLastFrame = 0;
                    eError = LCML_QueueBuffer(pLcmlHandle->pCodecinterfacehandle,
                                              EMMCodecOuputBuffer,  
                                              (OMX_U8 *)pBufHeader->pBuffer, 
                                              pBufHeader->nAllocLen,
                                              pBufHeader->nAllocLen,
                                              (OMX_U8 *) pLcmlHdr->pIpParam,
                                              sizeof(G729DEC_UAlgBufParamStruct),
                                              NULL);
                    if (eError != OMX_ErrorNone ) {
                        G729DEC_EPRINT ("%d :: IssuingDSP OP: Error Occurred\n",__LINE__);
                        eError = OMX_ErrorHardware;
                        goto EXIT;
                    }
                    G729DEC_DPRINT("%d\n", __LINE__);
                    pComponentPrivate->lcml_nOpBuf++;
                    pComponentPrivate->num_Op_Issued++;
                    G729DEC_DPRINT("%d\n", __LINE__);
                }
            }
            else if (pComponentPrivate->curState == OMX_StatePause) 
            {
                pComponentPrivate->pOutputBufHdrPending[pComponentPrivate->nNumOutputBufPending++] = pBufHeader;
            }
        }
        else {
            if (pComponentPrivate->curState == OMX_StateExecuting) {
                if (!G729DEC_IsPending(pComponentPrivate,pBufHeader,OMX_DirOutput)) {
                    G729DEC_SetPending(pComponentPrivate,pBufHeader,OMX_DirOutput);
                    eError = G729DECGetCorresponding_LCMLHeader(pComponentPrivate, pBufHeader->pBuffer, OMX_DirOutput, &pLcmlHdr);
                    if (eError != OMX_ErrorNone) {
                        G729DEC_EPRINT("%d :: Error: Invalid Buffer Came ...\n",__LINE__);
                        goto EXIT;
                    }
                    pLcmlHdr->pIpParam->usLastFrame = 0;
                    eError = LCML_QueueBuffer(pLcmlHandle->pCodecinterfacehandle,
                                              EMMCodecOuputBuffer,
                                              (OMX_U8 *)pBufHeader->pBuffer,
                                              pBufHeader->nAllocLen,
                                              pBufHeader->nAllocLen,
                                              (OMX_U8 *) pLcmlHdr->pIpParam,
                                              sizeof(G729DEC_UAlgBufParamStruct),
                                              NULL);
                    if (eError != OMX_ErrorNone ) {
                        G729DEC_EPRINT ("%d :: IssuingDSP OP: Error Occurred\n",__LINE__);
                        eError = OMX_ErrorHardware;
                        goto EXIT;
                    }
                    pComponentPrivate->lcml_nOpBuf++;
                    pComponentPrivate->num_Op_Issued++;
                }
            }
            else if (pComponentPrivate->curState == OMX_StatePause){
                pComponentPrivate->pOutputBufHdrPending[pComponentPrivate->nNumOutputBufPending++] = pBufHeader;
            }
        }
        if (pComponentPrivate->bFlushOutputPortCommandPending) 
        {
            OMX_SendCommand( pComponentPrivate->pHandle,
                             OMX_CommandFlush,
                             1,NULL);
        }
    } 
    else {
        eError = OMX_ErrorBadParameter;
    }

 EXIT:
    
    G729DEC_DPRINT("%d : Exiting from  G729DECHandleDataBuf_FromApp \n",__LINE__);
    return eError;
}

/*-------------------------------------------------------------------*/
/**
 * G729DECGetBufferDirection () This function is used by the component thread to
 * request a buffer from the application.  Since it was called from 2 places,
 * it made sense to turn this into a small function.
 *
 * @param pData pointer to G729 Decoder Context Structure
 * @param pCur pointer to the buffer to be requested to be filled
 *
 * @retval none
 **/
/*-------------------------------------------------------------------*/

OMX_ERRORTYPE G729DECGetBufferDirection(OMX_BUFFERHEADERTYPE *pBufHeader,
                                        OMX_DIRTYPE *eDir)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    G729DEC_COMPONENT_PRIVATE *pComponentPrivate = pBufHeader->pPlatformPrivate;
    OMX_U32 nBuf = 0;
    OMX_U32 i = 0;
    OMX_BUFFERHEADERTYPE *pBuf = NULL;
    OMX_S16 flag = 1;

    G729DEC_DPRINT ("%d :: Entering G729DECGetBufferDirection Function\n",__LINE__);

    /*Search this buffer in input buffers list */
    nBuf = pComponentPrivate->pInputBufferList->numBuffers;
    for(i=0; i<nBuf; i++) {
        pBuf = pComponentPrivate->pInputBufferList->pBufHdr[i];
        if(pBufHeader == pBuf) {
            *eDir = OMX_DirInput;
            G729DEC_DPRINT ("%d :: Buffer %p is INPUT BUFFER\n",__LINE__, pBufHeader);
            flag = 0;
            goto EXIT;
        }
    }

    /*Search this buffer in input buffers list */
    nBuf = pComponentPrivate->pOutputBufferList->numBuffers;
    for(i=0; i<nBuf; i++) {
        pBuf = pComponentPrivate->pOutputBufferList->pBufHdr[i];
        if(pBufHeader == pBuf) {
            *eDir = OMX_DirOutput;
            G729DEC_DPRINT ("%d :: Buffer %p is OUTPUT BUFFER\n",__LINE__, pBufHeader);
            flag = 0;
            goto EXIT;
        }
    }

    if (flag == 1) {
        G729DEC_DPRINT ("%d :: Buffer %p is Not Found in the List\n",__LINE__,pBufHeader);
        eError = OMX_ErrorUndefined;
        goto EXIT;
    }
 EXIT:
    G729DEC_DPRINT ("%d :: Exiting G729DECGetBufferDirection Function\n",__LINE__);
    return eError;
}

/* -------------------------------------------------------------------*/
/**
 *  Callback() function will be called LCML component to write the msg
 *
 * @param msgBuffer                 This buffer will be returned by the LCML
 *
 * @retval OMX_NoError              Success, ready to roll
 *         OMX_Error_BadParameter   The input parameter pointer is null
 **/
/*-------------------------------------------------------------------*/


OMX_ERRORTYPE G729DECLCML_Callback (TUsnCodecEvent event,void * args [10])
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U8 *pBuffer = args[1];
    LCML_G729DEC_BUFHEADERTYPE *pLcmlHdr = NULL;
    OMX_COMPONENTTYPE *pHandle = NULL;
    OMX_S16 i = 0;
    G729DEC_COMPONENT_PRIVATE* pComponentPrivate = NULL;
    OMX_ERRORTYPE rm_error = OMX_ErrorNone;


    G729DEC_DPRINT ("%d :: Entering the G729DECLCML_Callback Function\n",__LINE__);
    pComponentPrivate = pComponentPrivate_CC;
    G729DEC_DPRINT("event = %d\n",event);

    pHandle =  pComponentPrivate->pHandle;
    G729DEC_DPRINT("pHandle = %p\n", pHandle);
    G729DEC_DPRINT("pComponentPrivate = %p in lcmlCallback()\n", pComponentPrivate);

    switch(event) {
                
    case EMMCodecDspError:
        G729DEC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecDspError\n");
        break;

    case EMMCodecInternalError:
        G729DEC_EPRINT("[LCML CALLBACK EVENT]  EMMCodecInternalError\n");
        break;

    case EMMCodecInitError:
        G729DEC_EPRINT("[LCML CALLBACK EVENT]  EMMCodecInitError\n");
        break;

    case EMMCodecDspMessageRecieved:
        G729DEC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecDspMessageRecieved\n");
        break;

    case EMMCodecBufferProcessed:
        G729DEC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecBufferProcessed\n");
        break;

    case EMMCodecProcessingStarted:
        G729DEC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecProcessingStarted\n");
        break;
                        
    case EMMCodecProcessingPaused:
        G729DEC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecProcessingPaused\n");
        break;

    case EMMCodecProcessingStoped:
        G729DEC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecProcessingStoped\n");
        break;

    case EMMCodecProcessingEof:
        G729DEC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecProcessingEof\n");
        break;

    case EMMCodecBufferNotProcessed:
        G729DEC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecBufferNotProcessed\n");
        break;

    case EMMCodecAlgCtrlAck:
        G729DEC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecAlgCtrlAck\n");
        break;

    case EMMCodecStrmCtrlAck:
        G729DEC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecStrmCtrlAck\n");
        break;
    }
       
    if(event == EMMCodecBufferProcessed)
    {   
        if( (OMX_U32)args [0] == EMMCodecInputBuffer) {

            G729DEC_DPRINT("%d :: Input: pBufferr = %p\n",__LINE__, pBuffer);
            eError = G729DECGetCorresponding_LCMLHeader(pComponentPrivate, pBuffer, OMX_DirInput, &pLcmlHdr);
            if (eError != OMX_ErrorNone) {
                G729DEC_DPRINT("%d :: Error: Invalid Buffer Came ...\n",__LINE__);
                goto EXIT;
            }
            
#ifdef __PERF_INSTRUMENTATION__
            PERF_ReceivedFrame(pComponentPrivate->pPERFcomp,
                               PREF(pLcmlHdr->buffer, pBuffer), 0,
                               PERF_ModuleCommonLayer);
#endif            
            
            G729DEC_ClearPending(pComponentPrivate,pLcmlHdr->buffer,OMX_DirInput);
            pComponentPrivate->cbInfo.EmptyBufferDone (
                                                       pComponentPrivate->pHandle,
                                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                                       pLcmlHdr->buffer);
            pComponentPrivate->nEmptyBufferDoneCount++;
            pComponentPrivate->lcml_nIpBuf--;
            pComponentPrivate->app_nBuf++;
        } 
        else if ((OMX_U32)args [0] == EMMCodecOuputBuffer) {
            G729DEC_DPRINT("%d :: Output: pBufferr = %p\n",__LINE__, pBuffer);
            eError = G729DECGetCorresponding_LCMLHeader(pComponentPrivate, pBuffer, OMX_DirOutput, &pLcmlHdr);
            if (eError != OMX_ErrorNone) {
                G729DEC_DPRINT("%d :: Error: Invalid Buffer Came ...\n",__LINE__);
                goto EXIT;
            }
            pLcmlHdr->buffer->nFilledLen = (OMX_U32)args[8];
            
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

            G729DEC_DPRINT("G729DECLCML_Callback::: pLcmlHdr->buffer->nFilledLen = %d\n",pLcmlHdr->buffer->nFilledLen);
            pComponentPrivate->lcml_nCntOpReceived++;

            G729DEC_ClearPending(pComponentPrivate,pLcmlHdr->buffer,OMX_DirOutput);
            pComponentPrivate->nOutStandingFillDones++;
            pComponentPrivate->num_Reclaimed_Op_Buff++;
            eError = G729DECGetCorresponding_LCMLHeader(pComponentPrivate, pLcmlHdr->buffer->pBuffer, OMX_DirOutput, &pLcmlHdr);
            if(pComponentPrivate->bIsEOFSent){
                pLcmlHdr->buffer->nFlags=OMX_BUFFERFLAG_EOS; 
                pComponentPrivate->bIsEOFSent = 0;
                pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                                       OMX_EventBufferFlag,
                                                       pLcmlHdr->buffer->nOutputPortIndex,
                                                       pLcmlHdr->buffer->nFlags, NULL);
            }
            else{
                pLcmlHdr->buffer->nFlags=NORMAL_BUFFER;
            }
            /* Copying time stamp information to output buffer */
            pLcmlHdr->buffer->nTimeStamp = pComponentPrivate->arrTimestamp[pComponentPrivate->OpBufindex];
            pLcmlHdr->buffer->nTickCount = pComponentPrivate->arrTickCount[pComponentPrivate->OpBufindex];    
            pComponentPrivate->OpBufindex++;
            pComponentPrivate->OpBufindex %= pComponentPrivate->pPortDef[G729DEC_DIRECTION_INPUT]->nBufferCountActual;
            pComponentPrivate->cbInfo.FillBufferDone (
                                                      pComponentPrivate->pHandle,
                                                      pComponentPrivate->pHandle->pApplicationPrivate,
                                                      pLcmlHdr->buffer
                                                      );
            pComponentPrivate->lcml_nOpBuf--;
            pComponentPrivate->app_nBuf++;
            pComponentPrivate->nFillBufferDoneCount++;
            pComponentPrivate->nOutStandingFillDones--;
        }
    } 
    else if (event == EMMCodecStrmCtrlAck) 
    {
        G729DEC_DPRINT("%d :: GOT MESSAGE USN_DSPACK_STRMCTRL ----\n",__LINE__);
        if (args[1] == (void *)USN_STRMCMD_FLUSH)
        {
            G729DEC_DPRINT("revceived USN_STRMCMD_FLUSH\n");
            pHandle = pComponentPrivate->pHandle;
            if ( args[2] == (void *)EMMCodecInputBuffer)
            {
                if (args[0] == USN_ERR_NONE ) {
                    G729DEC_DPRINT("Flushing input port %d\n");
                    for (i=0; i < MAX_NUM_OF_BUFS; i++)
                    {
                        pComponentPrivate->pInputBufHdrPending[i] = NULL;
                    }
                    pComponentPrivate->nNumInputBufPending=0;
                    for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++)
                    {
                        pComponentPrivate->cbInfo.EmptyBufferDone (
                                                                   pComponentPrivate->pHandle,
                                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                                   pComponentPrivate->pInputBufferList->pBufHdr[i]
                                                                   );
                    }
                    pComponentPrivate->cbInfo.EventHandler(pHandle, pHandle->pApplicationPrivate,
                                                           OMX_EventCmdComplete,
                                                           OMX_CommandFlush,
                                                           G729DEC_INPUT_PORT,
                                                           NULL);
                }
                else 
                {
                    G729DEC_EPRINT("error flushing input port.\n");
                    goto EXIT;                            
                }
            }
            else if ( args[2] == (void *)EMMCodecOuputBuffer)
            {
                if (args[0] == USN_ERR_NONE )
                {
                    G729DEC_DPRINT("Flushing output port %d\n");
                    for (i=0; i < MAX_NUM_OF_BUFS; i++)
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
                                                           OMX_EventCmdComplete, OMX_CommandFlush,G729DEC_OUTPUT_PORT, NULL);
                }
                else
                {
                    G729DEC_EPRINT("error flushing output port.\n");
                    goto EXIT;                            
                }
            }
        }                 

                 
    }
    else if(event == EMMCodecProcessingStoped) {
        G729DEC_DPRINT("pComponentPrivate->bNoIdleOnStop = %d\n",pComponentPrivate->bNoIdleOnStop);
        if (!pComponentPrivate->bNoIdleOnStop) {
            pComponentPrivate->bIdleCommandPending = OMX_TRUE;
            G729DEC_TransitionToIdle(pComponentPrivate);
        }
        else {
            pComponentPrivate->bDspStoppedWhileExecuting = OMX_TRUE;
            int i;
            for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
                G729DEC_DPRINT("pComponentPrivate->pInputBufferList->bBufferPending[%d] = %d\n",i,pComponentPrivate->pInputBufferList->bBufferPending[i]);
                if (pComponentPrivate->pInputBufferList->bBufferPending[i]) 
                {
                    pComponentPrivate->cbInfo.EmptyBufferDone (
                                                               pComponentPrivate->pHandle,
                                                               pComponentPrivate->pHandle->pApplicationPrivate,
                                                               pComponentPrivate->pInputBufferList->pBufHdr[i]
                                                               );
                    G729DEC_ClearPending(pComponentPrivate, pComponentPrivate->pInputBufferList->pBufHdr[i], OMX_DirInput);
                }
            }
            pComponentPrivate->bNoIdleOnStop= OMX_FALSE;
        }
    }
    else if (event == EMMCodecProcessingPaused) {
        pComponentPrivate->curState = OMX_StatePause;
        /* Send StateChangeNotification to application */
        pComponentPrivate->cbInfo.EventHandler(
                                               pComponentPrivate->pHandle,
                                               pComponentPrivate->pHandle->pApplicationPrivate,
                                               OMX_EventCmdComplete, OMX_CommandStateSet, pComponentPrivate->curState,NULL);
        
    }

    else if (event == EMMCodecDspError) {
        if(((int)args[4] == USN_ERR_WARNING) && ((int)args[5] == IUALG_WARN_PLAYCOMPLETED)) {
            OMX_COMPONENTTYPE *pHandle = pComponentPrivate->pHandle;
            G729DEC_DPRINT("%d :: GOT MESSAGE IUALG_WARN_PLAYCOMPLETED\n",__LINE__);
            pComponentPrivate->bPlayCompleteFlag = 1;
            pComponentPrivate->curState = OMX_StateIdle;
            
#ifdef RESOURCE_MANAGER_ENABLED
            rm_error = RMProxy_NewSendCommand(pHandle, RMProxy_StateSet, OMX_G729_Decoder_COMPONENT, OMX_StateIdle, 3456,NULL);
#endif
        }
        if(((int)args[4] == USN_ERR_WARNING) && ((int)args[5] == IUALG_WARN_OVERFLOW)) {
            OMX_COMPONENTTYPE *pHandle = pComponentPrivate->pHandle;
            pComponentPrivate->cbInfo.EventHandler(pHandle, pHandle->pApplicationPrivate,
                                                   OMX_EventError, OMX_ErrorOverflow,
                                                   pComponentPrivate->curState, NULL);
            G729DEC_DPRINT("%d :: GOT WARNING IUALG_WARN_OVERFLOW\n",__LINE__);
        }
        if(((int)args[4] == USN_ERR_PROCESS) && ((int)args[5] == IUALG_ERR_DATA_CORRUPT)){
            OMX_COMPONENTTYPE *pHandle = pComponentPrivate->pHandle;
            pComponentPrivate->cbInfo.EventHandler(pHandle, pHandle->pApplicationPrivate,
                                                   OMX_EventError, OMX_ErrorStreamCorrupt,
                                                   pComponentPrivate->curState, NULL);
            G729DEC_DPRINT("%d :: GOT ERROR IUALG_ERR_DATA_CORRUPT\n",__LINE__);
        }       
    }

    if(event == EMMCodecDspMessageRecieved) {
        G729DEC_DPRINT("%d :: commandedState  = %d\n",__LINE__,args[0]);
        G729DEC_DPRINT("%d :: arg1 = %d\n",__LINE__,args[1]);
        G729DEC_DPRINT("%d :: arg2 = %d\n",__LINE__,args[2]);
    }

 EXIT:
    G729DEC_DPRINT ("%d :: Exiting the G729DECLCML_Callback Function\n",__LINE__);
    return eError;
}


OMX_ERRORTYPE G729DECGetCorresponding_LCMLHeader(G729DEC_COMPONENT_PRIVATE* pComponentPrivate,
                                                 OMX_U8 *pBuffer,
                                                 OMX_DIRTYPE eDir,
                                                 LCML_G729DEC_BUFHEADERTYPE **ppLcmlHdr)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    LCML_G729DEC_BUFHEADERTYPE *pLcmlBufHeader = NULL;
    OMX_S16 nIpBuf = 0;
    OMX_S16 nOpBuf = 0;
    OMX_S16 i = 0;

    nIpBuf = (OMX_S16)pComponentPrivate->pInputBufferList->numBuffers;
    nOpBuf = (OMX_S16)pComponentPrivate->pOutputBufferList->numBuffers;
    G729DEC_DPRINT("enter get corresponding LCML header\n");
    G729DEC_DPRINT("pComponentPrivate = %p\n",pComponentPrivate);
    G729DEC_DPRINT("eDir = %d\n",eDir);
    if(eDir == OMX_DirInput) {
        G729DEC_DPRINT("pComponentPrivate = %p\n",pComponentPrivate);
        pLcmlBufHeader = pComponentPrivate->pLcmlBufHeader[G729DEC_INPUT_PORT];
        for(i=0; i<nIpBuf; i++) {
            if(pBuffer == pLcmlBufHeader->buffer->pBuffer) {
                *ppLcmlHdr = pLcmlBufHeader;
                G729DEC_DPRINT("%d::Corresponding LCML Header Found\n",__LINE__);
                goto EXIT;
            }
            pLcmlBufHeader++;
        }
    } 
    else if (eDir == OMX_DirOutput) {
        pLcmlBufHeader = pComponentPrivate->pLcmlBufHeader[G729DEC_OUTPUT_PORT];
        for(i=0; i<nOpBuf; i++) {
            if(pBuffer == pLcmlBufHeader->buffer->pBuffer) {
                G729DEC_DPRINT("pBuffer = %p\n",pBuffer);
                G729DEC_DPRINT("pLcmlBufHeader->buffer->pBuffer = %p\n",pLcmlBufHeader->buffer->pBuffer);
                *ppLcmlHdr = pLcmlBufHeader;
                G729DEC_DPRINT("%d::Corresponding LCML Header Found\n",__LINE__);
                goto EXIT;
            }
            pLcmlBufHeader++;
        }
    }
    else { 
        G729DEC_DPRINT("%d:: Invalid Buffer Type :: exiting...\n",__LINE__);
    } 

 EXIT:
    return eError;
}


#ifndef UNDER_CE

OMX_HANDLETYPE G729DECGetLCMLHandle(G729DEC_COMPONENT_PRIVATE* pComponentPrivate)
{
    void *handle = NULL;
    OMX_ERRORTYPE (*fpGetHandle)(OMX_HANDLETYPE);
    OMX_HANDLETYPE pHandle = NULL;
    OMX_S8 *error = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    G729DEC_DPRINT("G729DECGetLCMLHandle %d\n",__LINE__);
    handle = dlopen("libLCML.so", RTLD_LAZY);
    if (!handle) {
        fputs(dlerror(), stderr);
        goto EXIT;
    }

    fpGetHandle = dlsym (handle, "GetHandle");
    if ((error = (OMX_S8 *)dlerror()) != NULL) {
        fputs((char *)error, stderr);
        goto EXIT;
    }
    eError = (*fpGetHandle)(&pHandle);
    if(eError != OMX_ErrorNone) {
        eError = OMX_ErrorUndefined;
        G729DEC_EPRINT("eError != OMX_ErrorNone...\n");
        pHandle = NULL;
        goto EXIT;
    }

    pComponentPrivate->bLcmlHandleOpened = 1;

 EXIT:
    G729DEC_DPRINT("G729GetLCMLHandle returning %p\n",pHandle);

    return pHandle;
}


#else

//WINDOWS Explicit dll load procedure

OMX_HANDLETYPE G729DECGetLCMLHandle(G729DEC_COMPONENT_PRIVATE* pComponentPrivate)
{
    typedef OMX_ERRORTYPE (*LPFNDLLFUNC1)(OMX_HANDLETYPE);
    OMX_HANDLETYPE pHandle = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    LPFNDLLFUNC1 fpGetHandle1;


    g_hLcmlDllHandle = LoadLibraryEx(TEXT("OAF_BML.dll"), NULL,0);
    if (g_hLcmlDllHandle == NULL)
    {
        //fputs(dlerror(), stderr);
        G729DEC_EPRINT("BML Load Failed!!!\n");
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
    if(eError != OMX_ErrorNone) {
        eError = OMX_ErrorUndefined;
        G729DEC_EPRINT("eError != OMX_ErrorNone...\n");
        FreeLibrary(g_hLcmlDllHandle);
        g_hLcmlDllHandle = NULL;
        pHandle = NULL;
        return pHandle;
    }
    return pHandle;
}


#endif


#ifndef UNDER_CE

OMX_ERRORTYPE G729DECFreeLCMLHandle(G729DEC_COMPONENT_PRIVATE* pComponentPrivate)
{

    OMX_S16 retValue = 0;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    if (pComponentPrivate->bLcmlHandleOpened) {
        retValue = dlclose(pComponentPrivate->pLcmlHandle);

        if (retValue != 0) {
            eError = OMX_ErrorUndefined;
        }
        pComponentPrivate->bLcmlHandleOpened = 0;
    }

    return eError;
}
#else


OMX_ERRORTYPE G729DECFreeLCMLHandle(G729DEC_COMPONENT_PRIVATE* pComponentPrivate)
{

    OMX_S16 retValue = 0;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    if (pComponentPrivate->bLcmlHandleOpened) {

        retValue = FreeLibrary(pComponentPrivate->pLcmlHandle);
        if (retValue == 0) {          /* Zero Indicates failure */
            eError = OMX_ErrorUndefined;
        }
        pComponentPrivate->bLcmlHandleOpened = 0;
    }

    return eError;
}



#endif

void G729DEC_SetPending(G729DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir) 
{
    OMX_U16 i = 0;

    if (eDir == OMX_DirInput) {
        for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pInputBufferList->pBufHdr[i]) {
                pComponentPrivate->pInputBufferList->bBufferPending[i] = 1;
                G729DEC_DPRINT("*******************INPUT BUFFER %d IS PENDING******************************\n",i);
            }
        }
    }
    else {
        for (i=0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pOutputBufferList->pBufHdr[i]) {
                pComponentPrivate->pOutputBufferList->bBufferPending[i] = 1;
                G729DEC_DPRINT("*******************OUTPUT BUFFER %d IS PENDING******************************\n",i);
            }
        }
    }
}

void G729DEC_ClearPending(G729DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir) 
{
    OMX_U16 i = 0;

    if (eDir == OMX_DirInput) {
        for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pInputBufferList->pBufHdr[i]) {
                pComponentPrivate->pInputBufferList->bBufferPending[i] = 0;
                G729DEC_DPRINT("*******************INPUT BUFFER %d IS RECLAIMED******************************\n",i);
            }
        }
    }
    else {
        for (i=0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pOutputBufferList->pBufHdr[i]) {
                pComponentPrivate->pOutputBufferList->bBufferPending[i] = 0;
                G729DEC_DPRINT("*******************OUTPUT BUFFER %d IS RECLAIMED ******************************\n",i);
            }
        }
    }
}

OMX_U32 G729DEC_IsPending(G729DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir) 
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


OMX_U32 G729DEC_IsValid(G729DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U8 *pBuffer, OMX_DIRTYPE eDir) 
{
    OMX_U16 i = 0;
    OMX_S16 found=0;

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


OMX_ERRORTYPE G729DECFill_LCMLInitParamsEx (OMX_HANDLETYPE  pComponent )
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nIpBuf = 0,nIpBufSize = 0,nOpBuf = 0,nOpBufSize = 0;
    OMX_U16 i = 0;
    OMX_BUFFERHEADERTYPE *pTemp = NULL;
    OMX_S16 size_lcml = 0;
    LCML_STRMATTR *strmAttr = NULL;
    LCML_DSP_INTERFACE *pHandle = (LCML_DSP_INTERFACE *)pComponent;
    G729DEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    LCML_G729DEC_BUFHEADERTYPE *pTemp_lcml = NULL;

    G729DEC_DPRINT("%d :: G729DECFill_LCMLInitParams\n ",__LINE__);
    G729DEC_DPRINT("%d :: pHandle = %p\n",__LINE__,pHandle);
    G729DEC_DPRINT("%d :: pHandle->pComponentPrivate = %p\n",__LINE__,pHandle->pComponentPrivate);
    pComponentPrivate = pHandle->pComponentPrivate;

    nIpBuf = pComponentPrivate->pInputBufferList->numBuffers;
    nIpBufSize = pComponentPrivate->pPortDef[G729DEC_INPUT_PORT]->nBufferSize;
        

    nOpBuf = pComponentPrivate->pOutputBufferList->numBuffers;
    nOpBufSize = pComponentPrivate->pPortDef[G729DEC_OUTPUT_PORT]->nBufferSize;


    size_lcml = (OMX_U16)(nIpBuf * sizeof(LCML_G729DEC_BUFHEADERTYPE));
    pTemp_lcml = (LCML_G729DEC_BUFHEADERTYPE *)malloc(size_lcml);
    G729DEC_MEMPRINT("%d:[ALLOC] %p\n",__LINE__,pTemp_lcml);
    if(pTemp_lcml == NULL) {
        G729DEC_EPRINT("%d :: Memory Allocation Failed\n",__LINE__);
        /* Free previously allocated memory before bailing */
        if (strmAttr) {
            free(strmAttr);
            strmAttr = NULL;
        }
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    memset(pTemp_lcml, 0x0, size_lcml);
    pComponentPrivate->pLcmlBufHeader[G729DEC_INPUT_PORT] = pTemp_lcml;

    for (i=0; i<nIpBuf; i++) {
        pTemp = pComponentPrivate->pInputBufferList->pBufHdr[i];
        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pTemp->nAllocLen = nIpBufSize;
        pTemp->nFilledLen = nIpBufSize;
        pTemp->nVersion.s.nVersionMajor = G729DEC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = G729DEC_MINOR_VER;
        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp_lcml->buffer = pTemp;
        pTemp_lcml->eDir = OMX_DirInput;

        OMX_G729MALLOC_STRUCT(pTemp_lcml->pIpParam, G729DEC_UAlgBufParamStruct);
        pTemp_lcml->pIpParam->usFrameLost = 0;
        pTemp_lcml->pIpParam->usLastFrame = 0;

        /* This means, it is not a last buffer. This flag is to be modified by
         * the application to indicate the last buffer */
        pTemp->nFlags = NORMAL_BUFFER;

        pTemp_lcml++;
    }

    /* Allocate memory for all output buffer headers..
     * This memory pointer will be sent to LCML */
    size_lcml = (OMX_U16)(nOpBuf * sizeof(LCML_G729DEC_BUFHEADERTYPE));
    pTemp_lcml = (LCML_G729DEC_BUFHEADERTYPE *)malloc(size_lcml);
    G729DEC_MEMPRINT("%d:[ALLOC] %p\n",__LINE__,pTemp_lcml);
    if(pTemp_lcml == NULL) {
        /* Free previously allocated memory before bailing */
        if (strmAttr) {
            free(strmAttr);
            strmAttr = NULL;
        }
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    memset(pTemp_lcml, 0x0, size_lcml);
    pComponentPrivate->pLcmlBufHeader[G729DEC_OUTPUT_PORT] = pTemp_lcml;

    for (i=0; i<nOpBuf; i++) {
        pTemp = pComponentPrivate->pOutputBufferList->pBufHdr[i];
        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pTemp->nAllocLen = nOpBufSize;
        pTemp->nFilledLen = nOpBufSize;
        pTemp->nVersion.s.nVersionMajor = G729DEC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = G729DEC_MINOR_VER;
        pComponentPrivate->nVersion = pTemp->nVersion.nVersion;
        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = 0;
        pTemp->nTimeStamp = 0;
        /* This means, it is not a last buffer. This flag is to be modified by
         * the application to indicate the last buffer */

        pTemp_lcml->buffer = pTemp;
        pTemp_lcml->eDir = OMX_DirOutput;
        G729DEC_DPRINT("%d:::pTemp_lcml = %p\n",__LINE__,pTemp_lcml);
        G729DEC_DPRINT("%d:::pTemp_lcml->buffer = %p\n",__LINE__,pTemp_lcml->buffer);

        OMX_G729MALLOC_STRUCT(pTemp_lcml->pIpParam, G729DEC_UAlgBufParamStruct);
        pTemp_lcml->pIpParam->usFrameLost = 0;
        pTemp_lcml->pIpParam->usLastFrame = 0;

        pTemp->nFlags = NORMAL_BUFFER;

        pTemp++;
        pTemp_lcml++;
    }
    pComponentPrivate->bPortDefsAllocated = 1;
    G729DEC_DPRINT("%d :: Exiting G729DECFill_LCMLInitParams",__LINE__);

    pComponentPrivate->bInitParamsInitialized = 1;
 EXIT:
    return eError;
}
/* ================================================================================= */
/**
 * @fn G729DEC_TransitionToIdle() Transition component to OMX_StateIdle 
 G729DEC_TransitionToIdle().  
 This method transitions the component to OMX_StateIdle
 * @param pComponent  handle for this instance of the component
 *
 * @pre
 *
 * @post
 *
 * @return OMX_ERRORTYPE
 */
/* ================================================================================ */
OMX_ERRORTYPE G729DEC_TransitionToIdle(G729DEC_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_ERRORTYPE rm_error = OMX_ErrorNone;
    OMX_U16 i = 0;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;

    G729DEC_DPRINT("Entered G729DEC_TransitionToIdle\n");
    G729DEC_DPRINT("pComponentPrivate->nOutStandingFillDones = %d\n",pComponentPrivate->nOutStandingFillDones);

        
    if (pComponentPrivate->nOutStandingFillDones <= 0) {

        /* If there are any buffers still marked as pending they must have 
           been queued after the socket node was stopped */
        for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
            if (pComponentPrivate->pInputBufferList->bBufferPending[i]) 
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
                G729DEC_ClearPending(pComponentPrivate, pComponentPrivate->pInputBufferList->pBufHdr[i], OMX_DirInput);
            }
        }
        for (i=0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {
            if (pComponentPrivate->pOutputBufferList->bBufferPending[i]) 
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
                pComponentPrivate->nOutStandingFillDones--;
                G729DEC_ClearPending(pComponentPrivate, pComponentPrivate->pOutputBufferList->pBufHdr[i], OMX_DirOutput);
            }
        }
        G729DEC_DPRINT("Setting to OMX_StateIdle - Line %d\n",__LINE__);
        pComponentPrivate->curState = OMX_StateIdle;
                
#ifdef RESOURCE_MANAGER_ENABLED
        rm_error = RMProxy_NewSendCommand(pHandle, RMProxy_StateSet, OMX_G729_Decoder_COMPONENT, OMX_StateIdle, 3456,NULL);
#endif

        if (pComponentPrivate->bPreempted == 0) 
        {
            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                   OMX_EventCmdComplete,
                                                   OMX_CommandStateSet,
                                                   pComponentPrivate->curState,
                                                   NULL);
        }
        else
        {
            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                   OMX_EventError,
                                                   OMX_ErrorResourcesPreempted,
                                                   0,
                                                   NULL);

        }
        pComponentPrivate->bIdleCommandPending = OMX_FALSE;
    }
    return eError;
}


#ifdef RESOURCE_MANAGER_ENABLED
/***********************************
 *  Callback to the RM                                       *
 ***********************************/
void G729DEC_ResourceManagerCallback(RMPROXY_COMMANDDATATYPE cbData)
{
    OMX_COMMANDTYPE Cmd = OMX_CommandStateSet;
    OMX_STATETYPE state = OMX_StateIdle;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)cbData.hComponent;
    G729DEC_COMPONENT_PRIVATE *pCompPrivate = NULL;

    pCompPrivate = (G729DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

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
