
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
* @file OMX_AmrEnc_ComponentThread.c
*
* This file implements NBAMR Encoder Component Thread and its functionality
* that is fully compliant with the Khronos OpenMAX (TM) 1.0 Specification
*
* @path  $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\nbamr_enc\src
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
#include <wchar.h>
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
/*-------program files ----------------------------------------*/
#include "OMX_AmrEnc_Utils.h"
#include "OMX_AmrEnc_ComponentThread.h"

/* ================================================================================= */
/**
* @fn NBAMRENC_CompThread() Component thread
*
*  @see         OMX_AmrEnc_ComponentThread.h
*/
/* ================================================================================ */

void* NBAMRENC_CompThread(void* pThreadData)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    int status;
    struct timespec tv;
    int fdmax;
    int ret = 0;
    fd_set rfds;
    OMX_U32 nRet;
    OMX_BUFFERHEADERTYPE *pBufHeader = NULL;
    AMRENC_COMPONENT_PRIVATE* pComponentPrivate = (AMRENC_COMPONENT_PRIVATE*)pThreadData;
    OMX_COMPONENTTYPE *pHandle = pComponentPrivate->pHandle;
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Entering NBAMRENC_CompThread\n", __LINE__);

#ifdef __PERF_INSTRUMENTATION__
    pComponentPrivate->pPERFcomp = PERF_Create(PERF_FOURCC('N', 'B', '_', 'E'),
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

#ifndef UNDER_CE
        sigset_t set;
        sigemptyset (&set);
        sigaddset (&set, SIGALRM);
        status = pselect (fdmax+1, &rfds, NULL, NULL, &tv, &set);
#else
        status = select (fdmax+1, &rfds, NULL, NULL, &tv);
#endif

        if (pComponentPrivate->bIsThreadstop == 1) {
            OMX_ERROR2(pComponentPrivate->dbg, ":: Comp Thrd Exiting here...\n");
            goto EXIT;
        }
        
        if (0 == status) {
            OMX_PRBUFFER1(pComponentPrivate->dbg, "%d :: bIsThreadstop = %ld\n",__LINE__,pComponentPrivate->bIsThreadstop);
            OMX_PRBUFFER1(pComponentPrivate->dbg, "%d :: lcml_nOpBuf = %ld\n",__LINE__,pComponentPrivate->lcml_nOpBuf);
            OMX_PRBUFFER1(pComponentPrivate->dbg, "%d :: lcml_nIpBuf = %ld\n",__LINE__,pComponentPrivate->lcml_nIpBuf);
            OMX_PRBUFFER1(pComponentPrivate->dbg, "%d :: app_nBuf = %ld\n",__LINE__,pComponentPrivate->app_nBuf);
            if (pComponentPrivate->bIsThreadstop == 1)  {
                pComponentPrivate->bIsStopping = 0;
                pComponentPrivate->bIsThreadstop = 0;
                pComponentPrivate->lcml_nOpBuf = 0;
                pComponentPrivate->lcml_nIpBuf = 0;
                pComponentPrivate->app_nBuf = 0;
                pComponentPrivate->num_Op_Issued = 0;
                if (pComponentPrivate->curState != OMX_StateIdle) {
                    OMX_ERROR4(pComponentPrivate->dbg, "%d :: pComponentPrivate->curState is not OMX_StateIdle\n",__LINE__);
                    goto EXIT;
                }
             }
             OMX_PRINT1(pComponentPrivate->dbg, "%d :: Component Time Out !!!!! \n",__LINE__);
        } else if(-1 == status) {
            OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error in Select\n", __LINE__);
            pComponentPrivate->cbInfo.EventHandler ( pHandle,
                                                     pHandle->pApplicationPrivate,
                                                     OMX_EventError,
                                                     OMX_ErrorInsufficientResources,
                                                     OMX_TI_ErrorSevere,
                                                     "Error from Component Thread in select");
            eError = OMX_ErrorInsufficientResources;

        } else if ((FD_ISSET (pComponentPrivate->dataPipe[0], &rfds))
                   && (pComponentPrivate->curState != OMX_StatePause)) {
            OMX_PRCOMM2(pComponentPrivate->dbg, "%d :: DATA pipe is set in Component Thread\n",__LINE__);
            ret = read(pComponentPrivate->dataPipe[0], &pBufHeader, sizeof(pBufHeader));
            if (ret == -1) {
                OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error while reading from the pipe\n",__LINE__);
                goto EXIT;
            }
            eError = NBAMRENC_HandleDataBufFromApp(pBufHeader,pComponentPrivate);
            if (eError != OMX_ErrorNone) {
                OMX_ERROR4(pComponentPrivate->dbg, "%d :: NBAMRENC_HandleDataBufFromApp returned error\n",__LINE__);
                break;
            }
        }

        else if(FD_ISSET (pComponentPrivate->cmdPipe[0], &rfds)) {
            /* Do not accept any command when the component is stopping */
            OMX_PRCOMM2(pComponentPrivate->dbg, "%d :: CMD pipe is set in Component Thread\n",__LINE__);
            nRet = NBAMRENC_HandleCommand(pComponentPrivate);
            if (nRet == NBAMRENC_EXIT_COMPONENT_THRD) {

                if(eError != OMX_ErrorNone) {
                    OMX_ERROR4(pComponentPrivate->dbg, "%d :: NBAMRENC_CleanupInitParams returned error\n",__LINE__);
                    goto EXIT;
                }
                pComponentPrivate->curState = OMX_StateLoaded;
#ifdef __PERF_INSTRUMENTATION__
                PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryComplete | PERF_BoundaryCleanup);
#endif

                if(pComponentPrivate->bPreempted==0){

                    if(RemoveStateTransition(pComponentPrivate, OMX_TRUE) != OMX_ErrorNone) {
                        return OMX_ErrorUndefined;
                    }

                    pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                           pComponentPrivate->pHandle->pApplicationPrivate,
                                                           OMX_EventCmdComplete,
                                                           OMX_CommandStateSet,
                                                           pComponentPrivate->curState,
                                                           NULL);

                }
                else{
                    pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                            pHandle->pApplicationPrivate,
                                                            OMX_EventError,
                                                            OMX_ErrorResourcesLost,
                                                            OMX_TI_ErrorMajor,
                                                            NULL);
                    pComponentPrivate->bPreempted = 0;
                }
                goto EXIT;
            }
        }
    
    }
EXIT:
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Exiting NBAMRENC_CompThread\n", __LINE__);
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Returning = 0x%x\n",__LINE__,eError);
#ifdef __PERF_INSTRUMENTATION__
    PERF_Done(pComponentPrivate->pPERFcomp);
#endif
    return (void*)eError;
}
