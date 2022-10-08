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
 *    Zeng Li <zeng.li@intel.com>
 *    Jason Hu <jason.hu@intel.com>
 *    Shengquan Yuan  <shengquan.yuan@intel.com>
 */

#include <va/va.h>
#include <va/va_backend.h>
#include <va/va_backend_tpi.h>
#include <va/va_backend_egl.h>
#include <va/va_drmcommon.h>
#include "psb_drv_video.h"
#include "psb_output.h"
#include "android/psb_android_glue.h"
#include "psb_drv_debug.h"
#include "vc1_defs.h"
#include "pnw_rotate.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <wsbm/wsbm_pool.h>
#include <wsbm/wsbm_manager.h>
#include <wsbm/wsbm_util.h>
#include <wsbm/wsbm_fencemgr.h>

#ifdef ANROID
#include <system/graphics.h>
#endif

#define INIT_DRIVER_DATA    psb_driver_data_p driver_data = (psb_driver_data_p) ctx->pDriverData
#define INIT_OUTPUT_PRIV    unsigned char* output = ((psb_driver_data_p)ctx->pDriverData)->ws_priv
#define SURFACE(id)    ((object_surface_p) object_heap_lookup( &driver_data->surface_heap, id ))
#define CONTEXT(id) ((object_context_p) object_heap_lookup( &driver_data->context_heap, id ))
#define CONFIG(id)  ((object_config_p) object_heap_lookup( &driver_data->config_heap, id ))

#define CHECK_SURFACE_REALLOC(psb_surface, msvdx_rotate, need)  \
do {                                                            \
    int old_rotate = GET_SURFACE_INFO_rotate(psb_surface);      \
    switch (msvdx_rotate) {                                     \
    case 2: /* 180 */                                           \
        if (old_rotate == 2)                                    \
            need = 0;                                           \
        else                                                    \
            need = 1;                                           \
        break;                                                  \
    case 1: /* 90 */                                            \
    case 3: /* 270 */                                           \
        if (old_rotate == 1 || old_rotate == 3)                 \
            need = 0;                                           \
        else                                                    \
            need = 1;                                           \
        break;                                                  \
    }                                                           \
} while (0)

static int get_surface_stride(int width, int tiling)
{
    int stride = 0;

    if (0) {
        ;
    } else if (512 >= width) {
        stride = 512;
    } else if (1024 >= width) {
        stride = 1024;
    } else if (1280 >= width) {
        stride = 1280;
#ifdef PSBVIDEO_MSVDX_DEC_TILING
        if (tiling) {
            stride = 2048;
        }
#endif
    } else if (2048 >= width) {
        stride = 2048;
    } else if (4096 >= width) {
        stride = 4096;
    } else {
        stride = (width + 0x3f) & ~0x3f;
    }

    return stride;
}
//#define OVERLAY_ENABLE_MIRROR

#ifdef PSBVIDEO_MRFL_VPP

static int isVppOn(void __maybe_unused *output) {
#ifdef TARGET_HAS_MULTIPLE_DISPLAY
    return psb_android_get_mds_vpp_state(output);
#else
    return psb_android_get_vpp_state();
#endif
}
#endif

void psb_InitOutLoop(VADriverContextP ctx)
{
    char env_value[64];
    INIT_DRIVER_DATA;

    /* VA rotate from APP */
    driver_data->va_rotate = VA_ROTATION_NONE;

    /* window manager rotation from OS */
    driver_data->mipi0_rotation = VA_ROTATION_NONE;
    driver_data->mipi1_rotation = VA_ROTATION_NONE;
    driver_data->hdmi_rotation = VA_ROTATION_NONE;

    /* final rotation of VA rotate+WM rotate */
    driver_data->local_rotation = VA_ROTATION_NONE;
    driver_data->extend_rotation = VA_ROTATION_NONE;

    /* MSVDX rotate */
    driver_data->msvdx_rotate_want = ROTATE_VA2MSVDX(VA_ROTATION_NONE);

    if (psb_parse_config("PSB_VIDEO_NOROTATE", &env_value[0]) == 0) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "MSVDX: disable MSVDX rotation\n");
        driver_data->disable_msvdx_rotate = 1;
    }
    /* FIXME: Disable rotation when VPP enabled, just a workround here*/
#ifdef PSBVIDEO_MRFL_VPP
    if (isVppOn((void*)driver_data->ws_priv)) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "For VPP: disable MSVDX rotation\n");
        driver_data->disable_msvdx_rotate = 1;
        driver_data->vpp_on = 1;
    }
#endif

#ifdef BAYTRAIL
    driver_data->disable_msvdx_rotate = 1;
#endif

    driver_data->disable_msvdx_rotate_backup = driver_data->disable_msvdx_rotate;
}

void psb_RecalcAlternativeOutput(object_context_p obj_context)
{
    psb_driver_data_p driver_data = obj_context->driver_data;
    object_surface_p obj_surface = obj_context->current_render_target;
    int angle, new_rotate, i;
    int old_rotate = driver_data->msvdx_rotate_want;
    int mode = INIT_VALUE;
#ifdef TARGET_HAS_MULTIPLE_DISPLAY
    mode = psb_android_get_mds_mode((void*)driver_data->ws_priv);
#endif

    if (mode != INIT_VALUE) {
        // clear device rotation info
        if (driver_data->mipi0_rotation != VA_ROTATION_NONE) {
            driver_data->mipi0_rotation = VA_ROTATION_NONE;
            driver_data->hdmi_rotation = VA_ROTATION_NONE;
        }
        // Disable msvdx rotation if
        // WIDI video is play and meta data rotation angle is 0
        if (mode == WIDI_VIDEO_ISPLAYING) {
            if (driver_data->va_rotate == VA_ROTATION_NONE)
                driver_data->disable_msvdx_rotate = 1;
            else {
                driver_data->mipi0_rotation = 0;
                driver_data->hdmi_rotation = 0;
                driver_data->disable_msvdx_rotate = 0;
            }
        } else {
            if (IS_MOFD(driver_data))
                driver_data->disable_msvdx_rotate = 1;
            else
                driver_data->disable_msvdx_rotate = driver_data->disable_msvdx_rotate_backup;
        }
    } else if (IS_MOFD(driver_data)) {
    /* Moorefield has overlay rotaion, so decoder doesn't generate rotation
     * output according to windows manager. It is controlled by payload info
     * in which HWC signal decoder to generate rotation output
     */
        long long hwc_timestamp = 0;
        int index = -1;

        for (i = 0; i < obj_context->num_render_targets; i++) {
            object_surface_p obj_surface = SURFACE(obj_context->render_targets[i]);
            /* traverse all surfaces' share info to find out the latest transform info */
            if (obj_surface && obj_surface->share_info) {
                if (obj_surface->share_info->hwc_timestamp > hwc_timestamp) {
                    hwc_timestamp = obj_surface->share_info->hwc_timestamp;
                    index = i;
                }
            }
        }
        if (index >= 0) {
            object_surface_p obj_surface = SURFACE(obj_context->render_targets[index]);
            if (obj_surface && obj_surface->share_info) {
                int transform = obj_surface->share_info->layer_transform;
                driver_data->mipi0_rotation = HAL2VAROTATION(transform);
                drv_debug_msg(VIDEO_DEBUG_GENERAL, "Signal from HWC to rotate %d\n", driver_data->mipi0_rotation);
            }
        }
    } else if (driver_data->native_window) {
        int display_rotate = 0;
        psb_android_surfaceflinger_rotate(driver_data->native_window, &display_rotate);
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "NativeWindow(0x%x), get surface flinger rotate %d\n", driver_data->native_window, display_rotate);

        if (driver_data->mipi0_rotation != display_rotate) {
            driver_data->mipi0_rotation = display_rotate;
        }
    } else {
        long long hwc_timestamp = 0;
        int index = -1;

        for (i = 0; i < obj_context->num_render_targets; i++) {
            object_surface_p obj_surface = SURFACE(obj_context->render_targets[i]);
            /* traverse all surfaces' share info to find out the latest transform info */
            if (obj_surface && obj_surface->share_info) {
                if (obj_surface->share_info->hwc_timestamp > hwc_timestamp) {
                    hwc_timestamp = obj_surface->share_info->hwc_timestamp;
                    index = i;
                }
            }
        }
        if (index >= 0) {
            object_surface_p obj_surface = SURFACE(obj_context->render_targets[index]);
            if (obj_surface && obj_surface->share_info) {
                int transform = obj_surface->share_info->layer_transform;
                driver_data->mipi0_rotation = HAL2VAROTATION(transform);
            }
        }
    }

#ifdef PSBVIDEO_MRFL
    if ((mode == HDMI_VIDEO_ISPLAYING) && driver_data->native_window) {
        int display_rotate = 0;
        psb_android_surfaceflinger_rotate(driver_data->native_window, &display_rotate);
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "NativeWindow(0x%x), get surface flinger rotate %d\n", driver_data->native_window, display_rotate);

        if (driver_data->mipi0_rotation != display_rotate && !IS_MOFD(driver_data)) {
            driver_data->mipi0_rotation = display_rotate;
        }
    }
#endif

    /* calc VA rotation and WM rotation, and assign to the final rotation degree */
    angle = Rotation2Angle(driver_data->va_rotate) + Rotation2Angle(driver_data->mipi0_rotation);
    driver_data->local_rotation = Angle2Rotation(angle);
    angle = Rotation2Angle(driver_data->va_rotate) + Rotation2Angle(driver_data->hdmi_rotation);
    driver_data->extend_rotation = Angle2Rotation(angle);

    /* On MOFD, no need to use meta rotation, just use rotation angle signal from HWC */
    if (IS_MOFD(driver_data)) {
        driver_data->local_rotation = driver_data->mipi0_rotation;
        driver_data->extend_rotation = Rotation2Angle(driver_data->hdmi_rotation);
    }

    /* for any case that local and extened rotation are not same, fallback to GPU */
    if ((driver_data->mipi1_rotation != VA_ROTATION_NONE) ||
        ((driver_data->local_rotation != VA_ROTATION_NONE) &&
         (driver_data->extend_rotation != VA_ROTATION_NONE) &&
         (driver_data->local_rotation != driver_data->extend_rotation))) {
        new_rotate = ROTATE_VA2MSVDX(driver_data->local_rotation);
        if (driver_data->is_android == 0) /*fallback to texblit path*/
            driver_data->output_method = PSB_PUTSURFACE_CTEXTURE;
    } else {
        if (driver_data->local_rotation == VA_ROTATION_NONE)
            new_rotate = driver_data->extend_rotation;
        else
            new_rotate = driver_data->local_rotation;

        if (driver_data->is_android == 0) {
            if (driver_data->output_method != PSB_PUTSURFACE_FORCE_CTEXTURE)
                driver_data->output_method = PSB_PUTSURFACE_COVERLAY;
        }
    }

    if (old_rotate != new_rotate) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "MSVDX: new rotation %d desired\n", new_rotate);
        driver_data->msvdx_rotate_want = new_rotate;
    }

#ifdef TARGET_HAS_MULTIPLE_DISPLAY
    int scaling_buffer_width = 1920, scaling_buffer_height = 1080 ;
    int scaling_width = 0, scaling_height = 0;
    int scaling_offset_x = 0, scaling_offset_y = 0;
    int old_bufw = 0, old_bufh = 0, old_x = 0, old_y = 0, old_w = 0, old_h = 0;
    int bScaleChanged = 0, size = 0;
    unsigned char * surface_data;

    int ret = psb_android_get_mds_decoder_output_resolution(
                (void*)driver_data->ws_priv,
                &scaling_width, &scaling_height,
                &scaling_offset_x, &scaling_offset_y,
                &scaling_buffer_width, &scaling_buffer_height);

    if ((old_bufw != scaling_buffer_width) || (old_bufh != scaling_buffer_height) ||
        (old_x != scaling_offset_x) || (old_y != scaling_offset_y) ||
        (old_w != scaling_width) || (old_h != scaling_height)) {
        bScaleChanged = 1;
    }

    old_x = scaling_offset_x;
    old_y = scaling_offset_y;
    old_w = scaling_width;
    old_h = scaling_height;
    old_bufw = scaling_buffer_width;
    old_bufh = scaling_buffer_height;

    /* turn off ved downscaling if width and height are 0.
     * Besides, scaling_width and scaling_height must be a multiple of 2.
     */
    if (!ret || (!scaling_width || !scaling_height) ||
             (scaling_width & 1) || (scaling_height & 1)) {
        obj_context->msvdx_scaling = 0;
        obj_context->scaling_width = 0;
        obj_context->scaling_height = 0;
        obj_context->scaling_offset_x= 0;
        obj_context->scaling_offset_y = 0;
        obj_context->scaling_buffer_width = 0;
        obj_context->scaling_buffer_height = 0;
    } else {
        obj_context->msvdx_scaling = 1;
        obj_context->scaling_width = scaling_width;
        obj_context->scaling_height = scaling_height;
        obj_context->scaling_offset_x= scaling_offset_x;
        obj_context->scaling_offset_y = scaling_offset_y;
        obj_context->scaling_buffer_width = scaling_buffer_width;
        obj_context->scaling_buffer_height = scaling_buffer_height;
    }
    if (bScaleChanged) {
        if ((obj_surface != NULL) &&
            (obj_surface->out_loop_surface != NULL)) {
            if (psb_buffer_map(&obj_surface->out_loop_surface->buf, &surface_data)) {
                drv_debug_msg(VIDEO_DEBUG_ERROR, "Failed to map rotation buffer before clear it");
            }
            else {
                size = obj_surface->out_loop_surface->chroma_offset;
                memset(surface_data, 0, size);
                memset(surface_data + size, 0x80, obj_surface->out_loop_surface->size - size);
                psb_buffer_unmap(&obj_context->current_render_target->out_loop_surface->buf);
            }
        }
    }
#endif
}


void psb_CheckInterlaceRotate(object_context_p obj_context, unsigned char *pic_param_tmp)
{
    object_surface_p obj_surface = obj_context->current_render_target;

    switch (obj_context->profile) {
    case VAProfileMPEG2Simple:
    case VAProfileMPEG2Main:
        break;
    case VAProfileMPEG4Simple:
    case VAProfileMPEG4AdvancedSimple:
    case VAProfileMPEG4Main:
    case VAProfileH263Baseline: {
        VAPictureParameterBufferMPEG4 *pic_params = (VAPictureParameterBufferMPEG4 *)pic_param_tmp;

        if (pic_params->vol_fields.bits.interlaced)
            obj_context->interlaced_stream = 1; /* is it the right way to check? */
        break;
    }
    case VAProfileH264Baseline:
    case VAProfileH264Main:
    case VAProfileH264High:
    case VAProfileH264ConstrainedBaseline: {
        VAPictureParameterBufferH264 *pic_params = (VAPictureParameterBufferH264 *)pic_param_tmp;
        /* is it the right way to check? */
        if (pic_params->pic_fields.bits.field_pic_flag || pic_params->seq_fields.bits.mb_adaptive_frame_field_flag)
            obj_context->interlaced_stream = 1;

        break;
    }
    case VAProfileVC1Simple:
    case VAProfileVC1Main:
    case VAProfileVC1Advanced: {
        VAPictureParameterBufferVC1 *pic_params = (VAPictureParameterBufferVC1 *)pic_param_tmp;

        /* is it the right way to check? */
        if (pic_params->sequence_fields.bits.interlace)
            obj_context->interlaced_stream = 1;

        break;
    }
    default:
        break;
    }

    if (obj_surface->share_info) {
        psb_surface_share_info_p share_info = obj_surface->share_info;
        if (obj_context->interlaced_stream) {
            SET_SURFACE_INFO_rotate(obj_surface->psb_surface, 0);
            obj_context->msvdx_rotate = 0;
            share_info->bob_deinterlace = 1;
        } else {
           share_info->bob_deinterlace = 0;
       }
    }
}
#if 0
/*
 * Detach a surface from obj_surface
 */
VAStatus psb_DestroyRotateSurface(
    VADriverContextP ctx,
    object_surface_p obj_surface,
    int rotate
)
{
    INIT_DRIVER_DATA;
    psb_surface_p psb_surface = obj_surface->out_loop_surface;
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    /* Allocate alternative output surface */
    if (psb_surface) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Try to allocate surface for alternative rotate output\n");
        psb_surface_destroy(obj_surface->out_loop_surface);
        free(psb_surface);

        obj_surface->out_loop_surface = NULL;
        obj_surface->width_r = obj_surface->width;
        obj_surface->height_r = obj_surface->height;
    }

    return vaStatus;
}
#endif
#ifdef TARGET_HAS_MULTIPLE_DISPLAY
/*
 * Create and attach a downscaling surface to obj_surface
 */
static void clearScalingInfo(psb_surface_share_info_p share_info) {
    if (share_info == NULL)
        return;
    share_info->width_s = 0;
    share_info->height_s = 0;
    share_info->scaling_khandle = 0;

    share_info->scaling_luma_stride = 0;
    share_info->scaling_chroma_u_stride = 0;
    share_info->scaling_chroma_v_stride = 0;
    return;
}

VAStatus psb_CreateScalingSurface(
        object_context_p obj_context,
        object_surface_p obj_surface
)
{
    psb_surface_p psb_surface;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    psb_surface_share_info_p share_info = obj_surface->share_info;
    unsigned int set_flags, clear_flags;
    int ret = 0;

    if (obj_context->driver_data->render_rect.width <= obj_context->scaling_width || obj_context->driver_data->render_rect.height <= obj_context->scaling_height) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Either downscaling is not required or upscaling is needed for the target resolution\n");
        obj_context->msvdx_scaling = 0; /* Disable downscaling */
        clearScalingInfo(share_info);
        return VA_STATUS_ERROR_OPERATION_FAILED;
    }

    psb_surface = obj_surface->scaling_surface;
    /* Check if downscaling resolution has been changed */
    if (psb_surface) {
        if (obj_surface->width_s != obj_context->scaling_width || obj_surface->height_s != obj_context->scaling_height) {
            psb_surface_destroy(psb_surface);
            free(psb_surface);
            psb_surface = NULL;

            drv_debug_msg(VIDEO_DEBUG_GENERAL, "downscaling buffer realloc: %d x %d -> %d x %d\n",
                    obj_surface->width_s, obj_surface->height_s, obj_context->scaling_width, obj_context->scaling_height);
            clearScalingInfo(share_info);
        }
    }

    if (!psb_surface) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Try to allocate surface for alternative scaling output: %dx%d\n",
                      obj_context->scaling_width, obj_context->scaling_height);
        psb_surface = (psb_surface_p) calloc(1, sizeof(struct psb_surface_s));
        CHECK_ALLOCATION(psb_surface);

        vaStatus = psb_surface_create(obj_context->driver_data, obj_context->scaling_width,
                                      (obj_context->scaling_height + 0x1f) & ~0x1f, VA_FOURCC_NV12,
                                      0, psb_surface);

        //set_flags = WSBM_PL_FLAG_CACHED | DRM_PSB_FLAG_MEM_MMU | WSBM_PL_FLAG_SHARED;
        //clear_flags = WSBM_PL_FLAG_UNCACHED | WSBM_PL_FLAG_WC;
        //ret = psb_buffer_setstatus(&psb_surface->buf, set_flags, clear_flags);

        if (VA_STATUS_SUCCESS != vaStatus || ret) {
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "allocate scaling buffer fail\n");
            free(psb_surface);
            obj_surface->scaling_surface = NULL;
            vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
            DEBUG_FAILURE;
            return vaStatus;
        }

        obj_surface->width_s = obj_context->scaling_width;
        obj_surface->height_s = obj_context->scaling_height;
        obj_surface->buffer_width_s = obj_context->scaling_width;
        obj_surface->buffer_height_s = obj_context->scaling_height;
        obj_surface->offset_x_s= obj_context->scaling_offset_x;
        obj_surface->offset_y_s= obj_context->scaling_offset_y;
        obj_context->scaling_update = 1;
    }
    obj_surface->scaling_surface = psb_surface;

    /* derive the protected flag from the primay surface */
    SET_SURFACE_INFO_protect(psb_surface,
                             GET_SURFACE_INFO_protect(obj_surface->psb_surface));

    /*notify hwc that rotated buffer is ready to use.
     * TODO: Do these in psb_SyncSurface()
     */
    if (share_info != NULL) {
        share_info->width_s = obj_surface->width_s;
        share_info->height_s = obj_surface->height_s;
        share_info->scaling_khandle =
        (uint32_t)(wsbmKBufHandle(wsbmKBuf(psb_surface->buf.drm_buf)));

        share_info->scaling_luma_stride = psb_surface->stride;
        share_info->scaling_chroma_u_stride = psb_surface->stride;
        share_info->scaling_chroma_v_stride = psb_surface->stride;
    }
    return vaStatus;
}
#else
VAStatus psb_CreateScalingSurface(
        object_context_p __maybe_unused obj_context,
        object_surface_p __maybe_unused obj_surface
)
{
    return VA_STATUS_ERROR_OPERATION_FAILED;
}
#endif

/*
 * Create and attach a rotate surface to obj_surface
 */
VAStatus psb_CreateRotateSurface(
    object_context_p obj_context,
    object_surface_p obj_surface,
    int msvdx_rotate
)
{
    int width, height;
    psb_surface_p rotate_surface;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    int need_realloc = 0;
    unsigned int flags = 0;
    psb_surface_share_info_p share_info = obj_surface->share_info;
    psb_driver_data_p driver_data = obj_context->driver_data;
    int rotate_stride = 0, rotate_tiling = 0;
    object_config_p obj_config = CONFIG(obj_context->config_id);
    unsigned char * surface_data;

    CHECK_CONFIG(obj_config);

    rotate_surface = obj_surface->out_loop_surface;

    if (msvdx_rotate == 0
#ifdef OVERLAY_ENABLE_MIRROR
        /*Bypass 180 degree rotate when overlay enabling mirror*/
        || msvdx_rotate == VA_ROTATION_180
#endif
        )
        return vaStatus;

    if (rotate_surface) {
        CHECK_SURFACE_REALLOC(rotate_surface, msvdx_rotate, need_realloc);
        if (need_realloc == 0) {
            goto exit;
        } else { /* free the old rotate surface */
            /*FIX ME: it is not safe to do that because surfaces may be in use for rendering.*/
            psb_surface_destroy(obj_surface->out_loop_surface);
            memset(rotate_surface, 0, sizeof(*rotate_surface));
        }
    } else {
        rotate_surface = (psb_surface_p) calloc(1, sizeof(struct psb_surface_s));
        CHECK_ALLOCATION(rotate_surface);
    }

#ifdef PSBVIDEO_MSVDX_DEC_TILING
    SET_SURFACE_INFO_tiling(rotate_surface, GET_SURFACE_INFO_tiling(obj_surface->psb_surface));
#endif
#ifdef PSBVIDEO_MRFL_VPP_ROTATE
    SET_SURFACE_INFO_rotate(rotate_surface, msvdx_rotate);
#endif
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Try to allocate surface for alternative rotate output\n");

    flags = IS_ROTATED;

    if (msvdx_rotate == 2 /* VA_ROTATION_180 */) {
        width = obj_surface->width;
        height = obj_surface->height;

#ifdef PSBVIDEO_MRFL_VPP_ROTATE
    if (obj_config->entrypoint == VAEntrypointVideoProc &&
            share_info && share_info->out_loop_khandle) {
            vaStatus = psb_surface_create_from_kbuf(driver_data, width, height,
                                  obj_surface->psb_surface->size, VA_FOURCC_NV12,
                                  share_info->out_loop_khandle,
                                  obj_surface->psb_surface->stride,
                                  obj_surface->psb_surface->stride,
                                  obj_surface->psb_surface->stride,
                                  0, 0, 0, rotate_surface);
    } else
#endif
            vaStatus = psb_surface_create(driver_data, width, height, VA_FOURCC_NV12,
                                      flags, rotate_surface);
    } else {
        width = obj_surface->height_origin;
        height = (obj_surface->width + 0x1f) & ~0x1f;

#ifdef PSBVIDEO_MRFL_VPP_ROTATE
        if (obj_config->entrypoint == VAEntrypointVideoProc &&
                share_info && share_info->out_loop_khandle != 0) {
                drv_debug_msg(VIDEO_DEBUG_GENERAL,"Create the surface from kbuf out_loop_khandle=%x!\n", share_info->out_loop_khandle);
                rotate_tiling = GET_SURFACE_INFO_tiling(rotate_surface);
                rotate_stride = get_surface_stride(width, rotate_tiling);
                vaStatus = psb_surface_create_from_kbuf(driver_data, width, height,
                                  (rotate_stride * height * 3) / 2, VA_FOURCC_NV12,
                                  share_info->out_loop_khandle,
                                  rotate_stride, rotate_stride, rotate_stride,
                                  0, rotate_stride * height, rotate_stride * height,
                                  rotate_surface);
        } else
#endif
        {
            drv_debug_msg(VIDEO_DEBUG_GENERAL,"Create rotated buffer. width=%d, height=%d\n", width, height);
            if (CONTEXT_SCALING(obj_context)) {
                width = obj_context->scaling_buffer_height;
                height = (obj_context->scaling_buffer_width+ 0x1f) & ~0x1f;
            }
            vaStatus = psb_surface_create(driver_data, width, height, VA_FOURCC_NV12,
                                      flags, rotate_surface);
        }
    }
    if (VA_STATUS_SUCCESS != vaStatus) {
        free(rotate_surface);
        obj_surface->out_loop_surface = NULL;
        vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
        DEBUG_FAILURE;
        return vaStatus;
    }

    //clear rotation surface
     if (CONTEXT_SCALING(obj_context)) {
        if (psb_buffer_map(&rotate_surface->buf, &surface_data)) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "Failed to map rotation buffer before clear it");
        }
        else {
            memset(surface_data, 0, rotate_surface->chroma_offset);
            memset(surface_data + rotate_surface->chroma_offset, 0x80,
                       rotate_surface->size - rotate_surface->chroma_offset);
            psb_buffer_unmap(&rotate_surface->buf);
        }
    }
    obj_surface->width_r = width;
    obj_surface->height_r = height;

#ifdef PSBVIDEO_MSVDX_DEC_TILING
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "attempt to update tile context\n");
    if (GET_SURFACE_INFO_tiling(rotate_surface) && obj_config->entrypoint != VAEntrypointVideoProc) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "update tile context\n");
        object_context_p obj_context = CONTEXT(obj_surface->context_id);
        if (NULL == obj_context) {
            vaStatus = VA_STATUS_ERROR_INVALID_CONTEXT;
            DEBUG_FAILURE;
            return vaStatus;
        }
        unsigned long msvdx_tile = psb__tile_stride_log2_256(obj_surface->width_r);
        obj_context->msvdx_tile &= 0xf; /* clear rotate tile */
        obj_context->msvdx_tile |= (msvdx_tile << 4);
        obj_context->ctp_type &= (~PSB_CTX_TILING_MASK); /* clear tile context */
        obj_context->ctp_type |= ((obj_context->msvdx_tile & 0xff) << 16);
        psb_update_context(driver_data, obj_context->ctp_type);
    }
#endif

exit:
    obj_surface->out_loop_surface = rotate_surface;
    SET_SURFACE_INFO_rotate(rotate_surface, msvdx_rotate);
    /* derive the protected flag from the primay surface */
    SET_SURFACE_INFO_protect(rotate_surface,
                             GET_SURFACE_INFO_protect(obj_surface->psb_surface));

    /*notify hwc that rotated buffer is ready to use.
    * TODO: Do these in psb_SyncSurface()
    */
    if (share_info != NULL) {
	share_info->width_r = rotate_surface->stride;
        share_info->height_r = obj_surface->height_r;
        share_info->out_loop_khandle =
            (uint32_t)(wsbmKBufHandle(wsbmKBuf(rotate_surface->buf.drm_buf)));
        share_info->metadata_rotate = VAROTATION2HAL(driver_data->va_rotate);
        share_info->surface_rotate = VAROTATION2HAL(msvdx_rotate);

        share_info->out_loop_luma_stride = rotate_surface->stride;
        share_info->out_loop_chroma_u_stride = rotate_surface->stride;
        share_info->out_loop_chroma_v_stride = rotate_surface->stride;
    }

    return vaStatus;
}

VAStatus psb_DestroyRotateBuffer(
    object_context_p obj_context,
    object_surface_p obj_surface)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    psb_surface_share_info_p share_info = obj_surface->share_info;
    psb_driver_data_p driver_data = obj_context->driver_data;
    psb_surface_p rotate_surface = obj_surface->out_loop_surface;
    struct psb_buffer_s psb_buf;

    if (share_info && share_info->out_loop_khandle) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL,"psb_DestroyRotateBuffer out_loop_khandle=%x\n", share_info->out_loop_khandle);
        vaStatus = psb_kbuffer_reference(driver_data, &psb_buf, share_info->out_loop_khandle);
        if (vaStatus != VA_STATUS_SUCCESS)
            return vaStatus;
        psb_buffer_destroy(&psb_buf);
        share_info->out_loop_khandle = 0;
    }

    if (rotate_surface)
        free(rotate_surface);

    return vaStatus;
}

