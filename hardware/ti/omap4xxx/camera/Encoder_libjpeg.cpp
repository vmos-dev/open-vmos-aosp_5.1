/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
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

/**
* @file Encoder_libjpeg.cpp
*
* This file encodes a YUV422I buffer to a jpeg
* TODO(XXX): Need to support formats other than yuv422i
*            Change interface to pre/post-proc algo framework
*
*/

#define LOG_TAG "CameraHAL"

#include "CameraHal.h"
#include "Encoder_libjpeg.h"
#include "NV12_resize.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

extern "C" {
    #include "jpeglib.h"
    #include "jerror.h"
}

#define ARRAY_SIZE(array) (sizeof((array)) / sizeof((array)[0]))
#define MIN(x,y) ((x < y) ? x : y)

namespace android {
struct integer_string_pair {
    unsigned int integer;
    const char* string;
};

static integer_string_pair degress_to_exif_lut [] = {
    // degrees, exif_orientation
    {0,   "1"},
    {90,  "6"},
    {180, "3"},
    {270, "8"},
};
struct libjpeg_destination_mgr : jpeg_destination_mgr {
    libjpeg_destination_mgr(uint8_t* input, int size);

    uint8_t* buf;
    int bufsize;
    size_t jpegsize;
};

static void libjpeg_init_destination (j_compress_ptr cinfo) {
    libjpeg_destination_mgr* dest = (libjpeg_destination_mgr*)cinfo->dest;

    dest->next_output_byte = dest->buf;
    dest->free_in_buffer = dest->bufsize;
    dest->jpegsize = 0;
}

static boolean libjpeg_empty_output_buffer(j_compress_ptr cinfo) {
    libjpeg_destination_mgr* dest = (libjpeg_destination_mgr*)cinfo->dest;

    dest->next_output_byte = dest->buf;
    dest->free_in_buffer = dest->bufsize;
    return TRUE; // ?
}

static void libjpeg_term_destination (j_compress_ptr cinfo) {
    libjpeg_destination_mgr* dest = (libjpeg_destination_mgr*)cinfo->dest;
    dest->jpegsize = dest->bufsize - dest->free_in_buffer;
}

libjpeg_destination_mgr::libjpeg_destination_mgr(uint8_t* input, int size) {
    this->init_destination = libjpeg_init_destination;
    this->empty_output_buffer = libjpeg_empty_output_buffer;
    this->term_destination = libjpeg_term_destination;

    this->buf = input;
    this->bufsize = size;

    jpegsize = 0;
}

/* private static functions */
static void nv21_to_yuv(uint8_t* dst, uint8_t* y, uint8_t* uv, int width) {
    if (!dst || !y || !uv) {
        return;
    }

    while ((width--) > 0) {
        uint8_t y0 = y[0];
        uint8_t v0 = uv[0];
        uint8_t u0 = *(uv+1);
        dst[0] = y0;
        dst[1] = u0;
        dst[2] = v0;
        dst += 3;
        y++;
        if(!(width % 2)) uv+=2;
    }
}

static void uyvy_to_yuv(uint8_t* dst, uint32_t* src, int width) {
    if (!dst || !src) {
        return;
    }

    if (width % 2) {
        return; // not supporting odd widths
    }

    // currently, neon routine only supports multiple of 16 width
    if (width % 16) {
        while ((width-=2) >= 0) {
            uint8_t u0 = (src[0] >> 0) & 0xFF;
            uint8_t y0 = (src[0] >> 8) & 0xFF;
            uint8_t v0 = (src[0] >> 16) & 0xFF;
            uint8_t y1 = (src[0] >> 24) & 0xFF;
            dst[0] = y0;
            dst[1] = u0;
            dst[2] = v0;
            dst[3] = y1;
            dst[4] = u0;
            dst[5] = v0;
            dst += 6;
            src++;
        }
    } else {
        int n = width;
        asm volatile (
        "   pld [%[src], %[src_stride], lsl #2]                         \n\t"
        "   cmp %[n], #16                                               \n\t"
        "   blt 5f                                                      \n\t"
        "0: @ 16 pixel swap                                             \n\t"
        "   vld2.8  {q0, q1} , [%[src]]! @ q0 = uv q1 = y               \n\t"
        "   vuzp.8 q0, q2                @ d1 = u d5 = v                \n\t"
        "   vmov d1, d0                  @ q0 = u0u1u2..u0u1u2...       \n\t"
        "   vmov d5, d4                  @ q2 = v0v1v2..v0v1v2...       \n\t"
        "   vzip.8 d0, d1                @ q0 = u0u0u1u1u2u2...         \n\t"
        "   vzip.8 d4, d5                @ q2 = v0v0v1v1v2v2...         \n\t"
        "   vswp q0, q1                  @ now q0 = y q1 = u q2 = v     \n\t"
        "   vst3.8  {d0,d2,d4},[%[dst]]!                                \n\t"
        "   vst3.8  {d1,d3,d5},[%[dst]]!                                \n\t"
        "   sub %[n], %[n], #16                                         \n\t"
        "   cmp %[n], #16                                               \n\t"
        "   bge 0b                                                      \n\t"
        "5: @ end                                                       \n\t"
#ifdef NEEDS_ARM_ERRATA_754319_754320
        "   vmov s0,s0  @ add noop for errata item                      \n\t"
#endif
        : [dst] "+r" (dst), [src] "+r" (src), [n] "+r" (n)
        : [src_stride] "r" (width)
        : "cc", "memory", "q0", "q1", "q2"
        );
    }
}

static void resize_nv12(Encoder_libjpeg::params* params, uint8_t* dst_buffer) {
    structConvImage o_img_ptr, i_img_ptr;

    if (!params || !dst_buffer) {
        return;
    }

    //input
    i_img_ptr.uWidth =  params->in_width;
    i_img_ptr.uStride =  i_img_ptr.uWidth;
    i_img_ptr.uHeight =  params->in_height;
    i_img_ptr.eFormat = IC_FORMAT_YCbCr420_lp;
    i_img_ptr.imgPtr = (uint8_t*) params->src;
    i_img_ptr.clrPtr = i_img_ptr.imgPtr + (i_img_ptr.uWidth * i_img_ptr.uHeight);

    //ouput
    o_img_ptr.uWidth = params->out_width;
    o_img_ptr.uStride = o_img_ptr.uWidth;
    o_img_ptr.uHeight = params->out_height;
    o_img_ptr.eFormat = IC_FORMAT_YCbCr420_lp;
    o_img_ptr.imgPtr = dst_buffer;
    o_img_ptr.clrPtr = o_img_ptr.imgPtr + (o_img_ptr.uWidth * o_img_ptr.uHeight);

    VT_resizeFrame_Video_opt2_lp(&i_img_ptr, &o_img_ptr, NULL, 0);
}

/* public static functions */
const char* ExifElementsTable::degreesToExifOrientation(unsigned int degrees) {
    for (unsigned int i = 0; i < ARRAY_SIZE(degress_to_exif_lut); i++) {
        if (degrees == degress_to_exif_lut[i].integer) {
            return degress_to_exif_lut[i].string;
        }
    }
    return NULL;
}

void ExifElementsTable::stringToRational(const char* str, unsigned int* num, unsigned int* den) {
    int len;
    char * tempVal = NULL;

    if (str != NULL) {
        len = strlen(str);
        tempVal = (char*) malloc( sizeof(char) * (len + 1));
    }

    if (tempVal != NULL) {
        // convert the decimal string into a rational
        size_t den_len;
        char *ctx;
        unsigned int numerator = 0;
        unsigned int denominator = 0;
        char* temp = NULL;

        memset(tempVal, '\0', len + 1);
        strncpy(tempVal, str, len);
        temp = strtok_r(tempVal, ".", &ctx);

        if (temp != NULL)
            numerator = atoi(temp);

        if (!numerator)
            numerator = 1;

        temp = strtok_r(NULL, ".", &ctx);
        if (temp != NULL) {
            den_len = strlen(temp);
            if(HUGE_VAL == den_len ) {
                den_len = 0;
            }

            denominator = static_cast<unsigned int>(pow(10, den_len));
            numerator = numerator * denominator + atoi(temp);
        } else {
            denominator = 1;
        }

        free(tempVal);

        *num = numerator;
        *den = denominator;
    }
}

bool ExifElementsTable::isAsciiTag(const char* tag) {
    // TODO(XXX): Add tags as necessary
    return (strcmp(tag, TAG_GPS_PROCESSING_METHOD) == 0);
}

void ExifElementsTable::insertExifToJpeg(unsigned char* jpeg, size_t jpeg_size) {
    ReadMode_t read_mode = (ReadMode_t)(READ_METADATA | READ_IMAGE);

    ResetJpgfile();
    if (ReadJpegSectionsFromBuffer(jpeg, jpeg_size, read_mode)) {
        jpeg_opened = true;
        create_EXIF(table, exif_tag_count, gps_tag_count, has_datetime_tag);
    }
}

status_t ExifElementsTable::insertExifThumbnailImage(const char* thumb, int len) {
    status_t ret = NO_ERROR;

    if ((len > 0) && jpeg_opened) {
        ret = ReplaceThumbnailFromBuffer(thumb, len);
        CAMHAL_LOGDB("insertExifThumbnailImage. ReplaceThumbnail(). ret=%d", ret);
    }

    return ret;
}

void ExifElementsTable::saveJpeg(unsigned char* jpeg, size_t jpeg_size) {
    if (jpeg_opened) {
       WriteJpegToBuffer(jpeg, jpeg_size);
       DiscardData();
       jpeg_opened = false;
    }
}

/* public functions */
ExifElementsTable::~ExifElementsTable() {
    int num_elements = gps_tag_count + exif_tag_count;

    for (int i = 0; i < num_elements; i++) {
        if (table[i].Value) {
            free(table[i].Value);
        }
    }

    if (jpeg_opened) {
        DiscardData();
    }
}

status_t ExifElementsTable::insertElement(const char* tag, const char* value) {
    unsigned int value_length = 0;
    status_t ret = NO_ERROR;

    if (!value || !tag) {
        return -EINVAL;
    }

    if (position >= MAX_EXIF_TAGS_SUPPORTED) {
        CAMHAL_LOGEA("Max number of EXIF elements already inserted");
        return NO_MEMORY;
    }

    if (isAsciiTag(tag)) {
        value_length = sizeof(ExifAsciiPrefix) + strlen(value + sizeof(ExifAsciiPrefix));
    } else {
        value_length = strlen(value);
    }

    if (IsGpsTag(tag)) {
        table[position].GpsTag = TRUE;
        table[position].Tag = GpsTagNameToValue(tag);
        gps_tag_count++;
    } else {
        table[position].GpsTag = FALSE;
        table[position].Tag = TagNameToValue(tag);
        exif_tag_count++;

        if (strcmp(tag, TAG_DATETIME) == 0) {
            has_datetime_tag = true;
        }
    }

    table[position].DataLength = 0;
    table[position].Value = (char*) malloc(sizeof(char) * (value_length + 1));

    if (table[position].Value) {
        memcpy(table[position].Value, value, value_length + 1);
        table[position].DataLength = value_length + 1;
    }

    position++;
    return ret;
}

/* private member functions */
size_t Encoder_libjpeg::encode(params* input) {
    jpeg_compress_struct    cinfo;
    jpeg_error_mgr jerr;
    jpeg_destination_mgr jdest;
    uint8_t* src = NULL, *resize_src = NULL;
    uint8_t* row_tmp = NULL;
    uint8_t* row_src = NULL;
    uint8_t* row_uv = NULL; // used only for NV12
    int out_width = 0, in_width = 0;
    int out_height = 0, in_height = 0;
    int bpp = 2; // for uyvy
    int right_crop = 0, start_offset = 0;

    if (!input) {
        return 0;
    }

    out_width = input->out_width;
    in_width = input->in_width;
    out_height = input->out_height;
    in_height = input->in_height;
    right_crop = input->right_crop;
    start_offset = input->start_offset;
    src = input->src;
    input->jpeg_size = 0;

    libjpeg_destination_mgr dest_mgr(input->dst, input->dst_size);

    // param check...
    if ((in_width < 2) || (out_width < 2) || (in_height < 2) || (out_height < 2) ||
         (src == NULL) || (input->dst == NULL) || (input->quality < 1) || (input->src_size < 1) ||
         (input->dst_size < 1) || (input->format == NULL)) {
        goto exit;
    }

    if (strcmp(input->format, CameraParameters::PIXEL_FORMAT_YUV420SP) == 0) {
        bpp = 1;
        if ((in_width != out_width) || (in_height != out_height)) {
            resize_src = (uint8_t*) malloc(input->dst_size);
            resize_nv12(input, resize_src);
            if (resize_src) src = resize_src;
        }
    } else if ((in_width != out_width) || (in_height != out_height)) {
        CAMHAL_LOGEB("Encoder: resizing is not supported for this format: %s", input->format);
        goto exit;
    } else if (strcmp(input->format, CameraParameters::PIXEL_FORMAT_YUV422I)) {
        // we currently only support yuv422i and yuv420sp
        CAMHAL_LOGEB("Encoder: format not supported: %s", input->format);
        goto exit;
    }

    cinfo.err = jpeg_std_error(&jerr);

    jpeg_create_compress(&cinfo);

    CAMHAL_LOGDB("encoding...  \n\t"
                 "width: %d    \n\t"
                 "height:%d    \n\t"
                 "dest %p      \n\t"
                 "dest size:%d \n\t"
                 "mSrc %p",
                 out_width, out_height, input->dst,
                 input->dst_size, src);

    cinfo.dest = &dest_mgr;
    cinfo.image_width = out_width - right_crop;
    cinfo.image_height = out_height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_YCbCr;
    cinfo.input_gamma = 1;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, input->quality, TRUE);
    cinfo.dct_method = JDCT_IFAST;

    jpeg_start_compress(&cinfo, TRUE);

    row_tmp = (uint8_t*)malloc(out_width * 3);
    row_src = src + start_offset;
    row_uv = src + out_width * out_height * bpp;

    while ((cinfo.next_scanline < cinfo.image_height) && !mCancelEncoding) {
        JSAMPROW row[1];    /* pointer to JSAMPLE row[s] */

        // convert input yuv format to yuv444
        if (strcmp(input->format, CameraParameters::PIXEL_FORMAT_YUV420SP) == 0) {
            nv21_to_yuv(row_tmp, row_src, row_uv, out_width - right_crop);
        } else {
            uyvy_to_yuv(row_tmp, (uint32_t*)row_src, out_width - right_crop);
        }

        row[0] = row_tmp;
        jpeg_write_scanlines(&cinfo, row, 1);
        row_src = row_src + out_width*bpp;

        // move uv row if input format needs it
        if (strcmp(input->format, CameraParameters::PIXEL_FORMAT_YUV420SP) == 0) {
            if (!(cinfo.next_scanline % 2))
                row_uv = row_uv +  out_width * bpp;
        }
    }

    // no need to finish encoding routine if we are prematurely stopping
    // we will end up crashing in dest_mgr since data is incomplete
    if (!mCancelEncoding)
        jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    if (resize_src) free(resize_src);
    if (row_tmp) free(row_tmp);

 exit:
    input->jpeg_size = dest_mgr.jpegsize;
    return dest_mgr.jpegsize;
}

} // namespace android
