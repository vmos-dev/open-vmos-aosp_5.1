
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
 * @file OMX_G722Dec_Utils.c
 *
 * This file implements various utilitiy functions for various activities
 * like handling command from application, callback from LCML etc.
 *
 * @path  $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\g722_dec\src
 *
 * @rev  1.0
 */
/* ----------------------------------------------------------------------------
 *!
 *! Revision History
 *! ===================================
 *! 21-sept-2006 bk: updated some review findings for alpha release
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
#include <stdlib.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <malloc.h>
#include <memory.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#endif

#include <pthread.h>
#include <dbapi.h>
#include <string.h>
#include <stdio.h>

/*------- Program Header Files -----------------------------------------------*/
#include "LCML_DspCodec.h"
#include "OMX_G722Dec_Utils.h"
#include "g722decsocket_ti.h"

#include <decode_common_ti.h>
#include "usn.h"


#ifdef UNDER_CE
#define HASHINGENABLE 1
void sleep(DWORD Duration)
{
    Sleep(Duration);

}
#endif

#ifndef UNDER_CE
#define G722DEC_DPRINT_ON(...)  fprintf(stdout, "%s %d::  ",__FUNCTION__, __LINE__); \
    fprintf(stdout, __VA_ARGS__);                                       \
    fprintf(stdout, "\n");
#endif

/* ================================================================================= * */
/**
 * @fn G722DEC_ComponentThread() This is component thread that keeps listening for
 * commands or event/messages/buffers from application or from LCML.
 *
 * @param pThreadData This is thread argument.
 *
 * @pre          None
 *
 * @post         None
 *
 *  @return      OMX_ErrorNone = Always
 *
 *  @see         None
 */
/* ================================================================================ * */
void* G722DEC_ComponentThread (void* pThreadData)
{
    int status = 0;
    struct timespec tv;
    int fdmax = 0;
    fd_set rfds;
    OMX_U32 nRet = 0;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    G722DEC_COMPONENT_PRIVATE* pComponentPrivate = (G722DEC_COMPONENT_PRIVATE*)pThreadData;
    OMX_COMPONENTTYPE *pHandle = pComponentPrivate->pHandle;

    G722DEC_DPRINT (":: Entering ComponentThread \n");

    fdmax = pComponentPrivate->cmdPipe[0];

    if (pComponentPrivate->dataPipe[0] > fdmax) {
        fdmax = pComponentPrivate->dataPipe[0];
    }

    while (1) {
        FD_ZERO (&rfds);
        FD_SET (pComponentPrivate->cmdPipe[0], &rfds);
        FD_SET (pComponentPrivate->dataPipe[0], &rfds);
        tv.tv_sec = 1;
        tv.tv_nsec = 0;

        sigset_t set;
        sigemptyset (&set);
        sigaddset (&set, SIGALRM);
        status = pselect (fdmax+1, &rfds, NULL, NULL, &tv, &set);

        if (0 == status) {
            G722DEC_DPRINT("\n\n\n!!!!!  Component Time Out !!!!!!!!!!!! \n");
            if (pComponentPrivate->bExitCompThrd == 1) {
                G722DEC_DPRINT(":: Comp Thrd Exiting here...\n");
                goto EXIT;
            }

        } else if (-1 == status) {
            G722DEC_DPRINT (":: Error in Select\n");
            pComponentPrivate->cbInfo.EventHandler (
                                                    pHandle,pHandle->pApplicationPrivate,
                                                    OMX_EventError,OMX_ErrorInsufficientResources, 0,
                                                    "Error from COmponent Thread in select");
            eError = OMX_ErrorInsufficientResources;

        } else if ((FD_ISSET (pComponentPrivate->dataPipe[0], &rfds))) {
            /*                   && (pComponentPrivate->curState != OMX_StatePause)) { */
            OMX_BUFFERHEADERTYPE *pBufHeader = NULL;

            G722DEC_DPRINT (":: DATA pipe is set in Component Thread\n");
            int ret = read(pComponentPrivate->dataPipe[0], &pBufHeader, sizeof(pBufHeader));
            if (ret == -1) {
                G722DEC_DPRINT (":: Error while reading from the pipe\n");
            }

            eError = G722DEC_HandleDataBuf_FromApp (pBufHeader,pComponentPrivate);
            if (eError != OMX_ErrorNone) {
                G722DEC_DPRINT (":: Error From HandleDataBuf_FromApp\n");
                break;
            }
        } else if (FD_ISSET (pComponentPrivate->cmdPipe[0], &rfds)) {
            G722DEC_DPRINT (":: CMD pipe is set in Component Thread\n");

            nRet = G722DEC_HandleCommand (pComponentPrivate);
            if (nRet == EXIT_COMPONENT_THRD) {
                G722DEC_DPRINT ("Exiting from Component thread\n");
                G722DEC_CleanupInitParams(pHandle);
                G722DEC_STATEPRINT("****************** Component State Set to Loaded\n\n");

                pComponentPrivate->curState = OMX_StateLoaded;
                pComponentPrivate->cbInfo.EventHandler(
                                                       pHandle, pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_ErrorNone,pComponentPrivate->curState, NULL);
            }
        }
    }
 EXIT:

    pComponentPrivate->bCompThreadStarted = 0;

    G722DEC_DPRINT (":: Exiting ComponentThread \n");
    return (void*)eError;
}
/* ================================================================================= * */
/**
 * @fn G722DEC_Fill_LCMLInitParams() fills the LCML initialization structure.
 *
 * @param pHandle This is component handle allocated by the OMX core.
 *
 * @param plcml_Init This structure is filled and sent to LCML.
 *
 * @pre          None
 *
 * @post         None
 *
 *  @return      OMX_ErrorNone = Successful Inirialization of the LCML struct.
 *               OMX_ErrorInsufficientResources = Not enough memory
 *
 *  @see         None
 */
/* ================================================================================ * */
OMX_ERRORTYPE G722DEC_Fill_LCMLInitParams(OMX_HANDLETYPE pComponent,
                                          LCML_DSP *plcml_Init, OMX_U16 arr[])
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nIpBuf = 0,nIpBufSize = 0,nOpBuf = 0,nOpBufSize = 0;
    OMX_U32 i = 0;
    OMX_BUFFERHEADERTYPE *pTemp = NULL;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    G722DEC_COMPONENT_PRIVATE *pComponentPrivate =
        (G722DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    G722D_LCML_BUFHEADERTYPE *pTemp_lcml = NULL;
    OMX_U32 size_lcml = 0;
    /*int inputPortFlag=0,outputPortFlag=0;*/
    LCML_STRMATTR *strmAttr = NULL;
    OMX_U8 *ptemp = NULL;

    G722DEC_DPRINT(":: Entered Fill_LCMLInitParams");

    G722DEC_DPRINT("%d :: OMX_StateLoaded [G722D_INPUT_PORT]->bPopulated  %d \n",
                   __LINE__,pComponentPrivate->pPortDef[G722D_INPUT_PORT]->bPopulated);
    G722DEC_DPRINT("%d :: OMX_StateLoaded [G722D_INPUT_PORT]->bEnabled    %d \n",
                   __LINE__,pComponentPrivate->pPortDef[G722D_INPUT_PORT]->bEnabled);
    G722DEC_DPRINT("%d :: OMX_StateLoaded [G722D_OUTPUT_PORT]->bPopulated %d \n",
                   __LINE__,pComponentPrivate->pPortDef[G722D_OUTPUT_PORT]->bPopulated);
    G722DEC_DPRINT("%d :: OMX_StateLoaded [G722D_OUTPUT_PORT]->bEnabled   %d \n",
                   __LINE__,pComponentPrivate->pPortDef[G722D_OUTPUT_PORT]->bEnabled);

    pComponentPrivate->strmAttr = NULL;

    nIpBuf = pComponentPrivate->pInputBufferList->numBuffers;
    pComponentPrivate->nRuntimeInputBuffers = nIpBuf;
    nOpBuf = pComponentPrivate->pOutputBufferList->numBuffers;
    pComponentPrivate->nRuntimeOutputBuffers = nOpBuf;
    nIpBufSize = pComponentPrivate->pPortDef[G722D_INPUT_PORT]->nBufferSize;
    nOpBufSize = pComponentPrivate->pPortDef[G722D_OUTPUT_PORT]->nBufferSize;

    G722DEC_BUFPRINT("Input Buffer Count = %ld\n",nIpBuf);
    G722DEC_BUFPRINT("Input Buffer Size = %ld\n",nIpBufSize);
    G722DEC_BUFPRINT("Output Buffer Count = %ld\n",nOpBuf);
    G722DEC_BUFPRINT("Output Buffer Size = %ld\n",nOpBufSize);

    plcml_Init->In_BufInfo.nBuffers = nIpBuf;
    plcml_Init->In_BufInfo.nSize = nIpBufSize;
    plcml_Init->In_BufInfo.DataTrMethod = DMM_METHOD;
    plcml_Init->Out_BufInfo.nBuffers = nOpBuf;
    plcml_Init->Out_BufInfo.nSize = nOpBufSize;
    plcml_Init->Out_BufInfo.DataTrMethod = DMM_METHOD;


    plcml_Init->NodeInfo.nNumOfDLLs = 3;

    memset(plcml_Init->NodeInfo.AllUUIDs[0].DllName,0, 
           sizeof(plcml_Init->NodeInfo.AllUUIDs[0].DllName));
    memset(plcml_Init->NodeInfo.AllUUIDs[1].DllName,0, 
           sizeof(plcml_Init->NodeInfo.AllUUIDs[1].DllName));
    memset(plcml_Init->NodeInfo.AllUUIDs[2].DllName,0, 
           sizeof(plcml_Init->NodeInfo.AllUUIDs[1].DllName));
    memset(plcml_Init->NodeInfo.AllUUIDs[0].DllName,0, 
           sizeof(plcml_Init->DeviceInfo.AllUUIDs[1].DllName));

    plcml_Init->NodeInfo.AllUUIDs[0].uuid = &G722DECSOCKET_TI_UUID;
    strcpy ((char*)plcml_Init->NodeInfo.AllUUIDs[0].DllName, G722DEC_DLL_NAME);
    plcml_Init->NodeInfo.AllUUIDs[0].eDllType = DLL_NODEOBJECT;

    plcml_Init->NodeInfo.AllUUIDs[1].uuid = &G722DECSOCKET_TI_UUID;
    strcpy ((char*)plcml_Init->NodeInfo.AllUUIDs[1].DllName, G722DEC_DLL_NAME);
    plcml_Init->NodeInfo.AllUUIDs[1].eDllType = DLL_DEPENDENT;

    plcml_Init->NodeInfo.AllUUIDs[2].uuid = &USN_TI_UUID;
    strcpy ((char*)plcml_Init->NodeInfo.AllUUIDs[2].DllName, USN_DLL_NAME);
    plcml_Init->NodeInfo.AllUUIDs[2].eDllType = DLL_DEPENDENT;

    plcml_Init->SegID = OMX_G722DEC_DEFAULT_SEGMENT;
    plcml_Init->Timeout = OMX_G722DEC_SN_TIMEOUT;
    plcml_Init->Alignment = 0;
    plcml_Init->Priority = OMX_G722DEC_SN_PRIORITY;

    plcml_Init->ProfileID = 0;

    G722DEC_BUFPRINT("DLL name0 = %s\n",plcml_Init->NodeInfo.AllUUIDs[0].DllName);
    G722DEC_BUFPRINT("DLL name1 = %s\n",plcml_Init->NodeInfo.AllUUIDs[1].DllName);
    G722DEC_BUFPRINT("DLL name2 = %s\n",plcml_Init->NodeInfo.AllUUIDs[2].DllName);

    if(pComponentPrivate->dasfmode == 1) {
#ifndef DSP_RENDERING_ON
        G722D_OMX_ERROR_EXIT(eError, OMX_ErrorInsufficientResources,
                             "Flag DSP_RENDERING_ON Must Be Defined To Use Rendering");
#else
        G722D_OMX_MALLOC(strmAttr,LCML_STRMATTR);
        pComponentPrivate->strmAttr = strmAttr;
        G722DEC_DPRINT(":: G722 DECODER IS RUNNING UNDER DASF MODE \n");

        strmAttr->uSegid = OMX_G722DEC_DEFAULT_SEGMENT;
        strmAttr->uAlignment = 0;
        strmAttr->uTimeout = OMX_G722DEC_SN_TIMEOUT;
        strmAttr->uBufsize = nOpBufSize;
        strmAttr->uNumBufs = G722D_NUM_OUTPUT_BUFFERS;
        strmAttr->lMode = STRMMODE_PROCCOPY;
        plcml_Init->DeviceInfo.TypeofDevice = 1;
        plcml_Init->DeviceInfo.TypeofRender = 0;
        plcml_Init->DeviceInfo.AllUUIDs[0].uuid = &DCTN_TI_UUID;
        plcml_Init->DeviceInfo.DspStream = strmAttr;
#endif
    } else {
        plcml_Init->DeviceInfo.TypeofDevice = 0;
    }

    if (pComponentPrivate->dasfmode == 0){
        G722DEC_DPRINT(":: FILE MODE CREATE PHASE PARAMETERS\n");
        arr[0] = 2;
        arr[1] = 0;
        arr[2] = 0;

        if(pComponentPrivate->pInputBufferList->numBuffers == 0) {
            arr[3] = G722D_NUM_INPUT_BUFFERS;
        } else {
            arr[3] = (OMX_U16) nIpBuf;
        }

        arr[4] = 1;
        arr[5] = 0;
        if(pComponentPrivate->pOutputBufferList->numBuffers == 0) {
            arr[6] = G722D_NUM_OUTPUT_BUFFERS;
        } else {
            arr[6] = (OMX_U16) nOpBuf;
        }

    } else {
        G722DEC_DPRINT(":: DASF MODE CREATE PHASE PARAMETERS\n");
        arr[0] = 2;
        arr[1] = 0;
        arr[2] = 0;

        if(pComponentPrivate->pInputBufferList->numBuffers == 0) {
            arr[3] = G722D_NUM_INPUT_BUFFERS;
        } else {
            arr[3] = (OMX_U16) nIpBuf;
        }

        arr[4] = 1;
        arr[5] = 2;
        arr[6] = G722D_NUM_OUTPUT_BUFFERS;
    }

    /* set codec selection (64kbps <-> 0 / 56kbps <-> 1 / 48kbps <-> 2) */
    if (pComponentPrivate->G722Params[G722D_INPUT_PORT]->nSamplingRate == 64000) {
        arr[7] = 0;
    } else if (pComponentPrivate->G722Params[G722D_INPUT_PORT]->nSamplingRate == 56000) {
        arr[7] = 1;
    } else if (pComponentPrivate->G722Params[G722D_INPUT_PORT]->nSamplingRate == 48000) {
        arr[7] = 2;
    } else {
        /* Free previously allocated memory before bailing */
        /*G722D_OMX_FREE(strmAttr);
          G722D_OMX_FREE(arr);*/
        G722D_OMX_ERROR_EXIT(eError, OMX_ErrorBadParameter,
                             "Input bit rate not supported");
    }
    arr[8] = END_OF_CR_PHASE_ARGS;
    plcml_Init->pCrPhArgs = arr;

    G722DEC_DPRINT(":: bufAlloced = %d\n",pComponentPrivate->bufAlloced);
    size_lcml = nIpBuf * sizeof(G722D_LCML_BUFHEADERTYPE);
    G722D_OMX_MALLOC_SIZE(pTemp_lcml,size_lcml,G722D_LCML_BUFHEADERTYPE);

    pComponentPrivate->pLcmlBufHeader[G722D_INPUT_PORT] = pTemp_lcml;

    for (i=0; i<nIpBuf; i++) {
        pTemp = pComponentPrivate->pInputBufferList->pBufHdr[i];

        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        G722DEC_DPRINT("IP: pTemp->nSize = %ld\n",pTemp->nSize);

        /*pTemp->nAllocLen = nIpBufSize;*/
        pTemp->nFilledLen = nIpBufSize;
        pTemp->nVersion.s.nVersionMajor = G722DEC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = G722DEC_MINOR_VER;

        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = DONT_CARE;

        pTemp_lcml->pBufHdr = pTemp;
        pTemp_lcml->eDir = OMX_DirInput;
        pTemp_lcml->pOtherParams[i] = NULL;

        G722D_OMX_MALLOC(pTemp_lcml->pIpParam,G722DEC_ParamStruct);
        pTemp_lcml->pIpParam->usLastFrame = 0;

        pTemp->nFlags = NORMAL_BUFFER;
        ((G722DEC_COMPONENT_PRIVATE *) pTemp->pPlatformPrivate)->pHandle = pHandle;

        G722DEC_DPRINT("::Comp: InBuffHeader[%ld] = %p\n", i, pTemp);
        G722DEC_DPRINT("::Comp:  >>>> InputBuffHeader[%ld]->pBuffer = %p\n", i, pTemp->pBuffer);
        G722DEC_DPRINT("::Comp: Ip : pTemp_lcml[%ld] = %p\n", i, pTemp_lcml);

        pTemp_lcml++;
    }

    size_lcml = nOpBuf * sizeof(G722D_LCML_BUFHEADERTYPE);
    G722D_OMX_MALLOC_SIZE(pTemp_lcml,size_lcml,G722D_LCML_BUFHEADERTYPE);
    pComponentPrivate->pLcmlBufHeader[G722D_OUTPUT_PORT] = pTemp_lcml;

    for (i=0; i<nOpBuf; i++) {
        pTemp = pComponentPrivate->pOutputBufferList->pBufHdr[i];

        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);

        pTemp->nAllocLen = nOpBufSize;
        pTemp->nVersion.s.nVersionMajor = G722DEC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = G722DEC_MINOR_VER;


        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = DONT_CARE;

        pTemp_lcml->pBufHdr = pTemp;
        pTemp_lcml->eDir = OMX_DirOutput;
        pTemp_lcml->pOtherParams[i] = NULL;

        pTemp->nFlags = NORMAL_BUFFER;
        ((G722DEC_COMPONENT_PRIVATE *)pTemp->pPlatformPrivate)->pHandle = pHandle;
        G722DEC_DPRINT("::Comp:  >>>>>>>>>>>>> OutBuffHeader[%ld] = %p\n", i, pTemp);
        G722DEC_DPRINT("::Comp:  >>>> OutBuffHeader[%ld]->pBuffer = %p\n", i, pTemp->pBuffer);
        G722DEC_DPRINT("::Comp: Op : pTemp_lcml[%ld] = %p\n", i, pTemp_lcml);
        pTemp_lcml++;
    }
    
    G722D_OMX_MALLOC_SIZE(pComponentPrivate->pParams,(sizeof(G722D_USN_AudioCodecParams) + 256),
                          G722D_USN_AudioCodecParams);
    ptemp = (OMX_U8*)pComponentPrivate->pParams;
    ptemp += 128;
    pComponentPrivate->pParams = (G722D_USN_AudioCodecParams*)ptemp;
    
    pComponentPrivate->bPortDefsAllocated = 1;
    pComponentPrivate->bInitParamsInitialized = 1;

 EXIT:
    if(eError == OMX_ErrorInsufficientResources || eError == OMX_ErrorBadParameter){
        G722D_OMX_FREE(strmAttr);
        G722D_OMX_FREE(arr);
        G722D_OMX_FREE(pTemp_lcml);
    }
    G722DEC_DPRINT("Exiting G722DEC_Fill_LCMLInitParams\n");
    return eError;
}

/* ================================================================================= * */
/**
 * @fn G722Dec_StartCompThread() starts the component thread. This is internal
 * function of the component.
 *
 * @param pHandle This is component handle allocated by the OMX core.
 *
 * @pre          None
 *
 * @post         None
 *
 *  @return      OMX_ErrorNone = Successful Inirialization of the component\n
 *               OMX_ErrorInsufficientResources = Not enough memory
 *
 *  @see         None
 */
/* ================================================================================ * */
OMX_ERRORTYPE G722Dec_StartCompThread(OMX_HANDLETYPE pComponent)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    G722DEC_COMPONENT_PRIVATE *pComponentPrivate =
        (G722DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    int nRet = 0;
#ifdef UNDER_CE
    pthread_attr_t attr;
    memset(&attr, 0, sizeof(attr));
    attr.__inheritsched = PTHREAD_EXPLICIT_SCHED;
    attr.__schedparam.__sched_priority = OMX_AUDIO_DECODER_THREAD_PRIORITY;
#endif

    G722DEC_DPRINT (":: Enetering  G722Dec_StartCompThread()\n");

    pComponentPrivate->lcml_nOpBuf = 0;
    pComponentPrivate->lcml_nIpBuf = 0;
    pComponentPrivate->app_nBuf = 0;
    pComponentPrivate->num_Op_Issued = 0;
    pComponentPrivate->num_Sent_Ip_Buff = 0;
    pComponentPrivate->num_Reclaimed_Op_Buff = 0;
    pComponentPrivate->bIsEOFSent = 0;

    nRet = pipe (pComponentPrivate->dataPipe);
    if (0 != nRet) {
        G722D_OMX_ERROR_EXIT(eError, OMX_ErrorInsufficientResources,
                             "Pipe Creation Failed");
    }

    nRet = pipe (pComponentPrivate->cmdPipe);
    if (0 != nRet) {
        G722D_OMX_ERROR_EXIT(eError, OMX_ErrorInsufficientResources,
                             "Pipe Creation Failed");
    }

    nRet = pipe (pComponentPrivate->cmdDataPipe);
    if (0 != nRet) {
        G722D_OMX_ERROR_EXIT(eError, OMX_ErrorInsufficientResources,
                             "Pipe Creation Failed");
    }

    nRet = pipe (pComponentPrivate->lcml_Pipe);
    if (0 != nRet) {
        G722D_OMX_ERROR_EXIT(eError, OMX_ErrorInsufficientResources,
                             "Pipe Creation Failed");
    }

#ifdef UNDER_CE
    nRet = pthread_create (&(pComponentPrivate->ComponentThread), &attr,
                           G722DEC_ComponentThread, pComponentPrivate);
#else
    nRet = pthread_create (&(pComponentPrivate->ComponentThread), NULL,
                           G722DEC_ComponentThread, pComponentPrivate);
#endif
    if ((0 != nRet) || (!pComponentPrivate->ComponentThread)) {
        G722D_OMX_ERROR_EXIT(eError, OMX_ErrorInsufficientResources,
                             "Thread Creation Failed");
    }

    pComponentPrivate->bCompThreadStarted = 1;
    G722DEC_DPRINT (":: Exiting from G722Dec_StartCompThread()\n");

 EXIT:
    return eError;
}


/* ================================================================================= * */
/**
 * @fn G722DEC_FreeCompResources() function frees the component resources.
 *
 * @param pComponent This is the component handle.
 *
 * @pre          None
 *
 * @post         None
 *
 *  @return      OMX_ErrorNone = Successful Inirialization of the component\n
 *               OMX_ErrorHardware = Hardware error has occured.
 *
 *  @see         None
 */
/* ================================================================================ * */
OMX_ERRORTYPE G722DEC_FreeCompResources(OMX_HANDLETYPE pComponent)
{
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    G722DEC_COMPONENT_PRIVATE *pComponentPrivate = (G722DEC_COMPONENT_PRIVATE *)
        pHandle->pComponentPrivate;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nIpBuf=0, nOpBuf=0;
    int nRet=0;

    G722DEC_DPRINT (":: G722Dec_FreeCompResources\n");

    G722DEC_DPRINT(":::pComponentPrivate->bPortDefsAllocated = %ld\n",
                   pComponentPrivate->bPortDefsAllocated);
    if (pComponentPrivate->bPortDefsAllocated) {
        nIpBuf = pComponentPrivate->pInputBufferList->numBuffers;
        nOpBuf = pComponentPrivate->pOutputBufferList->numBuffers;
    }
    G722DEC_DPRINT(":: Closing pipess.....\n");

    nRet = close (pComponentPrivate->dataPipe[0]);
    if (0 != nRet && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
    }

    nRet = close (pComponentPrivate->dataPipe[1]);
    if (0 != nRet && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
    }

    nRet = close (pComponentPrivate->cmdPipe[0]);
    if (0 != nRet && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
    }

    nRet = close (pComponentPrivate->cmdPipe[1]);
    if (0 != nRet && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
    }

    nRet = close (pComponentPrivate->cmdDataPipe[0]);
    if (0 != nRet && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
    }

    nRet = close (pComponentPrivate->cmdDataPipe[1]);
    if (0 != nRet && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
    }

    nRet = close (pComponentPrivate->lcml_Pipe[0]);
    if (0 != nRet && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
    }

    nRet = close (pComponentPrivate->lcml_Pipe[1]);
    if (0 != nRet && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
    }

    if (pComponentPrivate->bPortDefsAllocated) {
        G722D_OMX_FREE(pComponentPrivate->pPortDef[G722D_INPUT_PORT]);
        G722D_OMX_FREE(pComponentPrivate->pPortDef[G722D_OUTPUT_PORT]);
        G722D_OMX_FREE(pComponentPrivate->G722Params[G722D_INPUT_PORT]);
        G722D_OMX_FREE(pComponentPrivate->G722Params[G722D_OUTPUT_PORT]);
        G722D_OMX_FREE(pComponentPrivate->pCompPort[G722D_INPUT_PORT]->pPortFormat);
        G722D_OMX_FREE(pComponentPrivate->pCompPort[G722D_OUTPUT_PORT]->pPortFormat);
        G722D_OMX_FREE(pComponentPrivate->pCompPort[G722D_INPUT_PORT]);
        G722D_OMX_FREE(pComponentPrivate->pCompPort[G722D_OUTPUT_PORT]);
        G722D_OMX_FREE(pComponentPrivate->sPortParam);
        G722D_OMX_FREE(pComponentPrivate->pPriorityMgmt);
        G722D_OMX_FREE(pComponentPrivate->pInputBufferList);
        G722D_OMX_FREE(pComponentPrivate->pOutputBufferList);
        G722D_OMX_FREE(pComponentPrivate->componentRole);
    } 

    pComponentPrivate->bPortDefsAllocated = 0;

#ifndef UNDER_CE
    G722DEC_DPRINT("\n\n FreeCompResources: Destroying mutexes.\n\n");
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

    return eError;
}

/* ================================================================================= * */
/**
 * @fn G722DEC_HandleCommand() function handles the command sent by the application.
 * All the state transitions, except from nothing to loaded state, of the
 * component are done by this function.
 *
 * @param pComponentPrivate  This is component's private date structure.
 *
 * @pre          None
 *
 * @post         None
 *
 *  @return      OMX_ErrorNone = Successful processing.
 *               OMX_ErrorInsufficientResources = Not enough memory
 *               OMX_ErrorHardware = Hardware error has occured lile LCML failed
 *               to do any said operartion.
 *
 *  @see         None
 */
/* ================================================================================ * */
OMX_U32 G722DEC_HandleCommand (G722DEC_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_U32 i = 0,ret = 0;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    char *pArgs = "damedesuStr";
    OMX_COMPONENTTYPE *pHandle =
        (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;
    OMX_COMMANDTYPE command;
    OMX_STATETYPE commandedState = OMX_StateInvalid;
    OMX_U32 commandData = 0;
    OMX_HANDLETYPE pLcmlHandle = pComponentPrivate->pLcmlHandle;

    /* L89FLUSH */
    int numCalls = 0;
    OMX_U16 arr[10] = {0};

    G722DEC_DPRINT (":: >>> Entering HandleCommand Function\n");

    ret = read (pComponentPrivate->cmdPipe[0], &command, sizeof (command));
    if(ret == -1){
        G722DEC_DPRINT ("%d :: Error in Reading from the Data pipe\n", __LINE__);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }
    ret = read (pComponentPrivate->cmdDataPipe[0], &commandData, sizeof (commandData));
    if(ret == -1){
        G722DEC_DPRINT ("%d :: Error in Reading from the Data pipe\n", __LINE__);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }
    G722DEC_DPRINT("---------------------------------------------\n");
    G722DEC_DPRINT(":: command = %d\n",command);
    G722DEC_DPRINT(":: commandData = %ld\n",commandData);
    G722DEC_DPRINT("---------------------------------------------\n");

    if (command == OMX_CommandStateSet) {
        commandedState = (OMX_STATETYPE)commandData;
        if (pComponentPrivate->curState == commandedState) {
            pComponentPrivate->cbInfo.EventHandler (pHandle, pHandle->pApplicationPrivate,
                                                    OMX_EventError, OMX_ErrorSameState,0,
                                                    NULL);
            G722DEC_DPRINT(":: Error: Same State Given by Application\n");
        } else {
            switch(commandedState) {
            case OMX_StateIdle:
                G722DEC_DPRINT(": HandleCommand: Cmd Idle \n");
                if (pComponentPrivate->curState == OMX_StateLoaded) {
                    LCML_CALLBACKTYPE cb;
                    LCML_DSP *pLcmlDsp = NULL;
                    char *p = "damedesuStr";
                    int inputPortFlag=0,outputPortFlag=0;

                    if (pComponentPrivate->dasfmode == 1) {
                        pComponentPrivate->pPortDef[G722D_OUTPUT_PORT]->bEnabled= FALSE;
                        pComponentPrivate->pPortDef[G722D_OUTPUT_PORT]->bPopulated= FALSE;
                        if(pComponentPrivate->streamID == 0) { 
                            G722DEC_DPRINT("**************************************\n"); 
                            G722DEC_DPRINT(":: Error = OMX_ErrorInsufficientResources\n"); 
                            G722DEC_DPRINT("**************************************\n"); 
                            eError = OMX_ErrorInsufficientResources; 
                            pComponentPrivate->curState = OMX_StateInvalid; 
                            pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                                   pHandle->pApplicationPrivate, 
                                                                   OMX_EventError, OMX_ErrorInvalidState,
                                                                   0, 
                                                                   NULL);
                            goto EXIT; 
                        }
                    }

                    G722DEC_DPRINT("%d :: OMX_StateLoaded [G722D_INPUT_PORT]->bPopulated  %d \n",
                                   __LINE__,pComponentPrivate->pPortDef[G722D_INPUT_PORT]->bPopulated);
                    G722DEC_DPRINT("%d :: OMX_StateLoaded [G722D_INPUT_PORT]->bEnabled    %d \n",
                                   __LINE__,pComponentPrivate->pPortDef[G722D_INPUT_PORT]->bEnabled);
                    G722DEC_DPRINT("%d :: OMX_StateLoaded [G722D_OUTPUT_PORT]->bPopulated %d \n",
                                   __LINE__,pComponentPrivate->pPortDef[G722D_OUTPUT_PORT]->bPopulated);
                    G722DEC_DPRINT("%d :: OMX_StateLoaded [G722D_OUTPUT_PORT]->bEnabled   %d \n",
                                   __LINE__,pComponentPrivate->pPortDef[G722D_OUTPUT_PORT]->bEnabled);

                    if (pComponentPrivate->pPortDef[G722D_INPUT_PORT]->bPopulated &&  
                        pComponentPrivate->pPortDef[G722D_INPUT_PORT]->bEnabled)  {
                        inputPortFlag = 1;
                    }
                    if (!pComponentPrivate->pPortDef[G722D_INPUT_PORT]->bPopulated && 
                        !pComponentPrivate->pPortDef[G722D_INPUT_PORT]->bEnabled) {
                        inputPortFlag = 1;
                    }
                    if (pComponentPrivate->pPortDef[G722D_OUTPUT_PORT]->bPopulated && 
                        pComponentPrivate->pPortDef[G722D_OUTPUT_PORT]->bEnabled) {
                        outputPortFlag = 1;
                    }
                    if (!pComponentPrivate->pPortDef[G722D_OUTPUT_PORT]->bPopulated && 
                        !pComponentPrivate->pPortDef[G722D_OUTPUT_PORT]->bEnabled) {
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

                    pLcmlHandle = (OMX_HANDLETYPE) G722DEC_GetLCMLHandle(pComponentPrivate);
                    if (pLcmlHandle == NULL) {
                        G722DEC_DPRINT(":: LCML Handle is NULL........exiting..\n");
                        pComponentPrivate->curState = OMX_StateInvalid;
                        pComponentPrivate->cbInfo.EventHandler(pHandle, pHandle->pApplicationPrivate,
                                                               OMX_EventError, OMX_ErrorInvalidState,
                                                               0, NULL);
                        goto EXIT;
                    }

                    pLcmlDsp = (((LCML_DSP_INTERFACE*)pLcmlHandle)->dspCodec);
                    eError = G722DEC_Fill_LCMLInitParams(pHandle, pLcmlDsp, arr);
                    if(eError != OMX_ErrorNone) {
                        G722DEC_DPRINT(":: Error returned from Fill_LCMLInitParams()\n");
                        pComponentPrivate->curState = OMX_StateInvalid;
                        pComponentPrivate->cbInfo.EventHandler(pHandle, pHandle->pApplicationPrivate,
                                                               OMX_EventError, OMX_ErrorInvalidState,
                                                               0, NULL);
                        goto EXIT;
                    }

                    pComponentPrivate->pLcmlHandle = (LCML_DSP_INTERFACE *)pLcmlHandle;
                    cb.LCML_Callback = (void *) G722DEC_LCML_Callback;

#ifndef UNDER_CE
                    eError = LCML_InitMMCodecEx(((LCML_DSP_INTERFACE *)pLcmlHandle)->pCodecinterfacehandle,
                                                p,&pLcmlHandle,(void *)p,
                                                &cb,(OMX_STRING)pComponentPrivate->sDeviceString);
                    if (eError != OMX_ErrorNone){
                        G722DEC_DPRINT("%d :: Error : InitMMCodec failed...>>>>>> \n",__LINE__);
                        goto EXIT;
                    }
#else
                    eError = LCML_InitMMCodec(((LCML_DSP_INTERFACE *)pLcmlHandle)->pCodecinterfacehandle,
                                              p,&pLcmlHandle,(void *)p,&cb);
                    if (eError != OMX_ErrorNone){
                        G722DEC_DPRINT("%d :: Error : InitMMCodec failed...>>>>>> \n",__LINE__);
                        goto EXIT;
                    }
#endif

#ifdef HASHINGENABLE
                    /* Enable the Hashing Code */
                    eError = LCML_SetHashingState(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle, 
                                                  OMX_TRUE);
                    if (eError != OMX_ErrorNone) {
                        G722DEC_DPRINT("Failed to set Mapping State\n");
                        goto EXIT;
                    }
#endif


                    G722DEC_DPRINT(":: Control Came Here\n");
                    G722DEC_STATEPRINT("****************** Component State Set to Idle\n\n");
                    pComponentPrivate->curState = OMX_StateIdle;
                    pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventCmdComplete,
                                                           OMX_CommandStateSet,
                                                           pComponentPrivate->curState,
                                                           NULL);

                    G722DEC_DPRINT("G722DEC: State has been Set to Idle\n");

                } else if (pComponentPrivate->curState == OMX_StateExecuting) {
#ifdef HASHINGENABLE
                    /*Hashing Change*/
                    pLcmlHandle = (LCML_DSP_INTERFACE*)pComponentPrivate->pLcmlHandle;
                    /* clear out any mappings that might have accumulated */
                    eError = LCML_FlushHashes(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle);
                    if (eError != OMX_ErrorNone) {
                        G722DEC_DPRINT("Error occurred in Codec mapping flush!\n");
                        break;
                    }
#endif
                    pComponentPrivate->bDspStoppedWhileExecuting = OMX_TRUE;                    
                    G722DEC_DPRINT(":: In HandleCommand: Stopping the codec\n");
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               MMCodecControlStop,(void *)pArgs);
                    if(eError != OMX_ErrorNone) {
                        G722DEC_DPRINT(": Error Occurred in Codec Stop..\n");
                        pComponentPrivate->curState = OMX_StateInvalid;
                        pComponentPrivate->cbInfo.EventHandler(pHandle, pHandle->pApplicationPrivate,
                                                               OMX_EventError, OMX_ErrorInvalidState,0, NULL);
                        goto EXIT;
                    }
                } else if(pComponentPrivate->curState == OMX_StatePause) {
                    char *pArgs = "damedesuStr";
#ifdef HASHINGENABLE
                    /*Hashing Change*/
                    pLcmlHandle = (LCML_DSP_INTERFACE*)pComponentPrivate->pLcmlHandle;
                    /* clear out any mappings that might have accumulated */
                    eError = LCML_FlushHashes(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle);
                    if (eError != OMX_ErrorNone) {
                        G722DEC_DPRINT("Error occurred in Codec mapping flush!\n");
                        break;
                    }
#endif
                    G722DEC_DPRINT(":: Comp: Stop Command Received\n");
                    G722DEC_DPRINT(": G722DECUTILS::About to call LCML_ControlCodec\n");
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               MMCodecControlStop,(void *)pArgs);
                    if(eError != OMX_ErrorNone) {
                        G722DEC_DPRINT(": Error Occurred in Codec Stop..\n");
                        pComponentPrivate->curState = OMX_StateInvalid;
                        pComponentPrivate->cbInfo.EventHandler(pHandle, pHandle->pApplicationPrivate,
                                                               OMX_EventError, OMX_ErrorInvalidState,0, NULL);
                        goto EXIT;
                    }
                    G722DEC_STATEPRINT("****************** Component State Set to Idle\n\n");
                    pComponentPrivate->curState = OMX_StateIdle;

                    G722DEC_DPRINT ("%d :: The component is stopped\n",__LINE__);
                    pComponentPrivate->cbInfo.EventHandler(pHandle, pHandle->pApplicationPrivate,
                                                           OMX_EventCmdComplete, OMX_CommandStateSet, 
                                                           pComponentPrivate->curState, NULL);
                } else {
                    G722DEC_DPRINT(": Comp: Sending ErrorNotification: Invalid State\n");
                    pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventError,
                                                           OMX_ErrorIncorrectStateTransition, 0,
                                                           "Invalid State Error");
                }
                break;

            case OMX_StateExecuting:
                G722DEC_DPRINT(": HandleCommand: Cmd Executing \n");
                if (pComponentPrivate->curState == OMX_StateIdle) {
                    char *pArgs = "damedesuStr";
                    if(pComponentPrivate->dasfmode == 1) {
                        OMX_U32 pValues[4];
                        pComponentPrivate->pParams->unUUID = pComponentPrivate->streamID;
                        pComponentPrivate->pParams->ulSamplingFreq = 
                            pComponentPrivate->G722Params[G722D_OUTPUT_PORT]->nSamplingRate;
                        pComponentPrivate->pParams->unAudioFormat = 1; /*MONO stream */

                        G722DEC_DPRINT("::pComponentPrivate->pParams->unAudioFormat = %d\n",
                                       pComponentPrivate->pParams->unAudioFormat);
                        G722DEC_DPRINT("::pComponentPrivate->pParams->ulSamplingFreq = %ld\n",
                                       pComponentPrivate->pParams->ulSamplingFreq);
                        G722DEC_DPRINT("::pComponentPrivate->pParams->unUUID = %ld\n",
                                       pComponentPrivate->pParams->unUUID);

                        pValues[0] = USN_STRMCMD_SETCODECPARAMS;
                        pValues[1] = (OMX_U32)pComponentPrivate->pParams;
                        pValues[2] = sizeof(G722D_USN_AudioCodecParams);

                        eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                   EMMCodecControlStrmCtrl,(void *)pValues);
                        if(eError != OMX_ErrorNone) {
                            G722DEC_DPRINT(": Error Occurred in Codec StreamControl..\n");
                            pComponentPrivate->curState = OMX_StateInvalid;
                            pComponentPrivate->cbInfo.EventHandler(pHandle, pHandle->pApplicationPrivate,
                                                                   OMX_EventError, OMX_ErrorInvalidState,0, NULL);
                            goto EXIT;
                        }
                    }

                    pComponentPrivate->bDspStoppedWhileExecuting = OMX_FALSE; 
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               EMMCodecControlStart,(void *)pArgs);
                    if(eError != OMX_ErrorNone) {
                        G722DEC_DPRINT("Error Occurred in Codec Start..\n");
                        pComponentPrivate->curState = OMX_StateInvalid;
                        pComponentPrivate->cbInfo.EventHandler(pHandle, pHandle->pApplicationPrivate,
                                                               OMX_EventError, OMX_ErrorInvalidState,0, NULL);
                        goto EXIT;
                    }
                } else if (pComponentPrivate->curState == OMX_StatePause) {
                    char *pArgs = "damedesuStr";
                    G722DEC_DPRINT(": Comp: Resume Command Came from App\n");
                    G722DEC_DPRINT(": G722DECUTILS::About to call LCML_ControlCodec\n");
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               EMMCodecControlStart,(void *)pArgs);
                    if (eError != OMX_ErrorNone) {
                        G722DEC_DPRINT ("Error While Resuming the codec\n");
                        pComponentPrivate->curState = OMX_StateInvalid;
                        pComponentPrivate->cbInfo.EventHandler(pHandle, pHandle->pApplicationPrivate,
                                                               OMX_EventError, OMX_ErrorInvalidState,0, NULL);
                        goto EXIT;
                    }

                    for(i=0; i< pComponentPrivate->nPendingOutPausedBufs; i++){
                        pComponentPrivate->cbInfo.FillBufferDone( pHandle,
                                                                  pHandle->pApplicationPrivate,
                                                                  pComponentPrivate->pOutBufHdrWhilePaused[i]);
                        pComponentPrivate->nOutStandingFillDones--;
                        pComponentPrivate->lcml_nOpBuf--;
                        pComponentPrivate->app_nBuf++;
                        pComponentPrivate->nFillBufferDoneCount++;

                    }
                    pComponentPrivate->nPendingOutPausedBufs = 0;

                    if (pComponentPrivate->nNumInputBufPending < pComponentPrivate->pInputBufferList->numBuffers) {
                        pComponentPrivate->nNumInputBufPending = pComponentPrivate->pInputBufferList->numBuffers;
                    }
                    for (i=0; i < pComponentPrivate->nNumInputBufPending; i++) {
                        if (pComponentPrivate->pInputBufHdrPending[i]) {
                            G722D_LCML_BUFHEADERTYPE *pLcmlHdr;
                            G722DEC_GetCorresponding_LCMLHeader(pComponentPrivate,
                                                                pComponentPrivate->pInputBufHdrPending[i]->pBuffer,
                                                                OMX_DirInput, &pLcmlHdr);
                            G722DEC_SetPending(pComponentPrivate,pComponentPrivate->pInputBufHdrPending[i],OMX_DirInput,
                                               __LINE__);
                            eError = LCML_QueueBuffer(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                      EMMCodecInputBuffer,
                                                      pComponentPrivate->pInputBufHdrPending[i]->pBuffer,
                                                      pComponentPrivate->pInputBufHdrPending[i]->nAllocLen,
                                                      pComponentPrivate->pInputBufHdrPending[i]->nFilledLen,
                                                      (OMX_U8 *) pLcmlHdr->pIpParam,
                                                      sizeof(G722DEC_ParamStruct),
                                                      NULL);
                        }
                    }
                    pComponentPrivate->nNumInputBufPending = 0;
                    if (pComponentPrivate->nNumOutputBufPending < pComponentPrivate->pOutputBufferList->numBuffers) {
                        pComponentPrivate->nNumOutputBufPending = pComponentPrivate->pOutputBufferList->numBuffers;
                    }
                    for (i=0; i < pComponentPrivate->nNumOutputBufPending; i++) {
                        if (pComponentPrivate->pOutputBufHdrPending[i]) {
                            G722D_LCML_BUFHEADERTYPE *pLcmlHdr;
                            G722DEC_GetCorresponding_LCMLHeader(pComponentPrivate,
                                                                pComponentPrivate->pOutputBufHdrPending[i]->pBuffer, 
                                                                OMX_DirOutput, &pLcmlHdr);
                            if (!G722DEC_IsPending(pComponentPrivate, pComponentPrivate->pOutputBufHdrPending[i], 
                                                   OMX_DirOutput)) {
                                G722DEC_SetPending(pComponentPrivate,pComponentPrivate->pOutputBufHdrPending[i],
                                                   OMX_DirOutput,__LINE__);
                                eError = LCML_QueueBuffer(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                          EMMCodecOuputBuffer,
                                                          pComponentPrivate->pOutputBufHdrPending[i]->pBuffer,
                                                          pComponentPrivate->pOutputBufHdrPending[i]->nAllocLen,
                                                          pComponentPrivate->pOutputBufHdrPending[i]->nFilledLen,
                                                          NULL,
                                                          sizeof(G722DEC_ParamStruct),
                                                          NULL);
                            }
                        }
                    }
                    pComponentPrivate->nNumOutputBufPending = 0;
                }else {
                    pComponentPrivate->cbInfo.EventHandler (pHandle, pHandle->pApplicationPrivate,
                                                            OMX_EventError, OMX_ErrorIncorrectStateTransition, 0,
                                                            "Invalid State");
                    G722DEC_DPRINT(":: Error: Invalid State Given by \
                       Application\n");
                    goto EXIT;
                }

                G722DEC_STATEPRINT("****************** Component State Set to Executing\n\n");
                pComponentPrivate->curState = OMX_StateExecuting;

                pComponentPrivate->cbInfo.EventHandler(pHandle, pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandStateSet, pComponentPrivate->curState, NULL);
                break;

            case OMX_StateLoaded:
                G722DEC_DPRINT(": HandleCommand: Cmd Loaded\n");
                if (pComponentPrivate->curState == OMX_StateWaitForResources ){
                    G722DEC_STATEPRINT("***************** Component State Set to Loaded\n\n");
                    pComponentPrivate->curState = OMX_StateLoaded;
                    pComponentPrivate->cbInfo.EventHandler (pHandle, pHandle->pApplicationPrivate,
                                                            OMX_EventCmdComplete, OMX_CommandStateSet,
                                                            pComponentPrivate->curState,
                                                            NULL);
                    G722DEC_DPRINT(":: Tansitioning from WaitFor to Loaded\n");
                    break;
                }
                if (pComponentPrivate->curState != OMX_StateIdle) {
                    pComponentPrivate->cbInfo.EventHandler (pHandle, pHandle->pApplicationPrivate,
                                                            OMX_EventError, OMX_ErrorIncorrectStateTransition, 0,
                                                            "Invalid State");
                    G722DEC_DPRINT(":: Error: Invalid State Given by \
                       Application\n");
                    goto EXIT;
                }

                G722DEC_DPRINT("Current State = %d\n",pComponentPrivate->curState);
                G722DEC_DPRINT("pComponentPrivate->pInputBufferList->numBuffers = %ld\n",
                               pComponentPrivate->pInputBufferList->numBuffers);
                G722DEC_DPRINT("pComponentPrivate->pOutputBufferList->numBuffers = %ld\n",
                               pComponentPrivate->pOutputBufferList->numBuffers);

                G722DEC_DPRINT(":: Loaded State - in while(1) loop: ip : %ld : op: %ld\n",
                               pComponentPrivate->pInputBufferList->numBuffers,
                               pComponentPrivate->pOutputBufferList->numBuffers);
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
                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                           EMMCodecControlDestroy,(void *)pArgs);
                pComponentPrivate->bInitParamsInitialized = 0;
                eError = EXIT_COMPONENT_THRD;
                break;

            case OMX_StatePause:
                G722DEC_DPRINT("Cmd Pause: Cur State = %d\n", pComponentPrivate->curState);

                if ((pComponentPrivate->curState != OMX_StateExecuting) &&
                    (pComponentPrivate->curState != OMX_StateIdle)) {
                    pComponentPrivate->cbInfo.EventHandler (pHandle, pHandle->pApplicationPrivate,
                                                            OMX_EventError, OMX_ErrorIncorrectStateTransition, 0,
                                                            "Invalid State");
                    G722DEC_DPRINT(":: Error: Invalid State Given by \
                       Application\n");
                    goto EXIT;
                }

                G722DEC_DPRINT(": G722DECUTILS::About to call LCML_ControlCodec\n");
                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                           EMMCodecControlPause,(void *)pArgs);

                if (eError != OMX_ErrorNone) {
                    G722DEC_DPRINT(": Error: in Pausing the codec\n");
                    pComponentPrivate->curState = OMX_StateInvalid;
                    pComponentPrivate->cbInfo.EventHandler(pHandle, pHandle->pApplicationPrivate,
                                                           OMX_EventError, OMX_ErrorInvalidState,0, NULL);
                    goto EXIT;
                }
                G722DEC_DPRINT(":: Component: Codec Is Paused\n");

                G722DEC_STATEPRINT("****************** Component State Set to Pause\n\n");
                pComponentPrivate->curState = OMX_StatePause;
                pComponentPrivate->cbInfo.EventHandler(pHandle, pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete, OMX_CommandStateSet,
                                                       pComponentPrivate->curState, NULL);
                break;

            case OMX_StateWaitForResources:
                G722DEC_DPRINT(": HandleCommand: Cmd : OMX_StateWaitForResources\n");
                if (pComponentPrivate->curState == OMX_StateLoaded) {
                    pComponentPrivate->curState = OMX_StateWaitForResources;
                    G722DEC_DPRINT(": Transitioning from Loaded to OMX_StateWaitForResources\n");
                    pComponentPrivate->cbInfo.EventHandler(pHandle, pHandle->pApplicationPrivate,
                                                           OMX_EventCmdComplete,
                                                           OMX_CommandStateSet,pComponentPrivate->curState, NULL);
                } else {
                    pComponentPrivate->cbInfo.EventHandler(
                                                           pHandle, pHandle->pApplicationPrivate,
                                                           OMX_EventError, OMX_ErrorIncorrectStateTransition,0, NULL);
                }
                break;

            case OMX_StateInvalid:
                G722DEC_DPRINT(": HandleCommand: Cmd OMX_StateInvalid:\n");
                pComponentPrivate->curState = OMX_StateInvalid;

                pComponentPrivate->cbInfo.EventHandler(
                                                       pHandle, pHandle->pApplicationPrivate,
                                                       OMX_EventError, OMX_ErrorInvalidState,0, NULL);
                break;

            case OMX_StateMax:
                G722DEC_DPRINT(": HandleCommand: Cmd OMX_StateMax::\n");
                break;
            } /* End of Switch */
        }
    }
    else if (command == OMX_CommandMarkBuffer) {
        G722DEC_DPRINT("command OMX_CommandMarkBuffer received\n");
        if(!pComponentPrivate->pMarkBuf){
            G722DEC_DPRINT("command OMX_CommandMarkBuffer received \n");
            pComponentPrivate->pMarkBuf = (OMX_MARKTYPE *)(commandData);
        }
    }
    else if (command == OMX_CommandPortDisable) {
        if (!pComponentPrivate->bDisableCommandPending) {
            if(commandData == 0x0){
                /* disable port */
                for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
                    if (G722DEC_IsPending(pComponentPrivate,pComponentPrivate->pInputBufferList->pBufHdr[i],
                                          OMX_DirInput)) {
                        /* Real solution is flush buffers from DSP.  Until we have the ability to do that
                           we just call EmptyBufferDone() on any pending buffers */
                        pComponentPrivate->cbInfo.EmptyBufferDone (pComponentPrivate->pHandle,
                                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                                   pComponentPrivate->pInputBufferList->pBufHdr[i]);
                        pComponentPrivate->nEmptyBufferDoneCount++;
                    }
                }
                pComponentPrivate->pPortDef[G722D_INPUT_PORT]->bEnabled = OMX_FALSE;
            }
            if(commandData == -1){
                pComponentPrivate->pPortDef[G722D_INPUT_PORT]->bEnabled = OMX_FALSE;
            }
            if(commandData == 0x1 || commandData == -1){
                char *pArgs = "damedesuStr";
                pComponentPrivate->pPortDef[G722D_OUTPUT_PORT]->bEnabled = OMX_FALSE;
                if (pComponentPrivate->curState == OMX_StateExecuting) {
                    pComponentPrivate->bNoIdleOnStop = OMX_TRUE;
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               MMCodecControlStop,(void *)pArgs);
                }
            }
        }
        G722DEC_DPRINT("commandData = %ld\n",commandData);
        G722DEC_DPRINT("pComponentPrivate->pPortDef[G722D_INPUT_PORT]->bPopulated = %d\n",
                       pComponentPrivate->pPortDef[G722D_INPUT_PORT]->bPopulated);
        G722DEC_DPRINT("pComponentPrivate->pPortDef[G722D_OUTPUT_PORT]->bPopulated = %d\n",
                       pComponentPrivate->pPortDef[G722D_OUTPUT_PORT]->bPopulated);
        if(commandData == 0x0) {
            if(!pComponentPrivate->pPortDef[G722D_INPUT_PORT]->bPopulated){
                /* return cmdcomplete event if input unpopulated */
                pComponentPrivate->cbInfo.EventHandler(pHandle, pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete, OMX_CommandPortDisable,
                                                       G722D_INPUT_PORT, NULL);
                pComponentPrivate->bDisableCommandPending = 0;
            }
            else{
                pComponentPrivate->bDisableCommandPending = 1;
                pComponentPrivate->bDisableCommandParam = commandData;
            }
        }
        if(commandData == 0x1) {
            if (!pComponentPrivate->pPortDef[G722D_OUTPUT_PORT]->bPopulated){
                /* return cmdcomplete event if output unpopulated */
                pComponentPrivate->cbInfo.EventHandler(pHandle, pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete, OMX_CommandPortDisable,
                                                       G722D_OUTPUT_PORT, NULL);
                pComponentPrivate->bDisableCommandPending = 0;
            }
            else {
                pComponentPrivate->bDisableCommandPending = 1;
                pComponentPrivate->bDisableCommandParam = commandData;
            }
        }
        if(commandData == -1) {
            if (!pComponentPrivate->pPortDef[G722D_INPUT_PORT]->bPopulated &&
                !pComponentPrivate->pPortDef[G722D_OUTPUT_PORT]->bPopulated){
                /* return cmdcomplete event if inout & output unpopulated */
                pComponentPrivate->cbInfo.EventHandler(pHandle, pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete, OMX_CommandPortDisable,
                                                       G722D_INPUT_PORT, NULL);
                pComponentPrivate->cbInfo.EventHandler(pHandle, pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete, OMX_CommandPortDisable,
                                                       G722D_OUTPUT_PORT, NULL);
                pComponentPrivate->bDisableCommandPending = 0;
            }
            else {
                pComponentPrivate->bDisableCommandPending = 1;
                pComponentPrivate->bDisableCommandParam = commandData;
            }
#ifndef UNDER_CE
            sched_yield();
#else
            Sleep(0);
#endif
        }
    }
    else if (command == OMX_CommandPortEnable) {
        if(commandData == 0x0 || commandData == -1){
            /* enable in port */
            G722DEC_DPRINT("setting input port to enabled\n");
            pComponentPrivate->pPortDef[G722D_INPUT_PORT]->bEnabled = OMX_TRUE;
            G722DEC_DPRINT("pComponentPrivate->pPortDef[G722D_INPUT_PORT]->bEnabled = %d\n",
                           pComponentPrivate->pPortDef[G722D_INPUT_PORT]->bEnabled);

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
        }
        if(commandData == 0x1 || commandData == -1){
            char *pArgs = "damedesuStr";
            /* enable out port */
            G722DEC_DPRINT("setting output port to enabled\n");
            pComponentPrivate->pPortDef[G722D_OUTPUT_PORT]->bEnabled = OMX_TRUE;
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
            if(pComponentPrivate->curState == OMX_StateExecuting){
                pComponentPrivate->bDspStoppedWhileExecuting = OMX_FALSE;
                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                           EMMCodecControlStart,(void *)pArgs);
            }
            G722DEC_DPRINT("pComponentPrivate->pPortDef[G722D_OUTPUT_PORT]->bEnabled = %d\n",
                           pComponentPrivate->pPortDef[G722D_OUTPUT_PORT]->bEnabled);
        }
        while (1) {
            G722DEC_DPRINT("commandData = %ld\n",commandData);
            G722DEC_DPRINT("pComponentPrivate->curState = %d\n",pComponentPrivate->curState);
            G722DEC_DPRINT("pComponentPrivate->pPortDef[G722D_INPUT_PORT]->bPopulated = %d\n",
                           pComponentPrivate->pPortDef[G722D_INPUT_PORT]->bPopulated);

            if(commandData == 0x0 && (pComponentPrivate->curState == OMX_StateLoaded ||
                                      pComponentPrivate->pPortDef[G722D_INPUT_PORT]->bPopulated)){

                pComponentPrivate->cbInfo.EventHandler(pHandle, pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete, 
                                                       OMX_CommandPortEnable,G722D_INPUT_PORT,
                                                       NULL);
                break;
            }
            else if(commandData == 0x1 && (pComponentPrivate->curState == OMX_StateLoaded ||
                                           pComponentPrivate->pPortDef[G722D_OUTPUT_PORT]->bPopulated)){

                pComponentPrivate->cbInfo.EventHandler(pHandle, pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete, 
                                                       OMX_CommandPortEnable,G722D_OUTPUT_PORT,
                                                       NULL);
                break;
            }
            else if(commandData == -1 && (pComponentPrivate->curState == OMX_StateLoaded ||
                                          (pComponentPrivate->pPortDef[G722D_INPUT_PORT]->bPopulated &&
                                           pComponentPrivate->pPortDef[G722D_OUTPUT_PORT]->bPopulated))) {

                pComponentPrivate->cbInfo.EventHandler(pHandle, pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete, OMX_CommandPortEnable,
                                                       G722D_INPUT_PORT,
                                                       NULL);

                pComponentPrivate->cbInfo.EventHandler(pHandle, pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete, OMX_CommandPortEnable,
                                                       G722D_OUTPUT_PORT,
                                                       NULL);
                G722DECFill_LCMLInitParamsEx(pComponentPrivate->pHandle);
                break;
            }
#ifndef UNDER_CE
            sched_yield();
#else
            Sleep(0);
#endif
        }
    }
    else if (command == OMX_CommandFlush) {
        int j=0;
        G722DEC_DPRINT(":: Inside OMX_CommandFlush Command\n");
        if(commandData == 0x0 || commandData == -1) {
            for (i=0; i < MAX_NUM_OF_BUFS; i++) {
                pComponentPrivate->pInputBufHdrPending[i] = NULL;
            }
            pComponentPrivate->nNumInputBufPending=0;
            for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
                numCalls = pComponentPrivate->nEmptyThisBufferCount - pComponentPrivate->nEmptyBufferDoneCount;
                if (numCalls > 0) {
                    for (j=0; j < numCalls; j++) {
                        pComponentPrivate->cbInfo.EmptyBufferDone (pComponentPrivate->pHandle,
                                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                                   pComponentPrivate->pInputBufferList->pBufHdr[i]
                                                                   );
                        pComponentPrivate->nEmptyBufferDoneCount++;

                    }
                }
            }
            /* return all input buffers */
            pComponentPrivate->cbInfo.EventHandler(pHandle, pHandle->pApplicationPrivate,
                                                   OMX_EventCmdComplete, OMX_CommandFlush,G722D_INPUT_PORT, NULL);
        }
        if(commandData == 0x1 || commandData == -1){
            G722DEC_DPRINT("Flusing output queue\n");
            for (i=0; i < MAX_NUM_OF_BUFS; i++) {
                pComponentPrivate->pOutputBufHdrPending[i] = NULL;
            }
            pComponentPrivate->nNumOutputBufPending=0;
            /* return all output buffers */
            for (i=0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {
                pComponentPrivate->cbInfo.FillBufferDone (pComponentPrivate->pHandle,
                                                          pComponentPrivate->pHandle->pApplicationPrivate,
                                                          pComponentPrivate->pOutputBufferList->pBufHdr[i]);
                pComponentPrivate->nFillBufferDoneCount++;

            }
            pComponentPrivate->cbInfo.EventHandler(pHandle, pHandle->pApplicationPrivate,
                                                   OMX_EventCmdComplete, OMX_CommandFlush,G722D_OUTPUT_PORT, NULL);
        }
    }
 EXIT:
    G722DEC_DPRINT (":: Exiting HandleCommand Function\n");
    return eError;
}



/* ================================================================================= * */
/**
 * @fn G722DEC_HandleDataBuf_FromApp() function handles the input and output buffers
 * that come from the application. It is not direct function wich gets called by
 * the application rather, it gets called eventually.
 *
 * @param *pBufHeader This is the buffer header that needs to be processed.
 *
 * @param *pComponentPrivate  This is component's private date structure.
 *
 * @pre          None
 *
 * @post         None
 *
 *  @return      OMX_ErrorNone = Successful processing.
 *               OMX_ErrorInsufficientResources = Not enough memory
 *               OMX_ErrorHardware = Hardware error has occured lile LCML failed
 *               to do any said operartion.
 *
 *  @see         None
 */
/* ================================================================================ * */
OMX_ERRORTYPE G722DEC_HandleDataBuf_FromApp(OMX_BUFFERHEADERTYPE* pBufHeader,
                                            G722DEC_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_DIRTYPE eDir;
    G722DEC_DPRINT (":: Entering HandleDataBuf_FromApp Function\n");
    G722DEC_DPRINT (":: pBufHeader->pMarkData = %p\n",pBufHeader->pMarkData);

    pBufHeader->pPlatformPrivate  = pComponentPrivate;
    eError = G722DEC_GetBufferDirection(pBufHeader, &eDir);
    G722DEC_DPRINT (":: HandleDataBuf_FromApp Function\n");
    if (eError != OMX_ErrorNone) {
        G722DEC_DPRINT (":: The pBufHeader is not found in the list\n");
        goto EXIT;
    }

    if (eDir == OMX_DirInput) {
        LCML_DSP_INTERFACE *pLcmlHandle = (LCML_DSP_INTERFACE *)pComponentPrivate->pLcmlHandle;
        G722D_LCML_BUFHEADERTYPE *pLcmlHdr;

        eError = G722DEC_GetCorresponding_LCMLHeader(pComponentPrivate,
                                                     pBufHeader->pBuffer, OMX_DirInput, &pLcmlHdr);
        if (eError != OMX_ErrorNone) {
            G722DEC_DPRINT(":: Error: Invalid Buffer Came ...\n");
            goto EXIT;
        }

        /* Store time stamp information */ 
        pComponentPrivate->arrTimestamp[pComponentPrivate->IpBufindex] = pBufHeader->nTimeStamp; 
        /* Store nTickCount information */ 
        pComponentPrivate->arrTickCount[pComponentPrivate->IpBufindex] = pBufHeader->nTickCount; 
        pComponentPrivate->IpBufindex++; 
        pComponentPrivate->IpBufindex %= pComponentPrivate->pPortDef[OMX_DirInput]->nBufferCountActual; 
                             
        if (pBufHeader->nFilledLen > 0 || pBufHeader->nFlags == OMX_BUFFERFLAG_EOS) {
            pComponentPrivate->bBypassDSP = 0;
            G722DEC_DPRINT (":: HandleDataBuf_FromApp Function\n");
            G722DEC_DPRINT (":::Calling LCML_QueueBuffer\n");

            pLcmlHdr->pIpParam->usLastFrame = 0;
            if(pBufHeader->nFlags == OMX_BUFFERFLAG_EOS) {
                G722DEC_DPRINT(":: usLastFrame Is Set Here....\n");
                pLcmlHdr->pIpParam->usLastFrame = 1;
                pComponentPrivate->bIsEOFSent = 1;
                pBufHeader->nFlags = 0;
            }

            G722DEC_DPRINT ("Comp:: Sending Filled Input buffer = %p, %p to LCML\n",pBufHeader,pBufHeader->pBuffer);

            /* Store time stamp information */
            pComponentPrivate->arrTimestamp[pComponentPrivate->IpBufindex] = pBufHeader->nTimeStamp;
            pComponentPrivate->arrTickCount[pComponentPrivate->IpBufindex] = pBufHeader->nTickCount;                
            pComponentPrivate->IpBufindex++;
            pComponentPrivate->IpBufindex %= pComponentPrivate->pPortDef[G722D_INPUT_PORT]->nBufferCountActual; 
            if (pComponentPrivate->curState == OMX_StateExecuting) {
                if (!G722DEC_IsPending(pComponentPrivate,pBufHeader,OMX_DirInput)) {
                    if(!pComponentPrivate->bDspStoppedWhileExecuting) {
                        G722DEC_SetPending(pComponentPrivate,pBufHeader,OMX_DirInput,__LINE__);
                        eError = LCML_QueueBuffer(pLcmlHandle->pCodecinterfacehandle,
                                                  EMMCodecInputBuffer,
                                                  pBufHeader->pBuffer,
                                                  pBufHeader->nAllocLen,
                                                  pBufHeader->nFilledLen,
                                                  (OMX_U8 *) pLcmlHdr->pIpParam,
                                                  sizeof(G722DEC_ParamStruct),
                                                  pBufHeader->pBuffer);
                        if (eError != OMX_ErrorNone) {
                            G722DEC_DPRINT ("::Comp: SetBuff: IP: Error Occurred\n");
                            eError = OMX_ErrorHardware;
                            goto EXIT;
                        }

                        pComponentPrivate->lcml_nCntIp++;
                        pComponentPrivate->lcml_nIpBuf++;
                        pComponentPrivate->num_Sent_Ip_Buff++;
                        G722DEC_DPRINT ("Sending Input buffer to Codec\n");
                    }
                    else {
                        G722DEC_DPRINT("Calling EmptyBufferDone from line %d\n",__LINE__);
                        pComponentPrivate->cbInfo.EmptyBufferDone (pComponentPrivate->pHandle,
                                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                                   pBufHeader );
                    }

                }
            } else if (pComponentPrivate->curState == OMX_StatePause) {
                pComponentPrivate->pInputBufHdrPending[pComponentPrivate->nNumInputBufPending++] = pBufHeader;
            }
        } else {
            pComponentPrivate->bBypassDSP = 1;
            G722DEC_DPRINT ("Forcing EmptyBufferDone\n");
            if(pComponentPrivate->dasfmode == 0) {
                pComponentPrivate->cbInfo.EmptyBufferDone (pComponentPrivate->pHandle,
                                                           pComponentPrivate->pHandle->pApplicationPrivate,
                                                           pComponentPrivate->pInputBufferList->pBufHdr[0]);
                pComponentPrivate->nEmptyBufferDoneCount++;
            }
        }
        if(pBufHeader->pMarkData){
            G722DEC_DPRINT (":Detected pBufHeader->pMarkData\n");
            pComponentPrivate->pMarkData = pBufHeader->pMarkData;
            pComponentPrivate->hMarkTargetComponent = pBufHeader->hMarkTargetComponent;
            pComponentPrivate->pOutputBufferList->pBufHdr[0]->pMarkData = pBufHeader->pMarkData;
            pComponentPrivate->pOutputBufferList->pBufHdr[0]->hMarkTargetComponent = pBufHeader->hMarkTargetComponent;
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
    else if (eDir == OMX_DirOutput) {
        LCML_DSP_INTERFACE *pLcmlHandle = (LCML_DSP_INTERFACE *)pComponentPrivate->pLcmlHandle;
        G722D_LCML_BUFHEADERTYPE *pLcmlHdr = NULL;

        G722DEC_DPRINT(": pComponentPrivate->lcml_nOpBuf = %ld\n",
                       pComponentPrivate->lcml_nOpBuf);
        G722DEC_DPRINT(": pComponentPrivate->lcml_nIpBuf = %ld\n",
                       pComponentPrivate->lcml_nIpBuf);
        eError = G722DEC_GetCorresponding_LCMLHeader(pComponentPrivate,pBufHeader->pBuffer, OMX_DirOutput, &pLcmlHdr);
        if (eError != OMX_ErrorNone) {
            G722DEC_DPRINT(":: Error: Invalid Buffer Came ...\n");
            goto EXIT;
        }

        G722DEC_DPRINT (":::Calling LCML_QueueBuffer\n");
        if (pComponentPrivate->bBypassDSP == 0) {
            G722DEC_DPRINT ("Comp:: Sending Emptied Output buffer=%p to LCML\n",pBufHeader);
            if (pComponentPrivate->curState == OMX_StateExecuting) {
                G722DEC_DPRINT ("Comp:: in G722DEC UTILS pLcmlHandle->pCodecinterfacehandle= %p\n",
                                pLcmlHandle->pCodecinterfacehandle);
                G722DEC_DPRINT ("Comp:: in G722DEC UTILS EMMCodecOuputBuffer = %d\n",EMMCodecOuputBuffer);
                G722DEC_DPRINT ("Comp:: in G722DEC UTILS pBufHeader->pBuffer = %p\n",pBufHeader->pBuffer);
                G722DEC_DPRINT ("Comp:: in G722DEC UTILS pBufHeader->nAllocLen = %ld\n",pBufHeader->nAllocLen);
                if (!G722DEC_IsPending(pComponentPrivate,pBufHeader,OMX_DirOutput) &&
                    (pComponentPrivate->numPendingBuffers < pComponentPrivate->pOutputBufferList->numBuffers))  {
                    G722DEC_SetPending(pComponentPrivate,pBufHeader,OMX_DirOutput,__LINE__);
                    if(!pComponentPrivate->bDspStoppedWhileExecuting){
                        eError = LCML_QueueBuffer(pLcmlHandle->pCodecinterfacehandle,
                                                  EMMCodecOuputBuffer,
                                                  pBufHeader->pBuffer,
                                                  pBufHeader->nAllocLen,
                                                  0,
                                                  NULL,
                                                  sizeof(G722DEC_ParamStruct),
                                                  pBufHeader->pBuffer);
                        if (eError != OMX_ErrorNone ) {
                            G722DEC_DPRINT (":: Comp:: SetBuff OP: Error Occurred\n");
                            eError = OMX_ErrorHardware;
                            goto EXIT;
                        }
                        pComponentPrivate->lcml_nCntOp++;
                        pComponentPrivate->lcml_nOpBuf++;
                        pComponentPrivate->num_Op_Issued++;
                        G722DEC_DPRINT ("Comp:: in G722DEC UTILS \n");
                    }
                }
            } else {
                pComponentPrivate->pOutputBufHdrPending[pComponentPrivate->nNumOutputBufPending++] = pBufHeader;
            }
        }
    } /* end of OMX_DirOutput if struct */
    else {
        G722DEC_DPRINT(": BufferHeader %p, Buffer %p Unknown ..........\n",pBufHeader, pBufHeader->pBuffer);
        eError = OMX_ErrorBadParameter;
    }
 EXIT:
    G722DEC_DPRINT(": Exiting from  HandleDataBuf_FromApp: %x \n",eError);
    if(eError == OMX_ErrorBadParameter) {
        G722DEC_DPRINT(": Error = OMX_ErrorBadParameter\n");
    }
    return eError;
}


/* ================================================================================= * */
/**
 * @fn G722DEC_GetBufferDirection() function determines whether it is input buffer or
 * output buffer.
 *
 * @param *pBufHeader This is pointer to buffer header whose direction needs to
 *                    be determined.
 *
 * @param *eDir  This is output argument which stores the direction of buffer.
 *
 * @pre          None
 *
 * @post         None
 *
 *  @return      OMX_ErrorNone = Successful processing.
 *               OMX_ErrorBadParameter = In case of invalid buffer
 *
 *  @see         None
 */
/* ================================================================================ * */

OMX_ERRORTYPE G722DEC_GetBufferDirection(OMX_BUFFERHEADERTYPE *pBufHeader,
                                         OMX_DIRTYPE *eDir)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    G722DEC_COMPONENT_PRIVATE *pComponentPrivate = pBufHeader->pPlatformPrivate;
    OMX_U32 nBuf = pComponentPrivate->pInputBufferList->numBuffers;
    OMX_BUFFERHEADERTYPE *pBuf = NULL;
    int flag = 1;
    OMX_U32 i=0;

    G722DEC_DPRINT (":: Entering GetBufferDirection Function\n");
    for(i=0; i<nBuf; i++) {
        pBuf = pComponentPrivate->pInputBufferList->pBufHdr[i];
        if(pBufHeader == pBuf) {
            *eDir = OMX_DirInput;
            G722DEC_DPRINT (":: Buffer %p is INPUT BUFFER\n", pBufHeader);
            flag = 0;
            goto EXIT;
        }
    }

    nBuf = pComponentPrivate->pOutputBufferList->numBuffers;

    for(i=0; i<nBuf; i++) {
        pBuf = pComponentPrivate->pOutputBufferList->pBufHdr[i];
        if(pBufHeader == pBuf) {
            *eDir = OMX_DirOutput;
            G722DEC_DPRINT (":: Buffer %p is OUTPUT BUFFER\n", pBufHeader);
            flag = 0;
            goto EXIT;
        }
    }

    if (flag == 1) {
        G722D_OMX_ERROR_EXIT(eError, OMX_ErrorBadParameter,
                             "Buffer Not Found in List : OMX_ErrorBadParameter");
    }
 EXIT:
    G722DEC_DPRINT (":: Exiting GetBufferDirection Function\n");
    return eError;
}

/* ================================================================================= * */
/**
 * @fn G722DEC_LCML_Callback() function is callback which is called by LCML whenever
 * there is an even generated for the component.
 *
 * @param event  This is event that was generated.
 *
 * @param arg    This has other needed arguments supplied by LCML like handles
 *               etc.
 *
 * @pre          None
 *
 * @post         None
 *
 *  @return      OMX_ErrorNone = Successful processing.
 *               OMX_ErrorInsufficientResources = Not enough memory
 *
 *  @see         None
 */
/* ================================================================================ * */
OMX_ERRORTYPE G722DEC_LCML_Callback (TUsnCodecEvent event,void * args [10])
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U8 *pBuffer = args[1];
    G722D_LCML_BUFHEADERTYPE *pLcmlHdr = NULL;
    OMX_COMPONENTTYPE *pHandle = NULL;
    LCML_DSP_INTERFACE *pLcmlHandle = NULL;
    G722DEC_COMPONENT_PRIVATE* pComponentPrivate = NULL;
    pComponentPrivate = (G722DEC_COMPONENT_PRIVATE*)
        ((LCML_DSP_INTERFACE*)args[6])->pComponentPrivate;
    G722DEC_DPRINT("Component private handle: pComponentPrivate = %p\n",pComponentPrivate);
    G722DEC_DPRINT (":: Entering the LCML_Callback() : event = %d\n",event);

#ifdef G722DEC_DEBUG    
    printEmmEvent (event);    
#endif  
    
    if(event == EMMCodecBufferProcessed){
        G722DEC_DPRINT(":: --------- EMMCodecBufferProcessed Here\n");
        if( args[0] == (void *)EMMCodecInputBuffer) {
            G722DEC_DPRINT (" :: Inside the LCML_Callback EMMCodecInputBuffer\n");
            G722DEC_DPRINT(":: Input: pBufferr = %p\n", pBuffer);

            eError = G722DEC_GetCorresponding_LCMLHeader(pComponentPrivate,pBuffer, OMX_DirInput, &pLcmlHdr);
            G722DEC_DPRINT(":: Output: pLcmlHeader = %p\n", pLcmlHdr);
            G722DEC_DPRINT(":: Output: pLcmlHdr->eDir = %d\n", pLcmlHdr->eDir);
            G722DEC_DPRINT(":: Output: *pLcmlHdr->eDir = %d\n", pLcmlHdr->eDir);
            G722DEC_DPRINT(":: Output: Filled Len = %ld\n", pLcmlHdr->pBufHdr->nFilledLen);
            if (eError != OMX_ErrorNone) {
                G722DEC_DPRINT(":: Error: Invalid Buffer Came ...\n");
                goto EXIT;
            }
            G722DEC_DPRINT(":: Input: pLcmlHeader = %p\n", pLcmlHdr);

            pComponentPrivate->lcml_nCntIpRes++;

            G722DEC_ClearPending(pComponentPrivate,pLcmlHdr->pBufHdr,OMX_DirInput,__LINE__);
            /* >>>>>>>>>>>>>>>>>>>>> */
            G722DEC_DPRINT(": Component Sending Empty Input buffer%p to App\n", pLcmlHdr->pBufHdr->pBuffer);
            pComponentPrivate->cbInfo.EmptyBufferDone (pComponentPrivate->pHandle,
                                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                                       pLcmlHdr->pBufHdr );
            pComponentPrivate->nEmptyBufferDoneCount++;
            pComponentPrivate->lcml_nIpBuf--;
            pComponentPrivate->app_nBuf++;
            /* <<<<<<<<<<<<<<<<<<<<< */            
        } else if (args[0] == (void *)EMMCodecOuputBuffer) {
            G722DEC_DPRINT (" :: Inside the LCML_Callback EMMCodecOuputBuffer\n");
            G722DEC_DPRINT(":: Output: pBuffer = %p\n", pBuffer);
            if (!G722DEC_IsValid(pComponentPrivate,pBuffer,OMX_DirOutput)) {
                /* If the buffer we get back from the DSP is not valid call FillBufferDone
                   on a valid buffer */
                pComponentPrivate->cbInfo.FillBufferDone (pComponentPrivate->pHandle,
                                                          pComponentPrivate->pHandle->pApplicationPrivate,
                                                          pComponentPrivate->pOutputBufferList->pBufHdr[pComponentPrivate->nInvalidFrameCount++]);
                pComponentPrivate->nOutStandingFillDones--;
                pComponentPrivate->numPendingBuffers--;
            }
            else {
                pComponentPrivate->nOutStandingFillDones++;
                eError = G722DEC_GetCorresponding_LCMLHeader(pComponentPrivate,pBuffer, OMX_DirOutput, &pLcmlHdr);
                if (eError != OMX_ErrorNone) {
                    G722DEC_DPRINT(":: Error: Invalid Buffer Came ...\n");
                    goto EXIT;
                }
                pLcmlHdr->pBufHdr->nFilledLen = (int)args[8];
                G722DEC_DPRINT(":: Output: pLcmlHeader = %p\n", pLcmlHdr);
                G722DEC_DPRINT(":: Output: pLcmlHdr->eDir = %d\n", pLcmlHdr->eDir);
                G722DEC_DPRINT(":: Output: Filled Len = %ld\n", pLcmlHdr->pBufHdr->nFilledLen);
                pComponentPrivate->lcml_nCntOpReceived++;
                G722DEC_ClearPending(pComponentPrivate,pLcmlHdr->pBufHdr,OMX_DirOutput,__LINE__);
                /* >>>>>>>>>>>>>>>>>>>>> */
                G722DEC_DPRINT ("Sending Output buffer to Applcation\n");
                if (pComponentPrivate->bIsEOFSent){
                    G722DEC_DPRINT ("Adding EOS flag to the output buffer\n");
                    pLcmlHdr->pBufHdr->nFlags |= OMX_BUFFERFLAG_EOS;
                    pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                           pComponentPrivate->pHandle->pApplicationPrivate,
                                                           OMX_EventBufferFlag,
                                                           pLcmlHdr->pBufHdr->nOutputPortIndex,
                                                           pLcmlHdr->pBufHdr->nFlags, NULL);
                    pComponentPrivate->bIsEOFSent = 0;
                }
                if (pComponentPrivate->pMarkData) {
                    G722DEC_DPRINT ("pComponentPrivate->pMarkData set\n");
                    pLcmlHdr->pBufHdr->pMarkData = pComponentPrivate->pMarkData;
                    pLcmlHdr->pBufHdr->hMarkTargetComponent = pComponentPrivate->hMarkTargetComponent;
                }
                pComponentPrivate->num_Reclaimed_Op_Buff++;
                G722DEC_DPRINT(": Component Sending Filled Output buffer%p to App\n", pLcmlHdr->pBufHdr);
                G722DEC_DPRINT("pLcmlHdr->pBufHdr = 0x%p\n", pLcmlHdr->pBufHdr);
                if (pComponentPrivate->curState != OMX_StatePause) {
                    /* Copying time stamp information to output buffer */ 
                    pLcmlHdr->pBufHdr->nTimeStamp = (OMX_TICKS)pComponentPrivate->arrTimestamp[pComponentPrivate->OpBufindex];
                    /* Copying nTickCount information to output buffer */ 
                    pLcmlHdr->pBufHdr->nTickCount = pComponentPrivate->arrTickCount[pComponentPrivate->OpBufindex];

                    pComponentPrivate->OpBufindex++;
                    pComponentPrivate->OpBufindex %= pComponentPrivate->pPortDef[OMX_DirInput]->nBufferCountActual; 

                    pComponentPrivate->cbInfo.FillBufferDone (pComponentPrivate->pHandle, /* pHandle, */
                                                              pComponentPrivate->pHandle->pApplicationPrivate, /* pHandle->pApplicationPrivate, */
                                                              pLcmlHdr->pBufHdr);
                    pComponentPrivate->nOutStandingFillDones--;
                    pComponentPrivate->lcml_nOpBuf--;
                    pComponentPrivate->app_nBuf++;
                    pComponentPrivate->nFillBufferDoneCount++;
                }
                /* <<<<<<<<<<<<<<<<<<<<< */                        
            }
        }
    } else if(event == EMMCodecProcessingStoped) {
        if (!pComponentPrivate->bNoIdleOnStop) {        
            pComponentPrivate->curState = OMX_StateIdle;

            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                   OMX_EventCmdComplete,
                                                   OMX_CommandStateSet,
                                                   pComponentPrivate->curState,
                                                   NULL);
            pComponentPrivate->bIdleCommandPending = OMX_FALSE;
        } else { 
            pComponentPrivate->bNoIdleOnStop= OMX_FALSE;
        }        
    } else if(event == EMMCodecAlgCtrlAck) {
        G722DEC_DPRINT ("GOT MESSAGE USN_DSPACK_ALGCTRL \n");
    } else if (event == EMMCodecDspError) {
        G722DEC_DPRINT(":: commandedState  = %p\n",args[0]);
        G722DEC_DPRINT(":: arg4 = %p\n",args[4]);
        G722DEC_DPRINT(":: arg5 = %p\n",args[5]);
        G722DEC_DPRINT(":: --------- EMMCodecDspError Here\n");
        if(((int)args[4] == USN_ERR_WARNING) && ((int)args[5] == IUALG_WARN_PLAYCOMPLETED)) {
            /* add callback to application to indicate SN/USN has completed playing of current set of date */
            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,                  
                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                   OMX_EventBufferFlag,
                                                   (OMX_U32)NULL,
                                                   OMX_BUFFERFLAG_EOS,
                                                   NULL);
        }
        if((int)args[5] == IUALG_WARN_CONCEALED) {
            G722DEC_DPRINT( "Algorithm issued a warning. But can continue" );
            G722DEC_DPRINT("%d :: arg5 = %p\n",__LINE__,args[5]);
        }

        if((int)args[5] == IUALG_ERR_GENERAL) {
            G722DEC_DPRINT( "Algorithm error. Cannot continue" );
            G722DEC_DPRINT("%d :: arg5 = %p\n",__LINE__,args[5]);
            G722DEC_DPRINT("%d :: LCML_Callback: IUALG_ERR_GENERAL\n",__LINE__);
            pHandle = pComponentPrivate->pHandle;
            pLcmlHandle = (LCML_DSP_INTERFACE *)pComponentPrivate->pLcmlHandle;
        }

        if( (int)args[5] == IUALG_ERR_DATA_CORRUPT ){
            char *pArgs = "damedesuStr";
            G722DEC_DPRINT("%d :: arg5 = %p\n",__LINE__,args[5]);
            G722DEC_DPRINT("%d :: LCML_Callback: IUALG_ERR_DATA_CORRUPT\n",__LINE__);
            pHandle = pComponentPrivate->pHandle;
            pLcmlHandle = (LCML_DSP_INTERFACE *)pComponentPrivate->pLcmlHandle;

            eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                       MMCodecControlStop,(void *)pArgs);
            if(eError != OMX_ErrorNone) {
                G722DEC_DPRINT("%d: Error Occurred in Codec Stop..\n",
                               __LINE__);
                goto EXIT;
            }
            G722DEC_DPRINT("%d :: G722DEC: Codec has been Stopped here\n",__LINE__);
            pComponentPrivate->curState = OMX_StateIdle;

            pComponentPrivate->cbInfo.EventHandler(
                                                   pHandle, pHandle->pApplicationPrivate,
                                                   OMX_EventCmdComplete, OMX_ErrorNone,0, NULL);

        }

        if( (int)args[5] == IUALG_WARN_OVERFLOW ){
            G722DEC_DPRINT( "Algorithm error. Overflow" );
        }
        if( (int)args[5] == IUALG_WARN_UNDERFLOW ){
            G722DEC_DPRINT( "Algorithm error. Underflow" );
        }
    } else if (event == EMMCodecStrmCtrlAck) {
        G722DEC_DPRINT(":: GOT MESSAGE USN_DSPACK_STRMCTRL ----\n");
    }

 EXIT:
    G722DEC_DPRINT (":: Exiting the LCML_Callback() \n");
    return eError;
}


/* ================================================================================= * */
/**
 * @fn G722DEC_GetCorresponding_LCMLHeader() function gets the corresponding LCML
 * header from the actual data buffer for required processing.
 *
 * @param *pBuffer This is the data buffer pointer.
 *
 * @param eDir   This is direction of buffer. Input/Output.
 *
 * @param *G722D_LCML_BUFHEADERTYPE  This is pointer to LCML Buffer Header.
 *
 * @pre          None
 *
 * @post         None
 *
 *  @return      OMX_ErrorNone = Successful Inirialization of the component\n
 *               OMX_ErrorHardware = Hardware error has occured.
 *
 *  @see         None
 */
/* ================================================================================ * */
OMX_ERRORTYPE G722DEC_GetCorresponding_LCMLHeader(G722DEC_COMPONENT_PRIVATE *pComponentPrivate,
                                                  OMX_U8 *pBuffer,
                                                  OMX_DIRTYPE eDir,
                                                  G722D_LCML_BUFHEADERTYPE **ppLcmlHdr)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    G722D_LCML_BUFHEADERTYPE *pLcmlBufHeader = NULL;
    int nIpBuf=0, nOpBuf=0, i=0;

    G722DEC_DPRINT (":: Entering the G722DEC_GetCorresponding_LCMLHeader()\n");
    G722DEC_DPRINT (":: eDir = %d\n",eDir);

    while (!pComponentPrivate->bInitParamsInitialized) {
#ifndef UNDER_CE
        sched_yield();
#else
        Sleep(0);
#endif
    }

    if(eDir == OMX_DirInput) {
        G722DEC_DPRINT (":: In GetCorresponding_LCMLHeader()\n");

        nIpBuf = pComponentPrivate->pInputBufferList->numBuffers;

        pLcmlBufHeader = pComponentPrivate->pLcmlBufHeader[G722D_INPUT_PORT];

        for(i=0; i<nIpBuf; i++) {
            G722DEC_DPRINT("pBuffer = %p\n",pBuffer);
            G722DEC_DPRINT("pLcmlBufHeader->pBufHdr->pBuffer = %p\n",pLcmlBufHeader->pBufHdr->pBuffer);
            if(pBuffer == pLcmlBufHeader->pBufHdr->pBuffer) {
                *ppLcmlHdr = pLcmlBufHeader;
                G722DEC_DPRINT("::Corresponding LCML Header Found\n");
                goto EXIT;
            }
            pLcmlBufHeader++;
        }
    } else if (eDir == OMX_DirOutput) {
        i = 0;
        nOpBuf = pComponentPrivate->pOutputBufferList->numBuffers;

        pLcmlBufHeader = pComponentPrivate->pLcmlBufHeader[G722D_OUTPUT_PORT];
        G722DEC_DPRINT (":: nOpBuf = %d\n",nOpBuf);

        for(i=0; i<nOpBuf; i++) {
            G722DEC_DPRINT("pBuffer = %p\n",pBuffer);
            G722DEC_DPRINT("pLcmlBufHeader->pBufHdr->pBuffer = %p\n",pLcmlBufHeader->pBufHdr->pBuffer);
            if(pBuffer == pLcmlBufHeader->pBufHdr->pBuffer) {
                *ppLcmlHdr = pLcmlBufHeader;
                G722DEC_DPRINT("::Corresponding LCML Header Found\n");
                goto EXIT;
            }
            pLcmlBufHeader++;
        }
    } else {
        G722DEC_DPRINT(":: Invalid Buffer Type :: exiting...\n");
    }

 EXIT:
    G722DEC_DPRINT (":: Exiting the GetCorresponding_LCMLHeader() \n");
    return eError;
}

/* ================================================================================= * */
/**
 * @fn G722DEC_GetLCMLHandle() function gets the LCML handle and interacts with LCML
 * by using this LCML Handle.
 *
 * @param *pBufHeader This is the buffer header that needs to be processed.
 *
 * @param *pComponentPrivate  This is component's private date structure.
 *
 * @pre          None
 *
 * @post         None
 *
 *  @return      OMX_HANDLETYPE = Successful loading of LCML library.
 *               OMX_ErrorHardware = Hardware error has occured.
 *
 *  @see         None
 */
/* ================================================================================ * */
#ifndef UNDER_CE
OMX_HANDLETYPE G722DEC_GetLCMLHandle(G722DEC_COMPONENT_PRIVATE *pComponentPrivate)
{
    /* This must be taken care by WinCE */
    OMX_HANDLETYPE pHandle = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    void *handle = NULL;
    OMX_ERRORTYPE (*fpGetHandle)(OMX_HANDLETYPE);
    char *error = NULL;

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
        G722DEC_DPRINT("eError != OMX_ErrorNone...\n");
        pHandle = NULL;
        goto EXIT;
    }

    ((LCML_DSP_INTERFACE*)pHandle)->pComponentPrivate = pComponentPrivate;
 EXIT:
    return pHandle;
}
#else
/* WINDOWS Explicit dll load procedure */
OMX_HANDLETYPE G722DEC_GetLCMLHandle(G722DEC_COMPONENT_PRIVATE *pComponentPrivate)
{
    typedef OMX_ERRORTYPE (*LPFNDLLFUNC1)(OMX_HANDLETYPE);
    OMX_HANDLETYPE pHandle = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    HINSTANCE hDLL;               /* Handle to DLL */
    LPFNDLLFUNC1 fpGetHandle1;

    hDLL = LoadLibraryEx(TEXT("OAF_BML.dll"), NULL,0);
    if (hDLL == NULL)
    {
        /*fputs(dlerror(), stderr); */
        G722DEC_DPRINT("BML Load Failed!!!\n");
        return pHandle;
    }

    fpGetHandle1 = (LPFNDLLFUNC1)GetProcAddress(hDLL,TEXT("GetHandle"));
    if (!fpGetHandle1)
    {
        /* handle the error */
        FreeLibrary(hDLL);
        return pHandle;
    }

    /* call the function */
    eError = fpGetHandle1(&pHandle);
    if(eError != OMX_ErrorNone) {
        eError = OMX_ErrorUndefined;
        G722DEC_DPRINT("eError != OMX_ErrorNone...\n");
        pHandle = NULL;
        return pHandle;
    }
    ((LCML_DSP_INTERFACE*)pHandle)->pComponentPrivate = pComponentPrivate;
    return pHandle;
}
#endif

void G722DEC_CleanupInitParams(OMX_HANDLETYPE pComponent)
{
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    G722DEC_COMPONENT_PRIVATE *pComponentPrivate = (G722DEC_COMPONENT_PRIVATE *)
        pHandle->pComponentPrivate;
    G722D_LCML_BUFHEADERTYPE *pTemp_lcml = NULL;
    OMX_U32 nIpBuf = pComponentPrivate->nRuntimeInputBuffers;
    OMX_U32 nOpBuf = pComponentPrivate->nRuntimeOutputBuffers;
    OMX_U32 i=0;
    OMX_U8 *ptemp = NULL;

    G722DEC_DPRINT (":: G722DEC_CleanupInitParams()\n");
    G722DEC_MEMPRINT(":: Freeing:  pComponentPrivate->strmAttr = %p\n", pComponentPrivate->strmAttr);

    G722D_OMX_FREE(pComponentPrivate->strmAttr);
    /*pComponentPrivate->strmAttr = NULL;*/

    /*if (pComponentPrivate->dasfmode == 1) {*/
    G722DEC_MEMPRINT(":: Freeing: pComponentPrivate->pParams = %p\n",pComponentPrivate->pParams);
    ptemp = (OMX_U8*)pComponentPrivate->pParams;
    if(ptemp != NULL){
        ptemp -= 128;
    }
    pComponentPrivate->pParams = (G722D_USN_AudioCodecParams *)ptemp;
    G722D_OMX_FREE(pComponentPrivate->pParams);
    /*}*/

    pTemp_lcml = pComponentPrivate->pLcmlBufHeader[G722D_INPUT_PORT];
    for(i=0; i<nIpBuf; i++) {
        G722DEC_MEMPRINT(":: Freeing: pTemp_lcml->pIpParam = %p\n",pTemp_lcml->pIpParam);
        G722D_OMX_FREE(pTemp_lcml->pIpParam);
        pTemp_lcml++;
    }

    G722DEC_MEMPRINT(":: Freeing pComponentPrivate->pLcmlBufHeader[G722D_INPUT_PORT] = %p\n",
                     pComponentPrivate->pLcmlBufHeader[G722D_INPUT_PORT]);
    G722D_OMX_FREE(pComponentPrivate->pLcmlBufHeader[G722D_INPUT_PORT]);

    
    pTemp_lcml = pComponentPrivate->pLcmlBufHeader[G722D_OUTPUT_PORT];
    for(i=0; i<nOpBuf; i++) {
        G722DEC_MEMPRINT(":: Freeing: pTemp_lcml->pIpParam = %p\n",pTemp_lcml->pOpParam);
        G722D_OMX_FREE(pTemp_lcml->pOpParam);
        pTemp_lcml++;
    }

    G722DEC_MEMPRINT(":: Freeing: pComponentPrivate->pLcmlBufHeader[G722D_OUTPUT_PORT] = %p\n",
                     pComponentPrivate->pLcmlBufHeader[G722D_OUTPUT_PORT]);
    G722D_OMX_FREE(pComponentPrivate->pLcmlBufHeader[G722D_OUTPUT_PORT]);
    G722DEC_DPRINT ("Exiting Successfully G722DEC_CleanupInitParams()\n");
}
/* ========================================================================== */
/**
 * @G722DEC_SetPending() This function marks the buffer as pending when it is sent
 * to DSP/
 *
 * @param pComponentPrivate This is component's private date area.
 *
 * @param pBufHdr This is poiter to OMX Buffer header whose buffer is sent to DSP
 *
 * @param eDir This is direction of buffer i.e. input or output.
 *
 * @pre None
 *
 * @post None
 *
 * @return none
 */
/* ========================================================================== */
void G722DEC_SetPending(G722DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir, OMX_U32 lineNumber)
{
    OMX_U16 i = 0;

    G722DEC_DPRINT("Called G722DEC_SetPending\n");
    G722DEC_DPRINT("eDir = %d\n",eDir);

    if (eDir == OMX_DirInput) {
        for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pInputBufferList->pBufHdr[i]) {
                pComponentPrivate->pInputBufferList->bBufferPending[i] = 1;
                G722DEC_DPRINT("*******************INPUT BUFFER %d IS PENDING Line %ld******************************\n",i,lineNumber);
            }
        }
    }
    else {
        for (i=0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pOutputBufferList->pBufHdr[i]) {
                pComponentPrivate->pOutputBufferList->bBufferPending[i] = 1;
                G722DEC_DPRINT("*******************OUTPUT BUFFER %d IS PENDING Line %ld******************************\n",i,lineNumber);
            }
        }
    }
}

/* ========================================================================== */
/**
 * @G722DEC_ClearPending() This function clears the buffer status from pending
 * when it is received back from DSP.
 *
 * @param pComponentPrivate This is component's private date area.
 *
 * @param pBufHdr This is poiter to OMX Buffer header that is received from
 * DSP/LCML.
 *
 * @param eDir This is direction of buffer i.e. input or output.
 *
 * @pre None
 *
 * @post None
 *
 * @return none
 */
/* ========================================================================== */

void G722DEC_ClearPending(G722DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir, OMX_U32 lineNumber)
{
    OMX_U16 i = 0;

    if (eDir == OMX_DirInput) {
        for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pInputBufferList->pBufHdr[i]) {
                pComponentPrivate->pInputBufferList->bBufferPending[i] = 0;
                G722DEC_DPRINT("*******************INPUT BUFFER %d IS RECLAIMED Line %ld******************************\n",i,lineNumber);
            }
        }
    }
    else {
        for (i=0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pOutputBufferList->pBufHdr[i]) {
                pComponentPrivate->pOutputBufferList->bBufferPending[i] = 0;
                G722DEC_DPRINT("*******************OUTPUT BUFFER %d IS RECLAIMED Line %ld******************************\n",i,lineNumber);
            }
        }
    }
}

/* ========================================================================== */
/**
 * @G722DEC_IsPending() This function checks whether or not a buffer is pending.
 *
 * @param pComponentPrivate This is component's private date area.
 *
 * @param pBufHdr This is poiter to OMX Buffer header of interest.
 *
 * @param eDir This is direction of buffer i.e. input or output.
 *
 * @pre None
 *
 * @post None
 *
 * @return none
 */
/* ========================================================================== */

OMX_U32 G722DEC_IsPending(G722DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir)
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


/* ========================================================================== */
/**
 * @G722DEC_IsValid() This function identifies whether or not buffer recieved from
 * LCML is valid. It searches in the list of input/output buffers to do this.
 *
 * @param pComponentPrivate This is component's private date area.
 *
 * @param pBufHdr This is poiter to OMX Buffer header of interest.
 *
 * @param eDir This is direction of buffer i.e. input or output.
 *
 * @pre None
 *
 * @post None
 *
 * @return status of the buffer.
 */
/* ========================================================================== */

OMX_U32 G722DEC_IsValid(G722DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U8 *pBuffer, OMX_DIRTYPE eDir)
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
 * @G722DECFill_LCMLInitParamsEx() This function initializes the init parameter of
 * the LCML structure when a port is enabled and component is in idle state.
 *
 * @param pComponent This is component handle.
 *
 * @pre None
 *
 * @post None
 *
 * @return appropriate OMX Error.
 */
/* ========================================================================== */

OMX_ERRORTYPE G722DECFill_LCMLInitParamsEx(OMX_HANDLETYPE pComponent)

{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nIpBuf = 0,nIpBufSize = 0,nOpBuf = 0,nOpBufSize = 0;
    OMX_U32 i = 0;
    OMX_BUFFERHEADERTYPE *pTemp = NULL;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    G722DEC_COMPONENT_PRIVATE *pComponentPrivate =
        (G722DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    G722D_LCML_BUFHEADERTYPE *pTemp_lcml = NULL;
    OMX_U32 size_lcml = 0;
    OMX_U8 *ptr = NULL;


    G722DEC_DPRINT(":: Entered Fill_LCMLInitParams");


    nIpBuf = pComponentPrivate->pInputBufferList->numBuffers;
    nOpBuf = pComponentPrivate->pOutputBufferList->numBuffers;
    nIpBufSize = pComponentPrivate->pPortDef[G722D_INPUT_PORT]->nBufferSize;
    nOpBufSize = pComponentPrivate->pPortDef[G722D_OUTPUT_PORT]->nBufferSize;

    G722DEC_BUFPRINT("Input Buffer Count = %ld\n",nIpBuf);
    G722DEC_BUFPRINT("Input Buffer Size = %ld\n",nIpBufSize);
    G722DEC_BUFPRINT("Output Buffer Count = %ld\n",nOpBuf);
    G722DEC_BUFPRINT("Output Buffer Size = %ld\n",nOpBufSize);

    G722DEC_DPRINT(":: bufAlloced = %d\n",pComponentPrivate->bufAlloced);
    size_lcml = nIpBuf * sizeof(G722D_LCML_BUFHEADERTYPE);

    G722D_OMX_MALLOC_SIZE(ptr,size_lcml,OMX_U8);
    pTemp_lcml = (G722D_LCML_BUFHEADERTYPE *)ptr;

    pComponentPrivate->pLcmlBufHeader[G722D_INPUT_PORT] = pTemp_lcml;

    for (i=0; i<nIpBuf; i++) {
        if(pComponentPrivate->bufAlloced == 0) {
            G722D_OMX_MALLOC(pTemp, OMX_BUFFERHEADERTYPE);
        } else {
            G722DEC_DPRINT(":: IpBufferHeader %p is already there\n",
                           pComponentPrivate->pInputBufferList->pBufHdr[i]);
            pTemp = pComponentPrivate->pInputBufferList->pBufHdr[i];
        }

        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);

        pTemp->nAllocLen = nIpBufSize;
        pTemp->nFilledLen = nIpBufSize;
        pTemp->nVersion.s.nVersionMajor = G722DEC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = G722DEC_MINOR_VER;

        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = DONT_CARE;

        if (pComponentPrivate->bufAlloced == 0) {
            G722D_OMX_MALLOC_SIZE(pTemp->pBuffer,(nIpBufSize+256),OMX_U8);
            pTemp->pBuffer = pTemp->pBuffer + 128;
        } else {
            G722DEC_DPRINT(":: IpBuffer %p is already there\n",pTemp->pBuffer);
        }

        if (pTemp->pBuffer == NULL) {
            G722DEC_DPRINT(":: Malloc Failed...\n");
            goto EXIT;
        }

        pTemp_lcml->pBufHdr = pTemp;
        pTemp_lcml->eDir = OMX_DirInput;
        pTemp_lcml->pOtherParams[i] = NULL;

        G722D_OMX_MALLOC(pTemp_lcml->pIpParam, G722DEC_ParamStruct);
        pTemp_lcml->pIpParam->usLastFrame = 0;

        pTemp->nFlags = NORMAL_BUFFER;
        ((G722DEC_COMPONENT_PRIVATE *) pTemp->pPlatformPrivate)->pHandle = pHandle;

        G722DEC_DPRINT("::Comp: InBuffHeader[%ld] = %p\n", i, pTemp);
        G722DEC_DPRINT("::Comp:  >>>> InputBuffHeader[%ld]->pBuffer = %p\n", i, pTemp->pBuffer);
        G722DEC_DPRINT("::Comp: Ip : pTemp_lcml[%ld] = %p\n", i, pTemp_lcml);

        pTemp_lcml++;
    }

    size_lcml = nOpBuf * sizeof(G722D_LCML_BUFHEADERTYPE);
    
    G722D_OMX_MALLOC_SIZE(pTemp_lcml,size_lcml,G722D_LCML_BUFHEADERTYPE);
    pComponentPrivate->pLcmlBufHeader[G722D_OUTPUT_PORT] = pTemp_lcml;

    for (i=0; i<nOpBuf; i++) {
        if(pComponentPrivate->bufAlloced == 0) {
            G722D_OMX_MALLOC(pTemp, OMX_BUFFERHEADERTYPE);
        } else {
            G722DEC_DPRINT(":: OpBufferHeader %p is already there\n",
                           pComponentPrivate->pOutputBufferList->pBufHdr[i]);
            pTemp = pComponentPrivate->pOutputBufferList->pBufHdr[i];
        }

        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);

        pTemp->nAllocLen = nOpBufSize;
        pTemp->nFilledLen = nOpBufSize;
        pTemp->nVersion.s.nVersionMajor = G722DEC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = G722DEC_MINOR_VER;

        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = 0;
        pTemp->nTimeStamp = 0;

        if (pComponentPrivate->bufAlloced == 0) {
            G722D_OMX_MALLOC_SIZE(pTemp->pBuffer,(nOpBufSize+256),OMX_U8);
            
            pTemp->pBuffer += 128;
            G722DEC_DPRINT("%d:: OpBuffer %p is already there\n",__LINE__,pTemp->pBuffer);
        } else {
            G722DEC_DPRINT(":: OpBuffer %p is already there\n",pTemp->pBuffer);
        }

        pTemp_lcml->pBufHdr = pTemp;
        pTemp_lcml->eDir = OMX_DirOutput;
        pTemp_lcml->pOtherParams[i] = NULL;

        pTemp->nFlags = NORMAL_BUFFER;
        ((G722DEC_COMPONENT_PRIVATE *)pTemp->pPlatformPrivate)->pHandle = pHandle;
        G722DEC_DPRINT("::Comp:  >>>>>>>>>>>>> OutBuffHeader[%ld] = %p\n", i, pTemp);
        G722DEC_DPRINT("::Comp:  >>>> OutBuffHeader[%ld]->pBuffer = %p\n", i, pTemp->pBuffer);
        G722DEC_DPRINT("::Comp: Op : pTemp_lcml[%ld] = %p\n", i, pTemp_lcml);
        pTemp_lcml++;
    }
    pComponentPrivate->bPortDefsAllocated = 1;

    G722DEC_DPRINT(":: Exiting Fill_LCMLInitParams");

    pComponentPrivate->bInitParamsInitialized = 1;

 EXIT:
    return eError;
}
/**
 *
 */
void printEmmEvent (TUsnCodecEvent event)
{
    switch(event) {

    case EMMCodecDspError:
        G722DEC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecDspError >>>>>>>>>>\n");
        break;

    case EMMCodecInternalError:
        G722DEC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecInternalError >>>>>>>>>> \n");
        break;

    case EMMCodecInitError:
        G722DEC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecInitError>>>>>>>>>> \n");
        break;

    case EMMCodecDspMessageRecieved:
        G722DEC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecDspMessageRecieved>>>>>>>>>> \n");
        break;

    case EMMCodecBufferProcessed:
        G722DEC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecBufferProcessed>>>>>>>>>> \n");
        break;

    case EMMCodecProcessingStarted:
        G722DEC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecProcessingStarted>>>>>>>>>> \n");
        break;

    case EMMCodecProcessingPaused:
        G722DEC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecProcessingPaused>>>>>>>>>> \n");
        break;

    case EMMCodecProcessingStoped:
        G722DEC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecProcessingStoped>>>>>>>>>> \n");
        break;

    case EMMCodecProcessingEof:
        G722DEC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecProcessingEof>>>>>>>>>> \n");
        break;

    case EMMCodecBufferNotProcessed:
        G722DEC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecBufferNotProcessed>>>>>>>>>> \n");
        break;

    case EMMCodecAlgCtrlAck:
        G722DEC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecAlgCtrlAck>>>>>>>>>> \n");
        break;

    case EMMCodecStrmCtrlAck:
        G722DEC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecStrmCtrlAck>>>>>>>>>> \n");
        break;
    }
    return;
}
