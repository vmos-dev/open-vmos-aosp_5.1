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
#ifndef BVENTRY_H
#define BVENTRY_H
struct bvbuffdesc;
struct bvbltparams;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct bvcopparams;
typedef enum bverror (*BVFN_MAP) (struct bvbuffdesc *buffdesc);
typedef enum bverror (*BVFN_UNMAP) (struct bvbuffdesc *buffdesc);
typedef enum bverror (*BVFN_BLT) (struct bvbltparams *bltparams);
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
typedef enum bverror (*BVFN_CACHE)(struct bvcopparams *copparams);
struct bventry {
 unsigned int structsize;
 BVFN_MAP bv_map;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 BVFN_UNMAP bv_unmap;
 BVFN_BLT bv_blt;
 BVFN_CACHE bv_cache;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#endif
