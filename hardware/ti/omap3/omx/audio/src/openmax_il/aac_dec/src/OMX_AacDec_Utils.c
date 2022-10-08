
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
* @file OMX_AacDec_Utils.c
*
* This file implements OMX Component for AAC Decoder that
* is fully compliant with the OMX Audio specification 1.0.
*
* @path  $(CSLPATH)\
*
* @rev  1.0
*/
/* ----------------------------------------------------------------------------
*!
*! Revision History
*! ===================================
*! 13-Dec-2005 mf:  Initial Version. Change required per OMAPSWxxxxxxxxx
*! to provide _________________.
*!
*!
*! 17-Aug-2006 mf:
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
#endif

#include <dbapi.h>
#include <string.h>
#include <stdio.h>

/*------- Program Header Files -----------------------------------------------*/
#ifdef RESOURCE_MANAGER_ENABLED
#include <ResourceManagerProxyAPI.h>
#endif

#include "LCML_DspCodec.h"
#include "OMX_AacDec_Utils.h"
#include "Aacdecsocket_ti.h"
#include <decode_common_ti.h>
#include "usn.h"

#ifdef UNDER_CE
#define HASHINGENABLE 1
#endif

/* ================================================================================= * */
/**
* @fn AACDEC_Fill_LCMLInitParams() fills the LCML initialization structure.
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
OMX_ERRORTYPE AACDEC_Fill_LCMLInitParams(OMX_HANDLETYPE pComponent,
                                  LCML_DSP *plcml_Init, OMX_U16 arr[])
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nIpBuf,nIpBufSize,nOpBuf,nOpBufSize;
    OMX_U16 i;
    OMX_BUFFERHEADERTYPE *pTemp;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    AACDEC_COMPONENT_PRIVATE *pComponentPrivate = (AACDEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    AACD_LCML_BUFHEADERTYPE *pTemp_lcml;
    OMX_U32 size_lcml;
    char *ptr;
    pComponentPrivate->nRuntimeInputBuffers = 0;
    pComponentPrivate->nRuntimeOutputBuffers = 0;

    OMX_PRDSP2(pComponentPrivate->dbg, "%d:::pComponentPrivate->pPortDef[INPUT_PORT_AACDEC]->bPopulated = %d\n",
                  __LINE__,pComponentPrivate->pPortDef[INPUT_PORT_AACDEC]->bPopulated);
    OMX_PRDSP2(pComponentPrivate->dbg, "%d:::pComponentPrivate->pPortDef[INPUT_PORT_AACDEC]->bEnabled = %d\n",
                  __LINE__,pComponentPrivate->pPortDef[INPUT_PORT_AACDEC]->bEnabled);
    OMX_PRDSP2(pComponentPrivate->dbg, "%d:::pComponentPrivate->pPortDef[OUTPUT_PORT_AACDEC]->bPopulated = %d\n",
                  __LINE__,pComponentPrivate->pPortDef[OUTPUT_PORT_AACDEC]->bPopulated);
    OMX_PRDSP2(pComponentPrivate->dbg, "%d:::pComponentPrivate->pPortDef[OUTPUT_PORT_AACDEC]->bEnabled = %d\n",
                  __LINE__,pComponentPrivate->pPortDef[OUTPUT_PORT_AACDEC]->bEnabled);

    pComponentPrivate->strmAttr = NULL;
    nIpBuf = pComponentPrivate->pInputBufferList->numBuffers;
    pComponentPrivate->nRuntimeInputBuffers = nIpBuf;
    nOpBuf = pComponentPrivate->pOutputBufferList->numBuffers;
    pComponentPrivate->nRuntimeOutputBuffers = nOpBuf;
    nIpBufSize = pComponentPrivate->pPortDef[INPUT_PORT_AACDEC]->nBufferSize;
    nOpBufSize = pComponentPrivate->pPortDef[OUTPUT_PORT_AACDEC]->nBufferSize;

    OMX_PRBUFFER2(pComponentPrivate->dbg, "Input Buffer Count = %ld\n",nIpBuf);
    OMX_PRBUFFER2(pComponentPrivate->dbg, "Input Buffer Size = %ld\n",nIpBufSize);
    OMX_PRBUFFER2(pComponentPrivate->dbg, "Output Buffer Count = %ld\n",nOpBuf);
    OMX_PRBUFFER2(pComponentPrivate->dbg, "Output Buffer Size = %ld\n",nOpBufSize);

    plcml_Init->In_BufInfo.nBuffers = nIpBuf;
    plcml_Init->In_BufInfo.nSize = nIpBufSize;
    plcml_Init->In_BufInfo.DataTrMethod = DMM_METHOD;
    plcml_Init->Out_BufInfo.nBuffers = nOpBuf;
    plcml_Init->Out_BufInfo.nSize = nOpBufSize;
    plcml_Init->Out_BufInfo.DataTrMethod = DMM_METHOD;

    plcml_Init->NodeInfo.nNumOfDLLs = 3;

    memset(plcml_Init->NodeInfo.AllUUIDs[0].DllName,0, sizeof(plcml_Init->NodeInfo.AllUUIDs[0].DllName));
    memset(plcml_Init->NodeInfo.AllUUIDs[1].DllName,0, sizeof(plcml_Init->NodeInfo.AllUUIDs[1].DllName));
    memset(plcml_Init->NodeInfo.AllUUIDs[2].DllName,0, sizeof(plcml_Init->NodeInfo.AllUUIDs[1].DllName));
    memset(plcml_Init->NodeInfo.AllUUIDs[0].DllName,0, sizeof(plcml_Init->DeviceInfo.AllUUIDs[1].DllName));

    plcml_Init->NodeInfo.AllUUIDs[0].uuid = (struct DSP_UUID*)&MPEG4AACDEC_SN_UUID;
    strcpy ((char*)plcml_Init->NodeInfo.AllUUIDs[0].DllName, AACDEC_DLL_NAME);
    plcml_Init->NodeInfo.AllUUIDs[0].eDllType = DLL_NODEOBJECT;

    plcml_Init->NodeInfo.AllUUIDs[1].uuid = (struct DSP_UUID*)&MPEG4AACDEC_SN_UUID;
    strcpy ((char*)plcml_Init->NodeInfo.AllUUIDs[1].DllName, AACDEC_DLL_NAME);
    plcml_Init->NodeInfo.AllUUIDs[1].eDllType = DLL_DEPENDENT;

    plcml_Init->NodeInfo.AllUUIDs[2].uuid = &USN_TI_UUID;
    strcpy ((char*)plcml_Init->NodeInfo.AllUUIDs[2].DllName, AACDEC_USN_DLL_NAME);
    plcml_Init->NodeInfo.AllUUIDs[2].eDllType = DLL_DEPENDENT;

    plcml_Init->SegID = OMX_AACDEC_DEFAULT_SEGMENT;
    plcml_Init->Timeout = OMX_AACDEC_SN_TIMEOUT;
    plcml_Init->Alignment = 0;
    plcml_Init->Priority = OMX_AACDEC_SN_PRIORITY;
    plcml_Init->ProfileID = -1;

    OMX_PRINT1(pComponentPrivate->dbg, "DLL name0 = %s\n",plcml_Init->NodeInfo.AllUUIDs[0].DllName);
    OMX_PRINT1(pComponentPrivate->dbg, "DLL name1 = %s\n",plcml_Init->NodeInfo.AllUUIDs[1].DllName);
    OMX_PRINT1(pComponentPrivate->dbg, "DLL name2 = %s\n",plcml_Init->NodeInfo.AllUUIDs[2].DllName);

    plcml_Init->DeviceInfo.TypeofDevice = 0; /*Initialisation for F2F mode*/
    plcml_Init->DeviceInfo.TypeofRender = 0;
    if(pComponentPrivate->dasfmode == 1) {

#ifndef DSP_RENDERING_ON
        AACDEC_OMX_ERROR_EXIT(eError, OMX_ErrorInsufficientResources,
                              "Flag DSP_RENDERING_ON Must Be Defined To Use Rendering");
#else

        LCML_STRMATTR *strmAttr;
        OMX_MALLOC_GENERIC(strmAttr, LCML_STRMATTR);
        pComponentPrivate->strmAttr = strmAttr;
        OMX_PRDSP2(pComponentPrivate->dbg, "%d :: AAC DECODER IS RUNNING UNDER DASF MODE \n",__LINE__);

        strmAttr->uSegid = 0;
        strmAttr->uAlignment = 0;
        strmAttr->uTimeout = -1;
        strmAttr->uBufsize = pComponentPrivate->pPortDef[OUTPUT_PORT_AACDEC]->nBufferSize;/*Changed for DASF AAC*/
        OMX_PRBUFFER2(pComponentPrivate->dbg, "%d::strmAttr->uBufsize:%d\n",__LINE__,strmAttr->uBufsize);
        strmAttr->uNumBufs = 2;
        strmAttr->lMode = STRMMODE_PROCCOPY;
        plcml_Init->DeviceInfo.TypeofDevice = 1;
        plcml_Init->DeviceInfo.TypeofRender = 0;
        plcml_Init->DeviceInfo.AllUUIDs[0].uuid = &DCTN_TI_UUID;
        plcml_Init->DeviceInfo.DspStream = strmAttr;
#endif
    }

    if (pComponentPrivate->dasfmode == 0){
        OMX_PRCOMM2(pComponentPrivate->dbg, "%d :: FILE MODE CREATE PHASE PARAMETERS\n",__LINE__);
        arr[0] = STREAM_COUNT_AACDEC;                        /*Number of Streams*/
        arr[1] = INPUT_PORT_AACDEC;                          /*ID of the Input Stream*/
        arr[2] = 0;                                          /*Type of Input Stream */
        arr[3] = 4;                                          /*Number of buffers for Input Stream*/
        arr[4] = OUTPUT_PORT_AACDEC;                         /*ID of the Output Stream*/
        arr[5] = 0;                                          /*Type of Output Stream */
        arr[6] = 4;                                          /*Number of buffers for Output Stream*/
        arr[7] = 0;                                          /*Decoder Output PCM width is 24-bit or 16-bit*/
        if(pComponentPrivate->nOpBit == 1){
            arr[7] = 1;
        }
        arr[8] = pComponentPrivate->framemode;               /*Frame mode enable */
        arr[9] = END_OF_CR_PHASE_ARGS;
    } else {
        OMX_PRDSP2(pComponentPrivate->dbg, "%d :: DASF MODE CREATE PHASE PARAMETERS\n",__LINE__);
        arr[0] = STREAM_COUNT_AACDEC;                         /*Number of Streams*/
        arr[1] = INPUT_PORT_AACDEC;                           /*ID of the Input Stream*/
        arr[2] = 0;                                           /*Type of Input Stream */
        arr[3] = 4;                                           /*Number of buffers for Input Stream*/
        arr[4] = OUTPUT_PORT_AACDEC;                          /*ID of the Output Stream*/
        arr[5] = 2;                                           /*Type of Output Stream */
        arr[6] = 2;                                           /*Number of buffers for Output Stream*/
        arr[7] = 0;                                           /*Decoder Output PCM width is 24-bit or 16-bit*/
        if(pComponentPrivate->nOpBit == 1) {
            arr[7] = 1;
        }
        arr[8] = pComponentPrivate->framemode;          /*Frame mode enable */
        arr[9] = END_OF_CR_PHASE_ARGS;
    }


    plcml_Init->pCrPhArgs = arr;

    OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: bufAlloced = %lu\n",__LINE__,pComponentPrivate->bufAlloced);
    size_lcml = nIpBuf * sizeof(AACD_LCML_BUFHEADERTYPE);

    OMX_MALLOC_SIZE(ptr,size_lcml,char);
    pTemp_lcml = (AACD_LCML_BUFHEADERTYPE *)ptr;

    pComponentPrivate->pLcmlBufHeader[INPUT_PORT_AACDEC] = pTemp_lcml;

    for (i=0; i<nIpBuf; i++) {
        pTemp = pComponentPrivate->pInputBufferList->pBufHdr[i];
        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pTemp->nAllocLen = nIpBufSize;
        pTemp->nFilledLen = nIpBufSize;
        pTemp->nVersion.s.nVersionMajor = AACDEC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = AACDEC_MINOR_VER;

        pComponentPrivate->nVersion = pTemp->nVersion.nVersion;

        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = NOT_USED_AACDEC;

        pTemp_lcml->pBufHdr = pTemp;
        pTemp_lcml->eDir = OMX_DirInput;
        pTemp_lcml->pOtherParams[i] = NULL;

        OMX_MALLOC_SIZE_DSPALIGN(pTemp_lcml->pIpParam,
                             sizeof(AACDEC_UAlgInBufParamStruct),
                             AACDEC_UAlgInBufParamStruct);
        pTemp_lcml->pIpParam->bLastBuffer = 0;
        pTemp_lcml->pIpParam->bConcealBuffer = 0;

        pTemp->nFlags = NORMAL_BUFFER_AACDEC;
        ((AACDEC_COMPONENT_PRIVATE *) pTemp->pPlatformPrivate)->pHandle = pHandle;

        OMX_PRBUFFER2(pComponentPrivate->dbg, "%d ::Comp: InBuffHeader[%d] = %p\n", __LINE__, i, pTemp);
        OMX_PRBUFFER2(pComponentPrivate->dbg, "%d ::Comp:  >>>> InputBuffHeader[%d]->pBuffer = %p\n",
                      __LINE__, i, pTemp->pBuffer);
        OMX_PRDSP2(pComponentPrivate->dbg, "%d ::Comp: Ip : pTemp_lcml[%d] = %p\n", __LINE__, i, pTemp_lcml);

        pTemp_lcml++;
    }

    size_lcml = nOpBuf * sizeof(AACD_LCML_BUFHEADERTYPE);
    OMX_MALLOC_SIZE(pTemp_lcml,size_lcml,AACD_LCML_BUFHEADERTYPE);
    pComponentPrivate->pLcmlBufHeader[OUTPUT_PORT_AACDEC] = pTemp_lcml;

    for (i=0; i<nOpBuf; i++) {
        pTemp = pComponentPrivate->pOutputBufferList->pBufHdr[i];
        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);

        pTemp->nAllocLen = nOpBufSize;
        pTemp->nVersion.s.nVersionMajor = AACDEC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = AACDEC_MINOR_VER;
        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = NOT_USED_AACDEC;

        pTemp_lcml->pBufHdr = pTemp;
        pTemp_lcml->eDir = OMX_DirOutput;
        pTemp_lcml->pOtherParams[i] = NULL;

        OMX_MALLOC_SIZE_DSPALIGN(pTemp_lcml->pOpParam,
                             sizeof(AACDEC_UAlgOutBufParamStruct),
                             AACDEC_UAlgOutBufParamStruct);
        pTemp_lcml->pOpParam->ulFrameCount = DONT_CARE;
        pTemp_lcml->pOpParam->isLastBuffer = 0;
		
        pTemp->nFlags = NORMAL_BUFFER_AACDEC;
        ((AACDEC_COMPONENT_PRIVATE *)pTemp->pPlatformPrivate)->pHandle = pHandle;
        OMX_PRBUFFER2(pComponentPrivate->dbg, "%d ::Comp:  >>>>>>>>>>>>> OutBuffHeader[%d] = %p\n",
                      __LINE__, i, pTemp);
        OMX_PRBUFFER2(pComponentPrivate->dbg, "%d ::Comp:  >>>> OutBuffHeader[%d]->pBuffer = %p\n",
                      __LINE__, i, pTemp->pBuffer);
        OMX_PRDSP2(pComponentPrivate->dbg, "%d ::Comp: Op : pTemp_lcml[%d] = %p\n", __LINE__, i, pTemp_lcml);
        pTemp_lcml++;
    }
    pComponentPrivate->bPortDefsAllocated = 1;
    if (pComponentPrivate->aacParams->eAACProfile == OMX_AUDIO_AACObjectHE){
        pComponentPrivate->SBR = 1;
    } else if (pComponentPrivate->aacParams->eAACProfile == OMX_AUDIO_AACObjectHE_PS){
        pComponentPrivate->parameteric_stereo = PARAMETRIC_STEREO_AACDEC;
    }

    OMX_MALLOC_SIZE_DSPALIGN(pComponentPrivate->pParams,sizeof (USN_AudioCodecParams),
                           USN_AudioCodecParams);

    OMX_MALLOC_SIZE_DSPALIGN(pComponentPrivate->AACDEC_UALGParam,sizeof (MPEG4AACDEC_UALGParams),
                           MPEG4AACDEC_UALGParams);

#ifdef __PERF_INSTRUMENTATION__
    pComponentPrivate->nLcml_nCntIp = 0;
    pComponentPrivate->nLcml_nCntOpReceived = 0;
#endif

    pComponentPrivate->bInitParamsInitialized = 1;

 EXIT:

    return eError;
}

/* ================================================================================= * */
/**
* @fn AacDec_StartCompThread() starts the component thread. This is internal
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
OMX_ERRORTYPE AacDec_StartCompThread(OMX_HANDLETYPE pComponent)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    AACDEC_COMPONENT_PRIVATE *pComponentPrivate =
        (AACDEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    int nRet = 0;
#ifdef UNDER_CE
    pthread_attr_t attr;
    memset(&attr, 0, sizeof(attr));
    attr.__inheritsched = PTHREAD_EXPLICIT_SCHED;
    attr.__schedparam.__sched_priority = OMX_AUDIO_DECODER_THREAD_PRIORITY;
#endif

    pComponentPrivate->lcml_nOpBuf = 0;
    pComponentPrivate->lcml_nIpBuf = 0;
    pComponentPrivate->app_nBuf = 0;
    pComponentPrivate->num_Op_Issued = 0;
    pComponentPrivate->num_Sent_Ip_Buff = 0;
    pComponentPrivate->num_Reclaimed_Op_Buff = 0;
    pComponentPrivate->bIsEOFSent = 0;
    pComponentPrivate->first_output_buf_rcv = 0;

    nRet = pipe (pComponentPrivate->dataPipe);
    if (0 != nRet) {
        AACDEC_OMX_ERROR_EXIT(eError, OMX_ErrorInsufficientResources,"Pipe Creation Failed");
    }

    nRet = pipe (pComponentPrivate->cmdPipe);
    if (0 != nRet) {
        AACDEC_OMX_ERROR_EXIT(eError, OMX_ErrorInsufficientResources,"Pipe Creation Failed");
    }

    nRet = pipe (pComponentPrivate->cmdDataPipe);
    if (0 != nRet) {
        AACDEC_OMX_ERROR_EXIT(eError, OMX_ErrorInsufficientResources,"Pipe Creation Failed");
    }


#ifdef UNDER_CE
    nRet = pthread_create (&(pComponentPrivate->ComponentThread), &attr, AACDEC_ComponentThread, pComponentPrivate);
#else
    nRet = pthread_create (&(pComponentPrivate->ComponentThread), NULL, AACDEC_ComponentThread, pComponentPrivate);
#endif
    if ((0 != nRet) || (!pComponentPrivate->ComponentThread)) {
        AACDEC_OMX_ERROR_EXIT(eError, OMX_ErrorInsufficientResources,"Thread Creation Failed");
    }

    pComponentPrivate->bCompThreadStarted = 1;

 EXIT:
    return eError;
}


/* ================================================================================= * */
/**
* @fn AACDEC_FreeCompResources() function frees the component resources.
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

OMX_ERRORTYPE AACDEC_FreeCompResources(OMX_HANDLETYPE pComponent)
{
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    AACDEC_COMPONENT_PRIVATE *pComponentPrivate = (AACDEC_COMPONENT_PRIVATE *)
        pHandle->pComponentPrivate;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nIpBuf=0, nOpBuf=0;
    int nRet=0;

    OMX_PRINT1(pComponentPrivate->dbg, "%d:::pComponentPrivate->bPortDefsAllocated = %ld\n", __LINE__,pComponentPrivate->bPortDefsAllocated);
    if (pComponentPrivate->bPortDefsAllocated) {
        nIpBuf = pComponentPrivate->pInputBufferList->numBuffers;
        nOpBuf = pComponentPrivate->pOutputBufferList->numBuffers;
    }
    OMX_PRDSP2(pComponentPrivate->dbg, "%d :: Closing pipes.....\n",__LINE__);

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

    if (pComponentPrivate->bPortDefsAllocated) {
        OMX_MEMFREE_STRUCT(pComponentPrivate->pPortDef[INPUT_PORT_AACDEC]);
        OMX_MEMFREE_STRUCT(pComponentPrivate->pPortDef[OUTPUT_PORT_AACDEC]);
        OMX_MEMFREE_STRUCT(pComponentPrivate->aacParams);
        OMX_MEMFREE_STRUCT(pComponentPrivate->pcmParams);
        OMX_MEMFREE_STRUCT(pComponentPrivate->pCompPort[INPUT_PORT_AACDEC]->pPortFormat);
        OMX_MEMFREE_STRUCT(pComponentPrivate->pCompPort[OUTPUT_PORT_AACDEC]->pPortFormat);
        OMX_MEMFREE_STRUCT(pComponentPrivate->pCompPort[INPUT_PORT_AACDEC]);
        OMX_MEMFREE_STRUCT(pComponentPrivate->pCompPort[OUTPUT_PORT_AACDEC]);
        OMX_MEMFREE_STRUCT(pComponentPrivate->sPortParam);
        OMX_MEMFREE_STRUCT(pComponentPrivate->pPriorityMgmt);
        OMX_MEMFREE_STRUCT(pComponentPrivate->pInputBufferList);
        OMX_MEMFREE_STRUCT(pComponentPrivate->pOutputBufferList);
        OMX_MEMFREE_STRUCT(pComponentPrivate->componentRole);
    }


    pComponentPrivate->bPortDefsAllocated = 0;

#ifndef UNDER_CE
    OMX_PRDSP2(pComponentPrivate->dbg, "\n\n FreeCompResources: Destroying threads.\n\n");
    pthread_mutex_destroy(&pComponentPrivate->InLoaded_mutex);
    pthread_cond_destroy(&pComponentPrivate->InLoaded_threshold);

    pthread_mutex_destroy(&pComponentPrivate->InIdle_mutex);
    pthread_cond_destroy(&pComponentPrivate->InIdle_threshold);

    pthread_mutex_destroy(&pComponentPrivate->AlloBuf_mutex);
    pthread_cond_destroy(&pComponentPrivate->AlloBuf_threshold);

    pthread_mutex_destroy(&pComponentPrivate->codecStop_mutex);
    pthread_cond_destroy(&pComponentPrivate->codecStop_threshold);

    pthread_mutex_destroy(&pComponentPrivate->codecFlush_mutex);
    pthread_cond_destroy(&pComponentPrivate->codecFlush_threshold);
#else
    OMX_DestroyEvent(&(pComponentPrivate->InLoaded_event));
    OMX_DestroyEvent(&(pComponentPrivate->InIdle_event));
    OMX_DestroyEvent(&(pComponentPrivate->AlloBuf_event));
#endif
    return eError;
}


/* ================================================================================= * */
/**
* @fn AACDEC_HandleCommand() function handles the command sent by the application.
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

OMX_U32 AACDEC_HandleCommand (AACDEC_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_U32 i;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;
    OMX_COMMANDTYPE command;
    OMX_STATETYPE commandedState;
    OMX_U32 commandData;
    OMX_HANDLETYPE pLcmlHandle = pComponentPrivate->pLcmlHandle;
    OMX_U32 ret = 0;
    OMX_U16 arr[10];
    OMX_U32 aParam[3] = {0};
    int inputPortFlag = 0;
    int outputPortFlag = 0;
    char *pArgs = "damedesuStr";

#ifdef RESOURCE_MANAGER_ENABLED
    OMX_ERRORTYPE rm_error = OMX_ErrorNone;
#endif

    OMX_PRINT1(pComponentPrivate->dbg, "%d :: >>> Entering HandleCommand Function\n",__LINE__);
    OMX_PRSTATE1(pComponentPrivate->dbg, "%d :: UTIL: pComponentPrivate->curState = %d\n",__LINE__,pComponentPrivate->curState);

    ret = read (pComponentPrivate->cmdPipe[0], &command, sizeof (command));
    if (ret == -1) {
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error in Reading from the Data pipe\n", __LINE__);
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
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error in Reading from the Data pipe\n", __LINE__);
        eError = OMX_ErrorHardware;
        pComponentPrivate->cbInfo.EventHandler (pHandle, 
                                                pHandle->pApplicationPrivate,
                                                OMX_EventError, 
                                                eError,
                                                OMX_TI_ErrorSevere,
                                                NULL);
        goto EXIT;
    }
    OMX_PRDSP1(pComponentPrivate->dbg, "---------------------------------------------\n");
    OMX_PRDSP1(pComponentPrivate->dbg, "%d :: command = %d\n",__LINE__,command);
    OMX_PRDSP1(pComponentPrivate->dbg, "%d :: commandData = %ld\n",__LINE__,commandData);
    OMX_PRDSP1(pComponentPrivate->dbg, "---------------------------------------------\n");

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

            OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error: Same State Given by \
                       Application\n",__LINE__);
        }
        else {


            switch(commandedState) {
            case OMX_StateIdle:

                OMX_PRDSP2(pComponentPrivate->dbg, "%d: HandleCommand: Cmd Idle \n",__LINE__);

                if (pComponentPrivate->curState == OMX_StateLoaded || pComponentPrivate->curState == OMX_StateWaitForResources) { 
                    LCML_CALLBACKTYPE cb;
                    LCML_DSP *pLcmlDsp;
                    char *p = "damedesuStr";
#ifdef __PERF_INSTRUMENTATION__
                    PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryStart | PERF_BoundarySetup);
#endif
                    if (pComponentPrivate->dasfmode == 1) {
                        pComponentPrivate->pPortDef[OUTPUT_PORT_AACDEC]->bEnabled= FALSE;
                        pComponentPrivate->pPortDef[OUTPUT_PORT_AACDEC]->bPopulated= FALSE;
                        if(pComponentPrivate->streamID == 0) {
                            OMX_ERROR4(pComponentPrivate->dbg, "**************************************\n");
                            OMX_ERROR4(pComponentPrivate->dbg, ":: Error = OMX_ErrorInsufficientResources\n");
                            OMX_ERROR4(pComponentPrivate->dbg, "**************************************\n");
                            eError = OMX_ErrorInsufficientResources;
                            pComponentPrivate->curState = OMX_StateInvalid;
                            pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                                   pHandle->pApplicationPrivate,
                                                                   OMX_EventError, 
                                                                   eError,
                                                                   OMX_TI_ErrorMajor, 
                                                                   "AM: No Stream ID Available");
                            goto EXIT;
                        }
                    }

                    OMX_PRCOMM2(pComponentPrivate->dbg, "%d:::pComponentPrivate->pPortDef[INPUT_PORT_AACDEC]->bPopulated = %d\n",
                                  __LINE__,pComponentPrivate->pPortDef[INPUT_PORT_AACDEC]->bPopulated);
                    OMX_PRCOMM2(pComponentPrivate->dbg, "%d:::pComponentPrivate->pPortDef[INPUT_PORT_AACDEC]->bEnabled = %d\n",
                                  __LINE__,pComponentPrivate->pPortDef[INPUT_PORT_AACDEC]->bEnabled);
                    OMX_PRCOMM2(pComponentPrivate->dbg, "%d:::pComponentPrivate->pPortDef[OUTPUT_PORT_AACDEC]->bPopulated = %d\n",
                                  __LINE__,pComponentPrivate->pPortDef[OUTPUT_PORT_AACDEC]->bPopulated);
                    OMX_PRCOMM2(pComponentPrivate->dbg, "%d:::pComponentPrivate->pPortDef[OUTPUT_PORT_AACDEC]->bEnabled = %d\n",
                                  __LINE__,pComponentPrivate->pPortDef[OUTPUT_PORT_AACDEC]->bEnabled);

                    if (pComponentPrivate->pPortDef[INPUT_PORT_AACDEC]->bPopulated &&
                        pComponentPrivate->pPortDef[INPUT_PORT_AACDEC]->bEnabled)  {
                        inputPortFlag = 1;
                    }

                    if (!pComponentPrivate->pPortDef[INPUT_PORT_AACDEC]->bPopulated &&
                        !pComponentPrivate->pPortDef[INPUT_PORT_AACDEC]->bEnabled) {
                        inputPortFlag = 1;
                    }

                    if (pComponentPrivate->pPortDef[OUTPUT_PORT_AACDEC]->bPopulated &&
                        pComponentPrivate->pPortDef[OUTPUT_PORT_AACDEC]->bEnabled) {
                        outputPortFlag = 1;
                    }

                    if (!pComponentPrivate->pPortDef[OUTPUT_PORT_AACDEC]->bPopulated &&
                        !pComponentPrivate->pPortDef[OUTPUT_PORT_AACDEC]->bEnabled) {
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
	
                    pLcmlHandle = (OMX_HANDLETYPE) AACDEC_GetLCMLHandle(pComponentPrivate);
                    if (pLcmlHandle == NULL) {
                        OMX_ERROR4(pComponentPrivate->dbg, ":: LCML Handle is NULL........exiting..\n");
                        pComponentPrivate->curState = OMX_StateInvalid;
                        pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                                pHandle->pApplicationPrivate,
                                                                OMX_EventError,
                                                                OMX_ErrorHardware,
                                                                OMX_TI_ErrorSevere,
                                                                NULL);
                        goto EXIT;
                    } 
                    pLcmlDsp = (((LCML_DSP_INTERFACE*)pLcmlHandle)->dspCodec);
                    eError = AACDEC_Fill_LCMLInitParams(pHandle, pLcmlDsp, arr);
                    if(eError != OMX_ErrorNone) {
                        OMX_ERROR4(pComponentPrivate->dbg, ":: Error returned from Fill_LCMLInitParams()\n");
                        pComponentPrivate->curState = OMX_StateInvalid;
                        pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                                pHandle->pApplicationPrivate,
                                                                OMX_EventError,
                                                                eError,
                                                                OMX_TI_ErrorSevere,
                                                                NULL);
                        goto EXIT;
                    }
 
                    pComponentPrivate->pLcmlHandle = (LCML_DSP_INTERFACE *)pLcmlHandle;
                    cb.LCML_Callback = (void *) AACDEC_LCML_Callback;

#ifndef UNDER_CE
                    eError = LCML_InitMMCodecEx(((LCML_DSP_INTERFACE *)pLcmlHandle)->pCodecinterfacehandle,
                                                p,&pLcmlHandle,(void *)p,&cb, (OMX_STRING)pComponentPrivate->sDeviceString);
                    if (eError != OMX_ErrorNone) {
                        OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error : InitMMCodec failed...>>>>>> \n",__LINE__);
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
#else
                    eError = LCML_InitMMCodec(((LCML_DSP_INTERFACE *)pLcmlHandle)->pCodecinterfacehandle,
                                              p,&pLcmlHandle,(void *)p,&cb);
                    if (eError != OMX_ErrorNone) {
                        OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error : InitMMCodec failed...>>>>>> \n",__LINE__);
                        goto EXIT;
                    }
#endif

#ifdef HASHINGENABLE
                    /* Enable the Hashing Code */
                    eError = LCML_SetHashingState(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle, OMX_TRUE);
                    if (eError != OMX_ErrorNone) {
                        OMX_ERROR4(pComponentPrivate->dbg, "Failed to set Mapping State\n");
                        goto EXIT;
                    }
#endif

#ifdef RESOURCE_MANAGER_ENABLED
				/* Need check the resource with RM */
					pComponentPrivate->rmproxyCallback.RMPROXY_Callback =
											(void *) AACDEC_ResourceManagerCallback;
                    if (pComponentPrivate->curState != OMX_StateWaitForResources){
                        rm_error = RMProxy_NewSendCommand(pHandle,
                                                       RMProxy_RequestResource,
                                                       OMX_AAC_Decoder_COMPONENT,
                                                       AACDEC_CPU_USAGE,
                                                       3456,
                                                       &(pComponentPrivate->rmproxyCallback));
                        if(rm_error == OMX_ErrorNone) {
                            /* resource is available */
#ifdef __PERF_INSTRUMENTATION__
							PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryComplete | PERF_BoundarySetup);
#endif
                            pComponentPrivate->curState = OMX_StateIdle;
                            rm_error = RMProxy_NewSendCommand(pHandle,
                                                           RMProxy_StateSet,
                                                           OMX_AAC_Decoder_COMPONENT,
                                                           OMX_StateIdle,
                                                           3456,
                                                           NULL);
                            pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                               pHandle->pApplicationPrivate,
                                                               OMX_EventCmdComplete,
                                                               OMX_CommandStateSet,
                                                               pComponentPrivate->curState,
                                                               NULL);
                        }
                        else if(rm_error == OMX_ErrorInsufficientResources) {
                            /* resource is not available, need set state to
                               OMX_StateWaitForResources */
                            pComponentPrivate->curState = OMX_StateWaitForResources;
                            pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                                   pHandle->pApplicationPrivate,
                                                                   OMX_EventCmdComplete,
                                                                   OMX_CommandStateSet,
                                                                   pComponentPrivate->curState,
                                                                   NULL);
                        }
                    }else{
                        rm_error = RMProxy_NewSendCommand(pHandle,
                                                       RMProxy_StateSet,
                                                       OMX_AAC_Decoder_COMPONENT,
                                                       OMX_StateIdle,
                                                       3456,
                                                       NULL);

                        pComponentPrivate->curState = OMX_StateIdle;
                        pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                               pHandle->pApplicationPrivate,
                                                               OMX_EventCmdComplete,
                                                               OMX_CommandStateSet,
                                                               pComponentPrivate->curState,
                                                               NULL);
                    }

#else
#ifdef __PERF_INSTRUMENTATION__
                    PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryComplete | PERF_BoundarySetup);
#endif
					pComponentPrivate->curState = OMX_StateIdle;
                    pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventCmdComplete,
                                                           OMX_CommandStateSet,
                                                           pComponentPrivate->curState,
                                                           NULL);
#endif
 
                } else if (pComponentPrivate->curState == OMX_StateExecuting) {

#ifdef __PERF_INSTRUMENTATION__
                    PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryComplete | PERF_BoundarySteadyState);
#endif 
                    OMX_PRDSP2(pComponentPrivate->dbg, "%d :: In HandleCommand: Stopping the codec\n",__LINE__);
                    pComponentPrivate->bDspStoppedWhileExecuting = OMX_TRUE;
                    pComponentPrivate->bNoIdleOnStop = OMX_TRUE;
			if (pComponentPrivate->codecStop_waitingsignal == 0){
                        pthread_mutex_lock(&pComponentPrivate->codecStop_mutex);
                    }
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               MMCodecControlStop,(void *)pArgs);
			 if (pComponentPrivate->codecStop_waitingsignal == 0){
                        pthread_cond_wait(&pComponentPrivate->codecStop_threshold, &pComponentPrivate->codecStop_mutex);
                        pComponentPrivate->codecStop_waitingsignal = 0;
                        pthread_mutex_unlock(&pComponentPrivate->codecStop_mutex);
                    }
                    if(eError != OMX_ErrorNone) {
                        OMX_ERROR4(pComponentPrivate->dbg, ": Error Occurred in Codec Stop..\n");
                        pComponentPrivate->curState = OMX_StateInvalid;
                        pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                                pHandle->pApplicationPrivate,
                                                                OMX_EventError,
                                                                eError,
                                                                OMX_TI_ErrorSevere,
                                                                NULL);
                        goto EXIT;
                    }
#ifdef HASHINGENABLE
                    /*Hashing Change*/
                    pLcmlHandle = (LCML_DSP_INTERFACE*)pComponentPrivate->pLcmlHandle;
                    eError = LCML_FlushHashes(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle);
                    if (eError != OMX_ErrorNone) {
                        OMX_ERROR4(pComponentPrivate->dbg, "Error occurred in Codec mapping flush!\n");
                        break;
                    }
#endif
                } else if (pComponentPrivate->curState == OMX_StatePause) { 
                    char *pArgs = "damedesuStr";
#ifdef HASHINGENABLE
                    /*Hashing Change*/
                    pLcmlHandle = (LCML_DSP_INTERFACE*)pComponentPrivate->pLcmlHandle;
                    eError = LCML_FlushHashes(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle);
                    if (eError != OMX_ErrorNone) {
                        OMX_ERROR4(pComponentPrivate->dbg, "Error occurred in Codec mapping flush!\n");
                        break;
                    }
#endif
#ifdef __PERF_INSTRUMENTATION__
                    PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryComplete | PERF_BoundarySteadyState);
#endif
                    OMX_PRDSP2(pComponentPrivate->dbg, "%d :: Comp: Stop Command Received\n",__LINE__);
                    OMX_PRDSP2(pComponentPrivate->dbg, "%d: AACDECUTILS::About to call LCML_ControlCodec\n",__LINE__);
                    pComponentPrivate->bNoIdleOnStop = OMX_TRUE;
			if (pComponentPrivate->codecStop_waitingsignal == 0){
                        pthread_mutex_lock(&pComponentPrivate->codecStop_mutex);
                    }
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               MMCodecControlStop,(void *)pArgs);
			 if (pComponentPrivate->codecStop_waitingsignal == 0){
                        pthread_cond_wait(&pComponentPrivate->codecStop_threshold, &pComponentPrivate->codecStop_mutex);
                        pComponentPrivate->codecStop_waitingsignal = 0; // reset the wait condition for next time
                        pthread_mutex_unlock(&pComponentPrivate->codecStop_mutex);
                    }
                    if(eError != OMX_ErrorNone) {
                        OMX_ERROR4(pComponentPrivate->dbg, ": Error Occurred in Codec Stop..\n");
                        pComponentPrivate->curState = OMX_StateInvalid;
                        pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                                pHandle->pApplicationPrivate,
                                                                OMX_EventError,
                                                                eError,
                                                                OMX_TI_ErrorSevere,
                                                                NULL);
                        goto EXIT;
                    }
                    AACDEC_STATEPRINT("****************** Component State Set to Idle\n\n");
                    pComponentPrivate->curState = OMX_StateIdle;
#ifdef RESOURCE_MANAGER_ENABLED
                    rm_error = RMProxy_NewSendCommand(pHandle, RMProxy_StateSet, OMX_AAC_Decoder_COMPONENT, OMX_StateIdle, 3456, NULL);
#endif
                    OMX_PRDSP2(pComponentPrivate->dbg, "%d :: The component is stopped\n",__LINE__);
                    pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                            pHandle->pApplicationPrivate,
                                                            OMX_EventCmdComplete,
                                                            OMX_CommandStateSet,
                                                            pComponentPrivate->curState,
                                                            NULL);
                } else { 
                    OMX_ERROR4(pComponentPrivate->dbg, "%d: Comp: Sending ErrorNotification: Invalid State\n",__LINE__);
                    pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventError,
                                                           OMX_ErrorIncorrectStateTransition, 
                                                           OMX_TI_ErrorMinor,
                                                           "Invalid State Error");
                }
                break;

            case OMX_StateExecuting:

                OMX_PRDSP2(pComponentPrivate->dbg, "%d: HandleCommand: Cmd Executing \n",__LINE__);
                if (pComponentPrivate->curState == OMX_StateIdle) {
                  if(!pComponentPrivate->bConfigData){
/*  if running under Android (file mode), these values are not available during this state transition.
    We will have to set the codec config parameters after receiving the first buffer that carries
    the config data */
                    
                    char *pArgs = "damedesuStr";
                    OMX_U32 pValues[4];
                    OMX_U32 pValues1[4];
 
                    pComponentPrivate->AACDEC_UALGParam->size = sizeof(MPEG4AACDEC_UALGParams);
                    if(pComponentPrivate->dasfmode == 1) {
                        pComponentPrivate->pParams->unAudioFormat = STEREO_NONINTERLEAVED_STREAM_AACDEC;
                        if(pComponentPrivate->aacParams->nChannels == OMX_AUDIO_ChannelModeMono) {
                                pComponentPrivate->pParams->unAudioFormat = MONO_STREAM_AACDEC;
                                OMX_PRINT2(pComponentPrivate->dbg, "MONO MODE\n");
                        }

                       pComponentPrivate->pParams->ulSamplingFreq = pComponentPrivate->aacParams->nSampleRate;
                       pComponentPrivate->pParams->unUUID = pComponentPrivate->streamID;
 
                        OMX_PRINT2(pComponentPrivate->dbg, "%d ::pComponentPrivate->pParams->unAudioFormat   = %d\n",
                                          __LINE__,pComponentPrivate->pParams->unAudioFormat);
                        OMX_PRINT2(pComponentPrivate->dbg, "%d ::pComponentPrivate->pParams->ulSamplingFreq  = %ld\n",
                                          __LINE__,pComponentPrivate->aacParams->nSampleRate);

                        pValues[0] = USN_STRMCMD_SETCODECPARAMS;
                        pValues[1] = (OMX_U32)pComponentPrivate->pParams;
                        pValues[2] = sizeof(USN_AudioCodecParams);
                        eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                   EMMCodecControlStrmCtrl,(void *)pValues);
                        if(eError != OMX_ErrorNone) {
                            OMX_ERROR4(pComponentPrivate->dbg, "%d: Error Occurred in Codec StreamControl..\n",__LINE__);
                            pComponentPrivate->curState = OMX_StateInvalid;
                            pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                                    pHandle->pApplicationPrivate,
                                                                    OMX_EventError,
                                                                    eError,
                                                                    OMX_TI_ErrorSevere,
                                                                    NULL);
                            goto EXIT;
                        }
                    }

                    OMX_PRINT2(pComponentPrivate->dbg, "%d :: pComponentPrivate->dualMonoMode %lu \n",__LINE__,pComponentPrivate->dualMonoMode);
                    OMX_PRINT2(pComponentPrivate->dbg, "%d :: pComponentPrivate->parameteric_stereo  %lu \n",
                                   __LINE__,pComponentPrivate->parameteric_stereo);
                    OMX_PRINT2(pComponentPrivate->dbg, "%d :: pComponentPrivate->SBR  %lu \n",__LINE__,pComponentPrivate->SBR);
                    if(pComponentPrivate->parameteric_stereo == PARAMETRIC_STEREO_AACDEC){
                        if(pComponentPrivate->dasfmode == 1){
                            pComponentPrivate->AACDEC_UALGParam->lOutputFormat    = EAUDIO_BLOCK;
                        }else{
                            if(pComponentPrivate->pcmParams->bInterleaved){
                                pComponentPrivate->AACDEC_UALGParam->lOutputFormat    = EAUDIO_INTERLEAVED;
                            }else{
                                pComponentPrivate->AACDEC_UALGParam->lOutputFormat    = EAUDIO_BLOCK;
                            }
                        }
                        pComponentPrivate->AACDEC_UALGParam->iEnablePS        = 1;/*Added for eAAC*/
                        pComponentPrivate->AACDEC_UALGParam->dualMonoMode     = pComponentPrivate->dualMonoMode;
                        pComponentPrivate->AACDEC_UALGParam->lSamplingRateIdx = AACDec_GetSampleRateIndexL(pComponentPrivate->aacParams->nSampleRate);
                        pComponentPrivate->AACDEC_UALGParam->bRawFormat       = 0;
                        if(pComponentPrivate->aacParams->eAACStreamFormat == OMX_AUDIO_AACStreamFormatRAW){
                            pComponentPrivate->AACDEC_UALGParam->bRawFormat       = 1;
                        }
                        pComponentPrivate->AACDEC_UALGParam->DownSampleSbr    = 1;
                        }else{
                            OMX_PRINT2(pComponentPrivate->dbg, "Inside the non parametric stereo\n");
                            if(pComponentPrivate->dasfmode == 1){
                                pComponentPrivate->AACDEC_UALGParam->lOutputFormat    = EAUDIO_BLOCK;
                            }else{
                                if(pComponentPrivate->pcmParams->bInterleaved){
                                    pComponentPrivate->AACDEC_UALGParam->lOutputFormat    = EAUDIO_INTERLEAVED;
                                }else{
                                    pComponentPrivate->AACDEC_UALGParam->lOutputFormat    = EAUDIO_BLOCK;
                                }
                            }
                            pComponentPrivate->AACDEC_UALGParam->iEnablePS        = 0;
                            pComponentPrivate->AACDEC_UALGParam->dualMonoMode     = pComponentPrivate->dualMonoMode;
                            pComponentPrivate->AACDEC_UALGParam->lSamplingRateIdx = AACDec_GetSampleRateIndexL(pComponentPrivate->aacParams->nSampleRate);
                            pComponentPrivate->AACDEC_UALGParam->bRawFormat       = 0;
                            if(pComponentPrivate->aacParams->eAACStreamFormat == OMX_AUDIO_AACStreamFormatRAW){
                                pComponentPrivate->AACDEC_UALGParam->bRawFormat       = 1;
                            }
                            pComponentPrivate->AACDEC_UALGParam->DownSampleSbr    = 0;
                            if(pComponentPrivate->SBR ){
                                pComponentPrivate->AACDEC_UALGParam->DownSampleSbr    = 1;
                            }
                        }

                        OMX_PRINT2(pComponentPrivate->dbg, "%d::pComponentPrivate->AACDEC_UALGParam->lOutputFormat::%ld\n",
                                      __LINE__,pComponentPrivate->AACDEC_UALGParam->lOutputFormat);
                        OMX_PRINT2(pComponentPrivate->dbg, "%d::pComponentPrivate->AACDEC_UALGParam->DownSampleSbr::%ld\n",
                                      __LINE__,pComponentPrivate->AACDEC_UALGParam->DownSampleSbr);
                        OMX_PRINT2(pComponentPrivate->dbg, "%d::pComponentPrivate->AACDEC_UALGParam->iEnablePS::%ld\n",
                                      __LINE__,pComponentPrivate->AACDEC_UALGParam->iEnablePS);
                        OMX_PRINT2(pComponentPrivate->dbg, "%d::pComponentPrivate->AACDEC_UALGParam->lSamplingRateIdx::%ld\n",
                                      __LINE__,pComponentPrivate->AACDEC_UALGParam->lSamplingRateIdx);
                        OMX_PRINT2(pComponentPrivate->dbg, "%d::pComponentPrivate->SBR::%lu\n",__LINE__,pComponentPrivate->SBR);
                        OMX_PRINT2(pComponentPrivate->dbg, "%d::pComponentPrivate->AACDEC_UALGParam->dualMonoMode::%ld\n",
                                      __LINE__,pComponentPrivate->AACDEC_UALGParam->dualMonoMode);
                        OMX_PRINT2(pComponentPrivate->dbg, "%d::pComponentPrivate->AACDEC_UALGParam->bRawFormat::%ld\n",
                                      __LINE__,pComponentPrivate->AACDEC_UALGParam->bRawFormat);
                        pValues1[0] = IUALG_CMD_SETSTATUS;
                        pValues1[1] = (OMX_U32)pComponentPrivate->AACDEC_UALGParam;
                        pValues1[2] = sizeof(MPEG4AACDEC_UALGParams);

                        eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                   EMMCodecControlAlgCtrl,(void *)pValues1);
                        if(eError != OMX_ErrorNone) {
                            OMX_ERROR4(pComponentPrivate->dbg, "%d: Error Occurred in Codec StreamControl..\n",__LINE__);
                            pComponentPrivate->curState = OMX_StateInvalid;
                            pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                                   pHandle->pApplicationPrivate,
                                                                   OMX_EventError, 
                                                                   eError,
                                                                   OMX_TI_ErrorSevere, 
                                                                   NULL);
                            goto EXIT;
                        }
                        OMX_PRDSP2(pComponentPrivate->dbg, "%d :: Algcontrol has been sent to DSP\n",__LINE__);
                        pComponentPrivate->bDspStoppedWhileExecuting = OMX_FALSE;
                        eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                   EMMCodecControlStart,(void *)pArgs);
                        if(eError != OMX_ErrorNone) {
                            OMX_ERROR4(pComponentPrivate->dbg, "%d: Error Occurred in Codec Start..\n",__LINE__);
                            goto EXIT;
                        }
                    }
                } else if (pComponentPrivate->curState == OMX_StatePause) {
                    char *pArgs = "damedesuStr";
                    OMX_PRDSP2(pComponentPrivate->dbg, "%d: UTILS: Resume Command Came from App\n",__LINE__);
                    OMX_PRDSP2(pComponentPrivate->dbg, "%d: UTILS::About to call LCML_ControlCodec\n",__LINE__);
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               EMMCodecControlStart,(void *)pArgs);
                    if (eError != OMX_ErrorNone) {
                        OMX_ERROR4(pComponentPrivate->dbg, "Error While Resuming the codec\n");
                        pComponentPrivate->curState = OMX_StateInvalid;
                        pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                                pHandle->pApplicationPrivate,
                                                                OMX_EventError,
                                                                eError,
                                                                OMX_TI_ErrorSevere,
                                                                NULL);
                        goto EXIT;
                    }

                    for (i=0; i < pComponentPrivate->nNumInputBufPending; i++) {
                        OMX_PRBUFFER2(pComponentPrivate->dbg, "%d pComponentPrivate->pInputBufHdrPending[%lu] = %d\n",__LINE__,i,
                                      pComponentPrivate->pInputBufHdrPending[i] != NULL);
                        if (pComponentPrivate->pInputBufHdrPending[i] != NULL) {
                            AACD_LCML_BUFHEADERTYPE *pLcmlHdr;
                            AACDEC_GetCorresponding_LCMLHeader(pComponentPrivate,
                                                      pComponentPrivate->pInputBufHdrPending[i]->pBuffer,
                                                      OMX_DirInput,
                                                      &pLcmlHdr);
                            AACDEC_SetPending(pComponentPrivate,
                                              pComponentPrivate->pInputBufHdrPending[i],
                                              OMX_DirInput,
                                              __LINE__);
                            OMX_PRBUFFER2(pComponentPrivate->dbg,
                                          "Calling LCML_QueueBuffer Line %d\n",__LINE__);
                            eError = LCML_QueueBuffer(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                      EMMCodecInputBuffer,
                                                      pComponentPrivate->pInputBufHdrPending[i]->pBuffer,
                                                      pComponentPrivate->pInputBufHdrPending[i]->nAllocLen,
                                                      pComponentPrivate->pInputBufHdrPending[i]->nFilledLen,
                                                      (OMX_U8 *) pLcmlHdr->pIpParam,
                                                      sizeof(AACDEC_UAlgInBufParamStruct),
                                                      NULL);
                            if(eError != OMX_ErrorNone) {
                                OMX_ERROR4(pComponentPrivate->dbg, ": Error Occurred in LCML QueueBuffer for input\n");
                                pComponentPrivate->curState = OMX_StateInvalid;
                                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                                pHandle->pApplicationPrivate,
                                                                OMX_EventError,
                                                                eError,
                                                                OMX_TI_ErrorSevere,
                                                                NULL);
                                goto EXIT;
                            }
                        }
                    }
                    pComponentPrivate->nNumInputBufPending = 0;
                    for (i=0; i < pComponentPrivate->nNumOutputBufPending; i++) {
                        OMX_PRDSP2(pComponentPrivate->dbg, "%d pComponentPrivate->pOutputBufHdrPending[%lu] = %p\n",__LINE__,i,
                                      pComponentPrivate->pOutputBufHdrPending[i]);
                        if (pComponentPrivate->pOutputBufHdrPending[i] != NULL) {
                            AACD_LCML_BUFHEADERTYPE *pLcmlHdr;
                            AACDEC_GetCorresponding_LCMLHeader(pComponentPrivate,pComponentPrivate->pOutputBufHdrPending[i]->pBuffer,
                                                               OMX_DirOutput, &pLcmlHdr);
                                AACDEC_SetPending(pComponentPrivate,pComponentPrivate->pOutputBufHdrPending[i],OMX_DirOutput,__LINE__);
                                OMX_PRDSP2(pComponentPrivate->dbg, "Calling LCML_QueueBuffer Line %d\n",__LINE__);
                                eError = LCML_QueueBuffer(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                      EMMCodecOuputBuffer,
                                                      pComponentPrivate->pOutputBufHdrPending[i]->pBuffer,
                                                      pComponentPrivate->pOutputBufHdrPending[i]->nAllocLen,
                                                      0,
                                                      (OMX_U8 *) pLcmlHdr->pOpParam,
                                                      sizeof(AACDEC_UAlgOutBufParamStruct),
                                                      NULL);
                                if(eError != OMX_ErrorNone) {
                                OMX_ERROR4(pComponentPrivate->dbg, ": Error Occurred in LCML QueueBuffer for output\n");
                                pComponentPrivate->curState = OMX_StateInvalid;
                                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                                pHandle->pApplicationPrivate,
                                                                OMX_EventError,
                                                                eError,
                                                                OMX_TI_ErrorSevere,
                                                                NULL);
                                goto EXIT;
                            }
                        }
                    }
                    pComponentPrivate->nNumOutputBufPending = 0;
                } else {
                    pComponentPrivate->cbInfo.EventHandler ( pHandle,
                                                             pHandle->pApplicationPrivate,
                                                             OMX_EventError,
                                                             OMX_ErrorIncorrectStateTransition,
                                                             OMX_TI_ErrorMinor,
                                                             "Invalid State");
                    OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error: Invalid State Given by Application\n",__LINE__);
                    goto EXIT;
                }
                OMX_PRSTATE2(pComponentPrivate->dbg, "****************** Component State Set to Executing\n\n");
#ifdef RESOURCE_MANAGER_ENABLED
                rm_error = RMProxy_NewSendCommand(pHandle, 
                                                  RMProxy_StateSet, 
                                                  OMX_AAC_Decoder_COMPONENT, 
                                                  OMX_StateExecuting, 
                                                  3456, 
                                                  NULL);
#endif
                pComponentPrivate->curState = OMX_StateExecuting;
#ifdef __PERF_INSTRUMENTATION__
                PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryStart | PERF_BoundarySteadyState);
#endif
                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,
                                                        OMX_CommandStateSet,
                                                        pComponentPrivate->curState,
                                                        NULL);
                break;

            case OMX_StateLoaded: 
                OMX_PRDSP2(pComponentPrivate->dbg, "%d: HandleCommand: Cmd Loaded\n",__LINE__);
                if (pComponentPrivate->curState == OMX_StateWaitForResources ){
                    OMX_PRSTATE2(pComponentPrivate->dbg, "****************** Component State Set to Loaded\n\n");
#ifdef __PERF_INSTRUMENTATION__
                    PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryStart | PERF_BoundaryCleanup);
#endif
                    pComponentPrivate->curState = OMX_StateLoaded;
#ifdef __PERF_INSTRUMENTATION__
                    PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryComplete | PERF_BoundaryCleanup);
#endif
                    pComponentPrivate->cbInfo.EventHandler ( pHandle,
                                                             pHandle->pApplicationPrivate,
                                                             OMX_EventCmdComplete,
                                                             OMX_CommandStateSet,
                                                             pComponentPrivate->curState,
                                                             NULL);
                    OMX_PRDSP2(pComponentPrivate->dbg, "%d :: Tansitioning from WaitFor to Loaded\n",__LINE__);
                    break;
                }

                if (pComponentPrivate->curState != OMX_StateIdle) {
                    pComponentPrivate->cbInfo.EventHandler ( pHandle,
                                                             pHandle->pApplicationPrivate,
                                                             OMX_EventError,
                                                             OMX_ErrorIncorrectStateTransition,
                                                             OMX_TI_ErrorMinor,
                                                             "Incorrect State Transition");
                    OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error: Invalid State Given by \
                       Application\n",__LINE__);
                    goto EXIT;
                }
#ifdef __PERF_INSTRUMENTATION__
                PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryStart | PERF_BoundaryCleanup);
#endif

                OMX_PRSTATE2(pComponentPrivate->dbg,
                             "%d: AACDECUTILS::Current State = %d\n",
                             __LINE__,
                             pComponentPrivate->curState);
                OMX_PRBUFFER2(pComponentPrivate->dbg,
                              "pComponentPrivate->pInputBufferList->numBuffers = %lu\n",
                              pComponentPrivate->pInputBufferList->numBuffers);
                OMX_PRBUFFER2(pComponentPrivate->dbg,
                              "pComponentPrivate->pOutputBufferList->numBuffers = %lu\n",
                              pComponentPrivate->pOutputBufferList->numBuffers);

                if (pComponentPrivate->pInputBufferList->numBuffers || pComponentPrivate->pOutputBufferList->numBuffers) {
                    pComponentPrivate->InIdle_goingtoloaded = 1;
#ifndef UNDER_CE
                    pthread_mutex_lock(&pComponentPrivate->InIdle_mutex);
                    pthread_cond_wait(&pComponentPrivate->InIdle_threshold, &pComponentPrivate->InIdle_mutex);
                    pthread_mutex_unlock(&pComponentPrivate->InIdle_mutex);
#else
                    OMX_WaitForEvent(&(pComponentPrivate->InIdle_event));
#endif
                    pComponentPrivate->bLoadedCommandPending = OMX_FALSE;
                }

                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                           EMMCodecControlDestroy,(void *)pArgs);
#ifdef __PERF_INSTRUMENTATION__
                PERF_SendingCommand(pComponentPrivate->pPERF, -1, 0, PERF_ModuleComponent);
#endif

                eError = EXIT_COMPONENT_THRD;
                pComponentPrivate->bInitParamsInitialized = 0;
                break;

            case OMX_StatePause:
                OMX_PRSTATE2(pComponentPrivate->dbg, "%d: HandleCommand: Cmd Pause: Cur State = %d\n",__LINE__,
                              pComponentPrivate->curState);

                if ((pComponentPrivate->curState != OMX_StateExecuting) &&
                    (pComponentPrivate->curState != OMX_StateIdle)) {
                    pComponentPrivate->cbInfo.EventHandler ( pHandle,
                                                             pHandle->pApplicationPrivate,
                                                             OMX_EventError,
                                                             OMX_ErrorIncorrectStateTransition,
                                                             OMX_TI_ErrorMinor,
                                                             "Incorrect State Transition");
                    OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error: Invalid State Given by \
                       Application\n",__LINE__);
                    goto EXIT;
                }
#ifdef __PERF_INSTRUMENTATION__
                PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryComplete | PERF_BoundarySteadyState);
#endif

                OMX_PRDSP2(pComponentPrivate->dbg, "%d: AACDECUTILS::About to call LCML_ControlCodec\n",__LINE__);
                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                           EMMCodecControlPause,
                                           (void *)pArgs);
                if (eError != OMX_ErrorNone) {
                    pComponentPrivate->curState = OMX_StateInvalid;
                    pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                            pHandle->pApplicationPrivate,
                                                            OMX_EventError,
                                                            eError,
                                                            OMX_TI_ErrorSevere,
                                                            NULL);
                    OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error pausing codec\n",__LINE__);
                    goto EXIT;
                }
#ifdef RESOURCE_MANAGER_ENABLED
                    rm_error = RMProxy_NewSendCommand(pHandle, 
                                                      RMProxy_StateSet, 
                                                      OMX_AAC_Decoder_COMPONENT, 
                                                      OMX_StateWaitForResources, 
                                                      3456,
                                                      NULL);
#endif
                OMX_PRDSP2(pComponentPrivate->dbg, "%d :: Component: Codec Is Paused\n",__LINE__);
                break;

            case OMX_StateWaitForResources:
                OMX_PRDSP2(pComponentPrivate->dbg, "%d: HandleCommand: Cmd : OMX_StateWaitForResources\n",__LINE__);
                if (pComponentPrivate->curState == OMX_StateLoaded) {
#ifdef RESOURCE_MANAGER_ENABLED
                    rm_error = RMProxy_NewSendCommand(pHandle, 
                                                      RMProxy_StateSet, 
                                                      OMX_AAC_Decoder_COMPONENT, 
                                                      OMX_StateWaitForResources, 
                                                      3456,
                                                      NULL);
#endif
                    pComponentPrivate->curState = OMX_StateWaitForResources;
                    OMX_PRDSP2(pComponentPrivate->dbg, "%d: Transitioning from Loaded to OMX_StateWaitForResources\n",__LINE__);
                    pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                            pHandle->pApplicationPrivate,
                                                            OMX_EventCmdComplete,
                                                            OMX_CommandStateSet,
                                                            pComponentPrivate->curState,
                                                            NULL);
                }
                else {
                    pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                            pHandle->pApplicationPrivate,
                                                            OMX_EventError,
                                                            OMX_ErrorIncorrectStateTransition,
                                                            OMX_TI_ErrorMinor,
                                                            NULL);
                   OMX_ERROR4(pComponentPrivate->dbg, "%d :: state transition error\n",__LINE__);
                }
                break;

            case OMX_StateInvalid:
                OMX_PRDSP2(pComponentPrivate->dbg, "%d: HandleCommand: Cmd OMX_StateInvalid:\n",__LINE__);
                if (pComponentPrivate->curState != OMX_StateWaitForResources &&
                    pComponentPrivate->curState != OMX_StateInvalid &&
                    pComponentPrivate->curState != OMX_StateLoaded) {

                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                EMMCodecControlDestroy, (void *)pArgs);
                }

                pComponentPrivate->curState = OMX_StateInvalid;
                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventError,
                                                        OMX_ErrorInvalidState,
                                                        OMX_TI_ErrorSevere,
                                                        NULL);
                OMX_ERROR4(pComponentPrivate->dbg, "%d :: Component: Invalid State\n",__LINE__);

                AACDEC_CleanupInitParams(pHandle);

                break;

            case OMX_StateMax:
                OMX_PRDSP2(pComponentPrivate->dbg, "%d: HandleCommand: Cmd OMX_StateMax::\n",__LINE__);
                break;
            } /* End of Switch */
        }
    }
    else if (command == OMX_CommandMarkBuffer) {
        if(!pComponentPrivate->pMarkBuf) {
            pComponentPrivate->pMarkBuf = (OMX_MARKTYPE *)(commandData);
        }
    } else if (command == OMX_CommandPortDisable) {
        if (!pComponentPrivate->bDisableCommandPending) {
            if(commandData == 0x0){
                for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
                    OMX_PRBUFFER2(pComponentPrivate->dbg, "pComponentPrivate->pInputBufferList->bBufferPending[%lu] = %lu\n",i,
                                  pComponentPrivate->pInputBufferList->bBufferPending[i]);
                    if (AACDEC_IsPending(pComponentPrivate,pComponentPrivate->pInputBufferList->pBufHdr[i],OMX_DirInput)) {
#ifdef __PERF_INSTRUMENTATION__
                        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                          PREF(pComponentPrivate->pInputBufferList->pBufHdr[i], pBuffer),
                                          0,
                                          PERF_ModuleHLMM);
#endif
                        OMX_PRBUFFER2(pComponentPrivate->dbg, "Forcing EmptyBufferDone\n");
                        pComponentPrivate->cbInfo.EmptyBufferDone (pComponentPrivate->pHandle,
                                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                                   pComponentPrivate->pInputBufferList->pBufHdr[i]);
                        pComponentPrivate->nEmptyBufferDoneCount++;
                        SignalIfAllBuffersAreReturned(pComponentPrivate);
                    }
                }
                pComponentPrivate->pPortDef[INPUT_PORT_AACDEC]->bEnabled = OMX_FALSE;
            }
            if(commandData == -1){
                pComponentPrivate->pPortDef[INPUT_PORT_AACDEC]->bEnabled = OMX_FALSE;
            }
            if(commandData == 0x1 || commandData == -1){
                pComponentPrivate->pPortDef[OUTPUT_PORT_AACDEC]->bEnabled = OMX_FALSE;
            }
        }
        OMX_PRCOMM2(pComponentPrivate->dbg, "commandData = %ld\n",commandData);
        OMX_PRCOMM2(pComponentPrivate->dbg, "pComponentPrivate->pPortDef[INPUT_PORT_AACDEC]->bPopulated = %d\n",
                      pComponentPrivate->pPortDef[INPUT_PORT_AACDEC]->bPopulated);
        OMX_PRCOMM2(pComponentPrivate->dbg, "pComponentPrivate->pPortDef[OUTPUT_PORT_AACDEC]->bPopulated = %d\n",
                      pComponentPrivate->pPortDef[OUTPUT_PORT_AACDEC]->bPopulated);
        OMX_PRCOMM2(pComponentPrivate->dbg, "pComponentPrivate->bDisableCommandPending = %ld\n",pComponentPrivate->bDisableCommandPending);

        if(commandData == 0x0) {
            if(!pComponentPrivate->pPortDef[INPUT_PORT_AACDEC]->bPopulated){

                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,
                                                        OMX_CommandPortDisable,
                                                        INPUT_PORT_AACDEC,
                                                        NULL);
                pComponentPrivate->bDisableCommandPending = 0;

            }
            else {
                pComponentPrivate->bDisableCommandPending = 1;
                pComponentPrivate->bDisableCommandParam = commandData;
            }
        }

        if(commandData == 0x1) {
            if (!pComponentPrivate->pPortDef[OUTPUT_PORT_AACDEC]->bPopulated){
                OMX_PRCOMM2(pComponentPrivate->dbg, "Disable Output port completed\n\n");
                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,
                                                        OMX_CommandPortDisable,
                                                        OUTPUT_PORT_AACDEC,
                                                        NULL);
                
                pComponentPrivate->bDisableCommandPending = 0;
            }
            else {
                pComponentPrivate->bDisableCommandPending = 1;
                pComponentPrivate->bDisableCommandParam = commandData;
            }
        }
        
        if(commandData == -1) {
            if (!pComponentPrivate->pPortDef[INPUT_PORT_AACDEC]->bPopulated &&
                !pComponentPrivate->pPortDef[OUTPUT_PORT_AACDEC]->bPopulated){

                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,
                                                        OMX_CommandPortDisable,
                                                        INPUT_PORT_AACDEC,
                                                        NULL);

                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,
                                                        OMX_CommandPortDisable,
                                                        OUTPUT_PORT_AACDEC,
                                                        NULL);
                pComponentPrivate->bDisableCommandPending = 0;
            }
            else {
                pComponentPrivate->bDisableCommandPending = 1;
                pComponentPrivate->bDisableCommandParam = commandData;
            }
            OMX_PRINT2(pComponentPrivate->dbg, "pComponentPrivate->bDisableCommandParam = %ld\n", pComponentPrivate->bDisableCommandParam);
        }
    }
    else if (command == OMX_CommandPortEnable) {
        OMX_PRCOMM2(pComponentPrivate->dbg, "received port enable command\n");
        if(!pComponentPrivate->bEnableCommandPending) {
            if(commandData == 0x0 || commandData == -1){

                OMX_PRCOMM2(pComponentPrivate->dbg, "setting input port to enabled\n");
                pComponentPrivate->pPortDef[INPUT_PORT_AACDEC]->bEnabled = OMX_TRUE;
                OMX_PRCOMM2(pComponentPrivate->dbg, "pComponentPrivate->pPortDef[INPUT_PORT_AACDEC]->bEnabled = %d\n",
                              pComponentPrivate->pPortDef[INPUT_PORT_AACDEC]->bEnabled);

                if(pComponentPrivate->AlloBuf_waitingsignal){
                    pComponentPrivate->AlloBuf_waitingsignal = 0;
                }
            }
            if(commandData == 0x1 || commandData == -1){
                char *pArgs = "damedesuStr";

                
                if(pComponentPrivate->curState == OMX_StateExecuting) {
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               EMMCodecControlStart,(void *)pArgs);
                    pComponentPrivate->bDspStoppedWhileExecuting = OMX_FALSE;
                }
                OMX_PRCOMM2(pComponentPrivate->dbg, "setting output port to enabled\n");
                pComponentPrivate->pPortDef[OUTPUT_PORT_AACDEC]->bEnabled = OMX_TRUE;
                OMX_PRCOMM2(pComponentPrivate->dbg, "pComponentPrivate->pPortDef[OUTPUT_PORT_AACDEC]->bEnabled = %d\n",
                              pComponentPrivate->pPortDef[OUTPUT_PORT_AACDEC]->bEnabled);
            }
        }

        if(commandData == 0x0){
            if (pComponentPrivate->curState == OMX_StateLoaded ||
                pComponentPrivate->pPortDef[INPUT_PORT_AACDEC]->bPopulated) {
                                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                pHandle->pApplicationPrivate,
                                OMX_EventCmdComplete,
                                OMX_CommandPortEnable,
                                INPUT_PORT_AACDEC,
                                NULL);
                pComponentPrivate->bEnableCommandPending = 0;
                pComponentPrivate->reconfigInputPort = 0;
                
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

		/* Needed for port reconfiguration */   
                AACDEC_CleanupInitParamsEx(pHandle,commandData);
                AACDECFill_LCMLInitParamsEx(pHandle,commandData);
////
#if 1

                for (i=0; i < pComponentPrivate->nNumInputBufPending; i++) {
                        OMX_PRBUFFER2(pComponentPrivate->dbg, "%d pComponentPrivate->pInputBufHdrPending[%lu] = %d\n",__LINE__,i,
                                      pComponentPrivate->pInputBufHdrPending[i] != NULL);
                        if (pComponentPrivate->pInputBufHdrPending[i] != NULL) {
                            AACD_LCML_BUFHEADERTYPE *pLcmlHdr;
                            AACDEC_GetCorresponding_LCMLHeader(pComponentPrivate,pComponentPrivate->pInputBufHdrPending[i]->pBuffer,
                                                               OMX_DirInput, &pLcmlHdr);
                                AACDEC_SetPending(pComponentPrivate,pComponentPrivate->pInputBufHdrPending[i],OMX_DirInput,__LINE__);
                                OMX_PRDSP2(pComponentPrivate->dbg, "Calling LCML_QueueBuffer Line %d\n",__LINE__);
                                eError = LCML_QueueBuffer(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                          EMMCodecInputBuffer,
                                                          pComponentPrivate->pInputBufHdrPending[i]->pBuffer,
                                                          pComponentPrivate->pInputBufHdrPending[i]->nAllocLen,
                                                          pComponentPrivate->pInputBufHdrPending[i]->nFilledLen,
                                                          (OMX_U8 *) pLcmlHdr->pIpParam,
                                                          sizeof(AACDEC_UAlgInBufParamStruct),
                                                          NULL);
                                if(eError != OMX_ErrorNone) {
                                    OMX_ERROR4(pComponentPrivate->dbg, ": Error Occurred in LCML QueueBuffer for input\n");
                                    pComponentPrivate->curState = OMX_StateInvalid;
                                    pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                                    pHandle->pApplicationPrivate,
                                                                    OMX_EventError,
                                                                    eError,
                                                                    OMX_TI_ErrorSevere,
                                                                    NULL);
                                    goto EXIT;
                                }
                        }
                }
                pComponentPrivate->nNumInputBufPending = 0;
////
#endif
            }
            else {
                pComponentPrivate->bEnableCommandPending = 1;
                pComponentPrivate->bEnableCommandParam = commandData;
            }
        }
        else if(commandData == 0x1) {
			if ((pComponentPrivate->curState == OMX_StateLoaded) ||
                            (pComponentPrivate->pPortDef[OUTPUT_PORT_AACDEC]->bPopulated)){
                            OMX_PRCOMM2(pComponentPrivate->dbg, "Command port enable completed\n\n");
			    pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                                    pHandle->pApplicationPrivate,
                                                                    OMX_EventCmdComplete,
                                                                    OMX_CommandPortEnable,
                                                                    OUTPUT_PORT_AACDEC,
                                                                    NULL);
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
		    /* Needed for port reconfiguration */   
               AACDEC_CleanupInitParamsEx(pHandle,commandData);
               AACDECFill_LCMLInitParamsEx(pHandle,commandData);
//// release any buffers received during port disable for reconfig
                    for (i=0; i < pComponentPrivate->nNumOutputBufPending; i++) {
                        OMX_PRBUFFER2(pComponentPrivate->dbg, "%d pComponentPrivate->pOutputBufHdrPending[%lu] = %p\n",__LINE__,i,
                                      pComponentPrivate->pOutputBufHdrPending[i]);
                        if (pComponentPrivate->pOutputBufHdrPending[i] != NULL) {
                            AACD_LCML_BUFHEADERTYPE *pLcmlHdr;
                            AACDEC_GetCorresponding_LCMLHeader(pComponentPrivate,pComponentPrivate->pOutputBufHdrPending[i]->pBuffer,
                                                               OMX_DirOutput, &pLcmlHdr);
                                AACDEC_SetPending(pComponentPrivate,pComponentPrivate->pOutputBufHdrPending[i],OMX_DirOutput,__LINE__);
                                OMX_PRBUFFER2(pComponentPrivate->dbg, "Calling LCML_QueueBuffer Line %d\n",__LINE__);
                                eError = LCML_QueueBuffer(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                      EMMCodecOuputBuffer,
                                                      pComponentPrivate->pOutputBufHdrPending[i]->pBuffer,
                                                      pComponentPrivate->pOutputBufHdrPending[i]->nAllocLen,
                                                      0,
                                                      (OMX_U8 *) pLcmlHdr->pOpParam,
                                                      sizeof(AACDEC_UAlgOutBufParamStruct),
                                                      NULL);
                                if(eError != OMX_ErrorNone) {
                                    OMX_ERROR4(pComponentPrivate->dbg, ": Error Occurred in LCML QueueBuffer for input\n");
                                    pComponentPrivate->curState = OMX_StateInvalid;
                                    pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                                    pHandle->pApplicationPrivate,
                                                                    OMX_EventError,
                                                                    eError,
                                                                    OMX_TI_ErrorSevere,
                                                                    NULL);
                                    goto EXIT;
                                }
                        }
                    }
                    pComponentPrivate->nNumOutputBufPending = 0;    
                    pComponentPrivate->bEnableCommandPending = 0;
                    pComponentPrivate->reconfigOutputPort = 0;
                }
                else {
                    pComponentPrivate->bEnableCommandPending = 1;
                    pComponentPrivate->bEnableCommandParam = commandData;
                }
            }
            else if(commandData == -1) {
                if (pComponentPrivate->curState == OMX_StateLoaded ||
                    (pComponentPrivate->pPortDef[INPUT_PORT_AACDEC]->bPopulated &&
                     pComponentPrivate->pPortDef[OUTPUT_PORT_AACDEC]->bPopulated)){
                     pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                            pHandle->pApplicationPrivate,
                                                            OMX_EventCmdComplete,
                                                            OMX_CommandPortEnable,
                                                            INPUT_PORT_AACDEC,
                                                            NULL);
                     pComponentPrivate->reconfigInputPort = 0;
                     pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                            pHandle->pApplicationPrivate,
                                                            OMX_EventCmdComplete,
                                                            OMX_CommandPortEnable,
                                                            OUTPUT_PORT_AACDEC,
                                                            NULL);

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
                     pComponentPrivate->reconfigOutputPort = 0;
                     pComponentPrivate->bEnableCommandPending = 0;
                     AACDEC_CleanupInitParamsEx(pHandle,commandData);
                     AACDECFill_LCMLInitParamsEx(pHandle,commandData);


                     for (i=0; i < pComponentPrivate->nNumInputBufPending; i++) {
                         OMX_PRBUFFER2(pComponentPrivate->dbg, "%d pComponentPrivate->pInputBufHdrPending[%lu] = %d\n",__LINE__,i,
                                        pComponentPrivate->pInputBufHdrPending[i] != NULL);
                         if (pComponentPrivate->pInputBufHdrPending[i] != NULL) {
                             AACD_LCML_BUFHEADERTYPE *pLcmlHdr;
                             AACDEC_GetCorresponding_LCMLHeader(pComponentPrivate,pComponentPrivate->pInputBufHdrPending[i]->pBuffer,
                                                                OMX_DirInput, &pLcmlHdr);
                             AACDEC_SetPending(pComponentPrivate,pComponentPrivate->pInputBufHdrPending[i],OMX_DirInput,__LINE__);
                             OMX_PRBUFFER2(pComponentPrivate->dbg, "Calling LCML_QueueBuffer Line %d\n",__LINE__);
                             eError = LCML_QueueBuffer(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                          EMMCodecInputBuffer,
                                                          pComponentPrivate->pInputBufHdrPending[i]->pBuffer,
                                                          pComponentPrivate->pInputBufHdrPending[i]->nAllocLen,
                                                          pComponentPrivate->pInputBufHdrPending[i]->nFilledLen,
                                                          (OMX_U8 *) pLcmlHdr->pIpParam,
                                                          sizeof(AACDEC_UAlgInBufParamStruct),
                                                          NULL);
                             if(eError != OMX_ErrorNone) {
                                OMX_ERROR4(pComponentPrivate->dbg, ": Error Occurred in LCML QueueBuffer for input\n");
                                pComponentPrivate->curState = OMX_StateInvalid;
                                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                                pHandle->pApplicationPrivate,
                                                                OMX_EventError,
                                                                eError,
                                                                OMX_TI_ErrorSevere,
                                                                NULL);
                                goto EXIT;
                            }
                         }
                     }
                     pComponentPrivate->nNumInputBufPending = 0;
                     for (i=0; i < pComponentPrivate->nNumOutputBufPending; i++) {
                         OMX_PRBUFFER2(pComponentPrivate->dbg, "%d pComponentPrivate->pOutputBufHdrPending[%lu] = %p\n",__LINE__,i,
                                       pComponentPrivate->pOutputBufHdrPending[i]);
                         if (pComponentPrivate->pOutputBufHdrPending[i] != NULL) {
                             AACD_LCML_BUFHEADERTYPE *pLcmlHdr;
                             AACDEC_GetCorresponding_LCMLHeader(pComponentPrivate,pComponentPrivate->pOutputBufHdrPending[i]->pBuffer,
                                                                OMX_DirOutput, &pLcmlHdr);
                             AACDEC_SetPending(pComponentPrivate,pComponentPrivate->pOutputBufHdrPending[i],OMX_DirOutput,__LINE__);
                             OMX_PRDSP2(pComponentPrivate->dbg, "Calling LCML_QueueBuffer Line %d\n",__LINE__);
                             eError = LCML_QueueBuffer(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                      EMMCodecOuputBuffer,
                                                      pComponentPrivate->pOutputBufHdrPending[i]->pBuffer,
                                                      pComponentPrivate->pOutputBufHdrPending[i]->nAllocLen,
                                                      0,
                                                      (OMX_U8 *) pLcmlHdr->pOpParam,
                                                      sizeof(AACDEC_UAlgOutBufParamStruct),
                                                      NULL);
                             if(eError != OMX_ErrorNone) {
                                OMX_ERROR4(pComponentPrivate->dbg, ": Error Occurred in LCML QueueBuffer for input\n");
                                pComponentPrivate->curState = OMX_StateInvalid;
                                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                                pHandle->pApplicationPrivate,
                                                                OMX_EventError,
                                                                eError,
                                                                OMX_TI_ErrorSevere,
                                                                NULL);
                                goto EXIT;
                            }
                         }
                     }
                     pComponentPrivate->nNumOutputBufPending = 0;
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
        if(commandData == 0x0 || commandData == -1) {
            OMX_ERROR2(pComponentPrivate->dbg, "Flushing input port:: unhandled ETB's = %ld, handled ETB's = %ld\n", pComponentPrivate->nUnhandledEmptyThisBuffers, pComponentPrivate->nHandledEmptyThisBuffers);
            if (pComponentPrivate->nUnhandledEmptyThisBuffers == pComponentPrivate->nHandledEmptyThisBuffers) {
                pComponentPrivate->bFlushInputPortCommandPending = OMX_FALSE;
                pComponentPrivate->first_buff = 0;
                    OMX_PRCOMM2(pComponentPrivate->dbg, "about to be Flushing input port\n");
                if (pComponentPrivate->num_Sent_Ip_Buff){ //no buffers have been sent yet, no need to flush SN
                    aParam[0] = USN_STRMCMD_FLUSH;
                    aParam[1] = 0x0;
                    aParam[2] = 0x0;

                    OMX_PRCOMM2(pComponentPrivate->dbg, "Flushing input port DSP\n");
                    if (pComponentPrivate->codecFlush_waitingsignal == 0){
                            pthread_mutex_lock(&pComponentPrivate->codecFlush_mutex);
                    }
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                 EMMCodecControlStrmCtrl, (void*)aParam);
                    if (pComponentPrivate->codecFlush_waitingsignal == 0){
                        pthread_cond_wait(&pComponentPrivate->codecFlush_threshold, &pComponentPrivate->codecFlush_mutex);
                        pComponentPrivate->codecFlush_waitingsignal = 0;
                        pthread_mutex_unlock(&pComponentPrivate->codecFlush_mutex);
                    }
                    if (eError != OMX_ErrorNone) {
                        goto EXIT;
                    }
                }
                else{
                    OMX_ERROR2(pComponentPrivate->dbg, "skipped DSP, Flushing input port\n");
                    for (i=0; i < pComponentPrivate->nNumInputBufPending; i++) {
#ifdef __PERF_INSTRUMENTATION__
                        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                          PREF(pComponentPrivate->pInputBufHdrPending[i],pBuffer),
                                          0,
                                          PERF_ModuleHLMM);
#endif

                        pComponentPrivate->cbInfo.EmptyBufferDone (pComponentPrivate->pHandle,
                                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                                   pComponentPrivate->pInputBufHdrPending[i]);
                        pComponentPrivate->pInputBufHdrPending[i] = NULL;
                        pComponentPrivate->nEmptyBufferDoneCount++;
                        SignalIfAllBuffersAreReturned(pComponentPrivate);
                    }
                    pComponentPrivate->nNumInputBufPending=0;    
                    pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventCmdComplete, 
                                                           OMX_CommandFlush,
                                                           OMX_DirInput, 
                                                           NULL);
                }
            }else {
                pComponentPrivate->bFlushInputPortCommandPending = OMX_TRUE;
            }
        }
        if(commandData == 0x1 || commandData == -1){
            OMX_ERROR2(pComponentPrivate->dbg, "Flushing output port:: unhandled FTB's = %ld, handled FTB's = %ld\n", pComponentPrivate->nUnhandledFillThisBuffers, pComponentPrivate->nHandledFillThisBuffers);
            if (pComponentPrivate->nUnhandledFillThisBuffers == pComponentPrivate->nHandledFillThisBuffers) {
                pComponentPrivate->bFlushOutputPortCommandPending = OMX_FALSE;
                if (pComponentPrivate->first_output_buf_rcv != 0) {
                    pComponentPrivate->first_buff = 0;
                    pComponentPrivate->first_output_buf_rcv = 0;
                }
                OMX_ERROR2(pComponentPrivate->dbg, "About to be Flushing output port\n");
                if(pComponentPrivate->num_Op_Issued && !pComponentPrivate->reconfigOutputPort ){ //no buffers sent to DSP yet
                    aParam[0] = USN_STRMCMD_FLUSH;
                    aParam[1] = 0x1;
                    aParam[2] = 0x0;

                    OMX_ERROR2(pComponentPrivate->dbg, "Flushing output port dsp\n");
                    if (pComponentPrivate->codecFlush_waitingsignal == 0){
                            pthread_mutex_lock(&pComponentPrivate->codecFlush_mutex);
                    }
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               EMMCodecControlStrmCtrl, (void*)aParam);
                    if (pComponentPrivate->codecFlush_waitingsignal == 0){
                        pthread_cond_wait(&pComponentPrivate->codecFlush_threshold, &pComponentPrivate->codecFlush_mutex);
                        pComponentPrivate->codecFlush_waitingsignal = 0;
                        pthread_mutex_unlock(&pComponentPrivate->codecFlush_mutex);
                    }
                    if (eError != OMX_ErrorNone) {
                        goto EXIT;
                    }
                }else{
                    OMX_ERROR2(pComponentPrivate->dbg, "skipped dsp flush, Flushing output port\n");
//force FillBufferDone calls on pending buffers
                    for (i=0; i < pComponentPrivate->nNumOutputBufPending; i++) {
#ifdef __PERF_INSTRUMENTATION__
                        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                          PREF(pComponentPrivate->pOutputBufHdrPending[i],pBuffer),
                                          PREF(pComponentPrivate->pOutputBufHdrPending[i],nFilledLen),
                                          PERF_ModuleHLMM);
#endif  

                        pComponentPrivate->cbInfo.FillBufferDone (pComponentPrivate->pHandle,
                                                                  pComponentPrivate->pHandle->pApplicationPrivate,
                                                                  pComponentPrivate->pOutputBufHdrPending[i]
                                                                  );
                        pComponentPrivate->nOutStandingFillDones--;
                        pComponentPrivate->nFillBufferDoneCount++; 
                        SignalIfAllBuffersAreReturned(pComponentPrivate);
                        pComponentPrivate->pOutputBufHdrPending[i] = NULL;
                    }
                    pComponentPrivate->nNumOutputBufPending=0;

                    pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle, 
                                                           pComponentPrivate->pHandle->pApplicationPrivate,
                                                           OMX_EventCmdComplete, 
                                                           OMX_CommandFlush,
                                                           OMX_DirOutput,
                                                           NULL);

                }
            }
            else {
                pComponentPrivate->bFlushOutputPortCommandPending = OMX_TRUE;
            }
        }
    }
 EXIT:
    /* @NOTE: EXIT_COMPONENT_THRD is not REALLY an error, but a signal to ComponentThread.c */
    return eError;
}


OMX_U32 AACDEC_ParseHeader(OMX_BUFFERHEADERTYPE* pBufHeader,
                           AACDEC_COMPONENT_PRIVATE *pComponentPrivate)
{
    int iObjectType = 0;
    int iSampleRateIndex = 0;
    OMX_U32 nBitPosition = 0;
    OMX_U8* pHeaderStream = (OMX_U8*)pBufHeader->pBuffer;
    OMX_U32 syncExtensionType = 0;
    OMX_U32 extensionAudioObjectType = 0;
    OMX_U32 externsionSamplingFrequency = 0;
    OMX_U32 externsionSamplingFrequencyIdx = 0;

    iObjectType = AACDEC_GetBits(&nBitPosition, 5, pHeaderStream, OMX_TRUE);

    switch(iObjectType){
        case OBJECTTYPE_HE:
            pComponentPrivate->aacParams->eAACProfile = OMX_AUDIO_AACObjectHE;
            break;
        case OBJECTTYPE_HE2:
            pComponentPrivate->aacParams->eAACProfile = OMX_AUDIO_AACObjectHE_PS;
            break;
        case OBJECTTYPE_LTP:
            pComponentPrivate->aacParams->eAACProfile = OMX_AUDIO_AACObjectLTP;
            break;
        case OBJECTTYPE_LC:
        default:
            pComponentPrivate->aacParams->eAACProfile = OMX_AUDIO_AACObjectLC;
            break;
    }

                    
    iSampleRateIndex = AACDEC_GetBits(&nBitPosition, 4, pHeaderStream, OMX_TRUE);
    pComponentPrivate->AACDEC_UALGParam->lSamplingRateIdx = iSampleRateIndex;

    if(pComponentPrivate->pcmParams->nSamplingRate != AACDec_GetSampleRatebyIndex(iSampleRateIndex)){
        // output port needs reconfig. set the new values and mark the flag to do reconfig below.
        pComponentPrivate->aacParams->nSampleRate = AACDec_GetSampleRatebyIndex(iSampleRateIndex);
        pComponentPrivate->pcmParams->nSamplingRate = pComponentPrivate->aacParams->nSampleRate;
        OMX_PRDSP2(pComponentPrivate->dbg, "New Sample rate detected:: %ld (%ld)\n",pComponentPrivate->AACDEC_UALGParam->lSamplingRateIdx,
                      pComponentPrivate->pcmParams->nSamplingRate);
        pComponentPrivate->reconfigOutputPort = OMX_TRUE;

    }

    pComponentPrivate->pcmParams->nChannels = AACDEC_GetBits(&nBitPosition, 4, pHeaderStream, OMX_TRUE);
    OMX_PRINT2(pComponentPrivate->dbg, "nChannels %ld\n",pComponentPrivate->pcmParams->nChannels);
    /* Override nChannels to always be STEREO (2) */
    pComponentPrivate->pcmParams->nChannels = 2;


    if (iObjectType == OBJECTTYPE_HE){
        externsionSamplingFrequencyIdx = AACDEC_GetBits(&nBitPosition, 4, pHeaderStream, OMX_TRUE);
    }else {
        syncExtensionType = AACDEC_GetBits(&nBitPosition, 3, pHeaderStream, OMX_TRUE);
        syncExtensionType = AACDEC_GetBits(&nBitPosition, 11, pHeaderStream, OMX_TRUE);

        if(syncExtensionType == 0x2b7){
            extensionAudioObjectType = AACDEC_GetBits(&nBitPosition, 5, pHeaderStream, OMX_TRUE);
            if (extensionAudioObjectType == OBJECTTYPE_HE){
                OMX_PRDSP2(pComponentPrivate->dbg, "OBJECTTYPE_HE detected!\n");
                pComponentPrivate->aacParams->eAACProfile = OMX_AUDIO_AACObjectHE;
            }
            if (extensionAudioObjectType == OBJECTTYPE_HE2){
                OMX_PRDSP2(pComponentPrivate->dbg, "OBJECTTYPE_HE2 detected!\n");
                pComponentPrivate->aacParams->eAACProfile = OMX_AUDIO_AACObjectHE_PS;
            }
            pComponentPrivate->SBR = AACDEC_GetBits(&nBitPosition, 1, pHeaderStream, OMX_TRUE);
            if(pComponentPrivate->SBR){
                externsionSamplingFrequency = AACDEC_GetBits(&nBitPosition, 4, pHeaderStream, OMX_TRUE);
                OMX_PRDSP2(pComponentPrivate->dbg, "sbrPresentFlag detected, externsionSamplingFrequency %ld\n",
                              externsionSamplingFrequency);
            }else{
                OMX_PRDSP2(pComponentPrivate->dbg, "sbrPresentFlag not present! %ld\n",pComponentPrivate->pcmParams->nSamplingRate);
            }
        }
    }

    OMX_PRDSP2(pComponentPrivate->dbg, "%s: Parsing AudioSpecificConfig() %d\n",__FUNCTION__, __LINE__);
    OMX_PRDSP1(pComponentPrivate->dbg, "%s: profile=%d", __FUNCTION__, iObjectType);
    OMX_PRDSP1(pComponentPrivate->dbg, "%s: iSampleRateIndex=%d", __FUNCTION__, iSampleRateIndex);
    OMX_PRDSP1(pComponentPrivate->dbg, "%s: nFilledLen=%ld", __FUNCTION__, pBufHeader->nFilledLen);
                    
    // we are done with this config buffer, let the client know
    pBufHeader->nFilledLen = 0;
    pComponentPrivate->cbInfo.EmptyBufferDone (pComponentPrivate->pHandle,
                                               pComponentPrivate->pHandle->pApplicationPrivate,
                                               pBufHeader);
    pComponentPrivate->nEmptyBufferDoneCount++;
    SignalIfAllBuffersAreReturned(pComponentPrivate);


    return 0;

}
/* ================================================================================= * */
/**
* @fn AACDEC_HandleDataBuf_FromApp() function handles the input and output buffers
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

OMX_ERRORTYPE AACDEC_HandleDataBuf_FromApp(OMX_BUFFERHEADERTYPE* pBufHeader,
                                    AACDEC_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_DIRTYPE eDir;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefIn = NULL;
    char *pArgs = "damedesuStr";
    OMX_U32 pValues[4];
    OMX_U32 pValues1[4];
    int iObjectType = 0;
    int iSampleRateIndex = 0;
    OMX_U32 nBitPosition = 0;
    OMX_U8* pHeaderStream = (OMX_U8*)pBufHeader->pBuffer;
    OMX_U32 i = 0;

    pBufHeader->pPlatformPrivate  = pComponentPrivate;
    eError = AACDEC_GetBufferDirection(pBufHeader, &eDir);
    if (eError != OMX_ErrorNone) {
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: The pBufHeader is not found in the list\n",__LINE__);
        goto EXIT;
    }
    if (eDir == OMX_DirInput) {
        pComponentPrivate->nHandledEmptyThisBuffers++;
        if (pComponentPrivate->curState == OMX_StateIdle){
            pComponentPrivate->cbInfo.EmptyBufferDone (pComponentPrivate->pHandle,
                                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                                       pBufHeader);
            pComponentPrivate->nEmptyBufferDoneCount++;
            SignalIfAllBuffersAreReturned(pComponentPrivate);
            OMX_PRBUFFER2(pComponentPrivate->dbg, ":: %d %s In idle state return input buffers\n", __LINE__, __FUNCTION__);
            goto EXIT;
        }
        LCML_DSP_INTERFACE *pLcmlHandle = (LCML_DSP_INTERFACE *)pComponentPrivate->pLcmlHandle;
        AACD_LCML_BUFHEADERTYPE *pLcmlHdr;
        pPortDefIn = pComponentPrivate->pPortDef[OMX_DirInput];
        eError = AACDEC_GetCorresponding_LCMLHeader(pComponentPrivate,pBufHeader->pBuffer, OMX_DirInput, &pLcmlHdr);
        if (eError != OMX_ErrorNone) {
            OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error: Invalid Buffer Came ...\n",__LINE__);
            goto EXIT;
        }
        OMX_PRBUFFER1(pComponentPrivate->dbg, "%d:::IN:: pBufHeader->nFilledLen = %ld\n",__LINE__, pBufHeader->nFilledLen);

        if (pBufHeader->nFilledLen > 0 || (pBufHeader->nFlags & OMX_BUFFERFLAG_EOS)) {
            pComponentPrivate->bBypassDSP = 0;
            OMX_PRDSP2(pComponentPrivate->dbg, "%d:::Calling LCML_QueueBuffer\n",__LINE__);

#ifdef __PERF_INSTRUMENTATION__
            PERF_SendingFrame(pComponentPrivate->pPERFcomp,PREF(pBufHeader,pBuffer),
                              pPortDefIn->nBufferSize,
                              PERF_ModuleCommonLayer);
#endif
            pLcmlHdr->pIpParam->bLastBuffer = 0;
            pLcmlHdr->pIpParam->bConcealBuffer = 0;
            if (pBufHeader->nFlags == OMX_BUFFERFLAG_DATACORRUPT){
	        OMX_PRINT2(pComponentPrivate->dbg, "%d :: bConcealBuffer Is Set Here....\n",__LINE__);
	        pLcmlHdr->pIpParam->bConcealBuffer = 1;
            }

            if(pComponentPrivate->SendAfterEOS == 1){
                pComponentPrivate->AACDEC_UALGParam->size = sizeof(MPEG4AACDEC_UALGParams);
                if(pComponentPrivate->dasfmode == 1) {
                    pComponentPrivate->pParams->unAudioFormat = STEREO_NONINTERLEAVED_STREAM_AACDEC;
                    if(pComponentPrivate->aacParams->nChannels == OMX_AUDIO_ChannelModeMono) {
                        pComponentPrivate->pParams->unAudioFormat = MONO_STREAM_AACDEC;
                    }

                        pComponentPrivate->pParams->ulSamplingFreq = pComponentPrivate->aacParams->nSampleRate;
                        pComponentPrivate->pParams->unUUID = pComponentPrivate->streamID;

                        pValues[0] = USN_STRMCMD_SETCODECPARAMS;
                        pValues[1] = (OMX_U32)pComponentPrivate->pParams;
                        pValues[2] = sizeof(USN_AudioCodecParams);
                        eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                   EMMCodecControlStrmCtrl,(void *)pValues);
                        if(eError != OMX_ErrorNone) {
                            OMX_ERROR4(pComponentPrivate->dbg, "%d: Error Occurred in Codec StreamControl..\n",__LINE__);
                            pComponentPrivate->curState = OMX_StateInvalid;
                            pComponentPrivate->cbInfo.EventHandler( pComponentPrivate->pHandle,
                                                                    pComponentPrivate->pHandle->pApplicationPrivate,
                                                                    OMX_EventError,
                                                                    eError,
                                                                    OMX_TI_ErrorSevere,
                                                                    NULL);
                        
                            goto EXIT;
                        }
                }
                
                
#ifdef ANDROID
                if (pBufHeader->nFlags & OMX_BUFFERFLAG_CODECCONFIG ){
                    pComponentPrivate->bConfigData = 1;
                    AACDEC_ParseHeader(pBufHeader,pComponentPrivate);

                    // if port config is needed send the event to the client
                    if(pComponentPrivate->reconfigInputPort || pComponentPrivate->reconfigOutputPort){
                        if(pComponentPrivate->reconfigInputPort && pComponentPrivate->reconfigOutputPort){

                            // we need to also be able to guarantee that no FillBufferDone calls
                            // have been made yet. Otherwise the audio-MIO-node will assume
                            // that the port settings are valid.

                            // now send event to the ports that need re-config
                            
                            /* port settings changed, let the client know... */
		            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                               pComponentPrivate->pHandle->pApplicationPrivate,
                                               OMX_EventPortSettingsChanged,
                                               AACDEC_INPUT_PORT,
                                               0,
                                               NULL);

                            //TODO: add wait code.

		            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                               pComponentPrivate->pHandle->pApplicationPrivate,
                                               OMX_EventPortSettingsChanged,
                                               AACDEC_OUTPUT_PORT,
                                               0,
                                               NULL);
                        }
                        else{
                            OMX_PRBUFFER2(pComponentPrivate->dbg, "After EBD::pComponentPrivate->nFillBufferDoneCount = %ld\n", pComponentPrivate->nFillBufferDoneCount);
                            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                               pComponentPrivate->pHandle->pApplicationPrivate,
                                               OMX_EventPortSettingsChanged,
                                               pComponentPrivate->reconfigOutputPort ? AACDEC_OUTPUT_PORT : AACDEC_INPUT_PORT,
                                               0,
                                               NULL);
                        }
                    }
                    OMX_PRBUFFER2(pComponentPrivate->dbg, "After PortSettingsChangedEvent::pComponentPrivate->nFillBufferDoneCount = %ld\n\n", pComponentPrivate->nFillBufferDoneCount);
                    pComponentPrivate->bConfigData = 0;
                    goto EXIT;
                }

                pComponentPrivate->AACDEC_UALGParam->bRawFormat = 1;

/* dasf mode should always be false (for now) under Android */
                pComponentPrivate->AACDEC_UALGParam->lOutputFormat = EAUDIO_INTERLEAVED;

                switch(pComponentPrivate->aacParams->eAACProfile){
                    case OMX_AUDIO_AACObjectLTP:
                        pComponentPrivate->AACDEC_UALGParam->iEnablePS =  0;
                        pComponentPrivate->AACDEC_UALGParam->DownSampleSbr = 1;
                        break;
                    case OMX_AUDIO_AACObjectHE_PS:
                        pComponentPrivate->AACDEC_UALGParam->iEnablePS =  1;
                        pComponentPrivate->AACDEC_UALGParam->DownSampleSbr = 1;
                        pComponentPrivate->parameteric_stereo = PARAMETRIC_STEREO_AACDEC;
                        break;
                    case OMX_AUDIO_AACObjectHE:
                        pComponentPrivate->AACDEC_UALGParam->iEnablePS =  1;
                        pComponentPrivate->AACDEC_UALGParam->DownSampleSbr = 1;
			break;
                    case OMX_AUDIO_AACObjectLC:
                    default: /* we will use LC profile as the default, SSR and Main Profiles are not supported */
                        OMX_PRDSP2(pComponentPrivate->dbg, "%s: IN Switch::ObjectLC\n", __FUNCTION__);
                        pComponentPrivate->AACDEC_UALGParam->iEnablePS =  1;

                        // always use down sample flag on for LC content, 
                        // this will avoid the upsampled output issue
                        pComponentPrivate->AACDEC_UALGParam->DownSampleSbr = 1;
                        break;
                }

#else


                if(pComponentPrivate->parameteric_stereo == PARAMETRIC_STEREO_AACDEC){
                    if(pComponentPrivate->dasfmode == 1){
                        pComponentPrivate->AACDEC_UALGParam->lOutputFormat    = EAUDIO_BLOCK;
                    }
                    else{
                        pComponentPrivate->AACDEC_UALGParam->lOutputFormat    = EAUDIO_INTERLEAVED;
                    }
                    pComponentPrivate->AACDEC_UALGParam->iEnablePS        = 1;/*Added for eAAC*/
                    pComponentPrivate->AACDEC_UALGParam->dualMonoMode     = pComponentPrivate->dualMonoMode;
                    pComponentPrivate->AACDEC_UALGParam->lSamplingRateIdx = AACDec_GetSampleRateIndexL(pComponentPrivate->aacParams->nSampleRate);
                    pComponentPrivate->AACDEC_UALGParam->bRawFormat       = 0;
                    if(pComponentPrivate->aacParams->eAACStreamFormat == OMX_AUDIO_AACStreamFormatRAW){
                        pComponentPrivate->AACDEC_UALGParam->bRawFormat       = 1;
                    }
                    pComponentPrivate->AACDEC_UALGParam->DownSampleSbr    = 1;
                }else{

                    if(pComponentPrivate->dasfmode == 1){
                        pComponentPrivate->AACDEC_UALGParam->lOutputFormat    = EAUDIO_BLOCK;
                    }
                    else{
                        pComponentPrivate->AACDEC_UALGParam->lOutputFormat    = EAUDIO_INTERLEAVED;
                    }
                    pComponentPrivate->AACDEC_UALGParam->iEnablePS        = 0;
                    pComponentPrivate->AACDEC_UALGParam->dualMonoMode     = pComponentPrivate->dualMonoMode;
                    pComponentPrivate->AACDEC_UALGParam->lSamplingRateIdx = AACDec_GetSampleRateIndexL(pComponentPrivate->aacParams->nSampleRate);


                    pComponentPrivate->AACDEC_UALGParam->bRawFormat       = 0;
                    if(pComponentPrivate->aacParams->eAACStreamFormat == OMX_AUDIO_AACStreamFormatRAW){
                        pComponentPrivate->AACDEC_UALGParam->bRawFormat       = 1;
                    }
                    pComponentPrivate->AACDEC_UALGParam->DownSampleSbr    = 0;
                    if(pComponentPrivate->SBR ){
                        pComponentPrivate->AACDEC_UALGParam->DownSampleSbr    = 1;
                    }
                }
#endif

                OMX_PRCOMM2(pComponentPrivate->dbg, "Sending codec config params ::: \n");
                OMX_PRCOMM2(pComponentPrivate->dbg, "pComponentPrivate->AACDEC_UALGParam->dualMonoMode = %ld\n", pComponentPrivate->AACDEC_UALGParam->dualMonoMode);
                OMX_PRCOMM2(pComponentPrivate->dbg, "pComponentPrivate->AACDEC_UALGParam->lSamplingRateIdx = %ld\n", pComponentPrivate->AACDEC_UALGParam->lSamplingRateIdx);
                OMX_PRCOMM2(pComponentPrivate->dbg, "pComponentPrivate->AACDEC_UALGParam->iEnablePS = %ld\n", pComponentPrivate->AACDEC_UALGParam->iEnablePS);
                OMX_PRCOMM2(pComponentPrivate->dbg, "pComponentPrivate->AACDEC_UALGParam->DownSampleSbr = %ld\n", pComponentPrivate->AACDEC_UALGParam->DownSampleSbr);
                OMX_PRCOMM2(pComponentPrivate->dbg, "pComponentPrivate->AACDEC_UALGParam->bRawFormat = %ld\n", pComponentPrivate->AACDEC_UALGParam->bRawFormat);
                OMX_PRCOMM2(pComponentPrivate->dbg, "Codec params summary complete ::: \n");


                pValues1[0] = IUALG_CMD_SETSTATUS;
                pValues1[1] = (OMX_U32)pComponentPrivate->AACDEC_UALGParam;
                pValues1[2] = sizeof(MPEG4AACDEC_UALGParams);
                OMX_PRDSP2(pComponentPrivate->dbg, "LCML_ControlCodec called to send config data\n");
                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                           EMMCodecControlAlgCtrl,(void *)pValues1);
                if(eError != OMX_ErrorNone) {
                    pComponentPrivate->curState = OMX_StateInvalid;
                    pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                           pComponentPrivate->pHandle->pApplicationPrivate,
                                                           OMX_EventError,
                                                           eError,
                                                           OMX_TI_ErrorSevere,
                                                           NULL);
                                    OMX_ERROR4(pComponentPrivate->dbg, "ERROR::LCML_ControlCodec called to send config data\n");
                    goto EXIT;
                }

                pComponentPrivate->bDspStoppedWhileExecuting = OMX_FALSE;
                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                           EMMCodecControlStart,(void *)pArgs);
                if(eError != OMX_ErrorNone) {
                    OMX_ERROR4(pComponentPrivate->dbg, "%d: Error Occurred in Codec Start..\n",__LINE__);
                    goto EXIT;
                }

                pComponentPrivate->SendAfterEOS = 0;
		OMX_PRINT2(pComponentPrivate->dbg, "sample rate %ld\n",pComponentPrivate->pcmParams->nSamplingRate);
            }

            if(pBufHeader->nFlags & OMX_BUFFERFLAG_EOS) {
                OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: bLastBuffer Is Set Here....\n",__LINE__);
                pLcmlHdr->pIpParam->bLastBuffer = 1;
                pComponentPrivate->bIsEOFSent = 1;
                pComponentPrivate->SendAfterEOS = 1;
                pBufHeader->nFlags = 0;
            }

            /* Store time stamp information */
            pComponentPrivate->arrBufIndex[pComponentPrivate->IpBufindex] = pBufHeader->nTimeStamp;
            /*Store tick count information*/
            pComponentPrivate->arrBufIndexTick[pComponentPrivate->IpBufindex] = pBufHeader->nTickCount;
            pComponentPrivate->IpBufindex++;
            pComponentPrivate->IpBufindex %= pComponentPrivate->pPortDef[INPUT_PORT_AACDEC]->nBufferCountActual;

            if(!pComponentPrivate->framemode){
	        if(pComponentPrivate->first_buff == 0){
		    pComponentPrivate->first_TS = pBufHeader->nTimeStamp;
                    OMX_PRBUFFER2(pComponentPrivate->dbg, "in ts-%ld\n",pBufHeader->nTimeStamp);
		    pComponentPrivate->first_buff = 1;
	        }
            }
            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d Comp:: Sending Filled Input buffer = %p, %p\
                               to LCML\n", __LINE__,pBufHeader,pBufHeader->pBuffer);
            if (pComponentPrivate->curState == OMX_StateExecuting) {
                if (!AACDEC_IsPending(pComponentPrivate,pBufHeader,OMX_DirInput)) {
                    if(!(pComponentPrivate->bDspStoppedWhileExecuting || pComponentPrivate->bNoIdleOnStop)) {
                        if(!pComponentPrivate->reconfigInputPort){
                            AACDEC_SetPending(pComponentPrivate,pBufHeader,OMX_DirInput,__LINE__);
                            OMX_PRBUFFER2(pComponentPrivate->dbg, "Calling LCML_QueueBuffer Line %d\n",__LINE__);
                            OMX_PRBUFFER2(pComponentPrivate->dbg, "input pBufHeader->nFilledLen = %ld\n\n", pBufHeader->nFilledLen);
                            eError = LCML_QueueBuffer(pLcmlHandle->pCodecinterfacehandle,
                                                      EMMCodecInputBuffer,
                                                      pBufHeader->pBuffer,
                                                      pBufHeader->nAllocLen,
                                                      pBufHeader->nFilledLen,
                                                      (OMX_U8 *) pLcmlHdr->pIpParam,
                                                      sizeof(AACDEC_UAlgInBufParamStruct),
                                                      NULL);
                            if(eError != OMX_ErrorNone) {
                                OMX_ERROR4(pComponentPrivate->dbg, ": Error Occurred in LCML QueueBuffer for input\n");
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
                        else{
                           OMX_PRBUFFER4(pComponentPrivate->dbg, "DON'T queue buffers during a reconfig!!\n");
                           pComponentPrivate->pInputBufHdrPending[pComponentPrivate->nNumInputBufPending++] = pBufHeader;
                        }
                        if (eError != OMX_ErrorNone) {
                            OMX_ERROR4(pComponentPrivate->dbg, "%d ::Comp: SetBuff: IP: Error Occurred\n",__LINE__);
                            eError = OMX_ErrorHardware;
                            goto EXIT;
                        }
                    }else {
#ifdef __PERF_INSTRUMENTATION__
                        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                          PREF(pBufHeader, pBuffer),
                                          0,
                                          PERF_ModuleHLMM);
#endif
                        OMX_PRBUFFER2(pComponentPrivate->dbg, ":: %d %s DSP is stopping, returning input buffer \n",
                                      __LINE__, __FUNCTION__);
                        pComponentPrivate->cbInfo.EmptyBufferDone (pComponentPrivate->pHandle,
                                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                                   pBufHeader
                                                                   );
                        pComponentPrivate->nEmptyBufferDoneCount++;
                        SignalIfAllBuffersAreReturned(pComponentPrivate);

                    }
                    pComponentPrivate->lcml_nCntIp++;
                    pComponentPrivate->lcml_nIpBuf++;
                    pComponentPrivate->num_Sent_Ip_Buff++;
                    OMX_PRCOMM2(pComponentPrivate->dbg, "Sending Input buffer to Codec\n");
                }
            }else if (pComponentPrivate->curState == OMX_StatePause){
                pComponentPrivate->pInputBufHdrPending[pComponentPrivate->nNumInputBufPending++] = pBufHeader;
            }
        } else {
            pComponentPrivate->bBypassDSP = 1;
                OMX_PRBUFFER2(pComponentPrivate->dbg, "Forcing EmptyBufferDone\n");
#ifdef __PERF_INSTRUMENTATION__
                PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                  PREF(pComponentPrivate->pInputBufferList->pBufHdr[0], pBuffer),
                                  0,
                                  PERF_ModuleHLMM);
#endif
                pComponentPrivate->cbInfo.EmptyBufferDone (pComponentPrivate->pHandle,
                                                           pComponentPrivate->pHandle->pApplicationPrivate,
                                                           pComponentPrivate->pInputBufferList->pBufHdr[0]
                                                           );
                pComponentPrivate->nEmptyBufferDoneCount++;
                SignalIfAllBuffersAreReturned(pComponentPrivate);
        }
        if(pBufHeader->pMarkData){
            OMX_PRDSP2(pComponentPrivate->dbg, "%d:Detected pBufHeader->pMarkData\n",__LINE__);
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
        if (pComponentPrivate->bFlushInputPortCommandPending) {
            OMX_SendCommand(pComponentPrivate->pHandle,OMX_CommandFlush,0,NULL);
        }
    }else if (eDir == OMX_DirOutput) {
        pComponentPrivate->nHandledFillThisBuffers++;
        if (pComponentPrivate->curState == OMX_StateIdle){
            pComponentPrivate->cbInfo.FillBufferDone (pComponentPrivate->pHandle,
                                                      pComponentPrivate->pHandle->pApplicationPrivate,
                                                      pBufHeader);
            pComponentPrivate->nFillBufferDoneCount++;
            SignalIfAllBuffersAreReturned(pComponentPrivate);
            OMX_PRBUFFER2(pComponentPrivate->dbg, ":: %d %s In idle state return output buffers\n", __LINE__, __FUNCTION__);
            goto EXIT;
        }
        LCML_DSP_INTERFACE *pLcmlHandle = (LCML_DSP_INTERFACE *)pComponentPrivate->pLcmlHandle;
        AACD_LCML_BUFHEADERTYPE *pLcmlHdr;
        OMX_PRBUFFER2(pComponentPrivate->dbg, "%d : pComponentPrivate->lcml_nOpBuf = %ld\n",__LINE__,pComponentPrivate->lcml_nOpBuf);
        OMX_PRBUFFER2(pComponentPrivate->dbg, "%d : pComponentPrivate->lcml_nIpBuf = %ld\n",__LINE__,pComponentPrivate->lcml_nIpBuf);
        eError = AACDEC_GetCorresponding_LCMLHeader(pComponentPrivate,pBufHeader->pBuffer, OMX_DirOutput, &pLcmlHdr);
        if (eError != OMX_ErrorNone) {
            OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error: Invalid Buffer Came ...\n",__LINE__);
            goto EXIT;
        }
        OMX_PRBUFFER2(pComponentPrivate->dbg, "%d:::Calling LCML_QueueBuffer\n",__LINE__);
#ifdef __PERF_INSTRUMENTATION__
        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                          PREF(pBufHeader,pBuffer),
                          0,
                          PERF_ModuleCommonLayer);
#endif
        if (pComponentPrivate->bBypassDSP == 0) {
            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d Comp:: Sending Emptied Output buffer=%p to LCML\n",__LINE__,pBufHeader);
            if (pComponentPrivate->curState == OMX_StateExecuting) {
                OMX_PRDSP2(pComponentPrivate->dbg, "%d Comp:: in AACDEC UTILS pLcmlHandle->pCodecinterfacehandle= %p\n",
                                __LINE__,pLcmlHandle->pCodecinterfacehandle);
                OMX_PRBUFFER2(pComponentPrivate->dbg, "%d Comp:: in AACDEC UTILS EMMCodecOuputBuffer = %u\n",__LINE__,EMMCodecOuputBuffer);
                OMX_PRBUFFER2(pComponentPrivate->dbg, "%d OUT:: pBufHeader->nFilledLen = %ld\n",__LINE__,pBufHeader->nFilledLen);
                OMX_PRBUFFER2(pComponentPrivate->dbg, "%d Comp:: in AACDEC UTILS pBufHeader->nAllocLen = %lu\n",__LINE__,pBufHeader->nAllocLen);
                OMX_PRBUFFER2(pComponentPrivate->dbg, "pComponentPrivate->numPendingBuffers = %lu\n",pComponentPrivate->numPendingBuffers);

                    if (!AACDEC_IsPending(pComponentPrivate,pBufHeader,OMX_DirOutput) &&
                        (pComponentPrivate->numPendingBuffers < pComponentPrivate->pOutputBufferList->numBuffers))  {
                        if (!(pComponentPrivate->bDspStoppedWhileExecuting || pComponentPrivate->bNoIdleOnStop)){
                            if(!pComponentPrivate->reconfigOutputPort){
                                AACDEC_SetPending(pComponentPrivate,pBufHeader,OMX_DirOutput,__LINE__);
                                eError = LCML_QueueBuffer(pLcmlHandle->pCodecinterfacehandle,
                                                      EMMCodecOuputBuffer,
                                                      pBufHeader->pBuffer,
                                                      pBufHeader->nAllocLen,
                                                      0,
                                                      (OMX_U8 *) pLcmlHdr->pOpParam,
                                                      sizeof(AACDEC_UAlgOutBufParamStruct),
                                                      pBufHeader->pBuffer);
                                if (eError != OMX_ErrorNone ) {
                                    OMX_ERROR4(pComponentPrivate->dbg, "%d :: Comp:: SetBuff OP: Error Occurred\n", __LINE__);
                                    eError = OMX_ErrorHardware;
                                    pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                                pComponentPrivate->pHandle->pApplicationPrivate,
                                                                OMX_EventError,
                                                                eError,
                                                                OMX_TI_ErrorSevere,
                                                                NULL);
                                    goto EXIT;
                                }

                                pComponentPrivate->lcml_nCntOp++;
                                pComponentPrivate->lcml_nOpBuf++;
                                pComponentPrivate->num_Op_Issued++;
                        }
                        else{
                            pComponentPrivate->pOutputBufHdrPending[pComponentPrivate->nNumOutputBufPending++] = pBufHeader;
                            OMX_PRDSP2(pComponentPrivate->dbg, "Reconfig:: byPassDSP!!\n");
                        }
                    }else{
                        OMX_PRBUFFER2(pComponentPrivate->dbg, ":: %d %s DSP is stopping, returning output buffer \n",
                                      __LINE__, __FUNCTION__);
                        pComponentPrivate->cbInfo.FillBufferDone (pComponentPrivate->pHandle,
                                                                  pComponentPrivate->pHandle->pApplicationPrivate,
                                                                  pBufHeader);
                        pComponentPrivate->nFillBufferDoneCount++;
                        SignalIfAllBuffersAreReturned(pComponentPrivate);
                    }
                }
            }
            else if (pComponentPrivate->curState == OMX_StatePause) {

                pComponentPrivate->pOutputBufHdrPending[pComponentPrivate->nNumOutputBufPending++] = pBufHeader;
            }
        }
        if (pComponentPrivate->bFlushOutputPortCommandPending) {
            OMX_SendCommand( pComponentPrivate->pHandle,
                                  OMX_CommandFlush,
                                  1,NULL);
        }
    }
    else {
        OMX_PRBUFFER2(pComponentPrivate->dbg, "%d : BufferHeader %p, Buffer %p Unknown ..........\n",__LINE__,pBufHeader, pBufHeader->pBuffer);
        eError = OMX_ErrorBadParameter;
    }
 EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, "%d : Exiting from  HandleDataBuf_FromApp: %x \n",__LINE__,eError);
    if(eError == OMX_ErrorBadParameter) {
        OMX_ERROR4(pComponentPrivate->dbg, "%d : Error = OMX_ErrorBadParameter\n",__LINE__);
    }
    return eError;
}

/* ================================================================================= * */
/**
* @fn AACDEC_GetBufferDirection() function determines whether it is input buffer or
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

OMX_ERRORTYPE AACDEC_GetBufferDirection(OMX_BUFFERHEADERTYPE *pBufHeader,
                                                         OMX_DIRTYPE *eDir)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    AACDEC_COMPONENT_PRIVATE *pComponentPrivate = pBufHeader->pPlatformPrivate;
    OMX_U32 nBuf = pComponentPrivate->pInputBufferList->numBuffers;
    OMX_BUFFERHEADERTYPE *pBuf = NULL;
    int flag = 1;
    OMX_U32 i=0;

    for(i=0; i<nBuf; i++) {
        pBuf = pComponentPrivate->pInputBufferList->pBufHdr[i];
        if(pBufHeader == pBuf) {
            *eDir = OMX_DirInput;
            OMX_PRINT1(pComponentPrivate->dbg, "%d :: Buffer %p is INPUT BUFFER\n",__LINE__, pBufHeader);
            flag = 0;
            goto EXIT;
        }
    }

    nBuf = pComponentPrivate->pOutputBufferList->numBuffers;

    for(i=0; i<nBuf; i++) {
        pBuf = pComponentPrivate->pOutputBufferList->pBufHdr[i];
        if(pBufHeader == pBuf) {
            *eDir = OMX_DirOutput;
            OMX_PRINT1(pComponentPrivate->dbg, "%d :: Buffer %p is OUTPUT BUFFER\n",__LINE__, pBufHeader);
            flag = 0;
            goto EXIT;
        }
    }

    if (flag == 1) {
        AACDEC_OMX_ERROR_EXIT(eError, OMX_ErrorBadParameter,
                              "Buffer Not Found in List : OMX_ErrorBadParameter");
    }

 EXIT:
    return eError;
}

/* ================================================================================= * */
/**
* @fn AACDEC_LCML_Callback() function is callback which is called by LCML whenever
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
OMX_ERRORTYPE AACDEC_LCML_Callback (TUsnCodecEvent event,void * args [10])
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U8 *pBuffer = args[1];
#ifdef UNDER_CE
    OMX_U8 i;
#endif
    OMX_U32 pValues[4];
    AACD_LCML_BUFHEADERTYPE *pLcmlHdr;
    OMX_COMPONENTTYPE *pHandle = NULL;
    LCML_DSP_INTERFACE *pLcmlHandle;
    AACDEC_COMPONENT_PRIVATE* pComponentPrivate = NULL;
	OMX_U16 i;

#ifdef RESOURCE_MANAGER_ENABLED
    OMX_ERRORTYPE rm_error = OMX_ErrorNone;
#endif
    static double time_stmp = 0;
    FILE * fOutAAC = NULL; 
    FILE * fOutPCM = NULL; 

    pComponentPrivate = (AACDEC_COMPONENT_PRIVATE*)((LCML_DSP_INTERFACE*)args[6])->pComponentPrivate;

    switch(event) {

    case EMMCodecDspError:
        OMX_PRDSP2(pComponentPrivate->dbg, "[LCML CALLBACK EVENT]  EMMCodecDspError\n");
        break;

    case EMMCodecInternalError:
        OMX_PRDSP2(pComponentPrivate->dbg, "[LCML CALLBACK EVENT]  EMMCodecInternalError\n");
        break;

    case EMMCodecInitError:
        OMX_PRDSP2(pComponentPrivate->dbg, "[LCML CALLBACK EVENT]  EMMCodecInitError\n");
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

    if(event == EMMCodecBufferProcessed){
        if( args[0] == (void *)EMMCodecInputBuffer) {
            OMX_PRBUFFER2(pComponentPrivate->dbg, " :: Inside the LCML_Callback EMMCodecInputBuffer\n");
            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: Input: pBuffer = %p\n",__LINE__, pBuffer);
            eError = AACDEC_GetCorresponding_LCMLHeader(pComponentPrivate, pBuffer, OMX_DirInput, &pLcmlHdr);
            if (eError != OMX_ErrorNone) {
                OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error: Invalid Buffer Came ...\n",__LINE__);
                goto EXIT;
            }
            OMX_PRCOMM2(pComponentPrivate->dbg, "%d :: Input: pLcmlHeader = %p\n",__LINE__, pLcmlHdr);
            OMX_PRCOMM2(pComponentPrivate->dbg, "%d :: Input: pLcmlHdr->eDir = %u\n",__LINE__, pLcmlHdr->eDir);
            OMX_PRCOMM2(pComponentPrivate->dbg, "%d :: Input: *pLcmlHdr->eDir = %u\n",__LINE__, pLcmlHdr->eDir);
            OMX_PRCOMM2(pComponentPrivate->dbg, "%d :: Input: Filled Len = %ld\n",__LINE__, pLcmlHdr->pBufHdr->nFilledLen);
#ifdef __PERF_INSTRUMENTATION__
            PERF_ReceivedFrame(pComponentPrivate->pPERFcomp,
                               PREF(pLcmlHdr->pBufHdr,pBuffer),
                               0,
                               PERF_ModuleCommonLayer);
#endif
            pComponentPrivate->lcml_nCntIpRes++;
            AACDEC_ClearPending(pComponentPrivate,pLcmlHdr->pBufHdr,OMX_DirInput,__LINE__);

#ifdef __PERF_INSTRUMENTATION__
			PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                          PREF(pLcmlHdr->pBufHdr,pBuffer),
                          0,
                          PERF_ModuleHLMM);
#endif

            pComponentPrivate->cbInfo.EmptyBufferDone (pComponentPrivate->pHandle,
                                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                                       pLcmlHdr->pBufHdr);
            pComponentPrivate->nEmptyBufferDoneCount++;
            SignalIfAllBuffersAreReturned(pComponentPrivate);
            pComponentPrivate->lcml_nIpBuf--;
            pComponentPrivate->app_nBuf++;

        } else if (args[0] == (void *)EMMCodecOuputBuffer) {
            OMX_PRINT2(pComponentPrivate->dbg, " :: Inside the LCML_Callback EMMCodecOuputBuffer\n");
            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: Output: pBufferr = %p\n",__LINE__, pBuffer);
            if (!AACDEC_IsValid(pComponentPrivate,pBuffer,OMX_DirOutput)) {
#ifdef __PERF_INSTRUMENTATION__
                PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                  pComponentPrivate->pOutputBufferList->pBufHdr[pComponentPrivate->nInvalidFrameCount]->pBuffer,
                                  pComponentPrivate->pOutputBufferList->pBufHdr[pComponentPrivate->nInvalidFrameCount]->nFilledLen,
                                  PERF_ModuleHLMM);
#endif
                pComponentPrivate->cbInfo.FillBufferDone (pComponentPrivate->pHandle,
                                                          pComponentPrivate->pHandle->pApplicationPrivate,
                                                          pComponentPrivate->pOutputBufferList->pBufHdr[pComponentPrivate->nInvalidFrameCount++]
                                                          );
                pComponentPrivate->nFillBufferDoneCount++;
                pComponentPrivate->numPendingBuffers--;
                SignalIfAllBuffersAreReturned(pComponentPrivate);
            } else{
                pComponentPrivate->nOutStandingFillDones++;
                eError = AACDEC_GetCorresponding_LCMLHeader(pComponentPrivate, pBuffer, OMX_DirOutput, &pLcmlHdr);
                if (eError != OMX_ErrorNone) {
                    OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error: Invalid Buffer Came ...\n",__LINE__);
                    goto EXIT;
                }
                pLcmlHdr->pBufHdr->nFilledLen = (int)args[8];
                OMX_PRCOMM2(pComponentPrivate->dbg, "%d :: Output: pLcmlHeader = %p\n",__LINE__, pLcmlHdr);
                OMX_PRCOMM2(pComponentPrivate->dbg, "%d :: Output: pLcmlHdr->eDir = %u\n",__LINE__, pLcmlHdr->eDir);
                OMX_PRCOMM2(pComponentPrivate->dbg, "%d :: Output: Filled Len = %ld\n",__LINE__, pLcmlHdr->pBufHdr->nFilledLen);
                OMX_PRCOMM2(pComponentPrivate->dbg, "%d :: Output: pLcmlHeader->pBufHdr = %p\n",__LINE__, pLcmlHdr->pBufHdr);
                pComponentPrivate->lcml_nCntOpReceived++;
                pComponentPrivate->first_output_buf_rcv = 1;
#ifdef __PERF_INSTRUMENTATION__
                PERF_ReceivedFrame(pComponentPrivate->pPERFcomp,
                                   PREF(pLcmlHdr->pBufHdr,pBuffer),
                                   PREF(pLcmlHdr->pBufHdr,nFilledLen),
                                   PERF_ModuleCommonLayer);
                pComponentPrivate->nLcml_nCntOpReceived++;
                if ((pComponentPrivate->nLcml_nCntIp >= 1) && (pComponentPrivate->nLcml_nCntOpReceived == 1)) {
                    PERF_Boundary(pComponentPrivate->pPERFcomp,
                                  PERF_BoundaryStart | PERF_BoundarySteadyState);
                }
#endif
                AACDEC_ClearPending(pComponentPrivate,pLcmlHdr->pBufHdr,OMX_DirOutput,__LINE__);
			if (pComponentPrivate->pMarkData) {
				pLcmlHdr->pBufHdr->pMarkData = pComponentPrivate->pMarkData;
				pLcmlHdr->pBufHdr->hMarkTargetComponent = pComponentPrivate->hMarkTargetComponent;
			}
			pComponentPrivate->num_Reclaimed_Op_Buff++;
            if (pLcmlHdr->pOpParam->isLastBuffer){
				OMX_PRBUFFER2(pComponentPrivate->dbg, "%d : UTIL: Adding EOS flag to the output buffer\n",__LINE__);
				pLcmlHdr->pBufHdr->nFlags |= OMX_BUFFERFLAG_EOS;
				OMX_PRBUFFER2(pComponentPrivate->dbg, "%d : UTIL:: pLcmlHdr->pBufHdr = %p\n",__LINE__,pLcmlHdr->pBufHdr);
				OMX_PRBUFFER2(pComponentPrivate->dbg, "%d : UTIL:: pLcmlHdr->pBufHdr->nFlags = %x\n",__LINE__,(int)pLcmlHdr->pBufHdr->nFlags);
				pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                   OMX_EventBufferFlag,
                                                   pLcmlHdr->pBufHdr->nOutputPortIndex,
                                                   pLcmlHdr->pBufHdr->nFlags, NULL);
				pComponentPrivate->bIsEOFSent = 0;
				OMX_PRINT2(pComponentPrivate->dbg, "%d : UTIL: EOS flag has been propagated\n",__LINE__);
			}

			OMX_PRBUFFER2(pComponentPrivate->dbg, "pLcmlHdr->pBufHdr = 0x%p\n",pLcmlHdr->pBufHdr);

			if(pComponentPrivate->framemode){
				/* Copying time stamp information to output buffer */
				pLcmlHdr->pBufHdr->nTimeStamp = (OMX_TICKS)pComponentPrivate->arrBufIndex[pComponentPrivate->OpBufindex];
			}else{
            if(pComponentPrivate->first_buff == 1){
                pComponentPrivate->first_buff = 2;
                pLcmlHdr->pBufHdr->nTimeStamp = pComponentPrivate->first_TS;
                pComponentPrivate->temp_TS = pLcmlHdr->pBufHdr->nTimeStamp;
            }else{
                if(pComponentPrivate->pcmParams->nChannels == 2) {/* OMX_AUDIO_ChannelModeStereo */
                    time_stmp = pLcmlHdr->pBufHdr->nFilledLen / (2 * (pComponentPrivate->pcmParams->nBitPerSample / 8));
                }else {/* OMX_AUDIO_ChannelModeMono */
                    time_stmp = pLcmlHdr->pBufHdr->nFilledLen / (1 * (pComponentPrivate->pcmParams->nBitPerSample / 8));
                }
                time_stmp = (time_stmp / pComponentPrivate->pcmParams->nSamplingRate) * 1000000;
                /* Update time stamp information */
                pComponentPrivate->temp_TS += (OMX_U32)time_stmp;
                pLcmlHdr->pBufHdr->nTimeStamp = pComponentPrivate->temp_TS;
			}
            }
                        OMX_PRBUFFER2(pComponentPrivate->dbg, "outs-%lld\n",pLcmlHdr->pBufHdr->nTimeStamp);

  			/*Copying tick count information to output buffer*/
              pLcmlHdr->pBufHdr->nTickCount = (OMX_U32)pComponentPrivate->arrBufIndexTick[pComponentPrivate->OpBufindex];
              pComponentPrivate->OpBufindex++;
              pComponentPrivate->OpBufindex %= pComponentPrivate->pPortDef[OMX_DirInput]->nBufferCountActual;


#ifdef __PERF_INSTRUMENTATION__
				PERF_SendingBuffer(pComponentPrivate->pPERFcomp,
                               pLcmlHdr->pBufHdr->pBuffer,
                               pLcmlHdr->pBufHdr->nFilledLen,
                               PERF_ModuleHLMM);
#endif

                if(pComponentPrivate->reconfigOutputPort){
                    
                }else{
                    pComponentPrivate->cbInfo.FillBufferDone (pComponentPrivate->pHandle,
                                                              pComponentPrivate->pHandle->pApplicationPrivate,
                                                              pLcmlHdr->pBufHdr
                                                              );
                    pComponentPrivate->nFillBufferDoneCount++;
                    SignalIfAllBuffersAreReturned(pComponentPrivate);
                }

                pComponentPrivate->nOutStandingFillDones--;
                pComponentPrivate->lcml_nOpBuf--;
                pComponentPrivate->app_nBuf++;

            }
        }
    } else if(event == EMMCodecProcessingStoped) {
        /* If there are any buffers still marked as pending they must have
           been queued after the socket node was stopped */

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
		pComponentPrivate->nOutStandingFillDones--;
		pComponentPrivate->pOutputBufHdrPending[i] = NULL;
                pComponentPrivate->nFillBufferDoneCount++;
                SignalIfAllBuffersAreReturned(pComponentPrivate);
	}
        pComponentPrivate->nNumOutputBufPending=0;
	pthread_mutex_lock(&pComponentPrivate->codecStop_mutex);
        if(pComponentPrivate->codecStop_waitingsignal == 0){
            pComponentPrivate->codecStop_waitingsignal = 1;
            pthread_cond_signal(&pComponentPrivate->codecStop_threshold);
             OMX_PRDSP2(pComponentPrivate->dbg,"stop ack. received. stop waiting for sending disable command completed\n");
        }
	  pthread_mutex_unlock(&pComponentPrivate->codecStop_mutex);
            OMX_PRDSP2(pComponentPrivate->dbg, "setting state to idle after EMMCodecProcessingStoped event\n\n");
            pComponentPrivate->curState = OMX_StateIdle;

#ifdef RESOURCE_MANAGER_ENABLED
            rm_error = RMProxy_NewSendCommand(pComponentPrivate->pHandle, RMProxy_StateSet, OMX_AAC_Decoder_COMPONENT, OMX_StateIdle, 3456, NULL);
#endif

            if((pComponentPrivate->nEmptyThisBufferCount != pComponentPrivate->nEmptyBufferDoneCount) ||
               (pComponentPrivate->nFillThisBufferCount != pComponentPrivate->nFillBufferDoneCount)) {
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
                OMX_PRINT1(pComponentPrivate->dbg, "OMX has returned all input and output buffers");
            }

            if (pComponentPrivate->bPreempted == 0) {
                pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandStateSet,
                                                       pComponentPrivate->curState,
                                                       NULL);
            }
            else {
                pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                                       OMX_EventError,
                                                       OMX_ErrorResourcesPreempted,
                                                       OMX_TI_ErrorMajor,
                                                       NULL);
                                OMX_ERROR4(pComponentPrivate->dbg, "Error: pre-empted\n");
            }
            pComponentPrivate->bNoIdleOnStop = OMX_FALSE;
    } else if(event == EMMCodecAlgCtrlAck) {
        OMX_PRDSP2(pComponentPrivate->dbg, "GOT MESSAGE USN_DSPACK_ALGCTRL \n");
    } else if (event == EMMCodecDspError) {
        OMX_PRSTATE2(pComponentPrivate->dbg, "%d :: commandedState  = %d\n",__LINE__,(int)args[0]);
        OMX_PRDSP2(pComponentPrivate->dbg, "%d :: arg4 = %d\n",__LINE__,(int)args[4]);
        OMX_PRDSP2(pComponentPrivate->dbg, "%d :: arg5 = %d\n",__LINE__,(int)args[5]);
        OMX_PRDSP2(pComponentPrivate->dbg, "%d ::UTIL: EMMCodecDspError Here\n",__LINE__);
        switch ( (OMX_U32) args [4])
        {
            /* USN_ERR_NONE,: Indicates that no error encountered during execution of the command and the command execution completed succesfully.
             * USN_ERR_WARNING,: Indicates that process function returned a warning. The exact warning is returned in Arg2 of this message.
             * USN_ERR_PROCESS,: Indicates that process function returned a error type. The exact error type is returnd in Arg2 of this message.
             * USN_ERR_PAUSE,: Indicates that execution of pause resulted in error.
             * USN_ERR_STOP,: Indicates that execution of stop resulted in error.
             * USN_ERR_ALGCTRL,: Indicates that execution of alg control resulted in error.
             * USN_ERR_STRMCTRL,: Indiactes the execution of STRM control command, resulted in error.
             * USN_ERR_UNKNOWN_MSG,: Indicates that USN received an unknown command. */

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
                AACDEC_HandleUSNError (pComponentPrivate, (OMX_U32)args[5]);
                break;
            default:
                break;
        }
    } else if (event == EMMCodecStrmCtrlAck) {
        OMX_PRDSP2(pComponentPrivate->dbg, "%d :: GOT MESSAGE USN_DSPACK_STRMCTRL ----\n",__LINE__);
        if (args[1] == (void *)USN_STRMCMD_FLUSH) {
            pHandle = pComponentPrivate->pHandle;
            if ( args[2] == (void *)EMMCodecInputBuffer) {
                if (args[0] == (void *)USN_ERR_NONE ) {
                    OMX_PRCOMM2(pComponentPrivate->dbg, "Flushing input port in lcml_callback %d\n",__LINE__);

                    for (i=0; i < pComponentPrivate->nNumInputBufPending; i++) {
#ifdef __PERF_INSTRUMENTATION__
                        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                          PREF(pComponentPrivate->pInputBufHdrPending[i],pBuffer),
                                          0,
                                          PERF_ModuleHLMM);
#endif
                   pComponentPrivate->cbInfo.EmptyBufferDone (
                                      pComponentPrivate->pHandle,
                                      pComponentPrivate->pHandle->pApplicationPrivate,
                                      pComponentPrivate->pInputBufHdrPending[i]);

                    pComponentPrivate->nEmptyBufferDoneCount++;
                    pComponentPrivate->pInputBufHdrPending[i] = NULL;
                    SignalIfAllBuffersAreReturned(pComponentPrivate);
                   
					}
                    pComponentPrivate->nNumInputBufPending=0;
                    
                    pthread_mutex_lock(&pComponentPrivate->codecFlush_mutex);
                    if(pComponentPrivate->codecFlush_waitingsignal == 0){
                        pComponentPrivate->codecFlush_waitingsignal = 1; 
                        pthread_cond_signal(&pComponentPrivate->codecFlush_threshold);
                        OMX_PRCOMM2(pComponentPrivate->dbg, "flush ack. received. for input port\n");
                    }     
                    pthread_mutex_unlock(&pComponentPrivate->codecFlush_mutex);
                    
                    pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventCmdComplete,
                                                           OMX_CommandFlush,
                                                           INPUT_PORT_AACDEC,
                                                           NULL);
                } else {
                    OMX_ERROR4(pComponentPrivate->dbg, "LCML reported error while flushing input port\n");
                    goto EXIT;
                }
            }
            else if ( args[2] == (void *)EMMCodecOuputBuffer) {
                if (args[0] == (void *)USN_ERR_NONE ) {
                    OMX_PRCOMM2(pComponentPrivate->dbg, "Flushing output port in lcml_callback %d\n",__LINE__);
                    for (i=0; i < pComponentPrivate->nNumOutputBufPending; i++) {
#ifdef __PERF_INSTRUMENTATION__
                        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                          PREF(pComponentPrivate->pOutputBufHdrPending[i],pBuffer),
                                          PREF(pComponentPrivate->pOutputBufHdrPending[i],nFilledLen),
                                          PERF_ModuleHLMM);
#endif

                        pComponentPrivate->cbInfo.FillBufferDone (pComponentPrivate->pHandle,
                                                                  pComponentPrivate->pHandle->pApplicationPrivate,
                                                                  pComponentPrivate->pOutputBufHdrPending[i]
                                                                  );
                        pComponentPrivate->nOutStandingFillDones--;
                        pComponentPrivate->nFillBufferDoneCount++;
                        SignalIfAllBuffersAreReturned(pComponentPrivate);
                        pComponentPrivate->pOutputBufHdrPending[i] = NULL;
                    }
                    pComponentPrivate->nNumOutputBufPending=0;

                    pthread_mutex_lock(&pComponentPrivate->codecFlush_mutex);
                    if(pComponentPrivate->codecFlush_waitingsignal == 0){
                        pComponentPrivate->codecFlush_waitingsignal = 1;
                        pthread_cond_signal(&pComponentPrivate->codecFlush_threshold);
                        OMX_PRCOMM2(pComponentPrivate->dbg, "flush ack. received. for output port\n");
                    }
                    pthread_mutex_unlock(&pComponentPrivate->codecFlush_mutex);
                    pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                           pComponentPrivate->pHandle->pApplicationPrivate,
                                                           OMX_EventCmdComplete,
                                                           OMX_CommandFlush,
                                                           OUTPUT_PORT_AACDEC,
                                                           NULL);

                } else {
                    OMX_ERROR4(pComponentPrivate->dbg, "LCML reported error while flushing output port\n");
                    goto EXIT;
                }
            }
        }
    }else if (event == EMMCodecProcessingPaused) {
        pComponentPrivate->curState = OMX_StatePause;
        pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle, pComponentPrivate->pHandle->pApplicationPrivate,
                                               OMX_EventCmdComplete, OMX_CommandStateSet,
                                               pComponentPrivate->curState, NULL);
    }
#ifdef _ERROR_PROPAGATION__
    else if (event == EMMCodecInitError){
        /* Cheking for MMU_fault */
        if((args[4] == (void*)USN_ERR_UNKNOWN_MSG) && (args[5] == (void*)NULL)) {
            OMX_ERROR4(pComponentPrivate->dbg, "%d :: UTIL: MMU_Fault \n",__LINE__);
            pComponentPrivate->bIsInvalidState=OMX_TRUE;
            pComponentPrivate->curState = OMX_StateInvalid;
            pHandle = pComponentPrivate->pHandle;
            pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                   pHandle->pApplicationPrivate,
                                                   OMX_EventError,
                                                   OMX_ErrorStreamCorrupt,
                                                   OMX_TI_ErrorSevere,
                                                   NULL);
        }
    }
    else if (event ==EMMCodecInternalError){
        /* Cheking for MMU_fault */
        if((args[4] == (void*)USN_ERR_UNKNOWN_MSG) && (args[5] == (void*)NULL)) {
            OMX_ERROR4(pComponentPrivate->dbg, "%d :: UTIL: MMU_Fault \n",__LINE__);
            pComponentPrivate->bIsInvalidState=OMX_TRUE;
            pComponentPrivate->curState = OMX_StateInvalid;
            pHandle = pComponentPrivate->pHandle;
            pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                   pHandle->pApplicationPrivate,
                                                   OMX_EventError,
                                                   OMX_ErrorStreamCorrupt,
                                                   OMX_TI_ErrorSevere,
                                                   NULL);
        }
    }
#endif
 EXIT:
    return eError;
}


/* ================================================================================= * */
/**
* @fn AACDEC_GetCorresponding_LCMLHeader() function gets the corresponding LCML
* header from the actual data buffer for required processing.
*
* @param *pBuffer This is the data buffer pointer.
*
* @param eDir   This is direction of buffer. Input/Output.
*
* @param *AACD_LCML_BUFHEADERTYPE  This is pointer to LCML Buffer Header.
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
OMX_ERRORTYPE AACDEC_GetCorresponding_LCMLHeader(AACDEC_COMPONENT_PRIVATE* pComponentPrivate,
                                        OMX_U8 *pBuffer,
                                        OMX_DIRTYPE eDir,
                                        AACD_LCML_BUFHEADERTYPE **ppLcmlHdr)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    AACD_LCML_BUFHEADERTYPE *pLcmlBufHeader;
    int nIpBuf=0, nOpBuf=0, i=0;

    while (!pComponentPrivate->bInitParamsInitialized) {
#ifndef UNDER_CE
        sched_yield();
#else
        Sleep(0);
#endif
    }


    if(eDir == OMX_DirInput) {
        OMX_PRDSP2(pComponentPrivate->dbg, "%d :: In GetCorresponding_LCMLHeader()\n",__LINE__);
        nIpBuf = pComponentPrivate->pInputBufferList->numBuffers;
        pLcmlBufHeader = pComponentPrivate->pLcmlBufHeader[INPUT_PORT_AACDEC];

        for(i=0; i<nIpBuf; i++) {
            OMX_PRBUFFER2(pComponentPrivate->dbg, "pBuffer = %p\n",pBuffer);
            OMX_PRBUFFER2(pComponentPrivate->dbg, "pLcmlBufHeader->pBufHdr->pBuffer = %p\n",pLcmlBufHeader->pBufHdr->pBuffer);
            if(pBuffer == pLcmlBufHeader->pBufHdr->pBuffer) {
                *ppLcmlHdr = pLcmlBufHeader;
                OMX_PRINT1(pComponentPrivate->dbg, "%d::Corresponding LCML Header Found\n",__LINE__);
                goto EXIT;
            }
            pLcmlBufHeader++;
        }
    } else if (eDir == OMX_DirOutput) {
        i = 0;
        nOpBuf = pComponentPrivate->pOutputBufferList->numBuffers;
        pLcmlBufHeader = pComponentPrivate->pLcmlBufHeader[OUTPUT_PORT_AACDEC];
        OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: nOpBuf = %d\n",__LINE__,nOpBuf);

        for(i=0; i<nOpBuf; i++) {
            OMX_PRBUFFER2(pComponentPrivate->dbg, "pBuffer = %p\n",pBuffer);
            OMX_PRBUFFER2(pComponentPrivate->dbg, "pLcmlBufHeader->pBufHdr->pBuffer = %p\n",pLcmlBufHeader->pBufHdr->pBuffer);
            if(pBuffer == pLcmlBufHeader->pBufHdr->pBuffer) {
                *ppLcmlHdr = pLcmlBufHeader;
                OMX_PRINT1(pComponentPrivate->dbg, "%d::Corresponding LCML Header Found\n",__LINE__);
                goto EXIT;
            }
            pLcmlBufHeader++;
        }
    } else {
        OMX_ERROR2(pComponentPrivate->dbg, "%d:: Invalid Buffer Type :: exiting...\n",__LINE__);
    }

 EXIT:
    return eError;
}

/* ================================================================================= * */
/**
* @fn AACDEC_GetLCMLHandle() function gets the LCML handle and interacts with LCML
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
OMX_HANDLETYPE AACDEC_GetLCMLHandle(AACDEC_COMPONENT_PRIVATE* pComponentPrivate)
{
    /* This must be taken care by WinCE */
    OMX_HANDLETYPE pHandle = NULL;
    OMX_ERRORTYPE eError;
    void *handle;
    OMX_ERRORTYPE (*fpGetHandle)(OMX_HANDLETYPE);
    char *error;

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
        OMX_ERROR4(pComponentPrivate->dbg, "eError != OMX_ErrorNone...\n");
        pHandle = NULL;
        goto EXIT;
    }
    ((LCML_DSP_INTERFACE*)pHandle)->pComponentPrivate = pComponentPrivate;

 EXIT:
    return pHandle;
}
#else
OMX_HANDLETYPE AACDEC_GetLCMLHandle(AACDEC_COMPONENT_PRIVATE* pComponentPrivate)
{
    /* This must be taken care by WinCE */
    OMX_HANDLETYPE pHandle = NULL;
    typedef OMX_ERRORTYPE (*LPFNDLLFUNC1)(OMX_HANDLETYPE);
    OMX_ERRORTYPE eError;
    HINSTANCE hDLL;               // Handle to DLL
    LPFNDLLFUNC1 fpGetHandle1;



    hDLL = LoadLibraryEx(TEXT("OAF_BML.dll"), NULL,0);
    if (hDLL == NULL)
        {
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

/* ========================================================================== */
/**
* @AACDEC_CleanupInitParams() This function is called by the component during
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

void AACDEC_CleanupInitParams(OMX_HANDLETYPE pComponent)
{
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    AACDEC_COMPONENT_PRIVATE *pComponentPrivate = (AACDEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    AACD_LCML_BUFHEADERTYPE *pTemp_lcml;
    OMX_U32 nIpBuf = 0;
    OMX_U32 nOpBuf = 0;
    OMX_U32 i=0;

    OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: Freeing:  pComponentPrivate->strmAttr = %p\n",
                    __LINE__, pComponentPrivate->strmAttr);
    OMX_MEMFREE_STRUCT(pComponentPrivate->strmAttr);

    nIpBuf = pComponentPrivate->nRuntimeInputBuffers;
    pTemp_lcml = pComponentPrivate->pLcmlBufHeader[INPUT_PORT_AACDEC];
    for(i=0; i<nIpBuf; i++) {
        OMX_PRBUFFER2(pComponentPrivate->dbg, ":: Freeing: pTemp_lcml->pIpParam = %p\n",pTemp_lcml->pIpParam);
        OMX_MEMFREE_STRUCT_DSPALIGN(pTemp_lcml->pIpParam, AACDEC_UAlgInBufParamStruct);
        pTemp_lcml++;
    }

    OMX_PRBUFFER2(pComponentPrivate->dbg, ":: Freeing pComponentPrivate->pLcmlBufHeader[INPUT_PORT_AACDEC] = %p\n",
                    pComponentPrivate->pLcmlBufHeader[INPUT_PORT_AACDEC]);
    OMX_MEMFREE_STRUCT(pComponentPrivate->pLcmlBufHeader[INPUT_PORT_AACDEC]);

    nOpBuf = pComponentPrivate->nRuntimeOutputBuffers;
    pComponentPrivate->pLcmlBufHeader[INPUT_PORT_AACDEC] = NULL;
    pTemp_lcml = pComponentPrivate->pLcmlBufHeader[OUTPUT_PORT_AACDEC];
    for(i=0; i<nOpBuf; i++) {
        OMX_PRBUFFER2(pComponentPrivate->dbg, ":: Freeing: pTemp_lcml->pOpParam = %p\n",pTemp_lcml->pOpParam);
        OMX_MEMFREE_STRUCT_DSPALIGN(pTemp_lcml->pOpParam, AACDEC_UAlgOutBufParamStruct);
        pTemp_lcml++;
    }

    OMX_PRBUFFER2(pComponentPrivate->dbg, ":: Freeing: pComponentPrivate->pLcmlBufHeader[OUTPUT_PORT_AACDEC] = %p\n",
                    pComponentPrivate->pLcmlBufHeader[OUTPUT_PORT_AACDEC]);
    OMX_MEMFREE_STRUCT(pComponentPrivate->pLcmlBufHeader[OUTPUT_PORT_AACDEC]);

    OMX_MEMFREE_STRUCT_DSPALIGN(pComponentPrivate->pParams, USN_AudioCodecParams);

    OMX_MEMFREE_STRUCT_DSPALIGN(pComponentPrivate->AACDEC_UALGParam, MPEG4AACDEC_UALGParams);
}
/* ========================================================================== */
/**
* @AACDEC_CleanupInitParamsEx() This function is called by the component during
* portreconfiguration after port disable to free LCML buffers.
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

void AACDEC_CleanupInitParamsEx(OMX_HANDLETYPE pComponent,OMX_U32 indexport)
{
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    AACDEC_COMPONENT_PRIVATE *pComponentPrivate =
        (AACDEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    AACD_LCML_BUFHEADERTYPE *pTemp_lcml;
    OMX_U32 nIpBuf = 0;
    OMX_U32 nOpBuf = 0;
    OMX_U32 i=0;

    if(indexport == 0 || indexport == -1){
        nIpBuf = pComponentPrivate->nRuntimeInputBuffers;
        pTemp_lcml = pComponentPrivate->pLcmlBufHeader[INPUT_PORT_AACDEC];
        for(i=0; i<nIpBuf; i++) {
            OMX_PRBUFFER2(pComponentPrivate->dbg, "Freeing: pIpParam = %p\n",
                          pTemp_lcml->pIpParam);
            OMX_MEMFREE_STRUCT_DSPALIGN(pTemp_lcml->pIpParam, AACDEC_UAlgInBufParamStruct);
            pTemp_lcml++;
        }

        OMX_PRBUFFER2(pComponentPrivate->dbg, "Freeing pLcmlBufHeader[INPUT_PORT_AACDEC] = %p\n",
                      pComponentPrivate->pLcmlBufHeader[INPUT_PORT_AACDEC]);
        OMX_MEMFREE_STRUCT(pComponentPrivate->pLcmlBufHeader[INPUT_PORT_AACDEC]);

    }else if(indexport == 1 || indexport == -1){
        nOpBuf = pComponentPrivate->nRuntimeOutputBuffers;
        pTemp_lcml = pComponentPrivate->pLcmlBufHeader[OUTPUT_PORT_AACDEC];
        for(i=0; i<nOpBuf; i++) {
            OMX_PRBUFFER2(pComponentPrivate->dbg, "Freeing: pOpParam = %p\n",
                          pTemp_lcml->pOpParam);
            OMX_MEMFREE_STRUCT_DSPALIGN(pTemp_lcml->pOpParam, AACDEC_UAlgOutBufParamStruct);
            pTemp_lcml++;
        }

        OMX_PRBUFFER2(pComponentPrivate->dbg, "Freeing: pLcmlBufHeader[OUTPUT_PORT_AACDEC] = %p\n",
                      pComponentPrivate->pLcmlBufHeader[OUTPUT_PORT_AACDEC]);
        OMX_MEMFREE_STRUCT(pComponentPrivate->pLcmlBufHeader[OUTPUT_PORT_AACDEC]);

    }else{
        OMX_ERROR4(pComponentPrivate->dbg, "Bad indexport!\n");
    }
}


/*=======================================================================*/
/*! @fn AACDec_GetSampleRateIndexL

 * @brief Gets the sample rate index

 * @param  aRate : Actual Sampling Freq

 * @Return  Index

 */
/*=======================================================================*/
int AACDec_GetSampleRateIndexL( const int aRate)
{
    int index = 0;
    OMXDBG_PRINT(stderr, PRINT, 2, 0, "%d::aRate:%d\n",__LINE__,aRate);

    switch( aRate ){
    case 96000:
        index = 0;
        break;
    case 88200:
        index = 1;
        break;
    case 64000:
        index = 2;
        break;
    case 48000:
        index = 3;
        break;
    case 44100:
        index = 4;
        break;
    case 32000:
        index = 5;
        break;
    case 24000:
        index = 6;
        break;
    case 22050:
        index = 7;
        break;
    case 16000:
        index = 8;
        break;
    case 12000:
        index = 9;
        break;
    case 11025:
        index = 10;
        break;
    case 8000:
        index = 11;
        break;
    default:
        OMXDBG_PRINT(stderr, PRINT, 2, 0, "Invalid sampling frequency\n");
        break;
    }

    OMXDBG_PRINT(stderr, PRINT, 2, 0, "%d::index:%d\n",__LINE__,index);
    return index;
}


int AACDec_GetSampleRatebyIndex( const int index)
{
    int sample_rate = 0;

    switch( index ){
    case 0:
        sample_rate = 96000;
        break;
    case 1:
        sample_rate = 88200;
        break;
    case 2:
        sample_rate = 64000;
        break;
    case 3:
        sample_rate = 48000;
        break;
    case 4:
        sample_rate = 44100;
        break;
    case 5:
        sample_rate = 32000;
        break;
    case 6:
        sample_rate = 24000;
        break;
    case 7:
        sample_rate = 22050;
        break;
    case 8:
        sample_rate = 16000;
        break;
    case 9:
        sample_rate = 12000;
        break;
    case 10:
        sample_rate = 11025;
        break;
    case 11:
        sample_rate = 8000;
        break;
    default:
        OMXDBG_PRINT(stderr, PRINT, 2, 0, "Invalid index\n");
        break;
    }

    return sample_rate;
}
/* ========================================================================== */
/**
* @AACDEC_SetPending() This function marks the buffer as pending when it is sent
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
void AACDEC_SetPending(AACDEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir, OMX_U32 lineNumber)
{
    OMX_U16 i;

    if (eDir == OMX_DirInput) {
        for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pInputBufferList->pBufHdr[i]) {
                pComponentPrivate->pInputBufferList->bBufferPending[i] = 1;
                OMX_PRBUFFER2(pComponentPrivate->dbg, "*******************INPUT BUFFER %d IS PENDING Line %lu, :%p******************************\n",i,lineNumber,pBufHdr);
            }
        }
    }
    else {
        for (i=0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pOutputBufferList->pBufHdr[i]) {
                pComponentPrivate->pOutputBufferList->bBufferPending[i] = 1;
                OMX_PRBUFFER2(pComponentPrivate->dbg, "*******************OUTPUT BUFFER %d IS PENDING Line, %lu :%p******************************\n",i,lineNumber,pBufHdr);
            }
        }
    }
}

/* ========================================================================== */
/**
* @AACDEC_ClearPending() This function clears the buffer status from pending
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
void AACDEC_ClearPending(AACDEC_COMPONENT_PRIVATE *pComponentPrivate,
            OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir, OMX_U32 lineNumber)
{
    OMX_U16 i;

    if (eDir == OMX_DirInput) {
        for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pInputBufferList->pBufHdr[i]) {
                pComponentPrivate->pInputBufferList->bBufferPending[i] = 0;
                OMX_PRBUFFER2(pComponentPrivate->dbg, "*******************INPUT BUFFER %d IS RECLAIMED Line %lu, :%p******************************\n",i,lineNumber,pBufHdr);
            }
        }
    }
    else {
        for (i=0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pOutputBufferList->pBufHdr[i]) {
                pComponentPrivate->pOutputBufferList->bBufferPending[i] = 0;
                OMX_PRBUFFER2(pComponentPrivate->dbg, "*******************OUTPUT BUFFER %d IS RECLAIMED Line %lu, :%p******************************\n",i,lineNumber,pBufHdr);
            }
        }
    }
}

/* ========================================================================== */
/**
* @AACDEC_IsPending() This function checks whether or not a buffer is pending.
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
OMX_U32 AACDEC_IsPending(AACDEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir)
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


/* ========================================================================== */
/**
* @AACDEC_IsValid() This function identifies whether or not buffer recieved from
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
OMX_U32 AACDEC_IsValid(AACDEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U8 *pBuffer, OMX_DIRTYPE eDir)
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

/* ========================================================================== */
/**
* @AACDECFill_LCMLInitParamsEx() This function initializes the init parameter of
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
OMX_ERRORTYPE AACDECFill_LCMLInitParamsEx(OMX_HANDLETYPE pComponent,OMX_U32 indexport)

{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nIpBuf,nIpBufSize,nOpBuf,nOpBufSize;
    OMX_U16 i;
    OMX_BUFFERHEADERTYPE *pTemp;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    AACDEC_COMPONENT_PRIVATE *pComponentPrivate =
        (AACDEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    AACD_LCML_BUFHEADERTYPE *pTemp_lcml;
    OMX_U32 size_lcml;
    char *ptr;

    nIpBuf = pComponentPrivate->pInputBufferList->numBuffers;
    nOpBuf = pComponentPrivate->pOutputBufferList->numBuffers;
    nIpBufSize = pComponentPrivate->pPortDef[INPUT_PORT_AACDEC]->nBufferSize;
    nOpBufSize = pComponentPrivate->pPortDef[OUTPUT_PORT_AACDEC]->nBufferSize;


    OMX_PRBUFFER2(pComponentPrivate->dbg, "Input Buffer Count = %ld\n",nIpBuf);
    OMX_PRBUFFER2(pComponentPrivate->dbg, "Input Buffer Size = %ld\n",nIpBufSize);
    OMX_PRBUFFER2(pComponentPrivate->dbg, "Output Buffer Count = %ld\n",nOpBuf);
    OMX_PRBUFFER2(pComponentPrivate->dbg, "Output Buffer Size = %ld\n",nOpBufSize);
    OMX_PRBUFFER2(pComponentPrivate->dbg, "Input Buffer Count = %ld\n",nIpBuf);
    OMX_PRBUFFER2(pComponentPrivate->dbg, "Input Buffer Size = %ld\n",nIpBufSize);
    OMX_PRBUFFER2(pComponentPrivate->dbg, "Output Buffer Count = %ld\n",nOpBuf);
    OMX_PRBUFFER2(pComponentPrivate->dbg, "Output Buffer Size = %ld\n",nOpBufSize);


    if(indexport == 0 || indexport == -1){

        OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: bufAlloced = %lu\n",__LINE__,pComponentPrivate->bufAlloced);
        size_lcml = nIpBuf * sizeof(AACD_LCML_BUFHEADERTYPE);

        OMX_MALLOC_SIZE(ptr,size_lcml,char);
        pTemp_lcml = (AACD_LCML_BUFHEADERTYPE *)ptr;

        pComponentPrivate->pLcmlBufHeader[INPUT_PORT_AACDEC] = pTemp_lcml;

        for (i=0; i<nIpBuf; i++) {
            pTemp = pComponentPrivate->pInputBufferList->pBufHdr[i];
            pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);

            pTemp->nAllocLen = nIpBufSize;
            pTemp->nFilledLen = nIpBufSize;
            pTemp->nVersion.s.nVersionMajor = AACDEC_MAJOR_VER;
            pTemp->nVersion.s.nVersionMinor = AACDEC_MINOR_VER;

            pComponentPrivate->nVersion = pTemp->nVersion.nVersion;

            pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
            pTemp->nTickCount = NOT_USED_AACDEC;

            pTemp_lcml->pBufHdr = pTemp;
            pTemp_lcml->eDir = OMX_DirInput;
            pTemp_lcml->pOtherParams[i] = NULL;

            OMX_MALLOC_SIZE_DSPALIGN(pTemp_lcml->pIpParam,
                                   sizeof(AACDEC_UAlgInBufParamStruct),
                                   AACDEC_UAlgInBufParamStruct);

            pTemp_lcml->pIpParam->bLastBuffer = 0;

            pTemp->nFlags = NORMAL_BUFFER_AACDEC;
            ((AACDEC_COMPONENT_PRIVATE *) pTemp->pPlatformPrivate)->pHandle = pHandle;

            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d ::Comp: InBuffHeader[%d] = %p\n", __LINE__, i, pTemp);
            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d ::Comp:  >>>> InputBuffHeader[%d]->pBuffer = %p\n",
                          __LINE__, i, pTemp->pBuffer);
            OMX_PRDSP2(pComponentPrivate->dbg, "%d ::Comp: Ip : pTemp_lcml[%d] = %p\n", __LINE__, i, pTemp_lcml);

            pTemp_lcml++;
        }
    }
    if(indexport == 1 || indexport == -1){

        size_lcml = nOpBuf * sizeof(AACD_LCML_BUFHEADERTYPE);
        OMX_MALLOC_SIZE(pTemp_lcml,size_lcml,AACD_LCML_BUFHEADERTYPE);
        pComponentPrivate->pLcmlBufHeader[OUTPUT_PORT_AACDEC] = pTemp_lcml;

        for (i=0; i<nOpBuf; i++) {
            pTemp = pComponentPrivate->pOutputBufferList->pBufHdr[i];
            pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);

            pTemp->nAllocLen = nOpBufSize;
            pTemp->nFilledLen = nOpBufSize;
            pTemp->nVersion.s.nVersionMajor = AACDEC_MAJOR_VER;
            pTemp->nVersion.s.nVersionMinor = AACDEC_MINOR_VER;

            pComponentPrivate->nVersion = pTemp->nVersion.nVersion;

            pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
            pTemp->nTickCount = NOT_USED_AACDEC;

            pTemp_lcml->pBufHdr = pTemp;
            pTemp_lcml->eDir = OMX_DirOutput;
            pTemp_lcml->pOtherParams[i] = NULL;

            OMX_MALLOC_SIZE_DSPALIGN(pTemp_lcml->pOpParam,
                                   sizeof(AACDEC_UAlgOutBufParamStruct),
                                   AACDEC_UAlgOutBufParamStruct);

            pTemp_lcml->pOpParam->ulFrameCount = DONT_CARE;
            pTemp_lcml->pOpParam->isLastBuffer = 0;
			
            pTemp->nFlags = NORMAL_BUFFER_AACDEC;
            ((AACDEC_COMPONENT_PRIVATE *)pTemp->pPlatformPrivate)->pHandle = pHandle;
            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d ::Comp:  >>>>>>>>>>>>> OutBuffHeader[%d] = %p\n",
                          __LINE__, i, pTemp);
            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d ::Comp:  >>>> OutBuffHeader[%d]->pBuffer = %p\n",
                          __LINE__, i, pTemp->pBuffer);
            OMX_PRDSP2(pComponentPrivate->dbg, "%d ::Comp: Op : pTemp_lcml[%d] = %p\n", __LINE__, i, pTemp_lcml);
            pTemp_lcml++;
        }
    }
    pComponentPrivate->bPortDefsAllocated = 1;

#ifdef __PERF_INSTRUMENTATION__
    pComponentPrivate->nLcml_nCntIp = 0;
    pComponentPrivate->nLcml_nCntOpReceived = 0;
#endif
    pComponentPrivate->bInitParamsInitialized = 1;

 EXIT:
    return eError;
}

/*  =========================================================================*/
/*  func    GetBits                                                          */
/*                                                                           */
/*  desc    Gets aBits number of bits from position aPosition of one buffer  */
/*            and returns the value in a TUint value.                        */
/*  =========================================================================*/
OMX_U32 AACDEC_GetBits(OMX_U32* nPosition, OMX_U8 nBits, OMX_U8* pBuffer, OMX_BOOL bIcreasePosition)
{
    OMX_U32 nOutput;
    OMX_U32 nNumBitsRead = 0;
    OMX_U32 nBytePosition = 0;
    OMX_U8  nBitPosition =  0;
    nBytePosition = *nPosition / 8;
    nBitPosition =  *nPosition % 8;

    if (bIcreasePosition)
        *nPosition += nBits;
    nOutput = ((OMX_U32)pBuffer[nBytePosition] << (24+nBitPosition) );
    nNumBitsRead = nNumBitsRead + (8 - nBitPosition);
    if (nNumBitsRead < nBits)
    {
        nOutput = nOutput | ( pBuffer[nBytePosition + 1] << (16+nBitPosition));
        nNumBitsRead = nNumBitsRead + 8;
    }
    if (nNumBitsRead < nBits)
    {
        nOutput = nOutput | ( pBuffer[nBytePosition + 2] << (8+nBitPosition));
        nNumBitsRead = nNumBitsRead + 8;
    }
    if (nNumBitsRead < nBits)
    {
        nOutput = nOutput | ( pBuffer[nBytePosition + 3] << (nBitPosition));
        nNumBitsRead = nNumBitsRead + 8;
    }
    nOutput = nOutput >> (32 - nBits) ;
    return nOutput;
}
/* ========================================================================== */
/**
* @SignalIfAllBuffersAreReturned() This function send signals if OMX returned all buffers to app 
*
* @param AACDEC_COMPONENT_PRIVATE *pComponentPrivate
*
* @pre None
*
* @post None
*
* @return None
*/
/* ========================================================================== */
void SignalIfAllBuffersAreReturned(AACDEC_COMPONENT_PRIVATE *pComponentPrivate)
{
    if((pComponentPrivate->nEmptyThisBufferCount == pComponentPrivate->nEmptyBufferDoneCount) &&
       (pComponentPrivate->nFillThisBufferCount == pComponentPrivate->nFillBufferDoneCount))
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

void AACDEC_HandleUSNError (AACDEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U32 arg)
{
    OMX_COMPONENTTYPE *pHandle = NULL;
    OMX_U8 pending_buffers = OMX_FALSE;
    OMX_U32 i;
    switch (arg)
    {
        case AACDEC_SBR_CONTENT:
#ifndef ANDROID
            OMX_PRDSP2(pComponentPrivate->dbg, "%d :: LCML_Callback: SBR content detected \n" ,__LINE__);
            if(pComponentPrivate->aacParams->eAACProfile != OMX_AUDIO_AACObjectHE &&
               pComponentPrivate->aacParams->eAACProfile != OMX_AUDIO_AACObjectHE_PS){
                pComponentPrivate->aacParams->eAACProfile = OMX_AUDIO_AACObjectHE;
                pComponentPrivate->AACDEC_UALGParam->iEnablePS =  0;
                pComponentPrivate->AACDEC_UALGParam->DownSampleSbr = 1;

                pValues[0] = IUALG_CMD_SETSTATUS;
                pValues[1] = (OMX_U32)pComponentPrivate->AACDEC_UALGParam;
                pValues[2] = sizeof(MPEG4AACDEC_UALGParams);

                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pComponentPrivate->pLcmlHandle)->pCodecinterfacehandle,
                                           EMMCodecControlAlgCtrl,(void *)pValues);
                if(eError != OMX_ErrorNone) {
                    OMX_ERROR4(pComponentPrivate->dbg, "%d: Error Occurred in Codec StreamControl..\n",__LINE__);
                    pComponentPrivate->curState = OMX_StateInvalid;
                    pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                           pComponentPrivate->pHandle->pApplicationPrivate,
                                                           OMX_EventError,
                                                           eError,
                                                           OMX_TI_ErrorSevere,
                                                           NULL);
                }
#endif
                break;
        case AACDEC_PS_CONTENT:
#ifndef ANDROID
            OMX_PRDSP2(pComponentPrivate->dbg, "%d :: LCML_Callback: PS content detected \n" ,__LINE__);
            if(pComponentPrivate->aacParams->eAACProfile != OMX_AUDIO_AACObjectHE_PS){
                pComponentPrivate->AACDEC_UALGParam->lOutputFormat = EAUDIO_INTERLEAVED;
                pComponentPrivate->AACDEC_UALGParam->DownSampleSbr = 1;
                pComponentPrivate->AACDEC_UALGParam->iEnablePS =  1;

                pValues[0] = IUALG_CMD_SETSTATUS;
                pValues[1] = (OMX_U32)pComponentPrivate->AACDEC_UALGParam;
                pValues[2] = sizeof(MPEG4AACDEC_UALGParams);

                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pComponentPrivate->pLcmlHandle)->pCodecinterfacehandle,
                                           EMMCodecControlAlgCtrl,(void *)pValues);
                if(eError != OMX_ErrorNone) {
                    OMX_ERROR4(pComponentPrivate->dbg, "%d: Error Occurred in Codec StreamControl..\n",__LINE__);
                    pComponentPrivate->curState = OMX_StateInvalid;
                    pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                           pComponentPrivate->pHandle->pApplicationPrivate,
                                                           OMX_EventError,
                                                           OMX_ErrorInvalidState,
                                                           0,
                                                           NULL);
                }
            }
#endif
            break;
        case IAAC_WARN_DATA_CORRUPT:
            OMX_ERROR4(pComponentPrivate->dbg,  "LCML_Callback: Algorithm error, stream corrupt\n");
            break;
        case IUALG_WARN_CONCEALED:
        case IUALG_WARN_UNDERFLOW:
        case IUALG_WARN_OVERFLOW:
        case IUALG_WARN_ENDOFDATA:
            OMX_ERROR4(pComponentPrivate->dbg,  "Algorithm Error" );
            /* all of these are informative messages, Algo can recover, no need to notify the
             * IL Client at this stage of the implementation */
            break;
        case IUALG_WARN_PLAYCOMPLETED:

            {
                OMX_PRINT2(pComponentPrivate->dbg, "%d :: UTIL: IUALG_WARN_PLAYCOMPLETED/USN_ERR_WARNING event received\n", __LINE__);
#ifndef UNDER_CE
                pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                                       OMX_EventBufferFlag,
                                                       (OMX_U32)NULL,
                                                       OMX_BUFFERFLAG_EOS,
                                                       NULL);
                pComponentPrivate->pLcmlBufHeader[0]->pIpParam->bLastBuffer = 0;
#else
                /* add callback to application to indicate SN/USN has completed playing of current set of date */
                pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                                       OMX_EventBufferFlag,
                                                       (OMX_U32)NULL,
                                                       OMX_BUFFERFLAG_EOS,
                                                       NULL);
#endif
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
                OMX_ERROR4(pComponentPrivate->dbg,  "Algorithm Error, cannot recover" );
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

#ifdef RESOURCE_MANAGER_ENABLED
void AACDEC_ResourceManagerCallback(RMPROXY_COMMANDDATATYPE cbData)
{
    OMX_COMMANDTYPE Cmd = OMX_CommandStateSet;
    OMX_STATETYPE state = OMX_StateIdle;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)cbData.hComponent;
    AACDEC_COMPONENT_PRIVATE *pCompPrivate = NULL;

    pCompPrivate = (AACDEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    if (*(cbData.RM_Error) == OMX_RmProxyCallback_ResourcesPreempted) {
        if (pCompPrivate->curState == OMX_StateExecuting ||
            pCompPrivate->curState == OMX_StatePause) {
            write (pCompPrivate->cmdPipe[1], &Cmd, sizeof(Cmd));
            write (pCompPrivate->cmdDataPipe[1], &state ,sizeof(OMX_U32));
            pCompPrivate->bPreempted = 1;
        }
    }
    else if (*(cbData.RM_Error) == OMX_RmProxyCallback_ResourcesAcquired){
        pCompPrivate->cbInfo.EventHandler (pHandle,
                                           pHandle->pApplicationPrivate,
                                           OMX_EventResourcesAcquired, 0,0,
                                           NULL);
    }
}
#endif
