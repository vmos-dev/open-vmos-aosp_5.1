/*
 * dspbridge/mpu_api/inc/dbapi.h
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
 *  ======== dbapi.h ========
 *  Description:
 *      Top level header file for GPP side DSP/BIOS Bridge APIs.
 *
 *! Revision History:
 *! ================
 *! 22-Nov-2002 gp  Cleaned up comments, formatting.
 *! 13-Feb-2001 kc: Name changed from ddspapi.h to dbapi.h.
 *! 27-Jun-2000 rr: Name changed to ddspapi.h. Broken into various h files.
 *! 12-May-2000 gp: Removed PROC_UNKNOWN state.  Moved OEM DSPProcessor_ fxns
 *!                 to ddspoem.h. Changed DDSP_MSG to be fixed length;
 *!                 Changed DDSPStream_Issue/Reclaim to take DWORD dwArg.
 *! 11-May-2000 gp: Reformatted; converted tabs to spaces; changed NODEHANDLE
 *!                 to HNODE; changed GUID to UUID; added "Detail" sections
 *!                 documenting differences from DDSP API spec. (ver. 0.6);
 *!                 enhanced comments.
 *! 19-Apr-2000 ww: Updated based on code review.
 *! 12-Apr-2000 ww: Created based on DSP/BIOS Bridge API specification, Version 0.6.
 */

#ifndef DBAPI_
#define DBAPI_

#ifdef __cplusplus
extern "C" {
#endif

#include <dbdefs.h>		/* DSP/BIOS Bridge global definitions and constants */
#include <errbase.h>		/* DSP/BIOS Bridge status and error codes           */
#include <DSPManager.h>		/* DSP/BIOS Bridge Manager APIs                     */
#include <DSPProcessor.h>	/* DSP/BIOS Bridge Processor APIs                   */
#include <DSPNode.h>		/* DSP/BIOS Bridge Node APIs                        */
#include <DSPStream.h>		/* DSP/BIOS Bridge Stream APIs                      */

#ifdef __cplusplus
}
#endif
#endif				/* DBAPI_ */
