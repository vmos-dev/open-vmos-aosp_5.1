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
#ifndef _UAPI_MSM_AUDIO_AMR_WB_PLUS_H
#define _UAPI_MSM_AUDIO_AMR_WB_PLUS_H
#define AUDIO_GET_AMRWBPLUS_CONFIG_V2 _IOR(AUDIO_IOCTL_MAGIC,   (AUDIO_MAX_COMMON_IOCTL_NUM+2), struct msm_audio_amrwbplus_config_v2)
#define AUDIO_SET_AMRWBPLUS_CONFIG_V2 _IOW(AUDIO_IOCTL_MAGIC,   (AUDIO_MAX_COMMON_IOCTL_NUM+3), struct msm_audio_amrwbplus_config_v2)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct msm_audio_amrwbplus_config_v2 {
 unsigned int size_bytes;
 unsigned int version;
 unsigned int num_channels;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 unsigned int amr_band_mode;
 unsigned int amr_dtx_mode;
 unsigned int amr_frame_fmt;
 unsigned int amr_lsf_idx;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#endif

