/*
 * dspbridge/mpu_api/inc/dbdcd.h
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
 *  ======== dbdcd.h ========
 *  Description:
 *      Defines the DSP/BIOS Bridge Configuration Database (DCD) API.
 *
 *! Revision History
 *! ================
 *! 03-Dec-2003 map Changed DCD_OBJTYPE to DSP_DCDOBJTYPE
 *! 24-Feb-2003 kc  Updated DCD_AutoUnregister and DCD_GetObjects to simplify
 *!                 DCD implementation.
 *! 05-Aug-2002 jeh Added DCD_GetObjects().
 *! 11-Jul-2002 jeh Added DCD_GetDepLibs(), DCD_GetNumDepLibs().
 *! 22-Apr-2002 jeh Added DCD_GetLibraryName().
 *! 03-Apr-2001 sg  Changed error names to have DCD_E* format.
 *! 13-Feb-2001 kc  Name changed from dcdbs.h to dbdcd.h.
 *! 12-Dec-2000 kc  Added DCD_AutoUnregister.
 *! 09-Nov-2000 kc  Updated usage of DCD_EnumerateObject.
 *! 30-Oct-2000 kc  Added DCD_AutoRegister. Updated error DCD error codes.
 *! 29-Sep-2000 kc  Incorporated code review comments. See
 *!                 /src/reviews/dcd_review.txt.
 *! 26-Jul-2000 kc  Created.
 *!
 */

#ifndef DBDCD_
#define DBDCD_

#ifdef __cplusplus
extern "C" {
#endif

#include <dbdcddef.h>
#include <nldrdefs.h>

/*
 *  ======== DCD_AutoRegister ========
 *  Purpose:
 *      This function automatically registers DCD objects specified in a
 *      special COFF section called ".dcd_register"
 *  Parameters:
 *      hDcdMgr:                A DCD manager handle.
 *      pszCoffPath:            Pointer to name of COFF file containing DCD
 *                              objects to be registered.
 *  Returns:
 *      DSP_SOK:                Success.
 *      DSP_EDCDNOAUTOREGISTER: Unable to find auto-registration section.
 *      DSP_EDCDREADSECT:       Unable to read object code section.
 *      DSP_EDCDLOADBASE:       Unable to load code base.
 *      DSP_EHANDLE:            Invalid DCD_HMANAGER handle..
 *  Requires:
 *      DCD initialized.
 *  Ensures:
 *  Note:
 *      Due to the DCD database construction, it is essential for a DCD-enabled
 *      COFF file to contain the right COFF sections, especially
 *      ".dcd_register", which is used for auto registration.
 */
	extern DSP_STATUS DCD_AutoRegister(IN struct DCD_MANAGER* hDcdMgr,
					   IN CHAR * pszCoffPath);

/*
 *  ======== DCD_AutoUnregister ========
 *  Purpose:
 *      This function automatically unregisters DCD objects specified in a
 *      special COFF section called ".dcd_register"
 *  Parameters:
 *      hDcdMgr:                A DCD manager handle.
 *      pszCoffPath:            Pointer to name of COFF file containing
 *                              DCD objects to be unregistered.
 *  Returns:
 *      DSP_SOK:                Success.
 *      DSP_EDCDNOAUTOREGISTER: Unable to find auto-registration section.
 *      DSP_EDCDREADSECT:       Unable to read object code section.
 *      DSP_EDCDLOADBASE:       Unable to load code base.
 *      DSP_EHANDLE:            Invalid DCD_HMANAGER handle..
 *  Requires:
 *      DCD initialized.
 *  Ensures:
 *  Note:
 *      Due to the DCD database construction, it is essential for a DCD-enabled
 *      COFF file to contain the right COFF sections, especially
 *      ".dcd_register", which is used for auto unregistration.
 */
	extern DSP_STATUS DCD_AutoUnregister(IN struct DCD_MANAGER* hDcdMgr,
					     IN CHAR * pszCoffPath);

/*
 *  ======== DCD_CreateManager ========
 *  Purpose:
 *      This function creates a DCD module manager.
 *  Parameters:
 *      pszZlDllName:   Pointer to a DLL name string.
 *      phDcdMgr:       A pointer to a DCD manager handle.
 *  Returns:
 *      DSP_SOK:        Success.
 *      DSP_EMEMORY:    Unable to allocate memory for DCD manager handle.
 *      DSP_EFAIL:      General failure.
 *  Requires:
 *      DCD initialized.
 *      pszZlDllName is non-NULL.
 *      phDcdMgr is non-NULL.
 *  Ensures:
 *      A DCD manager handle is created.
 */
	extern DSP_STATUS DCD_CreateManager(IN CHAR * pszZlDllName,
					    OUT struct DCD_MANAGER* * phDcdMgr);

/*
 *  ======== DCD_DestroyManager ========
 *  Purpose:
 *      This function destroys a DCD module manager.
 *  Parameters:
 *      hDcdMgr:        A DCD manager handle.
 *  Returns:
 *      DSP_SOK:        Success.
 *      DSP_EHANDLE:    Invalid DCD manager handle.
 *  Requires:
 *      DCD initialized.
 *  Ensures:
 */
	extern DSP_STATUS DCD_DestroyManager(IN struct DCD_MANAGER* hDcdMgr);

/*
 *  ======== DCD_EnumerateObject ========
 *  Purpose:
 *      This function enumerates currently visible DSP/BIOS Bridge objects
 *      and returns the UUID and type of each enumerated object.
 *  Parameters:
 *      cIndex:             The object enumeration index.
 *      objType:            Type of object to enumerate.
 *      pUuid:              Pointer to a DSP_UUID object.
 *  Returns:
 *      DSP_SOK:            Success.
 *      DSP_EFAIL:          Unable to enumerate through the DCD database.
 *      DSP_SENUMCOMPLETE:  Enumeration completed. This is not an error code.
 *  Requires:
 *      DCD initialized.
 *      pUuid is a valid pointer.
 *  Ensures:
 *  Details:
 *      This function can be used in conjunction with DCD_GetObjectDef to
 *      retrieve object properties.
 */
	extern DSP_STATUS DCD_EnumerateObject(IN INT cIndex,
					      IN DSP_DCDOBJTYPE objType,
					      OUT struct DSP_UUID * pUuid);

/*
 *  ======== DCD_Exit ========
 *  Purpose:
 *      This function cleans up the DCD module.
 *  Parameters:
 *  Returns:
 *  Requires:
 *      DCD initialized.
 *  Ensures:
 */
	extern VOID DCD_Exit();

/*
 *  ======== DCD_GetDepLibs ========
 *  Purpose:
 *      Given the uuid of a library and size of array of uuids, this function
 *      fills the array with the uuids of all dependent libraries of the input
 *      library.
 *  Parameters:
 *      hDcdMgr: A DCD manager handle.
 *      pUuid: Pointer to a DSP_UUID for a library.
 *      numLibs: Size of uuid array (number of library uuids).
 *      pDepLibUuids: Array of dependent library uuids to be filled in.
 *      pPersistentDepLibs: Array indicating if corresponding lib is persistent.
 *      phase: phase to obtain correct input library
 *  Returns:
 *      DSP_SOK: Success.
 *      DSP_EMEMORY: Memory allocation failure.
 *      DSP_EDCDREADSECT: Failure to read section containing library info.
 *      DSP_EFAIL: General failure.
 *  Requires:
 *      DCD initialized.
 *      Valid hDcdMgr.
 *      pUuid != NULL
 *      pDepLibUuids != NULL.
 *  Ensures:
 */
	extern DSP_STATUS DCD_GetDepLibs(IN struct DCD_MANAGER* hDcdMgr,
					 IN struct DSP_UUID * pUuid,
					 USHORT numLibs,
					 OUT struct DSP_UUID * pDepLibUuids,
					 OUT bool * pPersistentDepLibs,
					 IN NLDR_PHASE phase);

/*
 *  ======== DCD_GetNumDepLibs ========
 *  Purpose:
 *      Given the uuid of a library, determine its number of dependent
 *      libraries.
 *  Parameters:
 *      hDcdMgr:        A DCD manager handle.
 *      pUuid:          Pointer to a DSP_UUID for a library.
 *      pNumLibs:       Size of uuid array (number of library uuids).
 *      pNumPersLibs:   number of persistent dependent library.
 *      phase:          Phase to obtain correct input library
 *  Returns:
 *      DSP_SOK: Success.
 *      DSP_EMEMORY: Memory allocation failure.
 *      DSP_EDCDREADSECT: Failure to read section containing library info.
 *      DSP_EFAIL: General failure.
 *  Requires:
 *      DCD initialized.
 *      Valid hDcdMgr.
 *      pUuid != NULL
 *      pNumLibs != NULL.
 *  Ensures:
 */
	extern DSP_STATUS DCD_GetNumDepLibs(IN struct DCD_MANAGER* hDcdMgr,
					    IN struct DSP_UUID * pUuid,
					    OUT USHORT * pNumLibs,
					    OUT USHORT * pNumPersLibs,
					    IN NLDR_PHASE phase);

/*
 *  ======== DCD_GetLibraryName ========
 *  Purpose:
 *      This function returns the name of a (dynamic) library for a given
 *      UUID.
 *  Parameters:
 *      hDcdMgr: A DCD manager handle.
 *      pUuid:          Pointer to a DSP_UUID that represents a unique DSP/BIOS
 *                      Bridge object.
 *      pstrLibName: Buffer to hold library name.
 *      pdwSize: Contains buffer size. Set to string size on output.
 *      phase:          Which phase to load
 *      fPhaseSplit:    Are phases in multiple libraries
 *  Returns:
 *      DSP_SOK: Success.
 *      DSP_EFAIL: General failure.
 *  Requires:
 *      DCD initialized.
 *      Valid hDcdMgr.
 *      pstrLibName != NULL.
 *      pUuid != NULL
 *      pdwSize != NULL.
 *  Ensures:
 */
	extern DSP_STATUS DCD_GetLibraryName(IN struct DCD_MANAGER* hDcdMgr,
					     IN struct DSP_UUID * pUuid,
					     IN OUT PSTR pstrLibName,
					     IN OUT DWORD * pdwSize,
					     IN NLDR_PHASE phase,
					     OUT bool * fPhaseSplit);

/*
 *  ======== DCD_GetObjectDef ========
 *  Purpose:
 *      This function returns the properties/attributes of a DSP/BIOS Bridge
 *      object.
 *  Parameters:
 *      hDcdMgr:            A DCD manager handle.
 *      pUuid:              Pointer to a DSP_UUID that represents a unique
 *                          DSP/BIOS Bridge object.
 *      objType:            The type of DSP/BIOS Bridge object to be
 *                          referenced (node, processor, etc).
 *      pObjDef:            Pointer to an object definition structure. A
 *                          union of various possible DCD object types.
 *  Returns:
 *      DSP_SOK: Success.
 *      DSP_EDCDPARSESECT:  Unable to parse content of object code section.
 *      DSP_EDCDREADSECT:   Unable to read object code section.
 *      DSP_EDCDGETSECT:    Unable to access object code section.
 *      DSP_EDCDLOADBASE:   Unable to load code base.
 *      DSP_EFAIL:          General failure.
 *      DSP_EHANDLE:        Invalid DCD_HMANAGER handle.
 *  Requires:
 *      DCD initialized.
 *      pObjUuid is non-NULL.
 *      pObjDef is non-NULL.
 *  Ensures:
 */
	extern DSP_STATUS DCD_GetObjectDef(IN struct DCD_MANAGER* hDcdMgr,
					   IN struct DSP_UUID * pObjUuid,
					   IN DSP_DCDOBJTYPE objType,
					   OUT struct DCD_GENERICOBJ *pObjDef);

/*
 *  ======== DCD_GetObjects ========
 *  Purpose:
 *      This function finds all DCD objects specified in a special
 *      COFF section called ".dcd_register", and for each object,
 *      call a "register" function.  The "register" function may perform
 *      various actions, such as 1) register nodes in the node database, 2)
 *      unregister nodes from the node database, and 3) add overlay nodes.
 *  Parameters:
 *      hDcdMgr:                A DCD manager handle.
 *      pszCoffPath:            Pointer to name of COFF file containing DCD
 *                              objects.
 *      registerFxn:            Callback fxn to be applied on each located
 *                              DCD object.
 *      handle:                 Handle to pass to callback.
 *  Returns:
 *      DSP_SOK:                Success.
 *      DSP_EDCDNOAUTOREGISTER: Unable to find .dcd_register section.
 *      DSP_EDCDREADSECT:       Unable to read object code section.
 *      DSP_EDCDLOADBASE:       Unable to load code base.
 *      DSP_EHANDLE:            Invalid DCD_HMANAGER handle..
 *  Requires:
 *      DCD initialized.
 *  Ensures:
 *  Note:
 *      Due to the DCD database construction, it is essential for a DCD-enabled
 *      COFF file to contain the right COFF sections, especially
 *      ".dcd_register", which is used for auto registration.
 */
	extern DSP_STATUS DCD_GetObjects(IN struct DCD_MANAGER* hDcdMgr,
					 IN CHAR * pszCoffPath,
					 DCD_REGISTERFXN registerFxn,
					 PVOID handle);

/*
 *  ======== DCD_Init ========
 *  Purpose:
 *      This function initializes DCD.
 *  Parameters:
 *  Returns:
 *      FALSE:  Initialization failed.
 *      TRUE:   Initialization succeeded.
 *  Requires:
 *  Ensures:
 *      DCD initialized.
 */
	extern bool DCD_Init();

/*
 *  ======== DCD_RegisterObject ========
 *  Purpose:
 *      This function registers a DSP/BIOS Bridge object in the DCD database.
 *  Parameters:
 *      pUuid:          Pointer to a DSP_UUID that identifies a DSP/BIOS
 *                      Bridge object.
 *      objType:        Type of object.
 *      pszPathName:    Path to the object's COFF file.
 *  Returns:
 *      DSP_SOK:        Success.
 *      DSP_EFAIL:      Failed to register object.
 *  Requires:
 *      DCD initialized.
 *      pUuid and szPathName are non-NULL values.
 *      objType is a valid type value.
 *  Ensures:
 */
	extern DSP_STATUS DCD_RegisterObject(IN struct DSP_UUID * pUuid,
					     IN DSP_DCDOBJTYPE objType,
					     IN CHAR * pszPathName);

/*
 *  ======== DCD_UnregisterObject ========
 *  Purpose:
 *      This function de-registers a valid DSP/BIOS Bridge object from the DCD
 *      database.
 *  Parameters:
 *      pUuid:      Pointer to a DSP_UUID that identifies a DSP/BIOS Bridge
 *                  object.
 *      objType:    Type of object.
 *  Returns:
 *      DSP_SOK:    Success.
 *      DSP_EFAIL:  Unable to de-register the specified object.
 *  Requires:
 *      DCD initialized.
 *      pUuid is a non-NULL value.
 *      objType is a valid type value.
 *  Ensures:
 */
	extern DSP_STATUS DCD_UnregisterObject(IN struct DSP_UUID * pUuid,
					       IN DSP_DCDOBJTYPE objType);

#ifdef __cplusplus
}
#endif
#endif				/* _DBDCD_H */
