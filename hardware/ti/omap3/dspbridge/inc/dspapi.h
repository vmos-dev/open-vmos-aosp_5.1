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
 *  ======== dspapi.h ========
 *  DSP-BIOS Bridge driver support functions for TI OMAP processors.
 *  Purpose:
 *      Defines function type modifiers used in all DSPSYS public header 
 *      files.
 * 
 *  Notes:
 *      Provides __stdcall (required by VB 4.0) and __declspec(dllimport) 
 *      function modifiers for fast dyna-linking.
 *
 *! Revision History:
 *! =================
 *! 23-Dec-1997 cr: Added WBKERNEL_API definition.
 *! 11-Oct-1996 gp: Created.
 */

#ifndef DSPAPI_
#define DSPAPI_

/* Define API decoration for direct importing of DLL references. */
#if !defined(_DSPSYSDLL32_)
#define DSPAPIDLL __declspec(dllimport)
#else
#define DSPAPIDLL
#endif

/* Full export modifier: */
#define DSPAPI DSPAPIDLL DSP_STATUS WINAPI

/* Explicitly define class driver calling conventions */
#define WBKERNEL_API CDECL

#endif				/* DSPAPI_ */
