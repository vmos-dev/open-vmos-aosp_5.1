/*
 * Copyright (C) 2007 The Android Open Source Project
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

#include <stdlib.h>
#include <unistd.h>

#include <fcntl.h>
#include <stdio.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <time.h>

#include <linux/fb.h>
#include <linux/kd.h>

struct simple_fb {
    void *data;
    int width;
    int height;
    int stride;
    int bpp;
};

static struct simple_fb gr_fbs[2];
static unsigned gr_active_fb = 0;

static int gr_fb_fd = -1;
static int gr_vt_fd = -1;

static struct fb_var_screeninfo vi;
struct fb_fix_screeninfo fi;
struct timespec tv, tv2;

static void dumpinfo(struct fb_fix_screeninfo *fi,
                     struct fb_var_screeninfo *vi);

static int get_framebuffer(struct simple_fb *fb, unsigned bpp)
{
    int fd;
    void *bits;
    int bytes_per_pixel;

    fd = open("/dev/graphics/fb0", O_RDWR);
    if (fd < 0) {
        printf("cannot open /dev/graphics/fb0, retrying with /dev/fb0\n");
        if ((fd = open("/dev/fb0", O_RDWR)) < 0) {
            perror("cannot open /dev/fb0");
            return -1;
        }
    }

    if(ioctl(fd, FBIOGET_VSCREENINFO, &vi) < 0) {
        perror("failed to get fb0 info");
        return -1;
    }

    if (bpp && vi.bits_per_pixel != bpp) {
        printf("bpp != %d, forcing...\n", bpp);
        vi.bits_per_pixel = bpp;
        if(ioctl(fd, FBIOPUT_VSCREENINFO, &vi) < 0) {
            perror("failed to force bpp");
            return -1;
        }
    }

    if(ioctl(fd, FBIOGET_FSCREENINFO, &fi) < 0) {
        perror("failed to get fb0 info");
        return -1;
    }

    dumpinfo(&fi, &vi);

    bits = mmap(0, fi.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(bits == MAP_FAILED) {
        perror("failed to mmap framebuffer");
        return -1;
    }

    bytes_per_pixel = vi.bits_per_pixel >> 3;

    fb->width = vi.xres;
    fb->height = vi.yres;
    fb->stride = fi.line_length / bytes_per_pixel;
    fb->data = bits;
    fb->bpp = vi.bits_per_pixel;

    fb++;

    fb->width = vi.xres;
    fb->height = vi.yres;
    fb->stride = fi.line_length / bytes_per_pixel;
    fb->data = (void *)((unsigned long)bits +
                        vi.yres * vi.xres * bytes_per_pixel);
    fb->bpp = vi.bits_per_pixel;

    return fd;
}

static void set_active_framebuffer(unsigned n)
{
    if(n > 1) return;
    vi.yres_virtual = vi.yres * 2;
    vi.yoffset = n * vi.yres;
    if(ioctl(gr_fb_fd, FBIOPUT_VSCREENINFO, &vi) < 0) {
        fprintf(stderr,"active fb swap failed!\n");
    } else
        printf("active buffer: %d\n", n);
}

static void dumpinfo(struct fb_fix_screeninfo *fi, struct fb_var_screeninfo *vi)
{
    fprintf(stderr,"vi.xres = %d\n", vi->xres);
    fprintf(stderr,"vi.yres = %d\n", vi->yres);
    fprintf(stderr,"vi.xresv = %d\n", vi->xres_virtual);
    fprintf(stderr,"vi.yresv = %d\n", vi->yres_virtual);
    fprintf(stderr,"vi.xoff = %d\n", vi->xoffset);
    fprintf(stderr,"vi.yoff = %d\n", vi->yoffset);
    fprintf(stderr, "vi.bits_per_pixel = %d\n", vi->bits_per_pixel);

    fprintf(stderr, "fi.line_length = %d\n", fi->line_length);

}

int gr_init(int bpp, int id)
{
    int fd = -1;

    if (!access("/dev/tty0", F_OK)) {
        fd = open("/dev/tty0", O_RDWR | O_SYNC);
        if(fd < 0)
            return -1;

        if(ioctl(fd, KDSETMODE, (void*) KD_GRAPHICS)) {
            close(fd);
            return -1;
        }
    }

    gr_fb_fd = get_framebuffer(gr_fbs, bpp);

    if(gr_fb_fd < 0) {
        if (fd >= 0) {
            ioctl(fd, KDSETMODE, (void*) KD_TEXT);
            close(fd);
        }
        return -1;
    }

    gr_vt_fd = fd;

        /* start with 0 as front (displayed) and 1 as back (drawing) */
    gr_active_fb = id;
    set_active_framebuffer(id);

    return 0;
}

void gr_exit(void)
{
    close(gr_fb_fd);
    gr_fb_fd = -1;

    if (gr_vt_fd >= 0) {
        ioctl(gr_vt_fd, KDSETMODE, (void*) KD_TEXT);
        close(gr_vt_fd);
        gr_vt_fd = -1;
    }
}

int gr_fb_width(void)
{
    return gr_fbs[0].width;
}

int gr_fb_height(void)
{
    return gr_fbs[0].height;
}

uint16_t red = 0xf800;
uint16_t green = 0x07e0;
uint16_t blue = 0x001f;
uint16_t white = 0xffff;
uint16_t black = 0x0;

uint32_t red32 = 0x00ff0000;
uint32_t green32 = 0x0000ff00;
uint32_t blue32 = 0x000000ff;
uint32_t white32 = 0x00ffffff;
uint32_t black32 = 0x0;

void draw_grid(int w, int h, void* _loc) {
    int i, j;
    int v;
    int stride = fi.line_length / (vi.bits_per_pixel >> 3);
    uint16_t *loc = _loc;
    uint32_t *loc32 = _loc;

    for (j = 0; j < h/2; j++) {
        for (i = 0; i < w/2; i++)
            if (vi.bits_per_pixel == 16)
                loc[i + j*(stride)] = red;
            else
                loc32[i + j*(stride)] = red32;
        for (; i < w; i++)
            if (vi.bits_per_pixel == 16)
                loc[i + j*(stride)] = green;
            else
                loc32[i + j*(stride)] = green32;
    }

    for (; j < h; j++) {
        for (i = 0; i < w/2; i++)
            if (vi.bits_per_pixel == 16)
                loc[i + j*(stride)] = blue;
            else
                loc32[i + j*(stride)] = blue32;
        for (; i < w; i++)
            if (vi.bits_per_pixel == 16)
                loc[i + j*(stride)] = white;
            else
                loc32[i + j*(stride)] = white32;
    }

}

void clear_screen(int w, int h, void* _loc)
{
    int i,j;
    int stride = fi.line_length / (vi.bits_per_pixel >> 3);
    uint16_t *loc = _loc;
    uint32_t *loc32 = _loc;

    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++)
            if (vi.bits_per_pixel == 16)
                loc[i + j*(stride)] = black;
            else
                loc32[i + j*(stride)] = black32;
}

int main(int argc, char **argv) {
  int w;
  int h;
  int id = 0;
  int bpp = 0;

  if (argc > 1)
      bpp = atoi(argv[1]);

  if (argc > 4)
      id = !!atoi(argv[4]);

  gr_init(bpp, id);

  if (argc > 3) {
      w = atoi(argv[2]);
      h = atoi(argv[3]);
  } else {
      w = vi.xres;
      h = vi.yres;
  }

  clear_screen(vi.xres, vi.yres, gr_fbs[0].data);
  clear_screen(vi.xres, vi.yres, gr_fbs[1].data);

  draw_grid(w, h, gr_fbs[id].data);

  set_active_framebuffer(!id);
  set_active_framebuffer(id);

  return 0;
}
