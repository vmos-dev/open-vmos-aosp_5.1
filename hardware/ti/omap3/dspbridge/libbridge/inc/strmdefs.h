/*
 * dspbridge/mpu_api/inc/strmdefs.h
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
 *  ======== strmdefs.h ========
 *  Purpose:
 *      Global STRM constants and types.
 *
 *! Revision History
 *! ================
 *! 19-Nov-2001 ag      Added STRM_INFO..
 *! 25-Sep-2000 jeh     Created.
 */

#ifndef STRMDEFS_
#define STRMDEFS_

#ifdef __cplusplus
extern "C" {
#endif

#define STRM_MAXEVTNAMELEN      32

	struct STRM_MGR;
	/*typedef struct STRM_MGR *STRM_HMGR;*/
	struct STRM_OBJECT;
	/*typedef struct STRM_OBJECT *STRM_HOBJECT;*/

	struct STRM_ATTR {
		HANDLE hUserEvent;
		PSTR pstrEventName;
		PVOID pVirtBase;	/* Process virtual base address of mapped SM */
		ULONG ulVirtSize;	/* Size of virtual space in bytes */
		struct DSP_STREAMATTRIN *pStreamAttrIn;
	} ;

	struct STRM_INFO {
		UINT lMode;	/* transport mode of stream(DMA, ZEROCOPY..) */
		UINT uSegment;	/* Segment strm allocs from. 0 is local mem */
		PVOID pVirtBase;	/*    "       "    Stream'process virt base */
		struct DSP_STREAMINFO *pUser;	/* User's stream information returned */
	} ;

#ifdef __cplusplus
}
#endif
#endif				/* STRMDEFS_ */

