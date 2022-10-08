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
#ifndef __MSM_AUDIO_AC3_H
#define __MSM_AUDIO_AC3_H
#include <linux/msm_audio.h>
#define AUDIO_SET_AC3_CONFIG _IOW(AUDIO_IOCTL_MAGIC,   (AUDIO_MAX_COMMON_IOCTL_NUM+0), unsigned)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define AUDIO_GET_AC3_CONFIG _IOR(AUDIO_IOCTL_MAGIC,   (AUDIO_MAX_COMMON_IOCTL_NUM+1), unsigned)
#define AUDAC3_DEF_WORDSIZE 0
#define AUDAC3_DEF_USER_DOWNMIX_FLAG 0x0
#define AUDAC3_DEF_USER_KARAOKE_FLAG 0x0
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define AUDAC3_DEF_ERROR_CONCEALMENT 0
#define AUDAC3_DEF_MAX_REPEAT_COUNT 0
struct msm_audio_ac3_config {
 unsigned short numChans;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 unsigned short wordSize;
 unsigned short kCapableMode;
 unsigned short compMode;
 unsigned short outLfeOn;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 unsigned short outputMode;
 unsigned short stereoMode;
 unsigned short dualMonoMode;
 unsigned short fsCod;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 unsigned short pcmScaleFac;
 unsigned short dynRngScaleHi;
 unsigned short dynRngScaleLow;
 unsigned short user_downmix_flag;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 unsigned short user_karaoke_flag;
 unsigned short dm_address_high;
 unsigned short dm_address_low;
 unsigned short ko_address_high;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 unsigned short ko_address_low;
 unsigned short error_concealment;
 unsigned short max_rep_count;
 unsigned short channel_routing_mode[6];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#endif
