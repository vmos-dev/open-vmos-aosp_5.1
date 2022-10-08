/*
 * dspbridge/mpu_api/inc/dbg.h
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
 *  ======== dbg.h ========
 *  Purpose:
 *      Provide debugging services for 'Bridge Mini Drivers.
 *
 *  Public Functions:
 *      DBG_Exit
 *      DBG_Init
 *      DBG_Printf
 *      DBG_Trace
 *
 *  Notes:
 *      WMD's must not call DBG_Init or DBG_Exit.
 *
 *! Revision History:
 *! ================
 *! 03-Feb-2000 rr: DBG Levels redefined.
 *! 29-Oct-1999 kc: Cleaned up for code review.
 *! 10-Oct-1997 cr: Added DBG_Printf service.
 *! 29-May-1996 gp: Removed WCD_ prefix.
 *! 15-May-1996 gp: Created.
 */

#ifndef DBG_
#define DBG_

#ifdef __cplusplus
extern "C" {
#endif

#include <dspapi.h>

/* Levels of trace debug messages: */
#ifndef LINUX			/* No DEBUGZONE in Linux, DBG mask == GT mask */
#define DBG_ENTER   (BYTE)(0x01 & DEBUGZONE(0))	/* Function entry point. */
#define DBG_LEVEL1  (BYTE)(0x02 & DEBUGZONE(1))	/* Display debugging state/varibles */
#define DBG_LEVEL2  (BYTE)(0x04 & DEBUGZONE(2))	/* Display debugging state/varibles */
#define DBG_LEVEL3  (BYTE)(0x08 & DEBUGZONE(3))	/* Display debugging state/varibles */
#define DBG_LEVEL4  (BYTE)(0x10 & DEBUGZONE(4))	/* Display debugging state/varibles */
#define DBG_LEVEL5  (BYTE)(0x20 & DEBUGZONE(5))	/* Module Init, Exit */
#define DBG_LEVEL6  (BYTE)(0x40 & DEBUGZONE(6))	/* Warn SERVICES Failures */
#define DBG_LEVEL7  (BYTE)(0x80 & DEBUGZONE(7))	/* Warn Critical Errors */
#else
#define DBG_ENTER   (BYTE)(0x01)	/* Function entry point. */
#define DBG_LEVEL1  (BYTE)(0x02)	/* Display debugging state/varibles */
#define DBG_LEVEL2  (BYTE)(0x04)	/* Display debugging state/varibles */
#define DBG_LEVEL3  (BYTE)(0x08)	/* Display debugging state/varibles */
#define DBG_LEVEL4  (BYTE)(0x10)	/* Display debugging state/varibles */
#define DBG_LEVEL5  (BYTE)(0x20)	/* Module Init, Exit */
#define DBG_LEVEL6  (BYTE)(0x40)	/* Warn SERVICES Failures */
#define DBG_LEVEL7  (BYTE)(0x80)	/* Warn Critical Errors */
#endif

#if ((defined DEBUG) || (defined DDSP_DEBUG_PRODUCT)) && GT_TRACE

/*
 *  ======== DBG_Exit ========
 *  Purpose:
 *      Discontinue usage of module; free resources when reference count 
 *      reaches 0.
 *  Parameters:
 *  Returns:
 *  Requires:
 *      DBG initialized.
 *  Ensures:
 *      Resources used by module are freed when cRef reaches zero.
 */
	extern VOID DBG_Exit();

/*
 *  ======== DBG_Init ========
 *  Purpose:
 *      Initializes private state of DBG module.
 *  Parameters:
 *  Returns:
 *      TRUE if initialized; FALSE if error occured.
 *  Requires:
 *  Ensures:
 */
	extern bool DBG_Init();

#ifndef LINUX
/*
 *  ======== DBG_Printf ========
 *  Purpose:
 *      Output a formatted string to the debugger.
 *  Parameters:
 *      pstrFormat: sprintf-style format string.
 *      ...:        Arguments for format string.
 *  Returns:
 *      DSP_SOK:    Success, or trace level masked.
 *      DSP_EFAIL:  On Error.
 *  Requires:
 *      DBG initialized.
 *  Ensures:
 */
	extern DSP_STATUS DBG_Printf(IN PSTR pstrFormat, ...);
#endif				// LINUX

/*
 *  ======== DBG_Trace ========
 *  Purpose:
 *      Output a trace message to the debugger, if the given trace level
 *      is unmasked.
 *  Parameters:
 *      bLevel:         Trace level.
 *      pstrFormat:     sprintf-style format string.
 *      ...:            Arguments for format string.
 *  Returns:
 *      DSP_SOK:        Success, or trace level masked.
 *      DSP_EFAIL:      On Error.
 *  Requires:
 *      DBG initialized.
 *  Ensures:
 *      Debug message is printed to debugger output window, if trace level
 *      is unmasked.
 */
	extern DSP_STATUS DBG_Trace(IN BYTE bLevel, IN PSTR pstrFormat, ...);
#else

#define DBG_Exit()
#define DBG_Init() TRUE
#define DBG_Trace(bLevel, pstrFormat, args...)

#endif				// ((defined DEBUG) || (defined DDSP_DEBUG_PRODUCT)) && GT_TRACE

#ifdef __cplusplus
}
#endif
#endif				/* DBG_ */
