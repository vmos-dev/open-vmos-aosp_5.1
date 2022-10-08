/*
 * dspbridge/src/api/linux/DSPProcessor.c
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
 *  ======== DSPProcessor.c ========
 *  Description:
 *      This is the source for the DSP/BIOS Bridge API processor module. The
 *      parameters are validated at the API level, but the bulk of the
 *      work is done at the driver level through the PM PROC module.
 *
 *  Public Functions:
 *      DSPProcessor_Attach
 *      DSPProcessor_Detach
 *      DSPProcessor_EnumNodes
 *      DSPProcessor_FlushMemory
 *      DSPProcessor_GetResourceInfo
 *      DSPProcessor_GetState
 *      DSPProcessor_Map
 *      DSPProcessor_RegisterNotify
 *      DSPProcessor_ReserveMemory
 *      DSPProcessor_UnMap
 *      DSPProcessor_UnReserveMemory
 *      DSPProcessor_InvalidateMemory

 *! Revision History
 *! ================
 *! 04-Apr-2007 sh  Added DSPProcessor_InvalidateMemory
 *! 19-Apr-2004 sb  Aligned DMM definitions with Symbian
 *! 08-Mar-2004 sb  Added the Dynamic Memory Mapping APIs
 *! 27-Jun-2001 rr: DSPProcessor_RegisterNotify allows EventMask =0
 *!                 for De Registration.
 *! 16-Feb-2001 jeh Fixed message in DSPProcessor_Detach.
 *! 12-Dec-2000 rr: DSP_ProcessorEnumNodes returns DSP_ESIZE if
 *!                 uNodeTabSize is zero and valid aNodeTab.
 *! 29-Nov-2000 rr: Incorporated code review changes.
 *! 09-Nov-2000 rr: Code cleaned up. Use of IsValidEvent/Mask Macros.
 *! 28-Sep-2000 rr: Updated to version 0.9.
 *! 07-Sep-2000 jeh Changed type HANDLE in DSPProcessor_RegisterNotify to
 *!                 DSP_HNOTIFICATION.
 *! 07-Aug-2000 rr: Enum fxns do not return ESIZE if the size of the data
 *!                 structure is less than the actual size for backward
 *!                 compatibility.
 *! 04-Aug-2000 rr: DSPProcessor_Attach does not check for pAttrin for NULL.
 *!                 file name changed to DSPProcessor.c
 *! 27-Jul-2000 rr: Updated to 0.8 ver API. GetTrace Implemented.
 *! 10-Jul-2000 rr: Calls into DSP Trap for the bulk of the functionality.
 *! 12-May-2000 gp: Removed PROC_UNKNOWN state.  Mapped to return DSP_EFAIL.
 *!                 Return DSP_EHANDLE in DSPProcessor_Ctrl()/Detach().
 *!                 Return DSP_EWRONGSTATE from DSPProcessor_Start().
 *! 03-May-2000 rr: Uses SERVICES CSL fxns
 *! 19-Apr-2000 ww: Updated based on code review.
 *! 12-Apr-2000 ww: Created based on DirectDSP API specification, Version 0.6.
 *
 */

/*  ----------------------------------- Host OS */
#include <host_os.h>

/*  ----------------------------------- DSP/BIOS Bridge */
#include <dbdefs.h>
#include <errbase.h>

/*  ----------------------------------- Others */
#include <dsptrap.h>

#ifdef DEBUG_BRIDGE_PERF
#include <perfutils.h>
#endif
/*  ----------------------------------- This */
#include "_dbdebug.h"
#include "_dbpriv.h"
#include <DSPProcessor.h>

/*
 *  ======== DSPProcessor_Attach ========
 *  Purpose:
 *      Prepare for communication with a particular DSP processor, and
 *		return a handle to the processor object.
 */
DBAPI DSPProcessor_Attach(UINT uProcessor,
		    OPTIONAL CONST struct DSP_PROCESSORATTRIN *pAttrIn,
		    OUT DSP_HPROCESSOR *phProcessor)
{
	DSP_STATUS status = DSP_SOK;
	Trapped_Args tempStruct;

	DEBUGMSG(DSPAPI_ZONE_FUNCTION, (TEXT("PROC: DSPProcessor_Attach\r\n")));
	if (!DSP_ValidWritePtr(phProcessor, sizeof(DSP_HPROCESSOR))) {
		if (uProcessor <= DSP_MAX_PROCESSOR) {
			tempStruct.ARGS_PROC_ATTACH.uProcessor = uProcessor;
			tempStruct.ARGS_PROC_ATTACH.pAttrIn =
				(struct DSP_PROCESSORATTRIN *)pAttrIn;
			tempStruct.ARGS_PROC_ATTACH.phProcessor = phProcessor;
			status = DSPTRAP_Trap(&tempStruct,
					CMD_PROC_ATTACH_OFFSET);
		} else {
			status = DSP_EINVALIDARG;
			DEBUGMSG(DSPAPI_ZONE_ERROR,
				(TEXT("PROC: invalid processor number\r\n")));
		}
	} else {
		/* Invalid pointer */
		status = DSP_EPOINTER;
		DEBUGMSG(DSPAPI_ZONE_ERROR,
				(TEXT("PROC: Invalid Pointer \r\n")));
	}

	return status;
}

/*
 *  ======== DSPProcessor_Detach ========
 *  Purpose:
 *      Close a DSP processor and de-allocate all (GPP) resources.
 */
DBAPI DSPProcessor_Detach(DSP_HPROCESSOR hProcessor)
{
	DSP_STATUS status = DSP_SOK;
	Trapped_Args tempStruct;

	DEBUGMSG(DSPAPI_ZONE_FUNCTION, (TEXT("PROC: DSPProcessor_Detach\r\n")));
	/* Check the handle */
	if (hProcessor) {
		tempStruct.ARGS_PROC_DETACH.hProcessor = hProcessor;
		status = DSPTRAP_Trap(&tempStruct, CMD_PROC_DETACH_OFFSET);
	} else {
		/* Invalid handle */
		status = DSP_EHANDLE;
		DEBUGMSG(DSPAPI_ZONE_ERROR,
			(TEXT("PROC: Invalid Handle \r\n")));
	}

	return status;
}

/*
 *  ======== DSPProcessor_EnumNodes ========
 *  Purpose:
 *      Enumerate and get configuration information about nodes allocated
 *      on a DSP processor.
 */
DBAPI DSPProcessor_EnumNodes(DSP_HPROCESSOR hProcessor,
		       IN DSP_HNODE *aNodeTab, IN UINT uNodeTabSize,
		       OUT UINT *puNumNodes, OUT UINT *puAllocated)
{
	DSP_STATUS status = DSP_SOK;
	Trapped_Args tempStruct;

	DEBUGMSG(DSPAPI_ZONE_FUNCTION,
			(TEXT("PROC:DSPProcessor_EnumNodes\r\n")));

	/* Check the handle */
	if (hProcessor) {
		if (!DSP_ValidWritePtr(puNumNodes, sizeof(UINT)) &&
		    !DSP_ValidWritePtr(puAllocated, sizeof(UINT)) &&
			(uNodeTabSize && !DSP_ValidWritePtr(aNodeTab,
					(sizeof(DSP_HNODE)*uNodeTabSize)))) {
			tempStruct.ARGS_PROC_ENUMNODE_INFO.hProcessor =
						hProcessor;
			tempStruct.ARGS_PROC_ENUMNODE_INFO.aNodeTab = aNodeTab;
			tempStruct.ARGS_PROC_ENUMNODE_INFO.uNodeTabSize =
						uNodeTabSize;
			tempStruct.ARGS_PROC_ENUMNODE_INFO.puNumNodes =
						puNumNodes;
			tempStruct.ARGS_PROC_ENUMNODE_INFO.puAllocated =
						puAllocated;
			status = DSPTRAP_Trap(&tempStruct,
					CMD_PROC_ENUMNODE_OFFSET);
		} else {
			if (uNodeTabSize <= 0 &&
			    !DSP_ValidWritePtr(puNumNodes, sizeof(UINT)) &&
			    !DSP_ValidWritePtr(puAllocated, sizeof(UINT)) &&
			    !DSP_ValidWritePtr(aNodeTab, sizeof(DSP_HNODE)*1)) {
				status = DSP_ESIZE;
			} else
				status = DSP_EPOINTER;

			DEBUGMSG(DSPAPI_ZONE_ERROR, (TEXT("PROC: "
					"pNodeInfo is invalid \r\n")));
		}
	} else {
		/* Invalid handle */
		status = DSP_EHANDLE;
		DEBUGMSG(DSPAPI_ZONE_ERROR,
				(TEXT("PROC: Invalid Handle \r\n")));
	}

	return status;
}

/*
 *  ======== DSPProcessor_FlushMemory ========
 *  Purpose:
 *      Flushes a buffer from the MPU data cache.
 */
DBAPI DSPProcessor_FlushMemory(DSP_HPROCESSOR hProcessor, PVOID pMpuAddr,
			 ULONG ulSize, ULONG ulFlags)
{
	DSP_STATUS status = DSP_SOK;
	Trapped_Args tempStruct;
#ifdef DEBUG_BRIDGE_PERF
	struct timeval tv_beg;
	struct timeval tv_end;
	struct timezone tz;
	int timeRetVal = 0;

timeRetVal = getTimeStamp(&tv_beg);

#endif

	DEBUGMSG(DSPAPI_ZONE_FUNCTION,
			(TEXT("PROC: DSPProcessor_FlushMemory\r\n")));

	/* Check the handle */
	if (hProcessor) {
		tempStruct.ARGS_PROC_FLUSHMEMORY.hProcessor = hProcessor;
		tempStruct.ARGS_PROC_FLUSHMEMORY.pMpuAddr = pMpuAddr;
		tempStruct.ARGS_PROC_FLUSHMEMORY.ulSize = ulSize;
		tempStruct.ARGS_PROC_FLUSHMEMORY.ulFlags = ulFlags;
		status = DSPTRAP_Trap(&tempStruct, CMD_PROC_FLUSHMEMORY_OFFSET);
	} else {
		/* Invalid handle */
		status = DSP_EHANDLE;
		DEBUGMSG(DSPAPI_ZONE_ERROR, (TEXT("PROC: Invalid Handle\r\n")));
	}
#ifdef DEBUG_BRIDGE_PERF
timeRetVal = getTimeStamp(&tv_end);
	PrintStatistics(&tv_beg, &tv_end, "DSPProcessor_FlushMemory", ulSize);
#endif

	return status;

}

/*
 *  ======== DSPProcessor_InvalidateMemory ========
 *  Purpose:
 *      Invalidates a buffer from MPU data cache.
 */
DBAPI DSPProcessor_InvalidateMemory(DSP_HPROCESSOR hProcessor,
					PVOID pMpuAddr, ULONG ulSize)
{
	DSP_STATUS status = DSP_SOK;
	Trapped_Args tempStruct;
#ifdef DEBUG_BRIDGE_PERF
	struct timeval tv_beg;
	struct timeval tv_end;
	struct timezone tz;
	int timeRetVal = 0;

	timeRetVal = getTimeStamp(&tv_beg);
#endif

	DEBUGMSG(DSPAPI_ZONE_FUNCTION,
			(TEXT("PROC: DSPProcessor_InvalidateMemory\r\n")));

	/* Check the handle */
	if (hProcessor) {
		tempStruct.ARGS_PROC_INVALIDATEMEMORY.hProcessor = hProcessor;
		tempStruct.ARGS_PROC_INVALIDATEMEMORY.pMpuAddr = pMpuAddr;
		tempStruct.ARGS_PROC_INVALIDATEMEMORY.ulSize = ulSize;
		status = DSPTRAP_Trap(&tempStruct,
				CMD_PROC_INVALIDATEMEMORY_OFFSET);
	} else {
		/* Invalid handle */
		status = DSP_EHANDLE;
		DEBUGMSG(DSPAPI_ZONE_ERROR, (TEXT("PROC: Invalid Handle\r\n")));
	}
#ifdef DEBUG_BRIDGE_PERF
	timeRetVal = getTimeStamp(&tv_end);
	PrintStatistics(&tv_beg, &tv_end,
			"DSPProcessor_InvalidateMemory", ulSize);
#endif

	return status;

}

/*
 *  ======== DSPProcessor_GetResourceInfo ========
 *  Purpose:
 *      Enumerate the resources currently available on a processor.
 */
DBAPI DSPProcessor_GetResourceInfo(DSP_HPROCESSOR hProcessor,
	     UINT uResourceType, OUT struct DSP_RESOURCEINFO *pResourceInfo,
	     UINT uResourceInfoSize)
{
	DSP_STATUS status = DSP_SOK;
	Trapped_Args tempStruct;

	DEBUGMSG(DSPAPI_ZONE_FUNCTION, (TEXT("PROC: DSPProcessor_Ctrl\r\n")));

	if (hProcessor) {
		if (!DSP_ValidWritePtr(pResourceInfo,
				sizeof(struct DSP_RESOURCEINFO))) {
			if (uResourceInfoSize >=
					sizeof(struct DSP_RESOURCEINFO)) {
				tempStruct.ARGS_PROC_ENUMRESOURCES.hProcessor =
							hProcessor;
				tempStruct.ARGS_PROC_ENUMRESOURCES\
					.uResourceType = uResourceType;
				tempStruct.ARGS_PROC_ENUMRESOURCES\
					.pResourceInfo = pResourceInfo;
				tempStruct.ARGS_PROC_ENUMRESOURCES\
					.uResourceInfoSize = uResourceInfoSize;
				status = DSPTRAP_Trap(&tempStruct,
						CMD_PROC_ENUMRESOURCES_OFFSET);
			} else {
				status = DSP_ESIZE;
				DEBUGMSG(DSPAPI_ZONE_ERROR,
					 (TEXT("PROC: uResourceInfoSize "
							"is small \r\n")));
			}
		} else {
			/* Invalid pointer */
			status = DSP_EPOINTER;
			DEBUGMSG(DSPAPI_ZONE_ERROR,
				(TEXT("PROC: pResourceInfo is invalid \r\n")));
		}
	} else {
		/* Invalid handle */
		status = DSP_EHANDLE;
		DEBUGMSG(DSPAPI_ZONE_ERROR,
			 (TEXT("PROC: Invalid Handle \r\n")));
	}

	return status;
}

/*
 *  ======== DSPProcessor_GetState ========
 *  Purpose:
 *      Report the state of the specified DSP processor.
 */
DBAPI DSPProcessor_GetState(DSP_HPROCESSOR hProcessor,
	      OUT struct DSP_PROCESSORSTATE *pProcStatus, UINT uStateInfoSize)
{
	DSP_STATUS status = DSP_SOK;
	Trapped_Args tempStruct;

	DEBUGMSG(DSPAPI_ZONE_FUNCTION, (TEXT("PROC: DSPProcessor_Ctrl\r\n")));

	/* Check the handle */
	if (hProcessor) {
		if (!DSP_ValidWritePtr(pProcStatus,
			sizeof(struct DSP_PROCESSORSTATE))) {
			if (uStateInfoSize >=
				sizeof(struct DSP_PROCESSORSTATE)) {
				tempStruct.ARGS_PROC_GETSTATE.hProcessor =
							hProcessor;
				tempStruct.ARGS_PROC_GETSTATE.pProcStatus =
							pProcStatus;
				tempStruct.ARGS_PROC_GETSTATE.uStateInfoSize =
							uStateInfoSize;
				status = DSPTRAP_Trap(&tempStruct,
						CMD_PROC_GETSTATE_OFFSET);
			} else {
				status = DSP_ESIZE;
				DEBUGMSG(DSPAPI_ZONE_ERROR,
				(TEXT("PROC: uStateInfoSize is small \r\n")));
			}
		} else {
			status = DSP_EPOINTER;
			DEBUGMSG(DSPAPI_ZONE_ERROR,
				(TEXT("PROC: pProcStatus is invalid \r\n")));
		}
	} else {
		/* Invalid handle */
		status = DSP_EHANDLE;
		DEBUGMSG(DSPAPI_ZONE_ERROR,
			(TEXT("PROC: Invalid Handle \r\n")));
	}

	return status;
}

/*
 *  ======== DSPProcessor_Map ========
 *  Purpose:
 *      Map an MPU buffer to a reserved virtual address
 */
DBAPI DSPProcessor_Map(DSP_HPROCESSOR hProcessor, PVOID pMpuAddr,
		ULONG ulSize, PVOID pReqAddr, PVOID *ppMapAddr, ULONG ulMapAttr)
{
	DSP_STATUS status = DSP_SOK;
	Trapped_Args tempStruct;
#ifdef DEBUG_BRIDGE_PERF
	struct timeval tv_beg;
	struct timeval tv_end;
	struct timezone tz;
	int timeRetVal = 0;

	timeRetVal = getTimeStamp(&tv_beg);
#endif

	DEBUGMSG(DSPAPI_ZONE_FUNCTION, (TEXT("PROC: DSPProcessor_Map\r\n")));

	/* Check the handle */
	if (hProcessor) {
		if (!DSP_ValidWritePtr(ppMapAddr, sizeof(PVOID *))
			&& !DSP_ValidReadPtr(pMpuAddr, sizeof(PVOID))
			&& (pReqAddr != NULL)) {
			if (ulSize > 0) {
#if 0
				INT i;
				ULONG ulLastByte;
				/* Physical memory pages are reserved by the
				 * driver (get_user_pages), so no need to
				 * reserve here. Ensure that physical memory
				 * pages are reserved */
				size_t page_size = getpagesize();
				for (i = 0; i < (INT)ulSize; i += page_size) {
					*(volatile BYTE *)(pMpuAddr + i) =
						*(BYTE *)(pMpuAddr + i);
				}
				/* Non page-aligned size: Write final byte */
				ulLastByte = pMpuAddr + ulSize - 1;
				*(volatile BYTE *)(ulLastByte) =
				    *(BYTE *)(ulLastByte);
#endif
				tempStruct.ARGS_PROC_MAPMEM.hProcessor =
						hProcessor;
				tempStruct.ARGS_PROC_MAPMEM.pMpuAddr = pMpuAddr;
				tempStruct.ARGS_PROC_MAPMEM.ulSize = ulSize;
				tempStruct.ARGS_PROC_MAPMEM.pReqAddr = pReqAddr;
				tempStruct.ARGS_PROC_MAPMEM.ppMapAddr =
						ppMapAddr;
				tempStruct.ARGS_PROC_MAPMEM.ulMapAttr =
						ulMapAttr;
				status = DSPTRAP_Trap(&tempStruct,
						CMD_PROC_MAPMEM_OFFSET);
			} else {
				status = DSP_ESIZE;
				DEBUGMSG(DSPAPI_ZONE_ERROR,
					(TEXT("PROC:size is zero\r\n")));
			}
		} else {
			status = DSP_EPOINTER;
			DEBUGMSG(DSPAPI_ZONE_ERROR, (TEXT
				  ("PROC: Atleast one pointer argument "
					"is invalid\r\n")));
		}
	} else {
		/* Invalid handle */
		status = DSP_EHANDLE;
		DEBUGMSG(DSPAPI_ZONE_ERROR, (TEXT("PROC: Invalid Handle\r\n")));
	}

#ifdef DEBUG_BRIDGE_PERF
	timeRetVal = getTimeStamp(&tv_end);
	PrintStatistics(&tv_beg, &tv_end, "DSPProcessor_Map", ulSize);
#endif


	return status;
}

/*
 *  ======== DSPProcessor_RegisterNotify ========
 *  Purpose:
 *      Register to be notified of specific processor events
 */
DBAPI DSPProcessor_RegisterNotify(DSP_HPROCESSOR hProcessor, UINT uEventMask,
		    UINT uNotifyType, struct DSP_NOTIFICATION *hNotification)
{
	DSP_STATUS status = DSP_SOK;
	Trapped_Args tempStruct;

	DEBUGMSG(DSPAPI_ZONE_FUNCTION,
			(TEXT("PROC: DSPProcessor_RegisterNotify\r\n")));

	/* Check the handle */
	if ((hProcessor) && (hNotification)) {
		if (IsValidProcEvent(uEventMask)) {
			if (IsValidNotifyMask(uNotifyType)) {
				tempStruct.ARGS_PROC_REGISTER_NOTIFY\
						.hProcessor = hProcessor;
				tempStruct.ARGS_PROC_REGISTER_NOTIFY\
						.uEventMask = uEventMask;
				tempStruct.ARGS_PROC_REGISTER_NOTIFY\
						.uNotifyType = uNotifyType;
				tempStruct.ARGS_PROC_REGISTER_NOTIFY\
						.hNotification = hNotification;

				status = DSPTRAP_Trap(&tempStruct,
						CMD_PROC_REGISTERNOTIFY_OFFSET);
			} else {
				status = DSP_ENOTIMPL;
				DEBUGMSG(DSPAPI_ZONE_ERROR,
				(TEXT("PROC: Invalid Notify Mask \r\n")));
			}
		} else {
			status = DSP_EVALUE;
			DEBUGMSG(DSPAPI_ZONE_ERROR,
				(TEXT("PROC: Invalid Evnet Mask \r\n")));
		}
	} else {
		/* Invalid handle */
		status = DSP_EHANDLE;
		DEBUGMSG(DSPAPI_ZONE_ERROR,
			(TEXT("PROC: Invalid Handle \r\n")));
	}

	return status;
}

/*
 *  ======== DSPProcessor_ReserveMemory ========
 *  Purpose:
 *      Reserve a chunk of memory from the DMM
 */
DBAPI DSPProcessor_ReserveMemory(DSP_HPROCESSOR hProcessor, ULONG ulSize,
		PVOID *ppRsvAddr)
{
	DSP_STATUS status = DSP_SOK;
	Trapped_Args tempStruct;
#ifdef DEBUG_BRIDGE_PERF
	struct timeval tv_beg;
	struct timeval tv_end;
	struct timezone tz;
	int timeRetVal = 0;

	timeRetVal = getTimeStamp(&tv_beg);
#endif


	DEBUGMSG(DSPAPI_ZONE_FUNCTION,
			(TEXT("PROC: DSPProcessor_ReserveMemory\r\n")));

	/* Check the handle */
	if (hProcessor) {
		if (!DSP_ValidWritePtr(ppRsvAddr, sizeof(PVOID *))) {
			if (ulSize > 0) {
				if ((ulSize & (PG_SIZE_4K - 1)) == 0) {
					tempStruct.ARGS_PROC_RSVMEM.hProcessor =
							hProcessor;
					tempStruct.ARGS_PROC_RSVMEM.ulSize =
							ulSize;
					tempStruct.ARGS_PROC_RSVMEM.ppRsvAddr =
							ppRsvAddr;
					status = DSPTRAP_Trap(&tempStruct,
							CMD_PROC_RSVMEM_OFFSET);
				} else {
					status = DSP_EINVALIDARG;
					DEBUGMSG(DSPAPI_ZONE_ERROR, (TEXT
						("PROC: size is not 4KB "
							"page-aligned\r\n")));
				}
			} else {
				status = DSP_ESIZE;
				DEBUGMSG(DSPAPI_ZONE_ERROR,
					(TEXT("PROC:size is zero\r\n")));
			}
		} else {
			status = DSP_EPOINTER;
			DEBUGMSG(DSPAPI_ZONE_ERROR,
				(TEXT("PROC:ppRsvAddr is invalid\r\n")));
		}
	} else {
		/* Invalid handle */
		status = DSP_EHANDLE;
		DEBUGMSG(DSPAPI_ZONE_ERROR, (TEXT("PROC: Invalid Handle\r\n")));
	}

#ifdef DEBUG_BRIDGE_PERF
	timeRetVal = getTimeStamp(&tv_end);
	PrintStatistics(&tv_beg, &tv_end, "DSPProcessor_ReserveMemory", ulSize);
#endif

	return status;
}

/*  ======== DSPProcessor_UnMap ========
 *  Purpose:
 *      UnMap an MPU buffer from a reserved virtual address
 */
DBAPI DSPProcessor_UnMap(DSP_HPROCESSOR hProcessor, PVOID pMapAddr)
{
	DSP_STATUS status = DSP_SOK;
	Trapped_Args tempStruct;
#ifdef DEBUG_BRIDGE_PERF
	struct timeval tv_beg;
	struct timeval tv_end;
	struct timezone tz;
	int timeRetVal = 0;

	timeRetVal = getTimeStamp(&tv_beg);
#endif

	DEBUGMSG(DSPAPI_ZONE_FUNCTION, (TEXT("PROC: DSPProcessor_UnMap\r\n")));

	/* Check the handle */
	if (hProcessor) {
		if ((pMapAddr != NULL)) {
			tempStruct.ARGS_PROC_UNMAPMEM.hProcessor = hProcessor;
			tempStruct.ARGS_PROC_UNMAPMEM.pMapAddr = pMapAddr;
			status = DSPTRAP_Trap(&tempStruct,
				CMD_PROC_UNMAPMEM_OFFSET);
		} else {
			status = DSP_EPOINTER;
			DEBUGMSG(DSPAPI_ZONE_ERROR,
				(TEXT("PROC: pMapAddr is invalid\r\n")));
		}
	} else {
		/* Invalid handle */
		status = DSP_EHANDLE;
		DEBUGMSG(DSPAPI_ZONE_ERROR, (TEXT("PROC: Invalid Handle\r\n")));
	}

#ifdef DEBUG_BRIDGE_PERF
	timeRetVal = getTimeStamp(&tv_end);
	PrintStatistics(&tv_beg, &tv_end, "DSPProcessor_UnMap", 0);
#endif

	return status;
}

/*
 *  ======== DSPProcessor_UnReserveMemory ========
 *  Purpose:
 *      Free a chunk of memory from the DMM
 */
DBAPI DSPProcessor_UnReserveMemory(DSP_HPROCESSOR hProcessor, PVOID pRsvAddr)
{
	DSP_STATUS status = DSP_SOK;
	Trapped_Args tempStruct;
#ifdef DEBUG_BRIDGE_PERF
	struct timeval tv_beg;
	struct timeval tv_end;
	struct timezone tz;
	int timeRetVal = 0;

	timeRetVal = getTimeStamp(&tv_beg);
#endif

	DEBUGMSG(DSPAPI_ZONE_FUNCTION,
			(TEXT("PROC: DSPProcessor_UnReserveMemory\r\n")));

	/* Check the handle */
	if (hProcessor) {
		if (pRsvAddr != NULL) {
			tempStruct.ARGS_PROC_UNRSVMEM.hProcessor = hProcessor;
			tempStruct.ARGS_PROC_UNRSVMEM.pRsvAddr = pRsvAddr;
			status = DSPTRAP_Trap(&tempStruct,
					CMD_PROC_UNRSVMEM_OFFSET);
		} else {
			status = DSP_EPOINTER;
			DEBUGMSG(DSPAPI_ZONE_ERROR, (TEXT(
					"PROC: pRsvAddr is invalid\r\n")));
		}
	} else {
		/* Invalid handle */
		status = DSP_EHANDLE;
		DEBUGMSG(DSPAPI_ZONE_ERROR, (TEXT("PROC: Invalid Handle\r\n")));
	}
#ifdef DEBUG_BRIDGE_PERF
	timeRetVal = getTimeStamp(&tv_end);
	PrintStatistics(&tv_beg, &tv_end, "DSPProcessor_UnReserveMemory", 0);
#endif

	return status;
}

