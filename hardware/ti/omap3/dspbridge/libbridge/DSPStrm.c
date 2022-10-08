/*
 * dspbridge/src/api/linux/DSPStrm.c
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
 *  ======== DSPStrm.c ========
 *  Description:
 *      This is the source for the DSP/BIOS Bridge API stream module. The
 *      parameters are validated at the API level, but the bulk of the
 *      work is done at the driver level through the PM STRM module.
 *
 *  Public Functions:
 *      DSPStream_AllocateBuffers
 *      DSPStream_Close
 *      DSPStream_FreeBuffers
 *      DSPStream_GetInfo
 *      DSPStream_Idle
 *      DSPStream_Issue
 *      DSPStream_Open
 *      DSPStream_Reclaim
 *      DSPStream_RegisterNotify
 *      DSPStream_Select
 *
 *! Revision History
 *! ================
 *! 13-Mar-2002 map Checking for invalid direction in DSPStream_Open()
 *! 12-Mar-2002 map Checking for invalid node handle in
 *!                 DSPStream_Open().
 *! 11-Mar-2002 map Checking that bufsize is not smaller than specified
 *!                  number of bytes in buffer in DSPStream_Issue().
 *! 06-Jan-2002 ag  STRMMODE_ZEROCOPY(SM buffer swap) enabled.
 *! 17-Dec-2001 ag  STRMMODE_RDMA(DDMA) enabled.
 *! 04-Dec-2001 ag  Changed user event name string in DSPStream_Open().
 *!                 Added stream direction and index.
 *! 16-Nov-2001 ag  Added SM allocation for streaming.
 *! 07-Jun-2001 sg  Made buffer allocate/free fxn names plural.
 *! 18-May-2001 jeh Close event handle in DSPStream_Open() if failure.
 *! 11-Apr-2001 rr: DSPStream_UnPrepareBuffer checks for pBuffer == NULL
 *!                 (not for *pBuffer).
 *! 13-Dec-2000 jeh Return DSP_EPOINTER, not DSP_EHANDLE in
 *!					DSPStream_Select() for NULL pointers.
 *!					Also set *pMask to 0 if nStreams is 0.
 *! 05-Dec-2000 jeh Return DSP_ESIZE, not DSP_EVALUE in DSPStream_GetInfo,
 *!                 set status to DSP_SOK in DSPStream_UnprepareBuffer().
 *! 10-Nov-2000 rr: DSP_PBUFFER modified to BYTE *. RegisterNotify
 *!                 catches Invalid Events and Masks.
 *! 23-Oct-2000 jeh Free buffers in DSPStream_FreeBuffer().
 *! 28-Sep-2000 jeh Removed DSP_BUFFERATTR param from DSP_StreamAllocateBuffer.
 *! 07-Sep-2000 jeh Changed type HANDLE in DSPStream_RegisterNotify to
 *!                 DSP_HNOTIFICATION.
 *! 04-Aug-2000 rr: Name changed to DSPStrm.c
 *! 27-Jul-2000 rr: Types updated to ver 0.8 API.
 *! 18-Jul-2000 rr: STRM API calls into the Class driver.
 *!                 Only parameters are validated here.
 *! 15-May-2000 gp: Return DSP_EHANDLE fromo DSPStream_Close().
 *! 19-Apr-2000 ww: Updated based on code review.
 *! 12-Apr-2000 ww: Created based on DirectDSP API specification, Version 0.6.
 *
 */

/*  ----------------------------------- Host OS */
#include <host_os.h>

/*  ----------------------------------- DSP/BIOS Bridge */
#include <std.h>
#include <dbdefs.h>
#include <errbase.h>

/*  ----------------------------------- OS Adaptation Layer */
#include <csl.h>

/*  ----------------------------------- Others */
#include <dsptrap.h>
#include <memry.h>

/*  ----------------------------------- This */
#include "_dbdebug.h"

#include <DSPStream.h>

/*  ----------------------------------- Defines, Data Structures, Typedefs */
#define STRM_MAXLOCKPAGES       64

/*  ----------------------------------- Globals */
extern int hMediaFile;		/* class driver handle */

/*  ----------------------------------- Function Prototypes */
static DSP_STATUS GetStrmInfo(DSP_HSTREAM hStream, struct STRM_INFO *pStrmInfo,
			      UINT uStreamInfoSize);

/*
 *  ======== DSPStream_AllocateBuffers ========
 *  Purpose:
 *      Allocate data buffers for use with a specific stream.
 */
DBAPI DSPStream_AllocateBuffers(DSP_HSTREAM hStream, UINT uSize,
			  OUT BYTE **apBuffer, UINT uNumBufs)
{
	UINT i;
	UINT uAllocated = 0;
	DSP_STATUS status = DSP_SOK;
	Trapped_Args tempStruct;
	PVOID pBuf = NULL;
	struct STRM_INFO strmInfo;
	struct DSP_STREAMINFO userInfo;

	DEBUGMSG(DSPAPI_ZONE_FUNCTION,
			(TEXT("NODE: DSPStream_AllocateBuffers:\r\n")));
	if (!hStream) {
		/* Invalid pointer */
		status = DSP_EHANDLE;
		DEBUGMSG(DSPAPI_ZONE_ERROR,
				(TEXT("NODE: DSPStream_AllocateBuffers: "
						"hStrm is Invalid \r\n")));
		return status;
	}
	if (!apBuffer) {
		/* Invalid parameter */
		status = DSP_EPOINTER;
		DEBUGMSG(DSPAPI_ZONE_ERROR,
			(TEXT("NODE: DSPStream_AllocateBuffers: "
				"Invalid pointer in the Input\r\n")));
		return status;
	}
	for (i = 0; i < uNumBufs; i++)
		apBuffer[i] = NULL;

	strmInfo.pUser = &userInfo;
	status = GetStrmInfo(hStream, &strmInfo, sizeof(struct DSP_STREAMINFO));
	if (!DSP_SUCCEEDED(status)) {
		status = DSP_EFAIL;
		DEBUGMSG(DSPAPI_ZONE_ERROR,
			(TEXT("DSPStream_AllocateBuffers: "
				"DSP_FAILED to get strm info\r\n")));
		return status;
	}
	if (strmInfo.uSegment > 0) {
		/* Alloc SM */
		tempStruct.ARGS_STRM_ALLOCATEBUFFER.hStream = hStream;
		tempStruct.ARGS_STRM_ALLOCATEBUFFER.uSize = uSize;
		tempStruct.ARGS_STRM_ALLOCATEBUFFER.apBuffer = apBuffer;
		tempStruct.ARGS_STRM_ALLOCATEBUFFER.uNumBufs = uNumBufs;
		/* Call DSP Trap */
		status = DSPTRAP_Trap(&tempStruct,
				CMD_STRM_ALLOCATEBUFFER_OFFSET);
	} else {
		/* Allocate local buffers */
		for (i = 0; i < uNumBufs; i++) {
			pBuf = MEM_Alloc(uSize, MEM_NONPAGED);
			if (!pBuf) {
				status = DSP_EMEMORY;
				uAllocated = i;
				break;
			} else
				apBuffer[i] = pBuf;

		}
		if (DSP_FAILED(status)) {
			/* Free buffers allocated so far */
			for (i = 0; i < uAllocated; i++) {
				MEM_Free(apBuffer[i]);
				apBuffer[i] = NULL;
			}
		}
	}
	return status;
}

/*
 *  ======== DSPStream_Close ========
 *  Purpose:
 *      Close a stream and free the underlying stream object.
 */
DBAPI DSPStream_Close(DSP_HSTREAM hStream)
{
#ifndef LINUX
	HANDLE hEvent;
#endif
	DSP_STATUS status = DSP_SOK;
	Trapped_Args tempStruct;
	struct STRM_INFO strmInfo;
	struct DSP_STREAMINFO userInfo;
	struct CMM_OBJECT *hCmm = NULL;	/* SM Mgr handle */
	struct CMM_INFO pInfo;	/* CMM info; use for virtual space allocation */

	DEBUGMSG(DSPAPI_ZONE_FUNCTION, (TEXT("NODE: DSPStream_Close:\r\n")));

	if (!hStream) {
		/* Invalid pointer */
		status = DSP_EHANDLE;
		DEBUGMSG(DSPAPI_ZONE_ERROR,
			(TEXT("NODE: DSPStream_Close: hStrm is Invalid \r\n")));
		return status;
	}
	/* Unmap stream's process virtual space, if any */
	strmInfo.pUser = &userInfo;
	status = GetStrmInfo(hStream, &strmInfo, sizeof(struct DSP_STREAMINFO));
	if (!DSP_SUCCEEDED(status)) {
		status = DSP_EFAIL;
		DEBUGMSG(DSPAPI_ZONE_ERROR, (TEXT("NODE: DSPStream_Close: "
					"ERROR in Getting Strm Info \r\n")));
		return status;
	}
	if (strmInfo.pVirtBase != NULL) {
		/* Get segment size.
		 >0 is SM segment. Get default SM Mgr */
		tempStruct.ARGS_CMM_GETHANDLE.hProcessor = NULL;
		tempStruct.ARGS_CMM_GETHANDLE.phCmmMgr = &hCmm;
		status = DSPTRAP_Trap(&tempStruct, CMD_CMM_GETHANDLE_OFFSET);
		if (DSP_SUCCEEDED(status)) {
			/* Get SM segment info from CMM */
			tempStruct.ARGS_CMM_GETINFO.hCmmMgr = hCmm;
			tempStruct.ARGS_CMM_GETINFO.pCmmInfo = &pInfo;
			status = DSPTRAP_Trap(&tempStruct,
					CMD_CMM_GETINFO_OFFSET);
		}
		/* strmInfo.uSegment is probably already OK here,
		   so following checks may not be required */
		if (DSP_SUCCEEDED(status) &&
			(pInfo.ulNumGPPSMSegs >= strmInfo.uSegment)) {
			/* segInfo index starts at 0 */
			if ((pInfo.segInfo[strmInfo.uSegment-1].dwSegBasePa
				!= 0) && (pInfo.segInfo[strmInfo.uSegment-1]\
					.ulTotalSegSize) > 0) {
				if (munmap(strmInfo.pVirtBase,
					pInfo.segInfo[strmInfo.uSegment-1]\
						.ulTotalSegSize)) {
					status = DSP_EFAIL;
				}
			}
		} else
			status = DSP_EBADSEGID;	/*no SM segments */

	}
#ifndef LINUX			/* Events are handled in kernel */
	if (DSP_SUCCEEDED(status)) {
		/* Get the user event from the stream */
		/* Set up the structure */
		tempStruct.ARGS_STRM_GETEVENTHANDLE.hStream = hStream;
		tempStruct.ARGS_STRM_GETEVENTHANDLE.phEvent = &hEvent;
		status = DSPTRAP_Trap(&tempStruct,
				CMD_STRM_GETEVENTHANDLE_OFFSET);
	}
#endif
	if (DSP_SUCCEEDED(status)) {
		/* Now close the stream */
		tempStruct.ARGS_STRM_CLOSE.hStream = hStream;
		status = DSPTRAP_Trap(&tempStruct, CMD_STRM_CLOSE_OFFSET);
	}
#ifndef LINUX			/* Events are handled in kernel */
	if (DSP_SUCCEEDED(status))
		CloseHandle(hEvent);

#endif
	return status;
}

/*
 *  ======== DSPStream_FreeBuffers ========
 *  Purpose:
 *      Free a previously allocated stream data buffer.
 */
DBAPI DSPStream_FreeBuffers(DSP_HSTREAM hStream, IN BYTE **apBuffer,
		UINT uNumBufs)
{
	UINT i;
	DSP_STATUS status = DSP_SOK;
	Trapped_Args tempStruct;
	struct STRM_INFO strmInfo;
	struct DSP_STREAMINFO userInfo;

	DEBUGMSG(DSPAPI_ZONE_FUNCTION,
			(TEXT("NODE:DSPStream_FreeBuffers:\r\n")));

	if (!hStream) {
		/* Invalid pointer */
		status = DSP_EHANDLE;
		DEBUGMSG(DSPAPI_ZONE_ERROR,
			(TEXT("NODE: DSPStream_FreeBuffers: "
						"hStrm is Invalid \r\n")));
		goto func_end;
	}
	if (!apBuffer) {
		/* Invalid parameter */
		status = DSP_EPOINTER;
		DEBUGMSG(DSPAPI_ZONE_ERROR,
				(TEXT("NODE: DSPStream_FreeBuffers: "
					"Invalid pointer in the Input\r\n")));
		goto func_end;
	}
	strmInfo.pUser = &userInfo;	/* need valid user info ptr */
	status = GetStrmInfo(hStream, &strmInfo, sizeof(struct DSP_STREAMINFO));
	if (!DSP_SUCCEEDED(status)) {
		DEBUGMSG(DSPAPI_ZONE_ERROR,
				(TEXT("DSPStream_FreeBuffers. "
						"Free Failed. Bad mode.")));
		status = DSP_EFAIL;
		goto func_end;
	}
	if (strmInfo.uSegment > 0) {
		/* Free SM allocations */
		tempStruct.ARGS_STRM_FREEBUFFER.hStream = hStream;
		tempStruct.ARGS_STRM_FREEBUFFER.apBuffer = apBuffer;
		tempStruct.ARGS_STRM_FREEBUFFER.uNumBufs = uNumBufs;
		/* Call DSP Trap */
		status = DSPTRAP_Trap(&tempStruct, CMD_STRM_FREEBUFFER_OFFSET);
		if (DSP_FAILED(status)) {
			DEBUGMSG(DSPAPI_ZONE_ERROR,
				(TEXT("DSPStream_FreeBuffers: "
						 "Failed to Free Buf")));
			status = DSP_EFAIL;
		}
	} else {
		for (i = 0; i < uNumBufs; i++) {
			/* Free local allocation */
			if (apBuffer[i]) {
				MEM_Free((PVOID)apBuffer[i]);
				apBuffer[i] = NULL;
			}
		}	/* end for */
	}
func_end:
	/* Return DSP_SOK if OS calls returned 0 */
	if (status == 0)
		status = DSP_SOK;

	return status;
}

/*
 *  ======== DSPStream_GetInfo ========
 *  Purpose:
 *      Get information about a stream.
 */
DBAPI DSPStream_GetInfo(DSP_HSTREAM hStream,
		  OUT struct DSP_STREAMINFO *pStreamInfo, UINT uStreamInfoSize)
{
	DSP_STATUS status = DSP_SOK;
	struct STRM_INFO strmInfo;/* include stream's private virt addr info */

	DEBUGMSG(DSPAPI_ZONE_FUNCTION, (TEXT("NODE: DSPStream_GetInfo:\r\n")));

	strmInfo.pUser = pStreamInfo;
	status = GetStrmInfo(hStream, &strmInfo, uStreamInfoSize);
	/* Return DSP_SOK if OS calls returned 0 */
	if (status == 0)
		status = DSP_SOK;

	return status;
}

/*
 *  ======== DSPStream_Idle ========
 *  Purpose:
 *      Terminate I/O with a particular stream, and (optionally)
 *      flush output data buffers.
 */
DBAPI DSPStream_Idle(DSP_HSTREAM hStream, bool bFlush)
{
	DSP_STATUS status = DSP_SOK;
	Trapped_Args tempStruct;

	DEBUGMSG(DSPAPI_ZONE_FUNCTION, (TEXT("NODE: DSPStream_Idle:\r\n")));

	if (hStream) {
		/* Set up the structure */
		/* Call DSP Trap */
		tempStruct.ARGS_STRM_IDLE.hStream = hStream;
		tempStruct.ARGS_STRM_IDLE.bFlush = bFlush;
		status = DSPTRAP_Trap(&tempStruct, CMD_STRM_IDLE_OFFSET);
	} else {
		/* Invalid pointer */
		status = DSP_EHANDLE;
		DEBUGMSG(DSPAPI_ZONE_ERROR, (TEXT("NODE: DSPStream_Idle: "
						"hStrm is Invalid \r\n")));
	}
	return status;
}

/*
 *  ======== DSPStream_Issue ========
 *  Purpose:
 *      Send a buffer of data to a stream.
 */
DBAPI DSPStream_Issue(DSP_HSTREAM hStream, IN BYTE *pBuffer,
		ULONG dwDataSize, ULONG dwBufSize, IN DWORD dwArg)
{
	DSP_STATUS status = DSP_SOK;
	Trapped_Args tempStruct;

	DEBUGMSG(DSPAPI_ZONE_FUNCTION, (TEXT("NODE: DSPStream_Issue:\r\n")));

	if (hStream) {
		/* Check the size of the buffer */
		if (pBuffer) {
			/* Check that the size isn't too small */
			if (dwDataSize > dwBufSize) {
				status = DSP_EINVALIDARG;
				DEBUGMSG(DSPAPI_ZONE_ERROR,
					(TEXT("NODE: DSPStream_Issue: "
					"Invalid argument in the Input\r\n")));
			} else {
				/* Set up the structure */
				tempStruct.ARGS_STRM_ISSUE.hStream = hStream;
				tempStruct.ARGS_STRM_ISSUE.pBuffer = pBuffer;
				tempStruct.ARGS_STRM_ISSUE.dwBytes = dwDataSize;
				tempStruct.ARGS_STRM_ISSUE.dwBufSize =
							dwBufSize;
				tempStruct.ARGS_STRM_ISSUE.dwArg = dwArg;
				/* Call DSP Trap */
				status = DSPTRAP_Trap(&tempStruct,
					CMD_STRM_ISSUE_OFFSET);
				/* Return DSP_SOK if OS calls returned 0 */
				if (status == 0)
					status = DSP_SOK;

			}
		} else {
			/* Invalid parameter */
			status = DSP_EPOINTER;
			DEBUGMSG(DSPAPI_ZONE_ERROR,
				(TEXT("NODE: DSPStream_Issue: "
					"Invalid pointer in the Input\r\n")));
		}
	} else {
		/* Invalid pointer */
		status = DSP_EHANDLE;
		DEBUGMSG(DSPAPI_ZONE_ERROR, (TEXT("NODE: DSPStream_Issue: "
						"hStrm is Invalid \r\n")));
	}

	return status;
}

/*
 *  ======== DSPStream_Open ========
 *  Purpose:
 *      Retrieve a stream handle for sending/receiving data buffers
 *      to/from a task node on a DSP.
 */
DBAPI DSPStream_Open(DSP_HNODE hNode, UINT uDirection, UINT uIndex,
	       IN OPTIONAL struct DSP_STREAMATTRIN *pAttrIn,
	       OUT DSP_HSTREAM *phStream)
{
	DSP_STATUS status = DSP_SOK;
	Trapped_Args tempStruct;
	struct STRM_ATTR strmAttrs;
#ifndef LINUX			/* Events are handled in kernel */
	CHAR szEventName[STRM_MAXEVTNAMELEN];
	WCHAR wszEventName[STRM_MAXEVTNAMELEN];
	CHAR szTemp[STRM_MAXEVTNAMELEN];
#endif
	struct CMM_OBJECT *hCmm = NULL;	/* SM Mgr handle */
	struct CMM_INFO pInfo;/* CMM info; use for virtual space allocation */

	DEBUGMSG(DSPAPI_ZONE_FUNCTION, (TEXT("NODE: DSPStream_Open:\r\n")));

	if (!hNode) {
		status = DSP_EHANDLE;
		DEBUGMSG(DSPAPI_ZONE_ERROR, (TEXT("NODE: DSPStream_Open: "
					"Invalid handle in the Input\r\n")));
		return status;
	}
	if (uDirection != DSP_TONODE && uDirection != DSP_FROMNODE) {
		status = DSP_EDIRECTION;
		DEBUGMSG(DSPAPI_ZONE_ERROR, (TEXT("NODE: DSPStream_Open: "
					"Invalid direction in the Input\r\n")));
		return status;
	}
	if (!phStream) {
		status = DSP_EPOINTER;
		DEBUGMSG(DSPAPI_ZONE_ERROR,
			(TEXT("NODE: DSPStream_Open: "
				"Invalid pointer in the Input\r\n")));
		return status;
	}
	*phStream = NULL;
	strmAttrs.hUserEvent = NULL;
#ifndef LINUX			/* Events are handled in kernel */
			 /* Create a 'named' user event that is unique.*/
	strmAttrs.pStreamAttrIn = pAttrIn;
	szEventName[0] = 'E';
	szEventName[1] = 'V';
	szEventName[2] = '\0';
	/* append hNode handle string */
	strncat(szEventName, _ultoa((ULONG)hNode, szTemp, 16), 8);
	/* now append stream index and direction */
	strncat(szEventName, _ultoa((ULONG)uDirection, szTemp, 16), 2);
	strmAttrs.pstrEventName =
		strncat(szEventName, _ultoa((ULONG)uIndex, szTemp, 16), 3);
	(Void)CSL_AnsiToWchar(wszEventName, szEventName, STRM_MAXEVTNAMELEN);
	/* Create an auto reset event. */
	strmAttrs.hUserEvent = CreateEvent(NULL,false,false,wszEventName);
	if (!strmAttrs.hUserEvent) {
		status = DSP_EFAIL;
		DEBUGMSG(DSPAPI_ZONE_ERROR, (TEXT("NODE: DSPStream_Open: "
					"Failed to Create the Event \r\n")));
	}
#endif
	 /*  Default stream mode is PROCCOPY.
	 *  Check for currently supported mode(s).*/
	if (pAttrIn) {
		if (pAttrIn->lMode == STRMMODE_LDMA) {
			/* No System-DMA support */
			status = DSP_ENOTIMPL;
		} else
		    if ((pAttrIn->lMode != STRMMODE_PROCCOPY)
				&& (pAttrIn->lMode != STRMMODE_ZEROCOPY)
				&& (pAttrIn->lMode != STRMMODE_RDMA)) {
			status = DSP_ESTRMMODE;	/* illegal stream mode */
		}
		pAttrIn->uSegment = abs(pAttrIn->uSegment);
		/* make non-neg */
	}
	 /*  If opening the stream for STRMMODE_ZEROCOPY or
	 *  STRMMODE_RDMA(DSP-DMA) stream mode, then setup the
	 *  stream's  CMM translator for the specified SM segment.*/
	strmAttrs.pVirtBase = NULL;
	strmAttrs.ulVirtSize = 0;
	if (DSP_SUCCEEDED(status) && pAttrIn) {
		if ((pAttrIn->lMode == STRMMODE_ZEROCOPY) ||
				(pAttrIn->lMode == STRMMODE_RDMA)) {
			if (pAttrIn->uSegment == 0) {
				status = DSP_ENOTSHAREDMEM;	/* must be
								SM segment */
				goto loop_end;
			}
			/* >0 is SM segment. Get default SM Mgr */
			tempStruct.ARGS_CMM_GETHANDLE.hProcessor = NULL;
			tempStruct.ARGS_CMM_GETHANDLE.phCmmMgr = &hCmm;
			status = DSPTRAP_Trap(&tempStruct,
					CMD_CMM_GETHANDLE_OFFSET);
			if (status == DSP_SOK) {
				/* Get SM segment info from CMM */
				tempStruct.ARGS_CMM_GETINFO.hCmmMgr = hCmm;
				tempStruct.ARGS_CMM_GETINFO.pCmmInfo = &pInfo;
				status = DSPTRAP_Trap(&tempStruct,
						CMD_CMM_GETINFO_OFFSET);
				if (status != DSP_SOK)
					status = DSP_EFAIL;
			} else
				status = DSP_EFAIL;

			if (!DSP_SUCCEEDED(status ||
				!(pInfo.ulNumGPPSMSegs >= pAttrIn->uSegment))) {
				status = DSP_EBADSEGID; /* no SM segments */
				goto loop_end;
			}
			/* segInfo index starts at 0 */
			if ((pInfo.segInfo[pAttrIn->uSegment-1].dwSegBasePa
				== 0) || (pInfo.segInfo[pAttrIn->uSegment-1]\
					.ulTotalSegSize) < 0) {
				status = DSP_EFAIL;
				DEBUGMSG(DSPAPI_ZONE_ERROR,
						(TEXT("STRM:DSPStream_Open: "
						"Bad SM info...why?\r\n")));
				goto loop_end;
			}
			strmAttrs.pVirtBase = mmap(NULL,
				pInfo.segInfo[pAttrIn->uSegment-1]\
				.ulTotalSegSize, PROT_READ | PROT_WRITE,
				MAP_SHARED | MAP_LOCKED, hMediaFile, pInfo\
				.segInfo[pAttrIn->uSegment-1].dwSegBasePa);
			if (strmAttrs.pVirtBase == NULL) {
				status = DSP_EFAIL;
				DEBUGMSG(DSPAPI_ZONE_ERROR,
					(TEXT("STRM: DSPStream_Open: "
						"Virt alloc failed\r\n")));
				goto loop_end;
			}
			strmAttrs.ulVirtSize =
			pInfo.segInfo[pAttrIn->uSegment-1].ulTotalSegSize;
		}
	}
loop_end:
	if (DSP_SUCCEEDED(status)) {
		/* Set up the structure */
		strmAttrs.pStreamAttrIn = pAttrIn;
		/* Call DSP Trap */
		tempStruct.ARGS_STRM_OPEN.hNode = hNode;
		tempStruct.ARGS_STRM_OPEN.uDirection = uDirection;
		tempStruct.ARGS_STRM_OPEN.uIndex = uIndex;
		tempStruct.ARGS_STRM_OPEN.pAttrIn = &strmAttrs;
		tempStruct.ARGS_STRM_OPEN.phStream = phStream;
		status = DSPTRAP_Trap(&tempStruct, CMD_STRM_OPEN_OFFSET);
#ifndef LINUX			/* Events are handled in kernel */
		if (DSP_FAILED(status))
			CloseHandle(strmAttrs.hUserEvent);

#endif
	}
	return status;
}

/*
 *  ======== DSPStream_PrepareBuffer ========
 *  Purpose:
 *      Prepares a buffer.
 */
DBAPI DSPStream_PrepareBuffer(DSP_HSTREAM hStream, UINT uSize, BYTE *pBuffer)
{
	DSP_STATUS status = DSP_SOK;
#ifndef LINUX
	/*  Pages are never swapped out (i.e. always locked in Linux) */
	ULONG aPageTab[STRM_MAXLOCKPAGES];
	/* Find the maximum # of pages that could be locked. x86 &
			ARM=4Kb pages */
	UINT cPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(NULL, uSize);
#endif
	/* Do error checking here to API spec. We don't call down to WCD */
	if (!hStream)
		status = DSP_EHANDLE;

	if (DSP_SUCCEEDED(status)) {
		if (!pBuffer)
			status = DSP_EPOINTER;
	}

	if (DSP_SUCCEEDED(status)) {
		if (uSize <= 0)
			status = DSP_ESIZE;
	}
#ifndef LINUX
	/*  Pages are never swapped out (i.e. always locked in Linux) */
	if (DSP_SUCCEEDED(status)) {
		if (cPages > STRM_MAXLOCKPAGES)
			status = DSP_EFAIL;
		else {
			if (!LockPages((LPVOID)pBuffer, uSize, aPageTab,
					LOCKFLAG_WRITE))
				status = DSP_EFAIL;
		}
	}
#endif

	return status;
}

/*
 *  ======== DSPStream_Reclaim ========
 *  Purpose:
 *      Request a buffer back from a stream.
 */
DBAPI DSPStream_Reclaim(DSP_HSTREAM hStream, OUT BYTE **pBufPtr,
		OUT ULONG *pDataSize, OUT ULONG *pBufSize, OUT DWORD *pdwArg)
{
	DSP_STATUS status = DSP_SOK;
	Trapped_Args tempStruct;

	DEBUGMSG(DSPAPI_ZONE_FUNCTION, (TEXT("NODE: DSPStream_Reclaim:\r\n")));

	if (hStream) {
		/* Check the size of the buffer */
		if ((pBufPtr) && (pDataSize) && (pdwArg)) {
			/* Set up the structure */
			/* Call DSP Trap */
			tempStruct.ARGS_STRM_RECLAIM.hStream = hStream;
			tempStruct.ARGS_STRM_RECLAIM.pBufPtr = pBufPtr;
			tempStruct.ARGS_STRM_RECLAIM.pBytes = pDataSize;
			tempStruct.ARGS_STRM_RECLAIM.pBufSize = pBufSize;
			tempStruct.ARGS_STRM_RECLAIM.pdwArg = pdwArg;
			status = DSPTRAP_Trap(&tempStruct,
					CMD_STRM_RECLAIM_OFFSET);
		} else {
			/* Invalid parameter */
			status = DSP_EPOINTER;
			DEBUGMSG(DSPAPI_ZONE_ERROR,
				(TEXT("NODE: DSPStream_Reclaim: "
					"Invalid pointer in the Input\r\n")));
		}
	} else {
		/* Invalid pointer */
		status = DSP_EHANDLE;
		DEBUGMSG(DSPAPI_ZONE_ERROR, (TEXT("NODE: DSPStream_Reclaim: "
						"hStrm is Invalid \r\n")));
	}

	return status;
}

/*
 *  ======== DSPStream_RegisterNotify ========
 *  Purpose:
 *      Register to be notified of specific events for this stream.
 */
DBAPI
DSPStream_RegisterNotify(DSP_HSTREAM hStream, UINT uEventMask,
		 UINT uNotifyType, struct DSP_NOTIFICATION *hNotification)
{
	DSP_STATUS status = DSP_SOK;
	Trapped_Args tempStruct;

	DEBUGMSG(DSPAPI_ZONE_FUNCTION,
			(TEXT("NODE: DSPStream_RegisterNotify:\r\n")));

	if ((hStream) && (hNotification)) {
		if (IsValidStrmEvent(uEventMask)) {
			if (IsValidNotifyMask(uNotifyType)) {
				/* Set up the structure */
				/* Call DSP Trap */
				tempStruct.ARGS_STRM_REGISTERNOTIFY.hStream =
						hStream;
				tempStruct.ARGS_STRM_REGISTERNOTIFY.uEventMask =
						uEventMask;
				tempStruct.ARGS_STRM_REGISTERNOTIFY\
						.uNotifyType = uNotifyType;
				tempStruct.ARGS_STRM_REGISTERNOTIFY\
						.hNotification = hNotification;
				status = DSPTRAP_Trap(&tempStruct,
						CMD_STRM_REGISTERNOTIFY_OFFSET);
			} else {
				status = DSP_ENOTIMPL;
				DEBUGMSG(DSPAPI_ZONE_ERROR,
					(TEXT("NODE: DSPStream_RegisterNotify: "
						"Invalid Notify Mask \r\n")));
			}
		} else {
			status = DSP_EVALUE;
			DEBUGMSG(DSPAPI_ZONE_ERROR,
					(TEXT("NODE: DSPStream_RegisterNotify: "
						"Invalid Event Mask \r\n")));
		}
	} else {
		/* Invalid handle */
		status = DSP_EHANDLE;
		DEBUGMSG(DSPAPI_ZONE_ERROR,
			(TEXT("NODE: DSPStream_RegisterNotify: "
					"Invalid Handle \r\n")));
	}

	return status;
}

/*
 *  ======== DSPStream_Select ========
 *  Purpose:
 *      Select a ready stream.
 */
DBAPI DSPStream_Select(IN DSP_HSTREAM *aStreamTab,
		 UINT nStreams, OUT UINT *pMask, UINT uTimeout)
{
	DSP_STATUS status = DSP_SOK;
	Trapped_Args tempStruct;

	DEBUGMSG(DSPAPI_ZONE_FUNCTION, (TEXT("NODE: DSPStream_Select:\r\n")));

	if ((aStreamTab) && (pMask)) {
		if (nStreams) {
			/* Set up the structure */
			/* Call DSP Trap */
			tempStruct.ARGS_STRM_SELECT.aStreamTab = aStreamTab;
			tempStruct.ARGS_STRM_SELECT.nStreams = nStreams;
			tempStruct.ARGS_STRM_SELECT.pMask = pMask;
			tempStruct.ARGS_STRM_SELECT.uTimeout = uTimeout;
			status = DSPTRAP_Trap(&tempStruct,
					CMD_STRM_SELECT_OFFSET);
		} else
			/* nStreams == 0 */
			*pMask = 0;
	} else {
		/* Invalid pointer */
		status = DSP_EPOINTER;
		DEBUGMSG(DSPAPI_ZONE_ERROR,
		 (TEXT("NODE: DSPStream_Select: hStrm is Invalid \r\n")));
	}

	return status;
}

/*
 *  ======== DSPStream_UnprepareBuffer ========
 *  Purpose:
 *      Unprepares a buffer.
 */
DBAPI DSPStream_UnprepareBuffer(DSP_HSTREAM hStream, UINT uSize,
				BYTE *pBuffer)
{
	DSP_STATUS status = DSP_SOK;

	/* Do error checking here to API spec. We don't call down to WCD */
	if (!hStream)
		status = DSP_EHANDLE;

	if (DSP_SUCCEEDED(status)) {
		if (!pBuffer)
			status = DSP_EPOINTER;
	}

	if (DSP_SUCCEEDED(status)) {
		/*|| ((LPVOID)pBuffer == NULL) - already checked above */
		if ((uSize <= 0))
			status = DSP_EFAIL;
	}
#ifndef LINUX			/*  Pages are never swapped out
						(i.e. always locked in Linux) */
	if (DSP_SUCCEEDED(status)) {
		if (!UnlockPages((LPVOID) pBuffer, uSize))
			status = DSP_EFAIL;
	}
#endif

	return status;
}

/*
 *  ======== GetStrmInfo ========
 */
static DSP_STATUS GetStrmInfo(DSP_HSTREAM hStream, struct STRM_INFO *pStrmInfo,
							UINT uStreamInfoSize)
{
	DSP_STATUS status = DSP_SOK;
	Trapped_Args tempStruct;

	if (hStream) {
		/* Check the size of the buffer */
		if (pStrmInfo && pStrmInfo->pUser) {
			if (uStreamInfoSize >= sizeof(struct DSP_STREAMINFO)) {
				/* user info */
				/* Set up the structure */
				/* Call DSP Trap */
				tempStruct.ARGS_STRM_GETINFO.hStream = hStream;
				tempStruct.ARGS_STRM_GETINFO.pStreamInfo =
						pStrmInfo;
				/* user returned struct DSP_STREAMINFO
						info size */
				tempStruct.ARGS_STRM_GETINFO.uStreamInfoSize =
						uStreamInfoSize;
				status = DSPTRAP_Trap(&tempStruct,
						CMD_STRM_GETINFO_OFFSET);
			} else {
				status = DSP_ESIZE;
				DEBUGMSG(DSPAPI_ZONE_ERROR,
					 (TEXT("NODE: DSPStream_GetInfo: "
					 "uStreamInfo size is less than the "
					 "size of struct DSP_STREAMINFO\r\n")));
			}
		} else {
			/* Invalid parameter */
			status = DSP_EPOINTER;
			DEBUGMSG(DSPAPI_ZONE_ERROR,
				 (TEXT("NODE: DSPStream_GetInfo: "
					"Invalid pointer\r\n")));
		}
	} else {
		/* Invalid pointer */
		status = DSP_EHANDLE;
		DEBUGMSG(DSPAPI_ZONE_ERROR, (TEXT(
			"NODE: DSPStream_GetInfo: hStrm is Invalid \r\n")));
	}

	return status;
}

