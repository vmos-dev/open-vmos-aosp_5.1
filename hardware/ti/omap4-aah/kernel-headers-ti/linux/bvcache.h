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
#ifndef BVCACHE_H_
#define BVCACHE_H_
struct bvbuffdesc;
struct bvsurfgeom;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct bvrect;
enum bvcacheop {
 BVCACHE_BIDIRECTIONAL = 0,
 BVCACHE_CPU_TO_DEVICE = 1,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 BVCACHE_CPU_FROM_DEVICE = 2,
 BVCACHE_RESERVED3 = 3,
};
struct bvcopparams {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 unsigned int structsize;
 struct bvbuffdesc *desc;
 struct bvsurfgeom *geom;
 struct bvrect *rect;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 enum bvcacheop cacheop;
};
#endif
