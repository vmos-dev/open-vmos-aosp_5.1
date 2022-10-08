/*
 * Copyright (C) 2010 The Android Open Source Project
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

#include "Decoder_libjpeg.h"

extern "C" {
    #include "jpeglib.h"
    #include "jerror.h"
}

#define NUM_COMPONENTS_IN_YUV 3

namespace Ti {
namespace Camera {

/* JPEG DHT Segment omitted from MJPEG data */
static unsigned char jpeg_odml_dht[0x1a6] = {
    0xff, 0xd8, /* Start of Image */
    0xff, 0xc4, 0x01, 0xa2, /* Define Huffman Table */

    0x00, 0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,

    0x01, 0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,

    0x10, 0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03, 0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7d,
    0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
    0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08, 0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
    0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
    0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
    0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
    0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
    0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
    0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
    0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
    0xf9, 0xfa,

    0x11, 0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04, 0x07, 0x05, 0x04, 0x04, 0x00, 0x01, 0x02, 0x77,
    0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
    0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
    0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34, 0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
    0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
    0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
    0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
    0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
    0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
    0xf9, 0xfa
};

struct libjpeg_source_mgr : jpeg_source_mgr {
    libjpeg_source_mgr(unsigned char *buffer_ptr, int len);
    ~libjpeg_source_mgr();

    unsigned char *mBufferPtr;
    int mFilledLen;
};

static void libjpeg_init_source(j_decompress_ptr cinfo) {
    libjpeg_source_mgr*  src = (libjpeg_source_mgr*)cinfo->src;
    src->next_input_byte = (const JOCTET*)src->mBufferPtr;
    src->bytes_in_buffer = 0;
    src->current_offset = 0;
}

static boolean libjpeg_seek_input_data(j_decompress_ptr cinfo, long byte_offset) {
    libjpeg_source_mgr* src = (libjpeg_source_mgr*)cinfo->src;
    src->current_offset = byte_offset;
    src->next_input_byte = (const JOCTET*)src->mBufferPtr + byte_offset;
    src->bytes_in_buffer = 0;
    return TRUE;
}

static boolean libjpeg_fill_input_buffer(j_decompress_ptr cinfo) {
    libjpeg_source_mgr* src = (libjpeg_source_mgr*)cinfo->src;
    src->current_offset += src->mFilledLen;
    src->next_input_byte = src->mBufferPtr;
    src->bytes_in_buffer = src->mFilledLen;
    return TRUE;
}

static void libjpeg_skip_input_data(j_decompress_ptr cinfo, long num_bytes) {
    libjpeg_source_mgr*  src = (libjpeg_source_mgr*)cinfo->src;

    if (num_bytes > (long)src->bytes_in_buffer) {
        CAMHAL_LOGEA("\n\n\n libjpeg_skip_input_data - num_bytes > (long)src->bytes_in_buffer \n\n\n");
    } else {
        src->next_input_byte += num_bytes;
        src->bytes_in_buffer -= num_bytes;
    }
}

static boolean libjpeg_resync_to_restart(j_decompress_ptr cinfo, int desired) {
    libjpeg_source_mgr*  src = (libjpeg_source_mgr*)cinfo->src;
    src->next_input_byte = (const JOCTET*)src->mBufferPtr;
    src->bytes_in_buffer = 0;
    return TRUE;
}

static void libjpeg_term_source(j_decompress_ptr /*cinfo*/) {}

libjpeg_source_mgr::libjpeg_source_mgr(unsigned char *buffer_ptr, int len) : mBufferPtr(buffer_ptr), mFilledLen(len) {
    init_source = libjpeg_init_source;
    fill_input_buffer = libjpeg_fill_input_buffer;
    skip_input_data = libjpeg_skip_input_data;
    resync_to_restart = libjpeg_resync_to_restart;
    term_source = libjpeg_term_source;
    seek_input_data = libjpeg_seek_input_data;
}

libjpeg_source_mgr::~libjpeg_source_mgr() {}

Decoder_libjpeg::Decoder_libjpeg()
{
    mWidth = 0;
    mHeight = 0;
    Y_Plane = NULL;
    U_Plane = NULL;
    V_Plane = NULL;
    UV_Plane = NULL;
}

Decoder_libjpeg::~Decoder_libjpeg()
{
    release();
}

void Decoder_libjpeg::release()
{
    if (Y_Plane) {
        free(Y_Plane);
        Y_Plane = NULL;
    }
    if (U_Plane) {
        free(U_Plane);
        U_Plane = NULL;
    }
    if (V_Plane) {
        free(V_Plane);
        V_Plane = NULL;
    }
    if (UV_Plane) {
        free(UV_Plane);
        UV_Plane = NULL;
    }
}

int Decoder_libjpeg::readDHTSize()
{
    return sizeof(jpeg_odml_dht);
}

int Decoder_libjpeg::appendDHT(unsigned char *jpeg_src, int filled_len, unsigned char *jpeg_with_dht_buffer, int buff_size)
{
    /* Appending DHT to JPEG */

    int len = filled_len + sizeof(jpeg_odml_dht) - 2; // final length of jpeg data
    if (len > buff_size)  {
        CAMHAL_LOGEA("\n\n\n Buffer size too small. filled_len=%d, buff_size=%d, sizeof(jpeg_odml_dht)=%d\n\n\n", filled_len, buff_size, sizeof(jpeg_odml_dht));
        return 0;
    }

    memcpy(jpeg_with_dht_buffer, jpeg_odml_dht, sizeof(jpeg_odml_dht));
    memcpy((jpeg_with_dht_buffer + sizeof(jpeg_odml_dht)), jpeg_src + 2, (filled_len - 2));
    return len;
}


bool Decoder_libjpeg::decode(unsigned char *jpeg_src, int filled_len, unsigned char *nv12_buffer, int stride)
{
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    struct libjpeg_source_mgr s_mgr(jpeg_src, filled_len);

    if (filled_len == 0)
        return false;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);

    cinfo.src = &s_mgr;
    int status = jpeg_read_header(&cinfo, true);
    if (status != JPEG_HEADER_OK) {
        CAMHAL_LOGEA("jpeg header corrupted");
        return false;
    }

    cinfo.out_color_space = JCS_YCbCr;
    cinfo.raw_data_out = true;
    status = jpeg_start_decompress(&cinfo);
    if (!status){
        CAMHAL_LOGEA("jpeg_start_decompress failed");
        return false;
    }

    if (mWidth == 0){
        mWidth = cinfo.output_width;
        mHeight = cinfo.output_height;
        CAMHAL_LOGEA("w x h = %d x %d. stride=%d", cinfo.output_width, cinfo.output_height, stride);
    }
    else if ((cinfo.output_width > mWidth) || (cinfo.output_height > mHeight)) {
        CAMHAL_LOGEA(" Free the existing buffers so that they are reallocated for new w x h. Old WxH = %dx%d. New WxH = %dx%d",
        mWidth, mHeight, cinfo.output_width, cinfo.output_height);
        release();
    }

    unsigned int decoded_uv_buffer_size = cinfo.output_width * cinfo.output_height / 2;
    if (Y_Plane == NULL)Y_Plane = (unsigned char **)malloc(cinfo.output_height * sizeof(unsigned char *));
    if (U_Plane == NULL)U_Plane = (unsigned char **)malloc(cinfo.output_height * sizeof(unsigned char *));
    if (V_Plane == NULL)V_Plane = (unsigned char **)malloc(cinfo.output_height * sizeof(unsigned char *));
    if (UV_Plane == NULL) UV_Plane = (unsigned char *)malloc(decoded_uv_buffer_size);

    unsigned char **YUV_Planes[NUM_COMPONENTS_IN_YUV];
    YUV_Planes[0] = Y_Plane;
    YUV_Planes[1] = U_Plane;
    YUV_Planes[2] = V_Plane;

    unsigned char *row = &nv12_buffer[0];

    // Y Component
    for (unsigned int j = 0; j < cinfo.output_height; j++, row += stride)
        YUV_Planes[0][j] = row;

    row = &UV_Plane[0];

    // U Component
    for (unsigned int j = 0; j < cinfo.output_height; j+=2, row += cinfo.output_width / 2){
        YUV_Planes[1][j+0] = row;
        YUV_Planes[1][j+1] = row;
    }

    // V Component
    for (unsigned int j = 0; j < cinfo.output_height; j+=2, row += cinfo.output_width / 2){
        YUV_Planes[2][j+0] = row;
        YUV_Planes[2][j+1] = row;
    }

    // Interleaving U and V
    for (unsigned int i = 0; i < cinfo.output_height; i += 8) {
        jpeg_read_raw_data(&cinfo, YUV_Planes, 8);
        YUV_Planes[0] += 8;
        YUV_Planes[1] += 8;
        YUV_Planes[2] += 8;
    }

    unsigned char *uv_ptr = nv12_buffer + (stride * cinfo.output_height);
    unsigned char *u_ptr = UV_Plane;
    unsigned char *v_ptr = UV_Plane + (decoded_uv_buffer_size / 2);
    for(unsigned int i = 0; i < cinfo.output_height / 2; i++){
        for(unsigned int j = 0; j < cinfo.output_width; j+=2){
            *(uv_ptr + j) = *u_ptr; u_ptr++;
            *(uv_ptr + j + 1) = *v_ptr; v_ptr++;
        }
        uv_ptr = uv_ptr + stride;
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    return true;
}

} // namespace Camera
} // namespace Ti

