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

#ifndef __VENDOR_DEFINITIONS_H__
#define __VENDOR_DEFINITIONS_H__

/*Internal to Android HAL component */
enum vendor_subcmds {
    /* subcommands for link layer statistics start here */
    QCA_NL80211_VENDOR_SUBCMD_LL_STATS_SET = 14,
    QCA_NL80211_VENDOR_SUBCMD_LL_STATS_GET = 15,
    QCA_NL80211_VENDOR_SUBCMD_LL_STATS_CLR = 16,
    QCA_NL80211_VENDOR_SUBCMD_LL_STATS_RADIO_RESULTS = 17,
    QCA_NL80211_VENDOR_SUBCMD_LL_STATS_IFACE_RESULTS = 18,
    QCA_NL80211_VENDOR_SUBCMD_LL_STATS_PEERS_RESULTS = 19,
    /* subcommands for gscan start here */
    QCA_NL80211_VENDOR_SUBCMD_GSCAN_START = 20,
    QCA_NL80211_VENDOR_SUBCMD_GSCAN_STOP = 21,
    QCA_NL80211_VENDOR_SUBCMD_GSCAN_GET_VALID_CHANNELS = 22,
    QCA_NL80211_VENDOR_SUBCMD_GSCAN_GET_CAPABILITIES = 23,
    QCA_NL80211_VENDOR_SUBCMD_GSCAN_GET_CACHED_RESULTS = 24,
    /* Used when report_threshold is reached in scan cache. */
    QCA_NL80211_VENDOR_SUBCMD_GSCAN_SCAN_RESULTS_AVAILABLE = 25,
    /* Used to report scan results when each probe rsp. is received,
     * if report_events enabled in wifi_scan_cmd_params.
     */
    QCA_NL80211_VENDOR_SUBCMD_GSCAN_FULL_SCAN_RESULT = 26,
    /* Indicates progress of scanning state-machine. */
    QCA_NL80211_VENDOR_SUBCMD_GSCAN_SCAN_EVENT = 27,
    /* Indicates BSSID Hotlist. */
    QCA_NL80211_VENDOR_SUBCMD_GSCAN_HOTLIST_AP_FOUND = 28,
    QCA_NL80211_VENDOR_SUBCMD_GSCAN_SET_BSSID_HOTLIST = 29,
    QCA_NL80211_VENDOR_SUBCMD_GSCAN_RESET_BSSID_HOTLIST = 30,
    QCA_NL80211_VENDOR_SUBCMD_GSCAN_SIGNIFICANT_CHANGE = 31,
    QCA_NL80211_VENDOR_SUBCMD_GSCAN_SET_SIGNIFICANT_CHANGE = 32,
    QCA_NL80211_VENDOR_SUBCMD_GSCAN_RESET_SIGNIFICANT_CHANGE = 33,
};

enum qca_wlan_vendor_attr_ll_stats_set
{
    QCA_WLAN_VENDOR_ATTR_LL_STATS_SET_INVALID = 0,
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_SET_CONFIG_MPDU_SIZE_THRESHOLD = 1,
    QCA_WLAN_VENDOR_ATTR_LL_STATS_SET_CONFIG_AGGRESSIVE_STATS_GATHERING,
    /* keep last */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_SET_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_LL_STATS_SET_MAX =
        QCA_WLAN_VENDOR_ATTR_LL_STATS_SET_AFTER_LAST - 1,
};

enum qca_wlan_vendor_attr_ll_stats_clr
{
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CLR_INVALID = 0,
    /* Unsigned 32bit bitmap  for clearing statistics
     * All radio statistics                     0x00000001
     * cca_busy_time (within radio statistics)  0x00000002
     * All channel stats (within radio statistics) 0x00000004
     * All scan statistics (within radio statistics) 0x00000008
     * All interface statistics                     0x00000010
     * All tx rate statistics (within interface statistics) 0x00000020
     * All ac statistics (with in interface statistics) 0x00000040
     * All contention (min, max, avg) statistics (within ac statisctics)
     * 0x00000080.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CLR_CONFIG_REQ_MASK,
    /* Unsigned 8bit value : Request to stop statistics collection */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CLR_CONFIG_STOP_REQ,

    /* Unsigned 32bit bitmap : Response from the driver
     * for the cleared statistics
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CLR_CONFIG_RSP_MASK,
    /* Unsigned 8bit value: Response from driver/firmware
     * for the stop request
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CLR_CONFIG_STOP_RSP,
    /* keep last */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CLR_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CLR_MAX =
        QCA_WLAN_VENDOR_ATTR_LL_STATS_CLR_AFTER_LAST - 1,
};

enum qca_wlan_vendor_attr_ll_stats_get
{
    QCA_WLAN_VENDOR_ATTR_LL_STATS_GET_INVALID = 0,
    /* Unsigned 32bit value provided by the caller issuing the GET stats
     * command. When reporting the stats results, the driver uses the same
     * value to indicate which GET request the results correspond to.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_GET_CONFIG_REQ_ID,
    /* Unsigned 32bit value - bit mask to identify what statistics are
     * requested for retrieval.
     * Radio Statistics 0x00000001
     * Interface Statistics 0x00000020
     * All Peer Statistics 0x00000040
     * Peer Statistics     0x00000080
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_GET_CONFIG_REQ_MASK,
    /* keep last */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_GET_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_LL_STATS_GET_MAX =
        QCA_WLAN_VENDOR_ATTR_LL_STATS_GET_AFTER_LAST - 1,
};

enum qca_wlan_vendor_attr_ll_stats_results
{
    QCA_WLAN_VENDOR_ATTR_LL_STATS_INVALID = 0,
    /* Unsigned 32bit value. Used by the driver; must match the request id
     * provided with the QCA_NL80211_VENDOR_SUBCMD_LL_STATS_GET command.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RESULTS_REQ_ID,

    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_BEACON_RX,
    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_MGMT_RX,
    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_MGMT_ACTION_RX,
    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_MGMT_ACTION_TX,
    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_RSSI_MGMT,
    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_RSSI_DATA,
    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_RSSI_ACK,

    /* Attributes of type QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_* are
     * nested within the interface stats.
     */

    /* Interface mode, e.g., STA, SOFTAP, IBSS, etc.
     * Type = enum wifi_interface_mode.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_MODE,
    /* Interface MAC address. An array of 6 Unsigned int8 */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_MAC_ADDR,
    /* Type = enum wifi_connection_state, e.g., DISCONNECTED,
     * AUTHENTICATING, etc. valid for STA, CLI only.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_STATE,
    /* Type = enum wifi_roam_state. Roaming state, e.g., IDLE or ACTIVE
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_ROAMING,
    /* Unsigned 32bit value. WIFI_CAPABILITY_XXX */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_CAPABILITIES,
    /* NULL terminated SSID. An array of 33 Unsigned 8bit values */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_SSID,
    /* BSSID. An array of 6 Unsigned 8bit values */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_BSSID,
    /* Country string advertised by AP. An array of 3 Unsigned 8bit
     * values.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_AP_COUNTRY_STR,
    /* Country string for this association. An array of 3 Unsigned 8bit
     * values.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_COUNTRY_STR,

    /* Attributes of type QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_* could
     * be nested within the interface stats.
     */

    /* Type = enum wifi_traffic_ac, e.g., V0, VI, BE and BK */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_AC,
    /* Unsigned int 32 value corresponding to respective AC */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_TX_MPDU,
    /* Unsigned int 32 value corresponding to respective AC */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_RX_MPDU,
    /* Unsigned int 32 value corresponding to respective AC */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_TX_MCAST,
    /* Unsigned int 32 value corresponding to respective AC */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_RX_MCAST,
    /* Unsigned int 32 value corresponding to respective AC */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_RX_AMPDU,
    /* Unsigned int 32 value corresponding to respective AC */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_TX_AMPDU,
    /* Unsigned int 32 value corresponding to respective AC */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_MPDU_LOST,
    /* Unsigned int 32 value corresponding to respective AC */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_RETRIES,
    /* Unsigned int 32 value corresponding to respective AC  */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_RETRIES_SHORT,
    /* Unsigned int 32 values corresponding to respective AC */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_RETRIES_LONG,
    /* Unsigned int 32 values corresponding to respective AC */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_CONTENTION_TIME_MIN,
    /* Unsigned int 32 values corresponding to respective AC */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_CONTENTION_TIME_MAX,
    /* Unsigned int 32 values corresponding to respective AC */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_CONTENTION_TIME_AVG,
    /* Unsigned int 32 values corresponding to respective AC */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_CONTENTION_NUM_SAMPLES,
    /* Unsigned 32bit value. Number of peers */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_NUM_PEERS,

    /* Attributes of type QCA_WLAN_VENDOR_ATTR_LL_STATS_PEER_INFO_* are
     * nested within the interface stats.
     */

    /* Type = enum wifi_peer_type. Peer type, e.g., STA, AP, P2P GO etc. */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_PEER_INFO_TYPE,
    /* MAC addr corresponding to respective peer. An array of 6 Unsigned
     * 8bit values.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_PEER_INFO_MAC_ADDRESS,
    /* Unsigned int 32bit value representing capabilities corresponding
     * to respective peer.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_PEER_INFO_CAPABILITIES,
    /* Unsigned 32bit value. Number of rates */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_PEER_INFO_NUM_RATES,

    /* Attributes of type QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_*
     * are nested within the rate stat.
     */

    /* Wi-Fi Rate - separate attributes defined for individual fields */

    /* Unsigned int 8bit value; 0: OFDM, 1:CCK, 2:HT 3:VHT 4..7 reserved */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_PREAMBLE,
    /* Unsigned int 8bit value; 0:1x1, 1:2x2, 3:3x3, 4:4x4 */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_NSS,
    /* Unsigned int 8bit value; 0:20MHz, 1:40Mhz, 2:80Mhz, 3:160Mhz */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_BW,
    /* Unsigned int 8bit value; OFDM/CCK rate code would be as per IEEE Std
     * in the units of 0.5mbps HT/VHT it would be mcs index */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_MCS_INDEX,

    /* Unsigned 32bit value. Bit rate in units of 100Kbps */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_BIT_RATE,


    /* Attributes of type QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_STAT_* could be
     * nested within the peer info stats.
     */

    /* Unsigned int 32bit value. Number of successfully transmitted data pkts,
     * i.e., with ACK received corresponding to the respective rate.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_TX_MPDU,
    /* Unsigned int 32bit value. Number of received data pkts corresponding
     *  to the respective rate.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_RX_MPDU,
    /* Unsigned int 32bit value. Number of data pkts losses, i.e., no ACK
     * received corresponding to *the respective rate.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_MPDU_LOST,
    /* Unsigned int 32bit value. Total number of data pkt retries for the
     * respective rate.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_RETRIES,
    /* Unsigned int 32bit value. Total number of short data pkt retries
     * for the respective rate.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_RETRIES_SHORT,
    /* Unsigned int 32bit value. Total number of long data pkt retries
     * for the respective rate.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_RETRIES_LONG,


    QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_ID,
    /* Unsigned 32bit value. Total number of msecs the radio is awake
     * accruing over time.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_ON_TIME,
    /* Unsigned 32bit value. Total number of msecs the radio is transmitting
     * accruing over time.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_TX_TIME,
    /* Unsigned 32bit value. Total number of msecs the radio is in active
     * receive accruing over time.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_RX_TIME,
    /* Unsigned 32bit value. Total number of msecs the radio is awake due
     * to all scan accruing over time.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_ON_TIME_SCAN,
    /* Unsigned 32bit value. Total number of msecs the radio is awake due
     * to NAN accruing over time.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_ON_TIME_NBD,
    /* Unsigned 32bit value. Total number of msecs the radio is awake due
     * to GSCAN accruing over time.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_ON_TIME_GSCAN,
    /* Unsigned 32bit value. Total number of msecs the radio is awake due
     * to roam scan accruing over time.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_ON_TIME_ROAM_SCAN,
    /* Unsigned 32bit value. Total number of msecs the radio is awake due
     * to PNO scan accruing over time.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_ON_TIME_PNO_SCAN,
    /* Unsigned 32bit value. Total number of msecs the radio is awake due
     * to HS2.0 scans and GAS *exchange accruing over time.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_ON_TIME_HS20,
    /* Unsigned 32bit value. Number of channels. */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_NUM_CHANNELS,

    /* Attributes of type QCA_WLAN_VENDOR_ATTR_LL_STATS_CHANNEL_INFO_* could
     * be nested within the channel stats.
     */

    /* Type = enum wifi_channel_width. Channel width, e.g., 20, 40, 80 */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CHANNEL_INFO_WIDTH,
    /* Unsigned 32bit value. Primary 20MHz channel. */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CHANNEL_INFO_CENTER_FREQ,
    /* Unsigned 32bit value. Center frequency (MHz) first segment. */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CHANNEL_INFO_CENTER_FREQ0,
    /* Unsigned 32bit value. Center frequency (MHz) second segment. */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CHANNEL_INFO_CENTER_FREQ1,

    /* Attributes of type QCA_WLAN_VENDOR_ATTR_LL_STATS_CHANNEL_* could be
     * nested within the radio stats.
     */

    /* Unsigned int 32bit value representing total number of msecs the radio
     * is awake on that *channel accruing over time, corresponding to the
     * respective channel.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CHANNEL_ON_TIME,
    /* Unsigned int 32bit value representing total number of msecs the CCA
     * register is busy accruing  *over time corresponding to the respective
     * channel.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CHANNEL_CCA_BUSY_TIME,

    QCA_WLAN_VENDOR_ATTR_LL_STATS_NUM_RADIOS,

    /* Signifies the nested list of channel attributes
     * QCA_WLAN_VENDOR_ATTR_LL_STATS_CHANNEL_INFO_*
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CH_INFO,

    /* Signifies the nested list of peer info attributes
     * QCA_WLAN_VENDOR_ATTR_LL_STATS_PEER_INFO_*
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_PEER_INFO,

    /* Signifies the nested list of rate info attributes
     * QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_*
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_PEER_INFO_RATE_INFO,

    /* Signifies the nested list of wmm info attributes
     * QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_*
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_INFO,

    /* Unsigned 8bit value. Used by the driver; if set to 1, it indicates that
     * more stats, e.g., peers or radio, are to follow in the next
     * QCA_NL80211_VENDOR_SUBCMD_LL_STATS_*_RESULTS event.
     * Otherwise, it is set to 0.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RESULTS_MORE_DATA,

    /* keep last */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_LL_STATS_MAX = QCA_WLAN_VENDOR_ATTR_LL_STATS_AFTER_LAST - 1,
};

enum qca_wlan_vendor_attr_gscan_config_params
{
    QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_INVALID = 0,

    /* Unsigned 32-bit value; Middleware provides it to the driver. Middle ware
     * either gets it from caller, e.g., framework, or generates one if
     * framework doesn't provide it.
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID,

    /* NL attributes for data used by
     * QCA_NL80211_VENDOR_SUBCMD_GSCAN_GET_VALID_CHANNELS sub command.
     */
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_GET_VALID_CHANNELS_CONFIG_PARAM_WIFI_BAND,
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_GET_VALID_CHANNELS_CONFIG_PARAM_MAX_CHANNELS,

    /* NL attributes for input params used by
     * QCA_NL80211_VENDOR_SUBCMD_GSCAN_START sub command.
     */

    /* Unsigned 32-bit value; channel frequency */
    QCA_WLAN_VENDOR_ATTR_GSCAN_CHANNEL_SPEC_CHANNEL,
    /* Unsigned 32-bit value; dwell time in ms. */
    QCA_WLAN_VENDOR_ATTR_GSCAN_CHANNEL_SPEC_DWELL_TIME,
    /* Unsigned 8-bit value; 0: active; 1: passive; N/A for DFS */
    QCA_WLAN_VENDOR_ATTR_GSCAN_CHANNEL_SPEC_PASSIVE,
    /* Unsigned 8-bit value; channel class */
    QCA_WLAN_VENDOR_ATTR_GSCAN_CHANNEL_SPEC_CLASS,

    /* Unsigned 8-bit value; bucket index, 0 based */
    QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_INDEX,
    /* Unsigned 8-bit value; band. */
    QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_BAND,
    /* Unsigned 32-bit value; desired period, in ms. */
    QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_PERIOD,
    /* Unsigned 8-bit value; report events semantics. */
    QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_REPORT_EVENTS,
    /* Unsigned 32-bit value. Followed by a nested array of GSCAN_CHANNEL_SPEC_*
     * attributes.
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_NUM_CHANNEL_SPECS,

    /* Array of QCA_WLAN_VENDOR_ATTR_GSCAN_CHANNEL_SPEC_* attributes.
     * Array size: QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_NUM_CHANNEL_SPECS
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_CHANNEL_SPEC,

    /* Unsigned 32-bit value; base timer period in ms. */
    QCA_WLAN_VENDOR_ATTR_GSCAN_SCAN_CMD_PARAMS_BASE_PERIOD,
    /* Unsigned 32-bit value; number of APs to store in each scan in the
     * BSSID/RSSI history buffer (keep the highest RSSI APs).
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_SCAN_CMD_PARAMS_MAX_AP_PER_SCAN,
    /* Unsigned 8-bit value; In %, when scan buffer is this much full, wake up
     * APPS.
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_SCAN_CMD_PARAMS_REPORT_THRESHOLD,
    /* Unsigned 8-bit value; number of scan bucket specs; followed by a nested
     * array of_GSCAN_BUCKET_SPEC_* attributes and values. The size of the
     * array is determined by NUM_BUCKETS.
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_SCAN_CMD_PARAMS_NUM_BUCKETS,

    /* Array of QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_* attributes.
     * Array size: QCA_WLAN_VENDOR_ATTR_GSCAN_SCAN_CMD_PARAMS_NUM_BUCKETS
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC,

    /* Unsigned 8-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_GET_CACHED_SCAN_RESULTS_CONFIG_PARAM_FLUSH,
    /* Unsigned 32-bit value; maximum number of results to be returned. */
    QCA_WLAN_VENDOR_ATTR_GSCAN_GET_CACHED_SCAN_RESULTS_CONFIG_PARAM_MAX,

    /* An array of 6 x Unsigned 8-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_AP_THRESHOLD_PARAM_BSSID,
    /* Signed 32-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_AP_THRESHOLD_PARAM_RSSI_LOW,
    /* Signed 32-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_AP_THRESHOLD_PARAM_RSSI_HIGH,
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_AP_THRESHOLD_PARAM_CHANNEL,


    /* Number of hotlist APs as unsigned 32-bit value, followed by a nested array of
     * AP_THRESHOLD_PARAM attributes and values. The size of the array is
     * determined by NUM_AP.
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_BSSID_HOTLIST_PARAMS_NUM_AP,

    /* Array of QCA_WLAN_VENDOR_ATTR_GSCAN_AP_THRESHOLD_PARAM_* attributes.
     * Array size: QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_NUM_CHANNEL_SPECS
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_AP_THRESHOLD_PARAM,

    /* Unsigned 32bit value; number of samples for averaging RSSI. */
    QCA_WLAN_VENDOR_ATTR_GSCAN_SIGNIFICANT_CHANGE_PARAMS_RSSI_SAMPLE_SIZE,
    /* Unsigned 32bit value; number of samples to confirm AP loss. */
    QCA_WLAN_VENDOR_ATTR_GSCAN_SIGNIFICANT_CHANGE_PARAMS_LOST_AP_SAMPLE_SIZE,
    /* Unsigned 32bit value; number of APs breaching threshold. */
    QCA_WLAN_VENDOR_ATTR_GSCAN_SIGNIFICANT_CHANGE_PARAMS_MIN_BREACHING,
    /* Unsigned 32bit value; number of APs. Followed by an array of
     * AP_THRESHOLD_PARAM attributes. Size of the array is NUM_AP.
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_SIGNIFICANT_CHANGE_PARAMS_NUM_AP,

    /* keep last */
    QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_MAX =
        QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_AFTER_LAST - 1,

};

enum qca_wlan_vendor_attr_gscan_results
{
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_INVALID = 0,

    /* Unsigned 32-bit value; must match the request Id supplied by Wi-Fi HAL
     * in the corresponding subcmd NL msg
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_REQUEST_ID,

    /* Unsigned 32-bit value; used to indicate the status response from
     * firmware/driver for the vendor sub-command.
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_STATUS,

    /* GSCAN Valid Channels attributes */
    /* Unsigned 32bit value; followed by a nested array of CHANNELS.
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_NUM_CHANNELS,
    /* An array of NUM_CHANNELS x Unsigned 32bit value integers representing
     * channel numbers
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CHANNELS,

    /* GSCAN Capabilities attributes */
    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_SCAN_CACHE_SIZE,
    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_SCAN_BUCKETS,
    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_AP_CACHE_PER_SCAN,
    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_RSSI_SAMPLE_SIZE,
    /* Signed 32bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_SCAN_REPORTING_THRESHOLD,
    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_HOTLIST_APS,
    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_SIGNIFICANT_WIFI_CHANGE_APS,
    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_BSSID_HISTORY_ENTRIES,

    /* GSCAN Attributes used with
     * QCA_NL80211_VENDOR_SUBCMD_GSCAN_SCAN_RESULTS_AVAILABLE sub-command.
     */

    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_NUM_RESULTS_AVAILABLE,


    /* GSCAN attributes used with
     * QCA_NL80211_VENDOR_SUBCMD_GSCAN_FULL_SCAN_RESULT sub-command.
     */

    /* An array of NUM_RESULTS_AVAILABLE x
     * QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_*
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_LIST,

    /* Unsigned 64-bit value; age of sample at the time of retrieval */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_TIME_STAMP,
    /* 33 x unsiged 8-bit value; NULL terminated SSID */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_SSID,
    /* An array of 6 x Unsigned 8-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_BSSID,
    /* Unsigned 32-bit value; channel frequency in MHz */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_CHANNEL,
    /* Signed 32-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_RSSI,
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_RTT,
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_RTT_SD,
    /* Unsigned 16-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_BEACON_PERIOD,
    /* Unsigned 16-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_CAPABILITY,
    /* Unsigned 32-bit value; size of the IE DATA blob */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_IE_LENGTH,
    /* An array of IE_LENGTH x Unsigned 8-bit value; blob of all the
     * information elements found in the beacon; this data should be a
     * packed list of wifi_information_element objects, one after the
     * other.
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_IE_DATA,

    /* Unsigned 8-bit value; set by driver to indicate more scan results are
     * available.
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_MORE_DATA,

    /* GSCAN attributes for
     * QCA_NL80211_VENDOR_SUBCMD_GSCAN_SCAN_EVENT sub-command.
     */
    /* Unsigned 8-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_EVENT_TYPE,
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_EVENT_STATUS,

    /* GSCAN attributes for
     * QCA_NL80211_VENDOR_SUBCMD_GSCAN_HOTLIST_AP_FOUND sub-command.
     */
    /* Use attr QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_NUM_RESULTS_AVAILABLE
     * to indicate number of results.
     * Also, use QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_LIST to indicate the list
     * of results.
     */

    /* GSCAN attributes for
     * QCA_NL80211_VENDOR_SUBCMD_GSCAN_SIGNIFICANT_CHANGE sub-command.
     */
    /* An array of 6 x Unsigned 8-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SIGNIFICANT_CHANGE_RESULT_BSSID,
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SIGNIFICANT_CHANGE_RESULT_CHANNEL,
    /* Unsigned 32-bit value.
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SIGNIFICANT_CHANGE_RESULT_NUM_RSSI,
    /* A nested array of signed 32-bit RSSI values. Size of the array is determined by
     * (NUM_RSSI of SIGNIFICANT_CHANGE_RESULT_NUM_RSSI.
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SIGNIFICANT_CHANGE_RESULT_RSSI_LIST,

    /* keep last */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_MAX =
        QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_AFTER_LAST - 1,
};

#endif
