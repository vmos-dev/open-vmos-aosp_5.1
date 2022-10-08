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
 * qosti_dspdecl.h
 *
 * DSP-BIOS Bridge driver support functions for TI OMAP processors.
 */

#ifndef _QOSTI_DSPDECL_H

#define _QOSTI_DSPDECL_H

// The QOS command codes (to match the DSP side equivalents).

/*

 *  ======== QOS_TI_Msg ========

 *  The enumeration defines the control command selector for *cmd*

 *  for the QOS_TI layer messages.

 */

typedef enum QOS_TI_Msg {

	QOS_TI_ERROR,

	QOS_TI_GETCPULOAD,

	QOS_TI_GETMEMSTAT,

	QOS_TI_GETSHAREDSCRATCH,

} QOS_TI_Msg;

/*

 *  ======== QOS_TI_MsgArg1 ========

 *  The enumeration defines the control command selector for *arg1*

 *  of the QOS_TI_GETMEMSTAT message.

 */

typedef enum QOS_TI_GetMemStatArg1 {

	/* Arg1: Range 0-0x100 used for HEAPID of a BIOS MEM Segment with Heap */

	ALLHEAPS = 0x100,	/* Get aggregate mem-stat info combining all heaps. */

	NUMHEAPS		/* Get number BIOS MEM segments with Heaps */
} QOS_TI_GetMemStatArg1;

/*

 *  ======== QOS_TI_GetMemStatArg2 ========

 *  The enumeration defines the control command selector for *arg2*

 *  of the QOS_TI_GETMEMSTAT message.

 */

typedef enum QOS_TI_GetMemStatArg2 {

	USED_HEAPSIZE = 0x100,

	LARGEST_FREE_BLOCKSIZE
} QOS_TI_GetMemStatArg2;

/*

 *  ======== QOS_TI_GetSharedScratchMsgArg2 ========

 *  The enumeration defines the control command selector for *arg2*

 *  of the QOS_TI_GETSHAREDSCRATCH message.

 */

typedef enum QOS_TI_GetSharedScratchMsgArg2 {

	/*

	 * If Arg2 is NOT set to ALL_SCRATCHGROUPS then  the assigned value

	 * taken as the Scratch Group/Mutex Id

	 */

	ALL_SCRATCHGROUPS
} QOS_TI_GetSharedScratchMsgArg2;

#endif

