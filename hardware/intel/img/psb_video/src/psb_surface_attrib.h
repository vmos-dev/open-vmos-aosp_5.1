/*
 * Copyright (c) 2011 Intel Corporation. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef _PSB_SURFACE_ATTRIB_H_
#define _PSB_SURFACE_ATTRIB_H_

#include <va/va_tpi.h>
#include "psb_drv_video.h"
#include "psb_surface.h"

/*
 * Create surface from virtual address
 * flags: 0 indicates cache, PSB_USER_BUFFER_UNCACHED, PSB_USER_BUFFER_WC
 */
VAStatus psb_surface_create_from_ub(
    psb_driver_data_p driver_data,
    int width, int height, int fourcc, VASurfaceAttributeTPI *graphic_buffers,
    psb_surface_p psb_surface, /* out */
    void *vaddr,
    unsigned int flags
);

VAStatus psb_surface_create_for_userptr(
    psb_driver_data_p driver_data,
    int width, int height,
    unsigned size, /* total buffer size need to be allocated */
    unsigned int fourcc, /* expected fourcc */
    unsigned int luma_stride, /* luma stride, could be width aligned with a special value */
    unsigned int chroma_u_stride, /* chroma stride */
    unsigned int chroma_v_stride,
    unsigned int luma_offset, /* could be 0 */
    unsigned int chroma_u_offset, /* UV offset from the beginning of the memory */
    unsigned int chroma_v_offset,
    psb_surface_p psb_surface /* out */
);

VAStatus psb_surface_create_from_kbuf(
    psb_driver_data_p driver_data,
    int width, int height,
    unsigned size, /* total buffer size need to be allocated */
    unsigned int fourcc, /* expected fourcc */
    int kbuf_handle, /*kernel handle */
    unsigned int luma_stride, /* luma stride, could be width aligned with a special value */
    unsigned int chroma_u_stride, /* chroma stride */
    unsigned int chroma_v_stride,
    unsigned int luma_offset, /* could be 0 */
    unsigned int chroma_u_offset, /* UV offset from the beginning of the memory */
    unsigned int chroma_v_offset,
    psb_surface_p psb_surface /* out */
);


VAStatus psb_surface_create_camera(
    psb_driver_data_p driver_data,
    int width, int height, int stride, int size,
    psb_surface_p psb_surface, /* out */
    int is_v4l2,
    unsigned int id_or_ofs
);

/* id_or_ofs: it is frame ID or frame offset in camear device memory
 *     for CI frame: it it always frame offset currently
 *     for v4l2 buf: it is offset used in V4L2 buffer mmap
 * user_ptr: virtual address of user buffer.
 */
VAStatus psb_surface_create_camera_from_ub(
    psb_driver_data_p driver_data,
    int width, int height, int stride, int size,
    psb_surface_p psb_surface, /* out */
    int is_v4l2,
    unsigned int id_or_ofs,
    const unsigned long *user_ptr
);

#ifdef ANDROID

VAStatus psb_DestroySurfaceGralloc(object_surface_p obj_surface);


VAStatus psb_CreateSurfacesFromGralloc(
    VADriverContextP ctx,
    int width,
    int height,
    int format,
    int num_surfaces,
    VASurfaceID *surface_list,        /* out */
    PsbSurfaceAttributeTPI *attribute_tpi
);

#else

static VAStatus psb_DestroySurfaceGralloc(object_surface_p obj_surface)
{
    return VA_STATUS_ERROR_INVALID_PARAMETER;
}

static VAStatus psb_CreateSurfacesFromGralloc(
    VADriverContextP ctx,
    int width,
    int height,
    int format,
    int num_surfaces,
    VASurfaceID *surface_list,        /* out */
    PsbSurfaceAttributeTPI *attribute_tpi
)
{
    return VA_STATUS_ERROR_INVALID_PARAMETER;
}

#endif


VAStatus psb_CreateSurfacesWithAttribute(
    VADriverContextP ctx,
    int width,
    int height,
    int format,
    int num_surfaces,
    VASurfaceID *surface_list,        /* out */
    VASurfaceAttributeTPI *attribute_tpi
);

#endif

