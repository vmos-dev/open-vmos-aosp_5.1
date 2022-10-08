/*
 * Copyright (c) 2011-2014, The Linux Foundation. All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of The Linux Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define DEBUG 0
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <fcntl.h>
#include <cutils/log.h>
#include <errno.h>
#include "gralloc_priv.h"
#include "ionalloc.h"

using gralloc::IonAlloc;

#define ION_DEVICE "/dev/ion"

int IonAlloc::open_device()
{
    if(mIonFd == FD_INIT)
        mIonFd = open(ION_DEVICE, O_RDONLY);

    if(mIonFd < 0 ) {
        ALOGE("%s: Failed to open ion device - %s",
              __FUNCTION__, strerror(errno));
        mIonFd = FD_INIT;
        return -errno;
    }
    return 0;
}

void IonAlloc::close_device()
{
    if(mIonFd >= 0)
        close(mIonFd);
    mIonFd = FD_INIT;
}

int IonAlloc::alloc_buffer(alloc_data& data)
{
    Locker::Autolock _l(mLock);
    int err = 0;
    struct ion_handle_data handle_data;
    struct ion_fd_data fd_data;
    struct ion_allocation_data ionAllocData;
    void *base = 0;

    ionAllocData.len = data.size;
    ionAllocData.align = data.align;
    ionAllocData.heap_mask = data.flags & ~ION_SECURE;
    ionAllocData.flags = data.uncached ? 0 : ION_FLAG_CACHED;
    // ToDo: replace usage of alloc data structure with
    //  ionallocdata structure.
    if (data.flags & ION_SECURE)
        ionAllocData.flags |= ION_SECURE;

    err = open_device();
    if (err)
        return err;
    if(ioctl(mIonFd, ION_IOC_ALLOC, &ionAllocData)) {
        err = -errno;
        ALOGE("ION_IOC_ALLOC failed with error - %s", strerror(errno));
        return err;
    }

    fd_data.handle = ionAllocData.handle;
    handle_data.handle = ionAllocData.handle;
    if(ioctl(mIonFd, ION_IOC_MAP, &fd_data)) {
        err = -errno;
        ALOGE("%s: ION_IOC_MAP failed with error - %s",
              __FUNCTION__, strerror(errno));
        ioctl(mIonFd, ION_IOC_FREE, &handle_data);
        return err;
    }

    if(!(data.flags & ION_SECURE)) {
        base = mmap(0, ionAllocData.len, PROT_READ|PROT_WRITE,
                    MAP_SHARED, fd_data.fd, 0);
        if(base == MAP_FAILED) {
            err = -errno;
            ALOGE("%s: Failed to map the allocated memory: %s",
                  __FUNCTION__, strerror(errno));
            ioctl(mIonFd, ION_IOC_FREE, &handle_data);
            return err;
        }
    }

    data.base = base;
    data.fd = fd_data.fd;
    ioctl(mIonFd, ION_IOC_FREE, &handle_data);
    ALOGD_IF(DEBUG, "ion: Allocated buffer base:%p size:%zu fd:%d",
          data.base, ionAllocData.len, data.fd);
    return 0;
}


int IonAlloc::free_buffer(void* base, size_t size, size_t offset, int fd)
{
    Locker::Autolock _l(mLock);
    ALOGD_IF(DEBUG, "ion: Freeing buffer base:%p size:%zu fd:%d",
          base, size, fd);
    int err = 0;
    err = open_device();
    if (err)
        return err;

    if(base)
        err = unmap_buffer(base, size, offset);
    close(fd);
    return err;
}

int IonAlloc::map_buffer(void **pBase, size_t size, size_t offset, int fd)
{
    int err = 0;
    void *base = 0;
    // It is a (quirky) requirement of ION to have opened the
    // ion fd in the process that is doing the mapping
    err = open_device();
    if (err)
        return err;

    base = mmap(0, size, PROT_READ| PROT_WRITE,
                MAP_SHARED, fd, 0);
    *pBase = base;
    if(base == MAP_FAILED) {
        err = -errno;
        ALOGE("ion: Failed to map memory in the client: %s",
              strerror(errno));
    } else {
        ALOGD_IF(DEBUG, "ion: Mapped buffer base:%p size:%zu offset:%d fd:%d",
              base, size, offset, fd);
    }
    return err;
}

int IonAlloc::unmap_buffer(void *base, size_t size, size_t /*offset*/)
{
    ALOGD_IF(DEBUG, "ion: Unmapping buffer  base:%p size:%zu", base, size);
    int err = 0;
    if(munmap(base, size)) {
        err = -errno;
        ALOGE("ion: Failed to unmap memory at %p : %s",
              base, strerror(errno));
    }
    return err;

}
int IonAlloc::clean_buffer(void *base, size_t size, size_t offset, int fd, int op)
{
    struct ion_flush_data flush_data;
    struct ion_fd_data fd_data;
    struct ion_handle_data handle_data;
    int err = 0;

    err = open_device();
    if (err)
        return err;

    fd_data.fd = fd;
    if (ioctl(mIonFd, ION_IOC_IMPORT, &fd_data)) {
        err = -errno;
        ALOGE("%s: ION_IOC_IMPORT failed with error - %s",
              __FUNCTION__, strerror(errno));
        return err;
    }

    handle_data.handle = fd_data.handle;
    flush_data.handle  = fd_data.handle;
    flush_data.vaddr   = base;
    // offset and length are uint32_t
    flush_data.offset  = (uint32_t) offset;
    flush_data.length  = (uint32_t) size;

    struct ion_custom_data d;
    switch(op) {
    case CACHE_CLEAN:
        d.cmd = ION_IOC_CLEAN_CACHES;
        break;
    case CACHE_INVALIDATE:
            d.cmd = ION_IOC_INV_CACHES;
        break;
    case CACHE_CLEAN_AND_INVALIDATE:
    default:
        d.cmd = ION_IOC_CLEAN_INV_CACHES;
    }

    d.arg = (unsigned long int)&flush_data;

    if(ioctl(mIonFd, ION_IOC_CUSTOM, &d)) {
        err = -errno;
        ALOGE("%s: ION_IOC_CLEAN_INV_CACHES failed with error - %s",

              __FUNCTION__, strerror(errno));
        ioctl(mIonFd, ION_IOC_FREE, &handle_data);
        return err;
    }
    ioctl(mIonFd, ION_IOC_FREE, &handle_data);
    return 0;
}

