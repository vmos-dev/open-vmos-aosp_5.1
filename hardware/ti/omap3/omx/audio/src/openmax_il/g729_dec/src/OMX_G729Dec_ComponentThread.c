
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
 * @file OMX_G729Dec_ComponentThread.c
 *
 * This file implements OMX Component for G729 decoder that
 * is fully compliant with the OMX Audio specification .
 *
 * @path  $(OMAPSW_MPU)\linux\audio\src\openmax_il\g729_dec\src
 *
 * @rev  0.1
 */
/* ----------------------------------------------------------------------------- 
 *! 
 *! Revision History 
 *! ===================================
 *! Date         Author(s)            Version  Description
 *! ---------    -------------------  -------  ---------------------------------
 *! 03-Jan-2007  A.Donjon             0.1      Code update for G729 DECODER
 *! 
 *!
 * ================================================================================= */

/* ------compilation control switches -------------------------*/
/****************************************************************
 *  INCLUDE FILES
 ****************************************************************/
/* ----- system and platform files ----------------------------*/
#ifdef UNDER_CE
#include <windows.h>
#else
#include <unistd.h>
#include <dbapi.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/time.h> 
#include <signal.h> 
#endif

/*-------program files ----------------------------------------*/
#include "OMX_G729Dec_Utils.h"
#include "OMX_G729Decoder.h"
#include "OMX_G729Dec_ComponentThread.h"

#ifdef RESOURCE_MANAGER_ENABLED
#include <ResourceManagerProxyAPI.h>
#endif
#ifdef __PERF_INSTRUMENTATION__
#include "perf.h"
#endif

/****************************************************************
 * EXTERNAL REFERENCES NOTE : only use if not found in header file
 ****************************************************************/
/*--------data declarations -----------------------------------*/
/*--------function prototypes ---------------------------------*/

/****************************************************************
 * PUBLIC DECLARATIONS Defined here, used elsewhere
 ****************************************************************/
/*--------data declarations -----------------------------------*/

/*--------function prototypes ---------------------------------*/

/****************************************************************
 * PRIVATE DECLARATIONS Defined here, used only here
 ****************************************************************/
/*--------data declarations -----------------------------------*/
/*--------function prototypes ---------------------------------*/
/*--------macros ----------------------------------------------*/


void* G729DEC_ComponentThread (void* pThreadData)
{
    OMX_S16 status = 0;
    struct timespec tv;
    OMX_S16 fdmax = 0;
    fd_set rfds;
    OMX_U32 nRet = 0;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    G729DEC_COMPONENT_PRIVATE* pComponentPrivate = (G729DEC_COMPONENT_PRIVATE*)pThreadData;
    OMX_COMPONENTTYPE *pHandle = pComponentPrivate->pHandle;
    OMX_BUFFERHEADERTYPE *pBufHeader = NULL;
    ssize_t ret = 0;
    
    G729DEC_DPRINT("OMX_G729Dec_ComponentThread:%d\n",__LINE__);
#ifdef __PERF_INSTRUMENTATION__
    pComponentPrivate->pPERFcomp = PERF_Create(PERF_FOURCC('7', '2', '9', 'E'),
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

        sigset_t set;
        sigemptyset (&set);
        sigaddset (&set, SIGALRM);
        status = pselect (fdmax+1, &rfds, NULL, NULL, &tv, &set);
        
        if (0 == status) {

            G729DEC_DPRINT("%d : bIsStopping = %d\n",__LINE__,
                           (int)pComponentPrivate->bIsStopping);

            G729DEC_DPRINT("%d : lcml_nOpBuf = %d\n",__LINE__,
                           (int)pComponentPrivate->lcml_nOpBuf);

            G729DEC_DPRINT("%d : lcml_nIpBuf = %d\n",__LINE__,
                           (int)pComponentPrivate->lcml_nIpBuf);
            G729DEC_DPRINT("%d : app_nBuf = %d\n",__LINE__,
                           (int)pComponentPrivate->app_nBuf);

            if (pComponentPrivate->bIsStopping == 1)  {
                G729DEC_DPRINT("%d:G729ComponentThread \n",__LINE__);

                G729DEC_DPRINT("%d:G729ComponentThread \n",__LINE__);
                if(eError != OMX_ErrorNone) {
                    G729DEC_DPRINT("%d: Error Occurred in Codec Stop..\n",__LINE__);
                    break;
                }
                pComponentPrivate->bIsStopping = 0;
                pComponentPrivate->lcml_nOpBuf = 0;
                pComponentPrivate->lcml_nIpBuf = 0;
                pComponentPrivate->app_nBuf = 0;
                pComponentPrivate->num_Op_Issued = 0;
                pComponentPrivate->num_Sent_Ip_Buff = 0;
                pComponentPrivate->num_Reclaimed_Op_Buff = 0;
                pComponentPrivate->bIsEOFSent = 0;

                G729DEC_DPRINT("%d:G729ComponentThread \n",__LINE__);
                if (pComponentPrivate->curState != OMX_StateIdle) {
                    G729DEC_DPRINT("%d:G729AComponentThread \n",__LINE__);
                    goto EXIT;
                }
            }
            G729DEC_DPRINT ("%d :: Component Time Out !!!!!!!!!!!! \n",__LINE__);

        } else if (-1 == status) {
            
            G729DEC_EPRINT ("%d :: Error in Select\n", __LINE__);
            pComponentPrivate->cbInfo.EventHandler (
                                                    pHandle,
                                                    pHandle->pApplicationPrivate,
                                                    OMX_EventError,OMX_ErrorInsufficientResources,0,
                                                    "Error from COmponent Thread in select");
            eError = OMX_ErrorInsufficientResources;

        } 
        else if ((FD_ISSET (pComponentPrivate->dataPipe[0], &rfds))){
            G729DEC_DPRINT ("%d :: DATA pipe is set in Component Thread\n",__LINE__);
            ret = read(pComponentPrivate->dataPipe[0], &pBufHeader, sizeof(pBufHeader));
            if (ret == -1) {
                G729DEC_DPRINT ("%d :: Error while reading from the pipe\n",__LINE__);
            }
            eError = G729DECHandleDataBuf_FromApp (pBufHeader,pComponentPrivate);
            if (eError != OMX_ErrorNone) {
                G729DEC_DPRINT ("%d :: Error From G729DECHandleDataBuf_FromApp\n", __LINE__);
                break;
            }

        }
        else if (FD_ISSET (pComponentPrivate->cmdPipe[0], &rfds)) {
            /* Do not accept any command when the component is stopping */
            G729DEC_DPRINT ("%d :: CMD pipe is set in Component Thread\n",__LINE__);
            nRet = G729DECHandleCommand (pComponentPrivate);
            if (nRet == EXIT_COMPONENT_THRD) {
                G729DEC_DPRINT ("Exiting from Component thread\n");
                G729DEC_CleanupInitParams(pHandle);
                if(eError != OMX_ErrorNone) {
                    G729DEC_DPRINT("%d :: Function G729Dec_FreeCompResources returned\
                                                                error\n",__LINE__);
                    goto EXIT;
                }
                G729DEC_DPRINT("%d :: ARM Side Resources Have Been Freed\n",__LINE__);
                pComponentPrivate->curState = OMX_StateLoaded;
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
    G729DEC_DPRINT("%d::Exiting ComponentThread\n",__LINE__);
    return (void*)eError;
}
