/*
 * dspbridge/mpu_api/inc/mgrpriv.h
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
 *  ======== mgrpriv.h ========
 *  Description:
 *      Global MGR constants and types, shared by PROC, MGR, and WCD.
 *
 *! Revision History:
 *! ================
 *! 29-July-2001 ag: added MGR_PROCESSOREXTINFO.
 *! 05-July-2000 rr: Created
 */

#ifndef MGRPRIV_
#define MGRPRIV_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * OMAP1510 specific
 */
#define MGR_MAXTLBENTRIES  32

/* RM MGR Object */
	struct MGR_OBJECT;
	/*typedef struct MGR_OBJECT *MGR_HOBJECT;*/

	struct MGR_TLBENTRY {
		ULONG ulDspVirt;	/* DSP virtual address */
		ULONG ulGppPhys;	/* GPP physical address */
	} ;

/*
 *  The DSP_PROCESSOREXTINFO structure describes additional extended 
 *  capabilities of a DSP processor not exposed to user.
 */
	struct MGR_PROCESSOREXTINFO {
		struct DSP_PROCESSORINFO tyBasic;	/* user processor info */
		/* private dsp mmu entries */
		struct MGR_TLBENTRY tyTlb[MGR_MAXTLBENTRIES];	
	} ;
	/*MGR_PROCESSOREXTINFO, *MGR_HPROCESSOREXTINFO;*/

#ifdef __cplusplus
}
#endif
#endif				/* MGRPRIV_ */
