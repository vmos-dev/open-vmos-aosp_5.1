/*
 *  Copyright 2001-2008 Texas Instruments - http://www.ti.com/
 * 
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/*
 *  ======== dsptrap.h ========
 *  DSP-BIOS Bridge driver support functions for TI OMAP processors.
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
