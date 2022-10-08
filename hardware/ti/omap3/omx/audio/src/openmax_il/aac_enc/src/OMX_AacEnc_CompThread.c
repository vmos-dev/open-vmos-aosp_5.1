
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
* @file OMX_AacEnc_CompThread.c
*
* This file implements OMX Component for AAC encoder that
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
#include "OMX_AacEncoder.h"
#include "OMX_AacEnc_Utils.h"
#include "OMX_AacEnc_CompThread.h"


void* AACENC_ComponentThread (void* pThreadData)
{
    int status;
    struct timespec tv;
    int fdmax;
    int ret = 0;
    fd_set rfds;
    OMX_U32 nRet;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pBufHeader;

    /* Recover the pointer to my component specific data */
    AACENC_COMPONENT_PRIVATE* pComponentPrivate = (AACENC_COMPONENT_PRIVATE*)pThreadData;
    OMX_COMPONENTTYPE *pHandle = pComponentPrivate->pHandle;


#ifdef __PERF_INSTRUMENTATION__
    pComponentPrivate->pPERFcomp = PERF_Create(PERF_FOURCC('A', 'A', 'C', 'E'),
                                                   PERF_ModuleComponent |
                                                   PERF_ModuleAudioDecode);
#endif

    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Entering ComponentThread\n", __LINE__);
    fdmax = pComponentPrivate->cmdPipe[0];

    if (pComponentPrivate->dataPipe[0] > fdmax) 
        fdmax = pComponentPrivate->dataPipe[0];

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

        if (pComponentPrivate->bIsThreadstop == 1) {
            OMX_ERROR4(pComponentPrivate->dbg, ":: Comp Thrd Exiting here...\n");
            goto EXIT;
        }

        if (status == 0) 
        {

            OMX_PRINT1(pComponentPrivate->dbg, "%d : bIsStopping = %ld\n",__LINE__, pComponentPrivate->bIsStopping);
            OMX_PRINT1(pComponentPrivate->dbg, "%d : lcml_nOpBuf = %ld\n",__LINE__, pComponentPrivate->lcml_nOpBuf);
            OMX_PRINT1(pComponentPrivate->dbg, "%d : lcml_nIpBuf = %ld\n",__LINE__, pComponentPrivate->lcml_nIpBuf);

            if (pComponentPrivate->bIsThreadstop == 1)  
            {
                OMX_PRINT1(pComponentPrivate->dbg, "%d  :: OMX_AACENC_ComponentThread \n",__LINE__);
                pComponentPrivate->bIsStopping = 0;
                pComponentPrivate->bIsThreadstop = 0;
                pComponentPrivate->lcml_nOpBuf = 0;
                pComponentPrivate->lcml_nIpBuf = 0;
                pComponentPrivate->app_nBuf = 0;            /* NOT USED */
                pComponentPrivate->num_Op_Issued = 0;
                pComponentPrivate->num_Sent_Ip_Buff = 0;
                pComponentPrivate->num_Reclaimed_Op_Buff = 0;
                pComponentPrivate->bIsEOFSent = 0;
                OMX_PRINT1(pComponentPrivate->dbg, "%d :: OMX_AACENC_ComponentThread \n",__LINE__);
                if (pComponentPrivate->curState != OMX_StateIdle) 
                {
                    OMX_PRINT1(pComponentPrivate->dbg, "%d ::OMX_AACENC_ComponentThread \n",__LINE__);
                    goto EXIT;
                }
             }
             OMX_PRINT2(pComponentPrivate->dbg, "%d :: Component Time Out !!!!! \n",__LINE__);
        } 
        else if(status == -1) 
        {
            OMX_ERROR2(pComponentPrivate->dbg, "%d :: Error in Select\n", __LINE__);
            pComponentPrivate->cbInfo.EventHandler (pHandle, pHandle->pApplicationPrivate, 
                                                    OMX_EventError,
                                                    OMX_ErrorInsufficientResources,
                                                    OMX_TI_ErrorSevere,
                                                    "Error from Component Thread in select");
            eError = OMX_ErrorInsufficientResources;
        }

        else if ((FD_ISSET (pComponentPrivate->dataPipe[0], &rfds)) && (pComponentPrivate->curState != OMX_StatePause)) 
        {
            OMX_PRCOMM2(pComponentPrivate->dbg, "%d :: DATA pipe is set in Component Thread\n",__LINE__);
            OMX_PRDSP2(pComponentPrivate->dbg, "%d :: pHandle: %p \n",__LINE__, pHandle);
            OMX_PRDSP1(pComponentPrivate->dbg, "%d :: pHandle->pComponentPrivate:%p \n",__LINE__, pHandle->pComponentPrivate);

            OMX_PRDSP1(pComponentPrivate->dbg, "%d :: pComponentPrivate:%p \n",__LINE__, pComponentPrivate);
            pBufHeader = NULL;
            ret = read(pComponentPrivate->dataPipe[0], &pBufHeader, sizeof(pBufHeader));
            if (ret == -1) 
            {
                OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error while reading from the pipe\n",__LINE__);
                eError = OMX_ErrorHardware;
                goto EXIT;
            }
            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: pBufHeader:%p \n",__LINE__, pBufHeader);
            eError = AACENCHandleDataBuf_FromApp(pBufHeader,pComponentPrivate);
            if (eError != OMX_ErrorNone) 
            {
                OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error From AACENCHandleDataBuf_FromApp\n",__LINE__);
                break;
            }

        }



        else if(FD_ISSET (pComponentPrivate->cmdPipe[0], &rfds)) 
        {
            OMX_PRDSP2(pComponentPrivate->dbg, "%d :: pHandle: %p \n",__LINE__,pHandle);
            /* Do not accept any command when the component is stopping */
            OMX_PRCOMM2(pComponentPrivate->dbg, "%d :: CMD pipe is set in Component Thread\n",__LINE__);
            nRet = AACENCHandleCommand (pComponentPrivate);
            if (nRet == EXIT_COMPONENT_THRD) 
            {
                OMX_ERROR2(pComponentPrivate->dbg, " %d :: Exiting from Component thread\n",__LINE__);

                AACENC_CleanupInitParams(pHandle);
                if(eError != OMX_ErrorNone) 
                {
                    OMX_ERROR4(pComponentPrivate->dbg, "%d :: AACENC_CleanupInitParams returned error\n",__LINE__);
                    goto EXIT;
                }
                OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: ARM Side Resources Have Been Freed\n",__LINE__);

                pComponentPrivate->curState = OMX_StateLoaded;

#ifdef __PERF_INSTRUMENTATION__
                PERF_Boundary(pComponentPrivate->pPERFcomp,PERF_BoundaryComplete | PERF_BoundaryCleanup);
#endif  

                if(pComponentPrivate->bPreempted==0){
                    if (RemoveStateTransition(pComponentPrivate, OMX_TRUE) != OMX_ErrorNone) {
                        return OMX_ErrorUndefined;
                    }
                    pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventCmdComplete,
                                                           OMX_CommandStateSet,
                                                           pComponentPrivate->curState,
                                                           NULL);

                }
                else{
                    pComponentPrivate->cbInfo.EventHandler(pHandle, 
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventError,
                                                           OMX_ErrorResourcesLost,
                                                           OMX_TI_ErrorMajor, 
                                                           NULL);
                    pComponentPrivate->bPreempted = 0;
                }

                pComponentPrivate->bLoadedCommandPending = OMX_FALSE;
                goto EXIT;
            }

        }
    }

EXIT:

#ifdef __PERF_INSTRUMENTATION__
    PERF_Done(pComponentPrivate->pPERFcomp);
#endif
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Exiting ComponentThread\n", __LINE__);
    return (void*)eError;
}

