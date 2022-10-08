#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>

#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include <errno.h>

#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/mman.h>

#include <linux/fb.h>

int64_t systemTime()
{
    struct timespec t;
    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (int64_t)(t.tv_sec)*1000000000LL + t.tv_nsec;
}

int main(int argc, char** argv)
{
    char const * const device_template[] = {
            "/dev/graphics/fb%u",
            "/dev/fb%u",
            0 };
    int fd = -1;
    int i=0;
    int j=0;
    char name[64];
    while ((fd==-1) && device_template[i]) {
        snprintf(name, 64, device_template[i], 0);
        fd = open(name, O_RDWR, 0);
        i++;
    }
    if (fd < 0)
        return -errno;
        
    struct fb_fix_screeninfo finfo;
    if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo) == -1)
        return -errno;
    
    struct fb_var_screeninfo info;
    if (ioctl(fd, FBIOGET_VSCREENINFO, &info) == -1)
        return -errno;
    
    info.reserved[0] = 0;
    info.reserved[1] = 0;
    info.reserved[2] = 0;
    info.xoffset = 0;
    info.yoffset = 0;
    info.bits_per_pixel = 16;
    info.activate = FB_ACTIVATE_NOW;
    
    if (ioctl(fd, FBIOPUT_VSCREENINFO, &info) == -1) {
        printf("FBIOPUT_VSCREENINFO failed (%d x %d)\n",
                info.xres_virtual, info.yres_virtual);
        return 0;
    }
        
    if (ioctl(fd, FBIOGET_VSCREENINFO, &info) == -1)
        return -errno;    
    
    uint64_t denominator = (uint64_t)( info.upper_margin + info.lower_margin + info.yres )
                         * ( info.left_margin  + info.right_margin + info.xres )
                         * info.pixclock;
    int refreshRate = denominator ? (1000000000000000LLU / denominator) : 0;
    
    float xdpi = (info.xres * 25.4f) / info.width; 
    float ydpi = (info.yres * 25.4f) / info.height;
    float fps  = refreshRate / 1000.0f; 
    
    printf( "using (fd=%d)\n"
            "id           = %s\n"
            "xres         = %d px\n"
            "yres         = %d px\n"
            "xres_virtual = %d px\n"
            "yres_virtual = %d px\n"
            "bpp          = %d\n"
            "r            = %2u:%u\n"
            "g            = %2u:%u\n"
            "b            = %2u:%u\n",
                fd,
                finfo.id,
                info.xres,
                info.yres,
                info.xres_virtual,
                info.yres_virtual,
                info.bits_per_pixel,
                info.red.offset, info.red.length,
                info.green.offset, info.green.length,
                info.blue.offset, info.blue.length
        );

    printf( "width        = %d mm (%f dpi)\n"
            "height       = %d mm (%f dpi)\n"
            "refresh rate = %.2f Hz\n",
                info.width,  xdpi,
                info.height, ydpi,
                fps
        );
    
    printf("upper_margin=%d, lower_margin=%d, left_margin=%d, right_margin=%d, pixclock=%d, finfo.smem_len=%d\n",
            info.upper_margin, info.lower_margin, info.left_margin, info.right_margin, info.pixclock, finfo.smem_len);

    if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo) == -1)
        return -errno;
    
    if (finfo.smem_len <= 0)
        return -errno;
    
    /*
     * Open and map the display.
     */
    
    uint16_t* buffer  = (uint16_t*) mmap(
            0, finfo.smem_len,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            fd, 0);
    
    if (buffer == MAP_FAILED)
        return -errno;
    
    // at least for now, always clear the fb
    memset(buffer, 0, finfo.smem_len);
    memset(buffer, 0xff, 320*(info.yres_virtual/2)*2);

    int l,t,w,h;
    l=0;
    t=0;
    w=320;
    h=480;
    info.reserved[0] = 0x54445055; // "UPDT";
    info.reserved[1] = (uint16_t)l | ((uint32_t)t << 16);
    info.reserved[2] = (uint16_t)(l+w) | ((uint32_t)(t+h) << 16);

    int err;
    int c = 0;
    int64_t time = systemTime();
    while (1) {
        
        info.activate = FB_ACTIVATE_VBL;
        info.yoffset = 0;
        ioctl(fd, FBIOPUT_VSCREENINFO, &info);

        info.activate = FB_ACTIVATE_VBL;
        info.yoffset = info.yres_virtual/2;
        err = ioctl(fd, FBIOPUT_VSCREENINFO, &info);

        c+=2;
        if (c==60*2) {
            int64_t now = systemTime();
            time = now - time;
            printf("refresh rate = %f Hz\n", (c*1000000000.0 / (double)time));
            c = 0;
            time = now;
        }
    }
    return 0;
}
