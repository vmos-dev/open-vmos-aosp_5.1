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


#ifndef _GETSECTION_H_
#define _GETSECTION_H_

#ifndef _SIZE_T_DEFINED_	/* Android sets _SIZE_T_DEFINED_ on defining size_t */
typedef unsigned int size_t;
#define _SIZE_T_DEFINED_
#endif
#include "dynamic_loader.h"

#ifdef __cplusplus
extern "C" {			/* C-only version */
#endif

/*
 * Get Section Information
 *
 * This file provides an API add-on to the dynamic loader that allows the user
 * to query section information and extract section data from dynamic load
 * modules. 
 *
 * NOTE:
 * Functions in this API assume that the supplied Dynamic_Loader_Stream object
 * supports the set_file_posn method.
 */

	typedef void *DLOAD_module_info;	/* opaque handle for module information */

/*****************************************************************************
 * Procedure DLOAD_module_open
 *
 * Parameters:
 *  module  The input stream that supplies the module image
 *  syms    Host-side malloc/free and error reporting functions.
 *          Other methods are unused.
 *
 * Effect:
 *  Reads header information from a dynamic loader module using the specified
 * stream object, and returns a handle for the module information.  This
 * handle may be used in subsequent query calls to obtain information
 * contained in the module.
 *
 * Returns:
 *  NULL if an error is encountered, otherwise a module handle for use
 * in subsequent operations.
 *****************************************************************************/
	extern DLOAD_module_info DLOAD_module_open(struct Dynamic_Loader_Stream *
						   module,
						   struct Dynamic_Loader_Sym * syms);

/*****************************************************************************
 * Procedure DLOAD_GetSectionInfo
 *
 * Parameters:
 *  minfo       Handle from DLOAD_module_open for this module
 *  sectionName Pointer to the string name of the section desired
 *  sectionInfo Address of a section info structure pointer to be initialized
 *
 * Effect:
 *  Finds the specified section in the module information, and fills in
 * the provided LDR_SECTION_INFO structure.
 *
 * Returns:
 *  TRUE for success, FALSE for section not found
 *****************************************************************************/
	extern int DLOAD_GetSectionInfo(DLOAD_module_info minfo,
					const char *sectionName,
					const struct LDR_SECTION_INFO **
					const sectionInfo);

/*****************************************************************************
 * Procedure DLOAD_GetSectionNum
 *
 * Parameters:
 *  minfo       Handle from DLOAD_module_open for this module
 *  secn        Section number 0..
 *  sectionInfo Address of a section info structure pointer to be initialized
 *
 * Effect:
 *  Finds the secn'th section in the specified module, and fills in
 * the provided LDR_SECTION_INFO structure.  If there are less than "secn+1"
 * sections in the module, returns NULL.
 *
 * Returns:
 *  TRUE for success, FALSE for failure
 *****************************************************************************/
	extern int DLOAD_GetSectionNum(DLOAD_module_info minfo,
				       const unsigned secn,
				       const struct LDR_SECTION_INFO **
				       const sectionInfo);

/*****************************************************************************
 * Procedure DLOAD_RoundUpSectionSize
 *
 * Parameters:
 *  sectSize    The actual size of the section in target addressable units
 *
 * Effect:
 *  Rounds up the section size to the next multiple of 32 bits.
 *
 * Returns:
 *  The rounded-up section size.
 *****************************************************************************/
	extern size_t DLOAD_RoundUpSectionSize(LDR_ADDR sectSize);

/*****************************************************************************
 * Procedure DLOAD_GetSection
 *
 * Parameters:
 *  minfo       Handle from DLOAD_module_open for this module
 *  sectionInfo Pointer to a section info structure for the desired section
 *  sectionData Buffer to contain the section initialized data
 *
 * Effect:
 *  Copies the initialized data for the specified section into the
 * supplied buffer.
 *
 * Returns:
 *  TRUE for success, FALSE for section not found
 *****************************************************************************/
	extern int DLOAD_GetSection(DLOAD_module_info minfo,
				    const struct LDR_SECTION_INFO * sectionInfo,
				    void *sectionData);

/*****************************************************************************
 * Procedure DLOAD_module_close
 *
 * Parameters:
 *  minfo       Handle from DLOAD_module_open for this module
 *
 * Effect:
 *  Releases any storage associated with the module handle.  On return,
 * the module handle is invalid.
 *
 * Returns:
 *  Zero for success. On error, the number of errors detected is returned.
 * Individual errors are reported using syms->Error_Report(), where syms was
 * an argument to DLOAD_module_open
 *****************************************************************************/
	extern void DLOAD_module_close(DLOAD_module_info minfo);

#ifdef __cplusplus
}
#endif
#endif				/* _GETSECTION_H_ */
