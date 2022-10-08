/*
 * Copyright (C) 2013 The Android Open Source Project
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include <pthread.h>

#include <hardware/hardware.h>
#include <hardware/gralloc.h>
#include <hardware/hwcomposer.h>

#include <system/window.h>
#include <cutils/native_handle.h>

// normalize and shorten type names
typedef struct android_native_base_t aBase;
typedef struct ANativeWindowBuffer aBuffer;
typedef struct ANativeWindow aWindow;

static int trace_level = 1;

#define _TRACE(n,fmt...) \
	do { if (trace_level >= n) fprintf(stderr, "CNW: " fmt); } while (0)

#define ERROR(fmt...) _TRACE(0, fmt)
#define INFO(fmt...) _TRACE(1, fmt)
#define LOG(fmt...) _TRACE(2, fmt)
#define TRACE(fmt...) _TRACE(3, fmt)

#define QCT_WORKAROUND 1

typedef struct CNativeBuffer {
	aBuffer base;
	struct CNativeBuffer *next;
	struct CNativeBuffer *prev;
	int ffd;
} CNativeBuffer;

typedef struct CNativeWindow {
	aWindow base;

	hwc_composer_device_1_t *hwc;
	framebuffer_device_t *fb;
	alloc_device_t *gr;

	pthread_mutex_t lock;
	pthread_cond_t cvar;

	aBuffer *front;
	aBuffer *spare;

	CNativeBuffer free_buffer_queue;

	unsigned width;
	unsigned height;
	unsigned xdpi;
	unsigned ydpi;
	unsigned format;

	hwc_display_contents_1_t *dclist[HWC_NUM_PHYSICAL_DISPLAY_TYPES];

	hwc_display_contents_1_t dc;
	hwc_layer_1_t layer[4];
} CNativeWindow;

static inline CNativeBuffer *from_abuffer(aBuffer *buf) {
	return (CNativeBuffer*) buf;
}

static CNativeBuffer *get_front(struct CNativeBuffer *queue) {
	CNativeBuffer *buf = queue->next;
	if (buf == queue)
		return 0;
	buf->next->prev = queue;
	queue->next = buf->next;
	buf->next = buf->prev = 0;
	return buf;
}

static void put_front(struct CNativeBuffer *queue, aBuffer *_buf) {
	struct CNativeBuffer *buf = (struct CNativeBuffer *) _buf;
	buf->prev = queue;
	buf->next = queue->next;
	queue->next->prev = buf;
	queue->next = buf;
}

static void put_back(struct CNativeBuffer *queue, aBuffer *_buf) {
	struct CNativeBuffer *buf = (struct CNativeBuffer *) _buf;
	buf->next = queue;
	buf->prev = queue->prev;
	queue->prev->next = buf;
	queue->prev = buf;
}

static void cnw_inc_ref(aBase *base) { TRACE("buf %p ref++\n",base); }
static void cnw_dec_ref(aBase *base) { TRACE("buf %p ref--\n",base); }

static inline CNativeWindow *from_base(aWindow *base) {
	return (CNativeWindow *) base;
}

static inline CNativeWindow *from_base_const(const aWindow *base) {
	return (CNativeWindow *) base;
}

static int cnw_set_swap_interval(aWindow *base, int interval) {
	CNativeWindow *win = from_base(base);
	if (win->fb && win->fb->setSwapInterval)
		return win->fb->setSwapInterval(win->fb, interval);
	return 0;
}

static int cnw_dequeue_buffer1(aWindow *base, aBuffer **buf, int *ffd) {
	CNativeWindow *win = from_base(base);
	CNativeBuffer *cnb;

	pthread_mutex_lock(&win->lock);

	while ((cnb = get_front(&win->free_buffer_queue)) == 0) {
		pthread_cond_wait(&win->cvar, &win->lock);
	}

	*ffd = cnb->ffd;
	*buf = &cnb->base;
	cnb->ffd = -1;
	LOG("<< dequeue buffer %p %d\n", *buf, *ffd);

	pthread_mutex_unlock(&win->lock);
	return 0;
}

static int cnw_lock_buffer0(aWindow *base, aBuffer *buffer) {
	return 0;
}

static void set_layer(hwc_layer_1_t *dl, aBuffer *buf, int ffd) {
	int right = buf->width;
	int bottom = buf->height;

	dl->compositionType = HWC_FRAMEBUFFER;
	dl->hints = 0;
	dl->flags = 0;

	dl->handle = buf->handle;
	dl->transform = 0;
	dl->blending = HWC_BLENDING_NONE;
	dl->sourceCrop.left = 0;
	dl->sourceCrop.top = 0;
	dl->sourceCrop.right = right;
	dl->sourceCrop.bottom = bottom;
	dl->displayFrame.left = 0;
	dl->displayFrame.top = 0;
	dl->displayFrame.right = right;
	dl->displayFrame.bottom = bottom;
	dl->visibleRegionScreen.numRects = 1;
	dl->visibleRegionScreen.rects = &dl->displayFrame;

	dl->acquireFenceFd = ffd;
	dl->releaseFenceFd = -1;
}

static void hwc_post(CNativeWindow *win, aBuffer *buf, int ffd) {
	hwc_composer_device_1_t *hwc = win->hwc;
	hwc_display_contents_1_t *dc = &(win->dc);
	hwc_layer_1_t *dl = win->dc.hwLayers;
	int r, i;

	dc->retireFenceFd = -1;
	dc->outbufAcquireFenceFd = -1;
	dc->flags = HWC_GEOMETRY_CHANGED;
	dc->numHwLayers = 1;

	// some hwcomposers fail if these are NULL
	dc->dpy = (void*) 0xdeadbeef;
	dc->sur = (void*) 0xdeadbeef;

	set_layer(&dl[0], buf, ffd);

	if (QCT_WORKAROUND) {
		set_layer(&dl[1], win->spare, -1);
		dl[1].compositionType = HWC_FRAMEBUFFER_TARGET;
		dc->numHwLayers++;
	}

	r = hwc->prepare(hwc, HWC_NUM_PHYSICAL_DISPLAY_TYPES, win->dclist);
	if (r) {
		ERROR("hwc->prepare failed r=%d\n",r);
		return;
	}

//	for (i = 0; i < dc->numHwLayers; i++)
//		LOG("dl[%d] ctype=0x%08x hints=0x%08x flags=0x%08x\n", i,
//			dl[i].compositionType, dl[0].hints, dl[0].flags);

	r = hwc->set(hwc, HWC_NUM_PHYSICAL_DISPLAY_TYPES, win->dclist);
	if (r) {
		ERROR("hwc->set failed, r=%d\n", r);
		return;
	}

	if (dc->retireFenceFd != -1)
		close(dc->retireFenceFd);
	if (dl->releaseFenceFd != -1) {
		CNativeBuffer *cnb = from_abuffer(buf);
		cnb->ffd = dl->releaseFenceFd;
	}
	if (QCT_WORKAROUND)
		if (dl[1].releaseFenceFd != -1)
			close(dl[1].releaseFenceFd);
}

static int cnw_queue_buffer1(aWindow *base, aBuffer *buffer, int ffd) {
	CNativeWindow *win = from_base(base);
	int res;
	LOG(">> queue buffer %p %d\n", buffer, ffd);
	if (win->fb) {
		res = win->fb->post(win->fb, buffer->handle);
		if (ffd != -1)
			close(ffd);
	} else {
		hwc_post(win, buffer, ffd);
		res = 0;
	}
	pthread_mutex_lock(&win->lock);
	if (win->front)
		put_back(&win->free_buffer_queue, win->front);
	win->front = buffer;
	pthread_cond_signal(&win->cvar);
	pthread_mutex_unlock(&win->lock);

	return res;
}

static int cnw_cancel_buffer1(aWindow *base, aBuffer *buf, int ffd) {
	CNativeWindow *win = from_base(base);
	CNativeBuffer *cnb = from_abuffer(buf);
	LOG("<< cancel buffer %p %d\n", buf, ffd);
	cnb->ffd = ffd;
	pthread_mutex_lock(&win->lock);
	put_front(&win->free_buffer_queue, buf);
	pthread_mutex_unlock(&win->lock);
	return 0;
}

static int cnw_dequeue_buffer0(aWindow *base, aBuffer **buf) {
	int ffd = -1;
	int r;
	r = cnw_dequeue_buffer1(base, buf, &ffd);
	if (ffd != -1)
		close(ffd);
	return r;
}

static int cnw_queue_buffer0(aWindow *base, aBuffer *buf) {
	return cnw_queue_buffer1(base, buf, -1);
}

static int cnw_cancel_buffer0(aWindow *base, aBuffer *buf) {
	return cnw_cancel_buffer1(base, buf, -1);
}

static int cnw_query(const aWindow *base, int what, int *value) {
	CNativeWindow *win = from_base_const(base);

	switch (what) {
	case NATIVE_WINDOW_WIDTH:
	case NATIVE_WINDOW_DEFAULT_WIDTH:
		*value = win->width;
		TRACE("query window width: %d\n", *value);
		return 0;
	case NATIVE_WINDOW_HEIGHT:
	case NATIVE_WINDOW_DEFAULT_HEIGHT:
		*value = win->height;
		TRACE("query window height: %d\n", *value);
		return 0;
	case NATIVE_WINDOW_FORMAT:
		*value = win->format;
		TRACE("query window format: %d\n", *value);
		return 0;
	case NATIVE_WINDOW_TRANSFORM_HINT:
		TRACE("query transform hint: 0\n");
		*value = 0;
		return 0;
	case NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS:
		TRACE("query min undequeued buffers: 1\n");
		*value = 1;
		return 0;
	default:
		*value = 0;
		ERROR("query %d unknown!\n", what);
		return -EINVAL;
	}
}

static int cnw_perform(aWindow *base, int op, ...) {
	CNativeWindow *win = from_base(base);
	va_list ap;
	va_start(ap, op);

	switch (op) {
	case NATIVE_WINDOW_SET_USAGE:
		TRACE("set usage %d\n", va_arg(ap,int));
		return 0;
	case NATIVE_WINDOW_CONNECT:
	case NATIVE_WINDOW_DISCONNECT:
	case NATIVE_WINDOW_API_CONNECT:
	case NATIVE_WINDOW_API_DISCONNECT:
		return 0;
	case NATIVE_WINDOW_SET_BUFFERS_FORMAT:
		TRACE("set buffers format %d\n", va_arg(ap,int));
		return 0;
	case NATIVE_WINDOW_SET_BUFFERS_TRANSFORM:
		TRACE("set buffers transform %d\n", va_arg(ap,int));
		return 0;
	case NATIVE_WINDOW_SET_BUFFERS_TIMESTAMP:
		TRACE("set buffers timestamp %lld\n", va_arg(ap,long long));
		return 0;
	case NATIVE_WINDOW_SET_SCALING_MODE:
		TRACE("set scaling mode %d\n", va_arg(ap,int));
		return 0;
	case NATIVE_WINDOW_SET_BUFFERS_DIMENSIONS: {
		int w = va_arg(ap,int);
		int h = va_arg(ap,int);
		if ((w == win->width) && (h == win->height)) {
			TRACE("set buffers dimensions %d x %d\n", w, h);
			return 0;
		}
		ERROR("cannot resize buffers to %d x %d\n", w, h);
		return -1;
	}
	default:
		ERROR("perform %d unknown!\n", op);
		return -ENODEV;
	}
}

static void hwc_invalidate(const struct hwc_procs *procs) {}
static void hwc_vsync(const struct hwc_procs *procs, int disp, int64_t ts) {}
static void hwc_hotplug(const struct hwc_procs *procs, int disp, int conn) {}

struct hwc_procs hprocs = {
	.invalidate = hwc_invalidate,
	.vsync = hwc_vsync,
	.hotplug = hwc_hotplug,
};

uint32_t attrs[] = {
	HWC_DISPLAY_WIDTH,
	HWC_DISPLAY_HEIGHT,
	HWC_DISPLAY_VSYNC_PERIOD,
	HWC_DISPLAY_DPI_X,
	HWC_DISPLAY_DPI_Y,
	HWC_DISPLAY_NO_ATTRIBUTE,
};

static int hwc_init(CNativeWindow *win) {
	hw_module_t const* module;
	hwc_composer_device_1_t *hwc;
	unsigned i;
	int r;
	uint32_t configs[32];
	uint32_t numconfigs = 32;
	int32_t values[8];

	if (hw_get_module(HWC_HARDWARE_MODULE_ID, &module) != 0) {
		ERROR("cannot open hw composer module\n");
		return -ENODEV;
	}

	if (hwc_open_1(module, &hwc)) {
		ERROR("cannot open hwc device\n");
		return -ENODEV;
	}
	win->hwc = hwc;

	LOG("hwc version 0x%08x\n", hwc->common.version);

	if ((hwc->common.version & 0xFFFF0000) < 0x01010000) {
		ERROR("hwc version less than 1.1\n");
		hwc_close_1(hwc);
		return -ENODEV;
	}

	hwc->registerProcs(hwc, &hprocs);

	if (hwc->getDisplayConfigs(hwc, 0, configs, &numconfigs)) {
		ERROR("cannot get configs\n");
		return -ENODEV;
	}
	for (i = 0; i < numconfigs; i++)
		LOG("cfg[%d] = 0x%08x\n", i, configs[i]);

	if ((r = hwc->getDisplayAttributes(hwc, 0, configs[0], attrs, values))) {
		ERROR("cannot get attributes %d\n", r);
		return -ENODEV;
	}

	win->width = values[0];
	win->height = values[1];
	win->xdpi = values[3];
	win->ydpi = values[4];
	win->format = HAL_PIXEL_FORMAT_RGBA_8888;

	hwc->blank(hwc, 0, 0);

	win->dclist[0] = &(win->dc);
	return 0;
}

static aBuffer *cnw_alloc(CNativeWindow *win, unsigned format, unsigned usage) {
	CNativeBuffer *cnb;
	aBuffer *buf;
	int err;

	if (!(cnb = malloc(sizeof(CNativeBuffer))))
		return 0;

	buf = &cnb->base;
	cnb->ffd = -1;

	buf->common.magic = ANDROID_NATIVE_BUFFER_MAGIC;
	buf->common.version = sizeof(aBuffer);
	buf->common.incRef = cnw_inc_ref;
	buf->common.decRef = cnw_dec_ref;

	buf->width = win->width;
	buf->height = win->height;
	buf->format = format;
	buf->usage = usage;

	err = win->gr->alloc(win->gr, win->width, win->height,
		format, usage, &buf->handle, &buf->stride);
	if (err) {
		ERROR("gralloc of %d x %d failed: err=%d\n",
			win->width, win->height, err);
		free(buf);
		return 0;
	}
	INFO("alloc buffer %p %d x %d\n", buf, win->width, win->height);
	return buf;
}

static int cnw_init(CNativeWindow *win) {
	hw_module_t const* module;
	framebuffer_device_t *fb = NULL;
	alloc_device_t *gr;
	int err, i, n;
	unsigned usage, format;

	memset(win, 0, sizeof(CNativeWindow));

	win->free_buffer_queue.next = &(win->free_buffer_queue);
	win->free_buffer_queue.prev = &(win->free_buffer_queue);

	if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module) != 0) {
		ERROR("cannot open gralloc module\n");
		return -ENODEV;
	}

	if (hwc_init(win)) {
		ERROR("cannot open hwcomposer, trying legacy fb HAL\n");
		err = framebuffer_open(module, &fb);
		if (err) {
			ERROR("cannot open fb HAL (%s)", strerror(-err));
			return -ENODEV;
		}
		win->width = fb->width;
		win->height = fb->height;
		win->format = fb->format;
		win->xdpi = fb->xdpi;
		win->ydpi = fb->ydpi;
		win->fb = fb;
	}

	INFO("display %d x %d fmt=%d\n",
		win->width, win->height, win->format);

	err = gralloc_open(module, &gr);
	if (err) {
		ERROR("couldn't open gralloc HAL (%s)", strerror(-err));
		return -ENODEV;
	}
	win->gr = gr;

	usage = GRALLOC_USAGE_HW_FB |
		GRALLOC_USAGE_HW_COMPOSER |
		GRALLOC_USAGE_HW_RENDER;

	for (i = 0; i < 2; i++) {
		aBuffer *buf = cnw_alloc(win, win->format, usage);
		if (!buf)
			return -ENOMEM;
		put_back(&win->free_buffer_queue, buf);
	}

	if (!win->fb && QCT_WORKAROUND) {
		win->spare = cnw_alloc(win, win->format, usage);
		if (!win->spare)
			return -ENOMEM;
	}

	// Disgusting, but we need to init these "const" fields
	// and unlike C++ we can't use const_cast<>
	*((float*) &win->base.xdpi) = win->xdpi;
	*((float*) &win->base.ydpi) = win->ydpi;
	*((int*) &win->base.minSwapInterval) = 1;
	*((int*) &win->base.maxSwapInterval) = 1;

	win->base.common.magic = ANDROID_NATIVE_WINDOW_MAGIC;
	win->base.common.version = sizeof(aWindow);
	win->base.common.incRef = cnw_inc_ref;
	win->base.common.decRef = cnw_dec_ref;

	win->base.setSwapInterval = cnw_set_swap_interval;
	win->base.dequeueBuffer_DEPRECATED = cnw_dequeue_buffer0;
	win->base.lockBuffer_DEPRECATED = cnw_lock_buffer0;
	win->base.queueBuffer_DEPRECATED = cnw_queue_buffer0;
	win->base.query = cnw_query;
	win->base.perform = cnw_perform;
	win->base.cancelBuffer_DEPRECATED = cnw_cancel_buffer0;
	win->base.dequeueBuffer = cnw_dequeue_buffer1;
	win->base.queueBuffer = cnw_queue_buffer1;
	win->base.cancelBuffer = cnw_cancel_buffer1;

	pthread_mutex_init(&win->lock, NULL);
	pthread_cond_init(&win->cvar, NULL);

	return 0;
}

void cnw_destroy(CNativeWindow *win) {
	if (win->fb)
		framebuffer_close(win->fb);
	if (win->hwc)
		hwc_close_1(win->hwc);
	if (win->gr)
		gralloc_close(win->gr);
	free(win);
}

CNativeWindow *cnw_create(void) {
	CNativeWindow *win;
	char *x;
	if ((x = getenv("CNWDEBUG")))
		trace_level = atoi(x);
	if (!(win = malloc(sizeof(CNativeWindow))))
		return NULL;
	if (cnw_init(win)) {
		cnw_destroy(win);
		return NULL;
	}
	return win;
}

void cnw_info(CNativeWindow *win, unsigned *w, unsigned *h, unsigned *fmt) {
	*w = win->width;
	*h = win->height;
	*fmt = win->format;
}

