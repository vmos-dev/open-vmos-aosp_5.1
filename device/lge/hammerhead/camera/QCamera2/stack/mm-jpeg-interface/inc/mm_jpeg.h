/* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
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

#ifndef MM_JPEG_H_
#define MM_JPEG_H_

#include <cam_semaphore.h>
#include "mm_jpeg_interface.h"
#include "cam_list.h"
#include "OMX_Types.h"
#include "OMX_Index.h"
#include "OMX_Core.h"
#include "OMX_Component.h"
#include "QOMX_JpegExtensions.h"

#define MM_JPEG_MAX_THREADS 30
#define MM_JPEG_CIRQ_SIZE 30
#define MM_JPEG_MAX_SESSION 10
#define MAX_EXIF_TABLE_ENTRIES 50
#define ASPECT_TOLERANCE 0.001

typedef struct {
  struct cam_list list;
  void* data;
} mm_jpeg_q_node_t;

typedef struct {
  mm_jpeg_q_node_t head; /* dummy head */
  uint32_t size;
  pthread_mutex_t lock;
} mm_jpeg_queue_t;

typedef enum {
  MM_JPEG_CMD_TYPE_JOB,          /* job cmd */
  MM_JPEG_CMD_TYPE_EXIT,         /* EXIT cmd for exiting jobMgr thread */
  MM_JPEG_CMD_TYPE_MAX
} mm_jpeg_cmd_type_t;

typedef struct {
  union {
    int i_data[MM_JPEG_CIRQ_SIZE];
    void *p_data[MM_JPEG_CIRQ_SIZE];
  };
  int front;
  int rear;
  int count;
  pthread_mutex_t lock;
} mm_jpeg_cirq_t;

typedef struct {
  uint32_t client_hdl;           /* client handler */
  uint32_t jobId;                /* job ID */
  uint32_t sessionId;            /* session ID */
  mm_jpeg_encode_params_t params; /* encode params */
  mm_jpeg_encode_job_t encode_job;             /* job description */
  pthread_t encode_pid;          /* encode thread handler*/

  void *jpeg_obj;                /* ptr to mm_jpeg_obj */
  jpeg_job_status_t job_status;  /* job status */

  int state_change_pending;      /* flag to indicate if state change is pending */
  OMX_ERRORTYPE error_flag;      /* variable to indicate error during encoding */
  OMX_BOOL abort_flag;      /* variable to indicate abort during encoding */

  /* OMX related */
  OMX_HANDLETYPE omx_handle;                      /* handle to omx engine */
  OMX_CALLBACKTYPE omx_callbacks;                 /* callbacks to omx engine */

  /* buffer headers */
  OMX_BUFFERHEADERTYPE *p_in_omx_buf[MM_JPEG_MAX_BUF];
  OMX_BUFFERHEADERTYPE *p_in_omx_thumb_buf[MM_JPEG_MAX_BUF];
  OMX_BUFFERHEADERTYPE *p_out_omx_buf[MM_JPEG_MAX_BUF];

  OMX_PARAM_PORTDEFINITIONTYPE inputPort;
  OMX_PARAM_PORTDEFINITIONTYPE outputPort;
  OMX_PARAM_PORTDEFINITIONTYPE inputTmbPort;

  /* event locks */
  pthread_mutex_t lock;
  pthread_cond_t cond;

  QEXIF_INFO_DATA exif_info_local[MAX_EXIF_TABLE_ENTRIES];  //all exif tags for JPEG encoder
  int exif_count_local;

  mm_jpeg_cirq_t cb_q;
  int32_t ebd_count;
  int32_t fbd_count;

  /* this flag represents whether the job is active */
  OMX_BOOL active;

  /* this flag indicates if the configration is complete */
  OMX_BOOL config;

  /* job history count to generate unique id */
  int job_hist;

  OMX_BOOL encoding;
} mm_jpeg_job_session_t;

typedef struct {
  mm_jpeg_encode_job_t encode_job;
  uint32_t job_id;
  uint32_t client_handle;
} mm_jpeg_encode_job_info_t;

typedef struct {
  mm_jpeg_cmd_type_t type;
  union {
    mm_jpeg_encode_job_info_t enc_info;
  };
} mm_jpeg_job_q_node_t;

typedef struct {
  uint8_t is_used;                /* flag: if is a valid client */
  uint32_t client_handle;         /* client handle */
  mm_jpeg_job_session_t session[MM_JPEG_MAX_SESSION];
  pthread_mutex_t lock;           /* job lock */
} mm_jpeg_client_t;

typedef struct {
  pthread_t pid;                  /* job cmd thread ID */
  cam_semaphore_t job_sem;        /* semaphore for job cmd thread */
  mm_jpeg_queue_t job_queue;      /* queue for job to do */
} mm_jpeg_job_cmd_thread_t;

#define MAX_JPEG_CLIENT_NUM 8
typedef struct mm_jpeg_obj_t {
  /* ClientMgr */
  int num_clients;                                /* num of clients */
  mm_jpeg_client_t clnt_mgr[MAX_JPEG_CLIENT_NUM]; /* client manager */

  /* JobMkr */
  pthread_mutex_t job_lock;                       /* job lock */
  mm_jpeg_job_cmd_thread_t job_mgr;               /* job mgr thread including todo_q*/
  mm_jpeg_queue_t ongoing_job_q;                  /* queue for ongoing jobs */
} mm_jpeg_obj;

extern int32_t mm_jpeg_init(mm_jpeg_obj *my_obj);
extern int32_t mm_jpeg_deinit(mm_jpeg_obj *my_obj);
extern uint32_t mm_jpeg_new_client(mm_jpeg_obj *my_obj);
extern int32_t mm_jpeg_start_job(mm_jpeg_obj *my_obj,
  mm_jpeg_job_t* job,
  uint32_t* jobId);
extern int32_t mm_jpeg_abort_job(mm_jpeg_obj *my_obj,
  uint32_t jobId);
extern int32_t mm_jpeg_close(mm_jpeg_obj *my_obj,
  uint32_t client_hdl);
extern int32_t mm_jpeg_create_session(mm_jpeg_obj *my_obj,
  uint32_t client_hdl,
  mm_jpeg_encode_params_t *p_params,
  uint32_t* p_session_id);
extern int32_t mm_jpeg_destroy_session_by_id(mm_jpeg_obj *my_obj,
  uint32_t session_id);
extern int32_t mm_jpeg_destroy_job(mm_jpeg_job_session_t *p_session);

/* utiltity fucntion declared in mm-camera-inteface2.c
 * and need be used by mm-camera and below*/
uint32_t mm_jpeg_util_generate_handler(uint8_t index);
uint8_t mm_jpeg_util_get_index_by_handler(uint32_t handler);

/* basic queue functions */
extern int32_t mm_jpeg_queue_init(mm_jpeg_queue_t* queue);
extern int32_t mm_jpeg_queue_enq(mm_jpeg_queue_t* queue, void* node);
extern void* mm_jpeg_queue_deq(mm_jpeg_queue_t* queue);
extern int32_t mm_jpeg_queue_deinit(mm_jpeg_queue_t* queue);
extern int32_t mm_jpeg_queue_flush(mm_jpeg_queue_t* queue);
extern uint32_t mm_jpeg_queue_get_size(mm_jpeg_queue_t* queue);
extern void* mm_jpeg_queue_peek(mm_jpeg_queue_t* queue);
extern int32_t addExifEntry(QOMX_EXIF_INFO *p_exif_info, exif_tag_id_t tagid,
  exif_tag_type_t type, uint32_t count, void *data);
extern int32_t releaseExifEntry(QEXIF_INFO_DATA *p_exif_data);
extern int process_meta_data_v1(cam_metadata_info_t *p_meta,
  QOMX_EXIF_INFO *exif_info, mm_jpeg_exif_params_t *p_cam_exif_params);
extern int process_meta_data_v3(metadata_buffer_t *p_meta,
  QOMX_EXIF_INFO *exif_info, mm_jpeg_exif_params_t *p_cam3a_params);

#endif /* MM_JPEG_H_ */


