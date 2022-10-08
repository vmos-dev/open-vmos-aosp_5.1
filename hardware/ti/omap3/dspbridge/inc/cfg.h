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
 *  ======== cfg.h ========
 *  DSP-BIOS Bridge driver support functions for TI OMAP processors.
 *  Purpose:
 *      PM Configuration module.
 *
 *  Private Functions:
 *      CFG_Exit
 *      CFG_GetAutoStart
 *      CFG_GetCDVersion
 *      CFG_GetDevObject
 *      CFG_GetDSPResources
 *      CFG_GetExecFile
 *      CFG_GetHostResources
 *      CFG_GetObject
 *      CFG_GetPerfValue
 *      CFG_GetWMDFileName
 *      CFG_GetZLFile
 *      CFG_Init
 *      CFG_SetDevObject
 *      CFG_SetObject
 *
 *! Revision History:
 *! =================
 *! 26-Feb-2003 kc  Removed unused CFG fxns.
 *! 28-Aug-2001 jeh  Added CFG_GetLoaderName.
 *! 26-Jul-2000 rr:  Added CFG_GetDCDName to retrieve the DCD Dll name.
 *! 13-Jul-2000 rr:  Added CFG_GetObject & CFG_SetObject.
 *! 13-Jan-2000 rr:  CFG_Get/SetPrivateDword renamed to CFG_Get/SetDevObject.
 *!                  CFG_GetWinBRIDGEDir/Directory,CFG_GetSearchPath removed.
 *! 15-Jan-1998 cr:  Code review cleanup.
 *! 16-Aug-1997 cr:  Added explicit cdecl identifiers.
 *! 12-Dec-1996 gp:  Moved CFG_FindInSearchPath to CSP module.
 *! 13-Sep-1996 gp:  Added CFG_GetBoardName().
 *! 22-Jul-1996 gp:  Added CFG_GetTraceStr, to retrieve an initial GT trace.
 *! 26-Jun-1996 cr:  Added CFG_FindInSearchPath.
 *! 25-Jun-1996 cr:  Added CFG_GetWinSPOXDir.
 *! 17-Jun-1996 cr:  Added CFG_GetDevNode.
 *! 11-Jun-1996 cr:  Cleaned up for code review.
 *! 07-Jun-1996 cr:  Added CFG_GetExecFile and CFG_GetZLFileName functions.
 *! 04-Jun-1996 gp:  Added AutoStart regkey and accessor function.  Placed
 *!                  OUT parameters in accessor function param. lists at end.
 *! 29-May-1996 gp:  Moved DEV_HDEVNODE to here and renamed CFG_HDEVNODE.
 *! 22-May-1996 cr:  Added GetHostResources, GetDSPResources, and
 *!                  GetWMDFileName services.
 *! 18-May-1996 gp:  Created.
 */

#ifndef CFG_
#define CFG_

#ifdef __cplusplus
extern "C" {
#endif

#include <dspapi.h>
#include <cfgdefs.h>

/*
 *  ======== CFG_Exit ========
 *  Purpose:
 *      Discontinue usage of the CFG module.
 *  Parameters:
 *  Returns:
 *  Requires:
 *      CFG_Init() was previously called.
 *  Ensures:
 *      Resources acquired in CFG_Init() are freed.
 */
	extern VOID CFG_Exit();

/*
 *  ======== CFG_GetAutoStart ========
 *  Purpose:
 *      Retreive the autostart mask, if any, for this board.
 *  Parameters:
 *      hDevNode:       Handle to the DevNode who's WMD we are querying.
 *      pdwAutoStart:   Ptr to location for 32 bit autostart mask.
 *  Returns:
 *      DSP_SOK:                Success.
 *      CFG_E_INVALIDHDEVNODE:  hDevNode is invalid.
 *      CFG_E_RESOURCENOTAVAIL: Unable to retreive resource.
 *  Requires:
 *      CFG initialized.
 *  Ensures:
 *      DSP_SOK:        *pdwAutoStart contains autostart mask for this devnode.
 */
	extern DSP_STATUS CFG_GetAutoStart(IN struct CFG_DEVNODE* hDevNode,
					   OUT DWORD * pdwAutoStart);

/*
 *  ======== CFG_GetCDVersion ========
 *  Purpose:
 *      Retrieves the version of the PM Class Driver.
 *  Parameters:
 *      pdwVersion: Ptr to DWORD to contain version number upon return.
 *  Returns:
 *      DSP_SOK:    Success.  pdwVersion contains Class Driver version in
 *                  the form: 0xAABBCCDD where AABB is Major version and
 *                  CCDD is Minor.
 *      DSP_EFAIL:  Failure.
 *  Requires:
 *      CFG initialized.
 *  Ensures:
 *      DSP_SOK:    Success.
 *      else:       *pdwVersion is NULL.
 */
	extern DSP_STATUS CFG_GetCDVersion(OUT DWORD * pdwVersion);

/*
 *  ======== CFG_GetDevObject ========
 *  Purpose:
 *      Retrieve the Device Object handle for a given devnode.
 *  Parameters:
 *      hDevNode:       Platform's DevNode handle from which to retrieve value.
 *      pdwValue:       Ptr to location to store the value.
 *  Returns:
 *      DSP_SOK:                Success.
 *      CFG_E_INVALIDHDEVNODE:  hDevNode is invalid.
 *      CFG_E_INVALIDPOINTER:   phDevObject is invalid.
 *      CFG_E_RESOURCENOTAVAIL: The resource is not available.
 *  Requires:
 *      CFG initialized.
 *  Ensures:
 *      DSP_SOK:    *pdwValue is set to the retrieved DWORD.
 *      else:       *pdwValue is set to 0L.
 */
	extern DSP_STATUS CFG_GetDevObject(IN struct CFG_DEVNODE* hDevNode,
					   OUT DWORD * pdwValue);

/*
 *  ======== CFG_GetDSPResources ========
 *  Purpose:
 *      Get the DSP resources available to a given device.
 *  Parameters:
 *      hDevNode:       Handle to the DEVNODE who's resources we are querying.
 *      pDSPResTable:   Ptr to a location to store the DSP resource table.
 *  Returns:
 *      DSP_SOK:                On success.
 *      CFG_E_INVALIDHDEVNODE:  hDevNode is invalid.
 *      CFG_E_RESOURCENOTAVAIL: The DSP Resource information is not
 *                              available
 *  Requires:
 *      CFG initialized.
 *  Ensures:
 *      DSP_SOK:    pDSPResTable points to a filled table of resources allocated
 *                  for the specified WMD.
 */
	extern DSP_STATUS CFG_GetDSPResources(IN struct CFG_DEVNODE* hDevNode,
					      OUT struct CFG_DSPRES * pDSPResTable);


/*
 *  ======== CFG_GetExecFile ========
 *  Purpose:
 *      Retreive the default executable, if any, for this board.
 *  Parameters:
 *      hDevNode:       Handle to the DevNode who's WMD we are querying.
 *      cBufSize:       Size of buffer.
 *      pstrExecFile:   Ptr to character buf to hold ExecFile.
 *  Returns:
 *      DSP_SOK:                Success.
 *      CFG_E_INVALIDHDEVNODE:  hDevNode is invalid.
 *      CFG_E_INVALIDPOINTER:   pstrExecFile is invalid.
 *      CFG_E_RESOURCENOTAVAIL: The resource is not available.
 *  Requires:
 *      CFG initialized.
 *  Ensures:
 *      DSP_SOK:    Not more than cBufSize bytes were copied into pstrExecFile,
 *                  and *pstrExecFile contains default executable for this 
 *                  devnode.
 */
	extern DSP_STATUS CFG_GetExecFile(IN struct CFG_DEVNODE* hDevNode,
					  IN ULONG cBufSize,
					  OUT PSTR pstrExecFile);

/*
 *  ======== CFG_GetHostResources ========
 *  Purpose:
 *      Get the Host PC allocated resources assigned to a given device.
 *  Parameters:
 *      hDevNode:       Handle to the DEVNODE who's resources we are querying.
 *      pHostResTable:  Ptr to a location to store the host resource table.
 *  Returns:
 *      DSP_SOK:                On success.
 *      CFG_E_INVALIDPOINTER:   pHostResTable is invalid.
 *      CFG_E_INVALIDHDEVNODE:  hDevNode is invalid.
 *      CFG_E_RESOURCENOTAVAIL: The resource is not available.
 *  Requires:
 *      CFG initialized.
 *  Ensures:
 *      DSP_SOK:    pHostResTable points to a filled table of resources 
 *                  allocated for the specified WMD.
 *
 */
	extern DSP_STATUS CFG_GetHostResources(IN struct CFG_DEVNODE* hDevNode,
					       OUT struct CFG_HOSTRES * pHostResTable);

/*
 *  ======== CFG_GetObject ========
 *  Purpose:
 *      Retrieve the Driver Object handle From the Registry
 *  Parameters:
 *      pdwValue:   Ptr to location to store the value.
 *      dwType      Type of Object to Get
 *  Returns:
 *      DSP_SOK:    Success.
 *  Requires:
 *      CFG initialized.
 *  Ensures:
 *      DSP_SOK:    *pdwValue is set to the retrieved DWORD(non-Zero).
 *      else:       *pdwValue is set to 0L.
 */
	extern DSP_STATUS CFG_GetObject(OUT DWORD * pdwValue, DWORD dwType);

/*
 *  ======== CFG_GetPerfValue ========
 *  Purpose:
 *      Retrieve a flag indicating whether PERF should log statistics for the
 *      PM class driver.
 *  Parameters:
 *      pfEnablePerf:   Location to store flag.  0 indicates the key was
 *                      not found, or had a zero value.  A nonzero value
 *                      means the key was found and had a nonzero value.
 *  Returns:
 *  Requires:
 *      pfEnablePerf != NULL;
 *  Ensures:
 */
	extern VOID CFG_GetPerfValue(OUT bool * pfEnablePerf);

/*
 *  ======== CFG_GetWMDFileName ========
 *  Purpose:
 *    Get the mini-driver file name for a given device.
 *  Parameters:
 *      hDevNode:       Handle to the DevNode who's WMD we are querying.
 *      cBufSize:       Size of buffer.
 *      pWMDFileName:   Ptr to a character buffer to hold the WMD filename.
 *  Returns:
 *      DSP_SOK:                On success.
 *      CFG_E_INVALIDHDEVNODE:  hDevNode is invalid.
 *      CFG_E_RESOURCENOTAVAIL: The filename is not available.
 *  Requires:
 *      CFG initialized.
 *  Ensures:
 *      DSP_SOK:        Not more than cBufSize bytes were copied
 *                      into pWMDFileName.
 *
 */
	extern DSP_STATUS CFG_GetWMDFileName(IN struct CFG_DEVNODE* hDevNode,
					     IN ULONG cBufSize,
					     OUT PSTR pWMDFileName);

/*
 *  ======== CFG_GetZLFile ========
 *  Purpose:
 *      Retreive the ZLFile, if any, for this board.
 *  Parameters:
 *      hDevNode:       Handle to the DevNode who's WMD we are querying.
 *      cBufSize:       Size of buffer.
 *      pstrZLFileName: Ptr to character buf to hold ZLFileName.
 *  Returns:
 *      DSP_SOK:                Success.
 *      CFG_E_INVALIDPOINTER:   pstrZLFileName is invalid.
 *      CFG_E_INVALIDHDEVNODE:  hDevNode is invalid.
 *      CFG_E_RESOURCENOTAVAIL: couldn't find the ZLFileName.
 *  Requires:
 *      CFG initialized.
 *  Ensures:
 *      DSP_SOK:    Not more than cBufSize bytes were copied into 
 *                  pstrZLFileName, and *pstrZLFileName contains ZLFileName 
 *                  for this devnode.
 */
	extern DSP_STATUS CFG_GetZLFile(IN struct CFG_DEVNODE* hDevNode,
					IN ULONG cBufSize,
					OUT PSTR pstrZLFileName);

/*
 *  ======== CFG_Init ========
 *  Purpose:
 *      Initialize the CFG module's private state.
 *  Parameters:
 *  Returns:
 *      TRUE if initialized; FALSE if error occured.
 *  Requires:
 *  Ensures:
 *      A requirement for each of the other public CFG functions.
 */
	extern bool CFG_Init();

/*
 *  ======== CFG_SetDevObject ========
 *  Purpose:
 *      Store the Device Object handle for a given devnode.
 *  Parameters:
 *      hDevNode:   Platform's DevNode handle we are storing value with.
 *      dwValue:    Arbitrary value to store.
 *  Returns:
 *      DSP_SOK:                Success.
 *      CFG_E_INVALIDHDEVNODE:  hDevNode is invalid.
 *      DSP_EFAIL:              Internal Error.
 *  Requires:
 *      CFG initialized.
 *  Ensures:
 *      DSP_SOK:    The Private DWORD was successfully set.
 */
	extern DSP_STATUS CFG_SetDevObject(IN struct CFG_DEVNODE* hDevNode,
					   IN DWORD dwValue);

/*
 *  ======== CFG_SetDrvObject ========
 *  Purpose:
 *      Store the Driver Object handle.
 *  Parameters:
 *      dwValue:        Arbitrary value to store.
 *      dwType          Type of Object to Store
 *  Returns:
 *      DSP_SOK:        Success.
 *      DSP_EFAIL:      Internal Error.
 *  Requires:
 *      CFG initialized.
 *  Ensures:
 *      DSP_SOK:        The Private DWORD was successfully set.
 */
	extern DSP_STATUS CFG_SetObject(IN DWORD dwValue, IN DWORD dwType);

	extern DSP_STATUS CFG_GetC55Procs(OUT DWORD * numProcs);
#ifdef __cplusplus
}
#endif
#endif				/* CFG_ */
