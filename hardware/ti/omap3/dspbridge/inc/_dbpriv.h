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
 *  ======== _dbpriv.h ========
 *  DSP-BIOS Bridge driver support functions for TI OMAP processors.
 *  Description:
 *      Private header file for the 'Bridge API. Include global defines
 *      and macros.
 *
 *  Public Functions:
 *
 *! Revision History:
 *! ================
 *! 29-Nov-2000 rr: Name changed to _ddsppriv.h. Old types removed.
 *!                 New Macros for checking BadRead/WritePtr.
 *! 12-Apr-2000 ww: Created based on DirectDSP API specification, Version 0.6.
 */

#ifndef _DBPRIV_
#define _DBPRIV_

#ifdef __cplusplus
extern "C" {
#endif

#define DSP_ValidWritePtr(x , y)    ((x)==NULL)
#define DSP_ValidReadPtr(x, y)      ((x)==NULL)
#define DSP_MAX_PROCESSOR           3

#ifdef __cplusplus
}
#endif
#endif				/* _DBPRIV_ */
