/*
 * dspbridge/mpu_api/inc/_dbpriv.h
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
 *  ======== _dbpriv.h ========
 *  Description:
 *      Private header file for the 'Bridge API. Include global defines
 *      and macros.
 *
 *  Public Functions:
 *
 *! Revision History:
 *! ================
 *! 29-Nov-2000 rr: Name changed to _ddsppriv.h. Old types removed.
 *!                 New Macros for checking BadRead/WritePtr.
 *! 12-Apr-2000 ww: Created based on DirectDSP API specification, Version 0.6.
 */

#ifndef _DBPRIV_
#define _DBPRIV_

#ifdef __cplusplus
extern "C" {
#endif

#define DSP_ValidWritePtr(x , y)    ((x)==NULL)
#define DSP_ValidReadPtr(x, y)      ((x)==NULL)
#define DSP_MAX_PROCESSOR           3

#ifdef __cplusplus
}
#endif
#endif				/* _DBPRIV_ */
