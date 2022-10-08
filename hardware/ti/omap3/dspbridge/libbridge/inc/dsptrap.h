/*
 * dspbridge/mpu_api/inc/dsptrap.h
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
 *  ======== dsptrap.h ========
 *  Purpose:
 *      Handles interaction between user and driver layers.
 *
 *! Revision History
 *! ================
 *! 13-Feb-2001 kc: DSP/BIOS Bridge name updates.
 *! 28-Jan-2000 rr: New define for the TI Function offset.
 *!                 NT_CMD_FROM_OFFSET moved in from wcdioctl.h
 *!                 It is not hard coded any more; can be used by the class
 *!                 driver as well.
 *! 08-Oct-1999 rr: header information changed to dbclsdrv.dll
 *! 18-Aug-1999 rr: Created
 */

#ifndef DSPTRAP_
#define DSPTRAP_

#include <wcdioctl.h>

#ifndef LINUX
#define TI_FUNCTION_OFFSET  0x5000

#define NT_CMD_FROM_OFFSET(x) CTL_CODE(FILE_DEVICE_UNKNOWN, \
    (TI_FUNCTION_OFFSET + (x)), METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

/* Function Prototypes */
extern DWORD DSPTRAP_Trap(Trapped_Args * args, int cmd);

#endif				/* DSPTRAP_ */
