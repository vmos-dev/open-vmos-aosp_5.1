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

#ifndef MM_JPEG_INTERFACE_H_
#define MM_JPEG_INTERFACE_H_
#include "QOMX_JpegExtensions.h"
#include "cam_intf.h"

#define MM_JPEG_MAX_PLANES 3
#define MM_JPEG_MAX_BUF CAM_MAX_NUM_BUFS_PER_STREAM

typedef enum {
  MM_JPEG_FMT_YUV,
  MM_JPEG_FMT_BITSTREAM
} mm_jpeg_format_t;

typedef struct {
  uint32_t sequence;          /* for jpeg bit streams, assembling is based on sequence. sequence starts from 0 */
  uint8_t *buf_vaddr;        /* ptr to buf */
  int fd;                    /* fd of buf */
  uint32_t buf_size;         /* total size of buf (header + image) */
  mm_jpeg_format_t format;   /* buffer format*/
  cam_frame_len_offset_t offset; /* offset of all the planes */
  int index; /* index used to identify the buffers */
} mm_jpeg_buf_t;

typedef struct {
  uint8_t *buf_vaddr;        /* ptr to buf */
  int fd;                    /* fd of buf */
  uint32_t buf_filled_len;   /* used for output image. filled by the client */
} mm_jpeg_output_t;

typedef enum {
  MM_JPEG_COLOR_FORMAT_YCRCBLP_H2V2,
  MM_JPEG_COLOR_FORMAT_YCBCRLP_H2V2,
  MM_JPEG_COLOR_FORMAT_YCRCBLP_H2V1,
  MM_JPEG_COLOR_FORMAT_YCBCRLP_H2V1,
  MM_JPEG_COLOR_FORMAT_YCRCBLP_H1V2,
  MM_JPEG_COLOR_FORMAT_YCBCRLP_H1V2,
  MM_JPEG_COLOR_FORMAT_YCRCBLP_H1V1,
  MM_JPEG_COLOR_FORMAT_YCBCRLP_H1V1,
  MM_JPEG_COLOR_FORMAT_BITSTREAM_H2V2,
  MM_JPEG_COLOR_FORMAT_BITSTREAM_H2V1,
  MM_JPEG_COLOR_FORMAT_BITSTREAM_H1V2,
  MM_JPEG_COLOR_FORMAT_BITSTREAM_H1V1,
  MM_JPEG_COLOR_FORMAT_MAX
} mm_jpeg_color_format;

typedef enum {
  JPEG_JOB_STATUS_DONE = 0,
  JPEG_JOB_STATUS_ERROR
} jpeg_job_status_t;

typedef void (*jpeg_encode_callback_t)(jpeg_job_status_t status,
  uint32_t client_hdl,
  uint32_t jobId,
  mm_jpeg_output_t *p_output,
  void *userData);

typedef struct {
  /* src img dimension */
  cam_dimension_t src_dim;

  /* jpeg output dimension */
  cam_dimension_t dst_dim;

  /* crop information */
  cam_rect_t crop;
} mm_jpeg_dim_t;

typedef struct {
  /* num of buf in src img */
  uint32_t num_src_bufs;

  /* num of src tmb bufs */
  uint32_t num_tmb_bufs;

  /* num of buf in src img */
  uint32_t num_dst_bufs;

  int8_t encode_thumbnail;

  /* src img bufs */
  mm_jpeg_buf_t src_main_buf[MM_JPEG_MAX_BUF];

  /* this will be used only for bitstream */
  mm_jpeg_buf_t src_thumb_buf[MM_JPEG_MAX_BUF];

  /* this will be used only for bitstream */
  mm_jpeg_buf_t dest_buf[MM_JPEG_MAX_BUF];

  /* color format */
  mm_jpeg_color_format color_format;

  /* jpeg quality: range 0~100 */
  uint32_t quality;

  /* buf to exif entries, caller needs to
   * take care of the memory manage with insider ptr */
  QOMX_EXIF_INFO exif_info;

  jpeg_encode_callback_t jpeg_cb;
  void* userdata;

} mm_jpeg_encode_params_t;

typedef struct {
  /* active indices of the buffers for encoding */
  uint32_t src_index;
  uint32_t dst_index;
  uint32_t thumb_index;
  mm_jpeg_dim_t thumb_dim;

  /* rotation informaiton */
  int rotation;

  /* main image dimension */
  mm_jpeg_dim_t main_dim;

  /*session id*/
  uint32_t session_id;

  /*Metadata stream*/
  cam_metadata_info_t *p_metadata;

} mm_jpeg_encode_job_t;

typedef enum {
  JPEG_JOB_TYPE_ENCODE,
  JPEG_JOB_TYPE_MAX
} mm_jpeg_job_type_t;

typedef struct {
  mm_jpeg_job_type_t job_type;
  union {
    mm_jpeg_encode_job_t encode_job;
  };
} mm_jpeg_job_t;

typedef struct {
  /* config a job -- async call */
  int (*start_job)(mm_jpeg_job_t* job, uint32_t* job_id);

  /* abort a job -- sync call */
  int (*abort_job)(uint32_t job_id);

  /* create a session */
  int (*create_session)(uint32_t client_hdl,
    mm_jpeg_encode_params_t *p_params, uint32_t *p_session_id);

  /* destroy session */
  int (*destroy_session)(uint32_t session_id);

  /* close a jpeg client -- sync call */
  int (*close) (uint32_t clientHdl);
} mm_jpeg_ops_t;

/* open a jpeg client -- sync call
 * returns client_handle.
 * failed if client_handle=0
 * jpeg ops tbl will be filled in if open succeeds */
uint32_t jpeg_open(mm_jpeg_ops_t *ops);

#endif /* MM_JPEG_INTERFACE_H_ */
