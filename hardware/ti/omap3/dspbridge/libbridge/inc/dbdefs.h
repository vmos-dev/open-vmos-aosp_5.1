/*
 * dspbridge/mpu_api/inc/dbdefs.h
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
 *  ======== dbdefs.h ========
 *  Description:
 *      Global definitions and constants for DSP/BIOS Bridge.
 *
 *! Revision History:
 *! ================
 *! 19-Apr-2004 sb  Aligned DMM definitions with Symbian
 *! 08-Mar-2004 sb  Added MAPATTR & ELEM_SIZE for Dynamic Memory Mapping feature
 *! 09-Feb-2004 vp  Added processor ID numbers for DSP and IVA
 *! 06-Feb-2003 kc  Removed DSP_POSTMESSAGE. Updated IsValid*Event macros.
 *! 22-Nov-2002 gp  Cleaned up comments, formatting.
 *!                 Removed unused DSP_ENUMLASTNODE define.
 *! 13-Feb-2002 jeh Added uSysStackSize to DSP_NDBPROPS.
 *! 23-Jan-2002 ag  Added #define DSP_SHMSEG0.
 *! 12-Dec-2001 ag  Added DSP_ESTRMMODE error code.
 *! 04-Dec-2001 jeh Added DSP_ENOTCONNECTED error code.
 *! 10-Dec-2001 kc: Modified macros and definitions to disable DSP_POSTMESSAGE.
 *! 01-Nov-2001 jeh Added DSP_EOVERLAYMEMORY.
 *! 18-Oct-2001 ag  Added DSP_STRMMODE type.
 *!                 Added DSP_ENOTSHAREDMEM.
 *! 21-Sep-2001 ag  Added additional error codes.
 *! 07-Jun-2001 sg  Made DSPStream_AllocateBuffer fxn name plural.
 *! 11-May-2001 jeh Changed DSP_NODE_MIN_PRIORITY from 0 to 1. Removed hNode
 *!                 from DSP_NODEINFO.
 *! 02-Apr-2001 sg  Added missing error codes, rearranged codes, switched to
 *!             hex offsets, renamed some codes to match API spec.
 *! 16-Jan-2001 jeh Added DSP_ESYMBOL, DSP_EUUID.
 *! 13-Feb-2001 kc: DSP/BIOS Bridge name updates.
 *! 05-Dec-2000 ag: Added DSP_RMSxxx user available message command codes.
 *! 09-Nov-2000 rr: Added DSP_PROCEESORRESTART define; Removed DSP_PBUFFER.
 *!                 Added DSP_DCD_ENOAUTOREGISTER, DSP_EUSER1-16, DSP_ESTRMFUL
 *!                 Removed DSP_EDONE. Macros's modified.
 *! 23-Oct-2000 jeh Replaced DSP_STREAMSTATECHANGE with DSP_STREAMDONE.
 *! 09-Oct-2000 jeh Updated to version 0.9 DSP Bridge API spec.
 *! 29-Sep-2000 kc  Added error codes for DCD and REG to simplify use of
 *!                 these codes within the RM module.
 *! 27-Sep-2000 jeh Added segid, alignment, uNumBufs to DSP_STREAMATTRIN.
 *! 29-Aug-2000 jeh Added DSP_NODETYPE enum, changed DSP_EALREADYATTACHED to
 *!                 DSP_EALREADYCONNECTED. Changed scStreamConnection[1]
 *!                 to scStreamConnection[16] in DSP_NODEINFO structure.
 *!                 Added DSP_NOTIFICATION, DSP_STRMATTR. PSTRING changed
 *!                 back to TCHAR * and moved to dbtype.h.
 *! 11-Aug-2000 rr: Macros to check valid events and notify masks added.
 *! 09-Aug-2000 rr: Changed PSTRING to *CHAR
 *! 07-Aug-2000 rr: PROC_IDLE/SYNCINIT/UNKNOWN state removed.
 *! 20-Jul-2000 rr: Updated to version 0.8
 *! 17-Jul-2000 rr: New PROC states added to the DSP_PROCSTATE.
 *! 27-Jun-2000 rr: Created from dspapi.h
 */

#ifndef DBDEFS_
#define DBDEFS_

#include <dbtype.h>		/* GPP side type definitions           */
#include <std.h>		/* DSP/BIOS type definitions           */
#include <rms_sh.h>		/* Types shared between GPP and DSP    */

#ifdef __cplusplus
extern "C" {
#endif

#define PG_SIZE_4K 4096
#define PG_MASK(pg_size) (~((pg_size)-1))
#define PG_ALIGN_LOW(addr, pg_size) ((addr) & PG_MASK(pg_size))
#define PG_ALIGN_HIGH(addr, pg_size) (((addr)+(pg_size)-1) & PG_MASK(pg_size))

/* API return value and calling convention */
#define DBAPI                       DSP_STATUS CDECL

/* Infinite time value for the uTimeout parameter to DSPStream_Select() */
#define DSP_FOREVER                 (-1)

/* Maximum length of node name, used in DSP_NDBPROPS */
#define DSP_MAXNAMELEN              32

/* uNotifyType values for the RegisterNotify() functions. */
#define DSP_SIGNALEVENT             0x00000001

/* Types of events for processors */
#define DSP_PROCESSORSTATECHANGE    0x00000001
#define DSP_PROCESSORATTACH         0x00000002
#define DSP_PROCESSORDETACH         0x00000004
#define DSP_PROCESSORRESTART        0x00000008

/* DSP exception events (DSP/BIOS and DSP MMU fault) */
#define DSP_MMUFAULT                0x00000010
#define DSP_SYSERROR                0x00000020

/* IVA exception events (IVA MMU fault) */
#define IVA_MMUFAULT                0x00000040
/* Types of events for nodes */
#define DSP_NODESTATECHANGE         0x00000100
#define DSP_NODEMESSAGEREADY        0x00000200

/* Types of events for streams */
#define DSP_STREAMDONE              0x00001000
#define DSP_STREAMIOCOMPLETION      0x00002000

/* Handle definition representing the GPP node in DSPNode_Connect() calls */
#define DSP_HGPPNODE                0xFFFFFFFF

/* Node directions used in DSPNode_Connect() */
#define DSP_TONODE                  1
#define DSP_FROMNODE                2

/* Define Node Minimum and Maximum Priorities */
#define DSP_NODE_MIN_PRIORITY       1
#define DSP_NODE_MAX_PRIORITY       15

/* Pre-Defined Message Command Codes available to user: */
#define DSP_RMSUSERCODESTART RMS_USER	/* Start of RMS user cmd codes */
#define DSP_RMSUSERCODEEND RMS_USER + RMS_MAXUSERCODES;	/* end of user codes */
#define DSP_RMSBUFDESC RMS_BUFDESC	/* MSG contains SM buffer description */

/* Shared memory identifier for MEM segment named "SHMSEG0" */
#define DSP_SHMSEG0     (UINT)(-1)

/* Processor ID numbers */
#define DSP_UNIT    0
#define IVA_UNIT    1

#define DSPWORD       BYTE
#define DSPWORDSIZE     sizeof(DSPWORD)

/* Success & Failure macros  */
#define DSP_SUCCEEDED(Status)      ((INT)(Status) >= 0)
#define DSP_FAILED(Status)         ((INT)(Status) < 0)

/* Power control enumerations */
#define PROC_PWRCONTROL             0x8070

#define PROC_PWRMGT_ENABLE          (PROC_PWRCONTROL + 0x3)
#define PROC_PWRMGT_DISABLE         (PROC_PWRCONTROL + 0x4)

/* Bridge Code Version */
#define BRIDGE_VERSION_CODE         333

#define    MAX_PROFILES     16

/* Types defined for 'Bridge API */
	typedef DWORD DSP_STATUS;	/* API return code type         */

	typedef HANDLE DSP_HNODE;	/* Handle to a DSP Node object  */
	typedef HANDLE DSP_HPROCESSOR;	/* Handle to a Processor object */
	typedef HANDLE DSP_HSTREAM;	/* Handle to a Stream object    */

	typedef ULONG DSP_PROCFAMILY;	/* Processor family             */
	typedef ULONG DSP_PROCTYPE;	/* Processor type (w/in family) */
	typedef ULONG DSP_RTOSTYPE;	/* Type of DSP RTOS             */

	typedef ULONG DSP_RESOURCEMASK;	/* Mask for processor resources */
	typedef ULONG DSP_ERRORMASK;	/* Mask for various error types */

/* Handy Macros */
#define IsValidProcEvent(x)    (((x) == 0) || (((x) & (DSP_PROCESSORSTATECHANGE | \
                                    DSP_PROCESSORATTACH | \
                                    DSP_PROCESSORDETACH | \
                                    DSP_PROCESSORRESTART | \
                                    DSP_NODESTATECHANGE | \
                                    DSP_STREAMDONE | \
                                    DSP_STREAMIOCOMPLETION | \
                                    DSP_MMUFAULT | \
                                    DSP_SYSERROR)) && \
                                !((x) & ~(DSP_PROCESSORSTATECHANGE | \
                                    DSP_PROCESSORATTACH | \
                                    DSP_PROCESSORDETACH | \
                                    DSP_PROCESSORRESTART | \
                                    DSP_NODESTATECHANGE | \
                                    DSP_STREAMDONE | \
                                    DSP_STREAMIOCOMPLETION | \
                                    DSP_MMUFAULT | \
                                    DSP_SYSERROR))))

#define IsValidNodeEvent(x)    (((x) == 0) || (((x) & (DSP_NODESTATECHANGE | \
                                DSP_NODEMESSAGEREADY)) && \
                                !((x) & ~(DSP_NODESTATECHANGE | \
                                DSP_NODEMESSAGEREADY))))

#define IsValidStrmEvent(x)     (((x) == 0) || (((x) & (DSP_STREAMDONE | \
                                DSP_STREAMIOCOMPLETION)) && \
                                !((x) & ~(DSP_STREAMDONE | \
                                DSP_STREAMIOCOMPLETION))))

#define IsValidNotifyMask(x)   ((x) & DSP_SIGNALEVENT)

/* The Node UUID structure */
	struct DSP_UUID {
		ULONG ulData1;
		USHORT usData2;
		USHORT usData3;
		BYTE ucData4;
		BYTE ucData5;
		UCHAR ucData6[6];
	};
	/*DSP_UUID, *DSP_HUUID;*/

/* DCD types */
	typedef enum {
		DSP_DCDNODETYPE,
		DSP_DCDPROCESSORTYPE,
		DSP_DCDLIBRARYTYPE,
		DSP_DCDCREATELIBTYPE,
		DSP_DCDEXECUTELIBTYPE,
		DSP_DCDDELETELIBTYPE
	} DSP_DCDOBJTYPE;

/* Processor states */
	typedef enum {
		PROC_STOPPED,
		PROC_LOADED,
		PROC_RUNNING,
		PROC_ERROR
	} DSP_PROCSTATE;

/* Node types */
	typedef enum {
		NODE_DEVICE,
		NODE_TASK,
		NODE_DAISSOCKET,
		NODE_MESSAGE
	} DSP_NODETYPE;

/* Node states */
	typedef enum {
		NODE_ALLOCATED,
		NODE_CREATED,
		NODE_RUNNING,
		NODE_PAUSED,
		NODE_DONE
	} DSP_NODESTATE;

/* Stream states */
	typedef enum {
		STREAM_IDLE,
		STREAM_READY,
		STREAM_PENDING,
		STREAM_DONE
	} DSP_STREAMSTATE;

/* Stream connect types */
	typedef enum {
		CONNECTTYPE_NODEOUTPUT,
		CONNECTTYPE_GPPOUTPUT,
		CONNECTTYPE_NODEINPUT,
		CONNECTTYPE_GPPINPUT
	} DSP_CONNECTTYPE;

/* Stream mode types */
	typedef enum {
		STRMMODE_PROCCOPY,	/* Processor(s) copy stream data payloads */
		STRMMODE_ZEROCOPY,	/* Stream buffer pointers swapped, no data copied */
		STRMMODE_LDMA,	/* Local DMA : OMAP's System-DMA device */
		STRMMODE_RDMA	/* Remote DMA: OMAP's DSP-DMA device */
	} DSP_STRMMODE;

/* Stream DMA priority. Only Low and High supported */
	typedef enum {
		DMAPRI_LOW,
		DMAPRI_HIGH
	} DSP_DMAPRIORITY;

/* Resource Types */
	typedef enum {
		DSP_RESOURCE_DYNDARAM = 0,
		DSP_RESOURCE_DYNSARAM,
		DSP_RESOURCE_DYNEXTERNAL,
		DSP_RESOURCE_DYNSRAM,
		DSP_RESOURCE_PROCLOAD
	} DSP_RESOURCEINFOTYPE;

/* Memory Segment Types */
	typedef enum {
		DSP_DYNDARAM = 0,
		DSP_DYNSARAM,
		DSP_DYNEXTERNAL,
		DSP_DYNSRAM
	} DSP_MEMTYPE;

/* Memory Flush Types */
       typedef enum {
		PROC_INVALIDATE_MEM = 0,
		PROC_WRITEBACK_MEM,
		PROC_WRITEBACK_INVALIDATE_MEM,
	} DSP_FLUSHTYPE;

/* Memory Segment Status Values */
	 struct DSP_MEMSTAT {
		ULONG ulSize;
		ULONG ulTotalFreeSize;
		ULONG ulLenMaxFreeBlock;
		ULONG ulNumFreeBlocks;
		ULONG ulNumAllocBlocks;
	} ;

/* Processor Load information Values */
	 struct DSP_PROCLOADSTAT {
		ULONG uCurrLoad;
		ULONG uPredictedLoad;
		ULONG uCurrDspFreq;
		ULONG uPredictedFreq;
	} ;

/* Attributes for STRM connections between nodes */
	struct DSP_STRMATTR {
		UINT uSegid;	/* Memory segment on DSP to allocate buffers */
		UINT uBufsize;	/* Buffer size (DSP words) */
		UINT uNumBufs;	/* Number of buffers */
		UINT uAlignment;	/* Buffer alignment */
		UINT uTimeout;	/* Timeout for blocking STRM calls */
		UINT lMode;	/* mode of stream when opened */
		UINT uDMAChnlId;	/* DMA chnl id if DSP_STRMMODE is LDMA or RDMA */
		UINT uDMAPriority;	/* DMA channel priority 0=lowest, >0=high */
	} ;

/* The DSP_CBDATA structure */
	struct DSP_CBDATA {
		ULONG cbData;
		BYTE cData[1];
	} ;
	/*DSP_CBDATA, *DSP_HCBDATA;*/

/* The DSP_MSG structure */
	struct DSP_MSG {
		DWORD dwCmd;
		DWORD dwArg1;
		DWORD dwArg2;
	} ;
	/*DSP_MSG, *DSP_HMSG;*/

/* The DSP_RESOURCEREQMTS structure for node's resource requirements  */
	struct DSP_RESOURCEREQMTS {
		DWORD cbStruct;
		UINT uStaticDataSize;
		UINT uGlobalDataSize;
		UINT uProgramMemSize;
		UINT uWCExecutionTime;
		UINT uWCPeriod;
		UINT uWCDeadline;
		UINT uAvgExectionTime;
		UINT uMinimumPeriod;
	} ;
	/*DSP_RESOURCEREQMTS, *DSP_HRESOURCEREQMTS;*/

/*
 * The DSP_STREAMCONNECT structure describes a stream connection
 * between two nodes, or between a node and the GPP
 */
	struct DSP_STREAMCONNECT {
		DWORD cbStruct;
		DSP_CONNECTTYPE lType;
		UINT uThisNodeStreamIndex;
		DSP_HNODE hConnectedNode;
		struct DSP_UUID uiConnectedNodeID;
		UINT uConnectedNodeStreamIndex;
	} ;
	/*DSP_STREAMCONNECT, *DSP_HSTREAMCONNECT;*/

	struct DSP_NODEPROFS {
		UINT ulHeapSize;
	} ;
	/*DSP_NODEPROFS, *DSP_HNODEPROFS;*/

/* The DSP_NDBPROPS structure reports the attributes of a node */
	struct DSP_NDBPROPS {
		DWORD cbStruct;
		struct DSP_UUID uiNodeID;
		CHARACTER acName[DSP_MAXNAMELEN];
		DSP_NODETYPE uNodeType;
		UINT bCacheOnGPP;
		struct DSP_RESOURCEREQMTS dspResourceReqmts;
		INT iPriority;
		UINT uStackSize;
		UINT uSysStackSize;
		UINT uStackSeg;
		UINT uMessageDepth;
		UINT uNumInputStreams;
		UINT uNumOutputStreams;
		UINT uTimeout;
		UINT uCountProfiles;	/* Number of supported profiles */
		/* Array of profiles */
		struct DSP_NODEPROFS aProfiles[MAX_PROFILES];
		UINT uStackSegName; /* Stack Segment Name */
	} ;
	/*DSP_NDBPROPS, *DSP_HNDBPROPS;*/

    /* The DSP_NODEATTRIN structure describes the attributes of a node client */
    struct DSP_NODEATTRIN {
            DWORD cbStruct;
            INT iPriority;
            UINT uTimeout;
            UINT    uProfileID;
			/* Reserved, for Bridge Internal use only */
            UINT    uHeapSize;   
            PVOID   pGPPVirtAddr; /* Reserved, for Bridge Internal use only */
        } ;
	/*DSP_NODEATTRIN, *DSP_HNODEATTRIN;*/

/* The DSP_NODEINFO structure is used to retrieve information about a node */
	struct DSP_NODEINFO {
		DWORD cbStruct;
		struct DSP_NDBPROPS nbNodeDatabaseProps;
		UINT uExecutionPriority;
		DSP_NODESTATE nsExecutionState;
		DSP_HNODE hDeviceOwner;
		UINT uNumberStreams;
		struct DSP_STREAMCONNECT scStreamConnection[16];
		UINT uNodeEnv;
	} ;
	/*DSP_NODEINFO, *DSP_HNODEINFO;*/

/* The DSP_NODEATTR structure describes the attributes of a node */
	struct DSP_NODEATTR {
		DWORD cbStruct;
		struct DSP_NODEATTRIN inNodeAttrIn;
		ULONG uInputs;
		ULONG uOutputs;
		struct DSP_NODEINFO iNodeInfo;
	} ;
	/*DSP_NODEATTR, *DSP_HNODEATTR;*/

/*
 *  Notification type: either the name of an opened event, or an event or
 *  window handle.
 */
	struct DSP_NOTIFICATION {
		PSTRING psName;
		HANDLE handle;
	} ;
	/*DSP_NOTIFICATION, *DSP_HNOTIFICATION;*/

/* The DSP_PROCESSORATTRIN structure describes the attributes of a processor */
	struct DSP_PROCESSORATTRIN{
		DWORD cbStruct;
		UINT uTimeout;
	} ;
	/*DSP_PROCESSORATTRIN, *DSP_HPROCESSORATTRIN;*/

	enum chipTypes {
		DSPTYPE_55 = 6,
		IVA_ARM7 = 0x97,
		DSPTYPE_64 = 0x99
	};

/*
 * The DSP_PROCESSORINFO structure describes basic capabilities of a
 * DSP processor
 */
	struct DSP_PROCESSORINFO {
		DWORD cbStruct;
		DSP_PROCFAMILY uProcessorFamily;
		DSP_PROCTYPE uProcessorType;
		UINT uClockRate;
		ULONG ulInternalMemSize;
		ULONG ulExternalMemSize;
		UINT uProcessorID;
		DSP_RTOSTYPE tyRunningRTOS;
		INT nNodeMinPriority;
		INT nNodeMaxPriority;
	} ;
	/*DSP_PROCESSORINFO, *DSP_HPROCESSORINFO;*/

/* Error information of last DSP exception signalled to the GPP */
	struct DSP_ERRORINFO {
		DWORD dwErrMask;
		DWORD dwVal1;
		DWORD dwVal2;
		DWORD dwVal3;
	} ;
	/*DSP_ERRORINFO;*/

/* The DSP_PROCESSORSTATE structure describes the state of a DSP processor */
	struct DSP_PROCESSORSTATE {
		DWORD cbStruct;
		DSP_PROCSTATE iState;
		struct DSP_ERRORINFO errInfo;
	} ;
	/*DSP_PROCESSORSTATE, *DSP_HPROCESSORSTATE;*/

/*
 * The DSP_RESOURCEINFO structure is used to retrieve information about a
 * processor's resources
 */
	struct DSP_RESOURCEINFO {
		DWORD cbStruct;
		DSP_RESOURCEINFOTYPE uResourceType;
		union {
			ULONG ulResource;
			struct DSP_MEMSTAT memStat;
			struct DSP_PROCLOADSTAT procLoadStat;
		} result;
	} ;
	/*DSP_RESOURCEINFO, *DSP_HRESOURCEINFO;*/

/*
 * The DSP_STREAMATTRIN structure describes the attributes of a stream,
 * including segment and alignment of data buffers allocated with
 * DSPStream_AllocateBuffers(), if applicable
 */
	struct DSP_STREAMATTRIN {
		DWORD cbStruct;
		UINT uTimeout;
		UINT uSegment;
		UINT uAlignment;
		UINT uNumBufs;
		UINT lMode;
		UINT uDMAChnlId;
		UINT uDMAPriority;
	} ;
	/*DSP_STREAMATTRIN, *DSP_HSTREAMATTRIN;*/

/* The DSP_BUFFERATTR structure describes the attributes of a data buffer */
	struct DSP_BUFFERATTR {
		DWORD cbStruct;
		UINT uSegment;
		UINT uAlignment;
	} ;
	/*DSP_BUFFERATTR, *DSP_HBUFFERATTR;*/

/*
 *  The DSP_STREAMINFO structure is used to retrieve information
 *  about a stream.
 */
	struct DSP_STREAMINFO {
		DWORD cbStruct;
		UINT uNumberBufsAllowed;
		UINT uNumberBufsInStream;
		ULONG ulNumberBytes;
		HANDLE hSyncObjectHandle;
		DSP_STREAMSTATE ssStreamState;
	} ;
	/*DSP_STREAMINFO, *DSP_HSTREAMINFO;*/

/* DMM MAP attributes 
It is a bit mask with each bit value indicating a specific attribute
bit 0 - GPP address type (user virtual=0, physical=1)
bit 1 - MMU Endianism (Big Endian=1, Little Endian=0)
bit 2 - MMU mixed page attribute (Mixed/ CPUES=1, TLBES =0)
bit 3 - MMU element size = 8bit (valid only for non mixed page entries)
bit 4 - MMU element size = 16bit (valid only for non mixed page entries)
bit 5 - MMU element size = 32bit (valid only for non mixed page entries)
bit 6 - MMU element size = 64bit (valid only for non mixed page entries)
*/

/* Types of mapping attributes */

/* MPU address is virtual and needs to be translated to physical addr */
#define DSP_MAPVIRTUALADDR          0x00000000
#define DSP_MAPPHYSICALADDR         0x00000001

/* Mapped data is big endian */
#define DSP_MAPBIGENDIAN            0x00000002
#define DSP_MAPLITTLEENDIAN         0x00000000

/* Element size is based on DSP r/w access size */
#define DSP_MAPMIXEDELEMSIZE        0x00000004

/*
 * Element size for MMU mapping (8, 16, 32, or 64 bit)
 * Ignored if DSP_MAPMIXEDELEMSIZE enabled
 */
#define DSP_MAPELEMSIZE8            0x00000008
#define DSP_MAPELEMSIZE16           0x00000010
#define DSP_MAPELEMSIZE32           0x00000020
#define DSP_MAPELEMSIZE64           0x00000040

#define DSP_MAPVMALLOCADDR         0x00000080

#define GEM_CACHE_LINE_SIZE     128
#define GEM_L1P_PREFETCH_SIZE   128

#ifdef __cplusplus
}
#endif
#endif				/* DBDEFS_ */
