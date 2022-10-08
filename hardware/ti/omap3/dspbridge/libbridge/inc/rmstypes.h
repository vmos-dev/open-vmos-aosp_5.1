/*
 * dspbridge/mpu_api/inc/rmstypes.h
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
 *  ======== rmstypes.h ========
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
