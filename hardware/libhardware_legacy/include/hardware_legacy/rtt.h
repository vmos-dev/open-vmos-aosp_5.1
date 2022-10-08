
#include "wifi_hal.h"

#ifndef __WIFI_HAL_RTT_H__
#define __WIFI_HAL_RTT_H__

/* channel operating width */

/* Ranging status */
typedef enum {
    RTT_STATUS_SUCCESS,
    RTT_STATUS_FAILURE,
    RTT_STATUS_FAIL_NO_RSP,
    RTT_STATUS_FAIL_REJECTED,
    RTT_STATUS_FAIL_NOT_SCHEDULED_YET,
    RTT_STATUS_FAIL_TM_TIMEOUT,
    RTT_STATUS_FAIL_AP_ON_DIFF_CHANNEL,
    RTT_STATUS_FAIL_NO_CAPABILITY,
    RTT_STATUS_ABORTED
} wifi_rtt_status;


/* RTT Type */
typedef enum {
    RTT_TYPE_INVALID,
    RTT_TYPE_1_SIDED,
    RTT_TYPE_2_SIDED,
    RTT_TYPE_AUTO,              // Two sided if remote supports; one sided otherwise
} wifi_rtt_type;

/* RTT configuration */
typedef struct {
    mac_addr addr;                     // peer device mac address
    wifi_rtt_type type;                // optional - rtt type hint. RTT_TYPE_INVALID implies best effort
    wifi_peer_type peer;               // optional - peer device hint (STA, P2P, AP)
    wifi_channel_info channel;         // Required for STA-AP mode, optional for P2P, NBD etc.
    byte continuous;                   // 0 = single shot or 1 = continuous ranging
    unsigned interval;                 // interval of RTT measurement (unit ms) when continuous = true
    unsigned num_measurements;         // total number of RTT measurements when continuous = true
    unsigned num_samples_per_measurement; // num of packets in each RTT measurement
    unsigned num_retries_per_measurement; // num of retries if sampling fails
} wifi_rtt_config;

/* RTT results */
typedef struct {
    mac_addr addr;               // device mac address
    unsigned measurement_num;    // measurement number in case of continuous ranging
    wifi_rtt_status status;      // ranging status
    wifi_rtt_type type;          // RTT type
    wifi_peer_type peer;         // peer device type (P2P, AP)
    wifi_channel_info channel;   // channel information
    wifi_rssi rssi;              // rssi in 0.5 dB steps e.g. 143 implies -71.5 dB
    wifi_rssi rssi_spread;       // rssi spread in 0.5 dB steps e.g. 5 implies 2.5 dB spread (optional)
    wifi_rate tx_rate;           // TX rate
    wifi_timespan rtt;           // round trip time in nanoseconds
    wifi_timespan rtt_sd;        // rtt standard deviation in nanoseconds
    wifi_timespan rtt_spread;    // difference between max and min rtt times recorded
    int distance;                // distance in cm (optional)
    int distance_sd;             // standard deviation in cm (optional)
    int distance_spread;         // difference between max and min distance recorded (optional)
    wifi_timestamp ts;           // time of the measurement (in microseconds since boot)
} wifi_rtt_result;

/* RTT result callback */
typedef struct {
    void (*on_rtt_results) (wifi_request_id id, unsigned num_results, wifi_rtt_result rtt_result[]);
} wifi_rtt_event_handler;

/* API to request RTT measurement */
wifi_error wifi_rtt_range_request(wifi_request_id id, wifi_interface_handle iface,
        unsigned num_rtt_config, wifi_rtt_config rtt_config[], wifi_rtt_event_handler handler);

/* API to cancel RTT measurements */
wifi_error wifi_rtt_range_cancel(wifi_request_id id,  wifi_interface_handle iface,
        unsigned num_devices, mac_addr addr[]);

/* NBD ranging channel map */
typedef struct {
    wifi_channel availablity[32];           // specifies the channel map for each of the 16 TU windows
                                            // frequency of 0 => unspecified; which means firmware is
                                            // free to do whatever it wants in this window.
} wifi_channel_map;

/* API to start publishing the channel map on responder device in a NBD cluster.
   Responder device will take this request and schedule broadcasting the channel map
   in a NBD ranging attribute in a SDF. DE will automatically remove the ranging
   attribute from the OTA queue after number of DW specified by num_dw
   where Each DW is 512 TUs apart */
wifi_error wifi_rtt_channel_map_set(wifi_request_id id,
        wifi_interface_handle iface, wifi_channel_map *params, unsigned num_dw);

/* API to clear the channel map on the responder device in a NBD cluster.
   Responder device will cancel future ranging channel request, starting from next
   DW interval and will also stop broadcasting NBD ranging attribute in SDF */
wifi_error wifi_rtt_channel_map_clear(wifi_request_id id,  wifi_interface_handle iface);

/* RTT Capabilities */
typedef struct {
   byte rtt_one_sided_supported;  // if 1-sided rtt data collection is supported
   byte rtt_11v_supported;        // if 11v rtt data collection is supported
   byte rtt_ftm_supported;        // if ftm rtt data collection is supported
} wifi_rtt_capabilities;

/*  RTT capabilities of the device */
wifi_error wifi_get_rtt_capabilities(wifi_interface_handle iface, wifi_rtt_capabilities *capabilities);

#endif

