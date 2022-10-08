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
 *  ======== cfgdefs.h ========
 *  DSP-BIOS Bridge driver support functions for TI OMAP processors.
 *  Purpose:
 *      Global CFG constants and types, shared between class and mini driver.
 * 
 *! Revision History:
 *! ================
 *! 24-Feb-2003 kc  Removed wIOPort* in CFG_HOSTRES.
 *! 06-Sep-2000 jeh Added channel info to CFG_HOSTRES.
 *! 09-May-2000 rr: CFG_HOSTRES now support multiple windows for PCI support.
 *! 31-Jan-2000 rr: Comments changed after code review.
 *! 06-Jan-2000 rr: Bus Type included in CFG_HOSTRES.
 *! 12-Nov-1999 rr: CFG_HOSTRES member names changed.
 *! 25-Oct-1999 rr: Modified the CFG_HOSTRES Structure
 *!                 PCMCIA ISR Register/Unregister fxn removed.. 
 *!                 New flag PCCARD introduced during compile time.
 *! 10-Sep-1999 ww: Added PCMCIA ISR Register/Unregister fxn.
 *! 01-Sep-1999 ag: Removed NT/95 specific fields in CFG_HOSTRES 
 *! 27-Oct-1997 cr: Updated CFG_HOSTRES struct to support 1+ IRQs per board.
 *! 17-Sep-1997 gp: Tacked some NT config info to end of CFG_HOSTRES structure.
 *! 12-Dec-1996 cr: Cleaned up after code review.
 *! 14-Nov-1996 gp: Renamed from wsxcfg.h
 *! 19-Jun-1996 cr: Created.
 */

#ifndef CFGDEFS_
#define CFGDEFS_

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum length of module search path. */
#define CFG_MAXSEARCHPATHLEN    255

/* Maximum length of general paths. */
#define CFG_MAXPATH             255

/* Host Resources:  */
#define CFG_MAXMEMREGISTERS     9
#define CFG_MAXIOPORTS          20
#define CFG_MAXIRQS             7
#define CFG_MAXDMACHANNELS      7

/* IRQ flag */
#define CFG_IRQSHARED           0x01	/* IRQ can be shared */

/* DSP Resources: */
#define CFG_DSPMAXMEMTYPES      10
#define CFG_DEFAULT_NUM_WINDOWS 1	/* We support only one window. */

/* A platform-related device handle: */
	/*typedef struct CFG_DEVNODE *CFG_HDEVNODE;*/
	struct CFG_DEVNODE;

/* 
 *  Host resource structure.  
 */
	struct CFG_HOSTRES {
		DWORD wNumMemWindows;	/* Set to default */
		/* This is the base.memory */
		DWORD dwMemBase[CFG_MAXMEMREGISTERS];	/* SHM virtual address */
		DWORD dwMemLength[CFG_MAXMEMREGISTERS];	/* Length of the  Base */
		DWORD dwMemPhys[CFG_MAXMEMREGISTERS];	/* SHM Physical address */
		BYTE bIRQRegisters;	/* IRQ Number */
		BYTE bIRQAttrib;	/* IRQ Attribute */
		DWORD dwOffsetForMonitor;	/* The Shared memory starts from
						 * dwMemBase + this offset
						 */
		DWORD dwBusType;	/* Bus type for this device */
		DWORD dwProgBase;	/* DSP ProgBase */
		DWORD dwProgLength;	/* DSP ProgBase Length */
		DWORD dwRegBase;	/* DSP memory mapped register base */
		DWORD dwRegLength;	/* DSP Register Base Length */
		DWORD ClientHandle;	/* Client Handle */
		DWORD SocketHandle;	/* Socket and Function Pair */
		DWORD CardInfo;	/* This will be used as a context data in 
				 * in the CardRequestIRQ
				 */
		/*
		 *  Info needed by NODE for allocating channels to communicate with RMS:
		 *      dwChnlOffset:       Offset of RMS channels. Lower channels are
		 *                          reserved.
		 *      dwChnlBufSize:      Size of channel buffer to send to RMS
		 *      dwNumChnls:       Total number of channels (including reserved).
		 */
		DWORD dwChnlOffset;
		DWORD dwChnlBufSize;
		DWORD dwNumChnls;

#ifdef OMAP_2430
		DWORD dwPrcmBase;
		DWORD dwWdTimerDspBase;
		DWORD dwMboxBase;
		DWORD dwDmmuBase;
		DWORD dwDipiBase;
		DWORD dwSysCtrlBase;
#endif

#ifdef OMAP_3430
		DWORD dwPrmBase;
		DWORD dwCmBase;
		DWORD dwPerBase;
		DWORD dwWdTimerDspBase;
		DWORD dwMboxBase;
		DWORD dwDmmuBase;
		DWORD dwDipiBase;
		DWORD dwSysCtrlBase;
#endif
	} ;

	struct CFG_DSPMEMDESC {
		UINT uMemType;	/* Type of memory.                        */
		ULONG ulMin;	/* Minimum amount of memory of this type. */
		ULONG ulMax;	/* Maximum amount of memory of this type. */
	} ;

	struct CFG_DSPRES {
		UINT uChipType;	/* DSP chip type.               */
		UINT uWordSize;	/* Number of bytes in a word    */
		UINT cChips;	/* Number of chips.             */
		UINT cMemTypes;	/* Types of memory.             */
		struct CFG_DSPMEMDESC aMemDesc[CFG_DSPMAXMEMTYPES];	
		/* DSP Memory types */
	} ;

#ifdef __cplusplus
}
#endif
#endif				/* CFGDEFS_ */
