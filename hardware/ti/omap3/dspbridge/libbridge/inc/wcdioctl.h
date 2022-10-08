/*
 * dspbridge/mpu_api/inc/wcdioctl.h
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
 *  ======== wcdioctl.h ========
 *  Purpose:
 *      Contains structures and commands that are used for interaction
 *      between the DDSP API and class driver.
 *
 *! Revision History
 *! ================
 *! 19-Apr-2004 sb  Aligned DMM definitions with Symbian
 *! 08-Mar-2004 sb  Added the Dynamic Memory Mapping structs & offsets
 *! 15-Oct-2002 kc  Updated definitions for private PERF module.
 *! 16-Aug-2002 map Added ARGS_MGR_REGISTEROBJECT & ARGS_MGR_UNREGISTEROBJECT
 *!                 Added CMD_MGR_REGISTEROBJECT_OFFSET &
 *!                 CMD_MGR_UNREGISTEROBJECT_OFFSET
 *! 15-Jan-2002 ag  Added actaul bufSize to ARGS_STRM_[RECLAIM][ISSUE].
 *! 15-Nov-2001 ag  change to STRMINFO in ARGS_STRM_GETINFO.
 *! 11-Sep-2001 ag  ARGS_CMM_GETHANDLE defn uses DSP_HPROCESSOR.
 *! 23-Apr-2001 jeh Added pStatus to NODE_TERMINATE args.
 *! 13-Feb-2001 kc  DSP/BIOS Bridge name updates.
 *! 22-Nov-2000 kc: Added CMD_MGR_GETPERF_DATA_OFFSET for acquiring PERF stats.
 *! 27-Oct-2000 jeh Added timeouts to NODE_GETMESSAGE, NODE_PUTMESSAGE args.
 *!                 Removed NODE_GETMESSAGESTRM args.
 *! 11-Oct-2000 ag: Added SM mgr(CMM) args.
 *! 27-Sep-2000 jeh Removed struct DSP_BUFFERATTR param from ARGS_STRM_ALLOCATEBUFFER.
 *! 25-Sep-2000 rr: Updated to Version 0.9
 *! 07-Sep-2000 jeh Changed HANDLE to DSP_HNOTIFICATION in RegisterNotify args.
 *!                 Added DSP_STRMATTR to DSPNode_Connect args.
 *! 04-Aug-2000 rr: MEM and UTIL added to RM.
 *! 27-Jul-2000 rr: NODE, MGR,STRM and PROC added
 *! 27-Jun-2000 rr: Modifed to Use either PM or DSP/BIOS Bridge
 *!                 IFDEF to build for PM or DSP/BIOS Bridge
 *! 28-Jan-2000 rr: NT_CMD_FROM_OFFSET moved out to dsptrap.h
 *! 24-Jan-2000 rr: Merged with Scott's code.
 *! 21-Jan-2000 sg: In ARGS_CHNL_GETMODE changed mode to be ULONG to be
 *!                 consistent with chnldefs.h.
 *! 11-Jan-2000 rr: CMD_CFG_GETCDVERSION_OFFSET added.
 *! 12-Nov-1999 rr: CMD_BRD_MONITOR_OFFSET added
 *! 09-Nov-1999 kc: Added MEMRY and enabled CMD_BRD_IOCTL_OFFSET.
 *! 05-Nov-1999 ag: Added CHNL.
 *! 02-Nov-1999 kc: Removed field from ARGS_UTIL_TESTDLL.
 *! 29-Oct-1999 kc: Cleaned up for code review.
 *! 08-Oct-1999 rr: Util control offsets added.
 *! 13-Sep-1999 kc: Added ARGS_UTIL_TESTDLL for PM test infrastructure.
 *! 19-Aug-1999 rr: Created from WSX. Minimal Implementaion of BRD_Start and BRD
 *!                 and BRD_Stop. IOCTL Offsets and CTRL Code.
 */

#ifndef WCDIOCTL_
#define WCDIOCTL_

#include <mem.h>
#include <cmm.h>
#include <strmdefs.h>
#include <dbdcd.h>

typedef union {

	/* MGR Module */
	struct {
		UINT uNode;
		struct DSP_NDBPROPS *pNDBProps;
		UINT uNDBPropsSize;
		UINT *puNumNodes;
	} ARGS_MGR_ENUMNODE_INFO;

	struct {
		UINT uProcessor;
		struct DSP_PROCESSORINFO *pProcessorInfo;
		UINT uProcessorInfoSize;
		UINT *puNumProcs;
	} ARGS_MGR_ENUMPROC_INFO;

	struct {
		struct DSP_UUID *pUuid;
		DSP_DCDOBJTYPE objType;
		CHAR *pszPathName;
	} ARGS_MGR_REGISTEROBJECT;

	struct {
		struct DSP_UUID *pUuid;
		DSP_DCDOBJTYPE objType;
	} ARGS_MGR_UNREGISTEROBJECT;

	struct {
		struct DSP_NOTIFICATION* *aNotifications;
		UINT uCount;
		UINT *puIndex;
		UINT uTimeout;
	} ARGS_MGR_WAIT;

	/* PROC Module */
	struct {
		UINT uProcessor;
		struct DSP_PROCESSORATTRIN *pAttrIn;
		DSP_HPROCESSOR *phProcessor;
	} ARGS_PROC_ATTACH;

	struct {
		DSP_HPROCESSOR hProcessor;
		ULONG dwCmd;
		struct DSP_CBDATA *pArgs;
	} ARGS_PROC_CTRL;

	struct {
		DSP_HPROCESSOR hProcessor;
	} ARGS_PROC_DETACH;

	struct {
		DSP_HPROCESSOR hProcessor;
		DSP_HNODE *aNodeTab;
		UINT uNodeTabSize;
		UINT *puNumNodes;
		UINT *puAllocated;
	} ARGS_PROC_ENUMNODE_INFO;

	struct {
		DSP_HPROCESSOR hProcessor;
		UINT uResourceType;
		struct DSP_RESOURCEINFO *pResourceInfo;
		UINT uResourceInfoSize;
	} ARGS_PROC_ENUMRESOURCES;

	struct {
		DSP_HPROCESSOR hProcessor;
		struct DSP_PROCESSORSTATE *pProcStatus;
		UINT uStateInfoSize;
	} ARGS_PROC_GETSTATE;

	struct {
		DSP_HPROCESSOR hProcessor;
		BYTE *pBuf;

	#ifndef RES_CLEANUP_DISABLE
	    BYTE *  pSize;
    #endif
		UINT uMaxSize;
	} ARGS_PROC_GETTRACE;

	struct {
		DSP_HPROCESSOR hProcessor;
		INT iArgc;
		CHAR **aArgv;
		CHAR **aEnvp;
	} ARGS_PROC_LOAD;

	struct {
		DSP_HPROCESSOR hProcessor;
		UINT uEventMask;
		UINT uNotifyType;
		struct DSP_NOTIFICATION* hNotification;
	} ARGS_PROC_REGISTER_NOTIFY;

	struct {
		DSP_HPROCESSOR hProcessor;
	} ARGS_PROC_START;

	struct {
		DSP_HPROCESSOR hProcessor;
		ULONG ulSize;
		PVOID *ppRsvAddr;
	} ARGS_PROC_RSVMEM;

	struct {
		DSP_HPROCESSOR hProcessor;
		ULONG ulSize;
		PVOID pRsvAddr;
	} ARGS_PROC_UNRSVMEM;

	struct {
		DSP_HPROCESSOR hProcessor;
		PVOID pMpuAddr;
		ULONG ulSize;
		PVOID pReqAddr;
		PVOID *ppMapAddr;
		ULONG ulMapAttr;
	} ARGS_PROC_MAPMEM;

	struct {
		DSP_HPROCESSOR hProcessor;
		ULONG ulSize;
		PVOID pMapAddr;
	} ARGS_PROC_UNMAPMEM;

	struct {
		DSP_HPROCESSOR hProcessor;
		PVOID pMpuAddr;
		ULONG ulSize;
		ULONG ulFlags;
	} ARGS_PROC_FLUSHMEMORY;

	struct {
		DSP_HPROCESSOR hProcessor;
	} ARGS_PROC_STOP;

	struct {
                DSP_HPROCESSOR hProcessor;
                PVOID pMpuAddr;
                ULONG ulSize;
        } ARGS_PROC_INVALIDATEMEMORY;


	/* NODE Module */
	struct {
		DSP_HPROCESSOR hProcessor;
		struct DSP_UUID *pNodeID;
		struct DSP_CBDATA *pArgs;
		struct DSP_NODEATTRIN *pAttrIn;
		DSP_HNODE *phNode;
	} ARGS_NODE_ALLOCATE;

	struct {
		DSP_HNODE hNode;
		UINT uSize;
		struct DSP_BUFFERATTR *pAttr;
		BYTE **pBuffer;
	} ARGS_NODE_ALLOCMSGBUF;

	struct {
		DSP_HNODE hNode;
		INT iPriority;
	} ARGS_NODE_CHANGEPRIORITY;

	struct {
		DSP_HNODE hNode;
		UINT uStream;
		DSP_HNODE hOtherNode;
		UINT uOtherStream;
		struct DSP_STRMATTR *pAttrs;
		struct DSP_CBDATA *pConnParam;
	} ARGS_NODE_CONNECT;

	struct {
		DSP_HNODE hNode;
	} ARGS_NODE_CREATE;

	struct {
		DSP_HNODE hNode;
	} ARGS_NODE_DELETE;

	struct {
		DSP_HNODE hNode;
		struct DSP_BUFFERATTR *pAttr;
		BYTE *pBuffer;
	} ARGS_NODE_FREEMSGBUF;

	struct {
		DSP_HNODE hNode;
		struct DSP_NODEATTR *pAttr;
		UINT uAttrSize;
	} ARGS_NODE_GETATTR;

	struct {
		DSP_HNODE hNode;
		struct DSP_MSG *pMessage;
		UINT uTimeout;
	} ARGS_NODE_GETMESSAGE;

	struct {
		DSP_HNODE hNode;
	} ARGS_NODE_PAUSE;

	struct {
		DSP_HNODE hNode;
		struct DSP_MSG *pMessage;
		UINT uTimeout;
	} ARGS_NODE_PUTMESSAGE;

	struct {
		DSP_HNODE hNode;
		UINT uEventMask;
		UINT uNotifyType;
		struct DSP_NOTIFICATION* hNotification;
	} ARGS_NODE_REGISTERNOTIFY;

	struct {
		DSP_HNODE hNode;
	} ARGS_NODE_RUN;

	struct {
		DSP_HNODE hNode;
		DSP_STATUS *pStatus;
	} ARGS_NODE_TERMINATE;

	struct {
		DSP_HPROCESSOR hProcessor;
		struct DSP_UUID *pNodeID;
		struct DSP_NDBPROPS *pNodeProps;
	} ARGS_NODE_GETUUIDPROPS;

        /* STRM module */

	struct {
		DSP_HSTREAM hStream;
		UINT uSize;
		BYTE **apBuffer;
		UINT uNumBufs;
	} ARGS_STRM_ALLOCATEBUFFER;

	struct {
		DSP_HSTREAM hStream;
	} ARGS_STRM_CLOSE;

	struct {
		DSP_HSTREAM hStream;
		BYTE **apBuffer;
		UINT uNumBufs;
	} ARGS_STRM_FREEBUFFER;

	struct {
		DSP_HSTREAM hStream;
		HANDLE *phEvent;
	} ARGS_STRM_GETEVENTHANDLE;

	struct {
		DSP_HSTREAM hStream;
		struct STRM_INFO *pStreamInfo;
		UINT uStreamInfoSize;
	} ARGS_STRM_GETINFO;

	struct {
		DSP_HSTREAM hStream;
		bool bFlush;
	} ARGS_STRM_IDLE;

	struct {
		DSP_HSTREAM hStream;
		BYTE *pBuffer;
		ULONG dwBytes;
		ULONG dwBufSize;
		DWORD dwArg;
	} ARGS_STRM_ISSUE;

	struct {
		DSP_HNODE hNode;
		UINT uDirection;
		UINT uIndex;
		struct STRM_ATTR *pAttrIn;
		DSP_HSTREAM *phStream;
	} ARGS_STRM_OPEN;

	struct {
		DSP_HSTREAM hStream;
		BYTE **pBufPtr;
		ULONG *pBytes;
		ULONG *pBufSize;
		DWORD *pdwArg;
	} ARGS_STRM_RECLAIM;

	struct {
		DSP_HSTREAM hStream;
		UINT uEventMask;
		UINT uNotifyType;
		struct DSP_NOTIFICATION* hNotification;
	} ARGS_STRM_REGISTERNOTIFY;

	struct {
		DSP_HSTREAM *aStreamTab;
		UINT nStreams;
		UINT *pMask;
		UINT uTimeout;
	} ARGS_STRM_SELECT;

	/* CMM Module */
	struct {
		struct CMM_OBJECT* hCmmMgr;
		UINT uSize;
		struct CMM_ATTRS *pAttrs;
		OUT PVOID *ppBufVA;
	} ARGS_CMM_ALLOCBUF;

	struct {
		struct CMM_OBJECT* hCmmMgr;
		PVOID pBufPA;
		ULONG ulSegId;
	} ARGS_CMM_FREEBUF;

	struct {
		DSP_HPROCESSOR hProcessor;
		struct CMM_OBJECT* *phCmmMgr;
	} ARGS_CMM_GETHANDLE;

	struct {
		struct CMM_OBJECT* hCmmMgr;
		struct CMM_INFO *pCmmInfo;
	} ARGS_CMM_GETINFO;

	/* MEM Module */
	struct {
		ULONG cBytes;
		MEM_POOLATTRS type;
		PVOID pMem;
	} ARGS_MEM_ALLOC;

	struct {
		ULONG cBytes;
		MEM_POOLATTRS type;
		PVOID pMem;
	} ARGS_MEM_CALLOC;

	struct {
		PVOID pMem;
	} ARGS_MEM_FREE;

	struct {
		PVOID pBuffer;
		ULONG cSize;
		PVOID pLockedBuffer;
	} ARGS_MEM_PAGELOCK;

	struct {
		PVOID pBuffer;
		ULONG cSize;
	} ARGS_MEM_PAGEUNLOCK;

	/* UTIL module */
	struct {
		INT cArgc;
		CHAR **ppArgv;
	} ARGS_UTIL_TESTDLL;

} Trapped_Args;

#define CMD_BASE                    100

/* MGR module offsets */
#define CMD_MGR_BASE_OFFSET             CMD_BASE
#define CMD_MGR_ENUMNODE_INFO_OFFSET    (CMD_MGR_BASE_OFFSET + 0)
#define CMD_MGR_ENUMPROC_INFO_OFFSET    (CMD_MGR_BASE_OFFSET + 1)
#define CMD_MGR_REGISTEROBJECT_OFFSET   (CMD_MGR_BASE_OFFSET + 2)
#define CMD_MGR_UNREGISTEROBJECT_OFFSET (CMD_MGR_BASE_OFFSET + 3)
#define CMD_MGR_WAIT_OFFSET             (CMD_MGR_BASE_OFFSET + 4)

#ifndef RES_CLEANUP_DISABLE
#define CMD_MGR_RESOUCES_OFFSET         (CMD_MGR_BASE_OFFSET + 5)
#define CMD_MGR_END_OFFSET              CMD_MGR_RESOUCES_OFFSET
#else
#define CMD_MGR_END_OFFSET              CMD_MGR_WAIT_OFFSET
#endif

#define CMD_PROC_BASE_OFFSET            (CMD_MGR_END_OFFSET + 1)
#define CMD_PROC_ATTACH_OFFSET          (CMD_PROC_BASE_OFFSET + 0)
#define CMD_PROC_CTRL_OFFSET            (CMD_PROC_BASE_OFFSET + 1)
#define CMD_PROC_DETACH_OFFSET          (CMD_PROC_BASE_OFFSET + 2)
#define CMD_PROC_ENUMNODE_OFFSET        (CMD_PROC_BASE_OFFSET + 3)
#define CMD_PROC_ENUMRESOURCES_OFFSET   (CMD_PROC_BASE_OFFSET + 4)
#define CMD_PROC_GETSTATE_OFFSET        (CMD_PROC_BASE_OFFSET + 5)
#define CMD_PROC_GETTRACE_OFFSET        (CMD_PROC_BASE_OFFSET + 6)
#define CMD_PROC_LOAD_OFFSET            (CMD_PROC_BASE_OFFSET + 7)
#define CMD_PROC_REGISTERNOTIFY_OFFSET  (CMD_PROC_BASE_OFFSET + 8)
#define CMD_PROC_START_OFFSET           (CMD_PROC_BASE_OFFSET + 9)
#define CMD_PROC_RSVMEM_OFFSET          (CMD_PROC_BASE_OFFSET + 10)
#define CMD_PROC_UNRSVMEM_OFFSET        (CMD_PROC_BASE_OFFSET + 11)
#define CMD_PROC_MAPMEM_OFFSET          (CMD_PROC_BASE_OFFSET + 12)
#define CMD_PROC_UNMAPMEM_OFFSET        (CMD_PROC_BASE_OFFSET + 13)
#define CMD_PROC_FLUSHMEMORY_OFFSET      (CMD_PROC_BASE_OFFSET + 14)
#define CMD_PROC_STOP_OFFSET            (CMD_PROC_BASE_OFFSET + 15)
#define CMD_PROC_INVALIDATEMEMORY_OFFSET (CMD_PROC_BASE_OFFSET + 16)
#define CMD_PROC_END_OFFSET             CMD_PROC_INVALIDATEMEMORY_OFFSET


#define CMD_NODE_BASE_OFFSET            (CMD_PROC_END_OFFSET + 1)
#define CMD_NODE_ALLOCATE_OFFSET        (CMD_NODE_BASE_OFFSET + 0)
#define CMD_NODE_ALLOCMSGBUF_OFFSET     (CMD_NODE_BASE_OFFSET + 1)
#define CMD_NODE_CHANGEPRIORITY_OFFSET  (CMD_NODE_BASE_OFFSET + 2)
#define CMD_NODE_CONNECT_OFFSET         (CMD_NODE_BASE_OFFSET + 3)
#define CMD_NODE_CREATE_OFFSET          (CMD_NODE_BASE_OFFSET + 4)
#define CMD_NODE_DELETE_OFFSET          (CMD_NODE_BASE_OFFSET + 5)
#define CMD_NODE_FREEMSGBUF_OFFSET      (CMD_NODE_BASE_OFFSET + 6)
#define CMD_NODE_GETATTR_OFFSET         (CMD_NODE_BASE_OFFSET + 7)
#define CMD_NODE_GETMESSAGE_OFFSET      (CMD_NODE_BASE_OFFSET + 8)
#define CMD_NODE_PAUSE_OFFSET           (CMD_NODE_BASE_OFFSET + 9)
#define CMD_NODE_PUTMESSAGE_OFFSET      (CMD_NODE_BASE_OFFSET + 10)
#define CMD_NODE_REGISTERNOTIFY_OFFSET  (CMD_NODE_BASE_OFFSET + 11)
#define CMD_NODE_RUN_OFFSET             (CMD_NODE_BASE_OFFSET + 12)
#define CMD_NODE_TERMINATE_OFFSET       (CMD_NODE_BASE_OFFSET + 13)
#define CMD_NODE_GETUUIDPROPS_OFFSET    (CMD_NODE_BASE_OFFSET + 14)
#define CMD_NODE_END_OFFSET             CMD_NODE_GETUUIDPROPS_OFFSET

#define CMD_STRM_BASE_OFFSET            (CMD_NODE_END_OFFSET + 1)
#define CMD_STRM_ALLOCATEBUFFER_OFFSET  (CMD_STRM_BASE_OFFSET + 0)
#define CMD_STRM_CLOSE_OFFSET           (CMD_STRM_BASE_OFFSET + 1)
#define CMD_STRM_FREEBUFFER_OFFSET      (CMD_STRM_BASE_OFFSET + 2)
#define CMD_STRM_GETEVENTHANDLE_OFFSET  (CMD_STRM_BASE_OFFSET + 3)
#define CMD_STRM_GETINFO_OFFSET         (CMD_STRM_BASE_OFFSET + 4)
#define CMD_STRM_IDLE_OFFSET            (CMD_STRM_BASE_OFFSET + 5)
#define CMD_STRM_ISSUE_OFFSET           (CMD_STRM_BASE_OFFSET + 6)
#define CMD_STRM_OPEN_OFFSET            (CMD_STRM_BASE_OFFSET + 7)
#define CMD_STRM_RECLAIM_OFFSET         (CMD_STRM_BASE_OFFSET + 8)
#define CMD_STRM_REGISTERNOTIFY_OFFSET  (CMD_STRM_BASE_OFFSET + 9)
#define CMD_STRM_SELECT_OFFSET          (CMD_STRM_BASE_OFFSET + 10)
#define CMD_STRM_END_OFFSET             CMD_STRM_SELECT_OFFSET

/* Communication Memory Manager (UCMM) */
#define CMD_CMM_BASE_OFFSET             (CMD_STRM_END_OFFSET + 1)
#define CMD_CMM_ALLOCBUF_OFFSET         (CMD_CMM_BASE_OFFSET + 0)
#define CMD_CMM_FREEBUF_OFFSET          (CMD_CMM_BASE_OFFSET + 1)
#define CMD_CMM_GETHANDLE_OFFSET        (CMD_CMM_BASE_OFFSET + 2)
#define CMD_CMM_GETINFO_OFFSET          (CMD_CMM_BASE_OFFSET + 3)
#define CMD_CMM_END_OFFSET              CMD_CMM_GETINFO_OFFSET

/* MEMRY module offsets */
#define CMD_MEM_BASE_OFFSET             (CMD_CMM_END_OFFSET + 1)
#define CMD_MEM_ALLOC_OFFSET            (CMD_MEM_BASE_OFFSET + 0)
#define CMD_MEM_CALLOC_OFFSET           (CMD_MEM_BASE_OFFSET + 1)
#define CMD_MEM_FREE_OFFSET             (CMD_MEM_BASE_OFFSET + 2)
#define CMD_MEM_PAGELOCK_OFFSET         (CMD_MEM_BASE_OFFSET + 3)
#define CMD_MEM_PAGEUNLOCK_OFFSET       (CMD_MEM_BASE_OFFSET + 4)
#define CMD_MEM_END_OFFSET              CMD_MEM_PAGEUNLOCK_OFFSET

/* UTIL module */
#define CMD_UTIL_BASE_OFFSET            (CMD_MEM_END_OFFSET + 1)
#define CMD_UTIL_TESTDLL_OFFSET         (CMD_UTIL_BASE_OFFSET + 0)
#define CMD_UTIL_END_OFFSET             CMD_UTIL_TESTDLL_OFFSET

/* !!! place all command modules before CMD_BASE_END_OFFSET */
#define CMD_BASE_END_OFFSET             CMD_UTIL_END_OFFSET

#endif				/* WCDIOCTL_ */
