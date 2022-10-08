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
 * Authors:
 *    Shengquan Yuan  <shengquan.yuan@intel.com>
 *    Zhaohan Ren  <zhaohan.ren@intel.com>
 *    Jason Hu <jason.hu@intel.com>
 *
 */

#ifndef ANDROID
#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>
#include <va/va_dricommon.h>
#include "x11/psb_x11.h"
#include "x11/psb_xrandr.h"
#endif
#include <va/va_backend.h>
#include <dlfcn.h>
#include <stdlib.h>
#include "psb_output.h"
#include "psb_surface.h"
#include "psb_buffer.h"
#include "psb_surface_ext.h"
#include "pnw_rotate.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <wsbm/wsbm_manager.h>
#include "psb_drv_debug.h"
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define INIT_DRIVER_DATA        psb_driver_data_p driver_data = (psb_driver_data_p) ctx->pDriverData;

#define SURFACE(id)     ((object_surface_p) object_heap_lookup( &driver_data->surface_heap, id ))
#define BUFFER(id)  ((object_buffer_p) object_heap_lookup( &driver_data->buffer_heap, id ))
#define IMAGE(id)  ((object_image_p) object_heap_lookup( &driver_data->image_heap, id ))
#define SUBPIC(id)  ((object_subpic_p) object_heap_lookup( &driver_data->subpic_heap, id ))
#define CONTEXT(id) ((object_context_p) object_heap_lookup( &driver_data->context_heap, id ))


/* surfaces link list associated with a subpicture */
typedef struct _subpic_surface {
    VASurfaceID surface_id;
    struct _subpic_surface *next;
} subpic_surface_s, *subpic_surface_p;


static VAImageFormat psb__SubpicFormat[] = {
    psb__ImageRGBA,
    //psb__ImageAYUV,
    //psb__ImageAI44
};

static VAImageFormat psb__CreateImageFormat[] = {
    psb__ImageNV12,
    psb__ImageRGBA,
    //psb__ImageAYUV,
    //psb__ImageAI44,
    psb__ImageYV16,
    psb__ImageYV32
};

unsigned char *psb_x11_output_init(VADriverContextP ctx);
VAStatus psb_x11_output_deinit(VADriverContextP ctx);
unsigned char *psb_android_output_init(VADriverContextP ctx);
VAStatus psb_android_output_deinit(VADriverContextP ctx);

int psb_coverlay_init(VADriverContextP ctx);
int psb_coverlay_deinit(VADriverContextP ctx);

VAStatus psb_initOutput(VADriverContextP ctx)
{
    INIT_DRIVER_DATA;
    unsigned char *ws_priv = NULL;
    char env_value[1024];

    pthread_mutex_init(&driver_data->output_mutex, NULL);

    if (psb_parse_config("PSB_VIDEO_PUTSURFACE_DUMMY", &env_value[0]) == 0) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "vaPutSurface: dummy mode, return directly\n");
        driver_data->dummy_putsurface = 0;

        return VA_STATUS_SUCCESS;
    }

    if (psb_parse_config("PSB_VIDEO_FPS", &env_value[0]) == 0) {
        driver_data->fixed_fps = atoi(env_value);
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Throttling at FPS=%d\n", driver_data->fixed_fps);
    } else
        driver_data->fixed_fps = 0;

    driver_data->outputmethod_checkinterval = 1;
    if (psb_parse_config("PSB_VIDEO_INTERVAL", &env_value[0]) == 0) {
        driver_data->outputmethod_checkinterval = atoi(env_value);
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Check output method at %d frames interval\n",
                                 driver_data->outputmethod_checkinterval);
    }

    driver_data->cur_displaying_surface = VA_INVALID_SURFACE;
    driver_data->last_displaying_surface = VA_INVALID_SURFACE;

    psb_InitOutLoop(ctx);

#ifdef ANDROID
    ws_priv = psb_android_output_init(ctx);
    driver_data->is_android = 1;
#else
    ws_priv = psb_x11_output_init(ctx);
    driver_data->is_android = 0;
#endif
    driver_data->ws_priv = ws_priv;


#if 0
    //use client textureblit
    if (driver_data->ctexture == 1) {
        int ret = psb_ctexture_init(ctx);
        if (ret != 0)
            driver_data->ctexture = 0;
    }
#endif

    /*
    //use texture streaming
    if (driver_data->ctexstreaming == 1)
    psb_ctexstreaing_init(ctx);
    */

    return VA_STATUS_SUCCESS;
}

VAStatus psb_deinitOutput(
    VADriverContextP ctx
)
{
    INIT_DRIVER_DATA;

#if 0
    //use client textureblit
    if (driver_data->ctexture == 1)
        psb_ctexture_deinit(ctx);
#endif

    if (driver_data->coverlay_init) {
        psb_coverlay_deinit(ctx);
        driver_data->coverlay_init = 0;
    }

#ifndef ANDROID
    psb_x11_output_deinit(ctx);
#else
    psb_android_output_deinit(ctx);
#endif
    /* free here, but allocate in window system specific */
    free(driver_data->ws_priv);
    /*
    //use texture streaming
    if (driver_data->ctexstreaming == 1)
        psb_ctexstreaing_deinit(ctx);
    */
    /* clean the displaying surface information in kernel */
#ifndef _FOR_FPGA_
    psb_surface_set_displaying(driver_data, 0, 0, NULL);
#endif
    pthread_mutex_destroy(&driver_data->output_mutex);

    return VA_STATUS_SUCCESS;
}

#ifndef VA_STATUS_ERROR_INVALID_IMAGE_FORMAT
#define VA_STATUS_ERROR_INVALID_IMAGE_FORMAT VA_STATUS_ERROR_UNKNOWN
#endif

static VAImageFormat *psb__VAImageCheckFourCC(
    VAImageFormat       *src_format,
    VAImageFormat       *dst_format,
    int                 dst_num
)
{
    int i;
    if (NULL == src_format || dst_format == NULL) {
        return NULL;
    }

    /* check VAImage at first */
    for (i = 0; i < dst_num; i++) {
        if (dst_format[i].fourcc == src_format->fourcc)
            return &dst_format[i];
    }

    drv_debug_msg(VIDEO_DEBUG_ERROR, "Unsupport fourcc 0x%x\n", src_format->fourcc);
    return NULL;
}

static void psb__VAImageCheckRegion(
    object_surface_p surface,
    VAImage *image,
    int *src_x,
    int *src_y,
    int *dest_x,
    int *dest_y,
    int *width,
    int *height
)
{
    /* check for image */
    if (*src_x < 0) *src_x = 0;
    if (*src_x > image->width) *src_x = image->width - 1;
    if (*src_y < 0) *src_y = 0;
    if (*src_y > image->height) *src_y = image->height - 1;

    if (((*width) + (*src_x)) > image->width) *width = image->width - *src_x;
    if (((*height) + (*src_y)) > image->height) *height = image->height - *src_x;

    /* check for surface */
    if (*dest_x < 0) *dest_x = 0;
    if (*dest_x > surface->width) *dest_x = surface->width - 1;
    if (*dest_y < 0) *dest_y = 0;
    if (*dest_y > surface->height) *dest_y = surface->height - 1;

    if (((*width) + (*dest_x)) > surface->width) *width = surface->width - *dest_x;
    if (((*height) + (*dest_y)) > surface->height) *height = surface->height - *dest_x;
}


VAStatus psb_QueryImageFormats(
    VADriverContextP __maybe_unused ctx,
    VAImageFormat *format_list,        /* out */
    int *num_formats           /* out */
)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    CHECK_INVALID_PARAM(format_list == NULL);
    CHECK_INVALID_PARAM(num_formats == NULL);

    memcpy(format_list, psb__CreateImageFormat, sizeof(psb__CreateImageFormat));
    *num_formats = PSB_MAX_IMAGE_FORMATS;

    return VA_STATUS_SUCCESS;
}

inline int min_POT(int n)
{
    if ((n & (n - 1)) == 0) /* already POT */
        return n;

    return n |= n >> 16, n |= n >> 8, n |= n >> 4, n |= n >> 2, n |= n >> 1, n + 1;
    /* return ((((n |= n>>16) |= n>>8) |= n>>4) |= n>>2) |= n>>1, n + 1; */
}

VAStatus psb_CreateImage(
    VADriverContextP ctx,
    VAImageFormat *format,
    int width,
    int height,
    VAImage *image     /* out */
)
{
    INIT_DRIVER_DATA;
    VAImageID imageID;
    object_image_p obj_image;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAImageFormat *img_fmt;
    int pitch_pot;

    (void)driver_data;

    img_fmt = psb__VAImageCheckFourCC(format, psb__CreateImageFormat,
                                      sizeof(psb__CreateImageFormat) / sizeof(VAImageFormat));
    if (img_fmt == NULL)
        return VA_STATUS_ERROR_UNKNOWN;

    CHECK_INVALID_PARAM(image == NULL);

    imageID = object_heap_allocate(&driver_data->image_heap);
    obj_image = IMAGE(imageID);
    CHECK_ALLOCATION(obj_image);

    MEMSET_OBJECT(obj_image, struct object_image_s);

    obj_image->image.image_id = imageID;
    obj_image->image.format = *img_fmt;
    obj_image->subpic_ref = 0;

    pitch_pot = min_POT(width);

    switch (format->fourcc) {
    case VA_FOURCC_NV12: {
        obj_image->image.width = width;
        obj_image->image.height = height;
        obj_image->image.data_size = pitch_pot * height /*Y*/ + 2 * (pitch_pot / 2) * (height / 2);/*UV*/
        obj_image->image.num_planes = 2;
        obj_image->image.pitches[0] = pitch_pot;
        obj_image->image.pitches[1] = pitch_pot;
        obj_image->image.offsets[0] = 0;
        obj_image->image.offsets[1] = pitch_pot * height;
        obj_image->image.num_palette_entries = 0;
        obj_image->image.entry_bytes = 0;
        obj_image->image.component_order[0] = 'Y';
        obj_image->image.component_order[1] = 'U';/* fixed me: packed UV packed here! */
        obj_image->image.component_order[2] = 'V';
        obj_image->image.component_order[3] = '\0';
        break;
    }
    case VA_FOURCC_AYUV: {
        obj_image->image.width = width;
        obj_image->image.height = height;
        obj_image->image.data_size = 4 * pitch_pot * height;
        obj_image->image.num_planes = 1;
        obj_image->image.pitches[0] = 4 * pitch_pot;
        obj_image->image.num_palette_entries = 0;
        obj_image->image.entry_bytes = 0;
        obj_image->image.component_order[0] = 'V';
        obj_image->image.component_order[1] = 'U';
        obj_image->image.component_order[2] = 'Y';
        obj_image->image.component_order[3] = 'A';
        break;
    }
    case VA_FOURCC_RGBA: {
        obj_image->image.width = width;
        obj_image->image.height = height;
        obj_image->image.data_size = 4 * pitch_pot * height;
        obj_image->image.num_planes = 1;
        obj_image->image.pitches[0] = 4 * pitch_pot;
        obj_image->image.num_palette_entries = 0;
        obj_image->image.entry_bytes = 0;
        obj_image->image.component_order[0] = 'R';
        obj_image->image.component_order[1] = 'G';
        obj_image->image.component_order[2] = 'B';
        obj_image->image.component_order[3] = 'A';
        break;
    }
    case VA_FOURCC_AI44: {
        obj_image->image.width = width;
        obj_image->image.height = height;
        obj_image->image.data_size = pitch_pot * height;/* one byte one element */
        obj_image->image.num_planes = 1;
        obj_image->image.pitches[0] = pitch_pot;
        obj_image->image.num_palette_entries = 16;
        obj_image->image.entry_bytes = 4; /* AYUV */
        obj_image->image.component_order[0] = 'I';
        obj_image->image.component_order[1] = 'A';
        obj_image->image.component_order[2] = '\0';
        obj_image->image.component_order[3] = '\0';
        break;
    }
    case VA_FOURCC_IYUV: {
        obj_image->image.width = width;
        obj_image->image.height = height;
        obj_image->image.data_size = pitch_pot * height /*Y*/ + 2 * (pitch_pot / 2) * (height / 2);/*UV*/
        obj_image->image.num_planes = 3;
        obj_image->image.pitches[0] = pitch_pot;
        obj_image->image.pitches[1] = pitch_pot / 2;
        obj_image->image.pitches[2] = pitch_pot / 2;
        obj_image->image.offsets[0] = 0;
        obj_image->image.offsets[1] = pitch_pot * height;
        obj_image->image.offsets[2] = pitch_pot * height + (pitch_pot / 2) * (height / 2);
        obj_image->image.num_palette_entries = 0;
        obj_image->image.entry_bytes = 0;
        obj_image->image.component_order[0] = 'Y';
        obj_image->image.component_order[1] = 'U';
        obj_image->image.component_order[2] = 'V';
        obj_image->image.component_order[3] = '\0';
        break;
    }
    case VA_FOURCC_YV32: {
        obj_image->image.width = width;
        obj_image->image.height = height;
        obj_image->image.data_size = 4 * pitch_pot * height;
        obj_image->image.num_planes = 4;
        obj_image->image.pitches[0] = pitch_pot;
        obj_image->image.pitches[1] = pitch_pot;
        obj_image->image.pitches[2] = pitch_pot;
        obj_image->image.extra_pitch = pitch_pot;
        obj_image->image.offsets[0] = 0;
        obj_image->image.offsets[1] = pitch_pot * height;
        obj_image->image.offsets[2] = pitch_pot * height * 2;
        obj_image->image.extra_offset = pitch_pot * height * 3;
        obj_image->image.num_palette_entries = 0;
        obj_image->image.entry_bytes = 0;
        obj_image->image.component_order[0] = 'V';
        obj_image->image.component_order[1] = 'U';
        obj_image->image.component_order[2] = 'Y';
        obj_image->image.component_order[3] = 'A';
        break;
    }
    default: {
        vaStatus = VA_STATUS_ERROR_INVALID_IMAGE_FORMAT;
        break;
    }
    }

    if (VA_STATUS_SUCCESS == vaStatus) {
        /* create the buffer */
        vaStatus = psb__CreateBuffer(driver_data, NULL, VAImageBufferType,
                                     obj_image->image.data_size, 1, NULL, &obj_image->image.buf);
    }

    obj_image->derived_surface = 0;

    if (VA_STATUS_SUCCESS != vaStatus) {
        object_heap_free(&driver_data->image_heap, (object_base_p) obj_image);
    } else {
        memcpy(image, &obj_image->image, sizeof(VAImage));
    }

    return vaStatus;
}

static int psb_CheckIEDStatus(VADriverContextP ctx)
{
    INIT_DRIVER_DATA;
    struct drm_lnc_video_getparam_arg arg;
    unsigned long temp;
    int ret = 0;

    /* not settled, we get it from current HW FRAMESKIP flag */
    arg.key = IMG_VIDEO_IED_STATE;
    arg.value = (uint64_t)((unsigned long) & temp);
    ret = drmCommandWriteRead(driver_data->drm_fd, driver_data->getParamIoctlOffset,
                              &arg, sizeof(arg));
    if (ret == 0) {
        if (temp == 1) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "IED is enabled, image is encrypted.\n");
            return 1;
        } else {
            return 0;
        }
    } else {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Failed to call IMG_VIDEO_IED_STATE.\n");
        return -1;
    }
}

VAStatus psb_DeriveImage(
    VADriverContextP ctx,
    VASurfaceID surface,
    VAImage *image     /* out */
)
{
    INIT_DRIVER_DATA;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VABufferID bufferID;
    object_buffer_p obj_buffer;
    VAImageID imageID;
    object_image_p obj_image;
    object_surface_p obj_surface = SURFACE(surface);
    unsigned int fourcc, fourcc_index = ~0, i;
    uint32_t srf_buf_ofs = 0;

    CHECK_SURFACE(obj_surface);
    CHECK_INVALID_PARAM(image == NULL);
    /* Can't derive image from reconstrued frame which is in tiled format */
    if (obj_surface->is_ref_surface == 1 || obj_surface->is_ref_surface == 2) {
	if (getenv("PSB_VIDEO_IGNORE_TILED_FORMAT")) {
	    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Ignore tiled memory format" \
			"of rec-frames\n");
	} else {
	    drv_debug_msg(VIDEO_DEBUG_ERROR, "Can't derive reference surface" \
			"which is tiled format\n");
	    return VA_STATUS_ERROR_OPERATION_FAILED;
	}
    }

    if (IS_MFLD(driver_data) && (psb_CheckIEDStatus(ctx) == 1)) {
        vaStatus = VA_STATUS_ERROR_INVALID_SURFACE;
        return vaStatus;
    }

    fourcc = obj_surface->psb_surface->extra_info[4];
    for (i = 0; i < PSB_MAX_IMAGE_FORMATS; i++) {
        if (psb__CreateImageFormat[i].fourcc == fourcc) {
            fourcc_index = i;
            break;
        }
    }
    if (i == PSB_MAX_IMAGE_FORMATS) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Can't support the Fourcc\n");
        vaStatus = VA_STATUS_ERROR_OPERATION_FAILED;
        return vaStatus;
    }

    /* create the image */
    imageID = object_heap_allocate(&driver_data->image_heap);
    obj_image = IMAGE(imageID);
    CHECK_ALLOCATION(obj_image);

    MEMSET_OBJECT(obj_image, struct object_image_s);

    /* create a buffer to represent surface buffer */
    bufferID = object_heap_allocate(&driver_data->buffer_heap);
    obj_buffer = BUFFER(bufferID);
    if (NULL == obj_buffer) {
        object_heap_free(&driver_data->image_heap, (object_base_p) obj_image);
        vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
        DEBUG_FAILURE;
        return vaStatus;
    }
    MEMSET_OBJECT(obj_buffer, struct object_buffer_s);

    obj_buffer->type = VAImageBufferType;
    obj_buffer->buffer_data = NULL;
    obj_buffer->psb_buffer = &obj_surface->psb_surface->buf;
    obj_buffer->size = obj_surface->psb_surface->size;
    obj_buffer->max_num_elements = 0;
    obj_buffer->alloc_size = obj_buffer->size;

    /* fill obj_image data structure */
    obj_image->image.image_id = imageID;
    obj_image->image.format = psb__CreateImageFormat[fourcc_index];
    obj_image->subpic_ref = 0;

    obj_image->image.buf = bufferID;
    obj_image->image.width = obj_surface->width;
    obj_image->image.height = obj_surface->height;
    obj_image->image.data_size = obj_surface->psb_surface->size;

    srf_buf_ofs = obj_surface->psb_surface->buf.buffer_ofs;

    switch (fourcc) {
    case VA_FOURCC_NV12: {
        obj_image->image.num_planes = 2;
        obj_image->image.pitches[0] = obj_surface->psb_surface->stride;
        obj_image->image.pitches[1] = obj_surface->psb_surface->stride;

        obj_image->image.offsets[0] = srf_buf_ofs;
        obj_image->image.offsets[1] = srf_buf_ofs + obj_surface->height * obj_surface->psb_surface->stride;
        obj_image->image.num_palette_entries = 0;
        obj_image->image.entry_bytes = 0;
        obj_image->image.component_order[0] = 'Y';
        obj_image->image.component_order[1] = 'U';/* fixed me: packed UV packed here! */
        obj_image->image.component_order[2] = 'V';
        obj_image->image.component_order[3] = '\0';
        break;
    }
    case VA_FOURCC_YV16: {
        obj_image->image.num_planes = 3;
        obj_image->image.pitches[0] = obj_surface->psb_surface->stride;
        obj_image->image.pitches[1] = obj_surface->psb_surface->stride / 2;
        obj_image->image.pitches[2] = obj_surface->psb_surface->stride / 2;

        obj_image->image.offsets[0] = srf_buf_ofs;
        obj_image->image.offsets[1] = srf_buf_ofs + obj_surface->height * obj_surface->psb_surface->stride;
        obj_image->image.offsets[2] = srf_buf_ofs + obj_surface->height * obj_surface->psb_surface->stride * 3 / 2;
        obj_image->image.num_palette_entries = 0;
        obj_image->image.entry_bytes = 0;
        obj_image->image.component_order[0] = 'Y';
        obj_image->image.component_order[1] = 'V';/* fixed me: packed UV packed here! */
        obj_image->image.component_order[2] = 'U';
        obj_image->image.component_order[3] = '\0';
        break;
    }
    case VA_FOURCC_YV32: {
        obj_image->image.num_planes = 3;
        obj_image->image.pitches[0] = obj_surface->psb_surface->stride;
        obj_image->image.pitches[1] = obj_surface->psb_surface->stride;
        obj_image->image.pitches[2] = obj_surface->psb_surface->stride;

        obj_image->image.offsets[0] = srf_buf_ofs;
        obj_image->image.offsets[1] = srf_buf_ofs + obj_surface->height * obj_surface->psb_surface->stride;
        obj_image->image.offsets[2] = srf_buf_ofs + obj_surface->height * obj_surface->psb_surface->stride * 2;
        obj_image->image.num_palette_entries = 0;
        obj_image->image.entry_bytes = 0;
        obj_image->image.component_order[0] = 'Y';
        obj_image->image.component_order[1] = 'U';/* fixed me: packed UV packed here! */
        obj_image->image.component_order[2] = 'V';
        obj_image->image.component_order[3] = '\0';
        break;
    }
    default:
        break;
    }

    obj_image->derived_surface = surface; /* this image is derived from a surface */
    obj_surface->derived_imgcnt++;

    memcpy(image, &obj_image->image, sizeof(VAImage));

    return vaStatus;
}

VAStatus psb__destroy_image(psb_driver_data_p driver_data, object_image_p obj_image)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    if (obj_image->subpic_ref > 0) {
        vaStatus = VA_STATUS_ERROR_OPERATION_FAILED;
        return vaStatus;
    }

    object_surface_p obj_surface = SURFACE(obj_image->derived_surface);

    if (obj_surface == NULL) { /* destroy the buffer */
        object_buffer_p obj_buffer = BUFFER(obj_image->image.buf);
        CHECK_BUFFER(obj_buffer);
        psb__suspend_buffer(driver_data, obj_buffer);
    } else {
        object_buffer_p obj_buffer = BUFFER(obj_image->image.buf);
        object_heap_free(&driver_data->buffer_heap, &obj_buffer->base);
        obj_surface->derived_imgcnt--;
    }
    object_heap_free(&driver_data->image_heap, (object_base_p) obj_image);

    return VA_STATUS_SUCCESS;
}

VAStatus psb_DestroyImage(
    VADriverContextP ctx,
    VAImageID image
)
{
    INIT_DRIVER_DATA
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    object_image_p obj_image;

    obj_image = IMAGE(image);
    CHECK_IMAGE(obj_image);
    return psb__destroy_image(driver_data, obj_image);
}

VAStatus psb_SetImagePalette(
    VADriverContextP ctx,
    VAImageID image,
    /*
     * pointer to an array holding the palette data.  The size of the array is
     * num_palette_entries * entry_bytes in size.  The order of the components
     * in the palette is described by the component_order in VAImage struct
     */
    unsigned char *palette
)
{
    INIT_DRIVER_DATA;
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    object_image_p obj_image = IMAGE(image);
    CHECK_IMAGE(obj_image);

    if (obj_image->image.format.fourcc != VA_FOURCC_AI44) {
        /* only support AI44 palette */
        vaStatus = VA_STATUS_ERROR_OPERATION_FAILED;
        return vaStatus;
    }

    if (obj_image->image.num_palette_entries > 16) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "image.num_palette_entries(%d) is too big\n", obj_image->image.num_palette_entries);
        memcpy(obj_image->palette, palette, 16);
    } else
        memcpy(obj_image->palette, palette, obj_image->image.num_palette_entries * sizeof(unsigned int));

    return vaStatus;
}

static VAStatus lnc_unpack_topaz_rec(int src_width, int src_height,
                                     unsigned char *p_srcY, unsigned char *p_srcUV,
                                     unsigned char *p_dstY, unsigned char *p_dstU, unsigned char *p_dstV,
                                     int dstY_stride, int dstU_stride, int dstV_stride,
                                     int surface_height)
{
    unsigned char *tmp_dstY = NULL;
    unsigned char *tmp_dstUV = NULL;

    int n, i, index;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Unpack reconstructed frame to image\n");

    /* do this one column at a time. */
    tmp_dstY = (unsigned char *)calloc(1, 16 * src_height);
    if (tmp_dstY == NULL)
        return  VA_STATUS_ERROR_ALLOCATION_FAILED;

    tmp_dstUV = (unsigned char*)calloc(1, 16 * src_height / 2);
    if (tmp_dstUV == NULL) {
        free(tmp_dstY);
        return VA_STATUS_ERROR_ALLOCATION_FAILED;
    }

    /*  Copy Y data */
    for (n = 0; n < src_width / 16; n++) {
        memcpy((void*)tmp_dstY, p_srcY, 16 * src_height);
        p_srcY += (16 * surface_height);
        for (i = 0; i < src_height; i++) {
            memcpy(p_dstY + dstY_stride * i + n * 16, tmp_dstY + 16 * i, 16);
        }
    }

    /* Copy U/V data */
    for (n = 0; n < src_width / 16; n++) {
        memcpy((void*)tmp_dstUV, p_srcUV, 16 * src_height / 2);
        p_srcUV += (16 * surface_height / 2);
        for (i = 0; i < src_height / 2; i++) {
            for (index = 0; index < 8; index++) {
                p_dstU[i*dstU_stride + n*8 + index] = tmp_dstUV[index*2 + i*16];
                p_dstV[i*dstV_stride + n*8 + index] = tmp_dstUV[index*2 + i*16+1];
            }
        }
    }
    if (tmp_dstY)
        free(tmp_dstY);
    if (tmp_dstUV)
        free(tmp_dstUV);

    return VA_STATUS_SUCCESS;
}

/*
* Convert the memroy format from tiled to linear
*/
static VAStatus tng_unpack_vsp_rec(
    int src_width, int src_height,
    unsigned char *p_srcY, unsigned char *p_srcUV,
    unsigned char *p_dstY, unsigned char *p_dstU,
    int dstY_stride, int dstU_stride, int __maybe_unused dstV_stride,
    int __maybe_unused surface_height)
{
    unsigned char *tmp_dstY = p_dstY;
    unsigned char *tmp_dstU = p_dstU;

    int n, t,x,y;

   //"	Y_address(x,y) =    Y_base + (((H            +64+63)/64) * (x/64) + (y/64))*4096 + (y%64)*64 + (x%64)
   //"	U_address(x,y) = UV_base + ((((H+1)/2+32+63)/64) * (x/32) + (y/64))*4096 + (y%64)*64 + (x%32)*2
   //"	V_address(x,y) = UV_base + ((((H+1)/2+32+63)/64) * (x/32) + (y/64))*4096 + (y%64)*64 + (x%32)*2 + 1

    /*  Copy Y data */
    for (y = 32; y < src_height + 32; y++) {
        for (x= 32;x < src_width + 32; x++) {
            * tmp_dstY++ =  *(p_srcY + (((src_height+64+63)/64) * (x/64) + (y/64))*4096 + (y%64)*64 + (x%64));
            }
            tmp_dstY += dstY_stride - src_width;
    }

    /*  Copy UV data */
    for (y = 16; y < 16 + ((src_height+1)>>1) ; y++) {
        for (x= 16;x < 16 + ( (src_width+1)>>1); x++) {
            * tmp_dstU++ = * (p_srcUV + ((((src_height+1)/2+32+63)/64) * (x/32) + (y/64))*4096 + (y%64)*64 + (x%32)*2);
            * tmp_dstU++ = * (p_srcUV + ((((src_height+1)/2+32+63)/64) * (x/32) + (y/64))*4096 + (y%64)*64 + (x%32)*2 +1);
            }
           tmp_dstU += dstU_stride - ((src_width));
    }

    return VA_STATUS_SUCCESS;
}
/*
* Convert the memroy format from tiled to linear
*/
static VAStatus tng_unpack_topaz_rec(
    int src_width, int src_height,
    unsigned char *p_srcY, unsigned char *p_srcUV,
    unsigned char *p_dstY, unsigned char *p_dstU, unsigned char *p_dstV,
    int dstY_stride, int __maybe_unused dstU_stride, int __maybe_unused dstV_stride,
    int __maybe_unused surface_height)
{
    unsigned char *tmp_dstY = p_dstY;
    unsigned char *tmp_dstU = p_dstU;
    unsigned char *tmp_dstV = p_dstV;
    unsigned char *tmp_srcY = p_srcY;
    unsigned char *tmp_srcX = p_srcUV;

    int i, j, n, t;
    int mb_src_y_w = src_width >> 4;
    int mb_src_y_h = src_height >> 5;
    int mb_src_y_p = src_height - (mb_src_y_h << 5);

    /*  Copy Y data */
    for (j = 0; j < mb_src_y_h; j++) {
        tmp_dstY = p_dstY + j * dstY_stride * 32;
        for (i = 0; i < mb_src_y_w; i++) {
            for (n = 0; n < 32; n++) {
                memcpy(tmp_dstY + dstY_stride * n, tmp_srcY,16);
                tmp_srcY += 16;
            }
            tmp_dstY += 16;
        }
    }

    if(mb_src_y_p != 0) {
        tmp_dstY = p_dstY + j * dstY_stride * 32;
        for (i = 0; i < mb_src_y_w; i++) {
            for (n = 0; n < mb_src_y_p; n++) {
                memcpy(tmp_dstY + dstY_stride * n, tmp_srcY,16);
                tmp_srcY += 16;
            }
            for (; n < 32; n++) {
                tmp_srcY += 16;
            }
            tmp_dstY += 16;
        }
    }

    for (j = 0; j < mb_src_y_h; j++) {
        tmp_dstU = p_dstU + j * dstY_stride * 16;
        for (i = 0; i < mb_src_y_w; i++) {
            for (n = 0; n < 16; n++) {
                for (t = 0; t < 16; t++) {
                    tmp_dstU[(n * dstY_stride) + t] = tmp_srcX[t];
                }
                tmp_srcX += 16;
            }
            tmp_dstU += 16;
        }
    }
    mb_src_y_p >>= 1;
    if(mb_src_y_p != 0) {
        tmp_dstU = p_dstU + j * dstY_stride * 16;
        for (i = 0; i < mb_src_y_w; i++) {
            for (n = 0; n < mb_src_y_p; n++) {
                for (t = 0; t < 16; t++) {
                   tmp_dstU[(n * dstY_stride) + t] = tmp_srcX[t];
                }
                tmp_srcX += 16;
            }

            for (; n < 16; n++) {
                tmp_srcX += 16;
            }

            tmp_dstU += 16;
        }
    }

    return VA_STATUS_SUCCESS;
}

VAStatus psb_GetImage(
    VADriverContextP ctx,
    VASurfaceID surface,
    int x,     /* coordinates of the upper left source pixel */
    int y,
    unsigned int width, /* width and height of the region */
    unsigned int height,
    VAImageID image_id
)
{
    INIT_DRIVER_DATA;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    int ret, src_x = 0, src_y = 0, dest_x = 0, dest_y = 0;

    (void)driver_data;
    (void)lnc_unpack_topaz_rec;

    object_image_p obj_image = IMAGE(image_id);
    CHECK_IMAGE(obj_image);

    if (IS_MFLD(driver_data) && (psb_CheckIEDStatus(ctx) == 1)) {
        vaStatus = VA_STATUS_ERROR_INVALID_SURFACE;
        return vaStatus;
    }

    if (obj_image->image.format.fourcc != VA_FOURCC_NV12) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "target VAImage fourcc should be NV12 or IYUV\n");
        vaStatus = VA_STATUS_ERROR_OPERATION_FAILED;
        return vaStatus;
    }

    object_surface_p obj_surface = SURFACE(surface);
    CHECK_SURFACE(obj_surface);

    psb__VAImageCheckRegion(obj_surface, &obj_image->image, &src_x, &src_y, &dest_x, &dest_y,
                            (int *)&width, (int *)&height);

    psb_surface_p psb_surface = obj_surface->psb_surface;
    unsigned char *surface_data;
    ret = psb_buffer_map(&psb_surface->buf, &surface_data);
    if (ret) {
        return VA_STATUS_ERROR_UNKNOWN;
    }

    object_buffer_p obj_buffer = BUFFER(obj_image->image.buf);
    CHECK_BUFFER(obj_buffer);

    unsigned char *image_data;
    ret = psb_buffer_map(obj_buffer->psb_buffer, &image_data);
    if (ret) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Map buffer failed\n");

        psb_buffer_unmap(&psb_surface->buf);
        return VA_STATUS_ERROR_UNKNOWN;
    }


    image_data += obj_surface->psb_surface->buf.buffer_ofs;


    switch (obj_image->image.format.fourcc) {
    case VA_FOURCC_NV12: {
        unsigned char *src_y, *src_uv, *dst_y, *dst_uv;
	unsigned char *dst_u, *dst_v;
        unsigned int i;

	/*
	 * For reconstructed frame, tiled to linear conversion
	 * must be done.
	*/
	if (obj_surface->is_ref_surface == 1) {
	    src_y = surface_data + y * psb_surface->stride + x;
	    src_uv = surface_data + ((height + 0x1f) & (~0x1f)) * width;

	    dst_y = image_data;
	    dst_u = image_data + obj_image->image.offsets[1],
	    dst_v = dst_u + (height * width) / 4;

	    tng_unpack_topaz_rec(width, height, \
				 src_y, src_uv, \
			         dst_y, dst_u, dst_v, \
			         obj_image->image.pitches[0], \
			         obj_image->image.width / 2, \
			         obj_image->image.width / 2, \
			         obj_surface->height);
	} else if (obj_surface->is_ref_surface == 2) {
	    src_y = surface_data + y * psb_surface->stride + x;
	    src_uv = surface_data + ((height + 2*32 + 63)/64*64) * ((width  + 2*32 + 63)/64*64);
	    dst_y = image_data;
	    dst_u = image_data +  obj_image->image.offsets[1];

	    tng_unpack_vsp_rec(width, height, \
				 src_y, src_uv, \
			         dst_y, dst_u, \
			         obj_image->image.pitches[0], \
			         obj_image->image.pitches[1], \
			         obj_image->image.pitches[1], \
			         obj_surface->height);
	} else{
            /* copy Y plane */
            dst_y = image_data;
            src_y = surface_data + y * psb_surface->stride + x;
            for (i = 0; i < height; i++)  {
		memcpy(dst_y, src_y, width);
		dst_y += obj_image->image.pitches[0];
		src_y += psb_surface->stride;
            }

	    /* copy UV plane */
	    dst_uv = image_data + obj_image->image.offsets[1];
            src_uv = surface_data + psb_surface->stride * obj_surface->height + (y / 2) * psb_surface->stride + x;;
            for (i = 0; i < obj_image->image.height / 2; i++) {
		memcpy(dst_uv, src_uv, width);
		dst_uv += obj_image->image.pitches[1];
		src_uv += psb_surface->stride;
	    }
	}
        break;
    }
#if 0
    case VA_FOURCC_IYUV: {
        unsigned char *source_y, *dst_y;
        unsigned char *source_uv, *source_u, *source_v, *dst_u, *dst_v;
        unsigned int i;

        if (psb_surface->extra_info[4] == VA_FOURCC_IREC) {
            /* copy Y plane */
            dst_y = image_data + obj_image->image.offsets[0] + src_y * obj_image->image.pitches[0] + src_x;
            dst_u = image_data + obj_image->image.offsets[1] + src_y * obj_image->image.pitches[1] + src_x;
            dst_v = image_data + obj_image->image.offsets[2] + src_y * obj_image->image.pitches[2] + src_x;

            source_y = surface_data + dest_y * psb_surface->stride + dest_x;
            source_uv = surface_data + obj_surface->height * psb_surface->stride
                        + dest_y * (psb_surface->stride / 2) + dest_x;

            vaStatus = lnc_unpack_topaz_rec(width, height, source_y, source_uv,
                                            dst_y, dst_u, dst_v,
                                            obj_image->image.pitches[0],
                                            obj_image->image.pitches[1],
                                            obj_image->image.pitches[2],
                                            obj_surface->height);
        }

        break;
    }
#endif
    default:
        break;
    }
    psb_buffer_unmap(obj_buffer->psb_buffer);
    psb_buffer_unmap(&psb_surface->buf);

    return vaStatus;
}

static VAStatus psb_PutImage2(
    VADriverContextP ctx,
    VASurfaceID surface,
    VAImageID image_id,
    int src_x,
    int src_y,
    unsigned int width,
    unsigned int height,
    int dest_x,
    int dest_y
)
{
    INIT_DRIVER_DATA;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    int ret;

    object_image_p obj_image = IMAGE(image_id);
    CHECK_IMAGE(obj_image);

    object_surface_p obj_surface = SURFACE(surface);
    CHECK_SURFACE(obj_surface);

    if (obj_image->image.format.fourcc != VA_FOURCC_NV12) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "target VAImage fourcc should be NV12 or IYUV\n");
        vaStatus = VA_STATUS_ERROR_OPERATION_FAILED;
        return vaStatus;
    }

    psb__VAImageCheckRegion(obj_surface, &obj_image->image, &src_x, &src_y, &dest_x, &dest_y,
                            (int *)&width, (int *)&height);

    psb_surface_p psb_surface = obj_surface->psb_surface;
    unsigned char *surface_data;
    ret = psb_buffer_map(&psb_surface->buf, &surface_data);
    if (ret) {
        return VA_STATUS_ERROR_UNKNOWN;
    }

    object_buffer_p obj_buffer = BUFFER(obj_image->image.buf);
    CHECK_BUFFER(obj_buffer);

    unsigned char *image_data;
    ret = psb_buffer_map(obj_buffer->psb_buffer, &image_data);
    if (ret) {
        psb_buffer_unmap(&psb_surface->buf);
        return VA_STATUS_ERROR_UNKNOWN;
    }

    image_data += obj_surface->psb_surface->buf.buffer_ofs;

    switch (obj_image->image.format.fourcc) {
    case VA_FOURCC_NV12: {
        unsigned char *source_y, *src_uv, *dst_y, *dst_uv;
        unsigned int i;

        /* copy Y plane */
        source_y = image_data + obj_image->image.offsets[0] + src_y * obj_image->image.pitches[0] + src_x;
        dst_y = surface_data + dest_y * psb_surface->stride + dest_x;
        for (i = 0; i < height; i++)  {
            memcpy(dst_y, source_y, width);
            source_y += obj_image->image.pitches[0];
            dst_y += psb_surface->stride;
        }

        /* copy UV plane */
        src_uv = image_data + obj_image->image.offsets[1] + (src_y / 2) * obj_image->image.pitches[1] + src_x;
        dst_uv = surface_data + psb_surface->stride * obj_surface->height + (dest_y / 2) * psb_surface->stride + dest_x;
        for (i = 0; i < obj_image->image.height / 2; i++) {
            memcpy(dst_uv, src_uv, width);
            src_uv += obj_image->image.pitches[1];
            dst_uv += psb_surface->stride;
        }
        break;
    }
#if 0
    case VA_FOURCC_IYUV: {
        char *source_y, *dst_y;
        char *source_u, *source_v, *dst_u, *dst_v;
        unsigned int i;

        /* copy Y plane */
        source_y = image_data + obj_image->image.offsets[0] + src_y * obj_image->image.pitches[0] + src_x;
        source_u = image_data + obj_image->image.offsets[1] + src_y * obj_image->image.pitches[1] + src_x;
        source_v = image_data + obj_image->image.offsets[2] + src_y * obj_image->image.pitches[2] + src_x;

        dst_y = surface_data + dest_y * psb_surface->stride + dest_x;
        dst_u = surface_data + obj_surface->height * psb_surface->stride
                + dest_y * (psb_surface->stride / 2) + dest_x;
        dst_v = surface_data + obj_surface->height * psb_surface->stride
                + (obj_surface->height / 2) * (psb_surface->stride / 2)
                + dest_y * (psb_surface->stride / 2) + dest_x;

        for (i = 0; i < height; i++)  {
            memcpy(dst_y, source_y, width);
            source_y += obj_image->image.pitches[0];
            dst_y += psb_surface->stride;
        }

        /* copy UV plane */
        for (i = 0; i < obj_image->image.height / 2; i++) {
            memcpy(dst_u, source_u, width);
            memcpy(dst_v, source_v, width);

            source_u += obj_image->image.pitches[1];
            source_v += obj_image->image.pitches[2];

            dst_u += psb_surface->stride / 2;
            dst_v += psb_surface->stride / 2;
        }
        break;
    }
#endif
    default:
        break;
    }

    psb_buffer_unmap(obj_buffer->psb_buffer);
    psb_buffer_unmap(&psb_surface->buf);

    return VA_STATUS_SUCCESS;
}


static void psb__VAImageCheckRegion2(
    object_surface_p surface,
    VAImage *image,
    int *src_x,
    int *src_y,
    unsigned int *src_width,
    unsigned int *src_height,
    int *dest_x,
    int *dest_y,
    int *dest_width,
    int *dest_height
)
{
    /* check for image */
    if (*src_x < 0) *src_x = 0;
    if (*src_x > image->width) *src_x = image->width - 1;
    if (*src_y < 0) *src_y = 0;
    if (*src_y > image->height) *src_y = image->height - 1;

    if (((*src_width) + (*src_x)) > image->width) *src_width = image->width - *src_x;
    if (((*src_height) + (*src_y)) > image->height) *src_height = image->height - *src_x;

    /* check for surface */
    if (*dest_x < 0) *dest_x = 0;
    if (*dest_x > surface->width) *dest_x = surface->width - 1;
    if (*dest_y < 0) *dest_y = 0;
    if (*dest_y > surface->height) *dest_y = surface->height - 1;

    if (((*dest_width) + (*dest_x)) > (int)surface->width) *dest_width = surface->width - *dest_x;
    if (((*dest_height) + (*dest_y)) > (int)surface->height) *dest_height = surface->height - *dest_x;
}

VAStatus psb_PutImage(
    VADriverContextP ctx,
    VASurfaceID surface,
    VAImageID image_id,
    int src_x,
    int src_y,
    unsigned int src_width,
    unsigned int src_height,
    int dest_x,
    int dest_y,
    unsigned int dest_width,
    unsigned int dest_height
)
{
    INIT_DRIVER_DATA;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    int ret;
    CHECK_INVALID_PARAM(((int)src_width == -1) ||
                        ((int)src_height == -1) ||
                        ((int)dest_width == ~0) ||
                        ((int)dest_height == ~0));

    if ((src_width == dest_width) && (src_height == dest_height)) {
        /* Shortcut if scaling is not required */
        return psb_PutImage2(ctx, surface, image_id, src_x, src_y, src_width, src_height, dest_x, dest_y);
    }

    object_image_p obj_image = IMAGE(image_id);
    CHECK_IMAGE(obj_image);

    if (obj_image->image.format.fourcc != VA_FOURCC_NV12) {
        /* only support NV12 getImage/putImage */
        vaStatus = VA_STATUS_ERROR_OPERATION_FAILED;
        return vaStatus;
    }

    object_surface_p obj_surface = SURFACE(surface);
    CHECK_SURFACE(obj_surface);

    psb__VAImageCheckRegion2(obj_surface, &obj_image->image,
                             &src_x, &src_y, &src_width, &src_height,
                             &dest_x, &dest_y, (int *)&dest_width, (int *)&dest_height);

    psb_surface_p psb_surface = obj_surface->psb_surface;
    unsigned char *surface_data;
    ret = psb_buffer_map(&psb_surface->buf, &surface_data);
    if (ret) {
        return VA_STATUS_ERROR_UNKNOWN;
    }

    object_buffer_p obj_buffer = BUFFER(obj_image->image.buf);
    CHECK_BUFFER(obj_buffer);

    unsigned char *image_data;
    ret = psb_buffer_map(obj_buffer->psb_buffer, &image_data);
    if (ret) {
        psb_buffer_unmap(&psb_surface->buf);
        return VA_STATUS_ERROR_UNKNOWN;
    }

    /* just a prototype, the algorithm is ugly and not optimized */
    switch (obj_image->image.format.fourcc) {
    case VA_FOURCC_NV12: {
        unsigned char *source_y, *dst_y;
        unsigned short *source_uv, *dst_uv;
        unsigned int i, j;
        float xratio = (float) src_width / dest_width;
        float yratio = (float) src_height / dest_height;

        /* dst_y/dst_uv: Y/UV plane of destination */
        dst_y = (unsigned char *)(surface_data + dest_y * psb_surface->stride + dest_x);
        dst_uv = (unsigned short *)(surface_data + psb_surface->stride * obj_surface->height
                                    + (dest_y / 2) * psb_surface->stride + dest_x);

        for (j = 0; j < dest_height; j++)  {
            unsigned char *dst_y_tmp = dst_y;
            unsigned short *dst_uv_tmp = dst_uv;

            for (i = 0; i < dest_width; i++)  {
                int x = (int)(i * xratio);
                int y = (int)(j * yratio);

                source_y = image_data + obj_image->image.offsets[0]
                           + (src_y + y) * obj_image->image.pitches[0]
                           + (src_x + x);
                *dst_y_tmp = *source_y;
                dst_y_tmp++;

                if (((i & 1) == 0)) {
                    source_uv = (unsigned short *)(image_data + obj_image->image.offsets[1]
                                                   + ((src_y + y) / 2) * obj_image->image.pitches[1])
                                + ((src_x + x) / 2);
                    *dst_uv_tmp = *source_uv;
                    dst_uv_tmp++;
                }
            }
            dst_y += psb_surface->stride;

            if (j & 1)
                dst_uv = (unsigned short *)((unsigned char *)dst_uv + psb_surface->stride);
        }
        break;
    }
    default:/* will not reach here */
        break;
    }

    psb_buffer_unmap(obj_buffer->psb_buffer);
    psb_buffer_unmap(&psb_surface->buf);

    return VA_STATUS_SUCCESS;
}

/*
 * Link supbicture into one surface, when update is zero, not need to
 * update the location information
 * The image informatio and its BO of subpicture will copied to surface
 * so need to update it when a vaSetSubpictureImage is called
 */
static VAStatus psb__LinkSubpictIntoSurface(
    psb_driver_data_p driver_data,
    object_surface_p obj_surface,
    object_subpic_p obj_subpic,
    short src_x,
    short src_y,
    unsigned short src_w,
    unsigned short src_h,
    short dest_x,
    short dest_y,
    unsigned short dest_w,
    unsigned short dest_h,
    int update /* update subpicture location */
)
{
    PsbVASurfaceRec *surface_subpic;
    object_image_p obj_image = IMAGE(obj_subpic->image_id);
    if (NULL == obj_image) {
        return VA_STATUS_ERROR_INVALID_IMAGE;
    }

    VAImage *image = &obj_image->image;
    object_buffer_p obj_buffer = BUFFER(image->buf);
    if (NULL == obj_buffer) {
        return VA_STATUS_ERROR_INVALID_BUFFER;
    }

    int found = 0;

    if (obj_surface->subpictures != NULL) {
        surface_subpic = (PsbVASurfaceRec *)obj_surface->subpictures;
        do {
            if (surface_subpic->subpic_id == obj_subpic->subpic_id) {
                found = 1;
                break;
            } else
                surface_subpic = surface_subpic->next;
        } while (surface_subpic);
    }

    if (found == 0) { /* new node */
        if (obj_surface->subpic_count >= PSB_SUBPIC_MAX_NUM) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "can't support so many sub-pictures for the surface\n");
            return VA_STATUS_ERROR_UNKNOWN;
        }

        surface_subpic = (PsbVASurfaceRec *)calloc(1, sizeof(*surface_subpic));
        if (NULL == surface_subpic)
            return VA_STATUS_ERROR_ALLOCATION_FAILED;
    }

    surface_subpic->subpic_id = obj_subpic->subpic_id;
    surface_subpic->fourcc = image->format.fourcc;
    surface_subpic->size = image->data_size;
    surface_subpic->bo = obj_buffer->psb_buffer->drm_buf;
    surface_subpic->bufid = wsbmKBufHandle(wsbmKBuf(obj_buffer->psb_buffer->drm_buf));
    surface_subpic->pl_flags = obj_buffer->psb_buffer->pl_flags;
    surface_subpic->subpic_flags = obj_subpic->flags;

    surface_subpic->width = image->width;
    surface_subpic->height = image->height;
    switch (surface_subpic->fourcc) {
    case VA_FOURCC_AYUV:
        surface_subpic->stride = image->pitches[0] / 4;
        break;
    case VA_FOURCC_RGBA:
        surface_subpic->stride = image->pitches[0] / 4;
        break;
    case VA_FOURCC_AI44:
        surface_subpic->stride = image->pitches[0];
        /* point to Image palette */
        surface_subpic->palette_ptr = (PsbAYUVSample8 *) & obj_image->palette[0];
        break;
    }

    if (update) {
        surface_subpic->subpic_srcx = src_x;
        surface_subpic->subpic_srcy = src_y;
        surface_subpic->subpic_dstx = dest_x;
        surface_subpic->subpic_dsty = dest_y;
        surface_subpic->subpic_srcw = src_w;
        surface_subpic->subpic_srch = src_h;
        surface_subpic->subpic_dstw = dest_w;
        surface_subpic->subpic_dsth = dest_h;
    }

    if (found == 0) { /* new node, link into the list */
        if (NULL == obj_surface->subpictures) {
            obj_surface->subpictures = (void *)surface_subpic;
        } else { /* insert as the head */
            surface_subpic->next = (PsbVASurfacePtr)obj_surface->subpictures;
            obj_surface->subpictures = (void *)surface_subpic;
        }
        obj_surface->subpic_count++;
    }

    return VA_STATUS_SUCCESS;
}


static VAStatus psb__LinkSurfaceIntoSubpict(
    object_subpic_p obj_subpic,
    VASurfaceID surface_id
)
{
    subpic_surface_s *subpic_surface;
    int found = 0;

    if (obj_subpic->surfaces != NULL) {
        subpic_surface = (subpic_surface_s *)obj_subpic->surfaces;
        do  {
            if (subpic_surface->surface_id == surface_id) {
                found = 1;
                return VA_STATUS_SUCCESS; /* reture directly */
            } else
                subpic_surface = subpic_surface->next;
        } while (subpic_surface);
    }

    /* not found */
    subpic_surface = (subpic_surface_s *)calloc(1, sizeof(*subpic_surface));
    if (NULL == subpic_surface)
        return VA_STATUS_ERROR_ALLOCATION_FAILED;

    subpic_surface->surface_id = surface_id;
    subpic_surface->next = NULL;

    if (NULL == obj_subpic->surfaces) {
        obj_subpic->surfaces = (void *)subpic_surface;
    } else { /* insert as the head */
        subpic_surface->next = (subpic_surface_p)obj_subpic->surfaces;
        obj_subpic->surfaces = (void *)subpic_surface;
    }

    return VA_STATUS_SUCCESS;
}

static VAStatus psb__DelinkSubpictFromSurface(
    object_surface_p obj_surface,
    VASubpictureID subpic_id
)
{
    PsbVASurfaceRec *surface_subpic, *pre_surface_subpic = NULL;
    int found = 0;

    if (obj_surface->subpictures != NULL) {
        surface_subpic = (PsbVASurfaceRec *)obj_surface->subpictures;
        do  {
            if (surface_subpic->subpic_id == subpic_id) {
                found = 1;
                break;
            } else {
                pre_surface_subpic = surface_subpic;
                surface_subpic = surface_subpic->next;
            }
        } while (surface_subpic);
    }

    if (found == 1) {
        if (pre_surface_subpic == NULL) { /* remove the first node */
            obj_surface->subpictures = (void *)surface_subpic->next;
        } else {
            pre_surface_subpic->next = surface_subpic->next;
        }
        free(surface_subpic);
        obj_surface->subpic_count--;
    }

    return VA_STATUS_SUCCESS;
}


static VAStatus psb__DelinkSurfaceFromSubpict(
    object_subpic_p obj_subpic,
    VASurfaceID surface_id
)
{
    subpic_surface_s *subpic_surface, *pre_subpic_surface = NULL;
    int found = 0;

    if (obj_subpic->surfaces != NULL) {
        subpic_surface = (subpic_surface_s *)obj_subpic->surfaces;
        do {
            if (subpic_surface->surface_id == surface_id) {
                found = 1;
                break;
            } else {
                pre_subpic_surface = subpic_surface;
                subpic_surface = subpic_surface->next;
            }
        } while (subpic_surface);
    }

    if (found == 1) {
        if (pre_subpic_surface == NULL) { /* remove the first node */
            obj_subpic->surfaces = (void *)subpic_surface->next;
        } else {
            pre_subpic_surface->next = subpic_surface->next;
        }
        free(subpic_surface);
    }

    return VA_STATUS_SUCCESS;
}


VAStatus psb_QuerySubpictureFormats(
    VADriverContextP __maybe_unused ctx,
    VAImageFormat *format_list,        /* out */
    unsigned int *flags,       /* out */
    unsigned int *num_formats  /* out */
)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    CHECK_INVALID_PARAM(format_list == NULL);
    CHECK_INVALID_PARAM(flags == NULL);
    CHECK_INVALID_PARAM(num_formats == NULL);

    memcpy(format_list, psb__SubpicFormat, sizeof(psb__SubpicFormat));
    *num_formats = PSB_MAX_SUBPIC_FORMATS;
    *flags = PSB_SUPPORTED_SUBPIC_FLAGS;

    return VA_STATUS_SUCCESS;
}


VAStatus psb_CreateSubpicture(
    VADriverContextP ctx,
    VAImageID image,
    VASubpictureID *subpicture   /* out */
)
{
    INIT_DRIVER_DATA;
    VASubpictureID subpicID;
    object_subpic_p obj_subpic;
    object_image_p obj_image;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAImageFormat *img_fmt;

    obj_image = IMAGE(image);
    CHECK_IMAGE(obj_image);
    CHECK_SUBPICTURE(subpicture);

    img_fmt = psb__VAImageCheckFourCC(&obj_image->image.format, psb__SubpicFormat,
                                      sizeof(psb__SubpicFormat) / sizeof(VAImageFormat));
    if (img_fmt == NULL)
        return VA_STATUS_ERROR_UNKNOWN;

    subpicID = object_heap_allocate(&driver_data->subpic_heap);
    obj_subpic = SUBPIC(subpicID);
    CHECK_ALLOCATION(obj_subpic);

    MEMSET_OBJECT(obj_subpic, struct object_subpic_s);

    obj_subpic->subpic_id = subpicID;
    obj_subpic->image_id = obj_image->image.image_id;
    obj_subpic->surfaces = NULL;
    obj_subpic->global_alpha = 255;

    obj_image->subpic_ref ++;

    *subpicture = subpicID;

    return VA_STATUS_SUCCESS;
}



VAStatus psb__destroy_subpicture(psb_driver_data_p driver_data, object_subpic_p obj_subpic)
{
    subpic_surface_s *subpic_surface = (subpic_surface_s *)obj_subpic->surfaces;
    VASubpictureID subpicture = obj_subpic->subpic_id;

    if (subpic_surface) {
        do {
            subpic_surface_s *tmp = subpic_surface;
            object_surface_p obj_surface = SURFACE(subpic_surface->surface_id);

            if (obj_surface) { /* remove subpict from surface */
                psb__DelinkSubpictFromSurface(obj_surface, subpicture);
            }
            subpic_surface = subpic_surface->next;
            free(tmp);
        } while (subpic_surface);
    }

    object_heap_free(&driver_data->subpic_heap, (object_base_p) obj_subpic);
    return VA_STATUS_SUCCESS;
}


VAStatus psb_DestroySubpicture(
    VADriverContextP ctx,
    VASubpictureID subpicture
)
{
    INIT_DRIVER_DATA;
    object_subpic_p obj_subpic;
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    obj_subpic = SUBPIC(subpicture);
    CHECK_SUBPICTURE(obj_subpic);

    return psb__destroy_subpicture(driver_data, obj_subpic);
}

VAStatus psb_SetSubpictureImage(
    VADriverContextP ctx,
    VASubpictureID subpicture,
    VAImageID image
)
{
    INIT_DRIVER_DATA;
    object_subpic_p obj_subpic;
    object_image_p obj_image;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    subpic_surface_s *subpic_surface;
    VAImageFormat *img_fmt;

    obj_image = IMAGE(image);
    CHECK_IMAGE(obj_image);

    img_fmt = psb__VAImageCheckFourCC(&obj_image->image.format,
                                      psb__SubpicFormat,
                                      sizeof(psb__SubpicFormat) / sizeof(VAImageFormat));
    CHECK_IMAGE(img_fmt);

    obj_subpic = SUBPIC(subpicture);
    CHECK_SUBPICTURE(obj_subpic);

    object_image_p old_obj_image = IMAGE(obj_subpic->image_id);
    if (old_obj_image) {
        old_obj_image->subpic_ref--;/* decrease reference count */
    }

    /* reset the image */
    obj_subpic->image_id = obj_image->image.image_id;
    obj_image->subpic_ref ++;

    /* relink again */
    if (obj_subpic->surfaces != NULL) {
        /* the subpicture already linked into surfaces
         * so not check the return value of psb__LinkSubpictIntoSurface
         */
        subpic_surface = (subpic_surface_s *)obj_subpic->surfaces;
        do {
            object_surface_p obj_surface = SURFACE(subpic_surface->surface_id);
            CHECK_SURFACE(obj_surface);

            psb__LinkSubpictIntoSurface(driver_data, obj_surface, obj_subpic,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0 /* not update location */
                                       );
            subpic_surface = subpic_surface->next;
        } while (subpic_surface);
    }


    return VA_STATUS_SUCCESS;
}


VAStatus psb_SetSubpictureChromakey(
    VADriverContextP ctx,
    VASubpictureID subpicture,
    unsigned int chromakey_min,
    unsigned int chromakey_max,
    unsigned int chromakey_mask
)
{
    INIT_DRIVER_DATA;
    (void)driver_data;
    /* TODO */
    if ((chromakey_mask < chromakey_min) || (chromakey_mask > chromakey_max)) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Invalid chromakey value %d, chromakey value should between min and max\n", chromakey_mask);
        return VA_STATUS_ERROR_INVALID_PARAMETER;
    }
    object_subpic_p obj_subpic = SUBPIC(subpicture);
    if (NULL == obj_subpic) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Invalid subpicture value %d\n", subpicture);
        return VA_STATUS_ERROR_INVALID_SUBPICTURE;
    }

    return VA_STATUS_SUCCESS;
}

VAStatus psb_SetSubpictureGlobalAlpha(
    VADriverContextP ctx,
    VASubpictureID subpicture,
    float global_alpha
)
{
    INIT_DRIVER_DATA;

    if (global_alpha < 0 || global_alpha > 1) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Invalid global alpha value %07f, global alpha value should between 0 and 1\n", global_alpha);
        return VA_STATUS_ERROR_INVALID_PARAMETER;
    }

    object_subpic_p obj_subpic = SUBPIC(subpicture);
    if (NULL == obj_subpic) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Invalid subpicture value %d\n", subpicture);
        return VA_STATUS_ERROR_INVALID_SUBPICTURE;
    }

    obj_subpic->global_alpha = global_alpha * 255;

    return VA_STATUS_SUCCESS;
}


VAStatus psb__AssociateSubpicture(
    VADriverContextP ctx,
    VASubpictureID subpicture,
    VASurfaceID *target_surfaces,
    int num_surfaces,
    short src_x, /* upper left offset in subpicture */
    short src_y,
    unsigned short src_w,
    unsigned short src_h,
    short dest_x, /* upper left offset in surface */
    short dest_y,
    unsigned short dest_w,
    unsigned short dest_h,
    /*
     * whether to enable chroma-keying or global-alpha
     * see VA_SUBPICTURE_XXX values
     */
    unsigned int flags
)
{
    INIT_DRIVER_DATA;

    object_subpic_p obj_subpic;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    int i;

    CHECK_INVALID_PARAM(num_surfaces <= 0);

    obj_subpic = SUBPIC(subpicture);
    CHECK_SUBPICTURE(obj_subpic);
    CHECK_SURFACE(target_surfaces);

    if (flags & ~PSB_SUPPORTED_SUBPIC_FLAGS) {
#ifdef VA_STATUS_ERROR_FLAG_NOT_SUPPORTED
        vaStatus = VA_STATUS_ERROR_FLAG_NOT_SUPPORTED;
#else
        vaStatus = VA_STATUS_ERROR_UNKNOWN;
#endif
        DEBUG_FAILURE;
        return vaStatus;
    } else {

        /* If flags are ok, copy them to the subpicture object */
        obj_subpic->flags = flags;

    }

    /* Validate input params */
    for (i = 0; i < num_surfaces; i++) {
        object_surface_p obj_surface = SURFACE(target_surfaces[i]);
        CHECK_SURFACE(obj_surface);
    }

    VASurfaceID *surfaces = target_surfaces;
    for (i = 0; i < num_surfaces; i++) {
        object_surface_p obj_surface = SURFACE(*surfaces);
        if (obj_surface) {
            vaStatus = psb__LinkSubpictIntoSurface(driver_data, obj_surface, obj_subpic,
                                                   src_x, src_y, src_w, src_h,
                                                   dest_x, dest_y, dest_w, dest_h, 1);
            if (VA_STATUS_SUCCESS == vaStatus) {
                vaStatus = psb__LinkSurfaceIntoSubpict(obj_subpic, *surfaces);
            }
            CHECK_VASTATUS();/* failed with malloc */
        } else {
            /* Should never get here */
            drv_debug_msg(VIDEO_DEBUG_ERROR, "Invalid surfaces,SurfaceID=0x%x\n", *surfaces);
        }

        surfaces++;
    }

    return VA_STATUS_SUCCESS;
}


VAStatus psb_AssociateSubpicture(
    VADriverContextP ctx,
    VASubpictureID subpicture,
    VASurfaceID *target_surfaces,
    int num_surfaces,
    short src_x, /* upper left offset in subpicture */
    short src_y,
    unsigned short src_width,
    unsigned short src_height,
    short dest_x, /* upper left offset in surface */
    short dest_y,
    unsigned short dest_width,
    unsigned short dest_height,
    /*
     * whether to enable chroma-keying or global-alpha
     * see VA_SUBPICTURE_XXX values
     */
    unsigned int flags
)
{
    return psb__AssociateSubpicture(ctx, subpicture, target_surfaces, num_surfaces,
                                    src_x, src_y, src_width, src_height,
                                    dest_x, dest_y, dest_width, dest_height,
                                    flags
                                   );
}


VAStatus psb_DeassociateSubpicture(
    VADriverContextP ctx,
    VASubpictureID subpicture,
    VASurfaceID *target_surfaces,
    int num_surfaces
)
{
    INIT_DRIVER_DATA;

    object_subpic_p obj_subpic;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    object_image_p obj_image;
    int i;

    CHECK_INVALID_PARAM(num_surfaces <= 0);

    obj_subpic = SUBPIC(subpicture);
    CHECK_SUBPICTURE(obj_subpic);
    CHECK_SURFACE(target_surfaces);

    VASurfaceID *surfaces = target_surfaces;
    for (i = 0; i < num_surfaces; i++) {
        object_surface_p obj_surface = SURFACE(*surfaces);

        if (obj_surface) {
            psb__DelinkSubpictFromSurface(obj_surface, subpicture);
            psb__DelinkSurfaceFromSubpict(obj_subpic, obj_surface->surface_id);
        } else {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "vaDeassociateSubpicture: Invalid surface, VASurfaceID=0x%08x\n", *surfaces);
        }

        surfaces++;
    }

    obj_image = IMAGE(obj_subpic->image_id);
    if (obj_image)
        obj_image->subpic_ref--;/* decrease reference count */

    return VA_STATUS_SUCCESS;
}


void psb_SurfaceDeassociateSubpict(
    psb_driver_data_p driver_data,
    object_surface_p obj_surface
)
{
    PsbVASurfaceRec *surface_subpic = (PsbVASurfaceRec *)obj_surface->subpictures;

    if (surface_subpic != NULL) {
        do  {
            PsbVASurfaceRec *tmp = surface_subpic;
            object_subpic_p obj_subpic = SUBPIC(surface_subpic->subpic_id);
            if (obj_subpic)
                psb__DelinkSurfaceFromSubpict(obj_subpic, obj_surface->surface_id);
            surface_subpic = surface_subpic->next;
            free(tmp);
        } while (surface_subpic);
    }
}


static  VADisplayAttribute psb__DisplayAttribute[] = {
    {
        VADisplayAttribBrightness,
        BRIGHTNESS_MIN,
        BRIGHTNESS_MAX,
        BRIGHTNESS_DEFAULT_VALUE,
        VA_DISPLAY_ATTRIB_GETTABLE | VA_DISPLAY_ATTRIB_SETTABLE
    },

    {
        VADisplayAttribContrast,
        CONTRAST_MIN,
        CONTRAST_MAX,
        CONTRAST_DEFAULT_VALUE,
        VA_DISPLAY_ATTRIB_GETTABLE | VA_DISPLAY_ATTRIB_SETTABLE
    },

    {
        VADisplayAttribHue,
        HUE_MIN,
        HUE_MAX,
        HUE_DEFAULT_VALUE,
        VA_DISPLAY_ATTRIB_GETTABLE | VA_DISPLAY_ATTRIB_SETTABLE
    },

    {
        VADisplayAttribSaturation,
        SATURATION_MIN,
        SATURATION_MAX,
        SATURATION_DEFAULT_VALUE,
        VA_DISPLAY_ATTRIB_GETTABLE | VA_DISPLAY_ATTRIB_SETTABLE
    },
    {
        VADisplayAttribBackgroundColor,
        0x00000000,
        0xffffffff,
        0x00000000,
        VA_DISPLAY_ATTRIB_GETTABLE | VA_DISPLAY_ATTRIB_SETTABLE
    },
    {
        VADisplayAttribRotation,
        VA_ROTATION_NONE,
        VA_ROTATION_270,
        VA_ROTATION_NONE,
        VA_DISPLAY_ATTRIB_GETTABLE | VA_DISPLAY_ATTRIB_SETTABLE
    },
    {
        VADisplayAttribOutofLoopDeblock,
        VA_OOL_DEBLOCKING_FALSE,
        VA_OOL_DEBLOCKING_TRUE,
        VA_OOL_DEBLOCKING_FALSE,
        VA_DISPLAY_ATTRIB_GETTABLE | VA_DISPLAY_ATTRIB_SETTABLE
    },
    {
        VADisplayAttribBlendColor,
        0x00000000,
        0xffffffff,
        0x00000000,
        VA_DISPLAY_ATTRIB_GETTABLE | VA_DISPLAY_ATTRIB_SETTABLE
    },
    {
        VADisplayAttribOverlayColorKey,
        0x00000000,
        0xffffffff,
        0x00000000,
        VA_DISPLAY_ATTRIB_GETTABLE | VA_DISPLAY_ATTRIB_SETTABLE
    },
    {
        VADisplayAttribOverlayAutoPaintColorKey,
        0x00000000,
        0xffffffff,
        0x00000000,
        VA_DISPLAY_ATTRIB_GETTABLE | VA_DISPLAY_ATTRIB_SETTABLE
    },
    {
        VADisplayAttribCSCMatrix,
        0x00000000,
        0xffffffff,
        0x00000000,
        VA_DISPLAY_ATTRIB_GETTABLE | VA_DISPLAY_ATTRIB_SETTABLE
    },
    {
        VADisplayAttribRenderDevice,
        0x00000000,
        0xffffffff,
        0x00000000,
        VA_DISPLAY_ATTRIB_GETTABLE | VA_DISPLAY_ATTRIB_SETTABLE
    },
    {
        VADisplayAttribRenderMode,
        0x00000000,
        0xffffffff,
        0x00000000,
        VA_DISPLAY_ATTRIB_GETTABLE | VA_DISPLAY_ATTRIB_SETTABLE
    },
    {
        VADisplayAttribRenderRect,
        0x00000000,
        0xffffffff,
        0x00000000,
        VA_DISPLAY_ATTRIB_GETTABLE | VA_DISPLAY_ATTRIB_SETTABLE
    }
};

/*
 * Query display attributes
 * The caller must provide a "attr_list" array that can hold at
 * least vaMaxNumDisplayAttributes() entries. The actual number of attributes
 * returned in "attr_list" is returned in "num_attributes".
 */
VAStatus psb_QueryDisplayAttributes(
    VADriverContextP __maybe_unused ctx,
    VADisplayAttribute *attr_list,      /* out */
    int *num_attributes         /* out */
)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    CHECK_INVALID_PARAM(attr_list == NULL);
    CHECK_INVALID_PARAM(num_attributes == NULL);

    *num_attributes = min(*num_attributes, PSB_MAX_DISPLAY_ATTRIBUTES);
    memcpy(attr_list, psb__DisplayAttribute, (*num_attributes)*sizeof(VADisplayAttribute));
    return VA_STATUS_SUCCESS;
}

/*
 * Get display attributes
 * This function returns the current attribute values in "attr_list".
 * Only attributes returned with VA_DISPLAY_ATTRIB_GETTABLE set in the "flags" field
 * from vaQueryDisplayAttributes() can have their values retrieved.
 */
VAStatus psb_GetDisplayAttributes(
    VADriverContextP ctx,
    VADisplayAttribute *attr_list,      /* in/out */
    int num_attributes
)
{
    INIT_DRIVER_DATA;
    VADisplayAttribute *p = attr_list;
    int i;
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    CHECK_INVALID_PARAM(attr_list == NULL);
    CHECK_INVALID_PARAM(num_attributes <= 0);

    for (i = 0; i < num_attributes; i++) {
        switch (p->type) {
        case VADisplayAttribBrightness:
            /* -50*(1<<10) ~ 50*(1<<10) ==> 0~100*/
            p->value = (driver_data->brightness.value / (1 << 10)) + 50;
            p->min_value = 0;
            p->max_value = 100;
            break;
        case VADisplayAttribContrast:
            /* 0 ~ 2*(1<<25) ==> 0~100 */
            p->value = (driver_data->contrast.value / (1 << 25)) * 50;
            p->min_value = 0;
            p->max_value = 100;
            break;
        case VADisplayAttribHue:
            /* -30*(1<<25) ~ 30*(1<<25) ==> 0~100*/
            p->value = ((driver_data->hue.value / (1 << 25)) + 30) * 10 / 6;
            p->min_value = 0;
            p->max_value = 100;
            break;
        case VADisplayAttribSaturation:
            /* 0 ~ 2*(1<<25) ==> 0~100 */
            p->value = (driver_data->saturation.value / (1 << 25)) * 50;
            p->min_value = 0;
            p->max_value = 100;
            break;
        case VADisplayAttribBackgroundColor:
            p->value = driver_data->clear_color;
            break;
        case VADisplayAttribBlendColor:
            p->value = driver_data->blend_color;
            break;
        case VADisplayAttribOverlayColorKey:
            p->value = driver_data->color_key;
            p->min_value = 0;
            p->max_value = 0xFFFFFF;
            break;
        case VADisplayAttribOverlayAutoPaintColorKey:
            p->value = driver_data->overlay_auto_paint_color_key;
            p->min_value = 0;
            p->max_value = 1;
            break;
        case VADisplayAttribRotation:
            p->value = driver_data->va_rotate = p->value;
            p->min_value = VA_ROTATION_NONE;
            p->max_value = VA_ROTATION_270;
            break;
        case VADisplayAttribOutofLoopDeblock:
            p->value = driver_data->is_oold = p->value;
            p->min_value = VA_OOL_DEBLOCKING_FALSE;
            p->max_value = VA_OOL_DEBLOCKING_TRUE;
            break;
        case VADisplayAttribCSCMatrix:
            p->value = driver_data->load_csc_matrix = p->value;
            p->min_value = 0;
            p->max_value = 255;
            break;
        case VADisplayAttribRenderDevice:
            p->value = driver_data->render_device = p->value;
            p->min_value = 0;
            p->max_value = 255;
            break;
        case VADisplayAttribRenderMode:
            p->value = driver_data->render_mode = p->value;
            p->min_value = 0;
            p->max_value = 255;
            break;
        case VADisplayAttribRenderRect:
            ((VARectangle *)(p->value))->x = driver_data->render_rect.x = ((VARectangle *)(p->value))->x;
            ((VARectangle *)(p->value))->y = driver_data->render_rect.y = ((VARectangle *)(p->value))->y;
            ((VARectangle *)(p->value))->width = driver_data->render_rect.width = ((VARectangle *)(p->value))->width;
            ((VARectangle *)(p->value))->height = driver_data->render_rect.height = ((VARectangle *)(p->value))->height;
            p->min_value = 0;
            p->max_value = 255;
            break;

        default:
            break;
        }
        p++;
    }

    return VA_STATUS_SUCCESS;
}

/*
 * Set display attributes
 * Only attributes returned with VA_DISPLAY_ATTRIB_SETTABLE set in the "flags" field
 * from vaQueryDisplayAttributes() can be set.  If the attribute is not settable or
 * the value is out of range, the function returns VA_STATUS_ERROR_ATTR_NOT_SUPPORTED
 */
#define CLAMP_ATTR(a,max,min) (a>max?max:(a<min?min:a))
VAStatus psb_SetDisplayAttributes(
    VADriverContextP ctx,
    VADisplayAttribute *attr_list,
    int num_attributes
)
{
    INIT_DRIVER_DATA;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    struct psb_texture_s *texture_priv = &driver_data->ctexture_priv;
    PsbPortPrivPtr overlay_priv = (PsbPortPrivPtr)(&driver_data->coverlay_priv);
    int j, k;

    CHECK_INVALID_PARAM(attr_list == NULL);

    VADisplayAttribute *p = attr_list;
    int i, update_coeffs = 0;
    unsigned int *p_tmp;

    if (num_attributes <= 0) {
        return VA_STATUS_ERROR_INVALID_PARAMETER;
    }

    for (i = 0; i < num_attributes; i++) {
        switch (p->type) {
        case VADisplayAttribBrightness:
            /* 0~100 ==> -50*(1<<10) ~ 50*(1<<10)*/
            driver_data->brightness.value = (p->value - 50) * (1 << 10);
            /* 0~100 ==> -50~50 */
            overlay_priv->brightness.Value = texture_priv->brightness.Value = p->value - 50;
            update_coeffs = 1;
            break;
        case VADisplayAttribContrast:
            /* 0~100 ==> 0 ~ 2*(1<<25) */
            driver_data->contrast.value = (p->value / 50) * (1 << 25);
            /* 0~100 ==> -100~100 */
            overlay_priv->contrast.Value = texture_priv->contrast.Value = p->value * 2 - 100;
            update_coeffs = 1;
            break;
        case VADisplayAttribHue:
            /* 0~100 ==> -30*(1<<25) ~ 30*(1<<25) */
            driver_data->hue.value = ((p->value * 2 - 100) * 3 / 10) * (1 << 25);
            /* 0~100 ==> -30~30 */
            overlay_priv->hue.Value = texture_priv->hue.Value = (p->value * 2 - 100) * 3 / 10;
            update_coeffs = 1;
            break;
        case VADisplayAttribSaturation:
            /* 0~100 ==> 0 ~ 2*(1<<25) */
            driver_data->contrast.value = (p->value / 50) * (1 << 25);
            /* 0~100 ==> 100~200 */
            overlay_priv->saturation.Value = texture_priv->saturation.Value = p->value + 100;
            update_coeffs = 1;
            break;
        case VADisplayAttribBackgroundColor:
            driver_data->clear_color = p->value;
            break;
        case VADisplayAttribOutofLoopDeblock:
            driver_data->is_oold = p->value;
            break;
        case VADisplayAttribRotation:
            driver_data->va_rotate = p->value;
            driver_data->rotation_dirty |= PSB_NEW_VA_ROTATION;
            break;

        case VADisplayAttribCSCMatrix:
            driver_data->load_csc_matrix = 1;
            p_tmp = (unsigned int *)p->value;
            for (j = 0; j < CSC_MATRIX_Y; j++)
                for (k = 0; k < CSC_MATRIX_X; k++) {
                    if (p_tmp)
                        driver_data->csc_matrix[j][k] = *p_tmp;
                   p_tmp++; 
                }
            break;

        case VADisplayAttribBlendColor:
            driver_data->blend_color = p->value;
            break;
        case VADisplayAttribOverlayColorKey:
            overlay_priv->colorKey = driver_data->color_key = p->value;
            break;
        case VADisplayAttribOverlayAutoPaintColorKey:
            driver_data->overlay_auto_paint_color_key = p->value;
            break;

        case VADisplayAttribRenderDevice:
            driver_data->render_device = p->value & VA_RENDER_DEVICE_MASK;
        case VADisplayAttribRenderMode:
#ifndef ANDROID
            if (p->value & VA_RENDER_MODE_EXTERNAL_GPU) {
                drv_debug_msg(VIDEO_DEBUG_ERROR, "%s:Invalid parameter.VARenderModeExternalGPU is not supported.\n", __FUNCTION__);
                return VA_STATUS_ERROR_INVALID_PARAMETER;
            }
            if (((p->value & VA_RENDER_MODE_LOCAL_OVERLAY) && (p->value & VA_RENDER_MODE_LOCAL_GPU)) ||
                ((p->value & VA_RENDER_MODE_EXTERNAL_OVERLAY) && (p->value & VA_RENDER_MODE_EXTERNAL_GPU))) {
                drv_debug_msg(VIDEO_DEBUG_ERROR, "%s:Invalid parameter. Conflict setting for VADisplayAttribRenderMode.\n", __FUNCTION__);
                return VA_STATUS_ERROR_INVALID_PARAMETER;
            }
#endif
            driver_data->render_mode = p->value & VA_RENDER_MODE_MASK;
            break;
        case VADisplayAttribRenderRect:
            driver_data->render_rect.x = ((VARectangle *)(p->value))->x;
            driver_data->render_rect.y = ((VARectangle *)(p->value))->y;
            driver_data->render_rect.width = ((VARectangle *)(p->value))->width;
            driver_data->render_rect.height = ((VARectangle *)(p->value))->height;
            break;

        default:
            break;
        }
        p++;
    }

    if (update_coeffs) {
        /* TODO */
#ifndef ANDROID
        texture_priv->update_coeffs = 1;
#endif
    }

    return VA_STATUS_SUCCESS;
}

