/*
 * Copyright (C) 2008 The Android Open Source Project
 * Copyright (c) 2011-2014, The Linux Foundation. All rights reserved.
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

#include <cutils/log.h>

#define ROUND_UP_PAGESIZE(x) ( (((unsigned long)(x)) + PAGE_SIZE-1)  & \
                               (~(PAGE_SIZE-1)) )

enum {
    /* gralloc usage bits indicating the type
     * of allocation that should be used */

    /* SYSTEM heap comes from kernel vmalloc,
     * can never be uncached, is not secured*/
    GRALLOC_USAGE_PRIVATE_SYSTEM_HEAP     =       GRALLOC_USAGE_PRIVATE_0,
    /* SF heap is used for application buffers, is not secured */
    GRALLOC_USAGE_PRIVATE_UI_CONTIG_HEAP  =       GRALLOC_USAGE_PRIVATE_1,
    /* IOMMU heap comes from manually allocated pages,
     * can be cached/uncached, is not secured */
    GRALLOC_USAGE_PRIVATE_IOMMU_HEAP      =       GRALLOC_USAGE_PRIVATE_2,
    /* MM heap is a carveout heap for video, can be secured*/
    GRALLOC_USAGE_PRIVATE_MM_HEAP         =       GRALLOC_USAGE_PRIVATE_3,
    /* ADSP heap is a carveout heap, is not secured*/
    GRALLOC_USAGE_PRIVATE_ADSP_HEAP       =       0x01000000,

    /* Set this for allocating uncached memory (using O_DSYNC)
     * cannot be used with noncontiguous heaps */
    GRALLOC_USAGE_PRIVATE_UNCACHED        =       0x02000000,

    /* Buffer content should be displayed on an primary display only */
    GRALLOC_USAGE_PRIVATE_INTERNAL_ONLY   =       0x04000000,

    /* Buffer content should be displayed on an external display only */
    GRALLOC_USAGE_PRIVATE_EXTERNAL_ONLY   =       0x08000000,

    /* This flag is set for WFD usecase */
    GRALLOC_USAGE_PRIVATE_WFD             =       0x00200000,

    /* CAMERA heap is a carveout heap for camera, is not secured*/
    GRALLOC_USAGE_PRIVATE_CAMERA_HEAP     =       0x00400000,

    /* This flag is used for SECURE display usecase */
    GRALLOC_USAGE_PRIVATE_SECURE_DISPLAY  =       0x00800000,
};

enum {
    /* Gralloc perform enums
    */
    GRALLOC_MODULE_PERFORM_CREATE_HANDLE_FROM_BUFFER = 1,
    // This will be deprecated from latest graphics drivers. This is kept
    // for those backward compatibility i.e., newer Display HAL + older graphics
    // libraries
    GRALLOC_MODULE_PERFORM_GET_STRIDE,
    GRALLOC_MODULE_PERFORM_GET_CUSTOM_STRIDE_FROM_HANDLE,
    GRALLOC_MODULE_PERFORM_GET_CUSTOM_STRIDE_AND_HEIGHT_FROM_HANDLE,
    GRALLOC_MODULE_PERFORM_GET_ATTRIBUTES,
    GRALLOC_MODULE_PERFORM_GET_COLOR_SPACE_FROM_HANDLE,
    GRALLOC_MODULE_PERFORM_GET_YUV_PLANE_INFO,
};

#define GRALLOC_HEAP_MASK   (GRALLOC_USAGE_PRIVATE_UI_CONTIG_HEAP |\
                             GRALLOC_USAGE_PRIVATE_SYSTEM_HEAP    |\
                             GRALLOC_USAGE_PRIVATE_IOMMU_HEAP     |\
                             GRALLOC_USAGE_PRIVATE_MM_HEAP        |\
                             GRALLOC_USAGE_PRIVATE_ADSP_HEAP)

#define INTERLACE_MASK 0x80
#define S3D_FORMAT_MASK 0xFF000
/*****************************************************************************/
enum {
    /* OEM specific HAL formats */
    HAL_PIXEL_FORMAT_NV12_ENCODEABLE        = 0x102,
    HAL_PIXEL_FORMAT_YCbCr_420_SP_VENUS     = 0x7FA30C04,
    HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED     = 0x7FA30C03,
    HAL_PIXEL_FORMAT_YCbCr_420_SP           = 0x109,
    HAL_PIXEL_FORMAT_YCrCb_420_SP_ADRENO    = 0x7FA30C01,
    HAL_PIXEL_FORMAT_YCrCb_422_SP           = 0x10B,
    HAL_PIXEL_FORMAT_R_8                    = 0x10D,
    HAL_PIXEL_FORMAT_RG_88                  = 0x10E,
    HAL_PIXEL_FORMAT_YCbCr_444_SP           = 0x10F,
    HAL_PIXEL_FORMAT_YCrCb_444_SP           = 0x110,
    HAL_PIXEL_FORMAT_YCrCb_422_I            = 0x111,
    HAL_PIXEL_FORMAT_BGRX_8888              = 0x112,
    HAL_PIXEL_FORMAT_NV21_ZSL               = 0x113,
    HAL_PIXEL_FORMAT_INTERLACE              = 0x180,
    //v4l2_fourcc('Y', 'U', 'Y', 'L'). 24 bpp YUYV 4:2:2 10 bit per component
    HAL_PIXEL_FORMAT_YCbCr_422_I_10BIT      = 0x4C595559,
    //v4l2_fourcc('Y', 'B', 'W', 'C'). 10 bit per component. This compressed
    //format reduces the memory access bandwidth
    HAL_PIXEL_FORMAT_YCbCr_422_I_10BIT_COMPRESSED = 0x43574259,

    //Khronos ASTC formats
    HAL_PIXEL_FORMAT_COMPRESSED_RGBA_ASTC_4x4_KHR    = 0x93B0,
    HAL_PIXEL_FORMAT_COMPRESSED_RGBA_ASTC_5x4_KHR    = 0x93B1,
    HAL_PIXEL_FORMAT_COMPRESSED_RGBA_ASTC_5x5_KHR    = 0x93B2,
    HAL_PIXEL_FORMAT_COMPRESSED_RGBA_ASTC_6x5_KHR    = 0x93B3,
    HAL_PIXEL_FORMAT_COMPRESSED_RGBA_ASTC_6x6_KHR    = 0x93B4,
    HAL_PIXEL_FORMAT_COMPRESSED_RGBA_ASTC_8x5_KHR    = 0x93B5,
    HAL_PIXEL_FORMAT_COMPRESSED_RGBA_ASTC_8x6_KHR    = 0x93B6,
    HAL_PIXEL_FORMAT_COMPRESSED_RGBA_ASTC_8x8_KHR    = 0x93B7,
    HAL_PIXEL_FORMAT_COMPRESSED_RGBA_ASTC_10x5_KHR   = 0x93B8,
    HAL_PIXEL_FORMAT_COMPRESSED_RGBA_ASTC_10x6_KHR   = 0x93B9,
    HAL_PIXEL_FORMAT_COMPRESSED_RGBA_ASTC_10x8_KHR   = 0x93BA,
    HAL_PIXEL_FORMAT_COMPRESSED_RGBA_ASTC_10x10_KHR  = 0x93BB,
    HAL_PIXEL_FORMAT_COMPRESSED_RGBA_ASTC_12x10_KHR  = 0x93BC,
    HAL_PIXEL_FORMAT_COMPRESSED_RGBA_ASTC_12x12_KHR  = 0x93BD,
    HAL_PIXEL_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR    = 0x93D0,
    HAL_PIXEL_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR    = 0x93D1,
    HAL_PIXEL_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR    = 0x93D2,
    HAL_PIXEL_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR    = 0x93D3,
    HAL_PIXEL_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR    = 0x93D4,
    HAL_PIXEL_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR    = 0x93D5,
    HAL_PIXEL_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR    = 0x93D6,
    HAL_PIXEL_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR    = 0x93D7,
    HAL_PIXEL_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR   = 0x93D8,
    HAL_PIXEL_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR   = 0x93D9,
    HAL_PIXEL_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR   = 0x93DA,
    HAL_PIXEL_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR  = 0x93DB,
    HAL_PIXEL_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR  = 0x93DC,
    HAL_PIXEL_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR  = 0x93DD,
};

/* possible formats for 3D content*/
enum {
    HAL_NO_3D                         = 0x0000,
    HAL_3D_IN_SIDE_BY_SIDE_L_R        = 0x10000,
    HAL_3D_IN_TOP_BOTTOM              = 0x20000,
    HAL_3D_IN_INTERLEAVE              = 0x40000,
    HAL_3D_IN_SIDE_BY_SIDE_R_L        = 0x80000,
    HAL_3D_OUT_SIDE_BY_SIDE           = 0x1000,
    HAL_3D_OUT_TOP_BOTTOM             = 0x2000,
    HAL_3D_OUT_INTERLEAVE             = 0x4000,
    HAL_3D_OUT_MONOSCOPIC             = 0x8000
};

enum {
    BUFFER_TYPE_UI = 0,
    BUFFER_TYPE_VIDEO
};

/*****************************************************************************/

#ifdef __cplusplus
struct private_handle_t : public native_handle {
#else
    struct private_handle_t {
        native_handle_t nativeHandle;
#endif
        enum {
            PRIV_FLAGS_FRAMEBUFFER        = 0x00000001,
            PRIV_FLAGS_USES_PMEM          = 0x00000002,
            PRIV_FLAGS_USES_PMEM_ADSP     = 0x00000004,
            PRIV_FLAGS_USES_ION           = 0x00000008,
            PRIV_FLAGS_USES_ASHMEM        = 0x00000010,
            PRIV_FLAGS_NEEDS_FLUSH        = 0x00000020,
            PRIV_FLAGS_DO_NOT_FLUSH       = 0x00000040,
            PRIV_FLAGS_SW_LOCK            = 0x00000080,
            PRIV_FLAGS_NONCONTIGUOUS_MEM  = 0x00000100,
            // Set by HWC when storing the handle
            PRIV_FLAGS_HWC_LOCK           = 0x00000200,
            PRIV_FLAGS_SECURE_BUFFER      = 0x00000400,
            // For explicit synchronization
            PRIV_FLAGS_UNSYNCHRONIZED     = 0x00000800,
            // Not mapped in userspace
            PRIV_FLAGS_NOT_MAPPED         = 0x00001000,
            // Display on external only
            PRIV_FLAGS_EXTERNAL_ONLY      = 0x00002000,
            PRIV_FLAGS_VIDEO_ENCODER      = 0x00010000,
            PRIV_FLAGS_CAMERA_WRITE       = 0x00020000,
            PRIV_FLAGS_CAMERA_READ        = 0x00040000,
            PRIV_FLAGS_HW_COMPOSER        = 0x00080000,
            PRIV_FLAGS_HW_TEXTURE         = 0x00100000,
            PRIV_FLAGS_ITU_R_601          = 0x00200000,
            PRIV_FLAGS_ITU_R_601_FR       = 0x00400000,
            PRIV_FLAGS_ITU_R_709          = 0x00800000,
            PRIV_FLAGS_SECURE_DISPLAY     = 0x01000000,
            // Buffer is rendered in Tile Format
            PRIV_FLAGS_TILE_RENDERED      = 0x02000000
        };

        // file-descriptors
        int     fd;
        int     fd_metadata;          // fd for the meta-data
        // ints
        int     magic;
        int     flags;
        size_t  size;
        size_t  offset;
        int     bufferType;
        uintptr_t base;
        size_t  offset_metadata;
        // The gpu address mapped into the mmu.
        uintptr_t gpuaddr;
        int     format;
        int     width;
        int     height;
        uintptr_t base_metadata;

#ifdef __cplusplus
        //TODO64: Revisit this on 64-bit
        static const int sNumInts = (6 + (3 * (sizeof(size_t)/sizeof(int))) +
                                    (3 * (sizeof(uintptr_t)/sizeof(int))));
        static const int sNumFds = 2;
        static const int sMagic = 'gmsm';

        private_handle_t(int fd, size_t size, int flags, int bufferType,
                         int format, int width, int height, int eFd = -1,
                         size_t eOffset = 0, uintptr_t eBase = 0) :
            fd(fd), fd_metadata(eFd), magic(sMagic),
            flags(flags), size(size), offset(0), bufferType(bufferType),
            base(0), offset_metadata(eOffset), gpuaddr(0),
            format(format), width(width), height(height),
            base_metadata(eBase)
        {
            version = (int) sizeof(native_handle);
            numInts = sNumInts;
            numFds = sNumFds;
        }
        ~private_handle_t() {
            magic = 0;
        }

        bool usesPhysicallyContiguousMemory() {
            return (flags & PRIV_FLAGS_USES_PMEM) != 0;
        }

        static int validate(const native_handle* h) {
            const private_handle_t* hnd = (const private_handle_t*)h;
            if (!h || h->version != sizeof(native_handle) ||
                h->numInts != sNumInts || h->numFds != sNumFds ||
                hnd->magic != sMagic)
            {
                ALOGD("Invalid gralloc handle (at %p): "
                      "ver(%d/%zu) ints(%d/%d) fds(%d/%d)"
                      "magic(%c%c%c%c/%c%c%c%c)",
                      h,
                      h ? h->version : -1, sizeof(native_handle),
                      h ? h->numInts : -1, sNumInts,
                      h ? h->numFds : -1, sNumFds,
                      hnd ? (((hnd->magic >> 24) & 0xFF)?
                             ((hnd->magic >> 24) & 0xFF) : '-') : '?',
                      hnd ? (((hnd->magic >> 16) & 0xFF)?
                             ((hnd->magic >> 16) & 0xFF) : '-') : '?',
                      hnd ? (((hnd->magic >> 8) & 0xFF)?
                             ((hnd->magic >> 8) & 0xFF) : '-') : '?',
                      hnd ? (((hnd->magic >> 0) & 0xFF)?
                             ((hnd->magic >> 0) & 0xFF) : '-') : '?',
                      (sMagic >> 24) & 0xFF,
                      (sMagic >> 16) & 0xFF,
                      (sMagic >> 8) & 0xFF,
                      (sMagic >> 0) & 0xFF);
                return -EINVAL;
            }
            return 0;
        }

        static private_handle_t* dynamicCast(const native_handle* in) {
            if (validate(in) == 0) {
                return (private_handle_t*) in;
            }
            return NULL;
        }
#endif
    };

#endif /* GRALLOC_PRIV_H_ */
