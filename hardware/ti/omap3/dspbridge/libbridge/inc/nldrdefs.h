/*
 * dspbridge/mpu_api/inc/nldrdefs.h
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
 *  ======== nldrdefs.h ========
 *  Description:
 *      Global Dynamic + static/overlay Node loader (NLDR) constants and types.
 *
 *! Revision History
 *! ================
 *! 07-Apr-2003 map     Consolidated dldrdefs.h into nldrdefs.h
 *! 05-Aug-2002 jeh     Created.
 */

#ifndef NLDRDEFS_
#define NLDRDEFS_

#ifdef __cplusplus
extern "C" {
#endif

#include <dbdcddef.h>
#include <devdefs.h>

#define NLDR_MAXPATHLENGTH       255
/* NLDR Objects: */
	struct  NLDR_OBJECT;
	/*typedef struct NLDR_OBJECT *NLDR_HOBJECT;*/
	struct NLDR_NODEOBJECT;
	/*typedef struct NLDR_NODEOBJECT *NLDR_HNODE;*/

/*
 *  ======== NLDR_LOADTYPE ========
 *  Load types for a node. Must match values in node.h55.
 */
	typedef enum NLDR_LOADTYPE {
		NLDR_STATICLOAD,	/* Linked in base image, not overlay */
		NLDR_DYNAMICLOAD,	/* Dynamically loaded node */
		NLDR_OVLYLOAD	/* Linked in base image, overlay node */
	} NLDR_LOADTYPE;

/*
 *  ======== NLDR_OVLYFXN ========
 *  Causes code or data to be copied from load address to run address. This
 *  is the "COD_WRITEFXN" that gets passed to the DBLL_Library and is used as
 *  the ZL write function.
 *
 *  Parameters:
 *      pPrivRef:       Handle to identify the node.
 *      ulDspRunAddr:   Run address of code or data.
 *      ulDspLoadAddr:  Load address of code or data.
 *      ulNumBytes:     Number of (GPP) bytes to copy.
 *      nMemSpace:      RMS_CODE or RMS_DATA.
 *  Returns:
 *      ulNumBytes:     Success.
 *      0:              Failure.
 *  Requires:
 *  Ensures:
 */
	typedef ULONG(CDECL * NLDR_OVLYFXN) (PVOID pPrivRef, ULONG ulDspRunAddr,
					     ULONG ulDspLoadAddr,
					     ULONG ulNumBytes, UINT nMemSpace);

/*
 *  ======== NLDR_WRITEFXN ========
 *  Write memory function. Used for dynamic load writes.
 *  Parameters:
 *      pPrivRef:       Handle to identify the node.
 *      ulDspAddr:      Address of code or data.
 *      pBuf:           Code or data to be written
 *      ulNumBytes:     Number of (GPP) bytes to write.
 *      nMemSpace:      DBLL_DATA or DBLL_CODE.
 *  Returns:
 *      ulNumBytes:     Success.
 *      0:              Failure.
 *  Requires:
 *  Ensures:
 */
	typedef ULONG(CDECL * NLDR_WRITEFXN) (PVOID pPrivRef,
					      ULONG ulDspAddr, PVOID pBuf,
					      ULONG ulNumBytes, UINT nMemSpace);

/*
 *  ======== NLDR_ATTRS ========
 *  Attributes passed to NLDR_Create function.
 */
	struct NLDR_ATTRS {
		NLDR_OVLYFXN pfnOvly;
		NLDR_WRITEFXN pfnWrite;
		USHORT usDSPWordSize;
		USHORT usDSPMauSize;
	} ;

/*
 *  ======== NLDR_PHASE ========
 *  Indicates node create, delete, or execute phase function.
 */
	typedef enum {
		NLDR_CREATE,
		NLDR_DELETE,
		NLDR_EXECUTE,
		NLDR_NOPHASE
	} NLDR_PHASE;

/*
 *  Typedefs of loader functions imported from a DLL, or defined in a
 *  function table.
 */

/*
 *  ======== NLDR_Allocate ========
 *  Allocate resources to manage the loading of a node on the DSP.
 *
 *  Parameters:
 *      hNldr:          Handle of loader that will load the node.
 *      pPrivRef:       Handle to identify the node.
 *      pNodeProps:     Pointer to a DCD_NODEPROPS for the node.
 *      phNldrNode:     Location to store node handle on output. This handle
 *                      will be passed to NLDR_Load/NLDR_Unload.
 *      pfPhaseSplit:   pointer to boolean variable referenced in node.c
 *  Returns:
 *      DSP_SOK:        Success.
 *      DSP_EMEMORY:    Insufficient memory on GPP.
 *  Requires:
 *      NLDR_Init() called.
 *      Valid hNldr.
 *      pNodeProps != NULL.
 *      phNldrNode != NULL.
 *  Ensures:
 *      DSP_SOK:        IsValidNode(*phNldrNode).
 *      error:          *phNldrNode == NULL.
 */
	typedef DSP_STATUS(*NLDR_ALLOCATEFXN) (struct NLDR_OBJECT* hNldr,
					       PVOID pPrivRef,
					       IN CONST struct DCD_NODEPROPS *pNodeProps,
					       OUT struct NLDR_NODEOBJECT* * phNldrNode,
					       OUT bool * pfPhaseSplit);

/*
 *  ======== NLDR_Create ========
 *  Create a loader object. This object handles the loading and unloading of
 *  create, delete, and execute phase functions of nodes on the DSP target.
 *
 *  Parameters:
 *      phNldr:         Location to store loader handle on output.
 *      hDevObject:     Device for this processor.
 *      pAttrs:         Loader attributes.
 *  Returns:
 *      DSP_SOK:        Success;
 *      DSP_EMEMORY:    Insufficient memory for requested resources.
 *  Requires:
 *      NLDR_Init() called.
 *      phNldr != NULL.
 *      hDevObject != NULL.
 *	pAttrs != NULL.
 *  Ensures:
 *      DSP_SOK:        Valid *phNldr.
 *      error:          *phNldr == NULL.
 */
	typedef DSP_STATUS(*NLDR_CREATEFXN) (OUT struct NLDR_OBJECT* * phNldr,
					     struct DEV_OBJECT* hDevObject,
					     IN CONST struct NLDR_ATTRS * pAttrs);

/*
 *  ======== NLDR_Delete ========
 *  Delete the NLDR loader.
 *
 *  Parameters:
 *      hNldr:          Node manager object.
 *  Returns:
 *  Requires:
 *      NLDR_Init() called.
 *      Valid hNldr.
 *  Ensures:
 *	hNldr invalid
 */
	typedef VOID(*NLDR_DELETEFXN) (struct NLDR_OBJECT* hNldr);

/*
 *  ======== NLDR_Exit ========
 *  Discontinue usage of NLDR module.
 *
 *  Parameters:
 *  Returns:
 *  Requires:
 *      NLDR_Init() successfully called before.
 *  Ensures:
 *      Any resources acquired in NLDR_Init() will be freed when last NLDR
 *      client calls NLDR_Exit().
 */
	typedef VOID(*NLDR_EXITFXN) ();

/*
 *  ======== NLDR_Free ========
 *  Free resources allocated in NLDR_Allocate.
 *
 *  Parameters:
 *      hNldrNode:      Handle returned from NLDR_Allocate().
 *  Returns:
 *  Requires:
 *      NLDR_Init() called.
 *      Valid hNldrNode.
 *  Ensures:
 */
	typedef VOID(*NLDR_FREEFXN) (struct NLDR_NODEOBJECT* hNldrNode);

/*
 *  ======== NLDR_GetFxnAddr ========
 *  Get address of create, delete, or execute phase function of a node on
 *  the DSP.
 *
 *  Parameters:
 *      hNldrNode:      Handle returned from NLDR_Allocate().
 *      pstrFxn:        Name of function.
 *      pulAddr:        Location to store function address.
 *  Returns:
 *      DSP_SOK:        Success.
 *      DSP_ESYMBOL:    Address of function not found.
 *  Requires:
 *      NLDR_Init() called.
 *      Valid hNldrNode.
 *      pulAddr != NULL;
 *      pstrFxn != NULL;
 *  Ensures:
 */
	typedef DSP_STATUS(*NLDR_GETFXNADDRFXN) (struct NLDR_NODEOBJECT* hNldrNode,
						 PSTR pstrFxn, ULONG * pulAddr);

/*
 *  ======== NLDR_Init ========
 *  Initialize the NLDR module.
 *
 *  Parameters:
 *  Returns:
 *      TRUE if initialization succeeded, FALSE otherwise.
 *  Ensures:
 */
	typedef bool(*NLDR_INITFXN) ();

/*
 *  ======== NLDR_Load ========
 *  Load create, delete, or execute phase function of a node on the DSP.
 *
 *  Parameters:
 *      hNldrNode:      Handle returned from NLDR_Allocate().
 *      phase:          Type of function to load (create, delete, or execute).
 *  Returns:
 *      DSP_SOK:                Success.
 *      DSP_EMEMORY:            Insufficient memory on GPP.
 *      DSP_EOVERLAYMEMORY:     Can't overlay phase because overlay memory
 *                              is already in use.
 *      DSP_EDYNLOAD:           Failure in dynamic loader library.
 *      DSP_EFWRITE:            Failed to write phase's code or date to target.
 *  Requires:
 *      NLDR_Init() called.
 *      Valid hNldrNode.
 *  Ensures:
 */
	typedef DSP_STATUS(*NLDR_LOADFXN) (struct NLDR_NODEOBJECT* hNldrNode,
					   NLDR_PHASE phase);

/*
 *  ======== NLDR_Unload ========
 *  Unload create, delete, or execute phase function of a node on the DSP.
 *
 *  Parameters:
 *      hNldrNode:      Handle returned from NLDR_Allocate().
 *      phase:          Node function to unload (create, delete, or execute).
 *  Returns:
 *      DSP_SOK:        Success.
 *      DSP_EMEMORY:    Insufficient memory on GPP.
 *  Requires:
 *      NLDR_Init() called.
 *      Valid hNldrNode.
 *  Ensures:
 */
	typedef DSP_STATUS(*NLDR_UNLOADFXN) (struct NLDR_NODEOBJECT* hNldrNode,
					     NLDR_PHASE phase);

/*
 *  ======== NLDR_FXNS ========
 */
	struct NLDR_FXNS {
		NLDR_ALLOCATEFXN pfnAllocate;
		NLDR_CREATEFXN pfnCreate;
		NLDR_DELETEFXN pfnDelete;
		NLDR_EXITFXN pfnExit;
		NLDR_FREEFXN pfnFree;
		NLDR_GETFXNADDRFXN pfnGetFxnAddr;
		NLDR_INITFXN pfnInit;
		NLDR_LOADFXN pfnLoad;
		NLDR_UNLOADFXN pfnUnload;
	} ;

#ifdef __cplusplus
}
#endif
#endif				/* NLDRDEFS_ */
