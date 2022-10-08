/*
 * dspbridge/mpu_api/inc/DSPProcessor_OEM.h
 *
 * DSP-BIOS Bridge driver support functions for TI OMAP processors.
 *
 * Copyright (C) 2007 Texas Instruments, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published 
 * by the Free Software Foundation version 2.1 of the License.
 *
 * This program is distributed .as is. WITHOUT ANY WARRANTY of any kind,
 * whether express or implied; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

/*
 *  ======== DSPProcessor_OEM.h ========
 *  Description:
 *      This is the header for processor OEM fxns.
 *
 *  Public Functions:
 *      DSPProcessor_Ctrl           (OEM-function)
 *      DSPProcessor_GetTrace       (OEM-function)
 *      DSPProcessor_Load           (OEM-function)
 *      DSPProcessor_Start          (OEM-function)
 *
 *  Notes:
 *
 *! Revision History:
 *! ================
 *! 23-Nov-2002 gp: Minor comment spelling correction.
 *! 13-Feb-2001 kc: DSP/BIOS Bridge name updates.
 *! 29-Nov-2000 rr: OEM Fxn's are seperated from DSPProcessor.h
 */

#ifndef DSPPROCESSOR_OEM_
#define DSPPROCESSOR_OEM_

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== DSPProcessor_Ctrl ========
 *  Purpose:
 *      Pass control information to the GPP device driver managing the DSP
 *      processor. This will be an OEM-only function, and not part of the
 *      DSP/BIOS Bridge application developer's API.
 *  Parameters:
 *      hProcessor:     The processor handle.
 *      dwCmd:          Private driver IOCTL cmd ID.
 *      pArgs:          Ptr to a driver defined argument structure.
 *  Returns:
 *      DSP_SOK:        Success.
 *      DSP_EHANDLE:    Invalid processor handle.
 *      DSP_ETIMEOUT:   A timeout occured before the control information
 *                      could be sent.
 *      DSP_ERESTART:   A critical error has occured and the DSP is being
 *                      restarted.
 *      DSP_EFAIL:      Unable to Send the control information.
 *  Details:
 *      This function Calls the WMD_BRD_IOCTL.
 */
	extern DBAPI DSPProcessor_Ctrl(DSP_HPROCESSOR hProcessor,
				       ULONG dwCmd,
				       IN OPTIONAL struct DSP_CBDATA * pArgs);

/*
 *  ======== DSPProcessor_Load ========
 *  Purpose:
 *      Reset a processor and load a new base program image.
 *      This will be an OEM-only function, and not part of the DSP/BIOS Bridge
 *      application developer's API.
 *  Parameters:
 *      hProcessor:         The processor handle.
 *      iArgc:              The number of arguments (strings) in aArgv[]
 *      aArgv:              An array of arguments (ANSI Strings)
 *      aEnvp:              An array of environment settings (ANSI Strings)
 *  Returns:
 *      DSP_SOK:            Success.
 *      DSP_EHANDLE:        Invalid processor handle.
 *      DSP_EFILE:          The DSP executable was not found
 *      DSP_ECORRUTFILE:    Unable to Parse the DSP Executable
 *      DSP_EINVALIDARG:    iArgc should be > 0.
 *      DSP_EPOINTER:       aArgv is invalid
 *      DSP_EATTACHED:      Abort because a GPP Client is attached to the
 *                          specified processor
 *      DSP_EFAIL:          Unable to load the processor
 *  Details:
 *      Does not implement access rights to control which GPP application
 *      can load the processor.  
 */
	extern DBAPI DSPProcessor_Load(DSP_HPROCESSOR hProcessor,
				       IN CONST INT iArgc,
				       IN CONST CHAR ** aArgv,
				       IN CONST CHAR ** aEnvp);

/*
 *  ======== DSPProcessor_Start ========
 *  Purpose:
 *      Start a processor running.
 *      Processor must be in PROC_LOADED state.
 *      This will be an OEM-only function, and not part of the DSP/BIOS Bridge
 *      application developer's API.
 *  Parameters:
 *      hProcessor:         The processor handle.
 *  Returns:
 *      DSP_SOK:            Success.
 *      DSP_EHANDLE:        Invalid processor handle.
 *      DSP_EWRONGSTATE:    Processor is not in PROC_LOADED state.
 *      DSP_EACCESSDENIED:  Client does not have the required access rights
 *                          to start the Processor
 *      DSP_EFAIL:          Unable to start the processor.
 *  Details:
 */
	extern DBAPI DSPProcessor_Start(DSP_HPROCESSOR hProcessor);

/*
 *  ======== DSPProcessor_Stop ========
 *  Purpose:
 *      Stop a running processor.
 *      Processor must be in PROC_LOADED or PROC_RUNNIG state.
 *      This will be an OEM-only function, and not part of the DSP/BIOS Bridge
 *      application developer's API.
 *  Parameters:
 *      hProcessor:         The processor handle.
 *  Returns:
 *      DSP_SOK:            Success.
 *      DSP_EHANDLE:        Invalid processor handle.
 *      DSP_EWRONGSTATE:    Processor is not in PROC_LOADED state.
 *      DSP_EACCESSDENIED:  Client does not have the required access rights
 *                          to start the Processor
 *      DSP_EFAIL:          Unable to start the processor.
 *  Details:
 */
	extern DBAPI DSPProcessor_Stop(DSP_HPROCESSOR hProcessor);

#ifdef __cplusplus
}
#endif
#endif				/* DSPPROCESSOR_OEM_ */
