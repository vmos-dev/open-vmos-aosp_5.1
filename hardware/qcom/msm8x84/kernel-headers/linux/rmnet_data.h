/****************************************************************************
 ****************************************************************************
 ***
 ***   This header was automatically generated from a Linux kernel header
 ***   of the same name, to make information necessary for userspace to
 ***   call into the kernel available to libc.  It contains only constants,
 ***   structures, and macros generated from the original header, and thus,
 ***   contains no copyrightable information.
 ***
 ***   To edit the content of this header, modify the corresponding
 ***   source file (e.g. under external/kernel-headers/original/) then
 ***   run bionic/libc/kernel/tools/update_all.py
 ***
 ***   Any manual change here will be lost the next time this script will
 ***   be run. You've been warned!
 ***
 ****************************************************************************
 ****************************************************************************/
#ifndef _RMNET_DATA_H_
#define _RMNET_DATA_H_
#define RMNET_LOCAL_LOGICAL_ENDPOINT -1
#define RMNET_EGRESS_FORMAT__RESERVED__ (1<<0)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define RMNET_EGRESS_FORMAT_MAP (1<<1)
#define RMNET_EGRESS_FORMAT_AGGREGATION (1<<2)
#define RMNET_EGRESS_FORMAT_MUXING (1<<3)
#define RMNET_INGRESS_FIX_ETHERNET (1<<0)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define RMNET_INGRESS_FORMAT_MAP (1<<1)
#define RMNET_INGRESS_FORMAT_DEAGGREGATION (1<<2)
#define RMNET_INGRESS_FORMAT_DEMUXING (1<<3)
#define RMNET_INGRESS_FORMAT_MAP_COMMANDS (1<<4)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define RMNET_NETLINK_PROTO 31
#define RMNET_MAX_STR_LEN 16
#define RMNET_NL_DATA_MAX_LEN 64
#define RMNET_NETLINK_MSG_COMMAND 0
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define RMNET_NETLINK_MSG_RETURNCODE 1
#define RMNET_NETLINK_MSG_RETURNDATA 2
struct rmnet_nl_msg_s {
 uint16_t reserved;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint16_t message_type;
 uint16_t reserved2:14;
 uint16_t crd:2;
 union {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint16_t arg_length;
 uint16_t return_code;
 };
 union {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint8_t data[RMNET_NL_DATA_MAX_LEN];
 struct {
 uint8_t dev[RMNET_MAX_STR_LEN];
 uint32_t flags;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint16_t agg_size;
 uint16_t agg_count;
 uint8_t tail_spacing;
 } data_format;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct {
 uint8_t dev[RMNET_MAX_STR_LEN];
 int32_t ep_id;
 uint8_t operating_mode;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint8_t next_dev[RMNET_MAX_STR_LEN];
 } local_ep_config;
 struct {
 uint32_t id;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint8_t vnd_name[RMNET_MAX_STR_LEN];
 } vnd;
 struct {
 uint32_t id;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t map_flow_id;
 uint32_t tc_flow_id;
 } flow_control;
 };
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
enum rmnet_netlink_message_types_e {
 RMNET_NETLINK_ASSOCIATE_NETWORK_DEVICE,
 RMNET_NETLINK_UNASSOCIATE_NETWORK_DEVICE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 RMNET_NETLINK_GET_NETWORK_DEVICE_ASSOCIATED,
 RMNET_NETLINK_SET_LINK_EGRESS_DATA_FORMAT,
 RMNET_NETLINK_GET_LINK_EGRESS_DATA_FORMAT,
 RMNET_NETLINK_SET_LINK_INGRESS_DATA_FORMAT,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 RMNET_NETLINK_GET_LINK_INGRESS_DATA_FORMAT,
 RMNET_NETLINK_SET_LOGICAL_EP_CONFIG,
 RMNET_NETLINK_UNSET_LOGICAL_EP_CONFIG,
 RMNET_NETLINK_GET_LOGICAL_EP_CONFIG,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 RMNET_NETLINK_NEW_VND,
 RMNET_NETLINK_NEW_VND_WITH_PREFIX,
 RMNET_NETLINK_GET_VND_NAME,
 RMNET_NETLINK_FREE_VND,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 RMNET_NETLINK_ADD_VND_TC_FLOW,
 RMNET_NETLINK_DEL_VND_TC_FLOW
};
enum rmnet_config_endpoint_modes_e {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 RMNET_EPMODE_NONE,
 RMNET_EPMODE_VND,
 RMNET_EPMODE_BRIDGE,
 RMNET_EPMODE_LENGTH
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
enum rmnet_config_return_codes_e {
 RMNET_CONFIG_OK,
 RMNET_CONFIG_UNKNOWN_MESSAGE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 RMNET_CONFIG_UNKNOWN_ERROR,
 RMNET_CONFIG_NOMEM,
 RMNET_CONFIG_DEVICE_IN_USE,
 RMNET_CONFIG_INVALID_REQUEST,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 RMNET_CONFIG_NO_SUCH_DEVICE,
 RMNET_CONFIG_BAD_ARGUMENTS,
 RMNET_CONFIG_BAD_EGRESS_DEVICE,
 RMNET_CONFIG_TC_HANDLE_FULL
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#endif

