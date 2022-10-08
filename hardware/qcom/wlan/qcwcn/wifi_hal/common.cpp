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

#include <stdlib.h>
#include <linux/pkt_sched.h>
#include <netlink/object-api.h>
#include <netlink-types.h>

#include "wifi_hal.h"
#include "common.h"
#include <netlink-types.h>

interface_info *getIfaceInfo(wifi_interface_handle handle)
{
    return (interface_info *)handle;
}

wifi_handle getWifiHandle(wifi_interface_handle handle)
{
    return getIfaceInfo(handle)->handle;
}

hal_info *getHalInfo(wifi_handle handle)
{
    return (hal_info *)handle;
}

hal_info *getHalInfo(wifi_interface_handle handle)
{
    return getHalInfo(getWifiHandle(handle));
}

wifi_handle getWifiHandle(hal_info *info)
{
    return (wifi_handle)info;
}

wifi_interface_handle getIfaceHandle(interface_info *info)
{
    return (wifi_interface_handle)info;
}

wifi_error wifi_register_handler(wifi_handle handle, int cmd, nl_recvmsg_msg_cb_t func, void *arg)
{
    hal_info *info = (hal_info *)handle;

    /* TODO: check for multiple handlers? */

    if (info->num_event_cb < info->alloc_event_cb) {
        info->event_cb[info->num_event_cb].nl_cmd  = cmd;
        info->event_cb[info->num_event_cb].vendor_id  = 0;
        info->event_cb[info->num_event_cb].vendor_subcmd  = 0;
        info->event_cb[info->num_event_cb].cb_func = func;
        info->event_cb[info->num_event_cb].cb_arg  = arg;
        info->num_event_cb++;
        ALOGI("Successfully added event handler %p for command %d", func, cmd);
        return WIFI_SUCCESS;
    } else {
        return WIFI_ERROR_OUT_OF_MEMORY;
    }
}

wifi_error wifi_register_vendor_handler(wifi_handle handle,
        uint32_t id, int subcmd, nl_recvmsg_msg_cb_t func, void *arg)
{
    hal_info *info = (hal_info *)handle;

    for (int i = 0; i < info->num_event_cb; i++) {
        if(info->event_cb[info->num_event_cb].vendor_id  == id &&
           info->event_cb[info->num_event_cb].vendor_subcmd == subcmd)
        {
            info->event_cb[info->num_event_cb].cb_func = func;
            info->event_cb[info->num_event_cb].cb_arg  = arg;
            ALOGI("Updated event handler %p for vendor 0x%0x, subcmd 0x%0x"
                " and arg %p", func, id, subcmd, arg);
            return WIFI_SUCCESS;
        }
    }

    if (info->num_event_cb < info->alloc_event_cb) {
        info->event_cb[info->num_event_cb].nl_cmd  = NL80211_CMD_VENDOR;
        info->event_cb[info->num_event_cb].vendor_id  = id;
        info->event_cb[info->num_event_cb].vendor_subcmd  = subcmd;
        info->event_cb[info->num_event_cb].cb_func = func;
        info->event_cb[info->num_event_cb].cb_arg  = arg;
        info->num_event_cb++;
        ALOGI("Added event handler %p for vendor 0x%0x, subcmd 0x%0x and arg"
            " %p", func, id, subcmd, arg);
        return WIFI_SUCCESS;
    } else {
        return WIFI_ERROR_OUT_OF_MEMORY;
    }
}

void wifi_unregister_handler(wifi_handle handle, int cmd)
{
    hal_info *info = (hal_info *)handle;

    if (cmd == NL80211_CMD_VENDOR) {
        ALOGE("Must use wifi_unregister_vendor_handler to remove vendor handlers");
    }

    for (int i = 0; i < info->num_event_cb; i++) {
        if (info->event_cb[i].nl_cmd == cmd) {
            memmove(&info->event_cb[i], &info->event_cb[i+1],
                (info->num_event_cb - i) * sizeof(cb_info));
            info->num_event_cb--;
            ALOGI("Successfully removed event handler for command %d", cmd);
            return;
        }
    }
}

void wifi_unregister_vendor_handler(wifi_handle handle, uint32_t id, int subcmd)
{
    hal_info *info = (hal_info *)handle;

    for (int i = 0; i < info->num_event_cb; i++) {

        if (info->event_cb[i].nl_cmd == NL80211_CMD_VENDOR
                && info->event_cb[i].vendor_id == id
                && info->event_cb[i].vendor_subcmd == subcmd) {

            memmove(&info->event_cb[i], &info->event_cb[i+1],
                (info->num_event_cb - i) * sizeof(cb_info));
            info->num_event_cb--;
            ALOGI("Successfully removed event handler for vendor 0x%0x", id);
            return;
        }
    }
}


wifi_error wifi_register_cmd(wifi_handle handle, int id, WifiCommand *cmd)
{
    hal_info *info = (hal_info *)handle;

    ALOGD("registering command %d", id);

    if (info->num_cmd < info->alloc_cmd) {
        info->cmd[info->num_cmd].id   = id;
        info->cmd[info->num_cmd].cmd  = cmd;
        info->num_cmd++;
        ALOGI("Successfully added command %d: %p", id, cmd);
        return WIFI_SUCCESS;
    } else {
        return WIFI_ERROR_OUT_OF_MEMORY;
    }
}

WifiCommand *wifi_unregister_cmd(wifi_handle handle, int id)
{
    hal_info *info = (hal_info *)handle;

    ALOGD("un-registering command %d", id);

    for (int i = 0; i < info->num_cmd; i++) {
        if (info->cmd[i].id == id) {
            WifiCommand *cmd = info->cmd[i].cmd;
            memmove(&info->cmd[i], &info->cmd[i+1], (info->num_cmd - i) * sizeof(cmd_info));
            info->num_cmd--;
            ALOGI("Successfully removed command %d: %p", id, cmd);
            return cmd;
        }
    }

    return NULL;
}

void wifi_unregister_cmd(wifi_handle handle, WifiCommand *cmd)
{
    hal_info *info = (hal_info *)handle;

    for (int i = 0; i < info->num_cmd; i++) {
        if (info->cmd[i].cmd == cmd) {
            int id = info->cmd[i].id;
            memmove(&info->cmd[i], &info->cmd[i+1], (info->num_cmd - i) * sizeof(cmd_info));
            info->num_cmd--;
            ALOGI("Successfully removed command %d: %p", id, cmd);
            return;
        }
    }
}

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

void hexdump(char *bytes, u16 len)
{
    int i=0;
    ALOGI("******HexDump len:%d*********", len);
    for (i = 0; ((i + 7) < len); i+=8) {
        ALOGI("%02x %02x %02x %02x   %02x %02x %02x %02x",
              bytes[i], bytes[i+1],
              bytes[i+2], bytes[i+3],
              bytes[i+4], bytes[i+5],
              bytes[i+6], bytes[i+7]);
    }
    if ((len - i) >= 4) {
        ALOGI("%02x %02x %02x %02x",
              bytes[i], bytes[i+1],
              bytes[i+2], bytes[i+3]);
        i+=4;
    }
    for (;i < len;i++) {
        ALOGI("%02x", bytes[i]);
    }
    ALOGI("******HexDump End***********");
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
