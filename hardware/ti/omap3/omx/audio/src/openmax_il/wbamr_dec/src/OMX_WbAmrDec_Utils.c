
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
* @file OMX_WbAmrDec_Utils.c
*
* This file implements OMX Component for WBAMR decoder that
* is fully compliant with the OMX Audio specification 1.0.
*
* @path  $(CSLPATH)\
*
* @rev  0.1
*/
/* ----------------------------------------------------------------------------
*!
*! Revision History
*! ===================================
*! 10-Sept-2005 mf:  Initial Version. Change required per OMAPSWxxxxxxxxx
*! to provide _________________.
*!
*!
*! 10-Sept-2005 mf:
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
#include <stdlib.h>
#else
#include <wchar.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
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
#include "OMX_WbAmrDecoder.h"
#include "OMX_WbAmrDec_Utils.h"
#include "wbamrdecsocket_ti.h"
#include <decode_common_ti.h>
#include "OMX_WbAmrDec_ComponentThread.h"
#include "usn.h"
#include "LCML_DspCodec.h"

/* ========================================================================== */
/**
* @WBAMR_DEC_Fill_LCMLInitParams () This function is used by the component thread to
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

OMX_ERRORTYPE WBAMR_DEC_Fill_LCMLInitParams(OMX_HANDLETYPE pComponent,
                                            LCML_DSP *plcml_Init,
                                            OMX_U16 arr[])
{

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nIpBuf,nOpBuf;
    OMX_U32 nIpBufSize,nOpBufSize;

    OMX_U16 i;
    OMX_BUFFERHEADERTYPE *pTemp;
    int size_lcml;
    LCML_STRMATTR *strmAttr = NULL;
    LCML_DSP_INTERFACE *pHandle = (LCML_DSP_INTERFACE *)pComponent;
    WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate;
    LCML_WBAMR_DEC_BUFHEADERTYPE *pTemp_lcml;

    pComponentPrivate = pHandle->pComponentPrivate;
    OMX_PRINT1(pComponentPrivate->dbg, "WBAMR_DEC_Fill_LCMLInitParams\n ");
    OMX_PRDSP1(pComponentPrivate->dbg, "pHandle = %p\n",pHandle);
    OMX_PRDSP1(pComponentPrivate->dbg, "pHandle->pComponentPrivate = %p\n",pHandle->pComponentPrivate);

    nIpBuf = pComponentPrivate->pInputBufferList->numBuffers;
    pComponentPrivate->nRuntimeInputBuffers = nIpBuf;
    if(pComponentPrivate->mimemode == 1)
    {
        nIpBufSize = INPUT_WBAMRDEC_BUFFER_SIZE_MIME;
    }
    else if (pComponentPrivate->mimemode == 2)
    {
        nIpBufSize = INPUT_WBAMRDEC_BUFFER_SIZE_IF2;
    }
    else
    {
        if (OMX_AUDIO_AMRDTXasEFR == pComponentPrivate->iAmrMode){
            nIpBufSize = WBAMR_DEC_INPUT_BUFF_SIZE_EFR;
        }else{
            nIpBufSize = INPUT_WBAMRDEC_BUFFER_SIZE;
        }
        //nIpBufSize = INPUT_WBAMRDEC_BUFFER_SIZE;
    }

    nOpBuf = pComponentPrivate->pOutputBufferList->numBuffers;
    pComponentPrivate->nRuntimeOutputBuffers = nOpBuf;

    nOpBufSize = OUTPUT_WBAMRDEC_BUFFER_SIZE;


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

    plcml_Init->NodeInfo.AllUUIDs[0].uuid = &WBAMRDEC_SN_UUID;
    strcpy ((char*)plcml_Init->NodeInfo.AllUUIDs[0].DllName,WBAMR_DEC_DLL_NAME);

    plcml_Init->NodeInfo.AllUUIDs[0].eDllType = DLL_NODEOBJECT;

    plcml_Init->NodeInfo.AllUUIDs[1].uuid = &WBAMRDEC_SN_UUID;
    strcpy ((char*)plcml_Init->NodeInfo.AllUUIDs[1].DllName,WBAMR_DEC_DLL_NAME);
    plcml_Init->NodeInfo.AllUUIDs[1].eDllType = DLL_DEPENDENT;

    plcml_Init->NodeInfo.AllUUIDs[2].uuid = &USN_TI_UUID;
    strcpy ((char*)plcml_Init->NodeInfo.AllUUIDs[2].DllName,WBAMR_DEC_USN_DLL_NAME);
    plcml_Init->NodeInfo.AllUUIDs[2].eDllType = DLL_DEPENDENT;

    plcml_Init->DeviceInfo.TypeofDevice = 0; /*Initialisation for F2F mode*/

    if(pComponentPrivate->dasfmode == 1) {
        OMX_PRINT1(pComponentPrivate->dbg, "pComponentPrivate->dasfmode = %ld\n",pComponentPrivate->dasfmode);
        OMX_MALLOC_GENERIC(strmAttr, LCML_STRMATTR);

        pComponentPrivate->strmAttr = strmAttr;

        strmAttr->uSegid = 0;
        strmAttr->uAlignment = 0;
        strmAttr->uTimeout = WBAMR_DEC_TIMEOUT;
        strmAttr->uBufsize = OUTPUT_WBAMRDEC_BUFFER_SIZE; /*640*/
        strmAttr->uNumBufs = 2;
        strmAttr->lMode = STRMMODE_PROCCOPY;

        plcml_Init->DeviceInfo.TypeofDevice =1;
        plcml_Init->DeviceInfo.TypeofRender =0;
        if(pComponentPrivate->acdnmode == 1)
        {
            /* ACDN mode */
            plcml_Init->DeviceInfo.AllUUIDs[0].uuid = &ACDN_TI_UUID;
        }
        else
        {
            /* DASF/TeeDN mode */
            plcml_Init->DeviceInfo.AllUUIDs[0].uuid = &MMMDN_TI_UUID;
        }
        plcml_Init->DeviceInfo.DspStream = strmAttr;
    }
    else {
        pComponentPrivate->strmAttr = NULL;
    }


    /*copy the other information */
    plcml_Init->SegID = OMX_WBAMR_DEC_DEFAULT_SEGMENT;
    plcml_Init->Timeout = OMX_WBAMR_DEC_SN_TIMEOUT;
    plcml_Init->Alignment = 0;
    plcml_Init->Priority = OMX_WBAMR_DEC_SN_PRIORITY;
    plcml_Init->ProfileID = -1;/*0;*/

    /* TODO: Set this using SetParameter() */
    pComponentPrivate->iAmrSamplingFrequeny = WBAMR_DEC_SAMPLING_FREQUENCY;



    /*Accessing these 2 has the problem/creates problem in state transition tests*/
    pComponentPrivate->iAmrChannels =
        pComponentPrivate->wbamrParams[WBAMR_DEC_OUTPUT_PORT]->nChannels;

    pComponentPrivate->iAmrMode =
        pComponentPrivate->wbamrParams[WBAMR_DEC_INPUT_PORT]->eAMRDTXMode;

    if(pComponentPrivate->mimemode == 1)
    {
        pComponentPrivate->iAmrMimeFlag = WBAMR_DEC_MODE_MIME;
    } else {
        pComponentPrivate->iAmrMimeFlag = WBAMR_DEC_MODE_NONMIME;
    }



    arr[0] = WBAMR_DEC_STREAM_COUNT;
    arr[1] = WBAMR_DEC_INPUT_PORT;
    arr[2] = WBAMR_DEC_DMM;
    if (pComponentPrivate->pInputBufferList->numBuffers) {
        arr[3] = pComponentPrivate->pInputBufferList->numBuffers;
    }
    else {
        arr[3] = 1;
    }

    arr[4] = WBAMR_DEC_OUTPUT_PORT;

    if(pComponentPrivate->dasfmode == 1) {
        OMX_PRDSP2(pComponentPrivate->dbg, "Setting up create phase params for DASF mode\n");
        arr[5] = WBAMR_DEC_OUTSTRM;
        arr[6] = NUM_WBAMRDEC_OUTPUT_BUFFERS_DASF;
    }
    else {

        OMX_PRDSP2(pComponentPrivate->dbg, "Setting up create phase params for FILE mode\n");
        arr[5] = WBAMR_DEC_DMM;
        if (pComponentPrivate->pOutputBufferList->numBuffers) {
            arr[6] = pComponentPrivate->pOutputBufferList->numBuffers;
        }
        else {
            arr[6] = 1;
        }

    }

    if(pComponentPrivate->iAmrMode == OMX_AUDIO_AMRDTXasEFR) {
        arr[7] = WBAMR_EFR;
    }
    else {
        arr[7] = WBAMR;
    }

    arr[8] = pComponentPrivate->mimemode; /*MIME, IF2 or FORMATCONFORMANCE*/
    arr[9] = END_OF_CR_PHASE_ARGS;

    plcml_Init->pCrPhArgs = arr;

    OMX_PRINT1(pComponentPrivate->dbg, "Comp: OMX_AmrDecUtils.c\n");
    size_lcml = nIpBuf * sizeof(LCML_WBAMR_DEC_BUFHEADERTYPE);
    OMX_MALLOC_SIZE(pTemp_lcml, size_lcml,LCML_WBAMR_DEC_BUFHEADERTYPE);

    pComponentPrivate->pLcmlBufHeader[WBAMR_DEC_INPUT_PORT] = pTemp_lcml;

    for (i=0; i<nIpBuf; i++) {
        pTemp = pComponentPrivate->pInputBufferList->pBufHdr[i];
        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pTemp->nFilledLen = nIpBufSize;
        pTemp->nVersion.s.nVersionMajor = WBAMR_DEC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = WBAMR_DEC_MINOR_VER;
        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = WBAMR_DEC_NOT_USED;
        pTemp_lcml->buffer = pTemp;
        pTemp_lcml->eDir = OMX_DirInput;

        OMX_MALLOC_SIZE_DSPALIGN(pTemp_lcml->pBufferParam,
                                  sizeof(WBAMRDEC_ParamStruct),
                                  WBAMRDEC_ParamStruct);
        pTemp_lcml->pBufferParam->usNbFrames =0;
        pTemp_lcml->pBufferParam->pParamElem = NULL;
        pTemp_lcml->pFrameParam = NULL;

        OMX_MALLOC_GENERIC(pTemp_lcml->pDmmBuf, DMM_BUFFER_OBJ);

        /* This means, it is not a last buffer. This flag is to be modified by
         * the application to indicate the last buffer */
        pTemp->nFlags = WBAMR_DEC_NORMAL_BUFFER;

        pTemp_lcml++;
    }

    /* Allocate memory for all output buffer headers..
     * This memory pointer will be sent to LCML */
    size_lcml = nOpBuf * sizeof(LCML_WBAMR_DEC_BUFHEADERTYPE);
    OMX_MALLOC_SIZE(pTemp_lcml, size_lcml,LCML_WBAMR_DEC_BUFHEADERTYPE);
    pComponentPrivate->pLcmlBufHeader[WBAMR_DEC_OUTPUT_PORT] = pTemp_lcml;

    for (i=0; i<nOpBuf; i++) {
        pTemp = pComponentPrivate->pOutputBufferList->pBufHdr[i];
        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pTemp->nFilledLen = nOpBufSize;
        pTemp->nVersion.s.nVersionMajor = WBAMR_DEC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = WBAMR_DEC_MINOR_VER;
        pComponentPrivate->nVersion = pTemp->nVersion.nVersion;
        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = WBAMR_DEC_NOT_USED;
        /* This means, it is not a last buffer. This flag is to be modified by
         * the application to indicate the last buffer */

        pTemp_lcml->buffer = pTemp;
        pTemp_lcml->eDir = OMX_DirOutput;
        pTemp_lcml->pFrameParam = NULL;

        OMX_MALLOC_SIZE_DSPALIGN(pTemp_lcml->pBufferParam,
                                  sizeof(WBAMRDEC_ParamStruct),
                                  WBAMRDEC_ParamStruct);

        pTemp_lcml->pBufferParam->usNbFrames =0;
        pTemp_lcml->pBufferParam->pParamElem = NULL;
        OMX_MALLOC_GENERIC(pTemp_lcml->pDmmBuf, DMM_BUFFER_OBJ);

        OMX_PRDSP1(pComponentPrivate->dbg, "pTemp_lcml = %p\n",pTemp_lcml);
        OMX_PRBUFFER1(pComponentPrivate->dbg, "pTemp_lcml->buffer = %p\n",pTemp_lcml->buffer);

        pTemp->nFlags = WBAMR_DEC_NORMAL_BUFFER;

        pTemp++;
        pTemp_lcml++;
    }
    pComponentPrivate->bPortDefsAllocated = 1;
#ifdef __PERF_INSTRUMENTATION__
    pComponentPrivate->nLcml_nCntIp = 0;
    pComponentPrivate->nLcml_nCntOpReceived = 0;
#endif

    OMX_PRINT1(pComponentPrivate->dbg, "Exiting WBAMR_DEC_Fill_LCMLInitParams");

    pComponentPrivate->bInitParamsInitialized = 1;
 EXIT:
    return eError;
}


/* ========================================================================== */
/**
* @WBAMR_DEC_StartComponentThread() This function is called by the component to create
* the component thread, command pipe, data pipe and LCML Pipe.
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

OMX_ERRORTYPE WBAMR_DEC_StartComponentThread(OMX_HANDLETYPE pComponent)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate =
        (WBAMR_DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
#ifdef UNDER_CE
    pthread_attr_t attr;
    memset(&attr, 0, sizeof(attr));
    attr.__inheritsched = PTHREAD_EXPLICIT_SCHED;
    attr.__schedparam.__sched_priority = OMX_AUDIO_DECODER_THREAD_PRIORITY;
#endif

    OMX_PRINT1(pComponentPrivate->dbg, "Inside  WBAMR_DEC_StartComponentThread\n");

    /* Initialize all the variables*/
    pComponentPrivate->bIsStopping = 0;
    pComponentPrivate->lcml_nOpBuf = 0;
    pComponentPrivate->lcml_nIpBuf = 0;
    pComponentPrivate->app_nBuf = 0;
    pComponentPrivate->num_Reclaimed_Op_Buff = 0;

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

    /* create the pipe used to send commands to the thread */
    /* Create the Component Thread */
#ifdef UNDER_CE
    eError = pthread_create (&(pComponentPrivate->WBAMR_DEC_ComponentThread), &attr, WBAMR_DEC_ComponentThread, pComponentPrivate);
#else
    eError = pthread_create (&(pComponentPrivate->WBAMR_DEC_ComponentThread), NULL, WBAMR_DEC_ComponentThread, pComponentPrivate);
#endif
    if (eError || !pComponentPrivate->WBAMR_DEC_ComponentThread) {
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    pComponentPrivate->bCompThreadStarted = 1;
 EXIT:
    return eError;
}

/* ========================================================================== */
/**
* @WBAMR_DEC_FreeCompResources() This function is called by the component during
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

OMX_ERRORTYPE WBAMR_DEC_FreeCompResources(OMX_HANDLETYPE pComponent)
{
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate = (WBAMR_DEC_COMPONENT_PRIVATE *)
        pHandle->pComponentPrivate;

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_ERRORTYPE err = OMX_ErrorNone;
    OMX_U32 nIpBuf = 0;
    OMX_U32 nOpBuf = 0;

    OMX_PRINT1(pComponentPrivate->dbg, "WBAMR_DEC_FreeCompResources\n");

    if (pComponentPrivate->bPortDefsAllocated) {
        nIpBuf = pComponentPrivate->pPortDef[WBAMR_DEC_INPUT_PORT]->nBufferCountActual;
        nOpBuf = pComponentPrivate->pPortDef[WBAMR_DEC_OUTPUT_PORT]->nBufferCountActual;
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
    OMX_MEMFREE_STRUCT (pComponentPrivate->pPriorityMgmt);
    OMX_MEMFREE_STRUCT (pComponentPrivate->pHoldBuffer);
    OMX_MEMFREE_STRUCT(pComponentPrivate->pPortDef[WBAMR_DEC_INPUT_PORT]);
    OMX_MEMFREE_STRUCT (pComponentPrivate->pPortDef[WBAMR_DEC_OUTPUT_PORT]);
    OMX_MEMFREE_STRUCT(pComponentPrivate->wbamrParams[WBAMR_DEC_INPUT_PORT]);
    OMX_MEMFREE_STRUCT (pComponentPrivate->wbamrParams[WBAMR_DEC_OUTPUT_PORT]);
    pComponentPrivate->bPortDefsAllocated = 0;
#ifndef UNDER_CE
    OMX_PRDSP1(pComponentPrivate->dbg, "\n\n FreeCompResources: Destroying mutexes.\n\n");
    pthread_mutex_destroy(&pComponentPrivate->InLoaded_mutex);
    pthread_cond_destroy(&pComponentPrivate->InLoaded_threshold);

    pthread_mutex_destroy(&pComponentPrivate->InIdle_mutex);
    pthread_cond_destroy(&pComponentPrivate->InIdle_threshold);

    pthread_mutex_destroy(&pComponentPrivate->AlloBuf_mutex);
    pthread_cond_destroy(&pComponentPrivate->AlloBuf_threshold);
#else
    pComponentPrivate->bPortDefsAllocated = 0;
    OMX_DestroyEvent(&(pComponentPrivate->InLoaded_event));
    OMX_DestroyEvent(&(pComponentPrivate->InIdle_event));
    OMX_DestroyEvent(&(pComponentPrivate->AlloBuf_event));
#endif
    return eError;
}



OMX_ERRORTYPE WBAMR_DEC_CleanupInitParams(OMX_HANDLETYPE pComponent)
{
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate = (WBAMR_DEC_COMPONENT_PRIVATE *)
        pHandle->pComponentPrivate;

    LCML_WBAMR_DEC_BUFHEADERTYPE *pTemp_lcml;

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nIpBuf = 0;
    OMX_U16 i=0;
    LCML_DSP_INTERFACE *pLcmlHandle;
    LCML_DSP_INTERFACE *pLcmlHandleAux;
    OMX_PRINT1(pComponentPrivate->dbg, "WBAMR_DEC_CleanupInitParams()\n");

    OMX_MEMFREE_STRUCT(pComponentPrivate->strmAttr);

    OMX_MEMFREE_STRUCT_DSPALIGN(pComponentPrivate->pParams, WBAMR_DEC_AudioCodecParams);

    nIpBuf = pComponentPrivate->nRuntimeInputBuffers;
    pTemp_lcml = pComponentPrivate->pLcmlBufHeader[WBAMR_DEC_INPUT_PORT];
    for(i=0; i<nIpBuf; i++) {

        if(pTemp_lcml->pFrameParam!=NULL){
            OMX_MEMFREE_STRUCT_DSPALIGN(pTemp_lcml->pFrameParam, WAMRDEC_FrameStruct);
            pLcmlHandle = (LCML_DSP_INTERFACE *)pComponentPrivate->pLcmlHandle;
            pLcmlHandleAux = (LCML_DSP_INTERFACE *)(((LCML_CODEC_INTERFACE *)pLcmlHandle->pCodecinterfacehandle)->pCodec);
            OMX_DmmUnMap(pLcmlHandleAux->dspCodec->hProc,
                         (void*)pTemp_lcml->pBufferParam->pParamElem,
                         pTemp_lcml->pDmmBuf->pReserved, pComponentPrivate->dbg);
        }

        OMX_MEMFREE_STRUCT_DSPALIGN(pTemp_lcml->pBufferParam, WBAMRDEC_ParamStruct);
        OMX_MEMFREE_STRUCT(pTemp_lcml->pDmmBuf);
        pTemp_lcml++;
    }

    /*Output*/
    pTemp_lcml = pComponentPrivate->pLcmlBufHeader[WBAMR_DEC_OUTPUT_PORT];
    for(i=0; i<pComponentPrivate->nRuntimeOutputBuffers; i++){

        if(pTemp_lcml->pFrameParam!=NULL){
            OMX_MEMFREE_STRUCT_DSPALIGN(pTemp_lcml->pFrameParam, WAMRDEC_FrameStruct);
            pLcmlHandle = (LCML_DSP_INTERFACE *)pComponentPrivate->pLcmlHandle;
            pLcmlHandleAux = (LCML_DSP_INTERFACE *)(((LCML_CODEC_INTERFACE *)pLcmlHandle->pCodecinterfacehandle)->pCodec);
            OMX_DmmUnMap(pLcmlHandleAux->dspCodec->hProc,
                         (void*)pTemp_lcml->pBufferParam->pParamElem,
                         pTemp_lcml->pDmmBuf->pReserved, pComponentPrivate->dbg);
        }

        OMX_MEMFREE_STRUCT_DSPALIGN(pTemp_lcml->pBufferParam, WBAMRDEC_ParamStruct);
        OMX_MEMFREE_STRUCT(pTemp_lcml->pDmmBuf);
        pTemp_lcml++;
    }

    OMX_MEMFREE_STRUCT(pComponentPrivate->pLcmlBufHeader[WBAMR_DEC_INPUT_PORT]);
    OMX_MEMFREE_STRUCT(pComponentPrivate->pLcmlBufHeader[WBAMR_DEC_OUTPUT_PORT]);
    return eError;
}

/* ========================================================================== */
/**
* @WBAMR_DEC_StopComponentThread() This function is called by the component during
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

OMX_ERRORTYPE WBAMR_DEC_StopComponentThread(OMX_HANDLETYPE pComponent)
{
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate = (WBAMR_DEC_COMPONENT_PRIVATE *)
        pHandle->pComponentPrivate;

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_ERRORTYPE threadError = OMX_ErrorNone;
    int pthreadError = 0;

    /*Join the component thread */
    pComponentPrivate->bIsStopping = 1;
    write (pComponentPrivate->cmdPipe[1], &pComponentPrivate->bIsStopping, sizeof(OMX_U16));
    pthreadError = pthread_join (pComponentPrivate->WBAMR_DEC_ComponentThread,
                                 (void*)&threadError);
    if (0 != pthreadError) {
        eError = OMX_ErrorHardware;
    }

    /*Check for the errors */
    if (OMX_ErrorNone != threadError && OMX_ErrorNone != eError) {
        eError = OMX_ErrorInsufficientResources;
        OMX_ERROR4(pComponentPrivate->dbg, "Error while closing Component Thread\n");
    }
    return eError;
}


/* ========================================================================== */
/**
* @WBAMR_DEC_HandleCommand() This function is called by the component when ever it
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

OMX_U32 WBAMR_DEC_HandleCommand (WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate)
{

    OMX_COMPONENTTYPE *pHandle;
    OMX_COMMANDTYPE command;
    OMX_STATETYPE commandedState;
    OMX_U32 commandData;
    OMX_HANDLETYPE pLcmlHandle = pComponentPrivate->pLcmlHandle;

    OMX_U16 i;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    OMX_U32 nBuf;
    OMX_U16 arr[100];
    char *p = "damedesuStr";

    LCML_CALLBACKTYPE cb;
    LCML_DSP *pLcmlDsp;
    WBAMR_DEC_AudioCodecParams *pParams;
    int ret;
    LCML_WBAMR_DEC_BUFHEADERTYPE *pLcmlHdr = NULL;
    int inputPortFlag=0,outputPortFlag=0;

#ifdef RESOURCE_MANAGER_ENABLED
    OMX_ERRORTYPE rm_error;
#endif

    pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;

    OMX_PRINT1(pComponentPrivate->dbg, "Entering WBAMR_DEC_HandleCommand Function - curState = %d\n",pComponentPrivate->curState);
    OMX_PRINT1(pComponentPrivate->dbg, "pComponentPrivate = %p\n",pComponentPrivate);
    OMX_PRDSP1(pComponentPrivate->dbg, "pHandle = %p\n",pHandle);
    ret = read (pComponentPrivate->cmdPipe[0], &command, sizeof (command));
    if (ret == -1) {
        OMX_ERROR4(pComponentPrivate->dbg, "Error While reading from the Pipe\n");
        eError = OMX_ErrorHardware;
        pComponentPrivate->cbInfo.EventHandler (pHandle,
                                                pHandle->pApplicationPrivate,
                                                OMX_EventError,
                                                eError,
                                                OMX_TI_ErrorSevere,
                                                NULL);
        goto EXIT;
    }

    ret = read (pComponentPrivate->cmdDataPipe[0], &commandData, sizeof (commandData));
    if (ret == -1) {
        OMX_ERROR4(pComponentPrivate->dbg, "Error While reading from the Pipe\n");
        eError = OMX_ErrorHardware;
        pComponentPrivate->cbInfo.EventHandler (pHandle,
                                                pHandle->pApplicationPrivate,
                                                OMX_EventError,
                                                eError,
                                                OMX_TI_ErrorSevere,
                                                NULL);
        goto EXIT;
    }
#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedCommand(pComponentPrivate->pPERFcomp,
                         command,
                         commandData,
                         PERF_ModuleLLMM);
#endif

    if (command == OMX_CommandStateSet) {
        commandedState = (OMX_STATETYPE)commandData;
        switch(commandedState) {
        case OMX_StateIdle:
            if (pComponentPrivate->curState == commandedState){
                pComponentPrivate->cbInfo.EventHandler (pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventError,
                                                        OMX_ErrorSameState,
                                                        OMX_TI_ErrorMinor,
                                                        NULL);
            }
            else if (pComponentPrivate->curState == OMX_StateLoaded || pComponentPrivate->curState == OMX_StateWaitForResources) {

                /*    if (pComponentPrivate->dasfmode == 1) {
                      if(pComponentPrivate->streamID == 0)
                      {
                      WBAMR_DEC_EPRINT("**************************************\n");
                      WBAMR_DEC_EPRINT(":: Error = OMX_ErrorInsufficientResources\n");
                      WBAMR_DEC_EPRINT("**************************************\n");
                      eError = OMX_ErrorInsufficientResources;
                      pComponentPrivate->curState = OMX_StateInvalid;
                      pComponentPrivate->cbInfo.EventHandler(
                      pHandle, pHandle->pApplicationPrivate,
                      OMX_EventError, OMX_ErrorInvalidState,OMX_TI_ErrorMajor,
                      "AM: No Stream ID Available");
                      goto EXIT;
                      }
                      }                */

#ifdef __PERF_INSTRUMENTATION__
                PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryStart | PERF_BoundarySetup);
#endif
                if (pComponentPrivate->pPortDef[WBAMR_DEC_INPUT_PORT]->bPopulated &&  pComponentPrivate->pPortDef[WBAMR_DEC_INPUT_PORT]->bEnabled)  {
                    inputPortFlag = 1;
                }

                if (!pComponentPrivate->pPortDef[WBAMR_DEC_INPUT_PORT]->bPopulated && !pComponentPrivate->pPortDef[WBAMR_DEC_INPUT_PORT]->bEnabled) {
                    inputPortFlag = 1;
                }

                if (pComponentPrivate->pPortDef[WBAMR_DEC_OUTPUT_PORT]->bPopulated && pComponentPrivate->pPortDef[WBAMR_DEC_OUTPUT_PORT]->bEnabled) {
                    outputPortFlag = 1;
                }

                if (!pComponentPrivate->pPortDef[WBAMR_DEC_OUTPUT_PORT]->bPopulated && !pComponentPrivate->pPortDef[WBAMR_DEC_OUTPUT_PORT]->bEnabled) {
                    outputPortFlag = 1;
                }

                if (!(inputPortFlag && outputPortFlag)) {
                    OMX_PRSTATE2(pComponentPrivate->dbg, "\n\n HandleCommand: In Loaded state. It does not go to sleep.\n\n");
                    pComponentPrivate->InLoaded_readytoidle = 1;
#ifndef UNDER_CE
                    pthread_mutex_lock(&pComponentPrivate->InLoaded_mutex);
                    pthread_cond_wait(&pComponentPrivate->InLoaded_threshold, &pComponentPrivate->InLoaded_mutex);
                    pthread_mutex_unlock(&pComponentPrivate->InLoaded_mutex);
#else
                    OMX_WaitForEvent(&(pComponentPrivate->InLoaded_event));
#endif
                }


                OMX_PRINT1(pComponentPrivate->dbg, "Inside WBAMR_DEC_HandleCommand\n");
                cb.LCML_Callback = (void *) WBAMR_DEC_LCML_Callback;
                pLcmlHandle = (OMX_HANDLETYPE) WBAMR_DEC_GetLCMLHandle(pComponentPrivate);
                OMX_PRINT1(pComponentPrivate->dbg, "Inside WBAMR_DEC_HandleCommand\n");

                if (pLcmlHandle == NULL) {
                    OMX_ERROR4(pComponentPrivate->dbg, "LCML Handle is NULL........exiting..\n");
                    pComponentPrivate->curState = OMX_StateInvalid;
                    eError = OMX_ErrorHardware;
                    pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventError,
                                                           OMX_ErrorHardware,
                                                           OMX_TI_ErrorSevere,
                                                           "Lcml Handle NULL");
                    goto EXIT;
                }
                OMX_PRINT1(pComponentPrivate->dbg, "WBAMR_DEC_HandleCommand\n");
                OMX_PRDSP1(pComponentPrivate->dbg, "pLcmlHandle = %p\n",pLcmlHandle);

                /* Got handle of dsp via phandle filling information about DSP
                   specific things */
                pLcmlDsp = (((LCML_DSP_INTERFACE*)pLcmlHandle)->dspCodec);
                OMX_PRDSP1(pComponentPrivate->dbg, "pLcmlDsp = %p\n",pLcmlDsp);

                OMX_PRINT1(pComponentPrivate->dbg, "WBAMR_DEC_HandleCommand\n");
                eError = WBAMR_DEC_Fill_LCMLInitParams(pHandle, pLcmlDsp, arr);
                if(eError != OMX_ErrorNone) {
                    OMX_ERROR4(pComponentPrivate->dbg, "Error returned from\
                                    WBAMR_DEC_Fill_LCMLInitParams()\n");
                    pComponentPrivate->curState = OMX_StateInvalid;
                    pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventError,
                                                           eError,
                                                           OMX_TI_ErrorSevere,
                                                           NULL);
                    goto EXIT;
                }

                OMX_PRINT2(pComponentPrivate->dbg, "Comp: OMX_AmrDecUtils.c\n");
                pComponentPrivate->pLcmlHandle = (LCML_DSP_INTERFACE *)pLcmlHandle;

                OMX_PRDSP2(pComponentPrivate->dbg, "Calling LCML_InitMMCodec...\n");
#ifndef UNDER_CE
                /* TeeDN will be default for decoder component */
                OMX_PRINT2(pComponentPrivate->dbg, "WBAMR decoder support TeeDN\n");
                eError = LCML_InitMMCodecEx(((LCML_DSP_INTERFACE *)pLcmlHandle)->pCodecinterfacehandle,
                                            p,&pLcmlHandle,(void *)p,&cb, (OMX_STRING)pComponentPrivate->sDeviceString);

#else

                eError = LCML_InitMMCodec(((LCML_DSP_INTERFACE *)pLcmlHandle)->pCodecinterfacehandle,
                                          p,&pLcmlHandle,(void *)p,&cb);

#endif

                if(eError != OMX_ErrorNone) {
                    OMX_ERROR4(pComponentPrivate->dbg, "Error returned from\
                        LCML_Init()\n");
                    goto EXIT;
                }



#ifdef RESOURCE_MANAGER_ENABLED
                /* need check the resource with RM */

                pComponentPrivate->rmproxyCallback.RMPROXY_Callback = (void *) WBAMRDEC_ResourceManagerCallback;
                if (pComponentPrivate->curState != OMX_StateWaitForResources) {
                    OMX_PRDSP2(pComponentPrivate->dbg, "AMRDEC: About to call RMProxy_SendCommand\n");
                    rm_error = RMProxy_NewSendCommand(pHandle,
                                                      RMProxy_RequestResource,
                                                      OMX_WBAMR_Decoder_COMPONENT,
                                                      WBAMR_DEC_CPU_LOAD,
                                                      3456,
                                                      &(pComponentPrivate->rmproxyCallback));
                    OMX_PRINT2(pComponentPrivate->dbg, "AMRDEC: RMProxy_SendCommand returned %d\n",rm_error);
                    if(rm_error == OMX_ErrorNone) {
                        /* resource is available */
                        pComponentPrivate->curState = OMX_StateIdle;
                        pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                               pHandle->pApplicationPrivate,
                                                               OMX_EventCmdComplete,
                                                               OMX_CommandStateSet,
                                                               pComponentPrivate->curState,
                                                               NULL);
                        rm_error = RMProxy_NewSendCommand(pHandle,
                                                          RMProxy_StateSet,
                                                          OMX_WBAMR_Decoder_COMPONENT,
                                                          OMX_StateIdle,
                                                          3456,
                                                          NULL);
                    }else if(rm_error == OMX_ErrorInsufficientResources) {
                        /* resource is not available, need set state to OMX_StateWaitForResources */
                        pComponentPrivate->curState = OMX_StateWaitForResources;
                        pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                               pHandle->pApplicationPrivate,
                                                               OMX_EventCmdComplete,
                                                               OMX_CommandStateSet,
                                                               pComponentPrivate->curState,
                                                               NULL);
                        OMX_ERROR4(pComponentPrivate->dbg, "AMRDEC: Error - insufficient resources\n");
                    }
                }else{
                    pComponentPrivate->curState = OMX_StateIdle;
                    pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventCmdComplete,
                                                           OMX_CommandStateSet,
                                                           pComponentPrivate->curState,
                                                           NULL);
                    rm_error = RMProxy_NewSendCommand(pHandle,
                                                      RMProxy_StateSet,
                                                      OMX_WBAMR_Decoder_COMPONENT,
                                                      OMX_StateIdle,
                                                      3456,
                                                      NULL);
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
                OMX_PRSTATE2(pComponentPrivate->dbg, "AMRDEC: State has been Set to Idle\n");

                if(pComponentPrivate->dasfmode == 1) {
                    OMX_U32 pValues[4];
                    OMX_PRDSP2(pComponentPrivate->dbg, "---- Comp: DASF Functionality is ON ---\n");
                    if(pComponentPrivate->streamID == 0)
                    {
                        OMX_ERROR4(pComponentPrivate->dbg, "**************************************\n");
                        OMX_ERROR4(pComponentPrivate->dbg, ":: Error = OMX_ErrorInsufficientResources\n");
                        OMX_ERROR4(pComponentPrivate->dbg, "**************************************\n");
                        eError = OMX_ErrorInsufficientResources;
                        pComponentPrivate->curState = OMX_StateInvalid;
                        pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                               pHandle->pApplicationPrivate,
                                                               OMX_EventError,
                                                               OMX_ErrorInvalidState,
                                                               OMX_TI_ErrorMajor,
                                                               "AM: No Stream ID Available");
                        goto EXIT;
                    }

                    OMX_MALLOC_SIZE_DSPALIGN(pComponentPrivate->pParams, sizeof(WBAMR_DEC_AudioCodecParams),WBAMR_DEC_AudioCodecParams);
                    pParams = pComponentPrivate->pParams;

                    pParams->iAudioFormat = 1;
                    pParams->iSamplingRate = 16000;

                    pParams->iStrmId = pComponentPrivate->streamID;

                    pValues[0] = USN_STRMCMD_SETCODECPARAMS;
                    pValues[1] = (OMX_U32)pParams;
                    pValues[2] = sizeof(WBAMR_DEC_AudioCodecParams);
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               EMMCodecControlStrmCtrl,(void *)pValues);

                    if(eError != OMX_ErrorNone) {
                        OMX_ERROR4(pComponentPrivate->dbg, "Error Occurred in Codec StreamControl..\n");
                        pComponentPrivate->curState = OMX_StateInvalid;
                        pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                               pHandle->pApplicationPrivate,
                                                               OMX_EventError,
                                                               eError,
                                                               OMX_TI_ErrorSevere,
                                                               NULL);
                        goto EXIT;
                    }
                }
            }
            else if (pComponentPrivate->curState == OMX_StateExecuting) {
                char *pArgs = "damedesuStr";
#ifdef __PERF_INSTRUMENTATION__
                PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryComplete | PERF_BoundarySteadyState);
#endif
                /*Set the bIsStopping bit */
                OMX_PRDSP2(pComponentPrivate->dbg, "AMRDEC: About to set bIsStopping bit\n");

                OMX_PRDSP2(pComponentPrivate->dbg, "About to call LCML_ControlCodec(STOP)\n");

                if (pComponentPrivate->codecStop_waitingsignal == 0){ 
                    pthread_mutex_lock(&pComponentPrivate->codecStop_mutex);    
                }
                eError = LCML_ControlCodec(
                                           ((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                           MMCodecControlStop,(void *)pArgs);
                if (pComponentPrivate->codecStop_waitingsignal == 0){
                    pthread_cond_wait(&pComponentPrivate->codecStop_threshold, &pComponentPrivate->codecStop_mutex);
                    pComponentPrivate->codecStop_waitingsignal = 0;
                    pthread_mutex_unlock(&pComponentPrivate->codecStop_mutex);
                }
                if(eError != OMX_ErrorNone) {
                    OMX_ERROR4(pComponentPrivate->dbg, "Error Occurred in Codec Stop..\n");
                    pComponentPrivate->curState = OMX_StateInvalid;
                    pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventError,
                                                           eError,
                                                           OMX_TI_ErrorSevere,
                                                           NULL);
                    goto EXIT;
                }
                pComponentPrivate->bStopSent=1;
                OMX_MEMFREE_STRUCT(pComponentPrivate->pHoldBuffer);
                pComponentPrivate->nHoldLength = 0;
            }

            else if(pComponentPrivate->curState == OMX_StatePause) {
                char *pArgs = "damedesuStr";
                if (pComponentPrivate->codecStop_waitingsignal == 0){ 
                    pthread_mutex_lock(&pComponentPrivate->codecStop_mutex);    
                }
                eError = LCML_ControlCodec(
                                           ((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                           MMCodecControlStop,(void *)pArgs);
                if (pComponentPrivate->codecStop_waitingsignal == 0){
                    pthread_cond_wait(&pComponentPrivate->codecStop_threshold, &pComponentPrivate->codecStop_mutex);
                    pComponentPrivate->codecStop_waitingsignal = 0;
                    pthread_mutex_unlock(&pComponentPrivate->codecStop_mutex);
                }
                if(eError != OMX_ErrorNone) {
                    OMX_ERROR4(pComponentPrivate->dbg, "Error Occurred in Codec Stop..\n");
                    pComponentPrivate->curState = OMX_StateInvalid;
                    pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventError,
                                                           eError,
                                                           OMX_TI_ErrorSevere,
                                                           NULL);
                    goto EXIT;
                }

#ifdef __PERF_INSTRUMENTATION__
                PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryComplete | PERF_BoundarySteadyState);
#endif
                OMX_PRINT1(pComponentPrivate->dbg, "Comp: Stop Command Received\n");
                pComponentPrivate->curState = OMX_StateIdle;
#ifdef RESOURCE_MANAGER_ENABLED
                rm_error = RMProxy_NewSendCommand(pHandle, RMProxy_StateSet, OMX_WBAMR_Decoder_COMPONENT, OMX_StateIdle, 3456, NULL);
#endif
                OMX_PRINT2(pComponentPrivate->dbg, "OMX_AmrDec_Utils.c :: The component is stopped\n");
                pComponentPrivate->cbInfo.EventHandler (
                                                        pHandle,pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,OMX_CommandStateSet,pComponentPrivate->curState,
                                                        NULL);
            }
            else {
                /* This means, it is invalid state from application */
                OMX_PRINT2(pComponentPrivate->dbg, "Comp: OMX_AmrDecUtils.c\n");
                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventError,
                                                       OMX_ErrorIncorrectStateTransition,
                                                       OMX_TI_ErrorMinor,
                                                       "Invalid State");
            }
            break;

        case OMX_StateExecuting:
            OMX_PRDSP2(pComponentPrivate->dbg, "WBAMR_DEC_HandleCommand: Cmd Executing \n");
            if (pComponentPrivate->curState == commandedState){
                pComponentPrivate->cbInfo.EventHandler (pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventError,
                                                        OMX_ErrorSameState,
                                                        OMX_TI_ErrorMinor,
                                                        "Invalid State");
                OMX_ERROR4(pComponentPrivate->dbg, "Error: Same State Given by \
                           Application\n");
                goto EXIT;

            }
            else if (pComponentPrivate->curState == OMX_StateIdle) {
                OMX_PRINT2(pComponentPrivate->dbg, "Comp: OMX_AmrDecUtils.c\n");

                pComponentPrivate->nFillThisBufferCount = 0;
                pComponentPrivate->nFillBufferDoneCount = 0;
                pComponentPrivate->bStopSent=0;

                pComponentPrivate->nEmptyBufferDoneCount = 0;
                pComponentPrivate->nEmptyThisBufferCount = 0;

                /* Sending commands to DSP via LCML_ControlCodec third argument
                   is not used for time being */

                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                           EMMCodecControlStart, (void *)p);

                if(eError != OMX_ErrorNone) {
                    OMX_ERROR4(pComponentPrivate->dbg, "Error Occurred in Codec Start..\n");
                    pComponentPrivate->curState = OMX_StateInvalid;
                    pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventError,
                                                           eError,
                                                           OMX_TI_ErrorSevere,
                                                           NULL);
                    goto EXIT;
                }
                /* Send input buffers to application */
                nBuf = pComponentPrivate->pInputBufferList->numBuffers;

                OMX_PRBUFFER2(pComponentPrivate->dbg, "nBuf =  %ld\n",nBuf);
                /* Send output buffers to codec */
            }
            else if (pComponentPrivate->curState == OMX_StatePause) {
                OMX_PRINT2(pComponentPrivate->dbg, "Comp: OMX_AmrDecUtils.c\n");
                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                           EMMCodecControlStart, (void *)p);
                if (eError != OMX_ErrorNone) {
                    OMX_ERROR4(pComponentPrivate->dbg, "Error While Resuming the codec\n");
                    pComponentPrivate->curState = OMX_StateInvalid;
                    pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventError,
                                                           eError,
                                                           OMX_TI_ErrorSevere,
                                                           NULL);
                    goto EXIT;
                }

                for (i=0; i < pComponentPrivate->nNumInputBufPending; i++) {
                    if (pComponentPrivate->pInputBufHdrPending[i]) {
                        WBAMR_DEC_GetCorresponding_LCMLHeader(pComponentPrivate,pComponentPrivate->pInputBufHdrPending[i]->pBuffer, OMX_DirInput, &pLcmlHdr);
                        WBAMR_DEC_SetPending(pComponentPrivate,pComponentPrivate->pInputBufHdrPending[i],OMX_DirInput,__LINE__);

                        eError = LCML_QueueBuffer(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                  EMMCodecInputBuffer,
                                                  pComponentPrivate->pInputBufHdrPending[i]->pBuffer,
                                                  pComponentPrivate->pInputBufHdrPending[i]->nAllocLen,
                                                  pComponentPrivate->pInputBufHdrPending[i]->nFilledLen,
                                                  (OMX_U8 *) pLcmlHdr->pBufferParam,
                                                  sizeof(WBAMRDEC_ParamStruct),
                                                  NULL);
                    }
                }
                pComponentPrivate->nNumInputBufPending = 0;

                /*                if (pComponentPrivate->nNumOutputBufPending < pComponentPrivate->pOutputBufferList->numBuffers) {
                                  pComponentPrivate->nNumOutputBufPending = pComponentPrivate->pOutputBufferList->numBuffers;
                                  }
                */
                for (i=0; i < pComponentPrivate->nNumOutputBufPending; i++) {
                    if (pComponentPrivate->pOutputBufHdrPending[i]) {
                        WBAMR_DEC_GetCorresponding_LCMLHeader(pComponentPrivate,pComponentPrivate->pOutputBufHdrPending[i]->pBuffer, OMX_DirOutput, &pLcmlHdr);
                        WBAMR_DEC_SetPending(pComponentPrivate,pComponentPrivate->pOutputBufHdrPending[i],OMX_DirOutput,__LINE__);
                        eError = LCML_QueueBuffer(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                  EMMCodecOuputBuffer,
                                                  pComponentPrivate->pOutputBufHdrPending[i]->pBuffer,
                                                  pComponentPrivate->pOutputBufHdrPending[i]->nAllocLen,
                                                  pComponentPrivate->pOutputBufHdrPending[i]->nFilledLen,
                                                  (OMX_U8 *) pLcmlHdr->pBufferParam,
                                                  sizeof(WBAMRDEC_ParamStruct),
                                                  NULL);
                    }

                }
                pComponentPrivate->nNumOutputBufPending = 0;
            }
            else {
                OMX_PRINT2(pComponentPrivate->dbg, "Comp: OMX_AmrDecUtils.c\n");
                pComponentPrivate->cbInfo.EventHandler (pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventError,
                                                        OMX_ErrorIncorrectStateTransition,
                                                        OMX_TI_ErrorMinor,
                                                        "Incorrect State Transition");
                OMX_ERROR4(pComponentPrivate->dbg, "Error: Invalid State Given by \
                       Application\n");
                goto EXIT;

            }
#ifdef RESOURCE_MANAGER_ENABLED
            rm_error = RMProxy_NewSendCommand(pHandle, RMProxy_StateSet, OMX_WBAMR_Decoder_COMPONENT, OMX_StateExecuting, 3456, NULL);

#endif
            pComponentPrivate->curState = OMX_StateExecuting;
#ifdef __PERF_INSTRUMENTATION__
            PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryStart | PERF_BoundarySteadyState);
#endif
            /*Send state change notificaiton to Application */
            pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                   pHandle->pApplicationPrivate,
                                                   OMX_EventCmdComplete,
                                                   OMX_CommandStateSet,
                                                   pComponentPrivate->curState,
                                                   NULL);
            break;

        case OMX_StateLoaded:
            OMX_PRSTATE2(pComponentPrivate->dbg, "WBAMR_DEC_HandleCommand: Cmd Loaded - curState = %d\n",pComponentPrivate->curState);
            if (pComponentPrivate->curState == commandedState){
                pComponentPrivate->cbInfo.EventHandler (pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventError,
                                                        OMX_ErrorSameState,
                                                        OMX_TI_ErrorMinor,
                                                        "Same State");
                OMX_ERROR2(pComponentPrivate->dbg, "Error: Same State Given by \
                           Application\n");
                break;
            }
            OMX_PRBUFFER2(pComponentPrivate->dbg, "pComponentPrivate->pInputBufferList->numBuffers = %d\n",pComponentPrivate->pInputBufferList->numBuffers);
            OMX_PRBUFFER2(pComponentPrivate->dbg, "pComponentPrivate->pOutputBufferList->numBuffers = %d\n",pComponentPrivate->pOutputBufferList->numBuffers);

            if (pComponentPrivate->curState == OMX_StateWaitForResources){
                OMX_PRDSP2(pComponentPrivate->dbg, "WBAMR_DEC_HandleCommand: Cmd Loaded\n");
#ifdef __PERF_INSTRUMENTATION__
                PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryStart | PERF_BoundaryCleanup);
#endif
                pComponentPrivate->curState = OMX_StateLoaded;
#ifdef __PERF_INSTRUMENTATION__
                PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryComplete | PERF_BoundaryCleanup);
#endif
                pComponentPrivate->cbInfo.EventHandler (
                                                        pHandle, pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete, OMX_CommandStateSet,pComponentPrivate->curState,
                                                        NULL);
                break;

            }
            OMX_PRSTATE2(pComponentPrivate->dbg, "In side OMX_StateLoaded State: \n");
            if (pComponentPrivate->curState != OMX_StateIdle &&
                pComponentPrivate->curState != OMX_StateWaitForResources) {
                OMX_PRINT2(pComponentPrivate->dbg, "Comp: OMX_AmrDecUtils.c\n");
                pComponentPrivate->cbInfo.EventHandler (pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventError,
                                                        OMX_ErrorIncorrectStateTransition,\
                                                        OMX_TI_ErrorMinor,
                                                        "Incorrect State Transition");
                OMX_ERROR4(pComponentPrivate->dbg, "Error: Invalid State Given by \
                       Application\n");
                goto EXIT;
            }

            OMX_PRBUFFER2(pComponentPrivate->dbg, "pComponentPrivate->pInputBufferList->numBuffers = %d\n",pComponentPrivate->pInputBufferList->numBuffers);
            OMX_PRBUFFER2(pComponentPrivate->dbg, "pComponentPrivate->pOutputBufferList->numBuffers = %d\n",pComponentPrivate->pOutputBufferList->numBuffers);
#ifdef __PERF_INSTRUMENTATION__
            PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryStart | PERF_BoundaryCleanup);
#endif
            if (pComponentPrivate->pInputBufferList->numBuffers ||
                pComponentPrivate->pOutputBufferList->numBuffers) {
                OMX_ERROR2(pComponentPrivate->dbg, "Wait for InIdle_mutex\n");
                pComponentPrivate->InIdle_goingtoloaded = 1;
#ifndef UNDER_CE
                pthread_mutex_lock(&pComponentPrivate->InIdle_mutex);
                pthread_cond_wait(&pComponentPrivate->InIdle_threshold, &pComponentPrivate->InIdle_mutex);
                pthread_mutex_unlock(&pComponentPrivate->InIdle_mutex);
#else
                OMX_WaitForEvent(&(pComponentPrivate->InIdle_event));
#endif
            }

            /* Now Deinitialize the component No error should be returned from
             * this function. It should clean the system as much as possible */
            OMX_PRSTATE2(pComponentPrivate->dbg, "In side OMX_StateLoaded State: \n");
            WBAMR_DEC_CleanupInitParams(pComponentPrivate->pHandle);

            eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                       EMMCodecControlDestroy, (void *)p);

            /*Closing LCML Lib*/
            if (pComponentPrivate->ptrLibLCML != NULL)
            {
                OMX_PRDSP2(pComponentPrivate->dbg, "OMX_WbAmrDecoder.c Closing LCML library\n");
                dlclose( pComponentPrivate->ptrLibLCML  );
                pComponentPrivate->ptrLibLCML = NULL;
            }

#ifdef __PERF_INSTRUMENTATION__
            PERF_SendingCommand(pComponentPrivate->pPERF, -1, 0, PERF_ModuleComponent);
#endif
            OMX_PRINT2(pComponentPrivate->dbg, "In side OMX_StateLoaded State: \n");
            if (eError != OMX_ErrorNone) {
                OMX_ERROR4(pComponentPrivate->dbg, "Error: in Destroying the codec: no.  %x\n", eError);
                goto EXIT;
            }
            OMX_PRDSP1(pComponentPrivate->dbg, "WBAMR_DEC_HandleCommand: Cmd Loaded\n");
            eError = WBAMR_DEC_EXIT_COMPONENT_THRD;
            pComponentPrivate->bInitParamsInitialized = 0;
            pComponentPrivate->bLoadedCommandPending = OMX_FALSE;
            /* Send StateChangeNotification to application */
            break;

        case OMX_StatePause:
            OMX_PRSTATE2(pComponentPrivate->dbg, "WBAMR_DEC_HandleCommand: Cmd Pause\n");
            if (pComponentPrivate->curState == commandedState){
                pComponentPrivate->cbInfo.EventHandler (pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventError,
                                                        OMX_ErrorSameState,
                                                        OMX_TI_ErrorMinor,
                                                        "Same State");
                OMX_ERROR4(pComponentPrivate->dbg, "Error: Same State Given by \
                           Application\n");
                break;
            }
            if (pComponentPrivate->curState != OMX_StateExecuting &&
                pComponentPrivate->curState != OMX_StateIdle) {
                OMX_PRINT2(pComponentPrivate->dbg, "Comp: OMX_AmrDecUtils.c\n");
                pComponentPrivate->cbInfo.EventHandler (pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventError,
                                                        OMX_ErrorIncorrectStateTransition,
                                                        OMX_TI_ErrorMinor,
                                                        "Incorrect State Transition");
                OMX_ERROR4(pComponentPrivate->dbg, "Error: Invalid State Given by \
                       Application\n");
                goto EXIT;
            }
#ifdef __PERF_INSTRUMENTATION__
            PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryComplete | PERF_BoundarySteadyState);
#endif
            eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                       EMMCodecControlPause, (void *)p);

            if (eError != OMX_ErrorNone) {
                OMX_ERROR4(pComponentPrivate->dbg, "Error: in Pausing the codec\n");
                pComponentPrivate->curState = OMX_StateInvalid;
                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventError,
                                                       eError,
                                                       OMX_TI_ErrorSevere,
                                                       NULL);
                goto EXIT;
            }
#ifdef RESOURCE_MANAGER_ENABLED
            rm_error = RMProxy_NewSendCommand(pHandle,
                                              RMProxy_StateSet,
                                              OMX_WBAMR_Decoder_COMPONENT,
                                              OMX_StatePause,
                                              3456,
                                              NULL);
#endif

            OMX_PRINT2(pComponentPrivate->dbg, "Comp: OMX_AmrDecUtils.c\n");
            break;

        case OMX_StateWaitForResources:

            if (pComponentPrivate->curState == commandedState){
                OMX_PRINT2(pComponentPrivate->dbg, "Comp: OMX_AmrDecUtils.c\n");
                pComponentPrivate->cbInfo.EventHandler (pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventError,
                                                        OMX_ErrorSameState,
                                                        OMX_TI_ErrorMinor,
                                                        "Same State");
                OMX_ERROR4(pComponentPrivate->dbg, "Error: Same State Given by \
                           Application\n");
            }
            else if (pComponentPrivate->curState == OMX_StateLoaded) {
                OMX_PRINT2(pComponentPrivate->dbg, "Comp: OMX_AmrDecUtils.c\n");

#ifdef RESOURCE_MANAGER_ENABLED
                rm_error = RMProxy_NewSendCommand(pHandle,
                                                  RMProxy_StateSet,
                                                  OMX_WBAMR_Decoder_COMPONENT,
                                                  OMX_StateWaitForResources,
                                                  3456,
                                                  NULL);
#endif

                pComponentPrivate->curState = OMX_StateWaitForResources;
                pComponentPrivate->cbInfo.EventHandler(
                                                       pHandle, pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete, OMX_CommandStateSet,pComponentPrivate->curState,NULL);
            }
            else{
                OMX_PRINT2(pComponentPrivate->dbg, "Comp: OMX_AmrDecUtils.c\n");
                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventError,
                                                       OMX_ErrorIncorrectStateTransition,
                                                       OMX_TI_ErrorMinor,
                                                       "Incorrect State Transition");
            }
            break;

        case OMX_StateInvalid:
            OMX_PRSTATE2(pComponentPrivate->dbg, "WBAMR_DEC_HandleCommand: Cmd OMX_StateInvalid:\n");
            if (pComponentPrivate->curState == commandedState){
                pComponentPrivate->cbInfo.EventHandler (pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventError,
                                                        OMX_ErrorSameState,
                                                        OMX_TI_ErrorSevere,
                                                        "Same State");
                OMX_ERROR4(pComponentPrivate->dbg, "Error: Same State Given by \
                           Application\n");
            }
            else{
                OMX_PRINT2(pComponentPrivate->dbg, "Comp: OMX_AmrDecUtils.c\n");
                if (pComponentPrivate->curState != OMX_StateWaitForResources &&
                    pComponentPrivate->curState != OMX_StateLoaded) {

                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               EMMCodecControlDestroy, (void *)p);

                    WBAMR_DEC_CleanupInitParams(pHandle);
                }
                pComponentPrivate->curState = OMX_StateInvalid;
                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventError,
                                                       OMX_ErrorInvalidState,
                                                       OMX_TI_ErrorSevere,
                                                       "Incorrect State Transition");
            }

            break;

        case OMX_StateMax:
            OMX_PRSTATE2(pComponentPrivate->dbg, "WBAMR_DEC_HandleCommand: Cmd OMX_StateMax::\n");
            break;
        default:
            break;
        } /* End of Switch */
    }
    else if (command == OMX_CommandMarkBuffer) {
        OMX_PRCOMM1(pComponentPrivate->dbg, "command OMX_CommandMarkBuffer received\n");
        if(!pComponentPrivate->pMarkBuf){
            OMX_PRCOMM1(pComponentPrivate->dbg, "command OMX_CommandMarkBuffer received\n");
            /* TODO Need to handle multiple marks */
            pComponentPrivate->pMarkBuf = (OMX_MARKTYPE *)(commandData);
        }
    }
    else if (command == OMX_CommandPortDisable) {
        if (!pComponentPrivate->bDisableCommandPending) {
            if(commandData == 0x0 || commandData == -1){
                /* disable port */
                pComponentPrivate->pPortDef[WBAMR_DEC_INPUT_PORT]->bEnabled = OMX_FALSE;
                for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
                    if (WBAMR_DEC_IsPending(pComponentPrivate,pComponentPrivate->pInputBufferList->pBufHdr[i],OMX_DirInput)) {
                        /* Real solution is flush buffers from DSP.  Until we have the ability to do that
                           we just call EmptyBufferDone() on any pending buffers */
#ifdef __PERF_INSTRUMENTATION__
                        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                          PREF(pComponentPrivate->pInputBufferList->pBufHdr[i], pBuffer),
                                          0,
                                          PERF_ModuleHLMM);
#endif
                        WBAMR_DEC_ClearPending(pComponentPrivate,pComponentPrivate->pInputBufferList->pBufHdr[i],OMX_DirInput,__LINE__);
                        pComponentPrivate->nEmptyBufferDoneCount++;
                        pComponentPrivate->cbInfo.EmptyBufferDone (
                                                                   pComponentPrivate->pHandle,
                                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                                   pComponentPrivate->pInputBufferList->pBufHdr[i]
                                                                   );
                        SignalIfAllBuffersAreReturned(pComponentPrivate);
                    }
                }
            }
            if(commandData == 0x1 || commandData == -1){
                char *pArgs = "damedesuStr";
                pComponentPrivate->pPortDef[WBAMR_DEC_OUTPUT_PORT]->bEnabled = OMX_FALSE;
                if (pComponentPrivate->curState == OMX_StateExecuting) {
                    pComponentPrivate->bNoIdleOnStop = OMX_TRUE;
                    OMX_PRINT1(pComponentPrivate->dbg, "OMX_WBAmrDec_Utils.c :: Calling LCML_ControlCodec()\n");

                if (pComponentPrivate->codecStop_waitingsignal == 0){ 
                    pthread_mutex_lock(&pComponentPrivate->codecStop_mutex);    
                }
                    eError = LCML_ControlCodec(
                                               ((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               MMCodecControlStop,(void *)pArgs);
                if (pComponentPrivate->codecStop_waitingsignal == 0){
                    pthread_cond_wait(&pComponentPrivate->codecStop_threshold, &pComponentPrivate->codecStop_mutex);
                    pComponentPrivate->codecStop_waitingsignal = 0;
                    pthread_mutex_unlock(&pComponentPrivate->codecStop_mutex);
                }
                }
            }

        }
        OMX_PRCOMM2(pComponentPrivate->dbg, "commandData = %ld\n",commandData);
        OMX_PRCOMM2(pComponentPrivate->dbg, "pComponentPrivate->pPortDef[WBAMR_DEC_INPUT_PORT]->bPopulated = %d\n",pComponentPrivate->pPortDef[WBAMR_DEC_INPUT_PORT]->bPopulated);
        OMX_PRCOMM2(pComponentPrivate->dbg, "pComponentPrivate->pPortDef[WBAMR_DEC_OUTPUT_PORT]->bPopulated = %d\n",pComponentPrivate->pPortDef[WBAMR_DEC_OUTPUT_PORT]->bPopulated);
        if(commandData == 0x0) {
            if(!pComponentPrivate->pPortDef[WBAMR_DEC_INPUT_PORT]->bPopulated){
                /* return cmdcomplete event if input unpopulated */
                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandPortDisable,
                                                       WBAMR_DEC_INPUT_PORT,
                                                       NULL);
                OMX_PRCOMM1(pComponentPrivate->dbg, "OMX_WBAmrDec_Utils.c :: Clearing bDisableCommandPending\n");
                pComponentPrivate->bDisableCommandPending = 0;
            }
            else{
                pComponentPrivate->bDisableCommandPending = 1;
                pComponentPrivate->bDisableCommandParam = commandData;
            }
        }

        if(commandData == 0x1) {
            if (!pComponentPrivate->pPortDef[WBAMR_DEC_OUTPUT_PORT]->bPopulated){
                /* return cmdcomplete event if output unpopulated */
                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandPortDisable,
                                                       WBAMR_DEC_OUTPUT_PORT,
                                                       NULL);
                OMX_PRCOMM1(pComponentPrivate->dbg, "OMX_WBAmrDec_Utils.c :: Clearing bDisableCommandPending\n");
                pComponentPrivate->bDisableCommandPending = 0;
            }
            else {
                pComponentPrivate->bDisableCommandPending = 1;
                pComponentPrivate->bDisableCommandParam = commandData;
            }
        }

        if(commandData == -1) {
            if (!pComponentPrivate->pPortDef[WBAMR_DEC_INPUT_PORT]->bPopulated &&
                !pComponentPrivate->pPortDef[WBAMR_DEC_OUTPUT_PORT]->bPopulated){

                /* return cmdcomplete event if inout & output unpopulated */
                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandPortDisable,
                                                       WBAMR_DEC_INPUT_PORT,
                                                       NULL);

                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandPortDisable,
                                                       WBAMR_DEC_OUTPUT_PORT,
                                                       NULL);
                OMX_PRCOMM1(pComponentPrivate->dbg, "OMX_WBAmrDec_Utils.c :: Clearing bDisableCommandPending\n");
                pComponentPrivate->bDisableCommandPending = 0;
            }
            else {
                pComponentPrivate->bDisableCommandPending = 1;
                pComponentPrivate->bDisableCommandParam = commandData;
            }

        }
    }
    else if (command == OMX_CommandPortEnable) {
        if(!pComponentPrivate->bEnableCommandPending) {
            if(commandData == 0x0 || commandData == -1){
                /* enable in port */
                OMX_PRCOMM2(pComponentPrivate->dbg, "setting input port to enabled\n");
                pComponentPrivate->pPortDef[WBAMR_DEC_INPUT_PORT]->bEnabled = OMX_TRUE;
                OMX_PRCOMM2(pComponentPrivate->dbg, "pComponentPrivate->pPortDef[WBAMR_DEC_INPUT_PORT]->bEnabled = %d\n",
                            pComponentPrivate->pPortDef[WBAMR_DEC_INPUT_PORT]->bEnabled);

                if(pComponentPrivate->AlloBuf_waitingsignal)
                {
                    pComponentPrivate->AlloBuf_waitingsignal = 0;
                }
            }
            if(commandData == 0x1 || commandData == -1){
                char *pArgs = "damedesuStr";
                /* enable out port */
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

                    pComponentPrivate->bDspStoppedWhileExecuting = OMX_FALSE;
                    eError = LCML_ControlCodec(
                                               ((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               EMMCodecControlStart,(void *)pArgs);
                }
                OMX_PRCOMM2(pComponentPrivate->dbg, "setting output port to enabled\n");
                pComponentPrivate->pPortDef[WBAMR_DEC_OUTPUT_PORT]->bEnabled = OMX_TRUE;
                OMX_PRCOMM2(pComponentPrivate->dbg, "pComponentPrivate->pPortDef[WBAMR_DEC_OUTPUT_PORT]->bEnabled = %d\n",pComponentPrivate->pPortDef[WBAMR_DEC_OUTPUT_PORT]->bEnabled);
            }
        }

        if(commandData == 0x0){
            if (pComponentPrivate->curState == OMX_StateLoaded ||
                pComponentPrivate->pPortDef[WBAMR_DEC_INPUT_PORT]->bPopulated){

                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandPortEnable,
                                                       WBAMR_DEC_INPUT_PORT,
                                                       NULL);
                pComponentPrivate->bEnableCommandPending = 0;
            }
            else{
                pComponentPrivate->bEnableCommandPending = 1;
                pComponentPrivate->bEnableCommandParam = commandData;
            }
        }
        else if(commandData == 0x1){
            if (pComponentPrivate->curState == OMX_StateLoaded ||
                pComponentPrivate->pPortDef[WBAMR_DEC_OUTPUT_PORT]->bPopulated){

                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandPortEnable,
                                                       WBAMR_DEC_OUTPUT_PORT,
                                                       NULL);
                pComponentPrivate->bEnableCommandPending = 0;
            }
            else{
                pComponentPrivate->bEnableCommandPending = 1;
                pComponentPrivate->bEnableCommandParam = commandData;

            }
        }
        else if(commandData == -1){
            if (pComponentPrivate->curState == OMX_StateLoaded ||
                (pComponentPrivate->pPortDef[WBAMR_DEC_INPUT_PORT]->bPopulated
                 && pComponentPrivate->pPortDef[WBAMR_DEC_OUTPUT_PORT]->bPopulated)){

                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandPortEnable,
                                                       WBAMR_DEC_INPUT_PORT,
                                                       NULL);

                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandPortEnable,
                                                       WBAMR_DEC_OUTPUT_PORT,
                                                       NULL);

                pComponentPrivate->bEnableCommandPending = 0;
                WBAMR_DEC_Fill_LCMLInitParamsEx(pComponentPrivate->pHandle);
            }
            else{
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
        OMX_U32 aParam[3] = {0};
        OMX_PRCOMM2(pComponentPrivate->dbg, "Flushing input port %d\n",pComponentPrivate->nUnhandledEmptyThisBuffers);
        if(commandData == 0x0 || commandData == -1) {
            if(pComponentPrivate->nUnhandledEmptyThisBuffers == 0) {
                pComponentPrivate->bFlushInputPortCommandPending = OMX_FALSE;

                aParam[0] = USN_STRMCMD_FLUSH;
                aParam[1] = 0x0;
                aParam[2] = 0x0;

                OMX_PRCOMM2(pComponentPrivate->dbg, "Flushing input port\n");
                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                           EMMCodecControlStrmCtrl,
                                           (void*)aParam);
                if (eError != OMX_ErrorNone) {
                    goto EXIT;
                }
            }else{
                pComponentPrivate->bFlushInputPortCommandPending = OMX_TRUE;
            }
        }
        if(commandData == 0x1 || commandData == -1){
            OMX_PRCOMM2(pComponentPrivate->dbg, "Flushing out port %d\n",pComponentPrivate->nUnhandledFillThisBuffers);
            if (pComponentPrivate->nUnhandledFillThisBuffers == 0)  {
                pComponentPrivate->bFlushOutputPortCommandPending = OMX_FALSE;

                aParam[0] = USN_STRMCMD_FLUSH;
                aParam[1] = 0x1;
                aParam[2] = 0x0;

                OMX_PRCOMM2(pComponentPrivate->dbg, "Flushing output port\n");
                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                           EMMCodecControlStrmCtrl,
                                           (void*)aParam);
                if (eError != OMX_ErrorNone) {
                    goto EXIT;
                }
            }else{
                pComponentPrivate->bFlushOutputPortCommandPending = OMX_TRUE;
            }
        }
    }
 EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, "Exiting WBAMR_DEC_HandleCommand Function\n");
    OMX_PRINT1(pComponentPrivate->dbg, "Returning %d\n",eError);
    if (eError != OMX_ErrorNone && eError != WBAMR_DEC_EXIT_COMPONENT_THRD ) {
        pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                               pComponentPrivate->pHandle->pApplicationPrivate,
                                               OMX_EventError,
                                               eError,
                                               OMX_TI_ErrorSevere,
                                               NULL);
    }
    return eError;
}


/* ========================================================================== */
/**
* @WBAMR_DEC_HandleDataBuf_FromApp() This function is called by the component when ever it
* receives the buffer from the application
*
* @param pComponentPrivate  Component private data
* @param pBufHeader WBAMR_DEC_Buffer from the application
*
* @pre
*
* @post
*
* @return none
*/
/* ========================================================================== */
OMX_ERRORTYPE WBAMR_DEC_HandleDataBuf_FromApp(OMX_BUFFERHEADERTYPE* pBufHeader,
                                              WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_DIRTYPE eDir;
    LCML_WBAMR_DEC_BUFHEADERTYPE *pLcmlHdr;
    LCML_DSP_INTERFACE *pLcmlHandle = (LCML_DSP_INTERFACE *)
        pComponentPrivate->pLcmlHandle;
    OMX_U32 index;
    OMX_U32 frameType;
    OMX_U32 frameLength;
    OMX_U8* pExtraData;
    OMX_U16 i;
    OMX_U32 holdBufferSize;
    OMX_U8 nFrames =0;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefIn = NULL;

    OMX_U32 nFilledLenLocal;
    OMX_U8 TOCentry, hh=0, *TOCframetype=0;
    OMX_U16 offset = 0;

    WBAMR_DEC_AudioCodecParams *pParams;
    OMX_STRING p = "damedesuStr";
    DSP_STATUS status;
    OMX_BOOL isFrameParamChanged=OMX_FALSE;

    LCML_DSP_INTERFACE * phandle;

    OMX_PRINT1(pComponentPrivate->dbg, "Entering WBAMR_DEC_HandleDataBuf_FromApp Function\n");

    holdBufferSize = INPUT_WBAMRDEC_BUFFER_SIZE * (pComponentPrivate->pInputBufferList->numBuffers + 1);
    /*Find the direction of the received buffer from buffer list */
    eError = WBAMR_DEC_GetBufferDirection(pBufHeader, &eDir);
    if (eError != OMX_ErrorNone) {
        OMX_ERROR4(pComponentPrivate->dbg, "The PBufHeader is not found in the list\n");
        goto EXIT;
    }

    if( pBufHeader->pBuffer == NULL) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (eDir == OMX_DirInput) {
        pComponentPrivate->nUnhandledEmptyThisBuffers--;
        if (pComponentPrivate->curState == OMX_StateIdle){
            pComponentPrivate->cbInfo.EmptyBufferDone (pComponentPrivate->pHandle,
                                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                                       pBufHeader);
            pComponentPrivate->nEmptyBufferDoneCount++;
            SignalIfAllBuffersAreReturned(pComponentPrivate);
            OMX_PRBUFFER2(pComponentPrivate->dbg, ":: %d %s In idle state return input buffers\n", __LINE__, __FUNCTION__);
            goto EXIT;
        }
        pPortDefIn = pComponentPrivate->pPortDef[OMX_DirInput];
        if ( pBufHeader->nFilledLen > 0)
        {
            if ( pComponentPrivate->nHoldLength == 0 )
            { /*There is no holding data*/
                if (pComponentPrivate->mimemode == WBAMRDEC_MIMEMODE)
                {
                    OMX_PRBUFFER2(pComponentPrivate->dbg, "WBAMR_DEC_HandleDataBuf_FromApp - reading WBAMR_DEC_MIMEMODE\n");
                    if(pComponentPrivate->using_rtsp==1){ /* formating data */
                        nFilledLenLocal=pBufHeader->nFilledLen; 
                        while(TRUE)
                        {
                            TOCframetype = (OMX_U8*)realloc(TOCframetype, ((hh + 1) * sizeof(OMX_U8)));
                            if (TOCframetype == NULL)
                            {
                              OMX_ERROR4(pComponentPrivate->dbg, "%d :WBAMR_DEC_HandleDataBuf_FromApp ERROR: Couldn't realloc memory!",__LINE__);
                              goto EXIT;
                            }
                            TOCentry = pBufHeader->pBuffer[0];
                            TOCframetype[hh]= TOCentry & 0x7C;
                            hh++;
                            if (!(TOCentry & 0x80))
                                break;
                            memmove(pBufHeader->pBuffer,
                                    pBufHeader->pBuffer + 1,
                                    nFilledLenLocal);
                        }
                        while(nFilledLenLocal> 0 ){
                            index = (TOCframetype[nFrames] >> 3) & 0x0F;
                            /* adding TOC to each frame */
                            if (offset > pBufHeader->nAllocLen){
                                OMX_ERROR4(pComponentPrivate->dbg, "%d :: WBAMR_DEC_HandleDataBuf_FromApp :: ERROR: Trying to write beyond buffer boundaries!",__LINE__);
                              goto EXIT;
                            }   
                            else
                                memcpy(pBufHeader->pBuffer + offset, 
                                                &TOCframetype[nFrames],
                                                sizeof(OMX_U8));
                            offset+=pComponentPrivate->wbamrMimeBytes[index];
                            if ( offset + 1 + nFilledLenLocal > pBufHeader->nAllocLen){
                                OMX_ERROR4(pComponentPrivate->dbg, "%d :: WBAMR_DEC_HandleDataBuf_FromApp :: ERROR: Trying to write beyond buffer boundaries!",__LINE__);
                              goto EXIT;
                            }   
                            else
                            memmove(pBufHeader->pBuffer + offset + 1,
                                                pBufHeader->pBuffer + offset,
                                                nFilledLenLocal);

                            if (pComponentPrivate->wbamrMimeBytes[index] > nFilledLenLocal){
                                        nFilledLenLocal = 0;
                            }else{
                                        nFilledLenLocal -= pComponentPrivate->wbamrMimeBytes[index];
                            }
                            nFrames++;
                        }
                        free(TOCframetype);
			 TOCframetype = NULL;
                    }
                    frameType = 0;
                    nFrames = 0;
                    i=0;
                    while( pBufHeader->nFilledLen > 0 )
                    {        /*Reorder the Mime buffer in case that comes*/
                        frameType = pBufHeader->pBuffer[i]; /*with more than 1 frame                    */
                        index = (frameType >> 3) & 0x0F;
                        if(nFrames)
                        { /*The first frame has no need to be moved*/
                            if (((nFrames*INPUT_WBAMRDEC_BUFFER_SIZE_MIME) + pBufHeader->nFilledLen) 
			       > pBufHeader->nAllocLen) {
                               OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_WbAmrDec_Utils.c :: ERROR: Trying to write beyond buffer boundaries!",__LINE__);
                               goto EXIT;
                           }
                            memmove(pBufHeader->pBuffer + (nFrames*INPUT_WBAMRDEC_BUFFER_SIZE_MIME),
                                    pBufHeader->pBuffer + i,
                                    pBufHeader->nFilledLen);
                        }
			if ((index >= NUM_MIME_BYTES_ARRAY) || 
			   ((index < NUM_MIME_BYTES_ARRAY) && 
			   (pComponentPrivate->wbamrMimeBytes[index] == 0))) {
                           OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: OMX_WbAmrDec_Utils.c :: no more frames index=%d", __LINE__, (int)index);
                           if (index < NUM_MIME_BYTES_ARRAY)
                               OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: OMX_WbAmrDec_Utils.c :: no more frames mimebytes=%d", __LINE__, (int) pComponentPrivate->wbamrMimeBytes[index]);
                               break;
                        }
                        pBufHeader->nFilledLen -= pComponentPrivate->wbamrMimeBytes[index];
                        i = (nFrames*INPUT_WBAMRDEC_BUFFER_SIZE_MIME) + (OMX_U16)pComponentPrivate->wbamrMimeBytes[index];
                        nFrames++;
                    }
                    pBufHeader->nFilledLen=nFrames*INPUT_WBAMRDEC_BUFFER_SIZE_MIME;
                }
                else if (pComponentPrivate->mimemode == WBAMRDEC_IF2)
                {
                    OMX_PRINT2(pComponentPrivate->dbg, "WBAMR_DEC_HandleDataBuf_FromApp - reading WBAMRDEC_IF2\n");
                    nFrames = 0;
                    i = 0;
                    while (pBufHeader->nFilledLen > 0)
                    {
                        /*Reorder the IF2 buffer in case that has more than 1 frame                 */
                        frameType = pBufHeader->pBuffer[i];
                        index = (frameType >> 4) & 0x0F;
                        if (nFrames)
                        {
                            if (((nFrames*INPUT_WBAMRDEC_BUFFER_SIZE_IF2) + pBufHeader->nFilledLen) 
			       > pBufHeader->nAllocLen) {
                               OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_WbAmrDec_Utils.c :: ERROR: Trying to write beyond buffer boundaries!",__LINE__);
                               goto EXIT;
                            }
                            memmove(pBufHeader->pBuffer + (nFrames *INPUT_WBAMRDEC_BUFFER_SIZE_IF2),
                                    pBufHeader->pBuffer + i,
                                    pBufHeader->nFilledLen);
                        }
                        if ((index >= NUM_IF2_BYTES_ARRAY) || 
			   ((index < NUM_IF2_BYTES_ARRAY) && 
			   (pComponentPrivate->wbamrIf2Bytes[index] == 0))) {
                           OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: OMX_WbAmrDec_Utils.c :: no more frames index=%d", __LINE__, (int)index);
                           if (index < NUM_IF2_BYTES_ARRAY)
                               OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: OMX_WbAmrDec_Utils.c :: no more frames mimebytes=%d", __LINE__, (int)pComponentPrivate->wbamrIf2Bytes[index]);
                               break;
                        }
                        pBufHeader->nFilledLen -= pComponentPrivate->wbamrIf2Bytes[index];
                        i = (nFrames *INPUT_WBAMRDEC_BUFFER_SIZE_IF2) + (OMX_U16)pComponentPrivate->wbamrIf2Bytes[index];
                        nFrames++;
                    }
                    pBufHeader->nFilledLen = nFrames * INPUT_WBAMRDEC_BUFFER_SIZE_IF2;
                }

                else
                {
                    OMX_PRBUFFER2(pComponentPrivate->dbg, "WBAMR_DEC_HandleDataBuf_FromApp - reading WBAMR_DEC_NONMIMEMODE\n");
                    frameLength = INPUT_WBAMRDEC_BUFFER_SIZE;  /*/ non Mime mode*/
                    nFrames = (OMX_U8)(pBufHeader->nFilledLen / frameLength);
                }

                if ( nFrames>=1 && (WBAMRDEC_FORMATCONFORMANCE == pComponentPrivate->mimemode))
                    /* At least there is 1 frame in the buffer */
                {
                    pComponentPrivate->nHoldLength = pBufHeader->nFilledLen - frameLength*nFrames;
                    if ( pComponentPrivate->nHoldLength > 0 ) {/* something need to be hold in iHoldBuffer */
                        if (pComponentPrivate->pHoldBuffer == NULL) {
                            OMX_MALLOC_SIZE(pComponentPrivate->pHoldBuffer, (INPUT_WBAMRDEC_BUFFER_SIZE * (pComponentPrivate->pInputBufferList->numBuffers + 3)),void);
                        }
                        /* Copy the extra data into pHoldBuffer. Size will be nHoldLength. */
                        pExtraData = pBufHeader->pBuffer + INPUT_WBAMRDEC_BUFFER_SIZE*nFrames;
			/* check the pHoldBuffer boundary before copying */
			if (pComponentPrivate->nHoldLength >
			   (INPUT_WBAMRDEC_BUFFER_SIZE * (pComponentPrivate->pInputBufferList->numBuffers + 3)))
			   {
                               OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_WbAmrDec_Utils.c :: ERROR: Trying to write beyond buffer boundaries!",__LINE__);
                               goto EXIT;
			   }
                        memcpy (pComponentPrivate->pHoldBuffer, pExtraData, pComponentPrivate->nHoldLength);
                    }
                }
                else
                {
                    if (pComponentPrivate->mimemode == WBAMRDEC_FORMATCONFORMANCE)
                    {
                        /* received buffer with less than 1 AMR frame. Save the data in iHoldBuffer.*/
                        pComponentPrivate->nHoldLength = pBufHeader->nFilledLen;
                        /* save the data into iHoldBuffer.*/
                        if (pComponentPrivate->pHoldBuffer == NULL) {
                            OMX_MALLOC_SIZE(pComponentPrivate->pHoldBuffer, (INPUT_WBAMRDEC_BUFFER_SIZE * (pComponentPrivate->pInputBufferList->numBuffers + 3)),void);
                        }
                        /* Not enough data to be sent. Copy all received data into iHoldBuffer.*/
                        /* Size to be copied will be iHoldLen == mmData->BufferSize() */
                        memset (pComponentPrivate->pHoldBuffer,
                                0,
                                INPUT_WBAMRDEC_BUFFER_SIZE * (pComponentPrivate->pInputBufferList->numBuffers + 1));
                        memcpy (pComponentPrivate->pHoldBuffer, pBufHeader->pBuffer, pComponentPrivate->nHoldLength);
                        /* since not enough data, we shouldn't send anything to SN, but instead request to EmptyBufferDone again.*/
                        OMX_PRBUFFER2(pComponentPrivate->dbg, "Calling EmptyBufferDone\n");
                        if (pComponentPrivate->curState != OMX_StatePause) {
                            pComponentPrivate->nEmptyBufferDoneCount++;
#ifdef __PERF_INSTRUMENTATION__
                            PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                              PREF(pBufHeader, pBuffer),
                                              0,
                                              PERF_ModuleHLMM);
#endif
                            pComponentPrivate->cbInfo.EmptyBufferDone (pComponentPrivate->pHandle,
                                                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                                                       pBufHeader);
                            SignalIfAllBuffersAreReturned(pComponentPrivate);
                        }
                        else {
                            pComponentPrivate->pInputBufHdrPending[pComponentPrivate->nNumInputBufPending++] = pBufHeader;
                        }
                        goto EXIT;
                    }
                }
            }
            else {
                /* iHoldBuffer has data. There is no possibility that data in iHoldBuffer is less for 1 frame without*/
                /* lastBufferFlag being set. Unless it's a corrupt file.*/
                /* Copy the data in dataPtr to iHoldBuffer. Update the iHoldBuffer size (iHoldLen).*/
                pExtraData = pComponentPrivate->pHoldBuffer + pComponentPrivate->nHoldLength;
                memcpy(pExtraData,pBufHeader->pBuffer,pBufHeader->nFilledLen);
                pComponentPrivate->nHoldLength += pBufHeader->nFilledLen;
                /* Check if it is mime mode or non-mime mode to decide the frame length to be sent down*/
                /* to DSP/ALG.*/
                if (pComponentPrivate->mimemode == WBAMRDEC_MIMEMODE)
                {
                    frameType = pComponentPrivate->pHoldBuffer[0];
                    index = ( frameType >> 3 ) & 0x0F;
                    frameLength = pComponentPrivate->wbamrMimeBytes[index];
                }
                else if(pComponentPrivate->mimemode == WBAMRDEC_IF2)
                {
                    frameType = pComponentPrivate->pHoldBuffer[0];
                    index = ( frameType >> 4 ) & 0x0F;
                    frameLength = pComponentPrivate->wbamrIf2Bytes[index];
                }

                else {
                    frameLength = INPUT_WBAMRDEC_BUFFER_SIZE;
                }
                nFrames = (OMX_U8)(pComponentPrivate->nHoldLength / frameLength);
                if ( nFrames >= 1 )  {
                    /* Copy the data from pComponentPrivate->pHoldBuffer to pBufHeader->pBuffer*/
		    /* check the pBufHeader boundery before copying */
		    if ((nFrames*frameLength) > pBufHeader->nAllocLen)
		    {
                        OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_WbAmrDec_Utils.c :: ERROR: Trying to write beyond buffer boundaries!",__LINE__);
			goto EXIT;
		    }
                    memcpy(pBufHeader->pBuffer,pComponentPrivate->pHoldBuffer,nFrames*frameLength);
                    pBufHeader->nFilledLen = nFrames*frameLength;
                    /* Now the pHoldBuffer has pBufHeader->nFilledLen fewer bytes, update nHoldLength*/
                    pComponentPrivate->nHoldLength = pComponentPrivate->nHoldLength - pBufHeader->nFilledLen;
                    /* Shift the remaining bytes to the beginning of the pHoldBuffer */
                    pExtraData = pComponentPrivate->pHoldBuffer + pBufHeader->nFilledLen;
		    if (pComponentPrivate->nHoldLength < pBufHeader->nFilledLen)
                        memcpy(pComponentPrivate->pHoldBuffer,pExtraData,pComponentPrivate->nHoldLength);
		    else
                        memmove(pComponentPrivate->pHoldBuffer,pExtraData,pComponentPrivate->nHoldLength);
                    /* Clear the rest of the data from the pHoldBuffer */
                    /*pExtraData = pComponentPrivate->pHoldBuffer + pComponentPrivate->nHoldLength;*/
                    /*mset(pExtraData,0,holdBufferSize - pComponentPrivate->nHoldLength);*/
                }
                else {
                    if (pComponentPrivate->curState != OMX_StatePause) {
                        OMX_PRBUFFER2(pComponentPrivate->dbg, "Calling EmptyBufferDone\n");
                        pComponentPrivate->nEmptyBufferDoneCount++;
#ifdef __PERF_INSTRUMENTATION__
                        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                          PREF(pBufHeader, pBuffer),
                                          0,
                                          PERF_ModuleHLMM);
#endif
                        pComponentPrivate->cbInfo.EmptyBufferDone (
                                                                   pComponentPrivate->pHandle,
                                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                                   pBufHeader);
                        SignalIfAllBuffersAreReturned(pComponentPrivate);
                        goto EXIT;
                    }
                    else {
                        pComponentPrivate->pInputBufHdrPending[pComponentPrivate->nNumInputBufPending++] = pBufHeader;
                    }
                }
            }
        }else{
            if((pBufHeader->nFlags & OMX_BUFFERFLAG_EOS) != OMX_BUFFERFLAG_EOS){
                pComponentPrivate->nEmptyBufferDoneCount++;
#ifdef __PERF_INSTRUMENTATION__
                PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                  pComponentPrivate->pInputBufferList->pBufHdr[0]->pBuffer,0,PERF_ModuleHLMM);
#endif
                pComponentPrivate->cbInfo.EmptyBufferDone( pComponentPrivate->pHandle,
                                                           pComponentPrivate->pHandle->pApplicationPrivate,
                                                           pComponentPrivate->pInputBufferList->pBufHdr[0]);
                SignalIfAllBuffersAreReturned(pComponentPrivate);
               goto EXIT;
            }
            else{
                nFrames=1;
            }
        }
        if(nFrames >= 1){
            eError = WBAMR_DEC_GetCorresponding_LCMLHeader(pComponentPrivate,pBufHeader->pBuffer, OMX_DirInput, &pLcmlHdr);
            if (eError != OMX_ErrorNone) {
                OMX_ERROR4(pComponentPrivate->dbg, "Error: Invalid WBAMR_DEC_Buffer Came ...\n");
                goto EXIT;
            }

#ifdef __PERF_INSTRUMENTATION__
            /*For Steady State Instumentation*/
#if 0
            if ((pComponentPrivate->nLcml_nCntIp == 1)) {
                PERF_Boundary(pComponentPrivate->pPERFcomp,
                              PERF_BoundaryStart | PERF_BoundarySteadyState);
            }
#endif
            PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                              PREF(pBufHeader,pBuffer),
                              pComponentPrivate->pPortDef[WBAMR_DEC_INPUT_PORT]->nBufferSize,
                              PERF_ModuleCommonLayer);
#endif


            phandle = (LCML_DSP_INTERFACE *)(((LCML_CODEC_INTERFACE *)pLcmlHandle->pCodecinterfacehandle)->pCodec);

            if( (pLcmlHdr->pBufferParam->usNbFrames < nFrames) && (pLcmlHdr->pFrameParam!=NULL) ){
                OMX_MEMFREE_STRUCT_DSPALIGN(pLcmlHdr->pFrameParam, WAMRDEC_FrameStruct); /*This means that more memory need to be used*/
                OMX_DmmUnMap(phandle->dspCodec->hProc, /*Unmap DSP memory used*/
                             (void*)pLcmlHdr->pBufferParam->pParamElem,
                             pLcmlHdr->pDmmBuf->pReserved, pComponentPrivate->dbg);
                pLcmlHdr->pBufferParam->pParamElem = NULL;
            }

            if(pLcmlHdr->pFrameParam==NULL ){
                OMX_MALLOC_SIZE_DSPALIGN(pLcmlHdr->pFrameParam, (sizeof(WAMRDEC_FrameStruct)*nFrames),WAMRDEC_FrameStruct);
                eError = OMX_DmmMap(phandle->dspCodec->hProc,
                                    nFrames*sizeof(WAMRDEC_FrameStruct),
                                    (void*)pLcmlHdr->pFrameParam,
                                    (pLcmlHdr->pDmmBuf), pComponentPrivate->dbg);

                if (eError != OMX_ErrorNone){
                    OMX_ERROR4(pComponentPrivate->dbg, "OMX_DmmMap ERRROR!!!!\n\n");
                    goto EXIT;
                }
                pLcmlHdr->pBufferParam->pParamElem = (WAMRDEC_FrameStruct *)pLcmlHdr->pDmmBuf->pMapped;/*DSP Address*/
            }
            for(i=0;i<nFrames;i++){
                (pLcmlHdr->pFrameParam+i)->usLastFrame = 0;
                (pLcmlHdr->pFrameParam+i)->usFrameLost = 0;
            }

            /* We only support frame lost error concealment if there is one frame per buffer */
            if (nFrames == 1)
            {
                /* if the bFrameLost flag is set it means that the client has
                   indicated that the next frame is corrupt so set the frame lost
                   frame parameter */
                if (pComponentPrivate->bFrameLost == 1)
                {
                    pLcmlHdr->pFrameParam->usFrameLost = 1;
                    /* clear the internal frame lost flag */
                    pComponentPrivate->bFrameLost = OMX_FALSE;
                }
            }
            isFrameParamChanged = OMX_TRUE;

            /** ring tone**/
            if(pComponentPrivate->SendAfterEOS == 1){
                OMX_PRINT2(pComponentPrivate->dbg, "reconfiguring SN\n");
                if(pComponentPrivate->dasfmode == 1) {
                    OMX_U32 pValues[4];
                    OMX_PRDSP2(pComponentPrivate->dbg, "---- Comp: DASF Functionality is ON ---\n");
                    if(pComponentPrivate->streamID == 0)
                    {
                        OMX_ERROR4(pComponentPrivate->dbg, "**************************************\n");
                        OMX_ERROR4(pComponentPrivate->dbg, ":: Error = OMX_ErrorInsufficientResources\n");
                        OMX_ERROR4(pComponentPrivate->dbg, "**************************************\n");
                        eError = OMX_ErrorInsufficientResources;
                        pComponentPrivate->curState = OMX_StateInvalid;
                        pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                               pComponentPrivate->pHandle->pApplicationPrivate,
                                                               OMX_EventError,
                                                               OMX_ErrorInvalidState,
                                                               OMX_TI_ErrorMajor,
                                                               "AM: No Stream ID Available");
                        goto EXIT;
                    }
                    OMX_MALLOC_SIZE_DSPALIGN(pComponentPrivate->pParams, sizeof(WBAMR_DEC_AudioCodecParams),WBAMR_DEC_AudioCodecParams);
                    pParams =  pComponentPrivate->pParams;

                    pParams->iAudioFormat = 1;
                    pParams->iSamplingRate = 16000;

                    pParams->iStrmId = pComponentPrivate->streamID;

                    pValues[0] = USN_STRMCMD_SETCODECPARAMS;
                    pValues[1] = (OMX_U32)pParams;
                    pValues[2] = sizeof(WBAMR_DEC_AudioCodecParams);
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               EMMCodecControlStrmCtrl,(void *)pValues);

                    if(eError != OMX_ErrorNone) {
                        OMX_ERROR4(pComponentPrivate->dbg, "Error Occurred in Codec StreamControl..\n");
                        pComponentPrivate->curState = OMX_StateInvalid;
                        pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                               pComponentPrivate->pHandle->pApplicationPrivate,
                                                               OMX_EventError,
                                                               eError,
                                                               OMX_TI_ErrorSevere,
                                                               NULL);
                        goto EXIT;
                    }
                }

                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                           EMMCodecControlStart, (void *)p);

                if(eError != OMX_ErrorNone) {
                    OMX_ERROR4(pComponentPrivate->dbg, "Error Occurred in Codec Start..\n");
                    pComponentPrivate->curState = OMX_StateInvalid;
                    pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                           pComponentPrivate->pHandle->pApplicationPrivate,
                                                           OMX_EventError,
                                                           eError,
                                                           OMX_TI_ErrorSevere,
                                                           NULL);
                    goto EXIT;
                }
                pComponentPrivate->SendAfterEOS = 0;
            }

            if((pBufHeader->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) {
                (pLcmlHdr->pFrameParam+(nFrames-1))->usLastFrame |= OMX_BUFFERFLAG_EOS;
                isFrameParamChanged = OMX_TRUE;
                pBufHeader->nFlags = 0;
                if(!pComponentPrivate->dasfmode){
                    if(!pBufHeader->nFilledLen){
                        pComponentPrivate->pOutputBufferList->pBufHdr[0]->nFlags |= OMX_BUFFERFLAG_EOS;
                    }
                    pComponentPrivate->cbInfo.EventHandler( pComponentPrivate->pHandle,
                                                            pComponentPrivate->pHandle->pApplicationPrivate,
                                                            OMX_EventBufferFlag,
                                                            pComponentPrivate->pOutputBufferList->pBufHdr[0]->nOutputPortIndex,
                                                            pComponentPrivate->pOutputBufferList->pBufHdr[0]->nFlags, NULL);
                }
                pComponentPrivate->SendAfterEOS = 1;
                OMX_PRINT1(pComponentPrivate->dbg, "OMX_WbAmrDec_Utils.c : pComponentPrivate->SendAfterEOS %d\n",pComponentPrivate->SendAfterEOS);
            }

            if (isFrameParamChanged == OMX_TRUE) {
               isFrameParamChanged = OMX_FALSE;
               //Issue an initial memory flush to ensure cache coherency */
               OMX_PRINT1(pComponentPrivate->dbg, "OMX_WbAmrDec_Utils.c : flushing pFrameParam\n");
              status = DSPProcessor_FlushMemory(phandle->dspCodec->hProc, pLcmlHdr->pFrameParam, nFrames*sizeof(WAMRDEC_FrameStruct), 0);
               if(DSP_FAILED(status))
               {
                 OMXDBG_PRINT(stderr, ERROR, 4, 0, "Unable to flush mapped buffer: error 0x%x",(int)status);
                 goto EXIT;
               }
            }
            pLcmlHdr->pBufferParam->usNbFrames = nFrames;
            /*---------------------------------------------------------------*/
            /* Store time stamp information */
            pComponentPrivate->arrBufIndex[pComponentPrivate->IpBufindex] = pBufHeader->nTimeStamp;
            /* Store nTickCount information */
            pComponentPrivate->arrTickCount[pComponentPrivate->IpBufindex] = pBufHeader->nTickCount;
            pComponentPrivate->IpBufindex++;
            pComponentPrivate->IpBufindex %= pPortDefIn->nBufferCountActual;


            for (i=0; i < INPUT_WBAMRDEC_BUFFER_SIZE_MIME; i++)
            {
                OMX_PRBUFFER2(pComponentPrivate->dbg, "Queueing pBufHeader->pBuffer[%d] = %x\n",i,pBufHeader->pBuffer[i]);
            }
            if (pComponentPrivate->curState == OMX_StateExecuting)
            {
                if (!WBAMR_DEC_IsPending(pComponentPrivate,pBufHeader,OMX_DirInput))
                {
                    WBAMR_DEC_SetPending(pComponentPrivate,pBufHeader,OMX_DirInput,__LINE__);
                    if (pComponentPrivate->mimemode == WBAMRDEC_MIMEMODE)
                        eError = LCML_QueueBuffer(pLcmlHandle->pCodecinterfacehandle,
                                                  EMMCodecInputBuffer,
                                                  (OMX_U8*)pBufHeader->pBuffer,
                                                  INPUT_WBAMRDEC_BUFFER_SIZE_MIME*nFrames,
                                                  INPUT_WBAMRDEC_BUFFER_SIZE_MIME*nFrames,
                                                  (OMX_U8*)pLcmlHdr->pBufferParam,
                                                  sizeof(WBAMRDEC_ParamStruct),
                                                  NULL);
                    else if (pComponentPrivate->mimemode == WBAMRDEC_IF2)
                        eError = LCML_QueueBuffer(pLcmlHandle->pCodecinterfacehandle,
                                                  EMMCodecInputBuffer,
                                                  (OMX_U8*)pBufHeader->pBuffer,
                                                  INPUT_WBAMRDEC_BUFFER_SIZE_IF2*nFrames,
                                                  INPUT_WBAMRDEC_BUFFER_SIZE_IF2*nFrames,
                                                  (OMX_U8*)pLcmlHdr->pBufferParam,
                                                  sizeof(WBAMRDEC_ParamStruct),
                                                  NULL);
                    else /*Standard*/
                        eError = LCML_QueueBuffer(pLcmlHandle->pCodecinterfacehandle,
                                                  EMMCodecInputBuffer,
                                                  (OMX_U8*)pBufHeader->pBuffer,
                                                  INPUT_WBAMRDEC_BUFFER_SIZE*nFrames,
                                                  INPUT_WBAMRDEC_BUFFER_SIZE*nFrames,
                                                  (OMX_U8*)pLcmlHdr->pBufferParam,
                                                  sizeof(WBAMRDEC_ParamStruct),
                                                  NULL);
                    if (eError != OMX_ErrorNone) {
                        eError = OMX_ErrorHardware;
                        goto EXIT;
                    }
                    pComponentPrivate->lcml_nIpBuf++;
                }
            }else if(pComponentPrivate->curState == OMX_StatePause){
                pComponentPrivate->pInputBufHdrPending[pComponentPrivate->nNumInputBufPending++] = pBufHeader;
            }
            if(pBufHeader->pMarkData){
                if(pComponentPrivate->pOutputBufferList->pBufHdr[0]!=NULL){
                    /* copy mark to output buffer header */
                    pComponentPrivate->pOutputBufferList->pBufHdr[0]->pMarkData = pBufHeader->pMarkData;
                    pComponentPrivate->pOutputBufferList->pBufHdr[0]->hMarkTargetComponent = pBufHeader->hMarkTargetComponent;
                }
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
        }
        else
        {
            OMX_PRBUFFER2(pComponentPrivate->dbg, "line %d:: No Frames in Buffer, calling EmptyBufferDone\n",__LINE__);
            pComponentPrivate->nEmptyBufferDoneCount++;
#ifdef __PERF_INSTRUMENTATION__
            PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                      pBufHeader->pBuffer,
                                      0,
                                      PERF_ModuleHLMM);
#endif
            pComponentPrivate->cbInfo.EmptyBufferDone( pComponentPrivate->pHandle,
                                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                                       pBufHeader);
            SignalIfAllBuffersAreReturned(pComponentPrivate);
        }

        if (pComponentPrivate->bFlushInputPortCommandPending) {
            OMX_SendCommand(pComponentPrivate->pHandle,OMX_CommandFlush,0,NULL);
        }
    }

    else if (eDir == OMX_DirOutput) {
        /* Make sure that output buffer is issued to output stream only when
         * there is an outstanding input buffer already issued on input stream
         */

        /*******/
        pComponentPrivate->nUnhandledFillThisBuffers--;
        if (pComponentPrivate->curState == OMX_StateIdle){
            pComponentPrivate->cbInfo.FillBufferDone (pComponentPrivate->pHandle,
                                                      pComponentPrivate->pHandle->pApplicationPrivate,
                                                      pBufHeader);
            pComponentPrivate->nFillBufferDoneCount++;
            SignalIfAllBuffersAreReturned(pComponentPrivate);
            OMX_PRBUFFER2(pComponentPrivate->dbg, ":: %d %s In idle state return output buffers\n", __LINE__, __FUNCTION__);
            goto EXIT;
        }
        eError = WBAMR_DEC_GetCorresponding_LCMLHeader(pComponentPrivate, pBufHeader->pBuffer, OMX_DirOutput, &pLcmlHdr);

        phandle = (LCML_DSP_INTERFACE *)(((LCML_CODEC_INTERFACE *)pLcmlHandle->pCodecinterfacehandle)->pCodec);

#ifdef __PERF_INSTRUMENTATION__
        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                          PREF(pBufHeader,pBuffer),
                          0,
                          PERF_ModuleCommonLayer);
#endif


        nFrames = (OMX_U8)(pBufHeader->nAllocLen/OUTPUT_WBAMRDEC_BUFFER_SIZE);

        if( (pLcmlHdr->pBufferParam->usNbFrames < nFrames) && (pLcmlHdr->pFrameParam!=NULL) ){
            OMX_MEMFREE_STRUCT_DSPALIGN(pLcmlHdr->pFrameParam, WAMRDEC_FrameStruct);
            OMX_DmmUnMap(phandle->dspCodec->hProc, /*Unmap DSP memory used*/
                         (void*)pLcmlHdr->pBufferParam->pParamElem,
                         pLcmlHdr->pDmmBuf->pReserved, pComponentPrivate->dbg);
            pLcmlHdr->pBufferParam->pParamElem = NULL;
        }

        if(pLcmlHdr->pFrameParam==NULL ){
            OMX_MALLOC_SIZE_DSPALIGN(pLcmlHdr->pFrameParam, (sizeof(WAMRDEC_FrameStruct)*nFrames),WAMRDEC_FrameStruct);
            eError = OMX_DmmMap(phandle->dspCodec->hProc,
                                nFrames*sizeof(WAMRDEC_FrameStruct),
                                (void*)pLcmlHdr->pFrameParam,
                                (pLcmlHdr->pDmmBuf),
                                pComponentPrivate->dbg);

            if (eError != OMX_ErrorNone)
            {
                OMX_ERROR4(pComponentPrivate->dbg, "OMX_DmmMap ERRROR!!!!\n");
                goto EXIT;
            }

            pLcmlHdr->pBufferParam->pParamElem = (WAMRDEC_FrameStruct *)pLcmlHdr->pDmmBuf->pMapped;/*DSP Address*/
        }

        pLcmlHdr->pBufferParam->usNbFrames = nFrames;

        for(i=0;i<pLcmlHdr->pBufferParam->usNbFrames;i++){
            (pLcmlHdr->pFrameParam+i)->usLastFrame = 0;
            (pLcmlHdr->pFrameParam+i)->usFrameLost = 0;
        }

        //Issue an initial memory flush to ensure cache coherency */
        OMX_PRINT1(pComponentPrivate->dbg, "OMX_WbAmrDec_Utils.c : flushing pFrameParam output\n");
        status = DSPProcessor_FlushMemory(phandle->dspCodec->hProc, pLcmlHdr->pFrameParam, nFrames*sizeof(WAMRDEC_FrameStruct), 0);
        if(DSP_FAILED(status))
        {
           OMXDBG_PRINT(stderr, ERROR, 4, 0, "Unable to flush mapped buffer: error 0x%x",(int)status);
           goto EXIT;
        }

        if (pComponentPrivate->curState == OMX_StateExecuting) {
            if (!WBAMR_DEC_IsPending(pComponentPrivate,pBufHeader,OMX_DirOutput)) {
                WBAMR_DEC_SetPending(pComponentPrivate,pBufHeader,OMX_DirOutput,__LINE__);
                eError = LCML_QueueBuffer(pLcmlHandle->pCodecinterfacehandle,
                                          EMMCodecOuputBuffer,
                                          (OMX_U8 *)pBufHeader->pBuffer,
                                          OUTPUT_WBAMRDEC_BUFFER_SIZE*nFrames,
                                          OUTPUT_WBAMRDEC_BUFFER_SIZE*nFrames,
                                          (OMX_U8 *) pLcmlHdr->pBufferParam,
                                          sizeof(WBAMRDEC_ParamStruct),
                                          NULL);

                if (eError != OMX_ErrorNone ) {
                    OMX_ERROR4(pComponentPrivate->dbg, "IssuingDSP OP: Error Occurred\n");
                    eError = OMX_ErrorHardware;
                    goto EXIT;
                }
                pComponentPrivate->lcml_nOpBuf++;
            }
        }
        else if (pComponentPrivate->curState == OMX_StatePause) {
            pComponentPrivate->pOutputBufHdrPending[pComponentPrivate->nNumOutputBufPending++] = pBufHeader;
        }

        if (pComponentPrivate->bFlushOutputPortCommandPending) {
            OMX_SendCommand( pComponentPrivate->pHandle,
                             OMX_CommandFlush,
                             1,NULL);
        }
    }
    else {
        eError = OMX_ErrorBadParameter;
    }

 EXIT:
    if (TOCframetype != NULL) {
	 free(TOCframetype);
    }
    OMX_PRINT1(pComponentPrivate->dbg, "Exiting from  WBAMR_DEC_HandleDataBuf_FromApp \n");
    OMX_PRINT1(pComponentPrivate->dbg, "Returning error %d\n",eError);
    if (eError != OMX_ErrorNone ) {
        pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                               pComponentPrivate->pHandle->pApplicationPrivate,
                                               OMX_EventError,
                                               eError,
                                               OMX_TI_ErrorSevere,
                                               NULL);
    }
    return eError;
}



/*-------------------------------------------------------------------*/
/**
* WBAMR_DEC_GetBufferDirection () This function is used by the component thread to
* request a buffer from the application.  Since it was called from 2 places,
* it made sense to turn this into a small function.
*
* @param pData pointer to AMR Decoder Context Structure
* @param pCur pointer to the buffer to be requested to be filled
*
* @retval none
**/
/*-------------------------------------------------------------------*/

OMX_ERRORTYPE WBAMR_DEC_GetBufferDirection(OMX_BUFFERHEADERTYPE *pBufHeader, OMX_DIRTYPE *eDir)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate = pBufHeader->pPlatformPrivate;
    OMX_U32 nBuf;
    OMX_BUFFERHEADERTYPE *pBuf = NULL;
    OMX_U16 flag = 1,i;

    OMX_PRINT1(pComponentPrivate->dbg, "Entering WBAMR_DEC_GetBufferDirection Function\n");

    /*Search this buffer in input buffers list */
    nBuf = pComponentPrivate->pInputBufferList->numBuffers;
    for(i=0; i<nBuf; i++)
    {
        pBuf = pComponentPrivate->pInputBufferList->pBufHdr[i];
        if(pBufHeader == pBuf)
        {
            *eDir = OMX_DirInput;
            OMX_PRBUFFER2(pComponentPrivate->dbg, "WBAMR_DEC_Buffer %p is INPUT BUFFER\n", pBufHeader);
            flag = 0;
            goto EXIT;
        }
    }

    /*Search this buffer in input buffers list */
    nBuf = pComponentPrivate->pOutputBufferList->numBuffers;

    for(i=0; i<nBuf; i++)
    {
        pBuf = pComponentPrivate->pOutputBufferList->pBufHdr[i];
        if(pBufHeader == pBuf)
        {
            *eDir = OMX_DirOutput;
            OMX_PRBUFFER2(pComponentPrivate->dbg, "WBAMR_DEC_Buffer %p is OUTPUT BUFFER\n", pBufHeader);
            flag = 0;
            goto EXIT;
        }
    }

    if (flag == 1)
    {
        OMX_ERROR4(pComponentPrivate->dbg, "WBAMR_DEC_Buffer %p is Not Found in the List\n",pBufHeader);
        eError = OMX_ErrorUndefined;
        goto EXIT;
    }
 EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, "Exiting WBAMR_DEC_GetBufferDirection Function\n");
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
OMX_ERRORTYPE WBAMR_DEC_LCML_Callback (TUsnCodecEvent event,void * args [10])
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U8 *pBuffer = args[1];
    LCML_WBAMR_DEC_BUFHEADERTYPE *pLcmlHdr;
#ifdef RESOURCE_MANAGER_ENABLED
    OMX_ERRORTYPE rm_error = OMX_ErrorNone;
#endif
    /*    ssize_t ret; */
    LCML_DSP_INTERFACE *pLcmlHandle;
    OMX_U8 i;
    WBAMRDEC_BUFDATA *OutputFrames;
#ifdef WBAMR_DEC_DEBUG
    LCML_DSP_INTERFACE *phandle = (LCML_DSP_INTERFACE *)args[6];
#endif
    OMX_COMPONENTTYPE *pHandle = NULL;
#ifndef UNDER_CE
    char *pArgs = "damedesuStr";
#endif
    DSP_STATUS status;
    LCML_DSP_INTERFACE *dspphandle = (LCML_DSP_INTERFACE *)args[6];

    WBAMR_DEC_COMPONENT_PRIVATE* pComponentPrivate = NULL;
    pComponentPrivate = (WBAMR_DEC_COMPONENT_PRIVATE*)((LCML_DSP_INTERFACE*)args[6])->pComponentPrivate;

    pHandle = pComponentPrivate->pHandle;

    OMX_PRINT1(pComponentPrivate->dbg, "Entering the WBAMR_DEC_LCML_Callback Function\n");
    OMX_PRINT2(pComponentPrivate->dbg, "args = %p ",args[0]);
    OMX_PRINT2(pComponentPrivate->dbg, "event = %d\n",event);

    switch(event) {

    case EMMCodecDspError:
        OMX_PRDSP2(pComponentPrivate->dbg, "[LCML CALLBACK EVENT]  EMMCodecDspError\n");
        break;

    case EMMCodecInternalError:
        OMX_ERROR4(pComponentPrivate->dbg, "[LCML CALLBACK EVENT]  EMMCodecInternalError\n");
        break;

    case EMMCodecInitError:
        OMX_ERROR4(pComponentPrivate->dbg, "[LCML CALLBACK EVENT]  EMMCodecInitError\n");
        break;

    case EMMCodecDspMessageRecieved:
        OMX_PRDSP2(pComponentPrivate->dbg, "[LCML CALLBACK EVENT]  EMMCodecDspMessageRecieved\n");
        break;

    case EMMCodecBufferProcessed:
        OMX_PRDSP2(pComponentPrivate->dbg, "[LCML CALLBACK EVENT]  EMMCodecBufferProcessed\n");
        break;

    case EMMCodecProcessingStarted:
        OMX_PRDSP2(pComponentPrivate->dbg, "[LCML CALLBACK EVENT]  EMMCodecProcessingStarted\n");
        break;

    case EMMCodecProcessingPaused:
        OMX_PRDSP2(pComponentPrivate->dbg, "[LCML CALLBACK EVENT]  EMMCodecProcessingPaused\n");
        break;

    case EMMCodecProcessingStoped:
        OMX_PRDSP2(pComponentPrivate->dbg, "[LCML CALLBACK EVENT]  EMMCodecProcessingStoped\n");
        break;

    case EMMCodecProcessingEof:
        OMX_PRDSP2(pComponentPrivate->dbg, "[LCML CALLBACK EVENT]  EMMCodecProcessingEof\n");
        break;

    case EMMCodecBufferNotProcessed:
        OMX_PRDSP2(pComponentPrivate->dbg, "[LCML CALLBACK EVENT]  EMMCodecBufferNotProcessed\n");
        break;

    case EMMCodecAlgCtrlAck:
        OMX_PRDSP2(pComponentPrivate->dbg, "[LCML CALLBACK EVENT]  EMMCodecAlgCtrlAck\n");
        break;

    case EMMCodecStrmCtrlAck:
        OMX_PRDSP2(pComponentPrivate->dbg, "[LCML CALLBACK EVENT]  EMMCodecStrmCtrlAck\n");
        break;
    }


    if(event == EMMCodecBufferProcessed)
    {
        if( ((OMX_U32)args [0] == EMMCodecInputBuffer) &&
            (pComponentPrivate->pPortDef[WBAMR_DEC_INPUT_PORT]->bEnabled)) {
            OMX_PRBUFFER2(pComponentPrivate->dbg, "Input: pBuffer = %p\n", pBuffer);

            eError = WBAMR_DEC_GetCorresponding_LCMLHeader(pComponentPrivate,pBuffer, OMX_DirInput, &pLcmlHdr);
#ifdef __PERF_INSTRUMENTATION__
            PERF_ReceivedFrame(pComponentPrivate->pPERFcomp,
                               PREF(pLcmlHdr->buffer,pBuffer),
                               0,
                               PERF_ModuleCommonLayer);
#endif
            if (eError != OMX_ErrorNone) {
                OMX_ERROR4(pComponentPrivate->dbg, "Error: Invalid WBAMR_DEC_Buffer Came ...\n");
                goto EXIT;
            }
            WBAMR_DEC_ClearPending(pComponentPrivate,pLcmlHdr->buffer,OMX_DirInput,__LINE__);

            OMX_PRBUFFER2(pComponentPrivate->dbg, "Calling EmptyBufferDone\n");
            OMX_PRBUFFER1(pComponentPrivate->dbg, "pComponentPrivate->nHoldLength = %ld\n",pComponentPrivate->nHoldLength);
            pComponentPrivate->nEmptyBufferDoneCount++;
#ifdef __PERF_INSTRUMENTATION__
            PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                              PREF(pLcmlHdr->buffer,pBuffer),
                              0,
                              PERF_ModuleHLMM);
#endif
            pComponentPrivate->cbInfo.EmptyBufferDone (pHandle,
                                                       pHandle->pApplicationPrivate,
                                                       pLcmlHdr->buffer);
            SignalIfAllBuffersAreReturned(pComponentPrivate);
            pComponentPrivate->lcml_nIpBuf--;
            pComponentPrivate->app_nBuf++;

        } else if ((OMX_U32)args [0] == EMMCodecOuputBuffer) {
            OMX_PRBUFFER2(pComponentPrivate->dbg, "Output: pBufferr = %p\n", pBuffer);

            eError = WBAMR_DEC_GetCorresponding_LCMLHeader(pComponentPrivate,pBuffer, OMX_DirOutput, &pLcmlHdr);
            if (eError != OMX_ErrorNone) {
                OMX_ERROR4(pComponentPrivate->dbg, "Error: Invalid WBAMR_DEC_Buffer Came ...\n");
                goto EXIT;
            }
            if (!pComponentPrivate->bStopSent)
            {
                pLcmlHdr->buffer->nFilledLen = (OMX_U32)args[8];
            }
            else
                pLcmlHdr->buffer->nFilledLen = 0;

            OMX_PRDSP2(pComponentPrivate->dbg, "WBAMR_DEC_LCML_Callback::: pLcmlHdr->buffer->nFilledLen = %ld\n",pLcmlHdr->buffer->nFilledLen);
            OutputFrames = (pLcmlHdr->buffer)->pOutputPortPrivate;
            OutputFrames->nFrames = (OMX_U8) ((OMX_U32)args[8] / OUTPUT_WBAMRDEC_BUFFER_SIZE);

#ifdef __PERF_INSTRUMENTATION__
            PERF_ReceivedFrame(pComponentPrivate->pPERFcomp,
                               PREF(pLcmlHdr->buffer,pBuffer),
                               PREF(pLcmlHdr->buffer,nFilledLen),
                               PERF_ModuleCommonLayer);
            pComponentPrivate->nLcml_nCntOpReceived++;
            if ((pComponentPrivate->nLcml_nCntIp >= 1) && (pComponentPrivate->nLcml_nCntOpReceived == 1)) {
                PERF_Boundary(pComponentPrivate->pPERFcomp,
                              PERF_BoundaryStart | PERF_BoundarySteadyState);
            }
#endif

            WBAMR_DEC_ClearPending(pComponentPrivate,pLcmlHdr->buffer,OMX_DirOutput,__LINE__);
            pComponentPrivate->nOutStandingFillDones++;
            OMX_PRINT2(pComponentPrivate->dbg, "Incremented pComponentPrivate->nOutStandingFillDones = %ld\n",pComponentPrivate->nOutStandingFillDones);
            for(i=0;i<pLcmlHdr->pBufferParam->usNbFrames;i++){
                if ((((pLcmlHdr->pFrameParam+i)->usLastFrame) & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS){
                    (pLcmlHdr->pFrameParam+i)->usLastFrame = 0;
                    pLcmlHdr->buffer->nFlags |= OMX_BUFFERFLAG_EOS;
                    OMX_PRCOMM2(pComponentPrivate->dbg, "On Component receiving OMX_BUFFERFLAG_EOS on output\n");
                    //Issue an initial memory flush to ensure cache coherency */
                    OMX_PRINT1(pComponentPrivate->dbg, "OMX_WbAmrDec_Utils.c : flushing pFrameParam2\n");
                    status = DSPProcessor_FlushMemory(dspphandle->dspCodec->hProc, pLcmlHdr->pFrameParam, pLcmlHdr->pBufferParam->usNbFrames*sizeof(WAMRDEC_FrameStruct), 0);
                    if(DSP_FAILED(status))
                    {
                      OMXDBG_PRINT(stderr, ERROR, 4, 0, "Unable to flush mapped buffer: error 0x%x",(int)status);
                      goto EXIT;
                    }
                    break;
                }
            }
            /* Copying time stamp information to output buffer */
            pLcmlHdr->buffer->nTimeStamp = (OMX_TICKS)pComponentPrivate->arrBufIndex[pComponentPrivate->OpBufindex];
            /* Copying nTickCount information to output buffer */
            pLcmlHdr->buffer->nTickCount = pComponentPrivate->arrTickCount[pComponentPrivate->OpBufindex];
            pComponentPrivate->OpBufindex++;
            pComponentPrivate->OpBufindex %= pComponentPrivate->pPortDef[OMX_DirInput]->nBufferCountActual;

            pComponentPrivate->LastOutbuf = pLcmlHdr->buffer;
            pComponentPrivate->num_Reclaimed_Op_Buff++;
            OMX_PRBUFFER2(pComponentPrivate->dbg, "Calling FillBufferDone From Line\n");
#ifdef __PERF_INSTRUMENTATION__
            PERF_SendingBuffer(pComponentPrivate->pPERFcomp,
                               PREF(pLcmlHdr->buffer,pBuffer),
                               PREF(pLcmlHdr->buffer,nFilledLen),
                               PERF_ModuleHLMM);
#endif
            pComponentPrivate->cbInfo.FillBufferDone (pHandle,
                                                      pHandle->pApplicationPrivate,
                                                      pLcmlHdr->buffer);
            SignalIfAllBuffersAreReturned(pComponentPrivate);
            pComponentPrivate->lcml_nOpBuf--;
            pComponentPrivate->app_nBuf++;
            pComponentPrivate->nFillBufferDoneCount++;
            pComponentPrivate->nOutStandingFillDones--;
            OMX_PRBUFFER2(pComponentPrivate->dbg, "Incrementing app_nBuf = %ld\n",pComponentPrivate->app_nBuf);
        }
    } else if (event == EMMCodecStrmCtrlAck) {
        OMX_PRINT1(pComponentPrivate->dbg, "GOT MESSAGE USN_DSPACK_STRMCTRL ----\n");
        if (args[1] == (void *)USN_STRMCMD_FLUSH) {
            pHandle = pComponentPrivate->pHandle;
            if ( args[2] == (void *)EMMCodecInputBuffer) {
                if (args[0] == (void *)USN_ERR_NONE ) {
                    OMX_PRCOMM2(pComponentPrivate->dbg, "Flushing input port\n");
                    for (i=0; i < pComponentPrivate->nNumInputBufPending; i++) {
                        pComponentPrivate->cbInfo.EmptyBufferDone (pHandle,
                                                                   pHandle->pApplicationPrivate,
                                                                   pComponentPrivate->pInputBufHdrPending[i]);
                        pComponentPrivate->pInputBufHdrPending[i] = NULL;
                        SignalIfAllBuffersAreReturned(pComponentPrivate);
                    }
                    pComponentPrivate->nNumInputBufPending=0;
                    pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventCmdComplete,
                                                           OMX_CommandFlush,
                                                           WBAMR_DEC_INPUT_PORT,
                                                           NULL);
                } else {
                    OMX_ERROR4(pComponentPrivate->dbg, "LCML reported error while flushing input port\n");
                    goto EXIT;
                }
            }
            else if ( args[2] == (void *)EMMCodecOuputBuffer) {
                if (args[0] == (void *)USN_ERR_NONE ) {
                    OMX_PRCOMM1(pComponentPrivate->dbg, "Flushing output port\n");
                    for (i=0; i < pComponentPrivate->nNumOutputBufPending; i++) {
                        pComponentPrivate->cbInfo.FillBufferDone (pHandle,
                                                                  pHandle->pApplicationPrivate,
                                                                  pComponentPrivate->pOutputBufHdrPending[i]);
                        pComponentPrivate->pOutputBufHdrPending[i] = NULL;
                        pComponentPrivate->nFillBufferDoneCount++;
                        SignalIfAllBuffersAreReturned(pComponentPrivate);
                    }
                    pComponentPrivate->nNumOutputBufPending=0;
                    pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventCmdComplete,
                                                           OMX_CommandFlush,
                                                           WBAMR_DEC_OUTPUT_PORT,
                                                           NULL);
                } else {
                    OMX_ERROR4(pComponentPrivate->dbg, "LCML reported error while flushing output port\n");
                    goto EXIT;
                }
            }
        }
    }
    else if(event == EMMCodecProcessingStoped) {
      for (i = 0; i < pComponentPrivate->nNumInputBufPending; i++) {
		pComponentPrivate->cbInfo.EmptyBufferDone (pComponentPrivate->pHandle,
				pComponentPrivate->pHandle->pApplicationPrivate,
				pComponentPrivate->pInputBufHdrPending[i]);
				pComponentPrivate->pInputBufHdrPending[i] = NULL;
				pComponentPrivate->nEmptyBufferDoneCount++;
                SignalIfAllBuffersAreReturned(pComponentPrivate);
	}
	pComponentPrivate->nNumInputBufPending = 0;
	for (i=0; i < pComponentPrivate->nNumOutputBufPending; i++) {
		pComponentPrivate->cbInfo.FillBufferDone (pComponentPrivate->pHandle,
			pComponentPrivate->pHandle->pApplicationPrivate,
			pComponentPrivate->pOutputBufHdrPending[i]);
        pComponentPrivate->nFillBufferDoneCount++;
        SignalIfAllBuffersAreReturned(pComponentPrivate);
 	    pComponentPrivate->nOutStandingFillDones--;
        pComponentPrivate->pOutputBufHdrPending[i] = NULL;
	}
	pComponentPrivate->nNumOutputBufPending=0;
        pthread_mutex_lock(&pComponentPrivate->codecStop_mutex);
        if(pComponentPrivate->codecStop_waitingsignal == 0){
            pComponentPrivate->codecStop_waitingsignal = 1;             
            pthread_cond_signal(&pComponentPrivate->codecStop_threshold);
            OMX_PRINT2(pComponentPrivate->dbg, "stop ack. received. stop waiting for sending disable command completed\n");
        }
        pthread_mutex_unlock(&pComponentPrivate->codecStop_mutex);

        OMX_PRINT2(pComponentPrivate->dbg, "pComponentPrivate->bNoIdleOnStop = %ld\n",pComponentPrivate->bNoIdleOnStop);
        if (!pComponentPrivate->bNoIdleOnStop) {
            pComponentPrivate->nNumOutputBufPending=0;
            /*            pComponentPrivate->ProcessingInputBuf=0;
                          pComponentPrivate->ProcessingOutputBuf=0; */
            pComponentPrivate->nHoldLength = 0;
            /*            pComponentPrivate->InBuf_Eos_alreadysent  =0; */
            /*            OMX_NBMEMFREE_STRUCT(pComponentPrivate->pHoldBuffer); */
            /*            OMX_NBMEMFREE_STRUCT(pComponentPrivate->iMMFDataLastBuffer); */
            pComponentPrivate->curState = OMX_StateIdle;
#ifdef RESOURCE_MANAGER_ENABLED
            eError = RMProxy_NewSendCommand(pHandle,
                                            RMProxy_StateSet,
                                            OMX_WBAMR_Decoder_COMPONENT,
                                            OMX_StateIdle,
                                            3456,
                                            NULL);
#endif
            if((pComponentPrivate->nEmptyThisBufferCount != pComponentPrivate->nEmptyBufferDoneCount) || (pComponentPrivate->nFillThisBufferCount != pComponentPrivate->nFillBufferDoneCount)) {
                if(pthread_mutex_lock(&bufferReturned_mutex) != 0) 
                {
                    OMX_ERROR4(pComponentPrivate->dbg, "%d :: UTIL: bufferReturned_mutex mutex lock error\n",__LINE__);
                }
                 OMX_PRINT2(pComponentPrivate->dbg, ":: pthread_cond_waiting for OMX to return all input and outbut buffers\n");
                pthread_cond_wait(&bufferReturned_condition, &bufferReturned_mutex);
                OMX_PRINT2(pComponentPrivate->dbg, ":: OMX has returned all input and output buffers\n"); 
                if(pthread_mutex_unlock(&bufferReturned_mutex) != 0) 
                {
                    OMX_ERROR4(pComponentPrivate->dbg, "%d :: UTIL: bufferReturned_mutex mutex unlock error\n",__LINE__); 
                }
            }
            else
            {
                OMXDBG_PRINT(stderr, PRINT, 1, 0, "OMX has returned all input and output buffers"); 
            }

            if (pComponentPrivate->bPreempted == 0) {
                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandStateSet,
                                                       pComponentPrivate->curState,
                                                       NULL);
            }
            else {
                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventError,
                                                       OMX_ErrorResourcesPreempted,
                                                       OMX_TI_ErrorSevere,
                                                       0);
            }
        }
        else {
            pComponentPrivate->bNoIdleOnStop = OMX_FALSE;
            pComponentPrivate->bDspStoppedWhileExecuting = OMX_TRUE;
        }
    }
    else if (event == EMMCodecProcessingPaused) {
        pComponentPrivate->curState = OMX_StatePause;
        /* Send StateChangeNotification to application */
        pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                               pComponentPrivate->pHandle->pApplicationPrivate,
                                               OMX_EventCmdComplete,
                                               OMX_CommandStateSet,
                                               pComponentPrivate->curState,
                                               NULL);

    }
    else if (event == EMMCodecDspError) {
        switch ( (OMX_U32) args [4])
        {
        /* USN_ERR_NONE,: Indicates that no error encountered during execution of the command and the command execution completed succesfully.
             * USN_ERR_WARNING,: Indicates that process function returned a warning. The exact warning is returned in Arg2 of this message.
             * USN_ERR_PROCESS,: Indicates that process function returned a error type. The exact error type is returnd in Arg2 of this message.
             * USN_ERR_PAUSE,: Indicates that execution of pause resulted in error.
             * USN_ERR_STOP,: Indicates that execution of stop resulted in error.
             * USN_ERR_ALGCTRL,: Indicates that execution of alg control resulted in error.
             * USN_ERR_STRMCTRL,: Indiactes the execution of STRM control command, resulted in error.
             * USN_ERR_UNKNOWN_MSG,: Indicates that USN received an unknown command. */

#ifdef _ERROR_PROPAGATION__
            case USN_ERR_PAUSE:
            case USN_ERR_STOP:
            case USN_ERR_ALGCTRL:
            case USN_ERR_STRMCTRL:
            case USN_ERR_UNKNOWN_MSG: 

                {
                    pComponentPrivate->bIsInvalidState=OMX_TRUE;
                    pComponentPrivate->curState = OMX_StateInvalid;
                    pHandle = pComponentPrivate->pHandle;
                    pComponentPrivate->cbInfo.EventHandler(pHandle, 
                            pHandle->pApplicationPrivate,
                            OMX_EventError,
                            OMX_ErrorInvalidState, 
                            OMX_TI_ErrorSevere,
                            NULL);
                }
                break;
#endif

            case USN_ERR_WARNING:
            case USN_ERR_PROCESS:
                WBAMRDEC_HandleUSNError (pComponentPrivate, (OMX_U32)args[5]);
                break;
            default:
                break;
        }

    }
    /***********************************************************/
    if(event == EMMCodecDspMessageRecieved) {
        OMX_PRSTATE2(pComponentPrivate->dbg, "commandedState  = %p\n",args[0]);
        OMX_PRINT2(pComponentPrivate->dbg, "arg1 = %p\n",args[1]);
        OMX_PRINT2(pComponentPrivate->dbg, "arg2 = %p\n",args[2]);
    }

#ifdef _ERROR_PROPAGATION__

    else if (event ==EMMCodecInitError){
        /* Cheking for MMU_fault */
        if(((int)args[4] == USN_ERR_UNKNOWN_MSG) && (args[5] == (void*) NULL)) {
            pComponentPrivate->bIsInvalidState=OMX_TRUE;
            pComponentPrivate->curState = OMX_StateInvalid;
            pHandle = pComponentPrivate->pHandle;
            pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                   pHandle->pApplicationPrivate,
                                                   OMX_EventError,
                                                   OMX_ErrorInvalidState,
                                                   OMX_TI_ErrorSevere,
                                                   NULL);
        }
    }
    else if (event ==EMMCodecInternalError){
        /* Cheking for MMU_fault */
        if(((int)args[4] == USN_ERR_UNKNOWN_MSG) && (args[5] == (void*)NULL)) {
            pComponentPrivate->bIsInvalidState=OMX_TRUE;
            pComponentPrivate->curState = OMX_StateInvalid;
            pHandle = pComponentPrivate->pHandle;
            pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                   pHandle->pApplicationPrivate,
                                                   OMX_EventError,
                                                   OMX_ErrorInvalidState,
                                                   OMX_TI_ErrorSevere,
                                                   NULL);
        }

    }
#endif
 EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, "Exiting the WBAMR_DEC_LCML_Callback Function\n");
    return eError;
}

/* -------------------------------------------------------------------*/
/**
  *  WBAMR_DEC_GetCorresponding_LCMLHeader() function will be called by WMADEC_LCML_Callback
  *                                 component to write the msg
  * @param *pBuffer,          Event which gives to details about USN status
  * @param LCML_WBAMR_DEC_BUFHEADERTYPE **ppLcmlHdr

  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
 **/
/* -------------------------------------------------------------------*/

OMX_ERRORTYPE WBAMR_DEC_GetCorresponding_LCMLHeader(WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate,
                                          OMX_U8 *pBuffer,
                                          OMX_DIRTYPE eDir,
                                                    LCML_WBAMR_DEC_BUFHEADERTYPE **ppLcmlHdr)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    LCML_WBAMR_DEC_BUFHEADERTYPE *pLcmlBufHeader;
    OMX_S16 nIpBuf;
    OMX_S16 nOpBuf;
    OMX_S16 i;

    pComponentPrivate = (WBAMR_DEC_COMPONENT_PRIVATE*) pComponentPrivate;
    nIpBuf = pComponentPrivate->pInputBufferList->numBuffers;
    nOpBuf = pComponentPrivate->pOutputBufferList->numBuffers;

    while (!pComponentPrivate->bInitParamsInitialized) {
        OMX_PRINT1(pComponentPrivate->dbg, "Waiting for init to complete\n");
#ifndef UNDER_CE
        sched_yield();
#else
        Sleep(1);
#endif
    }

    OMX_PRINT2(pComponentPrivate->dbg, "eDir = %d\n",eDir);
    if(eDir == OMX_DirInput) {
        pLcmlBufHeader = pComponentPrivate->pLcmlBufHeader[WBAMR_DEC_INPUT_PORT];
        for(i=0; i<nIpBuf; i++) {
            if(pBuffer == pLcmlBufHeader->buffer->pBuffer) {
                *ppLcmlHdr = pLcmlBufHeader;
                OMX_PRDSP2(pComponentPrivate->dbg, "Corresponding LCML Header Found\n");
                goto EXIT;
            }
            pLcmlBufHeader++;
        }
    } else if (eDir == OMX_DirOutput) {
        pLcmlBufHeader = pComponentPrivate->pLcmlBufHeader[WBAMR_DEC_OUTPUT_PORT];

        for(i=0; i<nOpBuf; i++) {

            if(pBuffer == pLcmlBufHeader->buffer->pBuffer) {
                *ppLcmlHdr = pLcmlBufHeader;
                OMX_PRDSP2(pComponentPrivate->dbg, "Corresponding LCML Header Found\n");
                goto EXIT;
            }
            pLcmlBufHeader++;
        }
    } else {
        OMX_PRINT1(pComponentPrivate->dbg, " Invalid WBAMR_DEC_Buffer Type :: exiting...\n");
    }

 EXIT:
    return eError;
}


#ifndef UNDER_CE
/* -------------------------------------------------------------------*/
/**
  *  WMADEC_GetLCMLHandle()
  *
  * @retval OMX_HANDLETYPE
  *
 -------------------------------------------------------------------*/
OMX_HANDLETYPE WBAMR_DEC_GetLCMLHandle(WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate)
{
    void *handle;
    OMX_ERRORTYPE (*fpGetHandle)(OMX_HANDLETYPE);
    OMX_HANDLETYPE pHandle = NULL;
    char *error;
    OMX_ERRORTYPE eError;

    OMX_PRINT1(pComponentPrivate->dbg, "WBAMR_DEC_GetLCMLHandle\n");
    handle = dlopen("libLCML.so", RTLD_LAZY);
    if (!handle) {
        fputs(dlerror(), stderr);
        goto EXIT;
    }

    fpGetHandle = dlsym (handle, "GetHandle");
    if ((error = dlerror()) != NULL) {
        fputs(error, stderr);
	 dlclose(handle);
        goto EXIT;
    }
    eError = (*fpGetHandle)(&pHandle);
    if(eError != OMX_ErrorNone) {
        eError = OMX_ErrorUndefined;
        OMX_ERROR4(pComponentPrivate->dbg, "eError != OMX_ErrorNone...\n");
        pHandle = NULL;
	 dlclose(handle);
        goto EXIT;
    }

    pComponentPrivate->bLcmlHandleOpened = 1;
    ((LCML_DSP_INTERFACE*)pHandle)->pComponentPrivate = pComponentPrivate;

    pComponentPrivate->ptrLibLCML=handle;           /* saving LCML lib pointer  */
 EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, "WBAMR_DEC_GetLCMLHandle returning %p\n",pHandle);

    return pHandle;
}


#else
//WINDOWS Explicit dll load procedure
OMX_HANDLETYPE WBAMR_DEC_GetLCMLHandle(WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate)
{
    typedef OMX_ERRORTYPE (*LPFNDLLFUNC1)(OMX_HANDLETYPE);
    OMX_HANDLETYPE pHandle = NULL;
    OMX_ERRORTYPE eError;
    HINSTANCE hDLL;               // Handle to DLL
    LPFNDLLFUNC1 fpGetHandle1;

    hDLL = LoadLibraryEx(TEXT("OAF_BML.dll"), NULL,0);
    if (hDLL == NULL)
    {
        //fputs(dlerror(), stderr);
        OMX_ERROR4(pComponentPrivate->dbg, "BML Load Failed!!!\n");
        return pHandle;
    }

    fpGetHandle1 = (LPFNDLLFUNC1)GetProcAddress(hDLL,TEXT("GetHandle"));
    if (!fpGetHandle1)
    {
        // handle the error
        FreeLibrary(hDLL);

        return pHandle;
    }

    // call the function
    eError = fpGetHandle1(&pHandle);
    if(eError != OMX_ErrorNone) {
        eError = OMX_ErrorUndefined;
        OMX_ERROR4(pComponentPrivate->dbg, "eError != OMX_ErrorNone...\n");


        pHandle = NULL;
        return pHandle;
    }

    ((LCML_DSP_INTERFACE*)pHandle)->pComponentPrivate = pComponentPrivate;

    return pHandle;
}
#endif


#ifndef UNDER_CE

OMX_ERRORTYPE WBAMR_DEC_FreeLCMLHandle(WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate)
{

    int retValue;
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


OMX_ERRORTYPE WBAMR_DEC_FreeLCMLHandle(WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate)
{

    int retValue;
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
/* ================================================================================= */
/**
* @fn WBAMR_DEC_SetPending() description for WMADEC_SetPending
WMADEC_SetPending().
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

void WBAMR_DEC_SetPending(WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir, OMX_U32 lineNumber)
{
    OMX_U16 i;

    if (eDir == OMX_DirInput) {
        for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pInputBufferList->pBufHdr[i]) {
                pComponentPrivate->pInputBufferList->bBufferPending[i] = 1;
                OMX_PRBUFFER2(pComponentPrivate->dbg, "*******************INPUT BUFFER %d IS PENDING Line %ld******************************\n",i,lineNumber);
            }
        }
    }
    else {
        for (i=0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pOutputBufferList->pBufHdr[i]) {
                pComponentPrivate->pOutputBufferList->bBufferPending[i] = 1;
                OMX_PRBUFFER2(pComponentPrivate->dbg, "*******************OUTPUT BUFFER %d IS PENDING Line %ld******************************\n",i,lineNumber);
            }
        }
    }
}
/* ================================================================================= */
/**
 * @fn WBAMR_DEC_ClearPending() description for WBAMR_DEC_ClearPending
 WBAMR_DEC_ClearPending().
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

void WBAMR_DEC_ClearPending(WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir, OMX_U32 lineNumber)
{
    OMX_U16 i;

    if (eDir == OMX_DirInput) {
        for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pInputBufferList->pBufHdr[i]) {
                pComponentPrivate->pInputBufferList->bBufferPending[i] = 0;
                OMX_PRBUFFER2(pComponentPrivate->dbg, "*******************INPUT BUFFER %d IS RECLAIMED Line %ld******************************\n",i,lineNumber);
            }
        }
    }
    else {
        for (i=0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pOutputBufferList->pBufHdr[i]) {
                pComponentPrivate->pOutputBufferList->bBufferPending[i] = 0;
                OMX_PRBUFFER2(pComponentPrivate->dbg, "*******************OUTPUT BUFFER %d IS RECLAIMED Line %ld******************************\n",i,lineNumber);
            }
        }
    }
}
/* ================================================================================= */
/**
 * @fn WBAMR_DEC_IsPending() description for WBAMR_DEC_IsPending
 WBAMR_DEC_IsPending().
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

OMX_U32 WBAMR_DEC_IsPending(WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir)
{
    OMX_U16 i;

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
 * @fn WBAMR_DEC_IsValid() description for WBAMR_DEC_IsValid
 WBAMR_DEC_IsValid().
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

OMX_U32 WBAMR_DEC_IsValid(WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U8 *pBuffer, OMX_DIRTYPE eDir)
{
    OMX_U16 i;
    int found=0;

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

/* ================================================================================= */
/**
* @fn WBAMR_DEC_Fill_LCMLInitParamsEx() description for WBAMR_DEC_Fill_LCMLInitParamsEx
WBAMR_DEC_Fill_LCMLInitParamsEx().
This method fills the LCML init parameters.
* @param pComponent  handle for this instance of the component
*
* @pre
*
* @post
*
* @return OMX_ERRORTYPE
*/
/* ================================================================================ */

OMX_ERRORTYPE  WBAMR_DEC_Fill_LCMLInitParamsEx (OMX_HANDLETYPE  pComponent )
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nIpBuf,nIpBufSize,nOpBuf,nOpBufSize;
    OMX_U16 i;
    OMX_BUFFERHEADERTYPE *pTemp;
    int size_lcml;

    LCML_DSP_INTERFACE *pHandle = (LCML_DSP_INTERFACE *)pComponent;
    WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate;
    LCML_WBAMR_DEC_BUFHEADERTYPE *pTemp_lcml;

    OMXDBG_PRINT(stderr, PRINT, 1, 0, "WBAMR_DEC_Fill_LCMLInitParams\n ");
    OMXDBG_PRINT(stderr, DSP, 1, 0, "pHandle = %p\n",pHandle);
    OMXDBG_PRINT(stderr, DSP, 1, 0, "pHandle->pComponentPrivate = %p\n",pHandle->pComponentPrivate);
    pComponentPrivate = pHandle->pComponentPrivate;

    nIpBuf = pComponentPrivate->pInputBufferList->numBuffers;

    if(pComponentPrivate->mimemode == 1)
        nIpBufSize = INPUT_WBAMRDEC_BUFFER_SIZE_MIME;
    else if (pComponentPrivate->mimemode == 2)
    {
        nIpBufSize = INPUT_WBAMRDEC_BUFFER_SIZE_IF2;
    }
    else {
        nIpBufSize = INPUT_WBAMRDEC_BUFFER_SIZE;
    }

    nOpBuf = pComponentPrivate->pOutputBufferList->numBuffers;
    nOpBufSize = OUTPUT_WBAMRDEC_BUFFER_SIZE;


    size_lcml = nIpBuf * sizeof(LCML_WBAMR_DEC_BUFHEADERTYPE);
    OMX_MALLOC_SIZE(pTemp_lcml, size_lcml,LCML_WBAMR_DEC_BUFHEADERTYPE);
    pComponentPrivate->pLcmlBufHeader[WBAMR_DEC_INPUT_PORT] = pTemp_lcml;

    for (i=0; i<nIpBuf; i++) {
        pTemp = pComponentPrivate->pInputBufferList->pBufHdr[i];
        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pTemp->nFilledLen = nIpBufSize;
        pTemp->nVersion.s.nVersionMajor = WBAMR_DEC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = WBAMR_DEC_MINOR_VER;
        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = WBAMR_DEC_NOT_USED;
        pTemp_lcml->buffer = pTemp;
        pTemp_lcml->eDir = OMX_DirInput;

        OMX_MALLOC_SIZE_DSPALIGN(pTemp_lcml->pBufferParam,
                                  sizeof(WBAMRDEC_ParamStruct),
                                  WBAMRDEC_ParamStruct);

        OMX_MALLOC_GENERIC(pTemp_lcml->pDmmBuf, DMM_BUFFER_OBJ);

        pTemp_lcml->pFrameParam = NULL;
        pTemp_lcml->pBufferParam->usNbFrames =0;
        pTemp_lcml->pBufferParam->pParamElem = NULL;

        /* This means, it is not a last buffer. This flag is to be modified by
         * the application to indicate the last buffer */
        pTemp->nFlags = WBAMR_DEC_NORMAL_BUFFER;

        pTemp_lcml++;
    }

    /* Allocate memory for all output buffer headers..
     * This memory pointer will be sent to LCML */
    size_lcml = nOpBuf * sizeof(LCML_WBAMR_DEC_BUFHEADERTYPE);
    OMX_MALLOC_SIZE(pTemp_lcml, size_lcml,LCML_WBAMR_DEC_BUFHEADERTYPE);
    pComponentPrivate->pLcmlBufHeader[WBAMR_DEC_OUTPUT_PORT] = pTemp_lcml;

    for (i=0; i<nOpBuf; i++) {
        pTemp = pComponentPrivate->pOutputBufferList->pBufHdr[i];
        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pTemp->nFilledLen = nOpBufSize;
        pTemp->nVersion.s.nVersionMajor = WBAMR_DEC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = WBAMR_DEC_MINOR_VER;
        pComponentPrivate->nVersion = pTemp->nVersion.nVersion;
        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = WBAMR_DEC_NOT_USED;
        pTemp_lcml->pFrameParam = NULL;

        OMX_MALLOC_SIZE_DSPALIGN(pTemp_lcml->pBufferParam,
                                  sizeof(WBAMRDEC_ParamStruct),
                                  WBAMRDEC_ParamStruct);

        pTemp_lcml->pBufferParam->usNbFrames =0;
        pTemp_lcml->pBufferParam->pParamElem = NULL;
        OMX_MALLOC_GENERIC(pTemp_lcml->pDmmBuf ,DMM_BUFFER_OBJ);

        pTemp_lcml->buffer = pTemp;
        pTemp_lcml->eDir = OMX_DirOutput;
        OMX_PRDSP1(pComponentPrivate->dbg, "pTemp_lcml = %p\n",pTemp_lcml);
        OMX_PRBUFFER2(pComponentPrivate->dbg, "pTemp_lcml->buffer = %p\n",pTemp_lcml->buffer);

        pTemp->nFlags = WBAMR_DEC_NORMAL_BUFFER;

        pTemp++;
        pTemp_lcml++;
    }
    pComponentPrivate->bPortDefsAllocated = 1;


    OMX_PRINT1(pComponentPrivate->dbg, "Exiting WBAMR_DEC_Fill_LCMLInitParams");

    pComponentPrivate->bInitParamsInitialized = 1;
 EXIT:
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
OMX_ERRORTYPE OMX_DmmMap(DSP_HPROCESSOR ProcHandle,
                         int size,
                         void* pArmPtr,
                         DMM_BUFFER_OBJ* pDmmBuf, struct OMX_TI_Debug dbg)
{
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    DSP_STATUS status;
    int nSizeReserved = 0;

    if(pDmmBuf == NULL)
    {
        OMX_ERROR4 (dbg, "pBuf is NULL\n");
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    if(pArmPtr == NULL)
    {
        OMX_ERROR4 (dbg, "pBuf is NULL\n");
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    /* Allocate */
    pDmmBuf->pAllocated = pArmPtr;

    /* Reserve */
    nSizeReserved = ROUND_TO_PAGESIZE(size) + 2*DMM_PAGE_SIZE ;
    status = DSPProcessor_ReserveMemory(ProcHandle, nSizeReserved, &(pDmmBuf->pReserved));

    OMX_PRBUFFER2(dbg, "OMX Reserve DSP: %p\n",pDmmBuf->pReserved);

    if(DSP_FAILED(status))
    {
        OMX_ERROR4 (dbg, "DSPProcessor_ReserveMemory() failed - error 0x%x", (int)status);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }
    pDmmBuf->nSize = size;
    OMX_PRINT2 (dbg, "DMM MAP Reserved: %p, size 0x%x (%d)\n", pDmmBuf->pReserved,nSizeReserved,nSizeReserved);

    /* Map */
    status = DSPProcessor_Map(ProcHandle,
                              pDmmBuf->pAllocated,/* Arm addres of data to Map on DSP*/
                              size , /* size to Map on DSP*/
                              pDmmBuf->pReserved, /* reserved space */
                              &(pDmmBuf->pMapped), /* returned map pointer */
                              0); /* final param is reserved.  set to zero. */
    if(DSP_FAILED(status))
    {
        OMX_ERROR2 (dbg, "DSPProcessor_Map() failed - error 0x%x", (int)status);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }
    OMX_PRINT2 (dbg, "DMM Mapped: %p, size 0x%x (%d)\n",pDmmBuf->pMapped, size,size);

    /* Issue an initial memory flush to ensure cache coherency */
    status = DSPProcessor_FlushMemory(ProcHandle, pDmmBuf->pAllocated, size, 0);
    if(DSP_FAILED(status))
    {
        OMX_ERROR4 (dbg, "Unable to flush mapped buffer: error 0x%x",(int)status);
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
OMX_ERRORTYPE OMX_DmmUnMap(DSP_HPROCESSOR ProcHandle, void* pMapPtr, void* pResPtr, struct OMX_TI_Debug dbg)
{
    DSP_STATUS status = DSP_SOK;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_PRINT1 (dbg, "OMX UnReserve DSP: %p\n",pResPtr);

    if(pMapPtr == NULL)
    {
        OMX_ERROR4 (dbg, "pMapPtr is NULL\n");
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if(pResPtr == NULL)
    {
        OMX_ERROR4 (dbg, "pResPtr is NULL\n");
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }
    status = DSPProcessor_UnMap(ProcHandle,pMapPtr);
    if(DSP_FAILED(status))
    {
        OMX_ERROR4 (dbg, "DSPProcessor_UnMap() failed - error 0x%x",(int)status);
    }

    OMX_PRINT2 (dbg, "unreserving  structure =0x%p\n",pResPtr);
    status = DSPProcessor_UnReserveMemory(ProcHandle,pResPtr);
    if(DSP_FAILED(status))
    {
        OMX_ERROR4 (dbg, "DSPProcessor_UnReserveMemory() failed - error 0x%x", (int)status);
    }

 EXIT:
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
    if(event == NULL){
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
     if(event == NULL){
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
     if(event == NULL){
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
     if(event == NULL){
        ret = OMX_ErrorBadParameter;
        goto EXIT;
     }
     CloseHandle(event->event);
EXIT:
     return ret;
}
#endif

/* ========================================================================== */
/**
* @SignalIfAllBuffersAreReturned() This function send signals if OMX returned all buffers to app 
*
* @param WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate
*
* @pre None
*
* @post None
*
* @return None
*/
/* ========================================================================== */
void SignalIfAllBuffersAreReturned(WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate)
{
    if((pComponentPrivate->nEmptyThisBufferCount == pComponentPrivate->nEmptyBufferDoneCount) && (pComponentPrivate->nFillThisBufferCount == pComponentPrivate->nFillBufferDoneCount))
    {
        if(pthread_mutex_lock(&bufferReturned_mutex) != 0) 
        {
            OMX_ERROR4(pComponentPrivate->dbg, "%d :: bufferReturned_mutex mutex lock error\n",__LINE__); 
        }
        pthread_cond_broadcast(&bufferReturned_condition);
        OMX_PRINT1(pComponentPrivate->dbg, "Sending pthread signal that OMX has returned all buffers to app"); 
        if(pthread_mutex_unlock(&bufferReturned_mutex) != 0) 
        {
            OMX_ERROR4(pComponentPrivate->dbg, "%d :: bufferReturned_mutex mutex unlock error\n",__LINE__); 
        }
        return;
    }
}

#ifdef RESOURCE_MANAGER_ENABLED
void WBAMRDEC_ResourceManagerCallback(RMPROXY_COMMANDDATATYPE cbData)
{
    OMX_COMMANDTYPE Cmd = OMX_CommandStateSet;
    OMX_STATETYPE state = OMX_StateIdle;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)cbData.hComponent;
    WBAMR_DEC_COMPONENT_PRIVATE *pCompPrivate = NULL;

    pCompPrivate = (WBAMR_DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    if (*(cbData.RM_Error) == OMX_RmProxyCallback_ResourcesPreempted) {
        if (pCompPrivate->curState == OMX_StateExecuting ||
            pCompPrivate->curState == OMX_StatePause) {
            write (pCompPrivate->cmdPipe[1], &Cmd, sizeof(Cmd));
            write (pCompPrivate->cmdDataPipe[1], &state ,sizeof(OMX_U32));

            pCompPrivate->bPreempted = 1;
        }
    }
    else if (*(cbData.RM_Error) == OMX_RmProxyCallback_ResourcesAcquired){
        pCompPrivate->cbInfo.EventHandler (
                            pHandle, pHandle->pApplicationPrivate,
                            OMX_EventResourcesAcquired, 0,0,
                            NULL);


    }

}
#endif

void WBAMRDEC_HandleUSNError (WBAMR_DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U32 arg)
{
    OMX_COMPONENTTYPE *pHandle = NULL;
    OMX_U8 pending_buffers = OMX_FALSE;
    OMX_U32 i;
    switch (arg)
    {
        case IUALG_WARN_CONCEALED:
        case IUALG_WARN_UNDERFLOW:
        case IUALG_WARN_OVERFLOW:
        case IUALG_WARN_ENDOFDATA:
            /* all of these are informative messages, Algo can recover, no need to notify the 
             * IL Client at this stage of the implementation */
            break;

        case IUALG_WARN_PLAYCOMPLETED:
        {
            pHandle = pComponentPrivate->pHandle;
            OMX_PRDSP1(pComponentPrivate->dbg, "%d :: GOT MESSAGE IUALG_WARN_PLAYCOMPLETED\n",__LINE__);
            if(pComponentPrivate->LastOutbuf!=NULL && !pComponentPrivate->dasfmode){
                pComponentPrivate->LastOutbuf->nFlags |= OMX_BUFFERFLAG_EOS;
            }
                 
            /* add callback to application to indicate SN/USN has completed playing of current set of date */
            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,                  
                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                   OMX_EventBufferFlag,
                                                   (OMX_U32)NULL,
                                                   OMX_BUFFERFLAG_EOS,
                                                   NULL);
        }
            break;

#ifdef _ERROR_PROPAGATION__
        case IUALG_ERR_BAD_HANDLE:
        case IUALG_ERR_DATA_CORRUPT:
        case IUALG_ERR_NOT_SUPPORTED:
        case IUALG_ERR_ARGUMENT:
        case IUALG_ERR_NOT_READY:
        case IUALG_ERR_GENERAL:
        {
        /* all of these are fatal messages, Algo can not recover
                 * hence return an error */
                pComponentPrivate->bIsInvalidState=OMX_TRUE;
                pComponentPrivate->curState = OMX_StateInvalid;
                pHandle = pComponentPrivate->pHandle;
                pComponentPrivate->cbInfo.EventHandler(pHandle,
                        pHandle->pApplicationPrivate,
                        OMX_EventError,
                        OMX_ErrorInvalidState,
                        OMX_TI_ErrorSevere,
                        NULL);
            }
            break;
#endif
        default:
            break;
    }
}


