/*--------------------------------------------------------------------------
Copyright (c) 2010 - 2013, The Linux Foundation. All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

    * Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
#define JB_MR1
#include <string.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#ifndef JB_MR1
#include <ihwc.h>
#include <binder/IServiceManager.h>
#endif
#include "power_module.h"
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

#if defined (_ANDROID_ICS_)
#include <genlock.h>
#include <qdMetaData.h>
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
#define MAX_NUM_SPS 32
#define MAX_NUM_PPS 256
#define MAX_INPUT_ERROR (MAX_NUM_SPS + MAX_NUM_PPS)
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

#ifdef USE_ION
#define MEM_DEVICE "/dev/ion"
#ifdef MAX_RES_720P
#define MEM_HEAP_ID ION_CAMERA_HEAP_ID
#else
#define MEM_HEAP_ID ION_CP_MM_HEAP_ID
#endif
#elif MAX_RES_720P
#define MEM_DEVICE "/dev/pmem_adsp"
#elif MAX_RES_1080P_EBI
#define MEM_DEVICE "/dev/pmem_adsp"
#elif MAX_RES_1080P
#define MEM_DEVICE "/dev/pmem_smipool"
#endif


#ifdef MAX_RES_720P
#define DEVICE_SCRATCH 0
#else
#define DEVICE_SCRATCH 64
#endif

/*
#ifdef _ANDROID_
extern "C"{
#include<utils/Log.h>
}
#endif//_ANDROID_
 */

#ifndef _ANDROID_
#include <glib.h>
#define strlcpy g_strlcpy
#endif

#define Log2(number, power)  { OMX_U32 temp = number; power = 0; while( (0 == (temp & 0x1)) &&  power < 16) { temp >>=0x1; power++; } }
#define Q16ToFraction(q,num,den) { OMX_U32 power; Log2(q,power);  num = q >> power; den = 0x1 << (16 - power); }

void* async_message_thread (void *input)
{
    struct vdec_ioctl_msg ioctl_msg;
    struct vdec_msginfo vdec_msg;
    omx_vdec *omx = reinterpret_cast<omx_vdec*>(input);
    int error_code = 0;
    DEBUG_PRINT_HIGH("omx_vdec: Async thread start");
    prctl(PR_SET_NAME, (unsigned long)"VideoDecCallBackThread", 0, 0, 0);

    while (1) {
        ioctl_msg.in = NULL;
        ioctl_msg.out = (void*)&vdec_msg;
        /*Wait for a message from the video decoder driver*/
        error_code = ioctl ( omx->drv_ctx.video_driver_fd,VDEC_IOCTL_GET_NEXT_MSG,
                (void*)&ioctl_msg);

        if (error_code == -512) { // ERESTARTSYS
            DEBUG_PRINT_ERROR("ERESTARTSYS received in ioctl read next msg!");
        } else if (error_code < 0) {
            DEBUG_PRINT_ERROR("Error in ioctl read next msg");
            break;
        }        /*Call Instance specific process function*/
        else if (omx->async_message_process(input,&vdec_msg) < 0) {
            DEBUG_PRINT_ERROR("ERROR:Wrong ioctl message");
        }
    }

    DEBUG_PRINT_HIGH("omx_vdec: Async thread stop");
    return NULL;
}

void* message_thread(void *input)
{
    omx_vdec* omx = reinterpret_cast<omx_vdec*>(input);
    unsigned char id;
    int n;

    DEBUG_PRINT_HIGH("omx_vdec: message thread start");
    prctl(PR_SET_NAME, (unsigned long)"VideoDecMsgThread", 0, 0, 0);

    while (1) {

        n = read(omx->m_pipe_in, &id, 1);

        if (0 == n) {
            break;
        }

        if (1 == n) {
            omx->process_event_cb(omx, id);
        }

        if ((n < 0) && (errno != EINTR)) {
            DEBUG_PRINT_ERROR("ERROR: read from pipe failed, ret %d errno %d", n, errno);
            break;
        }
    }

    DEBUG_PRINT_HIGH("omx_vdec: message thread stop");
    return 0;
}

void post_message(omx_vdec *omx, unsigned char id)
{
    int ret_value;
    DEBUG_PRINT_LOW("omx_vdec: post_message %d pipe out%d", id,omx->m_pipe_out);
    ret_value = write(omx->m_pipe_out, &id, 1);
    DEBUG_PRINT_LOW("post_message to pipe done %d",ret_value);
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

    if (m_size < OMX_CORE_CONTROL_CMDQ_SIZE) {
        m_q[m_write].id       = id;
        m_q[m_write].param1   = p1;
        m_q[m_write].param2   = p2;
        m_write++;
        m_size ++;

        if (m_write >= OMX_CORE_CONTROL_CMDQ_SIZE) {
            m_write = 0;
        }
    } else {
        ret = false;
        DEBUG_PRINT_ERROR("ERROR: %s()::Command Queue Full", __func__);
    }

    return ret;
}

// omx cmd queue pop
bool omx_vdec::omx_cmd_queue::pop_entry(unsigned *p1, unsigned *p2, unsigned *id)
{
    bool ret = true;

    if (m_size > 0) {
        *id = m_q[m_read].id;
        *p1 = m_q[m_read].param1;
        *p2 = m_q[m_read].param2;
        // Move the read pointer ahead
        ++m_read;
        --m_size;

        if (m_read >= OMX_CORE_CONTROL_CMDQ_SIZE) {
            m_read = 0;
        }
    } else {
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
    for ( ; idx < MAX_NUM_INPUT_OUTPUT_BUFFERS; idx++) {
        if (!m_ts_arr_list[idx].valid) {
            //found invalid or empty entry, save timestamp
            m_ts_arr_list[idx].valid = true;
            m_ts_arr_list[idx].timestamp = ts;
            DEBUG_PRINT_LOW("Insert_ts(): Inserting TIMESTAMP (%lld) at idx (%d)",
                    ts, idx);
            break;
        }
    }

    if (idx == MAX_NUM_INPUT_OUTPUT_BUFFERS) {
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

    for ( ; idx < MAX_NUM_INPUT_OUTPUT_BUFFERS; idx++) {

        if (m_ts_arr_list[idx].valid) {
            //found valid entry, save index
            if (min_idx < 0) {
                //first valid entry
                min_ts = m_ts_arr_list[idx].timestamp;
                min_idx = idx;
            } else if (m_ts_arr_list[idx].timestamp < min_ts) {
                min_ts = m_ts_arr_list[idx].timestamp;
                min_idx = idx;
            }
        }

    }

    if (min_idx < 0) {
        //no valid entries found
        DEBUG_PRINT_LOW("Timestamp array list is empty. Unsuccessful pop");
        ts = 0;
        ret = false;
    } else {
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

    for ( ; idx < MAX_NUM_INPUT_OUTPUT_BUFFERS; idx++) {
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
    m_ion_device_fd = devicefd;
    m_ion_handle = handle;
    MemoryHeapBase::init(ionMapfd, base, size, 0, MEM_DEVICE);
    //ionInit(devicefd, base, size, 0 , MEM_DEVICE,handle,ionMapfd);
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
    m_disp_hor_size(0),
    m_disp_vert_size(0),
#ifdef _ANDROID_
    m_heap_ptr(NULL),
    m_heap_count(0),
    m_enable_android_native_buffers(OMX_FALSE),
    m_use_android_native_buffers(OMX_FALSE),
#endif
    in_reconfig(false),
    m_use_output_pmem(OMX_FALSE),
    m_out_mem_region_smi(OMX_FALSE),
    m_out_pvt_entry_pmem(OMX_FALSE),
    secure_mode(false),
    external_meta_buffer(false),
    external_meta_buffer_iommu(false)
#ifdef _ANDROID_
,iDivXDrmDecrypt(NULL)
#endif
    ,m_desc_buffer_ptr(NULL)
    ,m_extradata(NULL)
,m_power_hinted(false)
{
    /* Assumption is that , to begin with , we have all the frames with decoder */
    DEBUG_PRINT_HIGH("In OMX vdec Constructor");
#ifdef _ANDROID_
    char property_value[PROPERTY_VALUE_MAX] = {0};
    property_get("vidc.dec.debug.perf", property_value, "0");
    perf_flag = atoi(property_value);

    if (perf_flag) {
        DEBUG_PRINT_HIGH("vidc.dec.debug.perf is %d", perf_flag);
        dec_time.start();
        proc_frms = latency = 0;
    }

    property_value[0] = NULL;
    property_get("vidc.dec.debug.ts", property_value, "0");
    m_debug_timestamp = atoi(property_value);
    DEBUG_PRINT_HIGH("vidc.dec.debug.ts value is %d",m_debug_timestamp);

    if (m_debug_timestamp) {
        time_stamp_dts.set_timestamp_reorder_mode(true);
        time_stamp_dts.enable_debug_print(true);
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
    msg_thread_created = false;
    async_thread_created = false;
#ifdef _ANDROID_ICS_
    memset(&native_buffer, 0 ,(sizeof(struct nativebuffer) * MAX_NUM_INPUT_OUTPUT_BUFFERS));
#endif
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
    m_fill_output_msg = OMX_COMPONENT_GENERATE_FTB;
    client_buffers.set_vdec_client(this);
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

    if (m_pipe_in) close(m_pipe_in);

    if (m_pipe_out) close(m_pipe_out);

    m_pipe_in = -1;
    m_pipe_out = -1;

    if (msg_thread_created) {
        DEBUG_PRINT_HIGH("Waiting on OMX Msg Thread exit");
        pthread_join(msg_thread_id,NULL);
    }

    if (async_thread_created) {
        DEBUG_PRINT_HIGH("Waiting on OMX Async Thread exit");
        pthread_join(async_thread_id,NULL);
    }

    DEBUG_PRINT_HIGH("Calling close() on Video Driver");
    close (drv_ctx.video_driver_fd);
    drv_ctx.video_driver_fd = -1;

    pthread_mutex_destroy(&m_lock);
    sem_destroy(&m_cmd_lock);
#ifdef _ANDROID_

    if (perf_flag) {
        DEBUG_PRINT_HIGH("--> TOTAL PROCESSING TIME");
        dec_time.end();
    }

#endif /* _ANDROID_ */
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

    if (!pThis) {
        DEBUG_PRINT_ERROR("ERROR: %s()::Context is incorrect, bailing out",
                __func__);
        return;
    }

    // Protect the shared queue data structure
    do {
        /*Read the message id's from the queue*/
        pthread_mutex_lock(&pThis->m_lock);
        qsize = pThis->m_cmd_q.m_size;

        if (qsize) {
            pThis->m_cmd_q.pop_entry(&p1,&p2,&ident);
        }

        if (qsize == 0 && pThis->m_state != OMX_StatePause) {
            qsize = pThis->m_ftb_q.m_size;

            if (qsize) {
                pThis->m_ftb_q.pop_entry(&p1,&p2,&ident);
            }
        }

        if (qsize == 0 && pThis->m_state != OMX_StatePause) {
            qsize = pThis->m_etb_q.m_size;

            if (qsize) {
                pThis->m_etb_q.pop_entry(&p1,&p2,&ident);
            }
        }

        pthread_mutex_unlock(&pThis->m_lock);

        /*process message if we have one*/
        if (qsize > 0) {
            id = ident;

            switch (id) {
                case OMX_COMPONENT_GENERATE_EVENT:

                    if (pThis->m_cb.EventHandler) {
                        switch (p1) {
                            case OMX_CommandStateSet:
                                pThis->m_state = (OMX_STATETYPE) p2;
                                DEBUG_PRINT_HIGH("OMX_CommandStateSet complete, m_state = %d",
                                        pThis->m_state);
                                pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                        OMX_EventCmdComplete, p1, p2, NULL);
                                break;

                            case OMX_EventError:

                                if (p2 == OMX_StateInvalid) {
                                    DEBUG_PRINT_ERROR("OMX_EventError: p2 is OMX_StateInvalid");
                                    pThis->m_state = (OMX_STATETYPE) p2;
                                    pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                            OMX_EventError, OMX_ErrorInvalidState, p2, NULL);
                                } else if (p2 == OMX_ErrorHardware) {
                                    pThis->omx_report_error();
                                } else {
                                    pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                            OMX_EventError, p2, NULL, NULL );
                                }

                                break;

                            case OMX_CommandPortDisable:
                                DEBUG_PRINT_HIGH("OMX_CommandPortDisable complete for port [%d]", p2);

                                if (BITMASK_PRESENT(&pThis->m_flags,
                                            OMX_COMPONENT_OUTPUT_FLUSH_IN_DISABLE_PENDING)) {
                                    BITMASK_SET(&pThis->m_flags, OMX_COMPONENT_DISABLE_OUTPUT_DEFERRED);
                                    break;
                                }

                                if (p2 == OMX_CORE_OUTPUT_PORT_INDEX && pThis->in_reconfig) {
                                    pThis->in_reconfig = false;
                                    pThis->drv_ctx.op_buf = pThis->op_buf_rcnfg;
                                    OMX_ERRORTYPE eRet = pThis->set_buffer_req(&pThis->drv_ctx.op_buf);

                                    if (eRet !=  OMX_ErrorNone) {
                                        DEBUG_PRINT_ERROR("set_buffer_req failed eRet = %d",eRet);
                                        pThis->omx_report_error();
                                        break;
                                    }
                                }

                                pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                        OMX_EventCmdComplete, p1, p2, NULL );
                                break;
                            case OMX_CommandPortEnable:
                                DEBUG_PRINT_HIGH("OMX_CommandPortEnable complete for port [%d]", p2);
                                pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,\
                                        OMX_EventCmdComplete, p1, p2, NULL );
                                break;

                            default:
                                pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                        OMX_EventCmdComplete, p1, p2, NULL );
                                break;

                        }
                    } else {
                        DEBUG_PRINT_ERROR("ERROR: %s()::EventHandler is NULL", __func__);
                    }

                    break;
                case OMX_COMPONENT_GENERATE_ETB_ARBITRARY:

                    if (pThis->empty_this_buffer_proxy_arbitrary((OMX_HANDLETYPE)p1,\
                                (OMX_BUFFERHEADERTYPE *)p2) != OMX_ErrorNone) {
                        DEBUG_PRINT_ERROR("empty_this_buffer_proxy_arbitrary failure");
                        pThis->omx_report_error ();
                    }

                    break;
                case OMX_COMPONENT_GENERATE_ETB:

                    if (pThis->empty_this_buffer_proxy((OMX_HANDLETYPE)p1,\
                                (OMX_BUFFERHEADERTYPE *)p2) != OMX_ErrorNone) {
                        DEBUG_PRINT_ERROR("empty_this_buffer_proxy failure");
                        pThis->omx_report_error ();
                    }

                    break;

                case OMX_COMPONENT_GENERATE_FTB:

                    if ( pThis->fill_this_buffer_proxy((OMX_HANDLETYPE)p1,\
                                (OMX_BUFFERHEADERTYPE *)p2) != OMX_ErrorNone) {
                        DEBUG_PRINT_ERROR("fill_this_buffer_proxy failure");
                        pThis->omx_report_error ();
                    }

                    break;

                case OMX_COMPONENT_GENERATE_COMMAND:
                    pThis->send_command_proxy(&pThis->m_cmp,(OMX_COMMANDTYPE)p1,\
                            (OMX_U32)p2,(OMX_PTR)NULL);
                    break;

                case OMX_COMPONENT_GENERATE_EBD:

                    if (p2 != VDEC_S_SUCCESS && p2 != VDEC_S_INPUT_BITSTREAM_ERR) {
                        DEBUG_PRINT_ERROR("OMX_COMPONENT_GENERATE_EBD failure");
                        pThis->omx_report_error ();
                    } else {
                        if (p2 == VDEC_S_INPUT_BITSTREAM_ERR && p1) {
                            pThis->m_inp_err_count++;
                            pThis->time_stamp_dts.remove_time_stamp(
                                    ((OMX_BUFFERHEADERTYPE *)p1)->nTimeStamp,
                                    (pThis->drv_ctx.interlace != VDEC_InterlaceFrameProgressive)
                                    ?true:false);
                        } else {
                            pThis->m_inp_err_count = 0;
                        }

                        if ( pThis->empty_buffer_done(&pThis->m_cmp,
                                    (OMX_BUFFERHEADERTYPE *)p1) != OMX_ErrorNone) {
                            DEBUG_PRINT_ERROR("empty_buffer_done failure");
                            pThis->omx_report_error ();
                        }

                        if (!pThis->arbitrary_bytes && pThis->m_inp_err_count > MAX_INPUT_ERROR) {
                            DEBUG_PRINT_ERROR("Input bitstream error for consecutive %d frames.", MAX_INPUT_ERROR);
                            pThis->omx_report_error ();
                        }
                    }

                    break;
                case OMX_COMPONENT_GENERATE_INFO_FIELD_DROPPED:
                    {
                        int64_t *timestamp = (int64_t *)p1;

                        if (p1) {
                            pThis->time_stamp_dts.remove_time_stamp(*timestamp,
                                    (pThis->drv_ctx.interlace != VDEC_InterlaceFrameProgressive)
                                    ?true:false);
                            free(timestamp);
                        }
                    }
                    break;
                case OMX_COMPONENT_GENERATE_FBD:

                    if (p2 != VDEC_S_SUCCESS) {
                        DEBUG_PRINT_ERROR("OMX_COMPONENT_GENERATE_FBD failure");
                        pThis->omx_report_error ();
                    } else if ( pThis->fill_buffer_done(&pThis->m_cmp,
                                (OMX_BUFFERHEADERTYPE *)p1) != OMX_ErrorNone ) {
                        DEBUG_PRINT_ERROR("fill_buffer_done failure");
                        pThis->omx_report_error ();
                    }

                    break;

                case OMX_COMPONENT_GENERATE_EVENT_INPUT_FLUSH:
                    DEBUG_PRINT_HIGH("Driver flush i/p Port complete");

                    if (!pThis->input_flush_progress) {
                        DEBUG_PRINT_ERROR("WARNING: Unexpected flush from driver");
                    } else {
                        pThis->execute_input_flush();

                        if (pThis->m_cb.EventHandler) {
                            if (p2 != VDEC_S_SUCCESS) {
                                DEBUG_PRINT_ERROR("OMX_COMPONENT_GENERATE_EVENT_INPUT_FLUSH failure");
                                pThis->omx_report_error ();
                            } else {
                                /*Check if we need generate event for Flush done*/
                                if (BITMASK_PRESENT(&pThis->m_flags,
                                            OMX_COMPONENT_INPUT_FLUSH_PENDING)) {
                                    BITMASK_CLEAR (&pThis->m_flags,OMX_COMPONENT_INPUT_FLUSH_PENDING);
                                    DEBUG_PRINT_LOW("Input Flush completed - Notify Client");
                                    pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                            OMX_EventCmdComplete,OMX_CommandFlush,
                                            OMX_CORE_INPUT_PORT_INDEX,NULL );
                                }

                                if (BITMASK_PRESENT(&pThis->m_flags,
                                            OMX_COMPONENT_IDLE_PENDING)) {
                                    if (!pThis->output_flush_progress) {
                                        DEBUG_PRINT_LOW("Output flush done hence issue stop");

                                        if (ioctl (pThis->drv_ctx.video_driver_fd,
                                                    VDEC_IOCTL_CMD_STOP,NULL ) < 0) {
                                            DEBUG_PRINT_ERROR("VDEC_IOCTL_CMD_STOP failed");
                                            pThis->omx_report_error ();
                                        }
                                    }
                                }
                            }
                        } else {
                            DEBUG_PRINT_ERROR("ERROR: %s()::EventHandler is NULL", __func__);
                        }
                    }

                    break;

                case OMX_COMPONENT_GENERATE_EVENT_OUTPUT_FLUSH:
                    DEBUG_PRINT_HIGH("Driver flush o/p Port complete");

                    if (!pThis->output_flush_progress) {
                        DEBUG_PRINT_ERROR("WARNING: Unexpected flush from driver");
                    } else {
                        pThis->execute_output_flush();

                        if (pThis->m_cb.EventHandler) {
                            if (p2 != VDEC_S_SUCCESS) {
                                DEBUG_PRINT_ERROR("OMX_COMPONENT_GENERATE_EVENT_OUTPUT_FLUSH failed");
                                pThis->omx_report_error ();
                            } else {
                                /*Check if we need generate event for Flush done*/
                                if (BITMASK_PRESENT(&pThis->m_flags,
                                            OMX_COMPONENT_OUTPUT_FLUSH_PENDING)) {
                                    DEBUG_PRINT_LOW("Notify Output Flush done");
                                    BITMASK_CLEAR (&pThis->m_flags,OMX_COMPONENT_OUTPUT_FLUSH_PENDING);
                                    pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                            OMX_EventCmdComplete,OMX_CommandFlush,
                                            OMX_CORE_OUTPUT_PORT_INDEX,NULL );
                                }

                                if (BITMASK_PRESENT(&pThis->m_flags,
                                            OMX_COMPONENT_OUTPUT_FLUSH_IN_DISABLE_PENDING)) {
                                    DEBUG_PRINT_LOW("Internal flush complete");
                                    BITMASK_CLEAR (&pThis->m_flags,
                                            OMX_COMPONENT_OUTPUT_FLUSH_IN_DISABLE_PENDING);

                                    if (BITMASK_PRESENT(&pThis->m_flags,
                                                OMX_COMPONENT_DISABLE_OUTPUT_DEFERRED)) {
                                        pThis->post_event(OMX_CommandPortDisable,
                                                OMX_CORE_OUTPUT_PORT_INDEX,
                                                OMX_COMPONENT_GENERATE_EVENT);
                                        BITMASK_CLEAR (&pThis->m_flags,
                                                OMX_COMPONENT_DISABLE_OUTPUT_DEFERRED);

                                    }
                                }

                                if (BITMASK_PRESENT(&pThis->m_flags ,OMX_COMPONENT_IDLE_PENDING)) {
                                    if (!pThis->input_flush_progress) {
                                        DEBUG_PRINT_LOW("Input flush done hence issue stop");

                                        if (ioctl (pThis->drv_ctx.video_driver_fd,
                                                    VDEC_IOCTL_CMD_STOP,NULL ) < 0) {
                                            DEBUG_PRINT_ERROR("VDEC_IOCTL_CMD_STOP failed");
                                            pThis->omx_report_error ();
                                        }
                                    }
                                }
                            }
                        } else {
                            DEBUG_PRINT_ERROR("ERROR: %s()::EventHandler is NULL", __func__);
                        }
                    }

                    break;

                case OMX_COMPONENT_GENERATE_START_DONE:
                    DEBUG_PRINT_HIGH("Rxd OMX_COMPONENT_GENERATE_START_DONE");

                    if (pThis->m_cb.EventHandler) {
                        if (p2 != VDEC_S_SUCCESS) {
                            DEBUG_PRINT_ERROR("OMX_COMPONENT_GENERATE_START_DONE Failure");
                            pThis->omx_report_error ();
                        } else {
                            DEBUG_PRINT_LOW("OMX_COMPONENT_GENERATE_START_DONE Success");

                            if (BITMASK_PRESENT(&pThis->m_flags,OMX_COMPONENT_EXECUTE_PENDING)) {
                                DEBUG_PRINT_LOW("Move to executing");
                                // Send the callback now
                                BITMASK_CLEAR((&pThis->m_flags),OMX_COMPONENT_EXECUTE_PENDING);
                                pThis->m_state = OMX_StateExecuting;
                                pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                        OMX_EventCmdComplete,OMX_CommandStateSet,
                                        OMX_StateExecuting, NULL);
                            } else if (BITMASK_PRESENT(&pThis->m_flags,
                                        OMX_COMPONENT_PAUSE_PENDING)) {
                                if (ioctl (pThis->drv_ctx.video_driver_fd,
                                            VDEC_IOCTL_CMD_PAUSE,NULL ) < 0) {
                                    DEBUG_PRINT_ERROR("VDEC_IOCTL_CMD_PAUSE failed");
                                    pThis->omx_report_error ();
                                }
                            }
                        }
                    } else {
                        DEBUG_PRINT_ERROR("Event Handler callback is NULL");
                    }

                    break;

                case OMX_COMPONENT_GENERATE_PAUSE_DONE:
                    DEBUG_PRINT_HIGH("Rxd OMX_COMPONENT_GENERATE_PAUSE_DONE");

                    if (pThis->m_cb.EventHandler) {
                        if (p2 != VDEC_S_SUCCESS) {
                            DEBUG_PRINT_ERROR("OMX_COMPONENT_GENERATE_PAUSE_DONE ret failed");
                            pThis->omx_report_error ();
                        } else {
                            pThis->complete_pending_buffer_done_cbs();

                            if (BITMASK_PRESENT(&pThis->m_flags,OMX_COMPONENT_PAUSE_PENDING)) {
                                DEBUG_PRINT_LOW("OMX_COMPONENT_GENERATE_PAUSE_DONE nofity");
                                //Send the callback now
                                BITMASK_CLEAR((&pThis->m_flags),OMX_COMPONENT_PAUSE_PENDING);
                                pThis->m_state = OMX_StatePause;
                                pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                        OMX_EventCmdComplete,OMX_CommandStateSet,
                                        OMX_StatePause, NULL);
                            }
                        }
                    } else {
                        DEBUG_PRINT_ERROR("ERROR: %s()::EventHandler is NULL", __func__);
                    }

                    break;

                case OMX_COMPONENT_GENERATE_RESUME_DONE:
                    DEBUG_PRINT_HIGH("Rxd OMX_COMPONENT_GENERATE_RESUME_DONE");

                    if (pThis->m_cb.EventHandler) {
                        if (p2 != VDEC_S_SUCCESS) {
                            DEBUG_PRINT_ERROR("OMX_COMPONENT_GENERATE_RESUME_DONE failed");
                            pThis->omx_report_error ();
                        } else {
                            if (BITMASK_PRESENT(&pThis->m_flags,OMX_COMPONENT_EXECUTE_PENDING)) {
                                DEBUG_PRINT_LOW("Moving the decoder to execute state");
                                // Send the callback now
                                BITMASK_CLEAR((&pThis->m_flags),OMX_COMPONENT_EXECUTE_PENDING);
                                pThis->m_state = OMX_StateExecuting;
                                pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                        OMX_EventCmdComplete,OMX_CommandStateSet,
                                        OMX_StateExecuting,NULL);
                            }
                        }
                    } else {
                        DEBUG_PRINT_ERROR("ERROR: %s()::EventHandler is NULL", __func__);
                    }

                    break;

                case OMX_COMPONENT_GENERATE_STOP_DONE:
                    DEBUG_PRINT_HIGH("Rxd OMX_COMPONENT_GENERATE_STOP_DONE");

                    if (pThis->m_cb.EventHandler) {
                        if (p2 != VDEC_S_SUCCESS) {
                            DEBUG_PRINT_ERROR("OMX_COMPONENT_GENERATE_STOP_DONE ret failed");
                            pThis->omx_report_error ();
                        } else {
                            pThis->complete_pending_buffer_done_cbs();

                            if (BITMASK_PRESENT(&pThis->m_flags,OMX_COMPONENT_IDLE_PENDING)) {
                                DEBUG_PRINT_LOW("OMX_COMPONENT_GENERATE_STOP_DONE Success");
                                // Send the callback now
                                BITMASK_CLEAR((&pThis->m_flags),OMX_COMPONENT_IDLE_PENDING);
                                pThis->m_state = OMX_StateIdle;
                                DEBUG_PRINT_LOW("Move to Idle State");
                                pThis->m_cb.EventHandler(&pThis->m_cmp,pThis->m_app_data,
                                        OMX_EventCmdComplete,OMX_CommandStateSet,
                                        OMX_StateIdle,NULL);
                            }
                        }
                    } else {
                        DEBUG_PRINT_ERROR("ERROR: %s()::EventHandler is NULL", __func__);
                    }

                    break;

                case OMX_COMPONENT_GENERATE_PORT_RECONFIG:
                    DEBUG_PRINT_HIGH("Rxd OMX_COMPONENT_GENERATE_PORT_RECONFIG");

                    if (pThis->start_port_reconfig() != OMX_ErrorNone)
                        pThis->omx_report_error();
                    else {
                        if (pThis->in_reconfig) {
                            if (pThis->m_cb.EventHandler) {
                                pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                        OMX_EventPortSettingsChanged, OMX_CORE_OUTPUT_PORT_INDEX, 0, NULL );
                            } else {
                                DEBUG_PRINT_ERROR("ERROR: %s()::EventHandler is NULL", __func__);
                            }
                        }

                        if (pThis->drv_ctx.interlace != VDEC_InterlaceFrameProgressive) {
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
                    DEBUG_PRINT_HIGH("Rxd OMX_COMPONENT_GENERATE_EOS_DONE");

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
                    DEBUG_PRINT_ERROR("OMX_COMPONENT_GENERATE_HARDWARE_ERROR");
                    pThis->omx_report_error ();
                    break;
                case OMX_COMPONENT_GENERATE_INFO_PORT_RECONFIG:
                    {
                        DEBUG_PRINT_HIGH("Rxd OMX_COMPONENT_GENERATE_INFO_PORT_RECONFIG");

                        if (pThis->m_cb.EventHandler) {
                            pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                    (OMX_EVENTTYPE)OMX_EventIndexsettingChanged, OMX_CORE_OUTPUT_PORT_INDEX, 0, NULL );
                        } else {
                            DEBUG_PRINT_ERROR("ERROR: %s()::EventHandler is NULL", __func__);
                        }

                        //update power HAL with new width, height and bitrate
                        pThis->power_module_deregister();
                        pThis->power_module_register();
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
    } while (qsize>0);

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
    unsigned int   alignment = 0,buffer_size = 0;
    int is_secure = 0;
    int i = 0;
    int fds[2];
    int r;
    OMX_STRING device_name = "/dev/msm_vidc_dec";

#ifndef JB_MR1
    sp<IServiceManager> sm;
    sp<hwcService::IHWComposer> hwcBinder = NULL;
#endif

    if (!strncmp(role, "OMX.qcom.video.decoder.avc.secure",OMX_MAX_STRINGNAME_SIZE)) {
        secure_mode = true;
        arbitrary_bytes = false;
        role = "OMX.qcom.video.decoder.avc";
        device_name =  "/dev/msm_vidc_dec_sec";
        is_secure = 1;
    } else if (!strncmp(role, "OMX.qcom.video.decoder.mpeg2.secure",OMX_MAX_STRINGNAME_SIZE)) {
        secure_mode = true;
        arbitrary_bytes = false;
        role = "OMX.qcom.video.decoder.mpeg2";
        device_name =  "/dev/msm_vidc_dec_sec";
        is_secure = 1;
    }

#ifndef JB_MR1

    if (secure_mode) {
        sm = defaultServiceManager();
        hwcBinder =
            interface_cast<hwcService::IHWComposer>(sm->getService(String16("display.hwcservice")));

        if (hwcBinder != NULL) {
            hwcBinder->setOpenSecureStart();
        } else {
            DEBUG_PRINT_HIGH("Failed to get ref to hwcBinder, "
                    "cannot call secure display start");
        }
    }

#endif
    DEBUG_PRINT_HIGH("omx_vdec::component_init(): Start of New Playback : role  = %s : DEVICE = %s",
            role, device_name);

    drv_ctx.video_driver_fd = open(device_name, O_RDWR | O_NONBLOCK);

    DEBUG_PRINT_HIGH("omx_vdec::component_init(): Open returned fd %d, errno %d",
            drv_ctx.video_driver_fd, errno);

    if (drv_ctx.video_driver_fd == 0) {
        drv_ctx.video_driver_fd = open(device_name, O_RDWR | O_NONBLOCK);
    }

    if (is_secure && drv_ctx.video_driver_fd < 0) {
        do {
            usleep(100 * 1000);
            drv_ctx.video_driver_fd = open(device_name, O_RDWR | O_NONBLOCK);

            if (drv_ctx.video_driver_fd > 0) {
                break;
            }
        } while (i++ < 50);
    }

    if (drv_ctx.video_driver_fd < 0) {
        DEBUG_PRINT_ERROR("Omx_vdec::Comp Init Returning failure, errno %d", errno);
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

    if (!strncmp(drv_ctx.kind,"OMX.qcom.video.decoder.mpeg4",\
                OMX_MAX_STRINGNAME_SIZE)) {
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
    } else if (!strncmp(drv_ctx.kind,"OMX.qcom.video.decoder.mpeg2",\
                OMX_MAX_STRINGNAME_SIZE)) {
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
    } else if (!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.h263",\
                OMX_MAX_STRINGNAME_SIZE)) {
        strlcpy((char *)m_cRole, "video_decoder.h263",OMX_MAX_STRINGNAME_SIZE);
        DEBUG_PRINT_LOW("H263 Decoder selected");
        drv_ctx.decoder_format = VDEC_CODECTYPE_H263;
        eCompressionFormat = OMX_VIDEO_CodingH263;
        codec_type_parse = CODEC_TYPE_H263;
        m_frame_parser.init_start_codes (codec_type_parse);
#ifdef INPUT_BUFFER_LOG
        strcat(inputfilename, "263");
#endif
    }

#ifdef MAX_RES_1080P
    else if (!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.divx311",\
                OMX_MAX_STRINGNAME_SIZE)) {
        strlcpy((char *)m_cRole, "video_decoder.divx",OMX_MAX_STRINGNAME_SIZE);
        DEBUG_PRINT_LOW ("DIVX 311 Decoder selected");
        drv_ctx.decoder_format = VDEC_CODECTYPE_DIVX_3;
        eCompressionFormat = (OMX_VIDEO_CODINGTYPE)QOMX_VIDEO_CodingDivx;
        codec_type_parse = CODEC_TYPE_DIVX;
        m_frame_parser.init_start_codes (codec_type_parse);
#ifdef _ANDROID_
        OMX_ERRORTYPE err = createDivxDrmContext();

        if ( err != OMX_ErrorNone ) {
            DEBUG_PRINT_ERROR("createDivxDrmContext Failed");
            eRet = err;
            goto cleanup;
        }

#endif //_ANDROID_
    } else if (!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.divx4",\
                OMX_MAX_STRINGNAME_SIZE)) {
        strlcpy((char *)m_cRole, "video_decoder.divx",OMX_MAX_STRINGNAME_SIZE);
        DEBUG_PRINT_ERROR ("DIVX 4 Decoder selected");
        drv_ctx.decoder_format = VDEC_CODECTYPE_DIVX_4;
        eCompressionFormat = (OMX_VIDEO_CODINGTYPE)QOMX_VIDEO_CodingDivx;
        codec_type_parse = CODEC_TYPE_DIVX;
        m_frame_parser.init_start_codes (codec_type_parse);
#ifdef _ANDROID_
        OMX_ERRORTYPE err = createDivxDrmContext();

        if ( err != OMX_ErrorNone ) {
            DEBUG_PRINT_ERROR("createDivxDrmContext Failed");
            eRet = err;
            goto cleanup;
        }

#endif //_ANDROID_
    } else if (!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.divx",\
                OMX_MAX_STRINGNAME_SIZE)) {
        strlcpy((char *)m_cRole, "video_decoder.divx",OMX_MAX_STRINGNAME_SIZE);
        DEBUG_PRINT_ERROR ("DIVX 5/6 Decoder selected");
        drv_ctx.decoder_format = VDEC_CODECTYPE_DIVX_6;
        eCompressionFormat = (OMX_VIDEO_CODINGTYPE)QOMX_VIDEO_CodingDivx;
        codec_type_parse = CODEC_TYPE_DIVX;
        m_frame_parser.init_start_codes (codec_type_parse);
#ifdef _ANDROID_
        OMX_ERRORTYPE err = createDivxDrmContext();

        if ( err != OMX_ErrorNone ) {
            DEBUG_PRINT_ERROR("createDivxDrmContext Failed");
            eRet = err;
            goto cleanup;
        }

#endif //_ANDROID_
    }

#else
    else if ((!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.divx4",\
                    OMX_MAX_STRINGNAME_SIZE)) || (!strncmp(drv_ctx.kind, \
                        "OMX.qcom.video.decoder.divx", OMX_MAX_STRINGNAME_SIZE))) {
        strlcpy((char *)m_cRole, "video_decoder.divx",OMX_MAX_STRINGNAME_SIZE);
        DEBUG_PRINT_ERROR ("DIVX Decoder selected");
        drv_ctx.decoder_format = VDEC_CODECTYPE_DIVX_5;
        eCompressionFormat = (OMX_VIDEO_CODINGTYPE)QOMX_VIDEO_CodingDivx;
        codec_type_parse = CODEC_TYPE_DIVX;
        m_frame_parser.init_start_codes (codec_type_parse);

#ifdef _ANDROID_
        OMX_ERRORTYPE err = createDivxDrmContext();

        if ( err != OMX_ErrorNone ) {
            DEBUG_PRINT_ERROR("createDivxDrmContext Failed");
            eRet = err;
            goto cleanup;
        }

#endif //_ANDROID_
    }

#endif
    else if (!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.avc",\
                OMX_MAX_STRINGNAME_SIZE)) {
        strlcpy((char *)m_cRole, "video_decoder.avc",OMX_MAX_STRINGNAME_SIZE);
        drv_ctx.decoder_format = VDEC_CODECTYPE_H264;
        eCompressionFormat = OMX_VIDEO_CodingAVC;
        codec_type_parse = CODEC_TYPE_H264;
        m_frame_parser.init_start_codes (codec_type_parse);
        m_frame_parser.init_nal_length(nal_length);
#ifdef INPUT_BUFFER_LOG
        strcat(inputfilename, "264");
#endif
    } else if (!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.vc1",\
                OMX_MAX_STRINGNAME_SIZE)) {
        strlcpy((char *)m_cRole, "video_decoder.vc1",OMX_MAX_STRINGNAME_SIZE);
        drv_ctx.decoder_format = VDEC_CODECTYPE_VC1;
        eCompressionFormat = OMX_VIDEO_CodingWMV;
        codec_type_parse = CODEC_TYPE_VC1;
        m_frame_parser.init_start_codes (codec_type_parse);
#ifdef INPUT_BUFFER_LOG
        strcat(inputfilename, "vc1");
#endif
    } else if (!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.wmv",\
                OMX_MAX_STRINGNAME_SIZE)) {
        strlcpy((char *)m_cRole, "video_decoder.vc1",OMX_MAX_STRINGNAME_SIZE);
        drv_ctx.decoder_format = VDEC_CODECTYPE_VC1_RCV;
        eCompressionFormat = OMX_VIDEO_CodingWMV;
        codec_type_parse = CODEC_TYPE_VC1;
        m_frame_parser.init_start_codes (codec_type_parse);
#ifdef INPUT_BUFFER_LOG
        strcat(inputfilename, "vc1");
#endif
    } else {
        DEBUG_PRINT_ERROR("ERROR:Unknown Component");
        eRet = OMX_ErrorInvalidComponentName;
    }

#ifdef INPUT_BUFFER_LOG
    inputBufferFile1 = fopen (inputfilename, "ab");
#endif

    if (eRet == OMX_ErrorNone) {
#ifdef MAX_RES_720P
        drv_ctx.output_format = VDEC_YUV_FORMAT_NV12;

#endif
#ifdef MAX_RES_1080P
        drv_ctx.output_format = VDEC_YUV_FORMAT_TILE_4x2;
        OMX_COLOR_FORMATTYPE dest_color_format = (OMX_COLOR_FORMATTYPE)
            QOMX_COLOR_FormatYUV420PackedSemiPlanar64x32Tile2m8ka;

        if (!client_buffers.set_color_format(dest_color_format)) {
            DEBUG_PRINT_ERROR("Setting color format failed");
            eRet = OMX_ErrorInsufficientResources;
        }

#endif
        /*Initialize Decoder with codec type and resolution*/
        ioctl_msg.in = &drv_ctx.decoder_format;
        ioctl_msg.out = NULL;

        if ( (eRet == OMX_ErrorNone) &&
                ioctl (drv_ctx.video_driver_fd,VDEC_IOCTL_SET_CODEC,
                    (void*)&ioctl_msg) < 0)

        {
            DEBUG_PRINT_ERROR("Set codec type failed");
            eRet = OMX_ErrorInsufficientResources;
        }

        /*Set the output format*/
        ioctl_msg.in = &drv_ctx.output_format;
        ioctl_msg.out = NULL;

        if ( (eRet == OMX_ErrorNone) &&
                ioctl (drv_ctx.video_driver_fd,VDEC_IOCTL_SET_OUTPUT_FORMAT,
                    (void*)&ioctl_msg) < 0) {
            DEBUG_PRINT_ERROR("Set output format failed");
            eRet = OMX_ErrorInsufficientResources;
        }

#ifdef MAX_RES_720P
        drv_ctx.video_resolution.frame_height =
            drv_ctx.video_resolution.scan_lines = 720;
        drv_ctx.video_resolution.frame_width =
            drv_ctx.video_resolution.stride = 1280;
#endif
#ifdef MAX_RES_1080P
        drv_ctx.video_resolution.frame_height =
            drv_ctx.video_resolution.scan_lines = 1088;
        drv_ctx.video_resolution.frame_width =
            drv_ctx.video_resolution.stride = 1920;
#endif

        ioctl_msg.in = &drv_ctx.video_resolution;
        ioctl_msg.out = NULL;

        if ( (eRet == OMX_ErrorNone) &&
                ioctl (drv_ctx.video_driver_fd,VDEC_IOCTL_SET_PICRES,
                    (void*)&ioctl_msg) < 0) {
            DEBUG_PRINT_ERROR("Set Resolution failed");
            eRet = OMX_ErrorInsufficientResources;
        }

        /*Get the Buffer requirements for input and output ports*/
        drv_ctx.ip_buf.buffer_type = VDEC_BUFFER_TYPE_INPUT;
        drv_ctx.op_buf.buffer_type = VDEC_BUFFER_TYPE_OUTPUT;
        drv_ctx.interlace = VDEC_InterlaceFrameProgressive;
        drv_ctx.extradata = 0;
        drv_ctx.picture_order = VDEC_ORDER_DISPLAY;
        drv_ctx.idr_only_decoding = 0;

        if (eRet == OMX_ErrorNone)
            eRet = get_buffer_req(&drv_ctx.ip_buf);

        if (eRet == OMX_ErrorNone)
            eRet = get_buffer_req(&drv_ctx.op_buf);

        m_state = OMX_StateLoaded;
#ifdef DEFAULT_EXTRADATA

        if (eRet == OMX_ErrorNone)
            eRet = enable_extradata(DEFAULT_EXTRADATA);

#endif

        if ( (codec_type_parse == CODEC_TYPE_VC1) ||
                (codec_type_parse == CODEC_TYPE_H264)) { //add CP check here
            //Check if dmx can be disabled
            struct vdec_ioctl_msg ioctl_msg = {NULL, NULL};
            OMX_ERRORTYPE eRet = OMX_ErrorNone;
            ioctl_msg.out = &drv_ctx.disable_dmx;

            if (ioctl(drv_ctx.video_driver_fd, VDEC_IOCTL_GET_DISABLE_DMX_SUPPORT, &ioctl_msg)) {
                DEBUG_PRINT_ERROR("Error VDEC_IOCTL_GET_DISABLE_DMX_SUPPORT");
                eRet = OMX_ErrorHardware;
            } else {
                if (drv_ctx.disable_dmx && !secure_mode) {
                    DEBUG_PRINT_HIGH("DMX disable is supported");

                    int rc = ioctl(drv_ctx.video_driver_fd,
                            VDEC_IOCTL_SET_DISABLE_DMX);

                    if (rc < 0) {
                        DEBUG_PRINT_ERROR("Failed to disable dmx on driver.");
                        drv_ctx.disable_dmx = false;
                        eRet = OMX_ErrorHardware;
                    }
                } else {
                    drv_ctx.disable_dmx = false;
                }
            }
        }

        if (drv_ctx.decoder_format == VDEC_CODECTYPE_H264) {
            if (m_frame_parser.mutils == NULL) {
                m_frame_parser.mutils = new H264_Utils();

                if (m_frame_parser.mutils == NULL) {
                    DEBUG_PRINT_ERROR("parser utils Allocation failed ");
                    eRet = OMX_ErrorInsufficientResources;
                } else {
                    h264_scratch.nAllocLen = drv_ctx.ip_buf.buffer_size - DEVICE_SCRATCH;
                    h264_scratch.pBuffer = (OMX_U8 *)malloc (h264_scratch.nAllocLen);
                    h264_scratch.nFilledLen = 0;
                    h264_scratch.nOffset = 0;

                    if (h264_scratch.pBuffer == NULL) {
                        DEBUG_PRINT_ERROR("h264_scratch.pBuffer Allocation failed ");
                        return OMX_ErrorInsufficientResources;
                    }

                    m_frame_parser.mutils->initialize_frame_checking_environment();
                    m_frame_parser.mutils->allocate_rbsp_buffer (drv_ctx.ip_buf.buffer_size);
                }
            }

            h264_parser = new h264_stream_parser();

            if (!h264_parser) {
                DEBUG_PRINT_ERROR("ERROR: H264 parser allocation failed!");
                eRet = OMX_ErrorInsufficientResources;
            }
        }

        if (pipe(fds)) {
            DEBUG_PRINT_ERROR("pipe creation failed.");
            eRet = OMX_ErrorInsufficientResources;
        } else {
            int temp1[2];

            if (fds[0] == 0 || fds[1] == 0) {
                if (pipe (temp1)) {
                    DEBUG_PRINT_ERROR("pipe creation failed..");
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

            if (r < 0) {
                DEBUG_PRINT_ERROR("component_init(): message_thread creation failed");
                eRet = OMX_ErrorInsufficientResources;
            } else {
                msg_thread_created = true;
                r = pthread_create(&async_thread_id,0,async_message_thread,this);

                if (r < 0) {
                    DEBUG_PRINT_ERROR("component_init(): async_message_thread creation failed");
                    eRet = OMX_ErrorInsufficientResources;
                } else {
                    async_thread_created = true;
                }
            }
        }
    }

    if (eRet != OMX_ErrorNone) {
        DEBUG_PRINT_ERROR("Component Init Failed");
        DEBUG_PRINT_HIGH("Calling VDEC_IOCTL_STOP_NEXT_MSG");
        (void)ioctl(drv_ctx.video_driver_fd, VDEC_IOCTL_STOP_NEXT_MSG,
                NULL);
    } else {
        DEBUG_PRINT_HIGH("omx_vdec::component_init() success");
    }

    memset(&h264_mv_buff,0,sizeof(struct h264_mv_buffer));
    memset(&meta_buff,0,sizeof(struct meta_buffer));
cleanup:

    if (!secure_mode) {
        return eRet;
    }

#ifndef JB_MR1

    if (hwcBinder != NULL) {
        (eRet == OMX_ErrorNone) ?
            hwcBinder->setOpenSecureEnd() :
            hwcBinder->setCloseSecureEnd();
    } else {
        DEBUG_PRINT_HIGH("hwcBinder not found, "
                "not calling secure end");
    }

#endif
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
    if (m_state == OMX_StateInvalid) {
        DEBUG_PRINT_ERROR("Get Comp Version in Invalid State");
        return OMX_ErrorInvalidState;
    }

    /* TBD -- Return the proper version */
    if (specVersion) {
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
    DEBUG_PRINT_HIGH("send_command: Recieved a Command from Client");

    if (m_state == OMX_StateInvalid) {
        DEBUG_PRINT_ERROR("ERROR: Send Command in Invalid State");
        return OMX_ErrorInvalidState;
    }

    if (cmd == OMX_CommandFlush && param1 != OMX_CORE_INPUT_PORT_INDEX
            && param1 != OMX_CORE_OUTPUT_PORT_INDEX && param1 != OMX_ALL) {
        DEBUG_PRINT_ERROR("send_command(): ERROR OMX_CommandFlush "
                "to invalid port: %d", param1);
        return OMX_ErrorBadPortIndex;
    }

    post_event((unsigned)cmd,(unsigned)param1,OMX_COMPONENT_GENERATE_COMMAND);
    sem_wait(&m_cmd_lock);
    DEBUG_PRINT_HIGH("send_command: Command Processed");
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
    int bFlag = 1,sem_posted = 0;

    DEBUG_PRINT_HIGH("send_command_proxy(): cmd = %d, Current State %d, Expected State %d",
            cmd, m_state, eState);

    if (cmd == OMX_CommandStateSet) {
        DEBUG_PRINT_HIGH("send_command_proxy(): OMX_CommandStateSet issued");
        DEBUG_PRINT_HIGH("Current State %d, Expected State %d", m_state, eState);

        /***************************/

        /* Current State is Loaded */

        /***************************/
        if (m_state == OMX_StateLoaded) {
            if (eState == OMX_StateIdle) {
                //if all buffers are allocated or all ports disabled
                if (allocate_done() ||
                        (m_inp_bEnabled == OMX_FALSE && m_out_bEnabled == OMX_FALSE)) {
                    DEBUG_PRINT_LOW("send_command_proxy(): Loaded-->Idle");
                } else {
                    DEBUG_PRINT_LOW("send_command_proxy(): Loaded-->Idle-Pending");
                    BITMASK_SET(&m_flags, OMX_COMPONENT_IDLE_PENDING);
                    // Skip the event notification
                    bFlag = 0;
                }
            }
            /* Requesting transition from Loaded to Loaded */
            else if (eState == OMX_StateLoaded) {
                DEBUG_PRINT_ERROR("ERROR::send_command_proxy(): Loaded-->Loaded");
                post_event(OMX_EventError,OMX_ErrorSameState,\
                        OMX_COMPONENT_GENERATE_EVENT);
                eRet = OMX_ErrorSameState;
            }
            /* Requesting transition from Loaded to WaitForResources */
            else if (eState == OMX_StateWaitForResources) {
                /* Since error is None , we will post an event
                   at the end of this function definition */
                DEBUG_PRINT_LOW("send_command_proxy(): Loaded-->WaitForResources");
            }
            /* Requesting transition from Loaded to Executing */
            else if (eState == OMX_StateExecuting) {
                DEBUG_PRINT_ERROR("ERROR::send_command_proxy(): Loaded-->Executing");
                post_event(OMX_EventError,OMX_ErrorIncorrectStateTransition,\
                        OMX_COMPONENT_GENERATE_EVENT);
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            /* Requesting transition from Loaded to Pause */
            else if (eState == OMX_StatePause) {
                DEBUG_PRINT_ERROR("ERROR::send_command_proxy(): Loaded-->Pause");
                post_event(OMX_EventError,OMX_ErrorIncorrectStateTransition,\
                        OMX_COMPONENT_GENERATE_EVENT);
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            /* Requesting transition from Loaded to Invalid */
            else if (eState == OMX_StateInvalid) {
                DEBUG_PRINT_ERROR("ERROR::send_command_proxy(): Loaded-->Invalid");
                post_event(OMX_EventError,eState,OMX_COMPONENT_GENERATE_EVENT);
                eRet = OMX_ErrorInvalidState;
            } else {
                DEBUG_PRINT_ERROR("ERROR::send_command_proxy(): Loaded-->Invalid(%d Not Handled)",\
                        eState);
                eRet = OMX_ErrorBadParameter;
            }
        }

        /***************************/
        /* Current State is IDLE */
        /***************************/
        else if (m_state == OMX_StateIdle) {
            if (eState == OMX_StateLoaded) {
                if (release_done()) {
                    /*
                       Since error is None , we will post an event at the end
                       of this function definition
                     */
                    DEBUG_PRINT_HIGH("send_command_proxy(): Idle-->Loaded");
                } else {
                    DEBUG_PRINT_HIGH("send_command_proxy(): Idle-->Loaded-Pending");
                    BITMASK_SET(&m_flags, OMX_COMPONENT_LOADING_PENDING);
                    // Skip the event notification
                    bFlag = 0;
                }
            }
            /* Requesting transition from Idle to Executing */
            else if (eState == OMX_StateExecuting) {
                DEBUG_PRINT_HIGH("send_command_proxy(): Idle-->Executing");
                BITMASK_SET(&m_flags, OMX_COMPONENT_EXECUTE_PENDING);
                bFlag = 0;

                if (ioctl (drv_ctx.video_driver_fd,VDEC_IOCTL_CMD_START,
                            NULL) < 0) {
                    DEBUG_PRINT_ERROR("VDEC_IOCTL_CMD_START FAILED");
                    omx_report_error ();
                    eRet = OMX_ErrorHardware;
                } else {
                    power_module_register();
                }
            }
            /* Requesting transition from Idle to Idle */
            else if (eState == OMX_StateIdle) {
                DEBUG_PRINT_ERROR("ERROR::send_command_proxy(): Idle-->Idle");
                post_event(OMX_EventError,OMX_ErrorSameState,\
                        OMX_COMPONENT_GENERATE_EVENT);
                eRet = OMX_ErrorSameState;
            }
            /* Requesting transition from Idle to WaitForResources */
            else if (eState == OMX_StateWaitForResources) {
                DEBUG_PRINT_ERROR("ERROR::send_command_proxy(): Idle-->WaitForResources");
                post_event(OMX_EventError,OMX_ErrorIncorrectStateTransition,\
                        OMX_COMPONENT_GENERATE_EVENT);
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            /* Requesting transition from Idle to Pause */
            else if (eState == OMX_StatePause) {
                /*To pause the Video core we need to start the driver*/
                if (ioctl (drv_ctx.video_driver_fd,VDEC_IOCTL_CMD_START,
                            NULL) < 0) {
                    DEBUG_PRINT_ERROR("VDEC_IOCTL_CMD_START FAILED");
                    omx_report_error ();
                    eRet = OMX_ErrorHardware;
                } else {
                    BITMASK_SET(&m_flags,OMX_COMPONENT_PAUSE_PENDING);
                    DEBUG_PRINT_HIGH("send_command_proxy(): Idle-->Pause");
                    bFlag = 0;
                }
            }
            /* Requesting transition from Idle to Invalid */
            else if (eState == OMX_StateInvalid) {
                DEBUG_PRINT_ERROR("ERROR::send_command_proxy(): Idle-->Invalid");
                post_event(OMX_EventError,eState,OMX_COMPONENT_GENERATE_EVENT);
                eRet = OMX_ErrorInvalidState;
            } else {
                DEBUG_PRINT_ERROR("ERROR::send_command_proxy(): Idle --> %d Not Handled",eState);
                eRet = OMX_ErrorBadParameter;
            }
        }

        /******************************/
        /* Current State is Executing */
        /******************************/
        else if (m_state == OMX_StateExecuting) {
            DEBUG_PRINT_HIGH("Command Recieved in OMX_StateExecuting");

            /* Requesting transition from Executing to Idle */
            if (eState == OMX_StateIdle) {
                /* Since error is None , we will post an event
                   at the end of this function definition
                 */
                DEBUG_PRINT_HIGH("send_command_proxy(): Executing --> Idle");
                BITMASK_SET(&m_flags,OMX_COMPONENT_IDLE_PENDING);

                if (!sem_posted) {
                    sem_posted = 1;
                    sem_post (&m_cmd_lock);
                    execute_omx_flush(OMX_ALL);
                }

                bFlag = 0;
            }
            /* Requesting transition from Executing to Paused */
            else if (eState == OMX_StatePause) {
                DEBUG_PRINT_HIGH("PAUSE Command Issued");

                if (ioctl (drv_ctx.video_driver_fd,VDEC_IOCTL_CMD_PAUSE,
                            NULL) < 0) {
                    DEBUG_PRINT_ERROR("Error In Pause State");
                    post_event(OMX_EventError,OMX_ErrorHardware,\
                            OMX_COMPONENT_GENERATE_EVENT);
                    eRet = OMX_ErrorHardware;
                } else {
                    BITMASK_SET(&m_flags,OMX_COMPONENT_PAUSE_PENDING);
                    DEBUG_PRINT_HIGH("send_command_proxy(): Executing-->Pause");
                    bFlag = 0;
                }
            }
            /* Requesting transition from Executing to Loaded */
            else if (eState == OMX_StateLoaded) {
                DEBUG_PRINT_ERROR("send_command_proxy(): Executing --> Loaded");
                post_event(OMX_EventError,OMX_ErrorIncorrectStateTransition,\
                        OMX_COMPONENT_GENERATE_EVENT);
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            /* Requesting transition from Executing to WaitForResources */
            else if (eState == OMX_StateWaitForResources) {
                DEBUG_PRINT_ERROR("send_command_proxy(): Executing --> WaitForResources");
                post_event(OMX_EventError,OMX_ErrorIncorrectStateTransition,\
                        OMX_COMPONENT_GENERATE_EVENT);
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            /* Requesting transition from Executing to Executing */
            else if (eState == OMX_StateExecuting) {
                DEBUG_PRINT_ERROR("send_command_proxy(): Executing --> Executing");
                post_event(OMX_EventError,OMX_ErrorSameState,\
                        OMX_COMPONENT_GENERATE_EVENT);
                eRet = OMX_ErrorSameState;
            }
            /* Requesting transition from Executing to Invalid */
            else if (eState == OMX_StateInvalid) {
                DEBUG_PRINT_ERROR("send_command_proxy(): Executing --> Invalid");
                post_event(OMX_EventError,eState,OMX_COMPONENT_GENERATE_EVENT);
                eRet = OMX_ErrorInvalidState;
            } else {
                DEBUG_PRINT_ERROR("ERROR::send_command_proxy(): Executing --> %d Not Handled",eState);
                eRet = OMX_ErrorBadParameter;
            }
        }
        /***************************/
        /* Current State is Pause  */
        /***************************/
        else if (m_state == OMX_StatePause) {
            /* Requesting transition from Pause to Executing */
            if (eState == OMX_StateExecuting) {
                DEBUG_PRINT_HIGH("Pause --> Executing");

                if (ioctl (drv_ctx.video_driver_fd,VDEC_IOCTL_CMD_RESUME,
                            NULL) < 0) {
                    DEBUG_PRINT_ERROR("VDEC_IOCTL_CMD_RESUME failed");
                    post_event(OMX_EventError,OMX_ErrorHardware,\
                            OMX_COMPONENT_GENERATE_EVENT);
                    eRet = OMX_ErrorHardware;
                } else {
                    BITMASK_SET(&m_flags,OMX_COMPONENT_EXECUTE_PENDING);
                    DEBUG_PRINT_HIGH("send_command_proxy(): Idle-->Executing");
                    post_event (NULL,VDEC_S_SUCCESS,\
                            OMX_COMPONENT_GENERATE_RESUME_DONE);
                    bFlag = 0;
                }
            }
            /* Requesting transition from Pause to Idle */
            else if (eState == OMX_StateIdle) {
                /* Since error is None , we will post an event
                   at the end of this function definition */
                DEBUG_PRINT_HIGH("Pause --> Idle..");
                BITMASK_SET(&m_flags,OMX_COMPONENT_IDLE_PENDING);

                if (!sem_posted) {
                    sem_posted = 1;
                    sem_post (&m_cmd_lock);
                    execute_omx_flush(OMX_ALL);
                }

                bFlag = 0;
            }
            /* Requesting transition from Pause to loaded */
            else if (eState == OMX_StateLoaded) {
                DEBUG_PRINT_ERROR("Pause --> loaded");
                post_event(OMX_EventError,OMX_ErrorIncorrectStateTransition,\
                        OMX_COMPONENT_GENERATE_EVENT);
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            /* Requesting transition from Pause to WaitForResources */
            else if (eState == OMX_StateWaitForResources) {
                DEBUG_PRINT_ERROR("Pause --> WaitForResources");
                post_event(OMX_EventError,OMX_ErrorIncorrectStateTransition,\
                        OMX_COMPONENT_GENERATE_EVENT);
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            /* Requesting transition from Pause to Pause */
            else if (eState == OMX_StatePause) {
                DEBUG_PRINT_ERROR("Pause --> Pause");
                post_event(OMX_EventError,OMX_ErrorSameState,\
                        OMX_COMPONENT_GENERATE_EVENT);
                eRet = OMX_ErrorSameState;
            }
            /* Requesting transition from Pause to Invalid */
            else if (eState == OMX_StateInvalid) {
                DEBUG_PRINT_ERROR("Pause --> Invalid");
                post_event(OMX_EventError,eState,OMX_COMPONENT_GENERATE_EVENT);
                eRet = OMX_ErrorInvalidState;
            } else {
                DEBUG_PRINT_ERROR("ERROR::send_command_proxy(): Paused --> %d Not Handled",eState);
                eRet = OMX_ErrorBadParameter;
            }
        }
        /***************************/
        /* Current State is WaitForResources  */
        /***************************/
        else if (m_state == OMX_StateWaitForResources) {
            /* Requesting transition from WaitForResources to Loaded */
            if (eState == OMX_StateLoaded) {
                /* Since error is None , we will post an event
                   at the end of this function definition */
                DEBUG_PRINT_HIGH("send_command_proxy(): WaitForResources-->Loaded");
            }
            /* Requesting transition from WaitForResources to WaitForResources */
            else if (eState == OMX_StateWaitForResources) {
                DEBUG_PRINT_ERROR("ERROR::send_command_proxy(): WaitForResources-->WaitForResources");
                post_event(OMX_EventError,OMX_ErrorSameState,
                        OMX_COMPONENT_GENERATE_EVENT);
                eRet = OMX_ErrorSameState;
            }
            /* Requesting transition from WaitForResources to Executing */
            else if (eState == OMX_StateExecuting) {
                DEBUG_PRINT_ERROR("ERROR::send_command_proxy(): WaitForResources-->Executing");
                post_event(OMX_EventError,OMX_ErrorIncorrectStateTransition,\
                        OMX_COMPONENT_GENERATE_EVENT);
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            /* Requesting transition from WaitForResources to Pause */
            else if (eState == OMX_StatePause) {
                DEBUG_PRINT_ERROR("ERROR::send_command_proxy(): WaitForResources-->Pause");
                post_event(OMX_EventError,OMX_ErrorIncorrectStateTransition,\
                        OMX_COMPONENT_GENERATE_EVENT);
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            /* Requesting transition from WaitForResources to Invalid */
            else if (eState == OMX_StateInvalid) {
                DEBUG_PRINT_ERROR("ERROR::send_command_proxy(): WaitForResources-->Invalid");
                post_event(OMX_EventError,eState,OMX_COMPONENT_GENERATE_EVENT);
                eRet = OMX_ErrorInvalidState;
            }

            /* Requesting transition from WaitForResources to Loaded -
               is NOT tested by Khronos TS */

        } else {
            DEBUG_PRINT_ERROR("ERROR::send_command_proxy(): %d --> %d(Not Handled)",m_state,eState);
            eRet = OMX_ErrorBadParameter;
        }
    }
    /********************************/
    /* Current State is Invalid */
    /*******************************/
    else if (m_state == OMX_StateInvalid) {
        /* State Transition from Inavlid to any state */
        if (eState == (OMX_StateLoaded || OMX_StateWaitForResources
                    || OMX_StateIdle || OMX_StateExecuting
                    || OMX_StatePause || OMX_StateInvalid)) {
            DEBUG_PRINT_ERROR("ERROR::send_command_proxy(): Invalid -->Loaded");
            post_event(OMX_EventError,OMX_ErrorInvalidState,\
                    OMX_COMPONENT_GENERATE_EVENT);
            eRet = OMX_ErrorInvalidState;
        }
    } else if (cmd == OMX_CommandFlush) {
        DEBUG_PRINT_HIGH("send_command_proxy(): OMX_CommandFlush issued "
                "with param1: %d", param1);

        if (OMX_CORE_INPUT_PORT_INDEX == param1 || OMX_ALL == param1) {
            BITMASK_SET(&m_flags, OMX_COMPONENT_INPUT_FLUSH_PENDING);
        }

        if (OMX_CORE_OUTPUT_PORT_INDEX == param1 || OMX_ALL == param1) {
            BITMASK_SET(&m_flags, OMX_COMPONENT_OUTPUT_FLUSH_PENDING);
        }

        if (!sem_posted) {
            sem_posted = 1;
            DEBUG_PRINT_LOW("Set the Semaphore");
            sem_post (&m_cmd_lock);
            execute_omx_flush(param1);
        }

        bFlag = 0;
    } else if ( cmd == OMX_CommandPortEnable) {
        DEBUG_PRINT_HIGH("send_command_proxy(): OMX_CommandPortEnable issued "
                "with param1: %d", param1);

        if (param1 == OMX_CORE_INPUT_PORT_INDEX || param1 == OMX_ALL) {
            m_inp_bEnabled = OMX_TRUE;

            if ( (m_state == OMX_StateLoaded &&
                        !BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING))
                    || allocate_input_done()) {
                post_event(OMX_CommandPortEnable,OMX_CORE_INPUT_PORT_INDEX,
                        OMX_COMPONENT_GENERATE_EVENT);
            } else {
                DEBUG_PRINT_HIGH("send_command_proxy(): Disabled-->Enabled Pending");
                BITMASK_SET(&m_flags, OMX_COMPONENT_INPUT_ENABLE_PENDING);
                // Skip the event notification
                bFlag = 0;
            }
        }

        if (param1 == OMX_CORE_OUTPUT_PORT_INDEX || param1 == OMX_ALL) {
            DEBUG_PRINT_HIGH("Enable output Port command recieved");
            m_out_bEnabled = OMX_TRUE;

            if ( (m_state == OMX_StateLoaded &&
                        !BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING))
                    || (allocate_output_done())) {
                post_event(OMX_CommandPortEnable,OMX_CORE_OUTPUT_PORT_INDEX,
                        OMX_COMPONENT_GENERATE_EVENT);

            } else {
                DEBUG_PRINT_HIGH("send_command_proxy(): Disabled-->Enabled Pending");
                BITMASK_SET(&m_flags, OMX_COMPONENT_OUTPUT_ENABLE_PENDING);
                // Skip the event notification
                bFlag = 0;
            }
        }
    } else if (cmd == OMX_CommandPortDisable) {
        DEBUG_PRINT_HIGH("send_command_proxy(): OMX_CommandPortDisable issued "
                "with param1: %d", param1);

        if (param1 == OMX_CORE_INPUT_PORT_INDEX || param1 == OMX_ALL) {
            m_inp_bEnabled = OMX_FALSE;

            if ((m_state == OMX_StateLoaded || m_state == OMX_StateIdle)
                    && release_input_done()) {
                post_event(OMX_CommandPortDisable,OMX_CORE_INPUT_PORT_INDEX,
                        OMX_COMPONENT_GENERATE_EVENT);
            } else {
                BITMASK_SET(&m_flags, OMX_COMPONENT_INPUT_DISABLE_PENDING);

                if (m_state == OMX_StatePause ||m_state == OMX_StateExecuting) {
                    if (!sem_posted) {
                        sem_posted = 1;
                        sem_post (&m_cmd_lock);
                    }

                    execute_omx_flush(OMX_CORE_INPUT_PORT_INDEX);
                }

                // Skip the event notification
                bFlag = 0;
            }
        }

        if (param1 == OMX_CORE_OUTPUT_PORT_INDEX || param1 == OMX_ALL) {
            m_out_bEnabled = OMX_FALSE;
            DEBUG_PRINT_HIGH("Disable output Port command recieved");

            if ((m_state == OMX_StateLoaded || m_state == OMX_StateIdle)
                    && release_output_done()) {
                post_event(OMX_CommandPortDisable,OMX_CORE_OUTPUT_PORT_INDEX,\
                        OMX_COMPONENT_GENERATE_EVENT);
            } else {
                BITMASK_SET(&m_flags, OMX_COMPONENT_OUTPUT_DISABLE_PENDING);

                if (m_state == OMX_StatePause ||m_state == OMX_StateExecuting) {
                    if (!sem_posted) {
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
    } else {
        DEBUG_PRINT_ERROR("Error: Invalid Command other than StateSet (%d)",cmd);
        eRet = OMX_ErrorNotImplemented;
    }

    if (eRet == OMX_ErrorNone && bFlag) {
        post_event(cmd,eState,OMX_COMPONENT_GENERATE_EVENT);
    }

    if (!sem_posted) {
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

    switch (flushType) {
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

    if (ioctl(drv_ctx.video_driver_fd, VDEC_IOCTL_CMD_FLUSH, &ioctl_msg) < 0) {
        DEBUG_PRINT_ERROR("Flush Port (%d) Failed ", (int)flush_dir);
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
    DEBUG_PRINT_HIGH("Initiate Output Flush");

    while (m_ftb_q.m_size) {
        DEBUG_PRINT_HIGH("Buffer queue size %d pending buf cnt %d",
                m_ftb_q.m_size,pending_output_buffers);
        m_ftb_q.pop_entry(&p1,&p2,&ident);
        DEBUG_PRINT_HIGH("ID(%x) P1(%x) P2(%x)", ident, p1, p2);

        if (ident == m_fill_output_msg ) {
            m_cb.FillBufferDone(&m_cmp, m_app_data, (OMX_BUFFERHEADERTYPE *)p2);
        } else if (ident == OMX_COMPONENT_GENERATE_FBD) {
            fill_buffer_done(&m_cmp,(OMX_BUFFERHEADERTYPE *)p1);
        }
    }

    pthread_mutex_unlock(&m_lock);
    output_flush_progress = false;

    if (arbitrary_bytes) {
        prev_ts = LLONG_MAX;
        rst_prev_ts = true;
    }

    DEBUG_PRINT_HIGH("OMX flush o/p Port complete PenBuf(%d)", pending_output_buffers);
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
    DEBUG_PRINT_HIGH("Initiate Input Flush");

    pthread_mutex_lock(&m_lock);
    DEBUG_PRINT_LOW("Check if the Queue is empty");

    while (m_etb_q.m_size) {
        m_etb_q.pop_entry(&p1,&p2,&ident);

        if (ident == OMX_COMPONENT_GENERATE_ETB_ARBITRARY) {
            DEBUG_PRINT_HIGH("Flush Input Heap Buffer %p",(OMX_BUFFERHEADERTYPE *)p2);
            m_cb.EmptyBufferDone(&m_cmp ,m_app_data, (OMX_BUFFERHEADERTYPE *)p2);
        } else if (ident == OMX_COMPONENT_GENERATE_ETB) {
            pending_input_buffers++;
            DEBUG_PRINT_HIGH("Flush Input OMX_COMPONENT_GENERATE_ETB %p, pending_input_buffers %d",
                    (OMX_BUFFERHEADERTYPE *)p2, pending_input_buffers);
            empty_buffer_done(&m_cmp,(OMX_BUFFERHEADERTYPE *)p2);
        } else if (ident == OMX_COMPONENT_GENERATE_EBD) {
            DEBUG_PRINT_HIGH("Flush Input OMX_COMPONENT_GENERATE_EBD %p",
                    (OMX_BUFFERHEADERTYPE *)p1);
            empty_buffer_done(&m_cmp,(OMX_BUFFERHEADERTYPE *)p1);
        }
    }

    time_stamp_dts.flush_timestamp();

    /*Check if Heap Buffers are to be flushed*/
    if (arbitrary_bytes && !(codec_config_flag)) {
        DEBUG_PRINT_HIGH("Reset all the variables before flusing");
        h264_scratch.nFilledLen = 0;
        nal_count = 0;
        look_ahead_nal = false;
        frame_count = 0;
        h264_last_au_ts = LLONG_MAX;
        h264_last_au_flags = 0;
        memset(m_demux_offsets, 0, ( sizeof(OMX_U32) * 8192) );
        m_demux_entries = 0;
        DEBUG_PRINT_HIGH("Initialize parser");

        if (m_frame_parser.mutils) {
            m_frame_parser.mutils->initialize_frame_checking_environment();
        }

        while (m_input_pending_q.m_size) {
            m_input_pending_q.pop_entry(&p1,&p2,&ident);
            m_cb.EmptyBufferDone(&m_cmp ,m_app_data, (OMX_BUFFERHEADERTYPE *)p1);
        }

        if (psource_frame) {
            m_cb.EmptyBufferDone(&m_cmp ,m_app_data,psource_frame);
            psource_frame = NULL;
        }

        if (pdest_frame) {
            pdest_frame->nFilledLen = 0;
            m_input_free_q.insert_entry((unsigned) pdest_frame,NULL,NULL);
            pdest_frame = NULL;
        }

        m_frame_parser.flush();
    } else if (codec_config_flag) {
        DEBUG_PRINT_HIGH("frame_parser flushing skipped due to codec config buffer "
                "is not sent to the driver yet");
    }

    pthread_mutex_unlock(&m_lock);
    input_flush_progress = false;

    if (!arbitrary_bytes) {
        prev_ts = LLONG_MAX;
        rst_prev_ts = true;
    }

#ifdef _ANDROID_

    if (m_debug_timestamp) {
        m_timestamp_list.reset_ts_list();
    }

#endif
    DEBUG_PRINT_HIGH("OMX flush i/p Port complete PenBuf(%d)", pending_input_buffers);
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

    if (id == m_fill_output_msg ||
            id == OMX_COMPONENT_GENERATE_FBD) {
        m_ftb_q.insert_entry(p1,p2,id);
    } else if (id == OMX_COMPONENT_GENERATE_ETB ||
            id == OMX_COMPONENT_GENERATE_EBD ||
            id == OMX_COMPONENT_GENERATE_ETB_ARBITRARY) {
        m_etb_q.insert_entry(p1,p2,id);
    } else {
        m_cmd_q.insert_entry(p1,p2,id);
    }

    bRet = true;
    DEBUG_PRINT_LOW("Value of this pointer in post_event %p",this);
    post_message(this, id);

    pthread_mutex_unlock(&m_lock);

    return bRet;
}
#ifdef MAX_RES_720P
OMX_ERRORTYPE omx_vdec::get_supported_profile_level_for_720p(OMX_VIDEO_PARAM_PROFILELEVELTYPE *profileLevelType)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;

    if (!profileLevelType)
        return OMX_ErrorBadParameter;

    if (profileLevelType->nPortIndex == 0) {
        if (!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.avc",OMX_MAX_STRINGNAME_SIZE)) {
            if (profileLevelType->nProfileIndex == 0) {
                profileLevelType->eProfile = OMX_VIDEO_AVCProfileBaseline;
                profileLevelType->eLevel   = OMX_VIDEO_AVCLevel31;

            } else if (profileLevelType->nProfileIndex == 1) {
                profileLevelType->eProfile = OMX_VIDEO_AVCProfileMain;
                profileLevelType->eLevel   = OMX_VIDEO_AVCLevel31;
            } else if (profileLevelType->nProfileIndex == 2) {
                profileLevelType->eProfile = OMX_VIDEO_AVCProfileHigh;
                profileLevelType->eLevel   = OMX_VIDEO_AVCLevel31;
            } else {
                DEBUG_PRINT_HIGH("get_parameter: OMX_IndexParamVideoProfileLevelQuerySupported nProfileIndex ret NoMore %d",
                        profileLevelType->nProfileIndex);
                eRet = OMX_ErrorNoMore;
            }
        } else if ((!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.h263",OMX_MAX_STRINGNAME_SIZE))) {
            if (profileLevelType->nProfileIndex == 0) {
                profileLevelType->eProfile = OMX_VIDEO_H263ProfileBaseline;
                profileLevelType->eLevel   = OMX_VIDEO_H263Level70;
            } else {
                DEBUG_PRINT_ERROR("get_parameter: OMX_IndexParamVideoProfileLevelQuerySupported nProfileIndex ret NoMore %d", profileLevelType->nProfileIndex);
                eRet = OMX_ErrorNoMore;
            }
        } else if (!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.mpeg4",OMX_MAX_STRINGNAME_SIZE)) {
            if (profileLevelType->nProfileIndex == 0) {
                profileLevelType->eProfile = OMX_VIDEO_MPEG4ProfileSimple;
                profileLevelType->eLevel   = OMX_VIDEO_MPEG4Level5;
            } else if (profileLevelType->nProfileIndex == 1) {
                profileLevelType->eProfile = OMX_VIDEO_MPEG4ProfileAdvancedSimple;
                profileLevelType->eLevel   = OMX_VIDEO_MPEG4Level5;
            } else {
                DEBUG_PRINT_ERROR("get_parameter: OMX_IndexParamVideoProfileLevelQuerySupported nProfileIndex ret NoMore %d", profileLevelType->nProfileIndex);
                eRet = OMX_ErrorNoMore;
            }
        }
    } else {
        DEBUG_PRINT_ERROR("get_parameter: OMX_IndexParamVideoProfileLevelQuerySupported should be queries on Input port only %d", profileLevelType->nPortIndex);
        eRet = OMX_ErrorBadPortIndex;
    }

    return eRet;
}
#endif
#ifdef MAX_RES_1080P
OMX_ERRORTYPE omx_vdec::get_supported_profile_level_for_1080p(OMX_VIDEO_PARAM_PROFILELEVELTYPE *profileLevelType)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNoMore;

    if (!profileLevelType)
        return OMX_ErrorBadParameter;

    if (profileLevelType->nPortIndex == 0) {
        if (!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.avc",OMX_MAX_STRINGNAME_SIZE)) {
            if (profileLevelType->nProfileIndex == 0) {
                profileLevelType->eProfile = OMX_VIDEO_AVCProfileBaseline;
                profileLevelType->eLevel   = OMX_VIDEO_AVCLevel4;

            } else if (profileLevelType->nProfileIndex == 1) {
                profileLevelType->eProfile = OMX_VIDEO_AVCProfileMain;
                profileLevelType->eLevel   = OMX_VIDEO_AVCLevel4;
            } else if (profileLevelType->nProfileIndex == 2) {
                profileLevelType->eProfile = OMX_VIDEO_AVCProfileHigh;
                profileLevelType->eLevel   = OMX_VIDEO_AVCLevel4;
            } else {
                DEBUG_PRINT_HIGH("get_parameter: OMX_IndexParamVideoProfileLevelQuerySupported nProfileIndex ret NoMore %d",
                        profileLevelType->nProfileIndex);
                eRet = OMX_ErrorNoMore;
            }
        } else if ((!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.h263",OMX_MAX_STRINGNAME_SIZE))) {
            if (profileLevelType->nProfileIndex == 0) {
                profileLevelType->eProfile = OMX_VIDEO_H263ProfileBaseline;
                profileLevelType->eLevel   = OMX_VIDEO_H263Level70;
            } else {
                DEBUG_PRINT_ERROR("get_parameter: OMX_IndexParamVideoProfileLevelQuerySupported nProfileIndex ret NoMore %d", profileLevelType->nProfileIndex);
                eRet = OMX_ErrorNoMore;
            }
        } else if (!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.mpeg4",OMX_MAX_STRINGNAME_SIZE)) {
            if (profileLevelType->nProfileIndex == 0) {
                profileLevelType->eProfile = OMX_VIDEO_MPEG4ProfileSimple;
                profileLevelType->eLevel   = OMX_VIDEO_MPEG4Level5;
            } else if (profileLevelType->nProfileIndex == 1) {
                profileLevelType->eProfile = OMX_VIDEO_MPEG4ProfileAdvancedSimple;
                profileLevelType->eLevel   = OMX_VIDEO_MPEG4Level5;
            } else {
                DEBUG_PRINT_ERROR("get_parameter: OMX_IndexParamVideoProfileLevelQuerySupported nProfileIndex ret NoMore %d", profileLevelType->nProfileIndex);
                eRet = OMX_ErrorNoMore;
            }
        } else if (!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.mpeg2",OMX_MAX_STRINGNAME_SIZE)) {
            if (profileLevelType->nProfileIndex == 0) {
                profileLevelType->eProfile = OMX_VIDEO_MPEG2ProfileSimple;
                profileLevelType->eLevel   = OMX_VIDEO_MPEG2LevelHL;
            } else if (profileLevelType->nProfileIndex == 1) {
                profileLevelType->eProfile = OMX_VIDEO_MPEG2ProfileMain;
                profileLevelType->eLevel   = OMX_VIDEO_MPEG2LevelHL;
            } else {
                DEBUG_PRINT_ERROR("get_parameter: OMX_IndexParamVideoProfileLevelQuerySupported nProfileIndex ret NoMore %d", profileLevelType->nProfileIndex);
                eRet = OMX_ErrorNoMore;
            }
        } else {
            DEBUG_PRINT_ERROR("get_parameter: OMX_IndexParamVideoProfileLevelQuerySupported ret NoMore");
            eRet = OMX_ErrorNoMore;
        }
    } else {
        DEBUG_PRINT_ERROR("get_parameter: OMX_IndexParamVideoProfileLevelQuerySupported should be queries on Input port only %d", profileLevelType->nPortIndex);
        eRet = OMX_ErrorBadPortIndex;
    }

    return eRet;
}
#endif

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

    DEBUG_PRINT_LOW("get_parameter:");

    if (m_state == OMX_StateInvalid) {
        DEBUG_PRINT_ERROR("Get Param in Invalid State");
        return OMX_ErrorInvalidState;
    }

    if (paramData == NULL) {
        DEBUG_PRINT_ERROR("Get Param in Invalid paramData");
        return OMX_ErrorBadParameter;
    }

    switch (paramIndex) {
        case OMX_IndexParamPortDefinition:
            {
                OMX_PARAM_PORTDEFINITIONTYPE *portDefn =
                    (OMX_PARAM_PORTDEFINITIONTYPE *) paramData;
                eRet = update_portdef(portDefn);

                if (eRet == OMX_ErrorNone)
                    m_port_def = *portDefn;

                DEBUG_PRINT_HIGH("Get_parameter: OMX_IndexParamPortDefinition: "
                        "nPortIndex (%d), nFrameWidth (%d), nFrameHeight (%d), "
                        "nStride (%d), nSliceHeight (%d), nBitrate (%d), xFramerate (0x%x), "
                        "nBufferSize (%d), nBufferCountMin (%d), nBufferCountActual (%d), "
                        "bBuffersContiguous (%d), nBufferAlignment (%d), "
                        "bEnabled (%d), bPopulated (%d), eCompressionFormat (0x%x), "
                        "eColorFormat (0x%x)" , (int)portDefn->nPortIndex,
                        (int)portDefn->format.video.nFrameWidth, (int)portDefn->format.video.nFrameHeight,
                        (int)portDefn->format.video.nStride, (int)portDefn->format.video.nSliceHeight,
                        (int)portDefn->format.video.nBitrate, (int)portDefn->format.video.xFramerate,
                        (int)portDefn->nBufferSize, (int)portDefn->nBufferCountMin,
                        (int)portDefn->nBufferCountActual, (int)portDefn->bBuffersContiguous,
                        (int)portDefn->nBufferAlignment, (int)portDefn->bEnabled, (int)portDefn->bPopulated,
                        (int)portDefn->format.video.eCompressionFormat, (int)portDefn->format.video.eColorFormat);
                break;
            }
        case OMX_IndexParamVideoInit:
            {
                OMX_PORT_PARAM_TYPE *portParamType =
                    (OMX_PORT_PARAM_TYPE *) paramData;
                DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamVideoInit");

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
                portFmt->nVersion.nVersion = OMX_SPEC_VERSION;
                portFmt->nSize             = sizeof(portFmt);

                if (0 == portFmt->nPortIndex) {
                    if (0 == portFmt->nIndex) {
                        portFmt->eColorFormat =  OMX_COLOR_FormatUnused;
                        portFmt->eCompressionFormat = eCompressionFormat;
                    } else {
                        DEBUG_PRINT_ERROR("get_parameter: OMX_IndexParamVideoPortFormat:"\
                                " NoMore compression formats");
                        eRet =  OMX_ErrorNoMore;
                    }
                } else if (1 == portFmt->nPortIndex) {
                    portFmt->eCompressionFormat =  OMX_VIDEO_CodingUnused;

                    if (0 == portFmt->nIndex)
                        portFmt->eColorFormat = (OMX_COLOR_FORMATTYPE)
                            QOMX_COLOR_FormatYUV420PackedSemiPlanar64x32Tile2m8ka;
                    else if (1 == portFmt->nIndex) {
                        portFmt->eColorFormat = OMX_COLOR_FormatYUV420Planar;
                    } else {
                        DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamVideoPortFormat:"\
                                " NoMore Color formats");
                        eRet =  OMX_ErrorNoMore;
                    }
                } else {
                    DEBUG_PRINT_ERROR("get_parameter: Bad port index %d",
                            (int)portFmt->nPortIndex);
                    eRet = OMX_ErrorBadPortIndex;
                }

                DEBUG_PRINT_HIGH("Get_parameter: OMX_IndexParamVideoPortFormat: "
                        "nPortIndex (%d), nIndex (%d), eCompressionFormat (0x%x), "
                        "eColorFormat (0x%x), xFramerate (0x%x)", (int)portFmt->nPortIndex,
                        (int)portFmt->nIndex, (int)portFmt->eCompressionFormat,
                        (int)portFmt->eColorFormat, (int)portFmt->xFramerate);
                break;
            }
            /*Component should support this port definition*/
        case OMX_IndexParamAudioInit:
            {
                OMX_PORT_PARAM_TYPE *audioPortParamType =
                    (OMX_PORT_PARAM_TYPE *) paramData;
                DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamAudioInit");
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
                DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamImageInit");
                imagePortParamType->nVersion.nVersion = OMX_SPEC_VERSION;
                imagePortParamType->nSize = sizeof(imagePortParamType);
                imagePortParamType->nPorts           = 0;
                imagePortParamType->nStartPortNumber = 0;
                break;

            }
            /*Component should support this port definition*/
        case OMX_IndexParamOtherInit:
            {
                DEBUG_PRINT_ERROR("get_parameter: OMX_IndexParamOtherInit %08x",
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

                DEBUG_PRINT_LOW("Getparameter: OMX_IndexParamStandardComponentRole %d",
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
                DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamPriorityMgmt");
                priorityMgmType->nVersion.nVersion = OMX_SPEC_VERSION;
                priorityMgmType->nSize = sizeof(priorityMgmType);

                break;
            }
            /* Added for parameter test */
        case OMX_IndexParamCompBufferSupplier:
            {
                OMX_PARAM_BUFFERSUPPLIERTYPE *bufferSupplierType =
                    (OMX_PARAM_BUFFERSUPPLIERTYPE*) paramData;
                DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamCompBufferSupplier");

                bufferSupplierType->nSize = sizeof(bufferSupplierType);
                bufferSupplierType->nVersion.nVersion = OMX_SPEC_VERSION;

                if (0 == bufferSupplierType->nPortIndex)
                    bufferSupplierType->nPortIndex = OMX_BufferSupplyUnspecified;
                else if (1 == bufferSupplierType->nPortIndex)
                    bufferSupplierType->nPortIndex = OMX_BufferSupplyUnspecified;
                else
                    eRet = OMX_ErrorBadPortIndex;


                break;
            }
        case OMX_IndexParamVideoAvc:
            {
                DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamVideoAvc %08x",
                        paramIndex);
                break;
            }
        case OMX_IndexParamVideoH263:
            {
                DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamVideoH263 %08x",
                        paramIndex);
                break;
            }
        case OMX_IndexParamVideoMpeg4:
            {
                DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamVideoMpeg4 %08x",
                        paramIndex);
                break;
            }
        case OMX_IndexParamVideoMpeg2:
            {
                DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamVideoMpeg2 %08x",
                        paramIndex);
                break;
            }
        case OMX_IndexParamVideoProfileLevelQuerySupported:
            {
                DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamVideoProfileLevelQuerySupported %08x", paramIndex);
                OMX_VIDEO_PARAM_PROFILELEVELTYPE *profileLevelType =
                    (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)paramData;
#ifdef MAX_RES_720P
                eRet = get_supported_profile_level_for_720p(profileLevelType);
#endif
#ifdef MAX_RES_1080P
                eRet = get_supported_profile_level_for_1080p(profileLevelType);
#endif
                break;
            }
#if defined (_ANDROID_HONEYCOMB_) || defined (_ANDROID_ICS_)
        case OMX_GoogleAndroidIndexGetAndroidNativeBufferUsage:
            {
                DEBUG_PRINT_HIGH("get_parameter: OMX_GoogleAndroidIndexGetAndroidNativeBufferUsage");
                GetAndroidNativeBufferUsageParams* nativeBuffersUsage = (GetAndroidNativeBufferUsageParams *) paramData;

                if (nativeBuffersUsage->nPortIndex == OMX_CORE_OUTPUT_PORT_INDEX) {
#ifdef USE_ION
#if defined (MAX_RES_720P)
                    nativeBuffersUsage->nUsage = (GRALLOC_USAGE_PRIVATE_CAMERA_HEAP | GRALLOC_USAGE_PRIVATE_UNCACHED);
                    DEBUG_PRINT_HIGH("ION:720P: nUsage 0x%x",nativeBuffersUsage->nUsage);
#else

                    if (secure_mode) {
                        nativeBuffersUsage->nUsage = (GRALLOC_USAGE_PRIVATE_MM_HEAP | GRALLOC_USAGE_PROTECTED |
                                GRALLOC_USAGE_PRIVATE_UNCACHED);
                        DEBUG_PRINT_HIGH("ION:secure_mode: nUsage 0x%x",nativeBuffersUsage->nUsage);
                    } else {
                        nativeBuffersUsage->nUsage = (GRALLOC_USAGE_PRIVATE_MM_HEAP |
                                GRALLOC_USAGE_PRIVATE_IOMMU_HEAP);
                        DEBUG_PRINT_HIGH("ION:non_secure_mode: nUsage 0x%x",nativeBuffersUsage->nUsage);
                    }

#endif //(MAX_RES_720P)
#else // USE_ION
#if defined (MAX_RES_720P) ||  defined (MAX_RES_1080P_EBI)
                    nativeBuffersUsage->nUsage = (GRALLOC_USAGE_PRIVATE_ADSP_HEAP | GRALLOC_USAGE_PRIVATE_UNCACHED);
                    DEBUG_PRINT_HIGH("720P/1080P_EBI: nUsage 0x%x",nativeBuffersUsage->nUsage);
#elif MAX_RES_1080P
                    nativeBuffersUsage->nUsage = (GRALLOC_USAGE_PRIVATE_SMI_HEAP | GRALLOC_USAGE_PRIVATE_UNCACHED);
                    DEBUG_PRINT_HIGH("1080P: nUsage 0x%x",nativeBuffersUsage->nUsage);
#endif
#endif // USE_ION
                } else {
                    DEBUG_PRINT_ERROR("get_parameter: OMX_GoogleAndroidIndexGetAndroidNativeBufferUsage failed!");
                    eRet = OMX_ErrorBadParameter;
                }
            }
            break;
#endif

        default:
            {
                DEBUG_PRINT_ERROR("get_parameter: unknown param %08x", paramIndex);
                eRet =OMX_ErrorUnsupportedIndex;
            }

    }

    DEBUG_PRINT_LOW("get_parameter returning WxH(%d x %d) SxSH(%d x %d)",
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

    if ((params == NULL) ||
            (params->nativeBuffer == NULL) ||
            (params->nativeBuffer->handle == NULL) ||
            !m_enable_android_native_buffers)
        return OMX_ErrorBadParameter;

    m_use_android_native_buffers = OMX_TRUE;
    sp<android_native_buffer_t> nBuf = params->nativeBuffer;
    private_handle_t *handle = (private_handle_t *)nBuf->handle;

    if ((OMX_U32)handle->size < drv_ctx.op_buf.buffer_size) {
        DEBUG_PRINT_ERROR("Insufficient sized buffer given for playback,"
                " expected %u, got %lu",
                drv_ctx.op_buf.buffer_size, (OMX_U32)handle->size);
        return OMX_ErrorBadParameter;
    }

    if (OMX_CORE_OUTPUT_PORT_INDEX == params->nPortIndex) { //android native buffers can be used only on Output port
        OMX_U8 *buffer = NULL;

        if (!secure_mode) {
            buffer = (OMX_U8*)mmap(0, handle->size,
                    PROT_READ|PROT_WRITE, MAP_SHARED, handle->fd, 0);

            if (buffer == MAP_FAILED) {
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

    if (m_state == OMX_StateInvalid) {
        DEBUG_PRINT_ERROR("Set Param in Invalid State");
        return OMX_ErrorInvalidState;
    }

    if (paramData == NULL) {
        DEBUG_PRINT_ERROR("Get Param in Invalid paramData");
        return OMX_ErrorBadParameter;
    }

    if ((m_state != OMX_StateLoaded) &&
            BITMASK_ABSENT(&m_flags,OMX_COMPONENT_OUTPUT_ENABLE_PENDING) &&
            (m_out_bEnabled == OMX_TRUE) &&
            BITMASK_ABSENT(&m_flags, OMX_COMPONENT_INPUT_ENABLE_PENDING) &&
            (m_inp_bEnabled == OMX_TRUE)) {
        DEBUG_PRINT_ERROR("Set Param in Invalid State");
        return OMX_ErrorIncorrectStateOperation;
    }

    switch (paramIndex) {
        case OMX_IndexParamPortDefinition:
            {
                OMX_PARAM_PORTDEFINITIONTYPE *portDefn;
                portDefn = (OMX_PARAM_PORTDEFINITIONTYPE *) paramData;
                //TODO: Check if any allocate buffer/use buffer/useNativeBuffer has
                //been called.
                DEBUG_PRINT_LOW("set_parameter: OMX_IndexParamPortDefinition H= %d, W = %d",
                        (int)portDefn->format.video.nFrameHeight,
                        (int)portDefn->format.video.nFrameWidth);

                if (OMX_DirOutput == portDefn->eDir) {
                    DEBUG_PRINT_LOW("set_parameter: OMX_IndexParamPortDefinition OP port");
                    m_display_id = portDefn->format.video.pNativeWindow;
                    unsigned int buffer_size;

                    if (!client_buffers.get_buffer_req(buffer_size)) {
                        DEBUG_PRINT_ERROR("Error in getting buffer requirements");
                        eRet = OMX_ErrorBadParameter;
                    } else {
                        if ( portDefn->nBufferCountActual >= drv_ctx.op_buf.mincount &&
                                portDefn->nBufferSize >=  buffer_size) {
                            drv_ctx.op_buf.actualcount = portDefn->nBufferCountActual;
                            drv_ctx.op_buf.buffer_size = portDefn->nBufferSize;
                            eRet = set_buffer_req(&drv_ctx.op_buf);

                            if (eRet == OMX_ErrorNone)
                                m_port_def = *portDefn;
                        } else {
                            DEBUG_PRINT_HIGH("ERROR: OP Requirements(#%d: %u) Requested(#%d: %u)",
                                    drv_ctx.op_buf.mincount, drv_ctx.op_buf.buffer_size,
                                    portDefn->nBufferCountActual, portDefn->nBufferSize);
                            eRet = OMX_ErrorBadParameter;
                        }
                    }
                } else if (OMX_DirInput == portDefn->eDir) {
                    if ((portDefn->format.video.xFramerate >> 16) > 0 &&
                            (portDefn->format.video.xFramerate >> 16) <= MAX_SUPPORTED_FPS) {
                        // Frame rate only should be set if this is a "known value" or to
                        // activate ts prediction logic (arbitrary mode only) sending input
                        // timestamps with max value (LLONG_MAX).
                        DEBUG_PRINT_LOW("set_parameter: frame rate set by omx client : %d",
                                portDefn->format.video.xFramerate >> 16);
                        Q16ToFraction(portDefn->format.video.xFramerate, drv_ctx.frame_rate.fps_numerator,
                                drv_ctx.frame_rate.fps_denominator);

                        if (!drv_ctx.frame_rate.fps_numerator) {
                            DEBUG_PRINT_ERROR("Numerator is zero setting to 30");
                            drv_ctx.frame_rate.fps_numerator = 30;
                        }

                        if (drv_ctx.frame_rate.fps_denominator)
                            drv_ctx.frame_rate.fps_numerator = (int)
                                drv_ctx.frame_rate.fps_numerator / drv_ctx.frame_rate.fps_denominator;

                        drv_ctx.frame_rate.fps_denominator = 1;
                        frm_int = drv_ctx.frame_rate.fps_denominator * 1e6 /
                            drv_ctx.frame_rate.fps_numerator;
                        ioctl_msg.in = &drv_ctx.frame_rate;

                        if (ioctl (drv_ctx.video_driver_fd, VDEC_IOCTL_SET_FRAME_RATE,
                                    (void*)&ioctl_msg) < 0) {
                            DEBUG_PRINT_ERROR("Setting frame rate to driver failed");
                        }

                        DEBUG_PRINT_LOW("set_parameter: frm_int(%u) fps(%.2f)",
                                frm_int, drv_ctx.frame_rate.fps_numerator /
                                (float)drv_ctx.frame_rate.fps_denominator);
                    }

                    DEBUG_PRINT_LOW("set_parameter: OMX_IndexParamPortDefinition IP port");

                    if (drv_ctx.video_resolution.frame_height !=
                            portDefn->format.video.nFrameHeight ||
                            drv_ctx.video_resolution.frame_width  !=
                            portDefn->format.video.nFrameWidth) {
                        DEBUG_PRINT_LOW("SetParam IP: WxH(%d x %d)",
                                portDefn->format.video.nFrameWidth,
                                portDefn->format.video.nFrameHeight);

                        if (portDefn->format.video.nFrameHeight != 0x0 &&
                                portDefn->format.video.nFrameWidth != 0x0) {
                            drv_ctx.video_resolution.frame_height =
                                drv_ctx.video_resolution.scan_lines =
                                portDefn->format.video.nFrameHeight;
                            drv_ctx.video_resolution.frame_width =
                                drv_ctx.video_resolution.stride =
                                portDefn->format.video.nFrameWidth;
                            ioctl_msg.in = &drv_ctx.video_resolution;
                            ioctl_msg.out = NULL;

                            if (ioctl (drv_ctx.video_driver_fd, VDEC_IOCTL_SET_PICRES,
                                        (void*)&ioctl_msg) < 0) {
                                DEBUG_PRINT_ERROR("Set Resolution failed");
                                eRet = OMX_ErrorUnsupportedSetting;
                            } else
                                eRet = get_buffer_req(&drv_ctx.op_buf);
                        }
                    } else if (portDefn->nBufferCountActual >= drv_ctx.ip_buf.mincount
                            && portDefn->nBufferSize == (drv_ctx.ip_buf.buffer_size - DEVICE_SCRATCH)) {
                        drv_ctx.ip_buf.actualcount = portDefn->nBufferCountActual;
                        drv_ctx.ip_buf.buffer_size = portDefn->nBufferSize + DEVICE_SCRATCH;
                        eRet = set_buffer_req(&drv_ctx.ip_buf);
                    } else {
                        DEBUG_PRINT_ERROR("ERROR: IP Requirements(#%d: %u) Requested(#%d: %u)",
                                drv_ctx.ip_buf.mincount, drv_ctx.ip_buf.buffer_size,
                                portDefn->nBufferCountActual, portDefn->nBufferSize);
                        eRet = OMX_ErrorBadParameter;
                    }
                } else if (portDefn->eDir ==  OMX_DirMax) {
                    DEBUG_PRINT_ERROR(" Set_parameter: Bad Port idx %d",
                            (int)portDefn->nPortIndex);
                    eRet = OMX_ErrorBadPortIndex;
                }

                DEBUG_PRINT_HIGH("Set_parameter: OMX_IndexParamPortDefinition: "
                        "nPortIndex (%d), nFrameWidth (%d), nFrameHeight (%d), "
                        "nStride (%d), nSliceHeight (%d), nBitrate (%d), xFramerate (0x%x), "
                        "nBufferSize (%d), nBufferCountMin (%d), nBufferCountActual (%d), "
                        "bBuffersContiguous (%d), nBufferAlignment (%d), "
                        "bEnabled (%d), bPopulated (%d), eCompressionFormat (0x%x), "
                        "eColorFormat (0x%x)" , (int)portDefn->nPortIndex,
                        (int)portDefn->format.video.nFrameWidth, (int)portDefn->format.video.nFrameHeight,
                        (int)portDefn->format.video.nStride, (int)portDefn->format.video.nSliceHeight,
                        (int)portDefn->format.video.nBitrate, (int)portDefn->format.video.xFramerate,
                        (int)portDefn->nBufferSize, (int)portDefn->nBufferCountMin,
                        (int)portDefn->nBufferCountActual, (int)portDefn->bBuffersContiguous,
                        (int)portDefn->nBufferAlignment, (int)portDefn->bEnabled, (int)portDefn->bPopulated,
                        (int)portDefn->format.video.eCompressionFormat, (int)portDefn->format.video.eColorFormat);
            }
            break;
        case OMX_IndexParamVideoPortFormat:
            {
                OMX_VIDEO_PARAM_PORTFORMATTYPE *portFmt =
                    (OMX_VIDEO_PARAM_PORTFORMATTYPE *)paramData;

                if (1 == portFmt->nPortIndex) {
                    enum vdec_output_fromat op_format;

                    if (portFmt->eColorFormat == OMX_COLOR_FormatYUV420SemiPlanar)
                        op_format = VDEC_YUV_FORMAT_NV12;
                    else if (portFmt->eColorFormat ==
                            QOMX_COLOR_FormatYUV420PackedSemiPlanar64x32Tile2m8ka ||
                            portFmt->eColorFormat == OMX_COLOR_FormatYUV420Planar)
                        op_format = VDEC_YUV_FORMAT_TILE_4x2;
                    else
                        eRet = OMX_ErrorBadParameter;

                    if (eRet == OMX_ErrorNone && drv_ctx.output_format != op_format) {
                        /*Set the output format*/
                        drv_ctx.output_format = op_format;
                        ioctl_msg.in = &drv_ctx.output_format;
                        ioctl_msg.out = NULL;

                        if (ioctl(drv_ctx.video_driver_fd, VDEC_IOCTL_SET_OUTPUT_FORMAT,
                                    (void*)&ioctl_msg) < 0) {
                            DEBUG_PRINT_ERROR("Set output format failed");
                            eRet = OMX_ErrorUnsupportedSetting;
                        } else {
                            eRet = get_buffer_req(&drv_ctx.op_buf);
                        }
                    }

                    if (eRet == OMX_ErrorNone) {
                        if (!client_buffers.set_color_format(portFmt->eColorFormat)) {
                            DEBUG_PRINT_ERROR("Set color format failed");
                            eRet = OMX_ErrorBadParameter;
                        }
                    }
                }

                DEBUG_PRINT_HIGH("Set_parameter: OMX_IndexParamVideoPortFormat: "
                        "nPortIndex (%d), nIndex (%d), eCompressionFormat (0x%x), "
                        "eColorFormat (0x%x), xFramerate (0x%x)", (int)portFmt->nPortIndex,
                        (int)portFmt->nIndex, (int)portFmt->eCompressionFormat,
                        (int)portFmt->eColorFormat, (int)portFmt->xFramerate);
            }
            break;

        case OMX_QcomIndexPortDefn:
            {
                OMX_QCOM_PARAM_PORTDEFINITIONTYPE *portFmt =
                    (OMX_QCOM_PARAM_PORTDEFINITIONTYPE *) paramData;
                DEBUG_PRINT_LOW("set_parameter: OMX_IndexQcomParamPortDefinitionType %d",
                        portFmt->nFramePackingFormat);

                /* Input port */
                if (portFmt->nPortIndex == 0) {
                    if (portFmt->nFramePackingFormat == OMX_QCOM_FramePacking_Arbitrary) {
                        if (secure_mode) {
                            arbitrary_bytes = false;
                            DEBUG_PRINT_ERROR("setparameter: cannot set to arbitary bytes mode in secure session");
                            eRet = OMX_ErrorUnsupportedSetting;
                        } else {
                            arbitrary_bytes = true;
                            DEBUG_PRINT_HIGH("setparameter: arbitrary_bytes enabled");
                        }
                    } else if (portFmt->nFramePackingFormat ==
                            OMX_QCOM_FramePacking_OnlyOneCompleteFrame) {
                        arbitrary_bytes = false;
                        DEBUG_PRINT_HIGH("setparameter: arbitrary_bytes disabled");
                    } else {
                        DEBUG_PRINT_ERROR("Setparameter: unknown FramePacking format %d",
                                portFmt->nFramePackingFormat);
                        eRet = OMX_ErrorUnsupportedSetting;
                    }
                } else if (portFmt->nPortIndex == OMX_CORE_OUTPUT_PORT_INDEX) {
                    DEBUG_PRINT_HIGH("set_parameter: OMX_IndexQcomParamPortDefinitionType OP Port");

                    if ( (portFmt->nMemRegion > OMX_QCOM_MemRegionInvalid &&
                                portFmt->nMemRegion < OMX_QCOM_MemRegionMax) &&
                            portFmt->nCacheAttr == OMX_QCOM_CacheAttrNone) {
                        m_out_mem_region_smi = OMX_TRUE;

                        if ((m_out_mem_region_smi && m_out_pvt_entry_pmem)) {
                            DEBUG_PRINT_HIGH("set_parameter: OMX_IndexQcomParamPortDefinitionType OP Port: set use_output_pmem");
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
                DEBUG_PRINT_LOW("set_parameter: OMX_IndexParamStandardComponentRole %s",
                        comp_role->cRole);

                if ((m_state == OMX_StateLoaded)&&
                        !BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING)) {
                    DEBUG_PRINT_LOW("Set Parameter called in valid state");
                } else {
                    DEBUG_PRINT_ERROR("Set Parameter called in Invalid State");
                    return OMX_ErrorIncorrectStateOperation;
                }

                if (!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.avc",OMX_MAX_STRINGNAME_SIZE)) {
                    if (!strncmp((char*)comp_role->cRole,"video_decoder.avc",OMX_MAX_STRINGNAME_SIZE)) {
                        strlcpy((char*)m_cRole,"video_decoder.avc",OMX_MAX_STRINGNAME_SIZE);
                    } else {
                        DEBUG_PRINT_ERROR("Setparameter: unknown Index %s", comp_role->cRole);
                        eRet =OMX_ErrorUnsupportedSetting;
                    }
                } else if (!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.mpeg4",OMX_MAX_STRINGNAME_SIZE)) {
                    if (!strncmp((const char*)comp_role->cRole,"video_decoder.mpeg4",OMX_MAX_STRINGNAME_SIZE)) {
                        strlcpy((char*)m_cRole,"video_decoder.mpeg4",OMX_MAX_STRINGNAME_SIZE);
                    } else {
                        DEBUG_PRINT_ERROR("Setparameter: unknown Index %s", comp_role->cRole);
                        eRet = OMX_ErrorUnsupportedSetting;
                    }
                } else if (!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.h263",OMX_MAX_STRINGNAME_SIZE)) {
                    if (!strncmp((const char*)comp_role->cRole,"video_decoder.h263",OMX_MAX_STRINGNAME_SIZE)) {
                        strlcpy((char*)m_cRole,"video_decoder.h263",OMX_MAX_STRINGNAME_SIZE);
                    } else {
                        DEBUG_PRINT_ERROR("Setparameter: unknown Index %s", comp_role->cRole);
                        eRet =OMX_ErrorUnsupportedSetting;
                    }
                } else if (!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.mpeg2",OMX_MAX_STRINGNAME_SIZE)) {
                    if (!strncmp((const char*)comp_role->cRole,"video_decoder.mpeg2",OMX_MAX_STRINGNAME_SIZE)) {
                        strlcpy((char*)m_cRole,"video_decoder.mpeg2",OMX_MAX_STRINGNAME_SIZE);
                    } else {
                        DEBUG_PRINT_ERROR("Setparameter: unknown Index %s", comp_role->cRole);
                        eRet = OMX_ErrorUnsupportedSetting;
                    }
                }

#ifdef MAX_RES_1080P
                else if ((!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.divx",OMX_MAX_STRINGNAME_SIZE)) ||
                        (!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.divx311",OMX_MAX_STRINGNAME_SIZE))
                        )
#else
                else if (!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.divx",OMX_MAX_STRINGNAME_SIZE))
#endif
                {

                    if (!strncmp((const char*)comp_role->cRole,"video_decoder.divx",OMX_MAX_STRINGNAME_SIZE)) {
                        strlcpy((char*)m_cRole,"video_decoder.divx",OMX_MAX_STRINGNAME_SIZE);
                    } else {
                        DEBUG_PRINT_ERROR("Setparameter: unknown Index %s", comp_role->cRole);
                        eRet =OMX_ErrorUnsupportedSetting;
                    }
                } else if ( (!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.vc1",OMX_MAX_STRINGNAME_SIZE)) ||
                        (!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.wmv",OMX_MAX_STRINGNAME_SIZE))
                        ) {
                    if (!strncmp((const char*)comp_role->cRole,"video_decoder.vc1",OMX_MAX_STRINGNAME_SIZE)) {
                        strlcpy((char*)m_cRole,"video_decoder.vc1",OMX_MAX_STRINGNAME_SIZE);
                    } else {
                        DEBUG_PRINT_ERROR("Setparameter: unknown Index %s", comp_role->cRole);
                        eRet =OMX_ErrorUnsupportedSetting;
                    }
                } else {
                    DEBUG_PRINT_ERROR("Setparameter: unknown param %s", drv_ctx.kind);
                    eRet = OMX_ErrorInvalidComponentName;
                }

                break;
            }

        case OMX_IndexParamPriorityMgmt:
            {
                if (m_state != OMX_StateLoaded) {
                    DEBUG_PRINT_ERROR("Set Parameter called in Invalid State");
                    return OMX_ErrorIncorrectStateOperation;
                }

                OMX_PRIORITYMGMTTYPE *priorityMgmtype = (OMX_PRIORITYMGMTTYPE*) paramData;
                DEBUG_PRINT_LOW("set_parameter: OMX_IndexParamPriorityMgmt %d",
                        priorityMgmtype->nGroupID);

                DEBUG_PRINT_LOW("set_parameter: priorityMgmtype %d",
                        priorityMgmtype->nGroupPriority);

                m_priority_mgm.nGroupID = priorityMgmtype->nGroupID;
                m_priority_mgm.nGroupPriority = priorityMgmtype->nGroupPriority;

                break;
            }

        case OMX_IndexParamCompBufferSupplier:
            {
                OMX_PARAM_BUFFERSUPPLIERTYPE *bufferSupplierType = (OMX_PARAM_BUFFERSUPPLIERTYPE*) paramData;
                DEBUG_PRINT_LOW("set_parameter: OMX_IndexParamCompBufferSupplier %d",
                        bufferSupplierType->eBufferSupplier);

                if (bufferSupplierType->nPortIndex == 0 || bufferSupplierType->nPortIndex ==1)
                    m_buffer_supplier.eBufferSupplier = bufferSupplierType->eBufferSupplier;

                else

                    eRet = OMX_ErrorBadPortIndex;

                break;

            }
        case OMX_IndexParamVideoAvc:
            {
                DEBUG_PRINT_LOW("set_parameter: OMX_IndexParamVideoAvc %d",
                        paramIndex);
                break;
            }
        case OMX_IndexParamVideoH263:
            {
                DEBUG_PRINT_LOW("set_parameter: OMX_IndexParamVideoH263 %d",
                        paramIndex);
                break;
            }
        case OMX_IndexParamVideoMpeg4:
            {
                DEBUG_PRINT_LOW("set_parameter: OMX_IndexParamVideoMpeg4 %d",
                        paramIndex);
                break;
            }
        case OMX_IndexParamVideoMpeg2:
            {
                DEBUG_PRINT_LOW("set_parameter: OMX_IndexParamVideoMpeg2 %d",
                        paramIndex);
                break;
            }
        case OMX_QcomIndexParamVideoDecoderPictureOrder:
            {
                QOMX_VIDEO_DECODER_PICTURE_ORDER *pictureOrder =
                    (QOMX_VIDEO_DECODER_PICTURE_ORDER *)paramData;
                enum vdec_output_order pic_order = VDEC_ORDER_DISPLAY;
                DEBUG_PRINT_HIGH("set_parameter: OMX_QcomIndexParamVideoDecoderPictureOrder %d",
                        pictureOrder->eOutputPictureOrder);

                if (pictureOrder->eOutputPictureOrder == QOMX_VIDEO_DISPLAY_ORDER)
                    pic_order = VDEC_ORDER_DISPLAY;
                else if (pictureOrder->eOutputPictureOrder == QOMX_VIDEO_DECODE_ORDER) {
                    pic_order = VDEC_ORDER_DECODE;
                    time_stamp_dts.set_timestamp_reorder_mode(false);
                } else
                    eRet = OMX_ErrorBadParameter;

#ifdef MAX_RES_720P

                if (drv_ctx.idr_only_decoding) {
                    if (pictureOrder->eOutputPictureOrder != QOMX_VIDEO_DECODE_ORDER) {
                        DEBUG_PRINT_HIGH("only decode order is supported for thumbnail mode");
                        eRet = OMX_ErrorBadParameter;
                    }
                }

#endif

                if (eRet == OMX_ErrorNone && pic_order != drv_ctx.picture_order) {
                    drv_ctx.picture_order = pic_order;
                    ioctl_msg.in = &drv_ctx.picture_order;
                    ioctl_msg.out = NULL;

                    if (ioctl(drv_ctx.video_driver_fd, VDEC_IOCTL_SET_PICTURE_ORDER,
                                (void*)&ioctl_msg) < 0) {
                        DEBUG_PRINT_ERROR("Set picture order failed");
                        eRet = OMX_ErrorUnsupportedSetting;
                    }
                }

                break;
            }
        case OMX_QcomIndexParamConcealMBMapExtraData:
            {
                eRet = enable_extradata(VDEC_EXTRADATA_MB_ERROR_MAP,
                        ((QOMX_ENABLETYPE *)paramData)->bEnable);
            }
            break;
        case OMX_QcomIndexParamFrameInfoExtraData:
            {
                eRet = enable_extradata(OMX_FRAMEINFO_EXTRADATA,
                        ((QOMX_ENABLETYPE *)paramData)->bEnable);
            }
            break;
        case OMX_QcomIndexParamInterlaceExtraData:
            {
                eRet = enable_extradata(OMX_INTERLACE_EXTRADATA,
                        ((QOMX_ENABLETYPE *)paramData)->bEnable);
            }
            break;
        case OMX_QcomIndexParamH264TimeInfo:
            {
                eRet = enable_extradata(OMX_TIMEINFO_EXTRADATA,
                        ((QOMX_ENABLETYPE *)paramData)->bEnable);
            }
            break;
        case OMX_QcomIndexParamVideoDivx:
            {
#ifdef MAX_RES_720P
                QOMX_VIDEO_PARAM_DIVXTYPE* divXType = (QOMX_VIDEO_PARAM_DIVXTYPE *) paramData;

                if ((divXType) && (divXType->eFormat == QOMX_VIDEO_DIVXFormat311)) {
                    DEBUG_PRINT_HIGH("set_parameter: DivX 3.11 not supported in 7x30 core.");
                    eRet = OMX_ErrorUnsupportedSetting;
                }

#endif
            }
            break;
        case OMX_QcomIndexPlatformPvt:
            {
                DEBUG_PRINT_HIGH("set_parameter: OMX_QcomIndexPlatformPvt OP Port");
                OMX_QCOM_PLATFORMPRIVATE_EXTN* entryType = (OMX_QCOM_PLATFORMPRIVATE_EXTN *) paramData;

                if (entryType->type != OMX_QCOM_PLATFORM_PRIVATE_PMEM) {
                    DEBUG_PRINT_HIGH("set_parameter: Platform Private entry type (%d) not supported.", entryType->type);
                    eRet = OMX_ErrorUnsupportedSetting;
                } else {
                    m_out_pvt_entry_pmem = OMX_TRUE;

                    if ((m_out_mem_region_smi && m_out_pvt_entry_pmem)) {
                        DEBUG_PRINT_HIGH("set_parameter: OMX_QcomIndexPlatformPvt OP Port: use output pmem set");
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
                int rc = ioctl(drv_ctx.video_driver_fd,
                        VDEC_IOCTL_SET_IDR_ONLY_DECODING);

                if (rc < 0) {
                    DEBUG_PRINT_ERROR("Failed to set IDR only decoding on driver.");
                    eRet = OMX_ErrorHardware;
                }

#ifdef MAX_RES_720P

                if (eRet == OMX_ErrorNone) {
                    DEBUG_PRINT_HIGH("set decode order for thumbnail mode");
                    drv_ctx.picture_order = VDEC_ORDER_DECODE;
                    ioctl_msg.in = &drv_ctx.picture_order;
                    ioctl_msg.out = NULL;

                    if (ioctl(drv_ctx.video_driver_fd, VDEC_IOCTL_SET_PICTURE_ORDER,
                                (void*)&ioctl_msg) < 0) {
                        DEBUG_PRINT_ERROR("Set picture order failed");
                        eRet = OMX_ErrorUnsupportedSetting;
                    }
                }

#endif
            }
            break;
#ifdef MAX_RES_1080P
        case OMX_QcomIndexParamIndexExtraDataType:
            {
                if (!secure_mode) {
                    QOMX_INDEXEXTRADATATYPE *extradataIndexType = (QOMX_INDEXEXTRADATATYPE *) paramData;

                    if ((extradataIndexType->nIndex == OMX_IndexParamPortDefinition) &&
                            (extradataIndexType->bEnabled == OMX_TRUE) &&
                            (extradataIndexType->nPortIndex == 1)) {
                        DEBUG_PRINT_HIGH("set_parameter:  OMX_QcomIndexParamIndexExtraDataType SmoothStreaming");
                        eRet = enable_extradata(OMX_PORTDEF_EXTRADATA, extradataIndexType->bEnabled);
                        // Set smooth streaming parameter
                        int rc = ioctl(drv_ctx.video_driver_fd,
                                VDEC_IOCTL_SET_CONT_ON_RECONFIG);

                        if (rc < 0) {
                            DEBUG_PRINT_ERROR("Failed to enable Smooth Streaming on driver.");
                            eRet = OMX_ErrorHardware;
                        }
                    }
                }
            }
            break;

        case OMX_QcomIndexParamEnableSmoothStreaming:
            {

                int rc = ioctl(drv_ctx.video_driver_fd,
                        VDEC_IOCTL_SET_CONT_ON_RECONFIG);

                if (rc < 0) {
                    DEBUG_PRINT_ERROR("Failed to enable Smooth Streaming on driver.");
                    eRet = OMX_ErrorHardware;
                }
            }
            break;
#endif
#if defined (_ANDROID_HONEYCOMB_) || defined (_ANDROID_ICS_)
            /* Need to allow following two set_parameters even in Idle
             * state. This is ANDROID architecture which is not in sync
             * with openmax standard. */
        case OMX_GoogleAndroidIndexEnableAndroidNativeBuffers:
            {
                EnableAndroidNativeBuffersParams* enableNativeBuffers = (EnableAndroidNativeBuffersParams *) paramData;

                if (enableNativeBuffers) {
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
                    } else
                        time_stamp_dts.set_timestamp_reorder_mode(false);
                } else {
                    time_stamp_dts.set_timestamp_reorder_mode(false);

                    if (reorder->bEnable == OMX_TRUE) {
                        eRet = OMX_ErrorUnsupportedSetting;
                    }
                }
            }
            break;
        case OMX_QcomIndexEnableExtnUserData:
            {
                eRet = enable_extradata(OMX_EXTNUSER_EXTRADATA,
                        ((QOMX_ENABLETYPE *)paramData)->bEnable);
            }
            break;
        default:
            {
                DEBUG_PRINT_ERROR("Setparameter: unknown param %d", paramIndex);
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

    if (m_state == OMX_StateInvalid) {
        DEBUG_PRINT_ERROR("Get Config in Invalid State");
        return OMX_ErrorInvalidState;
    }

    switch (configIndex) {
        case OMX_QcomIndexConfigInterlaced:
            {
                OMX_QCOM_CONFIG_INTERLACETYPE *configFmt =
                    (OMX_QCOM_CONFIG_INTERLACETYPE *) configData;

                if (configFmt->nPortIndex == 1) {
                    if (configFmt->nIndex == 0) {
                        configFmt->eInterlaceType = OMX_QCOM_InterlaceFrameProgressive;
                    } else if (configFmt->nIndex == 1) {
                        configFmt->eInterlaceType =
                            OMX_QCOM_InterlaceInterleaveFrameTopFieldFirst;
                    } else if (configFmt->nIndex == 2) {
                        configFmt->eInterlaceType =
                            OMX_QCOM_InterlaceInterleaveFrameBottomFieldFirst;
                    } else {
                        DEBUG_PRINT_ERROR("get_config: OMX_QcomIndexConfigInterlaced:"
                                " NoMore Interlaced formats");
                        eRet = OMX_ErrorNoMore;
                    }

                } else {
                    DEBUG_PRINT_ERROR("get_config: Bad port index %d queried on only o/p port",
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
                ioctl_msg.out = (void*)&decoderinstances->nNumOfInstances;
                (void)(ioctl(drv_ctx.video_driver_fd,
                            VDEC_IOCTL_GET_NUMBER_INSTANCES,&ioctl_msg));
                break;
            }
        case OMX_QcomIndexConfigVideoFramePackingArrangement:
            {
                if (drv_ctx.decoder_format == VDEC_CODECTYPE_H264) {
                    OMX_QCOM_FRAME_PACK_ARRANGEMENT *configFmt =
                        (OMX_QCOM_FRAME_PACK_ARRANGEMENT *) configData;
                    h264_parser->get_frame_pack_data(configFmt);
                } else {
                    DEBUG_PRINT_ERROR("get_config: Framepack data not supported for non H264 codecs");
                }

                break;
            }
        case OMX_QcomIndexParamFrameInfoExtraData:
            {
                OMX_QCOM_EXTRADATA_FRAMEINFO *extradata =
                    (OMX_QCOM_EXTRADATA_FRAMEINFO *) configData;

                if (m_extradata == NULL) {
                    DEBUG_PRINT_LOW("get_config: m_extradata not set. "
                            "Aspect Ratio information missing!!");
                } else {
                    extradata->aspectRatio.aspectRatioX =
                        m_extradata->aspectRatio.aspectRatioX;
                    extradata->aspectRatio.aspectRatioY =
                        m_extradata->aspectRatio.aspectRatioY;
                }

                break;
            }

        default:
            {
                DEBUG_PRINT_ERROR("get_config: unknown param %d",configIndex);
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
    if (m_state == OMX_StateInvalid) {
        DEBUG_PRINT_ERROR("Get Config in Invalid State");
        return OMX_ErrorInvalidState;
    }

    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_VIDEO_CONFIG_NALSIZE *pNal;

    DEBUG_PRINT_LOW("Set Config Called");

    if (m_state == OMX_StateExecuting) {
        DEBUG_PRINT_ERROR("set_config:Ignore in Exe state");
        return ret;
    }

    if (configIndex == OMX_IndexVendorVideoExtraData) {
        OMX_VENDOR_EXTRADATATYPE *config = (OMX_VENDOR_EXTRADATATYPE *) configData;
        DEBUG_PRINT_LOW("Index OMX_IndexVendorVideoExtraData called");

        if (!strcmp(drv_ctx.kind, "OMX.qcom.video.decoder.avc")) {
            DEBUG_PRINT_LOW("Index OMX_IndexVendorVideoExtraData AVC");
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

            if (nal_length > 2) {
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

            DEBUG_PRINT_LOW("Rxd SPS+PPS nPortIndex[%d] len[%d] data[0x%x]",
                    m_vendor_config.nPortIndex,
                    m_vendor_config.nDataSize,
                    m_vendor_config.pData);

            while (index < 2) {
                uint8 *psize;
                len = *pSrcBuf;
                len = len << 8;
                len |= *(pSrcBuf + 1);
                psize = (uint8 *) & len;
                memcpy(pDestBuf + nal_length, pSrcBuf + 2,len);

                for (int i = 0; i < nal_length; i++) {
                    pDestBuf[i] = psize[nal_length - 1 - i];
                }

                //memcpy(pDestBuf,pSrcBuf,(len+2));
                pDestBuf += len + nal_length;
                pSrcBuf += len + 2;
                index++;
                pSrcBuf++;   // skip picture param set
                len = 0;
            }
        } else if (!strcmp(drv_ctx.kind, "OMX.qcom.video.decoder.mpeg4") ||
                !strcmp(drv_ctx.kind, "OMX.qcom.video.decoder.mpeg2")) {
            m_vendor_config.nPortIndex = config->nPortIndex;
            m_vendor_config.nDataSize = config->nDataSize;
            m_vendor_config.pData = (OMX_U8 *) malloc((config->nDataSize));
            memcpy(m_vendor_config.pData, config->pData,config->nDataSize);
        } else if (!strcmp(drv_ctx.kind, "OMX.qcom.video.decoder.vc1")) {
            if (m_vendor_config.pData) {
                free(m_vendor_config.pData);
                m_vendor_config.pData = NULL;
                m_vendor_config.nDataSize = 0;
            }

            if (((*((OMX_U32 *) config->pData)) &
                        VC1_SP_MP_START_CODE_MASK) ==
                    VC1_SP_MP_START_CODE) {
                DEBUG_PRINT_LOW("set_config - VC1 simple/main profile");
                m_vendor_config.nPortIndex = config->nPortIndex;
                m_vendor_config.nDataSize = config->nDataSize;
                m_vendor_config.pData =
                    (OMX_U8 *) malloc(config->nDataSize);
                memcpy(m_vendor_config.pData, config->pData,
                        config->nDataSize);
                m_vc1_profile = VC1_SP_MP_RCV;
            } else if (*((OMX_U32 *) config->pData) == VC1_AP_SEQ_START_CODE) {
                DEBUG_PRINT_LOW("set_config - VC1 Advance profile");
                m_vendor_config.nPortIndex = config->nPortIndex;
                m_vendor_config.nDataSize = config->nDataSize;
                m_vendor_config.pData =
                    (OMX_U8 *) malloc((config->nDataSize));
                memcpy(m_vendor_config.pData, config->pData,
                        config->nDataSize);
                m_vc1_profile = VC1_AP;
            } else if ((config->nDataSize == VC1_STRUCT_C_LEN)) {
                DEBUG_PRINT_LOW("set_config - VC1 Simple/Main profile struct C only");
                m_vendor_config.nPortIndex = config->nPortIndex;
                m_vendor_config.nDataSize  = config->nDataSize;
                m_vendor_config.pData = (OMX_U8*)malloc(config->nDataSize);
                memcpy(m_vendor_config.pData,config->pData,config->nDataSize);
                m_vc1_profile = VC1_SP_MP_RCV;
            } else {
                DEBUG_PRINT_LOW("set_config - Error: Unknown VC1 profile");
            }
        }

        return ret;
    } else if (configIndex == OMX_IndexConfigVideoNalSize) {

        pNal = reinterpret_cast < OMX_VIDEO_CONFIG_NALSIZE * >(configData);
        nal_length = pNal->nNaluBytes;
        m_frame_parser.init_nal_length(nal_length);
        DEBUG_PRINT_LOW("OMX_IndexConfigVideoNalSize called with Size %d",nal_length);
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
    if (m_state == OMX_StateInvalid) {
        DEBUG_PRINT_ERROR("Get Extension Index in Invalid State");
        return OMX_ErrorInvalidState;
    } else if (!strncmp(paramName, "OMX.QCOM.index.param.video.SyncFrameDecodingMode",sizeof("OMX.QCOM.index.param.video.SyncFrameDecodingMode") - 1)) {
        *indexType = (OMX_INDEXTYPE)OMX_QcomIndexParamVideoSyncFrameDecodingMode;
    }

#ifdef MAX_RES_1080P
    else if (!strncmp(paramName, "OMX.QCOM.index.param.IndexExtraData",sizeof("OMX.QCOM.index.param.IndexExtraData") - 1)) {
        *indexType = (OMX_INDEXTYPE)OMX_QcomIndexParamIndexExtraDataType;
    }

#endif
#if defined (_ANDROID_HONEYCOMB_) || defined (_ANDROID_ICS_)
    else if (!strncmp(paramName,"OMX.google.android.index.enableAndroidNativeBuffers", sizeof("OMX.google.android.index.enableAndroidNativeBuffers") - 1)) {
        *indexType = (OMX_INDEXTYPE)OMX_GoogleAndroidIndexEnableAndroidNativeBuffers;
    } else if (!strncmp(paramName,"OMX.google.android.index.useAndroidNativeBuffer2", sizeof("OMX.google.android.index.enableAndroidNativeBuffer2") - 1)) {
        *indexType = (OMX_INDEXTYPE)OMX_GoogleAndroidIndexUseAndroidNativeBuffer2;
    } else if (!strncmp(paramName,"OMX.google.android.index.useAndroidNativeBuffer", sizeof("OMX.google.android.index.enableAndroidNativeBuffer") - 1)) {
        DEBUG_PRINT_ERROR("Extension: %s is supported", paramName);
        *indexType = (OMX_INDEXTYPE)OMX_GoogleAndroidIndexUseAndroidNativeBuffer;
    } else if (!strncmp(paramName,"OMX.google.android.index.getAndroidNativeBufferUsage", sizeof("OMX.google.android.index.getAndroidNativeBufferUsage") - 1)) {
        *indexType = (OMX_INDEXTYPE)OMX_GoogleAndroidIndexGetAndroidNativeBufferUsage;
    }

#endif
    else {
        DEBUG_PRINT_ERROR("Extension: %s not implemented", paramName);
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
    DEBUG_PRINT_LOW("get_state: Returning the state %d",*state);
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
    DEBUG_PRINT_ERROR("Error: component_tunnel_request Not Implemented");
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
#if defined(_ANDROID_HONEYCOMB_) || defined(_ANDROID_ICS_)
    private_handle_t *handle = NULL;
#endif
    OMX_U8 *buff = buffer;

    if (!m_out_mem_ptr) {
        DEBUG_PRINT_HIGH("Use_op_buf:Allocating output headers");
        eRet = allocate_output_headers();

#ifdef MAX_RES_1080P

        if (secure_mode) {
            eRet = vdec_alloc_meta_buffers();

            if (eRet) {
                DEBUG_PRINT_ERROR("ERROR in allocating meta buffers");
                return OMX_ErrorInsufficientResources;
            }
        }

        if (drv_ctx.decoder_format == VDEC_CODECTYPE_H264) {
            //allocate H264_mv_buffer
            eRet = vdec_alloc_h264_mv();

            if (eRet) {
                DEBUG_PRINT_ERROR("ERROR in allocating MV buffers");
                return OMX_ErrorInsufficientResources;
            }
        }

#endif

    }

    if (eRet == OMX_ErrorNone) {
        for (i=0; i< drv_ctx.op_buf.actualcount; i++) {
            if (BITMASK_ABSENT(&m_out_bm_count,i)) {
                break;
            }
        }
    }

    if (i >= drv_ctx.op_buf.actualcount) {
        eRet = OMX_ErrorInsufficientResources;
    }

    if (eRet == OMX_ErrorNone) {
#if defined(_ANDROID_HONEYCOMB_) || defined(_ANDROID_ICS_)

        if (m_enable_android_native_buffers) {
            if (m_use_android_native_buffers) {
                UseAndroidNativeBufferParams *params = (UseAndroidNativeBufferParams *)appData;
                sp<android_native_buffer_t> nBuf = params->nativeBuffer;
                handle = (private_handle_t *)nBuf->handle;
                privateAppData = params->pAppPrivate;
            } else {
                handle = (private_handle_t *)buff;
                privateAppData = appData;
            }

            if ((OMX_U32)handle->size < drv_ctx.op_buf.buffer_size) {
                DEBUG_PRINT_ERROR("Insufficient sized buffer given for playback,"
                        " expected %u, got %lu",
                        drv_ctx.op_buf.buffer_size, (OMX_U32)handle->size);
                return OMX_ErrorBadParameter;
            }

            if (!m_use_android_native_buffers) {
                if (!secure_mode) {
                    buff =  (OMX_U8*)mmap(0, handle->size,
                            PROT_READ|PROT_WRITE, MAP_SHARED, handle->fd, 0);

                    if (buff == MAP_FAILED) {
                        DEBUG_PRINT_ERROR("Failed to mmap pmem with fd = %d, size = %d", handle->fd, handle->size);
                        return OMX_ErrorInsufficientResources;
                    }
                }
            }

#if defined(_ANDROID_ICS_)
            native_buffer[i].nativehandle = handle;
#endif

            if (!handle) {
                DEBUG_PRINT_ERROR("Native Buffer handle is NULL");
                return OMX_ErrorBadParameter;
            }

            drv_ctx.ptr_outputbuffer[i].pmem_fd = handle->fd;
            drv_ctx.ptr_outputbuffer[i].offset = 0;
            drv_ctx.ptr_outputbuffer[i].bufferaddr = buff;
            drv_ctx.ptr_outputbuffer[i].mmaped_size =
                drv_ctx.ptr_outputbuffer[i].buffer_len = drv_ctx.op_buf.buffer_size;
#if defined(_ANDROID_ICS_)

            if (drv_ctx.interlace != VDEC_InterlaceFrameProgressive) {
                int enable = 1;
                setMetaData(handle, PP_PARAM_INTERLACED, (void*)&enable);
            }

#endif
        } else
#endif

            if (!ouput_egl_buffers && !m_use_output_pmem) {
#ifdef USE_ION
                drv_ctx.op_buf_ion_info[i].ion_device_fd = alloc_map_ion_memory(
                        drv_ctx.op_buf.buffer_size,drv_ctx.op_buf.alignment,
                        &drv_ctx.op_buf_ion_info[i].ion_alloc_data,
                        &drv_ctx.op_buf_ion_info[i].fd_ion_data,ION_FLAG_CACHED);

                if (drv_ctx.op_buf_ion_info[i].ion_device_fd < 0) {
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

                if (drv_ctx.ptr_outputbuffer[i].pmem_fd == 0) {
                    drv_ctx.ptr_outputbuffer[i].pmem_fd = \
                                                          open (MEM_DEVICE,O_RDWR);

                    if (drv_ctx.ptr_outputbuffer[i].pmem_fd < 0) {
                        return OMX_ErrorInsufficientResources;
                    }
                }

                if (!align_pmem_buffers(drv_ctx.ptr_outputbuffer[i].pmem_fd,
                            drv_ctx.op_buf.buffer_size,
                            drv_ctx.op_buf.alignment)) {
                    DEBUG_PRINT_ERROR("align_pmem_buffers() failed");
                    close(drv_ctx.ptr_outputbuffer[i].pmem_fd);
                    return OMX_ErrorInsufficientResources;
                }

#endif

                if (!secure_mode) {
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
            } else {

                DEBUG_PRINT_LOW("Use_op_buf: out_pmem=%d",m_use_output_pmem);

                if (!appData || !bytes ) {
                    DEBUG_PRINT_ERROR("Invalid appData or bytes");
                    return OMX_ErrorBadParameter;
                }

                if (!secure_mode && !buffer) {
                    DEBUG_PRINT_ERROR("Bad parameters for use buffer in EGL image case");
                    return OMX_ErrorBadParameter;
                }


                OMX_QCOM_PLATFORM_PRIVATE_LIST *pmem_list;
                OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO *pmem_info;
                pmem_list = (OMX_QCOM_PLATFORM_PRIVATE_LIST*) appData;

                if (!pmem_list->entryList || !pmem_list->entryList->entry ||
                        !pmem_list->nEntries ||
                        pmem_list->entryList->type != OMX_QCOM_PLATFORM_PRIVATE_PMEM) {
                    DEBUG_PRINT_ERROR("Pmem info not valid in use buffer");
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

        if (secure_mode)
            drv_ctx.ptr_outputbuffer[i].bufferaddr = *bufferHdr;

        setbuffers.buffer_type = VDEC_BUFFER_TYPE_OUTPUT;
        memcpy (&setbuffers.buffer,&drv_ctx.ptr_outputbuffer[i],
                sizeof (vdec_bufferpayload));

        ioctl_msg.in  = &setbuffers;
        ioctl_msg.out = NULL;

        DEBUG_PRINT_HIGH("Set the Output Buffer Idx: %d Addr: %x, pmem_fd=%0x%x", i,
                drv_ctx.ptr_outputbuffer[i],drv_ctx.ptr_outputbuffer[i].pmem_fd );

        if (ioctl (drv_ctx.video_driver_fd,VDEC_IOCTL_SET_BUFFER,
                    &ioctl_msg) < 0) {
            DEBUG_PRINT_ERROR("Set output buffer failed");
            return OMX_ErrorInsufficientResources;
        }

        // found an empty buffer at i
        (*bufferHdr)->nAllocLen = drv_ctx.op_buf.buffer_size;

        if (m_enable_android_native_buffers) {
            DEBUG_PRINT_LOW("setting pBuffer to private_handle_t %p", handle);
            (*bufferHdr)->pBuffer = (OMX_U8 *)handle;
        } else {
            (*bufferHdr)->pBuffer = buff;
        }

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
    DEBUG_PRINT_LOW("Inside %s, %p", __FUNCTION__, buffer);
    OMX_ERRORTYPE eRet = OMX_ErrorNone;

    if (!m_inp_heap_ptr)
        m_inp_heap_ptr = (OMX_BUFFERHEADERTYPE*)
            calloc( (sizeof(OMX_BUFFERHEADERTYPE)),
                    drv_ctx.ip_buf.actualcount);

    if (!m_phdr_pmem_ptr)
        m_phdr_pmem_ptr = (OMX_BUFFERHEADERTYPE**)
            calloc( (sizeof(OMX_BUFFERHEADERTYPE*)),
                    drv_ctx.ip_buf.actualcount);

    if (!m_inp_heap_ptr || !m_phdr_pmem_ptr) {
        DEBUG_PRINT_ERROR("Insufficent memory");
        eRet = OMX_ErrorInsufficientResources;
    } else if (m_in_alloc_cnt < drv_ctx.ip_buf.actualcount) {
        input_use_buffer = true;
        memset(&m_inp_heap_ptr[m_in_alloc_cnt], 0, sizeof(OMX_BUFFERHEADERTYPE));
        m_inp_heap_ptr[m_in_alloc_cnt].pBuffer = buffer;
        m_inp_heap_ptr[m_in_alloc_cnt].nAllocLen = bytes;
        m_inp_heap_ptr[m_in_alloc_cnt].pAppPrivate = appData;
        m_inp_heap_ptr[m_in_alloc_cnt].nInputPortIndex = (OMX_U32) OMX_DirInput;
        m_inp_heap_ptr[m_in_alloc_cnt].nOutputPortIndex = (OMX_U32) OMX_DirMax;
        *bufferHdr = &m_inp_heap_ptr[m_in_alloc_cnt];
        eRet = allocate_input_buffer(hComp, &m_phdr_pmem_ptr[m_in_alloc_cnt], port, appData, bytes);
        DEBUG_PRINT_HIGH("Heap buffer(%p) Pmem buffer(%p)", *bufferHdr, m_phdr_pmem_ptr[m_in_alloc_cnt]);

        if (!m_input_free_q.insert_entry((unsigned)m_phdr_pmem_ptr[m_in_alloc_cnt], NULL, NULL)) {
            DEBUG_PRINT_ERROR("ERROR:Free_q is full");
            return OMX_ErrorInsufficientResources;
        }

        m_in_alloc_cnt++;
    } else {
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

    if (bufferHdr == NULL || bytes == 0) {
        DEBUG_PRINT_ERROR("bad param 0x%p %ld",bufferHdr, bytes);
        return OMX_ErrorBadParameter;
    }

    if (!secure_mode && buffer == NULL) {
        DEBUG_PRINT_ERROR("bad param 0x%p",buffer);
        return OMX_ErrorBadParameter;
    }

    if (m_state == OMX_StateInvalid) {
        DEBUG_PRINT_ERROR("Use Buffer in Invalid State");
        return OMX_ErrorInvalidState;
    }

    if (port == OMX_CORE_INPUT_PORT_INDEX)
        error = use_input_heap_buffers(hComp, bufferHdr, port, appData, bytes, buffer);
    else if (port == OMX_CORE_OUTPUT_PORT_INDEX)
        error = use_output_buffer(hComp,bufferHdr,port,appData,bytes,buffer); //not tested
    else {
        DEBUG_PRINT_ERROR("Error: Invalid Port Index received %d",(int)port);
        error = OMX_ErrorBadPortIndex;
    }

    DEBUG_PRINT_LOW("Use Buffer: port %u, buffer %p, eRet %d", port, *bufferHdr, error);

    if (error == OMX_ErrorNone) {
        if (allocate_done() && BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING)) {
            // Send the callback now
            BITMASK_CLEAR((&m_flags),OMX_COMPONENT_IDLE_PENDING);
            post_event(OMX_CommandStateSet,OMX_StateIdle,
                    OMX_COMPONENT_GENERATE_EVENT);
        }

        if (port == OMX_CORE_INPUT_PORT_INDEX && m_inp_bPopulated &&
                BITMASK_PRESENT(&m_flags,OMX_COMPONENT_INPUT_ENABLE_PENDING)) {
            BITMASK_CLEAR((&m_flags),OMX_COMPONENT_INPUT_ENABLE_PENDING);
            post_event(OMX_CommandPortEnable,
                    OMX_CORE_INPUT_PORT_INDEX,
                    OMX_COMPONENT_GENERATE_EVENT);
        } else if (port == OMX_CORE_OUTPUT_PORT_INDEX && m_out_bPopulated &&
                BITMASK_PRESENT(&m_flags,OMX_COMPONENT_OUTPUT_ENABLE_PENDING)) {
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
    if (m_inp_heap_ptr && !input_use_buffer && arbitrary_bytes) {
        if (m_inp_heap_ptr[bufferindex].pBuffer)
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

    if (bufferHdr == NULL || m_inp_mem_ptr == NULL) {
        return OMX_ErrorBadParameter;
    }

    index = bufferHdr - m_inp_mem_ptr;
    DEBUG_PRINT_LOW("Free Input Buffer index = %d",index);

    if (index < drv_ctx.ip_buf.actualcount && drv_ctx.ptr_inputbuffer) {
        DEBUG_PRINT_LOW("Free Input Buffer index = %d",index);

        if (drv_ctx.ptr_inputbuffer[index].pmem_fd > 0) {
            struct vdec_ioctl_msg ioctl_msg = {NULL,NULL};
            struct vdec_setbuffer_cmd setbuffers;
            setbuffers.buffer_type = VDEC_BUFFER_TYPE_INPUT;
            memcpy (&setbuffers.buffer,&drv_ctx.ptr_inputbuffer[index],
                    sizeof (vdec_bufferpayload));
            ioctl_msg.in  = &setbuffers;
            ioctl_msg.out = NULL;
            int ioctl_r = ioctl (drv_ctx.video_driver_fd,
                    VDEC_IOCTL_FREE_BUFFER, &ioctl_msg);

            if (ioctl_r < 0) {
                DEBUG_PRINT_ERROR("VDEC_IOCTL_FREE_BUFFER returned error %d", ioctl_r);
            }

            if (!secure_mode) {
                DEBUG_PRINT_LOW("unmap the input buffer fd=%d",
                        drv_ctx.ptr_inputbuffer[index].pmem_fd);
                DEBUG_PRINT_LOW("unmap the input buffer size=%d  address = %d",
                        drv_ctx.ptr_inputbuffer[index].mmaped_size,
                        drv_ctx.ptr_inputbuffer[index].bufferaddr);
                munmap (drv_ctx.ptr_inputbuffer[index].bufferaddr,
                        drv_ctx.ptr_inputbuffer[index].mmaped_size);
            }

            close (drv_ctx.ptr_inputbuffer[index].pmem_fd);
            drv_ctx.ptr_inputbuffer[index].pmem_fd = -1;

            if (m_desc_buffer_ptr && m_desc_buffer_ptr[index].buf_addr) {
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

    if (bufferHdr == NULL || m_out_mem_ptr == NULL) {
        DEBUG_PRINT_ERROR("free_output_buffer ERROR");
        return OMX_ErrorBadParameter;
    }

    index = bufferHdr - m_out_mem_ptr;
    DEBUG_PRINT_LOW("Free ouput Buffer index = %d",index);

    if (index < drv_ctx.op_buf.actualcount
            && drv_ctx.ptr_outputbuffer) {
        DEBUG_PRINT_LOW("Free ouput Buffer index = %d addr = %x", index,
                drv_ctx.ptr_outputbuffer[index].bufferaddr);

        struct vdec_ioctl_msg ioctl_msg = {NULL,NULL};
        struct vdec_setbuffer_cmd setbuffers;
        setbuffers.buffer_type = VDEC_BUFFER_TYPE_OUTPUT;
        memcpy (&setbuffers.buffer,&drv_ctx.ptr_outputbuffer[index],
                sizeof (vdec_bufferpayload));
        ioctl_msg.in  = &setbuffers;
        ioctl_msg.out = NULL;
        DEBUG_PRINT_LOW("Release the Output Buffer");

        if (ioctl (drv_ctx.video_driver_fd, VDEC_IOCTL_FREE_BUFFER,
                    &ioctl_msg) < 0)
            DEBUG_PRINT_ERROR("Release output buffer failed in VCD");

#ifdef _ANDROID_

        if (m_enable_android_native_buffers) {
            if (drv_ctx.ptr_outputbuffer[index].pmem_fd > 0) {
                if (!secure_mode) {
                    munmap(drv_ctx.ptr_outputbuffer[index].bufferaddr,
                            drv_ctx.ptr_outputbuffer[index].mmaped_size);
                }
            }

            drv_ctx.ptr_outputbuffer[index].pmem_fd = -1;
        } else {
#endif

            if (drv_ctx.ptr_outputbuffer[index].pmem_fd > 0 && !ouput_egl_buffers && !m_use_output_pmem) {
                if (!secure_mode) {
                    DEBUG_PRINT_LOW("unmap the output buffer fd = %d",
                            drv_ctx.ptr_outputbuffer[index].pmem_fd);
                    DEBUG_PRINT_LOW("unmap the ouput buffer size=%d  address = %d",
                            drv_ctx.ptr_outputbuffer[index].mmaped_size,
                            drv_ctx.ptr_outputbuffer[index].bufferaddr);
                    munmap (drv_ctx.ptr_outputbuffer[index].bufferaddr,
                            drv_ctx.ptr_outputbuffer[index].mmaped_size);
                }

                close (drv_ctx.ptr_outputbuffer[index].pmem_fd);
                drv_ctx.ptr_outputbuffer[index].pmem_fd = -1;
#ifdef USE_ION
                free_ion_memory(&drv_ctx.op_buf_ion_info[index]);
#endif
#ifdef _ANDROID_
                m_heap_ptr[index].video_heap_ptr = NULL;
                m_heap_count = m_heap_count - 1;

                if (m_heap_count == 0) {
                    free(m_heap_ptr);
                    m_heap_ptr = NULL;
                }

#endif // _ANDROID_
            }

#ifdef _ANDROID_
        }

#endif
    }

#ifdef MAX_RES_1080P

    if (secure_mode) {
        vdec_dealloc_meta_buffers();
    }

    if (drv_ctx.decoder_format == VDEC_CODECTYPE_H264) {
        vdec_dealloc_h264_mv();
    }

#endif

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
    if (bufferHdr == NULL) {
        return OMX_ErrorBadParameter;
    }

    if (m_inp_heap_ptr == NULL) {
        m_inp_heap_ptr = (OMX_BUFFERHEADERTYPE*) \
                         calloc( (sizeof(OMX_BUFFERHEADERTYPE)),
                                 drv_ctx.ip_buf.actualcount);
        m_phdr_pmem_ptr = (OMX_BUFFERHEADERTYPE**) \
                          calloc( (sizeof(OMX_BUFFERHEADERTYPE*)),
                                  drv_ctx.ip_buf.actualcount);

        if (m_inp_heap_ptr == NULL) {
            DEBUG_PRINT_ERROR("m_inp_heap_ptr Allocation failed ");
            return OMX_ErrorInsufficientResources;
        }
    }

    /*Find a Free index*/
    for (i=0; i< drv_ctx.ip_buf.actualcount; i++) {
        if (BITMASK_ABSENT(&m_heap_inp_bm_count,i)) {
            DEBUG_PRINT_LOW("Free Input Buffer Index %d",i);
            break;
        }
    }

    if (i < drv_ctx.ip_buf.actualcount) {
        buf_addr = (unsigned char *)malloc (drv_ctx.ip_buf.buffer_size);

        if (buf_addr == NULL) {
            return OMX_ErrorInsufficientResources;
        }

        *bufferHdr = (m_inp_heap_ptr + i);
        input = *bufferHdr;
        BITMASK_SET(&m_heap_inp_bm_count,i);

        input->pBuffer           = (OMX_U8 *)buf_addr;
        input->nSize             = sizeof(OMX_BUFFERHEADERTYPE);
        input->nVersion.nVersion = OMX_SPEC_VERSION;
        input->nAllocLen         = drv_ctx.ip_buf.buffer_size - DEVICE_SCRATCH;
        input->pAppPrivate       = appData;
        input->nInputPortIndex   = OMX_CORE_INPUT_PORT_INDEX;
        DEBUG_PRINT_LOW("Address of Heap Buffer %p",*bufferHdr );
        eRet = allocate_input_buffer(hComp,&m_phdr_pmem_ptr [i],port,appData,bytes);
        DEBUG_PRINT_LOW("Address of Pmem Buffer %p",m_phdr_pmem_ptr [i] );

        /*Add the Buffers to freeq*/
        if (!m_input_free_q.insert_entry((unsigned)m_phdr_pmem_ptr [i],NULL,NULL)) {
            DEBUG_PRINT_ERROR("ERROR:Free_q is full");
            return OMX_ErrorInsufficientResources;
        }
    } else {
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

    if ((bytes + DEVICE_SCRATCH) != drv_ctx.ip_buf.buffer_size) {
        DEBUG_PRINT_LOW("Requested Size is wrong %d epected is %d",
                bytes, drv_ctx.ip_buf.buffer_size);
        //return OMX_ErrorBadParameter;
    }

    if (!m_inp_mem_ptr) {
        DEBUG_PRINT_HIGH("Allocate i/p buffer Header: Cnt(%d) Sz(%d)",
                drv_ctx.ip_buf.actualcount,
                drv_ctx.ip_buf.buffer_size);

        m_inp_mem_ptr = (OMX_BUFFERHEADERTYPE*) \
                        calloc( (sizeof(OMX_BUFFERHEADERTYPE)), drv_ctx.ip_buf.actualcount);

        if (m_inp_mem_ptr == NULL) {
            return OMX_ErrorInsufficientResources;
        }

        drv_ctx.ptr_inputbuffer = (struct vdec_bufferpayload *) \
                                  calloc ((sizeof (struct vdec_bufferpayload)),drv_ctx.ip_buf.actualcount);

        if (drv_ctx.ptr_inputbuffer == NULL) {
            return OMX_ErrorInsufficientResources;
        }

#ifdef USE_ION
        drv_ctx.ip_buf_ion_info = (struct vdec_ion *) \
                                  calloc ((sizeof (struct vdec_ion)),drv_ctx.ip_buf.actualcount);

        if (drv_ctx.ip_buf_ion_info == NULL) {
            return OMX_ErrorInsufficientResources;
        }

#endif

        for (i=0; i < drv_ctx.ip_buf.actualcount; i++) {
            drv_ctx.ptr_inputbuffer [i].pmem_fd = -1;
#ifdef USE_ION
            drv_ctx.ip_buf_ion_info[i].ion_device_fd = -1;
#endif
        }
    }

    for (i=0; i< drv_ctx.ip_buf.actualcount; i++) {
        if (BITMASK_ABSENT(&m_inp_bm_count,i)) {
            DEBUG_PRINT_LOW("Free Input Buffer Index %d",i);
            break;
        }
    }

    if (i < drv_ctx.ip_buf.actualcount) {
        DEBUG_PRINT_LOW("Allocate input Buffer");

#ifdef USE_ION
        drv_ctx.ip_buf_ion_info[i].ion_device_fd = alloc_map_ion_memory(
                drv_ctx.ip_buf.buffer_size,drv_ctx.op_buf.alignment,
                &drv_ctx.ip_buf_ion_info[i].ion_alloc_data,
                &drv_ctx.ip_buf_ion_info[i].fd_ion_data,ION_FLAG_CACHED);

        if (drv_ctx.ip_buf_ion_info[i].ion_device_fd < 0) {
            return OMX_ErrorInsufficientResources;
        }

        pmem_fd = drv_ctx.ip_buf_ion_info[i].fd_ion_data.fd;
#else
        pmem_fd = open (MEM_DEVICE,O_RDWR);

        if (pmem_fd < 0) {
            DEBUG_PRINT_ERROR("open failed for pmem/adsp for input buffer");
            return OMX_ErrorInsufficientResources;
        }

        if (pmem_fd == 0) {
            pmem_fd = open (MEM_DEVICE,O_RDWR);

            if (pmem_fd < 0) {
                DEBUG_PRINT_ERROR("open failed for pmem/adsp for input buffer");
                return OMX_ErrorInsufficientResources;
            }
        }

        if (!align_pmem_buffers(pmem_fd, drv_ctx.ip_buf.buffer_size,
                    drv_ctx.ip_buf.alignment)) {
            DEBUG_PRINT_ERROR("align_pmem_buffers() failed");
            close(pmem_fd);
            return OMX_ErrorInsufficientResources;
        }

#endif

        if (!secure_mode) {
            buf_addr = (unsigned char *)mmap(NULL,
                    drv_ctx.ip_buf.buffer_size,
                    PROT_READ|PROT_WRITE, MAP_SHARED, pmem_fd, 0);

            if (buf_addr == MAP_FAILED) {
                close(pmem_fd);
#ifdef USE_ION
                free_ion_memory(&drv_ctx.ip_buf_ion_info[i]);
#endif
                DEBUG_PRINT_ERROR("Map Failed to allocate input buffer");
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

        setbuffers.buffer_type = VDEC_BUFFER_TYPE_INPUT;
        memcpy (&setbuffers.buffer,&drv_ctx.ptr_inputbuffer [i],
                sizeof (vdec_bufferpayload));
        ioctl_msg.in  = &setbuffers;
        ioctl_msg.out = NULL;

        if (ioctl (drv_ctx.video_driver_fd,VDEC_IOCTL_SET_BUFFER,
                    &ioctl_msg) < 0) {
            DEBUG_PRINT_ERROR("Set Buffers Failed");
            return OMX_ErrorInsufficientResources;
        }

        input = *bufferHdr;
        BITMASK_SET(&m_inp_bm_count,i);
        DEBUG_PRINT_LOW("Buffer address %p of pmem",*bufferHdr);

        if (secure_mode)
            input->pBuffer = (OMX_U8 *)drv_ctx.ptr_inputbuffer [i].pmem_fd;
        else
            input->pBuffer           = (OMX_U8 *)buf_addr;

        input->nSize             = sizeof(OMX_BUFFERHEADERTYPE);
        input->nVersion.nVersion = OMX_SPEC_VERSION;
        input->nAllocLen         = drv_ctx.ip_buf.buffer_size - DEVICE_SCRATCH;
        input->pAppPrivate       = appData;
        input->nInputPortIndex   = OMX_CORE_INPUT_PORT_INDEX;
        input->pInputPortPrivate = (void *)&drv_ctx.ptr_inputbuffer [i];

        if (drv_ctx.disable_dmx) {
            eRet = allocate_desc_buffer(i);
        }
    } else {
        DEBUG_PRINT_ERROR("ERROR:Input Buffer Index not found");
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

    int nBufHdrSize        = 0;
    int nPlatformEntrySize = 0;
    int nPlatformListSize  = 0;
    int nPMEMInfoSize = 0;
    int pmem_fd = -1;
    unsigned char *pmem_baseaddress = NULL;

    OMX_QCOM_PLATFORM_PRIVATE_LIST      *pPlatformList;
    OMX_QCOM_PLATFORM_PRIVATE_ENTRY     *pPlatformEntry;
    OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO *pPMEMInfo;

    if (!m_out_mem_ptr) {
        DEBUG_PRINT_HIGH("Allocate o/p buffer Header: Cnt(%d) Sz(%d)",
                drv_ctx.op_buf.actualcount,
                drv_ctx.op_buf.buffer_size);

        DEBUG_PRINT_LOW("Allocating First Output Buffer(%d)",
                drv_ctx.op_buf.actualcount);

        nBufHdrSize        = drv_ctx.op_buf.actualcount *
            sizeof(OMX_BUFFERHEADERTYPE);

        nPMEMInfoSize      = drv_ctx.op_buf.actualcount *
            sizeof(OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO);
        nPlatformListSize  = drv_ctx.op_buf.actualcount *
            sizeof(OMX_QCOM_PLATFORM_PRIVATE_LIST);
        nPlatformEntrySize = drv_ctx.op_buf.actualcount *
            sizeof(OMX_QCOM_PLATFORM_PRIVATE_ENTRY);

        DEBUG_PRINT_LOW("TotalBufHdr %d BufHdrSize %d PMEM %d PL %d",nBufHdrSize,
                sizeof(OMX_BUFFERHEADERTYPE),
                nPMEMInfoSize,
                nPlatformListSize);
        DEBUG_PRINT_LOW("PE %d OutputBuffer Count %d",nPlatformEntrySize,
                drv_ctx.op_buf.actualcount);

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
#ifdef _ANDROID_
        m_heap_ptr = (struct vidc_heap *)\
                     calloc (sizeof(struct vidc_heap),
                             drv_ctx.op_buf.actualcount);
#endif

        if (m_out_mem_ptr && pPtr && drv_ctx.ptr_outputbuffer
                && drv_ctx.ptr_respbuffer
#ifdef _ANDROID_
                && m_heap_ptr
#endif
           ) {
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

            DEBUG_PRINT_LOW("Memory Allocation Succeeded for OUT port%p",m_out_mem_ptr);

            // Settting the entire storage nicely
            DEBUG_PRINT_LOW("bHdr %p OutMem %p PE %p",bufHdr, m_out_mem_ptr,pPlatformEntry);
            DEBUG_PRINT_LOW("Pmem Info = %p",pPMEMInfo);

            for (i=0; i < drv_ctx.op_buf.actualcount ; i++) {
                bufHdr->nSize              = sizeof(OMX_BUFFERHEADERTYPE);
                bufHdr->nVersion.nVersion  = OMX_SPEC_VERSION;
                // Set the values when we determine the right HxW param
                bufHdr->nAllocLen          = 0;
                bufHdr->nFilledLen         = 0;
                bufHdr->pAppPrivate        = NULL;
                bufHdr->nOutputPortIndex   = OMX_CORE_OUTPUT_PORT_INDEX;
                // Platform specific PMEM Information
                // Initialize the Platform Entry
                //DEBUG_PRINT_LOW("Initializing the Platform Entry for %d",i);
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
                drv_ctx.ptr_respbuffer[i].client_data = (void *)\
                                                        &drv_ctx.ptr_outputbuffer[i];
#ifdef _ANDROID_
                m_heap_ptr[i].video_heap_ptr = NULL;
#endif
                // Move the buffer and buffer header pointers
                bufHdr++;
                pPMEMInfo++;
                pPlatformEntry++;
                pPlatformList++;
            }

#ifdef MAX_RES_1080P

            if (eRet == OMX_ErrorNone && secure_mode) {
                eRet = vdec_alloc_meta_buffers();

                if (eRet) {
                    DEBUG_PRINT_ERROR("ERROR in allocating meta buffers");
                    return OMX_ErrorInsufficientResources;
                }
            }

            if (eRet == OMX_ErrorNone && drv_ctx.decoder_format == VDEC_CODECTYPE_H264) {
                //Allocate the h264_mv_buffer
                eRet = vdec_alloc_h264_mv();

                if (eRet) {
                    DEBUG_PRINT_ERROR("ERROR in allocating MV buffers");
                    return OMX_ErrorInsufficientResources;
                }
            }

#endif
        } else {
            DEBUG_PRINT_ERROR("Output buf mem alloc failed[0x%x][0x%x]",\
                    m_out_mem_ptr, pPtr);

            if (m_out_mem_ptr) {
                free(m_out_mem_ptr);
                m_out_mem_ptr = NULL;
            }

            if (pPtr) {
                free(pPtr);
                pPtr = NULL;
            }

            if (drv_ctx.ptr_outputbuffer) {
                free(drv_ctx.ptr_outputbuffer);
                drv_ctx.ptr_outputbuffer = NULL;
            }

            if (drv_ctx.ptr_respbuffer) {
                free(drv_ctx.ptr_respbuffer);
                drv_ctx.ptr_respbuffer = NULL;
            }

#ifdef USE_ION

            if (drv_ctx.op_buf_ion_info) {
                DEBUG_PRINT_LOW("Free o/p ion context");
                free(drv_ctx.op_buf_ion_info);
                drv_ctx.op_buf_ion_info = NULL;
            }

#endif
            eRet =  OMX_ErrorInsufficientResources;
        }
    }

    for (i=0; i< drv_ctx.op_buf.actualcount; i++) {
        if (BITMASK_ABSENT(&m_out_bm_count,i)) {
            DEBUG_PRINT_LOW("Found a Free Output Buffer Index %d",i);
            break;
        }
    }

    if (i < drv_ctx.op_buf.actualcount) {
        DEBUG_PRINT_LOW("Allocate Output Buffer");

#ifdef USE_ION
        drv_ctx.op_buf_ion_info[i].ion_device_fd = alloc_map_ion_memory(
                drv_ctx.op_buf.buffer_size,drv_ctx.op_buf.alignment,
                &drv_ctx.op_buf_ion_info[i].ion_alloc_data,
                &drv_ctx.op_buf_ion_info[i].fd_ion_data, ION_FLAG_CACHED);

        if (drv_ctx.op_buf_ion_info[i].ion_device_fd < 0) {
            return OMX_ErrorInsufficientResources;
        }

        pmem_fd = drv_ctx.op_buf_ion_info[i].fd_ion_data.fd;
#else
        pmem_fd = open (MEM_DEVICE,O_RDWR);

        if (pmem_fd < 0) {
            DEBUG_PRINT_ERROR("ERROR:pmem fd for output buffer %d",
                    drv_ctx.op_buf.buffer_size);
            return OMX_ErrorInsufficientResources;
        }

        if (pmem_fd == 0) {
            pmem_fd = open (MEM_DEVICE,O_RDWR);

            if (pmem_fd < 0) {
                DEBUG_PRINT_ERROR("ERROR:pmem fd for output buffer %d",
                        drv_ctx.op_buf.buffer_size);
                return OMX_ErrorInsufficientResources;
            }
        }

        if (!align_pmem_buffers(pmem_fd, drv_ctx.op_buf.buffer_size,
                    drv_ctx.op_buf.alignment)) {
            DEBUG_PRINT_ERROR("align_pmem_buffers() failed");
            close(pmem_fd);
            return OMX_ErrorInsufficientResources;
        }

#endif

        if (!secure_mode) {
            pmem_baseaddress = (unsigned char *)mmap(NULL,
                    drv_ctx.op_buf.buffer_size,
                    PROT_READ|PROT_WRITE,MAP_SHARED,pmem_fd,0);

            if (pmem_baseaddress == MAP_FAILED) {
                DEBUG_PRINT_ERROR("MMAP failed for Size %d",
                        drv_ctx.op_buf.buffer_size);
                close(pmem_fd);
#ifdef USE_ION
                free_ion_memory(&drv_ctx.op_buf_ion_info[i]);
#endif
                return OMX_ErrorInsufficientResources;
            }
        }

        *bufferHdr = (m_out_mem_ptr + i);

        if (secure_mode)
            drv_ctx.ptr_outputbuffer [i].bufferaddr = *bufferHdr;
        else
            drv_ctx.ptr_outputbuffer [i].bufferaddr = pmem_baseaddress;

        drv_ctx.ptr_outputbuffer [i].pmem_fd = pmem_fd;
        drv_ctx.ptr_outputbuffer [i].buffer_len = drv_ctx.op_buf.buffer_size;
        drv_ctx.ptr_outputbuffer [i].mmaped_size = drv_ctx.op_buf.buffer_size;
        drv_ctx.ptr_outputbuffer [i].offset = 0;

#ifdef _ANDROID_
#ifdef USE_ION
        m_heap_ptr[i].video_heap_ptr = new VideoHeap (drv_ctx.op_buf_ion_info[i].ion_device_fd,
                drv_ctx.op_buf.buffer_size,
                pmem_baseaddress,
                ion_alloc_data.handle,
                pmem_fd);
        m_heap_count = m_heap_count + 1;
#else
        m_heap_ptr[i].video_heap_ptr = new VideoHeap (pmem_fd,
                drv_ctx.op_buf.buffer_size,
                pmem_baseaddress);
#endif
#endif

        m_pmem_info[i].offset = drv_ctx.ptr_outputbuffer[i].offset;
#ifdef _ANDROID_
        m_pmem_info[i].pmem_fd = (OMX_U32) m_heap_ptr[i].video_heap_ptr.get ();
#else
        m_pmem_info[i].pmem_fd = drv_ctx.ptr_outputbuffer[i].pmem_fd ;
#endif
        setbuffers.buffer_type = VDEC_BUFFER_TYPE_OUTPUT;
        memcpy (&setbuffers.buffer,&drv_ctx.ptr_outputbuffer [i],
                sizeof (vdec_bufferpayload));
        ioctl_msg.in  = &setbuffers;
        ioctl_msg.out = NULL;

        DEBUG_PRINT_LOW("Set the Output Buffer Idx: %d Addr: %x", i, drv_ctx.ptr_outputbuffer[i]);

        if (ioctl (drv_ctx.video_driver_fd,VDEC_IOCTL_SET_BUFFER,
                    &ioctl_msg) < 0) {
            DEBUG_PRINT_ERROR("Set output buffer failed");
            return OMX_ErrorInsufficientResources;
        }

        // found an empty buffer at i
        (*bufferHdr)->nAllocLen = drv_ctx.op_buf.buffer_size;
        (*bufferHdr)->pBuffer = (OMX_U8*)drv_ctx.ptr_outputbuffer[i].bufferaddr;
        (*bufferHdr)->pAppPrivate = appData;
        BITMASK_SET(&m_out_bm_count,i);

    } else {
        DEBUG_PRINT_ERROR("ERROR:Output Buffer Index not found");
        eRet = OMX_ErrorInsufficientResources;
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

    DEBUG_PRINT_LOW("Allocate buffer on port %d", (int)port);

    if (m_state == OMX_StateInvalid) {
        DEBUG_PRINT_ERROR("Allocate Buf in Invalid State");
        return OMX_ErrorInvalidState;
    }

    if (port == OMX_CORE_INPUT_PORT_INDEX) {
        if (arbitrary_bytes) {
            eRet = allocate_input_heap_buffer (hComp,bufferHdr,port,appData,bytes);
        } else {
            eRet = allocate_input_buffer(hComp,bufferHdr,port,appData,bytes);
        }
    } else if (port == OMX_CORE_OUTPUT_PORT_INDEX) {
        eRet = client_buffers.allocate_buffers_color_convert(hComp,bufferHdr,port,
                appData,bytes);
    } else {
        DEBUG_PRINT_ERROR("Error: Invalid Port Index received %d",(int)port);
        eRet = OMX_ErrorBadPortIndex;
    }

    DEBUG_PRINT_LOW("Checking for Output Allocate buffer Done");

    if (eRet == OMX_ErrorNone) {
        if (allocate_done()) {
            if (BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING)) {
                // Send the callback now
                BITMASK_CLEAR((&m_flags),OMX_COMPONENT_IDLE_PENDING);
                post_event(OMX_CommandStateSet,OMX_StateIdle,
                        OMX_COMPONENT_GENERATE_EVENT);
            }
        }

        if (port == OMX_CORE_INPUT_PORT_INDEX && m_inp_bPopulated) {
            if (BITMASK_PRESENT(&m_flags,OMX_COMPONENT_INPUT_ENABLE_PENDING)) {
                BITMASK_CLEAR((&m_flags),OMX_COMPONENT_INPUT_ENABLE_PENDING);
                post_event(OMX_CommandPortEnable,
                        OMX_CORE_INPUT_PORT_INDEX,
                        OMX_COMPONENT_GENERATE_EVENT);
            }
        }

        if (port == OMX_CORE_OUTPUT_PORT_INDEX && m_out_bPopulated) {
            if (BITMASK_PRESENT(&m_flags,OMX_COMPONENT_OUTPUT_ENABLE_PENDING)) {
                BITMASK_CLEAR((&m_flags),OMX_COMPONENT_OUTPUT_ENABLE_PENDING);
                post_event(OMX_CommandPortEnable,
                        OMX_CORE_OUTPUT_PORT_INDEX,
                        OMX_COMPONENT_GENERATE_EVENT);
            }
        }
    }

    DEBUG_PRINT_LOW("Allocate Buffer exit with ret Code %d",eRet);
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

    DEBUG_PRINT_LOW("In for decoder free_buffer");

    if (m_state == OMX_StateIdle &&
            (BITMASK_PRESENT(&m_flags ,OMX_COMPONENT_LOADING_PENDING))) {
        DEBUG_PRINT_LOW(" free buffer while Component in Loading pending");
    } else if ((m_inp_bEnabled == OMX_FALSE && port == OMX_CORE_INPUT_PORT_INDEX)||
            (m_out_bEnabled == OMX_FALSE && port == OMX_CORE_OUTPUT_PORT_INDEX)) {
        DEBUG_PRINT_LOW("Free Buffer while port %d disabled", port);
    } else if (m_state == OMX_StateExecuting || m_state == OMX_StatePause) {
        DEBUG_PRINT_ERROR("Invalid state to free buffer,ports need to be disabled");
        post_event(OMX_EventError,
                OMX_ErrorPortUnpopulated,
                OMX_COMPONENT_GENERATE_EVENT);

        return OMX_ErrorIncorrectStateOperation;
    } else if (m_state != OMX_StateInvalid) {
        DEBUG_PRINT_ERROR("Invalid state to free buffer,port lost Buffers");
        post_event(OMX_EventError,
                OMX_ErrorPortUnpopulated,
                OMX_COMPONENT_GENERATE_EVENT);
    }

    if (port == OMX_CORE_INPUT_PORT_INDEX) {
        /*Check if arbitrary bytes*/
        if (!arbitrary_bytes && !input_use_buffer)
            nPortIndex = buffer - m_inp_mem_ptr;
        else
            nPortIndex = buffer - m_inp_heap_ptr;

        DEBUG_PRINT_LOW("free_buffer on i/p port - Port idx %d", nPortIndex);

        if (nPortIndex < drv_ctx.ip_buf.actualcount) {
            // Clear the bit associated with it.
            BITMASK_CLEAR(&m_inp_bm_count,nPortIndex);
            BITMASK_CLEAR(&m_heap_inp_bm_count,nPortIndex);

            if (input_use_buffer == true) {

                DEBUG_PRINT_LOW("Free pmem Buffer index %d",nPortIndex);

                if (m_phdr_pmem_ptr)
                    free_input_buffer(m_phdr_pmem_ptr[nPortIndex]);
            } else {
                if (arbitrary_bytes) {
                    if (m_phdr_pmem_ptr)
                        free_input_buffer(nPortIndex,m_phdr_pmem_ptr[nPortIndex]);
                    else
                        free_input_buffer(nPortIndex,NULL);
                } else
                    free_input_buffer(buffer);
            }

            m_inp_bPopulated = OMX_FALSE;

            /*Free the Buffer Header*/
            if (release_input_done()) {
                DEBUG_PRINT_HIGH("ALL input buffers are freed/released");
                free_input_buffer_header();
            }
        } else {
            DEBUG_PRINT_ERROR("Error: free_buffer ,Port Index Invalid");
            eRet = OMX_ErrorBadPortIndex;
        }

        if (BITMASK_PRESENT((&m_flags),OMX_COMPONENT_INPUT_DISABLE_PENDING)
                && release_input_done()) {
            DEBUG_PRINT_LOW("MOVING TO DISABLED STATE");
            BITMASK_CLEAR((&m_flags),OMX_COMPONENT_INPUT_DISABLE_PENDING);
            post_event(OMX_CommandPortDisable,
                    OMX_CORE_INPUT_PORT_INDEX,
                    OMX_COMPONENT_GENERATE_EVENT);
        }
    } else if (port == OMX_CORE_OUTPUT_PORT_INDEX) {
        // check if the buffer is valid
        nPortIndex = buffer - client_buffers.get_il_buf_hdr();

        if (nPortIndex < drv_ctx.op_buf.actualcount) {
            DEBUG_PRINT_LOW("free_buffer on o/p port - Port idx %d", nPortIndex);
            // Clear the bit associated with it.
            BITMASK_CLEAR(&m_out_bm_count,nPortIndex);
            m_out_bPopulated = OMX_FALSE;
            client_buffers.free_output_buffer (buffer);

            if (release_output_done()) {
                free_output_buffer_header();
            }
        } else {
            DEBUG_PRINT_ERROR("Error: free_buffer , Port Index Invalid");
            eRet = OMX_ErrorBadPortIndex;
        }

        if (BITMASK_PRESENT((&m_flags),OMX_COMPONENT_OUTPUT_DISABLE_PENDING)
                && release_output_done()) {
            DEBUG_PRINT_LOW("FreeBuffer : If any Disable event pending,post it");

            DEBUG_PRINT_LOW("MOVING TO DISABLED STATE");
            BITMASK_CLEAR((&m_flags),OMX_COMPONENT_OUTPUT_DISABLE_PENDING);
#ifdef _ANDROID_ICS_

            if (m_enable_android_native_buffers) {
                DEBUG_PRINT_LOW("FreeBuffer - outport disabled: reset native buffers");
                memset(&native_buffer, 0 ,(sizeof(struct nativebuffer) * MAX_NUM_INPUT_OUTPUT_BUFFERS));
            }

#endif

            post_event(OMX_CommandPortDisable,
                    OMX_CORE_OUTPUT_PORT_INDEX,
                    OMX_COMPONENT_GENERATE_EVENT);
        }
    } else {
        eRet = OMX_ErrorBadPortIndex;
    }

    if ((eRet == OMX_ErrorNone) &&
            (BITMASK_PRESENT(&m_flags ,OMX_COMPONENT_LOADING_PENDING))) {
        if (release_done()) {
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

    if (buffer->nFlags & OMX_BUFFERFLAG_CODECCONFIG) {
        codec_config_flag = true;
        DEBUG_PRINT_LOW("%s: codec_config buffer", __FUNCTION__);
    } else {
        codec_config_flag = false;
    }

    if (m_state == OMX_StateInvalid) {
        DEBUG_PRINT_ERROR("Empty this buffer in Invalid State");
        return OMX_ErrorInvalidState;
    }

    if (buffer == NULL) {
        DEBUG_PRINT_ERROR("ERROR:ETB Buffer is NULL");
        return OMX_ErrorBadParameter;
    }

    if (!m_inp_bEnabled) {
        DEBUG_PRINT_ERROR("ERROR:ETB incorrect state operation, input port is disabled.");
        return OMX_ErrorIncorrectStateOperation;
    }

    if (buffer->nInputPortIndex != OMX_CORE_INPUT_PORT_INDEX) {
        DEBUG_PRINT_ERROR("ERROR:ETB invalid port in header %d", buffer->nInputPortIndex);
        return OMX_ErrorBadPortIndex;
    }

#ifdef _ANDROID_

    if (iDivXDrmDecrypt) {
        OMX_ERRORTYPE drmErr = iDivXDrmDecrypt->Decrypt(buffer);

        if (drmErr != OMX_ErrorNone) {
            // this error can be ignored
            DEBUG_PRINT_LOW("ERROR:iDivXDrmDecrypt->Decrypt %d", drmErr);
        }
    }

    if (perf_flag) {
        if (!latency) {
            dec_time.stop();
            latency = dec_time.processing_time_us();
            dec_time.start();
        }
    }

#endif //_ANDROID_

    if (arbitrary_bytes) {
        nBufferIndex = buffer - m_inp_heap_ptr;
    } else {
        if (input_use_buffer == true) {
            nBufferIndex = buffer - m_inp_heap_ptr;
            m_inp_mem_ptr[nBufferIndex].nFilledLen = m_inp_heap_ptr[nBufferIndex].nFilledLen;
            m_inp_mem_ptr[nBufferIndex].nTimeStamp = m_inp_heap_ptr[nBufferIndex].nTimeStamp;
            m_inp_mem_ptr[nBufferIndex].nFlags = m_inp_heap_ptr[nBufferIndex].nFlags;
            buffer = &m_inp_mem_ptr[nBufferIndex];
            DEBUG_PRINT_LOW("Non-Arbitrary mode - buffer address is: malloc %p, pmem%p in Index %d, buffer %p of size %d",
                    &m_inp_heap_ptr[nBufferIndex], &m_inp_mem_ptr[nBufferIndex],nBufferIndex, buffer, buffer->nFilledLen);
        } else {
            nBufferIndex = buffer - m_inp_mem_ptr;
        }
    }

    if (nBufferIndex > drv_ctx.ip_buf.actualcount ) {
        DEBUG_PRINT_ERROR("ERROR:ETB nBufferIndex is invalid");
        return OMX_ErrorBadParameter;
    }

    DEBUG_PRINT_LOW("[ETB] BHdr(%p) pBuf(%p) nTS(%lld) nFL(%lu)",
            buffer, buffer->pBuffer, buffer->nTimeStamp, buffer->nFilledLen);

    if (arbitrary_bytes) {
        post_event ((unsigned)hComp,(unsigned)buffer,
                OMX_COMPONENT_GENERATE_ETB_ARBITRARY);
    } else {
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
#ifdef MAX_RES_1080P
    bool not_coded_vop = false;
#endif

    /*Should we generate a Aync error event*/
    if (buffer == NULL || buffer->pInputPortPrivate == NULL) {
        DEBUG_PRINT_ERROR("ERROR:empty_this_buffer_proxy is invalid");
        return OMX_ErrorBadParameter;
    }

    nPortIndex = buffer-((OMX_BUFFERHEADERTYPE *)m_inp_mem_ptr);

    if (nPortIndex > drv_ctx.ip_buf.actualcount) {
        DEBUG_PRINT_ERROR("ERROR:empty_this_buffer_proxy invalid nPortIndex[%u]",
                nPortIndex);
        return OMX_ErrorBadParameter;
    }

    pending_input_buffers++;

    /* return zero length and not an EOS buffer */
    if (!arbitrary_bytes && (buffer->nFilledLen == 0) &&
            ((buffer->nFlags & OMX_BUFFERFLAG_EOS) == 0)) {
        DEBUG_PRINT_HIGH("return zero legth buffer");
        post_event ((unsigned int)buffer,VDEC_S_SUCCESS,
                OMX_COMPONENT_GENERATE_EBD);
        return OMX_ErrorNone;
    }

#ifdef MAX_RES_1080P

    if (codec_type_parse == CODEC_TYPE_MPEG4 || codec_type_parse == CODEC_TYPE_DIVX) {
        mp4StreamType psBits;
        psBits.data = (unsigned char *)(buffer->pBuffer + buffer->nOffset);
        psBits.numBytes = buffer->nFilledLen;
        mp4_headerparser.parseHeader(&psBits);
        not_coded_vop = mp4_headerparser.is_notcodec_vop(
                (buffer->pBuffer + buffer->nOffset),buffer->nFilledLen);

        if (not_coded_vop) {
            DEBUG_PRINT_HIGH("Found Not coded vop len %d frame number %d",
                    buffer->nFilledLen,frame_count);

            if (buffer->nFlags & OMX_BUFFERFLAG_EOS) {
                DEBUG_PRINT_HIGH("Eos and Not coded Vop set len to zero");
                not_coded_vop = false;
                buffer->nFilledLen = 0;
            }
        }
    }

#endif

    if (input_flush_progress == true
#ifdef MAX_RES_1080P
            || not_coded_vop
#endif
       ) {
        DEBUG_PRINT_LOW("Flush in progress return buffer ");
        post_event ((unsigned int)buffer,VDEC_S_SUCCESS,
                OMX_COMPONENT_GENERATE_EBD);
        return OMX_ErrorNone;
    }

    temp_buffer = (struct vdec_bufferpayload *)buffer->pInputPortPrivate;

    if ((temp_buffer -  drv_ctx.ptr_inputbuffer) > drv_ctx.ip_buf.actualcount) {
        return OMX_ErrorBadParameter;
    }

    DEBUG_PRINT_LOW("ETBProxy: bufhdr = %p, bufhdr->pBuffer = %p", buffer, buffer->pBuffer);
    /*for use buffer we need to memcpy the data*/
    temp_buffer->buffer_len = buffer->nFilledLen;

    if (input_use_buffer) {
        if (buffer->nFilledLen <= temp_buffer->buffer_len) {
            if (arbitrary_bytes) {
                memcpy (temp_buffer->bufferaddr, (buffer->pBuffer + buffer->nOffset),buffer->nFilledLen);
            } else {
                memcpy (temp_buffer->bufferaddr, (m_inp_heap_ptr[nPortIndex].pBuffer + m_inp_heap_ptr[nPortIndex].nOffset),
                        buffer->nFilledLen);
            }
        } else {
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

    if (drv_ctx.disable_dmx && m_desc_buffer_ptr && m_desc_buffer_ptr[nPortIndex].buf_addr) {
        DEBUG_PRINT_LOW("ETB: dmx enabled");

        if (m_demux_entries == 0) {
            extract_demux_addr_offsets(buffer);
        }

        DEBUG_PRINT_LOW("ETB: handle_demux_data - entries=%d",m_demux_entries);
        handle_demux_data(buffer);
        frameinfo.desc_addr = (OMX_U8 *)m_desc_buffer_ptr[nPortIndex].buf_addr;
        frameinfo.desc_size = m_desc_buffer_ptr[nPortIndex].desc_data_size;
    } else {
        frameinfo.desc_addr = NULL;
        frameinfo.desc_size = 0;
    }

    if (!arbitrary_bytes) {
        frameinfo.flags |= buffer->nFlags;
    }


#ifdef _ANDROID_

    if (m_debug_timestamp) {
        if (arbitrary_bytes) {
            DEBUG_PRINT_LOW("Inserting TIMESTAMP (%lld) into queue", buffer->nTimeStamp);
            m_timestamp_list.insert_ts(buffer->nTimeStamp);
        } else if (!arbitrary_bytes && !(buffer->nFlags & OMX_BUFFERFLAG_CODECCONFIG)) {
            DEBUG_PRINT_LOW("Inserting TIMESTAMP (%lld) into queue", buffer->nTimeStamp);
            m_timestamp_list.insert_ts(buffer->nTimeStamp);
        }
    }

#endif

#ifdef INPUT_BUFFER_LOG

    if (inputBufferFile1) {
        fwrite((const char *)temp_buffer->bufferaddr,
                temp_buffer->buffer_len,1,inputBufferFile1);
    }

#endif

    if (buffer->nFlags & QOMX_VIDEO_BUFFERFLAG_EOSEQ) {
        frameinfo.flags |= QOMX_VIDEO_BUFFERFLAG_EOSEQ;
        buffer->nFlags &= ~QOMX_VIDEO_BUFFERFLAG_EOSEQ;
    }

    if (temp_buffer->buffer_len == 0 || (buffer->nFlags & OMX_BUFFERFLAG_EOS)) {
        DEBUG_PRINT_HIGH("Rxd i/p EOS, Notify Driver that EOS has been reached");
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

    DEBUG_PRINT_LOW("[ETBP] pBuf(%p) nTS(%lld) Sz(%d)",
            frameinfo.bufferaddr, frameinfo.timestamp, frameinfo.datalen);
    ioctl_msg.in = &frameinfo;
    ioctl_msg.out = NULL;

    if (ioctl(drv_ctx.video_driver_fd,VDEC_IOCTL_DECODE_FRAME,
                &ioctl_msg) < 0) {
        /*Generate an async error and move to invalid state*/
        DEBUG_PRINT_ERROR("ERROR:empty_this_buffer_proxy VDEC_IOCTL_DECODE_FRAME failed");

        if (!arbitrary_bytes) {
            DEBUG_PRINT_LOW("Return failed buffer");
            post_event ((unsigned int)buffer,VDEC_S_SUCCESS,
                    OMX_COMPONENT_GENERATE_EBD);
        }

        return OMX_ErrorBadParameter;
    } else
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

    if (m_state == OMX_StateInvalid) {
        DEBUG_PRINT_ERROR("FTB in Invalid State");
        return OMX_ErrorInvalidState;
    }

    if (!m_out_bEnabled) {
        DEBUG_PRINT_ERROR("ERROR:FTB incorrect state operation, output port is disabled.");
        return OMX_ErrorIncorrectStateOperation;
    }

    if (buffer == NULL ||
            ((buffer - client_buffers.get_il_buf_hdr()) >= drv_ctx.op_buf.actualcount)) {
        return OMX_ErrorBadParameter;
    }

    if (buffer->nOutputPortIndex != OMX_CORE_OUTPUT_PORT_INDEX) {
        DEBUG_PRINT_ERROR("ERROR:FTB invalid port in header %d", buffer->nOutputPortIndex);
        return OMX_ErrorBadPortIndex;
    }

    DEBUG_PRINT_LOW("[FTB] bufhdr = %p, bufhdr->pBuffer = %p", buffer, buffer->pBuffer);
    post_event((unsigned) hComp, (unsigned)buffer,m_fill_output_msg);
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
    struct vdec_fillbuffer_cmd fillbuffer;
    struct vdec_bufferpayload     *ptr_outputbuffer = NULL;
    struct vdec_output_frameinfo  *ptr_respbuffer = NULL;


    if (bufferAdd == NULL || ((buffer - client_buffers.get_il_buf_hdr()) >
                drv_ctx.op_buf.actualcount) )
        return OMX_ErrorBadParameter;

    DEBUG_PRINT_LOW("FTBProxy: bufhdr = %p, bufhdr->pBuffer = %p",
            bufferAdd, bufferAdd->pBuffer);

    /*Return back the output buffer to client*/
    if (m_out_bEnabled != OMX_TRUE || output_flush_progress == true) {
        DEBUG_PRINT_LOW("Output Buffers return flush/disable condition");
        buffer->nFilledLen = 0;
        m_cb.FillBufferDone (hComp,m_app_data,buffer);
        return OMX_ErrorNone;
    }

    pending_output_buffers++;
    buffer = client_buffers.get_dr_buf_hdr(bufferAdd);
    ptr_respbuffer = (struct vdec_output_frameinfo*)buffer->pOutputPortPrivate;

    if (ptr_respbuffer) {
        ptr_outputbuffer =  (struct vdec_bufferpayload*)ptr_respbuffer->client_data;
    }

    if (ptr_respbuffer == NULL || ptr_outputbuffer == NULL) {
        DEBUG_PRINT_ERROR("resp buffer or outputbuffer is NULL");
        buffer->nFilledLen = 0;
        m_cb.FillBufferDone (hComp,m_app_data,buffer);
        pending_output_buffers--;
        return OMX_ErrorBadParameter;
    }

    memcpy (&fillbuffer.buffer,ptr_outputbuffer,\
            sizeof(struct vdec_bufferpayload));
    fillbuffer.client_data = buffer;

#ifdef _ANDROID_ICS_

    if (m_enable_android_native_buffers) {
        // Acquire a write lock on this buffer.
        if (GENLOCK_NO_ERROR != genlock_lock_buffer(native_buffer[buffer - m_out_mem_ptr].nativehandle,
                    GENLOCK_WRITE_LOCK, GENLOCK_MAX_TIMEOUT)) {
            DEBUG_PRINT_ERROR("Failed to acquire genlock");
            buffer->nFilledLen = 0;
            m_cb.FillBufferDone (hComp,m_app_data,buffer);
            pending_output_buffers--;
            return OMX_ErrorInsufficientResources;
        } else {
            native_buffer[buffer - m_out_mem_ptr].inuse = true;
        }
    }

#endif

    ioctl_msg.in = &fillbuffer;
    ioctl_msg.out = NULL;

    if (ioctl (drv_ctx.video_driver_fd,
                VDEC_IOCTL_FILL_OUTPUT_BUFFER,&ioctl_msg) < 0) {
        DEBUG_PRINT_ERROR("Decoder frame failed");
#ifdef _ANDROID_ICS_

        if (m_enable_android_native_buffers) {
            // Unlock the buffer
            if (GENLOCK_NO_ERROR != genlock_unlock_buffer(native_buffer[buffer - m_out_mem_ptr].nativehandle)) {
                DEBUG_PRINT_ERROR("Releasing genlock failed");
                return OMX_ErrorInsufficientResources;
            } else {
                native_buffer[buffer - m_out_mem_ptr].inuse = false;
            }
        }

#endif
        m_cb.FillBufferDone (hComp,m_app_data,buffer);
        pending_output_buffers--;
        return OMX_ErrorBadParameter;
    }

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
    DEBUG_PRINT_LOW("Callbacks Set %p %p %p",m_cb.EmptyBufferDone,\
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
#ifndef JB_MR1
    sp<IServiceManager> sm;
    sp<hwcService::IHWComposer> hwcBinder = NULL;
#endif
#ifdef _ANDROID_

    if (iDivXDrmDecrypt) {
        delete iDivXDrmDecrypt;
        iDivXDrmDecrypt=NULL;
    }

#endif //_ANDROID_
    int i = 0;

    if (OMX_StateLoaded != m_state) {
        DEBUG_PRINT_ERROR("WARNING:Rxd DeInit,OMX not in LOADED state %d",\
                m_state);
        DEBUG_PRINT_ERROR("Playback Ended - FAILED");
    } else {
        DEBUG_PRINT_HIGH("Playback Ended - PASSED");
    }

#ifndef JB_MR1

    if (secure_mode) {
        sm = defaultServiceManager();
        hwcBinder =
            interface_cast<hwcService::IHWComposer>(sm->getService(String16("display.hwcservice")));

        if (hwcBinder != NULL) {
            hwcBinder->setCloseSecureStart();
        } else {
            DEBUG_PRINT_HIGH("Failed to get hwcbinder, "
                    "failed to call close secure start");
        }
    }

#endif

    /*Check if the output buffers have to be cleaned up*/
    if (m_out_mem_ptr) {
        DEBUG_PRINT_LOW("Freeing the Output Memory");

        for (i=0; i < drv_ctx.op_buf.actualcount; i++ ) {
            free_output_buffer (&m_out_mem_ptr[i]);
#ifdef _ANDROID_ICS_

            if (m_enable_android_native_buffers) {
                if (native_buffer[i].inuse) {
                    if (GENLOCK_NO_ERROR != genlock_unlock_buffer(native_buffer[i].nativehandle)) {
                        DEBUG_PRINT_ERROR("Unlocking genlock failed");
                    }

                    native_buffer[i].inuse = false;
                }
            }

#endif
        }

#ifdef _ANDROID_ICS_
        memset(&native_buffer, 0, (sizeof(nativebuffer) * MAX_NUM_INPUT_OUTPUT_BUFFERS));
#endif
    }

    /*Check if the input buffers have to be cleaned up*/
    if (m_inp_mem_ptr || m_inp_heap_ptr) {
        DEBUG_PRINT_LOW("Freeing the Input Memory");

        for (i=0; i<drv_ctx.ip_buf.actualcount; i++ ) {
            if (m_inp_mem_ptr)
                free_input_buffer (i,&m_inp_mem_ptr[i]);
            else
                free_input_buffer (i,NULL);
        }
    }

    free_input_buffer_header();
    free_output_buffer_header();

    if (h264_scratch.pBuffer) {
        free(h264_scratch.pBuffer);
        h264_scratch.pBuffer = NULL;
    }

    if (h264_parser) {
        delete h264_parser;
        h264_parser = NULL;
    }

    if (m_platform_list) {
        free(m_platform_list);
        m_platform_list = NULL;
    }

    if (m_vendor_config.pData) {
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

    if (m_debug_timestamp) {
        m_timestamp_list.reset_ts_list();
    }

#endif

    DEBUG_PRINT_LOW("Calling VDEC_IOCTL_STOP_NEXT_MSG");
    (void)ioctl(drv_ctx.video_driver_fd, VDEC_IOCTL_STOP_NEXT_MSG,
            NULL);
#ifdef _ANDROID_

    /* get strong count gets the refernce count of the pmem, the count will
     * be incremented by our kernal driver and surface flinger, by the time
     * we close the pmem, this cound needs to be zero, but there is no way
     * for us to know when surface flinger reduces its cound, so we wait
     * here in a infinite loop till the count is zero
     */
    if (m_heap_ptr) {
        for (int indx = 0; indx < drv_ctx.op_buf.actualcount; indx++)
            m_heap_ptr[indx].video_heap_ptr = NULL;

        free(m_heap_ptr);
        m_heap_ptr = NULL;
        m_heap_count = 0;
    }

#endif // _ANDROID_
#ifdef INPUT_BUFFER_LOG
    fclose (inputBufferFile1);
#endif
#ifdef OUTPUT_BUFFER_LOG
    fclose (outputBufferFile1);
#endif
#ifdef OUTPUT_EXTRADATA_LOG
    fclose (outputExtradataFile);
#endif
    power_module_deregister();
    DEBUG_PRINT_HIGH("omx_vdec::component_deinit() complete");
#ifndef JB_MR1

    if (secure_mode) {
        if (hwcBinder != NULL) {
            hwcBinder->setCloseSecureEnd();
        } else {
            DEBUG_PRINT_HIGH("Failed to get hwcbinder, "
                    "failed to call close secure start");
        }
    }

#endif
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
    DEBUG_PRINT_HIGH("use EGL image support for decoder");

    if (!bufferHdr || !eglImage|| port != OMX_CORE_OUTPUT_PORT_INDEX) {
        DEBUG_PRINT_ERROR("");
    }

#ifdef USE_EGL_IMAGE_GPU

    if (m_display_id == NULL) {
        DEBUG_PRINT_ERROR("Display ID is not set by IL client");
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

    if (temp_egl_id != NULL) {
        fd = temp_egl_id->pmem_fd;
        offset = temp_egl_id->offset;
    }

#endif

    if (fd < 0) {
        DEBUG_PRINT_ERROR("Improper pmem fd by EGL client %d",fd);
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
        DEBUG_PRINT_ERROR("use buffer call failed for egl image");
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

    if (!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.mpeg4",OMX_MAX_STRINGNAME_SIZE)) {
        if ((0 == index) && role) {
            strlcpy((char *)role, "video_decoder.mpeg4",OMX_MAX_STRINGNAME_SIZE);
            DEBUG_PRINT_LOW("component_role_enum: role %s",role);
        } else {
            eRet = OMX_ErrorNoMore;
        }
    }

    if (!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.mpeg2",OMX_MAX_STRINGNAME_SIZE)) {
        if ((0 == index) && role) {
            strlcpy((char *)role, "video_decoder.mpeg2",OMX_MAX_STRINGNAME_SIZE);
            DEBUG_PRINT_LOW("component_role_enum: role %s",role);
        } else {
            eRet = OMX_ErrorNoMore;
        }
    } else if (!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.h263",OMX_MAX_STRINGNAME_SIZE)) {
        if ((0 == index) && role) {
            strlcpy((char *)role, "video_decoder.h263",OMX_MAX_STRINGNAME_SIZE);
            DEBUG_PRINT_LOW("component_role_enum: role %s",role);
        } else {
            DEBUG_PRINT_LOW("No more roles");
            eRet = OMX_ErrorNoMore;
        }
    }

#ifdef MAX_RES_1080P
    else if ((!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.divx",OMX_MAX_STRINGNAME_SIZE)) ||
            (!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.divx311",OMX_MAX_STRINGNAME_SIZE))
            )
#else
    else if (!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.divx",OMX_MAX_STRINGNAME_SIZE))
#endif
    {

        if ((0 == index) && role) {
            strlcpy((char *)role, "video_decoder.divx",OMX_MAX_STRINGNAME_SIZE);
            DEBUG_PRINT_LOW("component_role_enum: role %s",role);
        } else {
            DEBUG_PRINT_LOW("No more roles");
            eRet = OMX_ErrorNoMore;
        }
    } else if (!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.avc",OMX_MAX_STRINGNAME_SIZE)) {
        if ((0 == index) && role) {
            strlcpy((char *)role, "video_decoder.avc",OMX_MAX_STRINGNAME_SIZE);
            DEBUG_PRINT_LOW("component_role_enum: role %s",role);
        } else {
            DEBUG_PRINT_LOW("No more roles");
            eRet = OMX_ErrorNoMore;
        }
    } else if ( (!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.vc1",OMX_MAX_STRINGNAME_SIZE)) ||
            (!strncmp(drv_ctx.kind, "OMX.qcom.video.decoder.wmv",OMX_MAX_STRINGNAME_SIZE))
            ) {
        if ((0 == index) && role) {
            strlcpy((char *)role, "video_decoder.vc1",OMX_MAX_STRINGNAME_SIZE);
            DEBUG_PRINT_LOW("component_role_enum: role %s",role);
        } else {
            DEBUG_PRINT_LOW("No more roles");
            eRet = OMX_ErrorNoMore;
        }
    } else {
        DEBUG_PRINT_ERROR("ERROR:Querying Role on Unknown Component");
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

    if (bRet_In && bRet_Out) {
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

    if (m_inp_mem_ptr == NULL) {
        return bRet;
    }

    if (m_inp_mem_ptr ) {
        for (; i<drv_ctx.ip_buf.actualcount; i++) {
            if (BITMASK_ABSENT(&m_inp_bm_count,i)) {
                break;
            }
        }
    }

    if (i == drv_ctx.ip_buf.actualcount) {
        bRet = true;
        DEBUG_PRINT_HIGH("Allocate done for all i/p buffers");
    }

    if (i==drv_ctx.ip_buf.actualcount && m_inp_bEnabled) {
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

    if (m_out_mem_ptr == NULL) {
        return bRet;
    }

    if (m_out_mem_ptr) {
        for (; j < drv_ctx.op_buf.actualcount; j++) {
            if (BITMASK_ABSENT(&m_out_bm_count,j)) {
                break;
            }
        }
    }

    if (j == drv_ctx.op_buf.actualcount) {
        bRet = true;
        DEBUG_PRINT_HIGH("Allocate done for all o/p buffers");

        if (m_out_bEnabled)
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

    if (release_input_done()) {
        if (release_output_done()) {
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

    DEBUG_PRINT_LOW("Value of m_out_mem_ptr %p",m_inp_mem_ptr);

    if (m_out_mem_ptr) {
        for (; j < drv_ctx.op_buf.actualcount ; j++) {
            if (BITMASK_PRESENT(&m_out_bm_count,j)) {
                break;
            }
        }

        if (j == drv_ctx.op_buf.actualcount) {
            m_out_bm_count = 0;
            bRet = true;
        }
    } else {
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

    DEBUG_PRINT_LOW("Value of m_inp_mem_ptr %p",m_inp_mem_ptr);

    if (m_inp_mem_ptr) {
        for (; j<drv_ctx.ip_buf.actualcount; j++) {
            if ( BITMASK_PRESENT(&m_inp_bm_count,j)) {
                break;
            }
        }

        if (j==drv_ctx.ip_buf.actualcount) {
            bRet = true;
        }
    } else {
        bRet = true;
    }

    return bRet;
}

OMX_ERRORTYPE omx_vdec::fill_buffer_done(OMX_HANDLETYPE hComp,
        OMX_BUFFERHEADERTYPE * buffer)
{
    OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO *pPMEMInfo = NULL;

    if (!buffer || (buffer - m_out_mem_ptr) >= drv_ctx.op_buf.actualcount) {
        DEBUG_PRINT_ERROR("[FBD] ERROR in ptr(%p)", buffer);
        return OMX_ErrorBadParameter;
    } else if (output_flush_progress) {
        DEBUG_PRINT_LOW("FBD: Buffer (%p) flushed", buffer);
        buffer->nFilledLen = 0;
        buffer->nTimeStamp = 0;
        buffer->nFlags &= ~OMX_BUFFERFLAG_EXTRADATA;
        buffer->nFlags &= ~QOMX_VIDEO_BUFFERFLAG_EOSEQ;
        buffer->nFlags &= ~OMX_BUFFERFLAG_DATACORRUPT;
    }

#ifdef _ANDROID_
    char value[PROPERTY_VALUE_MAX];
    property_get("vidc.dec.debug.panframedata", value, NULL);

    if (atoi(value)) {
        if (buffer->nFlags & QOMX_VIDEO_BUFFERFLAG_EOSEQ) {
            DEBUG_PRINT_HIGH("");
            DEBUG_PRINT_HIGH("***************************************************");
            DEBUG_PRINT_HIGH("FillBufferDone: End Of Sequence Received");
            DEBUG_PRINT_HIGH("***************************************************");
        }

        if (buffer->nFlags & OMX_BUFFERFLAG_DATACORRUPT) {
            DEBUG_PRINT_HIGH("");
            DEBUG_PRINT_HIGH("***************************************************");
            DEBUG_PRINT_HIGH("FillBufferDone: OMX_BUFFERFLAG_DATACORRUPT Received");
            DEBUG_PRINT_HIGH("***************************************************");
        }
    }

#endif

    DEBUG_PRINT_LOW("fill_buffer_done: bufhdr = %p, bufhdr->pBuffer = %p",
            buffer, buffer->pBuffer);
    pending_output_buffers --;

    if (buffer->nFlags & OMX_BUFFERFLAG_EOS) {
        DEBUG_PRINT_HIGH("Output EOS has been reached");

        if (!output_flush_progress)
            post_event(NULL,NULL,OMX_COMPONENT_GENERATE_EOS_DONE);

        if (psource_frame) {
            m_cb.EmptyBufferDone(&m_cmp, m_app_data, psource_frame);
            psource_frame = NULL;
        }

        if (pdest_frame) {
            pdest_frame->nFilledLen = 0;
            m_input_free_q.insert_entry((unsigned) pdest_frame,NULL,NULL);
            pdest_frame = NULL;
        }
    }

    DEBUG_PRINT_LOW("In fill Buffer done call address %p ",buffer);
#ifdef OUTPUT_BUFFER_LOG

    if (outputBufferFile1) {
        OMX_U32 index = buffer - m_out_mem_ptr;
        OMX_U8* pBuffer = (OMX_U8 *)drv_ctx.ptr_outputbuffer[index].bufferaddr;

        fwrite (pBuffer,1,buffer->nFilledLen,
                outputBufferFile1);
    }

#endif

    /* For use buffer we need to copy the data */
    if (!output_flush_progress) {
        time_stamp_dts.get_next_timestamp(buffer,
                (drv_ctx.interlace != VDEC_InterlaceFrameProgressive)
                ?true:false);
    }

    if (m_cb.FillBufferDone) {
        if (buffer->nFilledLen > 0) {
            if (client_extradata) {
                if (secure_mode)
                    handle_extradata_secure(buffer);
                else
                    handle_extradata(buffer);
            }

            if (client_extradata & OMX_TIMEINFO_EXTRADATA)
                // Keep min timestamp interval to handle corrupted bit stream scenario
                set_frame_rate(buffer->nTimeStamp);
            else if (arbitrary_bytes)
                adjust_timestamp(buffer->nTimeStamp);

#ifdef _ANDROID_

            if (perf_flag) {
                if (!proc_frms) {
                    dec_time.stop();
                    latency = dec_time.processing_time_us() - latency;
                    DEBUG_PRINT_HIGH(">>> FBD Metrics: Latency(%.2f)mS", latency / 1e3);
                    dec_time.start();
                    fps_metrics.start();
                }

                proc_frms++;

                if (buffer->nFlags & OMX_BUFFERFLAG_EOS) {
                    OMX_U64 proc_time = 0;
                    fps_metrics.stop();
                    proc_time = fps_metrics.processing_time_us();
                    DEBUG_PRINT_HIGH(">>> FBD Metrics: proc_frms(%lu) proc_time(%.2f)S fps(%.2f)",
                            proc_frms, (float)proc_time / 1e6,
                            (float)(1e6 * proc_frms) / proc_time);
                    proc_frms = 0;
                }
            }

#endif //_ANDROID_

#ifdef OUTPUT_EXTRADATA_LOG

            if (outputExtradataFile) {

                OMX_U32 index = buffer - m_out_mem_ptr;
                OMX_U8* pBuffer = (OMX_U8 *)drv_ctx.ptr_outputbuffer[index].bufferaddr;

                OMX_OTHER_EXTRADATATYPE *p_extra = NULL;
                p_extra = (OMX_OTHER_EXTRADATATYPE *)
                    ((unsigned)(pBuffer + buffer->nOffset +
                        buffer->nFilledLen + 3)&(~3));

                while (p_extra &&
                        (OMX_U8*)p_extra < (pBuffer + buffer->nAllocLen) ) {
                    DEBUG_PRINT_LOW("WRITING extradata, size=%d,type=%d",p_extra->nSize, p_extra->eType);
                    fwrite (p_extra,1,p_extra->nSize,outputExtradataFile);

                    if (p_extra->eType == OMX_ExtraDataNone) {
                        break;
                    }

                    p_extra = (OMX_OTHER_EXTRADATATYPE *) (((OMX_U8 *) p_extra) + p_extra->nSize);
                }
            }

#endif
        }

        if (buffer->nFlags & OMX_BUFFERFLAG_EOS) {
            prev_ts = LLONG_MAX;
            rst_prev_ts = true;
        }

        pPMEMInfo = (OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO *)
            ((OMX_QCOM_PLATFORM_PRIVATE_LIST *)
             buffer->pPlatformPrivate)->entryList->entry;
        DEBUG_PRINT_LOW("Before FBD callback Accessed Pmeminfo %d",pPMEMInfo->pmem_fd);
#ifdef _ANDROID_ICS_

        if (m_enable_android_native_buffers) {
            if (native_buffer[buffer - m_out_mem_ptr].inuse) {
                if (GENLOCK_NO_ERROR != genlock_unlock_buffer(native_buffer[buffer - m_out_mem_ptr].nativehandle)) {
                    DEBUG_PRINT_ERROR("Unlocking genlock failed");
                    return OMX_ErrorInsufficientResources;
                } else {
                    native_buffer[buffer - m_out_mem_ptr].inuse = false;
                }
            }
        }

#endif
        OMX_BUFFERHEADERTYPE *il_buffer;
        il_buffer = client_buffers.get_il_buf_hdr(buffer);

        if (il_buffer)
            m_cb.FillBufferDone (hComp,m_app_data,il_buffer);
        else {
            DEBUG_PRINT_ERROR("Invalid buffer address from get_il_buf_hdr");
            return OMX_ErrorBadParameter;
        }

        DEBUG_PRINT_LOW("After Fill Buffer Done callback %d",pPMEMInfo->pmem_fd);
    } else {
        return OMX_ErrorBadParameter;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE omx_vdec::empty_buffer_done(OMX_HANDLETYPE         hComp,
        OMX_BUFFERHEADERTYPE* buffer)
{

    if (buffer == NULL || ((buffer - m_inp_mem_ptr) > drv_ctx.ip_buf.actualcount)) {
        DEBUG_PRINT_ERROR("empty_buffer_done: ERROR bufhdr = %p", buffer);
        return OMX_ErrorBadParameter;
    }

    DEBUG_PRINT_LOW("empty_buffer_done: bufhdr = %p, bufhdr->pBuffer = %p",
            buffer, buffer->pBuffer);
    pending_input_buffers--;

    if (arbitrary_bytes) {
        if (pdest_frame == NULL && input_flush_progress == false) {
            DEBUG_PRINT_LOW("Push input from buffer done address of Buffer %p",buffer);
            pdest_frame = buffer;
            buffer->nFilledLen = 0;
            buffer->nTimeStamp = LLONG_MAX;
            push_input_buffer (hComp);
        } else {
            DEBUG_PRINT_LOW("Push buffer into freeq address of Buffer %p",buffer);
            buffer->nFilledLen = 0;

            if (!m_input_free_q.insert_entry((unsigned)buffer,NULL,NULL)) {
                DEBUG_PRINT_ERROR("ERROR:i/p free Queue is FULL Error");
            }
        }
    } else if (m_cb.EmptyBufferDone) {
        buffer->nFilledLen = 0;

        if (input_use_buffer == true) {
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
    struct vdec_output_frameinfo *output_respbuf = NULL;

    if (context == NULL || message == NULL) {
        DEBUG_PRINT_ERROR("FATAL ERROR in omx_vdec::async_message_process NULL Check");
        return -1;
    }

    vdec_msg = (struct vdec_msginfo *)message;

    omx = reinterpret_cast<omx_vdec*>(context);

#ifdef _ANDROID_

    if (omx->m_debug_timestamp) {
        if ( (vdec_msg->msgcode == VDEC_MSG_RESP_OUTPUT_BUFFER_DONE) &&
                !(omx->output_flush_progress) ) {
            OMX_TICKS expected_ts = 0;
            omx->m_timestamp_list.pop_min_ts(expected_ts);
            DEBUG_PRINT_LOW("Current timestamp (%lld),Popped TIMESTAMP (%lld) from list",
                    vdec_msg->msgdata.output_frame.time_stamp, expected_ts);

            if (vdec_msg->msgdata.output_frame.time_stamp != expected_ts) {
                DEBUG_PRINT_ERROR("ERROR in omx_vdec::async_message_process timestamp Check");
            }
        }
    }

#endif

    switch (vdec_msg->msgcode) {

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

            omxhdr = (OMX_BUFFERHEADERTYPE* )\
                     vdec_msg->msgdata.input_frame_clientdata;


            if (omxhdr == NULL ||
                    ((omxhdr - omx->m_inp_mem_ptr) > omx->drv_ctx.ip_buf.actualcount) ) {
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
                DEBUG_PRINT_HIGH("Field dropped time stamp is %lld",
                        vdec_msg->msgdata.output_frame.time_stamp);
            }

            break;
        case VDEC_MSG_RESP_OUTPUT_FLUSHED:
        case VDEC_MSG_RESP_OUTPUT_BUFFER_DONE:
            omxhdr = (OMX_BUFFERHEADERTYPE*)vdec_msg->msgdata.output_frame.client_data;
            DEBUG_PRINT_LOW("[RespBufDone] Buf(%p) Ts(%lld) Pic_type(%u)",
                    omxhdr, vdec_msg->msgdata.output_frame.time_stamp,
                    vdec_msg->msgdata.output_frame.pic_type);

            /* update SYNCFRAME flag */
            if (omx->eCompressionFormat == OMX_VIDEO_CodingAVC) {
                /* set SYNCFRAME flag if picture type is IDR for h264 */
                if (vdec_msg->msgdata.output_frame.pic_type == PICTURE_TYPE_IDR)
                    vdec_msg->msgdata.output_frame.flags |= OMX_BUFFERFLAG_SYNCFRAME;
                else
                    vdec_msg->msgdata.output_frame.flags &= ~OMX_BUFFERFLAG_SYNCFRAME;
            } else {
                /* set SYNCFRAME flag if picture type is I_TYPE */
                if (vdec_msg->msgdata.output_frame.pic_type == PICTURE_TYPE_I)
                    vdec_msg->msgdata.output_frame.flags |= OMX_BUFFERFLAG_SYNCFRAME;
                else
                    vdec_msg->msgdata.output_frame.flags &= ~OMX_BUFFERFLAG_SYNCFRAME;
            }

            if (omxhdr && omxhdr->pOutputPortPrivate &&
                    ((omxhdr - omx->m_out_mem_ptr) < omx->drv_ctx.op_buf.actualcount) &&
                    (((struct vdec_output_frameinfo *)omxhdr->pOutputPortPrivate
                      - omx->drv_ctx.ptr_respbuffer) < omx->drv_ctx.op_buf.actualcount)) {
                if (vdec_msg->msgdata.output_frame.len <=  omxhdr->nAllocLen) {
                    omxhdr->nFilledLen = vdec_msg->msgdata.output_frame.len;
                    omxhdr->nOffset = vdec_msg->msgdata.output_frame.offset;
                    omxhdr->nTimeStamp = vdec_msg->msgdata.output_frame.time_stamp;
                    omxhdr->nFlags = (vdec_msg->msgdata.output_frame.flags);

                    output_respbuf = (struct vdec_output_frameinfo *)\
                                     omxhdr->pOutputPortPrivate;
                    output_respbuf->framesize.bottom =
                        vdec_msg->msgdata.output_frame.framesize.bottom;
                    output_respbuf->framesize.left =
                        vdec_msg->msgdata.output_frame.framesize.left;
                    output_respbuf->framesize.right =
                        vdec_msg->msgdata.output_frame.framesize.right;
                    output_respbuf->framesize.top =
                        vdec_msg->msgdata.output_frame.framesize.top;
                    output_respbuf->len = vdec_msg->msgdata.output_frame.len;
                    output_respbuf->offset = vdec_msg->msgdata.output_frame.offset;
                    output_respbuf->time_stamp = vdec_msg->msgdata.output_frame.time_stamp;
                    output_respbuf->flags = vdec_msg->msgdata.output_frame.flags;
                    output_respbuf->pic_type = vdec_msg->msgdata.output_frame.pic_type;
                    output_respbuf->interlaced_format = vdec_msg->msgdata.output_frame.interlaced_format;
                    output_respbuf->aspect_ratio_info =
                        vdec_msg->msgdata.output_frame.aspect_ratio_info;

                    if (omx->output_use_buffer)
                        memcpy ( omxhdr->pBuffer,
                                (vdec_msg->msgdata.output_frame.bufferaddr +
                                 vdec_msg->msgdata.output_frame.offset),
                                vdec_msg->msgdata.output_frame.len );
                } else
                    omxhdr->nFilledLen = 0;

                omx->post_event ((unsigned int)omxhdr, vdec_msg->status_code,
                        OMX_COMPONENT_GENERATE_FBD);
            } else if (vdec_msg->msgdata.output_frame.flags & OMX_BUFFERFLAG_EOS)
                omx->post_event (NULL, vdec_msg->status_code,
                        OMX_COMPONENT_GENERATE_EOS_DONE);
            else
                omx->post_event (NULL, vdec_msg->status_code,
                        OMX_COMPONENT_GENERATE_HARDWARE_ERROR);

            break;
        case VDEC_MSG_EVT_CONFIG_CHANGED:
            DEBUG_PRINT_HIGH("Port settings changed");
            omx->post_event ((unsigned int)omxhdr,vdec_msg->status_code,
                    OMX_COMPONENT_GENERATE_PORT_RECONFIG);
            break;
        case VDEC_MSG_EVT_INFO_CONFIG_CHANGED:
            {
                DEBUG_PRINT_HIGH("Port settings changed info");
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

    return 1;
}

OMX_ERRORTYPE omx_vdec::empty_this_buffer_proxy_arbitrary (
        OMX_HANDLETYPE hComp,
        OMX_BUFFERHEADERTYPE *buffer
        )
{
    unsigned address,p2,id;
    DEBUG_PRINT_LOW("Empty this arbitrary");

    if (buffer == NULL) {
        return OMX_ErrorBadParameter;
    }

    DEBUG_PRINT_LOW("ETBProxyArb: bufhdr = %p, bufhdr->pBuffer = %p", buffer, buffer->pBuffer);
    DEBUG_PRINT_LOW("ETBProxyArb: nFilledLen %u, flags %d, timestamp %u",
            buffer->nFilledLen, buffer->nFlags, (unsigned)buffer->nTimeStamp);

    /* return zero length and not an EOS buffer */

    /* return buffer if input flush in progress */
    if ((input_flush_progress == true) || ((buffer->nFilledLen == 0) &&
                ((buffer->nFlags & OMX_BUFFERFLAG_EOS) == 0))) {
        DEBUG_PRINT_HIGH("return zero legth buffer or flush in progress");
        m_cb.EmptyBufferDone (hComp,m_app_data,buffer);
        return OMX_ErrorNone;
    }

    if (psource_frame == NULL) {
        DEBUG_PRINT_LOW("Set Buffer as source Buffer %p time stamp %d",buffer,buffer->nTimeStamp);
        psource_frame = buffer;
        DEBUG_PRINT_LOW("Try to Push One Input Buffer ");
        push_input_buffer (hComp);
    } else {
        DEBUG_PRINT_LOW("Push the source buffer into pendingq %p",buffer);

        if (!m_input_pending_q.insert_entry((unsigned)buffer,NULL,NULL)) {
            return OMX_ErrorBadParameter;
        }
    }


    return OMX_ErrorNone;
}

OMX_ERRORTYPE omx_vdec::push_input_buffer (OMX_HANDLETYPE hComp)
{
    unsigned address,p2,id;
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if (pdest_frame == NULL || psource_frame == NULL) {
        /*Check if we have a destination buffer*/
        if (pdest_frame == NULL) {
            DEBUG_PRINT_LOW("Get a Destination buffer from the queue");

            if (m_input_free_q.m_size) {
                m_input_free_q.pop_entry(&address,&p2,&id);
                pdest_frame = (OMX_BUFFERHEADERTYPE *)address;
                pdest_frame->nFilledLen = 0;
                pdest_frame->nTimeStamp = LLONG_MAX;
                DEBUG_PRINT_LOW("Address of Pmem Buffer %p",pdest_frame);
            }
        }

        /*Check if we have a destination buffer*/
        if (psource_frame == NULL) {
            DEBUG_PRINT_LOW("Get a source buffer from the queue");

            if (m_input_pending_q.m_size) {
                m_input_pending_q.pop_entry(&address,&p2,&id);
                psource_frame = (OMX_BUFFERHEADERTYPE *)address;
                DEBUG_PRINT_LOW("Next source Buffer %p time stamp %d",psource_frame,
                        psource_frame->nTimeStamp);
                DEBUG_PRINT_LOW("Next source Buffer flag %d length %d",
                        psource_frame->nFlags,psource_frame->nFilledLen);

            }
        }

    }

    while ((pdest_frame != NULL) && (psource_frame != NULL)) {
        switch (codec_type_parse) {
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

        if (ret != OMX_ErrorNone) {
            DEBUG_PRINT_ERROR("Pushing input Buffer Failed");
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

    DEBUG_PRINT_LOW("Start Parsing the bit stream address %p TimeStamp %d",
            psource_frame,psource_frame->nTimeStamp);

    if (m_frame_parser.parse_sc_frame(psource_frame,
                pdest_frame,&partial_frame) == -1) {
        DEBUG_PRINT_ERROR("Error In Parsing Return Error");
        return OMX_ErrorBadParameter;
    }

    if (partial_frame == 0) {
        DEBUG_PRINT_LOW("Frame size %d source %p frame count %d",
                pdest_frame->nFilledLen,psource_frame,frame_count);


        DEBUG_PRINT_LOW("TimeStamp updated %d",pdest_frame->nTimeStamp);

        /*First Parsed buffer will have only header Hence skip*/
        if (frame_count == 0) {
            DEBUG_PRINT_LOW("H263/MPEG4 Codec First Frame ");
#ifdef MAX_RES_1080P

            if (codec_type_parse == CODEC_TYPE_MPEG4 ||
                    codec_type_parse == CODEC_TYPE_DIVX) {
                mp4StreamType psBits;
                psBits.data = pdest_frame->pBuffer + pdest_frame->nOffset;
                psBits.numBytes = pdest_frame->nFilledLen;
                mp4_headerparser.parseHeader(&psBits);
            }

#endif
            frame_count++;
        } else {
            pdest_frame->nFlags &= ~OMX_BUFFERFLAG_EOS;

            if (pdest_frame->nFilledLen) {
                /*Push the frame to the Decoder*/
                if (empty_this_buffer_proxy(hComp,pdest_frame) != OMX_ErrorNone) {
                    return OMX_ErrorBadParameter;
                }

                frame_count++;
                pdest_frame = NULL;

                if (m_input_free_q.m_size) {
                    m_input_free_q.pop_entry(&address,&p2,&id);
                    pdest_frame = (OMX_BUFFERHEADERTYPE *) address;
                    pdest_frame->nFilledLen = 0;
                }
            } else if (!(psource_frame->nFlags & OMX_BUFFERFLAG_EOS)) {
                DEBUG_PRINT_ERROR("Zero len buffer return back to POOL");
                m_input_free_q.insert_entry((unsigned) pdest_frame,NULL,NULL);
                pdest_frame = NULL;
            }
        }
    } else {
        DEBUG_PRINT_LOW("Not a Complete Frame %d",pdest_frame->nFilledLen);

        /*Check if Destination Buffer is full*/
        if (pdest_frame->nAllocLen ==
                pdest_frame->nFilledLen + pdest_frame->nOffset) {
            DEBUG_PRINT_ERROR("ERROR:Frame Not found though Destination Filled");
            return OMX_ErrorStreamCorrupt;
        }
    }

    if (psource_frame->nFilledLen == 0) {
        if (psource_frame->nFlags & OMX_BUFFERFLAG_EOS) {
            if (pdest_frame) {
                pdest_frame->nFlags |= psource_frame->nFlags;
                DEBUG_PRINT_LOW("Frame Found start Decoding Size =%d TimeStamp = %x",
                        pdest_frame->nFilledLen,pdest_frame->nTimeStamp);
                DEBUG_PRINT_LOW("Found a frame size = %d number = %d",
                        pdest_frame->nFilledLen,frame_count++);

                /*Push the frame to the Decoder*/
                if (empty_this_buffer_proxy(hComp,pdest_frame) != OMX_ErrorNone) {
                    return OMX_ErrorBadParameter;
                }

                frame_count++;
                pdest_frame = NULL;
            } else {
                DEBUG_PRINT_LOW("Last frame in else dest addr") ;
                generate_ebd = OMX_FALSE;
            }
        }

        if (generate_ebd) {
            DEBUG_PRINT_LOW("Buffer Consumed return back to client %p",psource_frame);
            m_cb.EmptyBufferDone (hComp,m_app_data,psource_frame);
            psource_frame = NULL;

            if (m_input_pending_q.m_size) {
                DEBUG_PRINT_LOW("Pull Next source Buffer %p",psource_frame);
                m_input_pending_q.pop_entry(&address,&p2,&id);
                psource_frame = (OMX_BUFFERHEADERTYPE *) address;
                DEBUG_PRINT_LOW("Next source Buffer %p time stamp %d",psource_frame,
                        psource_frame->nTimeStamp);
                DEBUG_PRINT_LOW("Next source Buffer flag %d length %d",
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

    if (h264_scratch.pBuffer == NULL) {
        DEBUG_PRINT_ERROR("ERROR:H.264 Scratch Buffer not allocated");
        return OMX_ErrorBadParameter;
    }

    DEBUG_PRINT_LOW("Pending h264_scratch.nFilledLen %d "
            "look_ahead_nal %d", h264_scratch.nFilledLen, look_ahead_nal);
    DEBUG_PRINT_LOW("Pending pdest_frame->nFilledLen %d",pdest_frame->nFilledLen);

    if (h264_scratch.nFilledLen && look_ahead_nal) {
        look_ahead_nal = false;

        if ((pdest_frame->nAllocLen - pdest_frame->nFilledLen) >=
                h264_scratch.nFilledLen) {
            memcpy ((pdest_frame->pBuffer + pdest_frame->nFilledLen),
                    h264_scratch.pBuffer,h264_scratch.nFilledLen);
            pdest_frame->nFilledLen += h264_scratch.nFilledLen;
            DEBUG_PRINT_LOW("Copy the previous NAL (h264 scratch) into Dest frame");
            h264_scratch.nFilledLen = 0;
        } else {
            DEBUG_PRINT_ERROR("Error:1: Destination buffer overflow for H264");
            return OMX_ErrorBadParameter;
        }
    }

    if (nal_length == 0) {
        DEBUG_PRINT_LOW("Zero NAL, hence parse using start code");

        if (m_frame_parser.parse_sc_frame(psource_frame,
                    &h264_scratch,&partial_frame) == -1) {
            DEBUG_PRINT_ERROR("Error In Parsing Return Error");
            return OMX_ErrorBadParameter;
        }
    } else {
        DEBUG_PRINT_LOW("Non-zero NAL length clip, hence parse with NAL size %d ",nal_length);

        if (m_frame_parser.parse_h264_nallength(psource_frame,
                    &h264_scratch,&partial_frame) == -1) {
            DEBUG_PRINT_ERROR("Error In Parsing NAL size, Return Error");
            return OMX_ErrorBadParameter;
        }
    }

    if (partial_frame == 0) {
        if (nal_count == 0 && h264_scratch.nFilledLen == 0) {
            DEBUG_PRINT_LOW("First NAL with Zero Length, hence Skip");
            nal_count++;
            h264_scratch.nTimeStamp = psource_frame->nTimeStamp;
            h264_scratch.nFlags = psource_frame->nFlags;
        } else {
            DEBUG_PRINT_LOW("Parsed New NAL Length = %d",h264_scratch.nFilledLen);

            if (h264_scratch.nFilledLen) {
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

                if (m_frame_parser.mutils->nalu_type == NALU_TYPE_NON_IDR ||
                        m_frame_parser.mutils->nalu_type == NALU_TYPE_IDR) {
                    h264_last_au_ts = h264_scratch.nTimeStamp;
                    h264_last_au_flags = h264_scratch.nFlags;
#ifndef PROCESS_EXTRADATA_IN_OUTPUT_PORT

                    if (client_extradata & OMX_TIMEINFO_EXTRADATA) {
                        OMX_S64 ts_in_sei = h264_parser->process_ts_with_sei_vui(h264_last_au_ts);

                        if (!VALID_TS(h264_last_au_ts))
                            h264_last_au_ts = ts_in_sei;
                    }

#endif
                } else
                    h264_last_au_ts = LLONG_MAX;
            }

            if (!isNewFrame) {
                if ( (pdest_frame->nAllocLen - pdest_frame->nFilledLen) >=
                        h264_scratch.nFilledLen) {
                    DEBUG_PRINT_LOW("Not a NewFrame Copy into Dest len %d",
                            h264_scratch.nFilledLen);
                    memcpy ((pdest_frame->pBuffer + pdest_frame->nFilledLen),
                            h264_scratch.pBuffer,h264_scratch.nFilledLen);
                    pdest_frame->nFilledLen += h264_scratch.nFilledLen;

                    if (m_frame_parser.mutils->nalu_type == NALU_TYPE_EOSEQ)
                        pdest_frame->nFlags |= QOMX_VIDEO_BUFFERFLAG_EOSEQ;

                    h264_scratch.nFilledLen = 0;
                } else {
                    DEBUG_PRINT_ERROR("Error:2: Destination buffer overflow for H264");
                    return OMX_ErrorBadParameter;
                }
            } else {
                look_ahead_nal = true;
                DEBUG_PRINT_LOW("Frame Found start Decoding Size =%d TimeStamp = %x",
                        pdest_frame->nFilledLen,pdest_frame->nTimeStamp);
                DEBUG_PRINT_LOW("Found a frame size = %d number = %d",
                        pdest_frame->nFilledLen,frame_count++);

                if (pdest_frame->nFilledLen == 0) {
                    DEBUG_PRINT_LOW("Copy the Current Frame since and push it");
                    look_ahead_nal = false;

                    if ( (pdest_frame->nAllocLen - pdest_frame->nFilledLen) >=
                            h264_scratch.nFilledLen) {
                        memcpy ((pdest_frame->pBuffer + pdest_frame->nFilledLen),
                                h264_scratch.pBuffer,h264_scratch.nFilledLen);
                        pdest_frame->nFilledLen += h264_scratch.nFilledLen;
                        h264_scratch.nFilledLen = 0;
                    } else {
                        DEBUG_PRINT_ERROR("Error:3: Destination buffer overflow for H264");
                        return OMX_ErrorBadParameter;
                    }
                } else {
                    if (psource_frame->nFilledLen || h264_scratch.nFilledLen) {
                        DEBUG_PRINT_LOW("Reset the EOS Flag");
                        pdest_frame->nFlags &= ~OMX_BUFFERFLAG_EOS;
                    }

                    /*Push the frame to the Decoder*/
                    if (empty_this_buffer_proxy(hComp,pdest_frame) != OMX_ErrorNone) {
                        return OMX_ErrorBadParameter;
                    }

                    //frame_count++;
                    pdest_frame = NULL;

                    if (m_input_free_q.m_size) {
                        m_input_free_q.pop_entry(&address,&p2,&id);
                        pdest_frame = (OMX_BUFFERHEADERTYPE *) address;
                        DEBUG_PRINT_LOW("Pop the next pdest_buffer %p",pdest_frame);
                        pdest_frame->nFilledLen = 0;
                        pdest_frame->nFlags = 0;
                        pdest_frame->nTimeStamp = LLONG_MAX;
                    }
                }
            }
        }
    } else {
        DEBUG_PRINT_LOW("Not a Complete Frame, pdest_frame->nFilledLen %d",pdest_frame->nFilledLen);

        /*Check if Destination Buffer is full*/
        if (h264_scratch.nAllocLen ==
                h264_scratch.nFilledLen + h264_scratch.nOffset) {
            DEBUG_PRINT_ERROR("ERROR: Frame Not found though Destination Filled");
            return OMX_ErrorStreamCorrupt;
        }
    }

    if (!psource_frame->nFilledLen) {
        DEBUG_PRINT_LOW("Buffer Consumed return source %p back to client",psource_frame);

        if (psource_frame->nFlags & OMX_BUFFERFLAG_EOS) {
            if (pdest_frame) {
                DEBUG_PRINT_LOW("EOS Reached Pass Last Buffer");

                if ( (pdest_frame->nAllocLen - pdest_frame->nFilledLen) >=
                        h264_scratch.nFilledLen) {
                    memcpy ((pdest_frame->pBuffer + pdest_frame->nFilledLen),
                            h264_scratch.pBuffer,h264_scratch.nFilledLen);
                    pdest_frame->nFilledLen += h264_scratch.nFilledLen;
                    h264_scratch.nFilledLen = 0;
                } else {
                    DEBUG_PRINT_ERROR("ERROR:4: Destination buffer overflow for H264");
                    return OMX_ErrorBadParameter;
                }

                pdest_frame->nTimeStamp = h264_scratch.nTimeStamp;
                pdest_frame->nFlags = h264_scratch.nFlags | psource_frame->nFlags;
#ifdef MAX_RES_720P

                if (frame_count == 0) {
                    DEBUG_PRINT_HIGH("No frames sent to driver yet, "
                            "So send zero length EOS buffer");
                    pdest_frame->nFilledLen = 0;
                }

#endif
                DEBUG_PRINT_LOW("pdest_frame->nFilledLen = %d, nFlags = 0x%x, TimeStamp = %x",
                        pdest_frame->nFilledLen, pdest_frame->nFlags, pdest_frame->nTimeStamp);
                DEBUG_PRINT_LOW("Push AU frame number %d to driver", frame_count++);
#ifndef PROCESS_EXTRADATA_IN_OUTPUT_PORT

                if (client_extradata & OMX_TIMEINFO_EXTRADATA) {
                    OMX_S64 ts_in_sei = h264_parser->process_ts_with_sei_vui(pdest_frame->nTimeStamp);

                    if (!VALID_TS(pdest_frame->nTimeStamp))
                        pdest_frame->nTimeStamp = ts_in_sei;
                }

#endif

                /*Push the frame to the Decoder*/
                if (empty_this_buffer_proxy(hComp,pdest_frame) != OMX_ErrorNone) {
                    return OMX_ErrorBadParameter;
                }

                frame_count++;
                pdest_frame = NULL;
            } else {
                DEBUG_PRINT_LOW("Last frame in else dest addr %p size %d",
                        pdest_frame,h264_scratch.nFilledLen);
                generate_ebd = OMX_FALSE;
            }
        }
    }

    if (generate_ebd && !psource_frame->nFilledLen) {
        m_cb.EmptyBufferDone (hComp,m_app_data,psource_frame);
        psource_frame = NULL;

        if (m_input_pending_q.m_size) {
            DEBUG_PRINT_LOW("Pull Next source Buffer %p",psource_frame);
            m_input_pending_q.pop_entry(&address,&p2,&id);
            psource_frame = (OMX_BUFFERHEADERTYPE *) address;
            DEBUG_PRINT_LOW("Next source Buffer flag %d src length %d",
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

    if (first_frame == 0) {
        first_frame = 1;
        DEBUG_PRINT_LOW("First i/p buffer for VC1 arbitrary bytes");

        if (!m_vendor_config.pData) {
            DEBUG_PRINT_LOW("Check profile type in 1st source buffer");
            buf = psource_frame->pBuffer;
            buf_len = psource_frame->nFilledLen;

            if ((*((OMX_U32 *) buf) & VC1_SP_MP_START_CODE_MASK) ==
                    VC1_SP_MP_START_CODE) {
                m_vc1_profile = VC1_SP_MP_RCV;
            } else if (*((OMX_U32 *) buf) & VC1_AP_SEQ_START_CODE) {
                m_vc1_profile = VC1_AP;
            } else {
                DEBUG_PRINT_ERROR("Invalid sequence layer in first buffer");
                return OMX_ErrorStreamCorrupt;
            }
        } else {
            pdest = pdest_frame->pBuffer + pdest_frame->nFilledLen +
                pdest_frame->nOffset;
            dest_len = pdest_frame->nAllocLen - (pdest_frame->nFilledLen +
                    pdest_frame->nOffset);

            if (dest_len < m_vendor_config.nDataSize) {
                DEBUG_PRINT_ERROR("Destination buffer full");
                return OMX_ErrorBadParameter;
            } else {
                memcpy(pdest, m_vendor_config.pData, m_vendor_config.nDataSize);
                pdest_frame->nFilledLen += m_vendor_config.nDataSize;
            }
        }
    }

    switch (m_vc1_profile) {
        case VC1_AP:
            DEBUG_PRINT_LOW("VC1 AP, hence parse using frame start code");

            if (push_input_sc_codec(hComp) != OMX_ErrorNone) {
                DEBUG_PRINT_ERROR("Error In Parsing VC1 AP start code");
                return OMX_ErrorBadParameter;
            }

            break;

        case VC1_SP_MP_RCV:
        default:
            DEBUG_PRINT_ERROR("Unsupported VC1 profile in ArbitraryBytes Mode");
            return OMX_ErrorBadParameter;
    }

    return OMX_ErrorNone;
}

#ifndef USE_ION
bool omx_vdec::align_pmem_buffers(int pmem_fd, OMX_U32 buffer_size,
        OMX_U32 alignment)
{
    struct pmem_allocation allocation;
    allocation.size = buffer_size;
    allocation.align = clip2(alignment);

    if (allocation.align < 4096) {
        allocation.align = 4096;
    }

    if (ioctl(pmem_fd, PMEM_ALLOCATE_ALIGNED, &allocation) < 0) {
        DEBUG_PRINT_ERROR("Aligment(%u) failed with pmem driver Sz(%lu)",
                allocation.align, allocation.size);
        return false;
    }

    return true;
}
#endif

#ifdef USE_ION
int omx_vdec::alloc_map_ion_memory(OMX_U32 buffer_size,
        OMX_U32 alignment, struct ion_allocation_data *alloc_data,
        struct ion_fd_data *fd_data,int flag)
{
    int fd = -EINVAL;
    int rc = -EINVAL;
    int ion_dev_flag;
    struct vdec_ion ion_buf_info;

    if (!alloc_data || buffer_size <= 0 || !fd_data) {
        DEBUG_PRINT_ERROR("Invalid arguments to alloc_map_ion_memory");
        return -EINVAL;
    }

    ion_dev_flag = O_RDONLY;
    fd = open (MEM_DEVICE, ion_dev_flag);

    if (fd < 0) {
        DEBUG_PRINT_ERROR("opening ion device failed with fd = %d", fd);
        return fd;
    }

    alloc_data->flags = 0;

    if (!secure_mode && (flag & ION_FLAG_CACHED)) {
        alloc_data->flags |= ION_FLAG_CACHED;
    }

    alloc_data->len = buffer_size;
    alloc_data->align = clip2(alignment);

    if (alloc_data->align < 4096) {
        alloc_data->align = 4096;
    }

    if (secure_mode) {
        if (external_meta_buffer) {
            alloc_data->ION_HEAP_MASK = ION_HEAP(ION_CP_MFC_HEAP_ID);
            alloc_data->flags |= ION_SECURE;
        } else if (external_meta_buffer_iommu) {
            alloc_data->ION_HEAP_MASK = ION_HEAP(ION_IOMMU_HEAP_ID);
        } else {
            alloc_data->ION_HEAP_MASK = ION_HEAP(MEM_HEAP_ID);
            alloc_data->flags |= ION_SECURE;
        }
    } else {
#ifdef MAX_RES_720P
        alloc_data->len = (buffer_size + (alloc_data->align - 1)) & ~(alloc_data->align - 1);
        alloc_data->ION_HEAP_MASK = ION_HEAP(MEM_HEAP_ID);
#else
        alloc_data->ION_HEAP_MASK = (ION_HEAP(MEM_HEAP_ID) | ION_HEAP(ION_IOMMU_HEAP_ID));
#endif
    }

    rc = ioctl(fd,ION_IOC_ALLOC,alloc_data);

    if (rc || !alloc_data->handle) {
        DEBUG_PRINT_ERROR("ION ALLOC memory failed ");
        alloc_data->handle = NULL;
        close(fd);
        fd = -ENOMEM;
        return fd;
    }

    fd_data->handle = alloc_data->handle;
    rc = ioctl(fd,ION_IOC_MAP,fd_data);

    if (rc) {
        DEBUG_PRINT_ERROR("ION MAP failed ");
        ion_buf_info.ion_alloc_data = *alloc_data;
        ion_buf_info.ion_device_fd = fd;
        ion_buf_info.fd_ion_data = *fd_data;
        free_ion_memory(&ion_buf_info);
        fd_data->fd =-1;
        close(fd);
        fd = -ENOMEM;
    }

    DEBUG_PRINT_HIGH("ION: alloc_data: handle(0x%X), len(%u), align(%u), "
            "flags(0x%x), fd_data: handle(0x%x), fd(0x%x)",
            alloc_data->handle, alloc_data->len, alloc_data->align,
            alloc_data->flags, fd_data->handle, fd_data->fd);
    return fd;
}

void omx_vdec::free_ion_memory(struct vdec_ion *buf_ion_info)
{

    if (!buf_ion_info) {
        DEBUG_PRINT_ERROR("ION: free called with invalid fd/allocdata");
        return;
    }

    DEBUG_PRINT_HIGH("ION: free: handle(0x%X), len(%u), fd(0x%x)",
            buf_ion_info->ion_alloc_data.handle,
            buf_ion_info->ion_alloc_data.len,
            buf_ion_info->fd_ion_data.fd);

    if (ioctl(buf_ion_info->ion_device_fd,ION_IOC_FREE,
                &buf_ion_info->ion_alloc_data.handle)) {
        DEBUG_PRINT_ERROR("ION: free failed" );
    }

    close(buf_ion_info->ion_device_fd);
    buf_ion_info->ion_device_fd = -1;
    buf_ion_info->ion_alloc_data.handle = NULL;
    buf_ion_info->fd_ion_data.fd = -1;
}
#endif
void omx_vdec::free_output_buffer_header()
{
    DEBUG_PRINT_HIGH("ALL output buffers are freed/released");
    output_use_buffer = false;
    ouput_egl_buffers = false;

    if (m_out_mem_ptr) {
        free (m_out_mem_ptr);
        m_out_mem_ptr = NULL;
    }

    if (m_platform_list) {
        free(m_platform_list);
        m_platform_list = NULL;
    }

    if (drv_ctx.ptr_respbuffer) {
        free (drv_ctx.ptr_respbuffer);
        drv_ctx.ptr_respbuffer = NULL;
    }

    if (drv_ctx.ptr_outputbuffer) {
        free (drv_ctx.ptr_outputbuffer);
        drv_ctx.ptr_outputbuffer = NULL;
    }

#ifdef USE_ION

    if (drv_ctx.op_buf_ion_info) {
        DEBUG_PRINT_LOW("Free o/p ion context");
        free(drv_ctx.op_buf_ion_info);
        drv_ctx.op_buf_ion_info = NULL;
    }

#endif
}

void omx_vdec::free_input_buffer_header()
{
    input_use_buffer = false;

    if (arbitrary_bytes) {
        if (m_frame_parser.mutils) {
            DEBUG_PRINT_LOW("Free utils parser");
            delete (m_frame_parser.mutils);
            m_frame_parser.mutils = NULL;
        }

        if (m_inp_heap_ptr) {
            DEBUG_PRINT_LOW("Free input Heap Pointer");
            free (m_inp_heap_ptr);
            m_inp_heap_ptr = NULL;
        }

        if (m_phdr_pmem_ptr) {
            DEBUG_PRINT_LOW("Free input pmem header Pointer");
            free (m_phdr_pmem_ptr);
            m_phdr_pmem_ptr = NULL;
        }
    }

    if (m_inp_mem_ptr) {
        DEBUG_PRINT_LOW("Free input pmem Pointer area");
        free (m_inp_mem_ptr);
        m_inp_mem_ptr = NULL;
    }

    if (drv_ctx.ptr_inputbuffer) {
        DEBUG_PRINT_LOW("Free Driver Context pointer");
        free (drv_ctx.ptr_inputbuffer);
        drv_ctx.ptr_inputbuffer = NULL;
    }

#ifdef USE_ION

    if (drv_ctx.ip_buf_ion_info) {
        DEBUG_PRINT_LOW("Free ion context");
        free(drv_ctx.ip_buf_ion_info);
        drv_ctx.ip_buf_ion_info = NULL;
    }

#endif
}

OMX_ERRORTYPE omx_vdec::get_buffer_req(vdec_allocatorproperty *buffer_prop)
{
    struct vdec_ioctl_msg ioctl_msg = {NULL, NULL};
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    unsigned int buf_size = 0, extra_data_size = 0;
    DEBUG_PRINT_LOW("GetBufReq IN: ActCnt(%d) Size(%d)",
            buffer_prop->actualcount, buffer_prop->buffer_size);
    ioctl_msg.in = NULL;
    ioctl_msg.out = buffer_prop;

    if (ioctl (drv_ctx.video_driver_fd, VDEC_IOCTL_GET_BUFFER_REQ,
                (void*)&ioctl_msg) < 0) {
        DEBUG_PRINT_ERROR("Requesting buffer requirements failed");
        eRet = OMX_ErrorInsufficientResources;
    } else {
        buf_size = buffer_prop->buffer_size;

        if (client_extradata & OMX_FRAMEINFO_EXTRADATA) {
            DEBUG_PRINT_HIGH("Frame info extra data enabled!");
            extra_data_size += OMX_FRAMEINFO_EXTRADATA_SIZE;
        }

        if (client_extradata & OMX_INTERLACE_EXTRADATA) {
            DEBUG_PRINT_HIGH("Interlace extra data enabled!");
            extra_data_size += OMX_INTERLACE_EXTRADATA_SIZE;
        }

        if (client_extradata & OMX_PORTDEF_EXTRADATA) {
            extra_data_size += OMX_PORTDEF_EXTRADATA_SIZE;
            DEBUG_PRINT_HIGH("Smooth streaming enabled extra_data_size=%d",
                    extra_data_size);
        }

        if (extra_data_size) {
            extra_data_size += sizeof(OMX_OTHER_EXTRADATATYPE); //Space for terminator
            buf_size = ((buf_size + 3)&(~3)); //Align extradata start address to 64Bit
        }

        buf_size += extra_data_size;
        buf_size = (buf_size + buffer_prop->alignment - 1)&(~(buffer_prop->alignment - 1));
        DEBUG_PRINT_LOW("GetBufReq UPDATE: ActCnt(%d) Size(%d) BufSize(%d)",
                buffer_prop->actualcount, buffer_prop->buffer_size, buf_size);

        if (in_reconfig) // BufReq will be set to driver when port is disabled
            buffer_prop->buffer_size = buf_size;
        else if (buf_size != buffer_prop->buffer_size) {
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
    struct vdec_ioctl_msg ioctl_msg = {NULL, NULL};
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    unsigned buf_size = 0;
    DEBUG_PRINT_LOW("SetBufReq IN: ActCnt(%d) Size(%d)",
            buffer_prop->actualcount, buffer_prop->buffer_size);
    buf_size = (buffer_prop->buffer_size + buffer_prop->alignment - 1)&(~(buffer_prop->alignment - 1));

    if (buf_size != buffer_prop->buffer_size) {
        DEBUG_PRINT_ERROR("Buffer size alignment error: Requested(%d) Required(%d)",
                buffer_prop->buffer_size, buf_size);
        eRet = OMX_ErrorBadParameter;
    } else {
        ioctl_msg.in = buffer_prop;
        ioctl_msg.out = NULL;

        if (ioctl (drv_ctx.video_driver_fd, VDEC_IOCTL_SET_BUFFER_REQ,
                    (void*)&ioctl_msg) < 0) {
            DEBUG_PRINT_ERROR("Setting buffer requirements failed");
            eRet = OMX_ErrorInsufficientResources;
        } else {
            if (!client_buffers.update_buffer_req()) {
                DEBUG_PRINT_ERROR("Setting c2D buffer requirements failed");
                eRet = OMX_ErrorInsufficientResources;
            }
        }
    }

    return eRet;
}

OMX_ERRORTYPE omx_vdec::start_port_reconfig()
{
    struct vdec_ioctl_msg ioctl_msg = {NULL, NULL};
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    eRet = update_picture_resolution();

    if (eRet == OMX_ErrorNone) {
        ioctl_msg.out = &drv_ctx.interlace;

        if (ioctl(drv_ctx.video_driver_fd, VDEC_IOCTL_GET_INTERLACE_FORMAT, &ioctl_msg)) {
            DEBUG_PRINT_ERROR("Error VDEC_IOCTL_GET_INTERLACE_FORMAT");
            eRet = OMX_ErrorHardware;
        } else {
            if (drv_ctx.interlace != VDEC_InterlaceFrameProgressive) {
                DEBUG_PRINT_HIGH("Interlace format detected (%x)!", drv_ctx.interlace);
                client_extradata |= OMX_INTERLACE_EXTRADATA;
            }

            in_reconfig = true;
            op_buf_rcnfg.buffer_type = VDEC_BUFFER_TYPE_OUTPUT;
            eRet = get_buffer_req(&op_buf_rcnfg);
        }
    }

    if (eRet == OMX_ErrorNone) {
        //update power HAL with new width, height and bitrate
        power_module_deregister();
        power_module_register();
    }

    return eRet;
}

OMX_ERRORTYPE omx_vdec::update_picture_resolution()
{
    struct vdec_ioctl_msg ioctl_msg = {NULL, NULL};
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    ioctl_msg.in = NULL;
    ioctl_msg.out = &drv_ctx.video_resolution;

    if (ioctl(drv_ctx.video_driver_fd, VDEC_IOCTL_GET_PICRES, &ioctl_msg)) {
        DEBUG_PRINT_ERROR("Error VDEC_IOCTL_GET_PICRES");
        eRet = OMX_ErrorHardware;
    }

    return eRet;
}

OMX_ERRORTYPE omx_vdec::update_portdef(OMX_PARAM_PORTDEFINITIONTYPE *portDefn)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;

    if (!portDefn) {
        return OMX_ErrorBadParameter;
    }

    DEBUG_PRINT_LOW("omx_vdec::update_portdef");
    portDefn->nVersion.nVersion = OMX_SPEC_VERSION;
    portDefn->nSize = sizeof(portDefn);
    portDefn->eDomain    = OMX_PortDomainVideo;

    if (drv_ctx.frame_rate.fps_denominator > 0)
        portDefn->format.video.xFramerate = drv_ctx.frame_rate.fps_numerator /
            drv_ctx.frame_rate.fps_denominator;
    else {
        DEBUG_PRINT_ERROR("Error: Divide by zero");
        return OMX_ErrorBadParameter;
    }

    if (0 == portDefn->nPortIndex) {
        portDefn->eDir =  OMX_DirInput;
        portDefn->nBufferCountActual = drv_ctx.ip_buf.actualcount;
        portDefn->nBufferCountMin    = drv_ctx.ip_buf.mincount;
        portDefn->nBufferSize        = drv_ctx.ip_buf.buffer_size - DEVICE_SCRATCH;
        portDefn->format.video.eColorFormat = OMX_COLOR_FormatUnused;
        portDefn->format.video.eCompressionFormat = eCompressionFormat;
        portDefn->bEnabled   = m_inp_bEnabled;
        portDefn->bPopulated = m_inp_bPopulated;
    } else if (1 == portDefn->nPortIndex) {
        portDefn->eDir =  OMX_DirOutput;

        if (update_picture_resolution() != OMX_ErrorNone) {
            DEBUG_PRINT_ERROR("update_picture_resolution failed");
            return OMX_ErrorHardware;
        }

        if (!client_buffers.update_buffer_req()) {
            DEBUG_PRINT_ERROR("client_buffers.update_buffer_req Failed");
            return OMX_ErrorHardware;
        }

        if (in_reconfig) {
            portDefn->nBufferCountActual = op_buf_rcnfg.actualcount;
            portDefn->nBufferCountMin    = op_buf_rcnfg.mincount;
            portDefn->nBufferSize        = op_buf_rcnfg.buffer_size;
        } else {
            portDefn->nBufferCountActual = drv_ctx.op_buf.actualcount;
            portDefn->nBufferCountMin    = drv_ctx.op_buf.mincount;
            portDefn->nBufferSize        = drv_ctx.op_buf.buffer_size;
        }

        unsigned int buf_size = 0;

        if (!client_buffers.get_buffer_req(buf_size)) {
            DEBUG_PRINT_ERROR("update buffer requirements");
            return OMX_ErrorHardware;
        }

        portDefn->nBufferSize = buf_size;
        portDefn->format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
        portDefn->bEnabled   = m_out_bEnabled;
        portDefn->bPopulated = m_out_bPopulated;

        if (!client_buffers.get_color_format(portDefn->format.video.eColorFormat)) {
            DEBUG_PRINT_ERROR("Error in getting color format");
            return OMX_ErrorHardware;
        }
    } else {
        portDefn->eDir = OMX_DirMax;
        DEBUG_PRINT_LOW(" get_parameter: Bad Port idx %d",
                (int)portDefn->nPortIndex);
        eRet = OMX_ErrorBadPortIndex;
    }

    portDefn->format.video.nFrameHeight =  drv_ctx.video_resolution.frame_height;
    portDefn->format.video.nFrameWidth  =  drv_ctx.video_resolution.frame_width;
    portDefn->format.video.nStride = drv_ctx.video_resolution.stride;
    portDefn->format.video.nSliceHeight = drv_ctx.video_resolution.scan_lines;
    DEBUG_PRINT_LOW("update_portdef: PortIndex = %u, Width = %d Height = %d "
            "Stride = %u SliceHeight = %u output format = 0x%x, eColorFormat = 0x%x",
            portDefn->nPortIndex,
            portDefn->format.video.nFrameHeight,
            portDefn->format.video.nFrameWidth,
            portDefn->format.video.nStride,
            portDefn->format.video.nSliceHeight,
            drv_ctx.output_format,
            portDefn->format.video.eColorFormat);
    return eRet;
}

OMX_ERRORTYPE omx_vdec::allocate_output_headers()
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *bufHdr = NULL;
    unsigned i= 0;

    if (!m_out_mem_ptr) {
        DEBUG_PRINT_HIGH("Use o/p buffer case - Header List allocation");
        int nBufHdrSize        = 0;
        int nPlatformEntrySize = 0;
        int nPlatformListSize  = 0;
        int nPMEMInfoSize = 0;
        OMX_QCOM_PLATFORM_PRIVATE_LIST      *pPlatformList;
        OMX_QCOM_PLATFORM_PRIVATE_ENTRY     *pPlatformEntry;
        OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO *pPMEMInfo;

        DEBUG_PRINT_LOW("Setting First Output Buffer(%d)",
                drv_ctx.op_buf.actualcount);
        nBufHdrSize        = drv_ctx.op_buf.actualcount *
            sizeof(OMX_BUFFERHEADERTYPE);

        nPMEMInfoSize      = drv_ctx.op_buf.actualcount *
            sizeof(OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO);
        nPlatformListSize  = drv_ctx.op_buf.actualcount *
            sizeof(OMX_QCOM_PLATFORM_PRIVATE_LIST);
        nPlatformEntrySize = drv_ctx.op_buf.actualcount *
            sizeof(OMX_QCOM_PLATFORM_PRIVATE_ENTRY);

        DEBUG_PRINT_LOW("TotalBufHdr %d BufHdrSize %d PMEM %d PL %d",nBufHdrSize,
                sizeof(OMX_BUFFERHEADERTYPE),
                nPMEMInfoSize,
                nPlatformListSize);
        DEBUG_PRINT_LOW("PE %d bmSize %d ",nPlatformEntrySize,
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

        if (m_out_mem_ptr && pPtr && drv_ctx.ptr_outputbuffer
                && drv_ctx.ptr_respbuffer) {
            bufHdr          =  m_out_mem_ptr;
            m_platform_list = (OMX_QCOM_PLATFORM_PRIVATE_LIST *)(pPtr);
            m_platform_entry= (OMX_QCOM_PLATFORM_PRIVATE_ENTRY *)
                (((char *) m_platform_list)  + nPlatformListSize);
            m_pmem_info     = (OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO *)
                (((char *) m_platform_entry) + nPlatformEntrySize);
            pPlatformList   = m_platform_list;
            pPlatformEntry  = m_platform_entry;
            pPMEMInfo       = m_pmem_info;

            DEBUG_PRINT_LOW("Memory Allocation Succeeded for OUT port%p",m_out_mem_ptr);

            // Settting the entire storage nicely
            DEBUG_PRINT_LOW("bHdr %p OutMem %p PE %p",bufHdr,
                    m_out_mem_ptr,pPlatformEntry);
            DEBUG_PRINT_LOW(" Pmem Info = %p ",pPMEMInfo);

            for (i=0; i < drv_ctx.op_buf.actualcount ; i++) {
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
        } else {
            DEBUG_PRINT_ERROR("Output buf mem alloc failed[0x%x][0x%x]",\
                    m_out_mem_ptr, pPtr);

            if (m_out_mem_ptr) {
                free(m_out_mem_ptr);
                m_out_mem_ptr = NULL;
            }

            if (pPtr) {
                free(pPtr);
                pPtr = NULL;
            }

            if (drv_ctx.ptr_outputbuffer) {
                free(drv_ctx.ptr_outputbuffer);
                drv_ctx.ptr_outputbuffer = NULL;
            }

            if (drv_ctx.ptr_respbuffer) {
                free(drv_ctx.ptr_respbuffer);
                drv_ctx.ptr_respbuffer = NULL;
            }

#ifdef USE_ION

            if (drv_ctx.op_buf_ion_info) {
                DEBUG_PRINT_LOW("Free o/p ion context");
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
    while (m_ftb_q.m_size) {
        m_ftb_q.pop_entry(&p1,&p2,&ident);

        if (ident == OMX_COMPONENT_GENERATE_FBD) {
            pending_bd_q.insert_entry(p1,p2,ident);
        } else {
            tmp_q.insert_entry(p1,p2,ident);
        }
    }

    //return all non GENERATE FDB to ftb queue
    while (tmp_q.m_size) {
        tmp_q.pop_entry(&p1,&p2,&ident);
        m_ftb_q.insert_entry(p1,p2,ident);
    }

    // pop all pending GENERATE EDB from etb queue
    while (m_etb_q.m_size) {
        m_etb_q.pop_entry(&p1,&p2,&ident);

        if (ident == OMX_COMPONENT_GENERATE_EBD) {
            pending_bd_q.insert_entry(p1,p2,ident);
        } else {
            tmp_q.insert_entry(p1,p2,ident);
        }
    }

    //return all non GENERATE FDB to etb queue
    while (tmp_q.m_size) {
        tmp_q.pop_entry(&p1,&p2,&ident);
        m_etb_q.insert_entry(p1,p2,ident);
    }

    pthread_mutex_unlock(&m_lock);

    // process all pending buffer dones
    while (pending_bd_q.m_size) {
        pending_bd_q.pop_entry(&p1,&p2,&ident);

        switch (ident) {
            case OMX_COMPONENT_GENERATE_EBD:

                if (empty_buffer_done(&m_cmp, (OMX_BUFFERHEADERTYPE *)p1) != OMX_ErrorNone) {
                    DEBUG_PRINT_ERROR("ERROR: empty_buffer_done() failed!");
                    omx_report_error ();
                }

                break;

            case OMX_COMPONENT_GENERATE_FBD:

                if (fill_buffer_done(&m_cmp, (OMX_BUFFERHEADERTYPE *)p1) != OMX_ErrorNone ) {
                    DEBUG_PRINT_ERROR("ERROR: fill_buffer_done() failed!");
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
            && (((act_timestamp > prev_ts )? act_timestamp - prev_ts: prev_ts-act_timestamp)>2000)) {
        new_frame_interval = (act_timestamp > prev_ts)?
            act_timestamp - prev_ts :
            prev_ts - act_timestamp;

        if (new_frame_interval < frm_int || frm_int == 0) {
            frm_int = new_frame_interval;

            if (frm_int) {
                drv_ctx.frame_rate.fps_numerator = 1e6;
                drv_ctx.frame_rate.fps_denominator = frm_int;
                DEBUG_PRINT_LOW("set_frame_rate: frm_int(%u) fps(%f)",
                        frm_int, drv_ctx.frame_rate.fps_numerator /
                        (float)drv_ctx.frame_rate.fps_denominator);
                ioctl_msg.in = &drv_ctx.frame_rate;

                if (ioctl (drv_ctx.video_driver_fd, VDEC_IOCTL_SET_FRAME_RATE,
                            (void*)&ioctl_msg) < 0) {
                    DEBUG_PRINT_ERROR("Setting frame rate failed");
                }
            }
        }
    }

    prev_ts = act_timestamp;
}

void omx_vdec::adjust_timestamp(OMX_S64 &act_timestamp)
{
    if (rst_prev_ts && VALID_TS(act_timestamp)) {
        prev_ts = act_timestamp;
        rst_prev_ts = false;
    } else if (VALID_TS(prev_ts)) {
        bool codec_cond = (drv_ctx.timestamp_adjust)?
            (!VALID_TS(act_timestamp) || (((act_timestamp > prev_ts)?
                                           (act_timestamp - prev_ts):(prev_ts - act_timestamp)) <= 2000)):
            (!VALID_TS(act_timestamp) || act_timestamp == prev_ts);

        if (frm_int > 0 && codec_cond) {
            DEBUG_PRINT_LOW("adjust_timestamp: original ts[%lld]", act_timestamp);
            act_timestamp = prev_ts + frm_int;
            DEBUG_PRINT_LOW("adjust_timestamp: predicted ts[%lld]", act_timestamp);
            prev_ts = act_timestamp;
        } else
            set_frame_rate(act_timestamp);
    } else if (frm_int > 0)          // In this case the frame rate was set along
    {                               // with the port definition, start ts with 0
        act_timestamp = prev_ts = 0;  // and correct if a valid ts is received.
        rst_prev_ts = true;
    }
}

void omx_vdec::handle_extradata_secure(OMX_BUFFERHEADERTYPE *p_buf_hdr)
{
    OMX_OTHER_EXTRADATATYPE *p_extra = NULL, *p_sei = NULL, *p_vui = NULL, *p_extn_user[32];
    struct vdec_output_frameinfo *output_respbuf;
    OMX_U32 num_conceal_MB = 0;
    OMX_S64 ts_in_sei = 0;
    OMX_U32 frame_rate = 0;
    OMX_U32 extn_user_data_cnt = 0;

    OMX_U32 index = p_buf_hdr - m_out_mem_ptr;
    OMX_U8 *meta_buffer_virtual = (OMX_U8 *)meta_buff.buffer;


    p_extra = (OMX_OTHER_EXTRADATATYPE *)
        ((unsigned)(meta_buffer_virtual +
            index * drv_ctx.op_buf.meta_buffer_size + 3)&(~3));

    //mapping of ouput buffer to the corresponding meta buffer
    output_respbuf = (struct vdec_output_frameinfo *)\
                     p_buf_hdr->pOutputPortPrivate;
    output_respbuf->metadata_info.metabufaddr = (OMX_U8 *)p_extra;
    output_respbuf->metadata_info.size =
        drv_ctx.op_buf.meta_buffer_size;

    meta_buffer_virtual = (OMX_U8 *)p_extra;

    if (drv_ctx.extradata && (p_buf_hdr->nFlags & OMX_BUFFERFLAG_EXTRADATA)) {
        // Process driver extradata
        while (p_extra && p_extra->eType != VDEC_EXTRADATA_NONE) {
            DEBUG_PRINT_LOW("handle_extradata_secure : pBuf(%p) BufTS(%lld) Type(%x) DataSz(%u)",
                    p_buf_hdr, p_buf_hdr->nTimeStamp, p_extra->eType, p_extra->nDataSize);

            if (p_extra->nSize < p_extra->nDataSize) {
                DEBUG_PRINT_ERROR("Corrupt metadata Buffer size %d payload size %d",
                        p_extra->nSize, p_extra->nDataSize);
                p_extra = (OMX_OTHER_EXTRADATATYPE *) (((OMX_U8 *) p_extra) + p_extra->nSize);

                if ((OMX_U8*)p_extra > (meta_buffer_virtual + drv_ctx.op_buf.meta_buffer_size) ||
                        p_extra->nDataSize == 0 || p_extra->nSize == 0)
                    p_extra = NULL;

                continue;
            }

            if (p_extra->eType == VDEC_EXTRADATA_MB_ERROR_MAP) {
                if (client_extradata & OMX_FRAMEINFO_EXTRADATA)
                    num_conceal_MB = count_MB_in_extradata(p_extra);

                if (client_extradata & VDEC_EXTRADATA_MB_ERROR_MAP)
                    // Map driver extradata to corresponding OMX type
                    p_extra->eType = (OMX_EXTRADATATYPE)OMX_ExtraDataConcealMB;
                else
                    p_extra->eType = OMX_ExtraDataMax; // Invalid type to avoid expose this extradata to OMX client

#ifdef _ANDROID_

                if (m_debug_concealedmb) {
                    DEBUG_PRINT_HIGH("Concealed MB percentage is %u", num_conceal_MB);
                }

#endif /* _ANDROID_ */
            } else if (p_extra->eType == VDEC_EXTRADATA_SEI) {
                p_sei = p_extra;
#ifdef MAX_RES_1080P
                h264_parser->parse_nal((OMX_U8*)p_sei->data, p_sei->nDataSize, NALU_TYPE_SEI);
#endif
                p_extra->eType = OMX_ExtraDataMax; // Invalid type to avoid expose this extradata to OMX client
            } else if (p_extra->eType == VDEC_EXTRADATA_VUI) {
                p_vui = p_extra;
#ifdef MAX_RES_1080P
                h264_parser->parse_nal((OMX_U8*)p_vui->data, p_vui->nDataSize, NALU_TYPE_VUI, false);
#endif
                p_extra->eType = OMX_ExtraDataMax; // Invalid type to avoid expose this extradata to OMX client
            } else if (p_extra->eType == VDEC_EXTRADATA_EXT_DATA) {
                OMX_U8 *data_ptr = (OMX_U8*)p_extra->data;
                OMX_U32 value = 0;
                p_extn_user[extn_user_data_cnt] = p_extra;
                extn_user_data_cnt++;

                if ((*data_ptr & 0xf0) == 0x20) {
                    value = ((*data_ptr) & 0x01);
                    data_ptr++;

                    if (value)
                        data_ptr += 3;

                    value = *((OMX_U32*)data_ptr);
                    value = ((value << 24) | (((value >> 8)<<24)>>8) |
                            (((value >> 16)<<24)>>16) | (value >> 24));
                    m_disp_hor_size = (value & 0xfffc0000)>>18;
                    m_disp_vert_size = (value & 0x0001fff8)>>3;
                    DEBUG_PRINT_LOW("Display Vertical Size = %d Display Horizontal Size = %d",
                            m_disp_vert_size, m_disp_hor_size);
                }
            } else if (p_extra->eType == VDEC_EXTRADATA_USER_DATA) {
                p_extn_user[extn_user_data_cnt] = p_extra;
                extn_user_data_cnt++;
            }

            print_debug_extradata(p_extra);
            p_extra = (OMX_OTHER_EXTRADATATYPE *) (((OMX_U8 *) p_extra) + p_extra->nSize);

            if ((OMX_U8*)p_extra > (meta_buffer_virtual + drv_ctx.op_buf.meta_buffer_size) ||
                    p_extra->nDataSize == 0 || p_extra->nSize == 0)
                p_extra = NULL;
        }

        if (!(client_extradata & VDEC_EXTRADATA_MB_ERROR_MAP)) {
            //moving p_extra to the starting location of the metadata buffer
            p_extra = (OMX_OTHER_EXTRADATATYPE *)meta_buffer_virtual;
            // Driver extradata is only exposed if MB map is requested by client,
            // otherwise can be overwritten by omx extradata.
            p_buf_hdr->nFlags &= ~OMX_BUFFERFLAG_EXTRADATA;
        }
    }

    if (drv_ctx.decoder_format == VDEC_CODECTYPE_H264) {
        if (client_extradata & OMX_TIMEINFO_EXTRADATA) {
            if (p_vui)
                h264_parser->parse_nal((OMX_U8*)p_vui->data, p_vui->nDataSize, NALU_TYPE_VUI, false);

            if (p_sei)
                h264_parser->parse_nal((OMX_U8*)p_sei->data, p_sei->nDataSize, NALU_TYPE_SEI);

            ts_in_sei = h264_parser->process_ts_with_sei_vui(p_buf_hdr->nTimeStamp);

            if (!VALID_TS(p_buf_hdr->nTimeStamp))
                p_buf_hdr->nTimeStamp = ts_in_sei;
        } else if ((client_extradata & OMX_FRAMEINFO_EXTRADATA) && p_sei)
            // If timeinfo is present frame info from SEI is already processed
            h264_parser->parse_nal((OMX_U8*)p_sei->data, p_sei->nDataSize, NALU_TYPE_SEI);
    }

    if (client_extradata & OMX_EXTNUSER_EXTRADATA && p_extra) {
        p_buf_hdr->nFlags |= OMX_BUFFERFLAG_EXTRADATA;

        for (int i = 0; i < extn_user_data_cnt; i++) {
            if (((OMX_U8*)p_extra + p_extn_user[i]->nSize) <
                    (meta_buffer_virtual + drv_ctx.op_buf.meta_buffer_size)) {
                if (p_extn_user[i]->eType == VDEC_EXTRADATA_EXT_DATA) {
                    append_extn_extradata(p_extra, p_extn_user[i]);
                    p_extra = (OMX_OTHER_EXTRADATATYPE *) (((OMX_U8 *) p_extra) + p_extra->nSize);
                } else if (p_extn_user[i]->eType == VDEC_EXTRADATA_USER_DATA) {
                    append_user_extradata(p_extra, p_extn_user[i]);
                    p_extra = (OMX_OTHER_EXTRADATATYPE *) (((OMX_U8 *) p_extra) + p_extra->nSize);
                }
            }
        }
    }

    if ((client_extradata & OMX_INTERLACE_EXTRADATA) && p_extra &&
            ((OMX_U8*)p_extra + OMX_INTERLACE_EXTRADATA_SIZE) <
            (meta_buffer_virtual + drv_ctx.op_buf.meta_buffer_size)) {
        p_buf_hdr->nFlags |= OMX_BUFFERFLAG_EXTRADATA;
        append_interlace_extradata(p_extra,
                ((struct vdec_output_frameinfo *)p_buf_hdr->pOutputPortPrivate)->interlaced_format, index);
        p_extra = (OMX_OTHER_EXTRADATATYPE *) (((OMX_U8 *) p_extra) + p_extra->nSize);
    }

    if (client_extradata & OMX_FRAMEINFO_EXTRADATA && p_extra &&
            ((OMX_U8*)p_extra + OMX_FRAMEINFO_EXTRADATA_SIZE) <
            (meta_buffer_virtual + drv_ctx.op_buf.meta_buffer_size)) {
        p_buf_hdr->nFlags |= OMX_BUFFERFLAG_EXTRADATA;

        /* vui extra data (frame_rate) information */
        if (h264_parser)
            h264_parser->get_frame_rate(&frame_rate);

        append_frame_info_extradata(p_extra, num_conceal_MB,
                ((struct vdec_output_frameinfo *)p_buf_hdr->pOutputPortPrivate)->pic_type,
                p_buf_hdr->nTimeStamp, frame_rate,
                &((struct vdec_output_frameinfo *)
                    p_buf_hdr->pOutputPortPrivate)->aspect_ratio_info);
        p_extra = (OMX_OTHER_EXTRADATATYPE *) (((OMX_U8 *) p_extra) + p_extra->nSize);
    }

    if ((client_extradata & OMX_PORTDEF_EXTRADATA) &&
            p_extra != NULL &&
            ((OMX_U8*)p_extra + OMX_PORTDEF_EXTRADATA_SIZE) <
            (meta_buffer_virtual + drv_ctx.op_buf.meta_buffer_size)) {
        p_buf_hdr->nFlags |= OMX_BUFFERFLAG_EXTRADATA;
        append_portdef_extradata(p_extra);
        p_extra = (OMX_OTHER_EXTRADATATYPE *) (((OMX_U8 *) p_extra) + p_extra->nSize);
    }

    if (p_buf_hdr->nFlags & OMX_BUFFERFLAG_EXTRADATA)
        if (p_extra &&
                ((OMX_U8*)p_extra + OMX_FRAMEINFO_EXTRADATA_SIZE) <
                (meta_buffer_virtual + drv_ctx.op_buf.meta_buffer_size))
            append_terminator_extradata(p_extra);
        else {
            DEBUG_PRINT_ERROR("ERROR: Terminator extradata cannot be added");
            p_buf_hdr->nFlags &= ~OMX_BUFFERFLAG_EXTRADATA;
        }
}

void omx_vdec::handle_extradata(OMX_BUFFERHEADERTYPE *p_buf_hdr)
{
    OMX_OTHER_EXTRADATATYPE *p_extra = NULL, *p_sei = NULL, *p_vui = NULL, *p_extn_user[32];
    OMX_U32 num_conceal_MB = 0;
    OMX_S64 ts_in_sei = 0;
    OMX_U32 frame_rate = 0;
    OMX_U32 extn_user_data_cnt = 0;

    OMX_U32 index = p_buf_hdr - m_out_mem_ptr;
    OMX_U8* pBuffer = (OMX_U8 *)drv_ctx.ptr_outputbuffer[index].bufferaddr;
    p_extra = (OMX_OTHER_EXTRADATATYPE *)
        ((unsigned)(pBuffer + p_buf_hdr->nOffset +
            p_buf_hdr->nFilledLen + 3)&(~3));

    if ((OMX_U8*)p_extra > (pBuffer + p_buf_hdr->nAllocLen))
        p_extra = NULL;

    if (drv_ctx.extradata && (p_buf_hdr->nFlags & OMX_BUFFERFLAG_EXTRADATA)) {
        // Process driver extradata
        while (p_extra && p_extra->eType != VDEC_EXTRADATA_NONE) {
            DEBUG_PRINT_LOW("handle_extradata : pBuf(%p) BufTS(%lld) Type(%x) DataSz(%u)",
                    p_buf_hdr, p_buf_hdr->nTimeStamp, p_extra->eType, p_extra->nDataSize);

            if (p_extra->nSize < p_extra->nDataSize) {
                DEBUG_PRINT_ERROR("Corrupt metadata Buffer size %d payload size %d",
                        p_extra->nSize, p_extra->nDataSize);
                p_extra = (OMX_OTHER_EXTRADATATYPE *) (((OMX_U8 *) p_extra) + p_extra->nSize);

                if ((OMX_U8*)p_extra > (pBuffer + p_buf_hdr->nAllocLen) ||
                        p_extra->nDataSize == 0 || p_extra->nSize == 0)
                    p_extra = NULL;

                continue;
            }

            if (p_extra->eType == VDEC_EXTRADATA_MB_ERROR_MAP) {
                if (client_extradata & OMX_FRAMEINFO_EXTRADATA)
                    num_conceal_MB = count_MB_in_extradata(p_extra);

                if (client_extradata & VDEC_EXTRADATA_MB_ERROR_MAP)
                    // Map driver extradata to corresponding OMX type
                    p_extra->eType = (OMX_EXTRADATATYPE)OMX_ExtraDataConcealMB;
                else
                    p_extra->eType = OMX_ExtraDataMax; // Invalid type to avoid expose this extradata to OMX client

#ifdef _ANDROID_

                if (m_debug_concealedmb) {
                    DEBUG_PRINT_HIGH("Concealed MB percentage is %u", num_conceal_MB);
                }

#endif /* _ANDROID_ */
            } else if (p_extra->eType == VDEC_EXTRADATA_SEI) {
                p_sei = p_extra;
#ifdef MAX_RES_1080P
                h264_parser->parse_nal((OMX_U8*)p_sei->data, p_sei->nDataSize, NALU_TYPE_SEI);
#endif
                p_extra->eType = OMX_ExtraDataMax; // Invalid type to avoid expose this extradata to OMX client
            } else if (p_extra->eType == VDEC_EXTRADATA_VUI) {
                p_vui = p_extra;
#ifdef MAX_RES_1080P
                h264_parser->parse_nal((OMX_U8*)p_vui->data, p_vui->nDataSize, NALU_TYPE_VUI, false);
#endif
                p_extra->eType = OMX_ExtraDataMax; // Invalid type to avoid expose this extradata to OMX client
            } else if (p_extra->eType == VDEC_EXTRADATA_EXT_DATA) {
                OMX_U8 *data_ptr = (OMX_U8*)p_extra->data;
                OMX_U32 value = 0;
                p_extn_user[extn_user_data_cnt] = p_extra;
                extn_user_data_cnt++;

                if ((*data_ptr & 0xf0) == 0x20) {
                    value = ((*data_ptr) & 0x01);
                    data_ptr++;

                    if (value)
                        data_ptr += 3;

                    value = *((OMX_U32*)data_ptr);
                    value = ((value << 24) | (((value >> 8)<<24)>>8) |
                            (((value >> 16)<<24)>>16) | (value >> 24));
                    m_disp_hor_size = (value & 0xfffc0000)>>18;
                    m_disp_vert_size = (value & 0x0001fff8)>>3;
                    DEBUG_PRINT_LOW("Display Vertical Size = %d Display Horizontal Size = %d",
                            m_disp_vert_size, m_disp_hor_size);
                }
            } else if (p_extra->eType == VDEC_EXTRADATA_USER_DATA) {
                p_extn_user[extn_user_data_cnt] = p_extra;
                extn_user_data_cnt++;
            }

            print_debug_extradata(p_extra);
            p_extra = (OMX_OTHER_EXTRADATATYPE *) (((OMX_U8 *) p_extra) + p_extra->nSize);

            if ((OMX_U8*)p_extra > (pBuffer + p_buf_hdr->nAllocLen) ||
                    p_extra->nDataSize == 0 || p_extra->nSize == 0)
                p_extra = NULL;
        }

        if (!(client_extradata & VDEC_EXTRADATA_MB_ERROR_MAP)) {
            // Driver extradata is only exposed if MB map is requested by client,
            // otherwise can be overwritten by omx extradata.
            p_extra = (OMX_OTHER_EXTRADATATYPE *)
                ((unsigned)(pBuffer + p_buf_hdr->nOffset +
                    p_buf_hdr->nFilledLen + 3)&(~3));
            p_buf_hdr->nFlags &= ~OMX_BUFFERFLAG_EXTRADATA;
        }
    }

#ifdef PROCESS_EXTRADATA_IN_OUTPUT_PORT

    if (drv_ctx.decoder_format == VDEC_CODECTYPE_H264) {
        if (client_extradata & OMX_TIMEINFO_EXTRADATA) {
            if (p_vui)
                h264_parser->parse_nal((OMX_U8*)p_vui->data, p_vui->nDataSize, NALU_TYPE_VUI, false);

            if (p_sei)
                h264_parser->parse_nal((OMX_U8*)p_sei->data, p_sei->nDataSize, NALU_TYPE_SEI);

            ts_in_sei = h264_parser->process_ts_with_sei_vui(p_buf_hdr->nTimeStamp);

            if (!VALID_TS(p_buf_hdr->nTimeStamp))
                p_buf_hdr->nTimeStamp = ts_in_sei;
        } else if ((client_extradata & OMX_FRAMEINFO_EXTRADATA) && p_sei)
            // If timeinfo is present frame info from SEI is already processed
            h264_parser->parse_nal((OMX_U8*)p_sei->data, p_sei->nDataSize, NALU_TYPE_SEI);
    }

#endif

    if (client_extradata & OMX_EXTNUSER_EXTRADATA && p_extra) {
        p_buf_hdr->nFlags |= OMX_BUFFERFLAG_EXTRADATA;

        for (int i = 0; i < extn_user_data_cnt; i++) {
            if (((OMX_U8*)p_extra + p_extn_user[i]->nSize) <
                    (pBuffer + p_buf_hdr->nAllocLen)) {
                if (p_extn_user[i]->eType == VDEC_EXTRADATA_EXT_DATA) {
                    append_extn_extradata(p_extra, p_extn_user[i]);
                    p_extra = (OMX_OTHER_EXTRADATATYPE *) (((OMX_U8 *) p_extra) + p_extra->nSize);
                } else if (p_extn_user[i]->eType == VDEC_EXTRADATA_USER_DATA) {
                    append_user_extradata(p_extra, p_extn_user[i]);
                    p_extra = (OMX_OTHER_EXTRADATATYPE *) (((OMX_U8 *) p_extra) + p_extra->nSize);
                }
            }
        }
    }

    if ((client_extradata & OMX_INTERLACE_EXTRADATA) && p_extra &&
            ((OMX_U8*)p_extra + OMX_INTERLACE_EXTRADATA_SIZE) <
            (pBuffer + p_buf_hdr->nAllocLen)) {
        p_buf_hdr->nFlags |= OMX_BUFFERFLAG_EXTRADATA;
        append_interlace_extradata(p_extra,
                ((struct vdec_output_frameinfo *)p_buf_hdr->pOutputPortPrivate)->interlaced_format, index);
        p_extra = (OMX_OTHER_EXTRADATATYPE *) (((OMX_U8 *) p_extra) + p_extra->nSize);
    }

    if (client_extradata & OMX_FRAMEINFO_EXTRADATA && p_extra &&
            ((OMX_U8*)p_extra + OMX_FRAMEINFO_EXTRADATA_SIZE) <
            (pBuffer + p_buf_hdr->nAllocLen)) {
        p_buf_hdr->nFlags |= OMX_BUFFERFLAG_EXTRADATA;

        /* vui extra data (frame_rate) information */
        if (h264_parser)
            h264_parser->get_frame_rate(&frame_rate);

        append_frame_info_extradata(p_extra, num_conceal_MB,
                ((struct vdec_output_frameinfo *)p_buf_hdr->pOutputPortPrivate)->pic_type,
                p_buf_hdr->nTimeStamp, frame_rate,
                &((struct vdec_output_frameinfo *)
                    p_buf_hdr->pOutputPortPrivate)->aspect_ratio_info);
        p_extra = (OMX_OTHER_EXTRADATATYPE *) (((OMX_U8 *) p_extra) + p_extra->nSize);
    }

    if ((client_extradata & OMX_PORTDEF_EXTRADATA) &&
            p_extra != NULL &&
            ((OMX_U8*)p_extra + OMX_PORTDEF_EXTRADATA_SIZE) <
            (pBuffer + p_buf_hdr->nAllocLen)) {
        p_buf_hdr->nFlags |= OMX_BUFFERFLAG_EXTRADATA;
        append_portdef_extradata(p_extra);
        p_extra = (OMX_OTHER_EXTRADATATYPE *) (((OMX_U8 *) p_extra) + p_extra->nSize);
    }

    if (p_buf_hdr->nFlags & OMX_BUFFERFLAG_EXTRADATA)
        if (p_extra &&
                ((OMX_U8*)p_extra + OMX_FRAMEINFO_EXTRADATA_SIZE) <
                (pBuffer + p_buf_hdr->nAllocLen))
            append_terminator_extradata(p_extra);
        else {
            DEBUG_PRINT_ERROR("ERROR: Terminator extradata cannot be added");
            p_buf_hdr->nFlags &= ~OMX_BUFFERFLAG_EXTRADATA;
        }
}

OMX_ERRORTYPE omx_vdec::enable_extradata(OMX_U32 requested_extradata, bool enable)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_U32 driver_extradata = 0, extradata_size = 0;
    struct vdec_ioctl_msg ioctl_msg = {NULL, NULL};

    if (m_state != OMX_StateLoaded) {
        DEBUG_PRINT_ERROR("ERROR: enable extradata allowed in Loaded state only");
        return OMX_ErrorIncorrectStateOperation;
    }

    if (requested_extradata & OMX_FRAMEINFO_EXTRADATA)
        extradata_size += OMX_FRAMEINFO_EXTRADATA_SIZE;

    if (requested_extradata & OMX_INTERLACE_EXTRADATA)
        extradata_size += OMX_INTERLACE_EXTRADATA_SIZE;

    if (requested_extradata & OMX_PORTDEF_EXTRADATA) {
        extradata_size += OMX_PORTDEF_EXTRADATA_SIZE;
    }

    DEBUG_PRINT_ERROR("enable_extradata: actual[%x] requested[%x] enable[%d]",
            client_extradata, requested_extradata, enable);

    if (enable)
        requested_extradata |= client_extradata;
    else {
        requested_extradata = client_extradata & ~requested_extradata;
        extradata_size *= -1;
    }

    driver_extradata = requested_extradata & DRIVER_EXTRADATA_MASK;

    if (secure_mode)
        driver_extradata = driver_extradata | VDEC_EXTRADATA_EXT_BUFFER;

    if (requested_extradata & OMX_FRAMEINFO_EXTRADATA)
        driver_extradata |= VDEC_EXTRADATA_MB_ERROR_MAP; // Required for conceal MB frame info

    if (drv_ctx.decoder_format == VDEC_CODECTYPE_MPEG2) {
        driver_extradata |= ((requested_extradata & OMX_EXTNUSER_EXTRADATA)?
                VDEC_EXTRADATA_EXT_DATA | VDEC_EXTRADATA_USER_DATA : 0);
    }

#ifdef PROCESS_EXTRADATA_IN_OUTPUT_PORT

    if (drv_ctx.decoder_format == VDEC_CODECTYPE_H264) {
        driver_extradata |= ((requested_extradata & OMX_FRAMEINFO_EXTRADATA)?
                VDEC_EXTRADATA_SEI : 0); // Required for pan scan frame info
        driver_extradata |= ((requested_extradata & OMX_TIMEINFO_EXTRADATA)?
                VDEC_EXTRADATA_VUI | VDEC_EXTRADATA_SEI : 0); //Required for time info
    }

#endif

    if (driver_extradata != drv_ctx.extradata) {
        client_extradata = requested_extradata;
        drv_ctx.extradata = driver_extradata;
        ioctl_msg.in = &drv_ctx.extradata;
        ioctl_msg.out = NULL;

        if (ioctl(drv_ctx.video_driver_fd, VDEC_IOCTL_SET_EXTRADATA,
                    (void*)&ioctl_msg) < 0) {
            DEBUG_PRINT_ERROR("Set extradata failed");
            ret = OMX_ErrorUnsupportedSetting;
        } else
            ret = get_buffer_req(&drv_ctx.op_buf);
    } else if ((client_extradata & ~DRIVER_EXTRADATA_MASK) != (requested_extradata & ~DRIVER_EXTRADATA_MASK)) {
        client_extradata = requested_extradata;
        drv_ctx.op_buf.buffer_size += extradata_size;
        // align the buffer size
        drv_ctx.op_buf.buffer_size = (drv_ctx.op_buf.buffer_size + drv_ctx.op_buf.alignment - 1)&(~(drv_ctx.op_buf.alignment - 1));
        DEBUG_PRINT_LOW("Aligned buffer size with exreadata = %d", drv_ctx.op_buf.buffer_size);

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

    while (byte_count < extra->nDataSize) {
        data = *data_ptr;

        while (data) {
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
#ifdef _ANDROID_

    if (!m_debug_extradata)
        return;

    DEBUG_PRINT_HIGH(
            "============== Extra Data =============="
            "           Size: %u"
            "        Version: %u"
            "      PortIndex: %u"
            "           Type: %x"
            "       DataSize: %u",
            extra->nSize, extra->nVersion.nVersion,
            extra->nPortIndex, extra->eType, extra->nDataSize);

    if (extra->eType == OMX_ExtraDataInterlaceFormat) {
        OMX_STREAMINTERLACEFORMAT *intfmt = (OMX_STREAMINTERLACEFORMAT *)extra->data;
        DEBUG_PRINT_HIGH(
                "------ Interlace Format ------"
                "                Size: %u"
                "             Version: %u"
                "           PortIndex: %u"
                " Is Interlace Format: %u"
                "   Interlace Formats: %u"
                "=========== End of Interlace ===========",
                intfmt->nSize, intfmt->nVersion.nVersion, intfmt->nPortIndex,
                intfmt->bInterlaceFormat, intfmt->nInterlaceFormats);
    } else if (extra->eType == OMX_ExtraDataFrameInfo) {
        OMX_QCOM_EXTRADATA_FRAMEINFO *fminfo = (OMX_QCOM_EXTRADATA_FRAMEINFO *)extra->data;

        DEBUG_PRINT_HIGH(
                "-------- Frame Format --------"
                "             Picture Type: %u"
                "           Interlace Type: %u"
                " Pan Scan Total Frame Num: %u"
                "   Concealed Macro Blocks: %u"
                "               frame rate: %u"
                "           Aspect Ratio X: %u"
                "           Aspect Ratio Y: %u",
                fminfo->ePicType,
                fminfo->interlaceType,
                fminfo->panScan.numWindows,
                fminfo->nConcealedMacroblocks,
                fminfo->nFrameRate,
                fminfo->aspectRatio.aspectRatioX,
                fminfo->aspectRatio.aspectRatioY);

        for (int i = 0; i < fminfo->panScan.numWindows; i++) {
            DEBUG_PRINT_HIGH(
                    "------------------------------"
                    "     Pan Scan Frame Num: %d"
                    "            Rectangle x: %d"
                    "            Rectangle y: %d"
                    "           Rectangle dx: %d"
                    "           Rectangle dy: %d",
                    i, fminfo->panScan.window[i].x, fminfo->panScan.window[i].y,
                    fminfo->panScan.window[i].dx, fminfo->panScan.window[i].dy);
        }

        DEBUG_PRINT_HIGH("========= End of Frame Format ==========");
    } else if (extra->eType == OMX_ExtraDataNone) {
        DEBUG_PRINT_HIGH("========== End of Terminator ===========");
    } else {
        DEBUG_PRINT_HIGH("======= End of Driver Extradata ========");
    }

#endif /* _ANDROID_ */
}

void omx_vdec::append_interlace_extradata(OMX_OTHER_EXTRADATATYPE *extra,
        OMX_U32 interlaced_format_type, OMX_U32 buf_index)
{
    OMX_STREAMINTERLACEFORMAT *interlace_format;
    OMX_U32 mbaff = 0;
#if defined(_ANDROID_ICS_)
    OMX_U32 enable = 0;
    private_handle_t *handle = NULL;
    handle = (private_handle_t *)native_buffer[buf_index].nativehandle;

    if (!handle)
        DEBUG_PRINT_LOW("%s: Native Buffer handle is NULL",__func__);

#endif
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

    if ((interlaced_format_type == VDEC_InterlaceFrameProgressive)  && !mbaff) {
        interlace_format->bInterlaceFormat = OMX_FALSE;
        interlace_format->nInterlaceFormats = OMX_InterlaceFrameProgressive;
        drv_ctx.interlace = VDEC_InterlaceFrameProgressive;
#if defined(_ANDROID_ICS_)

        if (handle) {
            setMetaData(handle, PP_PARAM_INTERLACED, (void*)&enable);
        }

#endif
    } else {
        interlace_format->bInterlaceFormat = OMX_TRUE;
        interlace_format->nInterlaceFormats = OMX_InterlaceInterleaveFrameTopFieldFirst;
        drv_ctx.interlace = VDEC_InterlaceInterleaveFrameTopFieldFirst;
#if defined(_ANDROID_ICS_)
        enable = 1;

        if (handle) {
            setMetaData(handle, PP_PARAM_INTERLACED, (void*)&enable);
        }

#endif
    }

    print_debug_extradata(extra);
}

void omx_vdec::append_frame_info_extradata(OMX_OTHER_EXTRADATATYPE *extra,
        OMX_U32 num_conceal_mb, OMX_U32 picture_type, OMX_S64 timestamp,
        OMX_U32 frame_rate, struct vdec_aspectratioinfo *aspect_ratio_info)
{
    OMX_QCOM_EXTRADATA_FRAMEINFO *frame_info = NULL;
    extra->nSize = OMX_FRAMEINFO_EXTRADATA_SIZE;
    extra->nVersion.nVersion = OMX_SPEC_VERSION;
    extra->nPortIndex = OMX_CORE_OUTPUT_PORT_INDEX;
    extra->eType = (OMX_EXTRADATATYPE)OMX_ExtraDataFrameInfo;
    extra->nDataSize = sizeof(OMX_QCOM_EXTRADATA_FRAMEINFO);
    frame_info = (OMX_QCOM_EXTRADATA_FRAMEINFO *)extra->data;

    switch (picture_type) {
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
    memset(&frame_info->displayAspectRatio, 0, sizeof(frame_info->displayAspectRatio));

    if (drv_ctx.decoder_format == VDEC_CODECTYPE_H264) {
        h264_parser->fill_pan_scan_data(&frame_info->panScan, timestamp);
    }

    fill_aspect_ratio_info(aspect_ratio_info, frame_info);

    if (drv_ctx.decoder_format == VDEC_CODECTYPE_MPEG2) {
        if (m_disp_hor_size && m_disp_vert_size) {
            frame_info->displayAspectRatio.displayHorizontalSize = m_disp_hor_size;
            frame_info->displayAspectRatio.displayVerticalSize = m_disp_vert_size;
        }
    }

    frame_info->nConcealedMacroblocks = num_conceal_mb;
    frame_info->nFrameRate = frame_rate;
    print_debug_extradata(extra);
}

void omx_vdec::fill_aspect_ratio_info(
        struct vdec_aspectratioinfo *aspect_ratio_info,
        OMX_QCOM_EXTRADATA_FRAMEINFO *frame_info)
{
    m_extradata = frame_info;

    if (drv_ctx.decoder_format == VDEC_CODECTYPE_MPEG2) {
        switch (aspect_ratio_info->aspect_ratio) {
            case 1:
                m_extradata->aspectRatio.aspectRatioX = 1;
                m_extradata->aspectRatio.aspectRatioY = 1;
                break;
            case 2:

                if (m_disp_hor_size && m_disp_vert_size) {
                    m_extradata->aspectRatio.aspectRatioX = aspect_ratio_info->par_width *
                        m_disp_vert_size;
                    m_extradata->aspectRatio.aspectRatioY = aspect_ratio_info->par_height *
                        m_disp_hor_size;
                } else {
                    m_extradata->aspectRatio.aspectRatioX = aspect_ratio_info->par_width *
                        drv_ctx.video_resolution.frame_height;
                    m_extradata->aspectRatio.aspectRatioY = aspect_ratio_info->par_height *
                        drv_ctx.video_resolution.frame_width;
                }

                break;
            case 3:

                if (m_disp_hor_size && m_disp_vert_size) {
                    m_extradata->aspectRatio.aspectRatioX = aspect_ratio_info->par_width *
                        m_disp_vert_size;
                    m_extradata->aspectRatio.aspectRatioY = aspect_ratio_info->par_height *
                        m_disp_hor_size;
                } else {
                    m_extradata->aspectRatio.aspectRatioX = aspect_ratio_info->par_width *
                        drv_ctx.video_resolution.frame_height;
                    m_extradata->aspectRatio.aspectRatioY = aspect_ratio_info->par_height *
                        drv_ctx.video_resolution.frame_width;
                }

                break;
            case 4:

                if (m_disp_hor_size && m_disp_vert_size) {
                    m_extradata->aspectRatio.aspectRatioX = aspect_ratio_info->par_width *
                        m_disp_vert_size;
                    m_extradata->aspectRatio.aspectRatioY = aspect_ratio_info->par_height *
                        m_disp_hor_size;
                } else {
                    m_extradata->aspectRatio.aspectRatioX = aspect_ratio_info->par_width *
                        drv_ctx.video_resolution.frame_height;
                    m_extradata->aspectRatio.aspectRatioY = aspect_ratio_info->par_height *
                        drv_ctx.video_resolution.frame_width;
                }

                break;
            default:
                m_extradata->aspectRatio.aspectRatioX = 1;
                m_extradata->aspectRatio.aspectRatioY = 1;
        }
    } else {
        m_extradata->aspectRatio.aspectRatioX = aspect_ratio_info->par_width;
        m_extradata->aspectRatio.aspectRatioY = aspect_ratio_info->par_height;
    }

    DEBUG_PRINT_LOW("aspectRatioX %d aspectRatioX %d", m_extradata->aspectRatio.aspectRatioX,
            m_extradata->aspectRatio.aspectRatioY);
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
            "sliceheight = %u",portDefn->format.video.nFrameHeight,
            portDefn->format.video.nFrameWidth,
            portDefn->format.video.nStride,
            portDefn->format.video.nSliceHeight);
}

void omx_vdec::append_extn_extradata(OMX_OTHER_EXTRADATATYPE *extra,
        OMX_OTHER_EXTRADATATYPE *p_extn)
{
    extra->nDataSize = p_extn->nDataSize;
    extra->nSize = (sizeof(OMX_OTHER_EXTRADATATYPE) + extra->nDataSize);
    extra->nVersion.nVersion = OMX_SPEC_VERSION;
    extra->nPortIndex = OMX_CORE_OUTPUT_PORT_INDEX;
    extra->eType = (OMX_EXTRADATATYPE)OMX_ExtraDataMP2ExtnData;

    if (extra->data && p_extn->data && extra->nDataSize)
        memcpy(extra->data, p_extn->data, extra->nDataSize);
}

void omx_vdec::append_user_extradata(OMX_OTHER_EXTRADATATYPE *extra,
        OMX_OTHER_EXTRADATATYPE *p_user)
{
    extra->nDataSize = p_user->nDataSize;
    extra->nSize = (sizeof(OMX_OTHER_EXTRADATATYPE) + extra->nDataSize);
    extra->nVersion.nVersion = OMX_SPEC_VERSION;
    extra->nPortIndex = OMX_CORE_OUTPUT_PORT_INDEX;
    extra->eType = (OMX_EXTRADATATYPE)OMX_ExtraDataMP2UserData;

    if (extra->data && p_user->data && extra->nDataSize)
        memcpy(extra->data, p_user->data, extra->nDataSize);
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

    if (index >= drv_ctx.ip_buf.actualcount) {
        DEBUG_PRINT_ERROR("ERROR:Desc Buffer Index not found");
        return OMX_ErrorInsufficientResources;
    }

    if (m_desc_buffer_ptr == NULL) {
        m_desc_buffer_ptr = (desc_buffer_hdr*) \
                            calloc( (sizeof(desc_buffer_hdr)),
                                    drv_ctx.ip_buf.actualcount);

        if (m_desc_buffer_ptr == NULL) {
            DEBUG_PRINT_ERROR("m_desc_buffer_ptr Allocation failed ");
            return OMX_ErrorInsufficientResources;
        }
    }

    m_desc_buffer_ptr[index].buf_addr = (unsigned char *)malloc (DESC_BUFFER_SIZE * sizeof(OMX_U8));

    if (m_desc_buffer_ptr[index].buf_addr == NULL) {
        DEBUG_PRINT_ERROR("desc buffer Allocation failed ");
        return OMX_ErrorInsufficientResources;
    }

    return eRet;
}

void omx_vdec::insert_demux_addr_offset(OMX_U32 address_offset)
{
    DEBUG_PRINT_LOW("Inserting address offset (%d) at idx (%d)", address_offset,m_demux_entries);

    if (m_demux_entries < 8192) {
        m_demux_offsets[m_demux_entries++] = address_offset;
    }

    return;
}

void omx_vdec::extract_demux_addr_offsets(OMX_BUFFERHEADERTYPE *buf_hdr)
{
    OMX_U32 bytes_to_parse = buf_hdr->nFilledLen;
    OMX_U8 *buf = buf_hdr->pBuffer + buf_hdr->nOffset;
    OMX_U32 index = 0;
    OMX_U32 prev_sc_index = 0;

    m_demux_entries = 0;

    while (index < bytes_to_parse) {
        if ( ((buf[index] == 0x00) && (buf[index+1] == 0x00) &&
                    (buf[index+2] == 0x00) && (buf[index+3] == 0x01)) ||
                ((buf[index] == 0x00) && (buf[index+1] == 0x00) &&
                 (buf[index+2] == 0x01)) ) {
            if ((((index+3) - prev_sc_index) <= 4) && m_demux_entries) {
                DEBUG_PRINT_ERROR("FOUND Consecutive start Code, Hence skip one");
                m_demux_entries--;
            }

            //Found start code, insert address offset
            insert_demux_addr_offset(index);

            if (buf[index+2] == 0x01) // 3 byte start code
                index += 3;
            else                      //4 byte start code
                index += 4;

            prev_sc_index = index;
        } else
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

    if (m_desc_buffer_ptr == NULL) {
        DEBUG_PRINT_ERROR("m_desc_buffer_ptr is NULL. Cannot append demux entries.");
        return OMX_ErrorBadParameter;
    }

    buffer_index = p_buf_hdr - ((OMX_BUFFERHEADERTYPE *)m_inp_mem_ptr);

    if (buffer_index > drv_ctx.ip_buf.actualcount) {
        DEBUG_PRINT_ERROR("handle_demux_data:Buffer index is incorrect (%d)", buffer_index);
        return OMX_ErrorBadParameter;
    }

    p_demux_data = (OMX_U8 *) m_desc_buffer_ptr[buffer_index].buf_addr;

    if ( ((OMX_U8*)p_demux_data == NULL) ||
            ((m_demux_entries * 16) + 1) > DESC_BUFFER_SIZE) {
        DEBUG_PRINT_ERROR("Insufficient buffer. Cannot append demux entries.");
        return OMX_ErrorBadParameter;
    } else {
        for (; demux_index < m_demux_entries; demux_index++) {
            desc_data = 0;
            start_addr = m_demux_offsets[demux_index];

            if (p_buf_hdr->pBuffer[m_demux_offsets[demux_index] + 2] == 0x01) {
                suffix_byte = p_buf_hdr->pBuffer[m_demux_offsets[demux_index] + 3];
            } else {
                suffix_byte = p_buf_hdr->pBuffer[m_demux_offsets[demux_index] + 4];
            }

            if (demux_index < (m_demux_entries - 1)) {
                nal_size = m_demux_offsets[demux_index + 1] - m_demux_offsets[demux_index] - 2;
            } else {
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

        if (codec_type_parse == CODEC_TYPE_VC1) {
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

#ifdef MAX_RES_1080P

OMX_ERRORTYPE omx_vdec::vdec_alloc_meta_buffers()
{
    OMX_U32 pmem_fd = -1, pmem_fd_iommu = -1;
    OMX_U32 size, alignment;
    void *buf_addr = NULL;
    struct vdec_ioctl_msg ioctl_msg;
#ifndef USE_ION
    struct pmem_allocation allocation;
#endif
    struct vdec_meta_buffers meta_buffer;

    memset ((unsigned char*)&meta_buffer,0,sizeof (struct vdec_meta_buffers));

    //we already have meta buffer size.
    size = drv_ctx.op_buf.meta_buffer_size * drv_ctx.op_buf.actualcount;
    alignment = 8192;


#ifdef USE_ION
    external_meta_buffer = true;
    drv_ctx.meta_buffer.ion_device_fd = alloc_map_ion_memory(
            size, 8192,
            &drv_ctx.meta_buffer.ion_alloc_data,
            &drv_ctx.meta_buffer.fd_ion_data, 0);

    if (drv_ctx.meta_buffer.ion_device_fd < 0) {
        external_meta_buffer = false;
        return OMX_ErrorInsufficientResources;
    }

    external_meta_buffer = false;
    pmem_fd = drv_ctx.meta_buffer.fd_ion_data.fd;

    external_meta_buffer_iommu = true;
    drv_ctx.meta_buffer_iommu.ion_device_fd = alloc_map_ion_memory(
            size, 8192,
            &drv_ctx.meta_buffer_iommu.ion_alloc_data,
            &drv_ctx.meta_buffer_iommu.fd_ion_data, 0);

    if (drv_ctx.meta_buffer_iommu.ion_device_fd < 0) {
        external_meta_buffer_iommu = false;
        return OMX_ErrorInsufficientResources;
    }

    external_meta_buffer_iommu = false;
    pmem_fd_iommu = drv_ctx.meta_buffer_iommu.fd_ion_data.fd;
#else
    allocation.size = size;
    allocation.align = clip2(alignment);

    if (allocation.align != 8192)
        allocation.align = 8192;

    pmem_fd = open(MEM_DEVICE, O_RDWR);

    if ((int)(pmem_fd) < 0)
        return OMX_ErrorInsufficientResources;

    if (ioctl(pmem_fd, PMEM_ALLOCATE_ALIGNED, &allocation) < 0) {
        DEBUG_PRINT_ERROR("Aligment(%u) failed with pmem driver Sz(%lu)",
                allocation.align, allocation.size);
        return OMX_ErrorInsufficientResources;
    }

#endif

    buf_addr = mmap(NULL, size,
            PROT_READ | PROT_WRITE,
            MAP_SHARED, pmem_fd_iommu, 0);

    if (buf_addr == (void*) MAP_FAILED) {
        close(pmem_fd);
        close(pmem_fd_iommu);
#ifdef USE_ION
        free_ion_memory(&drv_ctx.meta_buffer);
        free_ion_memory(&drv_ctx.meta_buffer_iommu);
#endif
        pmem_fd = -1;
        pmem_fd_iommu = -1;
        DEBUG_PRINT_ERROR("Error returned in allocating recon buffers buf_addr: %p",buf_addr);
        return OMX_ErrorInsufficientResources;
    }

    meta_buffer.size = size;
    meta_buffer.count = drv_ctx.op_buf.actualcount;
    meta_buffer.pmem_fd = pmem_fd;
    meta_buffer.pmem_fd_iommu = pmem_fd_iommu;
    meta_buffer.offset = 0;

    ioctl_msg.in = (void*)&meta_buffer;
    ioctl_msg.out = NULL;

    if (ioctl (drv_ctx.video_driver_fd,VDEC_IOCTL_SET_META_BUFFERS, (void*)&ioctl_msg) < 0) {
        DEBUG_PRINT_ERROR("Failed to set the meta_buffers");
        return OMX_ErrorInsufficientResources;
    }

    meta_buff.buffer = (unsigned char *) buf_addr;
    meta_buff.size = size;
    meta_buff.count = drv_ctx.op_buf.actualcount;
    meta_buff.offset = 0;
    meta_buff.pmem_fd = pmem_fd;
    meta_buff.pmem_fd_iommu = pmem_fd_iommu;
    DEBUG_PRINT_HIGH("Saving virt:%p, FD: %d and FD_IOMMU %d of size %d count: %d", meta_buff.buffer,
            meta_buff.pmem_fd, meta_buff.pmem_fd_iommu, meta_buff.size, drv_ctx.op_buf.actualcount);
    return OMX_ErrorNone;
}

void omx_vdec::vdec_dealloc_meta_buffers()
{
    if (meta_buff.pmem_fd > 0) {
        if (ioctl(drv_ctx.video_driver_fd, VDEC_IOCTL_FREE_META_BUFFERS,NULL) < 0)
            DEBUG_PRINT_ERROR("VDEC_IOCTL_FREE_META_BUFFERS failed");

        close(meta_buff.pmem_fd);
#ifdef USE_ION
        free_ion_memory(&drv_ctx.meta_buffer);
#endif
    }

    if (meta_buff.pmem_fd_iommu > 0) {
        munmap(meta_buff.buffer, meta_buff.size);
        close(meta_buff.pmem_fd_iommu);
#ifdef USE_ION
        free_ion_memory(&drv_ctx.meta_buffer_iommu);
#endif
        DEBUG_PRINT_LOW("Cleaning Meta buffer of size %d",meta_buff.size);
        meta_buff.pmem_fd = -1;
        meta_buff.pmem_fd_iommu = -1;
        meta_buff.offset = 0;
        meta_buff.size = 0;
        meta_buff.count = 0;
        meta_buff.buffer = NULL;
    }
}

OMX_ERRORTYPE omx_vdec::vdec_alloc_h264_mv()
{
    OMX_U32 pmem_fd = -1;
    OMX_U32 width, height, size, alignment;
    void *buf_addr = NULL;
    struct vdec_ioctl_msg ioctl_msg;
#ifndef USE_ION
    struct pmem_allocation allocation;
#endif
    struct vdec_h264_mv h264_mv;
    struct vdec_mv_buff_size mv_buff_size;

    mv_buff_size.width = drv_ctx.video_resolution.stride;
    mv_buff_size.height = drv_ctx.video_resolution.scan_lines>>2;

    ioctl_msg.in = NULL;
    ioctl_msg.out = (void*)&mv_buff_size;

    if (ioctl (drv_ctx.video_driver_fd,VDEC_IOCTL_GET_MV_BUFFER_SIZE, (void*)&ioctl_msg) < 0) {
        DEBUG_PRINT_ERROR("GET_MV_BUFFER_SIZE Failed for width: %d, Height %d" ,
                mv_buff_size.width, mv_buff_size.height);
        return OMX_ErrorInsufficientResources;
    }

    DEBUG_PRINT_ERROR("GET_MV_BUFFER_SIZE returned: Size: %d and alignment: %d",
            mv_buff_size.size, mv_buff_size.alignment);

    size = mv_buff_size.size * drv_ctx.op_buf.actualcount;
    alignment = mv_buff_size.alignment;

    DEBUG_PRINT_LOW("Entered vdec_alloc_h264_mv act_width: %d, act_height: %d, size: %d, alignment %d",
            drv_ctx.video_resolution.frame_width, drv_ctx.video_resolution.frame_height,size,alignment);


#ifdef USE_ION
    drv_ctx.h264_mv.ion_device_fd = alloc_map_ion_memory(
            size, 8192,
            &drv_ctx.h264_mv.ion_alloc_data,
            &drv_ctx.h264_mv.fd_ion_data,ION_FLAG_CACHED);

    if (drv_ctx.h264_mv.ion_device_fd < 0) {
        return OMX_ErrorInsufficientResources;
    }

    pmem_fd = drv_ctx.h264_mv.fd_ion_data.fd;
#else
    allocation.size = size;
    allocation.align = clip2(alignment);

    if (allocation.align != 8192)
        allocation.align = 8192;

    pmem_fd = open(MEM_DEVICE, O_RDWR);

    if ((int)(pmem_fd) < 0)
        return OMX_ErrorInsufficientResources;

    if (ioctl(pmem_fd, PMEM_ALLOCATE_ALIGNED, &allocation) < 0) {
        DEBUG_PRINT_ERROR("Aligment(%u) failed with pmem driver Sz(%lu)",
                allocation.align, allocation.size);
        return OMX_ErrorInsufficientResources;
    }

#endif

    if (!secure_mode) {
        buf_addr = mmap(NULL, size,
                PROT_READ | PROT_WRITE,
                MAP_SHARED, pmem_fd, 0);

        if (buf_addr == (void*) MAP_FAILED) {
            close(pmem_fd);
#ifdef USE_ION
            free_ion_memory(&drv_ctx.h264_mv);
#endif
            pmem_fd = -1;
            DEBUG_PRINT_ERROR("Error returned in allocating recon buffers buf_addr: %p",buf_addr);
            return OMX_ErrorInsufficientResources;
        }
    } else
        buf_addr =(unsigned char *) (pmem_fd + 1234);

    DEBUG_PRINT_LOW("Allocated virt:%p, FD: %d of size %d count: %d", buf_addr,
            pmem_fd, size, drv_ctx.op_buf.actualcount);

    h264_mv.size = size;
    h264_mv.count = drv_ctx.op_buf.actualcount;
    h264_mv.pmem_fd = pmem_fd;
    h264_mv.offset = 0;

    ioctl_msg.in = (void*)&h264_mv;
    ioctl_msg.out = NULL;

    if (ioctl (drv_ctx.video_driver_fd,VDEC_IOCTL_SET_H264_MV_BUFFER, (void*)&ioctl_msg) < 0) {
        DEBUG_PRINT_ERROR("Failed to set the H264_mv_buffers");
        return OMX_ErrorInsufficientResources;
    }

    h264_mv_buff.buffer = (unsigned char *) buf_addr;
    h264_mv_buff.size = size;
    h264_mv_buff.count = drv_ctx.op_buf.actualcount;
    h264_mv_buff.offset = 0;
    h264_mv_buff.pmem_fd = pmem_fd;
    DEBUG_PRINT_LOW("Saving virt:%p, FD: %d of size %d count: %d", h264_mv_buff.buffer,
            h264_mv_buff.pmem_fd, h264_mv_buff.size, drv_ctx.op_buf.actualcount);
    return OMX_ErrorNone;
}

void omx_vdec::vdec_dealloc_h264_mv()
{
    if (h264_mv_buff.pmem_fd > 0) {
        if (ioctl(drv_ctx.video_driver_fd, VDEC_IOCTL_FREE_H264_MV_BUFFER,NULL) < 0)
            DEBUG_PRINT_ERROR("VDEC_IOCTL_FREE_H264_MV_BUFFER failed");

        if (!secure_mode)
            munmap(h264_mv_buff.buffer, h264_mv_buff.size);

        close(h264_mv_buff.pmem_fd);
#ifdef USE_ION
        free_ion_memory(&drv_ctx.h264_mv);
#endif
        DEBUG_PRINT_LOW("Cleaning H264_MV buffer of size %d",h264_mv_buff.size);
        h264_mv_buff.pmem_fd = -1;
        h264_mv_buff.offset = 0;
        h264_mv_buff.size = 0;
        h264_mv_buff.count = 0;
        h264_mv_buff.buffer = NULL;
    }
}

#endif

#ifdef _ANDROID_
OMX_ERRORTYPE omx_vdec::createDivxDrmContext()
{
    OMX_ERRORTYPE err = OMX_ErrorNone;
    iDivXDrmDecrypt = DivXDrmDecrypt::Create();

    if (iDivXDrmDecrypt) {
        OMX_ERRORTYPE err = iDivXDrmDecrypt->Init();

        if (err!=OMX_ErrorNone) {
            DEBUG_PRINT_ERROR("ERROR :iDivXDrmDecrypt->Init %d", err);
            delete iDivXDrmDecrypt;
            iDivXDrmDecrypt = NULL;
        }
    } else {
        DEBUG_PRINT_ERROR("Unable to Create DIVX DRM");
        err = OMX_ErrorUndefined;
    }

    return err;
}
#endif //_ANDROID_

OMX_ERRORTYPE omx_vdec::power_module_register()
{
    char powerHintMetadata[512];

    if (m_power_hinted) {
        return OMX_ErrorBadParameter; //need a proper error code
    }

    PowerModule * pm = PowerModule::getInstance();

    if (pm == NULL) {
        DEBUG_PRINT_ERROR("failed to get power module instance");
        return OMX_ErrorBadParameter;
    }

    power_module_t * ph = pm->getPowerModuleHandle();

    if (ph == NULL) {
        DEBUG_PRINT_ERROR("failed to get power module handle");
        return OMX_ErrorBadParameter;
    }

    if (ph->powerHint) {
        snprintf(powerHintMetadata, sizeof(powerHintMetadata) - 1,
                "state=1;framewidth=%u;frameheight=%u;bitrate=%u",
                m_port_def.format.video.nFrameWidth, m_port_def.format.video.nFrameHeight,
                m_port_def.format.video.nBitrate);
        powerHintMetadata[sizeof(powerHintMetadata) - 1] = '\0';

        ph->powerHint(ph, POWER_HINT_VIDEO_DECODE, (void *)powerHintMetadata);
        m_power_hinted = true;
    } else {
        DEBUG_PRINT_ERROR("No hint called for register");
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE omx_vdec::power_module_deregister()
{
    if (!m_power_hinted) {
        return OMX_ErrorBadParameter; //need a proper error code
    }

    PowerModule * pm = PowerModule::getInstance();

    if (pm == NULL) {
        DEBUG_PRINT_ERROR("failed to get power module instance");
        return OMX_ErrorBadParameter;
    }

    power_module_t * ph = pm->getPowerModuleHandle();

    if (ph == NULL) {
        DEBUG_PRINT_ERROR("failed to get power module handle");
        return OMX_ErrorBadParameter;
    }

    if (ph->powerHint) {
        ph->powerHint(ph, POWER_HINT_VIDEO_DECODE, (void *)"state=0");
        m_power_hinted = false;
    } else {
        DEBUG_PRINT_ERROR("No hint called for deregister");
    }

    return OMX_ErrorNone;
}
omx_vdec::allocate_color_convert_buf::allocate_color_convert_buf()
{
    enabled = false;
    omx = NULL;
    init_members();
    ColorFormat = OMX_COLOR_FormatMax;
}

void omx_vdec::allocate_color_convert_buf::set_vdec_client(void *client)
{
    omx = reinterpret_cast<omx_vdec*>(client);
}

void omx_vdec::allocate_color_convert_buf::init_members()
{
    allocated_count = 0;
    buffer_size_req = 0;
    buffer_alignment_req = 0;
    memset(m_platform_list_client,0,sizeof(m_platform_list_client));
    memset(m_platform_entry_client,0,sizeof(m_platform_entry_client));
    memset(m_pmem_info_client,0,sizeof(m_pmem_info_client));
    memset(m_out_mem_ptr_client,0,sizeof(m_out_mem_ptr_client));
    memset(op_buf_ion_info,0,sizeof(m_platform_entry_client));

    for (int i = 0; i < MAX_COUNT; i++)
        pmem_fd[i] = -1;
}

omx_vdec::allocate_color_convert_buf::~allocate_color_convert_buf()
{
    c2d.destroy();
}

bool omx_vdec::allocate_color_convert_buf::update_buffer_req()
{
    bool status = true;
    unsigned int src_size = 0, destination_size = 0;
    OMX_COLOR_FORMATTYPE drv_color_format;

    if (!omx) {
        DEBUG_PRINT_ERROR("Invalid client in color convert");
        return false;
    }

    if (!enabled) {
        DEBUG_PRINT_ERROR("No color conversion required");
        return status;
    }

    if (omx->drv_ctx.output_format != VDEC_YUV_FORMAT_TILE_4x2 &&
            ColorFormat != OMX_COLOR_FormatYUV420Planar) {
        DEBUG_PRINT_ERROR("update_buffer_req: Unsupported color conversion");
        return false;
    }

    c2d.close();
    status = c2d.open(omx->drv_ctx.video_resolution.frame_height,
            omx->drv_ctx.video_resolution.frame_width,
            YCbCr420Tile,YCbCr420P);

    if (status) {
        status = c2d.get_buffer_size(C2D_INPUT,src_size);

        if (status)
            status = c2d.get_buffer_size(C2D_OUTPUT,destination_size);
    }

    if (status) {
        if (!src_size || src_size > omx->drv_ctx.op_buf.buffer_size ||
                !destination_size) {
            DEBUG_PRINT_ERROR("ERROR: Size mismatch in C2D src_size %d"
                    "driver size %d destination size %d",
                    src_size,omx->drv_ctx.op_buf.buffer_size,destination_size);
            status = false;
            c2d.close();
            buffer_size_req = 0;
        } else {
            buffer_size_req = destination_size;

            if (buffer_size_req < omx->drv_ctx.op_buf.buffer_size)
                buffer_size_req = omx->drv_ctx.op_buf.buffer_size;

            if (buffer_alignment_req < omx->drv_ctx.op_buf.alignment)
                buffer_alignment_req = omx->drv_ctx.op_buf.alignment;
        }
    }

    return status;
}

bool omx_vdec::allocate_color_convert_buf::set_color_format(
        OMX_COLOR_FORMATTYPE dest_color_format)
{
    bool status = true;
    OMX_COLOR_FORMATTYPE drv_color_format;

    if (!omx) {
        DEBUG_PRINT_ERROR("Invalid client in color convert");
        return false;
    }

    if (omx->drv_ctx.output_format == VDEC_YUV_FORMAT_TILE_4x2)
        drv_color_format = (OMX_COLOR_FORMATTYPE)
            QOMX_COLOR_FormatYUV420PackedSemiPlanar64x32Tile2m8ka;
    else {
        DEBUG_PRINT_ERROR("Incorrect color format");
        status = false;
    }

    if (status && (drv_color_format != dest_color_format)) {
        DEBUG_PRINT_ERROR("");

        if (dest_color_format != OMX_COLOR_FormatYUV420Planar) {
            DEBUG_PRINT_ERROR("Unsupported color format for c2d");
            status = false;
        } else {
            ColorFormat = OMX_COLOR_FormatYUV420Planar;

            if (enabled)
                c2d.destroy();

            enabled = false;

            if (!c2d.init()) {
                DEBUG_PRINT_ERROR("open failed for c2d");
                status = false;
            } else
                enabled = true;
        }
    } else {
        if (enabled)
            c2d.destroy();

        enabled = false;
    }

    return status;
}

OMX_BUFFERHEADERTYPE* omx_vdec::allocate_color_convert_buf::get_il_buf_hdr()
{
    if (!omx) {
        DEBUG_PRINT_ERROR("Invalid param get_buf_hdr");
        return NULL;
    }

    if (!enabled)
        return omx->m_out_mem_ptr;

    return m_out_mem_ptr_client;
}

    OMX_BUFFERHEADERTYPE* omx_vdec::allocate_color_convert_buf::get_il_buf_hdr
(OMX_BUFFERHEADERTYPE *bufadd)
{
    if (!omx) {
        DEBUG_PRINT_ERROR("Invalid param get_buf_hdr");
        return NULL;
    }

    if (!enabled)
        return bufadd;

    unsigned index = 0;
    index = bufadd - omx->m_out_mem_ptr;

    if (index < omx->drv_ctx.op_buf.actualcount) {
        m_out_mem_ptr_client[index].nFlags = (bufadd->nFlags & OMX_BUFFERFLAG_EOS);
        m_out_mem_ptr_client[index].nTimeStamp = bufadd->nTimeStamp;
        bool status;

        if (!omx->in_reconfig && !omx->output_flush_progress) {
            status = c2d.convert(omx->drv_ctx.ptr_outputbuffer[index].pmem_fd,
                    bufadd->pBuffer,pmem_fd[index],pmem_baseaddress[index]);
            m_out_mem_ptr_client[index].nFilledLen = buffer_size_req;

            if (!status) {
                DEBUG_PRINT_ERROR("Failed color conversion %d", status);
                return NULL;
            }
        } else
            m_out_mem_ptr_client[index].nFilledLen = 0;

        return &m_out_mem_ptr_client[index];
    }

    DEBUG_PRINT_ERROR("Index messed up in the get_il_buf_hdr");
    return NULL;
}

    OMX_BUFFERHEADERTYPE* omx_vdec::allocate_color_convert_buf::get_dr_buf_hdr
(OMX_BUFFERHEADERTYPE *bufadd)
{
    if (!omx) {
        DEBUG_PRINT_ERROR("Invalid param get_buf_hdr");
        return NULL;
    }

    if (!enabled)
        return bufadd;

    unsigned index = 0;
    index = bufadd - m_out_mem_ptr_client;

    if (index < omx->drv_ctx.op_buf.actualcount) {
        return &omx->m_out_mem_ptr[index];
    }

    DEBUG_PRINT_ERROR("Index messed up in the get_dr_buf_hdr");
    return NULL;
}
    bool omx_vdec::allocate_color_convert_buf::get_buffer_req
(unsigned int &buffer_size)
{
    if (!enabled)
        buffer_size = omx->drv_ctx.op_buf.buffer_size;
    else if (!c2d.get_buffer_size(C2D_OUTPUT,buffer_size)) {
        DEBUG_PRINT_ERROR("Get buffer size failed");
        return false;
    }

    if (buffer_size < omx->drv_ctx.op_buf.buffer_size)
        buffer_size = omx->drv_ctx.op_buf.buffer_size;

    if (buffer_alignment_req < omx->drv_ctx.op_buf.alignment)
        buffer_alignment_req = omx->drv_ctx.op_buf.alignment;

    return true;
}
OMX_ERRORTYPE omx_vdec::allocate_color_convert_buf::free_output_buffer(
        OMX_BUFFERHEADERTYPE *bufhdr)
{
    unsigned int index = 0;

    if (!enabled)
        return omx->free_output_buffer(bufhdr);

    if (enabled && omx->is_component_secure())
        return OMX_ErrorNone;

    if (!allocated_count || !bufhdr) {
        DEBUG_PRINT_ERROR("Color convert no buffer to be freed %p",bufhdr);
        return OMX_ErrorBadParameter;
    }

    index = bufhdr - m_out_mem_ptr_client;

    if (index >= omx->drv_ctx.op_buf.actualcount) {
        DEBUG_PRINT_ERROR("Incorrect index color convert free_output_buffer");
        return OMX_ErrorBadParameter;
    }

    if (pmem_fd[index] > 0) {
        munmap(pmem_baseaddress[index], buffer_size_req);
        close(pmem_fd[index]);
    }

    pmem_fd[index] = -1;
    omx->free_ion_memory(&op_buf_ion_info[index]);
    m_heap_ptr[index].video_heap_ptr = NULL;

    if (allocated_count > 0)
        allocated_count--;
    else
        allocated_count = 0;

    if (!allocated_count) {
        c2d.close();
        init_members();
    }

    return omx->free_output_buffer(&omx->m_out_mem_ptr[index]);
}

OMX_ERRORTYPE omx_vdec::allocate_color_convert_buf::allocate_buffers_color_convert(OMX_HANDLETYPE hComp,
        OMX_BUFFERHEADERTYPE **bufferHdr,OMX_U32 port,OMX_PTR appData,OMX_U32 bytes)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;

    if (!enabled) {
        eRet = omx->allocate_output_buffer(hComp,bufferHdr,port,appData,bytes);
        return eRet;
    }

    if (enabled && omx->is_component_secure()) {
        DEBUG_PRINT_ERROR("Notin color convert mode secure_mode %d",
                omx->is_component_secure());
        return OMX_ErrorUnsupportedSetting;
    }

    if (!bufferHdr || bytes > buffer_size_req) {
        DEBUG_PRINT_ERROR("Invalid params allocate_buffers_color_convert %p", bufferHdr);
        DEBUG_PRINT_ERROR("color_convert buffer_size_req %d bytes %d",
                buffer_size_req,bytes);
        return OMX_ErrorBadParameter;
    }

    if (allocated_count >= omx->drv_ctx.op_buf.actualcount) {
        DEBUG_PRINT_ERROR("Actual count err in allocate_buffers_color_convert");
        return OMX_ErrorInsufficientResources;
    }

    OMX_BUFFERHEADERTYPE *temp_bufferHdr = NULL;
    eRet = omx->allocate_output_buffer(hComp,&temp_bufferHdr,
            port,appData,omx->drv_ctx.op_buf.buffer_size);

    if (eRet != OMX_ErrorNone || !temp_bufferHdr) {
        DEBUG_PRINT_ERROR("Buffer allocation failed color_convert");
        return eRet;
    }

    if ((temp_bufferHdr - omx->m_out_mem_ptr) >=
            omx->drv_ctx.op_buf.actualcount) {
        DEBUG_PRINT_ERROR("Invalid header index %d",
                (temp_bufferHdr - omx->m_out_mem_ptr));
        return OMX_ErrorUndefined;
    }

    unsigned int i = allocated_count;
    op_buf_ion_info[i].ion_device_fd = omx->alloc_map_ion_memory(
            buffer_size_req,buffer_alignment_req,
            &op_buf_ion_info[i].ion_alloc_data,&op_buf_ion_info[i].fd_ion_data,
            ION_FLAG_CACHED);

    pmem_fd[i] = op_buf_ion_info[i].fd_ion_data.fd;

    if (op_buf_ion_info[i].ion_device_fd < 0) {
        DEBUG_PRINT_ERROR("alloc_map_ion failed in color_convert");
        return OMX_ErrorInsufficientResources;
    }

    pmem_baseaddress[i] = (unsigned char *)mmap(NULL,buffer_size_req,
            PROT_READ|PROT_WRITE,MAP_SHARED,pmem_fd[i],0);

    if (pmem_baseaddress[i] == MAP_FAILED) {
        DEBUG_PRINT_ERROR("MMAP failed for Size %d",buffer_size_req);
        close(pmem_fd[i]);
        omx->free_ion_memory(&op_buf_ion_info[i]);
        return OMX_ErrorInsufficientResources;
    }

    m_heap_ptr[i].video_heap_ptr = new VideoHeap (
            op_buf_ion_info[i].ion_device_fd,buffer_size_req,
            pmem_baseaddress[i],op_buf_ion_info[i].ion_alloc_data.handle,pmem_fd[i]);
    m_pmem_info_client[i].pmem_fd = (OMX_U32)m_heap_ptr[i].video_heap_ptr.get();
    m_pmem_info_client[i].offset = 0;
    m_platform_entry_client[i].entry = (void *)&m_pmem_info_client[i];
    m_platform_entry_client[i].type = OMX_QCOM_PLATFORM_PRIVATE_PMEM;
    m_platform_list_client[i].nEntries = 1;
    m_platform_list_client[i].entryList = &m_platform_entry_client[i];
    m_out_mem_ptr_client[i].pOutputPortPrivate = NULL;
    m_out_mem_ptr_client[i].nAllocLen = buffer_size_req;
    m_out_mem_ptr_client[i].nFilledLen = 0;
    m_out_mem_ptr_client[i].nFlags = 0;
    m_out_mem_ptr_client[i].nOutputPortIndex = OMX_CORE_OUTPUT_PORT_INDEX;
    m_out_mem_ptr_client[i].nSize = sizeof(OMX_BUFFERHEADERTYPE);
    m_out_mem_ptr_client[i].nVersion.nVersion = OMX_SPEC_VERSION;
    m_out_mem_ptr_client[i].pPlatformPrivate = &m_platform_list_client[i];
    m_out_mem_ptr_client[i].pBuffer = pmem_baseaddress[i];
    m_out_mem_ptr_client[i].pAppPrivate = appData;
    *bufferHdr = &m_out_mem_ptr_client[i];
    DEBUG_PRINT_ERROR("IL client buffer header %p", *bufferHdr);
    allocated_count++;
    return eRet;
}

bool omx_vdec::is_component_secure()
{
    return secure_mode;
}

bool omx_vdec::allocate_color_convert_buf::get_color_format(OMX_COLOR_FORMATTYPE &dest_color_format)
{
    bool status = true;

    if (!enabled) {
        if (omx->drv_ctx.output_format == VDEC_YUV_FORMAT_TILE_4x2)
            dest_color_format =  (OMX_COLOR_FORMATTYPE)
                QOMX_COLOR_FormatYUV420PackedSemiPlanar64x32Tile2m8ka;
        else if (omx->drv_ctx.output_format == VDEC_YUV_FORMAT_NV12)
            dest_color_format = OMX_COLOR_FormatYUV420SemiPlanar;
        else
            status = false;
    } else {
        if (ColorFormat != OMX_COLOR_FormatYUV420Planar) {
            status = false;
        } else
            dest_color_format = OMX_COLOR_FormatYUV420Planar;
    }

    return status;
}
