
#include <stdint.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <linux/rtnetlink.h>
#include <netpacket/packet.h>
#include <linux/filter.h>
#include <linux/errqueue.h>

#include <linux/pkt_sched.h>
#include <netlink/object-api.h>
#include <netlink/netlink.h>
#include <netlink/socket.h>
#include <netlink/handlers.h>

#include "sync.h"

#define LOG_TAG  "WifiHAL"

#include <utils/Log.h>

#include "wifi_hal.h"
#include "common.h"
#include "cpp_bindings.h"

typedef enum {

    GSCAN_ATTRIBUTE_NUM_BUCKETS = 10,
    GSCAN_ATTRIBUTE_BASE_PERIOD,
    GSCAN_ATTRIBUTE_BUCKETS_BAND,
    GSCAN_ATTRIBUTE_BUCKET_ID,
    GSCAN_ATTRIBUTE_BUCKET_PERIOD,
    GSCAN_ATTRIBUTE_BUCKET_NUM_CHANNELS,
    GSCAN_ATTRIBUTE_BUCKET_CHANNELS,
    GSCAN_ATTRIBUTE_NUM_AP_PER_SCAN,
    GSCAN_ATTRIBUTE_REPORT_THRESHOLD,
    GSCAN_ATTRIBUTE_NUM_SCANS_TO_CACHE,
    GSCAN_ATTRIBUTE_BAND = GSCAN_ATTRIBUTE_BUCKETS_BAND,

    GSCAN_ATTRIBUTE_ENABLE_FEATURE = 20,
    GSCAN_ATTRIBUTE_SCAN_RESULTS_COMPLETE,              /* indicates no more results */
    GSCAN_ATTRIBUTE_FLUSH_FEATURE,                      /* Flush all the configs */
    GSCAN_ENABLE_FULL_SCAN_RESULTS,
    GSCAN_ATTRIBUTE_REPORT_EVENTS,

    /* remaining reserved for additional attributes */
    GSCAN_ATTRIBUTE_NUM_OF_RESULTS = 30,
    GSCAN_ATTRIBUTE_FLUSH_RESULTS,
    GSCAN_ATTRIBUTE_SCAN_RESULTS,                       /* flat array of wifi_scan_result */
    GSCAN_ATTRIBUTE_SCAN_ID,                            /* indicates scan number */
    GSCAN_ATTRIBUTE_SCAN_FLAGS,                         /* indicates if scan was aborted */
    GSCAN_ATTRIBUTE_AP_FLAGS,                           /* flags on significant change event */
    GSCAN_ATTRIBUTE_NUM_CHANNELS,
    GSCAN_ATTRIBUTE_CHANNEL_LIST,

    /* remaining reserved for additional attributes */

    GSCAN_ATTRIBUTE_SSID = 40,
    GSCAN_ATTRIBUTE_BSSID,
    GSCAN_ATTRIBUTE_CHANNEL,
    GSCAN_ATTRIBUTE_RSSI,
    GSCAN_ATTRIBUTE_TIMESTAMP,
    GSCAN_ATTRIBUTE_RTT,
    GSCAN_ATTRIBUTE_RTTSD,

    /* remaining reserved for additional attributes */

    GSCAN_ATTRIBUTE_HOTLIST_BSSIDS = 50,
    GSCAN_ATTRIBUTE_RSSI_LOW,
    GSCAN_ATTRIBUTE_RSSI_HIGH,
    GSCAN_ATTRIBUTE_HOTLIST_ELEM,
    GSCAN_ATTRIBUTE_HOTLIST_FLUSH,

    /* remaining reserved for additional attributes */
    GSCAN_ATTRIBUTE_RSSI_SAMPLE_SIZE = 60,
    GSCAN_ATTRIBUTE_LOST_AP_SAMPLE_SIZE,
    GSCAN_ATTRIBUTE_MIN_BREACHING,
    GSCAN_ATTRIBUTE_SIGNIFICANT_CHANGE_BSSIDS,
    GSCAN_ATTRIBUTE_SIGNIFICANT_CHANGE_FLUSH,

    GSCAN_ATTRIBUTE_MAX

} GSCAN_ATTRIBUTE;


// helper methods
wifi_error wifi_enable_full_scan_results(wifi_request_id id, wifi_interface_handle iface,
         wifi_scan_result_handler handler);
wifi_error wifi_disable_full_scan_results(wifi_request_id id, wifi_interface_handle iface);


/////////////////////////////////////////////////////////////////////////////

class GetCapabilitiesCommand : public WifiCommand
{
    wifi_gscan_capabilities *mCapabilities;
public:
    GetCapabilitiesCommand(wifi_interface_handle iface, wifi_gscan_capabilities *capabitlites)
        : WifiCommand(iface, 0), mCapabilities(capabitlites)
    {
        memset(mCapabilities, 0, sizeof(*mCapabilities));
    }

    virtual int create() {
        ALOGD("Creating message to get scan capablities; iface = %d", mIfaceInfo->id);

        int ret = mMsg.create(GOOGLE_OUI, GSCAN_SUBCMD_GET_CAPABILITIES);
        if (ret < 0) {
            return ret;
        }

        return ret;
    }

protected:
    virtual int handleResponse(WifiEvent& reply) {

        ALOGD("In GetCapabilities::handleResponse");

        if (reply.get_cmd() != NL80211_CMD_VENDOR) {
            ALOGD("Ignoring reply with cmd = %d", reply.get_cmd());
            return NL_SKIP;
        }

        int id = reply.get_vendor_id();
        int subcmd = reply.get_vendor_subcmd();

        void *data = reply.get_vendor_data();
        int len = reply.get_vendor_data_len();

        ALOGD("Id = %0x, subcmd = %d, len = %d, expected len = %d", id, subcmd, len,
                    sizeof(*mCapabilities));

        memcpy(mCapabilities, data, min(len, (int) sizeof(*mCapabilities)));

        return NL_OK;
    }
};


wifi_error wifi_get_gscan_capabilities(wifi_interface_handle handle,
        wifi_gscan_capabilities *capabilities)
{
    GetCapabilitiesCommand command(handle, capabilities);
    return (wifi_error) command.requestResponse();
}

class GetChannelListCommand : public WifiCommand
{
    wifi_channel *channels;
    int max_channels;
    int *num_channels;
    int band;
public:
    GetChannelListCommand(wifi_interface_handle iface, wifi_channel *channel_buf, int *ch_num,
        int num_max_ch, int band)
        : WifiCommand(iface, 0), channels(channel_buf), max_channels(num_max_ch), num_channels(ch_num),
        band(band)
    {
        memset(channels, 0, sizeof(wifi_channel) * max_channels);
    }
    virtual int create() {
        ALOGD("Creating message to get channel list; iface = %d", mIfaceInfo->id);

        int ret = mMsg.create(GOOGLE_OUI, GSCAN_SUBCMD_GET_CHANNEL_LIST);
        if (ret < 0) {
            return ret;
        }

        nlattr *data = mMsg.attr_start(NL80211_ATTR_VENDOR_DATA);
        ret = mMsg.put_u32(GSCAN_ATTRIBUTE_BAND, band);
        if (ret < 0) {
            return ret;
        }

        mMsg.attr_end(data);

        return ret;
    }

protected:
    virtual int handleResponse(WifiEvent& reply) {

        ALOGD("In GetChannelList::handleResponse");

        if (reply.get_cmd() != NL80211_CMD_VENDOR) {
            ALOGD("Ignoring reply with cmd = %d", reply.get_cmd());
            return NL_SKIP;
        }

        int id = reply.get_vendor_id();
        int subcmd = reply.get_vendor_subcmd();
        int num_channels_to_copy = 0;

        nlattr *vendor_data = reply.get_attribute(NL80211_ATTR_VENDOR_DATA);
        int len = reply.get_vendor_data_len();

        ALOGD("Id = %0x, subcmd = %d, len = %d", id, subcmd, len);
        if (vendor_data == NULL || len == 0) {
            ALOGE("no vendor data in GetChannelList response; ignoring it");
            return NL_SKIP;
        }

        for (nl_iterator it(vendor_data); it.has_next(); it.next()) {
            if (it.get_type() == GSCAN_ATTRIBUTE_NUM_CHANNELS) {
                num_channels_to_copy = it.get_u32();
                ALOGI("Got channel list with %d channels", num_channels_to_copy);
                if(num_channels_to_copy > max_channels)
                    num_channels_to_copy = max_channels;
                *num_channels = num_channels_to_copy;
            } else if (it.get_type() == GSCAN_ATTRIBUTE_CHANNEL_LIST && num_channels_to_copy) {
                memcpy(channels, it.get_data(), sizeof(int) * num_channels_to_copy);
            } else {
                ALOGW("Ignoring invalid attribute type = %d, size = %d",
                        it.get_type(), it.get_len());
            }
        }

        return NL_OK;
    }
};

wifi_error wifi_get_valid_channels(wifi_interface_handle handle,
        int band, int max_channels, wifi_channel *channels, int *num_channels)
{
    GetChannelListCommand command(handle, channels, num_channels,
                                        max_channels, band);
    return (wifi_error) command.requestResponse();
}
/////////////////////////////////////////////////////////////////////////////

/* helper functions */

static int parseScanResults(wifi_scan_result *results, int num, nlattr *attr)
{
    memset(results, 0, sizeof(wifi_scan_result) * num);

    int i = 0;
    for (nl_iterator it(attr); it.has_next() && i < num; it.next(), i++) {

        int index = it.get_type();
        ALOGI("retrieved scan result %d", index);
        nlattr *sc_data = (nlattr *) it.get_data();
        wifi_scan_result *result = results + i;

        for (nl_iterator it2(sc_data); it2.has_next(); it2.next()) {
            int type = it2.get_type();
            if (type == GSCAN_ATTRIBUTE_SSID) {
                strncpy(result->ssid, (char *) it2.get_data(), it2.get_len());
                result->ssid[it2.get_len()] = 0;
            } else if (type == GSCAN_ATTRIBUTE_BSSID) {
                memcpy(result->bssid, (byte *) it2.get_data(), sizeof(mac_addr));
            } else if (type == GSCAN_ATTRIBUTE_TIMESTAMP) {
                result->ts = it2.get_u64();
            } else if (type == GSCAN_ATTRIBUTE_CHANNEL) {
                result->ts = it2.get_u16();
            } else if (type == GSCAN_ATTRIBUTE_RSSI) {
                result->rssi = it2.get_u8();
            } else if (type == GSCAN_ATTRIBUTE_RTT) {
                result->rtt = it2.get_u64();
            } else if (type == GSCAN_ATTRIBUTE_RTTSD) {
                result->rtt_sd = it2.get_u64();
            }
        }

    }

    if (i >= num) {
        ALOGE("Got too many results; skipping some");
    }

    return i;
}

int createFeatureRequest(WifiRequest& request, int subcmd, int enable) {

    int result = request.create(GOOGLE_OUI, subcmd);
    if (result < 0) {
        return result;
    }

    nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
    result = request.put_u32(GSCAN_ATTRIBUTE_ENABLE_FEATURE, enable);
    if (result < 0) {
        return result;
    }

    request.attr_end(data);
    return WIFI_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////
class FullScanResultsCommand : public WifiCommand
{
    int *mParams;
    wifi_scan_result_handler mHandler;
public:
    FullScanResultsCommand(wifi_interface_handle iface, int id, int *params,
                wifi_scan_result_handler handler)
        : WifiCommand(iface, id), mParams(params), mHandler(handler)
    { }

    int createRequest(WifiRequest& request, int subcmd, int enable) {
        int result = request.create(GOOGLE_OUI, subcmd);
        if (result < 0) {
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put_u32(GSCAN_ENABLE_FULL_SCAN_RESULTS, enable);
        if (result < 0) {
            return result;
        }

        request.attr_end(data);
        return WIFI_SUCCESS;

    }

    int start() {
        ALOGD("Enabling Full scan results");
        WifiRequest request(familyId(), ifaceId());
        int result = createRequest(request, GSCAN_SUBCMD_ENABLE_FULL_SCAN_RESULTS, 1);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to create request; result = %d", result);
            return result;
        }

        registerVendorHandler(GOOGLE_OUI, GSCAN_EVENT_FULL_SCAN_RESULTS);

        result = requestResponse(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to enable full scan results; result = %d", result);
            unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_FULL_SCAN_RESULTS);
            return result;
        }

        return result;
    }

    virtual int cancel() {
        ALOGD("Disabling Full scan results");

        WifiRequest request(familyId(), ifaceId());
        int result = createRequest(request, GSCAN_SUBCMD_ENABLE_FULL_SCAN_RESULTS, 0);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to create request; result = %d", result);
        } else {
            result = requestResponse(request);
            if (result != WIFI_SUCCESS) {
                ALOGE("failed to disable full scan results;result = %d", result);
            }
        }

        unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_FULL_SCAN_RESULTS);
        return WIFI_SUCCESS;
    }

    virtual int handleResponse(WifiEvent& reply) {
         ALOGD("Request complete!");
        /* Nothing to do on response! */
        return NL_SKIP;
    }

    virtual int handleEvent(WifiEvent& event) {
        ALOGI("Full scan results:  Got an event");

        // event.log();

        nlattr *vendor_data = event.get_attribute(NL80211_ATTR_VENDOR_DATA);
        unsigned int len = event.get_vendor_data_len();

        if (vendor_data == NULL || len < sizeof(wifi_scan_result)) {
            ALOGI("No scan results found");
            return NL_SKIP;
        }

        wifi_scan_result *result = (wifi_scan_result *)event.get_vendor_data();

        if(*mHandler.on_full_scan_result)
            (*mHandler.on_full_scan_result)(id(), result);

        ALOGI("%-32s\t", result->ssid);

        ALOGI("%02x:%02x:%02x:%02x:%02x:%02x ", result->bssid[0], result->bssid[1],
                result->bssid[2], result->bssid[3], result->bssid[4], result->bssid[5]);

        ALOGI("%d\t", result->rssi);
        ALOGI("%d\t", result->channel);
        ALOGI("%lld\t", result->ts);
        ALOGI("%lld\t", result->rtt);
        ALOGI("%lld\n", result->rtt_sd);


        return NL_SKIP;
    }

};
/////////////////////////////////////////////////////////////////////////////

class ScanCommand : public WifiCommand
{
    wifi_scan_cmd_params *mParams;
    wifi_scan_result_handler mHandler;
    static unsigned mGlobalFullScanBuckets;
    bool mLocalFullScanBuckets;
public:
    ScanCommand(wifi_interface_handle iface, int id, wifi_scan_cmd_params *params,
                wifi_scan_result_handler handler)
        : WifiCommand(iface, id), mParams(params), mHandler(handler),
          mLocalFullScanBuckets(0)
    { }

    int createSetupRequest(WifiRequest& request) {
        int result = request.create(GOOGLE_OUI, GSCAN_SUBCMD_SET_CONFIG);
        if (result < 0) {
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put_u32(GSCAN_ATTRIBUTE_BASE_PERIOD, mParams->base_period);
        if (result < 0) {
            return result;
        }

        result = request.put_u32(GSCAN_ATTRIBUTE_NUM_BUCKETS, mParams->num_buckets);
        if (result < 0) {
            return result;
        }

        for (int i = 0; i < mParams->num_buckets; i++) {
            nlattr * bucket = request.attr_start(i);    // next bucket
            result = request.put_u32(GSCAN_ATTRIBUTE_BUCKET_ID, mParams->buckets[i].bucket);
            if (result < 0) {
                return result;
            }
            result = request.put_u32(GSCAN_ATTRIBUTE_BUCKET_PERIOD, mParams->buckets[i].period);
            if (result < 0) {
                return result;
            }
            result = request.put_u32(GSCAN_ATTRIBUTE_BUCKETS_BAND,
                    mParams->buckets[i].band);
            if (result < 0) {
                return result;
            }

            result = request.put_u32(GSCAN_ATTRIBUTE_REPORT_EVENTS,
                    mParams->buckets[i].report_events);
            if (result < 0) {
                return result;
            }

            result = request.put_u32(GSCAN_ATTRIBUTE_BUCKET_NUM_CHANNELS,
                    mParams->buckets[i].num_channels);
            if (result < 0) {
                return result;
            }

            if (mParams->buckets[i].num_channels) {
                nlattr *channels = request.attr_start(GSCAN_ATTRIBUTE_BUCKET_CHANNELS);
                for (int j = 0; j < mParams->buckets[i].num_channels; j++) {
                    result = request.put_u32(j, mParams->buckets[i].channels[j].channel);
                    if (result < 0) {
                        return result;
                    }
                }
                request.attr_end(channels);
            }

            request.attr_end(bucket);
        }

        request.attr_end(data);
        return WIFI_SUCCESS;
    }

    int createScanConfigRequest(WifiRequest& request) {
        int result = request.create(GOOGLE_OUI, GSCAN_SUBCMD_SET_SCAN_CONFIG);
        if (result < 0) {
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put_u32(GSCAN_ATTRIBUTE_NUM_AP_PER_SCAN, mParams->max_ap_per_scan);
        if (result < 0) {
            return result;
        }

        result = request.put_u32(GSCAN_ATTRIBUTE_REPORT_THRESHOLD, mParams->report_threshold);
        if (result < 0) {
            return result;
        }

        int num_scans = 20;
        for (int i = 0; i < mParams->num_buckets; i++) {
            if (mParams->buckets[i].report_events == 1) {
                ALOGD("Setting num_scans to 1");
                num_scans = 1;
                break;
            }
        }

        result = request.put_u32(GSCAN_ATTRIBUTE_NUM_SCANS_TO_CACHE, num_scans);
        if (result < 0) {
            return result;
        }

        request.attr_end(data);
        return WIFI_SUCCESS;
    }

    int createStartRequest(WifiRequest& request) {
        return createFeatureRequest(request, GSCAN_SUBCMD_ENABLE_GSCAN, 1);
    }

    int createStopRequest(WifiRequest& request) {
        return createFeatureRequest(request, GSCAN_SUBCMD_ENABLE_GSCAN, 0);
    }

    int enableFullScanResultsIfRequired() {
        /* temporary workaround till we have full support for per bucket scans */

        ALOGI("enabling full scan results if needed");
        int nBuckets = 0;
        for (int i = 0; i < mParams->num_buckets; i++) {
            if (mParams->buckets[i].report_events == 2) {
                nBuckets++;
            }
        }

        if (mGlobalFullScanBuckets == 0 && nBuckets != 0) {
            int result = wifi_enable_full_scan_results(0x1000, ifaceHandle(), mHandler);
            if (result != WIFI_SUCCESS) {
                ALOGI("failed to enable full scan results");
                return result;
            } else {
                ALOGI("successfully enabled full scan results");
            }
        } else {
            ALOGI("mGlobalFullScanBuckets = %d, nBuckets = %d", mGlobalFullScanBuckets, nBuckets);
        }

        mLocalFullScanBuckets = nBuckets;
        mGlobalFullScanBuckets += nBuckets;
        return WIFI_SUCCESS;
    }

    int disableFullScanResultsIfRequired() {
        /* temporary workaround till we have full support for per bucket scans */

        if (mLocalFullScanBuckets == 0) {
            return WIFI_SUCCESS;
        }

        mGlobalFullScanBuckets -= mLocalFullScanBuckets;
        if (mGlobalFullScanBuckets == 0) {
            int result = wifi_disable_full_scan_results(0x1000, ifaceHandle());
            if (result != WIFI_SUCCESS) {
                ALOGI("failed to disable full scan results");
            } else {
                ALOGI("successfully disable full scan results");
            }
        }

        return WIFI_SUCCESS;
    }

    int start() {
        ALOGD("Setting configuration");
        WifiRequest request(familyId(), ifaceId());
        int result = createSetupRequest(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to create setup request; result = %d", result);
            return result;
        }

        result = requestResponse(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to configure setup; result = %d", result);
            return result;
        }

        request.destroy();

        result = createScanConfigRequest(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to create scan config request; result = %d", result);
            return result;
        }

        result = requestResponse(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to configure scan; result = %d", result);
            return result;
        }

        ALOGD("Starting scan");

        result = createStartRequest(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to create start request; result = %d", result);
            return result;
        }

        registerVendorHandler(GOOGLE_OUI, GSCAN_EVENT_SCAN_RESULTS_AVAILABLE);
        registerVendorHandler(GOOGLE_OUI, GSCAN_EVENT_COMPLETE_SCAN);

        result = requestResponse(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to start scan; result = %d", result);
            registerVendorHandler(GOOGLE_OUI, GSCAN_EVENT_COMPLETE_SCAN);
            unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_SCAN_RESULTS_AVAILABLE);
            return result;
        }

        result = enableFullScanResultsIfRequired();
        return result;
    }

    virtual int cancel() {
        ALOGD("Stopping scan");

        WifiRequest request(familyId(), ifaceId());
        int result = createStopRequest(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to create stop request; result = %d", result);
        } else {
            result = requestResponse(request);
            if (result != WIFI_SUCCESS) {
                ALOGE("failed to stop scan; result = %d", result);
            }
        }

        unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_COMPLETE_SCAN);
        unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_SCAN_RESULTS_AVAILABLE);
        disableFullScanResultsIfRequired();

        return WIFI_SUCCESS;
    }

    virtual int handleResponse(WifiEvent& reply) {
        /* Nothing to do on response! */
        return NL_SKIP;
    }

    virtual int handleEvent(WifiEvent& event) {
        ALOGI("Got a scan results event");

        // event.log();

        nlattr *vendor_data = event.get_attribute(NL80211_ATTR_VENDOR_DATA);
        int len = event.get_vendor_data_len();
        int event_id = event.get_vendor_subcmd();

        if(event_id == GSCAN_EVENT_COMPLETE_SCAN) {
            if (vendor_data == NULL || len != 4) {
                ALOGI("Scan complete type not mentioned!");
                return NL_SKIP;
            }
            wifi_scan_event evt_type;

            evt_type = (wifi_scan_event) event.get_u32(NL80211_ATTR_VENDOR_DATA);
            ALOGI("Scan complete: Received event type %d", evt_type);
            if(*mHandler.on_scan_event)
                (*mHandler.on_scan_event)(evt_type, evt_type);
        } else {

            if (vendor_data == NULL || len != 4) {
                ALOGI("No scan results found");
                return NL_SKIP;
            }

            int num = event.get_u32(NL80211_ATTR_VENDOR_DATA);
            ALOGI("Found %d scan results", num);
            if(*mHandler.on_scan_results_available)
                (*mHandler.on_scan_results_available)(id(), num);
        }
        return NL_SKIP;
    }
};

unsigned ScanCommand::mGlobalFullScanBuckets = 0;

wifi_error wifi_start_gscan(
        wifi_request_id id,
        wifi_interface_handle iface,
        wifi_scan_cmd_params params,
        wifi_scan_result_handler handler)
{
    wifi_handle handle = getWifiHandle(iface);

    ALOGD("Starting GScan, halHandle = %p", handle);

    ScanCommand *cmd = new ScanCommand(iface, id, &params, handler);
    wifi_register_cmd(handle, id, cmd);
    return (wifi_error)cmd->start();
}

wifi_error wifi_stop_gscan(wifi_request_id id, wifi_interface_handle iface)
{
    ALOGD("Stopping GScan");
    wifi_handle handle = getWifiHandle(iface);

    if(id == -1) {
        wifi_scan_result_handler handler;
        wifi_scan_cmd_params dummy_params;
        wifi_handle handle = getWifiHandle(iface);
        memset(&handler, 0, sizeof(handler));

        ScanCommand *cmd = new ScanCommand(iface, id, &dummy_params, handler);
        cmd->cancel();
        cmd->releaseRef();
        return WIFI_SUCCESS;
    }


    WifiCommand *cmd = wifi_unregister_cmd(handle, id);
    if (cmd) {
        cmd->cancel();
        cmd->releaseRef();
        return WIFI_SUCCESS;
    }

    return WIFI_ERROR_INVALID_ARGS;
}


wifi_error wifi_enable_full_scan_results(
        wifi_request_id id,
        wifi_interface_handle iface,
        wifi_scan_result_handler handler)
{
    wifi_handle handle = getWifiHandle(iface);
    int params_dummy;
    ALOGD("Enabling full scan results, halHandle = %p", handle);

    FullScanResultsCommand *cmd = new FullScanResultsCommand(iface, id, &params_dummy, handler);
    wifi_register_cmd(handle, id, cmd);

    return (wifi_error)cmd->start();
}

wifi_error wifi_disable_full_scan_results(wifi_request_id id, wifi_interface_handle iface)
{
    ALOGD("Disabling full scan results");
    wifi_handle handle = getWifiHandle(iface);

    if(id == -1) {
        wifi_scan_result_handler handler;
        wifi_handle handle = getWifiHandle(iface);
        int params_dummy;

        memset(&handler, 0, sizeof(handler));
        FullScanResultsCommand *cmd = new FullScanResultsCommand(iface, 0, &params_dummy, handler);
        cmd->cancel();
        cmd->releaseRef();
        return WIFI_SUCCESS;
    }

    WifiCommand *cmd = wifi_unregister_cmd(handle, id);
    if (cmd) {
        cmd->cancel();
        cmd->releaseRef();
        return WIFI_SUCCESS;
    }

    return WIFI_ERROR_INVALID_ARGS;
}


/////////////////////////////////////////////////////////////////////////////

class GetScanResultsCommand : public WifiCommand {
    wifi_scan_result *mResults;
    int mMax;
    int *mNum;
    int mRetrieved;
    byte mFlush;
    int mCompleted;
public:
    GetScanResultsCommand(wifi_interface_handle iface, byte flush,
            wifi_scan_result *results, int max, int *num)
        : WifiCommand(iface, -1), mResults(results), mMax(max), mNum(num),
                mRetrieved(0), mFlush(flush), mCompleted(0)
    { }

    int createRequest(WifiRequest& request, int num, byte flush) {
        int result = request.create(GOOGLE_OUI, GSCAN_SUBCMD_GET_SCAN_RESULTS);
        if (result < 0) {
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put_u32(GSCAN_ATTRIBUTE_NUM_OF_RESULTS, num);
        if (result < 0) {
            return result;
        }

        result = request.put_u8(GSCAN_ATTRIBUTE_FLUSH_RESULTS, flush);
        if (result < 0) {
            return result;
        }

        request.attr_end(data);
        return WIFI_SUCCESS;
    }

    int execute() {
        WifiRequest request(familyId(), ifaceId());
        ALOGI("retrieving %d scan results", mMax);

        for (int i = 0; i < 10 && mRetrieved < mMax; i++) {
            int result = createRequest(request, (mMax - mRetrieved), mFlush);
            if (result < 0) {
                ALOGE("failed to create request");
                return result;
            }

            int prev_retrieved = mRetrieved;

            result = requestResponse(request);

            if (result != WIFI_SUCCESS) {
                ALOGE("failed to retrieve scan results; result = %d", result);
                return result;
            }

            if (mRetrieved == prev_retrieved || mCompleted) {
                /* no more items left to retrieve */
                break;
            }

            request.destroy();
        }

        ALOGE("GetScanResults read %d results", mRetrieved);
        *mNum = mRetrieved;
        return WIFI_SUCCESS;
    }

    virtual int handleResponse(WifiEvent& reply) {
        ALOGD("In GetScanResultsCommand::handleResponse");

        if (reply.get_cmd() != NL80211_CMD_VENDOR) {
            ALOGD("Ignoring reply with cmd = %d", reply.get_cmd());
            return NL_SKIP;
        }

        int id = reply.get_vendor_id();
        int subcmd = reply.get_vendor_subcmd();

        ALOGD("Id = %0x, subcmd = %d", id, subcmd);

        /*
        if (subcmd != GSCAN_SUBCMD_SCAN_RESULTS) {
            ALOGE("Invalid response to GetScanResultsCommand; ignoring it");
            return NL_SKIP;
        }
        */

        nlattr *vendor_data = reply.get_attribute(NL80211_ATTR_VENDOR_DATA);
        int len = reply.get_vendor_data_len();

        if (vendor_data == NULL || len == 0) {
            ALOGE("no vendor data in GetScanResults response; ignoring it");
            return NL_SKIP;
        }

        for (nl_iterator it(vendor_data); it.has_next(); it.next()) {
            if (it.get_type() == GSCAN_ATTRIBUTE_SCAN_RESULTS_COMPLETE) {
                mCompleted = it.get_u8();
                ALOGI("retrieved mCompleted flag : %d", mCompleted);
            } else if (it.get_type() == GSCAN_ATTRIBUTE_SCAN_RESULTS || it.get_type() == 0) {
                for (nl_iterator it2(it.get()); it2.has_next(); it2.next()) {
                    int scan_id = 0, flags = 0, num = 0;
                    if (it2.get_type() == GSCAN_ATTRIBUTE_SCAN_ID) {
                        scan_id = it.get_u32();
                    } else if (it2.get_type() == GSCAN_ATTRIBUTE_SCAN_FLAGS) {
                        flags = it.get_u8();
                    } else if (it2.get_type() == GSCAN_ATTRIBUTE_NUM_OF_RESULTS) {
                        num = it2.get_u32();
                    } else if (it2.get_type() == GSCAN_ATTRIBUTE_SCAN_RESULTS) {
                        num = it2.get_len() / sizeof(wifi_scan_result);
                        num = min(*mNum - mRetrieved, num);
                        memcpy(mResults + mRetrieved, it2.get_data(),
                                sizeof(wifi_scan_result) * num);
                        ALOGI("Retrieved %d scan results", num);
                        wifi_scan_result *results = (wifi_scan_result *)it2.get_data();
                        for (int i = 0; i < num; i++) {
                            wifi_scan_result *result = results + i;
                            ALOGI("%02d  %-32s  %02x:%02x:%02x:%02x:%02x:%02x  %04d", i,
                                result->ssid, result->bssid[0], result->bssid[1], result->bssid[2],
                                result->bssid[3], result->bssid[4], result->bssid[5],
                                result->rssi);
                        }
                        mRetrieved += num;
                    } else {
                        ALOGW("Ignoring invalid attribute type = %d, size = %d",
                                it.get_type(), it.get_len());
                    }
                }
            } else {
                ALOGW("Ignoring invalid attribute type = %d, size = %d",
                        it.get_type(), it.get_len());
            }
        }

        return NL_OK;
    }
};

wifi_error wifi_get_cached_gscan_results(wifi_interface_handle iface, byte flush,
        int max, wifi_scan_result *results, int *num) {

    ALOGD("Getting cached scan results, iface handle = %p, num = %d", iface, *num);

    GetScanResultsCommand *cmd = new GetScanResultsCommand(iface, flush, results, max, num);
    return (wifi_error)cmd->execute();
}

/////////////////////////////////////////////////////////////////////////////

class BssidHotlistCommand : public WifiCommand
{
private:
    wifi_bssid_hotlist_params mParams;
    wifi_hotlist_ap_found_handler mHandler;
    static const int MAX_RESULTS = 64;
    wifi_scan_result mResults[MAX_RESULTS];
public:
    BssidHotlistCommand(wifi_interface_handle handle, int id,
            wifi_bssid_hotlist_params params, wifi_hotlist_ap_found_handler handler)
        : WifiCommand(handle, id), mParams(params), mHandler(handler)
    { }

    int createSetupRequest(WifiRequest& request) {
        int result = request.create(GOOGLE_OUI, GSCAN_SUBCMD_SET_HOTLIST);
        if (result < 0) {
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put_u8(GSCAN_ATTRIBUTE_HOTLIST_FLUSH, 1);
        if (result < 0) {
            return result;
        }

        result = request.put_u32(GSCAN_ATTRIBUTE_LOST_AP_SAMPLE_SIZE, mParams.lost_ap_sample_size);
        if (result < 0) {
            return result;
        }

        struct nlattr * attr = request.attr_start(GSCAN_ATTRIBUTE_HOTLIST_BSSIDS);
        for (int i = 0; i < mParams.num_ap; i++) {
            nlattr *attr2 = request.attr_start(GSCAN_ATTRIBUTE_HOTLIST_ELEM);
            if (attr2 == NULL) {
                return WIFI_ERROR_OUT_OF_MEMORY;
            }
            result = request.put_addr(GSCAN_ATTRIBUTE_BSSID, mParams.ap[i].bssid);
            if (result < 0) {
                return result;
            }
            result = request.put_u8(GSCAN_ATTRIBUTE_RSSI_HIGH, mParams.ap[i].high);
            if (result < 0) {
                return result;
            }
            result = request.put_u8(GSCAN_ATTRIBUTE_RSSI_LOW, mParams.ap[i].low);
            if (result < 0) {
                return result;
            }
            request.attr_end(attr2);
        }

        request.attr_end(attr);
        request.attr_end(data);
        return result;
    }

    int createTeardownRequest(WifiRequest& request) {
        int result = request.create(GOOGLE_OUI, GSCAN_SUBCMD_SET_HOTLIST);
        if (result < 0) {
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put_u8(GSCAN_ATTRIBUTE_HOTLIST_FLUSH, 1);
        if (result < 0) {
            return result;
        }

        struct nlattr * attr = request.attr_start(GSCAN_ATTRIBUTE_HOTLIST_BSSIDS);
        request.attr_end(attr);
        request.attr_end(data);
        return result;
    }

    int start() {
        ALOGI("Executing hotlist setup request, num = %d", mParams.num_ap);
        WifiRequest request(familyId(), ifaceId());
        int result = createSetupRequest(request);
        if (result < 0) {
            return result;
        }

        result = requestResponse(request);
        if (result < 0) {
            ALOGI("Failed to execute hotlist setup request, result = %d", result);
            unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_HOTLIST_RESULTS_FOUND);
            unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_HOTLIST_RESULTS_LOST);
            return result;
        }

        ALOGI("Successfully set %d APs in the hotlist", mParams.num_ap);
        result = createFeatureRequest(request, GSCAN_SUBCMD_ENABLE_GSCAN, 1);
        if (result < 0) {
            return result;
        }

        registerVendorHandler(GOOGLE_OUI, GSCAN_EVENT_HOTLIST_RESULTS_FOUND);
        registerVendorHandler(GOOGLE_OUI, GSCAN_EVENT_HOTLIST_RESULTS_LOST);

        result = requestResponse(request);
        if (result < 0) {
            unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_HOTLIST_RESULTS_FOUND);
            unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_HOTLIST_RESULTS_LOST);
            return result;
        }

        ALOGI("successfully restarted the scan");
        return result;
    }

    virtual int cancel() {
        /* unregister event handler */
        unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_HOTLIST_RESULTS_FOUND);
        unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_HOTLIST_RESULTS_LOST);
        /* create set hotlist message with empty hotlist */
        WifiRequest request(familyId(), ifaceId());
        int result = createTeardownRequest(request);
        if (result < 0) {
            return result;
        }

        result = requestResponse(request);
        if (result < 0) {
            return result;
        }

        ALOGI("Successfully reset APs in current hotlist");
        return result;
    }

    virtual int handleResponse(WifiEvent& reply) {
        /* Nothing to do on response! */
        return NL_SKIP;
    }

    virtual int handleEvent(WifiEvent& event) {
        ALOGI("Hotlist AP event");
        int event_id = event.get_vendor_subcmd();
        // event.log();

        nlattr *vendor_data = event.get_attribute(NL80211_ATTR_VENDOR_DATA);
        int len = event.get_vendor_data_len();

        if (vendor_data == NULL || len == 0) {
            ALOGI("No scan results found");
            return NL_SKIP;
        }

        memset(mResults, 0, sizeof(wifi_scan_result) * MAX_RESULTS);

        int num = len / sizeof(wifi_scan_result);
        num = min(MAX_RESULTS, num);
        memcpy(mResults, event.get_vendor_data(), num * sizeof(wifi_scan_result));

        if (event_id == GSCAN_EVENT_HOTLIST_RESULTS_FOUND) {
            ALOGI("FOUND %d hotlist APs", num);
            if (*mHandler.on_hotlist_ap_found)
                (*mHandler.on_hotlist_ap_found)(id(), num, mResults);
        } else if (event_id == GSCAN_EVENT_HOTLIST_RESULTS_LOST) {
            ALOGI("LOST %d hotlist APs", num);
            if (*mHandler.on_hotlist_ap_lost)
                (*mHandler.on_hotlist_ap_lost)(id(), num, mResults);
        }
        return NL_SKIP;
    }
};

wifi_error wifi_set_bssid_hotlist(wifi_request_id id, wifi_interface_handle iface,
        wifi_bssid_hotlist_params params, wifi_hotlist_ap_found_handler handler)
{
    wifi_handle handle = getWifiHandle(iface);

    BssidHotlistCommand *cmd = new BssidHotlistCommand(iface, id, params, handler);
    wifi_register_cmd(handle, id, cmd);
    return (wifi_error)cmd->start();
}

wifi_error wifi_reset_bssid_hotlist(wifi_request_id id, wifi_interface_handle iface)
{
    wifi_handle handle = getWifiHandle(iface);

    WifiCommand *cmd = wifi_unregister_cmd(handle, id);
    if (cmd) {
        cmd->cancel();
        cmd->releaseRef();
        return WIFI_SUCCESS;
    }

    return WIFI_ERROR_INVALID_ARGS;
}


/////////////////////////////////////////////////////////////////////////////

class SignificantWifiChangeCommand : public WifiCommand
{
    typedef struct {
        mac_addr bssid;                     // BSSID
        wifi_channel channel;               // channel frequency in MHz
        int num_rssi;                       // number of rssi samples
        wifi_rssi rssi[8];                   // RSSI history in db
    } wifi_significant_change_result_internal;

private:
    wifi_significant_change_params mParams;
    wifi_significant_change_handler mHandler;
    static const int MAX_RESULTS = 64;
    wifi_significant_change_result_internal mResultsBuffer[MAX_RESULTS];
    wifi_significant_change_result *mResults[MAX_RESULTS];
public:
    SignificantWifiChangeCommand(wifi_interface_handle handle, int id,
            wifi_significant_change_params params, wifi_significant_change_handler handler)
        : WifiCommand(handle, id), mParams(params), mHandler(handler)
    { }

    int createSetupRequest(WifiRequest& request) {
        int result = request.create(GOOGLE_OUI, GSCAN_SUBCMD_SET_SIGNIFICANT_CHANGE_CONFIG);
        if (result < 0) {
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put_u8(GSCAN_ATTRIBUTE_SIGNIFICANT_CHANGE_FLUSH, 1);
        if (result < 0) {
            return result;
        }
        result = request.put_u16(GSCAN_ATTRIBUTE_RSSI_SAMPLE_SIZE, mParams.rssi_sample_size);
        if (result < 0) {
            return result;
        }
        result = request.put_u16(GSCAN_ATTRIBUTE_LOST_AP_SAMPLE_SIZE, mParams.lost_ap_sample_size);
        if (result < 0) {
            return result;
        }
        result = request.put_u16(GSCAN_ATTRIBUTE_MIN_BREACHING, mParams.min_breaching);
        if (result < 0) {
            return result;
        }

        struct nlattr * attr = request.attr_start(GSCAN_ATTRIBUTE_SIGNIFICANT_CHANGE_BSSIDS);

        for (int i = 0; i < mParams.num_ap; i++) {

            nlattr *attr2 = request.attr_start(i);
            if (attr2 == NULL) {
                return WIFI_ERROR_OUT_OF_MEMORY;
            }
            result = request.put_addr(GSCAN_ATTRIBUTE_BSSID, mParams.ap[i].bssid);
            if (result < 0) {
                return result;
            }
            result = request.put_u8(GSCAN_ATTRIBUTE_RSSI_HIGH, mParams.ap[i].high);
            if (result < 0) {
                return result;
            }
            result = request.put_u8(GSCAN_ATTRIBUTE_RSSI_LOW, mParams.ap[i].low);
            if (result < 0) {
                return result;
            }
            request.attr_end(attr2);
        }

        request.attr_end(attr);
        request.attr_end(data);

        return result;
    }

    int createTeardownRequest(WifiRequest& request) {
        int result = request.create(GOOGLE_OUI, GSCAN_SUBCMD_SET_SIGNIFICANT_CHANGE_CONFIG);
        if (result < 0) {
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put_u16(GSCAN_ATTRIBUTE_SIGNIFICANT_CHANGE_FLUSH, 1);
        if (result < 0) {
            return result;
        }

        request.attr_end(data);
        return result;
    }

    int start() {
        ALOGI("Set significant wifi change config");
        WifiRequest request(familyId(), ifaceId());

        int result = createSetupRequest(request);
        if (result < 0) {
            return result;
        }

        result = requestResponse(request);
        if (result < 0) {
            ALOGI("failed to set significant wifi change config %d", result);
            return result;
        }

        ALOGI("successfully set significant wifi change config");

        result = createFeatureRequest(request, GSCAN_SUBCMD_ENABLE_GSCAN, 1);
        if (result < 0) {
            return result;
        }

        registerVendorHandler(GOOGLE_OUI, GSCAN_EVENT_SIGNIFICANT_CHANGE_RESULTS);

        result = requestResponse(request);
        if (result < 0) {
            unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_SIGNIFICANT_CHANGE_RESULTS);
            return result;
        }

        ALOGI("successfully restarted the scan");
        return result;
    }

    virtual int cancel() {
        /* unregister event handler */
        unregisterVendorHandler(GOOGLE_OUI, GSCAN_EVENT_SIGNIFICANT_CHANGE_RESULTS);

        /* create set significant change monitor message with empty hotlist */
        WifiRequest request(familyId(), ifaceId());

        int result = createTeardownRequest(request);
        if (result < 0) {
            return result;
        }

        result = requestResponse(request);
        if (result < 0) {
            return result;
        }

        ALOGI("successfully reset significant wifi change config");
        return result;
    }

    virtual int handleResponse(WifiEvent& reply) {
        /* Nothing to do on response! */
        return NL_SKIP;
    }

    virtual int handleEvent(WifiEvent& event) {
        ALOGI("Got a significant wifi change event");

        nlattr *vendor_data = event.get_attribute(NL80211_ATTR_VENDOR_DATA);
        int len = event.get_vendor_data_len();

        if (vendor_data == NULL || len == 0) {
            ALOGI("No scan results found");
            return NL_SKIP;
        }

        typedef struct {
            uint16_t flags;
            uint16_t channel;
            mac_addr bssid;
            s8 rssi_history[8];
        } ChangeInfo;

        int num = min(len / sizeof(ChangeInfo), MAX_RESULTS);
        ChangeInfo *ci = (ChangeInfo *)event.get_vendor_data();

        for (int i = 0; i < num; i++) {
            memcpy(mResultsBuffer[i].bssid, ci[i].bssid, sizeof(mac_addr));
            mResultsBuffer[i].channel = ci[i].channel;
            mResultsBuffer[i].num_rssi = 8;
            for (int j = 0; j < mResultsBuffer[i].num_rssi; j++)
                mResultsBuffer[i].rssi[j] = (int) ci[i].rssi_history[j];
            mResults[i] = reinterpret_cast<wifi_significant_change_result *>(&(mResultsBuffer[i]));
        }

        ALOGI("Retrieved %d scan results", num);

        if (num != 0) {
            (*mHandler.on_significant_change)(id(), num, mResults);
        } else {
            ALOGW("No significant change reported");
        }

        return NL_SKIP;
    }
};

wifi_error wifi_set_significant_change_handler(wifi_request_id id, wifi_interface_handle iface,
        wifi_significant_change_params params, wifi_significant_change_handler handler)
{
    wifi_handle handle = getWifiHandle(iface);

    SignificantWifiChangeCommand *cmd = new SignificantWifiChangeCommand(
            iface, id, params, handler);
    wifi_register_cmd(handle, id, cmd);
    return (wifi_error)cmd->start();
}

wifi_error wifi_reset_significant_change_handler(wifi_request_id id, wifi_interface_handle iface)
{
    wifi_handle handle = getWifiHandle(iface);

    WifiCommand *cmd = wifi_unregister_cmd(handle, id);
    if (cmd) {
        cmd->cancel();
        cmd->releaseRef();
        return WIFI_SUCCESS;
    }

    return WIFI_ERROR_INVALID_ARGS;
}
