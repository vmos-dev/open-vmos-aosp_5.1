/*
 * Copyright (C) 2008, The Android Open Source Project
 * Copyright (c) 2011-2012, The Linux Foundation. All rights reserved.
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

#include <unistd.h>
#include <fcntl.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <cutils/properties.h>

#include "gr.h"
#include "gpu.h"
#include "memalloc.h"
#include "alloc_controller.h"

using namespace gralloc;

int fb_device_open(const hw_module_t* module, const char* name,
                   hw_device_t** device);

static int gralloc_device_open(const hw_module_t* module, const char* name,
                               hw_device_t** device);

extern int gralloc_lock(gralloc_module_t const* module,
                        buffer_handle_t handle, int usage,
                        int l, int t, int w, int h,
                        void** vaddr);

extern int gralloc_lock_ycbcr(gralloc_module_t const* module,
                        buffer_handle_t handle, int usage,
                        int l, int t, int w, int h,
                        struct android_ycbcr *ycbcr);

extern int gralloc_unlock(gralloc_module_t const* module,
                          buffer_handle_t handle);

extern int gralloc_register_buffer(gralloc_module_t const* module,
                                   buffer_handle_t handle);

extern int gralloc_unregister_buffer(gralloc_module_t const* module,
                                     buffer_handle_t handle);

extern int gralloc_perform(struct gralloc_module_t const* module,
                           int operation, ... );

// HAL module methods
static struct hw_module_methods_t gralloc_module_methods = {
    open: gralloc_device_open
};

// HAL module initialize
struct private_module_t HAL_MODULE_INFO_SYM = {
    base: {
        common: {
            tag: HARDWARE_MODULE_TAG,
            version_major: 1,
            version_minor: 0,
            id: GRALLOC_HARDWARE_MODULE_ID,
            name: "Graphics Memory Allocator Module",
            author: "The Android Open Source Project",
            methods: &gralloc_module_methods,
            dso: 0,
            reserved: {0},
        },
        registerBuffer: gralloc_register_buffer,
        unregisterBuffer: gralloc_unregister_buffer,
        lock: gralloc_lock,
        unlock: gralloc_unlock,
        perform: gralloc_perform,
        lock_ycbcr: gralloc_lock_ycbcr,
    },
    framebuffer: 0,
    fbFormat: 0,
    flags: 0,
    numBuffers: 0,
    bufferMask: 0,
    lock: PTHREAD_MUTEX_INITIALIZER,
    currentBuffer: 0,
};

// Open Gralloc device
int gralloc_device_open(const hw_module_t* module, const char* name,
                        hw_device_t** device)
{
    int status = -EINVAL;
    if (!strcmp(name, GRALLOC_HARDWARE_GPU0)) {
        const private_module_t* m = reinterpret_cast<const private_module_t*>(
            module);
        gpu_context_t *dev;
        IAllocController* alloc_ctrl = IAllocController::getInstance();
        dev = new gpu_context_t(m, alloc_ctrl);
        *device = &dev->common;
        status = 0;
    } else {
        status = fb_device_open(module, name, device);
    }
    return status;
}
