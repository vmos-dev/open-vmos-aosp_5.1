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
 *  ======== dbdcddef.h ========
 *  DSP-BIOS Bridge driver support functions for TI OMAP processors.
 *  Description:
 *      DCD (DSP/BIOS Bridge Configuration Database) constants and types.
 *
 *! Revision History:
 *! ================
 *! 03-Dec-2003 map Moved and renamed DCD_OBJTYPE to DSP_DCDOBJTYPE in dbdefs.h
 *! 05-Dec-2002 map Added DCD_CREATELIBTYPE, DCD_EXECUTELIBTYPE, 
 					DCD_DELETELIBTYPE
 *! 24-Feb-2003 kc  Updated REG entry names to DspBridge.
 *! 22-Nov-2002 gp  Cleaned up comments, formatting.
 *! 05-Aug-2002 jeh Added DCD_REGISTERFXN.
 *! 19-Apr-2002 jeh Added DCD_LIBRARYTYPE to DCD_OBJTYPE, dynamic load
 *!                 properties to DCD_NODEPROPS.
 *! 29-Jul-2001 ag  Added extended procObj.
 *! 13-Feb-2001 kc: Named changed from dcdbsdef.h dbdcddef.h.
 *! 12-Dec-2000 jeh Added DAIS iAlg name to DCD_NODEPROPS.
 *! 30-Oct-2000 kc: Added #defines for DCD_AutoRegister function.
 *! 05-Sep-2000 jeh Added DCD_NODEPROPS.
 *! 12-Aug-2000 kc: Incoroporated the use of types defined in <dspdefs.h>.
 *! 29-Jul-2000 kc: Created.
 */

#ifndef DBDCDDEF_
#define DBDCDDEF_

#ifdef __cplusplus
extern "C" {
#endif

#include <dbdefs.h>
#include <mgrpriv.h>		/* for MGR_PROCESSOREXTINFO */

/*
 *  The following defines are critical elements for the DCD module:
 *
 * - DCD_REGKEY enables DCD functions to locate registered DCD objects.
 * - DCD_REGISTER_SECTION identifies the COFF section where the UUID of
 *   registered DCD objects are stored.
 */
#define DCD_REGKEY              "Software\\TexasInstruments\\DspBridge\\DCD"
#define DCD_REGISTER_SECTION    ".dcd_register"

/* DCD Manager Object */
	struct DCD_MANAGER;
	/*typedef struct DCD_MANAGER *DCD_HMANAGER;*/

/* DCD Node Properties */
	struct DCD_NODEPROPS {
		struct DSP_NDBPROPS ndbProps;
		UINT uMsgSegid;
		UINT uMsgNotifyType;
		PSTR pstrCreatePhaseFxn;
		PSTR pstrDeletePhaseFxn;
		PSTR pstrExecutePhaseFxn;
		PSTR pstrIAlgName;

		/* Dynamic load properties */
		USHORT usLoadType;	/* Static, dynamic, overlay */
		ULONG ulDataMemSegMask;	/* Data memory requirements */
		ULONG ulCodeMemSegMask;	/* Code memory requirements */
	} ;

/* DCD Generic Object Type */
	struct DCD_GENERICOBJ {
		union dcdObjUnion {
			struct DCD_NODEPROPS nodeObj;	/* node object. */
			struct DSP_PROCESSORINFO procObj;	/* processor object. */
			/* extended proc object (private) */
			struct MGR_PROCESSOREXTINFO extProcObj;
		} objData;
	} ;

/* DCD Internal Callback Type */
	typedef DSP_STATUS(CDECL * DCD_REGISTERFXN) (IN struct DSP_UUID * pUuid,
						     IN DSP_DCDOBJTYPE objType,
						     IN PVOID handle);

	typedef DSP_STATUS(CDECL * DCD_UNREGISTERFXN) (IN struct DSP_UUID * pUuid,
						       IN DSP_DCDOBJTYPE
						       objType);

#ifdef __cplusplus
}
#endif
#endif				/* DBDCDDEF_ */

