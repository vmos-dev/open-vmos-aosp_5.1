/*
 * dspbridge/mpu_api/inc/DSPManager.h
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
 *  ======== DSPManager.h ========
 *  Description:
 *      This is the header for the DSP/BIOS Bridge manager module.
 *
 *  Public Functions:
 *      DspManager_Open
 *      DspManager_Close
 *      DSPManager_EnumNodeInfo
 *      DSPManager_EnumProcessorInfo
 *      DSPManager_WaitForEvents
 *      DSPManager_RegisterObject
 *      DSPManager_UnregisterObject
 *
 *! Revision History:
 *! ================
 *! 03-Dec-2003 map Replaced include of dbdcddefs.h with dbdefs.h
 *! 22-Nov-2002 gp  Replaced include of dbdcd.h w/ dbdcddefs.h (hiding DCD APIs)
 *!                 Formatting cleanup.
 *! 15-Oct-2002 kc  Removed legacy PERF definitions.
 *! 16-Aug-2002 map Added DSPManager_RegisterObject/UnregisterObject for
 *!                     registering Dynamic Libraries
 *! 13-Feb-2001 kc: DSP/BIOS Bridge name updates.
 *! 22-Nov-2000 kc: Added DSPManager_PerfGetStat to acquire PERF stats.
 *! 25-Sep-2000 rr: Updated to Version 0.9
 *! 14-Aug-2000 rr: Cleaned up.
 *! 20-Jul-2000 rr: Updated to Version 0.8.
 *! 27-Jun-2000 rr: Created from dbapi.h
 */

#ifndef DSPMANAGER_
#define DSPMANAGER_

#ifdef __cplusplus
extern "C" {
#endif

#include <dbdefs.h>

/*
 *  ======== DspManager_Open ========
 *  Purpose:
 *      Open handle to the DSP/BIOS Bridge driver
 *  Parameters:
 *      argc:               Reserved, set to zero
 *      argp:               Reserved, set to NULL
 *                          in the database will be returned.
 *  Returns:
 *      DSP_SOK:            Success.
 *      DSP_EFAIL:          Failed to open handle to the DSP/BIOS Bridge driver
 *  Details:
 */
	extern DBAPI DspManager_Open(UINT argc, PVOID argp);

/*
 *  ======== DspManager_Close ========
 *  Purpose:
 *      Close handle to the DSP/BIOS Bridge driver
 *  Parameters:
 *      argc:               Reserved, set to zero
 *      argp:               Reserved, set to NULL
 *                          in the database will be returned.
 *  Returns:
 *      DSP_SOK:            Success.
 *      DSP_EFAIL:          Failed to close handle to the DSP/BIOS Bridge driver
 *  Details:
 */
	extern DBAPI DspManager_Close(UINT argc, PVOID argp);

/*
 *  ======== DSPManager_EnumNodeInfo ========
 *  Purpose:
 *      Enumerate and get configuration information about nodes configured
 *      in the node database.
 *  Parameters:
 *      uNode:              The node index, counting up from 0.
 *      pNDBProps:          Ptr to the DSP_NDBPROPS structure for output.
 *      uNDBPropsSize:      Size of the DSP_NDBPROPS structure.
 *      puNumNodes:         Location where the number of nodes configured
 *                          in the database will be returned.
 *  Returns:
 *      DSP_SOK:            Success.
 *      DSP_EINVALIDARG:    Parameter uNode is out of range
 *      DSP_EPOINTER:       Parameter pNDBProps or puNumNodes is not valid
 *      DSP_EFAIL:          Unable to get the node information.
 *      DSP_ESIZE:          The size of the specified DSP_NDBPROPS structure
 *                          is too small to hold all node information,
 *                          (i.e., uNDBPropsSize is too small).
 *      DSP_ECHANGEDURINGENUM:  During Enumeration there has been a change in
 *                          the number of nodes configured or in the
 *                          the properties of the enumerated nodes.
 *  Details:
 */
	extern DBAPI DSPManager_EnumNodeInfo(UINT uNode,
					     OUT struct DSP_NDBPROPS * pNDBProps,
					     UINT uNDBPropsSize,
					     OUT UINT * puNumNodes);

/*
 *  ======== DSPManager_EnumProcessorInfo ========
 *  Purpose:
 *      Enumerate and get configuration information about available DSP
 *      processors.
 *  Parameters:
 *      uProcessor:         The processor index, counting up from 0.
 *      pProcessorInfo:     Ptr to the DSP_PROCESSORINFO structure .
 *      uProcessorInfoSize: Size of DSP_PROCESSORINFO structure.
 *      puNumProcs:         Location where the number of DSPs configured
 *                          in the database will be returned
 *  Returns:
 *      DSP_SOK:            Success.
 *      DSP_EINVALIDARG:    Parameter uProcessor is out of range
 *      DSP_EPOINTER:       Parameter pProcessorInfo or puNumProcs is not valid.
 *      DSP_EFAIL:          Unable to get the processor information.
 *      DSP_ESIZE:          The size of the specified DSP_PROCESSORINFO struct
 *                          is too small to hold all the processor information,
 *                          (i.e., uProcessorInfoSize is too small).
 *  Details:
 */
	extern DBAPI DSPManager_EnumProcessorInfo(UINT uProcessor,
						  OUT struct DSP_PROCESSORINFO *
						  pProcessorInfo,
						  UINT uProcessorInfoSize,
						  OUT UINT * puNumProcs);

/*
 *  ======== DSPManager_WaitForEvents ========
 *  Purpose:
 *      Block on any Bridge event(s)
 *  Parameters:
 *      aNotifications  : array of pointers to notification objects.
 *      uCount          : number of elements in above array
 *      puIndex         : index of signaled event object
 *      uTimeout        : timeout interval in milliseocnds
 *  Returns:
 *      DSP_SOK         : Success.
 *      DSP_ETIMEOUT    : Wait timed out. *puIndex is undetermined.
 *  Details:
 */
	extern DBAPI DSPManager_WaitForEvents(struct DSP_NOTIFICATION**
					      aNotifications, UINT uCount,
					      OUT UINT * puIndex,
					      UINT uTimeout);

/*
 *  ======== DSPManager_RegisterObject ========
 *  Purpose:
 *     Register object with DSP/BIOS Bridge Configuration database (DCD).
 *  Parameters:
 *     pUuid:          Pointer to UUID structure.
 *     objType:        Library Type
 *     pszPathName:    Path to library
 *  Returns:
 *     DSP_SOK:        Success.
 *     DSP_EFAIL:      Unable to register object with the DCD.
 *  Details:
 */
	extern DBAPI DSPManager_RegisterObject(struct DSP_UUID * pUuid,
					       DSP_DCDOBJTYPE objType,
					       CHAR * pszPathName);

/*
 *  ======== DSPManager_UnregisterObject ========
 *  Purpose:
 *     Unregister object with DSP/BIOS Bridge Configuration Database (DCD).
 *  Parameters:
 *     pUuid:          Pointer to UUID structure.
 *     objType:        Library Type
 *  Returns:
 *     DSP_SOK:        Success.
 *     DSP_EFAIL:      Unable to unregister object from the DCD.
 *  Details:
 */
	extern DBAPI DSPManager_UnregisterObject(struct DSP_UUID * pUuid,
						 DSP_DCDOBJTYPE objType);

#ifndef RES_CLEANUP_DISABLE
/*
 *  ======== DSPManager_GetProcResourceInfo========
 *  Purpose:
 *     Get GPP process resource information.
 *  Parameters:
 *     pBuf:           Pointer to information buffer.
 *  Returns:
 *     DSP_SOK:        Success.
 *     DSP_EFAIL:      Unable to unregister object from the DCD.
 *  Details:
 */
	extern DBAPI DSPManager_GetProcResourceInfo(UINT *pBuf, UINT *pSize);
#endif


#ifdef __cplusplus
}
#endif
#endif				/* DSPManager_ */
