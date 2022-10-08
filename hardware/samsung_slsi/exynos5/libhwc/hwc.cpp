/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <s3c-fb.h>

#include <EGL/egl.h>

#define HWC_REMOVE_DEPRECATED_VERSIONS 1

#include <cutils/compiler.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <hardware/gralloc.h>
#include <hardware/hardware.h>
#include <hardware/hwcomposer.h>
#include <hardware_legacy/uevent.h>
#include <utils/String8.h>
#include <utils/Vector.h>

#include <sync/sync.h>

#include "ion.h"
#include "gralloc_priv.h"
#include "exynos_gscaler.h"
#include "exynos_format.h"
#include "exynos_v4l2.h"
#include "s5p_tvout_v4l2.h"

const size_t NUM_HW_WINDOWS = 5;
const size_t NO_FB_NEEDED = NUM_HW_WINDOWS + 1;
const size_t MAX_PIXELS = 2560 * 1600 * 2;
const size_t GSC_W_ALIGNMENT = 16;
const size_t GSC_H_ALIGNMENT = 16;
const size_t GSC_DST_CROP_W_ALIGNMENT_RGB888 = 32;
const size_t GSC_DST_W_ALIGNMENT_RGB888 = 32;
const size_t GSC_DST_H_ALIGNMENT_RGB888 = 1;
const size_t FIMD_GSC_IDX = 0;
const size_t HDMI_GSC_IDX = 1;
const int AVAILABLE_GSC_UNITS[] = { 0, 3 };
const size_t NUM_GSC_UNITS = sizeof(AVAILABLE_GSC_UNITS) /
        sizeof(AVAILABLE_GSC_UNITS[0]);
const size_t BURSTLEN_BYTES = 16 * 8;
const size_t NUM_HDMI_BUFFERS = 3;

struct exynos5_hwc_composer_device_1_t;

struct exynos5_gsc_map_t {
    enum {
        GSC_NONE = 0,
        GSC_M2M,
        // TODO: GSC_LOCAL_PATH
    } mode;
    int idx;
};

struct exynos5_hwc_post_data_t {
    int                 overlay_map[NUM_HW_WINDOWS];
    exynos5_gsc_map_t   gsc_map[NUM_HW_WINDOWS];
    size_t              fb_window;
};

const size_t NUM_GSC_DST_BUFS = 3;
struct exynos5_gsc_data_t {
    void            *gsc;
    exynos_gsc_img  src_cfg;
    exynos_gsc_img  dst_cfg;
    buffer_handle_t dst_buf[NUM_GSC_DST_BUFS];
    int             dst_buf_fence[NUM_GSC_DST_BUFS];
    size_t          current_buf;
};

struct hdmi_layer_t {
    int     id;
    int     fd;
    bool    enabled;
    exynos_gsc_img  cfg;

    bool    streaming;
    size_t  current_buf;
    size_t  queued_buf;
};

struct exynos5_hwc_composer_device_1_t {
    hwc_composer_device_1_t base;

    int                     fd;
    int                     vsync_fd;
    exynos5_hwc_post_data_t bufs;

    const private_module_t  *gralloc_module;
    alloc_device_t          *alloc_device;
    const hwc_procs_t       *procs;
    pthread_t               vsync_thread;
    int                     force_gpu;

    int32_t                 xres;
    int32_t                 yres;
    int32_t                 xdpi;
    int32_t                 ydpi;
    int32_t                 vsync_period;

    int  hdmi_mixer0;
    bool hdmi_hpd;
    bool hdmi_enabled;
    bool hdmi_blanked;
    bool hdmi_fb_needed;
    int  hdmi_w;
    int  hdmi_h;

    hdmi_layer_t            hdmi_layers[2];

    exynos5_gsc_data_t      gsc[NUM_GSC_UNITS];

    struct s3c_fb_win_config last_config[NUM_HW_WINDOWS];
    size_t                  last_fb_window;
    const void              *last_handles[NUM_HW_WINDOWS];
    exynos5_gsc_map_t       last_gsc_map[NUM_HW_WINDOWS];
};

static void exynos5_cleanup_gsc_m2m(exynos5_hwc_composer_device_1_t *pdev,
        size_t gsc_idx);

static void dump_handle(private_handle_t *h)
{
    ALOGV("\t\tformat = %d, width = %u, height = %u, stride = %u, vstride = %u",
            h->format, h->width, h->height, h->stride, h->vstride);
}

static void dump_layer(hwc_layer_1_t const *l)
{
    ALOGV("\ttype=%d, flags=%08x, handle=%p, tr=%02x, blend=%04x, "
            "{%d,%d,%d,%d}, {%d,%d,%d,%d}",
            l->compositionType, l->flags, l->handle, l->transform,
            l->blending,
            l->sourceCrop.left,
            l->sourceCrop.top,
            l->sourceCrop.right,
            l->sourceCrop.bottom,
            l->displayFrame.left,
            l->displayFrame.top,
            l->displayFrame.right,
            l->displayFrame.bottom);

    if(l->handle && !(l->flags & HWC_SKIP_LAYER))
        dump_handle(private_handle_t::dynamicCast(l->handle));
}

static void dump_config(s3c_fb_win_config &c)
{
    ALOGV("\tstate = %u", c.state);
    if (c.state == c.S3C_FB_WIN_STATE_BUFFER) {
        ALOGV("\t\tfd = %d, offset = %u, stride = %u, "
                "x = %d, y = %d, w = %u, h = %u, "
                "format = %u, blending = %u",
                c.fd, c.offset, c.stride,
                c.x, c.y, c.w, c.h,
                c.format, c.blending);
    }
    else if (c.state == c.S3C_FB_WIN_STATE_COLOR) {
        ALOGV("\t\tcolor = %u", c.color);
    }
}

static void dump_gsc_img(exynos_gsc_img &c)
{
    ALOGV("\tx = %u, y = %u, w = %u, h = %u, fw = %u, fh = %u",
            c.x, c.y, c.w, c.h, c.fw, c.fh);
    ALOGV("\taddr = {%u, %u, %u}, rot = %u, cacheable = %u, drmMode = %u",
            c.yaddr, c.uaddr, c.vaddr, c.rot, c.cacheable, c.drmMode);
}

inline int WIDTH(const hwc_rect &rect) { return rect.right - rect.left; }
inline int HEIGHT(const hwc_rect &rect) { return rect.bottom - rect.top; }
template<typename T> inline T max(T a, T b) { return (a > b) ? a : b; }
template<typename T> inline T min(T a, T b) { return (a < b) ? a : b; }

static int dup_or_warn(int fence)
{
    int dup_fd = dup(fence);
    if (dup_fd < 0)
        ALOGW("fence dup failed: %s", strerror(errno));
    return dup_fd;
}

static int merge_or_warn(const char *name, int f1, int f2)
{
    int merge_fd = sync_merge(name, f1, f2);
    if (merge_fd < 0)
        ALOGW("fence merge failed: %s", strerror(errno));
    return merge_fd;
}

template<typename T> void align_crop_and_center(T &w, T &h,
        hwc_rect_t *crop, size_t alignment)
{
    double aspect = 1.0 * h / w;
    T w_orig = w, h_orig = h;

    w = ALIGN(w, alignment);
    h = round(aspect * w);
    if (crop) {
        crop->left = (w - w_orig) / 2;
        crop->top = (h - h_orig) / 2;
        crop->right = crop->left + w_orig;
        crop->bottom = crop->top + h_orig;
    }
}

static bool is_transformed(const hwc_layer_1_t &layer)
{
    return layer.transform != 0;
}

static bool is_rotated(const hwc_layer_1_t &layer)
{
    return (layer.transform & HAL_TRANSFORM_ROT_90) ||
            (layer.transform & HAL_TRANSFORM_ROT_180);
}

static bool is_scaled(const hwc_layer_1_t &layer)
{
    return WIDTH(layer.displayFrame) != WIDTH(layer.sourceCrop) ||
            HEIGHT(layer.displayFrame) != HEIGHT(layer.sourceCrop);
}

static inline bool gsc_dst_cfg_changed(exynos_gsc_img &c1, exynos_gsc_img &c2)
{
    return c1.x != c2.x ||
            c1.y != c2.y ||
            c1.w != c2.w ||
            c1.h != c2.h ||
            c1.format != c2.format ||
            c1.rot != c2.rot ||
            c1.cacheable != c2.cacheable ||
            c1.drmMode != c2.drmMode;
}

static inline bool gsc_src_cfg_changed(exynos_gsc_img &c1, exynos_gsc_img &c2)
{
    return gsc_dst_cfg_changed(c1, c2) ||
            c1.fw != c2.fw ||
            c1.fh != c2.fh;
}

static enum s3c_fb_pixel_format exynos5_format_to_s3c_format(int format)
{
    switch (format) {
    case HAL_PIXEL_FORMAT_RGBA_8888:
        return S3C_FB_PIXEL_FORMAT_RGBA_8888;
    case HAL_PIXEL_FORMAT_RGBX_8888:
        return S3C_FB_PIXEL_FORMAT_RGBX_8888;
    case HAL_PIXEL_FORMAT_RGB_565:
        return S3C_FB_PIXEL_FORMAT_RGB_565;
    case HAL_PIXEL_FORMAT_BGRA_8888:
        return S3C_FB_PIXEL_FORMAT_BGRA_8888;
    default:
        return S3C_FB_PIXEL_FORMAT_MAX;
    }
}

static bool exynos5_format_is_supported(int format)
{
    return exynos5_format_to_s3c_format(format) < S3C_FB_PIXEL_FORMAT_MAX;
}

static bool exynos5_format_is_rgb(int format)
{
    switch (format) {
    case HAL_PIXEL_FORMAT_RGBA_8888:
    case HAL_PIXEL_FORMAT_RGBX_8888:
    case HAL_PIXEL_FORMAT_RGB_888:
    case HAL_PIXEL_FORMAT_RGB_565:
    case HAL_PIXEL_FORMAT_BGRA_8888:
        return true;

    default:
        return false;
    }
}

static bool exynos5_format_is_supported_by_gscaler(int format)
{
    switch (format) {
    case HAL_PIXEL_FORMAT_RGBX_8888:
    case HAL_PIXEL_FORMAT_RGB_565:
    case HAL_PIXEL_FORMAT_EXYNOS_YV12:
    case HAL_PIXEL_FORMAT_YCbCr_420_SP:
    case HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED:
        return true;

    default:
        return false;
    }
}

static bool exynos5_format_is_ycrcb(int format)
{
    return format == HAL_PIXEL_FORMAT_EXYNOS_YV12;
}

static bool exynos5_format_requires_gscaler(int format)
{
    return (exynos5_format_is_supported_by_gscaler(format) &&
           (format != HAL_PIXEL_FORMAT_RGBX_8888) && (format != HAL_PIXEL_FORMAT_RGB_565));
}

static uint8_t exynos5_format_to_bpp(int format)
{
    switch (format) {
    case HAL_PIXEL_FORMAT_RGBA_8888:
    case HAL_PIXEL_FORMAT_RGBX_8888:
    case HAL_PIXEL_FORMAT_BGRA_8888:
        return 32;

    case HAL_PIXEL_FORMAT_RGB_565:
        return 16;

    default:
        ALOGW("unrecognized pixel format %u", format);
        return 0;
    }
}

static bool is_x_aligned(const hwc_layer_1_t &layer, int format)
{
    if (!exynos5_format_is_supported(format))
        return true;

    uint8_t bpp = exynos5_format_to_bpp(format);
    uint8_t pixel_alignment = 32 / bpp;

    return (layer.displayFrame.left % pixel_alignment) == 0 &&
            (layer.displayFrame.right % pixel_alignment) == 0;
}

static bool dst_crop_w_aligned(int dest_w)
{
    int dst_crop_w_alignement;

   /* GSC's dst crop size should be aligned 128Bytes */
    dst_crop_w_alignement = GSC_DST_CROP_W_ALIGNMENT_RGB888;

    return (dest_w % dst_crop_w_alignement) == 0;
}

static bool exynos5_supports_gscaler(hwc_layer_1_t &layer, int format,
        bool local_path)
{
    private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);

    int max_w = is_rotated(layer) ? 2048 : 4800;
    int max_h = is_rotated(layer) ? 2048 : 3344;

    bool rot90or270 = !!(layer.transform & HAL_TRANSFORM_ROT_90);
    // n.b.: HAL_TRANSFORM_ROT_270 = HAL_TRANSFORM_ROT_90 |
    //                               HAL_TRANSFORM_ROT_180

    int src_w = WIDTH(layer.sourceCrop), src_h = HEIGHT(layer.sourceCrop);
    int dest_w, dest_h;
    if (rot90or270) {
        dest_w = HEIGHT(layer.displayFrame);
        dest_h = WIDTH(layer.displayFrame);
    } else {
        dest_w = WIDTH(layer.displayFrame);
        dest_h = HEIGHT(layer.displayFrame);
    }

    if (handle->flags & GRALLOC_USAGE_PROTECTED)
        align_crop_and_center(dest_w, dest_h, NULL,
                GSC_DST_CROP_W_ALIGNMENT_RGB888);

    int max_downscale = local_path ? 4 : 16;
    const int max_upscale = 8;

    return exynos5_format_is_supported_by_gscaler(format) &&
            dst_crop_w_aligned(dest_w) &&
            handle->stride <= max_w &&
            handle->stride % GSC_W_ALIGNMENT == 0 &&
            src_w < dest_w * max_downscale &&
            dest_w <= src_w * max_upscale &&
            handle->vstride <= max_h &&
            handle->vstride % GSC_H_ALIGNMENT == 0 &&
            src_h < dest_h * max_downscale &&
            dest_h <= src_h * max_upscale &&
            // per 46.2
            (!rot90or270 || layer.sourceCrop.top % 2 == 0) &&
            (!rot90or270 || layer.sourceCrop.left % 2 == 0);
            // per 46.3.1.6
}

static bool exynos5_requires_gscaler(hwc_layer_1_t &layer, int format)
{
    return exynos5_format_requires_gscaler(format) || is_scaled(layer)
            || is_transformed(layer) || !is_x_aligned(layer, format);
}

int hdmi_get_config(struct exynos5_hwc_composer_device_1_t *dev)
{
    struct v4l2_dv_preset preset;
    struct v4l2_dv_enum_preset enum_preset;
    int index = 0;
    bool found = false;
    int ret;

    if (ioctl(dev->hdmi_layers[0].fd, VIDIOC_G_DV_PRESET, &preset) < 0) {
        ALOGE("%s: g_dv_preset error, %d", __func__, errno);
        return -1;
    }

    while (true) {
        enum_preset.index = index++;
        ret = ioctl(dev->hdmi_layers[0].fd, VIDIOC_ENUM_DV_PRESETS, &enum_preset);

        if (ret < 0) {
            if (errno == EINVAL)
                break;
            ALOGE("%s: enum_dv_presets error, %d", __func__, errno);
            return -1;
        }

        ALOGV("%s: %d preset=%02d width=%d height=%d name=%s",
                __func__, enum_preset.index, enum_preset.preset,
                enum_preset.width, enum_preset.height, enum_preset.name);

        if (preset.preset == enum_preset.preset) {
            dev->hdmi_w  = enum_preset.width;
            dev->hdmi_h  = enum_preset.height;
            found = true;
        }
    }

    return found ? 0 : -1;
}

static enum s3c_fb_blending exynos5_blending_to_s3c_blending(int32_t blending)
{
    switch (blending) {
    case HWC_BLENDING_NONE:
        return S3C_FB_BLENDING_NONE;
    case HWC_BLENDING_PREMULT:
        return S3C_FB_BLENDING_PREMULT;
    case HWC_BLENDING_COVERAGE:
        return S3C_FB_BLENDING_COVERAGE;

    default:
        return S3C_FB_BLENDING_MAX;
    }
}

static bool exynos5_blending_is_supported(int32_t blending)
{
    return exynos5_blending_to_s3c_blending(blending) < S3C_FB_BLENDING_MAX;
}


static int hdmi_enable_layer(struct exynos5_hwc_composer_device_1_t *dev,
                             hdmi_layer_t &hl)
{
    if (hl.enabled)
        return 0;

    struct v4l2_requestbuffers reqbuf;
    memset(&reqbuf, 0, sizeof(reqbuf));
    reqbuf.count  = NUM_HDMI_BUFFERS;
    reqbuf.type   = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    reqbuf.memory = V4L2_MEMORY_DMABUF;
    if (exynos_v4l2_reqbufs(hl.fd, &reqbuf) < 0) {
        ALOGE("%s: layer%d: reqbufs failed %d", __func__, hl.id, errno);
        return -1;
    }

    if (reqbuf.count != NUM_HDMI_BUFFERS) {
        ALOGE("%s: layer%d: didn't get buffer", __func__, hl.id);
        return -1;
    }

    if (hl.id == 1) {
        if (exynos_v4l2_s_ctrl(hl.fd, V4L2_CID_TV_PIXEL_BLEND_ENABLE, 1) < 0) {
            ALOGE("%s: layer%d: PIXEL_BLEND_ENABLE failed %d", __func__,
                                                                hl.id, errno);
            return -1;
        }
    }

    ALOGV("%s: layer%d enabled", __func__, hl.id);
    hl.enabled = true;
    return 0;
}

static void hdmi_disable_layer(struct exynos5_hwc_composer_device_1_t *dev,
                               hdmi_layer_t &hl)
{
    if (!hl.enabled)
        return;

    if (hl.streaming) {
        if (exynos_v4l2_streamoff(hl.fd, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) < 0)
            ALOGE("%s: layer%d: streamoff failed %d", __func__, hl.id, errno);
        hl.streaming = false;
    }

    struct v4l2_requestbuffers reqbuf;
    memset(&reqbuf, 0, sizeof(reqbuf));
    reqbuf.type   = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    reqbuf.memory = V4L2_MEMORY_DMABUF;
    if (exynos_v4l2_reqbufs(hl.fd, &reqbuf) < 0)
        ALOGE("%s: layer%d: reqbufs failed %d", __func__, hl.id, errno);

    memset(&hl.cfg, 0, sizeof(hl.cfg));
    hl.current_buf = 0;
    hl.queued_buf = 0;
    hl.enabled = false;

    ALOGV("%s: layer%d disabled", __func__, hl.id);
}

static void hdmi_hide_layer(struct exynos5_hwc_composer_device_1_t *dev,
                            hdmi_layer_t &hl)
{
    if (exynos_v4l2_s_ctrl(hl.fd, V4L2_CID_TV_LAYER_PRIO, 0) < 0)
        ALOGE("%s: layer%d: LAYER_PRIO failed %d", __func__,
                                                   hl.id, errno);
}

static void hdmi_show_layer(struct exynos5_hwc_composer_device_1_t *dev,
                            hdmi_layer_t &hl)
{
    int prio = hl.id ? 3 : 2;

    if (exynos_v4l2_s_ctrl(hl.fd, V4L2_CID_TV_LAYER_PRIO, prio) < 0)
        ALOGE("%s: layer%d: LAYER_PRIO failed %d", __func__,
                                                   hl.id, errno);
}

static int hdmi_enable(struct exynos5_hwc_composer_device_1_t *dev)
{
    /* hdmi not supported */
    if (dev->hdmi_mixer0 < 0)
        return 0;

    if (dev->hdmi_enabled)
        return 0;

    if (dev->hdmi_blanked)
        return 0;

    struct v4l2_subdev_format sd_fmt;
    memset(&sd_fmt, 0, sizeof(sd_fmt));
    sd_fmt.pad   = MIXER_G0_SUBDEV_PAD_SINK;
    sd_fmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;
    sd_fmt.format.width  = dev->hdmi_w;
    sd_fmt.format.height = dev->hdmi_h;
    sd_fmt.format.code   = V4L2_MBUS_FMT_XRGB8888_4X8_LE;
    if (exynos_subdev_s_fmt(dev->hdmi_mixer0, &sd_fmt) < 0) {
        ALOGE("%s: s_fmt failed pad=%d", __func__, sd_fmt.pad);
        return -1;
    }

    struct v4l2_subdev_crop sd_crop;
    memset(&sd_crop, 0, sizeof(sd_crop));
    sd_crop.pad   = MIXER_G0_SUBDEV_PAD_SINK;
    sd_crop.which = V4L2_SUBDEV_FORMAT_ACTIVE;
    sd_crop.rect.width  = dev->hdmi_w;
    sd_crop.rect.height = dev->hdmi_h;
    if (exynos_subdev_s_crop(dev->hdmi_mixer0, &sd_crop) < 0) {
        ALOGE("%s: s_crop failed pad=%d", __func__, sd_crop.pad);
        return -1;
    }

    memset(&sd_fmt, 0, sizeof(sd_fmt));
    sd_fmt.pad   = MIXER_G0_SUBDEV_PAD_SOURCE;
    sd_fmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;
    sd_fmt.format.width  = dev->hdmi_w;
    sd_fmt.format.height = dev->hdmi_h;
    sd_fmt.format.code   = V4L2_MBUS_FMT_XRGB8888_4X8_LE;
    if (exynos_subdev_s_fmt(dev->hdmi_mixer0, &sd_fmt) < 0) {
        ALOGE("%s: s_fmt failed pad=%d", __func__, sd_fmt.pad);
        return -1;
    }

    memset(&sd_crop, 0, sizeof(sd_crop));
    sd_crop.pad   = MIXER_G0_SUBDEV_PAD_SOURCE;
    sd_crop.which = V4L2_SUBDEV_FORMAT_ACTIVE;
    sd_crop.rect.width  = dev->hdmi_w;
    sd_crop.rect.height = dev->hdmi_h;
    if (exynos_subdev_s_crop(dev->hdmi_mixer0, &sd_crop) < 0) {
        ALOGE("%s: s_crop failed pad=%d", __func__, sd_crop.pad);
        return -1;
    }

    char value[PROPERTY_VALUE_MAX];
    property_get("persist.hdmi.hdcp_enabled", value, "1");
    int hdcp_enabled = atoi(value);

    if (exynos_v4l2_s_ctrl(dev->hdmi_layers[1].fd, V4L2_CID_TV_HDCP_ENABLE,
                           hdcp_enabled) < 0)
        ALOGE("%s: s_ctrl(CID_TV_HDCP_ENABLE) failed %d", __func__, errno);

    /* "3" is RGB709_16_235 */
    property_get("persist.hdmi.color_range", value, "3");
    int color_range = atoi(value);

    if (exynos_v4l2_s_ctrl(dev->hdmi_layers[1].fd, V4L2_CID_TV_SET_COLOR_RANGE,
                           color_range) < 0)
        ALOGE("%s: s_ctrl(CID_TV_COLOR_RANGE) failed %d", __func__, errno);

    hdmi_enable_layer(dev, dev->hdmi_layers[1]);

    dev->hdmi_enabled = true;
    return 0;
}

static void hdmi_disable(struct exynos5_hwc_composer_device_1_t *dev)
{
    if (!dev->hdmi_enabled)
        return;

    hdmi_disable_layer(dev, dev->hdmi_layers[0]);
    hdmi_disable_layer(dev, dev->hdmi_layers[1]);

    exynos5_cleanup_gsc_m2m(dev, HDMI_GSC_IDX);
    dev->hdmi_enabled = false;
}

static int hdmi_output(struct exynos5_hwc_composer_device_1_t *dev,
                       hdmi_layer_t &hl,
                       hwc_layer_1_t &layer,
                       private_handle_t *h,
                       int acquireFenceFd,
                       int *releaseFenceFd)
{
    int ret = 0;

    exynos_gsc_img cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.x = layer.displayFrame.left;
    cfg.y = layer.displayFrame.top;
    cfg.w = WIDTH(layer.displayFrame);
    cfg.h = HEIGHT(layer.displayFrame);

    if (gsc_src_cfg_changed(hl.cfg, cfg)) {
        hdmi_disable_layer(dev, hl);

        struct v4l2_format fmt;
        memset(&fmt, 0, sizeof(fmt));
        fmt.type  = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        fmt.fmt.pix_mp.width       = h->stride;
        fmt.fmt.pix_mp.height      = cfg.h;
        fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_BGR32;
        fmt.fmt.pix_mp.field       = V4L2_FIELD_ANY;
        fmt.fmt.pix_mp.num_planes  = 1;
        ret = exynos_v4l2_s_fmt(hl.fd, &fmt);
        if (ret < 0) {
            ALOGE("%s: layer%d: s_fmt failed %d", __func__, hl.id, errno);
            goto err;
        }

        struct v4l2_subdev_crop sd_crop;
        memset(&sd_crop, 0, sizeof(sd_crop));
        if (hl.id == 0)
            sd_crop.pad   = MIXER_G0_SUBDEV_PAD_SOURCE;
        else
            sd_crop.pad   = MIXER_G1_SUBDEV_PAD_SOURCE;
        sd_crop.which = V4L2_SUBDEV_FORMAT_ACTIVE;
        sd_crop.rect.left   = cfg.x;
        sd_crop.rect.top    = cfg.y;
        sd_crop.rect.width  = cfg.w;
        sd_crop.rect.height = cfg.h;
        if (exynos_subdev_s_crop(dev->hdmi_mixer0, &sd_crop) < 0) {
            ALOGE("%s: s_crop failed pad=%d", __func__, sd_crop.pad);
            goto err;
        }

        hdmi_enable_layer(dev, hl);

        ALOGV("HDMI layer%d configuration:", hl.id);
        dump_gsc_img(cfg);
        hl.cfg = cfg;
    }

    struct v4l2_buffer buffer;
    struct v4l2_plane planes[1];

    if (hl.queued_buf == NUM_HDMI_BUFFERS) {
        memset(&buffer, 0, sizeof(buffer));
        memset(planes, 0, sizeof(planes));
        buffer.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        buffer.memory = V4L2_MEMORY_DMABUF;
        buffer.length = 1;
        buffer.m.planes = planes;
        ret = exynos_v4l2_dqbuf(hl.fd, &buffer);
        if (ret < 0) {
            ALOGE("%s: layer%d: dqbuf failed %d", __func__, hl.id, errno);
            goto err;
        }
        hl.queued_buf--;
    }

    memset(&buffer, 0, sizeof(buffer));
    memset(planes, 0, sizeof(planes));
    buffer.index = hl.current_buf;
    buffer.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    buffer.memory = V4L2_MEMORY_DMABUF;
    buffer.flags = V4L2_BUF_FLAG_USE_SYNC;
    buffer.reserved = acquireFenceFd;
    buffer.length = 1;
    buffer.m.planes = planes;
    buffer.m.planes[0].m.fd = h->fd;
    if (exynos_v4l2_qbuf(hl.fd, &buffer) < 0) {
        ALOGE("%s: layer%d: qbuf failed %d", __func__, hl.id, errno);
        ret = -1;
        goto err;
    }

    if (releaseFenceFd)
        *releaseFenceFd = buffer.reserved;
    else
        close(buffer.reserved);

    hl.queued_buf++;
    hl.current_buf = (hl.current_buf + 1) % NUM_HDMI_BUFFERS;

    if (!hl.streaming) {
        if (exynos_v4l2_streamon(hl.fd, buffer.type) < 0) {
            ALOGE("%s: layer%d: streamon failed %d", __func__, hl.id, errno);
            ret = -1;
            goto err;
        }
        hl.streaming = true;
    }

err:
    if (acquireFenceFd >= 0)
        close(acquireFenceFd);

    return ret;
}

bool exynos5_is_offscreen(hwc_layer_1_t &layer,
        struct exynos5_hwc_composer_device_1_t *pdev)
{
    return layer.displayFrame.left > pdev->xres ||
            layer.displayFrame.right < 0 ||
            layer.displayFrame.top > pdev->yres ||
            layer.displayFrame.bottom < 0;
}

size_t exynos5_visible_width(hwc_layer_1_t &layer, int format,
        struct exynos5_hwc_composer_device_1_t *pdev)
{
    int bpp;
    if (exynos5_requires_gscaler(layer, format))
        bpp = 32;
    else
        bpp = exynos5_format_to_bpp(format);
    int left = max(layer.displayFrame.left, 0);
    int right = min(layer.displayFrame.right, pdev->xres);

    return (right - left) * bpp / 8;
}

bool exynos5_supports_overlay(hwc_layer_1_t &layer, size_t i,
        struct exynos5_hwc_composer_device_1_t *pdev)
{
    if (layer.flags & HWC_SKIP_LAYER) {
        ALOGV("\tlayer %u: skipping", i);
        return false;
    }

    private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);

    if (!handle) {
        ALOGV("\tlayer %u: handle is NULL", i);
        return false;
    }

    if (exynos5_requires_gscaler(layer, handle->format)) {
        if (!exynos5_supports_gscaler(layer, handle->format, false)) {
            ALOGV("\tlayer %u: gscaler required but not supported", i);
            return false;
        }
    } else {
        if (!exynos5_format_is_supported(handle->format)) {
            ALOGV("\tlayer %u: pixel format %u not supported", i, handle->format);
            return false;
        }
    }
    if (exynos5_visible_width(layer, handle->format, pdev) < BURSTLEN_BYTES) {
        ALOGV("\tlayer %u: visible area is too narrow", i);
        return false;
    }
    if (!exynos5_blending_is_supported(layer.blending)) {
        ALOGV("\tlayer %u: blending %d not supported", i, layer.blending);
        return false;
    }
    if (CC_UNLIKELY(exynos5_is_offscreen(layer, pdev))) {
        ALOGW("\tlayer %u: off-screen", i);
        return false;
    }

    return true;
}

inline bool intersect(const hwc_rect &r1, const hwc_rect &r2)
{
    return !(r1.left > r2.right ||
        r1.right < r2.left ||
        r1.top > r2.bottom ||
        r1.bottom < r2.top);
}

inline hwc_rect intersection(const hwc_rect &r1, const hwc_rect &r2)
{
    hwc_rect i;
    i.top = max(r1.top, r2.top);
    i.bottom = min(r1.bottom, r2.bottom);
    i.left = max(r1.left, r2.left);
    i.right = min(r1.right, r2.right);
    return i;
}

static int exynos5_prepare_fimd(exynos5_hwc_composer_device_1_t *pdev,
        hwc_display_contents_1_t* contents)
{
    ALOGV("preparing %u layers for FIMD", contents->numHwLayers);

    memset(pdev->bufs.gsc_map, 0, sizeof(pdev->bufs.gsc_map));

    bool force_fb = pdev->force_gpu;
    for (size_t i = 0; i < NUM_HW_WINDOWS; i++)
        pdev->bufs.overlay_map[i] = -1;

    bool fb_needed = false;
    size_t first_fb = 0, last_fb = 0;

    // find unsupported overlays
    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];

        if (layer.compositionType == HWC_FRAMEBUFFER_TARGET) {
            ALOGV("\tlayer %u: framebuffer target", i);
            continue;
        }

        if (layer.compositionType == HWC_BACKGROUND && !force_fb) {
            ALOGV("\tlayer %u: background supported", i);
            dump_layer(&contents->hwLayers[i]);
            continue;
        }

        if (exynos5_supports_overlay(contents->hwLayers[i], i, pdev) &&
                !force_fb) {
            ALOGV("\tlayer %u: overlay supported", i);
            layer.compositionType = HWC_OVERLAY;
            dump_layer(&contents->hwLayers[i]);
            continue;
        }

        if (!fb_needed) {
            first_fb = i;
            fb_needed = true;
        }
        last_fb = i;
        layer.compositionType = HWC_FRAMEBUFFER;

        dump_layer(&contents->hwLayers[i]);
    }

    // can't composite overlays sandwiched between framebuffers
    if (fb_needed)
        for (size_t i = first_fb; i < last_fb; i++)
            contents->hwLayers[i].compositionType = HWC_FRAMEBUFFER;

    // Incrementally try to add our supported layers to hardware windows.
    // If adding a layer would violate a hardware constraint, force it
    // into the framebuffer and try again.  (Revisiting the entire list is
    // necessary because adding a layer to the framebuffer can cause other
    // windows to retroactively violate constraints.)
    bool changed;
    bool gsc_used;
    do {
        android::Vector<hwc_rect> rects;
        android::Vector<hwc_rect> overlaps;
        size_t pixels_left, windows_left;

        gsc_used = false;

        if (fb_needed) {
            hwc_rect_t fb_rect;
            fb_rect.top = fb_rect.left = 0;
            fb_rect.right = pdev->xres - 1;
            fb_rect.bottom = pdev->yres - 1;
            pixels_left = MAX_PIXELS - pdev->xres * pdev->yres;
            windows_left = NUM_HW_WINDOWS - 1;
            rects.push_back(fb_rect);
        }
        else {
            pixels_left = MAX_PIXELS;
            windows_left = NUM_HW_WINDOWS;
        }

        changed = false;

        for (size_t i = 0; i < contents->numHwLayers; i++) {
            hwc_layer_1_t &layer = contents->hwLayers[i];
            if ((layer.flags & HWC_SKIP_LAYER) ||
                    layer.compositionType == HWC_FRAMEBUFFER_TARGET)
                continue;

            private_handle_t *handle = private_handle_t::dynamicCast(
                    layer.handle);

            // we've already accounted for the framebuffer above
            if (layer.compositionType == HWC_FRAMEBUFFER)
                continue;

            // only layer 0 can be HWC_BACKGROUND, so we can
            // unconditionally allow it without extra checks
            if (layer.compositionType == HWC_BACKGROUND) {
                windows_left--;
                continue;
            }

            size_t pixels_needed = WIDTH(layer.displayFrame) *
                    HEIGHT(layer.displayFrame);
            bool can_compose = windows_left && pixels_needed <= pixels_left;
            bool gsc_required = exynos5_requires_gscaler(layer, handle->format);
            if (gsc_required)
                can_compose = can_compose && !gsc_used;

            // hwc_rect_t right and bottom values are normally exclusive;
            // the intersection logic is simpler if we make them inclusive
            hwc_rect_t visible_rect = layer.displayFrame;
            visible_rect.right--; visible_rect.bottom--;

            // no more than 2 layers can overlap on a given pixel
            for (size_t j = 0; can_compose && j < overlaps.size(); j++) {
                if (intersect(visible_rect, overlaps.itemAt(j)))
                    can_compose = false;
            }

            if (!can_compose) {
                layer.compositionType = HWC_FRAMEBUFFER;
                if (!fb_needed) {
                    first_fb = last_fb = i;
                    fb_needed = true;
                }
                else {
                    first_fb = min(i, first_fb);
                    last_fb = max(i, last_fb);
                }
                changed = true;
                break;
            }

            for (size_t j = 0; j < rects.size(); j++) {
                const hwc_rect_t &other_rect = rects.itemAt(j);
                if (intersect(visible_rect, other_rect))
                    overlaps.push_back(intersection(visible_rect, other_rect));
            }
            rects.push_back(visible_rect);
            pixels_left -= pixels_needed;
            windows_left--;
            if (gsc_required)
                gsc_used = true;
        }

        if (changed)
            for (size_t i = first_fb; i < last_fb; i++)
                contents->hwLayers[i].compositionType = HWC_FRAMEBUFFER;
    } while(changed);

    unsigned int nextWindow = 0;

    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];

        if (fb_needed && i == first_fb) {
            ALOGV("assigning framebuffer to window %u\n",
                    nextWindow);
            nextWindow++;
            continue;
        }

        if (layer.compositionType != HWC_FRAMEBUFFER &&
                layer.compositionType != HWC_FRAMEBUFFER_TARGET) {
            ALOGV("assigning layer %u to window %u", i, nextWindow);
            pdev->bufs.overlay_map[nextWindow] = i;
            if (layer.compositionType == HWC_OVERLAY) {
                private_handle_t *handle =
                        private_handle_t::dynamicCast(layer.handle);
                if (exynos5_requires_gscaler(layer, handle->format)) {
                    ALOGV("\tusing gscaler %u", AVAILABLE_GSC_UNITS[FIMD_GSC_IDX]);
                    pdev->bufs.gsc_map[nextWindow].mode =
                            exynos5_gsc_map_t::GSC_M2M;
                    pdev->bufs.gsc_map[nextWindow].idx = FIMD_GSC_IDX;
                }
            }
            nextWindow++;
        }
    }

    if (!gsc_used)
        exynos5_cleanup_gsc_m2m(pdev, FIMD_GSC_IDX);

    if (fb_needed)
        pdev->bufs.fb_window = first_fb;
    else
        pdev->bufs.fb_window = NO_FB_NEEDED;

    return 0;
}

static int exynos5_prepare_hdmi(exynos5_hwc_composer_device_1_t *pdev,
        hwc_display_contents_1_t* contents)
{
    ALOGV("preparing %u layers for HDMI", contents->numHwLayers);
    hwc_layer_1_t *video_layer = NULL;

    pdev->hdmi_fb_needed = false;

    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];

        if (layer.compositionType == HWC_FRAMEBUFFER_TARGET) {
            ALOGV("\tlayer %u: framebuffer target", i);
            continue;
        }

        if (layer.compositionType == HWC_BACKGROUND) {
            ALOGV("\tlayer %u: background layer", i);
            dump_layer(&layer);
            continue;
        }

        if (layer.handle) {
            private_handle_t *h = private_handle_t::dynamicCast(layer.handle);
            if (h->flags & GRALLOC_USAGE_PROTECTED) {
                if (!video_layer) {
                    video_layer = &layer;
                    layer.compositionType = HWC_OVERLAY;
                    ALOGV("\tlayer %u: video layer", i);
                    dump_layer(&layer);
                    continue;
                }
            }
        }

        pdev->hdmi_fb_needed = true;
        layer.compositionType = HWC_FRAMEBUFFER;
        dump_layer(&layer);
    }

    return 0;
}

static int exynos5_prepare(hwc_composer_device_1_t *dev,
        size_t numDisplays, hwc_display_contents_1_t** displays)
{
    if (!numDisplays || !displays)
        return 0;

    exynos5_hwc_composer_device_1_t *pdev =
            (exynos5_hwc_composer_device_1_t *)dev;
    hwc_display_contents_1_t *fimd_contents = displays[HWC_DISPLAY_PRIMARY];
    hwc_display_contents_1_t *hdmi_contents = displays[HWC_DISPLAY_EXTERNAL];

    if (pdev->hdmi_hpd) {
        hdmi_enable(pdev);
    } else {
        hdmi_disable(pdev);
    }

    if (fimd_contents) {
        int err = exynos5_prepare_fimd(pdev, fimd_contents);
        if (err)
            return err;
    }

    if (hdmi_contents) {
        int err = exynos5_prepare_hdmi(pdev, hdmi_contents);
        if (err)
            return err;
    }

    return 0;
}

static int exynos5_config_gsc_m2m(hwc_layer_1_t &layer,
        alloc_device_t* alloc_device, exynos5_gsc_data_t *gsc_data,
        int gsc_idx, int dst_format, hwc_rect_t *sourceCrop)
{
    ALOGV("configuring gscaler %u for memory-to-memory", AVAILABLE_GSC_UNITS[gsc_idx]);

    private_handle_t *src_handle = private_handle_t::dynamicCast(layer.handle);
    buffer_handle_t dst_buf;
    private_handle_t *dst_handle;
    int ret = 0;

    exynos_gsc_img src_cfg, dst_cfg;
    memset(&src_cfg, 0, sizeof(src_cfg));
    memset(&dst_cfg, 0, sizeof(dst_cfg));

    hwc_rect_t sourceCropTemp;
    if (!sourceCrop)
        sourceCrop = &sourceCropTemp;

    src_cfg.x = layer.sourceCrop.left;
    src_cfg.y = layer.sourceCrop.top;
    src_cfg.w = WIDTH(layer.sourceCrop);
    src_cfg.fw = src_handle->stride;
    src_cfg.h = HEIGHT(layer.sourceCrop);
    src_cfg.fh = src_handle->vstride;
    src_cfg.yaddr = src_handle->fd;
    if (exynos5_format_is_ycrcb(src_handle->format)) {
        src_cfg.uaddr = src_handle->fd2;
        src_cfg.vaddr = src_handle->fd1;
    } else {
        src_cfg.uaddr = src_handle->fd1;
        src_cfg.vaddr = src_handle->fd2;
    }
    src_cfg.format = src_handle->format;
    src_cfg.drmMode = !!(src_handle->flags & GRALLOC_USAGE_PROTECTED);
    src_cfg.acquireFenceFd = layer.acquireFenceFd;
    layer.acquireFenceFd = -1;

    dst_cfg.x = 0;
    dst_cfg.y = 0;
    dst_cfg.w = WIDTH(layer.displayFrame);
    dst_cfg.h = HEIGHT(layer.displayFrame);
    dst_cfg.rot = layer.transform;
    dst_cfg.drmMode = src_cfg.drmMode;
    dst_cfg.format = dst_format;
    dst_cfg.narrowRgb = !exynos5_format_is_rgb(src_handle->format);
    if (dst_cfg.drmMode)
        align_crop_and_center(dst_cfg.w, dst_cfg.h, sourceCrop,
                GSC_DST_CROP_W_ALIGNMENT_RGB888);

    ALOGV("source configuration:");
    dump_gsc_img(src_cfg);

    bool reconfigure = gsc_src_cfg_changed(src_cfg, gsc_data->src_cfg) ||
            gsc_dst_cfg_changed(dst_cfg, gsc_data->dst_cfg);
    if (reconfigure) {
        int dst_stride;
        int usage = GRALLOC_USAGE_SW_READ_NEVER |
                GRALLOC_USAGE_SW_WRITE_NEVER |
                GRALLOC_USAGE_HW_COMPOSER;

        if (src_handle->flags & GRALLOC_USAGE_PROTECTED)
            usage |= GRALLOC_USAGE_PROTECTED;

        int w = ALIGN(dst_cfg.w, GSC_DST_W_ALIGNMENT_RGB888);
        int h = ALIGN(dst_cfg.h, GSC_DST_H_ALIGNMENT_RGB888);

        for (size_t i = 0; i < NUM_GSC_DST_BUFS; i++) {
            if (gsc_data->dst_buf[i]) {
                alloc_device->free(alloc_device, gsc_data->dst_buf[i]);
                gsc_data->dst_buf[i] = NULL;
            }

            if (gsc_data->dst_buf_fence[i] >= 0) {
                close(gsc_data->dst_buf_fence[i]);
                gsc_data->dst_buf_fence[i] = -1;
            }

            int ret = alloc_device->alloc(alloc_device, w, h,
                    HAL_PIXEL_FORMAT_RGBX_8888, usage, &gsc_data->dst_buf[i],
                    &dst_stride);
            if (ret < 0) {
                ALOGE("failed to allocate destination buffer: %s",
                        strerror(-ret));
                goto err_alloc;
            }
        }

        gsc_data->current_buf = 0;
    }

    dst_buf = gsc_data->dst_buf[gsc_data->current_buf];
    dst_handle = private_handle_t::dynamicCast(dst_buf);

    dst_cfg.fw = dst_handle->stride;
    dst_cfg.fh = dst_handle->vstride;
    dst_cfg.yaddr = dst_handle->fd;
    dst_cfg.acquireFenceFd = gsc_data->dst_buf_fence[gsc_data->current_buf];
    gsc_data->dst_buf_fence[gsc_data->current_buf] = -1;

    ALOGV("destination configuration:");
    dump_gsc_img(dst_cfg);

    if ((int)dst_cfg.w != WIDTH(layer.displayFrame))
        ALOGV("padding %u x %u output to %u x %u and cropping to {%u,%u,%u,%u}",
                WIDTH(layer.displayFrame), HEIGHT(layer.displayFrame),
                dst_cfg.w, dst_cfg.h, sourceCrop->left, sourceCrop->top,
                sourceCrop->right, sourceCrop->bottom);

    if (gsc_data->gsc) {
        ALOGV("reusing open gscaler %u", AVAILABLE_GSC_UNITS[gsc_idx]);
    } else {
        ALOGV("opening gscaler %u", AVAILABLE_GSC_UNITS[gsc_idx]);
        gsc_data->gsc = exynos_gsc_create_exclusive(
                AVAILABLE_GSC_UNITS[gsc_idx], GSC_M2M_MODE, GSC_DUMMY, true);
        if (!gsc_data->gsc) {
            ALOGE("failed to create gscaler handle");
            ret = -1;
            goto err_alloc;
        }
    }

    if (reconfigure) {
        ret = exynos_gsc_stop_exclusive(gsc_data->gsc);
        if (ret < 0) {
            ALOGE("failed to stop gscaler %u", gsc_idx);
            goto err_gsc_config;
        }

        ret = exynos_gsc_config_exclusive(gsc_data->gsc, &src_cfg, &dst_cfg);
        if (ret < 0) {
            ALOGE("failed to configure gscaler %u", gsc_idx);
            goto err_gsc_config;
        }
    }

    ret = exynos_gsc_run_exclusive(gsc_data->gsc, &src_cfg, &dst_cfg);
    if (ret < 0) {
        ALOGE("failed to run gscaler %u", gsc_idx);
        goto err_gsc_config;
    }

    gsc_data->src_cfg = src_cfg;
    gsc_data->dst_cfg = dst_cfg;

    layer.releaseFenceFd = src_cfg.releaseFenceFd;

    return 0;

err_gsc_config:
    exynos_gsc_destroy(gsc_data->gsc);
    gsc_data->gsc = NULL;
err_alloc:
    if (src_cfg.acquireFenceFd >= 0)
        close(src_cfg.acquireFenceFd);
    for (size_t i = 0; i < NUM_GSC_DST_BUFS; i++) {
       if (gsc_data->dst_buf[i]) {
           alloc_device->free(alloc_device, gsc_data->dst_buf[i]);
           gsc_data->dst_buf[i] = NULL;
       }
       if (gsc_data->dst_buf_fence[i] >= 0) {
           close(gsc_data->dst_buf_fence[i]);
           gsc_data->dst_buf_fence[i] = -1;
       }
    }
    memset(&gsc_data->src_cfg, 0, sizeof(gsc_data->src_cfg));
    memset(&gsc_data->dst_cfg, 0, sizeof(gsc_data->dst_cfg));
    return ret;
}


static void exynos5_cleanup_gsc_m2m(exynos5_hwc_composer_device_1_t *pdev,
        size_t gsc_idx)
{
    exynos5_gsc_data_t &gsc_data = pdev->gsc[gsc_idx];
    if (!gsc_data.gsc)
        return;

    ALOGV("closing gscaler %u", AVAILABLE_GSC_UNITS[gsc_idx]);

    exynos_gsc_stop_exclusive(gsc_data.gsc);
    exynos_gsc_destroy(gsc_data.gsc);
    for (size_t i = 0; i < NUM_GSC_DST_BUFS; i++) {
        if (gsc_data.dst_buf[i])
            pdev->alloc_device->free(pdev->alloc_device, gsc_data.dst_buf[i]);
        if (gsc_data.dst_buf_fence[i] >= 0)
            close(gsc_data.dst_buf_fence[i]);
    }

    memset(&gsc_data, 0, sizeof(gsc_data));
    for (size_t i = 0; i < NUM_GSC_DST_BUFS; i++)
        gsc_data.dst_buf_fence[i] = -1;
}

static void exynos5_config_handle(private_handle_t *handle,
        hwc_rect_t &sourceCrop, hwc_rect_t &displayFrame,
        int32_t blending, int fence_fd, s3c_fb_win_config &cfg,
        exynos5_hwc_composer_device_1_t *pdev)
{
    uint32_t x, y;
    uint32_t w = WIDTH(displayFrame);
    uint32_t h = HEIGHT(displayFrame);
    uint8_t bpp = exynos5_format_to_bpp(handle->format);
    uint32_t offset = (sourceCrop.top * handle->stride + sourceCrop.left) * bpp / 8;

    if (displayFrame.left < 0) {
        unsigned int crop = -displayFrame.left;
        ALOGV("layer off left side of screen; cropping %u pixels from left edge",
                crop);
        x = 0;
        w -= crop;
        offset += crop * bpp / 8;
    } else {
        x = displayFrame.left;
    }

    if (displayFrame.right > pdev->xres) {
        unsigned int crop = displayFrame.right - pdev->xres;
        ALOGV("layer off right side of screen; cropping %u pixels from right edge",
                crop);
        w -= crop;
    }

    if (displayFrame.top < 0) {
        unsigned int crop = -displayFrame.top;
        ALOGV("layer off top side of screen; cropping %u pixels from top edge",
                crop);
        y = 0;
        h -= crop;
        offset += handle->stride * crop * bpp / 8;
    } else {
        y = displayFrame.top;
    }

    if (displayFrame.bottom > pdev->yres) {
        int crop = displayFrame.bottom - pdev->yres;
        ALOGV("layer off bottom side of screen; cropping %u pixels from bottom edge",
                crop);
        h -= crop;
    }

    cfg.state = cfg.S3C_FB_WIN_STATE_BUFFER;
    cfg.fd = handle->fd;
    cfg.x = x;
    cfg.y = y;
    cfg.w = w;
    cfg.h = h;
    cfg.format = exynos5_format_to_s3c_format(handle->format);
    cfg.offset = offset;
    cfg.stride = handle->stride * bpp / 8;
    cfg.blending = exynos5_blending_to_s3c_blending(blending);
    cfg.fence_fd = fence_fd;
}

static void exynos5_config_overlay(hwc_layer_1_t *layer, s3c_fb_win_config &cfg,
        exynos5_hwc_composer_device_1_t *pdev)
{
    if (layer->compositionType == HWC_BACKGROUND) {
        hwc_color_t color = layer->backgroundColor;
        cfg.state = cfg.S3C_FB_WIN_STATE_COLOR;
        cfg.color = (color.r << 16) | (color.g << 8) | color.b;
        cfg.x = 0;
        cfg.y = 0;
        cfg.w = pdev->xres;
        cfg.h = pdev->yres;
        return;
    }

    private_handle_t *handle = private_handle_t::dynamicCast(layer->handle);
    exynos5_config_handle(handle, layer->sourceCrop, layer->displayFrame,
            layer->blending, layer->acquireFenceFd, cfg, pdev);
}

static int exynos5_post_fimd(exynos5_hwc_composer_device_1_t *pdev,
        hwc_display_contents_1_t* contents)
{
    exynos5_hwc_post_data_t *pdata = &pdev->bufs;
    struct s3c_fb_win_config_data win_data;
    struct s3c_fb_win_config *config = win_data.config;

    memset(config, 0, sizeof(win_data.config));
    for (size_t i = 0; i < NUM_HW_WINDOWS; i++)
        config[i].fence_fd = -1;

    for (size_t i = 0; i < NUM_HW_WINDOWS; i++) {
        int layer_idx = pdata->overlay_map[i];
        if (layer_idx != -1) {
            hwc_layer_1_t &layer = contents->hwLayers[layer_idx];
            private_handle_t *handle =
                    private_handle_t::dynamicCast(layer.handle);

            if (pdata->gsc_map[i].mode == exynos5_gsc_map_t::GSC_M2M) {
                int gsc_idx = pdata->gsc_map[i].idx;
                exynos5_gsc_data_t &gsc = pdev->gsc[gsc_idx];

                // RGBX8888 surfaces are already in the right color order from the GPU,
                // RGB565 and YUV surfaces need the Gscaler to swap R & B
                int dst_format = HAL_PIXEL_FORMAT_BGRA_8888;
                if (exynos5_format_is_rgb(handle->format) &&
                                handle->format != HAL_PIXEL_FORMAT_RGB_565)
                    dst_format = HAL_PIXEL_FORMAT_RGBX_8888;

                hwc_rect_t sourceCrop = { 0, 0,
                        WIDTH(layer.displayFrame), HEIGHT(layer.displayFrame) };
                int err = exynos5_config_gsc_m2m(layer, pdev->alloc_device, &gsc,
                        gsc_idx, dst_format, &sourceCrop);
                if (err < 0) {
                    ALOGE("failed to configure gscaler %u for layer %u",
                            gsc_idx, i);
                    pdata->gsc_map[i].mode = exynos5_gsc_map_t::GSC_NONE;
                    continue;
                }

                buffer_handle_t dst_buf = gsc.dst_buf[gsc.current_buf];
                private_handle_t *dst_handle =
                        private_handle_t::dynamicCast(dst_buf);
                int fence = gsc.dst_cfg.releaseFenceFd;
                exynos5_config_handle(dst_handle, sourceCrop,
                        layer.displayFrame, layer.blending, fence, config[i],
                        pdev);
            } else {
                exynos5_config_overlay(&layer, config[i], pdev);
            }
        }
        if (i == 0 && config[i].blending != S3C_FB_BLENDING_NONE) {
            ALOGV("blending not supported on window 0; forcing BLENDING_NONE");
            config[i].blending = S3C_FB_BLENDING_NONE;
        }

        ALOGV("window %u configuration:", i);
        dump_config(config[i]);
    }

    int ret = ioctl(pdev->fd, S3CFB_WIN_CONFIG, &win_data);
    for (size_t i = 0; i < NUM_HW_WINDOWS; i++)
        if (config[i].fence_fd != -1)
            close(config[i].fence_fd);
    if (ret < 0) {
        ALOGE("ioctl S3CFB_WIN_CONFIG failed: %s", strerror(errno));
        return ret;
    }

    memcpy(pdev->last_config, &win_data.config, sizeof(win_data.config));
    memcpy(pdev->last_gsc_map, pdata->gsc_map, sizeof(pdata->gsc_map));
    pdev->last_fb_window = pdata->fb_window;
    for (size_t i = 0; i < NUM_HW_WINDOWS; i++) {
        int layer_idx = pdata->overlay_map[i];
        if (layer_idx != -1) {
            hwc_layer_1_t &layer = contents->hwLayers[layer_idx];
            pdev->last_handles[i] = layer.handle;
        }
    }

    return win_data.fence;
}

static int exynos5_clear_fimd(exynos5_hwc_composer_device_1_t *pdev)
{
    struct s3c_fb_win_config_data win_data;
    memset(&win_data, 0, sizeof(win_data));

    int ret = ioctl(pdev->fd, S3CFB_WIN_CONFIG, &win_data);
    LOG_ALWAYS_FATAL_IF(ret < 0,
            "ioctl S3CFB_WIN_CONFIG failed to clear screen: %s",
            strerror(errno));
    // the causes of an empty config failing are all unrecoverable

    return win_data.fence;
}

static int exynos5_set_fimd(exynos5_hwc_composer_device_1_t *pdev,
        hwc_display_contents_1_t* contents)
{
    hwc_layer_1_t *fb_layer = NULL;
    int err = 0;

    if (pdev->bufs.fb_window != NO_FB_NEEDED) {
        for (size_t i = 0; i < contents->numHwLayers; i++) {
            if (contents->hwLayers[i].compositionType ==
                    HWC_FRAMEBUFFER_TARGET) {
                pdev->bufs.overlay_map[pdev->bufs.fb_window] = i;
                fb_layer = &contents->hwLayers[i];
                break;
            }
        }

        if (CC_UNLIKELY(!fb_layer)) {
            ALOGE("framebuffer target expected, but not provided");
            err = -EINVAL;
        } else {
            ALOGV("framebuffer target buffer:");
            dump_layer(fb_layer);
        }
    }

    int fence;
    if (!err) {
        fence = exynos5_post_fimd(pdev, contents);
        if (fence < 0)
            err = fence;
    }

    if (err)
        fence = exynos5_clear_fimd(pdev);

    for (size_t i = 0; i < NUM_HW_WINDOWS; i++) {
        if (pdev->bufs.overlay_map[i] != -1) {
            hwc_layer_1_t &layer =
                    contents->hwLayers[pdev->bufs.overlay_map[i]];
            int dup_fd = dup_or_warn(fence);
            if (pdev->bufs.gsc_map[i].mode == exynos5_gsc_map_t::GSC_M2M) {
                int gsc_idx = pdev->bufs.gsc_map[i].idx;
                exynos5_gsc_data_t &gsc = pdev->gsc[gsc_idx];
                gsc.dst_buf_fence[gsc.current_buf] = dup_fd;
                gsc.current_buf = (gsc.current_buf + 1) % NUM_GSC_DST_BUFS;
            } else {
                layer.releaseFenceFd = dup_fd;
            }
        }
    }
    contents->retireFenceFd = fence;

    return err;
}

static int exynos5_set_hdmi(exynos5_hwc_composer_device_1_t *pdev,
        hwc_display_contents_1_t* contents)
{
    hwc_layer_1_t *video_layer = NULL;

    if (!pdev->hdmi_enabled) {
        for (size_t i = 0; i < contents->numHwLayers; i++) {
            hwc_layer_1_t &layer = contents->hwLayers[i];
            if (layer.acquireFenceFd != -1) {
                close(layer.acquireFenceFd);
                layer.acquireFenceFd = -1;
            }
        }
        return 0;
    }

    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];

        if (layer.flags & HWC_SKIP_LAYER) {
            ALOGV("HDMI skipping layer %d", i);
            continue;
        }

        if (layer.compositionType == HWC_OVERLAY) {
             if (!layer.handle)
                continue;

            ALOGV("HDMI video layer:");
            dump_layer(&layer);

            exynos5_gsc_data_t &gsc = pdev->gsc[HDMI_GSC_IDX];
            int ret = exynos5_config_gsc_m2m(layer, pdev->alloc_device, &gsc, 1,
                                             HAL_PIXEL_FORMAT_RGBX_8888, NULL);
            if (ret < 0) {
                ALOGE("failed to configure gscaler for video layer");
                continue;
            }

            buffer_handle_t dst_buf = gsc.dst_buf[gsc.current_buf];
            private_handle_t *h = private_handle_t::dynamicCast(dst_buf);

            int acquireFenceFd = gsc.dst_cfg.releaseFenceFd;
            int releaseFenceFd = -1;

            hdmi_output(pdev, pdev->hdmi_layers[0], layer, h, acquireFenceFd,
                                                             &releaseFenceFd);
            video_layer = &layer;

            gsc.dst_buf_fence[gsc.current_buf] = releaseFenceFd;
            gsc.current_buf = (gsc.current_buf + 1) % NUM_GSC_DST_BUFS;
            if (contents->retireFenceFd < 0)
                contents->retireFenceFd = dup_or_warn(releaseFenceFd);
            else {
                int merged = merge_or_warn("hdmi",
                        contents->retireFenceFd, layer.releaseFenceFd);
                close(contents->retireFenceFd);
                contents->retireFenceFd = merged;
            }
        }

        if (layer.compositionType == HWC_FRAMEBUFFER_TARGET) {
            if (pdev->hdmi_fb_needed && layer.handle) {
                ALOGV("HDMI FB layer:");
                dump_layer(&layer);

                private_handle_t *h = private_handle_t::dynamicCast(layer.handle);
                hdmi_show_layer(pdev, pdev->hdmi_layers[1]);
                hdmi_output(pdev, pdev->hdmi_layers[1], layer, h, layer.acquireFenceFd,
                                                                 &layer.releaseFenceFd);

                if (contents->retireFenceFd < 0)
                    contents->retireFenceFd = dup_or_warn(layer.releaseFenceFd);
                else {
                    int merged = merge_or_warn("hdmi",
                            contents->retireFenceFd, layer.releaseFenceFd);
                    close(contents->retireFenceFd);
                    contents->retireFenceFd = merged;
                }
            } else {
                if (layer.acquireFenceFd >= 0) {
                    close(layer.acquireFenceFd);
                    layer.acquireFenceFd = -1;
                }
                hdmi_hide_layer(pdev, pdev->hdmi_layers[1]);
            }
        }
    }

    if (!video_layer) {
        hdmi_disable_layer(pdev, pdev->hdmi_layers[0]);
        exynos5_cleanup_gsc_m2m(pdev, HDMI_GSC_IDX);
    }

    if (exynos_v4l2_s_ctrl(pdev->hdmi_layers[1].fd, V4L2_CID_TV_UPDATE, 1) < 0) {
        ALOGE("%s: s_ctrl(CID_TV_UPDATE) failed %d", __func__, errno);
        return -1;
    }

    return 0;
}

static int exynos5_set(struct hwc_composer_device_1 *dev,
        size_t numDisplays, hwc_display_contents_1_t** displays)
{
    if (!numDisplays || !displays)
        return 0;

    exynos5_hwc_composer_device_1_t *pdev =
            (exynos5_hwc_composer_device_1_t *)dev;
    hwc_display_contents_1_t *fimd_contents = displays[HWC_DISPLAY_PRIMARY];
    hwc_display_contents_1_t *hdmi_contents = displays[HWC_DISPLAY_EXTERNAL];
    int fimd_err = 0, hdmi_err = 0;

    if (fimd_contents)
        fimd_err = exynos5_set_fimd(pdev, fimd_contents);

    if (hdmi_contents)
        hdmi_err = exynos5_set_hdmi(pdev, hdmi_contents);

    if (fimd_err)
        return fimd_err;

    return hdmi_err;
}

static void exynos5_registerProcs(struct hwc_composer_device_1* dev,
        hwc_procs_t const* procs)
{
    struct exynos5_hwc_composer_device_1_t* pdev =
            (struct exynos5_hwc_composer_device_1_t*)dev;
    pdev->procs = procs;
}

static int exynos5_query(struct hwc_composer_device_1* dev, int what, int *value)
{
    struct exynos5_hwc_composer_device_1_t *pdev =
            (struct exynos5_hwc_composer_device_1_t *)dev;

    switch (what) {
    case HWC_BACKGROUND_LAYER_SUPPORTED:
        // we support the background layer
        value[0] = 1;
        break;
    case HWC_VSYNC_PERIOD:
        // vsync period in nanosecond
        value[0] = pdev->vsync_period;
        break;
    default:
        // unsupported query
        return -EINVAL;
    }
    return 0;
}

static int exynos5_eventControl(struct hwc_composer_device_1 *dev, int dpy,
        int event, int enabled)
{
    struct exynos5_hwc_composer_device_1_t *pdev =
            (struct exynos5_hwc_composer_device_1_t *)dev;

    switch (event) {
    case HWC_EVENT_VSYNC:
        __u32 val = !!enabled;
        int err = ioctl(pdev->fd, S3CFB_SET_VSYNC_INT, &val);
        if (err < 0) {
            ALOGE("vsync ioctl failed");
            return -errno;
        }

        return 0;
    }

    return -EINVAL;
}

static void handle_hdmi_uevent(struct exynos5_hwc_composer_device_1_t *pdev,
        const char *buff, int len)
{
    const char *s = buff;
    s += strlen(s) + 1;

    while (*s) {
        if (!strncmp(s, "SWITCH_STATE=", strlen("SWITCH_STATE=")))
            pdev->hdmi_hpd = atoi(s + strlen("SWITCH_STATE=")) == 1;

        s += strlen(s) + 1;
        if (s - buff >= len)
            break;
    }

    if (pdev->hdmi_hpd) {
        if (hdmi_get_config(pdev)) {
            ALOGE("Error reading HDMI configuration");
            pdev->hdmi_hpd = false;
            return;
        }

        pdev->hdmi_blanked = false;
    }

    ALOGV("HDMI HPD changed to %s", pdev->hdmi_hpd ? "enabled" : "disabled");
    if (pdev->hdmi_hpd)
        ALOGI("HDMI Resolution changed to %dx%d", pdev->hdmi_h, pdev->hdmi_w);

    /* hwc_dev->procs is set right after the device is opened, but there is
     * still a race condition where a hotplug event might occur after the open
     * but before the procs are registered. */
    if (pdev->procs)
        pdev->procs->hotplug(pdev->procs, HWC_DISPLAY_EXTERNAL, pdev->hdmi_hpd);
}

static void handle_vsync_event(struct exynos5_hwc_composer_device_1_t *pdev)
{
    if (!pdev->procs)
        return;

    int err = lseek(pdev->vsync_fd, 0, SEEK_SET);
    if (err < 0) {
        ALOGE("error seeking to vsync timestamp: %s", strerror(errno));
        return;
    }

    char buf[4096];
    err = read(pdev->vsync_fd, buf, sizeof(buf));
    if (err < 0) {
        ALOGE("error reading vsync timestamp: %s", strerror(errno));
        return;
    }
    buf[sizeof(buf) - 1] = '\0';

    errno = 0;
    uint64_t timestamp = strtoull(buf, NULL, 0);
    if (!errno)
        pdev->procs->vsync(pdev->procs, 0, timestamp);
}

static void *hwc_vsync_thread(void *data)
{
    struct exynos5_hwc_composer_device_1_t *pdev =
            (struct exynos5_hwc_composer_device_1_t *)data;
    char uevent_desc[4096];
    memset(uevent_desc, 0, sizeof(uevent_desc));

    setpriority(PRIO_PROCESS, 0, HAL_PRIORITY_URGENT_DISPLAY);

    uevent_init();

    char temp[4096];
    int err = read(pdev->vsync_fd, temp, sizeof(temp));
    if (err < 0) {
        ALOGE("error reading vsync timestamp: %s", strerror(errno));
        return NULL;
    }

    struct pollfd fds[2];
    fds[0].fd = pdev->vsync_fd;
    fds[0].events = POLLPRI;
    fds[1].fd = uevent_get_fd();
    fds[1].events = POLLIN;

    while (true) {
        int err = poll(fds, 2, -1);

        if (err > 0) {
            if (fds[0].revents & POLLPRI) {
                handle_vsync_event(pdev);
            }
            else if (fds[1].revents & POLLIN) {
                int len = uevent_next_event(uevent_desc,
                        sizeof(uevent_desc) - 2);

                bool hdmi = !strcmp(uevent_desc,
                        "change@/devices/virtual/switch/hdmi");
                if (hdmi)
                    handle_hdmi_uevent(pdev, uevent_desc, len);
            }
        }
        else if (err == -1) {
            if (errno == EINTR)
                break;
            ALOGE("error in vsync thread: %s", strerror(errno));
        }
    }

    return NULL;
}

static int exynos5_blank(struct hwc_composer_device_1 *dev, int disp, int blank)
{
    struct exynos5_hwc_composer_device_1_t *pdev =
            (struct exynos5_hwc_composer_device_1_t *)dev;

    switch (disp) {
    case HWC_DISPLAY_PRIMARY: {
        int fb_blank = blank ? FB_BLANK_POWERDOWN : FB_BLANK_UNBLANK;
        int err = ioctl(pdev->fd, FBIOBLANK, fb_blank);
        if (err < 0) {
            if (errno == EBUSY)
                ALOGI("%sblank ioctl failed (display already %sblanked)",
                        blank ? "" : "un", blank ? "" : "un");
            else
                ALOGE("%sblank ioctl failed: %s", blank ? "" : "un",
                        strerror(errno));
            return -errno;
        }
        break;
    }

    case HWC_DISPLAY_EXTERNAL:
        if (pdev->hdmi_hpd) {
            if (blank && !pdev->hdmi_blanked)
                hdmi_disable(pdev);
            pdev->hdmi_blanked = !!blank;
        }
        break;

    default:
        return -EINVAL;

    }

    return 0;
}

static void exynos5_dump(hwc_composer_device_1* dev, char *buff, int buff_len)
{
    if (buff_len <= 0)
        return;

    struct exynos5_hwc_composer_device_1_t *pdev =
            (struct exynos5_hwc_composer_device_1_t *)dev;

    android::String8 result;

    result.appendFormat("  hdmi_enabled=%u\n", pdev->hdmi_enabled);
    if (pdev->hdmi_enabled)
        result.appendFormat("    w=%u, h=%u\n", pdev->hdmi_w, pdev->hdmi_h);
    result.append(
            "   type   |  handle  |  color   | blend | format |   position    |     size      | gsc \n"
            "----------+----------|----------+-------+--------+---------------+---------------------\n");
    //        8_______ | 8_______ | 8_______ | 5____ | 6_____ | [5____,5____] | [5____,5____] | 3__ \n"

    for (size_t i = 0; i < NUM_HW_WINDOWS; i++) {
        struct s3c_fb_win_config &config = pdev->last_config[i];
        if (config.state == config.S3C_FB_WIN_STATE_DISABLED) {
            result.appendFormat(" %8s | %8s | %8s | %5s | %6s | %13s | %13s",
                    "DISABLED", "-", "-", "-", "-", "-", "-");
        }
        else {
            if (config.state == config.S3C_FB_WIN_STATE_COLOR)
                result.appendFormat(" %8s | %8s | %8x | %5s | %6s", "COLOR",
                        "-", config.color, "-", "-");
            else
                result.appendFormat(" %8s | %8x | %8s | %5x | %6x",
                        pdev->last_fb_window == i ? "FB" : "OVERLAY",
                        intptr_t(pdev->last_handles[i]),
                        "-", config.blending, config.format);

            result.appendFormat(" | [%5d,%5d] | [%5u,%5u]", config.x, config.y,
                    config.w, config.h);
        }
        if (pdev->last_gsc_map[i].mode == exynos5_gsc_map_t::GSC_NONE)
            result.appendFormat(" | %3s", "-");
        else
            result.appendFormat(" | %3d",
                    AVAILABLE_GSC_UNITS[pdev->last_gsc_map[i].idx]);
        result.append("\n");
    }

    strlcpy(buff, result.string(), buff_len);
}

static int exynos5_getDisplayConfigs(struct hwc_composer_device_1 *dev,
        int disp, uint32_t *configs, size_t *numConfigs)
{
    struct exynos5_hwc_composer_device_1_t *pdev =
               (struct exynos5_hwc_composer_device_1_t *)dev;

    if (*numConfigs == 0)
        return 0;

    if (disp == HWC_DISPLAY_PRIMARY) {
        configs[0] = 0;
        *numConfigs = 1;
        return 0;
    } else if (disp == HWC_DISPLAY_EXTERNAL) {
        if (!pdev->hdmi_hpd) {
            return -EINVAL;
        }

        int err = hdmi_get_config(pdev);
        if (err) {
            return -EINVAL;
        }

        configs[0] = 0;
        *numConfigs = 1;
        return 0;
    }

    return -EINVAL;
}

static int32_t exynos5_fimd_attribute(struct exynos5_hwc_composer_device_1_t *pdev,
        const uint32_t attribute)
{
    switch(attribute) {
    case HWC_DISPLAY_VSYNC_PERIOD:
        return pdev->vsync_period;

    case HWC_DISPLAY_WIDTH:
        return pdev->xres;

    case HWC_DISPLAY_HEIGHT:
        return pdev->yres;

    case HWC_DISPLAY_DPI_X:
        return pdev->xdpi;

    case HWC_DISPLAY_DPI_Y:
        return pdev->ydpi;

    default:
        ALOGE("unknown display attribute %u", attribute);
        return -EINVAL;
    }
}

static int32_t exynos5_hdmi_attribute(struct exynos5_hwc_composer_device_1_t *pdev,
        const uint32_t attribute)
{
    switch(attribute) {
    case HWC_DISPLAY_VSYNC_PERIOD:
        return pdev->vsync_period;

    case HWC_DISPLAY_WIDTH:
        return pdev->hdmi_w;

    case HWC_DISPLAY_HEIGHT:
        return pdev->hdmi_h;

    case HWC_DISPLAY_DPI_X:
    case HWC_DISPLAY_DPI_Y:
        return 0; // unknown

    default:
        ALOGE("unknown display attribute %u", attribute);
        return -EINVAL;
    }
}

static int exynos5_getDisplayAttributes(struct hwc_composer_device_1 *dev,
        int disp, uint32_t config, const uint32_t *attributes, int32_t *values)
{
    struct exynos5_hwc_composer_device_1_t *pdev =
                   (struct exynos5_hwc_composer_device_1_t *)dev;

    for (int i = 0; attributes[i] != HWC_DISPLAY_NO_ATTRIBUTE; i++) {
        if (disp == HWC_DISPLAY_PRIMARY)
            values[i] = exynos5_fimd_attribute(pdev, attributes[i]);
        else if (disp == HWC_DISPLAY_EXTERNAL)
            values[i] = exynos5_hdmi_attribute(pdev, attributes[i]);
        else {
            ALOGE("unknown display type %u", disp);
            return -EINVAL;
        }
    }

    return 0;
}

static int exynos5_close(hw_device_t* device);

static void get_screen_res(const char *fbname, int32_t *xres, int32_t *yres,
                           int32_t *refresh)
{
    char *path;
    int fd;
    char buf[128];
    int ret;
    unsigned int _x, _y, _r;

    asprintf(&path, "/sys/class/graphics/%s/modes", fbname);
    if (!path)
        goto err_asprintf;
    fd = open(path, O_RDONLY);
    if (fd < 0)
        goto err_open;
    ret = read(fd, buf, sizeof(buf));
    if (ret <= 0)
        goto err_read;
    buf[sizeof(buf)-1] = '\0';

    ret = sscanf(buf, "U:%ux%up-%u", &_x, &_y, &_r);
    if (ret != 3)
        goto err_sscanf;

    ALOGI("Using %ux%u %uHz resolution for '%s' from modes list\n",
          _x, _y, _r, fbname);

    *xres = (int32_t)_x;
    *yres = (int32_t)_y;
    *refresh = (int32_t)_r;

    close(fd);
    free(path);
    return;

err_sscanf:
err_read:
    close(fd);
err_open:
    free(path);
err_asprintf:
    *xres = 2560;
    *yres = 1600;
    *refresh = 60;
}

static int exynos5_open(const struct hw_module_t *module, const char *name,
        struct hw_device_t **device)
{
    int ret;
    int refreshRate;
    int sw_fd;

    if (strcmp(name, HWC_HARDWARE_COMPOSER)) {
        return -EINVAL;
    }

    struct exynos5_hwc_composer_device_1_t *dev;
    dev = (struct exynos5_hwc_composer_device_1_t *)malloc(sizeof(*dev));
    memset(dev, 0, sizeof(*dev));

    if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID,
            (const struct hw_module_t **)&dev->gralloc_module)) {
        ALOGE("failed to get gralloc hw module");
        ret = -EINVAL;
        goto err_get_module;
    }

    if (gralloc_open((const hw_module_t *)dev->gralloc_module,
            &dev->alloc_device)) {
        ALOGE("failed to open gralloc");
        ret = -EINVAL;
        goto err_get_module;
    }

    dev->fd = open("/dev/graphics/fb0", O_RDWR);
    if (dev->fd < 0) {
        ALOGE("failed to open framebuffer");
        ret = dev->fd;
        goto err_open_fb;
    }

    struct fb_var_screeninfo info;
    if (ioctl(dev->fd, FBIOGET_VSCREENINFO, &info) == -1) {
        ALOGE("FBIOGET_VSCREENINFO ioctl failed: %s", strerror(errno));
        ret = -errno;
        goto err_ioctl;
    }

    get_screen_res("fb0", &dev->xres, &dev->yres, &refreshRate);
    if (refreshRate == 0) {
        ALOGW("invalid refresh rate, assuming 60 Hz");
        refreshRate = 60;
    }

    dev->xdpi = 1000 * (dev->xres * 25.4f) / info.width;
    dev->ydpi = 1000 * (dev->yres * 25.4f) / info.height;
    dev->vsync_period  = 1000000000 / refreshRate;

    ALOGV("using\n"
          "xres         = %d px\n"
          "yres         = %d px\n"
          "width        = %d mm (%f dpi)\n"
          "height       = %d mm (%f dpi)\n"
          "refresh rate = %d Hz\n",
          dev->xres, dev->yres, info.width, dev->xdpi / 1000.0,
          info.height, dev->ydpi / 1000.0, refreshRate);

    for (size_t i = 0; i < NUM_GSC_UNITS; i++)
        for (size_t j = 0; j < NUM_GSC_DST_BUFS; j++)
            dev->gsc[i].dst_buf_fence[j] = -1;

    dev->hdmi_mixer0 = open("/dev/v4l-subdev7", O_RDWR);
    if (dev->hdmi_mixer0 < 0)
        ALOGE("failed to open hdmi mixer0 subdev");

    dev->hdmi_layers[0].id = 0;
    dev->hdmi_layers[0].fd = open("/dev/video16", O_RDWR);
    if (dev->hdmi_layers[0].fd < 0) {
        ALOGE("failed to open hdmi layer0 device");
        ret = dev->hdmi_layers[0].fd;
        goto err_mixer0;
    }

    dev->hdmi_layers[1].id = 1;
    dev->hdmi_layers[1].fd = open("/dev/video17", O_RDWR);
    if (dev->hdmi_layers[1].fd < 0) {
        ALOGE("failed to open hdmi layer1 device");
        ret = dev->hdmi_layers[1].fd;
        goto err_hdmi0;
    }

    dev->vsync_fd = open("/sys/devices/platform/exynos5-fb.1/vsync", O_RDONLY);
    if (dev->vsync_fd < 0) {
        ALOGE("failed to open vsync attribute");
        ret = dev->vsync_fd;
        goto err_hdmi1;
    }

    sw_fd = open("/sys/class/switch/hdmi/state", O_RDONLY);
    if (sw_fd) {
        char val;
        if (read(sw_fd, &val, 1) == 1 && val == '1') {
            dev->hdmi_hpd = true;
            if (hdmi_get_config(dev)) {
                ALOGE("Error reading HDMI configuration");
                dev->hdmi_hpd = false;
            }
        }
    }

    dev->base.common.tag = HARDWARE_DEVICE_TAG;
    dev->base.common.version = HWC_DEVICE_API_VERSION_1_1;
    dev->base.common.module = const_cast<hw_module_t *>(module);
    dev->base.common.close = exynos5_close;

    dev->base.prepare = exynos5_prepare;
    dev->base.set = exynos5_set;
    dev->base.eventControl = exynos5_eventControl;
    dev->base.blank = exynos5_blank;
    dev->base.query = exynos5_query;
    dev->base.registerProcs = exynos5_registerProcs;
    dev->base.dump = exynos5_dump;
    dev->base.getDisplayConfigs = exynos5_getDisplayConfigs;
    dev->base.getDisplayAttributes = exynos5_getDisplayAttributes;

    *device = &dev->base.common;

    ret = pthread_create(&dev->vsync_thread, NULL, hwc_vsync_thread, dev);
    if (ret) {
        ALOGE("failed to start vsync thread: %s", strerror(ret));
        ret = -ret;
        goto err_vsync;
    }

    char value[PROPERTY_VALUE_MAX];
    property_get("debug.hwc.force_gpu", value, "0");
    dev->force_gpu = atoi(value);

    return 0;

err_vsync:
    close(dev->vsync_fd);
err_mixer0:
    if (dev->hdmi_mixer0 >= 0)
        close(dev->hdmi_mixer0);
err_hdmi1:
    close(dev->hdmi_layers[0].fd);
err_hdmi0:
    close(dev->hdmi_layers[1].fd);
err_ioctl:
    close(dev->fd);
err_open_fb:
    gralloc_close(dev->alloc_device);
err_get_module:
    free(dev);
    return ret;
}

static int exynos5_close(hw_device_t *device)
{
    struct exynos5_hwc_composer_device_1_t *dev =
            (struct exynos5_hwc_composer_device_1_t *)device;
    pthread_kill(dev->vsync_thread, SIGTERM);
    pthread_join(dev->vsync_thread, NULL);
    for (size_t i = 0; i < NUM_GSC_UNITS; i++)
        exynos5_cleanup_gsc_m2m(dev, i);
    gralloc_close(dev->alloc_device);
    close(dev->vsync_fd);
    if (dev->hdmi_mixer0 >= 0)
        close(dev->hdmi_mixer0);
    close(dev->hdmi_layers[0].fd);
    close(dev->hdmi_layers[1].fd);
    close(dev->fd);
    return 0;
}

static struct hw_module_methods_t exynos5_hwc_module_methods = {
    open: exynos5_open,
};

hwc_module_t HAL_MODULE_INFO_SYM = {
    common: {
        tag: HARDWARE_MODULE_TAG,
        module_api_version: HWC_MODULE_API_VERSION_0_1,
        hal_api_version: HARDWARE_HAL_API_VERSION,
        id: HWC_HARDWARE_MODULE_ID,
        name: "Samsung exynos5 hwcomposer module",
        author: "Google",
        methods: &exynos5_hwc_module_methods,
    }
};
