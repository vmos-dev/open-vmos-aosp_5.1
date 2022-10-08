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
#ifndef __UAPI_MEDIA_MSMB_BUF_MNGR_H__
#define __UAPI_MEDIA_MSMB_BUF_MNGR_H__
struct msm_buf_mngr_info {
 uint32_t session_id;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t stream_id;
 uint32_t frame_id;
 struct timeval timestamp;
 uint32_t index;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct v4l2_subdev *msm_buf_mngr_get_subdev(void);
#define VIDIOC_MSM_BUF_MNGR_GET_BUF   _IOWR('V', BASE_VIDIOC_PRIVATE + 33, struct msm_buf_mngr_info)
#define VIDIOC_MSM_BUF_MNGR_PUT_BUF   _IOWR('V', BASE_VIDIOC_PRIVATE + 34, struct msm_buf_mngr_info)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define VIDIOC_MSM_BUF_MNGR_BUF_DONE   _IOWR('V', BASE_VIDIOC_PRIVATE + 35, struct msm_buf_mngr_info)
#define VIDIOC_MSM_BUF_MNGR_INIT   _IOWR('V', BASE_VIDIOC_PRIVATE + 36, struct msm_buf_mngr_info)
#define VIDIOC_MSM_BUF_MNGR_DEINIT   _IOWR('V', BASE_VIDIOC_PRIVATE + 37, struct msm_buf_mngr_info)
#endif
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
