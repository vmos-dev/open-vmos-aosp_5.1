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

#include "wifi_hal.h"

#ifndef __WIFI_HAL_COMMON_H__
#define __WIFI_HAL_COMMON_H__

#ifndef LOG_TAG
#define LOG_TAG  "WifiHAL"
#endif

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
#include <netlink-types.h>

#include "nl80211_copy.h"

#include <utils/Log.h>

#define SOCKET_BUFFER_SIZE      (32768U)
#define RECV_BUF_SIZE           (4096)
#define DEFAULT_EVENT_CB_SIZE   (64)
#define DEFAULT_CMD_SIZE        (64)

typedef void (*wifi_internal_event_handler) (wifi_handle handle, int events);

class WifiCommand;

typedef struct {
    int nl_cmd;
    uint32_t vendor_id;
    int vendor_subcmd;
    nl_recvmsg_msg_cb_t cb_func;
    void *cb_arg;
} cb_info;

typedef struct {
    wifi_request_id id;
    WifiCommand *cmd;
} cmd_info;

typedef struct {
    wifi_handle handle;                             // handle to wifi data
    char name[8+1];                                 // interface name + trailing null
    int  id;                                        // id to use when talking to driver
} interface_info;

typedef struct {

    struct nl_sock *cmd_sock;                       // command socket object
    struct nl_sock *event_sock;                     // event socket object
    int nl80211_family_id;                          // family id for 80211 driver

    bool in_event_loop;                             // Indicates that event loop is active
    bool clean_up;                                  // Indication to clean up the socket

    wifi_internal_event_handler event_handler;      // default event handler
    wifi_cleaned_up_handler cleaned_up_handler;     // socket cleaned up handler

    cb_info *event_cb;                              // event callbacks
    int num_event_cb;                               // number of event callbacks
    int alloc_event_cb;                             // number of allocated callback objects

    cmd_info *cmd;                                  // Outstanding commands
    int num_cmd;                                    // number of commands
    int alloc_cmd;                                  // number of commands allocated

    interface_info **interfaces;                    // array of interfaces
    int num_interfaces;                             // number of interfaces

    // add other details
} hal_info;

wifi_error wifi_register_handler(wifi_handle handle, int cmd, nl_recvmsg_msg_cb_t func, void *arg);
wifi_error wifi_register_vendor_handler(wifi_handle handle,
            uint32_t id, int subcmd, nl_recvmsg_msg_cb_t func, void *arg);

void wifi_unregister_handler(wifi_handle handle, int cmd);
void wifi_unregister_vendor_handler(wifi_handle handle, uint32_t id, int subcmd);

wifi_error wifi_register_cmd(wifi_handle handle, int id, WifiCommand *cmd);
WifiCommand *wifi_unregister_cmd(wifi_handle handle, int id);
void wifi_unregister_cmd(wifi_handle handle, WifiCommand *cmd);

interface_info *getIfaceInfo(wifi_interface_handle);
wifi_handle getWifiHandle(wifi_interface_handle handle);
hal_info *getHalInfo(wifi_handle handle);
hal_info *getHalInfo(wifi_interface_handle handle);
wifi_handle getWifiHandle(hal_info *info);
wifi_interface_handle getIfaceHandle(interface_info *info);


// some common macros

#define min(x, y)       ((x) < (y) ? (x) : (y))
#define max(x, y)       ((x) > (y) ? (x) : (y))

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
void hexdump(char *bytes, u16 len);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

