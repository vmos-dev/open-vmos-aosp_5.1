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
#ifndef __MFD_TABLA_PDATA_H__
#define __MFD_TABLA_PDATA_H__
#include <linux/slimbus/slimbus.h>
#define MICBIAS_EXT_BYP_CAP 0x00
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define MICBIAS_NO_EXT_BYP_CAP 0x01
#define SITAR_LDOH_1P95_V 0x0
#define SITAR_LDOH_2P35_V 0x1
#define SITAR_LDOH_2P75_V 0x2
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define SITAR_LDOH_2P85_V 0x3
#define SITAR_CFILT1_SEL 0x0
#define SITAR_CFILT2_SEL 0x1
#define SITAR_CFILT3_SEL 0x2
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TABLA_LDOH_1P95_V 0x0
#define TABLA_LDOH_2P35_V 0x1
#define TABLA_LDOH_2P75_V 0x2
#define TABLA_LDOH_2P85_V 0x3
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TABLA_CFILT1_SEL 0x0
#define TABLA_CFILT2_SEL 0x1
#define TABLA_CFILT3_SEL 0x2
#define TAIKO_CFILT1_SEL 0x0
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TAIKO_CFILT2_SEL 0x1
#define TAIKO_CFILT3_SEL 0x2
#define TAIKO_LDOH_1P95_V 0x0
#define TAIKO_LDOH_2P35_V 0x1
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TAIKO_LDOH_2P75_V 0x2
#define TAIKO_LDOH_2P85_V 0x3
#define MAX_AMIC_CHANNEL 7
#define TABLA_OCP_300_MA 0x0
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TABLA_OCP_350_MA 0x2
#define TABLA_OCP_365_MA 0x3
#define TABLA_OCP_150_MA 0x4
#define TABLA_OCP_190_MA 0x6
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TABLA_OCP_220_MA 0x7
#define TABLA_DCYCLE_255 0x0
#define TABLA_DCYCLE_511 0x1
#define TABLA_DCYCLE_767 0x2
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TABLA_DCYCLE_1023 0x3
#define TABLA_DCYCLE_1279 0x4
#define TABLA_DCYCLE_1535 0x5
#define TABLA_DCYCLE_1791 0x6
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TABLA_DCYCLE_2047 0x7
#define TABLA_DCYCLE_2303 0x8
#define TABLA_DCYCLE_2559 0x9
#define TABLA_DCYCLE_2815 0xA
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TABLA_DCYCLE_3071 0xB
#define TABLA_DCYCLE_3327 0xC
#define TABLA_DCYCLE_3583 0xD
#define TABLA_DCYCLE_3839 0xE
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TABLA_DCYCLE_4095 0xF
struct wcd9xxx_amic {
 u8 legacy_mode:MAX_AMIC_CHANNEL;
 u8 txfe_enable:MAX_AMIC_CHANNEL;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 u8 txfe_buff:MAX_AMIC_CHANNEL;
 u8 use_pdata:MAX_AMIC_CHANNEL;
};
struct wcd9xxx_micbias_setting {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 u8 ldoh_v;
 u32 cfilt1_mv;
 u32 cfilt2_mv;
 u32 cfilt3_mv;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 u8 bias1_cfilt_sel;
 u8 bias2_cfilt_sel;
 u8 bias3_cfilt_sel;
 u8 bias4_cfilt_sel;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 u8 bias1_cap_mode;
 u8 bias2_cap_mode;
 u8 bias3_cap_mode;
 u8 bias4_cap_mode;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct wcd9xxx_ocp_setting {
 unsigned int use_pdata:1;
 unsigned int num_attempts:4;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 unsigned int run_time:4;
 unsigned int wait_time:4;
 unsigned int hph_ocp_limit:3;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define MAX_REGULATOR 7
#define WCD9XXX_CDC_VDDA_CP_CUR_MAX 500000
#define WCD9XXX_CDC_VDDA_RX_CUR_MAX 20000
#define WCD9XXX_CDC_VDDA_TX_CUR_MAX 20000
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define WCD9XXX_VDDIO_CDC_CUR_MAX 5000
#define WCD9XXX_VDDD_CDC_D_CUR_MAX 5000
#define WCD9XXX_VDDD_CDC_A_CUR_MAX 5000
struct wcd9xxx_regulator {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 const char *name;
 int min_uV;
 int max_uV;
 int optimum_uA;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct regulator *regulator;
};
struct wcd9xxx_pdata {
 int irq;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 int irq_base;
 int num_irqs;
 int reset_gpio;
 struct wcd9xxx_amic amic_settings;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct slim_device slimbus_slave_device;
 struct wcd9xxx_micbias_setting micbias;
 struct wcd9xxx_ocp_setting ocp;
 struct wcd9xxx_regulator regulator[MAX_REGULATOR];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#endif

