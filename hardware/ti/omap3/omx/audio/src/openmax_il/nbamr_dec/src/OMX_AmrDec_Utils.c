
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
* @file OMX_AmrDec_Utils.c
*
* This file implements OMX Component for PCM decoder that
* is fully compliant with the OMX Audio specification.
*
* @path  $(CSLPATH)\
*
* @rev  0.1
*/
/* ----------------------------------------------------------------------------*/

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
#include "OMX_AmrDecoder.h"
#include "OMX_AmrDec_Utils.h"
#include "amrdecsocket_ti.h"
#include <decode_common_ti.h>
#include "OMX_AmrDec_ComponentThread.h"
#include "usn.h"
#include "LCML_DspCodec.h"

#ifdef RESOURCE_MANAGER_ENABLED
#include <ResourceManagerProxyAPI.h>
#endif

/* Log for Android system*/
#include <utils/Log.h>

/* ======================================================================= */
/**
 * @def    DASF    Defines the value for identify DASF ON
 */
/* ======================================================================= */
#define DASF 1

#ifdef UNDER_CE
void sleep(DWORD Duration)
{
    Sleep(Duration);

}
#endif

/* ========================================================================== */
/**
* @NBAMRDECFill_LCMLInitParams () This function is used by the component thread to
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
OMX_ERRORTYPE NBAMRDECFill_LCMLInitParams(OMX_HANDLETYPE pComponent,
                                  LCML_DSP *plcml_Init, OMX_U16 arr[])
{

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nIpBuf,nIpBufSize,nOpBuf,nOpBufSize;
    OMX_U16 i;
    OMX_BUFFERHEADERTYPE *pTemp;
    OMX_U32 size_lcml;
    LCML_STRMATTR *strmAttr = NULL;
    LCML_DSP_INTERFACE *pHandle = (LCML_DSP_INTERFACE *)pComponent;
    AMRDEC_COMPONENT_PRIVATE *pComponentPrivate;
    LCML_NBAMRDEC_BUFHEADERTYPE *pTemp_lcml;
    char *ptr;

    pComponentPrivate = pHandle->pComponentPrivate; 
    OMX_PRINT1 (pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: NBAMRDECFill_LCMLInitParams\n ",__LINE__);
    
    nIpBuf = pComponentPrivate->pInputBufferList->numBuffers;
    pComponentPrivate->nRuntimeInputBuffers = nIpBuf;
        
    if(pComponentPrivate->mimemode == 1) 
    {
        nIpBufSize = INPUT_NBAMRDEC_BUFFER_SIZE_MIME;
    }
    else if (pComponentPrivate->mimemode == 2)
         {
               nIpBufSize = INPUT_NBAMRDEC_BUFFER_SIZE_IF2;
         }
    else 
    {
         if (OMX_AUDIO_AMRDTXasEFR == pComponentPrivate->iAmrMode)
         {
                nIpBufSize = INPUT_BUFF_SIZE_EFR;
         } 
         else{
                 nIpBufSize = STD_NBAMRDEC_BUF_SIZE;              
         }
    }

    nOpBuf = pComponentPrivate->pOutputBufferList->numBuffers;
    pComponentPrivate->nRuntimeOutputBuffers = nOpBuf;

    nOpBufSize = OUTPUT_NBAMRDEC_BUFFER_SIZE;


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

    plcml_Init->NodeInfo.AllUUIDs[0].uuid = &AMRDECSOCKET_TI_UUID;

    strcpy ((char*)plcml_Init->NodeInfo.AllUUIDs[0].DllName,NBAMRDEC_DLL_NAME);

    plcml_Init->NodeInfo.AllUUIDs[0].eDllType = DLL_NODEOBJECT;

    plcml_Init->NodeInfo.AllUUIDs[1].uuid = &AMRDECSOCKET_TI_UUID;
    strcpy ((char*)plcml_Init->NodeInfo.AllUUIDs[1].DllName,NBAMRDEC_DLL_NAME);
    plcml_Init->NodeInfo.AllUUIDs[1].eDllType = DLL_DEPENDENT;

    plcml_Init->NodeInfo.AllUUIDs[2].uuid = &USN_TI_UUID;
    strcpy ((char*)plcml_Init->NodeInfo.AllUUIDs[2].DllName,NBAMRDEC_USN_DLL_NAME);
    plcml_Init->NodeInfo.AllUUIDs[2].eDllType = DLL_DEPENDENT;


    if(pComponentPrivate->dasfmode == 1) {
        OMX_PRDSP2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: pComponentPrivate->dasfmode = %d\n",__LINE__,pComponentPrivate->dasfmode);
        OMX_MALLOC_GENERIC(strmAttr, LCML_STRMATTR);
        
        pComponentPrivate->strmAttr = strmAttr;
        strmAttr->uSegid = 0;
        strmAttr->uAlignment = 0;
        strmAttr->uTimeout = NBAMRD_TIMEOUT;
        strmAttr->uBufsize = OUTPUT_NBAMRDEC_BUFFER_SIZE;
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
    plcml_Init->SegID = OMX_AMRDEC_DEFAULT_SEGMENT;
    plcml_Init->Timeout = OMX_AMRDEC_SN_TIMEOUT;
    plcml_Init->Alignment = 0;
    plcml_Init->Priority = OMX_AMRDEC_SN_PRIORITY;
    plcml_Init->ProfileID = -1;

    /* TODO: Set this using SetParameter() */
    pComponentPrivate->iAmrSamplingFrequeny = NBAMRDEC_SAMPLING_FREQUENCY;
 
    pComponentPrivate->iAmrChannels = pComponentPrivate->amrParams[NBAMRDEC_OUTPUT_PORT]->nChannels;

    pComponentPrivate->iAmrMode =
        pComponentPrivate->amrParams[NBAMRDEC_INPUT_PORT]->eAMRDTXMode;

    /*if(pComponentPrivate->mimemode == 1)
    {
       pComponentPrivate->iAmrMimeFlag = MODE_MIME;
    } else {
        pComponentPrivate->iAmrMimeFlag = MODE_NONMIME;
    }*/

    arr[0] = STREAM_COUNT;
    arr[1] = NBAMRDEC_INPUT_PORT;
    arr[2] = NBAMRDEC_DMM;
    OMX_PRINT2(pComponentPrivate->dbg, "%s: IN %d", __FUNCTION__, pComponentPrivate->pOutputBufferList->numBuffers);
    if (pComponentPrivate->pInputBufferList->numBuffers) {
        arr[3] = pComponentPrivate->pInputBufferList->numBuffers;
    }
    else {
        arr[3] = 1;
    }

    arr[4] = NBAMRDEC_OUTPUT_PORT;

    if(pComponentPrivate->dasfmode == 1) {
        OMX_PRDSP2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Setting up create phase params for DASF mode\n",__LINE__);
        arr[5] = NBAMRDEC_OUTSTRM;
        arr[6] = NUM_NBAMRDEC_OUTPUT_BUFFERS_DASF;
    }
    else {
        OMX_PRDSP2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Setting up create phase params for FILE mode\n",__LINE__);
        arr[5] = NBAMRDEC_DMM;
    OMX_PRINT2(pComponentPrivate->dbg, "%s: OUT : %d", __FUNCTION__, pComponentPrivate->pOutputBufferList->numBuffers);

        if (pComponentPrivate->pOutputBufferList->numBuffers) {
            arr[6] = pComponentPrivate->pOutputBufferList->numBuffers;
        }
        else {
            arr[6] = 2;
        }

    }

    if(pComponentPrivate->iAmrMode == OMX_AUDIO_AMRDTXasEFR) {
        arr[7] = NBAMRDEC_EFR;
    }
    else {
        arr[7] = NBAMR;
    }
    
    /*if(1 == pComponentPrivate->mimemode) {
        arr[8] = NBAMRDEC_MIMEMODE;
    } 
    else {
        arr[8] = NBAMRDEC_NONMIMEMODE;
    }
    */
    arr[8] = pComponentPrivate->mimemode; /*MIME, IF2 or FORMATCONFORMANCE*/
    arr[9] = END_OF_CR_PHASE_ARGS;

    plcml_Init->pCrPhArgs = arr;

    OMX_PRINT2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Comp: OMX_AmrDecUtils.c\n",__LINE__);
    size_lcml = nIpBuf * sizeof(LCML_NBAMRDEC_BUFHEADERTYPE);
    OMX_MALLOC_SIZE(ptr, size_lcml,char);
    pTemp_lcml = (LCML_NBAMRDEC_BUFHEADERTYPE *)ptr;
    pComponentPrivate->pLcmlBufHeader[NBAMRDEC_INPUT_PORT] = pTemp_lcml;

    for (i=0; i<nIpBuf; i++) {
        pTemp = pComponentPrivate->pInputBufferList->pBufHdr[i];
        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);
/*        pTemp->nAllocLen = nIpBufSize;*/
        pTemp->nFilledLen = nIpBufSize;
        pTemp->nVersion.s.nVersionMajor = AMRDEC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = AMRDEC_MINOR_VER;
        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = NOT_USED;
        pTemp_lcml->buffer = pTemp;
        pTemp_lcml->eDir = OMX_DirInput;

        OMX_MALLOC_SIZE_DSPALIGN(pTemp_lcml->pBufferParam,
                                sizeof(NBAMRDEC_ParamStruct),
                                NBAMRDEC_ParamStruct);

        pTemp_lcml->pBufferParam->usNbFrames =0;
        pTemp_lcml->pBufferParam->pParamElem = NULL;
        pTemp_lcml->pFrameParam = NULL;
        OMX_MALLOC_GENERIC(pTemp_lcml->pDmmBuf, DMM_BUFFER_OBJ);

        /* This means, it is not a last buffer. This flag is to be modified by
         * the application to indicate the last buffer */
        pTemp->nFlags = NORMAL_BUFFER;

        pTemp_lcml++;
    }

    /* Allocate memory for all output buffer headers..
       * This memory pointer will be sent to LCML */
    size_lcml = nOpBuf * sizeof(LCML_NBAMRDEC_BUFHEADERTYPE);
    OMX_MALLOC_SIZE(pTemp_lcml, size_lcml,LCML_NBAMRDEC_BUFHEADERTYPE);
    pComponentPrivate->pLcmlBufHeader[NBAMRDEC_OUTPUT_PORT] = pTemp_lcml;

    for (i=0; i<nOpBuf; i++) {
        pTemp = pComponentPrivate->pOutputBufferList->pBufHdr[i];
        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);
/*        pTemp->nAllocLen = nOpBufSize;*/
        pTemp->nFilledLen = nOpBufSize;
        pTemp->nVersion.s.nVersionMajor = AMRDEC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = AMRDEC_MINOR_VER;
        pComponentPrivate->nVersion = pTemp->nVersion.nVersion;
        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = NOT_USED;
        /* This means, it is not a last buffer. This flag is to be modified by
         * the application to indicate the last buffer */

        pTemp_lcml->buffer = pTemp;
        pTemp_lcml->eDir = OMX_DirOutput;
        pTemp_lcml->pFrameParam = NULL;
                                                                               
        OMX_MALLOC_SIZE_DSPALIGN(pTemp_lcml->pBufferParam,
                                sizeof(NBAMRDEC_ParamStruct),
                                NBAMRDEC_ParamStruct); 
        
        pTemp_lcml->pBufferParam->usNbFrames =0;
        pTemp_lcml->pBufferParam->pParamElem = NULL;        
        OMX_MALLOC_GENERIC(pTemp_lcml->pDmmBuf, DMM_BUFFER_OBJ);

        OMX_PRDSP2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c ::pTemp_lcml = %p\n",__LINE__,pTemp_lcml);
        OMX_PRDSP2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c ::pTemp_lcml->buffer = %p\n",__LINE__,pTemp_lcml->buffer);

        pTemp->nFlags = NORMAL_BUFFER;

        pTemp++;
        pTemp_lcml++;
    }
    pComponentPrivate->bPortDefsAllocated = 1;
#ifdef __PERF_INSTRUMENTATION__
    pComponentPrivate->nLcml_nCntIp = 0;
    pComponentPrivate->nLcml_nCntOpReceived = 0;
#endif    

    OMX_PRINT1(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Exiting NBAMRDECFill_LCMLInitParams\n",__LINE__);

    pComponentPrivate->bInitParamsInitialized = 1;
EXIT:

    return eError;
}


/* ========================================================================== */
/**
* @NBAMRDEC_StartComponentThread() This function is called by the component to create
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
OMX_ERRORTYPE NBAMRDEC_StartComponentThread(OMX_HANDLETYPE pComponent)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    AMRDEC_COMPONENT_PRIVATE *pComponentPrivate =
                        (AMRDEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
#ifdef UNDER_CE        
    pthread_attr_t attr;
    memset(&attr, 0, sizeof(attr));
    attr.__inheritsched = PTHREAD_EXPLICIT_SCHED;
    attr.__schedparam.__sched_priority = OMX_AUDIO_DECODER_THREAD_PRIORITY;
#endif

    OMX_PRINT1(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Inside  NBAMRDEC_StartComponentThread\n", __LINE__);

    /* Initialize all the variables*/
    pComponentPrivate->bIsStopping = 0;
    pComponentPrivate->lcml_nOpBuf = 0;
    pComponentPrivate->lcml_nIpBuf = 0;
    pComponentPrivate->app_nBuf = 0;
    pComponentPrivate->num_Reclaimed_Op_Buff = 0;
    pComponentPrivate->first_output_buf_rcv = 0;

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
/*    eError = pipe (pComponentPrivate->lcml_Pipe);
    if (eError) {
       eError = OMX_ErrorInsufficientResources;
       goto EXIT;
    }
*/
    /* Create the Component Thread */
#ifdef UNDER_CE        
    eError = pthread_create (&(pComponentPrivate->ComponentThread), &attr, NBAMRDEC_ComponentThread, pComponentPrivate);
#else    
    eError = pthread_create (&(pComponentPrivate->ComponentThread), NULL, NBAMRDEC_ComponentThread, pComponentPrivate);
#endif
    if (eError || !pComponentPrivate->ComponentThread) {
       eError = OMX_ErrorInsufficientResources;
       goto EXIT;
    }
    pComponentPrivate->bCompThreadStarted = 1;
EXIT:
    return eError;
}

/* ========================================================================== */
/**
* @Mp3Dec_FreeCompResources() This function is called by the component during
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

OMX_ERRORTYPE NBAMRDEC_FreeCompResources(OMX_HANDLETYPE pComponent)
{
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    AMRDEC_COMPONENT_PRIVATE *pComponentPrivate = (AMRDEC_COMPONENT_PRIVATE *)
                                                     pHandle->pComponentPrivate;

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_ERRORTYPE err = OMX_ErrorNone;
    OMX_U32 nIpBuf = 0;
    OMX_U32 nOpBuf = 0;

    OMX_PRINT1(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: NBAMRDEC_FreeCompResources\n", __LINE__);

    if (pComponentPrivate->bPortDefsAllocated) {
        nIpBuf = pComponentPrivate->pPortDef[NBAMRDEC_INPUT_PORT]->nBufferCountActual;
        nOpBuf = pComponentPrivate->pPortDef[NBAMRDEC_OUTPUT_PORT]->nBufferCountActual;
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

/*        err = close (pComponentPrivate->lcml_Pipe[0]);
        if (0 != err && OMX_ErrorNone == eError) {
            eError = OMX_ErrorHardware;
        }

        err = close (pComponentPrivate->lcml_Pipe[1]);
        if (0 != err && OMX_ErrorNone == eError) {
            eError = OMX_ErrorHardware;
        } */
        
    }
    OMX_MEMFREE_STRUCT(pComponentPrivate->pPriorityMgmt);
    OMX_MEMFREE_STRUCT(pComponentPrivate->pHoldBuffer);
    OMX_MEMFREE_STRUCT(pComponentPrivate->pPortDef[NBAMRDEC_INPUT_PORT]);
    OMX_MEMFREE_STRUCT(pComponentPrivate->pPortDef[NBAMRDEC_OUTPUT_PORT]);
    OMX_MEMFREE_STRUCT(pComponentPrivate->amrParams[NBAMRDEC_INPUT_PORT]);
    OMX_MEMFREE_STRUCT(pComponentPrivate->amrParams[NBAMRDEC_OUTPUT_PORT]);
        
    pComponentPrivate->bPortDefsAllocated = 0;
    /* Removing sleep() calls. */
#ifndef UNDER_CE
    OMX_PRDSP2(pComponentPrivate->dbg, "\n\n FreeCompResources: Destroying mutexes.\n\n");
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
    /* Removing sleep() calls. */
    return eError;
}



OMX_ERRORTYPE NBAMRDEC_CleanupInitParams(OMX_HANDLETYPE pComponent)
{
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    AMRDEC_COMPONENT_PRIVATE *pComponentPrivate = (AMRDEC_COMPONENT_PRIVATE *)
                                                     pHandle->pComponentPrivate;

    LCML_NBAMRDEC_BUFHEADERTYPE *pTemp_lcml;

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nIpBuf = 0;
    OMX_U16 i=0;
    LCML_DSP_INTERFACE *pLcmlHandle;
    LCML_DSP_INTERFACE *pLcmlHandleAux;
    
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: NBAMRDEC_CleanupInitParams()\n", __LINE__);

    OMX_MEMFREE_STRUCT(pComponentPrivate->strmAttr);

    OMX_MEMFREE_STRUCT_DSPALIGN(pComponentPrivate->pParams,AMRDEC_AudioCodecParams);
          
    nIpBuf = pComponentPrivate->nRuntimeInputBuffers;
    pTemp_lcml = pComponentPrivate->pLcmlBufHeader[NBAMRDEC_INPUT_PORT];
    for(i=0; i<nIpBuf; i++) {
        if(pTemp_lcml->pFrameParam!=NULL){ 
            OMX_MEMFREE_STRUCT_DSPALIGN(pTemp_lcml->pFrameParam, NBAMRDEC_FrameStruct);
            pLcmlHandle = (LCML_DSP_INTERFACE *)pComponentPrivate->pLcmlHandle;             
            pLcmlHandleAux = (LCML_DSP_INTERFACE *)(((LCML_CODEC_INTERFACE *)pLcmlHandle->pCodecinterfacehandle)->pCodec);
            OMX_DmmUnMap(pLcmlHandleAux->dspCodec->hProc,
                               (void*)pTemp_lcml->pBufferParam->pParamElem,
                               pTemp_lcml->pDmmBuf->pReserved, pComponentPrivate->dbg);
        }
        OMX_MEMFREE_STRUCT_DSPALIGN(pTemp_lcml->pBufferParam, NBAMRDEC_ParamStruct);
        OMX_MEMFREE_STRUCT(pTemp_lcml->pDmmBuf);
        pTemp_lcml++;
    }

    pTemp_lcml = pComponentPrivate->pLcmlBufHeader[NBAMRDEC_OUTPUT_PORT];
    for(i=0; i<pComponentPrivate->nRuntimeOutputBuffers; i++){
        if(pTemp_lcml->pFrameParam!=NULL){
               OMX_MEMFREE_STRUCT_DSPALIGN(pTemp_lcml->pFrameParam, NBAMRDEC_FrameStruct);
               pLcmlHandle = (LCML_DSP_INTERFACE *)pComponentPrivate->pLcmlHandle;
               pLcmlHandleAux = (LCML_DSP_INTERFACE *)(((LCML_CODEC_INTERFACE *)pLcmlHandle->pCodecinterfacehandle)->pCodec);
               OMX_DmmUnMap(pLcmlHandleAux->dspCodec->hProc,
                              (void*)pTemp_lcml->pBufferParam->pParamElem,
                               pTemp_lcml->pDmmBuf->pReserved, pComponentPrivate->dbg);
        }
        OMX_MEMFREE_STRUCT_DSPALIGN(pTemp_lcml->pBufferParam, NBAMRDEC_ParamStruct);
        OMX_MEMFREE_STRUCT(pTemp_lcml->pDmmBuf);
        pTemp_lcml++;
    }    
    OMX_MEMFREE_STRUCT(pComponentPrivate->pLcmlBufHeader[NBAMRDEC_INPUT_PORT]);
    OMX_MEMFREE_STRUCT(pComponentPrivate->pLcmlBufHeader[NBAMRDEC_OUTPUT_PORT]);
    return eError;
}

/* ========================================================================== */
/**
* @NBAMRDEC_StopComponentThread() This function is called by the component during
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

OMX_ERRORTYPE NBAMRDEC_StopComponentThread(OMX_HANDLETYPE pComponent)
{
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    AMRDEC_COMPONENT_PRIVATE *pComponentPrivate = (AMRDEC_COMPONENT_PRIVATE *)
                                                     pHandle->pComponentPrivate;

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_ERRORTYPE threadError = OMX_ErrorNone;
    OMX_S16 pthreadError = 0;

    /*Join the component thread */
    pComponentPrivate->bIsStopping = 1;
    write (pComponentPrivate->cmdPipe[1], &pComponentPrivate->bIsStopping, sizeof(OMX_U16));
    pthreadError = pthread_join (pComponentPrivate->ComponentThread,
                                 (void*)&threadError);
    if (0 != pthreadError) {
        eError = OMX_ErrorHardware;
    }

    /*Check for the errors */
    if (OMX_ErrorNone != threadError && OMX_ErrorNone != eError) {
        eError = OMX_ErrorInsufficientResources;
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Error while closing Component Thread\n",__LINE__);
    }
   return eError;
}


/* ========================================================================== */
/**
* @NBAMRDECHandleCommand() This function is called by the component when ever it
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

OMX_U32 NBAMRDECHandleCommand (AMRDEC_COMPONENT_PRIVATE *pComponentPrivate)
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
    OMX_STRING p = "hello";
    LCML_CALLBACKTYPE cb;
    LCML_DSP *pLcmlDsp;
    AMRDEC_AudioCodecParams *pParams;
    ssize_t ret;
    LCML_NBAMRDEC_BUFHEADERTYPE *pLcmlHdr = NULL;
    int inputPortFlag=0,outputPortFlag=0;
#ifdef RESOURCE_MANAGER_ENABLED
    OMX_ERRORTYPE rm_error;
#endif

    pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;

    ret = read (pComponentPrivate->cmdPipe[0], &command, sizeof (command));
    if (ret == -1) {
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Error While reading from the Pipe\n",__LINE__);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }

    ret = read (pComponentPrivate->cmdDataPipe[0], &commandData, sizeof (commandData));
    if (ret == -1) {
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Error While reading from the Pipe\n",__LINE__);
        eError = OMX_ErrorHardware;
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
        while (1) {
            OMX_PRCOMM2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: In while loop OMX_StateLoaded [NBAMRDEC_INPUT_PORT]->bPopulated  %d \n",__LINE__,pComponentPrivate->pPortDef[NBAMRDEC_INPUT_PORT]->bPopulated);
            OMX_PRCOMM2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: In while loop OMX_StateLoaded [NBAMRDEC_INPUT_PORT]->bEnabled    %d \n",__LINE__,pComponentPrivate->pPortDef[NBAMRDEC_INPUT_PORT]->bEnabled);
            OMX_PRCOMM2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: In while loop OMX_StateLoaded [NBAMRDEC_OUTPUT_PORT]->bPopulated %d \n",__LINE__,pComponentPrivate->pPortDef[NBAMRDEC_OUTPUT_PORT]->bPopulated);
            OMX_PRCOMM2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: In while loop OMX_StateLoaded [NBAMRDEC_OUTPUT_PORT]->bEnabled   %d \n",__LINE__,pComponentPrivate->pPortDef[NBAMRDEC_OUTPUT_PORT]->bEnabled);
#ifdef __PERF_INSTRUMENTATION__
            PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryStart | PERF_BoundarySetup);
#endif

            if (pComponentPrivate->pPortDef[NBAMRDEC_INPUT_PORT]->bPopulated &&  pComponentPrivate->pPortDef[NBAMRDEC_INPUT_PORT]->bEnabled)  {
                inputPortFlag = 1;
            }

            if (!pComponentPrivate->pPortDef[NBAMRDEC_INPUT_PORT]->bPopulated && !pComponentPrivate->pPortDef[NBAMRDEC_INPUT_PORT]->bEnabled) {
                inputPortFlag = 1;
            }

            if (pComponentPrivate->pPortDef[NBAMRDEC_OUTPUT_PORT]->bPopulated && pComponentPrivate->pPortDef[NBAMRDEC_OUTPUT_PORT]->bEnabled) {
                outputPortFlag = 1;
            }

            if (!pComponentPrivate->pPortDef[NBAMRDEC_OUTPUT_PORT]->bPopulated && !pComponentPrivate->pPortDef[NBAMRDEC_OUTPUT_PORT]->bEnabled) {
                outputPortFlag = 1;
            }

            if (inputPortFlag && outputPortFlag) {
                break;
            }
            pComponentPrivate->InLoaded_readytoidle = 1;
#ifndef UNDER_CE
            pthread_mutex_lock(&pComponentPrivate->InLoaded_mutex); 
            pthread_cond_wait(&pComponentPrivate->InLoaded_threshold, &pComponentPrivate->InLoaded_mutex);
            pthread_mutex_unlock(&pComponentPrivate->InLoaded_mutex);
            break; 
#else
            OMX_WaitForEvent(&(pComponentPrivate->InLoaded_event));
#endif
            break; 
        }
 
                OMX_PRCOMM1(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Inside NBAMRDECHandleCommand\n",__LINE__);
                cb.LCML_Callback = (void *) NBAMRDECLCML_Callback;
                pLcmlHandle = (OMX_HANDLETYPE) NBAMRDECGetLCMLHandle(pComponentPrivate);
                OMX_PRCOMM1(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Inside NBAMRDECHandleCommand\n",__LINE__);

                if (pLcmlHandle == NULL) {
                    OMX_PRDSP2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: LCML Handle is NULL........exiting..\n",__LINE__);
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
                OMX_PRCOMM1(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: NBAMRDECHandleCommand\n",__LINE__);
                OMX_PRDSP2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: pLcmlHandle = %p\n",__LINE__,pLcmlHandle);

                /* Got handle of dsp via phandle filling information about DSP
                 specific things */
                pLcmlDsp = (((LCML_DSP_INTERFACE*)pLcmlHandle)->dspCodec);
                OMX_PRDSP2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: pLcmlDsp = %p\n",__LINE__,pLcmlDsp);

                OMX_PRCOMM1(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: NBAMRDECHandleCommand\n",__LINE__);
                eError = NBAMRDECFill_LCMLInitParams(pHandle, pLcmlDsp, arr);
                if(eError != OMX_ErrorNone) {
                    OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Error returned from\
                                    NBAMRDECFill_LCMLInitParams()\n",__LINE__);
                    pComponentPrivate->curState = OMX_StateInvalid;
                    pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                            pHandle->pApplicationPrivate,
                                                            OMX_EventError, 
                                                            eError,
                                                            OMX_TI_ErrorSevere, 
                                                            NULL);
                    goto EXIT;
                }
                OMX_PRINT2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Comp: OMX_AmrDecUtils.c\n",__LINE__);
                pComponentPrivate->pLcmlHandle = (LCML_DSP_INTERFACE *)pLcmlHandle;
                /*filling create phase params */
                cb.LCML_Callback = (void *) NBAMRDECLCML_Callback;
                OMX_PRDSP1(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Calling LCML_InitMMCodec...\n",__LINE__);

#ifndef UNDER_CE
                    /* TeeDN will be default for decoder component */
                    OMX_PRINT2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: NBAMR decoder support TeeDN\n",__LINE__);
                                        
                    eError = LCML_InitMMCodecEx(((LCML_DSP_INTERFACE *)pLcmlHandle)->pCodecinterfacehandle,
                                          p,&pLcmlHandle,(void *)p,&cb, (OMX_STRING)pComponentPrivate->sDeviceString);
#else

                    eError = LCML_InitMMCodec(((LCML_DSP_INTERFACE *)pLcmlHandle)->pCodecinterfacehandle,
                                          p,&pLcmlHandle,(void *)p,&cb);

#endif
            
                if(eError != OMX_ErrorNone) {
                    OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Error returned from\
                        LCML_Init()\n",__LINE__);
                    /* send an event to client */
                    /* client should unload the component if the codec is not able to load */
                    pComponentPrivate->cbInfo.EventHandler (pHandle, 
                                                pHandle->pApplicationPrivate,
                                                OMX_EventError, 
                                                eError,
                                                OMX_TI_ErrorSevere,
                                                NULL);
                    goto EXIT;
                }

                OMX_PRSTATE2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Setting to OMX_StateIdle\n",__LINE__);
                

#ifdef RESOURCE_MANAGER_ENABLED
        /* need check the resource with RM */
                pComponentPrivate->rmproxyCallback.RMPROXY_Callback = (void *) NBAMR_ResourceManagerCallback;
                if (pComponentPrivate->curState != OMX_StateWaitForResources) {
                    rm_error = RMProxy_NewSendCommand(pHandle, 
                                                    RMProxy_RequestResource, 
                                                    OMX_NBAMR_Decoder_COMPONENT, 
                                                    NBAMRDEC_CPU_LOAD, 
                                                    3456,
                                                    &(pComponentPrivate->rmproxyCallback));
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
                                                        OMX_NBAMR_Decoder_COMPONENT, 
                                                        OMX_StateIdle, 
                                                        3456,
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
                        OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: AMRDEC: Error - insufficient resources\n", __LINE__);
                    }
                }
                else {
                    pComponentPrivate->curState = OMX_StateIdle;
                    pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                            pHandle->pApplicationPrivate,
                                                            OMX_EventCmdComplete, 
                                                            OMX_CommandStateSet,
                                                            pComponentPrivate->curState, 
                                                            NULL);
                    rm_error = RMProxy_NewSendCommand(pHandle, 
                                                        RMProxy_StateSet, 
                                                        OMX_NBAMR_Decoder_COMPONENT, 
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
                OMX_PRSTATE2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: AMRDEC: State has been Set to Idle\n",
                                                                     __LINE__);
                if(pComponentPrivate->dasfmode == 1) {
                    OMX_U32 pValues[4];
                    OMX_PRDSP2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: ---- Comp: DASF Functionality is ON ---\n",__LINE__);
                    
                    if(pComponentPrivate->streamID == 0) 
                    { 
                        OMX_ERROR4(pComponentPrivate->dbg, "**************************************\n");
                        OMX_ERROR4(pComponentPrivate->dbg, ":: Error = OMX_ErrorInsufficientResources\n");
                        OMX_ERROR4(pComponentPrivate->dbg, "**************************************\n");
                        eError = OMX_ErrorInsufficientResources; 
                        pComponentPrivate->curState = OMX_StateInvalid; 
                        pComponentPrivate->cbInfo.EventHandler( pHandle, 
                                                                pHandle->pApplicationPrivate, 
                                                                OMX_EventError, 
                                                                OMX_ErrorInvalidState,
                                                                OMX_TI_ErrorMajor, 
                                                        "AM: No Stream ID Available");                 
                        goto EXIT; 
                    } 
            
                    OMX_MALLOC_SIZE_DSPALIGN(pComponentPrivate->pParams, sizeof(AMRDEC_AudioCodecParams), AMRDEC_AudioCodecParams);
                                    
                    pParams = pComponentPrivate->pParams;
                    pParams->iAudioFormat = 1;
                    pParams->iSamplingRate = 8000;
                    pParams->iStrmId = pComponentPrivate->streamID;

                    pValues[0] = USN_STRMCMD_SETCODECPARAMS;
                    pValues[1] = (OMX_U32)pParams;
                    pValues[2] = sizeof(AMRDEC_AudioCodecParams);
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                          EMMCodecControlStrmCtrl,(void *)pValues);

                    if(eError != OMX_ErrorNone) {
                        OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Error Occurred in Codec StreamControl..\n",__LINE__);
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
        else if (pComponentPrivate->curState == OMX_StateExecuting) 
        {
                OMX_U8 *pArgs = (void*)"damedesuStr";
#ifdef __PERF_INSTRUMENTATION__
                PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryComplete | PERF_BoundarySteadyState);
#endif            
                /*Set the bIsStopping bit */
                   OMX_PRDSP2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: AMRDEC: About to set bIsStopping bit\n", __LINE__);

                OMX_PRSTATE2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: About to call LCML_ControlCodec(STOP)\n",__LINE__);
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
                pComponentPrivate->bStopSent=1;
                if (pComponentPrivate->pHoldBuffer) {
                    OMX_MEMFREE_STRUCT(pComponentPrivate->pHoldBuffer);
                }
                pComponentPrivate->nHoldLength = 0;

                if(eError != OMX_ErrorNone) {
                    OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Error Occurred in Codec Stop..\n",
                                                                      __LINE__);
                    pComponentPrivate->curState = OMX_StateInvalid;
                    pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                            pHandle->pApplicationPrivate,
                                                            OMX_EventError, 
                                                            eError,
                                                            OMX_TI_ErrorSevere, 
                                                            NULL);  
                    goto EXIT;
                }
               /* pComponentPrivate->bStopSent=1;
                OMX_MEMFREE_STRUCT(pComponentPrivate->pHoldBuffer);
                pComponentPrivate->nHoldLength = 0;*/
            } 
            else if(pComponentPrivate->curState == OMX_StatePause) {
                OMX_U8 *pArgs = (void*)"damedesuStr";
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

#ifdef __PERF_INSTRUMENTATION__
                PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryComplete | PERF_BoundarySteadyState);
#endif                
                pComponentPrivate->curState = OMX_StateIdle;
#ifdef RESOURCE_MANAGER_ENABLED
                rm_error = RMProxy_NewSendCommand(pHandle, RMProxy_StateSet, OMX_NBAMR_Decoder_COMPONENT, OMX_StateIdle, 3456,NULL);
#endif

                OMX_PRINT2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: The component is stopped\n",__LINE__);
                pComponentPrivate->cbInfo.EventHandler (
                                 pHandle,pHandle->pApplicationPrivate,
                                 OMX_EventCmdComplete,OMX_CommandStateSet,pComponentPrivate->curState,
                                 NULL);
            } 
            else {
                /* This means, it is invalid state from application */
                OMX_PRINT2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Comp: OMX_AmrDecUtils.c\n",__LINE__);
                pComponentPrivate->cbInfo.EventHandler(
                            pHandle,
                            pHandle->pApplicationPrivate,
                            OMX_EventError,
                            OMX_ErrorIncorrectStateTransition,
                            OMX_TI_ErrorMinor,
                            "Invalid State");
            }
            break;

        case OMX_StateExecuting:
            OMX_PRINT2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: NBAMRDECHandleCommand: Cmd Executing \n",__LINE__);            
            if (pComponentPrivate->curState == commandedState){
                pComponentPrivate->cbInfo.EventHandler (pHandle, 
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventError, 
                                                        OMX_ErrorSameState,
                                                        OMX_TI_ErrorMinor,
                                                        "Invalid State");
                OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Error: Same State Given by \
                           Application\n",__LINE__);
                    goto EXIT;                            
            }
            else if (pComponentPrivate->curState == OMX_StateIdle) {
                OMX_PRINT2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Comp: OMX_AmrDecUtils.c\n",__LINE__);
                /* Sending commands to DSP via LCML_ControlCodec third argument
                is not used for time being */
                pComponentPrivate->nFillBufferDoneCount = 0;  
                pComponentPrivate->bStopSent=0;

                pComponentPrivate->nEmptyBufferDoneCount = 0;  
                pComponentPrivate->nEmptyThisBufferCount = 0;
                pComponentPrivate->nFillBufferDoneCount =0;
 
                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                EMMCodecControlStart, (void *)p);

                if(eError != OMX_ErrorNone) {
                    OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Error Occurred in Codec Start..\n",__LINE__);
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
                

                OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: nBuf =  %ld\n",__LINE__,nBuf);
                /* Send output buffers to codec */
            }
            else if (pComponentPrivate->curState == OMX_StatePause) {
                OMX_PRINT2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Comp: OMX_AmrDecUtils.c\n",__LINE__);
                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                EMMCodecControlStart, (void *)p);
                if (eError != OMX_ErrorNone) {
                    OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Error While Resuming the codec\n",__LINE__);
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
                        NBAMRDECGetCorresponding_LCMLHeader(pComponentPrivate,pComponentPrivate->pInputBufHdrPending[i]->pBuffer, OMX_DirInput, &pLcmlHdr);
                        NBAMRDEC_SetPending(pComponentPrivate,pComponentPrivate->pInputBufHdrPending[i],OMX_DirInput);

                        eError = LCML_QueueBuffer(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                       EMMCodecInputBuffer,  
                                        pComponentPrivate->pInputBufHdrPending[i]->pBuffer,
                                        pComponentPrivate->pInputBufHdrPending[i]->nAllocLen,
                                        pComponentPrivate->pInputBufHdrPending[i]->nFilledLen,
                                       (OMX_U8 *) pLcmlHdr->pBufferParam,
                                       sizeof(NBAMRDEC_ParamStruct),
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
                        NBAMRDECGetCorresponding_LCMLHeader(pComponentPrivate,pComponentPrivate->pOutputBufHdrPending[i]->pBuffer, OMX_DirOutput, &pLcmlHdr);
                        NBAMRDEC_SetPending(pComponentPrivate,pComponentPrivate->pOutputBufHdrPending[i],OMX_DirOutput);

                        eError = LCML_QueueBuffer(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                       EMMCodecOuputBuffer,  
                                        pComponentPrivate->pOutputBufHdrPending[i]->pBuffer,
                                        pComponentPrivate->pOutputBufHdrPending[i]->nAllocLen,
                                        pComponentPrivate->pOutputBufHdrPending[i]->nFilledLen,
                                       (OMX_U8 *) pLcmlHdr->pBufferParam,
                                       sizeof(NBAMRDEC_ParamStruct),
                                       NULL);
                    }
                    
                }
                pComponentPrivate->nNumOutputBufPending = 0;
            }
            else {
                OMX_PRINT2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Comp: OMX_AmrDecUtils.c\n",__LINE__);
                pComponentPrivate->cbInfo.EventHandler (pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventError, 
                                                        OMX_ErrorIncorrectStateTransition,
                                                        OMX_TI_ErrorMinor,
                                                        "Incorrect State Transition");
                OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Error: Invalid State Given by \
                       Application\n",__LINE__);
                goto EXIT;

            }
#ifdef RESOURCE_MANAGER_ENABLED
             rm_error = RMProxy_NewSendCommand(pHandle, RMProxy_StateSet, OMX_NBAMR_Decoder_COMPONENT, OMX_StateExecuting, 3456,NULL);
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
           OMX_PRSTATE2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: NBAMRDECHandleCommand: Cmd Loaded - curState = %d\n",__LINE__,pComponentPrivate->curState);
                if (pComponentPrivate->curState == commandedState){
                    pComponentPrivate->cbInfo.EventHandler (pHandle, 
                                                            pHandle->pApplicationPrivate,
                                                            OMX_EventError, 
                                                            OMX_ErrorSameState,
                                                            OMX_TI_ErrorMinor,
                                                            "Same State");
                    OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Error: Same State Given by \
                           Application\n",__LINE__);
                   break;
                   }
                OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: pComponentPrivate->pInputBufferList->numBuffers = %d\n",__LINE__,pComponentPrivate->pInputBufferList->numBuffers);
                OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: pComponentPrivate->pOutputBufferList->numBuffers = %d\n",__LINE__,pComponentPrivate->pOutputBufferList->numBuffers);

               if (pComponentPrivate->curState == OMX_StateWaitForResources){
           OMX_PRDSP2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: NBAMRDECHandleCommand: Cmd Loaded\n",__LINE__);
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
            OMX_PRSTATE2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: In side OMX_StateLoaded State: \n",__LINE__);
           if (pComponentPrivate->curState != OMX_StateIdle &&
           pComponentPrivate->curState != OMX_StateWaitForResources) {
                OMX_PRINT2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Comp: OMX_AmrDecUtils.c\n",__LINE__);
                pComponentPrivate->cbInfo.EventHandler (pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventError, 
                                                        OMX_ErrorIncorrectStateTransition,
                                                        OMX_TI_ErrorMinor,
                                                        "Incorrect State Transition");
                OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Error: Invalid State Given by \
                       Application\n",__LINE__);
                goto EXIT;
           }
#ifdef __PERF_INSTRUMENTATION__
            PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryStart | PERF_BoundaryCleanup);
#endif            
            while (1) {

                
                if (!pComponentPrivate->pInputBufferList->numBuffers &&
                    !pComponentPrivate->pOutputBufferList->numBuffers) {

                        break;
                    }
                    pComponentPrivate->InIdle_goingtoloaded = 1;
#ifndef UNDER_CE
                    pthread_mutex_lock(&pComponentPrivate->InIdle_mutex); 
                    pthread_cond_wait(&pComponentPrivate->InIdle_threshold, &pComponentPrivate->InIdle_mutex);
                    pthread_mutex_unlock(&pComponentPrivate->InIdle_mutex);
#else
                    OMX_WaitForEvent(&(pComponentPrivate->InIdle_event));
#endif
                    break;
            }

           /* Now Deinitialize the component No error should be returned from
            * this function. It should clean the system as much as possible */
            OMX_PRSTATE2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: In side OMX_StateLoaded State: \n",__LINE__);
            NBAMRDEC_CleanupInitParams(pComponentPrivate->pHandle);
            
            eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                        EMMCodecControlDestroy, (void *)p);      

    /*Closing LCML Lib*/
    if (pComponentPrivate->ptrLibLCML != NULL)
    {
        OMX_PRDSP1(pComponentPrivate->dbg, "%d OMX_AmrDecoder.c Closing LCML library\n",__LINE__);
        dlclose( pComponentPrivate->ptrLibLCML  );
        pComponentPrivate->ptrLibLCML = NULL;
    }
                                        
#ifdef __PERF_INSTRUMENTATION__
            PERF_SendingCommand(pComponentPrivate->pPERF, -1, 0, PERF_ModuleComponent);
#endif                                  
            OMX_PRSTATE2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: In side OMX_StateLoaded State: \n",__LINE__);
            if (eError != OMX_ErrorNone) {
                OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Error: in Destroying the codec: no.  %x\n",__LINE__, eError);
                goto EXIT;
            }
           OMX_PRDSP2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: NBAMRDECHandleCommand: Cmd Loaded\n",__LINE__);
           eError = EXIT_COMPONENT_THRD;
           pComponentPrivate->bInitParamsInitialized = 0;
           pComponentPrivate->bLoadedCommandPending = OMX_FALSE;
           /* Send StateChangeNotification to application */
           break;

        case OMX_StatePause:
           OMX_PRSTATE2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: NBAMRDECHandleCommand: Cmd Pause\n",__LINE__);
                if (pComponentPrivate->curState == commandedState){
                    pComponentPrivate->cbInfo.EventHandler (pHandle, 
                                                            pHandle->pApplicationPrivate,
                                                            OMX_EventError, 
                                                            OMX_ErrorSameState,
                                                            OMX_TI_ErrorMinor,
                                                            "Same State");
                   break;
                   }
           if (pComponentPrivate->curState != OMX_StateExecuting &&
           pComponentPrivate->curState != OMX_StateIdle) {
                OMX_PRINT2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Comp: OMX_AmrDecUtils.c\n",__LINE__);
                pComponentPrivate->cbInfo.EventHandler (pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventError, 
                                                        OMX_ErrorIncorrectStateTransition,
                                                        OMX_TI_ErrorMinor,
                                                        "Incorrect State Transition");
                OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Error: Invalid State Given by \
                       Application\n",__LINE__);
                goto EXIT;
           }
#ifdef __PERF_INSTRUMENTATION__
           PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryComplete | PERF_BoundarySteadyState);
#endif
           eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                        EMMCodecControlPause, (void *)p);

           if (eError != OMX_ErrorNone) {
               OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Error: in Pausing the codec\n",__LINE__);
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
/* notify RM of pause so resources can be redistributed if needed */
           rm_error = RMProxy_NewSendCommand(pHandle,
                                             RMProxy_StateSet,
                                             OMX_NBAMR_Decoder_COMPONENT,
                                             OMX_StatePause,
                                             3456,
                                             NULL);
#endif
                OMX_PRINT2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Comp: OMX_AmrDecUtils.c\n",__LINE__);
           break;

         case OMX_StateWaitForResources:

                if (pComponentPrivate->curState == commandedState){
                OMX_PRINT2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Comp: OMX_AmrDecUtils.c\n",__LINE__);
                    pComponentPrivate->cbInfo.EventHandler (pHandle, 
                                                            pHandle->pApplicationPrivate,
                                                            OMX_EventError, 
                                                            OMX_ErrorSameState,
                                                            OMX_TI_ErrorMinor,
                                                        "Same State");
                    OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Error: Same State Given by \
                           Application\n",__LINE__);
                   }
          else if (pComponentPrivate->curState == OMX_StateLoaded) {
#ifdef RESOURCE_MANAGER_ENABLED         
            rm_error = RMProxy_NewSendCommand(pHandle,
                                            RMProxy_StateSet, 
                                            OMX_NBAMR_Decoder_COMPONENT, 
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
                OMX_PRINT2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Comp: OMX_AmrDecUtils.c\n",__LINE__);
                pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventError, 
                                                        OMX_ErrorIncorrectStateTransition,
                                                        OMX_TI_ErrorMinor, 
                                                        "Incorrect State Transition");
                 }
                break;


        case OMX_StateInvalid:
           OMX_PRSTATE2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: NBAMRDECHandleCommand: Cmd OMX_StateInvalid:\n",__LINE__);
                if (pComponentPrivate->curState == commandedState){
                    pComponentPrivate->cbInfo.EventHandler (pHandle, 
                                                            pHandle->pApplicationPrivate,
                                                            OMX_EventError, 
                                                            OMX_ErrorSameState,
                                                            OMX_TI_ErrorSevere,
                                                            "Same State");
                    OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Error: Same State Given by \
                           Application\n",__LINE__);
                   }
                else{
                OMX_PRINT2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Comp: OMX_AmrDecUtils.c\n",__LINE__);
                if (pComponentPrivate->curState != OMX_StateWaitForResources && 
                    pComponentPrivate->curState != OMX_StateLoaded) {

                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                        EMMCodecControlDestroy, (void *)p);

                NBAMRDEC_CleanupInitParams(pHandle);
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
           OMX_PRINT2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: NBAMRDECHandleCommand: Cmd OMX_StateMax::\n",__LINE__);
           break;
        default:
            break;
    } /* End of Switch */
    }
    else if (command == OMX_CommandMarkBuffer) {
            OMX_PRBUFFER1(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: command OMX_CommandMarkBuffer received\n",__LINE__);
            if(!pComponentPrivate->pMarkBuf){
            OMX_PRBUFFER1(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: command OMX_CommandMarkBuffer received\n",__LINE__);
            /* TODO Need to handle multiple marks */
            pComponentPrivate->pMarkBuf = (OMX_MARKTYPE *)(commandData);
        }
    }
    else if (command == OMX_CommandPortDisable) {
        OMX_PRINT2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c ::\n",__LINE__);
        
        if (!pComponentPrivate->bDisableCommandPending) {                                                        
        if(commandData == 0x0 || commandData == -1){   /*Input*/
            /* disable port */
            pComponentPrivate->pPortDef[NBAMRDEC_INPUT_PORT]->bEnabled = OMX_FALSE;
            for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
                if (NBAMRDEC_IsPending(pComponentPrivate,pComponentPrivate->pInputBufferList->pBufHdr[i],OMX_DirInput)) {
                    /* Real solution is flush buffers from DSP.  Until we have the ability to do that 
                       we just call EmptyBufferDone() on any pending buffers */
#ifdef __PERF_INSTRUMENTATION__
                    PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                      PREF(pComponentPrivate->pInputBufferList->pBufHdr[i], pBuffer),
                                      0,
                                      PERF_ModuleHLMM);
#endif
                     NBAMRDEC_ClearPending(pComponentPrivate,pComponentPrivate->pInputBufferList->pBufHdr[i],OMX_DirInput);
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
        if(commandData == 0x1 || commandData == -1){      /*Output*/
            char *pArgs = "damedesuStr";
            pComponentPrivate->pPortDef[NBAMRDEC_OUTPUT_PORT]->bEnabled = OMX_FALSE;
            if (pComponentPrivate->curState == OMX_StateExecuting) {
                pComponentPrivate->bNoIdleOnStop = OMX_TRUE;
                OMX_PRDSP1(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Calling LCML_ControlCodec()\n",__LINE__);
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
        
    if(commandData == 0x0) {
        if(!pComponentPrivate->pPortDef[NBAMRDEC_INPUT_PORT]->bPopulated){
            /* return cmdcomplete event if input unpopulated */ 
            pComponentPrivate->cbInfo.EventHandler(
                pHandle, pHandle->pApplicationPrivate,
                OMX_EventCmdComplete, OMX_CommandPortDisable,NBAMRDEC_INPUT_PORT, NULL);
                OMX_PRCOMM2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Clearing bDisableCommandPending\n",__LINE__);
                pComponentPrivate->bDisableCommandPending = 0;
        }
        else{
                    pComponentPrivate->bDisableCommandPending = 1;
                    pComponentPrivate->bDisableCommandParam = commandData;
        }
    }

    if(commandData == 0x1) {
                if (!pComponentPrivate->pPortDef[NBAMRDEC_OUTPUT_PORT]->bPopulated){
                /* return cmdcomplete event if output unpopulated */ 
                pComponentPrivate->cbInfo.EventHandler(
                     pHandle, pHandle->pApplicationPrivate,
                     OMX_EventCmdComplete, OMX_CommandPortDisable,NBAMRDEC_OUTPUT_PORT, NULL);
                OMX_PRCOMM1(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Clearing bDisableCommandPending\n",__LINE__);
                pComponentPrivate->bDisableCommandPending = 0;
                }
                else {
                    pComponentPrivate->bDisableCommandPending = 1;
                    pComponentPrivate->bDisableCommandParam = commandData;
                }
            }

     if(commandData == -1) {
                if (!pComponentPrivate->pPortDef[NBAMRDEC_INPUT_PORT]->bPopulated && 
                !pComponentPrivate->pPortDef[NBAMRDEC_OUTPUT_PORT]->bPopulated){

                /* return cmdcomplete event if inout & output unpopulated */ 
                pComponentPrivate->cbInfo.EventHandler(
                     pHandle, pHandle->pApplicationPrivate,
                     OMX_EventCmdComplete, OMX_CommandPortDisable,NBAMRDEC_INPUT_PORT, NULL);

                pComponentPrivate->cbInfo.EventHandler(
                     pHandle, pHandle->pApplicationPrivate,
                     OMX_EventCmdComplete, OMX_CommandPortDisable,NBAMRDEC_OUTPUT_PORT, NULL);
                OMX_PRCOMM1(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Clearing bDisableCommandPending\n",__LINE__);
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
                OMX_PRCOMM1(pComponentPrivate->dbg, "setting input port to enabled\n");
                pComponentPrivate->pPortDef[NBAMRDEC_INPUT_PORT]->bEnabled = OMX_TRUE;
                OMX_PRCOMM2(pComponentPrivate->dbg, "pComponentPrivate->pPortDef[NBAMRDEC_INPUT_PORT]->bEnabled = %d\n",
                              pComponentPrivate->pPortDef[NBAMRDEC_INPUT_PORT]->bEnabled);

                if(pComponentPrivate->AlloBuf_waitingsignal){
                    pComponentPrivate->AlloBuf_waitingsignal = 0;
                }
            }
            if(commandData == 0x1 || commandData == -1){
                char *pArgs = "damedesuStr";
                /* enable out port */
                if(pComponentPrivate->AlloBuf_waitingsignal){
                    pComponentPrivate->AlloBuf_waitingsignal = 0;
#ifndef UNDER_CE
                    pthread_mutex_lock(&pComponentPrivate->AlloBuf_mutex); 
                    pthread_cond_signal(&pComponentPrivate->AlloBuf_threshold);
                    pthread_mutex_unlock(&pComponentPrivate->AlloBuf_mutex);    
#else
                    OMX_SignalEvent(&(pComponentPrivate->AlloBuf_event));
#endif
                }
                if(pComponentPrivate->curState == OMX_StateExecuting) {
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               EMMCodecControlStart,(void *)pArgs);
                }
                OMX_PRCOMM1(pComponentPrivate->dbg, "setting output port to enabled\n");
                pComponentPrivate->pPortDef[NBAMRDEC_OUTPUT_PORT]->bEnabled = OMX_TRUE;
                OMX_PRCOMM2(pComponentPrivate->dbg, "pComponentPrivate->pPortDef[NBAMRDEC_OUTPUT_PORT]->bEnabled = %d\n",
                              pComponentPrivate->pPortDef[NBAMRDEC_OUTPUT_PORT]->bEnabled);
            }
        }

        if(commandData == 0x0){
            if (pComponentPrivate->curState == OMX_StateLoaded || 
                pComponentPrivate->pPortDef[NBAMRDEC_INPUT_PORT]->bPopulated) {
                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandPortEnable,
                                                       NBAMRDEC_INPUT_PORT,
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
                pComponentPrivate->pPortDef[NBAMRDEC_OUTPUT_PORT]->bPopulated){
                pComponentPrivate->cbInfo.EventHandler( pHandle, 
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,
                                                        OMX_CommandPortEnable,
                                                        NBAMRDEC_OUTPUT_PORT, 
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
                (pComponentPrivate->pPortDef[NBAMRDEC_INPUT_PORT]->bPopulated
                && pComponentPrivate->pPortDef[NBAMRDEC_OUTPUT_PORT]->bPopulated)){
                pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete, 
                                                       OMX_CommandPortEnable,
                                                       NBAMRDEC_INPUT_PORT, 
                                                       NULL);
                pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete, 
                                                       OMX_CommandPortEnable,
                                                       NBAMRDEC_OUTPUT_PORT, 
                                                       NULL);
                pComponentPrivate->bEnableCommandPending = 0;
                NBAMRDECFill_LCMLInitParamsEx(pComponentPrivate->pHandle);
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
       OMX_U32 aParam[3] = {0};
        OMX_PRCOMM1(pComponentPrivate->dbg, "Flushing input port %d\n",pComponentPrivate->nUnhandledEmptyThisBuffers);
        if(commandData == 0x0 || commandData == -1) {
            if(pComponentPrivate->nUnhandledEmptyThisBuffers == 0) {
                pComponentPrivate->bFlushInputPortCommandPending = OMX_FALSE;
                pComponentPrivate->first_buff = 0;

                aParam[0] = USN_STRMCMD_FLUSH; 
                aParam[1] = 0x0; 
                aParam[2] = 0x0; 

                OMX_PRCOMM2(pComponentPrivate->dbg, "Sending USN_STRMCMD_FLUSH Command for IN port\n");
                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                           EMMCodecControlStrmCtrl, 
                                           (void*)aParam);
                if (eError != OMX_ErrorNone) {
                    goto EXIT;
                }
            } else{
                pComponentPrivate->bFlushInputPortCommandPending = OMX_TRUE;
            }
        }
        if(commandData == 0x1 || commandData == -1){
            OMX_PRCOMM1(pComponentPrivate->dbg, "Flushing out port %d\n",pComponentPrivate->nUnhandledFillThisBuffers);
            if (pComponentPrivate->nUnhandledFillThisBuffers == 0)  {
                pComponentPrivate->bFlushOutputPortCommandPending = OMX_FALSE;
                if (pComponentPrivate->first_output_buf_rcv != 0) {
                    pComponentPrivate->first_buff = 0;
                    pComponentPrivate->first_output_buf_rcv = 0;
                }

                aParam[0] = USN_STRMCMD_FLUSH; 
                aParam[1] = 0x1; 
                aParam[2] = 0x0; 

                OMX_PRCOMM1(pComponentPrivate->dbg, "Sending USN_STRMCMD_FLUSH Command for OUT port\n");
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
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Exiting NBAMRDECHandleCommand Function\n",__LINE__);
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Returning %d\n",__LINE__,eError);
    if (eError != OMX_ErrorNone && eError != EXIT_COMPONENT_THRD) {
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
* @NBAMRDECHandleDataBuf_FromApp() This function is called by the component when ever it
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
OMX_ERRORTYPE NBAMRDECHandleDataBuf_FromApp(OMX_BUFFERHEADERTYPE* pBufHeader,
                                    AMRDEC_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_DIRTYPE eDir;
    LCML_NBAMRDEC_BUFHEADERTYPE *pLcmlHdr;
    LCML_DSP_INTERFACE *pLcmlHandle = (LCML_DSP_INTERFACE *)
                                              pComponentPrivate->pLcmlHandle;
    OMX_U32 index;
    OMX_U32 frameType;
    OMX_U32 frameLength;
    OMX_U8* pExtraData;
    OMX_U16 i;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefIn = NULL;

    OMX_U32 holdBufferSize;
    OMX_U8 nFrames =0;    
    LCML_DSP_INTERFACE * phandle;
    OMX_U8 bufSize=0;
    AMRDEC_AudioCodecParams *pParams;
   OMX_STRING p = "";
    
    OMX_U32 nFilledLenLocal;
    OMX_U8 TOCentry, hh=0, *TOCframetype=0;
    OMX_U16 offset = 0;

    DSP_STATUS status;
    OMX_BOOL isFrameParamChanged=OMX_FALSE;

    if (OMX_AUDIO_AMRDTXasEFR == pComponentPrivate->iAmrMode){
        bufSize = INPUT_BUFF_SIZE_EFR;
    } 
    else{
        bufSize = STD_NBAMRDEC_BUF_SIZE;              
    }

    OMX_PRINT1(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Entering NBAMRDECHandleDataBuf_FromApp Function\n",__LINE__);

    holdBufferSize = bufSize * (pComponentPrivate->pInputBufferList->numBuffers + 1);
    /*Find the direction of the received buffer from buffer list */
    eError = NBAMRDECGetBufferDirection(pBufHeader, &eDir);
    if (eError != OMX_ErrorNone) {
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: The PBufHeader is not found in the list\n",
                                                                     __LINE__);
        goto EXIT;
    }    
    if (pBufHeader->pBuffer == NULL) {
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
        if ( pBufHeader->nFilledLen > 0) {
            pComponentPrivate->bBypassDSP = 0;
            if ( pComponentPrivate->nHoldLength == 0 ) 
            {
                if (pComponentPrivate->mimemode == NBAMRDEC_MIMEMODE) 
                {
                    OMX_PRDSP2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: NBAMRDECHandleDa\
taBuf_FromApp - reading NBAMRDEC_MIMEMODE\n",__LINE__);
                    frameLength=INPUT_NBAMRDEC_BUFFER_SIZE_MIME;
                    if(pComponentPrivate->using_rtsp==1){ /* formating data */
                        nFilledLenLocal=pBufHeader->nFilledLen; 
                        while(TRUE)
                        {
                            TOCframetype = (OMX_U8*)realloc(TOCframetype, ((hh + 1) * sizeof(OMX_U8)));
                            if (TOCframetype == NULL)
                            {
                              OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: ERROR: Couldn't realloc memory!",__LINE__);
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
                                OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: ERROR: Trying to write beyond buffer boundaries!",__LINE__);
                              goto EXIT;
                            }   
                            else
                                memcpy(pBufHeader->pBuffer + offset, 
                                                &TOCframetype[nFrames],
                                                sizeof(OMX_U8));
                            offset+=pComponentPrivate->amrMimeBytes[index];
                            if ( offset + 1 + nFilledLenLocal > pBufHeader->nAllocLen){
                                OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: ERROR: Trying to write beyond buffer boundaries!",__LINE__);
                              goto EXIT;
                            }   
                            else
                            memmove(pBufHeader->pBuffer + offset + 1,
                                                pBufHeader->pBuffer + offset,
                                                nFilledLenLocal);
                            if (pComponentPrivate->amrMimeBytes[index] > nFilledLenLocal){
                                        nFilledLenLocal = 0;
                            }else{
                                        nFilledLenLocal -= pComponentPrivate->amrMimeBytes[index];
                            }
                            nFrames++;                                                
                        }
                        free(TOCframetype);
			 TOCframetype = NULL;
                    }
                    frameType = 0;
                    nFrames = 0;
                    i=0; 
                    while(pBufHeader->nFilledLen > 0 )
                    {   /*Reorder the Mime buffer in case that has*/
                        frameType = pBufHeader->pBuffer[i]; /*more than 1 frame                 */
                        index = (frameType >> 3) & 0x0F;
                        if(nFrames)
                        {
                            if (((nFrames*INPUT_NBAMRDEC_BUFFER_SIZE_MIME) + pBufHeader->nFilledLen) 
			       > pBufHeader->nAllocLen) {
                               OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: ERROR: Trying to write beyond buffer boundaries!",__LINE__);
                               goto EXIT;
                           }
                            memmove(pBufHeader->pBuffer + (nFrames*INPUT_NBAMRDEC_BUFFER_SIZE_MIME),
                                    pBufHeader->pBuffer + i,
                                    pBufHeader->nFilledLen);                                                    
                        }
			if ((index >= NUM_MIME_BYTES_ARRAY) || 
			   ((index < NUM_MIME_BYTES_ARRAY) && 
			   (pComponentPrivate->amrMimeBytes[index] == 0))) {
                           OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: no more frames index=%d", __LINE__, (int)index);
                           if (index < NUM_MIME_BYTES_ARRAY)
                               OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: no more frames mimebytes=%d", __LINE__, (int)pComponentPrivate->amrMimeBytes[index]);
                               break;
                        }
                        if (pComponentPrivate->amrMimeBytes[index] > pBufHeader->nFilledLen){
                            pBufHeader->nFilledLen = 0;
                        }else{
                            pBufHeader->nFilledLen -= pComponentPrivate->amrMimeBytes[index];
                        }
                        i = (nFrames*INPUT_NBAMRDEC_BUFFER_SIZE_MIME) + (OMX_U16)pComponentPrivate->amrMimeBytes[index];
                        nFrames++;                                                
                    }
                    pBufHeader->nFilledLen=nFrames*INPUT_NBAMRDEC_BUFFER_SIZE_MIME;
                }
                else if (pComponentPrivate->mimemode == NBAMRDEC_PADMIMEMODE)
                                {
                                    OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: NBAMRDECHandleDa\
                                            taBuf_FromApp - reading NBAMRDEC_PADMIMEMODE\
                                            nFilledLen %ld nAllocLen %ld\n",__LINE__, pBufHeader->nFilledLen, pBufHeader->nAllocLen);
                                    frameLength=INPUT_NBAMRDEC_BUFFER_SIZE_MIME;
                                    nFrames=pBufHeader->nAllocLen / frameLength; /*to get the corresponding header in the LCML */
                                }
                else if (pComponentPrivate->mimemode == NBAMRDEC_IF2)
                {
                    OMX_PRDSP1(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: NBAMRDECHandleDataBuf_FromApp - reading NBAMRDEC_IF2MODE\n", __LINE__);
                    frameLength=INPUT_NBAMRDEC_BUFFER_SIZE_IF2;
                    nFrames = 0;
                    i = 0;
                    while (pBufHeader->nFilledLen > 0)
                    {
                        /*Reorder the IF2 buffer in case that has more than 1 frame */
                        frameType = pBufHeader->pBuffer[i]; 
                        index = frameType&0x0F;
                        if (nFrames)
                        {
                            if (((nFrames*INPUT_NBAMRDEC_BUFFER_SIZE_IF2) + pBufHeader->nFilledLen) 
			       > pBufHeader->nAllocLen) {
                               OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: ERROR: Trying to write beyond buffer boundaries!",__LINE__);
                               goto EXIT;
                            }
                            memmove(pBufHeader->pBuffer + (nFrames *INPUT_NBAMRDEC_BUFFER_SIZE_IF2), 
                                    pBufHeader->pBuffer + i, 
                                    pBufHeader->nFilledLen);
                        }
                        if ((index >= NUM_IF2_BYTES_ARRAY) || 
			   ((index < NUM_IF2_BYTES_ARRAY) && 
			   (pComponentPrivate->amrIF2Bytes[index] == 0))) {
                           OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: no more frames index=%d", __LINE__, (int)index);
                           if (index < NUM_IF2_BYTES_ARRAY)
                               OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: no more frames mimebytes=%d", __LINE__, (int)pComponentPrivate->amrIF2Bytes[index]);
                               break;
                        }
                        if(pComponentPrivate->amrIF2Bytes[index] > pBufHeader->nFilledLen){
                            pBufHeader->nFilledLen=0;
                        }else{
                            pBufHeader->nFilledLen -= pComponentPrivate->amrIF2Bytes[index];
                        }
                        i = (nFrames *INPUT_NBAMRDEC_BUFFER_SIZE_IF2) + (OMX_U16)pComponentPrivate->amrIF2Bytes[index];
                        nFrames++;
                    }
                    pBufHeader->nFilledLen = nFrames * INPUT_NBAMRDEC_BUFFER_SIZE_IF2;
                }

                else 
                {
                    OMX_PRDSP2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: NBAMRDECHandleDataBuf_FromApp - reading NBAMRDEC_NONMIMEMODE\n",__LINE__);
                    frameLength = STD_NBAMRDEC_BUF_SIZE;  /*/ non Mime mode*/
                    nFrames = (OMX_U8)(pBufHeader->nFilledLen / frameLength);
                }

                if (nFrames >= 1 && (NBAMRDEC_FORMATCONFORMANCE == pComponentPrivate->mimemode))
                {  
                    /* At least there is 1 frame in the buffer */
                    pComponentPrivate->nHoldLength = pBufHeader->nFilledLen - frameLength*nFrames;
                    if ( pComponentPrivate->nHoldLength > 0 ) {/* something need to be hold in iHoldBuffer */
                        if (pComponentPrivate->pHoldBuffer == NULL) {
                            OMX_MALLOC_SIZE(pComponentPrivate->pHoldBuffer, (bufSize * (pComponentPrivate->pInputBufferList->numBuffers + 3)),void);
                        }
                        /* Copy the extra data into pHoldBuffer. Size will be nHoldLength. */
                        pExtraData = pBufHeader->pBuffer + bufSize*nFrames;
			if (pComponentPrivate->nHoldLength >
			   (bufSize * (pComponentPrivate->pInputBufferList->numBuffers + 3)))
			   {
                               OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: ERROR: Trying to write beyond buffer boundaries!",__LINE__);
                               goto EXIT;
			   }
                        memcpy (pComponentPrivate->pHoldBuffer, pExtraData, pComponentPrivate->nHoldLength);
                    }
                }
                else {
                    if (pComponentPrivate->mimemode == NBAMRDEC_FORMATCONFORMANCE)
                    {
                        /* received buffer with less than 1 AMR frame. Save the data in iHoldBuffer.*/
                        pComponentPrivate->nHoldLength = pBufHeader->nFilledLen;
                        /* save the data into iHoldBuffer.*/
                        if (pComponentPrivate->pHoldBuffer == NULL) {
                            OMX_MALLOC_SIZE(pComponentPrivate->pHoldBuffer, (bufSize * (pComponentPrivate->pInputBufferList->numBuffers + 3)),void);
                        }
                        /* Not enough data to be sent. Copy all received data into iHoldBuffer.*/
                        /* Size to be copied will be iHoldLen == mmData->BufferSize() */
                        memset (pComponentPrivate->pHoldBuffer,0,bufSize * (pComponentPrivate->pInputBufferList->numBuffers + 1));
                        memcpy (pComponentPrivate->pHoldBuffer, pBufHeader->pBuffer, pComponentPrivate->nHoldLength);

                        /* since not enough data, we shouldn't send anything to SN, but instead request to EmptyBufferDone again.*/
                        OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Calling EmptyBufferDone\n",__LINE__);
                        if (pComponentPrivate->curState != OMX_StatePause) {
                            pComponentPrivate->nEmptyBufferDoneCount++;   
#ifdef __PERF_INSTRUMENTATION__
                            PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                              PREF(pBufHeader,pBuffer),
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
                if ( pComponentPrivate->mimemode == NBAMRDEC_MIMEMODE) 
                {
                    frameType = pComponentPrivate->pHoldBuffer[0];
                    index = ( frameType >> 3 ) & 0x0F;
                    frameLength = pComponentPrivate->amrMimeBytes[index];
                }
                else if(pComponentPrivate->mimemode == NBAMRDEC_IF2)
                {
                    frameType = pComponentPrivate->pHoldBuffer[0];
                    index = frameType&0x0F;
                    frameLength = pComponentPrivate->amrIF2Bytes[index];
                }
                else 
                {
                    frameLength = STD_NBAMRDEC_BUF_SIZE;
                }
                
                nFrames = (OMX_U8)(pComponentPrivate->nHoldLength / frameLength);
                if ( nFrames >= 1 )  {
                    /* Copy the data from pComponentPrivate->pHoldBuffer to pBufHeader->pBuffer*/
		    /* check the pBufHeader boundery before copying */
		    if ((nFrames*frameLength) > pBufHeader->nAllocLen)
		    {
                        OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: ERROR: Trying to write beyond buffer boundaries!",__LINE__);
			goto EXIT;
		    }
                    memcpy(pBufHeader->pBuffer,pComponentPrivate->pHoldBuffer,nFrames*frameLength);
                    pBufHeader->nFilledLen = nFrames*frameLength;
    
                    /* Now the pHoldBuffer has pBufHeader->nFilledLen fewer bytes, update nHoldLength*/
                    pComponentPrivate->nHoldLength = pComponentPrivate->nHoldLength - pBufHeader->nFilledLen;
                
                    /* Shift the remaining bytes to the beginning of the pHoldBuffer */
                    pExtraData = pComponentPrivate->pHoldBuffer + pBufHeader->nFilledLen;
    
                    memcpy(pComponentPrivate->pHoldBuffer,pExtraData,pComponentPrivate->nHoldLength);    
    
                    /* Clear the rest of the data from the pHoldBuffer */
                    /*pExtraData = pComponentPrivate->pHoldBuffer + pComponentPrivate->nHoldLength;*/
                    /*memset(pExtraData,0,holdBufferSize - pComponentPrivate->nHoldLength);*/
                }
                else {
                    if (pComponentPrivate->curState != OMX_StatePause) {
                        OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Calling EmptyBufferDone\n",__LINE__);
                        pComponentPrivate->nEmptyBufferDoneCount++; 
#ifdef __PERF_INSTRUMENTATION__
                        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                          PREF(pBufHeader,pBuffer),
                                          0,
                                          PERF_ModuleHLMM);
#endif
                        pComponentPrivate->cbInfo.EmptyBufferDone (pComponentPrivate->pHandle,
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
            if((((pBufHeader->nFlags)&(OMX_BUFFERFLAG_EOS)) != OMX_BUFFERFLAG_EOS) && !pBufHeader->pMarkData){
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
            eError = NBAMRDECGetCorresponding_LCMLHeader(pComponentPrivate,pBufHeader->pBuffer, OMX_DirInput, &pLcmlHdr);
            if (eError != OMX_ErrorNone) {
                OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Error: Invalid Buffer Came ...\n",__LINE__);
                goto EXIT;
            }

#ifdef __PERF_INSTRUMENTATION__
            PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                              PREF(pBufHeader,pBuffer),
                              pPortDefIn->nBufferSize, 
                              PERF_ModuleCommonLayer);
#endif
            /*---------------------------------------------------------------*/
               
            phandle = (LCML_DSP_INTERFACE *)(((LCML_CODEC_INTERFACE *)pLcmlHandle->pCodecinterfacehandle)->pCodec); 
         
            if( (pLcmlHdr->pBufferParam->usNbFrames < nFrames) && (pLcmlHdr->pFrameParam!=NULL) ){

                OMX_DmmUnMap(phandle->dspCodec->hProc, /*Unmap DSP memory used*/
                             (void*)pLcmlHdr->pBufferParam->pParamElem,
                             pLcmlHdr->pDmmBuf->pReserved, pComponentPrivate->dbg);
                pLcmlHdr->pBufferParam->pParamElem = NULL;
                                
                OMX_MEMFREE_STRUCT_DSPALIGN(pLcmlHdr->pFrameParam,NBAMRDEC_FrameStruct);
            }

            if(pLcmlHdr->pFrameParam==NULL ){
                OMX_MALLOC_SIZE_DSPALIGN(pLcmlHdr->pFrameParam,(sizeof(NBAMRDEC_FrameStruct)*nFrames),NBAMRDEC_FrameStruct);

                eError = OMX_DmmMap(phandle->dspCodec->hProc, 
                                    nFrames*sizeof(NBAMRDEC_FrameStruct),
                                    (void*)pLcmlHdr->pFrameParam, 
                                    (pLcmlHdr->pDmmBuf), pComponentPrivate->dbg);
                 
                if (eError != OMX_ErrorNone)
                {
                    OMX_ERROR4(pComponentPrivate->dbg, "OMX_DmmMap ERRROR!!!!\n\n");
                    goto EXIT;
                }
                pLcmlHdr->pBufferParam->pParamElem = (NBAMRDEC_FrameStruct *)pLcmlHdr->pDmmBuf->pMapped; /*DSP Address*/                    
            }

            for(i=0;i<nFrames;i++){
                (pLcmlHdr->pFrameParam+i)->usLastFrame = 0;
                (pLcmlHdr->pFrameParam+i)->usFrameLost = 0;
            }
           
            /* We only support frame lost error concealment if there is one frame per buffer */  
            if (nFrames == 1) 
            {
                /* if the bFrameLost flag is set it means that the client has 
                indicated that the next frame is corrupt so set the frame 
                lost frame parameter */
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
                OMX_PRINT2(pComponentPrivate->dbg, "%d :: reconfiguring SN\n",__LINE__);
                if(pComponentPrivate->dasfmode == 1) {
                    OMX_U32 pValues[4];
                    OMX_PRDSP2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: ---- Comp: DASF Functionality is ON ---\n",__LINE__);
                    
                    if(pComponentPrivate->streamID == 0) 
                    { 
                        OMX_ERROR4(pComponentPrivate->dbg, "**************************************\n");
                        OMX_ERROR4(pComponentPrivate->dbg, ":: Error = OMX_ErrorInsufficientResources\n");
                        OMX_ERROR4(pComponentPrivate->dbg, "**************************************\n");
                        eError = OMX_ErrorInsufficientResources; 
                        pComponentPrivate->curState = OMX_StateInvalid; 
                        pComponentPrivate->cbInfo.EventHandler( pComponentPrivate->pHandle, 
                                                                pComponentPrivate->pHandle->pApplicationPrivate, 
                                                                OMX_EventError, 
                                                                OMX_ErrorInvalidState,
                                                                OMX_TI_ErrorMajor, 
                                                        "AM: No Stream ID Available");                 
                        goto EXIT; 
                    } 
                    
                    OMX_MALLOC_SIZE_DSPALIGN(pComponentPrivate->pParams,sizeof(AMRDEC_AudioCodecParams),AMRDEC_AudioCodecParams);
                    pParams = pComponentPrivate->pParams;
                    pParams->iAudioFormat = 1;
                    pParams->iSamplingRate = 8000;
                    pParams->iStrmId = pComponentPrivate->streamID;

                    pValues[0] = USN_STRMCMD_SETCODECPARAMS;
                    pValues[1] = (OMX_U32)pParams;
                    pValues[2] = sizeof(AMRDEC_AudioCodecParams);
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                          EMMCodecControlStrmCtrl,(void *)pValues);

                    if(eError != OMX_ErrorNone) {
                        OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Error Occurred in Codec StreamControl..\n",__LINE__);
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
                    OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Error Occurred in Codec Start..\n",__LINE__);
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
            /** **/
            
            
            
            if(pBufHeader->nFlags & OMX_BUFFERFLAG_EOS) 
            {
                (pLcmlHdr->pFrameParam+(nFrames-1))->usLastFrame = OMX_BUFFERFLAG_EOS;
                isFrameParamChanged = OMX_TRUE;
                pBufHeader->nFlags = 0;
                if(!pComponentPrivate->dasfmode)
                {
                    if(!pBufHeader->nFilledLen)
                    {
                        pComponentPrivate->pOutputBufferList->pBufHdr[0]->nFlags |= OMX_BUFFERFLAG_EOS;
                    }
                    pComponentPrivate->cbInfo.EventHandler( pComponentPrivate->pHandle,
                                                            pComponentPrivate->pHandle->pApplicationPrivate,
                                                            OMX_EventBufferFlag,
                                                            pComponentPrivate->pOutputBufferList->pBufHdr[0]->nOutputPortIndex,
                                                            pComponentPrivate->pOutputBufferList->pBufHdr[0]->nFlags, NULL);
                }
                
                pComponentPrivate->SendAfterEOS = 1;
                OMX_PRCOMM2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c : pComponentPrivate->SendAfterEOS %d\n",__LINE__,pComponentPrivate->SendAfterEOS);
            }
            if (isFrameParamChanged == OMX_TRUE) {
               isFrameParamChanged = OMX_FALSE;
               //Issue an initial memory flush to ensure cache coherency */
               OMX_PRINT1(pComponentPrivate->dbg, "OMX_AmrDec_Utils.c : flushing pFrameParam\n");
               status = DSPProcessor_FlushMemory(phandle->dspCodec->hProc, pLcmlHdr->pFrameParam,  nFrames*sizeof(NBAMRDEC_FrameStruct), 0);
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

            if(pComponentPrivate->first_buff == 0){
                pComponentPrivate->first_TS = pBufHeader->nTimeStamp;
                OMX_PRBUFFER2(pComponentPrivate->dbg, "in ts-%ld\n",pBufHeader->nTimeStamp);
                pComponentPrivate->first_buff = 1;
            }
            
            for (i=0; i < INPUT_NBAMRDEC_BUFFER_SIZE_MIME; i++) {
                OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Queueing pBufHeader->pBuffer[%d] = %x\n",__LINE__,i,pBufHeader->pBuffer[i]);
            }

            if (pComponentPrivate->curState == OMX_StateExecuting) 
            {
                if (!NBAMRDEC_IsPending(pComponentPrivate,pBufHeader,OMX_DirInput)) 
                {
                    NBAMRDEC_SetPending(pComponentPrivate,pBufHeader,OMX_DirInput);
                    if (pComponentPrivate->mimemode == NBAMRDEC_MIMEMODE)
                        eError = LCML_QueueBuffer(pLcmlHandle->pCodecinterfacehandle, 
                                                EMMCodecInputBuffer, 
                                                (OMX_U8*)pBufHeader->pBuffer, 
                                                pBufHeader->nAllocLen,  
                                                pBufHeader->nFilledLen, 
                                                (OMX_U8*)pLcmlHdr->pBufferParam, 
                                                sizeof(NBAMRDEC_ParamStruct), 
                                                NULL);
                    else if (pComponentPrivate->mimemode == NBAMRDEC_IF2)
                        eError = LCML_QueueBuffer(pLcmlHandle->pCodecinterfacehandle, 
                                                    EMMCodecInputBuffer, 
                                                    (OMX_U8*)pBufHeader->pBuffer, 
                                                    INPUT_NBAMRDEC_BUFFER_SIZE_IF2*nFrames, 
                                                    INPUT_NBAMRDEC_BUFFER_SIZE_IF2*nFrames, 
                                                    (OMX_U8*)pLcmlHdr->pBufferParam, 
                                                    sizeof(NBAMRDEC_ParamStruct), 
                                                    NULL);
                    else /*Frame Conformace, 120 for EFR, 118 for Standart*/
                        eError = LCML_QueueBuffer(pLcmlHandle->pCodecinterfacehandle, 
                                                    EMMCodecInputBuffer, 
                                                    (OMX_U8*)pBufHeader->pBuffer, 
                                                    bufSize*nFrames, 
                                                    bufSize*nFrames, 
                                                    (OMX_U8*)pLcmlHdr->pBufferParam, 
                                                    sizeof(NBAMRDEC_ParamStruct), 
                                                    NULL);
                    if (eError != OMX_ErrorNone) {
                       eError = OMX_ErrorHardware;
                       goto EXIT;
                    }
                    pComponentPrivate->lcml_nCntIp++;
                    pComponentPrivate->lcml_nIpBuf++;
                }
            }else if(pComponentPrivate->curState == OMX_StatePause){
                pComponentPrivate->pInputBufHdrPending[pComponentPrivate->nNumInputBufPending++] = pBufHeader;
            }
            
            if(pBufHeader->pMarkData)
            {
                /* copy mark to output buffer header */ 
                if(pComponentPrivate->pOutputBufferList->pBufHdr[0]!=NULL){
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
                if (pComponentPrivate->curState != OMX_StatePause ) {
                    OMX_PRBUFFER2(pComponentPrivate->dbg, "line %d:: Calling EmptyBufferDone\n",__LINE__);
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
        eError = NBAMRDECGetCorresponding_LCMLHeader(pComponentPrivate, pBufHeader->pBuffer, OMX_DirOutput, &pLcmlHdr);     
        phandle = (LCML_DSP_INTERFACE *)(((LCML_CODEC_INTERFACE *)pLcmlHandle->pCodecinterfacehandle)->pCodec);

#ifdef __PERF_INSTRUMENTATION__
        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                          PREF(pBufHeader,pBuffer),
                          0,
                          PERF_ModuleCommonLayer);
#endif

        nFrames = (OMX_U8)(pBufHeader->nAllocLen/OUTPUT_NBAMRDEC_BUFFER_SIZE);

        if( (pLcmlHdr->pBufferParam->usNbFrames < nFrames) && (pLcmlHdr->pFrameParam!=NULL) ){

            OMX_DmmUnMap(phandle->dspCodec->hProc, /*Unmap DSP memory used*/
                         (void*)pLcmlHdr->pBufferParam->pParamElem,
                         pLcmlHdr->pDmmBuf->pReserved, pComponentPrivate->dbg);
            pLcmlHdr->pBufferParam->pParamElem = NULL;
                            
            OMX_MEMFREE_STRUCT_DSPALIGN(pLcmlHdr->pFrameParam,NBAMRDEC_FrameStruct);
        }

        if(pLcmlHdr->pFrameParam==NULL ){
            OMX_MALLOC_SIZE_DSPALIGN(pLcmlHdr->pFrameParam,(sizeof(NBAMRDEC_FrameStruct)*nFrames),NBAMRDEC_FrameStruct);
                    
            eError = OMX_DmmMap(phandle->dspCodec->hProc, 
                                nFrames*sizeof(NBAMRDEC_FrameStruct),
                                (void*)pLcmlHdr->pFrameParam, 
                                (pLcmlHdr->pDmmBuf), pComponentPrivate->dbg);
                        
            if (eError != OMX_ErrorNone)
            {
                OMX_ERROR4(pComponentPrivate->dbg, "OMX_DmmMap ERRROR!!!!\n");
                goto EXIT;
            }
        
            pLcmlHdr->pBufferParam->pParamElem = (NBAMRDEC_FrameStruct *)pLcmlHdr->pDmmBuf->pMapped;/*DSP Address*/     
        }                                           

        pLcmlHdr->pBufferParam->usNbFrames = nFrames;
 
        for(i=0;i<pLcmlHdr->pBufferParam->usNbFrames;i++){
            (pLcmlHdr->pFrameParam+i)->usLastFrame = 0;
            (pLcmlHdr->pFrameParam+i)->usFrameLost = 0;
        }     
    
        //Issue an initial memory flush to ensure cache coherency */
        OMX_PRINT1(pComponentPrivate->dbg, "OMX_AmrDec_Utils.c : flushing pFrameParam\n");
        status = DSPProcessor_FlushMemory(phandle->dspCodec->hProc, pLcmlHdr->pFrameParam,  nFrames*sizeof(NBAMRDEC_FrameStruct), 0);
        if(DSP_FAILED(status))
        {
           OMXDBG_PRINT(stderr, ERROR, 4, 0, "Unable to flush mapped buffer: error 0x%x",(int)status);
           goto EXIT;
        }

        if (pComponentPrivate->curState == OMX_StateExecuting) {
            if (!NBAMRDEC_IsPending(pComponentPrivate,pBufHeader,OMX_DirOutput)) {
                NBAMRDEC_SetPending(pComponentPrivate,pBufHeader,OMX_DirOutput);
                eError = LCML_QueueBuffer(pLcmlHandle->pCodecinterfacehandle,
                                          EMMCodecOuputBuffer, 
                                          (OMX_U8 *)pBufHeader->pBuffer, 
                                          OUTPUT_NBAMRDEC_BUFFER_SIZE*nFrames,
                                          OUTPUT_NBAMRDEC_BUFFER_SIZE*nFrames,
                                          (OMX_U8 *) pLcmlHdr->pBufferParam, 
                                          sizeof(NBAMRDEC_ParamStruct),
                                          NULL);
                if (eError != OMX_ErrorNone ) {
                    OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: IssuingDSP OP: Error Occurred\n",__LINE__);
                    eError = OMX_ErrorHardware;
                    goto EXIT;
                }
                pComponentPrivate->lcml_nOpBuf++;
            }
        }
        else if (pComponentPrivate->curState == OMX_StatePause){
            pComponentPrivate->pOutputBufHdrPending[pComponentPrivate->nNumOutputBufPending++] = pBufHeader;
        }

        if (pComponentPrivate->bFlushOutputPortCommandPending) {
            OMX_SendCommand(pComponentPrivate->pHandle,
                            OMX_CommandFlush,
                            1,
                            NULL);
        }
    } 
    else {
        eError = OMX_ErrorBadParameter;
    }

EXIT:
    if (TOCframetype != NULL) {
	 free(TOCframetype);
    }
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Exiting from  NBAMRDECHandleDataBuf_FromApp \n",__LINE__);
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Returning error %d\n",__LINE__,eError);
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
* NBAMRDECGetBufferDirection () This function is used by the component thread to
* request a buffer from the application.  Since it was called from 2 places,
* it made sense to turn this into a small function.
*
* @param pData pointer to AMR Decoder Context Structure
* @param pCur pointer to the buffer to be requested to be filled
*
* @retval none
**/
/*-------------------------------------------------------------------*/

OMX_ERRORTYPE NBAMRDECGetBufferDirection(OMX_BUFFERHEADERTYPE *pBufHeader,
                                                         OMX_DIRTYPE *eDir)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    AMRDEC_COMPONENT_PRIVATE *pComponentPrivate = pBufHeader->pPlatformPrivate;
    OMX_U32 nBuf;
    OMX_BUFFERHEADERTYPE *pBuf = NULL;
    OMX_U16 flag = 1,i;

    OMX_PRINT1(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Entering NBAMRDECGetBufferDirection Function\n",__LINE__);

    /*Search this buffer in input buffers list */
    nBuf = pComponentPrivate->pInputBufferList->numBuffers;
    for(i=0; i<nBuf; i++) {
        pBuf = pComponentPrivate->pInputBufferList->pBufHdr[i];
        if(pBufHeader == pBuf) {
            *eDir = OMX_DirInput;
            OMX_ERROR2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Buffer %p is INPUT BUFFER\n",__LINE__, pBufHeader);
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
            OMX_ERROR2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Buffer %p is OUTPUT BUFFER\n",__LINE__, pBufHeader);
            flag = 0;
            goto EXIT;
        }
    }

    if (flag == 1) {
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Buffer %p is Not Found in the List\n",__LINE__,pBufHeader);
        eError = OMX_ErrorUndefined;
        goto EXIT;
    }
EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Exiting NBAMRDECGetBufferDirection Function\n",__LINE__);
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
OMX_ERRORTYPE NBAMRDECLCML_Callback (TUsnCodecEvent event,void * args [10])
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U8 *pBuffer = args[1];
    LCML_NBAMRDEC_BUFHEADERTYPE *pLcmlHdr;
#ifdef RESOURCE_MANAGER_ENABLED
    OMX_ERRORTYPE rm_error = OMX_ErrorNone;
#endif
    /*    ssize_t ret; */
    OMX_COMPONENTTYPE *pHandle = NULL;
    LCML_DSP_INTERFACE *pLcmlHandle;
    OMX_U8 i;
    NBAMRDEC_BUFDATA *OutputFrames;
    DSP_STATUS status;
    LCML_DSP_INTERFACE *dspphandle = (LCML_DSP_INTERFACE *)args[6];
    
    AMRDEC_COMPONENT_PRIVATE* pComponentPrivate = NULL;
    pComponentPrivate = (AMRDEC_COMPONENT_PRIVATE*)((LCML_DSP_INTERFACE*)args[6])->pComponentPrivate;
    static double time_stmp = 0;
    pHandle = pComponentPrivate->pHandle;
    
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Entering the NBAMRDECLCML_Callback Function\n",__LINE__);
    OMX_PRINT2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: args = %p\n",__LINE__,args[0]);
    OMX_PRINT2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: event = %d\n",__LINE__,event);

    switch(event) {
        
    case EMMCodecDspError:
        OMX_PRDSP2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: [LCML CALLBACK EVENT]  EMMCodecDspError\n",__LINE__);
        break;

    case EMMCodecInternalError:
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: [LCML CALLBACK EVENT]  EMMCodecInternalError\n",__LINE__);
        break;

    case EMMCodecInitError:
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: [LCML CALLBACK EVENT]  EMMCodecInitError\n",__LINE__);
        break;

    case EMMCodecDspMessageRecieved:
        OMX_PRDSP2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: [LCML CALLBACK EVENT]  EMMCodecDspMessageRecieved\n",__LINE__);
        break;

    case EMMCodecBufferProcessed:
        OMX_PRDSP2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: [LCML CALLBACK EVENT]  EMMCodecBufferProcessed\n",__LINE__);
        break;

    case EMMCodecProcessingStarted:
        OMX_PRDSP2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: [LCML CALLBACK EVENT]  EMMCodecProcessingStarted\n",__LINE__);
        break;
            
    case EMMCodecProcessingPaused:
        OMX_PRDSP2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: [LCML CALLBACK EVENT]  EMMCodecProcessingPaused\n",__LINE__);
        break;

    case EMMCodecProcessingStoped:
        OMX_PRDSP2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: [LCML CALLBACK EVENT]  EMMCodecProcessingStoped\n",__LINE__);
        break;

    case EMMCodecProcessingEof:
        OMX_PRDSP2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: [LCML CALLBACK EVENT]  EMMCodecProcessingEof\n",__LINE__);
        break;

    case EMMCodecBufferNotProcessed:
        OMX_PRDSP2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: [LCML CALLBACK EVENT]  EMMCodecBufferNotProcessed\n",__LINE__);
        break;

    case EMMCodecAlgCtrlAck:
        OMX_PRDSP2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: [LCML CALLBACK EVENT]  EMMCodecAlgCtrlAck\n",__LINE__);
        break;

    case EMMCodecStrmCtrlAck:
        OMX_PRDSP2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: [LCML CALLBACK EVENT]  EMMCodecStrmCtrlAck\n",__LINE__);
        break;
    }


       
    if(event == EMMCodecBufferProcessed)
    {
        if( (OMX_U32)args [0] == EMMCodecInputBuffer) {
            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Input: pBufferr = %p\n",__LINE__, pBuffer);
            if( pComponentPrivate->pPortDef[NBAMRDEC_INPUT_PORT]->bEnabled != OMX_FALSE){
                eError = NBAMRDECGetCorresponding_LCMLHeader(pComponentPrivate, pBuffer, OMX_DirInput, &pLcmlHdr);
#ifdef __PERF_INSTRUMENTATION__
                PERF_ReceivedFrame(pComponentPrivate->pPERFcomp,
                                   PREF(pLcmlHdr->buffer,pBuffer),
                                   0,
                                   PERF_ModuleCommonLayer);
#endif
                if (eError != OMX_ErrorNone) {
                    OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Error: Invalid Buffer Came ...\n",__LINE__);
                    goto EXIT;
                }
                NBAMRDEC_ClearPending(pComponentPrivate,pLcmlHdr->buffer,OMX_DirInput);
                OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Calling EmptyBufferDone\n",__LINE__);
                OMX_PRINT2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: pComponentPrivate->n\
HoldLength = %ld\n",__LINE__,pComponentPrivate->nHoldLength);
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
            }
        } else if ((OMX_U32)args [0] == EMMCodecOuputBuffer) {
            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Output: pBufferr = %p\n",__LINE__, pBuffer);

            eError = NBAMRDECGetCorresponding_LCMLHeader(pComponentPrivate, pBuffer, OMX_DirOutput, &pLcmlHdr);
            if (eError != OMX_ErrorNone) {
                OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Error: Invalid Buffer Came ...\n",__LINE__);
                goto EXIT;
            }

            if (!pComponentPrivate->bStopSent){
                pLcmlHdr->buffer->nFilledLen = (OMX_U32)args[8];
            }
            else     
                pLcmlHdr->buffer->nFilledLen = 0;

            OMX_PRBUFFER1(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: NBAMRDECLCML_Callback:::\
pLcmlHdr->buffer->nFilledLen = %ld\n",__LINE__,pLcmlHdr->buffer->nFilledLen);
       
            OutputFrames = (pLcmlHdr->buffer)->pOutputPortPrivate;
            OutputFrames->nFrames = (OMX_U8) ((OMX_U32)args[8] / OUTPUT_NBAMRDEC_BUFFER_SIZE);

#ifdef __PERF_INSTRUMENTATION__
            PERF_ReceivedFrame(pComponentPrivate->pPERFcomp,
                               PREF(pLcmlHdr->buffer,pBuffer),
                               PREF(pLcmlHdr->buffer,nFilledLen),
                               PERF_ModuleCommonLayer);
            pComponentPrivate->nLcml_nCntOpReceived++;
            if ((pComponentPrivate->nLcml_nCntIp >= 1) && 
                (pComponentPrivate->nLcml_nCntOpReceived == 1)) {
                PERF_Boundary(pComponentPrivate->pPERFcomp,
                              PERF_BoundaryStart | PERF_BoundarySteadyState);
            }
#endif

            NBAMRDEC_ClearPending(pComponentPrivate,pLcmlHdr->buffer,OMX_DirOutput);
            pComponentPrivate->nOutStandingFillDones++;
            pComponentPrivate->first_output_buf_rcv = 1;

            for(i=0;i<pLcmlHdr->pBufferParam->usNbFrames;i++){
                 if ( (pLcmlHdr->pFrameParam+i)->usLastFrame & OMX_BUFFERFLAG_EOS){ 
                    (pLcmlHdr->pFrameParam+i)->usLastFrame = 0;
                    (pLcmlHdr->pFrameParam+i)->usLastFrame = 0;
                    pLcmlHdr->buffer->nFlags |= OMX_BUFFERFLAG_EOS;
                    OMX_PRCOMM1(pComponentPrivate->dbg, "%d :: On Component receiving OMX_BUFFERFLAG_EOS on output\n", __LINE__);
                    OMX_PRINT1(pComponentPrivate->dbg, "OMX_AmrDec_Utils.c : flushing pFrameParam2\n");
                    status = DSPProcessor_FlushMemory(dspphandle->dspCodec->hProc, pLcmlHdr->pFrameParam, pLcmlHdr->pBufferParam->usNbFrames*sizeof(NBAMRDEC_FrameStruct), 0);
                    if(DSP_FAILED(status))
                    {
                      OMXDBG_PRINT(stderr, ERROR, 4, 0, "Unable to flush mapped buffer: error 0x%x",(int)status);
                      goto EXIT;
                    }
                    break;
                }
            }
            /* Copying time stamp information to output buffer */
            if(pComponentPrivate->first_buff == 1){
                pComponentPrivate->first_buff = 2;
                pLcmlHdr->buffer->nTimeStamp = pComponentPrivate->first_TS;
                pComponentPrivate->temp_TS = pLcmlHdr->buffer->nTimeStamp;
            }else{
                time_stmp = pLcmlHdr->buffer->nFilledLen / (1 * (((OMX_AUDIO_PARAM_PCMMODETYPE*)pComponentPrivate->amrParams[NBAMRDEC_OUTPUT_PORT])->nBitPerSample / 8));
                time_stmp = (time_stmp / ((OMX_AUDIO_PARAM_PCMMODETYPE*)pComponentPrivate->amrParams[NBAMRDEC_OUTPUT_PORT])->nSamplingRate) * 1000;
                pComponentPrivate->temp_TS += (OMX_U32)(time_stmp*1000);
                pLcmlHdr->buffer->nTimeStamp = pComponentPrivate->temp_TS;
            }
            /* Copying nTickCount information to output buffer */
            pLcmlHdr->buffer->nTickCount = pComponentPrivate->arrTickCount[pComponentPrivate->OpBufindex];
            pComponentPrivate->OpBufindex++;
            pComponentPrivate->OpBufindex %= pComponentPrivate->pPortDef[OMX_DirInput]->nBufferCountActual;           
        
            pComponentPrivate->LastOutbuf = pLcmlHdr->buffer;                    
            pComponentPrivate->num_Reclaimed_Op_Buff++;

            OMX_PRBUFFER1(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Calling FillBufferDone\n",__LINE__);
           
            pComponentPrivate->nFillBufferDoneCount++;
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
            pComponentPrivate->nOutStandingFillDones--;

            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Incrementing app_nBuf = %ld\n",__LINE__,pComponentPrivate->app_nBuf);
        }
    } else if (event == EMMCodecStrmCtrlAck) {
        OMX_PRCOMM2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: GOT MESSAGE USN_DSPACK_STRMCTRL ----\n",__LINE__);
        if (args[1] == (void *)USN_STRMCMD_FLUSH) {
            pHandle = pComponentPrivate->pHandle; 
            if ( args[2] == (void *)EMMCodecInputBuffer) {
                if (args[0] == (void *)USN_ERR_NONE ) {
                    OMX_PRCOMM1(pComponentPrivate->dbg, "Flushing input port %d\n",__LINE__);
                    for (i=0; i < pComponentPrivate->nNumInputBufPending; i++) {
                        pComponentPrivate->cbInfo.EmptyBufferDone (pHandle,
                                                                   pHandle->pApplicationPrivate,
                                                                   pComponentPrivate->pInputBufHdrPending[i]);
                        pComponentPrivate->nEmptyBufferDoneCount++;
                        SignalIfAllBuffersAreReturned(pComponentPrivate);
                        pComponentPrivate->pInputBufHdrPending[i] = NULL;
                    }
                    pComponentPrivate->nNumInputBufPending=0;
                    pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventCmdComplete, 
                                                           OMX_CommandFlush,
                                                           NBAMRDEC_INPUT_PORT, 
                                                           NULL);    
                } else {
                    OMX_ERROR4(pComponentPrivate->dbg, "LCML reported error while flushing input port\n");
                    goto EXIT;                            
                }
            }
            else if ( args[2] == (void *)EMMCodecOuputBuffer) { 
                if (args[0] == (void *)USN_ERR_NONE ) {                      
                    OMX_PRCOMM1(pComponentPrivate->dbg, "Flushing output port %d\n",__LINE__);
                    for (i=0; i < pComponentPrivate->nNumOutputBufPending; i++) {
                        pComponentPrivate->cbInfo.FillBufferDone (pHandle,
                                                                  pHandle->pApplicationPrivate,
                                                                  pComponentPrivate->pOutputBufHdrPending[i]);
                        pComponentPrivate->nFillBufferDoneCount++;
                        SignalIfAllBuffersAreReturned(pComponentPrivate);
                        pComponentPrivate->pOutputBufHdrPending[i] = NULL;
                    }
                    pComponentPrivate->nNumOutputBufPending=0;
                    pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventCmdComplete, 
                                                           OMX_CommandFlush,
                                                           NBAMRDEC_OUTPUT_PORT, 
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
        pComponentPrivate->nEmptyBufferDoneCount++;   
		pComponentPrivate->pInputBufHdrPending[i] = NULL;
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
            OMX_ERROR2(pComponentPrivate->dbg, "stop ack. received. stop waiting for sending disable command completed\n");
        }
        pthread_mutex_unlock(&pComponentPrivate->codecStop_mutex);

        if (!pComponentPrivate->bNoIdleOnStop) {
            pComponentPrivate->nNumOutputBufPending=0;
    
            /*            pComponentPrivate->ProcessingInputBuf=0;
                          pComponentPrivate->ProcessingOutputBuf=0;
            */            
            pComponentPrivate->nHoldLength = 0;
            /*            pComponentPrivate->InBuf_Eos_alreadysent  =0; */
        
            /*            OMX_NBMEMFREE_STRUCT(pComponentPrivate->pHoldBuffer); */
            /*            OMX_NBMEMFREE_STRUCT(pComponentPrivate->iMMFDataLastBuffer); */
        
            pComponentPrivate->curState = OMX_StateIdle;
#ifdef RESOURCE_MANAGER_ENABLED
            eError = RMProxy_NewSendCommand(pHandle, 
                                            RMProxy_StateSet, 
                                            OMX_NBAMR_Decoder_COMPONENT, 
                                            OMX_StateIdle, 
                                            3456, 
                                            NULL);
#endif
            /*
            if((pComponentPrivate->nEmptyThisBufferCount != pComponentPrivate->nEmptyBufferDoneCount) || (pComponentPrivate->nFillThisBufferCount != pComponentPrivate->nFillBufferDoneCount)) {
                if(pthread_mutex_lock(&bufferReturned_mutex) != 0) 
                {
                    OMXDBG_PRINT(stderr, PRINT, 1, 0, "bufferReturned_mutex mutex lock error"); 
                }
                OMXDBG_PRINT(stderr, PRINT, 1, 0, "pthread_cond_waiting for OMX to return all input and outbut buffers");
                pthread_cond_wait(&bufferReturned_condition, &bufferReturned_mutex);
                OMXDBG_PRINT(stderr, PRINT, 1, 0, "OMX has returned all input and output buffers"); 
                if(pthread_mutex_unlock(&bufferReturned_mutex) != 0) 
                {
                    OMXDBG_PRINT(stderr, PRINT, 1, 0, "bufferReturned_mutex mutex unlock error");  
                }
            }
            else
            {
                OMXDBG_PRINT(stderr, PRINT, 1, 0, "OMX has returned all input and output buffers"); 
            }
            */
            if(pComponentPrivate->bPreempted == 0){
                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                   pHandle->pApplicationPrivate,
                                                   OMX_EventCmdComplete,
                                                   OMX_CommandStateSet,
                                                   pComponentPrivate->curState,
                                                   NULL);
            }else{
                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                    pHandle->pApplicationPrivate,
                                                    OMX_EventError,
                                                    OMX_ErrorResourcesPreempted,
                                                    OMX_TI_ErrorSevere,
                                                    0);
            }
        }else{
            pComponentPrivate->bNoIdleOnStop = OMX_FALSE;
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
                NBAMRDEC_HandleUSNError (pComponentPrivate, (OMX_U32)args[5]);
                break;
            default:
                break;
        }
    }

    if(event == EMMCodecDspMessageRecieved) {
        OMX_PRSTATE2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: commandedState  = %p\n",__LINE__,args[0]);
        OMX_PRINT2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: arg1 = %p\n",__LINE__,args[1]);
        OMX_PRINT2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: arg2 = %p\n",__LINE__,args[2]);
    }

#ifdef _ERROR_PROPAGATION__

    else if (event ==EMMCodecInitError){
        /* Cheking for MMU_fault */
        if(((int) args[4] == USN_ERR_UNKNOWN_MSG) && (args[5] == (void*) NULL)) {
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
#endif
 EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Exiting the NBAMRDECLCML_Callback Function\n",__LINE__);
    return eError;
}


OMX_ERRORTYPE NBAMRDECGetCorresponding_LCMLHeader(AMRDEC_COMPONENT_PRIVATE *pComponentPrivate,
                                          OMX_U8 *pBuffer,
                                          OMX_DIRTYPE eDir,
                                          LCML_NBAMRDEC_BUFHEADERTYPE **ppLcmlHdr)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    LCML_NBAMRDEC_BUFHEADERTYPE *pLcmlBufHeader;
    
    OMX_S16 nIpBuf;
    OMX_S16 nOpBuf;
    OMX_S16 i;

    nIpBuf = pComponentPrivate->pInputBufferList->numBuffers;
    nOpBuf = pComponentPrivate->pOutputBufferList->numBuffers;
    
    while (!pComponentPrivate->bInitParamsInitialized) 
    {
        OMX_PRINT2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Waiting for init to complete\n",__LINE__);
#ifndef UNDER_CE
        sched_yield();
#else
        Sleep(0);
#endif
    }
    OMX_PRDSP1(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Inside NBAMRDECGetCorresponding_LCMLHeader..\n",__LINE__);
    OMX_PRINT2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: pComponentPrivate = %p\n",__LINE__,pComponentPrivate);

    OMX_PRINT2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: eDir = %d\n",__LINE__,eDir);
    if(eDir == OMX_DirInput) {
        OMX_PRINT2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c ::\n",__LINE__);
    OMX_PRDSP1(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Inside NBAMRDECGetCorresponding_LCMLHeader..\n",__LINE__);
    OMX_PRINT2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: pComponentPrivate = %p\n",__LINE__,pComponentPrivate);
        pLcmlBufHeader = pComponentPrivate->pLcmlBufHeader[NBAMRDEC_INPUT_PORT];
    OMX_PRDSP1(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Inside NBAMRDECGetCorresponding_LCMLHeader..\n",__LINE__);
        for(i=0; i<nIpBuf; i++) {
    OMX_PRDSP1(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Inside NBAMRDECGetCorresponding_LCMLHeader..\n",__LINE__);
            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: pBuffer = %p\n",__LINE__,pBuffer);
    OMX_PRDSP1(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Inside NBAMRDECGetCorresponding_LCMLHeader..\n",__LINE__);
            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: pLcmlBufHeader->buffer->pBuffer = %p\n",__LINE__,pLcmlBufHeader->buffer->pBuffer);
    OMX_PRDSP1(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Inside NBAMRDECGetCorresponding_LCMLHeader..\n",__LINE__);
            if(pBuffer == pLcmlBufHeader->buffer->pBuffer) {
    OMX_PRDSP1(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Inside NBAMRDECGetCorresponding_LCMLHeader..\n",__LINE__);
                *ppLcmlHdr = pLcmlBufHeader;
                OMX_PRDSP2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Corresponding LCML Header Found\n",__LINE__);
                 goto EXIT;
            }
            pLcmlBufHeader++;
        }
    } else if (eDir == OMX_DirOutput) {
        OMX_PRINT2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c ::\n",__LINE__);
        pLcmlBufHeader = pComponentPrivate->pLcmlBufHeader[NBAMRDEC_OUTPUT_PORT];
        OMX_PRINT2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c ::\n",__LINE__);

        for(i=0; i<nOpBuf; i++) {
        OMX_PRINT2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c ::\n",__LINE__);

            if(pBuffer == pLcmlBufHeader->buffer->pBuffer) {

                OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: pBuffer = %p\n",__LINE__,pBuffer);
                OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: pLcmlBufHeader->buffer->pBuffer = %p\n",__LINE__,pLcmlBufHeader->buffer->pBuffer);

                *ppLcmlHdr = pLcmlBufHeader;
        OMX_PRINT2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c ::\n",__LINE__);

                 OMX_PRDSP2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Corresponding LCML Header Found\n",__LINE__);
        OMX_PRINT2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c ::\n",__LINE__);

                 goto EXIT;
            }
            pLcmlBufHeader++;
        }
    } else {
        OMX_PRINT2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c ::\n",__LINE__);
      OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Invalid Buffer Type :: exiting...\n",__LINE__);
    }
        OMX_PRINT2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c ::\n",__LINE__);

EXIT:
    return eError;
}


#ifndef UNDER_CE

OMX_HANDLETYPE NBAMRDECGetLCMLHandle(AMRDEC_COMPONENT_PRIVATE *pComponentPrivate)
{
    void *handle;
    OMX_ERRORTYPE (*fpGetHandle)(OMX_HANDLETYPE);
    OMX_HANDLETYPE pHandle = NULL;
    OMX_S8 *error;
    OMX_ERRORTYPE eError;

    OMX_PRINT1(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: NBAMRDECGetLCMLHandle\n",__LINE__);
    handle = dlopen("libLCML.so", RTLD_LAZY);
    if (!handle) {
        fputs(dlerror(), stderr);
        goto EXIT;
    }

    fpGetHandle = dlsym (handle, "GetHandle");
    if ((error = (void*)dlerror()) != NULL) 
    {
        fputs((void*)error, stderr);
	dlclose(handle);
        goto EXIT;
    }
    eError = (*fpGetHandle)(&pHandle);
    if(eError != OMX_ErrorNone) {
        eError = OMX_ErrorUndefined;
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: eError != OMX_ErrorNone...\n",__LINE__);
        pHandle = NULL;
        dlclose(handle);
        goto EXIT;
    }
    pComponentPrivate->bLcmlHandleOpened = 1;
 
    ((LCML_DSP_INTERFACE*)pHandle)->pComponentPrivate = pComponentPrivate;
 
    pComponentPrivate->ptrLibLCML=handle;            /* saving LCML lib pointer  */

EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: NBAMRDECGetLCMLHandle returning %p\n",__LINE__,pHandle);

    return pHandle;
}


#else
//WINDOWS Explicit dll load procedure
OMX_HANDLETYPE NBAMRDECGetLCMLHandle(AMRDEC_COMPONENT_PRIVATE *pComponentPrivate)
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
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: BML Load Failed!!!\n",__LINE__);
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
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: eError != OMX_ErrorNone...\n",__LINE__);

        pHandle = NULL;
        return pHandle;
    }

    ((LCML_DSP_INTERFACE*)pHandle)->pComponentPrivate = pComponentPrivate;
    return pHandle;
}
#endif


#ifndef UNDER_CE

OMX_ERRORTYPE NBAMRDECFreeLCMLHandle(AMRDEC_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_S16 retValue;
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


OMX_ERRORTYPE NBAMRDECFreeLCMLHandle(AMRDEC_COMPONENT_PRIVATE *pComponentPrivate)
{

    OMX_S16 retValue;
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
void NBAMRDEC_SetPending(AMRDEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir) 
{
    OMX_S16 i;

    if (eDir == OMX_DirInput) {
        for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pInputBufferList->pBufHdr[i]) {
                pComponentPrivate->pInputBufferList->bBufferPending[i] = 1;
                OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: *****INPUT BUFFER %d IS PENDING****\n",__LINE__,i);
            }
        }
    }
    else {
        for (i=0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pOutputBufferList->pBufHdr[i]) {
                pComponentPrivate->pOutputBufferList->bBufferPending[i] = 1;
                OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: *****OUTPUT BUFFER %d IS PENDING****\n",__LINE__,i);
            }
        }
    }
}

void NBAMRDEC_ClearPending(AMRDEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir) 
{
    OMX_S16 i;

    if (eDir == OMX_DirInput) {
        for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pInputBufferList->pBufHdr[i]) {
                pComponentPrivate->pInputBufferList->bBufferPending[i] = 0;
                OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: ****INPUT BUFFER %d IS RECLAIMED****\n",__LINE__,i);
            }
        }
    }
    else {
        for (i=0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pOutputBufferList->pBufHdr[i]) {
                pComponentPrivate->pOutputBufferList->bBufferPending[i] = 0;
                OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: ****OUTPUT BUFFER %d IS RECLAIMED****\n",__LINE__,i);
            }
        }
    }
}

OMX_U32 NBAMRDEC_IsPending(AMRDEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir) 
{
    OMX_S16 i;

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


OMX_U32 NBAMRDEC_IsValid(AMRDEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U8 *pBuffer, OMX_DIRTYPE eDir) 
{
    OMX_S16 i;
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


OMX_ERRORTYPE  NBAMRDECFill_LCMLInitParamsEx (OMX_HANDLETYPE  pComponent )
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nIpBuf,nIpBufSize,nOpBuf,nOpBufSize;
    OMX_U16 i;
    OMX_BUFFERHEADERTYPE *pTemp;
    OMX_S16 size_lcml;
    char *ptr;
        
    LCML_DSP_INTERFACE *pHandle = (LCML_DSP_INTERFACE *)pComponent;
    AMRDEC_COMPONENT_PRIVATE *pComponentPrivate;
    LCML_NBAMRDEC_BUFHEADERTYPE *pTemp_lcml;

    OMXDBG_PRINT(stderr, PRINT, 1, 0, "%d :: OMX_AmrDec_Utils.c :: NBAMRDECFill_LCMLInitParams\n ",__LINE__);
    OMXDBG_PRINT(stderr, PRINT, 2, 0, "%d :: OMX_AmrDec_Utils.c :: pHandle = %p\n",__LINE__,pHandle);
    OMXDBG_PRINT(stderr, PRINT, 2, 0, "%d :: OMX_AmrDec_Utils.c :: pHandle->pComponentPrivate = %p\n",__LINE__,pHandle->pComponentPrivate);
    pComponentPrivate = pHandle->pComponentPrivate;

    nIpBuf = pComponentPrivate->pInputBufferList->numBuffers;
    
    if(pComponentPrivate->mimemode == 1) {
        nIpBufSize = INPUT_NBAMRDEC_BUFFER_SIZE_MIME;
    }
    else {
        if (OMX_AUDIO_AMRDTXasEFR == pComponentPrivate->iAmrMode){
            nIpBufSize = INPUT_BUFF_SIZE_EFR;
        } 
        else{
            nIpBufSize = STD_NBAMRDEC_BUF_SIZE;              
        }        
    }

    nOpBuf = pComponentPrivate->pOutputBufferList->numBuffers;
    nOpBufSize = OUTPUT_NBAMRDEC_BUFFER_SIZE;


    size_lcml = (OMX_S16)nIpBuf * sizeof(LCML_NBAMRDEC_BUFHEADERTYPE);
    OMX_MALLOC_SIZE(ptr, size_lcml,char);
    pTemp_lcml = (LCML_NBAMRDEC_BUFHEADERTYPE *)ptr;
    
    pComponentPrivate->pLcmlBufHeader[NBAMRDEC_INPUT_PORT] = pTemp_lcml;

    for (i=0; i<nIpBuf; i++) {
        pTemp = pComponentPrivate->pInputBufferList->pBufHdr[i];
        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);
/*        pTemp->nAllocLen = nIpBufSize;*/
        pTemp->nFilledLen = nIpBufSize;
        pTemp->nVersion.s.nVersionMajor = AMRDEC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = AMRDEC_MINOR_VER;
        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = NOT_USED;
        pTemp_lcml->buffer = pTemp;
        pTemp_lcml->eDir = OMX_DirInput;
    
        OMX_MALLOC_SIZE_DSPALIGN(pTemp_lcml->pBufferParam,
                                sizeof(NBAMRDEC_ParamStruct),
                                NBAMRDEC_ParamStruct); 

        
        OMX_MALLOC_GENERIC(pTemp_lcml->pDmmBuf, DMM_BUFFER_OBJ);
        pTemp_lcml->pFrameParam = NULL;
        pTemp_lcml->pBufferParam->usNbFrames =0;
        pTemp_lcml->pBufferParam->pParamElem = NULL;

        /* This means, it is not a last buffer. This flag is to be modified by
         * the application to indicate the last buffer */
        pTemp->nFlags = NORMAL_BUFFER;

        pTemp_lcml++;
    }

    /* Allocate memory for all output buffer headers..
       * This memory pointer will be sent to LCML */
    size_lcml = (OMX_S16)nOpBuf * sizeof(LCML_NBAMRDEC_BUFHEADERTYPE);

    OMX_MALLOC_SIZE(pTemp_lcml, size_lcml,LCML_NBAMRDEC_BUFHEADERTYPE);

    pComponentPrivate->pLcmlBufHeader[NBAMRDEC_OUTPUT_PORT] = pTemp_lcml;

    for (i=0; i<nOpBuf; i++) {
        pTemp = pComponentPrivate->pOutputBufferList->pBufHdr[i];
        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);
/*        pTemp->nAllocLen = nOpBufSize;*/
        pTemp->nFilledLen = nOpBufSize;
        pTemp->nVersion.s.nVersionMajor = AMRDEC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = AMRDEC_MINOR_VER;
        pComponentPrivate->nVersion = pTemp->nVersion.nVersion;
        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = NOT_USED;
        pTemp_lcml->pFrameParam = NULL;
       
        OMX_MALLOC_SIZE_DSPALIGN(pTemp_lcml->pBufferParam,
                                sizeof(NBAMRDEC_ParamStruct),
                                NBAMRDEC_ParamStruct); 
                                
        pTemp_lcml->pBufferParam->usNbFrames =0;
        pTemp_lcml->pBufferParam->pParamElem = NULL;
        OMX_MALLOC_GENERIC(pTemp_lcml->pDmmBuf, DMM_BUFFER_OBJ);

        pTemp_lcml->buffer = pTemp;
        pTemp_lcml->eDir = OMX_DirOutput;
        OMX_PRDSP2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: pTemp_lcml = %p\n",__LINE__,pTemp_lcml);
        OMX_PRDSP2(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: pTemp_lcml->buffer = %p\n",__LINE__,pTemp_lcml->buffer);

        pTemp->nFlags = NORMAL_BUFFER;

        pTemp++;
        pTemp_lcml++;
    }
    pComponentPrivate->bPortDefsAllocated = 1;

    OMX_PRINT1(pComponentPrivate->dbg, "%d :: OMX_AmrDec_Utils.c :: Exiting NBAMRDECFill_LCMLInitParams",__LINE__);

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
    OMX_PRDSP2 (dbg, "\nOMX Reserve DSP: %p\n",pDmmBuf->pReserved);
    
    if(DSP_FAILED(status))
    {
        OMX_ERROR4 (dbg, "DSPProcessor_ReserveMemory() failed - error 0x%x", (int)status);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }
    pDmmBuf->nSize = size;
    OMX_PRBUFFER2 (dbg, " DMM MAP Reserved: %p, size 0x%x (%d)\n", pDmmBuf->pReserved,nSizeReserved,nSizeReserved);
    
    /* Map */
    status = DSPProcessor_Map(ProcHandle,
                              pDmmBuf->pAllocated,/* Arm addres of data to Map on DSP*/
                              size , /* size to Map on DSP*/
                              pDmmBuf->pReserved, /* reserved space */
                              &(pDmmBuf->pMapped), /* returned map pointer */
                              0); /* final param is reserved.  set to zero. */
    if(DSP_FAILED(status))
    {
        OMX_ERROR4 (dbg, "DSPProcessor_Map() failed - error 0x%x", (int)status);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }
    OMX_PRBUFFER2 (dbg, "DMM Mapped: %p, size 0x%x (%d)\n",pDmmBuf->pMapped, size,size);

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
    OMX_PRDSP2 (dbg, "\nOMX UnReserve DSP: %p\n",pResPtr);

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

void SignalIfAllBuffersAreReturned(AMRDEC_COMPONENT_PRIVATE *pComponentPrivate)
{
    if((pComponentPrivate->nEmptyThisBufferCount == pComponentPrivate->nEmptyBufferDoneCount) && (pComponentPrivate->nFillThisBufferCount == pComponentPrivate->nFillBufferDoneCount))
    {
        if(pthread_mutex_lock(&bufferReturned_mutex) != 0) 
        {
            OMX_ERROR4(pComponentPrivate->dbg, "%d :: UTIL: bufferReturned_mutex mutex lock error\n",__LINE__);
        }
        pthread_cond_broadcast(&bufferReturned_condition);
        OMX_PRINT2(pComponentPrivate->dbg, ":: Sending pthread signal that OMX has returned all buffers to app\n"); 
        if(pthread_mutex_unlock(&bufferReturned_mutex) != 0) 
        {
            OMX_ERROR4(pComponentPrivate->dbg, "%d :: bufferReturned_mutex mutex unlock error\n",__LINE__);
        }
        return;
    }
}
#ifdef RESOURCE_MANAGER_ENABLED
void NBAMR_ResourceManagerCallback(RMPROXY_COMMANDDATATYPE cbData)
{
    OMX_COMMANDTYPE Cmd = OMX_CommandStateSet;
    OMX_STATETYPE state = OMX_StateIdle;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)cbData.hComponent;
    AMRDEC_COMPONENT_PRIVATE *pCompPrivate = NULL;

    pCompPrivate = (AMRDEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

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

void NBAMRDEC_HandleUSNError (AMRDEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U32 arg)
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
