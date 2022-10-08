/*--------------------------------------------------------------------------
Copyright (c) 2012, Code Aurora Forum. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Code Aurora nor
      the names of its contributors may be used to endorse or promote
      products derived from this software without specific prior written
      permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
--------------------------------------------------------------------------*/

/*============================================================================
                            O p e n M A X   w r a p p e r s
                             O p e n  M A X   C o r e

*//** @file omx_vdec.cpp
  This module contains the implementation of the OpenMAX core & component.

*//*========================================================================*/

//////////////////////////////////////////////////////////////////////////////
//                             Include Files
//////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "omx_vdec.h"
#include <fcntl.h>
#include <limits.h>

#ifndef _ANDROID_
#include <sys/ioctl.h>
#include <sys/mman.h>
#endif //_ANDROID_

#ifdef _ANDROID_
#include <cutils/properties.h>
#undef USE_EGL_IMAGE_GPU
#endif

#if  defined (_ANDROID_HONEYCOMB_) || defined (_ANDROID_ICS_)
#include <gralloc_priv.h>
#endif

#ifdef _ANDROID_
#include "DivXDrmDecrypt.h"
#endif //_ANDROID_

#ifdef USE_EGL_IMAGE_GPU
#include <EGL/egl.h>
#include <EGL/eglQCOM.h>
#define EGL_BUFFER_HANDLE_QCOM 0x4F00
#define EGL_BUFFER_OFFSET_QCOM 0x4F01
#endif

#ifdef INPUT_BUFFER_LOG
#define INPUT_BUFFER_FILE_NAME "/data/input-bitstream.\0\0\0\0"
#define INPUT_BUFFER_FILE_NAME_LEN 30
FILE *inputBufferFile1;
char inputfilename [INPUT_BUFFER_FILE_NAME_LEN] = "\0";
#endif
#ifdef OUTPUT_BUFFER_LOG
FILE *outputBufferFile1;
char outputfilename [] = "/data/output.yuv";
#endif
#ifdef OUTPUT_EXTRADATA_LOG
FILE *outputExtradataFile;
char ouputextradatafilename [] = "/data/extradata";
#endif

#define DEFAULT_FPS 30
#define MAX_INPUT_ERROR DEFAULT_FPS
#define MAX_SUPPORTED_FPS 120

#define VC1_SP_MP_START_CODE        0xC5000000
#define VC1_SP_MP_START_CODE_MASK   0xFF000000
#define VC1_AP_SEQ_START_CODE       0x0F010000
#define VC1_STRUCT_C_PROFILE_MASK   0xF0
#define VC1_STRUCT_B_LEVEL_MASK     0xE0000000
#define VC1_SIMPLE_PROFILE          0
#define VC1_MAIN_PROFILE            1
#define VC1_ADVANCE_PROFILE         3
#define VC1_SIMPLE_PROFILE_LOW_LEVEL  0
#define VC1_SIMPLE_PROFILE_MED_LEVEL  2
#define VC1_STRUCT_C_LEN            4
#define VC1_STRUCT_C_POS            8
#define VC1_STRUCT_A_POS            12
#define VC1_STRUCT_B_POS            24
#define VC1_SEQ_LAYER_SIZE          36

#define MEM_DEVICE "/dev/ion"
#define MEM_HEAP_ID ION_CP_MM_HEAP_ID

#ifdef _ANDROID_
    extern "C"{
        #include<utils/Log.h>
    }
#endif//_ANDROID_

#define Log2(number, power)  { OMX_U32 temp = number; power = 0; while( (0 == (temp & 0x1)) &&  power < 16) { temp >>=0x1; power++; } }
#define Q16ToFraction(q,num,den) { OMX_U32 power; Log2(q,power);  num = q >> power; den = 0x1 << (16 - power); }

void* async_message_thread (void *input)
{
  struct vdec_ioctl_msg ioctl_msg;
  struct vdec_msginfo vdec_msg;
  OMX_BUFFERHEADERTYPE *buffer;
  struct v4l2_plane plane;
  struct pollfd pfd;
  struct v4l2_buffer v4l2_buf ={0};
   struct v4l2_event dqevent;
  pfd.events = POLLIN | POLLRDNORM | POLLOUT | POLLWRNORM | POLLRDBAND | POLLPRI;
  omx_vdec *omx = reinterpret_cast<omx_vdec*>(input);
  pfd.fd = omx->drv_ctx.video_driver_fd;
  int error_code = 0,rc=0;
  DEBUG_PRINT_HIGH("omx_vdec: Async thread start\n");
  prctl(PR_SET_NAME, (unsigned long)"VideoDecCallBackThread", 0, 0, 0);
  while (1)
  {
		rc = poll(&pfd, 1, TIMEOUT);
		if (!rc) {
			printf("Poll timedout\n");
			break;
		} else if (rc < 0) {
			printf("Error while polling: %d\n", rc);
			break;
		}
		if ((pfd.revents & POLLIN) || (pfd.revents & POLLRDNORM)) {
			v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
			v4l2_buf.memory = V4L2_MEMORY_USERPTR;
			v4l2_buf.length = 1;
			v4l2_buf.m.planes = &plane;
			rc = ioctl(pfd.fd, VIDIOC_DQBUF, &v4l2_buf);
			if (rc) {
				/*TODO: How to handle this case */
				printf("Failed to dequeue buf: %d from capture capability\n", rc);
				break;
			}
			vdec_msg.msgcode=VDEC_MSG_RESP_OUTPUT_BUFFER_DONE;
			vdec_msg.status_code=VDEC_S_SUCCESS;
			vdec_msg.msgdata.output_frame.client_data=(void*)&v4l2_buf;
			vdec_msg.msgdata.output_frame.len=plane.bytesused;
			vdec_msg.msgdata.output_frame.bufferaddr=(void*)plane.m.userptr;
		}
		else if((pfd.revents & POLLOUT) || (pfd.revents & POLLWRNORM)) {
			v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
			v4l2_buf.memory = V4L2_MEMORY_USERPTR;
			v4l2_buf.m.planes = &plane;
			rc = ioctl(pfd.fd, VIDIOC_DQBUF, &v4l2_buf);
			if (rc) {
				/*TODO: How to handle this case */
				printf("Failed to dequeue buf: %d from output capability\n", rc);
				break;
			}
            vdec_msg.msgcode=VDEC_MSG_RESP_INPUT_BUFFER_DONE;
			vdec_msg.status_code=VDEC_S_SUCCESS;
			vdec_msg.msgdata.input_frame_clientdata=(void*)&v4l2_buf;
		} else if (pfd.revents & POLLPRI){
			rc = ioctl(pfd.fd, VIDIOC_DQEVENT, &dqevent);
			if(dqevent.u.data[0] == MSM_VIDC_DECODER_EVENT_CHANGE){
				vdec_msg.msgcode=VDEC_MSG_EVT_CONFIG_CHANGED;
				vdec_msg.status_code=VDEC_S_SUCCESS;
				printf("\n VIDC Port Reconfig recieved \n");
			} else if (dqevent.u.data[0] == MSM_VIDC_DECODER_FLUSH_DONE){
				vdec_msg.msgcode=VDEC_MSG_RESP_FLUSH_OUTPUT_DONE;
				vdec_msg.status_code=VDEC_S_SUCCESS;
				printf("\n VIDC Flush Done Recieved \n");
			} else
				printf("\n VIDC Some Event recieved \n");
		} else if (pfd.revents & POLLERR){
			printf("\n async_message_thread Exited \n");
			break;
		} else{
			/*TODO: How to handle this case */
			continue;
		}

		if (omx->async_message_process(input,&vdec_msg) < 0) {
			printf("\n async_message_thread Exited  \n");
				break;
		}
  }
    DEBUG_PRINT_HIGH("omx_vdec: Async thread stop\n");
  return NULL;
}

void* message_thread(void *input)
{
  omx_vdec* omx = reinterpret_cast<omx_vdec*>(input);
  unsigned char id;
  int n;

  DEBUG_PRINT_HIGH("omx_vdec: message thread start\n");
  prctl(PR_SET_NAME, (unsigned long)"VideoDecMsgThread", 0, 0, 0);
  while (1)
  {

    n = read(omx->m_pipe_in, &id, 1);

    if(0 == n)
    {
      break;
    }

    if (1 == n)
    {
        omx->process_event_cb(omx, id);
    }
    if ((n < 0) && (errno != EINTR))
    {
      DEBUG_PRINT_ERROR("\nERROR: read from pipe failed, ret %d errno %d", n, errno);
      break;
    }
  }
  DEBUG_PRINT_HIGH("omx_vdec: message thread stop\n");
  return 0;
}

void post_message(omx_vdec *omx, unsigned char id)
{
      int ret_value;
      DEBUG_PRINT_LOW("omx_vdec: post_message %d pipe out%d\n", id,omx->m_pipe_out);
      ret_value = write(omx->m_pipe_out, &id, 1);
      DEBUG_PRINT_LOW("post_message to pipe done %d\n",ret_value);
}

// omx_cmd_queue destructor
omx_vdec::omx_cmd_queue::~omx_cmd_queue()
{
  // Nothing to do
}

// omx cmd queue constructor
omx_vdec::omx_cmd_queue::omx_cmd_queue(): m_read(0),m_write(0),m_size(0)
{
    memset(m_q,0,sizeof(omx_event)*OMX_CORE_CONTROL_CMDQ_SIZE);
}

// omx cmd queue insert
bool omx_vdec::omx_cmd_queue::insert_entry(unsigned p1, unsigned p2, unsigned id)
{
  bool ret = true;
  if(m_size < OMX_CORE_CONTROL_CMDQ_SIZE)
  {
    m_q[m_write].id       = id;
    m_q[m_write].param1   = p1;
    m_q[m_write].param2   = p2;
    m_write++;
    m_size ++;
    if(m_write >= OMX_CORE_CONTROL_CMDQ_SIZE)
    {
      m_write = 0;
    }
  }
  else
  {
    ret = false;
    DEBUG_PRINT_ERROR("ERROR: %s()::Command Queue Full\n", __func__);
  }
  return ret;
}

// omx cmd queue pop
bool omx_vdec::omx_cmd_queue::pop_entry(unsigned *p1, unsigned *p2, unsigned *id)
{
  bool ret = true;
  if (m_size > 0)
  {
    *id = m_q[m_read].id;
    *p1 = m_q[m_read].param1;
    *p2 = m_q[m_read].param2;
    // Move the read pointer ahead
    ++m_read;
    --m_size;
    if(m_read >= OMX_CORE_CONTROL_CMDQ_SIZE)
    {
      m_read = 0;
    }
  }
  else
  {
    ret = false;
  }
  return ret;
}

// Retrieve the first mesg type in the queue
unsigned omx_vdec::omx_cmd_queue::get_q_msg_type()
{
    return m_q[m_read].id;
}

#ifdef _ANDROID_
omx_vdec::ts_arr_list::ts_arr_list()
{
  //initialize timestamps array
  memset(m_ts_arr_list, 0, ( sizeof(ts_entry) * MAX_NUM_INPUT_OUTPUT_BUFFERS) );
}
omx_vdec::ts_arr_list::~ts_arr_list()
{
  //free m_ts_arr_list?
}

bool omx_vdec::ts_arr_list::insert_ts(OMX_TICKS ts)
{
  bool ret = true;
  bool duplicate_ts = false;
  int idx = 0;

  //insert at the first available empty location
  for ( ; idx < MAX_NUM_INPUT_OUTPUT_BUFFERS; idx++)
  {
    if (!m_ts_arr_list[idx].valid)
    {
      //found invalid or empty entry, save timestamp
      m_ts_arr_list[idx].valid = true;
      m_ts_arr_list[idx].timestamp = ts;
      DEBUG_PRINT_LOW("Insert_ts(): Inserting TIMESTAMP (%lld) at idx (%d)",
                       ts, idx);
      break;
    }
  }

  if (idx == MAX_NUM_INPUT_OUTPUT_BUFFERS)
  {
    DEBUG_PRINT_LOW("Timestamp array list is FULL. Unsuccessful insert");
    ret = false;
  }
  return ret;
}

bool omx_vdec::ts_arr_list::pop_min_ts(OMX_TICKS &ts)
{
  bool ret = true;
  int min_idx = -1;
  OMX_TICKS min_ts = 0;
  int idx = 0;

  for ( ; idx < MAX_NUM_INPUT_OUTPUT_BUFFERS; idx++)
  {

    if (m_ts_arr_list[idx].valid)
    {
      //found valid entry, save index
      if (min_idx < 0)
      {
        //first valid entry
        min_ts = m_ts_arr_list[idx].timestamp;
        min_idx = idx;
      }
      else if (m_ts_arr_list[idx].timestamp < min_ts)
      {
        min_ts = m_ts_arr_list[idx].timestamp;
        min_idx = idx;
      }
    }

  }

  if (min_idx < 0)
  {
    //no valid entries found
    DEBUG_PRINT_LOW("Timestamp array list is empty. Unsuccessful pop");
    ts = 0;
    ret = false;
  }
  else
  {
    ts = m_ts_arr_list[min_idx].timestamp;
    m_ts_arr_list[min_idx].valid = false;
    DEBUG_PRINT_LOW("Pop_min_ts:Timestamp (%lld), index(%d)",
                     ts, min_idx);
  }

  return ret;

}


bool omx_vdec::ts_arr_list::reset_ts_list()
{
  bool ret = true;
  int idx = 0;

  DEBUG_PRINT_LOW("reset_ts_list(): Resetting timestamp array list");
  for ( ; idx < MAX_NUM_INPUT_OUTPUT_BUFFERS; idx++)
  {
    m_ts_arr_list[idx].valid = false;
  }
  return ret;
}
#endif

// factory function executed by the core to create instances
void *get_omx_component_factory_fn(void)
{
  return (new omx_vdec);
}

#ifdef _ANDROID_
#ifdef USE_ION
VideoHeap::VideoHeap(int devicefd, size_t size, void* base,
                     struct ion_handle *handle, int ionMapfd)
{
    ionInit(devicefd, base, size, 0 , MEM_DEVICE,handle,ionMapfd);
}
#else
VideoHeap::VideoHeap(int fd, size_t size, void* base)
{
    // dup file descriptor, map once, use pmem
    init(dup(fd), base, size, 0 , MEM_DEVICE);
}
#endif
#endif // _ANDROID_
/* ======================================================================
FUNCTION
  omx_vdec::omx_vdec

DESCRIPTION
  Constructor

PARAMETERS
  None

RETURN VALUE
  None.
========================================================================== */
omx_vdec::omx_vdec(): m_state(OMX_StateInvalid),
                      m_app_data(NULL),
                      m_inp_mem_ptr(NULL),
                      m_out_mem_ptr(NULL),
                      m_phdr_pmem_ptr(NULL),
                      pending_input_buffers(0),
                      pending_output_buffers(0),
                      m_out_bm_count(0),
                      m_inp_bm_count(0),
                      m_inp_bPopulated(OMX_FALSE),
                      m_out_bPopulated(OMX_FALSE),
                      m_flags(0),
                      m_inp_bEnabled(OMX_TRUE),
                      m_out_bEnabled(OMX_TRUE),
                      m_platform_list(NULL),
                      m_platform_entry(NULL),
                      m_pmem_info(NULL),
                      output_flush_progress (false),
                      input_flush_progress (false),
                      input_use_buffer (false),
                      output_use_buffer (false),
                      arbitrary_bytes (true),
                      psource_frame (NULL),
                      pdest_frame (NULL),
                      m_inp_heap_ptr (NULL),
                      m_heap_inp_bm_count (0),
                      codec_type_parse ((codec_type)0),
                      first_frame_meta (true),
                      frame_count (0),
                      nal_length(0),
                      nal_count (0),
                      look_ahead_nal (false),
                      first_frame(0),
                      first_buffer(NULL),
                      first_frame_size (0),
                      m_error_propogated(false),
                      m_device_file_ptr(NULL),
                      m_vc1_profile((vc1_profile_type)0),
                      prev_ts(LLONG_MAX),
                      rst_prev_ts(true),
                      frm_int(0),
                      m_in_alloc_cnt(0),
                      m_display_id(NULL),
                      ouput_egl_buffers(false),
                      h264_parser(NULL),
                      client_extradata(0),
                      h264_last_au_ts(LLONG_MAX),
                      h264_last_au_flags(0),
                      m_inp_err_count(0),
#ifdef _ANDROID_
                      m_heap_ptr(NULL),
                      m_enable_android_native_buffers(OMX_FALSE),
                      m_use_android_native_buffers(OMX_FALSE),
#endif
                      in_reconfig(false),
                      m_use_output_pmem(OMX_FALSE),
                      m_out_mem_region_smi(OMX_FALSE),
                      m_out_pvt_entry_pmem(OMX_FALSE),
                      secure_mode(false)
#ifdef _ANDROID_
                    ,iDivXDrmDecrypt(NULL)
#endif
                    ,m_desc_buffer_ptr(NULL)
                    ,streaming({false, false})
{
  /* Assumption is that , to begin with , we have all the frames with decoder */
  DEBUG_PRINT_HIGH("In OMX vdec Constructor");
#ifdef _ANDROID_
  char property_value[PROPERTY_VALUE_MAX] = {0};
  property_get("vidc.dec.debug.perf", property_value, "0");
  perf_flag = atoi(property_value);
  if (perf_flag)
  {
    DEBUG_PRINT_HIGH("vidc.dec.debug.perf is %d", perf_flag);
    dec_time.start();
    proc_frms = latency = 0;
  }
  property_value[0] = NULL;
  property_get("vidc.dec.debug.ts", property_value, "0");
  m_debug_timestamp = atoi(property_value);
  DEBUG_PRINT_HIGH("vidc.dec.debug.ts value is %d",m_debug_timestamp);
  if (m_debug_timestamp)
  {
    time_stamp_dts.set_timestamp_reorder_mode(true);
  }

  property_value[0] = NULL;
  property_get("vidc.dec.debug.concealedmb", property_value, "0");
  m_debug_concealedmb = atoi(property_value);
  DEBUG_PRINT_HIGH("vidc.dec.debug.concealedmb value is %d",m_debug_concealedmb);

#endif
  memset(&m_cmp,0,sizeof(m_cmp));
  memset(&m_cb,0,sizeof(m_cb));
  memset (&drv_ctx,0,sizeof(drv_ctx));
  memset (&h264_scratch,0,sizeof (OMX_BUFFERHEADERTYPE));
  memset (m_hwdevice_name,0,sizeof(m_hwdevice_name));
  memset(&op_buf_rcnfg, 0 ,sizeof(vdec_allocatorproperty));
  memset(m_demux_offsets, 0, ( sizeof(OMX_U32) * 8192) );
  m_demux_entries = 0;
  drv_ctx.timestamp_adjust = false;
  drv_ctx.video_driver_fd = -1;
  m_vendor_config.pData = NULL;
  pthread_mutex_init(&m_lock, NULL);
  sem_init(&m_cmd_lock,0,0);
#ifdef _ANDROID_
  char extradata_value[PROPERTY_VALUE_MAX] = {0};
  property_get("vidc.dec.debug.extradata", extradata_value, "0");
  m_debug_extradata = atoi(extradata_value);
  DEBUG_PRINT_HIGH("vidc.dec.debug.extradata value is %d",m_debug_extradata);
#endif

}


/* ======================================================================
FUNCTION
  omx_vdec::~omx_vdec

DESCRIPTION
  Destructor

PARAMETERS
  None

RETURN VALUE
  None.
========================================================================== */
omx_vdec::~omx_vdec()
{
  m_pmem_info = NULL;
  DEBUG_PRINT_HIGH("In OMX vdec Destructor");
  if(m_pipe_in) close(m_pipe_in);
  if(m_pipe_out) close(m_pipe_out);
  m_pipe_in = -1;
  m_pipe_out = -1;
  DEBUG_PRINT_HIGH("Waiting on OMX Msg Thread exit");
  pthread_join(msg_thread_id,NULL);
  DEBUG_PRINT_HIGH("Waiting on OMX Async Thread exit");
  pthread_join(async_thread_id,NULL);
  close(drv_ctx.video_driver_fd);
  pthread_mutex_destroy(&m_lock);
  sem_destroy(&m_cmd_lock);
  if (perf_flag)
  {
    DEBUG_PRINT_HIGH("--> TOTAL PROCESSING TIME");
    dec_time.end();
  }
  DEBUG_PRINT_HIGH("Exit OMX vdec Destructor");
}

/* ======================================================================
FUNCTION
  omx_vdec::OMXCntrlProcessMsgCb

DESCRIPTION
  IL Client callbacks are generated through this routine. The decoder
  provides the thread context for this routine.

PARAMETERS
  ctxt -- Context information related to the self.
  id   -- Event identifier. This could be any of the following:
          1. Command completion event
          2. Buffer done callback event
          3. Frame done callback event

RETURN VALUE
  None.

========================================================================== */
void omx_vdec::process_event_cb(void *ctxt, unsigned char id)
{
  unsigned p1; // Parameter - 1
  unsigned p2; // Parameter - 2
  unsigned ident;
  unsigned qsize=0; // qsize
  omx_vdec *pThis = (omx_vdec *) ctxt;

  if(!pThis)
  {
    DEBUG_PRINT_ERROR("ERROR: %s()::Context is incorrect, bailing out\n",
        __func__);
    return;
  }

  // Protect the shared queue data structure
  do
  {
    /*Read the message id's from the queue*/
    pthread_mutex_lock(&pThis->m_lock);
    qsize = pThis->m_cmd_q.m_size;
    if(qsize)
    {
      pThis->m_cmd_q.pop_entry(&p1,&p2,&ident);
    }

    if (qsize == 0 && pThis->m_state != OMX_StatePause)
    {
      qsize = pThis->m_ftb_q.m_size;
      if (qsize)
      {
        pThis->m_ftb_q.pop_entry(&p1,&p2,&ident);
      }
    }

    if (qsize == 0 && pThis->m_state != OMX_StatePause)
    {
      qsize = pThis->m_etb_q.m_size;
      if (qsize)
      {
        pThis->m_etb_q.pop_entry(&p1,&p2,&ident);
      }
    }
    pthread_mutex_unlock(&pThis->m_lock);

    /*process message if we have one*/
    if(qsize > 0)
    {
      id = ident;
      switch (id)
      {
        case OMX_COMPONENT_GENERATE_EVENT:
          if (pThis->m_cb.EventHandler)
          {
            switch (p1)
            {
              case OMX_CommandStateSet:
                pThis->m_state = (OMX_STATETYPE) p2;
                DEBUG_PRINT_HIGH("\n OMX_CommandStateSet complete, m_state = %d",
                    pThis->m_state);
                pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                      OMX_EventCmdComplete, p1, p2, NULL);
                break;

              case OMX_EventError:
                if(p2 == OMX_StateInvalid)
                {
                    DEBUG_PRINT_ERROR("\n OMX_EventError: p2 is OMX_StateInvalid");
                    pThis->m_state = (OMX_STATETYPE) p2;
                    pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                               OMX_EventError, OMX_ErrorInvalidState, p2, NULL);
                }
                else if (p2 == OMX_ErrorHardware)
                {
                   pThis->omx_report_error();
                }
                else
		  {
                    pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                      OMX_EventError, p2, NULL, NULL );
                }
                break;

              case OMX_CommandPortDisable:
                DEBUG_PRINT_HIGH("\n OMX_CommandPortDisable complete for port [%d]", p2);
                if (BITMASK_PRESENT(&pThis->m_flags,
                    OMX_COMPONENT_OUTPUT_FLUSH_IN_DISABLE_PENDING))
                {
                  BITMASK_SET(&pThis->m_flags, OMX_COMPONENT_DISABLE_OUTPUT_DEFERRED);
                  break;
                }
                if (p2 == OMX_CORE_OUTPUT_PORT_INDEX && pThis->in_reconfig)
                {
                  pThis->op_buf_rcnfg.buffer_type = VDEC_BUFFER_TYPE_OUTPUT;
				  OMX_ERRORTYPE eRet = OMX_ErrorNone;
				  pThis->stream_off();
				  OMX_ERRORTYPE eRet1 = pThis->get_buffer_req(&pThis->op_buf_rcnfg);
				  pThis->in_reconfig = false;
                  if(eRet !=  OMX_ErrorNone)
                  {
                      DEBUG_PRINT_ERROR("set_buffer_req failed eRet = %d",eRet);
                      pThis->omx_report_error();
                      break;
                  }
                }
                pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                      OMX_EventCmdComplete, p1, p2, NULL );
                break;
              case OMX_CommandPortEnable:
                DEBUG_PRINT_HIGH("\n OMX_CommandPortEnable complete for port [%d]", p2);
                pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,\
                                      OMX_EventCmdComplete, p1, p2, NULL );
                break;

              default:
                pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                         OMX_EventCmdComplete, p1, p2, NULL );
                break;

            }
          }
          else
          {
            DEBUG_PRINT_ERROR("ERROR: %s()::EventHandler is NULL\n", __func__);
          }
          break;
        case OMX_COMPONENT_GENERATE_ETB_ARBITRARY:
          if (pThis->empty_this_buffer_proxy_arbitrary((OMX_HANDLETYPE)p1,\
              (OMX_BUFFERHEADERTYPE *)p2) != OMX_ErrorNone)
          {
            DEBUG_PRINT_ERROR("\n empty_this_buffer_proxy_arbitrary failure");
            pThis->omx_report_error ();
          }
      break;
        case OMX_COMPONENT_GENERATE_ETB:
          if (pThis->empty_this_buffer_proxy((OMX_HANDLETYPE)p1,\
              (OMX_BUFFERHEADERTYPE *)p2) != OMX_ErrorNone)
          {
            DEBUG_PRINT_ERROR("\n empty_this_buffer_proxy failure");
            pThis->omx_report_error ();
          }
         break;

        case OMX_COMPONENT_GENERATE_FTB:
          if ( pThis->fill_this_buffer_proxy((OMX_HANDLETYPE)p1,\
               (OMX_BUFFERHEADERTYPE *)p2) != OMX_ErrorNone)
          {
             DEBUG_PRINT_ERROR("\n fill_this_buffer_proxy failure");
             pThis->omx_report_error ();
          }
        break;

        case OMX_COMPONENT_GENERATE_COMMAND:
          pThis->send_command_proxy(&pThis->m_cmp,(OMX_COMMANDTYPE)p1,\
                                    (OMX_U32)p2,(OMX_PTR)NULL);
          break;

        case OMX_COMPONENT_GENERATE_EBD:

          if (p2 != VDEC_S_SUCCESS && p2 != VDEC_S_INPUT_BITSTREAM_ERR)
          {
            DEBUG_PRINT_ERROR("\n OMX_COMPONENT_GENERATE_EBD failure");
            pThis->omx_report_error ();
          }
          else
          {
            if (p2 == VDEC_S_INPUT_BITSTREAM_ERR && p1)
            {
              pThis->m_inp_err_count++;
              pThis->time_stamp_dts.remove_time_stamp(
              ((OMX_BUFFERHEADERTYPE *)p1)->nTimeStamp,
              (pThis->drv_ctx.interlace != VDEC_InterlaceFrameProgressive)
                ?true:false);
            }
            else
            {
              pThis->m_inp_err_count = 0;
            }
            if ( pThis->empty_buffer_done(&pThis->m_cmp,
                 (OMX_BUFFERHEADERTYPE *)p1) != OMX_ErrorNone)
            {
               DEBUG_PRINT_ERROR("\n empty_buffer_done failure");
               pThis->omx_report_error ();
            }
            if(pThis->m_inp_err_count >= MAX_INPUT_ERROR)
            {
               DEBUG_PRINT_ERROR("\n Input bitstream error for consecutive %d frames.", MAX_INPUT_ERROR);
               pThis->omx_report_error ();
            }
          }
          break;
        case OMX_COMPONENT_GENERATE_INFO_FIELD_DROPPED:
          {
            int64_t *timestamp = (int64_t *)p1;
            if (p1)
            {
              pThis->time_stamp_dts.remove_time_stamp(*timestamp,
              (pThis->drv_ctx.interlace != VDEC_InterlaceFrameProgressive)
              ?true:false);
              free(timestamp);
            }
          }
          break;
        case OMX_COMPONENT_GENERATE_FBD:
          if (p2 != VDEC_S_SUCCESS)
          {
            DEBUG_PRINT_ERROR("\n OMX_COMPONENT_GENERATE_FBD failure");
            pThis->omx_report_error ();
          }
          else if ( pThis->fill_buffer_done(&pThis->m_cmp,
                  (OMX_BUFFERHEADERTYPE *)p1) != OMX_ErrorNone )
          {
            DEBUG_PRINT_ERROR("\n fill_buffer_done failure");
            pThis->omx_report_error ();
          }
          break;

        case OMX_COMPONENT_GENERATE_EVENT_INPUT_FLUSH:
          DEBUG_PRINT_HIGH("\n Driver flush i/p Port complete");
          if (!pThis->input_flush_progress)
          {
            DEBUG_PRINT_ERROR("\n WARNING: Unexpected flush from driver");
          }
          else
          {
            pThis->execute_input_flush();
            if (pThis->m_cb.EventHandler)
            {
              if (p2 != VDEC_S_SUCCESS)
              {
                DEBUG_PRINT_ERROR("\nOMX_COMPONENT_GENERATE_EVENT_INPUT_FLUSH failure");
                pThis->omx_report_error ();
              }
              else
              {
                /*Check if we need generate event for Flush done*/
                if(BITMASK_PRESENT(&pThis->m_flags,
                                   OMX_COMPONENT_INPUT_FLUSH_PENDING))
                {
                  BITMASK_CLEAR (&pThis->m_flags,OMX_COMPONENT_INPUT_FLUSH_PENDING);
                  DEBUG_PRINT_LOW("\n Input Flush completed - Notify Client");
                  pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                           OMX_EventCmdComplete,OMX_CommandFlush,
                                           OMX_CORE_INPUT_PORT_INDEX,NULL );
                }
                if (BITMASK_PRESENT(&pThis->m_flags,
                                         OMX_COMPONENT_IDLE_PENDING))
                {
                  if (!pThis->output_flush_progress)
                  {
                     DEBUG_PRINT_LOW("\n Output flush done hence issue stop");
                     if (/*ioctl (pThis->drv_ctx.video_driver_fd,
                                VDEC_IOCTL_CMD_STOP,NULL ) < 0*/0)
                     {
                       DEBUG_PRINT_ERROR("\n VDEC_IOCTL_CMD_STOP failed");
                       pThis->omx_report_error ();
                     }
                  }
                }
              }
            }
            else
            {
              DEBUG_PRINT_ERROR("ERROR: %s()::EventHandler is NULL", __func__);
            }
          }
          break;

        case OMX_COMPONENT_GENERATE_EVENT_OUTPUT_FLUSH:
          DEBUG_PRINT_HIGH("\n Driver flush o/p Port complete");
          if (!pThis->output_flush_progress)
          {
            DEBUG_PRINT_ERROR("\n WARNING: Unexpected flush from driver");
          }
          else
          {
            pThis->execute_output_flush();
            if (pThis->m_cb.EventHandler)
            {
              if (p2 != VDEC_S_SUCCESS)
              {
                DEBUG_PRINT_ERROR("\n OMX_COMPONENT_GENERATE_EVENT_OUTPUT_FLUSH failed");
                pThis->omx_report_error ();
              }
              else
              {
                /*Check if we need generate event for Flush done*/
                if(BITMASK_PRESENT(&pThis->m_flags,
                                   OMX_COMPONENT_OUTPUT_FLUSH_PENDING))
                {
                  DEBUG_PRINT_LOW("\n Notify Output Flush done");
                  BITMASK_CLEAR (&pThis->m_flags,OMX_COMPONENT_OUTPUT_FLUSH_PENDING);
                  pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                           OMX_EventCmdComplete,OMX_CommandFlush,
                                           OMX_CORE_OUTPUT_PORT_INDEX,NULL );
                }
                if(BITMASK_PRESENT(&pThis->m_flags,
                       OMX_COMPONENT_OUTPUT_FLUSH_IN_DISABLE_PENDING))
                {
                  DEBUG_PRINT_LOW("\n Internal flush complete");
                  BITMASK_CLEAR (&pThis->m_flags,
                                 OMX_COMPONENT_OUTPUT_FLUSH_IN_DISABLE_PENDING);
                  if (BITMASK_PRESENT(&pThis->m_flags,
                          OMX_COMPONENT_DISABLE_OUTPUT_DEFERRED))
                  {
                    pThis->post_event(OMX_CommandPortDisable,
                               OMX_CORE_OUTPUT_PORT_INDEX,
                               OMX_COMPONENT_GENERATE_EVENT);
                    BITMASK_CLEAR (&pThis->m_flags,
                                   OMX_COMPONENT_DISABLE_OUTPUT_DEFERRED);

                  }
                }

                if (BITMASK_PRESENT(&pThis->m_flags ,OMX_COMPONENT_IDLE_PENDING))
                {
                  if (!pThis->input_flush_progress)
                  {
                    DEBUG_PRINT_LOW("\n Input flush done hence issue stop");
                    if (/*ioctl (pThis->drv_ctx.video_driver_fd,
                               VDEC_IOCTL_CMD_STOP,NULL ) < */0)
                    {
                      DEBUG_PRINT_ERROR("\n VDEC_IOCTL_CMD_STOP failed");
                      pThis->omx_report_error ();
                    }
                  }
                }
              }
            }
            else
            {
              DEBUG_PRINT_ERROR("ERROR: %s()::EventHandler is NULL", __func__);
            }
          }
          break;

        case OMX_COMPONENT_GENERATE_START_DONE:
          DEBUG_PRINT_HIGH("\n Rxd OMX_COMPONENT_GENERATE_START_DONE");

          if (pThis->m_cb.EventHandler)
          {
            if (p2 != VDEC_S_SUCCESS)
            {
              DEBUG_PRINT_ERROR("\n OMX_COMPONENT_GENERATE_START_DONE Failure");
              pThis->omx_report_error ();
            }
            else
            {
              DEBUG_PRINT_LOW("\n OMX_COMPONENT_GENERATE_START_DONE Success");
              if(BITMASK_PRESENT(&pThis->m_flags,OMX_COMPONENT_EXECUTE_PENDING))
              {
                DEBUG_PRINT_LOW("\n Move to executing");
                // Send the callback now
                BITMASK_CLEAR((&pThis->m_flags),OMX_COMPONENT_EXECUTE_PENDING);
                pThis->m_state = OMX_StateExecuting;
                pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                       OMX_EventCmdComplete,OMX_CommandStateSet,
                                       OMX_StateExecuting, NULL);
              }
              else if (BITMASK_PRESENT(&pThis->m_flags,
                                       OMX_COMPONENT_PAUSE_PENDING))
              {
                if (/*ioctl (pThis->drv_ctx.video_driver_fd,
                           VDEC_IOCTL_CMD_PAUSE,NULL ) < */0)
                {
                  DEBUG_PRINT_ERROR("\n VDEC_IOCTL_CMD_PAUSE failed");
                  pThis->omx_report_error ();
                }
              }
            }
          }
          else
          {
            DEBUG_PRINT_LOW("\n Event Handler callback is NULL");
          }
          break;

        case OMX_COMPONENT_GENERATE_PAUSE_DONE:
          DEBUG_PRINT_HIGH("\n Rxd OMX_COMPONENT_GENERATE_PAUSE_DONE");
          if (pThis->m_cb.EventHandler)
          {
            if (p2 != VDEC_S_SUCCESS)
            {
              DEBUG_PRINT_ERROR("OMX_COMPONENT_GENERATE_PAUSE_DONE ret failed");
              pThis->omx_report_error ();
            }
            else
            {
              pThis->complete_pending_buffer_done_cbs();
              if(BITMASK_PRESENT(&pThis->m_flags,OMX_COMPONENT_PAUSE_PENDING))
              {
                DEBUG_PRINT_LOW("\n OMX_COMPONENT_GENERATE_PAUSE_DONE nofity");
                //Send the callback now
                BITMASK_CLEAR((&pThis->m_flags),OMX_COMPONENT_PAUSE_PENDING);
                pThis->m_state = OMX_StatePause;
                pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                       OMX_EventCmdComplete,OMX_CommandStateSet,
                                       OMX_StatePause, NULL);
              }
            }
          }
          else
          {
            DEBUG_PRINT_ERROR("ERROR: %s()::EventHandler is NULL", __func__);
          }

          break;

        case OMX_COMPONENT_GENERATE_RESUME_DONE:
          DEBUG_PRINT_HIGH("\n Rxd OMX_COMPONENT_GENERATE_RESUME_DONE");
          if (pThis->m_cb.EventHandler)
          {
            if (p2 != VDEC_S_SUCCESS)
            {
              DEBUG_PRINT_ERROR("\n OMX_COMPONENT_GENERATE_RESUME_DONE failed");
              pThis->omx_report_error ();
            }
            else
            {
              if(BITMASK_PRESENT(&pThis->m_flags,OMX_COMPONENT_EXECUTE_PENDING))
              {
                DEBUG_PRINT_LOW("\n Moving the decoder to execute state");
                // Send the callback now
                BITMASK_CLEAR((&pThis->m_flags),OMX_COMPONENT_EXECUTE_PENDING);
                pThis->m_state = OMX_StateExecuting;
                pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                       OMX_EventCmdComplete,OMX_CommandStateSet,
                                       OMX_StateExecuting,NULL);
              }
            }
          }
          else
          {
            DEBUG_PRINT_ERROR("ERROR: %s()::EventHandler is NULL", __func__);
          }

          break;

        case OMX_COMPONENT_GENERATE_STOP_DONE:
          DEBUG_PRINT_HIGH("\n Rxd OMX_COMPONENT_GENERATE_STOP_DONE");
          if (pThis->m_cb.EventHandler)
          {
            if (p2 != VDEC_S_SUCCESS)
            {
              DEBUG_PRINT_ERROR("\n OMX_COMPONENT_GENERATE_STOP_DONE ret failed");
              pThis->omx_report_error ();
            }
            else
            {
              pThis->complete_pending_buffer_done_cbs();
              if(BITMASK_PRESENT(&pThis->m_flags,OMX_COMPONENT_IDLE_PENDING))
              {
                DEBUG_PRINT_LOW("\n OMX_COMPONENT_GENERATE_STOP_DONE Success");
                // Send the callback now
                BITMASK_CLEAR((&pThis->m_flags),OMX_COMPONENT_IDLE_PENDING);
                pThis->m_state = OMX_StateIdle;
                DEBUG_PRINT_LOW("\n Move to Idle State");
                pThis->m_cb.EventHandler(&pThis->m_cmp,pThis->m_app_data,
                                         OMX_EventCmdComplete,OMX_CommandStateSet,
                                         OMX_StateIdle,NULL);
              }
            }
          }
          else
          {
            DEBUG_PRINT_ERROR("ERROR: %s()::EventHandler is NULL", __func__);
          }

          break;

        case OMX_COMPONENT_GENERATE_PORT_RECONFIG:
          DEBUG_PRINT_HIGH("\n Rxd OMX_COMPONENT_GENERATE_PORT_RECONFIG");
          if (pThis->start_port_reconfig() != OMX_ErrorNone)
              pThis->omx_report_error();
          else
          {
            if (pThis->in_reconfig)
            {
              if (pThis->m_cb.EventHandler) {
                pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                    OMX_EventPortSettingsChanged, OMX_CORE_OUTPUT_PORT_INDEX, 0, NULL );
              } else {
                DEBUG_PRINT_ERROR("ERROR: %s()::EventHandler is NULL", __func__);
              }
            }
            if (pThis->drv_ctx.interlace != VDEC_InterlaceFrameProgressive)
            {
              OMX_INTERLACETYPE format = (OMX_INTERLACETYPE)-1;
              OMX_EVENTTYPE event = (OMX_EVENTTYPE)OMX_EventIndexsettingChanged;
              if (pThis->drv_ctx.interlace == VDEC_InterlaceInterleaveFrameTopFieldFirst)
                  format = OMX_InterlaceInterleaveFrameTopFieldFirst;
              else if (pThis->drv_ctx.interlace == VDEC_InterlaceInterleaveFrameBottomFieldFirst)
                  format = OMX_InterlaceInterleaveFrameBottomFieldFirst;
              else //unsupported interlace format; raise a error
                  event = OMX_EventError;
              if (pThis->m_cb.EventHandler) {
                pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                    event, format, 0, NULL );
              } else {
                DEBUG_PRINT_ERROR("ERROR: %s()::EventHandler is NULL", __func__);
              }
            }
          }
        break;

        case OMX_COMPONENT_GENERATE_EOS_DONE:
          DEBUG_PRINT_HIGH("\n Rxd OMX_COMPONENT_GENERATE_EOS_DONE");
          if (pThis->m_cb.EventHandler) {
            pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data, OMX_EventBufferFlag,
                            OMX_CORE_OUTPUT_PORT_INDEX, OMX_BUFFERFLAG_EOS, NULL );
          } else {
            DEBUG_PRINT_ERROR("ERROR: %s()::EventHandler is NULL", __func__);
          }
          pThis->prev_ts = LLONG_MAX;
          pThis->rst_prev_ts = true;
          break;

        case OMX_COMPONENT_GENERATE_HARDWARE_ERROR:
          DEBUG_PRINT_ERROR("\n OMX_COMPONENT_GENERATE_HARDWARE_ERROR");
          pThis->omx_report_error ();
          break;
        case OMX_COMPONENT_GENERATE_INFO_PORT_RECONFIG:
        {
          DEBUG_PRINT_HIGH("\n Rxd OMX_COMPONENT_GENERATE_INFO_PORT_RECONFIG");
          if (pThis->m_cb.EventHandler) {
            pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                (OMX_EVENTTYPE)OMX_EventIndexsettingChanged, OMX_CORE_OUTPUT_PORT_INDEX, 0, NULL );
          } else {
            DEBUG_PRINT_ERROR("ERROR: %s()::EventHandler is NULL", __func__);
          }
        }
        default:
          break;
        }
      }
    pthread_mutex_lock(&pThis->m_lock);
    qsize = pThis->m_cmd_q.m_size;
    if (pThis->m_state != OMX_StatePause)
        qsize += (pThis->m_ftb_q.m_size + pThis->m_etb_q.m_size);
    pthread_mutex_unlock(&pThis->m_lock);
  }
  while(qsize>0);

}



/* ======================================================================
FUNCTION
  omx_vdec::ComponentInit

DESCRIPTION
  Initialize the component.

PARAMETERS
  ctxt -- Context information related to the self.
  id   -- Event identifier. This could be any of the following:
          1. Command completion event
          2. Buffer done callback event
          3. Frame done callback event

RETURN VALUE
  None.

========================================================================== */
OMX_ERRORTYPE omx_vdec::component_init(OMX_STRING role)
{

	OMX_ERRORTYPE eRet = OMX_ErrorNone;
	struct vdec_ioctl_msg ioctl_msg = {NULL,NULL};
	struct v4l2_fmtdesc fdesc;
	struct v4l2_format fmt;
	struct v4l2_requestbuffers bufreq;
	unsigned int   alignment = 0,buffer_size = 0;
	int fds[2];
	int r,ret=0;
	bool codec_ambiguous = false;
	OMX_STRING device_name = "/dev/video32";

	drv_ctx.video_driver_fd = open("/dev/video32", O_RDWR);

	DEBUG_PRINT_HIGH("\n omx_vdec::component_init(): Open returned fd %d, errno %d",
			drv_ctx.video_driver_fd, errno);

	if(drv_ctx.video_driver_fd == 0){
		drv_ctx.video_driver_fd = open(device_name, O_RDWR);
	}

	if(drv_ctx.video_driver_fd < 0)
	{
		DEBUG_PRINT_ERROR("Omx_vdec::Comp Init Returning failure, errno %d\n", errno);
		return OMX_ErrorInsufficientResources;
	}
	drv_ctx.frame_rate.fps_numerator = DEFAULT_FPS;
	drv_ctx.frame_rate.fps_denominator = 1;


#ifdef INPUT_BUFFER_LOG
	strcpy(inputfilename, INPUT_BUFFER_FILE_NAME);
#endif
#ifdef OUTPUT_BUFFER_LOG
	outputBufferFile1 = fopen (outputfilename, "ab");
#endif
#ifdef OUTPUT_EXTRADATA_LOG
	outputExtradataFile = fopen (ouputextradatafilename, "ab");
#endif

	// Copy the role information which provides the decoder kind
	strlcpy(drv_ctx.kind,role,128);
	if(!strncmp(drv_ctx.kind,"OMX.qcom.video.decoder.mpeg4",\
				OMX_MAX_STRINGNAME_SIZE))
	{
		strlcpy((char *)m_cRole, "video_decoder.mpeg4",\
				OMX_MAX_STRINGNAME_SIZE);
		drv_ctx.timestamp_adjust = true;
		drv_ctx.decoder_format = VDEC_CODECTYPE_MPEG4;
		eCompressionFormat = OMX_VIDEO_CodingMPEG4;
		/*Initialize Start Code for MPEG4*/
		codec_type_parse = CODEC_TYPE_MPEG4;
		m_frame_parser.init_start_codes (codec_type_parse);
#ifdef INPUT_BUFFER_LOG
		strcat(inputfilename, "m4v");
#endif
	}
	else if(!strncmp(drv_ctx.kind,"OMX.qcom.video.decoder.mpeg2",\
				OMX_MAX_STRINGNAME_SIZE))
	{
		strlcpy((char *)m_cRole, "video_decoder.mpeg2",\
				OMX_MAX_STRINGNAME_SIZE);
		drv_ctx.decoder_format = VDEC_CODECTYPE_MPEG2;
		eCompressionFormat = OMX_VIDEO_CodingMPEG2;
		/*Initialize Start Code for MPEG2*/
		codec_type_parse = CODEC_TYPE_MPEG2;
		m_frame_parser.init_start_codes (codec_type_parse);
#ifdef INPUT_BUFFER_LOG
		strcat(inputfilename, "mpg");
#endif
	}
	else if(!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.h263",\
				OMX_MAX_STRINGNAME_SIZE))
	{
		strlcpy((char *)m_cRole, "video_decoder.h263",OMX_MAX_STRINGNAME_SIZE);
		DEBUG_PRINT_LOW("\n H263 Decoder selected");
		drv_ctx.decoder_format = VDEC_CODECTYPE_H263;
		eCompressionFormat = OMX_VIDEO_CodingH263;
		codec_type_parse = CODEC_TYPE_H263;
		m_frame_parser.init_start_codes (codec_type_parse);
#ifdef INPUT_BUFFER_LOG
		strcat(inputfilename, "263");
#endif
	}
	else if(!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.divx311",\
				OMX_MAX_STRINGNAME_SIZE))
	{
		strlcpy((char *)m_cRole, "video_decoder.divx",OMX_MAX_STRINGNAME_SIZE);
		DEBUG_PRINT_LOW ("\n DIVX 311 Decoder selected");
		drv_ctx.decoder_format = VDEC_CODECTYPE_DIVX_3;
		output_capability = V4L2_PIX_FMT_DIVX_311;
		eCompressionFormat = (OMX_VIDEO_CODINGTYPE)QOMX_VIDEO_CodingDivx;
		codec_type_parse = CODEC_TYPE_DIVX;
		m_frame_parser.init_start_codes (codec_type_parse);

	}
	else if(!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.divx4",\
				OMX_MAX_STRINGNAME_SIZE))
	{
		strlcpy((char *)m_cRole, "video_decoder.divx",OMX_MAX_STRINGNAME_SIZE);
		DEBUG_PRINT_ERROR ("\n DIVX 4 Decoder selected");
		drv_ctx.decoder_format = VDEC_CODECTYPE_DIVX_4;
		output_capability = V4L2_PIX_FMT_DIVX;
		eCompressionFormat = (OMX_VIDEO_CODINGTYPE)QOMX_VIDEO_CodingDivx;
		codec_type_parse = CODEC_TYPE_DIVX;
		codec_ambiguous = true;
		m_frame_parser.init_start_codes (codec_type_parse);

	}
	else if(!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.divx",\
				OMX_MAX_STRINGNAME_SIZE))
	{
		strlcpy((char *)m_cRole, "video_decoder.divx",OMX_MAX_STRINGNAME_SIZE);
		DEBUG_PRINT_ERROR ("\n DIVX 5/6 Decoder selected");
		drv_ctx.decoder_format = VDEC_CODECTYPE_DIVX_6;
		output_capability = V4L2_PIX_FMT_DIVX;
		eCompressionFormat = (OMX_VIDEO_CODINGTYPE)QOMX_VIDEO_CodingDivx;
		codec_type_parse = CODEC_TYPE_DIVX;
		codec_ambiguous = true;
		m_frame_parser.init_start_codes (codec_type_parse);

	}
	else if(!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.avc",\
				OMX_MAX_STRINGNAME_SIZE))
	{
		strlcpy((char *)m_cRole, "video_decoder.avc",OMX_MAX_STRINGNAME_SIZE);
		drv_ctx.decoder_format = VDEC_CODECTYPE_H264;
		output_capability=V4L2_PIX_FMT_H264;
		eCompressionFormat = OMX_VIDEO_CodingAVC;
		codec_type_parse = CODEC_TYPE_H264;
		m_frame_parser.init_start_codes (codec_type_parse);
		m_frame_parser.init_nal_length(nal_length);
#ifdef INPUT_BUFFER_LOG
		strcat(inputfilename, "264");
#endif
	}
	else if(!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.vc1",\
				OMX_MAX_STRINGNAME_SIZE))
	{
		strlcpy((char *)m_cRole, "video_decoder.vc1",OMX_MAX_STRINGNAME_SIZE);
		drv_ctx.decoder_format = VDEC_CODECTYPE_VC1;
		eCompressionFormat = OMX_VIDEO_CodingWMV;
		codec_type_parse = CODEC_TYPE_VC1;
		m_frame_parser.init_start_codes (codec_type_parse);
#ifdef INPUT_BUFFER_LOG
		strcat(inputfilename, "vc1");
#endif
	}
	else if(!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.wmv",\
				OMX_MAX_STRINGNAME_SIZE))
	{
		strlcpy((char *)m_cRole, "video_decoder.vc1",OMX_MAX_STRINGNAME_SIZE);
		drv_ctx.decoder_format = VDEC_CODECTYPE_VC1_RCV;
		eCompressionFormat = OMX_VIDEO_CodingWMV;
		codec_type_parse = CODEC_TYPE_VC1;
		m_frame_parser.init_start_codes (codec_type_parse);
#ifdef INPUT_BUFFER_LOG
		strcat(inputfilename, "vc1");
#endif
	}
	else
	{
		DEBUG_PRINT_ERROR("\nERROR:Unknown Component\n");
		eRet = OMX_ErrorInvalidComponentName;
	}
#ifdef INPUT_BUFFER_LOG
	inputBufferFile1 = fopen (inputfilename, "ab");
#endif
	if (eRet == OMX_ErrorNone)
	{

		drv_ctx.output_format = VDEC_YUV_FORMAT_TILE_4x2;
		capture_capability= V4L2_PIX_FMT_NV12;

		struct v4l2_event_subscription sub;
		sub.type=V4L2_EVENT_ALL;
		ret = ioctl(drv_ctx.video_driver_fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
		if (ret) {
		printf("\n Subscribe Event Failed \n");
		return OMX_ErrorInsufficientResources;
		}

		struct v4l2_capability cap;
		ret = ioctl(drv_ctx.video_driver_fd, VIDIOC_QUERYCAP, &cap);
		if (ret) {
		  printf("Failed to query capabilities\n");
		  /*TODO: How to handle this case */
		} else {
		  printf("Capabilities: driver_name = %s, card = %s, bus_info = %s,"
				" version = %d, capabilities = %x\n", cap.driver, cap.card,
				cap.bus_info, cap.version, cap.capabilities);
		}
		ret=0;
		fdesc.type=V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		fdesc.index=0;
		while (ioctl(drv_ctx.video_driver_fd, VIDIOC_ENUM_FMT, &fdesc) == 0) {
			printf("fmt: description: %s, fmt: %x, flags = %x\n", fdesc.description,
					fdesc.pixelformat, fdesc.flags);
			fdesc.index++;
		}
		fdesc.type=V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
		fdesc.index=0;
		while (ioctl(drv_ctx.video_driver_fd, VIDIOC_ENUM_FMT, &fdesc) == 0) {

			printf("fmt: description: %s, fmt: %x, flags = %x\n", fdesc.description,
					fdesc.pixelformat, fdesc.flags);
			fdesc.index++;
		}

		drv_ctx.video_resolution.frame_height=drv_ctx.video_resolution.scan_lines=240;
		drv_ctx.video_resolution.frame_width=drv_ctx.video_resolution.stride=320;
		fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
		fmt.fmt.pix_mp.height = drv_ctx.video_resolution.frame_height;
		fmt.fmt.pix_mp.width = drv_ctx.video_resolution.frame_width;
		fmt.fmt.pix_mp.pixelformat = output_capability;
		ret = ioctl(drv_ctx.video_driver_fd, VIDIOC_S_FMT, &fmt);
		if (ret) {
			/*TODO: How to handle this case */
			printf("Failed to set format on capture port\n");
				}
		printf("\n Set Format was successful \n ");
		if (codec_ambiguous) {
			if (output_capability == V4L2_PIX_FMT_DIVX) {
				struct v4l2_control divx_ctrl;

				if (drv_ctx.decoder_format == VDEC_CODECTYPE_DIVX_4) {
					divx_ctrl.id = V4L2_MPEG_VIDC_VIDEO_DIVX_FORMAT_4;
				} else if (drv_ctx.decoder_format == VDEC_CODECTYPE_DIVX_5) {
					divx_ctrl.id = V4L2_MPEG_VIDC_VIDEO_DIVX_FORMAT_5;
				} else {
					divx_ctrl.id = V4L2_MPEG_VIDC_VIDEO_DIVX_FORMAT_5;
				}

				divx_ctrl.value = V4L2_CID_MPEG_VIDC_VIDEO_DIVX_FORMAT;
				ret = ioctl(drv_ctx.video_driver_fd, VIDIOC_S_CTRL, &fmt);
				if (ret) {
					DEBUG_PRINT_ERROR("Failed to set divx version\n");
				}
			} else {
				DEBUG_PRINT_ERROR("Codec should not be ambiguous");
			}
		}

		fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		fmt.fmt.pix_mp.height = drv_ctx.video_resolution.frame_height;
		fmt.fmt.pix_mp.width = drv_ctx.video_resolution.frame_width;
		fmt.fmt.pix_mp.pixelformat = output_capability;
		ret = ioctl(drv_ctx.video_driver_fd, VIDIOC_S_FMT, &fmt);
		if (ret) {
			/*TODO: How to handle this case */
			printf("Failed to set format on capture port\n");
				}
		printf("\n Set Format was successful \n ");

		/*Get the Buffer requirements for input and output ports*/
		drv_ctx.ip_buf.buffer_type = VDEC_BUFFER_TYPE_INPUT;
		drv_ctx.op_buf.buffer_type = VDEC_BUFFER_TYPE_OUTPUT;
		drv_ctx.op_buf.alignment=4096;
		drv_ctx.ip_buf.alignment=4096;
		drv_ctx.interlace = VDEC_InterlaceFrameProgressive;
		drv_ctx.extradata = 0;
		drv_ctx.picture_order = VDEC_ORDER_DECODE;
		drv_ctx.idr_only_decoding = 0;

		m_state = OMX_StateLoaded;
		eRet=get_buffer_req(&drv_ctx.ip_buf);
		printf("Input Buffer Size =%d \n ",drv_ctx.ip_buf.buffer_size);

#ifdef DEFAULT_EXTRADATA
		if (eRet == OMX_ErrorNone && !secure_mode)
			eRet = enable_extradata(DEFAULT_EXTRADATA);
#endif
		if (drv_ctx.decoder_format == VDEC_CODECTYPE_H264)
		{
			if (m_frame_parser.mutils == NULL)
			{
				m_frame_parser.mutils = new H264_Utils();

				if (m_frame_parser.mutils == NULL)
				{
					DEBUG_PRINT_ERROR("\n parser utils Allocation failed ");
					eRet = OMX_ErrorInsufficientResources;
				}
				else
				{
					h264_scratch.nAllocLen = drv_ctx.ip_buf.buffer_size;
					h264_scratch.pBuffer = (OMX_U8 *)malloc (drv_ctx.ip_buf.buffer_size);
					h264_scratch.nFilledLen = 0;
					h264_scratch.nOffset = 0;

					if (h264_scratch.pBuffer == NULL)
					{
						DEBUG_PRINT_ERROR("\n h264_scratch.pBuffer Allocation failed ");
						return OMX_ErrorInsufficientResources;
					}
					m_frame_parser.mutils->initialize_frame_checking_environment();
					m_frame_parser.mutils->allocate_rbsp_buffer (drv_ctx.ip_buf.buffer_size);
				}
			}

			h264_parser = new h264_stream_parser();
			if (!h264_parser)
			{
				DEBUG_PRINT_ERROR("ERROR: H264 parser allocation failed!");
				eRet = OMX_ErrorInsufficientResources;
			}
		}

		if(pipe(fds))
		{
			DEBUG_PRINT_ERROR("pipe creation failed\n");
			eRet = OMX_ErrorInsufficientResources;
		}
		else
		{
			int temp1[2];
			if(fds[0] == 0 || fds[1] == 0)
			{
				if (pipe (temp1))
				{
					DEBUG_PRINT_ERROR("pipe creation failed\n");
					return OMX_ErrorInsufficientResources;
				}
				//close (fds[0]);
				//close (fds[1]);
				fds[0] = temp1 [0];
				fds[1] = temp1 [1];
			}
			m_pipe_in = fds[0];
			m_pipe_out = fds[1];
			r = pthread_create(&msg_thread_id,0,message_thread,this);

			if(r < 0)
			{
				DEBUG_PRINT_ERROR("\n component_init(): message_thread creation failed");
				eRet = OMX_ErrorInsufficientResources;
			}
		}
	}

	if (eRet != OMX_ErrorNone)
	{
		DEBUG_PRINT_ERROR("\n Component Init Failed");
		DEBUG_PRINT_HIGH("\n Calling VDEC_IOCTL_STOP_NEXT_MSG");
		(void)ioctl(drv_ctx.video_driver_fd, VDEC_IOCTL_STOP_NEXT_MSG,
				NULL);
		DEBUG_PRINT_HIGH("\n Calling close() on Video Driver");
		close (drv_ctx.video_driver_fd);
		drv_ctx.video_driver_fd = -1;
	}
	else
	{
		DEBUG_PRINT_HIGH("\n omx_vdec::component_init() success");
	}

	//memset(&h264_mv_buff,0,sizeof(struct h264_mv_buffer));
	return eRet;
}

/* ======================================================================
FUNCTION
  omx_vdec::GetComponentVersion

DESCRIPTION
  Returns the component version.

PARAMETERS
  TBD.

RETURN VALUE
  OMX_ErrorNone.

========================================================================== */
OMX_ERRORTYPE  omx_vdec::get_component_version
                                     (
                                      OMX_IN OMX_HANDLETYPE hComp,
                                      OMX_OUT OMX_STRING componentName,
                                      OMX_OUT OMX_VERSIONTYPE* componentVersion,
                                      OMX_OUT OMX_VERSIONTYPE* specVersion,
                                      OMX_OUT OMX_UUIDTYPE* componentUUID
                                      )
{
    if(m_state == OMX_StateInvalid)
    {
        DEBUG_PRINT_ERROR("Get Comp Version in Invalid State\n");
        return OMX_ErrorInvalidState;
    }
  /* TBD -- Return the proper version */
  if (specVersion)
  {
    specVersion->nVersion = OMX_SPEC_VERSION;
  }
  return OMX_ErrorNone;
}
/* ======================================================================
FUNCTION
  omx_vdec::SendCommand

DESCRIPTION
  Returns zero if all the buffers released..

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
OMX_ERRORTYPE  omx_vdec::send_command(OMX_IN OMX_HANDLETYPE hComp,
                                      OMX_IN OMX_COMMANDTYPE cmd,
                                      OMX_IN OMX_U32 param1,
                                      OMX_IN OMX_PTR cmdData
                                      )
{
    DEBUG_PRINT_LOW("\n send_command: Recieved a Command from Client");
    if(m_state == OMX_StateInvalid)
    {
        DEBUG_PRINT_ERROR("ERROR: Send Command in Invalid State\n");
        return OMX_ErrorInvalidState;
    }
    if (cmd == OMX_CommandFlush && param1 != OMX_CORE_INPUT_PORT_INDEX
      && param1 != OMX_CORE_OUTPUT_PORT_INDEX && param1 != OMX_ALL)
    {
      DEBUG_PRINT_ERROR("\n send_command(): ERROR OMX_CommandFlush "
        "to invalid port: %d", param1);
      return OMX_ErrorBadPortIndex;
    }
    post_event((unsigned)cmd,(unsigned)param1,OMX_COMPONENT_GENERATE_COMMAND);
    sem_wait(&m_cmd_lock);
    DEBUG_PRINT_LOW("\n send_command: Command Processed\n");
    return OMX_ErrorNone;
}

/* ======================================================================
FUNCTION
  omx_vdec::SendCommand

DESCRIPTION
  Returns zero if all the buffers released..

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
OMX_ERRORTYPE  omx_vdec::send_command_proxy(OMX_IN OMX_HANDLETYPE hComp,
                                            OMX_IN OMX_COMMANDTYPE cmd,
                                            OMX_IN OMX_U32 param1,
                                            OMX_IN OMX_PTR cmdData
                                            )
{
  OMX_ERRORTYPE eRet = OMX_ErrorNone;
  OMX_STATETYPE eState = (OMX_STATETYPE) param1;
  int bFlag = 1,sem_posted = 0,ret=0;

  DEBUG_PRINT_LOW("\n send_command_proxy(): cmd = %d", cmd);
  DEBUG_PRINT_HIGH("\n send_command_proxy(): Current State %d, Expected State %d",
    m_state, eState);

  if(cmd == OMX_CommandStateSet)
  {
    DEBUG_PRINT_HIGH("\n send_command_proxy(): OMX_CommandStateSet issued");
    DEBUG_PRINT_HIGH("\n Current State %d, Expected State %d", m_state, eState);
    /***************************/
    /* Current State is Loaded */
    /***************************/
    if(m_state == OMX_StateLoaded)
    {
      if(eState == OMX_StateIdle)
      {
        //if all buffers are allocated or all ports disabled
        if(allocate_done() ||
          (m_inp_bEnabled == OMX_FALSE && m_out_bEnabled == OMX_FALSE))
        {
          DEBUG_PRINT_LOW("send_command_proxy(): Loaded-->Idle\n");
        }
        else
        {
          DEBUG_PRINT_LOW("send_command_proxy(): Loaded-->Idle-Pending\n");
          BITMASK_SET(&m_flags, OMX_COMPONENT_IDLE_PENDING);
          // Skip the event notification
          bFlag = 0;
        }
      }
      /* Requesting transition from Loaded to Loaded */
      else if(eState == OMX_StateLoaded)
      {
        DEBUG_PRINT_ERROR("ERROR::send_command_proxy(): Loaded-->Loaded\n");
        post_event(OMX_EventError,OMX_ErrorSameState,\
                   OMX_COMPONENT_GENERATE_EVENT);
        eRet = OMX_ErrorSameState;
      }
      /* Requesting transition from Loaded to WaitForResources */
      else if(eState == OMX_StateWaitForResources)
      {
        /* Since error is None , we will post an event
           at the end of this function definition */
        DEBUG_PRINT_LOW("send_command_proxy(): Loaded-->WaitForResources\n");
      }
      /* Requesting transition from Loaded to Executing */
      else if(eState == OMX_StateExecuting)
      {
        DEBUG_PRINT_ERROR("ERROR::send_command_proxy(): Loaded-->Executing\n");
        post_event(OMX_EventError,OMX_ErrorIncorrectStateTransition,\
                   OMX_COMPONENT_GENERATE_EVENT);
        eRet = OMX_ErrorIncorrectStateTransition;
      }
      /* Requesting transition from Loaded to Pause */
      else if(eState == OMX_StatePause)
      {
        DEBUG_PRINT_ERROR("ERROR::send_command_proxy(): Loaded-->Pause\n");
        post_event(OMX_EventError,OMX_ErrorIncorrectStateTransition,\
                   OMX_COMPONENT_GENERATE_EVENT);
        eRet = OMX_ErrorIncorrectStateTransition;
      }
      /* Requesting transition from Loaded to Invalid */
      else if(eState == OMX_StateInvalid)
      {
        DEBUG_PRINT_ERROR("ERROR::send_command_proxy(): Loaded-->Invalid\n");
        post_event(OMX_EventError,eState,OMX_COMPONENT_GENERATE_EVENT);
        eRet = OMX_ErrorInvalidState;
      }
      else
      {
        DEBUG_PRINT_ERROR("ERROR::send_command_proxy(): Loaded-->Invalid(%d Not Handled)\n",\
                          eState);
        eRet = OMX_ErrorBadParameter;
      }
    }

    /***************************/
    /* Current State is IDLE */
    /***************************/
    else if(m_state == OMX_StateIdle)
    {
      if(eState == OMX_StateLoaded)
      {
        if(release_done())
        {
          /*
             Since error is None , we will post an event at the end
             of this function definition
          */
          DEBUG_PRINT_LOW("send_command_proxy(): Idle-->Loaded\n");
        }
        else
        {
          DEBUG_PRINT_LOW("send_command_proxy(): Idle-->Loaded-Pending\n");
          BITMASK_SET(&m_flags, OMX_COMPONENT_LOADING_PENDING);
          // Skip the event notification
          bFlag = 0;
        }
      }
      /* Requesting transition from Idle to Executing */
      else if(eState == OMX_StateExecuting)
      {
	    DEBUG_PRINT_LOW("send_command_proxy(): Idle-->Executing\n");
        //BITMASK_SET(&m_flags, OMX_COMPONENT_EXECUTE_PENDING);
        bFlag = 1;
	    DEBUG_PRINT_LOW("send_command_proxy(): Idle-->Executing\n");
	    m_state=OMX_StateExecuting;
	    printf("Stream On CAPTURE Was successful\n");
      }
      /* Requesting transition from Idle to Idle */
      else if(eState == OMX_StateIdle)
      {
        DEBUG_PRINT_ERROR("ERROR::send_command_proxy(): Idle-->Idle\n");
        post_event(OMX_EventError,OMX_ErrorSameState,\
                   OMX_COMPONENT_GENERATE_EVENT);
        eRet = OMX_ErrorSameState;
      }
      /* Requesting transition from Idle to WaitForResources */
      else if(eState == OMX_StateWaitForResources)
      {
        DEBUG_PRINT_ERROR("ERROR::send_command_proxy(): Idle-->WaitForResources\n");
        post_event(OMX_EventError,OMX_ErrorIncorrectStateTransition,\
                   OMX_COMPONENT_GENERATE_EVENT);
        eRet = OMX_ErrorIncorrectStateTransition;
      }
       /* Requesting transition from Idle to Pause */
       else if(eState == OMX_StatePause)
      {
         /*To pause the Video core we need to start the driver*/
         if (/*ioctl (drv_ctx.video_driver_fd,VDEC_IOCTL_CMD_START,
                    NULL) < */0)
         {
           DEBUG_PRINT_ERROR("\n VDEC_IOCTL_CMD_START FAILED");
           omx_report_error ();
           eRet = OMX_ErrorHardware;
         }
         else
         {
           BITMASK_SET(&m_flags,OMX_COMPONENT_PAUSE_PENDING);
           DEBUG_PRINT_LOW("send_command_proxy(): Idle-->Pause\n");
           bFlag = 0;
         }
      }
      /* Requesting transition from Idle to Invalid */
       else if(eState == OMX_StateInvalid)
      {
        DEBUG_PRINT_ERROR("ERROR::send_command_proxy(): Idle-->Invalid\n");
        post_event(OMX_EventError,eState,OMX_COMPONENT_GENERATE_EVENT);
        eRet = OMX_ErrorInvalidState;
      }
      else
      {
        DEBUG_PRINT_ERROR("ERROR::send_command_proxy(): Idle --> %d Not Handled\n",eState);
        eRet = OMX_ErrorBadParameter;
      }
    }

    /******************************/
    /* Current State is Executing */
    /******************************/
    else if(m_state == OMX_StateExecuting)
    {
       DEBUG_PRINT_LOW("\n Command Recieved in OMX_StateExecuting");
       /* Requesting transition from Executing to Idle */
       if(eState == OMX_StateIdle)
       {
         /* Since error is None , we will post an event
         at the end of this function definition
         */
         DEBUG_PRINT_LOW("\n send_command_proxy(): Executing --> Idle \n");
         //BITMASK_SET(&m_flags,OMX_COMPONENT_IDLE_PENDING);
         if(!sem_posted)
         {
           sem_posted = 1;
           sem_post (&m_cmd_lock);
           execute_omx_flush(OMX_ALL);
         }
         bFlag = 1;
	 int rc=0;
	 enum v4l2_buf_type btype;
	 btype = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	 rc = ioctl(drv_ctx.video_driver_fd, VIDIOC_STREAMOFF, &btype);
	 if (rc) {
		 /*TODO: How to handle this case */
		 printf("\n Failed to call streamoff on OUTPUT Port \n");
	 } else {
		 streaming[OUTPUT_PORT] = false;
	 }
	 btype = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	 rc = ioctl(drv_ctx.video_driver_fd, VIDIOC_STREAMOFF, &btype);
	 if (rc) {
		 /*TODO: How to handle this case */
		 printf("\n Failed to call streamoff on CAPTURE Port \n");
	 } else {
		 streaming[CAPTURE_PORT] = false;
	 }
		struct v4l2_event_subscription sub;
		sub.type=V4L2_EVENT_ALL;
		ret = ioctl(drv_ctx.video_driver_fd, VIDIOC_UNSUBSCRIBE_EVENT, &sub);
		if (ret) {
		printf("\n Subscribe Event Failed \n");
		eRet = OMX_ErrorHardware;
		}
	 m_state == OMX_StateIdle;
       }
       /* Requesting transition from Executing to Paused */
       else if(eState == OMX_StatePause)
       {
         DEBUG_PRINT_LOW("\n PAUSE Command Issued");
         if (/*ioctl (drv_ctx.video_driver_fd,VDEC_IOCTL_CMD_PAUSE,
                    NULL) < */0)
         {
           DEBUG_PRINT_ERROR("\n Error In Pause State");
           post_event(OMX_EventError,OMX_ErrorHardware,\
                      OMX_COMPONENT_GENERATE_EVENT);
           eRet = OMX_ErrorHardware;
         }
         else
         {
           BITMASK_SET(&m_flags,OMX_COMPONENT_PAUSE_PENDING);
           DEBUG_PRINT_LOW("send_command_proxy(): Executing-->Pause\n");
           bFlag = 0;
         }
       }
       /* Requesting transition from Executing to Loaded */
       else if(eState == OMX_StateLoaded)
       {
         DEBUG_PRINT_ERROR("\n send_command_proxy(): Executing --> Loaded \n");
         post_event(OMX_EventError,OMX_ErrorIncorrectStateTransition,\
                    OMX_COMPONENT_GENERATE_EVENT);
         eRet = OMX_ErrorIncorrectStateTransition;
       }
       /* Requesting transition from Executing to WaitForResources */
       else if(eState == OMX_StateWaitForResources)
       {
         DEBUG_PRINT_ERROR("\n send_command_proxy(): Executing --> WaitForResources \n");
         post_event(OMX_EventError,OMX_ErrorIncorrectStateTransition,\
                    OMX_COMPONENT_GENERATE_EVENT);
         eRet = OMX_ErrorIncorrectStateTransition;
       }
       /* Requesting transition from Executing to Executing */
       else if(eState == OMX_StateExecuting)
       {
         DEBUG_PRINT_ERROR("\n send_command_proxy(): Executing --> Executing \n");
         post_event(OMX_EventError,OMX_ErrorSameState,\
                    OMX_COMPONENT_GENERATE_EVENT);
         eRet = OMX_ErrorSameState;
       }
       /* Requesting transition from Executing to Invalid */
       else if(eState == OMX_StateInvalid)
       {
         DEBUG_PRINT_ERROR("\n send_command_proxy(): Executing --> Invalid \n");
         post_event(OMX_EventError,eState,OMX_COMPONENT_GENERATE_EVENT);
         eRet = OMX_ErrorInvalidState;
       }
       else
       {
         DEBUG_PRINT_ERROR("ERROR::send_command_proxy(): Executing --> %d Not Handled\n",eState);
         eRet = OMX_ErrorBadParameter;
       }
    }
    /***************************/
    /* Current State is Pause  */
    /***************************/
    else if(m_state == OMX_StatePause)
    {
      /* Requesting transition from Pause to Executing */
      if(eState == OMX_StateExecuting)
      {
        DEBUG_PRINT_LOW("\n Pause --> Executing \n");
        if (/*ioctl (drv_ctx.video_driver_fd,VDEC_IOCTL_CMD_RESUME,
                   NULL) < */0)
        {
          DEBUG_PRINT_ERROR("\n VDEC_IOCTL_CMD_RESUME failed");
          post_event(OMX_EventError,OMX_ErrorHardware,\
                     OMX_COMPONENT_GENERATE_EVENT);
          eRet = OMX_ErrorHardware;
        }
        else
        {
          BITMASK_SET(&m_flags,OMX_COMPONENT_EXECUTE_PENDING);
          DEBUG_PRINT_LOW("send_command_proxy(): Idle-->Executing\n");
          post_event (NULL,VDEC_S_SUCCESS,\
                      OMX_COMPONENT_GENERATE_RESUME_DONE);
          bFlag = 0;
        }
      }
      /* Requesting transition from Pause to Idle */
      else if(eState == OMX_StateIdle)
      {
        /* Since error is None , we will post an event
        at the end of this function definition */
        DEBUG_PRINT_LOW("\n Pause --> Idle \n");
         BITMASK_SET(&m_flags,OMX_COMPONENT_IDLE_PENDING);
         if(!sem_posted)
         {
           sem_posted = 1;
           sem_post (&m_cmd_lock);
           execute_omx_flush(OMX_ALL);
         }
         bFlag = 0;
      }
      /* Requesting transition from Pause to loaded */
      else if(eState == OMX_StateLoaded)
      {
        DEBUG_PRINT_ERROR("\n Pause --> loaded \n");
        post_event(OMX_EventError,OMX_ErrorIncorrectStateTransition,\
                   OMX_COMPONENT_GENERATE_EVENT);
        eRet = OMX_ErrorIncorrectStateTransition;
      }
      /* Requesting transition from Pause to WaitForResources */
      else if(eState == OMX_StateWaitForResources)
      {
        DEBUG_PRINT_ERROR("\n Pause --> WaitForResources \n");
        post_event(OMX_EventError,OMX_ErrorIncorrectStateTransition,\
                   OMX_COMPONENT_GENERATE_EVENT);
        eRet = OMX_ErrorIncorrectStateTransition;
      }
      /* Requesting transition from Pause to Pause */
      else if(eState == OMX_StatePause)
      {
        DEBUG_PRINT_ERROR("\n Pause --> Pause \n");
        post_event(OMX_EventError,OMX_ErrorSameState,\
                   OMX_COMPONENT_GENERATE_EVENT);
        eRet = OMX_ErrorSameState;
      }
       /* Requesting transition from Pause to Invalid */
      else if(eState == OMX_StateInvalid)
      {
        DEBUG_PRINT_ERROR("\n Pause --> Invalid \n");
        post_event(OMX_EventError,eState,OMX_COMPONENT_GENERATE_EVENT);
        eRet = OMX_ErrorInvalidState;
      }
      else
      {
        DEBUG_PRINT_ERROR("ERROR::send_command_proxy(): Paused --> %d Not Handled\n",eState);
        eRet = OMX_ErrorBadParameter;
      }
    }
     /***************************/
    /* Current State is WaitForResources  */
    /***************************/
    else if(m_state == OMX_StateWaitForResources)
    {
      /* Requesting transition from WaitForResources to Loaded */
      if(eState == OMX_StateLoaded)
      {
        /* Since error is None , we will post an event
        at the end of this function definition */
        DEBUG_PRINT_LOW("send_command_proxy(): WaitForResources-->Loaded\n");
      }
      /* Requesting transition from WaitForResources to WaitForResources */
      else if (eState == OMX_StateWaitForResources)
      {
        DEBUG_PRINT_ERROR("ERROR::send_command_proxy(): WaitForResources-->WaitForResources\n");
        post_event(OMX_EventError,OMX_ErrorSameState,
                   OMX_COMPONENT_GENERATE_EVENT);
        eRet = OMX_ErrorSameState;
      }
      /* Requesting transition from WaitForResources to Executing */
      else if(eState == OMX_StateExecuting)
      {
        DEBUG_PRINT_ERROR("ERROR::send_command_proxy(): WaitForResources-->Executing\n");
        post_event(OMX_EventError,OMX_ErrorIncorrectStateTransition,\
                   OMX_COMPONENT_GENERATE_EVENT);
        eRet = OMX_ErrorIncorrectStateTransition;
      }
      /* Requesting transition from WaitForResources to Pause */
      else if(eState == OMX_StatePause)
      {
        DEBUG_PRINT_ERROR("ERROR::send_command_proxy(): WaitForResources-->Pause\n");
        post_event(OMX_EventError,OMX_ErrorIncorrectStateTransition,\
                   OMX_COMPONENT_GENERATE_EVENT);
        eRet = OMX_ErrorIncorrectStateTransition;
      }
      /* Requesting transition from WaitForResources to Invalid */
      else if(eState == OMX_StateInvalid)
      {
        DEBUG_PRINT_ERROR("ERROR::send_command_proxy(): WaitForResources-->Invalid\n");
        post_event(OMX_EventError,eState,OMX_COMPONENT_GENERATE_EVENT);
        eRet = OMX_ErrorInvalidState;
      }
      /* Requesting transition from WaitForResources to Loaded -
      is NOT tested by Khronos TS */

    }
    else
    {
      DEBUG_PRINT_ERROR("ERROR::send_command_proxy(): %d --> %d(Not Handled)\n",m_state,eState);
      eRet = OMX_ErrorBadParameter;
    }
  }
  /********************************/
  /* Current State is Invalid */
  /*******************************/
  else if(m_state == OMX_StateInvalid)
  {
    /* State Transition from Inavlid to any state */
    if(eState == (OMX_StateLoaded || OMX_StateWaitForResources
                  || OMX_StateIdle || OMX_StateExecuting
                  || OMX_StatePause || OMX_StateInvalid))
    {
      DEBUG_PRINT_ERROR("ERROR::send_command_proxy(): Invalid -->Loaded\n");
      post_event(OMX_EventError,OMX_ErrorInvalidState,\
                 OMX_COMPONENT_GENERATE_EVENT);
      eRet = OMX_ErrorInvalidState;
    }
  }
  else if (cmd == OMX_CommandFlush)
  {
    DEBUG_PRINT_HIGH("\n send_command_proxy(): OMX_CommandFlush issued"
        "with param1: %d", param1);
    if(OMX_CORE_INPUT_PORT_INDEX == param1 || OMX_ALL == param1)
    {
      BITMASK_SET(&m_flags, OMX_COMPONENT_INPUT_FLUSH_PENDING);
    }
    if(OMX_CORE_OUTPUT_PORT_INDEX == param1 || OMX_ALL == param1)
    {
      BITMASK_SET(&m_flags, OMX_COMPONENT_OUTPUT_FLUSH_PENDING);
    }
    if (!sem_posted){
      sem_posted = 1;
      DEBUG_PRINT_LOW("\n Set the Semaphore");
      sem_post (&m_cmd_lock);
      execute_omx_flush(param1);
    }
    bFlag = 0;
  }
  else if ( cmd == OMX_CommandPortEnable)
  {
    DEBUG_PRINT_HIGH("\n send_command_proxy(): OMX_CommandPortEnable issued"
        "with param1: %d", param1);
    if(param1 == OMX_CORE_INPUT_PORT_INDEX || param1 == OMX_ALL)
      {
        m_inp_bEnabled = OMX_TRUE;

        if( (m_state == OMX_StateLoaded &&
             !BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING))
            || allocate_input_done())
        {
          post_event(OMX_CommandPortEnable,OMX_CORE_INPUT_PORT_INDEX,
                     OMX_COMPONENT_GENERATE_EVENT);
        }
        else
        {
          DEBUG_PRINT_LOW("send_command_proxy(): Disabled-->Enabled Pending\n");
          BITMASK_SET(&m_flags, OMX_COMPONENT_INPUT_ENABLE_PENDING);
          // Skip the event notification
          bFlag = 0;
        }
      }
      if(param1 == OMX_CORE_OUTPUT_PORT_INDEX || param1 == OMX_ALL)
      {
          DEBUG_PRINT_LOW("\n Enable output Port command recieved");
          m_out_bEnabled = OMX_TRUE;

          if( (m_state == OMX_StateLoaded &&
              !BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING))
              || (allocate_output_done()))
          {
             post_event(OMX_CommandPortEnable,OMX_CORE_OUTPUT_PORT_INDEX,
                        OMX_COMPONENT_GENERATE_EVENT);

          }
          else
          {
              DEBUG_PRINT_LOW("send_command_proxy(): Disabled-->Enabled Pending\n");
              BITMASK_SET(&m_flags, OMX_COMPONENT_OUTPUT_ENABLE_PENDING);
              // Skip the event notification
              bFlag = 0;
          }
      }
  }
  else if (cmd == OMX_CommandPortDisable)
  {
      DEBUG_PRINT_HIGH("\n send_command_proxy(): OMX_CommandPortDisable issued"
          "with param1: %d", param1);
      if(param1 == OMX_CORE_INPUT_PORT_INDEX || param1 == OMX_ALL)
      {
          m_inp_bEnabled = OMX_FALSE;
          if((m_state == OMX_StateLoaded || m_state == OMX_StateIdle)
              && release_input_done())
          {
             post_event(OMX_CommandPortDisable,OMX_CORE_INPUT_PORT_INDEX,
                        OMX_COMPONENT_GENERATE_EVENT);
          }
          else
          {
             BITMASK_SET(&m_flags, OMX_COMPONENT_INPUT_DISABLE_PENDING);
             if(m_state == OMX_StatePause ||m_state == OMX_StateExecuting)
             {
               if(!sem_posted)
               {
                 sem_posted = 1;
                 sem_post (&m_cmd_lock);
               }
               execute_omx_flush(OMX_CORE_INPUT_PORT_INDEX);
             }

             // Skip the event notification
             bFlag = 0;
          }
      }
      if(param1 == OMX_CORE_OUTPUT_PORT_INDEX || param1 == OMX_ALL)
      {
          m_out_bEnabled = OMX_FALSE;
          DEBUG_PRINT_LOW("\n Disable output Port command recieved");
          if((m_state == OMX_StateLoaded || m_state == OMX_StateIdle)
              && release_output_done())
          {
             post_event(OMX_CommandPortDisable,OMX_CORE_OUTPUT_PORT_INDEX,\
                        OMX_COMPONENT_GENERATE_EVENT);
          }
          else
         {
            BITMASK_SET(&m_flags, OMX_COMPONENT_OUTPUT_DISABLE_PENDING);
            if(m_state == OMX_StatePause ||m_state == OMX_StateExecuting)
            {
              if (!sem_posted)
              {
                sem_posted = 1;
                sem_post (&m_cmd_lock);
              }
                BITMASK_SET(&m_flags, OMX_COMPONENT_OUTPUT_FLUSH_IN_DISABLE_PENDING);
                execute_omx_flush(OMX_CORE_OUTPUT_PORT_INDEX);
            }
            // Skip the event notification
            bFlag = 0;

         }
      }
  }
  else
  {
    DEBUG_PRINT_ERROR("Error: Invalid Command other than StateSet (%d)\n",cmd);
    eRet = OMX_ErrorNotImplemented;
  }
  if(eRet == OMX_ErrorNone && bFlag)
  {
    post_event(cmd,eState,OMX_COMPONENT_GENERATE_EVENT);
  }
  if(!sem_posted)
  {
    sem_post(&m_cmd_lock);
  }

  return eRet;
}

/* ======================================================================
FUNCTION
  omx_vdec::ExecuteOmxFlush

DESCRIPTION
  Executes the OMX flush.

PARAMETERS
  flushtype - input flush(1)/output flush(0)/ both.

RETURN VALUE
  true/false

========================================================================== */
bool omx_vdec::execute_omx_flush(OMX_U32 flushType)
{
  struct vdec_ioctl_msg ioctl_msg = {NULL, NULL};
  enum vdec_bufferflush flush_dir;
  bool bRet = false;
  switch (flushType)
  {
    case OMX_CORE_INPUT_PORT_INDEX:
      input_flush_progress = true;
      flush_dir = VDEC_FLUSH_TYPE_INPUT;
    break;
    case OMX_CORE_OUTPUT_PORT_INDEX:
      output_flush_progress = true;
      flush_dir = VDEC_FLUSH_TYPE_OUTPUT;
    break;
    default:
      input_flush_progress = true;
      output_flush_progress = true;
      flush_dir = VDEC_FLUSH_TYPE_ALL;
  }
  ioctl_msg.in = &flush_dir;
  ioctl_msg.out = NULL;
  if (/*ioctl(drv_ctx.video_driver_fd, VDEC_IOCTL_CMD_FLUSH, &ioctl_msg) < */0)
  {
    DEBUG_PRINT_ERROR("\n Flush Port (%d) Failed ", (int)flush_dir);
    bRet = false;
  }
  return bRet;
}
/*=========================================================================
FUNCTION : execute_output_flush

DESCRIPTION
  Executes the OMX flush at OUTPUT PORT.

PARAMETERS
  None.

RETURN VALUE
  true/false
==========================================================================*/
bool omx_vdec::execute_output_flush()
{
  unsigned      p1 = 0; // Parameter - 1
  unsigned      p2 = 0; // Parameter - 2
  unsigned      ident = 0;
  bool bRet = true;

  /*Generate FBD for all Buffers in the FTBq*/
  pthread_mutex_lock(&m_lock);
  DEBUG_PRINT_LOW("\n Initiate Output Flush");
  while (m_ftb_q.m_size)
  {
    DEBUG_PRINT_LOW("\n Buffer queue size %d pending buf cnt %d",
                       m_ftb_q.m_size,pending_output_buffers);
    m_ftb_q.pop_entry(&p1,&p2,&ident);
    DEBUG_PRINT_LOW("\n ID(%x) P1(%x) P2(%x)", ident, p1, p2);
    if(ident == OMX_COMPONENT_GENERATE_FTB )
    {
      pending_output_buffers++;
      fill_buffer_done(&m_cmp,(OMX_BUFFERHEADERTYPE *)p2);
    }
    else if (ident == OMX_COMPONENT_GENERATE_FBD)
    {
      fill_buffer_done(&m_cmp,(OMX_BUFFERHEADERTYPE *)p1);
    }
  }
  pthread_mutex_unlock(&m_lock);
  output_flush_progress = false;

  if (arbitrary_bytes)
  {
    prev_ts = LLONG_MAX;
    rst_prev_ts = true;
  }
  DEBUG_PRINT_HIGH("\n OMX flush o/p Port complete PenBuf(%d)", pending_output_buffers);
  return bRet;
}
/*=========================================================================
FUNCTION : execute_input_flush

DESCRIPTION
  Executes the OMX flush at INPUT PORT.

PARAMETERS
  None.

RETURN VALUE
  true/false
==========================================================================*/
bool omx_vdec::execute_input_flush()
{
  unsigned       i =0;
  unsigned      p1 = 0; // Parameter - 1
  unsigned      p2 = 0; // Parameter - 2
  unsigned      ident = 0;
  bool bRet = true;

  /*Generate EBD for all Buffers in the ETBq*/
  DEBUG_PRINT_LOW("\n Initiate Input Flush \n");

  pthread_mutex_lock(&m_lock);
  DEBUG_PRINT_LOW("\n Check if the Queue is empty \n");
  while (m_etb_q.m_size)
  {
    m_etb_q.pop_entry(&p1,&p2,&ident);

    if (ident == OMX_COMPONENT_GENERATE_ETB_ARBITRARY)
    {
      DEBUG_PRINT_LOW("\n Flush Input Heap Buffer %p",(OMX_BUFFERHEADERTYPE *)p2);
      m_cb.EmptyBufferDone(&m_cmp ,m_app_data, (OMX_BUFFERHEADERTYPE *)p2);
    }
    else if(ident == OMX_COMPONENT_GENERATE_ETB)
    {
      pending_input_buffers++;
      DEBUG_PRINT_LOW("\n Flush Input OMX_COMPONENT_GENERATE_ETB %p, pending_input_buffers %d",
        (OMX_BUFFERHEADERTYPE *)p2, pending_input_buffers);
      empty_buffer_done(&m_cmp,(OMX_BUFFERHEADERTYPE *)p2);
    }
    else if (ident == OMX_COMPONENT_GENERATE_EBD)
    {
      DEBUG_PRINT_LOW("\n Flush Input OMX_COMPONENT_GENERATE_EBD %p",
        (OMX_BUFFERHEADERTYPE *)p1);
      empty_buffer_done(&m_cmp,(OMX_BUFFERHEADERTYPE *)p1);
    }
  }
  time_stamp_dts.flush_timestamp();
  /*Check if Heap Buffers are to be flushed*/
  if (arbitrary_bytes)
  {
    DEBUG_PRINT_LOW("\n Reset all the variables before flusing");
    h264_scratch.nFilledLen = 0;
    nal_count = 0;
    look_ahead_nal = false;
    frame_count = 0;
    h264_last_au_ts = LLONG_MAX;
    h264_last_au_flags = 0;
    memset(m_demux_offsets, 0, ( sizeof(OMX_U32) * 8192) );
    m_demux_entries = 0;
    DEBUG_PRINT_LOW("\n Initialize parser");
    if (m_frame_parser.mutils)
    {
      m_frame_parser.mutils->initialize_frame_checking_environment();
    }

    while (m_input_pending_q.m_size)
    {
      m_input_pending_q.pop_entry(&p1,&p2,&ident);
      m_cb.EmptyBufferDone(&m_cmp ,m_app_data, (OMX_BUFFERHEADERTYPE *)p1);
    }

    if (psource_frame)
    {
      m_cb.EmptyBufferDone(&m_cmp ,m_app_data,psource_frame);
      psource_frame = NULL;
    }

    if (pdest_frame)
    {
      pdest_frame->nFilledLen = 0;
      m_input_free_q.insert_entry((unsigned) pdest_frame,NULL,NULL);
      pdest_frame = NULL;
    }
    m_frame_parser.flush();
  }
  pthread_mutex_unlock(&m_lock);
  input_flush_progress = false;
  if (!arbitrary_bytes)
  {
    prev_ts = LLONG_MAX;
    rst_prev_ts = true;
  }
#ifdef _ANDROID_
  if (m_debug_timestamp)
  {
    m_timestamp_list.reset_ts_list();
  }
#endif
  DEBUG_PRINT_HIGH("\n OMX flush i/p Port complete PenBuf(%d)", pending_input_buffers);
  return bRet;
}


/* ======================================================================
FUNCTION
  omx_vdec::SendCommandEvent

DESCRIPTION
  Send the event to decoder pipe.  This is needed to generate the callbacks
  in decoder thread context.

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
bool omx_vdec::post_event(unsigned int p1,
                          unsigned int p2,
                          unsigned int id)
{
  bool bRet      =                      false;


  pthread_mutex_lock(&m_lock);

  if (id == OMX_COMPONENT_GENERATE_FTB ||
      id == OMX_COMPONENT_GENERATE_FBD)
  {
    m_ftb_q.insert_entry(p1,p2,id);
  }
  else if (id == OMX_COMPONENT_GENERATE_ETB ||
           id == OMX_COMPONENT_GENERATE_EBD ||
           id == OMX_COMPONENT_GENERATE_ETB_ARBITRARY)
  {
	  m_etb_q.insert_entry(p1,p2,id);
  }
  else
  {
    m_cmd_q.insert_entry(p1,p2,id);
  }

  bRet = true;
  DEBUG_PRINT_LOW("\n Value of this pointer in post_event %p",this);
  post_message(this, id);

  pthread_mutex_unlock(&m_lock);

  return bRet;
}

OMX_ERRORTYPE omx_vdec::get_supported_profile_level_for_1080p(OMX_VIDEO_PARAM_PROFILELEVELTYPE *profileLevelType)
{
  OMX_ERRORTYPE eRet = OMX_ErrorNone;
  if(!profileLevelType)
    return OMX_ErrorBadParameter;

  if(profileLevelType->nPortIndex == 0) {
    if (!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.avc",OMX_MAX_STRINGNAME_SIZE))
    {
      if (profileLevelType->nProfileIndex == 0)
      {
        profileLevelType->eProfile = OMX_VIDEO_AVCProfileBaseline;
        profileLevelType->eLevel   = OMX_VIDEO_AVCLevel4;

      }
      else if (profileLevelType->nProfileIndex == 1)
      {
        profileLevelType->eProfile = OMX_VIDEO_AVCProfileMain;
        profileLevelType->eLevel   = OMX_VIDEO_AVCLevel4;
      }
      else if(profileLevelType->nProfileIndex == 2)
      {
        profileLevelType->eProfile = OMX_VIDEO_AVCProfileHigh;
        profileLevelType->eLevel   = OMX_VIDEO_AVCLevel4;
      }
      else
      {
        DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamVideoProfileLevelQuerySupported nProfileIndex ret NoMore %d\n",
            profileLevelType->nProfileIndex);
        eRet = OMX_ErrorNoMore;
      }
    }
    else if((!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.h263",OMX_MAX_STRINGNAME_SIZE)))
    {
      if (profileLevelType->nProfileIndex == 0)
      {
        profileLevelType->eProfile = OMX_VIDEO_H263ProfileBaseline;
        profileLevelType->eLevel   = OMX_VIDEO_H263Level70;
      }
      else
      {
        DEBUG_PRINT_ERROR("get_parameter: OMX_IndexParamVideoProfileLevelQuerySupported nProfileIndex ret NoMore %d\n", profileLevelType->nProfileIndex);
        eRet = OMX_ErrorNoMore;
      }
    }
    else if (!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.mpeg4",OMX_MAX_STRINGNAME_SIZE))
    {
      if (profileLevelType->nProfileIndex == 0)
      {
        profileLevelType->eProfile = OMX_VIDEO_MPEG4ProfileSimple;
        profileLevelType->eLevel   = OMX_VIDEO_MPEG4Level5;
      }
      else if(profileLevelType->nProfileIndex == 1)
      {
        profileLevelType->eProfile = OMX_VIDEO_MPEG4ProfileAdvancedSimple;
        profileLevelType->eLevel   = OMX_VIDEO_MPEG4Level5;
      }
      else
      {
        DEBUG_PRINT_ERROR("get_parameter: OMX_IndexParamVideoProfileLevelQuerySupported nProfileIndex ret NoMore %d\n", profileLevelType->nProfileIndex);
        eRet = OMX_ErrorNoMore;
      }
    }
    else if (!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.mpeg2",OMX_MAX_STRINGNAME_SIZE))
    {
      if (profileLevelType->nProfileIndex == 0)
      {
        profileLevelType->eProfile = OMX_VIDEO_MPEG2ProfileSimple;
        profileLevelType->eLevel   = OMX_VIDEO_MPEG2LevelHL;
      }
      else if(profileLevelType->nProfileIndex == 1)
      {
        profileLevelType->eProfile = OMX_VIDEO_MPEG2ProfileMain;
        profileLevelType->eLevel   = OMX_VIDEO_MPEG2LevelHL;
      }
      else
      {
        DEBUG_PRINT_ERROR("get_parameter: OMX_IndexParamVideoProfileLevelQuerySupported nProfileIndex ret NoMore %d\n", profileLevelType->nProfileIndex);
        eRet = OMX_ErrorNoMore;
      }
    }
  }
  else
  {
    DEBUG_PRINT_ERROR("get_parameter: OMX_IndexParamVideoProfileLevelQuerySupported should be queries on Input port only %d\n", profileLevelType->nPortIndex);
    eRet = OMX_ErrorBadPortIndex;
  }
  return eRet;
}

/* ======================================================================
FUNCTION
  omx_vdec::GetParameter

DESCRIPTION
  OMX Get Parameter method implementation

PARAMETERS
  <TBD>.

RETURN VALUE
  Error None if successful.

========================================================================== */
OMX_ERRORTYPE  omx_vdec::get_parameter(OMX_IN OMX_HANDLETYPE     hComp,
                                           OMX_IN OMX_INDEXTYPE paramIndex,
                                           OMX_INOUT OMX_PTR     paramData)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;

    DEBUG_PRINT_LOW("get_parameter: \n");
    if(m_state == OMX_StateInvalid)
    {
        DEBUG_PRINT_ERROR("Get Param in Invalid State\n");
        return OMX_ErrorInvalidState;
    }
    if(paramData == NULL)
    {
        DEBUG_PRINT_LOW("Get Param in Invalid paramData \n");
        return OMX_ErrorBadParameter;
    }
  switch(paramIndex)
  {
    case OMX_IndexParamPortDefinition:
    {
      OMX_PARAM_PORTDEFINITIONTYPE *portDefn =
                            (OMX_PARAM_PORTDEFINITIONTYPE *) paramData;
      DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamPortDefinition\n");
      eRet = update_portdef(portDefn);
      if (eRet == OMX_ErrorNone)
          m_port_def = *portDefn;
      break;
    }
    case OMX_IndexParamVideoInit:
    {
      OMX_PORT_PARAM_TYPE *portParamType =
                              (OMX_PORT_PARAM_TYPE *) paramData;
      DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamVideoInit\n");

      portParamType->nVersion.nVersion = OMX_SPEC_VERSION;
      portParamType->nSize = sizeof(portParamType);
      portParamType->nPorts           = 2;
      portParamType->nStartPortNumber = 0;
      break;
    }
    case OMX_IndexParamVideoPortFormat:
    {
      OMX_VIDEO_PARAM_PORTFORMATTYPE *portFmt =
                     (OMX_VIDEO_PARAM_PORTFORMATTYPE *)paramData;
      DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamVideoPortFormat\n");

      portFmt->nVersion.nVersion = OMX_SPEC_VERSION;
      portFmt->nSize             = sizeof(portFmt);

      if (0 == portFmt->nPortIndex)
      {
        if (0 == portFmt->nIndex)
        {
              portFmt->eColorFormat =  OMX_COLOR_FormatUnused;
              portFmt->eCompressionFormat = eCompressionFormat;
        }
        else
        {
          DEBUG_PRINT_ERROR("get_parameter: OMX_IndexParamVideoPortFormat:"\
              " NoMore compression formats\n");
          eRet =  OMX_ErrorNoMore;
        }
      }
      else if (1 == portFmt->nPortIndex)
      {
        portFmt->eCompressionFormat =  OMX_VIDEO_CodingUnused;

        if(0 == portFmt->nIndex)
          portFmt->eColorFormat = (OMX_COLOR_FORMATTYPE)
            QOMX_COLOR_FormatYUV420PackedSemiPlanar64x32Tile2m8ka;
        else
        {
           DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamVideoPortFormat:"\
                  " NoMore Color formats\n");
           eRet =  OMX_ErrorNoMore;
        }
	portFmt->eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
      }
      else
      {
        DEBUG_PRINT_ERROR("get_parameter: Bad port index %d\n",
                          (int)portFmt->nPortIndex);
        eRet = OMX_ErrorBadPortIndex;
      }
      break;
    }
    /*Component should support this port definition*/
    case OMX_IndexParamAudioInit:
    {
        OMX_PORT_PARAM_TYPE *audioPortParamType =
                                              (OMX_PORT_PARAM_TYPE *) paramData;
        DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamAudioInit\n");
        audioPortParamType->nVersion.nVersion = OMX_SPEC_VERSION;
        audioPortParamType->nSize = sizeof(audioPortParamType);
        audioPortParamType->nPorts           = 0;
        audioPortParamType->nStartPortNumber = 0;
        break;
    }
    /*Component should support this port definition*/
    case OMX_IndexParamImageInit:
    {
        OMX_PORT_PARAM_TYPE *imagePortParamType =
                                              (OMX_PORT_PARAM_TYPE *) paramData;
        DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamImageInit\n");
        imagePortParamType->nVersion.nVersion = OMX_SPEC_VERSION;
        imagePortParamType->nSize = sizeof(imagePortParamType);
        imagePortParamType->nPorts           = 0;
        imagePortParamType->nStartPortNumber = 0;
        break;

    }
    /*Component should support this port definition*/
    case OMX_IndexParamOtherInit:
    {
        DEBUG_PRINT_ERROR("get_parameter: OMX_IndexParamOtherInit %08x\n",
                          paramIndex);
        eRet =OMX_ErrorUnsupportedIndex;
        break;
    }
    case OMX_IndexParamStandardComponentRole:
    {
        OMX_PARAM_COMPONENTROLETYPE *comp_role;
        comp_role = (OMX_PARAM_COMPONENTROLETYPE *) paramData;
        comp_role->nVersion.nVersion = OMX_SPEC_VERSION;
        comp_role->nSize = sizeof(*comp_role);

        DEBUG_PRINT_LOW("Getparameter: OMX_IndexParamStandardComponentRole %d\n",
                    paramIndex);
        strlcpy((char*)comp_role->cRole,(const char*)m_cRole,
                    OMX_MAX_STRINGNAME_SIZE);
        break;
    }
    /* Added for parameter test */
    case OMX_IndexParamPriorityMgmt:
        {

            OMX_PRIORITYMGMTTYPE *priorityMgmType =
                                             (OMX_PRIORITYMGMTTYPE *) paramData;
            DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamPriorityMgmt\n");
            priorityMgmType->nVersion.nVersion = OMX_SPEC_VERSION;
            priorityMgmType->nSize = sizeof(priorityMgmType);

            break;
        }
    /* Added for parameter test */
    case OMX_IndexParamCompBufferSupplier:
        {
            OMX_PARAM_BUFFERSUPPLIERTYPE *bufferSupplierType =
                                     (OMX_PARAM_BUFFERSUPPLIERTYPE*) paramData;
            DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamCompBufferSupplier\n");

            bufferSupplierType->nSize = sizeof(bufferSupplierType);
            bufferSupplierType->nVersion.nVersion = OMX_SPEC_VERSION;
            if(0 == bufferSupplierType->nPortIndex)
                bufferSupplierType->nPortIndex = OMX_BufferSupplyUnspecified;
            else if (1 == bufferSupplierType->nPortIndex)
                bufferSupplierType->nPortIndex = OMX_BufferSupplyUnspecified;
            else
                eRet = OMX_ErrorBadPortIndex;


            break;
        }
    case OMX_IndexParamVideoAvc:
        {
            DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamVideoAvc %08x\n",
                        paramIndex);
            break;
        }
    case OMX_IndexParamVideoH263:
        {
            DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamVideoH263 %08x\n",
                        paramIndex);
            break;
        }
    case OMX_IndexParamVideoMpeg4:
        {
            DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamVideoMpeg4 %08x\n",
                        paramIndex);
            break;
        }
    case OMX_IndexParamVideoMpeg2:
        {
          DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamVideoMpeg2 %08x\n",
              paramIndex);
          break;
        }
    case OMX_IndexParamVideoProfileLevelQuerySupported:
        {
          DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamVideoProfileLevelQuerySupported %08x\n", paramIndex);
          OMX_VIDEO_PARAM_PROFILELEVELTYPE *profileLevelType =
            (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)paramData;
    eRet = get_supported_profile_level_for_1080p(profileLevelType);
          break;
        }
#if defined (_ANDROID_HONEYCOMB_) || defined (_ANDROID_ICS_)
    case OMX_GoogleAndroidIndexGetAndroidNativeBufferUsage:
        {
            DEBUG_PRINT_LOW("get_parameter: OMX_GoogleAndroidIndexGetAndroidNativeBufferUsage\n");
            GetAndroidNativeBufferUsageParams* nativeBuffersUsage = (GetAndroidNativeBufferUsageParams *) paramData;
            if(nativeBuffersUsage->nPortIndex == OMX_CORE_OUTPUT_PORT_INDEX) {

                if(secure_mode) {
                        nativeBuffersUsage->nUsage = (GRALLOC_USAGE_PRIVATE_MM_HEAP | GRALLOC_USAGE_PROTECTED |
                                                      GRALLOC_USAGE_PRIVATE_UNCACHED);
                } else {
                        nativeBuffersUsage->nUsage = (GRALLOC_USAGE_PRIVATE_MM_HEAP | GRALLOC_USAGE_PRIVATE_UNCACHED);
                }
            } else {
                DEBUG_PRINT_HIGH("get_parameter: OMX_GoogleAndroidIndexGetAndroidNativeBufferUsage failed!\n");
                eRet = OMX_ErrorBadParameter;
            }
        }
        break;
#endif

    default:
    {
      DEBUG_PRINT_ERROR("get_parameter: unknown param %08x\n", paramIndex);
      eRet =OMX_ErrorUnsupportedIndex;
    }

  }

  DEBUG_PRINT_LOW("\n get_parameter returning WxH(%d x %d) SxSH(%d x %d)\n",
      drv_ctx.video_resolution.frame_width,
      drv_ctx.video_resolution.frame_height,
      drv_ctx.video_resolution.stride,
      drv_ctx.video_resolution.scan_lines);

  return eRet;
}

#if defined (_ANDROID_HONEYCOMB_) || defined (_ANDROID_ICS_)
OMX_ERRORTYPE omx_vdec::use_android_native_buffer(OMX_IN OMX_HANDLETYPE hComp, OMX_PTR data)
{
    DEBUG_PRINT_LOW("Inside use_android_native_buffer");
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    UseAndroidNativeBufferParams *params = (UseAndroidNativeBufferParams *)data;

    if((params == NULL) ||
      (params->nativeBuffer == NULL) ||
      (params->nativeBuffer->handle == NULL) ||
      !m_enable_android_native_buffers)
        return OMX_ErrorBadParameter;
    m_use_android_native_buffers = OMX_TRUE;
    sp<android_native_buffer_t> nBuf = params->nativeBuffer;
    private_handle_t *handle = (private_handle_t *)nBuf->handle;
    if(OMX_CORE_OUTPUT_PORT_INDEX == params->nPortIndex) {  //android native buffers can be used only on Output port
        OMX_U8 *buffer = NULL;
        if(!secure_mode) {
                buffer = (OMX_U8*)mmap(0, handle->size,
                    PROT_READ|PROT_WRITE, MAP_SHARED, handle->fd, 0);
                if(buffer == MAP_FAILED) {
                    DEBUG_PRINT_ERROR("Failed to mmap pmem with fd = %d, size = %d", handle->fd, handle->size);
                    return OMX_ErrorInsufficientResources;
            }
        }
        eRet = use_buffer(hComp,params->bufferHeader,params->nPortIndex,data,handle->size,buffer);
    } else {
        eRet = OMX_ErrorBadParameter;
    }
    return eRet;
}
#endif
/* ======================================================================
FUNCTION
  omx_vdec::Setparameter

DESCRIPTION
  OMX Set Parameter method implementation.

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if successful.

========================================================================== */
OMX_ERRORTYPE  omx_vdec::set_parameter(OMX_IN OMX_HANDLETYPE     hComp,
                                           OMX_IN OMX_INDEXTYPE paramIndex,
                                           OMX_IN OMX_PTR        paramData)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    struct vdec_ioctl_msg ioctl_msg = {NULL,NULL};

    if(m_state == OMX_StateInvalid)
    {
        DEBUG_PRINT_ERROR("Set Param in Invalid State\n");
        return OMX_ErrorInvalidState;
    }
    if(paramData == NULL)
    {
         DEBUG_PRINT_ERROR("Get Param in Invalid paramData \n");
         return OMX_ErrorBadParameter;
    }
    if((m_state != OMX_StateLoaded) &&
          BITMASK_ABSENT(&m_flags,OMX_COMPONENT_OUTPUT_ENABLE_PENDING) &&
          (m_out_bEnabled == OMX_TRUE) &&
          BITMASK_ABSENT(&m_flags, OMX_COMPONENT_INPUT_ENABLE_PENDING) &&
          (m_inp_bEnabled == OMX_TRUE)) {
        DEBUG_PRINT_ERROR("Set Param in Invalid State \n");
        return OMX_ErrorIncorrectStateOperation;
    }
  switch(paramIndex)
  {
    case OMX_IndexParamPortDefinition:
    {
      OMX_PARAM_PORTDEFINITIONTYPE *portDefn;
      portDefn = (OMX_PARAM_PORTDEFINITIONTYPE *) paramData;
      //TODO: Check if any allocate buffer/use buffer/useNativeBuffer has
      //been called.
      DEBUG_PRINT_LOW("set_parameter: OMX_IndexParamPortDefinition H= %d, W = %d\n",
             (int)portDefn->format.video.nFrameHeight,
             (int)portDefn->format.video.nFrameWidth);
      if(OMX_DirOutput == portDefn->eDir)
      {
          DEBUG_PRINT_LOW("set_parameter: OMX_IndexParamPortDefinition OP port\n");
          m_display_id = portDefn->format.video.pNativeWindow;
          if ( portDefn->nBufferCountActual >= drv_ctx.op_buf.mincount &&
               portDefn->nBufferSize >=  drv_ctx.op_buf.buffer_size )
            {
              drv_ctx.op_buf.actualcount = portDefn->nBufferCountActual;
              drv_ctx.op_buf.buffer_size = portDefn->nBufferSize;
              eRet = set_buffer_req(&drv_ctx.op_buf);
              if (eRet == OMX_ErrorNone)
                  m_port_def = *portDefn;
          }
          else
          {
              DEBUG_PRINT_ERROR("ERROR: OP Requirements(#%d: %u) Requested(#%d: %u)\n",
                drv_ctx.op_buf.mincount, drv_ctx.op_buf.buffer_size,
                portDefn->nBufferCountActual, portDefn->nBufferSize);
              eRet = OMX_ErrorBadParameter;
          }
      }
      else if(OMX_DirInput == portDefn->eDir)
      {
        if((portDefn->format.video.xFramerate >> 16) > 0 &&
           (portDefn->format.video.xFramerate >> 16) <= MAX_SUPPORTED_FPS)
        {
            // Frame rate only should be set if this is a "known value" or to
            // activate ts prediction logic (arbitrary mode only) sending input
            // timestamps with max value (LLONG_MAX).
            DEBUG_PRINT_HIGH("set_parameter: frame rate set by omx client : %d",
                             portDefn->format.video.xFramerate >> 16);
            Q16ToFraction(portDefn->format.video.xFramerate, drv_ctx.frame_rate.fps_numerator,
                          drv_ctx.frame_rate.fps_denominator);
            if(!drv_ctx.frame_rate.fps_numerator)
            {
              DEBUG_PRINT_ERROR("Numerator is zero setting to 30");
              drv_ctx.frame_rate.fps_numerator = 30;
            }
            if(drv_ctx.frame_rate.fps_denominator)
              drv_ctx.frame_rate.fps_numerator = (int)
                  drv_ctx.frame_rate.fps_numerator / drv_ctx.frame_rate.fps_denominator;
              drv_ctx.frame_rate.fps_denominator = 1;
            frm_int = drv_ctx.frame_rate.fps_denominator * 1e6 /
                      drv_ctx.frame_rate.fps_numerator;
            ioctl_msg.in = &drv_ctx.frame_rate;
            if (/*ioctl (drv_ctx.video_driver_fd, VDEC_IOCTL_SET_FRAME_RATE,
                       (void*)&ioctl_msg) < */0)
            {
              DEBUG_PRINT_ERROR("Setting frame rate to driver failed");
            }
            DEBUG_PRINT_LOW("set_parameter: frm_int(%u) fps(%.2f)",
                             frm_int, drv_ctx.frame_rate.fps_numerator /
                             (float)drv_ctx.frame_rate.fps_denominator);
        }
         DEBUG_PRINT_LOW("set_parameter: OMX_IndexParamPortDefinition IP port\n");
         if(drv_ctx.video_resolution.frame_height !=
               portDefn->format.video.nFrameHeight ||
             drv_ctx.video_resolution.frame_width  !=
               portDefn->format.video.nFrameWidth)
         {
             DEBUG_PRINT_LOW("\n SetParam IP: WxH(%d x %d)\n",
                           portDefn->format.video.nFrameWidth,
                           portDefn->format.video.nFrameHeight);
             if (portDefn->format.video.nFrameHeight != 0x0 &&
                 portDefn->format.video.nFrameWidth != 0x0)
             {
               drv_ctx.video_resolution.frame_height =
                 drv_ctx.video_resolution.scan_lines =
                 portDefn->format.video.nFrameHeight;
               drv_ctx.video_resolution.frame_width =
                 drv_ctx.video_resolution.stride =
                 portDefn->format.video.nFrameWidth;
               ioctl_msg.in = &drv_ctx.video_resolution;
               ioctl_msg.out = NULL;
               if (/*ioctl (drv_ctx.video_driver_fd, VDEC_IOCTL_SET_PICRES,
                            (void*)&ioctl_msg) < */0)
               {
                   DEBUG_PRINT_ERROR("\n Set Resolution failed");
                   eRet = OMX_ErrorUnsupportedSetting;
               }
               else
                   eRet = get_buffer_req(&drv_ctx.op_buf);
             }
         }
         else if (portDefn->nBufferCountActual >= drv_ctx.ip_buf.mincount
                  && portDefn->nBufferSize == drv_ctx.ip_buf.buffer_size)
         {
             drv_ctx.ip_buf.actualcount = portDefn->nBufferCountActual;
             drv_ctx.ip_buf.buffer_size = portDefn->nBufferSize;
             eRet = set_buffer_req(&drv_ctx.ip_buf);
         }
         else
         {
             DEBUG_PRINT_ERROR("ERROR: IP Requirements(#%d: %u) Requested(#%d: %u)\n",
               drv_ctx.ip_buf.mincount, drv_ctx.ip_buf.buffer_size,
               portDefn->nBufferCountActual, portDefn->nBufferSize);
             eRet = OMX_ErrorBadParameter;
         }
      }
      else if (portDefn->eDir ==  OMX_DirMax)
      {
          DEBUG_PRINT_ERROR(" Set_parameter: Bad Port idx %d",
                      (int)portDefn->nPortIndex);
          eRet = OMX_ErrorBadPortIndex;
      }
    }
    break;
    case OMX_IndexParamVideoPortFormat:
    {
      OMX_VIDEO_PARAM_PORTFORMATTYPE *portFmt =
                     (OMX_VIDEO_PARAM_PORTFORMATTYPE *)paramData;
      int ret=0;
      struct v4l2_format fmt;
      DEBUG_PRINT_LOW("set_parameter: OMX_IndexParamVideoPortFormat %d\n",
              portFmt->eColorFormat);

      if(1 == portFmt->nPortIndex)
      {
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	fmt.fmt.pix_mp.height = drv_ctx.video_resolution.frame_height;
	fmt.fmt.pix_mp.width = drv_ctx.video_resolution.frame_width;
	fmt.fmt.pix_mp.pixelformat = capture_capability;
	enum vdec_output_fromat op_format;
	if(portFmt->eColorFormat == OMX_COLOR_FormatYUV420SemiPlanar)
	  op_format = VDEC_YUV_FORMAT_NV12;
	else if(portFmt->eColorFormat ==
		QOMX_COLOR_FormatYUV420PackedSemiPlanar64x32Tile2m8ka)
	  op_format = VDEC_YUV_FORMAT_TILE_4x2;
         else
	   eRet = OMX_ErrorBadParameter;

         if(eRet == OMX_ErrorNone)
         {
	   drv_ctx.output_format = op_format;
	   ret = ioctl(drv_ctx.video_driver_fd, VIDIOC_S_FMT, &fmt);
	   if(ret)
           {
             DEBUG_PRINT_ERROR("\n Set output format failed");
             eRet = OMX_ErrorUnsupportedSetting;
			/*TODO: How to handle this case */
           }
           else
	     {
	       eRet = get_buffer_req(&drv_ctx.op_buf);
	     }
	 }
      }
    }
    break;

    case OMX_QcomIndexPortDefn:
    {
        OMX_QCOM_PARAM_PORTDEFINITIONTYPE *portFmt =
            (OMX_QCOM_PARAM_PORTDEFINITIONTYPE *) paramData;
        DEBUG_PRINT_LOW("set_parameter: OMX_IndexQcomParamPortDefinitionType %d\n",
            portFmt->nFramePackingFormat);

        /* Input port */
        if (portFmt->nPortIndex == 0)
        {
            if (portFmt->nFramePackingFormat == OMX_QCOM_FramePacking_Arbitrary)
            {
              if(secure_mode) {
                arbitrary_bytes = false;
                DEBUG_PRINT_ERROR("setparameter: cannot set to arbitary bytes mode in secure session");
                eRet = OMX_ErrorUnsupportedSetting;
              } else {
               arbitrary_bytes = true;
              }
            }
            else if (portFmt->nFramePackingFormat ==
                OMX_QCOM_FramePacking_OnlyOneCompleteFrame)
            {
               arbitrary_bytes = false;
            }
            else
            {
                DEBUG_PRINT_ERROR("Setparameter: unknown FramePacking format %d\n",
                    portFmt->nFramePackingFormat);
                eRet = OMX_ErrorUnsupportedSetting;
            }
        }
        else if (portFmt->nPortIndex == OMX_CORE_OUTPUT_PORT_INDEX)
        {
          DEBUG_PRINT_HIGH("set_parameter: OMX_IndexQcomParamPortDefinitionType OP Port\n");
          if( (portFmt->nMemRegion > OMX_QCOM_MemRegionInvalid &&
               portFmt->nMemRegion < OMX_QCOM_MemRegionMax) &&
              portFmt->nCacheAttr == OMX_QCOM_CacheAttrNone)
          {
            m_out_mem_region_smi = OMX_TRUE;
            if ((m_out_mem_region_smi && m_out_pvt_entry_pmem))
            {
              DEBUG_PRINT_HIGH("set_parameter: OMX_IndexQcomParamPortDefinitionType OP Port: out pmem set\n");
              m_use_output_pmem = OMX_TRUE;
            }
          }
        }
    }
    break;

     case OMX_IndexParamStandardComponentRole:
     {
          OMX_PARAM_COMPONENTROLETYPE *comp_role;
          comp_role = (OMX_PARAM_COMPONENTROLETYPE *) paramData;
          DEBUG_PRINT_LOW("set_parameter: OMX_IndexParamStandardComponentRole %s\n",
                       comp_role->cRole);

          if((m_state == OMX_StateLoaded)&&
              !BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING))
          {
           DEBUG_PRINT_LOW("Set Parameter called in valid state");
          }
          else
          {
             DEBUG_PRINT_ERROR("Set Parameter called in Invalid State\n");
             return OMX_ErrorIncorrectStateOperation;
          }

          if(!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.avc",OMX_MAX_STRINGNAME_SIZE))
          {
              if(!strncmp((char*)comp_role->cRole,"video_decoder.avc",OMX_MAX_STRINGNAME_SIZE))
              {
                  strlcpy((char*)m_cRole,"video_decoder.avc",OMX_MAX_STRINGNAME_SIZE);
              }
              else
              {
                  DEBUG_PRINT_ERROR("Setparameter: unknown Index %s\n", comp_role->cRole);
                  eRet =OMX_ErrorUnsupportedSetting;
              }
          }
          else if(!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.mpeg4",OMX_MAX_STRINGNAME_SIZE))
          {
              if(!strncmp((const char*)comp_role->cRole,"video_decoder.mpeg4",OMX_MAX_STRINGNAME_SIZE))
              {
                  strlcpy((char*)m_cRole,"video_decoder.mpeg4",OMX_MAX_STRINGNAME_SIZE);
              }
              else
              {
                  DEBUG_PRINT_ERROR("Setparameter: unknown Index %s\n", comp_role->cRole);
                  eRet = OMX_ErrorUnsupportedSetting;
              }
          }
          else if(!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.h263",OMX_MAX_STRINGNAME_SIZE))
          {
              if(!strncmp((const char*)comp_role->cRole,"video_decoder.h263",OMX_MAX_STRINGNAME_SIZE))
              {
                  strlcpy((char*)m_cRole,"video_decoder.h263",OMX_MAX_STRINGNAME_SIZE);
              }
              else
              {
                  DEBUG_PRINT_ERROR("Setparameter: unknown Index %s\n", comp_role->cRole);
                  eRet =OMX_ErrorUnsupportedSetting;
              }
          }
          else if(!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.mpeg2",OMX_MAX_STRINGNAME_SIZE))
          {
            if(!strncmp((const char*)comp_role->cRole,"video_decoder.mpeg2",OMX_MAX_STRINGNAME_SIZE))
            {
              strlcpy((char*)m_cRole,"video_decoder.mpeg2",OMX_MAX_STRINGNAME_SIZE);
            }
            else
            {
              DEBUG_PRINT_ERROR("Setparameter: unknown Index %s\n", comp_role->cRole);
              eRet = OMX_ErrorUnsupportedSetting;
            }
          }
          else if((!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.divx",OMX_MAX_STRINGNAME_SIZE)) ||
                  (!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.divx311",OMX_MAX_STRINGNAME_SIZE))
                  )
          {
              if(!strncmp((const char*)comp_role->cRole,"video_decoder.divx",OMX_MAX_STRINGNAME_SIZE))
              {
                  strlcpy((char*)m_cRole,"video_decoder.divx",OMX_MAX_STRINGNAME_SIZE);
              }
              else
              {
                  DEBUG_PRINT_ERROR("Setparameter: unknown Index %s\n", comp_role->cRole);
                  eRet =OMX_ErrorUnsupportedSetting;
              }
          }
          else if ( (!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.vc1",OMX_MAX_STRINGNAME_SIZE)) ||
                    (!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.wmv",OMX_MAX_STRINGNAME_SIZE))
                    )
          {
              if(!strncmp((const char*)comp_role->cRole,"video_decoder.vc1",OMX_MAX_STRINGNAME_SIZE))
              {
                  strlcpy((char*)m_cRole,"video_decoder.vc1",OMX_MAX_STRINGNAME_SIZE);
              }
              else
              {
                  DEBUG_PRINT_ERROR("Setparameter: unknown Index %s\n", comp_role->cRole);
                  eRet =OMX_ErrorUnsupportedSetting;
              }
          }
          else
          {
               DEBUG_PRINT_ERROR("Setparameter: unknown param %s\n", drv_ctx.kind);
               eRet = OMX_ErrorInvalidComponentName;
          }
          break;
     }

    case OMX_IndexParamPriorityMgmt:
        {
            if(m_state != OMX_StateLoaded)
            {
               DEBUG_PRINT_ERROR("Set Parameter called in Invalid State\n");
               return OMX_ErrorIncorrectStateOperation;
            }
            OMX_PRIORITYMGMTTYPE *priorityMgmtype = (OMX_PRIORITYMGMTTYPE*) paramData;
            DEBUG_PRINT_LOW("set_parameter: OMX_IndexParamPriorityMgmt %d\n",
              priorityMgmtype->nGroupID);

            DEBUG_PRINT_LOW("set_parameter: priorityMgmtype %d\n",
             priorityMgmtype->nGroupPriority);

            m_priority_mgm.nGroupID = priorityMgmtype->nGroupID;
            m_priority_mgm.nGroupPriority = priorityMgmtype->nGroupPriority;

            break;
        }

      case OMX_IndexParamCompBufferSupplier:
      {
          OMX_PARAM_BUFFERSUPPLIERTYPE *bufferSupplierType = (OMX_PARAM_BUFFERSUPPLIERTYPE*) paramData;
            DEBUG_PRINT_LOW("set_parameter: OMX_IndexParamCompBufferSupplier %d\n",
                bufferSupplierType->eBufferSupplier);
             if(bufferSupplierType->nPortIndex == 0 || bufferSupplierType->nPortIndex ==1)
                m_buffer_supplier.eBufferSupplier = bufferSupplierType->eBufferSupplier;

             else

             eRet = OMX_ErrorBadPortIndex;

          break;

      }
      case OMX_IndexParamVideoAvc:
          {
              DEBUG_PRINT_LOW("set_parameter: OMX_IndexParamVideoAvc %d\n",
                    paramIndex);
              break;
          }
      case OMX_IndexParamVideoH263:
          {
              DEBUG_PRINT_LOW("set_parameter: OMX_IndexParamVideoH263 %d\n",
                    paramIndex);
              break;
          }
      case OMX_IndexParamVideoMpeg4:
          {
              DEBUG_PRINT_LOW("set_parameter: OMX_IndexParamVideoMpeg4 %d\n",
                    paramIndex);
              break;
          }
      case OMX_IndexParamVideoMpeg2:
          {
              DEBUG_PRINT_LOW("set_parameter: OMX_IndexParamVideoMpeg2 %d\n",
                    paramIndex);
              break;
          }
       case OMX_QcomIndexParamVideoDecoderPictureOrder:
          {
              QOMX_VIDEO_DECODER_PICTURE_ORDER *pictureOrder =
                  (QOMX_VIDEO_DECODER_PICTURE_ORDER *)paramData;
              enum vdec_output_order pic_order = VDEC_ORDER_DISPLAY;
              DEBUG_PRINT_HIGH("set_parameter: OMX_QcomIndexParamVideoDecoderPictureOrder %d\n",
                    pictureOrder->eOutputPictureOrder);
              if (pictureOrder->eOutputPictureOrder == QOMX_VIDEO_DISPLAY_ORDER)
                  pic_order = VDEC_ORDER_DISPLAY;
              else if (pictureOrder->eOutputPictureOrder == QOMX_VIDEO_DECODE_ORDER){
                  pic_order = VDEC_ORDER_DECODE;
                  time_stamp_dts.set_timestamp_reorder_mode(false);
              }
              else
                  eRet = OMX_ErrorBadParameter;

              if (eRet == OMX_ErrorNone && pic_order != drv_ctx.picture_order)
              {
                  drv_ctx.picture_order = pic_order;
		  // ioctl_msg.in = &drv_ctx.picture_order;
                  //ioctl_msg.out = NULL;
                  if (/*ioctl(drv_ctx.video_driver_fd, VDEC_IOCTL_SET_PICTURE_ORDER,
                      (void*)&ioctl_msg) < */0)
                  {
                      DEBUG_PRINT_ERROR("\n Set picture order failed");
                      eRet = OMX_ErrorUnsupportedSetting;
                  }
              }
              break;
          }
    case OMX_QcomIndexParamConcealMBMapExtraData:
      if(!secure_mode)
          eRet = enable_extradata(VDEC_EXTRADATA_MB_ERROR_MAP,
                                  ((QOMX_ENABLETYPE *)paramData)->bEnable);
      else {
          DEBUG_PRINT_ERROR("\n secure mode setting not supported");
          eRet = OMX_ErrorUnsupportedSetting;
      }
      break;
    case OMX_QcomIndexParamFrameInfoExtraData:
      {
        if(!secure_mode)
            eRet = enable_extradata(OMX_FRAMEINFO_EXTRADATA,
                                ((QOMX_ENABLETYPE *)paramData)->bEnable);
        else {
            DEBUG_PRINT_ERROR("\n secure mode setting not supported");
            eRet = OMX_ErrorUnsupportedSetting;
        }
       break;
      }
    case OMX_QcomIndexParamInterlaceExtraData:
      if(!secure_mode)
          eRet = enable_extradata(OMX_INTERLACE_EXTRADATA,
                              ((QOMX_ENABLETYPE *)paramData)->bEnable);
      else {
          DEBUG_PRINT_ERROR("\n secure mode setting not supported");
          eRet = OMX_ErrorUnsupportedSetting;
      }
      break;
    case OMX_QcomIndexParamH264TimeInfo:
      if(!secure_mode)
          eRet = enable_extradata(OMX_TIMEINFO_EXTRADATA,
                              ((QOMX_ENABLETYPE *)paramData)->bEnable);
      else {
          DEBUG_PRINT_ERROR("\n secure mode setting not supported");
          eRet = OMX_ErrorUnsupportedSetting;
      }
      break;
    case OMX_QcomIndexParamVideoDivx:
      {
        QOMX_VIDEO_PARAM_DIVXTYPE* divXType = (QOMX_VIDEO_PARAM_DIVXTYPE *) paramData;

#if 0
         createDivxDrmContext( divXType->pDrmHandle );
#endif
      }
      break;
    case OMX_QcomIndexPlatformPvt:
      {
        DEBUG_PRINT_HIGH("set_parameter: OMX_QcomIndexPlatformPvt OP Port\n");
        OMX_QCOM_PLATFORMPRIVATE_EXTN* entryType = (OMX_QCOM_PLATFORMPRIVATE_EXTN *) paramData;
        if (entryType->type != OMX_QCOM_PLATFORM_PRIVATE_PMEM)
        {
          DEBUG_PRINT_HIGH("set_parameter: Platform Private entry type (%d) not supported.", entryType->type);
          eRet = OMX_ErrorUnsupportedSetting;
        }
        else
        {
          m_out_pvt_entry_pmem = OMX_TRUE;
          if ((m_out_mem_region_smi && m_out_pvt_entry_pmem))
          {
            DEBUG_PRINT_HIGH("set_parameter: OMX_QcomIndexPlatformPvt OP Port: out pmem set\n");
            m_use_output_pmem = OMX_TRUE;
          }
        }

      }
      break;
    case OMX_QcomIndexParamVideoSyncFrameDecodingMode:
      {
          DEBUG_PRINT_HIGH("set_parameter: OMX_QcomIndexParamVideoSyncFrameDecodingMode");
          DEBUG_PRINT_HIGH("set idr only decoding for thumbnail mode");
          drv_ctx.idr_only_decoding = 1;
          int rc; //= ioctl(drv_ctx.video_driver_fd,
               //       VDEC_IOCTL_SET_IDR_ONLY_DECODING);
          if(rc < 0) {
              DEBUG_PRINT_ERROR("Failed to set IDR only decoding on driver.");
              eRet = OMX_ErrorHardware;
          }
      }
      break;

    case OMX_QcomIndexParamIndexExtraDataType:
      {
        if(!secure_mode) {
            QOMX_INDEXEXTRADATATYPE *extradataIndexType = (QOMX_INDEXEXTRADATATYPE *) paramData;
            if ((extradataIndexType->nIndex == OMX_IndexParamPortDefinition) &&
                   (extradataIndexType->bEnabled == OMX_TRUE) &&
                   (extradataIndexType->nPortIndex == 1))
            {
              DEBUG_PRINT_HIGH("set_parameter:  OMX_QcomIndexParamIndexExtraDataType SmoothStreaming\n");
              eRet = enable_extradata(OMX_PORTDEF_EXTRADATA, extradataIndexType->bEnabled);
              // Set smooth streaming parameter
              int rc;// = ioctl(drv_ctx.video_driver_fd,
                   //         VDEC_IOCTL_SET_CONT_ON_RECONFIG);
              if(rc < 0) {
                  DEBUG_PRINT_ERROR("Failed to enable Smooth Streaming on driver.");
                  eRet = OMX_ErrorHardware;
              }
            }
         }
       }
      break;

#if defined (_ANDROID_HONEYCOMB_) || defined (_ANDROID_ICS_)
      /* Need to allow following two set_parameters even in Idle
       * state. This is ANDROID architecture which is not in sync
       * with openmax standard. */
    case OMX_GoogleAndroidIndexEnableAndroidNativeBuffers:
      {
          EnableAndroidNativeBuffersParams* enableNativeBuffers = (EnableAndroidNativeBuffersParams *) paramData;
          if(enableNativeBuffers) {
              m_enable_android_native_buffers = enableNativeBuffers->enable;
          }
      }
      break;
    case OMX_GoogleAndroidIndexUseAndroidNativeBuffer:
      {
          eRet = use_android_native_buffer(hComp, paramData);
      }
      break;
#endif
    case OMX_QcomIndexParamEnableTimeStampReorder:
      {
        QOMX_INDEXTIMESTAMPREORDER *reorder = (QOMX_INDEXTIMESTAMPREORDER *)paramData;
        if (drv_ctx.picture_order == QOMX_VIDEO_DISPLAY_ORDER) {
          if (reorder->bEnable == OMX_TRUE) {
              frm_int =0;
              time_stamp_dts.set_timestamp_reorder_mode(true);
          }
          else
            time_stamp_dts.set_timestamp_reorder_mode(false);
        } else {
          time_stamp_dts.set_timestamp_reorder_mode(false);
          if (reorder->bEnable == OMX_TRUE)
          {
            eRet = OMX_ErrorUnsupportedSetting;
          }
        }
      }
      break;
    default:
    {
      DEBUG_PRINT_ERROR("Setparameter: unknown param %d\n", paramIndex);
      eRet = OMX_ErrorUnsupportedIndex;
    }
  }
  return eRet;
}

/* ======================================================================
FUNCTION
  omx_vdec::GetConfig

DESCRIPTION
  OMX Get Config Method implementation.

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if successful.

========================================================================== */
OMX_ERRORTYPE  omx_vdec::get_config(OMX_IN OMX_HANDLETYPE      hComp,
                                        OMX_IN OMX_INDEXTYPE configIndex,
                                        OMX_INOUT OMX_PTR     configData)
{
  OMX_ERRORTYPE eRet = OMX_ErrorNone;

  if (m_state == OMX_StateInvalid)
  {
     DEBUG_PRINT_ERROR("Get Config in Invalid State\n");
     return OMX_ErrorInvalidState;
  }

  switch (configIndex)
  {
    case OMX_QcomIndexConfigInterlaced:
    {
      OMX_QCOM_CONFIG_INTERLACETYPE *configFmt =
                                   (OMX_QCOM_CONFIG_INTERLACETYPE *) configData;
      if (configFmt->nPortIndex == 1)
      {
        if (configFmt->nIndex == 0)
        {
          configFmt->eInterlaceType = OMX_QCOM_InterlaceFrameProgressive;
        }
        else if (configFmt->nIndex == 1)
        {
          configFmt->eInterlaceType =
                                  OMX_QCOM_InterlaceInterleaveFrameTopFieldFirst;
        }
        else if (configFmt->nIndex == 2)
        {
          configFmt->eInterlaceType =
          OMX_QCOM_InterlaceInterleaveFrameBottomFieldFirst;
        }
        else
        {
          DEBUG_PRINT_ERROR("get_config: OMX_QcomIndexConfigInterlaced:"
                            " NoMore Interlaced formats\n");
          eRet = OMX_ErrorNoMore;
        }

      }
      else
      {
        DEBUG_PRINT_ERROR("get_config: Bad port index %d queried on only o/p port\n",
        (int)configFmt->nPortIndex);
        eRet = OMX_ErrorBadPortIndex;
      }
    break;
    }
    case OMX_QcomIndexQueryNumberOfVideoDecInstance:
    {
        struct vdec_ioctl_msg ioctl_msg = {NULL,NULL};
        QOMX_VIDEO_QUERY_DECODER_INSTANCES *decoderinstances =
          (QOMX_VIDEO_QUERY_DECODER_INSTANCES*)configData;
        //ioctl_msg.out = (void*)&decoderinstances->nNumOfInstances;
        //(void)(ioctl(drv_ctx.video_driver_fd,
               //VDEC_IOCTL_GET_NUMBER_INSTANCES,&ioctl_msg));

	decoderinstances->nNumOfInstances = 16;
	/*TODO: How to handle this case */
    break;
    }
  case OMX_QcomIndexConfigVideoFramePackingArrangement:
    {
      if (drv_ctx.decoder_format == VDEC_CODECTYPE_H264)
      {
        OMX_QCOM_FRAME_PACK_ARRANGEMENT *configFmt =
          (OMX_QCOM_FRAME_PACK_ARRANGEMENT *) configData;
        h264_parser->get_frame_pack_data(configFmt);
      }
      else
      {
        DEBUG_PRINT_ERROR("get_config: Framepack data not supported for non H264 codecs");
      }
      break;
    }
    default:
    {
      DEBUG_PRINT_ERROR("get_config: unknown param %d\n",configIndex);
      eRet = OMX_ErrorBadParameter;
    }

  }

  return eRet;
}

/* ======================================================================
FUNCTION
  omx_vdec::SetConfig

DESCRIPTION
  OMX Set Config method implementation

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if successful.
========================================================================== */
OMX_ERRORTYPE  omx_vdec::set_config(OMX_IN OMX_HANDLETYPE      hComp,
                                        OMX_IN OMX_INDEXTYPE configIndex,
                                        OMX_IN OMX_PTR        configData)
{
  if(m_state == OMX_StateInvalid)
  {
      DEBUG_PRINT_ERROR("Get Config in Invalid State\n");
      return OMX_ErrorInvalidState;
  }

  OMX_ERRORTYPE ret = OMX_ErrorNone;
  OMX_VIDEO_CONFIG_NALSIZE *pNal;

  DEBUG_PRINT_LOW("\n Set Config Called");

  if (m_state == OMX_StateExecuting)
  {
     DEBUG_PRINT_ERROR("set_config:Ignore in Exe state\n");
     return ret;
  }

  if (configIndex == OMX_IndexVendorVideoExtraData)
  {
    OMX_VENDOR_EXTRADATATYPE *config = (OMX_VENDOR_EXTRADATATYPE *) configData;
    DEBUG_PRINT_LOW("\n Index OMX_IndexVendorVideoExtraData called");
    if (!strcmp(drv_ctx.kind, "OMX.qcom.video.decoder.avc"))
    {
      DEBUG_PRINT_LOW("\n Index OMX_IndexVendorVideoExtraData AVC");
      OMX_U32 extra_size;
      // Parsing done here for the AVC atom is definitely not generic
      // Currently this piece of code is working, but certainly
      // not tested with all .mp4 files.
      // Incase of failure, we might need to revisit this
      // for a generic piece of code.

      // Retrieve size of NAL length field
      // byte #4 contains the size of NAL lenght field
      nal_length = (config->pData[4] & 0x03) + 1;

      extra_size = 0;
      if (nal_length > 2)
      {
        /* Presently we assume that only one SPS and one PPS in AvC1 Atom */
        extra_size = (nal_length - 2) * 2;
      }

      // SPS starts from byte #6
      OMX_U8 *pSrcBuf = (OMX_U8 *) (&config->pData[6]);
      OMX_U8 *pDestBuf;
      m_vendor_config.nPortIndex = config->nPortIndex;

      // minus 6 --> SPS starts from byte #6
      // minus 1 --> picture param set byte to be ignored from avcatom
      m_vendor_config.nDataSize = config->nDataSize - 6 - 1 + extra_size;
      m_vendor_config.pData = (OMX_U8 *) malloc(m_vendor_config.nDataSize);
      OMX_U32 len;
      OMX_U8 index = 0;
      // case where SPS+PPS is sent as part of set_config
      pDestBuf = m_vendor_config.pData;

      DEBUG_PRINT_LOW("Rxd SPS+PPS nPortIndex[%d] len[%d] data[0x%x]\n",
           m_vendor_config.nPortIndex,
           m_vendor_config.nDataSize,
           m_vendor_config.pData);
      while (index < 2)
      {
        uint8 *psize;
        len = *pSrcBuf;
        len = len << 8;
        len |= *(pSrcBuf + 1);
        psize = (uint8 *) & len;
        memcpy(pDestBuf + nal_length, pSrcBuf + 2,len);
        for (int i = 0; i < nal_length; i++)
        {
          pDestBuf[i] = psize[nal_length - 1 - i];
        }
        //memcpy(pDestBuf,pSrcBuf,(len+2));
        pDestBuf += len + nal_length;
        pSrcBuf += len + 2;
        index++;
        pSrcBuf++;   // skip picture param set
        len = 0;
      }
    }
    else if (!strcmp(drv_ctx.kind, "OMX.qcom.video.decoder.mpeg4") ||
             !strcmp(drv_ctx.kind, "OMX.qcom.video.decoder.mpeg2"))
    {
      m_vendor_config.nPortIndex = config->nPortIndex;
      m_vendor_config.nDataSize = config->nDataSize;
      m_vendor_config.pData = (OMX_U8 *) malloc((config->nDataSize));
      memcpy(m_vendor_config.pData, config->pData,config->nDataSize);
    }
    else if (!strcmp(drv_ctx.kind, "OMX.qcom.video.decoder.vc1"))
    {
        if(m_vendor_config.pData)
        {
            free(m_vendor_config.pData);
            m_vendor_config.pData = NULL;
            m_vendor_config.nDataSize = 0;
        }

        if (((*((OMX_U32 *) config->pData)) &
             VC1_SP_MP_START_CODE_MASK) ==
             VC1_SP_MP_START_CODE)
        {
            DEBUG_PRINT_LOW("set_config - VC1 simple/main profile\n");
            m_vendor_config.nPortIndex = config->nPortIndex;
            m_vendor_config.nDataSize = config->nDataSize;
            m_vendor_config.pData =
                (OMX_U8 *) malloc(config->nDataSize);
            memcpy(m_vendor_config.pData, config->pData,
                   config->nDataSize);
            m_vc1_profile = VC1_SP_MP_RCV;
        }
        else if (*((OMX_U32 *) config->pData) == VC1_AP_SEQ_START_CODE)
        {
            DEBUG_PRINT_LOW("set_config - VC1 Advance profile\n");
            m_vendor_config.nPortIndex = config->nPortIndex;
            m_vendor_config.nDataSize = config->nDataSize;
            m_vendor_config.pData =
                (OMX_U8 *) malloc((config->nDataSize));
            memcpy(m_vendor_config.pData, config->pData,
                   config->nDataSize);
            m_vc1_profile = VC1_AP;
        }
        else if ((config->nDataSize == VC1_STRUCT_C_LEN))
        {
            DEBUG_PRINT_LOW("set_config - VC1 Simple/Main profile struct C only\n");
            m_vendor_config.nPortIndex = config->nPortIndex;
            m_vendor_config.nDataSize  = config->nDataSize;
            m_vendor_config.pData = (OMX_U8*)malloc(config->nDataSize);
            memcpy(m_vendor_config.pData,config->pData,config->nDataSize);
            m_vc1_profile = VC1_SP_MP_RCV;
        }
        else
        {
            DEBUG_PRINT_LOW("set_config - Error: Unknown VC1 profile\n");
        }
    }
    return ret;
  }
  else if (configIndex == OMX_IndexConfigVideoNalSize)
  {

    pNal = reinterpret_cast < OMX_VIDEO_CONFIG_NALSIZE * >(configData);
    nal_length = pNal->nNaluBytes;
    m_frame_parser.init_nal_length(nal_length);
    DEBUG_PRINT_LOW("\n OMX_IndexConfigVideoNalSize called with Size %d",nal_length);
    return ret;
  }

  return OMX_ErrorNotImplemented;
}

/* ======================================================================
FUNCTION
  omx_vdec::GetExtensionIndex

DESCRIPTION
  OMX GetExtensionIndex method implementaion.  <TBD>

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if everything successful.

========================================================================== */
OMX_ERRORTYPE  omx_vdec::get_extension_index(OMX_IN OMX_HANDLETYPE      hComp,
                                                OMX_IN OMX_STRING      paramName,
                                                OMX_OUT OMX_INDEXTYPE* indexType)
{
    if(m_state == OMX_StateInvalid)
    {
        DEBUG_PRINT_ERROR("Get Extension Index in Invalid State\n");
        return OMX_ErrorInvalidState;
    }
    else if (!strncmp(paramName, "OMX.QCOM.index.param.video.SyncFrameDecodingMode",sizeof("OMX.QCOM.index.param.video.SyncFrameDecodingMode") - 1)) {
        *indexType = (OMX_INDEXTYPE)OMX_QcomIndexParamVideoSyncFrameDecodingMode;
    }
    else if (!strncmp(paramName, "OMX.QCOM.index.param.IndexExtraData",sizeof("OMX.QCOM.index.param.IndexExtraData") - 1))
    {
        *indexType = (OMX_INDEXTYPE)OMX_QcomIndexParamIndexExtraDataType;
    }
#if defined (_ANDROID_HONEYCOMB_) || defined (_ANDROID_ICS_)
    else if(!strncmp(paramName,"OMX.google.android.index.enableAndroidNativeBuffers", sizeof("OMX.google.android.index.enableAndroidNativeBuffers") - 1)) {
        *indexType = (OMX_INDEXTYPE)OMX_GoogleAndroidIndexEnableAndroidNativeBuffers;
    }
    else if(!strncmp(paramName,"OMX.google.android.index.useAndroidNativeBuffer2", sizeof("OMX.google.android.index.enableAndroidNativeBuffer2") - 1)) {
        *indexType = (OMX_INDEXTYPE)OMX_GoogleAndroidIndexUseAndroidNativeBuffer2;
    }
    else if(!strncmp(paramName,"OMX.google.android.index.useAndroidNativeBuffer", sizeof("OMX.google.android.index.enableAndroidNativeBuffer") - 1)) {
        DEBUG_PRINT_ERROR("Extension: %s is supported\n", paramName);
        *indexType = (OMX_INDEXTYPE)OMX_GoogleAndroidIndexUseAndroidNativeBuffer;
    }
    else if(!strncmp(paramName,"OMX.google.android.index.getAndroidNativeBufferUsage", sizeof("OMX.google.android.index.getAndroidNativeBufferUsage") - 1)) {
        *indexType = (OMX_INDEXTYPE)OMX_GoogleAndroidIndexGetAndroidNativeBufferUsage;
    }
#endif
	else {
        DEBUG_PRINT_ERROR("Extension: %s not implemented\n", paramName);
        return OMX_ErrorNotImplemented;
    }
    return OMX_ErrorNone;
}

/* ======================================================================
FUNCTION
  omx_vdec::GetState

DESCRIPTION
  Returns the state information back to the caller.<TBD>

PARAMETERS
  <TBD>.

RETURN VALUE
  Error None if everything is successful.
========================================================================== */
OMX_ERRORTYPE  omx_vdec::get_state(OMX_IN OMX_HANDLETYPE  hComp,
                                       OMX_OUT OMX_STATETYPE* state)
{
  *state = m_state;
  DEBUG_PRINT_LOW("get_state: Returning the state %d\n",*state);
  return OMX_ErrorNone;
}

/* ======================================================================
FUNCTION
  omx_vdec::ComponentTunnelRequest

DESCRIPTION
  OMX Component Tunnel Request method implementation. <TBD>

PARAMETERS
  None.

RETURN VALUE
  OMX Error None if everything successful.

========================================================================== */
OMX_ERRORTYPE  omx_vdec::component_tunnel_request(OMX_IN OMX_HANDLETYPE                hComp,
                                                     OMX_IN OMX_U32                        port,
                                                     OMX_IN OMX_HANDLETYPE        peerComponent,
                                                     OMX_IN OMX_U32                    peerPort,
                                                     OMX_INOUT OMX_TUNNELSETUPTYPE* tunnelSetup)
{
  DEBUG_PRINT_ERROR("Error: component_tunnel_request Not Implemented\n");
  return OMX_ErrorNotImplemented;
}

/* ======================================================================
FUNCTION
  omx_vdec::UseOutputBuffer

DESCRIPTION
  Helper function for Use buffer in the input pin

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
OMX_ERRORTYPE  omx_vdec::use_output_buffer(
                         OMX_IN OMX_HANDLETYPE            hComp,
                         OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
                         OMX_IN OMX_U32                   port,
                         OMX_IN OMX_PTR                   appData,
                         OMX_IN OMX_U32                   bytes,
                         OMX_IN OMX_U8*                   buffer)
{
  OMX_ERRORTYPE eRet = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE       *bufHdr= NULL; // buffer header
  unsigned                         i= 0; // Temporary counter
  struct vdec_ioctl_msg ioctl_msg = {NULL,NULL};
  struct vdec_setbuffer_cmd setbuffers;
  OMX_PTR privateAppData = NULL;
  private_handle_t *handle = NULL;
  OMX_U8 *buff = buffer;
  if (!m_out_mem_ptr) {
    DEBUG_PRINT_HIGH("Use_op_buf:Allocating output headers");
    eRet = allocate_output_headers();
  }

  if (eRet == OMX_ErrorNone) {
    for(i=0; i< drv_ctx.op_buf.actualcount; i++) {
      if(BITMASK_ABSENT(&m_out_bm_count,i))
      {
        break;
      }
    }
  }

  if(i >= drv_ctx.op_buf.actualcount) {
    eRet = OMX_ErrorInsufficientResources;
  }

  if (eRet == OMX_ErrorNone) {
#if defined(_ANDROID_HONEYCOMB_) || defined(_ANDROID_ICS_)
    if(m_enable_android_native_buffers) {
        if(m_use_android_native_buffers) {
           UseAndroidNativeBufferParams *params = (UseAndroidNativeBufferParams *)appData;
           sp<android_native_buffer_t> nBuf = params->nativeBuffer;
           handle = (private_handle_t *)nBuf->handle;
           privateAppData = params->pAppPrivate;
        }
        else {
           handle = (private_handle_t *)buff;
           if(!secure_mode) {
	       buff =  (OMX_U8*)mmap(0, handle->size,
                             PROT_READ|PROT_WRITE, MAP_SHARED, handle->fd, 0);
               if (buff == MAP_FAILED) {
                   DEBUG_PRINT_ERROR("Failed to mmap pmem with fd = %d, size = %d", handle->fd, handle->size);
                   return OMX_ErrorInsufficientResources;
               }
	    }
           privateAppData = appData;
        }
        if(!handle) {
            DEBUG_PRINT_ERROR("Native Buffer handle is NULL");
            return OMX_ErrorBadParameter;
        }
        drv_ctx.ptr_outputbuffer[i].pmem_fd = handle->fd;
        drv_ctx.ptr_outputbuffer[i].offset = 0;
        drv_ctx.ptr_outputbuffer[i].bufferaddr = buff;
        drv_ctx.ptr_outputbuffer[i].mmaped_size =
            drv_ctx.ptr_outputbuffer[i].buffer_len = drv_ctx.op_buf.buffer_size;
    } else
#endif

    if (!ouput_egl_buffers && !m_use_output_pmem) {
#ifdef USE_ION
        drv_ctx.op_buf_ion_info[i].ion_device_fd = alloc_map_ion_memory(
                drv_ctx.op_buf.buffer_size,drv_ctx.op_buf.alignment,
                &drv_ctx.op_buf_ion_info[i].ion_alloc_data,
                &drv_ctx.op_buf_ion_info[i].fd_ion_data, 0);
        if(drv_ctx.op_buf_ion_info[i].ion_device_fd < 0) {
          return OMX_ErrorInsufficientResources;
        }
        drv_ctx.ptr_outputbuffer[i].pmem_fd = \
          drv_ctx.op_buf_ion_info[i].fd_ion_data.fd;
#else
        drv_ctx.ptr_outputbuffer[i].pmem_fd = \
          open (MEM_DEVICE,O_RDWR);

        if (drv_ctx.ptr_outputbuffer[i].pmem_fd < 0) {
          return OMX_ErrorInsufficientResources;
        }

        if(drv_ctx.ptr_outputbuffer[i].pmem_fd == 0)
        {
          drv_ctx.ptr_outputbuffer[i].pmem_fd = \
            open (MEM_DEVICE,O_RDWR);
          if (drv_ctx.ptr_outputbuffer[i].pmem_fd < 0) {
            return OMX_ErrorInsufficientResources;
          }
        }

        if(!align_pmem_buffers(drv_ctx.ptr_outputbuffer[i].pmem_fd,
          drv_ctx.op_buf.buffer_size,
          drv_ctx.op_buf.alignment))
        {
          DEBUG_PRINT_ERROR("\n align_pmem_buffers() failed");
          close(drv_ctx.ptr_outputbuffer[i].pmem_fd);
          return OMX_ErrorInsufficientResources;
        }
#endif
        if(!secure_mode) {
            drv_ctx.ptr_outputbuffer[i].bufferaddr =
              (unsigned char *)mmap(NULL, drv_ctx.op_buf.buffer_size,
              PROT_READ|PROT_WRITE, MAP_SHARED,
              drv_ctx.ptr_outputbuffer[i].pmem_fd,0);
            if (drv_ctx.ptr_outputbuffer[i].bufferaddr == MAP_FAILED) {
                close(drv_ctx.ptr_outputbuffer[i].pmem_fd);
#ifdef USE_ION
                free_ion_memory(&drv_ctx.op_buf_ion_info[i]);
#endif
              return OMX_ErrorInsufficientResources;
            }
        }
        drv_ctx.ptr_outputbuffer[i].offset = 0;
        privateAppData = appData;
     }
     else {

       DEBUG_PRINT_LOW("Use_op_buf: out_pmem=%d",m_use_output_pmem);
        if (!appData || !bytes ) {
          if(!secure_mode && !buffer) {
              DEBUG_PRINT_ERROR("\n Bad parameters for use buffer in EGL image case");
              return OMX_ErrorBadParameter;
          }
        }

        OMX_QCOM_PLATFORM_PRIVATE_LIST *pmem_list;
        OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO *pmem_info;
        pmem_list = (OMX_QCOM_PLATFORM_PRIVATE_LIST*) appData;
        if (!pmem_list->entryList || !pmem_list->entryList->entry ||
            !pmem_list->nEntries ||
            pmem_list->entryList->type != OMX_QCOM_PLATFORM_PRIVATE_PMEM) {
          DEBUG_PRINT_ERROR("\n Pmem info not valid in use buffer");
          return OMX_ErrorBadParameter;
        }
        pmem_info = (OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO *)
                    pmem_list->entryList->entry;
        DEBUG_PRINT_LOW("vdec: use buf: pmem_fd=0x%x",
                          pmem_info->pmem_fd);
        drv_ctx.ptr_outputbuffer[i].pmem_fd = pmem_info->pmem_fd;
        drv_ctx.ptr_outputbuffer[i].offset = pmem_info->offset;
        drv_ctx.ptr_outputbuffer[i].bufferaddr = buff;
        drv_ctx.ptr_outputbuffer[i].mmaped_size =
        drv_ctx.ptr_outputbuffer[i].buffer_len = drv_ctx.op_buf.buffer_size;
        privateAppData = appData;
     }
     m_pmem_info[i].offset = drv_ctx.ptr_outputbuffer[i].offset;
     m_pmem_info[i].pmem_fd = drv_ctx.ptr_outputbuffer[i].pmem_fd;

     *bufferHdr = (m_out_mem_ptr + i );
     if(secure_mode)
          drv_ctx.ptr_outputbuffer[i].bufferaddr = *bufferHdr;
     //setbuffers.buffer_type = VDEC_BUFFER_TYPE_OUTPUT;
     memcpy (&setbuffers.buffer,&drv_ctx.ptr_outputbuffer[i],
             sizeof (vdec_bufferpayload));

     //     ioctl_msg.in  = &setbuffers;
     //     ioctl_msg.out = NULL;

     DEBUG_PRINT_HIGH("\n Set the Output Buffer Idx: %d Addr: %x, pmem_fd=%0x%x", i,
                       drv_ctx.ptr_outputbuffer[i],drv_ctx.ptr_outputbuffer[i].pmem_fd );
     // if (ioctl (drv_ctx.video_driver_fd,VDEC_IOCTL_SET_BUFFER,
     //      &ioctl_msg) < 0)
     //  {
     //  DEBUG_PRINT_ERROR("\n Set output buffer failed");
     //   return OMX_ErrorInsufficientResources;
     //  }
     // found an empty buffer at i
     (*bufferHdr)->nAllocLen = drv_ctx.op_buf.buffer_size;
     (*bufferHdr)->pBuffer = buff;
     (*bufferHdr)->pAppPrivate = privateAppData;
     BITMASK_SET(&m_out_bm_count,i);
  }
  return eRet;
}

/* ======================================================================
FUNCTION
  omx_vdec::use_input_heap_buffers

DESCRIPTION
  OMX Use Buffer Heap allocation method implementation.

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None , if everything successful.

========================================================================== */
OMX_ERRORTYPE  omx_vdec::use_input_heap_buffers(
                         OMX_IN OMX_HANDLETYPE            hComp,
                         OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
                         OMX_IN OMX_U32                   port,
                         OMX_IN OMX_PTR                   appData,
                         OMX_IN OMX_U32                   bytes,
                         OMX_IN OMX_U8*                   buffer)
{
  DEBUG_PRINT_LOW("Inside %s, %p\n", __FUNCTION__, buffer);
  OMX_ERRORTYPE eRet = OMX_ErrorNone;
  if(!m_inp_heap_ptr)
    m_inp_heap_ptr = (OMX_BUFFERHEADERTYPE*)
               calloc( (sizeof(OMX_BUFFERHEADERTYPE)),
               drv_ctx.ip_buf.actualcount);
  if(!m_phdr_pmem_ptr)
    m_phdr_pmem_ptr = (OMX_BUFFERHEADERTYPE**)
               calloc( (sizeof(OMX_BUFFERHEADERTYPE*)),
               drv_ctx.ip_buf.actualcount);
  if(!m_inp_heap_ptr || !m_phdr_pmem_ptr)
  {
    DEBUG_PRINT_ERROR("Insufficent memory");
    eRet = OMX_ErrorInsufficientResources;
  }
  else if (m_in_alloc_cnt < drv_ctx.ip_buf.actualcount)
  {
    input_use_buffer = true;
    memset(&m_inp_heap_ptr[m_in_alloc_cnt], 0, sizeof(OMX_BUFFERHEADERTYPE));
    m_inp_heap_ptr[m_in_alloc_cnt].pBuffer = buffer;
    m_inp_heap_ptr[m_in_alloc_cnt].nAllocLen = bytes;
    m_inp_heap_ptr[m_in_alloc_cnt].pAppPrivate = appData;
    m_inp_heap_ptr[m_in_alloc_cnt].nInputPortIndex = (OMX_U32) OMX_DirInput;
    m_inp_heap_ptr[m_in_alloc_cnt].nOutputPortIndex = (OMX_U32) OMX_DirMax;
    *bufferHdr = &m_inp_heap_ptr[m_in_alloc_cnt];
    eRet = allocate_input_buffer(hComp, &m_phdr_pmem_ptr[m_in_alloc_cnt], port, appData, bytes);
    DEBUG_PRINT_HIGH("\n Heap buffer(%p) Pmem buffer(%p)", *bufferHdr, m_phdr_pmem_ptr[m_in_alloc_cnt]);
    if (!m_input_free_q.insert_entry((unsigned)m_phdr_pmem_ptr[m_in_alloc_cnt], NULL, NULL))
    {
      DEBUG_PRINT_ERROR("\nERROR:Free_q is full");
      return OMX_ErrorInsufficientResources;
    }
    m_in_alloc_cnt++;
  }
  else
  {
    DEBUG_PRINT_ERROR("All i/p buffers have been set!");
    eRet = OMX_ErrorInsufficientResources;
  }
  return eRet;
}

/* ======================================================================
FUNCTION
  omx_vdec::UseBuffer

DESCRIPTION
  OMX Use Buffer method implementation.

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None , if everything successful.

========================================================================== */
OMX_ERRORTYPE  omx_vdec::use_buffer(
                         OMX_IN OMX_HANDLETYPE            hComp,
                         OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
                         OMX_IN OMX_U32                   port,
                         OMX_IN OMX_PTR                   appData,
                         OMX_IN OMX_U32                   bytes,
                         OMX_IN OMX_U8*                   buffer)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  struct vdec_setbuffer_cmd setbuffers;
  struct vdec_ioctl_msg ioctl_msg = {NULL,NULL};

  if (bufferHdr == NULL || bytes == 0)
  {
      if(!secure_mode && buffer == NULL) {
          DEBUG_PRINT_ERROR("bad param 0x%p %ld 0x%p",bufferHdr, bytes, buffer);
          return OMX_ErrorBadParameter;
      }
  }
  if(m_state == OMX_StateInvalid)
  {
    DEBUG_PRINT_ERROR("Use Buffer in Invalid State\n");
    return OMX_ErrorInvalidState;
  }
  if(port == OMX_CORE_INPUT_PORT_INDEX)
    error = use_input_heap_buffers(hComp, bufferHdr, port, appData, bytes, buffer);
  else if(port == OMX_CORE_OUTPUT_PORT_INDEX)
    error = use_output_buffer(hComp,bufferHdr,port,appData,bytes,buffer); //not tested
  else
  {
    DEBUG_PRINT_ERROR("Error: Invalid Port Index received %d\n",(int)port);
    error = OMX_ErrorBadPortIndex;
  }
  DEBUG_PRINT_LOW("Use Buffer: port %u, buffer %p, eRet %d", port, *bufferHdr, error);
  if(error == OMX_ErrorNone)
  {
    if(allocate_done() && BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING))
    {
      // Send the callback now
      BITMASK_CLEAR((&m_flags),OMX_COMPONENT_IDLE_PENDING);
      post_event(OMX_CommandStateSet,OMX_StateIdle,
                         OMX_COMPONENT_GENERATE_EVENT);
    }
    if(port == OMX_CORE_INPUT_PORT_INDEX && m_inp_bPopulated &&
       BITMASK_PRESENT(&m_flags,OMX_COMPONENT_INPUT_ENABLE_PENDING))
    {
      BITMASK_CLEAR((&m_flags),OMX_COMPONENT_INPUT_ENABLE_PENDING);
      post_event(OMX_CommandPortEnable,
          OMX_CORE_INPUT_PORT_INDEX,
          OMX_COMPONENT_GENERATE_EVENT);
    }
    else if(port == OMX_CORE_OUTPUT_PORT_INDEX && m_out_bPopulated &&
            BITMASK_PRESENT(&m_flags,OMX_COMPONENT_OUTPUT_ENABLE_PENDING))
    {
      BITMASK_CLEAR((&m_flags),OMX_COMPONENT_OUTPUT_ENABLE_PENDING);
      post_event(OMX_CommandPortEnable,
                 OMX_CORE_OUTPUT_PORT_INDEX,
                 OMX_COMPONENT_GENERATE_EVENT);
    }
  }
  return error;
}

OMX_ERRORTYPE omx_vdec::free_input_buffer(unsigned int bufferindex,
                                OMX_BUFFERHEADERTYPE *pmem_bufferHdr)
{
  if (m_inp_heap_ptr && !input_use_buffer && arbitrary_bytes)
  {
    if(m_inp_heap_ptr[bufferindex].pBuffer)
      free(m_inp_heap_ptr[bufferindex].pBuffer);
    m_inp_heap_ptr[bufferindex].pBuffer = NULL;
  }
  if (pmem_bufferHdr)
    free_input_buffer(pmem_bufferHdr);
  return OMX_ErrorNone;
}

OMX_ERRORTYPE omx_vdec::free_input_buffer(OMX_BUFFERHEADERTYPE *bufferHdr)
{
  unsigned int index = 0;
  if (bufferHdr == NULL || m_inp_mem_ptr == NULL)
  {
    return OMX_ErrorBadParameter;
  }

  index = bufferHdr - m_inp_mem_ptr;
  DEBUG_PRINT_LOW("\n Free Input Buffer index = %d",index);

  if (index < drv_ctx.ip_buf.actualcount && drv_ctx.ptr_inputbuffer)
  {
    DEBUG_PRINT_LOW("\n Free Input Buffer index = %d",index);
    if (drv_ctx.ptr_inputbuffer[index].pmem_fd > 0)
    {
       struct vdec_ioctl_msg ioctl_msg = {NULL,NULL};
       struct vdec_setbuffer_cmd setbuffers;
       setbuffers.buffer_type = VDEC_BUFFER_TYPE_INPUT;
       memcpy (&setbuffers.buffer,&drv_ctx.ptr_inputbuffer[index],
          sizeof (vdec_bufferpayload));
       ioctl_msg.in  = &setbuffers;
       ioctl_msg.out = NULL;
       int ioctl_r; //= ioctl (drv_ctx.video_driver_fd,
           //                 VDEC_IOCTL_FREE_BUFFER, &ioctl_msg);
       if (ioctl_r < 0)
       {
          DEBUG_PRINT_ERROR("\nVDEC_IOCTL_FREE_BUFFER returned error %d", ioctl_r);
       }

       DEBUG_PRINT_LOW("\n unmap the input buffer fd=%d",
                    drv_ctx.ptr_inputbuffer[index].pmem_fd);
       DEBUG_PRINT_LOW("\n unmap the input buffer size=%d  address = %d",
                    drv_ctx.ptr_inputbuffer[index].mmaped_size,
                    drv_ctx.ptr_inputbuffer[index].bufferaddr);
       munmap (drv_ctx.ptr_inputbuffer[index].bufferaddr,
               drv_ctx.ptr_inputbuffer[index].mmaped_size);
       close (drv_ctx.ptr_inputbuffer[index].pmem_fd);
       drv_ctx.ptr_inputbuffer[index].pmem_fd = -1;
       if (m_desc_buffer_ptr && m_desc_buffer_ptr[index].buf_addr)
       {
         free(m_desc_buffer_ptr[index].buf_addr);
         m_desc_buffer_ptr[index].buf_addr = NULL;
         m_desc_buffer_ptr[index].desc_data_size = 0;
       }
#ifdef USE_ION
       free_ion_memory(&drv_ctx.ip_buf_ion_info[index]);
#endif
    }
  }

  return OMX_ErrorNone;
}

OMX_ERRORTYPE omx_vdec::free_output_buffer(OMX_BUFFERHEADERTYPE *bufferHdr)
{
  unsigned int index = 0;

  if (bufferHdr == NULL || m_out_mem_ptr == NULL)
  {
    return OMX_ErrorBadParameter;
  }

  index = bufferHdr - m_out_mem_ptr;
  DEBUG_PRINT_LOW("\n Free ouput Buffer index = %d",index);

  if (index < drv_ctx.op_buf.actualcount
      && drv_ctx.ptr_outputbuffer)
  {
    DEBUG_PRINT_LOW("\n Free ouput Buffer index = %d addr = %x", index,
                    drv_ctx.ptr_outputbuffer[index].bufferaddr);

    struct vdec_ioctl_msg ioctl_msg = {NULL,NULL};
    struct vdec_setbuffer_cmd setbuffers;
    setbuffers.buffer_type = VDEC_BUFFER_TYPE_OUTPUT;
    memcpy (&setbuffers.buffer,&drv_ctx.ptr_outputbuffer[index],
        sizeof (vdec_bufferpayload));
    ioctl_msg.in  = &setbuffers;
    ioctl_msg.out = NULL;
    DEBUG_PRINT_LOW("\nRelease the Output Buffer");
    if (/*ioctl (drv_ctx.video_driver_fd, VDEC_IOCTL_FREE_BUFFER,
          &ioctl_msg) < */0)
      DEBUG_PRINT_ERROR("\nRelease output buffer failed in VCD");

#ifdef _ANDROID_
    if(m_enable_android_native_buffers) {
        if(drv_ctx.ptr_outputbuffer[index].pmem_fd > 0) {
            munmap(drv_ctx.ptr_outputbuffer[index].bufferaddr,
                    drv_ctx.ptr_outputbuffer[index].mmaped_size);
        }
        drv_ctx.ptr_outputbuffer[index].pmem_fd = -1;
    } else {
#endif
        if (drv_ctx.ptr_outputbuffer[0].pmem_fd > 0 && !ouput_egl_buffers && !m_use_output_pmem)
        {
            DEBUG_PRINT_LOW("\n unmap the output buffer fd = %d",
                    drv_ctx.ptr_outputbuffer[0].pmem_fd);
            DEBUG_PRINT_LOW("\n unmap the ouput buffer size=%d  address = %d",
                    drv_ctx.ptr_outputbuffer[0].mmaped_size,
                    drv_ctx.ptr_outputbuffer[0].bufferaddr);
            munmap (drv_ctx.ptr_outputbuffer[0].bufferaddr,
                    drv_ctx.ptr_outputbuffer[0].mmaped_size);
          close (drv_ctx.ptr_outputbuffer[0].pmem_fd);
          drv_ctx.ptr_outputbuffer[0].pmem_fd = -1;
#ifdef USE_ION
       free_ion_memory(&drv_ctx.op_buf_ion_info[0]);
#endif
        }
#ifdef _ANDROID_
    }
#endif
  }

  return OMX_ErrorNone;

}

OMX_ERRORTYPE omx_vdec::allocate_input_heap_buffer(OMX_HANDLETYPE       hComp,
                                         OMX_BUFFERHEADERTYPE **bufferHdr,
                                         OMX_U32              port,
                                         OMX_PTR              appData,
                                         OMX_U32              bytes)
{
  OMX_BUFFERHEADERTYPE *input = NULL;
  unsigned char *buf_addr = NULL;
  OMX_ERRORTYPE eRet = OMX_ErrorNone;
  unsigned   i = 0;

  /* Sanity Check*/
  if (bufferHdr == NULL)
  {
    return OMX_ErrorBadParameter;
  }

  if (m_inp_heap_ptr == NULL)
  {
    m_inp_heap_ptr = (OMX_BUFFERHEADERTYPE*) \
                     calloc( (sizeof(OMX_BUFFERHEADERTYPE)),
                     drv_ctx.ip_buf.actualcount);
    m_phdr_pmem_ptr = (OMX_BUFFERHEADERTYPE**) \
                     calloc( (sizeof(OMX_BUFFERHEADERTYPE*)),
                     drv_ctx.ip_buf.actualcount);

    if (m_inp_heap_ptr == NULL)
    {
      DEBUG_PRINT_ERROR("\n m_inp_heap_ptr Allocation failed ");
      return OMX_ErrorInsufficientResources;
    }
  }

  /*Find a Free index*/
  for(i=0; i< drv_ctx.ip_buf.actualcount; i++)
  {
    if(BITMASK_ABSENT(&m_heap_inp_bm_count,i))
    {
      DEBUG_PRINT_LOW("\n Free Input Buffer Index %d",i);
      break;
    }
  }

  if (i < drv_ctx.ip_buf.actualcount)
  {
    buf_addr = (unsigned char *)malloc (drv_ctx.ip_buf.buffer_size);

    if (buf_addr == NULL)
    {
      return OMX_ErrorInsufficientResources;
    }

    *bufferHdr = (m_inp_heap_ptr + i);
    input = *bufferHdr;
    BITMASK_SET(&m_heap_inp_bm_count,i);

    input->pBuffer           = (OMX_U8 *)buf_addr;
    input->nSize             = sizeof(OMX_BUFFERHEADERTYPE);
    input->nVersion.nVersion = OMX_SPEC_VERSION;
    input->nAllocLen         = drv_ctx.ip_buf.buffer_size;
    input->pAppPrivate       = appData;
    input->nInputPortIndex   = OMX_CORE_INPUT_PORT_INDEX;
    DEBUG_PRINT_LOW("\n Address of Heap Buffer %p",*bufferHdr );
    eRet = allocate_input_buffer(hComp,&m_phdr_pmem_ptr [i],port,appData,bytes);
    DEBUG_PRINT_LOW("\n Address of Pmem Buffer %p",m_phdr_pmem_ptr [i] );
    /*Add the Buffers to freeq*/
    if (!m_input_free_q.insert_entry((unsigned)m_phdr_pmem_ptr [i],NULL,NULL))
    {
      DEBUG_PRINT_ERROR("\nERROR:Free_q is full");
      return OMX_ErrorInsufficientResources;
    }
  }
  else
  {
    return OMX_ErrorBadParameter;
  }

  return eRet;

}


/* ======================================================================
FUNCTION
  omx_vdec::AllocateInputBuffer

DESCRIPTION
  Helper function for allocate buffer in the input pin

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
OMX_ERRORTYPE  omx_vdec::allocate_input_buffer(
                         OMX_IN OMX_HANDLETYPE            hComp,
                         OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
                         OMX_IN OMX_U32                   port,
                         OMX_IN OMX_PTR                   appData,
                         OMX_IN OMX_U32                   bytes)
{

  OMX_ERRORTYPE eRet = OMX_ErrorNone;
  struct vdec_setbuffer_cmd setbuffers;
  OMX_BUFFERHEADERTYPE *input = NULL;
  struct vdec_ioctl_msg ioctl_msg = {NULL,NULL};
  unsigned   i = 0;
  unsigned char *buf_addr = NULL;
  int pmem_fd = -1;

  if(bytes != drv_ctx.ip_buf.buffer_size)
  {
    DEBUG_PRINT_LOW("\n Requested Size is wrong %d epected is %d",
      bytes, drv_ctx.ip_buf.buffer_size);
    return OMX_ErrorBadParameter;
  }

  if(!m_inp_mem_ptr)
  {
    DEBUG_PRINT_HIGH("\n Allocate i/p buffer Header: Cnt(%d) Sz(%d)",
      drv_ctx.ip_buf.actualcount,
      drv_ctx.ip_buf.buffer_size);

    m_inp_mem_ptr = (OMX_BUFFERHEADERTYPE*) \
    calloc( (sizeof(OMX_BUFFERHEADERTYPE)), drv_ctx.ip_buf.actualcount);

    if (m_inp_mem_ptr == NULL)
    {
      return OMX_ErrorInsufficientResources;
    }

    drv_ctx.ptr_inputbuffer = (struct vdec_bufferpayload *) \
    calloc ((sizeof (struct vdec_bufferpayload)),drv_ctx.ip_buf.actualcount);

    if (drv_ctx.ptr_inputbuffer == NULL)
    {
      return OMX_ErrorInsufficientResources;
    }
#ifdef USE_ION
    drv_ctx.ip_buf_ion_info = (struct vdec_ion *) \
    calloc ((sizeof (struct vdec_ion)),drv_ctx.ip_buf.actualcount);

    if (drv_ctx.ip_buf_ion_info == NULL)
    {
      return OMX_ErrorInsufficientResources;
    }
#endif

    for (i=0; i < drv_ctx.ip_buf.actualcount; i++)
    {
      drv_ctx.ptr_inputbuffer [i].pmem_fd = -1;
#ifdef USE_ION
      drv_ctx.ip_buf_ion_info[i].ion_device_fd = -1;
#endif
    }
  }

  for(i=0; i< drv_ctx.ip_buf.actualcount; i++)
  {
    if(BITMASK_ABSENT(&m_inp_bm_count,i))
    {
      DEBUG_PRINT_LOW("\n Free Input Buffer Index %d",i);
      break;
    }
  }

  if(i < drv_ctx.ip_buf.actualcount)
  {
    struct v4l2_buffer buf;
    struct v4l2_plane plane;
    int rc;
    DEBUG_PRINT_LOW("\n Allocate input Buffer");
#ifdef USE_ION
 drv_ctx.ip_buf_ion_info[i].ion_device_fd = alloc_map_ion_memory(
                    drv_ctx.ip_buf.buffer_size,drv_ctx.op_buf.alignment,
                    &drv_ctx.ip_buf_ion_info[i].ion_alloc_data,
		    &drv_ctx.ip_buf_ion_info[i].fd_ion_data, 0);
    if(drv_ctx.ip_buf_ion_info[i].ion_device_fd < 0) {
        return OMX_ErrorInsufficientResources;
     }
    pmem_fd = drv_ctx.ip_buf_ion_info[i].fd_ion_data.fd;
#else
    pmem_fd = open (MEM_DEVICE,O_RDWR);

    if (pmem_fd < 0)
    {
      DEBUG_PRINT_ERROR("\n open failed for pmem/adsp for input buffer");
      return OMX_ErrorInsufficientResources;
    }

    if (pmem_fd == 0)
    {
      pmem_fd = open (MEM_DEVICE,O_RDWR);

      if (pmem_fd < 0)
      {
        DEBUG_PRINT_ERROR("\n open failed for pmem/adsp for input buffer");
        return OMX_ErrorInsufficientResources;
      }
    }

    if(!align_pmem_buffers(pmem_fd, drv_ctx.ip_buf.buffer_size,
      drv_ctx.ip_buf.alignment))
    {
      DEBUG_PRINT_ERROR("\n align_pmem_buffers() failed");
      close(pmem_fd);
      return OMX_ErrorInsufficientResources;
    }
#endif
    if (!secure_mode) {
        buf_addr = (unsigned char *)mmap(NULL,
          drv_ctx.ip_buf.buffer_size,
          PROT_READ|PROT_WRITE, MAP_SHARED, pmem_fd, 0);

        if (buf_addr == MAP_FAILED)
        {
            close(pmem_fd);
#ifdef USE_ION
            free_ion_memory(&drv_ctx.ip_buf_ion_info[i]);
#endif
          DEBUG_PRINT_ERROR("\n Map Failed to allocate input buffer");
          return OMX_ErrorInsufficientResources;
        }
    }
    *bufferHdr = (m_inp_mem_ptr + i);
    if (secure_mode)
        drv_ctx.ptr_inputbuffer [i].bufferaddr = *bufferHdr;
    else
        drv_ctx.ptr_inputbuffer [i].bufferaddr = buf_addr;
    drv_ctx.ptr_inputbuffer [i].pmem_fd = pmem_fd;
    drv_ctx.ptr_inputbuffer [i].buffer_len = drv_ctx.ip_buf.buffer_size;
    drv_ctx.ptr_inputbuffer [i].mmaped_size = drv_ctx.ip_buf.buffer_size;
    drv_ctx.ptr_inputbuffer [i].offset = 0;


    buf.index = i;
    buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    buf.memory = V4L2_MEMORY_USERPTR;
    plane.bytesused = 0;
    plane.length = drv_ctx.ptr_inputbuffer [i].mmaped_size;
    plane.m.userptr = (unsigned long)drv_ctx.ptr_inputbuffer[i].bufferaddr;
    plane.reserved[0] =drv_ctx.ptr_inputbuffer [i].pmem_fd;
    plane.reserved[1] = 0;
    plane.data_offset = drv_ctx.ptr_inputbuffer[i].offset;
    buf.m.planes = &plane;
    buf.length = 1;

     DEBUG_PRINT_LOW("\n Set the Output Buffer Idx: %d Addr: %x", i, drv_ctx.ptr_inputbuffer[i]);

     rc = ioctl(drv_ctx.video_driver_fd, VIDIOC_PREPARE_BUF, &buf);

     if (rc) {
       printf("Failed to prepare bufs\n");
	   /*TODO: How to handle this case */
       return OMX_ErrorInsufficientResources;
     }

    input = *bufferHdr;
    BITMASK_SET(&m_inp_bm_count,i);
    DEBUG_PRINT_LOW("\n Buffer address %p of pmem",*bufferHdr);
    if (secure_mode)
         input->pBuffer = (OMX_U8 *)drv_ctx.ptr_inputbuffer [i].pmem_fd;
    else
         input->pBuffer           = (OMX_U8 *)buf_addr;
    input->nSize             = sizeof(OMX_BUFFERHEADERTYPE);
    input->nVersion.nVersion = OMX_SPEC_VERSION;
    input->nAllocLen         = drv_ctx.ip_buf.buffer_size;
    input->pAppPrivate       = appData;
    input->nInputPortIndex   = OMX_CORE_INPUT_PORT_INDEX;
    input->pInputPortPrivate = (void *)&drv_ctx.ptr_inputbuffer [i];

    if (drv_ctx.disable_dmx)
    {
      eRet = allocate_desc_buffer(i);
    }
  }
  else
  {
    DEBUG_PRINT_ERROR("\nERROR:Input Buffer Index not found");
    eRet = OMX_ErrorInsufficientResources;
  }
  return eRet;
}


/* ======================================================================
FUNCTION
  omx_vdec::AllocateOutputBuffer

DESCRIPTION
  Helper fn for AllocateBuffer in the output pin

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if everything went well.

========================================================================== */
OMX_ERRORTYPE  omx_vdec::allocate_output_buffer(
                         OMX_IN OMX_HANDLETYPE            hComp,
                         OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
                         OMX_IN OMX_U32                   port,
                         OMX_IN OMX_PTR                   appData,
                         OMX_IN OMX_U32                   bytes)
{
  OMX_ERRORTYPE eRet = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE       *bufHdr= NULL; // buffer header
  unsigned                         i= 0; // Temporary counter
  struct vdec_ioctl_msg ioctl_msg = {NULL,NULL};
  struct vdec_setbuffer_cmd setbuffers;
#ifdef USE_ION
  int ion_device_fd =-1;
  struct ion_allocation_data ion_alloc_data;
  struct ion_fd_data fd_ion_data;
#endif
  if(!m_out_mem_ptr)
  {
    DEBUG_PRINT_HIGH("\n Allocate o/p buffer Header: Cnt(%d) Sz(%d)",
      drv_ctx.op_buf.actualcount,
      drv_ctx.op_buf.buffer_size);
    int nBufHdrSize        = 0;
    int nPlatformEntrySize = 0;
    int nPlatformListSize  = 0;
    int nPMEMInfoSize = 0;
    int pmem_fd = -1;
    unsigned char *pmem_baseaddress = NULL;

    OMX_QCOM_PLATFORM_PRIVATE_LIST      *pPlatformList;
    OMX_QCOM_PLATFORM_PRIVATE_ENTRY     *pPlatformEntry;
    OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO *pPMEMInfo;

    DEBUG_PRINT_LOW("Allocating First Output Buffer(%d)\n",
      drv_ctx.op_buf.actualcount);
    nBufHdrSize        = drv_ctx.op_buf.actualcount *
                         sizeof(OMX_BUFFERHEADERTYPE);

    nPMEMInfoSize      = drv_ctx.op_buf.actualcount *
                         sizeof(OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO);
    nPlatformListSize  = drv_ctx.op_buf.actualcount *
                         sizeof(OMX_QCOM_PLATFORM_PRIVATE_LIST);
    nPlatformEntrySize = drv_ctx.op_buf.actualcount *
                         sizeof(OMX_QCOM_PLATFORM_PRIVATE_ENTRY);

    DEBUG_PRINT_LOW("TotalBufHdr %d BufHdrSize %d PMEM %d PL %d\n",nBufHdrSize,
                         sizeof(OMX_BUFFERHEADERTYPE),
                         nPMEMInfoSize,
                         nPlatformListSize);
    DEBUG_PRINT_LOW("PE %d OutputBuffer Count %d \n",nPlatformEntrySize,
                         drv_ctx.op_buf.actualcount);
#ifdef USE_ION
 ion_device_fd = alloc_map_ion_memory(
                    drv_ctx.op_buf.buffer_size * drv_ctx.op_buf.actualcount,
                    drv_ctx.op_buf.alignment,
                    &ion_alloc_data, &fd_ion_data, 0);
    if (ion_device_fd < 0) {
        return OMX_ErrorInsufficientResources;
    }
    pmem_fd = fd_ion_data.fd;
#else
    pmem_fd = open (MEM_DEVICE,O_RDWR);

    if (pmem_fd < 0)
    {
      DEBUG_PRINT_ERROR("\nERROR:pmem fd for output buffer %d",
        drv_ctx.op_buf.buffer_size);
      return OMX_ErrorInsufficientResources;
    }

    if(pmem_fd == 0)
    {
      pmem_fd = open (MEM_DEVICE,O_RDWR);

      if (pmem_fd < 0)
      {
        DEBUG_PRINT_ERROR("\nERROR:pmem fd for output buffer %d",
          drv_ctx.op_buf.buffer_size);
        return OMX_ErrorInsufficientResources;
      }
    }

    if(!align_pmem_buffers(pmem_fd, drv_ctx.op_buf.buffer_size *
      drv_ctx.op_buf.actualcount,
      drv_ctx.op_buf.alignment))
    {
      DEBUG_PRINT_ERROR("\n align_pmem_buffers() failed");
      close(pmem_fd);
      return OMX_ErrorInsufficientResources;
    }
#endif
   if (!secure_mode) {
        pmem_baseaddress = (unsigned char *)mmap(NULL,
                           (drv_ctx.op_buf.buffer_size *
                            drv_ctx.op_buf.actualcount),
                            PROT_READ|PROT_WRITE,MAP_SHARED,pmem_fd,0);
        if (pmem_baseaddress == MAP_FAILED)
        {
          DEBUG_PRINT_ERROR("\n MMAP failed for Size %d",
          drv_ctx.op_buf.buffer_size);
          close(pmem_fd);
#ifdef USE_ION
          free_ion_memory(&drv_ctx.op_buf_ion_info[i]);
#endif
          return OMX_ErrorInsufficientResources;
        }
    }
    m_out_mem_ptr = (OMX_BUFFERHEADERTYPE  *)calloc(nBufHdrSize,1);
    // Alloc mem for platform specific info
    char *pPtr=NULL;
    pPtr = (char*) calloc(nPlatformListSize + nPlatformEntrySize +
                                     nPMEMInfoSize,1);
    drv_ctx.ptr_outputbuffer = (struct vdec_bufferpayload *)\
      calloc (sizeof(struct vdec_bufferpayload),
      drv_ctx.op_buf.actualcount);
    drv_ctx.ptr_respbuffer = (struct vdec_output_frameinfo  *)\
      calloc (sizeof (struct vdec_output_frameinfo),
      drv_ctx.op_buf.actualcount);
#ifdef USE_ION
    drv_ctx.op_buf_ion_info = (struct vdec_ion *)\
      calloc (sizeof(struct vdec_ion),
      drv_ctx.op_buf.actualcount);
#endif

    if(m_out_mem_ptr && pPtr && drv_ctx.ptr_outputbuffer
       && drv_ctx.ptr_respbuffer)
    {
      drv_ctx.ptr_outputbuffer[0].mmaped_size =
        (drv_ctx.op_buf.buffer_size *
         drv_ctx.op_buf.actualcount);
      bufHdr          =  m_out_mem_ptr;
      m_platform_list = (OMX_QCOM_PLATFORM_PRIVATE_LIST *)(pPtr);
      m_platform_entry= (OMX_QCOM_PLATFORM_PRIVATE_ENTRY *)
                        (((char *) m_platform_list)  + nPlatformListSize);
      m_pmem_info     = (OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO *)
                        (((char *) m_platform_entry) + nPlatformEntrySize);
      pPlatformList   = m_platform_list;
      pPlatformEntry  = m_platform_entry;
      pPMEMInfo       = m_pmem_info;

      DEBUG_PRINT_LOW("Memory Allocation Succeeded for OUT port%p\n",m_out_mem_ptr);

      // Settting the entire storage nicely
      DEBUG_PRINT_LOW("bHdr %p OutMem %p PE %p\n",bufHdr, m_out_mem_ptr,pPlatformEntry);
      DEBUG_PRINT_LOW(" Pmem Info = %p \n",pPMEMInfo);
      for(i=0; i < drv_ctx.op_buf.actualcount ; i++)
      {
        bufHdr->nSize              = sizeof(OMX_BUFFERHEADERTYPE);
        bufHdr->nVersion.nVersion  = OMX_SPEC_VERSION;
        // Set the values when we determine the right HxW param
        bufHdr->nAllocLen          = bytes;
        bufHdr->nFilledLen         = 0;
        bufHdr->pAppPrivate        = appData;
        bufHdr->nOutputPortIndex   = OMX_CORE_OUTPUT_PORT_INDEX;
        // Platform specific PMEM Information
        // Initialize the Platform Entry
        //DEBUG_PRINT_LOW("Initializing the Platform Entry for %d\n",i);
        pPlatformEntry->type       = OMX_QCOM_PLATFORM_PRIVATE_PMEM;
        pPlatformEntry->entry      = pPMEMInfo;
        // Initialize the Platform List
        pPlatformList->nEntries    = 1;
        pPlatformList->entryList   = pPlatformEntry;
        // Keep pBuffer NULL till vdec is opened
        bufHdr->pBuffer            = NULL;
        bufHdr->nOffset            = 0;

        pPMEMInfo->offset          =  drv_ctx.op_buf.buffer_size*i;
        pPMEMInfo->pmem_fd = 0;
        bufHdr->pPlatformPrivate = pPlatformList;

        drv_ctx.ptr_outputbuffer[i].pmem_fd = pmem_fd;
#ifdef USE_ION
        drv_ctx.op_buf_ion_info[i].ion_device_fd = ion_device_fd;
        drv_ctx.op_buf_ion_info[i].ion_alloc_data = ion_alloc_data;
        drv_ctx.op_buf_ion_info[i].fd_ion_data = fd_ion_data;
#endif

        /*Create a mapping between buffers*/
        bufHdr->pOutputPortPrivate = &drv_ctx.ptr_respbuffer[i];
        drv_ctx.ptr_respbuffer[i].client_data = (void *)\
                                            &drv_ctx.ptr_outputbuffer[i];
        drv_ctx.ptr_outputbuffer[i].offset = drv_ctx.op_buf.buffer_size*i;
        drv_ctx.ptr_outputbuffer[i].bufferaddr =
          pmem_baseaddress + (drv_ctx.op_buf.buffer_size*i);

        DEBUG_PRINT_LOW("\n pmem_fd = %d offset = %d address = %p",
          pmem_fd, drv_ctx.ptr_outputbuffer[i].offset,
          drv_ctx.ptr_outputbuffer[i].bufferaddr);
        // Move the buffer and buffer header pointers
        bufHdr++;
        pPMEMInfo++;
        pPlatformEntry++;
        pPlatformList++;
      }
    }
    else
    {
      DEBUG_PRINT_ERROR("Output buf mem alloc failed[0x%x][0x%x]\n",\
                                        m_out_mem_ptr, pPtr);
      if(m_out_mem_ptr)
      {
        free(m_out_mem_ptr);
        m_out_mem_ptr = NULL;
      }
      if(pPtr)
      {
        free(pPtr);
        pPtr = NULL;
      }
      if(drv_ctx.ptr_outputbuffer)
      {
        free(drv_ctx.ptr_outputbuffer);
        drv_ctx.ptr_outputbuffer = NULL;
      }
      if(drv_ctx.ptr_respbuffer)
      {
        free(drv_ctx.ptr_respbuffer);
        drv_ctx.ptr_respbuffer = NULL;
      }
#ifdef USE_ION
    if (drv_ctx.op_buf_ion_info) {
        DEBUG_PRINT_LOW("\n Free o/p ion context");
	free(drv_ctx.op_buf_ion_info);
        drv_ctx.op_buf_ion_info = NULL;
    }
#endif
      eRet =  OMX_ErrorInsufficientResources;
    }
  }

  for(i=0; i< drv_ctx.op_buf.actualcount; i++)
  {
    if(BITMASK_ABSENT(&m_out_bm_count,i))
    {
      DEBUG_PRINT_LOW("\n Found a Free Output Buffer %d",i);
      break;
    }
  }

  if (eRet == OMX_ErrorNone)
  {
    if(i < drv_ctx.op_buf.actualcount)
    {
      struct v4l2_buffer buf;
      struct v4l2_plane plane;
      int rc;
      m_pmem_info[i].offset = drv_ctx.ptr_outputbuffer[i].offset;

      drv_ctx.ptr_outputbuffer[i].buffer_len =
        drv_ctx.op_buf.buffer_size;

    *bufferHdr = (m_out_mem_ptr + i );
    if (secure_mode) {
       drv_ctx.ptr_outputbuffer[i].bufferaddr = *bufferHdr;
    }
   drv_ctx.ptr_outputbuffer[i].mmaped_size = drv_ctx.op_buf.buffer_size;

     buf.index = i;
     buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
     buf.memory = V4L2_MEMORY_USERPTR;
     plane.length = drv_ctx.op_buf.buffer_size;
     plane.m.userptr = (unsigned long)(drv_ctx.ptr_outputbuffer[i].bufferaddr-drv_ctx.ptr_outputbuffer[i].offset);
#ifdef USE_ION
     plane.reserved[0] = drv_ctx.op_buf_ion_info[i].fd_ion_data.fd;
#endif
     plane.reserved[1] = drv_ctx.ptr_outputbuffer[i].offset;
     plane.data_offset = 0;
     buf.m.planes = &plane;
     buf.length = 1;

	 DEBUG_PRINT_LOW("\n Set the Output Buffer Idx: %d Addr: %x", i, drv_ctx.ptr_outputbuffer[i]);
      rc = ioctl(drv_ctx.video_driver_fd, VIDIOC_PREPARE_BUF, &buf);
      if (rc) {
		/*TODO: How to handle this case */
       return OMX_ErrorInsufficientResources;
     }

	  if (i == (drv_ctx.op_buf.actualcount -1 ) && !streaming[CAPTURE_PORT]) {
		enum v4l2_buf_type buf_type;
		buf_type=V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		rc=ioctl(drv_ctx.video_driver_fd, VIDIOC_STREAMON,&buf_type);
		if (rc) {
			return OMX_ErrorInsufficientResources;
		} else {
			streaming[CAPTURE_PORT] = true;
			DEBUG_PRINT_LOW("\n STREAMON Successful \n ");
		}
	  }

      (*bufferHdr)->pBuffer = (OMX_U8*)drv_ctx.ptr_outputbuffer[i].bufferaddr;
      (*bufferHdr)->pAppPrivate = appData;
      BITMASK_SET(&m_out_bm_count,i);
    }
    else
    {
      DEBUG_PRINT_ERROR("All the Output Buffers have been Allocated ; Returning Insufficient \n");
      eRet = OMX_ErrorInsufficientResources;
    }
  }

  return eRet;
}


// AllocateBuffer  -- API Call
/* ======================================================================
FUNCTION
  omx_vdec::AllocateBuffer

DESCRIPTION
  Returns zero if all the buffers released..

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
OMX_ERRORTYPE  omx_vdec::allocate_buffer(OMX_IN OMX_HANDLETYPE                hComp,
                                     OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
                                     OMX_IN OMX_U32                        port,
                                     OMX_IN OMX_PTR                     appData,
                                     OMX_IN OMX_U32                       bytes)
{
    unsigned i = 0;
    OMX_ERRORTYPE eRet = OMX_ErrorNone; // OMX return type

    DEBUG_PRINT_LOW("\n Allocate buffer on port %d \n", (int)port);
    if(m_state == OMX_StateInvalid)
    {
        DEBUG_PRINT_ERROR("Allocate Buf in Invalid State\n");
        return OMX_ErrorInvalidState;
    }

    if(port == OMX_CORE_INPUT_PORT_INDEX)
    {
      if (arbitrary_bytes)
      {
          eRet = allocate_input_heap_buffer (hComp,bufferHdr,port,appData,bytes);
      }
      else
      {
        eRet = allocate_input_buffer(hComp,bufferHdr,port,appData,bytes);
      }
    }
    else if(port == OMX_CORE_OUTPUT_PORT_INDEX)
    {
      eRet = allocate_output_buffer(hComp,bufferHdr,port,appData,bytes);
    }
    else
    {
      DEBUG_PRINT_ERROR("Error: Invalid Port Index received %d\n",(int)port);
      eRet = OMX_ErrorBadPortIndex;
    }
    DEBUG_PRINT_LOW("Checking for Output Allocate buffer Done");
    if(eRet == OMX_ErrorNone)
    {
        if(allocate_done()){
            if(BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING))
            {
                // Send the callback now
                BITMASK_CLEAR((&m_flags),OMX_COMPONENT_IDLE_PENDING);
                post_event(OMX_CommandStateSet,OMX_StateIdle,
                                   OMX_COMPONENT_GENERATE_EVENT);
            }
        }
        if(port == OMX_CORE_INPUT_PORT_INDEX && m_inp_bPopulated)
        {
          if(BITMASK_PRESENT(&m_flags,OMX_COMPONENT_INPUT_ENABLE_PENDING))
          {
             BITMASK_CLEAR((&m_flags),OMX_COMPONENT_INPUT_ENABLE_PENDING);
             post_event(OMX_CommandPortEnable,
                        OMX_CORE_INPUT_PORT_INDEX,
                        OMX_COMPONENT_GENERATE_EVENT);
          }
        }
        if(port == OMX_CORE_OUTPUT_PORT_INDEX && m_out_bPopulated)
            {
          if(BITMASK_PRESENT(&m_flags,OMX_COMPONENT_OUTPUT_ENABLE_PENDING))
          {
             BITMASK_CLEAR((&m_flags),OMX_COMPONENT_OUTPUT_ENABLE_PENDING);
                post_event(OMX_CommandPortEnable,
                           OMX_CORE_OUTPUT_PORT_INDEX,
                           OMX_COMPONENT_GENERATE_EVENT);
            }
        }
    }
    DEBUG_PRINT_LOW("Allocate Buffer exit with ret Code %d\n",eRet);
    return eRet;
}

// Free Buffer - API call
/* ======================================================================
FUNCTION
  omx_vdec::FreeBuffer

DESCRIPTION

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
OMX_ERRORTYPE  omx_vdec::free_buffer(OMX_IN OMX_HANDLETYPE         hComp,
                                      OMX_IN OMX_U32                 port,
                                      OMX_IN OMX_BUFFERHEADERTYPE* buffer)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    unsigned int nPortIndex;
    DEBUG_PRINT_LOW("In for decoder free_buffer \n");

    if(m_state == OMX_StateIdle &&
       (BITMASK_PRESENT(&m_flags ,OMX_COMPONENT_LOADING_PENDING)))
    {
        DEBUG_PRINT_LOW(" free buffer while Component in Loading pending\n");
    }
    else if((m_inp_bEnabled == OMX_FALSE && port == OMX_CORE_INPUT_PORT_INDEX)||
            (m_out_bEnabled == OMX_FALSE && port == OMX_CORE_OUTPUT_PORT_INDEX))
    {
        DEBUG_PRINT_LOW("Free Buffer while port %d disabled\n", port);
    }
    else if(m_state == OMX_StateExecuting || m_state == OMX_StatePause)
    {
        DEBUG_PRINT_ERROR("Invalid state to free buffer,ports need to be disabled\n");
        post_event(OMX_EventError,
                   OMX_ErrorPortUnpopulated,
                   OMX_COMPONENT_GENERATE_EVENT);

        return OMX_ErrorIncorrectStateOperation;
    }
    else if (m_state != OMX_StateInvalid)
    {
        DEBUG_PRINT_ERROR("Invalid state to free buffer,port lost Buffers\n");
        post_event(OMX_EventError,
                   OMX_ErrorPortUnpopulated,
                   OMX_COMPONENT_GENERATE_EVENT);
    }

    if(port == OMX_CORE_INPUT_PORT_INDEX)
    {
      /*Check if arbitrary bytes*/
      if(!arbitrary_bytes && !input_use_buffer)
        nPortIndex = buffer - m_inp_mem_ptr;
      else
        nPortIndex = buffer - m_inp_heap_ptr;

        DEBUG_PRINT_LOW("free_buffer on i/p port - Port idx %d \n", nPortIndex);
        if(nPortIndex < drv_ctx.ip_buf.actualcount)
        {
         // Clear the bit associated with it.
         BITMASK_CLEAR(&m_inp_bm_count,nPortIndex);
         BITMASK_CLEAR(&m_heap_inp_bm_count,nPortIndex);
         if (input_use_buffer == true)
         {

            DEBUG_PRINT_LOW("\n Free pmem Buffer index %d",nPortIndex);
            if(m_phdr_pmem_ptr)
              free_input_buffer(m_phdr_pmem_ptr[nPortIndex]);
         }
         else
         {
            if (arbitrary_bytes)
            {
              if(m_phdr_pmem_ptr)
                free_input_buffer(nPortIndex,m_phdr_pmem_ptr[nPortIndex]);
              else
                free_input_buffer(nPortIndex,NULL);
            }
            else
              free_input_buffer(buffer);
         }
         m_inp_bPopulated = OMX_FALSE;
         /*Free the Buffer Header*/
          if (release_input_done())
          {
            DEBUG_PRINT_HIGH("\n ALL input buffers are freed/released");
            free_input_buffer_header();
          }
        }
        else
        {
            DEBUG_PRINT_ERROR("Error: free_buffer ,Port Index Invalid\n");
            eRet = OMX_ErrorBadPortIndex;
        }

        if(BITMASK_PRESENT((&m_flags),OMX_COMPONENT_INPUT_DISABLE_PENDING)
           && release_input_done())
        {
            DEBUG_PRINT_LOW("MOVING TO DISABLED STATE \n");
            BITMASK_CLEAR((&m_flags),OMX_COMPONENT_INPUT_DISABLE_PENDING);
            post_event(OMX_CommandPortDisable,
                       OMX_CORE_INPUT_PORT_INDEX,
                       OMX_COMPONENT_GENERATE_EVENT);
        }
    }
    else if(port == OMX_CORE_OUTPUT_PORT_INDEX)
    {
        // check if the buffer is valid
        nPortIndex = buffer - (OMX_BUFFERHEADERTYPE*)m_out_mem_ptr;
        if(nPortIndex < drv_ctx.op_buf.actualcount)
        {
            DEBUG_PRINT_LOW("free_buffer on o/p port - Port idx %d \n", nPortIndex);
            // Clear the bit associated with it.
            BITMASK_CLEAR(&m_out_bm_count,nPortIndex);
            m_out_bPopulated = OMX_FALSE;
            free_output_buffer (buffer);

            if (release_output_done())
            {
              free_output_buffer_header();
            }
        }
        else
        {
            DEBUG_PRINT_ERROR("Error: free_buffer , Port Index Invalid\n");
            eRet = OMX_ErrorBadPortIndex;
        }
        if(BITMASK_PRESENT((&m_flags),OMX_COMPONENT_OUTPUT_DISABLE_PENDING)
           && release_output_done())
        {
            DEBUG_PRINT_LOW("FreeBuffer : If any Disable event pending,post it\n");

                DEBUG_PRINT_LOW("MOVING TO DISABLED STATE \n");
                BITMASK_CLEAR((&m_flags),OMX_COMPONENT_OUTPUT_DISABLE_PENDING);

                post_event(OMX_CommandPortDisable,
                           OMX_CORE_OUTPUT_PORT_INDEX,
                           OMX_COMPONENT_GENERATE_EVENT);
        }
    }
    else
    {
        eRet = OMX_ErrorBadPortIndex;
    }
    if((eRet == OMX_ErrorNone) &&
       (BITMASK_PRESENT(&m_flags ,OMX_COMPONENT_LOADING_PENDING)))
    {
        if(release_done())
        {
            // Send the callback now
            BITMASK_CLEAR((&m_flags),OMX_COMPONENT_LOADING_PENDING);
            post_event(OMX_CommandStateSet, OMX_StateLoaded,
                                      OMX_COMPONENT_GENERATE_EVENT);
        }
    }
    return eRet;
}


/* ======================================================================
FUNCTION
  omx_vdec::EmptyThisBuffer

DESCRIPTION
  This routine is used to push the encoded video frames to
  the video decoder.

PARAMETERS
  None.

RETURN VALUE
  OMX Error None if everything went successful.

========================================================================== */
OMX_ERRORTYPE  omx_vdec::empty_this_buffer(OMX_IN OMX_HANDLETYPE         hComp,
                                           OMX_IN OMX_BUFFERHEADERTYPE* buffer)
{
  OMX_ERRORTYPE ret1 = OMX_ErrorNone;
  unsigned int nBufferIndex = drv_ctx.ip_buf.actualcount;

  if(m_state == OMX_StateInvalid)
  {
      DEBUG_PRINT_ERROR("Empty this buffer in Invalid State\n");
      return OMX_ErrorInvalidState;
  }

  if (buffer == NULL)
  {
    DEBUG_PRINT_ERROR("\nERROR:ETB Buffer is NULL");
    return OMX_ErrorBadParameter;
  }

  if (!m_inp_bEnabled)
  {
    DEBUG_PRINT_ERROR("\nERROR:ETB incorrect state operation, input port is disabled.");
    return OMX_ErrorIncorrectStateOperation;
  }

  if (buffer->nInputPortIndex != OMX_CORE_INPUT_PORT_INDEX)
  {
    DEBUG_PRINT_ERROR("\nERROR:ETB invalid port in header %d", buffer->nInputPortIndex);
    return OMX_ErrorBadPortIndex;
  }

#ifdef _ANDROID_
  if(iDivXDrmDecrypt)
  {
    OMX_ERRORTYPE drmErr = iDivXDrmDecrypt->Decrypt(buffer);
    if(drmErr != OMX_ErrorNone) {
        // this error can be ignored
        DEBUG_PRINT_LOW("\nERROR:iDivXDrmDecrypt->Decrypt %d", drmErr);
    }
  }
#endif //_ANDROID_
  if (perf_flag)
  {
    if (!latency)
    {
      dec_time.stop();
      latency = dec_time.processing_time_us();
      dec_time.start();
    }
  }

  if (arbitrary_bytes)
  {
    nBufferIndex = buffer - m_inp_heap_ptr;
  }
  else
  {
     if (input_use_buffer == true)
     {
       nBufferIndex = buffer - m_inp_heap_ptr;
       m_inp_mem_ptr[nBufferIndex].nFilledLen = m_inp_heap_ptr[nBufferIndex].nFilledLen;
       m_inp_mem_ptr[nBufferIndex].nTimeStamp = m_inp_heap_ptr[nBufferIndex].nTimeStamp;
       m_inp_mem_ptr[nBufferIndex].nFlags = m_inp_heap_ptr[nBufferIndex].nFlags;
       buffer = &m_inp_mem_ptr[nBufferIndex];
       DEBUG_PRINT_LOW("Non-Arbitrary mode - buffer address is: malloc %p, pmem%p in Index %d, buffer %p of size %d",
                         &m_inp_heap_ptr[nBufferIndex], &m_inp_mem_ptr[nBufferIndex],nBufferIndex, buffer, buffer->nFilledLen);
     }
     else{
       nBufferIndex = buffer - m_inp_mem_ptr;
     }
  }

  if (nBufferIndex > drv_ctx.ip_buf.actualcount )
  {
    DEBUG_PRINT_ERROR("\nERROR:ETB nBufferIndex is invalid");
    return OMX_ErrorBadParameter;
  }

  DEBUG_PRINT_LOW("[ETB] BHdr(%p) pBuf(%p) nTS(%lld) nFL(%lu)",
    buffer, buffer->pBuffer, buffer->nTimeStamp, buffer->nFilledLen);
  if (arbitrary_bytes)
  {
    post_event ((unsigned)hComp,(unsigned)buffer,
                OMX_COMPONENT_GENERATE_ETB_ARBITRARY);
  }
  else
  {
    if (!(client_extradata & OMX_TIMEINFO_EXTRADATA))
      set_frame_rate(buffer->nTimeStamp);
    post_event ((unsigned)hComp,(unsigned)buffer,OMX_COMPONENT_GENERATE_ETB);
  }
  return OMX_ErrorNone;
}

/* ======================================================================
FUNCTION
  omx_vdec::empty_this_buffer_proxy

DESCRIPTION
  This routine is used to push the encoded video frames to
  the video decoder.

PARAMETERS
  None.

RETURN VALUE
  OMX Error None if everything went successful.

========================================================================== */
OMX_ERRORTYPE  omx_vdec::empty_this_buffer_proxy(OMX_IN OMX_HANDLETYPE         hComp,
                                                 OMX_IN OMX_BUFFERHEADERTYPE* buffer)
{
  int push_cnt = 0,i=0;
  unsigned nPortIndex = 0;
  OMX_ERRORTYPE ret = OMX_ErrorNone;
  struct vdec_input_frameinfo frameinfo;
  struct vdec_bufferpayload *temp_buffer;
  struct vdec_ioctl_msg ioctl_msg;
  struct vdec_seqheader seq_header;
  bool port_setting_changed = true;
  bool not_coded_vop = false;

  /*Should we generate a Aync error event*/
  if (buffer == NULL || buffer->pInputPortPrivate == NULL)
  {
    DEBUG_PRINT_ERROR("\nERROR:empty_this_buffer_proxy is invalid");
    return OMX_ErrorBadParameter;
  }

  nPortIndex = buffer-((OMX_BUFFERHEADERTYPE *)m_inp_mem_ptr);

  if (nPortIndex > drv_ctx.ip_buf.actualcount)
  {
    DEBUG_PRINT_ERROR("\nERROR:empty_this_buffer_proxy invalid nPortIndex[%u]",
        nPortIndex);
    return OMX_ErrorBadParameter;
  }

  pending_input_buffers++;

  /* return zero length and not an EOS buffer */
  if (!arbitrary_bytes && (buffer->nFilledLen == 0) &&
     ((buffer->nFlags & OMX_BUFFERFLAG_EOS) == 0))
  {
    DEBUG_PRINT_HIGH("\n return zero legth buffer");
    post_event ((unsigned int)buffer,VDEC_S_SUCCESS,
                     OMX_COMPONENT_GENERATE_EBD);
    return OMX_ErrorNone;
  }


  if(codec_type_parse == CODEC_TYPE_MPEG4 || codec_type_parse == CODEC_TYPE_DIVX){
    mp4StreamType psBits;
    psBits.data = (unsigned char *)(buffer->pBuffer + buffer->nOffset);
    psBits.numBytes = buffer->nFilledLen;
    mp4_headerparser.parseHeader(&psBits);
    not_coded_vop = mp4_headerparser.is_notcodec_vop(
            (buffer->pBuffer + buffer->nOffset),buffer->nFilledLen);
    if(not_coded_vop) {
        DEBUG_PRINT_HIGH("\n Found Not coded vop len %d frame number %d",
             buffer->nFilledLen,frame_count);
        if(buffer->nFlags & OMX_BUFFERFLAG_EOS){
          DEBUG_PRINT_HIGH("\n Eos and Not coded Vop set len to zero");
          not_coded_vop = false;
          buffer->nFilledLen = 0;
        }
    }
  }

  if(input_flush_progress == true

     || not_coded_vop

     )
  {
    DEBUG_PRINT_LOW("\n Flush in progress return buffer ");
    post_event ((unsigned int)buffer,VDEC_S_SUCCESS,
                     OMX_COMPONENT_GENERATE_EBD);
    return OMX_ErrorNone;
  }

  temp_buffer = (struct vdec_bufferpayload *)buffer->pInputPortPrivate;

  if ((temp_buffer -  drv_ctx.ptr_inputbuffer) > drv_ctx.ip_buf.actualcount)
  {
    return OMX_ErrorBadParameter;
  }

  DEBUG_PRINT_LOW("\n ETBProxy: bufhdr = %p, bufhdr->pBuffer = %p", buffer, buffer->pBuffer);
  /*for use buffer we need to memcpy the data*/
  temp_buffer->buffer_len = buffer->nFilledLen;

  if (input_use_buffer)
  {
    if (buffer->nFilledLen <= temp_buffer->buffer_len)
    {
      if(arbitrary_bytes)
      {
        memcpy (temp_buffer->bufferaddr, (buffer->pBuffer + buffer->nOffset),buffer->nFilledLen);
      }
      else
      {
        memcpy (temp_buffer->bufferaddr, (m_inp_heap_ptr[nPortIndex].pBuffer + m_inp_heap_ptr[nPortIndex].nOffset),
                buffer->nFilledLen);
      }
    }
    else
    {
      return OMX_ErrorBadParameter;
    }

  }

  frameinfo.bufferaddr = temp_buffer->bufferaddr;
  frameinfo.client_data = (void *) buffer;
  frameinfo.datalen = temp_buffer->buffer_len;
  frameinfo.flags = 0;
  frameinfo.offset = buffer->nOffset;
  frameinfo.pmem_fd = temp_buffer->pmem_fd;
  frameinfo.pmem_offset = temp_buffer->offset;
  frameinfo.timestamp = buffer->nTimeStamp;
  if (drv_ctx.disable_dmx && m_desc_buffer_ptr && m_desc_buffer_ptr[nPortIndex].buf_addr)
  {
    DEBUG_PRINT_LOW("ETB: dmx enabled");
    if (m_demux_entries == 0)
    {
      extract_demux_addr_offsets(buffer);
    }

    DEBUG_PRINT_LOW("ETB: handle_demux_data - entries=%d",m_demux_entries);
    handle_demux_data(buffer);
    frameinfo.desc_addr = (OMX_U8 *)m_desc_buffer_ptr[nPortIndex].buf_addr;
    frameinfo.desc_size = m_desc_buffer_ptr[nPortIndex].desc_data_size;
  }
  else
  {
    frameinfo.desc_addr = NULL;
    frameinfo.desc_size = 0;
  }
  if(!arbitrary_bytes)
  {
      frameinfo.flags |= buffer->nFlags;
  }

#ifdef _ANDROID_
  if (m_debug_timestamp)
  {
    if(arbitrary_bytes)
    {
      DEBUG_PRINT_LOW("\n Inserting TIMESTAMP (%lld) into queue", buffer->nTimeStamp);
      m_timestamp_list.insert_ts(buffer->nTimeStamp);
    }
    else if(!arbitrary_bytes && !(buffer->nFlags & OMX_BUFFERFLAG_CODECCONFIG))
    {
      DEBUG_PRINT_LOW("\n Inserting TIMESTAMP (%lld) into queue", buffer->nTimeStamp);
      m_timestamp_list.insert_ts(buffer->nTimeStamp);
    }
  }
#endif

#ifdef INPUT_BUFFER_LOG
  if (inputBufferFile1)
  {
    fwrite((const char *)temp_buffer->bufferaddr,
      temp_buffer->buffer_len,1,inputBufferFile1);
  }
#endif

  if(buffer->nFlags & QOMX_VIDEO_BUFFERFLAG_EOSEQ)
  {
    frameinfo.flags |= QOMX_VIDEO_BUFFERFLAG_EOSEQ;
    buffer->nFlags &= ~QOMX_VIDEO_BUFFERFLAG_EOSEQ;
  }

  if (temp_buffer->buffer_len == 0 || (buffer->nFlags & OMX_BUFFERFLAG_EOS))
  {
    DEBUG_PRINT_HIGH("\n Rxd i/p EOS, Notify Driver that EOS has been reached");
    frameinfo.flags |= VDEC_BUFFERFLAG_EOS;
    h264_scratch.nFilledLen = 0;
    nal_count = 0;
    look_ahead_nal = false;
    frame_count = 0;
    if (m_frame_parser.mutils)
      m_frame_parser.mutils->initialize_frame_checking_environment();
    m_frame_parser.flush();
    h264_last_au_ts = LLONG_MAX;
    h264_last_au_flags = 0;
    memset(m_demux_offsets, 0, ( sizeof(OMX_U32) * 8192) );
    m_demux_entries = 0;
  }
    struct v4l2_buffer buf = {0};
	struct v4l2_plane plane;
	int rc;
	unsigned long  print_count;
  if (temp_buffer->buffer_len == 0 || (buffer->nFlags & OMX_BUFFERFLAG_EOS))
  {  buf.flags = V4L2_BUF_FLAG_EOS;
  printf("\n  INPUT EOS reached \n") ;
  }
	OMX_ERRORTYPE eRet = OMX_ErrorNone;
	buf.index = nPortIndex;
	buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	buf.memory = V4L2_MEMORY_USERPTR;
	plane.bytesused = temp_buffer->buffer_len;
	plane.length = drv_ctx.ip_buf.buffer_size;
	plane.m.userptr = (unsigned long)(temp_buffer->bufferaddr-temp_buffer->offset);
	plane.reserved[0] = temp_buffer->pmem_fd;
	plane.reserved[1] = temp_buffer->offset;
	plane.data_offset = 0;
	buf.m.planes = &plane;
	buf.length = 1;
	rc = ioctl(drv_ctx.video_driver_fd, VIDIOC_QBUF, &buf);
  if(!streaming[OUTPUT_PORT])
  {
	enum v4l2_buf_type buf_type;
	int ret,r;
	buf_type=V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        DEBUG_PRINT_LOW("send_command_proxy(): Idle-->Executing\n");
	ret=ioctl(drv_ctx.video_driver_fd, VIDIOC_STREAMON,&buf_type);
	if(!ret) {
		printf("Streamon on OUTPUT Plane was successful \n");
		streaming[OUTPUT_PORT] = true;
		ret = pthread_create(&async_thread_id,0,async_message_thread,this);
		if(ret < 0)
			printf("\n Failed to create async_message_thread \n");
	} else{
		/*TODO: How to handle this case */
		printf(" \n Failed to call streamon on OUTPUT \n");
	}
}
  DEBUG_PRINT_LOW("[ETBP] pBuf(%p) nTS(%lld) Sz(%d)",
    frameinfo.bufferaddr, frameinfo.timestamp, frameinfo.datalen);
      time_stamp_dts.insert_timestamp(buffer);

  return ret;
}

/* ======================================================================
FUNCTION
  omx_vdec::FillThisBuffer

DESCRIPTION
  IL client uses this method to release the frame buffer
  after displaying them.

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
OMX_ERRORTYPE  omx_vdec::fill_this_buffer(OMX_IN OMX_HANDLETYPE  hComp,
                                          OMX_IN OMX_BUFFERHEADERTYPE* buffer)
{

  if(m_state == OMX_StateInvalid)
  {
      DEBUG_PRINT_ERROR("FTB in Invalid State\n");
      return OMX_ErrorInvalidState;
  }

  if (!m_out_bEnabled)
  {
    DEBUG_PRINT_ERROR("\nERROR:FTB incorrect state operation, output port is disabled.");
    return OMX_ErrorIncorrectStateOperation;
  }

  if (buffer == NULL || ((buffer - m_out_mem_ptr) >= drv_ctx.op_buf.actualcount))
  {
    return OMX_ErrorBadParameter;
  }

  if (buffer->nOutputPortIndex != OMX_CORE_OUTPUT_PORT_INDEX)
  {
    DEBUG_PRINT_ERROR("\nERROR:FTB invalid port in header %d", buffer->nOutputPortIndex);
    return OMX_ErrorBadPortIndex;
  }

  DEBUG_PRINT_LOW("[FTB] bufhdr = %p, bufhdr->pBuffer = %p", buffer, buffer->pBuffer);
  post_event((unsigned) hComp, (unsigned)buffer,OMX_COMPONENT_GENERATE_FTB);
  return OMX_ErrorNone;
}
/* ======================================================================
FUNCTION
  omx_vdec::fill_this_buffer_proxy

DESCRIPTION
  IL client uses this method to release the frame buffer
  after displaying them.

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
OMX_ERRORTYPE  omx_vdec::fill_this_buffer_proxy(
                         OMX_IN OMX_HANDLETYPE        hComp,
                         OMX_IN OMX_BUFFERHEADERTYPE* bufferAdd)
{
  OMX_ERRORTYPE nRet = OMX_ErrorNone;
  struct vdec_ioctl_msg ioctl_msg = {NULL,NULL};
  OMX_BUFFERHEADERTYPE *buffer = bufferAdd;
  unsigned nPortIndex = 0;
  struct vdec_fillbuffer_cmd fillbuffer;
  struct vdec_bufferpayload     *ptr_outputbuffer = NULL;
  struct vdec_output_frameinfo  *ptr_respbuffer = NULL;

  nPortIndex = buffer-((OMX_BUFFERHEADERTYPE *)m_out_mem_ptr);

  if (bufferAdd == NULL || ((buffer - m_out_mem_ptr) >
      drv_ctx.op_buf.actualcount) )
    return OMX_ErrorBadParameter;

  DEBUG_PRINT_LOW("\n FTBProxy: bufhdr = %p, bufhdr->pBuffer = %p",
      bufferAdd, bufferAdd->pBuffer);
  /*Return back the output buffer to client*/
  if(m_out_bEnabled != OMX_TRUE || output_flush_progress == true)
  {
    DEBUG_PRINT_LOW("\n Output Buffers return flush/disable condition");
    buffer->nFilledLen = 0;
    m_cb.FillBufferDone (hComp,m_app_data,buffer);
    return OMX_ErrorNone;
  }
  pending_output_buffers++;
  ptr_respbuffer = (struct vdec_output_frameinfo*)buffer->pOutputPortPrivate;
  if (ptr_respbuffer)
  {
    ptr_outputbuffer =  (struct vdec_bufferpayload*)ptr_respbuffer->client_data;
  }

  if (ptr_respbuffer == NULL || ptr_outputbuffer == NULL)
  {
      DEBUG_PRINT_ERROR("resp buffer or outputbuffer is NULL");
      buffer->nFilledLen = 0;
      m_cb.FillBufferDone (hComp,m_app_data,buffer);
      pending_output_buffers--;
      return OMX_ErrorBadParameter;
  }

 // memcpy (&fillbuffer.buffer,ptr_outputbuffer,\
          sizeof(struct vdec_bufferpayload));
 // fillbuffer.client_data = bufferAdd;

	int rc = 0;
	struct v4l2_buffer buf={0};
	struct v4l2_plane plane;

	buf.index = nPortIndex;
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	buf.memory = V4L2_MEMORY_USERPTR;
	plane.bytesused = buffer->nFilledLen;
	plane.length = drv_ctx.op_buf.buffer_size;
	plane.m.userptr = (unsigned long)(drv_ctx.ptr_outputbuffer[nPortIndex].bufferaddr-drv_ctx.ptr_outputbuffer[nPortIndex].offset);
	plane.reserved[0] = drv_ctx.ptr_outputbuffer[nPortIndex].pmem_fd;
	plane.reserved[1] = drv_ctx.ptr_outputbuffer[nPortIndex].offset;
	plane.data_offset = 0;
	buf.m.planes = &plane;
	buf.length = 1;

	rc = ioctl(drv_ctx.video_driver_fd, VIDIOC_QBUF, &buf);
	if (rc) {
		/*TODO: How to handle this case */
		printf("Failed to qbuf to driver");
	}
    //m_cb.FillBufferDone (hComp,m_app_data,buffer);
   // pending_output_buffers--;
   // return OMX_ErrorBadParameter;
  //}
  return OMX_ErrorNone;
}

/* ======================================================================
FUNCTION
  omx_vdec::SetCallbacks

DESCRIPTION
  Set the callbacks.

PARAMETERS
  None.

RETURN VALUE
  OMX Error None if everything successful.

========================================================================== */
OMX_ERRORTYPE  omx_vdec::set_callbacks(OMX_IN OMX_HANDLETYPE        hComp,
                                           OMX_IN OMX_CALLBACKTYPE* callbacks,
                                           OMX_IN OMX_PTR             appData)
{

  m_cb       = *callbacks;
  DEBUG_PRINT_LOW("\n Callbacks Set %p %p %p",m_cb.EmptyBufferDone,\
               m_cb.EventHandler,m_cb.FillBufferDone);
  m_app_data =    appData;
  return OMX_ErrorNotImplemented;
}

/* ======================================================================
FUNCTION
  omx_vdec::ComponentDeInit

DESCRIPTION
  Destroys the component and release memory allocated to the heap.

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if everything successful.

========================================================================== */
OMX_ERRORTYPE  omx_vdec::component_deinit(OMX_IN OMX_HANDLETYPE hComp)
{
#ifdef _ANDROID_
    if(iDivXDrmDecrypt)
    {
        delete iDivXDrmDecrypt;
        iDivXDrmDecrypt=NULL;
    }
#endif //_ANDROID_

    int i = 0;
    if (OMX_StateLoaded != m_state)
    {
        DEBUG_PRINT_ERROR("WARNING:Rxd DeInit,OMX not in LOADED state %d\n",\
                          m_state);
        DEBUG_PRINT_ERROR("\nPlayback Ended - FAILED");
    }
    else
    {
      DEBUG_PRINT_HIGH("\n Playback Ended - PASSED");
    }

    /*Check if the output buffers have to be cleaned up*/
    if(m_out_mem_ptr)
    {
        DEBUG_PRINT_LOW("Freeing the Output Memory\n");
        for (i=0; i < drv_ctx.op_buf.actualcount; i++ )
        {
          free_output_buffer (&m_out_mem_ptr[i]);
        }
    }

    /*Check if the input buffers have to be cleaned up*/
    if(m_inp_mem_ptr || m_inp_heap_ptr)
    {
        DEBUG_PRINT_LOW("Freeing the Input Memory\n");
        for (i=0; i<drv_ctx.ip_buf.actualcount; i++ )
        {
          if (m_inp_mem_ptr)
            free_input_buffer (i,&m_inp_mem_ptr[i]);
          else
            free_input_buffer (i,NULL);
        }
    }
    free_input_buffer_header();
    free_output_buffer_header();
    if(h264_scratch.pBuffer)
    {
        free(h264_scratch.pBuffer);
        h264_scratch.pBuffer = NULL;
    }

    if (h264_parser)
    {
        delete h264_parser;
	h264_parser = NULL;
    }

    if(m_platform_list)
    {
        free(m_platform_list);
        m_platform_list = NULL;
    }
    if(m_vendor_config.pData)
    {
        free(m_vendor_config.pData);
        m_vendor_config.pData = NULL;
    }

    // Reset counters in mesg queues
    m_ftb_q.m_size=0;
    m_cmd_q.m_size=0;
    m_etb_q.m_size=0;
    m_ftb_q.m_read = m_ftb_q.m_write =0;
    m_cmd_q.m_read = m_cmd_q.m_write =0;
    m_etb_q.m_read = m_etb_q.m_write =0;
#ifdef _ANDROID_
    if (m_debug_timestamp)
    {
      m_timestamp_list.reset_ts_list();
    }
#endif

    DEBUG_PRINT_LOW("\n Calling VDEC_IOCTL_STOP_NEXT_MSG");
    //(void)ioctl(drv_ctx.video_driver_fd, VDEC_IOCTL_STOP_NEXT_MSG,
       // NULL);
    DEBUG_PRINT_HIGH("\n Close the driver instance");

#ifdef INPUT_BUFFER_LOG
    fclose (inputBufferFile1);
#endif
#ifdef OUTPUT_BUFFER_LOG
    fclose (outputBufferFile1);
#endif
#ifdef OUTPUT_EXTRADATA_LOG
    fclose (outputExtradataFile);
#endif
  DEBUG_PRINT_HIGH("\n omx_vdec::component_deinit() complete");
  return OMX_ErrorNone;
}

/* ======================================================================
FUNCTION
  omx_vdec::UseEGLImage

DESCRIPTION
  OMX Use EGL Image method implementation <TBD>.

PARAMETERS
  <TBD>.

RETURN VALUE
  Not Implemented error.

========================================================================== */
OMX_ERRORTYPE  omx_vdec::use_EGL_image(OMX_IN OMX_HANDLETYPE                hComp,
                                          OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
                                          OMX_IN OMX_U32                        port,
                                          OMX_IN OMX_PTR                     appData,
                                          OMX_IN void*                      eglImage)
{
  OMX_QCOM_PLATFORM_PRIVATE_LIST pmem_list;
  OMX_QCOM_PLATFORM_PRIVATE_ENTRY pmem_entry;
  OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO pmem_info;

#ifdef USE_EGL_IMAGE_GPU
   PFNEGLQUERYIMAGEQUALCOMMPROC egl_queryfunc;
   EGLint fd = -1, offset = 0,pmemPtr = 0;
#else
   int fd = -1, offset = 0;
#endif
   DEBUG_PRINT_HIGH("\nuse EGL image support for decoder");
   if (!bufferHdr || !eglImage|| port != OMX_CORE_OUTPUT_PORT_INDEX) {
     DEBUG_PRINT_ERROR("\n ");
   }
#ifdef USE_EGL_IMAGE_GPU
   if(m_display_id == NULL) {
        DEBUG_PRINT_ERROR("Display ID is not set by IL client \n");
        return OMX_ErrorInsufficientResources;
   }
   egl_queryfunc = (PFNEGLQUERYIMAGEQUALCOMMPROC)
                    eglGetProcAddress("eglQueryImageKHR");
   egl_queryfunc(m_display_id, eglImage, EGL_BUFFER_HANDLE_QCOM,&fd);
   egl_queryfunc(m_display_id, eglImage, EGL_BUFFER_OFFSET_QCOM,&offset);
   egl_queryfunc(m_display_id, eglImage, EGL_BITMAP_POINTER_KHR,&pmemPtr);
#else //with OMX test app
    struct temp_egl {
        int pmem_fd;
        int offset;
    };
    struct temp_egl *temp_egl_id = NULL;
    void * pmemPtr = (void *) eglImage;
    temp_egl_id = (struct temp_egl *)eglImage;
    if (temp_egl_id != NULL)
    {
        fd = temp_egl_id->pmem_fd;
        offset = temp_egl_id->offset;
    }
#endif
    if (fd < 0) {
        DEBUG_PRINT_ERROR("Improper pmem fd by EGL client %d  \n",fd);
        return OMX_ErrorInsufficientResources;
   }
   pmem_info.pmem_fd = (OMX_U32) fd;
   pmem_info.offset = (OMX_U32) offset;
   pmem_entry.entry = (void *) &pmem_info;
   pmem_entry.type = OMX_QCOM_PLATFORM_PRIVATE_PMEM;
   pmem_list.entryList = &pmem_entry;
   pmem_list.nEntries = 1;
   ouput_egl_buffers = true;
   if (OMX_ErrorNone != use_buffer(hComp,bufferHdr, port,
       (void *)&pmem_list, drv_ctx.op_buf.buffer_size,
        (OMX_U8 *)pmemPtr)) {
     DEBUG_PRINT_ERROR("use buffer call failed for egl image\n");
     return OMX_ErrorInsufficientResources;
   }
   return OMX_ErrorNone;
}

/* ======================================================================
FUNCTION
  omx_vdec::ComponentRoleEnum

DESCRIPTION
  OMX Component Role Enum method implementation.

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if everything is successful.
========================================================================== */
OMX_ERRORTYPE  omx_vdec::component_role_enum(OMX_IN OMX_HANDLETYPE hComp,
                                                OMX_OUT OMX_U8*        role,
                                                OMX_IN OMX_U32        index)
{
  OMX_ERRORTYPE eRet = OMX_ErrorNone;

  if(!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.mpeg4",OMX_MAX_STRINGNAME_SIZE))
  {
    if((0 == index) && role)
    {
      strlcpy((char *)role, "video_decoder.mpeg4",OMX_MAX_STRINGNAME_SIZE);
      DEBUG_PRINT_LOW("component_role_enum: role %s\n",role);
    }
    else
    {
      eRet = OMX_ErrorNoMore;
    }
  }
  if(!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.mpeg2",OMX_MAX_STRINGNAME_SIZE))
  {
    if((0 == index) && role)
    {
      strlcpy((char *)role, "video_decoder.mpeg2",OMX_MAX_STRINGNAME_SIZE);
      DEBUG_PRINT_LOW("component_role_enum: role %s\n",role);
    }
    else
    {
      eRet = OMX_ErrorNoMore;
    }
  }
  else if(!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.h263",OMX_MAX_STRINGNAME_SIZE))
  {
    if((0 == index) && role)
    {
      strlcpy((char *)role, "video_decoder.h263",OMX_MAX_STRINGNAME_SIZE);
      DEBUG_PRINT_LOW("component_role_enum: role %s\n",role);
    }
    else
    {
      DEBUG_PRINT_LOW("\n No more roles \n");
      eRet = OMX_ErrorNoMore;
    }
  }

  else if((!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.divx",OMX_MAX_STRINGNAME_SIZE)) ||
          (!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.divx311",OMX_MAX_STRINGNAME_SIZE))
          )

  {
    if((0 == index) && role)
    {
      strlcpy((char *)role, "video_decoder.divx",OMX_MAX_STRINGNAME_SIZE);
      DEBUG_PRINT_LOW("component_role_enum: role %s\n",role);
    }
    else
    {
      DEBUG_PRINT_LOW("\n No more roles \n");
      eRet = OMX_ErrorNoMore;
    }
  }
  else if(!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.avc",OMX_MAX_STRINGNAME_SIZE))
  {
    if((0 == index) && role)
    {
      strlcpy((char *)role, "video_decoder.avc",OMX_MAX_STRINGNAME_SIZE);
      DEBUG_PRINT_LOW("component_role_enum: role %s\n",role);
    }
    else
    {
      DEBUG_PRINT_LOW("\n No more roles \n");
      eRet = OMX_ErrorNoMore;
    }
  }
  else if( (!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.vc1",OMX_MAX_STRINGNAME_SIZE)) ||
           (!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.wmv",OMX_MAX_STRINGNAME_SIZE))
           )
  {
    if((0 == index) && role)
    {
      strlcpy((char *)role, "video_decoder.vc1",OMX_MAX_STRINGNAME_SIZE);
      DEBUG_PRINT_LOW("component_role_enum: role %s\n",role);
    }
    else
    {
      DEBUG_PRINT_LOW("\n No more roles \n");
      eRet = OMX_ErrorNoMore;
    }
  }
  else
  {
    DEBUG_PRINT_ERROR("\nERROR:Querying Role on Unknown Component\n");
    eRet = OMX_ErrorInvalidComponentName;
  }
  return eRet;
}




/* ======================================================================
FUNCTION
  omx_vdec::AllocateDone

DESCRIPTION
  Checks if entire buffer pool is allocated by IL Client or not.
  Need this to move to IDLE state.

PARAMETERS
  None.

RETURN VALUE
  true/false.

========================================================================== */
bool omx_vdec::allocate_done(void)
{
  bool bRet = false;
  bool bRet_In = false;
  bool bRet_Out = false;

  bRet_In = allocate_input_done();
  bRet_Out = allocate_output_done();

  if(bRet_In && bRet_Out)
  {
      bRet = true;
  }

  return bRet;
}
/* ======================================================================
FUNCTION
  omx_vdec::AllocateInputDone

DESCRIPTION
  Checks if I/P buffer pool is allocated by IL Client or not.

PARAMETERS
  None.

RETURN VALUE
  true/false.

========================================================================== */
bool omx_vdec::allocate_input_done(void)
{
  bool bRet = false;
  unsigned i=0;

  if (m_inp_mem_ptr == NULL)
  {
      return bRet;
  }
  if(m_inp_mem_ptr )
  {
    for(;i<drv_ctx.ip_buf.actualcount;i++)
    {
      if(BITMASK_ABSENT(&m_inp_bm_count,i))
      {
        break;
      }
    }
  }
  if(i == drv_ctx.ip_buf.actualcount)
  {
    bRet = true;
    DEBUG_PRINT_HIGH("Allocate done for all i/p buffers");
  }
  if(i==drv_ctx.ip_buf.actualcount && m_inp_bEnabled)
  {
     m_inp_bPopulated = OMX_TRUE;
  }
  return bRet;
}
/* ======================================================================
FUNCTION
  omx_vdec::AllocateOutputDone

DESCRIPTION
  Checks if entire O/P buffer pool is allocated by IL Client or not.

PARAMETERS
  None.

RETURN VALUE
  true/false.

========================================================================== */
bool omx_vdec::allocate_output_done(void)
{
  bool bRet = false;
  unsigned j=0;

  if (m_out_mem_ptr == NULL)
  {
      return bRet;
  }

  if (m_out_mem_ptr)
  {
    for(;j < drv_ctx.op_buf.actualcount;j++)
    {
      if(BITMASK_ABSENT(&m_out_bm_count,j))
      {
        break;
      }
    }
  }

  if(j == drv_ctx.op_buf.actualcount)
  {
    bRet = true;
    DEBUG_PRINT_HIGH("Allocate done for all o/p buffers");
    if(m_out_bEnabled)
       m_out_bPopulated = OMX_TRUE;
  }

  return bRet;
}

/* ======================================================================
FUNCTION
  omx_vdec::ReleaseDone

DESCRIPTION
  Checks if IL client has released all the buffers.

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
bool omx_vdec::release_done(void)
{
  bool bRet = false;

  if(release_input_done())
  {
    if(release_output_done())
    {
        bRet = true;
    }
  }
  return bRet;
}


/* ======================================================================
FUNCTION
  omx_vdec::ReleaseOutputDone

DESCRIPTION
  Checks if IL client has released all the buffers.

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
bool omx_vdec::release_output_done(void)
{
  bool bRet = false;
  unsigned i=0,j=0;

  DEBUG_PRINT_LOW("\n Value of m_out_mem_ptr %p",m_inp_mem_ptr);
  if(m_out_mem_ptr)
  {
      for(;j < drv_ctx.op_buf.actualcount ; j++)
      {
        if(BITMASK_PRESENT(&m_out_bm_count,j))
        {
          break;
        }
      }
    if(j == drv_ctx.op_buf.actualcount)
    {
      m_out_bm_count = 0;
      bRet = true;
    }
  }
  else
  {
    m_out_bm_count = 0;
    bRet = true;
  }
  return bRet;
}
/* ======================================================================
FUNCTION
  omx_vdec::ReleaseInputDone

DESCRIPTION
  Checks if IL client has released all the buffers.

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
bool omx_vdec::release_input_done(void)
{
  bool bRet = false;
  unsigned i=0,j=0;

  DEBUG_PRINT_LOW("\n Value of m_inp_mem_ptr %p",m_inp_mem_ptr);
  if(m_inp_mem_ptr)
  {
      for(;j<drv_ctx.ip_buf.actualcount;j++)
      {
        if( BITMASK_PRESENT(&m_inp_bm_count,j))
        {
          break;
        }
      }
    if(j==drv_ctx.ip_buf.actualcount)
    {
      bRet = true;
    }
  }
  else
  {
    bRet = true;
  }
  return bRet;
}

OMX_ERRORTYPE omx_vdec::fill_buffer_done(OMX_HANDLETYPE hComp,
                               OMX_BUFFERHEADERTYPE * buffer)
{
  OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO *pPMEMInfo = NULL;
  if (!buffer || (buffer - m_out_mem_ptr) >= drv_ctx.op_buf.actualcount)
  {
    DEBUG_PRINT_ERROR("\n [FBD] ERROR in ptr(%p)", buffer);
    return OMX_ErrorBadParameter;
  }
  else if (output_flush_progress)
  {
    DEBUG_PRINT_LOW("FBD: Buffer (%p) flushed", buffer);
    buffer->nFilledLen = 0;
    buffer->nTimeStamp = 0;
    buffer->nFlags &= ~OMX_BUFFERFLAG_EXTRADATA;
    buffer->nFlags &= ~QOMX_VIDEO_BUFFERFLAG_EOSEQ;
    buffer->nFlags &= ~OMX_BUFFERFLAG_DATACORRUPT;
  }

  DEBUG_PRINT_LOW("\n fill_buffer_done: bufhdr = %p, bufhdr->pBuffer = %p",
      buffer, buffer->pBuffer);
  pending_output_buffers --;

  if (buffer->nFlags & OMX_BUFFERFLAG_EOS)
  {
    DEBUG_PRINT_HIGH("\n Output EOS has been reached");
    if (!output_flush_progress)
      post_event(NULL,NULL,OMX_COMPONENT_GENERATE_EOS_DONE);

    if (psource_frame)
    {
      m_cb.EmptyBufferDone(&m_cmp, m_app_data, psource_frame);
      psource_frame = NULL;
    }
    if (pdest_frame)
    {
      pdest_frame->nFilledLen = 0;
      m_input_free_q.insert_entry((unsigned) pdest_frame,NULL,NULL);
      pdest_frame = NULL;
    }
  }

  DEBUG_PRINT_LOW("\n In fill Buffer done call address %p ",buffer);
#ifdef OUTPUT_BUFFER_LOG
  if (outputBufferFile1)
  {
    fwrite (buffer->pBuffer,1,buffer->nFilledLen,
                  outputBufferFile1);
  }
#endif

  /* For use buffer we need to copy the data */
  if (!output_flush_progress)
  {
    time_stamp_dts.get_next_timestamp(buffer,
    (drv_ctx.interlace != VDEC_InterlaceFrameProgressive)
     ?true:false);
  }
  if (m_cb.FillBufferDone)
  {
    if (buffer->nFilledLen > 0)
    {
      if (client_extradata)
        handle_extradata(buffer);
      if (client_extradata & OMX_TIMEINFO_EXTRADATA)
        // Keep min timestamp interval to handle corrupted bit stream scenario
        set_frame_rate(buffer->nTimeStamp);
      else if (arbitrary_bytes)
        adjust_timestamp(buffer->nTimeStamp);
      if (perf_flag)
      {
        if (!proc_frms)
        {
          dec_time.stop();
          latency = dec_time.processing_time_us() - latency;
          DEBUG_PRINT_HIGH(">>> FBD Metrics: Latency(%.2f)mS", latency / 1e3);
          dec_time.start();
          fps_metrics.start();
        }
        proc_frms++;
        if (buffer->nFlags & OMX_BUFFERFLAG_EOS)
        {
          OMX_U64 proc_time = 0;
          fps_metrics.stop();
          proc_time = fps_metrics.processing_time_us();
          DEBUG_PRINT_HIGH(">>> FBD Metrics: proc_frms(%lu) proc_time(%.2f)S fps(%.2f)",
                            proc_frms, (float)proc_time / 1e6,
                            (float)(1e6 * proc_frms) / proc_time);
          proc_frms = 0;
        }
      }

#ifdef OUTPUT_EXTRADATA_LOG
  if (outputExtradataFile)
  {

    OMX_OTHER_EXTRADATATYPE *p_extra = NULL;
    p_extra = (OMX_OTHER_EXTRADATATYPE *)
           ((unsigned)(buffer->pBuffer + buffer->nOffset +
            buffer->nFilledLen + 3)&(~3));
    while(p_extra &&
          (OMX_U8*)p_extra < (buffer->pBuffer + buffer->nAllocLen) )
    {
      DEBUG_PRINT_LOW("\nWRITING extradata, size=%d,type=%d",p_extra->nSize, p_extra->eType);
      fwrite (p_extra,1,p_extra->nSize,outputExtradataFile);
      if (p_extra->eType == OMX_ExtraDataNone)
      {
        break;
      }
      p_extra = (OMX_OTHER_EXTRADATATYPE *) (((OMX_U8 *) p_extra) + p_extra->nSize);
    }
  }
#endif
    }
    if (buffer->nFlags & OMX_BUFFERFLAG_EOS){
      prev_ts = LLONG_MAX;
      rst_prev_ts = true;
      }

    pPMEMInfo = (OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO *)
                ((OMX_QCOM_PLATFORM_PRIVATE_LIST *)
                buffer->pPlatformPrivate)->entryList->entry;
    DEBUG_PRINT_LOW("\n Before FBD callback Accessed Pmeminfo %d",pPMEMInfo->pmem_fd);
    m_cb.FillBufferDone (hComp,m_app_data,buffer);
    DEBUG_PRINT_LOW("\n After Fill Buffer Done callback %d",pPMEMInfo->pmem_fd);
  }
  else
  {
    return OMX_ErrorBadParameter;
  }

  return OMX_ErrorNone;
}

OMX_ERRORTYPE omx_vdec::empty_buffer_done(OMX_HANDLETYPE         hComp,
                                          OMX_BUFFERHEADERTYPE* buffer)
{

    if (buffer == NULL || ((buffer - m_inp_mem_ptr) > drv_ctx.ip_buf.actualcount))
    {
        DEBUG_PRINT_ERROR("\n empty_buffer_done: ERROR bufhdr = %p", buffer);
       return OMX_ErrorBadParameter;
    }

    DEBUG_PRINT_LOW("\n empty_buffer_done: bufhdr = %p, bufhdr->pBuffer = %p",
        buffer, buffer->pBuffer);
    pending_input_buffers--;

    if (arbitrary_bytes)
    {
      if (pdest_frame == NULL && input_flush_progress == false)
      {
        DEBUG_PRINT_LOW("\n Push input from buffer done address of Buffer %p",buffer);
        pdest_frame = buffer;
        buffer->nFilledLen = 0;
        buffer->nTimeStamp = LLONG_MAX;
        push_input_buffer (hComp);
      }
      else
      {
        DEBUG_PRINT_LOW("\n Push buffer into freeq address of Buffer %p",buffer);
        buffer->nFilledLen = 0;
        if (!m_input_free_q.insert_entry((unsigned)buffer,NULL,NULL))
        {
          DEBUG_PRINT_ERROR("\nERROR:i/p free Queue is FULL Error");
        }
      }
    }
    else if(m_cb.EmptyBufferDone)
    {
        buffer->nFilledLen = 0;
        if (input_use_buffer == true){
            buffer = &m_inp_heap_ptr[buffer-m_inp_mem_ptr];
        }
        m_cb.EmptyBufferDone(hComp ,m_app_data, buffer);
    }
    return OMX_ErrorNone;
}


int omx_vdec::async_message_process (void *context, void* message)
{
  omx_vdec* omx = NULL;
  struct vdec_msginfo *vdec_msg = NULL;
  OMX_BUFFERHEADERTYPE* omxhdr = NULL;
  struct v4l2_buffer *v4l2_buf_ptr=NULL;
  struct vdec_output_frameinfo *output_respbuf = NULL;
	int rc=1;
  if (context == NULL || message == NULL)
  {
    DEBUG_PRINT_ERROR("\n FATAL ERROR in omx_vdec::async_message_process NULL Check");
    return -1;
  }
  vdec_msg = (struct vdec_msginfo *)message;

  omx = reinterpret_cast<omx_vdec*>(context);

#ifdef _ANDROID_
  if (omx->m_debug_timestamp)
  {
    if ( (vdec_msg->msgcode == VDEC_MSG_RESP_OUTPUT_BUFFER_DONE) &&
         !(omx->output_flush_progress) )
    {
      OMX_TICKS expected_ts = 0;
      omx->m_timestamp_list.pop_min_ts(expected_ts);
      DEBUG_PRINT_LOW("\n Current timestamp (%lld),Popped TIMESTAMP (%lld) from list",
                       vdec_msg->msgdata.output_frame.time_stamp, expected_ts);

      if (vdec_msg->msgdata.output_frame.time_stamp != expected_ts)
      {
        DEBUG_PRINT_ERROR("\n ERROR in omx_vdec::async_message_process timestamp Check");
      }
    }
  }
#endif

  switch (vdec_msg->msgcode)
  {

  case VDEC_MSG_EVT_HW_ERROR:
    omx->post_event (NULL,vdec_msg->status_code,\
                     OMX_COMPONENT_GENERATE_HARDWARE_ERROR);
  break;

  case VDEC_MSG_RESP_START_DONE:
    omx->post_event (NULL,vdec_msg->status_code,\
                     OMX_COMPONENT_GENERATE_START_DONE);
  break;

  case VDEC_MSG_RESP_STOP_DONE:
    omx->post_event (NULL,vdec_msg->status_code,\
                     OMX_COMPONENT_GENERATE_STOP_DONE);
  break;

  case VDEC_MSG_RESP_RESUME_DONE:
    omx->post_event (NULL,vdec_msg->status_code,\
                     OMX_COMPONENT_GENERATE_RESUME_DONE);
  break;

  case VDEC_MSG_RESP_PAUSE_DONE:
    omx->post_event (NULL,vdec_msg->status_code,\
                     OMX_COMPONENT_GENERATE_PAUSE_DONE);
  break;

  case VDEC_MSG_RESP_FLUSH_INPUT_DONE:
    omx->post_event (NULL,vdec_msg->status_code,\
                     OMX_COMPONENT_GENERATE_EVENT_INPUT_FLUSH);
    break;
  case VDEC_MSG_RESP_FLUSH_OUTPUT_DONE:
    omx->post_event (NULL,vdec_msg->status_code,\
                     OMX_COMPONENT_GENERATE_EVENT_OUTPUT_FLUSH);
    break;
  case VDEC_MSG_RESP_INPUT_FLUSHED:
  case VDEC_MSG_RESP_INPUT_BUFFER_DONE:

    // omxhdr = (OMX_BUFFERHEADERTYPE* )		\
      //        vdec_msg->msgdata.input_frame_clientdata;

    v4l2_buf_ptr = (v4l2_buffer*)vdec_msg->msgdata.input_frame_clientdata;
    omxhdr=omx->m_inp_mem_ptr+v4l2_buf_ptr->index;
    if (omxhdr == NULL ||
       ((omxhdr - omx->m_inp_mem_ptr) > omx->drv_ctx.ip_buf.actualcount) )
    {
       omxhdr = NULL;
       vdec_msg->status_code = VDEC_S_EFATAL;
    }

    omx->post_event ((unsigned int)omxhdr,vdec_msg->status_code,
                     OMX_COMPONENT_GENERATE_EBD);
    break;
    case VDEC_MSG_EVT_INFO_FIELD_DROPPED:
      int64_t *timestamp;
      timestamp = (int64_t *) malloc(sizeof(int64_t));
      if (timestamp) {
        *timestamp = vdec_msg->msgdata.output_frame.time_stamp;
        omx->post_event ((unsigned int)timestamp, vdec_msg->status_code,
                         OMX_COMPONENT_GENERATE_INFO_FIELD_DROPPED);
        DEBUG_PRINT_HIGH("\nField dropped time stamp is %lld",
             vdec_msg->msgdata.output_frame.time_stamp);
      }
      break;
  case VDEC_MSG_RESP_OUTPUT_FLUSHED:
    case VDEC_MSG_RESP_OUTPUT_BUFFER_DONE:

      v4l2_buf_ptr = (v4l2_buffer*)vdec_msg->msgdata.output_frame.client_data;
      omxhdr=omx->m_out_mem_ptr+v4l2_buf_ptr->index;
      DEBUG_PRINT_LOW("[RespBufDone] Buf(%p) Ts(%lld) Pic_type(%u)",
		      omxhdr, vdec_msg->msgdata.output_frame.time_stamp,
		      vdec_msg->msgdata.output_frame.pic_type);

    if (omxhdr && omxhdr->pOutputPortPrivate &&
        ((omxhdr - omx->m_out_mem_ptr) < omx->drv_ctx.op_buf.actualcount) &&
         (((struct vdec_output_frameinfo *)omxhdr->pOutputPortPrivate
            - omx->drv_ctx.ptr_respbuffer) < omx->drv_ctx.op_buf.actualcount))
    {
      if ( vdec_msg->msgdata.output_frame.len <=  omxhdr->nAllocLen)
      {
	omxhdr->nFilledLen = vdec_msg->msgdata.output_frame.len;
	omxhdr->nOffset = vdec_msg->msgdata.output_frame.offset;
        omxhdr->nTimeStamp = omx->m_out_mem_ptr[v4l2_buf_ptr->index].nTimeStamp;
        omxhdr->nFlags = omx->m_out_mem_ptr[v4l2_buf_ptr->index].nFlags;

	if (v4l2_buf_ptr->flags & V4L2_BUF_FLAG_EOS)
	{
	  omxhdr->nFlags |= OMX_BUFFERFLAG_EOS;
	  //rc = -1;
	}
	vdec_msg->msgdata.output_frame.bufferaddr=omx->drv_ctx.ptr_outputbuffer[v4l2_buf_ptr->index].bufferaddr;
        output_respbuf = (struct vdec_output_frameinfo *)\
                          omxhdr->pOutputPortPrivate;
	// output_respbuf->framesize.bottom =
        //  vdec_msg->msgdata.output_frame.framesize.bottom;
	// output_respbuf->framesize.left =
	//   vdec_msg->msgdata.output_frame.framesize.left;
	// output_respbuf->framesize.right =
	//   vdec_msg->msgdata.output_frame.framesize.right;
	// output_respbuf->framesize.top =
	//   vdec_msg->msgdata.output_frame.framesize.top;
        output_respbuf->len = vdec_msg->msgdata.output_frame.len;
        output_respbuf->offset = vdec_msg->msgdata.output_frame.offset;
	// output_respbuf->time_stamp = vdec_msg->msgdata.output_frame.time_stamp;
	// output_respbuf->flags = vdec_msg->msgdata.output_frame.flags;
	// output_respbuf->pic_type = vdec_msg->msgdata.output_frame.pic_type;
	// output_respbuf->interlaced_format = vdec_msg->msgdata.output_frame.interlaced_format;

        if (omx->output_use_buffer)
          memcpy ( omxhdr->pBuffer,
                   (vdec_msg->msgdata.output_frame.bufferaddr +
                    vdec_msg->msgdata.output_frame.offset),
                    vdec_msg->msgdata.output_frame.len );
      }
      else
        omxhdr->nFilledLen = 0;
      omx->post_event ((unsigned int)omxhdr, vdec_msg->status_code,
                       OMX_COMPONENT_GENERATE_FBD);
    }
    else if (vdec_msg->msgdata.output_frame.flags & OMX_BUFFERFLAG_EOS)
      omx->post_event (NULL, vdec_msg->status_code,
                       OMX_COMPONENT_GENERATE_EOS_DONE);
    else
      omx->post_event (NULL, vdec_msg->status_code,
                       OMX_COMPONENT_GENERATE_HARDWARE_ERROR);
    break;
  case VDEC_MSG_EVT_CONFIG_CHANGED:
    DEBUG_PRINT_HIGH("\n Port settings changed");
	omx->post_event ((unsigned int)omxhdr,vdec_msg->status_code,
                     OMX_COMPONENT_GENERATE_PORT_RECONFIG);
    break;
  case VDEC_MSG_EVT_INFO_CONFIG_CHANGED:
  {
    DEBUG_PRINT_HIGH("\n Port settings changed info");
    // get_buffer_req and populate port defn structure
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    omx->m_port_def.nPortIndex = 1;
    eRet = omx->update_portdef(&(omx->m_port_def));
    omx->post_event ((unsigned int)omxhdr,vdec_msg->status_code,
                     OMX_COMPONENT_GENERATE_INFO_PORT_RECONFIG);
    break;
  }
  default:
    break;
  }
  return rc;
}

OMX_ERRORTYPE omx_vdec::empty_this_buffer_proxy_arbitrary (
                                                   OMX_HANDLETYPE hComp,
                                                   OMX_BUFFERHEADERTYPE *buffer
                                                           )
{
  unsigned address,p2,id;
  DEBUG_PRINT_LOW("\n Empty this arbitrary");

  if (buffer == NULL)
  {
    return OMX_ErrorBadParameter;
  }
  DEBUG_PRINT_LOW("\n ETBProxyArb: bufhdr = %p, bufhdr->pBuffer = %p", buffer, buffer->pBuffer);
  DEBUG_PRINT_LOW("\n ETBProxyArb: nFilledLen %u, flags %d, timestamp %u",
        buffer->nFilledLen, buffer->nFlags, (unsigned)buffer->nTimeStamp);

  /* return zero length and not an EOS buffer */
  /* return buffer if input flush in progress */
  if ((input_flush_progress == true) || ((buffer->nFilledLen == 0) &&
     ((buffer->nFlags & OMX_BUFFERFLAG_EOS) == 0)))
  {
    DEBUG_PRINT_HIGH("\n return zero legth buffer or flush in progress");
    m_cb.EmptyBufferDone (hComp,m_app_data,buffer);
    return OMX_ErrorNone;
  }

  if (psource_frame == NULL)
  {
    DEBUG_PRINT_LOW("\n Set Buffer as source Buffer %p time stamp %d",buffer,buffer->nTimeStamp);
    psource_frame = buffer;
    DEBUG_PRINT_LOW("\n Try to Push One Input Buffer ");
    push_input_buffer (hComp);
  }
  else
  {
    DEBUG_PRINT_LOW("\n Push the source buffer into pendingq %p",buffer);
    if (!m_input_pending_q.insert_entry((unsigned)buffer,NULL,NULL))
    {
      return OMX_ErrorBadParameter;
    }
  }


  return OMX_ErrorNone;
}

OMX_ERRORTYPE omx_vdec::push_input_buffer (OMX_HANDLETYPE hComp)
{
  unsigned address,p2,id;
  OMX_ERRORTYPE ret = OMX_ErrorNone;

  if (pdest_frame == NULL || psource_frame == NULL)
  {
    /*Check if we have a destination buffer*/
    if (pdest_frame == NULL)
    {
      DEBUG_PRINT_LOW("\n Get a Destination buffer from the queue");
      if (m_input_free_q.m_size)
      {
        m_input_free_q.pop_entry(&address,&p2,&id);
        pdest_frame = (OMX_BUFFERHEADERTYPE *)address;
        pdest_frame->nFilledLen = 0;
        pdest_frame->nTimeStamp = LLONG_MAX;
        DEBUG_PRINT_LOW("\n Address of Pmem Buffer %p",pdest_frame);
      }
    }

    /*Check if we have a destination buffer*/
    if (psource_frame == NULL)
    {
      DEBUG_PRINT_LOW("\n Get a source buffer from the queue");
      if (m_input_pending_q.m_size)
      {
        m_input_pending_q.pop_entry(&address,&p2,&id);
        psource_frame = (OMX_BUFFERHEADERTYPE *)address;
        DEBUG_PRINT_LOW("\n Next source Buffer %p time stamp %d",psource_frame,
                psource_frame->nTimeStamp);
        DEBUG_PRINT_LOW("\n Next source Buffer flag %d length %d",
        psource_frame->nFlags,psource_frame->nFilledLen);

      }
    }

  }

  while ((pdest_frame != NULL) && (psource_frame != NULL))
  {
    switch (codec_type_parse)
    {
      case CODEC_TYPE_MPEG4:
      case CODEC_TYPE_H263:
      case CODEC_TYPE_MPEG2:
        ret =  push_input_sc_codec(hComp);
      break;
      case CODEC_TYPE_H264:
        ret = push_input_h264(hComp);
      break;
      case CODEC_TYPE_VC1:
        ret = push_input_vc1(hComp);
      break;
    }
    if (ret != OMX_ErrorNone)
    {
      DEBUG_PRINT_ERROR("\n Pushing input Buffer Failed");
      omx_report_error ();
      break;
    }
  }

  return ret;
}

OMX_ERRORTYPE omx_vdec::push_input_sc_codec(OMX_HANDLETYPE hComp)
{
  OMX_U32 partial_frame = 1;
  OMX_BOOL generate_ebd = OMX_TRUE;
  unsigned address,p2,id;

  DEBUG_PRINT_LOW("\n Start Parsing the bit stream address %p TimeStamp %d",
        psource_frame,psource_frame->nTimeStamp);
  if (m_frame_parser.parse_sc_frame(psource_frame,
                                       pdest_frame,&partial_frame) == -1)
  {
    DEBUG_PRINT_ERROR("\n Error In Parsing Return Error");
    return OMX_ErrorBadParameter;
  }

  if (partial_frame == 0)
  {
    DEBUG_PRINT_LOW("\n Frame size %d source %p frame count %d",
          pdest_frame->nFilledLen,psource_frame,frame_count);


    DEBUG_PRINT_LOW("\n TimeStamp updated %d",pdest_frame->nTimeStamp);
    /*First Parsed buffer will have only header Hence skip*/
    if (frame_count == 0)
    {
      DEBUG_PRINT_LOW("\n H263/MPEG4 Codec First Frame ");

      if(codec_type_parse == CODEC_TYPE_MPEG4 ||
         codec_type_parse == CODEC_TYPE_DIVX) {
        mp4StreamType psBits;
        psBits.data = pdest_frame->pBuffer + pdest_frame->nOffset;
        psBits.numBytes = pdest_frame->nFilledLen;
        mp4_headerparser.parseHeader(&psBits);
      }

      frame_count++;
    }
    else
    {
      pdest_frame->nFlags &= ~OMX_BUFFERFLAG_EOS;
      if(pdest_frame->nFilledLen)
      {
        /*Push the frame to the Decoder*/
        if (empty_this_buffer_proxy(hComp,pdest_frame) != OMX_ErrorNone)
        {
          return OMX_ErrorBadParameter;
        }
        frame_count++;
        pdest_frame = NULL;

        if (m_input_free_q.m_size)
        {
          m_input_free_q.pop_entry(&address,&p2,&id);
          pdest_frame = (OMX_BUFFERHEADERTYPE *) address;
          pdest_frame->nFilledLen = 0;
        }
      }
      else if(!(psource_frame->nFlags & OMX_BUFFERFLAG_EOS))
      {
        DEBUG_PRINT_ERROR("\nZero len buffer return back to POOL");
        m_input_free_q.insert_entry((unsigned) pdest_frame,NULL,NULL);
        pdest_frame = NULL;
      }
    }
  }
  else
  {
    DEBUG_PRINT_LOW("\n Not a Complete Frame %d",pdest_frame->nFilledLen);
    /*Check if Destination Buffer is full*/
    if (pdest_frame->nAllocLen ==
        pdest_frame->nFilledLen + pdest_frame->nOffset)
    {
      DEBUG_PRINT_ERROR("\nERROR:Frame Not found though Destination Filled");
      return OMX_ErrorStreamCorrupt;
    }
  }

  if (psource_frame->nFilledLen == 0)
  {
    if (psource_frame->nFlags & OMX_BUFFERFLAG_EOS)
    {
      if (pdest_frame)
      {
        pdest_frame->nFlags |= psource_frame->nFlags;
        DEBUG_PRINT_LOW("\n Frame Found start Decoding Size =%d TimeStamp = %x",
                     pdest_frame->nFilledLen,pdest_frame->nTimeStamp);
        DEBUG_PRINT_LOW("\n Found a frame size = %d number = %d",
                     pdest_frame->nFilledLen,frame_count++);
        /*Push the frame to the Decoder*/
        if (empty_this_buffer_proxy(hComp,pdest_frame) != OMX_ErrorNone)
        {
          return OMX_ErrorBadParameter;
        }
        frame_count++;
        pdest_frame = NULL;
      }
      else
      {
        DEBUG_PRINT_LOW("\n Last frame in else dest addr") ;
        generate_ebd = OMX_FALSE;
      }
   }
    if(generate_ebd)
    {
      DEBUG_PRINT_LOW("\n Buffer Consumed return back to client %p",psource_frame);
      m_cb.EmptyBufferDone (hComp,m_app_data,psource_frame);
      psource_frame = NULL;

      if (m_input_pending_q.m_size)
      {
        DEBUG_PRINT_LOW("\n Pull Next source Buffer %p",psource_frame);
        m_input_pending_q.pop_entry(&address,&p2,&id);
        psource_frame = (OMX_BUFFERHEADERTYPE *) address;
        DEBUG_PRINT_LOW("\n Next source Buffer %p time stamp %d",psource_frame,
                psource_frame->nTimeStamp);
        DEBUG_PRINT_LOW("\n Next source Buffer flag %d length %d",
        psource_frame->nFlags,psource_frame->nFilledLen);
      }
    }
   }
  return OMX_ErrorNone;
}

OMX_ERRORTYPE omx_vdec::push_input_h264 (OMX_HANDLETYPE hComp)
{
  OMX_U32 partial_frame = 1;
  unsigned address,p2,id;
  OMX_BOOL isNewFrame = OMX_FALSE;
  OMX_BOOL generate_ebd = OMX_TRUE;

  if (h264_scratch.pBuffer == NULL)
  {
    DEBUG_PRINT_ERROR("\nERROR:H.264 Scratch Buffer not allocated");
    return OMX_ErrorBadParameter;
  }
  DEBUG_PRINT_LOW("\n Pending h264_scratch.nFilledLen %d "
      "look_ahead_nal %d", h264_scratch.nFilledLen, look_ahead_nal);
  DEBUG_PRINT_LOW("\n Pending pdest_frame->nFilledLen %d",pdest_frame->nFilledLen);
  if (h264_scratch.nFilledLen && look_ahead_nal)
  {
    look_ahead_nal = false;
    if ((pdest_frame->nAllocLen - pdest_frame->nFilledLen) >=
         h264_scratch.nFilledLen)
    {
      memcpy ((pdest_frame->pBuffer + pdest_frame->nFilledLen),
              h264_scratch.pBuffer,h264_scratch.nFilledLen);
      pdest_frame->nFilledLen += h264_scratch.nFilledLen;
      DEBUG_PRINT_LOW("\n Copy the previous NAL (h264 scratch) into Dest frame");
      h264_scratch.nFilledLen = 0;
    }
    else
    {
      DEBUG_PRINT_ERROR("\n Error:1: Destination buffer overflow for H264");
      return OMX_ErrorBadParameter;
    }
  }
  if (nal_length == 0)
  {
    DEBUG_PRINT_LOW("\n Zero NAL, hence parse using start code");
    if (m_frame_parser.parse_sc_frame(psource_frame,
        &h264_scratch,&partial_frame) == -1)
    {
      DEBUG_PRINT_ERROR("\n Error In Parsing Return Error");
      return OMX_ErrorBadParameter;
    }
  }
  else
  {
    DEBUG_PRINT_LOW("\n Non-zero NAL length clip, hence parse with NAL size %d ",nal_length);
    if (m_frame_parser.parse_h264_nallength(psource_frame,
        &h264_scratch,&partial_frame) == -1)
    {
      DEBUG_PRINT_ERROR("\n Error In Parsing NAL size, Return Error");
      return OMX_ErrorBadParameter;
    }
  }

  if (partial_frame == 0)
  {
    if (nal_count == 0 && h264_scratch.nFilledLen == 0)
    {
      DEBUG_PRINT_LOW("\n First NAL with Zero Length, hence Skip");
      nal_count++;
      h264_scratch.nTimeStamp = psource_frame->nTimeStamp;
      h264_scratch.nFlags = psource_frame->nFlags;
    }
    else
    {
      DEBUG_PRINT_LOW("\n Parsed New NAL Length = %d",h264_scratch.nFilledLen);
      if(h264_scratch.nFilledLen)
      {
          h264_parser->parse_nal((OMX_U8*)h264_scratch.pBuffer, h264_scratch.nFilledLen,
                                 NALU_TYPE_SPS);
#ifndef PROCESS_EXTRADATA_IN_OUTPUT_PORT
        if (client_extradata & OMX_TIMEINFO_EXTRADATA)
          h264_parser->parse_nal((OMX_U8*)h264_scratch.pBuffer,
                                  h264_scratch.nFilledLen, NALU_TYPE_SEI);
        else if (client_extradata & OMX_FRAMEINFO_EXTRADATA)
          // If timeinfo is present frame info from SEI is already processed
          h264_parser->parse_nal((OMX_U8*)h264_scratch.pBuffer,
                                  h264_scratch.nFilledLen, NALU_TYPE_SEI);
#endif
        m_frame_parser.mutils->isNewFrame(&h264_scratch, 0, isNewFrame);
        nal_count++;
        if (VALID_TS(h264_last_au_ts) && !VALID_TS(pdest_frame->nTimeStamp)) {
          pdest_frame->nTimeStamp = h264_last_au_ts;
          pdest_frame->nFlags = h264_last_au_flags;
#ifdef PANSCAN_HDLR
          if (client_extradata & OMX_FRAMEINFO_EXTRADATA)
            h264_parser->update_panscan_data(h264_last_au_ts);
#endif
        }
        if(m_frame_parser.mutils->nalu_type == NALU_TYPE_NON_IDR ||
           m_frame_parser.mutils->nalu_type == NALU_TYPE_IDR) {
          h264_last_au_ts = h264_scratch.nTimeStamp;
          h264_last_au_flags = h264_scratch.nFlags;
#ifndef PROCESS_EXTRADATA_IN_OUTPUT_PORT
          if (client_extradata & OMX_TIMEINFO_EXTRADATA)
          {
            OMX_S64 ts_in_sei = h264_parser->process_ts_with_sei_vui(h264_last_au_ts);
            if (!VALID_TS(h264_last_au_ts))
              h264_last_au_ts = ts_in_sei;
          }
#endif
        } else
          h264_last_au_ts = LLONG_MAX;
      }

      if (!isNewFrame)
      {
        if ( (pdest_frame->nAllocLen - pdest_frame->nFilledLen) >=
            h264_scratch.nFilledLen)
        {
          DEBUG_PRINT_LOW("\n Not a NewFrame Copy into Dest len %d",
              h264_scratch.nFilledLen);
          memcpy ((pdest_frame->pBuffer + pdest_frame->nFilledLen),
              h264_scratch.pBuffer,h264_scratch.nFilledLen);
          pdest_frame->nFilledLen += h264_scratch.nFilledLen;
          if(m_frame_parser.mutils->nalu_type == NALU_TYPE_EOSEQ)
            pdest_frame->nFlags |= QOMX_VIDEO_BUFFERFLAG_EOSEQ;
          h264_scratch.nFilledLen = 0;
        }
        else
        {
          DEBUG_PRINT_LOW("\n Error:2: Destination buffer overflow for H264");
          return OMX_ErrorBadParameter;
        }
      }
      else
      {
        look_ahead_nal = true;
        DEBUG_PRINT_LOW("\n Frame Found start Decoding Size =%d TimeStamp = %x",
                     pdest_frame->nFilledLen,pdest_frame->nTimeStamp);
        DEBUG_PRINT_LOW("\n Found a frame size = %d number = %d",
                     pdest_frame->nFilledLen,frame_count++);

        if (pdest_frame->nFilledLen == 0)
        {
          DEBUG_PRINT_LOW("\n Copy the Current Frame since and push it");
          look_ahead_nal = false;
          if ( (pdest_frame->nAllocLen - pdest_frame->nFilledLen) >=
               h264_scratch.nFilledLen)
          {
            memcpy ((pdest_frame->pBuffer + pdest_frame->nFilledLen),
                    h264_scratch.pBuffer,h264_scratch.nFilledLen);
            pdest_frame->nFilledLen += h264_scratch.nFilledLen;
            h264_scratch.nFilledLen = 0;
          }
          else
          {
            DEBUG_PRINT_ERROR("\n Error:3: Destination buffer overflow for H264");
            return OMX_ErrorBadParameter;
          }
        }
        else
        {
          if(psource_frame->nFilledLen || h264_scratch.nFilledLen)
          {
            DEBUG_PRINT_LOW("\n Reset the EOS Flag");
            pdest_frame->nFlags &= ~OMX_BUFFERFLAG_EOS;
          }
          /*Push the frame to the Decoder*/
          if (empty_this_buffer_proxy(hComp,pdest_frame) != OMX_ErrorNone)
          {
            return OMX_ErrorBadParameter;
          }
          //frame_count++;
          pdest_frame = NULL;
          if (m_input_free_q.m_size)
          {
            m_input_free_q.pop_entry(&address,&p2,&id);
            pdest_frame = (OMX_BUFFERHEADERTYPE *) address;
            DEBUG_PRINT_LOW("\n Pop the next pdest_buffer %p",pdest_frame);
            pdest_frame->nFilledLen = 0;
            pdest_frame->nFlags = 0;
            pdest_frame->nTimeStamp = LLONG_MAX;
          }
        }
      }
    }
  }
  else
  {
    DEBUG_PRINT_LOW("\n Not a Complete Frame, pdest_frame->nFilledLen %d",pdest_frame->nFilledLen);
    /*Check if Destination Buffer is full*/
    if (h264_scratch.nAllocLen ==
        h264_scratch.nFilledLen + h264_scratch.nOffset)
    {
      DEBUG_PRINT_ERROR("\nERROR: Frame Not found though Destination Filled");
      return OMX_ErrorStreamCorrupt;
    }
  }

  if (!psource_frame->nFilledLen)
  {
    DEBUG_PRINT_LOW("\n Buffer Consumed return source %p back to client",psource_frame);

    if (psource_frame->nFlags & OMX_BUFFERFLAG_EOS)
    {
      if (pdest_frame)
      {
        DEBUG_PRINT_LOW("\n EOS Reached Pass Last Buffer");
        if ( (pdest_frame->nAllocLen - pdest_frame->nFilledLen) >=
             h264_scratch.nFilledLen)
        {
          memcpy ((pdest_frame->pBuffer + pdest_frame->nFilledLen),
                  h264_scratch.pBuffer,h264_scratch.nFilledLen);
          pdest_frame->nFilledLen += h264_scratch.nFilledLen;
          h264_scratch.nFilledLen = 0;
        }
        else
        {
          DEBUG_PRINT_ERROR("\nERROR:4: Destination buffer overflow for H264");
          return OMX_ErrorBadParameter;
        }
        pdest_frame->nTimeStamp = h264_scratch.nTimeStamp;
        pdest_frame->nFlags = h264_scratch.nFlags | psource_frame->nFlags;

        DEBUG_PRINT_LOW("\n pdest_frame->nFilledLen =%d TimeStamp = %x",
                     pdest_frame->nFilledLen,pdest_frame->nTimeStamp);
        DEBUG_PRINT_LOW("\n Push AU frame number %d to driver", frame_count++);
#ifndef PROCESS_EXTRADATA_IN_OUTPUT_PORT
        if (client_extradata & OMX_TIMEINFO_EXTRADATA)
        {
          OMX_S64 ts_in_sei = h264_parser->process_ts_with_sei_vui(pdest_frame->nTimeStamp);
          if (!VALID_TS(pdest_frame->nTimeStamp))
            pdest_frame->nTimeStamp = ts_in_sei;
        }
#endif
        /*Push the frame to the Decoder*/
        if (empty_this_buffer_proxy(hComp,pdest_frame) != OMX_ErrorNone)
        {
          return OMX_ErrorBadParameter;
        }
        frame_count++;
        pdest_frame = NULL;
      }
      else
      {
        DEBUG_PRINT_LOW("\n Last frame in else dest addr %p size %d",
                     pdest_frame,h264_scratch.nFilledLen);
        generate_ebd = OMX_FALSE;
      }
    }
  }
  if(generate_ebd && !psource_frame->nFilledLen)
  {
    m_cb.EmptyBufferDone (hComp,m_app_data,psource_frame);
    psource_frame = NULL;
    if (m_input_pending_q.m_size)
    {
      DEBUG_PRINT_LOW("\n Pull Next source Buffer %p",psource_frame);
      m_input_pending_q.pop_entry(&address,&p2,&id);
      psource_frame = (OMX_BUFFERHEADERTYPE *) address;
      DEBUG_PRINT_LOW("\nNext source Buffer flag %d src length %d",
      psource_frame->nFlags,psource_frame->nFilledLen);
    }
  }
  return OMX_ErrorNone;
}

OMX_ERRORTYPE omx_vdec::push_input_vc1 (OMX_HANDLETYPE hComp)
{
    OMX_U8 *buf, *pdest;
    OMX_U32 partial_frame = 1;
    OMX_U32 buf_len, dest_len;

    if(first_frame == 0)
    {
        first_frame = 1;
        DEBUG_PRINT_LOW("\nFirst i/p buffer for VC1 arbitrary bytes\n");
        if(!m_vendor_config.pData)
        {
            DEBUG_PRINT_LOW("\nCheck profile type in 1st source buffer\n");
            buf = psource_frame->pBuffer;
            buf_len = psource_frame->nFilledLen;

            if ((*((OMX_U32 *) buf) & VC1_SP_MP_START_CODE_MASK) ==
                VC1_SP_MP_START_CODE)
            {
                m_vc1_profile = VC1_SP_MP_RCV;
            }
            else if(*((OMX_U32 *) buf) & VC1_AP_SEQ_START_CODE)
            {
                m_vc1_profile = VC1_AP;
            }
            else
            {
                DEBUG_PRINT_ERROR("\nInvalid sequence layer in first buffer\n");
                return OMX_ErrorStreamCorrupt;
            }
        }
        else
        {
            pdest = pdest_frame->pBuffer + pdest_frame->nFilledLen +
                pdest_frame->nOffset;
            dest_len = pdest_frame->nAllocLen - (pdest_frame->nFilledLen +
                pdest_frame->nOffset);

            if(dest_len < m_vendor_config.nDataSize)
            {
                DEBUG_PRINT_ERROR("\nDestination buffer full\n");
                return OMX_ErrorBadParameter;
            }
            else
            {
                memcpy(pdest, m_vendor_config.pData, m_vendor_config.nDataSize);
                pdest_frame->nFilledLen += m_vendor_config.nDataSize;
            }
        }
    }

    switch(m_vc1_profile)
    {
        case VC1_AP:
            DEBUG_PRINT_LOW("\n VC1 AP, hence parse using frame start code");
            if (push_input_sc_codec(hComp) != OMX_ErrorNone)
            {
                DEBUG_PRINT_ERROR("\n Error In Parsing VC1 AP start code");
                return OMX_ErrorBadParameter;
            }
        break;

        case VC1_SP_MP_RCV:
        default:
            DEBUG_PRINT_ERROR("\n Unsupported VC1 profile in ArbitraryBytes Mode\n");
            return OMX_ErrorBadParameter;
    }
    return OMX_ErrorNone;
}

#ifdef USE_ION
int omx_vdec::alloc_map_ion_memory(OMX_U32 buffer_size,
              OMX_U32 alignment, struct ion_allocation_data *alloc_data,
	      struct ion_fd_data *fd_data, int flag)
{
  int fd = -EINVAL;
  int rc = -EINVAL;
  int ion_dev_flag;
  struct vdec_ion ion_buf_info;
  if (!alloc_data || buffer_size <= 0 || !fd_data) {
     DEBUG_PRINT_ERROR("Invalid arguments to alloc_map_ion_memory\n");
     return -EINVAL;
  }
  ion_dev_flag = O_RDONLY;
  fd = open (MEM_DEVICE, ion_dev_flag);
  if (fd < 0) {
    DEBUG_PRINT_ERROR("opening ion device failed with fd = %d\n", fd);
    return fd;
  }
  alloc_data->flags = 0;
  if(!secure_mode && (flag & ION_FLAG_CACHED))
  {
    alloc_data->flags |= ION_FLAG_CACHED;
  }
  alloc_data->len = buffer_size;
  alloc_data->align = clip2(alignment);
  if (alloc_data->align < 4096)
  {
    alloc_data->align = 4096;
  }
  if(secure_mode) {
    alloc_data->heap_id_mask = ION_HEAP(MEM_HEAP_ID);
    alloc_data->flags |= ION_SECURE;
  } else {
    alloc_data->heap_id_mask = ION_HEAP(MEM_HEAP_ID);
  }
  rc = ioctl(fd,ION_IOC_ALLOC,alloc_data);
  if (rc || !alloc_data->handle) {
    DEBUG_PRINT_ERROR("\n ION ALLOC memory failed ");
    alloc_data->handle = NULL;
    close(fd);
    fd = -ENOMEM;
    return fd;
  }
  fd_data->handle = alloc_data->handle;
  rc = ioctl(fd,ION_IOC_MAP,fd_data);
  if (rc) {
    DEBUG_PRINT_ERROR("\n ION MAP failed ");
    ion_buf_info.ion_alloc_data = *alloc_data;
    ion_buf_info.ion_device_fd = fd;
    ion_buf_info.fd_ion_data = *fd_data;
    free_ion_memory(&ion_buf_info);
    fd_data->fd =-1;
    close(fd);
    fd = -ENOMEM;
  }

  return fd;
}

void omx_vdec::free_ion_memory(struct vdec_ion *buf_ion_info) {

     if(!buf_ion_info) {
       DEBUG_PRINT_ERROR("\n ION: free called with invalid fd/allocdata");
       return;
     }
     if(ioctl(buf_ion_info->ion_device_fd,ION_IOC_FREE,
             &buf_ion_info->ion_alloc_data.handle)) {
       DEBUG_PRINT_ERROR("\n ION: free failed" );
     }
     close(buf_ion_info->ion_device_fd);
     buf_ion_info->ion_device_fd = -1;
     buf_ion_info->ion_alloc_data.handle = NULL;
     buf_ion_info->fd_ion_data.fd = -1;
}
#else
bool omx_vdec::align_pmem_buffers(int pmem_fd, OMX_U32 buffer_size,
                                  OMX_U32 alignment)
{
  struct pmem_allocation allocation;
  allocation.size = buffer_size;
  allocation.align = clip2(alignment);
  if (allocation.align < 4096)
  {
    allocation.align = 4096;
  }
  if (ioctl(pmem_fd, PMEM_ALLOCATE_ALIGNED, &allocation) < 0)
  {
    DEBUG_PRINT_ERROR("\n Aligment(%u) failed with pmem driver Sz(%lu)",
      allocation.align, allocation.size);
    return false;
  }
  return true;
}
#endif
void omx_vdec::free_output_buffer_header()
{
  DEBUG_PRINT_HIGH("\n ALL output buffers are freed/released");
  output_use_buffer = false;
  ouput_egl_buffers = false;

  if (m_out_mem_ptr)
  {
    free (m_out_mem_ptr);
    m_out_mem_ptr = NULL;
  }

  if(m_platform_list)
  {
    free(m_platform_list);
    m_platform_list = NULL;
  }

  if (drv_ctx.ptr_respbuffer)
  {
    free (drv_ctx.ptr_respbuffer);
    drv_ctx.ptr_respbuffer = NULL;
  }
  if (drv_ctx.ptr_outputbuffer)
  {
    free (drv_ctx.ptr_outputbuffer);
    drv_ctx.ptr_outputbuffer = NULL;
  }
#ifdef USE_ION
    if (drv_ctx.op_buf_ion_info) {
        DEBUG_PRINT_LOW("\n Free o/p ion context");
	free(drv_ctx.op_buf_ion_info);
        drv_ctx.op_buf_ion_info = NULL;
    }
#endif
}

void omx_vdec::free_input_buffer_header()
{
    input_use_buffer = false;
    if (arbitrary_bytes)
    {
      if (m_frame_parser.mutils)
      {
        DEBUG_PRINT_LOW("\n Free utils parser");
        delete (m_frame_parser.mutils);
        m_frame_parser.mutils = NULL;
      }

      if (m_inp_heap_ptr)
      {
        DEBUG_PRINT_LOW("\n Free input Heap Pointer");
        free (m_inp_heap_ptr);
        m_inp_heap_ptr = NULL;
      }

      if (m_phdr_pmem_ptr)
      {
        DEBUG_PRINT_LOW("\n Free input pmem header Pointer");
        free (m_phdr_pmem_ptr);
        m_phdr_pmem_ptr = NULL;
      }
    }
    if (m_inp_mem_ptr)
    {
      DEBUG_PRINT_LOW("\n Free input pmem Pointer area");
      free (m_inp_mem_ptr);
      m_inp_mem_ptr = NULL;
    }
    if (drv_ctx.ptr_inputbuffer)
    {
      DEBUG_PRINT_LOW("\n Free Driver Context pointer");
      free (drv_ctx.ptr_inputbuffer);
      drv_ctx.ptr_inputbuffer = NULL;
    }
#ifdef USE_ION
    if (drv_ctx.ip_buf_ion_info) {
        DEBUG_PRINT_LOW("\n Free ion context");
	free(drv_ctx.ip_buf_ion_info);
        drv_ctx.ip_buf_ion_info = NULL;
    }
#endif
}
void omx_vdec::stream_off()
{
	int rc=0;
	enum v4l2_buf_type btype;
	btype = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	rc = ioctl(drv_ctx.video_driver_fd, VIDIOC_STREAMOFF, &btype);
	if (rc) {
		/*TODO: How to handle this case */
		printf("\n Failed to call streamoff on OUTPUT Port \n");
	} else {
		streaming[CAPTURE_PORT] = false;
	}
}

OMX_ERRORTYPE omx_vdec::get_buffer_req(vdec_allocatorproperty *buffer_prop)
{
  OMX_ERRORTYPE eRet = OMX_ErrorNone;
  struct v4l2_requestbuffers bufreq;
  unsigned int buf_size = 0, extra_data_size = 0;
  struct v4l2_format fmt;
  int ret;
    DEBUG_PRINT_LOW("GetBufReq IN: ActCnt(%d) Size(%d)",
    buffer_prop->actualcount, buffer_prop->buffer_size);
	bufreq.memory = V4L2_MEMORY_USERPTR;
	if(in_reconfig == true)
	bufreq.count = 0;
	else
	bufreq.count = 2;
   if(buffer_prop->buffer_type == VDEC_BUFFER_TYPE_INPUT){
    bufreq.type=V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	fmt.type =V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    fmt.fmt.pix_mp.pixelformat = output_capability;
  }else if (buffer_prop->buffer_type == VDEC_BUFFER_TYPE_OUTPUT){
    bufreq.type=V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	fmt.type =V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    fmt.fmt.pix_mp.pixelformat = capture_capability;
  }else {eRet = OMX_ErrorBadParameter;}
  if(eRet==OMX_ErrorNone){
  ret = ioctl(drv_ctx.video_driver_fd,VIDIOC_REQBUFS, &bufreq);
  }
  if(ret)
  {
    DEBUG_PRINT_ERROR("Requesting buffer requirements failed");
	/*TODO: How to handle this case */
    eRet = OMX_ErrorInsufficientResources;
	return eRet;
  }
  else
  {
  buffer_prop->actualcount = bufreq.count;
  buffer_prop->mincount = bufreq.count;
  printf("Count = %d \n ",bufreq.count);
  }
  DEBUG_PRINT_LOW("GetBufReq IN: ActCnt(%d) Size(%d)",
    buffer_prop->actualcount, buffer_prop->buffer_size);

  fmt.fmt.pix_mp.height = drv_ctx.video_resolution.frame_height;
  fmt.fmt.pix_mp.width = drv_ctx.video_resolution.frame_width;

  ret = ioctl(drv_ctx.video_driver_fd, VIDIOC_G_FMT, &fmt);

  drv_ctx.video_resolution.frame_height = fmt.fmt.pix_mp.height;
  drv_ctx.video_resolution.frame_width = fmt.fmt.pix_mp.width;

  printf("Buffer Size = %d \n ",fmt.fmt.pix_mp.plane_fmt[0].sizeimage);

  if(ret)
  {
	/*TODO: How to handle this case */
    DEBUG_PRINT_ERROR("Requesting buffer requirements failed");
    eRet = OMX_ErrorInsufficientResources;
  }
  else
  {
    buffer_prop->buffer_size = fmt.fmt.pix_mp.plane_fmt[0].sizeimage;
    buf_size = buffer_prop->buffer_size;
    if (client_extradata & OMX_FRAMEINFO_EXTRADATA)
    {
      DEBUG_PRINT_HIGH("Frame info extra data enabled!");
      extra_data_size += OMX_FRAMEINFO_EXTRADATA_SIZE;
    }
    if (client_extradata & OMX_INTERLACE_EXTRADATA)
    {
      DEBUG_PRINT_HIGH("Interlace extra data enabled!");
      extra_data_size += OMX_INTERLACE_EXTRADATA_SIZE;
    }
    if (client_extradata & OMX_PORTDEF_EXTRADATA)
    {
       extra_data_size += OMX_PORTDEF_EXTRADATA_SIZE;
       DEBUG_PRINT_HIGH("Smooth streaming enabled extra_data_size=%d\n",
         extra_data_size);
    }
    if (extra_data_size)
    {
      extra_data_size += sizeof(OMX_OTHER_EXTRADATATYPE); //Space for terminator
      buf_size = ((buf_size + 3)&(~3)); //Align extradata start address to 64Bit
    }
    buf_size += extra_data_size;
    buf_size = (buf_size + buffer_prop->alignment - 1)&(~(buffer_prop->alignment - 1));
    DEBUG_PRINT_LOW("GetBufReq UPDATE: ActCnt(%d) Size(%d) BufSize(%d)",
      buffer_prop->actualcount, buffer_prop->buffer_size, buf_size);
    if (in_reconfig) // BufReq will be set to driver when port is disabled
      buffer_prop->buffer_size = buf_size;
    else if (buf_size != buffer_prop->buffer_size)
    {
      buffer_prop->buffer_size = buf_size;
      eRet = set_buffer_req(buffer_prop);
    }
  }
  DEBUG_PRINT_LOW("GetBufReq OUT: ActCnt(%d) Size(%d)",
    buffer_prop->actualcount, buffer_prop->buffer_size);
  return eRet;
}

OMX_ERRORTYPE omx_vdec::set_buffer_req(vdec_allocatorproperty *buffer_prop)
{
  OMX_ERRORTYPE eRet = OMX_ErrorNone;
  unsigned buf_size = 0;
  struct v4l2_format fmt;
  int ret;
  DEBUG_PRINT_LOW("SetBufReq IN: ActCnt(%d) Size(%d)",
    buffer_prop->actualcount, buffer_prop->buffer_size);
  buf_size = (buffer_prop->buffer_size + buffer_prop->alignment - 1)&(~(buffer_prop->alignment - 1));
  if (buf_size != buffer_prop->buffer_size)
  {
    DEBUG_PRINT_ERROR("Buffer size alignment error: Requested(%d) Required(%d)",
      buffer_prop->buffer_size, buf_size);
    eRet = OMX_ErrorBadParameter;
  }
  else
  {
    fmt.fmt.pix_mp.height = drv_ctx.video_resolution.frame_height;
	fmt.fmt.pix_mp.width = drv_ctx.video_resolution.frame_width;
	if(buffer_prop->buffer_type == VDEC_BUFFER_TYPE_INPUT){
	fmt.type =V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	fmt.fmt.pix_mp.pixelformat = output_capability;
	}else if (buffer_prop->buffer_type == VDEC_BUFFER_TYPE_OUTPUT){
fmt.type =V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	fmt.fmt.pix_mp.pixelformat = capture_capability;
} else {eRet = OMX_ErrorBadParameter;}
    ret = ioctl(drv_ctx.video_driver_fd, VIDIOC_S_FMT, &fmt);
    if(ret)
    {
	  /*TODO: How to handle this case */
      DEBUG_PRINT_ERROR("Setting buffer requirements failed");
      eRet = OMX_ErrorInsufficientResources;
    }
  }
  return eRet;
}

OMX_ERRORTYPE omx_vdec::start_port_reconfig()
{
  struct vdec_ioctl_msg ioctl_msg = {NULL, NULL};
  OMX_ERRORTYPE eRet = OMX_ErrorNone;
  enum v4l2_buf_type btype;
  int rc = 0,i;
  struct v4l2_plane plane;
  struct v4l2_buffer v4l2_buf ={0};
  struct v4l2_decoder_cmd dec;
  dec.cmd = V4L2_DEC_CMD_STOP;
  rc = ioctl(drv_ctx.video_driver_fd, VIDIOC_DECODER_CMD, &dec);
  btype = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
  in_reconfig = true;
  return eRet;
}

OMX_ERRORTYPE omx_vdec::update_picture_resolution()
{
  struct vdec_ioctl_msg ioctl_msg = {NULL, NULL};
  OMX_ERRORTYPE eRet = OMX_ErrorNone;
  ioctl_msg.in = NULL;
  ioctl_msg.out = &drv_ctx.video_resolution;
  if (/*ioctl(drv_ctx.video_driver_fd, VDEC_IOCTL_GET_PICRES, &ioctl_msg)*/0)
  {
    DEBUG_PRINT_ERROR("Error VDEC_IOCTL_GET_PICRES");
    eRet = OMX_ErrorHardware;
  }
  return eRet;
}

OMX_ERRORTYPE omx_vdec::update_portdef(OMX_PARAM_PORTDEFINITIONTYPE *portDefn)
{
  OMX_ERRORTYPE eRet = OMX_ErrorNone;
  if (!portDefn)
  {
    return OMX_ErrorBadParameter;
  }
  DEBUG_PRINT_LOW("omx_vdec::update_portdef\n");
  portDefn->nVersion.nVersion = OMX_SPEC_VERSION;
  portDefn->nSize = sizeof(portDefn);
  portDefn->eDomain    = OMX_PortDomainVideo;
  if (drv_ctx.frame_rate.fps_denominator > 0)
    portDefn->format.video.xFramerate = drv_ctx.frame_rate.fps_numerator /
                                        drv_ctx.frame_rate.fps_denominator;
  else {
    DEBUG_PRINT_ERROR("Error: Divide by zero \n");
    return OMX_ErrorBadParameter;
  }
  if (0 == portDefn->nPortIndex)
  {
    portDefn->eDir =  OMX_DirInput;
    portDefn->nBufferCountActual = drv_ctx.ip_buf.actualcount;
    portDefn->nBufferCountMin    = drv_ctx.ip_buf.mincount;
    portDefn->nBufferSize        = drv_ctx.ip_buf.buffer_size;
    portDefn->format.video.eColorFormat = OMX_COLOR_FormatUnused;
    portDefn->format.video.eCompressionFormat = eCompressionFormat;
    portDefn->bEnabled   = m_inp_bEnabled;
    portDefn->bPopulated = m_inp_bPopulated;
  }
  else if (1 == portDefn->nPortIndex)
  {
    portDefn->eDir =  OMX_DirOutput;
    eRet=get_buffer_req(&drv_ctx.op_buf);
    if (in_reconfig)
    {
      portDefn->nBufferCountActual = op_buf_rcnfg.actualcount;
      portDefn->nBufferCountMin    = op_buf_rcnfg.mincount;
      portDefn->nBufferSize        = op_buf_rcnfg.buffer_size;
    }
    else
    {
      portDefn->nBufferCountActual = drv_ctx.op_buf.actualcount;
      portDefn->nBufferCountMin    = drv_ctx.op_buf.mincount;
      portDefn->nBufferSize        = drv_ctx.op_buf.buffer_size;
    }
    portDefn->format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    portDefn->bEnabled   = m_out_bEnabled;
    portDefn->bPopulated = m_out_bPopulated;
    if (drv_ctx.output_format == VDEC_YUV_FORMAT_NV12)
      portDefn->format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
    else if (drv_ctx.output_format == VDEC_YUV_FORMAT_TILE_4x2)
      portDefn->format.video.eColorFormat = (OMX_COLOR_FORMATTYPE)
        QOMX_COLOR_FormatYUV420PackedSemiPlanar64x32Tile2m8ka;
    else
    {
      DEBUG_PRINT_ERROR("ERROR: Color format unknown: %x\n", drv_ctx.output_format);
    }
  }
  else
  {
    portDefn->eDir = OMX_DirMax;
    DEBUG_PRINT_LOW(" get_parameter: Bad Port idx %d",
             (int)portDefn->nPortIndex);
    eRet = OMX_ErrorBadPortIndex;
  }
  portDefn->format.video.nFrameHeight =  drv_ctx.video_resolution.frame_height;
  portDefn->format.video.nFrameWidth  =  drv_ctx.video_resolution.frame_width;
  portDefn->format.video.nStride = drv_ctx.video_resolution.stride;
  portDefn->format.video.nSliceHeight = drv_ctx.video_resolution.scan_lines;
  DEBUG_PRINT_LOW("update_portdef Width = %d Height = %d Stride = %u"
    "SliceHeight = %u \n", portDefn->format.video.nFrameHeight,
    portDefn->format.video.nFrameWidth,
    portDefn->format.video.nStride,
    portDefn->format.video.nSliceHeight);
  return eRet;

}

OMX_ERRORTYPE omx_vdec::allocate_output_headers()
{
  OMX_ERRORTYPE eRet = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE *bufHdr = NULL;
  unsigned i= 0;

  if(!m_out_mem_ptr) {
    DEBUG_PRINT_HIGH("\n Use o/p buffer case - Header List allocation");
    int nBufHdrSize        = 0;
    int nPlatformEntrySize = 0;
    int nPlatformListSize  = 0;
    int nPMEMInfoSize = 0;
    OMX_QCOM_PLATFORM_PRIVATE_LIST      *pPlatformList;
    OMX_QCOM_PLATFORM_PRIVATE_ENTRY     *pPlatformEntry;
    OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO *pPMEMInfo;

    DEBUG_PRINT_LOW("Setting First Output Buffer(%d)\n",
      drv_ctx.op_buf.actualcount);
    nBufHdrSize        = drv_ctx.op_buf.actualcount *
                         sizeof(OMX_BUFFERHEADERTYPE);

    nPMEMInfoSize      = drv_ctx.op_buf.actualcount *
                         sizeof(OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO);
    nPlatformListSize  = drv_ctx.op_buf.actualcount *
                         sizeof(OMX_QCOM_PLATFORM_PRIVATE_LIST);
    nPlatformEntrySize = drv_ctx.op_buf.actualcount *
                         sizeof(OMX_QCOM_PLATFORM_PRIVATE_ENTRY);

    DEBUG_PRINT_LOW("TotalBufHdr %d BufHdrSize %d PMEM %d PL %d\n",nBufHdrSize,
                         sizeof(OMX_BUFFERHEADERTYPE),
                         nPMEMInfoSize,
                         nPlatformListSize);
    DEBUG_PRINT_LOW("PE %d bmSize %d \n",nPlatformEntrySize,
                         m_out_bm_count);
    m_out_mem_ptr = (OMX_BUFFERHEADERTYPE  *)calloc(nBufHdrSize,1);
    // Alloc mem for platform specific info
    char *pPtr=NULL;
    pPtr = (char*) calloc(nPlatformListSize + nPlatformEntrySize +
                                     nPMEMInfoSize,1);
    drv_ctx.ptr_outputbuffer = (struct vdec_bufferpayload *) \
      calloc (sizeof(struct vdec_bufferpayload),
      drv_ctx.op_buf.actualcount);
    drv_ctx.ptr_respbuffer = (struct vdec_output_frameinfo  *)\
      calloc (sizeof (struct vdec_output_frameinfo),
      drv_ctx.op_buf.actualcount);
#ifdef USE_ION
    drv_ctx.op_buf_ion_info = (struct vdec_ion * ) \
      calloc (sizeof(struct vdec_ion),drv_ctx.op_buf.actualcount);
#endif

    if(m_out_mem_ptr && pPtr && drv_ctx.ptr_outputbuffer
       && drv_ctx.ptr_respbuffer)
    {
      bufHdr          =  m_out_mem_ptr;
      m_platform_list = (OMX_QCOM_PLATFORM_PRIVATE_LIST *)(pPtr);
      m_platform_entry= (OMX_QCOM_PLATFORM_PRIVATE_ENTRY *)
                        (((char *) m_platform_list)  + nPlatformListSize);
      m_pmem_info     = (OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO *)
                        (((char *) m_platform_entry) + nPlatformEntrySize);
      pPlatformList   = m_platform_list;
      pPlatformEntry  = m_platform_entry;
      pPMEMInfo       = m_pmem_info;

      DEBUG_PRINT_LOW("Memory Allocation Succeeded for OUT port%p\n",m_out_mem_ptr);

      // Settting the entire storage nicely
      DEBUG_PRINT_LOW("bHdr %p OutMem %p PE %p\n",bufHdr,
                      m_out_mem_ptr,pPlatformEntry);
      DEBUG_PRINT_LOW(" Pmem Info = %p \n",pPMEMInfo);
      for(i=0; i < drv_ctx.op_buf.actualcount ; i++)
      {
        bufHdr->nSize              = sizeof(OMX_BUFFERHEADERTYPE);
        bufHdr->nVersion.nVersion  = OMX_SPEC_VERSION;
        // Set the values when we determine the right HxW param
        bufHdr->nAllocLen          = 0;
        bufHdr->nFilledLen         = 0;
        bufHdr->pAppPrivate        = NULL;
        bufHdr->nOutputPortIndex   = OMX_CORE_OUTPUT_PORT_INDEX;
        pPlatformEntry->type       = OMX_QCOM_PLATFORM_PRIVATE_PMEM;
        pPlatformEntry->entry      = pPMEMInfo;
        // Initialize the Platform List
        pPlatformList->nEntries    = 1;
        pPlatformList->entryList   = pPlatformEntry;
        // Keep pBuffer NULL till vdec is opened
        bufHdr->pBuffer            = NULL;
        pPMEMInfo->offset          =  0;
        pPMEMInfo->pmem_fd = 0;
        bufHdr->pPlatformPrivate = pPlatformList;
        drv_ctx.ptr_outputbuffer[i].pmem_fd = -1;
#ifdef USE_ION
        drv_ctx.op_buf_ion_info[i].ion_device_fd =-1;
#endif
        /*Create a mapping between buffers*/
        bufHdr->pOutputPortPrivate = &drv_ctx.ptr_respbuffer[i];
        drv_ctx.ptr_respbuffer[i].client_data = (void *) \
                                            &drv_ctx.ptr_outputbuffer[i];
        // Move the buffer and buffer header pointers
        bufHdr++;
        pPMEMInfo++;
        pPlatformEntry++;
        pPlatformList++;
      }
    }
    else
    {
      DEBUG_PRINT_ERROR("Output buf mem alloc failed[0x%x][0x%x]\n",\
                                        m_out_mem_ptr, pPtr);
      if(m_out_mem_ptr)
      {
        free(m_out_mem_ptr);
        m_out_mem_ptr = NULL;
      }
      if(pPtr)
      {
        free(pPtr);
        pPtr = NULL;
      }
      if(drv_ctx.ptr_outputbuffer)
      {
        free(drv_ctx.ptr_outputbuffer);
        drv_ctx.ptr_outputbuffer = NULL;
      }
      if(drv_ctx.ptr_respbuffer)
      {
        free(drv_ctx.ptr_respbuffer);
        drv_ctx.ptr_respbuffer = NULL;
      }
#ifdef USE_ION
    if (drv_ctx.op_buf_ion_info) {
        DEBUG_PRINT_LOW("\n Free o/p ion context");
	free(drv_ctx.op_buf_ion_info);
        drv_ctx.op_buf_ion_info = NULL;
    }
#endif
      eRet =  OMX_ErrorInsufficientResources;
    }
  } else {
    eRet =  OMX_ErrorInsufficientResources;
  }
  return eRet;
}

void omx_vdec::complete_pending_buffer_done_cbs()
{
  unsigned p1;
  unsigned p2;
  unsigned ident;
  omx_cmd_queue tmp_q, pending_bd_q;
  pthread_mutex_lock(&m_lock);
  // pop all pending GENERATE FDB from ftb queue
  while (m_ftb_q.m_size)
  {
    m_ftb_q.pop_entry(&p1,&p2,&ident);
    if(ident == OMX_COMPONENT_GENERATE_FBD)
    {
      pending_bd_q.insert_entry(p1,p2,ident);
    }
    else
    {
      tmp_q.insert_entry(p1,p2,ident);
    }
  }
  //return all non GENERATE FDB to ftb queue
  while(tmp_q.m_size)
  {
    tmp_q.pop_entry(&p1,&p2,&ident);
    m_ftb_q.insert_entry(p1,p2,ident);
  }
  // pop all pending GENERATE EDB from etb queue
  while (m_etb_q.m_size)
  {
    m_etb_q.pop_entry(&p1,&p2,&ident);
    if(ident == OMX_COMPONENT_GENERATE_EBD)
    {
      pending_bd_q.insert_entry(p1,p2,ident);
    }
    else
    {
      tmp_q.insert_entry(p1,p2,ident);
    }
  }
  //return all non GENERATE FDB to etb queue
  while(tmp_q.m_size)
  {
    tmp_q.pop_entry(&p1,&p2,&ident);
    m_etb_q.insert_entry(p1,p2,ident);
  }
  pthread_mutex_unlock(&m_lock);
  // process all pending buffer dones
  while(pending_bd_q.m_size)
  {
    pending_bd_q.pop_entry(&p1,&p2,&ident);
    switch(ident)
    {
      case OMX_COMPONENT_GENERATE_EBD:
        if(empty_buffer_done(&m_cmp, (OMX_BUFFERHEADERTYPE *)p1) != OMX_ErrorNone)
        {
          DEBUG_PRINT_ERROR("\nERROR: empty_buffer_done() failed!\n");
          omx_report_error ();
        }
        break;

      case OMX_COMPONENT_GENERATE_FBD:
        if(fill_buffer_done(&m_cmp, (OMX_BUFFERHEADERTYPE *)p1) != OMX_ErrorNone )
        {
          DEBUG_PRINT_ERROR("\nERROR: fill_buffer_done() failed!\n");
          omx_report_error ();
        }
        break;
    }
  }
}

void omx_vdec::set_frame_rate(OMX_S64 act_timestamp)
{
  OMX_U32 new_frame_interval = 0;
  struct vdec_ioctl_msg ioctl_msg = {NULL, NULL};
  if (VALID_TS(act_timestamp) && VALID_TS(prev_ts) && act_timestamp != prev_ts
     && (((act_timestamp > prev_ts )? act_timestamp - prev_ts: prev_ts-act_timestamp)>2000))
  {
    new_frame_interval = (act_timestamp > prev_ts)?
                          act_timestamp - prev_ts :
                          prev_ts - act_timestamp;
    if (new_frame_interval < frm_int || frm_int == 0)
    {
      frm_int = new_frame_interval;
      if(frm_int)
      {
        drv_ctx.frame_rate.fps_numerator = 1e6;
        drv_ctx.frame_rate.fps_denominator = frm_int;
        DEBUG_PRINT_LOW("set_frame_rate: frm_int(%u) fps(%f)",
                         frm_int, drv_ctx.frame_rate.fps_numerator /
                         (float)drv_ctx.frame_rate.fps_denominator);
        ioctl_msg.in = &drv_ctx.frame_rate;
        if (/*ioctl (drv_ctx.video_driver_fd, VDEC_IOCTL_SET_FRAME_RATE,
                  (void*)&ioctl_msg) < */0)
        {
          DEBUG_PRINT_ERROR("Setting frame rate failed");
        }
      }
    }
  }
  prev_ts = act_timestamp;
}

void omx_vdec::adjust_timestamp(OMX_S64 &act_timestamp)
{
  if (rst_prev_ts && VALID_TS(act_timestamp))
  {
    prev_ts = act_timestamp;
    rst_prev_ts = false;
  }
  else if (VALID_TS(prev_ts))
  {
    bool codec_cond = (drv_ctx.timestamp_adjust)?
                      (!VALID_TS(act_timestamp) || (((act_timestamp > prev_ts)?
                      (act_timestamp - prev_ts):(prev_ts - act_timestamp)) <= 2000)):
                      (!VALID_TS(act_timestamp) || act_timestamp == prev_ts);
    if(frm_int > 0 && codec_cond)
    {
      DEBUG_PRINT_LOW("adjust_timestamp: original ts[%lld]", act_timestamp);
      act_timestamp = prev_ts + frm_int;
      DEBUG_PRINT_LOW("adjust_timestamp: predicted ts[%lld]", act_timestamp);
      prev_ts = act_timestamp;
    }
    else
      set_frame_rate(act_timestamp);
  }
  else if (frm_int > 0)           // In this case the frame rate was set along
  {                               // with the port definition, start ts with 0
    act_timestamp = prev_ts = 0;  // and correct if a valid ts is received.
    rst_prev_ts = true;
  }
}

void omx_vdec::handle_extradata(OMX_BUFFERHEADERTYPE *p_buf_hdr)
{
  OMX_OTHER_EXTRADATATYPE *p_extra = NULL, *p_sei = NULL, *p_vui = NULL;
  OMX_U32 num_conceal_MB = 0;
  OMX_S64 ts_in_sei = 0;
  OMX_U32 frame_rate = 0;
  p_extra = (OMX_OTHER_EXTRADATATYPE *)
           ((unsigned)(p_buf_hdr->pBuffer + p_buf_hdr->nOffset +
            p_buf_hdr->nFilledLen + 3)&(~3));
  if ((OMX_U8*)p_extra > (p_buf_hdr->pBuffer + p_buf_hdr->nAllocLen))
    p_extra = NULL;
  if (drv_ctx.extradata && (p_buf_hdr->nFlags & OMX_BUFFERFLAG_EXTRADATA))
  {
    // Process driver extradata
    while(p_extra && p_extra->eType != VDEC_EXTRADATA_NONE)
    {
      DEBUG_PRINT_LOW("handle_extradata : pBuf(%p) BufTS(%lld) Type(%x) DataSz(%u)",
           p_buf_hdr, p_buf_hdr->nTimeStamp, p_extra->eType, p_extra->nDataSize);
      if (p_extra->nSize < p_extra->nDataSize)
      {
        DEBUG_PRINT_ERROR(" \n Corrupt metadata Buffer size %d payload size %d",
                          p_extra->nSize, p_extra->nDataSize);
        p_extra = (OMX_OTHER_EXTRADATATYPE *) (((OMX_U8 *) p_extra) + p_extra->nSize);
        if ((OMX_U8*)p_extra > (p_buf_hdr->pBuffer + p_buf_hdr->nAllocLen) ||
            p_extra->nDataSize == 0)
          p_extra = NULL;
          continue;
      }
      if (p_extra->eType == VDEC_EXTRADATA_MB_ERROR_MAP)
      {
        if (client_extradata & OMX_FRAMEINFO_EXTRADATA)
          num_conceal_MB = count_MB_in_extradata(p_extra);
        if (client_extradata & VDEC_EXTRADATA_MB_ERROR_MAP)
          // Map driver extradata to corresponding OMX type
          p_extra->eType = (OMX_EXTRADATATYPE)OMX_ExtraDataConcealMB;
        else
          p_extra->eType = OMX_ExtraDataMax; // Invalid type to avoid expose this extradata to OMX client
        if (m_debug_concealedmb) {
            DEBUG_PRINT_HIGH("Concealed MB percentage is %u", num_conceal_MB);
        }
      }
      else if (p_extra->eType == VDEC_EXTRADATA_SEI)
      {
        p_sei = p_extra;

        h264_parser->parse_nal((OMX_U8*)p_sei->data, p_sei->nDataSize, NALU_TYPE_SEI);

        p_extra->eType = OMX_ExtraDataMax; // Invalid type to avoid expose this extradata to OMX client
      }
      else if (p_extra->eType == VDEC_EXTRADATA_VUI)
      {
        p_vui = p_extra;

        h264_parser->parse_nal((OMX_U8*)p_vui->data, p_vui->nDataSize, NALU_TYPE_VUI, false);

        p_extra->eType = OMX_ExtraDataMax; // Invalid type to avoid expose this extradata to OMX client
      }
      print_debug_extradata(p_extra);
      p_extra = (OMX_OTHER_EXTRADATATYPE *) (((OMX_U8 *) p_extra) + p_extra->nSize);
      if ((OMX_U8*)p_extra > (p_buf_hdr->pBuffer + p_buf_hdr->nAllocLen) ||
          p_extra->nDataSize == 0)
        p_extra = NULL;
    }
    if (!(client_extradata & VDEC_EXTRADATA_MB_ERROR_MAP))
    {
      // Driver extradata is only exposed if MB map is requested by client,
      // otherwise can be overwritten by omx extradata.
      p_extra = (OMX_OTHER_EXTRADATATYPE *)
               ((unsigned)(p_buf_hdr->pBuffer + p_buf_hdr->nOffset +
                p_buf_hdr->nFilledLen + 3)&(~3));
      p_buf_hdr->nFlags &= ~OMX_BUFFERFLAG_EXTRADATA;
    }
  }

#ifdef PROCESS_EXTRADATA_IN_OUTPUT_PORT
  if (drv_ctx.decoder_format == VDEC_CODECTYPE_H264)
  {
    if (client_extradata & OMX_TIMEINFO_EXTRADATA)
    {
      if (p_vui)
        h264_parser->parse_nal((OMX_U8*)p_vui->data, p_vui->nDataSize, NALU_TYPE_VUI, false);
      if (p_sei)
        h264_parser->parse_nal((OMX_U8*)p_sei->data, p_sei->nDataSize, NALU_TYPE_SEI);
      ts_in_sei = h264_parser->process_ts_with_sei_vui(p_buf_hdr->nTimeStamp);
      if (!VALID_TS(p_buf_hdr->nTimeStamp))
        p_buf_hdr->nTimeStamp = ts_in_sei;
    }
    else if ((client_extradata & OMX_FRAMEINFO_EXTRADATA) && p_sei)
      // If timeinfo is present frame info from SEI is already processed
      h264_parser->parse_nal((OMX_U8*)p_sei->data, p_sei->nDataSize, NALU_TYPE_SEI);
  }
#endif
   if ((client_extradata & OMX_INTERLACE_EXTRADATA) && p_extra &&
      ((OMX_U8*)p_extra + OMX_INTERLACE_EXTRADATA_SIZE) <
       (p_buf_hdr->pBuffer + p_buf_hdr->nAllocLen))
  {
    p_buf_hdr->nFlags |= OMX_BUFFERFLAG_EXTRADATA;
    append_interlace_extradata(p_extra,
         ((struct vdec_output_frameinfo *)p_buf_hdr->pOutputPortPrivate)->interlaced_format);
    p_extra = (OMX_OTHER_EXTRADATATYPE *) (((OMX_U8 *) p_extra) + p_extra->nSize);
  }
  if (client_extradata & OMX_FRAMEINFO_EXTRADATA && p_extra &&
      ((OMX_U8*)p_extra + OMX_FRAMEINFO_EXTRADATA_SIZE) <
       (p_buf_hdr->pBuffer + p_buf_hdr->nAllocLen))
  {
    p_buf_hdr->nFlags |= OMX_BUFFERFLAG_EXTRADATA;
    /* vui extra data (frame_rate) information */
    if (h264_parser)
        h264_parser->get_frame_rate(&frame_rate);
    append_frame_info_extradata(p_extra, num_conceal_MB,
        ((struct vdec_output_frameinfo *)p_buf_hdr->pOutputPortPrivate)->pic_type,
        p_buf_hdr->nTimeStamp, frame_rate, NULL);
    p_extra = (OMX_OTHER_EXTRADATATYPE *) (((OMX_U8 *) p_extra) + p_extra->nSize);
  }
  if ((client_extradata & OMX_PORTDEF_EXTRADATA) &&
       p_extra != NULL &&
      ((OMX_U8*)p_extra + OMX_PORTDEF_EXTRADATA_SIZE) <
       (p_buf_hdr->pBuffer + p_buf_hdr->nAllocLen))
  {
    p_buf_hdr->nFlags |= OMX_BUFFERFLAG_EXTRADATA;
    append_portdef_extradata(p_extra);
    p_extra = (OMX_OTHER_EXTRADATATYPE *) (((OMX_U8 *) p_extra) + p_extra->nSize);
  }
  if (p_buf_hdr->nFlags & OMX_BUFFERFLAG_EXTRADATA)
    if (p_extra &&
      ((OMX_U8*)p_extra + OMX_FRAMEINFO_EXTRADATA_SIZE) <
       (p_buf_hdr->pBuffer + p_buf_hdr->nAllocLen))
      append_terminator_extradata(p_extra);
    else
    {
      DEBUG_PRINT_ERROR("ERROR: Terminator extradata cannot be added");
      p_buf_hdr->nFlags &= ~OMX_BUFFERFLAG_EXTRADATA;
    }
}

OMX_ERRORTYPE omx_vdec::enable_extradata(OMX_U32 requested_extradata, bool enable)
{
  OMX_ERRORTYPE ret = OMX_ErrorNone;
  OMX_U32 driver_extradata = 0, extradata_size = 0;
  struct vdec_ioctl_msg ioctl_msg = {NULL, NULL};
  if(m_state != OMX_StateLoaded)
  {
     DEBUG_PRINT_ERROR("ERROR: enable extradata allowed in Loaded state only");
     return OMX_ErrorIncorrectStateOperation;
  }
  if (requested_extradata & OMX_FRAMEINFO_EXTRADATA)
    extradata_size += OMX_FRAMEINFO_EXTRADATA_SIZE;
  if (requested_extradata & OMX_INTERLACE_EXTRADATA)
    extradata_size += OMX_INTERLACE_EXTRADATA_SIZE;
  if (requested_extradata & OMX_PORTDEF_EXTRADATA)
  {
    extradata_size += OMX_PORTDEF_EXTRADATA_SIZE;
  }
  DEBUG_PRINT_ERROR("enable_extradata: actual[%x] requested[%x] enable[%d]",
    client_extradata, requested_extradata, enable);

  if (enable)
    requested_extradata |= client_extradata;
  else
  {
    requested_extradata = client_extradata & ~requested_extradata;
    extradata_size *= -1;
  }

  driver_extradata = requested_extradata & DRIVER_EXTRADATA_MASK;
  if (requested_extradata & OMX_FRAMEINFO_EXTRADATA)
    driver_extradata |= VDEC_EXTRADATA_MB_ERROR_MAP; // Required for conceal MB frame info
#ifdef PROCESS_EXTRADATA_IN_OUTPUT_PORT
  if (drv_ctx.decoder_format == VDEC_CODECTYPE_H264)
  {
    driver_extradata |= ((requested_extradata & OMX_FRAMEINFO_EXTRADATA)?
                          VDEC_EXTRADATA_SEI : 0); // Required for pan scan frame info
    driver_extradata |= ((requested_extradata & OMX_TIMEINFO_EXTRADATA)?
                          VDEC_EXTRADATA_VUI | VDEC_EXTRADATA_SEI : 0); //Required for time info
  }

#endif
  if (driver_extradata != drv_ctx.extradata)
  {
    client_extradata = requested_extradata;
    drv_ctx.extradata = driver_extradata;
    //ioctl_msg.in = &drv_ctx.extradata;
    //ioctl_msg.out = NULL;
    //if (ioctl(drv_ctx.video_driver_fd, VDEC_IOCTL_SET_EXTRADATA,
    //    (void*)&ioctl_msg) < 0)
    //{
      //  DEBUG_PRINT_ERROR("\nSet extradata failed");
      //  ret = OMX_ErrorUnsupportedSetting;
    //}   // else
    // ret = get_buffer_req(&drv_ctx.op_buf);
  }
  else if ((client_extradata & ~DRIVER_EXTRADATA_MASK) != (requested_extradata & ~DRIVER_EXTRADATA_MASK))
  {
    client_extradata = requested_extradata;
    drv_ctx.op_buf.buffer_size += extradata_size;
    // align the buffer size
    drv_ctx.op_buf.buffer_size = (drv_ctx.op_buf.buffer_size + drv_ctx.op_buf.alignment - 1)&(~(drv_ctx.op_buf.alignment - 1));
    DEBUG_PRINT_LOW("Aligned buffer size with exreadata = %d\n", drv_ctx.op_buf.buffer_size);
    if (!(client_extradata & ~DRIVER_EXTRADATA_MASK)) // If no omx extradata is required remove space for terminator
      drv_ctx.op_buf.buffer_size -= sizeof(OMX_OTHER_EXTRADATATYPE);
	  ret = set_buffer_req(&drv_ctx.op_buf);
  }
  return ret;
}

OMX_U32 omx_vdec::count_MB_in_extradata(OMX_OTHER_EXTRADATATYPE *extra)
{
  OMX_U32 num_MB = 0, byte_count = 0, num_MB_in_frame = 0;
  OMX_U8 *data_ptr = extra->data, data = 0;
  while (byte_count < extra->nDataSize)
  {
    data = *data_ptr;
    while (data)
    {
      num_MB += (data&0x01);
      data >>= 1;
    }
    data_ptr++;
    byte_count++;
  }
  num_MB_in_frame = ((drv_ctx.video_resolution.frame_width + 15) *
                     (drv_ctx.video_resolution.frame_height + 15)) >> 8;
  return ((num_MB_in_frame > 0)?(num_MB * 100 / num_MB_in_frame) : 0);
}

void omx_vdec::print_debug_extradata(OMX_OTHER_EXTRADATATYPE *extra)
{
  if (!m_debug_extradata)
     return;

  DEBUG_PRINT_HIGH(
    "============== Extra Data ==============\n"
    "           Size: %u \n"
    "        Version: %u \n"
    "      PortIndex: %u \n"
    "           Type: %x \n"
    "       DataSize: %u \n",
    extra->nSize, extra->nVersion.nVersion,
    extra->nPortIndex, extra->eType, extra->nDataSize);

  if (extra->eType == OMX_ExtraDataInterlaceFormat)
  {
    OMX_STREAMINTERLACEFORMAT *intfmt = (OMX_STREAMINTERLACEFORMAT *)extra->data;
    DEBUG_PRINT_HIGH(
      "------ Interlace Format ------\n"
      "                Size: %u \n"
      "             Version: %u \n"
      "           PortIndex: %u \n"
      " Is Interlace Format: %u \n"
      "   Interlace Formats: %u \n"
      "=========== End of Interlace ===========\n",
      intfmt->nSize, intfmt->nVersion.nVersion, intfmt->nPortIndex,
      intfmt->bInterlaceFormat, intfmt->nInterlaceFormats);
  }
  else if (extra->eType == OMX_ExtraDataFrameInfo)
  {
    OMX_QCOM_EXTRADATA_FRAMEINFO *fminfo = (OMX_QCOM_EXTRADATA_FRAMEINFO *)extra->data;

    DEBUG_PRINT_HIGH(
      "-------- Frame Format --------\n"
      "             Picture Type: %u \n"
      "           Interlace Type: %u \n"
      " Pan Scan Total Frame Num: %u \n"
      "   Concealed Macro Blocks: %u \n"
      "               frame rate: %u \n"
      "           Aspect Ratio X: %u \n"
      "           Aspect Ratio Y: %u \n",
      fminfo->ePicType,
      fminfo->interlaceType,
      fminfo->panScan.numWindows,
      fminfo->nConcealedMacroblocks,
      fminfo->nFrameRate,
      fminfo->aspectRatio.aspectRatioX,
      fminfo->aspectRatio.aspectRatioY);

    for (int i = 0; i < fminfo->panScan.numWindows; i++)
    {
      DEBUG_PRINT_HIGH(
        "------------------------------\n"
        "     Pan Scan Frame Num: %d \n"
        "            Rectangle x: %d \n"
        "            Rectangle y: %d \n"
        "           Rectangle dx: %d \n"
        "           Rectangle dy: %d \n",
        i, fminfo->panScan.window[i].x, fminfo->panScan.window[i].y,
        fminfo->panScan.window[i].dx, fminfo->panScan.window[i].dy);
    }

    DEBUG_PRINT_HIGH("========= End of Frame Format ==========");
  }
  else if (extra->eType == OMX_ExtraDataNone)
  {
    DEBUG_PRINT_HIGH("========== End of Terminator ===========");
  }
  else
  {
    DEBUG_PRINT_HIGH("======= End of Driver Extradata ========");
  }
}

void omx_vdec::append_interlace_extradata(OMX_OTHER_EXTRADATATYPE *extra,
                                          OMX_U32 interlaced_format_type)
{
  OMX_STREAMINTERLACEFORMAT *interlace_format;
  OMX_U32 mbaff = 0;
  extra->nSize = OMX_INTERLACE_EXTRADATA_SIZE;
  extra->nVersion.nVersion = OMX_SPEC_VERSION;
  extra->nPortIndex = OMX_CORE_OUTPUT_PORT_INDEX;
  extra->eType = (OMX_EXTRADATATYPE)OMX_ExtraDataInterlaceFormat;
  extra->nDataSize = sizeof(OMX_STREAMINTERLACEFORMAT);
  interlace_format = (OMX_STREAMINTERLACEFORMAT *)extra->data;
  interlace_format->nSize = sizeof(OMX_STREAMINTERLACEFORMAT);
  interlace_format->nVersion.nVersion = OMX_SPEC_VERSION;
  interlace_format->nPortIndex = OMX_CORE_OUTPUT_PORT_INDEX;
  mbaff = (h264_parser)? (h264_parser->is_mbaff()): false;
  if ((interlaced_format_type == VDEC_InterlaceFrameProgressive)  && !mbaff)
  {
    interlace_format->bInterlaceFormat = OMX_FALSE;
    interlace_format->nInterlaceFormats = OMX_InterlaceFrameProgressive;
    drv_ctx.interlace = VDEC_InterlaceFrameProgressive;
  }
  else
  {
    interlace_format->bInterlaceFormat = OMX_TRUE;
    interlace_format->nInterlaceFormats = OMX_InterlaceInterleaveFrameTopFieldFirst;
    drv_ctx.interlace = VDEC_InterlaceInterleaveFrameTopFieldFirst;
  }
  print_debug_extradata(extra);
}


void omx_vdec::append_frame_info_extradata(OMX_OTHER_EXTRADATATYPE *extra,
    OMX_U32 num_conceal_mb, OMX_U32 picture_type, OMX_S64 timestamp, OMX_U32 frame_rate,
	vdec_aspectratioinfo *aspect_ratio_info)
{
  OMX_QCOM_EXTRADATA_FRAMEINFO *frame_info = NULL;
  extra->nSize = OMX_FRAMEINFO_EXTRADATA_SIZE;
  extra->nVersion.nVersion = OMX_SPEC_VERSION;
  extra->nPortIndex = OMX_CORE_OUTPUT_PORT_INDEX;
  extra->eType = (OMX_EXTRADATATYPE)OMX_ExtraDataFrameInfo;
  extra->nDataSize = sizeof(OMX_QCOM_EXTRADATA_FRAMEINFO);
  frame_info = (OMX_QCOM_EXTRADATA_FRAMEINFO *)extra->data;
  switch (picture_type)
  {
    case PICTURE_TYPE_I:
      frame_info->ePicType = OMX_VIDEO_PictureTypeI;
    break;
    case PICTURE_TYPE_P:
      frame_info->ePicType = OMX_VIDEO_PictureTypeP;
    break;
    case PICTURE_TYPE_B:
      frame_info->ePicType = OMX_VIDEO_PictureTypeB;
    break;
    default:
       frame_info->ePicType = (OMX_VIDEO_PICTURETYPE)0;
  }
  if (drv_ctx.interlace == VDEC_InterlaceInterleaveFrameTopFieldFirst)
    frame_info->interlaceType = OMX_QCOM_InterlaceInterleaveFrameTopFieldFirst;
  else if (drv_ctx.interlace == VDEC_InterlaceInterleaveFrameBottomFieldFirst)
    frame_info->interlaceType = OMX_QCOM_InterlaceInterleaveFrameBottomFieldFirst;
  else
    frame_info->interlaceType = OMX_QCOM_InterlaceFrameProgressive;
  memset(&frame_info->panScan,0,sizeof(frame_info->panScan));
  memset(&frame_info->aspectRatio, 0, sizeof(frame_info->aspectRatio));
  if (drv_ctx.decoder_format == VDEC_CODECTYPE_H264)
  {
    h264_parser->fill_pan_scan_data(&frame_info->panScan, timestamp);
    h264_parser->fill_aspect_ratio_info(&frame_info->aspectRatio);
  }
  frame_info->nConcealedMacroblocks = num_conceal_mb;
  frame_info->nFrameRate = frame_rate;
  print_debug_extradata(extra);
}

void omx_vdec::append_portdef_extradata(OMX_OTHER_EXTRADATATYPE *extra)
{
  OMX_PARAM_PORTDEFINITIONTYPE *portDefn = NULL;
  extra->nSize = OMX_PORTDEF_EXTRADATA_SIZE;
  extra->nVersion.nVersion = OMX_SPEC_VERSION;
  extra->nPortIndex = OMX_CORE_OUTPUT_PORT_INDEX;
  extra->eType = (OMX_EXTRADATATYPE)OMX_ExtraDataPortDef;
  extra->nDataSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
  portDefn = (OMX_PARAM_PORTDEFINITIONTYPE *)extra->data;
  *portDefn = m_port_def;
  DEBUG_PRINT_LOW("append_portdef_extradata height = %u width = %u stride = %u"
     "sliceheight = %u \n",portDefn->format.video.nFrameHeight,
     portDefn->format.video.nFrameWidth,
     portDefn->format.video.nStride,
     portDefn->format.video.nSliceHeight);
}

void omx_vdec::append_terminator_extradata(OMX_OTHER_EXTRADATATYPE *extra)
{
  extra->nSize = sizeof(OMX_OTHER_EXTRADATATYPE);
  extra->nVersion.nVersion = OMX_SPEC_VERSION;
  extra->eType = OMX_ExtraDataNone;
  extra->nDataSize = 0;
  extra->data[0] = 0;

  print_debug_extradata(extra);
}

OMX_ERRORTYPE  omx_vdec::allocate_desc_buffer(OMX_U32 index)
{
  OMX_ERRORTYPE eRet = OMX_ErrorNone;
  if (index >= drv_ctx.ip_buf.actualcount)
  {
    DEBUG_PRINT_ERROR("\nERROR:Desc Buffer Index not found");
    return OMX_ErrorInsufficientResources;
  }
  if (m_desc_buffer_ptr == NULL)
  {
    m_desc_buffer_ptr = (desc_buffer_hdr*) \
                     calloc( (sizeof(desc_buffer_hdr)),
                     drv_ctx.ip_buf.actualcount);
    if (m_desc_buffer_ptr == NULL)
    {
      DEBUG_PRINT_ERROR("\n m_desc_buffer_ptr Allocation failed ");
      return OMX_ErrorInsufficientResources;
    }
  }

  m_desc_buffer_ptr[index].buf_addr = (unsigned char *)malloc (DESC_BUFFER_SIZE * sizeof(OMX_U8));
  if (m_desc_buffer_ptr[index].buf_addr == NULL)
  {
    DEBUG_PRINT_ERROR("\ndesc buffer Allocation failed ");
    return OMX_ErrorInsufficientResources;
  }

  return eRet;
}

void omx_vdec::insert_demux_addr_offset(OMX_U32 address_offset)
{
  DEBUG_PRINT_LOW("Inserting address offset (%d) at idx (%d)", address_offset,m_demux_entries);
  if (m_demux_entries < 8192)
  {
    m_demux_offsets[m_demux_entries++] = address_offset;
  }
  return;
}

void omx_vdec::extract_demux_addr_offsets(OMX_BUFFERHEADERTYPE *buf_hdr)
{
  OMX_U32 bytes_to_parse = buf_hdr->nFilledLen;
  OMX_U8 *buf = buf_hdr->pBuffer + buf_hdr->nOffset;
  OMX_U32 index = 0;

  m_demux_entries = 0;

  while (index < bytes_to_parse)
  {
    if ( ((buf[index] == 0x00) && (buf[index+1] == 0x00) &&
          (buf[index+2] == 0x00) && (buf[index+3] == 0x01)) ||
         ((buf[index] == 0x00) && (buf[index+1] == 0x00) &&
          (buf[index+2] == 0x01)) )
    {
      //Found start code, insert address offset
      insert_demux_addr_offset(index);
      if (buf[index+2] == 0x01) // 3 byte start code
        index += 3;
      else                      //4 byte start code
        index += 4;
    }
    else
      index++;
  }
  DEBUG_PRINT_LOW("Extracted (%d) demux entry offsets",m_demux_entries);
  return;
}

OMX_ERRORTYPE omx_vdec::handle_demux_data(OMX_BUFFERHEADERTYPE *p_buf_hdr)
{
  //fix this, handle 3 byte start code, vc1 terminator entry
  OMX_U8 *p_demux_data = NULL;
  OMX_U32 desc_data = 0;
  OMX_U32 start_addr = 0;
  OMX_U32 nal_size = 0;
  OMX_U32 suffix_byte = 0;
  OMX_U32 demux_index = 0;
  OMX_U32 buffer_index = 0;

  if (m_desc_buffer_ptr == NULL)
  {
    DEBUG_PRINT_ERROR("m_desc_buffer_ptr is NULL. Cannot append demux entries.");
    return OMX_ErrorBadParameter;
  }

  buffer_index = p_buf_hdr - ((OMX_BUFFERHEADERTYPE *)m_inp_mem_ptr);
  if (buffer_index > drv_ctx.ip_buf.actualcount)
  {
    DEBUG_PRINT_ERROR("handle_demux_data:Buffer index is incorrect (%d)", buffer_index);
    return OMX_ErrorBadParameter;
  }

  p_demux_data = (OMX_U8 *) m_desc_buffer_ptr[buffer_index].buf_addr;

  if ( ((OMX_U8*)p_demux_data == NULL) ||
      ((m_demux_entries * 16) + 1) > DESC_BUFFER_SIZE)
  {
    DEBUG_PRINT_ERROR("Insufficient buffer. Cannot append demux entries.");
    return OMX_ErrorBadParameter;
  }
  else
  {
    for (; demux_index < m_demux_entries; demux_index++)
    {
      desc_data = 0;
      start_addr = m_demux_offsets[demux_index];
      if (p_buf_hdr->pBuffer[m_demux_offsets[demux_index] + 2] == 0x01)
      {
        suffix_byte = p_buf_hdr->pBuffer[m_demux_offsets[demux_index] + 3];
      }
      else
      {
        suffix_byte = p_buf_hdr->pBuffer[m_demux_offsets[demux_index] + 4];
      }
      if (demux_index < (m_demux_entries - 1))
      {
        nal_size = m_demux_offsets[demux_index + 1] - m_demux_offsets[demux_index] - 2;
      }
      else
      {
        nal_size = p_buf_hdr->nFilledLen - m_demux_offsets[demux_index] - 2;
      }
      DEBUG_PRINT_LOW("Start_addr(%p), suffix_byte(0x%x),nal_size(%d),demux_index(%d)",
                        start_addr,
                        suffix_byte,
                        nal_size,
                        demux_index);
      desc_data = (start_addr >> 3) << 1;
      desc_data |= (start_addr & 7) << 21;
      desc_data |= suffix_byte << 24;

      memcpy(p_demux_data, &desc_data, sizeof(OMX_U32));
      memcpy(p_demux_data + 4, &nal_size, sizeof(OMX_U32));
      memset(p_demux_data + 8, 0, sizeof(OMX_U32));
      memset(p_demux_data + 12, 0, sizeof(OMX_U32));

      p_demux_data += 16;
    }
    if (codec_type_parse == CODEC_TYPE_VC1)
    {
      DEBUG_PRINT_LOW("VC1 terminator entry");
      desc_data = 0;
      desc_data = 0x82 << 24;
      memcpy(p_demux_data, &desc_data, sizeof(OMX_U32));
      memset(p_demux_data + 4, 0, sizeof(OMX_U32));
      memset(p_demux_data + 8, 0, sizeof(OMX_U32));
      memset(p_demux_data + 12, 0, sizeof(OMX_U32));
      p_demux_data += 16;
      m_demux_entries++;
    }
    //Add zero word to indicate end of descriptors
    memset(p_demux_data, 0, sizeof(OMX_U32));

    m_desc_buffer_ptr[buffer_index].desc_data_size = (m_demux_entries * 16) + sizeof(OMX_U32);
    DEBUG_PRINT_LOW("desc table data size=%d", m_desc_buffer_ptr[buffer_index].desc_data_size);
  }
  memset(m_demux_offsets, 0, ( sizeof(OMX_U32) * 8192) );
  m_demux_entries = 0;
  DEBUG_PRINT_LOW("Demux table complete!");
  return OMX_ErrorNone;
}

#if 0
OMX_ERRORTYPE omx_vdec::createDivxDrmContext( OMX_PTR drmHandle )
{
     OMX_ERRORTYPE err = OMX_ErrorNone;
     if( drmHandle == NULL ) {
        DEBUG_PRINT_HIGH("\n This clip is not DRM encrypted");
        iDivXDrmDecrypt = NULL;
        return err;
     }

     iDivXDrmDecrypt = DivXDrmDecrypt::Create( drmHandle );
     if (iDivXDrmDecrypt) {
          DEBUG_PRINT_LOW("\nCreated DIVX DRM, now calling Init");
          OMX_ERRORTYPE err = iDivXDrmDecrypt->Init();
          if(err!=OMX_ErrorNone) {
            DEBUG_PRINT_ERROR("\nERROR:iDivXDrmDecrypt->Init %d", err);
            delete iDivXDrmDecrypt;
            iDivXDrmDecrypt = NULL;
          }
     }
     else {
          DEBUG_PRINT_ERROR("\nUnable to Create DIVX DRM");
          return OMX_ErrorUndefined;
     }
     return err;
}
#endif

