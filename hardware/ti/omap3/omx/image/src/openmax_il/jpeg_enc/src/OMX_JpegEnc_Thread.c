
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
* @file OMX_JpegEnc_Thread.c
*
* This file implements OMX Component for JPEG encoder that
* is fully compliant with the OMX specification 1.5.
*
* @path  $(CSLPATH)\src
*
* @rev  0.1
*/
/* -------------------------------------------------------------------------------- */
/* ================================================================================
*!
*! Revision History
*! ===================================
*!
*! 22-May-2006 mf: Revisions appear in reverse chronological order;
*! that is, newest first.  The date format is dd-Mon-yyyy.
* ================================================================================= */

/****************************************************************
*  INCLUDE FILES
****************************************************************/

/* ----- System and Platform Files ----------------------------*/
#ifdef UNDER_CE
#include <windows.h>
#include <oaf_osal.h>
#include <omx_core.h>
#else
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
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

/*------- Program Header Files ----------------------------------------*/

#include "OMX_JpegEnc_Utils.h"


#define OMX_MAX_TIMEOUTS 200

/*-------- Function Implementations ---------------------------------*/
/*-------------------------------------------------------------------*/
/**
  * OMX_JpegEnc_Thread()
  *
  * Called by Start_ComponentThread function.
  *
  * @param pThreadData
  *
  * @retval OMX_ErrorNone                  success, ready to roll
  *         OMX_ErrorInsufficientResources if the malloc fails
  **/
/*-------------------------------------------------------------------*/
void* OMX_JpegEnc_Thread (void* pThreadData)
{
    int status;
    struct timeval tv;
    int fdmax;
    fd_set rfds;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMMANDTYPE eCmd;
    OMX_U32 nParam1;
    sigset_t set; 	


    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pThreadData;
    JPEGENC_COMPONENT_PRIVATE *pComponentPrivate = (JPEGENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

#ifdef __PERF_INSTRUMENTATION__
    pComponentPrivate->pPERFcomp = PERF_Create(PERF_FOURS("JPET"),
                                               PERF_ModuleComponent |PERF_ModuleImageEncode);
#endif

    /**Looking for highest number of file descriptor for pipes
       inorder to put in select loop */
    fdmax = pComponentPrivate->nCmdPipe[0];
    
    if ( pComponentPrivate->free_outBuf_Q[0] > fdmax ) {
        fdmax = pComponentPrivate->free_outBuf_Q[0];
    }
    

    if ( pComponentPrivate->filled_inpBuf_Q[0] > fdmax ) {
        fdmax = pComponentPrivate->filled_inpBuf_Q[0];
    }

    OMX_TRACE2(pComponentPrivate->dbg, "fd max is %d\n",fdmax);

    while ( 1 ) {
        FD_ZERO (&rfds);
        FD_SET (pComponentPrivate->nCmdPipe[0], &rfds);
        if (pComponentPrivate->nCurState != OMX_StatePause) {
            FD_SET (pComponentPrivate->free_outBuf_Q[0], &rfds); 
            FD_SET (pComponentPrivate->filled_inpBuf_Q[0], &rfds);
        }


        tv.tv_sec = 1;
        tv.tv_usec = 0;
		
	 sigemptyset(&set)	;
	 sigaddset(&set,SIGALRM);
        status = pselect (fdmax+1, &rfds, NULL, NULL, NULL,&set);

        if ( 0 == status ) {
            OMX_TRACE2(pComponentPrivate->dbg, "Component Thread Time Out!!!\n");
        } else if ( -1 == status ) {
            OMX_TRACE4(pComponentPrivate->dbg, "Error in Select\n");

            pComponentPrivate->cbInfo.EventHandler (pComponentPrivate->pHandle, pComponentPrivate->pHandle->pApplicationPrivate,
                                                    OMX_EventError, OMX_ErrorInsufficientResources, OMX_TI_ErrorSevere,
                                                    "Error from COmponent Thread in select");
	     eError = OMX_ErrorInsufficientResources;
            break;
        } else {
            if ( (FD_ISSET (pComponentPrivate->filled_inpBuf_Q[0], &rfds))
                 && (pComponentPrivate->nCurState != OMX_StatePause) ) {
                OMX_PRBUFFER2(pComponentPrivate->dbg, "filled_inpBuf_Q pipe is set\n");

                eError = HandleJpegEncDataBuf_FromApp (pComponentPrivate);

                if ( eError != OMX_ErrorNone ) {
                    OMX_PRBUFFER4(pComponentPrivate->dbg, "Error while processing free queue buffers\n");
                    pComponentPrivate->cbInfo.EventHandler (pComponentPrivate->pHandle, pComponentPrivate->pHandle->pApplicationPrivate,
                                                            OMX_EventError, OMX_ErrorUndefined, OMX_TI_ErrorSevere,
                                                            "1-Error from Component Thread while processing free Q\n");
                }
            }

            if ( FD_ISSET (pComponentPrivate->free_outBuf_Q[0], &rfds) ) {
                OMX_PRBUFFER2(pComponentPrivate->dbg, "free_outBuf_Q has some buffers in Component Thread\n");
                eError = HandleJpegEncFreeOutputBufferFromApp(pComponentPrivate);
                if ( eError != OMX_ErrorNone ) {
                    OMX_PRBUFFER4(pComponentPrivate->dbg, "Error while processing free Q Buffers\n");
                    pComponentPrivate->cbInfo.EventHandler (pComponentPrivate->pHandle, pComponentPrivate->pHandle->pApplicationPrivate,
                                                            OMX_EventError, OMX_ErrorUndefined, OMX_TI_ErrorSevere,
                                                            "3-Error from Component Thread while processing free Q\n");
                }
            }
            if ( FD_ISSET (pComponentPrivate->nCmdPipe[0], &rfds) ) {
                /* Do not accept any command when the component is stopping */
		OMX_PRCOMM2(pComponentPrivate->dbg, "CMD pipe is set in Component Thread\n");
                
                read (pComponentPrivate->nCmdPipe[0], &eCmd, sizeof (eCmd));
                read (pComponentPrivate->nCmdDataPipe[0], &nParam1, sizeof (nParam1));

#ifdef __PERF_INSTRUMENTATION__
                                PERF_ReceivedCommand(pComponentPrivate->pPERFcomp,
                                                     eCmd, nParam1,
                                                     PERF_ModuleLLMM);
#endif

                OMX_PRINT2(pComponentPrivate->dbg, "eCmd %d, nParam1 %d\n", (int)eCmd, (int)nParam1);
                if ( eCmd == OMX_CommandStateSet ) {
                    OMX_PRINT2(pComponentPrivate->dbg, "processing OMX_CommandStateSet\n");
                    if ( (int)nParam1 != -1 ){
                        if(nParam1 == OMX_StateInvalid){
                            pComponentPrivate->nToState = OMX_StateInvalid;
                        }
                        eError = HandleJpegEncCommand (pComponentPrivate, nParam1);
                        if ( eError != OMX_ErrorNone ) {
                            OMX_PRINT4(pComponentPrivate->dbg, "Error returned by HandleJpegEncCommand\n");
                            pComponentPrivate->cbInfo.EventHandler (pComponentPrivate->pHandle, pComponentPrivate->pHandle->pApplicationPrivate,
                                                                    OMX_EventError, OMX_ErrorHardware, OMX_TI_ErrorSevere,
                                                                    "Error returned by HandleJpegEncCommand\n");
                        }
                        
                    }
                    else{
                        break;
                    }
                } 
                else if ( eCmd == OMX_CommandPortDisable ) {
                    OMX_PRBUFFER2(pComponentPrivate->dbg, "Before Disable Port function Port %d\n",(int)nParam1);
                    eError = JpegEncDisablePort(pComponentPrivate, nParam1);
                    OMX_PRBUFFER2(pComponentPrivate->dbg, "After JPEG Encoder Sisable Port error = %d\n", eError);
                    if (eError != OMX_ErrorNone ) {
                        break;
                        }
                    } 
                else if ( eCmd == OMX_CommandPortEnable ) {   /*TODO: Check errors*/
                    eError = JpegEncEnablePort(pComponentPrivate, nParam1);
                    if (eError != OMX_ErrorNone ) {
                        break;
                        }
                    } 
                    
                else if ( eCmd == OMX_CustomCommandStopThread ) {
                    /*eError = 10;*/
                    goto EXIT;
                    }
                else if ( eCmd == OMX_CommandFlush ) {
                      OMX_PRBUFFER2(pComponentPrivate->dbg, "eCmd =  OMX_CommandFlush\n");
                      eError = HandleJpegEncCommandFlush (pComponentPrivate, nParam1);
                      if (eError != OMX_ErrorNone) {
                          break;
                          }
                    }
                    if (pComponentPrivate->nCurState == OMX_StatePause)
                        continue;
            }


        }
    }
    OMX_PRINT1(pComponentPrivate->dbg, "Component Thread Exit while loop\n");

EXIT:

#ifdef __PERF_INSTRUMENTATION__
    PERF_Done(pComponentPrivate->pPERFcomp);
#endif

    OMX_PRINT1(pComponentPrivate->dbg, "Component Thread Exit while loop from EXIT label\n");
    return(void*)eError; /*OMX_ErrorNone;*/

}
