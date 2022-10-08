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
#ifndef _LINUX_OMAP_HWC_H
#define _LINUX_OMAP_HWC_H
#define HWC_BLT_DESC_FLAG 0x80000000
#define HWC_BLT_DESC_FB 0x40000000
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define HWC_BLT_DESC_FB_FN(ovlno)   (HWC_BLT_DESC_FLAG | HWC_BLT_DESC_FB | (ovlno))
#define HWC_BLT_FLAG_USE_FB (1 << 0)
struct rgz_blt_entry {
 struct bvbltparams bp;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct bvsurfgeom dstgeom;
 struct bvsurfgeom src1geom;
 struct bvbuffdesc src1desc;
 struct bvsurfgeom src2geom;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct bvbuffdesc src2desc;
};
struct omap_hwc_blit_data {
 __u16 rgz_flags;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u16 rgz_items;
 struct rgz_blt_entry rgz_blts[0];
};
struct omap_hwc_data {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct dsscomp_setup_dispc_data dsscomp_data;
 struct omap_hwc_blit_data blit_data;
};
#endif
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
