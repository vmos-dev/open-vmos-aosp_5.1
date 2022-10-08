/* Copyright (c) 2013, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef GPS_EXTENDED_C_H
#define GPS_EXTENDED_C_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <ctype.h>
#include <stdbool.h>
#include <hardware/gps.h>

/** Location has valid source information. */
#define LOCATION_HAS_SOURCE_INFO   0x0020
/** GpsLocation has valid "is indoor?" flag */
#define GPS_LOCATION_HAS_IS_INDOOR   0x0040
/** GpsLocation has valid floor number */
#define GPS_LOCATION_HAS_FLOOR_NUMBER   0x0080
/** GpsLocation has valid map URL*/
#define GPS_LOCATION_HAS_MAP_URL   0x0100
/** GpsLocation has valid map index */
#define GPS_LOCATION_HAS_MAP_INDEX   0x0200

/** Sizes for indoor fields */
#define GPS_LOCATION_MAP_URL_SIZE 400
#define GPS_LOCATION_MAP_INDEX_SIZE 16

/** Position source is ULP */
#define ULP_LOCATION_IS_FROM_HYBRID   0x0001
/** Position source is GNSS only */
#define ULP_LOCATION_IS_FROM_GNSS   0x0002

#define ULP_MIN_INTERVAL_INVALID 0xffffffff


typedef struct {
    /** set to sizeof(UlpLocation) */
    size_t          size;
    GpsLocation     gpsLocation;
    /* Provider indicator for HYBRID or GPS */
    uint16_t        position_source;
    /*allows HAL to pass additional information related to the location */
    int             rawDataSize;         /* in # of bytes */
    void            * rawData;
    bool            is_indoor;
    float           floor_number;
    char            map_url[GPS_LOCATION_MAP_URL_SIZE];
    unsigned char   map_index[GPS_LOCATION_MAP_INDEX_SIZE];
} UlpLocation;

/** AGPS type */
typedef int16_t AGpsExtType;
#define AGPS_TYPE_INVALID       -1
#define AGPS_TYPE_ANY           0
#define AGPS_TYPE_SUPL          1
#define AGPS_TYPE_C2K           2
#define AGPS_TYPE_WWAN_ANY      3
#define AGPS_TYPE_WIFI          4
#define AGPS_TYPE_SUPL_ES       5

/** SSID length */
#define SSID_BUF_SIZE (32+1)

typedef int16_t AGpsBearerType;
#define AGPS_APN_BEARER_INVALID    -1
#define AGPS_APN_BEARER_IPV4        0
#define AGPS_APN_BEARER_IPV6        1
#define AGPS_APN_BEARER_IPV4V6      2

#define GPS_DELETE_ALMANAC_CORR     0x00001000
#define GPS_DELETE_FREQ_BIAS_EST    0x00002000
#define GPS_DELETE_EPHEMERIS_GLO    0x00004000
#define GPS_DELETE_ALMANAC_GLO      0x00008000
#define GPS_DELETE_SVDIR_GLO        0x00010000
#define GPS_DELETE_SVSTEER_GLO      0x00020000
#define GPS_DELETE_ALMANAC_CORR_GLO 0x00040000
#define GPS_DELETE_TIME_GPS         0x00080000
#define GPS_DELETE_TIME_GLO         0x00100000

/** GPS extended callback structure. */
typedef struct {
    /** set to sizeof(GpsCallbacks) */
    size_t      size;
    gps_set_capabilities set_capabilities_cb;
    gps_acquire_wakelock acquire_wakelock_cb;
    gps_release_wakelock release_wakelock_cb;
    gps_create_thread create_thread_cb;
    gps_request_utc_time request_utc_time_cb;
} GpsExtCallbacks;

/** Callback to report the xtra server url to the client.
 *  The client should use this url when downloading xtra unless overwritten
 *  in the gps.conf file
 */
typedef void (* report_xtra_server)(const char*, const char*, const char*);

/** Callback structure for the XTRA interface. */
typedef struct {
    gps_xtra_download_request download_request_cb;
    gps_create_thread create_thread_cb;
    report_xtra_server report_xtra_server_cb;
} GpsXtraExtCallbacks;

/** Represents the status of AGPS. */
typedef struct {
    /** set to sizeof(AGpsExtStatus) */
    size_t          size;

    AGpsExtType type;
    AGpsStatusValue status;
    uint32_t        ipv4_addr;
    char            ipv6_addr[16];
    char            ssid[SSID_BUF_SIZE];
    char            password[SSID_BUF_SIZE];
} AGpsExtStatus;

/** Callback with AGPS status information.
 *  Can only be called from a thread created by create_thread_cb.
 */
typedef void (* agps_status_extended)(AGpsExtStatus* status);

/** Callback structure for the AGPS interface. */
typedef struct {
    agps_status_extended status_cb;
    gps_create_thread create_thread_cb;
} AGpsExtCallbacks;


/** GPS NI callback structure. */
typedef struct
{
    /**
     * Sends the notification request from HAL to GPSLocationProvider.
     */
    gps_ni_notify_callback notify_cb;
    gps_create_thread create_thread_cb;
} GpsNiExtCallbacks;

typedef enum loc_server_type {
    LOC_AGPS_CDMA_PDE_SERVER,
    LOC_AGPS_CUSTOM_PDE_SERVER,
    LOC_AGPS_MPC_SERVER,
    LOC_AGPS_SUPL_SERVER
} LocServerType;

typedef enum loc_position_mode_type {
    LOC_POSITION_MODE_STANDALONE,
    LOC_POSITION_MODE_MS_BASED,
    LOC_POSITION_MODE_MS_ASSISTED,
    LOC_POSITION_MODE_RESERVED_1,
    LOC_POSITION_MODE_RESERVED_2,
    LOC_POSITION_MODE_RESERVED_3,
    LOC_POSITION_MODE_RESERVED_4,
    LOC_POSITION_MODE_RESERVED_5
} LocPositionMode;

#define MIN_POSSIBLE_FIX_INTERVAL 1000 /* msec */

/** Flags to indicate which values are valid in a GpsLocationExtended. */
typedef uint16_t GpsLocationExtendedFlags;
/** GpsLocationExtended has valid pdop, hdop, vdop. */
#define GPS_LOCATION_EXTENDED_HAS_DOP 0x0001
/** GpsLocationExtended has valid altitude mean sea level. */
#define GPS_LOCATION_EXTENDED_HAS_ALTITUDE_MEAN_SEA_LEVEL 0x0002
/** UlpLocation has valid magnetic deviation. */
#define GPS_LOCATION_EXTENDED_HAS_MAG_DEV 0x0004
/** UlpLocation has valid mode indicator. */
#define GPS_LOCATION_EXTENDED_HAS_MODE_IND 0x0008
/** GpsLocationExtended has valid vertical uncertainty */
#define GPS_LOCATION_EXTENDED_HAS_VERT_UNC 0x0010
/** GpsLocationExtended has valid speed uncertainty */
#define GPS_LOCATION_EXTENDED_HAS_SPEED_UNC 0x0020

/** Represents gps location extended. */
typedef struct {
    /** set to sizeof(GpsLocationExtended) */
    size_t          size;
    /** Contains GpsLocationExtendedFlags bits. */
    uint16_t        flags;
    /** Contains the Altitude wrt mean sea level */
    float           altitudeMeanSeaLevel;
    /** Contains Position Dilusion of Precision. */
    float           pdop;
    /** Contains Horizontal Dilusion of Precision. */
    float           hdop;
    /** Contains Vertical Dilusion of Precision. */
    float           vdop;
    /** Contains Magnetic Deviation. */
    float           magneticDeviation;
    /** vertical uncertainty in meters */
    float           vert_unc;
    /** speed uncertainty in m/s */
    float           speed_unc;
} GpsLocationExtended;

enum loc_sess_status {
    LOC_SESS_SUCCESS,
    LOC_SESS_INTERMEDIATE,
    LOC_SESS_FAILURE
};

typedef uint32_t LocPosTechMask;
#define LOC_POS_TECH_MASK_DEFAULT ((LocPosTechMask)0x00000000)
#define LOC_POS_TECH_MASK_SATELLITE ((LocPosTechMask)0x00000001)
#define LOC_POS_TECH_MASK_CELLID ((LocPosTechMask)0x00000002)
#define LOC_POS_TECH_MASK_WIFI ((LocPosTechMask)0x00000004)
#define LOC_POS_TECH_MASK_SENSORS ((LocPosTechMask)0x00000008)
#define LOC_POS_TECH_MASK_REFERENCE_LOCATION ((LocPosTechMask)0x00000010)
#define LOC_POS_TECH_MASK_INJECTED_COARSE_POSITION ((LocPosTechMask)0x00000020)
#define LOC_POS_TECH_MASK_AFLT ((LocPosTechMask)0x00000040)
#define LOC_POS_TECH_MASK_HYBRID ((LocPosTechMask)0x00000080)

typedef enum {
  LOC_ENG_IF_REQUEST_SENDER_ID_QUIPC = 0,
  LOC_ENG_IF_REQUEST_SENDER_ID_MSAPM,
  LOC_ENG_IF_REQUEST_SENDER_ID_MSAPU,
  LOC_ENG_IF_REQUEST_SENDER_ID_GPSONE_DAEMON,
  LOC_ENG_IF_REQUEST_SENDER_ID_MODEM,
  LOC_ENG_IF_REQUEST_SENDER_ID_UNKNOWN
} loc_if_req_sender_id_e_type;


#define smaller_of(a, b) (((a) > (b)) ? (b) : (a))
#define MAX_APN_LEN 100

// This will be overridden by the individual adapters
// if necessary.
#define DEFAULT_IMPL(rtv)                                     \
{                                                             \
    LOC_LOGD("%s: default implementation invoked", __func__); \
    return rtv;                                               \
}

enum loc_api_adapter_err {
    LOC_API_ADAPTER_ERR_SUCCESS             = 0,
    LOC_API_ADAPTER_ERR_GENERAL_FAILURE     = 1,
    LOC_API_ADAPTER_ERR_UNSUPPORTED         = 2,
    LOC_API_ADAPTER_ERR_INVALID_HANDLE      = 4,
    LOC_API_ADAPTER_ERR_INVALID_PARAMETER   = 5,
    LOC_API_ADAPTER_ERR_ENGINE_BUSY         = 6,
    LOC_API_ADAPTER_ERR_PHONE_OFFLINE       = 7,
    LOC_API_ADAPTER_ERR_TIMEOUT             = 8,
    LOC_API_ADAPTER_ERR_SERVICE_NOT_PRESENT = 9,

    LOC_API_ADAPTER_ERR_ENGINE_DOWN         = 100,
    LOC_API_ADAPTER_ERR_FAILURE,
    LOC_API_ADAPTER_ERR_UNKNOWN
};

enum loc_api_adapter_event_index {
    LOC_API_ADAPTER_REPORT_POSITION = 0,       // Position report comes in loc_parsed_position_s_type
    LOC_API_ADAPTER_REPORT_SATELLITE,          // Satellite in view report
    LOC_API_ADAPTER_REPORT_NMEA_1HZ,           // NMEA report at 1HZ rate
    LOC_API_ADAPTER_REPORT_NMEA_POSITION,      // NMEA report at position report rate
    LOC_API_ADAPTER_REQUEST_NI_NOTIFY_VERIFY,  // NI notification/verification request
    LOC_API_ADAPTER_REQUEST_ASSISTANCE_DATA,   // Assistance data, eg: time, predicted orbits request
    LOC_API_ADAPTER_REQUEST_LOCATION_SERVER,   // Request for location server
    LOC_API_ADAPTER_REPORT_IOCTL,              // Callback report for loc_ioctl
    LOC_API_ADAPTER_REPORT_STATUS,             // Misc status report: eg, engine state
    LOC_API_ADAPTER_REQUEST_WIFI,              //
    LOC_API_ADAPTER_SENSOR_STATUS,             //
    LOC_API_ADAPTER_REQUEST_TIME_SYNC,         //
    LOC_API_ADAPTER_REPORT_SPI,                //
    LOC_API_ADAPTER_REPORT_NI_GEOFENCE,        //
    LOC_API_ADAPTER_GEOFENCE_GEN_ALERT,        //
    LOC_API_ADAPTER_REPORT_GENFENCE_BREACH,    //
    LOC_API_ADAPTER_PEDOMETER_CTRL,            //
    LOC_API_ADAPTER_MOTION_CTRL,               //

    LOC_API_ADAPTER_EVENT_MAX
};

#define LOC_API_ADAPTER_BIT_PARSED_POSITION_REPORT   (1<<LOC_API_ADAPTER_REPORT_POSITION)
#define LOC_API_ADAPTER_BIT_SATELLITE_REPORT         (1<<LOC_API_ADAPTER_REPORT_SATELLITE)
#define LOC_API_ADAPTER_BIT_NMEA_1HZ_REPORT          (1<<LOC_API_ADAPTER_REPORT_NMEA_1HZ)
#define LOC_API_ADAPTER_BIT_NMEA_POSITION_REPORT     (1<<LOC_API_ADAPTER_REPORT_NMEA_POSITION)
#define LOC_API_ADAPTER_BIT_NI_NOTIFY_VERIFY_REQUEST (1<<LOC_API_ADAPTER_REQUEST_NI_NOTIFY_VERIFY)
#define LOC_API_ADAPTER_BIT_ASSISTANCE_DATA_REQUEST  (1<<LOC_API_ADAPTER_REQUEST_ASSISTANCE_DATA)
#define LOC_API_ADAPTER_BIT_LOCATION_SERVER_REQUEST  (1<<LOC_API_ADAPTER_REQUEST_LOCATION_SERVER)
#define LOC_API_ADAPTER_BIT_IOCTL_REPORT             (1<<LOC_API_ADAPTER_REPORT_IOCTL)
#define LOC_API_ADAPTER_BIT_STATUS_REPORT            (1<<LOC_API_ADAPTER_REPORT_STATUS)
#define LOC_API_ADAPTER_BIT_REQUEST_WIFI             (1<<LOC_API_ADAPTER_REQUEST_WIFI)
#define LOC_API_ADAPTER_BIT_SENSOR_STATUS            (1<<LOC_API_ADAPTER_SENSOR_STATUS)
#define LOC_API_ADAPTER_BIT_REQUEST_TIME_SYNC        (1<<LOC_API_ADAPTER_REQUEST_TIME_SYNC)
#define LOC_API_ADAPTER_BIT_REPORT_SPI               (1<<LOC_API_ADAPTER_REPORT_SPI)
#define LOC_API_ADAPTER_BIT_REPORT_NI_GEOFENCE       (1<<LOC_API_ADAPTER_REPORT_NI_GEOFENCE)
#define LOC_API_ADAPTER_BIT_GEOFENCE_GEN_ALERT       (1<<LOC_API_ADAPTER_GEOFENCE_GEN_ALERT)
#define LOC_API_ADAPTER_BIT_REPORT_GENFENCE_BREACH   (1<<LOC_API_ADAPTER_REPORT_GENFENCE_BREACH)
#define LOC_API_ADAPTER_BIT_PEDOMETER_CTRL           (1<<LOC_API_ADAPTER_PEDOMETER_CTRL)
#define LOC_API_ADAPTER_BIT_MOTION_CTRL              (1<<LOC_API_ADAPTER_MOTION_CTRL)

typedef unsigned int LOC_API_ADAPTER_EVENT_MASK_T;


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* GPS_EXTENDED_C_H */

