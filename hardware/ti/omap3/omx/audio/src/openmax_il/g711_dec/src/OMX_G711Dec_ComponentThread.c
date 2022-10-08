
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
 * @file OMX_G711Dec_ComponentThread.c
 *
 * This file implements OMX Component for PCM decoder that
 * is fully compliant with the OMX Audio specification .
 *
 * @path  $(CSLPATH)\
 *
 * @rev  0.1
 */
/* ----------------------------------------------------------------------------*/

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
#include <sys/time.h>
#include <signal.h>
#endif
#include "OMX_G711Dec_Utils.h"
#ifdef RESOURCE_MANAGER_ENABLED
#include <ResourceManagerProxyAPI.h>
#endif

void* ComponentThread (void* pThreadData)
{
    OMX_S16 status = 0;
    struct timespec tv;
    OMX_S16 fdmax = 0;
    fd_set rfds;
    OMX_U32 nRet = 0;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    G711DEC_COMPONENT_PRIVATE* pComponentPrivate = (G711DEC_COMPONENT_PRIVATE*)pThreadData;
    OMX_COMPONENTTYPE *pHandle = pComponentPrivate->pHandle;
    OMX_BUFFERHEADERTYPE *pBufHeader = NULL;
    ssize_t ret = 0;

    G711DEC_DPRINT("OMX_G711Dec_ComponentThread:%d\n",__LINE__);

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

        if (0 == status) {
            if (pComponentPrivate->bIsStopping == 1)  {
                pComponentPrivate->bIsStopping = 0;
                pComponentPrivate->lcml_nOpBuf = 0;
                pComponentPrivate->lcml_nIpBuf = 0;
                pComponentPrivate->app_nBuf = 0;
                pComponentPrivate->num_Op_Issued = 0;
                pComponentPrivate->num_Sent_Ip_Buff = 0;
                pComponentPrivate->num_Reclaimed_Op_Buff = 0;
                pComponentPrivate->bIsEOFSent = 0;

                if (pComponentPrivate->curState != OMX_StateIdle) {
                    goto EXIT;
                }
            }
            G711DEC_DPRINT ("%d :: Component Time Out !!!!!!!!!!!! \n",__LINE__);
        } else if (-1 == status) {
            G711DEC_DPRINT ("%d :: Error in Select\n", __LINE__);
            pComponentPrivate->cbInfo.EventHandler (pHandle, pHandle->pApplicationPrivate,
                                                    OMX_EventError,
                                                    OMX_ErrorInsufficientResources,0,
                                                    "Error from COmponent Thread in select");
            eError = OMX_ErrorInsufficientResources;

        } else if (FD_ISSET(pComponentPrivate->dataPipe[0], &rfds)){
            G711DEC_DPRINT ("%d :: DATA pipe is set in Component Thread\n",__LINE__);
            ret = read(pComponentPrivate->dataPipe[0], &pBufHeader, sizeof(pBufHeader));
            
            if (ret == -1) {
                G711DEC_DPRINT ("%d :: Error while reading from the pipe\n",__LINE__);
            }
            
            eError = G711DECHandleDataBuf_FromApp (pBufHeader,pComponentPrivate);
            
            if (eError != OMX_ErrorNone) {
                G711DEC_DPRINT ("%d :: Error From G711DECHandleDataBuf_FromApp\n");
                break;
            }
        }
        else if (FD_ISSET (pComponentPrivate->cmdPipe[0], &rfds)) {
            /* Do not accept any command when the component is stopping */
            G711DEC_DPRINT ("%d :: CMD pipe is set in Component Thread\n",__LINE__);
            nRet = G711DECHandleCommand (pComponentPrivate);
            if (nRet == EXIT_COMPONENT_THRD) {
                G711DEC_DPRINT ("Exiting from Component thread\n");
                G711DEC_CleanupInitParams(pHandle);
                if(eError != OMX_ErrorNone) {
                    G711DEC_DPRINT("%d :: Function G711Dec_FreeCompResources returned\
                                                                error\n",__LINE__);
                    goto EXIT;
                }
                G711DEC_DPRINT("%d :: ARM Side Resources Have Been Freed\n",__LINE__);

                pComponentPrivate->curState = OMX_StateLoaded;
                pComponentPrivate->cbInfo.EventHandler( pHandle, 
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,
                                                        OMX_ErrorNone,
                                                        pComponentPrivate->curState, 
                                                        NULL);
            }
        }
    }
 EXIT:
    G711DEC_DPRINT("%d::Exiting ComponentThread\n",__LINE__);
    return (void*)eError;
}
