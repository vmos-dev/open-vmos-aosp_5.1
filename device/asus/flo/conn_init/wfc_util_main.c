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
#include <getopt.h>
#include "wfc_util_fctrl.h"
#include "wfc_util_common.h"

#ifdef WLAN_CHIP_VERSION_WCNSS
#ifndef WFC_UTIL_CFG_FILE_NAME
#define WFC_UTIL_CFG_FILE_NAME          "./WCNSS_qcom_cfg.ini"
#endif
#ifndef WFC_UTIL_NV_BIN_FILE_NAME
#define WFC_UTIL_NV_BIN_FILE_NAME       "./WCNSS_qcom_wlan_nv.bin"
#endif
#else  /* WLAN_CHIP_VERSION_WCN1314 */
#ifndef WFC_UTIL_CFG_FILE_NAME
#define WFC_UTIL_CFG_FILE_NAME          "./WCN1314_qcom_cfg.ini"
#endif
#ifndef WFC_UTIL_NV_BIN_FILE_NAME
#define WFC_UTIL_NV_BIN_FILE_NAME       "./WCN1314_qcom_wlan_nv.bin"
#endif
#endif /* WLAN_CHIP_VERSION_XXXX */
#define WFC_UTIL_CFG_TAG_END_OF_CFG     "END"
#define WFC_UTIL_CFG_TAG_MAC_ADDRESS    "NetworkAddress="
#define WFC_UTIL_CFG_TAG_AP_MAC_ADDRESS "gAPMacAddr="
#define WFC_UTIL_CFG_TAG_END_OF_LINE    "\n"
#define WFC_UTIL_CFG_LENGHT_MAC         (6)
#define WFC_UTIL_CFG_LENGHT_MAC_STRING  (WFC_UTIL_CFG_LENGHT_MAC*2)

/*
 * persist/WCNSS_qcom_wlan_nv.bin
 *
 * typedef PACKED_PRE struct PACKED_POST
 * {
 *     //always ensure fields are aligned to 32-bit boundaries
 *     tANI_U16  productId;
 *     tANI_U8   productBands;
 *     tANI_U8   wlanNvRevId;
 *
 *     tANI_U8   numOfTxChains;
 *     tANI_U8   numOfRxChains;
 *     tANI_U8   macAddr[NV_FIELD_MAC_ADDR_SIZE];
 *     tANI_U8   mfgSN[NV_FIELD_MFG_SN_SIZE];
 * } sNvFields;
 */
#define WFC_UTIL_NV_BIN_HEADER_LENGTH    (4)
#define WFC_UTIL_NV_BIN_POS_PRODUCT_ID   (WFC_UTIL_NV_BIN_HEADER_LENGTH + 0)
#define WFC_UTIL_NV_BIN_POS_PRODUCT_BAND (WFC_UTIL_NV_BIN_HEADER_LENGTH + 2)
#define WFC_UTIL_NV_BIN_POS_MAC_ADDR     (WFC_UTIL_NV_BIN_HEADER_LENGTH + 6)

int main(int argc, char **argv)
{
	int ret = 0;
	char mac_add_buff[WFC_UTIL_CFG_LENGHT_MAC_STRING+1];
	unsigned char mac_add_buff_2[WFC_UTIL_CFG_LENGHT_MAC] = {0x88, 0xcd, 0xba, 0x0c, 0x90, 0x00};
	unsigned char mac_add_buff_3[WFC_UTIL_CFG_LENGHT_MAC] = {0x00, 0x90, 0x0c, 0xba, 0xcd, 0x88};

	printf("wfc_util_main is started\n");

	if(0 < wfc_util_fget_string(WFC_UTIL_CFG_FILE_NAME,
	                            WFC_UTIL_CFG_TAG_END_OF_CFG,
	                            WFC_UTIL_CFG_TAG_MAC_ADDRESS,
	                            WFC_UTIL_CFG_TAG_END_OF_LINE,
	                            mac_add_buff,
	                            WFC_UTIL_CFG_LENGHT_MAC_STRING+1)) {
		printf("wfc_util_main : %s%s\n", WFC_UTIL_CFG_TAG_MAC_ADDRESS, mac_add_buff);
	} else {
		printf("wfc_util_main : %s is not found\n", WFC_UTIL_CFG_TAG_MAC_ADDRESS);
	}

	wfc_util_fset_string(WFC_UTIL_CFG_FILE_NAME,
	                     WFC_UTIL_CFG_TAG_END_OF_CFG,
	                     WFC_UTIL_CFG_TAG_AP_MAC_ADDRESS,
	                     WFC_UTIL_CFG_TAG_END_OF_LINE,
	                     "00900cbacd88");

	wfc_util_fset_string(WFC_UTIL_CFG_FILE_NAME,
	                     WFC_UTIL_CFG_TAG_END_OF_CFG,
	                     WFC_UTIL_CFG_TAG_MAC_ADDRESS,
	                     WFC_UTIL_CFG_TAG_END_OF_LINE,
	                     "00900cbacd88");

	if(0 < wfc_util_fget_string(WFC_UTIL_CFG_FILE_NAME,
	                            WFC_UTIL_CFG_TAG_END_OF_CFG,
	                            WFC_UTIL_CFG_TAG_MAC_ADDRESS,
	                            WFC_UTIL_CFG_TAG_END_OF_LINE,
	                            mac_add_buff,
	                            WFC_UTIL_CFG_LENGHT_MAC_STRING+1)) {
		printf("wfc_util_main : %s%s\n", WFC_UTIL_CFG_TAG_MAC_ADDRESS, mac_add_buff);

		wfc_util_atoh(mac_add_buff, strlen(mac_add_buff), mac_add_buff_2, WFC_UTIL_CFG_LENGHT_MAC);
		printf("wfc_util_main : %s%02x:%02x:%02x:%02x:%02x:%02x\n",
		                        WFC_UTIL_CFG_TAG_MAC_ADDRESS,
		                        mac_add_buff_2[0], mac_add_buff_2[1], mac_add_buff_2[2],
		                        mac_add_buff_2[3], mac_add_buff_2[4], mac_add_buff_2[5]);

		wfc_util_htoa(mac_add_buff_2, WFC_UTIL_CFG_LENGHT_MAC, mac_add_buff, WFC_UTIL_CFG_LENGHT_MAC_STRING);
		printf("wfc_util_main : %s%s\n", WFC_UTIL_CFG_TAG_MAC_ADDRESS, mac_add_buff);

	} else {
		printf("wfc_util_main : %s is not found\n", WFC_UTIL_CFG_TAG_MAC_ADDRESS);
	}

	wfc_util_fset_buffer(WFC_UTIL_NV_BIN_FILE_NAME,
	                     WFC_UTIL_NV_BIN_POS_MAC_ADDR,
	                     mac_add_buff_3,
	                     WFC_UTIL_CFG_LENGHT_MAC);

	if(0 < wfc_util_fget_buffer(WFC_UTIL_NV_BIN_FILE_NAME,
	                            WFC_UTIL_NV_BIN_POS_MAC_ADDR,
	                            6,
	                            mac_add_buff_2,
	                            WFC_UTIL_CFG_LENGHT_MAC)) {
		printf("wfc_util_main : wfc_util_fget_buffer[%02x:%02x:%02x:%02x:%02x:%02x]\n",
		                        mac_add_buff_2[0], mac_add_buff_2[1], mac_add_buff_2[2],
		                        mac_add_buff_2[3], mac_add_buff_2[4], mac_add_buff_2[5]);
	} else {
		printf("wfc_util_main : %s is not found\n", WFC_UTIL_CFG_TAG_MAC_ADDRESS);
	}

	return ret;
}

