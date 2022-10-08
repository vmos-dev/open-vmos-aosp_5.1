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

#include "sync.h"
#define LOG_TAG  "WifiHAL"
#include <utils/Log.h>
#include <time.h>

#include "common.h"
#include "cpp_bindings.h"
#include "gscancommand.h"
#include "gscan_event_handler.h"

#define GSCAN_EVENT_WAIT_TIME_SECONDS 4

/* Used to handle gscan command events from driver/firmware. */
GScanCommandEventHandler *GScanStartCmdEventHandler = NULL;
GScanCommandEventHandler *GScanSetBssidHotlistCmdEventHandler = NULL;
GScanCommandEventHandler *GScanSetSignificantChangeCmdEventHandler = NULL;

/* Implementation of the API functions exposed in gscan.h */
wifi_error wifi_get_valid_channels(wifi_interface_handle handle,
       int band, int max_channels, wifi_channel *channels, int *num_channels)
{
    int requestId, ret = 0;
    GScanCommand *gScanCommand;
    struct nlattr *nlData;
    interface_info *ifaceInfo = getIfaceInfo(handle);
    wifi_handle wifiHandle = getWifiHandle(handle);

    if (channels == NULL) {
        ALOGE("%s: NULL channels pointer provided. Exit.",
            __func__);
        return WIFI_ERROR_INVALID_ARGS;
    }

    /* No request id from caller, so generate one and pass it on to the driver.
     * Generate one randomly.
     */
    srand( time(NULL) );
    requestId = rand();

    gScanCommand = new GScanCommand(
                            wifiHandle,
                            requestId,
                            OUI_QCA,
                            QCA_NL80211_VENDOR_SUBCMD_GSCAN_GET_VALID_CHANNELS);
    if (gScanCommand == NULL) {
        ALOGE("%s: Error GScanCommand NULL", __func__);
        return WIFI_ERROR_UNKNOWN;
    }
    /* Create the NL message. */
    ret = gScanCommand->create();
    if (ret < 0)
        goto cleanup;

    /* Set the interface Id of the message. */
    ret = gScanCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = gScanCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    if (gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID,
            requestId) ||
        gScanCommand->put_u32(
        QCA_WLAN_VENDOR_ATTR_GSCAN_GET_VALID_CHANNELS_CONFIG_PARAM_WIFI_BAND,
            band) ||
        gScanCommand->put_u32(
        QCA_WLAN_VENDOR_ATTR_GSCAN_GET_VALID_CHANNELS_CONFIG_PARAM_MAX_CHANNELS,
            max_channels) )
    {
        goto cleanup;
    }
    gScanCommand->attr_end(nlData);
    /* Populate the input received from caller/framework. */
    gScanCommand->setMaxChannels(max_channels);
    gScanCommand->setChannels(channels);
    gScanCommand->setNumChannelsPtr(num_channels);

    /* Send the msg and wait for a response. */
    ret = gScanCommand->requestResponse();
    if (ret) {
        ALOGE("%s: Error %d happened. ", __func__, ret);
    }

cleanup:
    ALOGI("%s: Delete object.", __func__);
    delete gScanCommand;
    return (wifi_error)ret;
}

void get_gscan_capabilities_cb(int status, wifi_gscan_capabilities capa)
{
    ALOGD("%s: Status = %d.", __func__, status);
    ALOGD("%s: Capabilities. max_ap_cache_per_scan:%d, "
            "max_bssid_history_entries:%d, max_hotlist_aps:%d, "
            "max_rssi_sample_size:%d, max_scan_buckets:%d, "
            "max_scan_cache_size:%d, max_scan_reporting_threshold:%d, "
            "max_significant_wifi_change_aps:%d.",
            __func__, capa.max_ap_cache_per_scan,
            capa.max_bssid_history_entries,
            capa.max_hotlist_aps, capa.max_rssi_sample_size,
            capa.max_scan_buckets,
            capa.max_scan_cache_size, capa.max_scan_reporting_threshold,
            capa.max_significant_wifi_change_aps);
}

wifi_error wifi_get_gscan_capabilities(wifi_interface_handle handle,
                                 wifi_gscan_capabilities *capabilities)
{
    int requestId, ret = 0;
    GScanCommand *gScanCommand;
    struct nlattr *nlData;
    wifi_gscan_capabilities tCapabilities;
    interface_info *ifaceInfo = getIfaceInfo(handle);
    wifi_handle wifiHandle = getWifiHandle(handle);

    if (capabilities == NULL) {
        ALOGE("%s: NULL capabilities pointer provided. Exit.",
            __func__);
        return WIFI_ERROR_INVALID_ARGS;
    }

    /* No request id from caller, so generate one and pass it on to the driver.
     * Generate it randomly.
     */
    srand(time(NULL));
    requestId = rand();

    gScanCommand = new GScanCommand(
                            wifiHandle,
                            requestId,
                            OUI_QCA,
                            QCA_NL80211_VENDOR_SUBCMD_GSCAN_GET_CAPABILITIES);
    if (gScanCommand == NULL) {
        ALOGE("%s: Error GScanCommand NULL", __func__);
        return WIFI_ERROR_UNKNOWN;
    }

    GScanCallbackHandler callbackHandler;
    memset(&callbackHandler, 0, sizeof(callbackHandler));
    callbackHandler.get_capabilities = get_gscan_capabilities_cb;

    ret = gScanCommand->setCallbackHandler(callbackHandler);
    if (ret < 0)
        goto cleanup;

    /* Create the NL message. */
    ret = gScanCommand->create();
    if (ret < 0)
        goto cleanup;

    /* Set the interface Id of the message. */
    ret = gScanCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = gScanCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    ret = gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID,
            requestId);
    if (ret < 0)
        goto cleanup;

    gScanCommand->attr_end(nlData);
    ret = gScanCommand->allocRspParams(eGScanGetCapabilitiesRspParams);
    if (ret != 0) {
        ALOGE("%s: Failed to allocate memory fo response struct. Error:%d",
            __func__, ret);
        goto cleanup;
    }

    gScanCommand->waitForRsp(true);
    ret = gScanCommand->requestEvent();
    if (ret != 0) {
        ALOGE("%s: requestEvent Error:%d",__func__, ret);
        goto cleanup;
    }

    gScanCommand->getGetCapabilitiesRspParams(capabilities, (u32 *)&ret);

cleanup:
    gScanCommand->freeRspParams(eGScanGetCapabilitiesRspParams);
    ALOGI("%s: Delete object.", __func__);
    delete gScanCommand;
    return (wifi_error)ret;
}

void start_gscan_cb(int status)
{
    ALOGD("%s: Status = %d.", __func__, status);
}

wifi_error wifi_start_gscan(wifi_request_id id,
                            wifi_interface_handle iface,
                            wifi_scan_cmd_params params,
                            wifi_scan_result_handler handler)
{
    int ret = 0;
    u32 i, j;
    GScanCommand *gScanCommand;
    struct nlattr *nlData;
    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);
    u32 num_scan_buckets, numChannelSpecs;
    wifi_scan_bucket_spec bucketSpec;
    struct nlattr *nlBuckectSpecList;

    /* Check if a similar request to start gscan was made earlier.
     * Right now we don't differentiate between the case where (i) the new
     * Request Id is different from the current one vs (ii) case where both
     * new and Request IDs are the same.
     */
    if (GScanStartCmdEventHandler) {
        if (id == GScanStartCmdEventHandler->get_request_id()) {
            ALOGE("wifi_start_gscan(): GSCAN Start for request Id %d is still "
                 "running. Exit", id);
            return WIFI_ERROR_TOO_MANY_REQUESTS;
        } else {
            ALOGE("wifi_start_gscan(): GSCAN Start for a different request "
                "Id %d is requested. Not supported. Exit", id);
            return WIFI_ERROR_NOT_SUPPORTED;
        }
    }

    gScanCommand = new GScanCommand(
                                wifiHandle,
                                id,
                                OUI_QCA,
                                QCA_NL80211_VENDOR_SUBCMD_GSCAN_START);
    if (gScanCommand == NULL) {
        ALOGE("wifi_start_gscan(): Error GScanCommand NULL");
        return WIFI_ERROR_UNKNOWN;
    }

    GScanCallbackHandler callbackHandler;
    memset(&callbackHandler, 0, sizeof(callbackHandler));
    callbackHandler.start = start_gscan_cb;

    ret = gScanCommand->setCallbackHandler(callbackHandler);
    if (ret < 0)
        goto cleanup;

    /* Create the NL message. */
    ret = gScanCommand->create();
    if (ret < 0)
        goto cleanup;

    /* Set the interface Id of the message. */
    ret = gScanCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = gScanCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    num_scan_buckets = (unsigned int)params.num_buckets > MAX_BUCKETS ?
                            MAX_BUCKETS : params.num_buckets;

    if (gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID,
            id) ||
        gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SCAN_CMD_PARAMS_BASE_PERIOD,
            params.base_period) ||
        gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SCAN_CMD_PARAMS_MAX_AP_PER_SCAN,
            params.max_ap_per_scan) ||
        gScanCommand->put_u8(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SCAN_CMD_PARAMS_REPORT_THRESHOLD,
            params.report_threshold) ||
        gScanCommand->put_u8(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SCAN_CMD_PARAMS_NUM_BUCKETS,
            num_scan_buckets))
    {
        goto cleanup;
    }

    nlBuckectSpecList =
        gScanCommand->attr_start(QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC);
    /* Add NL attributes for scan bucket specs . */
    for (i = 0; i < num_scan_buckets; i++) {
        bucketSpec = params.buckets[i];
        numChannelSpecs = (unsigned int)bucketSpec.num_channels > MAX_CHANNELS ?
                                MAX_CHANNELS : bucketSpec.num_channels;
        struct nlattr *nlBucketSpec = gScanCommand->attr_start(i);
        if (gScanCommand->put_u8(
                QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_INDEX,
                bucketSpec.bucket) ||
            gScanCommand->put_u8(
                QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_BAND,
                bucketSpec.band) ||
            gScanCommand->put_u32(
                QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_PERIOD,
                bucketSpec.period) ||
            gScanCommand->put_u8(
                QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_REPORT_EVENTS,
                bucketSpec.report_events) ||
            gScanCommand->put_u32(
                QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_NUM_CHANNEL_SPECS,
                numChannelSpecs))
        {
            goto cleanup;
        }

        struct nlattr *nl_channelSpecList =
            gScanCommand->attr_start(QCA_WLAN_VENDOR_ATTR_GSCAN_CHANNEL_SPEC);

        /* Add NL attributes for scan channel specs . */
        for (j = 0; j < numChannelSpecs; j++) {
            struct nlattr *nl_channelSpec = gScanCommand->attr_start(j);
            wifi_scan_channel_spec channel_spec = bucketSpec.channels[j];

            if ( gScanCommand->put_u32(
                    QCA_WLAN_VENDOR_ATTR_GSCAN_CHANNEL_SPEC_CHANNEL,
                    channel_spec.channel) ||
                gScanCommand->put_u32(
                    QCA_WLAN_VENDOR_ATTR_GSCAN_CHANNEL_SPEC_DWELL_TIME,
                    channel_spec.dwellTimeMs) ||
                gScanCommand->put_u8(
                    QCA_WLAN_VENDOR_ATTR_GSCAN_CHANNEL_SPEC_PASSIVE,
                    channel_spec.passive) )
            {
                goto cleanup;
            }

            gScanCommand->attr_end(nl_channelSpec);
        }
        gScanCommand->attr_end(nl_channelSpecList);
        gScanCommand->attr_end(nlBucketSpec);
    }
    gScanCommand->attr_end(nlBuckectSpecList);

    gScanCommand->attr_end(nlData);

    ret = gScanCommand->allocRspParams(eGScanStartRspParams);
    if (ret != 0) {
        ALOGE("wifi_start_gscan(): Failed to allocate memory to the response "
            "struct. Error:%d", ret);
        goto cleanup;
    }

    /* Set the callback handler functions for related events. */
    callbackHandler.on_scan_results_available =
                        handler.on_scan_results_available;
    callbackHandler.on_full_scan_result = handler.on_full_scan_result;
    callbackHandler.on_scan_event = handler.on_scan_event;
    /* Create an object to handle the related events from firmware/driver. */
    GScanStartCmdEventHandler = new GScanCommandEventHandler(
                                wifiHandle,
                                id,
                                OUI_QCA,
                                QCA_NL80211_VENDOR_SUBCMD_GSCAN_START,
                                callbackHandler);
    if (GScanStartCmdEventHandler == NULL) {
        ALOGE("wifi_start_gscan(): Error GScanStartCmdEventHandler NULL");
        ret = WIFI_ERROR_UNKNOWN;
        goto cleanup;
    }

    gScanCommand->waitForRsp(true);
    ret = gScanCommand->requestEvent();
    if (ret != 0) {
        ALOGE("wifi_start_gscan(): requestEvent Error:%d", ret);
        goto cleanup;
    }

    gScanCommand->getStartGScanRspParams((u32 *)&ret);
    if (ret != 0)
    {
        goto cleanup;
    }

cleanup:
    gScanCommand->freeRspParams(eGScanStartRspParams);
    ALOGI("wifi_start_gscan(): Delete object.");
    delete gScanCommand;
    /* Delete the command event handler object if ret != 0 */
    if (ret && GScanStartCmdEventHandler) {
        ALOGI("wifi_start_gscan(): Error ret:%d, delete event handler object.",
            ret);
        delete GScanStartCmdEventHandler;
        GScanStartCmdEventHandler = NULL;
    }
    return (wifi_error)ret;

}

void stop_gscan_cb(int status)
{
    ALOGD("%s: Status = %d.", __func__, status);
}

wifi_error wifi_stop_gscan(wifi_request_id id,
                            wifi_interface_handle iface)
{
    int ret = 0;
    GScanCommand *gScanCommand;
    struct nlattr *nlData;

    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);

    ALOGI("Stopping GScan, halHandle = %p", wifiHandle);

    if (GScanStartCmdEventHandler) {
        if (id != GScanStartCmdEventHandler->get_request_id()) {
            ALOGE("wifi_stop_gscan: GSCAN Stop requested for request Id %d "
                "not matching that of existing GScan Start:%d. Exit",
                id, GScanStartCmdEventHandler->get_request_id());
            return WIFI_ERROR_INVALID_REQUEST_ID;
        }
    } else {
        ALOGE("wifi_stop_gscan: GSCAN isn't running or already stopped. "
            "Nothing to do. Exit");
        return WIFI_ERROR_NOT_AVAILABLE;
    }

    gScanCommand = new GScanCommand(
                                wifiHandle,
                                id,
                                OUI_QCA,
                                QCA_NL80211_VENDOR_SUBCMD_GSCAN_STOP);
    if (gScanCommand == NULL) {
        ALOGE("%s: Error GScanCommand NULL", __func__);
        return WIFI_ERROR_UNKNOWN;
    }

    GScanCallbackHandler callbackHandler;
    memset(&callbackHandler, 0, sizeof(callbackHandler));
    callbackHandler.stop = stop_gscan_cb;

    ret = gScanCommand->setCallbackHandler(callbackHandler);
    if (ret < 0)
        goto cleanup;

    /* Create the NL message. */
    ret = gScanCommand->create();
    if (ret < 0)
        goto cleanup;

    /* Set the interface Id of the message. */
    ret = gScanCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = gScanCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    ret = gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID,
            id);
    if (ret < 0)
        goto cleanup;

    gScanCommand->attr_end(nlData);

    ret = gScanCommand->allocRspParams(eGScanStopRspParams);
    if (ret != 0) {
        ALOGE("%s: Failed to allocate memory to the response struct. "
            "Error:%d", __func__, ret);
        goto cleanup;
    }

    gScanCommand->waitForRsp(true);
    ret = gScanCommand->requestEvent();
    if (ret != 0) {
        ALOGE("%s: requestEvent Error:%d",__func__, ret);
        goto cleanup;
    }

    gScanCommand->getStopGScanRspParams((u32 *)&ret);
    if (ret != 0)
    {
        goto cleanup;
    }

    /* Delete different GSCAN event handlers for the specified Request ID. */
    if (GScanStartCmdEventHandler) {
        delete GScanStartCmdEventHandler;
        GScanStartCmdEventHandler = NULL;
    }

cleanup:
    gScanCommand->freeRspParams(eGScanStopRspParams);
    ALOGI("%s: Delete object.", __func__);
    delete gScanCommand;
    return (wifi_error)ret;
}

void set_bssid_hotlist_cb(int status)
{
    ALOGD("%s: Status = %d.", __func__, status);
}

/* Set the GSCAN BSSID Hotlist. */
wifi_error wifi_set_bssid_hotlist(wifi_request_id id,
                                    wifi_interface_handle iface,
                                    wifi_bssid_hotlist_params params,
                                    wifi_hotlist_ap_found_handler handler)
{
    int i, numAp, ret = 0;
    GScanCommand *gScanCommand;
    struct nlattr *nlData, *nlApThresholdParamList;
    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);

    ALOGD("Setting GScan BSSID Hotlist, halHandle = %p", wifiHandle);

    /* Check if a similar request to set bssid hotlist was made earlier.
     * Right now we don't differentiate between the case where (i) the new
     * Request Id is different from the current one vs (ii) case where both
     * new and Request IDs are the same.
     */
    if (GScanSetBssidHotlistCmdEventHandler)
        if (id == GScanSetBssidHotlistCmdEventHandler->get_request_id()) {
            ALOGE("%s: GSCAN Set BSSID Hotlist for request Id %d is still"
                "running. Exit", __func__, id);
            return WIFI_ERROR_TOO_MANY_REQUESTS;
        } else {
            ALOGD("%s: GSCAN Set BSSID Hotlist for a different Request Id:%d"
                "is requested. Not supported. Exit", __func__, id);
            return WIFI_ERROR_NOT_SUPPORTED;
        }

    gScanCommand =
        new GScanCommand(
                    wifiHandle,
                    id,
                    OUI_QCA,
                    QCA_NL80211_VENDOR_SUBCMD_GSCAN_SET_BSSID_HOTLIST);
    if (gScanCommand == NULL) {
        ALOGE("%s: Error GScanCommand NULL", __func__);
        return WIFI_ERROR_UNKNOWN;
    }

    GScanCallbackHandler callbackHandler;
    memset(&callbackHandler, 0, sizeof(callbackHandler));
    callbackHandler.set_bssid_hotlist = set_bssid_hotlist_cb,

    ret = gScanCommand->setCallbackHandler(callbackHandler);
    if (ret < 0)
        goto cleanup;

    /* Create the NL message. */
    ret = gScanCommand->create();
    if (ret < 0)
        goto cleanup;

    /* Set the interface Id of the message. */
    ret = gScanCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = gScanCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    numAp = (unsigned int)params.num_ap > MAX_HOTLIST_APS ? MAX_HOTLIST_APS : params.num_ap;
    if (gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID,
            id) ||
        gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_BSSID_HOTLIST_PARAMS_NUM_AP,
            numAp))
    {
        goto cleanup;
    }

    /* Add the vendor specific attributes for the NL command. */
    nlApThresholdParamList =
        gScanCommand->attr_start(
                                QCA_WLAN_VENDOR_ATTR_GSCAN_AP_THRESHOLD_PARAM);
    if (!nlApThresholdParamList)
        goto cleanup;

    /* Add nested NL attributes for AP Threshold Param. */
    for (i = 0; i < numAp; i++) {
        ap_threshold_param apThreshold = params.ap[i];
        struct nlattr *nlApThresholdParam = gScanCommand->attr_start(i);
        if (!nlApThresholdParam)
            goto cleanup;
        if (gScanCommand->put_addr(
                QCA_WLAN_VENDOR_ATTR_GSCAN_AP_THRESHOLD_PARAM_BSSID,
                apThreshold.bssid) ||
            gScanCommand->put_s32(
                QCA_WLAN_VENDOR_ATTR_GSCAN_AP_THRESHOLD_PARAM_RSSI_LOW,
                apThreshold.low) ||
            gScanCommand->put_s32(
                QCA_WLAN_VENDOR_ATTR_GSCAN_AP_THRESHOLD_PARAM_RSSI_HIGH,
                apThreshold.high) ||
            gScanCommand->put_u32(
                QCA_WLAN_VENDOR_ATTR_GSCAN_AP_THRESHOLD_PARAM_CHANNEL,
                apThreshold.channel))
        {
            goto cleanup;
        }
        gScanCommand->attr_end(nlApThresholdParam);
    }

    gScanCommand->attr_end(nlApThresholdParamList);

    gScanCommand->attr_end(nlData);

    ret = gScanCommand->allocRspParams(eGScanSetBssidHotlistRspParams);
    if (ret != 0) {
        ALOGE("%s: Failed to allocate memory to the response struct. "
            "Error:%d", __func__, ret);
        goto cleanup;
    }

    callbackHandler.on_hotlist_ap_found = handler.on_hotlist_ap_found;
    /* Create an object of the event handler class to take care of the
      * asychronous events on the north-bound.
      */
    GScanSetBssidHotlistCmdEventHandler = new GScanCommandEventHandler(
                            wifiHandle,
                            id,
                            OUI_QCA,
                            QCA_NL80211_VENDOR_SUBCMD_GSCAN_SET_BSSID_HOTLIST,
                            callbackHandler);
    if (GScanSetBssidHotlistCmdEventHandler == NULL) {
        ALOGE("%s: Error instantiating GScanSetBssidHotlistCmdEventHandler.",
            __func__);
        ret = WIFI_ERROR_UNKNOWN;
        goto cleanup;
    }

    ALOGD("%s: Handler object was created for HOTLIST_AP_FOUND.", __func__);

    gScanCommand->waitForRsp(true);
    ret = gScanCommand->requestEvent();
    if (ret != 0) {
        ALOGE("%s: requestEvent Error:%d",__func__, ret);
        goto cleanup;
    }

    gScanCommand->getSetBssidHotlistRspParams((u32 *)&ret);
    if (ret != 0)
    {
        goto cleanup;
    }

cleanup:
    gScanCommand->freeRspParams(eGScanSetBssidHotlistRspParams);
    ALOGI("%s: Delete object. ", __func__);
    delete gScanCommand;
    /* Delete the command event handler object if ret != 0 */
    if (ret && GScanSetBssidHotlistCmdEventHandler) {
        delete GScanSetBssidHotlistCmdEventHandler;
        GScanSetBssidHotlistCmdEventHandler = NULL;
    }
    return (wifi_error)ret;
}

void reset_bssid_hotlist_cb(int status)
{
    ALOGD("%s: Status = %d.", __func__, status);
}

wifi_error wifi_reset_bssid_hotlist(wifi_request_id id,
                            wifi_interface_handle iface)
{
    int ret = 0;
    GScanCommand *gScanCommand;
    struct nlattr *nlData;
    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);

    ALOGE("Resetting GScan BSSID Hotlist, halHandle = %p", wifiHandle);

    if (GScanSetBssidHotlistCmdEventHandler) {
        if (id != GScanSetBssidHotlistCmdEventHandler->get_request_id()) {
            ALOGE("%s: GSCAN Reset Hotlist BSSID requested for request Id %d"
                "not matching that of existing GScan Set Hotlist BSSID:%d. "
                "Exit", __func__, id,
                GScanSetBssidHotlistCmdEventHandler->get_request_id());
            return WIFI_ERROR_INVALID_REQUEST_ID;
        }
    }

    gScanCommand = new GScanCommand(
                        wifiHandle,
                        id,
                        OUI_QCA,
                        QCA_NL80211_VENDOR_SUBCMD_GSCAN_RESET_BSSID_HOTLIST);

    if (gScanCommand == NULL) {
        ALOGE("%s: Error GScanCommand NULL", __func__);
        return WIFI_ERROR_UNKNOWN;
    }

    GScanCallbackHandler callbackHandler;
    memset(&callbackHandler, 0, sizeof(callbackHandler));
    callbackHandler.reset_bssid_hotlist = reset_bssid_hotlist_cb;

    ret = gScanCommand->setCallbackHandler(callbackHandler);
    if (ret < 0)
        goto cleanup;

    /* Create the NL message. */
    ret = gScanCommand->create();
    if (ret < 0)
        goto cleanup;

    /* Set the interface Id of the message. */
    ret = gScanCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = gScanCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    ret = gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID, id);
    if (ret < 0)
        goto cleanup;

    gScanCommand->attr_end(nlData);

    ret = gScanCommand->allocRspParams(eGScanResetBssidHotlistRspParams);
    if (ret != 0) {
        ALOGE("%s: Failed to allocate memory to the response struct. "
            "Error:%d", __func__, ret);
        goto cleanup;
    }

    gScanCommand->waitForRsp(true);
    ret = gScanCommand->requestEvent();
    if (ret != 0) {
        ALOGE("%s: requestEvent Error:%d",__func__, ret);
        goto cleanup;
    }

    gScanCommand->getResetBssidHotlistRspParams((u32 *)&ret);
    if (ret != 0)
    {
        goto cleanup;
    }

    if (GScanSetBssidHotlistCmdEventHandler) {
        delete GScanSetBssidHotlistCmdEventHandler;
        GScanSetBssidHotlistCmdEventHandler = NULL;
    }

cleanup:
    gScanCommand->freeRspParams(eGScanResetBssidHotlistRspParams);
    ALOGI("%s: Delete object.", __func__);
    delete gScanCommand;
    return (wifi_error)ret;
}

void set_significant_change_cb(int status)
{
    ALOGD("%s: Status = %d.", __func__, status);
}

/* Set the GSCAN Significant AP Change list. */
wifi_error wifi_set_significant_change_handler(wifi_request_id id,
                                            wifi_interface_handle iface,
                                    wifi_significant_change_params params,
                                    wifi_significant_change_handler handler)
{
    int i, numAp, ret = 0;
    GScanCommand *gScanCommand;
    struct nlattr *nlData, *nlApThresholdParamList;
    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);

    ALOGE("Setting GScan Significant Change, halHandle = %p", wifiHandle);

    /* Check if a similar request to set significant change was made earlier.
     * Right now we don't differentiate between the case where (i) the new
     * Request Id is different from the current one vs (ii) both new and
     * Request Ids are the same.
     */
    if (GScanSetSignificantChangeCmdEventHandler)
        if (id == GScanSetSignificantChangeCmdEventHandler->get_request_id()) {
            ALOGE("%s: GSCAN Set Significant Change for request Id %d is still"
                "running. Exit", __func__, id);
            return WIFI_ERROR_TOO_MANY_REQUESTS;
        } else {
            ALOGE("%s: GSCAN Set Significant Change for a different Request "
                "Id:%d is requested. Not supported. Exit", __func__, id);
            return WIFI_ERROR_NOT_SUPPORTED;
        }

    gScanCommand = new GScanCommand(
                    wifiHandle,
                    id,
                    OUI_QCA,
                    QCA_NL80211_VENDOR_SUBCMD_GSCAN_SET_SIGNIFICANT_CHANGE);
    if (gScanCommand == NULL) {
        ALOGE("%s: Error GScanCommand NULL", __func__);
        return WIFI_ERROR_UNKNOWN;
    }

    GScanCallbackHandler callbackHandler;
    memset(&callbackHandler, 0, sizeof(callbackHandler));
    callbackHandler.set_significant_change = set_significant_change_cb;

    ret = gScanCommand->setCallbackHandler(callbackHandler);
    if (ret < 0)
        goto cleanup;

    /* Create the NL message. */
    ret = gScanCommand->create();
    if (ret < 0)
        goto cleanup;

    /* Set the interface Id of the message. */
    ret = gScanCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = gScanCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    numAp = (unsigned int)params.num_ap > MAX_SIGNIFICANT_CHANGE_APS ?
        MAX_SIGNIFICANT_CHANGE_APS : params.num_ap;

    if (gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID,
            id) ||
        gScanCommand->put_u32(
        QCA_WLAN_VENDOR_ATTR_GSCAN_SIGNIFICANT_CHANGE_PARAMS_RSSI_SAMPLE_SIZE,
            params.rssi_sample_size) ||
        gScanCommand->put_u32(
        QCA_WLAN_VENDOR_ATTR_GSCAN_SIGNIFICANT_CHANGE_PARAMS_LOST_AP_SAMPLE_SIZE,
            params.lost_ap_sample_size) ||
        gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SIGNIFICANT_CHANGE_PARAMS_MIN_BREACHING,
            params.min_breaching) ||
        gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SIGNIFICANT_CHANGE_PARAMS_NUM_AP,
            numAp))
    {
        goto cleanup;
    }

    /* Add the vendor specific attributes for the NL command. */
    nlApThresholdParamList =
        gScanCommand->attr_start(
                                QCA_WLAN_VENDOR_ATTR_GSCAN_AP_THRESHOLD_PARAM);
    if (!nlApThresholdParamList)
        goto cleanup;

    /* Add nested NL attributes for AP Threshold Param list. */
    for (i = 0; i < numAp; i++) {
        ap_threshold_param apThreshold = params.ap[i];
        struct nlattr *nlApThresholdParam = gScanCommand->attr_start(i);
        if (!nlApThresholdParam)
            goto cleanup;
        if ( gScanCommand->put_addr(
                QCA_WLAN_VENDOR_ATTR_GSCAN_AP_THRESHOLD_PARAM_BSSID,
                apThreshold.bssid) ||
            gScanCommand->put_s32(
                QCA_WLAN_VENDOR_ATTR_GSCAN_AP_THRESHOLD_PARAM_RSSI_LOW,
                apThreshold.low) ||
            gScanCommand->put_s32(
                QCA_WLAN_VENDOR_ATTR_GSCAN_AP_THRESHOLD_PARAM_RSSI_HIGH,
                apThreshold.high) ||
            gScanCommand->put_u32(
                QCA_WLAN_VENDOR_ATTR_GSCAN_AP_THRESHOLD_PARAM_CHANNEL,
                apThreshold.channel) )
        {
            goto cleanup;
        }
        gScanCommand->attr_end(nlApThresholdParam);
    }

    gScanCommand->attr_end(nlApThresholdParamList);

    gScanCommand->attr_end(nlData);

    ret = gScanCommand->allocRspParams(eGScanSetSignificantChangeRspParams);
    if (ret != 0) {
        ALOGE("%s: Failed to allocate memory to the response struct. "
            "Error:%d", __func__, ret);
        goto cleanup;
    }

    callbackHandler.on_significant_change = handler.on_significant_change;
    /* Create an object of the event handler class to take care of the
      * asychronous events on the north-bound.
      */
    GScanSetSignificantChangeCmdEventHandler = new GScanCommandEventHandler(
                     wifiHandle,
                     id,
                     OUI_QCA,
                     QCA_NL80211_VENDOR_SUBCMD_GSCAN_SET_SIGNIFICANT_CHANGE,
                     callbackHandler);
    if (GScanSetSignificantChangeCmdEventHandler == NULL) {
        ALOGE("%s: Error in instantiating, "
            "GScanSetSignificantChangeCmdEventHandler.",
            __func__);
        ret = WIFI_ERROR_UNKNOWN;
        goto cleanup;
    }

    ALOGD("%s: Event handler object was created for SIGNIFICANT_CHANGE.",
            __func__);

    gScanCommand->waitForRsp(true);
    ret = gScanCommand->requestEvent();
    if (ret != 0) {
        ALOGE("%s: requestEvent Error:%d",__func__, ret);
        goto cleanup;
    }

    gScanCommand->getSetSignificantChangeRspParams((u32 *)&ret);
    if (ret != 0)
    {
        goto cleanup;
    }

cleanup:
    gScanCommand->freeRspParams(eGScanSetSignificantChangeRspParams);
    ALOGI("%s: Delete object.", __func__);
    /* Delete the command event handler object if ret != 0 */
    if (ret && GScanSetSignificantChangeCmdEventHandler) {
        delete GScanSetSignificantChangeCmdEventHandler;
        GScanSetSignificantChangeCmdEventHandler = NULL;
    }
    delete gScanCommand;
    return (wifi_error)ret;
}

void reset_significant_change_cb(int status)
{
    ALOGD("%s: Status = %d.", __func__, status);
}

/* Clear the GSCAN Significant AP change list. */
wifi_error wifi_reset_significant_change_handler(wifi_request_id id,
                                            wifi_interface_handle iface)
{
    int ret = 0;
    GScanCommand *gScanCommand;
    struct nlattr *nlData;
    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);

    ALOGD("Resetting GScan Significant Change, halHandle = %p", wifiHandle);

    if (GScanSetSignificantChangeCmdEventHandler) {
        if (id != GScanSetSignificantChangeCmdEventHandler->get_request_id()) {
            ALOGE("%s: GSCAN Reset Significant Change requested for request "
                "Id %d not matching that of existing GScan Set Hotlist "
                "BSSID:%d. Exit", __func__, id,
                GScanSetSignificantChangeCmdEventHandler->get_request_id());
            return WIFI_ERROR_INVALID_REQUEST_ID;
        }
    }

    gScanCommand =
        new GScanCommand
                    (
                    wifiHandle,
                    id,
                    OUI_QCA,
                    QCA_NL80211_VENDOR_SUBCMD_GSCAN_RESET_SIGNIFICANT_CHANGE);
    if (gScanCommand == NULL) {
        ALOGE("%s: Error GScanCommand NULL", __func__);
        return WIFI_ERROR_UNKNOWN;
    }

    GScanCallbackHandler callbackHandler;
    memset(&callbackHandler, 0, sizeof(callbackHandler));
    callbackHandler.reset_significant_change = reset_significant_change_cb;

    ret = gScanCommand->setCallbackHandler(callbackHandler);
    if (ret < 0)
        goto cleanup;

    /* Create the NL message. */
    ret = gScanCommand->create();
    if (ret < 0)
        goto cleanup;

    /* Set the interface Id of the message. */
    ret = gScanCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = gScanCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    ret = gScanCommand->put_u32(
                    QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID,
                    id);
    if (ret < 0)
        goto cleanup;

    gScanCommand->attr_end(nlData);

    ret = gScanCommand->allocRspParams(eGScanResetSignificantChangeRspParams);
    if (ret != 0) {
        ALOGE("%s: Failed to allocate memory to the response struct. "
            "Error:%d", __func__, ret);
        goto cleanup;
    }

    gScanCommand->waitForRsp(true);
    ret = gScanCommand->requestEvent();
    if (ret != 0) {
        ALOGE("%s: requestEvent Error:%d",__func__, ret);
        goto cleanup;
    }

    gScanCommand->getResetSignificantChangeRspParams((u32 *)&ret);
    if (ret != 0)
    {
        goto cleanup;
    }

    if (GScanSetSignificantChangeCmdEventHandler) {
        delete GScanSetSignificantChangeCmdEventHandler;
        GScanSetSignificantChangeCmdEventHandler = NULL;
    }

cleanup:
    gScanCommand->freeRspParams(eGScanResetSignificantChangeRspParams);
    ALOGI("%s: Delete object.", __func__);
    delete gScanCommand;
    return (wifi_error)ret;
}

void get_gscan_cached_results_cb(u8 moreData, u32 numResults)
{
    ALOGD("%s: More data = %d.", __func__, moreData);
    ALOGD("%s: Number of cached results = %d.", __func__, numResults);
}

/* Get the GSCAN cached scan results. */
wifi_error wifi_get_cached_gscan_results(wifi_interface_handle iface,
                                                byte flush, int max,
                                                wifi_scan_result *results,
                                                int *num)
{
    int requestId, ret = 0;
    wifi_scan_result *result = results;
    u32 j = 0;
    int i = 0;
    u8 moreData = 0;
    u16 waitTime = GSCAN_EVENT_WAIT_TIME_SECONDS;
    GScanCommand *gScanCommand;
    struct nlattr *nlData;

    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);

    if (results == NULL) {
        ALOGE("%s: NULL results pointer provided. Exit.",
            __func__);
        return WIFI_ERROR_INVALID_ARGS;
    }

    /* No request id from caller, so generate one and pass it on to the driver. */
    /* Generate it randomly */
    srand(time(NULL));
    requestId = rand();

    ALOGE("Getting GScan Cached Results, halHandle = %p", wifiHandle);

    gScanCommand = new GScanCommand(
                        wifiHandle,
                        requestId,
                        OUI_QCA,
                        QCA_NL80211_VENDOR_SUBCMD_GSCAN_GET_CACHED_RESULTS);
    if (gScanCommand == NULL) {
        ALOGE("%s: Error GScanCommand NULL", __func__);
        return WIFI_ERROR_UNKNOWN;
    }

    GScanCallbackHandler callbackHandler;
    memset(&callbackHandler, 0, sizeof(callbackHandler));
    callbackHandler.get_cached_results = get_gscan_cached_results_cb;

    ret = gScanCommand->setCallbackHandler(callbackHandler);
    if (ret < 0)
        goto cleanup;

    /* Create the NL message. */
    ret = gScanCommand->create();
    if (ret < 0)
        goto cleanup;

    /* Set the interface Id of the message. */
    ret = gScanCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = gScanCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    if (ret < 0)
        goto cleanup;

    if (gScanCommand->put_u32(
         QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID,
            requestId) ||
        gScanCommand->put_u8(
         QCA_WLAN_VENDOR_ATTR_GSCAN_GET_CACHED_SCAN_RESULTS_CONFIG_PARAM_FLUSH,
            flush) ||
        gScanCommand->put_u32(
         QCA_WLAN_VENDOR_ATTR_GSCAN_GET_CACHED_SCAN_RESULTS_CONFIG_PARAM_MAX,
            max))
    {
        goto cleanup;
    }
    gScanCommand->attr_end(nlData);
    ret = gScanCommand->allocRspParams(eGScanGetCachedResultsRspParams);
    if (ret != 0) {
        ALOGE("%s: Failed to allocate memory fo response struct. Error:%d",
            __func__, ret);
        goto cleanup;
    }

    gScanCommand->waitForRsp(true);
    ret = gScanCommand->requestEvent();
    if (ret != 0) {
        ALOGE("%s: requestEvent Error:%d",__func__, ret);
        goto cleanup;
    }

    /* Read more data flag and number of results of retrieved cached results
     * from driver/firmware.
     * If more data is 0 or numResults >= max, return with results populated.
     * Otherwise, loop in 4s wait for next results fragment(s).
     */
    ret = gScanCommand->getGetCachedResultsRspParams(max,
                                               (u8 *)&moreData,
                                               num,
                                               results);
    while (!ret && moreData && (*num < max)) {
        int res = gScanCommand->timed_wait(waitTime);
        if (res == ETIMEDOUT) {
            ALOGE("%s: Time out happened.", __func__);
            /*Proceed to cleanup & return whatever data avaiable at this time*/
            goto cleanup;
        }
        ALOGD("%s: Command invoked return value:%d",__func__, res);
        /* Read the moreData and numResults again and possibly append new
         * cached results to the list.
         */
        ret = gScanCommand->getGetCachedResultsRspParams(max,
                                                   (u8 *)&moreData,
                                                   num,
                                                   results);
    }
    if (!ret) {
        for(i=0; i< *num; i++)
        {
            ALOGI("HAL:  Result : %d\n", i+1);
            ALOGI("HAL:  ts  %lld \n", result->ts);
            ALOGI("HAL:  SSID  %s \n", result->ssid);
            ALOGI("HAL:  BSSID: "
               "%02x:%02x:%02x:%02x:%02x:%02x \n",
               result->bssid[0], result->bssid[1], result->bssid[2],
               result->bssid[3], result->bssid[4], result->bssid[5]);
            ALOGI("HAL:  channel %d \n", result->channel);
            ALOGI("HAL:  rssi  %d \n", result->rssi);
            ALOGI("HAL:  rtt  %lld \n", result->rtt);
            ALOGI("HAL:  rtt_sd  %lld \n", result->rtt_sd);
            ALOGI("HAL:  beacon period  %d \n",
            result->beacon_period);
            ALOGI("HAL:  capability  %d \n", result->capability);
            ALOGI("HAL:  IE length  %d \n", result->ie_length);
            ALOGI("HAL:  IE Data \n");
            hexdump(result->ie_data, result->ie_length);
            result = (wifi_scan_result *)
               ((u8 *)&results[i] + sizeof(wifi_scan_result) +
                result->ie_length);
        }
    }
cleanup:
    gScanCommand->freeRspParams(eGScanGetCachedResultsRspParams);
    ALOGI("%s: Delete object.", __func__);
    delete gScanCommand;
    return (wifi_error)ret;
}

GScanCommand::GScanCommand(wifi_handle handle, int id, u32 vendor_id,
                                  u32 subcmd)
        : WifiVendorCommand(handle, id, vendor_id, subcmd)
{
    ALOGD("GScanCommand %p constructed", this);
    /* Initialize the member data variables here */
    mStartGScanRspParams = NULL;
    mStopGScanRspParams = NULL;
    mSetBssidHotlistRspParams = NULL;
    mResetBssidHotlistRspParams = NULL;
    mSetSignificantChangeRspParams = NULL;
    mResetSignificantChangeRspParams = NULL;
    mGetCapabilitiesRspParams = NULL;
    mGetCachedResultsRspParams = NULL;
    mGetCachedResultsNumResults = 0;
    mChannels = NULL;
    mMaxChannels = 0;
    mNumChannelsPtr = NULL;
    mWaitforRsp = false;

    mRequestId = id;
    memset(&mHandler, 0,sizeof(mHandler));
}

GScanCommand::~GScanCommand()
{
    ALOGD("GScanCommand %p destructor", this);
    unregisterVendorHandler(mVendor_id, mSubcmd);
}


/* This function implements creation of Vendor command */
int GScanCommand::create() {
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

     ALOGI("%s: mVendor_id = %d, Subcmd = %d.",
        __func__, mVendor_id, mSubcmd);

out:
    return ret;
}

/* Callback handlers registered for nl message send */
static int error_handler_gscan(struct sockaddr_nl *nla, struct nlmsgerr *err,
                                   void *arg)
{
    struct sockaddr_nl *tmp;
    int *ret = (int *)arg;
    tmp = nla;
    *ret = err->error;
    ALOGE("%s: Error code:%d (%s)", __func__, *ret, strerror(-(*ret)));
    return NL_STOP;
}

/* Callback handlers registered for nl message send */
static int ack_handler_gscan(struct nl_msg *msg, void *arg)
{
    int *ret = (int *)arg;
    struct nl_msg * a;

    ALOGE("%s: called", __func__);
    a = msg;
    *ret = 0;
    return NL_STOP;
}

/* Callback handlers registered for nl message send */
static int finish_handler_gscan(struct nl_msg *msg, void *arg)
{
  int *ret = (int *)arg;
  struct nl_msg * a;

  ALOGE("%s: called", __func__);
  a = msg;
  *ret = 0;
  return NL_SKIP;
}

/*
 * Override base class requestEvent and implement little differently here.
 * This will send the request message.
 * We don't wait for any response back in case of gscan as it is asynchronous,
 * thus no wait for condition.
 */
int GScanCommand::requestEvent()
{
    int res = -1;
    struct nl_cb *cb;

    ALOGD("%s: Entry.", __func__);

    cb = nl_cb_alloc(NL_CB_DEFAULT);
    if (!cb) {
        ALOGE("%s: Callback allocation failed",__func__);
        res = -1;
        goto out;
    }

    /* Send message */
    ALOGE("%s:Handle:%p Socket Value:%p", __func__, mInfo, mInfo->cmd_sock);
    res = nl_send_auto_complete(mInfo->cmd_sock, mMsg.getMessage());
    if (res < 0)
        goto out;
    res = 1;

    nl_cb_err(cb, NL_CB_CUSTOM, error_handler_gscan, &res);
    nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler_gscan, &res);
    nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler_gscan, &res);

    /* Err is populated as part of finish_handler. */
    while (res > 0){
         nl_recvmsgs(mInfo->cmd_sock, cb);
    }

    ALOGD("%s: Msg sent, res=%d, mWaitForRsp=%d", __func__, res, mWaitforRsp);
    /* Only wait for the asynchronous event if HDD returns success, res=0 */
    if (!res && (mWaitforRsp == true)) {
        struct timespec abstime;
        abstime.tv_sec = 4;
        abstime.tv_nsec = 0;
        res = mCondition.wait(abstime);
        if (res == ETIMEDOUT)
        {
            ALOGE("%s: Time out happened.", __func__);
        }
        ALOGD("%s: Command invoked return value:%d, mWaitForRsp=%d",
            __func__, res, mWaitforRsp);
    }
out:
    /* Free the VendorData */
    if (mVendorData) {
        free(mVendorData);
    }
    mVendorData = NULL;
    /* Cleanup the mMsg */
    mMsg.destroy();
    return res;
}

int GScanCommand::requestResponse()
{
    ALOGD("%s: request a response", __func__);
    return WifiCommand::requestResponse(mMsg);
}

int GScanCommand::handleResponse(WifiEvent &reply) {
    ALOGI("Received a GScan response message from Driver");
    u32 status;
    int i = 0;
    WifiVendorCommand::handleResponse(reply);

    switch(mSubcmd)
    {
        case QCA_NL80211_VENDOR_SUBCMD_GSCAN_GET_VALID_CHANNELS:
            {
                struct nlattr *tb_vendor[
                    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_MAX + 1];
                nla_parse(tb_vendor, QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_MAX,
                            (struct nlattr *)mVendorData,mDataLen, NULL);

                if (tb_vendor[
                    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_NUM_CHANNELS]) {
                    u32 val;
                    val = nla_get_u32(
                        tb_vendor[
                        QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_NUM_CHANNELS]);

                    ALOGD("%s: Num channels : %d", __func__, val);
                    val = val > (unsigned int)mMaxChannels ?
                          (unsigned int)mMaxChannels : val;
                    *mNumChannelsPtr = val;

                    /* Extract the list of channels. */
                    if (*mNumChannelsPtr > 0 &&
                        tb_vendor[
                        QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CHANNELS]) {
                        nla_memcpy(mChannels,
                            tb_vendor[
                            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CHANNELS],
                            sizeof(wifi_channel) * (*mNumChannelsPtr));
                    }

                    ALOGD("%s: Get valid channels response received.",
                        __func__);
                    ALOGD("%s: Num channels : %d",
                        __func__, *mNumChannelsPtr);
                    ALOGD("%s: List of valid channels is: ", __func__);
                    for(i = 0; i < *mNumChannelsPtr; i++)
                    {
                        ALOGD("%u", *(mChannels + i));
                    }
                }
            }
            break;
        default :
            ALOGE("%s: Wrong GScan subcmd response received %d",
                __func__, mSubcmd);
    }
    return NL_SKIP;
}

/* Called to parse and extract cached results. */
static int gscan_get_cached_results(u32 num_results,
                                          wifi_scan_result *results,
                                          u32 starting_index,
                                          struct nlattr **tb_vendor)
{
    u32 i = starting_index;
    struct nlattr *scanResultsInfo;
    int rem = 0;
    u32 len = 0;
    ALOGE("starting counter: %d", i);

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
            ALOGE("gscan_get_cached_results: RESULTS_SCAN_RESULT_TIME_STAMP"
                " not found");
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
            ALOGE("gscan_get_cached_results: RESULTS_SCAN_RESULT_SSID "
                "not found");
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
            ALOGE("gscan_get_cached_results: RESULTS_SCAN_RESULT_BSSID "
                "not found");
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
            ALOGE("gscan_get_cached_results: RESULTS_SCAN_RESULT_CHANNEL "
                "not found");
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
            ALOGE("gscan_get_cached_results: RESULTS_SCAN_RESULT_RSSI "
                "not found");
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
            ALOGE("gscan_get_cached_results: RESULTS_SCAN_RESULT_RTT "
                "not found");
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
            ALOGE("gscan_get_cached_results: RESULTS_SCAN_RESULT_RTT_SD "
                "not found");
            return WIFI_ERROR_INVALID_ARGS;
        }
        results[i].rtt_sd =
            nla_get_u32(
            tb2[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_RTT_SD]);

        ALOGE("gscan_get_cached_results: ts  %lld ", results[i].ts);
        ALOGE("gscan_get_cached_results: SSID  %s ", results[i].ssid);
        ALOGE("gscan_get_cached_results: "
            "BSSID: %02x:%02x:%02x:%02x:%02x:%02x \n",
            results[i].bssid[0], results[i].bssid[1], results[i].bssid[2],
            results[i].bssid[3], results[i].bssid[4], results[i].bssid[5]);
        ALOGE("gscan_get_cached_results: channel %d ", results[i].channel);
        ALOGE("gscan_get_cached_results: rssi  %d ", results[i].rssi);
        ALOGE("gscan_get_cached_results: rtt  %lld ", results[i].rtt);
        ALOGE("gscan_get_cached_results: rtt_sd  %lld ", results[i].rtt_sd);
        /* Increment loop index for next record */
        i++;
    }
    ALOGE("%s: Exited the for loop", __func__);
    return WIFI_SUCCESS;
}

/* This function will be the main handler for incoming (from driver)  GSscan_SUBCMD.
 *  Calls the appropriate callback handler after parsing the vendor data.
 */
int GScanCommand::handleEvent(WifiEvent &event)
{
    ALOGI("Got a GSCAN Event message from the Driver.");
    unsigned i = 0;
    u32 status;
    int ret = WIFI_SUCCESS;
    WifiVendorCommand::handleEvent(event);

    struct nlattr *tbVendor[
        QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_MAX + 1];
    nla_parse(tbVendor, QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_MAX,
            (struct nlattr *)mVendorData,
            mDataLen, NULL);

    switch(mSubcmd)
    {
        case QCA_NL80211_VENDOR_SUBCMD_GSCAN_START:
        {
            if (mStartGScanRspParams){
                mStartGScanRspParams->status =
                    nla_get_u32(tbVendor[QCA_WLAN_VENDOR_ATTR_GSCAN_STATUS]);
                if (mHandler.start)
                    (*mHandler.start)(mStartGScanRspParams->status);
            }
            waitForRsp(false);
        }
        break;

        case QCA_NL80211_VENDOR_SUBCMD_GSCAN_STOP:
        {
            if (mStopGScanRspParams){
                mStopGScanRspParams->status =
                    nla_get_u32(tbVendor[QCA_WLAN_VENDOR_ATTR_GSCAN_STATUS]);
                if (mHandler.stop)
                    (*mHandler.stop)(mStopGScanRspParams->status);
            }
            waitForRsp(false);
        }
        break;

        case QCA_NL80211_VENDOR_SUBCMD_GSCAN_SET_BSSID_HOTLIST:
        {
            if (mSetBssidHotlistRspParams){
                mSetBssidHotlistRspParams->status =
                    nla_get_u32(tbVendor[QCA_WLAN_VENDOR_ATTR_GSCAN_STATUS]);
                if (mHandler.set_bssid_hotlist)
                    (*mHandler.set_bssid_hotlist)
                            (mSetBssidHotlistRspParams->status);
            }
            waitForRsp(false);
        }
        break;

        case QCA_NL80211_VENDOR_SUBCMD_GSCAN_RESET_BSSID_HOTLIST:
        {
            if (mResetBssidHotlistRspParams){
                mResetBssidHotlistRspParams->status =
                    nla_get_u32(tbVendor[QCA_WLAN_VENDOR_ATTR_GSCAN_STATUS]);
                if (mHandler.reset_bssid_hotlist)
                    (*mHandler.reset_bssid_hotlist)
                            (mResetBssidHotlistRspParams->status);
            }
            waitForRsp(false);
        }
        break;

        case QCA_NL80211_VENDOR_SUBCMD_GSCAN_SET_SIGNIFICANT_CHANGE:
        {
            if (mSetSignificantChangeRspParams){
                mSetSignificantChangeRspParams->status =
                    nla_get_u32(tbVendor[QCA_WLAN_VENDOR_ATTR_GSCAN_STATUS]);
                if (mHandler.set_significant_change)
                    (*mHandler.set_significant_change)
                            (mSetSignificantChangeRspParams->status);
            }
            waitForRsp(false);
        }
        break;

        case QCA_NL80211_VENDOR_SUBCMD_GSCAN_RESET_SIGNIFICANT_CHANGE:
        {
            if (mResetSignificantChangeRspParams){
                mResetSignificantChangeRspParams->status =
                    nla_get_u32(tbVendor[QCA_WLAN_VENDOR_ATTR_GSCAN_STATUS]);
                if (mHandler.reset_significant_change)
                    (*mHandler.reset_significant_change)
                            (mResetSignificantChangeRspParams->status);
            }
            waitForRsp(false);
        }
        break;

        case QCA_NL80211_VENDOR_SUBCMD_GSCAN_GET_CAPABILITIES:
        {
            if (!mGetCapabilitiesRspParams){
                ALOGE("%s: mGetCapabilitiesRspParams ptr is NULL. Exit. ",
                    __func__);
                break;
            }

            if (!tbVendor[QCA_WLAN_VENDOR_ATTR_GSCAN_STATUS]) {
                ALOGE("%s: QCA_WLAN_VENDOR_ATTR_GSCAN_STATUS "
                    "not found", __func__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            mGetCapabilitiesRspParams->status =
                nla_get_u32(tbVendor[QCA_WLAN_VENDOR_ATTR_GSCAN_STATUS]);

            if (!tbVendor[
            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_SCAN_CACHE_SIZE
                    ]) {
                ALOGE("%s: QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_"
                    "CAPABILITIES_MAX_SCAN_CACHE_SIZE not found", __func__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            mGetCapabilitiesRspParams->capabilities.max_scan_cache_size =
                nla_get_u32(tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_SCAN_CACHE_SIZE]);

            if (!tbVendor[
            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_SCAN_BUCKETS
                    ]) {
                ALOGE("%s: QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX"
                    "_SCAN_BUCKETS not found", __func__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            mGetCapabilitiesRspParams->capabilities.max_scan_buckets =
                nla_get_u32(tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_SCAN_BUCKETS]
                                );

            if (!tbVendor[
        QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_AP_CACHE_PER_SCAN
                    ]) {
                ALOGE("%s: QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX"
                    "_AP_CACHE_PER_SCAN not found", __func__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            mGetCapabilitiesRspParams->capabilities.max_ap_cache_per_scan =
                    nla_get_u32(tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_AP_CACHE_PER_SCAN]);

            if (!tbVendor[
            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_RSSI_SAMPLE_SIZE
                    ]) {
                ALOGE("%s: QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX"
                    "_RSSI_SAMPLE_SIZE not found", __func__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            mGetCapabilitiesRspParams->capabilities.max_rssi_sample_size =
                    nla_get_u32(tbVendor[
                    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_RSSI_SAMPLE_SIZE]);

            if (!tbVendor[
            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_SCAN_REPORTING_THRESHOLD
                    ]) {
                ALOGE("%s: QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_"
                    "MAX_SCAN_REPORTING_THRESHOLD not found", __func__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            mGetCapabilitiesRspParams->capabilities.max_scan_reporting_threshold =
                    nla_get_u32(tbVendor[
            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_SCAN_REPORTING_THRESHOLD
            ]);

            if (!tbVendor[
            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_HOTLIST_APS
                    ]) {
                ALOGE("%s: QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_"
                    "MAX_HOTLIST_APS not found", __func__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            mGetCapabilitiesRspParams->capabilities.max_hotlist_aps =
                    nla_get_u32(tbVendor[
                    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_HOTLIST_APS]);

            if (!tbVendor[
            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_SIGNIFICANT_WIFI_CHANGE_APS
                    ]) {
                ALOGE("%s: QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX"
                    "_SIGNIFICANT_WIFI_CHANGE_APS not found", __func__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            mGetCapabilitiesRspParams->capabilities.max_significant_wifi_change_aps =
                    nla_get_u32(tbVendor[
            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_SIGNIFICANT_WIFI_CHANGE_APS]);

            if (!tbVendor[
            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_BSSID_HISTORY_ENTRIES
                    ]) {
                ALOGE("%s: QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX"
                    "_BSSID_HISTORY_ENTRIES not found", __func__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            mGetCapabilitiesRspParams->capabilities.max_bssid_history_entries =
                    nla_get_u32(tbVendor[
            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_BSSID_HISTORY_ENTRIES
            ]);
            /* Call the call back handler func. */
            if (mHandler.get_capabilities)
                (*mHandler.get_capabilities)
                        (mGetCapabilitiesRspParams->status,
                        mGetCapabilitiesRspParams->capabilities);
            waitForRsp(false);
        }
        break;

        case QCA_NL80211_VENDOR_SUBCMD_GSCAN_GET_CACHED_RESULTS:
        {
            wifi_request_id id;
            u32 resultsBufSize = 0;
            u32 numResults = 0;
            u32 startingIndex, sizeOfObtainedScanResults;

            if (!tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_REQUEST_ID]) {
                ALOGE("%s: GSCAN_RESULTS_REQUEST_ID not"
                    "found", __func__);
                break;
            }
            id = nla_get_u32(
                    tbVendor[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_REQUEST_ID]
                    );
            ALOGE("%s: Event has Req. ID:%d, ours:%d",
                __func__, id, mRequestId);
            /* If this is not for us, just ignore it. */
            if (id != mRequestId) {
                ALOGE("%s: Event has Req. ID:%d <> ours:%d",
                    __func__, id, mRequestId);
                break;
            }
            if (!tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_NUM_RESULTS_AVAILABLE]) {
                ALOGE("%s: GSCAN_RESULTS_NUM_RESULTS_AVAILABLE not"
                    "found", __func__);
                break;
            }
            numResults = nla_get_u32(tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_NUM_RESULTS_AVAILABLE]);
            ALOGE("%s: number of results:%d", __func__,
                numResults);

            if (!mGetCachedResultsRspParams) {
                ALOGE("%s: mGetCachedResultsRspParams is NULL, exit.",
                    __func__);
                break;
            }

            /* Get the memory size of previous fragments, if any. */
            sizeOfObtainedScanResults = mGetCachedResultsNumResults *
                              sizeof(wifi_scan_result);

            mGetCachedResultsNumResults += numResults;
            resultsBufSize += mGetCachedResultsNumResults *
                                                sizeof(wifi_scan_result);
            /* Check if this chunck of cached scan results is a continuation of
             * a previous one, i.e., a new results fragment.
             */
            if (mGetCachedResultsRspParams->more_data) {
                mGetCachedResultsRspParams->results = (wifi_scan_result *)
                    realloc (mGetCachedResultsRspParams->results,
                    resultsBufSize);
            } else {
                mGetCachedResultsRspParams->results = (wifi_scan_result *)
                    malloc (resultsBufSize);
            }

            ALOGE("%s: Total num of cached results received: %d. \n",
                __func__, mGetCachedResultsNumResults);

            if (!mGetCachedResultsRspParams->results) {
                ALOGE("%s: Failed to alloc memory for results array. Exit.\n",
                    __func__);
                ret = WIFI_ERROR_OUT_OF_MEMORY;
                break;
            }

            ALOGD("(u8 *)mGetCachedResultsRspParams->results : %p "
                     "resultsBufSize :%d oldSizeResults : %d . \n",
                     (u8 *)mGetCachedResultsRspParams->results,
                     resultsBufSize, sizeOfObtainedScanResults);
            /* Initialize the newly allocated memory area with 0. */
            memset((u8 *)mGetCachedResultsRspParams->results +
                sizeOfObtainedScanResults,
                0,
                resultsBufSize - sizeOfObtainedScanResults);

            /* To support fragmentation from firmware, monitor the
             * MORE_DTATA flag and cache results until MORE_DATA = 0.
             */
            if (!tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_MORE_DATA]) {
                ALOGE("%s: GSCAN_RESULTS_NUM_RESULTS_MORE_DATA "
                    "not found", __func__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            } else {
                mGetCachedResultsRspParams->more_data = nla_get_u8(
                    tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_MORE_DATA]);
                ALOGE("%s: More data = %d. \n", __func__,
                                mGetCachedResultsRspParams->more_data);
            }

            mGetCachedResultsRspParams->num_results =
                                        mGetCachedResultsNumResults;
            if (numResults) {
                ALOGD("%s: Extract cached results received.\n", __func__);
                startingIndex =
                    mGetCachedResultsNumResults - numResults;
                ALOGD("%s: starting_index:%d",
                    __func__, startingIndex);
                ret = gscan_get_cached_results(numResults,
                                        mGetCachedResultsRspParams->results,
                                        startingIndex,
                                        tbVendor);
                /* If a parsing error occurred, exit and proceed for cleanup. */
                if (ret)
                    break;
            }
            /* Send the results if no more result data fragments are expected. */
            if (mHandler.get_cached_results) {
                (*mHandler.get_cached_results)
                    (mGetCachedResultsRspParams->more_data,
                    mGetCachedResultsRspParams->num_results);
            }
            waitForRsp(false);
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
            case QCA_NL80211_VENDOR_SUBCMD_GSCAN_GET_CACHED_RESULTS:
            {
                freeRspParams(eGScanGetCachedResultsRspParams);
            }
            break;

            case QCA_NL80211_VENDOR_SUBCMD_GSCAN_GET_CAPABILITIES:
            break;

            default:
                ALOGE("%s: Wrong GScan subcmd received %d", __func__, mSubcmd);
        }
    }

    return NL_SKIP;
}

int GScanCommand::setCallbackHandler(GScanCallbackHandler nHandler)
{
    int res = 0;
    mHandler = nHandler;
    res = registerVendorHandler(mVendor_id, mSubcmd);
    if (res != 0) {
        /* Error case: should not happen, so print a log when it does. */
        ALOGE("%s: Unable to register Vendor Handler Vendor Id=0x%x subcmd=%u",
              __func__, mVendor_id, mSubcmd);
    }
    return res;
}

/*
 * Allocates memory for the subCmd response struct and initializes status = -1
 */
int GScanCommand::allocRspParams(eGScanRspRarams cmd)
{
    int ret = 0;
    switch(cmd)
    {
        case eGScanStartRspParams:
            mStartGScanRspParams = (GScanStartRspParams *)
                malloc(sizeof(GScanStartRspParams));
            if (!mStartGScanRspParams)
                ret = -1;
            else
                mStartGScanRspParams->status = -1;
        break;
        case eGScanStopRspParams:
            mStopGScanRspParams = (GScanStopRspParams *)
                malloc(sizeof(GScanStopRspParams));
            if (!mStopGScanRspParams)
                ret = -1;
            else
                mStopGScanRspParams->status = -1;
        break;
        case eGScanSetBssidHotlistRspParams:
            mSetBssidHotlistRspParams = (GScanSetBssidHotlistRspParams *)
                malloc(sizeof(GScanSetBssidHotlistRspParams));
            if (!mSetBssidHotlistRspParams)
                ret = -1;
            else
                mSetBssidHotlistRspParams->status = -1;
        break;
        case eGScanResetBssidHotlistRspParams:
            mResetBssidHotlistRspParams = (GScanResetBssidHotlistRspParams *)
                malloc(sizeof(GScanResetBssidHotlistRspParams));
            if (!mResetBssidHotlistRspParams)
                ret = -1;
            else
                mResetBssidHotlistRspParams->status = -1;
        break;
        case eGScanSetSignificantChangeRspParams:
            mSetSignificantChangeRspParams =
                (GScanSetSignificantChangeRspParams *)
                malloc(sizeof(GScanSetSignificantChangeRspParams));
            if (!mSetSignificantChangeRspParams)
                ret = -1;
            else
                mSetSignificantChangeRspParams->status = -1;
        break;
        case eGScanResetSignificantChangeRspParams:
            mResetSignificantChangeRspParams =
                (GScanResetSignificantChangeRspParams *)
                malloc(sizeof(GScanResetSignificantChangeRspParams));
            if (!mResetSignificantChangeRspParams)
                ret = -1;
            else
                mResetSignificantChangeRspParams->status = -1;
        break;
        case eGScanGetCapabilitiesRspParams:
            mGetCapabilitiesRspParams = (GScanGetCapabilitiesRspParams *)
                malloc(sizeof(GScanGetCapabilitiesRspParams));
            if (!mGetCapabilitiesRspParams)
                ret = -1;
            else  {
                memset(&mGetCapabilitiesRspParams->capabilities, 0,
                    sizeof(wifi_gscan_capabilities));
                mGetCapabilitiesRspParams->status = -1;
            }
        break;
        case eGScanGetCachedResultsRspParams:
            mGetCachedResultsRspParams = (GScanGetCachedResultsRspParams *)
                malloc(sizeof(GScanGetCachedResultsRspParams));
            if (!mGetCachedResultsRspParams)
                ret = -1;
            else {
                mGetCachedResultsRspParams->num_results = 0;
                mGetCachedResultsRspParams->more_data = false;
                mGetCachedResultsRspParams->results = NULL;
            }
        break;
        default:
            ALOGD("%s: Wrong request for alloc.", __func__);
            ret = -1;
    }
    return ret;
}

void GScanCommand::freeRspParams(eGScanRspRarams cmd)
{
    switch(cmd)
    {
        case eGScanStartRspParams:
            if (mStartGScanRspParams) {
                free(mStartGScanRspParams);
                mStartGScanRspParams = NULL;
            }
        break;
        case eGScanStopRspParams:
            if (mStopGScanRspParams) {
                free(mStopGScanRspParams);
                mStopGScanRspParams = NULL;
            }
        break;
        case eGScanSetBssidHotlistRspParams:
            if (mSetBssidHotlistRspParams) {
                free(mSetBssidHotlistRspParams);
                mSetBssidHotlistRspParams = NULL;
            }
        break;
        case eGScanResetBssidHotlistRspParams:
            if (mResetBssidHotlistRspParams) {
                free(mResetBssidHotlistRspParams);
                mResetBssidHotlistRspParams = NULL;
            }
        break;
        case eGScanSetSignificantChangeRspParams:
            if (mSetSignificantChangeRspParams) {
                free(mSetSignificantChangeRspParams);
                mSetSignificantChangeRspParams = NULL;
            }
        break;
        case eGScanResetSignificantChangeRspParams:
            if (mResetSignificantChangeRspParams) {
                free(mResetSignificantChangeRspParams);
                mResetSignificantChangeRspParams = NULL;
            }
        break;
        case eGScanGetCapabilitiesRspParams:
            if (mGetCapabilitiesRspParams) {
                free(mGetCapabilitiesRspParams);
                mGetCapabilitiesRspParams = NULL;
            }
        break;
        case eGScanGetCachedResultsRspParams:
            if (mGetCachedResultsRspParams) {
                if (mGetCachedResultsRspParams->results) {
                    free(mGetCachedResultsRspParams->results);
                    mGetCachedResultsRspParams->results = NULL;
                }
                free(mGetCachedResultsRspParams);
                mGetCachedResultsRspParams = NULL;
            }
        break;

        default:
            ALOGD("%s: Wrong request for free.", __func__);
    }
}

wifi_error GScanCommand::getGetCachedResultsRspParams(
                                                    int max,
                                                    u8 *moreData,
                                                    int *numResults,
                                                    wifi_scan_result *results)
{
    wifi_error ret = WIFI_SUCCESS;

    if (mGetCachedResultsRspParams && results)
    {
        *moreData = mGetCachedResultsRspParams->more_data;
        *numResults = mGetCachedResultsRspParams->num_results > (unsigned int)max ?
                        max : mGetCachedResultsRspParams->num_results;
        memcpy(results,
            mGetCachedResultsRspParams->results,
            *numResults * sizeof(wifi_scan_result));
    } else {
        ALOGD("%s: mGetCachedResultsRspParams is NULL", __func__);
        ret = WIFI_ERROR_INVALID_ARGS;
    }
    return ret;
}

void GScanCommand::getGetCapabilitiesRspParams(
                                        wifi_gscan_capabilities *capabilities,
                                        u32 *status)
{
    if (mGetCapabilitiesRspParams && capabilities)
    {
        *status = mGetCapabilitiesRspParams->status;
        memcpy(capabilities,
            &mGetCapabilitiesRspParams->capabilities,
            sizeof(wifi_gscan_capabilities));
    } else {
        ALOGD("%s: mGetCapabilitiesRspParams is NULL", __func__);
    }
}

void GScanCommand::getStartGScanRspParams(u32 *status)
{
    if (mStartGScanRspParams)
    {
        *status = mStartGScanRspParams->status;
    } else {
        ALOGD("%s: mStartGScanRspParams is NULL", __func__);
    }
}

void GScanCommand::getStopGScanRspParams(u32 *status)
{
    if (mStopGScanRspParams)
    {
        *status = mStopGScanRspParams->status;
    } else {
        ALOGD("%s: mStopGScanRspParams is NULL", __func__);
    }
}

void GScanCommand::getSetBssidHotlistRspParams(u32 *status)
{
    if (mSetBssidHotlistRspParams)
    {
        *status = mSetBssidHotlistRspParams->status;
    } else {
        ALOGD("%s: mSetBssidHotlistRspParams is NULL", __func__);
    }
}

void GScanCommand::getResetBssidHotlistRspParams(u32 *status)
{
    if (mResetBssidHotlistRspParams)
    {
        *status = mResetBssidHotlistRspParams->status;
    } else {
        ALOGD("%s: mResetBssidHotlistRspParams is NULL", __func__);
    }
}

void GScanCommand::getSetSignificantChangeRspParams(u32 *status)
{
    if (mSetSignificantChangeRspParams)
    {
        *status = mSetSignificantChangeRspParams->status;
    } else {
        ALOGD("%s: mSetSignificantChangeRspParams is NULL", __func__);
    }
}

void GScanCommand::getResetSignificantChangeRspParams(u32 *status)
{
    if (mResetSignificantChangeRspParams)
    {
        *status = mResetSignificantChangeRspParams->status;
    } else {
        ALOGD("%s: mResetSignificantChangeRspParams is NULL", __func__);
    }
}

int GScanCommand::timed_wait(u16 wait_time)
{
    struct timespec absTime;
    int res;
    absTime.tv_sec = wait_time;
    absTime.tv_nsec = 0;
    return mCondition.wait(absTime);
}

void GScanCommand::waitForRsp(bool wait)
{
    mWaitforRsp = wait;
}

void GScanCommand::setMaxChannels(int max_channels) {
    mMaxChannels = max_channels;
}

void GScanCommand::setChannels(int *channels) {
    mChannels = channels;
}

void GScanCommand::setNumChannelsPtr(int *num_channels) {
    mNumChannelsPtr = num_channels;
}

