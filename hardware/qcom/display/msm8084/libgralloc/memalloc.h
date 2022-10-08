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

#ifndef GRALLOC_MEMALLOC_H
#define GRALLOC_MEMALLOC_H

#include <stdlib.h>

namespace gralloc {

enum {
    CACHE_CLEAN = 0x1,
    CACHE_INVALIDATE,
    CACHE_CLEAN_AND_INVALIDATE,
};

struct alloc_data {
    void           *base;
    int            fd;
    size_t         offset;
    size_t         size;
    size_t         align;
    uintptr_t      pHandle;
    bool           uncached;
    unsigned int   flags;
    int            allocType;
};

class IMemAlloc {

    public:
    // Allocate buffer - fill in the alloc_data
    // structure and pass it in. Mapped address
    // and fd are returned in the alloc_data struct
    virtual int alloc_buffer(alloc_data& data) = 0;

    // Free buffer
    virtual int free_buffer(void *base, size_t size,
                            size_t offset, int fd) = 0;

    // Map buffer
    virtual int map_buffer(void **pBase, size_t size,
                           size_t offset, int fd) = 0;

    // Unmap buffer
    virtual int unmap_buffer(void *base, size_t size,
                             size_t offset) = 0;

    // Clean and invalidate
    virtual int clean_buffer(void *base, size_t size,
                             size_t offset, int fd, int op) = 0;

    // Destructor
    virtual ~IMemAlloc() {};

    enum {
        FD_INIT = -1,
    };

};

} // end gralloc namespace
#endif // GRALLOC_MEMALLOC_H
