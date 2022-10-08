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
#ifndef __INTEL_SST_IOCTL_H__
#define __INTEL_SST_IOCTL_H__
#include <linux/types.h>
struct snd_ppp_params {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u8 algo_id;
 __u8 str_id;
 __u8 enable;
 __u8 operation;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 size;
 void *params;
} __packed;
struct snd_sst_driver_info {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 max_streams;
};
struct snd_sst_tuning_params {
 __u8 type;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u8 str_id;
 __u8 size;
 __u8 rsvd;
 __u64 addr;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
} __packed;
#define SNDRV_SST_DRIVER_INFO _IOR('L', 0x10, struct snd_sst_driver_info)
#define SNDRV_SST_SET_ALGO _IOW('L', 0x30, struct snd_ppp_params)
#define SNDRV_SST_GET_ALGO _IOWR('L', 0x31, struct snd_ppp_params)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define SNDRV_SST_TUNING_PARAMS _IOW('L', 0x32, struct snd_sst_tuning_params)
#endif

