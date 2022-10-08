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
#ifndef TTM_FENCE_USER_H
#define TTM_FENCE_USER_H
#ifndef _KERNEL
#include <stdint.h>
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#endif
#define TTM_FENCE_MAJOR 0
#define TTM_FENCE_MINOR 1
#define TTM_FENCE_PL 0
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TTM_FENCE_DATE "080819"
struct ttm_fence_signaled_req {
 uint32_t handle;
 uint32_t fence_type;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 int32_t flush;
 uint32_t pad64;
};
struct ttm_fence_rep {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t signaled_types;
 uint32_t fence_error;
};
union ttm_fence_signaled_arg {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct ttm_fence_signaled_req req;
 struct ttm_fence_rep rep;
};
#define TTM_FENCE_FINISH_MODE_LAZY (1 << 0)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TTM_FENCE_FINISH_MODE_NO_BLOCK (1 << 1)
struct ttm_fence_finish_req {
 uint32_t handle;
 uint32_t fence_type;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t mode;
 uint32_t pad64;
};
union ttm_fence_finish_arg {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct ttm_fence_finish_req req;
 struct ttm_fence_rep rep;
};
struct ttm_fence_unref_arg {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 uint32_t handle;
 uint32_t pad64;
};
#define TTM_FENCE_SIGNALED 0x01
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TTM_FENCE_FINISH 0x02
#define TTM_FENCE_UNREF 0x03
#endif

