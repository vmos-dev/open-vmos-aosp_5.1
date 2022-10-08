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
 *  ======== uuidutil.h ========
 *  DSP-BIOS Bridge driver support functions for TI OMAP processors.
 *  Description:
 *      This file contains the specification of UUID helper functions.
 *
 *! Revision History
 *! ================
 *! 09-Nov-2000 kc: Modified description of UUID utility functions.
 *! 29-Sep-2000 kc: Appended "UUID_" prefix to UUID helper functions.
 *! 10-Aug-2000 kc: Created.
 *!
 */

#ifndef UUIDUTIL_
#define UUIDUTIL_

#ifdef __cplusplus
extern "C" {
#endif

#define MAXUUIDLEN  37

/*
 *  ======== UUID_UuidToString ========
 *  Purpose:
 *      Converts a DSP_UUID to an ANSI string.
 *  Parameters:
 *      pUuid:      Pointer to a DSP_UUID object.
 *      pszUuid:    Pointer to a buffer to receive a NULL-terminated UUID 
 *                  string.
 *      size:	    Maximum size of the pszUuid string.
 *  Returns:
 *  Requires:
 *      pUuid & pszUuid are non-NULL values.
 *  Ensures:
 *      Lenghth of pszUuid is less than MAXUUIDLEN.
 *  Details:
 *      UUID string limit currently set at MAXUUIDLEN.
 */
	VOID UUID_UuidToString(IN struct DSP_UUID * pUuid, OUT CHAR * pszUuid,
			       INT size);

/*
 *  ======== UUID_UuidFromString ========
 *  Purpose:
 *      Converts an ANSI string to a DSP_UUID.
 *  Parameters:
 *      pszUuid:    Pointer to a string that represents a DSP_UUID object.
 *      pUuid:      Pointer to a DSP_UUID object.
 *  Returns:
 *  Requires:
 *      pUuid & pszUuid are non-NULL values.
 *  Ensures:
 *  Details:
 *      We assume the string representation of a UUID has the following format:
 *      "12345678_1234_1234_1234_123456789abc".
 */
	extern VOID UUID_UuidFromString(IN CHAR * pszUuid,
					OUT struct DSP_UUID * pUuid);

#ifdef __cplusplus
}
#endif
#endif				/* UUIDUTIL_ */
