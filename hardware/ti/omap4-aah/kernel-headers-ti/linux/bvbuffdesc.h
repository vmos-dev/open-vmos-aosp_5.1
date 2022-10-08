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
#ifndef BVBUFFDESC_H
#define BVBUFFDESC_H
struct bvbuffmap;
#define BVATDEF_VENDOR_SHIFT 24
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define BVATDEF_VENDOR_MASK (0xFF << BVATDEF_VENDOR_SHIFT)
#define BVATDEF_VENDOR_ALL (0x00 << BVATDEF_VENDOR_SHIFT)
#define BVATDEF_VENDOR_TI (0x01 << BVATDEF_VENDOR_SHIFT)
enum bvauxtype {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 BVAT_NONE = 0,
 BVAT_PHYSDESC =
 BVATDEF_VENDOR_ALL + 1,
#ifdef BVAT_EXTERNAL_INCLUDE
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#include BVAT_EXTERNAL_INCLUDE
#endif
};
struct bvphysdesc {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 unsigned int structsize;
 unsigned long pagesize;
 unsigned long *pagearray;
 unsigned int pagecount;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 unsigned long pageoffset;
};
struct bvbuffdesc {
 unsigned int structsize;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 void *virtaddr;
 unsigned long length;
 struct bvbuffmap *map;
 enum bvauxtype auxtype;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 void *auxptr;
};
#endif
