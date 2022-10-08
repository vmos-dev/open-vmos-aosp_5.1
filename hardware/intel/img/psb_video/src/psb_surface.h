/*
 * Copyright (c) 2011 Intel Corporation. All Rights Reserved.
 * Copyright (c) Imagination Technologies Limited, UK
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
 * Authors:
 *    Waldo Bastian <waldo.bastian@intel.com>
 */

#ifndef _PSB_SURFACE_H_
#define _PSB_SURFACE_H_

#include <va/va.h>
#include <va/va_tpi.h>
#include "psb_buffer.h"
//#include "xf86mm.h"

/* MSVDX specific */
typedef enum {
    STRIDE_352  = 0,
    STRIDE_720  = 1,
    STRIDE_1280 = 2,
    STRIDE_1920 = 3,
    STRIDE_512  = 4,
    STRIDE_1024 = 5,
    STRIDE_2048 = 6,
    STRIDE_4096 = 7,
    STRIDE_NA,
    STRIDE_UNDEFINED,
} psb_surface_stride_t;

typedef enum {
    IS_PROTECTED = 0x1,
    IS_ROTATED   = 0x2,
} psb_surface_flags_t;

typedef struct psb_surface_s *psb_surface_p;

struct psb_surface_s {
    struct psb_buffer_s buf;
    struct psb_buffer_s *in_loop_buf;
    struct psb_buffer_s *ref_buf;
    psb_surface_stride_t stride_mode;
    unsigned int stride;
    unsigned int luma_offset;
    unsigned int chroma_offset;
    /* Used to store driver private data, e.g. decoder specific intermediate status data
     * extra_info[0-3]: used for decode
     * extra_info[4]: surface fourcc
     * extra_info[5]: surface skippeded or not for encode, rotate info for decode
     * extra_info[6]: mfld protected surface
     * extra_info[7]: linear or tiled
     * extra_info[8]: the fourcc set by application
     */
    int extra_info[9];
    int size;
    unsigned int bc_buffer;
    void *handle;
};

/*
 * Create surface
 */
VAStatus psb_surface_create(psb_driver_data_p driver_data,
                            int width, int height, int fourcc, unsigned int flags,
                            psb_surface_p psb_surface /* out */
                           );


#define SET_SURFACE_INFO_rotate(psb_surface, rotate) psb_surface->extra_info[5] = (uint32_t) rotate;
#define GET_SURFACE_INFO_rotate(psb_surface) ((int) psb_surface->extra_info[5])
#define GET_SURFACE_INFO_protect(psb_surface) ((int) psb_surface->extra_info[6])
#define SET_SURFACE_INFO_protect(psb_surface, protect) (psb_surface->extra_info[6] = protect)
#define SET_SURFACE_INFO_tiling(psb_surface, tiling) psb_surface->extra_info[7] = (uint32_t) tiling;
#define GET_SURFACE_INFO_tiling(psb_surface) ((unsigned long) psb_surface->extra_info[7])


/*
 * Temporarily map surface and set all chroma values of surface to 'chroma'
 */
VAStatus psb_surface_set_chroma(psb_surface_p psb_surface, int chroma);

/*
 * Destroy surface
 */
void psb_surface_destroy(psb_surface_p psb_surface);

/*
 * Wait for surface to become idle
 */
VAStatus psb_surface_sync(psb_surface_p psb_surface);

/*
 * Return surface status
 */
VAStatus psb_surface_query_status(psb_surface_p psb_surface, VASurfaceStatus *status);

/*
 * Set current displaying surface info to kernel
 */
int psb_surface_set_displaying(psb_driver_data_p driver_data,
                               int width, int height,
                               psb_surface_p psb_surface);

#endif /* _PSB_SURFACE_H_ */
