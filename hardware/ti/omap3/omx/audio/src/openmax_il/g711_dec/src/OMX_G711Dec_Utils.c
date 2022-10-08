
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
 * @file OMX_G711Dec_Utils.c
 *
 * This file implements OMX Component for G711 decoder that
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
#include <unistd.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/wait.h>
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
#include "OMX_G711Dec_Utils.h"
#include "g711decsocket_ti.h"
#include "decode_common_ti.h"
#include "usn.h"
#ifdef RESOURCE_MANAGER_ENABLED
#include <ResourceManagerProxyAPI.h>
#endif

/* ======================================================================= */
/**
 * @def    DASF    Defines the value for identify DASF ON
 */
/* ======================================================================= */
#define DASF 1

#ifdef DASF 
int iAudioFormat = 1;
int iSamplingRate = 8000;
#endif

#ifdef UNDER_CE
#define HASHINGENABLE 1
void sleep(DWORD Duration)
{
    Sleep(Duration);
}
#endif

#ifdef G711DEC_MEMDEBUG
#define newmalloc(x) mymalloc(__LINE__,__FILE__,x)
#define newfree(z) myfree(z,__LINE__,__FILE__)
#else
#define newmalloc(x) malloc(x)
#define newfree(z) free(z)
#endif

/* ========================================================================== */
/**
 * @G711DECFill_LCMLInitParams () This function is used by the component thread to
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
static G711DEC_COMPONENT_PRIVATE *pComponentPrivate_CC = NULL;

OMX_ERRORTYPE G711DECFill_LCMLInitParams(OMX_HANDLETYPE pComponent,
                                         LCML_DSP *plcml_Init, OMX_U16 arr[])
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nIpBuf = 0,nIpBufSize = 0,nOpBuf = 0,nOpBufSize = 0;
    OMX_U16 i = 0;
    OMX_BUFFERHEADERTYPE *pTemp = NULL;
    OMX_U16 size_lcml = 0;
    LCML_STRMATTR *strmAttr = NULL;

    LCML_DSP_INTERFACE *pHandle = (LCML_DSP_INTERFACE *)pComponent;
    G711DEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    LCML_G711DEC_BUFHEADERTYPE *pTemp_lcml = NULL;

    G711DEC_DPRINT("%d :: G711DECFill_LCMLInitParams\n ",__LINE__);
    G711DEC_DPRINT("%d :: pHandle = %p\n",__LINE__,pHandle);
    G711DEC_DPRINT("%d :: pHandle->pComponentPrivate = %p\n",__LINE__,pHandle->pComponentPrivate);
        
    pComponentPrivate = pHandle->pComponentPrivate;

    nIpBuf = (OMX_U16)pComponentPrivate->pInputBufferList->numBuffers;
    nIpBufSize = pComponentPrivate->pPortDef[G711DEC_INPUT_PORT]->nBufferSize;
    pComponentPrivate->nRuntimeInputBuffers = nIpBuf;
    nOpBuf = pComponentPrivate->pOutputBufferList->numBuffers;
    nOpBufSize = pComponentPrivate->pPortDef[G711DEC_OUTPUT_PORT]->nBufferSize;

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

    plcml_Init->NodeInfo.AllUUIDs[0].uuid = &G711DECSOCKET_TI_UUID;
    strcpy ((char*)plcml_Init->NodeInfo.AllUUIDs[0].DllName,G711DEC_DLL_NAME);
    plcml_Init->NodeInfo.AllUUIDs[0].eDllType = DLL_NODEOBJECT;

    plcml_Init->NodeInfo.AllUUIDs[1].uuid = &G711DECSOCKET_TI_UUID;
    strcpy ((char*)plcml_Init->NodeInfo.AllUUIDs[1].DllName,G711DEC_DLL_NAME);
    plcml_Init->NodeInfo.AllUUIDs[1].eDllType = DLL_DEPENDENT;

    plcml_Init->NodeInfo.AllUUIDs[2].uuid = &USN_TI_UUID;
    strcpy ((char*)plcml_Init->NodeInfo.AllUUIDs[2].DllName,G711DEC_USN_DLL_NAME);
    plcml_Init->NodeInfo.AllUUIDs[2].eDllType = DLL_DEPENDENT;


    if(pComponentPrivate->dasfmode == 1) {
        G711DEC_DPRINT("pComponentPrivate->dasfmode = %d\n",pComponentPrivate->dasfmode);
        G711D_OMX_MALLOC(strmAttr, LCML_STRMATTR);
        pComponentPrivate->strmAttr = strmAttr;

        strmAttr->uSegid = 0;
        strmAttr->uAlignment = 0;
        strmAttr->uTimeout = G711D_TIMEOUT;
        strmAttr->uBufsize = nOpBufSize;
        strmAttr->uNumBufs = NUM_G711DEC_OUTPUT_BUFFERS_DASF;
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
    plcml_Init->SegID = OMX_G711DEC_DEFAULT_SEGMENT;
    plcml_Init->Timeout = OMX_G711DEC_SN_TIMEOUT;
    plcml_Init->Alignment = 0;
    plcml_Init->Priority = OMX_G711DEC_SN_PRIORITY;
    plcml_Init->ProfileID = -1;

    arr[0] = STREAM_COUNT;
    arr[1] = G711DEC_INPUT_PORT;
    arr[2] = G711DEC_DMM;
    
    if (pComponentPrivate->pInputBufferList->numBuffers) {
        arr[3] = (OMX_U16) (pComponentPrivate->pInputBufferList->numBuffers );
    }
    else {
        arr[3] = 1;
    }

    arr[4] = G711DEC_OUTPUT_PORT;

    if(pComponentPrivate->dasfmode == 1) {
        G711DEC_DPRINT("Setting up create phase params for DASF mode\n");
        arr[5] = G711DEC_OUTSTRM;
        arr[6] = NUM_G711DEC_OUTPUT_BUFFERS_DASF;
    }
    else {

        G711DEC_DPRINT("Setting up create phase params for FILE mode\n");
        
        arr[5] = G711DEC_DMM;
        
        if (pComponentPrivate->pOutputBufferList->numBuffers) {
            arr[6] = (OMX_U16) pComponentPrivate->pOutputBufferList->numBuffers;
        }
        else {
            arr[6] = 1;
        }
    }

    /* set companding mode (A-Law or Mu-Law) */
    arr[7] = (OMX_U16)pComponentPrivate->g711Params[G711DEC_INPUT_PORT]->ePCMMode;
    arr[8] = (OMX_S16)pComponentPrivate->ftype;
    arr[9] = pComponentPrivate->nmulevel;
    arr[10] = pComponentPrivate->noiselp;
    arr[11] = pComponentPrivate->dbmnoise;
    arr[12] = pComponentPrivate->packetlostc;
    arr[13] = END_OF_CR_PHASE_ARGS;

    plcml_Init->pCrPhArgs = arr;

    G711DEC_DPRINT("%d :: Comp: OMX_G711DecUtils.c\n",__LINE__);
    
    size_lcml = (OMX_U16) (nIpBuf * sizeof(LCML_G711DEC_BUFHEADERTYPE));
    G711D_OMX_MALLOC_SIZE(pTemp_lcml, size_lcml, LCML_G711DEC_BUFHEADERTYPE);
    
    pComponentPrivate->pLcmlBufHeader[G711DEC_INPUT_PORT] = pTemp_lcml;
    
    for (i=0; i<nIpBuf; i++) {
        pTemp = pComponentPrivate->pInputBufferList->pBufHdr[i];
        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pTemp->nAllocLen = nIpBufSize;
        pTemp->nFilledLen = nIpBufSize;
        pTemp->nVersion.s.nVersionMajor = G711DEC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = G711DEC_MINOR_VER;
        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = NOT_USED;
        pTemp_lcml->buffer = pTemp;
        pTemp_lcml->eDir = OMX_DirInput;

        G711D_OMX_MALLOC(pTemp_lcml->pIpParam, G711DEC_UAlgInBufParamStruct);
        
        pTemp_lcml->pIpParam->usFrameLost = 0;
        pTemp_lcml->pIpParam->usEndOfFile = 0;

        G711D_OMX_MALLOC(pTemp_lcml->pBufferParam,G711DEC_ParamStruct);
        G711D_OMX_MALLOC(pTemp_lcml->pDmmBuf,DMM_BUFFER_OBJ); 

        /* This means, it is not a last buffer. This flag is to be modified by
         * the application to indicate the last buffer */
        pTemp->nFlags = NORMAL_BUFFER;

        pTemp_lcml++;
    }

    /* Allocate memory for all output buffer headers..
     * This memory pointer will be sent to LCML */
    size_lcml = (OMX_U16) nOpBuf * sizeof(LCML_G711DEC_BUFHEADERTYPE);
    G711D_OMX_MALLOC_SIZE(pTemp_lcml, size_lcml, LCML_G711DEC_BUFHEADERTYPE);
    
    pComponentPrivate->pLcmlBufHeader[G711DEC_OUTPUT_PORT] = pTemp_lcml;

    for (i=0; i<nOpBuf; i++) {
        pTemp = pComponentPrivate->pOutputBufferList->pBufHdr[i];
        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pTemp->nAllocLen = nOpBufSize;
        pTemp->nFilledLen = nOpBufSize;
        pTemp->nVersion.s.nVersionMajor = G711DEC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = G711DEC_MINOR_VER;
        pComponentPrivate->nVersion = pTemp->nVersion.nVersion;
        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = NOT_USED;
        /* This means, it is not a last buffer. This flag is to be modified by
         * the application to indicate the last buffer */
                    
        pTemp_lcml->buffer = pTemp;
        pTemp_lcml->eDir = OMX_DirOutput;
        
        G711DEC_DPRINT("%d:::pTemp_lcml = %p\n",__LINE__,pTemp_lcml);
        G711DEC_DPRINT("%d:::pTemp_lcml->buffer = %p\n",__LINE__,pTemp_lcml->buffer);

        pTemp->nFlags = NORMAL_BUFFER;

        pTemp_lcml++;
    }
    
    pComponentPrivate->bPortDefsAllocated = 1;
    G711DEC_DPRINT("%d :: Exiting G711DECFill_LCMLInitParams",__LINE__);

    pComponentPrivate->bInitParamsInitialized = 1;
    
 EXIT:

    if(eError == OMX_ErrorInsufficientResources)
    {
        OMX_G711DECMEMFREE_STRUCT(pComponentPrivate->strmAttr);
        OMX_G711DECMEMFREE_STRUCT(pComponentPrivate->pLcmlBufHeader[G711DEC_INPUT_PORT]);
        OMX_G711DECMEMFREE_STRUCT(pComponentPrivate->pLcmlBufHeader[G711DEC_OUTPUT_PORT]);
	if (pTemp_lcml != NULL) {
	    OMX_G711DECMEMFREE_STRUCT(pTemp_lcml->pIpParam);
	}
    }
        
    return eError;
}


/* ========================================================================== */
/**
 * @G711DEC_StartComponentThread() This function is called by the component to create
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

OMX_ERRORTYPE G711DEC_StartComponentThread(OMX_HANDLETYPE pComponent)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    
    G711DEC_COMPONENT_PRIVATE *pComponentPrivate =
        (G711DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
                        
#ifdef UNDER_CE
    pthread_attr_t attr;
    memset(&attr, 0, sizeof(attr));
    attr.__inheritsched = PTHREAD_EXPLICIT_SCHED;
    attr.__schedparam.__sched_priority = OMX_AUDIO_DECODER_THREAD_PRIORITY;
#endif

    G711DEC_DPRINT ("%d :: Inside  G711DEC_StartComponentThread\n", __LINE__);

    /* Initialize all the variables*/
    pComponentPrivate->bIsStopping = 0;
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
                             ComponentThread, pComponentPrivate);
#else
    eError = pthread_create (&(pComponentPrivate->ComponentThread), NULL,
                             ComponentThread, pComponentPrivate);
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
 * @G711Dec_FreeCompResources() This function is called by the component during
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

OMX_ERRORTYPE G711DEC_FreeCompResources(OMX_HANDLETYPE pComponent)
{
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    
    G711DEC_COMPONENT_PRIVATE *pComponentPrivate = 
        (G711DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_ERRORTYPE err = OMX_ErrorNone;
    OMX_U32 nIpBuf = 0;
    OMX_U32 nOpBuf = 0;

    G711DEC_DPRINT ("%d :: G711DEC_FreeCompResources\n", __LINE__);

    if (pComponentPrivate->bPortDefsAllocated) {
        nIpBuf = pComponentPrivate->pPortDef[G711DEC_INPUT_PORT]->nBufferCountActual;
        nOpBuf = pComponentPrivate->pPortDef[G711DEC_OUTPUT_PORT]->nBufferCountActual;
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

    OMX_G711DECMEMFREE_STRUCT(pComponentPrivate->pHoldBuffer);

    if (pComponentPrivate->bPortDefsAllocated) {
        G711DEC_DPRINT("%d:::[G711DEC_FreeCompResources] \n", __LINE__);
        OMX_G711DECMEMFREE_STRUCT(pComponentPrivate->pPortDef[G711DEC_INPUT_PORT]);
        OMX_G711DECMEMFREE_STRUCT(pComponentPrivate->pPortDef[G711DEC_OUTPUT_PORT]);
        OMX_G711DECMEMFREE_STRUCT(pComponentPrivate->g711Params[G711DEC_INPUT_PORT]);
        OMX_G711DECMEMFREE_STRUCT(pComponentPrivate->g711Params[G711DEC_OUTPUT_PORT]);
        OMX_G711DECMEMFREE_STRUCT(pComponentPrivate->pCompPort[G711DEC_INPUT_PORT]->pPortFormat);
        OMX_G711DECMEMFREE_STRUCT(pComponentPrivate->pCompPort[G711DEC_OUTPUT_PORT]->pPortFormat);
        OMX_G711DECMEMFREE_STRUCT(pComponentPrivate->pCompPort[G711DEC_INPUT_PORT] );
        OMX_G711DECMEMFREE_STRUCT(pComponentPrivate->pCompPort[G711DEC_OUTPUT_PORT] );
        OMX_G711DECMEMFREE_STRUCT(pComponentPrivate->pInputBufferList);
        OMX_G711DECMEMFREE_STRUCT(pComponentPrivate->pOutputBufferList);
        OMX_G711DECMEMFREE_STRUCT(pComponentPrivate->pParams);
    }
    
    pComponentPrivate->bPortDefsAllocated = 0;

#ifndef UNDER_CE
    G711DEC_DPRINT("\n\n FreeCompResources: Destroying mutexes.\n\n");
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
/*==========================================================================================================*/
/** Cleanup init params()                                                                            */                                                           
/*===========================================================================================================*/
OMX_ERRORTYPE G711DEC_CleanupInitParams(OMX_HANDLETYPE pComponent)
{
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    G711DEC_COMPONENT_PRIVATE *pComponentPrivate = (G711DEC_COMPONENT_PRIVATE *)
        pHandle->pComponentPrivate;

    LCML_G711DEC_BUFHEADERTYPE *pTemp_lcml = NULL;
    OMX_U8 *pBufParmsTemp = NULL;

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nIpBuf = 0;
    OMX_U16 i=0;

    G711DEC_DPRINT ("%d :: G711DEC_CleanupInitParams()\n", __LINE__);
    
    nIpBuf = pComponentPrivate->nRuntimeInputBuffers;
    OMX_G711DECMEMFREE_STRUCT(pComponentPrivate->strmAttr);

    pTemp_lcml = pComponentPrivate->pLcmlBufHeader[G711DEC_INPUT_PORT];
    
    for(i=0; i<nIpBuf; i++) {
        OMX_G711DECMEMFREE_STRUCT(pTemp_lcml->pIpParam);
        OMX_G711DECMEMFREE_STRUCT(pTemp_lcml->pBufferParam);
        OMX_G711DECMEMFREE_STRUCT(pTemp_lcml->pDmmBuf);
        pBufParmsTemp = (OMX_U8*)pTemp_lcml->pFrameParam;
        pBufParmsTemp -= 128;
        OMX_G711DECMEMFREE_STRUCT(pBufParmsTemp);
        pTemp_lcml++;
    }

    OMX_G711DECMEMFREE_STRUCT(pComponentPrivate->pLcmlBufHeader[G711DEC_INPUT_PORT]);
    OMX_G711DECMEMFREE_STRUCT(pComponentPrivate->pLcmlBufHeader[G711DEC_OUTPUT_PORT]);

    return eError;
}

/* ========================================================================== */
/**
 * @G711DEC_StopComponentThread() This function is called by the component during
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

OMX_ERRORTYPE G711DEC_StopComponentThread(OMX_HANDLETYPE pComponent)
{
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    G711DEC_COMPONENT_PRIVATE *pComponentPrivate = (G711DEC_COMPONENT_PRIVATE *)
        pHandle->pComponentPrivate;

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_ERRORTYPE threadError = OMX_ErrorNone;
    OMX_S16 pthreadError = 0;

    /*Join the component thread */
    pComponentPrivate->bIsStopping = 1;
    pthreadError = pthread_join (pComponentPrivate->ComponentThread,
                                 (void*)&threadError);
                                 
    if (0 != pthreadError) {
        eError = OMX_ErrorHardware;
    }

    /*Check for the errors */
    if (OMX_ErrorNone != threadError && OMX_ErrorNone != eError) {
        eError = OMX_ErrorInsufficientResources;
        G711DEC_DPRINT ("%d :: Error while closing Component Thread\n",__LINE__);
    }
    
    return eError;
}


/* ========================================================================== */
/**
 * @G711DECHandleCommand() This function is called by the component when ever it
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

OMX_U32 G711DECHandleCommand (G711DEC_COMPONENT_PRIVATE *pComponentPrivate)
{

    OMX_COMPONENTTYPE *pHandle = NULL;
    OMX_COMMANDTYPE command;
    OMX_STATETYPE commandedState = OMX_StateInvalid;
    OMX_U32 commandData = 0;
    OMX_HANDLETYPE pLcmlHandle = pComponentPrivate->pLcmlHandle;
    
#ifdef RESOURCE_MANAGER_ENABLED
    OMX_ERRORTYPE rm_error = OMX_ErrorNone;
#endif

    OMX_U16 i = 0;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nBuf = 0;
    OMX_U16 arr[100] = {0};
    char *p = "hello";

    LCML_CALLBACKTYPE cb;
    LCML_DSP *pLcmlDsp = NULL;
    G711DEC_AudioCodecParams *pParams = NULL;
    OMX_S16 ret = 0;
    LCML_G711DEC_BUFHEADERTYPE *pLcmlHdr = NULL;
    int inputPortFlag=0,outputPortFlag=0;

    pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;

    G711DEC_DPRINT ("%d :: Entering G711DECHandleCommand Function - curState = %d\n",__LINE__,pComponentPrivate->curState);
    G711DEC_DPRINT ("%d :: pComponentPrivate = %p\n", __LINE__, pComponentPrivate);
    G711DEC_DPRINT ("%d :: pHandle = %p\n", __LINE__, pHandle);
    G711DEC_DPRINT ("%d :: Reading from the cmdPipe\n",__LINE__);
    
    ret = (OMX_U16) (read (pComponentPrivate->cmdPipe[0], &command, sizeof (command)));
    
    if (ret == -1) {
        G711DEC_DPRINT ("%d :: Error While reading from the Pipe\n",__LINE__);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }
    
    G711DEC_DPRINT ("%d :: Reading from the cmdDataPipe\n",__LINE__);
    
    ret = (OMX_U16) (read (pComponentPrivate->cmdDataPipe[0], &commandData, sizeof (commandData)));
    
    if (ret == -1) {
        G711DEC_DPRINT ("%d :: Error While reading from the Pipe\n",__LINE__);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }

    if (command == OMX_CommandStateSet) {
        commandedState = (OMX_STATETYPE)commandData;
        switch(commandedState) {
        case OMX_StateIdle:
                
            G711DEC_DPRINT("%d :: G711DEC_HandleCommand :: OMX_StateIdle \n",__LINE__);
            G711DEC_DPRINT("%d :: pComponentPrivate->curState = %d\n",__LINE__,pComponentPrivate->curState);
                
            if (pComponentPrivate->curState == commandedState){
                pComponentPrivate->cbInfo.EventHandler (pHandle, 
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventError, 
                                                        OMX_ErrorSameState,
                                                        0, NULL);
            }
            else if (pComponentPrivate->curState == OMX_StateLoaded ||
                     pComponentPrivate->curState == OMX_StateWaitForResources) {
                    
                if (pComponentPrivate->dasfmode == 1)
                {
                    pComponentPrivate->pPortDef[G711DEC_OUTPUT_PORT]->bEnabled= FALSE;
                    pComponentPrivate->pPortDef[G711DEC_OUTPUT_PORT]->bPopulated= FALSE;
                        
                    if(pComponentPrivate->streamID == 0)
                    {
                        G711DEC_DPRINT("**************************************\n");
                        G711DEC_DPRINT(":: Error = OMX_ErrorInsufficientResources\n");
                        G711DEC_DPRINT("**************************************\n");
                            
                        eError = OMX_ErrorInsufficientResources;
                        pComponentPrivate->curState = OMX_StateInvalid;
                            
                        pComponentPrivate->cbInfo.EventHandler(
                                                               pHandle, pHandle->pApplicationPrivate,
                                                               OMX_EventError, OMX_ErrorInvalidState,0, NULL);
                                     
                        goto EXIT;
                    }
                }

                if (pComponentPrivate->pPortDef[G711DEC_INPUT_PORT]->bPopulated &&  
                    pComponentPrivate->pPortDef[G711DEC_INPUT_PORT]->bEnabled)
                {
                    inputPortFlag = 1;
                }

                if (!pComponentPrivate->pPortDef[G711DEC_INPUT_PORT]->bPopulated && 
                    !pComponentPrivate->pPortDef[G711DEC_INPUT_PORT]->bEnabled)
                {
                    inputPortFlag = 1;
                }

                if (pComponentPrivate->pPortDef[G711DEC_OUTPUT_PORT]->bPopulated && 
                    pComponentPrivate->pPortDef[G711DEC_OUTPUT_PORT]->bEnabled)
                {
                    outputPortFlag = 1;
                }

                if (!pComponentPrivate->pPortDef[G711DEC_OUTPUT_PORT]->bPopulated && 
                    !pComponentPrivate->pPortDef[G711DEC_OUTPUT_PORT]->bEnabled)
                {
                    outputPortFlag = 1;
                }

                if(!(inputPortFlag && outputPortFlag))
                {
                    /* From Loaded to Idle. All enable ports are populated. */
                    pComponentPrivate->InLoaded_readytoidle = 1;            
#ifndef UNDER_CE
                    pthread_mutex_lock(&pComponentPrivate->InLoaded_mutex); 
                    pthread_cond_wait(&pComponentPrivate->InLoaded_threshold, 
                                      &pComponentPrivate->InLoaded_mutex);
                    pthread_mutex_unlock(&pComponentPrivate->InLoaded_mutex);
#else
                    Sleep(0);
#endif
                }

                G711DEC_DPRINT ("%d :: Inside G711DECHandleCommand\n",__LINE__);
                cb.LCML_Callback = (void *) G711DECLCML_Callback;
                pLcmlHandle = (OMX_HANDLETYPE) G711DECGetLCMLHandle();
                G711DEC_DPRINT ("%d :: Inside G711DECHandleCommand\n",__LINE__);

                if (pLcmlHandle == NULL) {
                    G711DEC_DPRINT("%d :: LCML Handle is NULL........exiting..\n",__LINE__);
                    goto EXIT;
                }
                    
                G711DEC_DPRINT("G711DECHandleCommand %d\n",__LINE__);
                G711DEC_DPRINT("pLcmlHandle = %p\n",pLcmlHandle);

                /* Got handle of dsp via phandle filling information about DSP  specific things */
                pLcmlDsp = (((LCML_DSP_INTERFACE*)pLcmlHandle)->dspCodec);
                G711DEC_DPRINT("pLcmlDsp = %p\n",pLcmlDsp);

                G711DEC_DPRINT("G711DECHandleCommand %d\n",__LINE__);
                eError = G711DECFill_LCMLInitParams(pHandle, pLcmlDsp, arr);
                    
                if(eError != OMX_ErrorNone) {
                    G711DEC_DPRINT("%d :: Error returned from\
                                        G711DECFill_LCMLInitParams()\n",__LINE__);
                    goto EXIT;
                }

                G711DEC_DPRINT("%d :: Comp: OMX_G711DecUtils.c\n",__LINE__);
                pComponentPrivate->pLcmlHandle = (LCML_DSP_INTERFACE *)pLcmlHandle;
                    
                /*filling create phase params */
                cb.LCML_Callback = (void *) G711DECLCML_Callback;
                G711DEC_DPRINT("%d :: Calling LCML_InitMMCodec...\n",__LINE__);
                    
                eError = LCML_InitMMCodecEx(((LCML_DSP_INTERFACE *)pLcmlHandle)->pCodecinterfacehandle,
                                            p,&pLcmlHandle,(void *)p,&cb,(OMX_STRING)pComponentPrivate->sDeviceString);
                    
                if(eError != OMX_ErrorNone) {
                    G711DEC_DPRINT("%d :: Error returned from\
                            LCML_Init()\n",__LINE__);
                    goto EXIT;
                }

#ifdef RESOURCE_MANAGER_ENABLED
                /* need check the resource with RM */
                G711DEC_DPRINT("%d :: G711DEC: About to call RMProxy_SendCommand\n", __LINE__);
    
                pComponentPrivate->rmproxyCallback.RMPROXY_Callback = 
                    (void *) G711DEC_ResourceManagerCallback;
                    
                if (pComponentPrivate->curState != OMX_StateWaitForResources){
                    rm_error = RMProxy_NewSendCommand(pHandle, RMProxy_RequestResource, 
                                                      OMX_PCM_Decoder_COMPONENT, 
                                                      OMX_G711DEC_CPU,
                                                      1234, &(pComponentPrivate->rmproxyCallback));
                    
                    G711DEC_DPRINT("%d :: G711DEC: Returned from RMProxy_SendCommand\n", __LINE__);
                    G711DEC_DPRINT("%d :: G711DEC: RMProxy_SendCommand returned %d\n", __LINE__,rm_error);
                    
                    if(rm_error == OMX_ErrorNone) {
                        /* resource is available */
                        G711DEC_DPRINT("Setting to OMX_StateIdle - Line %d\n",__LINE__);
                        
                        pComponentPrivate->curState = OMX_StateIdle;
                        pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                                pHandle->pApplicationPrivate,
                                                                OMX_EventCmdComplete, 
                                                                OMX_CommandStateSet,
                                                                pComponentPrivate->curState, 
                                                                NULL);
                                                                
                        rm_error = RMProxy_NewSendCommand(pHandle,
                                                          RMProxy_StateSet,
                                                          OMX_G711_Decoder_COMPONENT,
                                                          OMX_StateIdle, 1234, NULL);
                    }
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
                    G711DEC_DPRINT("%d :: G711DEC: Error - insufficient resources\n", __LINE__);
                }
#else
                G711DEC_DPRINT("Setting to OMX_StateIdle - Line %d\n",__LINE__);
                pComponentPrivate->curState = OMX_StateIdle;
                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete, 
                                                        OMX_CommandStateSet,
                                                        pComponentPrivate->curState, 
                                                        NULL);
#endif
                if(pComponentPrivate->dasfmode == 1) {
                    OMX_U32 pValues[4];
                    G711DEC_DPRINT("%d :: ---- Comp: DASF Functionality is ON ---\n",__LINE__);
                    G711D_OMX_MALLOC(pParams, G711DEC_AudioCodecParams);

                    pComponentPrivate->pParams = pParams;
                    pParams->iAudioFormat = iAudioFormat;
                    pParams->iSamplingRate = iSamplingRate;

                    pParams->iStrmId = pComponentPrivate->streamID;

                    pValues[0] = USN_STRMCMD_SETCODECPARAMS;
                    pValues[1] = (OMX_U32)pParams;
                    pValues[2] = sizeof(G711DEC_AudioCodecParams);
                        
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               EMMCodecControlStrmCtrl,(void *)pValues);

                    if(eError != OMX_ErrorNone) {
                        G711DEC_DPRINT("%d: Error Occurred in Codec StreamControl..\n",__LINE__);
                        goto EXIT;
                    }
                }
            }
            else if (pComponentPrivate->curState == OMX_StateExecuting) {
                char *pArgs = "damedesuStr";
                
#ifdef HASHINGENABLE
                /*Hashing Change*/
                pLcmlHandle = (LCML_DSP_INTERFACE*)pComponentPrivate->pLcmlHandle;
                eError = LCML_FlushHashes(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle);
                if (eError != OMX_ErrorNone) {
                    G711DEC_DPRINT("Error occurred in Codec mapping flush!\n");
                    break;
                }
#endif
                /*Set the bIsStopping bit */
                G711DEC_DPRINT("%d :: G711DEC: About to set bIsStopping bit\n", __LINE__);
                G711DEC_DPRINT("About to call LCML_ControlCodec(STOP)\n");
                
                eError = LCML_ControlCodec(
                                           ((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                           MMCodecControlStop,(void *)pArgs);
                            
                if(eError != OMX_ErrorNone) {
                    G711DEC_DPRINT("%d: Error Occurred in Codec Stop..\n", __LINE__);
                    goto EXIT;
                }

                pComponentPrivate->bStopSent=1;
                OMX_G711DECMEMFREE_STRUCT(pComponentPrivate->pHoldBuffer);
                pComponentPrivate->nHoldLength = 0;
            }
                
            else if(pComponentPrivate->curState == OMX_StatePause) {
#ifdef HASHINGENABLE
                /*Hashing Change*/
                pLcmlHandle = (LCML_DSP_INTERFACE*)pComponentPrivate->pLcmlHandle;
                eError = LCML_FlushHashes(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle);
                if (eError != OMX_ErrorNone) {
                    G711DEC_DPRINT("Error occurred in Codec mapping flush!\n");
                    break;
                }
#endif
                G711DEC_DPRINT("%d :: Comp: Stop Command Received\n",__LINE__);
                G711DEC_DPRINT("Setting to OMX_StateIdle - Line %d\n",__LINE__);
                pComponentPrivate->curState = OMX_StateIdle;
#ifdef RESOURCE_MANAGER_ENABLED
                rm_error = RMProxy_NewSendCommand(pHandle,
                                                  RMProxy_StateSet,
                                                  OMX_G711_Decoder_COMPONENT,
                                                  OMX_StateIdle, 1234, NULL);
#endif
                G711DEC_DPRINT ("%d :: The component is stopped\n",__LINE__);
                pComponentPrivate->cbInfo.EventHandler(pHandle,pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandStateSet,
                                                       pComponentPrivate->curState,
                                                       NULL);
            }
            else {
                /* This means, it is invalid state from application */
                G711DEC_DPRINT("%d :: Comp: OMX_G711DecUtils.c\n",__LINE__);
                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventError,
                                                       OMX_ErrorIncorrectStateTransition,
                                                       0, NULL);
            }
            break;

        case OMX_StateExecuting:
            G711DEC_DPRINT("%d: G711DECHandleCommand: Cmd Executing \n",__LINE__);
            if (pComponentPrivate->curState == commandedState){
                pComponentPrivate->cbInfo.EventHandler (pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventError, 
                                                        OMX_ErrorSameState,
                                                        0, NULL);
                G711DEC_DPRINT("%d :: Error: Same State Given by Application\n",__LINE__);
                goto EXIT;
            }
            else if (pComponentPrivate->curState == OMX_StateIdle) {
                G711DEC_DPRINT("%d :: Comp: OMX_G711DecUtils.c\n",__LINE__);
                /* Sending commands to DSP via LCML_ControlCodec third argument  is not used for time being */
                pComponentPrivate->nFillBufferDoneCount = 0;
                pComponentPrivate->bStopSent=0;

                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                           EMMCodecControlStart, (void *)p);

                if(eError != OMX_ErrorNone) {
                    G711DEC_DPRINT("%d: Error Occurred in Codec Start..\n",__LINE__);
                    goto EXIT;
                }
                /* Send input buffers to application */
                nBuf = pComponentPrivate->pInputBufferList->numBuffers;
                G711DEC_DPRINT ("nBuf =  %d\n",(int)nBuf);
                /* Send output buffers to codec */
            }
            else if (pComponentPrivate->curState == OMX_StatePause) {
                G711DEC_DPRINT("%d :: Comp: OMX_G711DecUtils.c\n",__LINE__);
                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                           EMMCodecControlStart, (void *)p);
                if (eError != OMX_ErrorNone) {
                    G711DEC_DPRINT ("Error While Resuming the codec\n");
                    goto EXIT;
                }

                for (i=0; i < pComponentPrivate->nNumInputBufPending; i++) {
                    if (pComponentPrivate->pInputBufHdrPending[i]) {
                        G711DECGetCorresponding_LCMLHeader(pComponentPrivate->pInputBufHdrPending[i]->pBuffer, OMX_DirInput, &pLcmlHdr);
                        G711DEC_SetPending(pComponentPrivate,pComponentPrivate->pInputBufHdrPending[i],OMX_DirInput);

                        eError = LCML_QueueBuffer(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                  EMMCodecInputBuffer,
                                                  pComponentPrivate->pInputBufHdrPending[i]->pBuffer,
                                                  pComponentPrivate->pInputBufHdrPending[i]->nAllocLen,
                                                  pComponentPrivate->pInputBufHdrPending[i]->nFilledLen,
                                                  (OMX_U8 *) pLcmlHdr->pIpParam,
                                                  sizeof(G711DEC_UAlgInBufParamStruct),
                                                  NULL);
                    }
                }
                pComponentPrivate->nNumInputBufPending = 0;

                G711DEC_DPRINT("%d :: pComponentPrivate->nNumOutputBufPending %d\n", __LINE__,(int)pComponentPrivate->nNumOutputBufPending);
                for (i=0; i < pComponentPrivate->nNumOutputBufPending; i++) {
                    if (pComponentPrivate->pOutputBufHdrPending[i]) {
                        G711DECGetCorresponding_LCMLHeader(pComponentPrivate->pOutputBufHdrPending[i]->pBuffer, OMX_DirOutput, &pLcmlHdr);
                        G711DEC_SetPending(pComponentPrivate,pComponentPrivate->pOutputBufHdrPending[i],OMX_DirOutput);
                        eError = LCML_QueueBuffer(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                  EMMCodecOuputBuffer, 
                                                  pComponentPrivate->pOutputBufHdrPending[i]->pBuffer, 
                                                  pComponentPrivate->pOutputBufHdrPending[i]->nAllocLen,
                                                  pComponentPrivate->pOutputBufHdrPending[i]->nFilledLen,
                                                  NULL, 0, NULL);                        
                        G711DEC_DPRINT("%d :: eError LCML_QueueBuffer 0x%x\n",__LINE__,eError);
                    }

                }
                pComponentPrivate->nNumOutputBufPending = 0;
            }
            else {
                G711DEC_DPRINT("%d :: Comp: OMX_G711DecUtils.c\n",__LINE__);
                pComponentPrivate->cbInfo.EventHandler (pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventError, 
                                                        OMX_ErrorIncorrectStateTransition,
                                                        0, NULL);
                G711DEC_DPRINT("%d :: Error: Invalid State Given by \
                       Application\n",__LINE__);
                goto EXIT;
            }

#ifdef RESOURCE_MANAGER_ENABLED
            rm_error = RMProxy_NewSendCommand(pHandle, RMProxy_StateSet,
                                              OMX_G711_Decoder_COMPONENT,
                                              OMX_StateExecuting, 1234, NULL);
#endif
            /*Send state change notificaiton to Application */
            pComponentPrivate->curState = OMX_StateExecuting;
            pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                   pHandle->pApplicationPrivate,
                                                   OMX_EventCmdComplete, 
                                                   OMX_CommandStateSet,
                                                   pComponentPrivate->curState, NULL);
            break;

        case OMX_StateLoaded:
            G711DEC_DPRINT("%d: G711DECHandleCommand: Cmd Loaded - curState = %d\n",__LINE__,pComponentPrivate->curState);
            if (pComponentPrivate->curState == commandedState){
                pComponentPrivate->cbInfo.EventHandler (pHandle, 
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventError, 
                                                        OMX_ErrorSameState,
                                                        0, NULL);
                G711DEC_DPRINT("%d :: Error: Same State Given by Application\n",__LINE__);
                break;
            }
            G711DEC_DPRINT("%d: pComponentPrivate->pInputBufferList->numBuffers = %d\n",__LINE__,(int)pComponentPrivate->pInputBufferList->numBuffers);
            G711DEC_DPRINT("%d: pComponentPrivate->pOutputBufferList->numBuffers = %d\n",__LINE__,(int)pComponentPrivate->pOutputBufferList->numBuffers);

            if (pComponentPrivate->curState == OMX_StateWaitForResources){
                G711DEC_DPRINT("%d: G711DECHandleCommand: Cmd Loaded\n",__LINE__);
                pComponentPrivate->curState = OMX_StateLoaded;

                pComponentPrivate->cbInfo.EventHandler (pHandle, 
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete, 
                                                        OMX_CommandStateSet,
                                                        pComponentPrivate->curState,NULL);
                break;
            }
            
            G711DEC_DPRINT("%d :: In side OMX_StateLoaded State: \n",__LINE__);
            
            if (pComponentPrivate->curState != OMX_StateIdle &&
                pComponentPrivate->curState != OMX_StateWaitForResources) {
                G711DEC_DPRINT("%d :: Comp: OMX_G711DecUtils.c\n",__LINE__);
                pComponentPrivate->cbInfo.EventHandler (pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventError, 
                                                        OMX_ErrorIncorrectStateTransition,
                                                        0, NULL);
                G711DEC_DPRINT("%d :: Error: Invalid State Given by \
                                                Application\n",__LINE__);
                goto EXIT;
            }

            G711DEC_DPRINT("pComponentPrivate->pInputBufferList->numBuffers = %d\n",
                           (int)pComponentPrivate->pInputBufferList->numBuffers);
            G711DEC_DPRINT("pComponentPrivate->pOutputBufferList->numBuffers = %d\n",
                           (int)pComponentPrivate->pOutputBufferList->numBuffers);
                            
            if (pComponentPrivate->pInputBufferList->numBuffers &&
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
            }

            /* Now Deinitialize the component No error should be returned from
             * this function. It should clean the system as much as possible */
            G711DEC_DPRINT("%d :: In side OMX_StateLoaded State: \n",__LINE__);
            eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                       EMMCodecControlDestroy, (void *)p);
                                        
            G711DEC_DPRINT("%d :: In side OMX_StateLoaded State: \n",__LINE__);
            if (eError != OMX_ErrorNone) {
                G711DEC_DPRINT("%d : Error: in Destroying the codec: no.  %x\n",__LINE__, eError);
                goto EXIT;
            }
            
            G711DEC_DPRINT("%d: G711DECHandleCommand: Cmd Loaded\n",__LINE__);
            eError = EXIT_COMPONENT_THRD;
            pComponentPrivate->bInitParamsInitialized = 0;
            pComponentPrivate->bLoadedCommandPending = OMX_FALSE;
            /* Send StateChangeNotification to application */
            break;

        case OMX_StatePause:
            G711DEC_DPRINT("%d: G711DECHandleCommand: Cmd Pause\n",__LINE__);
            if (pComponentPrivate->curState == commandedState){
                pComponentPrivate->cbInfo.EventHandler (pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventError,
                                                        OMX_ErrorSameState, 0, NULL);
                                                            
                G711DEC_DPRINT("%d :: Error: Same State Given by \
                           Application\n",__LINE__);
                break;
            }
            if (pComponentPrivate->curState != OMX_StateExecuting &&
                pComponentPrivate->curState != OMX_StateIdle) {
                G711DEC_DPRINT("%d :: Comp: OMX_G711DecUtils.c\n",__LINE__);
                pComponentPrivate->cbInfo.EventHandler (pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventError, 
                                                        OMX_ErrorIncorrectStateTransition,
                                                        0, NULL);
                G711DEC_DPRINT("%d :: Error: Invalid State Given by \
                       Application\n",__LINE__);
                goto EXIT;
            }
            eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                       EMMCodecControlPause, (void *)p);

            if (eError != OMX_ErrorNone) {
                G711DEC_DPRINT("%d : Error: in Pausing the codec\n",__LINE__);
                goto EXIT;
            }
            break;

        case OMX_StateWaitForResources:

            if (pComponentPrivate->curState == commandedState){
                G711DEC_DPRINT("%d :: Comp: OMX_G711DecUtils.c\n",__LINE__);
                pComponentPrivate->cbInfo.EventHandler (pHandle, 
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventError, 
                                                        OMX_ErrorSameState,
                                                        0, NULL);
                                                            
                G711DEC_DPRINT("%d :: Error: Same State Given by \
                           Application\n",__LINE__);
            }
            else if (pComponentPrivate->curState == OMX_StateLoaded) {
                G711DEC_DPRINT("%d :: Comp: OMX_G711DecUtils.c\n",__LINE__);
                pComponentPrivate->curState = OMX_StateWaitForResources;
                pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete, 
                                                       OMX_CommandStateSet,
                                                       pComponentPrivate->curState,NULL);
            }
            else{
                G711DEC_DPRINT("%d :: Comp: OMX_G711DecUtils.c\n",__LINE__);
                pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventError, 
                                                       OMX_ErrorIncorrectStateTransition,
                                                       0, "NULL");
            }
            break;

        case OMX_StateInvalid:
            G711DEC_DPRINT("%d: G711DECHandleCommand: Cmd OMX_StateInvalid:\n",__LINE__);
            if (pComponentPrivate->curState == commandedState){
                pComponentPrivate->cbInfo.EventHandler (pHandle, 
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventError, 
                                                        OMX_ErrorSameState,
                                                        0, NULL);
                G711DEC_DPRINT("%d :: Error: Same State Given by \
                           Application\n",__LINE__);
            }
            else{
                G711DEC_DPRINT("%d :: Comp: OMX_G711DecUtils.c\n",__LINE__);
                if (pComponentPrivate->curState != OMX_StateWaitForResources && 
                    pComponentPrivate->curState != OMX_StateLoaded) {
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               EMMCodecControlDestroy, (void *)p);
                }

                pComponentPrivate->curState = OMX_StateInvalid;
                pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventError, 
                                                       OMX_ErrorInvalidState,
                                                       0, NULL);
                G711DEC_CleanupInitParams(pHandle);
            }
            break;

        case OMX_StateMax:
            G711DEC_DPRINT("%d: G711DECHandleCommand: Cmd OMX_StateMax::\n",__LINE__);
            break;
        } /* End of Switch */
    }
    else if (command == OMX_CommandMarkBuffer) {
        G711DEC_DPRINT("command OMX_CommandMarkBuffer received %d\n",__LINE__);
        if(!pComponentPrivate->pMarkBuf){
            G711DEC_DPRINT("command OMX_CommandMarkBuffer received %d\n",__LINE__);
            pComponentPrivate->pMarkBuf = (OMX_MARKTYPE *)(commandData);
        }
    }
    else if (command == OMX_CommandPortDisable) {
        if (!pComponentPrivate->bDisableCommandPending) {
            if(commandData == 0x0 || commandData == -1){
                /* disable port */
                pComponentPrivate->pPortDef[G711DEC_INPUT_PORT]->bEnabled = OMX_FALSE;
            }
            
            if(commandData == 0x1 || commandData == -1){
                char *pArgs = "damedesuStr";
                pComponentPrivate->pPortDef[G711DEC_OUTPUT_PORT]->bEnabled = OMX_FALSE;
                
                if (pComponentPrivate->curState == OMX_StateExecuting) {
                    pComponentPrivate->bNoIdleOnStop = OMX_TRUE;
                    G711DEC_DPRINT("Calling LCML_ControlCodec() Line %d\n",__LINE__);
                    eError = LCML_ControlCodec(
                                               ((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               MMCodecControlStop,(void *)pArgs);
                }
            }
        }

        if(commandData == 0x0) {
            if(!pComponentPrivate->pPortDef[G711DEC_INPUT_PORT]->bPopulated){
                /* return cmdcomplete event if input unpopulated */
                pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete, 
                                                       OMX_CommandPortDisable,
                                                       G711DEC_INPUT_PORT, NULL);
                G711DEC_DPRINT("Clearing bDisableCommandPending Line %d\n",__LINE__);
                pComponentPrivate->bDisableCommandPending = 0;
            }
            else{
                pComponentPrivate->bDisableCommandPending = 1;
                pComponentPrivate->bDisableCommandParam = commandData;
            }
        }

        if(commandData == 0x1) {
            if (!pComponentPrivate->pPortDef[G711DEC_OUTPUT_PORT]->bPopulated){
                /* return cmdcomplete event if output unpopulated */
                pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete, 
                                                       OMX_CommandPortDisable,
                                                       G711DEC_OUTPUT_PORT, NULL);
                                                    
                G711DEC_DPRINT("Clearing bDisableCommandPending Line %d\n",__LINE__);
                pComponentPrivate->bDisableCommandPending = 0;
            }
            else {
                pComponentPrivate->bDisableCommandPending = 1;
                pComponentPrivate->bDisableCommandParam = commandData;
            }
        }

        if(commandData == -1) {
            if (!pComponentPrivate->pPortDef[G711DEC_INPUT_PORT]->bPopulated &&
                !pComponentPrivate->pPortDef[G711DEC_OUTPUT_PORT]->bPopulated){

                /* return cmdcomplete event if inout & output unpopulated */
                pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete, 
                                                       OMX_CommandPortDisable,
                                                       G711DEC_INPUT_PORT, NULL);

                pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete, 
                                                       OMX_CommandPortDisable,
                                                       G711DEC_OUTPUT_PORT, NULL);
                                                    
                G711DEC_DPRINT("Clearing bDisableCommandPending Line %d\n",__LINE__);
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
                G711DEC_DPRINT ("setting input port to enabled\n");
                
                G711DEC_DPRINT ("pComponentPrivate->pPortDef[G711DEC_INPUT_PORT]->bEnabled = %d\n",pComponentPrivate->pPortDef[G711DEC_INPUT_PORT]->bEnabled);
                if(pComponentPrivate->AlloBuf_waitingsignal)
                {
                    G711DEC_DPRINT ("%d :: Unblock AlloBuf_threshold\n", __LINE__);                
                    pComponentPrivate->AlloBuf_waitingsignal = 0;
                }
                pComponentPrivate->pPortDef[G711DEC_INPUT_PORT]->bEnabled = OMX_TRUE;
                /* Removing sleep calls. */
                G711DEC_DPRINT ("pComponentPrivate->pPortDef[INPUT_PORT]->bEnabled = %d\n",
                                pComponentPrivate->pPortDef[G711DEC_INPUT_PORT]->bEnabled);
            }
            if(commandData == 0x1 || commandData == -1){
                /* Removing sleep() calls. */
                if(pComponentPrivate->AlloBuf_waitingsignal)
                {
                    G711DEC_DPRINT ("%d :: Unblock AlloBuf_threshold", __LINE__); 
                    pComponentPrivate->AlloBuf_waitingsignal = 0;
#ifndef UNDER_CE                 
                    pthread_mutex_lock(&pComponentPrivate->AlloBuf_mutex); 
                    pthread_cond_signal(&pComponentPrivate->AlloBuf_threshold);
                    pthread_mutex_unlock(&pComponentPrivate->AlloBuf_mutex);    
#else
                    OMX_SignalEvent(&(pComponentPrivate->AlloBuf_event));
#endif                 
                }

                /* Removing sleep() calls. */            
                /* enable out port */
                if (pComponentPrivate->curState == OMX_StateExecuting) 
                {
                    char *pArgs = "damedesuStr";
                    pComponentPrivate->bDspStoppedWhileExecuting = OMX_FALSE;
                    eError = LCML_ControlCodec(
                                               ((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               EMMCodecControlStart,(void *)pArgs);
                }
                G711DEC_DPRINT("setting output port to enabled\n");
                pComponentPrivate->pPortDef[G711DEC_OUTPUT_PORT]->bEnabled = OMX_TRUE;
                G711DEC_DPRINT("pComponentPrivate->pPortDef[G711DEC_OUTPUT_PORT]->bEnabled = %d\n",pComponentPrivate->pPortDef[G711DEC_OUTPUT_PORT]->bEnabled);
            }
        }
        if(commandData == 0x0)
        {
            if (pComponentPrivate->curState == OMX_StateLoaded || 
                pComponentPrivate->pPortDef[G711DEC_INPUT_PORT]->bPopulated)
            {
                pComponentPrivate->cbInfo.EventHandler( pHandle, 
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete, 
                                                        OMX_CommandPortEnable,
                                                        G711DEC_INPUT_PORT, 
                                                        NULL);
                pComponentPrivate->bEnableCommandPending = 0;
            }
            else {
                pComponentPrivate->bEnableCommandPending = 1;
                pComponentPrivate->bEnableCommandParam = commandData;
            }
        }
        else if(commandData == 0x1){
            if (pComponentPrivate->curState == OMX_StateLoaded || 
                pComponentPrivate->pPortDef[G711DEC_OUTPUT_PORT]->bPopulated){
                pComponentPrivate->cbInfo.EventHandler( pHandle, 
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete, 
                                                        OMX_CommandPortEnable,
                                                        G711DEC_OUTPUT_PORT, 
                                                        NULL);
                pComponentPrivate->bEnableCommandPending = 0;
            }
            else {
                pComponentPrivate->bEnableCommandPending = 1;
                pComponentPrivate->bEnableCommandParam = commandData;
            }
        }
        else if(commandData == -1 ){
            if (pComponentPrivate->curState == OMX_StateLoaded || 
                (pComponentPrivate->pPortDef[G711DEC_INPUT_PORT]->bPopulated
                 && pComponentPrivate->pPortDef[G711DEC_OUTPUT_PORT]->bPopulated)){
                pComponentPrivate->cbInfo.EventHandler( pHandle, 
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete, 
                                                        OMX_CommandPortEnable,
                                                        G711DEC_INPUT_PORT, 
                                                        NULL);

                pComponentPrivate->cbInfo.EventHandler( pHandle, 
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete, 
                                                        OMX_CommandPortEnable,
                                                        G711DEC_OUTPUT_PORT, 
                                                        NULL);
                pComponentPrivate->bEnableCommandPending = 0;
                G711DECFill_LCMLInitParamsEx(pComponentPrivate->pHandle);
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
        if(commandData == 0x0 || commandData == -1){
            for (i=0; i < MAX_NUM_OF_BUFS; i++)
            {
                pComponentPrivate->pInputBufHdrPending[i] = NULL;
            }            
            for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++)
            {
                G711DEC_DPRINT("line %d:: Calling EmptyBufferDone\n",__LINE__);
                pComponentPrivate->cbInfo.EmptyBufferDone (
                                                           pComponentPrivate->pHandle,
                                                           pComponentPrivate->pHandle->pApplicationPrivate,
                                                           pComponentPrivate->pInputBufferList->pBufHdr[i]
                                                           );
                pComponentPrivate->nEmptyBufferDoneCount++;
                pComponentPrivate->nNumInputBufPending = 0;
            }

            /* return all input buffers */
            pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                   pHandle->pApplicationPrivate,
                                                   OMX_EventCmdComplete, 
                                                   OMX_CommandFlush,
                                                   G711DEC_INPUT_PORT, NULL);
        }
        if(commandData == 0x1 || commandData == -1){
            for (i=0; i < MAX_NUM_OF_BUFS; i++)
            {
                pComponentPrivate->pOutputBufHdrPending[i] = NULL;
            }
            /* return all output buffers */
            for (i=0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {
                G711DEC_DPRINT("Calling FillBufferDone From Line %d\n",__LINE__);
                pComponentPrivate->cbInfo.FillBufferDone (
                                                          pComponentPrivate->pHandle,
                                                          pComponentPrivate->pHandle->pApplicationPrivate,
                                                          pComponentPrivate->pOutputBufferList->pBufHdr[i]
                                                          );
                pComponentPrivate->nFillBufferDoneCount++;
                pComponentPrivate->nNumOutputBufPending = 0;
            }

            pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                   pHandle->pApplicationPrivate,
                                                   OMX_EventCmdComplete, 
                                                   OMX_CommandFlush,
                                                   G711DEC_OUTPUT_PORT, NULL);
        }
    }

 EXIT:
    G711DEC_DPRINT ("%d :: Exiting G711DECHandleCommand Function\n",__LINE__);
    G711DEC_DPRINT ("%d :: Returning %d\n",__LINE__,eError);
    return eError;
}


/* ========================================================================== */
/**
 * @G711DECHandleDataBuf_FromApp() This function is called by the component when ever it
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
OMX_ERRORTYPE G711DECHandleDataBuf_FromApp(OMX_BUFFERHEADERTYPE* pBufHeader,
                                           G711DEC_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_DIRTYPE eDir;
    LCML_G711DEC_BUFHEADERTYPE *pLcmlHdr = NULL;
    LCML_DSP_INTERFACE *pLcmlHandle = (LCML_DSP_INTERFACE *)
        pComponentPrivate->pLcmlHandle;
    OMX_U32 frameLength = 0;
    OMX_U8* pExtraData = NULL;
    OMX_U32 holdBufferSize = 0;

    OMX_U8 nFrames = 0;
    OMX_U8 *frameType = NULL;
    LCML_DSP_INTERFACE * phandle = NULL;
    OMX_U8 *pBufParmsTemp = NULL;
    
    G711DEC_DPRINT ("%d :: Entering G711DECHandleDataBuf_FromApp Function\n",__LINE__);

    holdBufferSize = (pComponentPrivate->pPortDef[G711DEC_INPUT_PORT]->nBufferSize) * 
        (pComponentPrivate->pInputBufferList->numBuffers + 1);
    /*Find the direction of the received buffer from buffer list */
    eError = G711DECGetBufferDirection(pBufHeader, &eDir);
    if (eError != OMX_ErrorNone) {
        G711DEC_DPRINT ("%d :: The PBufHeader is not found in the list\n",
                        __LINE__);
        goto EXIT;
    }
    
    if (eDir == OMX_DirInput) {
        
        if ( pBufHeader->nFilledLen > 0 ) {
            pComponentPrivate->bBypassDSP = 0;
            
            if ( pComponentPrivate->nHoldLength == 0 ) {

                G711DEC_DPRINT("G711DECHandleDataBuf_FromApp - reading G711DEC\n");
                frameLength = pComponentPrivate->pPortDef[G711DEC_INPUT_PORT]->nBufferSize;

                if ( pBufHeader->nFilledLen >= frameLength ) {
                
                    /* Copy the aFillBufferPtr from the 2nd frame to iHoldBuffer*/
                    /* The 1st frame will stay in aFillBufferPtr to be passed down.*/
                    /* the length of remaining Frames in aFillBufferPtr.*/
                    pComponentPrivate->nHoldLength = pBufHeader->nFilledLen - frameLength;

                    if ( pComponentPrivate->nHoldLength > 0 ) {/* something need to be hold in iHoldBuffer */
                        if (pComponentPrivate->pHoldBuffer == NULL) {
                            G711D_OMX_MALLOC_SIZE(pComponentPrivate->pHoldBuffer, holdBufferSize, OMX_U8);
                        }

                        /* Copy the extra data into pHoldBuffer. Size will be nHoldLength. */
                        pExtraData = pBufHeader->pBuffer + (pComponentPrivate->pPortDef[G711DEC_INPUT_PORT]->nBufferSize);
                        memcpy (pComponentPrivate->pHoldBuffer, pExtraData, pComponentPrivate->nHoldLength);
                    }
                }
                else {
                    /* received buffer with less than 1 G711 frame. Save the data in iHoldBuffer.*/
                    pComponentPrivate->nHoldLength = pBufHeader->nFilledLen;

                    /* save the data into iHoldBuffer.*/
                    if (pComponentPrivate->pHoldBuffer == NULL) {
                        G711D_OMX_MALLOC_SIZE(pComponentPrivate->pHoldBuffer, holdBufferSize, OMX_U8);
                    }
                    /* Not enough data to be sent. Copy all received data into iHoldBuffer.*/
                    /* Size to be copied will be iHoldLen == mmData->BufferSize() */
                    memcpy (pComponentPrivate->pHoldBuffer, 
                            pBufHeader->pBuffer, pComponentPrivate->nHoldLength);

                    /* since not enough data, we shouldn't send anything to SN, but instead request to EmptyBufferDone again.*/
                    G711DEC_DPRINT("line %d:: Calling EmptyBufferDone\n",__LINE__);
                    if (pComponentPrivate->curState != OMX_StatePause) {
                        pComponentPrivate->cbInfo.EmptyBufferDone (
                                                                   pComponentPrivate->pHandle,
                                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                                   pBufHeader);
                        pComponentPrivate->nEmptyBufferDoneCount++;
                    }
                    else {
                        pComponentPrivate->pInputBufHdrPending[pComponentPrivate->nNumInputBufPending++] = pBufHeader;
                    }


                    goto EXIT;
                }
            }
            else {
                /* iHoldBuffer has data. There is no possibility that data in iHoldBuffer is less for 1 frame without*/
                /* lastBufferFlag being set. Unless it's a corrupt file.*/
                /* Copy the data in dataPtr to iHoldBuffer. Update the iHoldBuffer size (iHoldLen).*/


                pExtraData = pComponentPrivate->pHoldBuffer + pComponentPrivate->nHoldLength;
                memcpy(pExtraData,pBufHeader->pBuffer,pBufHeader->nFilledLen);

                pComponentPrivate->nHoldLength += pBufHeader->nFilledLen;

                frameLength = (pComponentPrivate->pPortDef[G711DEC_INPUT_PORT]->nBufferSize);

                if ( pComponentPrivate->nHoldLength >= frameLength )  {
                    /* Copy the data from pComponentPrivate->pHoldBuffer to pBufHeader->pBuffer*/
                    memcpy(pBufHeader->pBuffer,pComponentPrivate->pHoldBuffer,frameLength);

                    /* Now the pHoldBuffer has framelength fewer bytes - update nHoldLength*/
                    pComponentPrivate->nHoldLength = pComponentPrivate->nHoldLength - frameLength;
                    
                    /* Shift the remaining bytes to the beginning of the pHoldBuffer */
                    pExtraData = pComponentPrivate->pHoldBuffer + frameLength;
                    if (frameLength >= pComponentPrivate->nHoldLength)
                        memcpy(pComponentPrivate->pHoldBuffer,pExtraData, pComponentPrivate->nHoldLength);
                    else
                        memmove(pComponentPrivate->pHoldBuffer,pExtraData, pComponentPrivate->nHoldLength);


                    /* Clear the rest of the data from the pHoldBuffer */
                    pExtraData = pComponentPrivate->pHoldBuffer + pComponentPrivate->nHoldLength;
                    memset(pExtraData,0,holdBufferSize - pComponentPrivate->nHoldLength);
                }
                else {
                    if (pComponentPrivate->curState != OMX_StatePause) {
                        G711DEC_DPRINT("line %d:: Calling EmptyBufferDone\n",__LINE__);
                        pComponentPrivate->cbInfo.EmptyBufferDone (
                                                                   pComponentPrivate->pHandle,
                                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                                   pBufHeader);
                        pComponentPrivate->nEmptyBufferDoneCount++;
                        goto EXIT;
                    }
                    else {
                        pComponentPrivate->pInputBufHdrPending[pComponentPrivate->nNumInputBufPending++] = pBufHeader;
                    }
                }
            }
        
            eError = G711DECGetCorresponding_LCMLHeader(pBufHeader->pBuffer, OMX_DirInput, &pLcmlHdr);
            if (eError != OMX_ErrorNone) {
                G711DEC_DPRINT("%d :: Error: Invalid Buffer Came ...\n",__LINE__);
                goto EXIT;
            }
            phandle = (LCML_DSP_INTERFACE *)(
                                             ((LCML_CODEC_INTERFACE *)pLcmlHandle->pCodecinterfacehandle)->pCodec);     
            
            nFrames = (OMX_U8)(pBufHeader->nFilledLen / RTP_Framesize);
            frameType = pBufHeader->pBuffer;
            frameType += RTP_Framesize - 1;
     
            if( (pLcmlHdr->pBufferParam->usNbFrames < nFrames) && 
                (pLcmlHdr->pFrameParam != NULL)){
                OMX_DmmUnMap(phandle->dspCodec->hProc,
                             (void*)pLcmlHdr->pBufferParam->pParamElem,
                             pLcmlHdr->pDmmBuf->pReserved);
                             
                pLcmlHdr->pBufferParam->pParamElem = NULL; 
                
                pBufParmsTemp = (OMX_U8*)pLcmlHdr->pFrameParam;
                pBufParmsTemp -= 128;
                newfree(pBufParmsTemp);
                pLcmlHdr->pFrameParam = NULL;                           
            }      

            if(pLcmlHdr->pFrameParam == NULL ){
                G711D_OMX_MALLOC_SIZE(pBufParmsTemp,
                                      ((sizeof(G711DEC_FrameStruct)*nFrames) + 256),
                                      OMX_U8);
                
                pLcmlHdr->pFrameParam = (G711DEC_FrameStruct*)(pBufParmsTemp + 128);
                eError = OMX_DmmMap(phandle->dspCodec->hProc, 
                                    nFrames*sizeof(G711DEC_FrameStruct),
                                    (void*)pLcmlHdr->pFrameParam, (pLcmlHdr->pDmmBuf));        
                
                if (eError != OMX_ErrorNone){
                    G711DEC_PRINT("OMX_DmmMap ERRROR!!!!\n\n");
                    goto EXIT;
                }
                pLcmlHdr->pBufferParam->pParamElem = 
                    (G711DEC_FrameStruct *)pLcmlHdr->pDmmBuf->pMapped;
            }
        
            if(pBufHeader->nFlags == OMX_BUFFERFLAG_EOS) {
                (pLcmlHdr->pFrameParam+(nFrames-1))->usLastFrame = OMX_BUFFERFLAG_EOS;
                pComponentPrivate->bPlayCompleteFlag = 1;
                pBufHeader->nFlags = 0;
            }
            
            pLcmlHdr->pBufferParam->usNbFrames = nFrames;

            /*Store tick count information*/
            pComponentPrivate->arrBufIndexTick[pComponentPrivate->IpBufindex] = pBufHeader->nTickCount;
           
            /* Store time stamp information */
            pComponentPrivate->arrBufIndex[pComponentPrivate->IpBufindex] = pBufHeader->nTimeStamp;
            pComponentPrivate->IpBufindex++;
            pComponentPrivate->IpBufindex %= pComponentPrivate->pPortDef[G711DEC_INPUT_PORT]->nBufferCountActual;
            if (pComponentPrivate->curState == OMX_StateExecuting) {
                if (!G711DEC_IsPending(pComponentPrivate,pBufHeader,OMX_DirInput)) {
                    G711DEC_SetPending(pComponentPrivate,pBufHeader,OMX_DirInput);
                 
                    eError = LCML_QueueBuffer(pLcmlHandle->pCodecinterfacehandle,
                                              EMMCodecInputBuffer,  
                                              (OMX_U8 *)pBufHeader->pBuffer, 
                                              STD_G711DEC_BUF_SIZE*nFrames,
                                              STD_G711DEC_BUF_SIZE*nFrames,
                                              (OMX_U8 *) pLcmlHdr->pBufferParam,
                                              sizeof(G711DEC_ParamStruct),
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
            else if (pComponentPrivate->curState == OMX_StatePause){
                pComponentPrivate->pInputBufHdrPending[pComponentPrivate->nNumInputBufPending++] = pBufHeader;
            }

        }
        else {
            pComponentPrivate->bBypassDSP = 1;
            if (pComponentPrivate->dasfmode == 0 && pBufHeader->nFlags != OMX_BUFFERFLAG_EOS) {
                G711DEC_DPRINT("line %d:: Calling EmptyBufferDone\n",__LINE__);
                pComponentPrivate->cbInfo.EmptyBufferDone (
                                                           pComponentPrivate->pHandle,
                                                           pComponentPrivate->pHandle->pApplicationPrivate,
                                                           pComponentPrivate->pInputBufferList->pBufHdr[0]
                                                           );
                pComponentPrivate->nEmptyBufferDoneCount++;
            }
        }
        if(pBufHeader->nFlags == OMX_BUFFERFLAG_EOS)
        {
            if(pComponentPrivate->dasfmode == 0 && pBufHeader->nFilledLen == 0)
            {
                pComponentPrivate->pOutputBufferList->pBufHdr[0]->nFlags |= OMX_BUFFERFLAG_EOS;
            }
        
            pComponentPrivate->bIsEOFSent = 0;
            if(pComponentPrivate->dasfmode == 0) {
                pComponentPrivate->pOutputBufferList->pBufHdr[0]->nFlags |= OMX_BUFFERFLAG_EOS;
                pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                                       OMX_EventBufferFlag,
                                                       pComponentPrivate->pOutputBufferList->pBufHdr[0]->nOutputPortIndex,
                                                       pComponentPrivate->pOutputBufferList->pBufHdr[0]->nFlags, NULL);
             
                pComponentPrivate->cbInfo.FillBufferDone (pComponentPrivate->pHandle,
                                                          pComponentPrivate->pHandle->pApplicationPrivate,
                                                          pComponentPrivate->pOutputBufferList->pBufHdr[0]);
             
                pComponentPrivate->cbInfo.EmptyBufferDone (pComponentPrivate->pHandle,
                                                           pComponentPrivate->pHandle->pApplicationPrivate,
                                                           pComponentPrivate->pInputBufferList->pBufHdr[0]);
                G711DEC_DPRINT ("%d :: Flags has been propagated from input to output port\n",__LINE__);
            }
            pBufHeader->nFlags = 0;
        
        }
        if(pBufHeader->pMarkData){
            /* copy mark to output buffer header */
            pComponentPrivate->pOutputBufferList->pBufHdr[0]->pMarkData = pBufHeader->pMarkData;
            pComponentPrivate->pOutputBufferList->pBufHdr[0]->hMarkTargetComponent = pBufHeader->hMarkTargetComponent;

            /* trigger event handler if we are supposed to */
            if(pBufHeader->hMarkTargetComponent == pComponentPrivate->pHandle && pBufHeader->pMarkData){
                pComponentPrivate_CC->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                          pComponentPrivate->pHandle->pApplicationPrivate,
                                                          OMX_EventMark, 0, 0, pBufHeader->pMarkData);
            }
        }
    }
    else if (eDir == OMX_DirOutput) {
        /* Make sure that output buffer is issued to output stream only when
         * there is an outstanding input buffer already issued on input stream
         */
        if (!(pComponentPrivate->bIsStopping)) {
            if (pComponentPrivate->bBypassDSP == 0) {
                G711DEC_DPRINT ("%d: Sending Empty OUTPUT BUFFER to Codec = %p\n",__LINE__,pBufHeader->pBuffer);

                if (pComponentPrivate->curState == OMX_StateExecuting) {
                    if (!G711DEC_IsPending(pComponentPrivate,pBufHeader,OMX_DirOutput)) {
                        G711DEC_SetPending(pComponentPrivate,pBufHeader,OMX_DirOutput);

                        
                        eError = LCML_QueueBuffer(pLcmlHandle->pCodecinterfacehandle,
                                                  EMMCodecOuputBuffer, 
                                                  (OMX_U8 *)pBufHeader->pBuffer, 
                                                  (pComponentPrivate->pPortDef[G711DEC_OUTPUT_PORT]->nBufferSize),
                                                  (pComponentPrivate->pPortDef[G711DEC_OUTPUT_PORT]->nBufferSize),
                                                  NULL, 0, NULL);

                        G711DEC_DPRINT("%d :: LCML_QueueBuffer eError 0x%x\n",__LINE__, eError);
                        if (eError != OMX_ErrorNone ) {
                            G711DEC_DPRINT ("%d :: IssuingDSP OP: Error Occurred\n",__LINE__);
                            eError = OMX_ErrorHardware;
                            goto EXIT;
                        }
                        pComponentPrivate->lcml_nOpBuf++;
                        pComponentPrivate->num_Op_Issued++;
                    }
                }
                else if (pComponentPrivate->curState == OMX_StatePause){
                    pComponentPrivate->pOutputBufHdrPending[pComponentPrivate->nNumOutputBufPending++] = pBufHeader;
                    G711DEC_DPRINT("%d :: pComponentPrivate->nNumOutputBufPending 0x%x\n",__LINE__,(int)pComponentPrivate->nNumOutputBufPending);
                }
            }
            
            else {
                pComponentPrivate->cbInfo.FillBufferDone (pComponentPrivate->pHandle,
                                                          pComponentPrivate->pHandle->pApplicationPrivate,
                                                          pComponentPrivate->pOutputBufferList->pBufHdr[0]);
            }
        }
        else {
            if (pComponentPrivate->curState == OMX_StateExecuting) {
                if (!G711DEC_IsPending(pComponentPrivate,pBufHeader,OMX_DirOutput)) {
                    
                    G711DEC_SetPending(pComponentPrivate,pBufHeader,OMX_DirOutput);
                    eError = LCML_QueueBuffer(pLcmlHandle->pCodecinterfacehandle,
                                              EMMCodecOuputBuffer, (OMX_U8 *)pBufHeader->pBuffer, 
                                              (pComponentPrivate->pPortDef[G711DEC_OUTPUT_PORT]->nBufferSize),
                                              (pComponentPrivate->pPortDef[G711DEC_OUTPUT_PORT]->nBufferSize),
                                              NULL, 0, NULL);
                                              
                    if (eError != OMX_ErrorNone ) {
                        G711DEC_DPRINT ("%d :: IssuingDSP OP: Error Occurred\n",__LINE__);
                        eError = OMX_ErrorHardware;
                        goto EXIT;
                    }
                    pComponentPrivate->lcml_nOpBuf++;
                    pComponentPrivate->num_Op_Issued++;
                }
            }
            else if (pComponentPrivate->curState == OMX_StatePause){
                pComponentPrivate->pOutputBufHdrPending[pComponentPrivate->nNumOutputBufPending++] = pBufHeader;
                G711DEC_DPRINT("%d :: pComponentPrivate->nNumOutputBufPending 0x%x\n",__LINE__,(int)pComponentPrivate->nNumOutputBufPending);
            }
        }
    }
    else {
        eError = OMX_ErrorBadParameter;
    }

 EXIT:
    G711DEC_DPRINT("%d : Exiting from  G711DECHandleDataBuf_FromApp \n",__LINE__);
    G711DEC_DPRINT("Returning error %d\n",eError);
    return eError;
}

/*-------------------------------------------------------------------*/
/**
 * G711DECGetBufferDirection () This function is used by the component thread to
 * request a buffer from the application.  Since it was called from 2 places,
 * it made sense to turn this into a small function.
 *
 * @param pData pointer to G711 Decoder Context Structure
 * @param pCur pointer to the buffer to be requested to be filled
 *
 * @retval none
 **/
/*-------------------------------------------------------------------*/

OMX_ERRORTYPE G711DECGetBufferDirection(OMX_BUFFERHEADERTYPE *pBufHeader,
                                        OMX_DIRTYPE *eDir)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    G711DEC_COMPONENT_PRIVATE *pComponentPrivate = pBufHeader->pPlatformPrivate;
    OMX_U32 nBuf = 0;
    OMX_BUFFERHEADERTYPE *pBuf = NULL;
    OMX_S16 flag = 1;
    OMX_U16 i = 0;

    G711DEC_DPRINT ("%d :: Entering G711DECGetBufferDirection Function\n",__LINE__);

    /*Search this buffer in input buffers list */
    nBuf = pComponentPrivate->pInputBufferList->numBuffers;
    
    for(i=0; i<nBuf; i++) {
        pBuf = pComponentPrivate->pInputBufferList->pBufHdr[i];
        if(pBufHeader == pBuf) {
            *eDir = OMX_DirInput;
            G711DEC_DPRINT ("%d :: Buffer %p is INPUT BUFFER\n",__LINE__, pBufHeader);
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
            G711DEC_DPRINT ("%d :: Buffer %p is OUTPUT BUFFER\n",__LINE__, pBufHeader);
            flag = 0;
            goto EXIT;
        }
    }

    if (flag == 1) {
        G711DEC_DPRINT ("%d :: Buffer %p is Not Found in the List\n",__LINE__,pBufHeader);
        eError = OMX_ErrorUndefined;
        goto EXIT;
    }
 EXIT:
    G711DEC_DPRINT ("%d :: Exiting G711DECGetBufferDirection Function\n",__LINE__);
    return eError;
}

/*-------------------------------------------------------------------*/
/**
 * G711DECHandleDataBuf_FromLCML () This function is used by the component thread to
 * request a buffer from the application.  Since it was called from 2 places,
 * it made sense to turn this into a small function.
 *
 * @param pData pointer to G711 Decoder Context Structure
 * @param pCur pointer to the buffer to be requested to be filled
 *
 * @retval none
 **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE G711DECHandleDataBuf_FromLCML(G711DEC_COMPONENT_PRIVATE* pComponentPrivate, 
                                            LCML_G711DEC_BUFHEADERTYPE* msgBuffer)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 checkBeforeFilling = 0;
    OMX_U32 inputBufferSize = 0, frameLength = 0;
    LCML_DSP_INTERFACE *pLcmlHandle = (LCML_DSP_INTERFACE *)
        pComponentPrivate->pLcmlHandle;
    LCML_G711DEC_BUFHEADERTYPE *pLcmlHdr = NULL;
    OMX_U16 i = 0;

    OMX_COMPONENTTYPE* pHandle = (OMX_COMPONENTTYPE*)pComponentPrivate->pHandle;

    G711DEC_DPRINT ("%d :: Entering G711DECHandleDataBuf_FromLCML Function\n",__LINE__);

    if (!(pComponentPrivate->bIsStopping)) {
        if (msgBuffer->eDir == G711DEC_DIRECTION_INPUT) {
            G711DEC_DPRINT("%d: Component Sending Empty Input buffer%p to App\n",
                           __LINE__,msgBuffer->buffer);
            inputBufferSize = (pComponentPrivate->pPortDef[G711DEC_INPUT_PORT]->nBufferSize);

            if ( pComponentPrivate->pInputBufferList->numBuffers == 1 ) {
                checkBeforeFilling = inputBufferSize;
            }
            else {
                checkBeforeFilling = inputBufferSize * (pComponentPrivate->pInputBufferList->numBuffers - 1);
            }

            if  (pComponentPrivate->nHoldLength < checkBeforeFilling) {
                G711DEC_DPRINT("line %d:: Calling EmptyBufferDone\n",__LINE__);
                G711DEC_DPRINT("pComponentPrivate->nHoldLength = %d\n",(int)pComponentPrivate->nHoldLength);
                G711DEC_DPRINT("checkBeforeFilling = %d\n",(int)checkBeforeFilling);
                
                pComponentPrivate->cbInfo.EmptyBufferDone (pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           msgBuffer->buffer);
                                                           
                pComponentPrivate->nEmptyBufferDoneCount++;
                pComponentPrivate->lcml_nIpBuf--;
                pComponentPrivate->app_nBuf++;
            }

            /*
             * We possibly have enough data in iHoldBuffer.
             * If we don't have enough data to be send, then we need to refill when last buffer is not set
             * even though FillThisHwBuffer has already been sent.
             * Send QueueBuffer from iHoldBuffer then reflushed the iHoldBuffer.
             */
            else {
                if ( pComponentPrivate->nHoldLength > 0 ) {

                    frameLength = (pComponentPrivate->pPortDef[G711DEC_INPUT_PORT]->nBufferSize);

                    if ( pComponentPrivate->nHoldLength >= frameLength ) {
                        /* Copy the data from iHoldBuffer to dataPtr */
                        if ((msgBuffer->buffer->pBuffer == NULL) ||
                           (pComponentPrivate->pHoldBuffer == NULL)) {
                           eError = OMX_ErrorBadParameter;
			   goto EXIT;
			}

                        memcpy(msgBuffer->buffer->pBuffer,
                               pComponentPrivate->pHoldBuffer,
                               frameLength);

                        memcpy(pComponentPrivate->pHoldBuffer,
                               pComponentPrivate->pHoldBuffer + frameLength,
                               pComponentPrivate->nHoldLength - frameLength);

                        pComponentPrivate->nHoldLength = pComponentPrivate->nHoldLength - frameLength;
                        eError = G711DECGetCorresponding_LCMLHeader(msgBuffer->buffer->pBuffer, OMX_DirInput, &pLcmlHdr);


                        for (i=0; i < inputBufferSize; i++) {
                            G711DEC_DPRINT("%d::Queueing msgBuffer->buffer->pBuffer[%d] = %x\n",__LINE__,i,msgBuffer->buffer->pBuffer[i]);
                        }
                        G711DEC_SetPending(pComponentPrivate,msgBuffer->buffer,OMX_DirInput);
                        eError = LCML_QueueBuffer(pLcmlHandle->pCodecinterfacehandle,
                                                  EMMCodecInputBuffer,
                                                  (OMX_U8 *)msgBuffer->buffer->pBuffer,
                                                  inputBufferSize,
                                                  frameLength,(OMX_U8 *) pLcmlHdr->pIpParam,
                                                  sizeof(G711DEC_UAlgInBufParamStruct),
                                                  NULL);
                    }
                    else {
                        /*We need to refill more since iHoldBuffer only has partial data.*/

                        G711DEC_DPRINT("line %d:: Calling EmptyBufferDone\n",__LINE__);

                        if (pComponentPrivate->curState != OMX_StatePause) {
                            pComponentPrivate->cbInfo.EmptyBufferDone (
                                                                       pHandle,
                                                                       pHandle->pApplicationPrivate,
                                                                       msgBuffer->buffer);
                            pComponentPrivate->nEmptyBufferDoneCount++;
                        }
                        else {
                            pComponentPrivate->pInputBufHdrPending[pComponentPrivate->nNumInputBufPending++] = msgBuffer->buffer;
                        }

                    }
                }
            }
        }
        else if (msgBuffer->eDir == G711DEC_DIRECTION_OUTPUT) {
            pComponentPrivate->num_Reclaimed_Op_Buff++;

            if (pComponentPrivate->bIsEOFSent) {
                msgBuffer->buffer->nFlags |= OMX_BUFFERFLAG_EOS;
                pComponentPrivate->bIsEOFSent = 0;
            }

            /*Copying tick count information to output buffer*/
            msgBuffer->buffer->nTickCount = (OMX_U32)pComponentPrivate->arrBufIndexTick[pComponentPrivate->OpBufindex];

            /* Copying time stamp information to output buffer */
            msgBuffer->buffer->nTimeStamp = (OMX_TICKS)pComponentPrivate->arrBufIndex[pComponentPrivate->OpBufindex];
            pComponentPrivate->OpBufindex++;
            pComponentPrivate->OpBufindex %= pComponentPrivate->pPortDef[OMX_DirInput]->nBufferCountActual;
           
            G711DEC_DPRINT("Calling FillBufferDone From Line %d, buffer %p\n",__LINE__, msgBuffer->buffer);
            G711DEC_DPRINT("%d :: Output buff %p, TimeStamp %lld\n",__LINE__, msgBuffer->buffer, msgBuffer->buffer->nTimeStamp);
            pComponentPrivate->cbInfo.FillBufferDone ( pHandle,
                                                       pHandle->pApplicationPrivate,
                                                       msgBuffer->buffer);
            pComponentPrivate->lcml_nOpBuf--;
            pComponentPrivate->app_nBuf++;
            pComponentPrivate->nFillBufferDoneCount++;
            pComponentPrivate->nOutStandingFillDones--;
        
            G711DEC_DPRINT("%d:Incrementing app_nBuf = %d\n",__LINE__,(int)pComponentPrivate->app_nBuf);
        }
        else {
            eError = OMX_ErrorBadParameter;
            goto EXIT;
        }
    }
    else {

        if (msgBuffer->eDir == G711DEC_DIRECTION_INPUT) {
            pComponentPrivate->lcml_nIpBuf--;
            G711DEC_DPRINT("line %d:: Calling EmptyBufferDone\n",__LINE__);
            pComponentPrivate->cbInfo.EmptyBufferDone (pHandle,
                                                       pHandle->pApplicationPrivate,
                                                       msgBuffer->buffer);
            pComponentPrivate->app_nBuf++;
            pComponentPrivate->nEmptyBufferDoneCount++;
        }
        else if (msgBuffer->eDir == G711DEC_DIRECTION_OUTPUT) {
            pComponentPrivate->lcml_nOpBuf--;
            pComponentPrivate->num_Reclaimed_Op_Buff++;
            G711DEC_DPRINT("%d: Component Sending Filled Output buffer%p to App\n",__LINE__,msgBuffer->buffer);
            G711DEC_DPRINT ("Sending Filled OUTPUT BUFFER to App = %p\n",msgBuffer->buffer->pBuffer);

            if (pComponentPrivate->bIsEOFSent) {
                msgBuffer->buffer->nFlags |= OMX_BUFFERFLAG_EOS;
                pComponentPrivate->bIsEOFSent = 0;
            }
            G711DEC_DPRINT("Calling FillBufferDone From Line %d\n",__LINE__);

            pComponentPrivate->cbInfo.FillBufferDone (pHandle,
                                                      pHandle->pApplicationPrivate,
                                                      msgBuffer->buffer);
            pComponentPrivate->app_nBuf++;
            pComponentPrivate->nFillBufferDoneCount++;
            pComponentPrivate->nOutStandingFillDones--;


            G711DEC_DPRINT("%d:Incrementing app_nBuf = %d\n",__LINE__,(int)pComponentPrivate->app_nBuf);
        }
    }
 EXIT:
    G711DEC_DPRINT ("%d :: Exiting G711DECHandleDataBuf_FromLCML Function\n",__LINE__);
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
/*    gLcmlPipeWr = pComponentPrivate->lcml_Pipe[1];*/

OMX_ERRORTYPE G711DECLCML_Callback (TUsnCodecEvent event,void * args [10])
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U8 *pBuffer = args[1];
    LCML_G711DEC_BUFHEADERTYPE *pLcmlHdr = NULL;
    OMX_S16 ret = 0;

#ifdef G711DEC_DEBUG
    LCML_DSP_INTERFACE *phandle = (LCML_DSP_INTERFACE *)args[6];
#endif

    G711DEC_DPRINT ("%d :: Entering the G711DECLCML_Callback Function\n",__LINE__);
    G711DEC_DPRINT("args = %d ",(int)args[0]);
    G711DEC_DPRINT("event = %d\n",event);



    switch(event) {

    case EMMCodecDspError:
        G711DEC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecDspError\n");
        break;

    case EMMCodecInternalError:
        G711DEC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecInternalError\n");
        break;

    case EMMCodecInitError:
        G711DEC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecInitError\n");
        break;

    case EMMCodecDspMessageRecieved:
        G711DEC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecDspMessageRecieved\n");
        break;

    case EMMCodecBufferProcessed:
        G711DEC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecBufferProcessed\n");
        break;

    case EMMCodecProcessingStarted:
        G711DEC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecProcessingStarted\n");
        break;

    case EMMCodecProcessingPaused:
        G711DEC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecProcessingPaused\n");
        break;

    case EMMCodecProcessingStoped:
        G711DEC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecProcessingStoped\n");
        break;

    case EMMCodecProcessingEof:
        G711DEC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecProcessingEof\n");
        break;

    case EMMCodecBufferNotProcessed:
        G711DEC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecBufferNotProcessed\n");
        break;

    case EMMCodecAlgCtrlAck:
        G711DEC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecAlgCtrlAck\n");
        break;

    case EMMCodecStrmCtrlAck:
        G711DEC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecStrmCtrlAck\n");
        break;
    }

    if(event == EMMCodecBufferProcessed)
    {
        if( (OMX_U32)args [0] == EMMCodecInputBuffer) {
            G711DEC_DPRINT("%d :: Input: pBufferr = %p\n",__LINE__, pBuffer);

            eError = G711DECGetCorresponding_LCMLHeader(pBuffer, OMX_DirInput, &pLcmlHdr);
            if (eError != OMX_ErrorNone) {
                G711DEC_DPRINT("%d :: Error: Invalid Buffer Came ...\n",__LINE__);
                goto EXIT;
            }
            G711DEC_ClearPending(pComponentPrivate_CC,pLcmlHdr->buffer,OMX_DirInput);
            ret = (OMX_U16) G711DECHandleDataBuf_FromLCML(pComponentPrivate_CC, pLcmlHdr);
            if (ret != OMX_ErrorNone) {
                G711DEC_DPRINT ("%d :: Error in Writing to the Data pipe\n", __LINE__);
                G711DEC_PRINT("Error writting pipe\n");
                eError = OMX_ErrorHardware;
                goto EXIT;
            }
        } 
        else if ((OMX_U32)args [0] == EMMCodecOuputBuffer) {
        
            G711DEC_DPRINT("%d :: Output: pBufferr = %p\n",__LINE__, pBuffer);

            eError = G711DECGetCorresponding_LCMLHeader(pBuffer, OMX_DirOutput, &pLcmlHdr);
            if (eError != OMX_ErrorNone) {
                G711DEC_DPRINT("%d :: Error: Invalid Buffer Came ...\n",__LINE__);
                goto EXIT;
            }
            if (!pComponentPrivate_CC->bStopSent)
                pLcmlHdr->buffer->nFilledLen = (OMX_U32)args[2];
            else
                pLcmlHdr->buffer->nFilledLen = 0;
            G711DEC_DPRINT("G711DECLCML_Callback::: pLcmlHdr->buffer->nFilledLen = %d\n",(int)pLcmlHdr->buffer->nFilledLen);
            pComponentPrivate_CC->lcml_nCntOpReceived++;

            G711DEC_ClearPending(pComponentPrivate_CC,pLcmlHdr->buffer,OMX_DirOutput);
            ret = (OMX_U16) G711DECHandleDataBuf_FromLCML(pComponentPrivate_CC, pLcmlHdr);
            pComponentPrivate_CC->nOutStandingFillDones++;
            
            if (ret != OMX_ErrorNone) {
                G711DEC_DPRINT ("%d :: Error in Writing to the Data pipe\n", __LINE__);
                eError = OMX_ErrorHardware;
                goto EXIT;
            }
        }
    } else if (event == EMMCodecStrmCtrlAck) {
        G711DEC_DPRINT("%d :: GOT MESSAGE USN_DSPACK_STRMCTRL ----\n",__LINE__);
    }
    else if(event == EMMCodecProcessingStoped) {
        G711DEC_DPRINT("pComponentPrivate_CC->bNoIdleOnStop = %d\n",(int)pComponentPrivate_CC->bNoIdleOnStop);
        
        if (!pComponentPrivate_CC->bNoIdleOnStop) {
            pComponentPrivate_CC->curState = OMX_StateIdle;
#ifdef RESOURCE_MANAGER_ENABLED
            eError = RMProxy_NewSendCommand(pComponentPrivate_CC->pHandle,
                                            RMProxy_StateSet, OMX_G711_Decoder_COMPONENT,
                                            OMX_StateIdle, 1234, NULL);
#endif
            pComponentPrivate_CC->cbInfo.EventHandler(pComponentPrivate_CC->pHandle,
                                                      pComponentPrivate_CC->pHandle->pApplicationPrivate,
                                                      OMX_EventCmdComplete,
                                                      OMX_CommandStateSet,
                                                      pComponentPrivate_CC->curState,
                                                      NULL);
        }
        else {
            pComponentPrivate_CC->bDspStoppedWhileExecuting = OMX_TRUE;
            pComponentPrivate_CC->bNoIdleOnStop= OMX_FALSE;
        }
        
    }
    else if (event == EMMCodecProcessingPaused) {
        pComponentPrivate_CC->curState = OMX_StatePause;
        pComponentPrivate_CC->cbInfo.EventHandler( pComponentPrivate_CC->pHandle,
                                                   pComponentPrivate_CC->pHandle->pApplicationPrivate,
                                                   OMX_EventCmdComplete,
                                                   OMX_CommandStateSet,
                                                   pComponentPrivate_CC->curState,
                                                   NULL);
    }


     
    if(event == EMMCodecDspMessageRecieved) {
        G711DEC_DPRINT("%d :: commandedState  = %d\n",__LINE__,(int)args[0]);
        G711DEC_DPRINT("%d :: arg1 = %d\n",__LINE__,(int)args[1]);
        G711DEC_DPRINT("%d :: arg2 = %d\n",__LINE__,(int)args[2]);
    }

 EXIT:
    G711DEC_DPRINT ("%d :: Exiting the G711DECLCML_Callback Function\n",__LINE__);
    return eError;
}
/* -------------------------------------------------------------------*/
/**
 * G711DEC_GetCorrespondingLCMLHeader() function will be called by LCML_Callback
 * component to write the msg
 * @param *pBuffer,          Event which gives to details about USN status
 * @param G711DEC_LCML_BUFHEADERTYPE **ppLcmlHdr
 * @param  OMX_DIRTYPE eDir this gives direction of the buffer
 * @retval OMX_NoError              Success, ready to roll
 *         OMX_Error_BadParameter   The input parameter pointer is null
 **/
/* -------------------------------------------------------------------*/
OMX_ERRORTYPE G711DECGetCorresponding_LCMLHeader(OMX_U8 *pBuffer,
                                                 OMX_DIRTYPE eDir,
                                                 LCML_G711DEC_BUFHEADERTYPE **ppLcmlHdr)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    
    LCML_G711DEC_BUFHEADERTYPE *pLcmlBufHeader = NULL;
    OMX_S16 nIpBuf = (OMX_S16) pComponentPrivate_CC->pInputBufferList->numBuffers;
    OMX_S16 nOpBuf = (OMX_S16) pComponentPrivate_CC->pOutputBufferList->numBuffers;
    OMX_U16 i = 0;

    while (!pComponentPrivate_CC->bInitParamsInitialized) {
        G711DEC_DPRINT("Waiting for init to complete\n");
        
#ifndef UNDER_CE
        sched_yield();
#else
        Sleep(0);
#endif
    }
    
    G711DEC_DPRINT("%d :: Inside G711DECGetCorresponding_LCMLHeader..\n",__LINE__);
    G711DEC_DPRINT("pComponentPrivate_CC = %p\n",pComponentPrivate_CC);
    G711DEC_DPRINT("eDir = %d\n",eDir);
    
    if(eDir == OMX_DirInput) {
        G711DEC_DPRINT("Line %d\n",__LINE__);
        G711DEC_DPRINT("%d :: Inside G711DECGetCorresponding_LCMLHeader..\n",__LINE__);
        G711DEC_DPRINT("pComponentPrivate_CC = %p\n",pComponentPrivate_CC);
        
        pLcmlBufHeader = pComponentPrivate_CC->pLcmlBufHeader[G711DEC_INPUT_PORT];
        G711DEC_DPRINT("%d :: Inside G711DECGetCorresponding_LCMLHeader..\n",__LINE__);
        
        for(i=0; i<nIpBuf; i++) {
            G711DEC_DPRINT("%d :: Inside G711DECGetCorresponding_LCMLHeader..\n",__LINE__);
            G711DEC_DPRINT("pBuffer = %p\n",pBuffer);
            G711DEC_DPRINT("%d :: Inside G711DECGetCorresponding_LCMLHeader..\n",__LINE__);
            G711DEC_DPRINT("pLcmlBufHeader->buffer->pBuffer = %p\n",pLcmlBufHeader->buffer->pBuffer);
            G711DEC_DPRINT("%d :: Inside G711DECGetCorresponding_LCMLHeader..\n",__LINE__);
           
            if(pBuffer == pLcmlBufHeader->buffer->pBuffer) {
                G711DEC_DPRINT("%d :: Inside G711DECGetCorresponding_LCMLHeader..\n",__LINE__);
                *ppLcmlHdr = pLcmlBufHeader;
                G711DEC_DPRINT("%d::Corresponding LCML Header Found\n",__LINE__);
                goto EXIT;
            }
            pLcmlBufHeader++;
        }
    } 
    else if (eDir == OMX_DirOutput) {
        G711DEC_DPRINT("Line %d\n",__LINE__);
        pLcmlBufHeader = pComponentPrivate_CC->pLcmlBufHeader[G711DEC_OUTPUT_PORT];
        G711DEC_DPRINT("Line %d\n",__LINE__);

        for(i=0; i<nOpBuf; i++) {
            G711DEC_DPRINT("Line %d\n",__LINE__);
            if(pBuffer == pLcmlBufHeader->buffer->pBuffer) {

                G711DEC_DPRINT("pBuffer = %p\n",pBuffer);
                G711DEC_DPRINT("pLcmlBufHeader->buffer->pBuffer = %p\n",pLcmlBufHeader->buffer->pBuffer);
                *ppLcmlHdr = pLcmlBufHeader;
                G711DEC_DPRINT("Line %d\n",__LINE__);
                G711DEC_DPRINT("%d::Corresponding LCML Header Found\n",__LINE__);
                G711DEC_DPRINT("Line %d\n",__LINE__);

                goto EXIT;
            }
            pLcmlBufHeader++;
        }
    } else {
        G711DEC_DPRINT("Line %d\n",__LINE__);
        G711DEC_DPRINT("%d:: Invalid Buffer Type :: exiting...\n",__LINE__);
    }
    G711DEC_DPRINT("Line %d\n",__LINE__);

 EXIT:
    return eError;
}


#ifndef UNDER_CE

OMX_HANDLETYPE G711DECGetLCMLHandle()
{
    void *handle = NULL;
    OMX_ERRORTYPE (*fpGetHandle)(OMX_HANDLETYPE);
    OMX_HANDLETYPE pHandle = NULL;
    char *error = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    G711DEC_DPRINT("G711DECGetLCMLHandle %d\n",__LINE__);
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
        G711DEC_DPRINT("eError != OMX_ErrorNone...\n");
        pHandle = NULL;
        goto EXIT;
    }

    pComponentPrivate_CC->bLcmlHandleOpened = 1;

 EXIT:
    G711DEC_DPRINT("G711DECGetLCMLHandle returning %p\n",pHandle);

    return pHandle;
}


#else
/*=======================================================================*/
/**WINDOWS Explicit dll load procedure                                                            */
/**                                                                         */
/*=======================================================================*/
OMX_HANDLETYPE G711DECGetLCMLHandle()
{
    typedef OMX_ERRORTYPE (*LPFNDLLFUNC1)(OMX_HANDLETYPE);
    OMX_HANDLETYPE pHandle = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    HINSTANCE hDLL;               /* Handle to DLL */
    LPFNDLLFUNC1 fpGetHandle1;



    hDLL = LoadLibraryEx(TEXT("OAF_BML.dll"), NULL,0);
    if (hDLL == NULL)
    {
        /* fputs(dlerror(), stderr); */
        G711DEC_DPRINT("BML Load Failed!!!\n");
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
        G711DEC_DPRINT("eError != OMX_ErrorNone...\n");
        pHandle = NULL;
        return pHandle;
    }
    return pHandle;
}


#endif
/*=======================================================================*/
/** FREE HANDLE                                                                          */
/**                                                                         */
/*=======================================================================*/
#ifndef UNDER_CE

OMX_ERRORTYPE G711DECFreeLCMLHandle()
{

    OMX_S16 retValue = 0;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    if (pComponentPrivate_CC->bLcmlHandleOpened) {
        retValue = dlclose(pComponentPrivate_CC->pLcmlHandle);

        if (retValue != 0) {
            eError = OMX_ErrorUndefined;
        }
        pComponentPrivate_CC->bLcmlHandleOpened = 0;
    }

    return eError;
}
#else

/*=======================================================================*/
/** FREE HANDLE                                                                          */
/**                                                                         */
/*=======================================================================*/
OMX_ERRORTYPE G711DECFreeLCMLHandle()
{

    OMX_S16 retValue = 0;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    if (pComponentPrivate_CC->bLcmlHandleOpened) {

        retValue = FreeLibrary(pComponentPrivate_CC->pLcmlHandle);
        if (retValue == 0) {          /* Zero Indicates failure */
            eError = OMX_ErrorUndefined;
        }
        pComponentPrivate_CC->bLcmlHandleOpened = 0;
    }

    return eError;
}

/*=======================================================================*/
/** SET PENDING                                                                          */
/**                                                                         */
/*=======================================================================*/
#endif
void G711DEC_SetPending(G711DEC_COMPONENT_PRIVATE *pComponentPrivate, 
                        OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir)
{
    OMX_U16 i = 0;
    if (eDir == OMX_DirInput) {
        for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pInputBufferList->pBufHdr[i]) {
                pComponentPrivate->pInputBufferList->bBufferPending[i] = 1;
                G711DEC_DPRINT("*******************INPUT BUFFER %d IS PENDING******************************\n",i);
            }
        }
    }
    else {
        for (i=0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pOutputBufferList->pBufHdr[i]) {
                pComponentPrivate->pOutputBufferList->bBufferPending[i] = 1;
                G711DEC_DPRINT("*******************OUTPUT BUFFER %d IS PENDING******************************\n",i);
            }
        }
    }
}
/*=======================================================================*/
/** CLEAR PENDING                                                                            */
/**                                                                         */
/*=======================================================================*/
void G711DEC_ClearPending(G711DEC_COMPONENT_PRIVATE *pComponentPrivate, 
                          OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir)
{
    OMX_U16 i = 0;

    if (eDir == OMX_DirInput) {
        for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pInputBufferList->pBufHdr[i]) {
                pComponentPrivate->pInputBufferList->bBufferPending[i] = 0;
                G711DEC_DPRINT("*******************INPUT BUFFER %d IS RECLAIMED******************************\n",i);
            }
        }
    }
    else {
        for (i=0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pOutputBufferList->pBufHdr[i]) {
                pComponentPrivate->pOutputBufferList->bBufferPending[i] = 0;
                G711DEC_DPRINT("*******************OUTPUT BUFFER %d IS RECLAIMED ******************************\n",i);
            }
        }
    }
}

OMX_U32 G711DEC_IsPending(G711DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir)
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


OMX_U32 G711DEC_IsValid(G711DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U8 *pBuffer, OMX_DIRTYPE eDir)
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

/*=======================================================================*/
/** G711DECFill_LCMLInitParamsEx                                                                    */
/**                                                                         */
/*=======================================================================*/
OMX_ERRORTYPE  G711DECFill_LCMLInitParamsEx (OMX_HANDLETYPE  pComponent )
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nIpBuf = 0,nIpBufSize = 0,nOpBuf = 0,nOpBufSize = 0;
    OMX_U16 i = 0;
    OMX_BUFFERHEADERTYPE *pTemp = NULL;
    OMX_S16 size_lcml = 0;
    LCML_STRMATTR *strmAttr = NULL;

    LCML_DSP_INTERFACE *pHandle = (LCML_DSP_INTERFACE *)pComponent;
    G711DEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    LCML_G711DEC_BUFHEADERTYPE *pTemp_lcml = NULL;

    G711DEC_DPRINT("%d :: G711DECFill_LCMLInitParams\n ",__LINE__);
    G711DEC_DPRINT("%d :: pHandle = %p\n",__LINE__,pHandle);
    G711DEC_DPRINT("%d :: pHandle->pComponentPrivate = %p\n",__LINE__,pHandle->pComponentPrivate);
    pComponentPrivate = pHandle->pComponentPrivate;

    nIpBuf = pComponentPrivate->pInputBufferList->numBuffers;

    nIpBufSize = (pComponentPrivate->pPortDef[G711DEC_INPUT_PORT]->nBufferSize);

    nOpBuf = pComponentPrivate->pOutputBufferList->numBuffers;
    nOpBufSize = (pComponentPrivate->pPortDef[G711DEC_OUTPUT_PORT]->nBufferSize);


    size_lcml = (OMX_U16) (nIpBuf * sizeof(LCML_G711DEC_BUFHEADERTYPE));
    G711D_OMX_MALLOC_SIZE(pTemp_lcml, size_lcml, LCML_G711DEC_BUFHEADERTYPE);
    pComponentPrivate->pLcmlBufHeader[G711DEC_INPUT_PORT] = pTemp_lcml;

    for (i=0; i<nIpBuf; i++) {
        pTemp = pComponentPrivate->pInputBufferList->pBufHdr[i];
        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pTemp->nAllocLen = nIpBufSize;
        pTemp->nFilledLen = nIpBufSize;
        pTemp->nVersion.s.nVersionMajor = G711DEC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = G711DEC_MINOR_VER;
        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = NOT_USED;
        pTemp_lcml->buffer = pTemp;
        pTemp_lcml->eDir = OMX_DirInput;

        G711D_OMX_MALLOC(pTemp_lcml->pIpParam, G711DEC_UAlgInBufParamStruct);
        
        pTemp_lcml->pIpParam->usFrameLost = 0;
        pTemp_lcml->pIpParam->usEndOfFile = 0;
        
        G711D_OMX_MALLOC(pTemp_lcml->pDmmBuf,DMM_BUFFER_OBJ);
        /* This means, it is not a last buffer. This flag is to be modified by the application to indicate the last buffer */
        pTemp->nFlags = NORMAL_BUFFER;
        pTemp_lcml++;
    }

    /* Allocate memory for all output buffer headers. This memory pointer will be sent to LCML */
    size_lcml = (OMX_U16) (nOpBuf * sizeof(LCML_G711DEC_BUFHEADERTYPE) );
    G711D_OMX_MALLOC_SIZE(pTemp_lcml, size_lcml, LCML_G711DEC_BUFHEADERTYPE);
    pComponentPrivate->pLcmlBufHeader[G711DEC_OUTPUT_PORT] = pTemp_lcml;

    for (i=0; i<nOpBuf; i++) {
        pTemp = pComponentPrivate->pOutputBufferList->pBufHdr[i];
        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pTemp->nAllocLen = nOpBufSize;
        pTemp->nFilledLen = nOpBufSize;
        pTemp->nVersion.s.nVersionMajor = G711DEC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = G711DEC_MINOR_VER;
        pComponentPrivate->nVersion = pTemp->nVersion.nVersion;
        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = NOT_USED;
        
        /* This means, it is not a last buffer. This flag is to be modified by the application to indicate the last buffer */
        pTemp_lcml->buffer = pTemp;
        pTemp_lcml->eDir = OMX_DirOutput;
        G711DEC_DPRINT("%d:::pTemp_lcml = %p\n",__LINE__,pTemp_lcml);
        G711DEC_DPRINT("%d:::pTemp_lcml->buffer = %p\n",__LINE__,pTemp_lcml->buffer);

        pTemp->nFlags = NORMAL_BUFFER;

        pTemp++;
        pTemp_lcml++;
    }
    pComponentPrivate->bPortDefsAllocated = 1;
    G711DEC_DPRINT("%d :: Exiting G711DECFill_LCMLInitParams",__LINE__);

    pComponentPrivate->bInitParamsInitialized = 1;
 EXIT:
    if(eError == OMX_ErrorInsufficientResources)
    {
        OMX_G711DECMEMFREE_STRUCT(pComponentPrivate->pLcmlBufHeader[G711DEC_INPUT_PORT]);
        OMX_G711DECMEMFREE_STRUCT(strmAttr);
	if (pTemp_lcml != NULL) {
	    OMX_G711DECMEMFREE_STRUCT(pTemp_lcml->pIpParam);
	}
        OMX_G711DECMEMFREE_STRUCT(pComponentPrivate->pLcmlBufHeader[G711DEC_OUTPUT_PORT]);
    }
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
                         DMM_BUFFER_OBJ* pDmmBuf)
{
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    DSP_STATUS status = DSP_SOK;
    int nSizeReserved = 0;

    G711DEC_PRINT("OMX_DmmMap %d\n",__LINE__);
    if(pDmmBuf == NULL)
    {
        G711DEC_PRINT("pBuf is NULL\n");
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    if(pArmPtr == NULL)
    {
        G711DEC_PRINT("pBuf is NULL\n");
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }
    
    /* Allocate */
    pDmmBuf->pAllocated = pArmPtr;
    /* Reserve */
    nSizeReserved = ROUND_TO_PAGESIZE(size) + 2*DMM_PAGE_SIZE ;
    status = DSPProcessor_ReserveMemory(ProcHandle, nSizeReserved, &(pDmmBuf->pReserved));
    G711DEC_DPRINT("\nOMX Reserve DSP: %p\n",pDmmBuf->pReserved);
                          
    if(DSP_FAILED(status))
    {
        G711DEC_PRINT("DSPProcessor_ReserveMemory() failed - error 0x%x", (int)status);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }
    pDmmBuf->nSize = size;
    G711DEC_DPRINT(" DMM MAP Reserved: %p, size 0x%x (%d)\n", pDmmBuf->pReserved,nSizeReserved,nSizeReserved);
    
    /* Map */
    status = DSPProcessor_Map(ProcHandle,
                              pDmmBuf->pAllocated,/* Arm addres of data to Map on DSP*/
                              size , /* size to Map on DSP*/
                              pDmmBuf->pReserved, /* reserved space */
                              &(pDmmBuf->pMapped), /* returned map pointer */
                              0); /* final param is reserved.  set to zero. */
    
    if(DSP_FAILED(status))
    {
        G711DEC_PRINT("DSPProcessor_Map() failed - error 0x%x", (int)status);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }
    G711DEC_DPRINT("DMM Mapped: %p, size 0x%x (%d)\n",pDmmBuf->pMapped, size,size);

    /* Issue an initial memory flush to ensure cache coherency */
    status = DSPProcessor_FlushMemory(ProcHandle, pDmmBuf->pAllocated, size, 0);
    if(DSP_FAILED(status))
    {
        G711DEC_PRINT("Unable to flush mapped buffer: error 0x%x",(int)status);
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
    G711DEC_PRINT("OMX_DmmUnMap %d\n",__LINE__);
    DSP_STATUS status = DSP_SOK;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    G711DEC_DPRINT("\nOMX UnReserve DSP: %p\n",pResPtr);

    if(pMapPtr == NULL)
    {
        G711DEC_PRINT("pMapPtr is NULL\n");
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if(pResPtr == NULL)
    {
        G711DEC_PRINT("pResPtr is NULL\n");
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }
    status = DSPProcessor_UnMap(ProcHandle,pMapPtr);
    if(DSP_FAILED(status))
    {
        G711DEC_PRINT("DSPProcessor_UnMap() failed - error 0x%x",(int)status);
    }

    G711DEC_DPRINT("unreserving  structure =0x%p\n",pResPtr );
    status = DSPProcessor_UnReserveMemory(ProcHandle,pResPtr);
    if(DSP_FAILED(status))
    {
        G711DEC_PRINT("DSPProcessor_UnReserveMemory() failed - error 0x%x", (int)status);
    }

 EXIT:
    return eError;
}

#ifdef RESOURCE_MANAGER_ENABLED
/***********************************
 *  Callback to the RM                                       *
 ***********************************/
void G711DEC_ResourceManagerCallback(RMPROXY_COMMANDDATATYPE cbData)
{
    OMX_COMMANDTYPE Cmd = OMX_CommandStateSet;
    OMX_STATETYPE state = OMX_StateIdle;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)cbData.hComponent;
    G711DEC_COMPONENT_PRIVATE *pCompPrivate = NULL;

    pCompPrivate = (G711DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

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
