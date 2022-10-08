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
#ifndef _MSM_THERMAL_IOCTL_H
#define _MSM_THERMAL_IOCTL_H
#include <linux/ioctl.h>
#define MSM_THERMAL_IOCTL_NAME "msm_thermal_query"
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct __attribute__((__packed__)) cpu_freq_arg {
 uint32_t cpu_num;
 uint32_t freq_req;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct __attribute__((__packed__)) msm_thermal_ioctl {
 uint32_t size;
 union {
 struct cpu_freq_arg cpu_freq;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 };
};
enum {
 MSM_SET_CPU_MAX_FREQ = 0x00,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 MSM_SET_CPU_MIN_FREQ = 0x01,
 MSM_CMD_MAX_NR,
};
#define MSM_THERMAL_MAGIC_NUM 0xCA
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define MSM_THERMAL_SET_CPU_MAX_FREQUENCY _IOW(MSM_THERMAL_MAGIC_NUM,  MSM_SET_CPU_MAX_FREQ, struct msm_thermal_ioctl)
#define MSM_THERMAL_SET_CPU_MIN_FREQUENCY _IOW(MSM_THERMAL_MAGIC_NUM,  MSM_SET_CPU_MIN_FREQ, struct msm_thermal_ioctl)
#endif

