
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
 * @file OMX_WbAmrEnc_Utils.c
 *
 * This file implements WBAMR Encoder Component Specific APIs and its functionality
 * that is fully compliant with the Khronos OpenMAX (TM) 1.0 Specification
 *
 * @path  $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\wbamr_enc\src
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
#include <wchar.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <malloc.h>
#include <memory.h>
#include <fcntl.h>
#include <errno.h>

#include <semaphore.h>
#endif

#include <dbapi.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
/*-------program files ----------------------------------------*/
#include "OMX_WbAmrEncoder.h"
#include "OMX_WbAmrEnc_Utils.h"
#include "wbamrencsocket_ti.h"
#include <encode_common_ti.h>
#include "OMX_WbAmrEnc_CompThread.h"
#include "usn.h"
#include "LCML_DspCodec.h"

#ifdef RESOURCE_MANAGER_ENABLED
#include <ResourceManagerProxyAPI.h>
#endif

/* ========================================================================== */
/**
 * @WBAMRENC_FillLCMLInitParams () This function is used by the component thread to
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

OMX_ERRORTYPE WBAMRENC_FillLCMLInitParams(OMX_HANDLETYPE pComponent,
        LCML_DSP *plcml_Init, OMX_U16 arr[]) {
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nIpBuf, nIpBufSize, nOpBuf, nOpBufSize;
    OMX_BUFFERHEADERTYPE *pTemp;
    LCML_DSP_INTERFACE *pHandle = (LCML_DSP_INTERFACE *)pComponent;
    WBAMRENC_COMPONENT_PRIVATE *pComponentPrivate = pHandle->pComponentPrivate;
    WBAMRENC_LCML_BUFHEADERTYPE *pTemp_lcml = NULL;
    OMX_U32 i;
    OMX_U32 size_lcml;
    char *pTemp_char = NULL;

    OMX_PRINT1(pComponentPrivate->dbg, "Entering\n");
    nIpBuf = pComponentPrivate->pInputBufferList->numBuffers;
    pComponentPrivate->nRuntimeInputBuffers = nIpBuf;

    nIpBufSize = pComponentPrivate->pPortDef[WBAMRENC_INPUT_PORT]->nBufferSize;
    nOpBuf = pComponentPrivate->pOutputBufferList->numBuffers;
    pComponentPrivate->nRuntimeOutputBuffers = nOpBuf;

    nOpBufSize = pComponentPrivate->pPortDef[WBAMRENC_OUTPUT_PORT]->nBufferSize;
    OMX_PRBUFFER2(pComponentPrivate->dbg,
                  "------ Buffer Details -----------\n");
    OMX_PRBUFFER2(pComponentPrivate->dbg,
                  "Input  Buffer Count = %ld\n", nIpBuf);
    OMX_PRBUFFER2(pComponentPrivate->dbg,
                  "Input  Buffer Size = %ld\n", nIpBufSize);
    OMX_PRBUFFER2(pComponentPrivate->dbg,
                  "Output Buffer Count = %ld\n", nOpBuf);
    OMX_PRBUFFER2(pComponentPrivate->dbg,
                  "Output Buffer Size = %ld\n", nOpBufSize);
    OMX_PRBUFFER2(pComponentPrivate->dbg,
                  "------ Buffer Details ------------\n");
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

    plcml_Init->NodeInfo.AllUUIDs[0].uuid = &WBAMRENCSOCKET_TI_UUID;
    strcpy ((char*)plcml_Init->NodeInfo.AllUUIDs[0].DllName, WBAMRENC_DLL_NAME);
    plcml_Init->NodeInfo.AllUUIDs[0].eDllType = DLL_NODEOBJECT;

    plcml_Init->NodeInfo.AllUUIDs[1].uuid = &WBAMRENCSOCKET_TI_UUID;
    strcpy ((char*)plcml_Init->NodeInfo.AllUUIDs[1].DllName, WBAMRENC_DLL_NAME);
    plcml_Init->NodeInfo.AllUUIDs[1].eDllType = DLL_DEPENDENT;

    plcml_Init->NodeInfo.AllUUIDs[2].uuid = &USN_TI_UUID;
    strcpy ((char*)plcml_Init->NodeInfo.AllUUIDs[2].DllName,
            WBAMRENC_USN_DLL_NAME);
    plcml_Init->NodeInfo.AllUUIDs[2].eDllType = DLL_DEPENDENT;
    plcml_Init->DeviceInfo.TypeofDevice = 0;

    if (pComponentPrivate->dasfMode == 1) {
        OMX_PRDSP2(pComponentPrivate->dbg,
                   "Codec is configuring to DASF mode\n");
        OMX_MALLOC_GENERIC(pComponentPrivate->strmAttr, LCML_STRMATTR);
        pComponentPrivate->strmAttr->uSegid = WBAMRENC_DEFAULT_SEGMENT;
        pComponentPrivate->strmAttr->uAlignment = 0;
        pComponentPrivate->strmAttr->uTimeout = WBAMRENC_SN_TIMEOUT;
        pComponentPrivate->strmAttr->uBufsize = WBAMRENC_INPUT_BUFFER_SIZE_DASF;
        pComponentPrivate->strmAttr->uNumBufs = WBAMRENC_NUM_INPUT_BUFFERS_DASF;
        pComponentPrivate->strmAttr->lMode = STRMMODE_PROCCOPY;
        /* Device is Configuring to DASF Mode */
        plcml_Init->DeviceInfo.TypeofDevice = 1;
        /* Device is Configuring to Record Mode */
        plcml_Init->DeviceInfo.TypeofRender = 1;

        if (pComponentPrivate->acdnMode == 1) {
            /* ACDN mode */
            plcml_Init->DeviceInfo.AllUUIDs[0].uuid = &ACDN_TI_UUID;
        } else {
            /* DASF/TeeDN mode */
            plcml_Init->DeviceInfo.AllUUIDs[0].uuid = &DCTN_TI_UUID;
        }

        plcml_Init->DeviceInfo.DspStream = pComponentPrivate->strmAttr;
    }

    /*copy the other information*/
    plcml_Init->SegID = WBAMRENC_DEFAULT_SEGMENT;
    plcml_Init->Timeout = WBAMRENC_SN_TIMEOUT;
    plcml_Init->Alignment = 0;
    plcml_Init->Priority = WBAMRENC_SN_PRIORITY;
    plcml_Init->ProfileID = -1;

    /* Setting Creat Phase Parameters here */
    arr[0] = WBAMRENC_STREAM_COUNT;
    arr[1] = WBAMRENC_INPUT_PORT;

    if (pComponentPrivate->dasfMode == 1) {
        arr[2] = WBAMRENC_INSTRM;
        arr[3] = WBAMRENC_NUM_INPUT_BUFFERS_DASF;
    } else {
        arr[2] = WBAMRENC_DMM;

        if (pComponentPrivate->pInputBufferList->numBuffers) {
            arr[3] = (OMX_U16) pComponentPrivate->pInputBufferList->numBuffers;
        } else {
            arr[3] = 1;
        }
    }

    arr[4] = WBAMRENC_OUTPUT_PORT;
    arr[5] = WBAMRENC_DMM;

    if (pComponentPrivate->pOutputBufferList->numBuffers) {
        arr[6] = (OMX_U16) pComponentPrivate->pOutputBufferList->numBuffers;
    } else {
        arr[6] = 1;
    }

    OMX_PRDSP2(pComponentPrivate->dbg, "Codec is configuring to WBAMR mode\n");
    arr[7] = WBAMRENC_WBAMR;

    if (pComponentPrivate->frameMode == WBAMRENC_MIMEMODE) {
        OMX_PRDSP2(pComponentPrivate->dbg, "Codec is configuring MIME mode\n");
        arr[8] = WBAMRENC_MIMEMODE;
    } else if (pComponentPrivate->frameMode == WBAMRENC_IF2 ) {
        OMX_PRDSP2(pComponentPrivate->dbg, "Codec is configuring IF2 mode\n");
        arr[8] = WBAMRENC_IF2;
    } else {
        OMX_PRDSP2(pComponentPrivate->dbg,
                   "Codec is configuring FORMAT CONFORMANCE mode\n");
        arr[8] = WBAMRENC_FORMATCONFORMANCE;
    }

    arr[9] = END_OF_CR_PHASE_ARGS;

    plcml_Init->pCrPhArgs = arr;

    /* Allocate memory for all input buffer headers..
     * This memory pointer will be sent to LCML */
    size_lcml = nIpBuf * sizeof(WBAMRENC_LCML_BUFHEADERTYPE);


    OMX_MALLOC_SIZE(pTemp_lcml, size_lcml, WBAMRENC_LCML_BUFHEADERTYPE);

    pComponentPrivate->pLcmlBufHeader[WBAMRENC_INPUT_PORT] = pTemp_lcml;

    for (i = 0; i < nIpBuf; i++) {
        OMX_PRINT2(pComponentPrivate->dbg, "INPUT--------- Inside Ip Loop\n");
        pTemp = pComponentPrivate->pInputBufferList->pBufHdr[i];
        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pTemp->nFilledLen = nIpBufSize;
        pTemp->nVersion.s.nVersionMajor = WBAMRENC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = WBAMRENC_MINOR_VER;
        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = WBAMRENC_NOT_USED;
        pTemp_lcml->buffer = pTemp;
        OMX_PRBUFFER2(pComponentPrivate->dbg,
                      "pTemp_lcml->buffer->pBuffer = %p \n",
                      pTemp_lcml->buffer->pBuffer);
        pTemp_lcml->eDir = OMX_DirInput;

        OMX_MALLOC_SIZE_DSPALIGN(pTemp_lcml->pBufferParam ,
                                 sizeof(WBAMRENC_ParamStruct),
                                 OMX_U8);

        pTemp_lcml->pBufferParam->usNbFrames = 0;
        pTemp_lcml->pBufferParam->pParamElem = NULL;
        pTemp_lcml->pFrameParam = NULL;

        OMX_MALLOC_GENERIC(pTemp_lcml->pDmmBuf, DMM_BUFFER_OBJ);

        pTemp->nFlags = WBAMRENC_NORMAL_BUFFER;
        pTemp++;
        pTemp_lcml++;
    }

    /* Allocate memory for all output buffer headers..
     * This memory pointer will be sent to LCML */
    size_lcml = nOpBuf * sizeof(WBAMRENC_LCML_BUFHEADERTYPE);

    OMX_MALLOC_SIZE(pTemp_lcml, size_lcml, WBAMRENC_LCML_BUFHEADERTYPE);

    pComponentPrivate->pLcmlBufHeader[WBAMRENC_OUTPUT_PORT] = pTemp_lcml;

    for (i = 0; i < nOpBuf; i++) {
        OMX_PRINT2(pComponentPrivate->dbg, "OUTPUT--------- Inside Op Loop\n");
        pTemp = pComponentPrivate->pOutputBufferList->pBufHdr[i];
        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pTemp->nFilledLen = nOpBufSize;
        pTemp->nVersion.s.nVersionMajor = WBAMRENC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = WBAMRENC_MINOR_VER;
        pComponentPrivate->nVersion = pTemp->nVersion.nVersion;
        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = WBAMRENC_NOT_USED;
        pTemp_lcml->buffer = pTemp;
        OMX_PRBUFFER2(pComponentPrivate->dbg,
                      "pTemp_lcml->buffer->pBuffer = %p \n",
                      pTemp_lcml->buffer->pBuffer);
        pTemp_lcml->eDir = OMX_DirOutput;

        OMX_MALLOC_SIZE_DSPALIGN(pTemp_lcml->pBufferParam,
                                 sizeof(WBAMRENC_ParamStruct),
                                 WBAMRENC_ParamStruct);

        pTemp_lcml->pBufferParam->usNbFrames = 0;
        pTemp_lcml->pBufferParam->pParamElem = NULL;
        pTemp_lcml->pFrameParam = NULL;

        OMX_MALLOC_GENERIC(pTemp_lcml->pDmmBuf, DMM_BUFFER_OBJ);

        pTemp->nFlags = WBAMRENC_NORMAL_BUFFER;
        pTemp++;
        pTemp_lcml++;
    }

#ifdef __PERF_INSTRUMENTATION__
    pComponentPrivate->nLcml_nCntIp = 0;
    pComponentPrivate->nLcml_nCntOpReceived = 0;
#endif


    pComponentPrivate->bInitParamsInitialized = 1;
EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, "Exiting\n");
    OMX_PRINT1(pComponentPrivate->dbg, "Returning = 0x%x\n", eError);
    return eError;
}

/* ========================================================================== */
/**
 * @WBAMRENC_StartComponentThread() This function is called by the component to create
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

OMX_ERRORTYPE WBAMRENC_StartComponentThread(OMX_HANDLETYPE pComponent) {
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    WBAMRENC_COMPONENT_PRIVATE *pComponentPrivate =
        (WBAMRENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
#ifdef UNDER_CE
    pthread_attr_t attr;
    memset(&attr, 0, sizeof(attr));
    attr.__inheritsched = PTHREAD_EXPLICIT_SCHED;
    attr.__schedparam.__sched_priority = OMX_AUDIO_ENCODER_THREAD_PRIORITY;
#endif

    OMX_PRINT1(pComponentPrivate->dbg, "Entering\n");

    /* Initialize all the variables*/
    pComponentPrivate->bIsThreadstop = 0;
    pComponentPrivate->lcml_nOpBuf = 0;
    pComponentPrivate->lcml_nIpBuf = 0;
    pComponentPrivate->app_nBuf = 0;

    /* create the pipe used to send buffers to the thread */
    eError = pipe (pComponentPrivate->cmdDataPipe);

    if (eError) {
        eError = OMX_ErrorInsufficientResources;
        OMX_ERROR4(pComponentPrivate->dbg,
                   "Error while creating cmdDataPipe\n");
        goto EXIT;
    }

    /* create the pipe used to send buffers to the thread */
    eError = pipe (pComponentPrivate->dataPipe);

    if (eError) {
        eError = OMX_ErrorInsufficientResources;
        OMX_ERROR4(pComponentPrivate->dbg, "Error while creating dataPipe\n");
        goto EXIT;
    }

    /* create the pipe used to send commands to the thread */
    eError = pipe (pComponentPrivate->cmdPipe);

    if (eError) {
        eError = OMX_ErrorInsufficientResources;
        OMX_ERROR4(pComponentPrivate->dbg, "Error while creating cmdPipe\n");
        goto EXIT;
    }

    /* Create the Component Thread */
#ifdef UNDER_CE
    eError = pthread_create (&(pComponentPrivate->ComponentThread),
                             &attr,
                             WBAMRENC_CompThread,
                             pComponentPrivate);
#else
    eError = pthread_create (&(pComponentPrivate->ComponentThread),
                             NULL,
                             WBAMRENC_CompThread,
                             pComponentPrivate);
#endif

    if (eError || !pComponentPrivate->ComponentThread) {
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    pComponentPrivate->bCompThreadStarted = 1;
EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, "Exiting\n");
    OMX_PRINT1(pComponentPrivate->dbg, "Returning = 0x%x\n", eError);
    return eError;
}

/* ========================================================================== */
/**
 * @WBAMRENC_FreeCompResources() This function is called by the component during
 * de-init , to newfree Command pipe, data pipe & LCML pipe.
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

OMX_ERRORTYPE WBAMRENC_FreeCompResources(OMX_HANDLETYPE pComponent) {
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_ERRORTYPE err = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    WBAMRENC_COMPONENT_PRIVATE *pComponentPrivate = (WBAMRENC_COMPONENT_PRIVATE *)
            pHandle->pComponentPrivate;

    OMX_PRINT1(pComponentPrivate->dbg, "Entering\n");

    if (pComponentPrivate->bCompThreadStarted) {
        OMX_PRDSP1(pComponentPrivate->dbg, "Closing pipes\n");
        OMX_WBCLOSE_PIPE(pComponentPrivate->dataPipe[0], err);
        OMX_WBCLOSE_PIPE(pComponentPrivate->dataPipe[1], err);
        OMX_WBCLOSE_PIPE(pComponentPrivate->cmdPipe[0], err);
        OMX_WBCLOSE_PIPE(pComponentPrivate->cmdPipe[1], err);
        OMX_WBCLOSE_PIPE(pComponentPrivate->cmdDataPipe[0], err);
        OMX_WBCLOSE_PIPE(pComponentPrivate->cmdDataPipe[1], err);
    }

    OMX_MEMFREE_STRUCT_DSPALIGN(pComponentPrivate->pAlgParam, WBAMRENC_TALGCtrl);

    OMX_MEMFREE_STRUCT_DSPALIGN(pComponentPrivate->pParams, WBAMRENC_AudioCodecParams);

    OMX_PRDSP1(pComponentPrivate->dbg, "Freeing private memory structures\n");
    OMX_MEMFREE_STRUCT(pComponentPrivate->pPortDef[WBAMRENC_INPUT_PORT]);
    OMX_MEMFREE_STRUCT(pComponentPrivate->pPortDef[WBAMRENC_OUTPUT_PORT]);
    OMX_MEMFREE_STRUCT(pComponentPrivate->pcmParams);
    OMX_MEMFREE_STRUCT(pComponentPrivate->amrParams);

    OMX_MEMFREE_STRUCT(pComponentPrivate->pCompPort[WBAMRENC_INPUT_PORT]->pPortFormat);
    OMX_MEMFREE_STRUCT(pComponentPrivate->pCompPort[WBAMRENC_OUTPUT_PORT]->pPortFormat);
    OMX_MEMFREE_STRUCT(pComponentPrivate->pCompPort[WBAMRENC_INPUT_PORT]);
    OMX_MEMFREE_STRUCT(pComponentPrivate->pCompPort[WBAMRENC_OUTPUT_PORT]);

    OMX_MEMFREE_STRUCT(pComponentPrivate->sPortParam);
    OMX_MEMFREE_STRUCT(pComponentPrivate->sPriorityMgmt);
    OMX_MEMFREE_STRUCT(pComponentPrivate->pInputBufferList);
    OMX_MEMFREE_STRUCT(pComponentPrivate->pOutputBufferList);

#ifndef UNDER_CE
    OMX_PRDSP1(pComponentPrivate->dbg, "Destroying mutexes\n");
    pthread_mutex_destroy(&pComponentPrivate->InLoaded_mutex);
    pthread_cond_destroy(&pComponentPrivate->InLoaded_threshold);

    pthread_mutex_destroy(&pComponentPrivate->InIdle_mutex);
    pthread_cond_destroy(&pComponentPrivate->InIdle_threshold);

    pthread_mutex_destroy(&pComponentPrivate->AlloBuf_mutex);
    pthread_cond_destroy(&pComponentPrivate->AlloBuf_threshold);
#else
    OMX_DestroyEvent(&(pComponentPrivate->InLoaded_event));
    OMX_DestroyEvent(&(pComponentPrivate->InIdle_event));
    OMX_DestroyEvent(&(pComponentPrivate->AlloBuf_event));
#endif
EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, "Exiting\n");
    OMX_PRINT1(pComponentPrivate->dbg, "Returning = 0x%x\n", eError);
    return eError;
}

/* ========================================================================== */
/**
 * @WBAMRENC_CleanupInitParams() This function is called by the component during
 * de-init to newfree structues that are been allocated at intialization stage
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

OMX_ERRORTYPE WBAMRENC_CleanupInitParams(OMX_HANDLETYPE pComponent) {
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nIpBuf = 0;
    OMX_U32 nOpBuf = 0;
    OMX_U32 i = 0;
    WBAMRENC_LCML_BUFHEADERTYPE *pTemp_lcml;

    LCML_DSP_INTERFACE *pLcmlHandle;
    LCML_DSP_INTERFACE *pLcmlHandleAux;

    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    WBAMRENC_COMPONENT_PRIVATE *pComponentPrivate = (WBAMRENC_COMPONENT_PRIVATE *)
            pHandle->pComponentPrivate;
    OMX_PRINT1(pComponentPrivate->dbg, "Entering\n");

    if (pComponentPrivate->dasfMode == 1) {
        OMX_MEMFREE_STRUCT(pComponentPrivate->strmAttr);
    }

    OMX_MEMFREE_STRUCT_DSPALIGN(pComponentPrivate->pAlgParam, WBAMRENC_TALGCtrl);

    OMX_MEMFREE_STRUCT_DSPALIGN(pComponentPrivate->pAlgParamDTX, WBAMRENC_TALGCtrlDTX);

    if (pComponentPrivate->nMultiFrameMode == 1) {
        OMX_MEMFREE_STRUCT(pComponentPrivate->pHoldBuffer);
        OMX_MEMFREE_STRUCT(pComponentPrivate->iHoldBuffer);
        OMX_MEMFREE_STRUCT(pComponentPrivate->iMMFDataLastBuffer);
    }

    pTemp_lcml = pComponentPrivate->pLcmlBufHeader[WBAMRENC_INPUT_PORT];
    nIpBuf = pComponentPrivate->nRuntimeInputBuffers;

    for (i = 0; i < nIpBuf; i++) {
        if (pTemp_lcml->pFrameParam != NULL) {

            pLcmlHandle = (LCML_DSP_INTERFACE *)pComponentPrivate->pLcmlHandle;
            pLcmlHandleAux = (LCML_DSP_INTERFACE *)(((LCML_CODEC_INTERFACE *)pLcmlHandle->pCodecinterfacehandle)->pCodec);
            OMX_DmmUnMap(pLcmlHandleAux->dspCodec->hProc,
                         (void*)pTemp_lcml->pBufferParam->pParamElem,
                         pTemp_lcml->pDmmBuf->pReserved, pComponentPrivate->dbg);

            OMX_MEMFREE_STRUCT_DSPALIGN(pTemp_lcml->pFrameParam, OMX_U8);
        }

        OMX_MEMFREE_STRUCT_DSPALIGN(pTemp_lcml->pBufferParam, WBAMRENC_ParamStruct);


        if (pTemp_lcml->pDmmBuf != NULL) {
            OMX_MEMFREE_STRUCT(pTemp_lcml->pDmmBuf);
            pTemp_lcml->pDmmBuf = NULL;
        }

        pTemp_lcml++;
    }

    pTemp_lcml = pComponentPrivate->pLcmlBufHeader[WBAMRENC_OUTPUT_PORT];
    nOpBuf = pComponentPrivate->nRuntimeOutputBuffers;

    for (i = 0; i < nOpBuf; i++) {

        if (pTemp_lcml->pFrameParam != NULL) {
            pLcmlHandle = (LCML_DSP_INTERFACE *)pComponentPrivate->pLcmlHandle;
            pLcmlHandleAux = (LCML_DSP_INTERFACE *)(((LCML_CODEC_INTERFACE *)pLcmlHandle->pCodecinterfacehandle)->pCodec);
#ifndef UNDER_CE
            OMX_DmmUnMap(pLcmlHandleAux->dspCodec->hProc,
                         (void*)pTemp_lcml->pBufferParam->pParamElem,
                         pTemp_lcml->pDmmBuf->pReserved, pComponentPrivate->dbg);
#endif


            OMX_MEMFREE_STRUCT_DSPALIGN(pTemp_lcml->pFrameParam, OMX_U8);
        }

        OMX_MEMFREE_STRUCT_DSPALIGN(pTemp_lcml->pBufferParam, WBAMRENC_ParamStruct);

        if (pTemp_lcml->pDmmBuf != NULL) {
            OMX_MEMFREE_STRUCT(pTemp_lcml->pDmmBuf);
            pTemp_lcml->pDmmBuf = NULL;
        }

        pTemp_lcml++;
    }

    OMX_MEMFREE_STRUCT(pComponentPrivate->pLcmlBufHeader[WBAMRENC_INPUT_PORT]);
    OMX_MEMFREE_STRUCT(pComponentPrivate->pLcmlBufHeader[WBAMRENC_OUTPUT_PORT]);

    OMX_PRINT1(pComponentPrivate->dbg, "Exiting\n");
    OMX_PRINT1(pComponentPrivate->dbg, "Returning = 0x%x\n", eError);
    return eError;
}

/* ========================================================================== */
/**
 * @WBAMRENC_StopComponentThread() This function is called by the component during
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

OMX_ERRORTYPE WBAMRENC_StopComponentThread(OMX_HANDLETYPE pComponent) {
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_ERRORTYPE threadError = OMX_ErrorNone;
    int pthreadError = 0;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    WBAMRENC_COMPONENT_PRIVATE *pComponentPrivate = (WBAMRENC_COMPONENT_PRIVATE *)
            pHandle->pComponentPrivate;
    OMX_PRINT1(pComponentPrivate->dbg, "Entering\n");
    pComponentPrivate->bIsThreadstop = 1;
    OMX_PRINT2(pComponentPrivate->dbg, "About to call pthread_join\n");
    write (pComponentPrivate->cmdPipe[1], &pComponentPrivate->bIsThreadstop, sizeof(OMX_U16));
    pthreadError = pthread_join (pComponentPrivate->ComponentThread,
                                 (void*) & threadError);

    if (0 != pthreadError) {
        eError = OMX_ErrorHardware;
        OMX_ERROR4(pComponentPrivate->dbg,
                   "Error closing ComponentThread - pthreadError = %d\n", pthreadError);
        goto EXIT;
    }

    if (OMX_ErrorNone != threadError && OMX_ErrorNone != eError) {
        eError = OMX_ErrorInsufficientResources;
        OMX_ERROR4(pComponentPrivate->dbg,
                   "Error while closing Component Thread\n");
        goto EXIT;
    }

EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, "Exiting\n");
    OMX_PRINT1(pComponentPrivate->dbg, "Returning = 0x%x\n", eError);
    return eError;
}


/* ========================================================================== */
/**
 * @WBAMRENC_HandleCommand() This function is called by the component when ever it
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

OMX_U32 WBAMRENC_HandleCommand (WBAMRENC_COMPONENT_PRIVATE *pComponentPrivate,
                                OMX_COMMANDTYPE cmd,
                                OMX_U32 cmdData) {
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMMANDTYPE command = cmd;
    OMX_STATETYPE commandedState;
    OMX_HANDLETYPE pLcmlHandle;
#ifdef RESOURCE_MANAGER_ENABLED
    OMX_ERRORTYPE rm_error;
#endif
    LCML_CALLBACKTYPE cb;
    LCML_DSP *pLcmlDsp;
    OMX_U32 cmdValues[4];
    OMX_U32 pValues[4];
    OMX_U32 commandData = cmdData;
    OMX_U16 arr[100];
    char *pArgs = "damedesuStr";
    OMX_U32 i = 0;
    OMX_U8 inputPortFlag = 0, outputPortFlag = 0;
    WBAMRENC_LCML_BUFHEADERTYPE *pLcmlHdr = NULL;

    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;
    pLcmlHandle = pComponentPrivate->pLcmlHandle;

    OMX_PRINT1(pComponentPrivate->dbg, "Entering \n");
    OMX_PRINT1(pComponentPrivate->dbg, "curState = %d\n",
               pComponentPrivate->curState);

#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedCommand(pComponentPrivate->pPERFcomp,
                         command,
                         commandData,
                         PERF_ModuleLLMM);
#endif

    if (command == OMX_CommandStateSet) {
        commandedState = (OMX_STATETYPE)commandData;

        if (pComponentPrivate->curState == commandedState) {
            pComponentPrivate->cbInfo.EventHandler ( pHandle,
                    pHandle->pApplicationPrivate,
                    OMX_EventError,
                    OMX_ErrorSameState,
                    OMX_TI_ErrorMinor,
                    NULL);
            OMX_ERROR4(pComponentPrivate->dbg,
                       "OMX_ErrorSameState Given by Comp\n");
        } else {
            switch (commandedState) {
                case OMX_StateIdle:
                    OMX_PRSTATE2(pComponentPrivate->dbg, "To OMX_StateIdle \n");
                    OMX_PRSTATE2(pComponentPrivate->dbg,
                                 "curState = %d\n",
                                 pComponentPrivate->curState);

                    if (pComponentPrivate->curState == OMX_StateLoaded) {
                        OMX_PRSTATE2(pComponentPrivate->dbg,
                                     "OMX_StateLoaded -> OMX_StateIdle \n");
#ifdef __PERF_INSTRUMENTATION__
                        PERF_Boundary(pComponentPrivate->pPERFcomp, PERF_BoundaryStart | PERF_BoundarySetup);
#endif

                        if (pComponentPrivate->dasfMode == 1) {
                            pComponentPrivate->pPortDef[WBAMRENC_INPUT_PORT]->bEnabled = FALSE;
                            pComponentPrivate->pPortDef[WBAMRENC_INPUT_PORT]->bPopulated = FALSE;

                            if (pComponentPrivate->streamID == 0) {
                                OMX_ERROR4(pComponentPrivate->dbg, "**************************************\n");
                                OMX_ERROR4(pComponentPrivate->dbg, "Error = OMX_ErrorInsufficientResources\n");
                                OMX_ERROR4(pComponentPrivate->dbg, "**************************************\n");
                                eError = OMX_ErrorInsufficientResources;
                                pComponentPrivate->curState = OMX_StateInvalid;
                                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                                       pHandle->pApplicationPrivate,
                                                                       OMX_EventError,
                                                                       OMX_ErrorInvalidState,
                                                                       OMX_TI_ErrorMajor,
                                                                       "No Stream ID Available");
                                goto EXIT;
                            }
                        }

                        if (pComponentPrivate->pPortDef[WBAMRENC_INPUT_PORT]->bPopulated &&
                                pComponentPrivate->pPortDef[WBAMRENC_INPUT_PORT]->bEnabled)  {
                            inputPortFlag = 1;
                        }

                        if (pComponentPrivate->pPortDef[WBAMRENC_OUTPUT_PORT]->bPopulated &&
                                pComponentPrivate->pPortDef[WBAMRENC_OUTPUT_PORT]->bEnabled) {
                            outputPortFlag = 1;
                        }

                        if (!pComponentPrivate->pPortDef[WBAMRENC_INPUT_PORT]->bPopulated &&
                                !pComponentPrivate->pPortDef[WBAMRENC_INPUT_PORT]->bEnabled)  {
                            inputPortFlag = 1;
                        }

                        if (!pComponentPrivate->pPortDef[WBAMRENC_OUTPUT_PORT]->bPopulated &&
                                !pComponentPrivate->pPortDef[WBAMRENC_OUTPUT_PORT]->bEnabled) {
                            outputPortFlag = 1;
                        }

                        if (!(inputPortFlag && outputPortFlag)) {
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

                        cb.LCML_Callback = (void *) WBAMRENC_LCMLCallback;
                        pLcmlHandle = (OMX_HANDLETYPE) WBAMRENC_GetLCMLHandle(pComponentPrivate);

                        if (pLcmlHandle == NULL) {
                            OMX_ERROR4(pComponentPrivate->dbg,
                                       "LCML Handle is NULL........exiting..\n");
                            goto EXIT;
                        }

                        /* Got handle of dsp via phandle filling information about DSP Specific things */
                        pLcmlDsp = (((LCML_DSP_INTERFACE*)pLcmlHandle)->dspCodec);
                        eError = WBAMRENC_FillLCMLInitParams(pHandle, pLcmlDsp, arr);

                        if (eError != OMX_ErrorNone) {
                            OMX_ERROR4(pComponentPrivate->dbg,
                                       "Error from WBAMRENCFill_LCMLInitParams()\n");
                            goto EXIT;
                        }

                        pComponentPrivate->pLcmlHandle = (LCML_DSP_INTERFACE *)pLcmlHandle;
                        cb.LCML_Callback = (void *) WBAMRENC_LCMLCallback;

#ifndef UNDER_CE

                        OMX_PRDSP2(pComponentPrivate->dbg,
                                   "Calling LCML_InitMMCodecEx...\n");

                        eError = LCML_InitMMCodecEx(((LCML_DSP_INTERFACE *)pLcmlHandle)->pCodecinterfacehandle,
                                                    pArgs,
                                                    &pLcmlHandle,
                                                    (void *)pArgs,
                                                    &cb,
                                                    (OMX_STRING)pComponentPrivate->sDeviceString);
#else
                        eError = LCML_InitMMCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                  pArgs, &pLcmlHandle,
                                                  (void *)pArgs,
                                                  &cb);
#endif

                        if (eError != OMX_ErrorNone) {
                            OMX_ERROR4(pComponentPrivate->dbg,
                                       "Error returned from LCML_InitMMCodecEx\n");
                            /* send an event to client */
                            /* client should unload the component if the codec is not able to load */
                        eError = OMX_ErrorInvalidState;
                            pComponentPrivate->cbInfo.EventHandler (pHandle,
                                                                    pHandle->pApplicationPrivate,
                                                                    OMX_EventError,
                                                                    eError,
                                                                    OMX_TI_ErrorSevere,
                                                                    NULL);
                            goto EXIT;
                        }

#ifdef RESOURCE_MANAGER_ENABLED
                        /* Need check the resource with RM */

                        pComponentPrivate->rmproxyCallback.RMPROXY_Callback = (void *) WBAMRENC_ResourceManagerCallback;

                        if (pComponentPrivate->curState != OMX_StateWaitForResources) {

                            rm_error = RMProxy_NewSendCommand(pHandle,
                                                              RMProxy_RequestResource,
                                                              OMX_WBAMR_Encoder_COMPONENT,
                                                              WBAMRENC_CPU_LOAD,
                                                              3456,
                                                              &(pComponentPrivate->rmproxyCallback));

                            if (rm_error == OMX_ErrorNone) {
                                /* resource is available */
#ifdef __PERF_INSTRUMENTATION__
                                PERF_Boundary(pComponentPrivate->pPERFcomp, PERF_BoundaryComplete | PERF_BoundarySetup);
#endif
                                pComponentPrivate->curState = OMX_StateIdle;

                            /* Decrement reference count with signal enabled */
                            if(RemoveStateTransition(pComponentPrivate, OMX_TRUE) != OMX_ErrorNone) {
                                return OMX_ErrorUndefined;
                            }

                                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                                        pHandle->pApplicationPrivate,
                                                                        OMX_EventCmdComplete,
                                                                        OMX_CommandStateSet,
                                                                        pComponentPrivate->curState,
                                                                        NULL);

                                rm_error = RMProxy_NewSendCommand(pHandle,
                                                                  RMProxy_StateSet,
                                                                  OMX_WBAMR_Encoder_COMPONENT,
                                                                  OMX_StateIdle,
                                                                  3456,
                                                                  NULL);
                            } else if (rm_error == OMX_ErrorInsufficientResources) {
                                /* resource is not available, need set state to OMX_StateWaitForResources */
                                pComponentPrivate->curState = OMX_StateWaitForResources;
                                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                                        pHandle->pApplicationPrivate,
                                                                        OMX_EventCmdComplete,
                                                                        OMX_CommandStateSet,
                                                                        pComponentPrivate->curState,
                                                                        NULL);
                                OMX_ERROR4(pComponentPrivate->dbg,
                                           "Comp: OMX_ErrorInsufficientResources\n");
                            }
                        } else {

                        pComponentPrivate->curState = OMX_StateIdle;
                        /* Decrement reference count with signal enabled */
                        if(RemoveStateTransition(pComponentPrivate, OMX_TRUE) != OMX_ErrorNone) {
                            return OMX_ErrorUndefined;
                        }

                            pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                                    pHandle->pApplicationPrivate,
                                                                    OMX_EventCmdComplete,
                                                                    OMX_CommandStateSet,
                                                                    pComponentPrivate->curState,
                                                                    NULL);
                            rm_error = RMProxy_NewSendCommand(pHandle,
                                                              RMProxy_StateSet,
                                                              OMX_WBAMR_Encoder_COMPONENT,
                                                              OMX_StateIdle,
                                                              3456,
                                                              NULL);

                        }

#else
                        pComponentPrivate->curState = OMX_StateIdle;

                    /* Decrement reference count with signal enabled */
                    if(RemoveStateTransition(pComponentPrivate, OMX_TRUE) != OMX_ErrorNone) {
                         return OMX_ErrorUndefined;
                    }

                        pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                                pHandle->pApplicationPrivate,
                                                                OMX_EventCmdComplete,
                                                                OMX_CommandStateSet,
                                                                pComponentPrivate->curState,
                                                                NULL);
#endif


#ifdef __PERF_INSTRUMENTATION__
                        PERF_Boundary(pComponentPrivate->pPERFcomp,
                                      PERF_BoundaryComplete | PERF_BoundarySetup);
#endif


                    } else if (pComponentPrivate->curState == OMX_StateExecuting) {
                        OMX_PRSTATE2(pComponentPrivate->dbg,
                                     "OMX_StateExecuting -> OMX_StateIdle \n");
                        OMX_PRDSP1(pComponentPrivate->dbg, "Stop codec\n");
                        OMX_PRINT2(pComponentPrivate->dbg,
                                   "Codec - MMCodecControlStop\n");
#ifdef __PERF_INSTRUMENTATION__
                        PERF_Boundary(pComponentPrivate->pPERFcomp,
                                      PERF_BoundaryComplete | PERF_BoundarySteadyState);
#endif
                        eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                   MMCodecControlStop,
                                                   (void *)pArgs);

                        OMX_MEMFREE_STRUCT_DSPALIGN(pComponentPrivate->pAlgParam, WBAMRENC_TALGCtrl);
                        OMX_MEMFREE_STRUCT_DSPALIGN(pComponentPrivate->pAlgParamDTX, WBAMRENC_TALGCtrlDTX);

                        if (pComponentPrivate->pHoldBuffer) {
                            OMX_MEMFREE_STRUCT(pComponentPrivate->pHoldBuffer);
                            pComponentPrivate->pHoldBuffer = NULL;
                        }

                        pComponentPrivate->nOutStandingFillDones = 0;
                        pComponentPrivate->nOutStandingEmptyDones = 0;
                        pComponentPrivate->nHoldLength = 0;
                        pComponentPrivate->InBuf_Eos_alreadysent = 0;
                        OMX_MEMFREE_STRUCT_DSPALIGN(pComponentPrivate->pParams, WBAMRENC_AudioCodecParams);

                        if (eError != OMX_ErrorNone) {
                            OMX_ERROR4(pComponentPrivate->dbg,
                                       "Error from LCML_ControlCodec MMCodecControlStop..\n");
                            pComponentPrivate->curState = OMX_StateInvalid;
                            pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                                    pHandle->pApplicationPrivate,
                                                                    OMX_EventError,
                                                                    eError,
                                                                    OMX_TI_ErrorSevere,
                                                                    NULL);
                            goto EXIT;
                        }

                    } else if (pComponentPrivate->curState == OMX_StatePause) {
                        OMX_PRSTATE2(pComponentPrivate->dbg,
                                     "OMX_StatePause -> OMX_StateIdle \n");

                        pComponentPrivate->curState = OMX_StateIdle;

                    /* Decrement reference count with signal enabled */
                    if(RemoveStateTransition(pComponentPrivate, OMX_TRUE) != OMX_ErrorNone) {
                        return OMX_ErrorUndefined;
                    }

#ifdef __PERF_INSTRUMENTATION__
                        PERF_Boundary(pComponentPrivate->pPERFcomp,
                                      PERF_BoundaryComplete | PERF_BoundarySteadyState);
#endif
#ifdef RESOURCE_MANAGER_ENABLED
                        rm_error = RMProxy_NewSendCommand(pHandle,
                                                          RMProxy_StateSet,
                                                          OMX_WBAMR_Encoder_COMPONENT,
                                                          OMX_StateIdle,
                                                          3456,
                                                          NULL);
#endif
                        OMX_PRINT1(pComponentPrivate->dbg,
                                   "The component is stopped\n");
                        pComponentPrivate->cbInfo.EventHandler ( pHandle,
                                pHandle->pApplicationPrivate,
                                OMX_EventCmdComplete,
                                OMX_CommandStateSet,
                                pComponentPrivate->curState,
                                NULL);
                    } else {    /* This means, it is invalid state from application */
                        pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                                pHandle->pApplicationPrivate,
                                                                OMX_EventError,
                                                                OMX_ErrorIncorrectStateTransition,
                                                                OMX_TI_ErrorMinor,
                                                                "Invalid State");
                        OMX_ERROR4(pComponentPrivate->dbg,
                                   "OMX_ErrorIncorrectStateTransition\n");
                    }

                    break;

                case OMX_StateExecuting:
                    OMX_PRSTATE2(pComponentPrivate->dbg, "To OMX_StateExecuting\n");

                    if (pComponentPrivate->curState == OMX_StateIdle) {
                        /* Sending commands to DSP via LCML_ControlCodec third argument
                           is not used for time being */
                        OMX_PRSTATE2(pComponentPrivate->dbg,
                                     "OMX_StateIdle -> OMX_StateExecuting\n");

                        if ( pComponentPrivate->pAlgParam == NULL) {
                            OMX_MALLOC_SIZE_DSPALIGN(pComponentPrivate->pAlgParam,
                                                     sizeof(WBAMRENC_TALGCtrl),
                                                     OMX_U8);
                            OMX_PRBUFFER2(pComponentPrivate->dbg,
                                          "pAlgParam %p\n", pComponentPrivate->pAlgParam);
                        }

                        if ( pComponentPrivate->pAlgParamDTX == NULL) {
                            OMX_MALLOC_SIZE_DSPALIGN(pComponentPrivate->pAlgParamDTX,
                                                     sizeof(WBAMRENC_TALGCtrl),
                                                     OMX_U8);
                            OMX_PRBUFFER2(pComponentPrivate->dbg,
                                          "pAlgParamDTX %p\n", pComponentPrivate->pAlgParamDTX);
                        }

                        pComponentPrivate->nNumInputBufPending = 0;
                        pComponentPrivate->nNumOutputBufPending = 0;

                        pComponentPrivate->nNumOfFramesSent = 0;

                        pComponentPrivate->nEmptyBufferDoneCount = 0;
                        pComponentPrivate->nEmptyThisBufferCount = 0;

                        pComponentPrivate->pAlgParam->iBitrate = pComponentPrivate->amrParams->eAMRBandMode;
                        pComponentPrivate->pAlgParamDTX->iVADFlag = pComponentPrivate->amrParams->eAMRDTXMode;
                        pComponentPrivate->pAlgParam->iSize = sizeof (WBAMRENC_TALGCtrl);
                        pComponentPrivate->pAlgParamDTX->iSize = sizeof (WBAMRENC_TALGCtrl);

                        OMX_PRINT2(pComponentPrivate->dbg,
                                   "pAlgParam->iBitrate = %d\n",
                                   pComponentPrivate->pAlgParam->iBitrate);
                        OMX_PRINT2(pComponentPrivate->dbg,
                                   "pAlgParam->iDTX  = %d\n",
                                   pComponentPrivate->pAlgParamDTX->iVADFlag);

                        cmdValues[0] = ALGCMD_BITRATE;                  /*setting the bit-rate*/
                        cmdValues[1] = (OMX_U32)pComponentPrivate->pAlgParam;
                        cmdValues[2] = sizeof (WBAMRENC_TALGCtrl);

                        OMX_PRINT2(pComponentPrivate->dbg,
                                   "Codec - EMMCodecControlAlgCtrl\n");
                        /* Sending ALGCTRL MESSAGE DTX to DSP via LCML_ControlCodec*/
                        eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                   EMMCodecControlAlgCtrl,
                                                   (void *)cmdValues);

                        if (eError != OMX_ErrorNone) {
                            OMX_ERROR4(pComponentPrivate->dbg,
                                       "Error from LCML_ControlCodec EMMCodecControlAlgCtrl = %x\n", eError);
                            goto EXIT;
                        }

                        cmdValues[0] = ALGCMD_DTX; /*setting DTX mode*/
                        cmdValues[1] = (OMX_U32)pComponentPrivate->pAlgParamDTX;
                        cmdValues[2] = sizeof (WBAMRENC_TALGCtrlDTX);

                        OMX_PRINT2(pComponentPrivate->dbg,
                                   "Codec - EMMCodecControlAlgCtrl\n");
                        /* Sending ALGCTRL MESSAGE BITRATE to DSP via LCML_ControlCodec*/
                        eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                   EMMCodecControlAlgCtrl,
                                                   (void *)cmdValues);

                        if (eError != OMX_ErrorNone) {
                            OMX_ERROR4(pComponentPrivate->dbg,
                                       "Error from LCML_ControlCodec EMMCodecControlAlgCtrl = %x\n", eError);
                            goto EXIT;
                        }

                        if (pComponentPrivate->dasfMode == 1) {
                            OMX_PRDSP2(pComponentPrivate->dbg,
                                       "---- Comp: DASF Functionality is ON ---\n");

                            OMX_MALLOC_SIZE_DSPALIGN(pComponentPrivate->pParams,
                                                     sizeof(WBAMRENC_AudioCodecParams),
                                                     OMX_U8);

                            OMX_PRBUFFER2(pComponentPrivate->dbg,
                                          "pParams %p\n", pComponentPrivate->pParams);

                            pComponentPrivate->pParams->iAudioFormat = 1;
                            pComponentPrivate->pParams->iStrmId = pComponentPrivate->streamID;
                            pComponentPrivate->pParams->iSamplingRate = WBAMRENC_SAMPLING_FREQUENCY;
                            pValues[0] = USN_STRMCMD_SETCODECPARAMS;
                            pValues[1] = (OMX_U32)pComponentPrivate->pParams;
                            pValues[2] = sizeof(WBAMRENC_AudioCodecParams);
                            /* Sending STRMCTRL MESSAGE to DSP via LCML_ControlCodec*/
                            eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                       EMMCodecControlStrmCtrl,
                                                       (void *)pValues);

                            if (eError != OMX_ErrorNone) {
                                OMX_ERROR4(pComponentPrivate->dbg,
                                           "Error from LCML_ControlCodec EMMCodecControlStrmCtrl = %x\n", eError);
                                goto EXIT;
                            }
                        }

                        /* Sending START MESSAGE to DSP via LCML_ControlCodec*/
                        eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                   EMMCodecControlStart,
                                                   (void *)pArgs);

                        if (eError != OMX_ErrorNone) {
                            OMX_ERROR4(pComponentPrivate->dbg,
                                       "Error from LCML_ControlCodec EMMCodecControlStart = %x\n", eError);
                            goto EXIT;
                        }

                    } else if (pComponentPrivate->curState == OMX_StatePause) {
                        OMX_PRSTATE2(pComponentPrivate->dbg,
                                     "OMX_StatePause -> OMX_StateExecuting\n");

                        eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                   EMMCodecControlStart,
                                                   (void *)pArgs);

                        if (eError != OMX_ErrorNone) {
                            OMX_ERROR4(pComponentPrivate->dbg,
                                       "Error While Resuming the codec = %x\n", eError);
                            goto EXIT;
                        }

                        for (i = 0; i < pComponentPrivate->nNumInputBufPending; i++) {
                            if (pComponentPrivate->pInputBufHdrPending[i]) {
                                WBAMRENC_GetCorrespondingLCMLHeader(pComponentPrivate, pComponentPrivate->pInputBufHdrPending[i]->pBuffer, OMX_DirInput, &pLcmlHdr);
                                WBAMRENC_SetPending(pComponentPrivate, pComponentPrivate->pInputBufHdrPending[i], OMX_DirInput, __LINE__);

                                eError = LCML_QueueBuffer(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                          EMMCodecInputBuffer,
                                                          pComponentPrivate->pInputBufHdrPending[i]->pBuffer,
                                                          pComponentPrivate->pInputBufHdrPending[i]->nAllocLen,
                                                          pComponentPrivate->pInputBufHdrPending[i]->nFilledLen,
                                                          (OMX_U8 *) pLcmlHdr->pBufferParam,
                                                          sizeof(WBAMRENC_ParamStruct),
                                                          NULL);
                            }
                        }

                        pComponentPrivate->nNumInputBufPending = 0;

                        for (i = 0; i < pComponentPrivate->nNumOutputBufPending; i++) {
                            if (pComponentPrivate->pOutputBufHdrPending[i]) {
                                WBAMRENC_GetCorrespondingLCMLHeader(pComponentPrivate, pComponentPrivate->pOutputBufHdrPending[i]->pBuffer, OMX_DirOutput, &pLcmlHdr);
                                WBAMRENC_SetPending(pComponentPrivate, pComponentPrivate->pOutputBufHdrPending[i], OMX_DirOutput, __LINE__);
                                eError = LCML_QueueBuffer(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                          EMMCodecOuputBuffer,
                                                          pComponentPrivate->pOutputBufHdrPending[i]->pBuffer,
                                                          pComponentPrivate->pOutputBufHdrPending[i]->nAllocLen,
                                                          pComponentPrivate->pOutputBufHdrPending[i]->nFilledLen,
                                                          (OMX_U8 *) pLcmlHdr->pBufferParam,
                                                          sizeof(WBAMRENC_ParamStruct),
                                                          NULL);
                            }
                        }

                        pComponentPrivate->nNumOutputBufPending = 0;
                    } else {
                        pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                                pHandle->pApplicationPrivate,
                                                                OMX_EventError,
                                                                OMX_ErrorIncorrectStateTransition,
                                                                OMX_TI_ErrorMinor,
                                                                "Incorrect State Transition");
                        OMX_ERROR4(pComponentPrivate->dbg,
                                   "OMX_ErrorIncorrectStateTransition Given by Comp\n");
                        goto EXIT;

                    }

#ifdef RESOURCE_MANAGER_ENABLED
                    rm_error = RMProxy_NewSendCommand(pHandle, RMProxy_StateSet, OMX_WBAMR_Encoder_COMPONENT, OMX_StateExecuting, 3456, NULL);
#endif
                    pComponentPrivate->curState = OMX_StateExecuting;
#ifdef __PERF_INSTRUMENTATION__
                    PERF_Boundary(pComponentPrivate->pPERFcomp, PERF_BoundaryStart | PERF_BoundarySteadyState);
#endif

                /* Decrement reference count with signal enabled */
                if(RemoveStateTransition(pComponentPrivate, OMX_TRUE) != OMX_ErrorNone) {
                      return OMX_ErrorUndefined;
                }

                    pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                            pHandle->pApplicationPrivate,
                                                            OMX_EventCmdComplete,
                                                            OMX_CommandStateSet,
                                                            pComponentPrivate->curState,
                                                            NULL);
                    OMX_PRINT2(pComponentPrivate->dbg, "OMX_CommandStateSet Given by Comp\n");
                    break;

                case OMX_StateLoaded:
                    OMX_PRDSP2(pComponentPrivate->dbg, "To OMX_StateLoaded\n");

                    if (pComponentPrivate->curState == OMX_StateWaitForResources) {
                        OMX_PRSTATE2(pComponentPrivate->dbg,
                                     "OMX_StateWaitForResources -> OMX_StateLoaded\n");
                        OMX_PRMGR2(pComponentPrivate->dbg,
                                   "OMX_StateWaitForResources\n");
#ifdef __PERF_INSTRUMENTATION__
                        PERF_Boundary(pComponentPrivate->pPERFcomp,
                                      PERF_BoundaryStart | PERF_BoundaryCleanup);
#endif
                        pComponentPrivate->curState = OMX_StateLoaded;
#ifdef __PERF_INSTRUMENTATION__
                        PERF_Boundary(pComponentPrivate->pPERFcomp, PERF_BoundaryComplete | PERF_BoundaryCleanup);
#endif

                   /* Decrement reference count with signal enabled */
                   if(RemoveStateTransition(pComponentPrivate, OMX_TRUE) != OMX_ErrorNone) {
                       return OMX_ErrorUndefined;
                   }

                        pComponentPrivate->cbInfo.EventHandler ( pHandle,
                                pHandle->pApplicationPrivate,
                                OMX_EventCmdComplete,
                                OMX_CommandStateSet,
                                pComponentPrivate->curState,
                                NULL);
                        OMX_PRINT2(pComponentPrivate->dbg,
                                   "OMX_CommandStateSet Given by Comp\n");
                        break;
                    }

                    if (pComponentPrivate->curState != OMX_StateIdle) {
                        OMX_PRSTATE2(pComponentPrivate->dbg,
                                     "Not OMX_StateIdle -> OMX_StateLoaded\n");

                        pComponentPrivate->cbInfo.EventHandler ( pHandle,
                                pHandle->pApplicationPrivate,
                                OMX_EventError,
                                OMX_ErrorIncorrectStateTransition,
                                OMX_TI_ErrorMinor,
                                "Incorrect State Transition");
                        OMX_ERROR4(pComponentPrivate->dbg,
                                   "Error: OMX_ErrorIncorrectStateTransition Given by Comp\n");
                        goto EXIT;
                    }

#ifdef __PERF_INSTRUMENTATION__
                    PERF_Boundary(pComponentPrivate->pPERFcomp, PERF_BoundaryStart | PERF_BoundaryCleanup);
#endif
                    OMX_PRBUFFER2(pComponentPrivate->dbg,
                                  "Evaluating if all buffers are free\n");

                    if (pComponentPrivate->pInputBufferList->numBuffers ||
                            pComponentPrivate->pOutputBufferList->numBuffers) {
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
                    WBAMRENC_CleanupInitParams(pHandle);
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               EMMCodecControlDestroy,
                                               (void *)pArgs);

                    if (eError != OMX_ErrorNone) {
                        OMX_ERROR4(pComponentPrivate->dbg,
                                   "Error: LCML_ControlCodec EMMCodecControlDestroy = %x\n", eError);
                        goto EXIT;
                    }

                    /*Closing LCML Lib*/
                    if (pComponentPrivate->ptrLibLCML != NULL) {
                        OMX_PRDSP2(pComponentPrivate->dbg, "Closing LCML library\n");
                        dlclose( pComponentPrivate->ptrLibLCML);
                        pComponentPrivate->ptrLibLCML = NULL;
                    }

#ifdef __PERF_INSTRUMENTATION__
                    PERF_SendingCommand(pComponentPrivate->pPERF, -1, 0, PERF_ModuleComponent);
#endif
                    eError = WBAMRENC_EXIT_COMPONENT_THRD;
                    pComponentPrivate->bInitParamsInitialized = 0;
                    pComponentPrivate->bLoadedCommandPending = OMX_FALSE;
                    break;

                case OMX_StatePause:
                    OMX_PRSTATE2(pComponentPrivate->dbg, "To OMX_StatePause\n");

                    if (pComponentPrivate->curState != OMX_StateExecuting &&
                            pComponentPrivate->curState != OMX_StateIdle) {
                        OMX_PRSTATE2(pComponentPrivate->dbg,
                                     "Not OMX_StateExecuting not OMX_StateIdle -> OMX_StatePause\n");
                        pComponentPrivate->cbInfo.EventHandler ( pHandle,
                                pHandle->pApplicationPrivate,
                                OMX_EventError,
                                OMX_ErrorIncorrectStateTransition,
                                OMX_TI_ErrorMinor,
                                "Incorrect State Transition");
                        OMX_ERROR4(pComponentPrivate->dbg,
                                   "Error: OMX_ErrorIncorrectStateTransition Given by Comp\n");
                        goto EXIT;
                    }

#ifdef __PERF_INSTRUMENTATION__
                    PERF_Boundary(pComponentPrivate->pPERFcomp, PERF_BoundaryComplete | PERF_BoundarySteadyState);
#endif
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               EMMCodecControlPause,
                                               (void *)pArgs);

                    if (eError != OMX_ErrorNone) {
                        OMX_ERROR4(pComponentPrivate->dbg,
                                   "Error: LCML_ControlCodec EMMCodecControlPause = %x\n", eError);
                        goto EXIT;
                    }

                    OMX_PRSTATE2(pComponentPrivate->dbg,
                                 "OMX_CommandStateSet Given by Comp\n");
                    break;

                case OMX_StateWaitForResources:
                    OMX_PRSTATE2(pComponentPrivate->dbg, "To OMX_StateWaitForResources\n");

                    if (pComponentPrivate->curState == OMX_StateLoaded) {
                        OMX_PRSTATE2(pComponentPrivate->dbg,
                                     "OMX_StateLoaded -> OMX_StateWaitForResources\n");

#ifdef RESOURCE_MANAGER_ENABLED
                        rm_error = RMProxy_NewSendCommand(pHandle,
                                                          RMProxy_StateSet,
                                                          OMX_WBAMR_Encoder_COMPONENT,
                                                          OMX_StateWaitForResources,
                                                          3456,
                                                          NULL);
#endif

                        pComponentPrivate->curState = OMX_StateWaitForResources;
                        pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                                pHandle->pApplicationPrivate,
                                                                OMX_EventCmdComplete,
                                                                OMX_CommandStateSet,
                                                                pComponentPrivate->curState,
                                                                NULL);
                        OMX_PRINT2(pComponentPrivate->dbg,
                                   "OMX_CommandStateSet Given by Comp\n");
                    } else {
                        pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                                pHandle->pApplicationPrivate,
                                                                OMX_EventError,
                                                                OMX_ErrorIncorrectStateTransition,
                                                                OMX_TI_ErrorMinor,
                                                                "Incorrect State Transition");
                        OMX_ERROR4(pComponentPrivate->dbg,
                                   "Error: OMX_ErrorIncorrectStateTransition Given by Comp\n");
                    }

                    break;

                case OMX_StateInvalid:
                    OMX_PRSTATE2(pComponentPrivate->dbg, "To OMX_StateInvalid\n");

                    if (pComponentPrivate->curState != OMX_StateWaitForResources &&
                            pComponentPrivate->curState != OMX_StateInvalid &&
                            pComponentPrivate->curState != OMX_StateLoaded) {

                        eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                   EMMCodecControlDestroy,
                                                   (void *)pArgs);
                    }

                    pComponentPrivate->curState = OMX_StateInvalid;
                    pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                            pHandle->pApplicationPrivate,
                                                            OMX_EventError,
                                                            OMX_ErrorInvalidState,
                                                            OMX_TI_ErrorSevere,
                                                            "Incorrect State Transition");

                    OMX_ERROR4(pComponentPrivate->dbg,
                               "OMX_ErrorInvalidState Given by Comp\n");
                    WBAMRENC_CleanupInitParams(pHandle);
                    break;

                case OMX_StateMax:
                    OMX_PRINT2(pComponentPrivate->dbg, "To Cmd OMX_StateMax\n");
                    break;
            } /* End of Switch */
        }
    } else if (command == OMX_CommandMarkBuffer) {
        OMX_PRBUFFER2(pComponentPrivate->dbg, "OMX_CommandMarkBuffer\n");

        if (!pComponentPrivate->pMarkBuf) {
            /* TODO Need to handle multiple marks */
            pComponentPrivate->pMarkBuf = (OMX_MARKTYPE *)(commandData);
        }
    } else if (command == OMX_CommandPortDisable) {
        if (!pComponentPrivate->bDisableCommandPending) {
            if (commandData == 0x0 || commandData == -1) {
                pComponentPrivate->pPortDef[WBAMRENC_INPUT_PORT]->bEnabled = OMX_FALSE;
            }

            if (commandData == 0x1 || commandData == -1) {
                pComponentPrivate->pPortDef[WBAMRENC_OUTPUT_PORT]->bEnabled = OMX_FALSE;

                if (pComponentPrivate->curState == OMX_StateExecuting) {
                    pComponentPrivate->bNoIdleOnStop = OMX_TRUE;
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               MMCodecControlStop, (void *)pArgs);
                }
            }
        }

        OMX_PRCOMM2(pComponentPrivate->dbg, "commandData = %ld\n", commandData);
        OMX_PRCOMM2(pComponentPrivate->dbg,
                    "WBAMRENC_INPUT_PORT bPopulated = %d\n",
                    pComponentPrivate->pPortDef[WBAMRENC_INPUT_PORT]->bPopulated);
        OMX_PRCOMM2(pComponentPrivate->dbg,
                    "WBAMRENC_OUTPUT_PORT bPopulated = %d\n",
                    pComponentPrivate->pPortDef[WBAMRENC_OUTPUT_PORT]->bPopulated);

        if (commandData == 0x0) {
            if (!pComponentPrivate->pPortDef[WBAMRENC_INPUT_PORT]->bPopulated) {
                /* return cmdcomplete event if input unpopulated */
                pComponentPrivate->cbInfo.EventHandler(pHandle, pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandPortDisable,
                                                       WBAMRENC_INPUT_PORT,
                                                       NULL);
                pComponentPrivate->bDisableCommandPending = 0;
            } else {
                pComponentPrivate->bDisableCommandPending = 1;
                pComponentPrivate->bDisableCommandParam = commandData;
            }
        }

        if (commandData == 0x1) {
            if (!pComponentPrivate->pPortDef[WBAMRENC_OUTPUT_PORT]->bPopulated) {
                /* return cmdcomplete event if output unpopulated */
                pComponentPrivate->cbInfo.EventHandler(pHandle, pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandPortDisable,
                                                       WBAMRENC_OUTPUT_PORT,
                                                       NULL);
                pComponentPrivate->bDisableCommandPending = 0;
            } else {
                pComponentPrivate->bDisableCommandPending = 1;
                pComponentPrivate->bDisableCommandParam = commandData;
            }
        }

        if (commandData == -1) {
            if (!pComponentPrivate->pPortDef[WBAMRENC_INPUT_PORT]->bPopulated &&
                    !pComponentPrivate->pPortDef[WBAMRENC_OUTPUT_PORT]->bPopulated) {

                /* return cmdcomplete event if inout & output unpopulated */
                pComponentPrivate->cbInfo.EventHandler(pHandle, pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandPortDisable,
                                                       WBAMRENC_INPUT_PORT,
                                                       NULL);

                pComponentPrivate->cbInfo.EventHandler(pHandle, pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandPortDisable,
                                                       WBAMRENC_OUTPUT_PORT,
                                                       NULL);
                pComponentPrivate->bDisableCommandPending = 0;
            } else {
                pComponentPrivate->bDisableCommandPending = 1;
                pComponentPrivate->bDisableCommandParam = commandData;
            }
        }

    } else if (command == OMX_CommandPortEnable) {
        if (!pComponentPrivate->bEnableCommandPending) {
            if (commandData == 0x0 || commandData == -1) {
                /* enable in port */
                OMX_PRCOMM2(pComponentPrivate->dbg, "setting input port to enabled\n");
                pComponentPrivate->pPortDef[WBAMRENC_INPUT_PORT]->bEnabled = OMX_TRUE;

                if (pComponentPrivate->AlloBuf_waitingsignal) {
                    pComponentPrivate->AlloBuf_waitingsignal = 0;
                }
            }

            if (commandData == 0x1 || commandData == -1) {
                /* enable out port */
                if (pComponentPrivate->AlloBuf_waitingsignal) {
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
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               EMMCodecControlStart, (void *)pArgs);
                }

                OMX_PRCOMM2(pComponentPrivate->dbg, "setting output port to enabled\n");
                pComponentPrivate->pPortDef[WBAMRENC_OUTPUT_PORT]->bEnabled = OMX_TRUE;
                OMX_PRCOMM2(pComponentPrivate->dbg,
                            "WBAMRENC_OUTPUT_PORT bEnabled = %d\n",
                            pComponentPrivate->pPortDef[WBAMRENC_OUTPUT_PORT]->bEnabled);
            }
        }

        if (commandData == 0x0) {
            if (pComponentPrivate->curState == OMX_StateLoaded ||
                    pComponentPrivate->pPortDef[WBAMRENC_INPUT_PORT]->bPopulated) {
                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandPortEnable,
                                                       WBAMRENC_INPUT_PORT,
                                                       NULL);
                pComponentPrivate->bEnableCommandPending = 0;
            } else {
                pComponentPrivate->bEnableCommandPending = 1;
                pComponentPrivate->bEnableCommandParam = commandData;
            }
        } else if (commandData == 0x1) {
            if (pComponentPrivate->curState == OMX_StateLoaded ||
                    pComponentPrivate->pPortDef[WBAMRENC_OUTPUT_PORT]->bPopulated) {
                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,
                                                        OMX_CommandPortEnable,
                                                        WBAMRENC_OUTPUT_PORT,
                                                        NULL);
                pComponentPrivate->bEnableCommandPending = 0;
            } else {
                pComponentPrivate->bEnableCommandPending = 1;
                pComponentPrivate->bEnableCommandParam = commandData;
            }
        } else if (commandData == -1) {
            if (pComponentPrivate->curState == OMX_StateLoaded ||
                    (pComponentPrivate->pPortDef[WBAMRENC_INPUT_PORT]->bPopulated
                     && pComponentPrivate->pPortDef[WBAMRENC_OUTPUT_PORT]->bPopulated)) {
                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandPortEnable,
                                                       WBAMRENC_INPUT_PORT,
                                                       NULL);
                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandPortEnable,
                                                       WBAMRENC_OUTPUT_PORT,
                                                       NULL);
                pComponentPrivate->bEnableCommandPending = 0;
                WBAMRENC_FillLCMLInitParamsEx(pHandle);
            } else {
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

    } else if (command == OMX_CommandFlush) {
        if (commandData == 0x0 || commandData == -1) {
            OMX_PRCOMM2(pComponentPrivate->dbg, "Flushing input port\n");

            for (i = 0; i < pComponentPrivate->nNumInputBufPending; i++) {
#ifdef __PERF_INSTRUMENTATION__
                PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                  pComponentPrivate->pInputBufferList->pBufHdr[i]->pBuffer,
                                  0,
                                  PERF_ModuleHLMM);
#endif

                pComponentPrivate->cbInfo.EmptyBufferDone (pComponentPrivate->pHandle,
                        pComponentPrivate->pHandle->pApplicationPrivate,
                        pComponentPrivate->pInputBufHdrPending[i]);
                pComponentPrivate->nEmptyBufferDoneCount++;
                pComponentPrivate->nOutStandingEmptyDones--;
            }

            pComponentPrivate->nNumInputBufPending = 0;
            pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                   pHandle->pApplicationPrivate,
                                                   OMX_EventCmdComplete,
                                                   OMX_CommandFlush,
                                                   WBAMRENC_INPUT_PORT,
                                                   NULL);
        }

        if (commandData == 0x1 || commandData == -1) {
            OMX_PRCOMM2(pComponentPrivate->dbg, "Flushing output port\n");

            for (i = 0; i < pComponentPrivate->nNumOutputBufPending; i++) {
#ifdef __PERF_INSTRUMENTATION__
                PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                  pComponentPrivate->pOutputBufferList->pBufHdr[i]->pBuffer,
                                  pComponentPrivate->pOutputBufferList->pBufHdr[i]->nFilledLen,
                                  PERF_ModuleHLMM);
#endif
                pComponentPrivate->cbInfo.FillBufferDone (pComponentPrivate->pHandle,
                        pComponentPrivate->pHandle->pApplicationPrivate,
                        pComponentPrivate->pOutputBufHdrPending[i]);
                pComponentPrivate->nFillBufferDoneCount++;
                pComponentPrivate->nOutStandingFillDones--;
            }

            pComponentPrivate->nNumOutputBufPending = 0;
            pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                   pHandle->pApplicationPrivate,
                                                   OMX_EventCmdComplete,
                                                   OMX_CommandFlush,
                                                   WBAMRENC_OUTPUT_PORT,
                                                   NULL);
        }
    }

EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, "Exiting\n");
    OMX_PRINT1(pComponentPrivate->dbg, "Returning = 0x%x\n", eError);

    if (eError != OMX_ErrorNone && eError != WBAMRENC_EXIT_COMPONENT_THRD) {
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
 * @WBAMRENC_HandleDataBufFromApp() This function is called by the component when ever it
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
OMX_ERRORTYPE WBAMRENC_HandleDataBufFromApp(OMX_BUFFERHEADERTYPE* pBufHeader,
        WBAMRENC_COMPONENT_PRIVATE *pComponentPrivate) {
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_DIRTYPE eDir;
    WBAMRENC_LCML_BUFHEADERTYPE *pLcmlHdr;
    LCML_DSP_INTERFACE *pLcmlHandle = (LCML_DSP_INTERFACE *)
                                      pComponentPrivate->pLcmlHandle;
    OMX_U32 frameLength, remainingBytes;
    OMX_U8* pExtraData;
    OMX_U8 nFrames = 0, i;
    LCML_DSP_INTERFACE * phandle;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefIn = NULL;

    OMX_PRINT1(pComponentPrivate->dbg, "Entering\n");
    /*Find the direction of the received buffer from buffer list */
    eError = WBAMRENC_GetBufferDirection(pBufHeader, &eDir);

    if (eError != OMX_ErrorNone) {
        OMX_ERROR4(pComponentPrivate->dbg, "The pBufHeader is not found in the list\n");
        goto EXIT;
    }

    if (eDir == OMX_DirInput) {
        pComponentPrivate->nEmptyThisBufferCount++;
        pPortDefIn = pComponentPrivate->pPortDef[OMX_DirInput];

        if (pBufHeader->nFilledLen > 0) {
            if (pComponentPrivate->nHoldLength == 0) {
                frameLength = WBAMRENC_INPUT_FRAME_SIZE;
                nFrames = (OMX_U8)(pBufHeader->nFilledLen / frameLength);

                if ( nFrames >= 1 ) {/* At least there is 1 frame in the buffer */

                    pComponentPrivate->nHoldLength = pBufHeader->nFilledLen - frameLength * nFrames;

                    if (pComponentPrivate->nHoldLength > 0) {/* something need to be hold in pHoldBuffer */
                        if (pComponentPrivate->pHoldBuffer == NULL) {
                            OMX_MALLOC_SIZE(pComponentPrivate->pHoldBuffer,
                                            WBAMRENC_INPUT_BUFFER_SIZE,
                                            OMX_U8);
                        }

                        memset(pComponentPrivate->pHoldBuffer, 0, WBAMRENC_INPUT_BUFFER_SIZE);
                        /* Copy the extra data into pHoldBuffer. Size will be nHoldLength. */
                        pExtraData = pBufHeader->pBuffer + frameLength * nFrames;

                        if (pComponentPrivate->nHoldLength <= WBAMRENC_INPUT_BUFFER_SIZE) {
                            memcpy(pComponentPrivate->pHoldBuffer,
                                   pExtraData,
                                   pComponentPrivate->nHoldLength);
                        } else {
                            OMX_ERROR4(pComponentPrivate->dbg,
                                       "Error: pHoldLength is bigger than the input frame size\n");
                            goto EXIT;
                        }

                        pBufHeader->nFilledLen -= pComponentPrivate->nHoldLength;
                    }
                } else {
                    if ( !pComponentPrivate->InBuf_Eos_alreadysent ) {
                        /* received buffer with less than 1 AMR frame length. Save the data in pHoldBuffer.*/
                        pComponentPrivate->nHoldLength = pBufHeader->nFilledLen;

                        /* save the data into pHoldBuffer */
                        if (pComponentPrivate->pHoldBuffer == NULL) {
                            OMX_MALLOC_SIZE(pComponentPrivate->pHoldBuffer,
                                            WBAMRENC_INPUT_BUFFER_SIZE,
                                            OMX_U8);
                        }

                        /* Not enough data to be sent. Copy all received data into pHoldBuffer.*/
                        /* Size to be copied will be iHoldLen == mmData->BufferSize() */
                        memset(pComponentPrivate->pHoldBuffer, 0, WBAMRENC_INPUT_BUFFER_SIZE);

                        if (pComponentPrivate->nHoldLength <= WBAMRENC_INPUT_BUFFER_SIZE) {
                            memcpy(pComponentPrivate->pHoldBuffer,
                                   pBufHeader->pBuffer,
                                   pComponentPrivate->nHoldLength);
                            /* since not enough data, we shouldn't send anything to SN, but instead request to EmptyBufferDone again.*/
                        } else {
                            OMX_ERROR4(pComponentPrivate->dbg,
                                       "Error: pHoldLength is bigger than the input frame size\n");
                            goto EXIT;
                        }
                    }

                    if (pComponentPrivate->curState != OMX_StatePause) {
                        OMX_PRBUFFER2(pComponentPrivate->dbg, "Calling EmptyBufferDone\n");
#ifdef __PERF_INSTRUMENTATION__
                        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                          pBufHeader->pBuffer,
                                          0,
                                          PERF_ModuleHLMM);
#endif
                        pComponentPrivate->cbInfo.EmptyBufferDone( pComponentPrivate->pHandle,
                                pComponentPrivate->pHandle->pApplicationPrivate,
                                pBufHeader);
                        pComponentPrivate->nEmptyBufferDoneCount++;
                    } else {
                        pComponentPrivate->pInputBufHdrPending[pComponentPrivate->nNumInputBufPending++] = pBufHeader;
                    }

                    pComponentPrivate->ProcessingInputBuf--;
                    goto EXIT;
                }
            } else {
                if ((pComponentPrivate->nHoldLength + pBufHeader->nFilledLen) > pBufHeader->nAllocLen) {
                    /*means that a second Acumulator must be used to insert holdbuffer to pbuffer and save remaining bytes
                      into hold buffer*/
                    remainingBytes = pComponentPrivate->nHoldLength + pBufHeader->nFilledLen - pBufHeader->nAllocLen;

                    if (pComponentPrivate->pHoldBuffer2 == NULL) {
                        OMX_MALLOC_SIZE(pComponentPrivate->pHoldBuffer2,
                                        WBAMRENC_INPUT_BUFFER_SIZE,
                                        OMX_U8);
                    }

                    pExtraData = (pBufHeader->pBuffer) + (pBufHeader->nFilledLen - remainingBytes);
                    memcpy(pComponentPrivate->pHoldBuffer2,
                           pExtraData,
                           remainingBytes);
                    pBufHeader->nFilledLen -= remainingBytes;
                    memmove(pBufHeader->pBuffer + pComponentPrivate->nHoldLength,
                            pBufHeader->pBuffer,
                            pBufHeader->nFilledLen);
                    memcpy(pBufHeader->pBuffer,
                           pComponentPrivate->pHoldBuffer,
                           pComponentPrivate->nHoldLength);
                    pBufHeader->nFilledLen += pComponentPrivate->nHoldLength;
                    memcpy(pComponentPrivate->pHoldBuffer,
                           pComponentPrivate->pHoldBuffer2,
                           remainingBytes);
                    pComponentPrivate->nHoldLength = remainingBytes;
                    remainingBytes = 0;
                } else {
                    memmove(pBufHeader->pBuffer + pComponentPrivate->nHoldLength,
                            pBufHeader->pBuffer,
                            pBufHeader->nFilledLen);
                    memcpy(pBufHeader->pBuffer,
                           pComponentPrivate->pHoldBuffer,
                           pComponentPrivate->nHoldLength);
                    pBufHeader->nFilledLen += pComponentPrivate->nHoldLength;
                    pComponentPrivate->nHoldLength = 0;
                }

                frameLength = WBAMRENC_INPUT_BUFFER_SIZE;
                nFrames = (OMX_U8)(pBufHeader->nFilledLen / frameLength);
                pComponentPrivate->nHoldLength = pBufHeader->nFilledLen - frameLength * nFrames;
                pExtraData = pBufHeader->pBuffer + pBufHeader->nFilledLen - pComponentPrivate->nHoldLength;
                memcpy(pComponentPrivate->pHoldBuffer,
                       pExtraData,
                       pComponentPrivate->nHoldLength);
                pBufHeader->nFilledLen -= pComponentPrivate->nHoldLength;

                if (nFrames < 1 ) {
                    if (pComponentPrivate->curState != OMX_StatePause) {
                        OMX_PRBUFFER2(pComponentPrivate->dbg, "Calling EmptyBufferDone\n");
#ifdef __PERF_INSTRUMENTATION__
                        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                          pBufHeader->pBuffer,
                                          0,
                                          PERF_ModuleHLMM);
#endif
                        pComponentPrivate->cbInfo.EmptyBufferDone( pComponentPrivate->pHandle,
                                pComponentPrivate->pHandle->pApplicationPrivate,
                                pBufHeader);
                        pComponentPrivate->nEmptyBufferDoneCount++;
                        goto EXIT;
                    } else {
                        pComponentPrivate->pInputBufHdrPending[pComponentPrivate->nNumInputBufPending++] = pBufHeader;
                    }
                }
            }
        } else {
            if ((pBufHeader->nFlags & OMX_BUFFERFLAG_EOS) != OMX_BUFFERFLAG_EOS) {
                if (pComponentPrivate->dasfMode == 0 && !pBufHeader->pMarkData) {
#ifdef __PERF_INSTRUMENTATION__
                    PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                      pComponentPrivate->pInputBufferList->pBufHdr[0]->pBuffer,
                                      0,
                                      PERF_ModuleHLMM);
#endif
                    pComponentPrivate->cbInfo.EmptyBufferDone( pComponentPrivate->pHandle,
                            pComponentPrivate->pHandle->pApplicationPrivate,
                            pComponentPrivate->pInputBufferList->pBufHdr[0]);
                    pComponentPrivate->nEmptyBufferDoneCount++;
                    pComponentPrivate->ProcessingInputBuf--;
                }
            } else {
                frameLength = WBAMRENC_INPUT_FRAME_SIZE;
                nFrames = 1;
            }
        }

        if (nFrames >= 1) {

            eError = WBAMRENC_GetCorrespondingLCMLHeader(pComponentPrivate,
                     pBufHeader->pBuffer,
                     OMX_DirInput,
                     &pLcmlHdr);

            if (eError != OMX_ErrorNone) {
                OMX_PRBUFFER2(pComponentPrivate->dbg, "Error: Invalid Buffer Came ...\n");
                goto EXIT;
            }

#ifdef __PERF_INSTRUMENTATION__
            PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                              PREF(pBufHeader, pBuffer),
                              pPortDefIn->nBufferSize,
                              PERF_ModuleCommonLayer);
#endif
            /*---------------------------------------------------------------*/

            pComponentPrivate->nNumOfFramesSent = nFrames;
            phandle = (LCML_DSP_INTERFACE *)(((LCML_CODEC_INTERFACE *)pLcmlHandle->pCodecinterfacehandle)->pCodec);

            if ( (pLcmlHdr->pBufferParam->usNbFrames < nFrames) && (pLcmlHdr->pFrameParam != NULL) ) {
                OMX_MEMFREE_STRUCT_DSPALIGN(pLcmlHdr->pFrameParam, OMX_U8);

                OMX_DmmUnMap(phandle->dspCodec->hProc, /*Unmap DSP memory used*/
                             (void*)pLcmlHdr->pBufferParam->pParamElem,
                             pLcmlHdr->pDmmBuf->pReserved, pComponentPrivate->dbg);
                pLcmlHdr->pBufferParam->pParamElem = NULL;
            }

            if (pLcmlHdr->pFrameParam == NULL ) {
                OMX_MALLOC_SIZE_DSPALIGN(pLcmlHdr->pFrameParam,
                                         (sizeof(WBAMRENC_FrameStruct)*(nFrames)),
                                         OMX_U8);

                eError = OMX_DmmMap(phandle->dspCodec->hProc,
                                    (nFrames * sizeof(WBAMRENC_FrameStruct)),
                                    (void*)pLcmlHdr->pFrameParam,
                                    (pLcmlHdr->pDmmBuf), pComponentPrivate->dbg);

                if (eError != OMX_ErrorNone) {
                    OMX_ERROR4(pComponentPrivate->dbg, "OMX_DmmMap ERRROR!!!!\n");
                    goto EXIT;
                }

                pLcmlHdr->pBufferParam->pParamElem = (WBAMRENC_FrameStruct *) pLcmlHdr->pDmmBuf->pMapped;/*DSP Address*/
            }

            for (i = 0; i < nFrames; i++) {
                (pLcmlHdr->pFrameParam + i)->usLastFrame = 0;
            }

            if (pBufHeader->nFlags & OMX_BUFFERFLAG_EOS) {
                (pLcmlHdr->pFrameParam + (nFrames - 1))->usLastFrame = OMX_BUFFERFLAG_EOS;
                pComponentPrivate->InBuf_Eos_alreadysent = 1; /*TRUE*/
            }

            pLcmlHdr->pBufferParam->usNbFrames = nFrames;
            /* Store time stamp information */
            pComponentPrivate->arrBufIndex[pComponentPrivate->IpBufindex] = pBufHeader->nTimeStamp;
            /* Store nTickCount information */
            pComponentPrivate->arrTickCount[pComponentPrivate->IpBufindex] = pBufHeader->nTickCount;
            pComponentPrivate->IpBufindex++;
            pComponentPrivate->IpBufindex %= pComponentPrivate->pPortDef[OMX_DirOutput]->nBufferCountActual;

            if (pComponentPrivate->curState == OMX_StateExecuting) {
                if (!pComponentPrivate->bDspStoppedWhileExecuting) {
                    if (!WBAMRENC_IsPending(pComponentPrivate, pBufHeader, OMX_DirInput)) {
                        WBAMRENC_SetPending(pComponentPrivate, pBufHeader, OMX_DirInput, __LINE__);
                        OMX_PRINT1(pComponentPrivate->dbg,
                                   "IN BUFFER = %p (%ld)\n",
                                   pBufHeader->pBuffer,
                                   pBufHeader->nFilledLen);
                        eError = LCML_QueueBuffer( pLcmlHandle->pCodecinterfacehandle,
                                                   EMMCodecInputBuffer,
                                                   (OMX_U8 *)pBufHeader->pBuffer,
                                                   frameLength * nFrames,
                                                   pBufHeader->nFilledLen,
                                                   (OMX_U8 *) pLcmlHdr->pBufferParam,
                                                   sizeof(WBAMRENC_ParamStruct),
                                                   NULL);

                        if (eError != OMX_ErrorNone) {
                            eError = OMX_ErrorHardware;
                            goto EXIT;
                        }

                        pComponentPrivate->lcml_nCntIp++;
                        pComponentPrivate->lcml_nIpBuf++;
                    }
                }
            } else if (pComponentPrivate->curState == OMX_StatePause) {
                pComponentPrivate->pInputBufHdrPending[pComponentPrivate->nNumInputBufPending++] = pBufHeader;
            }

            pComponentPrivate->ProcessingInputBuf--;
        }

        if (pBufHeader->pMarkData) {
            if (pComponentPrivate->pOutputBufferList->pBufHdr[0] != NULL) {
                /* copy mark to output buffer header */
                OMX_PRBUFFER2(pComponentPrivate->dbg,
                              "Copy mark to buffer %p\n",
                              pComponentPrivate->pOutputBufferList->pBufHdr[0]);
                pComponentPrivate->pOutputBufferList->pBufHdr[0]->pMarkData = pBufHeader->pMarkData;
                pComponentPrivate->pOutputBufferList->pBufHdr[0]->hMarkTargetComponent = pBufHeader->hMarkTargetComponent;
            } else {
                OMX_PRBUFFER2(pComponentPrivate->dbg, "Buffer Mark on NULL\n");
            }

            /* trigger event handler if we are supposed to */
            if ((pBufHeader->hMarkTargetComponent == pComponentPrivate->pHandle) && pBufHeader->pMarkData) {
                OMX_PRINT2(pComponentPrivate->dbg, "OMX Event Mark\n");
                pComponentPrivate->cbInfo.EventHandler( pComponentPrivate->pHandle,
                                                        pComponentPrivate->pHandle->pApplicationPrivate,
                                                        OMX_EventMark,
                                                        0,
                                                        0,
                                                        pBufHeader->pMarkData);
            }

            if (pComponentPrivate->curState != OMX_StatePause &&
                    !WBAMRENC_IsPending(pComponentPrivate, pBufHeader, OMX_DirInput)) {
                OMX_PRBUFFER2(pComponentPrivate->dbg, "Calling EmptyBufferDone\n");
#ifdef __PERF_INSTRUMENTATION__
                PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                  pBufHeader->pBuffer,
                                  0,
                                  PERF_ModuleHLMM);
#endif
                pComponentPrivate->cbInfo.EmptyBufferDone( pComponentPrivate->pHandle,
                        pComponentPrivate->pHandle->pApplicationPrivate,
                        pBufHeader);
                pComponentPrivate->nEmptyBufferDoneCount++;
            }
        }
    } else if (eDir == OMX_DirOutput) {
        /* Make sure that output buffer is issued to output stream only when
         * there is an outstanding input buffer already issued on input stream
         */

        nFrames = pComponentPrivate->nNumOfFramesSent;

        if (nFrames == 0)
            nFrames = 1;

        eError = WBAMRENC_GetCorrespondingLCMLHeader(pComponentPrivate, pBufHeader->pBuffer, OMX_DirOutput, &pLcmlHdr);

        phandle = (LCML_DSP_INTERFACE *)(((LCML_CODEC_INTERFACE *)pLcmlHandle->pCodecinterfacehandle)->pCodec);

        if ( (pLcmlHdr->pBufferParam->usNbFrames < nFrames) && (pLcmlHdr->pFrameParam != NULL) ) {
            OMX_MEMFREE_STRUCT_DSPALIGN(pLcmlHdr->pFrameParam, OMX_U8);
#ifndef UNDER_CE
            OMX_DmmUnMap(phandle->dspCodec->hProc,
                         (void*)pLcmlHdr->pBufferParam->pParamElem,
                         pLcmlHdr->pDmmBuf->pReserved, pComponentPrivate->dbg);
            pLcmlHdr->pBufferParam->pParamElem = NULL;
#endif
        }

        if (pLcmlHdr->pFrameParam == NULL ) {
            OMX_MALLOC_SIZE_DSPALIGN(pLcmlHdr->pFrameParam,
                                     sizeof(WBAMRENC_FrameStruct)*nFrames,
                                     OMX_U8);

            pLcmlHdr->pBufferParam->pParamElem = NULL;
#ifndef UNDER_CE
            eError = OMX_DmmMap(phandle->dspCodec->hProc,
                                (nFrames * sizeof(WBAMRENC_FrameStruct)),
                                (void*)pLcmlHdr->pFrameParam,
                                (pLcmlHdr->pDmmBuf), pComponentPrivate->dbg );

            if (eError != OMX_ErrorNone) {
                OMX_ERROR4(pComponentPrivate->dbg, "OMX_DmmMap ERRROR!!!!\n");
                goto EXIT;
            }

            pLcmlHdr->pBufferParam->pParamElem = (WBAMRENC_FrameStruct *)pLcmlHdr->pDmmBuf->pMapped; /*DSP Address*/
#endif
        }

        pLcmlHdr->pBufferParam->usNbFrames = nFrames;

#ifdef __PERF_INSTRUMENTATION__
        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                          PREF(pBufHeader, pBuffer),
                          0,
                          PERF_ModuleCommonLayer);
#endif

        if (pComponentPrivate->curState == OMX_StateExecuting) {
            if (!WBAMRENC_IsPending(pComponentPrivate, pBufHeader, OMX_DirOutput)) {
                WBAMRENC_SetPending(pComponentPrivate, pBufHeader, OMX_DirOutput, __LINE__);
                OMX_PRINT1(pComponentPrivate->dbg,
                           "OUT BUFFER = %p (%d)\n",
                           pBufHeader->pBuffer,
                           WBAMRENC_OUTPUT_FRAME_SIZE * nFrames);
                eError = LCML_QueueBuffer( pLcmlHandle->pCodecinterfacehandle,
                                           EMMCodecOuputBuffer,
                                           (OMX_U8 *)pBufHeader->pBuffer,
                                           WBAMRENC_OUTPUT_FRAME_SIZE * nFrames,
                                           0,
                                           (OMX_U8 *) pLcmlHdr->pBufferParam,
                                           sizeof(WBAMRENC_ParamStruct),
                                           NULL);

                if (eError != OMX_ErrorNone ) {
                    OMX_ERROR4(pComponentPrivate->dbg, "IssuingDSP OP: Error Occurred\n");
                    eError = OMX_ErrorHardware;
                    goto EXIT;
                }

                pComponentPrivate->lcml_nOpBuf++;
            }
        } else if (pComponentPrivate->curState == OMX_StatePause) {
            pComponentPrivate->pOutputBufHdrPending[pComponentPrivate->nNumOutputBufPending++] = pBufHeader;
        }

        pComponentPrivate->ProcessingOutputBuf--;
    } else {
        eError = OMX_ErrorBadParameter;
    }

EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, "Exiting\n");
    OMX_PRINT1(pComponentPrivate->dbg, "Returning error %d\n", eError);

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
 * WBAMRENC_GetBufferDirection () This function is used by the component
 * to get the direction of the buffer
 * @param eDir pointer will be updated with buffer direction
 * @param pBufHeader pointer to the buffer to be requested to be filled
 *
 * @retval none
 **/
/*-------------------------------------------------------------------*/

OMX_ERRORTYPE WBAMRENC_GetBufferDirection(OMX_BUFFERHEADERTYPE *pBufHeader,
        OMX_DIRTYPE *eDir) {
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    WBAMRENC_COMPONENT_PRIVATE *pComponentPrivate = pBufHeader->pPlatformPrivate;
    OMX_U32 nBuf = 0;
    OMX_BUFFERHEADERTYPE *pBuf = NULL;
    OMX_U32 flag = 1, i = 0;
    OMX_PRINT1(pComponentPrivate->dbg, "Entering\n");
    /*Search this buffer in input buffers list */
    nBuf = pComponentPrivate->pInputBufferList->numBuffers;

    for (i = 0; i < nBuf; i++) {
        pBuf = pComponentPrivate->pInputBufferList->pBufHdr[i];

        if (pBufHeader == pBuf) {
            *eDir = OMX_DirInput;
            OMX_PRBUFFER2(pComponentPrivate->dbg,
                          "pBufHeader = %p is INPUT BUFFER pBuf = %p\n",
                          pBufHeader,
                          pBuf);
            flag = 0;
            goto EXIT;
        }
    }

    /*Search this buffer in output buffers list */
    nBuf = pComponentPrivate->pOutputBufferList->numBuffers;

    for (i = 0; i < nBuf; i++) {
        pBuf = pComponentPrivate->pOutputBufferList->pBufHdr[i];

        if (pBufHeader == pBuf) {
            *eDir = OMX_DirOutput;
            OMX_PRBUFFER2(pComponentPrivate->dbg, "pBufHeader = %p is OUTPUT BUFFER pBuf = %p\n",
                          pBufHeader,
                          pBuf);
            flag = 0;
            goto EXIT;
        }
    }

    if (flag == 1) {
        OMX_ERROR4(pComponentPrivate->dbg,
                   "Buffer %p is Not Found in the List\n", pBufHeader);
        eError = OMX_ErrorUndefined;
        goto EXIT;
    }

EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, "Exiting\n");
    OMX_PRINT1(pComponentPrivate->dbg, "Returning = 0x%x\n", eError);
    return eError;
}

/* -------------------------------------------------------------------*/
/**
 * WBAMRENC_GetCorrespondingLCMLHeader() function will be called by LCML_Callback
 *  component to write the msg
 * @param *pBuffer,          Event which gives to details about USN status
 * @param WBAMRENC_LCML_BUFHEADERTYPE **ppLcmlHdr
 * @param  OMX_DIRTYPE eDir this gives direction of the buffer
 * @retval OMX_NoError              Success, ready to roll
 *         OMX_Error_BadParameter   The input parameter pointer is null
 **/
/* -------------------------------------------------------------------*/
OMX_ERRORTYPE WBAMRENC_GetCorrespondingLCMLHeader(WBAMRENC_COMPONENT_PRIVATE *pComponentPrivate,
        OMX_U8 *pBuffer,
        OMX_DIRTYPE eDir,
        WBAMRENC_LCML_BUFHEADERTYPE **ppLcmlHdr) {
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    WBAMRENC_LCML_BUFHEADERTYPE *pLcmlBufHeader;

    OMX_U32 nIpBuf;
    OMX_U32 nOpBuf;
    OMX_U16 i;

    nIpBuf = pComponentPrivate->pInputBufferList->numBuffers;
    nOpBuf = pComponentPrivate->pOutputBufferList->numBuffers;

    OMX_PRINT1(pComponentPrivate->dbg, "Entering\n");

    while (!pComponentPrivate->bInitParamsInitialized) {
        OMX_PRSTATE2(pComponentPrivate->dbg,
                     "Waiting for init to complete........\n");
#ifndef UNDER_CE
        sched_yield();
#else
        Sleep(0);
#endif
    }

    if (eDir == OMX_DirInput) {
        pLcmlBufHeader = pComponentPrivate->pLcmlBufHeader[WBAMRENC_INPUT_PORT];

        for (i = 0; i < nIpBuf; i++) {
            OMX_PRBUFFER2(pComponentPrivate->dbg, "pBuffer = %p\n", pBuffer);
            OMX_PRBUFFER2(pComponentPrivate->dbg,
                          "pLcmlBufHeader->buffer->pBuffer = %p\n",
                          pLcmlBufHeader->buffer->pBuffer);

            if (pBuffer == pLcmlBufHeader->buffer->pBuffer) {
                *ppLcmlHdr = pLcmlBufHeader;
                OMX_PRDSP2(pComponentPrivate->dbg,
                           "Corresponding Input LCML Header Found = %p\n",
                           pLcmlBufHeader);
                eError = OMX_ErrorNone;
                goto EXIT;
            }

            pLcmlBufHeader++;
        }
    } else if (eDir == OMX_DirOutput) {
        pLcmlBufHeader = pComponentPrivate->pLcmlBufHeader[WBAMRENC_OUTPUT_PORT];

        for (i = 0; i < nOpBuf; i++) {
            OMX_PRBUFFER2(pComponentPrivate->dbg, "pBuffer = %p\n", pBuffer);
            OMX_PRBUFFER2(pComponentPrivate->dbg,
                          "pLcmlBufHeader->buffer->pBuffer = %p\n",
                          pLcmlBufHeader->buffer->pBuffer);

            if (pBuffer == pLcmlBufHeader->buffer->pBuffer) {
                *ppLcmlHdr = pLcmlBufHeader;
                OMX_PRDSP2(pComponentPrivate->dbg,
                           "Corresponding Output LCML Header Found = %p\n",
                           pLcmlBufHeader);
                eError = OMX_ErrorNone;
                goto EXIT;
            }

            pLcmlBufHeader++;
        }
    } else {
        OMX_ERROR4(pComponentPrivate->dbg, "Invalid Buffer Type :: exiting...\n");
        eError = OMX_ErrorUndefined;
    }

EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, "Exiting\n");
    OMX_PRINT1(pComponentPrivate->dbg, "Returning = 0x%x\n", eError);
    return eError;
}

/* -------------------------------------------------------------------*/
/**
 *  WBAMRENC_LCMLCallback() will be called LCML component to write the msg
 *
 * @param event                 Event which gives to details about USN status
 * @param void * args         //    args [0] //bufType;
                              //    args [1] //arm address fpr buffer
                              //    args [2] //BufferSize;
                              //    args [3]  //arm address for param
                              //    args [4] //ParamSize;
                              //    args [6] //LCML Handle
                              * @retval OMX_NoError              Success, ready to roll
                              *         OMX_Error_BadParameter   The input parameter pointer is null
                              **/
/*-------------------------------------------------------------------*/

OMX_ERRORTYPE WBAMRENC_LCMLCallback (TUsnCodecEvent event, void * args[10]) {
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U8 *pBuffer = args[1];
    WBAMRENC_LCML_BUFHEADERTYPE *pLcmlHdr;

    OMX_U16 i, index, frameLength, length;
    OMX_COMPONENTTYPE *pHandle;
    LCML_DSP_INTERFACE *pLcmlHandle;
    OMX_U8 nFrames;

    WBAMRENC_COMPONENT_PRIVATE* pComponentPrivate = NULL;
    pComponentPrivate = (WBAMRENC_COMPONENT_PRIVATE*)((LCML_DSP_INTERFACE*)args[6])->pComponentPrivate;
    pHandle = pComponentPrivate->pHandle;

    OMX_PRINT1(pComponentPrivate->dbg, "Entering\n");

    switch (event) {

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

    if (event == EMMCodecBufferProcessed) {
        if ((OMX_U32)args[0] == EMMCodecInputBuffer) {
            pComponentPrivate->nOutStandingEmptyDones++;
            eError = WBAMRENC_GetCorrespondingLCMLHeader(pComponentPrivate, pBuffer, OMX_DirInput, &pLcmlHdr);

            if (eError != OMX_ErrorNone) {
                OMX_ERROR4(pComponentPrivate->dbg, "Error: Invalid Buffer Came ...\n");
                goto EXIT;
            }

#ifdef __PERF_INSTRUMENTATION__
            PERF_ReceivedFrame(pComponentPrivate->pPERFcomp,
                               PREF(pLcmlHdr->buffer, pBuffer),
                               0,
                               PERF_ModuleCommonLayer);
#endif

            WBAMRENC_ClearPending(pComponentPrivate, pLcmlHdr->buffer, OMX_DirInput, __LINE__);

            OMX_PRBUFFER2(pComponentPrivate->dbg, "Calling EmptyBufferDone\n");
#ifdef __PERF_INSTRUMENTATION__
            PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                              pLcmlHdr->buffer->pBuffer,
                              0,
                              PERF_ModuleHLMM);
#endif
            pComponentPrivate->cbInfo.EmptyBufferDone(pHandle,
                    pHandle->pApplicationPrivate,
                    pLcmlHdr->buffer);


            pComponentPrivate->nEmptyBufferDoneCount++;
            pComponentPrivate->nOutStandingEmptyDones--;
            pComponentPrivate->lcml_nIpBuf--;
            pComponentPrivate->app_nBuf++;
        } else if ((OMX_U32)args[0] == EMMCodecOuputBuffer) {

            if (!WBAMRENC_IsValid(pComponentPrivate, pBuffer, OMX_DirOutput)) {

                for (i = 0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {
#ifdef __PERF_INSTRUMENTATION__
                    PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                      pComponentPrivate->pOutputBufferList->pBufHdr[i]->pBuffer,
                                      pComponentPrivate->pOutputBufferList->pBufHdr[i]->nFilledLen,
                                      PERF_ModuleHLMM);
#endif
                    pComponentPrivate->cbInfo.FillBufferDone (pComponentPrivate->pHandle,
                            pComponentPrivate->pHandle->pApplicationPrivate,
                            pComponentPrivate->pOutputBufferList->pBufHdr[i++]
                                                             );
                }
            } else {
                OMX_PRBUFFER2(pComponentPrivate->dbg, "OUTPUT: pBuffer = %p\n", pBuffer);
                pComponentPrivate->nOutStandingFillDones++;
                eError = WBAMRENC_GetCorrespondingLCMLHeader(pComponentPrivate,
                         pBuffer,
                         OMX_DirOutput,
                         &pLcmlHdr);

                if (eError != OMX_ErrorNone) {
                    OMX_ERROR4(pComponentPrivate->dbg, "Error: Invalid Buffer Came ...\n");
                    goto EXIT;
                }

                OMX_PRBUFFER2(pComponentPrivate->dbg,
                              "Output: pLcmlHdr->buffer->pBuffer = %p\n",
                              pLcmlHdr->buffer->pBuffer);
                pLcmlHdr->buffer->nFilledLen = (OMX_U32)args[8];
                pComponentPrivate->lcml_nCntOpReceived++;
#ifdef __PERF_INSTRUMENTATION__
                PERF_ReceivedFrame(pComponentPrivate->pPERFcomp,
                                   PREF(pLcmlHdr->buffer, pBuffer),
                                   PREF(pLcmlHdr->buffer, nFilledLen),
                                   PERF_ModuleCommonLayer);
                pComponentPrivate->nLcml_nCntOpReceived++;

                if ((pComponentPrivate->nLcml_nCntIp >= 1) && (pComponentPrivate->nLcml_nCntOpReceived == 1)) {
                    PERF_Boundary(pComponentPrivate->pPERFcomp,
                                  PERF_BoundaryStart | PERF_BoundarySteadyState);
                }

#endif
                OMX_PRBUFFER2(pComponentPrivate->dbg,
                              "Output: pBuffer = %ld\n",
                              pLcmlHdr->buffer->nFilledLen);

                WBAMRENC_ClearPending(pComponentPrivate,
                                      pLcmlHdr->buffer,
                                      OMX_DirOutput,
                                      __LINE__);
                pComponentPrivate->LastOutbuf = pLcmlHdr->buffer;

                if ((pComponentPrivate->frameMode == WBAMRENC_MIMEMODE) && (pLcmlHdr->buffer->nFilledLen)) {
                    nFrames = (OMX_U8)(pLcmlHdr->buffer->nFilledLen / WBAMRENC_OUTPUT_BUFFER_SIZE_MIME);
                    frameLength = 0;
                    length = 0;

                    for (i = 0; i < nFrames; i++) {
                        index = (pLcmlHdr->buffer->pBuffer[i*WBAMRENC_OUTPUT_BUFFER_SIZE_MIME] >> 3) & 0x0F;

                        if (pLcmlHdr->buffer->nFilledLen == 0)
                            length = 0;
                        else
                            length = (OMX_U16)pComponentPrivate->amrMimeBytes[index];

                        if (i) {
                            memmove( pLcmlHdr->buffer->pBuffer + frameLength,
                                     pLcmlHdr->buffer->pBuffer + (i * WBAMRENC_OUTPUT_BUFFER_SIZE_MIME),
                                     length);
                        }

                        frameLength += length;
                    }

                    pLcmlHdr->buffer->nFilledLen = frameLength;

                }

                else if ((pComponentPrivate->frameMode == WBAMRENC_IF2) && (pLcmlHdr->buffer->nFilledLen)) {
                    nFrames = (OMX_U8)( pLcmlHdr->buffer->nFilledLen / WBAMRENC_OUTPUT_BUFFER_SIZE_IF2);
                    frameLength = 0;
                    length = 0;

                    for (i = 0; i < nFrames; i++) {
                        index = (pLcmlHdr->buffer->pBuffer[i*WBAMRENC_OUTPUT_BUFFER_SIZE_IF2] >> 4) & 0x0F;

                        if (pLcmlHdr->buffer->nFilledLen == 0)
                            length = 0;
                        else
                            length = (OMX_U16)pComponentPrivate->amrIf2Bytes[index];

                        if (i) {
                            memmove( pLcmlHdr->buffer->pBuffer + frameLength,
                                     pLcmlHdr->buffer->pBuffer + (i * WBAMRENC_OUTPUT_BUFFER_SIZE_IF2),
                                     length);
                        }

                        frameLength += length;
                    }

                    pLcmlHdr->buffer->nFilledLen = frameLength;
                } else {
                    nFrames = pLcmlHdr->buffer->nFilledLen / WBAMRENC_OUTPUT_FRAME_SIZE;
                }

                if ( !pComponentPrivate->dasfMode ) {
                    /* Copying time stamp information to output buffer */
                    pLcmlHdr->buffer->nTimeStamp = (OMX_TICKS)pComponentPrivate->arrBufIndex[pComponentPrivate->OpBufindex];
                    /* Copying nTickCount information to output buffer */
                    pLcmlHdr->buffer->nTickCount = pComponentPrivate->arrTickCount[pComponentPrivate->OpBufindex];
                    pComponentPrivate->OpBufindex++;
                    pComponentPrivate->OpBufindex %= pComponentPrivate->pPortDef[OMX_DirOutput]->nBufferCountActual;
                }

                if (pComponentPrivate->InBuf_Eos_alreadysent) {

                    pLcmlHdr->buffer->nFlags |= OMX_BUFFERFLAG_EOS;

                    pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventBufferFlag,
                                                           pLcmlHdr->buffer->nOutputPortIndex,
                                                           pLcmlHdr->buffer->nFlags,
                                                           NULL);
                    pComponentPrivate->InBuf_Eos_alreadysent = 0;
                }

                /* Non Multi Frame Mode has been tested here */
#ifdef __PERF_INSTRUMENTATION__
                PERF_SendingBuffer(pComponentPrivate->pPERFcomp,
                                   pLcmlHdr->buffer->pBuffer,
                                   pLcmlHdr->buffer->nFilledLen,
                                   PERF_ModuleHLMM);
#endif
                pComponentPrivate->nFillBufferDoneCount++;
                pComponentPrivate->nOutStandingFillDones--;
                pComponentPrivate->lcml_nOpBuf--;
                pComponentPrivate->app_nBuf++;

                pComponentPrivate->cbInfo.FillBufferDone(pHandle,
                        pHandle->pApplicationPrivate,
                        pLcmlHdr->buffer);

                OMX_PRBUFFER2(pComponentPrivate->dbg,
                              "Incrementing app_nBuf = %ld\n",
                              pComponentPrivate->app_nBuf);
            }
        }
    } else if (event == EMMCodecStrmCtrlAck) {
        OMX_PRDSP2(pComponentPrivate->dbg, "GOT MESSAGE USN_DSPACK_STRMCTRL \n");

        if (args[1] == (void *)USN_STRMCMD_FLUSH) {
            pHandle = pComponentPrivate->pHandle;

            if ( args[2] == (void *)EMMCodecInputBuffer) {
                if (args[0] == (void *)USN_ERR_NONE ) {
                    OMX_PRCOMM2(pComponentPrivate->dbg, "Flushing input port\n");

                    for (i = 0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {

#ifdef __PERF_INSTRUMENTATION__
                        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                          pComponentPrivate->pInputBufferList->pBufHdr[i]->pBuffer,
                                          0,
                                          PERF_ModuleHLMM);
#endif

                        pComponentPrivate->cbInfo.EmptyBufferDone (pHandle,
                                pHandle->pApplicationPrivate,
                                pComponentPrivate->pInputBufHdrPending[i]);
                        pComponentPrivate->nEmptyBufferDoneCount++;
                        pComponentPrivate->nOutStandingEmptyDones--;

                    }

                    pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventCmdComplete,
                                                           OMX_CommandFlush,
                                                           WBAMRENC_INPUT_PORT,
                                                           NULL);
                } else {
                    OMX_ERROR4(pComponentPrivate->dbg,
                               "LCML reported error while flushing input port\n");
                    goto EXIT;
                }
            } else if ( args[2] == (void *)EMMCodecOuputBuffer) {
                if (args[0] == (void *)USN_ERR_NONE ) {
                    OMX_PRCOMM2(pComponentPrivate->dbg, "Flushing output port\n");

                    for (i = 0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {
#ifdef __PERF_INSTRUMENTATION__
                        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                          pComponentPrivate->pOutputBufferList->pBufHdr[i]->pBuffer,
                                          pComponentPrivate->pOutputBufferList->pBufHdr[i]->nFilledLen,
                                          PERF_ModuleHLMM);
#endif
                        pComponentPrivate->cbInfo.FillBufferDone (pHandle,
                                pHandle->pApplicationPrivate,
                                pComponentPrivate->pOutputBufHdrPending[i]);
                        pComponentPrivate->nFillBufferDoneCount++;
                        pComponentPrivate->nOutStandingFillDones--;

                    }

                    pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventCmdComplete,
                                                           OMX_CommandFlush,
                                                           WBAMRENC_OUTPUT_PORT,
                                                           NULL);
                } else {
                    OMX_ERROR4(pComponentPrivate->dbg, "LCML reported error while flushing output port\n");
                    goto EXIT;
                }
            }
        }
    } else if (event == EMMCodecProcessingStoped) {
        OMX_PRINT2(pComponentPrivate->dbg, "GOT MESSAGE USN_DSPACK_STOP \n");

        if ((pComponentPrivate->nMultiFrameMode == 1) && (pComponentPrivate->frameMode == WBAMRENC_MIMEMODE)) {
            /*Sending Last Buufer Data which on iHoldBuffer to App */
            OMX_PRBUFFER2(pComponentPrivate->dbg,
                          "Sending iMMFDataLastBuffer Data which on iHoldBuffer to App\n");
            OMX_MALLOC_SIZE(pComponentPrivate->iMMFDataLastBuffer,
                            WBAMRENC_OUTPUT_BUFFER_SIZE_MIME * (pComponentPrivate->pOutputBufferList->numBuffers + 1),
                            OMX_BUFFERHEADERTYPE);

            OMX_PRINT2(pComponentPrivate->dbg,
                       "pComponentPrivate->iHoldLen = %ld \n",
                       pComponentPrivate->iHoldLen);
            /* Copy the data from iHoldBuffer to dataPtr */
	    if (pComponentPrivate->iHoldBuffer == NULL) {
	        eError = OMX_ErrorBadParameter;
		goto EXIT;
	    }
            memcpy(pComponentPrivate->iMMFDataLastBuffer,
                   pComponentPrivate->iHoldBuffer,
                   pComponentPrivate->iHoldLen);
            pComponentPrivate->iMMFDataLastBuffer->nFilledLen = pComponentPrivate->iHoldLen;
            OMX_PRINT2(pComponentPrivate->dbg,
                       "pComponentPrivate->iMMFDataLastBuffer->nFilledLen = %ld \n",
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
            pComponentPrivate->cbInfo.FillBufferDone( pComponentPrivate->pHandle,
                    pComponentPrivate->pHandle->pApplicationPrivate,
                    pComponentPrivate->iMMFDataLastBuffer);
            pComponentPrivate->nFillBufferDoneCount++;
            pComponentPrivate->nOutStandingFillDones--;
            pComponentPrivate->lcml_nOpBuf--;
            pComponentPrivate->app_nBuf++;

            OMX_PRBUFFER2(pComponentPrivate->dbg,
                          "Incrementing app_nBuf = %ld\n",
                          pComponentPrivate->app_nBuf);
        }

        for (i = 0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {

            if (pComponentPrivate->pInputBufferList->bBufferPending[i]) {
#ifdef __PERF_INSTRUMENTATION__
                PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                  pComponentPrivate->pInputBufferList->pBufHdr[i]->pBuffer,
                                  0,
                                  PERF_ModuleHLMM);
#endif

                pComponentPrivate->cbInfo.EmptyBufferDone (pComponentPrivate->pHandle,
                        pComponentPrivate->pHandle->pApplicationPrivate,
                        pComponentPrivate->pInputBufferList->pBufHdr[i]);
                pComponentPrivate->nEmptyBufferDoneCount++;

                pComponentPrivate->nOutStandingEmptyDones--;
                WBAMRENC_ClearPending(pComponentPrivate,
                                      pComponentPrivate->pInputBufferList->pBufHdr[i],
                                      OMX_DirInput,
                                      __LINE__);
            }
        }

        for (i = 0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {
            if (pComponentPrivate->pOutputBufferList->bBufferPending[i]) {
#ifdef __PERF_INSTRUMENTATION__
                PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                  pComponentPrivate->pOutputBufferList->pBufHdr[i]->pBuffer,
                                  pComponentPrivate->pOutputBufferList->pBufHdr[i]->nFilledLen,
                                  PERF_ModuleHLMM);
#endif
                pComponentPrivate->cbInfo.FillBufferDone (pComponentPrivate->pHandle,
                        pComponentPrivate->pHandle->pApplicationPrivate,
                        pComponentPrivate->pOutputBufferList->pBufHdr[i]);
                pComponentPrivate->nFillBufferDoneCount++;
                pComponentPrivate->nOutStandingFillDones--;

                WBAMRENC_ClearPending(pComponentPrivate,
                                      pComponentPrivate->pOutputBufferList->pBufHdr[i],
                                      OMX_DirOutput,
                                      __LINE__);
            }
        }


        if (!pComponentPrivate->bNoIdleOnStop) {
            pComponentPrivate->nNumOutputBufPending = 0;

            pComponentPrivate->ProcessingInputBuf = 0;
            pComponentPrivate->ProcessingOutputBuf = 0;
            pComponentPrivate->InBuf_Eos_alreadysent  = 0;
            pComponentPrivate->curState = OMX_StateIdle;

            /* Decrement reference count with signal enabled */
            if(RemoveStateTransition(pComponentPrivate, OMX_TRUE) != OMX_ErrorNone) {
                 return OMX_ErrorUndefined;
            }

#ifdef RESOURCE_MANAGER_ENABLED
            eError = RMProxy_NewSendCommand(pHandle,
                                            RMProxy_StateSet,
                                            OMX_WBAMR_Encoder_COMPONENT,
                                            OMX_StateIdle,
                                            3456,
                                            NULL);
#endif

            if (pComponentPrivate->bPreempted == 0) {
                pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandStateSet,
                                                       pComponentPrivate->curState,
                                                       NULL);
            } else {
                pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                                       OMX_EventError,
                                                       OMX_ErrorResourcesPreempted,
                                                       OMX_TI_ErrorSevere,
                                                       NULL);
            }

        } else {
            pComponentPrivate->bNoIdleOnStop = OMX_FALSE;
            pComponentPrivate->bDspStoppedWhileExecuting = OMX_TRUE;
        }
    }

    else if (event == EMMCodecDspMessageRecieved) {
        OMX_PRSTATE2(pComponentPrivate->dbg, "commandedState  = %ld\n", (OMX_U32)args[0]);
        OMX_PRINT2(pComponentPrivate->dbg, "arg1 = %ld\n", (OMX_U32)args[1]);
        OMX_PRINT2(pComponentPrivate->dbg, "arg2 = %ld\n", (OMX_U32)args[2]);

        if (0x0500 == (OMX_U32)args[2]) {
            OMX_PRINT2(pComponentPrivate->dbg, "EMMCodecDspMessageRecieved\n");
        }
    } else if (event == EMMCodecAlgCtrlAck) {
        OMX_PRDSP2(pComponentPrivate->dbg, "GOT MESSAGE USN_DSPACK_ALGCTRL \n");
    } else if (event == EMMCodecDspError) {
        switch ( (OMX_U32) args [4]) {
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
            case USN_ERR_UNKNOWN_MSG: {
                pComponentPrivate->bIsInvalidState = OMX_TRUE;
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
                WBAMRENC_HandleUSNError (pComponentPrivate, (OMX_U32)args[5]);
                break;
            default:
                break;
        }
    } else if (event == EMMCodecProcessingPaused) {
        pComponentPrivate->curState = OMX_StatePause;

        /* Decrement reference count with signal enabled */
        if(RemoveStateTransition(pComponentPrivate, OMX_TRUE) != OMX_ErrorNone) {
              return OMX_ErrorUndefined;
        }

        pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                pHandle->pApplicationPrivate,
                                                OMX_EventCmdComplete,
                                                OMX_CommandStateSet,
                                                pComponentPrivate->curState,
                                                NULL);
    }

#ifdef _ERROR_PROPAGATION__
    else if (event == EMMCodecInitError) {
        /* Cheking for MMU_fault */
        if (((int)args[4] == USN_ERR_UNKNOWN_MSG) && (args[5] == (void*)NULL)) {
            pComponentPrivate->bIsInvalidState = OMX_TRUE;
            pComponentPrivate->curState = OMX_StateInvalid;
            pHandle = pComponentPrivate->pHandle;
            pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                   pHandle->pApplicationPrivate,
                                                   OMX_EventError,
                                                   OMX_ErrorInvalidState,
                                                   OMX_TI_ErrorSevere,
                                                   NULL);
        }
    } else if (event == EMMCodecInternalError) {
        /* Cheking for MMU_fault */
        if (((int)args[4] == USN_ERR_UNKNOWN_MSG) && (args[5] == (void*)NULL)) {
            OMX_ERROR4(pComponentPrivate->dbg, "MMU_Fault\n");
            pComponentPrivate->bIsInvalidState = OMX_TRUE;
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
    OMX_PRINT1(pComponentPrivate->dbg, "Exiting\n");
    OMX_PRINT1(pComponentPrivate->dbg, "Returning = 0x%x\n", eError);

    return eError;
}

/* ================================================================================= */
/**
 *  WBAMRENC_GetLCMLHandle()
 *
 * @retval OMX_HANDLETYPE
 */
/* ================================================================================= */
#ifndef UNDER_CE
OMX_HANDLETYPE WBAMRENC_GetLCMLHandle(WBAMRENC_COMPONENT_PRIVATE *pComponentPrivate) {
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_ERRORTYPE (*fpGetHandle)(OMX_HANDLETYPE);
    OMX_HANDLETYPE pHandle = NULL;
    void *handle;
    char *error;

    OMX_PRINT1(pComponentPrivate->dbg, "Entering\n");
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

    if (eError != OMX_ErrorNone) {
        eError = OMX_ErrorUndefined;
        OMX_ERROR4(pComponentPrivate->dbg, "OMX_ErrorUndefined...\n");
        pHandle = NULL;
        goto EXIT;
    }

    pComponentPrivate = (WBAMRENC_COMPONENT_PRIVATE*)pComponentPrivate;
    ((LCML_DSP_INTERFACE*)pHandle)->pComponentPrivate = pComponentPrivate;

    pComponentPrivate->ptrLibLCML = handle;         /* saving LCML lib pointer  */

EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, "Exiting\n");
    OMX_PRINT1(pComponentPrivate->dbg, "Returning = 0x%x\n", eError);
    return pHandle;
}

#else
/*WINDOWS Explicit dll load procedure*/
OMX_HANDLETYPE WBAMRENC_GetLCMLHandle(WBAMRENC_COMPONENT_PRIVATE *pComponentPrivate) {
    typedef OMX_ERRORTYPE (*LPFNDLLFUNC1)(OMX_HANDLETYPE);
    OMX_HANDLETYPE pHandle = NULL;
    OMX_ERRORTYPE eError;
    HINSTANCE hDLL;               // Handle to DLL
    LPFNDLLFUNC1 fpGetHandle1;
    hDLL = LoadLibraryEx(TEXT("OAF_BML.dll"), NULL, 0);

    if (hDLL == NULL) {

        OMX_ERROR4(pComponentPrivate->dbg, "BML Load Failed!!!\n");
        return pHandle;
    }

    fpGetHandle1 = (LPFNDLLFUNC1)GetProcAddress(hDLL, TEXT("GetHandle"));

    if (!fpGetHandle1) {
        // handle the error
        FreeLibrary(hDLL);

        return pHandle;
    }

    // call the function
    eError = fpGetHandle1(&pHandle);

    if (eError != OMX_ErrorNone) {
        eError = OMX_ErrorUndefined;
        OMX_ERROR4(pComponentPrivate->dbg, "eError != OMX_ErrorNone...\n");


        pHandle = NULL;
        return pHandle;
    }

    ((LCML_DSP_INTERFACE*)pHandle)->pComponentPrivate = pComponentPrivate;
    return pHandle;
}
#endif

/* ================================================================================= */
/**
 * @fn WBAMRENC_SetPending() description for WBAMRENC_SetPending
 WBAMRENC_SetPending().
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
void WBAMRENC_SetPending(WBAMRENC_COMPONENT_PRIVATE *pComponentPrivate,
                         OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir, OMX_U32 lineNumber) {
    OMX_U16 i;

    if (eDir == OMX_DirInput) {
        for (i = 0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pInputBufferList->pBufHdr[i]) {
                pComponentPrivate->pInputBufferList->bBufferPending[i] = 1;
            }
        }
    } else {
        for (i = 0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pOutputBufferList->pBufHdr[i]) {

                pComponentPrivate->pOutputBufferList->bBufferPending[i] = 1;
            }
        }
    }
}
/* ================================================================================= */
/**
 * @fn WBAMRENC_ClearPending() description for WBAMRENC_ClearPending
 WBAMRENC_ClearPending().
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
void WBAMRENC_ClearPending(WBAMRENC_COMPONENT_PRIVATE *pComponentPrivate,
                           OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir, OMX_U32 lineNumber) {
    OMX_U16 i;

    if (eDir == OMX_DirInput) {
        for (i = 0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pInputBufferList->pBufHdr[i]) {
                pComponentPrivate->pInputBufferList->bBufferPending[i] = 0;
            }
        }
    } else {
        for (i = 0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pOutputBufferList->pBufHdr[i]) {
                pComponentPrivate->pOutputBufferList->bBufferPending[i] = 0;
            }
        }
    }
}
/* ================================================================================= */
/**
 * @fn WBAMRENC_IsPending() description for WBAMRENC_IsPending
 WBAMRENC_IsPending().
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
OMX_U32 WBAMRENC_IsPending(WBAMRENC_COMPONENT_PRIVATE *pComponentPrivate,
                           OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir) {
    OMX_U16 i;

    if (eDir == OMX_DirInput) {
        for (i = 0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pInputBufferList->pBufHdr[i]) {
                return pComponentPrivate->pInputBufferList->bBufferPending[i];
            }
        }
    } else {
        for (i = 0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pOutputBufferList->pBufHdr[i]) {
                return pComponentPrivate->pOutputBufferList->bBufferPending[i];
            }
        }
    }

    return -1;
}
/* ================================================================================= */
/**
 * @fn WBAMRENC_IsValid() description for WBAMRENC_IsValid
 WBAMRENC_IsValid().
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
OMX_U32 WBAMRENC_IsValid(WBAMRENC_COMPONENT_PRIVATE *pComponentPrivate,
                         OMX_U8 *pBuffer, OMX_DIRTYPE eDir) {
    OMX_U16 i;
    int found = 0;

    if (eDir == OMX_DirInput) {
        for (i = 0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
            if (pBuffer == pComponentPrivate->pInputBufferList->pBufHdr[i]->pBuffer) {
                found = 1;
            }
        }
    } else {
        for (i = 0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {
            if (pBuffer == pComponentPrivate->pOutputBufferList->pBufHdr[i]->pBuffer) {
                found = 1;
            }
        }
    }

    return found;
}
/* ========================================================================== */
/**
 * @WBAMRENC_FillLCMLInitParamsEx() This function is used by the component thread to
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
OMX_ERRORTYPE WBAMRENC_FillLCMLInitParamsEx(OMX_HANDLETYPE pComponent) {
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nIpBuf, nIpBufSize, nOpBuf, nOpBufSize;
    OMX_BUFFERHEADERTYPE *pTemp;
    char *ptr;
    LCML_DSP_INTERFACE *pHandle = (LCML_DSP_INTERFACE *)pComponent;
    WBAMRENC_COMPONENT_PRIVATE *pComponentPrivate = pHandle->pComponentPrivate;
    WBAMRENC_LCML_BUFHEADERTYPE *pTemp_lcml = NULL;
    OMX_U16 i;
    OMX_U32 size_lcml;

    OMX_PRINT1(pComponentPrivate->dbg, "Entering\n");
    nIpBuf = pComponentPrivate->pInputBufferList->numBuffers;
    nIpBufSize = pComponentPrivate->pPortDef[WBAMRENC_INPUT_PORT]->nBufferSize;
    nOpBuf = pComponentPrivate->pOutputBufferList->numBuffers;
    nOpBufSize = pComponentPrivate->pPortDef[WBAMRENC_OUTPUT_PORT]->nBufferSize;
    OMX_PRBUFFER2(pComponentPrivate->dbg, "------ Buffer Details -----------\n");
    OMX_PRBUFFER2(pComponentPrivate->dbg, "Input  Buffer Count = %ld\n", nIpBuf);
    OMX_PRBUFFER2(pComponentPrivate->dbg, "Input  Buffer Size = %ld\n", nIpBufSize);
    OMX_PRBUFFER2(pComponentPrivate->dbg, "Output Buffer Count = %ld\n", nOpBuf);
    OMX_PRBUFFER2(pComponentPrivate->dbg, "Output Buffer Size = %ld\n", nOpBufSize);
    OMX_PRBUFFER2(pComponentPrivate->dbg, "------ Buffer Details ------------\n");
    /* Allocate memory for all input buffer headers..
     * This memory pointer will be sent to LCML */
    size_lcml = nIpBuf * sizeof(WBAMRENC_LCML_BUFHEADERTYPE);
    OMX_MALLOC_SIZE(pTemp_lcml, size_lcml, WBAMRENC_LCML_BUFHEADERTYPE);


    pComponentPrivate->pLcmlBufHeader[WBAMRENC_INPUT_PORT] = pTemp_lcml;

    for (i = 0; i < nIpBuf; i++) {
        OMX_PRCOMM2(pComponentPrivate->dbg, "INPUT--------- Inside Ip Loop\n");
        pTemp = pComponentPrivate->pInputBufferList->pBufHdr[i];
        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pTemp->nFilledLen = nIpBufSize;
        pTemp->nVersion.s.nVersionMajor = WBAMRENC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = WBAMRENC_MINOR_VER;
        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = WBAMRENC_NOT_USED;
        pTemp_lcml->buffer = pTemp;
        OMX_PRDSP2(pComponentPrivate->dbg, "pTemp_lcml->buffer->pBuffer = %p \n", pTemp_lcml->buffer->pBuffer);
        pTemp_lcml->eDir = OMX_DirInput;

        OMX_MALLOC_SIZE_DSPALIGN(pTemp_lcml->pBufferParam, sizeof(WBAMRENC_ParamStruct), OMX_U8);

        pTemp_lcml->pBufferParam->usNbFrames = 0;
        pTemp_lcml->pBufferParam->pParamElem = NULL;
        pTemp_lcml->pFrameParam = NULL;

        OMX_MALLOC_GENERIC(pTemp_lcml->pDmmBuf, DMM_BUFFER_OBJ);


        /* This means, it is not a last buffer. This flag is to be modified by
         * the application to indicate the last buffer */
        pTemp->nFlags = WBAMRENC_NORMAL_BUFFER;
        pTemp++;
        pTemp_lcml++;
    }

    /* Allocate memory for all output buffer headers..
     * This memory pointer will be sent to LCML */
    size_lcml = nOpBuf * sizeof(WBAMRENC_LCML_BUFHEADERTYPE);

    OMX_MALLOC_SIZE(pTemp_lcml, size_lcml, WBAMRENC_LCML_BUFHEADERTYPE);


    pComponentPrivate->pLcmlBufHeader[WBAMRENC_OUTPUT_PORT] = pTemp_lcml;

    for (i = 0; i < nOpBuf; i++) {
        OMX_PRCOMM2(pComponentPrivate->dbg, "OUTPUT--------- Inside Op Loop\n");
        pTemp = pComponentPrivate->pOutputBufferList->pBufHdr[i];
        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pTemp->nFilledLen = nOpBufSize;
        pTemp->nVersion.s.nVersionMajor = WBAMRENC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = WBAMRENC_MINOR_VER;
        pComponentPrivate->nVersion = pTemp->nVersion.nVersion;
        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = WBAMRENC_NOT_USED;
        pTemp_lcml->buffer = pTemp;
        OMX_PRBUFFER2(pComponentPrivate->dbg, "pTemp_lcml->buffer->pBuffer = %p \n", pTemp_lcml->buffer->pBuffer);
        pTemp_lcml->eDir = OMX_DirOutput;

        OMX_MALLOC_SIZE_DSPALIGN(pTemp_lcml->pBufferParam,
                                 sizeof(WBAMRENC_ParamStruct),
                                 WBAMRENC_ParamStruct);

        pTemp_lcml->pBufferParam->usNbFrames = 0;
        pTemp_lcml->pBufferParam->pParamElem = NULL;
        pTemp_lcml->pFrameParam = NULL;


        OMX_MALLOC_GENERIC(pTemp_lcml->pDmmBuf, DMM_BUFFER_OBJ);

        /* This means, it is not a last buffer. This flag is to be modified by
         * the application to indicate the last buffer */
        pTemp->nFlags = WBAMRENC_NORMAL_BUFFER;
        pTemp++;
        pTemp_lcml++;
    }

    pComponentPrivate->bInitParamsInitialized = 1;
EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, "Exiting\n");
    OMX_PRINT1(pComponentPrivate->dbg, "Returning = 0x%x\n", eError);
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
                         DMM_BUFFER_OBJ* pDmmBuf, struct OMX_TI_Debug dbg) {
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    DSP_STATUS status;
    int nSizeReserved = 0;

    if (pDmmBuf == NULL) {
        OMX_ERROR4 (dbg, "pBuf is NULL\n");
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    if (pArmPtr == NULL) {
        OMX_ERROR4 (dbg, "pBuf is NULL\n");
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    /* Allocate */
    pDmmBuf->pAllocated = pArmPtr;

    /* Reserve */
    nSizeReserved = ROUND_TO_PAGESIZE(size) + 2 * DMM_PAGE_SIZE ;
    status = DSPProcessor_ReserveMemory(ProcHandle, nSizeReserved, &(pDmmBuf->pReserved));

    if (DSP_FAILED(status)) {
        OMX_ERROR4 (dbg, "DSPProcessor_ReserveMemory() failed - error 0x%x", (int) status);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }

    pDmmBuf->nSize = size;
    OMX_PRBUFFER2 (dbg, "DMM MAP Reserved: %p, size 0x%x (%d)\n",
                   pDmmBuf->pReserved,
                   nSizeReserved,
                   nSizeReserved);
    /* Map */
    status = DSPProcessor_Map(ProcHandle,
                              pDmmBuf->pAllocated,/* Arm addres of data to Map on DSP*/
                              size , /* size to Map on DSP*/
                              pDmmBuf->pReserved, /* reserved space */
                              &(pDmmBuf->pMapped), /* returned map pointer */
                              0); /* final param is reserved.  set to zero. */

    if (DSP_FAILED(status)) {
        OMX_ERROR4 (dbg, "DSPProcessor_Map() failed - error 0x%x",
                    (int)status);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }

    OMX_PRBUFFER2 (dbg, "DMM Mapped: %p, size 0x%x (%d)\n",
                   pDmmBuf->pMapped, size, size);

    /* Issue an initial memory flush to ensure cache coherency */
    status = DSPProcessor_FlushMemory(ProcHandle, pDmmBuf->pAllocated, size, 0);

    if (DSP_FAILED(status)) {
        OMX_ERROR4 (dbg, "Unable to flush mapped buffer: error 0x%x",
                    (int)status);
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
OMX_ERRORTYPE OMX_DmmUnMap(DSP_HPROCESSOR ProcHandle, void* pMapPtr, void* pResPtr, struct OMX_TI_Debug dbg) {
    DSP_STATUS status = DSP_SOK;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    /*    printf("OMX UnReserve DSP: %p\n",pResPtr);*/

    if (pMapPtr == NULL) {
        OMX_ERROR4 (dbg, "pMapPtr is NULL\n");
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    if (pResPtr == NULL) {
        OMX_ERROR4 (dbg, "pResPtr is NULL\n");
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    status = DSPProcessor_UnMap(ProcHandle, pMapPtr);

    if (DSP_FAILED(status)) {
        OMX_ERROR4 (dbg, "DSPProcessor_UnMap() failed - error 0x%x", (int)status);
    }

    OMX_PRINT2 (dbg, "unreserving  structure =0x%p\n", pResPtr);
    status = DSPProcessor_UnReserveMemory(ProcHandle, pResPtr);

    if (DSP_FAILED(status)) {
        OMX_ERROR4(dbg, "DSPProcessor_UnReserveMemory() failed - error 0x%x", (int)status);
    }

EXIT:
    return eError;
}
#ifdef RESOURCE_MANAGER_ENABLED
void WBAMRENC_ResourceManagerCallback(RMPROXY_COMMANDDATATYPE cbData) {
    OMX_COMMANDTYPE Cmd = OMX_CommandStateSet;
    OMX_STATETYPE state = OMX_StateIdle;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)cbData.hComponent;
    WBAMRENC_COMPONENT_PRIVATE *pCompPrivate = NULL;

    pCompPrivate = (WBAMRENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    if (*(cbData.RM_Error) == OMX_RmProxyCallback_ResourcesPreempted) {
        if (pCompPrivate->curState == OMX_StateExecuting ||
                pCompPrivate->curState == OMX_StatePause) {
            write (pCompPrivate->cmdPipe[1], &Cmd, sizeof(Cmd));
            write (pCompPrivate->cmdDataPipe[1], &state , sizeof(OMX_U32));

            pCompPrivate->bPreempted = 1;
        }
    } else if (*(cbData.RM_Error) == OMX_RmProxyCallback_ResourcesAcquired) {
        pCompPrivate->cbInfo.EventHandler (
            pHandle, pHandle->pApplicationPrivate,
            OMX_EventResourcesAcquired, 0, 0,
            NULL);


    }

}
#endif

void WBAMRENC_HandleUSNError (WBAMRENC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U32 arg) {
    OMX_COMPONENTTYPE *pHandle = NULL;
    OMX_U8 pending_buffers = OMX_FALSE;
    OMX_U32 i;

    switch (arg) {
        case IUALG_WARN_CONCEALED:
        case IUALG_WARN_UNDERFLOW:
        case IUALG_WARN_OVERFLOW:
        case IUALG_WARN_ENDOFDATA:
            /* all of these are informative messages, Algo can recover, no need to notify the
             * IL Client at this stage of the implementation */
            break;

        case IUALG_WARN_PLAYCOMPLETED: {
            pHandle = pComponentPrivate->pHandle;
            OMX_PRDSP2(pComponentPrivate->dbg, "%d :: GOT MESSAGE IUALG_WARN_PLAYCOMPLETED\n", __LINE__);
            OMX_PRINT2(pComponentPrivate->dbg, "IUALG_WARN_PLAYCOMPLETED Received\n");

            if (pComponentPrivate->LastOutbuf != NULL) {
                pComponentPrivate->LastOutbuf->nFlags |= OMX_BUFFERFLAG_EOS;
            }

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
        case IUALG_ERR_GENERAL: {
            /* all of these are fatal messages, Algo can not recover
                     * hence return an error */
            pComponentPrivate->bIsInvalidState = OMX_TRUE;
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
OMX_ERRORTYPE AddStateTransition(WBAMRENC_COMPONENT_PRIVATE* pComponentPrivate) {

    OMX_ERRORTYPE eError = OMX_ErrorNone;
     
    if(pthread_mutex_lock(&pComponentPrivate->mutexStateChangeRequest)) {
       return OMX_ErrorUndefined;
    }

    /* Increment state change request reference count */
    pComponentPrivate->nPendingStateChangeRequests++;
    
    ALOGI("addstatetranstion: %ld @ %d", pComponentPrivate->nPendingStateChangeRequests, pComponentPrivate->curState);

    if(pthread_mutex_unlock(&pComponentPrivate->mutexStateChangeRequest)) {
       return OMX_ErrorUndefined;
    }

    return eError;
}

OMX_ERRORTYPE RemoveStateTransition(WBAMRENC_COMPONENT_PRIVATE* pComponentPrivate, OMX_BOOL bEnableSignal) {
    OMX_ERRORTYPE eError = OMX_ErrorNone;
     
     /* Decrement state change request reference count*/
    if(pthread_mutex_lock(&pComponentPrivate->mutexStateChangeRequest)) {
       return OMX_ErrorUndefined;
    }

    pComponentPrivate->nPendingStateChangeRequests--;
     
    ALOGI("removestatetranstion: %ld @ %d", pComponentPrivate->nPendingStateChangeRequests, pComponentPrivate->curState);
    /* If there are no more pending requests, signal the thread waiting on this*/
    if(!pComponentPrivate->nPendingStateChangeRequests && bEnableSignal) {
       pthread_cond_signal(&(pComponentPrivate->StateChangeCondition));
    }
 
    if(pthread_mutex_unlock(&pComponentPrivate->mutexStateChangeRequest)) {
       return OMX_ErrorUndefined;
    }
    
    return eError;
}

