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

#ifndef __WFC_UTIL_COMMON_H__
#define __WFC_UTIL_COMMON_H__

/*
 * wfc_util_htoa
 *
 * return : void
 */
extern void wfc_util_htoa(unsigned char *pHexaBuff, int szHexaBuff, char *pAsciiStringBuff, int szAsciiStringBuff);

/*
 * wfc_util_atoh
 *
 * return : void
 */
extern void wfc_util_atoh(char *pAsciiString, int szAsciiString, unsigned char *pHexaBuff, int szHexaBuff);

/*
 * wfc_util_is_random_mac
 *
 * return : it will return 1 if [mac_add] is same with WFC_UTIL_RANDOM_MAC_HEADER
 *          or will return 0 if not.
 */
extern int wfc_util_is_random_mac(char *mac_add);

/*
 * wfc_util_random_mac
 *
 * Create random MAC address
 *
 * return : void
 */
void wfc_util_random_mac(unsigned char* mac_addr);

#endif
