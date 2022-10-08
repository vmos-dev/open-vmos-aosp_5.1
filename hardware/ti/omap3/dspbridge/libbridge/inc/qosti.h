/*
 * dspbridge/mpu_api/inc/qosti.h
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


#ifndef _RQOSTI_H
#define _RQOSTI_H
#include <dbapi.h>
#include <qosti_dspdecl.h>
#include <stdio.h>
/* This must match QOS_TI_GetMemStatArg1::ALLHEAPS */
#define KAllHeaps 0x100
/* This must match QOS_TI_GetSharedScratchMsgArg2::ALL_SCRATCHGROUPS */
#define KAllScratchGroups 0
/*
 *  ======== QOS_TI_GetDynLoaderMemStatArg1 ========
 *  The enumeration defines the memory heap resources available for querying.
 */
typedef enum QOS_TI_GetDynLoaderMemStatArg1 {
	EDynloadDaram = 0,
	EDynloadSaram = 1,
	EDynloadExternal = 2,
	EDynloadSram = 3
} QOS_TI_GetDynLoaderMemStatArg1;

/*
 *  ======== QOS_TI_GetSharedScratchMsgArg1 ========
 *  The enumeration defines the control command selector for *arg1*
 *  of the QOS_TI_GETSHAREDSCRATCH message.
 */
typedef enum QOS_TI_GetSharedScratchMsgArg1 {

	ESharedScratchAllHeaps,

	ESharedScratchDaram,

	ESharedScratchSaram
} QOS_TI_GetSharedScratchMsgArg1;

void DbgMsg(DWORD dwZone, char *szFormat, ...);

/*  ============================================================================
  func   MsgToDsp
  desc   Send Message to DSP
  ret    DSP_SOK if Message was transferred to DSP successfully.
 ============================================================================*/

DSP_STATUS QosTI_DspMsg(DWORD dwCmd, DWORD dwArg1, DWORD dwArg2, DWORD *dwOut1,
																DWORD *dwOut2);

/*  ========================================================================
  func   Create
  desc   Create the Qos service.
  ret    DSP_SOK if successfully created.
  ========================================================================*/

DSP_STATUS QosTI_Create();

/* ========================================================================
  func   Delete
  desc   Delete Qos service.
  ========================================================================*/

void QosTI_Delete();

/*  ========================================================================
  func   GetDynLoaderMemStat
  desc   Get the current memory utilization for heaps used in dynamic loading.
  arg   IN heapDesc:  value in range 0..4 => Heap Identifier
                  Valid values:
                        EDynloadDaram    = DYN_DARAM heap (internal)
                        EDynloadSaram    = DYN_SARAM heap (internal)
                        EDynloadExternal = DYN_EXTERNAL heap (external)
                        EDynloadSram     = DYN_SRAM heap (internal)
  arg   OUT memInitSize:             initially configured size of heap
  arg   OUT memUsed:                 size of heap in use (not free)
  arg   OUT memLargestFreeBlockSize: size of largest contiguous free memory
  arg   OUT memFreeBlocks:           number of free blocks in heap
  arg   OUT memAllocBlocks:          number of allocated blocks in heap
  ret   DSP_SOK if successful.
  ========================================================================*/

DSP_STATUS QosTI_GetDynLoaderMemStat(UINT heapDesc, UINT *memInitSize, 
								UINT *memUsed, UINT *memLargestFreeBlockSize,
								UINT *memFreeBlocks, UINT *memAllocBlocks);


/*  ========================================================================
    func   QosTI_GetProcLoadStat
    desc   Get the Processor load statistics
    arg   OUT currentLoad:             
    arg   OUT predLoad:               
    arg   OUT currDspFreq: 
    arg   OUT predictedFreq:  
    ret   DSP_SOK if successful.
    ======================================================================== */
DSP_STATUS QosTI_GetProcLoadStat(UINT *currentLoad, UINT *predLoad, 
									    UINT *currDspFreq, UINT *predictedFreq);

#endif				/* _RQOSTI_H*/

