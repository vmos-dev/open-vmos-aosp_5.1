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

#include "wifi_hal.h"
#include "common.h"
#include "cpp_bindings.h"
#include "llstatscommand.h"

//Singleton Static Instance
LLStatsCommand* LLStatsCommand::mLLStatsCommandInstance  = NULL;

// This function implements creation of Vendor command
// For LLStats just call base Vendor command create
int LLStatsCommand::create() {
    int ifindex;
    int ret = mMsg.create(NL80211_CMD_VENDOR, 0, 0);
    if (ret < 0) {
        return ret;
    }
    // insert the oui in the msg
    ret = mMsg.put_u32(NL80211_ATTR_VENDOR_ID, mVendor_id);
    if (ret < 0)
        goto out;

    // insert the subcmd in the msg
    ret = mMsg.put_u32(NL80211_ATTR_VENDOR_SUBCMD, mSubcmd);
    if (ret < 0)
        goto out;

    ALOGI("mVendor_id = %d, Subcmd = %d in  %s:%d\n", mVendor_id, mSubcmd, __func__, __LINE__);
out:
    return ret;
}

LLStatsCommand::LLStatsCommand(wifi_handle handle, int id, u32 vendor_id, u32 subcmd)
        : WifiVendorCommand(handle, id, vendor_id, subcmd)
{
    ALOGV("LLStatsCommand %p constructed", this);
    memset(&mClearRspParams, 0,sizeof(LLStatsClearRspParams));
    memset(&mResultsParams, 0,sizeof(LLStatsResultsParams));
    memset(&mHandler, 0,sizeof(mHandler));
}

LLStatsCommand::~LLStatsCommand()
{
    ALOGW("LLStatsCommand %p distructor", this);
    mLLStatsCommandInstance = NULL;
    unregisterVendorHandler(mVendor_id, mSubcmd);
}

LLStatsCommand* LLStatsCommand::instance(wifi_handle handle)
{
    if (handle == NULL) {
        ALOGE("Interface Handle is invalid");
        return NULL;
    }
    if (mLLStatsCommandInstance == NULL) {
        mLLStatsCommandInstance = new LLStatsCommand(handle, 0,
                OUI_QCA,
                QCA_NL80211_VENDOR_SUBCMD_LL_STATS_SET);
        ALOGV("LLStatsCommand %p created", mLLStatsCommandInstance);
        return mLLStatsCommandInstance;
    }
    else
    {
        if (handle != getWifiHandle(mLLStatsCommandInstance->mInfo))
        {
            ALOGE("Handle different");
            return NULL;
        }
    }
    ALOGV("LLStatsCommand %p created already", mLLStatsCommandInstance);
    return mLLStatsCommandInstance;
}

void LLStatsCommand::initGetContext(u32 reqId)
{
    mRequestId = reqId;
    memset(&mResultsParams, 0,sizeof(LLStatsResultsParams));
    memset(&mHandler, 0,sizeof(mHandler));
}

void LLStatsCommand::setSubCmd(u32 subcmd)
{
    mSubcmd = subcmd;
}
//callback handlers registered for nl message send
static int error_handler_LLStats(struct sockaddr_nl *nla, struct nlmsgerr *err,
                         void *arg)
{
    struct sockaddr_nl * tmp;
    int *ret = (int *)arg;
    tmp = nla;
    *ret = err->error;
    ALOGE("%s: Error code:%d (%s)", __func__, *ret, strerror(-(*ret)));
    return NL_STOP;
}

//callback handlers registered for nl message send
static int ack_handler_LLStats(struct nl_msg *msg, void *arg)
{
    int *ret = (int *)arg;
    struct nl_msg * a;

    ALOGE("%s: called", __func__);
    a = msg;
    *ret = 0;
    return NL_STOP;
}

//callback handlers registered for nl message send
static int finish_handler_LLStats(struct nl_msg *msg, void *arg)
{
  int *ret = (int *)arg;
  struct nl_msg * a;

  ALOGE("%s: called", __func__);
  a = msg;
  *ret = 0;
  return NL_SKIP;
}

static void get_wifi_interface_info(wifi_interface_link_layer_info *stats, struct nlattr **tb_vendor)
{
    u32 len = 0;
    u8 *data;

    stats->mode = (wifi_interface_mode)nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_MODE]);
    len = nla_len(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_MAC_ADDR]);
    len = ((sizeof(stats->mac_addr) <= len) ? sizeof(stats->mac_addr) : len);
    memcpy(&stats->mac_addr[0], nla_data(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_MAC_ADDR]), len);
    stats->state = (wifi_connection_state)nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_STATE]);
    stats->roaming = (wifi_roam_state)nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_ROAMING]);
    stats->capabilities = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_CAPABILITIES]);

    len = nla_len(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_SSID]);
    len = ((sizeof(stats->ssid) <= len) ? sizeof(stats->ssid) : len);
    memcpy(&stats->ssid[0], nla_data(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_SSID]), len);

    len = nla_len(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_BSSID]);
    len = ((sizeof(stats->bssid) <= len) ? sizeof(stats->bssid) : len);
    memcpy(&stats->bssid[0], nla_data(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_BSSID]), len);
    len = nla_len(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_AP_COUNTRY_STR]);
    len = ((sizeof(stats->ap_country_str) <= len) ? sizeof(stats->ap_country_str) : len);
    memcpy(&stats->ap_country_str[0], nla_data(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_AP_COUNTRY_STR]),
           len);
    len = nla_len(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_COUNTRY_STR]);
    len = ((sizeof(stats->country_str) < len) ? sizeof(stats->country_str) : len);
    memcpy(&stats->country_str[0], nla_data(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_COUNTRY_STR]),
           len);

    ALOGI("STATS IFACE: Mode %d", stats->mode);
    ALOGI("STATS IFACE: MAC %pM", stats->mac_addr);
    ALOGI("STATS IFACE: State %d ", stats->state);
    ALOGI("STATS IFACE: Roaming %d ", stats->roaming);
    ALOGI("STATS IFACE: capabilities %0x ", stats->capabilities);
    ALOGI("STATS IFACE: SSID %s ", stats->ssid);
    ALOGI("STATS IFACE: BSSID %pM ", stats->bssid);
    ALOGI("STATS IFACE: AP country str %c%c%c ", stats->ap_country_str[0],
            stats->ap_country_str[1], stats->ap_country_str[2]);
    ALOGI("STATS IFACE:Country String for this Association %c%c%c", stats->country_str[0],
            stats->country_str[1], stats->country_str[2]);
}

static void get_wifi_wmm_ac_stat(wifi_wmm_ac_stat *stats, struct nlattr **tb_vendor)
{
    stats->ac                     = (wifi_traffic_ac)nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_AC]);
    stats->tx_mpdu                = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_TX_MPDU]);
    stats->rx_mpdu                = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_RX_MPDU]);
    stats->tx_mcast               = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_TX_MCAST]);
    stats->rx_mcast               = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_RX_MCAST]);
    stats->rx_ampdu               = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_RX_AMPDU]);
    stats->tx_ampdu               = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_TX_AMPDU]);
    stats->mpdu_lost              = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_MPDU_LOST]);
    stats->retries                = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_RETRIES]);
    stats->retries_short          = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_RETRIES_SHORT]);
    stats->retries_long           = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_RETRIES_LONG]);
    stats->contention_time_min    = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_CONTENTION_TIME_MIN]);
    stats->contention_time_max    = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_CONTENTION_TIME_MAX]);
    stats->contention_time_avg    = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_CONTENTION_TIME_AVG]);
    stats->contention_num_samples = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_CONTENTION_NUM_SAMPLES]);

    ALOGI("STATS IFACE: ac  %u ", stats->ac);
    ALOGI("STATS IFACE: txMpdu  %u ", stats->tx_mpdu) ;
    ALOGI("STATS IFACE: rxMpdu  %u ", stats->rx_mpdu);
    ALOGI("STATS IFACE: txMcast  %u ", stats->tx_mcast);
    ALOGI("STATS IFACE: rxMcast  %u ", stats->rx_mcast);
    ALOGI("STATS IFACE: rxAmpdu  %u ", stats->rx_ampdu);
    ALOGI("STATS IFACE: txAmpdu  %u ", stats->tx_ampdu);
    ALOGI("STATS IFACE: mpduLost  %u ", stats->mpdu_lost);
    ALOGI("STATS IFACE: retries %u  ", stats->retries);
    ALOGI("STATS IFACE: retriesShort  %u ",
            stats->retries_short);
    ALOGI("STATS IFACE: retriesLong  %u  ",
            stats->retries_long);
    ALOGI("STATS IFACE: contentionTimeMin  %u ",
            stats->contention_time_min);
    ALOGI("STATS IFACE: contentionTimeMax  %u ",
            stats->contention_time_max);
    ALOGI("STATS IFACE: contentionTimeAvg  %u ",
            stats->contention_time_avg);
    ALOGI("STATS IFACE: contentionNumSamples  %u ",
            stats->contention_num_samples);
}

static void get_wifi_rate_stat(wifi_rate_stat *stats, struct nlattr **tb_vendor)
{
    stats->rate.preamble        = nla_get_u8(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_PREAMBLE]);
    stats->rate.nss             = nla_get_u8(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_NSS]);
    stats->rate.bw              = nla_get_u8(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_BW]);
    stats->rate.rateMcsIdx      = nla_get_u8(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_MCS_INDEX]);
    stats->rate.bitrate         = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_BIT_RATE]);

    stats->tx_mpdu              = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_TX_MPDU]);
    stats->rx_mpdu              = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_RX_MPDU]);
    stats->mpdu_lost            = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_MPDU_LOST]);
    stats->retries              = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_RETRIES]);
    stats->retries_short        = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_RETRIES_SHORT]);
    stats->retries_long         = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_RETRIES_LONG]);


    ALOGI("STATS PEER_ALL : preamble  %u", stats->rate.preamble);
    ALOGI("STATS PEER_ALL : nss %u", stats->rate.nss);
    ALOGI("STATS PEER_ALL : bw %u", stats->rate.bw);
    ALOGI("STATS PEER_ALL : rateMcsIdx  %u", stats->rate.rateMcsIdx);
    ALOGI("STATS PEER_ALL : bitrate %u", stats->rate.bitrate);

    ALOGI("STATS PEER_ALL : txMpdu %u", stats->tx_mpdu);
    ALOGI("STATS PEER_ALL : rxMpdu %u", stats->rx_mpdu);
    ALOGI("STATS PEER_ALL : mpduLost %u", stats->mpdu_lost);
    ALOGI("STATS PEER_ALL : retries %u", stats->retries);
    ALOGI("STATS PEER_ALL : retriesShort %u", stats->retries_short);
    ALOGI("STATS PEER_ALL : retriesLong %u", stats->retries_long);
}

static void get_wifi_peer_info(wifi_peer_info *stats, struct nlattr **tb_vendor)
{
    u32 i = 0, len = 0;
    int rem;
    wifi_rate_stat * pRateStats;
    struct nlattr *rateInfo;
    stats->type                   = (wifi_peer_type)nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_PEER_INFO_TYPE]);
    len = nla_len(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_PEER_INFO_MAC_ADDRESS]);
    len = ((sizeof(stats->peer_mac_address) <= len) ? sizeof(stats->peer_mac_address) : len);
    memcpy((void *)&stats->peer_mac_address[0], nla_data(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_PEER_INFO_MAC_ADDRESS]),
            len);
    stats->capabilities           = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_PEER_INFO_CAPABILITIES]);

    stats->num_rate               = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_PEER_INFO_NUM_RATES]);

    ALOGI("STATS PEER_ALL : numPeers %u", stats->type);
    ALOGI("STATS PEER_ALL : peerMacAddress  %0x:%0x:%0x:%0x:%0x:%0x ",
            stats->peer_mac_address[0], stats->peer_mac_address[1],
            stats->peer_mac_address[2],stats->peer_mac_address[3],
            stats->peer_mac_address[4],stats->peer_mac_address[5]);
    ALOGI("STATS PEER_ALL : capabilities %0x", stats->capabilities);
    ALOGI("STATS PEER_ALL :  numRate %u", stats->num_rate);
    for (rateInfo = (struct nlattr *) nla_data(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_PEER_INFO_RATE_INFO]), rem = nla_len(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_PEER_INFO_RATE_INFO]);
            nla_ok(rateInfo, rem);
            rateInfo = nla_next(rateInfo, &(rem)))
    {
        struct nlattr *tb2[ QCA_WLAN_VENDOR_ATTR_LL_STATS_MAX+ 1];
        pRateStats = (wifi_rate_stat *) ((u8 *)stats->rate_stats + (i++ * sizeof(wifi_rate_stat)));

        nla_parse(tb2, QCA_WLAN_VENDOR_ATTR_LL_STATS_MAX, (struct nlattr *) nla_data(rateInfo), nla_len(rateInfo), NULL);
        get_wifi_rate_stat(pRateStats, tb2);
    }
}

static void get_wifi_iface_stats(wifi_iface_stat *stats, struct nlattr **tb_vendor)
{
    struct nlattr *wmmInfo;
    wifi_wmm_ac_stat *pWmmStats;
    int i=0, rem;

    stats->beacon_rx       = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_BEACON_RX]);
    stats->mgmt_rx         = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_MGMT_RX]);
    stats->mgmt_action_rx  = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_MGMT_ACTION_RX]);
    stats->mgmt_action_tx  = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_MGMT_ACTION_TX]);
    stats->rssi_mgmt       = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_RSSI_MGMT]);
    stats->rssi_data       = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_RSSI_DATA]);
    stats->rssi_ack        = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_RSSI_ACK]);

    ALOGI("STATS IFACE: beaconRx : %u ", stats->beacon_rx);
    ALOGI("STATS IFACE: mgmtRx %u ", stats->mgmt_rx);
    ALOGI("STATS IFACE: mgmtActionRx  %u ", stats->mgmt_action_rx);
    ALOGI("STATS IFACE: mgmtActionTx %u ", stats->mgmt_action_tx);
    ALOGI("STATS IFACE: rssiMgmt %u ", stats->rssi_mgmt);
    ALOGI("STATS IFACE: rssiData %u ", stats->rssi_data);
    ALOGI("STATS IFACE: rssiAck  %u ", stats->rssi_ack);
    for (wmmInfo = (struct nlattr *) nla_data(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_INFO]), rem = nla_len(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_INFO]);
            nla_ok(wmmInfo, rem);
            wmmInfo = nla_next(wmmInfo, &(rem)))
    {
        struct nlattr *tb2[ QCA_WLAN_VENDOR_ATTR_LL_STATS_MAX+ 1];
        pWmmStats = (wifi_wmm_ac_stat *) ((u8 *)stats->ac + (i * sizeof(wifi_wmm_ac_stat)));
        nla_parse(tb2, QCA_WLAN_VENDOR_ATTR_LL_STATS_MAX, (struct nlattr *) nla_data(wmmInfo), nla_len(wmmInfo), NULL);
        get_wifi_wmm_ac_stat(pWmmStats, tb2);
    }
}

static void get_wifi_radio_stats(wifi_radio_stat *stats, struct nlattr **tb_vendor)
{
    u32 i = 0;
    struct nlattr *chInfo;
    wifi_channel_stat *pChStats;
    int rem;
                    printf("sunil %d : %s \n",__LINE__,__func__);

    stats->radio             = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_ID]);
                    printf("sunil %d : %s \n",__LINE__,__func__);
    stats->on_time           = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_ON_TIME]);
                    printf("sunil %d : %s \n",__LINE__,__func__);
    stats->tx_time           = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_TX_TIME]);
                    printf("sunil %d : %s \n",__LINE__,__func__);
    stats->rx_time           = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_RX_TIME]);
    ALOGI("<<<< rxTime is %u ", stats->rx_time);
    stats->on_time_scan      = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_ON_TIME_SCAN]);
    stats->on_time_nbd       = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_ON_TIME_NBD]);
    stats->on_time_gscan     = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_ON_TIME_GSCAN]);
    stats->on_time_roam_scan = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_ON_TIME_ROAM_SCAN]);
    stats->on_time_pno_scan  = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_ON_TIME_PNO_SCAN]);
    stats->on_time_hs20      = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_ON_TIME_HS20]);

    stats->num_channels                           = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_NUM_CHANNELS]);

    for (chInfo = (struct nlattr *) nla_data(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_CH_INFO]), rem = nla_len(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_CH_INFO]);
            nla_ok(chInfo, rem);
            chInfo = nla_next(chInfo, &(rem)))
    {
        struct nlattr *tb2[ QCA_WLAN_VENDOR_ATTR_LL_STATS_MAX+ 1];
        pChStats = (wifi_channel_stat *) ((u8 *)stats->channels + (i++ * (sizeof(wifi_channel_stat))));
        nla_parse(tb2, QCA_WLAN_VENDOR_ATTR_LL_STATS_MAX, (struct nlattr *) nla_data(chInfo), nla_len(chInfo), NULL);
        pChStats->channel.width                  = (wifi_channel_width)nla_get_u32(tb2[QCA_WLAN_VENDOR_ATTR_LL_STATS_CHANNEL_INFO_WIDTH]);
        pChStats->channel.center_freq            = (wifi_channel)nla_get_u32(tb2[QCA_WLAN_VENDOR_ATTR_LL_STATS_CHANNEL_INFO_CENTER_FREQ]);
        pChStats->channel.center_freq0           = (wifi_channel)nla_get_u32(tb2[QCA_WLAN_VENDOR_ATTR_LL_STATS_CHANNEL_INFO_CENTER_FREQ0]);
        pChStats->channel.center_freq1           = (wifi_channel)nla_get_u32(tb2[QCA_WLAN_VENDOR_ATTR_LL_STATS_CHANNEL_INFO_CENTER_FREQ1]);
        pChStats->on_time                = nla_get_u32(tb2[QCA_WLAN_VENDOR_ATTR_LL_STATS_CHANNEL_ON_TIME]);
        pChStats->cca_busy_time          = nla_get_u32(tb2[QCA_WLAN_VENDOR_ATTR_LL_STATS_CHANNEL_CCA_BUSY_TIME]);
    }
}

// This function will be the main handler for incoming event LLStats_SUBCMD
//Call the appropriate callback handler after parsing the vendor data.
int LLStatsCommand::handleEvent(WifiEvent &event)
{
    ALOGI("Got a LLStats message from Driver");
    unsigned i=0;
    u32 status;
    WifiVendorCommand::handleEvent(event);

    // Parse the vendordata and get the attribute

    switch(mSubcmd)
    {
        case QCA_NL80211_VENDOR_SUBCMD_LL_STATS_RADIO_RESULTS:
            {
                wifi_request_id id;
                u32 resultsBufSize = 0;
                struct nlattr *tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_MAX + 1];
                int rem;
                nla_parse(tb_vendor, QCA_WLAN_VENDOR_ATTR_LL_STATS_MAX,
                        (struct nlattr *)mVendorData,
                        mDataLen, NULL);
                resultsBufSize += (nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_NUM_CHANNELS]) * sizeof(wifi_channel_stat)
                        + sizeof(wifi_radio_stat));
                mResultsParams.radio_stat = (wifi_radio_stat *)malloc(resultsBufSize);
                memset(mResultsParams.radio_stat, 0, resultsBufSize);
                ALOGI(" rxTime is %u\n ", mResultsParams.radio_stat->rx_time);
                ALOGI(" NumChan is %d\n ",
                        nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_NUM_CHANNELS]));

                if(mResultsParams.radio_stat){
                    wifi_channel_stat *pWifiChannelStats;
                    u32 i =0;
                    printf("sunil %d : %s \n",__LINE__,__func__);
                    get_wifi_radio_stats(mResultsParams.radio_stat, tb_vendor);

                    ALOGI(" radio is %u ", mResultsParams.radio_stat->radio);
                    ALOGI(" onTime is %u ", mResultsParams.radio_stat->on_time);
                    ALOGI(" txTime is %u ", mResultsParams.radio_stat->tx_time);
                    ALOGI(" rxTime is %u ", mResultsParams.radio_stat->rx_time);
                    ALOGI(" onTimeScan is %u ", mResultsParams.radio_stat->on_time_scan);
                    ALOGI(" onTimeNbd is %u ", mResultsParams.radio_stat->on_time_nbd);
                    ALOGI(" onTimeGscan is %u ", mResultsParams.radio_stat->on_time_gscan);
                    ALOGI(" onTimeRoamScan is %u", mResultsParams.radio_stat->on_time_roam_scan);
                    ALOGI(" onTimePnoScan is %u ", mResultsParams.radio_stat->on_time_pno_scan);
                    ALOGI(" onTimeHs20 is %u ", mResultsParams.radio_stat->on_time_hs20);
                    ALOGI(" numChannels is %u ", mResultsParams.radio_stat->num_channels);
                    for ( i=0; i < mResultsParams.radio_stat->num_channels; i++)
                    {
                        pWifiChannelStats = (wifi_channel_stat *) ((u8 *)mResultsParams.radio_stat->channels + (i * sizeof(wifi_channel_stat)));

                        ALOGI("  width is %u ", pWifiChannelStats->channel.width);
                        ALOGI("  CenterFreq %u ", pWifiChannelStats->channel.center_freq);
                        ALOGI("  CenterFreq0 %u ", pWifiChannelStats->channel.center_freq0);
                        ALOGI("  CenterFreq1 %u ", pWifiChannelStats->channel.center_freq1);
                        ALOGI("  onTime %u ", pWifiChannelStats->on_time);
                        ALOGI("  ccaBusyTime %u ", pWifiChannelStats->cca_busy_time);
                    }
                    ALOGI(" rxTime is %u in %s:%d\n", mResultsParams.radio_stat->rx_time, __func__, __LINE__);
                }
            }
            break;

        case QCA_NL80211_VENDOR_SUBCMD_LL_STATS_IFACE_RESULTS:
            {
                wifi_request_id id;
                u32 resultsBufSize = 0;
                struct nlattr *tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_MAX + 1];
                nla_parse(tb_vendor, QCA_WLAN_VENDOR_ATTR_LL_STATS_MAX,
                        (struct nlattr *)mVendorData,
                        mDataLen, NULL);

                resultsBufSize = sizeof(wifi_iface_stat);   // Do we need no.of peers here??
                mResultsParams.iface_stat = (wifi_iface_stat *) malloc (sizeof (wifi_iface_stat));
                get_wifi_interface_info(&mResultsParams.iface_stat->info, tb_vendor);
                get_wifi_iface_stats(mResultsParams.iface_stat, tb_vendor);
            }
            break;

        case QCA_NL80211_VENDOR_SUBCMD_LL_STATS_PEERS_RESULTS:
            {
                wifi_request_id id;
                u32 resultsBufSize = 0, i=0, num_rates = 0;
                u32 numPeers;
                int rem;
                struct nlattr *tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_MAX + 1];
                struct nlattr *peerInfo;
                wifi_iface_stat *pIfaceStat;
                nla_parse(tb_vendor, QCA_WLAN_VENDOR_ATTR_LL_STATS_MAX,
                        (struct nlattr *)mVendorData,
                        mDataLen, NULL);

                ALOGI(" rxTime is %u in %s:%d\n", mResultsParams.radio_stat->rx_time, __func__, __LINE__);
                ALOGI(" numPeers is %u in %s:%d\n", nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_NUM_PEERS]), __func__, __LINE__);
                ALOGI(" rxTe is %u in %s:%d\n", mResultsParams.radio_stat->rx_time, __func__, __LINE__);
                if((numPeers = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_NUM_PEERS])) > 0)
                {

                    for (peerInfo = (struct nlattr *) nla_data(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_PEER_INFO]), rem = nla_len(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_PEER_INFO]);
                            nla_ok(peerInfo, rem);
                            peerInfo = nla_next(peerInfo, &(rem)))
                    {
                        struct nlattr *tb2[ QCA_WLAN_VENDOR_ATTR_LL_STATS_MAX+ 1];
                        nla_parse(tb2, QCA_WLAN_VENDOR_ATTR_LL_STATS_MAX, (struct nlattr *) nla_data(peerInfo), nla_len(peerInfo), NULL);
                        num_rates += nla_get_u32(tb2[QCA_WLAN_VENDOR_ATTR_LL_STATS_PEER_INFO_NUM_RATES]);
                    }
                    resultsBufSize += (numPeers * sizeof(wifi_peer_info)
                            + num_rates * sizeof(wifi_rate_stat) + sizeof (wifi_iface_stat));
                    pIfaceStat = (wifi_iface_stat *) malloc (resultsBufSize);

                    if(pIfaceStat){
                        memcpy ( pIfaceStat, mResultsParams.iface_stat , sizeof(wifi_iface_stat));
                        wifi_peer_info *pPeerStats;
                        pIfaceStat->num_peers = numPeers;
                        for (peerInfo = (struct nlattr *) nla_data(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_PEER_INFO]), rem = nla_len(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_PEER_INFO]);
                                nla_ok(peerInfo, rem);
                                peerInfo = nla_next(peerInfo, &(rem)))
                        {
                            struct nlattr *tb2[ QCA_WLAN_VENDOR_ATTR_LL_STATS_MAX+ 1];
                            pPeerStats = (wifi_peer_info *) ((u8 *)pIfaceStat->peer_info + (i++ * sizeof(wifi_peer_info)));
                            nla_parse(tb2, QCA_WLAN_VENDOR_ATTR_LL_STATS_MAX, (struct nlattr *) nla_data(peerInfo), nla_len(peerInfo), NULL);
                            get_wifi_peer_info(pPeerStats, tb2);
                        }
                    }
                    if(mResultsParams.iface_stat)
                        free (mResultsParams.iface_stat);
                    mResultsParams.iface_stat = pIfaceStat;
                }
                // Number of Radios are 1 for now : TODO get this info from the driver
                mHandler.on_link_stats_results(mRequestId,
                                               mResultsParams.iface_stat, 1, mResultsParams.radio_stat);
                if(mResultsParams.radio_stat)
                {
                    free(mResultsParams.radio_stat);
                    mResultsParams.radio_stat = NULL;
                }
                if(mResultsParams.iface_stat)
                {
                    free(mResultsParams.iface_stat);
                    mResultsParams.iface_stat = NULL;
                }
            }
            break;

        default:
            //error case should not happen print log
            ALOGE("%s: Wrong LLStats subcmd received %d", __func__, mSubcmd);
    }

    return NL_SKIP;
}

int LLStatsCommand::setCallbackHandler(LLStatsCallbackHandler nHandler, u32 event)
{
    int res = 0;
    mHandler = nHandler;
    res = registerVendorHandler(mVendor_id, event);
    if (res != 0) {
        //error case should not happen print log
        ALOGE("%s: Unable to register Vendor Handler Vendor Id=0x%x subcmd=%u",
              __func__, mVendor_id, mSubcmd);
    }
    return res;
}

void LLStatsCommand::unregisterHandler(u32 subCmd)
{
    unregisterVendorHandler(mVendor_id, subCmd);
}

void LLStatsCommand::getClearRspParams(u32 *stats_clear_rsp_mask, u8 *stop_rsp)
{
    *stats_clear_rsp_mask =  mClearRspParams.stats_clear_rsp_mask;
    *stop_rsp = mClearRspParams.stop_rsp;
}

int LLStatsCommand::requestResponse()
{
    return WifiCommand::requestResponse(mMsg);
}

int LLStatsCommand::handleResponse(WifiEvent &reply)
{
    ALOGI("Got a LLStats message from Driver");
    unsigned i=0;
    u32 status;
    WifiVendorCommand::handleResponse(reply);

    // Parse the vendordata and get the attribute

    switch(mSubcmd)
    {
        case QCA_NL80211_VENDOR_SUBCMD_LL_STATS_CLR:
            {
                struct nlattr *tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_CLR_MAX + 1];
                nla_parse(tb_vendor, QCA_WLAN_VENDOR_ATTR_LL_STATS_CLR_MAX,
                        (struct nlattr *)mVendorData,
                        mDataLen, NULL);
                ALOGI("Resp mask : %d\n", nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_CLR_CONFIG_RSP_MASK]));
                ALOGI("STOP resp : %d\n", nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_CLR_CONFIG_STOP_RSP]));
                mClearRspParams.stats_clear_rsp_mask = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_CLR_CONFIG_RSP_MASK]);
                mClearRspParams.stop_rsp = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_LL_STATS_CLR_CONFIG_STOP_RSP]);
                break;
            }
        default :
            ALOGE("%s: Wrong LLStats subcmd received %d", __func__, mSubcmd);
    }
    return NL_SKIP;
}

//Implementation of the functions exposed in linklayer.h
wifi_error wifi_set_link_stats(wifi_interface_handle iface,
                               wifi_link_layer_params params)
{
    int ret = 0;
    LLStatsCommand *LLCommand;
    struct nlattr *nl_data;
    interface_info *iinfo = getIfaceInfo(iface);
    wifi_handle handle = getWifiHandle(iface);
    LLCommand = LLStatsCommand::instance(handle);
    if (LLCommand == NULL) {
        ALOGE("%s: Error LLStatsCommand NULL", __func__);
        return WIFI_ERROR_UNKNOWN;
    }
    LLCommand->setSubCmd(QCA_NL80211_VENDOR_SUBCMD_LL_STATS_SET);

    /* create the message */
    ret = LLCommand->create();
    if (ret < 0)
        goto cleanup;

    ret = LLCommand->set_iface_id(iinfo->name);
    if (ret < 0)
        goto cleanup;

    /*add the attributes*/
    nl_data = LLCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nl_data)
        goto cleanup;
    /**/
    ret = LLCommand->put_u32(QCA_WLAN_VENDOR_ATTR_LL_STATS_SET_CONFIG_MPDU_SIZE_THRESHOLD,
                                  params.mpdu_size_threshold);
    if (ret < 0)
        goto cleanup;
    /**/
    ret = LLCommand->put_u32(
                QCA_WLAN_VENDOR_ATTR_LL_STATS_SET_CONFIG_AGGRESSIVE_STATS_GATHERING,
                params.aggressive_statistics_gathering);
    if (ret < 0)
        goto cleanup;
    LLCommand->attr_end(nl_data);

    ret = LLCommand->requestResponse();
    if (ret != 0) {
        ALOGE("%s: requestResponse Error:%d",__func__, ret);
    }

cleanup:
    return (wifi_error)ret;
}

//Implementation of the functions exposed in LLStats.h
wifi_error wifi_get_link_stats(wifi_request_id id,
                               wifi_interface_handle iface,
                               wifi_stats_result_handler handler)
{
    int ret = 0;
    LLStatsCommand *LLCommand;
    struct nlattr *nl_data;
    interface_info *iinfo = getIfaceInfo(iface);
    wifi_handle handle = getWifiHandle(iface);
    pthread_t tid;

    LLCommand = LLStatsCommand::instance(handle);
    if (LLCommand == NULL) {
        ALOGE("%s: Error LLStatsCommand NULL", __func__);
        return WIFI_ERROR_UNKNOWN;
    }
    LLCommand->setSubCmd(QCA_NL80211_VENDOR_SUBCMD_LL_STATS_GET);

    LLCommand->initGetContext(id);

    LLStatsCallbackHandler callbackHandler =
    {
        .on_link_stats_results = handler.on_link_stats_results
    };

    /* create the message */
    ret = LLCommand->create();
    if (ret < 0)
        goto cleanup;

    ret = LLCommand->set_iface_id(iinfo->name);
    if (ret < 0)
        goto cleanup;
    /*add the attributes*/
    nl_data = LLCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nl_data)
        goto cleanup;
    ret = LLCommand->put_u32(QCA_WLAN_VENDOR_ATTR_LL_STATS_GET_CONFIG_REQ_ID,
                                  id);
    if (ret < 0)
        goto cleanup;
    ret = LLCommand->put_u32(QCA_WLAN_VENDOR_ATTR_LL_STATS_GET_CONFIG_REQ_MASK,
                                  7);
    if (ret < 0)
        goto cleanup;

    /**/
    LLCommand->attr_end(nl_data);

    ret = LLCommand->requestResponse();
    if (ret != 0) {
        ALOGE("%s: requestResponse Error:%d",__func__, ret);
    }
    if (ret < 0)
        goto cleanup;

    ret = LLCommand->setCallbackHandler(callbackHandler, QCA_NL80211_VENDOR_SUBCMD_LL_STATS_RADIO_RESULTS);
    if (ret < 0)
        goto cleanup;
    ret = LLCommand->setCallbackHandler(callbackHandler, QCA_NL80211_VENDOR_SUBCMD_LL_STATS_IFACE_RESULTS);
    if (ret < 0)
        goto cleanup;
    ret = LLCommand->setCallbackHandler(callbackHandler, QCA_NL80211_VENDOR_SUBCMD_LL_STATS_PEERS_RESULTS);
    if (ret < 0)
        goto cleanup;
cleanup:
    return (wifi_error)ret;
}


//Implementation of the functions exposed in LLStats.h
wifi_error wifi_clear_link_stats(wifi_interface_handle iface,
                                 u32 stats_clear_req_mask,
                                 u32 *stats_clear_rsp_mask,
                                 u8 stop_req, u8 *stop_rsp)
{
    int ret = 0;
    LLStatsCommand *LLCommand;
    struct nlattr *nl_data;
    interface_info *iinfo = getIfaceInfo(iface);
    wifi_handle handle = getWifiHandle(iface);

    LLCommand = LLStatsCommand::instance(handle);
    if (LLCommand == NULL) {
        ALOGE("%s: Error LLStatsCommand NULL", __func__);
        return WIFI_ERROR_UNKNOWN;
    }
    LLCommand->setSubCmd(QCA_NL80211_VENDOR_SUBCMD_LL_STATS_CLR);

    /* create the message */
    ret = LLCommand->create();
    if (ret < 0)
        goto cleanup;

    ret = LLCommand->set_iface_id(iinfo->name);
    if (ret < 0)
        goto cleanup;
    /*add the attributes*/
    nl_data = LLCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nl_data)
        goto cleanup;
    /**/
    ret = LLCommand->put_u32(QCA_WLAN_VENDOR_ATTR_LL_STATS_CLR_CONFIG_REQ_MASK,
                                  stats_clear_req_mask);
    if (ret < 0)
        goto cleanup;
    /**/
    ret = LLCommand->put_u8(QCA_WLAN_VENDOR_ATTR_LL_STATS_CLR_CONFIG_STOP_REQ,
                                   stop_req);
    if (ret < 0)
        goto cleanup;
    LLCommand->attr_end(nl_data);

    ret = LLCommand->requestResponse();
    if (ret != 0) {
        ALOGE("%s: requestResponse Error:%d",__func__, ret);
    }

    LLCommand->getClearRspParams(stats_clear_rsp_mask, stop_rsp);

cleanup:
    LLCommand->unregisterHandler(QCA_NL80211_VENDOR_SUBCMD_LL_STATS_RADIO_RESULTS);
    LLCommand->unregisterHandler(QCA_NL80211_VENDOR_SUBCMD_LL_STATS_IFACE_RESULTS);
    LLCommand->unregisterHandler(QCA_NL80211_VENDOR_SUBCMD_LL_STATS_PEERS_RESULTS);
    delete LLCommand;
    return (wifi_error)ret;
}

