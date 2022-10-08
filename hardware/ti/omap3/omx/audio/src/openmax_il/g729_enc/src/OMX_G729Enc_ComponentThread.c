
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
 * @file OMX_G729Enc_ComponentThread.c
 *
 * This file implements G729 Encoder Component Thread and its functionality
 * that is fully compliant with the Khronos OpenMAX (TM) 1.0 Specification
 *
 * @path  $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\g729_enc\src
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
 *! 21-Jun-2006 bk: Khronos OpenMAX (TM) 1.0 migration done
 *! 22-May-2006 bk: DASF recording quality improved
 *! 19-Apr-2006 bk: DASF recording speed issue resloved
 *! 23-Feb-2006 bk: DASF functionality added
 *! 18-Jan-2006 bk: Repated recording issue fixed and LCML changes taken care
 *! 14-Dec-2005 bk: Initial Version
 *! 16-Nov-2005 bk: Initial Version
 *! 23-Sept-2005 bk: Initial Version
 *! 10-Sept-2005 bk: Initial Version
 *! 10-Sept-2005 bk:
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
#include <dbapi.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <sys/select.h>
#endif
#ifdef RESOURCE_MANAGER_ENABLED
#include <ResourceManagerProxyAPI.h>
#endif
#ifdef __PERF_INSTRUMENTATION__
#include "perf.h"
#endif

/*-------program files ----------------------------------------*/
#include "OMX_G729Enc_Utils.h"
#include "OMX_G729Enc_ComponentThread.h"

/* ================================================================================= */
/**
 * @fn G729ENC_CompThread() Component thread
 *
 *  @see         OMX_G729Enc_ComponentThread.h
 */
/* ================================================================================ */

void* G729ENC_CompThread(void* pThreadData)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    int status = 0;
    struct timespec tv;
    int fdmax = 0;
    int ret = 0;
    fd_set rfds;
    OMX_U32 nRet = 0;
    OMX_BUFFERHEADERTYPE *pBufHeader = NULL;
    G729ENC_COMPONENT_PRIVATE* pComponentPrivate = (G729ENC_COMPONENT_PRIVATE*)pThreadData;
    OMX_COMPONENTTYPE *pHandle = pComponentPrivate->pHandle;
    
    G729ENC_DPRINT("Entering\n");
#ifdef __PERF_INSTRUMENTATION__
    pComponentPrivate->pPERFcomp = PERF_Create(PERF_FOURCC('7', '2', '9', 'E'),
                                               PERF_ModuleComponent |
                                               PERF_ModuleAudioDecode);
#endif
    fdmax = pComponentPrivate->cmdPipe[0];

    if (pComponentPrivate->dataPipe[0] > fdmax)
    {
        fdmax = pComponentPrivate->dataPipe[0];
    }

    while (1)
    {
        FD_ZERO (&rfds);
        FD_SET (pComponentPrivate->cmdPipe[0], &rfds);
        FD_SET (pComponentPrivate->dataPipe[0], &rfds);
        tv.tv_sec = 1;
        tv.tv_nsec = 0;

#ifndef UNDER_CE
        sigset_t set;
        sigemptyset (&set);
        sigaddset (&set, SIGALRM);
        status = pselect (fdmax+1, &rfds, NULL, NULL, &tv, &set);
#else
        status = select (fdmax+1, &rfds, NULL, NULL, &tv);
#endif        
        if (0 == status)
        {
            G729ENC_DPRINT("bIsThreadstop=%ld\n", pComponentPrivate->bIsThreadstop);
            G729ENC_DPRINT("lcml_nOpBuf=%ld\n", pComponentPrivate->lcml_nOpBuf);
            G729ENC_DPRINT("lcml_nIpBuf=%ld\n", pComponentPrivate->lcml_nIpBuf);
            G729ENC_DPRINT("app_nBuf=%ld\n", pComponentPrivate->app_nBuf);
            if (pComponentPrivate->bIsThreadstop == 1)
            {
                pComponentPrivate->bIsStopping = 0;
                pComponentPrivate->bIsThreadstop = 0;
                pComponentPrivate->lcml_nOpBuf = 0;
                pComponentPrivate->lcml_nIpBuf = 0;
                pComponentPrivate->app_nBuf = 0;
                pComponentPrivate->num_Op_Issued = 0;
                pComponentPrivate->num_Sent_Ip_Buff = 0;
                pComponentPrivate->num_Reclaimed_Op_Buff = 0;
                pComponentPrivate->bIsEOFSent = 0;
                if (pComponentPrivate->curState != OMX_StateIdle)
                {
                    G729ENC_DPRINT("pComponentPrivate->curState not OMX_StateIdle\n");
                    goto EXIT;
                }
            }
            G729ENC_DPRINT("Component Time Out !!!!! \n");
        }
        else if(-1 == status)
        {
            OMX_EPRINT("from CompThread in select\n");
            pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                   pHandle->pApplicationPrivate,
                                                   OMX_EventError,
                                                   OMX_ErrorInsufficientResources,
                                                   0,
                                                   "");
            eError = OMX_ErrorInsufficientResources;
        }
        else if ((FD_ISSET (pComponentPrivate->dataPipe[0], &rfds))) 
        {
            G729ENC_DPRINT("DATA pipe is set in Component Thread\n");
            ret = read(pComponentPrivate->dataPipe[0], &pBufHeader,
                       sizeof(pBufHeader));
            if (ret == -1)
            {
                OMX_EPRINT("while reading from the pipe\n");
                goto EXIT;
            }
            eError = G729ENC_HandleDataBufFromApp(pBufHeader,pComponentPrivate);
            if (eError != OMX_ErrorNone)
            {
                OMX_EPRINT("from G729ENC_HandleDataBufFromApp\n");
                break;
            }
        }
         
        else if(FD_ISSET (pComponentPrivate->cmdPipe[0], &rfds))
        {
            /* Do not accept any command when the component is stopping */
            G729ENC_DPRINT("CMD pipe is set in Component Thread\n");
            nRet = G729ENC_HandleCommand(pComponentPrivate);
            if (nRet == G729ENC_EXIT_COMPONENT_THRD)
            {
                if(eError != OMX_ErrorNone)
                {
                    OMX_EPRINT("from G729ENC_CleanupInitParams\n");
                    goto EXIT;
                }
                pComponentPrivate->curState = OMX_StateLoaded;
#ifdef __PERF_INSTRUMENTATION__
                PERF_Boundary(pComponentPrivate->pPERFcomp,
                              PERF_BoundaryComplete | PERF_BoundaryCleanup);
#endif
                if (pComponentPrivate->bPreempted == 0) { 
                    pComponentPrivate->cbInfo.EventHandler(
                                                           pHandle, pHandle->pApplicationPrivate,
                                                           OMX_EventCmdComplete,
                                                           OMX_ErrorNone,pComponentPrivate->curState, NULL);
                }
                else {
                    pComponentPrivate->cbInfo.EventHandler(
                                                           pHandle, pHandle->pApplicationPrivate,
                                                           OMX_EventError,
                                                           OMX_ErrorResourcesLost,pComponentPrivate->curState, NULL);
                    pComponentPrivate->bPreempted = 0;
                }
            }

        }   
    }
 EXIT:
#ifdef __PERF_INSTRUMENTATION__
    PERF_Done(pComponentPrivate->pPERFcomp);
#endif
    G729ENC_DPRINT("Exiting. Returning = 0x%x\n", eError);
    return (void*)eError;
}
