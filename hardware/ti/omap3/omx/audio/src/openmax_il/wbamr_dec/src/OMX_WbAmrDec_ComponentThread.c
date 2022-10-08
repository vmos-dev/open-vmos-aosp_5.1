
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
 * @file OMX_WbAmrDec_ComponentThread.c
 *
 * This file implements OMX Component for WBMAR decoder that
 * is fully compliant with the OMX Audio specification 1.0.
 *
 * @path  $(CSLPATH)\
 *
 * @rev  0.1
 */
/* ----------------------------------------------------------------------------
 *!
 *! Revision History
 *! ===================================
 *! 10-Sept-2005 mf:  Initial Version. Change required per OMAPSWxxxxxxxxx
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
#else
#include <wchar.h>
#include <unistd.h>
#include <dbapi.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <signal.h>
#endif
#ifdef ANDROID
#include <sys/resource.h>
#include <utils/threads.h>
#include <sys/prctl.h>
#endif
#include "OMX_WbAmrDec_Utils.h"
#include "OMX_WbAmrDecoder.h"
#include "OMX_WbAmrDec_ComponentThread.h"

/* ================================================================================= */
/**
 * @fn WBAMR_DEC_ComponentThread() Component thread
 *
 *  @see         OMX_ComponentThread.h
 */
/* ================================================================================ */

void* WBAMR_DEC_ComponentThread (void* pThreadData)
{
    OMX_S16 status;
    struct timespec tv;
    OMX_S16 fdmax;
    fd_set rfds;
    OMX_U32 nRet;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    WBAMR_DEC_COMPONENT_PRIVATE* pComponentPrivate = (WBAMR_DEC_COMPONENT_PRIVATE*)pThreadData;
    OMX_COMPONENTTYPE *pHandle = pComponentPrivate->pHandle;
    OMX_BUFFERHEADERTYPE *pBufHeader = NULL;
    ssize_t ret;

#ifdef ANDROID
    setpriority(PRIO_PROCESS, 0, ANDROID_PRIORITY_AUDIO);
    prctl(PR_SET_NAME, (unsigned long)"WBAMRComponent", 0, 0, 0);
#endif

    OMX_PRINT1(pComponentPrivate->dbg, "Entering\n");

#ifdef __PERF_INSTRUMENTATION__
    pComponentPrivate->pPERFcomp = PERF_Create(PERF_FOURCC('W', 'B', '_', 'D'),
                                               PERF_ModuleComponent |
                                               PERF_ModuleAudioDecode);
#endif

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

        OMX_PRINT1(pComponentPrivate->dbg, "AmrComponentThread \n");
#ifndef UNDER_CE
        sigset_t set;
        sigemptyset (&set);
        sigaddset (&set, SIGALRM);
        status = pselect (fdmax+1, &rfds, NULL, NULL, &tv, &set);
#else
        status = select (fdmax+1, &rfds, NULL, NULL, &tv);
#endif

        if (pComponentPrivate->bIsStopping == 1) {
            OMX_ERROR4(pComponentPrivate->dbg, "Comp Thrd Exiting here...\n");
            goto EXIT;
        }
        if (0 == status) {

            OMX_PRSTATE2(pComponentPrivate->dbg, "bIsStopping = %ld\n",
                         pComponentPrivate->bIsStopping);

            OMX_PRBUFFER2(pComponentPrivate->dbg, "lcml_nOpBuf = %ld\n",
                          pComponentPrivate->lcml_nOpBuf);

            OMX_PRBUFFER2(pComponentPrivate->dbg, "lcml_nIpBuf = %ld\n",
                          pComponentPrivate->lcml_nIpBuf);
            OMX_PRBUFFER2(pComponentPrivate->dbg, "app_nBuf = %ld\n",
                          pComponentPrivate->app_nBuf);

            if (pComponentPrivate->bIsStopping == 1)  {

                pComponentPrivate->bIsStopping = 0;
                pComponentPrivate->lcml_nOpBuf = 0;
                pComponentPrivate->lcml_nIpBuf = 0;
                pComponentPrivate->app_nBuf = 0;
                pComponentPrivate->num_Reclaimed_Op_Buff = 0;

                OMX_PRINT1(pComponentPrivate->dbg, "AmrComponentThread \n");
                if (pComponentPrivate->curState != OMX_StateIdle) {
                    OMX_PRINT1(pComponentPrivate->dbg, "AmrComponentThread \n");
                    goto EXIT;
                }
            }
            OMX_PRINT2(pComponentPrivate->dbg,"Component Time Out !!!!!!!!!!!! \n");
        } else if (-1 == status) {
            OMX_ERROR4(pComponentPrivate->dbg, "Error in Select\n");
            pComponentPrivate->cbInfo.EventHandler (pHandle,
                                                    pHandle->pApplicationPrivate,
                                                    OMX_EventError,
                                                    OMX_ErrorInsufficientResources,
                                                    OMX_TI_ErrorSevere,
                                                    "Error from Component Thread in select");
            eError = OMX_ErrorInsufficientResources;

        } else if (FD_ISSET (pComponentPrivate->dataPipe[0], &rfds)) {
            OMX_PRCOMM2(pComponentPrivate->dbg, "DATA pipe is set in Component Thread\n");
            ret = read(pComponentPrivate->dataPipe[0], &pBufHeader, sizeof(pBufHeader));
            if (ret == -1) {
                OMX_ERROR4(pComponentPrivate->dbg, "Error while reading from the pipe\n");
            }
            eError = WBAMR_DEC_HandleDataBuf_FromApp (pBufHeader,pComponentPrivate);
            if (eError != OMX_ErrorNone) {
                OMX_ERROR2(pComponentPrivate->dbg, "Error From WBAMR_DEC_HandleDataBuf_FromApp\n");
                break;
            }
        }
        else if (FD_ISSET (pComponentPrivate->cmdPipe[0], &rfds)) {
            /* Do not accept any command when the component is stopping */
            OMX_PRCOMM2(pComponentPrivate->dbg, "CMD pipe is set in Component Thread\n");
            nRet = WBAMR_DEC_HandleCommand (pComponentPrivate);
            if (nRet == WBAMR_DEC_EXIT_COMPONENT_THRD) {
                OMX_PRINT1(pComponentPrivate->dbg, "Exiting from Component thread\n");

                if(eError != OMX_ErrorNone) {
                    OMX_ERROR4(pComponentPrivate->dbg, "Function Mp3Dec_FreeCompResources returned\
                                                                error\n");
                    goto EXIT;
                }
                OMX_PRBUFFER2(pComponentPrivate->dbg, "ARM Side Resources Have Been Freed\n");

                pComponentPrivate->curState = OMX_StateLoaded;
#ifdef __PERF_INSTRUMENTATION__
                PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryComplete | PERF_BoundaryCleanup);
#endif
                if (pComponentPrivate->bPreempted == 0) {
                    pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventCmdComplete,
                                                           OMX_ErrorNone,
                                                           pComponentPrivate->curState,
                                                           NULL);
                }
                else {
                    pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventError,
                                                           OMX_ErrorResourcesLost,
                                                           OMX_TI_ErrorMajor,
                                                           NULL);
                    pComponentPrivate->bPreempted = 0;
                }

            }
        }
    }
 EXIT:
#ifdef __PERF_INSTRUMENTATION__
    PERF_Done(pComponentPrivate->pPERFcomp);
#endif
    OMX_PRINT1(pComponentPrivate->dbg, "Exiting\n");
    return (void*)eError;
}
