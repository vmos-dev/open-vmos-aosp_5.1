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
 *    Zhaohan Ren  <zhaohan.ren@intel.com>
 *    Shengquan Yuan  <shengquan.yuan@intel.com>
 *
 */

#include <stdio.h>
#include <math.h>

#include <psb_drm.h>
#include <va/va_backend.h>
#include <va/va_dricommon.h>

#include <wsbm/wsbm_manager.h>
#include <X11/Xlib.h>
#include <X11/X.h>

#include "psb_drv_video.h"
#include "psb_x11.h"
#include "psb_output.h"
#include "psb_xrandr.h"
#include "psb_surface_ext.h"

#include "psb_texture.h"
#include "psb_drv_debug.h"

#define INIT_DRIVER_DATA    psb_driver_data_p driver_data = (psb_driver_data_p) ctx->pDriverData;
#define INIT_OUTPUT_PRIV    psb_x11_output_p output = (psb_x11_output_p)(((psb_driver_data_p)ctx->pDriverData)->ws_priv)

#define SURFACE(id) ((object_surface_p) object_heap_lookup( &driver_data->surface_heap, id ))
#define BUFFER(id)  ((object_buffer_p) object_heap_lookup( &driver_data->buffer_heap, id ))
#define IMAGE(id)  ((object_image_p) object_heap_lookup( &driver_data->image_heap, id ))
#define SUBPIC(id)  ((object_subpic_p) object_heap_lookup( &driver_data->subpic_heap, id ))
#define CONTEXT(id) ((object_context_p) object_heap_lookup( &driver_data->context_heap, id ))

static VAStatus psb_extend_dri_init(VADriverContextP ctx, unsigned int destx, unsigned desty, unsigned destw, unsigned desth)
{
    INIT_DRIVER_DATA;
    INIT_OUTPUT_PRIV;

    int i, ret;
    union dri_buffer *extend_dri_buffer;
    PPVR2DMEMINFO dri2_bb_export_meminfo;

    struct psb_texture_s *texture_priv = &driver_data->ctexture_priv;

    output->extend_drawable = (Window)psb_xrandr_create_full_screen_window(destx, desty, destw, desth);
    if (!output->extend_drawable) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: Failed to create drawable for extend display # %d\n", __func__, output->extend_drawable);
    }

    if (texture_priv->extend_dri_drawable) {
        free_drawable(ctx, texture_priv->extend_dri_drawable);
        texture_priv->extend_dri_drawable = NULL;
    }

    texture_priv->extend_dri_drawable = dri_get_drawable(ctx, output->extend_drawable);
    if (!texture_priv->extend_dri_drawable) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s(): Failed to get extend_dri_drawable\n", __func__);
        return VA_STATUS_ERROR_UNKNOWN;
    }

    extend_dri_buffer = dri_get_rendering_buffer(ctx, texture_priv->extend_dri_drawable);
    if (!extend_dri_buffer) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s(): Failed to get extend_dri_buffer\n", __func__);
        return VA_STATUS_ERROR_UNKNOWN;
    }

    ret = PVR2DMemMap(driver_data->hPVR2DContext, 0, (PVR2D_HANDLE)extend_dri_buffer->dri2.name, &dri2_bb_export_meminfo);
    if (ret != PVR2D_OK) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s(): PVR2DMemMap failed, ret = %d\n", __func__, ret);
        return VA_STATUS_ERROR_UNKNOWN;
    }

    memcpy(&texture_priv->extend_dri2_bb_export, dri2_bb_export_meminfo->pBase, sizeof(PVRDRI2BackBuffersExport));

    for (i = 0; i < DRI2_BLIT_BUFFERS_NUM; i++) {
        ret = PVR2DMemMap(driver_data->hPVR2DContext, 0, texture_priv->extend_dri2_bb_export.hBuffers[i], &texture_priv->extend_blt_meminfo[i]);
        if (ret != PVR2D_OK) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "%s(): PVR2DMemMap failed, ret = %d\n", __func__, ret);
            return VA_STATUS_ERROR_UNKNOWN;
        }
    }

    texture_priv->extend_dri_init_flag = 1;

    return VA_STATUS_SUCCESS;
}

/* reset buffer to prevent non-video area distorting when rendering into part of a drawable */
static void psb_dri_reset_mem(VADriverContextP ctx)
{
    INIT_DRIVER_DATA;
    unsigned int i, size;
    struct dri_drawable *tmp_drawable;
    struct psb_texture_s *texture_priv = &driver_data->ctexture_priv;

    tmp_drawable = (struct dri_drawable *)texture_priv->dri_drawable;
    size = tmp_drawable->width * tmp_drawable->height * 4;

    if (!tmp_drawable->is_window) {
        memset(texture_priv->blt_meminfo_pixmap->pBase, 0x0, size);
        return;
    } else {
        if (texture_priv->dri2_bb_export.ui32Type == DRI2_BACK_BUFFER_EXPORT_TYPE_BUFFERS)
            for (i = 0; i < DRI2_BLIT_BUFFERS_NUM; i++)
                memset(texture_priv->blt_meminfo[i]->pBase, 0x0, size);
        if (texture_priv->dri2_bb_export.ui32Type == DRI2_BACK_BUFFER_EXPORT_TYPE_SWAPCHAIN)
            for (i = 0; i < DRI2_FLIP_BUFFERS_NUM; i++)
                memset(texture_priv->blt_meminfo[i]->pBase, 0x0, size);
    }

    return;
}

static VAStatus psb_dri_init(VADriverContextP ctx, Drawable draw)
{
    INIT_DRIVER_DATA;
    union dri_buffer *dri_buffer;
    PPVR2DMEMINFO dri2_bb_export_meminfo;
    struct psb_texture_s *texture_priv = &driver_data->ctexture_priv;
    struct dri_drawable *tmp_drawable;
    int i, ret;

    /* free the previous drawable buffer */
    if (texture_priv->dri_drawable) {
        free_drawable(ctx, texture_priv->dri_drawable);
        texture_priv->dri_drawable = NULL;
    }

    texture_priv->dri_drawable = dri_get_drawable(ctx, draw);
    if (!texture_priv->dri_drawable) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s(): Failed to get dri_drawable\n", __func__);
        return VA_STATUS_ERROR_UNKNOWN;
    }
    tmp_drawable = (struct dri_drawable *)texture_priv->dri_drawable;

    dri_buffer = dri_get_rendering_buffer(ctx, texture_priv->dri_drawable);
    if (!dri_buffer) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s(): Failed to get dri_buffer\n", __func__);
        return VA_STATUS_ERROR_UNKNOWN;
    }

    /* pixmap */
    if (!tmp_drawable->is_window) {
        if (texture_priv->blt_meminfo_pixmap)
            PVR2DMemFree(driver_data->hPVR2DContext, texture_priv->blt_meminfo_pixmap);

        ret = PVR2DMemMap(driver_data->hPVR2DContext, 0, (PVR2D_HANDLE)(dri_buffer->dri2.name & 0x00FFFFFF), &texture_priv->blt_meminfo_pixmap);
        if (ret != PVR2D_OK) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "%s(): PVR2DMemMap failed, ret = %d\n", __func__, ret);
            return VA_STATUS_ERROR_UNKNOWN;
        }
        /* window */
    } else {
        ret = PVR2DMemMap(driver_data->hPVR2DContext, 0, (PVR2D_HANDLE)(dri_buffer->dri2.name & 0x00FFFFFF), &dri2_bb_export_meminfo);
        if (ret != PVR2D_OK) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "%s(): PVR2DMemMap failed, ret = %d\n", __func__, ret);
            return VA_STATUS_ERROR_UNKNOWN;
        }

        memcpy(&texture_priv->dri2_bb_export, dri2_bb_export_meminfo->pBase, sizeof(PVRDRI2BackBuffersExport));

        if (texture_priv->dri2_bb_export.ui32Type == DRI2_BACK_BUFFER_EXPORT_TYPE_BUFFERS) {
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "psb_dri_init: Now map buffer, DRI2 back buffer export type: DRI2_BACK_BUFFER_EXPORT_TYPE_BUFFERS\n");

            for (i = 0; i < DRI2_BLIT_BUFFERS_NUM; i++) {
                ret = PVR2DMemMap(driver_data->hPVR2DContext, 0, texture_priv->dri2_bb_export.hBuffers[i], &texture_priv->blt_meminfo[i]);
                if (ret != PVR2D_OK) {
                    drv_debug_msg(VIDEO_DEBUG_ERROR, "%s(): PVR2DMemMap failed, ret = %d\n", __func__, ret);
                    return VA_STATUS_ERROR_UNKNOWN;
                }
            }
        } else if (texture_priv->dri2_bb_export.ui32Type == DRI2_BACK_BUFFER_EXPORT_TYPE_SWAPCHAIN) {
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "psb_dri_init: Now map buffer, DRI2 back buffer export type: DRI2_BACK_BUFFER_EXPORT_TYPE_SWAPCHAIN\n");

            for (i = 0; i < DRI2_FLIP_BUFFERS_NUM; i++) {
                ret = PVR2DMemMap(driver_data->hPVR2DContext, 0, texture_priv->dri2_bb_export.hBuffers[i], &texture_priv->flip_meminfo[i]);
                if (ret != PVR2D_OK) {
                    drv_debug_msg(VIDEO_DEBUG_ERROR, "%s(): PVR2DMemMap failed, ret = %d\n", __func__, ret);
                    return VA_STATUS_ERROR_UNKNOWN;
                }
            }
        }

        PVR2DMemFree(driver_data->hPVR2DContext, dri2_bb_export_meminfo);
    }

    texture_priv->dri_init_flag = 1;

    psb_dri_reset_mem(ctx);
    return VA_STATUS_SUCCESS;
}

VAStatus psb_putsurface_ctexture(
    VADriverContextP ctx,
    VASurfaceID surface,
    Drawable draw,
    short srcx,
    short srcy,
    unsigned short srcw,
    unsigned short srch,
    short destx,
    short desty,
    unsigned short destw,
    unsigned short desth,
    unsigned int flags /* de-interlacing flags */
)
{
    INIT_DRIVER_DATA;
    INIT_OUTPUT_PRIV;
    int ret;
    int local_crtc_x, local_crtc_y, extend_crtc_x, extend_crtc_y;
    int display_width = 0, display_height = 0, extend_display_width = 0, extend_display_height = 0;
    int surface_width, surface_height;
    psb_xrandr_location extend_location = NORMAL;
    psb_extvideo_subtitle subtitle = NOSUBTITLE;
    object_surface_p obj_surface = SURFACE(surface);
    psb_surface_p psb_surface;
    Rotation local_rotation, extend_rotation;
    psb_output_device local_device, extend_device;
    unsigned short tmp;
    struct psb_texture_s *texture_priv = &driver_data->ctexture_priv;
    struct dri_drawable *tmp_drawable;


    obj_surface = SURFACE(surface);
    if (NULL == obj_surface) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: Invalid surface ID 0x%08x.\n", __func__, surface);
        return VA_STATUS_ERROR_INVALID_SURFACE;
    }

    if (driver_data->va_rotate == VA_ROTATION_NONE) {
        psb_surface = obj_surface->psb_surface;
        surface_width = obj_surface->width;
        surface_height = obj_surface->height;
    } else {
        psb_surface = obj_surface->out_loop_surface;
        if (driver_data->va_rotate != VA_ROTATION_180) {
            tmp = srcw;
            srcw = srch;
            srch = tmp;
        }
        surface_width = obj_surface->width_r;
        surface_height = obj_surface->height_r;
    }

    if (!psb_surface)
        psb_surface = obj_surface->psb_surface;

    if (output->output_drawable != draw) {
        output->output_drawable = draw;
        if (texture_priv->dri_init_flag)
            texture_priv->drawable_update_flag = 1;
    }

    if (driver_data->use_xrandr_thread && !driver_data->xrandr_thread_id) {
        ret = psb_xrandr_thread_create(ctx);
        if (ret != 0) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: Failed to create psb xrandr thread error # %d\n", __func__, ret);
            return VA_STATUS_ERROR_UNKNOWN;
        }
    }

    if (!texture_priv->dri_init_flag) {
        texture_priv->destw_save = destw;
        texture_priv->desth_save = desth;
    } else {
        if (texture_priv->destw_save != destw || texture_priv->desth_save != desth) {
            texture_priv->destw_save = destw;
            texture_priv->desth_save = desth;
            texture_priv->drawable_update_flag = 1;
        }
    }

    ret = psb_xrandr_local_crtc_coordinate(&local_device, &local_crtc_x, &local_crtc_y, &display_width, &display_height, &local_rotation);
    if (ret != VA_STATUS_SUCCESS) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: Failed to get local crtc coordinates error # %d\n", __func__, ret);
        return VA_STATUS_ERROR_UNKNOWN;
    }

    if (!psb_xrandr_single_mode() && IS_MFLD(driver_data)) {
        ret = psb_xrandr_extend_crtc_coordinate(&extend_device, &extend_crtc_x, &extend_crtc_y, &extend_display_width, &extend_display_height, &extend_location, &extend_rotation);
        if (ret != VA_STATUS_SUCCESS) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: Failed to get extend crtc coordinates error # %d\n", __func__, ret);
            return VA_STATUS_ERROR_UNKNOWN;
        }
    }

    //Reinit DRI if screen rotates
    if (texture_priv->local_rotation_save != local_rotation) {
        texture_priv->local_rotation_save = local_rotation;
        texture_priv->dri_init_flag = 0;
    }
    if (texture_priv->extend_rotation_save != extend_rotation) {
        texture_priv->extend_rotation_save = extend_rotation;
        texture_priv->extend_dri_init_flag = 0;
    }

    if (!psb_xrandr_extvideo_mode()) {
        if (!texture_priv->dri_init_flag || texture_priv->drawable_update_flag) {
            if (texture_priv->drawable_update_flag) {
                drv_debug_msg(VIDEO_DEBUG_GENERAL, "Drawable update, reinit DRI\n");
                texture_priv->drawable_update_flag = 0;
            }

            ret = psb_dri_init(ctx, output->output_drawable);
            if (ret != VA_STATUS_SUCCESS)
                return VA_STATUS_ERROR_UNKNOWN;
        }
    }

    /* Extend video */
    if (psb_xrandr_clone_mode() && local_rotation == extend_rotation) {
        if (output->extend_drawable) {
            XDestroyWindow(ctx->native_dpy, output->extend_drawable);
            output->extend_drawable = 0;
            texture_priv->extend_dri_init_flag = 0;
        }

        drv_debug_msg(VIDEO_DEBUG_GENERAL, "psb_putsurface_ctexture: Clone Mode\n");
    } else if (psb_xrandr_extend_mode()) {
        if (output->extend_drawable) {
            XDestroyWindow(ctx->native_dpy, output->extend_drawable);
            output->extend_drawable = 0;
            texture_priv->extend_dri_init_flag = 0;
        }
        switch (extend_location) {
        case LEFT_OF:
            display_height = display_height > extend_display_height ? display_height : extend_display_height;
            if (destw > display_width + extend_display_width)
                destw = display_width + extend_display_width;
            if (desth > display_height)
                desth = display_height;
            break;
        case RIGHT_OF:
            display_height = display_height > extend_display_height ? display_height : extend_display_height;
            if (destw > display_width + extend_display_width)
                destw = display_width + extend_display_width;
            if (desth > display_height)
                desth = display_height;
            break;
        case BELOW:
            display_width = display_width > extend_display_width ? display_width : extend_display_width;
            if (destw > display_width)
                destw = display_width;
            if (desth > display_height + extend_display_height)
                desth = display_height + extend_display_height;
            break;
        case ABOVE:
            display_width = display_width > extend_display_width ? display_width : extend_display_width;
            if (destw > display_width)
                destw = display_width;
            if (desth > display_height + extend_display_height)
                desth = display_height + extend_display_height;
            break;
        case NORMAL:
        default:
            break;
        }

        drv_debug_msg(VIDEO_DEBUG_GENERAL, "psb_putsurface_ctexture: Extend Mode, Location: %08x\n", extend_location);
    } else if (psb_xrandr_extvideo_mode()) {

        unsigned int xres, yres, xoffset, yoffset, overscanmode, pannelfitting;
        psb_extvideo_center center;

        psb_xrandr_extvideo_prop(&xres, &yres, &xoffset, &yoffset, &center, &subtitle, &overscanmode, &pannelfitting);

        xres = extend_display_width - xoffset;
        yres = extend_display_height - yoffset;
        //Init DRI for extend display
        if (!texture_priv->extend_dri_init_flag) {
            ret = psb_extend_dri_init(ctx, xoffset, yoffset, xres, yres);
            if (ret != VA_STATUS_SUCCESS)
                return VA_STATUS_ERROR_UNKNOWN;
        }

        drv_debug_msg(VIDEO_DEBUG_GENERAL, "psb_putsurface_ctexture: ExtendVideo Mode, Location: %08x\n", extend_location);
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "psb_putsurface_ctexture: ExtVideo coordinate srcx= %d, srcy=%d, \
				  srcw=%d, srch=%d, destx=%d, desty=%d, destw=%d, desth=%d, cur_buffer=%d\n",
                                 srcx, srcy, srcw, srch, xoffset, yoffset, xres, yres, texture_priv->extend_current_blt_buffer);

        if (subtitle == BOTH || subtitle == ONLY_HDMI)
            psb_putsurface_textureblit(ctx, (unsigned char *)texture_priv->extend_blt_meminfo[texture_priv->extend_current_blt_buffer], surface, srcx, srcy, srcw, srch, 0, 0,
                                       xres, yres, 1,
                                       surface_width, surface_height,
                                       psb_surface->stride, psb_surface->buf.drm_buf,
                                       psb_surface->buf.pl_flags, 0 /* no wrap for dst */);
        else
            psb_putsurface_textureblit(ctx, (unsigned char *)texture_priv->extend_blt_meminfo[texture_priv->extend_current_blt_buffer], surface, srcx, srcy, srcw, srch, 0, 0,
                                       xres, yres, 0,
                                       surface_width, surface_height,
                                       psb_surface->stride, psb_surface->buf.drm_buf,
                                       psb_surface->buf.pl_flags, 0 /* no wrap for dst */);

        dri_swap_buffer(ctx, texture_priv->extend_dri_drawable);
        texture_priv->extend_current_blt_buffer = (texture_priv->extend_current_blt_buffer + 1) & 0x01;

        /* int this mode, destination retangle may be larger than MIPI resolution, if so setting it to MIPI resolution */
        if (destw > display_width)
            destw = display_width;
        if (desth > display_height)
            desth = display_height;

        /* adjust local window on MIPI, make sure it is not covered by HDMI image */
        if (!texture_priv->adjust_window_flag) {
            switch (extend_location) {
            case ABOVE:
                XMoveResizeWindow(ctx->native_dpy, output->output_drawable, 0, extend_display_height, destw, desth);
                break;
            case LEFT_OF:
                XMoveResizeWindow(ctx->native_dpy, output->output_drawable, extend_display_width, 0, destw, desth);
                break;
            case BELOW:
            case RIGHT_OF:
            case NORMAL:
            default:
                break;
            }
            XFlush(ctx->native_dpy);
            texture_priv->adjust_window_flag = 1;
        }

        if (!texture_priv->dri_init_flag || texture_priv->drawable_update_flag) {
            if (texture_priv->drawable_update_flag) {
                drv_debug_msg(VIDEO_DEBUG_GENERAL, "Drawable update, reinit DRI\n");
                texture_priv->drawable_update_flag = 0;
            }

            ret = psb_dri_init(ctx, output->output_drawable);
            if (ret != VA_STATUS_SUCCESS)
                return VA_STATUS_ERROR_UNKNOWN;

        }
    }

    /* Main Video for pixmap*/
    tmp_drawable = (struct dri_drawable *)texture_priv->dri_drawable;
    if (!tmp_drawable->is_window) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "psb_putsurface_ctexture: Main video Pixmap, coordinate: srcx= %d, srcy=%d, srcw=%d, srch=%d, destx=%d, desty=%d, destw=%d, desth=%d, cur_buffer=%d\n",
                                 srcx, srcy, srcw, srch, destx, desty, destw, desth, texture_priv->current_blt_buffer);
        if (subtitle == BOTH || (subtitle == NOSUBTITLE && obj_surface->subpic_count))
            psb_putsurface_textureblit(ctx, (unsigned char *)texture_priv->blt_meminfo_pixmap, surface, srcx, srcy, srcw, srch, destx, desty, destw, desth, 1,
                                       surface_width, surface_height,
                                       psb_surface->stride, psb_surface->buf.drm_buf,
                                       psb_surface->buf.pl_flags, 0 /* no wrap for dst */);
        else
            psb_putsurface_textureblit(ctx, (unsigned char *)texture_priv->blt_meminfo_pixmap, surface, srcx, srcy, srcw, srch, destx, desty, destw, desth, 0,
                                       surface_width, surface_height,
                                       psb_surface->stride, psb_surface->buf.drm_buf,
                                       psb_surface->buf.pl_flags, 0 /* no wrap for dst */);

        return VA_STATUS_SUCCESS;
    }

    /* Main Video for window*/
    if (texture_priv->dri2_bb_export.ui32Type == DRI2_BACK_BUFFER_EXPORT_TYPE_BUFFERS) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "psb_putsurface_ctexture: Main video, swap buffer, coordinate: srcx= %d, srcy=%d, srcw=%d, srch=%d, destx=%d, desty=%d, destw=%d, desth=%d, cur_buffer=%d\n",
                                 srcx, srcy, srcw, srch, destx, desty, destw, desth, texture_priv->current_blt_buffer);

        if (subtitle == BOTH || (subtitle == NOSUBTITLE && obj_surface->subpic_count))
            psb_putsurface_textureblit(ctx, (unsigned char *)texture_priv->blt_meminfo[texture_priv->current_blt_buffer], surface, srcx, srcy, srcw, srch, destx, desty, destw, desth, 1,
                                       surface_width, surface_height,
                                       psb_surface->stride, psb_surface->buf.drm_buf,
                                       psb_surface->buf.pl_flags, 0 /* no wrap for dst */);
        else
            psb_putsurface_textureblit(ctx, (unsigned char *)texture_priv->blt_meminfo[texture_priv->current_blt_buffer], surface, srcx, srcy, srcw, srch, destx, desty, destw, desth, 0,
                                       surface_width, surface_height,
                                       psb_surface->stride, psb_surface->buf.drm_buf,
                                       psb_surface->buf.pl_flags, 0 /* no wrap for dst */);

        dri_swap_buffer(ctx, texture_priv->dri_drawable);
        texture_priv->current_blt_buffer = (texture_priv->current_blt_buffer + 1) & 0x01;

    } else if (texture_priv->dri2_bb_export.ui32Type == DRI2_BACK_BUFFER_EXPORT_TYPE_SWAPCHAIN) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "psb_putsurface_ctexture: Main video, flip chain, coordinate: srcx= %d, srcy=%d, srcw=%d, srch=%d, destx=%d, desty=%d, destw=%d, desth=%d, cur_buffer=%d\n",
                                 srcx, srcy, srcw, srch, destx, desty, display_width, display_height, texture_priv->current_blt_buffer);

        if (subtitle == BOTH || (subtitle == NOSUBTITLE && obj_surface->subpic_count))
            psb_putsurface_textureblit(ctx, (unsigned char *)texture_priv->flip_meminfo[texture_priv->current_blt_buffer], surface, srcx, srcy, srcw, srch, destx, desty,
                                       display_width, display_height, 1, surface_width, surface_height,
                                       psb_surface->stride, psb_surface->buf.drm_buf,
                                       psb_surface->buf.pl_flags, 0 /* no wrap for dst */);
        else
            psb_putsurface_textureblit(ctx, (unsigned char *)texture_priv->flip_meminfo[texture_priv->current_blt_buffer], surface, srcx, srcy, srcw, srch, destx, desty,
                                       display_width, display_height, 0, surface_width, surface_height,
                                       psb_surface->stride, psb_surface->buf.drm_buf,
                                       psb_surface->buf.pl_flags, 0 /* no wrap for dst */);

        dri_swap_buffer(ctx, texture_priv->dri_drawable);
        texture_priv->current_blt_buffer++;
        if (texture_priv->current_blt_buffer == DRI2_FLIP_BUFFERS_NUM)
            texture_priv->current_blt_buffer = 0;
    }

    return VA_STATUS_SUCCESS;
}
