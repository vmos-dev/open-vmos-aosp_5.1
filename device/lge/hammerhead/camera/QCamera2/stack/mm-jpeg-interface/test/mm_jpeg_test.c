/* Copyright (c) 2013, The Linux Foundation. All rights reserved.
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

#include "mm_jpeg_interface.h"
#include "mm_jpeg_ionbuf.h"
#include <sys/time.h>

/** DUMP_TO_FILE:
 *  @filename: file name
 *  @p_addr: address of the buffer
 *  @len: buffer length
 *
 *  dump the image to the file
 **/
#define DUMP_TO_FILE(filename, p_addr, len) ({ \
  int rc = 0; \
  FILE *fp = fopen(filename, "w+"); \
  if (fp) { \
    rc = fwrite(p_addr, 1, len, fp); \
    fclose(fp); \
  } else { \
    CDBG_ERROR("%s:%d] cannot dump image", __func__, __LINE__); \
  } \
})

static int g_count = 1, g_i;

typedef struct {
  char *filename;
  int width;
  int height;
  char *out_filename;
} jpeg_test_input_t;

static jpeg_test_input_t jpeg_input[] = {
  {"/data/test.yuv", 1280, 720, "/data/test.jpg"}
};

typedef struct {
  char *filename;
  int width;
  int height;
  char *out_filename;
  pthread_mutex_t lock;
  pthread_cond_t cond;
  buffer_test_t input;
  buffer_test_t output;
  int use_ion;
  uint32_t handle;
  mm_jpeg_ops_t ops;
  uint32_t job_id[5];
  mm_jpeg_encode_params_t params;
  mm_jpeg_job_t job;
  uint32_t session_id;
} mm_jpeg_intf_test_t;

static void mm_jpeg_encode_callback(jpeg_job_status_t status,
  uint32_t client_hdl,
  uint32_t jobId,
  mm_jpeg_output_t *p_output,
  void *userData)
{
  mm_jpeg_intf_test_t *p_obj = (mm_jpeg_intf_test_t *)userData;

  if (status == JPEG_JOB_STATUS_ERROR) {
    CDBG_ERROR("%s:%d] Encode error", __func__, __LINE__);
  } else {
    CDBG_ERROR("%s:%d] Encode success file%s addr %p len %d",
      __func__, __LINE__, p_obj->out_filename,
      p_output->buf_vaddr, p_output->buf_filled_len);
    DUMP_TO_FILE(p_obj->out_filename, p_output->buf_vaddr, p_output->buf_filled_len);
  }
  g_i++;
  if (g_i >= g_count) {
    CDBG_ERROR("%s:%d] Signal the thread", __func__, __LINE__);
    pthread_cond_signal(&p_obj->cond);
  }
}

int mm_jpeg_test_alloc(buffer_test_t *p_buffer, int use_pmem)
{
  int ret = 0;
  /*Allocate buffers*/
  if (use_pmem) {
    p_buffer->addr = (uint8_t *)buffer_allocate(p_buffer);
    if (NULL == p_buffer->addr) {
      CDBG_ERROR("%s:%d] Error",__func__, __LINE__);
      return -1;
    }
  } else {
    /* Allocate heap memory */
    p_buffer->addr = (uint8_t *)malloc(p_buffer->size);
    if (NULL == p_buffer->addr) {
      CDBG_ERROR("%s:%d] Error",__func__, __LINE__);
      return -1;
    }
  }
  return ret;
}

void mm_jpeg_test_free(buffer_test_t *p_buffer)
{
  if (p_buffer->addr == NULL)
    return;

  if (p_buffer->p_pmem_fd > 0)
    buffer_deallocate(p_buffer);
  else
    free(p_buffer->addr);

  memset(p_buffer, 0x0, sizeof(buffer_test_t));
}

int mm_jpeg_test_read(mm_jpeg_intf_test_t *p_obj)
{
  int rc = 0;
  FILE *fp = NULL;
  int file_size = 0;
  fp = fopen(p_obj->filename, "rb");
  if (!fp) {
    CDBG_ERROR("%s:%d] error", __func__, __LINE__);
    return -1;
  }
  fseek(fp, 0, SEEK_END);
  file_size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  CDBG_ERROR("%s:%d] input file size is %d buf_size %ld",
    __func__, __LINE__, file_size, p_obj->input.size);

  if (p_obj->input.size > file_size) {
    CDBG_ERROR("%s:%d] error", __func__, __LINE__);
    fclose(fp);
    return -1;
  }
  fread(p_obj->input.addr, 1, p_obj->input.size, fp);
  fclose(fp);
  return 0;
}

static int encode_init(jpeg_test_input_t *p_input, mm_jpeg_intf_test_t *p_obj)
{
  int rc = -1;
  int size = p_input->width * p_input->height;
  mm_jpeg_encode_params_t *p_params = &p_obj->params;
  mm_jpeg_encode_job_t *p_job_params = &p_obj->job.encode_job;

  p_obj->filename = p_input->filename;
  p_obj->width = p_input->width;
  p_obj->height = p_input->height;
  p_obj->out_filename = p_input->out_filename;
  p_obj->use_ion = 1;

  pthread_mutex_init(&p_obj->lock, NULL);
  pthread_cond_init(&p_obj->cond, NULL);

  /* allocate buffers */
  p_obj->input.size = size * 3/2;
  rc = mm_jpeg_test_alloc(&p_obj->input, p_obj->use_ion);
  if (rc) {
    CDBG_ERROR("%s:%d] Error",__func__, __LINE__);
    return -1;
  }

  p_obj->output.size = size * 3/2;
  rc = mm_jpeg_test_alloc(&p_obj->output, 0);
  if (rc) {
    CDBG_ERROR("%s:%d] Error",__func__, __LINE__);
    return -1;
  }

  rc = mm_jpeg_test_read(p_obj);
  if (rc) {
    CDBG_ERROR("%s:%d] Error",__func__, __LINE__);
    return -1;
  }

  /* set encode parameters */
  p_params->jpeg_cb = mm_jpeg_encode_callback;
  p_params->userdata = p_obj;
  p_params->color_format = MM_JPEG_COLOR_FORMAT_YCRCBLP_H2V2;

  /* dest buffer config */
  p_params->dest_buf[0].buf_size = p_obj->output.size;
  p_params->dest_buf[0].buf_vaddr = p_obj->output.addr;
  p_params->dest_buf[0].fd = p_obj->output.p_pmem_fd;
  p_params->dest_buf[0].index = 0;
  p_params->num_dst_bufs = 1;

  /* src buffer config*/
  p_params->src_main_buf[0].buf_size = p_obj->input.size;
  p_params->src_main_buf[0].buf_vaddr = p_obj->input.addr;
  p_params->src_main_buf[0].fd = p_obj->input.p_pmem_fd;
  p_params->src_main_buf[0].index = 0;
  p_params->src_main_buf[0].format = MM_JPEG_FMT_YUV;
  p_params->src_main_buf[0].offset.mp[0].len = size;
  p_params->src_main_buf[0].offset.mp[1].len = size >> 1;
  p_params->num_src_bufs = 1;

  p_params->encode_thumbnail = 1;
  p_params->exif_info.numOfEntries = 0;
  p_params->quality = 80;

  p_job_params->dst_index = 0;
  p_job_params->src_index = 0;
  p_job_params->rotation = 0;

  /* main dimension */
  p_job_params->main_dim.src_dim.width = p_obj->width;
  p_job_params->main_dim.src_dim.height = p_obj->height;
  p_job_params->main_dim.dst_dim.width = p_obj->width;
  p_job_params->main_dim.dst_dim.height = p_obj->height;
  p_job_params->main_dim.crop.top = 0;
  p_job_params->main_dim.crop.left = 0;
  p_job_params->main_dim.crop.width = p_obj->width;
  p_job_params->main_dim.crop.height = p_obj->height;

  /* thumb dimension */
  p_job_params->thumb_dim.src_dim.width = p_obj->width;
  p_job_params->thumb_dim.src_dim.height = p_obj->height;
  p_job_params->thumb_dim.dst_dim.width = 512;
  p_job_params->thumb_dim.dst_dim.height = 384;
  p_job_params->thumb_dim.crop.top = 0;
  p_job_params->thumb_dim.crop.left = 0;
  p_job_params->thumb_dim.crop.width = p_obj->width;
  p_job_params->thumb_dim.crop.height = p_obj->height;
  return 0;
}

static int encode_test(jpeg_test_input_t *p_input)
{
  int rc = 0;
  mm_jpeg_intf_test_t jpeg_obj;
  int i = 0;

  memset(&jpeg_obj, 0x0, sizeof(jpeg_obj));
  rc = encode_init(p_input, &jpeg_obj);
  if (rc) {
    CDBG_ERROR("%s:%d] Error",__func__, __LINE__);
    return -1;
  }

  jpeg_obj.handle = jpeg_open(&jpeg_obj.ops);
  if (jpeg_obj.handle == 0) {
    CDBG_ERROR("%s:%d] Error",__func__, __LINE__);
    goto end;
  }

  rc = jpeg_obj.ops.create_session(jpeg_obj.handle, &jpeg_obj.params,
    &jpeg_obj.job.encode_job.session_id);
  if (jpeg_obj.job.encode_job.session_id == 0) {
    CDBG_ERROR("%s:%d] Error",__func__, __LINE__);
    goto end;
  }

  for (i = 0; i < g_count; i++) {
    jpeg_obj.job.job_type = JPEG_JOB_TYPE_ENCODE;
    rc = jpeg_obj.ops.start_job(&jpeg_obj.job, &jpeg_obj.job_id[i]);
    if (rc) {
      CDBG_ERROR("%s:%d] Error",__func__, __LINE__);
      goto end;
    }
  }

  /*
  usleep(5);
  jpeg_obj.ops.abort_job(jpeg_obj.job_id[0]);
  */
  pthread_mutex_lock(&jpeg_obj.lock);
  pthread_cond_wait(&jpeg_obj.cond, &jpeg_obj.lock);
  pthread_mutex_unlock(&jpeg_obj.lock);

  jpeg_obj.ops.destroy_session(jpeg_obj.job.encode_job.session_id);

  jpeg_obj.ops.close(jpeg_obj.handle);


end:
  mm_jpeg_test_free(&jpeg_obj.input);
  mm_jpeg_test_free(&jpeg_obj.output);
  return 0;
}

/** main:
 *
 *  Arguments:
 *    @argc
 *    @argv
 *
 *  Return:
 *       0 or -ve values
 *
 *  Description:
 *       main function
 *
 **/
int main(int argc, char* argv[])
{
  return encode_test(&jpeg_input[0]);
}


