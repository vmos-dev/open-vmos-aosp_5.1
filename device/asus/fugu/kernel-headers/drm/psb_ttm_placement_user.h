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
#ifndef _TTM_PLACEMENT_USER_H_
#define _TTM_PLACEMENT_USER_H_
#ifndef _KERNEL
#include <stdint.h>
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#else
#include <linux/kernel.h>
#endif
#include "ttm/ttm_placement.h"
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TTM_PLACEMENT_MAJOR 0
#define TTM_PLACEMENT_MINOR 1
#define TTM_PLACEMENT_PL 0
#define TTM_PLACEMENT_DATE "080819"
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct ttm_pl_create_req {
 uint64_t size;
 uint32_t placement;
 uint32_t page_alignment;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct ttm_pl_create_ub_req {
 uint64_t size;
 uint64_t user_address;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t placement;
 uint32_t page_alignment;
};
struct ttm_pl_rep {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint64_t gpu_offset;
 uint64_t bo_size;
 uint64_t map_handle;
 uint32_t placement;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t handle;
 uint32_t sync_object_arg;
 uint32_t pad64;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct ttm_pl_setstatus_req {
 uint32_t set_placement;
 uint32_t clr_placement;
 uint32_t handle;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t pad64;
};
struct ttm_pl_reference_req {
 uint32_t handle;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t pad64;
};
#define TTM_PL_SYNCCPU_MODE_READ TTM_ACCESS_READ
#define TTM_PL_SYNCCPU_MODE_WRITE TTM_ACCESS_WRITE
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TTM_PL_SYNCCPU_MODE_NO_BLOCK (1 << 2)
#define TTM_PL_SYNCCPU_MODE_TRYCACHED (1 << 3)
struct ttm_pl_synccpu_arg {
 uint32_t handle;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t access_mode;
 enum {
 TTM_PL_SYNCCPU_OP_GRAB,
 TTM_PL_SYNCCPU_OP_RELEASE
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 } op;
 uint32_t pad64;
};
#define TTM_PL_WAITIDLE_MODE_LAZY (1 << 0)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TTM_PL_WAITIDLE_MODE_NO_BLOCK (1 << 1)
struct ttm_pl_waitidle_arg {
 uint32_t handle;
 uint32_t mode;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
union ttm_pl_create_arg {
 struct ttm_pl_create_req req;
 struct ttm_pl_rep rep;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
union ttm_pl_reference_arg {
 struct ttm_pl_reference_req req;
 struct ttm_pl_rep rep;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
union ttm_pl_setstatus_arg {
 struct ttm_pl_setstatus_req req;
 struct ttm_pl_rep rep;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
union ttm_pl_create_ub_arg {
 struct ttm_pl_create_ub_req req;
 struct ttm_pl_rep rep;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#define TTM_PL_CREATE 0x00
#define TTM_PL_REFERENCE 0x01
#define TTM_PL_UNREF 0x02
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TTM_PL_SYNCCPU 0x03
#define TTM_PL_WAITIDLE 0x04
#define TTM_PL_SETSTATUS 0x05
#define TTM_PL_CREATE_UB 0x06
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#endif

