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
#ifndef __WCD9310_SLIMSLAVE_H_
#define __WCD9310_SLIMSLAVE_H_
#include <linux/slimbus/slimbus.h>
#include <linux/mfd/wcd9xxx/core.h>
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum {
 SLIM_TX_1 = 128,
 SLIM_TX_2 = 129,
 SLIM_TX_3 = 130,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 SLIM_TX_4 = 131,
 SLIM_TX_5 = 132,
 SLIM_TX_6 = 133,
 SLIM_TX_7 = 134,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 SLIM_TX_8 = 135,
 SLIM_TX_9 = 136,
 SLIM_TX_10 = 137,
 SLIM_RX_1 = 138,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 SLIM_RX_2 = 139,
 SLIM_RX_3 = 140,
 SLIM_RX_4 = 141,
 SLIM_RX_5 = 142,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 SLIM_RX_6 = 143,
 SLIM_RX_7 = 144,
 SLIM_MAX = 145
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TABLA_SB_PGD_MAX_NUMBER_OF_TX_SLAVE_DEV_PORTS 10
#define TAIKO_SB_PGD_MAX_NUMBER_OF_TX_SLAVE_DEV_PORTS 16
#define SLIM_MAX_TX_PORTS TAIKO_SB_PGD_MAX_NUMBER_OF_TX_SLAVE_DEV_PORTS
#define TABLA_SB_PGD_OFFSET_OF_RX_SLAVE_DEV_PORTS   TABLA_SB_PGD_MAX_NUMBER_OF_TX_SLAVE_DEV_PORTS
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TAIKO_SB_PGD_OFFSET_OF_RX_SLAVE_DEV_PORTS   TAIKO_SB_PGD_MAX_NUMBER_OF_TX_SLAVE_DEV_PORTS
#define TABLA_SB_PGD_MAX_NUMBER_OF_RX_SLAVE_DEV_PORTS 7
#define TAIKO_SB_PGD_MAX_NUMBER_OF_RX_SLAVE_DEV_PORTS 13
#define SLIM_MAX_RX_PORTS TAIKO_SB_PGD_MAX_NUMBER_OF_RX_SLAVE_DEV_PORTS
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TABLA_SB_PGD_RX_PORT_MULTI_CHANNEL_0_START_PORT_ID   TABLA_SB_PGD_OFFSET_OF_RX_SLAVE_DEV_PORTS
#define TAIKO_SB_PGD_RX_PORT_MULTI_CHANNEL_0_START_PORT_ID   TAIKO_SB_PGD_OFFSET_OF_RX_SLAVE_DEV_PORTS
#define TABLA_SB_PGD_RX_PORT_MULTI_CHANNEL_0_END_PORT_ID 16
#define TAIKO_SB_PGD_RX_PORT_MULTI_CHANNEL_0_END_PORT_ID 31
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TABLA_SB_PGD_TX_PORT_MULTI_CHANNEL_1_END_PORT_ID 9
#define TAIKO_SB_PGD_TX_PORT_MULTI_CHANNEL_1_END_PORT_ID 15
#define SB_PGD_PORT_BASE 0x000
#define SB_PGD_PORT_CFG_BYTE_ADDR(offset, port_num)   (SB_PGD_PORT_BASE + offset + (1 * port_num))
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define SB_PGD_TX_PORT_MULTI_CHANNEL_0(port_num)   (SB_PGD_PORT_BASE + 0x100 + 4*port_num)
#define SB_PGD_TX_PORT_MULTI_CHANNEL_0_START_PORT_ID 0
#define SB_PGD_TX_PORT_MULTI_CHANNEL_0_END_PORT_ID 7
#define SB_PGD_TX_PORT_MULTI_CHANNEL_1(port_num)   (SB_PGD_PORT_BASE + 0x101 + 4*port_num)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define SB_PGD_TX_PORT_MULTI_CHANNEL_1_START_PORT_ID 8
#define SB_PGD_RX_PORT_MULTI_CHANNEL_0(offset, port_num)   (SB_PGD_PORT_BASE + offset + (4 * port_num))
#define SLAVE_PORT_WATER_MARK_VALUE 2
#define SLAVE_PORT_WATER_MARK_SHIFT 1
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define SLAVE_PORT_ENABLE 1
#define SLAVE_PORT_DISABLE 0
#define BASE_CH_NUM 128
#endif
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */

