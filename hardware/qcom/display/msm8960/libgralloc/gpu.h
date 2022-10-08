/*
 * Copyright (C) 2008 The Android Open Source Project
 * Copyright (c) 2011-2013, The Linux Foundation. All rights reserved.
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

#ifndef GRALLOC_GPU_H_
#define GRALLOC_GPU_H_

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include <cutils/log.h>
#include <cutils/ashmem.h>

#include "gralloc_priv.h"
#include "fb_priv.h"

namespace gralloc {
class IAllocController;
class gpu_context_t : public alloc_device_t {
    public:
    gpu_context_t(const private_module_t* module,
                  IAllocController* alloc_ctrl);

    int gralloc_alloc_buffer(size_t size, int usage,
                             buffer_handle_t* pHandle,
                             int bufferType, int format,
                             int width, int height);

    int free_impl(private_handle_t const* hnd);

    int alloc_impl(int w, int h, int format, int usage,
                   buffer_handle_t* pHandle, int* pStride,
                   size_t bufferSize = 0);

    static int gralloc_alloc(alloc_device_t* dev, int w, int h,
                             int format, int usage,
                             buffer_handle_t* pHandle,
                             int* pStride);
    int gralloc_alloc_framebuffer_locked(size_t size, int usage,
                                         buffer_handle_t* pHandle);

    int gralloc_alloc_framebuffer(size_t size, int usage,
                                  buffer_handle_t* pHandle);

    static int gralloc_free(alloc_device_t* dev, buffer_handle_t handle);

    static int gralloc_alloc_size(alloc_device_t* dev,
                                  int w, int h, int format,
                                  int usage, buffer_handle_t* pHandle,
                                  int* pStride, int bufferSize);

    static int gralloc_close(struct hw_device_t *dev);

    private:
   IAllocController* mAllocCtrl;
    void getGrallocInformationFromFormat(int inputFormat,
                                         int *bufferType);
};
}
#endif  // GRALLOC_GPU_H
