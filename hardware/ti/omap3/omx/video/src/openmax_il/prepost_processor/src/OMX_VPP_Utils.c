
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
* @file OMX_VPP_Utils.c
*
* This file implements OMX Component for PCM decoder that 
* is fully compliant with the OMX specification 1.1.
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
*! 13-Dec-2005 mf:
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
#include <malloc.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <sched.h>
#include <pthread.h>
#endif

#include <dbapi.h>
#include <string.h>
#include <stdio.h>


#include "LCML_DspCodec.h"
#include "OMX_VPP.h"
#include "OMX_VPP_Utils.h"
#include "VPPsocket_ti.h"
#include "OMX_VPP_CompThread.h"
#include <OMX_Component.h>
#include "usn.h"

#ifdef RESOURCE_MANAGER_ENABLED
#include <ResourceManagerProxyAPI.h>
#endif

#define OMX_VPP_STRNCPY(dst, src, size)         strncpy(dst, src, size)
#define OMX_VPP_ITOA(value, buffer)    sprintf((char*)buffer, "%d", value);
#define OMX_VPP_MAX(x, y)                       ((x) > (y) ? (x) : (y))
#ifdef UNDER_CE
HINSTANCE g_hLcmlDllHandle = NULL;
#endif

#ifdef RESOURCE_MANAGER_ENABLED
void ResourceManagerCallback(RMPROXY_COMMANDDATATYPE cbData);
#endif

/* ========================================================================== */
/**
* @
*
* @param 
* @param 
* 
* @pre 
*
* @post 
*
* @return none
*/
/* ========================================================================== */
OMX_ERRORTYPE VPP_IsValidBuffer(OMX_BUFFERHEADERTYPE *pBufHeader, 
                                VPP_COMPONENT_PRIVATE *pComponentPrivate,
                                OMX_U32 pIndex,
                                OMX_U32 *pCount)
{
    OMX_U32 nCount = 0;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    VPP_DPRINT("Entering Valid buffer -- %lu\n ",pIndex); 

    while (pComponentPrivate->sCompPorts[pIndex].pVPPBufHeader[nCount].pBufHeader != pBufHeader)
    {
        nCount ++;
        if (nCount >= NUM_OF_VPP_BUFFERS) { 
            OMX_SET_ERROR_BAIL(eError, OMX_ErrorBadParameter);
        }
    }
    *pCount = nCount;
    VPP_DPRINT("Exiting Valid buffer -- %lu\n ",nCount); 

EXIT:  
    return eError;

}



OMX_ERRORTYPE VPP_GetPortDefFromBufHeader(OMX_BUFFERHEADERTYPE *pBufHeader, 
                                        OMX_PARAM_PORTDEFINITIONTYPE **portDef )
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    if ((pBufHeader->nOutputPortIndex != OMX_VPP_RGB_OUTPUT_PORT) && 
            (pBufHeader->nOutputPortIndex != OMX_VPP_YUV_OUTPUT_PORT) && 
            ((pBufHeader->nInputPortIndex == OMX_VPP_INPUT_PORT) ||
            (pBufHeader->nInputPortIndex == OMX_VPP_INPUT_OVERLAY_PORT ))){   /* input port */
        
        *portDef = pBufHeader->pInputPortPrivate;
        
    } 
    else if ((pBufHeader->nOutputPortIndex == OMX_VPP_RGB_OUTPUT_PORT) ||
            (pBufHeader->nOutputPortIndex == OMX_VPP_YUV_OUTPUT_PORT)){ /* output port */
        
        *portDef = pBufHeader->pOutputPortPrivate;
        
    }
    else {
        eError = OMX_ErrorBadParameter;
    }

    return eError;
}



OMX_ERRORTYPE VPP_Fill_LCMLInitParams(OMX_HANDLETYPE pComponent, OMX_U16 arr[], LCML_DSP *plcml_Init)
{
    
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nIpBuf,nIpBufSize,nOpBuf,nOpBufSize;
    char  valueStr[52]; /*Changed length*/                      
    OMX_U32 Input_FrameWidth;    
    OMX_U32 Output_FrameWidth;    
    OMX_U16 OutputRGB_Format;    
    OMX_U16 Input_FrameFormat;    
    OMX_U16 Output_FrameFormat;    
    OMX_U16 Overlay;        
    OMX_U16 Alpha = 0; /*Not implemented at OMX level*/
    OMX_U16 ParamSize = 0;
    char * pcSNArgs = NULL;
    OMX_U8 *pTemp = NULL;
    int index;
    VPP_COMPONENT_PRIVATE *pComponentPrivate = NULL; 
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;

    if (!pHandle) {
        eError=OMX_ErrorBadParameter;
        goto EXIT;
    }

    pComponentPrivate = (VPP_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    VPP_DPRINT("VPP::%d :: Entered Fill_LCMLInitParams\n",__LINE__); 

    pComponentPrivate->NumofOutputPort = 0;
    pComponentPrivate->IsYUVdataout    = 0;
    pComponentPrivate->IsRGBdataout    = 0;
    pComponentPrivate->IsOverlay       = 0; 

    nIpBuf = pComponentPrivate->sCompPorts[OMX_VPP_INPUT_PORT].pPortDef.nBufferCountMin;
    nIpBufSize = pComponentPrivate->sCompPorts[OMX_VPP_INPUT_PORT].pPortDef.nBufferSize;

    nOpBuf = OMX_VPP_MAX(pComponentPrivate->sCompPorts[OMX_VPP_RGB_OUTPUT_PORT].pPortDef.nBufferCountMin,
    pComponentPrivate->sCompPorts[OMX_VPP_YUV_OUTPUT_PORT].pPortDef.nBufferCountMin);
    nOpBufSize = OMX_VPP_MAX(pComponentPrivate->sCompPorts[OMX_VPP_RGB_OUTPUT_PORT].pPortDef.nBufferSize,
    pComponentPrivate->sCompPorts[OMX_VPP_YUV_OUTPUT_PORT].pPortDef.nBufferSize);

    plcml_Init->In_BufInfo.nBuffers      = nIpBuf;
    plcml_Init->In_BufInfo.nSize         = nIpBufSize;
    plcml_Init->In_BufInfo.DataTrMethod  = DMM_METHOD;
    plcml_Init->Out_BufInfo.nBuffers     = nOpBuf;
    plcml_Init->Out_BufInfo.nSize        = nOpBufSize;
    plcml_Init->Out_BufInfo.DataTrMethod = DMM_METHOD;

    plcml_Init->DeviceInfo.TypeofDevice       = 0;
    plcml_Init->DeviceInfo.DspStream          = NULL;
    plcml_Init->NodeInfo.nNumOfDLLs           = 3;
    plcml_Init->NodeInfo.AllUUIDs[0].uuid     = &VPPNODE_TI_UUID;
    strcpy ((char *)plcml_Init->NodeInfo.AllUUIDs[0].DllName, VPP_NODE_DLL);
    plcml_Init->NodeInfo.AllUUIDs[0].eDllType = DLL_NODEOBJECT;

    plcml_Init->NodeInfo.AllUUIDs[1].uuid     = &VPPNODE_TI_UUID;
    strcpy ((char *)plcml_Init->NodeInfo.AllUUIDs[1].DllName, VPP_NODE_DLL);
    plcml_Init->NodeInfo.AllUUIDs[1].eDllType = DLL_DEPENDENT;

    plcml_Init->NodeInfo.AllUUIDs[2].uuid     = (struct DSP_UUID *) &COMMON_TI_UUID;
    strcpy ((char *)plcml_Init->NodeInfo.AllUUIDs[2].DllName, USN_DLL_NAME);
    plcml_Init->NodeInfo.AllUUIDs[2].eDllType = DLL_DEPENDENT;
    
    plcml_Init->SegID     = 0;
    plcml_Init->Timeout   = -1;
    plcml_Init->Alignment = 0;
    plcml_Init->Priority = 5;
    VPP_DPRINT("priority is %d\n", plcml_Init->Priority);
    
    plcml_Init->ProfileID = 0; 
    /*Main input port */
    arr[0] = 5; /*# of Streams*/
    arr[1] = 0; /*Stream ID*/
    arr[2] = 0; /*Stream based input stream*/
    arr[3] = NUM_OF_VPP_BUFFERS; /*Number of buffers on input stream*/
    /*Overlay input port*/
    arr[4] = 1; /*Stream ID*/
    arr[5] = 0; /*Stream based input stream*/
    arr[6] = NUM_OF_VPP_BUFFERS; /*Number of buffers on input stream*/
    /*RGB output port*/
    arr[7] = 2; /*Stream ID*/
    arr[8] = 0; /*Stream basedoutput stream for RGB data*/
    arr[9] = NUM_OF_VPP_BUFFERS; /*Number of buffers on output stream*/
    /*YUV output port*/
    arr[10] = 3; /*Stream ID*/
    arr[11] = 0; /*Stream based output stream for YUV data*/
    arr[12] = NUM_OF_VPP_BUFFERS; /*Number of buffers on output stream*/
    /*Alpha input port, Not implemented at OMX level*/
    arr[13] = 4; /*Stream ID*/
    arr[14] = 0; /*Stream based input stream*/
    arr[15] = NUM_OF_VPP_BUFFERS; /*Number of buffers on output stream*/
    

    pcSNArgs = (char *) (arr + 16);

    Input_FrameWidth = pComponentPrivate->sCompPorts[OMX_VPP_INPUT_PORT].pPortDef.format.video.nFrameWidth;
    Output_FrameWidth = OMX_VPP_MAX(pComponentPrivate->sCompPorts[OMX_VPP_RGB_OUTPUT_PORT].pPortDef.format.video.nFrameWidth,
    pComponentPrivate->sCompPorts[OMX_VPP_YUV_OUTPUT_PORT].pPortDef.format.video.nFrameWidth);
    VPP_DPRINT("VPP:: INPPUT WIDTH=  in Fill_LCMLInitParams  %d\n ",Input_FrameWidth); 
    VPP_DPRINT("VPP:: OUTPUT WIDTH=  in Fill_LCMLInitParams  %d\n ",Output_FrameWidth);

    /* RGB type for output*/
    if (pComponentPrivate->sCompPorts[OMX_VPP_RGB_OUTPUT_PORT].pPortDef.bEnabled) {
        switch (pComponentPrivate->sCompPorts[OMX_VPP_RGB_OUTPUT_PORT].pPortDef.format.video.eColorFormat)
        {
        case OMX_COLOR_Format16bitRGB565:
            OutputRGB_Format = VGPOP_ERGB16_OUT;
            pComponentPrivate->NumofOutputPort++;
            pComponentPrivate->IsRGBdataout = 1;
            break;
            
        case OMX_COLOR_Format24bitRGB888:
            OutputRGB_Format = VGPOP_ERGB24_OUT;
            pComponentPrivate->NumofOutputPort++;
            pComponentPrivate->IsRGBdataout = 1;
            break;
            
        case OMX_COLOR_Format32bitARGB8888:
            OutputRGB_Format = VGPOP_ERGB32_OUT;
            pComponentPrivate->NumofOutputPort++;
            pComponentPrivate->IsRGBdataout = 1;
            break;
            
        case OMX_COLOR_Format12bitRGB444:
            OutputRGB_Format = VGPOP_ERGB12_OUT;
            pComponentPrivate->NumofOutputPort++;
            pComponentPrivate->IsRGBdataout = 1;
            break;
            
        case OMX_COLOR_Format8bitRGB332:
            OutputRGB_Format = VGPOP_ERGB8_OUT;
            pComponentPrivate->NumofOutputPort++;
            pComponentPrivate->IsRGBdataout = 1;
            break;
        case OMX_IndexCustomRGB4ColorFormat:
            OutputRGB_Format = VGPOP_ERGB4_OUT;
            pComponentPrivate->NumofOutputPort++;
            pComponentPrivate->IsRGBdataout = 1;
            break;
            
            
        case OMX_COLOR_FormatL8:
            OutputRGB_Format = VGPOP_EGRAY8_OUT;
            pComponentPrivate->NumofOutputPort++;
            pComponentPrivate->IsRGBdataout = 1;
            break;
            
        case OMX_COLOR_FormatL4:
            OutputRGB_Format = VGPOP_EGRAY4_OUT;
            pComponentPrivate->NumofOutputPort++;
            pComponentPrivate->IsRGBdataout = 1;
            break;
            
        case OMX_COLOR_FormatL2:
            OutputRGB_Format = VGPOP_EGRAY2_OUT;
            pComponentPrivate->NumofOutputPort++;
            pComponentPrivate->IsRGBdataout = 1;
            break;
            
        case OMX_COLOR_FormatMonochrome:
            OutputRGB_Format = VGPOP_EGRAY1_OUT;
            pComponentPrivate->NumofOutputPort++;
            pComponentPrivate->IsRGBdataout = 1;
            break;
            
        default:
            OutputRGB_Format = VGPOP_ERGB_NONE;
            pComponentPrivate->IsRGBdataout = 0;
            break;
        }
    }
    else {
        OutputRGB_Format = VGPOP_ERGB_NONE;
    }

    /* Input frame format*/
    switch (pComponentPrivate->sCompPorts[OMX_VPP_INPUT_PORT].pPortDef.format.video.eColorFormat)
    {
    case OMX_COLOR_FormatYUV420PackedPlanar:
        Input_FrameFormat = VGPOP_E420_IN;
        break;

    case OMX_COLOR_FormatCbYCrY:
        Input_FrameFormat = VGPOP_E422_IN_UY;
        break;
    case OMX_COLOR_FormatYCbYCr:
        Input_FrameFormat = VGPOP_E422_IN_YU;
        break;
        
        /*image formats*/
    case OMX_COLOR_Format16bitRGB565:
        Input_FrameFormat = VGPOP_ERGB16_IN;
        break;
    case OMX_COLOR_Format12bitRGB444:
        Input_FrameFormat = VGPOP_ERGB12_IN;
        break;
    case OMX_COLOR_Format8bitRGB332:
        Input_FrameFormat = VGPOP_ERGB8_IN;
        break;  
    case OMX_IndexCustomRGB4ColorFormat:
        Input_FrameFormat = VGPOP_ERGB4_IN;
        break;
    case OMX_COLOR_FormatL8:
        Input_FrameFormat = VGPOP_EGRAY8_IN;
        break;
    case OMX_COLOR_FormatL4:
        Input_FrameFormat = VGPOP_EGRAY4_IN;
        break;
    case OMX_COLOR_FormatL2:
        Input_FrameFormat = VGPOP_EGRAY2_IN;
        break;
    case OMX_COLOR_FormatMonochrome:
        Input_FrameFormat = VGPOP_EGRAY1_IN;
        break;
    case OMX_COLOR_Format24bitRGB888:
        Input_FrameFormat = VGPOP_ERGB24_IN;
        break;
    default:
        Input_FrameFormat = VGPOP_E420_IN;
        VPP_DPRINT("%d :: NOT SUPPORTED INPUT FORMAT setting default as 420 planar",__LINE__); 
        break;
    }

    /* Output YUV frame format*/
    if (pComponentPrivate->sCompPorts[OMX_VPP_YUV_OUTPUT_PORT].pPortDef.bEnabled) {
        switch (pComponentPrivate->sCompPorts[OMX_VPP_YUV_OUTPUT_PORT].pPortDef.format.video.eColorFormat)
        {
        case OMX_COLOR_FormatYUV420PackedPlanar:
            Output_FrameFormat = VGPOP_E420_OUT;
            pComponentPrivate->NumofOutputPort++;
            pComponentPrivate->IsYUVdataout = 1;
            break;
            
        case OMX_COLOR_FormatYCbYCr:
            Output_FrameFormat = VGPOP_E422_OUT_YU;
            pComponentPrivate->NumofOutputPort++;
            pComponentPrivate->IsYUVdataout = 1;
            break;
            
        case OMX_COLOR_FormatCbYCrY:
            Output_FrameFormat = VGPOP_E422_OUT_UY;
            pComponentPrivate->NumofOutputPort++;
            pComponentPrivate->IsYUVdataout = 1;
            break;

        default:
            Output_FrameFormat = VGPOP_EYUV_NONE;
            pComponentPrivate->IsYUVdataout=0;
            break;
        }
    }
    else {
        Output_FrameFormat = VGPOP_EYUV_NONE;
    }

    VPP_DPRINT(":: Ports Available in Fill_LCMLInitParams  %ld\n ",pComponentPrivate->NumofOutputPort); 

    /*for overlay*/
    if (pComponentPrivate->sCompPorts[OMX_VPP_INPUT_OVERLAY_PORT].pPortDef.bEnabled) {
        Overlay = 1;
        pComponentPrivate->IsOverlay = 1 ;
        VPP_DPRINT("VPP::OVERLAY ENABLED"); 
    }
    else {
        Overlay = 0;
    }

    memset(valueStr, 0, sizeof(valueStr));
    VPP_DPRINT(":%lu:%lu:%u:%u:%u:%d:%d\n",
    Input_FrameWidth,
    Output_FrameWidth,
    OutputRGB_Format,
    Input_FrameFormat,
    Output_FrameFormat,
    Overlay,
    Alpha);
    sprintf(valueStr, ":%lu:%lu:%u:%u:%u:%d:%d\n",
    Input_FrameWidth,
    Output_FrameWidth,
    OutputRGB_Format,
    Input_FrameFormat,
    Output_FrameFormat,
    Overlay,
    Alpha);

    while(valueStr[ParamSize] != '\0'){
        ParamSize++;
    }
    VPP_DPRINT("ParamSize is %d\n", ParamSize);

    /*Copy VPP parameters */
    pTemp = memcpy(pcSNArgs,valueStr,ParamSize);
    if(pTemp == NULL){
        eError = OMX_ErrorUndefined;
        goto EXIT;
    }

    if ( (ParamSize % 2) != 0) {
        index =(ParamSize+1) >> 1;
    }
    else {
        index = ParamSize >> 1;  /*Divide by 2*/
    }
    index = index + 16;  /*Add 16 to the index in order to point to the correct location*/

    arr[index] = END_OF_CR_PHASE_ARGS;
    plcml_Init->pCrPhArgs = arr;

    VPP_DPRINT("VPP::%d :: Exiting Fill_LCMLInitParams",__LINE__); 

EXIT:
    return eError;
}



/* ========================================================================== */
/**
* @Start_ComponentThread() This function is called by the component to create 
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
/* ==========================================================================* */
OMX_ERRORTYPE VPP_Start_ComponentThread(OMX_HANDLETYPE pComponent)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent; 
    VPP_COMPONENT_PRIVATE *pComponentPrivate = NULL;
#ifdef UNDER_CE
    pthread_attr_t attr;
    memset(&attr, 0, sizeof(attr));
    attr.__inheritsched = PTHREAD_EXPLICIT_SCHED;
    attr.__schedparam.__sched_priority = OMX_VGPOP_THREAD_PRIORITY;
#endif
    
    VPP_DPRINT("VPP::%d :: Enetering  Start_ComponentThread\n", __LINE__);
   
    pComponentPrivate = (VPP_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    /* create the pipe used to send commands to the thread */
    eError = pipe (pComponentPrivate->cmdPipe);
    if (eError) {
        eError = OMX_ErrorContentPipeCreationFailed;
        goto EXIT;
    }

    /* create the pipe used to send commands data to the thread */
    eError = pipe (pComponentPrivate->nCmdDataPipe);
    if (eError) {
        eError = OMX_ErrorContentPipeCreationFailed;
        goto EXIT;
    }

    /*Create pipe to hold filled input buffers from APP to Component*/
    eError = pipe(pComponentPrivate->nFilled_iPipe);
    if (eError) {
        eError = OMX_ErrorContentPipeCreationFailed;
        goto EXIT;
    }
    /*Create pipe to hold empty output buffers from APP to Component*/
    eError = pipe(pComponentPrivate->nFree_oPipe);
    if (eError) {
        eError = OMX_ErrorContentPipeCreationFailed;
        goto EXIT;
    }
    
#ifdef UNDER_CE
    eError = pthread_create (&(pComponentPrivate->ComponentThread), 
                            &attr,
                            VPP_ComponentThreadFunc, 
                            pComponentPrivate);
#else
    eError = pthread_create (&(pComponentPrivate->ComponentThread),
                                NULL,
                                VPP_ComponentThreadFunc, 
                                pComponentPrivate);
#endif
    if (eError || !pComponentPrivate->ComponentThread) {
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

#ifdef __PERF_INSTRUMENTATION__
    PERF_ThreadCreated(pComponentPrivate->pPERF,
                        pComponentPrivate->ComponentThread,
                        PERF_FOURCC('V','P','P','T'));
#endif

    VPP_DPRINT ("VPP::%d :: Exiting from Start_ComponentThread\n", __LINE__);

EXIT:
    return eError;   
}


/* ========================================================================== */
/**
* Free_ComponentResources() This function is called by the component during
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

OMX_ERRORTYPE VPP_Free_ComponentResources(OMX_HANDLETYPE pComponent)
{
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    VPP_COMPONENT_PRIVATE *pComponentPrivate = (VPP_COMPONENT_PRIVATE *) pHandle->pComponentPrivate;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_ERRORTYPE threadError = OMX_ErrorNone;
    OMX_ERRORTYPE err = OMX_ErrorNone;
    OMX_HANDLETYPE pLcmlHandle = pComponentPrivate->pLcmlHandle;
    OMX_COMMANDTYPE stop = EXIT_COMPONENT_THRD;
    int i=0;

#ifdef __PERF_INSTRUMENTATION__
    PERF_Boundary(pComponentPrivate->pPERF,
        PERF_BoundaryStart | PERF_BoundaryCleanup);
    PERF_SendingCommand(pComponentPrivate->pPERF, stop, 0, PERF_ModuleComponent);
#endif

    if (pLcmlHandle !=NULL) {
        VPP_DPRINT (" IN ComponentDeInit calling EMMCodecControlDestroy  \n");
        eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle, EMMCodecControlDestroy, NULL);
        if (eError != OMX_ErrorNone) {
            VPP_DPRINT("%d : Error: in Destroying the codec\n",__LINE__);
            goto EXIT;
        }
    }
    err = write (pComponentPrivate->cmdPipe[1], &stop, sizeof(OMX_COMMANDTYPE));
    if (err == -1) {
        VPP_DPRINT ("%d :: Error in Writing to the cmd  pipe  In deinit\n", eError);
        eError = OMX_ErrorHardware;
        goto EXIT;
    }
        
    VPP_DPRINT ("%d :: Free_ComponentResources \n",__LINE__);
    if(pComponentPrivate->ComponentThread){
#ifndef UNDER_CE
        err = pthread_join (pComponentPrivate->ComponentThread, 
                                    (void*)&threadError);
#else
        err = oaf_pthread_join (pComponentPrivate->ComponentThread, 
                                        (void*)&threadError);
#endif
        if (err) {
            eError = OMX_ErrorHardware;
            VPP_DPRINT ("VPP::%d :: Error while closing Component Thread\n",__LINE__);
        }
    }
    else{
        eError = OMX_ErrorUndefined;
            VPP_DPRINT ("VPP::%d :: Error Component Thread = NULL\n",__LINE__);
    }
    for (i=0; i<2; i++) {
        err = close (pComponentPrivate->cmdPipe[i]);
        if (err && OMX_ErrorNone == eError) {
            eError = OMX_ErrorHardware;
            VPP_DPRINT ("VPP::%d :: Error while closing cmdPipe\n",__LINE__);
        }
        
        err = close (pComponentPrivate->nCmdDataPipe[i]);
        if (err && OMX_ErrorNone == eError) {
            eError = OMX_ErrorHardware;
            VPP_DPRINT ("VPP::%d :: Error while closing Command Data Pipe\n",__LINE__);
        }

        /*close the data pipe handles*/
        err = close(pComponentPrivate->nFree_oPipe[i]);
        if (err && OMX_ErrorNone == eError) {
            eError = OMX_ErrorHardware;
            VPP_DPRINT ("VPP::%d :: Error while closing Free Output pipe\n",__LINE__);
        }
        err = close(pComponentPrivate->nFilled_iPipe[i]);
        if (err && OMX_ErrorNone == eError) {
            eError = OMX_ErrorHardware;
            VPP_DPRINT ("VPP::%d :: Error while closing Filled Input pipe\n",__LINE__);
        }
    }

    pthread_mutex_destroy(&pComponentPrivate->vpp_mutex);
    pthread_cond_destroy(&pComponentPrivate->stop_cond);
    pthread_mutex_destroy(&pComponentPrivate->buf_mutex);

#ifdef __PERF_INSTRUMENTATION__
    PERF_Boundary(pComponentPrivate->pPERF,
        PERF_BoundaryComplete | PERF_BoundaryCleanup);
        PERF_Done(pComponentPrivate->pPERF);
#endif

EXIT:
    /* LinkedList_DisplayAll(&AllocList); */
    OMX_FREEALL();
    LinkedList_Destroy(&AllocList);
    
    VPP_DPRINT ("Exiting Successfully After Freeing All Resources\n");
    return eError; 
}

/* ========================================================================== */
/**
* @VPP_DisablePort() This function is called by the component when ever it
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
OMX_ERRORTYPE VPP_DisablePort (VPP_COMPONENT_PRIVATE* pComponentPrivate, OMX_U32 nParam1)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE* pHandle = NULL;
    
    if (!pComponentPrivate) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pHandle = (OMX_COMPONENTTYPE*)pComponentPrivate->pHandle;
    
   
   if (pComponentPrivate->curState == OMX_StateExecuting || pComponentPrivate->curState == OMX_StatePause) {
       if (((nParam1 >= 0) && (nParam1 < 4)) || (nParam1 == -1)) {
           eError = VPP_HandleCommandFlush(pComponentPrivate, nParam1, OMX_FALSE);
       } 
   }
   
EXIT:
    return eError;
}



/* ========================================================================== */
/**
* @VPP_EnablePort() This function is called by the component when ever it
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
OMX_ERRORTYPE VPP_EnablePort (VPP_COMPONENT_PRIVATE* pComponentPrivate, OMX_U32 nParam1)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE* pHandle = NULL;
    int ports;
    OMX_U32 nTimeout;

    VPP_DPRINT("VPP: Enable port index=%ld",nParam1);

    if (!pComponentPrivate) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pHandle = (OMX_COMPONENTTYPE*)pComponentPrivate->pHandle;
    
    if (nParam1 >= 0 && nParam1 < NUM_OF_VPP_PORTS ){
        /* enable port*/
        pComponentPrivate->sCompPorts[nParam1].pPortDef.bEnabled = OMX_TRUE;
    }
    
    if ( nParam1 == -1) {
        for (ports = 0; ports < NUM_OF_VPP_PORTS; ports++)
        {
            pComponentPrivate->sCompPorts[ports].pPortDef.bEnabled = OMX_TRUE;
        }
    }
    
    nTimeout = 0;
    while (OMX_TRUE)
    {
        if ((nParam1 >= 0 && nParam1 < NUM_OF_VPP_PORTS) && 
                (pComponentPrivate->curState == OMX_StateLoaded || 
                    pComponentPrivate->sCompPorts[nParam1].pPortDef.bPopulated)) {
            pComponentPrivate->cbInfo.EventHandler (pHandle, 
                                                    pHandle->pApplicationPrivate,
                                                    OMX_EventCmdComplete,
                                                    OMX_CommandPortEnable, 
                                                    nParam1,
                                                    NULL);
            break;
        }
        else if (nParam1 == -1 && 
                (pComponentPrivate->curState == OMX_StateLoaded || 
                    (pComponentPrivate->sCompPorts[0].pPortDef.bPopulated && 
                        pComponentPrivate->sCompPorts[1].pPortDef.bPopulated && 
                        pComponentPrivate->sCompPorts[2].pPortDef.bPopulated && 
                        pComponentPrivate->sCompPorts[3].pPortDef.bPopulated))) {
            for (ports = 0; ports < NUM_OF_VPP_PORTS; ports++) {  
                pComponentPrivate->cbInfo.EventHandler (pHandle, 
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,
                                                        OMX_CommandPortEnable, 
                                                        ports,
                                                        NULL);
            }
            break;
        }
        else if (nTimeout++ > 0xEFFFFFFE) {
            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle, 
                                            pComponentPrivate->pHandle->pApplicationPrivate,
                                            OMX_EventError, 
                                            OMX_ErrorInsufficientResources, 
                                            OMX_TI_ErrorMajor, 
                                            "Port Unresponsive - Idle");
            break;
        }
        
        sched_yield();
    }
    
EXIT:
    return eError;
}

/* ========================================================================== */
/**
* @VPP_EnablePort() This function is called by the component when ever it
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
OMX_ERRORTYPE VPP_HandleCommandFlush (VPP_COMPONENT_PRIVATE* pComponentPrivate, OMX_U32 nParam1, OMX_BOOL return_event)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE* pHandle = NULL;
    LCML_DSP_INTERFACE *pLcmlHandle = NULL;
    OMX_U32 nCount = 0;
    char    *pArgs      = "damedesuStr";


    OMX_BUFFERHEADERTYPE * pBufHeader;
    OMX_PARAM_PORTDEFINITIONTYPE *portDef ;

    int                    nRet;
    int                    i;
    OMX_BOOL               bFoundBuffer;

    if (!pComponentPrivate) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pHandle = (OMX_COMPONENTTYPE*)pComponentPrivate->pHandle;
 
    VPP_DPRINT("nParam1 %d return_event is %x OMX_FALSE %x\n", nParam1, return_event, OMX_FALSE);

    pLcmlHandle = pComponentPrivate->pLcmlHandle;
    pComponentPrivate->bDisable = OMX_FALSE;
    VPP_DPRINT("VPP_UTILS: send STOP as flush\n");
    eError = LCML_ControlCodec(
                            ((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                            MMCodecControlStop,
                            (void *)pArgs);
    if (eError != OMX_ErrorNone) {
        VPP_DPRINT("VPP::%d: Error 0x%X Occurred in Codec Stop..\n",__LINE__,eError);
        goto EXIT;
    }

    while (pComponentPrivate->bDisable == OMX_FALSE) {
        sched_yield();
    }

    eError = LCML_ControlCodec(
                            ((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                            EMMCodecControlStart,
                            (void *)pArgs);
    if (eError != OMX_ErrorNone) {
        VPP_DPRINT("VPP::%d: Error 0x%X Occurred in Codec Start..\n",__LINE__,eError);
        goto EXIT;
    }

    for (i = 0; i < NUM_OF_VPP_PORTS; i ++) {
        if (return_event == OMX_TRUE) {
            for (nCount = 0; nCount < NUM_OF_VPP_BUFFERS; nCount ++){
                if (pComponentPrivate->sCompPorts[i].pVPPBufHeader[nCount].eBufferOwner  == VPP_BUFFER_DSP ||
                        pComponentPrivate->sCompPorts[i].pVPPBufHeader[nCount].eBufferOwner  == VPP_BUFFER_COMPONENT_IN ||
                        pComponentPrivate->sCompPorts[i].pVPPBufHeader[nCount].eBufferOwner  == VPP_BUFFER_COMPONENT_OUT){

                    switch (pComponentPrivate->sCompPorts[i].pVPPBufHeader[nCount].eBufferOwner) {
                    case VPP_BUFFER_DSP:
                        pComponentPrivate->sCompPorts[i].pVPPBufHeader[nCount].eBufferOwner = VPP_BUFFER_CLIENT;
                        pComponentPrivate->sCompPorts[i].pVPPBufHeader[nCount].pBufHeader->nFilledLen = 0;
                        /* pComponentPrivate->nInPortOut ++; */
#ifdef __PERF_INSTRUMENTATION__
                        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                          PREF(((OMX_BUFFERHEADERTYPE*) pComponentPrivate->sCompPorts[0].pVPPBufHeader[nCount].pBufHeader), pBuffer),
                                          PREF(((OMX_BUFFERHEADERTYPE*) pComponentPrivate->sCompPorts[0].pVPPBufHeader[nCount].pBufHeader), nFilledLen),
                                          PERF_ModuleHLMM);
#endif
                        if (i == OMX_VPP_INPUT_PORT || 
                            i == OMX_VPP_INPUT_OVERLAY_PORT) {
                            pComponentPrivate->cbInfo.EmptyBufferDone(pComponentPrivate->pHandle,
                                                pComponentPrivate->pHandle->pApplicationPrivate,
                                                pComponentPrivate->sCompPorts[i].pVPPBufHeader[nCount].pBufHeader);
                        } else if (i == OMX_VPP_RGB_OUTPUT_PORT ||
                            i == OMX_VPP_YUV_OUTPUT_PORT) {
                            pComponentPrivate->cbInfo.FillBufferDone(pComponentPrivate->pHandle,
                                                pComponentPrivate->pHandle->pApplicationPrivate,
                                                pComponentPrivate->sCompPorts[i].pVPPBufHeader[nCount].pBufHeader);
                        }
                        break;
                    case VPP_BUFFER_COMPONENT_IN:
                        bFoundBuffer = OMX_FALSE;

                        while (bFoundBuffer == OMX_FALSE) {
                            if (i == OMX_VPP_INPUT_PORT || i == OMX_VPP_INPUT_OVERLAY_PORT) {
                                nRet = read(pComponentPrivate->nFilled_iPipe[0], &(pBufHeader),sizeof(pBufHeader));
                                if (-1 == nRet) {
                                    VPP_DPRINT ("%d :: Error while reading from the pipe\n",__LINE__);
                                }
                                eError = VPP_GetPortDefFromBufHeader(pBufHeader, &portDef);

                                if (eError != OMX_ErrorNone) {
                                    VPP_DPRINT("VPP:: Got error in _GetPortDefFromBufHeader. Code %x\n", eError);
                                    goto EXIT;
                                }

                                if (portDef->nPortIndex == i) {
                                    pComponentPrivate->sCompPorts[i].pVPPBufHeader[nCount].eBufferOwner = VPP_BUFFER_CLIENT;
                                    pComponentPrivate->cbInfo.EmptyBufferDone(pComponentPrivate->pHandle,
                                                    pComponentPrivate->pHandle->pApplicationPrivate,
                                                    pBufHeader);
                                    bFoundBuffer = OMX_TRUE;
                                }
                                else {
                                    write(pComponentPrivate->nFilled_iPipe[1], &(pBufHeader), sizeof(pBufHeader));
                                }
                            }
                            else if (i == OMX_VPP_RGB_OUTPUT_PORT ||i == OMX_VPP_YUV_OUTPUT_PORT) {
                                nRet = read(pComponentPrivate->nFree_oPipe[0], &pBufHeader, sizeof(pBufHeader));
                                if (-1 == nRet) {
                                    VPP_DPRINT ("%d :: Error while reading from the pipe\n",__LINE__);
                                }
                                eError = VPP_GetPortDefFromBufHeader(pBufHeader, &portDef);
                                if (eError != OMX_ErrorNone) {
                                    VPP_DPRINT("Error in _GetPortDefFromBufHeader. Code %d\n", eError);
                                    goto EXIT;
                                }
                                if (portDef->nPortIndex == i) {
                                    pComponentPrivate->sCompPorts[i].pVPPBufHeader[nCount].eBufferOwner = VPP_BUFFER_CLIENT;
                                    pComponentPrivate->cbInfo.FillBufferDone(pHandle,
                                                    pHandle->pApplicationPrivate,
                                                    pBufHeader);
                                    bFoundBuffer = OMX_TRUE;
                                } 
                                else {
                                    write(pComponentPrivate->nFree_oPipe[1],&pBufHeader,sizeof(OMX_BUFFERHEADERTYPE*));
                                }
                            }
                        } /* end of while () */
                        break;
                    case VPP_BUFFER_COMPONENT_OUT:
                        /* since we don't have this queue, there is nothing 
                        to flush.  Buffers are handled immediately */
                        break;
                    case VPP_BUFFER_CLIENT:
                    case VPP_BUFFER_TUNNEL_COMPONENT:
                        break;
                    }
                }
            }
            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                pComponentPrivate->pHandle->pApplicationPrivate, 
                                                OMX_EventCmdComplete,
                                                OMX_CommandFlush,
                                                i, 
                                                NULL);
        }
    } /* for (i = 0; i < NUM_OF_VPP_PORTS; i ++) */

EXIT:
    return eError;
}



/* ========================================================================== */
/**
* @StateToIdle() This function is called by the component when ever it
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
OMX_ERRORTYPE VPP_StateToIdle(VPP_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE                eError      = OMX_ErrorNone;
    OMX_COMPONENTTYPE            *pHandle    = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;
    OMX_HANDLETYPE               pLcmlHandle = pComponentPrivate->pLcmlHandle;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef   = NULL;
    VPP_PORT_TYPE                *pPortTp    = NULL;
    OMX_U8                  *pBufferAligned = NULL;
    OMX_U8                  *pBufferStart = NULL;
    char                         *pArgs      = "damedesuStr";
    OMX_U32                      nTimeout;
    OMX_U16                      array[100];  /*Used to pass to Fill_LCMLInitParams*/

    VPP_DPRINT("VPP::%d: HandleCommand: Cmd Idle \n",__LINE__);
    VPP_DPRINT("Current state is %d = %d\n", pComponentPrivate->curState, OMX_StateLoaded);

    if (pComponentPrivate->curState == OMX_StateInvalid) {
        pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                            pComponentPrivate->pHandle->pApplicationPrivate, 
                            OMX_EventError, 
                            OMX_ErrorIncorrectStateTransition, 
                            OMX_TI_ErrorSevere, 
                            NULL);
        goto EXIT;
    }

    pComponentPrivate->toState = OMX_StateIdle;
    
    if ((pComponentPrivate->curState == OMX_StateLoaded) ||
        (pComponentPrivate->curState == OMX_StateWaitForResources)) { /* from Loaded to Idle */
        
        LCML_CALLBACKTYPE cb;
        LCML_DSP *pLcmlDsp;
        char *p = "damedesuStr"; 
        int nPortIndex = 0;

#ifdef __PERF_INSTRUMENTATION__
        PERF_Boundary(pComponentPrivate->pPERFcomp,
                    PERF_BoundaryStart | PERF_BoundarySetup);
#endif
        
        VPP_DPRINT("pComponentPrivate->sCompPorts[0].pPortDef.nPortIndex = %d\n", pComponentPrivate->sCompPorts[0].pPortDef.nPortIndex);

        pLcmlHandle = (OMX_HANDLETYPE) VPP_GetLCMLHandle(pComponentPrivate);
        if (pLcmlHandle == NULL) {
            VPP_DPRINT("%d :: LCML Handle is NULL........exiting..\n",__LINE__);
            goto EXIT;
        }
        VPP_DPRINT("%d pComponentPrivate->sCompPorts[0].pPortDef.nPortIndex = %d\n", __LINE__, pComponentPrivate->sCompPorts[0].pPortDef.nPortIndex);
       
        pLcmlDsp = (((LCML_DSP_INTERFACE*)pLcmlHandle)->dspCodec);
        VPP_DPRINT("VPP::%d: before init LCML \n",__LINE__);
        
        for (nPortIndex = 0; nPortIndex < NUM_OF_VPP_PORTS; nPortIndex++) {
            OMX_U32 nBuf;
            pPortTp = &(pComponentPrivate->sCompPorts[nPortIndex]);
            pPortDef = &(pComponentPrivate->sCompPorts[nPortIndex].pPortDef);
            if ((pPortTp->hTunnelComponent != NULL ) && 
                    ((pPortTp->eSupplierSetting == OMX_BufferSupplyInput && 2 > nPortIndex) || 
                        (pPortTp->eSupplierSetting == OMX_BufferSupplyOutput && 2 < nPortIndex))) {
                
                /* assuming i am the supplier */
                for (nBuf=0; nBuf< pPortDef->nBufferCountActual; nBuf++) {
                    OMX_U32 nsize; 
                    OMX_U8 *nbuffer = NULL; 

                    nsize = pPortDef->format.video.nFrameWidth * pPortDef->format.video.nFrameHeight * 2;
                    OMX_MALLOC(pBufferStart, nsize + 32 + 256);
                    VPP_DPRINT("allocated pBufferStart with address %p\n", nbuffer);

                    pBufferAligned = pBufferStart;
                    while ((((int)pBufferAligned) & 0x1f) != 0)
                    {
                        pBufferAligned++;
                    }
                    pBufferAligned            = ((OMX_U8*)pBufferAligned)+128;
                    pComponentPrivate->sCompPorts[nPortIndex].pVPPBufHeader[nBuf].pBufferStart = pBufferStart;
                    nbuffer            = pBufferAligned;

#ifdef __PERF_INSTRUMENTATION__
                    PERF_XferingFrame(pComponentPrivate->pPERFcomp,
                    nbuffer, nsize,
                    PERF_ModuleMemory,
                    PERF_ModuleLLMM);
#endif

                    eError = OMX_UseBuffer(
                                        pPortTp->hTunnelComponent,
                                        &(pPortTp->pVPPBufHeader[nBuf].pBufHeader),
                                        pPortTp->nTunnelPort,
                                        NULL,
                                        nsize,
                                        nbuffer);

                    if (pPortTp->eSupplierSetting == OMX_BufferSupplyInput) {           
                        pComponentPrivate->sCompPorts[nPortIndex].pVPPBufHeader[nBuf].pBufHeader->nFilledLen = nsize;
                        pComponentPrivate->sCompPorts[nPortIndex].pVPPBufHeader[nBuf].pBufHeader->nAllocLen = nsize;
                        pComponentPrivate->sCompPorts[nPortIndex].pVPPBufHeader[nBuf].nIndex = OMX_VPP_INPUT_PORT;
                        pComponentPrivate->sCompPorts[nPortIndex].pVPPBufHeader[nBuf].bSelfAllocated = OMX_TRUE;
                        pComponentPrivate->sCompPorts[nPortIndex].pVPPBufHeader[nBuf].bHolding = OMX_TRUE;
                        pComponentPrivate->sCompPorts[nPortIndex].nBufSupplier = OMX_TRUE;
                        pComponentPrivate->sCompPorts[nPortIndex].pVPPBufHeader[nBuf].pBufHeader->pInputPortPrivate = &pComponentPrivate->sCompPorts[OMX_VPP_INPUT_PORT].pPortDef;
                        pComponentPrivate->sCompPorts[nPortIndex].pPortDef.bPopulated = OMX_TRUE;
                        pComponentPrivate->sCompPorts[nPortIndex].nBufferCount ++;
                    }
                    else { 
                        pComponentPrivate->sCompPorts[nPortIndex].pVPPBufHeader[nBuf].pBufHeader->nFilledLen = nsize;
                        pComponentPrivate->sCompPorts[nPortIndex].pVPPBufHeader[nBuf].pBufHeader->nAllocLen = nsize;
                        pComponentPrivate->sCompPorts[nPortIndex].pVPPBufHeader[nBuf].nIndex = nPortIndex;
                        pComponentPrivate->sCompPorts[nPortIndex].pVPPBufHeader[nBuf].bSelfAllocated = OMX_TRUE;
                        pComponentPrivate->sCompPorts[nPortIndex].pVPPBufHeader[nBuf].bHolding = OMX_TRUE;
                        pComponentPrivate->sCompPorts[nPortIndex].nBufSupplier = OMX_TRUE;
                        pComponentPrivate->sCompPorts[nPortIndex].pVPPBufHeader[nBuf].pBufHeader->pOutputPortPrivate = &pComponentPrivate->sCompPorts[nPortIndex].pPortDef;
                        pComponentPrivate->sCompPorts[nPortIndex].pPortDef.bPopulated = OMX_TRUE;
                        pComponentPrivate->sCompPorts[nPortIndex].nBufferCount ++;
                    }
                }
                VPP_InitBufferDataPropagation(pComponentPrivate, nPortIndex);                                
            } /* end if I am a supplier */
        
            if (pPortDef->bEnabled == OMX_TRUE) {
                nTimeout = 0;

                while(1) 
                {
                    if(pPortDef->bPopulated) {
                        break;
                    }
                    else if (nTimeout ++ > 0xEFFFFFFE) {
                        VPP_DPRINT("TimeOut Error ! .. Buffers not allocated in time.\n");
                        pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle, 
                                            pComponentPrivate->pHandle->pApplicationPrivate,
                                            OMX_EventError, 
                                            OMX_ErrorPortUnresponsiveDuringDeallocation, 
                                            OMX_TI_ErrorSevere, 
                                            "Port Unresponsive - Idle");      
                        break;
                    }

                    sched_yield();
                }
            }
        } /* end of for loop */
        VPP_DPRINT("%d pComponentPrivate->sCompPorts[0].pPortDef.nPortIndex = %d\n", __LINE__, pComponentPrivate->sCompPorts[0].pPortDef.nPortIndex);
       
        eError = VPP_Fill_LCMLInitParams(pHandle,array, pLcmlDsp);
        VPP_DPRINT("%d pComponentPrivate->sCompPorts[0].pPortDef.nPortIndex = %d\n", __LINE__, pComponentPrivate->sCompPorts[0].pPortDef.nPortIndex);
        
        if (eError != OMX_ErrorNone) {
            VPP_DPRINT("VPP::%d :: Error 0x%X returned from Fill_LCMLInitParams()\n",__LINE__,eError);
            goto EXIT;
        }
        
        pComponentPrivate->pLcmlHandle = (LCML_DSP_INTERFACE *)pLcmlHandle;
        cb.LCML_Callback = (void *) VPP_LCML_Callback;

#ifdef __PERF_INSTRUMENTATION__
        pComponentPrivate->lcml_nCntIp = 0;
        pComponentPrivate->lcml_nCntOpReceived = 0;
#endif

        eError = LCML_InitMMCodec(((LCML_DSP_INTERFACE *)pLcmlHandle)->pCodecinterfacehandle,
                                    p,
                                    &pLcmlHandle,
                                    (void *)p,
                                    &cb);
       if (eError != OMX_ErrorNone) {
            VPP_DPRINT("%d :: Error 0x%X : InitMMCodec failed...>>>>>> \n",__LINE__,eError);
            goto EXIT;
        }
        VPP_DPRINT("%d pComponentPrivate->sCompPorts[0].pPortDef.nPortIndex = %d\n", __LINE__, pComponentPrivate->sCompPorts[0].pPortDef.nPortIndex);
       
#ifdef LCML_USE_HASH
#ifdef VPP_USE_HASH
        /* Enable Hashing for this component */
        VPP_DPRINT("enable hashing\n");
        LCML_SetHashingState(((LCML_DSP_INTERFACE *)pLcmlHandle)->pCodecinterfacehandle, OMX_TRUE);
#endif
#endif


#ifdef RESOURCE_MANAGER_ENABLED /* Resource Manager Proxy Calls */
            pComponentPrivate->rmproxyCallback.RMPROXY_Callback = (void *)ResourceManagerCallback;
            if (pComponentPrivate->curState != OMX_StateWaitForResources) {

                eError = RMProxy_NewSendCommand(pHandle, RMProxy_RequestResource, OMX_VPP_COMPONENT, 50, 3456, &(pComponentPrivate->rmproxyCallback));/*50Mhz*/
                if (eError != OMX_ErrorNone) {
                    /* resource is not available, need set state to OMX_StateWaitForResources*/
                    VPP_DPRINT("Resource is not available\n");

                    pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventError,
                                                           OMX_ErrorInsufficientResources,
                                                           OMX_TI_ErrorSevere,
                                                           NULL);
                    eError = OMX_ErrorNone;
                    goto EXIT;
                }
            }
#endif

#ifdef __PERF_INSTRUMENTATION__
            PERF_Boundary(pComponentPrivate->pPERFcomp,
                        PERF_BoundaryComplete | PERF_BoundarySetup);
#endif
    
        VPP_DPRINT("%d pComponentPrivate->sCompPorts[0].pPortDef.nPortIndex = %d\n", __LINE__, pComponentPrivate->sCompPorts[0].pPortDef.nPortIndex);

        pComponentPrivate->curState = OMX_StateIdle;
            
#ifdef RESOURCE_MANAGER_ENABLED
            eError = RMProxy_NewSendCommand(pHandle, RMProxy_StateSet, OMX_VPP_COMPONENT, OMX_StateIdle, 3456, NULL);
            if (eError != OMX_ErrorNone) {
                VPP_DPRINT("Resources not available Loaded ->Idle\n");

                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventError,
                                                       OMX_ErrorInsufficientResources,
                                                       OMX_TI_ErrorSevere,
                                                       NULL);
                goto EXIT;
            }

#endif         

        pComponentPrivate->cbInfo.EventHandler(
                                pHandle, 
                                pHandle->pApplicationPrivate,
                                OMX_EventCmdComplete, 
                                OMX_CommandStateSet, 
                                pComponentPrivate->curState, 
                                NULL);

        VPP_DPRINT("VPP::%d :: VPP: State has been Set to Idle\n",__LINE__);
        
    } 
    else if (pComponentPrivate->curState == OMX_StateExecuting ||  
            pComponentPrivate->curState == OMX_StatePause ) {
        int nIndex = 0;
        OMX_U32 nCount = 0;
 
        int nFilledInBuf = 0;
        int nFreeInBuf = 0;
        int nFilledOutBuf = 0;
        int nFreeOutBuf = 0;
        int kk;

        pComponentPrivate->bIsStopping = OMX_TRUE;
        pComponentPrivate->toState = OMX_StateIdle;
#ifdef LCML_USE_HASH
        /* clear out any mappings that might have accumulated */
        eError = LCML_FlushHashes(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle);
        if (eError != OMX_ErrorNone) {
            VPP_DPRINT("Error occurred in Codec mapping flush!\n");
            goto EXIT;
        }
#endif
        VPP_DPRINT("%d :: In HandleCommand: Stopping the codec\n",__LINE__);
        
#ifdef __PERF_INSTRUMENTATION__
        PERF_Boundary(pComponentPrivate->pPERFcomp,
                    PERF_BoundaryComplete | PERF_BoundarySteadyState);
        /* PERF_SendingCommand(pComponentPrivate->pPERFcomp,
                            MMCodecControlStop,
                            (OMX_U32) pArgs,
                            PERF_ModuleCommonLayer); */
#endif
        eError = LCML_ControlCodec(
        ((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
            MMCodecControlStop,
            (void *)pArgs);
        if (eError != OMX_ErrorNone) {
            VPP_DPRINT("VPP::%d: Error 0x%X Occurred in Codec Stop..\n",__LINE__,eError);
            goto EXIT;
        }

        pthread_mutex_lock(&pComponentPrivate->vpp_mutex);
        while ((pComponentPrivate->ExeToIdleFlag & VPP_DSPSTOP) == 0) {
            pthread_cond_wait(&pComponentPrivate->stop_cond, &pComponentPrivate->vpp_mutex);
        }
        pthread_mutex_unlock(&pComponentPrivate->vpp_mutex);

        VPP_DPRINT("VPP_Utils.c: get STOP back from DSP\n");
        for( nIndex = 0; nIndex < NUM_OF_VPP_PORTS; nIndex++) {
            VPP_DPRINT("port %d is %d (%p)\n", nIndex, pComponentPrivate->sCompPorts[nIndex].pPortDef.bEnabled,pComponentPrivate->sCompPorts[nIndex].hTunnelComponent);

            /*if (!(pComponentPrivate->sCompPorts[nIndex].pPortDef.bEnabled == OMX_TRUE)) {
                continue;
            }*/
            if (pComponentPrivate->sCompPorts[nIndex].hTunnelComponent != NULL) {
                for (nCount = 0; nCount < pComponentPrivate->sCompPorts[nIndex].nBufferCount; nCount++) {
                    if (!(pComponentPrivate->sCompPorts[nIndex].pVPPBufHeader[nCount].bSelfAllocated == OMX_TRUE)) {
                        VPP_DPRINT("VPP return buf to tunneled: %d %d\n", 
                        pComponentPrivate->sCompPorts[nIndex].pVPPBufHeader[nCount].pBufHeader->nFlags,
                        pComponentPrivate->sCompPorts[nIndex].pVPPBufHeader[nCount].pBufHeader->nFilledLen);
                        pComponentPrivate->sCompPorts[nIndex].pVPPBufHeader[nCount].pBufHeader->nFlags = OMX_BUFFERFLAG_EOS;
                        pComponentPrivate->sCompPorts[nIndex].pVPPBufHeader[nCount].pBufHeader->nFilledLen = 0;
                        if (pComponentPrivate->sCompPorts[nIndex].pPortDef.eDir == OMX_DirOutput) {
                            VPP_DPRINT("VPP is at output port\n");

#ifdef __PERF_INSTRUMENTATION__
                            PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                pComponentPrivate->sCompPorts[nIndex].pVPPBufHeader[nCount].pBufHeader->pBuffer,
                                pComponentPrivate->sCompPorts[nIndex].pVPPBufHeader[nCount].pBufHeader->nFilledLen,
                                PERF_ModuleLLMM);
#endif
                            if(pComponentPrivate->sCompPorts[nIndex].pVPPBufHeader[nCount].eBufferOwner != VPP_BUFFER_CLIENT){
                                pComponentPrivate->sCompPorts[nIndex].pVPPBufHeader[nCount].eBufferOwner = VPP_BUFFER_CLIENT;
                                eError = OMX_EmptyThisBuffer(
                                                    pComponentPrivate->sCompPorts[nIndex].hTunnelComponent, 
                                                    pComponentPrivate->sCompPorts[nIndex].pVPPBufHeader[nCount].pBufHeader);
                            }
                        } 
                        else { /* pComponentPrivate->sCompPorts[nIndex].pPortDef.eDir == OMX_DirInput */
                            VPP_DPRINT("VPP is at input port\n");

#ifdef __PERF_INSTRUMENTATION__
                            PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                pComponentPrivate->sCompPorts[nIndex].pVPPBufHeader[nCount].pBufHeader->pBuffer,
                                0,
                                PERF_ModuleLLMM);
#endif

                            VPP_DPRINT("VPP return buffer to tunnel\n");
                            if(pComponentPrivate->sCompPorts[nIndex].pVPPBufHeader[nCount].eBufferOwner != VPP_BUFFER_CLIENT){
                                pComponentPrivate->sCompPorts[nIndex].pVPPBufHeader[nCount].eBufferOwner = VPP_BUFFER_CLIENT;
                                VPP_DPRINT("VPP_UTILS: call to OMX_FillThisBuffer():: %d\n", __LINE__);
                                eError = OMX_FillThisBuffer(
                                                    pComponentPrivate->sCompPorts[nIndex].hTunnelComponent,
                                                    pComponentPrivate->sCompPorts[nIndex].pVPPBufHeader[nCount].pBufHeader);
                           }
                        }
                    }
                }
    } else {  /* pComponentPrivate->sCompPorts[nIndex].hTunnelComponent == NULL */


        /* for (nIndex = 0; nIndex < NUM_OF_VPP_PORTS; nIndex ++) { */
        VPP_DPRINT("VPP_Utils.c: (%d) %d %p\n", __LINE__, nIndex, pComponentPrivate->sCompPorts[nIndex].hTunnelComponent);
            for (nCount = 0; nCount < pComponentPrivate->sCompPorts[nIndex].nBufferCount; nCount++) {
                VPP_DPRINT("VPP:: port %d count %d bufHeader %p owner %d\n", nIndex, nCount, 
                    pComponentPrivate->sCompPorts[nIndex].pVPPBufHeader[nCount].pBufHeader,
                    pComponentPrivate->sCompPorts[nIndex].pVPPBufHeader[nCount].eBufferOwner);
                pthread_mutex_lock(&pComponentPrivate->buf_mutex);
                if (pComponentPrivate->sCompPorts[nIndex].pVPPBufHeader[nCount].eBufferOwner != VPP_BUFFER_CLIENT) {
                if (pComponentPrivate->sCompPorts[nIndex].pVPPBufHeader[nCount].eBufferOwner == VPP_BUFFER_COMPONENT_IN){
                    if (nIndex == 0 || nIndex == 1) {
                        nFilledInBuf ++;
                    } else {
                        VPP_DPRINT("index %d cnt %d owner %d %p\n", nIndex, nCount, 
                            pComponentPrivate->sCompPorts[nIndex].pVPPBufHeader[nCount].eBufferOwner,
                            pComponentPrivate->sCompPorts[nIndex].pVPPBufHeader[nCount].pBufHeader);
                        nFreeOutBuf ++;
                    }
                } else if (pComponentPrivate->sCompPorts[nIndex].pVPPBufHeader[nCount].eBufferOwner == VPP_BUFFER_COMPONENT_OUT){
                    if (nIndex == 0 || nIndex == 1) {
                        nFreeInBuf ++;
                    } else {
                        nFilledOutBuf ++;
                    }
                } else {
                    VPP_DPRINT("Buffer %p is in DSP, error!\n", pComponentPrivate->sCompPorts[nIndex].pVPPBufHeader[nCount].pBufHeader);
                    pComponentPrivate->sCompPorts[nIndex].pVPPBufHeader[nCount].eBufferOwner = VPP_BUFFER_CLIENT;
                    if (nIndex == 0 || nIndex == 1) {

                        pComponentPrivate->cbInfo.EmptyBufferDone(
                                           pHandle,
                                           pHandle->pApplicationPrivate,
                                           pComponentPrivate->sCompPorts[nIndex].pVPPBufHeader[nCount].pBufHeader);
                    } else {
                        pComponentPrivate->cbInfo.FillBufferDone(
                                           pHandle,
                                           pHandle->pApplicationPrivate,
                                           pComponentPrivate->sCompPorts[nIndex].pVPPBufHeader[nCount].pBufHeader);
                    }
                }
                }
                pthread_mutex_unlock(&pComponentPrivate->buf_mutex);
            }
        VPP_DPRINT("nFilledInBuf %d nFreeInBuf %d nFilledOutBuf %d nFreeOutBuf %d\n", nFilledInBuf, nFreeInBuf, nFilledOutBuf, nFreeOutBuf);
    }
    }

        VPP_DPRINT("nFilledInBuf %d nFreeInBuf %d nFilledOutBuf %d nFreeOutBuf %d\n", nFilledInBuf, nFreeInBuf, nFilledOutBuf, nFreeOutBuf);
        for (kk = 0; kk < nFilledInBuf; kk ++) {
            VPP_Process_FilledInBuf(pComponentPrivate);
        }
        for (kk = 0; kk < nFreeOutBuf; kk ++) {
            VPP_Process_FreeOutBuf(pComponentPrivate);      
        }
        VPP_DPRINT("VPP after loop: nFilledInBuf %d nFreeInBuf %d nFilledOutBuf %d nFreeOutBuf %d\n", nFilledInBuf, nFreeInBuf, nFilledOutBuf, nFreeOutBuf);

        for( nIndex = 0; nIndex < NUM_OF_VPP_PORTS; nIndex++) {
            VPP_DPRINT("port %d is %d (%p)\n", nIndex, pComponentPrivate->sCompPorts[nIndex].pPortDef.bEnabled,pComponentPrivate->sCompPorts[nIndex].hTunnelComponent);
            if (pComponentPrivate->sCompPorts[nIndex].pPortDef.bEnabled == OMX_FALSE) {
                continue;
            }
            if (pComponentPrivate->sCompPorts[nIndex].hTunnelComponent != NULL) {
                for (nCount = 0; nCount < pComponentPrivate->sCompPorts[nIndex].nBufferCount; nCount++) {
                     if (pComponentPrivate->sCompPorts[nIndex].pPortDef.eDir == OMX_DirOutput 
                                && pComponentPrivate->sCompPorts[nIndex].eSupplierSetting == OMX_BufferSupplyOutput) {
                         VPP_DPRINT("VPP :: pHandle=%p, eBufferOwner= %d, nIndex= %d\n", pComponentPrivate->pHandle, pComponentPrivate->sCompPorts[nIndex].pVPPBufHeader[nCount].eBufferOwner, nIndex);


        if(pComponentPrivate->sCompPorts[nIndex].pVPPBufHeader[nCount].eBufferOwner == VPP_BUFFER_DSP) {
                                 pComponentPrivate->sCompPorts[nIndex].pVPPBufHeader[nCount].eBufferOwner = VPP_BUFFER_COMPONENT_OUT;;
            }
                while((pComponentPrivate->sCompPorts[nIndex].pVPPBufHeader[nCount].eBufferOwner != VPP_BUFFER_COMPONENT_IN) && 
                                        (pComponentPrivate->sCompPorts[nIndex].pVPPBufHeader[nCount].eBufferOwner != VPP_BUFFER_COMPONENT_OUT)){
                                 pComponentPrivate->sCompPorts[nIndex].pVPPBufHeader[nCount].eBufferOwner = VPP_BUFFER_COMPONENT_OUT;
                    sched_yield();
                        }
                         VPP_DPRINT("VPP:: Component have all the buffers, eBufferOwner= %d\n", pComponentPrivate->sCompPorts[nIndex].pVPPBufHeader[nCount].eBufferOwner);
                    }
                    else if(pComponentPrivate->sCompPorts[nIndex].pPortDef.eDir == OMX_DirInput
                                && pComponentPrivate->sCompPorts[nIndex].eSupplierSetting == OMX_BufferSupplyInput) {
                        VPP_DPRINT("VPP Utils :: pHandle=%p, eBufferOwner= %d, nIndex= %d\n", pComponentPrivate->pHandle, pComponentPrivate->sCompPorts[nIndex].pVPPBufHeader[nCount].eBufferOwner, nIndex);


        if(pComponentPrivate->sCompPorts[nIndex].pVPPBufHeader[nCount].eBufferOwner == VPP_BUFFER_DSP) {
                                 pComponentPrivate->sCompPorts[nIndex].pVPPBufHeader[nCount].eBufferOwner = VPP_BUFFER_COMPONENT_OUT;;
            }
                while((pComponentPrivate->sCompPorts[nIndex].pVPPBufHeader[nCount].eBufferOwner != VPP_BUFFER_COMPONENT_IN) && 
                                        (pComponentPrivate->sCompPorts[nIndex].pVPPBufHeader[nCount].eBufferOwner != VPP_BUFFER_COMPONENT_OUT)){
                    sched_yield();
                        }
                         VPP_DPRINT("VPP Utils:: Component have all the buffers, eBufferOwner= %d\n", pComponentPrivate->sCompPorts[nIndex].pVPPBufHeader[nCount].eBufferOwner);
                   
                    }
                }
            }
        }

        pComponentPrivate->ExeToIdleFlag |= VPP_BUFFERBACK;
        if (pComponentPrivate->ExeToIdleFlag == VPP_IDLEREADY) {
            pComponentPrivate->curState = OMX_StateIdle;
            pComponentPrivate->cbInfo.EventHandler (
                                pHandle,
                                pHandle->pApplicationPrivate,
                                OMX_EventCmdComplete,
                                OMX_ErrorNone,
                                OMX_StateIdle,
                                "NULL");
            pComponentPrivate->ExeToIdleFlag = VPP_ZERO;
        }
#ifdef RESOURCE_MANAGER_ENABLED

            eError = RMProxy_NewSendCommand(pHandle, RMProxy_StateSet, OMX_VPP_COMPONENT, OMX_StateIdle, 3456, NULL);
#endif    

    }
    else { 
        VPP_DPRINT("%d: Comp: Sending ErrorNotification: Invalid State\n", __LINE__);
        pComponentPrivate->cbInfo.EventHandler(
                            pHandle, 
                            pHandle->pApplicationPrivate,
                            OMX_EventCmdComplete, 
                            OMX_ErrorInvalidState,
                            0,
                            "Invalid State Error from VPP");
    }
EXIT:
    return eError;
}



/* ========================================================================== */
/**
* @StateToExecuting() This function is called by the component when ever it
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
OMX_ERRORTYPE VPP_StateToExecuting(VPP_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;
    OMX_HANDLETYPE    pLcmlHandle = pComponentPrivate->pLcmlHandle;
    OMX_BUFFERHEADERTYPE *pBufHdr = NULL;
    int     i, j;
    int     nBuf;
    char *pArgs = "damedesuStr";


    VPP_DPRINT("VPP::%d: HandleCommand: Cmd Executing \n",__LINE__);

    if (pComponentPrivate->curState == OMX_StateExecuting) {
        VPP_DPRINT("VPP: send OMX_ErrorSameState from OMX_StateInvalid\n");
        pComponentPrivate->cbInfo.EventHandler(
                            pComponentPrivate->pHandle, 
                            pComponentPrivate->pHandle->pApplicationPrivate, 
                            OMX_EventError, 
                            OMX_ErrorSameState, 
                            OMX_TI_ErrorMinor, 
                            NULL);
        if (eError != OMX_ErrorNone) {
        }
        goto EXIT;
    }

     pComponentPrivate->toState = OMX_StateExecuting;

    if (pComponentPrivate->curState == OMX_StateIdle) {/* from Idle to Executing */
        OMX_U32 Inputports = 1;
        int bufCount;

        pComponentPrivate->tVPPIOConf->overlayInputImage = 0;
        pComponentPrivate->tVPPIOConf->YUVOutputImage = 0;
        pComponentPrivate->tVPPIOConf->RGBOutputImage = 0;
        
        pComponentPrivate->sCompPorts[OMX_VPP_INPUT_PORT].nReturnedBufferCount = pComponentPrivate->sCompPorts[OMX_VPP_INPUT_PORT].nBufferCount; /*usmc*/
        
        
        if(pComponentPrivate->IsOverlay == OMX_TRUE) {
            pComponentPrivate->tVPPIOConf->overlayInputImage = 1;
            Inputports =2;
        }
        
        if(pComponentPrivate->NumofOutputPort && pComponentPrivate->NumofOutputPort < 2 ) {
            if (pComponentPrivate->IsYUVdataout) {
                pComponentPrivate->tVPPIOConf->YUVOutputImage = 1;
            }
            else {
                pComponentPrivate->tVPPIOConf->RGBOutputImage = 1;
            }
        }
        else if(pComponentPrivate->NumofOutputPort == 2) {
            pComponentPrivate->tVPPIOConf->YUVOutputImage = 1;
            pComponentPrivate->tVPPIOConf->RGBOutputImage = 1;
        }


        VPP_DPRINT("VPP::%d: before START control \n",__LINE__);
        

        eError = LCML_ControlCodec(
                    ((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                    EMMCodecControlStart,
                    (void *)pArgs);
        if (eError != OMX_ErrorNone) {
            VPP_DPRINT("VPP::%d: Error 0x%X Occurred in Codec Start..\n",__LINE__,eError);
            goto EXIT;
        }

        pComponentPrivate->bIsStopping=0;
        
        VPP_DPRINT ("VPP::%d :: Comp :: After LCML_StartCodec function \n",__LINE__);
        
        for( j=0; j<(int)Inputports; j++) {
            nBuf =pComponentPrivate->sCompPorts[j].nBufferCount;
            VPP_DPRINT ("VPP::Sending Input buffer to Application bufcount=%lu \n",nBuf);
            
            /*TUNNEL HERE */
            for (bufCount = 0; bufCount < nBuf; bufCount++) {
                pBufHdr = pComponentPrivate->sCompPorts[j].pVPPBufHeader[bufCount].pBufHeader;
                if ((pComponentPrivate->sCompPorts[j].hTunnelComponent != NULL) &&
                        (pComponentPrivate->sCompPorts[j].eSupplierSetting == OMX_BufferSupplyInput)) {
                /* VPP owns this buffer */
                    
                    VPP_DPRINT("VPP: send fillthisbuffer, out index %p, %d\n", pBufHdr, pBufHdr->nOutputPortIndex);

#ifdef __PERF_INSTRUMENTATION__
                    PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                    PREF(pBufHdr,pBuffer),
                                    0,
                                    PERF_ModuleLLMM);
#endif

                    pComponentPrivate->sCompPorts[j].pVPPBufHeader[bufCount].eBufferOwner = VPP_BUFFER_CLIENT;
                    VPP_DPRINT("VPP_UTILS: call to OMX_FillThisBuffer():: %d\n", __LINE__);
                    OMX_FillThisBuffer(pComponentPrivate->sCompPorts[j].hTunnelComponent, pBufHdr); 
                }
            }
        }
        
        VPP_DPRINT("VPP:: %d:: Ports Available in Fill_LCMLInitParams  %ld\n ",__LINE__, pComponentPrivate->NumofOutputPort);

        if (pComponentPrivate->IsYUVdataout){
            nBuf = pComponentPrivate->sCompPorts[OMX_VPP_YUV_OUTPUT_PORT].nBufferCount;
            if ((pComponentPrivate->sCompPorts[OMX_VPP_YUV_OUTPUT_PORT].hTunnelComponent != NULL) &&
                    (pComponentPrivate->sCompPorts[OMX_VPP_YUV_OUTPUT_PORT].eSupplierSetting == OMX_BufferSupplyOutput)) {
                for (i=0; i < nBuf; i++) {
                    pBufHdr = pComponentPrivate->sCompPorts[OMX_VPP_YUV_OUTPUT_PORT].pVPPBufHeader[i].pBufHeader;

#ifdef __PERF_INSTRUMENTATION__
                    PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                    pBufHdr->pBuffer,
                                    pBufHdr->nFilledLen,
                                    PERF_ModuleCommonLayer);
#endif
                    VPP_DPRINT("LCML_QueueBuffer YUV: %s::%s: %d: VPP\n", __FILE__, __FUNCTION__, __LINE__);
                    eError = LCML_QueueBuffer(
                                ((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                EMMCodecStream3,
                                pBufHdr->pBuffer,
                                pBufHdr->nAllocLen,0,
                                (OMX_U8 *)pComponentPrivate->pOpYUVFrameStatus,
                                sizeof(GPPToVPPOutputFrameStatus),  
                                (void *)pBufHdr);
                    if (eError != OMX_ErrorNone) {
                        VPP_DPRINT("VPP::%d :: Comp:: Error 0x%X While sending the output buffers to Codec\n", __LINE__,eError);
                        goto EXIT;
                    }
                    VPP_DPRINT ("VPP::%d :: Component Sending Output buffer to Codec %p\n",__LINE__, pBufHdr);
                }
            }
        }
        else if(pComponentPrivate->IsRGBdataout){
            nBuf = pComponentPrivate->sCompPorts[OMX_VPP_RGB_OUTPUT_PORT].nBufferCount;
            if ((pComponentPrivate->sCompPorts[OMX_VPP_RGB_OUTPUT_PORT].hTunnelComponent != NULL) &&
                    (pComponentPrivate->sCompPorts[OMX_VPP_RGB_OUTPUT_PORT].eSupplierSetting == OMX_BufferSupplyOutput)) {
                for (i=0; i < nBuf; i++) {
                    pBufHdr = pComponentPrivate->sCompPorts[OMX_VPP_RGB_OUTPUT_PORT].pVPPBufHeader[i].pBufHeader;

#ifdef __PERF_INSTRUMENTATION__
                    PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                    pBufHdr->pBuffer,
                                    pBufHdr->nFilledLen,
                                    PERF_ModuleCommonLayer);
#endif
                    VPP_DPRINT("LCML_QueueBuffer RGB: %s::%s: %d: VPP\n", __FILE__, __FUNCTION__, __LINE__);
                    eError = LCML_QueueBuffer(
                                ((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                                EMMCodecStream2,
                                pBufHdr->pBuffer,
                                pBufHdr->nAllocLen,0,
                                (OMX_U8 *)pComponentPrivate->pOpRGBFrameStatus,
                                sizeof(GPPToVPPOutputFrameStatus),  
                                (void *)pBufHdr);
                    if (eError != OMX_ErrorNone) {
                        VPP_DPRINT("VPP::%d :: Comp:: Error 0x%X While sending the output buffers to Codec\n", __LINE__,eError);
                        goto EXIT;
                    }
                    VPP_DPRINT ("VPP::%d :: Component Sending Output buffer to Codec %p\n",__LINE__, pBufHdr);
                }
            }
        }
        else{
            eError = OMX_ErrorUndefined;
            VPP_DPRINT("VPP:: %d : No Port enable\n");
            goto EXIT;
        }
    } 
    else if (pComponentPrivate->curState == OMX_StatePause) {
#ifdef RESOURCE_MANAGER_ENABLED
        VPP_DPRINT("%d: Comp: Resume Command Came from App\n",__LINE__);
#endif
        
        /* char *pArgs = "damedesuStr";*/
        eError = LCML_ControlCodec(
                    ((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                    EMMCodecControlStart,(void *)pArgs);

        if (eError != OMX_ErrorNone) {
            VPP_DPRINT ("Error While Resuming the codec\n");
            goto EXIT;
        }
    } 
    else { /* if current state is not Idle or Pause ... */
        pComponentPrivate->cbInfo.EventHandler (
                            pHandle, pHandle->pApplicationPrivate,
                            OMX_EventError, 
                            OMX_ErrorIncorrectStateTransition,OMX_TI_ErrorMinor,
                            "Invalid State from VPP");
        VPP_DPRINT("%d :: Error: Invalid State Given by Application\n",__LINE__);
        goto EXIT;
    }

    pComponentPrivate->ExeToIdleFlag = VPP_ZERO;

    pComponentPrivate->toState = OMX_StateExecuting;
#ifdef RESOURCE_MANAGER_ENABLED
    eError = RMProxy_NewSendCommand(pHandle, RMProxy_StateSet, OMX_VPP_COMPONENT, OMX_StateExecuting, 3456, NULL);
#endif
    pComponentPrivate->curState = OMX_StateExecuting;
    pComponentPrivate->cbInfo.EventHandler(
                        pHandle, 
                        pHandle->pApplicationPrivate,
                        OMX_EventCmdComplete, 
                        OMX_ErrorNone,
                        OMX_StateExecuting, 
                        NULL);
    
EXIT:
    return eError;
}



/* ========================================================================== */
/**
* @StateToLoaded() This function is called by the component when ever it
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
OMX_ERRORTYPE VPP_StateToLoaded(VPP_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE     eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;
    OMX_HANDLETYPE    pLcmlHandle = pComponentPrivate->pLcmlHandle;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = NULL;
    int     nPortIndex;
    OMX_U32 nTimeout = 0;
    
    VPP_DPRINT("VPP::%d: HandleCommand: Cmd Loaded\n",__LINE__);
    VPP_DPRINT("VPP: %d: HandleCommand: Cmd Loaded, current state: %d\n",__LINE__, pComponentPrivate->curState);

    if (pComponentPrivate->curState != OMX_StateIdle && pComponentPrivate->curState != OMX_StateWaitForResources ) {
        pComponentPrivate->cbInfo.EventHandler (
                            pHandle, 
                            pHandle->pApplicationPrivate,
                            OMX_EventError, 
                            OMX_ErrorIncorrectStateTransition,
                            OMX_TI_ErrorMinor, 
                            "Invalid State from VPP");
        VPP_DPRINT("%d :: Error: Invalid State Given by Application\n",__LINE__);
        goto EXIT;
    }

     pComponentPrivate->toState = OMX_StateLoaded;
     
    if (pComponentPrivate->curState == OMX_StateIdle || 
            pComponentPrivate->curState == OMX_StateWaitForResources) {

#ifdef __PERF_INSTRUMENTATION__
        PERF_Boundary(pComponentPrivate->pPERFcomp,
        PERF_BoundaryStart | PERF_BoundaryCleanup);
#endif

#ifdef RESOURCE_MANAGER_ENABLED
            if (pComponentPrivate->curState == OMX_StateWaitForResources) {
                eError= RMProxy_NewSendCommand(pHandle,  RMProxy_CancelWaitForResource, OMX_VPP_COMPONENT, 0, 3456, NULL);
                if (eError != OMX_ErrorNone) {
                    VPP_DPRINT("CancelWaitForResource Failed\n");
                    pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventError,
                                                           OMX_ErrorUndefined,
                                                           OMX_TI_ErrorSevere,
                                                           NULL);
                    goto EXIT;
                }
            }
            
            if (pComponentPrivate->curState != OMX_StateWaitForResources) {
                eError= RMProxy_NewSendCommand(pHandle,  RMProxy_FreeResource, OMX_VPP_COMPONENT, 0, 3456, NULL);
                if (eError != OMX_ErrorNone) {
                    VPP_DPRINT("Cannot Free Resources\n");                    
                    pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventError,
                                                           OMX_ErrorUndefined,
                                                           OMX_TI_ErrorSevere,
                                                           NULL);
                    goto EXIT;
                }
            }
#endif

        if (pLcmlHandle !=NULL) {
            VPP_DPRINT("VPP::%d: HandleCommand: : Loaded calling destroy\n",__LINE__);
            eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                        EMMCodecControlDestroy,
                        NULL);
#ifdef UNDER_CE
             FreeLibrary(g_hLcmlDllHandle);
            g_hLcmlDllHandle = NULL;
#else
            
        VPP_DPRINT("VPP: %d\n", __LINE__);
          if(pComponentPrivate->pLcmlHandle){
               dlclose(pComponentPrivate->pDllHandle);
                pComponentPrivate->pLcmlHandle = NULL;
                pComponentPrivate->pLCML = NULL;
            }
            
#endif                        
            if (eError != OMX_ErrorNone) {
                VPP_DPRINT("VPP::%d : Error 0x%X: in Destroying the codec\n",__LINE__,eError);
                goto EXIT;
            }
        }
        VPP_DPRINT("VPP: %d\n", __LINE__);
       for(nPortIndex = 0; nPortIndex < NUM_OF_VPP_PORTS; nPortIndex++) {
                VPP_DPRINT("VPP free tunneled buf %d %p %x %x\n", nPortIndex,
                pComponentPrivate->sCompPorts[nPortIndex].hTunnelComponent,
                pComponentPrivate->sCompPorts[nPortIndex].nBufSupplier,
                pComponentPrivate->sCompPorts[nPortIndex].pPortDef.bEnabled);

            if (pComponentPrivate->sCompPorts[nPortIndex].hTunnelComponent != NULL && 
                pComponentPrivate->sCompPorts[nPortIndex].nBufSupplier == OMX_TRUE 
                    /*&& pComponentPrivate->sCompPorts[nPortIndex].pVPPBufHeader[0].bSelfAllocated*/) {
                OMX_U32 nBuf;
                OMX_U8 *pBufferStart = NULL;
                OMX_BUFFERHEADERTYPE *pBufHeader;

               for (nBuf=0; nBuf<pComponentPrivate->sCompPorts[nPortIndex].pPortDef.nBufferCountActual; nBuf++) {
                    VPP_DPRINT("PORT  %d is Supplier !! .....\n",nPortIndex);
                    pBufferStart = pComponentPrivate->sCompPorts[nPortIndex].pVPPBufHeader[nBuf].pBufferStart;
                    pBufHeader = pComponentPrivate->sCompPorts[nPortIndex].pVPPBufHeader[nBuf].pBufHeader;


#ifdef __PERF_INSTRUMENTATION__
                    PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                    PREF(pBufHeader,pBuffer),
                                    PREF(pBufHeader,nAllocLen),
                                    PERF_ModuleLLMM);
#endif

                    if(pBufHeader != NULL){
                        OMX_FREE(pBufferStart);
                        pBufferStart = NULL;
                        pBufHeader->pBuffer = NULL;
                    }

                    pComponentPrivate->sCompPorts[nPortIndex].nBufferCount --;
                    pComponentPrivate->sCompPorts[nPortIndex].pPortDef.bPopulated = OMX_FALSE;
                    eError = OMX_FreeBuffer(pComponentPrivate->sCompPorts[nPortIndex].hTunnelComponent, 
                                pComponentPrivate->sCompPorts[nPortIndex].nTunnelPort, 
                                pComponentPrivate->sCompPorts[nPortIndex].pVPPBufHeader[nBuf].pBufHeader
                                ); 
                    if (eError != OMX_ErrorNone) {
                        VPP_DPRINT ("OMX_FreeBuffer Failed !! .....\n");
                        goto EXIT;
                    }
                }
                
            }/*End of Tunneling component*/

            pComponentPrivate->nInputFrame = 0;
            pComponentPrivate->nOverlayFrame = 0;
            pComponentPrivate->nInYUVBufferCount = 0;
            pComponentPrivate->nInRGBBufferCount = 0;
            pComponentPrivate->nOutYUVBufferCount = 0;
            pComponentPrivate->nOutRGBBufferCount = 0;
    
            pPortDef = &(pComponentPrivate->sCompPorts[nPortIndex].pPortDef);
            VPP_DPRINT("%d pPortDef.bEnabled %d\n", nPortIndex, pComponentPrivate->sCompPorts[nPortIndex].pPortDef.bEnabled);
            if (pComponentPrivate->sCompPorts[nPortIndex].pPortDef.bEnabled == OMX_TRUE) {
                nTimeout = 0;
                while(1)
                {
                    if (!pComponentPrivate->sCompPorts[nPortIndex].pPortDef.bPopulated) {
                        break;
                    }
                    else if (nTimeout++ > 0xEFFFFFFE) {
                        pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle, 
                                            pComponentPrivate->pHandle->pApplicationPrivate,
                                            OMX_EventError, 
                                            OMX_ErrorPortUnresponsiveDuringDeallocation, 
                                            OMX_TI_ErrorSevere, 
                                            "Port Unresponsive - Idle");      
                        break;
                    }
                    sched_yield();
                }
            }
       }
    }


#if 0
#ifdef __PERF_INSTRUMENTATION__
    PERF_Boundary(pComponentPrivate->pPERFcomp,
                PERF_BoundaryComplete | PERF_BoundaryCleanup);
#endif
#endif



    if ((pComponentPrivate->curState == OMX_StateIdle) &&
         (pComponentPrivate->bPreempted == 1 )){

        pComponentPrivate->curState = OMX_StateLoaded;
        pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                               pComponentPrivate->pHandle->pApplicationPrivate,
                                               OMX_EventError,
                                               OMX_ErrorResourcesLost,
                                               OMX_TI_ErrorSevere,
                                               NULL);
        pComponentPrivate->bPreempted = 0;
    }
    else {
        pComponentPrivate->curState = OMX_StateLoaded;
        pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                               pComponentPrivate->pHandle->pApplicationPrivate,
                                               OMX_EventCmdComplete,
                                               OMX_CommandStateSet,
                                               OMX_StateLoaded,
                                               NULL);
    }
    
EXIT:
    return eError;
}



/* ========================================================================== */
/**
* @HandleCommand() This function is called by the component when ever it
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
OMX_ERRORTYPE VPP_HandleCommand (VPP_COMPONENT_PRIVATE *pComponentPrivate, OMX_U32 nParam1)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    char *pArgs = "damedesuStr";
    /*OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;*/
    OMX_HANDLETYPE pLcmlHandle = pComponentPrivate->pLcmlHandle;

    VPP_DPRINT ("VPP::%d :: >>> Entering HandleCommand Function\n",__LINE__);

    if (pComponentPrivate->curState == nParam1) {
        VPP_DPRINT("VPP: send OMX_ErrorSameState from OMX_StateInvalid\n");
        pComponentPrivate->cbInfo.EventHandler(
                            pComponentPrivate->pHandle, 
                            pComponentPrivate->pHandle->pApplicationPrivate, 
                            OMX_EventError, 
                            OMX_ErrorSameState, 
                            OMX_TI_ErrorMinor, 
                            NULL);
        if (eError != OMX_ErrorNone) {
            VPP_DPRINT("VPP::%d : Error 0x%X: in Destroying the codec\n",__LINE__,eError);
        }
        goto EXIT;
    }      
    
    switch(nParam1)
    {
    case OMX_StateInvalid:
        if (pComponentPrivate->curState == OMX_StateIdle || 
                pComponentPrivate->curState == OMX_StateExecuting ||  
                pComponentPrivate->curState == OMX_StatePause  ) {
            eError = LCML_ControlCodec(
                        ((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                        EMMCodecControlDestroy,
                        NULL);
#ifdef UNDER_CE
            FreeLibrary(g_hLcmlDllHandle);
            g_hLcmlDllHandle = NULL;
#else
            if(pComponentPrivate->pLcmlHandle){
                dlclose(pComponentPrivate->pDllHandle);
                pComponentPrivate->pLcmlHandle = NULL;
                pComponentPrivate->pLCML = NULL;
            }
#endif
        }
        pComponentPrivate->curState = OMX_StateInvalid;
        pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle, 
                            pComponentPrivate->pHandle->pApplicationPrivate, 
                            OMX_EventError, 
                            OMX_ErrorInvalidState, 
                            OMX_TI_ErrorCritical, 
                            NULL); 
        pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                            pComponentPrivate->pHandle->pApplicationPrivate,
                            OMX_EventCmdComplete, 
                            OMX_CommandStateSet, 
                            pComponentPrivate->curState, 
                            NULL); 
        break;
    case OMX_StateIdle:
        eError = VPP_StateToIdle(pComponentPrivate);
        break;
    case OMX_StateExecuting:
        eError = VPP_StateToExecuting(pComponentPrivate);
        break;
    case OMX_StateLoaded:
        eError = VPP_StateToLoaded(pComponentPrivate);
        break;
    case OMX_StatePause:
        VPP_DPRINT("%d: HandleCommand: Cmd Pause: Cur State = %d\n",__LINE__, pComponentPrivate->curState);
        if ( pComponentPrivate->curState == OMX_StateExecuting || 
                pComponentPrivate->curState == OMX_StateIdle ) {

#ifdef __PERF_INSTRUMENTATION__
            PERF_Boundary(pComponentPrivate->pPERFcomp,
                        PERF_BoundaryComplete | PERF_BoundarySteadyState);
#endif

            pComponentPrivate->toState = OMX_StatePause;
            pComponentPrivate->ExeToIdleFlag = VPP_ZERO;
            eError = LCML_ControlCodec(
                        ((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                        EMMCodecControlPause,
                        (void *)pArgs);
            
            if (eError != OMX_ErrorNone) {
                VPP_DPRINT("VPP::%d : Error0x%X: in Pausing the codec\n",__LINE__,eError);
                goto EXIT;
            }

            /*Sending to Idle until receiving EMMCodecProcessingPaused call back*/

        }
        else {
            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                pComponentPrivate->pHandle->pApplicationPrivate, 
                                OMX_EventError, 
                                OMX_ErrorIncorrectStateTransition, 
                                OMX_TI_ErrorMinor, 
                                NULL);
            VPP_DPRINT ("VPP::%d :: Error: Invalid State Given by Application\n",__LINE__);
        }
        break;
    case OMX_StateWaitForResources:
        VPP_DPRINT("VPP: SetState to WaitForResources, curState is %d\n", pComponentPrivate->curState);
        if (pComponentPrivate->curState == OMX_StateLoaded) {

#ifdef RESOURCE_MANAGER_ENABLED
            eError= RMProxy_NewSendCommand(pComponentPrivate->pHandle, RMProxy_StateSet, OMX_VPP_COMPONENT, OMX_StateWaitForResources, 3456, NULL);
            if (eError != OMX_ErrorNone) {
                VPP_DPRINT("RMProxy_NewSendCommand(OMX_StateWaitForResources) failed\n");
                pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                                       OMX_EventError,
                                                       OMX_ErrorUndefined,
                                                       OMX_TI_ErrorSevere,
                                                       NULL);
                break;
            }
#endif
            
            pComponentPrivate->curState = OMX_StateWaitForResources;
            VPP_DPRINT("VPP: my state is %d, from OMX_StateLoaded, before call EventHandler\n", pComponentPrivate->curState);
            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                pComponentPrivate->pHandle->pApplicationPrivate,
                                OMX_EventCmdComplete, 
                                OMX_CommandStateSet, 
                                pComponentPrivate->curState, 
                                NULL);
            VPP_DPRINT("VPP: after call EventHandler\n");
        }
        else {
            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                pComponentPrivate->pHandle->pApplicationPrivate, 
                                OMX_EventError, 
                                OMX_ErrorIncorrectStateTransition, 
                                OMX_TI_ErrorMinor, 
                                NULL);
        }
        break;
    case OMX_StateMax:
        VPP_DPRINT("VPP::%d: HandleCommand: Cmd OMX_StateMax::\n",__LINE__);
        break;
    default:
        break;
    }
EXIT:
    VPP_DPRINT ("VPP::%d :: Exiting HandleCommand Function, eError=0x%X,\n",__LINE__,eError);
    return eError;
}



/**
* @VPP_ProcessFilledInBuf() This function is called by the component  Thread whenever it
* receives the an input buffer from the application
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
OMX_ERRORTYPE VPP_Process_FilledInBuf(VPP_COMPONENT_PRIVATE* pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_DIRTYPE eDir = OMX_DirMax;
    OMX_PARAM_PORTDEFINITIONTYPE *portDef = NULL;
    OMX_U32 nIndex;
    OMX_VPP_COMPONENT_BUFFER *pComponentBuf = NULL;
    LCML_DSP_INTERFACE *pLcmlHandle = NULL;
    OMX_BUFFERHEADERTYPE* pBufHeader = NULL;
    OMX_COMPONENTTYPE               *pHandle = NULL;
    OMX_U8 *pTemp = NULL;
    int nRet=0;
    
    pHandle = pComponentPrivate->pHandle;

    VPP_DPRINT("In VPP_Process_FilledInBuf\n");

    nRet = read(pComponentPrivate->nFilled_iPipe[0], &(pBufHeader),sizeof(pBufHeader));
    if (-1 == nRet) {
        VPP_DPRINT ("%d :: Error while reading from the pipe\n",__LINE__);
    }
    VPP_DPRINT("%d :: Entering VPP_Process_FilledInBuf with pBufHeader=%p\n",__LINE__, pBufHeader);

    if (pBufHeader->nFlags & OMX_BUFFERFLAG_EOS) {
        VPP_DPRINT("EOS flag is in input buffer (len %d)\n", pBufHeader->nFilledLen);
    }

    eError = VPP_GetPortDefFromBufHeader(pBufHeader, &portDef);
    
    if (eError != OMX_ErrorNone) {
        VPP_DPRINT("VPP:: Got error in _GetPortDefFromBufHeader. Code %x\n", eError);
        goto EXIT;
    }
    VPP_DPRINT("THE PORT INDEX BEFORE VPP_ISVALIDBUFFER IS %d\n", portDef->nPortIndex);
    
    eError = VPP_IsValidBuffer(pBufHeader, pComponentPrivate, portDef->nPortIndex, &nIndex);
    if (eError != OMX_ErrorNone) {
        OMX_SET_ERROR_BAIL(eError, OMX_ErrorBadParameter);
    }
    pComponentPrivate->sCompPorts[portDef->nPortIndex].pVPPBufHeader[nIndex].bHolding = OMX_TRUE;
    
    if (!pComponentPrivate->sCompPorts[portDef->nPortIndex].pPortDef.bEnabled) {
        VPP_DPRINT("cur port %p is disabled\n", portDef);
        pComponentPrivate->sCompPorts[portDef->nPortIndex].pVPPBufHeader[nIndex].eBufferOwner = VPP_BUFFER_CLIENT;
        pComponentPrivate->cbInfo.EmptyBufferDone (
                                pHandle, 
                                pHandle->pApplicationPrivate,
                                pBufHeader
                                );
            goto EXIT;
    }

    if (pComponentPrivate->bIsStopping == OMX_TRUE) {
        VPP_DPRINT("VPP: stop! return buffer to %p\n", pComponentPrivate->sCompPorts[portDef->nPortIndex].hTunnelComponent);
        if (pComponentPrivate->sCompPorts[portDef->nPortIndex].hTunnelComponent == NULL) {
                  pComponentPrivate->sCompPorts[portDef->nPortIndex].pVPPBufHeader[nIndex].eBufferOwner = VPP_BUFFER_CLIENT;
            pComponentPrivate->cbInfo.EmptyBufferDone (pHandle,
                               pHandle->pApplicationPrivate,
                               pBufHeader
                               );
        } else {
            if(pComponentPrivate->sCompPorts[portDef->nPortIndex].eSupplierSetting == OMX_BufferSupplyOutput){
                pComponentPrivate->sCompPorts[portDef->nPortIndex].pVPPBufHeader[nIndex].eBufferOwner = VPP_BUFFER_TUNNEL_COMPONENT;
                VPP_DPRINT("VPP_UTILS: call to OMX_FillThisBuffer():: %d\n", __LINE__);
                eError = OMX_FillThisBuffer(
                                   pComponentPrivate->sCompPorts[portDef->nPortIndex].hTunnelComponent,
                                   pComponentPrivate->sCompPorts[portDef->nPortIndex].pVPPBufHeader[nIndex].pBufHeader);
            }
            else{
                            pComponentPrivate->sCompPorts[portDef->nPortIndex].pVPPBufHeader[nIndex].eBufferOwner = VPP_BUFFER_COMPONENT_IN;
            }
        }
        goto EXIT;
    }
    if (portDef->nPortIndex == OMX_VPP_INPUT_PORT || portDef->nPortIndex == OMX_VPP_INPUT_OVERLAY_PORT) {
        VPP_DPRINT("VPP:: INPUT Buffer Came %ld ...\n",portDef->nPortIndex);
        eDir = OMX_DirInput;
    }
    else {
        VPP_DPRINT ("VPP::%d :: The PBufHeader is not found in the list\n", __LINE__);
        goto EXIT;
    }

    if (pBufHeader->nFilledLen >= 0) {
        pLcmlHandle = (LCML_DSP_INTERFACE *) pComponentPrivate->pLcmlHandle;
        eError = VPP_GetCorresponding_LCMLHeader(pComponentPrivate, pBufHeader->pBuffer, OMX_DirInput, &pComponentBuf, portDef->nPortIndex );
        if (eError != OMX_ErrorNone) {
            VPP_DPRINT("%d :: Error: Invalid Buffer Came ...\n",__LINE__);
            goto EXIT;
        }
        
        if (pComponentPrivate->bIsStopping == 1) {
            pComponentBuf->eBufferOwner = VPP_BUFFER_CLIENT;
                pComponentPrivate->cbInfo.EmptyBufferDone (pHandle,
                                    pHandle->pApplicationPrivate,
                                    pComponentBuf->pBufHeader
                                    );
            goto EXIT;
        }

        /*check for overlay data if yes then go for no parameter BUFER */
        if (portDef->nPortIndex == OMX_VPP_INPUT_PORT) {

#ifdef __PERF_INSTRUMENTATION__
            PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                pBufHeader->pBuffer,
                pBufHeader->nFilledLen,
                PERF_ModuleCommonLayer);
#endif

            pComponentPrivate->sCompPorts[portDef->nPortIndex].pVPPBufHeader[nIndex].eBufferOwner = VPP_BUFFER_DSP;

            VPP_DPRINT("VPP: queue input buffer nFilledLen = (%d), BufHdr = %p\n", pBufHeader->nFilledLen, pBufHeader);
            VPP_DPRINT("Queued Input Buffer: Input Width= %lu, Input Height=%lu, Inp. Offset: %lu \
            RGBRotation = %lu, ulYUVRotation = %lu, ulMirror = %lu\n",
                pComponentPrivate->pIpFrameStatus->ulInWidth,
                pComponentPrivate->pIpFrameStatus->ulInHeight,
                pComponentPrivate->pIpFrameStatus->ulCInOffset,
                pComponentPrivate->pIpFrameStatus->ulRGBRotation,
                pComponentPrivate->pIpFrameStatus->ulYUVRotation,
                pComponentPrivate->pIpFrameStatus->ulMirror);
            
            eError = LCML_QueueBuffer(pLcmlHandle->pCodecinterfacehandle,
                        EMMCodecInputBuffer,
                        pBufHeader->pBuffer,
                        pBufHeader->nAllocLen,
                        pBufHeader->nFilledLen,
                        (OMX_U8 *) pComponentPrivate->pIpFrameStatus,
                        sizeof(GPPToVPPInputFrameStatus),
                        (void *) pBufHeader);


            
        }
        else if (portDef->nPortIndex == OMX_VPP_INPUT_OVERLAY_PORT) {
            pTemp = memcpy(pComponentPrivate->RGBbuffer,pBufHeader->pBuffer,pBufHeader->nFilledLen);
            if(pTemp == NULL){
                eError = OMX_ErrorUndefined;
                goto EXIT;
            }
            VPP_DPRINT("VPP::%d: before calling ComputeTiOverlayImgFormat \n",__LINE__);
            eError = ComputeTiOverlayImgFormat(pComponentPrivate,
                                    pComponentPrivate->RGBbuffer,
                                    pBufHeader->pBuffer,
                                    pComponentPrivate->colorKey);
            if (eError != OMX_ErrorNone) {
                VPP_DPRINT ("VPP::%d ::ComputeTiOverlayImgFormat, Error Occurred: %x\n",__LINE__, eError);
                goto EXIT;
            }
            VPP_DPRINT("VPP::%d: after calling ComputeTiOverlayImgFormat \n",__LINE__);
            pBufHeader->nFilledLen= (pBufHeader->nFilledLen*2)/3;
#if 0

    FILE *fp;

    fp = fopen("mytestcvnew.raw", "w");
    fwrite(pBufHeader->pBuffer, 1, pBufHeader->nFilledLen, fp);
    fclose(fp);
           VPP_DPRINT("write %d bytes to mytestcvnew.raw\n", pBufHeader->nFilledLen);
           exit(0);
#endif

#ifdef __PERF_INSTRUMENTATION__
            PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                            pBufHeader->pBuffer,
                            pBufHeader->nFilledLen,
                            PERF_ModuleCommonLayer);
#endif
            pComponentPrivate->sCompPorts[portDef->nPortIndex].pVPPBufHeader[nIndex].eBufferOwner = VPP_BUFFER_DSP;
            eError = LCML_QueueBuffer(
                        pLcmlHandle->pCodecinterfacehandle,
                        EMMCodecStream1,
                        pBufHeader->pBuffer,
                        pBufHeader->nAllocLen,
                        pBufHeader->nFilledLen,
                        NULL,
                        0,
                        (void *) pBufHeader);
            
            VPP_DPRINT("LCML_QueueBuffer from OMX_VPP_INPUT_OVERLAY_PORT, pBufHeader %p, ->pBuffer %p\n", 
                pBufHeader, pBufHeader->pBuffer);
        }
        if (eError != OMX_ErrorNone) {
            VPP_DPRINT ("VPP::%d ::Comp: SetBuff: IP: Error Occurred\n",__LINE__);
            eError = OMX_ErrorHardware;
            goto EXIT;
        }
    }
    VPP_DPRINT ("VPP::Sending Input buffer to Codec\n");
EXIT:
    return eError;
}



/**
  * VPP_Process_FreeOutBuf()
  *
  * Called by component thread, handles free output buffers from app.
  *
  * @param pComponentPrivate private component structure for this instance of the component
  *
  * @param phandle LCML_DSP_INTERFACE handle for this instance of the component
  *
  * @retval OMX_ErrorNone                  success, ready to roll
  *         OMX_ErrorInsufficientResources if the malloc fails
  **/
OMX_ERRORTYPE VPP_Process_FreeOutBuf(VPP_COMPONENT_PRIVATE* pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE *portDef = NULL;
    OMX_U32 nIndex;
    OMX_VPP_COMPONENT_BUFFER *pComponentBuf  = NULL;
    LCML_DSP_INTERFACE *pLcmlHandle = NULL;
    OMX_BUFFERHEADERTYPE* pBufHeader = NULL;
    OMX_COMPONENTTYPE               *pHandle = NULL;
    int nRet = 0;

    VPP_DPRINT("In VPP_Process_FreeOutBuf\n");

    pHandle = pComponentPrivate->pHandle;
    
    nRet = read(pComponentPrivate->nFree_oPipe[0], &pBufHeader,sizeof(pBufHeader));
    if (-1 == nRet) {
        VPP_DPRINT ("%d :: Error while reading from the pipe\n",__LINE__);
    }
    VPP_DPRINT("In VPP_Process_FreeOutBuf\n");

 

    pLcmlHandle = (LCML_DSP_INTERFACE *) pComponentPrivate->pLcmlHandle;
    eError = VPP_GetPortDefFromBufHeader(pBufHeader, &portDef);
    if (eError != OMX_ErrorNone) {
        VPP_DPRINT("VPP: Error in _GetPortDefFromBufHeader. Code %d\n", eError);
        goto EXIT;
    }



    eError = VPP_IsValidBuffer(pBufHeader, pComponentPrivate, portDef->nPortIndex, &nIndex);

    if ( eError != OMX_ErrorNone) {
        goto EXIT;
    }

    if ((pComponentPrivate->bIsStopping != OMX_FALSE ) || (pComponentPrivate->curState == OMX_StateIdle)) {
        VPP_DPRINT("VPP is not in executing state (in FreeOutBuf %d %d %p)\n", portDef->nPortIndex, nIndex, pBufHeader);
        VPP_DPRINT("cur state %d to state %d\n", pComponentPrivate->curState, pComponentPrivate->toState);
        pthread_mutex_lock(&pComponentPrivate->buf_mutex);
        VPP_DPRINT("VPP: return buffer to (%d) %p\n", portDef->nPortIndex, pComponentPrivate->sCompPorts[portDef->nPortIndex].hTunnelComponent);
        if (pComponentPrivate->sCompPorts[portDef->nPortIndex].hTunnelComponent == NULL) {
            pComponentPrivate->sCompPorts[portDef->nPortIndex].pVPPBufHeader[nIndex].eBufferOwner = VPP_BUFFER_CLIENT;
            pComponentPrivate->cbInfo.FillBufferDone (
                                pHandle, 
                                pHandle->pApplicationPrivate,
                                pBufHeader
                                );
        } else {
            if(pComponentPrivate->sCompPorts[portDef->nPortIndex].eSupplierSetting == OMX_BufferSupplyInput){
                pComponentPrivate->sCompPorts[portDef->nPortIndex].pVPPBufHeader[nIndex].eBufferOwner = VPP_BUFFER_TUNNEL_COMPONENT;
                 eError = OMX_EmptyThisBuffer(
                                    pComponentPrivate->sCompPorts[portDef->nPortIndex].hTunnelComponent, 
                                    pComponentPrivate->sCompPorts[portDef->nPortIndex].pVPPBufHeader[nIndex].pBufHeader);
            }
            else{
                pComponentPrivate->sCompPorts[portDef->nPortIndex].pVPPBufHeader[nIndex].eBufferOwner = VPP_BUFFER_COMPONENT_IN;
            }
        }
        pthread_mutex_unlock(&pComponentPrivate->buf_mutex);

       goto EXIT;
    }  

    if (!pComponentPrivate->sCompPorts[portDef->nPortIndex].pPortDef.bEnabled) {
        VPP_DPRINT("In VPP_Process_FreeOutBuf port %p is disabled %p\n", portDef, pBufHeader);
        pComponentPrivate->sCompPorts[portDef->nPortIndex].pVPPBufHeader[nIndex].eBufferOwner = VPP_BUFFER_CLIENT;
            pComponentPrivate->cbInfo.FillBufferDone (
                                pHandle, 
                                pHandle->pApplicationPrivate,
                                pBufHeader
                                );
            goto EXIT;
    }

    pComponentPrivate->sCompPorts[portDef->nPortIndex].pVPPBufHeader[nIndex].bHolding = OMX_TRUE;
    eError = VPP_GetCorresponding_LCMLHeader(pComponentPrivate, pBufHeader->pBuffer, OMX_DirOutput, &pComponentBuf, portDef->nPortIndex);
    if (eError != OMX_ErrorNone) {
        VPP_DPRINT("VPP::%d :: Error: Invalid Buffer Came ...\n",__LINE__);
        goto EXIT;
    }



    if (pComponentPrivate->bIsStopping == OMX_FALSE) {
#ifdef __PERF_INSTRUMENTATION__
        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                            pBufHeader->pBuffer,
                            0,
                            PERF_ModuleCommonLayer);
#endif

        pComponentPrivate->sCompPorts[portDef->nPortIndex].pVPPBufHeader[nIndex].eBufferOwner = VPP_BUFFER_DSP;
        if (portDef->nPortIndex == OMX_VPP_RGB_OUTPUT_PORT) {
            eError = LCML_QueueBuffer(
                    pLcmlHandle->pCodecinterfacehandle,
                    EMMCodecStream2,
                    pBufHeader->pBuffer,
                    pBufHeader->nAllocLen,0,
                    (OMX_U8 *) pComponentPrivate->pOpRGBFrameStatus,
                    sizeof(GPPToVPPOutputFrameStatus),
                    (void *) pBufHeader);
            VPP_DPRINT("VPP queue OMX_VPP_RGB_OUTPUT_PORT %p\n", pBufHeader);
        } else { /* portDef->nPortIndex == OMX_VPP_YUV_OUTPUT_PORT) */
           eError = LCML_QueueBuffer(
                    pLcmlHandle->pCodecinterfacehandle,
                    EMMCodecStream3,
                    pBufHeader->pBuffer,
                    pBufHeader->nAllocLen,0,
                    (OMX_U8 *) pComponentPrivate->pOpYUVFrameStatus,
                    sizeof(GPPToVPPOutputFrameStatus),
                    (void *) pBufHeader);
            VPP_DPRINT("VPP queue OMX_VPP_YUV_OUTPUT_PORT %p\n", pBufHeader);
        }

        VPP_DPRINT("Queued Output Buffer: Out Width= %lu, Out Height=%lu, Out. Offset: %lu, befferlen: %lu\n",
            pComponentPrivate->pOpYUVFrameStatus->ulOutWidth,
            pComponentPrivate->pOpYUVFrameStatus->ulOutHeight,
            pComponentPrivate->pOpYUVFrameStatus->ulCOutOffset,
            pBufHeader->nAllocLen);

        if (eError != OMX_ErrorNone ) {
            VPP_DPRINT ("VPP::%d :: Comp:: SetBuff OP: Error Occurred\n", __LINE__);
            VPP_DPRINT("%s::%d::Error 0x%X from LCML_QueueBuffer\n",__FILE__,__LINE__,eError);
            eError = OMX_ErrorHardware;
            goto EXIT;
        }
    }
EXIT:
    return eError;
}



/**
* @VPP_Process_FreeInBuf() This function is called by the component  Thread whenever it
* receives the a Freed Input buffer from the DSP
*
* @param pComponentPrivate  Component private data
 
* @pre 
*
* @post 
*
* @return none
*/
OMX_ERRORTYPE VPP_Process_FreeInBuf(VPP_COMPONENT_PRIVATE* pComponentPrivate,
                                    OMX_VPP_COMPONENT_BUFFER *pComponentBuf)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE* pHandle = (OMX_COMPONENTTYPE*)pComponentPrivate->pHandle;
    OMX_U32 nIndex;
    OMX_PARAM_PORTDEFINITIONTYPE *portDef = NULL;


    if (pComponentPrivate->toState == OMX_StateIdle) {
    VPP_DPRINT ("%d :: Entering HandleDataBuf_FromLCML Function\n",__LINE__); 
    }

    VPP_DPRINT("VPP::%d: Component Sending Empty Input buffer%p to App\n",__LINE__,pComponentBuf->pBufHeader->pBuffer);
    portDef = pComponentBuf->pBufHeader->pInputPortPrivate;
    
    eError = VPP_IsValidBuffer(pComponentBuf->pBufHeader, pComponentPrivate, portDef->nPortIndex, &nIndex);
    if (pComponentPrivate->toState == OMX_StateIdle) {
    VPP_DPRINT("VPP_Process_FreeInBuf: VPP_IsValidBuffer %d\n", eError);
    }
    if ( eError !=OMX_ErrorNone) {
        OMX_SET_ERROR_BAIL(eError, OMX_ErrorBadParameter);
    }
    /*If Tunneling*/
    if (pComponentPrivate->toState == OMX_StateIdle) {
    VPP_DPRINT("tunneling %p\n", pComponentPrivate->sCompPorts[pComponentBuf->pBufHeader->nInputPortIndex].hTunnelComponent);
    }
    if (pComponentPrivate->sCompPorts[pComponentBuf->pBufHeader->nInputPortIndex].hTunnelComponent != NULL) { 
        if (OMX_StateExecuting == pComponentPrivate->curState) {
            if ((!pComponentPrivate->bIsStopping) ||
                    (OMX_TRUE != pComponentPrivate->sCompPorts[portDef->nPortIndex].pVPPBufHeader[nIndex].bSelfAllocated)) {
                pComponentPrivate->sCompPorts[portDef->nPortIndex].pVPPBufHeader[nIndex].bHolding = OMX_FALSE;
                VPP_DPRINT ("VPP::Sending INput buffer to TUNNEL component (%d)\n", pComponentPrivate->bIsStopping);

#ifdef __PERF_INSTRUMENTATION__
                PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                pComponentBuf->pBufHeader->pBuffer,
                                0,
                                PERF_ModuleLLMM);
#endif
                if(pComponentBuf->eBufferOwner != VPP_BUFFER_CLIENT ){
                    pComponentBuf->eBufferOwner = VPP_BUFFER_CLIENT;
                    VPP_DPRINT("$$$VPP_UTILS: call to OMX_FillThisBuffer():: %d\n", __LINE__);
                    eError = OMX_FillThisBuffer(pComponentPrivate->sCompPorts[pComponentBuf->pBufHeader->nInputPortIndex].hTunnelComponent, pComponentBuf->pBufHeader);
                    VPP_DPRINT ("VPP:: buffer is sent to tunnel component\n");
                }
                else{
                    VPP_DPRINT("VPP:: buffer is already in tunnel component\n");
                }
            }
        }
    }
    else {
        VPP_DPRINT("pComponentPrivate->bIsEOFSent %d\n", pComponentPrivate->bIsEOFSent);
        if (1) { /* if (pComponentPrivate->bIsEOFSent != 1) { */
            pComponentPrivate->sCompPorts[portDef->nPortIndex].pVPPBufHeader[nIndex].bHolding = OMX_FALSE;

#ifdef __PERF_INSTRUMENTATION__
            PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                            PREF(pComponentBuf->pBufHeader,pBuffer),
                            0,
                            PERF_ModuleHLMM);
#endif

    if (pComponentPrivate->toState == OMX_StateIdle) {
            VPP_DPRINT("pComponentBuf->eBufferOwner %d (%p)\n", pComponentBuf->eBufferOwner, pComponentBuf->pBufHeader);
    }
            if(pComponentBuf->eBufferOwner != VPP_BUFFER_CLIENT){
                pComponentBuf->eBufferOwner = VPP_BUFFER_CLIENT;
                if (pComponentBuf->pBufHeader->pMarkData) {
                   VPP_DPRINT("return marked buffer %x %d\n", pComponentBuf->pBufHeader->pMarkData, pComponentBuf->pBufHeader->nInputPortIndex);
                }
                VPP_DPRINT("VPP:: Sent buffer to the client\n");
                pComponentPrivate->cbInfo.EmptyBufferDone (pHandle,
                                    pHandle->pApplicationPrivate,
                                    pComponentBuf->pBufHeader
                                    );
    if (pComponentPrivate->toState == OMX_StateIdle) {
                VPP_DPRINT("VPP:: Sent buffer to the client\n");
    }
           }
            else{
                VPP_DPRINT("VPP:: Buffer already with the client\n");
            }
        }
        else {
            VPP_DPRINT(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
            VPP_DPRINT("%d :: Comp: Last IP Buffer: So will not be sent to app\n", __LINE__);
            VPP_DPRINT(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
        }
    }
EXIT:
    return eError;
}



/**
* @VPP_ProcessFilledOutBuf() This function is called by the component  Thread whenever it
* receives the an Filled output buffer from the DSP
*
* @param pComponentPrivate  Component private data
 
* @pre 
*
* @post 
*
* @return none
*/
OMX_ERRORTYPE VPP_Process_FilledOutBuf(VPP_COMPONENT_PRIVATE* pComponentPrivate,
                                       OMX_VPP_COMPONENT_BUFFER *pComponentBuf)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE* pHandle = (OMX_COMPONENTTYPE*)pComponentPrivate->pHandle;
    OMX_U32 nIndex;
    OMX_PARAM_PORTDEFINITIONTYPE *portDef = NULL;

    VPP_DPRINT ("VPP %d :: Entering HandleDataBuf_FromLCML Function\n",__LINE__); 

    portDef = pComponentBuf->pBufHeader->pOutputPortPrivate;
    VPP_DPRINT("VPP::%d: Component Sending Filled Output buffer of index %lu to App\n",__LINE__,portDef->nPortIndex);
    eError = VPP_IsValidBuffer(pComponentBuf->pBufHeader, pComponentPrivate, portDef->nPortIndex, &nIndex);
    if ( eError !=OMX_ErrorNone){
        OMX_SET_ERROR_BAIL(eError, OMX_ErrorBadParameter);
    }

    if (pComponentBuf->pBufHeader->pMarkData && pComponentBuf->pBufHeader->hMarkTargetComponent == pComponentPrivate->pHandle) {
        VPP_DPRINT("Send OMX_MarkEvent\n");
        if (pComponentBuf->pBufHeader->nOutputPortIndex == OMX_VPP_YUV_OUTPUT_PORT) { 
            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                        pComponentPrivate->pHandle->pApplicationPrivate,
                                        OMX_EventMark,
                                        OMX_VPP_YUV_OUTPUT_PORT,
                                        0,
                                        pComponentBuf->pBufHeader->pMarkData);
            }
        else { /*OMX_VPP_RGB_OUTPUT_PORT*/
            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                        pComponentPrivate->pHandle->pApplicationPrivate,
                                        OMX_EventMark,
                                        OMX_VPP_RGB_OUTPUT_PORT,
                                        0,
                                        pComponentBuf->pBufHeader->pMarkData);
        }
    }

    if(pComponentBuf->pBufHeader->nFlags & OMX_BUFFERFLAG_EOS){
       VPP_DPRINT("set EOS flag at YUV output buffer\n");
       pComponentPrivate->cbInfo.EventHandler (pComponentPrivate->pHandle, 
                                        pComponentPrivate->pHandle->pApplicationPrivate,
                                        OMX_EventBufferFlag, 
                                        pComponentBuf->pBufHeader->nOutputPortIndex, 
                                        OMX_BUFFERFLAG_EOS, 
                                        NULL);
    }
    
    VPP_DPRINT("VPP: VPP_Process_FilledOutBuf: nPortIndex=%d, nIndex= %d, bHolding= %d\n", portDef->nPortIndex, nIndex, pComponentPrivate->sCompPorts[portDef->nPortIndex].pVPPBufHeader[nIndex].bHolding);
    
    /*TUNNEL HERE*/
    if (pComponentPrivate->sCompPorts[portDef->nPortIndex].hTunnelComponent != NULL) {
        pComponentPrivate->sCompPorts[portDef->nPortIndex].pVPPBufHeader[nIndex].bHolding = OMX_FALSE;
        

#ifdef __PERF_INSTRUMENTATION__
        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                        pComponentBuf->pBufHeader->pBuffer,
                        pComponentBuf->pBufHeader->nFilledLen,
                        PERF_ModuleLLMM);
#endif

        if((pComponentBuf->eBufferOwner != VPP_BUFFER_CLIENT) && (pComponentPrivate->toState != OMX_StateIdle)){
            VPP_DPRINT("VPP::Sending Output buffer to TUNNEL component\n");
            pComponentBuf->eBufferOwner = VPP_BUFFER_CLIENT;
            eError = OMX_EmptyThisBuffer(pComponentPrivate->sCompPorts[pComponentBuf->pBufHeader->nOutputPortIndex].hTunnelComponent, pComponentBuf->pBufHeader);
        }
        else{
            VPP_DPRINT("VPP:: Output buffer already with the TUNNEL component\n");
        }
#if 0
        FILE *fp;

        fp = fopen("mytestcv.yuv", "w");
        fwrite(pComponentBuf->pBufHeader->pBuffer, 1, pComponentBuf->pBufHeader->nFilledLen, fp);
        fclose(fp);
#endif
    }
    else {

        pComponentPrivate->sCompPorts[portDef->nPortIndex].pVPPBufHeader[nIndex].bHolding = OMX_FALSE;

#ifdef __PERF_INSTRUMENTATION__
        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                        PREF(pComponentBuf->pBufHeader,pBuffer),
                        PREF(pComponentBuf->pBufHeader,nFilledLen),
                        PERF_ModuleHLMM);
#endif

        if(pComponentBuf->eBufferOwner != VPP_BUFFER_CLIENT){
            VPP_DPRINT("VPP::Sending Output buffer to Applcation %p (%p %p)\n", pComponentBuf->pBufHeader, pComponentBuf->pBufHeader->hMarkTargetComponent, pComponentBuf->pBufHeader->pMarkData);
            pComponentBuf->eBufferOwner = VPP_BUFFER_CLIENT;
            pComponentPrivate->cbInfo.FillBufferDone (
                                pHandle, 
                                pHandle->pApplicationPrivate,
                                pComponentBuf->pBufHeader
                                );
        }
        else{
            VPP_DPRINT("VPP:: Buffer already with the client\n");
        }
        
    }

EXIT:
    VPP_DPRINT ("VPP::%d :: VPP_Process_FilledOutBuf Function with eError %d\n",__LINE__, eError); 
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
OMX_ERRORTYPE VPP_LCML_Callback (TUsnCodecEvent event,void * args [10])
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U8 *pBuffer = args[1];
    OMX_VPP_COMPONENT_BUFFER *pComponentBuf = NULL;

    VPP_COMPONENT_PRIVATE* pComponentPrivate = NULL;
    OMX_COMPONENTTYPE* pHandle = NULL;
    LCML_DSP_INTERFACE* pLcmlDspInterface = NULL;
    VPP_BUFFERDATA_PROPAGATION *pDataProp = NULL;
    OMX_U8 i = 0;

    if (args[6]) { 
        pLcmlDspInterface = (LCML_DSP_INTERFACE*)args[6];
        
        pComponentPrivate = (VPP_COMPONENT_PRIVATE*)pLcmlDspInterface->pComponentPrivate;
        
        pHandle = (OMX_COMPONENTTYPE *)pComponentPrivate->pHandle;
        
    }
    else {
        VPP_DPRINT("wrong in LCML callback, exit\n");
        goto EXIT;
    }

    VPP_DPRINT ("VPP::%d :: Entering the LCML_Callback Function, event = %d\n",__LINE__, event);

    switch (event) 
    {
    case EMMCodecBufferProcessed:
        switch ((int)args[0]) 
        {
        case EMMCodecInputBuffer:
            VPP_DPRINT ("VPP :: Inside the LCML_Callback EMMCodecInputBuffer\n");
            VPP_DPRINT("VPP::%d :: Input: pBufferr = %p\n",__LINE__, pBuffer);
            eError = VPP_GetCorresponding_LCMLHeader(pComponentPrivate, pBuffer, OMX_DirInput, &pComponentBuf, 0);
            if (eError != OMX_ErrorNone) {
                VPP_DPRINT("VPP::%d :: Error: Invalid Buffer Came ...\n",__LINE__);
                goto EXIT;
            }
            VPP_DPRINT("VPP::%d :: Input: pLcmlHeader = %p.\n",__LINE__, pComponentBuf->pBufHeader);

            if (pComponentPrivate->bFlushComplete == OMX_FALSE && pComponentPrivate->nFlushPort == 0) {
                pComponentBuf->eBufferOwner = VPP_BUFFER_CLIENT;
                pComponentPrivate->cbInfo.EmptyBufferDone (pComponentPrivate->pHandle,
                                    pComponentPrivate->pHandle->pApplicationPrivate,
                                    pComponentBuf->pBufHeader
                                    );
                break;
            }
#ifdef __PERF_INSTRUMENTATION__
            PERF_ReceivedFrame(pComponentPrivate->pPERFcomp,
                                PREF(pComponentBuf->pBufHeader,pBuffer),
                                0,
                                PERF_ModuleCommonLayer);
#endif

            pComponentBuf->eBufferOwner = VPP_BUFFER_COMPONENT_OUT;

            /*Freed Input buffers from DSP to component*/
            eError = VPP_Process_FreeInBuf(pComponentPrivate, pComponentBuf);
            if (eError != OMX_ErrorNone) {
                pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                    pComponentPrivate->pHandle->pApplicationPrivate,
                                    OMX_EventError,
                                    OMX_ErrorUndefined,
                                    OMX_TI_ErrorSevere,
                                    NULL);
                goto EXIT;
            }
            break;
        case EMMCodecStream1:
            VPP_DPRINT ("VPP:: Inside the LCML_Callback EMMCodecInputBuffer Overlay\n");
            VPP_DPRINT("VPP::%d :: Overlay: pBuffer = %p\n",__LINE__, pBuffer);
            eError = VPP_GetCorresponding_LCMLHeader(pComponentPrivate, pBuffer, OMX_DirInput, &pComponentBuf,1);
            if (eError != OMX_ErrorNone) {
                VPP_DPRINT("VPP::%d :: Error: Invalid Buffer Came ...\n",__LINE__);
                goto EXIT;
            }
            VPP_DPRINT("VPP::%d :: Input: pLcmlHeader = %p\n",__LINE__, pComponentBuf);
            VPP_DPRINT("VPP::%d :: Overlay: pLcmlHeader = %p.\n",__LINE__, pComponentBuf->pBufHeader);

            if (pComponentPrivate->bFlushComplete == OMX_FALSE && pComponentPrivate->nFlushPort == 1) {
                pComponentBuf->eBufferOwner = VPP_BUFFER_CLIENT;
                pComponentPrivate->cbInfo.EmptyBufferDone (pComponentPrivate->pHandle,
                                    pComponentPrivate->pHandle->pApplicationPrivate,
                                    pComponentBuf->pBufHeader
                                    );
                break;
            }
#ifdef __PERF_INSTRUMENTATION__
            PERF_ReceivedFrame(pComponentPrivate->pPERFcomp,
                            PREF(pComponentBuf->pBufHeader,pBuffer),
                            0,
                            PERF_ModuleCommonLayer);
#endif
            pComponentBuf->eBufferOwner = VPP_BUFFER_COMPONENT_OUT;

            /*Freed Input buffers from DSP to component*/
            eError = VPP_Process_FreeInBuf(pComponentPrivate, pComponentBuf);
            if (eError != OMX_ErrorNone) {
                pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                    pComponentPrivate->pHandle->pApplicationPrivate,
                                    OMX_EventError,
                                    OMX_ErrorUndefined,
                                    OMX_TI_ErrorSevere,
                                    NULL);
                goto EXIT;
            }
            break;
        case EMMCodecStream2:
            VPP_DPRINT("VPP :: Inside the LCML_Callback EMMCodecOuputBuffer stream2 \n");
            VPP_DPRINT("VPP::%d :: Output: pBufferr = %p\n",__LINE__, pBuffer);
            eError = VPP_GetCorresponding_LCMLHeader(pComponentPrivate, pBuffer, OMX_DirOutput, &pComponentBuf, 2);
            if (eError != OMX_ErrorNone) {
                VPP_DPRINT("VPP::%d :: Error: Invalid Buffer Came ...\n",__LINE__);
                goto EXIT;
            }
            pComponentBuf->pBufHeader->nFilledLen = (int)args[8]; 
            VPP_DPRINT("VPP::%d :: Output(2): pLcmlHeader = %p\n",__LINE__, pComponentBuf);
            VPP_DPRINT("VPP::%d :: Output: Filled Len = %ld\n",__LINE__, pComponentBuf->pBufHeader->nFilledLen);

            if(pComponentBuf->eBufferOwner == VPP_BUFFER_DSP){
                pComponentPrivate->nOutRGBBufferCount ++;
            }
            VPP_DPRINT("RGB Filled Data from DSP \n");
            VPP_DPRINT("buffer summary (LCML for output buffer %p) %d %d %d %d\n", pComponentBuf->pBufHeader, 
                    pComponentPrivate->nInYUVBufferCount,
                    pComponentPrivate->nInRGBBufferCount,
                    pComponentPrivate->nOutYUVBufferCount,
                    pComponentPrivate->nOutRGBBufferCount);


            for (i = 0; i < pComponentPrivate->sCompPorts[OMX_VPP_INPUT_PORT].pPortDef.nBufferCountActual; i ++) {
                pDataProp = &(pComponentPrivate->sCompPorts[OMX_VPP_INPUT_PORT].sBufferDataProp[i]);
                if (pDataProp->buffer_idRGB == pComponentPrivate->nOutRGBBufferCount) {
                    VPP_DPRINT("Output RGB buffer %d has data from Input port. \nFlag=%x, nTickCount=%ld, nTimeStamp=%Ld\n", 
                               pDataProp->buffer_idRGB, 
                               pDataProp->flag,
                               pDataProp->nTickCount,
                               pDataProp->nTimeStamp);
                    pComponentBuf->pBufHeader->nFlags = pDataProp->flag;
                    pComponentBuf->pBufHeader->pMarkData = pDataProp->pMarkData;
                    pComponentBuf->pBufHeader->hMarkTargetComponent = pDataProp->hMarkTargetComponent;
                    pComponentBuf->pBufHeader->nTickCount = pDataProp->nTickCount;
                    pComponentBuf->pBufHeader->nTimeStamp = pDataProp->nTimeStamp;
                    pDataProp->buffer_idRGB = 0xFFFFFFFF;
                    break;
                }
            }

            for (i = 0; i < pComponentPrivate->sCompPorts[OMX_VPP_INPUT_OVERLAY_PORT].pPortDef.nBufferCountActual; i ++) {
                pDataProp = &(pComponentPrivate->sCompPorts[OMX_VPP_INPUT_OVERLAY_PORT].sBufferDataProp[i]);
                if (pDataProp->buffer_idRGB == pComponentPrivate->nOutRGBBufferCount) {
                    VPP_DPRINT("Output RGB buffer %d has data from Overlay port. \nFlag=%x, nTickCount=%ld, nTimeStamp=%Ld\n", 
                               pDataProp->buffer_idRGB, 
                               pDataProp->flag,
                               pDataProp->nTickCount,
                               pDataProp->nTimeStamp);
                    pComponentBuf->pBufHeader->nFlags |= pDataProp->flag;
                    /*if both input ports are been mark RGB output port propagate Input overlay mark*/
                    pComponentBuf->pBufHeader->pMarkData = pDataProp->pMarkData;
                    pComponentBuf->pBufHeader->hMarkTargetComponent = pDataProp->hMarkTargetComponent;
#if 0
                    if(pComponentBuf->pBufHeader->hMarkTargetComponent == NULL){ /*OMX_VPP_INPUT_PORT has preference while marking data*/
                        pComponentBuf->pBufHeader->pMarkData = pDataProp->pMarkData;
                        pComponentBuf->pBufHeader->hMarkTargetComponent = pDataProp->hMarkTargetComponent;
                    }
#endif
                    pComponentBuf->pBufHeader->nTickCount = pDataProp->nTickCount;
                    pComponentBuf->pBufHeader->nTimeStamp = pDataProp->nTimeStamp;
                    pDataProp->buffer_idRGB = 0xFFFFFFFF;
                    break;
                }
            }

            if (pComponentPrivate->bFlushComplete == OMX_FALSE && pComponentPrivate->nFlushPort == 2) {
                pComponentBuf->eBufferOwner = VPP_BUFFER_CLIENT;
                pComponentPrivate->cbInfo.FillBufferDone (pComponentPrivate->pHandle,
                                    pComponentPrivate->pHandle->pApplicationPrivate,
                                    pComponentBuf->pBufHeader
                                    );
                break;
            }

#ifdef __PERF_INSTRUMENTATION__
            PERF_ReceivedFrame(pComponentPrivate->pPERFcomp,
                            pComponentBuf->pBufHeader->pBuffer,
                            pComponentBuf->pBufHeader->nFilledLen,
                            PERF_ModuleCommonLayer);
                            pComponentPrivate->lcml_nCntOpReceived++; /*CRITICAL: increment Op counter!!! */
            if ((pComponentPrivate->lcml_nCntIp >= 1) &&
                    (pComponentPrivate->lcml_nCntOpReceived == 1)) {
                PERF_Boundary(pComponentPrivate->pPERFcomp,
                            PERF_BoundaryStart | PERF_BoundarySteadyState);
            }
#endif

            pComponentBuf->eBufferOwner = VPP_BUFFER_COMPONENT_OUT;

            /* Filled Output buffer from DSP to Component */
            eError = VPP_Process_FilledOutBuf(pComponentPrivate, pComponentBuf);
            if (eError != OMX_ErrorNone) {
                pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                                       OMX_EventError,
                                                       OMX_ErrorUndefined,
                                                       OMX_TI_ErrorSevere,
                                                       NULL);
                goto EXIT;
            }
            break;
        case EMMCodecStream3:
            VPP_DPRINT ("VPP::Inside the LCML_Callback EMMCodecOuputBuffer stream3\n");
            VPP_DPRINT("VPP::%d :: Output: pBufferr = %p\n",__LINE__, pBuffer);
            eError = VPP_GetCorresponding_LCMLHeader(pComponentPrivate, pBuffer, OMX_DirOutput, &pComponentBuf,3);
            if (eError != OMX_ErrorNone) {
                VPP_DPRINT("VPP::%d :: Error: Invalid Buffer Came ...\n",__LINE__);
                goto EXIT;
            }
            pComponentBuf->pBufHeader->nFilledLen = (int)args[8]; 
            VPP_DPRINT("VPP::%d :: Output(3): pLcmlHeader = %p\n",__LINE__, pComponentBuf);
            VPP_DPRINT("VPP::%d :: Output: Filled Len = %ld\n",__LINE__, pComponentBuf->pBufHeader->nFilledLen);

            if(pComponentBuf->eBufferOwner == VPP_BUFFER_DSP){
                pComponentPrivate->nOutYUVBufferCount ++;
            }
            VPP_DPRINT("YUV Filled Data from DSP \n");
            VPP_DPRINT("buffer summary (LCML for output buffer %p) %d %d %d %d\n", pComponentBuf->pBufHeader, 
                    pComponentPrivate->nInYUVBufferCount,
                    pComponentPrivate->nInRGBBufferCount,
                    pComponentPrivate->nOutYUVBufferCount,
                    pComponentPrivate->nOutRGBBufferCount);
            
            for (i = 0; i < pComponentPrivate->sCompPorts[OMX_VPP_INPUT_PORT].pPortDef.nBufferCountActual; i ++) {
                pDataProp = &(pComponentPrivate->sCompPorts[OMX_VPP_INPUT_PORT].sBufferDataProp[i]);
                if (pDataProp->buffer_idYUV == pComponentPrivate->nOutYUVBufferCount) {
                    VPP_DPRINT("Output YUV buffer %d has data from Input port. \nFlag=%x, nTickCount=%ld, nTimeStamp=%Ld\n\n", 
                               pDataProp->buffer_idYUV, 
                               pDataProp->flag,
                               pDataProp->nTickCount,
                               pDataProp->nTimeStamp);

                    pComponentBuf->pBufHeader->nFlags = pDataProp->flag;
                    pComponentBuf->pBufHeader->pMarkData = pDataProp->pMarkData;
                    pComponentBuf->pBufHeader->hMarkTargetComponent = pDataProp->hMarkTargetComponent;
                    pComponentBuf->pBufHeader->nTickCount = pDataProp->nTickCount;
                    pComponentBuf->pBufHeader->nTimeStamp = pDataProp->nTimeStamp;
                    pDataProp->buffer_idYUV = 0xFFFFFFFF;
                    break;
                }
            }

            for (i = 0; i < pComponentPrivate->sCompPorts[OMX_VPP_INPUT_OVERLAY_PORT].pPortDef.nBufferCountActual; i ++) {
                pDataProp = &(pComponentPrivate->sCompPorts[OMX_VPP_INPUT_OVERLAY_PORT].sBufferDataProp[i]);
                if (pDataProp->buffer_idYUV == pComponentPrivate->nOutYUVBufferCount) {
                    VPP_DPRINT("Output YUV buffer %d has data from Overlay port. \nFlag=%x, nTickCount=%ld, nTimeStamp=%Ld\n\n", 
                               pDataProp->buffer_idYUV, 
                               pDataProp->flag,
                               pDataProp->nTickCount,
                               pDataProp->nTimeStamp);
                    pComponentBuf->pBufHeader->nFlags |= pDataProp->flag;
                    if(pComponentBuf->pBufHeader->hMarkTargetComponent == NULL){ /*OMX_VPP_INPUT_PORT has preference while marking data*/
                        pComponentBuf->pBufHeader->pMarkData = pDataProp->pMarkData;
                        pComponentBuf->pBufHeader->hMarkTargetComponent = pDataProp->hMarkTargetComponent;
                    }
                    pComponentBuf->pBufHeader->nTickCount = pDataProp->nTickCount;
                    pComponentBuf->pBufHeader->nTimeStamp = pDataProp->nTimeStamp;
                    pDataProp->buffer_idYUV = 0xFFFFFFFF;
                    break;
                }
            }


            if (pComponentPrivate->bFlushComplete == OMX_FALSE && pComponentPrivate->nFlushPort == 3) {
                pComponentBuf->eBufferOwner = VPP_BUFFER_CLIENT;
                pComponentPrivate->cbInfo.FillBufferDone (pComponentPrivate->pHandle,
                                    pComponentPrivate->pHandle->pApplicationPrivate,
                                    pComponentBuf->pBufHeader
                                    );
                break;
            }
            
            #ifdef __PERF_INSTRUMENTATION__
            PERF_ReceivedFrame(pComponentPrivate->pPERFcomp,
                            pComponentBuf->pBufHeader->pBuffer,
                            pComponentBuf->pBufHeader->nFilledLen,
                            PERF_ModuleCommonLayer);
            pComponentPrivate->lcml_nCntOpReceived++; /*CRITICAL: increment Op counter!!! */
            if ((pComponentPrivate->lcml_nCntIp >= 1) &&
                    (pComponentPrivate->lcml_nCntOpReceived == 1)) {
                PERF_Boundary(pComponentPrivate->pPERFcomp,
                            PERF_BoundaryStart | PERF_BoundarySteadyState);
            }
#endif

           pComponentBuf->eBufferOwner = VPP_BUFFER_COMPONENT_OUT;

            /* Filled Output buffer from DSP to Component */
            eError = VPP_Process_FilledOutBuf(pComponentPrivate, pComponentBuf);
            if (eError != OMX_ErrorNone) {
                pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                                       OMX_EventError,
                                                       OMX_ErrorUndefined,
                                                       OMX_TI_ErrorSevere,
                                                       NULL);
                goto EXIT;
            }
            break;
        }
        break;
    case EMMCodecProcessingStoped:
        VPP_DPRINT("VPP::%d :: Comp: Inside the LCML_Callback: EMMCodecProcessingStopped\n",__LINE__);
        VPP_DPRINT("VPP::%d :: VPP: State has been Set to Idle\n",__LINE__);
        if (pComponentPrivate->toState == OMX_StateIdle) {
            pComponentPrivate->ExeToIdleFlag |= VPP_DSPSTOP;
            VPP_DPRINT("LCML_Callback: pComponentPrivate->ExeToIdleFlag  = %x\n", pComponentPrivate->ExeToIdleFlag);

            pthread_mutex_lock(&pComponentPrivate->vpp_mutex);
            pthread_cond_signal(&pComponentPrivate->stop_cond);
            pthread_mutex_unlock(&pComponentPrivate->vpp_mutex);

        } else {
            pComponentPrivate->bDisable = OMX_TRUE;
        }
        break;
    case EMMCodecDspError:
        VPP_DPRINT("VPP::LCML_Callback.  Received EMMCodecDSPError\n");
        VPP_DPRINT("EMMCodec Args -> %x, %x, %x\n", (int)args[0], (int)args[4], (int)args[5]);
        if ((int)args[4] != 0x1 || (int)args[5] != 0x500) {
           pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                     pComponentPrivate->pHandle->pApplicationPrivate,
                                     OMX_EventError, 
                                     OMX_ErrorHardware, 
                                     OMX_TI_ErrorCritical,
                                     NULL);
           
            pComponentPrivate->curState  = OMX_StateInvalid;
            pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                   pHandle->pApplicationPrivate,
                                                   OMX_EventError,
                                                   OMX_ErrorInvalidState, 
                                                   OMX_TI_ErrorCritical,
                                                   "DSP Hardware Error");
            goto EXIT;
       }
#ifdef DSP_MMU_FAULT_HANDLING
        /* Cheking for MMU_fault */
        if((args[4] == (void *)NULL) && (args[5] == (void*)NULL)) {
            VPP_DPRINT("DSP MMU_Fault");
            pComponentPrivate->curState = OMX_StateInvalid;
            pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                   pHandle->pApplicationPrivate,
                                                   OMX_EventError,
                                                   OMX_ErrorInvalidState, 
                                                   OMX_TI_ErrorCritical,
                                                   "DSP MMU FAULT");
        }
#endif
        break;
    case EMMCodecInternalError:
        VPP_DPRINT("VPP::LCML_Callback.  EMMCodecInternalError\n");
        eError = OMX_ErrorHardware;
        pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                            pComponentPrivate->pHandle->pApplicationPrivate,
                            OMX_EventError, 
                            OMX_ErrorHardware, 
                            OMX_TI_ErrorCritical,
                            NULL);
        goto EXIT;
        break;
    case EMMCodecInitError:
        VPP_DPRINT("VPP::LCML_Callback.  EMMCodecInitError\n");
        eError = OMX_ErrorHardware;
        pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                            pComponentPrivate->pHandle->pApplicationPrivate,
                            OMX_EventError, 
                            OMX_ErrorHardware, 
                            OMX_TI_ErrorCritical,
                            NULL);
        goto EXIT;
        break;
    case EMMCodecDspMessageRecieved:
        VPP_DPRINT("VPP::LCML_Callback.  EMMCodecDspMessageReceived\n");
        break;
    case EMMCodecProcessingStarted:
        VPP_DPRINT("VPP::LCML_Callback.  EMMCodecProcessingStarted\n");
        break;
    case EMMCodecProcessingPaused:
        VPP_DPRINT("VPP::LCML_Callback.  EMMCodecProcessingPaused\n");
        if (pComponentPrivate->toState == OMX_StatePause) {
            pComponentPrivate->curState = OMX_StatePause;
            VPP_DPRINT ("%d :: The component %p is paused after get stop from DSP\n",__LINE__, pHandle);
            VPP_DPRINT("LCML_Callback: pComponentPrivate->ExeToIdleFlag  = %x\n", pComponentPrivate->ExeToIdleFlag);

            pComponentPrivate->cbInfo.EventHandler (
                                pHandle,
                                pHandle->pApplicationPrivate,
                                OMX_EventCmdComplete,
                                OMX_ErrorNone,
                                pComponentPrivate->curState,
                                "NULL");
        }
        break;
    case EMMCodecProcessingEof:
        VPP_DPRINT("VPP::LCML_Callback.  EMMCodecProcessingEof\n");
        break;
    case EMMCodecBufferNotProcessed:
        VPP_DPRINT("VPP::LCML_Callback.  EMMCodecBufferNotProcessed\n");
        break;
    case EMMCodecAlgCtrlAck:
        VPP_DPRINT("VPP::LCML_Callback.  EMMCodecAlgCtrlAck\n");
        pComponentPrivate->CodecAlgCtrlAck = 1;
        break;
    case EMMCodecStrmCtrlAck:
        VPP_DPRINT("VPP::LCML_Callback.  EMMCodecStrmCtrlAck\n");
#if 1
        if (1) { /* ((int)args[0] == USN_ERR_NONE) { */
            VPP_DPRINT("Callback: EMMCodecStrmCtrlAck\n");
            pComponentPrivate->bFlushComplete = OMX_TRUE;
        } else {
            VPP_DPRINT("callback error %x\n", args[0]);
        }
#endif
        break;
    default:
        VPP_DPRINT ("VPP::Comp: Inside the LCML_Callback: EVENT UNKNOWN %d\n", event);
        break;
    }

EXIT:
    VPP_DPRINT ("VPP::%d :: Exiting the LCML_Callback Function\n",__LINE__);
    return eError;
}



/* -------------------------------------------------------------------*/
/**
  *  VPP_GetCorresponding_LCMLHeader() function retrun correponding Parameter buffer stored
  *
  * @param pBuffer                 This buffer will be returned by the LCML
           eDir
       ppLcmlHdr               pointer where LCML header is returned
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE VPP_GetCorresponding_LCMLHeader(VPP_COMPONENT_PRIVATE* pComponentPrivate,
                                              OMX_U8 *pBuffer, 
                                              OMX_DIRTYPE eDir,
                                              OMX_VPP_COMPONENT_BUFFER** ppCmpBuf,
                                              OMX_U32 Index)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_VPP_COMPONENT_BUFFER* pComponentBuffer = NULL;
    int i = 0 ;
    int nBuf = pComponentPrivate->sCompPorts[Index].nBufferCount;

    VPP_DPRINT("VPP:: Buffer Count :: %ld\n",nBuf);
  
    VPP_DPRINT("VPP:: Index of  Buffer Type :: %ld\n",Index);
  
    for (i=0; i<nBuf; i++) {
        pComponentBuffer = &pComponentPrivate->sCompPorts[Index].pVPPBufHeader[i];
        if (pBuffer == pComponentBuffer->pBufHeader->pBuffer) {
            *ppCmpBuf = pComponentBuffer;
            VPP_DPRINT("VPP::%d::Corresponding LCML Header Found\n",__LINE__);
            goto EXIT;
        }
    }

    VPP_DPRINT("VPP: %d, Haven't found the header...\n", __LINE__);
    eError = OMX_ErrorMax;
EXIT:
    return eError;
}



/* -------------------------------------------------------------------*/
/**
  *  GetLCMLHandle() function will be called to load LCML component 
  *
  * 
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_ErrorUndefined    The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/
OMX_HANDLETYPE VPP_GetLCMLHandle(VPP_COMPONENT_PRIVATE* pComponentPrivate)
{
#ifndef UNDER_CE 
    void *handle;
    OMX_ERRORTYPE (*fpGetHandle)(OMX_HANDLETYPE);
    OMX_HANDLETYPE pHandle = NULL;
    char *error = NULL;
    OMX_ERRORTYPE eError;

    handle = dlopen("libLCML.so", RTLD_LAZY);
    if (!handle) {
        fputs(dlerror(), stderr);
        goto EXIT;
    }
    fpGetHandle = dlsym (handle, "GetHandle");
    if ((error = dlerror()) != NULL) {
        if(fpGetHandle){
                dlclose(handle);
                handle = NULL;
            }
        fputs(error, stderr);
        goto EXIT;
    }
    eError = (*fpGetHandle)(&pHandle);
    if(eError != OMX_ErrorNone) {
        eError = OMX_ErrorUndefined;
        VPP_DPRINT("eError != OMX_ErrorNone...\n");
        pHandle = NULL;
        goto EXIT;
    }
    pComponentPrivate->pDllHandle = handle;
    pComponentPrivate->pLCML = (void*)pHandle;
    pComponentPrivate->pLCML->pComponentPrivate = (VPP_COMPONENT_PRIVATE *)pComponentPrivate;

EXIT:
    return pHandle;
#else

    typedef OMX_ERRORTYPE (*LPFNDLLFUNC1)(OMX_HANDLETYPE);
    OMX_HANDLETYPE pHandle = NULL;
    OMX_ERRORTYPE eError;
    LPFNDLLFUNC1 fpGetHandle1;

    g_hLcmlDllHandle = LoadLibraryEx(TEXT("OAF_BML.dll"), NULL, 0);
    if (g_hLcmlDllHandle == NULL) {
        VPP_DPRINT("BML Load Failed!!!\n");
        return pHandle;
    }

    fpGetHandle1 = (LPFNDLLFUNC1)GetProcAddress(g_hLcmlDllHandle,TEXT("GetHandle"));
    if (!fpGetHandle1) {
        FreeLibrary(g_hLcmlDllHandle);
        g_hLcmlDllHandle = NULL;
        return pHandle;
    }

    eError = fpGetHandle1(&pHandle);
    if(eError != OMX_ErrorNone) {
        FreeLibrary(g_hLcmlDllHandle);
        g_hLcmlDllHandle = NULL;
        eError = OMX_ErrorUndefined;
        VPP_DPRINT("eError != OMX_ErrorNone...\n");
        pHandle = NULL;
        goto EXIT;
    }
    
    (LCML_DSP_INTERFACE*)pComponentPrivate->pLCML = (LCML_DSP_INTERFACE*)pHandle;
    pComponentPrivate->pLCML->pComponentPrivate = (VPP_COMPONENT_PRIVATE *)pComponentPrivate;
EXIT:
    return pHandle;
#endif
}



OMX_ERRORTYPE VPP_Initialize_PrivateStruct(VPP_COMPONENT_PRIVATE *pComponentPrivate)
{
    int port;
    int buffers;
    
    OMX_ERRORTYPE eError=OMX_ErrorNone;

    OMX_INIT_STRUCT(pComponentPrivate->pPortParamTypeVideo, OMX_PORT_PARAM_TYPE);
    pComponentPrivate->pPortParamTypeVideo->nPorts           = NUM_OF_VPP_PORTS;
    pComponentPrivate->pPortParamTypeVideo->nStartPortNumber = 0;
    
    OMX_INIT_STRUCT(pComponentPrivate->pPortParamTypeImage, OMX_PORT_PARAM_TYPE);
    pComponentPrivate->pPortParamTypeImage->nPorts           = 0;
    pComponentPrivate->pPortParamTypeImage->nStartPortNumber = -1;
    
    OMX_INIT_STRUCT(pComponentPrivate->pPortParamTypeAudio, OMX_PORT_PARAM_TYPE);
    pComponentPrivate->pPortParamTypeAudio->nPorts           = 0;
    pComponentPrivate->pPortParamTypeAudio->nStartPortNumber = -1;
    
    OMX_INIT_STRUCT(pComponentPrivate->pPortParamTypeOthers, OMX_PORT_PARAM_TYPE);
    pComponentPrivate->pPortParamTypeOthers->nPorts           = 0;
    pComponentPrivate->pPortParamTypeOthers->nStartPortNumber = -1;
    OMX_INIT_STRUCT(pComponentPrivate->pCrop, OMX_CONFIG_RECTTYPE);
    pComponentPrivate->pCrop->nWidth = DEFAULT_WIDTH;
    pComponentPrivate->pCrop->nHeight = 220;
    
    /* Set component version */
    pComponentPrivate->ComponentVersion.s.nVersionMajor = VPP_MAJOR_VER;
    pComponentPrivate->ComponentVersion.s.nVersionMinor = VPP_MINOR_VER;
    pComponentPrivate->ComponentVersion.s.nRevision     = VPP_REVISION;
    pComponentPrivate->ComponentVersion.s.nStep         = VPP_STEP;

    
    /* Set Default values for each port supports qcif size and two streams */
    pComponentPrivate->sCompPorts[OMX_VPP_INPUT_PORT].pPortDef.nPortIndex         = OMX_VPP_INPUT_PORT;
    pComponentPrivate->sCompPorts[OMX_VPP_INPUT_PORT].pPortDef.eDir               = OMX_DirInput;
    pComponentPrivate->sCompPorts[OMX_VPP_INPUT_PORT].pPortDef.eDomain            = OMX_PortDomainVideo;
    pComponentPrivate->sCompPorts[OMX_VPP_INPUT_PORT].pPortDef.nBufferCountActual = MIN_NUM_OF_VPP_BUFFERS;
    pComponentPrivate->sCompPorts[OMX_VPP_INPUT_PORT].pPortDef.nBufferCountMin    = MIN_NUM_OF_VPP_BUFFERS;
    pComponentPrivate->sCompPorts[OMX_VPP_INPUT_PORT].pPortDef.nBufferSize        = DEFAULT_WIDTH * 220*1.5;
    pComponentPrivate->sCompPorts[OMX_VPP_INPUT_PORT].pPortDef.bEnabled           = OMX_TRUE;
    pComponentPrivate->sCompPorts[OMX_VPP_INPUT_PORT].pPortDef.bPopulated         = OMX_FALSE;
    pComponentPrivate->sCompPorts[OMX_VPP_INPUT_PORT].pPortDef.format.video.nFrameWidth  = DEFAULT_WIDTH;
    pComponentPrivate->sCompPorts[OMX_VPP_INPUT_PORT].pPortDef.format.video.nFrameHeight = 220;
    pComponentPrivate->sCompPorts[OMX_VPP_INPUT_PORT].pPortDef.format.video.nStride = 176;
    pComponentPrivate->sCompPorts[OMX_VPP_INPUT_PORT].pPortDef.format.video.nSliceHeight = 16;
    pComponentPrivate->sCompPorts[OMX_VPP_INPUT_PORT].pPortDef.format.video.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;/*OMX_COLOR_FormatCbYCrY;*/
    pComponentPrivate->sCompPorts[OMX_VPP_INPUT_PORT].pPortDef.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    pComponentPrivate->sCompPorts[OMX_VPP_INPUT_PORT].eSupplierSetting            = OMX_BufferSupplyInput;
    pComponentPrivate->sCompPorts[OMX_VPP_INPUT_PORT].hTunnelComponent            = NULL;
    pComponentPrivate->sCompPorts[OMX_VPP_INPUT_PORT].nReturnedBufferCount        = 0;
    pComponentPrivate->sCompPorts[OMX_VPP_INPUT_PORT].eMirror                           = OMX_MirrorNone;

    pComponentPrivate->sCompPorts[OMX_VPP_INPUT_OVERLAY_PORT].pPortDef.nPortIndex  = OMX_VPP_INPUT_OVERLAY_PORT;
    pComponentPrivate->sCompPorts[OMX_VPP_INPUT_OVERLAY_PORT].pPortDef.eDir        = OMX_DirInput;
    pComponentPrivate->sCompPorts[OMX_VPP_INPUT_OVERLAY_PORT].pPortDef.eDomain     = OMX_PortDomainVideo;
    pComponentPrivate->sCompPorts[OMX_VPP_INPUT_OVERLAY_PORT].pPortDef.nBufferCountActual = MIN_NUM_OF_VPP_BUFFERS;
    pComponentPrivate->sCompPorts[OMX_VPP_INPUT_OVERLAY_PORT].pPortDef.nBufferCountMin    = MIN_NUM_OF_VPP_BUFFERS;
    pComponentPrivate->sCompPorts[OMX_VPP_INPUT_OVERLAY_PORT].pPortDef.nBufferSize =  DEFAULT_HEIGHT *DEFAULT_WIDTH * 3;
    pComponentPrivate->sCompPorts[OMX_VPP_INPUT_OVERLAY_PORT].pPortDef.bEnabled    = OMX_TRUE;
    pComponentPrivate->sCompPorts[OMX_VPP_INPUT_OVERLAY_PORT].pPortDef.bPopulated  = OMX_FALSE;
    pComponentPrivate->sCompPorts[OMX_VPP_INPUT_OVERLAY_PORT].pPortDef.format.video.nFrameWidth  = DEFAULT_WIDTH;
    pComponentPrivate->sCompPorts[OMX_VPP_INPUT_OVERLAY_PORT].pPortDef.format.video.nFrameHeight = DEFAULT_HEIGHT;
    pComponentPrivate->sCompPorts[OMX_VPP_INPUT_OVERLAY_PORT].pPortDef.format.video.eColorFormat = OMX_COLOR_Format24bitRGB888;
    pComponentPrivate->sCompPorts[OMX_VPP_INPUT_OVERLAY_PORT].pPortDef.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    pComponentPrivate->sCompPorts[OMX_VPP_INPUT_OVERLAY_PORT].eSupplierSetting            = OMX_BufferSupplyInput;
    pComponentPrivate->sCompPorts[OMX_VPP_INPUT_OVERLAY_PORT].hTunnelComponent            = NULL;
    
    pComponentPrivate->sCompPorts[OMX_VPP_INPUT_OVERLAY_PORT].nReturnedBufferCount = 0; 
    pComponentPrivate->sCompPorts[OMX_VPP_INPUT_OVERLAY_PORT].eMirror                           = OMX_MirrorNone;
    
    pComponentPrivate->sCompPorts[OMX_VPP_RGB_OUTPUT_PORT].pPortDef.nPortIndex  = OMX_VPP_RGB_OUTPUT_PORT;
    pComponentPrivate->sCompPorts[OMX_VPP_RGB_OUTPUT_PORT].pPortDef.eDir        = OMX_DirOutput;
    pComponentPrivate->sCompPorts[OMX_VPP_RGB_OUTPUT_PORT].pPortDef.eDomain     = OMX_PortDomainVideo;
    pComponentPrivate->sCompPorts[OMX_VPP_RGB_OUTPUT_PORT].pPortDef.nBufferCountActual =MIN_NUM_OF_VPP_BUFFERS;
    pComponentPrivate->sCompPorts[OMX_VPP_RGB_OUTPUT_PORT].pPortDef.nBufferCountMin =  MIN_NUM_OF_VPP_BUFFERS;
    pComponentPrivate->sCompPorts[OMX_VPP_RGB_OUTPUT_PORT].pPortDef.nBufferSize = DEFAULT_HEIGHT *DEFAULT_WIDTH *2;
    pComponentPrivate->sCompPorts[OMX_VPP_RGB_OUTPUT_PORT].pPortDef.bEnabled    = OMX_TRUE;
    pComponentPrivate->sCompPorts[OMX_VPP_RGB_OUTPUT_PORT].pPortDef.bPopulated  = OMX_FALSE;
    pComponentPrivate->sCompPorts[OMX_VPP_RGB_OUTPUT_PORT].pPortDef.format.video.nFrameWidth  = DEFAULT_WIDTH; 
    pComponentPrivate->sCompPorts[OMX_VPP_RGB_OUTPUT_PORT].pPortDef.format.video.nFrameHeight = DEFAULT_HEIGHT;
    pComponentPrivate->sCompPorts[OMX_VPP_RGB_OUTPUT_PORT].pPortDef.format.video.eColorFormat = OMX_COLOR_Format16bitRGB565;
    pComponentPrivate->sCompPorts[OMX_VPP_RGB_OUTPUT_PORT].pPortDef.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    pComponentPrivate->sCompPorts[OMX_VPP_RGB_OUTPUT_PORT].eSupplierSetting     = OMX_BufferSupplyInput;
    pComponentPrivate->sCompPorts[OMX_VPP_RGB_OUTPUT_PORT].hTunnelComponent     = NULL;
    pComponentPrivate->sCompPorts[OMX_VPP_RGB_OUTPUT_PORT].nReturnedBufferCount = 0; 
    pComponentPrivate->sCompPorts[OMX_VPP_RGB_OUTPUT_PORT].eMirror                           = OMX_MirrorNone;

    pComponentPrivate->sCompPorts[OMX_VPP_YUV_OUTPUT_PORT].pPortDef.nPortIndex  = OMX_VPP_YUV_OUTPUT_PORT;
    pComponentPrivate->sCompPorts[OMX_VPP_YUV_OUTPUT_PORT].pPortDef.eDir        = OMX_DirOutput;
    pComponentPrivate->sCompPorts[OMX_VPP_YUV_OUTPUT_PORT].pPortDef.eDomain     = OMX_PortDomainVideo;
    pComponentPrivate->sCompPorts[OMX_VPP_YUV_OUTPUT_PORT].pPortDef.nBufferCountActual = 1;
    pComponentPrivate->sCompPorts[OMX_VPP_YUV_OUTPUT_PORT].pPortDef.nBufferCountMin = 1;
    pComponentPrivate->sCompPorts[OMX_VPP_YUV_OUTPUT_PORT].pPortDef.nBufferSize = (DEFAULT_HEIGHT *DEFAULT_WIDTH *2);
    pComponentPrivate->sCompPorts[OMX_VPP_YUV_OUTPUT_PORT].pPortDef.bEnabled    = OMX_TRUE;
    pComponentPrivate->sCompPorts[OMX_VPP_YUV_OUTPUT_PORT].pPortDef.bPopulated  = OMX_FALSE;
    pComponentPrivate->sCompPorts[OMX_VPP_YUV_OUTPUT_PORT].pPortDef.format.video.nFrameWidth = DEFAULT_WIDTH; 
    pComponentPrivate->sCompPorts[OMX_VPP_YUV_OUTPUT_PORT].pPortDef.format.video.nFrameHeight = DEFAULT_HEIGHT;
    pComponentPrivate->sCompPorts[OMX_VPP_YUV_OUTPUT_PORT].pPortDef.format.video.eColorFormat = OMX_COLOR_FormatCbYCrY;
    pComponentPrivate->sCompPorts[OMX_VPP_YUV_OUTPUT_PORT].pPortDef.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    pComponentPrivate->sCompPorts[OMX_VPP_YUV_OUTPUT_PORT].eSupplierSetting     = OMX_BufferSupplyInput;
    pComponentPrivate->sCompPorts[OMX_VPP_YUV_OUTPUT_PORT].hTunnelComponent     = NULL;
    pComponentPrivate->sCompPorts[OMX_VPP_YUV_OUTPUT_PORT].nReturnedBufferCount = 0; 
    pComponentPrivate->sCompPorts[OMX_VPP_YUV_OUTPUT_PORT].eMirror                           = OMX_MirrorNone;

    
    /* Set pInPortFormat defaults */
    OMX_INIT_STRUCT(pComponentPrivate->pInPortFormat, OMX_VIDEO_PARAM_PORTFORMATTYPE);
    pComponentPrivate->pInPortFormat->nPortIndex   = OMX_VPP_INPUT_PORT;
    pComponentPrivate->pInPortFormat->nIndex       = 9;
    pComponentPrivate->pInPortFormat->eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;/*OMX_COLOR_FormatCbYCrY; */
    pComponentPrivate->pInPortFormat->eCompressionFormat = OMX_VIDEO_CodingUnused;
    

    OMX_INIT_STRUCT(pComponentPrivate->pInPortOverlayFormat, OMX_VIDEO_PARAM_PORTFORMATTYPE);
    pComponentPrivate->pInPortOverlayFormat->nPortIndex   = OMX_VPP_INPUT_OVERLAY_PORT;
    pComponentPrivate->pInPortOverlayFormat->nIndex       = 1;
    pComponentPrivate->pInPortOverlayFormat->eColorFormat = OMX_COLOR_Format24bitRGB888;
    pComponentPrivate->pInPortOverlayFormat->eCompressionFormat = OMX_VIDEO_CodingUnused;

    /* Set pOutPortFormat defaults */
    OMX_INIT_STRUCT(pComponentPrivate->pOutPortRGBFormat, OMX_VIDEO_PARAM_PORTFORMATTYPE);
    pComponentPrivate->pOutPortRGBFormat->nPortIndex   = OMX_VPP_RGB_OUTPUT_PORT;
    pComponentPrivate->pOutPortRGBFormat->nIndex       = 8;
    pComponentPrivate->pOutPortRGBFormat->eColorFormat = OMX_COLOR_Format16bitRGB565;
    pComponentPrivate->pOutPortRGBFormat->eCompressionFormat = OMX_VIDEO_CodingUnused;

    /* Set pOutPortFormat defaults */
    OMX_INIT_STRUCT(pComponentPrivate->pOutPortYUVFormat, OMX_VIDEO_PARAM_PORTFORMATTYPE);
    pComponentPrivate->pOutPortYUVFormat->nPortIndex   = OMX_VPP_YUV_OUTPUT_PORT;
    pComponentPrivate->pOutPortYUVFormat->nIndex       = 2;
    pComponentPrivate->pOutPortYUVFormat->eColorFormat = OMX_COLOR_FormatCbYCrY;
    pComponentPrivate->pOutPortYUVFormat->eCompressionFormat = OMX_VIDEO_CodingUnused;

    /*Set sScale defaults*/
    pComponentPrivate->sScale.nSize = sizeof(OMX_CONFIG_SCALEFACTORTYPE);
    pComponentPrivate->sScale.nVersion.s.nVersionMajor = VPP_MAJOR_VER;
    pComponentPrivate->sScale.nVersion.s.nVersionMinor = VPP_MINOR_VER;
    pComponentPrivate->sScale.nVersion.s.nRevision = VPP_REVISION;
    pComponentPrivate->sScale.nVersion.s.nStep = VPP_STEP; 
    pComponentPrivate->sScale.xHeight = 0;
    pComponentPrivate->sScale.xWidth = 0;

    /* Set pPriorityMgmt defaults */
    OMX_INIT_STRUCT(pComponentPrivate->pPriorityMgmt, OMX_PRIORITYMGMTTYPE);
    pComponentPrivate->pPriorityMgmt->nGroupPriority = 0; 
    pComponentPrivate->pPriorityMgmt->nGroupID       = 0; 

    pComponentPrivate->pMarkData             = NULL;      
    pComponentPrivate->hMarkTargetComponent  = NULL;  
    pComponentPrivate->bIsStopping = 0;

    pComponentPrivate->nInputFrame = 0;
    pComponentPrivate->nOverlayFrame = 0;
    pComponentPrivate->nInYUVBufferCount = 0;
    pComponentPrivate->nInRGBBufferCount = 0;
    pComponentPrivate->nOutYUVBufferCount = 0;
    pComponentPrivate->nOutRGBBufferCount = 0;

    pComponentPrivate->nFlushPort = OMX_NOPORT;

    pthread_mutex_init(&pComponentPrivate->vpp_mutex, NULL);
    pthread_cond_init(&pComponentPrivate->stop_cond, NULL);
    pthread_mutex_init(&pComponentPrivate->buf_mutex, NULL);

    /* Set pInBufSupplier defaults */
    for(port=0; port<NUM_OF_VPP_PORTS; port++) {
        for (buffers = 0; buffers < NUM_OF_VPP_BUFFERS; buffers++) {
            pComponentPrivate->sCompPorts[port].pVPPBufHeader[buffers].pBufHeader = NULL;
            pComponentPrivate->sCompPorts[port].pVPPBufHeader[buffers].pBufferStart = NULL;
            pComponentPrivate->sCompPorts[port].nBufferCount                      = 0;
            pComponentPrivate->sCompPorts[port].nBufSupplier                      = OMX_FALSE;
        }
    }
    pComponentPrivate->RGBbuffer = NULL;
    pComponentPrivate->pLcmlHandle = NULL;
    pComponentPrivate->bPreempted = OMX_FALSE;

    return eError;
}



/*-------------------------------------------------------------------*/
/**
  * IsTIOMXComponent()
  *
  * Check if the component is TI component.
  *
  * @param hTunneledComp Component Tunnel Pipe
  *  
  * @retval OMX_TRUE   Input is a TI component.
  *         OMX_FALSE  Input is a not a TI component. 
  *
  **/
/*-------------------------------------------------------------------*/

OMX_BOOL IsTIOMXComponent(OMX_HANDLETYPE hComp)
{

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_STRING pTunnelcComponentName = NULL;
    OMX_VERSIONTYPE* pTunnelComponentVersion = NULL;
    OMX_VERSIONTYPE* pSpecVersion = NULL;
    OMX_UUIDTYPE* pComponentUUID = NULL;
    char *pSubstring = NULL;
    OMX_BOOL bResult = OMX_TRUE;

    OMX_MALLOC(pTunnelcComponentName, 128);
    OMX_MALLOC(pTunnelComponentVersion, sizeof(OMX_VERSIONTYPE));
    OMX_MALLOC(pSpecVersion, sizeof(OMX_VERSIONTYPE));    
    OMX_MALLOC(pComponentUUID, sizeof(OMX_UUIDTYPE));    

    eError = OMX_GetComponentVersion (hComp, pTunnelcComponentName, pTunnelComponentVersion, pSpecVersion, pComponentUUID);

    /* Check if tunneled component is a TI component */
    pSubstring = strstr(pTunnelcComponentName, "OMX.TI.");

    if(pSubstring == NULL) {
        bResult = OMX_FALSE;
    }

EXIT:
    OMX_FREE(pTunnelcComponentName);
    OMX_FREE(pTunnelComponentVersion);
    OMX_FREE(pSpecVersion);
    OMX_FREE(pComponentUUID);

    return bResult;
} /* End of IsTIOMXComponent */



/*-------------------------------------------------------------------*/
/**
  *  VPP_InitBufferDataPropagation()
  *
  *
  *
  *
  * @param
  * @param
  * @param
  *
  * @retval OMX_NoError              Success, ready to roll
  *         
  **/
/*-------------------------------------------------------------------*/
void VPP_InitBufferDataPropagation(
    VPP_COMPONENT_PRIVATE *pComponentPrivate, 
    OMX_U32 nPortIndex)
{
    VPP_PORT_TYPE *pPortType = NULL;
    int i;

     pPortType = &(pComponentPrivate->sCompPorts[nPortIndex]);

    /* assume  pPortType->pPortDef->nBufferCountActual <= NUM_OF_BUFFERSJPEG */
    for (i = 0; i < pPortType->pPortDef.nBufferCountActual; i ++) {
        pPortType->sBufferDataProp[i].flag = 0;
        pPortType->sBufferDataProp[i].buffer_idYUV = 0xFFFFFFFF;
        pPortType->sBufferDataProp[i].buffer_idRGB = 0xFFFFFFFF;
        pPortType->sBufferDataProp[i].pMarkData = NULL;
        pPortType->sBufferDataProp[i].hMarkTargetComponent = NULL;
        pPortType->sBufferDataProp[i].nTickCount = 0;
        pPortType->sBufferDataProp[i].nTimeStamp = 0;
    }
}


#ifdef RESOURCE_MANAGER_ENABLED
/* ========================================================================== */
/**
 *  ResourceManagerCallback() - handle callbacks from Resource Manager
 * @param cbData    Resource Manager Command Data Structure
 * @return: void
  **/
/* ========================================================================== */

void ResourceManagerCallback(RMPROXY_COMMANDDATATYPE cbData)
{
    OMX_COMMANDTYPE Cmd = OMX_CommandStateSet;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)cbData.hComponent;
    VPP_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_ERRORTYPE RM_Error = *(cbData.RM_Error);
    
    VPP_DPRINT("%s: %d: RM_Error = %x\n", __FUNCTION__, __LINE__, RM_Error);

    pComponentPrivate = (VPP_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    if (RM_Error == OMX_RmProxyCallback_ResourcesPreempted) {

        pComponentPrivate->bPreempted = 1;
        
        if (pComponentPrivate->curState == OMX_StateExecuting || 
            pComponentPrivate->curState == OMX_StatePause) {

            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                   OMX_EventError,
                                                   OMX_ErrorResourcesPreempted,
                                                   OMX_TI_ErrorMajor,
                                                   NULL);
            
            pComponentPrivate->toState = OMX_StateIdle;
            pComponentPrivate->bIsStopping = OMX_TRUE;
            VPP_DPRINT("Component Preempted. Going to IDLE State.\n");
        }
        else if (pComponentPrivate->curState == OMX_StateIdle){
            pComponentPrivate->toState = OMX_StateLoaded;            
            VPP_DPRINT("Component Preempted. Going to LOADED State.\n");            
        }
        
#ifdef __PERF_INSTRUMENTATION__
        PERF_SendingCommand(pComponentPrivate->pPERF, Cmd, pComponentPrivate->toState, PERF_ModuleComponent);
#endif
        
        write (pComponentPrivate->cmdPipe[1], &Cmd, sizeof(Cmd));
        write (pComponentPrivate->nCmdDataPipe[1], &(pComponentPrivate->toState) ,sizeof(OMX_U32));
        
    }
    else if (RM_Error == OMX_RmProxyCallback_ResourcesAcquired ){

        if (pComponentPrivate->curState == OMX_StateWaitForResources) /* Wait for Resource Response */
        {
            pComponentPrivate->cbInfo.EventHandler (
    	                        pHandle, pHandle->pApplicationPrivate,
    	                        OMX_EventResourcesAcquired, 0,0,
    	                        NULL);
            
            pComponentPrivate->toState = OMX_StateIdle;
            pComponentPrivate->bIsStopping = OMX_TRUE;
            
#ifdef __PERF_INSTRUMENTATION__
            PERF_SendingCommand(pComponentPrivate->pPERF, Cmd, pComponentPrivate->toState, PERF_ModuleComponent);
#endif
        
            write (pComponentPrivate->cmdPipe[1], &Cmd, sizeof(Cmd));
            write (pComponentPrivate->nCmdDataPipe[1], &(pComponentPrivate->toState) ,sizeof(OMX_U32));
            VPP_DPRINT("OMX_RmProxyCallback_ResourcesAcquired.\n");
        }            
        
    }

}
#endif

void LinkedList_Create(LinkedList *LinkedList) {
    LinkedList->pRoot = NULL;
}

void LinkedList_AddElement(LinkedList *LinkedList, void *pValue) {
    /* create new node and fill the value */
    Node *pNewNode = (Node *)malloc(sizeof(Node));
    if (pNewNode != NULL) {
        pNewNode->pValue = (void *)pValue;
        /*printf("LinkedList:::: Pointer=%p has been added.\n", pNewNode->pValue); */
        /* add new node on the root to implement quick FIFO */
        /* modify new node pointers */
        if (LinkedList->pRoot == NULL) {
            pNewNode->pNextNode = NULL;
        }
        else {
             pNewNode->pNextNode = LinkedList->pRoot;
        }   
        /*modify root */
        LinkedList->pRoot = pNewNode;
    }
}

void LinkedList_FreeElement(LinkedList *LinkedList, void *pValue) {
    Node *pNode = LinkedList->pRoot;
    Node *pPastNode = NULL;
    while (pNode != NULL) {
        if (pNode->pValue == pValue) {
            Node *pTempNode = pNode->pNextNode;
            if(pPastNode == NULL) {
                LinkedList->pRoot = pTempNode;
            }
            else {
                pPastNode->pNextNode = pTempNode;
            }
            /*printf("LinkedList:::: Pointer=%p has been freed\n", pNode->pValue); */
            free(pNode->pValue);
            free(pNode);
            break;
        }
        pPastNode = pNode;
        pNode = pNode->pNextNode;
    }
}

void LinkedList_FreeAll(LinkedList *LinkedList) {
    Node *pTempNode;
    int nodes = 0;
    while (LinkedList->pRoot != NULL) {
        pTempNode = LinkedList->pRoot->pNextNode;
        /*printf("LinkedList:::: Pointer=%p has been freed\n", LinkedList->pRoot->pValue); */
        free(LinkedList->pRoot->pValue);
        free(LinkedList->pRoot); 
        LinkedList->pRoot = pTempNode;
        nodes++;
    } 
    /* printf("==================No. of deleted nodes: %d=======================================\n\n", nodes); */
}

void LinkedList_DisplayAll(LinkedList *LinkedList) {
    Node *pNode = LinkedList->pRoot;
    int nodes = 0;
    printf("\n================== Displaying contents of linked list=%p=====================\n", LinkedList);
    printf("root->\n");
    while (pNode != NULL) {
        printf("[Value=%p, NextNode=%p]->\n", pNode->pValue, pNode->pNextNode);
        pNode = pNode->pNextNode;
        nodes++;
    } 
     printf("==================No. of existing nodes: %d=======================================\n\n", nodes);
}

void LinkedList_Destroy(LinkedList *LinkedList) {
    /* do nothing */
}

