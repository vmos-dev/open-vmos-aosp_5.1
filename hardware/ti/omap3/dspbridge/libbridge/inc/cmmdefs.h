/*
 * dspbridge/mpu_api/inc/cmmdefs.h
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
 *  ======== cmmdefs.h ========
 *  Purpose:
 *      Global MEM constants and types.
 *
 *! Revision History:
 *! ================
 *! 12-Nov-2001 ag  CMM_KERNMAPTYPE added for dsp<->device process addr map'n.
 *!                 This allows addr conversion from drvr process <-> DSP addr.
 *! 29-Aug-2001 ag  Added CMM_ALLSEGMENTS.
 *! 08-Dec-2000 ag  Added bus address conversion type CMM_POMAPEMIF2DSPBUS.
 *! 05-Dec-2000 ag  Added default CMM_DEFLTCONVFACTOR & CMM_DEFLTDSPADDROFFSET.
 *! 29-Oct-2000 ag  Added converstion factor for GPP DSP Pa translation.
 *! 15-Oct-2000 ag  Added address translator attributes and defaults.
 *! 12-Jul-2000 ag  Created.
 */

#ifndef CMMDEFS_
#define CMMDEFS_

#ifdef __cplusplus
extern "C" {
#endif

#include <list.h>

/* Cmm attributes used in CMM_Create() */
	struct CMM_MGRATTRS {
		ULONG ulMinBlockSize;	/* Minimum SM allocation; default 32 bytes.  */
	};

/* Attributes for CMM_AllocBuf() & CMM_AllocDesc() */
	struct CMM_ATTRS {
		ULONG ulSegId;	/*  1,2... are SM segments. 0 is not. */
		ULONG ulAlignment;	/*  0,1,2,4....ulMinBlockSize */
	} ;

/* 
 *  DSPPa to GPPPa Conversion Factor.
 *  
 *  For typical platforms:
 *      converted Address = PaDSP + ( cFactor * addressToConvert).
 */
	typedef enum {
		CMM_SUBFROMDSPPA = -1,
		CMM_POMAPEMIF2DSPBUS = 0,	/* PreOMAP is special case: not simple offset */
		CMM_ADDTODSPPA = 1
	} CMM_CNVTTYPE;

#define CMM_DEFLTDSPADDROFFSET  0
#define CMM_DEFLTCONVFACTOR     CMM_POMAPEMIF2DSPBUS	/* PreOMAP DSPBUS<->EMIF */
#define CMM_ALLSEGMENTS         0xFFFFFF	/* All SegIds */
#define CMM_MAXGPPSEGS          1	/* Maximum # of SM segs */

/* 
 *  SMSEGs are SM segments the DSP allocates from.
 *  
 *  This info is used by the GPP to xlate DSP allocated PAs.
 */
	 struct CMM_SMSEG {
		DWORD dwSMBasePA;	/* Physical SM Base address(GPP). */
		ULONG ulTotalSizePA;	/* Size of SM segment in GPP bytes. */
		DWORD dwPAPAConvert;	/* DSP PA to GPP PA Conversion.  */
		CMM_CNVTTYPE cFactor;	/* CMM_ADDTOPA=1, CMM_SUBFROMPA=-1 */
	} ;

/* Fixed size memory descriptor */
	 struct CMM_FDESC {
		struct LST_ELEM link;	/* must be 1st element */
		DWORD dwPA;
		DWORD dwVA;
		DWORD dwSize;
		DWORD dwAttrs;	/*  [31-1 reserved ][0 - SM]  */
	} ;

	struct CMM_SEGINFO {
		DWORD dwSegBasePa;	/* Start Phys address of SM segment */
		ULONG ulTotalSegSize;	/* Total size in bytes of segment: DSP+GPP */
		DWORD dwGPPBasePA;	/* Start Phys addr of Gpp SM seg */
		ULONG ulGPPSize;	/* Size of Gpp SM seg in bytes */
		DWORD dwDSPBaseVA;	/* DSP virt base byte address */
		ULONG ulDSPSize;	/* DSP seg size in bytes */
		ULONG ulInUseCnt;	/* # of current GPP allocations from this segment */
		DWORD dwSegBaseVa;	/* Start Virt address of SM seg */

	} ;

/* CMM useful information */
	struct CMM_INFO {
		ULONG ulNumGPPSMSegs;	/* # of SM segments registered with this Cmm. */
		ULONG ulTotalInUseCnt;	/* Total # of allocations outstanding for CMM */
		/* Min SM block size allocation from CMM_Create() */
		ULONG ulMinBlockSize;	
		/* Info per registered SM segment. */
		struct CMM_SEGINFO segInfo[CMM_MAXGPPSEGS];	
	} ;

/* XlatorCreate attributes */
	struct CMM_XLATORATTRS {
		ULONG ulSegId;	/* segment Id used for SM allocations */
		DWORD dwDSPBufs;	/* # of DSP-side bufs */
		DWORD dwDSPBufSize;	/* size of DSP-side bufs in GPP bytes */
		PVOID pVmBase;	/* Vm base address alloc'd in client process context */
		DWORD dwVmSize;	/* dwVmSize must be >= (dwMaxNumBufs * dwMaxSize) */
	} ;

/* Descriptor attributes */
	typedef enum {
		CMM_LOCAL = 0,	/* Bit 0 =0 is local memory from default heap */
		CMM_SHARED = 1,	/* Bit 0 =1 descriptor is SM */
		/* Bits 1- 31 RESERVED for future use */
	} CMM_DESCTYPE;

/*
 * Cmm translation types. Use to map SM addresses to process context.
 */
	typedef enum {
		CMM_VA2PA = 0,	/* Virtual to GPP physical address xlation */
		CMM_PA2VA = 1,	/* GPP Physical to virtual  */
		CMM_VA2DSPPA = 2,	/* Va to DSP Pa  */
		CMM_PA2DSPPA = 3,	/* GPP Pa to DSP Pa */
		CMM_DSPPA2PA = 4,	/* DSP Pa to GPP Pa */
	} CMM_XLATETYPE;

/*
 *  Used to "map" between device process virt addr and dsp addr. 
 */
	typedef enum {
		CMM_KERNVA2DSP = 0,	/* Device process context to dsp address. */
		CMM_DSP2KERNVA = 1,	/* Dsp address to device process context. */
	} CMM_KERNMAPTYPE;

	struct CMM_OBJECT;
	/*typedef struct CMM_OBJECT *CMM_HMGR;*/
	struct CMM_XLATOROBJECT;
	/*typedef struct CMM_XLATOROBJECT *CMM_HXLATOR;*/

#ifdef __cplusplus
}
#endif
#endif				/* CMMDEFS_ */
