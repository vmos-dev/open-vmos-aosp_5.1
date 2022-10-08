
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
* @file OMX_JpegDec_Thread.c
*
* This file implements OMX Component for JPEG decoder.This contains
* functionality for the  pipe - Buffer manager thread.
*
* @patth $(CSLPATH)\jpeg_dec\src\OMX_JpegDec_Thread.c
*
* @rev 0.2
*/

#ifdef UNDER_CE
    #include <windows.h>
    #include <oaf_osal.h>
#else
    #include <wchar.h>
    #include <unistd.h>
    #include <sys/time.h>
    #include <sys/types.h>
    #include <sys/ioctl.h>
    #include <sys/select.h>
    #include <errno.h>
    #include <fcntl.h>
    #include <signal.h>	
#endif

#include <dbapi.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "OMX_JpegDec_Utils.h"

/*------------------------- Function Implementation ------------------*/


/* ========================================================================== */
/**
 * @fn OMX_JpegDec_Thread - Implements the JPEG decoder OMX component thread
 * @param pThreadData - components private structure
 * @return: void*
 *          OMX_ErrorNone on all count
 */
/* ========================================================================== */
void* OMX_JpegDec_Thread (void* pThreadData)
{
    struct timespec tv;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMMANDTYPE eCmd;
    OMX_U32 nParam1;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pThreadData;
    int nStatus;
    int nFdmax;
    fd_set rfds;
    JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate = (JPEGDEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    OMX_U32 error = 0;
    sigset_t set;	

#ifdef __PERF_INSTRUMENTATION__
    pComponentPrivate->pPERFcomp = PERF_Create(PERF_FOURS("JPDT"),
					    PERF_ModuleComponent | PERF_ModuleImageDecode);
#endif

    /**Looking for highest number of file descriptor for pipes
       inorder to put in select loop */

    nFdmax = pComponentPrivate->nCmdPipe[0];

    if (pComponentPrivate->nFree_outBuf_Q[0] > nFdmax) {
        nFdmax = pComponentPrivate->nFree_outBuf_Q[0];
    }

    if (pComponentPrivate->nFilled_inpBuf_Q[0] > nFdmax) {
        nFdmax = pComponentPrivate->nFilled_inpBuf_Q[0];
    }

    OMX_PRINT1(pComponentPrivate->dbg, "fd max is %d\n", nFdmax);

    while (1)
    {
        FD_ZERO (&rfds);
        FD_SET (pComponentPrivate->nCmdPipe[0], &rfds);
        if (pComponentPrivate->nCurState != OMX_StatePause) {
          FD_SET (pComponentPrivate->nFree_outBuf_Q[0], &rfds);
          FD_SET (pComponentPrivate->nFilled_inpBuf_Q[0], &rfds);
        }

        tv.tv_sec = 1;

        tv.tv_nsec = 0;	

        sigemptyset(&set);
        sigaddset(&set, SIGALRM);
        nStatus = pselect (nFdmax+1, &rfds, NULL, NULL, NULL,&set);

        if (-1 == nStatus) {
	    OMX_TRACE5(pComponentPrivate->dbg, "Error in Select\n");
            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                   OMX_EventError,
                                                   OMX_ErrorInsufficientResources,
                                                   OMX_TI_ErrorSevere,
                                                   "Error from COmponent Thread in select");
	     eError = OMX_ErrorInsufficientResources;
        }
        else {
            if ((FD_ISSET(pComponentPrivate->nCmdPipe[0], &rfds)) ||
                (FD_ISSET(pComponentPrivate->nCmdDataPipe[0], &rfds))) {
                /* Do not accept any command when the component is stopping */
		OMX_PRCOMM2(pComponentPrivate->dbg, "CMD pipe is set in Component Thread\n");

                read (pComponentPrivate->nCmdPipe[0], &eCmd, sizeof (eCmd));   /*Manage error from any read and write*/
                OMX_PRCOMM1(pComponentPrivate->dbg, "read ecmd %d\n", eCmd); 
                read (pComponentPrivate->nCmdDataPipe[0], &nParam1, sizeof (nParam1));
                OMX_PRCOMM1(pComponentPrivate->dbg, "read nParam1 %lu\n", nParam1); 

#ifdef __PERF_INSTRUMENTATION__
                PERF_ReceivedCommand(pComponentPrivate->pPERFcomp,
                                     eCmd, nParam1, PERF_ModuleLLMM);

#endif

                if (eCmd == OMX_CommandStateSet) {
		    if ((int)nParam1 != -1) {
			    OMX_PRINT2(pComponentPrivate->dbg, "calling handlecommand  from JPEGDEC (%lu)\n", nParam1);
                        if(nParam1 == OMX_StateInvalid){
                            pComponentPrivate->nToState = OMX_StateInvalid;
                        }
                        error =  HandleCommandJpegDec (pComponentPrivate, nParam1);
                        OMX_PRINT2(pComponentPrivate->dbg, "after called handlecommand  from JPEGDEC (%lu)\n", error);
                        if (error != OMX_ErrorNone) {
                            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                                   OMX_EventError,
                                                                   error,
                                                                   OMX_TI_ErrorSevere,
                                                                   NULL);
                        }
                    }
                    else
                        break;
                }
                else if (eCmd == OMX_CommandPortDisable) {
		    OMX_PRINT2(pComponentPrivate->dbg, "PORT DISABLE\n");
                    error = DisablePortJpegDec(pComponentPrivate, nParam1);
                    if(error != OMX_ErrorNone){
                        pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                       OMX_EventError,
                                       error,
                                       OMX_TI_ErrorSevere,
                                       NULL);
                    }
                }
                else if (eCmd == OMX_CommandPortEnable) {
		    OMX_PRINT2(pComponentPrivate->dbg, "PORT Enable\n");
                    error = EnablePortJpegDec(pComponentPrivate, nParam1);
                    if(error != OMX_ErrorNone){
                        pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                       OMX_EventError,
                                       error,
                                       OMX_TI_ErrorSevere,
                                       NULL);
                    }
                }
                else if (eCmd == OMX_CustomCommandStopThread) {
		    OMX_PRINT2(pComponentPrivate->dbg, "cmd nStop\n");
                    goto EXIT;
                }
                else if (eCmd == OMX_CommandMarkBuffer) {
		    OMX_PRBUFFER2(pComponentPrivate->dbg, "Command OMX_CommandMarkBuffer received \n");
                    if (!pComponentPrivate->pMarkBuf) {
			OMX_PRBUFFER2(pComponentPrivate->dbg, "Command OMX_CommandMarkBuffer received \n");
                        /* TODO Need to handle multiple marks */
                        pComponentPrivate->pMarkBuf = (OMX_MARKTYPE *)(nParam1);
                    }
                }
                else if (eCmd == OMX_CommandFlush) {
		    OMX_PRBUFFER2(pComponentPrivate->dbg, "eCmd =  OMX_CommandFlush\n");
                    error = HandleCommandFlush (pComponentPrivate, nParam1);
                    if(error != OMX_ErrorNone){
                        pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                       OMX_EventError,
                                       error,
                                       OMX_TI_ErrorSevere,
                                       NULL);
                    }
                }
                continue;
            }

            if ((FD_ISSET(pComponentPrivate->nFilled_inpBuf_Q[0], &rfds)) &&
                (pComponentPrivate->nCurState != OMX_StatePause)) {

                eError = HandleDataBuf_FromAppJpegDec (pComponentPrivate);
                if (eError != OMX_ErrorNone) {
		    OMX_PRBUFFER4(pComponentPrivate->dbg, "Error while processing free Q Buffers\n");
                    pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                           pComponentPrivate->pHandle->pApplicationPrivate,
                                                           OMX_EventError,
                                                           eError,
                                                           OMX_TI_ErrorSevere,
                                                           "Error from COmponent Thread while processing filled inp Q");
                }
            }

            if (FD_ISSET (pComponentPrivate->nFree_outBuf_Q[0], &rfds)) {

	        OMX_PRBUFFER2(pComponentPrivate->dbg, "nFree_outBuf_Q has some buffers in Component Thread\n");
                eError = HandleFreeOutputBufferFromAppJpegDec(pComponentPrivate);
                if (eError != OMX_ErrorNone) {
		    OMX_PRBUFFER4(pComponentPrivate->dbg, "Error while processing free Q Buffers\n");
                    pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                           pComponentPrivate->pHandle->pApplicationPrivate,
                                                           OMX_EventError,
                                                           eError,
                                                           OMX_TI_ErrorSevere,
                                                           "Error from COmponent Thread while processing free out Q");
                }
            }
        }
    }
EXIT:
#ifdef __PERF_INSTRUMENTATION__
    PERF_Done(pComponentPrivate->pPERFcomp);
#endif
    return (void *)eError;

}


