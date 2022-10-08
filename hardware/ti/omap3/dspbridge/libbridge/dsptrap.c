/*
 * dspbridge/src/api/linux/dsptrap.c
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
 *  ======== dsptrap.c ========
 *  Description:
 *      Source for trap hand-shaking (involves DeviceIOControl).
 *
 *
 *! Revision History
 *! =================
 *! 28-Jan-2000 rr: NT_CMD_FROM_OFFSET moved to dsptrap.h
 *! 02-Dec-1999 rr: DeviceIOControl now returns BOOL Value so !fSuccess
 *!                 indicates failure.
 *! 02-Nov-1999 kc: Modified to enable return values from BRD API calls.
 *! 01-Oct-1999 ag: DSPTRAP_Trap() now returns correct status.
 *! 18-Aug-1999 rr: Created.Ported from WSX tree.
 *
 */

/*  ----------------------------------- Host OS */
#include <host_os.h>

/*  ----------------------------------- DSP/BIOS Bridge */
#include <dbdefs.h>
#include <errbase.h>

/*  ----------------------------------- This */
#include <dsptrap.h>
#include <_dbdebug.h>

/*  ----------------------------------- Globals */
extern int hMediaFile;		/* class driver handle */

/*
 * ======== DSPTRAP_Trap ========
 */
DWORD DSPTRAP_Trap(Trapped_Args *args, int cmd)
{
	DWORD dwResult = DSP_EHANDLE;/* returned from call into class driver */

	if (hMediaFile >= 0)
		dwResult = ioctl(hMediaFile, cmd, args);
	else
		DEBUGMSG(DSPAPI_ZONE_FUNCTION, "Invalid handle to driver\n");

	return dwResult;
}
