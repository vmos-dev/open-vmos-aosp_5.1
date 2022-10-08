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
 *  ======== memdefs.h ========
 *  DSP-BIOS Bridge driver support functions for TI OMAP processors.
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
