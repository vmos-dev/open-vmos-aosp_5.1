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

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#ifdef ANDROID
#include <linux/ion.h>
#endif
#include <va/va_tpi.h>
#include "psb_drv_video.h"
#include "psb_drv_debug.h"
#include "psb_surface.h"
#include "psb_surface_attrib.h"


#define INIT_DRIVER_DATA    psb_driver_data_p driver_data = (psb_driver_data_p) ctx->pDriverData;

#define CONFIG(id)  ((object_config_p) object_heap_lookup( &driver_data->config_heap, id ))
#define CONTEXT(id) ((object_context_p) object_heap_lookup( &driver_data->context_heap, id ))
#define SURFACE(id)    ((object_surface_p) object_heap_lookup( &driver_data->surface_heap, id ))
#define BUFFER(id)  ((object_buffer_p) object_heap_lookup( &driver_data->buffer_heap, id ))


/*
 * Create surface
 */
VAStatus psb_surface_create_from_ub(
    psb_driver_data_p driver_data,
    int width, int height, int fourcc,
    VASurfaceAttributeTPI *graphic_buffers,
    psb_surface_p psb_surface, /* out */
    void *vaddr,
    unsigned flags
)
{
    int ret = 0;

    if ((fourcc == VA_FOURCC_NV12) || (fourcc == VA_FOURCC_YV16) || (fourcc == VA_FOURCC_IYUV) || (fourcc == VA_FOURCC_RGBA)) {
        if ((width <= 0) || (width * height > 5120 * 5120) || (height <= 0)) {
            return VA_STATUS_ERROR_ALLOCATION_FAILED;
        }

        psb_surface->stride = graphic_buffers->luma_stride;
        if (0) {
            ;
        } else if (512 == graphic_buffers->luma_stride) {
            psb_surface->stride_mode = STRIDE_512;
        } else if (1024 == graphic_buffers->luma_stride) {
            psb_surface->stride_mode = STRIDE_1024;
        } else if (1280 == graphic_buffers->luma_stride) {
            psb_surface->stride_mode = STRIDE_1280;
#ifdef PSBVIDEO_MSVDX_DEC_TILING
            if (graphic_buffers->tiling) {
                psb_surface->stride_mode = STRIDE_2048;
                psb_surface->stride = 2048;
            }
#endif
        } else if (2048 == graphic_buffers->luma_stride) {
            psb_surface->stride_mode = STRIDE_2048;
        } else if (4096 == graphic_buffers->luma_stride) {
            psb_surface->stride_mode = STRIDE_4096;
        } else {
            psb_surface->stride_mode = STRIDE_NA;
        }
        if (psb_surface->stride != graphic_buffers->luma_stride) {
            return VA_STATUS_ERROR_ALLOCATION_FAILED;
        }

        psb_surface->luma_offset = 0;
        psb_surface->chroma_offset = psb_surface->stride * height;

        if (VA_FOURCC_NV12 == fourcc) {
            psb_surface->size = ((psb_surface->stride * height) * 3) / 2;
            psb_surface->extra_info[4] = VA_FOURCC_NV12;
        }
        else if (VA_FOURCC_YV16 == fourcc) {
            psb_surface->size = (psb_surface->stride * height) * 2;
            psb_surface->extra_info[4] = VA_FOURCC_YV16;
        }
        else if (VA_FOURCC_IYUV == fourcc) {
            psb_surface->size = ((psb_surface->stride * height) * 3) / 2;
            psb_surface->extra_info[4] = VA_FOURCC_IYUV;
        }
	else if (VA_FOURCC_RGBA == fourcc) {
            psb_surface->size = (psb_surface->stride * height) * 4;
            psb_surface->extra_info[4] = VA_FOURCC_RGBA;
        }

        psb_surface->extra_info[8] = psb_surface->extra_info[4];

    } else {
        return VA_STATUS_ERROR_ALLOCATION_FAILED;
    }
#ifdef PSBVIDEO_MSVDX_DEC_TILING
    if (graphic_buffers->tiling)
        ret = psb_buffer_create_from_ub(driver_data, psb_surface->size,
                psb_bt_mmu_tiling, &psb_surface->buf,
                vaddr, 0);
    else
#endif
        ret = psb_buffer_create_from_ub(driver_data, psb_surface->size,
                psb_bt_surface, &psb_surface->buf,
                vaddr, flags);

    return ret ? VA_STATUS_ERROR_ALLOCATION_FAILED : VA_STATUS_SUCCESS;
}

#if 0
VAStatus psb_CreateSurfaceFromV4L2Buf(
    VADriverContextP ctx,
    int v4l2_fd,         /* file descriptor of V4L2 device */
    struct v4l2_format *v4l2_fmt,       /* format of V4L2 */
    struct v4l2_buffer *v4l2_buf,       /* V4L2 buffer */
    VASurfaceID *surface        /* out */
)
{
    INIT_DRIVER_DATA;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    int surfaceID;
    object_surface_p obj_surface;
    psb_surface_p psb_surface;
    int width, height, buf_stride, buf_offset, size;
    unsigned long *user_ptr = NULL;

    if (IS_MRST(driver_data) == 0 && IS_MFLD(driver_data) == 0) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "CreateSurfaceFromV4L2Buf isn't supported on non-MRST platform\n");
        return VA_STATUS_ERROR_UNKNOWN;
    }

    /* Todo:
     * sanity check if the v4l2 device on MRST is supported
     */
    if (V4L2_MEMORY_USERPTR == v4l2_buf->memory) {
        unsigned long tmp = (unsigned long)(v4l2_buf->m.userptr);

        if (tmp & 0xfff) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "The buffer address 0x%08x must be page aligned\n", tmp);
            return VA_STATUS_ERROR_UNKNOWN;
        }
    }

    surfaceID = object_heap_allocate(&driver_data->surface_heap);
    obj_surface = SURFACE(surfaceID);
    CHECK_ALLOCATION(obj_surface);

    MEMSET_OBJECT(obj_surface, struct object_surface_s);

    width = v4l2_fmt->fmt.pix.width;
    height = v4l2_fmt->fmt.pix.height;

    buf_stride = width; /* ? */
    buf_offset = v4l2_buf->m.offset;
    size = v4l2_buf->length;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Create Surface from V4L2 buffer: %dx%d, stride=%d, buffer offset=0x%08x, size=%d\n",
                             width, height, buf_stride, buf_offset, size);

    obj_surface->surface_id = surfaceID;
    *surface = surfaceID;
    obj_surface->context_id = -1;
    obj_surface->width = width;
    obj_surface->height = height;
    obj_surface->subpictures = NULL;
    obj_surface->subpic_count = 0;
    obj_surface->derived_imgcnt = 0;
    obj_surface->display_timestamp = 0;

    psb_surface = (psb_surface_p) calloc(1, sizeof(struct psb_surface_s));
    if (NULL == psb_surface) {
        object_heap_free(&driver_data->surface_heap, (object_base_p) obj_surface);
        obj_surface->surface_id = VA_INVALID_SURFACE;

        vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;

        DEBUG_FAILURE;

        return vaStatus;
    }

#if PSB_MFLD_DUMMY_CODE
    /* current assume it is NV12 */
    if (IS_MRST(driver_data))
        vaStatus = psb_surface_create_camera(driver_data, width, height, buf_stride, size, psb_surface, 1, buf_offset);
    else {
        if (V4L2_MEMORY_USERPTR == v4l2_buf->memory)
            user_ptr = (unsigned long *)(v4l2_buf->m.userptr);
        else {
            user_ptr = mmap(NULL /* start anywhere */ ,
                            v4l2_buf->length,
                            PROT_READ ,
                            MAP_SHARED /* recommended */ ,
                            v4l2_fd, v4l2_buf->m.offset);
        }

        if (NULL != user_ptr && MAP_FAILED != user_ptr)
            vaStatus = psb_surface_create_camera_from_ub(driver_data, width, height,
                       buf_stride, size, psb_surface, 1, buf_offset, user_ptr);
        else {
            DEBUG_FAILURE;
            vaStatus = VA_STATUS_ERROR_UNKNOWN;
        }
    }
#else
        vaStatus = VA_STATUS_ERROR_UNKNOWN;
#endif


    if (VA_STATUS_SUCCESS != vaStatus) {
        free(psb_surface);
        object_heap_free(&driver_data->surface_heap, (object_base_p) obj_surface);
        obj_surface->surface_id = VA_INVALID_SURFACE;

        DEBUG_FAILURE;

        return vaStatus;
    }

    memset(psb_surface->extra_info, 0, sizeof(psb_surface->extra_info));
    psb_surface->extra_info[4] = VA_FOURCC_NV12; /* temp treat is as IYUV */

    obj_surface->psb_surface = psb_surface;

    /* Error recovery */
    if (VA_STATUS_SUCCESS != vaStatus) {
        object_surface_p obj_surface = SURFACE(*surface);
        psb__destroy_surface(driver_data, obj_surface);
        *surface = VA_INVALID_SURFACE;
    }

    return vaStatus;
}
#endif


VAStatus psb_CreateSurfacesForUserPtr(
    VADriverContextP ctx,
    int Width,
    int Height,
    int format,
    int num_surfaces,
    VASurfaceID *surface_list,       /* out */
    unsigned size, /* total buffer size need to be allocated */
    unsigned int fourcc, /* expected fourcc */
    unsigned int luma_stride, /* luma stride, could be width aligned with a special value */
    unsigned int chroma_u_stride, /* chroma stride */
    unsigned int chroma_v_stride,
    unsigned int luma_offset, /* could be 0 */
    unsigned int chroma_u_offset, /* UV offset from the beginning of the memory */
    unsigned int chroma_v_offset,
    unsigned int tiling
)
{
    INIT_DRIVER_DATA
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    int i, height_origin;
    unsigned long buffer_stride;

    /* silient compiler warning */
    unsigned int width = (unsigned int)Width;
    unsigned int height = (unsigned int)Height;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Create surface: width %d, height %d, format 0x%08x"
                             "\n\t\t\t\t\tnum_surface %d, buffer size %d, fourcc 0x%08x"
                             "\n\t\t\t\t\tluma_stride %d, chroma u stride %d, chroma v stride %d"
                             "\n\t\t\t\t\tluma_offset %d, chroma u offset %d, chroma v offset %d\n",
                             width, height, format,
                             num_surfaces, size, fourcc,
                             luma_stride, chroma_u_stride, chroma_v_stride,
                             luma_offset, chroma_u_offset, chroma_v_offset);

    CHECK_INVALID_PARAM(num_surfaces <= 0);
    CHECK_SURFACE(surface_list);

    /* We only support one format */
    if ((VA_RT_FORMAT_YUV420 != format) && (VA_RT_FORMAT_RGB32 != format)) {
        vaStatus = VA_STATUS_ERROR_UNSUPPORTED_RT_FORMAT;
        DEBUG_FAILURE;
        return vaStatus;
    }

    /* We only support NV12 */
    if ((VA_RT_FORMAT_YUV420 == format) && (fourcc != VA_FOURCC_NV12)) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Only support NV12 format\n");
        return VA_STATUS_ERROR_UNKNOWN;
    }

    vaStatus = psb__checkSurfaceDimensions(driver_data, width, height);
    CHECK_VASTATUS();

    if (VA_RT_FORMAT_YUV420 == format) {
    CHECK_INVALID_PARAM((size < width * height * 1.5) ||
        (luma_stride < width) ||
        (chroma_u_stride * 2 < width) ||
        (chroma_v_stride * 2 < width) ||
        (chroma_u_offset < luma_offset + width * height) ||
        (chroma_v_offset < luma_offset + width * height));
    } else if (VA_RT_FORMAT_RGB32 == format) {
    CHECK_INVALID_PARAM((size < width * height * 4) ||
        (luma_stride < width) ||
        (chroma_u_stride * 2 < width) ||
        (chroma_v_stride * 2 < width) ||
        (chroma_u_offset < luma_offset + width * height) ||
        (chroma_v_offset < luma_offset + width * height));
    }

    height_origin = height;

    for (i = 0; i < num_surfaces; i++) {
        int surfaceID;
        object_surface_p obj_surface;
        psb_surface_p psb_surface;

        surfaceID = object_heap_allocate(&driver_data->surface_heap);
        obj_surface = SURFACE(surfaceID);
        if (NULL == obj_surface) {
            vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
            DEBUG_FAILURE;
            break;
        }
        MEMSET_OBJECT(obj_surface, struct object_surface_s);

        obj_surface->surface_id = surfaceID;
        surface_list[i] = surfaceID;
        obj_surface->context_id = -1;
        obj_surface->width = width;
        obj_surface->height = height;
        obj_surface->width_r = width;
        obj_surface->height_r = height;
        obj_surface->height_origin = height_origin;
	obj_surface->is_ref_surface = 0;

        psb_surface = (psb_surface_p) calloc(1, sizeof(struct psb_surface_s));
        if (NULL == psb_surface) {
            object_heap_free(&driver_data->surface_heap, (object_base_p) obj_surface);
            obj_surface->surface_id = VA_INVALID_SURFACE;

            vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;

            DEBUG_FAILURE;
            break;
        }


        vaStatus = psb_surface_create_for_userptr(driver_data, width, height,
                   size,
                   fourcc,
                   luma_stride,
                   chroma_u_stride,
                   chroma_v_stride,
                   luma_offset,
                   chroma_u_offset,
                   chroma_v_offset,
                   psb_surface
                                                 );

        if (VA_STATUS_SUCCESS != vaStatus) {
            free(psb_surface);
            object_heap_free(&driver_data->surface_heap, (object_base_p) obj_surface);
            obj_surface->surface_id = VA_INVALID_SURFACE;

            DEBUG_FAILURE;
            break;
        }
        buffer_stride = psb_surface->stride;
        /* by default, surface fourcc is NV12 */
        memset(psb_surface->extra_info, 0, sizeof(psb_surface->extra_info));
        psb_surface->extra_info[4] = fourcc;
        psb_surface->extra_info[8] = fourcc;
#ifdef PSBVIDEO_MSVDX_DEC_TILING
	psb_surface->extra_info[7] = tiling;
#endif
        obj_surface->psb_surface = psb_surface;
    }

    /* Error recovery */
    if (VA_STATUS_SUCCESS != vaStatus) {
        /* surface_list[i-1] was the last successful allocation */
        for (; i--;) {
            object_surface_p obj_surface = SURFACE(surface_list[i]);
            psb__destroy_surface(driver_data, obj_surface);
            surface_list[i] = VA_INVALID_SURFACE;
        }
    }


    return vaStatus;
}

VAStatus  psb_CreateSurfaceFromKBuf(
    VADriverContextP ctx,
    int _width,
    int _height,
    int format,
    VASurfaceID *surface,       /* out */
    unsigned int kbuf_handle, /* kernel buffer handle*/
    unsigned size, /* kernel buffer size */
    unsigned int kBuf_fourcc, /* expected fourcc */
    unsigned int luma_stride, /* luma stride, could be width aligned with a special value */
    unsigned int chroma_u_stride, /* chroma stride */
    unsigned int chroma_v_stride,
    unsigned int luma_offset, /* could be 0 */
    unsigned int chroma_u_offset, /* UV offset from the beginning of the memory */
    unsigned int chroma_v_offset,
    unsigned int tiling
)
{
    INIT_DRIVER_DATA
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    unsigned long buffer_stride;

    /* silient compiler warning */
    unsigned int width = (unsigned int)_width;
    unsigned int height = (unsigned int)_height;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Create surface: width %d, height %d, format 0x%08x"
                             "\n\t\t\t\t\tnum_surface %d, buffer size %d, fourcc 0x%08x"
                             "\n\t\t\t\t\tluma_stride %d, chroma u stride %d, chroma v stride %d"
                             "\n\t\t\t\t\tluma_offset %d, chroma u offset %d, chroma v offset %d\n",
                             width, height, format,
                             size, kBuf_fourcc,
                             luma_stride, chroma_u_stride, chroma_v_stride,
                             luma_offset, chroma_u_offset, chroma_v_offset);

    CHECK_SURFACE(surface);

    /* We only support one format */
    if (VA_RT_FORMAT_YUV420 != format) {
        vaStatus = VA_STATUS_ERROR_UNSUPPORTED_RT_FORMAT;
        DEBUG_FAILURE;
        return vaStatus;
    }

    /* We only support NV12/YV12 */

    if ((VA_RT_FORMAT_YUV420 == format) && (kBuf_fourcc != VA_FOURCC_NV12)) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Only support NV12 format\n");
        return VA_STATUS_ERROR_UNKNOWN;
    }
    /*
    vaStatus = psb__checkSurfaceDimensions(driver_data, width, height);
    CHECK_VASTATUS();
    */

    CHECK_INVALID_PARAM((size < width * height * 1.5) ||
        (luma_stride < width) ||
        (chroma_u_stride * 2 < width) ||
        (chroma_v_stride * 2 < width) ||
        (chroma_u_offset < luma_offset + width * height) ||
        (chroma_v_offset < luma_offset + width * height));

    int surfaceID;
    object_surface_p obj_surface;
    psb_surface_p psb_surface;

    surfaceID = object_heap_allocate(&driver_data->surface_heap);
    obj_surface = SURFACE(surfaceID);
    CHECK_ALLOCATION(obj_surface);

    MEMSET_OBJECT(obj_surface, struct object_surface_s);

    obj_surface->surface_id = surfaceID;
    *surface = surfaceID;
    obj_surface->context_id = -1;
    obj_surface->width = width;
    obj_surface->height = height;
    obj_surface->width_r = width;
    obj_surface->height_r = height;
    obj_surface->height_origin = height;
    obj_surface->is_ref_surface = 0;

    psb_surface = (psb_surface_p) calloc(1, sizeof(struct psb_surface_s));
    if (NULL == psb_surface) {
        object_heap_free(&driver_data->surface_heap, (object_base_p) obj_surface);
        obj_surface->surface_id = VA_INVALID_SURFACE;

        vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;

        DEBUG_FAILURE;
        return vaStatus;
    }

    vaStatus = psb_surface_create_from_kbuf(driver_data, width, height,
                                            size,
                                            kBuf_fourcc,
                                            kbuf_handle,
                                            luma_stride,
                                            chroma_u_stride,
                                            chroma_v_stride,
                                            luma_offset,
                                            chroma_u_offset,
                                            chroma_v_offset,
                                            psb_surface);

    if (VA_STATUS_SUCCESS != vaStatus) {
        free(psb_surface);
        object_heap_free(&driver_data->surface_heap, (object_base_p) obj_surface);
        obj_surface->surface_id = VA_INVALID_SURFACE;

        DEBUG_FAILURE;
        return vaStatus;
    }
    buffer_stride = psb_surface->stride;
    /* by default, surface fourcc is NV12 */
    memset(psb_surface->extra_info, 0, sizeof(psb_surface->extra_info));
    psb_surface->extra_info[4] = kBuf_fourcc;
    psb_surface->extra_info[8] = kBuf_fourcc;
#ifdef PSBVIDEO_MSVDX_DEC_TILING
    psb_surface->extra_info[7] = tiling;
#endif
    obj_surface->psb_surface = psb_surface;

    /* Error recovery */
    if (VA_STATUS_SUCCESS != vaStatus) {
        object_surface_p obj_surface = SURFACE(surfaceID);
        psb__destroy_surface(driver_data, obj_surface);
        *surface = VA_INVALID_SURFACE;
    }

    return vaStatus;
}

VAStatus  psb_CreateSurfaceFromUserspace(
        VADriverContextP ctx,
        int width,
        int height,
        int format,
        int num_surfaces,
        VASurfaceID *surface_list,        /* out */
        VASurfaceAttributeTPI *attribute_tpi
)
{
    INIT_DRIVER_DATA;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
#ifdef ANDROID
    unsigned int *vaddr;
    unsigned long fourcc;
    int surfaceID;
    object_surface_p obj_surface;
    psb_surface_p psb_surface;
    int i;

    switch (format) {
    case VA_RT_FORMAT_YUV422:
        fourcc = VA_FOURCC_YV16;
        break;
    case VA_RT_FORMAT_YUV420:
    default:
        fourcc = VA_FOURCC_NV12;
        break;
    }

    for (i=0; i < num_surfaces; i++) {
        vaddr = (unsigned int *)(attribute_tpi->buffers[i]);
        surfaceID = object_heap_allocate(&driver_data->surface_heap);
        obj_surface = SURFACE(surfaceID);
        if (NULL == obj_surface) {
            vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
            DEBUG_FAILURE;
            break;
        }
        MEMSET_OBJECT(obj_surface, struct object_surface_s);

        obj_surface->surface_id = surfaceID;
        surface_list[i] = surfaceID;
        obj_surface->context_id = -1;
        obj_surface->width = attribute_tpi->width;
        obj_surface->height = attribute_tpi->height;
        obj_surface->width_r = attribute_tpi->width;
        obj_surface->height_r = attribute_tpi->height;
	obj_surface->is_ref_surface = 0;

        psb_surface = (psb_surface_p) calloc(1, sizeof(struct psb_surface_s));
        if (NULL == psb_surface) {
            object_heap_free(&driver_data->surface_heap, (object_base_p) obj_surface);
            obj_surface->surface_id = VA_INVALID_SURFACE;
            vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
            DEBUG_FAILURE;
            break;
        }

        if (attribute_tpi->type == VAExternalMemoryNoneCacheUserPointer)
            vaStatus = psb_surface_create_from_ub(driver_data, width, height, fourcc,
                    attribute_tpi, psb_surface, vaddr, PSB_USER_BUFFER_UNCACHED);
        else
            vaStatus = psb_surface_create_from_ub(driver_data, width, height, fourcc,
                    attribute_tpi, psb_surface, vaddr, 0);
        obj_surface->psb_surface = psb_surface;

        if (VA_STATUS_SUCCESS != vaStatus) {
            free(psb_surface);
            object_heap_free(&driver_data->surface_heap, (object_base_p) obj_surface);
            obj_surface->surface_id = VA_INVALID_SURFACE;
            DEBUG_FAILURE;
            break;
        }
        /* by default, surface fourcc is NV12 */
        memset(psb_surface->extra_info, 0, sizeof(psb_surface->extra_info));
        psb_surface->extra_info[4] = fourcc;
        psb_surface->extra_info[8] = fourcc;
        obj_surface->psb_surface = psb_surface;

        /* Error recovery */
        if (VA_STATUS_SUCCESS != vaStatus) {
            object_surface_p obj_surface = SURFACE(surfaceID);
            psb__destroy_surface(driver_data, obj_surface);
        }
    }
#endif
    return vaStatus;
}

VAStatus  psb_CreateSurfaceFromION(
        VADriverContextP ctx,
        int width,
        int height,
        int format,
        int num_surfaces,
        VASurfaceID *surface_list,        /* out */
        VASurfaceAttributeTPI *attribute_tpi
)
{
    INIT_DRIVER_DATA;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
#ifdef ANDROID
    unsigned int *vaddr = NULL;
    unsigned long fourcc;
    int surfaceID;
    object_surface_p obj_surface;
    psb_surface_p psb_surface;
    int i;
    unsigned int source_size = 0;
    int ion_fd = 0;
    int ion_ret = 0;
    struct ion_fd_data ion_source_share;

    switch (format) {
    case VA_RT_FORMAT_YUV422:
        fourcc = VA_FOURCC_YV16;
        break;
    case VA_RT_FORMAT_YUV420:
    default:
        fourcc = VA_FOURCC_NV12;
        break;
    }

    ion_fd = open("/dev/ion", O_RDWR);
    if (ion_fd < 0) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: Fail to open the ion device!\n", __FUNCTION__);
        return VA_STATUS_ERROR_UNKNOWN;
    }

    for (i=0; i < num_surfaces; i++) {
        ion_source_share.handle = 0;
        ion_source_share.fd = (int)(attribute_tpi->buffers[i]);
        ion_ret = ioctl(ion_fd, ION_IOC_IMPORT, &ion_source_share);
            if ((ion_ret < 0) || (0 == ion_source_share.handle)) {
            close(ion_fd);
            drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: Fail to import the ion fd!\n", __FUNCTION__);
            return VA_STATUS_ERROR_UNKNOWN;
        }

        if (VA_FOURCC_NV12 == fourcc)
            source_size = attribute_tpi->width * attribute_tpi->height * 1.5;
        else
            source_size = attribute_tpi->width * attribute_tpi->height * 2;

        vaddr = mmap(NULL, source_size, PROT_READ|PROT_WRITE, MAP_SHARED, ion_source_share.fd, 0);
        if (MAP_FAILED == vaddr) {
            close(ion_fd);
            drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: Fail to mmap the ion buffer!\n", __FUNCTION__);
            return VA_STATUS_ERROR_UNKNOWN;
        }

        surfaceID = object_heap_allocate(&driver_data->surface_heap);
        obj_surface = SURFACE(surfaceID);
        if (NULL == obj_surface) {
            close(ion_fd);
            vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
            DEBUG_FAILURE;
            break;
        }
        MEMSET_OBJECT(obj_surface, struct object_surface_s);

        obj_surface->surface_id = surfaceID;
        surface_list[i] = surfaceID;
        obj_surface->context_id = -1;
        obj_surface->width = attribute_tpi->width;
        obj_surface->height = attribute_tpi->height;
        obj_surface->width_r = attribute_tpi->width;
        obj_surface->height_r = attribute_tpi->height;
	obj_surface->is_ref_surface = 0;

        psb_surface = (psb_surface_p) calloc(1, sizeof(struct psb_surface_s));
        if (NULL == psb_surface) {
            object_heap_free(&driver_data->surface_heap, (object_base_p) obj_surface);
            obj_surface->surface_id = VA_INVALID_SURFACE;
            close(ion_fd);
            vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
            DEBUG_FAILURE;
            break;
        }

        vaStatus = psb_surface_create_from_ub(driver_data, width, height, fourcc,
                attribute_tpi, psb_surface, vaddr, 0);
        obj_surface->psb_surface = psb_surface;

        if (VA_STATUS_SUCCESS != vaStatus) {
            free(psb_surface);
            object_heap_free(&driver_data->surface_heap, (object_base_p) obj_surface);
            obj_surface->surface_id = VA_INVALID_SURFACE;
            close(ion_fd);
            DEBUG_FAILURE;
            break;
        }
        /* by default, surface fourcc is NV12 */
        memset(psb_surface->extra_info, 0, sizeof(psb_surface->extra_info));
        psb_surface->extra_info[4] = fourcc;
        psb_surface->extra_info[8] = fourcc;
        obj_surface->psb_surface = psb_surface;

        /* Error recovery */
        if (VA_STATUS_SUCCESS != vaStatus) {
            object_surface_p obj_surface = SURFACE(surfaceID);
            psb__destroy_surface(driver_data, obj_surface);
            close(ion_fd);
        }

        vaddr = NULL;
    }

    close(ion_fd);
#endif
    return vaStatus;
}

VAStatus psb_CreateSurfacesWithAttribute(
    VADriverContextP ctx,
    int width,
    int height,
    int format,
    int num_surfaces,
    VASurfaceID *surface_list,        /* out */
    VASurfaceAttributeTPI *attribute_tpi
)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    int i;
    int tiling;

    CHECK_INVALID_PARAM(attribute_tpi == NULL);

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Create %d surface(%dx%d) with type %d, tiling is %d\n",
            num_surfaces, width, height, attribute_tpi->type, attribute_tpi->tiling);

    tiling = attribute_tpi->tiling;
    switch (attribute_tpi->type) {
    case VAExternalMemoryNULL:
        vaStatus = psb_CreateSurfacesForUserPtr(ctx, width, height, format, num_surfaces, surface_list,
                                     attribute_tpi->size, attribute_tpi->pixel_format,
                                     attribute_tpi->luma_stride, attribute_tpi->chroma_u_stride,
                                     attribute_tpi->chroma_v_stride, attribute_tpi->luma_offset,
                                     attribute_tpi->chroma_u_offset, attribute_tpi->chroma_v_offset,
                                     attribute_tpi->tiling);
        return vaStatus;
#ifdef ANDROID
    case VAExternalMemoryNoneCacheUserPointer:
#endif
    case VAExternalMemoryUserPointer:
        vaStatus = psb_CreateSurfaceFromUserspace(ctx, width, height,
                                                 format, num_surfaces, surface_list,
                                                 attribute_tpi);
        return vaStatus;
    case VAExternalMemoryKernelDRMBufffer:
        for (i=0; i < num_surfaces; i++) {
            vaStatus = psb_CreateSurfaceFromKBuf(
                ctx, width, height, format, &surface_list[i],
                attribute_tpi->buffers[i],
                attribute_tpi->size, attribute_tpi->pixel_format,
                attribute_tpi->luma_stride, attribute_tpi->chroma_u_stride,
                attribute_tpi->chroma_v_stride, attribute_tpi->luma_offset,
                attribute_tpi->chroma_u_offset, attribute_tpi->chroma_v_offset, tiling);
            CHECK_VASTATUS();
        }
        return vaStatus;
    case VAExternalMemoryAndroidGrallocBuffer:
        vaStatus = psb_CreateSurfacesFromGralloc(ctx, width, height,
                                                 format, num_surfaces, surface_list,
                                                 (PsbSurfaceAttributeTPI *)attribute_tpi);
        return vaStatus;
#ifdef ANDROID
    case VAExternalMemoryIONSharedFD:
        vaStatus = psb_CreateSurfaceFromION(ctx, width, height,
                                                 format, num_surfaces, surface_list,
                                                 attribute_tpi);
        return vaStatus;
#endif
    default:
        return VA_STATUS_ERROR_INVALID_PARAMETER;
    }

    return VA_STATUS_ERROR_INVALID_PARAMETER;
}
