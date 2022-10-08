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

#ifndef __WFC_UTIL_QCOM_H__
#define __WFC_UTIL_QCOM_H__

#ifdef CONFIG_LGE_WLAN_QCOM_PATCH
/*
 * wfc_util_qcom_check_config
 *
 * check the qcom wlan driver config file
 *
 * return : it will return 0 if procedure is success
 *          or will return -1 if not.
 */
extern int wfc_util_qcom_check_config(unsigned char *nv_mac_addr);

/*
 * wfc_util_qcom_reset_mac
 *
 * reset the mac address of config file
 *
 * return : void
 */
extern void wfc_util_qcom_reset_mac(void);

/*
 * wfc_util_qcom_ota_enable/disable
 *
 * enable OTA mode for Wi-Fi related certificiation
 *
 * return : int (boolean)
 */
extern int wfc_util_qcom_ota_enable(void);
extern int wfc_util_qcom_ota_disable(void);
extern int wfc_util_qcom_checkt_roaming_off(void);
#endif /* CONFIG_LGE_WLAN_QCOM_PATCH */
#endif

