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
#ifndef _UAPI_MEDIA_MSMB_ISP_H
#define _UAPI_MEDIA_MSMB_ISP_H
#include <linux/videodev2.h>
#define MAX_PLANES_PER_STREAM 3
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define MAX_NUM_STREAM 7
#define ISP_VERSION_40 40
#define ISP_VERSION_32 32
#define ISP_NATIVE_BUF_BIT 0x10000
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define ISP0_BIT 0x20000
#define ISP1_BIT 0x40000
#define ISP_STATS_STREAM_BIT 0x80000000
enum ISP_START_PIXEL_PATTERN {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 ISP_BAYER_RGRGRG,
 ISP_BAYER_GRGRGR,
 ISP_BAYER_BGBGBG,
 ISP_BAYER_GBGBGB,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 ISP_YUV_YCbYCr,
 ISP_YUV_YCrYCb,
 ISP_YUV_CbYCrY,
 ISP_YUV_CrYCbY,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 ISP_PIX_PATTERN_MAX
};
enum msm_vfe_plane_fmt {
 Y_PLANE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 CB_PLANE,
 CR_PLANE,
 CRCB_PLANE,
 CBCR_PLANE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 VFE_PLANE_FMT_MAX
};
enum msm_vfe_input_src {
 VFE_PIX_0,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 VFE_RAW_0,
 VFE_RAW_1,
 VFE_RAW_2,
 VFE_SRC_MAX,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
enum msm_vfe_axi_stream_src {
 PIX_ENCODER,
 PIX_VIEWFINDER,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 CAMIF_RAW,
 IDEAL_RAW,
 RDI_INTF_0,
 RDI_INTF_1,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 RDI_INTF_2,
 VFE_AXI_SRC_MAX
};
enum msm_vfe_frame_skip_pattern {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 NO_SKIP,
 EVERY_2FRAME,
 EVERY_3FRAME,
 EVERY_4FRAME,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 EVERY_5FRAME,
 EVERY_6FRAME,
 EVERY_7FRAME,
 EVERY_8FRAME,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 EVERY_16FRAME,
 EVERY_32FRAME,
 MAX_SKIP,
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum msm_vfe_camif_input {
 CAMIF_DISABLED,
 CAMIF_PAD_REG_INPUT,
 CAMIF_MIDDI_INPUT,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 CAMIF_MIPI_INPUT,
};
struct msm_vfe_camif_cfg {
 uint32_t lines_per_frame;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t pixels_per_line;
 uint32_t first_pixel;
 uint32_t last_pixel;
 uint32_t first_line;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t last_line;
 uint32_t epoch_line0;
 uint32_t epoch_line1;
 enum msm_vfe_camif_input camif_input;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
enum msm_vfe_inputmux {
 CAMIF,
 TESTGEN,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 EXTERNAL_READ,
};
struct msm_vfe_pix_cfg {
 struct msm_vfe_camif_cfg camif_cfg;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 enum msm_vfe_inputmux input_mux;
 enum ISP_START_PIXEL_PATTERN pixel_pattern;
};
struct msm_vfe_rdi_cfg {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint8_t cid;
 uint8_t frame_based;
};
struct msm_vfe_input_cfg {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 union {
 struct msm_vfe_pix_cfg pix_cfg;
 struct msm_vfe_rdi_cfg rdi_cfg;
 } d;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 enum msm_vfe_input_src input_src;
 uint32_t input_pix_clk;
};
struct msm_vfe_axi_plane_cfg {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t output_width;
 uint32_t output_height;
 uint32_t output_stride;
 uint32_t output_scan_lines;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t output_plane_format;
 uint32_t plane_addr_offset;
 uint8_t csid_src;
 uint8_t rdi_cid;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct msm_vfe_axi_stream_request_cmd {
 uint32_t session_id;
 uint32_t stream_id;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t output_format;
 enum msm_vfe_axi_stream_src stream_src;
 struct msm_vfe_axi_plane_cfg plane_cfg[MAX_PLANES_PER_STREAM];
 uint32_t burst_count;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t hfr_mode;
 uint8_t frame_base;
 uint32_t init_frame_drop;
 enum msm_vfe_frame_skip_pattern frame_skip_pattern;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint8_t buf_divert;
 uint32_t axi_stream_handle;
};
struct msm_vfe_axi_stream_release_cmd {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t stream_handle;
};
enum msm_vfe_axi_stream_cmd {
 STOP_STREAM,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 START_STREAM,
};
struct msm_vfe_axi_stream_cfg_cmd {
 uint8_t num_streams;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t stream_handle[MAX_NUM_STREAM];
 enum msm_vfe_axi_stream_cmd cmd;
};
enum msm_vfe_axi_stream_update_type {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 ENABLE_STREAM_BUF_DIVERT,
 DISABLE_STREAM_BUF_DIVERT,
 UPDATE_STREAM_FRAMEDROP_PATTERN,
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct msm_vfe_axi_stream_update_cmd {
 uint32_t stream_handle;
 enum msm_vfe_axi_stream_update_type update_type;
 enum msm_vfe_frame_skip_pattern skip_pattern;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
enum msm_isp_stats_type {
 MSM_ISP_STATS_AEC,
 MSM_ISP_STATS_AF,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 MSM_ISP_STATS_AWB,
 MSM_ISP_STATS_RS,
 MSM_ISP_STATS_CS,
 MSM_ISP_STATS_IHIST,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 MSM_ISP_STATS_SKIN,
 MSM_ISP_STATS_BG,
 MSM_ISP_STATS_BF,
 MSM_ISP_STATS_BE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 MSM_ISP_STATS_BHIST,
 MSM_ISP_STATS_MAX
};
struct msm_vfe_stats_stream_request_cmd {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t session_id;
 uint32_t stream_id;
 enum msm_isp_stats_type stats_type;
 uint32_t composite_flag;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t framedrop_pattern;
 uint32_t irq_subsample_pattern;
 uint32_t buffer_offset;
 uint32_t stream_handle;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct msm_vfe_stats_stream_release_cmd {
 uint32_t stream_handle;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct msm_vfe_stats_stream_cfg_cmd {
 uint8_t num_streams;
 uint32_t stream_handle[MSM_ISP_STATS_MAX];
 uint8_t enable;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
enum msm_vfe_reg_cfg_type {
 VFE_WRITE,
 VFE_WRITE_MB,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 VFE_READ,
 VFE_CFG_MASK,
 VFE_WRITE_DMI_16BIT,
 VFE_WRITE_DMI_32BIT,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 VFE_WRITE_DMI_64BIT,
 VFE_READ_DMI_16BIT,
 VFE_READ_DMI_32BIT,
 VFE_READ_DMI_64BIT,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct msm_vfe_cfg_cmd2 {
 uint16_t num_cfg;
 uint16_t cmd_len;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 void __user *cfg_data;
 void __user *cfg_cmd;
};
struct msm_vfe_reg_rw_info {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t reg_offset;
 uint32_t cmd_data_offset;
 uint32_t len;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct msm_vfe_reg_mask_info {
 uint32_t reg_offset;
 uint32_t mask;
 uint32_t val;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct msm_vfe_reg_dmi_info {
 uint32_t hi_tbl_offset;
 uint32_t lo_tbl_offset;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t len;
};
struct msm_vfe_reg_cfg_cmd {
 union {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct msm_vfe_reg_rw_info rw_info;
 struct msm_vfe_reg_mask_info mask_info;
 struct msm_vfe_reg_dmi_info dmi_info;
 } u;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 enum msm_vfe_reg_cfg_type cmd_type;
};
enum msm_isp_buf_type {
 ISP_PRIVATE_BUF,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 ISP_SHARE_BUF,
 MAX_ISP_BUF_TYPE,
};
struct msm_isp_buf_request {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t session_id;
 uint32_t stream_id;
 uint8_t num_buf;
 uint32_t handle;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 enum msm_isp_buf_type buf_type;
};
struct msm_isp_qbuf_info {
 uint32_t handle;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 int buf_idx;
 struct v4l2_buffer buffer;
 uint32_t dirty_buf;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct msm_vfe_axi_src_state {
 enum msm_vfe_input_src input_src;
 uint32_t src_active;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum msm_isp_event_idx {
 ISP_REG_UPDATE = 0,
 ISP_START_ACK = 1,
 ISP_STOP_ACK = 2,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 ISP_IRQ_VIOLATION = 3,
 ISP_WM_BUS_OVERFLOW = 4,
 ISP_STATS_OVERFLOW = 5,
 ISP_CAMIF_ERROR = 6,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 ISP_SOF = 7,
 ISP_EOF = 8,
 ISP_EVENT_MAX = 9
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define ISP_EVENT_OFFSET 8
#define ISP_EVENT_BASE (V4L2_EVENT_PRIVATE_START)
#define ISP_BUF_EVENT_BASE (ISP_EVENT_BASE + (1 << ISP_EVENT_OFFSET))
#define ISP_STATS_EVENT_BASE (ISP_EVENT_BASE + (2 << ISP_EVENT_OFFSET))
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define ISP_EVENT_REG_UPDATE (ISP_EVENT_BASE + ISP_REG_UPDATE)
#define ISP_EVENT_START_ACK (ISP_EVENT_BASE + ISP_START_ACK)
#define ISP_EVENT_STOP_ACK (ISP_EVENT_BASE + ISP_STOP_ACK)
#define ISP_EVENT_IRQ_VIOLATION (ISP_EVENT_BASE + ISP_IRQ_VIOLATION)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define ISP_EVENT_WM_BUS_OVERFLOW (ISP_EVENT_BASE + ISP_WM_BUS_OVERFLOW)
#define ISP_EVENT_STATS_OVERFLOW (ISP_EVENT_BASE + ISP_STATS_OVERFLOW)
#define ISP_EVENT_CAMIF_ERROR (ISP_EVENT_BASE + ISP_CAMIF_ERROR)
#define ISP_EVENT_SOF (ISP_EVENT_BASE + ISP_SOF)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define ISP_EVENT_EOF (ISP_EVENT_BASE + ISP_EOF)
#define ISP_EVENT_BUF_DIVERT (ISP_BUF_EVENT_BASE)
#define ISP_EVENT_STATS_NOTIFY (ISP_STATS_EVENT_BASE)
#define ISP_EVENT_COMP_STATS_NOTIFY (ISP_EVENT_STATS_NOTIFY + MSM_ISP_STATS_MAX)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct msm_isp_buf_event {
 uint32_t session_id;
 uint32_t stream_id;
 uint32_t handle;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 int8_t buf_idx;
};
struct msm_isp_stats_event {
 uint32_t stats_mask;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint8_t stats_buf_idxs[MSM_ISP_STATS_MAX];
};
struct msm_isp_stream_ack {
 uint32_t session_id;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t stream_id;
 uint32_t handle;
};
struct msm_isp_event_data {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct timeval timestamp;
 struct timeval mono_timestamp;
 uint32_t frame_id;
 union {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct msm_isp_stream_ack stream_ack;
 enum msm_vfe_input_src input_src;
 struct msm_isp_stats_event stats;
 uint32_t irq_status_mask;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct msm_isp_buf_event buf_done;
 } u;
};
#define V4L2_PIX_FMT_QBGGR8 v4l2_fourcc('Q', 'B', 'G', '8')
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define V4L2_PIX_FMT_QGBRG8 v4l2_fourcc('Q', 'G', 'B', '8')
#define V4L2_PIX_FMT_QGRBG8 v4l2_fourcc('Q', 'G', 'R', '8')
#define V4L2_PIX_FMT_QRGGB8 v4l2_fourcc('Q', 'R', 'G', '8')
#define V4L2_PIX_FMT_QBGGR10 v4l2_fourcc('Q', 'B', 'G', '0')
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define V4L2_PIX_FMT_QGBRG10 v4l2_fourcc('Q', 'G', 'B', '0')
#define V4L2_PIX_FMT_QGRBG10 v4l2_fourcc('Q', 'G', 'R', '0')
#define V4L2_PIX_FMT_QRGGB10 v4l2_fourcc('Q', 'R', 'G', '0')
#define V4L2_PIX_FMT_QBGGR12 v4l2_fourcc('Q', 'B', 'G', '2')
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define V4L2_PIX_FMT_QGBRG12 v4l2_fourcc('Q', 'G', 'B', '2')
#define V4L2_PIX_FMT_QGRBG12 v4l2_fourcc('Q', 'G', 'R', '2')
#define V4L2_PIX_FMT_QRGGB12 v4l2_fourcc('Q', 'R', 'G', '2')
#define VIDIOC_MSM_VFE_REG_CFG   _IOWR('V', BASE_VIDIOC_PRIVATE, struct msm_vfe_cfg_cmd2)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define VIDIOC_MSM_ISP_REQUEST_BUF   _IOWR('V', BASE_VIDIOC_PRIVATE+1, struct msm_isp_buf_request)
#define VIDIOC_MSM_ISP_ENQUEUE_BUF   _IOWR('V', BASE_VIDIOC_PRIVATE+2, struct msm_isp_qbuf_info)
#define VIDIOC_MSM_ISP_RELEASE_BUF   _IOWR('V', BASE_VIDIOC_PRIVATE+3, struct msm_isp_buf_request)
#define VIDIOC_MSM_ISP_REQUEST_STREAM   _IOWR('V', BASE_VIDIOC_PRIVATE+4, struct msm_vfe_axi_stream_request_cmd)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define VIDIOC_MSM_ISP_CFG_STREAM   _IOWR('V', BASE_VIDIOC_PRIVATE+5, struct msm_vfe_axi_stream_cfg_cmd)
#define VIDIOC_MSM_ISP_RELEASE_STREAM   _IOWR('V', BASE_VIDIOC_PRIVATE+6, struct msm_vfe_axi_stream_release_cmd)
#define VIDIOC_MSM_ISP_INPUT_CFG   _IOWR('V', BASE_VIDIOC_PRIVATE+7, struct msm_vfe_input_cfg)
#define VIDIOC_MSM_ISP_SET_SRC_STATE   _IOWR('V', BASE_VIDIOC_PRIVATE+8, struct msm_vfe_axi_src_state)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define VIDIOC_MSM_ISP_REQUEST_STATS_STREAM   _IOWR('V', BASE_VIDIOC_PRIVATE+9,   struct msm_vfe_stats_stream_request_cmd)
#define VIDIOC_MSM_ISP_CFG_STATS_STREAM   _IOWR('V', BASE_VIDIOC_PRIVATE+10, struct msm_vfe_stats_stream_cfg_cmd)
#define VIDIOC_MSM_ISP_RELEASE_STATS_STREAM   _IOWR('V', BASE_VIDIOC_PRIVATE+11,   struct msm_vfe_stats_stream_release_cmd)
#define VIDIOC_MSM_ISP_CFG_STATS_COMP_POLICY   _IOWR('V', BASE_VIDIOC_PRIVATE+12,   struct msm_vfe_stats_comp_policy_cfg)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define VIDIOC_MSM_ISP_UPDATE_STREAM   _IOWR('V', BASE_VIDIOC_PRIVATE+13, struct msm_vfe_axi_stream_update_cmd)
#endif
