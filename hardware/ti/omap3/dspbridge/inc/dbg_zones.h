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
 *  ======== dbg_zones.h ========
 *  DSP-BIOS Bridge driver support functions for TI OMAP processors.
 *  Purpose:
 *      Common debug zone definitions used for DDsp tracing. Currently the GT
 *      module uses a single debug zone to output debug messages.
 *
 

 *
 *  Public Functions:
 *
 *  Notes:
 *      Need to call DBG_INSTANTIATE_ZONES(initialZones, modname ,opt1, opt2)
 *      before using.
 *
 *      - initialZone(s) should typically be 0x0001 (GTTRC) for GT.
 *      - modname is a string such as "DDSP driver"
 *      - opt1 and opt2 are user defined zones e.g. "Reg0 Write" *
 *
 *! Revision History:
 *! ================
 *! 03-Feb-2000 rr: DBGPARAM fields changed.
 *! 02-Dec-1999 rr: DBG_SetGT define changed(for building retail and Debug)
 *! 01-Oct-1999 ag: Removed #include <windows.h>
 *! 26-Aug-1999 ag: Created.
 *!
 */

#ifndef _DBG_ZONES_H_
#define _DBG_ZONES_H_

#ifndef LINUX			/* No DEBUGZONE in Linux */

#include <dbdefs.h>

#include <dspapi.h>		/* Defines used by this header */

#define DBG_ZONE_GTTRC              DEBUGZONE(0)
/* The following zones are undefined */
#define DBG_ZONE_XXX1               DEBUGZONE(1)
#define DBG_ZONE_XXX2               DEBUGZONE(2)
#define DBG_ZONE_XXX3               DEBUGZONE(3)
#define DBG_ZONE_XXX4               DEBUGZONE(4)
#define DBG_ZONE_XXX5               DEBUGZONE(5)
#define DBG_ZONE_XXX6               DEBUGZONE(6)
#define DBG_ZONE_XXX7               DEBUGZONE(7)
#define DBG_ZONE_XXX8               DEBUGZONE(8)
#define DBG_ZONE_XXX9               DEBUGZONE(9)
#define DBG_ZONE_XXX10              DEBUGZONE(10)
#define DBG_ZONE_XXX11              DEBUGZONE(11)
#define DBG_ZONE_XXX12              DEBUGZONE(12)
#define DBG_ZONE_XXX13              DEBUGZONE(13)
//
// The next two are user defined
// #define DBG_ZONE_                 DEBUGZONE(14)
// #define DBG_ZONE_                 DEBUGZONE(15)
#define DBG_INSTANTIATE_ZONES(initialZones,modname,opt1,opt2) \
    DBGPARAM dpCurSettings =                \
{                                           \
    TEXT(modname),                          \
    {                                       \
    TEXT("FUNCTION"),           /* 0  */    \
    TEXT("CLASS1"),             /* 1  */    \
    TEXT("CLASS2"),             /* 2  */    \
    TEXT("CLASS3"),             /* 3  */    \
    TEXT("CLASS4"),             /* 4  */    \
    TEXT("CLASS5"),             /* 5  */    \
    TEXT("SERVICES ERRORS"),	/* 6  */    \
    TEXT("CRITICAL ERRORS"),    /* 7  */    \
    TEXT("Unknown"),            /* 8  */    \
    TEXT("Unknown"),            /* 9  */    \
    TEXT("Unknown"),            /* 10 */    \
    TEXT("Unknown"),            /* 11 */    \
    TEXT("Unknown"),            /* 12 */    \
    TEXT("Unknown"),            /* 13 */    \
    TEXT(opt1),                 /* 14 */    \
    TEXT(opt2),                 /* 15 */    \
    },                                      \
    (initialZones)                          \
};
extern DSP_STATUS DBG_SetGT_DBG();
extern DBGPARAM dpCurSettings;

#define DBG_SetGT() DBG_SetGT_DBG()

#endif				/* ifndef LINUX */

#endif				/*ifndef _DBG_ZONES_H_ */
