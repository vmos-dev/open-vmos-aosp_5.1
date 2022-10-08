/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "gscan_event_handler.h"
#include "vendor_definitions.h"

/* This function implements creation of Vendor command event handler. */
int GScanCommandEventHandler::create() {
    int ret = mMsg.create(NL80211_CMD_VENDOR, 0, 0);
    if (ret < 0) {
        return ret;
    }

    /* Insert the oui in the msg */
    ret = mMsg.put_u32(NL80211_ATTR_VENDOR_ID, mVendor_id);
    if (ret < 0)
        goto out;

    /* Insert the subcmd in the msg */
    ret = mMsg.put_u32(NL80211_ATTR_VENDOR_SUBCMD, mSubcmd);
    if (ret < 0)
        goto out;
out:
    return ret;
}

int GScanCommandEventHandler::get_request_id()
{
    return mRequestId;
}

GScanCommandEventHandler::GScanCommandEventHandler(wifi_handle handle, int id,
                                                u32 vendor_id,
                                                u32 subcmd,
                                                GScanCallbackHandler handler)
        : WifiVendorCommand(handle, id, vendor_id, subcmd)
{
    int ret = 0;
    ALOGD("GScanCommandEventHandler %p constructed", this);
    mRequestId = id;
    mHandler = handler;
    mSubCommandId = subcmd;
    mHotlistApFoundResults = NULL;
    mHotlistApFoundNumResults = 0;
    mHotlistApFoundMoreData = false;
    mSignificantChangeResults = NULL;
    mSignificantChangeNumResults = 0;
    mSignificantChangeMoreData = false;

    switch(mSubCommandId)
    {
        case QCA_NL80211_VENDOR_SUBCMD_GSCAN_START:
        {
            /* Register handlers for northbound asychronous scan events. */
            ALOGD("%s: wait for GSCAN_RESULTS_AVAILABLE, "
                "FULL_SCAN_RESULT, and SCAN EVENT events. \n", __func__);
            ret = registerVendorHandler(mVendor_id,
                    QCA_NL80211_VENDOR_SUBCMD_GSCAN_SCAN_RESULTS_AVAILABLE) ||
                  registerVendorHandler(mVendor_id,
                    QCA_NL80211_VENDOR_SUBCMD_GSCAN_FULL_SCAN_RESULT) ||
                  registerVendorHandler(mVendor_id,
                    QCA_NL80211_VENDOR_SUBCMD_GSCAN_SCAN_EVENT);
            if (ret < 0)
                ALOGD("%s: Error in registering handler for "
                    "GSCAN_START. \n", __func__);
        }
        break;

        case QCA_NL80211_VENDOR_SUBCMD_GSCAN_SET_SIGNIFICANT_CHANGE:
        {
            ret = registerVendorHandler(mVendor_id,
                    QCA_NL80211_VENDOR_SUBCMD_GSCAN_SIGNIFICANT_CHANGE);
            if (ret < 0)
                ALOGD("%s: Error in registering handler for "
                    "GSCAN_SIGNIFICANT_CHANGE. \n", __func__);
        }
        break;

        case QCA_NL80211_VENDOR_SUBCMD_GSCAN_SET_BSSID_HOTLIST:
        {
            ret = registerVendorHandler(mVendor_id,
                    QCA_NL80211_VENDOR_SUBCMD_GSCAN_HOTLIST_AP_FOUND);
            if (ret < 0)
                ALOGD("%s: Error in registering handler for"
                    " GSCAN_HOTLIST_AP_FOUND. \n", __func__);
        }
        break;
    }
}

GScanCommandEventHandler::~GScanCommandEventHandler()
{
    ALOGD("GScanCommandEventHandler %p destructor", this);
    switch(mSubCommandId)
    {
        case QCA_NL80211_VENDOR_SUBCMD_GSCAN_START:
        {
            /* Unregister event handlers. */
            ALOGD("%s: Unregister handlers for GSCAN_RESULTS_AVAILABLE, "
            "FULL_SCAN_RESULT, and SCAN EVENT events. \n", __func__);
            unregisterVendorHandler(mVendor_id,
                    QCA_NL80211_VENDOR_SUBCMD_GSCAN_SCAN_RESULTS_AVAILABLE);
            unregisterVendorHandler(mVendor_id,
                    QCA_NL80211_VENDOR_SUBCMD_GSCAN_FULL_SCAN_RESULT);
            unregisterVendorHandler(mVendor_id,
                    QCA_NL80211_VENDOR_SUBCMD_GSCAN_SCAN_EVENT);
        }
        break;

        case QCA_NL80211_VENDOR_SUBCMD_GSCAN_SET_SIGNIFICANT_CHANGE:
        {
            unregisterVendorHandler(mVendor_id,
                    QCA_NL80211_VENDOR_SUBCMD_GSCAN_SIGNIFICANT_CHANGE);
        }
        break;

        case QCA_NL80211_VENDOR_SUBCMD_GSCAN_SET_BSSID_HOTLIST:
        {
            unregisterVendorHandler(mVendor_id,
                    QCA_NL80211_VENDOR_SUBCMD_GSCAN_HOTLIST_AP_FOUND);
        }
        break;
    }
}

static wifi_error gscan_get_hotlist_ap_found_results(u32 num_results,
                                            wifi_scan_result *results,
                                            u32 starting_index,
                                            struct nlattr **tb_vendor)
{
    u32 i = starting_index;
    struct nlattr *scanResultsInfo;
    int rem = 0;
    u32 len = 0;
    ALOGE("gscan_get_hotlist_ap_found_results: starting counter: %d", i);

    for (scanResultsInfo = (struct nlattr *) nla_data(tb_vendor[
            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_LIST]),
            rem = nla_len(tb_vendor[
            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_LIST
            ]);
                nla_ok(scanResultsInfo, rem);
                scanResultsInfo = nla_next(scanResultsInfo, &(rem)))
    {
        struct nlattr *tb2[ QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_MAX + 1];
        nla_parse(tb2, QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_MAX,
        (struct nlattr *) nla_data(scanResultsInfo),
                nla_len(scanResultsInfo), NULL);

        if (!
            tb2[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_TIME_STAMP
                ])
        {
            ALOGE("gscan_get_hotlist_ap_found_results: "
                "RESULTS_SCAN_RESULT_TIME_STAMP not found");
            return WIFI_ERROR_INVALID_ARGS;
        }
        results[i].ts =
            nla_get_u64(
            tb2[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_TIME_STAMP
                ]);
        if (!
            tb2[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_SSID
                ])
        {
            ALOGE("gscan_get_hotlist_ap_found_results: "
                "RESULTS_SCAN_RESULT_SSID not found");
            return WIFI_ERROR_INVALID_ARGS;
        }
        len = nla_len(tb2[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_SSID]);
        len =
            sizeof(results->ssid) <= len ? sizeof(results->ssid) : len;
        memcpy((void *)&results[i].ssid,
            nla_data(
            tb2[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_SSID]), len);
        if (!
            tb2[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_BSSID
                ])
        {
            ALOGE("gscan_get_hotlist_ap_found_results: "
                "RESULTS_SCAN_RESULT_BSSID not found");
            return WIFI_ERROR_INVALID_ARGS;
        }
        len = nla_len(
            tb2[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_BSSID]);
        len =
            sizeof(results->bssid) <= len ? sizeof(results->bssid) : len;
        memcpy(&results[i].bssid,
            nla_data(
            tb2[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_BSSID]), len);
        if (!
            tb2[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_CHANNEL
                ])
        {
            ALOGE("gscan_get_hotlist_ap_found_results: "
                "RESULTS_SCAN_RESULT_CHANNEL not found");
            return WIFI_ERROR_INVALID_ARGS;
        }
        results[i].channel =
            nla_get_u32(
            tb2[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_CHANNEL]);
        if (!
            tb2[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_RSSI
                ])
        {
            ALOGE("gscan_get_hotlist_ap_found_results: "
                "RESULTS_SCAN_RESULT_RSSI not found");
            return WIFI_ERROR_INVALID_ARGS;
        }
        results[i].rssi =
            nla_get_u32(
            tb2[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_RSSI]);
        if (!
            tb2[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_RTT
                ])
        {
            ALOGE("gscan_get_hotlist_ap_found_results: "
                "RESULTS_SCAN_RESULT_RTT not found");
            return WIFI_ERROR_INVALID_ARGS;
        }
        results[i].rtt =
            nla_get_u32(
            tb2[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_RTT]);
        if (!
            tb2[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_RTT_SD
            ])
        {
            ALOGE("gscan_get_hotlist_ap_found_results: "
                "RESULTS_SCAN_RESULT_RTT_SD not found");
            return WIFI_ERROR_INVALID_ARGS;
        }
        results[i].rtt_sd =
            nla_get_u32(
            tb2[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_RTT_SD]);

        ALOGE("gscan_get_hotlist_ap_found_results: ts  %lld ", results[i].ts);
        ALOGE("gscan_get_hotlist_ap_found_results: SSID  %s ",
            results[i].ssid) ;
        ALOGE("gscan_get_hotlist_ap_found_results: "
            "BSSID: %02x:%02x:%02x:%02x:%02x:%02x \n",
            results[i].bssid[0], results[i].bssid[1], results[i].bssid[2],
            results[i].bssid[3], results[i].bssid[4], results[i].bssid[5]);
        ALOGE("gscan_get_hotlist_ap_found_results: channel %d ",
            results[i].channel);
        ALOGE("gscan_get_hotlist_ap_found_results: rssi %d ", results[i].rssi);
        ALOGE("gscan_get_hotlist_ap_found_results: rtt %lld ", results[i].rtt);
        ALOGE("gscan_get_hotlist_ap_found_results: rtt_sd %lld ",
            results[i].rtt_sd);
        /* Increment loop index for next record */
        i++;
    }
    return WIFI_SUCCESS;
}

static wifi_error gscan_get_significant_change_results(u32 num_results,
                                    wifi_significant_change_result **results,
                                    u32 starting_index,
                                    struct nlattr **tb_vendor)
{
    u32 i = starting_index;
    int j;
    int rem = 0;
    u32 len = 0;
    struct nlattr *scanResultsInfo;

    ALOGE("gscan_get_significant_change_results: starting counter: %d", i);

    for (scanResultsInfo = (struct nlattr *) nla_data(tb_vendor[
            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_LIST]),
            rem = nla_len(tb_vendor[
            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_LIST]);
        nla_ok(scanResultsInfo, rem);
        scanResultsInfo = nla_next(scanResultsInfo, &(rem)))
    {
        struct nlattr *tb2[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_MAX + 1];
        nla_parse(tb2, QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_MAX,
            (struct nlattr *) nla_data(scanResultsInfo),
                nla_len(scanResultsInfo), NULL);
        if (!
            tb2[
            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SIGNIFICANT_CHANGE_RESULT_BSSID
            ])
        {
            ALOGE("gscan_get_significant_change_results: "
                "SIGNIFICANT_CHANGE_RESULT_BSSID not found");
            return WIFI_ERROR_INVALID_ARGS;
        }
        len = nla_len(
            tb2[
            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SIGNIFICANT_CHANGE_RESULT_BSSID]
            );
        len =
            sizeof(results[i]->bssid) <= len ? sizeof(results[i]->bssid) : len;
        memcpy(&results[i]->bssid[0],
            nla_data(
            tb2[
        QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SIGNIFICANT_CHANGE_RESULT_BSSID]),
            len);
        ALOGI("\nsignificant_change_result:%d, BSSID:"
            "%02x:%02x:%02x:%02x:%02x:%02x \n", i, results[i]->bssid[0],
            results[i]->bssid[1], results[i]->bssid[2], results[i]->bssid[3],
            results[i]->bssid[4], results[i]->bssid[5]);

        if (!
            tb2[
        QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SIGNIFICANT_CHANGE_RESULT_CHANNEL
                ])
        {
            ALOGE("gscan_get_significant_change_results: "
                "SIGNIFICANT_CHANGE_RESULT_CHANNEL not found");
            return WIFI_ERROR_INVALID_ARGS;
        }
        results[i]->channel =
            nla_get_u32(
            tb2[
        QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SIGNIFICANT_CHANGE_RESULT_CHANNEL]);
        ALOGI("significant_change_result:%d, channel:%d.\n",
            i, results[i]->channel);

        if (!
            tb2[
        QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SIGNIFICANT_CHANGE_RESULT_NUM_RSSI
            ])
        {
            ALOGE("gscan_get_significant_change_results: "
                "SIGNIFICANT_CHANGE_RESULT_NUM_RSSI not found");
            return WIFI_ERROR_INVALID_ARGS;
        }
        results[i]->num_rssi =
            nla_get_u32(
            tb2[
        QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SIGNIFICANT_CHANGE_RESULT_NUM_RSSI]);
        ALOGI("gscan_get_significant_change_results: "
            "significant_change_result:%d, num_rssi:%d.\n",
            i, results[i]->num_rssi);

        if (!
            tb2[
        QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SIGNIFICANT_CHANGE_RESULT_RSSI_LIST
            ])
        {
            ALOGE("gscan_get_significant_change_results: "
                "SIGNIFICANT_CHANGE_RESULT_RSSI_LIST not found");
            return WIFI_ERROR_INVALID_ARGS;
        }
        ALOGI("gscan_get_significant_change_results: before reading the RSSI "
            "list: num_rssi:%d, size_of_rssi:%d, total size:%d, ",
            results[i]->num_rssi,
            sizeof(wifi_rssi), results[i]->num_rssi * sizeof(wifi_rssi));

        memcpy(&(results[i]->rssi[0]),
            nla_data(
            tb2[
        QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SIGNIFICANT_CHANGE_RESULT_RSSI_LIST]
            ), results[i]->num_rssi * sizeof(wifi_rssi));

        for (j = 0; j < results[i]->num_rssi; j++)
            ALOGI("     significant_change_result: %d, rssi[%d]:%d, ",
            i, j, results[i]->rssi[j]);

        /* Increment loop index to prase next record. */
        i++;
    }
    return WIFI_SUCCESS;
}

/* This function will be the main handler for incoming (from driver)  GSscan_SUBCMD.
 *  Calls the appropriate callback handler after parsing the vendor data.
 */
int GScanCommandEventHandler::handleEvent(WifiEvent &event)
{
    ALOGI("GScanCommandEventHandler::handleEvent: Got a GSCAN Event"
        " message from the Driver.");
    unsigned i=0;
    int ret = WIFI_SUCCESS;
    u32 status;
    wifi_scan_result *result;
    struct nlattr *tbVendor[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_MAX + 1];

    WifiVendorCommand::handleEvent(event);

    nla_parse(tbVendor, QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_MAX,
                        (struct nlattr *)mVendorData,
                        mDataLen, NULL);

    switch(mSubcmd)
    {
        case QCA_NL80211_VENDOR_SUBCMD_GSCAN_FULL_SCAN_RESULT:
        {
            wifi_request_id reqId;
            u32 len = 0;
            u32 resultsBufSize = 0;
            u32 lengthOfInfoElements = 0;

            ALOGD("Event QCA_NL80211_VENDOR_SUBCMD_GSCAN_FULL_SCAN_RESULT "
                "received.");

            if (!tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_REQUEST_ID])
            {
                ALOGE("%s: ATTR_GSCAN_RESULTS_REQUEST_ID not found. Exit.",
                    __func__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            reqId = nla_get_u32(
                    tbVendor[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_REQUEST_ID]
                    );
            /* If this is not for us, just ignore it. */
            if (reqId != mRequestId) {
                ALOGE("%s: Event has Req. ID:%d <> Ours:%d, ignore it.",
                    __func__, reqId, mRequestId);
                break;
            }

            /* Parse and extract the results. */
            if (!
                tbVendor[
                    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_IE_LENGTH
                ])
            {
                ALOGE("%s:RESULTS_SCAN_RESULT_IE_LENGTH not found", __func__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            lengthOfInfoElements =
                nla_get_u32(
                tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_IE_LENGTH]);
            ALOGI("%s: RESULTS_SCAN_RESULT_IE_LENGTH =%d",
                __func__, lengthOfInfoElements);
            resultsBufSize =
                lengthOfInfoElements + sizeof(wifi_scan_result);
            result = (wifi_scan_result *) malloc (resultsBufSize);
            if (!result) {
                ALOGE("%s: Failed to alloc memory for result struct. Exit.\n",
                    __func__);
                ret = WIFI_ERROR_OUT_OF_MEMORY;
                break;
            }
            memset(result, 0, resultsBufSize);

            result->ie_length = lengthOfInfoElements;

            /* Extract and fill out the wifi_scan_result struct. */
            if (!
            tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_TIME_STAMP
                ])
            {
                ALOGE("%s: RESULTS_SCAN_RESULT_TIME_STAMP not found",
                    __func__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            result->ts =
                nla_get_u64(
                tbVendor[
                    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_TIME_STAMP
                    ]);

            if (!
                tbVendor[
                    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_SSID
                    ])
            {
                ALOGE("%s: RESULTS_SCAN_RESULT_SSID not found", __func__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            len = nla_len(tbVendor[
                    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_SSID]);
            len =
                sizeof(result->ssid) <= len ? sizeof(result->ssid) : len;
            memcpy((void *)&result->ssid,
                nla_data(
                tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_SSID]), len);

            if (!
                tbVendor[
                    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_BSSID
                    ])
            {
                ALOGE("%s: RESULTS_SCAN_RESULT_BSSID not found", __func__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            len = nla_len(
                tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_BSSID]);
            len =
                sizeof(result->bssid) <= len ? sizeof(result->bssid) : len;
            memcpy(&result->bssid,
                nla_data(
                tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_BSSID]), len);

            if (!
                tbVendor[
                    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_CHANNEL
                    ])
            {
                ALOGE("%s: RESULTS_SCAN_RESULT_CHANNEL not found", __func__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            result->channel =
                nla_get_u32(
                tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_CHANNEL]);

            if (!
                tbVendor[
                    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_RSSI
                    ])
            {
                ALOGE("%s: RESULTS_SCAN_RESULT_RSSI not found", __func__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            result->rssi =
                nla_get_u32(
                tbVendor[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_RSSI]
                );

            if (!
                tbVendor[
                    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_RTT
                    ])
            {
                ALOGE("%s: RESULTS_SCAN_RESULT_RTT not found", __func__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            result->rtt =
                nla_get_u32(
                tbVendor[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_RTT]);

            if (!
                tbVendor[
                    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_RTT_SD
                ])
            {
                ALOGE("%s: RESULTS_SCAN_RESULT_RTT_SD not found", __func__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            result->rtt_sd =
                nla_get_u32(
                tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_RTT_SD]);

            if (!
                tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_BEACON_PERIOD])
            {
                ALOGE("%s: RESULTS_SCAN_RESULT_BEACON_PERIOD not found",
                    __func__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            result->beacon_period =
                nla_get_u16(
                tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_BEACON_PERIOD]);

            if (!
                tbVendor[
                    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_CAPABILITY
                    ])
            {
                ALOGE("%s: RESULTS_SCAN_RESULT_CAPABILITY not found", __func__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            result->capability =
                nla_get_u16(
                tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_CAPABILITY]);

            if (!
                tbVendor[
                    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_IE_DATA
                ])
            {
                ALOGE("%s: RESULTS_SCAN_RESULT_IE_DATA not found", __func__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            memcpy(&(result->ie_data[0]),
                nla_data(tbVendor[
                    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_IE_DATA]),
                lengthOfInfoElements);

            ALOGE("handleEvent:FULL_SCAN_RESULTS: ts  %lld ", result->ts);
            ALOGE("handleEvent:FULL_SCAN_RESULTS: SSID  %s ", result->ssid) ;
            ALOGE("handleEvent:FULL_SCAN_RESULTS: "
                "BSSID: %02x:%02x:%02x:%02x:%02x:%02x \n",
                result->bssid[0], result->bssid[1], result->bssid[2],
                result->bssid[3], result->bssid[4], result->bssid[5]);
            ALOGE("handleEvent:FULL_SCAN_RESULTS: channel %d ",
                result->channel);
            ALOGE("handleEvent:FULL_SCAN_RESULTS: rssi  %d ", result->rssi);
            ALOGE("handleEvent:FULL_SCAN_RESULTS: rtt  %lld ", result->rtt);
            ALOGE("handleEvent:FULL_SCAN_RESULTS: rtt_sd  %lld ",
                result->rtt_sd);
            ALOGE("handleEvent:FULL_SCAN_RESULTS: beacon period  %d ",
                result->beacon_period);
            ALOGE("handleEvent:FULL_SCAN_RESULTS: capability  %d ",
                result->capability);
            ALOGE("handleEvent:FULL_SCAN_RESULTS: IE length  %d ",
                result->ie_length);

            ALOGE("%s: Invoking the callback. \n", __func__);
            if (mHandler.on_full_scan_result) {
                (*mHandler.on_full_scan_result)(reqId, result);
                /* Reset flag and num counter. */
                free(result);
                result = NULL;
            }
        }
        break;

        case QCA_NL80211_VENDOR_SUBCMD_GSCAN_SCAN_RESULTS_AVAILABLE:
        {
            wifi_request_id id;
            u32 numResults = 0;

            ALOGD("Event "
                "QCA_NL80211_VENDOR_SUBCMD_GSCAN_SCAN_RESULTS_AVAILABLE "
                "received.");

            if (!tbVendor[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_REQUEST_ID]) {
                ALOGE("%s: QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_REQUEST_ID"
                    "not found. Exit", __func__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            id = nla_get_u32(
                    tbVendor[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_REQUEST_ID]
                    );
            /* If this is not for us, then ignore it. */
            if (id != mRequestId) {
                ALOGE("%s: Event has Req. ID:%d <> ours:%d",
                    __func__, id, mRequestId);
                break;
            }
            if (!tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_NUM_RESULTS_AVAILABLE]) {
                ALOGE("%s: GSCAN_RESULTS_NUM_RESULTS_AVAILABLE not found",
                    __func__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            numResults = nla_get_u32(tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_NUM_RESULTS_AVAILABLE]);
            ALOGE("%s: number of results:%d", __func__, numResults);

            /* Invoke the callback func to report the number of results. */
            ALOGE("%s: Calling on_scan_results_available handler",
                __func__);
            if (!mHandler.on_scan_results_available) {
                break;
            }
            (*mHandler.on_scan_results_available)(id, numResults);
        }
        break;

        case QCA_NL80211_VENDOR_SUBCMD_GSCAN_HOTLIST_AP_FOUND:
        {
            wifi_request_id id;
            u32 resultsBufSize = 0;
            u32 numResults = 0;
            u32 startingIndex, sizeOfObtainedResults;

            ALOGD("Event QCA_NL80211_VENDOR_SUBCMD_GSCAN_HOTLIST_AP_FOUND "
                "received.");

            id = nla_get_u32(
                    tbVendor[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_REQUEST_ID]
                    );
            /* If this is not for us, just ignore it. */
            if (id != mRequestId) {
                ALOGE("%s: Event has Req. ID:%d <> ours:%d",
                    __func__, id, mRequestId);
                break;
            }
            if (!tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_NUM_RESULTS_AVAILABLE]) {
                ALOGE("%s: GSCAN_RESULTS_NUM_RESULTS_AVAILABLE not found",
                    __func__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            numResults = nla_get_u32(tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_NUM_RESULTS_AVAILABLE]);
            ALOGE("%s: number of results:%d", __func__, numResults);

            /* Get the memory size of previous fragments, if any. */
            sizeOfObtainedResults = mHotlistApFoundNumResults *
                          sizeof(wifi_scan_result);

            mHotlistApFoundNumResults += numResults;
            resultsBufSize += mHotlistApFoundNumResults *
                                            sizeof(wifi_scan_result);

            /* Check if this chunck of scan results is a continuation of
             * a previous one.
             */
            if (mHotlistApFoundMoreData) {
                mHotlistApFoundResults = (wifi_scan_result *)
                            realloc (mHotlistApFoundResults, resultsBufSize);
            } else {
                mHotlistApFoundResults = (wifi_scan_result *)
                            malloc (resultsBufSize);
            }

            if (!mHotlistApFoundResults) {
                ALOGE("%s: Failed to alloc memory for results array. Exit.\n",
                    __func__);
                ret = WIFI_ERROR_OUT_OF_MEMORY;
                break;
            }
            /* Initialize the newly allocated memory area with 0. */
            memset((u8 *)mHotlistApFoundResults + sizeOfObtainedResults, 0,
                    resultsBufSize - sizeOfObtainedResults);

            ALOGE("%s: Num of AP FOUND results = %d. \n", __func__,
                                            mHotlistApFoundNumResults);

            /* To support fragmentation from firmware, monitor the
             * MORE_DTATA flag and cache results until MORE_DATA = 0.
             * Only then we can pass on the results to framework through
             * the callback function.
             */
            if (!tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_MORE_DATA]) {
                ALOGE("%s: GSCAN_RESULTS_NUM_RESULTS_MORE_DATA not"
                    " found", __func__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            } else {
                mHotlistApFoundMoreData = nla_get_u8(
                    tbVendor[
                    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_MORE_DATA]);
                ALOGE("%s: More data = %d. \n",
                    __func__, mHotlistApFoundMoreData);
            }

            ALOGE("%s: Extract hotlist_ap_found results.\n", __func__);
            startingIndex = mHotlistApFoundNumResults - numResults;
            ALOGE("%s: starting_index:%d",
                __func__, startingIndex);
            ret = gscan_get_hotlist_ap_found_results(numResults,
                                                mHotlistApFoundResults,
                                                startingIndex,
                                                tbVendor);
            /* If a parsing error occurred, exit and proceed for cleanup. */
            if (ret)
                break;
            /* Send the results if no more result data fragments are expected. */
            if (!mHotlistApFoundMoreData) {
                (*mHandler.on_hotlist_ap_found)(id,
                                                mHotlistApFoundNumResults,
                                                mHotlistApFoundResults);
                /* Reset flag and num counter. */
                free(mHotlistApFoundResults);
                mHotlistApFoundResults = NULL;
                mHotlistApFoundMoreData = false;
                mHotlistApFoundNumResults = 0;
            }
        }
        break;

        case QCA_NL80211_VENDOR_SUBCMD_GSCAN_SIGNIFICANT_CHANGE:
        {
            wifi_request_id reqId;
            u32 numResults = 0, sizeOfObtainedResults;
            u32 startingIndex, index = 0;
            struct nlattr *scanResultsInfo;
            int rem = 0;

            ALOGD("Event QCA_NL80211_VENDOR_SUBCMD_GSCAN_SIGNIFICANT_CHANGE "
                "received.");

            if (!tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_REQUEST_ID])
            {
                ALOGE("%s: ATTR_GSCAN_RESULTS_REQUEST_ID not found. Exit.",
                    __func__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            reqId = nla_get_u32(
                    tbVendor[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_REQUEST_ID]
                    );
            /* If this is not for us, just ignore it. */
            if (reqId != mRequestId) {
                ALOGE("%s: Event has Req. ID:%d <> ours:%d",
                    __func__, reqId, mRequestId);
                break;
            }
            if (!tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_NUM_RESULTS_AVAILABLE])
            {
                ALOGE("%s: ATTR_GSCAN_RESULTS_NUM_RESULTS_AVAILABLE not found."
                    "Exit.", __func__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            numResults = nla_get_u32(tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_NUM_RESULTS_AVAILABLE]);
            /* Get the memory size of previous fragments, if any. */
            sizeOfObtainedResults = sizeof(wifi_significant_change_result *) *
                                mSignificantChangeNumResults;

            index = mSignificantChangeNumResults;
            mSignificantChangeNumResults += numResults;
            /*
             * Check if this chunck of wifi_significant_change results is a
             * continuation of a previous one.
             */
            if (mSignificantChangeMoreData) {
                mSignificantChangeResults =
                    (wifi_significant_change_result **)
                        realloc (mSignificantChangeResults,
                        sizeof(wifi_significant_change_result *) *
                                mSignificantChangeNumResults);
            } else {
                mSignificantChangeResults =
                    (wifi_significant_change_result **)
                        malloc (sizeof(wifi_significant_change_result *) *
                                mSignificantChangeNumResults);
            }

            if (!mSignificantChangeResults) {
                ALOGE("%s: Failed to alloc memory for results array. Exit.\n",
                    __func__);
                ret = WIFI_ERROR_OUT_OF_MEMORY;
                break;
            }
            /* Initialize the newly allocated memory area with 0. */
            memset((u8 *)mSignificantChangeResults + sizeOfObtainedResults, 0,
                    sizeof(wifi_significant_change_result *) *
                                numResults);
            ALOGD("%s: mSignificantChangeMoreData = %d",
                    __func__, mSignificantChangeMoreData);

            for (scanResultsInfo = (struct nlattr *) nla_data(tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_LIST]),
                rem = nla_len(tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_LIST]);
                nla_ok(scanResultsInfo, rem);
                scanResultsInfo = nla_next(scanResultsInfo, &(rem)))
            {
                u32 num_rssi = 0;
                u32 resultsBufSize = 0;
                struct nlattr *tb2[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_MAX + 1];
                nla_parse(tb2, QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_MAX,
                    (struct nlattr *) nla_data(scanResultsInfo),
                    nla_len(scanResultsInfo), NULL);
                if (!tb2[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SIGNIFICANT_CHANGE_RESULT_NUM_RSSI
                    ])
                {
                    ALOGE("%s: QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_"
                        "SIGNIFICANT_CHANGE_RESULT_NUM_RSSI not found. "
                        "Exit.", __func__);
                    ret = WIFI_ERROR_INVALID_ARGS;
                    break;
                }
                num_rssi = nla_get_u32(tb2[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SIGNIFICANT_CHANGE_RESULT_NUM_RSSI
                        ]);
                resultsBufSize = sizeof(wifi_significant_change_result) +
                            num_rssi * sizeof(wifi_rssi);
                mSignificantChangeResults[index] =
                    (wifi_significant_change_result *) malloc (resultsBufSize);

                if (!mSignificantChangeResults[index]) {
                    ALOGE("%s: Failed to alloc memory for results array. Exit.\n",
                        __func__);
                    ret = WIFI_ERROR_OUT_OF_MEMORY;
                    break;
                }
                /* Initialize the newly allocated memory area with 0. */
                memset((u8 *)mSignificantChangeResults[index],
                        0, resultsBufSize);

                ALOGE("%s: For Significant Change results[%d], num_rssi:%d\n",
                    __func__, index, num_rssi);
                index++;
            }

            ALOGE("%s: Extract significant change results.\n", __func__);
            startingIndex =
                mSignificantChangeNumResults - numResults;
            ret = gscan_get_significant_change_results(numResults,
                                                mSignificantChangeResults,
                                                startingIndex,
                                                tbVendor);
            /* If a parsing error occurred, exit and proceed for cleanup. */
            if (ret)
                break;
            /* To support fragmentation from firmware, monitor the
             * MORE_DTATA flag and cache results until MORE_DATA = 0.
             * Only then we can pass on the results to framework through
             * the callback function.
             */
            if (!tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_MORE_DATA]) {
                ALOGE("%s: GSCAN_RESULTS_NUM_RESULTS_MORE_DATA not"
                    " found. Stop parsing and exit.", __func__);
                break;
            }
            mSignificantChangeMoreData = nla_get_u8(
                tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_MORE_DATA]);
            ALOGE("%s: More data = %d. \n",
                __func__, mSignificantChangeMoreData);

            /* Send the results if no more result fragments are expected */
            if (!mSignificantChangeMoreData) {
                ALOGE("%s: Invoking the callback. \n", __func__);
                (*mHandler.on_significant_change)(reqId,
                                              mSignificantChangeNumResults,
                                              mSignificantChangeResults);
                /* Reset flag and num counter. */
                for (index = 0; index  < mSignificantChangeNumResults; index++)
                {
                    free(mSignificantChangeResults[index]);
                    mSignificantChangeResults[index] = NULL;
                }
                free(mSignificantChangeResults);
                mSignificantChangeResults = NULL;
                mSignificantChangeNumResults = 0;
                mSignificantChangeMoreData = false;
            }
        }
        break;

        case QCA_NL80211_VENDOR_SUBCMD_GSCAN_SCAN_EVENT:
        {
            wifi_scan_event scanEvent;
            u32 scanEventStatus = 0;
            wifi_request_id reqId;

            ALOGD("Event QCA_NL80211_VENDOR_SUBCMD_GSCAN_SCAN_EVENT "
                "received.");

            if (!tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_REQUEST_ID])
            {
                ALOGE("%s: ATTR_GSCAN_RESULTS_REQUEST_ID not found. Exit.",
                    __func__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            reqId = nla_get_u32(
                    tbVendor[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_REQUEST_ID]
                    );
            /* If this is not for us, just ignore it. */
            if (reqId != mRequestId) {
                ALOGE("%s: Event has Req. ID:%d <> ours:%d",
                    __func__, reqId, mRequestId);
                break;
            }

            if (!tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_EVENT_TYPE]) {
                ALOGE("%s: GSCAN_RESULTS_SCAN_EVENT_TYPE not"
                    " found. Stop parsing and exit.", __func__);
                break;
            }
            scanEvent = (wifi_scan_event) nla_get_u8(tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_EVENT_TYPE]);

            if (!tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_EVENT_STATUS]) {
                ALOGE("%s: GSCAN_RESULTS_SCAN_EVENT_STATUS not"
                    " found. Stop parsing and exit.", __func__);
                break;
            }
            scanEventStatus = nla_get_u32(tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_EVENT_STATUS]);

            ALOGE("%s: Scan event type: %d, status = %d. \n", __func__,
                                    scanEvent, scanEventStatus);
            /* Send the results if no more result fragments are expected. */
            (*mHandler.on_scan_event)(scanEvent, scanEventStatus);
        }
        break;

        default:
            /* Error case should not happen print log */
            ALOGE("%s: Wrong GScan subcmd received %d", __func__, mSubcmd);
    }

    /* A parsing error occurred, do the cleanup of gscan result lists. */
    if (ret) {
        switch(mSubcmd)
        {
            case QCA_NL80211_VENDOR_SUBCMD_GSCAN_FULL_SCAN_RESULT:
            {
                free(result);
                result = NULL;
            }
            break;

            case QCA_NL80211_VENDOR_SUBCMD_GSCAN_HOTLIST_AP_FOUND:
            {
                /* Reset flag and num counter. */
                free(mHotlistApFoundResults);
                mHotlistApFoundResults = NULL;
                mHotlistApFoundMoreData = false;
                mHotlistApFoundNumResults = 0;
            }
            break;

            case QCA_NL80211_VENDOR_SUBCMD_GSCAN_SIGNIFICANT_CHANGE:
            {
                for (i = 0; i < mSignificantChangeNumResults; i++)
                {
                    if (mSignificantChangeResults[i]) {
                        free(mSignificantChangeResults[i]);
                        mSignificantChangeResults[i] = NULL;
                    }
                }
                free(mSignificantChangeResults);
                mSignificantChangeResults = NULL;
                mSignificantChangeNumResults = 0;
                mSignificantChangeMoreData = false;
            }
            break;

            case QCA_NL80211_VENDOR_SUBCMD_GSCAN_SCAN_RESULTS_AVAILABLE:
            break;

            case QCA_NL80211_VENDOR_SUBCMD_GSCAN_SCAN_EVENT:
            break;

            default:
                ALOGE("%s: Parsing err handler: wrong GScan subcmd "
                    "received %d", __func__, mSubcmd);
        }
    }
    return NL_SKIP;
}
