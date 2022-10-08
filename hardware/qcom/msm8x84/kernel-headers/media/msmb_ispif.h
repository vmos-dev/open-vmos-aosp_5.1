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
#ifndef MSM_CAM_ISPIF_H
#define MSM_CAM_ISPIF_H
#define CSID_VERSION_V20 0x02000011
#define CSID_VERSION_V22 0x02001000
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define CSID_VERSION_V30 0x30000000
#define CSID_VERSION_V3 0x30000000
enum msm_ispif_vfe_intf {
 VFE0,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 VFE1,
 VFE_MAX
};
#define VFE0_MASK (1 << VFE0)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define VFE1_MASK (1 << VFE1)
enum msm_ispif_intftype {
 PIX0,
 RDI0,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 PIX1,
 RDI1,
 RDI2,
 INTF_MAX
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#define MAX_PARAM_ENTRIES (INTF_MAX * 2)
#define MAX_CID_CH 8
#define PIX0_MASK (1 << PIX0)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define PIX1_MASK (1 << PIX1)
#define RDI0_MASK (1 << RDI0)
#define RDI1_MASK (1 << RDI1)
#define RDI2_MASK (1 << RDI2)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum msm_ispif_vc {
 VC0,
 VC1,
 VC2,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 VC3,
 VC_MAX
};
enum msm_ispif_cid {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 CID0,
 CID1,
 CID2,
 CID3,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 CID4,
 CID5,
 CID6,
 CID7,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 CID8,
 CID9,
 CID10,
 CID11,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 CID12,
 CID13,
 CID14,
 CID15,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 CID_MAX
};
enum msm_ispif_csid {
 CSID0,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 CSID1,
 CSID2,
 CSID3,
 CSID_MAX
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct msm_ispif_params_entry {
 enum msm_ispif_vfe_intf vfe_intf;
 enum msm_ispif_intftype intftype;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 int num_cids;
 enum msm_ispif_cid cids[3];
 enum msm_ispif_csid csid;
 int crop_enable;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint16_t crop_start_pixel;
 uint16_t crop_end_pixel;
};
struct msm_ispif_param_data {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t num;
 struct msm_ispif_params_entry entries[MAX_PARAM_ENTRIES];
};
struct msm_isp_info {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t max_resolution;
 uint32_t id;
 uint32_t ver;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct msm_ispif_vfe_info {
 int num_vfe;
 struct msm_isp_info info[VFE_MAX];
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum ispif_cfg_type_t {
 ISPIF_CLK_ENABLE,
 ISPIF_CLK_DISABLE,
 ISPIF_INIT,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 ISPIF_CFG,
 ISPIF_START_FRAME_BOUNDARY,
 ISPIF_STOP_FRAME_BOUNDARY,
 ISPIF_STOP_IMMEDIATELY,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 ISPIF_RELEASE,
 ISPIF_ENABLE_REG_DUMP,
 ISPIF_SET_VFE_INFO,
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct ispif_cfg_data {
 enum ispif_cfg_type_t cfg_type;
 union {
 int reg_dump;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t csid_version;
 struct msm_ispif_vfe_info vfe_info;
 struct msm_ispif_param_data params;
 };
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#define VIDIOC_MSM_ISPIF_CFG   _IOWR('V', BASE_VIDIOC_PRIVATE, struct ispif_cfg_data)
#endif
