/* Copyright (c) 2012, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
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
 *
 */

#ifndef OMX_JPEG_EXT_H_
#define OMX_JPEG_EXT_H_

#include <OMX_Image.h>
#include "QCamera_Intf.h"
#include "omx_jpeg_common.h"

typedef struct omx_jpeg_pmem_info {
    int fd;
    int offset;
} omx_jpeg_pmem_info;

typedef struct omx_jpeg_exif_info_tag {
    exif_tag_id_t      tag_id;
    exif_tag_entry_t  tag_entry;

} omx_jpeg_exif_info_tag;

typedef struct omx_jpeg_buffer_offset {
    int width;
    int height;
    int yOffset;
    int cbcrOffset;
    int totalSize;
    int paddedFrameSize;
} omx_jpeg_buffer_offset;


#define OMX_JPEG_PREFIX "omx.qcom.jpeg.exttype."
#define OMX_JPEG_PREFIX_LENGTH 22

/*adding to enum also add to the char name array down*/
typedef enum {
    OMX_JPEG_EXT_START = 0x7F000000,
    OMX_JPEG_EXT_EXIF,
    OMX_JPEG_EXT_THUMBNAIL,
    OMX_JPEG_EXT_THUMBNAIL_QUALITY,
    OMX_JPEG_EXT_BUFFER_OFFSET,
    OMX_JPEG_EXT_ACBCR_OFFSET,
    OMX_JPEG_EXT_USER_PREFERENCES,
    OMX_JPEG_EXT_REGION,
    OMX_JPEG_EXT_IMAGE_TYPE,
    OMX_JPEG_EXT_END,
} omx_jpeg_ext_index;

extern char * omx_jpeg_ext_name[];
/*char * omx_jpeg_ext_name[] = {
    "start",
    "exif",
    "thumbnail",
    "thumbnail_quality",
    "buffer_offset",
    "acbcr_offset",
    "user_preferences",
    "region",
    "end"
};*/

/*assume main img scaling*/

typedef struct omx_jpeg_thumbnail {
    int width;
    int height;
    int scaling;
    int cropWidth;
    int cropHeight;
    int left;
    int top;
} omx_jpeg_thumbnail;

typedef struct omx_jpeg_thumbnail_quality {
    OMX_U32 nQFactor;
} omx_jpeg_thumbnail_quality;

typedef struct omx_jpeg_user_preferences {
    omx_jpeg_color_format color_format;
    omx_jpeg_color_format thumbnail_color_format;
    omx_jpeg_preference preference;
} omx_jpeg_user_preferences;

typedef struct omx_jpeg_region{
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
}omx_jpeg_region;

typedef struct omx_jpeg_type{
  omx_jpeg_image_type image_type;
}omx_jpeg_type;

#endif /* OMX_JPEG_EXT_H_ */
