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
#define CSID_VERSION_V2 0x02000011
#define CSID_VERSION_V3 0x30000000
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum msm_ispif_vfe_intf {
 VFE0,
 VFE1,
 VFE_MAX
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#define VFE0_MASK (1 << VFE0)
#define VFE1_MASK (1 << VFE1)
enum msm_ispif_intftype {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 PIX0,
 RDI0,
 PIX1,
 RDI1,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 RDI2,
 INTF_MAX
};
#define MAX_PARAM_ENTRIES (INTF_MAX * 2)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define PIX0_MASK (1 << PIX0)
#define PIX1_MASK (1 << PIX1)
#define RDI0_MASK (1 << RDI0)
#define RDI1_MASK (1 << RDI1)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define RDI2_MASK (1 << RDI2)
enum msm_ispif_vc {
 VC0,
 VC1,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 VC2,
 VC3,
 VC_MAX
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum msm_ispif_cid {
 CID0,
 CID1,
 CID2,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 CID3,
 CID4,
 CID5,
 CID6,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 CID7,
 CID8,
 CID9,
 CID10,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 CID11,
 CID12,
 CID13,
 CID14,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 CID15,
 CID_MAX
};
enum msm_ispif_csid {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 CSID0,
 CSID1,
 CSID2,
 CSID3,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 CSID_MAX
};
struct msm_ispif_params_entry {
 enum msm_ispif_vfe_intf vfe_intf;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 enum msm_ispif_intftype intftype;
 int num_cids;
 enum msm_ispif_cid cids[3];
 enum msm_ispif_csid csid;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 int crop_enable;
 uint16_t crop_start_pixel;
 uint16_t crop_end_pixel;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct msm_ispif_param_data {
 uint32_t num;
 struct msm_ispif_params_entry entries[MAX_PARAM_ENTRIES];
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct msm_isp_info {
 uint32_t max_resolution;
 uint32_t id;
 uint32_t ver;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct msm_ispif_vfe_info {
 int num_vfe;
 struct msm_isp_info info[VFE_MAX];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
enum ispif_cfg_type_t {
 ISPIF_CLK_ENABLE,
 ISPIF_CLK_DISABLE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 ISPIF_INIT,
 ISPIF_CFG,
 ISPIF_START_FRAME_BOUNDARY,
 ISPIF_STOP_FRAME_BOUNDARY,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 ISPIF_STOP_IMMEDIATELY,
 ISPIF_RELEASE,
 ISPIF_ENABLE_REG_DUMP,
 ISPIF_SET_VFE_INFO,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct ispif_cfg_data {
 enum ispif_cfg_type_t cfg_type;
 union {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 int reg_dump;
 uint32_t csid_version;
 struct msm_ispif_vfe_info vfe_info;
 struct msm_ispif_param_data params;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 };
};
#define VIDIOC_MSM_ISPIF_CFG   _IOWR('V', BASE_VIDIOC_PRIVATE, struct ispif_cfg_data)
#endif
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
