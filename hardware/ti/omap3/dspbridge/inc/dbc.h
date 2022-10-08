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
 *  ======== dbc.h ========
 *  DSP-BIOS Bridge driver support functions for TI OMAP processors.
 *  Purpose:
 *      "Design by Contract" programming macros.
 * 
 *  Public Functions:
 *      DBC_Assert
 *      DBC_Require
 *      DBC_Ensure
 *
 *  Notes:
 *      Requires that the GT->ERROR function has been defaulted to a valid 
 *      error handler for the given execution environment.
 *
 *      Does not require that GT_init() be called.
 *
 *! Revision History:
 *! ================
 *! 11-Aug-2000 ag: Removed include <std.h>
 *! 22-Apr-1996 gp: Created. 
 */

#ifndef DBC_
#define DBC_

#ifdef __cplusplus
extern "C" {
#endif

/* Assertion Macros: */
#if GT_TRACE

#include <gt.h>

#define DBC_Assert( exp ) \
    if (!(exp)) \
            (*GT->ERRORFXN)("%s, line %d: Assertion (" #exp ") failed.\n", \
            __FILE__, __LINE__)
#define DBC_Require DBC_Assert	/* Function Precondition.  */
#define DBC_Ensure  DBC_Assert	/* Function Postcondition. */

#else

#define DBC_Assert(exp)
#define DBC_Require(exp)
#define DBC_Ensure(exp)

#endif				/* DEBUG */

#ifdef __cplusplus
}
#endif
#endif				/* DBC_ */
