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
 * Qosti.h
 * DSP-BIOS Bridge driver support functions for TI OMAP processors.
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

#endif				/* _RQOSTI_H*/

