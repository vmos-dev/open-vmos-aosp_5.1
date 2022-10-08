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
#ifndef __UAPI_MSM_FD__
#define __UAPI_MSM_FD__
#include <linux/videodev2.h>
struct msm_fd_event {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 buf_index;
 __u32 frame_id;
 __u32 face_cnt;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum msm_fd_pose {
 MSM_FD_POSE_FRONT,
 MSM_FD_POSE_RIGHT_DIAGONAL,
 MSM_FD_POSE_RIGHT,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 MSM_FD_POSE_LEFT_DIAGONAL,
 MSM_FD_POSE_LEFT,
};
struct msm_fd_face_data {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 pose;
 __u32 angle;
 __u32 confidence;
 __u32 reserved;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct v4l2_rect face;
};
struct msm_fd_result {
 __u32 frame_id;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 face_cnt;
 struct msm_fd_face_data __user *face_data;
};
#define VIDIOC_MSM_FD_GET_RESULT   _IOWR('V', BASE_VIDIOC_PRIVATE, struct msm_fd_result)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define MSM_EVENT_FD (V4L2_EVENT_PRIVATE_START)
#define V4L2_CID_FD_SPEED (V4L2_CID_PRIVATE_BASE)
#define V4L2_CID_FD_FACE_ANGLE (V4L2_CID_PRIVATE_BASE + 1)
#define V4L2_CID_FD_MIN_FACE_SIZE (V4L2_CID_PRIVATE_BASE + 2)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define V4L2_CID_FD_FACE_DIRECTION (V4L2_CID_PRIVATE_BASE + 3)
#define V4L2_CID_FD_DETECTION_THRESHOLD (V4L2_CID_PRIVATE_BASE + 4)
#define V4L2_CID_FD_WORK_MEMORY_SIZE (V4L2_CID_PRIVATE_BASE + 5)
#define V4L2_CID_FD_WORK_MEMORY_FD (V4L2_CID_PRIVATE_BASE + 6)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#endif
