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
 *
 */

#include <va/va_backend.h>
#include <va/va_backend_tpi.h>
#include <va/va_backend_egl.h>
#ifdef PSBVIDEO_MRFL_VPP
#include <va/va_backend_vpp.h>
#endif
#ifdef PSBVIDEO_MFLD
#include <va/va_backend_vpp.h>
#endif
#include <va/va_drmcommon.h>
#include <va/va_android.h>
#include <va/va_tpi.h>

#include "psb_drv_video.h"
#include "psb_texture.h"
#include "psb_cmdbuf.h"
#ifndef BAYTRAIL
#include "pnw_cmdbuf.h"
#include "tng_cmdbuf.h"
#endif
#ifdef PSBVIDEO_MRFL_VPP
#include "vsp_cmdbuf.h"
#endif
#include "psb_surface.h"

#include "pnw_MPEG2.h"
#include "pnw_MPEG4.h"
#include "pnw_H264.h"
#include "pnw_VC1.h"
#include "tng_jpegdec.h"
#include "tng_VP8.h"
#include "tng_yuv_processor.h"

#ifdef PSBVIDEO_MFLD
#include "pnw_MPEG4ES.h"
#include "pnw_H264ES.h"
#include "pnw_H263ES.h"
#include "pnw_jpeg.h"
#endif
#ifdef PSBVIDEO_MRFL
#include "tng_H264ES.h"
#include "tng_H263ES.h"
#include "tng_MPEG4ES.h"
#include "tng_jpegES.h"
#endif
#ifdef PSBVIDEO_MRFL_VPP
#include "vsp_VPP.h"
#include "vsp_vp8.h"
#endif
#include "psb_output.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <wsbm/wsbm_pool.h>
#include <wsbm/wsbm_manager.h>
#include <wsbm/wsbm_util.h>
#include <wsbm/wsbm_fencemgr.h>
#include <linux/videodev2.h>
#include <errno.h>

#include "psb_def.h"
#include "psb_drv_debug.h"
#ifndef BAYTRAIL
#include "psb_ws_driver.h"
#endif
#include "pnw_rotate.h"
#include "psb_surface_attrib.h"
#include "android/psb_gralloc.h"

#ifndef PSB_PACKAGE_VERSION
#define PSB_PACKAGE_VERSION "Undefined"
#endif

#define PSB_DRV_VERSION  PSB_PACKAGE_VERSION
#define PSB_CHG_REVISION "(0X00000071)"

#define PSB_STR_VENDOR_MRST     "Intel GMA500-MRST-" PSB_DRV_VERSION " " PSB_CHG_REVISION
#define PSB_STR_VENDOR_MFLD     "Intel GMA500-MFLD-" PSB_DRV_VERSION " " PSB_CHG_REVISION
#define PSB_STR_VENDOR_MRFL     "Intel GMA500-MRFL-" PSB_DRV_VERSION " " PSB_CHG_REVISION
#define PSB_STR_VENDOR_BAYTRAIL     "Intel GMA500-BAYTRAIL-" PSB_DRV_VERSION " " PSB_CHG_REVISION
#define PSB_STR_VENDOR_LEXINGTON     "Intel GMA500-LEXINGTON-" PSB_DRV_VERSION " " PSB_CHG_REVISION

#define MAX_UNUSED_BUFFERS      16

#define PSB_MAX_FLIP_DELAY (1000/30/10)

#include <signal.h>

#define EXPORT __attribute__ ((visibility("default")))

#define INIT_DRIVER_DATA    psb_driver_data_p driver_data = (psb_driver_data_p) ctx->pDriverData;

#ifdef PSBVIDEO_MRFL_VPP
#define INIT_FORMAT_VTABLE format_vtable_p format_vtable = ((profile < PSB_MAX_PROFILES) && (entrypoint < PSB_MAX_ENTRYPOINTS)) ? (profile == VAProfileNone? driver_data->vpp_profile : driver_data->profile2Format[profile][entrypoint]) : NULL;
#endif
#ifdef PSBVIDEO_MFLD
#define INIT_FORMAT_VTABLE format_vtable_p format_vtable = ((profile < PSB_MAX_PROFILES) && (entrypoint < PSB_MAX_ENTRYPOINTS)) ? (profile == VAProfileNone? driver_data->vpp_profile : driver_data->profile2Format[profile][entrypoint]) : NULL;
#endif

#define CONFIG(id)  ((object_config_p) object_heap_lookup( &driver_data->config_heap, id ))
#define CONTEXT(id) ((object_context_p) object_heap_lookup( &driver_data->context_heap, id ))
#define SURFACE(id)    ((object_surface_p) object_heap_lookup( &driver_data->surface_heap, id ))
#define BUFFER(id)  ((object_buffer_p) object_heap_lookup( &driver_data->buffer_heap, id ))

#define CONFIG_ID_OFFSET        0x01000000
#define CONTEXT_ID_OFFSET       0x02000000
#define SURFACE_ID_OFFSET       0x03000000
#define BUFFER_ID_OFFSET        0x04000000
#define IMAGE_ID_OFFSET         0x05000000
#define SUBPIC_ID_OFFSET        0x06000000

static int psb_get_device_info(VADriverContextP ctx);


void psb_init_surface_pvr2dbuf(psb_driver_data_p driver_data);
void psb_free_surface_pvr2dbuf(psb_driver_data_p driver_data);

VAStatus psb_QueryConfigProfiles(
    VADriverContextP ctx,
    VAProfile *profile_list,    /* out */
    int *num_profiles            /* out */
)
{
    DEBUG_FUNC_ENTER
    (void) ctx; /* unused */
    int i = 0;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    INIT_DRIVER_DATA

    CHECK_INVALID_PARAM(profile_list == NULL);
    CHECK_INVALID_PARAM(num_profiles == NULL);

#ifdef PSBVIDEO_MRFL_VPP
    profile_list[i++] = VAProfileNone;
#endif
//    profile_list[i++] = VAProfileMPEG2Simple;
    profile_list[i++] = VAProfileMPEG2Main;
    profile_list[i++] = VAProfileMPEG4Simple;
    profile_list[i++] = VAProfileMPEG4AdvancedSimple;
//    profile_list[i++] = VAProfileMPEG4Main;
    profile_list[i++] = VAProfileH264Baseline;
    profile_list[i++] = VAProfileH264Main;
    profile_list[i++] = VAProfileH264High;
    profile_list[i++] = VAProfileH264StereoHigh;
    profile_list[i++] = VAProfileVC1Simple;
    profile_list[i++] = VAProfileVC1Main;
    profile_list[i++] = VAProfileVC1Advanced;

    if (IS_MRFL(driver_data) || IS_BAYTRAIL(driver_data)) {
        profile_list[i++] = VAProfileH263Baseline;
        profile_list[i++] = VAProfileJPEGBaseline;
        profile_list[i++] = VAProfileVP8Version0_3;
    } else if (IS_MFLD(driver_data)) {
        profile_list[i++] = VAProfileH263Baseline;
        profile_list[i++] = VAProfileJPEGBaseline;
    }
    profile_list[i++] = VAProfileH264ConstrainedBaseline;

    /* If the assert fails then PSB_MAX_PROFILES needs to be bigger */
    ASSERT(i <= PSB_MAX_PROFILES);
    *num_profiles = i;
    DEBUG_FUNC_EXIT
    return VA_STATUS_SUCCESS;
}

VAStatus psb_QueryConfigEntrypoints(
    VADriverContextP ctx,
    VAProfile profile,
    VAEntrypoint  *entrypoint_list,    /* out */
    int *num_entrypoints        /* out */
)
{
    DEBUG_FUNC_ENTER
    INIT_DRIVER_DATA
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    int entrypoints = 0;
    int i;

    CHECK_INVALID_PARAM(entrypoint_list == NULL);
    CHECK_INVALID_PARAM((num_entrypoints == NULL) || (profile >= PSB_MAX_PROFILES));

    for (i = 0; i < PSB_MAX_ENTRYPOINTS; i++) {
#ifndef BAYTRAIL
#ifdef PSBVIDEO_MRFL_VPP
        if (profile == VAProfileNone && driver_data->vpp_profile &&
            i == VAEntrypointVideoProc) {
                entrypoints++;
                *entrypoint_list++ = i;
        } else
#endif
#endif
        if (profile != VAProfileNone && driver_data->profile2Format[profile][i]) {
                entrypoints++;
                *entrypoint_list++ = i;
        }
    }

    /* If the assert fails then PSB_MAX_ENTRYPOINTS needs to be bigger */
    ASSERT(entrypoints <= PSB_MAX_ENTRYPOINTS);

    if (0 == entrypoints) {
        return VA_STATUS_ERROR_UNSUPPORTED_PROFILE;
    }

    *num_entrypoints = entrypoints;
    DEBUG_FUNC_EXIT
    return VA_STATUS_SUCCESS;
}

/*
 * Figure out if we should return VA_STATUS_ERROR_UNSUPPORTED_PROFILE
 * or VA_STATUS_ERROR_UNSUPPORTED_ENTRYPOINT
 */
static VAStatus psb__error_unsupported_profile_entrypoint(psb_driver_data_p driver_data, VAProfile profile, VAEntrypoint __maybe_unused entrypoint)
{
    /* Does the driver support _any_ entrypoint for this profile? */
    if (profile < PSB_MAX_PROFILES) {
        int i;

        /* Do the parameter check for MFLD and MRFLD */
        if (profile == VAProfileNone)
            return VA_STATUS_ERROR_UNSUPPORTED_ENTRYPOINT;

        for (i = 0; i < PSB_MAX_ENTRYPOINTS; i++) {
            if (driver_data->profile2Format[profile][i]) {
                /* There is an entrypoint, so the profile is supported */
                return VA_STATUS_ERROR_UNSUPPORTED_ENTRYPOINT;
            }
        }
    }
    return VA_STATUS_ERROR_UNSUPPORTED_PROFILE;
}

VAStatus psb_GetConfigAttributes(
    VADriverContextP ctx,
    VAProfile profile,
    VAEntrypoint entrypoint,
    VAConfigAttrib *attrib_list,    /* in/out */
    int num_attribs
)
{
    DEBUG_FUNC_ENTER
    INIT_DRIVER_DATA

#if defined(BAYTRAIL)
    format_vtable_p format_vtable = driver_data->profile2Format[profile][entrypoint];
#elif defined(PSBVIDEO_MRFL_VPP)
    INIT_FORMAT_VTABLE
#else
    format_vtable_p format_vtable = driver_data->profile2Format[profile][entrypoint];
#endif
    int i;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    if (NULL == format_vtable) {
        return psb__error_unsupported_profile_entrypoint(driver_data, profile, entrypoint);
    }

    CHECK_INVALID_PARAM(attrib_list == NULL);
    CHECK_INVALID_PARAM(num_attribs <= 0);

    /* Generic attributes */
    for (i = 0; i < num_attribs; i++) {
        switch (attrib_list[i].type) {
        case VAConfigAttribRTFormat:
            attrib_list[i].value = VA_RT_FORMAT_YUV420;
            if (entrypoint == VAEntrypointEncPicture)
                attrib_list[i].value |= VA_RT_FORMAT_YUV422;
            if ((profile == VAProfileJPEGBaseline) && (entrypoint == VAEntrypointVLD))
                attrib_list[i].value |= VA_RT_FORMAT_YUV444;
            break;

        default:
            attrib_list[i].value = VA_ATTRIB_NOT_SUPPORTED;
            break;
        }
    }
    /* format specific attributes */
    format_vtable->queryConfigAttributes(profile, entrypoint, attrib_list, num_attribs);
    DEBUG_FUNC_EXIT
    return VA_STATUS_SUCCESS;
}

static VAStatus psb__update_attribute(object_config_p obj_config, VAConfigAttrib *attrib)
{
    int i;
    /* Check existing attributes */
    for (i = 0; i < obj_config->attrib_count; i++) {
        if (obj_config->attrib_list[i].type == attrib->type) {
            /* Update existing attribute */
            obj_config->attrib_list[i].value = attrib->value;
            return VA_STATUS_SUCCESS;
        }
    }
    if (obj_config->attrib_count < PSB_MAX_CONFIG_ATTRIBUTES) {
        i = obj_config->attrib_count;
        obj_config->attrib_list[i].type = attrib->type;
        obj_config->attrib_list[i].value = attrib->value;
        obj_config->attrib_count++;
        return VA_STATUS_SUCCESS;
    }
    return VA_STATUS_ERROR_MAX_NUM_EXCEEDED;
}

static VAStatus psb__validate_config(object_config_p obj_config)
{
    int i;
    /* Check all attributes */
    for (i = 0; i < obj_config->attrib_count; i++) {
        switch (obj_config->attrib_list[i].type) {
        case VAConfigAttribRTFormat:
            if (!(obj_config->attrib_list[i].value == VA_RT_FORMAT_YUV420
                  || (obj_config->attrib_list[i].value == VA_RT_FORMAT_YUV422 &&
                      obj_config->entrypoint == VAEntrypointEncPicture)
                  || (obj_config->attrib_list[i].value == (VA_RT_FORMAT_YUV444 | VA_RT_FORMAT_YUV420 )))) {
                return VA_STATUS_ERROR_UNSUPPORTED_RT_FORMAT;
            }
            break;

        default:
            /*
             * Ignore unknown attributes here, it
             * may be format specific.
             */
            break;
        }
    }
    return VA_STATUS_SUCCESS;
}

static int psb_get_active_entrypoint_number(
    VADriverContextP ctx,
    unsigned int entrypoint)
{
    INIT_DRIVER_DATA;
    struct drm_lnc_video_getparam_arg arg;
    int count = 0;
    int ret;

    if (VAEntrypointVLD > entrypoint ||
        entrypoint > VAEntrypointEncPicture) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s :Invalid entrypoint %d.\n",
                           __FUNCTION__, entrypoint);
        return -1;
    }

    arg.key = PNW_VIDEO_QUERY_ENTRY;
    arg.value = (uint64_t)((unsigned long) &count);
    arg.arg = (uint64_t)((unsigned int)&entrypoint);
    ret = drmCommandWriteRead(driver_data->drm_fd, driver_data->getParamIoctlOffset,
                              &arg, sizeof(arg));
    if (ret) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s drmCommandWriteRead fails %d.\n",
                           __FUNCTION__, ret);
        return -1;
    }

    return count;
}

VAStatus psb_CreateConfig(
    VADriverContextP ctx,
    VAProfile profile,
    VAEntrypoint entrypoint,
    VAConfigAttrib *attrib_list,
    int num_attribs,
    VAConfigID *config_id        /* out */
)
{
    DEBUG_FUNC_ENTER
    INIT_DRIVER_DATA
#if defined(BAYTRAIL)
    format_vtable_p format_vtable = driver_data->profile2Format[profile][entrypoint];
#elif defined(PSBVIDEO_MRFL_VPP)
    INIT_FORMAT_VTABLE
#else
    format_vtable_p format_vtable = driver_data->profile2Format[profile][entrypoint];
#endif
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    int configID;
    object_config_p obj_config;
    int i;

    drv_debug_msg(VIDEO_DEBUG_INIT, "CreateConfig profile:%d, entrypoint:%d, num_attribs:%d.\n",
        profile, entrypoint, num_attribs);
    /*echo 8 > /sys/module/pvrsrvkm/parameters/no_ec will disable error concealment*/
    if ((profile == VAProfileH264ConstrainedBaseline) && (VAEntrypointVLD == entrypoint)) {
        char ec_disable[2];
        FILE *ec_fp = fopen("/sys/module/pvrsrvkm/parameters/no_ec", "r");
        if (ec_fp) {
            if (fgets(ec_disable, 2, ec_fp) != NULL) {
                /* force profile to VAProfileH264High */
                if (strcmp(ec_disable, "8") == 0) {
                    drv_debug_msg(VIDEO_DEBUG_INIT, "disabled error concealment by setting profile to VAProfileH264High\n");
                    profile = VAProfileH264High;
                }
            }
            fclose(ec_fp);
        }
    }

    CHECK_INVALID_PARAM(config_id == NULL);
    CHECK_INVALID_PARAM(num_attribs < 0);
    CHECK_INVALID_PARAM(attrib_list == NULL);

    if (NULL == format_vtable) {
        vaStatus = psb__error_unsupported_profile_entrypoint(driver_data, profile, entrypoint);
    }

    CHECK_VASTATUS();

    if ((IS_MFLD(driver_data)) &&
            ((VAEntrypointEncPicture == entrypoint)
                    || (VAEntrypointEncSlice == entrypoint))) {
        int active_slc, active_pic;
        /* Only allow one encoding entrypoint at the sametime.
         * But if video encoding request comes when process JPEG encoding,
         * it will wait until current JPEG picture encoding finish.
         * Further JPEG encoding should fall back to software path.*/
        active_slc = psb_get_active_entrypoint_number(ctx, VAEntrypointEncSlice);
        active_pic = psb_get_active_entrypoint_number(ctx, VAEntrypointEncPicture);

        if (active_slc > 0) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "There already is a active video encoding entrypoint."
                    "Entrypoint %d isn't available.\n", entrypoint);
            return VA_STATUS_ERROR_HW_BUSY;
        }
        else if (active_pic > 0 && VAEntrypointEncPicture == entrypoint) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "There already is a active picture encoding entrypoint."
                    "Entrypoint %d isn't available.\n", entrypoint);
            return VA_STATUS_ERROR_HW_BUSY;
        }
    }

    configID = object_heap_allocate(&driver_data->config_heap);
    obj_config = CONFIG(configID);
    CHECK_ALLOCATION(obj_config);

    MEMSET_OBJECT(obj_config, struct object_config_s);

    obj_config->profile = profile;
    obj_config->format_vtable = format_vtable;
    obj_config->entrypoint = entrypoint;
    obj_config->attrib_list[0].type = VAConfigAttribRTFormat;
    obj_config->attrib_list[0].value = VA_RT_FORMAT_YUV420;
    obj_config->attrib_count = 1;

    for (i = 0; i < num_attribs; i++) {
        if (attrib_list[i].type > VAConfigAttribTypeMax)
            return VA_STATUS_ERROR_ATTR_NOT_SUPPORTED;

        vaStatus = psb__update_attribute(obj_config, &(attrib_list[i]));
        if (VA_STATUS_SUCCESS != vaStatus) {
            break;
        }
    }

    if (VA_STATUS_SUCCESS == vaStatus) {
        vaStatus = psb__validate_config(obj_config);
    }

    if (VA_STATUS_SUCCESS == vaStatus) {
        vaStatus = format_vtable->validateConfig(obj_config);
    }

    /* Error recovery */
    if (VA_STATUS_SUCCESS != vaStatus) {
        object_heap_free(&driver_data->config_heap, (object_base_p) obj_config);
    } else {
        *config_id = configID;
    }

#ifdef PSBVIDEO_MSVDX_EC
    if((getenv("PSB_VIDEO_NOEC") == NULL)
        && (profile == VAProfileH264ConstrainedBaseline)) {
        drv_debug_msg(VIDEO_DEBUG_INIT, "profile is VAProfileH264ConstrainedBaseline, error concealment is enabled. \n");
        driver_data->ec_enabled = 1;
    } else {
        driver_data->ec_enabled = 0;
    }

    if (profile == VAProfileVP8Version0_3 ||
        profile == VAProfileH264Baseline ||
        profile == VAProfileH264Main ||
        profile == VAProfileH264High ||
        profile == VAProfileH264ConstrainedBaseline)
                driver_data->ec_enabled = 1;

    if (!IS_MRFL(driver_data)) {
        if (profile == VAProfileMPEG4Simple ||
            profile == VAProfileMPEG4AdvancedSimple ||
            profile == VAProfileMPEG4Main)
                driver_data->ec_enabled = 1;
    }

#else
    driver_data->ec_enabled = 0;
#endif

    DEBUG_FUNC_EXIT
    return vaStatus;
}

VAStatus psb_DestroyConfig(
    VADriverContextP ctx,
    VAConfigID config_id
)
{
    DEBUG_FUNC_ENTER
    INIT_DRIVER_DATA
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    object_config_p obj_config;

    obj_config = CONFIG(config_id);
    CHECK_CONFIG(obj_config);

    object_heap_free(&driver_data->config_heap, (object_base_p) obj_config);
    DEBUG_FUNC_EXIT
    return vaStatus;
}

VAStatus psb_QueryConfigAttributes(
    VADriverContextP ctx,
    VAConfigID config_id,
    VAProfile *profile,        /* out */
    VAEntrypoint *entrypoint,     /* out */
    VAConfigAttrib *attrib_list,    /* out */
    int *num_attribs        /* out */
)
{
    DEBUG_FUNC_ENTER
    INIT_DRIVER_DATA
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    object_config_p obj_config;
    int i;

    CHECK_INVALID_PARAM(profile == NULL);
    CHECK_INVALID_PARAM(entrypoint == NULL);
    CHECK_INVALID_PARAM(attrib_list == NULL);
    CHECK_INVALID_PARAM(num_attribs == NULL);

    obj_config = CONFIG(config_id);
    CHECK_CONFIG(obj_config);

    *profile = obj_config->profile;
    *entrypoint = obj_config->entrypoint;
    *num_attribs =  obj_config->attrib_count;
    for (i = 0; i < obj_config->attrib_count; i++) {
        attrib_list[i] = obj_config->attrib_list[i];
    }

    DEBUG_FUNC_EXIT
    return vaStatus;
}

void psb__destroy_surface(psb_driver_data_p driver_data, object_surface_p obj_surface)
{
    if (NULL != obj_surface) {
        /* delete subpicture association */
        psb_SurfaceDeassociateSubpict(driver_data, obj_surface);

        obj_surface->is_ref_surface = 0;

        psb_surface_sync(obj_surface->psb_surface);
        psb_surface_destroy(obj_surface->psb_surface);

        if (obj_surface->out_loop_surface) {
            psb_surface_destroy(obj_surface->out_loop_surface);
        }

        if (obj_surface->scaling_surface) {
            psb_surface_destroy(obj_surface->scaling_surface);
        }

        free(obj_surface->psb_surface);
        object_heap_free(&driver_data->surface_heap, (object_base_p) obj_surface);
    }
}

VAStatus psb__checkSurfaceDimensions(psb_driver_data_p driver_data, int width, int height)
{
    if (driver_data->video_sd_disabled) {
        return VA_STATUS_ERROR_RESOLUTION_NOT_SUPPORTED;
    }
    if ((width <= 0) || (width * height > 5120 * 5120) || (height <= 0)) {
        return VA_STATUS_ERROR_RESOLUTION_NOT_SUPPORTED;
    }
    if (driver_data->video_hd_disabled) {
        if ((width > 1024) || (height > 576)) {
            return VA_STATUS_ERROR_RESOLUTION_NOT_SUPPORTED;
        }
    }

    return VA_STATUS_SUCCESS;
}

VAStatus psb_GetSurfaceAttributes(
        VADriverContextP    __maybe_unused ctx,
        VAConfigID __maybe_unused config,
        VASurfaceAttrib *attrib_list,
        unsigned int num_attribs
        )
{
    DEBUG_FUNC_ENTER

    uint32_t i;
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    CHECK_INVALID_PARAM(attrib_list == NULL);
    CHECK_INVALID_PARAM(num_attribs <= 0);

    /* Generic attributes */
    for (i = 0; i < num_attribs; i++) {
        switch (attrib_list[i].type) {
        case VASurfaceAttribMemoryType:
            attrib_list[i].flags = VA_SURFACE_ATTRIB_SETTABLE | VA_SURFACE_ATTRIB_GETTABLE;
            attrib_list[i].value.type = VAGenericValueTypeInteger;
            attrib_list[i].value.value.i =
                VA_SURFACE_ATTRIB_MEM_TYPE_VA |
                VA_SURFACE_ATTRIB_MEM_TYPE_USER_PTR |
                VA_SURFACE_ATTRIB_MEM_TYPE_KERNEL_DRM |
                VA_SURFACE_ATTRIB_MEM_TYPE_ANDROID_GRALLOC |
                VA_SURFACE_ATTRIB_MEM_TYPE_ANDROID_ION;
            break;

        case VASurfaceAttribExternalBufferDescriptor:
            attrib_list[i].flags = VA_SURFACE_ATTRIB_SETTABLE;
            attrib_list[i].value.type = VAGenericValueTypePointer;
            break;

        default:
            attrib_list[i].flags = VA_SURFACE_ATTRIB_NOT_SUPPORTED;
            break;
        }
    }

    DEBUG_FUNC_EXIT
    return VA_STATUS_SUCCESS;

}

VAStatus psb_CreateSurfaces(
        VADriverContextP __maybe_unused ctx,
        int __maybe_unused width,
        int __maybe_unused height,
        int __maybe_unused format,
        int __maybe_unused num_surfaces,
        VASurfaceID __maybe_unused * surface_list        /* out */
)
{
    return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus psb_CreateSurfaces2(
    VADriverContextP ctx,
    unsigned int format,
    unsigned int width,
    unsigned int height,
    VASurfaceID *surface_list,        /* out */
    unsigned int num_surfaces,
    VASurfaceAttrib *attrib_list,
    unsigned int num_attribs
)
{
    DEBUG_FUNC_ENTER
    INIT_DRIVER_DATA
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    unsigned int i;
    int height_origin, buffer_stride = 0;
    driver_data->protected = (VA_RT_FORMAT_PROTECTED & format);
    unsigned long fourcc;
    unsigned int flags = 0;
    int memory_type = -1;
    unsigned int initalized_info_flag = 1;
    VASurfaceAttribExternalBuffers  *pExternalBufDesc = NULL;
    PsbSurfaceAttributeTPI attribute_tpi;

    CHECK_INVALID_PARAM(num_surfaces <= 0);
    CHECK_SURFACE(surface_list);

    if ((attrib_list != NULL) && (num_attribs > 0)) {
        for (i = 0; i < num_attribs; i++, attrib_list++) {
            if (!attrib_list)
                return VA_STATUS_ERROR_INVALID_PARAMETER;
            switch (attrib_list->type) {
            case VASurfaceAttribExternalBufferDescriptor:
                {
                    pExternalBufDesc = (VASurfaceAttribExternalBuffers *)attrib_list->value.value.p;
                    if (pExternalBufDesc == NULL) {
                        drv_debug_msg(VIDEO_DEBUG_ERROR, "Invalid VASurfaceAttribExternalBuffers.\n");
                        return VA_STATUS_ERROR_INVALID_PARAMETER;
                    }
                    attribute_tpi.type = memory_type;
                    attribute_tpi.buffers = malloc(sizeof(long) * pExternalBufDesc->num_buffers);
                    attribute_tpi.width = pExternalBufDesc->width;
                    attribute_tpi.height = pExternalBufDesc->height;
                    attribute_tpi.count = pExternalBufDesc->num_buffers;
                    memcpy((void*)attribute_tpi.buffers, (void*)pExternalBufDesc->buffers,
                            sizeof(pExternalBufDesc->buffers[0]) *
                            pExternalBufDesc->num_buffers);
                    attribute_tpi.pixel_format = pExternalBufDesc->pixel_format;
                    attribute_tpi.size = pExternalBufDesc->data_size;
                    attribute_tpi.luma_stride = pExternalBufDesc->pitches[0];
                    attribute_tpi.chroma_u_stride = pExternalBufDesc->pitches[1];
                    attribute_tpi.chroma_v_stride = pExternalBufDesc->pitches[2];
                    attribute_tpi.luma_offset = pExternalBufDesc->offsets[0];
                    attribute_tpi.chroma_u_offset = pExternalBufDesc->offsets[1];
                    attribute_tpi.chroma_v_offset = pExternalBufDesc->offsets[2];
                    attribute_tpi.reserved[0] = (unsigned long) pExternalBufDesc->private_data;
                    if (pExternalBufDesc->flags & VA_SURFACE_EXTBUF_DESC_ENABLE_TILING)
                        attribute_tpi.tiling = 1;
                    else
                        attribute_tpi.tiling = 0;
                }
                break;
            case VASurfaceAttribMemoryType:
                {
                    switch (attrib_list->value.value.i) {
                        case VA_SURFACE_ATTRIB_MEM_TYPE_USER_PTR:
                            memory_type = VAExternalMemoryUserPointer;
                            break;
                        case VA_SURFACE_ATTRIB_MEM_TYPE_KERNEL_DRM:
                            memory_type = VAExternalMemoryKernelDRMBufffer;
                            break;
                        case VA_SURFACE_ATTRIB_MEM_TYPE_ANDROID_GRALLOC:
                            memory_type = VAExternalMemoryAndroidGrallocBuffer;
                            break;
                        case VA_SURFACE_ATTRIB_MEM_TYPE_ANDROID_ION:
                            memory_type = VAExternalMemoryIONSharedFD;
                            break;
                        case VA_SURFACE_ATTRIB_MEM_TYPE_VA:
                            memory_type = VAExternalMemoryNULL;
                            break;
                        default:
                            drv_debug_msg(VIDEO_DEBUG_ERROR, "Unsupported memory type.\n");
                            return VA_STATUS_ERROR_INVALID_PARAMETER;

                    }
                }
                break;
            case VASurfaceAttribUsageHint:
                {
                    /* Share info is to be initialized when created sufaces by default (for the data producer)
                     * VPP Read indicate we do not NOT touch share info (for data consumer, which share buffer with data
                     * producer, such as of VPP).
                     */
                    drv_debug_msg(VIDEO_DEBUG_GENERAL, "VASurfaceAttribUsageHint.\n");
                    if ((attrib_list->value.value.i & VA_SURFACE_ATTRIB_USAGE_HINT_VPP_READ)!= 0){
                        initalized_info_flag = 0;
                        drv_debug_msg(VIDEO_DEBUG_GENERAL, "explicat not initialized share info.\n");
                    }
                }
                break;
            default:
                drv_debug_msg(VIDEO_DEBUG_ERROR, "Unsupported attribute.\n");
                return VA_STATUS_ERROR_INVALID_PARAMETER;
            }
        }
    }

    if ((memory_type == -1 && pExternalBufDesc != NULL) ||
            (memory_type != -1 && pExternalBufDesc == NULL)) {
        return VA_STATUS_ERROR_INVALID_PARAMETER;
    }
    else if(memory_type !=-1 && pExternalBufDesc != NULL) {
        attribute_tpi.type = memory_type;
        //set initialized share info in reserverd 1, by default we will initialized share_info
        attribute_tpi.reserved[2] = (unsigned int)initalized_info_flag;
        vaStatus = psb_CreateSurfacesWithAttribute(ctx, width, height, format, num_surfaces, surface_list, (VASurfaceAttributeTPI *)&attribute_tpi);
        pExternalBufDesc->private_data = (void *)(attribute_tpi.reserved[1]);
        if (attribute_tpi.buffers) free(attribute_tpi.buffers);
        return vaStatus;
    }

    format = format & (~VA_RT_FORMAT_PROTECTED);

    /* We only support one format */
    if ((VA_RT_FORMAT_YUV420 != format)
        && (VA_RT_FORMAT_YUV422 != format)
        && (VA_RT_FORMAT_YUV444 != format)) {
        vaStatus = VA_STATUS_ERROR_UNSUPPORTED_RT_FORMAT;
        DEBUG_FAILURE;
        return vaStatus;
    }

    vaStatus = psb__checkSurfaceDimensions(driver_data, width, height);
    CHECK_VASTATUS();

    /* Adjust height to be a multiple of 32 (height of macroblock in interlaced mode) */
    height_origin = height;
    height = (height + 0x1f) & ~0x1f;


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
        obj_surface->share_info = NULL;

        psb_surface = (psb_surface_p) calloc(1, sizeof(struct psb_surface_s));
        if (NULL == psb_surface) {
            object_heap_free(&driver_data->surface_heap, (object_base_p) obj_surface);
            obj_surface->surface_id = VA_INVALID_SURFACE;
            vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
            DEBUG_FAILURE;
            break;
        }

        switch (format) {
        case VA_RT_FORMAT_YUV444:
            fourcc = VA_FOURCC_YV32; /* allocate 4 planar */
            break;
        case VA_RT_FORMAT_YUV422:
            fourcc = VA_FOURCC_YV16;
            break;
        case VA_RT_FORMAT_YUV420:
        default:
            fourcc = VA_FOURCC_NV12;
            break;
        }

        flags |= driver_data->protected ? IS_PROTECTED : 0;
        vaStatus = psb_surface_create(driver_data, width, height, fourcc,
                                      flags, psb_surface);

        if (VA_STATUS_SUCCESS != vaStatus) {
            free(psb_surface);
            object_heap_free(&driver_data->surface_heap, (object_base_p) obj_surface);
            obj_surface->surface_id = VA_INVALID_SURFACE;
            DEBUG_FAILURE;
            break;
        }
        buffer_stride = psb_surface->stride;
        /* by default, surface fourcc is NV12 */
        psb_surface->extra_info[4] = fourcc;
        psb_surface->extra_info[8] = fourcc;
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
        drv_debug_msg(VIDEO_DEBUG_ERROR, "CreateSurfaces failed\n");
        return vaStatus;
    }
    DEBUG_FUNC_EXIT
    return vaStatus;
}

VAStatus psb_DestroySurfaces(
    VADriverContextP ctx,
    VASurfaceID *surface_list,
    int num_surfaces
)
{
    INIT_DRIVER_DATA
    int i;
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    if (num_surfaces <= 0) {
        return VA_STATUS_ERROR_INVALID_PARAMETER;
    }

    CHECK_SURFACE(surface_list);

#if 0
    /* Free PVR2D buffer wrapped from the surfaces */
    psb_free_surface_pvr2dbuf(driver_data);
#endif

    /* Make validation happy */
    for (i = 0; i < num_surfaces; i++) {
        object_surface_p obj_surface = SURFACE(surface_list[i]);
        if (obj_surface == NULL) {
            return VA_STATUS_ERROR_INVALID_SURFACE;
        }
        if (obj_surface->derived_imgcnt > 0) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "Some surface is deriving by images\n");
            return VA_STATUS_ERROR_OPERATION_FAILED;
        }
    }

    for (i = 0; i < num_surfaces; i++) {
        object_surface_p obj_surface = SURFACE(surface_list[i]);
        if (obj_surface == NULL)
            return VA_STATUS_ERROR_INVALID_SURFACE;

        if (driver_data->cur_displaying_surface == surface_list[i]) {
            /* Surface is being displaying. Need to stop overlay here */
            psb_coverlay_stop(ctx);
        }
        drv_debug_msg(VIDEO_DEBUG_INIT, "%s : obj_surface->surface_id = 0x%x\n",__FUNCTION__, obj_surface->surface_id);
        if (obj_surface->share_info) {
            psb_DestroySurfaceGralloc(obj_surface);
        }
        psb__destroy_surface(driver_data, obj_surface);
        surface_list[i] = VA_INVALID_SURFACE;
    }

    DEBUG_FUNC_EXIT
    return VA_STATUS_SUCCESS;
}

int psb_new_context(psb_driver_data_p driver_data, uint64_t ctx_type)
{
    struct drm_lnc_video_getparam_arg arg;
    int ret = 0;

    arg.key = IMG_VIDEO_NEW_CONTEXT;
    arg.value = (uint64_t)((unsigned long) & ctx_type);
    ret = drmCommandWriteRead(driver_data->drm_fd, driver_data->getParamIoctlOffset,
                              &arg, sizeof(arg));
    if (ret != 0)
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Set context %d failed\n", ctx_type);

    return ret;
}

#ifdef PSBVIDEO_MSVDX_DEC_TILING
int psb_update_context(psb_driver_data_p driver_data, unsigned long ctx_type)
{
    struct drm_lnc_video_getparam_arg arg;
    int ret = 0;

    arg.key = IMG_VIDEO_UPDATE_CONTEXT;
    arg.value = (uint64_t)((unsigned long) & ctx_type);
    ret = drmCommandWriteRead(driver_data->drm_fd, driver_data->getParamIoctlOffset,
                              &arg, sizeof(arg));
    if (ret != 0)
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Update context %d failed\n", ctx_type);

    return ret;
}
#endif

int psb_rm_context(psb_driver_data_p driver_data)
{
    struct drm_lnc_video_getparam_arg arg;
    int tmp;
    int ret = 0;

    arg.key = IMG_VIDEO_RM_CONTEXT;
    arg.value = (uint64_t)((unsigned long) & tmp); /* value is ignored */
    ret = drmCommandWriteRead(driver_data->drm_fd, driver_data->getParamIoctlOffset,
                              &arg, sizeof(arg));
    if (ret != 0)
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Remove context failed\n");

    return ret;
}

#ifdef PSBVIDEO_MSVDX_DEC_TILING
unsigned long psb__tile_stride_log2_256(int w)
{
    int stride_mode = 0;

    if (512 >= w)
        stride_mode = 1;
    else if (1024 >= w)
        stride_mode = 2;
    else if (2048 >= w)
        stride_mode = 3;
    else if (4096 >= w)
        stride_mode = 4;

    return stride_mode;
}

unsigned long psb__tile_stride_log2_512(int w)
{
    int stride_mode = 0;

    if (512 >= w)
        stride_mode = 0;
    else if (1024 >= w)
        stride_mode = 1;
    else if (2048 >= w)
        stride_mode = 2;
    else if (4096 >= w)
        stride_mode = 3;

    return stride_mode;
}
#endif

VAStatus psb_CreateContext(
    VADriverContextP ctx,
    VAConfigID config_id,
    int picture_width,
    int picture_height,
    int flag,
    VASurfaceID *render_targets,
    int num_render_targets,
    VAContextID *context        /* out */
)
{
    DEBUG_FUNC_ENTER
    INIT_DRIVER_DATA
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    object_config_p obj_config;
    int cmdbuf_num, encode = 0, proc = 0;
    int i;
    drv_debug_msg(VIDEO_DEBUG_ERROR, "CreateContext config_id:%d, pic_w:%d, pic_h:%d, flag:%d, num_render_targets:%d.\n",
        config_id, picture_width, picture_height, flag, num_render_targets);

    //CHECK_INVALID_PARAM(num_render_targets <= 0);

    //CHECK_SURFACE(render_targets);
    CHECK_CONTEXT(context);

    vaStatus = psb__checkSurfaceDimensions(driver_data, picture_width, picture_height);
    CHECK_VASTATUS();

    obj_config = CONFIG(config_id);
    CHECK_CONFIG(obj_config);

    int contextID = object_heap_allocate(&driver_data->context_heap);
    object_context_p obj_context = CONTEXT(contextID);
    CHECK_ALLOCATION(obj_context);

    *context = contextID;

    MEMSET_OBJECT(obj_context, struct object_context_s);

    obj_context->driver_data = driver_data;
    obj_context->current_render_target = NULL;
    obj_context->ec_target = NULL;
    obj_context->ec_candidate = NULL;
    obj_context->is_oold = driver_data->is_oold;
    obj_context->context_id = contextID;
    obj_context->config_id = config_id;
    obj_context->picture_width = picture_width;
    obj_context->picture_height = picture_height;
    obj_context->num_render_targets = num_render_targets;
    obj_context->msvdx_scaling = 0;
#ifdef SLICE_HEADER_PARSING
    obj_context->msvdx_frame_end = 0;
    for (i = 0; i < obj_config->attrib_count; i++) {
        if ((obj_config->attrib_list[i].type == VAConfigAttribDecSliceMode) &&
            (obj_config->attrib_list[i].value == VA_DEC_SLICE_MODE_SUBSAMPLE)) {
            obj_context->modular_drm = 1;
            break;
        }
    }
#endif
    obj_context->scaling_width = 0;
    obj_context->scaling_height = 0;

    if (num_render_targets > 0) {
        obj_context->render_targets = (VASurfaceID *) calloc(1, num_render_targets * sizeof(VASurfaceID));
        if (obj_context->render_targets == NULL) {
            vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
            DEBUG_FAILURE;

            object_heap_free(&driver_data->context_heap, (object_base_p) obj_context);

            return vaStatus;
        }
    }

    /* allocate buffer points for vaRenderPicture */
    obj_context->num_buffers = 10;
    obj_context->buffer_list = (object_buffer_p *) calloc(1, sizeof(object_buffer_p) * obj_context->num_buffers);
    if (obj_context->buffer_list == NULL) {
        vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
        DEBUG_FAILURE;

        if (NULL != obj_context->render_targets)
            free(obj_context->render_targets);
        object_heap_free(&driver_data->context_heap, (object_base_p) obj_context);

        return vaStatus;
    }

    memset(obj_context->buffers_unused, 0, sizeof(obj_context->buffers_unused));
    memset(obj_context->buffers_unused_count, 0, sizeof(obj_context->buffers_unused_count));
    memset(obj_context->buffers_unused_tail, 0, sizeof(obj_context->buffers_unused_tail));
    memset(obj_context->buffers_active, 0, sizeof(obj_context->buffers_active));

    if (obj_config->entrypoint == VAEntrypointEncSlice
        || obj_config->entrypoint == VAEntrypointEncPicture) {
        encode = 1;
    }
#ifdef PSBVIDEO_MRFL_VPP
    if (obj_config->entrypoint == VAEntrypointVideoProc)
        proc = 1;

    //todo: fixme
    if (obj_config->entrypoint == VAEntrypointEncSlice && obj_config->profile == VAProfileVP8Version0_3){
            proc = 1;
            encode = 0;
    }
#endif

    if (encode)
        cmdbuf_num = LNC_MAX_CMDBUFS_ENCODE;
    else if (proc)
        cmdbuf_num = VSP_MAX_CMDBUFS;
    else
        cmdbuf_num = PSB_MAX_CMDBUFS;

    if (num_render_targets > 0) {
        for (i = 0; i < num_render_targets; i++) {
            object_surface_p obj_surface = SURFACE(render_targets[i]);
            psb_surface_p psb_surface;

            if (NULL == obj_surface) {
                vaStatus = VA_STATUS_ERROR_INVALID_SURFACE;
                DEBUG_FAILURE;
                break;
            }

            if (!driver_data->protected && obj_surface->share_info)
                obj_surface->share_info->force_output_method = 0;

            psb_surface = obj_surface->psb_surface;

            /* Clear format specific surface info */
            obj_context->render_targets[i] = render_targets[i];
            obj_surface->context_id = contextID; /* Claim ownership of surface */
#ifdef PSBVIDEO_MSVDX_DEC_TILING
            if (GET_SURFACE_INFO_tiling(psb_surface))
#ifdef BAYTRAIL
                obj_context->msvdx_tile = psb__tile_stride_log2_512(obj_surface->width);
#else
            if (obj_config->entrypoint == VAEntrypointVideoProc && obj_config->profile == VAProfileNone)
                // It's for two pass rotation case
                // Need the source surface width for tile stride setting
                obj_context->msvdx_tile = psb__tile_stride_log2_256(obj_context->picture_width);
            else
                obj_context->msvdx_tile = psb__tile_stride_log2_256(obj_surface->width);
#endif
        else {
            ;
        }
#endif
#if 0
            /* for decode, move the surface into |TT */
            if ((encode == 0) && /* decode */
                    ((psb_surface->buf.pl_flags & DRM_PSB_FLAG_MEM_RAR) == 0)) /* surface not in RAR */
                psb_buffer_setstatus(&obj_surface->psb_surface->buf,
                        WSBM_PL_FLAG_TT | WSBM_PL_FLAG_SHARED, DRM_PSB_FLAG_MEM_MMU);
#endif
        }
    }

    obj_context->va_flags = flag;
    obj_context->format_vtable = obj_config->format_vtable;
    obj_context->format_data = NULL;

    if (VA_STATUS_SUCCESS == vaStatus) {
        vaStatus = obj_context->format_vtable->createContext(obj_context, obj_config);
    }

    /* Error recovery */
    if (VA_STATUS_SUCCESS != vaStatus) {
        obj_context->context_id = -1;
        obj_context->config_id = -1;
        obj_context->picture_width = 0;
        obj_context->picture_height = 0;
        if (NULL != obj_context->render_targets)
            free(obj_context->render_targets);
        free(obj_context->buffer_list);
        obj_context->num_buffers = 0;
        obj_context->render_targets = NULL;
        obj_context->num_render_targets = 0;
        obj_context->va_flags = 0;
        object_heap_free(&driver_data->context_heap, (object_base_p) obj_context);

        return vaStatus;
    }

    /* initialize cmdbuf */
    for (i = 0; i < PNW_MAX_CMDBUFS_ENCODE; i++) {
        obj_context->pnw_cmdbuf_list[i] = NULL;
    }

#ifdef PSBVIDEO_MRFL
    for (i = 0; i < TNG_MAX_CMDBUFS_ENCODE; i++) {
        obj_context->tng_cmdbuf_list[i] = NULL;
    }
#endif

#ifdef PSBVIDEO_MRFL_VPP
    for (i = 0; i < VSP_MAX_CMDBUFS; i++) {
        obj_context->vsp_cmdbuf_list[i] = NULL;
    }
#endif

    for (i = 0; i < PSB_MAX_CMDBUFS; i++) {
        obj_context->cmdbuf_list[i] = NULL;
    }

    for (i = 0; i < cmdbuf_num; i++) {
        void  *cmdbuf = NULL;
#ifndef BAYTRAIL
        if (encode) { /* Topaz encode context */
#ifdef PSBVIDEO_MRFL
            if (IS_MRFL(obj_context->driver_data))
                cmdbuf = calloc(1, sizeof(struct tng_cmdbuf_s));
#endif
#ifdef PSBVIDEO_MFLD
            if (IS_MFLD(obj_context->driver_data))
                cmdbuf = calloc(1, sizeof(struct pnw_cmdbuf_s));
#endif
        } else if (proc) { /* VSP VPP context */
            /* VED two pass rotation under VPP API */
            if (driver_data->ved_vpp)
                cmdbuf =  calloc(1, sizeof(struct psb_cmdbuf_s));
#ifdef PSBVIDEO_MRFL_VPP
            else if (IS_MRFL(obj_context->driver_data))
                cmdbuf = calloc(1, sizeof(struct vsp_cmdbuf_s));
#endif
        } else /* MSVDX decode context */
#endif
            cmdbuf =  calloc(1, sizeof(struct psb_cmdbuf_s));

        if (NULL == cmdbuf) {
            vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
            DEBUG_FAILURE;
            break;
        }

#ifndef BAYTRAIL
        if (encode) { /* Topaz encode context */

#ifdef PSBVIDEO_MRFL
            if (IS_MRFL(obj_context->driver_data))
                vaStatus = tng_cmdbuf_create(obj_context, driver_data, (tng_cmdbuf_p)cmdbuf);
#endif
#ifdef PSBVIDEO_MFLD
            if (IS_MFLD(obj_context->driver_data))
                vaStatus = pnw_cmdbuf_create(obj_context, driver_data, (pnw_cmdbuf_p)cmdbuf);
#endif
        } else if (proc) { /* VSP VPP context */
            if (driver_data->ved_vpp)
                vaStatus = psb_cmdbuf_create(obj_context, driver_data, (psb_cmdbuf_p)cmdbuf);
#ifdef PSBVIDEO_MRFL_VPP
            else if (IS_MRFL(obj_context->driver_data))
                vaStatus = vsp_cmdbuf_create(obj_context, driver_data, (vsp_cmdbuf_p)cmdbuf);
#endif
        } else /* MSVDX decode context */
#endif
            vaStatus = psb_cmdbuf_create(obj_context, driver_data, (psb_cmdbuf_p)cmdbuf);

        if (VA_STATUS_SUCCESS != vaStatus) {
            free(cmdbuf);
            DEBUG_FAILURE;
            break;
        }

#ifndef BAYTRAIL
        if (encode) { /* Topaz encode context */
            if (i >= LNC_MAX_CMDBUFS_ENCODE) {
                free(cmdbuf);
                DEBUG_FAILURE;
                break;
            }

#ifdef PSBVIDEO_MRFL
            if (IS_MRFL(obj_context->driver_data))
                obj_context->tng_cmdbuf_list[i] = (tng_cmdbuf_p)cmdbuf;
#endif
#ifdef PSBVIDEO_MFLD
            if (IS_MFLD(obj_context->driver_data))
                obj_context->pnw_cmdbuf_list[i] = (pnw_cmdbuf_p)cmdbuf;
#endif
        } else if (proc) { /* VSP VPP context */
            if (driver_data->ved_vpp)
                obj_context->cmdbuf_list[i] = (psb_cmdbuf_p)cmdbuf;
#ifdef PSBVIDEO_MRFL_VPP
            else if (IS_MRFL(obj_context->driver_data))
                obj_context->vsp_cmdbuf_list[i] = (vsp_cmdbuf_p)cmdbuf;
#endif
        } else /* MSVDX decode context */
#endif
            obj_context->cmdbuf_list[i] = (psb_cmdbuf_p)cmdbuf;
    }

    obj_context->cmdbuf_current = -1;
    obj_context->cmdbuf = NULL;
    obj_context->pnw_cmdbuf = NULL;
    obj_context->tng_cmdbuf = NULL;
#ifdef PSBVIDEO_MRFL_VPP
    obj_context->vsp_cmdbuf = NULL;
#endif
    obj_context->frame_count = 0;
    obj_context->slice_count = 0;
    obj_context->msvdx_context = ((driver_data->msvdx_context_base & 0xff0000) >> 16) |
                                 ((contextID & 0xff000000) >> 16);
#ifdef ANDROID
    obj_context->msvdx_context = ((driver_data->drm_fd & 0xf) << 4) |
                                 ((unsigned int)gettid() & 0xf);
#endif
    obj_context->profile = obj_config->profile;
    obj_context->entry_point = obj_config->entrypoint;

    /* Error recovery */
    if (VA_STATUS_SUCCESS != vaStatus) {
        if (cmdbuf_num > LNC_MAX_CMDBUFS_ENCODE)
            cmdbuf_num = LNC_MAX_CMDBUFS_ENCODE;
        for (i = 0; i < cmdbuf_num; i++) {
#ifndef BAYTRAIL
            if (obj_context->pnw_cmdbuf_list[i]) {
                pnw_cmdbuf_destroy(obj_context->pnw_cmdbuf_list[i]);
                free(obj_context->pnw_cmdbuf_list[i]);
                obj_context->pnw_cmdbuf_list[i] = NULL;
            }
#endif
#ifdef PSBVIDEO_MRFL
            if (obj_context->tng_cmdbuf_list[i]) {
                tng_cmdbuf_destroy(obj_context->tng_cmdbuf_list[i]);
                free(obj_context->tng_cmdbuf_list[i]);
                obj_context->tng_cmdbuf_list[i] = NULL;
            }
#endif
            if (obj_context->cmdbuf_list[i]) {
                psb_cmdbuf_destroy(obj_context->cmdbuf_list[i]);
                free(obj_context->cmdbuf_list[i]);
                obj_context->cmdbuf_list[i] = NULL;
            }
#ifdef PSBVIDEO_MRFL_VPP
            if (obj_context->vsp_cmdbuf_list[i]) {
                vsp_cmdbuf_destroy(obj_context->vsp_cmdbuf_list[i]);
                free(obj_context->vsp_cmdbuf_list[i]);
                obj_context->vsp_cmdbuf_list[i] = NULL;
            }
#endif
        }

        obj_context->cmdbuf = NULL;
#ifdef PSBVIDEO_MRFL_VPP
        obj_context->vsp_cmdbuf = NULL;
#endif

        obj_context->context_id = -1;
        obj_context->config_id = -1;
        obj_context->picture_width = 0;
        obj_context->picture_height = 0;
        if (NULL != obj_context->render_targets)
            free(obj_context->render_targets);
        free(obj_context->buffer_list);
        obj_context->num_buffers = 0;
        obj_context->render_targets = NULL;
        obj_context->num_render_targets = 0;
        obj_context->va_flags = 0;
        object_heap_free(&driver_data->context_heap, (object_base_p) obj_context);
    }
    obj_context->ctp_type = (((obj_config->profile << 8) |
                             obj_config->entrypoint | driver_data->protected) & 0xffff);

    /* VSP's PM rely on VPP ctx, so ved vpp use diferent profile/level for ctx */
    if (driver_data->ved_vpp)
        obj_context->ctp_type = (((obj_config->profile << 8) |
                             VAEntrypointVLD | driver_data->protected) & 0xffff);

    if (!encode) {
        obj_context->ctp_type |= ((obj_context->msvdx_tile & 0xff) << 16);
    }

    if (obj_config->profile == VAProfileVC1Simple ||
        obj_config->profile == VAProfileVC1Main ||
        obj_config->profile == VAProfileVC1Advanced) {
        uint64_t width_in_mb = ((driver_data->render_rect.x + driver_data->render_rect.width + 15) / 16);
        obj_context->ctp_type |= (width_in_mb << 32);
    }

    /* add ctx_num to save vp8 enc context num to support dual vp8 encoding */
    int ret = psb_new_context(driver_data, obj_context->ctp_type | driver_data->protected);
    if (ret)
        vaStatus = VA_STATUS_ERROR_UNKNOWN;

    DEBUG_FUNC_EXIT
    return vaStatus;
}

static VAStatus psb__allocate_malloc_buffer(object_buffer_p obj_buffer, int size)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    obj_buffer->buffer_data = realloc(obj_buffer->buffer_data, size);
    CHECK_ALLOCATION(obj_buffer->buffer_data);

    return vaStatus;
}

static VAStatus psb__unmap_buffer(object_buffer_p obj_buffer);

static VAStatus psb__allocate_BO_buffer(psb_driver_data_p driver_data, object_context_p __maybe_unused obj_context, object_buffer_p obj_buffer, int size, unsigned char *data, VABufferType type)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    ASSERT(NULL == obj_buffer->buffer_data);

    if (obj_buffer->psb_buffer && (psb_bs_queued == obj_buffer->psb_buffer->status)) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Abandoning BO for buffer %08x type %s\n", obj_buffer->base.id,
                                 buffer_type_to_string(obj_buffer->type));
        /* need to set psb_buffer aside and get another one */
        obj_buffer->psb_buffer->status = psb_bs_abandoned;
        obj_buffer->psb_buffer = NULL;
        obj_buffer->size = 0;
        obj_buffer->alloc_size = 0;
    }

    if (type == VAProtectedSliceDataBufferType) {
        if (obj_buffer->psb_buffer) {
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "RAR: old RAR slice buffer with RAR handle 0%08x, current RAR handle 0x%08x\n",
                                     obj_buffer->psb_buffer->rar_handle, (uint32_t)data);
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "RAR: force old RAR buffer destroy and new buffer re-allocation by set size=0\n");
            obj_buffer->alloc_size = 0;
        }
    }

    if (obj_buffer->alloc_size < (unsigned int)size) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Buffer size mismatch: Need %d, currently have %d\n", size, obj_buffer->alloc_size);
        if (obj_buffer->psb_buffer) {
            if (obj_buffer->buffer_data) {
                psb__unmap_buffer(obj_buffer);
            }
            psb_buffer_destroy(obj_buffer->psb_buffer);
            obj_buffer->alloc_size = 0;
        } else {
            obj_buffer->psb_buffer = (psb_buffer_p) calloc(1, sizeof(struct psb_buffer_s));
            if (NULL == obj_buffer->psb_buffer) {
                vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
                DEBUG_FAILURE;
            }
        }
        if (VA_STATUS_SUCCESS == vaStatus) {
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "Allocate new GPU buffers for vaCreateBuffer:type=%s,size=%d.\n",
                                     buffer_type_to_string(obj_buffer->type), size);

            size = (size + 0x7fff) & ~0x7fff; /* Round up */
            if (obj_buffer->type == VAImageBufferType) /* Xserver side PutSurface, Image/subpicture buffer
                                                        * should be shared between two process
                                                        */
                vaStatus = psb_buffer_create(driver_data, size, psb_bt_cpu_vpu_shared, obj_buffer->psb_buffer);
#ifndef BAYTRAIL
            else if (obj_buffer->type == VAProtectedSliceDataBufferType) {
                vaStatus = psb_buffer_reference_imr(driver_data, (uint32_t)data, obj_buffer->psb_buffer);
            }
#endif
            else if (obj_buffer->type == VAEncCodedBufferType)
                vaStatus = psb_buffer_create(driver_data, size, psb_bt_cpu_vpu, obj_buffer->psb_buffer);
            else
                vaStatus = psb_buffer_create(driver_data, size, psb_bt_cpu_vpu, obj_buffer->psb_buffer);

            if (VA_STATUS_SUCCESS != vaStatus) {
                free(obj_buffer->psb_buffer);
                obj_buffer->psb_buffer = NULL;
                DEBUG_FAILURE;
            } else {
                obj_buffer->alloc_size = size;
            }
        }
    }
    return vaStatus;
}

static VAStatus psb__map_buffer(object_buffer_p obj_buffer)
{
    if (obj_buffer->psb_buffer) {
        return psb_buffer_map(obj_buffer->psb_buffer, &obj_buffer->buffer_data);
    }
    return VA_STATUS_SUCCESS;
}

static VAStatus psb__unmap_buffer(object_buffer_p obj_buffer)
{
    if (obj_buffer->psb_buffer) {
        obj_buffer->buffer_data = NULL;
        return psb_buffer_unmap(obj_buffer->psb_buffer);
    }
    return VA_STATUS_SUCCESS;
}

static void psb__destroy_buffer(psb_driver_data_p driver_data, object_buffer_p obj_buffer)
{
    if (obj_buffer->psb_buffer) {
        if (obj_buffer->buffer_data) {
            psb__unmap_buffer(obj_buffer);
        }
        psb_buffer_destroy(obj_buffer->psb_buffer);
        free(obj_buffer->psb_buffer);
        obj_buffer->psb_buffer = NULL;
    }

    if (NULL != obj_buffer->buffer_data) {
        free(obj_buffer->buffer_data);
        obj_buffer->buffer_data = NULL;
        obj_buffer->size = 0;
    }

    object_heap_free(&driver_data->buffer_heap, (object_base_p) obj_buffer);
}

void psb__suspend_buffer(psb_driver_data_p driver_data, object_buffer_p obj_buffer)
{
    if (obj_buffer->context) {
        VABufferType type = obj_buffer->type;
        object_context_p obj_context = obj_buffer->context;

        if (type >= PSB_MAX_BUFFERTYPES) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "Invalid buffer type %d\n", type);
            return;
        }

        /* Remove buffer from active list */
        *obj_buffer->pptr_prev_next = obj_buffer->ptr_next;

        /* Add buffer to tail of unused list */
        obj_buffer->ptr_next = NULL;
        obj_buffer->last_used = obj_context->frame_count;
        if (obj_context->buffers_unused_tail[type]) {
            obj_buffer->pptr_prev_next = &(obj_context->buffers_unused_tail[type]->ptr_next);
        } else {
            obj_buffer->pptr_prev_next = &(obj_context->buffers_unused[type]);
        }
        *obj_buffer->pptr_prev_next = obj_buffer;
        obj_context->buffers_unused_tail[type] = obj_buffer;
        obj_context->buffers_unused_count[type]++;

        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Adding buffer %08x type %s to unused list. unused count = %d\n", obj_buffer->base.id,
                                 buffer_type_to_string(obj_buffer->type), obj_context->buffers_unused_count[type]);

        object_heap_suspend_object((object_base_p) obj_buffer, 1); /* suspend */
        return;
    }

    if (obj_buffer->psb_buffer && (psb_bs_queued == obj_buffer->psb_buffer->status)) {
        /* need to set psb_buffer aside */
        obj_buffer->psb_buffer->status = psb_bs_abandoned;
        obj_buffer->psb_buffer = NULL;
    }

    psb__destroy_buffer(driver_data, obj_buffer);
}

static void psb__destroy_context(psb_driver_data_p driver_data, object_context_p obj_context)
{
    int encode, i;

    if (obj_context->entry_point == VAEntrypointEncSlice)
        encode = 1;
    else
        encode = 0;

    obj_context->format_vtable->destroyContext(obj_context);

    for (i = 0; i < PSB_MAX_BUFFERTYPES; i++) {
        object_buffer_p obj_buffer;
        obj_buffer = obj_context->buffers_active[i];
        for (; obj_buffer; obj_buffer = obj_buffer->ptr_next) {
            drv_debug_msg(VIDEO_DEBUG_INIT, "%s: destroying active buffer %08x\n", __FUNCTION__, obj_buffer->base.id);
            psb__destroy_buffer(driver_data, obj_buffer);
        }
        obj_buffer = obj_context->buffers_unused[i];
        for (; obj_buffer; obj_buffer = obj_buffer->ptr_next) {
            drv_debug_msg(VIDEO_DEBUG_INIT, "%s: destroying unused buffer %08x\n", __FUNCTION__, obj_buffer->base.id);
            psb__destroy_buffer(driver_data, obj_buffer);
        }
        obj_context->buffers_unused_count[i] = 0;
    }
#ifndef BAYTRAIL
    for (i = 0; i < LNC_MAX_CMDBUFS_ENCODE; i++) {
        if (obj_context->pnw_cmdbuf_list[i]) {
            pnw_cmdbuf_destroy(obj_context->pnw_cmdbuf_list[i]);
            free(obj_context->pnw_cmdbuf_list[i]);
            obj_context->pnw_cmdbuf_list[i] = NULL;
        }
    }
#endif
#ifdef PSBVIDEO_MRFL
    for (i = 0; i < TNG_MAX_CMDBUFS_ENCODE; i++) {
        if (obj_context->tng_cmdbuf_list[i]) {
            tng_cmdbuf_destroy(obj_context->tng_cmdbuf_list[i]);
            free(obj_context->tng_cmdbuf_list[i]);
            obj_context->tng_cmdbuf_list[i] = NULL;
        }
    }
#endif
#ifdef PSBVIDEO_MRFL_VPP
    for (i = 0; i < VSP_MAX_CMDBUFS; i++) {
        if (obj_context->vsp_cmdbuf_list[i]) {
            vsp_cmdbuf_destroy(obj_context->vsp_cmdbuf_list[i]);
            free(obj_context->vsp_cmdbuf_list[i]);
            obj_context->vsp_cmdbuf_list[i] = NULL;
        }
    }
#endif

    for (i = 0; i < PSB_MAX_CMDBUFS; i++) {
        if (obj_context->cmdbuf_list[i]) {
            psb_cmdbuf_destroy(obj_context->cmdbuf_list[i]);
            free(obj_context->cmdbuf_list[i]);
            obj_context->cmdbuf_list[i] = NULL;
        }
    }
    obj_context->cmdbuf = NULL;
#ifdef PSBVIDEO_MRFL_VPP
    obj_context->vsp_cmdbuf = NULL;
#endif

    obj_context->context_id = -1;
    obj_context->config_id = -1;
    obj_context->picture_width = 0;
    obj_context->picture_height = 0;
    if (obj_context->render_targets)
        free(obj_context->render_targets);
    obj_context->render_targets = NULL;
    obj_context->num_render_targets = 0;
    obj_context->va_flags = 0;

    obj_context->current_render_target = NULL;
    obj_context->ec_target = NULL;
    obj_context->ec_candidate = NULL;
    if (obj_context->buffer_list)
        free(obj_context->buffer_list);
    obj_context->num_buffers = 0;

    object_heap_free(&driver_data->context_heap, (object_base_p) obj_context);

    psb_rm_context(driver_data);
}

VAStatus psb_DestroyContext(
    VADriverContextP ctx,
    VAContextID context
)
{
    DEBUG_FUNC_ENTER
    INIT_DRIVER_DATA
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    object_context_p obj_context = CONTEXT(context);
    CHECK_CONTEXT(obj_context);

    psb__destroy_context(driver_data, obj_context);

    DEBUG_FUNC_EXIT
    return vaStatus;
}

VAStatus psb__CreateBuffer(
    psb_driver_data_p driver_data,
    object_context_p obj_context,       /* in */
    VABufferType type,  /* in */
    unsigned int size,          /* in */
    unsigned int num_elements, /* in */
    unsigned char *data,         /* in */
    VABufferID *buf_desc    /* out */
)
{
    DEBUG_FUNC_ENTER
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    int bufferID;
    object_buffer_p obj_buffer;
    int unused_count;

    /*PSB_MAX_BUFFERTYPES is the size of array buffers_unused*/
    if (type >= PSB_MAX_BUFFERTYPES) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Invalid buffer type %d\n", type);
        return VA_STATUS_ERROR_UNSUPPORTED_BUFFERTYPE;
    }


    obj_buffer = obj_context ? obj_context->buffers_unused[type] : NULL;
    unused_count = obj_context ? obj_context->buffers_unused_count[type] : 0;

    /*
     * Buffer Management
     * For each buffer type, maintain
     *   - a LRU sorted list of unused buffers
     *   - a list of active buffers
     * We only create a new buffer when
     *   - no unused buffers are available
     *   - the last unused buffer is still queued
     *   - the last unused buffer was used very recently and may still be fenced
     *      - used recently is defined as within the current frame_count (subject to tweaks)
     *
     * The buffer that is returned will be moved to the list of active buffers
     *   - vaDestroyBuffer and vaRenderPicture will move the active buffer back to the list of unused buffers
    */
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Requesting buffer creation, size=%d,elements=%d,type=%s\n", size, num_elements,
                             buffer_type_to_string(type));

    /* on MFLD, data is IMR offset, and could be 0 */
    /*
    if ((type == VAProtectedSliceDataBufferType) && (data == NULL)) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "RAR: Create protected slice buffer, but RAR handle is NULL\n");
        return VA_STATUS_ERROR_UNSUPPORTED_BUFFERTYPE ;
    }
    */

    if (obj_buffer && obj_buffer->psb_buffer) {
        if (psb_bs_queued == obj_buffer->psb_buffer->status) {
            /* Buffer is still queued, allocate new buffer instead */
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "Skipping idle buffer %08x, still queued\n", obj_buffer->base.id);
            obj_buffer = NULL;
        } else if ((obj_buffer->last_used == obj_context->frame_count) && (unused_count < MAX_UNUSED_BUFFERS)) {
            /* Buffer was used for this frame, allocate new buffer instead */
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "Skipping idle buffer %08x, recently used. Unused = %d\n", obj_buffer->base.id, unused_count);
            obj_buffer = NULL;
        } else if (obj_context->frame_count - obj_buffer->last_used < 5) {
            /* Buffer was used for previous frame, allocate new buffer instead */
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "Skipping idle buffer %08x used by frame %d. Unused = %d\n", obj_buffer->base.id, obj_buffer->last_used, unused_count);
            obj_buffer = NULL;
        }
    }

    if (obj_buffer) {
        bufferID = obj_buffer->base.id;
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Reusing buffer %08x type %s from unused list. Unused = %d\n", bufferID,
                                 buffer_type_to_string(type), unused_count);

        /* Remove from unused list */
        obj_context->buffers_unused[type] = obj_buffer->ptr_next;
        if (obj_context->buffers_unused[type]) {
            obj_context->buffers_unused[type]->pptr_prev_next = &(obj_context->buffers_unused[type]);
            ASSERT(obj_context->buffers_unused_tail[type] != obj_buffer);
        } else {
            ASSERT(obj_context->buffers_unused_tail[type] == obj_buffer);
            obj_context->buffers_unused_tail[type] = 0;
        }
        obj_context->buffers_unused_count[type]--;

        object_heap_suspend_object((object_base_p)obj_buffer, 0); /* Make BufferID valid again */
        ASSERT(type == obj_buffer->type);
        ASSERT(obj_context == obj_buffer->context);
    } else {
        bufferID = object_heap_allocate(&driver_data->buffer_heap);
        obj_buffer = BUFFER(bufferID);
        CHECK_ALLOCATION(obj_buffer);

        MEMSET_OBJECT(obj_buffer, struct object_buffer_s);

        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Allocating new buffer %08x type %s.\n", bufferID, buffer_type_to_string(type));
        obj_buffer->type = type;
        obj_buffer->buffer_data = NULL;
        obj_buffer->psb_buffer = NULL;
        obj_buffer->size = 0;
        obj_buffer->max_num_elements = 0;
        obj_buffer->alloc_size = 0;
        obj_buffer->context = obj_context;
    }
    if (obj_context) {
        /* Add to front of active list */
        obj_buffer->ptr_next = obj_context->buffers_active[type];
        if (obj_buffer->ptr_next) {
            obj_buffer->ptr_next->pptr_prev_next = &(obj_buffer->ptr_next);
        }
        obj_buffer->pptr_prev_next = &(obj_context->buffers_active[type]);
        *obj_buffer->pptr_prev_next = obj_buffer;
    }

    switch (obj_buffer->type) {
    case VABitPlaneBufferType:
    case VASliceDataBufferType:
    case VAResidualDataBufferType:
    case VAImageBufferType:
    case VASliceGroupMapBufferType:
    case VAEncCodedBufferType:
    case VAProtectedSliceDataBufferType:
#ifdef SLICE_HEADER_PARSING
    case VAParseSliceHeaderGroupBufferType:
#endif
        vaStatus = psb__allocate_BO_buffer(driver_data, obj_context,obj_buffer, size * num_elements, data, obj_buffer->type);
        DEBUG_FAILURE;
        break;
    case VAPictureParameterBufferType:
    case VAIQMatrixBufferType:
    case VASliceParameterBufferType:
    case VAMacroblockParameterBufferType:
    case VADeblockingParameterBufferType:
    case VAEncPackedHeaderParameterBufferType:
    case VAEncPackedHeaderDataBufferType:
    case VAEncSequenceParameterBufferType:
    case VAEncPictureParameterBufferType:
    case VAEncSliceParameterBufferType:
    case VAQMatrixBufferType:
    case VAEncMiscParameterBufferType:
    case VAProbabilityBufferType:
    case VAHuffmanTableBufferType:
    case VAProcPipelineParameterBufferType:
    case VAProcFilterParameterBufferType:
#ifdef SLICE_HEADER_PARSING
    case VAParsePictureParameterBufferType:
#endif
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "Allocate new malloc buffers for vaCreateBuffer:type=%s,size=%d, buffer_data=%p.\n",
                                 buffer_type_to_string(type), size, obj_buffer->buffer_data);
        vaStatus = psb__allocate_malloc_buffer(obj_buffer, size * num_elements);
        DEBUG_FAILURE;
        break;

    default:
        vaStatus = VA_STATUS_ERROR_UNSUPPORTED_BUFFERTYPE;
        DEBUG_FAILURE;
        break;;
    }

    if (VA_STATUS_SUCCESS == vaStatus) {
        obj_buffer->size = size;
        obj_buffer->max_num_elements = num_elements;
        obj_buffer->num_elements = num_elements;
        if (data && (obj_buffer->type != VAProtectedSliceDataBufferType)) {
            vaStatus = psb__map_buffer(obj_buffer);
            if (VA_STATUS_SUCCESS == vaStatus) {
                memcpy(obj_buffer->buffer_data, data, size * num_elements);

                psb__unmap_buffer(obj_buffer);
            }
        }
    }
    if (VA_STATUS_SUCCESS == vaStatus) {
        *buf_desc = bufferID;
    } else {
        psb__destroy_buffer(driver_data, obj_buffer);
    }

    DEBUG_FUNC_EXIT
    return vaStatus;
}

VAStatus psb_CreateBuffer(
    VADriverContextP ctx,
    VAContextID context,        /* in */
    VABufferType type,  /* in */
    unsigned int size,          /* in */
    unsigned int num_elements, /* in */
    void *data,         /* in */
    VABufferID *buf_desc    /* out */
)
{
    DEBUG_FUNC_ENTER
    INIT_DRIVER_DATA
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    CHECK_INVALID_PARAM(num_elements <= 0);

    switch (type) {
    case VABitPlaneBufferType:
    case VASliceDataBufferType:
    case VAProtectedSliceDataBufferType:
    case VAResidualDataBufferType:
    case VASliceGroupMapBufferType:
    case VAPictureParameterBufferType:
    case VAIQMatrixBufferType:
    case VASliceParameterBufferType:
    case VAMacroblockParameterBufferType:
    case VADeblockingParameterBufferType:
    case VAEncCodedBufferType:
    case VAEncSequenceParameterBufferType:
    case VAEncPictureParameterBufferType:
    case VAEncSliceParameterBufferType:
    case VAEncPackedHeaderParameterBufferType:
    case VAEncPackedHeaderDataBufferType:
    case VAQMatrixBufferType:
    case VAEncMiscParameterBufferType:
    case VAProbabilityBufferType:
    case VAHuffmanTableBufferType:
    case VAProcPipelineParameterBufferType:
    case VAProcFilterParameterBufferType:
#ifdef SLICE_HEADER_PARSING
    case VAParsePictureParameterBufferType:
    case VAParseSliceHeaderGroupBufferType:
#endif
        break;

    default:
        vaStatus = VA_STATUS_ERROR_UNSUPPORTED_BUFFERTYPE;
        DEBUG_FAILURE;
        return vaStatus;
    }

    object_context_p obj_context = CONTEXT(context);
    CHECK_CONTEXT(obj_context);
    CHECK_INVALID_PARAM(buf_desc == NULL);

    vaStatus = psb__CreateBuffer(driver_data, obj_context, type, size, num_elements, data, buf_desc);

    DEBUG_FUNC_EXIT
    return vaStatus;
}


VAStatus psb_BufferInfo(
    VADriverContextP ctx,
    VABufferID buf_id,  /* in */
    VABufferType *type, /* out */
    unsigned int *size,         /* out */
    unsigned int *num_elements /* out */
)
{
    DEBUG_FUNC_ENTER
    INIT_DRIVER_DATA
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    object_buffer_p obj_buffer = BUFFER(buf_id);
    CHECK_BUFFER(obj_buffer);

    *type = obj_buffer->type;
    *size = obj_buffer->size;
    *num_elements = obj_buffer->num_elements;
    DEBUG_FUNC_EXIT
    return VA_STATUS_SUCCESS;
}


VAStatus psb_BufferSetNumElements(
    VADriverContextP ctx,
    VABufferID buf_id,    /* in */
    unsigned int num_elements    /* in */
)
{
    DEBUG_FUNC_ENTER
    INIT_DRIVER_DATA
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    object_buffer_p obj_buffer = BUFFER(buf_id);
    CHECK_BUFFER(obj_buffer);

    if ((num_elements <= 0) || (num_elements > obj_buffer->max_num_elements)) {
        vaStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
    }
    if (VA_STATUS_SUCCESS == vaStatus) {
        obj_buffer->num_elements = num_elements;
    }

    DEBUG_FUNC_EXIT
    return vaStatus;
}

VAStatus psb_MapBuffer(
    VADriverContextP ctx,
    VABufferID buf_id,    /* in */
    void **pbuf         /* out */
)
{
    DEBUG_FUNC_ENTER
    INIT_DRIVER_DATA
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    object_buffer_p obj_buffer = BUFFER(buf_id);
    CHECK_BUFFER(obj_buffer);

    CHECK_INVALID_PARAM(pbuf == NULL);

    vaStatus = psb__map_buffer(obj_buffer);
    CHECK_VASTATUS();

    if (NULL != obj_buffer->buffer_data) {
        *pbuf = obj_buffer->buffer_data;

        /* specifically for Topaz encode
         * write validate coded data offset in CodedBuffer
         */
        if (obj_buffer->type == VAEncCodedBufferType)
            psb_codedbuf_map_mangle(ctx, obj_buffer, pbuf);
        /* *(IMG_UINT32 *)((unsigned char *)obj_buffer->buffer_data + 4) = 16; */
    } else {
        vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
    }
    DEBUG_FUNC_EXIT
    return vaStatus;
}

VAStatus psb_UnmapBuffer(
    VADriverContextP ctx,
    VABufferID buf_id    /* in */
)
{
    DEBUG_FUNC_ENTER
    INIT_DRIVER_DATA
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    object_buffer_p obj_buffer = BUFFER(buf_id);
    CHECK_BUFFER(obj_buffer);

    vaStatus = psb__unmap_buffer(obj_buffer);
    DEBUG_FUNC_EXIT
    return vaStatus;
}


VAStatus psb_DestroyBuffer(
    VADriverContextP ctx,
    VABufferID buffer_id
)
{
    DEBUG_FUNC_ENTER
    INIT_DRIVER_DATA
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    object_buffer_p obj_buffer = BUFFER(buffer_id);
    if (NULL == obj_buffer) {
        return vaStatus;
    }
    psb__suspend_buffer(driver_data, obj_buffer);
    DEBUG_FUNC_EXIT
    return vaStatus;
}


VAStatus psb_BeginPicture(
    VADriverContextP ctx,
    VAContextID context,
    VASurfaceID render_target
)
{
    DEBUG_FUNC_ENTER
    INIT_DRIVER_DATA
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    object_context_p obj_context;
    object_surface_p obj_surface;
    object_config_p obj_config;

    obj_context = CONTEXT(context);
    CHECK_CONTEXT(obj_context);

    /* Must not be within BeginPicture / EndPicture already */
    ASSERT(obj_context->current_render_target == NULL);

    obj_surface = SURFACE(render_target);
    CHECK_SURFACE(obj_surface);

    obj_context->current_render_surface_id = render_target;
    obj_context->current_render_target = obj_surface;
    obj_context->slice_count = 0;

    obj_config = CONFIG(obj_context->config_id);
    if (obj_config == NULL)
        return VA_STATUS_ERROR_INVALID_CONFIG;

    /* if the surface is decode render target, and in displaying */
    if (obj_config &&
        (obj_config->entrypoint != VAEntrypointEncSlice) &&
        (driver_data->cur_displaying_surface == render_target))
        drv_debug_msg(VIDEO_DEBUG_ERROR, "WARNING: rendering a displaying surface, may see tearing\n");

    if (VA_STATUS_SUCCESS == vaStatus) {
        vaStatus = obj_context->format_vtable->beginPicture(obj_context);
    }

#ifdef ANDROID
    /* want msvdx to do rotate
     * but check per-context stream type: interlace or not
     */
    if ((obj_config->entrypoint != VAEntrypointEncSlice) &&
        (obj_config->entrypoint != VAEntrypointEncPicture)) {
        psb_RecalcAlternativeOutput(obj_context);
    }
#endif
#ifdef PSBVIDEO_MRFL_VPP_ROTATE
    if (driver_data->vpp_on && GET_SURFACE_INFO_tiling(obj_surface->psb_surface))
        driver_data->disable_msvdx_rotate = 0;
#endif
    if (obj_context->interlaced_stream || driver_data->disable_msvdx_rotate) {
        int i;
        obj_context->msvdx_rotate = 0;
        if (obj_context->num_render_targets > 0) {
            for (i = 0; i < obj_context->num_render_targets; i++) {
                object_surface_p obj_surface = SURFACE(obj_context->render_targets[i]);
                /*we invalidate all surfaces's rotate buffer share info here.*/
                if (obj_surface && obj_surface->share_info) {
                    obj_surface->share_info->surface_rotate = 0;
                }
            }
        }
    }
    else
        obj_context->msvdx_rotate = driver_data->msvdx_rotate_want;

    /* the main surface track current rotate information
     * try to reuse the allocated rotate surfaces and don't destroy them
     * thus the rotation info in obj_surface->out_loop_surface may not be updated
     */

    SET_SURFACE_INFO_rotate(obj_surface->psb_surface, obj_context->msvdx_rotate);

     if (CONTEXT_SCALING(obj_context) && obj_config->entrypoint != VAEntrypointEncSlice)
          if(VA_STATUS_SUCCESS != psb_CreateScalingSurface(obj_context, obj_surface)) {
             obj_context->msvdx_scaling = 0;
             ALOGE("%s: fail to allocate scaling surface", __func__);
          }

    if (CONTEXT_ROTATE(obj_context)) {
#ifdef PSBVIDEO_MRFL_VPP_ROTATE
        /* The VSP rotation is just for 1080P with tilling */
        if (driver_data->vpp_on && GET_SURFACE_INFO_tiling(obj_surface->psb_surface)) {
            if (obj_config->entrypoint == VAEntrypointVideoProc)
                vaStatus = psb_CreateRotateSurface(obj_context, obj_surface, obj_context->msvdx_rotate);
            else {
                SET_SURFACE_INFO_rotate(obj_surface->psb_surface, 0);
                obj_context->msvdx_rotate = 0;
                vaStatus = psb_DestroyRotateBuffer(obj_context, obj_surface);
            }
        } else
#endif
        vaStatus = psb_CreateRotateSurface(obj_context, obj_surface, obj_context->msvdx_rotate);
        if (VA_STATUS_SUCCESS !=vaStatus)
            ALOGE("%s: fail to allocate out loop surface", __func__);

    } else {
        if (obj_surface && obj_surface->share_info) {
            obj_surface->share_info->metadata_rotate = VAROTATION2HAL(driver_data->va_rotate);
            obj_surface->share_info->surface_rotate = VAROTATION2HAL(obj_context->msvdx_rotate);
        }
    }

    if (obj_surface && obj_surface->share_info &&
        obj_config->entrypoint == VAEntrypointVLD) {
        obj_surface->share_info->crop_width = driver_data->render_rect.width;
        obj_surface->share_info->crop_height = driver_data->render_rect.height;
    }

    if (driver_data->is_oold &&  !obj_surface->psb_surface->in_loop_buf) {
        psb_surface_p psb_surface = obj_surface->psb_surface;

        psb_surface->in_loop_buf = calloc(1, sizeof(struct psb_buffer_s));
        CHECK_ALLOCATION(psb_surface->in_loop_buf);

        /* FIXME: For RAR surface, need allocate RAR buffer  */
        vaStatus = psb_buffer_create(obj_context->driver_data,
                                     psb_surface->size,
                                     psb_bt_surface,
                                     psb_surface->in_loop_buf);
    } else if (!driver_data->is_oold && obj_surface->psb_surface->in_loop_buf) {
        psb_surface_p psb_surface = obj_surface->psb_surface;

        psb_buffer_destroy(psb_surface->in_loop_buf);
        free(psb_surface->in_loop_buf);
        psb_surface->in_loop_buf = NULL;
    }
    obj_context->is_oold = driver_data->is_oold;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "---BeginPicture 0x%08x for frame %d --\n",
                             render_target, obj_context->frame_count);
    psb__trace_message("------Trace frame %d------\n", obj_context->frame_count);

    DEBUG_FUNC_EXIT
    return vaStatus;
}

VAStatus psb_RenderPicture(
    VADriverContextP ctx,
    VAContextID context,
    VABufferID *buffers,
    int num_buffers
)
{
    DEBUG_FUNC_ENTER
    INIT_DRIVER_DATA
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    object_context_p obj_context;
    object_buffer_p *buffer_list;
    int i;

    obj_context = CONTEXT(context);
    CHECK_CONTEXT(obj_context);

    CHECK_INVALID_PARAM(num_buffers <= 0);
    /* Don't crash on NULL pointers */
    CHECK_BUFFER(buffers);
    /* Must be within BeginPicture / EndPicture */
    ASSERT(obj_context->current_render_target != NULL);

    if (num_buffers > obj_context->num_buffers) {
        free(obj_context->buffer_list);

        obj_context->buffer_list = (object_buffer_p *) calloc(1, sizeof(object_buffer_p) * num_buffers);
        if (obj_context->buffer_list == NULL) {
            vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
            obj_context->num_buffers = 0;
        }

        obj_context->num_buffers = num_buffers;
    }
    buffer_list = obj_context->buffer_list;

    if (VA_STATUS_SUCCESS == vaStatus) {
        /* Lookup buffer references */
        for (i = 0; i < num_buffers; i++) {
            object_buffer_p obj_buffer = BUFFER(buffers[i]);
            CHECK_BUFFER(obj_buffer);

            buffer_list[i] = obj_buffer;
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "Render buffer %08x type %s\n", obj_buffer->base.id,
                                     buffer_type_to_string(obj_buffer->type));
        }
    }

    if (VA_STATUS_SUCCESS == vaStatus) {
        vaStatus = obj_context->format_vtable->renderPicture(obj_context, buffer_list, num_buffers);
    }

    if (buffer_list) {
        /* Release buffers */
        for (i = 0; i < num_buffers; i++) {
            if (buffer_list[i]) {
                psb__suspend_buffer(driver_data, buffer_list[i]);
            }
        }
    }

    DEBUG_FUNC_EXIT
    return vaStatus;
}

VAStatus psb_EndPicture(
    VADriverContextP ctx,
    VAContextID context
)
{
    DEBUG_FUNC_ENTER
    INIT_DRIVER_DATA
    VAStatus vaStatus;
    object_context_p obj_context;

    obj_context = CONTEXT(context);
    CHECK_CONTEXT(obj_context);

    vaStatus = obj_context->format_vtable->endPicture(obj_context);

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "---EndPicture for frame %d --\n", obj_context->frame_count);

    obj_context->current_render_target = NULL;
    obj_context->frame_count++;

    psb__trace_message("FrameCount = %03d\n", obj_context->frame_count);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "FrameCount = %03d\n", obj_context->frame_count);
    psb__trace_message(NULL);


    //psb_SyncSurface(ctx, obj_context->current_render_surface_id);
    DEBUG_FUNC_EXIT
    return vaStatus;
}


static void psb__surface_usage(
    psb_driver_data_p driver_data,
    object_surface_p obj_surface,
    int *decode, int *encode, int *rc_enable, int *proc
)
{
    object_context_p obj_context;
    object_config_p obj_config;
    VAEntrypoint tmp;
    unsigned int eRCmode;
    int i;


    *decode = 0;
    *encode = 0;
    *rc_enable = 0;
    *proc = 0;

    obj_context = CONTEXT(obj_surface->context_id);
    if (NULL == obj_context) /* not associate with a context */
        return;

    obj_config = CONFIG(obj_context->config_id);
    if (NULL == obj_config) /* not have a validate context */
        return;

    tmp = obj_config->entrypoint;

    *encode = (tmp == VAEntrypointEncSlice) || (tmp == VAEntrypointEncPicture);
    *decode = (VAEntrypointVLD <= tmp) && (tmp <= VAEntrypointDeblocking);
#ifdef PSBVIDEO_MRFL_VPP
    *proc = (VAEntrypointVideoProc == tmp);
#endif

    if (*encode) {
        for (i = 0; i < obj_config->attrib_count; i++) {
            if (obj_config->attrib_list[i].type == VAConfigAttribRateControl)
                break;
        }

        if (i >= obj_config->attrib_count)
            eRCmode = VA_RC_NONE;
        else
            eRCmode = obj_config->attrib_list[i].value;

        if (eRCmode == VA_RC_NONE)
            *rc_enable = 0;
        else
            *rc_enable = 1;
    }
}

VAStatus psb_SyncSurface(
    VADriverContextP ctx,
    VASurfaceID render_target
)
{
    DEBUG_FUNC_ENTER
    INIT_DRIVER_DATA
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    object_surface_p obj_surface;
    int decode = 0, encode = 0, rc_enable = 0, proc = 0;
    object_context_p obj_context = NULL;
    object_config_p obj_config = NULL;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "psb_SyncSurface: 0x%08x\n", render_target);

    obj_surface = SURFACE(render_target);
    CHECK_SURFACE(obj_surface);

    obj_context = CONTEXT(obj_surface->context_id);
    if (obj_context) {
        obj_config = CONFIG(obj_context->config_id);
    }

    /* The cur_displaying_surface indicates the surface being displayed by overlay.
     * The diaplay_timestamp records the time point of put surface, which would
     * be set to zero while using texture blit.*/

    /* don't use mutex here for performance concern... */
    //pthread_mutex_lock(&output->output_mutex);
    if (render_target == driver_data->cur_displaying_surface)
        vaStatus = VA_STATUS_ERROR_SURFACE_IN_DISPLAYING;
    else if ((VA_INVALID_SURFACE != driver_data->cur_displaying_surface)    /* use overlay */
             && (render_target == driver_data->last_displaying_surface)) {  /* It's the last displaying surface*/
        object_surface_p cur_obj_surface = SURFACE(driver_data->cur_displaying_surface);
        /*  The flip operation on current displaying surface could be delayed to
         *  next VBlank and hadn't been finished yet. Then, the last displaying
         *  surface shouldn't be freed, because the hardware may not
         *  complete loading data of it. Any change of the last surface could
         *  have a impect on the scrren.*/
        if (NULL != cur_obj_surface) {
            while ((GetTickCount() - cur_obj_surface->display_timestamp) < PSB_MAX_FLIP_DELAY)
                usleep(PSB_MAX_FLIP_DELAY * 1000);
        }
    }
    //pthread_mutex_unlock(&output->output_mutex);

    if (vaStatus != VA_STATUS_ERROR_SURFACE_IN_DISPLAYING) {
#ifdef PSBVIDEO_MRFL_VPP_ROTATE
        /* For VPP buffer, will sync the rotated buffer */
        if (obj_config && obj_config->entrypoint == VAEntrypointVideoProc) {
            if (GET_SURFACE_INFO_tiling(obj_surface->psb_surface) &&
                (obj_context->msvdx_rotate == VA_ROTATION_90 || obj_context->msvdx_rotate == VA_ROTATION_270) &&
                obj_surface->out_loop_surface)
                vaStatus = psb_surface_sync(obj_surface->out_loop_surface);
            else
                vaStatus = psb_surface_sync(obj_surface->psb_surface);
        } else
#endif
        vaStatus = psb_surface_sync(obj_surface->psb_surface);
    }

    /* report any error of decode for Android */
    psb__surface_usage(driver_data, obj_surface, &decode, &encode, &rc_enable, &proc);
#if 0
    if (decode && IS_MRST(driver_data)) {
        struct drm_lnc_video_getparam_arg arg;
        uint32_t ret, handle, fw_status = 0;
        handle = wsbmKBufHandle(wsbmKBuf(obj_surface->psb_surface->buf.drm_buf));
        arg.key = IMG_VIDEO_DECODE_STATUS;
        arg.arg = (uint64_t)((unsigned long) & handle);
        arg.value = (uint64_t)((unsigned long) & fw_status);
        ret = drmCommandWriteRead(driver_data->drm_fd, driver_data->getParamIoctlOffset,
                                  &arg, sizeof(arg));
        if (ret == 0) {
            if (fw_status != 0)
                vaStatus = VA_STATUS_ERROR_DECODING_ERROR;
        } else {
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "IMG_VIDEO_DECODE_STATUS ioctl return failed.\n");
            vaStatus = VA_STATUS_ERROR_UNKNOWN;
        }
    } else if (proc && IS_MRFL(driver_data)) {
        /* FIXME: does it need a new surface sync mechanism for FRC? */
    }
#endif
    if (proc && IS_MRFL(driver_data)) {
        /* FIXME: does it need a new surface sync mechanism for FRC? */
    }

    //psb__dump_NV_buffers(obj_surface->psb_surface, 0, 0, obj_surface->width, obj_surface->height);
    //psb__dump_NV_buffers(obj_surface->psb_surface_rotate, 0, 0, obj_surface->height, ((obj_surface->width + 0x1f) & (~0x1f)));
    if (obj_surface->scaling_surface)
        psb__dump_NV12_buffers(obj_surface->scaling_surface, 0, 0, obj_surface->width_s, obj_surface->height_s);
    DEBUG_FAILURE;
    DEBUG_FUNC_EXIT
    return vaStatus;
}


VAStatus psb_QuerySurfaceStatus(
    VADriverContextP ctx,
    VASurfaceID render_target,
    VASurfaceStatus *status    /* out */
)
{
    DEBUG_FUNC_ENTER
    INIT_DRIVER_DATA
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    object_surface_p obj_surface;
    VASurfaceStatus surface_status;
    int frame_skip = 0, encode = 0, decode = 0, rc_enable = 0, proc = 0;
    object_context_p obj_context = NULL;

    obj_surface = SURFACE(render_target);
    CHECK_SURFACE(obj_surface);

    CHECK_INVALID_PARAM(status == NULL);

    psb__surface_usage(driver_data, obj_surface, &decode, &encode, &rc_enable, &proc);
#ifdef PSBVIDEO_MRFL_VPP_ROTATE
    /* For VPP 1080P, will query the rotated buffer */
    if (proc) {
        obj_context = CONTEXT(obj_surface->context_id);
        CHECK_CONTEXT(obj_context);
        if (GET_SURFACE_INFO_tiling(obj_surface->psb_surface) &&
            (obj_context->msvdx_rotate == VA_ROTATION_90 || obj_context->msvdx_rotate == VA_ROTATION_270) &&
            obj_surface->out_loop_surface)
            vaStatus = psb_surface_query_status(obj_surface->out_loop_surface, &surface_status);
        else
            vaStatus = psb_surface_query_status(obj_surface->psb_surface, &surface_status);
    } else
#endif
        vaStatus = psb_surface_query_status(obj_surface->psb_surface, &surface_status);

    /* The cur_displaying_surface indicates the surface being displayed by overlay.
     * The diaplay_timestamp records the time point of put surface, which would
     * be set to zero while using texture blit.*/
    pthread_mutex_lock(&driver_data->output_mutex);
    if (render_target == driver_data->cur_displaying_surface)
        surface_status = VASurfaceDisplaying;
    else if ((VA_INVALID_SURFACE != driver_data->cur_displaying_surface)    /* use overlay */
             && (render_target == driver_data->last_displaying_surface)) {  /* It's the last displaying surface*/
        object_surface_p cur_obj_surface = SURFACE(driver_data->cur_displaying_surface);
        /*The flip operation on current displaying surface could be delayed to
         *  next VBlank and hadn't been finished yet. Then, the last displaying
         *  surface shouldn't be freed, because the hardware may not
         *  complete loading data of it. Any change of the last surface could
         *  have a impect on the scrren.*/
        if ((NULL != cur_obj_surface)
            && ((GetTickCount() - cur_obj_surface->display_timestamp) < PSB_MAX_FLIP_DELAY)) {
            surface_status = VASurfaceDisplaying;
        }
    }
    pthread_mutex_unlock(&driver_data->output_mutex);

    /* try to get frameskip flag for encode */
#ifndef BAYTRAIL
    if (!decode) {
        /* The rendering surface may not be associated with any context. So driver should
           check the frame skip flag even variable encode is 0 */
#ifdef PSBVIDEO_MRFL
        if (IS_MRFL(driver_data))
            tng_surface_get_frameskip(driver_data, obj_surface->psb_surface, &frame_skip);
        else
#endif
            pnw_surface_get_frameskip(driver_data, obj_surface->psb_surface, &frame_skip);

        if (frame_skip == 1) {
            surface_status = surface_status | VASurfaceSkipped;
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s next frame of 0x%08x is skipped",
                    __FUNCTION__, render_target);
        }
    } else
#endif
    if (decode) {
#ifdef ANDROID
        if (obj_surface->psb_surface->buf.handle) {
            buffer_handle_t handle = obj_surface->psb_surface->buf.handle;
            int display_status;
            int err;
            err = gralloc_getdisplaystatus(handle, &display_status);
            if (!err) {
                if (display_status)
                    surface_status = VASurfaceDisplaying;
                else
                    surface_status = VASurfaceReady;
            } else {
                surface_status = VASurfaceReady;
            }

            /* if not used by display, then check whether surface used by widi */
            if (surface_status == VASurfaceReady && obj_surface->share_info) {
                if (obj_surface->share_info->renderStatus == 1) {
                    surface_status = VASurfaceDisplaying;
                }
            }
        }
#endif
    } else if (proc) {
        /* FIXME: does it need a new surface sync mechanism for FRC? */
    }

    *status = surface_status;
    DEBUG_FUNC_EXIT
    return vaStatus;
}

VAStatus psb_QuerySurfaceError(
    VADriverContextP ctx,
    VASurfaceID render_target,
    VAStatus error_status,
    void **error_info /*out*/
)
{
    DEBUG_FUNC_ENTER
    INIT_DRIVER_DATA
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    object_surface_p obj_surface;
    uint32_t i;

    obj_surface = SURFACE(render_target);
    CHECK_SURFACE(obj_surface);

#ifdef PSBVIDEO_MSVDX_EC
    if (driver_data->ec_enabled == 0) {
#else
    {
#endif
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "error concealment is not supported for this profile.\n");
        error_info = NULL;
        return VA_STATUS_ERROR_UNKNOWN;
    }

    if (error_status == VA_STATUS_ERROR_DECODING_ERROR) {
        drm_psb_msvdx_decode_status_t *decode_status = driver_data->msvdx_decode_status;
        struct drm_lnc_video_getparam_arg arg;
        uint32_t ret, handle;
        handle = wsbmKBufHandle(wsbmKBuf(obj_surface->psb_surface->buf.drm_buf));

        arg.key = IMG_VIDEO_MB_ERROR;
        arg.arg = (uint64_t)((unsigned long) & handle);
        arg.value = (uint64_t)((unsigned long)decode_status);
        ret = drmCommandWriteRead(driver_data->drm_fd, driver_data->getParamIoctlOffset,
                                  &arg, sizeof(arg));
        if (ret != 0) {
                drv_debug_msg(VIDEO_DEBUG_GENERAL,"return value is %d drmCommandWriteRead\n",ret);
                return VA_STATUS_ERROR_UNKNOWN;
        }
#ifndef _FOR_FPGA_
        if (decode_status->num_region > MAX_MB_ERRORS) {
            drv_debug_msg(VIDEO_DEBUG_GENERAL, "too much mb errors are reported.\n");
            return VA_STATUS_ERROR_UNKNOWN;
        }
        i = 0;
        for (i = 0; i < decode_status->num_region; ++i) {
            driver_data->surface_mb_error[i].status = 1;
            driver_data->surface_mb_error[i].start_mb = decode_status->mb_regions[i].start;
            driver_data->surface_mb_error[i].end_mb = decode_status->mb_regions[i].end;
            //driver_data->surface_mb_error[i].start_mb = decode_status->start_error_mb_list[i];
            //driver_data->surface_mb_error[i].end_mb = decode_status->end_error_mb_list[i];
            //driver_data->surface_mb_error[i].decode_error_type = decode_status->slice_missing_or_error[i];
        }
#endif
        driver_data->surface_mb_error[i].status = -1;
        *error_info = driver_data->surface_mb_error;
    } else {
        error_info = NULL;
        return VA_STATUS_ERROR_UNKNOWN;
    }
    DEBUG_FUNC_EXIT
    return vaStatus;
}

#define PSB_MAX_SURFACE_ATTRIBUTES 16

VAStatus psb_QuerySurfaceAttributes(VADriverContextP ctx,
                            VAConfigID config,
                            VASurfaceAttrib *attrib_list,
                            unsigned int *num_attribs)
{
    DEBUG_FUNC_ENTER
    INIT_DRIVER_DATA

    VAStatus vaStatus = VA_STATUS_SUCCESS;
    object_config_p obj_config;
    unsigned int i = 0;

    CHECK_INVALID_PARAM(num_attribs == NULL);

    if (attrib_list == NULL) {
        *num_attribs = PSB_MAX_SURFACE_ATTRIBUTES;
        return VA_STATUS_SUCCESS;
    }

    obj_config = CONFIG(config);
    CHECK_CONFIG(obj_config);

    VASurfaceAttrib *attribs = NULL;
    attribs = malloc(PSB_MAX_SURFACE_ATTRIBUTES *sizeof(VASurfaceAttrib));
    if (attribs == NULL)
        return VA_STATUS_ERROR_ALLOCATION_FAILED;

    attribs[i].type = VASurfaceAttribPixelFormat;
    attribs[i].value.type = VAGenericValueTypeInteger;
    attribs[i].flags = VA_SURFACE_ATTRIB_GETTABLE | VA_SURFACE_ATTRIB_SETTABLE;
    attribs[i].value.value.i = VA_FOURCC('N', 'V', '1', '2');
    i++;

    attribs[i].type = VASurfaceAttribMemoryType;
    attribs[i].value.type = VAGenericValueTypeInteger;
    attribs[i].flags = VA_SURFACE_ATTRIB_GETTABLE | VA_SURFACE_ATTRIB_SETTABLE;
    if (obj_config->entrypoint == VAEntrypointEncSlice && obj_config->profile == VAProfileVP8Version0_3) {
        attribs[i].value.value.i = VA_SURFACE_ATTRIB_MEM_TYPE_VA |
            VA_SURFACE_ATTRIB_MEM_TYPE_KERNEL_DRM |
            VA_SURFACE_ATTRIB_MEM_TYPE_ANDROID_GRALLOC |
            VA_SURFACE_ATTRIB_MEM_TYPE_ANDROID_ION;
    } else {
        attribs[i].value.value.i = VA_SURFACE_ATTRIB_MEM_TYPE_VA |
            VA_SURFACE_ATTRIB_MEM_TYPE_KERNEL_DRM |
            VA_SURFACE_ATTRIB_MEM_TYPE_USER_PTR |
            VA_SURFACE_ATTRIB_MEM_TYPE_ANDROID_GRALLOC |
            VA_SURFACE_ATTRIB_MEM_TYPE_ANDROID_ION;
    }
    i++;

    attribs[i].type = VASurfaceAttribExternalBufferDescriptor;
    attribs[i].value.type = VAGenericValueTypePointer;
    attribs[i].flags = VA_SURFACE_ATTRIB_SETTABLE;
    attribs[i].value.value.p = NULL;
    i++;

    //modules have speical formats to support
    if (obj_config->entrypoint == VAEntrypointVLD) { /* decode */

    } else if (obj_config->entrypoint == VAEntrypointEncSlice ||  /* encode */
                   obj_config->entrypoint == VAEntrypointEncPicture) {
    #ifdef PSBVIDEO_MFLD
        if (IS_MFLD(driver_data)) {}
    #endif
    #ifdef PSBVIDEO_MRFL
        if (IS_MRFL(driver_data)) {}
    #endif
    #ifdef BAYTRAIL
        if (IS_BAYTRAIL(driver_data)) {}
    #endif
    }
    else if (obj_config->entrypoint == VAEntrypointVideoProc) { /* vpp */

    }

    if (i > *num_attribs) {
        *num_attribs = i;
        return VA_STATUS_ERROR_MAX_NUM_EXCEEDED;
    }

    *num_attribs = i;
    memcpy(attrib_list, attribs, i * sizeof(*attribs));
    free(attribs);

    DEBUG_FUNC_EXIT
    return vaStatus;
}

VAStatus psb_LockSurface(
    VADriverContextP ctx,
    VASurfaceID surface,
    unsigned int *fourcc, /* following are output argument */
    unsigned int *luma_stride,
    unsigned int *chroma_u_stride,
    unsigned int *chroma_v_stride,
    unsigned int *luma_offset,
    unsigned int *chroma_u_offset,
    unsigned int *chroma_v_offset,
    unsigned int *buffer_name,
    void **buffer
)
{
    DEBUG_FUNC_ENTER
    INIT_DRIVER_DATA
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    unsigned char *surface_data;
    int ret;

    object_surface_p obj_surface = SURFACE(surface);
    psb_surface_p psb_surface;
    CHECK_SURFACE(obj_surface);

    psb_surface = obj_surface->psb_surface;
    if (buffer_name)
        *buffer_name = (uint32_t)(wsbmKBufHandle(wsbmKBuf(psb_surface->buf.drm_buf)));

    if (buffer) { /* map the surface buffer */
        uint32_t srf_buf_ofs = 0;
        ret = psb_buffer_map(&psb_surface->buf, &surface_data);
        if (ret) {
            *buffer = NULL;
            vaStatus = VA_STATUS_ERROR_UNKNOWN;
            DEBUG_FAILURE;
            return vaStatus;
        }
        srf_buf_ofs = psb_surface->buf.buffer_ofs;
        *buffer = surface_data + srf_buf_ofs;
    }

    *fourcc = VA_FOURCC_NV12;
    *luma_stride = psb_surface->stride;
    *chroma_u_stride = psb_surface->stride;
    *chroma_v_stride = psb_surface->stride;
    *luma_offset = 0;
    *chroma_u_offset = obj_surface->height * psb_surface->stride;
    *chroma_v_offset = obj_surface->height * psb_surface->stride + 1;
    DEBUG_FUNC_EXIT
    return vaStatus;
}


VAStatus psb_UnlockSurface(
    VADriverContextP ctx,
    VASurfaceID surface
)
{
    DEBUG_FUNC_ENTER
    INIT_DRIVER_DATA
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    object_surface_p obj_surface = SURFACE(surface);
    CHECK_SURFACE(obj_surface);

    psb_surface_p psb_surface = obj_surface->psb_surface;

    psb_buffer_unmap(&psb_surface->buf);

    DEBUG_FUNC_EXIT
    return VA_STATUS_SUCCESS;
}

VAStatus psb_GetEGLClientBufferFromSurface(
    VADriverContextP ctx,
    VASurfaceID surface,
    void **buffer
)
{
    DEBUG_FUNC_ENTER
    INIT_DRIVER_DATA
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    object_surface_p obj_surface = SURFACE(surface);
    CHECK_SURFACE(obj_surface);

    psb_surface_p psb_surface = obj_surface->psb_surface;
    *buffer = (unsigned char *)psb_surface->bc_buffer;

    DEBUG_FUNC_EXIT
    return vaStatus;
}

VAStatus psb_PutSurfaceBuf(
    VADriverContextP ctx,
    VASurfaceID surface,
    unsigned char __maybe_unused * data,
    int __maybe_unused * data_len,
    short __maybe_unused srcx,
    short __maybe_unused srcy,
    unsigned short __maybe_unused srcw,
    unsigned short __maybe_unused srch,
    short __maybe_unused destx,
    short __maybe_unused desty,
    unsigned short __maybe_unused destw,
    unsigned short __maybe_unused desth,
    VARectangle __maybe_unused * cliprects, /* client supplied clip list */
    unsigned int __maybe_unused number_cliprects, /* number of clip rects in the clip list */
    unsigned int __maybe_unused flags /* de-interlacing flags */
)
{
    DEBUG_FUNC_ENTER
    INIT_DRIVER_DATA;
    object_surface_p obj_surface = SURFACE(surface);
    psb_surface_p psb_surface;

    obj_surface = SURFACE(surface);
    if (obj_surface == NULL)
        return VA_STATUS_ERROR_INVALID_SURFACE;

    psb_surface = obj_surface->psb_surface;

#if 0
    psb_putsurface_textureblit(ctx, data, surface, srcx, srcy, srcw, srch, destx, desty, destw, desth, 1, /* check subpicture */
                               obj_surface->width, obj_surface->height,
                               psb_surface->stride, psb_surface->buf.drm_buf,
                               psb_surface->buf.pl_flags, 1 /* wrap dst */);
#endif

    DEBUG_FUNC_EXIT
    return VA_STATUS_SUCCESS;
}

VAStatus psb_SetTimestampForSurface(
    VADriverContextP ctx,
    VASurfaceID surface,
    long long timestamp
)
{
    INIT_DRIVER_DATA;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    object_surface_p obj_surface = SURFACE(surface);

    obj_surface = SURFACE(surface);
    CHECK_SURFACE(obj_surface);

    if (obj_surface->share_info) {
        obj_surface->share_info->timestamp = timestamp;
        return VA_STATUS_SUCCESS;
    } else {
        return VA_STATUS_ERROR_UNKNOWN;
    }
}

int  LOCK_HARDWARE(psb_driver_data_p driver_data)
{
    char ret = 0;

    if (driver_data->dri2 || driver_data->dri_dummy)
        return 0;

    pthread_mutex_lock(&driver_data->drm_mutex);
    DRM_CAS(driver_data->drm_lock, driver_data->drm_context,
            (DRM_LOCK_HELD | driver_data->drm_context), ret);
    if (ret) {
        ret = drmGetLock(driver_data->drm_fd, driver_data->drm_context, 0);
        /* driver_data->contended_lock=1; */
    }

    return ret;
}

int UNLOCK_HARDWARE(psb_driver_data_p driver_data)
{
    /* driver_data->contended_lock=0; */
    if (driver_data->dri2 || driver_data->dri_dummy)
        return 0;

    DRM_UNLOCK(driver_data->drm_fd, driver_data->drm_lock, driver_data->drm_context);
    pthread_mutex_unlock(&driver_data->drm_mutex);

    return 0;
}


static void psb__deinitDRM(VADriverContextP ctx)
{
    INIT_DRIVER_DATA

    if (driver_data->main_pool) {
        driver_data->main_pool->takeDown(driver_data->main_pool);
        driver_data->main_pool = NULL;
    }
    if (driver_data->fence_mgr) {
        wsbmFenceMgrTTMTakedown(driver_data->fence_mgr);
        driver_data->fence_mgr = NULL;
    }

    if (wsbmIsInitialized())
        wsbmTakedown();

    driver_data->drm_fd = -1;
}


static VAStatus psb__initDRI(VADriverContextP ctx)
{
    INIT_DRIVER_DATA
    struct drm_state *drm_state = (struct drm_state *)ctx->drm_state;

    assert(dri_state);
#ifdef _FOR_FPGA_
    dri_state->driConnectedFlag = VA_DUMMY;
    /* ON FPGA machine, psb may co-exist with gfx's drm driver */
    dri_state->fd = open("/dev/dri/card1", O_RDWR);
    if (dri_state->fd < 0)
        dri_state->fd = open("/dev/dri/card0", O_RDWR);
    assert(dri_state->fd >= 0);
#endif
    assert(dri_state->driConnectedFlag == VA_DRI2 ||
           dri_state->driConnectedFlag == VA_DUMMY);

    driver_data->drm_fd = drm_state->fd;
    driver_data->dri_dummy = 1;
    driver_data->dri2 = 0;
    driver_data->ws_priv = NULL;
    driver_data->bus_id = NULL;

    return VA_STATUS_SUCCESS;
}


static VAStatus psb__initTTM(VADriverContextP ctx)
{
    INIT_DRIVER_DATA

    const char drm_ext[] = "psb_ttm_placement_alphadrop";
    union drm_psb_extension_arg arg;
    struct _WsbmBufferPool *pool;
    int ret;
    const char exec_ext[] = "psb_ttm_execbuf_alphadrop";
    union drm_psb_extension_arg exec_arg;
    const char lncvideo_getparam_ext[] = "lnc_video_getparam";
    union drm_psb_extension_arg lncvideo_getparam_arg;

    /* init wsbm
     * WSBM node is not used in driver, thus can pass NULL Node callback
     */
    ret = wsbmInit(wsbmNullThreadFuncs(), NULL/*psbVNodeFuncs()*/);
    if (ret) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "failed initializing libwsbm.\n");
        return VA_STATUS_ERROR_UNKNOWN;
    }

    strncpy(arg.extension, drm_ext, sizeof(arg.extension));
    /* FIXME: should check dri enabled?
     * it seems not init dri here at all
     */
    ret = drmCommandWriteRead(driver_data->drm_fd, DRM_PSB_EXTENSION,
                              &arg, sizeof(arg));
    if (ret != 0 || !arg.rep.exists) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "failed to detect DRM extension \"%s\", fd=%d\n",
                      drm_ext, driver_data->drm_fd);
        drv_debug_msg(VIDEO_DEBUG_ERROR, "found error %s (ret=%d), arg.rep.exists=%d",
                      strerror(errno), ret, arg.rep.exists);

        driver_data->main_pool = NULL;
        return VA_STATUS_ERROR_UNKNOWN;
    } else {
        pool = wsbmTTMPoolInit(driver_data->drm_fd,
                               arg.rep.driver_ioctl_offset);
        if (pool == NULL) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "failed to get ttm pool\n");
            return VA_STATUS_ERROR_UNKNOWN;
        }
        driver_data->main_pool = pool;
    }

    strncpy(exec_arg.extension, exec_ext, sizeof(exec_arg.extension));
    ret = drmCommandWriteRead(driver_data->drm_fd, DRM_PSB_EXTENSION, &exec_arg,
                              sizeof(exec_arg));
    if (ret != 0 || !exec_arg.rep.exists) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "failed to detect DRM extension \"%s\".\n",
                           exec_ext);
        return FALSE;
    }
    driver_data->execIoctlOffset = exec_arg.rep.driver_ioctl_offset;

    strncpy(lncvideo_getparam_arg.extension, lncvideo_getparam_ext, sizeof(lncvideo_getparam_arg.extension));
    ret = drmCommandWriteRead(driver_data->drm_fd, DRM_PSB_EXTENSION, &lncvideo_getparam_arg,
                              sizeof(lncvideo_getparam_arg));
    if (ret != 0 || !lncvideo_getparam_arg.rep.exists) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "failed to detect DRM extension \"%s\".\n",
                           lncvideo_getparam_ext);
        /* return FALSE; */ /* not reture FALSE, so it still can run */
    }
    driver_data->getParamIoctlOffset = lncvideo_getparam_arg.rep.driver_ioctl_offset;
    return VA_STATUS_SUCCESS;
}

static VAStatus psb__initDRM(VADriverContextP ctx)
{
    VAStatus vaStatus;

    vaStatus = psb__initDRI(ctx);

    if (vaStatus == VA_STATUS_SUCCESS)
        return psb__initTTM(ctx);
    else
        return vaStatus;
}

VAStatus psb_Terminate(VADriverContextP ctx)
{
    DEBUG_FUNC_ENTER
    INIT_DRIVER_DATA
    object_subpic_p obj_subpic;
    object_image_p obj_image;
    object_buffer_p obj_buffer;
    object_surface_p obj_surface;
    object_context_p obj_context;
    object_config_p obj_config;
    object_heap_iterator iter;

    drv_debug_msg(VIDEO_DEBUG_INIT, "vaTerminate: begin to tear down\n");

    /* Clean up left over contexts */
    obj_context = (object_context_p) object_heap_first(&driver_data->context_heap, &iter);
    while (obj_context) {
        drv_debug_msg(VIDEO_DEBUG_INIT, "vaTerminate: contextID %08x still allocated, destroying\n", obj_context->base.id);
        psb__destroy_context(driver_data, obj_context);
        obj_context = (object_context_p) object_heap_next(&driver_data->context_heap, &iter);
    }
    object_heap_destroy(&driver_data->context_heap);

    /* Clean up SubpicIDs */
    obj_subpic = (object_subpic_p) object_heap_first(&driver_data->subpic_heap, &iter);
    while (obj_subpic) {
        drv_debug_msg(VIDEO_DEBUG_INIT, "vaTerminate: subpictureID %08x still allocated, destroying\n", obj_subpic->base.id);
        psb__destroy_subpicture(driver_data, obj_subpic);
        obj_subpic = (object_subpic_p) object_heap_next(&driver_data->subpic_heap, &iter);
    }
    object_heap_destroy(&driver_data->subpic_heap);

    /* Clean up ImageIDs */
    obj_image = (object_image_p) object_heap_first(&driver_data->image_heap, &iter);
    while (obj_image) {
        drv_debug_msg(VIDEO_DEBUG_INIT, "vaTerminate: imageID %08x still allocated, destroying\n", obj_image->base.id);
        psb__destroy_image(driver_data, obj_image);
        obj_image = (object_image_p) object_heap_next(&driver_data->image_heap, &iter);
    }
    object_heap_destroy(&driver_data->image_heap);

    /* Clean up left over buffers */
    obj_buffer = (object_buffer_p) object_heap_first(&driver_data->buffer_heap, &iter);
    while (obj_buffer) {
        drv_debug_msg(VIDEO_DEBUG_INIT, "vaTerminate: bufferID %08x still allocated, destroying\n", obj_buffer->base.id);
        psb__destroy_buffer(driver_data, obj_buffer);
        obj_buffer = (object_buffer_p) object_heap_next(&driver_data->buffer_heap, &iter);
    }
    object_heap_destroy(&driver_data->buffer_heap);

    /* Clean up left over surfaces */

#if 0
    /* Free PVR2D buffer wrapped from the surfaces */
    psb_free_surface_pvr2dbuf(driver_data);
#endif
    obj_surface = (object_surface_p) object_heap_first(&driver_data->surface_heap, &iter);
    while (obj_surface) {
        drv_debug_msg(VIDEO_DEBUG_INIT, "vaTerminate: surfaceID %08x still allocated, destroying\n", obj_surface->base.id);
        psb__destroy_surface(driver_data, obj_surface);
        obj_surface = (object_surface_p) object_heap_next(&driver_data->surface_heap, &iter);
    }
    object_heap_destroy(&driver_data->surface_heap);

    /* Clean up configIDs */
    obj_config = (object_config_p) object_heap_first(&driver_data->config_heap, &iter);
    while (obj_config) {
        object_heap_free(&driver_data->config_heap, (object_base_p) obj_config);
        obj_config = (object_config_p) object_heap_next(&driver_data->config_heap, &iter);
    }
    object_heap_destroy(&driver_data->config_heap);

    if (driver_data->camera_bo) {
        drv_debug_msg(VIDEO_DEBUG_INIT, "vaTerminate: clearup camera global BO\n");

        psb_buffer_destroy((psb_buffer_p)driver_data->camera_bo);
        free(driver_data->camera_bo);
        driver_data->camera_bo = NULL;
    }

    if (driver_data->rar_bo) {
        drv_debug_msg(VIDEO_DEBUG_INIT, "vaTerminate: clearup RAR global BO\n");

        psb_buffer_destroy((psb_buffer_p)driver_data->rar_bo);
        free(driver_data->rar_bo);
        driver_data->rar_bo = NULL;
    }

    if (driver_data->ws_priv) {
        drv_debug_msg(VIDEO_DEBUG_INIT, "vaTerminate: tear down output portion\n");

        psb_deinitOutput(ctx);
        driver_data->ws_priv = NULL;
    }

    drv_debug_msg(VIDEO_DEBUG_INIT, "vaTerminate: de-initialized DRM\n");

    psb__deinitDRM(ctx);

    if (driver_data->msvdx_decode_status)
        free(driver_data->msvdx_decode_status);

    if (driver_data->surface_mb_error)
        free(driver_data->surface_mb_error);

    pthread_mutex_destroy(&driver_data->drm_mutex);
    free(ctx->pDriverData);
    free(ctx->vtable_egl);
    free(ctx->vtable_tpi);

    ctx->pDriverData = NULL;
    drv_debug_msg(VIDEO_DEBUG_INIT, "vaTerminate: goodbye\n\n");

    psb__close_log();
    DEBUG_FUNC_EXIT
    return VA_STATUS_SUCCESS;
}

EXPORT VAStatus __vaDriverInit_0_31(VADriverContextP ctx)
{
    psb_driver_data_p driver_data;
    struct VADriverVTableTPI *tpi;
    struct VADriverVTableEGL *va_egl;
    int result;
    if (psb_video_trace_fp) {
        /* make gdb always stop here */
        signal(SIGUSR1, SIG_IGN);
        kill(getpid(), SIGUSR1);
    }

    psb__open_log();

    drv_debug_msg(VIDEO_DEBUG_INIT, "vaInitilize: start the journey\n");

    ctx->version_major = 0;
    ctx->version_minor = 31;

    ctx->max_profiles = PSB_MAX_PROFILES;
    ctx->max_entrypoints = PSB_MAX_ENTRYPOINTS;
    ctx->max_attributes = PSB_MAX_CONFIG_ATTRIBUTES;
    ctx->max_image_formats = PSB_MAX_IMAGE_FORMATS;
    ctx->max_subpic_formats = PSB_MAX_SUBPIC_FORMATS;
    ctx->max_display_attributes = PSB_MAX_DISPLAY_ATTRIBUTES;

    ctx->vtable->vaTerminate = psb_Terminate;
    ctx->vtable->vaQueryConfigEntrypoints = psb_QueryConfigEntrypoints;
    ctx->vtable->vaTerminate = psb_Terminate;
    ctx->vtable->vaQueryConfigProfiles = psb_QueryConfigProfiles;
    ctx->vtable->vaQueryConfigEntrypoints = psb_QueryConfigEntrypoints;
    ctx->vtable->vaQueryConfigAttributes = psb_QueryConfigAttributes;
    ctx->vtable->vaCreateConfig = psb_CreateConfig;
    ctx->vtable->vaDestroyConfig = psb_DestroyConfig;
    ctx->vtable->vaGetConfigAttributes = psb_GetConfigAttributes;
    ctx->vtable->vaCreateSurfaces2 = psb_CreateSurfaces2;
    ctx->vtable->vaCreateSurfaces = psb_CreateSurfaces;
    ctx->vtable->vaGetSurfaceAttributes = psb_GetSurfaceAttributes;
    ctx->vtable->vaDestroySurfaces = psb_DestroySurfaces;
    ctx->vtable->vaCreateContext = psb_CreateContext;
    ctx->vtable->vaDestroyContext = psb_DestroyContext;
    ctx->vtable->vaCreateBuffer = psb_CreateBuffer;
    ctx->vtable->vaBufferSetNumElements = psb_BufferSetNumElements;
    ctx->vtable->vaMapBuffer = psb_MapBuffer;
    ctx->vtable->vaUnmapBuffer = psb_UnmapBuffer;
    ctx->vtable->vaDestroyBuffer = psb_DestroyBuffer;
    ctx->vtable->vaBeginPicture = psb_BeginPicture;
    ctx->vtable->vaRenderPicture = psb_RenderPicture;
    ctx->vtable->vaEndPicture = psb_EndPicture;
    ctx->vtable->vaSyncSurface = psb_SyncSurface;
    ctx->vtable->vaQuerySurfaceStatus = psb_QuerySurfaceStatus;
    ctx->vtable->vaQuerySurfaceError = psb_QuerySurfaceError;
    ctx->vtable->vaPutSurface = psb_PutSurface;
    ctx->vtable->vaQueryImageFormats = psb_QueryImageFormats;
    ctx->vtable->vaCreateImage = psb_CreateImage;
    ctx->vtable->vaDeriveImage = psb_DeriveImage;
    ctx->vtable->vaDestroyImage = psb_DestroyImage;
    ctx->vtable->vaSetImagePalette = psb_SetImagePalette;
    ctx->vtable->vaGetImage = psb_GetImage;
    ctx->vtable->vaPutImage = psb_PutImage;
    ctx->vtable->vaQuerySubpictureFormats = psb_QuerySubpictureFormats;
    ctx->vtable->vaCreateSubpicture = psb_CreateSubpicture;
    ctx->vtable->vaDestroySubpicture = psb_DestroySubpicture;
    ctx->vtable->vaSetSubpictureImage = psb_SetSubpictureImage;
    ctx->vtable->vaSetSubpictureChromakey = psb_SetSubpictureChromakey;
    ctx->vtable->vaSetSubpictureGlobalAlpha = psb_SetSubpictureGlobalAlpha;
    ctx->vtable->vaAssociateSubpicture = psb_AssociateSubpicture;
    ctx->vtable->vaDeassociateSubpicture = psb_DeassociateSubpicture;
    ctx->vtable->vaQueryDisplayAttributes = psb_QueryDisplayAttributes;
    ctx->vtable->vaGetDisplayAttributes = psb_GetDisplayAttributes;
    ctx->vtable->vaSetDisplayAttributes = psb_SetDisplayAttributes;
    ctx->vtable->vaQuerySurfaceAttributes = psb_QuerySurfaceAttributes;
    ctx->vtable->vaBufferInfo = psb_BufferInfo;
    ctx->vtable->vaLockSurface = psb_LockSurface;
    ctx->vtable->vaUnlockSurface = psb_UnlockSurface;
#ifdef PSBVIDEO_MRFL_VPP
    ctx->vtable_vpp->vaQueryVideoProcFilters = vsp_QueryVideoProcFilters;
    ctx->vtable_vpp->vaQueryVideoProcFilterCaps = vsp_QueryVideoProcFilterCaps;
    ctx->vtable_vpp->vaQueryVideoProcPipelineCaps = vsp_QueryVideoProcPipelineCaps;
#endif

#ifdef PSBVIDEO_MFLD
    ctx->vtable_vpp->vaQueryVideoProcFilters = ved_QueryVideoProcFilters;
    ctx->vtable_vpp->vaQueryVideoProcFilterCaps = ved_QueryVideoProcFilterCaps;
    ctx->vtable_vpp->vaQueryVideoProcPipelineCaps = ved_QueryVideoProcPipelineCaps;
#endif

    ctx->vtable_tpi = calloc(1, sizeof(struct VADriverVTableTPI));
    if (NULL == ctx->vtable_tpi)
        return VA_STATUS_ERROR_ALLOCATION_FAILED;

    tpi = (struct VADriverVTableTPI *)ctx->vtable_tpi;
    tpi->vaCreateSurfacesWithAttribute = psb_CreateSurfacesWithAttribute;
    tpi->vaPutSurfaceBuf = psb_PutSurfaceBuf;
        tpi->vaSetTimestampForSurface = psb_SetTimestampForSurface;

    ctx->vtable_egl = calloc(1, sizeof(struct VADriverVTableEGL));
    if (NULL == ctx->vtable_egl)
        return VA_STATUS_ERROR_ALLOCATION_FAILED;

    va_egl = (struct VADriverVTableEGL *)ctx->vtable_egl;
    va_egl->vaGetEGLClientBufferFromSurface = psb_GetEGLClientBufferFromSurface;

    driver_data = (psb_driver_data_p) calloc(1, sizeof(*driver_data));
    ctx->pDriverData = (unsigned char *) driver_data;
    if (NULL == driver_data) {
        if (ctx->vtable_tpi)
            free(ctx->vtable_tpi);
        if (ctx->vtable_egl)
            free(ctx->vtable_egl);
        return VA_STATUS_ERROR_ALLOCATION_FAILED;
    }

    if (VA_STATUS_SUCCESS != psb__initDRM(ctx)) {
        free(ctx->pDriverData);
        ctx->pDriverData = NULL;
        return VA_STATUS_ERROR_UNKNOWN;
    }

    pthread_mutex_init(&driver_data->drm_mutex, NULL);

    /*
     * To read PBO.MSR.CCF Mode and Status Register C-Spec -p112
     */
#define PCI_PORT5_REG80_VIDEO_SD_DISABLE        0x0008
#define PCI_PORT5_REG80_VIDEO_HD_DISABLE        0x0010

#if 0
    struct drm_psb_hw_info hw_info;
    do {
        result = drmCommandRead(driver_data->drm_fd, DRM_PSB_HW_INFO, &hw_info, sizeof(hw_info));
    } while (result == EAGAIN);

    if (result != 0) {
        psb__deinitDRM(ctx);
        free(ctx->pDriverData);
        ctx->pDriverData = NULL;
        return VA_STATUS_ERROR_UNKNOWN;
    }

    driver_data->video_sd_disabled = !!(hw_info.caps & PCI_PORT5_REG80_VIDEO_SD_DISABLE);
    driver_data->video_hd_disabled = !!(hw_info.caps & PCI_PORT5_REG80_VIDEO_HD_DISABLE);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "hw_info: rev_id = %08x capabilities = %08x\n", hw_info.rev_id, hw_info.caps);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "hw_info: video_sd_disable=%d,video_hd_disable=%d\n",
                             driver_data->video_sd_disabled, driver_data->video_hd_disabled);
    if (driver_data->video_sd_disabled != 0) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "MRST: hw_info shows video_sd_disable is true,fix it manually\n");
        driver_data->video_sd_disabled = 0;
    }
    if (driver_data->video_hd_disabled != 0) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "MRST: hw_info shows video_hd_disable is true,fix it manually\n");
        driver_data->video_hd_disabled = 0;
    }
#endif

    if (0 != psb_get_device_info(ctx)) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "ERROR: failed to get video device info\n");
        driver_data->encode_supported = 1;
        driver_data->decode_supported = 1;
        driver_data->hd_encode_supported = 1;
        driver_data->hd_decode_supported = 1;
    }

#if 0
    psb_init_surface_pvr2dbuf(driver_data);
#endif

    if (VA_STATUS_SUCCESS != psb_initOutput(ctx)) {
        pthread_mutex_destroy(&driver_data->drm_mutex);
        psb__deinitDRM(ctx);
        free(ctx->pDriverData);
        ctx->pDriverData = NULL;
        return VA_STATUS_ERROR_UNKNOWN;
    }

    driver_data->msvdx_context_base = (((unsigned int) getpid()) & 0xffff) << 16;
#ifdef PSBVIDEO_MRFL
    if (IS_MRFL(driver_data)) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "merrifield topazhp encoder\n");
        driver_data->profile2Format[VAProfileH264Baseline][VAEntrypointEncSlice] = &tng_H264ES_vtable;
        driver_data->profile2Format[VAProfileH264Main][VAEntrypointEncSlice] = &tng_H264ES_vtable;
        driver_data->profile2Format[VAProfileH264High][VAEntrypointEncSlice] = &tng_H264ES_vtable;
        driver_data->profile2Format[VAProfileH264StereoHigh][VAEntrypointEncSlice] = &tng_H264ES_vtable;
        driver_data->profile2Format[VAProfileH263Baseline][VAEntrypointEncSlice] = &tng_H263ES_vtable;
        driver_data->profile2Format[VAProfileJPEGBaseline][VAEntrypointEncPicture] = &tng_JPEGES_vtable;
        driver_data->profile2Format[VAProfileMPEG4Simple][VAEntrypointEncSlice] = &tng_MPEG4ES_vtable;
        driver_data->profile2Format[VAProfileMPEG4AdvancedSimple][VAEntrypointEncSlice] = &tng_MPEG4ES_vtable;
        driver_data->profile2Format[VAProfileH264ConstrainedBaseline][VAEntrypointEncSlice] = &tng_H264ES_vtable;
    }
#endif
#ifdef PSBVIDEO_MRFL_VPP
    if (IS_MRFL(driver_data)) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "merrifield vsp vpp\n");
        driver_data->vpp_profile = &vsp_VPP_vtable;
        driver_data->profile2Format[VAProfileVP8Version0_3][VAEntrypointEncSlice] = &vsp_VP8_vtable;
    }
#endif

#ifdef PSBVIDEO_VXD392
    if (IS_MRFL(driver_data) || IS_BAYTRAIL(driver_data)) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "merrifield VXD392 decoder\n");
        driver_data->profile2Format[VAProfileVP8Version0_3][VAEntrypointVLD] = &tng_VP8_vtable;
        driver_data->profile2Format[VAProfileJPEGBaseline][VAEntrypointVLD] = &tng_JPEG_vtable;

        driver_data->profile2Format[VAProfileMPEG4Simple][VAEntrypointVLD] = &pnw_MPEG4_vtable;
        driver_data->profile2Format[VAProfileMPEG4AdvancedSimple][VAEntrypointVLD] = &pnw_MPEG4_vtable;

        driver_data->profile2Format[VAProfileH263Baseline][VAEntrypointVLD] = &pnw_MPEG4_vtable;

        driver_data->profile2Format[VAProfileH264Baseline][VAEntrypointVLD] = &pnw_H264_vtable;
        driver_data->profile2Format[VAProfileH264Main][VAEntrypointVLD] = &pnw_H264_vtable;
        driver_data->profile2Format[VAProfileH264High][VAEntrypointVLD] = &pnw_H264_vtable;

        driver_data->profile2Format[VAProfileVC1Simple][VAEntrypointVLD] = &pnw_VC1_vtable;
        driver_data->profile2Format[VAProfileVC1Main][VAEntrypointVLD] = &pnw_VC1_vtable;
        driver_data->profile2Format[VAProfileVC1Advanced][VAEntrypointVLD] = &pnw_VC1_vtable;
        driver_data->profile2Format[VAProfileH264ConstrainedBaseline][VAEntrypointVLD] = &pnw_H264_vtable;
    }
#endif

#ifdef PSBVIDEO_MRFL_VPP
    if (IS_MRFL(driver_data)) {
        if (*((unsigned int *)ctx->native_dpy) == 0x56454450 /* VEDP */) {

            drv_debug_msg(VIDEO_DEBUG_GENERAL, "merrifield ved vpp\n");
            driver_data->vpp_profile = &tng_yuv_processor_vtable;
            ctx->vtable_vpp->vaQueryVideoProcFilters = ved_QueryVideoProcFilters;
            ctx->vtable_vpp->vaQueryVideoProcFilterCaps = ved_QueryVideoProcFilterCaps;
            ctx->vtable_vpp->vaQueryVideoProcPipelineCaps = ved_QueryVideoProcPipelineCaps;
            driver_data->ved_vpp = 1;
        }
    }
#endif

#ifdef PSBVIDEO_MFLD
    if (IS_MFLD(driver_data)) {
        driver_data->profile2Format[VAProfileH263Baseline][VAEntrypointEncSlice] = &pnw_H263ES_vtable;
        driver_data->profile2Format[VAProfileH264Baseline][VAEntrypointEncSlice] = &pnw_H264ES_vtable;
        driver_data->profile2Format[VAProfileH264Main][VAEntrypointEncSlice] = &pnw_H264ES_vtable;
        driver_data->profile2Format[VAProfileMPEG4Simple][VAEntrypointEncSlice] = &pnw_MPEG4ES_vtable;
        driver_data->profile2Format[VAProfileMPEG4AdvancedSimple][VAEntrypointEncSlice] = &pnw_MPEG4ES_vtable;
        driver_data->profile2Format[VAProfileJPEGBaseline][VAEntrypointEncPicture] = &pnw_JPEG_vtable;

        driver_data->profile2Format[VAProfileMPEG2Main][VAEntrypointVLD] = &pnw_MPEG2_vtable;

        driver_data->profile2Format[VAProfileMPEG4Simple][VAEntrypointVLD] = &pnw_MPEG4_vtable;
        driver_data->profile2Format[VAProfileMPEG4AdvancedSimple][VAEntrypointVLD] = &pnw_MPEG4_vtable;

        driver_data->profile2Format[VAProfileH263Baseline][VAEntrypointVLD] = &pnw_MPEG4_vtable;

        driver_data->profile2Format[VAProfileH264Baseline][VAEntrypointVLD] = &pnw_H264_vtable;
        driver_data->profile2Format[VAProfileH264Main][VAEntrypointVLD] = &pnw_H264_vtable;
        driver_data->profile2Format[VAProfileH264High][VAEntrypointVLD] = &pnw_H264_vtable;

        driver_data->profile2Format[VAProfileVC1Simple][VAEntrypointVLD] = &pnw_VC1_vtable;
        driver_data->profile2Format[VAProfileVC1Main][VAEntrypointVLD] = &pnw_VC1_vtable;
        driver_data->profile2Format[VAProfileVC1Advanced][VAEntrypointVLD] = &pnw_VC1_vtable;
        driver_data->profile2Format[VAProfileH264ConstrainedBaseline][VAEntrypointVLD] = &pnw_H264_vtable;

        driver_data->vpp_profile = &tng_yuv_processor_vtable;
        driver_data->ved_vpp = 1;
    }
#endif

    result = object_heap_init(&driver_data->config_heap, sizeof(struct object_config_s), CONFIG_ID_OFFSET);
    ASSERT(result == 0);

    result = object_heap_init(&driver_data->context_heap, sizeof(struct object_context_s), CONTEXT_ID_OFFSET);
    ASSERT(result == 0);

    result = object_heap_init(&driver_data->surface_heap, sizeof(struct object_surface_s), SURFACE_ID_OFFSET);
    ASSERT(result == 0);

    result = object_heap_init(&driver_data->buffer_heap, sizeof(struct object_buffer_s), BUFFER_ID_OFFSET);
    ASSERT(result == 0);

    result = object_heap_init(&driver_data->image_heap, sizeof(struct object_image_s), IMAGE_ID_OFFSET);
    ASSERT(result == 0);

    result = object_heap_init(&driver_data->subpic_heap, sizeof(struct object_subpic_s), SUBPIC_ID_OFFSET);
    ASSERT(result == 0);

    driver_data->cur_displaying_surface = VA_INVALID_SURFACE;
    driver_data->last_displaying_surface = VA_INVALID_SURFACE;

    driver_data->clear_color = 0;
    driver_data->blend_color = 0;
    driver_data->blend_mode = 0;
    driver_data->overlay_auto_paint_color_key = 0;

    if (IS_BAYTRAIL(driver_data))
        ctx->str_vendor = PSB_STR_VENDOR_BAYTRAIL;
    if (IS_MRFL(driver_data))
        ctx->str_vendor = PSB_STR_VENDOR_MRFL;
    else if (IS_MFLD(driver_data))
    {
        if (IS_LEXINGTON(driver_data))
            ctx->str_vendor = PSB_STR_VENDOR_LEXINGTON;
        else
            ctx->str_vendor = PSB_STR_VENDOR_MFLD;
    }
    else
        ctx->str_vendor = PSB_STR_VENDOR_MRST;

    driver_data->msvdx_decode_status = calloc(1, sizeof(drm_psb_msvdx_decode_status_t));
    if (NULL == driver_data->msvdx_decode_status) {
        pthread_mutex_destroy(&driver_data->drm_mutex);
        return VA_STATUS_ERROR_ALLOCATION_FAILED;
    }
    driver_data->surface_mb_error = calloc(MAX_MB_ERRORS, sizeof(VASurfaceDecodeMBErrors));
    if (NULL == driver_data->surface_mb_error) {
        pthread_mutex_destroy(&driver_data->drm_mutex);
        return VA_STATUS_ERROR_ALLOCATION_FAILED;
    }

    drv_debug_msg(VIDEO_DEBUG_INIT, "vaInitilize: succeeded!\n\n");

#ifdef ANDROID
#ifndef PSBVIDEO_VXD392
    gralloc_init();
#endif
#endif

    return VA_STATUS_SUCCESS;
}


EXPORT VAStatus __vaDriverInit_0_32(VADriverContextP ctx)
{
    return __vaDriverInit_0_31(ctx);
}



static int psb_get_device_info(VADriverContextP ctx)
{
    INIT_DRIVER_DATA;
    struct drm_lnc_video_getparam_arg arg;
    unsigned long device_info;
    int ret = 0;
    unsigned long video_capability;
    unsigned long pci_device;

    driver_data->dev_id = 0x4100; /* by default MRST */

    arg.key = LNC_VIDEO_DEVICE_INFO;
    arg.value = (uint64_t)((unsigned long) & device_info);
    ret = drmCommandWriteRead(driver_data->drm_fd, driver_data->getParamIoctlOffset,
                              &arg, sizeof(arg));
    if (ret != 0) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "failed to get video device info\n");
        return ret;
    }

    pci_device = (device_info >> 16) & 0xffff;
    video_capability = device_info & 0xffff;

    driver_data->dev_id = pci_device;
    drv_debug_msg(VIDEO_DEBUG_INIT, "Retrieve Device ID 0x%08x\n", driver_data->dev_id);

    if (IS_MFLD(driver_data) || IS_MRFL(driver_data))
        driver_data->encode_supported = 1;
    else /* 0x4101 or other device hasn't encode support */
        driver_data->encode_supported = 0;

    driver_data->decode_supported = 1;
    driver_data->hd_decode_supported = 1;
    driver_data->hd_encode_supported = 1;

    return ret;
}
