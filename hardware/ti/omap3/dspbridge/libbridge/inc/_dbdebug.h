/*
 * dspbridge/mpu_api/inc/_dbdebug.h
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
 *  ======== _dbdebug.h ========
 *  Description:
 *      This is private header file to define the debug zones. This is
 *      WinCE Specific.
 *
 *  Notes:
 *
 *! Revision History:
 *! ================
 *! 29-Nov-2000 rr: Name changed to _ddspdebug.h and this header changed to
 *!                 TISB Coding standard.
 */

#ifndef _DBDEBUG_
#define _DBDEBUG_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef LINUX

/* Enable/Disable user API print messages in Linux */
#define DSPAPI_ZONE_INIT         0x0
#define DSPAPI_ZONE_MGR          0x1
#define DSPAPI_ZONE_PROC         0x2
#define DSPAPI_ZONE_NODE         0x3
#define DSPAPI_ZONE_STREAM       0x4
#define DSPAPI_ZONE_TEST         0x5
#define DSPAPI_ZONE_FUNCTION     0xd
#define DSPAPI_ZONE_WARNING      0xe
#define DSPAPI_ZONE_ERROR        0xf
#define DSPAPI_DEBUG_NONE        0xff

/* statically configure debug level */
#ifdef DDSP_DEBUG_PRODUCT
#define DSPAPI_DEBUG_LEVEL DSPAPI_ZONE_WARNING
#else				/* default debug level */
#define DSPAPI_DEBUG_LEVEL DSPAPI_ZONE_WARNING
#endif

#define DEBUGMSG(x,y) if(x >= DSPAPI_DEBUG_LEVEL) printf(y)

#else				/* ifdef LINUX */

/*
 * DEBUG macro support.
 */
#ifdef DEBUG

#define DSPAPI_ZONE_ID_INIT         0x0
#define DSPAPI_ZONE_ID_MGR          0x1
#define DSPAPI_ZONE_ID_PROC         0x2
#define DSPAPI_ZONE_ID_NODE         0x3
#define DSPAPI_ZONE_ID_STREAM       0x4
#define DSPAPI_ZONE_ID_TEST         0x5
#define DSPAPI_ZONE_ID_FUNCTION     0xd
#define DSPAPI_ZONE_ID_WARNING      0xe
#define DSPAPI_ZONE_ID_ERROR        0xf

#define DSPAPI_ZONE_MASK_INIT       ( 0x1 << DSPAPI_ZONE_ID_INIT )
#define DSPAPI_ZONE_MASK_MGR        ( 0x1 << DSPAPI_ZONE_ID_MGR )
#define DSPAPI_ZONE_MASK_PROC       ( 0x1 << DSPAPI_ZONE_ID_PROC )
#define DSPAPI_ZONE_MASK_NODE       ( 0x1 << DSPAPI_ZONE_ID_NODE )
#define DSPAPI_ZONE_MASK_STREAM     ( 0x1 << DSPAPI_ZONE_ID_STREAM )
#define DSPAPI_ZONE_MASK_TEST       ( 0x1 << DSPAPI_ZONE_ID_TEST )
#define DSPAPI_ZONE_MASK_FUNCTION   ( 0x1 << DSPAPI_ZONE_ID_FUNCTION )
#define DSPAPI_ZONE_MASK_WARNING    ( 0x1 << DSPAPI_ZONE_ID_WARNING )
#define DSPAPI_ZONE_MASK_ERROR      ( 0x1 << DSPAPI_ZONE_ID_ERROR )

#define DSPAPI_ZONE_INIT            DEBUGZONE( DSPAPI_ZONE_ID_INIT )
#define DSPAPI_ZONE_MGR             DEBUGZONE( DSPAPI_ZONE_ID_MGR )
#define DSPAPI_ZONE_PROC            DEBUGZONE( DSPAPI_ZONE_ID_PROC )
#define DSPAPI_ZONE_NODE            DEBUGZONE( DSPAPI_ZONE_ID_NODE )
#define DSPAPI_ZONE_STREAM          DEBUGZONE( DSPAPI_ZONE_ID_STREAM )
#define DSPAPI_ZONE_TEST            DEBUGZONE( DSPAPI_ZONE_ID_TEST )
#define DSPAPI_ZONE_FUNCTION        DEBUGZONE( DSPAPI_ZONE_ID_FUNCTION )
#define DSPAPI_ZONE_WARNING         DEBUGZONE( DSPAPI_ZONE_ID_WARNING )
#define DSPAPI_ZONE_ERROR           DEBUGZONE( DSPAPI_ZONE_ID_ERROR )

#endif

#endif				/* ifdef LINUX */

#ifdef __cplusplus
}
#endif
#endif				/* _DBDEBUG_ */
