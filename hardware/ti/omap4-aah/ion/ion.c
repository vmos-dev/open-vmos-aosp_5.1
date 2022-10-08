/*
 *  ion.c
 *
 * Memory Allocator functions for ion
 *
 *   Copyright 2011 Google, Inc
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
#define LOG_TAG "ion"

#include <cutils/log.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>

#include <linux/ion.h>
#include <ion/ion.h>

int ion_open()
{
        int fd = open("/dev/ion", O_RDWR);
        if (fd < 0)
                ALOGE("open /dev/ion failed!\n");
        return fd;
}

int ion_close(int fd)
{
        return close(fd);
}

static int ion_ioctl(int fd, int req, void *arg)
{
        int ret = ioctl(fd, req, arg);
        if (ret < 0) {
                ALOGE("ioctl %d failed with code %d: %s\n", req,
                       ret, strerror(errno));
                return -errno;
        }
        return ret;
}

int ion_alloc(int fd, size_t len, size_t align,
	      unsigned int flags, struct ion_handle **handle)
{
        int ret;
        struct ion_allocation_data data = {
                .len = len,
                .align = align,
                .flags = flags,
        };

        ret = ion_ioctl(fd, ION_IOC_ALLOC, &data);
        if (ret < 0)
                return ret;
        *handle = data.handle;
        return ret;
}

int ion_alloc_tiler(int fd, size_t w, size_t h, int fmt, unsigned int flags,
            struct ion_handle **handle, size_t *stride)
{
        int ret;
        struct omap_ion_tiler_alloc_data alloc_data = {
                .w = w,
                .h = h,
                .fmt = fmt,
                .flags = flags,
                .out_align = PAGE_SIZE,
                .token = 0,
        };

        struct ion_custom_data custom_data = {
                .cmd = OMAP_ION_TILER_ALLOC,
                .arg = (unsigned long)(&alloc_data),
        };

        ret = ion_ioctl(fd, ION_IOC_CUSTOM, &custom_data);
        if (ret < 0)
                return ret;
        *stride = alloc_data.stride;
        *handle = alloc_data.handle;
        return ret;
}

int ion_free(int fd, struct ion_handle *handle)
{
        struct ion_handle_data data = {
                .handle = handle,
        };
        return ion_ioctl(fd, ION_IOC_FREE, &data);
}

int ion_map(int fd, struct ion_handle *handle, size_t length, int prot,
            int flags, off_t offset, unsigned char **ptr, int *map_fd)
{
        struct ion_fd_data data = {
                .handle = handle,
        };
        int ret = ion_ioctl(fd, ION_IOC_MAP, &data);
        if (ret < 0)
                return ret;
        *map_fd = data.fd;
        if (*map_fd < 0) {
                ALOGE("map ioctl returned negative fd\n");
                return -EINVAL;
        }
        *ptr = mmap(NULL, length, prot, flags, *map_fd, offset);
        if (*ptr == MAP_FAILED) {
                ALOGE("mmap failed: %s\n", strerror(errno));
                return -errno;
        }
        return ret;
}

int ion_share(int fd, struct ion_handle *handle, int *share_fd)
{
        int map_fd;
        struct ion_fd_data data = {
                .handle = handle,
        };
        int ret = ion_ioctl(fd, ION_IOC_SHARE, &data);
        if (ret < 0)
                return ret;
        *share_fd = data.fd;
        if (*share_fd < 0) {
                ALOGE("map ioctl returned negative fd\n");
                return -EINVAL;
        }
        return ret;
}

int ion_import(int fd, int share_fd, struct ion_handle **handle)
{
        struct ion_fd_data data = {
                .fd = share_fd,
        };
        int ret = ion_ioctl(fd, ION_IOC_IMPORT, &data);
        if (ret < 0)
                return ret;
        *handle = data.handle;
        return ret;
}

#if 0
int ion_sync_fd(int fd, int handle_fd)
{
	struct ion_fd_data data = {
		.fd = handle_fd,
	};
	return ion_ioctl(fd, ION_IOC_SYNC, &data);
}
#endif

int ion_map_cacheable(int fd, struct ion_handle *handle, size_t length, int prot,
            int flags, off_t offset, unsigned char **ptr, int *map_fd)
{
        struct ion_fd_data data = {
                .handle = handle,
                .cacheable = 1,
        };
        int ret = ion_ioctl(fd, ION_IOC_MAP, &data);
        if (ret < 0)
                return ret;
        *map_fd = data.fd;
        if (*map_fd < 0) {
                ALOGE("map ioctl returned negative fd\n");
                return -EINVAL;
        }
        *ptr = mmap(NULL, length, prot, flags, *map_fd, offset);
        if (*ptr == MAP_FAILED) {
                ALOGE("mmap failed: %s\n", strerror(errno));
                return -errno;
        }
        return ret;
}

int ion_flush_cached(int fd, struct ion_handle *handle, size_t length,
            unsigned char *ptr)
{
        struct ion_cached_user_buf_data data = {
                .handle = handle,
                .vaddr = (unsigned long)ptr,
                .size = length,
        };
        return ion_ioctl(fd, ION_IOC_FLUSH_CACHED, &data);
}

int ion_inval_cached(int fd, struct ion_handle *handle, size_t length,
            unsigned char *ptr)
{
        struct ion_cached_user_buf_data data = {
                .handle = handle,
                .vaddr = (unsigned long)ptr,
                .size = length,
        };
        return ion_ioctl(fd, ION_IOC_INVAL_CACHED, &data);
}
