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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "wfc_util_log.h"

#define WFC_UTIL_RANDOM_MAC_HEADER "001122"

void wfc_util_htoa(unsigned char *pHexaBuff, int szHexaBuff, char *pAsciiStringBuff, int szAsciiStringBuff)
{
	int i, j;
	char hex_table[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	                                 'A', 'B', 'C', 'D', 'E', 'F'};

	if ((szHexaBuff*2) > szAsciiStringBuff) {
		wfc_util_log_error("wfc_util_htoa : not enough buffer size(%d)", szAsciiStringBuff);
		return;
	}

	memset(pAsciiStringBuff, 0, szAsciiStringBuff);

	/* for (i = szHexaBuff-1, j = 0; i >= 0; i--, j += 2) { */
	for (i = 0, j = 0; i < szHexaBuff; i++, j += 2) {
		/*pAsciiStringBuff[j]     = hex_table[(pHexaBuff[i] >> 4) & 0x0F];
		*/
		pAsciiStringBuff[j]     = hex_table[pHexaBuff[i] >> 4];
		pAsciiStringBuff[j + 1] = hex_table[pHexaBuff[i] & 0x0F];
	}

	return;
}

void wfc_util_atoh(char *pAsciiString, int szAsciiString, unsigned char *pHexaBuff, int szHexaBuff)
{
	int i, pos;
	char temp;

	if ( 0!=(szAsciiString%2) || (szHexaBuff*2) < szAsciiString) {
		wfc_util_log_error("wfc_util_atoh : not enough buffer size(%d)", szHexaBuff);
		return;
	}

	memset(pHexaBuff, 0, szHexaBuff);

	for (i=0 ; i<szAsciiString ; i++) {

		/* pos = (szAsciiString - i - 1) / 2; */
		pos = i / 2;
		temp = pAsciiString[i];

		if (temp >= '0' && temp <= '9') {
			temp = temp - '0';
		} else if ( temp >= 'a' && temp <= 'f' ) {
			temp = temp - 'a' + 10;
		} else if ( temp >= 'A' && temp <= 'F' ) {
			temp = temp - 'A' + 10;
		} else {
			temp = 0;
		}

		if (0==i%2) {
			pHexaBuff[pos] = temp<<4;
		} else {
			pHexaBuff[pos] |= temp;
		}
	}

	return;
}

/*
 * wfc_util_is_random_mac
 *
 * return : it will return 1 if [mac_add] is same with WFC_UTIL_RANDOM_MAC_HEADER
 *          or will return 0 if not.
 */
int wfc_util_is_random_mac(char *mac_add)
{
	if(0 == strncmp(mac_add, WFC_UTIL_RANDOM_MAC_HEADER, 6)) {
		return 1;
	}

	return 0;
}

/*
 * wfc_util_random_mac
 *
 * Create random MAC address
 *
 * return : void
 */
void wfc_util_random_mac(unsigned char* mac_addr)
{
	unsigned long int rand_mac;

	if(NULL == mac_addr) {
		wfc_util_log_error("wfc_util_random_mac : buffer is NULL");
		return;
	}

	/* Create random MAC address: offset 3, 4 and 5 */
	srandom(time(NULL));
	rand_mac=random();

	#ifndef WFC_UTIL_RANDOM_MAC_HEADER
	mac_addr[0] = (unsigned char)0x00;
	mac_addr[1] = (unsigned char)0x11;
	mac_addr[2] = (unsigned char)0x22;
	#else  /* WFC_UTIL_RANDOM_MAC_HEADER */
	wfc_util_atoh(WFC_UTIL_RANDOM_MAC_HEADER, 6, mac_addr, 3);
	#endif /* WFC_UTIL_RANDOM_MAC_HEADER */
	mac_addr[3] = (unsigned char)rand_mac;
	mac_addr[4] = (unsigned char)(rand_mac >> 8);
	mac_addr[5] = (unsigned char)(rand_mac >> 16);

	return;
}
