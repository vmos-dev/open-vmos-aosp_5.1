/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
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
#include <malloc.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <linux/omapfb.h>
#include <sys/mman.h>
#include <sys/resource.h>

#include <cutils/properties.h>
#include <cutils/log.h>
#include <cutils/native_handle.h>
#define HWC_REMOVE_DEPRECATED_VERSIONS 0
#include <hardware/hardware.h>
#include <hardware/hwcomposer.h>
#include <EGL/egl.h>
#include <hardware_legacy/uevent.h>
#include <png.h>
#include <utils/Timers.h>

#include <system/graphics.h>
#include <linux/bltsville.h>

#define MAX_HWC_LAYERS 32

#define ASPECT_RATIO_TOLERANCE 0.02f

#define min(a, b) ( { typeof(a) __a = (a), __b = (b); __a < __b ? __a : __b; } )
#define max(a, b) ( { typeof(a) __a = (a), __b = (b); __a > __b ? __a : __b; } )
#define swap(a, b) do { typeof(a) __a = (a); (a) = (b); (b) = __a; } while (0)

#define WIDTH(rect) ((rect).right - (rect).left)
#define HEIGHT(rect) ((rect).bottom - (rect).top)

#define DIV_ROUND_UP(a, b) (((a) + (b) - 1) / (b))

#include <video/dsscomp.h>
#include <video/omap_hwc.h>

#include "hal_public.h"
#include "rgz_2d.h"

#include <linux/ion.h>
#include <linux/omap_ion.h>
#include <ion/ion.h>

#define MAX_HW_OVERLAYS 4
#define NUM_NONSCALING_OVERLAYS 1
#define NUM_EXT_DISPLAY_BACK_BUFFERS 2

struct ext_transform_t {
    __u8 rotation : 3;          /* 90-degree clockwise rotations */
    __u8 hflip    : 1;          /* flip l-r (after rotation) */
    __u8 enabled  : 1;          /* cloning enabled */
    __u8 docking  : 1;          /* docking vs. mirroring - used for state */
};

/* cloning support and state */
struct omap4_hwc_ext {
    /* support */
    struct ext_transform_t mirror;      /* mirroring settings */
    struct ext_transform_t dock;        /* docking settings */
    float lcd_xpy;                      /* pixel ratio for UI */
    __u8 avoid_mode_change;             /* use HDMI mode used for mirroring if possible */
    __u8 force_dock;                     /* must dock */
    __u8 hdmi_state;                     /* whether HDMI is connected */

    /* state */
    __u8 on_tv;                         /* using a tv */
    struct ext_transform_t current;     /* current settings */
    struct ext_transform_t last;        /* last-used settings */

    /* configuration */
    __u32 last_xres_used;               /* resolution and pixel ratio used for mode selection */
    __u32 last_yres_used;
    __u32 last_mode;                    /* 2-s complement of last HDMI mode set, 0 if none */
    __u32 mirror_mode;                  /* 2-s complement of mode used when mirroring */
    float last_xpy;
    __u16 width;                        /* external screen dimensions */
    __u16 height;
    __u32 xres;                         /* external screen resolution */
    __u32 yres;
    float m[2][3];                      /* external transformation matrix */
    hwc_rect_t mirror_region;           /* region of screen to mirror */
};
typedef struct omap4_hwc_ext omap4_hwc_ext_t;

/* used by property settings */
enum {
    EXT_ROTATION    = 3,        /* rotation while mirroring */
    EXT_HFLIP       = (1 << 2), /* flip l-r on output (after rotation) */
};

enum bltpolicy {
    BLTPOLICY_DISABLED = 0,
    BLTPOLICY_DEFAULT = 1,    /* Default blit policy */
    BLTPOLICY_ALL,            /* Test mode to attempt to blit all */
};

enum bltmode {
    BLTMODE_PAINT = 0,    /* Attempt to blit layer by layer */
    BLTMODE_REGION = 1,   /* Attempt to blit layers via regions */
};

/* ARGB image */
struct omap4_hwc_img {
    int width;
    int height;
    int rowbytes;
    int size;
    unsigned char *ptr;
} dock_image = { .rowbytes = 0 };

struct omap4_hwc_module {
    hwc_module_t base;

    IMG_framebuffer_device_public_t *fb_dev;
};
typedef struct omap4_hwc_module omap4_hwc_module_t;

struct counts {
    unsigned int possible_overlay_layers;
    unsigned int composited_layers;
    unsigned int scaled_layers;
    unsigned int RGB;
    unsigned int BGR;
    unsigned int NV12;
    unsigned int dockable;
    unsigned int protected;

    unsigned int max_hw_overlays;
    unsigned int max_scaling_overlays;
    unsigned int mem;
    unsigned int s3d;

    unsigned int large_rgb32_layers;
};

struct omap4_hwc_device {
    /* static data */
    hwc_composer_device_1_t base;
    hwc_procs_t *procs;
    pthread_t hdmi_thread;
    pthread_mutex_t lock;

    IMG_framebuffer_device_public_t *fb_dev;
    struct dsscomp_display_info fb_dis;
    int fb_fd;                  /* file descriptor for /dev/fb0 */
    int dsscomp_fd;             /* file descriptor for /dev/dsscomp */
    int hdmi_fb_fd;             /* file descriptor for /dev/fb1 */
    int wakeup_evt;             /* eventfd used to wakeup event thread */

    int img_mem_size;           /* size of fb for hdmi */
    void *img_mem_ptr;          /* start of fb for hdmi */

    int flags_rgb_order;
    int flags_nv12_only;
    float upscaled_nv12_limit;

    int on_tv;                  /* using a tv */
    int force_sgx;
    omap4_hwc_ext_t ext;        /* external mirroring data */
    int idle;

    float primary_m[2][3];          /* internal transformation matrix */
    int primary_transform;
    int primary_rotation;
    hwc_rect_t primary_region;

    buffer_handle_t *buffers;
    int use_sgx;
    int swap_rb;
    unsigned int post2_layers; /* Buffers used with DSS pipes*/
    unsigned int post2_blit_buffers; /* Buffers used with blit */
    int ext_ovls;               /* # of overlays on external display for current composition */
    int ext_ovls_wanted;        /* # of overlays that should be on external display for current composition */
    int last_ext_ovls;          /* # of overlays on external/internal display for last composition */
    int last_int_ovls;

    enum bltmode blt_mode;
    enum bltpolicy blt_policy;

    int blit_flags;
    int blit_num;
    struct omap_hwc_data comp_data; /* This is a kernel data structure */
    struct rgz_blt_entry blit_ops[RGZ_MAX_BLITS];
    struct counts stats;
    int    ion_fd;
    struct ion_handle *ion_handles[2];

    /* fake vsync event state */
    pthread_mutex_t vsync_lock;
    int vsync_enabled;
    uint64_t last_vsync_time;
    int last_vsync_time_valid;
    uint64_t fake_vsync_period;
};
typedef struct omap4_hwc_device omap4_hwc_device_t;

#define HAL_FMT(f) ((f) == HAL_PIXEL_FORMAT_TI_NV12 ? "NV12" : \
                    (f) == HAL_PIXEL_FORMAT_TI_NV12_1D ? "NV12" : \
                    (f) == HAL_PIXEL_FORMAT_YV12 ? "YV12" : \
                    (f) == HAL_PIXEL_FORMAT_BGRX_8888 ? "xRGB32" : \
                    (f) == HAL_PIXEL_FORMAT_RGBX_8888 ? "xBGR32" : \
                    (f) == HAL_PIXEL_FORMAT_BGRA_8888 ? "ARGB32" : \
                    (f) == HAL_PIXEL_FORMAT_RGBA_8888 ? "ABGR32" : \
                    (f) == HAL_PIXEL_FORMAT_RGB_565 ? "RGB565" : "??")

#define DSS_FMT(f) ((f) == OMAP_DSS_COLOR_NV12 ? "NV12" : \
                    (f) == OMAP_DSS_COLOR_RGB24U ? "xRGB32" : \
                    (f) == OMAP_DSS_COLOR_ARGB32 ? "ARGB32" : \
                    (f) == OMAP_DSS_COLOR_RGB16 ? "RGB565" : "??")

static int debug = 0;
static int debugpost2 = 0;
static int debugblt = 0;
static rgz_t grgz;
static struct bvsurfgeom gscrngeom;

static void showfps(void)
{
    static int framecount = 0;
    static int lastframecount = 0;
    static nsecs_t lastfpstime = 0;
    static float fps = 0;
    char value[PROPERTY_VALUE_MAX];

    property_get("debug.hwc.showfps", value, "0");
    if (!atoi(value)) {
        return;
    }

    framecount++;
    if (!(framecount & 0x7)) {
        nsecs_t now = systemTime(SYSTEM_TIME_MONOTONIC);
        nsecs_t diff = now - lastfpstime;
        fps = ((framecount - lastframecount) * (float)(s2ns(1))) / diff;
        lastfpstime = now;
        lastframecount = framecount;
        ALOGI("%d Frames, %f FPS", framecount, fps);
    }
}

static void dump_layer(hwc_layer_1_t const* l)
{
    ALOGD("\ttype=%d, flags=%08x, handle=%p, tr=%02x, blend=%04x, {%d,%d,%d,%d}, {%d,%d,%d,%d}",
         l->compositionType, l->flags, l->handle, l->transform, l->blending,
         l->sourceCrop.left,
         l->sourceCrop.top,
         l->sourceCrop.right,
         l->sourceCrop.bottom,
         l->displayFrame.left,
         l->displayFrame.top,
         l->displayFrame.right,
         l->displayFrame.bottom);
}

static void dump_dsscomp(struct dsscomp_setup_dispc_data *d)
{
    unsigned i;

    ALOGD("[%08x] set: %c%c%c %d ovls\n",
         d->sync_id,
         (d->mode & DSSCOMP_SETUP_MODE_APPLY) ? 'A' : '-',
         (d->mode & DSSCOMP_SETUP_MODE_DISPLAY) ? 'D' : '-',
         (d->mode & DSSCOMP_SETUP_MODE_CAPTURE) ? 'C' : '-',
         d->num_ovls);

    for (i = 0; i < d->num_mgrs; i++) {
        struct dss2_mgr_info *mi = &d->mgrs[i];
        ALOGD(" (dis%d alpha=%d col=%08x ilace=%d)\n",
             mi->ix,
             mi->alpha_blending, mi->default_color,
             mi->interlaced);
    }

    for (i = 0; i < d->num_ovls; i++) {
        struct dss2_ovl_info *oi = &d->ovls[i];
        struct dss2_ovl_cfg *c = &oi->cfg;
        if (c->zonly)
            ALOGD("ovl%d(%s z%d)\n",
                 c->ix, c->enabled ? "ON" : "off", c->zorder);
        else
            ALOGD("ovl%d(%s z%d %s%s *%d%% %d*%d:%d,%d+%d,%d rot%d%s => %d,%d+%d,%d %p/%p|%d)\n",
                 c->ix, c->enabled ? "ON" : "off", c->zorder, DSS_FMT(c->color_mode),
                 c->pre_mult_alpha ? " premult" : "",
                 (c->global_alpha * 100 + 128) / 255,
                 c->width, c->height, c->crop.x, c->crop.y,
                 c->crop.w, c->crop.h,
                 c->rotation, c->mirror ? "+mir" : "",
                 c->win.x, c->win.y, c->win.w, c->win.h,
                 (void *) oi->ba, (void *) oi->uv, c->stride);
    }
}

struct dump_buf {
    char *buf;
    int buf_len;
    int len;
};

static void dump_printf(struct dump_buf *buf, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    buf->len += vsnprintf(buf->buf + buf->len, buf->buf_len - buf->len, fmt, ap);
    va_end(ap);
}

static void dump_set_info(omap4_hwc_device_t *hwc_dev, hwc_display_contents_1_t* list)
{
    struct dsscomp_setup_dispc_data *dsscomp = &hwc_dev->comp_data.dsscomp_data;
    char logbuf[1024];
    struct dump_buf log = {
        .buf = logbuf,
        .buf_len = sizeof(logbuf),
    };
    unsigned int i;

    dump_printf(&log, "set H{");
    for (i = 0; list && i < list->numHwLayers; i++) {
        if (i)
            dump_printf(&log, " ");
        hwc_layer_1_t *layer = &list->hwLayers[i];
        IMG_native_handle_t *handle = (IMG_native_handle_t *)layer->handle;
        if (hwc_dev->post2_blit_buffers) {
            if ((i + 1) < hwc_dev->post2_layers)
                dump_printf(&log, "%p:%s,", handle, "DSS");
            else
                dump_printf(&log, "%p:%s,", handle, "BV2D");
        }
        else
            dump_printf(&log, "%p:%s,", handle, layer->compositionType == HWC_OVERLAY ? "DSS" : "SGX");
        if ((layer->flags & HWC_SKIP_LAYER) || !handle) {
            dump_printf(&log, "SKIP");
            continue;
        }
        if (layer->flags & HWC_HINT_CLEAR_FB)
            dump_printf(&log, "CLR,");
        dump_printf(&log, "%d*%d(%s)", handle->iWidth, handle->iHeight, HAL_FMT(handle->iFormat));
        if (layer->transform)
            dump_printf(&log, "~%d", layer->transform);
    }
    dump_printf(&log, "} D{");
    for (i = 0; i < dsscomp->num_ovls; i++) {
        if (i)
            dump_printf(&log, " ");
        dump_printf(&log, "%d=", dsscomp->ovls[i].cfg.ix);
        if (dsscomp->ovls[i].cfg.enabled)
            dump_printf(&log, "%08x:%d*%d,%s",
                        dsscomp->ovls[i].ba,
                        dsscomp->ovls[i].cfg.width,
                        dsscomp->ovls[i].cfg.height,
                        DSS_FMT(dsscomp->ovls[i].cfg.color_mode));
        else
            dump_printf(&log, "-");
    }
    dump_printf(&log, "} L{");
    for (i = 0; i < hwc_dev->post2_layers; i++) {
        if (i)
            dump_printf(&log, " ");
        dump_printf(&log, "%p", hwc_dev->buffers[i]);
    }
    if (hwc_dev->post2_blit_buffers) {
        dump_printf(&log, "} B{");
        for (i = hwc_dev->post2_layers;
             i < hwc_dev->post2_blit_buffers + hwc_dev->post2_layers; i++) {
            dump_printf(&log, "%p ", hwc_dev->buffers[i]);
        }
    }
    dump_printf(&log, "}%s\n", hwc_dev->use_sgx ? " swap" : "");

    ALOGD("%s", log.buf);
}

static int sync_id = 0;

static int omap4_hwc_is_valid_format(int format)
{
    switch(format) {
    case HAL_PIXEL_FORMAT_RGB_565:
    case HAL_PIXEL_FORMAT_RGBX_8888:
    case HAL_PIXEL_FORMAT_RGBA_8888:
    case HAL_PIXEL_FORMAT_BGRA_8888:
    case HAL_PIXEL_FORMAT_BGRX_8888:
    case HAL_PIXEL_FORMAT_TI_NV12:
    case HAL_PIXEL_FORMAT_TI_NV12_1D:
        return 1;

    default:
        return 0;
    }
}

static int scaled(hwc_layer_1_t *layer)
{
    int w = WIDTH(layer->sourceCrop);
    int h = HEIGHT(layer->sourceCrop);

    if (layer->transform & HWC_TRANSFORM_ROT_90)
        swap(w, h);

    return WIDTH(layer->displayFrame) != w || HEIGHT(layer->displayFrame) != h;
}

static int is_protected(hwc_layer_1_t *layer)
{
    IMG_native_handle_t *handle = (IMG_native_handle_t *)layer->handle;

    return (handle->usage & GRALLOC_USAGE_PROTECTED);
}

#define is_BLENDED(layer) ((layer)->blending != HWC_BLENDING_NONE)

static int is_RGB32(IMG_native_handle_t *handle)
{
    switch(handle->iFormat)
    {
    case HAL_PIXEL_FORMAT_BGRA_8888:
    case HAL_PIXEL_FORMAT_BGRX_8888:
        return 1;
    default:
        return 0;
    }
}

static int is_RGB(IMG_native_handle_t *handle)
{
    switch(handle->iFormat)
    {
    case HAL_PIXEL_FORMAT_BGRA_8888:
    case HAL_PIXEL_FORMAT_BGRX_8888:
    case HAL_PIXEL_FORMAT_RGB_565:
        return 1;
    default:
        return 0;
    }
}
static int get_rgb_bpp(IMG_native_handle_t *handle)
{
    switch(handle->iFormat)
    {
    case HAL_PIXEL_FORMAT_BGRA_8888:
    case HAL_PIXEL_FORMAT_BGRX_8888:
    case HAL_PIXEL_FORMAT_RGBX_8888:
    case HAL_PIXEL_FORMAT_RGBA_8888:
        return 32;
    case HAL_PIXEL_FORMAT_RGB_565:
        return 16;
    default:
        return 0;
    }
}

static int is_BGR_format(int format)
{
    switch (format) {
    case HAL_PIXEL_FORMAT_RGBX_8888:
    case HAL_PIXEL_FORMAT_RGBA_8888:
        return 1;
    default:
        return 0;
    }
}

static int is_BGR(IMG_native_handle_t *handle)
{
    return is_BGR_format(handle->iFormat);
}

static int is_NV12(IMG_native_handle_t *handle)
{
    switch(handle->iFormat)
    {
    case HAL_PIXEL_FORMAT_TI_NV12:
    case HAL_PIXEL_FORMAT_TI_NV12_1D:
        return 1;
    default:
        return 0;
    }
}

static int is_upscaled_NV12(omap4_hwc_device_t *hwc_dev, hwc_layer_1_t *layer)
{
    if (!layer)
        return 0;

    IMG_native_handle_t *handle = (IMG_native_handle_t *)layer->handle;
    if (!is_NV12(handle))
        return 0;

    int w = WIDTH(layer->sourceCrop);
    int h = HEIGHT(layer->sourceCrop);

    if (layer->transform & HWC_TRANSFORM_ROT_90)
        swap(w, h);

    return (WIDTH(layer->displayFrame) >= w * hwc_dev->upscaled_nv12_limit ||
            HEIGHT(layer->displayFrame) >= h * hwc_dev->upscaled_nv12_limit);
}

static int dockable(hwc_layer_1_t *layer)
{
    IMG_native_handle_t *handle = (IMG_native_handle_t *)layer->handle;

    return (handle->usage & GRALLOC_USAGE_EXTERNAL_DISP);
}

/* test if layer appears to be RGB32 (4 Bpp) and > 1280x720 */
static int is_large_rgb32_layer(const hwc_layer_1_t *layer)
{
    IMG_native_handle_t *handle = (IMG_native_handle_t *)layer->handle;

    return is_RGB32(handle) &&
        (((layer->sourceCrop.right - layer->sourceCrop.left) > 1280) ||
        ((layer->sourceCrop.bottom - layer->sourceCrop.top) > 720));
}

static uint64_t vsync_clock_now()
{
    uint64_t now = 0;
    struct timespec ts;

    if (!clock_gettime(CLOCK_MONOTONIC, &ts))
        now = ((uint64_t)ts.tv_sec) * 1000000000ull + ts.tv_nsec;

    return now;
}

static void wakeup_hdmi_thread(omap4_hwc_device_t *hwc_dev)
{
    uint64_t tmp = 1;
    write(hwc_dev->wakeup_evt, &tmp, sizeof(tmp));
}

static void fire_vsync_event(omap4_hwc_device_t *hwc_dev, uint64_t timestamp)
{
    pthread_mutex_lock(&hwc_dev->vsync_lock);

    hwc_dev->last_vsync_time_valid = 1;
    hwc_dev->last_vsync_time = timestamp;

    pthread_mutex_unlock(&hwc_dev->vsync_lock);

    if (hwc_dev->procs && hwc_dev->procs->vsync)
        hwc_dev->procs->vsync(hwc_dev->procs, 0, timestamp);
}

static unsigned int mem1d(IMG_native_handle_t *handle)
{
    if (handle == NULL || is_NV12(handle))
        return 0;

    int bpp = handle->iFormat == HAL_PIXEL_FORMAT_RGB_565 ? 2 : 4;
    int stride = ALIGN(handle->iWidth, HW_ALIGN) * bpp;
    return stride * handle->iHeight;
}

static void
omap4_hwc_setup_layer_base(struct dss2_ovl_cfg *oc, int index, int format, int blended, int width, int height)
{
    unsigned int bits_per_pixel;

    /* YUV2RGB conversion */
    const struct omap_dss_cconv_coefs ctbl_bt601_5 = {
        298,  409,    0,  298, -208, -100,  298,    0,  517, 0,
    };

    /* convert color format */
    switch (format) {
    case HAL_PIXEL_FORMAT_RGBA_8888:
    case HAL_PIXEL_FORMAT_BGRA_8888:
        oc->color_mode = OMAP_DSS_COLOR_ARGB32;
        bits_per_pixel = 32;
        if (blended)
                break;

    case HAL_PIXEL_FORMAT_RGBX_8888:
    case HAL_PIXEL_FORMAT_BGRX_8888:
        oc->color_mode = OMAP_DSS_COLOR_RGB24U;
        bits_per_pixel = 32;
        break;

    case HAL_PIXEL_FORMAT_RGB_565:
        oc->color_mode = OMAP_DSS_COLOR_RGB16;
        bits_per_pixel = 16;
        break;

    case HAL_PIXEL_FORMAT_TI_NV12:
    case HAL_PIXEL_FORMAT_TI_NV12_1D:
        oc->color_mode = OMAP_DSS_COLOR_NV12;
        bits_per_pixel = 8;
        oc->cconv = ctbl_bt601_5;
        break;

    default:
        /* Should have been filtered out */
        ALOGV("Unsupported pixel format");
        return;
    }

    oc->width = width;
    oc->height = height;
    oc->stride = ALIGN(width, HW_ALIGN) * bits_per_pixel / 8;

    oc->enabled = 1;
    oc->global_alpha = 255;
    oc->zorder = index;
    oc->ix = 0;

    /* defaults for SGX framebuffer renders */
    oc->crop.w = oc->win.w = width;
    oc->crop.h = oc->win.h = height;

    /* for now interlacing and vc1 info is not supplied */
    oc->ilace = OMAP_DSS_ILACE_NONE;
    oc->vc1.enable = 0;
}

static void
omap4_hwc_setup_layer(omap4_hwc_device_t *hwc_dev, struct dss2_ovl_info *ovl,
                      hwc_layer_1_t *layer, int index,
                      int format, int width, int height)
{
    struct dss2_ovl_cfg *oc = &ovl->cfg;

    //dump_layer(layer);

    omap4_hwc_setup_layer_base(oc, index, format, is_BLENDED(layer), width, height);

    /* convert transformation - assuming 0-set config */
    if (layer->transform & HWC_TRANSFORM_FLIP_H)
        oc->mirror = 1;
    if (layer->transform & HWC_TRANSFORM_FLIP_V) {
        oc->rotation = 2;
        oc->mirror = !oc->mirror;
    }
    if (layer->transform & HWC_TRANSFORM_ROT_90) {
        oc->rotation += oc->mirror ? -1 : 1;
        oc->rotation &= 3;
    }

    oc->pre_mult_alpha = layer->blending == HWC_BLENDING_PREMULT;

    /* display position */
    oc->win.x = layer->displayFrame.left;
    oc->win.y = layer->displayFrame.top;
    oc->win.w = WIDTH(layer->displayFrame);
    oc->win.h = HEIGHT(layer->displayFrame);

    /* crop */
    oc->crop.x = layer->sourceCrop.left;
    oc->crop.y = layer->sourceCrop.top;
    oc->crop.w = WIDTH(layer->sourceCrop);
    oc->crop.h = HEIGHT(layer->sourceCrop);
}

const float m_unit[2][3] = { { 1., 0., 0. }, { 0., 1., 0. } };

static inline void m_translate(float m[2][3], int dx, int dy)
{
    m[0][2] += dx;
    m[1][2] += dy;
}

static inline void m_scale1(float m[3], int from, int to)
{
    m[0] = m[0] * to / from;
    m[1] = m[1] * to / from;
    m[2] = m[2] * to / from;
}

static inline void m_scale(float m[2][3], int x_from, int x_to, int y_from, int y_to)
{
    m_scale1(m[0], x_from, x_to);
    m_scale1(m[1], y_from, y_to);
}

static void m_rotate(float m[2][3], int quarter_turns)
{
    if (quarter_turns & 2)
        m_scale(m, 1, -1, 1, -1);
    if (quarter_turns & 1) {
        int q;
        q = m[0][0]; m[0][0] = -m[1][0]; m[1][0] = q;
        q = m[0][1]; m[0][1] = -m[1][1]; m[1][1] = q;
        q = m[0][2]; m[0][2] = -m[1][2]; m[1][2] = q;
    }
}

static inline int m_round(float x)
{
    /* int truncates towards 0 */
    return (int) (x < 0 ? x - 0.5 : x + 0.5);
}

/*
 * assuming xpy (xratio:yratio) original pixel ratio, calculate the adjusted width
 * and height for a screen of xres/yres and physical size of width/height.
 * The adjusted size is the largest that fits into the screen.
 */
static void get_max_dimensions(__u32 orig_xres, __u32 orig_yres,
                               float xpy,
                               __u32 scr_xres, __u32 scr_yres,
                               __u32 scr_width, __u32 scr_height,
                               __u32 *adj_xres, __u32 *adj_yres)
{
    /* assume full screen (largest size)*/
    *adj_xres = scr_xres;
    *adj_yres = scr_yres;

    /* assume 1:1 pixel ratios if none supplied */
    if (!scr_width || !scr_height) {
        scr_width = scr_xres;
        scr_height = scr_yres;
    }

    /* trim to keep aspect ratio */
    float x_factor = orig_xres * xpy * scr_height;
    float y_factor = orig_yres *       scr_width;

    /* allow for tolerance so we avoid scaling if framebuffer is standard size */
    if (x_factor < y_factor * (1.f - ASPECT_RATIO_TOLERANCE))
        *adj_xres = (__u32) (x_factor * *adj_xres / y_factor + 0.5);
    else if (x_factor * (1.f - ASPECT_RATIO_TOLERANCE) > y_factor)
        *adj_yres = (__u32) (y_factor * *adj_yres / x_factor + 0.5);
}

static void set_ext_matrix(omap4_hwc_ext_t *ext, struct hwc_rect region)
{
    int orig_w = WIDTH(region);
    int orig_h = HEIGHT(region);
    float xpy = ext->lcd_xpy;

    /* reorientation matrix is:
       m = (center-from-target-center) * (scale-to-target) * (mirror) * (rotate) * (center-to-original-center) */

    memcpy(ext->m, m_unit, sizeof(m_unit));
    m_translate(ext->m, -(orig_w >> 1) - region.left, -(orig_h >> 1) - region.top);
    m_rotate(ext->m, ext->current.rotation);
    if (ext->current.hflip)
        m_scale(ext->m, 1, -1, 1, 1);

    if (ext->current.rotation & 1) {
        swap(orig_w, orig_h);
        xpy = 1. / xpy;
    }

    /* get target size */
    __u32 adj_xres, adj_yres;
    get_max_dimensions(orig_w, orig_h, xpy,
                       ext->xres, ext->yres, ext->width, ext->height,
                       &adj_xres, &adj_yres);

    m_scale(ext->m, orig_w, adj_xres, orig_h, adj_yres);
    m_translate(ext->m, ext->xres >> 1, ext->yres >> 1);
}

static int
crop_to_rect(struct dss2_ovl_cfg *cfg, struct hwc_rect vis_rect)
{
    struct {
        int xy[2];
        int wh[2];
    } crop, win;
    struct {
        int lt[2];
        int rb[2];
    } vis;
    win.xy[0] = cfg->win.x; win.xy[1] = cfg->win.y;
    win.wh[0] = cfg->win.w; win.wh[1] = cfg->win.h;
    crop.xy[0] = cfg->crop.x; crop.xy[1] = cfg->crop.y;
    crop.wh[0] = cfg->crop.w; crop.wh[1] = cfg->crop.h;
    vis.lt[0] = vis_rect.left; vis.lt[1] = vis_rect.top;
    vis.rb[0] = vis_rect.right; vis.rb[1] = vis_rect.bottom;

    int c, swap = cfg->rotation & 1;

    /* align crop window with display coordinates */
    if (swap)
        crop.xy[1] -= (crop.wh[1] = -crop.wh[1]);
    if (cfg->rotation & 2)
        crop.xy[!swap] -= (crop.wh[!swap] = -crop.wh[!swap]);
    if ((!cfg->mirror) ^ !(cfg->rotation & 2))
        crop.xy[swap] -= (crop.wh[swap] = -crop.wh[swap]);

    for (c = 0; c < 2; c++) {
        /* see if complete buffer is outside the vis or it is
          fully cropped or scaled to 0 */
        if (win.wh[c] <= 0 || vis.rb[c] <= vis.lt[c] ||
            win.xy[c] + win.wh[c] <= vis.lt[c] ||
            win.xy[c] >= vis.rb[c] ||
            !crop.wh[c ^ swap])
            return -ENOENT;

        /* crop left/top */
        if (win.xy[c] < vis.lt[c]) {
            /* correction term */
            int a = (vis.lt[c] - win.xy[c]) * crop.wh[c ^ swap] / win.wh[c];
            crop.xy[c ^ swap] += a;
            crop.wh[c ^ swap] -= a;
            win.wh[c] -= vis.lt[c] - win.xy[c];
            win.xy[c] = vis.lt[c];
        }
        /* crop right/bottom */
        if (win.xy[c] + win.wh[c] > vis.rb[c]) {
            crop.wh[c ^ swap] = crop.wh[c ^ swap] * (vis.rb[c] - win.xy[c]) / win.wh[c];
            win.wh[c] = vis.rb[c] - win.xy[c];
        }

        if (!crop.wh[c ^ swap] || !win.wh[c])
            return -ENOENT;
    }

    /* realign crop window to buffer coordinates */
    if (cfg->rotation & 2)
        crop.xy[!swap] -= (crop.wh[!swap] = -crop.wh[!swap]);
    if ((!cfg->mirror) ^ !(cfg->rotation & 2))
        crop.xy[swap] -= (crop.wh[swap] = -crop.wh[swap]);
    if (swap)
        crop.xy[1] -= (crop.wh[1] = -crop.wh[1]);

    cfg->win.x = win.xy[0]; cfg->win.y = win.xy[1];
    cfg->win.w = win.wh[0]; cfg->win.h = win.wh[1];
    cfg->crop.x = crop.xy[0]; cfg->crop.y = crop.xy[1];
    cfg->crop.w = crop.wh[0]; cfg->crop.h = crop.wh[1];

    return 0;
}

static void
omap4_hwc_apply_transform(float transform[2][3],struct dss2_ovl_cfg *oc)
{
    float x, y, w, h;

    /* display position */
    x = transform[0][0] * oc->win.x + transform[0][1] * oc->win.y + transform[0][2];
    y = transform[1][0] * oc->win.x + transform[1][1] * oc->win.y + transform[1][2];
    w = transform[0][0] * oc->win.w + transform[0][1] * oc->win.h;
    h = transform[1][0] * oc->win.w + transform[1][1] * oc->win.h;
    oc->win.x = m_round(w > 0 ? x : x + w);
    oc->win.y = m_round(h > 0 ? y : y + h);
    oc->win.w = m_round(w > 0 ? w : -w);
    oc->win.h = m_round(h > 0 ? h : -h);
}

static void
omap4_hwc_adjust_ext_layer(omap4_hwc_ext_t *ext, struct dss2_ovl_info *ovl)
{
    struct dss2_ovl_cfg *oc = &ovl->cfg;

    /* crop to clone region if mirroring */
    if (!ext->current.docking &&
        crop_to_rect(&ovl->cfg, ext->mirror_region) != 0) {
        ovl->cfg.enabled = 0;
        return;
    }

    omap4_hwc_apply_transform(ext->m, oc);

    /* combining transformations: F^a*R^b*F^i*R^j = F^(a+b)*R^(j+b*(-1)^i), because F*R = R^(-1)*F */
    oc->rotation += (oc->mirror ? -1 : 1) * ext->current.rotation;
    oc->rotation &= 3;
    if (ext->current.hflip)
        oc->mirror = !oc->mirror;
}

static struct dsscomp_platform_info limits;

static void
omap4_hwc_adjust_primary_display_layer(omap4_hwc_device_t *hwc_dev, struct dss2_ovl_info *ovl)
{
    struct dss2_ovl_cfg *oc = &ovl->cfg;

    if (crop_to_rect(&ovl->cfg, hwc_dev->primary_region) != 0) {
        ovl->cfg.enabled = 0;
        return;
    }

    omap4_hwc_apply_transform(hwc_dev->primary_m, oc);

    /* combining transformations: F^a*R^b*F^i*R^j = F^(a+b)*R^(j+b*(-1)^i), because F*R = R^(-1)*F */
    oc->rotation += (oc->mirror ? -1 : 1) * hwc_dev->primary_rotation;
    oc->rotation &= 3;
}

static int omap4_hwc_can_scale(__u32 src_w, __u32 src_h, __u32 dst_w, __u32 dst_h, int is_2d,
                               struct dsscomp_display_info *dis, struct dsscomp_platform_info *limits,
                               __u32 pclk, void *handle)
{
    __u32 fclk = limits->fclk / 1000;
    __u32 min_src_w = DIV_ROUND_UP(src_w, is_2d ? limits->max_xdecim_2d : limits->max_xdecim_1d);
    __u32 min_src_h = DIV_ROUND_UP(src_h, is_2d ? limits->max_ydecim_2d : limits->max_ydecim_1d);

    /* ERRATAs */
    /* cannot render 1-width layers on DSI video mode panels - we just disallow all 1-width LCD layers */
    if (dis->channel != OMAP_DSS_CHANNEL_DIGIT && dst_w < limits->min_width)
        return 0;

    /* NOTE: no support for checking YUV422 layers that are tricky to scale */

    /* FIXME: limit vertical downscale well below theoretical limit as we saw display artifacts */
    if (dst_h < src_h / 4)
        return 0;

    /* max downscale */
    if (dst_h * limits->max_downscale < min_src_h)
        return 0;

    /* for manual panels pclk is 0, and there are no pclk based scaling limits */
    if (!pclk)
        return !(dst_w < src_w / limits->max_downscale / (is_2d ? limits->max_xdecim_2d : limits->max_xdecim_1d));

    /* :HACK: limit horizontal downscale well below theoretical limit as we saw display artifacts */
    if (dst_w * 4 < src_w)
        return 0;

    if (handle)
        if (get_rgb_bpp(handle) == 32 && src_w > 1280 && dst_w * 3 < src_w)
            return 0;

    /* max horizontal downscale is 4, or the fclk/pixclk */
    if (fclk > pclk * limits->max_downscale)
        fclk = pclk * limits->max_downscale;
    /* for small parts, we need to use integer fclk/pixclk */
    if (src_w < limits->integer_scale_ratio_limit)
        fclk = fclk / pclk * pclk;
    if ((__u32) dst_w * fclk < min_src_w * pclk)
        return 0;

    return 1;
}

static int omap4_hwc_can_scale_layer(omap4_hwc_device_t *hwc_dev, hwc_layer_1_t *layer, IMG_native_handle_t *handle)
{
    int src_w = WIDTH(layer->sourceCrop);
    int src_h = HEIGHT(layer->sourceCrop);
    int dst_w = WIDTH(layer->displayFrame);
    int dst_h = HEIGHT(layer->displayFrame);

    /* account for 90-degree rotation */
    if (layer->transform & HWC_TRANSFORM_ROT_90)
        swap(src_w, src_h);

    /* NOTE: layers should be able to be scaled externally since
       framebuffer is able to be scaled on selected external resolution */
    return omap4_hwc_can_scale(src_w, src_h, dst_w, dst_h, is_NV12(handle), &hwc_dev->fb_dis, &limits,
                               hwc_dev->fb_dis.timings.pixel_clock, handle);
}

static int omap4_hwc_is_valid_layer(omap4_hwc_device_t *hwc_dev,
                                    hwc_layer_1_t *layer,
                                    IMG_native_handle_t *handle)
{
    /* Skip layers are handled by SF */
    if ((layer->flags & HWC_SKIP_LAYER) || !handle)
        return 0;

    if (!omap4_hwc_is_valid_format(handle->iFormat))
        return 0;

    /* 1D buffers: no transform, must fit in TILER slot */
    if (!is_NV12(handle)) {
        if (layer->transform)
            return 0;
        if (mem1d(handle) > limits.tiler1d_slot_size)
            return 0;
    }

    return omap4_hwc_can_scale_layer(hwc_dev, layer, handle);
}

static __u32 add_scaling_score(__u32 score,
                               __u32 xres, __u32 yres, __u32 refresh,
                               __u32 ext_xres, __u32 ext_yres,
                               __u32 mode_xres, __u32 mode_yres, __u32 mode_refresh)
{
    __u32 area = xres * yres;
    __u32 ext_area = ext_xres * ext_yres;
    __u32 mode_area = mode_xres * mode_yres;

    /* prefer to upscale (1% tolerance) [0..1] (insert after 1st bit) */
    int upscale = (ext_xres >= xres * 99 / 100 && ext_yres >= yres * 99 / 100);
    score = (((score & ~1) | upscale) << 1) | (score & 1);

    /* pick minimum scaling [0..16] */
    if (ext_area > area)
        score = (score << 5) | (16 * area / ext_area);
    else
        score = (score << 5) | (16 * ext_area / area);

    /* pick smallest leftover area [0..16] */
    score = (score << 5) | ((16 * ext_area + (mode_area >> 1)) / mode_area);

    /* adjust mode refresh rate */
    mode_refresh += mode_refresh % 6 == 5;

    /* prefer same or higher frame rate */
    upscale = (mode_refresh >= refresh);
    score = (score << 1) | upscale;

    /* pick closest frame rate */
    if (mode_refresh > refresh)
        score = (score << 8) | (240 * refresh / mode_refresh);
    else
        score = (score << 8) | (240 * mode_refresh / refresh);

    return score;
}

static int omap4_hwc_set_best_hdmi_mode(omap4_hwc_device_t *hwc_dev, __u32 xres, __u32 yres,
                                        float xpy)
{
    int dis_ix = hwc_dev->on_tv ? 0 : 1;
    int forced_preferred_mode = 0;

    struct _qdis {
        struct dsscomp_display_info dis;
        struct dsscomp_videomode modedb[32];
    } d = { .dis = { .ix = dis_ix } };
    omap4_hwc_ext_t *ext = &hwc_dev->ext;

    d.dis.modedb_len = sizeof(d.modedb) / sizeof(*d.modedb);
    int ret = ioctl(hwc_dev->dsscomp_fd, DSSCIOC_QUERY_DISPLAY, &d);
    if (ret)
        return ret;

    if (d.dis.timings.x_res * d.dis.timings.y_res == 0 ||
        xres * yres == 0)
        return -EINVAL;

    __u32 i, best = ~0, best_score = 0;
    ext->width = d.dis.width_in_mm;
    ext->height = d.dis.height_in_mm;
    ext->xres = d.dis.timings.x_res;
    ext->yres = d.dis.timings.y_res;

    /* use VGA external resolution as default */
    if (!ext->xres || !ext->yres) {
        ext->xres = 640;
        ext->yres = 480;
    }

    /*
     * copy the xres/yres from the preferred mode
     */
    __u32 preferred_mode_xres = 0;
    __u32 preferred_mode_yres = 0;

    char value[PROPERTY_VALUE_MAX];
    if (property_get("persist.hwc.preferred_mode", value, "") <= 0 ||
        sscanf(value, "%dx%d", &preferred_mode_xres,
               &preferred_mode_yres) != 2) {
        for (i = 0; i < d.dis.modedb_len; i++) {
            if (d.modedb[i].flag & FB_FLAG_PREFERRED) {
                preferred_mode_xres = d.modedb[i].xres;
                preferred_mode_yres = d.modedb[i].yres;
                ALOGD("preferred mode %d: xres %u yres %u\n",
                    i, d.modedb[i].xres, d.modedb[i].yres);
                break;
            }
        }
    } else {
        ALOGD("forced preferred mode xres %u yres %u\n",
             preferred_mode_xres, preferred_mode_yres);
        forced_preferred_mode = 1;
    }

    __u32 ext_fb_xres, ext_fb_yres;
    for (i = 0; i < d.dis.modedb_len; i++) {
        __u32 score = 0;
        __u32 mode_xres = d.modedb[i].xres;
        __u32 mode_yres = d.modedb[i].yres;
        __u32 ext_width = d.dis.width_in_mm;
        __u32 ext_height = d.dis.height_in_mm;

        /* reject it because the hw says it can't actually use this mode */
        if ((d.modedb[i].flag & FB_FLAG_HW_CAPABLE) == 0)
            continue;

        if (d.modedb[i].vmode & FB_VMODE_INTERLACED)
            mode_yres /= 2;

        if (d.modedb[i].flag & FB_FLAG_RATIO_4_3) {
            ext_width = 4;
            ext_height = 3;
        } else if (d.modedb[i].flag & FB_FLAG_RATIO_16_9) {
            ext_width = 16;
            ext_height = 9;
        }

        if (!mode_xres || !mode_yres)
            continue;

        get_max_dimensions(xres, yres, xpy, mode_xres, mode_yres,
                           ext_width, ext_height, &ext_fb_xres, &ext_fb_yres);

        /* we need to ensure that even TILER2D buffers can be scaled */
        if (!d.modedb[i].pixclock ||
            (d.modedb[i].vmode & ~FB_VMODE_INTERLACED) ||
            !omap4_hwc_can_scale(xres, yres, ext_fb_xres, ext_fb_yres,
                                 1, &d.dis, &limits,
                                 1000000000 / d.modedb[i].pixclock, NULL))
            continue;

        /* prefer CEA modes */
        if (d.modedb[i].flag & (FB_FLAG_RATIO_4_3 | FB_FLAG_RATIO_16_9))
            score += 1;

        /* prefer modes that match the preferred mode's resolution */
        if (d.modedb[i].xres == preferred_mode_xres &&
            d.modedb[i].yres == preferred_mode_yres) {
            if (forced_preferred_mode)
                score += 10;
            else
                score += 1;
        }

        /* prefer modes the kernel has hinted is the correct mode */
        if (!forced_preferred_mode && (d.modedb[i].flag & FB_FLAG_PREFERRED))
            score += 1;

        /* prefer the same mode as we use for mirroring to avoid mode change */
        score = (score << 1) | (i == ~ext->mirror_mode && ext->avoid_mode_change);

        score = add_scaling_score(score, xres, yres, 60, ext_fb_xres, ext_fb_yres,
                                  mode_xres, mode_yres, d.modedb[i].refresh ? : 1);

        ALOGD("#%d: %dx%d %dHz flag 0x%x vmode 0x%x, score 0x%x",
             i, mode_xres, mode_yres,
             d.modedb[i].refresh, d.modedb[i].flag, d.modedb[i].vmode,
             score);

        if (debug)
            ALOGD("  score=0x%x adj.res=%dx%d", score, ext_fb_xres, ext_fb_yres);
        if (best_score < score) {
            ext->width = ext_width;
            ext->height = ext_height;
            ext->xres = mode_xres;
            ext->yres = mode_yres;
            best = i;
            best_score = score;
        }
    }
    if (~best) {
        struct dsscomp_setup_display_data sdis = { .ix = dis_ix, };
        sdis.mode = d.dis.modedb[best];
        ALOGD("picking #%d", best);
        /* only reconfigure on change */
        if (ext->last_mode != ~best) {
            /* set a property that apps that care (e.g. YouTube) can use
             * to determine whether or not to stream lower resolution
             * videos when the hdmi mode is < 1080p.
             * otherwise, they'd give us 1080p and we'd just scale it
             * down to the hdmi mode res.  UI apps are always going
             * to draw at 1080p and we'll scale down because the
             * system can't support dynamic dpi changes.
             */
            char display[PROPERTY_VALUE_MAX];
            snprintf(display, sizeof(display), "%dx%d",
                     d.modedb[best].xres, d.modedb[best].yres);
            ALOGD("setting property sys.display-size to %s", display);
            property_set("sys.display-size", display);

            ioctl(hwc_dev->dsscomp_fd, DSSCIOC_SETUP_DISPLAY, &sdis);
        }
        ext->last_mode = ~best;
    } else {
        __u32 ext_width = d.dis.width_in_mm;
        __u32 ext_height = d.dis.height_in_mm;
        __u32 ext_fb_xres, ext_fb_yres;

        get_max_dimensions(xres, yres, xpy, d.dis.timings.x_res, d.dis.timings.y_res,
                           ext_width, ext_height, &ext_fb_xres, &ext_fb_yres);
        if (!d.dis.timings.pixel_clock ||
            !omap4_hwc_can_scale(xres, yres, ext_fb_xres, ext_fb_yres,
                                 1, &d.dis, &limits,
                                 d.dis.timings.pixel_clock, NULL)) {
            ALOGW("DSS scaler cannot support HDMI cloning");
            return -1;
        }
    }
    ext->last_xres_used = xres;
    ext->last_yres_used = yres;
    ext->last_xpy = xpy;
    if (d.dis.channel == OMAP_DSS_CHANNEL_DIGIT)
        ext->on_tv = 1;
    return 0;
}

static void gather_layer_statistics(omap4_hwc_device_t *hwc_dev, struct counts *num, hwc_display_contents_1_t *list)
{
    unsigned int i;

    /* Figure out how many layers we can support via DSS */
    for (i = 0; list && i < list->numHwLayers; i++) {
        hwc_layer_1_t *layer = &list->hwLayers[i];
        IMG_native_handle_t *handle = (IMG_native_handle_t *)layer->handle;

        layer->compositionType = HWC_FRAMEBUFFER;

        if (omap4_hwc_is_valid_layer(hwc_dev, layer, handle)) {
            num->possible_overlay_layers++;

            /* NV12 layers can only be rendered on scaling overlays */
            if (scaled(layer) || is_NV12(handle) || hwc_dev->primary_transform)
                num->scaled_layers++;

            if (is_BGR(handle))
                num->BGR++;
            else if (is_RGB(handle))
                num->RGB++;
            else if (is_NV12(handle))
                num->NV12++;

            if (dockable(layer))
                num->dockable++;

            if (is_protected(layer))
                num->protected++;

            num->mem += mem1d(handle);

            if (is_large_rgb32_layer(layer))
                num->large_rgb32_layers++;
        }
    }
    hwc_dev->stats = *num;
}

static void decide_supported_cloning(omap4_hwc_device_t *hwc_dev, struct counts *num)
{
    omap4_hwc_ext_t *ext = &hwc_dev->ext;
    int nonscaling_ovls = NUM_NONSCALING_OVERLAYS;
    num->max_hw_overlays = MAX_HW_OVERLAYS;

    /*
     * We cannot atomically switch overlays from one display to another.  First, they
     * have to be disabled, and the disabling has to take effect on the current display.
     * We keep track of the available number of overlays here.
     */
    if (ext->dock.enabled && !(ext->mirror.enabled && !(num->dockable || ext->force_dock))) {
        /* some overlays may already be used by the external display, so we account for this */

        /* reserve just a video pipeline for HDMI if docking */
        hwc_dev->ext_ovls = (num->dockable || ext->force_dock) ? 1 : 0;
        num->max_hw_overlays -= max(hwc_dev->ext_ovls, hwc_dev->last_ext_ovls);

        /* use mirroring transform if we are auto-switching to docking mode while mirroring*/
        if (ext->mirror.enabled) {
            ext->current = ext->mirror;
            ext->current.docking = 1;
        } else {
            ext->current = ext->dock;
        }
    } else if (ext->mirror.enabled) {
        /*
         * otherwise, manage just from half the pipelines.  NOTE: there is
         * no danger of having used too many overlays for external display here.
         */

        num->max_hw_overlays >>= 1;
        nonscaling_ovls >>= 1;
        hwc_dev->ext_ovls = MAX_HW_OVERLAYS - num->max_hw_overlays;
        ext->current = ext->mirror;
    } else {
        num->max_hw_overlays -= hwc_dev->last_ext_ovls;
        hwc_dev->ext_ovls = 0;
        ext->current.enabled = 0;
    }

    /*
     * :TRICKY: We may not have enough overlays on the external display.  We "reserve" them
     * here to figure out if mirroring is supported, but may not do mirroring for the first
     * frame while the overlays required for it are cleared.
     */
    hwc_dev->ext_ovls_wanted = hwc_dev->ext_ovls;
    hwc_dev->ext_ovls = min(MAX_HW_OVERLAYS - hwc_dev->last_int_ovls, hwc_dev->ext_ovls);

    /* if mirroring, we are limited by both internal and external overlays.  However,
       ext_ovls is always <= MAX_HW_OVERLAYS / 2 <= max_hw_overlays */
    if (!num->protected && hwc_dev->ext_ovls && ext->current.enabled && !ext->current.docking)
        num->max_hw_overlays = hwc_dev->ext_ovls;

    /* If FB is not same resolution as LCD don't use GFX pipe line*/
    if (hwc_dev->primary_transform) {
        num->max_hw_overlays -= NUM_NONSCALING_OVERLAYS;
        num->max_scaling_overlays = num->max_hw_overlays;
    } else
        num->max_scaling_overlays = num->max_hw_overlays - nonscaling_ovls;
}

/* how many large layers can be composited by the DSS */
static const unsigned int MAX_DSS_LARGE_LAYERS = 4;

static int can_dss_render_all(omap4_hwc_device_t *hwc_dev, struct counts *num)
{
    omap4_hwc_ext_t *ext = &hwc_dev->ext;
    int on_tv = hwc_dev->on_tv || (ext->on_tv && ext->current.enabled);
    int tform = ext->current.enabled && (ext->current.rotation || ext->current.hflip);

    return  !hwc_dev->force_sgx &&
            /* must have at least one layer if using composition bypass to get sync object */
            num->possible_overlay_layers &&
            num->possible_overlay_layers <= num->max_hw_overlays &&
            num->possible_overlay_layers == num->composited_layers &&
            num->scaled_layers <= num->max_scaling_overlays &&
            num->NV12 <= num->max_scaling_overlays &&
            /* fits into TILER slot */
            num->mem <= limits.tiler1d_slot_size &&
            /* we cannot clone non-NV12 transformed layers */
            (!tform || (num->NV12 == num->possible_overlay_layers) ||
            (num->NV12 && ext->current.docking)) &&
            /* HDMI cannot display BGR */
            (num->BGR == 0 || (num->RGB == 0 && !on_tv) || !hwc_dev->flags_rgb_order) &&
            /* current hardware can only handle a limited number of 'large' RGB32 layers */
            num->large_rgb32_layers <= MAX_DSS_LARGE_LAYERS;
}

static inline int can_dss_render_layer(omap4_hwc_device_t *hwc_dev,
            hwc_layer_1_t *layer)
{
    IMG_native_handle_t *handle = (IMG_native_handle_t *)layer->handle;

    omap4_hwc_ext_t *ext = &hwc_dev->ext;
    int cloning = ext->current.enabled && (!ext->current.docking || (handle!=NULL ? dockable(layer) : 0));
    int on_tv = hwc_dev->on_tv || (ext->on_tv && cloning);
    int tform = cloning && (ext->current.rotation || ext->current.hflip);

    return omap4_hwc_is_valid_layer(hwc_dev, layer, handle) &&
           /* cannot rotate non-NV12 layers on external display */
           (!tform || is_NV12(handle)) &&
           /* skip non-NV12 layers if also using SGX (if nv12_only flag is set) */
           (!hwc_dev->flags_nv12_only || (!hwc_dev->use_sgx || is_NV12(handle))) &&
           /* make sure RGB ordering is consistent (if rgb_order flag is set) */
           (!(hwc_dev->swap_rb ? is_RGB(handle) : is_BGR(handle)) ||
            !hwc_dev->flags_rgb_order) &&
           /* TV can only render RGB */
           !(on_tv && is_BGR(handle));
}

static inline int display_area(struct dss2_ovl_info *o)
{
    return o->cfg.win.w * o->cfg.win.h;
}

static int clone_layer(omap4_hwc_device_t *hwc_dev, int ix) {
    struct dsscomp_setup_dispc_data *dsscomp = &hwc_dev->comp_data.dsscomp_data;
    omap4_hwc_ext_t *ext = &hwc_dev->ext;
    int ext_ovl_ix = dsscomp->num_ovls - hwc_dev->post2_layers;
    struct dss2_ovl_info *o = &dsscomp->ovls[dsscomp->num_ovls];

    if (dsscomp->num_ovls >= MAX_HW_OVERLAYS) {
        ALOGE("**** cannot clone layer #%d. using all %d overlays.", ix, dsscomp->num_ovls);
        return -EBUSY;
    }

    memcpy(o, dsscomp->ovls + ix, sizeof(*o));

    /* reserve overlays at end for other display */
    o->cfg.ix = MAX_HW_OVERLAYS - 1 - ext_ovl_ix;
    o->cfg.mgr_ix = 1;
    /*
    * Here the assumption is that overlay0 is the one attached to FB.
    * Hence this clone_layer call is for FB cloning (provided use_sgx is true).
    */
    /* For the external displays whose transform is the same as
    * that of primary display, ion_handles would be NULL hence
    * the below logic doesn't execute.
    */
    if (ix == 0 && hwc_dev->ion_handles[sync_id%2] && hwc_dev->use_sgx) {
        o->addressing = OMAP_DSS_BUFADDR_ION;
        o->ba = (int)hwc_dev->ion_handles[sync_id%2];
    } else {
        o->addressing = OMAP_DSS_BUFADDR_OVL_IX;
        o->ba = ix;
    }

    /* use distinct z values (to simplify z-order checking) */
    o->cfg.zorder += hwc_dev->post2_layers;

    omap4_hwc_adjust_ext_layer(&hwc_dev->ext, o);
    dsscomp->num_ovls++;
    return 0;
}

static int clone_external_layer(omap4_hwc_device_t *hwc_dev, int ix) {
    struct dsscomp_setup_dispc_data *dsscomp = &hwc_dev->comp_data.dsscomp_data;
    omap4_hwc_ext_t *ext = &hwc_dev->ext;

    /* mirror only 1 external layer */
    struct dss2_ovl_info *o = &dsscomp->ovls[ix];

    /* full screen video after transformation */
    __u32 xres = o->cfg.crop.w, yres = o->cfg.crop.h;
    if ((ext->current.rotation + o->cfg.rotation) & 1)
        swap(xres, yres);
    float xpy = ext->lcd_xpy * o->cfg.win.w / o->cfg.win.h;
    if (o->cfg.rotation & 1)
        xpy = o->cfg.crop.h / xpy / o->cfg.crop.w;
    else
        xpy = o->cfg.crop.h * xpy / o->cfg.crop.w;
    if (ext->current.rotation & 1)
        xpy = 1. / xpy;

    /* adjust hdmi mode based on resolution */
    if (xres != ext->last_xres_used ||
        yres != ext->last_yres_used ||
        xpy < ext->last_xpy * (1.f - ASPECT_RATIO_TOLERANCE) ||
        xpy * (1.f - ASPECT_RATIO_TOLERANCE) > ext->last_xpy) {
        ALOGD("set up HDMI for %d*%d\n", xres, yres);
        if (omap4_hwc_set_best_hdmi_mode(hwc_dev, xres, yres, xpy)) {
            ext->current.enabled = 0;
            return -ENODEV;
        }
    }

    struct hwc_rect region = {
        .left = o->cfg.win.x, .top = o->cfg.win.y,
        .right = o->cfg.win.x + o->cfg.win.w,
        .bottom = o->cfg.win.y + o->cfg.win.h
    };
    set_ext_matrix(&hwc_dev->ext, region);

    return clone_layer(hwc_dev, ix);
}

static int setup_mirroring(omap4_hwc_device_t *hwc_dev)
{
    omap4_hwc_ext_t *ext = &hwc_dev->ext;

    __u32 xres = WIDTH(ext->mirror_region);
    __u32 yres = HEIGHT(ext->mirror_region);
    if (ext->current.rotation & 1)
       swap(xres, yres);
    if (omap4_hwc_set_best_hdmi_mode(hwc_dev, xres, yres, ext->lcd_xpy))
        return -ENODEV;
    set_ext_matrix(ext, ext->mirror_region);
    return 0;
}

/*
 * We're using "implicit" synchronization, so make sure we aren't passing any
 * sync object descriptors around.
 */
static void check_sync_fds(size_t numDisplays, hwc_display_contents_1_t** displays)
{
    //ALOGD("checking sync FDs");
    unsigned int i, j;
    for (i = 0; i < numDisplays; i++) {
        hwc_display_contents_1_t* list = displays[i];
        if (list->retireFenceFd >= 0) {
            ALOGW("retireFenceFd[%u] was %d", i, list->retireFenceFd);
            list->retireFenceFd = -1;
        }

        for (j = 0; j < list->numHwLayers; j++) {
            hwc_layer_1_t* layer = &list->hwLayers[j];
            if (layer->acquireFenceFd >= 0) {
                ALOGW("acquireFenceFd[%u][%u] was %d, closing", i, j, layer->acquireFenceFd);
                close(layer->acquireFenceFd);
                layer->acquireFenceFd = -1;
            }
            if (layer->releaseFenceFd >= 0) {
                ALOGW("releaseFenceFd[%u][%u] was %d", i, j, layer->releaseFenceFd);
                layer->releaseFenceFd = -1;
            }
        }
    }
}

static void blit_reset(omap4_hwc_device_t *hwc_dev, int flags)
{
    hwc_dev->blit_flags = 0;
    hwc_dev->blit_num = 0;
    hwc_dev->post2_blit_buffers = 0;
    hwc_dev->comp_data.blit_data.rgz_items = 0;

    /* We want to maintain the rgz dirty region data if there are no geometry changes */
    if (flags & HWC_GEOMETRY_CHANGED)
        rgz_release(&grgz);
}

static int blit_layers(omap4_hwc_device_t *hwc_dev, hwc_display_contents_1_t *list, int bufoff)
{
    /* Do not blit if this frame will be composed entirely by the GPU */
    if (!list || hwc_dev->force_sgx)
        goto err_out;

    int rgz_in_op;
    int rgz_out_op;

    switch (hwc_dev->blt_mode) {
        case BLTMODE_PAINT:
            rgz_in_op = RGZ_IN_HWCCHK;
            rgz_out_op = RGZ_OUT_BVCMD_PAINT;
            break;
        case BLTMODE_REGION:
        default:
            rgz_in_op = RGZ_IN_HWC;
            rgz_out_op = RGZ_OUT_BVCMD_REGION;
            break;
    }

    rgz_in_params_t in = {
        .op = rgz_in_op,
        .data = {
            .hwc = {
                .dstgeom = &gscrngeom,
                .layers = list->hwLayers,
                .layerno = list->numHwLayers
            }
        }
    };

    /*
     * This means if all the layers marked for the FRAMEBUFFER cannot be
     * blitted, do not blit, for e.g. SKIP layers
     */
    if (rgz_in(&in, &grgz) != RGZ_ALL)
        goto err_out;

    unsigned int i, count = 0;
    for (i = 0; i < list->numHwLayers; i++) {
        if (list->hwLayers[i].compositionType != HWC_OVERLAY) {
            count++;
        }
    }

    rgz_out_params_t out = {
        .op = rgz_out_op,
        .data = {
            .bvc = {
                .dstgeom = &gscrngeom,
                .noblend = 0,
            }
        }
    };

    if (rgz_out(&grgz, &out) != 0) {
        ALOGE("Failed generating blits");
        goto err_out;
    }

    /* This is a special situation where the regionizer decided no blits are
     * needed for this frame but there are blit buffers to synchronize with. Can
     * happen only if the regionizer is enabled otherwise it's likely a bug
     */
    if (rgz_out_op != RGZ_OUT_BVCMD_REGION && out.data.bvc.out_blits == 0 && out.data.bvc.out_nhndls > 0) {
        ALOGE("Regionizer invalid output blit_num %d, post2_blit_buffers %d", out.data.bvc.out_blits, out.data.bvc.out_nhndls);
        goto err_out;
    }

    hwc_dev->blit_flags |= HWC_BLT_FLAG_USE_FB;
    hwc_dev->blit_num = out.data.bvc.out_blits;
    hwc_dev->post2_blit_buffers = out.data.bvc.out_nhndls;
    for (i = 0; i < hwc_dev->post2_blit_buffers; i++) {
        //ALOGI("blit buffers[%d] = %p", bufoff, out.data.bvc.out_hndls[i]);
        hwc_dev->buffers[bufoff++] = out.data.bvc.out_hndls[i];
    }

    struct rgz_blt_entry *res_blit_ops = (struct rgz_blt_entry *) out.data.bvc.cmdp;
    memcpy(hwc_dev->comp_data.blit_data.rgz_blts, res_blit_ops, sizeof(*res_blit_ops) * out.data.bvc.cmdlen);
    ALOGI_IF(debugblt, "blt struct sz %d", sizeof(*res_blit_ops) * out.data.bvc.cmdlen);
    ALOGE_IF(hwc_dev->blit_num != out.data.bvc.cmdlen,"blit_num != out.data.bvc.cmdlen, %d != %d", hwc_dev->blit_num, out.data.bvc.cmdlen);

    /* all layers will be rendered without SGX help either via DSS or blitter */
    for (i = 0; i < list->numHwLayers; i++) {
        if (list->hwLayers[i].compositionType != HWC_OVERLAY) {
            list->hwLayers[i].compositionType = HWC_OVERLAY;
            //ALOGI("blitting layer %d", i);
            list->hwLayers[i].hints &= ~HWC_HINT_TRIPLE_BUFFER;
        }
        list->hwLayers[i].hints &= ~HWC_HINT_CLEAR_FB;
    }
    return 1;

err_out:
    rgz_release(&grgz);
    return 0;
}

void debug_post2(omap4_hwc_device_t *hwc_dev, int nbufs)
{
    if (!debugpost2)
        return;
    struct dsscomp_setup_dispc_data *dsscomp = &hwc_dev->comp_data.dsscomp_data;
    int i;
    for (i=0; i<nbufs; i++) {
        ALOGI("buf[%d] hndl %p", i, hwc_dev->buffers[i]);
    }
    for (i=0; i < dsscomp->num_ovls; i++) {
        ALOGI("ovl[%d] ba %d", i, dsscomp->ovls[i].ba);
    }
}

static int free_tiler2d_buffers(omap4_hwc_device_t *hwc_dev)
{
    int i;

    for (i = 0 ; i < NUM_EXT_DISPLAY_BACK_BUFFERS; i++) {
        ion_free(hwc_dev->ion_fd, hwc_dev->ion_handles[i]);
        hwc_dev->ion_handles[i] = NULL;
    }
    return 0;
}

static int allocate_tiler2d_buffers(omap4_hwc_device_t *hwc_dev)
{
    int ret, i;
    size_t stride;

    if (hwc_dev->ion_fd < 0) {
        ALOGE("No ion fd, hence can't allocate tiler2d buffers");
        return -1;
    }

    for (i = 0; i < NUM_EXT_DISPLAY_BACK_BUFFERS; i++) {
        if (hwc_dev->ion_handles[i])
            return 0;
    }

    for (i = 0 ; i < NUM_EXT_DISPLAY_BACK_BUFFERS; i++) {
        ret = ion_alloc_tiler(hwc_dev->ion_fd, hwc_dev->fb_dev->base.width, hwc_dev->fb_dev->base.height,
                                            TILER_PIXEL_FMT_32BIT, 0, &hwc_dev->ion_handles[i], &stride);
        if (ret)
            goto handle_error;

        ALOGI("ion handle[%d][%p]", i, hwc_dev->ion_handles[i]);
    }
    return 0;

handle_error:
    free_tiler2d_buffers(hwc_dev);
    return -1;
}

static int omap4_hwc_prepare(struct hwc_composer_device_1 *dev, size_t numDisplays,
        hwc_display_contents_1_t** displays)
{
    if (!numDisplays || displays == NULL) {
        return 0;
    }

    hwc_display_contents_1_t* list = displays[0];  // ignore displays beyond the first
    omap4_hwc_device_t *hwc_dev = (omap4_hwc_device_t *)dev;
    struct dsscomp_setup_dispc_data *dsscomp = &hwc_dev->comp_data.dsscomp_data;
    struct counts num = { .composited_layers = list ? list->numHwLayers : 0 };
    unsigned int i, ix;

    pthread_mutex_lock(&hwc_dev->lock);
    memset(dsscomp, 0x0, sizeof(*dsscomp));
    dsscomp->sync_id = sync_id++;

    gather_layer_statistics(hwc_dev, &num, list);

    decide_supported_cloning(hwc_dev, &num);

    /* phase 3 logic */
    if (can_dss_render_all(hwc_dev, &num)) {
        /* All layers can be handled by the DSS -- don't use SGX for composition */
        hwc_dev->use_sgx = 0;
        hwc_dev->swap_rb = num.BGR != 0;
    } else {
        /* Use SGX for composition plus first 3 layers that are DSS renderable */
        hwc_dev->use_sgx = 1;
        hwc_dev->swap_rb = is_BGR_format(hwc_dev->fb_dev->base.format);
    }

    /* setup pipes */
    int z = 0;
    int fb_z = -1;
    int scaled_gfx = 0;
    int ix_docking = -1;
    unsigned int big_layers = 0;

    int blit_all = 0;
    blit_reset(hwc_dev, list ? list->flags : 0);

    /* If the SGX is used or we are going to blit something we need a framebuffer
     * and a DSS pipe
     */
    int needs_fb = hwc_dev->use_sgx;

    if (hwc_dev->blt_policy == BLTPOLICY_ALL) {
        /* Check if we can blit everything */
        blit_all = blit_layers(hwc_dev, list, 0);
        if (blit_all) {
            needs_fb = 1;
            hwc_dev->use_sgx = 0;
        }
    }

    /* If a framebuffer is needed, begin using VID1 for DSS overlay layers,
     * we need GFX for FB
     */
    dsscomp->num_ovls = needs_fb ? 1 /*VID1*/ : 0 /*GFX*/;

    /* set up if DSS layers */
    unsigned int mem_used = 0;
    for (i = 0; list && i < list->numHwLayers && !blit_all; i++) {
        hwc_layer_1_t *layer = &list->hwLayers[i];
        IMG_native_handle_t *handle = (IMG_native_handle_t *)layer->handle;

        if (dsscomp->num_ovls < num.max_hw_overlays &&
            can_dss_render_layer(hwc_dev, layer) &&
            (!hwc_dev->force_sgx ||
             /* render protected and dockable layers via DSS */
             is_protected(layer) ||
             is_upscaled_NV12(hwc_dev, layer) ||
             (hwc_dev->ext.current.docking && hwc_dev->ext.current.enabled && dockable(layer))) &&
            mem_used + mem1d(handle) <= limits.tiler1d_slot_size &&
            /* can't have a transparent overlay in the middle of the framebuffer stack */
            !(is_BLENDED(layer) && fb_z >= 0) &&
            /* current hardware can only handle a limited number of 'large' RGB32 layers */
            !(is_large_rgb32_layer(layer) && big_layers >= MAX_DSS_LARGE_LAYERS)) {

            /* render via DSS overlay */
            mem_used += mem1d(handle);
            layer->compositionType = HWC_OVERLAY;
            /*
             * This hint will not be used in vanilla ICS, but maybe in
             * JellyBean, it is useful to distinguish between blts and true
             * overlays
             */
            layer->hints |= HWC_HINT_TRIPLE_BUFFER;

            /* clear FB above all opaque layers if rendering via SGX */
            if (hwc_dev->use_sgx && !is_BLENDED(layer))
                layer->hints |= HWC_HINT_CLEAR_FB;

            hwc_dev->buffers[dsscomp->num_ovls] = layer->handle;

            omap4_hwc_setup_layer(hwc_dev,
                                  &dsscomp->ovls[dsscomp->num_ovls],
                                  layer,
                                  z,
                                  handle->iFormat,
                                  handle->iWidth,
                                  handle->iHeight);

            dsscomp->ovls[dsscomp->num_ovls].cfg.ix = dsscomp->num_ovls + hwc_dev->primary_transform;
            dsscomp->ovls[dsscomp->num_ovls].addressing = OMAP_DSS_BUFADDR_LAYER_IX;
            dsscomp->ovls[dsscomp->num_ovls].ba = dsscomp->num_ovls;

            /* ensure GFX layer is never scaled */
            if ((dsscomp->num_ovls == 0) && (!hwc_dev->primary_transform)) {
                scaled_gfx = scaled(layer) || is_NV12(handle);
            } else if (scaled_gfx && !scaled(layer) && !is_NV12(handle)) {
                /* swap GFX layer with this one */
                dsscomp->ovls[dsscomp->num_ovls].cfg.ix = 0;
                dsscomp->ovls[0].cfg.ix = dsscomp->num_ovls;
                scaled_gfx = 0;
            }

            /* remember largest dockable layer */
            if (dockable(layer) &&
                (ix_docking < 0 ||
                 display_area(&dsscomp->ovls[dsscomp->num_ovls]) > display_area(&dsscomp->ovls[ix_docking])))
                ix_docking = dsscomp->num_ovls;

            dsscomp->num_ovls++;
            z++;

            /* record whether or not this was a 'big' RGB32 layer */
            if (is_large_rgb32_layer(layer)) {
                big_layers++;
            }
        } else if (hwc_dev->use_sgx) {
            if (fb_z < 0) {
                /* NOTE: we are not handling transparent cutout for now */
                fb_z = z;
                z++;
            } else {
                /* move fb z-order up (by lowering dss layers) */
                while (fb_z < z - 1)
                    dsscomp->ovls[1 + fb_z++].cfg.zorder--;
            }
        }
    }

    /* if scaling GFX (e.g. only 1 scaled surface) use a VID pipe */
    if (scaled_gfx)
        dsscomp->ovls[0].cfg.ix = dsscomp->num_ovls;

    if (hwc_dev->blt_policy == BLTPOLICY_DEFAULT) {
        if (hwc_dev->use_sgx) {
            if (blit_layers(hwc_dev, list, dsscomp->num_ovls == 1 ? 0 : dsscomp->num_ovls)) {
                hwc_dev->use_sgx = 0;
            }
        }
    }

    /* If the SGX is not used and there is blit data we need a framebuffer and
     * a DSS pipe well configured for it
     */
    if (needs_fb) {
        /* assign a z-layer for fb */
        if (fb_z < 0) {
            if (!hwc_dev->blt_policy != BLTPOLICY_DISABLED && num.composited_layers)
                ALOGE("**** should have assigned z-layer for fb");
            fb_z = z++;
        }
        /*
         * This is needed because if we blit all we would lose the handle of
         * the first layer
         */
        if (hwc_dev->use_sgx) {
            hwc_dev->buffers[0] = NULL;
        }
        omap4_hwc_setup_layer_base(&dsscomp->ovls[0].cfg, fb_z,
                                   hwc_dev->fb_dev->base.format,
                                   1,   /* FB is always premultiplied */
                                   hwc_dev->fb_dev->base.width,
                                   hwc_dev->fb_dev->base.height);
        dsscomp->ovls[0].cfg.pre_mult_alpha = 1;
        dsscomp->ovls[0].addressing = OMAP_DSS_BUFADDR_LAYER_IX;
        dsscomp->ovls[0].ba = 0;
        dsscomp->ovls[0].cfg.ix = hwc_dev->primary_transform;
    }

    /* mirror layers */
    hwc_dev->post2_layers = dsscomp->num_ovls;

    omap4_hwc_ext_t *ext = &hwc_dev->ext;
    if (ext->current.enabled && hwc_dev->ext_ovls) {
        if (ext->current.docking && ix_docking >= 0) {
            if (clone_external_layer(hwc_dev, ix_docking) == 0)
                dsscomp->ovls[dsscomp->num_ovls - 1].cfg.zorder = z++;
        } else if (ext->current.docking && ix_docking < 0 && ext->force_dock) {
            ix_docking = dsscomp->num_ovls;
            struct dss2_ovl_info *oi = &dsscomp->ovls[ix_docking];
            omap4_hwc_setup_layer_base(&oi->cfg, 0, HAL_PIXEL_FORMAT_BGRA_8888, 1,
                                       dock_image.width, dock_image.height);
            oi->cfg.stride = dock_image.rowbytes;
            if (clone_external_layer(hwc_dev, ix_docking) == 0) {
                oi->addressing = OMAP_DSS_BUFADDR_FB;
                oi->ba = 0;
                z++;
            }
        } else if (!ext->current.docking) {
            int res = 0;

            /* reset mode if we are coming from docking */
            if (ext->last.docking)
                res = setup_mirroring(hwc_dev);

            /* mirror all layers */
            for (ix = 0; res == 0 && ix < hwc_dev->post2_layers; ix++) {
                if (clone_layer(hwc_dev, ix))
                    break;
                z++;
            }
        }
    }

    /* Apply transform for primary display */
    if (hwc_dev->primary_transform)
        for (i = 0; i < dsscomp->num_ovls; i++) {
            if(dsscomp->ovls[i].cfg.mgr_ix == 0)
                omap4_hwc_adjust_primary_display_layer(hwc_dev, &dsscomp->ovls[i]);
        }

    ext->last = ext->current;

    if (z != dsscomp->num_ovls || dsscomp->num_ovls > MAX_HW_OVERLAYS)
        ALOGE("**** used %d z-layers for %d overlays\n", z, dsscomp->num_ovls);

    /* verify all z-orders and overlay indices are distinct */
    for (i = z = ix = 0; i < dsscomp->num_ovls; i++) {
        struct dss2_ovl_cfg *c = &dsscomp->ovls[i].cfg;

        if (z & (1 << c->zorder))
            ALOGE("**** used z-order #%d multiple times", c->zorder);
        if (ix & (1 << c->ix))
            ALOGE("**** used ovl index #%d multiple times", c->ix);
        z |= 1 << c->zorder;
        ix |= 1 << c->ix;
    }
    dsscomp->mode = DSSCOMP_SETUP_DISPLAY;
    dsscomp->mgrs[0].ix = 0;
    dsscomp->mgrs[0].alpha_blending = 1;
    dsscomp->mgrs[0].swap_rb = hwc_dev->swap_rb;
    dsscomp->num_mgrs = 1;

    if (ext->current.enabled || hwc_dev->last_ext_ovls) {
        dsscomp->mgrs[1] = dsscomp->mgrs[0];
        dsscomp->mgrs[1].ix = 1;
        dsscomp->num_mgrs++;
        hwc_dev->ext_ovls = dsscomp->num_ovls - hwc_dev->post2_layers;
    }

    /*
     * Whilst the mode of the display is being changed drop compositions to the
     * display
     */
    if (ext->last_mode == 0 && hwc_dev->on_tv) {
        dsscomp->num_ovls = 0;
    }

    if (debug) {
        ALOGD("prepare (%d) - %s (comp=%d, poss=%d/%d scaled, RGB=%d,BGR=%d,NV12=%d) (ext=%s%s%ddeg%s %dex/%dmx (last %dex,%din)\n",
             dsscomp->sync_id,
             hwc_dev->use_sgx ? "SGX+OVL" : "all-OVL",
             num.composited_layers,
             num.possible_overlay_layers, num.scaled_layers,
             num.RGB, num.BGR, num.NV12,
             ext->on_tv ? "tv+" : "",
             ext->current.enabled ? ext->current.docking ? "dock+" : "mirror+" : "OFF+",
             ext->current.rotation * 90,
             ext->current.hflip ? "+hflip" : "",
             hwc_dev->ext_ovls, num.max_hw_overlays, hwc_dev->last_ext_ovls, hwc_dev->last_int_ovls);
    }

    pthread_mutex_unlock(&hwc_dev->lock);
    return 0;
}

static void omap4_hwc_reset_screen(omap4_hwc_device_t *hwc_dev)
{
    static int first_set = 1;
    int ret;

    if (first_set) {
        first_set = 0;
        struct dsscomp_setup_dispc_data d = {
            .num_mgrs = 1,
        };
        /* remove bootloader image from the screen as blank/unblank does not change the composition */
        ret = ioctl(hwc_dev->dsscomp_fd, DSSCIOC_SETUP_DISPC, &d);
        if (ret)
            ALOGW("failed to remove bootloader image");

        /* blank and unblank fd to make sure display is properly programmed on boot.
         * This is needed because the bootloader can not be trusted.
         */
        ret = ioctl(hwc_dev->fb_fd, FBIOBLANK, FB_BLANK_POWERDOWN);
        if (ret)
            ALOGW("failed to blank display");

        ret = ioctl(hwc_dev->fb_fd, FBIOBLANK, FB_BLANK_UNBLANK);
        if (ret)
            ALOGW("failed to blank display");
    }
}

static int omap4_hwc_set(struct hwc_composer_device_1 *dev,
        size_t numDisplays, hwc_display_contents_1_t** displays)
{
    if (!numDisplays || displays == NULL) {
        ALOGD("set: empty display list");
        return 0;
    }
    hwc_display_t dpy = NULL;
    hwc_surface_t sur = NULL;
    hwc_display_contents_1_t* list = displays[0];  // ignore displays beyond the first
    if (list != NULL) {
        dpy = list->dpy;
        sur = list->sur;
    }
    omap4_hwc_device_t *hwc_dev = (omap4_hwc_device_t *)dev;
    struct dsscomp_setup_dispc_data *dsscomp = &hwc_dev->comp_data.dsscomp_data;
    int err = 0;
    int invalidate;

    pthread_mutex_lock(&hwc_dev->lock);

    /* disable resetting the screen on the first boot for devices
     * with hdmi as primary input.
     */
    if (!hwc_dev->on_tv)
        omap4_hwc_reset_screen(hwc_dev);

    invalidate = hwc_dev->ext_ovls_wanted && (hwc_dev->ext_ovls < hwc_dev->ext_ovls_wanted) &&
                                              (hwc_dev->stats.protected || !hwc_dev->ext_ovls);

    if (debug)
        dump_set_info(hwc_dev, list);

    if (dpy && sur) {
        // list can be NULL which means hwc is temporarily disabled.
        // however, if dpy and sur are null it means we're turning the
        // screen off. no shall not call eglSwapBuffers() in that case.

        if (hwc_dev->use_sgx) {
            if (!eglSwapBuffers((EGLDisplay)dpy, (EGLSurface)sur)) {
                ALOGE("eglSwapBuffers error");
                err = HWC_EGL_ERROR;
                goto err_out;
            }
        }

        //dump_dsscomp(dsscomp);

        // signal the event thread that a post has happened
        wakeup_hdmi_thread(hwc_dev);
        if (hwc_dev->force_sgx > 0)
            hwc_dev->force_sgx--;

        hwc_dev->comp_data.blit_data.rgz_flags = hwc_dev->blit_flags;
        hwc_dev->comp_data.blit_data.rgz_items = hwc_dev->blit_num;
        int omaplfb_comp_data_sz = sizeof(hwc_dev->comp_data) +
            (hwc_dev->comp_data.blit_data.rgz_items * sizeof(struct rgz_blt_entry));


        unsigned int nbufs = hwc_dev->post2_layers;
        if (hwc_dev->post2_blit_buffers) {
            /*
             * We don't want to pass a NULL entry in the Post2, but we need to
             * fix up buffer handle array and overlay indexes to account for
             * this
             */
            nbufs += hwc_dev->post2_blit_buffers - 1;

            if (hwc_dev->post2_layers > 1) {
                unsigned int i, j;
                for (i = 0; i < nbufs; i++) {
                    hwc_dev->buffers[i] = hwc_dev->buffers[i+1];
                }
                for (i = 1, j= 1; j < hwc_dev->post2_layers; i++, j++) {
                    dsscomp->ovls[j].ba = i;
                }
            }
        }
        ALOGI_IF(debugblt && hwc_dev->blt_policy != BLTPOLICY_DISABLED,
            "Post2, blits %d, ovl_buffers %d, blit_buffers %d sgx %d",
            hwc_dev->blit_num, hwc_dev->post2_layers, hwc_dev->post2_blit_buffers,
            hwc_dev->use_sgx);

        debug_post2(hwc_dev, nbufs);
        err = hwc_dev->fb_dev->Post2((framebuffer_device_t *)hwc_dev->fb_dev,
                                 hwc_dev->buffers,
                                 nbufs,
                                 dsscomp, omaplfb_comp_data_sz);
        showfps();

#if 0
        if (!hwc_dev->use_sgx) {
            __u32 crt = 0;
            int err2 = ioctl(hwc_dev->fb_fd, FBIO_WAITFORVSYNC, &crt);
            if (err2) {
                ALOGE("failed to wait for vsync (%d)", errno);
                err = err ? : -errno;
            }
        }
#endif
    }
    hwc_dev->last_ext_ovls = hwc_dev->ext_ovls;
    hwc_dev->last_int_ovls = hwc_dev->post2_layers;
    if (err)
        ALOGE("Post2 error");

    check_sync_fds(numDisplays, displays);

err_out:
    pthread_mutex_unlock(&hwc_dev->lock);

    if (invalidate && hwc_dev->procs && hwc_dev->procs->invalidate)
        hwc_dev->procs->invalidate(hwc_dev->procs);

    return err;
}

static void omap4_hwc_dump(struct hwc_composer_device_1 *dev, char *buff, int buff_len)
{
    omap4_hwc_device_t *hwc_dev = (omap4_hwc_device_t *)dev;
    struct dsscomp_setup_dispc_data *dsscomp = &hwc_dev->comp_data.dsscomp_data;
    struct dump_buf log = {
        .buf = buff,
        .buf_len = buff_len,
    };
    int i;

    dump_printf(&log, "omap4_hwc %d:\n", dsscomp->num_ovls);
    dump_printf(&log, "  idle timeout: %dms\n", hwc_dev->idle);

    for (i = 0; i < dsscomp->num_ovls; i++) {
        struct dss2_ovl_cfg *cfg = &dsscomp->ovls[i].cfg;

        dump_printf(&log, "  layer %d:\n", i);
        dump_printf(&log, "     enabled:%s buff:%p %dx%d stride:%d\n",
                          cfg->enabled ? "true" : "false", hwc_dev->buffers[i],
                          cfg->width, cfg->height, cfg->stride);
        dump_printf(&log, "     src:(%d,%d) %dx%d dst:(%d,%d) %dx%d ix:%d zorder:%d\n",
                          cfg->crop.x, cfg->crop.y, cfg->crop.w, cfg->crop.h,
                          cfg->win.x, cfg->win.y, cfg->win.w, cfg->win.h,
                          cfg->ix, cfg->zorder);
    }

    if (hwc_dev->blt_policy != BLTPOLICY_DISABLED) {
        dump_printf(&log, "  bltpolicy: %s, bltmode: %s\n",
            hwc_dev->blt_policy == BLTPOLICY_DEFAULT ? "default" :
                hwc_dev->blt_policy == BLTPOLICY_ALL ? "all" : "unknown",
                    hwc_dev->blt_mode == BLTMODE_PAINT ? "paint" : "regionize");
    }
    dump_printf(&log, "\n");
}

static void free_png_image(omap4_hwc_device_t *hwc_dev, struct omap4_hwc_img *img)
{
    memset(img, 0, sizeof(*img));
}

static int load_png_image(omap4_hwc_device_t *hwc_dev, char *path, struct omap4_hwc_img *img)
{
    void *ptr = NULL;
    png_bytepp row_pointers = NULL;

    FILE *fd = fopen(path, "rb");
    if (!fd) {
        ALOGE("failed to open PNG file %s: (%d)", path, errno);
        return -EINVAL;
    }

    const int SIZE_PNG_HEADER = 8;
    __u8 header[SIZE_PNG_HEADER];
    fread(header, 1, SIZE_PNG_HEADER, fd);
    if (png_sig_cmp(header, 0, SIZE_PNG_HEADER)) {
        ALOGE("%s is not a PNG file", path);
        goto fail;
    }

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr)
         goto fail_alloc;
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
         goto fail_alloc;

    if (setjmp(png_jmpbuf(png_ptr)))
        goto fail_alloc;

    png_init_io(png_ptr, fd);
    png_set_sig_bytes(png_ptr, SIZE_PNG_HEADER);
    png_set_user_limits(png_ptr, limits.max_width, limits.max_height);
    png_read_info(png_ptr, info_ptr);

    __u8 bit_depth = png_get_bit_depth(png_ptr, info_ptr);
    __u32 width = png_get_image_width(png_ptr, info_ptr);
    __u32 height = png_get_image_height(png_ptr, info_ptr);
    __u8 color_type = png_get_color_type(png_ptr, info_ptr);

    switch (color_type) {
    case PNG_COLOR_TYPE_PALETTE:
        png_set_palette_to_rgb(png_ptr);
        png_set_filler(png_ptr, 128, PNG_FILLER_AFTER);
        break;
    case PNG_COLOR_TYPE_GRAY:
        if (bit_depth < 8) {
            png_set_expand_gray_1_2_4_to_8(png_ptr);
            if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
                png_set_tRNS_to_alpha(png_ptr);
        } else {
            png_set_filler(png_ptr, 128, PNG_FILLER_AFTER);
        }
        /* fall through */
    case PNG_COLOR_TYPE_GRAY_ALPHA:
        png_set_gray_to_rgb(png_ptr);
        break;
    case PNG_COLOR_TYPE_RGB:
        png_set_filler(png_ptr, 128, PNG_FILLER_AFTER);
        /* fall through */
    case PNG_COLOR_TYPE_RGB_ALPHA:
        png_set_bgr(png_ptr);
        break;
    default:
        ALOGE("unsupported PNG color: %x", color_type);
        goto fail_alloc;
    }

    if (bit_depth == 16)
        png_set_strip_16(png_ptr);

    const int bpp = 4;
    img->size = ALIGN(width * height * bpp, 4096);
    if (img->size > hwc_dev->img_mem_size) {
        ALOGE("image does not fit into framebuffer area (%d > %d)", img->size, hwc_dev->img_mem_size);
        goto fail_alloc;
    }
    img->ptr = hwc_dev->img_mem_ptr;

    row_pointers = calloc(height, sizeof(*row_pointers));
    if (!row_pointers) {
        ALOGE("failed to allocate row pointers");
        goto fail_alloc;
    }
    __u32 i;
    for (i = 0; i < height; i++)
        row_pointers[i] = img->ptr + i * width * bpp;
    png_set_rows(png_ptr, info_ptr, row_pointers);
    png_read_update_info(png_ptr, info_ptr);
    img->rowbytes = png_get_rowbytes(png_ptr, info_ptr);

    png_read_image(png_ptr, row_pointers);
    png_read_end(png_ptr, NULL);
    free(row_pointers);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fd);
    img->width = width;
    img->height = height;
    return 0;

fail_alloc:
    free_png_image(hwc_dev, img);
    free(row_pointers);
    if (!png_ptr || !info_ptr)
        ALOGE("failed to allocate PNG structures");
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
fail:
    fclose(fd);
    return -EINVAL;
}


static int omap4_hwc_device_close(hw_device_t* device)
{
    omap4_hwc_device_t *hwc_dev = (omap4_hwc_device_t *) device;;

    if (hwc_dev) {
        if (hwc_dev->dsscomp_fd >= 0)
            close(hwc_dev->dsscomp_fd);
        if (hwc_dev->hdmi_fb_fd >= 0)
            close(hwc_dev->hdmi_fb_fd);
        if (hwc_dev->fb_fd >= 0)
            close(hwc_dev->fb_fd);
        if (hwc_dev->ion_fd >= 0)
            ion_close(hwc_dev->ion_fd);

        /* pthread will get killed when parent process exits */
        pthread_mutex_destroy(&hwc_dev->lock);
        pthread_mutex_destroy(&hwc_dev->vsync_lock);
        free(hwc_dev);
    }

    return 0;
}

static int omap4_hwc_open_fb_hal(IMG_framebuffer_device_public_t **fb_dev)
{
    const struct hw_module_t *psModule;
    IMG_gralloc_module_public_t *psGrallocModule;
    int err;

    err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &psModule);
    psGrallocModule = (IMG_gralloc_module_public_t *) psModule;

    if(err)
        goto err_out;

    if (strcmp(psGrallocModule->base.common.author, "Imagination Technologies")) {
        err = -EINVAL;
        goto err_out;
    }

    *fb_dev = psGrallocModule->psFrameBufferDevice;

    return 0;

err_out:
    ALOGE("Composer HAL failed to load compatible Graphics HAL");
    return err;
}

static void set_primary_display_transform_matrix(omap4_hwc_device_t *hwc_dev)
{
    /* create primary display translation matrix */
    hwc_dev->fb_dis.ix = 0;/*Default display*/

    int ret = ioctl(hwc_dev->dsscomp_fd, DSSCIOC_QUERY_DISPLAY, &hwc_dev->fb_dis);
    if (ret)
        ALOGE("failed to get display info (%d): %m", errno);

    int lcd_w = hwc_dev->fb_dis.timings.x_res;
    int lcd_h = hwc_dev->fb_dis.timings.y_res;
    int orig_w = hwc_dev->fb_dev->base.width;
    int orig_h = hwc_dev->fb_dev->base.height;
    hwc_rect_t region = {.left = 0, .top = 0, .right = orig_w, .bottom = orig_h};
    hwc_dev->primary_region = region;
    hwc_dev->primary_rotation = ((lcd_w > lcd_h) ^ (orig_w > orig_h)) ? 1 : 0;
    hwc_dev->primary_transform = ((lcd_w != orig_w)||(lcd_h != orig_h)) ? 1 : 0;

    ALOGI("transforming FB (%dx%d) => (%dx%d) rot%d", orig_w, orig_h, lcd_w, lcd_h, hwc_dev->primary_rotation);

    /* reorientation matrix is:
       m = (center-from-target-center) * (scale-to-target) * (mirror) * (rotate) * (center-to-original-center) */

    memcpy(hwc_dev->primary_m, m_unit, sizeof(m_unit));
    m_translate(hwc_dev->primary_m, -(orig_w >> 1), -(orig_h >> 1));
    m_rotate(hwc_dev->primary_m, hwc_dev->primary_rotation);
    if (hwc_dev->primary_rotation & 1)
         swap(orig_w, orig_h);
    m_scale(hwc_dev->primary_m, orig_w, lcd_w, orig_h, lcd_h);
    m_translate(hwc_dev->primary_m, lcd_w >> 1, lcd_h >> 1);
}


static void handle_hotplug(omap4_hwc_device_t *hwc_dev)
{
    omap4_hwc_ext_t *ext = &hwc_dev->ext;
    __u8 state = ext->hdmi_state;

    /* Ignore external HDMI logic if the primary display is HDMI */
    if (hwc_dev->on_tv) {
        ALOGI("Primary display is HDMI - skip clone/dock logic");

        if (state) {
            __u32 xres = hwc_dev->fb_dev->base.width;
            __u32 yres = hwc_dev->fb_dev->base.height;
            if (omap4_hwc_set_best_hdmi_mode(hwc_dev, xres, yres, ext->lcd_xpy)) {
                ALOGE("Failed to set HDMI mode");
            }
            set_primary_display_transform_matrix(hwc_dev);

            ioctl(hwc_dev->fb_fd, FBIOBLANK, FB_BLANK_UNBLANK);

            if (hwc_dev->procs && hwc_dev->procs->invalidate) {
                hwc_dev->procs->invalidate(hwc_dev->procs);
            }
        } else
            ext->last_mode = 0;

        return;
    }

    pthread_mutex_lock(&hwc_dev->lock);
    ext->dock.enabled = ext->mirror.enabled = 0;
    if (state) {
        /* check whether we can clone and/or dock */
        char value[PROPERTY_VALUE_MAX];
        property_get("persist.hwc.docking.enabled", value, "1");
        ext->dock.enabled = atoi(value) > 0;
        property_get("persist.hwc.mirroring.enabled", value, "1");
        ext->mirror.enabled = atoi(value) > 0;
        property_get("persist.hwc.avoid_mode_change", value, "1");
        ext->avoid_mode_change = atoi(value) > 0;

        /* get cloning transformation */
        property_get("persist.hwc.docking.transform", value, "0");
        ext->dock.rotation = atoi(value) & EXT_ROTATION;
        ext->dock.hflip = (atoi(value) & EXT_HFLIP) > 0;
        ext->dock.docking = 1;
        property_get("persist.hwc.mirroring.transform", value, hwc_dev->fb_dis.timings.y_res > hwc_dev->fb_dis.timings.x_res ? "3" : "0");
        ext->mirror.rotation = atoi(value) & EXT_ROTATION;
        ext->mirror.hflip = (atoi(value) & EXT_HFLIP) > 0;
        ext->mirror.docking = 0;

        if (ext->force_dock) {
            /* restrict to docking with no transform */
            ext->mirror.enabled = 0;
            ext->dock.rotation = 0;
            ext->dock.hflip = 0;

            if (!dock_image.rowbytes) {
                property_get("persist.hwc.dock_image", value, "/vendor/res/images/dock/dock.png");
                load_png_image(hwc_dev, value, &dock_image);
            }
        }

        /* select best mode for mirroring */
        if (ext->mirror.enabled) {
            ext->current = ext->mirror;
            ext->mirror_mode = 0;
            if (setup_mirroring(hwc_dev) == 0) {
                ext->mirror_mode = ext->last_mode;
                ioctl(hwc_dev->hdmi_fb_fd, FBIOBLANK, FB_BLANK_UNBLANK);
            } else
                ext->mirror.enabled = 0;
        }
        /* Allocate backup buffers for FB rotation
        * This is required only if the FB tranform is different from that
        * of the external display and the FB is not in TILER2D space
        */
        if (ext->mirror.rotation && (limits.fbmem_type != DSSCOMP_FBMEM_TILER2D))
            allocate_tiler2d_buffers(hwc_dev);

    } else {
        ext->last_mode = 0;
        if (ext->mirror.rotation && (limits.fbmem_type != DSSCOMP_FBMEM_TILER2D)) {
            /* free tiler 2D buffer on detach */
            free_tiler2d_buffers(hwc_dev);
        }
    }
    ALOGI("external display changed (state=%d, mirror={%s tform=%ddeg%s}, dock={%s tform=%ddeg%s%s}, tv=%d", state,
         ext->mirror.enabled ? "enabled" : "disabled",
         ext->mirror.rotation * 90,
         ext->mirror.hflip ? "+hflip" : "",
         ext->dock.enabled ? "enabled" : "disabled",
         ext->dock.rotation * 90,
         ext->dock.hflip ? "+hflip" : "",
         ext->force_dock ? " forced" : "",
         ext->on_tv);

    pthread_mutex_unlock(&hwc_dev->lock);

    /* hwc_dev->procs is set right after the device is opened, but there is
     * still a race condition where a hotplug event might occur after the open
     * but before the procs are registered. */
    if (hwc_dev->procs && hwc_dev->procs->invalidate)
            hwc_dev->procs->invalidate(hwc_dev->procs);
}

static void handle_uevents(omap4_hwc_device_t *hwc_dev, const char *buff, int len)
{
    int dock;
    int hdmi;
    int vsync;
    int state = 0;
    uint64_t timestamp = 0;
    const char *s = buff;

    dock = !strcmp(s, "change@/devices/virtual/switch/dock");
    hdmi = !strcmp(s, "change@/devices/virtual/switch/hdmi");
    vsync = !strcmp(s, "change@/devices/platform/omapfb") ||
        !strcmp(s, "change@/devices/virtual/switch/omapfb-vsync");

    if (!dock && !vsync && !hdmi)
       return;

    s += strlen(s) + 1;

    while(*s) {
        if (!strncmp(s, "SWITCH_STATE=", strlen("SWITCH_STATE=")))
            state = atoi(s + strlen("SWITCH_STATE="));
        else if (!strncmp(s, "SWITCH_TIME=", strlen("SWITCH_TIME=")))
            timestamp = strtoull(s + strlen("SWITCH_TIME="), NULL, 0);
        else if (!strncmp(s, "VSYNC=", strlen("VSYNC=")))
            timestamp = strtoull(s + strlen("VSYNC="), NULL, 0);

        s += strlen(s) + 1;
        if (s - buff >= len)
            break;
    }

    if (vsync) {
        fire_vsync_event(hwc_dev, timestamp);
    } else {
        if (dock) {
            hwc_dev->ext.force_dock = state == 1;
        } else {
            /* If the primary display is HDMI, VSYNC is enabled, and HDMI's plug
             * state has just gone from 1->0, then we need to be sure to start
             * faking the VSYNC events.
             */
            if (hwc_dev->on_tv) {
                int new_state, state_change;

                pthread_mutex_lock(&hwc_dev->vsync_lock);

                new_state = state == 1;
                state_change = (new_state != hwc_dev->ext.hdmi_state);
                hwc_dev->ext.hdmi_state = new_state;
                if (state_change && !new_state)
                    wakeup_hdmi_thread(hwc_dev);

                pthread_mutex_unlock(&hwc_dev->vsync_lock);
            } else {
                hwc_dev->ext.hdmi_state = state == 1;
            }
        }
        handle_hotplug(hwc_dev);
    }
}

static void *omap4_hwc_hdmi_thread(void *data)
{
    omap4_hwc_device_t *hwc_dev = data;
    static char uevent_desc[4096];
    struct pollfd fds[2];
    int invalidate = 0;
    int timeout;
    int err;

    setpriority(PRIO_PROCESS, 0, HAL_PRIORITY_URGENT_DISPLAY);

    uevent_init();

    fds[0].fd = uevent_get_fd();
    fds[0].events = POLLIN;
    fds[1].fd = hwc_dev->wakeup_evt;
    fds[1].events = POLLIN;

    timeout = hwc_dev->idle ? hwc_dev->idle : -1;

    memset(uevent_desc, 0, sizeof(uevent_desc));

    do {
        uint64_t idle_wakeup  = (uint64_t)(-1);
        uint64_t vsync_wakeup = (uint64_t)(-1);
        uint64_t now = vsync_clock_now();
        uint64_t effective_wakeup;
        int effective_timeout;

        if (timeout >= 0)
            idle_wakeup = now + (((uint64_t)timeout) * 1000000);

        if (hwc_dev->on_tv) {
            pthread_mutex_lock(&hwc_dev->vsync_lock);

            if (!hwc_dev->ext.hdmi_state && hwc_dev->vsync_enabled) {
                vsync_wakeup = hwc_dev->last_vsync_time_valid
                             ? hwc_dev->last_vsync_time
                             : now;

                vsync_wakeup += hwc_dev->fake_vsync_period;

                if (vsync_wakeup < now)
                    vsync_wakeup = now;
            }

            pthread_mutex_unlock(&hwc_dev->vsync_lock);
        }

        effective_wakeup = idle_wakeup < vsync_wakeup
                         ? idle_wakeup
                         : vsync_wakeup;
        if (effective_wakeup == (uint64_t)(-1))
            effective_timeout = -1;
        else if (effective_wakeup <= now)
            effective_timeout = 0;
        else
            effective_timeout = (int)((effective_wakeup - now + 999999) / 1000000);

        if (effective_timeout)
            err = poll(fds, 2, effective_timeout);
        else
            err = 0;

        now = vsync_clock_now();

        if (err == 0) {
            int fired = 0;

            if (now >= vsync_wakeup) {
                fire_vsync_event(hwc_dev, vsync_wakeup);
                fired = 1;
            }

            if (hwc_dev->idle && (now >= idle_wakeup)) {
                if (hwc_dev->procs) {
                    pthread_mutex_lock(&hwc_dev->lock);
                    invalidate = hwc_dev->last_int_ovls > 1 && !hwc_dev->force_sgx;
                    if (invalidate) {
                        hwc_dev->force_sgx = 2;
                    }
                    pthread_mutex_unlock(&hwc_dev->lock);

                    if (invalidate) {
                        hwc_dev->procs->invalidate(hwc_dev->procs);
                        timeout = -1;
                    }
                }

                fired = 1;
            }

            if (fired)
                continue;
        }

        if (err == -1) {
            if (errno != EINTR)
                ALOGE("event error: %m");
            continue;
        }

        if (fds[1].revents & POLLIN) {
            uint64_t tmp;

            read(hwc_dev->wakeup_evt, &tmp, sizeof(tmp));

            if (!hwc_dev->force_sgx)
                timeout = hwc_dev->idle ? hwc_dev->idle : -1;
        }

        if (fds[0].revents & POLLIN) {
            /* keep last 2 zeroes to ensure double 0 termination */
            int len = uevent_next_event(uevent_desc, sizeof(uevent_desc) - 2);
            handle_uevents(hwc_dev, uevent_desc, len);
        }
    } while (1);

    return NULL;
}

static void omap4_hwc_registerProcs(struct hwc_composer_device_1* dev,
                                    hwc_procs_t const* procs)
{
    omap4_hwc_device_t *hwc_dev = (omap4_hwc_device_t *) dev;

    hwc_dev->procs = (typeof(hwc_dev->procs)) procs;
}

static int omap4_hwc_query(struct hwc_composer_device_1* dev,
        int what, int* value)
{
    omap4_hwc_device_t *hwc_dev = (omap4_hwc_device_t *) dev;

    switch (what) {
    case HWC_BACKGROUND_LAYER_SUPPORTED:
        // we don't support the background layer yet
        value[0] = 0;
        break;
    case HWC_VSYNC_PERIOD:
        // vsync period in nanosecond
        value[0] = 1000000000.0 / hwc_dev->fb_dev->base.fps;
        break;
    default:
        // unsupported query
        return -EINVAL;
    }
    return 0;
}

static int omap4_hwc_event_control(struct hwc_composer_device_1* dev,
        int dpy, int event, int enabled)
{
    omap4_hwc_device_t *hwc_dev = (omap4_hwc_device_t *) dev;

    switch (event) {
    case HWC_EVENT_VSYNC:
    {
        int val = !!enabled;
        int err;

        /* If the primary display is HDMI, then we need to be sure to fake a
         * stream vsync events if vsync is enabled, but HDMI happens to be
         * disconnected.
         */
        if (hwc_dev->on_tv) {
            pthread_mutex_lock(&hwc_dev->vsync_lock);

            if (!val)
                hwc_dev->last_vsync_time_valid = 0;

            /* If VSYNC is enabled, but HDMI is not actually plugged in, we need
             * to fake it.  Poke the work thread to make sure it is taking care
             * of things.
             */
            if (!hwc_dev->ext.hdmi_state && !hwc_dev->vsync_enabled && val)
                wakeup_hdmi_thread(hwc_dev);

            hwc_dev->vsync_enabled = val;

            err = ioctl(hwc_dev->fb_fd, OMAPFB_ENABLEVSYNC, &val);
            pthread_mutex_unlock(&hwc_dev->vsync_lock);
        } else {
            err = ioctl(hwc_dev->fb_fd, OMAPFB_ENABLEVSYNC, &val);
        }

        if (err < 0)
            return -errno;

        return 0;
    }
    default:
        return -EINVAL;
    }
}

static int omap4_hwc_blank(struct hwc_composer_device_1 *dev, int dpy, int blank)
{
    // We're using an older method of screen blanking based on
    // early_suspend in the kernel.  No need to do anything here.
    return 0;
}

static int omap4_hwc_device_open(const hw_module_t* module, const char* name,
                hw_device_t** device)
{
    omap4_hwc_module_t *hwc_mod = (omap4_hwc_module_t *)module;
    omap4_hwc_device_t *hwc_dev;
    int err = 0;

    if (strcmp(name, HWC_HARDWARE_COMPOSER)) {
        return -EINVAL;
    }

    if (!hwc_mod->fb_dev) {
        err = omap4_hwc_open_fb_hal(&hwc_mod->fb_dev);
        if (err)
            return err;

        if (!hwc_mod->fb_dev) {
            ALOGE("Framebuffer HAL not opened before HWC");
            return -EFAULT;
        }
        hwc_mod->fb_dev->bBypassPost = 1;
    }

    hwc_dev = (omap4_hwc_device_t *)malloc(sizeof(*hwc_dev));
    if (hwc_dev == NULL)
        return -ENOMEM;

    memset(hwc_dev, 0, sizeof(*hwc_dev));

    hwc_dev->base.common.tag = HARDWARE_DEVICE_TAG;
    hwc_dev->base.common.version = HWC_DEVICE_API_VERSION_1_0;
    hwc_dev->base.common.module = (hw_module_t *)module;
    hwc_dev->base.common.close = omap4_hwc_device_close;
    hwc_dev->base.prepare = omap4_hwc_prepare;
    hwc_dev->base.set = omap4_hwc_set;
    hwc_dev->base.eventControl = omap4_hwc_event_control;
    hwc_dev->base.blank = omap4_hwc_blank;
    hwc_dev->base.query = omap4_hwc_query;
    hwc_dev->base.registerProcs = omap4_hwc_registerProcs;
    hwc_dev->base.dump = omap4_hwc_dump;
    hwc_dev->fb_dev = hwc_mod->fb_dev;
    hwc_dev->wakeup_evt = -1;
    *device = &hwc_dev->base.common;

    hwc_dev->vsync_enabled = 0;
    hwc_dev->last_vsync_time_valid = 0;
    hwc_dev->fake_vsync_period = 1000000000ull/60;

    hwc_dev->dsscomp_fd = open("/dev/dsscomp", O_RDWR);
    if (hwc_dev->dsscomp_fd < 0) {
        ALOGE("failed to open dsscomp (%d)", errno);
        err = -errno;
        goto done;
    }

    int ret = ioctl(hwc_dev->dsscomp_fd, DSSCIOC_QUERY_PLATFORM, &limits);
    if (ret) {
        ALOGE("failed to get platform limits (%d): %m", errno);
        err = -errno;
        goto done;
    }

    hwc_dev->fb_fd = open("/dev/graphics/fb0", O_RDWR);
    if (hwc_dev->fb_fd < 0) {
        ALOGE("failed to open fb (%d)", errno);
        err = -errno;
        goto done;
    }

    struct fb_fix_screeninfo fix;
    if (ioctl(hwc_dev->fb_fd, FBIOGET_FSCREENINFO, &fix)) {
        ALOGE("failed to get fb info (%d)", errno);
        err = -errno;
        goto done;
    }

    hwc_dev->img_mem_size = fix.smem_len;
    hwc_dev->img_mem_ptr = mmap(NULL, fix.smem_len, PROT_WRITE, MAP_SHARED, hwc_dev->fb_fd, 0);
    if (hwc_dev->img_mem_ptr == MAP_FAILED) {
        ALOGE("failed to map fb memory");
        err = -errno;
        goto done;
    }

    /* Allocate the maximum buffers that we can receive from HWC */
    hwc_dev->buffers = malloc(sizeof(buffer_handle_t) * MAX_HWC_LAYERS);
    if (!hwc_dev->buffers) {
        err = -ENOMEM;
        goto done;
    }

    ret = ioctl(hwc_dev->dsscomp_fd, DSSCIOC_QUERY_DISPLAY, &hwc_dev->fb_dis);
    if (ret) {
        ALOGE("failed to get display info (%d): %m", errno);
        err = -errno;
        goto done;
    }

    hwc_dev->ion_fd = ion_open();
    if (hwc_dev->ion_fd < 0) {
        ALOGE("failed to open ion driver (%d)", errno);
    }

    int i;
    for (i = 0; i < NUM_EXT_DISPLAY_BACK_BUFFERS; i++) {
        hwc_dev->ion_handles[i] = NULL;
    }

    /* use default value in case some of requested display parameters missing */
    hwc_dev->ext.lcd_xpy = 1.0;
    if (hwc_dev->fb_dis.timings.x_res && hwc_dev->fb_dis.height_in_mm) {
        hwc_dev->ext.lcd_xpy = (float)
            hwc_dev->fb_dis.width_in_mm / hwc_dev->fb_dis.timings.x_res /
            hwc_dev->fb_dis.height_in_mm * hwc_dev->fb_dis.timings.y_res;
    }

    if (hwc_dev->fb_dis.channel == OMAP_DSS_CHANNEL_DIGIT) {
        ALOGI("Primary display is HDMI");
        hwc_dev->on_tv = 1;
    }
    else {
        hwc_dev->hdmi_fb_fd = open("/dev/graphics/fb1", O_RDWR);
        if (hwc_dev->hdmi_fb_fd < 0) {
            ALOGE("failed to open hdmi fb (%d)", errno);
            err = -errno;
            goto done;
        }
    }
    set_primary_display_transform_matrix(hwc_dev);

    if ((hwc_dev->wakeup_evt = eventfd(0, EFD_NONBLOCK)) < 0) {
            ALOGE("failed to eventfd (%d): %m", errno);
            err = -errno;
            goto done;
    }

    if (pthread_mutex_init(&hwc_dev->lock, NULL)) {
        ALOGE("failed to create mutex (%d): %m", errno);
        err = -errno;
        goto done;
    }

    if (pthread_mutex_init(&hwc_dev->vsync_lock, NULL)) {
        ALOGE("failed to create vsync mutex (%d): %m", errno);
        err = -errno;
        goto done;
    }

    if (pthread_create(&hwc_dev->hdmi_thread, NULL, omap4_hwc_hdmi_thread, hwc_dev))
    {
        ALOGE("failed to create HDMI listening thread (%d): %m", errno);
        err = -errno;
        goto done;
    }

    /* get debug properties */

    /* see if hwc is enabled at all */
    char value[PROPERTY_VALUE_MAX];
    property_get("debug.hwc.rgb_order", value, "1");
    hwc_dev->flags_rgb_order = atoi(value);
    property_get("debug.hwc.nv12_only", value, "0");
    hwc_dev->flags_nv12_only = atoi(value);
    property_get("debug.hwc.idle", value, "250");
    hwc_dev->idle = atoi(value);

    /* get the board specific clone properties */
    /* 0:0:1280:720 */
    if (property_get("persist.hwc.mirroring.region", value, "") <= 0 ||
        sscanf(value, "%d:%d:%d:%d",
               &hwc_dev->ext.mirror_region.left, &hwc_dev->ext.mirror_region.top,
               &hwc_dev->ext.mirror_region.right, &hwc_dev->ext.mirror_region.bottom) != 4 ||
        hwc_dev->ext.mirror_region.left >= hwc_dev->ext.mirror_region.right ||
        hwc_dev->ext.mirror_region.top >= hwc_dev->ext.mirror_region.bottom) {
        struct hwc_rect fb_region = { .right = hwc_dev->fb_dev->base.width, .bottom = hwc_dev->fb_dev->base.height };
        hwc_dev->ext.mirror_region = fb_region;
    }
    ALOGI("clone region is set to (%d,%d) to (%d,%d)",
         hwc_dev->ext.mirror_region.left, hwc_dev->ext.mirror_region.top,
         hwc_dev->ext.mirror_region.right, hwc_dev->ext.mirror_region.bottom);

    /* read switch state */
    int sw_fd = open("/sys/class/switch/hdmi/state", O_RDONLY);
    if (sw_fd >= 0) {
        char value;
        if (read(sw_fd, &value, 1) == 1)
            hwc_dev->ext.hdmi_state = value == '1';
        close(sw_fd);
    }
    sw_fd = open("/sys/class/switch/dock/state", O_RDONLY);
    if (sw_fd >= 0) {
        char value;
        if (read(sw_fd, &value, 1) == 1)
            hwc_dev->ext.force_dock = value == '1';
        close(sw_fd);
    }
    handle_hotplug(hwc_dev);

    ALOGI("omap4_hwc_device_open(rgb_order=%d nv12_only=%d)",
        hwc_dev->flags_rgb_order, hwc_dev->flags_nv12_only);

    int gc2d_fd = open("/dev/gcioctl", O_RDWR);
    if (gc2d_fd < 0) {
        ALOGI("Unable to open gc-core device (%d), blits disabled", errno);
        hwc_dev->blt_policy = BLTPOLICY_DISABLED;
    } else {
        property_get("persist.hwc.bltmode", value, "1");
        hwc_dev->blt_mode = atoi(value);
        property_get("persist.hwc.bltpolicy", value, "1");
        hwc_dev->blt_policy = atoi(value);
        ALOGI("blitter present, blits mode %d, blits policy %d", hwc_dev->blt_mode, hwc_dev->blt_policy);
        close(gc2d_fd);

        if (rgz_get_screengeometry(hwc_dev->fb_fd, &gscrngeom,
                hwc_dev->fb_dev->base.format) != 0) {
            err = -EINVAL;
            goto done;
        }
    }

    property_get("persist.hwc.upscaled_nv12_limit", value, "2.");
    sscanf(value, "%f", &hwc_dev->upscaled_nv12_limit);
    if (hwc_dev->upscaled_nv12_limit < 0. || hwc_dev->upscaled_nv12_limit > 2048.) {
        ALOGW("Invalid upscaled_nv12_limit (%s), setting to 2.", value);
        hwc_dev->upscaled_nv12_limit = 2.;
    }

done:
    if (err && hwc_dev) {
        if (hwc_dev->dsscomp_fd >= 0)
            close(hwc_dev->dsscomp_fd);
        if (hwc_dev->hdmi_fb_fd >= 0)
            close(hwc_dev->hdmi_fb_fd);
        if (hwc_dev->fb_fd >= 0)
            close(hwc_dev->fb_fd);
        if (hwc_dev->wakeup_evt >= 0)
            close(hwc_dev->wakeup_evt);
        pthread_mutex_destroy(&hwc_dev->lock);
        pthread_mutex_destroy(&hwc_dev->vsync_lock);
        free(hwc_dev->buffers);
        free(hwc_dev);
    }

    return err;
}

static struct hw_module_methods_t omap4_hwc_module_methods = {
    .open = omap4_hwc_device_open,
};

omap4_hwc_module_t HAL_MODULE_INFO_SYM = {
    .base = {
        .common = {
            .tag =                  HARDWARE_MODULE_TAG,
            .module_api_version =   HWC_MODULE_API_VERSION_0_1,
            .hal_api_version =      HARDWARE_HAL_API_VERSION,
            .id =                   HWC_HARDWARE_MODULE_ID,
            .name =                 "OMAP 44xx Hardware Composer HAL",
            .author =               "Texas Instruments",
            .methods =              &omap4_hwc_module_methods,
        },
    },
};
