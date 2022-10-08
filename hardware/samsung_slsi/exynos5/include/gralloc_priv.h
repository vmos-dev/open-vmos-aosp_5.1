/*
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef GRALLOC_PRIV_H_
#define GRALLOC_PRIV_H_

#include <stdint.h>
#include <limits.h>
#include <sys/cdefs.h>
#include <hardware/gralloc.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>

#include <cutils/native_handle.h>

#include <linux/fb.h>

/*****************************************************************************/

struct private_module_t;
struct private_handle_t;
typedef int ion_user_handle_t;

struct private_module_t {
    gralloc_module_t base;

    struct private_handle_t* framebuffer;
    uint32_t flags;
    uint32_t numBuffers;
    uint32_t bufferMask;
    pthread_mutex_t lock;
    unsigned int refcount;
    buffer_handle_t currentBuffer;
    int ionfd;

    int xres;
    int yres;
    int line_length;
    float xdpi;
    float ydpi;
    float fps;
    void *queue;
    pthread_mutex_t queue_lock;

};

/*****************************************************************************/

#ifdef __cplusplus
struct private_handle_t : public native_handle {
#else
struct private_handle_t {
    struct native_handle nativeHandle;
#endif

// set if using video encoding colorspace
#define GRALLOC_USAGE_PRIVATE_CHROMA (GRALLOC_USAGE_PRIVATE_0)

    enum {
        PRIV_FLAGS_FRAMEBUFFER = 0x00000001,
        PRIV_FLAGS_USES_UMP    = 0x00000002,
        PRIV_FLAGS_USES_ION    = 0x00000020
    };

    // file-descriptors
    int     fd;
    int     fd1;
    int     fd2;
    // ints
    int     magic;
    int     flags;
    int     size;
    int     offset;

    int     format;
    int     width;
    int     height;
    int     stride;
    int     vstride;
    int     gamut;
    int     chroma;

    // FIXME: the attributes below should be out-of-line
    void    *base;
    void    *base1;
    void    *base2;
    ion_user_handle_t handle;
    ion_user_handle_t handle1;
    ion_user_handle_t handle2;

#ifdef __cplusplus
    static const int sNumFds = 3;
    static const int sNumInts = 17;
    static const int sMagic = 0x3141592;


    private_handle_t(int fd, int size, int flags, int w,
		     int h, int format, int stride, int vstride) :
        fd(fd), fd1(-1), fd2(-1), magic(sMagic), flags(flags), size(size),
        offset(0), format(format), width(w), height(h), stride(stride),
        vstride(vstride), gamut(0), chroma(0), base(0), handle(0), handle1(0),
        handle2(0)
    {
        version = sizeof(native_handle);
        numInts = sNumInts + 2;
        numFds = sNumFds - 2;
    }

    private_handle_t(int fd, int fd1, int size, int flags, int w,
		     int h, int format, int stride, int vstride) :
        fd(fd), fd1(fd1), fd2(-1), magic(sMagic), flags(flags), size(size),
        offset(0), format(format), width(w), height(h), stride(stride),
        vstride(vstride), gamut(0), chroma(0), base(0), base1(0), base2(0),
        handle(0), handle1(0), handle2(0)
    {
        version = sizeof(native_handle);
        numInts = sNumInts + 1;
        numFds = sNumFds - 1;
    }

    private_handle_t(int fd, int fd1, int fd2, int size, int flags, int w,
		     int h, int format, int stride, int vstride) :
        fd(fd), fd1(fd1), fd2(fd2), magic(sMagic), flags(flags), size(size),
        offset(0), format(format), width(w), height(h), stride(stride),
        vstride(vstride), gamut(0), chroma(0), base(0), base1(0), base2(0),
        handle(0), handle1(0), handle2(0)
    {
        version = sizeof(native_handle);
        numInts = sNumInts;
        numFds = sNumFds;
    }
    ~private_handle_t() {
        magic = 0;
    }

    static int validate(const native_handle* h) {
        const private_handle_t* hnd = (const private_handle_t*)h;
        if (!h || h->version != sizeof(native_handle) ||
            hnd->numInts + hnd->numFds != sNumInts + sNumFds || 
            hnd->magic != sMagic) 
        {
            ALOGE("invalid gralloc handle (at %p)", reinterpret_cast<void *>(const_cast<native_handle *>(h)));
            return -EINVAL;
        }
        return 0;
    }

    static private_handle_t* dynamicCast(const native_handle* in)
    {
        if (validate(in) == 0)
            return (private_handle_t*) in;

        return NULL;
    }

#endif
};

#endif /* GRALLOC_PRIV_H_ */
