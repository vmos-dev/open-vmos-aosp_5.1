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
 *  ======== devdefs.h ========
 *  DSP-BIOS Bridge driver support functions for TI OMAP processors.
 *  Purpose:
 *      Definition of common include typedef between wmd.h and dev.h. Required
 *      to break circular dependency between WMD and DEV include files.
 * 
 *! Revision History:
 *! ================
 *! 12-Nov-1996 gp: Renamed from dev1.h.
 *! 30-May-1996 gp: Broke out from dev.h
 */

#ifndef DEVDEFS_
#define DEVDEFS_

#ifdef __cplusplus
extern "C" {
#endif

/* WCD Device Object */
	struct DEV_OBJECT;
	/*typedef struct DEV_OBJECT *DEV_HOBJECT;*/

#ifdef __cplusplus
}
#endif
#endif				/* DEVDEFS_ */
