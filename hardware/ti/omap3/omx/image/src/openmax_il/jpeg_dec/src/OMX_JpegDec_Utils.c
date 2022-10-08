
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
/* ====================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
* ==================================================================== */

/**
* @file OMX_JpegDec_Utils.c
*
* This file implements OMX Component for JPEG decoder
*
* @patth $(CSLPATH)\jpeg_dec\src\OMX_JpegDec_Utils.c
*
* @rev 0.2
*/


/****************************************************************
 *  INCLUDE FILES
*****************************************************************/

/* -----------System and Platform Files ------------------------*/

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
    #include <sched.h>
#endif


    #include <dbapi.h>
    #include <string.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <pthread.h>

/*--------------------- Program Header Files ----------------------------*/

#include <OMX_Types.h>
#include <OMX_Component.h>
#include <OMX_Core.h>
#include <OMX_Index.h>
#include <OMX_Image.h>
#include <OMX_Audio.h>
#include <OMX_Video.h>
#include <OMX_IVCommon.h>
#include <OMX_Other.h>
#include "OMX_JpegDec_Utils.h"
#include <usn.h>

#ifdef RESOURCE_MANAGER_ENABLED
    #include <ResourceManagerProxyAPI.h>
#endif

#define JPEGDEC_TIMEOUT 10

#ifdef UNDER_CE
    HINSTANCE g_hLcmlDllHandle = NULL;
#endif

OMX_ERRORTYPE LCML_CallbackJpegDec(TUsnCodecEvent event,
                                   void * args [10]);


/*------------------------- Function Implementation ------------------*/

/* ========================================================================== */
/**
 * @fn GetLCMLHandleJpegDec - Implements the functionality to get LCML handle
 * @param pComponent - components private structure
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on failure
 */
/* ========================================================================== */
OMX_ERRORTYPE GetLCMLHandleJpegDec(OMX_HANDLETYPE pComponent)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
#ifndef UNDER_CE
    OMX_HANDLETYPE LCML_pHandle;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate = (JPEGDEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    jpegdec_fpo fpGetHandle;
    void *handle = NULL;
    char *error = NULL;

    JPEGDEC_OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);

    handle = dlopen("libLCML.so", RTLD_LAZY);
    if (!handle) {
	if ((error = (char *)dlerror()) != NULL) {
            fputs(error, stderr);
        }
        eError = OMX_ErrorComponentNotFound;
        goto EXIT;
    }

    fpGetHandle = dlsym(handle, "GetHandle");

    if ((error = (char *)dlerror()) != NULL) {
        fputs(error, stderr);
        eError = OMX_ErrorInvalidComponent;
        goto EXIT;
    }

    /*calling gethandle and passing phandle to be filled */
    if ( fpGetHandle != NULL ) {
        eError = (*fpGetHandle)(&LCML_pHandle);
    }
    else  {
        eError = OMX_ErrorInvalidComponent;
        goto EXIT;
    }
    if (eError != OMX_ErrorNone) {
        eError = OMX_ErrorUndefined;
        OMX_PRDSP5(pComponentPrivate->dbg, "eError != OMX_ErrorNone... in (*fpGetHandle)(&LCML_pHandle);\n");
        goto EXIT;
    }

    pComponentPrivate->pDllHandle = handle;
    pComponentPrivate->pLCML = (void *)LCML_pHandle;
    pComponentPrivate->pLCML->pComponentPrivate = (JPEGDEC_COMPONENT_PRIVATE *)pComponentPrivate;

EXIT:
#else
    typedef OMX_ERRORTYPE (*LPFNDLLFUNC1)(OMX_HANDLETYPE);
    LPFNDLLFUNC1 fpGetHandle1;
    OMX_HANDLETYPE LCML_pHandle = NULL;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate = (JPEGDEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    g_hLcmlDllHandle = LoadLibraryEx(TEXT("OAF_BML.dll"), NULL, 0);
    
    if (g_hLcmlDllHandle == NULL) 
    {
        eError = OMX_ErrorComponentNotFound;
        goto EXIT;
    }

    fpGetHandle1 = (LPFNDLLFUNC1)GetProcAddress(g_hLcmlDllHandle,TEXT("GetHandle"));
    if (!fpGetHandle1) {
        FreeLibrary(g_hLcmlDllHandle);
        g_hLcmlDllHandle = NULL;
        eError = OMX_ErrorComponentNotFound;
        goto EXIT;
    }

    eError = fpGetHandle1(&LCML_pHandle);
    if (eError != OMX_ErrorNone) {
        FreeLibrary(g_hLcmlDllHandle);
        g_hLcmlDllHandle = NULL;
        eError = OMX_ErrorUndefined;
        LCML_pHandle = NULL;
        goto EXIT;
    }

    (LCML_DSP_INTERFACE*)pComponentPrivate->pLCML = (LCML_DSP_INTERFACE *)LCML_pHandle;
    pComponentPrivate->pLCML->pComponentPrivate = (JPEGDEC_COMPONENT_PRIVATE *)pComponentPrivate;

EXIT:
#endif
    return eError;

}   /* End of GetLCMLHandle */

/* ========================================================================== */
/**
 * @fn DisablePortJpegDec - Implements the functionality to disable the ports
 * @param pComponentPrivate - components private structure
 * @param nParam1 - paramerer specifying the port type (Index port)
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on failure
 */
/* ========================================================================== */
OMX_ERRORTYPE DisablePortJpegDec(JPEGDEC_COMPONENT_PRIVATE* pComponentPrivate,
                                 OMX_U32 nParam1)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CHECK_PARAM(pComponentPrivate);

    JPEGDEC_OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1); 

    OMX_PRINT1(pComponentPrivate->dbg, "In DisablePortJpegDec %lu\n", nParam1);

    if (pComponentPrivate->nCurState == OMX_StateExecuting || pComponentPrivate->nCurState == OMX_StatePause) {
	if ((nParam1 == 0) || (nParam1 == 1) || ((int)nParam1 == -1)) {
            eError = HandleInternalFlush(pComponentPrivate, nParam1);
        }
    }

EXIT:
    return eError;
}   /* End of DisablePort */



/* ========================================================================== */
/**
 * @fn EnablePortJpegDec - Implements the functionality to enable the ports
 * @param pComponentPrivate - components private structure
 * @param nParam1 - paramerer specifying the port type (Index port)
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on failure
 */
/* ========================================================================== */
OMX_ERRORTYPE EnablePortJpegDec(JPEGDEC_COMPONENT_PRIVATE* pComponentPrivate,
                                OMX_U32 nParam1)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CHECK_PARAM(pComponentPrivate);

    JPEGDEC_OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1); 

    while (1) {
        if ((nParam1 == 0x0) &&
            ((pComponentPrivate->nCurState == OMX_StateLoaded) ||
             (pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef->bPopulated))) {

            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                   OMX_EventCmdComplete,
                                                   OMX_CommandPortEnable,
                                                   JPEGDEC_INPUT_PORT,
                                                   NULL);
            break;
        }
        else if ((nParam1 == 0x1) &&
                 ((pComponentPrivate->nCurState == OMX_StateLoaded) ||
                  (pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef->bPopulated))) {

            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                   OMX_EventCmdComplete,
                                                   OMX_CommandPortEnable,
                                                   JPEGDEC_OUTPUT_PORT,
                                                   NULL);
            break;
        }
        else if (((int)nParam1 == -1) &&
                ((pComponentPrivate->nCurState == OMX_StateLoaded) ||
                 ((pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef->bPopulated) &&
                  (pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef->bPopulated)))) {

            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                   OMX_EventCmdComplete,
                                                   OMX_CommandPortEnable,
                                                   JPEGDEC_INPUT_PORT,
                                                   NULL);

            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                   OMX_EventCmdComplete,
                                                   OMX_CommandPortEnable,
                                                   JPEGDEC_OUTPUT_PORT,
                                                   NULL);
            break;

        }
        else {
                JPEGDEC_WAIT_PORT_POPULATION(pComponentPrivate);
        }
    }
    OMX_PRINT1(pComponentPrivate->dbg, "Exiting EnablePortJpegDec(), Ports are enabled if no error\n");
EXIT:
    return eError;
}   /* End of EnablePort */


/* ========================================================================== */
/**
 * @fn Start_ComponentThreadJpegDec - Implements the functionality to start
 *  the component thread. Creates data pipes, commmand pipes and initializes
 *  Component thread.
 * @param pComponentPrivate - components private structure
 * @param nParam1 - paramerer specifying the port type (Index port)
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on failure
 */
/* ========================================================================== */
OMX_ERRORTYPE Start_ComponentThreadJpegDec(OMX_HANDLETYPE pComponent)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = NULL;
    JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
#ifdef UNDER_CE
    pthread_attr_t attr;
    memset(&attr, 0, sizeof(attr));
    attr.__inheritsched = PTHREAD_EXPLICIT_SCHED;
    attr.__schedparam.__sched_priority = OMX_IMAGE_DECODER_THREAD_PRIORITY;
#endif

    OMX_CHECK_PARAM(pComponent);
    pHandle = (OMX_COMPONENTTYPE *)pComponent;
    pComponentPrivate = (JPEGDEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    JPEGDEC_OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);

    /* create the pipe used to maintain free output buffers*/
    eError = pipe (pComponentPrivate->nFree_outBuf_Q);
    if (eError) {
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    /* create the pipe used to maintain filled input buffers*/
    eError = pipe (pComponentPrivate->nFilled_inpBuf_Q);
    if (eError) {
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    /* create the pipe used to send commands to the thread */
    eError = pipe (pComponentPrivate->nCmdPipe);
    if (eError) {
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    /* create the pipe used to send commands to the thread */
    eError = pipe (pComponentPrivate->nCmdDataPipe);
    if (eError) {
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    OMX_PRINT2(pComponentPrivate->dbg, "JPEG Start_ComponentThread\n");
    /* Create the Component Thread */
#ifdef UNDER_CE
    eError = pthread_create (&(pComponentPrivate->pComponentThread), &attr, OMX_JpegDec_Thread, pComponent);
#else
    eError = pthread_create (&(pComponentPrivate->pComponentThread), NULL, OMX_JpegDec_Thread, pComponent);
#endif

    if (eError || !pComponentPrivate->pComponentThread) {
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

#ifdef __PERF_INSTRUMENTATION__
    PERF_ThreadCreated(pComponentPrivate->pPERF,
                       pComponentPrivate->pComponentThread,
                       PERF_FOURS("JPDT"));
#endif

EXIT:
    return eError;
}   /* End of Start_ComponentThreadJpegDec */



/* ========================================================================== */
/**
 * @fn Free_ComponentResourcesJpegDec - Implements the functionality to de-init
 *  the component thread. close component thread, Command pipe, data pipe &
 *  LCML pipe.
 * @param pComponentPrivate - components private structure
 * @param nParam1 - paramerer specifying the port type (Index port)
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on failure
 */
/* ========================================================================== */
OMX_ERRORTYPE Free_ComponentResourcesJpegDec(JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError        = OMX_ErrorNone;
    OMX_ERRORTYPE threadError   = OMX_ErrorNone;
    OMX_ERRORTYPE eErr          = OMX_ErrorNone;
    int pthreadError = 0, nRet = 0;
    OMX_COMMANDTYPE eCmd = OMX_CustomCommandStopThread;
    struct OMX_TI_Debug dbg;

    OMX_DBG_INIT_BASE(dbg);
    if (!pComponentPrivate) {
	OMXDBG_PRINT(stderr, ERROR, 5, 0, "pComponentPrivate is NULL.\n");
        goto EXIT;
    }

    JPEGDEC_OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);

    dbg = pComponentPrivate->dbg; 

#ifdef __PERF_INSTRUMENTATION__
    PERF_Boundary(pComponentPrivate->pPERF,
                  PERF_BoundaryStart | PERF_BoundaryCleanup);
#endif

    OMX_PRINT1(pComponentPrivate->dbg, "Inside Free_ComponentResourcesJpegDec \n");
    /* should clean up in case of crash - implement*/



    if (pComponentPrivate->nIsLCMLActive ==1) {
	OMX_PRINT1(pComponentPrivate->dbg, "EMMCodecControlDestroy inside Free_ComponentResourcesJpegDec\n");
        LCML_ControlCodec(((LCML_DSP_INTERFACE*)pComponentPrivate->pLCML)->pCodecinterfacehandle,EMMCodecControlDestroy,NULL);
        pComponentPrivate->nIsLCMLActive = 0;
#ifdef UNDER_CE
        FreeLibrary(g_hLcmlDllHandle);
        g_hLcmlDllHandle = NULL;
#else
        dlclose(pComponentPrivate->pDllHandle);
        pComponentPrivate->pDllHandle = NULL;
#endif
    }

#ifdef __PERF_INSTRUMENTATION__
    PERF_SendingCommand(pComponentPrivate->pPERF,
                        eCmd, 0, PERF_ModuleComponent);
#endif

    OMX_PRINT2(pComponentPrivate->dbg, "Freeing resources\n");

    nRet = write(pComponentPrivate->nCmdPipe[1], &eCmd, sizeof(eCmd));
    if (nRet == -1) {
        OMX_PRCOMM4(pComponentPrivate->dbg, "Error while writing into nCmdPipe\n");
        eError = OMX_ErrorHardware;
    }

    nRet = write(pComponentPrivate->nCmdDataPipe[1], &eCmd, sizeof(eCmd));
    if (nRet == -1) {
        OMX_PRCOMM4(pComponentPrivate->dbg, "Error while writing into nCmdDataPipe\n");
        eError = OMX_ErrorHardware;
    }

    pthreadError = pthread_join(pComponentPrivate->pComponentThread, (void*)&threadError);
    if (0 != pthreadError)    {
        eError = OMX_ErrorHardware;
        OMX_TRACE4(pComponentPrivate->dbg, "Error while closing Component Thread\n");
    }

    if (OMX_ErrorNone != threadError && OMX_ErrorNone != eError) {
	OMX_TRACE5(pComponentPrivate->dbg, "OMX_ErrorInsufficientResources\n");
        eError = OMX_ErrorInsufficientResources;
        OMX_TRACE4(pComponentPrivate->dbg, "Error while closing Component Thread\n");
    }

    /* close the data pipe handles*/
    eErr = close(pComponentPrivate->nFree_outBuf_Q[0]);
    if (0 != eErr && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
        OMX_PRCOMM4(pComponentPrivate->dbg, "Error while closing data pipe\n");
    }

    eErr = close(pComponentPrivate->nFilled_inpBuf_Q[0]);
    if (0 != eErr && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
        OMX_PRCOMM4(pComponentPrivate->dbg, "Error while closing data pipe\n");
    }

    eErr = close(pComponentPrivate->nFree_outBuf_Q[1]);
    if (0 != eErr && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
        OMX_PRCOMM4(pComponentPrivate->dbg, "Error while closing data pipe\n");
    }

    eErr = close(pComponentPrivate->nFilled_inpBuf_Q[1]);
    if (0 != eErr && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
	OMX_PRCOMM4(pComponentPrivate->dbg, "Error while closing data pipe\n");
    }

    /*Close the command pipe handles*/
    eErr = close(pComponentPrivate->nCmdPipe[0]);
    if (0 != eErr && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
	OMX_PRCOMM4(pComponentPrivate->dbg, "Error while closing cmd pipe\n");
    }

    eErr = close(pComponentPrivate->nCmdPipe[1]);
    if (0 != eErr && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
	OMX_PRCOMM4(pComponentPrivate->dbg, "Error while closing cmd pipe\n");
    }

    /*Close the command data pipe handles*/
    eErr = close(pComponentPrivate->nCmdDataPipe[0]);
    if (0 != eErr && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
	OMX_PRCOMM4(pComponentPrivate->dbg, "Error while closing cmd pipe\n");
    }

    eErr = close(pComponentPrivate->nCmdDataPipe[1]);
    if (0 != eErr && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
	OMX_PRCOMM4(pComponentPrivate->dbg, "Error while closing cmd pipe\n");
    }
    
     if (pthread_mutex_destroy(&(pComponentPrivate->mJpegDecMutex)) != 0){
         OMX_TRACE4(pComponentPrivate->dbg, "Error with pthread_mutex_destroy");
    }

    if(pthread_cond_destroy(&(pComponentPrivate->sStop_cond)) != 0){
	OMX_TRACE4(pComponentPrivate->dbg, "Error with pthread_cond_desroy");
    }


#ifdef __PERF_INSTRUMENTATION__
    PERF_Boundary(pComponentPrivate->pPERF,
                  PERF_BoundaryComplete | PERF_BoundaryCleanup);
    PERF_Done(pComponentPrivate->pPERF);
#endif

    /*LinkedList_DisplayAll (&AllocList); */
    OMX_FREEALL();
    LinkedList_Destroy(&AllocList);

EXIT:
    OMX_PRINT1(dbg, "Exiting Successfully After Freeing All Resources Errror %x, \n", eError);
    return eError;
}   /* End of Free_ComponentResourcesJpegDec */



/* ========================================================================== */
/**
 * @fn Fill_LCMLInitParamsJpegDec - This function fills the create phase
 *  parameters used by DSP
 * @param lcml_dsp    handle for this instance of the LCML
 * @param arr[]       array with the parameters
 * @param pComponent  handle for this instance of component
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on failure
 */
/* ========================================================================== */
OMX_ERRORTYPE Fill_LCMLInitParamsJpegDec(LCML_DSP *lcml_dsp,
                                                OMX_U16 arr[],
                                                OMX_HANDLETYPE pComponent)
{

    OMX_ERRORTYPE eError            = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle  =  NULL;
    JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate    = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefOut       = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefIn        = NULL;
    OMX_U16 nScaleFactor;
    OMX_U16 nFrameWidth;
    OMX_U16 nFrameHeight;

    OMX_CHECK_PARAM(pComponent);
    pHandle = (OMX_COMPONENTTYPE *)pComponent;
    pComponentPrivate = (JPEGDEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    JPEGDEC_OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);

    pPortDefIn = pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef;
    pPortDefOut = pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef;

    lcml_dsp->In_BufInfo.DataTrMethod = DMM_METHOD;
    lcml_dsp->Out_BufInfo.DataTrMethod = DMM_METHOD;

    lcml_dsp->NodeInfo.nNumOfDLLs = OMX_JPEGDEC_NUM_DLLS;
    lcml_dsp->NodeInfo.AllUUIDs[0].uuid = (struct DSP_UUID *)&JPEGDSOCKET_TI_UUID;
    strcpy ((char *)lcml_dsp->NodeInfo.AllUUIDs[0].DllName,JPEG_DEC_NODE_DLL);
    lcml_dsp->NodeInfo.AllUUIDs[0].eDllType = DLL_NODEOBJECT;

    lcml_dsp->NodeInfo.AllUUIDs[1].uuid = (struct DSP_UUID *)&JPEGDSOCKET_TI_UUID;
    strcpy ((char *)lcml_dsp->NodeInfo.AllUUIDs[1].DllName,JPEG_DEC_NODE_DLL);
    lcml_dsp->NodeInfo.AllUUIDs[1].eDllType = DLL_DEPENDENT;

    lcml_dsp->NodeInfo.AllUUIDs[2].uuid =(struct DSP_UUID *) &USN_UUID;
    strcpy ((char *)lcml_dsp->NodeInfo.AllUUIDs[2].DllName,USN_DLL);
    lcml_dsp->NodeInfo.AllUUIDs[2].eDllType = DLL_DEPENDENT;

    lcml_dsp->DeviceInfo.TypeofDevice = 0;
    lcml_dsp->SegID = 0;
    lcml_dsp->Timeout = -1;
    lcml_dsp->Alignment = 0;
    lcml_dsp->Priority = 5;


    switch(pComponentPrivate->pScalePrivate->xWidth){
        case (0):
            nScaleFactor = 100;
            break;
        case (1):
            nScaleFactor = 50;
            break;
        case (2):
            nScaleFactor = 25;
            break;
        case (3):
            nScaleFactor = 13; /*12.5*/
            break;
        case (4):
            nScaleFactor = 200;
            break;
        case (5):
            nScaleFactor = 400;
            break;
        case (6):
            nScaleFactor = 800;
            break;
        default:
            nScaleFactor = 100;
            break;
    }

    nFrameWidth = pPortDefIn->format.image.nFrameWidth * nScaleFactor / 100;
    nFrameHeight = pPortDefIn->format.image.nFrameHeight * nScaleFactor / 100;    
    
    if (pComponentPrivate->nProgressive == 1) {
        if (nFrameHeight <= 144 &&
            nFrameWidth<= 176) {
            lcml_dsp->ProfileID = 0;
        }
        else if (nFrameHeight <= 288 &&
            nFrameWidth<= 352) {
            lcml_dsp->ProfileID = 1;
        }
        else if (nFrameHeight <= 480 &&
            nFrameWidth <= 640) {
            lcml_dsp->ProfileID = 2;
        }
        else if (nFrameHeight<= 1024 &&
            nFrameWidth <= 1280) {
            lcml_dsp->ProfileID = 4;
        }
        else if (nFrameHeight <= 1200 &&
            nFrameWidth<= 1920) {
            lcml_dsp->ProfileID = 5;
        }
        else if (nFrameHeight<= 1536 &&
            nFrameWidth<= 2048) {
            lcml_dsp->ProfileID = 6;
        }
        else if (nFrameHeight<= 1600 &&
            nFrameWidth<= 2560) {
            lcml_dsp->ProfileID = 7;
        }
        else if (nFrameHeight <= 2048 &&
            nFrameWidth<= 2560) {
            lcml_dsp->ProfileID = 8;
        }
        else if (nFrameHeight <= 2048 &&
            nFrameWidth<= 3200) {
            lcml_dsp->ProfileID = 9;
        }
        else {
            lcml_dsp->ProfileID = 3;
        }
    }
    else if (pComponentPrivate->nProgressive == 0) {
        lcml_dsp->ProfileID = -1;
    }
    pComponentPrivate->nProfileID = lcml_dsp->ProfileID;

    /*filling create phase params*/
    arr[0] = JPGDEC_SNTEST_STRMCNT;
    arr[1] = JPGDEC_SNTEST_INSTRMID;
    arr[2] = 0;
    arr[3] = JPGDEC_SNTEST_INBUFCNT;
    arr[4] = JPGDEC_SNTEST_OUTSTRMID;
    arr[5] = 0;
    arr[6] = JPGDEC_SNTEST_OUTBUFCNT;

    if (pComponentPrivate->nProgressive == 1) {
        OMX_PRINT2(pComponentPrivate->dbg, "JPEGdec:: nProgressive IMAGE");
        arr[7] = nFrameHeight;
        if ((arr[7]%2) != 0) arr[7]++;
        arr[8] = nFrameWidth;
        if ((arr[8]%2) != 0) arr[8]++; 
        
        arr[9] = JPGDEC_SNTEST_PROG_FLAG;
    }
    else {
        OMX_PRINT2(pComponentPrivate->dbg, "****** Max Width %d Max Height %d\n",(int)pComponentPrivate->sMaxResolution.nWidth,(int)pComponentPrivate->sMaxResolution.nHeight);

        arr[7] = pComponentPrivate->sMaxResolution.nHeight;
        arr[8] = pComponentPrivate->sMaxResolution.nWidth;
        arr[9] = 0;
    }

    if (pPortDefOut->format.image.eColorFormat == OMX_COLOR_FormatCbYCrY) {
        arr[10] = 4;
    }
    else if (pPortDefOut->format.image.eColorFormat == OMX_COLOR_Format16bitRGB565) {
        arr[10] = 9;
    }
    else if (pPortDefOut->format.image.eColorFormat == OMX_COLOR_Format24bitRGB888) {
        arr[10] = 10;
    }
    else if (pPortDefOut->format.image.eColorFormat == OMX_COLOR_Format32bitARGB8888) {
        arr[10] = 11;
    }
    else if (pPortDefOut->format.image.eColorFormat == OMX_COLOR_Format32bitBGRA8888) {
        arr[10] = 11;
    }
    else { /*Set DEFAULT (Original) color format*/
        arr[10] = 1;
    }
    /*arr[11] doesn't need to be filled*/
    
    if(pComponentPrivate->pSectionDecode->bSectionsInput){ /*Slide decoding enable*/
        arr[12] = 1;
    }
    else{
        arr[12] = 0;
    }

    if(pComponentPrivate->pSectionDecode->bSectionsOutput){ /*Slide decoding enable*/
        arr[13] = 1;
    }
    else{
        arr[13] = 0;
    }

	/* RGB or ARGB output format for RGB32 images */
    arr[14] = 0;
    if (pPortDefOut->format.image.eColorFormat == OMX_COLOR_Format32bitARGB8888)
    {
    	arr[14] = 1; /* RGB32 output mode */
    }

    arr[15] = END_OF_CR_PHASE_ARGS;

    lcml_dsp->pCrPhArgs = arr;

    OMX_PRINT2(pComponentPrivate->dbg, "Image Width\t= %d\n", arr[8]);
    OMX_PRINT2(pComponentPrivate->dbg, "Image Height\t= %d\n", arr[7]);
    OMX_PRINT2(pComponentPrivate->dbg, "Progressive\t= %d\n", arr[9]);
    OMX_PRINT2(pComponentPrivate->dbg, "Color Format\t= %d\n", arr[10]);
    OMX_PRINT2(pComponentPrivate->dbg, "Section Decode(Input)\t= %d\n", arr[12]);
    OMX_PRINT2(pComponentPrivate->dbg, "Section Decode(Output)\t= %d\n", arr[13]);
    
EXIT:
    return eError;

}   /* End of Fill_LCMLInitParamsJpegDec */



/*-------------------------------------------------------------------*/
/**
  *  HandleInternalFlush() This function return request to USN to return all buffers
  * via USN 
  * *  
  *
  * @param pComponentPrivate
  * @param nParam1
  *
  * @retval OMX_NoError              Success, ready to roll
  *
  *
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE HandleInternalFlush(JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U32 nParam1)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 aParam[4];
    LCML_DSP_INTERFACE *pLCML = NULL;
    OMX_U8 nCount = 0;
#ifdef UNDER_CE
    OMX_U32 nTimeout = 0;
#endif
    OMX_CHECK_PARAM(pComponentPrivate);

    JPEGDEC_OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);

    if ( nParam1 == 0x0 || (int)nParam1 == -1 ) {

        aParam[0] = USN_STRMCMD_FLUSH;
        aParam[1] = 0;
        aParam[2] = 0;
        pLCML = (LCML_DSP_INTERFACE*)pComponentPrivate->pLCML;
        pComponentPrivate->bFlushComplete = OMX_FALSE;

        eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLCML)->pCodecinterfacehandle,EMMCodecControlStrmCtrl, (void*)aParam);
        if (eError != OMX_ErrorNone) {
            goto EXIT;
        }
#ifdef UNDER_CE
        nTimeout = 0;
#endif
        while (pComponentPrivate->bFlushComplete == OMX_FALSE) {
#ifdef UNDER_CE
            sched_yield();
            if (nTimeout++ > 200000) {
		OMX_PRDSP4(pComponentPrivate->dbg, "Flush input Timeout Error\n");
                break;
            }
#else
            JPEGDEC_WAIT_FLUSH(pComponentPrivate);
#endif
        }
        for (nCount = 0; nCount < pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef->nBufferCountActual; nCount++) {
            JPEGDEC_BUFFER_PRIVATE* pBuffPrivate = pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pBufferPrivate[nCount];

            if (pBuffPrivate->eBufferOwner != JPEGDEC_BUFFER_CLIENT) {
                OMX_BUFFERHEADERTYPE* pBuffHead = NULL;
                int nRet;
#ifdef __PERF_INSTRUMENTATION__
                    PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                      PREF(((OMX_BUFFERHEADERTYPE*) pComponentPrivate->pCompPort[0]->pBufferPrivate[nCount]->pBufferHdr), pBuffer),
                                      PREF(((OMX_BUFFERHEADERTYPE*) pComponentPrivate->pCompPort[0]->pBufferPrivate[nCount]->pBufferHdr), nFilledLen),
                                      PERF_ModuleLLMM);
#endif
        
                if (pBuffPrivate->eBufferOwner == JPEGDEC_BUFFER_COMPONENT_IN) {
			OMX_PRBUFFER2(pComponentPrivate->dbg, "disgard %p from InDir\n", pBuffPrivate->pBufferHdr);
                    nRet = read(pComponentPrivate->nFilled_inpBuf_Q[0], &pBuffHead, sizeof(pBuffHead));
                } 
                pBuffPrivate->eBufferOwner = JPEGDEC_BUFFER_CLIENT;
                OMX_PRBUFFER2(pComponentPrivate->dbg, "return input buffer %p in idle\n", pBuffPrivate->pBufferHdr);
                pComponentPrivate->cbInfo.EmptyBufferDone(pComponentPrivate->pHandle,
                                             pComponentPrivate->pHandle->pApplicationPrivate,
                                             pBuffPrivate->pBufferHdr);
            }
        }

        pComponentPrivate->bFlushComplete = OMX_FALSE;
    }
    if ( nParam1 == 0x1 || (int)nParam1 == -1 ) {

        aParam[0] = USN_STRMCMD_FLUSH;
        aParam[1] = 1;
        aParam[2] = 0;
        pLCML = (LCML_DSP_INTERFACE*)pComponentPrivate->pLCML;
        pComponentPrivate->bFlushComplete = OMX_FALSE;
        
        eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLCML)->pCodecinterfacehandle,EMMCodecControlStrmCtrl, (void*)aParam);
        if (eError != OMX_ErrorNone) {
            goto EXIT;
        }
#ifdef UNDER_CE
        nTimeout = 0;
#endif
        while (pComponentPrivate->bFlushComplete == OMX_FALSE) {
#ifdef UNDER_CE
            sched_yield();
            if (nTimeout++ > 200000) {
		OMX_PRDSP4(pComponentPrivate->dbg, "Flush output Timeout Error\n");
                break;
            }
#else
            JPEGDEC_WAIT_FLUSH(pComponentPrivate);
#endif
        }
        
        for (nCount = 0; nCount < pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef->nBufferCountActual; nCount++) {
            JPEGDEC_BUFFER_PRIVATE* pBuffPrivate = pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pBufferPrivate[nCount];

            if (pBuffPrivate->eBufferOwner != JPEGDEC_BUFFER_CLIENT) {
            OMX_BUFFERHEADERTYPE* pBuffHead = NULL;
            int nRet;
#ifdef __PERF_INSTRUMENTATION__
                PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                  PREF(((OMX_BUFFERHEADERTYPE*) pComponentPrivate->pCompPort[1]->pBufferPrivate[nCount]->pBufferHdr), pBuffer),
                                  PREF(((OMX_BUFFERHEADERTYPE*) pComponentPrivate->pCompPort[1]->pBufferPrivate[nCount]->pBufferHdr), nFilledLen),
                                  PERF_ModuleLLMM);
#endif

            if (pBuffPrivate->eBufferOwner == JPEGDEC_BUFFER_COMPONENT_IN) {
	        OMX_PRBUFFER2(pComponentPrivate->dbg, "discard %p from InDir\n", pBuffPrivate->pBufferHdr);
                pComponentPrivate->nOutPortOut ++;
                 nRet = read(pComponentPrivate->nFree_outBuf_Q[0], &pBuffHead, sizeof(pBuffHead));
              } 
#if 0  /* since we don't have this queue anymore, there is nothing to flush.  Buffers are handled immediately */
            else if (pBuffPrivate->eBufferOwner == JPEGDEC_BUFFER_COMPONENT_OUT) {
		 OMX_PRBUFFER2(pComponentPrivate->dbg, "disgard %p from OutDir\n", pBuffPrivate->pBufferHdr);
                 nRet = read(pComponentPrivate->nFilled_outBuf_Q[0], &pBuffHead, sizeof(pBuffHead));
              } 
#endif            
	    OMX_PRBUFFER2(pComponentPrivate->dbg, "return output buffer %p in idle (%d)\n", pBuffPrivate->pBufferHdr, pBuffPrivate->eBufferOwner);
              pBuffPrivate->eBufferOwner = JPEGDEC_BUFFER_CLIENT;
              pComponentPrivate->cbInfo.FillBufferDone(pComponentPrivate->pHandle,
                                             pComponentPrivate->pHandle->pApplicationPrivate,
                                             pBuffPrivate->pBufferHdr);
            }
        }
        
        pComponentPrivate->bFlushComplete = OMX_FALSE;
    }

EXIT:
    return eError;

}



/* ========================================================================== */
/**
 * @fn HandleCommandFlush - Implements the functionality to send flush command.
 * @param pComponentPrivate - components private structure
 * @param nParam1 - paramerer specifying the port type (Index port)
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on failure
 */
/* ========================================================================== */
OMX_U32 HandleCommandFlush(JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate,
                           OMX_U32 nParam1)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U8 nCount = 0;
    OMX_COMPONENTTYPE* pHandle = NULL;
    LCML_DSP_INTERFACE* pLcmlHandle = NULL;
    OMX_U32 aParam[3];
    OMX_U8 nBuffersIn = 0;
    OMX_U8 nBuffersOut = 0;
#ifdef UNDER_CE
    OMX_U32 nTimeOut = 0;
#endif

    OMX_CHECK_PARAM(pComponentPrivate);
    JPEGDEC_OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);
    pHandle = pComponentPrivate->pHandle;
    
    nBuffersIn = pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef->nBufferCountActual;
    nBuffersOut = pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef->nBufferCountActual;

    if ( nParam1 == JPEGDEC_INPUT_PORT || (int)nParam1 == -1 )   {

        aParam[0] = USN_STRMCMD_FLUSH;
        aParam[1] = 0;
        aParam[2] = 0;
        pLcmlHandle = (LCML_DSP_INTERFACE*)(pComponentPrivate->pLCML);
        pComponentPrivate->bFlushComplete = OMX_FALSE;
        OMX_PRDSP2(pComponentPrivate->dbg, "pLcmlHandle %p\n", pLcmlHandle);
        eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,EMMCodecControlStrmCtrl, (void*)aParam);
        OMX_PRDSP2(pComponentPrivate->dbg, "eError %x\n", eError); 
        if (eError != OMX_ErrorNone) {
            goto PRINT_EXIT;
        }
#ifdef UNDER_CE
    nTimeOut = 0;
#endif
    while(pComponentPrivate->bFlushComplete == OMX_FALSE){
#ifdef UNDER_CE
        sched_yield();
         if (nTimeOut++ > 200000){
	    OMX_PRDSP4(pComponentPrivate->dbg, "Flushing Input port timeout Error\n");
            break;
        }
#else
        JPEGDEC_WAIT_FLUSH(pComponentPrivate);
#endif
    }
    pComponentPrivate->bFlushComplete = OMX_FALSE;

        for (nCount = 0; nCount < pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef->nBufferCountActual; nCount++) {
              JPEGDEC_BUFFER_PRIVATE* pBuffPrivate = pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pBufferPrivate[nCount];

              if (pBuffPrivate->eBufferOwner != JPEGDEC_BUFFER_CLIENT) {
                OMX_BUFFERHEADERTYPE* pBuffHead = NULL;
                int nRet;
#ifdef __PERF_INSTRUMENTATION__
                    PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                      PREF(((OMX_BUFFERHEADERTYPE*) pComponentPrivate->pCompPort[0]->pBufferPrivate[nCount]->pBufferHdr), pBuffer),
                                      PREF(((OMX_BUFFERHEADERTYPE*) pComponentPrivate->pCompPort[0]->pBufferPrivate[nCount]->pBufferHdr), nFilledLen),
                                      PERF_ModuleLLMM);
#endif
        
                if (pBuffPrivate->eBufferOwner == JPEGDEC_BUFFER_COMPONENT_IN) {
		     OMX_PRBUFFER2(pComponentPrivate->dbg, "disgard %p from InDir\n", pBuffPrivate->pBufferHdr);
                     nRet = read(pComponentPrivate->nFilled_inpBuf_Q[0], &pBuffHead, sizeof(pBuffHead));
                  } 
                  pBuffPrivate->eBufferOwner = JPEGDEC_BUFFER_CLIENT;
		  OMX_PRBUFFER2(pComponentPrivate->dbg, "return input buffer %p in idle\n", pBuffPrivate->pBufferHdr);
                  pComponentPrivate->cbInfo.EmptyBufferDone(pComponentPrivate->pHandle,
                                                 pComponentPrivate->pHandle->pApplicationPrivate,
                                                 pBuffPrivate->pBufferHdr);
              }
        }

        pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                pComponentPrivate->pHandle->pApplicationPrivate, 
                                                OMX_EventCmdComplete,
                                                OMX_CommandFlush,
                                                JPEGDEC_INPUT_PORT, 
                                                NULL);
        }

    
    if ( nParam1 == JPEGDEC_OUTPUT_PORT|| (int)nParam1 == -1 ){
        /* return all output buffers */
        aParam[0] = USN_STRMCMD_FLUSH;
        aParam[1] = 1;
        aParam[2] = 0;
        pLcmlHandle = (LCML_DSP_INTERFACE*)pComponentPrivate->pLCML;
        pComponentPrivate->bFlushComplete = OMX_FALSE;
        OMX_PRDSP2(pComponentPrivate->dbg, "pLcmlHandle %p\n", pLcmlHandle);
        eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,EMMCodecControlStrmCtrl, (void*)aParam);
	OMX_PRDSP2(pComponentPrivate->dbg, "eError %x\n", eError); 
       if (eError != OMX_ErrorNone) {
            goto PRINT_EXIT;
        }
#ifdef UNDER_CE
        nTimeOut = 0;
#endif
        while (pComponentPrivate->bFlushComplete == OMX_FALSE) {
#ifdef UNDER_CE
            sched_yield();
            if (nTimeOut++ > 200000) {
		    OMX_PRDSP4(pComponentPrivate->dbg, "Flushing Input port timeout Error\n");
                break;
            }
#else
            JPEGDEC_WAIT_FLUSH(pComponentPrivate);
#endif
        }

        pComponentPrivate->bFlushComplete = OMX_FALSE;

        for (nCount = 0; nCount < pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef->nBufferCountActual; nCount++) {
            JPEGDEC_BUFFER_PRIVATE* pBuffPrivate = pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pBufferPrivate[nCount];

        if (pBuffPrivate->eBufferOwner != JPEGDEC_BUFFER_CLIENT) {
            OMX_BUFFERHEADERTYPE* pBuffHead = NULL;
            int nRet;
#ifdef __PERF_INSTRUMENTATION__
                PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                  PREF(((OMX_BUFFERHEADERTYPE*) pComponentPrivate->pCompPort[1]->pBufferPrivate[nCount]->pBufferHdr), pBuffer),
                                  PREF(((OMX_BUFFERHEADERTYPE*) pComponentPrivate->pCompPort[1]->pBufferPrivate[nCount]->pBufferHdr), nFilledLen),
                                  PERF_ModuleLLMM);
#endif

            if (pBuffPrivate->eBufferOwner == JPEGDEC_BUFFER_COMPONENT_IN) {
		OMX_PRBUFFER2(pComponentPrivate->dbg, "disgard %p from InDir\n", pBuffPrivate->pBufferHdr);
                pComponentPrivate->nOutPortOut ++;
                 nRet = read(pComponentPrivate->nFree_outBuf_Q[0], &pBuffHead, sizeof(pBuffHead));
            }

#if 0  /* since we don't have this queue anymore, there is nothing to flush.  Buffers are handled immediately */
            else if (pBuffPrivate->eBufferOwner == JPEGDEC_BUFFER_COMPONENT_OUT) {
		 OMX_PRBUFFER2(pComponentPrivate->dbg, "disgard %p from OutDir\n", pBuffPrivate->pBufferHdr);
                 nRet = read(pComponentPrivate->nFilled_outBuf_Q[0], &pBuffHead, sizeof(pBuffHead));
            }
#endif            
            OMX_PRBUFFER2(pComponentPrivate->dbg, "return output buffer %p in idle (%d)\n", pBuffPrivate->pBufferHdr, pBuffPrivate->eBufferOwner);
            pBuffPrivate->eBufferOwner = JPEGDEC_BUFFER_CLIENT;
            pComponentPrivate->cbInfo.FillBufferDone(pComponentPrivate->pHandle,
                                         pComponentPrivate->pHandle->pApplicationPrivate,
                                         pBuffPrivate->pBufferHdr);
            }
        }

        pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                       OMX_EventCmdComplete,
                                       OMX_CommandFlush,
                                       JPEGDEC_OUTPUT_PORT,
                                       NULL);
    }
PRINT_EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, "Exiting HandleCommand nFlush Function\n");
EXIT:
    return eError;

}   /* End of HandleCommandFlush */



/* ========================================================================== */
/**
 * @fn HandleCommandJpegDec - andle State type commands. Depending on the
 *  State Command received it executes the corresponding code.
 * @param pComponentPrivate - components private structure
 * @param nParam1 - state to change.
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on failure
 */
/* ========================================================================== */
OMX_U32 HandleCommandJpegDec(JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate,
                             OMX_U32 nParam1)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefIn = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefOut = NULL;
    OMX_HANDLETYPE pLcmlHandle = NULL;
    LCML_DSP *lcml_dsp;
    OMX_U16 arr[100];
    LCML_CALLBACKTYPE cb;
    OMX_U8 nCount = 0;
    int    nBufToReturn;
#ifdef RESOURCE_MANAGER_ENABLED
    OMX_U16 nMHzRM = 0;
    OMX_U32 lImageResolution = 0;
#endif

    JPEGDEC_OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);

    OMX_CHECK_PARAM(pComponentPrivate);
    pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;
    pPortDefIn = pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef;
    pPortDefOut= pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef;


    switch ((OMX_STATETYPE)(nParam1))
    {
    case OMX_StateIdle:
		OMX_PRINT2(pComponentPrivate->dbg, "JPEG HandleCommand: Cmd Idle \n");
        OMX_PRSTATE2(pComponentPrivate->dbg, "CURRENT STATE IS %d\n",pComponentPrivate->nCurState);
        if (pComponentPrivate->nCurState == OMX_StateIdle) {
            eError = OMX_ErrorSameState;
            break;
        }
        else if ((pComponentPrivate->nCurState == OMX_StateLoaded) ||
                    (pComponentPrivate->nCurState == OMX_StateWaitForResources)) {

#ifdef __PERF_INSTRUMENTATION__
            PERF_Boundary(pComponentPrivate->pPERFcomp,
                           PERF_BoundaryStart | PERF_BoundarySetup);
#endif

            OMX_PRSTATE2(pComponentPrivate->dbg, "Transition state from loaded to idle\n");

#ifdef RESOURCE_MANAGER_ENABLED /* Resource Manager Proxy Calls */
            pComponentPrivate->rmproxyCallback.RMPROXY_Callback = (void *)ResourceManagerCallback;
            lImageResolution = pPortDefIn->format.image.nFrameWidth * pPortDefIn->format.image.nFrameHeight;
            OMX_GET_RM_VALUE(lImageResolution, nMHzRM);
            OMX_PRMGR2(pComponentPrivate->dbg, "Value sent to RM = %d\n", nMHzRM);
            if (pComponentPrivate->nCurState != OMX_StateWaitForResources) {

                eError = RMProxy_NewSendCommand(pHandle, RMProxy_RequestResource, OMX_JPEG_Decoder_COMPONENT, nMHzRM, 3456, &(pComponentPrivate->rmproxyCallback));

                if (eError != OMX_ErrorNone) {
                    /* resource is not available, need set state to OMX_StateWaitForResources*/
                    OMX_PRMGR4(pComponentPrivate->dbg, "Resource is not available\n");
                    eError = OMX_ErrorInsufficientResources;
                    break;
                }
            }
#endif

            if ((pPortDefIn->bEnabled == OMX_TRUE) &&
                (pPortDefOut->bEnabled == OMX_TRUE)) {

                while (1) {
                    if ((pPortDefIn->bPopulated) && (pPortDefOut->bPopulated)) {
                        break;
                    }
                    else {
                        JPEGDEC_WAIT_PORT_POPULATION(pComponentPrivate);

                    }
                }
                if(eError != OMX_ErrorNone){
                    OMX_PRBUFFER4(pComponentPrivate->dbg, "Port population time out\n");
                    goto PRINT_EXIT;
                }
            }

            eError =  GetLCMLHandleJpegDec(pHandle);
            if (eError != OMX_ErrorNone) {
		OMX_PRDSP5(pComponentPrivate->dbg, "GetLCMLHandle failed...\n");
                goto PRINT_EXIT;
            }

            pLcmlHandle =(LCML_DSP_INTERFACE*)pComponentPrivate->pLCML;
            lcml_dsp = (((LCML_DSP_INTERFACE*)pLcmlHandle)->dspCodec);

            OMX_PRDSP2(pComponentPrivate->dbg, "Fill_LCMLInitParams in JPEG\n");
            Fill_LCMLInitParamsJpegDec(lcml_dsp,arr, pHandle);

            cb.LCML_Callback = (void *) LCML_CallbackJpegDec;

            if (pComponentPrivate->nIsLCMLActive == 1) {
		OMX_PRDSP2(pComponentPrivate->dbg, "nIsLCMLActive is active\n");
            }
            /*calling initMMCodec to init codec with details filled earlier */
            eError = LCML_InitMMCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle, NULL, &pLcmlHandle, NULL, &cb);
            if (eError != OMX_ErrorNone) {
	        OMX_PRDSP4(pComponentPrivate->dbg, "InitMMCodec failed...\n");
                goto PRINT_EXIT;
            }
            else {
                pComponentPrivate->nIsLCMLActive = 1;
            }
            OMX_PRDSP1(pComponentPrivate->dbg, "LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle %p\n" , ((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle);
            eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle, EMMCodecControlUsnEos, NULL);
            if (eError != OMX_ErrorNone) {
                OMX_PRDSP4(pComponentPrivate->dbg, "Enable EOS at LCML failed...\n");
                goto PRINT_EXIT;
            }
            /* need check the resource with RM */

#ifdef __PERF_INSTRUMENTATION__
            PERF_Boundary(pComponentPrivate->pPERFcomp,
                          PERF_BoundaryComplete | PERF_BoundarySetup);
#endif

#ifdef RESOURCE_MANAGER_ENABLED
            eError= RMProxy_NewSendCommand(pHandle, RMProxy_StateSet, OMX_JPEG_Decoder_COMPONENT, OMX_StateIdle,  3456, NULL);
            if (eError != OMX_ErrorNone) {
		OMX_PRMGR4(pComponentPrivate->dbg, "Resources not available Loaded ->Idle\n");
                break;
            }
#endif
            pComponentPrivate->nCurState = OMX_StateIdle;

            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                   OMX_EventCmdComplete,
                                                   OMX_CommandStateSet,
                                                   pComponentPrivate->nCurState,
                                                   NULL);
            break;
            OMX_PRSTATE2(pComponentPrivate->dbg, "JPEGDEC: State has been Set to Idle\n");
        }
        else if ((pComponentPrivate->nCurState == OMX_StateExecuting) ||
                 (pComponentPrivate->nCurState == OMX_StatePause)) {
/*            if (pComponentPrivate->bPreempted == 1){
                    eError = OMX_ErrorResourcesPreempted;
            }
*/            
            nCount = 0;
            pComponentPrivate->ExeToIdleFlag = 0;
            OMX_PRDSP2(pComponentPrivate->dbg, "OMX_StateIdle->OMX_StateExecuting-THE CODEC IS STOPPING!!!\n");
            pLcmlHandle =(LCML_DSP_INTERFACE*)pComponentPrivate->pLCML;
            eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle, MMCodecControlStop, NULL);
#ifdef __PERF_INSTRUMENTATION__
            PERF_Boundary(pComponentPrivate->pPERFcomp,
                          PERF_BoundaryComplete | PERF_BoundarySteadyState);
#endif

	    OMX_TRACE2(pComponentPrivate->dbg, "before stop lock\n");
        pthread_mutex_lock(&pComponentPrivate->mJpegDecMutex);
        while ((pComponentPrivate->ExeToIdleFlag & JPEGD_DSPSTOP) == 0) {
            pthread_cond_wait(&pComponentPrivate->sStop_cond, &pComponentPrivate->mJpegDecMutex);
        }
        pthread_mutex_unlock(&pComponentPrivate->mJpegDecMutex);


        nBufToReturn = 0;
        if ((pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->hTunnelComponent != NULL)  &&
                (pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pParamBufSupplier->eBufferSupplier == OMX_BufferSupplyInput)) {
                for (nCount = 0; nCount < pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef->nBufferCountActual ; nCount++) {
                   JPEGDEC_BUFFER_PRIVATE* pBuffPrivate = pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pBufferPrivate[nCount];
                   OMX_PRBUFFER2(pComponentPrivate->dbg, "Jpeg Returning buffers to Display\n");

                   if (pBuffPrivate->eBufferOwner != JPEGDEC_BUFFER_CLIENT) {
                       OMX_BUFFERHEADERTYPE* pBuffHead = NULL;
                       int nRet;

#ifdef __PERF_INSTRUMENTATION__
                    PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                      PREF(((OMX_BUFFERHEADERTYPE*) pComponentPrivate->pCompPort[1]->pBufferPrivate[nCount]->pBufferHdr), pBuffer),
                                      PREF(((OMX_BUFFERHEADERTYPE*) pComponentPrivate->pCompPort[1]->pBufferPrivate[nCount]->pBufferHdr), nFilledLen),
                                      PERF_ModuleLLMM);
#endif

                       if (pBuffPrivate->eBufferOwner == JPEGDEC_BUFFER_COMPONENT_IN) {
			  OMX_PRBUFFER2(pComponentPrivate->dbg, "disgard %p from InDir\n", pBuffPrivate->pBufferHdr);
                          nRet = read(pComponentPrivate->nFree_outBuf_Q[0], &pBuffHead, sizeof(pBuffHead));
                       } 

#if 0  /* since we don't have this queue anymore, there is nothing to discard.  Buffers are handled immediately */
                       else if (pBuffPrivate->eBufferOwner == JPEGDEC_BUFFER_COMPONENT_OUT) {
			  OMX_PRBUFFER2(pComponentPrivate->dbg, "discard %p from OutDir\n", pBuffPrivate->pBufferHdr);
                          nRet = read(pComponentPrivate->nFilled_outBuf_Q[0], &pBuffHead, sizeof(pBuffHead));
                       } 
#endif                       
                       OMX_PRBUFFER2(pComponentPrivate->dbg, "return output buffer %p in idle (%d)\n", pBuffPrivate->pBufferHdr, pBuffPrivate->eBufferOwner);
                       pBuffPrivate->eBufferOwner = JPEGDEC_BUFFER_CLIENT;

                       eError = OMX_EmptyThisBuffer(pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->hTunnelComponent,
                                    (OMX_BUFFERHEADERTYPE*) pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pBufferPrivate[nCount]->pBufferHdr);
                    }
                 }
        }
        else { /* output port is not tunneled */
            for (nCount = 0; nCount < pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef->nBufferCountActual; nCount++) {
              JPEGDEC_BUFFER_PRIVATE* pBuffPrivate = pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pBufferPrivate[nCount];

              if (pBuffPrivate->eBufferOwner != JPEGDEC_BUFFER_CLIENT) {
                OMX_BUFFERHEADERTYPE* pBuffHead = NULL;
                int nRet;
#ifdef __PERF_INSTRUMENTATION__
                    PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                      PREF(((OMX_BUFFERHEADERTYPE*) pComponentPrivate->pCompPort[1]->pBufferPrivate[nCount]->pBufferHdr), pBuffer),
                                      PREF(((OMX_BUFFERHEADERTYPE*) pComponentPrivate->pCompPort[1]->pBufferPrivate[nCount]->pBufferHdr), nFilledLen),
                                      PERF_ModuleLLMM);
#endif
        
                if (pBuffPrivate->eBufferOwner == JPEGDEC_BUFFER_COMPONENT_IN) {
		     OMX_PRBUFFER2(pComponentPrivate->dbg, "discard %p from InDir\n", pBuffPrivate->pBufferHdr);
                     pComponentPrivate->nOutPortOut ++;
                     nRet = read(pComponentPrivate->nFree_outBuf_Q[0], &pBuffHead, sizeof(pBuffHead));
                }
#if 0  /* since we don't have this queue anymore, there is nothing to discard.  Buffers are handled immediately */                
                else if (pBuffPrivate->eBufferOwner == JPEGDEC_BUFFER_COMPONENT_OUT) {
		     OMX_PRBUFFER2(pComponentPrivate->dbg, "discard %p from OutDir\n", pBuffPrivate->pBufferHdr);
                     nRet = read(pComponentPrivate->nFilled_outBuf_Q[0], &pBuffHead, sizeof(pBuffHead));
                }
#endif                
                OMX_PRBUFFER2(pComponentPrivate->dbg, "return output buffer %p in idle (%d)\n", pBuffPrivate->pBufferHdr, pBuffPrivate->eBufferOwner);
                pBuffPrivate->eBufferOwner = JPEGDEC_BUFFER_CLIENT;
                pComponentPrivate->cbInfo.FillBufferDone(pComponentPrivate->pHandle,
                                                 pComponentPrivate->pHandle->pApplicationPrivate,
                                                 pBuffPrivate->pBufferHdr);
               }
            }
        }

        OMX_PRBUFFER2(pComponentPrivate->dbg, "all buffers are returned\n");

        nBufToReturn = 0;
        for (nCount = 0; nCount < pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef->nBufferCountActual; nCount++) {
              JPEGDEC_BUFFER_PRIVATE* pBuffPrivate = pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pBufferPrivate[nCount];

              if (pBuffPrivate->eBufferOwner != JPEGDEC_BUFFER_CLIENT) {
                OMX_BUFFERHEADERTYPE* pBuffHead = NULL;
                int nRet;
#ifdef __PERF_INSTRUMENTATION__
                    PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                      PREF(((OMX_BUFFERHEADERTYPE*) pComponentPrivate->pCompPort[0]->pBufferPrivate[nCount]->pBufferHdr), pBuffer),
                                      PREF(((OMX_BUFFERHEADERTYPE*) pComponentPrivate->pCompPort[0]->pBufferPrivate[nCount]->pBufferHdr), nFilledLen),
                                      PERF_ModuleLLMM);
#endif
        
                  nBufToReturn ++;
                  if (pBuffPrivate->eBufferOwner == JPEGDEC_BUFFER_COMPONENT_IN) {
		     OMX_PRBUFFER2(pComponentPrivate->dbg, "disgard %p from InDir\n", pBuffPrivate->pBufferHdr);
                     nRet = read(pComponentPrivate->nFilled_inpBuf_Q[0], &pBuffHead, sizeof(pBuffHead));
                  } 
                  pBuffPrivate->eBufferOwner = JPEGDEC_BUFFER_CLIENT;
                  OMX_PRBUFFER2(pComponentPrivate->dbg, "return input buffer %p in idle\n", pBuffPrivate->pBufferHdr);
                  pComponentPrivate->cbInfo.EmptyBufferDone(pComponentPrivate->pHandle,
                                                 pComponentPrivate->pHandle->pApplicationPrivate,
                                                 pBuffPrivate->pBufferHdr);
               }
        }

#ifdef RESOURCE_MANAGER_ENABLED
            eError= RMProxy_NewSendCommand(pHandle, RMProxy_StateSet, OMX_JPEG_Decoder_COMPONENT, OMX_StateIdle, 3456, NULL);
            if (eError != OMX_ErrorNone) {
                OMX_PRMGR4(pComponentPrivate->dbg, "Resources not available Executing ->Idle\n");
                pComponentPrivate->nCurState = OMX_StateWaitForResources;
                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandStateSet,
                                                       pComponentPrivate->nCurState,
                                                       NULL);
                eError = OMX_ErrorNone;
                break;
            }
#endif
         pComponentPrivate->nCurState = OMX_StateIdle;
         OMX_PRSTATE2(pComponentPrivate->dbg, "current state is %d\n", pComponentPrivate->nCurState);
         pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                   OMX_EventCmdComplete,
                                                   OMX_CommandStateSet,
                                                   OMX_StateIdle,
                                                   NULL);
         pComponentPrivate->ExeToIdleFlag = 0;
         OMX_PRSTATE2(pComponentPrivate->dbg, "JPEG-DEC in idle\n");
        }
        else { /* This means, it is invalid state from application */
	    OMX_PRSTATE4(pComponentPrivate->dbg, "Error: Invalid State Given by Application\n");
            eError = OMX_ErrorInvalidState;
        }
        break;

    case OMX_StateExecuting:
	OMX_PRINT1(pComponentPrivate->dbg, "HandleCommand: Cmd Executing \n");
        if (pComponentPrivate->nCurState == OMX_StateExecuting) {
            eError = OMX_ErrorSameState;
        }
        else if (pComponentPrivate->nCurState == OMX_StateIdle ||
                  pComponentPrivate->nCurState == OMX_StatePause) {

#ifdef __PERF_INSTRUMENTATION__
            PERF_Boundary(pComponentPrivate->pPERFcomp,
                          PERF_BoundaryStart | PERF_BoundarySteadyState);
#endif

            pLcmlHandle =(LCML_DSP_INTERFACE*)pComponentPrivate->pLCML;
            eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle, EMMCodecControlStart, NULL);

            OMX_PRDSP2(pComponentPrivate->dbg, "eError is %x\n", eError);
#ifdef RESOURCE_MANAGER_ENABLED
            eError= RMProxy_NewSendCommand(pHandle, RMProxy_StateSet, OMX_JPEG_Decoder_COMPONENT, OMX_StateExecuting, 3456, NULL);
            if (eError != OMX_ErrorNone) {
		OMX_PRMGR4(pComponentPrivate->dbg, "Resources not available\n");
                pComponentPrivate->nCurState = OMX_StateWaitForResources;
                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandStateSet,
                                                       pComponentPrivate->nCurState,
                                                       NULL);
                eError = OMX_ErrorNone;
                break;
            }
#endif

            pComponentPrivate->nCurState = OMX_StateExecuting;
            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                   OMX_EventCmdComplete,
                                                   OMX_CommandStateSet,
                                                   pComponentPrivate->nCurState,
                                                   NULL);
	    OMX_PRSTATE2(pComponentPrivate->dbg, "JPEG-DEC in OMX_StateExecuting\n");
        }
        else {
            eError = OMX_ErrorIncorrectStateTransition;
        }
        break;


    case OMX_StatePause:
	OMX_PRINT1(pComponentPrivate->dbg, "HandleCommand: Cmd Pause\n");
        if (pComponentPrivate->nCurState == OMX_StatePause) {
            eError = OMX_ErrorSameState;
        }
        else if ((pComponentPrivate->nCurState == OMX_StateIdle) ||
                 (pComponentPrivate->nCurState == OMX_StateExecuting)) {
            pLcmlHandle =(LCML_DSP_INTERFACE*)pComponentPrivate->pLCML;
            eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle, EMMCodecControlPause, NULL);
            if (eError != OMX_ErrorNone) {
		OMX_PRDSP4(pComponentPrivate->dbg, "Error during EMMCodecControlPause.. error is %d.\n", eError);
                break;
            }

#ifdef __PERF_INSTRUMENTATION__
            PERF_Boundary(pComponentPrivate->pPERFcomp,
                          PERF_BoundaryComplete | PERF_BoundarySteadyState);
#endif

            pComponentPrivate->nCurState = OMX_StatePause;
        }
        else {
	    OMX_PRSTATE4(pComponentPrivate->dbg, "Error: Invalid State Given by Application\n");
            eError = OMX_ErrorIncorrectStateTransition;
        }
        break;

    case OMX_StateInvalid:
	OMX_PRINT1(pComponentPrivate->dbg, "HandleCommand: Cmd OMX_StateInvalid::\n");
        if (pComponentPrivate->nCurState == OMX_StateInvalid) {
            eError = OMX_ErrorSameState;
            break;
        }
        if (pComponentPrivate->nCurState == OMX_StateExecuting 
                || pComponentPrivate->nCurState == OMX_StatePause){
	    OMX_PRBUFFER2(pComponentPrivate->dbg, "HandleInternalFlush\n\n");
            eError = HandleInternalFlush(pComponentPrivate, OMX_ALL); /*OMX_ALL = -1 OpenMax 1.1*/
            if(eError != OMX_ErrorNone){
		OMX_PRBUFFER4(pComponentPrivate->dbg, "eError from HandleInternalFlush = %x\n", eError);
                eError = OMX_ErrorNone; /* Clean error, already sending the component to Invalid state*/
            }
        }
        OMX_PRSTATE2(pComponentPrivate->dbg, "OMX_StateInvalid\n\n");
        pComponentPrivate->nCurState = OMX_StateInvalid;

        if(pComponentPrivate->nToState == OMX_StateInvalid){ /*if the IL client call directly send to invalid state*/
            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                   OMX_EventCmdComplete, 
                                   OMX_CommandStateSet, 
                                   pComponentPrivate->nCurState, 
                                   NULL);
        }
        else{ /*When the component go to invalid state by it self*/
            eError = OMX_ErrorInvalidState;
        }
        break;

    case OMX_StateLoaded:
       OMX_PRSTATE2(pComponentPrivate->dbg, "go to loaded state\n");
       if (pComponentPrivate->nCurState == OMX_StateLoaded) {
            eError = OMX_ErrorSameState;
        }
        else if ((pComponentPrivate->nCurState == OMX_StateIdle) ||
                 (pComponentPrivate->nCurState == OMX_StateWaitForResources)) {

#ifdef __PERF_INSTRUMENTATION__
            PERF_Boundary(pComponentPrivate->pPERFcomp,
                          PERF_BoundaryStart | PERF_BoundaryCleanup);
#endif

#ifdef RESOURCE_MANAGER_ENABLED
            if (pComponentPrivate->nCurState == OMX_StateWaitForResources) {
                eError= RMProxy_NewSendCommand(pHandle,  RMProxy_CancelWaitForResource, OMX_JPEG_Decoder_COMPONENT, 0, 3456, NULL);
                if (eError != OMX_ErrorNone) {
		    OMX_PRMGR4(pComponentPrivate->dbg, "CancelWaitForResource Failed\n");
                    break;
                }
            }
            
#endif

            /* Ports have to be unpopulated before transition completes */
            while (1) {
                if ((!pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef->bPopulated) &&
                        (!pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef->bPopulated)) {
                    break;
                }
                else {
                    JPEGDEC_WAIT_PORT_UNPOPULATION(pComponentPrivate);
                }
            }
            if (eError != OMX_ErrorNone){ /*Verify if UnPopulation compleate*/
                goto PRINT_EXIT;
            }

#ifdef RESOURCE_MANAGER_ENABLED
            if (pComponentPrivate->nCurState != OMX_StateWaitForResources) {
                eError= RMProxy_NewSendCommand(pHandle,  RMProxy_FreeResource, OMX_JPEG_Decoder_COMPONENT, 0, 3456, NULL);
                if (eError != OMX_ErrorNone) {
		    OMX_PRMGR4(pComponentPrivate->dbg, "Cannot Free Resources\n");
                    break;
                }
            }
#endif

            if ((pComponentPrivate->pLCML != NULL) &&
                (pComponentPrivate->nIsLCMLActive == 1)) {
                pLcmlHandle =(LCML_DSP_INTERFACE*)pComponentPrivate->pLCML;
                LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle, EMMCodecControlDestroy, NULL);
                pComponentPrivate->pLCML = NULL;
                pComponentPrivate->nIsLCMLActive = 0;
#ifdef UNDER_CE
                FreeLibrary(g_hLcmlDllHandle);
                g_hLcmlDllHandle = NULL;
#else
                dlclose(pComponentPrivate->pDllHandle);
                pComponentPrivate->pDllHandle = NULL;
#endif
            }


#ifdef __PERF_INSTRUMENTATION__
            PERF_Boundary(pComponentPrivate->pPERFcomp,
                          PERF_BoundaryComplete | PERF_BoundaryCleanup);
#endif

            /*Restart Buffer counting*/
            pComponentPrivate->nInPortIn = 0;
            pComponentPrivate->nOutPortOut = 0;

            pComponentPrivate->nCurState = OMX_StateLoaded;            
            
            if ((pComponentPrivate->nCurState == OMX_StateIdle) &&
                 (pComponentPrivate->bPreempted == 1 )){
                pComponentPrivate->bPreempted = 0;
                eError = OMX_ErrorResourcesLost;
            }
            else {
                pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandStateSet,
                                                       OMX_StateLoaded,
                                                       NULL);
            }
       }
        else {
            eError = OMX_ErrorIncorrectStateTransition;
        }
        break;

    case OMX_StateWaitForResources:

        if (pComponentPrivate->nCurState == OMX_StateWaitForResources) {
            eError = OMX_ErrorSameState;
        }
        else if (pComponentPrivate->nCurState == OMX_StateLoaded) {
            
#ifdef RESOURCE_MANAGER_ENABLED
            eError= RMProxy_NewSendCommand(pHandle, RMProxy_StateSet, OMX_JPEG_Decoder_COMPONENT, OMX_StateWaitForResources, 3456, NULL);
            if (eError != OMX_ErrorNone) {
		OMX_PRMGR4(pComponentPrivate->dbg, "RMProxy_NewSendCommand(OMX_StateWaitForResources) failed\n");
                break;
            }
#endif
            
            pComponentPrivate->nCurState = OMX_StateWaitForResources;
            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                   OMX_EventCmdComplete,
                                                   OMX_CommandStateSet,
                                                   pComponentPrivate->nCurState,
                                                   NULL);
        }
        else {
            eError = OMX_ErrorIncorrectStateTransition;
        }
        break;

    case OMX_StateMax:
        OMX_PRINT2(pComponentPrivate->dbg, "HandleCommand: Cmd OMX_StateMax::\n"); 
        break;
    } /* End of Switch */

PRINT_EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, "Exiting HandleCommand Function %x\n", eError);
EXIT:
    return eError;
} 
  /* End of HandleCommandJpegDec */

/* ========================================================================== */
/**
 * @fn HandleFreeOutputBufferFromAppJpegDec - Handle free output buffer from
 *  application reading in the nFree_outBuf_Q pipe, and queue to the LCML.
 * @param pComponentPrivate - components private structure
 * @param nParam1 - state to change.
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on failure
 */
/* ========================================================================== */
OMX_ERRORTYPE HandleFreeOutputBufferFromAppJpegDec(JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE* pBuffHead = NULL;
    JPEGDEC_UAlgOutBufParamStruct *ptJPGDecUALGOutBufParam = NULL;
    LCML_DSP_INTERFACE* pLcmlHandle = NULL;
    JPEGDEC_BUFFER_PRIVATE* pBuffPrivate = NULL;
    int nRet;

    OMX_CHECK_PARAM(pComponentPrivate);
    JPEGDEC_OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);
    pLcmlHandle = (LCML_DSP_INTERFACE*)pComponentPrivate->pLCML;

    OMX_PRBUFFER2(pComponentPrivate->dbg, "%s: read outport (in) buff header %p\n", __FUNCTION__, pBuffHead);

    nRet = read(pComponentPrivate->nFree_outBuf_Q[0], &pBuffHead, sizeof(pBuffHead));

    if (nRet == -1) {
	OMX_PRCOMM4(pComponentPrivate->dbg, "Error while reading from the pipe\n");
        goto EXIT;
    }



    pBuffPrivate = pBuffHead->pOutputPortPrivate;

    if ((pComponentPrivate->nCurState == OMX_StateIdle) || (pComponentPrivate->nToState == OMX_StateIdle)) {
        if (pBuffPrivate->eBufferOwner != JPEGDEC_BUFFER_CLIENT) {
	OMX_PRBUFFER2(pComponentPrivate->dbg, "Going to state %d, return buffer %p to client\n", pComponentPrivate->nToState, pBuffHead);
        pBuffPrivate->eBufferOwner = JPEGDEC_BUFFER_CLIENT;
        pComponentPrivate->nOutPortOut ++;
        pComponentPrivate->cbInfo.FillBufferDone(pComponentPrivate->pHandle,
                    pComponentPrivate->pHandle->pApplicationPrivate,
                    pBuffHead);
        }
        goto EXIT;
    }


    ptJPGDecUALGOutBufParam = (JPEGDEC_UAlgOutBufParamStruct *)pBuffPrivate->pUALGParams;
    ptJPGDecUALGOutBufParam->lOutBufCount = 0;
    ptJPGDecUALGOutBufParam->ulOutNumFrame = 1;
    ptJPGDecUALGOutBufParam->ulOutFrameAlign = 4;
    ptJPGDecUALGOutBufParam->ulOutFrameSize = pBuffHead->nAllocLen;

    pBuffPrivate->eBufferOwner = JPEGDEC_BUFFER_DSP;

#ifdef __PERF_INSTRUMENTATION__
    PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                      pBuffHead->pBuffer,
                      pBuffHead->nFilledLen,
                      PERF_ModuleCommonLayer);
#endif

    eError = LCML_QueueBuffer(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                              EMMCodecOuputBuffer,
                              pBuffHead->pBuffer,
                              pBuffHead->nAllocLen,
                              pBuffHead->nFilledLen,
                              (OMX_U8*)ptJPGDecUALGOutBufParam,
                              sizeof(JPEGDEC_UAlgOutBufParamStruct),
                              (OMX_U8*)pBuffHead);
    if (eError != OMX_ErrorNone) {
        goto EXIT;
    }
EXIT:
    return eError;
}   /* end of HandleFreeOutputBufferFromAppJpegDec */


/* ========================================================================== */
/**
 * @fn HandleDataBuf_FromAppJpegDec - Handle data to be encoded form
 *  application and queue to the LCML.
 * @param pComponentPrivate - components private structure
 * @param nParam1 - state to change.
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on failure
 */
/* ========================================================================== */
OMX_ERRORTYPE HandleDataBuf_FromAppJpegDec(JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE* pBuffHead =  NULL;
    LCML_DSP_INTERFACE* pLcmlHandle = NULL;
    JPEGDEC_UAlgInBufParamStruct *ptJPGDecUALGInBufParam = NULL;
    JPEGDEC_BUFFER_PRIVATE* pBuffPrivate = NULL;
    int nRet;

    OMX_CHECK_PARAM(pComponentPrivate);
    JPEGDEC_OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);
    pLcmlHandle = (LCML_DSP_INTERFACE*)pComponentPrivate->pLCML;

    nRet = read(pComponentPrivate->nFilled_inpBuf_Q[0], &(pBuffHead), sizeof(pBuffHead));
    if (nRet == -1) {
	OMX_PRCOMM4(pComponentPrivate->dbg, "Error while reading from the pipe\n");
    }

    OMX_PRBUFFER2(pComponentPrivate->dbg, "HandleDataBuf_FromAppJpegDec: read inport (in) buff header %p\n", pBuffHead);

    pBuffPrivate = pBuffHead->pInputPortPrivate;
    if ((pComponentPrivate->nCurState == OMX_StateIdle) || (pComponentPrivate->nToState == OMX_StateIdle)) {
        pBuffPrivate->eBufferOwner = JPEGDEC_BUFFER_CLIENT;
        OMX_PRBUFFER2(pComponentPrivate->dbg, "Going to state %d, return buffer %p to client\n", pComponentPrivate->nToState, pBuffHead);

        pComponentPrivate->cbInfo.EmptyBufferDone(pComponentPrivate->pHandle,
                    pComponentPrivate->pHandle->pApplicationPrivate,
                    pBuffHead);
        goto EXIT;
    }

    ptJPGDecUALGInBufParam = (JPEGDEC_UAlgInBufParamStruct *)pBuffPrivate->pUALGParams;
    ptJPGDecUALGInBufParam->ulAlphaRGB = 0xFF;
    ptJPGDecUALGInBufParam->lInBufCount = 0;
    ptJPGDecUALGInBufParam->ulInNumFrame = 1;
    ptJPGDecUALGInBufParam->ulInFrameAlign = 4;
    ptJPGDecUALGInBufParam->ulInFrameSize = pBuffHead->nFilledLen;
    ptJPGDecUALGInBufParam->ulInDisplayWidth = (int)pComponentPrivate->nInputFrameWidth;
    ptJPGDecUALGInBufParam->ulInResizeOption = (int)pComponentPrivate->pScalePrivate->xWidth;
    /*Slide decode*/
    ptJPGDecUALGInBufParam->ulNumMCURow = (int)pComponentPrivate->pSectionDecode->nMCURow;
    ptJPGDecUALGInBufParam->ulnumAU = (int)pComponentPrivate->pSectionDecode->nAU;
    /*Section decode*/
    ptJPGDecUALGInBufParam->ulXOrg = (int)pComponentPrivate->pSubRegionDecode->nXOrg;
    ptJPGDecUALGInBufParam->ulYOrg = (int)pComponentPrivate->pSubRegionDecode->nYOrg;
    ptJPGDecUALGInBufParam->ulXLength = (int)pComponentPrivate->pSubRegionDecode->nXLength;
    ptJPGDecUALGInBufParam->ulYLength = (int)pComponentPrivate->pSubRegionDecode->nYLength;
    

    if (pComponentPrivate->nOutputColorFormat == OMX_COLOR_FormatCbYCrY) {
        ptJPGDecUALGInBufParam->forceChromaFormat= 4;
        ptJPGDecUALGInBufParam->RGB_Format = 9; /*RGB_Format should be set even if it's not use*/
    }
    else if (pComponentPrivate->nOutputColorFormat == OMX_COLOR_Format16bitRGB565) {
        ptJPGDecUALGInBufParam->forceChromaFormat =  9;
        ptJPGDecUALGInBufParam->RGB_Format = 9;
    }
    else if (pComponentPrivate->nOutputColorFormat == OMX_COLOR_Format24bitRGB888) {
        ptJPGDecUALGInBufParam->forceChromaFormat = 10;
        ptJPGDecUALGInBufParam->RGB_Format = 10;
    }
    else if (pComponentPrivate->nOutputColorFormat == OMX_COLOR_Format32bitARGB8888 ||
			pComponentPrivate->nOutputColorFormat == OMX_COLOR_Format32bitBGRA8888 ) {
        ptJPGDecUALGInBufParam->forceChromaFormat = 11;
        ptJPGDecUALGInBufParam->RGB_Format = 11;
    }
    else { /*Set DEFAULT (Original) color format*/
        ptJPGDecUALGInBufParam->forceChromaFormat = 1;
        ptJPGDecUALGInBufParam->RGB_Format = 9; /*RGB_Format should be set even if it's not use*/
    }
    OMX_PRDSP0(pComponentPrivate->dbg, "ptJPGDecUALGInBufParam->forceChromaFormat = %lu\n", ptJPGDecUALGInBufParam->forceChromaFormat );
    pBuffPrivate->eBufferOwner = JPEGDEC_BUFFER_DSP;

#ifdef __PERF_INSTRUMENTATION__
    PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                      pBuffHead->pBuffer,
                      pBuffHead->nFilledLen,
                      PERF_ModuleCommonLayer);
#endif

    OMX_PRDSP0(pComponentPrivate->dbg, "forceChromaFormat\t= %lu\n", ptJPGDecUALGInBufParam->forceChromaFormat);
    OMX_PRDSP0(pComponentPrivate->dbg, "RGB_Format\t= %lu\n", ptJPGDecUALGInBufParam->RGB_Format);
    OMX_PRDSP0(pComponentPrivate->dbg, "ulInFrameSize\t= %lu\n", ptJPGDecUALGInBufParam->ulInFrameSize);
    OMX_PRDSP0(pComponentPrivate->dbg, "ulInDisplayWidth\t= %lu\n", ptJPGDecUALGInBufParam->ulInDisplayWidth);
    OMX_PRDSP0(pComponentPrivate->dbg, "ulInResizeOption\t= %lu\n", ptJPGDecUALGInBufParam->ulInResizeOption);
    OMX_PRDSP0(pComponentPrivate->dbg, "ulNumMCURow\t= %lu\n", ptJPGDecUALGInBufParam->ulNumMCURow);
    OMX_PRDSP0(pComponentPrivate->dbg, "ulnumAU\t= %lu\n", ptJPGDecUALGInBufParam->ulnumAU);
    OMX_PRDSP0(pComponentPrivate->dbg, "ulXOrg\t= %lu    ", ptJPGDecUALGInBufParam->ulXOrg);
    OMX_PRDSP0(pComponentPrivate->dbg, "ulYOrg\t= %lu\n", ptJPGDecUALGInBufParam->ulYOrg);
    OMX_PRDSP0(pComponentPrivate->dbg, "ulXLength\t= %lu    ", ptJPGDecUALGInBufParam->ulXLength);
    OMX_PRDSP0(pComponentPrivate->dbg, "ulXLenght\t= %lu\n", ptJPGDecUALGInBufParam->ulYLength);
    OMX_PRBUFFER0(pComponentPrivate->dbg, "pBuffHead->nFlags\t= %lu\n", pBuffHead->nFlags);
    OMX_PRBUFFER0(pComponentPrivate->dbg, "Queue INPUT bufheader %p\n", pBuffHead);
    eError = LCML_QueueBuffer(pLcmlHandle->pCodecinterfacehandle,
                              EMMCodecInputBuffer,
                              pBuffHead->pBuffer,
                              pBuffHead->nAllocLen,
                              pBuffHead->nFilledLen,
                              (OMX_U8 *) ptJPGDecUALGInBufParam,
                              sizeof(JPEGDEC_UAlgInBufParamStruct),
                              (OMX_U8 *)pBuffHead);

    if (eError != OMX_ErrorNone) {
        goto EXIT;
    }

EXIT:
    return eError;
}   /* End of HandleDataBuf_FromAppJpegDec */


/* ========================================================================== */
/**
 * @fn HandleDataBuf_FromDspJpegDec - Handle encoded data form DSP and
 *  render to application or another component.
 * @param pComponentPrivate - components private structure
 * @param nParam1 - state to change.
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on failure
 */
/* ========================================================================== */
OMX_ERRORTYPE HandleDataBuf_FromDspJpegDec(JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE* pBuffHead)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    JPEGDEC_BUFFER_PRIVATE* pBuffPrivate = NULL;

    OMX_CHECK_PARAM(pComponentPrivate);
    JPEGDEC_OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);

    OMX_PRBUFFER2(pComponentPrivate->dbg, "Buffer Came From DSP (output port)\n");

    pBuffPrivate = pBuffHead->pOutputPortPrivate;

    if (pBuffHead->pMarkData && pBuffHead->hMarkTargetComponent == pComponentPrivate->pHandle) {
	OMX_PRBUFFER2(pComponentPrivate->dbg, "send OMX_MarkEvent\n");
        pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                        pComponentPrivate->pHandle->pApplicationPrivate,
                                        OMX_EventMark,
                                        JPEGDEC_OUTPUT_PORT,
                                        0,
                                        pBuffHead->pMarkData);
    }

    /*TUNNEL HERE*/
    if (pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->hTunnelComponent != NULL) {
	OMX_PRBUFFER2(pComponentPrivate->dbg, "Jpeg Sending Output buffer to TUNNEL component\n");

#ifdef __PERF_INSTRUMENTATION__
        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                          pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pBufferPrivate[0]->pBufferHdr->pBuffer,
                          pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pBufferPrivate[0]->pBufferHdr->nFilledLen,
                          PERF_ModuleLLMM);
#endif

        pBuffPrivate->eBufferOwner = JPEGDEC_BUFFER_TUNNEL_COMPONENT;
        eError = OMX_EmptyThisBuffer(pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->hTunnelComponent, pBuffHead);
    }
    else {

#ifdef __PERF_INSTRUMENTATION__
        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                          pBuffHead->pBuffer,
                          pBuffHead->nFilledLen,
                          PERF_ModuleHLMM);
#endif


        if (pBuffHead->nFlags & OMX_BUFFERFLAG_EOS) {
	    OMX_PRBUFFER2(pComponentPrivate->dbg, "%s::%d:Received OMX_BUFFERFLAG_EOS, nFalgs= %lx\n", __FUNCTION__, __LINE__, pBuffHead->nFlags);
            pComponentPrivate->cbInfo.EventHandler (pComponentPrivate->pHandle,
                                                pComponentPrivate->pHandle->pApplicationPrivate,
                                                OMX_EventBufferFlag,
                                                JPEGDEC_OUTPUT_PORT,
                                                pBuffHead->nFlags,
                                                NULL);
        }

        OMX_PRBUFFER1(pComponentPrivate->dbg, "HandleDataBuf_FromDspJpegDec: buf %p pBuffPrivate->eBufferOwner %d\n", pBuffHead, pBuffPrivate->eBufferOwner);
        if (pBuffPrivate->eBufferOwner != JPEGDEC_BUFFER_CLIENT) {
            pBuffPrivate->eBufferOwner = JPEGDEC_BUFFER_CLIENT;

            pComponentPrivate->cbInfo.FillBufferDone(pComponentPrivate->pHandle,
                                                 pComponentPrivate->pHandle->pApplicationPrivate,
                                                 pBuffHead);
        }
    }


    OMX_PRINT1(pComponentPrivate->dbg, "Exit\n");
EXIT:
    return eError;
}   /* End of HandleDataBuf_FromDspJpegDec */



/* ========================================================================== */
/**
 * @fn HandleFreeDataBufJpegDec - Handle emptied input data from DSP and
 *  return to application or another component.
 * @param pComponentPrivate - components private structure
 * @param nParam1 - state to change.
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on failure
 */
/* ========================================================================== */
OMX_ERRORTYPE HandleFreeDataBufJpegDec(JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE* pBuffHead )
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    JPEGDEC_BUFFER_PRIVATE* pBuffPrivate = NULL;

    OMX_CHECK_PARAM(pComponentPrivate);
    JPEGDEC_OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);

    OMX_PRINT1(pComponentPrivate->dbg, "JPEG Entering HandleFreeBuf Function\n");

    pBuffPrivate = pBuffHead->pInputPortPrivate;

    if (pBuffPrivate->eBufferOwner != JPEGDEC_BUFFER_CLIENT) {
        pBuffPrivate->eBufferOwner = JPEGDEC_BUFFER_CLIENT;

#ifdef __PERF_INSTRUMENTATION__
        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                      PREF(pBuffHead,pBuffer),
                      0,
                      PERF_ModuleHLMM);
#endif

        OMX_PRBUFFER2(pComponentPrivate->dbg, "emptydone buf %p\n", pBuffHead);

        pComponentPrivate->cbInfo.EmptyBufferDone(pComponentPrivate->pHandle,
                                              pComponentPrivate->pHandle->pApplicationPrivate,
                                              pBuffHead);
    }

    OMX_PRINT1(pComponentPrivate->dbg, "JPEGexiting\n");
EXIT:
    return eError;
}   /* End of HandleFreeDataBufJpegDec */



/* ========================================================================== */
/**
 *  LCML_CallbackJpegDec() - handle callbacks from LCML
 * @param pComponentPrivate    handle for this instance of the component
 * @param argsCb = argument list
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on failure
  **/
/* ========================================================================== */
OMX_ERRORTYPE LCML_CallbackJpegDec (TUsnCodecEvent event,
                                    void * argsCb [10])
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_BUFFERHEADERTYPE* pBuffHead = NULL;
    JPEGDEC_UAlgInBufParamStruct * ptJPGDecUALGInBufParam = NULL;
    JPEGDEC_PORT_TYPE *pPortType = NULL;
    OMX_U8* pBuffer = NULL;
    JPEGDEC_BUFFER_PRIVATE* pBuffPrivate = NULL;
    int i = 0;

    if ( ((LCML_DSP_INTERFACE*)argsCb[6] ) != NULL ) {
        pComponentPrivate = (JPEGDEC_COMPONENT_PRIVATE*)((LCML_DSP_INTERFACE*)argsCb[6])->pComponentPrivate;
    }
    else {
        OMXDBG_PRINT(stderr, ERROR, 5, 0, "wrong in LCML callback, exit\n");
        goto EXIT;
    }

    JPEGDEC_OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);

    if (event == EMMCodecBufferProcessed) {
        if ((int)argsCb [0] == EMMCodecOuputBuffer) {
            pBuffHead = (OMX_BUFFERHEADERTYPE*)argsCb[7];
            pBuffer = (OMX_U8*)argsCb[1];
            pBuffPrivate = pBuffHead->pOutputPortPrivate;
            pBuffHead->nFilledLen = (int)argsCb[8];
            OMX_PRDSP1(pComponentPrivate->dbg, "nFilled Len from DSP = %d\n",(int)argsCb[8]);

#ifdef __PERF_INSTRUMENTATION__
            PERF_ReceivedFrame(pComponentPrivate->pPERFcomp,
                               pBuffer,
                               (OMX_U32) argsCb[2],
                               PERF_ModuleCommonLayer);
#endif

        if (pBuffPrivate->eBufferOwner == JPEGDEC_BUFFER_DSP) {
            pComponentPrivate->nOutPortOut ++;
        }

        OMX_PRDSP2(pComponentPrivate->dbg, "Filled Data from DSP \n");
        OMX_PRBUFFER1(pComponentPrivate->dbg, "buffer summary (LCML for output buffer %p) %lu %lu\n", pBuffHead, 
                    pComponentPrivate->nInPortIn,
                    pComponentPrivate->nOutPortOut);

        pPortType = pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT];
        if((pBuffHead->nFlags == OMX_FALSE) && (pComponentPrivate->pSectionDecode->nMCURow == OMX_FALSE)){
		for (i = 0; i < (int)(pPortType->pPortDef->nBufferCountActual); i ++) {
                if (pPortType->sBufferFlagTrack[i].buffer_id == pComponentPrivate->nOutPortOut) {
		    OMX_PRBUFFER1(pComponentPrivate->dbg, "JPEGdec:: %d: output buffer %lu has flag %lx\n", __LINE__,
                               pPortType->sBufferFlagTrack[i].buffer_id, 
                               pPortType->sBufferFlagTrack[i].flag);
                    pBuffHead->nFlags = pPortType->sBufferFlagTrack[i].flag;
                    pPortType->sBufferFlagTrack[i].flag = 0;
                    pPortType->sBufferFlagTrack[i].buffer_id = 0xFFFFFFFF;
                    break;
                }
            }
        }else{
		for (i = 0; i < (int)pPortType->pPortDef->nBufferCountActual; i ++) {
                if (pPortType->sBufferFlagTrack[i].buffer_id == pComponentPrivate->nOutPortOut) {
		    OMX_PRBUFFER1(pComponentPrivate->dbg, "JPEGdec:: %d: OUTPUT buffer %lu has flag %lx\n", __LINE__,
                               pPortType->sBufferFlagTrack[i].buffer_id, 
                               pPortType->sBufferFlagTrack[i].flag);
                    if(pPortType->sBufferFlagTrack[i].flag & OMX_BUFFERFLAG_EOS){
                        pPortType->sBufferFlagTrack[i].flag = pPortType->sBufferFlagTrack[i].flag & (!(OMX_BUFFERFLAG_EOS));
                        pBuffHead->nFlags |= pPortType->sBufferFlagTrack[i].flag;
                    }
                    pPortType->sBufferFlagTrack[i].flag = 0;
                    pPortType->sBufferFlagTrack[i].buffer_id = 0xFFFFFFFF;
                    break;
                }
            }
        }
        for (i = 0; i < (int)pPortType->pPortDef->nBufferCountActual; i ++) {
            if (pPortType->sBufferMarkTrack[i].buffer_id == pComponentPrivate->nOutPortOut) {
		OMX_PRBUFFER2(pComponentPrivate->dbg, "buffer ID %lu has mark (output port)\n", pPortType->sBufferMarkTrack[i].buffer_id);
                pBuffHead->pMarkData = pPortType->sBufferMarkTrack[i].pMarkData;
                pBuffHead->hMarkTargetComponent = pPortType->sBufferMarkTrack[i].hMarkTargetComponent;
                pPortType->sBufferMarkTrack[i].buffer_id = 0xFFFFFFFF;
                break;
            }
        }
        if (pBuffPrivate->eBufferOwner == JPEGDEC_BUFFER_DSP) {
            pBuffPrivate->eBufferOwner = JPEGDEC_BUFFER_COMPONENT_OUT;
            eError = HandleDataBuf_FromDspJpegDec(pComponentPrivate, pBuffHead);
            if (eError != OMX_ErrorNone) {
		OMX_PRBUFFER4(pComponentPrivate->dbg, "Error while reading dsp out q\n");
                pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                                       OMX_EventError,
                                                       OMX_ErrorUndefined,
                                                       OMX_TI_ErrorSevere,
                                                       "Error from Component Thread while processing dsp Responses");
            }
        }
        }
        if ((int) argsCb [0] == EMMCodecInputBuffer) {
            pBuffHead = (OMX_BUFFERHEADERTYPE*)argsCb[7];
            ptJPGDecUALGInBufParam = (JPEGDEC_UAlgInBufParamStruct *)argsCb[3];
            pBuffer = (OMX_U8*)argsCb[1];
            pBuffPrivate = pBuffHead->pInputPortPrivate;

#ifdef __PERF_INSTRUMENTATION__
            PERF_ReceivedFrame(pComponentPrivate->pPERFcomp,
                               pBuffer,
                               (OMX_U32) argsCb[8],
                               PERF_ModuleCommonLayer);
#endif
            if (pBuffPrivate->eBufferOwner == JPEGDEC_BUFFER_DSP) {
                pBuffPrivate->eBufferOwner = JPEGDEC_BUFFER_COMPONENT_OUT;
                eError = HandleFreeDataBufJpegDec(pComponentPrivate, pBuffHead);
                if (eError != OMX_ErrorNone) {
		    OMX_PRBUFFER4(pComponentPrivate->dbg, "Error while processing free input Buffers\n");
                    pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                           pComponentPrivate->pHandle->pApplicationPrivate,
                                                           OMX_EventError,
                                                           OMX_ErrorUndefined,
                                                           OMX_TI_ErrorSevere,
                                                           "Error while processing free input buffers");
                }
            }
        }
    }

    if (event == EMMCodecProcessingStoped) {
	OMX_PRDSP2(pComponentPrivate->dbg, "ENTERING TO EMMCodecProcessingStoped \n\n");
        if (pComponentPrivate->nToState == OMX_StateIdle) {
            pComponentPrivate->ExeToIdleFlag |= JPEGD_DSPSTOP;
        }

        pthread_mutex_lock(&pComponentPrivate->mJpegDecMutex);
        pthread_cond_signal(&pComponentPrivate->sStop_cond);
        pthread_mutex_unlock(&pComponentPrivate->mJpegDecMutex);

        goto EXIT;
    }
    if (event == EMMCodecDspError) {
	OMX_PRDSP1(pComponentPrivate->dbg, "LCML_Callback : DSP [0]->%x, [4]->%x, [5]->%x\n", (int)argsCb[0] ,(int)argsCb[4], (int)argsCb[5]);
        OMX_PRDSP1(pComponentPrivate->dbg, "Play compleated if: 0x500 = %x\n", (int)argsCb[5]);
        if(!((int)argsCb[5] == 0x500)){
            eError = OMX_ErrorHardware;
            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                   OMX_EventError,
                                                   OMX_ErrorHardware,
                                                   OMX_TI_ErrorCritical,
                                                   NULL);
            pComponentPrivate->nCurState = OMX_StateInvalid;
            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle, 
                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                   OMX_EventError,
                                                   OMX_ErrorInvalidState, 
                                                   OMX_TI_ErrorCritical,
                                                   "DSP Hardware Error");
        }
        goto EXIT;

#ifdef DSP_MMU_FAULT_HANDLING
        /* Cheking for MMU_fault */
        if((argsCb[4] == (void *)NULL) && (argsCb[5] == (void*)NULL)) {
            pComponentPrivate->nCurState = OMX_StateInvalid;
            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle, 
                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                   OMX_EventError,
                                                   OMX_ErrorInvalidState, 
                                                   OMX_TI_ErrorCritical,
                                                   "DSP MMU FAULT");
        }
#endif
    }

    if (event == EMMCodecInternalError) {
        eError = OMX_ErrorHardware;
        OMX_PRDSP4(pComponentPrivate->dbg, "JPEG-D: EMMCodecInternalError\n");
        pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                               pComponentPrivate->pHandle->pApplicationPrivate,
                                               OMX_EventError,
                                               OMX_ErrorHardware,
                                               OMX_TI_ErrorCritical,
                                               NULL);
        goto EXIT;
    }
    if (event == EMMCodecProcessingPaused) {
        pComponentPrivate->nCurState = OMX_StatePause;
        /* Send StateChangeNotification to application */
        OMX_PRDSP2(pComponentPrivate->dbg, "ENTERING TO EMMCodecProcessingPaused \n");
        pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                               pComponentPrivate->pHandle->pApplicationPrivate,
                                               OMX_EventCmdComplete,
                                               OMX_CommandStateSet,
                                               pComponentPrivate->nCurState,
                                               NULL);
    }
    if (event == EMMCodecStrmCtrlAck) {
	OMX_PRDSP1(pComponentPrivate->dbg, "event = EMMCodecStrmCtrlAck\n");
        if ((int)argsCb [0] == USN_ERR_NONE) {
	    OMX_PRDSP1(pComponentPrivate->dbg, "Callback: no error\n");
            pComponentPrivate->bFlushComplete = OMX_TRUE;
            pthread_mutex_lock(&(pComponentPrivate->mJpegDecFlushMutex));
            pthread_cond_signal(&(pComponentPrivate->sFlush_cond));
            pthread_mutex_unlock(&(pComponentPrivate->mJpegDecFlushMutex));
       }
    }


    OMX_PRDSP1(pComponentPrivate->dbg, "Exiting the LCML_Callback function\n");
EXIT:
    return eError;
}   /* End of LCML_CallbackJpegDec */



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
    JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_ERRORTYPE eError = *(cbData.RM_Error);
    
    pComponentPrivate = (JPEGDEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    JPEGDEC_OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1); 

    OMX_PRINT1(pComponentPrivate->dbg, "RM_Error = %x\n", eError);

    if (eError == OMX_RmProxyCallback_ResourcesPreempted) {

        pComponentPrivate->bPreempted = 1;
        
        if (pComponentPrivate->nCurState == OMX_StateExecuting || 
            pComponentPrivate->nCurState == OMX_StatePause) {

            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                   OMX_EventError,
                                                   OMX_ErrorResourcesPreempted,
                                                   OMX_TI_ErrorSevere,
                                                   NULL);
            
            pComponentPrivate->nToState = OMX_StateIdle;
            OMX_PRMGR2(pComponentPrivate->dbg, "Component Preempted. Going to IDLE State.\n");
        }
        else if (pComponentPrivate->nCurState == OMX_StateIdle){
            pComponentPrivate->nToState = OMX_StateLoaded;
            OMX_PRMGR2(pComponentPrivate->dbg, "Component Preempted. Going to LOADED State.\n");            
        }
        
#ifdef __PERF_INSTRUMENTATION__
        PERF_SendingCommand(pComponentPrivate->pPERF, Cmd, pComponentPrivate->nToState, PERF_ModuleComponent);
#endif
        
        write (pComponentPrivate->nCmdPipe[1], &Cmd, sizeof(Cmd));
        write (pComponentPrivate->nCmdDataPipe[1], &(pComponentPrivate->nToState) ,sizeof(OMX_U32));
        
    }
    else if (eError == OMX_RmProxyCallback_ResourcesAcquired ){

        if (pComponentPrivate->nCurState == OMX_StateWaitForResources) /* Wait for Resource Response */
        {
            pComponentPrivate->cbInfo.EventHandler (
    	                        pHandle, pHandle->pApplicationPrivate,
    	                        OMX_EventResourcesAcquired, 0,0,
    	                        NULL);
            
            pComponentPrivate->nToState = OMX_StateIdle;
            
#ifdef __PERF_INSTRUMENTATION__
            PERF_SendingCommand(pComponentPrivate->pPERF, Cmd, pComponentPrivate->nToState, PERF_ModuleComponent);
#endif
        
            write (pComponentPrivate->nCmdPipe[1], &Cmd, sizeof(Cmd));
            write (pComponentPrivate->nCmdDataPipe[1], &(pComponentPrivate->nToState) ,sizeof(OMX_U32));
            OMX_PRMGR2(pComponentPrivate->dbg, "OMX_RmProxyCallback_ResourcesAcquired.\n");
        }            
        
    }
    EXIT:
        OMX_PRMGR2(pComponentPrivate->dbg, "OMX_RmProxyCallback exiting.\n");
}
#endif




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

void LinkedList_Create(LinkedList *LinkedList) {
    LinkedList->pRoot = NULL;
}

void LinkedList_AddElement(LinkedList *LinkedList, void *pValue) {
    /* create new node and fill the value */
    Node *pNewNode = (Node *)malloc(sizeof(Node));
    if ( pNewNode != NULL )  {
        pNewNode->pValue = (void *)pValue;
        /*printf("LinkedList:::: Pointer=%p has been added.\n", pNewNode->pValue); */
        /* add new node on the root to implement quick FIFO */
        /* modify new node pointers */
        if(LinkedList->pRoot == NULL) {
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
    /*printf("==================No. of deleted nodes: %d=======================================\n\n", nodes); */
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


