/*
 * dspbridge/mpu_api/inc/devdefs.h
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
 *  ======== devdefs.h ========
 *  Purpose:
 *      Definition of common include typedef between wmd.h and dev.h. Required
 *      to break circular dependency between WMD and DEV include files.
 * 
 *! Revision History:
 *! ================
 *! 12-Nov-1996 gp: Renamed from dev1.h.
 *! 30-May-1996 gp: Broke out from dev.h
 */

#ifndef DEVDEFS_
#define DEVDEFS_

#ifdef __cplusplus
extern "C" {
#endif

/* WCD Device Object */
	struct DEV_OBJECT;
	/*typedef struct DEV_OBJECT *DEV_HOBJECT;*/

#ifdef __cplusplus
}
#endif
#endif				/* DEVDEFS_ */
