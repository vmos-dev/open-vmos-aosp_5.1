#include "wifi_hal.h"

#ifndef __WIFI_HAL_STATS_H
#define __WIFI_HAL_STATS_H

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#define STATS_MAJOR_VERSION      1
#define STATS_MINOR_VERSION      0
#define STATS_MICRO_VERSION      0

typedef int wifi_radio;
typedef int wifi_channel;

/* channel operating width */
typedef enum {
   WIFI_CHAN_WIDTH_20    = 0,
   WIFI_CHAN_WIDTH_40    = 1,
   WIFI_CHAN_WIDTH_80    = 2,
   WIFI_CHAN_WIDTH_160   = 3,
   WIFI_CHAN_WIDTH_80P80 = 4,
   WIFI_CHAN_WIDTH_5     = 5,
   WIFI_CHAN_WIDTH_10    = 6,
   WIFI_CHAN_WIDTH_INVALID = -1
} wifi_channel_width;

typedef enum {
    WIFI_DISCONNECTED = 0,
    WIFI_AUTHENTICATING = 1,
    WIFI_ASSOCIATING = 2,
    WIFI_ASSOCIATED = 3,
    WIFI_EAPOL_STARTED = 4,   // if done by firmware/driver
    WIFI_EAPOL_COMPLETED = 5, // if done by firmware/driver
} wifi_connection_state;

typedef enum {
    WIFI_ROAMING_IDLE = 0,
    WIFI_ROAMING_ACTIVE = 1,
} wifi_roam_state;

typedef enum {
    WIFI_INTERFACE_STA = 0,
    WIFI_INTERFACE_SOFTAP = 1,
    WIFI_INTERFACE_IBSS = 2,
    WIFI_INTERFACE_P2P_CLIENT = 3,
    WIFI_INTERFACE_P2P_GO = 4,
    WIFI_INTERFACE_NAN = 5,
    WIFI_INTERFACE_MESH = 6,
    WIFI_INTERFACE_UNKNOWN = -1
 } wifi_interface_mode;

#define WIFI_CAPABILITY_QOS          0x00000001     // set for QOS association
#define WIFI_CAPABILITY_PROTECTED    0x00000002     // set for protected association (802.11 beacon frame control protected bit set)
#define WIFI_CAPABILITY_INTERWORKING 0x00000004     // set if 802.11 Extended Capabilities element interworking bit is set
#define WIFI_CAPABILITY_HS20         0x00000008     // set for HS20 association
#define WIFI_CAPABILITY_SSID_UTF8    0x00000010     // set is 802.11 Extended Capabilities element UTF-8 SSID bit is set
#define WIFI_CAPABILITY_COUNTRY      0x00000020     // set is 802.11 Country Element is present

typedef struct {
   wifi_interface_mode mode;     // interface mode
   u8 mac_addr[6];               // interface mac address (self)
   wifi_connection_state state;  // connection state (valid for STA, CLI only)
   wifi_roam_state roaming;      // roaming state
   u32 capabilities;             // WIFI_CAPABILITY_XXX (self)
   u8 ssid[33];                  // null terminated SSID
   u8 bssid[6];                  // bssid
   u8 ap_country_str[3];         // country string advertised by AP
   u8 country_str[3];            // country string for this association
} wifi_interface_link_layer_info;

/* channel information */
typedef struct {
   wifi_channel_width width;   // channel width (20, 40, 80, 80+80, 160)
   wifi_channel center_freq;   // primary 20 MHz channel
   wifi_channel center_freq0;  // center frequency (MHz) first segment
   wifi_channel center_freq1;  // center frequency (MHz) second segment
} wifi_channel_info;

/* wifi rate */
typedef struct {
   u32 preamble   :3;   // 0: OFDM, 1:CCK, 2:HT 3:VHT 4..7 reserved
   u32 nss        :2;   // 0:1x1, 1:2x2, 3:3x3, 4:4x4
   u32 bw         :3;   // 0:20MHz, 1:40Mhz, 2:80Mhz, 3:160Mhz
   u32 rateMcsIdx :8;   // OFDM/CCK rate code would be as per ieee std in the units of 0.5mbps
                        // HT/VHT it would be mcs index
   u32 reserved  :16;   // reserved
   u32 bitrate;         // units of 100 Kbps
} wifi_rate;

/* channel statistics */
typedef struct {
   wifi_channel_info channel;  // channel
   u32 on_time;                // msecs the radio is awake (32 bits number accruing over time)
   u32 cca_busy_time;          // msecs the CCA register is busy (32 bits number accruing over time)
} wifi_channel_stat;

/* radio statistics */
typedef struct {
   wifi_radio radio;               // wifi radio (if multiple radio supported)
   u32 on_time;                    // msecs the radio is awake (32 bits number accruing over time)
   u32 tx_time;                    // msecs the radio is transmitting (32 bits number accruing over time)
   u32 rx_time;                    // msecs the radio is in active receive (32 bits number accruing over time)
   u32 on_time_scan;               // msecs the radio is awake due to all scan (32 bits number accruing over time)
   u32 on_time_nbd;                // msecs the radio is awake due to NAN (32 bits number accruing over time)
   u32 on_time_gscan;              // msecs the radio is awake due to G?scan (32 bits number accruing over time)
   u32 on_time_roam_scan;          // msecs the radio is awake due to roam?scan (32 bits number accruing over time)
   u32 on_time_pno_scan;           // msecs the radio is awake due to PNO scan (32 bits number accruing over time)
   u32 on_time_hs20;               // msecs the radio is awake due to HS2.0 scans and GAS exchange (32 bits number accruing over time)
   u32 num_channels;               // number of channels
   wifi_channel_stat channels[];   // channel statistics
} wifi_radio_stat;

/* per rate statistics */
typedef struct {
   wifi_rate rate;     // rate information
   u32 tx_mpdu;        // number of successfully transmitted data pkts (ACK rcvd)
   u32 rx_mpdu;        // number of received data pkts
   u32 mpdu_lost;      // number of data packet losses (no ACK)
   u32 retries;        // total number of data pkt retries
   u32 retries_short;  // number of short data pkt retries
   u32 retries_long;   // number of long data pkt retries
} wifi_rate_stat;

/* access categories */
typedef enum {
   WIFI_AC_VO  = 0,
   WIFI_AC_VI  = 1,
   WIFI_AC_BE  = 2,
   WIFI_AC_BK  = 3,
   WIFI_AC_MAX = 4,
} wifi_traffic_ac;

/* wifi peer type */
typedef enum
{
   WIFI_PEER_STA,
   WIFI_PEER_AP,
   WIFI_PEER_P2P_GO,
   WIFI_PEER_P2P_CLIENT,
   WIFI_PEER_NAN,
   WIFI_PEER_TDLS,
   WIFI_PEER_INVALID,
} wifi_peer_type;

/* per peer statistics */
typedef struct {
   wifi_peer_type type;           // peer type (AP, TDLS, GO etc.)
   u8 peer_mac_address[6];        // mac address
   u32 capabilities;              // peer WIFI_CAPABILITY_XXX
   u32 num_rate;                  // number of rates
   wifi_rate_stat rate_stats[];   // per rate statistics, number of entries  = num_rate
} wifi_peer_info;

/* per access category statistics */
typedef struct {
   wifi_traffic_ac ac;             // access category (VI, VO, BE, BK)
   u32 tx_mpdu;                    // number of successfully transmitted unicast data pkts (ACK rcvd)
   u32 rx_mpdu;                    // number of received unicast mpdus
   u32 tx_mcast;                   // number of succesfully transmitted multicast data packets
                                   // STA case: implies ACK received from AP for the unicast packet in which mcast pkt was sent
   u32 rx_mcast;                   // number of received multicast data packets
   u32 rx_ampdu;                   // number of received unicast a-mpdus
   u32 tx_ampdu;                   // number of transmitted unicast a-mpdus
   u32 mpdu_lost;                  // number of data pkt losses (no ACK)
   u32 retries;                    // total number of data pkt retries
   u32 retries_short;              // number of short data pkt retries
   u32 retries_long;               // number of long data pkt retries
   u32 contention_time_min;        // data pkt min contention time (usecs)
   u32 contention_time_max;        // data pkt max contention time (usecs)
   u32 contention_time_avg;        // data pkt avg contention time (usecs)
   u32 contention_num_samples;     // num of data pkts used for contention statistics
} wifi_wmm_ac_stat;

/* interface statistics */
typedef struct {
   wifi_interface_handle iface;          // wifi interface
   wifi_interface_link_layer_info info;  // current state of the interface
   u32 beacon_rx;                        // access point beacon received count from connected AP
   u32 mgmt_rx;                          // access point mgmt frames received count from connected AP (including Beacon)
   u32 mgmt_action_rx;                   // action frames received count
   u32 mgmt_action_tx;                   // action frames transmit count
   wifi_rssi rssi_mgmt;                  // access Point Beacon and Management frames RSSI (averaged)
   wifi_rssi rssi_data;                  // access Point Data Frames RSSI (averaged) from connected AP
   wifi_rssi rssi_ack;                   // access Point ACK RSSI (averaged) from connected AP
   wifi_wmm_ac_stat ac[WIFI_AC_MAX];     // per ac data packet statistics
   u32 num_peers;                        // number of peers
   wifi_peer_info peer_info[];           // per peer statistics
} wifi_iface_stat;

/* configuration params */
typedef struct {
   u32 mpdu_size_threshold;             // threshold to classify the pkts as short or long
                                        // packet size < mpdu_size_threshold => short
   u32 aggressive_statistics_gathering; // set for field debug mode. Driver should collect all statistics regardless of performance impact.
} wifi_link_layer_params;

/* API to trigger the link layer statistics collection.
   Unless his API is invoked - link layer statistics will not be collected.
   Radio statistics (once started) do not stop or get reset unless wifi_clear_link_stats is invoked
   Interface statistics (once started) reset and start afresh after each connection */
wifi_error wifi_set_link_stats(wifi_interface_handle iface, wifi_link_layer_params params);

/* callback for reporting link layer stats */
typedef struct {
  void (*on_link_stats_results) (wifi_request_id id, wifi_iface_stat *iface_stat,
         int num_radios, wifi_radio_stat *radio_stat);
} wifi_stats_result_handler;

/* api to collect the link layer statistics for a given iface and all the radio stats */
wifi_error wifi_get_link_stats(wifi_request_id id,
        wifi_interface_handle iface, wifi_stats_result_handler handler);

/* wifi statistics bitmap  */
#define WIFI_STATS_RADIO              0x00000001      // all radio statistics
#define WIFI_STATS_RADIO_CCA          0x00000002      // cca_busy_time (within radio statistics)
#define WIFI_STATS_RADIO_CHANNELS     0x00000004      // all channel statistics (within radio statistics)
#define WIFI_STATS_RADIO_SCAN         0x00000008      // all scan statistics (within radio statistics)
#define WIFI_STATS_IFACE              0x00000010      // all interface statistics
#define WIFI_STATS_IFACE_TXRATE       0x00000020      // all tx rate statistics (within interface statistics)
#define WIFI_STATS_IFACE_AC           0x00000040      // all ac statistics (within interface statistics)
#define WIFI_STATS_IFACE_CONTENTION   0x00000080      // all contention (min, max, avg) statistics (within ac statisctics)

/* clear api to reset statistics, stats_clear_rsp_mask identifies what stats have been cleared
   stop_req = 1 will imply whether to stop the statistics collection.
   stop_rsp = 1 will imply that stop_req was honored and statistics collection was stopped.
 */
wifi_error wifi_clear_link_stats(wifi_interface_handle iface,
      u32 stats_clear_req_mask, u32 *stats_clear_rsp_mask, u8 stop_req, u8 *stop_rsp);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*__WIFI_HAL_STATS_ */

