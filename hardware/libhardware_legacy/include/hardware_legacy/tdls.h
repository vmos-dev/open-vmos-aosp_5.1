
#include "wifi_hal.h"

#ifndef _TDLS_H_
#define _TDLS_H_

typedef enum {
    WIFI_TDLS_DISABLED,                 /* TDLS is not enabled, or is disabled now */
    WIFI_TDLS_ENABLED,                  /* TDLS is enabled, but not yet tried */
    WIFI_TDLS_TRYING,                   /* Direct link is being attempted (optional) */
    WIFI_TDLS_ESTABLISHED,              /* Direct link is established */
    WIFI_TDLS_ESTABLISHED_OFF_CHANNEL,  /* Direct link is established using MCC */
    WIFI_TDLS_DROPPED,                  /* Direct link was established, but is now dropped */
    WIFI_TDLS_FAILED                    /* Direct link failed */
} wifi_tdls_state;

typedef enum {
    WIFI_TDLS_SUCCESS,                              /* Success */
    WIFI_TDLS_UNSPECIFIED           = -1,           /* Unspecified reason */
    WIFI_TDLS_NOT_SUPPORTED         = -2,           /* Remote side doesn't support TDLS */
    WIFI_TDLS_UNSUPPORTED_BAND      = -3,           /* Remote side doesn't support this band */
    WIFI_TDLS_NOT_BENEFICIAL        = -4,           /* Going to AP is better than going direct */
    WIFI_TDLS_DROPPED_BY_REMOTE     = -5            /* Remote side doesn't want it anymore */
} wifi_tdls_reason;

typedef struct {
    int channel;                        /* channel hint, in channel number (NOT frequency ) */
    int global_operating_class;         /* operating class to use */
    int max_latency_ms;                 /* max latency that can be tolerated by apps */
    int min_bandwidth_kbps;             /* bandwidth required by apps, in kilo bits per second */
} wifi_tdls_params;

typedef struct {
    int channel;
    int global_operating_class;
    wifi_tdls_state state;
    wifi_tdls_reason reason;
} wifi_tdls_status;

typedef struct {
    /* on_tdls_state_changed - reports state to TDLS link
     * Report this event when the state of TDLS link changes */
    void (*on_tdls_state_changed)(mac_addr addr, wifi_tdls_status status);
} wifi_tdls_handler;


/* wifi_enable_tdls - enables TDLS-auto mode for a specific route
 *
 * params specifies hints, which provide more information about
 * why TDLS is being sought. The firmware should do its best to
 * honor the hints before downgrading regular AP link
 *
 * On successful completion, must fire on_tdls_state_changed event
 * to indicate the status of TDLS operation.
 */
wifi_error wifi_enable_tdls(wifi_interface_handle iface, mac_addr addr,
        wifi_tdls_params params, wifi_tdls_handler handler);

/* wifi_disable_tdls - disables TDLS-auto mode for a specific route
 *
 * This terminates any existing TDLS with addr device, and frees the
 * device resources to make TDLS connections on new routes.
 *
 * DON'T fire any more events on 'handler' specified in earlier call to
 * wifi_enable_tdls after this action.
 */
wifi_error wifi_disable_tdls(wifi_interface_handle iface, mac_addr addr);

/* wifi_get_tdls_status - allows getting the status of TDLS for a specific route */
wifi_error wifi_get_tdls_status(wifi_interface_handle iface, mac_addr addr,
        wifi_tdls_status *status);

#endif