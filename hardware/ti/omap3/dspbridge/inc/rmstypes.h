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
 *  ======== rmstypes.h ========
 *  DSP-BIOS Bridge driver support functions for TI OMAP processors.
 *
 *  DSP/BIOS Bridge Resource Manager Server shared data type definitions.
 *
 *! Revision History
 *! ================
 *! 06-Oct-2000 sg  Added LgFxn type.
 *! 05-Oct-2000 sg  Changed RMS_STATUS to LgUns.
 *! 31-Aug-2000 sg  Added RMS_DSPMSG.
 *! 25-Aug-2000 sg  Initial.
 */

#ifndef RMSTYPES_
#define RMSTYPES_

#ifdef _GPP_
/*
 *  GPP-side type definitions.
 */
typedef DWORD RMS_WORD;
typedef DWORD RMS_CHAR;
typedef DWORD RMS_STATUS;

#else				/* default to DSP-side */
/*
 *  DSP-side definitions.
 */
#include <std.h>
typedef LgUns RMS_WORD;
typedef Char RMS_CHAR;
typedef LgUns RMS_STATUS;
typedef LgUns(*LgFxn) ();	/* generic LgUns function type */

#endif

/* GPP<->DSP Message Structure: */
struct RMS_DSPMSG {
	RMS_WORD cmd;		/* Message code */
	RMS_WORD arg1;		/* First message argument */
	RMS_WORD arg2;		/* Second message argument */
};

#endif				/* RMSTYPES_ */
