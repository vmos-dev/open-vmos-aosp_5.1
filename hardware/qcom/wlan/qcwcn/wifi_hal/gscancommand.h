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

#ifndef __WIFI_HAL_GSCAN_COMMAND_H__
#define __WIFI_HAL_GSCAN_COMMAND_H__

#include "common.h"
#include "cpp_bindings.h"
#ifdef __GNUC__
#define PRINTF_FORMAT(a,b) __attribute__ ((format (printf, (a), (b))))
#define STRUCT_PACKED __attribute__ ((packed))
#else
#define PRINTF_FORMAT(a,b)
#define STRUCT_PACKED
#endif
#include "qca-vendor.h"
#include "vendor_definitions.h"
#include "gscan.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

typedef struct{
    u32 status;
    u32 num_channels;
    wifi_channel channels[];
} GScanGetValidChannelsRspParams;

typedef struct{
    u32 status;
    wifi_gscan_capabilities capabilities;
} GScanGetCapabilitiesRspParams;

typedef struct{
    u32 status;
} GScanStartRspParams;

typedef struct{
    u32 status;
} GScanStopRspParams;

typedef struct{
    u32 status;
} GScanSetBssidHotlistRspParams;

typedef struct{
    u32 status;
} GScanResetBssidHotlistRspParams;

typedef struct{
    u8  more_data;
    u32 num_results;
    wifi_scan_result *results;
} GScanGetCachedResultsRspParams;

typedef struct{
    u32 status;
} GScanSetSignificantChangeRspParams;

typedef struct{
    u32 status;
} GScanResetSignificantChangeRspParams;

typedef struct {
    int max_channels;
    wifi_channel *channels;
    int *number_channels;
} GScan_get_valid_channels_cb_data;

typedef enum{
    eGScanRspParamsInvalid = 0,
    eGScanGetValidChannelsRspParams,
    eGScanGetCapabilitiesRspParams,
    eGScanGetCachedResultsRspParams,
    eGScanStartRspParams,
    eGScanStopRspParams,
    eGScanSetBssidHotlistRspParams,
    eGScanResetBssidHotlistRspParams,
    eGScanSetSignificantChangeRspParams,
    eGScanResetSignificantChangeRspParams,
} eGScanRspRarams;

/* Response and Event Callbacks */
typedef struct {
    /* Various Events Callback */
    void (*get_capabilities)(int status, wifi_gscan_capabilities capabilities);
    void (*start)(int status);
    void (*stop)(int status);
    void (*set_bssid_hotlist)(int status);
    void (*reset_bssid_hotlist)(int status);
    void (*set_significant_change)(int status);
    void (*reset_significant_change)(int status);
    void (*on_hotlist_ap_found)(wifi_request_id id,
        unsigned num_results, wifi_scan_result *results);
    void (*get_cached_results)(u8 more_data, u32 num_results);
    void (*on_significant_change)(wifi_request_id id,
                unsigned num_results,
                wifi_significant_change_result **results);
    /* Reported when report_threshold is reached in scan cache */
    void (*on_scan_results_available) (wifi_request_id id,
                                    unsigned num_results_available);
    /* Reported when each probe response is received, if report_events
     * enabled in wifi_scan_cmd_params
     */
    void (*on_full_scan_result) (wifi_request_id id, wifi_scan_result *result);
    /* Optional event - indicates progress of scanning statemachine */
    void (*on_scan_event) (wifi_scan_event event, unsigned status);
} GScanCallbackHandler;

class GScanCommand: public WifiVendorCommand
{
private:
    GScanStopRspParams                  *mStopGScanRspParams;
    GScanStartRspParams                 *mStartGScanRspParams;
    GScanSetBssidHotlistRspParams       *mSetBssidHotlistRspParams;
    GScanResetBssidHotlistRspParams     *mResetBssidHotlistRspParams;
    GScanSetSignificantChangeRspParams  *mSetSignificantChangeRspParams;
    GScanResetSignificantChangeRspParams*mResetSignificantChangeRspParams;
    GScanGetCapabilitiesRspParams       *mGetCapabilitiesRspParams;
    GScanGetCachedResultsRspParams      *mGetCachedResultsRspParams;
    u32                                 mGetCachedResultsNumResults;
    GScanCallbackHandler                mHandler;
    int                                 mRequestId;
    int                                 *mChannels;
    int                                 mMaxChannels;
    int                                 *mNumChannelsPtr;
    bool                                mWaitforRsp;

public:
    GScanCommand(wifi_handle handle, int id, u32 vendor_id, u32 subcmd);
    virtual ~GScanCommand();

    /* This function implements creation of GSCAN specific Request
     * based on  the request type.
     */
    virtual int create();
    virtual int requestEvent();
    virtual int requestResponse();
    virtual int handleResponse(WifiEvent &reply);
    virtual int handleEvent(WifiEvent &event);
    virtual int setCallbackHandler(GScanCallbackHandler nHandler);
    virtual void setMaxChannels(int max_channels);
    virtual void setChannels(int *channels);
    virtual void setNumChannelsPtr(int *num_channels);
    virtual int allocRspParams(eGScanRspRarams cmd);
    virtual void freeRspParams(eGScanRspRarams cmd);
    virtual void getGetCapabilitiesRspParams(
                    wifi_gscan_capabilities *capabilities,
                    u32 *status);
    virtual void getStartGScanRspParams(u32 *status);
    virtual void getStopGScanRspParams(u32 *status);
    virtual void getSetBssidHotlistRspParams(u32 *status);
    virtual void getResetBssidHotlistRspParams(u32 *status);
    virtual void getSetSignificantChangeRspParams(u32 *status);
    virtual void getResetSignificantChangeRspParams(u32 *status);
    virtual wifi_error getGetCachedResultsRspParams(int max,
                                                    u8 *moreData,
                                                    int *numResults,
                                                    wifi_scan_result *results);
    /* Takes wait time in seconds. */
    virtual int timed_wait(u16 wait_time);
    virtual void waitForRsp(bool wait);
};

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
