/*
 * dspbridge/mpu_api/inc/qosti_dspdecl.h
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

