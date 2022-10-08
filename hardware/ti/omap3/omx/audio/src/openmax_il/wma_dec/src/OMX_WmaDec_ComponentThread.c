
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
* @file OMX_WmaDec_ComponentThread.c
*
* This file implements OMX Component for WMA decoder that
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
#include <signal.h>
#endif
#include "OMX_WmaDec_Utils.h" 
/* ================================================================================= */
/**
* @fn WMADEC_ComponentThread() Component thread
*
*  @see         OMX_ComponentThread.h
*/
/* ================================================================================ */
void* WMADEC_ComponentThread (void* pThreadData)
{
    int status;
    struct timespec tv;
    int fdmax;
    fd_set rfds;
    OMX_U32 nRet;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pBufHeader = NULL;
    int ret;

    /* Recover the pointer to my component specific data */

    WMADEC_COMPONENT_PRIVATE* pComponentPrivate = (WMADEC_COMPONENT_PRIVATE*)pThreadData;
    OMX_COMPONENTTYPE *pHandle = pComponentPrivate->pHandle;

    OMX_PRINT1(pComponentPrivate->dbg, "OMX_WmaDec_ComponentThread:%d\n",__LINE__);
#ifdef __PERF_INSTRUMENTATION__
OMX_PRDSP2(pComponentPrivate->dbg, "PERF%d :: OMX_WmaDec_ComponentThread.c\n",__LINE__);
    pComponentPrivate->pPERFcomp = PERF_Create(PERF_FOURCC('W', 'M', 'A', 'D'),
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
        tv.tv_nsec = 0;/*WMAD_TIMEOUT * 1000;*/

#ifndef UNDER_CE
		sigset_t set;
		sigemptyset (&set);
		sigaddset (&set, SIGALRM);
		status = pselect (fdmax+1, &rfds, NULL, NULL, &tv, &set);
#else
        status = select (fdmax+1, &rfds, NULL, NULL, &tv);
#endif

        if (pComponentPrivate->bIsStopping == 1) {
            OMX_ERROR4(pComponentPrivate->dbg, ":: Comp Thrd Exiting here...\n");
            goto EXIT;
        }

        if (0 == status) {
            OMX_PRDSP1(pComponentPrivate->dbg, "%d : bIsStopping = %ld\n",__LINE__, pComponentPrivate->bIsStopping);
            if (pComponentPrivate->bIsStopping == 1)  {
                OMX_PRINT2(pComponentPrivate->dbg, "%d:WmaComponentThread \n",__LINE__);
				pComponentPrivate->bIsStopping = 0;
                pComponentPrivate->lcml_nOpBuf = 0;
                pComponentPrivate->lcml_nIpBuf = 0;
                pComponentPrivate->app_nBuf = 0;
                pComponentPrivate->num_Reclaimed_Op_Buff = 0;
                if (pComponentPrivate->curState != OMX_StateIdle) {
                    goto EXIT;
                }
                pComponentPrivate->bIsEOFSent = 0;

                if (pComponentPrivate->curState != OMX_StateIdle) {
                   OMX_ERROR4(pComponentPrivate->dbg, "%d:WmaComponentThread \n",__LINE__);
                    goto EXIT;
                }
            }
            OMX_PRDSP2(pComponentPrivate->dbg, "%d :: Component Time Out !!!!!!!!!!!! \n",__LINE__);
        } 
        else if (-1 == status) {
            OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error in Select\n", __LINE__);
            pComponentPrivate->cbInfo.EventHandler ( pHandle,
                                                     pHandle->pApplicationPrivate,
                                                     OMX_EventError,
                                                     OMX_ErrorInsufficientResources,
                                                     0,
                                                     "Error from Component Thread in select");
            eError = OMX_ErrorInsufficientResources;
        } 
        else if (FD_ISSET (pComponentPrivate->dataPipe[0], &rfds)) {
            OMX_PRCOMM2(pComponentPrivate->dbg, "%d :: DATA pipe is set in Component Thread\n",__LINE__);
            ret = read(pComponentPrivate->dataPipe[0], &pBufHeader, sizeof(pBufHeader));
            if (ret == -1) {
                OMX_ERROR2(pComponentPrivate->dbg, "%d :: Error while reading from the pipe\n",__LINE__);
            }

            eError = WMADECHandleDataBuf_FromApp (pBufHeader,pComponentPrivate);
            if (eError != OMX_ErrorNone) {
		    OMX_ERROR2(pComponentPrivate->dbg, "%d :: Error From WMADECHandleDataBuf_FromApp\n",__LINE__);
                break;
            }

        }
        else if (FD_ISSET (pComponentPrivate->cmdPipe[0], &rfds)) {
            /* Do not accept any command when the component is stopping */
            OMX_PRCOMM2(pComponentPrivate->dbg, "%d :: CMD pipe is set in Component Thread\n",__LINE__);
            nRet = WMADECHandleCommand (pComponentPrivate);
            if (nRet == EXIT_COMPONENT_THRD) {
                OMX_PRDSP2(pComponentPrivate->dbg, "Exiting from Component thread\n");
                WMADEC_CleanupInitParams(pHandle);
                if(eError != OMX_ErrorNone) {
                    OMX_ERROR4(pComponentPrivate->dbg, "%d :: Function Mp3Dec_FreeCompResources returned\
                                                                error\n",__LINE__);
                    goto EXIT;
                }
                OMX_PRDSP2(pComponentPrivate->dbg, "%d :: ARM Side Resources Have Been Freed\n",__LINE__);
                pComponentPrivate->curState = OMX_StateLoaded;

#ifdef __PERF_INSTRUMENTATION__
                OMX_PRINT2(pComponentPrivate->dbg, "PERF%d :: OMX_WmaDec_ComponentThread.c\n",__LINE__);
                PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryComplete | PERF_BoundaryCleanup);
#endif
                pComponentPrivate->cbInfo.EventHandler( pHandle, 
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,
                                                        OMX_ErrorNone,
                                                        pComponentPrivate->curState, 
                                                        NULL);
                if (pComponentPrivate->bPreempted == 0) {
                        pComponentPrivate->cbInfo.EventHandler( pHandle, 
                                                                pHandle->pApplicationPrivate,
                                                                OMX_EventCmdComplete,
                                                                OMX_ErrorNone,
                                                                pComponentPrivate->curState, 
                                                                NULL);
                        OMX_PRDSP2(pComponentPrivate->dbg, "%d %s Component loaded (OMX_StateLoaded)\n",__LINE__,__FUNCTION__);
                }
                else{
                    pComponentPrivate->cbInfo.EventHandler( pHandle, 
                                                            pHandle->pApplicationPrivate,
                                                            OMX_EventError,
                                                            OMX_ErrorResourcesLost,
                                                            pComponentPrivate->curState, 
                                                            NULL);
                    pComponentPrivate->bPreempted = 0;
                }
                /*break;*/
            }

        }
    }
EXIT:
#ifdef __PERF_INSTRUMENTATION__
    OMX_PRINT1(pComponentPrivate->dbg, "PERF%d :: OMX_WmaDec_ComponentThread.c\n",__LINE__);
    PERF_Done(pComponentPrivate->pPERFcomp);
#endif
    OMX_PRINT1(pComponentPrivate->dbg, "%d::Exiting ComponentThread\n",__LINE__);
    return (void*)eError;
}
