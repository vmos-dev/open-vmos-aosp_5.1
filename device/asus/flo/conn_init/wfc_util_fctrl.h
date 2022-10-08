/*
 * Copyright 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __WFC_UTIL_FCTRL_H__
#define __WFC_UTIL_FCTRL_H__

/*
 * wfc_util_fset_buffer
 *
 * return : void
 */
void wfc_util_fset_buffer(char *pFileName, int positionStart, unsigned char *pNewValue, int newValueLength);

/*
 * wfc_util_fget_buffer
 *
 * return : it will return the length of the stored buffer value if procedure is success
 *          or will return 0 if not.
 */
extern int wfc_util_fget_buffer(char *pFileName, int positionStart, int lengthToRead, unsigned char *pValueBuff, int buffLength);

/*
 * wfc_util_fset_string
 *
 * The following format string will be added or updated to the file pFileName.
 * [pSTagString][pNewValueString][pETagString]
 *
 * pFileName       : file name and path
 * pEndOfCfg       : tag string to notify the end of configuration file
 * pSTagString     : tag string to notify purpose of the value
 * pETagString     : tag string to notify the end of the value
 * pNewValueString : string to set for pSTagString
 *
 * return : void
 */
extern void wfc_util_fset_string(char *pFileName, char *pEndOfCfg, char *pSTagString, char *pETagString, char *pNewValueString);

/*
 * wfc_util_fget_string
 *
 * Read value from the following format string in the file pFileName.
 * [pSTagString][string value to read][pETagString]
 *
 * pFileName        : file name and path
 * pEndOfCfg        : tag string to notify the end of configuration file
 * pSTagString      : tag string to notify purpose of the value
 * pETagString      : tag string to notify the end of the value
 * pValueStringBuff : string buffer to get string value
 * stringBuffLength : the length of pValueStringBuff
 *
 * return : it will return the length of the stored string value if procedure is success
 *          or will return 0 if not.
 */
extern int wfc_util_fget_string(char *pFileName, char *pEndOfCfg, char *pSTagString, char *pETagString, char *pValueStringBuff, int stringBuffLength);

/*
 * wfc_util_ffile_check
 *
 * check whether pDestFName file exist or not
 *
 * pFileName   : file name and path
 * access_mode : R_OK | W_OK | X_OK | F_OK
 *
 * return : it will return 0 if the file exist
 *          or will return -1 if not.
 */
extern int wfc_util_ffile_check(char *pDestFName, int access_mode);

/*
 * wfc_util_ffile_check_copy
 *
 * check whether pDestFName file exist if not it will copy from pSourceFName file
 *
 * return : it will return 0 if procedure is success
 *          or will return -1 if not.
 */
extern int wfc_util_ffile_check_copy(char *pDestFName, char *pSourceFName, mode_t mode,  uid_t uID, gid_t gID);

#endif
