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
#ifndef __MEDIA_INFO_H__
#define __MEDIA_INFO_H__
#ifndef MSM_MEDIA_ALIGN
#define MSM_MEDIA_ALIGN(__sz, __align) (((__sz) + (__align-1)) & (~(__align-1)))
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#endif
enum color_fmts {
 COLOR_FMT_NV12,
 COLOR_FMT_NV21,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#define VENUS_Y_STRIDE(_color_fmt, _width) MSM_MEDIA_ALIGN(_width, 128)
#define VENUS_UV_STRIDE(_color_fmt, _width) MSM_MEDIA_ALIGN(_width, 128)
#define VENUS_Y_SCANLINES(_color_fmt, _width) MSM_MEDIA_ALIGN(_width, 32)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define VENUS_UV_SCANLINES(_color_fmt, _width) MSM_MEDIA_ALIGN(_width, 16)
#define VENUS_BUFFER_SIZE_UNALIGNED(_color_fmt, _width, _height)   ((VENUS_Y_STRIDE(_color_fmt, _width) * VENUS_Y_SCANLINES(_color_fmt, _height)) +   (VENUS_UV_STRIDE(_color_fmt, _width) * VENUS_UV_SCANLINES(_color_fmt, _height) + 4096))
#define VENUS_BUFFER_SIZE(_color_fmt, _width, _height)   MSM_MEDIA_ALIGN(VENUS_BUFFER_SIZE_UNALIGNED(_color_fmt, _width, _height),4096)
#endif
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */

