/*
 * dspbridge/mpu_api/inc/memdefs.h
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
 *  ======== memdefs.h ========
 *  Purpose:
 *      Global MEM constants and types, shared between WSX, WCD, and WMD.
 *
 *! Revision History:
 *! ================
 *! 28-Aug-2001 ag:  Added MEM_[SET][GET]VIRTUALSEGID.
 *! 10-Aug-1999 kc:  Based on wsx-c18.
 *! 15-Nov-1996 gp:  Renamed from wsxmem.h and moved to kwinos.
 *! 21-Aug-1996 cr:  Created from mem.h.
 */

#ifndef MEMDEFS_
#define MEMDEFS_

#ifdef __cplusplus
extern "C" {
#endif

/* Memory Pool Attributes: */
	typedef enum {
		MEM_PAGED = 0,
		MEM_NONPAGED = 1,
		MEM_LARGEVIRTMEM = 2
	} MEM_POOLATTRS;

/*
 *  MEM_VIRTUALSEGID is used by Node & Strm to access virtual address space in
 *  the correct client process context. 
 */
#define MEM_SETVIRTUALSEGID     0x10000000
#define MEM_GETVIRTUALSEGID     0x20000000
#define MEM_MASKVIRTUALSEGID    (MEM_SETVIRTUALSEGID | MEM_GETVIRTUALSEGID)

#ifdef LINUX

#define TO_VIRTUAL_UNCACHED(x) x
#define INTREG_TO_VIRTUAL_UNCACHED(x) x

#endif

#ifdef __cplusplus
}
#endif
#endif				/* MEMDEFS_ */
