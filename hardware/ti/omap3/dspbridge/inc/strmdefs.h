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
 *  ======== strmdefs.h ========
 *  DSP-BIOS Bridge driver support functions for TI OMAP processors.
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

