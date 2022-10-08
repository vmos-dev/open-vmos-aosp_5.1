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
 *  ======== mgrpriv.h ========
 *  DSP-BIOS Bridge driver support functions for TI OMAP processors.
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
