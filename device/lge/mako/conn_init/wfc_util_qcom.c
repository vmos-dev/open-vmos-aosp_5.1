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

#ifdef CONFIG_LGE_WLAN_QCOM_PATCH

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wfc_util_log.h"
#include "wfc_util_fctrl.h"
#include "wfc_util_common.h"

#include "private/android_filesystem_config.h"

#define WFC_UTIL_FEAUTRE_COPY_NV_BIN

#ifdef WLAN_CHIP_VERSION_WCNSS
#ifndef WFC_UTIL_CFG_FILE_NAME
#define WFC_UTIL_CFG_FILE_NAME          "/data/misc/wifi/WCNSS_qcom_cfg.ini"
#endif
#ifndef WFC_UTIL_CFG_TEMPFILE_NAME
#define WFC_UTIL_CFG_TEMPFILE_NAME      "/system/etc/wifi/WCNSS_qcom_cfg.ini"
#endif
#else  /* WLAN_CHIP_VERSION_WCN1314 */
#ifndef WFC_UTIL_CFG_FILE_NAME
#define WFC_UTIL_CFG_FILE_NAME          "/data/misc/wifi/WCN1314_qcom_cfg.ini"
#endif
#ifndef WFC_UTIL_CFG_TEMPFILE_NAME
#define WFC_UTIL_CFG_TEMPFILE_NAME      "/system/etc/wifi/WCN1314_qcom_cfg.ini"
#endif
#endif /* WLAN_CHIP_VERSION_XXXX */

#ifdef WFC_UTIL_FEAUTRE_COPY_NV_BIN
#ifdef WLAN_CHIP_VERSION_WCNSS
#ifndef WFC_UTIL_NV_BIN_TEMPFILE_NAME
#define WFC_UTIL_NV_BIN_TEMPFILE_NAME   "/system/etc/wifi/WCNSS_qcom_wlan_nv.bin"
#endif
#ifndef WFC_UTIL_NV_BIN_FILE_NAME
#define WFC_UTIL_NV_BIN_FILE_NAME       "/data/misc/wifi/WCNSS_qcom_wlan_nv.bin"
#endif
#else  /* WLAN_CHIP_VERSION_WCN1314 */
#ifndef WFC_UTIL_NV_BIN_TEMPFILE_NAME
#define WFC_UTIL_NV_BIN_TEMPFILE_NAME   "/persist/WCN1314_qcom_wlan_nv.bin"
#endif
#ifndef WFC_UTIL_NV_BIN_FILE_NAME
#define WFC_UTIL_NV_BIN_FILE_NAME       "/data/misc/wifi/WCN1314_qcom_wlan_nv.bin"
#endif
#endif /* WLAN_CHIP_VERSION_XXXX */
#else  /* WFC_UTIL_FEAUTRE_COPY_NV_BIN */
#ifndef WFC_UTIL_NV_BIN_FILE_NAME
#ifdef WLAN_CHIP_VERSION_WCNSS
#define WFC_UTIL_NV_BIN_FILE_NAME       "/persist/WCNSS_qcom_wlan_nv.bin"
#else  /* WLAN_CHIP_VERSION_WCN1314 */
#define WFC_UTIL_NV_BIN_FILE_NAME       "/persist/WCN1314_qcom_wlan_nv.bin"
#endif /* WLAN_CHIP_VERSION_XXXX */
#endif
#endif /* WFC_UTIL_FEAUTRE_COPY_NV_BIN */

#define WFC_UTIL_CFG_TAG_END_OF_CFG     "END"
/*
 * Station Mode MAC Address
 */
#ifdef WLAN_CHIP_VERSION_WCNSS
#define WFC_UTIL_CFG_TAG_MAC_ADDRESS    "Intf0MacAddress="
#else  /* WLAN_CHIP_VERSION_WCN1314 */
#define WFC_UTIL_CFG_TAG_MAC_ADDRESS    "NetworkAddress="
#endif /* WLAN_CHIP_VERSION_XXXX */
/*
 * AP Mode MAC Address
 */
#define WFC_UTIL_CFG_TAG_AP_MAC_ADDRESS "gAPMacAddr="

/*
 * Idle Mode Power Save enable/disable for OTA test
 */
#define WFC_UTIL_CFG_TAG_IDLE_MODE_POWER_SAVE "gEnableImps="

/*
 * Beacon Mode Power Save enable/disable for OTA test
 */
#define WFC_UTIL_CFG_TAG_POWER_SAVE "gEnableBmps="

/*
 * L2 roaming on/off for OTA test
 */
#define WFC_UTIL_CFG_TAG_L2Roaming "gEnableHandoff="

/*
 * Heartbeat24 changing for OtA test
 */
#define WFC_UTIL_CFG_TAG_HEARTBEAT24 "gHeartbeat24="

/*
 * TAG for end of line
 */
#define WFC_UTIL_CFG_TAG_END_OF_LINE    "\n"

#define WFC_UTIL_CFG_LENGHT_MAC         (6)
#define WFC_UTIL_CFG_LENGHT_MAC_STRING  (WFC_UTIL_CFG_LENGHT_MAC*2)

/*
 * persist/WCNSS_qcom_wlan_nv.bin
 *
 * NV validity bitmap (4 bytes)
 * {
 *     Bit 0 - Regulatory domain tables
 *     Bit 1 - Fields(including product ID, product bands, number of Tx/Rx chains, MAC address, manufacturing board number)
 *     Bit 2 - Optimal power per rate table
 *     Bit 3 - Default regulatory domain and country code
 *     Bit 4:31 - Reserved; always 0
 * }
 *
 * typedef PACKED_PRE struct PACKED_POST
 * {
 *     //always ensure fields are aligned to 32-bit boundaries
 *     tANI_U16  productId;
 *     tANI_U8   productBands;        //0: 0.4 GHz, 1: 2.4+5.0 GHz, 2: 5.0 GHz
 *     tANI_U8   wlanNvRevId;         //0: WCN1312, 1: WCN1314, 2: PRIMA
 *
 *     tANI_U8   numOfTxChains;
 *     tANI_U8   numOfRxChains;
 *     tANI_U8   macAddr[NV_FIELD_MAC_ADDR_SIZE];
 *     tANI_U8   mfgSN[NV_FIELD_MFG_SN_SIZE];
 * } sNvFields;
 */
#define WFC_UTIL_NV_BIN_HEADER_LENGTH   (4)
#define WFC_UTIL_NV_BIN_POS_PRODUCT_ID  (WFC_UTIL_NV_BIN_HEADER_LENGTH + 0)
#define WFC_UTIL_NV_BIN_POS_MAC_ADDR    (WFC_UTIL_NV_BIN_HEADER_LENGTH + 6)

#ifdef WLAN_CHIP_VERSION_WCNSS
/* refer to prima/CORE/WDA/src/wlan_nv.c */
static unsigned char nvFilelds_default[6] = {0, 0, /* productId */
                                             1,    /* productBands */
                                             2,    /* wlanNvRevId */
                                             1,    /* numOfTxChains */
                                             2};   /* numOfRxChains */
#else  /* WLAN_CHIP_VERSION_WCN1314 */
static unsigned char nvFilelds_default[6] = {1, 0, /* productId */
                                             1,    /* productBands */
                                             1,    /* wlanNvRevId */
                                             1,    /* numOfTxChains */
                                             1};   /* numOfRxChains */
#endif /* WLAN_CHIP_VERSION_XXXX */

/*
 * wfc_util_qcom_is_default_mac
 *
 *
 *
 * return : it will return 1 if mac_add is default mac address,
 *          2 if mac_add is RFT mac address
 *          or 0 if not.
 */
static int wfc_util_qcom_is_default_mac(char *mac_add)
{
#define WFC_UTIL_CFG_DEFAULT_MAC_RFT     "00900CBACD88"
#define WFC_UTIL_CFG_DEFAULT_MAC_00      "000000000000"
#define WFC_UTIL_CFG_DEFAULT_MAC_FF      "FFFFFFFFFFFF"
#define WFC_UTIL_CFG_DEFAULT_MAC_QCOM_I0 "000AF58989FF"
#define WFC_UTIL_CFG_DEFAULT_MAC_QCOM_I1 "000AF58989FE"
#define WFC_UTIL_CFG_DEFAULT_MAC_QCOM_I2 "000AF58989FD"
#define WFC_UTIL_CFG_DEFAULT_MAC_QCOM_I3 "000AF58989FC"
#define WFC_UTIL_CFG_DEFAULT_MAC_QCOM_AP "000AF58989EF"

	int i, sZarray=0;
	/*
	 * default mac address array
	 */
	char mac_add_buff[][WFC_UTIL_CFG_LENGHT_MAC_STRING+1] = {
			{WFC_UTIL_CFG_DEFAULT_MAC_00},
			{WFC_UTIL_CFG_DEFAULT_MAC_FF},
			{WFC_UTIL_CFG_DEFAULT_MAC_QCOM_I0}
	};

	sZarray = sizeof(mac_add_buff) / sizeof(mac_add_buff[0]);

	for(i=0; i<sZarray ;i++) {
		if(0 == strncmp(mac_add, mac_add_buff[i], WFC_UTIL_CFG_LENGHT_MAC_STRING)) {
			wfc_util_log_error("This is default MAC address [%s]", mac_add_buff[i]);
			return 1;
		}
	}

	/*
	if(1 == wfc_util_is_random_mac(mac_add)) {
		wfc_util_log_error("This is Random MAC address");
		return 1;
	}
	*/

	if(0 == strncmp(mac_add, WFC_UTIL_CFG_DEFAULT_MAC_RFT, WFC_UTIL_CFG_LENGHT_MAC_STRING)) {
		wfc_util_log_error("This is RFT MAC address [%s]", WFC_UTIL_CFG_DEFAULT_MAC_RFT);
		return 2;
	}

	return 0;
}

static void wfc_util_qcom_write_mac(char *mac_add)
{
	/*
	 * Station Mode MAC Address
	 */
	wfc_util_fset_string(WFC_UTIL_CFG_FILE_NAME,
	                     WFC_UTIL_CFG_TAG_END_OF_CFG,
	                     WFC_UTIL_CFG_TAG_MAC_ADDRESS,
	                     WFC_UTIL_CFG_TAG_END_OF_LINE,
	                     mac_add);

	/*
	 * AP Mode MAC Address
	 */
	wfc_util_fset_string(WFC_UTIL_CFG_FILE_NAME,
	                     WFC_UTIL_CFG_TAG_END_OF_CFG,
	                     WFC_UTIL_CFG_TAG_AP_MAC_ADDRESS,
	                     WFC_UTIL_CFG_TAG_END_OF_LINE,
	                     mac_add);

	return;
}

/*
 *  When OTA is enabled, power save mode and L2 roaming trigger should be off
 */
static void wfc_util_qcom_write_ota_enable(void)
{
/*
 * write Beacon Mode Power Save off and L2 Roaming off
 */
	char *PowerSaveOff = "0";
	//char *L2RoamingOff = "0";
	char *Heartbeat24 = "120";

	char string_buff[5];

	wfc_util_fset_string(WFC_UTIL_CFG_FILE_NAME,
	                     WFC_UTIL_CFG_TAG_END_OF_CFG,
	                     WFC_UTIL_CFG_TAG_IDLE_MODE_POWER_SAVE,
	                     WFC_UTIL_CFG_TAG_END_OF_LINE,
	                     PowerSaveOff);

	wfc_util_fset_string(WFC_UTIL_CFG_FILE_NAME,
	                     WFC_UTIL_CFG_TAG_END_OF_CFG,
	                     WFC_UTIL_CFG_TAG_POWER_SAVE,
	                     WFC_UTIL_CFG_TAG_END_OF_LINE,
	                     PowerSaveOff);

/* We don't need to change this becasue the default value of WFC_UTIL_CFG_TAG_L2Roaming is 0.
	wfc_util_fset_string(WFC_UTIL_CFG_FILE_NAME,
	                     WFC_UTIL_CFG_TAG_END_OF_CFG,
	                     WFC_UTIL_CFG_TAG_L2Roaming,
	                     WFC_UTIL_CFG_TAG_END_OF_LINE,
	                     L2RoamingOff);
*/

	if(0 < wfc_util_fget_string(WFC_UTIL_CFG_FILE_NAME,
	                            WFC_UTIL_CFG_TAG_END_OF_CFG,
	                            WFC_UTIL_CFG_TAG_HEARTBEAT24,
	                            WFC_UTIL_CFG_TAG_END_OF_LINE,
	                            string_buff,
	                            5)) {
		wfc_util_fset_string(WFC_UTIL_CFG_FILE_NAME,
		                     WFC_UTIL_CFG_TAG_END_OF_CFG,
		                     WFC_UTIL_CFG_TAG_HEARTBEAT24,
		                     WFC_UTIL_CFG_TAG_END_OF_LINE,
		                     Heartbeat24);
	} else {
		wfc_util_log_error("%s is not exist", WFC_UTIL_CFG_TAG_HEARTBEAT24);
	}

	return;
}

/*
 *  When OTA is enabled, power save mode and L2 roaming trigger should be off
 */
static void wfc_util_qcom_write_ota_disable(void)
{
/*
 * write Beacon Mode Power Save on and L2 Roaming on
 */
	char *PowerSaveOff = "1";
	//char *L2RoamingOff = "1";
	char *Heartbeat24 = "40";

	char string_buff[5];

	wfc_util_fset_string(WFC_UTIL_CFG_FILE_NAME,
	                     WFC_UTIL_CFG_TAG_END_OF_CFG,
	                     WFC_UTIL_CFG_TAG_IDLE_MODE_POWER_SAVE,
	                     WFC_UTIL_CFG_TAG_END_OF_LINE,
	                     PowerSaveOff);

	wfc_util_fset_string(WFC_UTIL_CFG_FILE_NAME,
	                     WFC_UTIL_CFG_TAG_END_OF_CFG,
	                     WFC_UTIL_CFG_TAG_POWER_SAVE,
	                     WFC_UTIL_CFG_TAG_END_OF_LINE,
	                     PowerSaveOff);

/* We don't need to change this becasue the default value of WFC_UTIL_CFG_TAG_L2Roaming is 0.
	wfc_util_fset_string(WFC_UTIL_CFG_FILE_NAME,
	                     WFC_UTIL_CFG_TAG_END_OF_CFG,
	                     WFC_UTIL_CFG_TAG_L2Roaming,
	                     WFC_UTIL_CFG_TAG_END_OF_LINE,
	                     L2RoamingOff);
*/

	if(0 < wfc_util_fget_string(WFC_UTIL_CFG_FILE_NAME,
	                            WFC_UTIL_CFG_TAG_END_OF_CFG,
	                            WFC_UTIL_CFG_TAG_HEARTBEAT24,
	                            WFC_UTIL_CFG_TAG_END_OF_LINE,
	                            string_buff,
	                            5)) {
		wfc_util_fset_string(WFC_UTIL_CFG_FILE_NAME,
		                     WFC_UTIL_CFG_TAG_END_OF_CFG,
		                     WFC_UTIL_CFG_TAG_HEARTBEAT24,
		                     WFC_UTIL_CFG_TAG_END_OF_LINE,
		                     Heartbeat24);
	} else {
		wfc_util_log_error("%s is not exist", WFC_UTIL_CFG_TAG_HEARTBEAT24);
	}

	return;
}

static void wfc_util_qcom_write_mac_to_bin(unsigned char *mac_add)
{
	unsigned char nvValidityBitmap[WFC_UTIL_NV_BIN_HEADER_LENGTH];

	if(0 != wfc_util_ffile_check(WFC_UTIL_NV_BIN_FILE_NAME,
	                             F_OK|R_OK|W_OK)) {
		wfc_util_log_error("We don't access file [%s]", WFC_UTIL_NV_BIN_FILE_NAME);
		return;
	}

	memset(nvValidityBitmap, 0, WFC_UTIL_NV_BIN_HEADER_LENGTH);

	/*
	 * Write RFT MAC Address
	 */
	wfc_util_fset_buffer(WFC_UTIL_NV_BIN_FILE_NAME,
	                     WFC_UTIL_NV_BIN_POS_MAC_ADDR,
	                     mac_add,
	                     WFC_UTIL_CFG_LENGHT_MAC);

	/*
	 * Read NV validity bitmap
	 */
	if (0 < wfc_util_fget_buffer(WFC_UTIL_NV_BIN_FILE_NAME,
	                             0,
	                             WFC_UTIL_NV_BIN_HEADER_LENGTH,
	                             nvValidityBitmap,
	                             WFC_UTIL_NV_BIN_HEADER_LENGTH)){
	        /*
	         * Check whether Fields bit(Bit 1) is set
	         */
		if (0x02 & nvValidityBitmap[0]) {
			wfc_util_log_info("We don't need to write the default value for NvFilelds");
		} else {
		        /*
		         * Update the Fields bit(Bit 1)
		         */
			nvValidityBitmap[0] |= 0x02;
			wfc_util_fset_buffer(WFC_UTIL_NV_BIN_FILE_NAME,
			                     0,
			                     nvValidityBitmap,
			                     WFC_UTIL_NV_BIN_HEADER_LENGTH);

		        /*
		         * Write the default value for NvFilelds
		         */
			wfc_util_fset_buffer(WFC_UTIL_NV_BIN_FILE_NAME,
			                     WFC_UTIL_NV_BIN_POS_PRODUCT_ID,
			                     nvFilelds_default,
			                     6);
		}
	} else {
		wfc_util_log_error("Read Fail nvValidityBitmap");
	}

	return;
}

/*
 * wfc_util_qcom_reset_mac_to_bin
 *
 * reset the mac address of nv bin file
 *
 * return : void
 */
static void wfc_util_qcom_reset_mac_to_bin(void)
{
	unsigned char mac_addr[WFC_UTIL_CFG_LENGHT_MAC];

	if(0 != wfc_util_ffile_check(WFC_UTIL_NV_BIN_FILE_NAME,
	                             F_OK|R_OK|W_OK)) {
		wfc_util_log_error("We don't access file [%s]", WFC_UTIL_NV_BIN_FILE_NAME);
		return;
	}

	if(0 < wfc_util_fget_buffer(WFC_UTIL_NV_BIN_FILE_NAME,
	                            WFC_UTIL_NV_BIN_POS_MAC_ADDR,
	                            WFC_UTIL_CFG_LENGHT_MAC,
	                            mac_addr,
	                            WFC_UTIL_CFG_LENGHT_MAC)) {
		if(0x00 == mac_addr[0] && 0x00 == mac_addr[1] && 0x00 == mac_addr[2] &&
		   0x00 == mac_addr[3] && 0x00 == mac_addr[4] && 0x00 == mac_addr[5])
		{
			return;
		}
	}

	memset(mac_addr, 0, WFC_UTIL_CFG_LENGHT_MAC);

	wfc_util_fset_buffer(WFC_UTIL_NV_BIN_FILE_NAME,
	                     WFC_UTIL_NV_BIN_POS_MAC_ADDR,
	                     mac_addr,
	                     WFC_UTIL_CFG_LENGHT_MAC);

	return;
}

static int wfc_util_qcom_write_mac_process(unsigned char *nv_mac_addr, char *mac_add_buff)
{
	char nv_mac_add_buff[WFC_UTIL_CFG_LENGHT_MAC_STRING+1];
	int  is_default_nv_mac = 0;
	int  is_same_mac = -1;

	if (NULL == nv_mac_addr) {
		return 0;
	}

	wfc_util_htoa(nv_mac_addr, WFC_UTIL_CFG_LENGHT_MAC,
	              nv_mac_add_buff, WFC_UTIL_CFG_LENGHT_MAC_STRING+1);

	is_default_nv_mac = wfc_util_qcom_is_default_mac(nv_mac_add_buff);

	is_same_mac = strncmp(mac_add_buff, nv_mac_add_buff, WFC_UTIL_CFG_LENGHT_MAC_STRING);

	/*
	 * 1. nv mac address is not a default mac address
	 * 2. same with mac address of config file
	 */
	if (((!is_default_nv_mac) && (0==is_same_mac)) ||
	/*
	 * 1. nv mac address is RFT mac address
	 * 2. same with mac address of config file
	 */
	    ((2==is_default_nv_mac) && (0==is_same_mac))
	   ) {
		return 1;
	}
	/*
	 * 1. nv mac address not a default mac address excepting RFT mac address
	 * 2. does not same with mac address of config file
	 */
	else if ((1!=is_default_nv_mac) && (0!=is_same_mac)) {
		wfc_util_log_error("Change %s%s", WFC_UTIL_CFG_TAG_MAC_ADDRESS, nv_mac_add_buff);
		/*
		 * Update MAC address
		 */
		wfc_util_qcom_write_mac(nv_mac_add_buff);

#ifdef WFC_UTIL_FEATURE_DO_NOT_WRITE_MAC_TO_BIN
		/*
		 * Write RFT MAC address to nv.bin
		 */
		if (2==is_default_nv_mac) {
			wfc_util_qcom_write_mac_to_bin(nv_mac_addr);
		/*
		 * reset mac address of nv.bin if nv_mac_addr is not RFT mac address
		 */
		} else {
			wfc_util_qcom_reset_mac_to_bin();
		}
#else  /* WFC_UTIL_FEATURE_DO_NOT_WRITE_MAC_TO_BIN */
		/*
		 * Write MAC address to nv.bin
		 */
		wfc_util_qcom_write_mac_to_bin(nv_mac_addr);
#endif /* WFC_UTIL_FEATURE_DO_NOT_WRITE_MAC_TO_BIN */

		return 1;
	}

	return 0;
}

static void wfc_util_qcom_create_random_mac(void)
{
	unsigned char random_mac_addr[WFC_UTIL_CFG_LENGHT_MAC];
	char mac_add_buff[WFC_UTIL_CFG_LENGHT_MAC_STRING+1];

	wfc_util_log_info("wfc_util_qcom_create_random_mac");

	wfc_util_random_mac(random_mac_addr);

	wfc_util_htoa(random_mac_addr, WFC_UTIL_CFG_LENGHT_MAC, mac_add_buff, WFC_UTIL_CFG_LENGHT_MAC_STRING+1);

	wfc_util_qcom_write_mac(mac_add_buff);

#ifdef WFC_UTIL_FEATURE_DO_NOT_WRITE_MAC_TO_BIN
	wfc_util_qcom_reset_mac_to_bin();
#else  /* WFC_UTIL_FEATURE_DO_NOT_WRITE_MAC_TO_BIN */
	wfc_util_qcom_write_mac_to_bin(random_mac_addr);
#endif /* WFC_UTIL_FEATURE_DO_NOT_WRITE_MAC_TO_BIN */

	return;
}

/*
 * wfc_util_qcom_check_config
 *
 * check the qcom wlan driver config file
 *
 * return : it will return 0 if procedure is success
 *          or will return -1 if not.
 */
int wfc_util_qcom_check_config(unsigned char *nv_mac_addr)
{
	char mac_add_buff[WFC_UTIL_CFG_LENGHT_MAC_STRING+1];

	/* make sure driver config file exists */
	if(0 > wfc_util_ffile_check_copy(WFC_UTIL_CFG_FILE_NAME,
	                             WFC_UTIL_CFG_TEMPFILE_NAME,
	                             0660,
	                             AID_SYSTEM,
	                             /* we use "radio" for gid to access from "rild" for AT cmd. */
	                             AID_WIFI/*AID_WIFI*/)) {
		wfc_util_log_error("Fail to Access [%s]", WFC_UTIL_CFG_FILE_NAME);
		return -1;
	}

#ifdef WFC_UTIL_FEAUTRE_COPY_NV_BIN
	if(0 > wfc_util_ffile_check_copy(WFC_UTIL_NV_BIN_FILE_NAME,
	                             WFC_UTIL_NV_BIN_TEMPFILE_NAME,
	                             0660,
	                             AID_SYSTEM,
	                             /* we use "radio" for gid to access from "rild" for AT cmd. */
	                             AID_WIFI/*AID_WIFI*/)) {
		wfc_util_log_error("Fail to Access [%s]", WFC_UTIL_NV_BIN_FILE_NAME);
		return -1;
	}
#endif /* WFC_UTIL_FEAUTRE_COPY_NV_BIN */

	/*
	 * Read MAC address from config file
	 */
	if(0 < wfc_util_fget_string(WFC_UTIL_CFG_FILE_NAME,
	                        WFC_UTIL_CFG_TAG_END_OF_CFG,
	                        WFC_UTIL_CFG_TAG_MAC_ADDRESS,
	                        WFC_UTIL_CFG_TAG_END_OF_LINE,
	                        mac_add_buff,
	                        WFC_UTIL_CFG_LENGHT_MAC_STRING+1)) {
		wfc_util_log_info("%s%s", WFC_UTIL_CFG_TAG_MAC_ADDRESS, mac_add_buff);

		/*
		 * Write nv mac address
		 */
		if (1 != wfc_util_qcom_write_mac_process(nv_mac_addr, mac_add_buff)) {
			/*
			 * Check whether this is default mac address or not
			 */
			if (wfc_util_qcom_is_default_mac(mac_add_buff)) {
				/*
				 * Create random MAC address
				 */
				wfc_util_qcom_create_random_mac();
			}
		}
	} else {
		wfc_util_log_error("%s does not have mac address", WFC_UTIL_CFG_FILE_NAME);

		memset( mac_add_buff, 0, WFC_UTIL_CFG_LENGHT_MAC_STRING+1 );

		/*
		 * Write nv mac address
		 */
		if (1 != wfc_util_qcom_write_mac_process(nv_mac_addr, mac_add_buff)) {
			/*
			 * Create random MAC address
			 */
			wfc_util_qcom_create_random_mac();
		}
	}

	return 0;
}

/*
 * wfc_util_qcom_reset_mac
 *
 * reset the mac address of config file
 *
 * return : void
 */
void wfc_util_qcom_reset_mac(void)
{
	wfc_util_qcom_write_mac("000000000000");

	wfc_util_qcom_reset_mac_to_bin();

	return;
}

/*
 * wfc_util_qcom_ota_enable
 *
 * enable ota mode by reconfiguring BMPS and L2Roaming
 *
 * return : int (boolean)
 */
int wfc_util_qcom_ota_enable(void)
{
	wfc_util_qcom_write_ota_enable();
	return 1;
}

/*
 * wfc_util_qcom_ota_disable
 *
 * disable ota mode by reconfiguring BMPS and L2Roaming
 *
 * return : int (boolean)
 */
int wfc_util_qcom_ota_disable(void)
{
	wfc_util_qcom_write_ota_disable();
	return 1;
}

/*
 * wfc_util_qcom_checkt_roaming_off
 *
 * Check L2Roaming configuration
 *
 * return : int (boolean)
 */
int wfc_util_qcom_checkt_roaming_off(void)
{
	char string_buff[5];
	/*
	 * check whether OTA test is enabled or not.
	 */
	if(0 < wfc_util_fget_string(WFC_UTIL_CFG_FILE_NAME,
	                            WFC_UTIL_CFG_TAG_END_OF_CFG,
	                            //WFC_UTIL_CFG_TAG_L2Roaming,
	                            WFC_UTIL_CFG_TAG_POWER_SAVE,
	                            WFC_UTIL_CFG_TAG_END_OF_LINE,
	                            string_buff,
	                            5)) {
		//wfc_util_log_info("%s%s", WFC_UTIL_CFG_TAG_L2Roaming, string_buff);
		wfc_util_log_info("%s%s", WFC_UTIL_CFG_TAG_POWER_SAVE, string_buff);
		if(0 == strncmp(string_buff, "0", 1)) {
			return 1;
		}
	}
	return 0;
}

#endif /* CONFIG_LGE_WLAN_QCOM_PATCH */

