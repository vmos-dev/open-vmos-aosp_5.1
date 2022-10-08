
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
/* ==============================================================================
*             Texas Instruments OMAP (TM) Platform Software
*  (c) Copyright Texas Instruments, Incorporated.  All Rights Reserved.
*
*  Use of this software is controlled by the terms and conditions found 
*  in the license agreement under which this software has been supplied.
* ============================================================================ */
/**
* @file OMX_VPP_ComponentThread.c
*
* This file implements OMX Component for video post processing that 
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
* ============================================================================= */
 

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

#define _XOPEN_SOURCE 600

#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/select.h>

#include <fcntl.h>
#include <errno.h>
#endif
#include <dbapi.h>
#include <string.h>

#include <stdio.h>

#include "OMX_VPP.h"
#include "OMX_VPP_Utils.h"
#include "OMX_VPP_CompThread.h"
#include <OMX_Component.h>
#include <signal.h>

/** Default timeout used to come out of blocking calls*/
#define VPP_THREAD_TIMEOUT (100)


/* -------------------------------------------------------------------*/
/**
  *  ComponentThread() thread polling for messages and data in pipe 
  *
  * @param pThreadData                 
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/
void* VPP_ComponentThreadFunc (void* pThreadData)
{
    int status;
    struct timeval tv;
    int fdmax;
    fd_set rfds;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMMANDTYPE eCmd = -1;
    OMX_U32 nParam1;
    int nRet = -1;
    OMX_PTR pCmdData = NULL;
    sigset_t set;
    

    
    VPP_COMPONENT_PRIVATE* pComponentPrivate = (VPP_COMPONENT_PRIVATE*)pThreadData;
    OMX_COMPONENTTYPE *pHandle = pComponentPrivate->pHandle;

#ifdef __PERF_INSTRUMENTATION__
    pComponentPrivate->pPERFcomp = PERF_Create(PERF_FOURCC('V','P','P','T'),
                                            PERF_ModuleComponent |
                                            PERF_ModuleVideoEncode | PERF_ModuleImageEncode |
                                            PERF_ModuleVideoDecode | PERF_ModuleImageDecode);
#endif

    fdmax = pComponentPrivate->cmdPipe[0];

    /** Looking for highest number of file descriptor 
        for pipes inorder to put in select loop */
    if (pComponentPrivate->nFree_oPipe[0] > fdmax) {
        fdmax = pComponentPrivate->nFree_oPipe[0];
    }

    if (pComponentPrivate->nFilled_iPipe[0] > fdmax) {
        fdmax = pComponentPrivate->nFilled_iPipe[0];
    }

    while (1) {
        FD_ZERO (&rfds);
        FD_SET (pComponentPrivate->cmdPipe[0], &rfds);

        if (pComponentPrivate->curState != OMX_StatePause) {
        FD_SET (pComponentPrivate->nFree_oPipe[0], &rfds);
        FD_SET (pComponentPrivate->nFilled_iPipe[0], &rfds);
        }
        
        tv.tv_sec  = 0;
        tv.tv_usec = VPP_THREAD_TIMEOUT * 1000;

	sigemptyset(&set);
	sigaddset(&set,SIGALRM);
	
        status = pselect (fdmax+1, &rfds, NULL, NULL, NULL, &set);
        
        if (0 == status) { 
            /*VPP_DPRINT("\n\n\n%d ::!!!!!  Component Time Out !!!!!!!!!!!! \n",__LINE__);*/
            if (pComponentPrivate->bIsStopping == 1) {
                pComponentPrivate->bIsStopping = 0;
                pComponentPrivate->bIsEOFSent  = 0;
            }
            continue;
        } 
        
        if (-1 == status) { 
            VPP_DPRINT ("%d :: Error in Select\n", __LINE__);
            pComponentPrivate->cbInfo.EventHandler (
                                    pHandle,pHandle->pApplicationPrivate,
                                    OMX_EventError,OMX_ErrorInsufficientResources,OMX_TI_ErrorSevere,
                                    "Error from Component Thread in select");
            goto EXIT;
        } 
        
        if (FD_ISSET (pComponentPrivate->cmdPipe[0], &rfds)) {
            nRet = read(pComponentPrivate->cmdPipe[0], &eCmd, sizeof(eCmd));
            if (nRet == -1) {
                VPP_DPRINT ("Error while writing to the free_oPipe\n");
                eError = OMX_ErrorInsufficientResources;
                goto EXIT;
            }

            if (eCmd == EXIT_COMPONENT_THRD)
            {

#ifdef __PERF_INSTRUMENTATION__
                PERF_ReceivedCommand(pComponentPrivate->pPERFcomp,
                                    eCmd, 0, PERF_ModuleLLMM);
#endif

                VPP_DPRINT ("VPP::%d: Exiting thread Cmd : \n",__LINE__);
                break;
            }


            if (eCmd == OMX_CommandMarkBuffer) {
                nRet = read(pComponentPrivate->nCmdDataPipe[0], &pCmdData, sizeof(pCmdData));
                if (nRet == -1) {
                    VPP_DPRINT ("Error while writing to the free_oPipe\n");
                    eError = OMX_ErrorInsufficientResources;
                    goto EXIT;
                }
            }
            else {
                nRet = read(pComponentPrivate->nCmdDataPipe[0], &nParam1, sizeof(nParam1));
                if (nRet == -1) {
                    VPP_DPRINT ("Error while writing to the free_oPipe\n");
                    eError = OMX_ErrorInsufficientResources;
                    goto EXIT;
                }
            }

#ifdef __PERF_INSTRUMENTATION__
            PERF_ReceivedCommand(pComponentPrivate->pPERFcomp,
                                eCmd,
                                (eCmd == OMX_CommandMarkBuffer) ? ((OMX_U32) pCmdData) : nParam1,
                                PERF_ModuleLLMM);
#endif

            switch (eCmd)
            {
            case OMX_CommandPortDisable:
                    VPP_DisablePort(pComponentPrivate, nParam1);
                    break;
                
            case OMX_CommandStateSet:
                eError = VPP_HandleCommand(pComponentPrivate, nParam1);
                if(eError != OMX_ErrorNone) {
#ifdef RESOURCE_MANAGER_ENABLED
                    pComponentPrivate->curState = OMX_StateInvalid;
#endif
                    pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle, 
                                        pComponentPrivate->pHandle->pApplicationPrivate,
                                        OMX_EventError, 
                                        OMX_ErrorInsufficientResources, 
                                        OMX_TI_ErrorMajor, 
                                        "Error from Component Thread while processing Command Pipe.\n");
                    goto EXIT;
                }
                VPP_DPRINT("return from StateSet %d\n", nParam1);
                break;
                
            case OMX_CommandPortEnable:
                VPP_EnablePort(pComponentPrivate, nParam1);
                break;
                
            case OMX_CommandMarkBuffer:
                /* OMX_CommandMarkBuffer is handled directly on VPP_SendCommand() function*/
                break;

            case OMX_CommandFlush:
                VPP_HandleCommandFlush(pComponentPrivate, nParam1, OMX_TRUE); 
                break;
            case OMX_CommandMax:
                break;
            } 
            continue;
        }  

        /*Filled Input Buffer from Application to component*/
        if ((FD_ISSET(pComponentPrivate->nFilled_iPipe[0], &rfds))) { 
            eError = VPP_Process_FilledInBuf(pComponentPrivate);      
            if (eError != OMX_ErrorNone) {
                pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                    pComponentPrivate->pHandle->pApplicationPrivate,
                                    OMX_EventError,
                                    OMX_ErrorUndefined,
                                    OMX_TI_ErrorSevere,
                                    NULL);
            }
        }
        /*Free output buffers from Application to component*/
        if (FD_ISSET(pComponentPrivate->nFree_oPipe[0], &rfds)) { 
            eError = VPP_Process_FreeOutBuf(pComponentPrivate);
            if (eError != OMX_ErrorNone) {
                pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                    pComponentPrivate->pHandle->pApplicationPrivate,
                                    OMX_EventError,
                                    OMX_ErrorUndefined,
                                    OMX_TI_ErrorSevere,
                                    NULL);
            }
        }
    }

EXIT:

#ifdef __PERF_INSTRUMENTATION__
    PERF_Done(pComponentPrivate->pPERFcomp);
#endif

    return (void*)OMX_ErrorNone;
}
