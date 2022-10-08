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
#ifndef _TTM_PLACEMENT_H_
#define _TTM_PLACEMENT_H_
#define TTM_PL_SYSTEM 0
#define TTM_PL_TT 1
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TTM_PL_VRAM 2
#define TTM_PL_PRIV0 3
#define TTM_PL_PRIV1 4
#define TTM_PL_PRIV2 5
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TTM_PL_PRIV3 6
#define TTM_PL_PRIV4 7
#define TTM_PL_PRIV5 8
#define TTM_PL_SWAPPED 15
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TTM_PL_FLAG_SYSTEM (1 << TTM_PL_SYSTEM)
#define TTM_PL_FLAG_TT (1 << TTM_PL_TT)
#define TTM_PL_FLAG_VRAM (1 << TTM_PL_VRAM)
#define TTM_PL_FLAG_PRIV0 (1 << TTM_PL_PRIV0)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TTM_PL_FLAG_PRIV1 (1 << TTM_PL_PRIV1)
#define TTM_PL_FLAG_PRIV2 (1 << TTM_PL_PRIV2)
#define TTM_PL_FLAG_PRIV3 (1 << TTM_PL_PRIV3)
#define TTM_PL_FLAG_PRIV4 (1 << TTM_PL_PRIV4)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TTM_PL_FLAG_PRIV5 (1 << TTM_PL_PRIV5)
#define TTM_PL_FLAG_SWAPPED (1 << TTM_PL_SWAPPED)
#define TTM_PL_MASK_MEM 0x0000FFFF
#define TTM_PL_FLAG_CACHED (1 << 16)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TTM_PL_FLAG_UNCACHED (1 << 17)
#define TTM_PL_FLAG_WC (1 << 18)
#define TTM_PL_FLAG_SHARED (1 << 20)
#define TTM_PL_FLAG_NO_EVICT (1 << 21)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TTM_PL_MASK_CACHING (TTM_PL_FLAG_CACHED |   TTM_PL_FLAG_UNCACHED |   TTM_PL_FLAG_WC)
#define TTM_PL_MASK_MEMTYPE (TTM_PL_MASK_MEM | TTM_PL_MASK_CACHING)
#define TTM_ACCESS_READ (1 << 0)
#define TTM_ACCESS_WRITE (1 << 1)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#endif

