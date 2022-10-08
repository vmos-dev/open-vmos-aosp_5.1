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
#ifndef _H_MSM_VPU_H_
#define _H_MSM_VPU_H_
#include <linux/videodev2.h>
#define V4L2_PLANE_MEM_OFFSET 0
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum vpu_colorspace {
 VPU_CS_MIN = 0,
 VPU_CS_RGB_FULL = 1,
 VPU_CS_RGB_LIMITED = 2,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 VPU_CS_REC601_FULL = 3,
 VPU_CS_REC601_LIMITED = 4,
 VPU_CS_REC709_FULL = 5,
 VPU_CS_REC709_LIMITED = 6,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 VPU_CS_SMPTE240_FULL = 7,
 VPU_CS_SMPTE240_LIMITED = 8,
 VPU_CS_MAX = 9,
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define VPU_FMT_EXT_FLAG_BT 1
#define VPU_FMT_EXT_FLAG_TB 2
#define VPU_FMT_EXT_FLAG_3D 4
struct v4l2_format_vpu_extension {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u8 flag;
 __u8 gap_in_lines;
};
#define V4L2_PIX_FMT_XRGB2 v4l2_fourcc('X', 'R', 'G', '2')
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define V4L2_PIX_FMT_XBGR2 v4l2_fourcc('X', 'B', 'G', '2')
#define V4L2_PIX_FMT_YUYV10 v4l2_fourcc('Y', 'U', 'Y', 'L')
#define V4L2_PIX_FMT_YUV8 v4l2_fourcc('Y', 'U', 'V', '8')
#define V4L2_PIX_FMT_YUV10 v4l2_fourcc('Y', 'U', 'V', 'L')
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define V4L2_PIX_FMT_YUYV10BWC v4l2_fourcc('Y', 'B', 'W', 'C')
#define VPU_INPUT_TYPE_HOST 0
#define VPU_INPUT_TYPE_VCAP 1
#define VPU_OUTPUT_TYPE_HOST 0
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define VPU_OUTPUT_TYPE_DISPLAY 1
#define VPU_PIPE_VCAP0 (1 << 16)
#define VPU_PIPE_VCAP1 (1 << 17)
#define VPU_PIPE_DISPLAY0 (1 << 18)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define VPU_PIPE_DISPLAY1 (1 << 19)
#define VPU_PIPE_DISPLAY2 (1 << 20)
#define VPU_PIPE_DISPLAY3 (1 << 21)
#define VPU_PRIVATE_EVENT_BASE (V4L2_EVENT_PRIVATE_START + 6 * 1000)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum VPU_PRIVATE_EVENT {
 VPU_EVENT_START = VPU_PRIVATE_EVENT_BASE,
 VPU_EVENT_FLUSH_DONE = VPU_EVENT_START + 1,
 VPU_EVENT_ACTIVE_REGION_CHANGED = VPU_EVENT_START + 2,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 VPU_EVENT_SESSION_TIMESTAMP = VPU_EVENT_START + 3,
 VPU_EVENT_HW_ERROR = VPU_EVENT_START + 11,
 VPU_EVENT_INVALID_CONFIG = VPU_EVENT_START + 12,
 VPU_EVENT_FAILED_SESSION_STREAMING = VPU_EVENT_START + 13,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 VPU_EVENT_END
};
struct vpu_ctrl_standard {
 __u32 enable;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __s32 value;
};
struct vpu_ctrl_auto_manual {
 __u32 enable;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 auto_mode;
 __s32 value;
};
struct vpu_ctrl_range_mapping {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 enable;
 __u32 y_range;
 __u32 uv_range;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define VPU_ACTIVE_REGION_N_EXCLUSIONS 1
struct vpu_ctrl_active_region_param {
 __u32 enable;
 __u32 num_exclusions;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct v4l2_rect detection_region;
 struct v4l2_rect excluded_regions[VPU_ACTIVE_REGION_N_EXCLUSIONS];
};
struct vpu_ctrl_deinterlacing_mode {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 field_polarity;
 __u32 mvp_mode;
};
struct vpu_ctrl_hqv {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 enable;
 __u32 sharpen_strength;
 __u32 auto_nr_strength;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct vpu_info_frame_timestamp {
 __u32 pts_low;
 __u32 pts_high;
 __u32 qtime_low;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 qtime_high;
};
struct vpu_control {
 __u32 control_id;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 union control_data {
 __s32 value;
 struct vpu_ctrl_standard standard;
 struct vpu_ctrl_auto_manual auto_manual;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct vpu_ctrl_range_mapping range_mapping;
 struct vpu_ctrl_active_region_param active_region_param;
 struct v4l2_rect active_region_result;
 struct vpu_ctrl_deinterlacing_mode deinterlacing_mode;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct vpu_ctrl_hqv hqv;
 struct vpu_info_frame_timestamp timestamp;
 __u8 reserved[124];
 } data;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#define VPU_CTRL_ID_MIN 0
#define VPU_CTRL_NOISE_REDUCTION 1
#define VPU_CTRL_IMAGE_ENHANCEMENT 2
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define VPU_CTRL_ANAMORPHIC_SCALING 3
#define VPU_CTRL_DIRECTIONAL_INTERPOLATION 4
#define VPU_CTRL_BACKGROUND_COLOR 5
#define VPU_CTRL_RANGE_MAPPING 6
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define VPU_CTRL_DEINTERLACING_MODE 7
#define VPU_CTRL_ACTIVE_REGION_PARAM 8
#define VPU_CTRL_ACTIVE_REGION_RESULT 9
#define VPU_CTRL_PRIORITY 10
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define VPU_CTRL_CONTENT_PROTECTION 11
#define VPU_CTRL_DISPLAY_REFRESH_RATE 12
#define VPU_CTRL_HQV 20
#define VPU_CTRL_HQV_SHARPEN 21
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define VPU_CTRL_HQV_AUTONR 22
#define VPU_CTRL_ACE 23
#define VPU_CTRL_ACE_BRIGHTNESS 24
#define VPU_CTRL_ACE_CONTRAST 25
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define VPU_CTRL_2D3D 26
#define VPU_CTRL_2D3D_DEPTH 27
#define VPU_CTRL_FRC 28
#define VPU_CTRL_FRC_MOTION_SMOOTHNESS 29
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define VPU_CTRL_FRC_MOTION_CLEAR 30
#define VPU_INFO_TIMESTAMP 35
#define VPU_CTRL_TIMESTAMP_INFO_MODE 36
#define VPU_INFO_STATISTICS 37
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define VPU_CTRL_LATENCY 38
#define VPU_CTRL_LATENCY_MODE 39
#define VPU_CTRL_ID_MAX 40
#define VPU_MAX_EXT_DATA_SIZE 720
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct vpu_control_extended {
 __u32 type;
 __u32 data_len;
 void __user *data_ptr;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 buf_size;
 void __user *buf_ptr;
};
struct vpu_control_port {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 control_id;
 __u32 port;
 union control_port_data {
 __u32 framerate;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 } data;
};
#define VPU_CTRL_FPS 1000
#define VPU_QUERY_SESSIONS _IOR('V', (BASE_VIDIOC_PRIVATE + 0), int)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define VPU_ATTACH_TO_SESSION _IOW('V', (BASE_VIDIOC_PRIVATE + 1), int)
#define VPU_CREATE_SESSION _IOR('V', (BASE_VIDIOC_PRIVATE + 2), int)
#define VPU_COMMIT_CONFIGURATION _IO('V', (BASE_VIDIOC_PRIVATE + 10))
#define VPU_FLUSH_BUFS _IOW('V', (BASE_VIDIOC_PRIVATE + 15),   enum v4l2_buf_type)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define VPU_G_CONTROL _IOWR('V', (BASE_VIDIOC_PRIVATE + 20),   struct vpu_control)
#define VPU_S_CONTROL _IOW('V', (BASE_VIDIOC_PRIVATE + 21),   struct vpu_control)
#define VPU_G_CONTROL_EXTENDED _IOWR('V', (BASE_VIDIOC_PRIVATE + 22),   struct vpu_control_extended)
#define VPU_S_CONTROL_EXTENDED _IOW('V', (BASE_VIDIOC_PRIVATE + 23),   struct vpu_control_extended)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define VPU_G_CONTROL_PORT _IOWR('V', (BASE_VIDIOC_PRIVATE + 24),   struct vpu_control_port)
#define VPU_S_CONTROL_PORT _IOW('V', (BASE_VIDIOC_PRIVATE + 25),   struct vpu_control_port)
#endif
