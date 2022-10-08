
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
* @file OMX_G726Enc_Utils.c
*
* This file implements G726 Encoder Component Specific APIs and its functionality
* that is fully compliant with the Khronos OpenMAX (TM) 1.0 Specification
*
* @path  $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\g726_enc\src
*
* @rev  1.0
*/
/* ----------------------------------------------------------------------------
*!
*! Revision History
*! ===================================
*! Gyancarlo Garcia: Initial Verision
*! 05-Oct-2007
*!
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

#ifdef RESOURCE_MANAGER_ENABLED
#include <ResourceManagerProxyAPI.h>
#endif

#include "OMX_G726Enc_Utils.h"
#include "g726enc_sn_uuid.h"
#include <encode_common_ti.h>
#include "usn.h"

#ifdef UNDER_CE
#define HASHINGENABLE 1
#endif

#ifdef G726ENC_DEBUGMEM
extern void *arr[500] = {NULL};
extern int lines[500] = {0};
extern int bytes[500] = {0};
extern char file[500][50] ={""};

void * DebugMalloc(int line, char *s, int size);
int DebugFree(void *dp, int line, char *s);

#define SafeMalloc(x) DebugMalloc(__LINE__,__FILE__,x)
#define SafeFree(z) DebugFree(z,__LINE__,__FILE__)

void * DebugMalloc(int line, char *s, int size)
{
   void *p = NULL;    
   int e=0;
   p = calloc(1,size);
   if(p==NULL){
       printf("__ Memory not available\n");
       exit(1);
       }
   else{
         while((lines[e]!=0)&& (e<500) ){
              e++;
         }
         arr[e]=p;
         lines[e]=line;
         bytes[e]=size;
         strcpy(file[e],s);
         printf("__ Allocating %d bytes on address %p, line %d file %s\n", size, p, line, s);
         return p;
   }
}

int DebugFree(void *dp, int line, char *s){
    int q = 0;
    if(dp==NULL){
                 printf("__ NULL can't be deleted\n");
                 return 0;
    }
    for(q=0;q<500;q++){
        if(arr[q]==dp){
           printf("__ Deleting %d bytes on address %p, line %d file %s\n", bytes[q],dp, line, s);
           lines[q]=0;
           strcpy(file[q],"");           
           free(dp);
           dp = NULL;
           break;
        }            
     }    
     if(500==q)
         printf("\n\n__ Pointer not found. Line:%d    File%s!!\n\n",line, s);
}
#else
#define SafeMalloc(x) calloc(1,x)
#define SafeFree(z) free(z)
#endif

/* ========================================================================== */
/**
* @G726ENC_FillLCMLInitParams () This function is used by the component thread to
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

OMX_ERRORTYPE G726ENC_FillLCMLInitParams(OMX_HANDLETYPE pComponent,
                                  LCML_DSP *plcml_Init, OMX_U16 arr[])
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nIpBuf = 0,nIpBufSize = 0,nOpBuf = 0,nOpBufSize = 0;
    OMX_BUFFERHEADERTYPE *pTemp = NULL;
    LCML_DSP_INTERFACE *pHandle = (LCML_DSP_INTERFACE *)pComponent;
    G726ENC_COMPONENT_PRIVATE *pComponentPrivate = pHandle->pComponentPrivate;
    G726ENC_LCML_BUFHEADERTYPE *pTemp_lcml = NULL;
    OMX_U32 i = 0;
    OMX_U32 size_lcml = 0;
    OMX_U8 *pstrTemp = NULL;
    OMX_U8 *pBufferParamTemp = NULL;
    G726ENC_DPRINT("%d :: Entering G726ENC_FillLCMLInitParams\n",__LINE__);

    nIpBuf = pComponentPrivate->pInputBufferList->numBuffers;
    nIpBufSize = pComponentPrivate->pPortDef[G726ENC_INPUT_PORT]->nBufferSize;
    nOpBuf = pComponentPrivate->pOutputBufferList->numBuffers;
    nOpBufSize = pComponentPrivate->pPortDef[G726ENC_OUTPUT_PORT]->nBufferSize;
    
    pComponentPrivate->nRuntimeInputBuffers = (OMX_U8)nIpBuf;
    pComponentPrivate->nRuntimeOutputBuffers = (OMX_U8) nOpBuf;
    
    G726ENC_DPRINT("%d :: ------ Buffer Details -----------\n",__LINE__);
    G726ENC_DPRINT("%d :: Input  Buffer Count = %ld\n",__LINE__,nIpBuf);
    G726ENC_DPRINT("%d :: Input  Buffer Size = %ld\n",__LINE__,nIpBufSize);
    G726ENC_DPRINT("%d :: Output Buffer Count = %ld\n",__LINE__,nOpBuf);
    G726ENC_DPRINT("%d :: Output Buffer Size = %ld\n",__LINE__,nOpBufSize);
    G726ENC_DPRINT("%d :: ------ Buffer Details ------------\n",__LINE__);
    
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

    plcml_Init->NodeInfo.AllUUIDs[0].uuid = &G726ENCSOCKET_TI_UUID;
    strcpy ((char*)plcml_Init->NodeInfo.AllUUIDs[0].DllName,G726ENC_DLL_NAME);
    plcml_Init->NodeInfo.AllUUIDs[0].eDllType = DLL_NODEOBJECT;

    plcml_Init->NodeInfo.AllUUIDs[1].uuid = &G726ENCSOCKET_TI_UUID;
    strcpy ((char*)plcml_Init->NodeInfo.AllUUIDs[1].DllName,G726ENC_DLL_NAME);
    plcml_Init->NodeInfo.AllUUIDs[1].eDllType = DLL_DEPENDENT;

    /*plcml_Init->NodeInfo.AllUUIDs[2].uuid = &USN_TI_UUID;*/
    plcml_Init->NodeInfo.AllUUIDs[2].uuid = &USN_TI_UUID;
    strcpy ((char*)plcml_Init->NodeInfo.AllUUIDs[2].DllName,G726ENC_USN_DLL_NAME);
    plcml_Init->NodeInfo.AllUUIDs[2].eDllType = DLL_DEPENDENT;

    plcml_Init->DeviceInfo.TypeofDevice = 0;

    if(pComponentPrivate->dasfMode == 1) {
        G726ENC_DPRINT("%d :: Codec is configuring to DASF mode\n",__LINE__);
        pstrTemp = (OMX_U8*)SafeMalloc(sizeof(LCML_STRMATTR) + 256);
        if(pstrTemp == NULL){
                        G726ENC_EPRINT("***********************************\n");
                        G726ENC_EPRINT("%d :: Malloc Failed\n",__LINE__);
                        G726ENC_EPRINT("***********************************\n");
                        eError = OMX_ErrorInsufficientResources;
                        goto EXIT;
        }
        pComponentPrivate->strmAttr = (LCML_STRMATTR*)(pstrTemp + 128);                
        pComponentPrivate->strmAttr->uSegid = G726ENC_DEFAULT_SEGMENT;
        pComponentPrivate->strmAttr->uAlignment = 0;
        pComponentPrivate->strmAttr->uTimeout = G726ENC_SN_TIMEOUT;
        pComponentPrivate->strmAttr->uBufsize = nIpBufSize;
        pComponentPrivate->strmAttr->uNumBufs = G726ENC_NUM_INPUT_BUFFERS_DASF;
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
    plcml_Init->SegID = G726ENC_DEFAULT_SEGMENT;
    plcml_Init->Timeout = G726ENC_SN_TIMEOUT;
    plcml_Init->Alignment = 0;
    plcml_Init->Priority = G726ENC_SN_PRIORITY;
    plcml_Init->ProfileID = 0;

    /* Setting Creat Phase Parameters here */
    arr[0] = G726ENC_STREAM_COUNT;                                           /* Number of communication paths      */
    arr[1] = G726ENC_INPUT_PORT;                                             /* Input Path id                      */

    if(pComponentPrivate->dasfMode == 1) {
        arr[2] = G726ENC_INSTRM;                                             /* Streaming on input                 */
        arr[3] = G726ENC_NUM_INPUT_BUFFERS_DASF;                             /* Audio Devices are connected        */
    }
    else {
            arr[2] = G726ENC_DMM;                                                /* DMM buffers will be used           */                                              if (pComponentPrivate->pInputBufferList->numBuffers) {
            arr[3] = (OMX_U16) pComponentPrivate->pInputBufferList->numBuffers;/* #Bufs exchanged between SN and GPP */
        }
    }

    arr[4] = G726ENC_OUTPUT_PORT;                                            /* Output Path id                     */
    arr[5] = G726ENC_DMM;                                                    /* DMM buffers will be used           */
    if (pComponentPrivate->pOutputBufferList->numBuffers) {
        arr[6] = (OMX_U16) pComponentPrivate->pOutputBufferList->numBuffers;   /* #Bufs exchanged between SN and GPP */
    }
    else {
        arr[6] = (OMX_U16)1;                                                   /* 1 Buf exchanged between SN and GPP */
    }

    arr[7] = pComponentPrivate->G726Params[G726ENC_OUTPUT_PORT]->eG726Mode-1;    /* 0 - 16kbps  Codec Selection */
                                                                                 /* 1 - 24 kbps Codec Selection */
                                                                                 /* 2 - 32 kbps Codec Selection */
                                                                                 /* 3 - 40 kbps Codec Selection */

    arr[8] =  (OMX_U16) pComponentPrivate->rtpMode;                    /* 0 - Linear Packing Type */
                                                                       /* 1 - RTP Packing Type    */ 
                                                                       /*What's the right value??*/
    arr[9] = END_OF_CR_PHASE_ARGS;

    plcml_Init->pCrPhArgs = arr;

    /* Allocate memory for all input buffer headers..
     * This memory pointer will be sent to LCML */
    size_lcml = nIpBuf * sizeof(G726ENC_LCML_BUFHEADERTYPE);
    pTemp_lcml = (G726ENC_LCML_BUFHEADERTYPE *)SafeMalloc(size_lcml);    
    
    G726ENC_MEMPRINT("%d :: ALLOCATING MEMORY = %p\n",__LINE__,pTemp_lcml);
    if(pTemp_lcml == NULL) {
        G726ENC_DPRINT("%d :: Memory Allocation Failed\n",__LINE__);
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    pComponentPrivate->pLcmlBufHeader[G726ENC_INPUT_PORT] = pTemp_lcml;
    for (i=0; i<nIpBuf; i++) {
        G726ENC_DPRINT("%d :: INPUT--------- Inside Ip Loop\n",__LINE__);
        pTemp = pComponentPrivate->pInputBufferList->pBufHdr[i];
        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pTemp->nFilledLen = nIpBufSize;
        pTemp->nVersion.s.nVersionMajor = G726ENC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = G726ENC_MINOR_VER;
        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = G726ENC_NOT_USED;
        pTemp_lcml->buffer = pTemp;
        G726ENC_DPRINT("%d :: pTemp_lcml->buffer->pBuffer = %p \n",__LINE__,pTemp_lcml->buffer->pBuffer);
        pTemp_lcml->eDir = OMX_DirInput;
        
        pBufferParamTemp = (OMX_U8*)SafeMalloc( sizeof(G726ENC_ParamStruct) + 256);
	if (pBufferParamTemp == NULL) {
	    G726ENC_DPRINT("%d :: Memory Allocation Failed\n", __LINE__);
	    eError = OMX_ErrorInsufficientResources;
	    goto EXIT;
	}
        memset(pBufferParamTemp, 0x0, sizeof(G726ENC_ParamStruct) + 256);
        pTemp_lcml->pIpParam =  (G726ENC_ParamStruct*)(pBufferParamTemp + 128);
        
        pTemp_lcml->pIpParam->bLastBuffer = 0;
        /* This means, it is not a last buffer. This flag is to be modified by
         * the application to indicate the last buffer */
        pTemp->nFlags = G726ENC_NORMAL_BUFFER;
        pTemp++;
        pTemp_lcml++;
    }

    /* Allocate memory for all output buffer headers..
     * This memory pointer will be sent to LCML */
    size_lcml = nOpBuf * sizeof(G726ENC_LCML_BUFHEADERTYPE);
    pTemp_lcml = (G726ENC_LCML_BUFHEADERTYPE *)SafeMalloc(size_lcml);
    G726ENC_MEMPRINT("%d :: ALLOCATING MEMORY = %p\n",__LINE__,pTemp_lcml);
    if(pTemp_lcml == NULL) {
        G726ENC_DPRINT("%d :: Memory Allocation Failed\n",__LINE__);
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    pComponentPrivate->pLcmlBufHeader[G726ENC_OUTPUT_PORT] = pTemp_lcml;
    for (i=0; i<nOpBuf; i++) {
        G726ENC_DPRINT("%d :: OUTPUT--------- Inside Op Loop\n",__LINE__);
        pTemp = pComponentPrivate->pOutputBufferList->pBufHdr[i];
        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pTemp->nFilledLen = nOpBufSize;
        pTemp->nVersion.s.nVersionMajor = G726ENC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = G726ENC_MINOR_VER;
        pComponentPrivate->nVersion = pTemp->nVersion.nVersion;
        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = G726ENC_NOT_USED;
        pTemp_lcml->buffer = pTemp;
        G726ENC_DPRINT("%d :: pTemp_lcml->buffer->pBuffer = %p \n",__LINE__,pTemp_lcml->buffer->pBuffer);
        pTemp_lcml->eDir = OMX_DirOutput;

        /* This means, it is not a last buffer. This flag is to be modified by
         * the application to indicate the last buffer */
        pTemp->nFlags = G726ENC_NORMAL_BUFFER;
        pTemp++;
        pTemp_lcml++;
    }

    pComponentPrivate->bPortDefsAllocated = 1;
    pComponentPrivate->bInitParamsInitialized = 1;
EXIT:
    G726ENC_DPRINT("%d :: Exiting G726ENC_FillLCMLInitParams\n",__LINE__);
    G726ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}

/* ========================================================================== */
/**
* @G726ENC_StartComponentThread() This function is called by the component to create
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
OMX_ERRORTYPE G726ENC_StartComponentThread(OMX_HANDLETYPE pComponent)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    G726ENC_COMPONENT_PRIVATE *pComponentPrivate =
                        (G726ENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
#ifdef UNDER_CE
    pthread_attr_t attr;
    memset(&attr, 0, sizeof(attr));
    attr.__inheritsched = PTHREAD_EXPLICIT_SCHED;
    attr.__schedparam.__sched_priority = OMX_AUDIO_ENCODER_THREAD_PRIORITY;
#endif

    G726ENC_DPRINT ("%d :: Enetering  G726ENC_StartComponentThread\n", __LINE__);
    /* Initialize all the variables*/
    pComponentPrivate->bIsStopping = 0;

    /* create the pipe used to send buffers to the thread */
    eError = pipe (pComponentPrivate->cmdDataPipe);
    if (eError) {
       eError = OMX_ErrorInsufficientResources;
       G726ENC_DPRINT("%d :: Error while creating cmdDataPipe\n",__LINE__);
       goto EXIT;
    }
    /* create the pipe used to send buffers to the thread */
    eError = pipe (pComponentPrivate->dataPipe);
    if (eError) {
       eError = OMX_ErrorInsufficientResources;
       G726ENC_DPRINT("%d :: Error while creating dataPipe\n",__LINE__);
       goto EXIT;
    }

    /* create the pipe used to send commands to the thread */
    eError = pipe (pComponentPrivate->cmdPipe);
    if (eError) {
       eError = OMX_ErrorInsufficientResources;
       G726ENC_DPRINT("%d :: Error while creating cmdPipe\n",__LINE__);
       goto EXIT;
    }

    /* Create the Component Thread */
#ifdef UNDER_CE
    eError = pthread_create (&(pComponentPrivate->ComponentThread), &attr,
                                       G726ENC_CompThread, pComponentPrivate);
#else
    eError = pthread_create (&(pComponentPrivate->ComponentThread), NULL,
                                       G726ENC_CompThread, pComponentPrivate);
#endif
    if (eError || !pComponentPrivate->ComponentThread) {
       eError = OMX_ErrorInsufficientResources;
       goto EXIT;
    }

    pComponentPrivate->bCompThreadStarted = 1;
EXIT:
    G726ENC_DPRINT("%d :: Exiting G726ENC_StartComponentThread\n", __LINE__);
    G726ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}

/* ========================================================================== */
/**
* @G726ENC_FreeCompResources() This function is called by the component during
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

OMX_ERRORTYPE G726ENC_FreeCompResources(OMX_HANDLETYPE pComponent)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_ERRORTYPE err = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    G726ENC_COMPONENT_PRIVATE *pComponentPrivate = (G726ENC_COMPONENT_PRIVATE *)
                                                     pHandle->pComponentPrivate;
    G726ENC_DPRINT("%d :: Entering G726ENC_FreeCompResources\n",__LINE__);

    if (pComponentPrivate->bCompThreadStarted) {
        OMX_NBCLOSE_PIPE(pComponentPrivate->dataPipe[0],err);
        OMX_NBCLOSE_PIPE(pComponentPrivate->dataPipe[1],err);
        OMX_NBCLOSE_PIPE(pComponentPrivate->cmdPipe[0],err);
        OMX_NBCLOSE_PIPE(pComponentPrivate->cmdPipe[1],err);
        OMX_NBCLOSE_PIPE(pComponentPrivate->cmdDataPipe[0],err);
        OMX_NBCLOSE_PIPE(pComponentPrivate->cmdDataPipe[1],err);
    }

/*    if (pComponentPrivate->bPortDefsAllocated) {*/
        OMX_NBMEMFREE_STRUCT(pComponentPrivate->pPortDef[G726ENC_INPUT_PORT]);
        OMX_NBMEMFREE_STRUCT(pComponentPrivate->pPortDef[G726ENC_OUTPUT_PORT]);
        OMX_NBMEMFREE_STRUCT(pComponentPrivate->G726Params[G726ENC_INPUT_PORT]);
        OMX_NBMEMFREE_STRUCT(pComponentPrivate->G726Params[G726ENC_OUTPUT_PORT]);

        OMX_NBMEMFREE_STRUCT(pComponentPrivate->pCompPort[G726ENC_INPUT_PORT]->pPortFormat);
        OMX_NBMEMFREE_STRUCT(pComponentPrivate->pCompPort[G726ENC_OUTPUT_PORT]->pPortFormat);
        OMX_NBMEMFREE_STRUCT(pComponentPrivate->pCompPort[G726ENC_INPUT_PORT]);
        OMX_NBMEMFREE_STRUCT(pComponentPrivate->pCompPort[G726ENC_OUTPUT_PORT]);

        OMX_NBMEMFREE_STRUCT(pComponentPrivate->sPortParam);
        OMX_NBMEMFREE_STRUCT(pComponentPrivate->sPriorityMgmt);
        OMX_NBMEMFREE_STRUCT(pComponentPrivate->pInputBufferList);
        OMX_NBMEMFREE_STRUCT(pComponentPrivate->pOutputBufferList);

 /*   }*/
    pComponentPrivate->bPortDefsAllocated = 0;
EXIT:
    G726ENC_DPRINT("%d :: Exiting G726ENC_FreeCompResources()\n",__LINE__);
    G726ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}

/* ========================================================================== */
/**
* @G726ENC_CleanupInitParams() This function is called by the component during
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

OMX_ERRORTYPE G726ENC_CleanupInitParams(OMX_HANDLETYPE pComponent)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 i = 0;
    OMX_U8 *pTemp = NULL;
    G726ENC_LCML_BUFHEADERTYPE *pTemp_lcml = NULL;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    OMX_U8* pBufParmsTemp = NULL;
    OMX_U8* pParamsTemp = NULL;        
    G726ENC_COMPONENT_PRIVATE *pComponentPrivate = (G726ENC_COMPONENT_PRIVATE *)
                                                     pHandle->pComponentPrivate;
    G726ENC_DPRINT("%d :: Entering G726ENC_CleanupInitParams()\n", __LINE__);

    if(pComponentPrivate->dasfMode == 1) {
        pTemp = (OMX_U8*)pComponentPrivate->strmAttr;
        if(pTemp!=NULL)
               pTemp -=128;
        OMX_NBMEMFREE_STRUCT(pTemp);
    }

    pTemp_lcml = pComponentPrivate->pLcmlBufHeader[G726ENC_INPUT_PORT];
         
    for(i=0; i<pComponentPrivate->nRuntimeInputBuffers; i++) {
          pBufParmsTemp = (OMX_U8*)pTemp_lcml->pIpParam;
          pBufParmsTemp-=128;
          SafeFree(pBufParmsTemp);
          pTemp_lcml->pIpParam = NULL;
          pTemp_lcml++;
    }

    pParamsTemp = (OMX_U8*)pComponentPrivate->pParams;
    if (pParamsTemp != NULL)
        pParamsTemp -= 128;
    OMX_NBMEMFREE_STRUCT(pParamsTemp);
    pTemp_lcml = pComponentPrivate->pLcmlBufHeader[G726ENC_OUTPUT_PORT];

    for(i=0; i<pComponentPrivate->nRuntimeOutputBuffers; i++) {
/*        OMX_NBMEMFREE_STRUCT(pTemp_lcml->pOpParam);*/ /* according to the SN guide, the params on the */
                                                        /* output buffer shoul not be needed                 */
        pTemp_lcml++;
    }

    OMX_NBMEMFREE_STRUCT(pComponentPrivate->pLcmlBufHeader[G726ENC_INPUT_PORT]);
    OMX_NBMEMFREE_STRUCT(pComponentPrivate->pLcmlBufHeader[G726ENC_OUTPUT_PORT]);

    G726ENC_DPRINT("%d :: Exiting G726ENC_CleanupInitParams()\n",__LINE__);
    G726ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}

/* ========================================================================== */
/**
* @G726ENC_StopComponentThread() This function is called by the component during
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

OMX_ERRORTYPE G726ENC_StopComponentThread(OMX_HANDLETYPE pComponent)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_ERRORTYPE threadError = OMX_ErrorNone;
    int pthreadError = 0;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    G726ENC_COMPONENT_PRIVATE *pComponentPrivate = (G726ENC_COMPONENT_PRIVATE *)
                                                     pHandle->pComponentPrivate;
    G726ENC_DPRINT("%d :: Entering G726ENC_StopComponentThread\n",__LINE__);
    G726ENC_DPRINT("%d :: About to call pthread_join\n",__LINE__);
    pthreadError = pthread_join (pComponentPrivate->ComponentThread,
                                 (void*)&threadError);
    if (0 != pthreadError) {
        eError = OMX_ErrorHardware;
        G726ENC_DPRINT("%d :: Error closing ComponentThread - pthreadError = %d\n",__LINE__,pthreadError);
        goto EXIT;
    }
    if (OMX_ErrorNone != threadError && OMX_ErrorNone != eError) {
        eError = OMX_ErrorInsufficientResources;
        G726ENC_DPRINT("%d :: Error while closing Component Thread\n",__LINE__);
        goto EXIT;
    }
EXIT:
   G726ENC_DPRINT("%d :: Exiting G726ENC_StopComponentThread\n",__LINE__);
   G726ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
   return eError;
}


/* ========================================================================== */
/**
* @G726ENC_HandleCommand() This function is called by the component when ever it
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

OMX_U32 G726ENC_HandleCommand (G726ENC_COMPONENT_PRIVATE *pComponentPrivate)
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
    OMX_STRING p = "damedesuStr";
    OMX_U32 i = 0;
    OMX_U32 ret = 0;
    OMX_U32 nTimeout = 0;
    G726ENC_LCML_BUFHEADERTYPE *pLcmlHdr = NULL;
    OMX_U8 inputPortFlag=0,outputPortFlag=0;
    OMX_U8* pParamsTemp = NULL;        
       
#ifdef RESOURCE_MANAGER_ENABLED
    OMX_ERRORTYPE rm_error = OMX_ErrorNone;
#endif

    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;
    pLcmlHandle = pComponentPrivate->pLcmlHandle;

    ret = read(pComponentPrivate->cmdPipe[0], &command, sizeof (command));
    if (ret == -1) {
        G726ENC_DPRINT("%d :: Error in Reading from the Data pipe\n", __LINE__);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }
    ret = read(pComponentPrivate->cmdDataPipe[0], &commandData, sizeof (commandData));
    if (ret == -1) {
        G726ENC_DPRINT("%d :: Error in Reading from the Data pipe\n", __LINE__);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }

    if (command == OMX_CommandStateSet) {
        commandedState = (OMX_STATETYPE)commandData;
        switch(commandedState) {
        case OMX_StateIdle:
            G726ENC_DPRINT("%d :: G726ENC_HandleCommand :: OMX_StateIdle \n",__LINE__);
            G726ENC_DPRINT("%d :: pComponentPrivate->curState = %d\n",__LINE__,pComponentPrivate->curState);
            if (pComponentPrivate->curState == commandedState){
                pComponentPrivate->cbInfo.EventHandler ( pHandle,
                                                         pHandle->pApplicationPrivate,
                                                         OMX_EventError,
                                                         OMX_ErrorSameState,
                                                         0,
                                                         NULL);
                G726ENC_DPRINT("%d :: Error: Same State Given by Application\n",__LINE__);
            }
            else if (pComponentPrivate->curState == OMX_StateLoaded || pComponentPrivate->curState == OMX_StateWaitForResources) {
                          
                        if (pComponentPrivate->pPortDef[G726ENC_INPUT_PORT]->bPopulated &&  pComponentPrivate->pPortDef[G726ENC_INPUT_PORT]->bEnabled){
                                        inputPortFlag = 1;
                        }
                        if (pComponentPrivate->pPortDef[G726ENC_OUTPUT_PORT]->bPopulated && pComponentPrivate->pPortDef[G726ENC_OUTPUT_PORT]->bEnabled){
                                        outputPortFlag = 1;
                        }
                        if(( pComponentPrivate->dasfMode && !outputPortFlag) ||
                           (!pComponentPrivate->dasfMode && (!inputPortFlag || !outputPortFlag)))                                   
                        {
                              /* Sleep for a while, so the application thread can allocate buffers */
                              G726ENC_DPRINT("%d :: Sleeping...\n",__LINE__);
                              pComponentPrivate->InLoaded_readytoidle = 1;
#ifndef UNDER_CE
                              pthread_mutex_lock(&pComponentPrivate->InLoaded_mutex);
                              pthread_cond_wait(&pComponentPrivate->InLoaded_threshold, &pComponentPrivate->InLoaded_mutex);
                              pthread_mutex_unlock(&pComponentPrivate->InLoaded_mutex);
#endif
                        }
                                    
                cb.LCML_Callback = (void *) G726ENC_LCMLCallback;

                pLcmlHandle = (OMX_HANDLETYPE) G726ENC_GetLCMLHandle(pComponentPrivate);

                if (pLcmlHandle == NULL) {
                    G726ENC_DPRINT("%d :: LCML Handle is NULL........exiting..\n",__LINE__);
                    goto EXIT;
                }

                /* Got handle of dsp via phandle filling information about DSP Specific things */
                pLcmlDsp = (((LCML_DSP_INTERFACE*)pLcmlHandle)->dspCodec);
                eError = G726ENC_FillLCMLInitParams(pHandle, pLcmlDsp, arr);
                if(eError != OMX_ErrorNone) {
                    G726ENC_DPRINT("%d :: Error from G726ENCFill_LCMLInitParams()\n",__LINE__);
                    goto EXIT;
                }

                pComponentPrivate->pLcmlHandle = (LCML_DSP_INTERFACE *)pLcmlHandle;
                cb.LCML_Callback = (void *) G726ENC_LCMLCallback;
            
#ifndef UNDER_CE
				eError = LCML_InitMMCodecEx(((LCML_DSP_INTERFACE *)pLcmlHandle)->pCodecinterfacehandle,
                                          p,&pLcmlHandle,(void *)p,&cb, (OMX_STRING)pComponentPrivate->sDeviceString);

#else
				eError = LCML_InitMMCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
						                      					    p,&pLcmlHandle, (void *)p, &cb);
#endif                
                
                if(eError != OMX_ErrorNone) {
                    G726ENC_DPRINT("%d :: Error returned from LCML_Init()\n",__LINE__);
                    goto EXIT;
                }
                
#ifdef RESOURCE_MANAGER_ENABLED
                /* Need check the resource with RM */
                pComponentPrivate->rmproxyCallback.RMPROXY_Callback = (void *) G726ENC_ResourceManagerCallback;
                if (pComponentPrivate->curState != OMX_StateWaitForResources) {
                    rm_error = RMProxy_NewSendCommand(pHandle, RMProxy_RequestResource,
                                                      OMX_G726_Encoder_COMPONENT, 
                                                      G726ENC_CPU,
                                                      3456,
                                                      &(pComponentPrivate->rmproxyCallback));
                if(rm_error == OMX_ErrorNone) {
                    /* resource is available */
                    pComponentPrivate->curState = OMX_StateIdle;
                    rm_error = RMProxy_NewSendCommand(pHandle,
                                                      RMProxy_StateSet,
                                                      OMX_G726_Encoder_COMPONENT,
                                                      OMX_StateIdle, 3456,NULL);
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
                    G726ENC_DPRINT("%d :: Comp: OMX_ErrorInsufficientResources\n", __LINE__);
                }
              }
				else {
                    rm_error = RMProxy_NewSendCommand(pHandle, RMProxy_StateSet, OMX_G726_Encoder_COMPONENT, OMX_StateIdle, 3456,NULL);
				
              }
                pComponentPrivate->curState = OMX_StateIdle;
                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,
                                                        OMX_CommandStateSet,
                                                        pComponentPrivate->curState,
                                                        NULL);    				
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
                G726ENC_DPRINT("%d :: Setting Component to OMX_StateIdle\n",__LINE__);

#ifdef HASHINGENABLE
		/*Hashing Change*/
		pLcmlHandle = (LCML_DSP_INTERFACE*)pComponentPrivate->pLcmlHandle;
		eError = LCML_FlushHashes(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle);
		if (eError != OMX_ErrorNone) {
			G726ENC_DPRINT("Error occurred in Codec mapping flush!\n");
			break;
		}
#endif
                G726ENC_DPRINT("%d :: G726ENC: About to Call MMCodecControlStop\n", __LINE__);
                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                                    MMCodecControlStop,(void *)p);
                if(eError != OMX_ErrorNone) {
                    G726ENC_DPRINT("%d :: Error from LCML_ControlCodec MMCodecControlStop..\n",__LINE__);
                    goto EXIT;
                }
                
                if(pComponentPrivate->dasfMode == 1) {
                      pParamsTemp = (OMX_U8*)pComponentPrivate->pParams;
                      if (pParamsTemp != NULL)
                            pParamsTemp -= 128;
                      OMX_NBMEMFREE_STRUCT(pParamsTemp);
                }

                if(pComponentPrivate->ptempBuffer!=NULL){
                      OMX_NBMEMFREE_STRUCT(pComponentPrivate->ptempBuffer);
                }

                if(pComponentPrivate->pHoldBuffer!=NULL){
                      OMX_NBMEMFREE_STRUCT(pComponentPrivate->pHoldBuffer);
                }

                pComponentPrivate->nHoldLength = 0;
                pComponentPrivate->lastOutBufArrived=NULL;
                pComponentPrivate->LastBufSent = 0;

            }
            else if(pComponentPrivate->curState == OMX_StatePause) {

#ifdef HASHINGENABLE
		/*Hashing Change*/
		pLcmlHandle = (LCML_DSP_INTERFACE*)pComponentPrivate->pLcmlHandle;
		/* clear out any mappings that might have accumulated */
		eError = LCML_FlushHashes(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle);
		if (eError != OMX_ErrorNone) {
			G726ENC_DPRINT("Error occurred in Codec mapping flush!\n");
			break;
		}
#endif				
		
                pComponentPrivate->curState = OMX_StateIdle;

#ifdef RESOURCE_MANAGER_ENABLED
                rm_error = RMProxy_NewSendCommand(pHandle, RMProxy_StateSet, OMX_G726_Encoder_COMPONENT, OMX_StateIdle, 3456,NULL);
#endif
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
                                                        0,
                                                        NULL);
                G726ENC_DPRINT("%d :: Comp: OMX_ErrorIncorrectStateTransition\n",__LINE__);
            }
            break;

        case OMX_StateExecuting:
            G726ENC_DPRINT("%d :: G726ENC_HandleCommand :: OMX_StateExecuting \n",__LINE__);
            if (pComponentPrivate->curState == commandedState){
                pComponentPrivate->cbInfo.EventHandler ( pHandle,
                                                         pHandle->pApplicationPrivate,
                                                         OMX_EventError,
                                                         OMX_ErrorSameState,
                                                         0,
                                                         NULL);
                G726ENC_DPRINT("%d :: Comp: OMX_ErrorSameState Given by Comp\n",__LINE__);
            }
            else if (pComponentPrivate->curState == OMX_StateIdle) 
                {                                  

                if(pComponentPrivate->dasfMode == 1) {
                    G726ENC_DPRINT("%d :: ---- Comp: DASF Functionality is ON ---\n",__LINE__);
                  	pParamsTemp = (OMX_U8*)SafeMalloc(sizeof(G726ENC_AudioCodecParams) + 256);
                	if(pParamsTemp == NULL){
                        G726ENC_EPRINT("***********************************\n");
                        G726ENC_EPRINT("%d :: Malloc Failed\n",__LINE__);
                        G726ENC_EPRINT("***********************************\n");
                        eError = OMX_ErrorInsufficientResources;
                        goto EXIT;
                    }

                    pComponentPrivate->pParams = (G726ENC_AudioCodecParams*)(pParamsTemp + 128);
                    pComponentPrivate->pParams->iAudioFormat = 1;
                    pComponentPrivate->pParams->iStrmId = pComponentPrivate->streamID;
                    pComponentPrivate->pParams->iSamplingRate = G726ENC_SAMPLING_FREQUENCY;
                    pValues[0] = USN_STRMCMD_SETCODECPARAMS;
                    pValues[1] = (OMX_U32)pComponentPrivate->pParams;
                    pValues[2] = sizeof(G726ENC_AudioCodecParams);
                    /* Sending STRMCTRL MESSAGE to DSP via LCML_ControlCodec*/
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                              EMMCodecControlStrmCtrl,(void *)pValues);
                    if(eError != OMX_ErrorNone) {
                       G726ENC_DPRINT("%d :: Error from LCML_ControlCodec EMMCodecControlStrmCtrl = %x\n",__LINE__,eError);
                       goto EXIT;
                    }
                }

                /* Sending START MESSAGE to DSP via LCML_ControlCodec*/
                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                EMMCodecControlStart, (void *)p);
                if(eError != OMX_ErrorNone) {
                    G726ENC_DPRINT("%d :: Error from LCML_ControlCodec EMMCodecControlStart = %x\n",__LINE__,eError);
                    goto EXIT;
                }

            } else if (pComponentPrivate->curState == OMX_StatePause) {
                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                EMMCodecControlStart, (void *)p);
                if (eError != OMX_ErrorNone) {
                    G726ENC_DPRINT("%d :: Error While Resuming the codec = %x\n",__LINE__,eError);
                    goto EXIT;
                }

                for (i=0; i < pComponentPrivate->nNumInputBufPending; i++) {
                    if (pComponentPrivate->pInputBufHdrPending[i]) {
                        G726ENC_GetCorrespondingLCMLHeader(pComponentPrivate, pComponentPrivate->pInputBufHdrPending[i]->pBuffer, OMX_DirInput, &pLcmlHdr);
                        G726ENC_SetPending(pComponentPrivate,pComponentPrivate->pInputBufHdrPending[i],OMX_DirInput,__LINE__);

                        eError = LCML_QueueBuffer(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                    EMMCodecInputBuffer,
                                                    pComponentPrivate->pInputBufHdrPending[i]->pBuffer,
                                                    pComponentPrivate->pInputBufHdrPending[i]->nAllocLen,
                                                    pComponentPrivate->pInputBufHdrPending[i]->nFilledLen,
                                                    (OMX_U8 *) pLcmlHdr->pIpParam,
                                                    sizeof(G726ENC_ParamStruct),
                                                    NULL);
                    }
                }
                pComponentPrivate->nNumInputBufPending = 0;


                for (i=0; i < pComponentPrivate->nNumOutputBufPending; i++) {
                    if (pComponentPrivate->pOutputBufHdrPending[i]) {
                        G726ENC_GetCorrespondingLCMLHeader(pComponentPrivate, pComponentPrivate->pOutputBufHdrPending[i]->pBuffer, OMX_DirOutput, &pLcmlHdr);
                        G726ENC_SetPending(pComponentPrivate,pComponentPrivate->pOutputBufHdrPending[i],OMX_DirOutput,__LINE__);
                        eError = LCML_QueueBuffer(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                    EMMCodecOuputBuffer,
                                                    pComponentPrivate->pOutputBufHdrPending[i]->pBuffer,
                                                    pComponentPrivate->pOutputBufHdrPending[i]->nAllocLen,
                                                    /*pComponentPrivate->pOutputBufHdrPending[i]->nFilledLen*/0,
                                                    /*(OMX_U8 *) pLcmlHdr->pIpParam*/NULL,
                                                    /*sizeof(G726ENC_ParamStruct)*/0,
                                                    NULL);
                    }
                }
                pComponentPrivate->nNumOutputBufPending = 0;
            } else {
                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventError,
                                                        OMX_ErrorIncorrectStateTransition,
                                                        0,
                                                        NULL);
                G726ENC_DPRINT("%d :: Comp: OMX_ErrorIncorrectStateTransition Given by Comp\n",__LINE__);
                goto EXIT;

            }
            pComponentPrivate->curState = OMX_StateExecuting;
#ifdef RESOURCE_MANAGER_ENABLED
            rm_error = RMProxy_NewSendCommand(pHandle, RMProxy_StateSet, OMX_G726_Encoder_COMPONENT, OMX_StateExecuting, 3456,NULL);
#endif
            pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                    pHandle->pApplicationPrivate,
                                                    OMX_EventCmdComplete,
                                                    OMX_CommandStateSet,
                                                    pComponentPrivate->curState,
                                                    NULL);
            G726ENC_DPRINT("%d :: Comp: OMX_CommandStateSet Given by Comp\n",__LINE__);
            break;

        case OMX_StateLoaded:
            G726ENC_DPRINT("%d :: G726ENC_HandleCommand :: OMX_StateLoaded\n",__LINE__);
            if (pComponentPrivate->curState == commandedState){
                pComponentPrivate->cbInfo.EventHandler ( pHandle,
                                                         pHandle->pApplicationPrivate,
                                                         OMX_EventError,
                                                         OMX_ErrorSameState,
                                                         0,
                                                         NULL);
                G726ENC_DPRINT("%d :: Comp: OMX_ErrorSameState Given by Comp\n",__LINE__);
                break;
             }
            if (pComponentPrivate->curState == OMX_StateWaitForResources){
                G726ENC_DPRINT("%d :: G726ENC_HandleCommand :: OMX_StateWaitForResources\n",__LINE__);
                pComponentPrivate->curState = OMX_StateLoaded;
                pComponentPrivate->cbInfo.EventHandler ( pHandle,
                                                         pHandle->pApplicationPrivate,
                                                         OMX_EventCmdComplete,
                                                         OMX_CommandStateSet,
                                                         pComponentPrivate->curState,
                                                         NULL);
                G726ENC_DPRINT("%d :: Comp: OMX_CommandStateSet Given by Comp\n",__LINE__);
                break;
            }
            if (pComponentPrivate->curState != OMX_StateIdle &&
                pComponentPrivate->curState != OMX_StateWaitForResources) {
                G726ENC_DPRINT("%d :: G726ENC_HandleCommand :: OMX_StateIdle && OMX_StateWaitForResources\n",__LINE__);
                pComponentPrivate->cbInfo.EventHandler ( pHandle,
                                                         pHandle->pApplicationPrivate,
                                                         OMX_EventError,
                                                         OMX_ErrorIncorrectStateTransition,
                                                         0,
                                                         NULL);
                G726ENC_DPRINT("%d :: Error: OMX_ErrorIncorrectStateTransition Given by Comp\n",__LINE__);
                goto EXIT;
            }
            if (pComponentPrivate->pInputBufferList->numBuffers &&
                    pComponentPrivate->pOutputBufferList->numBuffers) { 
                         pComponentPrivate->InIdle_goingtoloaded = 1;
                         pthread_mutex_lock(&pComponentPrivate->InIdle_mutex);
                         pthread_cond_wait(&pComponentPrivate->InIdle_threshold, &pComponentPrivate->InIdle_mutex);
                         pthread_mutex_unlock(&pComponentPrivate->InIdle_mutex);
                                     
            }

            /* Now Deinitialize the component No error should be returned from
            * this function. It should clean the system as much as possible */
            G726ENC_CleanupInitParams(pHandle);
            
            eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                        EMMCodecControlDestroy, (void *)p);
            if (eError != OMX_ErrorNone) {
                G726ENC_DPRINT("%d :: Error: LCML_ControlCodec EMMCodecControlDestroy = %x\n",__LINE__, eError);
                goto EXIT;
            }
            eError = G726ENC_EXIT_COMPONENT_THRD;
            pComponentPrivate->bInitParamsInitialized = 0;
            break;

        case OMX_StatePause:
            G726ENC_DPRINT("%d :: G726ENC_HandleCommand :: OMX_StatePause\n",__LINE__);
            if (pComponentPrivate->curState == commandedState){
                pComponentPrivate->cbInfo.EventHandler ( pHandle,
                                                         pHandle->pApplicationPrivate,
                                                         OMX_EventError,
                                                         OMX_ErrorSameState,
                                                         0,
                                                         NULL);
                G726ENC_DPRINT("%d :: Error: OMX_ErrorSameState Given by Comp\n",__LINE__);
                break;
            }
            if (pComponentPrivate->curState != OMX_StateExecuting &&
                pComponentPrivate->curState != OMX_StateIdle) {
                pComponentPrivate->cbInfo.EventHandler ( pHandle,
                                                         pHandle->pApplicationPrivate,
                                                         OMX_EventError,
                                                         OMX_ErrorIncorrectStateTransition,
                                                         0,
                                                         NULL);
                G726ENC_DPRINT("%d :: Error: OMX_ErrorIncorrectStateTransition Given by Comp\n",__LINE__);
                goto EXIT;
            }
            eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                        EMMCodecControlPause, (void *)p);
            if (eError != OMX_ErrorNone) {
                G726ENC_DPRINT("%d :: Error: LCML_ControlCodec EMMCodecControlPause = %x\n",__LINE__,eError);
                goto EXIT;
            }
            pComponentPrivate->curState = OMX_StatePause;
            pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                    pHandle->pApplicationPrivate,
                                                    OMX_EventCmdComplete,
                                                    OMX_CommandStateSet,
                                                    pComponentPrivate->curState,
                                                    NULL);
            G726ENC_DPRINT("%d :: Comp: OMX_CommandStateSet Given by Comp\n",__LINE__);
            break;

        case OMX_StateWaitForResources:
            if (pComponentPrivate->curState == commandedState) {
                pComponentPrivate->cbInfo.EventHandler ( pHandle,
                                                         pHandle->pApplicationPrivate,
                                                         OMX_EventError,
                                                         OMX_ErrorSameState,
                                                         0,
                                                         NULL);
                G726ENC_DPRINT("%d :: Error: OMX_ErrorSameState Given by Comp\n",__LINE__);
            } else if (pComponentPrivate->curState == OMX_StateLoaded) {
                pComponentPrivate->curState = OMX_StateWaitForResources;
                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,
                                                        OMX_CommandStateSet,
                                                        pComponentPrivate->curState,
                                                        NULL);
                G726ENC_DPRINT("%d :: Comp: OMX_CommandStateSet Given by Comp\n",__LINE__);
            } else {
                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventError,
                                                        OMX_ErrorIncorrectStateTransition,
                                                        0,
                                                        NULL);
                G726ENC_DPRINT("%d :: Error: OMX_ErrorIncorrectStateTransition Given by Comp\n",__LINE__);
            }
            break;

        case OMX_StateInvalid:
            G726ENC_DPRINT("%d :: G726ENC_HandleCommand :: OMX_StateInvalid\n",__LINE__);
            if (pComponentPrivate->curState == commandedState){
                pComponentPrivate->cbInfo.EventHandler ( pHandle,
                                                         pHandle->pApplicationPrivate,
                                                         OMX_EventError,
                                                         OMX_ErrorSameState,
                                                         0,
                                                         NULL);
                G726ENC_DPRINT("%d :: Error: OMX_ErrorSameState Given by Comp\n",__LINE__);
            } else{
                pComponentPrivate->curState = OMX_StateInvalid;
                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventError,
                                                        OMX_ErrorInvalidState,
                                                        0,
                                                        NULL);
                G726ENC_DPRINT("%d :: Comp: OMX_ErrorInvalidState Given by Comp\n",__LINE__);
            }
            break;

        case OMX_StateMax:
            G726ENC_DPRINT("%d :: G726ENC_HandleCommand :: Cmd OMX_StateMax\n",__LINE__);
            break;
        } /* End of Switch */
        } else if (command == OMX_CommandMarkBuffer) {
        G726ENC_DPRINT("%d :: command OMX_CommandMarkBuffer received\n",__LINE__);
        if(!pComponentPrivate->pMarkBuf){
            /* TODO Need to handle multiple marks */
            pComponentPrivate->pMarkBuf = (OMX_MARKTYPE *)(commandData);
        }
    } else if (command == OMX_CommandPortDisable) 
        {
        if (!pComponentPrivate->bDisableCommandPending) {
            if(commandData == 0x0 || commandData == -1){
                /* disable port */
                pComponentPrivate->pPortDef[G726ENC_INPUT_PORT]->bEnabled = OMX_FALSE;
                G726ENC_DPRINT("%d :: command disabled input port\n",__LINE__);
            }
            if(commandData == 0x1 || commandData == -1){
                 /* disable output port */
                pComponentPrivate->pPortDef[G726ENC_OUTPUT_PORT]->bEnabled = OMX_FALSE;
                G726ENC_DPRINT("%d :: command disabled output port\n",__LINE__);
                if (pComponentPrivate->curState == OMX_StateExecuting) {
                    pComponentPrivate->bNoIdleOnStop = OMX_TRUE;
                    eError = LCML_ControlCodec(
                                      ((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                      MMCodecControlStop,(void *)p);
                }
            }
        }
        G726ENC_DPRINT("commandData = %ld\n",commandData);
        G726ENC_DPRINT("pComponentPrivate->pPortDef[INPUT_PORT]->bPopulated = %d\n",pComponentPrivate->pPortDef[G726ENC_INPUT_PORT]->bPopulated);
        G726ENC_DPRINT("pComponentPrivate->pPortDef[OUTPUT_PORT]->bPopulated = %d\n",pComponentPrivate->pPortDef[G726ENC_OUTPUT_PORT]->bPopulated);
        if(commandData == 0x0) {
            if(!pComponentPrivate->pPortDef[G726ENC_INPUT_PORT]->bPopulated){
                /* return cmdcomplete event if input unpopulated */
                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,
                                                        OMX_CommandPortDisable,
                                                        G726ENC_INPUT_PORT,
                                                        NULL);
                pComponentPrivate->bDisableCommandPending = 0;
            } else {
                pComponentPrivate->bDisableCommandPending = 1;
                pComponentPrivate->bDisableCommandParam = commandData;
            }
        }
        if(commandData == 0x1) {
            if (!pComponentPrivate->pPortDef[G726ENC_OUTPUT_PORT]->bPopulated){
                 /* return cmdcomplete event if output unpopulated */
                 pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                         pHandle->pApplicationPrivate,
                                                         OMX_EventCmdComplete,
                                                         OMX_CommandPortDisable,
                                                         G726ENC_OUTPUT_PORT,
                                                         NULL);
                 pComponentPrivate->bDisableCommandPending = 0;
            } else {
                pComponentPrivate->bDisableCommandPending = 1;
                pComponentPrivate->bDisableCommandParam = commandData;
            }
        }
        if(commandData == -1) {
            if (!pComponentPrivate->pPortDef[G726ENC_INPUT_PORT]->bPopulated &&
                !pComponentPrivate->pPortDef[G726ENC_OUTPUT_PORT]->bPopulated){
                 /* return cmdcomplete event if inout & output unpopulated */
                 pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                         pHandle->pApplicationPrivate,
                                                         OMX_EventCmdComplete,
                                                         OMX_CommandPortDisable,
                                                         G726ENC_INPUT_PORT,
                                                         NULL);
                 pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                         pHandle->pApplicationPrivate,
                                                         OMX_EventCmdComplete,
                                                         OMX_CommandPortDisable,
                                                         G726ENC_OUTPUT_PORT,
                                                         NULL);
                 pComponentPrivate->bDisableCommandPending = 0;
            } else {
                pComponentPrivate->bDisableCommandPending = 1;
                pComponentPrivate->bDisableCommandParam = commandData;
            }
            sched_yield();           
        }
    } else if (command == OMX_CommandPortEnable) {
        if(commandData == 0x0 || commandData == -1){
            /* enable in port */
            G726ENC_DPRINT("%d :: setting input port to enabled\n",__LINE__);
            pComponentPrivate->pPortDef[G726ENC_INPUT_PORT]->bEnabled = OMX_TRUE;
            if(pComponentPrivate->AlloBuf_waitingsignal)
            {
                 pComponentPrivate->AlloBuf_waitingsignal = 0;
#ifndef UNDER_CE
                 pthread_mutex_lock(&pComponentPrivate->AlloBuf_mutex);
                 pthread_cond_signal(&pComponentPrivate->AlloBuf_threshold);
                 pthread_mutex_unlock(&pComponentPrivate->AlloBuf_mutex);
#endif
            }
            G726ENC_DPRINT("pComponentPrivate->pPortDef[G726ENC_INPUT_PORT]->bEnabled = %d\n",pComponentPrivate->pPortDef[G726ENC_INPUT_PORT]->bEnabled);
        }
        if(commandData == 0x1 || commandData == -1){
            /* enable out port */
            if(pComponentPrivate->AlloBuf_waitingsignal)
            {
                 pComponentPrivate->AlloBuf_waitingsignal = 0;
#ifndef UNDER_CE
                 pthread_mutex_lock(&pComponentPrivate->AlloBuf_mutex);
                 pthread_cond_signal(&pComponentPrivate->AlloBuf_threshold);
                 pthread_mutex_unlock(&pComponentPrivate->AlloBuf_mutex);
#endif
            }
            if (pComponentPrivate->curState == OMX_StateExecuting) {
                pComponentPrivate->bDspStoppedWhileExecuting = OMX_FALSE;
                eError = LCML_ControlCodec(
                                      ((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                      EMMCodecControlStart,(void *)p);
            }
            G726ENC_DPRINT("%d :: setting output port to enabled\n",__LINE__);
            pComponentPrivate->pPortDef[G726ENC_OUTPUT_PORT]->bEnabled = OMX_TRUE;
            G726ENC_DPRINT("pComponentPrivate->pPortDef[G726ENC_OUTPUT_PORT]->bEnabled = %d\n",pComponentPrivate->pPortDef[G726ENC_OUTPUT_PORT]->bEnabled);
        }

        while (1) {
            G726ENC_DPRINT("pComponentPrivate->curState = %d\n",pComponentPrivate->curState);
            G726ENC_DPRINT("pComponentPrivate->pPortDef[G726ENC_INPUT_PORT]->bPopulated = %d\n",pComponentPrivate->pPortDef[G726ENC_INPUT_PORT]->bPopulated);
            if(commandData == 0x0 && (pComponentPrivate->curState == OMX_StateLoaded ||
                pComponentPrivate->pPortDef[G726ENC_INPUT_PORT]->bPopulated)){
                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,
                                                        OMX_CommandPortEnable,
                                                        G726ENC_INPUT_PORT,
                                                        NULL);
                G726ENC_DPRINT("%d :: setting Input port to enabled\n",__LINE__);
                break;
            } else if(commandData == 0x1 && (pComponentPrivate->curState == OMX_StateLoaded ||
              pComponentPrivate->pPortDef[G726ENC_OUTPUT_PORT]->bPopulated)){
                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,
                                                        OMX_CommandPortEnable,
                                                        G726ENC_OUTPUT_PORT,
                                                        NULL);
                G726ENC_DPRINT("%d :: setting output port to enabled\n",__LINE__);
                break;
            } else if(commandData == -1 && (pComponentPrivate->curState == OMX_StateLoaded ||
              (pComponentPrivate->pPortDef[G726ENC_INPUT_PORT]->bPopulated
              && pComponentPrivate->pPortDef[G726ENC_OUTPUT_PORT]->bPopulated))){
                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,
                                                        OMX_CommandPortEnable,
                                                        G726ENC_INPUT_PORT,
                                                        NULL);
                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,
                                                        OMX_CommandPortEnable,
                                                        G726ENC_OUTPUT_PORT,
                                                        NULL);
                G726ENC_FillLCMLInitParamsEx(pComponentPrivate->pHandle);
                G726ENC_DPRINT("%d :: setting Input & Output port to enabled\n",__LINE__);
                break;
            }
            if(nTimeout++ > G726ENC_OMX_MAX_TIMEOUTS){
                /* return error as we have waited long enough */
                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventError,
                                                        OMX_ErrorPortUnresponsiveDuringAllocation,
                                                        0 ,
                                                        NULL);
                break;
            }
            sched_yield();
        }
    } else if (command == OMX_CommandFlush) {
		OMX_U32 aParam[3] = {0};
        if(commandData == 0x0 || commandData == -1) {
		    aParam[0] = USN_STRMCMD_FLUSH; 
		    aParam[1] = 0x0; 
		    aParam[2] = 0x0; 

		    G726ENC_DPRINT("Flushing input port\n");
		    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                        EMMCodecControlStrmCtrl, (void*)aParam);
            if (eError != OMX_ErrorNone) {
                 goto EXIT;
            }
        }
		if(commandData == 0x1 || commandData == -1){

            aParam[0] = USN_STRMCMD_FLUSH; 
            aParam[1] = 0x1; 
            aParam[2] = 0x0; 

            G726ENC_DPRINT("Flushing output port\n");
            eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
										EMMCodecControlStrmCtrl, (void*)aParam);
            if (eError != OMX_ErrorNone) {
                 goto EXIT;
            }

		}
    }

EXIT:
    G726ENC_DPRINT("%d :: Exiting G726ENC_HandleCommand Function\n",__LINE__);
    G726ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}

/* ========================================================================== */
/**
* @G726ENC_HandleDataBufFromApp() This function is called by the component when ever it
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
OMX_ERRORTYPE G726ENC_HandleDataBufFromApp(OMX_BUFFERHEADERTYPE* pBufHeader,
                                    G726ENC_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_DIRTYPE eDir;
    G726ENC_LCML_BUFHEADERTYPE *pLcmlHdr=NULL;
    LCML_DSP_INTERFACE *pLcmlHandle = (LCML_DSP_INTERFACE *) pComponentPrivate->pLcmlHandle;

    G726ENC_DPRINT ("%d :: Entering G726ENC_HandleDataBufFromApp Function\n",__LINE__);
    /*Find the direction of the received buffer from buffer list */
    eError = G726ENC_GetBufferDirection(pBufHeader, &eDir);
    if (eError != OMX_ErrorNone) {
        G726ENC_DPRINT ("%d :: The pBufHeader is not found in the list\n", __LINE__);
        goto EXIT;
    }

    if (eDir == OMX_DirInput) {
        if(pComponentPrivate->dasfMode == 0) {
            if ((pBufHeader->nFilledLen > 0) || (pBufHeader->nFlags == OMX_BUFFERFLAG_EOS)) {

                eError = G726ENC_GetCorrespondingLCMLHeader(pComponentPrivate, pBufHeader->pBuffer, OMX_DirInput, &pLcmlHdr);
                if (eError != OMX_ErrorNone) {
                    G726ENC_DPRINT("%d :: Error: Invalid Buffer Came ...\n",__LINE__);
                    goto EXIT;
                }
                               
                if(pBufHeader->nFlags == OMX_BUFFERFLAG_EOS) {
                    pComponentPrivate->LastBufSent = 1; 
                    pLcmlHdr->pIpParam->bLastBuffer = 1;
                }
                else {
                    pLcmlHdr->pIpParam->bLastBuffer = 0;
                }                

                /* Store time stamp information */
                pComponentPrivate->arrTimestamp[pComponentPrivate->IpBufindex] = pBufHeader->nTimeStamp;
                /* Store nTickCount information */
                pComponentPrivate->arrTickCount[pComponentPrivate->IpBufindex] = pBufHeader->nTickCount;
                pComponentPrivate->IpBufindex++;
                pComponentPrivate->IpBufindex %= pComponentPrivate->pPortDef[G726ENC_OUTPUT_PORT]->nBufferCountActual;
                            
                if (pComponentPrivate->curState == OMX_StateExecuting) {
                    if(!pComponentPrivate->bDspStoppedWhileExecuting) {
                        if (!G726ENC_IsPending(pComponentPrivate,pBufHeader,OMX_DirInput)) {
                            G726ENC_SetPending(pComponentPrivate,pBufHeader,OMX_DirInput,__LINE__);


                            eError = LCML_QueueBuffer( pLcmlHandle->pCodecinterfacehandle,
                                                       EMMCodecInputBuffer,
                                                       (OMX_U8 *)pBufHeader->pBuffer,
                                                       pBufHeader->nAllocLen,
                                                       pBufHeader->nFilledLen,
                                                       (OMX_U8 *) pLcmlHdr->pIpParam,
                                                       sizeof(G726ENC_ParamStruct),
                                                       NULL);
                            if (eError != OMX_ErrorNone) {
                            G726ENC_DPRINT("OMX_ErrorHardware Occurred!!!!!!!!\n");                                       
                                eError = OMX_ErrorHardware;
                                goto EXIT;
                            }

                        }
                    } else{
                        pComponentPrivate->cbInfo.EmptyBufferDone (
                                           pComponentPrivate->pHandle,
                                           pComponentPrivate->pHandle->pApplicationPrivate,
                                           pBufHeader
                                           );
                    }

                } else {
                    pComponentPrivate->pInputBufHdrPending[pComponentPrivate->nNumInputBufPending++] = pBufHeader;
                }
            } else {
                
                G726ENC_DPRINT("%d :: Calling EmptyBufferDone\n",__LINE__);
                pComponentPrivate->cbInfo.EmptyBufferDone( pComponentPrivate->pHandle,
                                                           pComponentPrivate->pHandle->pApplicationPrivate,
                                                           pComponentPrivate->pInputBufferList->pBufHdr[0]);
                pComponentPrivate->nEmptyBufferDoneCount++;
            }
            if(pBufHeader->nFlags == OMX_BUFFERFLAG_EOS) {
              pComponentPrivate->pOutputBufferList->pBufHdr[0]->nFlags |= OMX_BUFFERFLAG_EOS;
            }
            if(pBufHeader->pMarkData){
                /* copy mark to output buffer header */
                pComponentPrivate->pOutputBufferList->pBufHdr[0]->pMarkData = pBufHeader->pMarkData;
                pComponentPrivate->pOutputBufferList->pBufHdr[0]->hMarkTargetComponent = pBufHeader->hMarkTargetComponent;
                /* trigger event handler if we are supposed to */
                if(pBufHeader->hMarkTargetComponent == pComponentPrivate->pHandle && pBufHeader->pMarkData){
                    pComponentPrivate->cbInfo.EventHandler( pComponentPrivate->pHandle,
                                                            pComponentPrivate->pHandle->pApplicationPrivate,
                                                            OMX_EventMark,
                                                            0,
                                                            0,
                                                            pBufHeader->pMarkData);
                }
            }
        }
    } else if (eDir == OMX_DirOutput) {
        /* Make sure that output buffer is issued to output stream only when
         * there is an outstanding input buffer already issued on input stream
         */

        eError = G726ENC_GetCorrespondingLCMLHeader(pComponentPrivate, pBufHeader->pBuffer, OMX_DirOutput, &pLcmlHdr);
        if (eError != OMX_ErrorNone) {
                    G726ENC_DPRINT("%d :: Error: Invalid Buffer Came ...\n",__LINE__);
                    goto EXIT;
        }
/*        pLcmlHdr->pOpParam->usNbFrames =1;*/   /*<<<<<<<<<<---------------- according SN guide this is not needed!!!!!!*/
                                             /*            The SN should be the one that set this Information            */
                                             /*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
       
        if (!(pComponentPrivate->bIsStopping)) {
                G726ENC_DPRINT ("%d: Sending Empty OUTPUT BUFFER to Codec = %p\n",__LINE__,pBufHeader->pBuffer);

                if (pComponentPrivate->curState == OMX_StateExecuting) {
                    if (!G726ENC_IsPending(pComponentPrivate,pBufHeader,OMX_DirOutput)) {
                        G726ENC_SetPending(pComponentPrivate,pBufHeader,OMX_DirOutput,__LINE__);

                        eError = LCML_QueueBuffer( pLcmlHandle->pCodecinterfacehandle,
                                                   EMMCodecOuputBuffer,
                                                   (OMX_U8 *)pBufHeader->pBuffer,
                                                   pBufHeader->nAllocLen,
                                                   pBufHeader->nFilledLen,
                                                   NULL,
                                                   0,
                                                   NULL);
                        if (eError != OMX_ErrorNone ) {
                            G726ENC_DPRINT ("%d :: IssuingDSP OP: Error Occurred\n",__LINE__);
                            eError = OMX_ErrorHardware;
                            goto EXIT;
                        }
                    }
                } else if (pComponentPrivate->curState == OMX_StatePause) {
                    pComponentPrivate->pOutputBufHdrPending[pComponentPrivate->nNumOutputBufPending++] = pBufHeader;
                }
         
        } else {
            if (pComponentPrivate->curState == OMX_StateExecuting) {
                if (!G726ENC_IsPending(pComponentPrivate,pBufHeader,OMX_DirOutput)) {
                    G726ENC_SetPending(pComponentPrivate,pBufHeader,OMX_DirOutput,__LINE__);

                    eError = LCML_QueueBuffer( pLcmlHandle->pCodecinterfacehandle,
                                               EMMCodecOuputBuffer,
                                               (OMX_U8 *)pBufHeader->pBuffer,
                                               pBufHeader->nAllocLen,
                                               pBufHeader->nFilledLen,
                                               NULL,
                                               0,
                                               NULL);
                    if (eError != OMX_ErrorNone ) {
                        G726ENC_DPRINT ("%d :: IssuingDSP OP: Error Occurred\n",__LINE__);
                          eError = OMX_ErrorHardware;
                          goto EXIT;
                    }


                }
            } else if (pComponentPrivate->curState == OMX_StatePause) {
                pComponentPrivate->pOutputBufHdrPending[pComponentPrivate->nNumOutputBufPending++] = pBufHeader;
            }
        }
    } else {
        eError = OMX_ErrorBadParameter;
    }

EXIT:
    G726ENC_DPRINT("%d :: Exiting from  G726ENC_HandleDataBufFromApp \n",__LINE__);
    G726ENC_DPRINT("%d :: Returning error %d\n",__LINE__,eError);
    return eError;
}

/*-------------------------------------------------------------------*/
/**
* G726ENC_GetBufferDirection () This function is used by the component
* to get the direction of the buffer
* @param eDir pointer will be updated with buffer direction
* @param pBufHeader pointer to the buffer to be requested to be filled
*
* @retval none
**/
/*-------------------------------------------------------------------*/

OMX_ERRORTYPE G726ENC_GetBufferDirection(OMX_BUFFERHEADERTYPE *pBufHeader,
                                                         OMX_DIRTYPE *eDir)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    G726ENC_COMPONENT_PRIVATE *pComponentPrivate = pBufHeader->pPlatformPrivate;
    OMX_U32 nBuf = 0;
    OMX_BUFFERHEADERTYPE *pBuf = NULL;
    OMX_U32 flag = 1,i = 0;
    G726ENC_DPRINT("%d :: Entering G726ENC_GetBufferDirection Function\n",__LINE__);
    /*Search this buffer in input buffers list */
    nBuf = pComponentPrivate->pInputBufferList->numBuffers;
    for(i=0; i<nBuf; i++) {
        pBuf = pComponentPrivate->pInputBufferList->pBufHdr[i];
        if(pBufHeader == pBuf) {
            *eDir = OMX_DirInput;
            G726ENC_DPRINT("%d :: pBufHeader = %p is INPUT BUFFER pBuf = %p\n",__LINE__,pBufHeader,pBuf);
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
            G726ENC_DPRINT("%d :: pBufHeader = %p is OUTPUT BUFFER pBuf = %p\n",__LINE__,pBufHeader,pBuf);
            flag = 0;
            goto EXIT;
        }
    }

    if (flag == 1) {
        G726ENC_DPRINT("%d :: Buffer %p is Not Found in the List\n",__LINE__, pBufHeader);
        eError = OMX_ErrorUndefined;
        goto EXIT;
    }
EXIT:
    G726ENC_DPRINT("%d :: Exiting G726ENC_GetBufferDirection Function\n",__LINE__);
    G726ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}

/* -------------------------------------------------------------------*/
/**
  * G726ENC_GetCorrespondingLCMLHeader() function will be called by LCML_Callback
  * component to write the msg
  * @param *pBuffer,          Event which gives to details about USN status
  * @param G726ENC_LCML_BUFHEADERTYPE **ppLcmlHdr
  * @param  OMX_DIRTYPE eDir this gives direction of the buffer
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
 **/
/* -------------------------------------------------------------------*/
OMX_ERRORTYPE G726ENC_GetCorrespondingLCMLHeader(G726ENC_COMPONENT_PRIVATE *pComponentPrivate, 
                                                  OMX_U8 *pBuffer,
                                                  OMX_DIRTYPE eDir,
                                                  G726ENC_LCML_BUFHEADERTYPE **ppLcmlHdr)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    G726ENC_LCML_BUFHEADERTYPE *pLcmlBufHeader = NULL;
    OMX_U32 i = 0,nIpBuf = 0,nOpBuf = 0;
    nIpBuf = pComponentPrivate->pInputBufferList->numBuffers;
    nOpBuf = pComponentPrivate->pOutputBufferList->numBuffers;
    G726ENC_DPRINT("%d :: Entering G726ENC_GetCorrespondingLCMLHeader..\n",__LINE__);
    while (!pComponentPrivate->bInitParamsInitialized) {
        G726ENC_DPRINT("%d :: Waiting for init to complete........\n",__LINE__);

        sched_yield();

    }
    if(eDir == OMX_DirInput) {
        G726ENC_DPRINT("%d :: Entering G726ENC_GetCorrespondingLCMLHeader..\n",__LINE__);
        pLcmlBufHeader = pComponentPrivate->pLcmlBufHeader[G726ENC_INPUT_PORT];
        for(i = 0; i < nIpBuf; i++) {
            G726ENC_DPRINT("%d :: pBuffer = %p\n",__LINE__,pBuffer);
            G726ENC_DPRINT("%d :: pLcmlBufHeader->buffer->pBuffer = %p\n",__LINE__,pLcmlBufHeader->buffer->pBuffer);
            if(pBuffer == pLcmlBufHeader->buffer->pBuffer) {
                *ppLcmlHdr = pLcmlBufHeader;
                 G726ENC_DPRINT("%d :: Corresponding Input LCML Header Found = %p\n",__LINE__,pLcmlBufHeader);
                 eError = OMX_ErrorNone;
                 goto EXIT;
            }
            pLcmlBufHeader++;
        }
    } else if (eDir == OMX_DirOutput) {
        G726ENC_DPRINT("%d :: Entering G726ENC_GetCorrespondingLCMLHeader..\n",__LINE__);
        pLcmlBufHeader = pComponentPrivate->pLcmlBufHeader[G726ENC_OUTPUT_PORT];
        for(i = 0; i < nOpBuf; i++) {
            G726ENC_DPRINT("%d :: pBuffer = %p\n",__LINE__,pBuffer);
            G726ENC_DPRINT("%d :: pLcmlBufHeader->buffer->pBuffer = %p\n",__LINE__,pLcmlBufHeader->buffer->pBuffer);
            if(pBuffer == pLcmlBufHeader->buffer->pBuffer) {
                *ppLcmlHdr = pLcmlBufHeader;
                 G726ENC_DPRINT("%d :: Corresponding Output LCML Header Found = %p\n",__LINE__,pLcmlBufHeader);
                 eError = OMX_ErrorNone;
                 goto EXIT;
            }
            pLcmlBufHeader++;
        }
    } else {
      G726ENC_DPRINT("%d :: Invalid Buffer Type :: exiting...\n",__LINE__);
      eError = OMX_ErrorUndefined;
    }

EXIT:
    G726ENC_DPRINT("%d :: Exiting G726ENC_GetCorrespondingLCMLHeader..\n",__LINE__);
    G726ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}

/* -------------------------------------------------------------------*/
/**
  *  G726ENC_LCMLCallback() will be called LCML component to write the msg
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

OMX_ERRORTYPE G726ENC_LCMLCallback (TUsnCodecEvent event,void * args[10])
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U8 *pBuffer = args[1];
    G726ENC_LCML_BUFHEADERTYPE *pLcmlHdr = NULL;
    G726ENC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_U16 i=0;    
    OMX_COMPONENTTYPE* pHandle = NULL;
#ifdef RESOURCE_MANAGER_ENABLED 
    OMX_ERRORTYPE rm_error = OMX_ErrorNone;
#endif
 
    G726ENC_DPRINT("%d :: Entering the G726ENC_LCMLCallback Function\n",__LINE__);
    pComponentPrivate = (G726ENC_COMPONENT_PRIVATE*)((LCML_DSP_INTERFACE*)args[6])->pComponentPrivate;
    
    pHandle = pComponentPrivate->pHandle;    

#ifdef G726ENC_DEBUG
    switch(event) {

        case EMMCodecDspError:
            G726ENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecDspError\n");
            break;

        case EMMCodecInternalError:
            G726ENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecInternalError\n");
            break;

        case EMMCodecInitError:
            G726ENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecInitError\n");
            break;

        case EMMCodecDspMessageRecieved:
            G726ENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecDspMessageRecieved\n");
            break;

        case EMMCodecBufferProcessed:
            G726ENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecBufferProcessed\n");
            break;

        case EMMCodecProcessingStarted:
            G726ENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecProcessingStarted\n");
            break;

        case EMMCodecProcessingPaused:
            G726ENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecProcessingPaused\n");
            break;

        case EMMCodecProcessingStoped:
            G726ENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecProcessingStoped\n");
            break;

        case EMMCodecProcessingEof:
            G726ENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecProcessingEof\n");
            break;

        case EMMCodecBufferNotProcessed:
            G726ENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecBufferNotProcessed\n");
            break;

        case EMMCodecAlgCtrlAck:
            G726ENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecAlgCtrlAck\n");
            break;

        case EMMCodecStrmCtrlAck:
            G726ENC_DPRINT("[LCML CALLBACK EVENT]  EMMCodecStrmCtrlAck\n");
            break;
    }
#endif

    if(event == EMMCodecBufferProcessed)
    {
        if((OMX_U32)args[0] == EMMCodecInputBuffer) {
            G726ENC_DPRINT("%d :: INPUT: pBuffer = %p\n",__LINE__, pBuffer);

            eError = G726ENC_GetCorrespondingLCMLHeader(pComponentPrivate, pBuffer, OMX_DirInput, &pLcmlHdr);
            if (eError != OMX_ErrorNone) {
                G726ENC_DPRINT("%d :: Error: Invalid Buffer Came ...\n",__LINE__);
                goto EXIT;
            }
            G726ENC_ClearPending(pComponentPrivate,pLcmlHdr->buffer,OMX_DirInput,__LINE__);
            if(pComponentPrivate->dasfMode == 0) {
                G726ENC_DPRINT("%d: Component Sending Empty Input buffer%p to App\n",__LINE__,pLcmlHdr->buffer->pBuffer);
                pComponentPrivate->cbInfo.EmptyBufferDone (
                                       pHandle,
                                       pHandle->pApplicationPrivate,
                                       pLcmlHdr->buffer
                                       );

                pComponentPrivate->nEmptyBufferDoneCount++;
            }
        } else if((OMX_U32)args[0] == EMMCodecOuputBuffer) {
            G726ENC_DPRINT("%d :: OUTPUT: pBuffer = %p\n",__LINE__, pBuffer);
            pComponentPrivate->nOutStandingFillDones++;
            eError = G726ENC_GetCorrespondingLCMLHeader(pComponentPrivate, pBuffer, OMX_DirOutput, &pLcmlHdr);
            if (eError != OMX_ErrorNone) {
                G726ENC_DPRINT("%d :: Error: Invalid Buffer Came ...\n",__LINE__);
                goto EXIT;
            }
            G726ENC_DPRINT("%d :: Output: pLcmlHdr->buffer->pBuffer = %p\n",__LINE__, pLcmlHdr->buffer->pBuffer);
            pLcmlHdr->buffer->nFilledLen = (OMX_U32)args[8];

            G726ENC_DPRINT("%d :: Output: pBuffer = %ld\n",__LINE__, pLcmlHdr->buffer->nFilledLen);
            pComponentPrivate->lastOutBufArrived = pLcmlHdr->buffer;
            G726ENC_ClearPending(pComponentPrivate,pLcmlHdr->buffer,OMX_DirOutput,__LINE__);
            /* Copying time stamp information to output buffer */
            pLcmlHdr->buffer->nTimeStamp = (OMX_TICKS)pComponentPrivate->arrTimestamp[pComponentPrivate->OpBufindex];
            /* Copying nTickCount information to output buffer */
            pLcmlHdr->buffer->nTickCount = pComponentPrivate->arrTickCount[pComponentPrivate->OpBufindex];
        
            pComponentPrivate->OpBufindex++;
            pComponentPrivate->OpBufindex %= pComponentPrivate->pPortDef[G726ENC_OUTPUT_PORT]->nBufferCountActual;              

            pComponentPrivate->cbInfo.FillBufferDone( pHandle,
                                                  pHandle->pApplicationPrivate,
                                                  pLcmlHdr->buffer);

            pComponentPrivate->nFillBufferDoneCount++;

            pComponentPrivate->nOutStandingFillDones--;
        }
    }
    else if (event == EMMCodecStrmCtrlAck) {
        G726ENC_DPRINT("%d :: GOT MESSAGE USN_DSPACK_STRMCTRL \n",__LINE__);
		if (args[1] == (void *)USN_STRMCMD_FLUSH) {
                 if ( args[2] == (void *)EMMCodecInputBuffer) {
                     if (args[0] == (void *)USN_ERR_NONE ) {
                         G726ENC_DPRINT("Flushing input port %d\n",__LINE__);
                       
                         for (i=0; i < pComponentPrivate->nNumInputBufPending; i++) {
                              G726ENC_DPRINT("%d :: Calling EmptyBufferDone\n", __LINE__);
                              pComponentPrivate->cbInfo.EmptyBufferDone (
                                       pComponentPrivate->pHandle,
                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                       pComponentPrivate->pInputBufHdrPending[i]);
                              pComponentPrivate->pInputBufHdrPending[i] = NULL;
                              pComponentPrivate->nEmptyBufferDoneCount++;
                              G726ENC_DPRINT("%d :: Incrementing nEmptyBufferDoneCount = %ld\n",__LINE__,pComponentPrivate->nEmptyBufferDoneCount);
                         }
			             pComponentPrivate->nNumInputBufPending=0;                         
                         /* return all input buffers */
                         pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                    pHandle->pApplicationPrivate,
                                                    OMX_EventCmdComplete,
                                                    OMX_CommandFlush,
                                                    G726ENC_INPUT_PORT,
                                                    NULL);	
                     } else {
                         G726ENC_DPRINT ("LCML reported error while flushing input port\n");
                         goto EXIT;                            
                     }
                 }
                 else if ( args[2] == (void *)EMMCodecOuputBuffer) { 
                     if (args[0] == (void *)USN_ERR_NONE ) {                      
                         G726ENC_DPRINT("Flushing output port %d\n",__LINE__);

                        for (i=0; i < pComponentPrivate->nNumOutputBufPending; i++) {
                             G726ENC_DPRINT("%d :: Calling FillBufferDone\n", __LINE__);
                             pComponentPrivate->cbInfo.FillBufferDone (
                                       pComponentPrivate->pHandle,
                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                       pComponentPrivate->pOutputBufHdrPending[i]);
                             pComponentPrivate->pOutputBufHdrPending[i] = NULL;
                             pComponentPrivate->nFillBufferDoneCount++;
                             G726ENC_DPRINT("%d :: Incrementing nFillBufferDoneCount = %ld\n",__LINE__,pComponentPrivate->nFillBufferDoneCount);
                        }
                        pComponentPrivate->nNumOutputBufPending=0;
                        /* return all output buffers */
                        pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                    pHandle->pApplicationPrivate,
                                                    OMX_EventCmdComplete,
                                                    OMX_CommandFlush,
                                                    G726ENC_OUTPUT_PORT,
                                                    NULL);
                     } else {
                         G726ENC_DPRINT ("LCML reported error while flushing output port\n");
                         goto EXIT;                            
                     }
                 }
            }        
    }
    else if(event == EMMCodecProcessingStoped) {

        G726ENC_DPRINT("%d :: GOT MESSAGE USN_DSPACK_STOP \n",__LINE__);
        if (!pComponentPrivate->bNoIdleOnStop) {
            pComponentPrivate->curState = OMX_StateIdle;
#ifdef RESOURCE_MANAGER_ENABLED
            rm_error = RMProxy_NewSendCommand(pHandle,
                                              RMProxy_StateSet,
                                              OMX_G726_Encoder_COMPONENT,
                                              OMX_StateIdle, 3456,NULL);
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
        G726ENC_DPRINT("%d :: commandedState  = %ld\n",__LINE__,(OMX_U32)args[0]);
        G726ENC_DPRINT("%d :: arg1 = %ld\n",__LINE__,(OMX_U32)args[1]);
        G726ENC_DPRINT("%d :: arg2 = %ld\n",__LINE__,(OMX_U32)args[2]);

        if(0x0500 == (OMX_U32)args[2]) {
            G726ENC_DPRINT("%d :: EMMCodecDspMessageRecieved\n",__LINE__);
        }
    }
    else if(event == EMMCodecAlgCtrlAck) {
        G726ENC_DPRINT("%d :: GOT MESSAGE USN_DSPACK_ALGCTRL \n",__LINE__);
    }
    else if (event == EMMCodecDspError) {
        if(((int)args[4] == USN_ERR_WARNING) && ((int)args[5] == IUALG_WARN_PLAYCOMPLETED)) {
            G726ENC_DPRINT("%d :: GOT MESSAGE IUALG_WARN_PLAYCOMPLETED\n",__LINE__);
            if(pComponentPrivate->lastOutBufArrived!=NULL && !pComponentPrivate->dasfMode){
                     pComponentPrivate->lastOutBufArrived->nFlags = OMX_BUFFERFLAG_EOS;
                     pComponentPrivate->LastBufSent=0;
                     /*TODO: add eventhandler to report eos to application*/
            }
        }
    }
EXIT:
    G726ENC_DPRINT("%d :: Exiting the G726ENC_LCMLCallback Function\n",__LINE__);
    G726ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}

/* ================================================================================= */
/**
  *  G726ENC_GetLCMLHandle()
  *
  * @retval OMX_HANDLETYPE
  */
/* ================================================================================= */
#ifndef UNDER_CE
OMX_HANDLETYPE G726ENC_GetLCMLHandle(G726ENC_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_ERRORTYPE (*fpGetHandle)(OMX_HANDLETYPE);
    OMX_HANDLETYPE pHandle = NULL;
    void *handle = NULL;
    char *error = NULL;
    G726ENC_DPRINT("%d :: Entering G726ENC_GetLCMLHandle..\n",__LINE__);
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
        G726ENC_DPRINT("%d :: OMX_ErrorUndefined...\n",__LINE__);
        pHandle = NULL;
        goto EXIT;
    }
   
    ((LCML_DSP_INTERFACE*)pHandle)->pComponentPrivate = pComponentPrivate;
    
    pComponentPrivate->ptrLibLCML=handle;			/* saving LCML lib pointer  */

EXIT:
    G726ENC_DPRINT("%d :: Exiting G726ENC_GetLCMLHandle..\n",__LINE__);
    G726ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
    return pHandle;
}

#else
/*WINDOWS Explicit dll load procedure*/
OMX_HANDLETYPE G726ENC_GetLCMLHandle()
{
    typedef OMX_ERRORTYPE (*LPFNDLLFUNC1)(OMX_HANDLETYPE);
    OMX_HANDLETYPE pHandle = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    HINSTANCE hDLL;               // Handle to DLL
    LPFNDLLFUNC1 fpGetHandle1;
    hDLL = LoadLibraryEx(TEXT("OAF_BML.dll"), NULL,0);
    if (hDLL == NULL) {
        //fputs(dlerror(), stderr);
        G726ENC_DPRINT("BML Load Failed!!!\n");
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
        G726ENC_DPRINT("eError != OMX_ErrorNone...\n");
        pHandle = NULL;
        return pHandle;
    }
    return pHandle;
}
#endif

/* ================================================================================= */
/**
* @fn G726ENC_SetPending() description for G726ENC_SetPending
G726ENC_SetPending().
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
void G726ENC_SetPending(G726ENC_COMPONENT_PRIVATE *pComponentPrivate,
                         OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir, OMX_U32 lineNumber)
{
    OMX_U16 i = 0;

    if (eDir == OMX_DirInput) {
        for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pInputBufferList->pBufHdr[i]) {
                pComponentPrivate->pInputBufferList->bBufferPending[i] = 1;
                G726ENC_DPRINT("****INPUT BUFFER %d IS PENDING Line %ld******\n",i,lineNumber);
            }
        }
    }
    else {
        for (i=0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pOutputBufferList->pBufHdr[i]) {
                pComponentPrivate->pOutputBufferList->bBufferPending[i] = 1;
                G726ENC_DPRINT("****OUTPUT BUFFER %d IS PENDING Line %ld*****\n",i,lineNumber);
            }
        }
    }
}
/* ================================================================================= */
/**
* @fn G726ENC_ClearPending() description for G726ENC_ClearPending
G726ENC_ClearPending().
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
void G726ENC_ClearPending(G726ENC_COMPONENT_PRIVATE *pComponentPrivate,
                           OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir, OMX_U32 lineNumber)
{
    OMX_U16 i = 0;

    if (eDir == OMX_DirInput) {
        for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pInputBufferList->pBufHdr[i]) {
                pComponentPrivate->pInputBufferList->bBufferPending[i] = 0;
                G726ENC_DPRINT("****INPUT BUFFER %d IS RECLAIMED Line %ld*****\n",i,lineNumber);
            }
        }
    }
    else {
        for (i=0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pOutputBufferList->pBufHdr[i]) {
                pComponentPrivate->pOutputBufferList->bBufferPending[i] = 0;
                G726ENC_DPRINT("****OUTPUT BUFFER %d IS RECLAIMED Line %ld*****\n",i,lineNumber);
            }
        }
    }
}
/* ================================================================================= */
/**
* @fn G726ENC_IsPending() description for G726ENC_IsPending
G726ENC_IsPending().
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
OMX_U32 G726ENC_IsPending(G726ENC_COMPONENT_PRIVATE *pComponentPrivate,
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
* @fn G726ENC_IsValid() description for G726ENC_IsValid
G726ENC_IsValid().
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
OMX_U32 G726ENC_IsValid(G726ENC_COMPONENT_PRIVATE *pComponentPrivate,
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
* @G726ENC_FillLCMLInitParamsEx() This function is used by the component thread to
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
OMX_ERRORTYPE G726ENC_FillLCMLInitParamsEx(OMX_HANDLETYPE pComponent)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nIpBuf = 0,nIpBufSize = 0,nOpBuf = 0,nOpBufSize = 0;
    OMX_BUFFERHEADERTYPE *pTemp = NULL;
    LCML_DSP_INTERFACE *pHandle = (LCML_DSP_INTERFACE *)pComponent;
    G726ENC_COMPONENT_PRIVATE *pComponentPrivate = pHandle->pComponentPrivate;
    G726ENC_LCML_BUFHEADERTYPE *pTemp_lcml = NULL;
    OMX_U32 i = 0;
    OMX_U32 size_lcml = 0;
    G726ENC_DPRINT("%d :: G726ENC_FillLCMLInitParamsEx\n",__LINE__);
    nIpBuf = pComponentPrivate->pInputBufferList->numBuffers;
    nIpBufSize = pComponentPrivate->pPortDef[G726ENC_INPUT_PORT]->nBufferSize;
    nOpBuf = pComponentPrivate->pOutputBufferList->numBuffers;
    nOpBufSize = pComponentPrivate->pPortDef[G726ENC_OUTPUT_PORT]->nBufferSize;
    
    pComponentPrivate->nRuntimeInputBuffers = nIpBuf;
    pComponentPrivate->nRuntimeOutputBuffers = nOpBuf;
    
    G726ENC_DPRINT("%d :: ------ Buffer Details -----------\n",__LINE__);
    G726ENC_DPRINT("%d :: Input  Buffer Count = %ld\n",__LINE__,nIpBuf);
    G726ENC_DPRINT("%d :: Input  Buffer Size = %ld\n",__LINE__,nIpBufSize);
    G726ENC_DPRINT("%d :: Output Buffer Count = %ld\n",__LINE__,nOpBuf);
    G726ENC_DPRINT("%d :: Output Buffer Size = %ld\n",__LINE__,nOpBufSize);
    G726ENC_DPRINT("%d :: ------ Buffer Details ------------\n",__LINE__);
    /* Allocate memory for all input buffer headers..
     * This memory pointer will be sent to LCML */
    size_lcml = nIpBuf * sizeof(G726ENC_LCML_BUFHEADERTYPE);
    pTemp_lcml = (G726ENC_LCML_BUFHEADERTYPE *)SafeMalloc(size_lcml);
    G726ENC_MEMPRINT("%d :: [ALLOC] %p\n",__LINE__,pTemp_lcml);
    if(pTemp_lcml == NULL) {
        G726ENC_DPRINT("%d :: Memory Allocation Failed\n",__LINE__);
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    pComponentPrivate->pLcmlBufHeader[G726ENC_INPUT_PORT] = pTemp_lcml;
    for (i=0; i<nIpBuf; i++) {
        G726ENC_DPRINT("%d :: INPUT--------- Inside Ip Loop\n",__LINE__);
        pTemp = pComponentPrivate->pInputBufferList->pBufHdr[i];
        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pTemp->nAllocLen = nIpBufSize;
        pTemp->nFilledLen = nIpBufSize;
        pTemp->nVersion.s.nVersionMajor = G726ENC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = G726ENC_MINOR_VER;
        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = G726ENC_NOT_USED;
        pTemp_lcml->buffer = pTemp;
        G726ENC_DPRINT("%d :: pTemp_lcml->buffer->pBuffer = %p \n",__LINE__,pTemp_lcml->buffer->pBuffer);
        pTemp_lcml->eDir = OMX_DirInput;
        OMX_NBMALLOC_STRUCT(pTemp_lcml->pIpParam, G726ENC_ParamStruct);



        /*pTemp_lcml->pIpParam->usLastFrame = 0;*/
        /* This means, it is not a last buffer. This flag is to be modified by
         * the application to indicate the last buffer */
        pTemp->nFlags = G726ENC_NORMAL_BUFFER;
        pTemp++;
        pTemp_lcml++;
    }

    /* Allocate memory for all output buffer headers..
     * This memory pointer will be sent to LCML */
    size_lcml = nOpBuf * sizeof(G726ENC_LCML_BUFHEADERTYPE);
    pTemp_lcml = (G726ENC_LCML_BUFHEADERTYPE *)SafeMalloc(size_lcml);
    G726ENC_MEMPRINT("%d :: [ALLOC] %p\n",__LINE__,pTemp_lcml);
    if(pTemp_lcml == NULL) {
        G726ENC_DPRINT("%d :: Memory Allocation Failed\n",__LINE__);
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    pComponentPrivate->pLcmlBufHeader[G726ENC_OUTPUT_PORT] = pTemp_lcml;
    for (i=0; i<nOpBuf; i++) {
        G726ENC_DPRINT("%d :: OUTPUT--------- Inside Op Loop\n",__LINE__);
        pTemp = pComponentPrivate->pOutputBufferList->pBufHdr[i];
        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pTemp->nAllocLen = nOpBufSize;
        pTemp->nFilledLen = nOpBufSize;
        pTemp->nVersion.s.nVersionMajor = G726ENC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = G726ENC_MINOR_VER;
        pComponentPrivate->nVersion = pTemp->nVersion.nVersion;
        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = G726ENC_NOT_USED;
        pTemp_lcml->buffer = pTemp;
        G726ENC_DPRINT("%d :: pTemp_lcml->buffer->pBuffer = %p \n",__LINE__,pTemp_lcml->buffer->pBuffer);
        pTemp_lcml->eDir = OMX_DirOutput;
/*        OMX_NBMALLOC_STRUCT(pTemp_lcml->pOpParam, G726ENC_ParamStruct);*//*According SN guide this variable should be neede*/
/*        memset(pTemp_lcml->pOpParam,0,sizeof(G726ENC_ParamStruct));*/

        /* This means, it is not a last buffer. This flag is to be modified by
         * the application to indicate the last buffer */
        pTemp->nFlags = G726ENC_NORMAL_BUFFER;
        pTemp++;
        pTemp_lcml++;
    }
    pComponentPrivate->bPortDefsAllocated = 1;
    pComponentPrivate->bInitParamsInitialized = 1;
EXIT:
    G726ENC_DPRINT("%d :: Exiting G726ENC_FillLCMLInitParamsEx\n",__LINE__);
    G726ENC_DPRINT("%d :: Returning = 0x%x\n",__LINE__,eError);
    return eError;
}

#ifdef RESOURCE_MANAGER_ENABLED
/***********************************
 *  Callback to the RM                                       *
 ***********************************/
void G726ENC_ResourceManagerCallback(RMPROXY_COMMANDDATATYPE cbData)
{
    OMX_COMMANDTYPE Cmd = OMX_CommandStateSet;
    OMX_STATETYPE state = OMX_StateIdle;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)cbData.hComponent;
    G726ENC_COMPONENT_PRIVATE *pCompPrivate = NULL;

    pCompPrivate = (G726ENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

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
