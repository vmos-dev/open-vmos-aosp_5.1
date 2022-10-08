
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
 * @file OMX_Mp3Dec_Utils.c
 *
 * This file implements various utilitiy functions for various activities
 * like handling command from application, callback from LCML etc.
 *
 * @path  $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\mp3_dec\src
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
#include <pthread.h>

/*------- Program Header Files -----------------------------------------------*/
#include "LCML_DspCodec.h"
#include "OMX_Mp3Dec_Utils.h"
#include "mp3decsocket_ti.h"
#include <decode_common_ti.h>
#include "usn.h"

#ifdef RESOURCE_MANAGER_ENABLED
#include <ResourceManagerProxyAPI.h>
#endif

#ifdef UNDER_CE
#define HASHINGENABLE 1
HINSTANCE g_hLcmlDllHandle = NULL;
void sleep(DWORD Duration)
{
    Sleep(Duration);

}
#endif

/* ================================================================================= * */
/**
 * @fn MP3DEC_Fill_LCMLInitParams() fills the LCML initialization structure.
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
OMX_ERRORTYPE MP3DEC_Fill_LCMLInitParams(OMX_HANDLETYPE pComponent,LCML_DSP *plcml_Init,OMX_U16 arr[])
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nIpBuf,nIpBufSize,nOpBuf,nOpBufSize;
    OMX_U32 i;
    OMX_BUFFERHEADERTYPE *pTemp = NULL;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    MP3DEC_COMPONENT_PRIVATE *pComponentPrivate =(MP3DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    MP3D_LCML_BUFHEADERTYPE *pTemp_lcml;
    OMX_U32 size_lcml;
    OMX_U8 *ptr;

    pComponentPrivate->nRuntimeInputBuffers = 0;
    pComponentPrivate->nRuntimeOutputBuffers = 0;
 
    OMX_PRINT1(pComponentPrivate->dbg, "Entered MP3DEC_Fill_LCMLInitParams\n");
    OMX_PRCOMM2(pComponentPrivate->dbg, ":::pComponentPrivate->pPortDef[MP3D_INPUT_PORT]->bPopulated = %d\n",
                  pComponentPrivate->pPortDef[MP3D_INPUT_PORT]->bPopulated);
    OMX_PRCOMM2(pComponentPrivate->dbg, ":::pComponentPrivate->pPortDef[MP3D_INPUT_PORT]->bEnabled = %d\n",
                  pComponentPrivate->pPortDef[MP3D_INPUT_PORT]->bEnabled);
    OMX_PRCOMM2(pComponentPrivate->dbg, ":::pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]->bPopulated = %d\n",
                  pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]->bPopulated);
    OMX_PRCOMM2(pComponentPrivate->dbg, ":::pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]->bEnabled = %d\n",
                  pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]->bEnabled);

    pComponentPrivate->strmAttr = NULL;

    nIpBuf = pComponentPrivate->pInputBufferList->numBuffers;
    pComponentPrivate->nRuntimeInputBuffers = nIpBuf;
    nOpBuf = pComponentPrivate->pOutputBufferList->numBuffers;
    pComponentPrivate->nRuntimeOutputBuffers = nOpBuf;
    nIpBufSize = pComponentPrivate->pPortDef[MP3D_INPUT_PORT]->nBufferSize;
    nOpBufSize = pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]->nBufferSize;


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

    plcml_Init->NodeInfo.AllUUIDs[0].uuid = &MP3DECSOCKET_TI_UUID;
    strcpy ((char*)plcml_Init->NodeInfo.AllUUIDs[0].DllName, MP3DEC_DLL_NAME);
    plcml_Init->NodeInfo.AllUUIDs[0].eDllType = DLL_NODEOBJECT;

    plcml_Init->NodeInfo.AllUUIDs[1].uuid = &MP3DECSOCKET_TI_UUID;
    strcpy ((char*)plcml_Init->NodeInfo.AllUUIDs[1].DllName, MP3DEC_DLL_NAME);
    plcml_Init->NodeInfo.AllUUIDs[1].eDllType = DLL_DEPENDENT;

    plcml_Init->NodeInfo.AllUUIDs[2].uuid = &USN_TI_UUID;
    strcpy ((char*)plcml_Init->NodeInfo.AllUUIDs[2].DllName, MP3DEC_USN_DLL_NAME);
    plcml_Init->NodeInfo.AllUUIDs[2].eDllType = DLL_DEPENDENT;

    plcml_Init->SegID = OMX_MP3DEC_DEFAULT_SEGMENT;
    plcml_Init->Timeout = OMX_MP3DEC_SN_TIMEOUT;
    plcml_Init->Alignment = 0;
    plcml_Init->Priority = OMX_MP3DEC_SN_PRIORITY;
    plcml_Init->ProfileID = -1;

    if(pComponentPrivate->dasfmode == 1) {
#ifndef DSP_RENDERING_ON
        MP3D_OMX_ERROR_EXIT(eError, OMX_ErrorInsufficientResources,
                            "Flag DSP_RENDERING_ON Must Be Defined To Use Rendering");
#else
        LCML_STRMATTR *strmAttr;
        OMX_MALLOC_GENERIC(strmAttr, LCML_STRMATTR);
        OMX_PRBUFFER2(pComponentPrivate->dbg, ": Malloc strmAttr = %p\n",strmAttr);
        pComponentPrivate->strmAttr = strmAttr;
        OMX_PRDSP2(pComponentPrivate->dbg, ":: MP3 DECODER IS RUNNING UNDER DASF MODE \n");

        strmAttr->uSegid = 0;
        strmAttr->uAlignment = 0;
        strmAttr->uTimeout = -1;

	strmAttr->uBufsize = MP3D_OUTPUT_BUFFER_SIZE;
	
	OMX_PRBUFFER2(pComponentPrivate->dbg, "::strmAttr->uBufsize:%d\n",strmAttr->uBufsize);

        strmAttr->uNumBufs = 2;
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
        OMX_PRDSP2(pComponentPrivate->dbg, ":: FILE MODE CREATE PHASE PARAMETERS\n");
        arr[0] = 2;            /* Number of Streams */
        arr[1] = 0;            /* ID of the Input Stream */
        arr[2] = 0;            /* Type of Input Stream DMM (0) / STRM (1) */
#ifndef UNDER_CE
        arr[3] = 4;            /* Number of buffers for Input Stream */
#else
        arr[3] = 1;            /* WinCE Number of buffers for Input Stream */
#endif
        arr[4] = 1;            /* ID of the Output Stream */
        arr[5] = 0;            /* Type of Output Stream  */
#ifndef UNDER_CE
        arr[6] = 4;            /* Number of buffers for Output Stream */
#else
        arr[6] = 1;            /* WinCE Number of buffers for Output Stream */
#endif

        if(pComponentPrivate->pcmParams->nBitPerSample == 24){
            OMX_PRCOMM2(pComponentPrivate->dbg, " PCM 24 bit output\n");
            arr[7] = 24;
        } else {
            OMX_PRCOMM2(pComponentPrivate->dbg, " PCM 16 bit output\n");
            arr[7] = 16;
        }
    
        if(pComponentPrivate->frameMode) {
            OMX_PRMGR2(pComponentPrivate->dbg, " frame mode is on\n");
            arr[8] = 1;   /* frame mode is on */
        } else {
            arr[8] = 0;
            OMX_PRMGR2(pComponentPrivate->dbg, " frame mode is off\n");
        }
        arr[9] = END_OF_CR_PHASE_ARGS;
    } else {
        OMX_PRDSP2(pComponentPrivate->dbg, ":: DASF MODE CREATE PHASE PARAMETERS\n");
        arr[0] = 2;        /* Number of Streams */
        arr[1] = 0;        /* ID of the Input Stream */
        arr[2] = 0;        /* Type of Input Stream DMM (0) / STRM (1) */
        arr[3] = 4;        /* Number of buffers for Input Stream */
        arr[4] = 1;        /* ID of the Output Stream */
        arr[5] = 2;        /* Type of Output Stream  */
        arr[6] = 2;        /* Number of buffers for Output Stream */
        arr[7] = 16;       /*Decoder Output PCM width is 24-bit or 16-bit */
        arr[8] = 0;        /* frame mode off */

        arr[9] = END_OF_CR_PHASE_ARGS;
    }

    plcml_Init->pCrPhArgs = arr;

    OMX_PRBUFFER2(pComponentPrivate->dbg, ":: bufAlloced = %d\n",pComponentPrivate->bufAlloced);
    size_lcml = nIpBuf * sizeof(MP3D_LCML_BUFHEADERTYPE);
    OMX_MALLOC_SIZE(ptr,size_lcml,OMX_U8);
    pTemp_lcml = (MP3D_LCML_BUFHEADERTYPE *)ptr;

    pComponentPrivate->pLcmlBufHeader[MP3D_INPUT_PORT] = pTemp_lcml;

    for (i=0; i<nIpBuf; i++) {
        pTemp = pComponentPrivate->pInputBufferList->pBufHdr[i];
        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);

        pTemp->nAllocLen = nIpBufSize;
        pTemp->nFilledLen = nIpBufSize;
        pTemp->nVersion.s.nVersionMajor = MP3DEC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = MP3DEC_MINOR_VER;

        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = 0;

        pTemp_lcml->pBufHdr = pTemp;
        pTemp_lcml->eDir = OMX_DirInput;
        pTemp_lcml->pOtherParams[i] = NULL;
        OMX_MALLOC_SIZE_DSPALIGN(pTemp_lcml->pIpParam,
                             sizeof(MP3DEC_UAlgInBufParamStruct),
                             MP3DEC_UAlgInBufParamStruct);
        pTemp_lcml->pIpParam->bLastBuffer = 0;

        pTemp->nFlags = NORMAL_BUFFER;
        ((MP3DEC_COMPONENT_PRIVATE *) pTemp->pPlatformPrivate)->pHandle = pHandle;

        OMX_PRBUFFER2(pComponentPrivate->dbg, "::Comp: InBuffHeader[%ld] = %p\n", i, pTemp);
        OMX_PRBUFFER2(pComponentPrivate->dbg, "::Comp:  >>>> InputBuffHeader[%ld]->pBuffer = %p\n", i, pTemp->pBuffer);
        OMX_PRDSP2(pComponentPrivate->dbg, "::Comp: Ip : pTemp_lcml[%ld] = %p\n", i, pTemp_lcml);

        pTemp_lcml++;
    }

    size_lcml = nOpBuf * sizeof(MP3D_LCML_BUFHEADERTYPE);
    OMX_MALLOC_SIZE(pTemp_lcml,size_lcml,MP3D_LCML_BUFHEADERTYPE);
    pComponentPrivate->pLcmlBufHeader[MP3D_OUTPUT_PORT] = pTemp_lcml;

    for (i=0; i<nOpBuf; i++) {
        pTemp = pComponentPrivate->pOutputBufferList->pBufHdr[i];
        pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);

        pTemp->nAllocLen = nOpBufSize;

        OMX_PRBUFFER2(pComponentPrivate->dbg, ":: nOpBufSize = %ld\n", nOpBufSize);

        pTemp->nVersion.s.nVersionMajor = MP3DEC_MAJOR_VER;
        pTemp->nVersion.s.nVersionMinor = MP3DEC_MINOR_VER;
        pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
        pTemp->nTickCount = 0;

        pTemp_lcml->pBufHdr = pTemp;
        pTemp_lcml->eDir = OMX_DirOutput;
        pTemp_lcml->pOtherParams[i] = NULL;
        OMX_MALLOC_SIZE_DSPALIGN(pTemp_lcml->pOpParam,
                             sizeof(MP3DEC_UAlgOutBufParamStruct),
                             MP3DEC_UAlgOutBufParamStruct);
        pTemp_lcml->pOpParam->ulFrameCount = DONT_CARE;
        pTemp_lcml->pOpParam->ulIsLastBuffer = 0;

        pTemp->nFlags = NORMAL_BUFFER;
        ((MP3DEC_COMPONENT_PRIVATE *)pTemp->pPlatformPrivate)->pHandle = pHandle;
        OMX_PRBUFFER2(pComponentPrivate->dbg, "::Comp:  >>>>>>>>>>>>> OutBuffHeader[%ld] = %p\n", i, pTemp);
        OMX_PRBUFFER2(pComponentPrivate->dbg, "::Comp:  >>>> OutBuffHeader[%ld]->pBuffer = %p\n", i, pTemp->pBuffer);
        OMX_PRBUFFER2(pComponentPrivate->dbg, "::Comp: Op : pTemp_lcml[%ld] = %p\n", i, pTemp_lcml);
        pTemp_lcml++;
    }
    pComponentPrivate->bPortDefsAllocated = 1;
    OMX_MALLOC_SIZE_DSPALIGN(pComponentPrivate->pParams,sizeof(USN_AudioCodecParams),
                         USN_AudioCodecParams);
    OMX_MALLOC_SIZE_DSPALIGN(pComponentPrivate->ptAlgDynParams,sizeof(MP3DEC_UALGParams),
                         MP3DEC_UALGParams);

#ifdef __PERF_INSTRUMENTATION__
    pComponentPrivate->nLcml_nCntIp = 0;
    pComponentPrivate->nLcml_nCntOpReceived = 0;
#endif  

    pComponentPrivate->bInitParamsInitialized = 1;

 EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, "Exiting MP3DEC_Fill_LCMLInitParams. error=%d\n", eError);

    return eError;
}


/* ================================================================================= * */
/**
 * @fn Mp3Dec_StartCompThread() starts the component thread. This is internal
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
OMX_ERRORTYPE Mp3Dec_StartCompThread(OMX_HANDLETYPE pComponent)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    MP3DEC_COMPONENT_PRIVATE *pComponentPrivate =
        (MP3DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    int nRet = 0;

#ifdef UNDER_CE
    pthread_attr_t attr;
    memset(&attr, 0, sizeof(attr));
    attr.__inheritsched = PTHREAD_EXPLICIT_SCHED;
    attr.__schedparam.__sched_priority = OMX_AUDIO_DECODER_THREAD_PRIORITY;
#endif

    OMX_PRINT1(pComponentPrivate->dbg, ":: Enetering  Mp3Dec_StartCompThread()\n");

    pComponentPrivate->lcml_nOpBuf = 0;
    pComponentPrivate->lcml_nIpBuf = 0;
    pComponentPrivate->app_nBuf = 0;
    pComponentPrivate->num_Op_Issued = 0;
    pComponentPrivate->num_Sent_Ip_Buff = 0;
    pComponentPrivate->num_Reclaimed_Op_Buff = 0;
    pComponentPrivate->bIsEOFSent = 0;

    nRet = pipe (pComponentPrivate->dataPipe);
    if (0 != nRet) {
        MP3D_OMX_ERROR_EXIT(eError, OMX_ErrorInsufficientResources,
                            "Pipe Creation Failed");
    }

    nRet = pipe (pComponentPrivate->cmdPipe);
    if (0 != nRet) {
        MP3D_OMX_ERROR_EXIT(eError, OMX_ErrorInsufficientResources,
                            "Pipe Creation Failed");
    }

    nRet = pipe (pComponentPrivate->cmdDataPipe);
    if (0 != nRet) {
        MP3D_OMX_ERROR_EXIT(eError, OMX_ErrorInsufficientResources,
                            "Pipe Creation Failed");
    }


#ifdef UNDER_CE
    nRet = pthread_create (&(pComponentPrivate->ComponentThread), &attr,
                           MP3DEC_ComponentThread, pComponentPrivate);
#else
    nRet = pthread_create (&(pComponentPrivate->ComponentThread), NULL,
                           MP3DEC_ComponentThread, pComponentPrivate);
#endif                                       
    if ((0 != nRet) || (!pComponentPrivate->ComponentThread)) {
        MP3D_OMX_ERROR_EXIT(eError, OMX_ErrorInsufficientResources,
                            "Thread Creation Failed");
    }

    pComponentPrivate->bCompThreadStarted = 1;

    OMX_PRINT1(pComponentPrivate->dbg, ":: Exiting from Mp3Dec_StartCompThread()\n");

 EXIT:
    return eError;
}


/* ================================================================================= * */
/**
 * @fn MP3DEC_FreeCompResources() function newfrees the component resources.
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

OMX_ERRORTYPE MP3DEC_FreeCompResources(OMX_HANDLETYPE pComponent)
{
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    MP3DEC_COMPONENT_PRIVATE *pComponentPrivate = (MP3DEC_COMPONENT_PRIVATE *)
        pHandle->pComponentPrivate;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nIpBuf=0, nOpBuf=0;
    int nRet=0;

    OMX_PRINT1(pComponentPrivate->dbg, ":: Mp3Dec_FreeCompResources\n");

    OMX_PRBUFFER2(pComponentPrivate->dbg, ":::pComponentPrivate->bPortDefsAllocated = %ld\n",pComponentPrivate->bPortDefsAllocated);
    if (pComponentPrivate->bPortDefsAllocated) {
        nIpBuf = pComponentPrivate->pInputBufferList->numBuffers;
        nOpBuf = pComponentPrivate->pOutputBufferList->numBuffers;
    }
    OMX_PRCOMM2(pComponentPrivate->dbg, ":: Closing pipess.....\n");

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

        OMX_MEMFREE_STRUCT(pComponentPrivate->pPortDef[MP3D_INPUT_PORT]);
        OMX_MEMFREE_STRUCT(pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]);
        OMX_MEMFREE_STRUCT(pComponentPrivate->mp3Params);
        OMX_MEMFREE_STRUCT (pComponentPrivate->pcmParams);
        OMX_MEMFREE_STRUCT(pComponentPrivate->pCompPort[MP3D_INPUT_PORT]->pPortFormat);
        OMX_MEMFREE_STRUCT (pComponentPrivate->pCompPort[MP3D_OUTPUT_PORT]->pPortFormat);
        OMX_MEMFREE_STRUCT (pComponentPrivate->pCompPort[MP3D_INPUT_PORT]);
        OMX_MEMFREE_STRUCT (pComponentPrivate->pCompPort[MP3D_OUTPUT_PORT]);
        OMX_MEMFREE_STRUCT (pComponentPrivate->sPortParam);
        OMX_MEMFREE_STRUCT (pComponentPrivate->pPriorityMgmt);
        OMX_MEMFREE_STRUCT(pComponentPrivate->pInputBufferList);
        OMX_MEMFREE_STRUCT(pComponentPrivate->pOutputBufferList);
    }

    pComponentPrivate->bPortDefsAllocated = 0;

#ifndef UNDER_CE
    OMX_PRDSP2(pComponentPrivate->dbg, "\n\n FreeCompResources: Destroying mutexes.\n\n");
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

static void signalAlloBufThresholdIfNecessary(
        MP3DEC_COMPONENT_PRIVATE *pComponentPrivate) {
#ifndef UNDER_CE
    pthread_mutex_lock(&pComponentPrivate->AlloBuf_mutex);
    if (pComponentPrivate->AlloBuf_waitingsignal) {
        pComponentPrivate->AlloBuf_waitingsignal = 0;
        pthread_cond_signal(&pComponentPrivate->AlloBuf_threshold);
    }
    pthread_mutex_unlock(&pComponentPrivate->AlloBuf_mutex);
#else
    if (pComponentPrivate->AlloBuf_waitingsignal) {
        // I am fairly sure this will suffer from similar issues without
        // proper mutex protection and a loop under WinCE...
        pComponentPrivate->AlloBuf_waitingsignal = 0;
        OMX_SignalEvent(&(pComponentPrivate->AlloBuf_event));
    }
#endif
}


/* ================================================================================= * */
/**
 * @fn MP3DEC_HandleCommand() function handles the command sent by the application.
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

OMX_U32 MP3DEC_HandleCommand (MP3DEC_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_U32 i,ret = 0;
    OMX_U16 arr[24];
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    char *pArgs = "damedesuStr";
    OMX_U32 pValues[4];
    OMX_U32 pValues1[4];
    OMX_COMPONENTTYPE *pHandle =(OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;
    OMX_COMMANDTYPE command;
    OMX_STATETYPE commandedState;
    OMX_U32 commandData;
    OMX_HANDLETYPE pLcmlHandle = pComponentPrivate->pLcmlHandle;

#ifdef RESOURCE_MANAGER_ENABLED
    OMX_ERRORTYPE rm_error = OMX_ErrorNone;
#endif
    
    OMX_PRINT1(pComponentPrivate->dbg, ":: >>> Entering HandleCommand Function\n");

    ret = read(pComponentPrivate->cmdPipe[0], &command, sizeof (command));
    if(ret == -1){
        MP3D_OMX_ERROR_EXIT(eError, 
                            OMX_ErrorHardware,
                            "Error while reading the command pipe");
        pComponentPrivate->cbInfo.EventHandler (pHandle, 
                                                pHandle->pApplicationPrivate,
                                                OMX_EventError, 
                                                eError,
                                                OMX_TI_ErrorSevere,
                                                NULL);
    }
    ret = read(pComponentPrivate->cmdDataPipe[0], &commandData, sizeof (commandData));
    if(ret == -1){
        MP3D_OMX_ERROR_EXIT(eError, OMX_ErrorHardware,
                            "Error while reading the commandData pipe");
        pComponentPrivate->cbInfo.EventHandler (pHandle, 
                                                pHandle->pApplicationPrivate,
                                                OMX_EventError, 
                                                eError,
                                                OMX_TI_ErrorSevere,
                                                NULL);
    }
    OMX_PRDSP2(pComponentPrivate->dbg, "---------------------------------------------\n");
    OMX_PRDSP2(pComponentPrivate->dbg, ":: command = %d\n",command);
    OMX_PRDSP2(pComponentPrivate->dbg, ":: commandData = %ld\n",commandData);
    OMX_PRDSP2(pComponentPrivate->dbg, "---------------------------------------------\n");
#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedCommand(pComponentPrivate->pPERFcomp,
                         command,
                         commandData,
                         PERF_ModuleLLMM);
#endif  
    if (command == OMX_CommandStateSet){
        commandedState = (OMX_STATETYPE)commandData;
        if (pComponentPrivate->curState == commandedState) {
            pComponentPrivate->cbInfo.EventHandler (pHandle, 
                                                    pHandle->pApplicationPrivate,
                                                    OMX_EventError, 
                                                    OMX_ErrorSameState,
                                                    OMX_TI_ErrorMinor,
                                                    NULL);

            OMX_ERROR4(pComponentPrivate->dbg, ":: Error: Same State Given by Application\n");
        } else {

            switch(commandedState) {

            case OMX_StateIdle:
                OMX_PRINT1(pComponentPrivate->dbg, ": HandleCommand: Cmd Idle \n");
                if (pComponentPrivate->curState == OMX_StateLoaded || pComponentPrivate->curState == OMX_StateWaitForResources) {
                    LCML_CALLBACKTYPE cb;
                    LCML_DSP *pLcmlDsp;
                    char *p = "damedesuStr";
#ifdef __PERF_INSTRUMENTATION__
                    PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryStart | PERF_BoundarySetup);
#endif
                    int inputPortFlag=0,outputPortFlag=0;

                    if (pComponentPrivate->dasfmode == 1) {
                        pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]->bEnabled= FALSE;
                        pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]->bPopulated= FALSE;
                        if(pComponentPrivate->streamID == 0) { 
                            OMX_ERROR4(pComponentPrivate->dbg, "**************************************\n"); 
                            OMX_ERROR4(pComponentPrivate->dbg, ":: Error = OMX_ErrorInsufficientResources\n"); 
                            OMX_ERROR4(pComponentPrivate->dbg, "**************************************\n"); 
                            pComponentPrivate->curState = OMX_StateInvalid; 
                            eError = OMX_ErrorInsufficientResources; 
                            pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                                   pHandle->pApplicationPrivate, 
                                                                   OMX_EventError, 
                                                                   eError,
                                                                   OMX_TI_ErrorMajor, 
                                                                   "AM: No Stream ID Available");                 
                            goto EXIT; 
                        } 
                    }


                    OMX_PRINT2(pComponentPrivate->dbg, "In while loop: IP : %p OP: %p\n",pComponentPrivate->pPortDef[MP3D_INPUT_PORT], 
                                  pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]);
                    OMX_PRCOMM2(pComponentPrivate->dbg, "pComponentPrivate->pPortDef[MP3D_INPUT_PORT]->bPopulated = %d\n",
                                  pComponentPrivate->pPortDef[MP3D_INPUT_PORT]->bPopulated);
                    OMX_PRCOMM2(pComponentPrivate->dbg, "pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]->bPopulated = %d\n",
                                  pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]->bPopulated);
                    OMX_PRCOMM2(pComponentPrivate->dbg, "pComponentPrivate->pPortDef[MP3D_INPUT_PORT]->bEnabled = %d\n",
                                  pComponentPrivate->pPortDef[MP3D_INPUT_PORT]->bEnabled);
                    OMX_PRCOMM2(pComponentPrivate->dbg, "pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]->bEnabled = %d\n",
                                  pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]->bEnabled);
                     

                    if (pComponentPrivate->pPortDef[MP3D_INPUT_PORT]->bPopulated && 
                        pComponentPrivate->pPortDef[MP3D_INPUT_PORT]->bEnabled)  {
                        inputPortFlag = 1;
                    }

                    if (!pComponentPrivate->pPortDef[MP3D_INPUT_PORT]->bPopulated && 
                        !pComponentPrivate->pPortDef[MP3D_INPUT_PORT]->bEnabled) {
                        inputPortFlag = 1;
                    }

                    if (pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]->bPopulated && 
                        pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]->bEnabled) {
                        outputPortFlag = 1;
                    }

                    if (!pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]->bPopulated && 
                        !pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]->bEnabled) {
                        outputPortFlag = 1;
                    }

                    if(!(inputPortFlag && outputPortFlag)) {
                        pComponentPrivate->InLoaded_readytoidle = 1;
#ifndef UNDER_CE        
                        pthread_mutex_lock(&pComponentPrivate->InLoaded_mutex); 
                        pthread_cond_wait(&pComponentPrivate->InLoaded_threshold, &pComponentPrivate->InLoaded_mutex);
                        pthread_mutex_unlock(&pComponentPrivate->InLoaded_mutex);
#else
                        OMX_WaitForEvent(&(pComponentPrivate->InLoaded_event));
#endif
                    }

                    pLcmlHandle = (OMX_HANDLETYPE) MP3DEC_GetLCMLHandle(pComponentPrivate);
                    if (pLcmlHandle == NULL) {
                        OMX_ERROR4(pComponentPrivate->dbg, ":: LCML Handle is NULL........exiting..\n");
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

                    pLcmlDsp = (((LCML_DSP_INTERFACE*)pLcmlHandle)->dspCodec);
                    eError = MP3DEC_Fill_LCMLInitParams(pHandle, pLcmlDsp,arr);
                    if(eError != OMX_ErrorNone) {
                        OMX_ERROR4(pComponentPrivate->dbg, ":: Error returned from Fill_LCMLInitParams()\n");
                        pComponentPrivate->curState = OMX_StateInvalid;
                        pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                               pHandle->pApplicationPrivate,
                                                               OMX_EventError, 
                                                               eError,
                                                               OMX_TI_ErrorSevere, 
                                                               NULL);
                        goto EXIT;
                    }
                    pComponentPrivate->pLcmlHandle = (LCML_DSP_INTERFACE *)pLcmlHandle;
                    cb.LCML_Callback = (void *) MP3DEC_LCML_Callback;

#ifndef UNDER_CE
                    eError = LCML_InitMMCodecEx(((LCML_DSP_INTERFACE *)pLcmlHandle)->pCodecinterfacehandle,
                                                p,&pLcmlHandle,(void *)p,&cb, (OMX_STRING)pComponentPrivate->sDeviceString);
                    if (eError != OMX_ErrorNone){
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
                    if (eError != OMX_ErrorNone){
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
                        (void *) MP3_ResourceManagerCallback;
                    if (pComponentPrivate->curState != OMX_StateWaitForResources){
                        rm_error = RMProxy_NewSendCommand(pHandle, 
                                                          RMProxy_RequestResource, 
                                                          OMX_MP3_Decoder_COMPONENT,
                                                          MP3_CPU,
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
                                                              OMX_MP3_Decoder_COMPONENT,
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
                                                          OMX_MP3_Decoder_COMPONENT,
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

                    OMX_PRDSP2(pComponentPrivate->dbg, ":: Control Came Here\n");
                    OMX_PRSTATE2(pComponentPrivate->dbg, "****************** Component State Set to Idle\n\n");
                    OMX_PRSTATE2(pComponentPrivate->dbg, "MP3DEC: State has been Set to Idle\n");
                } else if (pComponentPrivate->curState == OMX_StateExecuting){
#ifdef __PERF_INSTRUMENTATION__
                    PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryComplete | PERF_BoundarySteadyState);
#endif          
                    pComponentPrivate->bDspStoppedWhileExecuting = OMX_TRUE;                    
                    OMX_PRDSP2(pComponentPrivate->dbg, ": MP3DECUTILS::About to call LCML_ControlCodec STOP %d\n", __LINE__);
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
                        pComponentPrivate->cbInfo.EventHandler(pHandle, 
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
                } else if(pComponentPrivate->curState == OMX_StatePause) {
                    char *pArgs = "damedesuStr";
                    OMX_PRCOMM2(pComponentPrivate->dbg, ":: Comp: Stop Command Received\n");
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
                    OMX_PRDSP2(pComponentPrivate->dbg, ": MP3DECUTILS::About to call LCML_ControlCodec %d\n", __LINE__);
                    if (pComponentPrivate->codecStop_waitingsignal == 0){
                        pthread_mutex_lock(&pComponentPrivate->codecStop_mutex); 
                    } 
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               MMCodecControlStop,(void *)pArgs);
                    // lock mutex here, wait for stop ack. from lcml
                    if (pComponentPrivate->codecStop_waitingsignal == 0){
                        pthread_cond_wait(&pComponentPrivate->codecStop_threshold, &pComponentPrivate->codecStop_mutex);
                        pComponentPrivate->codecStop_waitingsignal = 0; // reset the wait condition for next time
                        pthread_mutex_unlock(&pComponentPrivate->codecStop_mutex);
                    }
                    if(eError != OMX_ErrorNone) {
                        OMX_ERROR4(pComponentPrivate->dbg, ": Error Occurred in Codec Stop..\n");
                        pComponentPrivate->curState = OMX_StateInvalid;
                        pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                               pHandle->pApplicationPrivate,
                                                               OMX_EventError, 
                                                               eError,
                                                               OMX_TI_ErrorSevere, 
                                                               NULL);
                        goto EXIT;
                    }
                    OMX_PRSTATE2(pComponentPrivate->dbg, "****************** Component State Set to Idle\n\n");
                    pComponentPrivate->curState = OMX_StateIdle;
#ifdef RESOURCE_MANAGER_ENABLED
                    rm_error = RMProxy_NewSendCommand(pHandle, 
                                                      RMProxy_StateSet, 
                                                      OMX_MP3_Decoder_COMPONENT, 
                                                      OMX_StateIdle, 
                                                      3456, 
                                                      NULL);
#endif                  
                    OMX_PRDSP2(pComponentPrivate->dbg, ":: The component is stopped\n");
                    pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventCmdComplete, 
                                                           OMX_CommandStateSet, 
                                                           pComponentPrivate->curState, 
                                                           NULL);

                } else {
                    OMX_ERROR2(pComponentPrivate->dbg, ": Comp: Sending ErrorNotification: Invalid State\n");
                    pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventError,
                                                           OMX_ErrorIncorrectStateTransition, 
                                                           OMX_TI_ErrorMinor,
                                                           "Invalid State Error");
                }
                break;

            case OMX_StateExecuting:
                OMX_PRDSP2(pComponentPrivate->dbg, ": HandleCommand: Cmd Executing \n");
                if (pComponentPrivate->curState == OMX_StateIdle) {
                    char *pArgs = "damedesuStr";
                    OMX_U32 pValues[4];
                    OMX_U32 pValues1[4];

                    if(!pComponentPrivate->SendAfterEOS){
                        if(pComponentPrivate->dasfmode == 1) {
                            pComponentPrivate->pParams->unAudioFormat = (unsigned short)pComponentPrivate->mp3Params->nChannels;
                            if (pComponentPrivate->pParams->unAudioFormat == MP3D_STEREO_STREAM) {
                                pComponentPrivate->pParams->unAudioFormat = MP3D_STEREO_NONINTERLEAVED_STREAM;
                            }

                            pComponentPrivate->pParams->ulSamplingFreq = pComponentPrivate->mp3Params->nSampleRate;
                            pComponentPrivate->pParams->unUUID = pComponentPrivate->streamID;

                            OMX_PRCOMM2(pComponentPrivate->dbg, "::pParams->unAudioFormat   = %ld\n",pComponentPrivate->mp3Params->nChannels);
                            OMX_PRCOMM2(pComponentPrivate->dbg, "::pParams->ulSamplingFreq  = %ld\n",pComponentPrivate->mp3Params->nSampleRate);

                            pValues[0] = USN_STRMCMD_SETCODECPARAMS;
                            pValues[1] = (OMX_U32)pComponentPrivate->pParams;
                            pValues[2] = sizeof(USN_AudioCodecParams);
                            OMX_PRDSP2(pComponentPrivate->dbg, ": MP3DECUTILS::About to call LCML_ControlCodec %d\n", __LINE__);
                            eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                         EMMCodecControlStrmCtrl,(void *)pValues);
                            if(eError != OMX_ErrorNone) {
                                OMX_ERROR4(pComponentPrivate->dbg, ": Error Occurred in Codec StreamControl..\n");
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
                        if(pComponentPrivate->dasfmode == 0 && 
                           pComponentPrivate->pcmParams->bInterleaved) {
                            pComponentPrivate->ptAlgDynParams->lOutputFormat  = IAUDIO_INTERLEAVED;
                        } else {
                            pComponentPrivate->ptAlgDynParams->lOutputFormat  = IAUDIO_BLOCK;
                        }
                    
                        pComponentPrivate->ptAlgDynParams->lMonoToStereoCopy = 0;
                        pComponentPrivate->ptAlgDynParams->lStereoToMonoCopy = 0;
                        pComponentPrivate->ptAlgDynParams->size = sizeof(MP3DEC_UALGParams);
                
                        pValues1[0] = IUALG_CMD_SETSTATUS;
                        pValues1[1] = (OMX_U32) pComponentPrivate->ptAlgDynParams;
                        pValues1[2] = sizeof(MP3DEC_UALGParams);
                        OMX_PRDSP2(pComponentPrivate->dbg, ": MP3DECUTILS::About to call LCML_ControlCodec %d\n", __LINE__);
                        eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                   EMMCodecControlAlgCtrl,(void *)pValues1);
                        if(eError != OMX_ErrorNone) {
                            OMX_ERROR4(pComponentPrivate->dbg, "Error Occurred in Codec Set Status DynParams..\n");
                            pComponentPrivate->curState = OMX_StateInvalid;
                            pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                                   pHandle->pApplicationPrivate,
                                                                   OMX_EventError, 
                                                                   eError,
                                                                   OMX_TI_ErrorSevere, 
                                                                   NULL);
                            goto EXIT;
                        }
                        pComponentPrivate->bDspStoppedWhileExecuting = OMX_FALSE;
                        OMX_PRDSP2(pComponentPrivate->dbg, ":: Algcontrol has been sent to DSP\n");
                        OMX_PRDSP2(pComponentPrivate->dbg, ": MP3DECUTILS::About to call LCML_ControlCodec START %d\n", __LINE__);
                        eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                   EMMCodecControlStart,(void *)pArgs);

                        if(eError != OMX_ErrorNone) {
                            OMX_ERROR4(pComponentPrivate->dbg, "%d: Error Occurred in Codec Start..\n", __LINE__);
                            goto EXIT;
                        }
                        OMX_PRDSP2(pComponentPrivate->dbg, ": Codec Has Been Started \n");
                    
                        pComponentPrivate->SendAfterEOS = 1;
                    } 
                } else if (pComponentPrivate->curState == OMX_StatePause) {
                    char *pArgs = "damedesuStr";
                    OMX_PRDSP2(pComponentPrivate->dbg, ": Comp: Resume Command Came from App\n");
                    OMX_PRDSP2(pComponentPrivate->dbg, ": MP3DECUTILS::About to call LCML_ControlCodec\n");
                    OMX_PRDSP2(pComponentPrivate->dbg, ": MP3DECUTILS::About to call LCML_ControlCodec  START %d\n", __LINE__);
                    pComponentPrivate->bDspStoppedWhileExecuting = OMX_FALSE;
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               EMMCodecControlStart,(void *)pArgs);

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
                        if (pComponentPrivate->pInputBufHdrPending[i] != NULL) {
                            MP3D_LCML_BUFHEADERTYPE *pLcmlHdr;
                            MP3DEC_GetCorresponding_LCMLHeader(pComponentPrivate, 
                                                               pComponentPrivate->pInputBufHdrPending[i]->pBuffer, OMX_DirInput, &pLcmlHdr);
                            MP3DEC_SetPending(pComponentPrivate,pComponentPrivate->pInputBufHdrPending[i],OMX_DirInput,__LINE__);
                            eError = LCML_QueueBuffer(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                      EMMCodecInputBuffer,
                                                      pComponentPrivate->pInputBufHdrPending[i]->pBuffer,
                                                      pComponentPrivate->pInputBufHdrPending[i]->nAllocLen,
                                                      pComponentPrivate->pInputBufHdrPending[i]->nFilledLen,
                                                      (OMX_U8 *) pLcmlHdr->pIpParam,
                                                      sizeof(MP3DEC_UAlgInBufParamStruct),
                                                      NULL);
                        }
                    }
                    pComponentPrivate->nNumInputBufPending = 0;

                    for (i=0; i < pComponentPrivate->nNumOutputBufPending; i++) {
                        if (pComponentPrivate->pOutputBufHdrPending[i]) {
                            MP3D_LCML_BUFHEADERTYPE *pLcmlHdr;
                            MP3DEC_GetCorresponding_LCMLHeader(pComponentPrivate, 
                                                               pComponentPrivate->pOutputBufHdrPending[i]->pBuffer, 
                                                               OMX_DirOutput, 
                                                               &pLcmlHdr);
                            MP3DEC_SetPending(pComponentPrivate,
                                              pComponentPrivate->pOutputBufHdrPending[i],
                                              OMX_DirOutput,
                                              __LINE__);
                            eError = LCML_QueueBuffer(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                      EMMCodecOuputBuffer,
                                                      pComponentPrivate->pOutputBufHdrPending[i]->pBuffer,
                                                      pComponentPrivate->pOutputBufHdrPending[i]->nAllocLen,
                                                      0,
                                                      (OMX_U8 *) pLcmlHdr->pOpParam,
                                                      sizeof(MP3DEC_UAlgOutBufParamStruct),
                                                      NULL);
                        }
                    }
                    pComponentPrivate->nNumOutputBufPending = 0;
                }else {
                    pComponentPrivate->cbInfo.EventHandler (pHandle, 
                                                            pHandle->pApplicationPrivate,
                                                            OMX_EventError, 
                                                            OMX_ErrorIncorrectStateTransition, 
                                                            OMX_TI_ErrorMinor,
                                                            "Invalid State");
                    OMX_ERROR4(pComponentPrivate->dbg, ":: Error: Invalid State Given by Application\n");
                    goto EXIT;
                }

                OMX_PRSTATE2(pComponentPrivate->dbg, "****************** Component State Set to Executing\n\n");
#ifdef RESOURCE_MANAGER_ENABLED         
                rm_error = RMProxy_NewSendCommand(pHandle, RMProxy_StateSet, OMX_MP3_Decoder_COMPONENT, OMX_StateExecuting, 3456, NULL);
#endif          
                pComponentPrivate->curState = OMX_StateExecuting;

#ifdef __PERF_INSTRUMENTATION__
                PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryStart | PERF_BoundarySteadyState);
#endif          
                pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandStateSet, 
                                                       pComponentPrivate->curState, 
                                                       NULL);

                break;

            case OMX_StateLoaded:
                OMX_PRDSP2(pComponentPrivate->dbg, ": HandleCommand: Cmd Loaded\n");

                if (pComponentPrivate->curState == OMX_StateWaitForResources ){
                    OMX_PRSTATE2(pComponentPrivate->dbg, "****************** Component State Set to Loaded\n\n");
#ifdef __PERF_INSTRUMENTATION__
                    PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryStart | PERF_BoundaryCleanup); 
#endif              
                    pComponentPrivate->curState = OMX_StateLoaded;
#ifdef __PERF_INSTRUMENTATION__
                    PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryComplete | PERF_BoundaryCleanup);
#endif              
                    pComponentPrivate->cbInfo.EventHandler (pHandle, 
                                                            pHandle->pApplicationPrivate,
                                                            OMX_EventCmdComplete, 
                                                            OMX_CommandStateSet,
                                                            pComponentPrivate->curState,
                                                            NULL);
                    OMX_PRCOMM2(pComponentPrivate->dbg, ":: Transitioning from WaitFor to Loaded\n");
                    break;
                }

                if (pComponentPrivate->curState != OMX_StateIdle) {
                    pComponentPrivate->cbInfo.EventHandler (pHandle, 
                                                            pHandle->pApplicationPrivate,
                                                            OMX_EventError, 
                                                            OMX_ErrorIncorrectStateTransition, 
                                                            OMX_TI_ErrorMinor,
                                                            "Invalid State");
                    OMX_ERROR4(pComponentPrivate->dbg, ":: Error: Invalid State Given by \
                       Application\n");
                    goto EXIT;
                }
#ifdef __PERF_INSTRUMENTATION__
                PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryStart | PERF_BoundaryCleanup);
#endif          

                OMX_PRSTATE2(pComponentPrivate->dbg, "Current State = %d\n",pComponentPrivate->curState);
                OMX_PRSTATE2(pComponentPrivate->dbg, "pComponentPrivate->pInputBufferList->numBuffers = %ld\n",pComponentPrivate->pInputBufferList->numBuffers);
                OMX_PRSTATE2(pComponentPrivate->dbg, "pComponentPrivate->pOutputBufferList->numBuffers = %ld\n",pComponentPrivate->pOutputBufferList->numBuffers);

                if (pComponentPrivate->pInputBufferList->numBuffers || pComponentPrivate->pOutputBufferList->numBuffers) {
                    pComponentPrivate->InIdle_goingtoloaded = 1;
#ifndef UNDER_CE
                    pthread_mutex_lock(&pComponentPrivate->InIdle_mutex); 
                    pthread_cond_wait(&pComponentPrivate->InIdle_threshold, &pComponentPrivate->InIdle_mutex);
                    pthread_mutex_unlock(&pComponentPrivate->InIdle_mutex);
#else
                    OMX_WaitForEvent(&(pComponentPrivate->InIdle_event));
#endif
                    /* Send StateChangeNotification to application */
                    pComponentPrivate->bLoadedCommandPending = OMX_FALSE;
                }
                OMX_PRDSP2(pComponentPrivate->dbg, ": MP3DECUTILS::About to call LCML_ControlCodec DESTROY %d\n", __LINE__);
                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                           EMMCodecControlDestroy,(void *)pArgs);
#ifdef UNDER_CE
                FreeLibrary(g_hLcmlDllHandle);
                g_hLcmlDllHandle = NULL;
#endif
#ifdef __PERF_INSTRUMENTATION__
                PERF_SendingCommand(pComponentPrivate->pPERF, -1, 0, PERF_ModuleComponent);
#endif        
                eError = EXIT_COMPONENT_THRD;
                pComponentPrivate->bInitParamsInitialized = 0;
                break;

            case OMX_StatePause:
                OMX_PRSTATE2(pComponentPrivate->dbg, ": HandleCommand: Cmd Pause: Cur State = %d\n",
                              pComponentPrivate->curState);

                if ((pComponentPrivate->curState != OMX_StateExecuting) &&
                    (pComponentPrivate->curState != OMX_StateIdle)) {
                    pComponentPrivate->cbInfo.EventHandler (pHandle, 
                                                            pHandle->pApplicationPrivate,
                                                            OMX_EventError, 
                                                            OMX_ErrorIncorrectStateTransition, 
                                                            OMX_TI_ErrorMinor,
                                                            "Invalid State");
                    OMX_ERROR4(pComponentPrivate->dbg, ":: Error: Invalid State Given by \
                       Application\n");
                    goto EXIT;
                }
#ifdef __PERF_INSTRUMENTATION__
                PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryComplete | PERF_BoundarySteadyState);
#endif
                OMX_PRDSP2(pComponentPrivate->dbg, ": MP3DECUTILS::About to call LCML_ControlCodec\n");
                OMX_PRDSP2(pComponentPrivate->dbg, ": MP3DECUTILS::About to call LCML_ControlCodec PAUSE %d\n", __LINE__);
                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                           EMMCodecControlPause,(void *)pArgs);
                if (eError != OMX_ErrorNone) {
                    OMX_ERROR4(pComponentPrivate->dbg, ": Error: in Pausing the codec\n");
                    pComponentPrivate->curState = OMX_StateInvalid;
                    pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventError, 
                                                           eError,
                                                           OMX_TI_ErrorSevere, 
                                                           NULL);
                    goto EXIT;
                }
                OMX_PRSTATE2(pComponentPrivate->dbg, "****************** Component State Set to Pause\n\n");
#ifdef RESOURCE_MANAGER_ENABLED         
                rm_error = RMProxy_NewSendCommand(pHandle,
                                                  RMProxy_StateSet,
                                                  OMX_MP3_Decoder_COMPONENT,
                                                  OMX_StatePause,
                                                  3456,
                                                  NULL);
#endif  

                break;

            case OMX_StateWaitForResources:
                OMX_PRDSP2(pComponentPrivate->dbg, ": HandleCommand: Cmd : OMX_StateWaitForResources\n");
                if (pComponentPrivate->curState == OMX_StateLoaded) {
#ifdef RESOURCE_MANAGER_ENABLED         
                    rm_error = RMProxy_NewSendCommand(pHandle, 
                                                      RMProxy_StateSet, 
                                                      OMX_MP3_Decoder_COMPONENT, 
                                                      OMX_StateWaitForResources, 
                                                      3456, 
                                                      NULL);
#endif  

                    pComponentPrivate->curState = OMX_StateWaitForResources;
                    OMX_PRDSP2(pComponentPrivate->dbg, ": Transitioning from Loaded to OMX_StateWaitForResources\n");
                    pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventCmdComplete,
                                                           OMX_CommandStateSet,
                                                           pComponentPrivate->curState, 
                                                           NULL);
                } else {
                    pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventError, 
                                                           OMX_ErrorIncorrectStateTransition,
                                                           OMX_TI_ErrorMinor, 
                                                           NULL);
                }
                break;

            case OMX_StateInvalid:
                OMX_PRDSP2(pComponentPrivate->dbg, ": HandleCommand: Cmd OMX_StateInvalid:\n");
                if (pComponentPrivate->curState != OMX_StateWaitForResources && 
                    pComponentPrivate->curState != OMX_StateLoaded &&
                    pComponentPrivate->curState != OMX_StateInvalid ) {

                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               EMMCodecControlDestroy, (void *)pArgs);
                }

                pComponentPrivate->curState = OMX_StateInvalid;
                pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventError, 
                                                       OMX_ErrorInvalidState,
                                                       OMX_TI_ErrorSevere,
                                                       NULL);
                MP3DEC_CleanupInitParams(pHandle);

                break;

            case OMX_StateMax:
                OMX_PRSTATE2(pComponentPrivate->dbg, ": HandleCommand: Cmd OMX_StateMax::\n");
                break;
            } /* End of Switch */
        }
    }
    else if (command == OMX_CommandMarkBuffer) {
        OMX_PRDSP2(pComponentPrivate->dbg, "command OMX_CommandMarkBuffer received\n");
        if(!pComponentPrivate->pMarkBuf){
            OMX_PRBUFFER2(pComponentPrivate->dbg, "command OMX_CommandMarkBuffer received \n");
            pComponentPrivate->pMarkBuf = (OMX_MARKTYPE *)(commandData);
        }
    } else if (command == OMX_CommandPortDisable) {
        if (!pComponentPrivate->bDisableCommandPending) {
            if(commandData == 0x0){
                /* disable port */
                for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
                    OMX_PRBUFFER2(pComponentPrivate->dbg, "pComponentPrivate->pInputBufferList->bBufferPending[%ld] = %ld\n",i,
                                  pComponentPrivate->pInputBufferList->bBufferPending[i]);
                    if (MP3DEC_IsPending(pComponentPrivate,pComponentPrivate->pInputBufferList->pBufHdr[i],OMX_DirInput)) {
                        /* Real solution is flush buffers from DSP.  Until we have the ability to do that 
                           we just call EmptyBufferDone() on any pending buffers */
                        OMX_PRBUFFER2(pComponentPrivate->dbg, "Forcing EmptyBufferDone\n");
#ifdef __PERF_INSTRUMENTATION__
                        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                          PREF(pComponentPrivate->pInputBufferList->pBufHdr[i], pBuffer),
                                          0,
                                          PERF_ModuleHLMM);
#endif                  
                        pComponentPrivate->cbInfo.EmptyBufferDone (pComponentPrivate->pHandle,
                                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                                   pComponentPrivate->pInputBufferList->pBufHdr[i]);
                        pComponentPrivate->nEmptyBufferDoneCount++;
                        SignalIfAllBuffersAreReturned(pComponentPrivate);
                    }
                }

                pComponentPrivate->pPortDef[MP3D_INPUT_PORT]->bEnabled = OMX_FALSE;
            }
            if(commandData == -1){
                /* disable port */
                pComponentPrivate->pPortDef[MP3D_INPUT_PORT]->bEnabled = OMX_FALSE;
            }
            if(commandData == 0x1 || commandData == -1){
                char *pArgs = "damedesuStr";
                pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]->bEnabled = OMX_FALSE;
            }
        }
        OMX_PRCOMM2(pComponentPrivate->dbg, "commandData = %ld\n",commandData);
        OMX_PRCOMM2(pComponentPrivate->dbg, "pComponentPrivate->pPortDef[MP3D_INPUT_PORT]->bPopulated = %d\n",
                      pComponentPrivate->pPortDef[MP3D_INPUT_PORT]->bPopulated);
        OMX_PRCOMM2(pComponentPrivate->dbg, "pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]->bPopulated = %d\n",
                      pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]->bPopulated);
        if(commandData == 0x0) {
            if(!pComponentPrivate->pPortDef[MP3D_INPUT_PORT]->bPopulated){
                /* return cmdcomplete event if input unpopulated */ 
                pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete, 
                                                       OMX_CommandPortDisable,
                                                       MP3D_INPUT_PORT, 
                                                       NULL);
                pComponentPrivate->bDisableCommandPending = 0;
            }
            else{
                pComponentPrivate->bDisableCommandPending = 1;
                pComponentPrivate->bDisableCommandParam = commandData;
            }
        }

        if(commandData == 0x1) {
            if (!pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]->bPopulated){
                /* return cmdcomplete event if output unpopulated */ 
                OMX_PRCOMM2(pComponentPrivate->dbg, "CMD COMPLETE PORT DISABLE OUTPUT\n");
                pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete, 
                                                       OMX_CommandPortDisable,
                                                       MP3D_OUTPUT_PORT, 
                                                       NULL);
                pComponentPrivate->bDisableCommandPending = 0;
            }
            else {
                pComponentPrivate->bDisableCommandPending = 1;
                pComponentPrivate->bDisableCommandParam = commandData;
            }
        }

        if(commandData == -1) {
            if (!pComponentPrivate->pPortDef[MP3D_INPUT_PORT]->bPopulated && 
                !pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]->bPopulated){

                /* return cmdcomplete event if inout & output unpopulated */ 
                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete, 
                                                       OMX_CommandPortDisable,
                                                       MP3D_INPUT_PORT, 
                                                       NULL);

                pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete, 
                                                       OMX_CommandPortDisable,
                                                       MP3D_OUTPUT_PORT, 
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
        if (pComponentPrivate->bDspStoppedWhileExecuting){
            // set up the codec with corrected settings
            // it was not done in idle->executing transition
            if(pComponentPrivate->dasfmode == 0 && 
                pComponentPrivate->pcmParams->bInterleaved) {
                pComponentPrivate->ptAlgDynParams->lOutputFormat  = IAUDIO_INTERLEAVED;
            } else {
                pComponentPrivate->ptAlgDynParams->lOutputFormat  = IAUDIO_BLOCK;
            }
            pComponentPrivate->ptAlgDynParams->lMonoToStereoCopy = 0;
            pComponentPrivate->ptAlgDynParams->lStereoToMonoCopy = 0;
            pComponentPrivate->ptAlgDynParams->size = sizeof(MP3DEC_UALGParams);
                
            pValues1[0] = IUALG_CMD_SETSTATUS;
            pValues1[1] = (OMX_U32) pComponentPrivate->ptAlgDynParams;
            pValues1[2] = sizeof(MP3DEC_UALGParams);
            OMX_PRDSP2(pComponentPrivate->dbg, ": MP3DECUTILS::About to call LCML_ControlCodec ALG %d\n", __LINE__);
            eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                       EMMCodecControlAlgCtrl,(void *)pValues1);
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

            pComponentPrivate->bDspStoppedWhileExecuting = OMX_FALSE;
            OMX_PRDSP2(pComponentPrivate->dbg, ": MP3DECUTILS::About to call LCML_ControlCodec start %d\n", __LINE__);
            eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                         EMMCodecControlStart,(void *)pArgs);
            
            if(eError != OMX_ErrorNone) {
                OMX_ERROR4(pComponentPrivate->dbg, "%d: Error Occurred in Codec Start..\n", __LINE__);
                goto EXIT;
            }
            OMX_PRDSP1(pComponentPrivate->dbg, ": Codec Has Been Started \n");
        }
        else{
            OMX_PRDSP1(pComponentPrivate->dbg, "codec was previusly started! %d\n", __LINE__);
        }
        if (!pComponentPrivate->bEnableCommandPending) {
            if(commandData == 0x0 || commandData == -1){
                /* enable in port */
                OMX_PRCOMM2(pComponentPrivate->dbg, "setting input port to enabled\n");
                pComponentPrivate->pPortDef[MP3D_INPUT_PORT]->bEnabled = OMX_TRUE;
                OMX_PRSTATE2(pComponentPrivate->dbg, "WAKE UP!! HandleCommand: En utils setting output port to enabled. \n");
                if(pComponentPrivate->AlloBuf_waitingsignal){
                    pComponentPrivate->AlloBuf_waitingsignal = 0;
                }
                OMX_PRCOMM2(pComponentPrivate->dbg, "pComponentPrivate->pPortDef[MP3D_INPUT_PORT]->bEnabled = %d\n",
                              pComponentPrivate->pPortDef[MP3D_INPUT_PORT]->bEnabled);
            }
            if(commandData == 0x1 || commandData == -1){
                
                /* enable out port */
                pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]->bEnabled = OMX_TRUE;
                OMX_PRCOMM2(pComponentPrivate->dbg, "pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]->bEnabled = %d\n",
                              pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]->bEnabled);

                signalAlloBufThresholdIfNecessary(pComponentPrivate);
                OMX_PRCOMM2(pComponentPrivate->dbg, "setting output port to enabled\n");
            }
        }
        if(commandData == 0x0){
            if (pComponentPrivate->curState == OMX_StateLoaded || 
                pComponentPrivate->pPortDef[MP3D_INPUT_PORT]->bPopulated) {
                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandPortEnable,
                                                       MP3D_INPUT_PORT,
                                                       NULL);
                
                signalAlloBufThresholdIfNecessary(pComponentPrivate);

                //MP3DECFill_LCMLInitParamsEx(pHandle, 0);
                // queue the pending buffers received while doing the config
                for (i=0; i < pComponentPrivate->nNumInputBufPending; i++) {
                    OMX_PRCOMM2(pComponentPrivate->dbg, "in queuePending loop INPUT %d\n", __LINE__);
                    if (pComponentPrivate->pInputBufHdrPending[i] != NULL) {
                        MP3D_LCML_BUFHEADERTYPE *pLcmlHdr;
                        MP3DEC_GetCorresponding_LCMLHeader(pComponentPrivate, 
                                                           pComponentPrivate->pInputBufHdrPending[i]->pBuffer, OMX_DirInput, &pLcmlHdr);
                        MP3DEC_SetPending(pComponentPrivate,pComponentPrivate->pInputBufHdrPending[i],OMX_DirInput,__LINE__);
                        OMX_PRBUFFER2(pComponentPrivate->dbg, "QueueBuffer pending port config line %d\n", __LINE__);
                        eError = LCML_QueueBuffer(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                  EMMCodecInputBuffer,
                                                  pComponentPrivate->pInputBufHdrPending[i]->pBuffer,
                                                  pComponentPrivate->pInputBufHdrPending[i]->nAllocLen,
                                                  pComponentPrivate->pInputBufHdrPending[i]->nFilledLen,
                                                  (OMX_U8 *) pLcmlHdr->pIpParam,
                                                  sizeof(MP3DEC_UAlgInBufParamStruct),
                                                  NULL);
                        OMX_PRBUFFER2(pComponentPrivate->dbg, "QueueBuffer pending port config line %d, error = %d\n", __LINE__, eError);
                    }
                }
                pComponentPrivate->nNumInputBufPending = 0;
                pComponentPrivate->reconfigInputPort = 0;
                pComponentPrivate->bEnableCommandPending = 0;
            }
            else {
                pComponentPrivate->bEnableCommandPending = 1;
                pComponentPrivate->bEnableCommandParam = commandData;
            }
        }
        else if(commandData == 0x1) {
            if (pComponentPrivate->curState == OMX_StateLoaded || 
                pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]->bPopulated){
                pComponentPrivate->cbInfo.EventHandler( pHandle, 
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,
                                                        OMX_CommandPortEnable,
                                                        MP3D_OUTPUT_PORT, 
                                                        NULL);

                signalAlloBufThresholdIfNecessary(pComponentPrivate);
                OMX_PRINT2(pComponentPrivate->dbg, "reconfigOut = %d!, but should be true!\n",pComponentPrivate->reconfigOutputPort);
                if(pComponentPrivate->reconfigOutputPort){
                    //make sure new VA's are used
                    MP3DEC_CleanupInitParamsEx(pHandle,commandData);
                    MP3DECFill_LCMLInitParamsEx(pHandle, 1);
                    OMX_PRDSP2(pComponentPrivate->dbg, "completed MP3DEC_MapLCMLParamsEx! %d\n", __LINE__);
                    
                }

                for (i=0; i < pComponentPrivate->nNumOutputBufPending; i++) {
                    if (pComponentPrivate->pOutputBufHdrPending[i]) {
                        MP3D_LCML_BUFHEADERTYPE *pLcmlHdr;
                        MP3DEC_GetCorresponding_LCMLHeader(pComponentPrivate, 
                                                           pComponentPrivate->pOutputBufHdrPending[i]->pBuffer, 
                                                           OMX_DirOutput, 
                                                           &pLcmlHdr);
                        MP3DEC_SetPending(pComponentPrivate,
                                          pComponentPrivate->pOutputBufHdrPending[i],
                                          OMX_DirOutput,
                                          __LINE__);
                        OMX_PRCOMM2(pComponentPrivate->dbg, "QueueBuffer pending port config line %d\n", __LINE__);
                        eError = LCML_QueueBuffer(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                  EMMCodecOuputBuffer,
                                                  pComponentPrivate->pOutputBufHdrPending[i]->pBuffer,
                                                  pComponentPrivate->pOutputBufHdrPending[i]->nAllocLen,
                                                  0,
                                                  (OMX_U8 *) pLcmlHdr->pOpParam,
                                                  sizeof(MP3DEC_UAlgOutBufParamStruct),
                                                  NULL);
                        OMX_PRCOMM2(pComponentPrivate->dbg, "QueueBuffer pending port config line %d, error = %d\n", __LINE__, eError);
                    }
                }
                pComponentPrivate->nNumOutputBufPending = 0;
                // queue the pending buffers received while doing the config
                for (i=0; i < pComponentPrivate->nNumInputBufPending; i++) {
                    OMX_PRCOMM2(pComponentPrivate->dbg, "in queuePending loop INPUT %d\n", __LINE__);
                    if (pComponentPrivate->pInputBufHdrPending[i] != NULL) {
                        MP3D_LCML_BUFHEADERTYPE *pLcmlHdr;
                        MP3DEC_GetCorresponding_LCMLHeader(pComponentPrivate, 
                                                           pComponentPrivate->pInputBufHdrPending[i]->pBuffer, OMX_DirInput, &pLcmlHdr);
                        MP3DEC_SetPending(pComponentPrivate,pComponentPrivate->pInputBufHdrPending[i],OMX_DirInput,__LINE__);
                        OMX_PRCOMM2(pComponentPrivate->dbg, "QueueBuffer pending port config line %d\n", __LINE__);
                        eError = LCML_QueueBuffer(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                  EMMCodecInputBuffer,
                                                  pComponentPrivate->pInputBufHdrPending[i]->pBuffer,
                                                  pComponentPrivate->pInputBufHdrPending[i]->nAllocLen,
                                                  pComponentPrivate->pInputBufHdrPending[i]->nFilledLen,
                                                  (OMX_U8 *) pLcmlHdr->pIpParam,
                                                  sizeof(MP3DEC_UAlgInBufParamStruct),
                                                  NULL);
                        OMX_PRCOMM2(pComponentPrivate->dbg, "QueueBuffer pending port config line %d, error = %d\n", __LINE__, eError);
                    }
                }
                pComponentPrivate->nNumInputBufPending = 0;
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
                (pComponentPrivate->pPortDef[MP3D_INPUT_PORT]->bPopulated
                 && pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]->bPopulated)){
                pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete, 
                                                       OMX_CommandPortEnable,
                                                       MP3D_INPUT_PORT, 
                                                       NULL);
                pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete, 
                                                       OMX_CommandPortEnable,
                                                       MP3D_OUTPUT_PORT, 
                                                       NULL);

                signalAlloBufThresholdIfNecessary(pComponentPrivate);
                MP3DEC_CleanupInitParamsEx(pHandle,commandData);
                MP3DECFill_LCMLInitParamsEx(pHandle, -1);

                // queue the pending buffers received while doing the config, output then input
                for (i=0; i < pComponentPrivate->nNumOutputBufPending; i++) {
                    if (pComponentPrivate->pOutputBufHdrPending[i]) {
                        MP3D_LCML_BUFHEADERTYPE *pLcmlHdr;
                        MP3DEC_GetCorresponding_LCMLHeader(pComponentPrivate, 
                                                           pComponentPrivate->pOutputBufHdrPending[i]->pBuffer, 
                                                           OMX_DirOutput, 
                                                           &pLcmlHdr);
                        MP3DEC_SetPending(pComponentPrivate,
                                          pComponentPrivate->pOutputBufHdrPending[i],
                                          OMX_DirOutput,
                                          __LINE__);
                        eError = LCML_QueueBuffer(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                  EMMCodecOuputBuffer,
                                                  pComponentPrivate->pOutputBufHdrPending[i]->pBuffer,
                                                  pComponentPrivate->pOutputBufHdrPending[i]->nAllocLen,
                                                  0,
                                                  (OMX_U8 *) pLcmlHdr->pOpParam,
                                                  sizeof(MP3DEC_UAlgOutBufParamStruct),
                                                  NULL);
                    }
                }
                pComponentPrivate->nNumOutputBufPending = 0;
                for (i=0; i < pComponentPrivate->nNumInputBufPending; i++) {
                    OMX_PRCOMM2(pComponentPrivate->dbg, "in queuePending loop INPUT %d\n", __LINE__);
                    if (pComponentPrivate->pInputBufHdrPending[i] != NULL) {
                        MP3D_LCML_BUFHEADERTYPE *pLcmlHdr;
                        MP3DEC_GetCorresponding_LCMLHeader(pComponentPrivate, 
                                                           pComponentPrivate->pInputBufHdrPending[i]->pBuffer, OMX_DirInput, &pLcmlHdr);
                        MP3DEC_SetPending(pComponentPrivate,pComponentPrivate->pInputBufHdrPending[i],OMX_DirInput,__LINE__);
                        OMX_PRCOMM2(pComponentPrivate->dbg, "QueueBuffer pending port config line %d\n", __LINE__);
                        eError = LCML_QueueBuffer(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                                  EMMCodecInputBuffer,
                                                  pComponentPrivate->pInputBufHdrPending[i]->pBuffer,
                                                  pComponentPrivate->pInputBufHdrPending[i]->nAllocLen,
                                                  pComponentPrivate->pInputBufHdrPending[i]->nFilledLen,
                                                  (OMX_U8 *) pLcmlHdr->pIpParam,
                                                  sizeof(MP3DEC_UAlgInBufParamStruct),
                                                  NULL);
                        OMX_PRCOMM2(pComponentPrivate->dbg, "QueueBuffer pending port config line %d, error = %d\n", __LINE__, eError);
                    }
                }
                pComponentPrivate->nNumInputBufPending = 0;
                pComponentPrivate->bEnableCommandPending = 0;
            }
            else {
                pComponentPrivate->bEnableCommandPending = 1;
                pComponentPrivate->bEnableCommandParam = commandData;
            }
        }
    }
    else if (command == OMX_CommandFlush) {
        OMX_U32 aParam[3] = {0};
        if(commandData == 0x0 || commandData == -1) {
            OMX_ERROR2(pComponentPrivate->dbg, "Flushing input port:: unhandled ETB's = %ld, handled ETB's = %ld\n",
                       pComponentPrivate->nEmptyThisBufferCount, pComponentPrivate->nHandledEmptyThisBuffers);
            if (pComponentPrivate->nEmptyThisBufferCount == pComponentPrivate->nHandledEmptyThisBuffers)  {
                pComponentPrivate->bFlushInputPortCommandPending = OMX_FALSE;
                pComponentPrivate->first_buff = 0;
                OMX_ERROR2(pComponentPrivate->dbg, "in flush IN:lcml_nCntApp && app_nBuf = %ld && %ld\n", pComponentPrivate->lcml_nCntApp, pComponentPrivate->app_nBuf);
                if (pComponentPrivate->num_Sent_Ip_Buff){ //no buffers have been sent yet, no need to flush SN
                    aParam[0] = USN_STRMCMD_FLUSH;        
                    aParam[1] = 0x0; 
                    aParam[2] = 0x0; 

                    OMX_ERROR4(pComponentPrivate->dbg, "Flushing input port\n");
                    OMX_ERROR2(pComponentPrivate->dbg, ": MP3DECUTILS::About to call LCML_ControlCodec FLUSH in %d\n", __LINE__);
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
                    OMX_PRCOMM2(pComponentPrivate->dbg, "Flushing input port %d\n",__LINE__);
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
                                                           MP3D_INPUT_PORT, 
                                                           NULL);
                }
            }else {
                pComponentPrivate->bFlushInputPortCommandPending = OMX_TRUE;
            }
        }
        if(commandData == 0x1 || commandData == -1){
            OMX_ERROR2(pComponentPrivate->dbg, "Flushing output port:: unhandled FTB's = %ld handled FTB's = %ld\n",
                       pComponentPrivate->nFillThisBufferCount, pComponentPrivate->nHandledFillThisBuffers);
            if (pComponentPrivate->nFillThisBufferCount == pComponentPrivate->nHandledFillThisBuffers)  {
                pComponentPrivate->bFlushOutputPortCommandPending = OMX_FALSE;
                /*pComponentPrivate->first_buff = 0;*/
                OMX_PRBUFFER2(pComponentPrivate->dbg, "in flush OUT:lcml_nCntApp && app_nBuf = %ld && %ld\n", pComponentPrivate->lcml_nCntApp, pComponentPrivate->app_nBuf);
                OMX_PRBUFFER2(pComponentPrivate->dbg, "in flush OUT:lcml_nOpBuf = %ld \n", pComponentPrivate->lcml_nOpBuf);
                if (pComponentPrivate->num_Op_Issued && !pComponentPrivate->reconfigOutputPort){ //if no buffers have been sent yet, no need to flush SN
                    aParam[0] = USN_STRMCMD_FLUSH; 
                    aParam[1] = 0x1; 
                    aParam[2] = 0x0; 

                    OMX_ERROR4(pComponentPrivate->dbg, "Flushing output port\n");
                    OMX_PRDSP2(pComponentPrivate->dbg, ": MP3DECUTILS::About to call LCML_ControlCodec FLUSH out %d\n", __LINE__);
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
                        pComponentPrivate->nFillBufferDoneCount++;
                        pComponentPrivate->pOutputBufHdrPending[i] = NULL;
                        SignalIfAllBuffersAreReturned(pComponentPrivate);
                    }
                    pComponentPrivate->nNumOutputBufPending=0;

                    pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle, 
                                                           pComponentPrivate->pHandle->pApplicationPrivate,
                                                           OMX_EventCmdComplete, 
                                                           OMX_CommandFlush,
                                                           MP3D_OUTPUT_PORT,
                                                           NULL);
                }
            } else {
                pComponentPrivate->bFlushOutputPortCommandPending = OMX_TRUE; 
            }
        }
    }
 EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, ":: Exiting HandleCommand Function, error = %d\n", eError);
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


/* ================================================================================= * */
/**
 * @fn MP3DEC_HandleDataBuf_FromApp() function handles the input and output buffers
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

OMX_ERRORTYPE MP3DEC_HandleDataBuf_FromApp(OMX_BUFFERHEADERTYPE* pBufHeader,
                                           MP3DEC_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_DIRTYPE eDir;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefIn = NULL;
    char *pArgs = "damedesuStr";
    OMX_U32 pValues[4];
    OMX_U32 pValues1[4];
    OMX_U32 nBitPosition = 0;
    OMX_U8* pHeaderStream = (OMX_U8*)pBufHeader->pBuffer;
    OMX_U32 temp = -1;
    OMX_U32 temp2 = -1;

    OMX_PRINT1(pComponentPrivate->dbg, ":: Entering HandleDataBuf_FromApp Function\n");
    OMX_PRBUFFER2(pComponentPrivate->dbg, ":: pBufHeader->pMarkData = %p\n",pBufHeader->pMarkData);

    pBufHeader->pPlatformPrivate  = pComponentPrivate;
    eError = MP3DEC_GetBufferDirection(pBufHeader, &eDir);
    OMX_PRBUFFER2(pComponentPrivate->dbg, ":: HandleDataBuf_FromApp Function\n");
    if (eError != OMX_ErrorNone) {
        OMX_ERROR4(pComponentPrivate->dbg, ":: The pBufHeader is not found in the list\n");
        goto EXIT;
    }

    if (eDir == OMX_DirInput) {
        pComponentPrivate->nHandledEmptyThisBuffers++;
        if (pComponentPrivate->curState == OMX_StateIdle){
            pComponentPrivate->cbInfo.EmptyBufferDone (pComponentPrivate->pHandle,
                                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                                       pBufHeader);
            OMX_PRBUFFER2(pComponentPrivate->dbg, ":: %d %s In idle state return input buffers\n", __LINE__, __FUNCTION__);
            pComponentPrivate->nEmptyBufferDoneCount++;
            SignalIfAllBuffersAreReturned(pComponentPrivate);
            goto EXIT;
        }
        LCML_DSP_INTERFACE *pLcmlHandle = (LCML_DSP_INTERFACE *)pComponentPrivate->pLcmlHandle;
        MP3D_LCML_BUFHEADERTYPE *pLcmlHdr;
        pPortDefIn = pComponentPrivate->pPortDef[OMX_DirInput];
        
        eError = MP3DEC_GetCorresponding_LCMLHeader(pComponentPrivate, pBufHeader->pBuffer, OMX_DirInput, &pLcmlHdr);
        if (eError != OMX_ErrorNone) {
            OMX_ERROR4(pComponentPrivate->dbg, ":: Error: Invalid Buffer Came ...\n");
            goto EXIT;
        }
 
        if ((pBufHeader->nFilledLen > 0) || (pBufHeader->nFlags & OMX_BUFFERFLAG_EOS)) {
            pComponentPrivate->bBypassDSP = 0;
            OMX_PRBUFFER2(pComponentPrivate->dbg, ":: HandleDataBuf_FromApp Function\n");
            OMX_PRBUFFER2(pComponentPrivate->dbg, ":::Calling LCML_QueueBuffer\n");

#ifdef __PERF_INSTRUMENTATION__
            /*For Steady State Instumentation*/
            PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                              PREF(pBufHeader,pBuffer),
                              pPortDefIn->nBufferSize, 
                              PERF_ModuleCommonLayer);
#endif
            if(pComponentPrivate->SendAfterEOS){
                if(pComponentPrivate->dasfmode == 1) {
                    pComponentPrivate->pParams->unAudioFormat = 
                        (unsigned short)pComponentPrivate->mp3Params->nChannels;
                    if (pComponentPrivate->pParams->unAudioFormat == MP3D_STEREO_STREAM) {
                        pComponentPrivate->pParams->unAudioFormat = MP3D_STEREO_NONINTERLEAVED_STREAM;
                    }

                    pComponentPrivate->pParams->ulSamplingFreq = 
                        pComponentPrivate->mp3Params->nSampleRate;
                    pComponentPrivate->pParams->unUUID = pComponentPrivate->streamID;

                    pValues[0] = USN_STRMCMD_SETCODECPARAMS;
                    pValues[1] = (OMX_U32)pComponentPrivate->pParams;
                    pValues[2] = sizeof(USN_AudioCodecParams);
                    OMX_PRDSP2(pComponentPrivate->dbg, ": MP3DECUTILS::About to call LCML_ControlCodec stream control %d\n", __LINE__);
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               EMMCodecControlStrmCtrl,(void *)pValues);
                    if(eError != OMX_ErrorNone) {
                        OMX_ERROR4(pComponentPrivate->dbg, ": Error Occurred in Codec StreamControl..\n");
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

                if(pBufHeader->nFlags & OMX_BUFFERFLAG_CODECCONFIG || pComponentPrivate->bConfigData){
		    OMX_PRBUFFER2(pComponentPrivate->dbg, "Detected OMX_BUFFERFLAG_CODECCONFIG\n, \tproceed with parsing config data\n"); 
                    // parse the frame header
                    pComponentPrivate->pStreamData.nSyncWord = MP3DEC_GetBits(&nBitPosition, 11, pHeaderStream, OMX_TRUE);
                    pComponentPrivate->pStreamData.nMpegVersion = MP3DEC_GetBits(&nBitPosition, 2, pHeaderStream, OMX_TRUE);
                    pComponentPrivate->pStreamData.nLayer = MP3DEC_GetBits(&nBitPosition, 2, pHeaderStream, OMX_TRUE);
                    temp = MP3DEC_GetBits(&nBitPosition, 1, pHeaderStream, OMX_TRUE); // prot. bit
                    pComponentPrivate->pStreamData.nBitRate = MP3DEC_GetBits(&nBitPosition, 4, pHeaderStream, OMX_TRUE);
                    pComponentPrivate->pStreamData.nFrequency = MP3DEC_GetBits(&nBitPosition, 2, pHeaderStream, OMX_TRUE);
                    temp = MP3DEC_GetBits(&nBitPosition, 2, pHeaderStream, OMX_TRUE);  // pad bit, prov. bit
                    pComponentPrivate->pStreamData.nChannelMode = MP3DEC_GetBits(&nBitPosition, 2, pHeaderStream, OMX_TRUE);

                    //save the current value of pcmParams->nSamplingRate
                    temp =  pComponentPrivate->pcmParams->nSamplingRate;

                    // parsing completed, now compare to existing values and set port params as needed
                    switch(pComponentPrivate->pStreamData.nFrequency){
                        // these frequency values are based on mpeg1 supported freqs
                        // if the stream is actually mpeg2 or mpeg2.5, we will divide appropriately below.
                        case 0:
                            pComponentPrivate->pcmParams->nSamplingRate = 44100;
                            break;
                        case 1:
                            pComponentPrivate->pcmParams->nSamplingRate = 48000;
                            break;
                        case 2:
                            pComponentPrivate->pcmParams->nSamplingRate = 32000;
                            break;
                        default:
                            OMX_ERROR4(pComponentPrivate->dbg, "Unsupported Frequency\n");
                            break;
                    }
                    if (pComponentPrivate->pStreamData.nMpegVersion == 2){
                        // the actual sampling frequency is dependant upon the mpeg version
                        // if Mpeg2 is used, divide the sampling rate from above by 2
                        pComponentPrivate->pcmParams->nSamplingRate /= 2;
                    }
                    else if (pComponentPrivate->pStreamData.nMpegVersion == 0){
                        // the actual sampling frequency is dependant upon the mpeg version
                        // divide by 4 for mpeg 2.5
                        pComponentPrivate->pcmParams->nSamplingRate /= 4;
                    }
                    // save the current value of nChannels
                    temp2 = pComponentPrivate->pcmParams->nChannels;
                    // value of 3 = mono
                    if (pComponentPrivate->pStreamData.nChannelMode == 3){
                        pComponentPrivate->pcmParams->nChannels = 1;
                    }
                    else{ // if stereo,joint stereo or dual channel, then pcm channels is stereo. Otherwise pcm output will be mono
                         pComponentPrivate->pcmParams->nChannels = MP3D_STEREO_STREAM;
                    }
                    // decide if dynamic reconfig is needed
                    OMX_PRCOMM2(pComponentPrivate->dbg, ": decide on reconfig ports...\n");
                    if (temp !=  pComponentPrivate->pcmParams->nSamplingRate || 
                        temp2 != pComponentPrivate->pcmParams->nChannels){
                        pComponentPrivate->reconfigOutputPort = OMX_TRUE;
                        OMX_PRCOMM2(pComponentPrivate->dbg, ": reconfif output port set to true...\n");
                    }
                    else{
                        OMX_PRCOMM2(pComponentPrivate->dbg, ": no port config needed, skip\n");
                    }
                    
                    OMX_PRINT2(pComponentPrivate->dbg, "CODEC CONFIG: \n\tsample rate = %ld\n\tchannels = %ld\n\t channel mode = %ld (0/1 = stereo, 2/3 = mono)\n",
                          pComponentPrivate->pcmParams->nSamplingRate, 
                          pComponentPrivate->pcmParams->nChannels,
                          pComponentPrivate->pStreamData.nChannelMode);
                    // set up the codec with corrected settings
                    // it was not done in idle->executing transition
                    if(pComponentPrivate->dasfmode == 0 && 
                       pComponentPrivate->pcmParams->bInterleaved) {
                        pComponentPrivate->ptAlgDynParams->lOutputFormat  = IAUDIO_INTERLEAVED;
                    } else {
                        pComponentPrivate->ptAlgDynParams->lOutputFormat  = IAUDIO_BLOCK;
                    }

                    pComponentPrivate->ptAlgDynParams->lMonoToStereoCopy = 0;
                    pComponentPrivate->ptAlgDynParams->lStereoToMonoCopy = 0;
                    pComponentPrivate->ptAlgDynParams->size = sizeof(MP3DEC_UALGParams);
                
                    pValues1[0] = IUALG_CMD_SETSTATUS;
                    pValues1[1] = (OMX_U32) pComponentPrivate->ptAlgDynParams;
                    pValues1[2] = sizeof(MP3DEC_UALGParams);
                    OMX_PRDSP2(pComponentPrivate->dbg, ": MP3DECUTILS::About to call LCML_ControlCodec ALG %d\n", __LINE__);
                    eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                               EMMCodecControlAlgCtrl,(void *)pValues1);
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
                    
                    // adding port config
                    if(pComponentPrivate->reconfigOutputPort){
                        OMX_PRCOMM2(pComponentPrivate->dbg, ": send event PortSettingsChanged for ouput port...\n");
                        pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                               pComponentPrivate->pHandle->pApplicationPrivate,
                                               OMX_EventPortSettingsChanged,
                                               OUTPUT_PORT_MP3DEC,
                                               0,
                                               NULL);
                    }
                    pComponentPrivate->bConfigData = 0;
                }
                pComponentPrivate->bDspStoppedWhileExecuting = OMX_FALSE;
                eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                           EMMCodecControlStart,
                                           (void *)pArgs);
                if(eError != OMX_ErrorNone) {
                    OMX_ERROR4(pComponentPrivate->dbg, "%d: Error Occurred in Codec Start..\n", __LINE__);
                    goto EXIT;
                }
                pComponentPrivate->SendAfterEOS = 0;
                //pComponentPrivate->first_buff = 0;
            } //end SendAfterEOS

            pLcmlHdr->pIpParam->bLastBuffer = 0;

            if(pBufHeader->nFlags & OMX_BUFFERFLAG_EOS) {
                OMX_PRBUFFER2(pComponentPrivate->dbg, ":: bLastBuffer Is Set Here....\n");
                pLcmlHdr->pIpParam->bLastBuffer = 1;
                pComponentPrivate->bIsEOFSent = 1;
                pComponentPrivate->SendAfterEOS = 1;
                pBufHeader->nFlags = 0;
            }

            /* Store time stamp information */
            pComponentPrivate->arrBufIndex[pComponentPrivate->IpBufindex] = pBufHeader->nTimeStamp;
            /*add on: Store tic count information*/
            pComponentPrivate->arrBufIndexTick[pComponentPrivate->IpBufindex] = pBufHeader->nTickCount;
            pComponentPrivate->IpBufindex++;
            pComponentPrivate->IpBufindex %= pPortDefIn->nBufferCountActual;
            
	    if(!pComponentPrivate->frameMode){
                if(pComponentPrivate->first_buff == 0){
                    pComponentPrivate->first_buff = 1;
                    pComponentPrivate->first_TS = pBufHeader->nTimeStamp;
                }
            }

            OMX_PRCOMM2(pComponentPrivate->dbg, "Comp:: Sending Filled Input buffer = %p, %ld\
                               to LCML\n",pBufHeader,pBufHeader->nFilledLen);

            if (pComponentPrivate->curState == OMX_StateExecuting) {
                if (!MP3DEC_IsPending(pComponentPrivate,pBufHeader,OMX_DirInput)) {
                    if(!pComponentPrivate->bDspStoppedWhileExecuting) {
                        if(!(pComponentPrivate->reconfigInputPort || pComponentPrivate->reconfigOutputPort)){
                            MP3DEC_SetPending(pComponentPrivate,pBufHeader,OMX_DirInput,__LINE__);
                            eError = LCML_QueueBuffer(pLcmlHandle->pCodecinterfacehandle,
                                                  EMMCodecInputBuffer,
                                                  pBufHeader->pBuffer,
                                                  pBufHeader->nAllocLen,
                                                  pBufHeader->nFilledLen,
                                                  (OMX_U8 *) pLcmlHdr->pIpParam,
                                                  sizeof(MP3DEC_UAlgInBufParamStruct),
                                                  NULL);
                            if (eError != OMX_ErrorNone) {
                                OMX_ERROR4(pComponentPrivate->dbg, "::Comp: SetBuff: IP: Error Occurred = %x\n", eError);
                                eError = OMX_ErrorHardware;
                                goto EXIT;
                            }
                            OMX_PRINT2(pComponentPrivate->dbg, "%d Sent IP %p, len %ld\n",__LINE__ , 
                                          pBufHeader->pBuffer,
                                          pBufHeader->nFilledLen);
                        }
                        else{
                            pComponentPrivate->pInputBufHdrPending[pComponentPrivate->nNumInputBufPending++] = pBufHeader;
                            OMX_PRBUFFER2(pComponentPrivate->dbg, "Don't queue buffers during a reconfig, num IN pending is %ld\n", pComponentPrivate->nNumInputBufPending);
                        }
                    }else {
                        OMX_PRBUFFER2(pComponentPrivate->dbg, "Calling EmptyBufferDone from line %d\n",__LINE__);
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
                        pComponentPrivate->nEmptyBufferDoneCount++;
                        SignalIfAllBuffersAreReturned(pComponentPrivate);
                    }
                    pComponentPrivate->lcml_nCntIp++;
                    pComponentPrivate->lcml_nIpBuf++;
                    pComponentPrivate->num_Sent_Ip_Buff++;
                    OMX_PRBUFFER2(pComponentPrivate->dbg, "Sending Input buffer to Codec\n");
                }
            }
            else if (pComponentPrivate->curState == OMX_StatePause) {
                pComponentPrivate->pInputBufHdrPending[pComponentPrivate->nNumInputBufPending++] = pBufHeader;
            }
        }else {
            pComponentPrivate->bBypassDSP = 1;
            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: Forcing EmptyBufferDone\n",__LINE__);
#ifdef __PERF_INSTRUMENTATION__
            PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                              PREF(pComponentPrivate->pInputBufferList->pBufHdr[0], pBuffer),
                              0,
                              PERF_ModuleHLMM);
#endif
            pComponentPrivate->cbInfo.EmptyBufferDone (pComponentPrivate->pHandle,
                                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                                       pComponentPrivate->pInputBufferList->pBufHdr[0]);
            pComponentPrivate->nEmptyBufferDoneCount++;
            SignalIfAllBuffersAreReturned(pComponentPrivate);
        }
        if(pBufHeader->pMarkData){
            OMX_PRBUFFER2(pComponentPrivate->dbg, ":Detected pBufHeader->pMarkData\n");

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
    }
    else if (eDir == OMX_DirOutput) {
        pComponentPrivate->nHandledFillThisBuffers++;
        if (pComponentPrivate->curState == OMX_StateIdle){
            pComponentPrivate->cbInfo.FillBufferDone (pComponentPrivate->pHandle,
                                                      pComponentPrivate->pHandle->pApplicationPrivate,
                                                      pBufHeader);
            OMX_PRBUFFER2(pComponentPrivate->dbg, ":: %d %s In idle state return output buffers\n", __LINE__, __FUNCTION__);
            pComponentPrivate->nFillBufferDoneCount++;
            SignalIfAllBuffersAreReturned(pComponentPrivate);
            goto EXIT;
        }
        LCML_DSP_INTERFACE *pLcmlHandle = (LCML_DSP_INTERFACE *)pComponentPrivate->pLcmlHandle;
        MP3D_LCML_BUFHEADERTYPE *pLcmlHdr;
        OMX_PRBUFFER2(pComponentPrivate->dbg, ": pComponentPrivate->lcml_nOpBuf = %ld\n",pComponentPrivate->lcml_nOpBuf);
        OMX_PRBUFFER2(pComponentPrivate->dbg, ": pComponentPrivate->lcml_nIpBuf = %ld\n",pComponentPrivate->lcml_nIpBuf);
        eError = MP3DEC_GetCorresponding_LCMLHeader(pComponentPrivate, pBufHeader->pBuffer, OMX_DirOutput, &pLcmlHdr);
        if (eError != OMX_ErrorNone) {
            OMX_ERROR4(pComponentPrivate->dbg, ":: Error: Invalid Buffer Came ...\n");
            goto EXIT;
        }
        OMX_PRDSP2(pComponentPrivate->dbg, ":::Calling LCML_QueueBuffer\n");
#ifdef __PERF_INSTRUMENTATION__
        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                          PREF(pBufHeader,pBuffer),
                          0,
                          PERF_ModuleCommonLayer);
#endif
        if (pComponentPrivate->bBypassDSP == 0) {
            if (pComponentPrivate->curState == OMX_StateExecuting) {
                if(!(pComponentPrivate->reconfigInputPort || pComponentPrivate->reconfigOutputPort)){
                    if (!MP3DEC_IsPending(pComponentPrivate,pBufHeader,OMX_DirOutput) && 
                        (pComponentPrivate->numPendingBuffers < pComponentPrivate->pOutputBufferList->numBuffers)){
                        if(!pComponentPrivate->bDspStoppedWhileExecuting){  
                            MP3DEC_SetPending(pComponentPrivate,pBufHeader,OMX_DirOutput,__LINE__);
                            eError = LCML_QueueBuffer(pLcmlHandle->pCodecinterfacehandle,
                                                  EMMCodecOuputBuffer,
                                                  pBufHeader->pBuffer,
                                                  pBufHeader->nAllocLen,
                                                  0,
                                                  (OMX_U8 *) pLcmlHdr->pOpParam,
                                                  sizeof(MP3DEC_UAlgOutBufParamStruct),
                                                  pBufHeader->pBuffer);
                            if (eError != OMX_ErrorNone ) {
                                OMX_ERROR4(pComponentPrivate->dbg, ":: Comp:: SetBuff OP: Error Occurred = %d\n", eError);
                                OMX_ERROR4(pComponentPrivate->dbg, "nFilledLen = %ld\n nAllocLen = %ld\n", pBufHeader->nFilledLen, pBufHeader->nAllocLen);
                                eError = OMX_ErrorHardware;
                                goto EXIT;
                            }
                            OMX_PRINT2(pComponentPrivate->dbg, "%d Sent OP %p\n",__LINE__,pBufHeader->pBuffer);
                            pComponentPrivate->lcml_nCntOp++;
                            pComponentPrivate->lcml_nOpBuf++;
                            pComponentPrivate->num_Op_Issued++;
                        }
                    }
                       
                } else{
                    pComponentPrivate->pOutputBufHdrPending[pComponentPrivate->nNumOutputBufPending++] = pBufHeader;
                    OMX_PRBUFFER2(pComponentPrivate->dbg, "Don't queue while doing a reconfig:: output buffer, num pending = %ld\n", pComponentPrivate->nNumOutputBufPending);
                }
            }else if (pComponentPrivate->curState == OMX_StatePause) {
                pComponentPrivate->pOutputBufHdrPending[pComponentPrivate->nNumOutputBufPending++] = pBufHeader;
            }
        }
        if (pComponentPrivate->bFlushOutputPortCommandPending) {
            OMX_SendCommand( pComponentPrivate->pHandle, OMX_CommandFlush, 1, NULL);
        }
    }
    else {
        OMX_ERROR4(pComponentPrivate->dbg, ": BufferHeader %p, Buffer %p Unknown ..........\n",pBufHeader, pBufHeader->pBuffer);
        eError = OMX_ErrorBadParameter;
    }
 EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, ": Exiting from  HandleDataBuf_FromApp: %x \n",eError);
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


/* ================================================================================= * */
/**
 * @fn MP3DEC_GetBufferDirection() function determines whether it is input buffer or
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

OMX_ERRORTYPE MP3DEC_GetBufferDirection(OMX_BUFFERHEADERTYPE *pBufHeader,
                                        OMX_DIRTYPE *eDir)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    MP3DEC_COMPONENT_PRIVATE *pComponentPrivate = pBufHeader->pPlatformPrivate;
    OMX_U32 nBuf = pComponentPrivate->pInputBufferList->numBuffers;
    OMX_BUFFERHEADERTYPE *pBuf = NULL;
    int flag = 1;
    OMX_U32 i=0;

    OMX_PRINT1(pComponentPrivate->dbg, ":: Entering GetBufferDirection Function\n");
    for(i=0; i<nBuf; i++) {
        pBuf = pComponentPrivate->pInputBufferList->pBufHdr[i];
        if(pBufHeader == pBuf) {
            *eDir = OMX_DirInput;
            OMX_PRINT2(pComponentPrivate->dbg, ":: Buffer %p is INPUT BUFFER\n", pBufHeader);
            flag = 0;
            goto EXIT;
        }
    }

    nBuf = pComponentPrivate->pOutputBufferList->numBuffers;

    for(i=0; i<nBuf; i++) {
        pBuf = pComponentPrivate->pOutputBufferList->pBufHdr[i];
        if(pBufHeader == pBuf) {
            *eDir = OMX_DirOutput;
            OMX_PRINT2(pComponentPrivate->dbg, ":: Buffer %p is OUTPUT BUFFER\n", pBufHeader);
            flag = 0;
            goto EXIT;
        }
    }

    if (flag == 1) {
        MP3D_OMX_ERROR_EXIT(eError, OMX_ErrorBadParameter,
                            "Buffer Not Found in List : OMX_ErrorBadParameter");
    }
 EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, ":: Exiting GetBufferDirection Function\n");
    return eError;
}

/** ================================================================================= * */
/**
 * @fn MP3DEC_LCML_Callback() function is callback which is called by LCML whenever
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
OMX_ERRORTYPE MP3DEC_LCML_Callback (TUsnCodecEvent event,void * args [10])
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U8 *pBuffer = args[1];
    MP3D_LCML_BUFHEADERTYPE *pLcmlHdr;
    OMX_COMPONENTTYPE *pHandle;
    LCML_DSP_INTERFACE *pLcmlHandle;
    OMX_U32 i;
    MP3DEC_BUFDATA *OutputFrames;
    MP3DEC_COMPONENT_PRIVATE* pComponentPrivate = NULL;
#ifdef RESOURCE_MANAGER_ENABLED 
    OMX_ERRORTYPE rm_error = OMX_ErrorNone;
#endif  
    static double time_stmp = 0;

    pComponentPrivate = (MP3DEC_COMPONENT_PRIVATE*)((LCML_DSP_INTERFACE*)args[6])->pComponentPrivate;    

    OMX_PRINT1(pComponentPrivate->dbg, ":: Entering the LCML_Callback() : event = %d\n",event);
    
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


    if(event == EMMCodecBufferProcessed) {
        if( args[0] == (void *)EMMCodecInputBuffer) {
            OMX_PRBUFFER2(pComponentPrivate->dbg, " :: Inside the LCML_Callback EMMCodecInputBuffer\n");
            OMX_PRBUFFER2(pComponentPrivate->dbg, ":: Input: pBufferr = %p\n", pBuffer);

            eError = MP3DEC_GetCorresponding_LCMLHeader(pComponentPrivate, pBuffer, OMX_DirInput, &pLcmlHdr);
            if (eError != OMX_ErrorNone) {
                OMX_ERROR4(pComponentPrivate->dbg, ":: Error: Invalid Buffer Came ...\n");
                goto EXIT;
            }
            OMX_PRBUFFER2(pComponentPrivate->dbg, ":: Input: pLcmlHeader = %p\n", pLcmlHdr);
            OMX_PRBUFFER2(pComponentPrivate->dbg, ":: Input: pLcmlHdr->eDir = %d\n", pLcmlHdr->eDir);
            OMX_PRBUFFER2(pComponentPrivate->dbg, ":: Input: Filled Len = %ld\n", pLcmlHdr->pBufHdr->nFilledLen);
            pLcmlHdr->pBufHdr->nFilledLen = 0;
            OMX_PRINT1(pComponentPrivate->dbg, ":: Input: Filled Len = %ld\n", pLcmlHdr->pBufHdr->nFilledLen);
#ifdef __PERF_INSTRUMENTATION__
            PERF_ReceivedFrame(pComponentPrivate->pPERFcomp,
                               PREF(pLcmlHdr->pBufHdr,pBuffer),
                               0,
                               PERF_ModuleCommonLayer);
#endif
            pComponentPrivate->lcml_nCntIpRes++;
            
            MP3DEC_ClearPending(pComponentPrivate,pLcmlHdr->pBufHdr,OMX_DirInput,__LINE__);

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
            OMX_PRBUFFER2(pComponentPrivate->dbg, " :: Inside the LCML_Callback EMMCodecOuputBuffer\n");

            OMX_PRINT2(pComponentPrivate->dbg, "%d\t\t\t\t............ Received OP %p, \tlen %d\n",
                          __LINE__,pBuffer,(int)args[8]);

            OMX_PRBUFFER2(pComponentPrivate->dbg, ":: Output: pBufferr = %p\n", pBuffer);
            if (!MP3DEC_IsValid(pComponentPrivate,pBuffer,OMX_DirOutput)) {

                OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: ############ FillBufferDone Invalid\n",__LINE__);
                /* If the buffer we get back from the DSP is not valid call FillBufferDone
                   on a valid buffer */
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
            }else {
                eError = MP3DEC_GetCorresponding_LCMLHeader(pComponentPrivate, pBuffer, OMX_DirOutput, &pLcmlHdr);
                if (eError != OMX_ErrorNone) {
                    OMX_ERROR4(pComponentPrivate->dbg, ":: Error: Invalid Buffer Came ...\n");
                    goto EXIT;
                }
                pLcmlHdr->pBufHdr->nFilledLen = (int)args[8];
                OMX_PRCOMM2(pComponentPrivate->dbg, ":: Output: pLcmlHeader = %p\n", pLcmlHdr);
                OMX_PRCOMM2(pComponentPrivate->dbg, ":: Output: pLcmlHdr->eDir = %d\n", pLcmlHdr->eDir);
                OMX_PRCOMM2(pComponentPrivate->dbg, ":: Output: Filled Len = %ld\n", pLcmlHdr->pBufHdr->nFilledLen);
                /* Recover MP3DEC_UAlgOutBufParamStruct from SN*/
                OutputFrames = pLcmlHdr->pBufHdr->pOutputPortPrivate;
                OutputFrames->nFrames = (OMX_U8)(pLcmlHdr->pOpParam->ulFrameCount);

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
                MP3DEC_ClearPending(pComponentPrivate,pLcmlHdr->pBufHdr,OMX_DirOutput,__LINE__);

                if (pComponentPrivate->pMarkData) {
                    pLcmlHdr->pBufHdr->pMarkData = pComponentPrivate->pMarkData;
                    pLcmlHdr->pBufHdr->hMarkTargetComponent = pComponentPrivate->hMarkTargetComponent;
                }
                pComponentPrivate->num_Reclaimed_Op_Buff++;

                if (pLcmlHdr->pOpParam->ulIsLastBuffer){ 		
                    OMX_PRBUFFER2(pComponentPrivate->dbg, "Adding EOS flag to the output buffer\n");
                    pLcmlHdr->pBufHdr->nFlags |= OMX_BUFFERFLAG_EOS;
                    pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                           pComponentPrivate->pHandle->pApplicationPrivate,
                                                           OMX_EventBufferFlag,
                                                           pLcmlHdr->pBufHdr->nOutputPortIndex,
                                                           pLcmlHdr->pBufHdr->nFlags, NULL);
                    pComponentPrivate->bIsEOFSent = 0;
                    pLcmlHdr->pOpParam->ulIsLastBuffer=0;
                }

                if(pComponentPrivate->frameMode){
                    /* Copying time stamp information to output buffer */
                    pLcmlHdr->pBufHdr->nTimeStamp = (OMX_TICKS)pComponentPrivate->arrBufIndex[pComponentPrivate->OpBufindex];
                }else{
                    if(pComponentPrivate->first_buff == 1){
                        pComponentPrivate->first_buff = 2;
                        pLcmlHdr->pBufHdr->nTimeStamp = pComponentPrivate->first_TS;
                        pComponentPrivate->temp_TS = pLcmlHdr->pBufHdr->nTimeStamp;
                        OMX_PRBUFFER2(pComponentPrivate->dbg, "first_ts = %lld\n",
                                   pComponentPrivate->temp_TS);
                    }else{ 
                        time_stmp = pLcmlHdr->pBufHdr->nFilledLen / (pComponentPrivate->pcmParams->nChannels * 
                                                                     (pComponentPrivate->pcmParams->nBitPerSample / 8));
                        time_stmp = (time_stmp / pComponentPrivate->pcmParams->nSamplingRate) * 1000000;
                        /* Update time stamp information */
                        pComponentPrivate->temp_TS += time_stmp;
                        pLcmlHdr->pBufHdr->nTimeStamp = pComponentPrivate->temp_TS;
                        OMX_PRBUFFER2(pComponentPrivate->dbg, "out ts = %lld\n",
                                   pComponentPrivate->temp_TS);
                    }
                }
                /*add on: Copyint tick count information to output buffer*/
                pLcmlHdr->pBufHdr->nTickCount = (OMX_U32)pComponentPrivate->arrBufIndexTick[pComponentPrivate->OpBufindex];
                pComponentPrivate->OpBufindex++;
                pComponentPrivate->OpBufindex %= pComponentPrivate->pPortDef[OMX_DirInput]->nBufferCountActual;


#ifdef __PERF_INSTRUMENTATION__
                PERF_SendingBuffer(pComponentPrivate->pPERFcomp,
                                   pLcmlHdr->pBufHdr->pBuffer,
                                   pLcmlHdr->pBufHdr->nFilledLen,
                                   PERF_ModuleHLMM);
#endif

                pComponentPrivate->cbInfo.FillBufferDone (pComponentPrivate->pHandle,
                                                          pComponentPrivate->pHandle->pApplicationPrivate,
                                                          pLcmlHdr->pBufHdr);
                pComponentPrivate->lcml_nOpBuf--;
                pComponentPrivate->app_nBuf++;
                pComponentPrivate->nFillBufferDoneCount++;
                SignalIfAllBuffersAreReturned(pComponentPrivate);
            }
        }
    }else if(event == EMMCodecProcessingStoped) { 
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
						  pComponentPrivate->pOutputBufHdrPending[i]
						  );
                    pComponentPrivate->nFillBufferDoneCount++;
			        pComponentPrivate->pOutputBufHdrPending[i] = NULL;
                    SignalIfAllBuffersAreReturned(pComponentPrivate);
			}
			pComponentPrivate->nNumOutputBufPending=0;

        pthread_mutex_lock(&pComponentPrivate->codecStop_mutex);
        if(pComponentPrivate->codecStop_waitingsignal == 0){
            pComponentPrivate->codecStop_waitingsignal = 1;             
            pthread_cond_signal(&pComponentPrivate->codecStop_threshold);
            OMX_ERROR4(pComponentPrivate->dbg, "stop ack. received. stop waiting for sending disable command completed\n");
        }
        pthread_mutex_unlock(&pComponentPrivate->codecStop_mutex);
        if (!pComponentPrivate->bNoIdleOnStop) {
            pComponentPrivate->curState = OMX_StateIdle;

#ifdef RESOURCE_MANAGER_ENABLED
            rm_error = RMProxy_NewSendCommand(pComponentPrivate->pHandle,
                                              RMProxy_StateSet,
                                              OMX_MP3_Decoder_COMPONENT,
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
                OMX_PRINT2(pComponentPrivate->dbg, ":: OMX has returned all input and output buffers\n");
            }
            if(pComponentPrivate->bPreempted==0){
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
                                                       OMX_TI_ErrorMajor,
                                                       NULL);
            }
        }else{
            pComponentPrivate->bDspStoppedWhileExecuting = OMX_TRUE;
            pComponentPrivate->bNoIdleOnStop = OMX_FALSE;
        }
    }
    else if(event == EMMCodecAlgCtrlAck) {
        OMX_PRDSP2(pComponentPrivate->dbg, "GOT MESSAGE USN_DSPACK_ALGCTRL \n");
    }
    else if (event == EMMCodecDspError) { 

        OMX_PRDSP1(pComponentPrivate->dbg, ":: commandedState  = %p\n",args[0]);
        OMX_PRINT2(pComponentPrivate->dbg, ":: arg4 = %p\n",args[4]);
        OMX_PRINT2(pComponentPrivate->dbg, ":: arg5 = %p\n",args[5]);
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
                MP3DEC_HandleUSNError (pComponentPrivate, (OMX_U32)args[5]);
                break;
            default:
                break;
        }
    } else if (event == EMMCodecStrmCtrlAck) {
 

        OMX_PRDSP2(pComponentPrivate->dbg, ":: GOT MESSAGE USN_DSPACK_STRMCTRL ----\n");
        if (args[1] == (void *)USN_STRMCMD_FLUSH) {
            pHandle = pComponentPrivate->pHandle; 
            if ( args[2] == (void *)EMMCodecInputBuffer) {
                if (args[0] == (void *)USN_ERR_NONE ) {
                    OMX_PRCOMM2(pComponentPrivate->dbg, "Flushing input port %d\n",__LINE__);
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
                        pComponentPrivate->nEmptyBufferDoneCount++;
                        pComponentPrivate->pInputBufHdrPending[i] = NULL;
                        SignalIfAllBuffersAreReturned(pComponentPrivate);
                    }
                    pComponentPrivate->nNumInputBufPending=0;

                    pthread_mutex_lock(&pComponentPrivate->codecFlush_mutex);
                    if(pComponentPrivate->codecFlush_waitingsignal == 0){
                        pComponentPrivate->codecFlush_waitingsignal = 1; 
                        pthread_cond_signal(&pComponentPrivate->codecFlush_threshold);
                        OMX_ERROR4(pComponentPrivate->dbg, "flush ack. received. for input port\n");
                    }     
                    pthread_mutex_unlock(&pComponentPrivate->codecFlush_mutex);

                    pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventCmdComplete, 
                                                           OMX_CommandFlush,
                                                           MP3D_INPUT_PORT, 
                                                           NULL); 
                } else {
                    OMX_ERROR4(pComponentPrivate->dbg, "LCML reported error while flushing input port\n");
                    goto EXIT;                            
                }
            }
            else if ( args[2] == (void *)EMMCodecOuputBuffer) { 

  

                if (args[0] == (void *)USN_ERR_NONE ) {                      
                    OMX_PRCOMM2(pComponentPrivate->dbg, "Flushing output port %d\n",__LINE__);
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
                        pComponentPrivate->nFillBufferDoneCount++;
                        pComponentPrivate->pOutputBufHdrPending[i] = NULL;
                        SignalIfAllBuffersAreReturned(pComponentPrivate);
                    }
                    pComponentPrivate->nNumOutputBufPending=0;

                    pthread_mutex_lock(&pComponentPrivate->codecFlush_mutex);
                    if(pComponentPrivate->codecFlush_waitingsignal == 0){
                        pComponentPrivate->codecFlush_waitingsignal = 1; 
                        pthread_cond_signal(&pComponentPrivate->codecFlush_threshold);
                        OMX_ERROR4(pComponentPrivate->dbg, "flush ack. received. for output port\n");
                    }     
                    pthread_mutex_unlock(&pComponentPrivate->codecFlush_mutex);
                    pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle, 
                                                           pComponentPrivate->pHandle->pApplicationPrivate,
                                                           OMX_EventCmdComplete, 
                                                           OMX_CommandFlush,
                                                           MP3D_OUTPUT_PORT,
                                                           NULL);
                } else {
                    OMX_ERROR4(pComponentPrivate->dbg, "LCML reported error while flushing output port\n");
                    goto EXIT;                            
                }
            }
        }
    }
    else if (event == EMMCodecProcessingPaused) { 

        pComponentPrivate->curState = OMX_StatePause;
        pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle, 
                                               pComponentPrivate->pHandle->pApplicationPrivate,
                                               OMX_EventCmdComplete, OMX_CommandStateSet,
                                               pComponentPrivate->curState, NULL);
    }
#ifdef _ERROR_PROPAGATION__
    else if (event == EMMCodecInitError){
 

        /* Cheking for MMU_fault */
        if((args[4] == (void*)USN_ERR_UNKNOWN_MSG) && (args[5] == (void*)NULL)) {
            OMX_ERROR4(pComponentPrivate->dbg, "%d :: UTIL: MMU_Fault \n",__LINE__);
            pComponentPrivate->bIsInvalidState = OMX_TRUE;
            pComponentPrivate->curState = OMX_StateInvalid;
            pHandle = pComponentPrivate->pHandle;
            pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                   pHandle->pApplicationPrivate,
                                                   OMX_EventError,
                                                   OMX_ErrorHardware, 
                                                   OMX_TI_ErrorSevere,
                                                   NULL);
        }   
    }
    else if (event == EMMCodecInternalError){

 

        /* Cheking for MMU_fault */
        if((args[4] == (void*)USN_ERR_UNKNOWN_MSG) && (args[5] == (void*)NULL)) {
            OMX_ERROR4(pComponentPrivate->dbg, "%d :: UTIL: MMU_Fault \n",__LINE__);
            pComponentPrivate->bIsInvalidState = OMX_TRUE;
            pComponentPrivate->curState = OMX_StateInvalid;
            pHandle = pComponentPrivate->pHandle;
            pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                   pHandle->pApplicationPrivate,
                                                   OMX_EventError,
                                                   OMX_ErrorHardware, 
                                                   OMX_TI_ErrorSevere,
                                                   NULL);
        }

    }
#endif
 EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, ":: Exiting the LCML_Callback() \n");
 
    return eError;
}


/* ================================================================================= * */
/**
 * @fn MP3DEC_GetCorresponding_LCMLHeader() function gets the corresponding LCML
 * header from the actual data buffer for required processing.
 *
 * @param *pBuffer This is the data buffer pointer. 
 *
 * @param eDir   This is direction of buffer. Input/Output.
 *
 * @param *MP3D_LCML_BUFHEADERTYPE  This is pointer to LCML Buffer Header.
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
OMX_ERRORTYPE MP3DEC_GetCorresponding_LCMLHeader(MP3DEC_COMPONENT_PRIVATE* pComponentPrivate,
                                                 OMX_U8 *pBuffer,
                                                 OMX_DIRTYPE eDir,
                                                 MP3D_LCML_BUFHEADERTYPE **ppLcmlHdr)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    MP3D_LCML_BUFHEADERTYPE *pLcmlBufHeader;
    int nIpBuf=0, nOpBuf=0, i=0;

    OMX_PRINT1(pComponentPrivate->dbg, ":: Entering the MP3DEC_GetCorresponding_LCMLHeader()\n");

    OMX_PRINT2(pComponentPrivate->dbg, ":: eDir = %d\n",eDir);

    while (!pComponentPrivate->bInitParamsInitialized) {
#ifndef UNDER_CE
        sched_yield();
#else
        Sleep(0);
#endif
    }


    if(eDir == OMX_DirInput) {
        OMX_PRDSP2(pComponentPrivate->dbg, ":: In GetCorresponding_LCMLHeader()\n");

        nIpBuf = pComponentPrivate->pInputBufferList->numBuffers;

        pLcmlBufHeader = pComponentPrivate->pLcmlBufHeader[MP3D_INPUT_PORT];

        for(i=0; i<nIpBuf; i++) {
            OMX_PRBUFFER2(pComponentPrivate->dbg, "pBuffer = %p\n",pBuffer);
            OMX_PRBUFFER2(pComponentPrivate->dbg, "pLcmlBufHeader->pBufHdr->pBuffer = %p\n",pLcmlBufHeader->pBufHdr->pBuffer);
            if(pBuffer == pLcmlBufHeader->pBufHdr->pBuffer) {
                *ppLcmlHdr = pLcmlBufHeader;
                OMX_PRINT1(pComponentPrivate->dbg, "::Corresponding LCML Header Found\n");
                goto EXIT;
            }
            pLcmlBufHeader++;
        }
    } else if (eDir == OMX_DirOutput) {
        i = 0;
        nOpBuf = pComponentPrivate->pOutputBufferList->numBuffers;

        pLcmlBufHeader = pComponentPrivate->pLcmlBufHeader[MP3D_OUTPUT_PORT];
        OMX_PRBUFFER2(pComponentPrivate->dbg, ":: nOpBuf = %d\n",nOpBuf);

        for(i=0; i<nOpBuf; i++) {
            OMX_PRBUFFER2(pComponentPrivate->dbg, "pBuffer = %p\n",pBuffer);
            OMX_PRBUFFER2(pComponentPrivate->dbg, "pLcmlBufHeader->pBufHdr->pBuffer = %p\n",pLcmlBufHeader->pBufHdr->pBuffer);

            if(pBuffer == pLcmlBufHeader->pBufHdr->pBuffer) {
                *ppLcmlHdr = pLcmlBufHeader;
                OMX_PRINT1(pComponentPrivate->dbg, "::Corresponding LCML Header Found\n");
                goto EXIT;
            }
            pLcmlBufHeader++;
        }
    } else {
        OMX_PRBUFFER2(pComponentPrivate->dbg, ":: Invalid Buffer Type :: exiting...\n");
    }

 EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, ":: Exiting the GetCorresponding_LCMLHeader() \n");
    return eError;
}

/* ================================================================================= * */
/**
 * @fn MP3DEC_GetLCMLHandle() function gets the LCML handle and interacts with LCML
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
OMX_HANDLETYPE MP3DEC_GetLCMLHandle(MP3DEC_COMPONENT_PRIVATE *pComponentPrivate)
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
        OMXDBG_PRINT(stderr, ERROR, 4, 0, "eError != OMX_ErrorNone...\n");
        pHandle = NULL;
        goto EXIT;
    }

    ((LCML_DSP_INTERFACE*)pHandle)->pComponentPrivate = pComponentPrivate;

 EXIT:
    return pHandle;
}
#else
/* WINDOWS Explicit dll load procedure */
OMX_HANDLETYPE MP3DEC_GetLCMLHandle(MP3DEC_COMPONENT_PRIVATE *pComponentPrivate)
{
    typedef OMX_ERRORTYPE (*LPFNDLLFUNC1)(OMX_HANDLETYPE);
    OMX_HANDLETYPE pHandle = NULL;
    OMX_ERRORTYPE eError;
    LPFNDLLFUNC1 fpGetHandle1;

    g_hLcmlDllHandle = LoadLibraryEx(TEXT("OAF_BML.dll"), NULL,0);
    if (g_hLcmlDllHandle == NULL)
        {
            /* fputs(dlerror(), stderr); */
	    OMXDBG_PRINT(stderr, ERROR, 4, 0, "BML Load Failed!!!\n");
            return pHandle;
        }
    fpGetHandle1 = (LPFNDLLFUNC1)GetProcAddress(g_hLcmlDllHandle,TEXT("GetHandle"));
    if (!fpGetHandle1)
        {
            /* handle the error*/
            FreeLibrary(g_hLcmlDllHandle);
            g_hLcmlDllHandle = NULL;
            return pHandle;
        }
    /* call the function */
    eError = fpGetHandle1(&pHandle);
    if(eError != OMX_ErrorNone) 
        {
            eError = OMX_ErrorUndefined;
            OMXDBG_PRINT(stderr, ERROR, 4, 0, "eError != OMX_ErrorNone...\n");
            FreeLibrary(g_hLcmlDllHandle);
            g_hLcmlDllHandle = NULL;
            pHandle = NULL;
            return pHandle;
        }

    ((LCML_DSP_INTERFACE*)pHandle)->pComponentPrivate = pComponentPrivate;

    return pHandle;
}
#endif

#ifndef UNDER_CE
OMX_ERRORTYPE MP3DECFreeLCMLHandle(MP3DEC_COMPONENT_PRIVATE *pComponentPrivate)
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
OMX_ERRORTYPE MP3DECFreeLCMLHandle(MP3DEC_COMPONENT_PRIVATE *pComponentPrivate)
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

/* ========================================================================== */
/**
 * @MP3DEC_CleanupInitParams() This function is called by the component during
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

void MP3DEC_CleanupInitParams(OMX_HANDLETYPE pComponent)
{
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    MP3DEC_COMPONENT_PRIVATE *pComponentPrivate = (MP3DEC_COMPONENT_PRIVATE *)
        pHandle->pComponentPrivate;
    MP3D_LCML_BUFHEADERTYPE *pTemp_lcml;

    OMX_U32 nIpBuf = pComponentPrivate->nRuntimeInputBuffers;
    OMX_U32 nOpBuf = pComponentPrivate->nRuntimeOutputBuffers;

    OMX_U32 i=0;

    OMX_PRINT1(pComponentPrivate->dbg, ":: MP3DEC_CleanupInitParams()\n");

    OMX_PRBUFFER2(pComponentPrivate->dbg, ":: Freeing:  pComponentPrivate->strmAttr = %p\n", pComponentPrivate->strmAttr);
    OMX_MEMFREE_STRUCT(pComponentPrivate->strmAttr); 

    pTemp_lcml = pComponentPrivate->pLcmlBufHeader[MP3D_INPUT_PORT];
	
    for(i=0; i<nIpBuf; i++) {
        OMX_PRBUFFER2(pComponentPrivate->dbg, ":: Freeing: pTemp_lcml->pIpParam = %p\n",pTemp_lcml->pIpParam);
        OMX_MEMFREE_STRUCT_DSPALIGN(pTemp_lcml->pIpParam, MP3DEC_UAlgInBufParamStruct);

        pTemp_lcml++;
    }


    
    OMX_PRBUFFER2(pComponentPrivate->dbg, ":: Freeing pComponentPrivate->pLcmlBufHeader[MP3D_INPUT_PORT] = %p\n",
                    pComponentPrivate->pLcmlBufHeader[MP3D_INPUT_PORT]);
    OMX_MEMFREE_STRUCT(pComponentPrivate->pLcmlBufHeader[MP3D_INPUT_PORT]);

    pTemp_lcml = pComponentPrivate->pLcmlBufHeader[MP3D_OUTPUT_PORT];
    for(i=0; i<nOpBuf; i++) {
        OMX_PRBUFFER2(pComponentPrivate->dbg, ":: Freeing: pTemp_lcml->pOpParam = %p\n",pTemp_lcml->pOpParam);
        OMX_MEMFREE_STRUCT_DSPALIGN(pTemp_lcml->pOpParam, MP3DEC_UAlgOutBufParamStruct);
        pTemp_lcml++;
    }

    OMX_PRBUFFER2(pComponentPrivate->dbg, ":: Freeing: pComponentPrivate->pLcmlBufHeader[MP3D_OUTPUT_PORT] = %p\n",
                    pComponentPrivate->pLcmlBufHeader[MP3D_OUTPUT_PORT]);
    OMX_MEMFREE_STRUCT(pComponentPrivate->pLcmlBufHeader[MP3D_OUTPUT_PORT]);
    
    OMX_MEMFREE_STRUCT_DSPALIGN(pComponentPrivate->pParams, USN_AudioCodecParams);
    
    OMX_MEMFREE_STRUCT_DSPALIGN(pComponentPrivate->ptAlgDynParams, MP3DEC_UALGParams);
    
    OMX_PRINT1(pComponentPrivate->dbg, "Exiting Successfully MP3DEC_CleanupInitParams()\n");

}

/* ========================================================================== */
/**
* @MP3DEC_CleanupInitParamsEx() This function is called by the component during
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

void MP3DEC_CleanupInitParamsEx(OMX_HANDLETYPE pComponent,OMX_U32 indexport)
{
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    MP3DEC_COMPONENT_PRIVATE *pComponentPrivate =
        (MP3DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    MP3D_LCML_BUFHEADERTYPE *pTemp_lcml;
    OMX_U32 nIpBuf = 0;
    OMX_U32 nOpBuf = 0;
    OMX_U32 i=0;

    if(indexport == 0 || indexport == -1){
        nIpBuf = pComponentPrivate->nRuntimeInputBuffers;
        pTemp_lcml = pComponentPrivate->pLcmlBufHeader[MP3D_INPUT_PORT];
        for(i=0; i<nIpBuf; i++) {
            OMX_PRBUFFER2(pComponentPrivate->dbg, "Freeing: pIpParam = %p\n",
                          pTemp_lcml->pIpParam);
            OMX_MEMFREE_STRUCT_DSPALIGN(pTemp_lcml->pIpParam, MP3DEC_UAlgInBufParamStruct);
            pTemp_lcml++;
        }

        OMX_PRBUFFER2(pComponentPrivate->dbg, "Freeing pLcmlBufHeader[MP3D_INPUT_PORT] = %p\n",
                      pComponentPrivate->pLcmlBufHeader[MP3D_INPUT_PORT]);
        OMX_MEMFREE_STRUCT(pComponentPrivate->pLcmlBufHeader[MP3D_INPUT_PORT]);

    }else if(indexport == 1 || indexport == -1){
        nOpBuf = pComponentPrivate->nRuntimeOutputBuffers;
        pTemp_lcml = pComponentPrivate->pLcmlBufHeader[MP3D_OUTPUT_PORT];
        for(i=0; i<nOpBuf; i++) {
            OMX_PRBUFFER2(pComponentPrivate->dbg, "Freeing: pOpParam = %p\n",
                          pTemp_lcml->pOpParam);
            OMX_MEMFREE_STRUCT_DSPALIGN(pTemp_lcml->pOpParam, MP3DEC_UAlgOutBufParamStruct);
            pTemp_lcml++;
        }

        OMX_PRBUFFER2(pComponentPrivate->dbg, "Freeing: pLcmlBufHeader[MP3D_OUTPUT_PORT] = %p\n",
                      pComponentPrivate->pLcmlBufHeader[MP3D_OUTPUT_PORT]);
        OMX_MEMFREE_STRUCT(pComponentPrivate->pLcmlBufHeader[MP3D_OUTPUT_PORT]);

    }else{
        OMX_ERROR4(pComponentPrivate->dbg, "Bad indexport!\n");
    }
}

/* ========================================================================== */
/**
 * @MP3DEC_SetPending() This function marks the buffer as pending when it is sent
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
void MP3DEC_SetPending(MP3DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir, OMX_U32 lineNumber)
{
    OMX_U16 i;

    if (eDir == OMX_DirInput) {
        for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pInputBufferList->pBufHdr[i]) {
                pComponentPrivate->pInputBufferList->bBufferPending[i] = 1;
                OMX_PRBUFFER2(pComponentPrivate->dbg, "INPUT BUFFER %d IS PENDING Line %ld\n",i,lineNumber);
            }
        }
    }
    else {
        for (i=0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pOutputBufferList->pBufHdr[i]) {
                pComponentPrivate->pOutputBufferList->bBufferPending[i] = 1;
                OMX_PRBUFFER2(pComponentPrivate->dbg, "OUTPUT BUFFER %d IS PENDING Line %ld\n",i,lineNumber);
            }
        }
    }
}

/* ========================================================================== */
/**
 * @MP3DEC_ClearPending() This function clears the buffer status from pending
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

void MP3DEC_ClearPending(MP3DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir, OMX_U32 lineNumber)
{
    OMX_U16 i;

    if (eDir == OMX_DirInput) {
        for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pInputBufferList->pBufHdr[i]) {
                pComponentPrivate->pInputBufferList->bBufferPending[i] = 0;
                OMX_PRBUFFER2(pComponentPrivate->dbg, "INPUT BUFFER %d IS RECLAIMED Line %ld\n",i,lineNumber);
            }
        }
    }
    else {
        for (i=0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pOutputBufferList->pBufHdr[i]) {
                pComponentPrivate->pOutputBufferList->bBufferPending[i] = 0;
                OMX_PRBUFFER2(pComponentPrivate->dbg, "OUTPUT BUFFER %d IS RECLAIMED Line %ld\n",i,lineNumber);
            }
        }
    }
}
  
/* ========================================================================== */
/**
 * @MP3DEC_IsPending() This function checks whether or not a buffer is pending.
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

OMX_U32 MP3DEC_IsPending(MP3DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir)
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
 * @MP3DEC_IsValid() This function identifies whether or not buffer recieved from
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

OMX_U32 MP3DEC_IsValid(MP3DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U8 *pBuffer, OMX_DIRTYPE eDir)
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
 * @MP3DECFill_LCMLInitParamsEx() This function initializes the init parameter of
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

OMX_ERRORTYPE MP3DECFill_LCMLInitParamsEx(OMX_HANDLETYPE pComponent, OMX_U32 indexport)

{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nIpBuf,nIpBufSize,nOpBuf,nOpBufSize;
    OMX_U16 i;
    OMX_BUFFERHEADERTYPE *pTemp;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    MP3DEC_COMPONENT_PRIVATE *pComponentPrivate =
        (MP3DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    MP3D_LCML_BUFHEADERTYPE *pTemp_lcml;
    OMX_U32 size_lcml;
    OMX_U8 *ptr;

    OMX_PRINT1(pComponentPrivate->dbg, ":: Entered Fill_LCMLInitParams");

    nIpBuf = pComponentPrivate->pInputBufferList->numBuffers;
    nOpBuf = pComponentPrivate->pOutputBufferList->numBuffers;
    nIpBufSize = pComponentPrivate->pPortDef[MP3D_INPUT_PORT]->nBufferSize;
    nOpBufSize = pComponentPrivate->pPortDef[MP3D_OUTPUT_PORT]->nBufferSize;

    OMX_PRBUFFER2(pComponentPrivate->dbg, "Input Buffer Count = %ld\n",nIpBuf);
    OMX_PRBUFFER2(pComponentPrivate->dbg, "Input Buffer Size = %ld\n",nIpBufSize);
    OMX_PRBUFFER2(pComponentPrivate->dbg, "Output Buffer Count = %ld\n",nOpBuf);
    OMX_PRBUFFER2(pComponentPrivate->dbg, "Output Buffer Size = %ld\n",nOpBufSize);

    if(indexport == 0 || indexport == -1){

        OMX_PRBUFFER2(pComponentPrivate->dbg, ":: bufAlloced = %d\n",pComponentPrivate->bufAlloced);
        size_lcml = nIpBuf * sizeof(MP3D_LCML_BUFHEADERTYPE);

        OMX_MALLOC_SIZE(ptr,size_lcml,OMX_U8);
        pTemp_lcml = (MP3D_LCML_BUFHEADERTYPE *)ptr;

        pComponentPrivate->pLcmlBufHeader[MP3D_INPUT_PORT] = pTemp_lcml;

        for (i=0; i<nIpBuf; i++) {
            pTemp = pComponentPrivate->pInputBufferList->pBufHdr[i];
            pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);

            pTemp->nAllocLen = nIpBufSize;
            pTemp->nFilledLen = nIpBufSize;
            pTemp->nVersion.s.nVersionMajor = MP3DEC_MAJOR_VER;
            pTemp->nVersion.s.nVersionMinor = MP3DEC_MINOR_VER;

            pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
            pTemp->nTickCount = 0;

            pTemp_lcml->pBufHdr = pTemp;
            pTemp_lcml->eDir = OMX_DirInput;
            pTemp_lcml->pOtherParams[i] = NULL;

            OMX_MALLOC_SIZE_DSPALIGN(pTemp_lcml->pIpParam,
                                 sizeof(MP3DEC_UAlgInBufParamStruct),
                                 MP3DEC_UAlgInBufParamStruct);
            pTemp_lcml->pIpParam->bLastBuffer = 0;

            pTemp->nFlags = NORMAL_BUFFER;
            ((MP3DEC_COMPONENT_PRIVATE *) pTemp->pPlatformPrivate)->pHandle = pHandle;

            OMX_PRBUFFER2(pComponentPrivate->dbg, "::Comp: InBuffHeader[%d] = %p\n", i, pTemp);
            OMX_PRBUFFER2(pComponentPrivate->dbg, "::Comp:  >>>> InputBuffHeader[%d]->pBuffer = %p\n", i, pTemp->pBuffer);
            OMX_PRBUFFER2(pComponentPrivate->dbg, "::Comp: Ip : pTemp_lcml[%d] = %p\n", i, pTemp_lcml);
  
            pTemp_lcml++;
        }
    }

    if(indexport == 1 || indexport == -1){
        size_lcml = nOpBuf * sizeof(MP3D_LCML_BUFHEADERTYPE);
        OMX_MALLOC_SIZE(pTemp_lcml,size_lcml,MP3D_LCML_BUFHEADERTYPE);
        pComponentPrivate->pLcmlBufHeader[MP3D_OUTPUT_PORT] = pTemp_lcml;

        for (i=0; i<nOpBuf; i++) {
            pTemp = pComponentPrivate->pOutputBufferList->pBufHdr[i];
            pTemp->nSize = sizeof(OMX_BUFFERHEADERTYPE);
            pTemp->nAllocLen = nOpBufSize;
            pTemp->nFilledLen = nOpBufSize;
            pTemp->nVersion.s.nVersionMajor = MP3DEC_MAJOR_VER;
            pTemp->nVersion.s.nVersionMinor = MP3DEC_MINOR_VER;

            pTemp->pPlatformPrivate = pHandle->pComponentPrivate;
            pTemp->nTickCount = 0;

            pTemp_lcml->pBufHdr = pTemp;
            pTemp_lcml->eDir = OMX_DirOutput;
            pTemp_lcml->pOtherParams[i] = NULL;

            OMX_MALLOC_SIZE_DSPALIGN(pTemp_lcml->pOpParam,
                                 sizeof(MP3DEC_UAlgOutBufParamStruct),
                                 MP3DEC_UAlgOutBufParamStruct);
            pTemp_lcml->pOpParam->ulFrameCount = DONT_CARE;
            pTemp_lcml->pOpParam->ulIsLastBuffer = 0;

            pTemp->nFlags = NORMAL_BUFFER;
            ((MP3DEC_COMPONENT_PRIVATE *)pTemp->pPlatformPrivate)->pHandle = pHandle;
            OMX_PRBUFFER2(pComponentPrivate->dbg, "::Comp:  >>>>>>>>>>>>> OutBuffHeader[%d] = %p\n", i, pTemp);
            OMX_PRBUFFER2(pComponentPrivate->dbg, "::Comp:  >>>> OutBuffHeader[%d]->pBuffer = %p\n", i, pTemp->pBuffer);
            OMX_PRBUFFER2(pComponentPrivate->dbg, "::Comp: Op : pTemp_lcml[%d] = %p\n", i, pTemp_lcml);
            pTemp_lcml++;
        }
    }
    pComponentPrivate->bPortDefsAllocated = 1;

    OMX_PRINT1(pComponentPrivate->dbg, ":: Exiting Fill_LCMLInitParams");

    pComponentPrivate->bInitParamsInitialized = 1;

 EXIT:
    return eError;
}


OMX_U32 MP3DEC_GetBits(OMX_U32* nPosition, OMX_U8 nBits, OMX_U8* pBuffer, OMX_BOOL bIcreasePosition)
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
void SignalIfAllBuffersAreReturned(MP3DEC_COMPONENT_PRIVATE *pComponentPrivate)
{
    if((pComponentPrivate->nEmptyThisBufferCount == pComponentPrivate->nEmptyBufferDoneCount) && (pComponentPrivate->nFillThisBufferCount == pComponentPrivate->nFillBufferDoneCount))
    {
        if(pthread_mutex_lock(&bufferReturned_mutex) != 0) 
        {
            OMX_ERROR4(pComponentPrivate->dbg, "%d :: bufferReturned_mutex mutex lock error\n",__LINE__);
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

/*  =========================================================================*/
/*  func    MP3DEC_HandleUSNError
/*
/*  desc    Handles error messages returned by the dsp
/*
/*@return n/a
/*
/*  =========================================================================*/
void MP3DEC_HandleUSNError (MP3DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U32 arg)
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
            OMX_ERROR4(pComponentPrivate->dbg,  "Algorithm Error" );
            /* all of these are informative messages, Algo can recover, no need to notify the
             * IL Client at this stage of the implementation */
            break;
        case IUALG_WARN_PLAYCOMPLETED:

            {
            OMX_ERROR4(pComponentPrivate->dbg, "IUALG_WARN_PLAYCOMPLETED!\n");
#ifndef UNDER_CE
            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                   OMX_EventBufferFlag,
                                                   OMX_DirOutput,
                                                   OMX_BUFFERFLAG_EOS,
                                                   NULL);
            if(pComponentPrivate->dasfmode){
                pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                                       OMX_EventBufferFlag,
                                                       (OMX_U32)NULL,
                                                       OMX_BUFFERFLAG_EOS,
                                                       NULL);
            }
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
void MP3_ResourceManagerCallback(RMPROXY_COMMANDDATATYPE cbData)
{
    OMX_COMMANDTYPE Cmd = OMX_CommandStateSet;
    OMX_STATETYPE state = OMX_StateIdle;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)cbData.hComponent;
    MP3DEC_COMPONENT_PRIVATE *pCompPrivate = NULL;

    pCompPrivate = (MP3DEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

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
